class HST_MissionConvoyAdmissionResult
{
	bool m_bSuccess;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
}

class HST_MissionConvoyOperationService
{
	static const int EXACT_CONTRACT_VERSION = 1;
	static const int QUARANTINED_CONTRACT_VERSION = -52;
	static const int EXACT_ROUTE_VERSION = 1;
	static const int EXACT_VEHICLE_COUNT = 3;
	static const float EXACT_SPEED_METERS_PER_SECOND = 9.0;
	static const float EXACT_FORMATION_SPACING_METERS = 22.0;
	static const float EXACT_MATERIALIZE_IN_RADIUS_METERS = 1800.0;
	static const float EXACT_MATERIALIZE_OUT_RADIUS_METERS = 2200.0;
	static const float EXACT_ARRIVAL_RADIUS_METERS = 50.0;
	static const int EXACT_ARRIVAL_CONFIRMATION_SAMPLES = 2;
	static const int EXACT_PHYSICAL_MINIMUM_SECONDS = 60;
	static const int EXACT_MATERIALIZATION_TIMEOUT_SECONDS = 180;
	static const int EXACT_MAX_VIRTUAL_CATCHUP_SECONDS = 900;
	static const string EXACT_FORCE_KIND = "mission_convoy";
	static const string EXACT_POLICY_ID = "exact_mission_convoy_v1";
	static const string EXACT_ASSIGNMENT_KIND = "interdictable_delivery";
	static const string EXACT_SETTLEMENT_POLICY = "mission_convoy_outcome_once";
	static const string EXACT_RECALL_POLICY = "no_recall";
	static const string CONVOY_PRIMITIVE = "convoy_intercept";
	static const string EXACT_RUNTIME_TYPE = "convoy_route_intercept";
	static const string CONVOY_STAGING = "convoy_staging";
	static const string CONVOY_MOVING = "convoy_moving";
	static const string CONVOY_CONTACT = "convoy_contact";
	static const string CONVOY_ELIMINATED = "convoy_eliminated";
	static const string CONVOY_FAILED = "failed";
	static const string CONVOY_COMPLETE_EVENT = "convoy_complete";
	static const string CONVOY_FAIL_EVENT = "convoy_failed";
	static const string VEHICLE_ROLE = "convoy_vehicle";
	static const string PAYLOAD_ROLE = "convoy_payload";
	static const string CAPTIVE_ROLE = "convoy_captive";
	static const string CARGO_KIND = "cargo";
	static const string CAPTIVE_KIND = "captive";

	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();
	protected ref HST_ForceCatalogService m_Catalog = new HST_ForceCatalogService();
	protected ref HST_PhysicalWarService m_PhysicalWar;
	protected ref HST_MissionRuntimeService m_MissionRuntime;

	void SetRuntimeServices(HST_PhysicalWarService physicalWar, HST_MissionRuntimeService missionRuntime = null)
	{
		m_PhysicalWar = physicalWar;
		m_MissionRuntime = missionRuntime;
	}

	static bool IsExactMission(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sRuntimePrimitive == CONVOY_PRIMITIVE
			&& mission.m_iOperationContractVersion == EXACT_CONTRACT_VERSION;
	}

	static string ValidateElementDispositionAssetState(
		HST_ConvoyElementState element,
		HST_MissionAssetState asset,
		bool openAuthority)
	{
		if (!element || !asset)
			return "convoy element or vehicle asset is missing";

		bool capturedAsset = !asset.m_bDestroyed
			&& (asset.m_bDelivered || asset.m_sLastInteraction == "captured");
		bool unresolvedAsset = !asset.m_bDestroyed && !capturedAsset
			&& asset.m_sLastInteraction != "retired";
		if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE)
		{
			if (!unresolvedAsset || !element.m_bMobile || element.m_iSurvivingCrewCount <= 0
				|| element.m_fVehicleDamageFraction >= 1.0)
				return "active element conflicts with resolved vehicle authority";
			return "";
		}
		if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED)
		{
			if (!unresolvedAsset || element.m_bMobile || element.m_iSurvivingCrewCount != 0
				|| element.m_fVehicleDamageFraction >= 1.0)
				return "abandoned element conflicts with crew or vehicle authority";
			return "";
		}
		if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED)
		{
			if (!asset.m_bDestroyed || element.m_bMobile || element.m_fVehicleDamageFraction < 1.0)
				return "destroyed element conflicts with vehicle authority";
			return "";
		}
		if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED)
		{
			if (!capturedAsset || element.m_bMobile || element.m_fVehicleDamageFraction >= 1.0)
				return "captured element conflicts with vehicle authority";
			return "";
		}
		if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED)
		{
			if (!unresolvedAsset || element.m_bMobile || element.m_iSurvivingCrewCount <= 0
				|| element.m_fVehicleDamageFraction >= 1.0)
				return "arrived element conflicts with vehicle or crew authority";
			return "";
		}
		if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_RETIRED)
		{
			if (openAuthority || element.m_bMobile)
				return "retired element remains inside open or mobile authority";
			return "";
		}
		return "element disposition is unknown";
	}

	bool ShouldDeferGenericMissionCompletion(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		return IsExactMission(mission) && HasPendingRecoveryOutcome(state, mission);
	}

	bool PrepareNewMissionContract(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;
		if (mission.m_iOperationContractVersion == EXACT_CONTRACT_VERSION)
			return false;

		mission.m_iOperationContractVersion = EXACT_CONTRACT_VERSION;
		return true;
	}

	HST_MissionConvoyAdmissionResult AdmitNewMission(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_ActiveMissionState mission)
	{
		HST_MissionConvoyAdmissionResult result = new HST_MissionConvoyAdmissionResult();
		string failure = ValidateAdmissionPreflight(state, preset, mission);
		if (!failure.IsEmpty())
		{
			result.m_sFailureReason = failure;
			ResetUncommittedAdmissionContract(state, mission);
			FailExactMissionContract(state, mission, failure);
			result.m_bStateChanged = true;
			return result;
		}

		mission.m_sOperationId = BuildOperationId(mission);
		mission.m_sManifestId = BuildManifestId(mission);
		mission.m_sSpawnResultId = BuildSpawnResultId(mission);
		if (!m_PhysicalWar.PrepareExactMissionConvoyDurableGroups(state, preset, mission))
		{
			failure = "exact mission convoy durable groups could not be prepared";
			RollbackUncommittedAdmission(state, mission);
			result.m_sFailureReason = failure;
			FailExactMissionContract(state, mission, failure);
			result.m_bStateChanged = true;
			return result;
		}

		failure = ValidatePreparedCrewCatalog(state, mission);
		if (!failure.IsEmpty())
		{
			RollbackUncommittedAdmission(state, mission);
			result.m_sFailureReason = failure;
			FailExactMissionContract(state, mission, failure);
			result.m_bStateChanged = true;
			return result;
		}

		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		ref array<vector> routePositions = BuildRoutePositions(route, ResolveVehicleAsset(state, mission, 0));
		float routeDistance = CalculateRouteDistance(routePositions);
		if (routePositions.Count() < 2 || routeDistance <= EXACT_ARRIVAL_RADIUS_METERS)
		{
			failure = "exact mission convoy road route is missing or too short";
			RollbackUncommittedAdmission(state, mission);
			result.m_sFailureReason = failure;
			FailExactMissionContract(state, mission, failure);
			result.m_bStateChanged = true;
			return result;
		}

		HST_ForceManifestState manifest = BuildManifest(state, mission, route);
		HST_OperationRecordState operation = BuildOperation(state, mission, manifest, route, routePositions, routeDistance);
		HST_ForceSpawnResultState batch = BuildHeldRosterBatch(state, mission, manifest);
		ref array<ref HST_ConvoyElementState> elements = BuildElements(state, mission, manifest);
		failure = ValidatePreparedAuthority(state, mission, manifest, operation, batch, elements);
		if (!failure.IsEmpty())
		{
			RollbackUncommittedAdmission(state, mission);
			result.m_sFailureReason = failure;
			FailExactMissionContract(state, mission, failure);
			result.m_bStateChanged = true;
			return result;
		}

		state.m_aForceManifests.Insert(manifest);
		state.m_aOperations.Insert(operation);
		state.m_aForceSpawnResults.Insert(batch);
		foreach (HST_ConvoyElementState element : elements)
			state.m_aConvoyElements.Insert(element);
		LinkPreparedAuthority(state, mission, manifest, operation, batch, elements);

		result.m_bSuccess = true;
		result.m_bStateChanged = true;
		result.m_Operation = operation;
		result.m_Manifest = manifest;
		result.m_Batch = batch;
		Print(string.Format("Partisan exact mission convoy | admitted %1 | operation %2 | manifest %3 | vehicles %4 | route %5m", mission.m_sInstanceId, operation.m_sOperationId, manifest.m_sManifestId, manifest.m_iAcceptedVehicleCount, Math.Round(routeDistance)));
		return result;
	}

	bool TickBeforePhysical(HST_CampaignState state, HST_CampaignPreset preset, int elapsedSeconds)
	{
		if (!state || !m_PhysicalWar)
			return false;

		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMission(mission))
				continue;

			HST_OperationRecordState operation;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			string failure = ResolveAuthority(state, mission, operation, manifest, batch);
			if (!failure.IsEmpty())
			{
				if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				{
					changed = QuarantineAmbiguousAuthority(state, mission, failure) || changed;
					changed = FailExactMissionContract(state, mission, failure) || changed;
				}
				continue;
			}

			if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				continue;
			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (IsTerminalRuntimePhase(mission) && !IsRecoveryHold(mission))
				continue;
			changed = NormalizeRestoredProjection(state, mission, operation) || changed;
			if (mission.m_sRuntimePhase == CONVOY_FAILED)
				continue;
			if (CountLivingCrews(state, mission) <= 0)
			{
				changed = MarkAllCrewsEliminated(state, mission) || changed;
				if (HasPendingRecoveryOutcome(state, mission))
				{
					changed = EnterRecoveryHold(state, mission, operation) || changed;
					changed = TryBeginMaterialization(state, mission, operation, true) || changed;
				}
				continue;
			}

			if (mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == "created" || mission.m_sRuntimePhase == "active" || mission.m_sRuntimePhase == "convoy_static")
			{
				mission.m_sRuntimePhase = CONVOY_STAGING;
				changed = true;
			}

			if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING
				&& mission.m_iRuntimeCounterA >= mission.m_iRuntimeCounterB)
			{
				operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
				operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
				operation.m_iDutyStateEnteredAtSecond = state.m_iElapsedSeconds;
				operation.m_iStrategicLastUpdateSecond = state.m_iElapsedSeconds;
				operation.m_iRevision++;
				mission.m_sRuntimePhase = CONVOY_MOVING;
				mission.m_sLastRuntimeEventKey = "convoy_moving_pending";
				changed = true;
			}

			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
			{
				changed = AdvanceVirtualRoute(state, mission, operation, elapsedSeconds) || changed;
				if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN || mission.m_sRuntimePhase == CONVOY_FAILED)
					continue;

				changed = TryBeginMaterialization(state, mission, operation, false) || changed;
			}
		}

		return changed;
	}

	bool TickAfterPhysical(HST_CampaignState state)
	{
		if (!state || !m_PhysicalWar)
			return false;

		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMission(mission))
				continue;
			HST_OperationRecordState operation;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			string authorityFailure = ResolveAuthority(state, mission, operation, manifest, batch);
			if (!authorityFailure.IsEmpty())
			{
				if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				{
					changed = QuarantineAmbiguousAuthority(state, mission, authorityFailure) || changed;
					changed = FailExactMissionContract(state, mission, authorityFailure) || changed;
				}
				continue;
			}

			if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			{
				if (!m_PhysicalWar.HasExactMissionConvoyRuntime(mission)
					&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
				{
					operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
					operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
					operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
					operation.m_iRevision++;
					changed = true;
				}
				continue;
			}

			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				&& CountLivingCrews(state, mission) <= 0 && !HasPendingRecoveryOutcome(state, mission))
			{
				changed = MarkAllCrewsEliminated(state, mission) || changed;
				continue;
			}

			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				&& IsRecoveryHold(mission)
				&& !m_PhysicalWar.IsExactMissionConvoyRecoveryProjectionReady(state, mission))
			{
				string recoveryMaterializationReason;
				if (m_PhysicalWar.MaterializeExactMissionConvoyRecoveryVehicles(state, mission, recoveryMaterializationReason))
					changed = true;
				else if (!recoveryMaterializationReason.IsEmpty()
					&& operation.m_sLastProjectionReason != recoveryMaterializationReason)
				{
					operation.m_sLastProjectionReason = recoveryMaterializationReason;
					operation.m_iRevision++;
					changed = true;
				}
			}

			// A real casualty or terminal vehicle transition can occur after the first
			// exact root spawns but before the remaining roots are ready.  Reconcile
			// only the runtime evidence PhysicalWar has already published so readiness
			// compares against the current durable roster instead of timing out on the
			// admission snapshot.
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				&& !IsRecoveryHold(mission)
				&& m_PhysicalWar.HasExactMissionConvoyRuntime(mission))
				changed = SyncPhysicalProjection(state, mission, operation) || changed;

			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				&& IsPhysicalProjectionReady(state, mission))
			{
				operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
				operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
				operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
				operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
				if (IsRecoveryHold(mission))
					operation.m_sLastProjectionReason = "all eligible exact convoy recovery vehicles are physical";
				else
					operation.m_sLastProjectionReason = "every required surviving exact convoy vehicle/crew root is physical";
				operation.m_iRevision++;
				changed = SetElementsPhysicalized(state, mission, true) || changed;
			}
			else if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				&& state.m_iElapsedSeconds >= operation.m_iMaterializationStateEnteredAtSecond + EXACT_MATERIALIZATION_TIMEOUT_SECONDS)
			{
				if (IsRecoveryHold(mission) && !HasMaterializableRecoveryRoot(state, mission))
				{
					operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
					operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
					operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
					operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
					operation.m_sLastProjectionReason = "recovery has no unresolved materializable vehicle or ground-cargo root";
					operation.m_iRevision++;
					changed = SetElementsPhysicalized(state, mission, false) || changed;
					changed = true;
					continue;
				}
				changed = FailExactMissionContract(state, mission, "all-or-nothing convoy materialization did not confirm every required surviving crew/vehicle root before timeout") || changed;
				continue;
			}

			if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
				continue;
			if (!IsRecoveryHold(mission) && m_PhysicalWar.IsExactMissionConvoyOutboundProjectionTransactionOpen(mission))
			{
				string publicationFailure;
				if (!m_PhysicalWar.CommitExactMissionConvoyOutboundProjectionTransaction(state, mission, m_MissionRuntime, publicationFailure))
				{
					if (!publicationFailure.IsEmpty())
						Print(string.Format("Partisan exact mission convoy | outbound publication failed %1 | %2", mission.m_sInstanceId, publicationFailure), LogLevel.WARNING);
					changed = true;
					continue;
				}
				changed = true;
			}

			changed = SyncPhysicalProjection(state, mission, operation) || changed;
			if (CountLivingCrews(state, mission) <= 0)
			{
				changed = MarkAllCrewsEliminated(state, mission) || changed;
				if (HasPendingRecoveryOutcome(state, mission))
					changed = EnterRecoveryHold(state, mission, operation) || changed;
				else
					continue;
			}
			if (mission.m_sRuntimePhase == CONVOY_CONTACT)
			{
				if (operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT)
				{
					operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT;
					operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
					operation.m_iLastContactAtSecond = state.m_iElapsedSeconds;
					operation.m_iRevision++;
					changed = true;
				}
			}
			else if (operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
			{
				operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
				operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
				operation.m_iRevision++;
				changed = true;
			}

			if (TryConfirmPhysicalArrival(state, mission, operation))
			{
				changed = true;
				continue;
			}

			float nearestPlayer = ResolveNearestLivingPlayerDistanceForConvoy(state, mission, operation);
			bool minimumPhysicalTimeElapsed = state.m_iElapsedSeconds >= operation.m_iMaterializationStateEnteredAtSecond + EXACT_PHYSICAL_MINIMUM_SECONDS;
			bool movingFold = mission.m_sRuntimePhase == CONVOY_MOVING
				&& operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
			bool recoveryFold = IsRecoveryHold(mission)
				&& operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
				&& HasPendingRecoveryOutcome(state, mission);
			if ((movingFold || recoveryFold)
				&& operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR
				&& minimumPhysicalTimeElapsed
				&& (nearestPlayer < 0 || nearestPlayer >= EXACT_MATERIALIZE_OUT_RADIUS_METERS))
			{
				string foldReason;
				operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING;
				operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
				if (m_PhysicalWar.FoldExactMissionConvoyRuntime(state, mission, foldReason))
				{
					operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
					operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
					operation.m_iStrategicLastUpdateSecond = state.m_iElapsedSeconds;
					operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
					operation.m_sLastProjectionReason = "exact convoy folded outside player bubble: " + foldReason;
					operation.m_iRevision++;
					changed = SetElementsPhysicalized(state, mission, false) || changed;
				}
				else
				{
					operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
					operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
					operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
					operation.m_sLastProjectionReason = "exact convoy fold deferred: " + foldReason;
					operation.m_iRevision++;
				}
				changed = true;
			}
		}

		return changed;
	}

	bool TickAfterOutcomes(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		array<string> reconciledMissionIds = {};
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMission(mission) || mission.m_sInstanceId.IsEmpty()
				|| reconciledMissionIds.Contains(mission.m_sInstanceId))
				continue;
			reconciledMissionIds.Insert(mission.m_sInstanceId);
			changed = ReconcileMissionOutcomeForInstance(state, mission.m_sInstanceId) || changed;
		}

		return changed;
	}

	bool ReconcileMissionOutcomeForInstance(HST_CampaignState state, string missionInstanceId)
	{
		HST_ActiveMissionState mission = ResolveCanonicalExactMissionForInstance(state, missionInstanceId);
		if (!mission)
			return false;

		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		string authorityFailure = ResolveAuthority(state, mission, operation, manifest, batch);
		if (!authorityFailure.IsEmpty())
		{
			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				return false;
			bool authorityChanged = QuarantineAmbiguousAuthority(state, mission, authorityFailure);
			return FailExactMissionContract(state, mission, authorityFailure) || authorityChanged;
		}
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			return false;

		HST_EOperationTerminalResult terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN;
		string settlementKind;
		string reason;
		bool durableArrival = HasDurableArrivalEvidence(state, mission, operation);
		if (durableArrival && !mission.m_bConvoyArrivalOutcomeApplied)
		{
			// Arrival and its strategic consequence are two adjacent durable
			// transitions.  A save may land between them; keep the operation open
			// until the once-only outcome owner records its receipt.
			return false;
		}
		if (mission.m_bConvoyArrivalOutcomeApplied && durableArrival)
		{
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_COMPLETED;
			settlementKind = "arrived";
			reason = "convoy reached destination with living crew";
		}
		else if (mission.m_bConvoyCrewEliminatedOutcomeApplied && (mission.m_sRuntimePhase == CONVOY_ELIMINATED || mission.m_sLastRuntimeEventKey == CONVOY_COMPLETE_EVENT))
		{
			if (HasPendingRecoveryOutcome(state, mission))
				return EnterRecoveryHold(state, mission, operation);
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
			settlementKind = "crew_eliminated";
			reason = "all exact convoy crews eliminated";
		}
		else if (mission.m_bConvoyExpiredOutcomeApplied || mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
		{
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED;
			settlementKind = "expired";
			reason = "convoy mission expired";
		}
		else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
		{
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED;
			settlementKind = "mission_succeeded";
			reason = "convoy mission succeeded";
		}
		else if (mission.m_sRuntimePhase == CONVOY_FAILED || mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED)
		{
			terminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_ROUTE_FAILED;
			settlementKind = "failed";
			reason = mission.m_sRuntimeFailureReason;
		}

		if (terminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN)
			return false;

		return SettleOperation(state, mission, operation, terminalResult, settlementKind, reason);
	}

	protected bool HasDurableArrivalEvidence(HST_CampaignState state, HST_ActiveMissionState mission, HST_OperationRecordState operation)
	{
		if (!state || !mission || !operation || mission.m_sRuntimePhase != CONVOY_FAILED
			|| mission.m_sLastRuntimeEventKey != CONVOY_FAIL_EVENT
			|| !mission.m_sRuntimeFailureReason.Contains("Convoy reached its destination:"))
			return false;
		if (operation.m_fRouteTotalDistanceMeters <= 0.0
			|| operation.m_fRouteProgressMeters < operation.m_fRouteTotalDistanceMeters - EXACT_ARRIVAL_RADIUS_METERS)
			return false;

		int arrivedElements;
		for (int ordinal = 0; ordinal < EXACT_VEHICLE_COUNT; ordinal++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, ordinal));
			if (!element)
				return false;
			if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE)
				return false;
			if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED)
				arrivedElements++;
		}
		return arrivedElements > 0;
	}

	bool ReconcileAfterRestore(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMission(mission))
				continue;

			HST_OperationRecordState operation;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			string failure = ResolveAuthority(state, mission, operation, manifest, batch);
			if (!failure.IsEmpty())
			{
				if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				{
					changed = QuarantineAmbiguousAuthority(state, mission, "restore authority conflict: " + failure) || changed;
					changed = FailExactMissionContract(state, mission, "restore authority conflict: " + failure) || changed;
				}
				continue;
			}
			if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				continue;
			bool pendingArrivalOutcome = HasDurableArrivalEvidence(state, mission, operation)
				&& !mission.m_bConvoyArrivalOutcomeApplied;
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
				&& (!IsTerminalRuntimePhase(mission) || IsRecoveryHold(mission) || pendingArrivalOutcome))
				changed = NormalizeRestoredProjection(state, mission, operation) || changed;
		}

		changed = TickAfterOutcomes(state) || changed;
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			changed = SettleOpenOperationsForCampaignStop(state, "restored campaign phase does not permit active convoy operations") || changed;
		changed = ReconcileSettledRuntimeCleanup(state) || changed;
		return changed;
	}

	bool SettleOpenOperationsForCampaignStop(HST_CampaignState state, string reason)
	{
		if (!state)
			return false;
		if (reason.IsEmpty())
			reason = "campaign phase no longer permits an active mission convoy";

		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMission(mission))
				continue;
			HST_OperationRecordState operation;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			string authorityFailure = ResolveAuthority(state, mission, operation, manifest, batch);
			if (!authorityFailure.IsEmpty())
			{
				if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				{
					changed = QuarantineAmbiguousAuthority(state, mission, "campaign-stop authority conflict: " + authorityFailure) || changed;
					changed = FailExactMissionContract(state, mission, "campaign-stop authority conflict: " + authorityFailure) || changed;
				}
				continue;
			}
			if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				continue;

			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			{
				mission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
				mission.m_sRuntimePhase = "expired";
				mission.m_sRuntimeFailureReason = reason;
				mission.m_sLastRuntimeEventKey = "convoy_expired";
				changed = true;
			}
			changed = SettleOperation(
				state,
				mission,
				operation,
				HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED,
				"campaign_stopped",
				reason) || changed;
		}
		return changed;
	}

	bool ReconcileSettledRuntimeCleanup(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		if (m_PhysicalWar)
			changed = m_PhysicalWar.ReconcileInactiveMissionConvoyRuntime(state) || changed;
		array<string> reconciledMissionIds = {};
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMission(mission) || mission.m_sInstanceId.IsEmpty()
				|| reconciledMissionIds.Contains(mission.m_sInstanceId))
				continue;
			reconciledMissionIds.Insert(mission.m_sInstanceId);
			changed = ReconcileSettledRuntimeCleanupForInstance(state, mission.m_sInstanceId) || changed;
		}
		return changed;
	}

	bool ReconcileSettledRuntimeCleanupForInstance(HST_CampaignState state, string missionInstanceId)
	{
		HST_ActiveMissionState mission = ResolveCanonicalExactMissionForInstance(state, missionInstanceId);
		if (!mission || mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE || !m_PhysicalWar)
			return false;

		HST_OperationRecordState operation;
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch;
		if (!ResolveAuthority(state, mission, operation, manifest, batch).IsEmpty()
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return false;

		bool runtimeRetired;
		bool changed = m_PhysicalWar.ReconcileInactiveMissionConvoyRuntimeForInstance(
			state,
			mission.m_sInstanceId,
			runtimeRetired);
		if (!runtimeRetired)
			return changed;

		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC)
		{
			operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
			operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
			operation.m_iRevision++;
			changed = true;
		}
		changed = SetElementsPhysicalized(state, mission, false) || changed;
		if (mission.m_bRuntimeSpawned)
		{
			mission.m_bRuntimeSpawned = false;
			changed = true;
		}
		if (!mission.m_bRuntimeCleanupComplete)
		{
			mission.m_bRuntimeCleanupComplete = true;
			changed = true;
		}
		return changed;
	}

	protected string ValidateAdmissionPreflight(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !preset || !mission || !m_PhysicalWar)
			return "exact mission convoy admission services are unavailable";
		if (!IsExactMission(mission) || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return "mission is not an active exact convoy contract";
		if (!mission.m_sOperationId.IsEmpty() || !mission.m_sManifestId.IsEmpty() || !mission.m_sSpawnResultId.IsEmpty() || !mission.m_sSettlementId.IsEmpty())
			return "new exact mission convoy already contains authority or settlement links";
		if (state.CountMissionAssets(mission.m_sInstanceId, VEHICLE_ROLE) != EXACT_VEHICLE_COUNT)
			return string.Format("exact mission convoy requires exactly %1 vehicle assets", EXACT_VEHICLE_COUNT);
		if (!ResolveMissionRoute(state, mission))
			return "exact mission convoy requires one persisted generated road route";
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_MissionAssetState asset = ResolveVehicleAsset(state, mission, i);
			if (!asset || asset.m_sPrefab.IsEmpty() || IsZeroVector(asset.m_vSourcePosition) || IsZeroVector(asset.m_vTargetPosition))
				return string.Format("exact mission convoy vehicle asset %1 is incomplete", i);
		}
		string cargoFailure = HST_MissionConvoyP1Policy.ValidateAdmissionCargo(state, mission);
		if (!cargoFailure.IsEmpty())
			return cargoFailure;
		string operationId = BuildOperationId(mission);
		string manifestId = BuildManifestId(mission);
		string spawnResultId = BuildSpawnResultId(mission);
		if (state.FindOperation(operationId) || state.FindForceManifest(manifestId) || state.FindForceSpawnResult(spawnResultId))
			return "exact mission convoy deterministic authority identity already exists";
		foreach (HST_ConvoyElementState element : state.m_aConvoyElements)
		{
			if (element && (element.m_sMissionInstanceId == mission.m_sInstanceId || element.m_sOperationId == operationId))
				return "exact mission convoy element authority already exists";
		}
		return "";
	}

	protected string ValidatePreparedCrewCatalog(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !m_Catalog)
			return "exact mission convoy crew catalog is unavailable";
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ActiveGroupState group = state.FindActiveGroup(BuildGroupId(mission, i));
			HST_ForceGroupCatalogEntry catalogGroup = ResolveCrewCatalogEntry(group);
			if (!group || !catalogGroup)
				return string.Format("exact mission convoy crew group %1 does not resolve to one frozen catalog entry", i);
			if (catalogGroup.m_aMemberSlots.Count() != group.m_iInfantryCount || catalogGroup.m_aMemberSlots.Count() <= 0)
				return string.Format("exact mission convoy crew group %1 catalog roster count conflicts with its durable count", i);
			foreach (HST_ForceGroupCatalogSlot memberSlot : catalogGroup.m_aMemberSlots)
			{
				if (!memberSlot || memberSlot.m_sSlotId.IsEmpty() || memberSlot.m_sPrefab.IsEmpty() || memberSlot.m_sRole.IsEmpty())
					return string.Format("exact mission convoy crew group %1 contains an incomplete character slot", i);
			}
		}
		return "";
	}

	protected HST_ForceGroupCatalogEntry ResolveCrewCatalogEntry(HST_ActiveGroupState group)
	{
		if (!group || !m_Catalog || group.m_sFactionKey.IsEmpty() || group.m_sPrefab.IsEmpty())
			return null;
		HST_ForceGroupCatalogEntry resolved;
		foreach (HST_ForceGroupCatalogEntry candidate : m_Catalog.BuildGroupCatalog(group.m_sFactionKey))
		{
			if (!candidate || (candidate.m_sExecutionPrefab != group.m_sPrefab && candidate.m_sAuthoredPrefab != group.m_sPrefab))
				continue;
			if (resolved)
				return null;
			resolved = candidate;
		}
		return resolved;
	}

	protected HST_ForceManifestState BuildManifest(HST_CampaignState state, HST_ActiveMissionState mission, HST_GeneratedRouteState route)
	{
		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = mission.m_sManifestId;
		manifest.m_sOperationId = mission.m_sOperationId;
		manifest.m_sCommandRequestId = mission.m_sInstanceId;
		manifest.m_sForceKind = EXACT_FORCE_KIND;
		manifest.m_sFactionRole = "mission_convoy";
		manifest.m_sIntentId = mission.m_sMissionId;
		manifest.m_sSourceZoneId = route.m_sSourceZoneId;
		manifest.m_sTargetZoneId = mission.m_sTargetZoneId;
		manifest.m_sCatalogVersion = HST_ForceCatalogService.CATALOG_VERSION;
		manifest.m_sPolicyId = EXACT_POLICY_ID;
		manifest.m_iRequestedVehicleCount = EXACT_VEHICLE_COUNT;
		manifest.m_iAcceptedVehicleCount = EXACT_VEHICLE_COUNT;
		manifest.m_iDeterministicSeed = BuildDeterministicSeed(state, mission);
		manifest.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		manifest.m_bFrozen = true;

		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_MissionAssetState vehicleAsset = ResolveVehicleAsset(state, mission, i);
			HST_ActiveGroupState group = state.FindActiveGroup(BuildGroupId(mission, i));
			HST_ForceGroupCatalogEntry catalogGroup = ResolveCrewCatalogEntry(group);
			if (i == 0 && group)
			{
				manifest.m_sFactionKey = group.m_sFactionKey;
				manifest.m_sGroupPrefab = group.m_sPrefab;
			}

			HST_ForceManifestGroupState groupSlot = new HST_ForceManifestGroupState();
			groupSlot.m_sElementId = BuildCrewGroupElementId(mission, i);
			groupSlot.m_sCatalogEntryId = catalogGroup.m_sEntryId;
			groupSlot.m_sPrefab = group.m_sPrefab;
			groupSlot.m_sRole = "convoy_crew";
			groupSlot.m_iOrdinal = i;
			groupSlot.m_iExpectedMemberCount = catalogGroup.m_aMemberSlots.Count();
			groupSlot.m_bRequired = true;
			manifest.m_aGroups.Insert(groupSlot);

			HST_ForceManifestVehicleState vehicleSlot = new HST_ForceManifestVehicleState();
			vehicleSlot.m_sSlotId = BuildVehicleSlotId(mission, i);
			vehicleSlot.m_sCatalogEntryId = vehicleAsset.m_sPrefab;
			vehicleSlot.m_sGroupElementId = groupSlot.m_sElementId;
			vehicleSlot.m_sPrefab = vehicleAsset.m_sPrefab;
			vehicleSlot.m_sRole = VEHICLE_ROLE;
			vehicleSlot.m_iOrdinal = i;
			vehicleSlot.m_iRequiredCrew = groupSlot.m_iExpectedMemberCount;
			vehicleSlot.m_bRequired = true;
			manifest.m_aVehicles.Insert(vehicleSlot);

			for (int crewIndex = 0; crewIndex < groupSlot.m_iExpectedMemberCount; crewIndex++)
			{
				HST_ForceGroupCatalogSlot catalogMember = catalogGroup.m_aMemberSlots[crewIndex];
				HST_ForceManifestMemberState member = new HST_ForceManifestMemberState();
				member.m_sSlotId = BuildMemberSlotId(mission, i, crewIndex);
				member.m_sCatalogSlotId = catalogMember.m_sSlotId;
				member.m_sGroupElementId = groupSlot.m_sElementId;
				member.m_sPrefab = catalogMember.m_sPrefab;
				member.m_sRole = catalogMember.m_sRole;
				member.m_sAssignedVehicleSlotId = vehicleSlot.m_sSlotId;
				member.m_sSeatRole = "passenger";
				member.m_iSeatIndex = crewIndex;
				if (crewIndex == 0)
				{
					member.m_sSeatRole = "driver";
					member.m_iSeatIndex = 0;
				}
				member.m_iOrdinal = manifest.m_aMembers.Count();
				member.m_iHRCost = 0;
				member.m_bRequired = true;
				manifest.m_aMembers.Insert(member);
			}
		}

		manifest.m_iRequestedMemberCount = manifest.m_aMembers.Count();
		manifest.m_iAcceptedMemberCount = manifest.m_aMembers.Count();
		HST_MissionAssetState cargoAsset = ResolveConvoyCargoAsset(state, mission);
		if (cargoAsset)
		{
			HST_ForceManifestAssetState assetSlot = new HST_ForceManifestAssetState();
			assetSlot.m_sSlotId = BuildCargoSlotId(mission, cargoAsset);
			assetSlot.m_sKind = cargoAsset.m_sKind;
			assetSlot.m_sPrefab = cargoAsset.m_sPrefab;
			assetSlot.m_sRole = cargoAsset.m_sRole;
			assetSlot.m_sAssignedVehicleSlotId = BuildVehicleSlotId(mission, 0);
			assetSlot.m_iQuantity = 1;
			assetSlot.m_iOrdinal = 0;
			assetSlot.m_bRequired = true;
			manifest.m_aAssets.Insert(assetSlot);
		}

		manifest.m_sManifestHash = m_Integrity.BuildManifestHash(manifest);
		return manifest;
	}

	protected HST_OperationRecordState BuildOperation(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_GeneratedRouteState route,
		array<vector> routePositions,
		float routeDistance)
	{
		HST_MissionAssetState leadAsset = ResolveVehicleAsset(state, mission, 0);
		HST_OperationRecordState operation = new HST_OperationRecordState();
		operation.m_sOperationId = mission.m_sOperationId;
		operation.m_eType = HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY;
		operation.m_iContractVersion = EXACT_CONTRACT_VERSION;
		operation.m_sOwnerFactionKey = manifest.m_sFactionKey;
		operation.m_sMissionInstanceId = mission.m_sInstanceId;
		operation.m_sIssueRequestId = mission.m_sInstanceId;
		operation.m_sManifestId = manifest.m_sManifestId;
		operation.m_sSpawnResultId = mission.m_sSpawnResultId;
		operation.m_sForceId = BuildForceId(mission);
		operation.m_sProjectionId = BuildProjectionId(mission);
		operation.m_sOriginZoneId = route.m_sSourceZoneId;
		operation.m_vOriginPosition = routePositions[0];
		operation.m_sAssignmentKind = EXACT_ASSIGNMENT_KIND;
		operation.m_sAssignmentZoneId = mission.m_sTargetZoneId;
		operation.m_vAssignmentPosition = routePositions[routePositions.Count() - 1];
		operation.m_sTacticalTargetZoneId = mission.m_sTargetZoneId;
		operation.m_vTacticalTargetPosition = operation.m_vAssignmentPosition;
		operation.m_vStrategicPosition = leadAsset.m_vCurrentPosition;
		operation.m_sCurrentRouteId = route.m_sRouteId;
		operation.m_sRouteContractHash = BuildRouteContractHash(route, routePositions);
		operation.m_iProjectionContractVersion = EXACT_CONTRACT_VERSION;
		operation.m_iRouteVersion = EXACT_ROUTE_VERSION;
		operation.m_vRouteStartPosition = routePositions[0];
		operation.m_vRouteEndPosition = routePositions[routePositions.Count() - 1];
		operation.m_fRouteTotalDistanceMeters = routeDistance;
		operation.m_fRouteProgressMeters = 0.0;
		operation.m_fStrategicSpeedMetersPerSecond = EXACT_SPEED_METERS_PER_SECOND;
		operation.m_iStrategicLastUpdateSecond = state.m_iElapsedSeconds;
		operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
		operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		operation.m_sLastProjectionReason = "exact convoy admitted in strategic hold";
		operation.m_sRecallPolicyId = EXACT_RECALL_POLICY;
		operation.m_sSettlementPolicyId = EXACT_SETTLEMENT_POLICY;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		operation.m_eTerminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
		operation.m_iDeterministicSeed = manifest.m_iDeterministicSeed;
		operation.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		operation.m_iDutyStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iRevision = 1;
		return operation;
	}

	protected HST_ForceSpawnResultState BuildHeldRosterBatch(HST_CampaignState state, HST_ActiveMissionState mission, HST_ForceManifestState manifest)
	{
		HST_ForceSpawnResultState batch = new HST_ForceSpawnResultState();
		batch.m_sResultId = mission.m_sSpawnResultId;
		batch.m_sRequestId = mission.m_sInstanceId;
		batch.m_sManifestId = manifest.m_sManifestId;
		batch.m_sManifestHash = manifest.m_sManifestHash;
		batch.m_sOperationId = mission.m_sOperationId;
		batch.m_sForceId = BuildForceId(mission);
		batch.m_sProjectionId = BuildProjectionId(mission);
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
		batch.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		batch.m_iUpdatedAtSecond = state.m_iElapsedSeconds;
		batch.m_iStrategicHoldSinceSecond = state.m_iElapsedSeconds;
		batch.m_iLifecycleRevision = 1;
		batch.m_iLastLifecycleSecond = state.m_iElapsedSeconds;
		batch.m_bStrategicProjectionHeld = true;

		foreach (HST_ForceManifestGroupState group : manifest.m_aGroups)
			batch.m_aSlotResults.Insert(BuildRosterSlot(group.m_sElementId, "group", group.m_sPrefab, state.m_iElapsedSeconds, false));
		foreach (HST_ForceManifestVehicleState vehicle : manifest.m_aVehicles)
			batch.m_aSlotResults.Insert(BuildRosterSlot(vehicle.m_sSlotId, "vehicle", vehicle.m_sPrefab, state.m_iElapsedSeconds, true));
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
			batch.m_aSlotResults.Insert(BuildRosterSlot(member.m_sSlotId, "member", member.m_sPrefab, state.m_iElapsedSeconds, true));
		foreach (HST_ForceManifestAssetState asset : manifest.m_aAssets)
			batch.m_aSlotResults.Insert(BuildRosterSlot(asset.m_sSlotId, "asset", asset.m_sPrefab, state.m_iElapsedSeconds, true));
		batch.m_iExpectedSlotCount = batch.m_aSlotResults.Count();
		return batch;
	}

	protected HST_ForceSpawnSlotResultState BuildRosterSlot(string slotId, string kind, string prefab, int nowSecond, bool alive)
	{
		HST_ForceSpawnSlotResultState slot = new HST_ForceSpawnSlotResultState();
		slot.m_sSlotId = slotId;
		slot.m_sSlotKind = kind;
		slot.m_sSpawnedPrefab = prefab;
		slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED;
		slot.m_iUpdatedAtSecond = nowSecond;
		slot.m_iLifecycleRevision = 1;
		slot.m_bFactionVerified = true;
		slot.m_bGroupVerified = true;
		slot.m_bProjectionVerified = true;
		slot.m_bAliveVerified = alive;
		slot.m_bEverAlive = alive;
		return slot;
	}

	protected ref array<ref HST_ConvoyElementState> BuildElements(HST_CampaignState state, HST_ActiveMissionState mission, HST_ForceManifestState manifest)
	{
		ref array<ref HST_ConvoyElementState> elements = {};
		HST_MissionAssetState cargoAsset = ResolveConvoyCargoAsset(state, mission);
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_MissionAssetState vehicleAsset = ResolveVehicleAsset(state, mission, i);
			HST_ActiveGroupState group = state.FindActiveGroup(BuildGroupId(mission, i));
			HST_ConvoyElementState element = new HST_ConvoyElementState();
			element.m_sElementId = BuildElementId(mission, i);
			element.m_sOperationId = mission.m_sOperationId;
			element.m_sMissionInstanceId = mission.m_sInstanceId;
			element.m_sManifestId = manifest.m_sManifestId;
			element.m_sVehicleSlotId = BuildVehicleSlotId(mission, i);
			element.m_sCrewGroupElementId = BuildCrewGroupElementId(mission, i);
			element.m_sVehicleAssetId = vehicleAsset.m_sAssetId;
			element.m_sGroupId = group.m_sGroupId;
			if (i == 0 && cargoAsset)
				element.m_sCargoAssetId = cargoAsset.m_sAssetId;
			element.m_sVehiclePrefab = vehicleAsset.m_sPrefab;
			element.m_sCrewGroupPrefab = group.m_sPrefab;
			vector formationOffset = "0 0 0";
			formationOffset[2] = -EXACT_FORMATION_SPACING_METERS * i;
			element.m_vFormationOffset = formationOffset;
			element.m_vCurrentPosition = vehicleAsset.m_vCurrentPosition;
			element.m_iOrdinal = i;
			element.m_iOriginalCrewCount = Math.Max(1, group.m_iInfantryCount);
			element.m_iSurvivingCrewCount = element.m_iOriginalCrewCount;
			element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE;
			element.m_bPhysicalized = false;
			element.m_bMobile = true;
			elements.Insert(element);
		}
		return elements;
	}

	protected string ValidatePreparedAuthority(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		array<ref HST_ConvoyElementState> elements)
	{
		if (!state || !mission || !manifest || !operation || !batch || !elements)
			return "exact mission convoy prepared authority is incomplete";
		if (!manifest.m_bFrozen || manifest.m_sManifestHash.IsEmpty() || m_Integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash)
			return "exact mission convoy frozen manifest hash is invalid";
		if (manifest.m_aGroups.Count() != EXACT_VEHICLE_COUNT || manifest.m_aVehicles.Count() != EXACT_VEHICLE_COUNT || elements.Count() != EXACT_VEHICLE_COUNT)
			return "exact mission convoy prepared authority is not exactly three rooted vehicles";
		if (manifest.m_iAcceptedMemberCount <= 0 || batch.m_iExpectedSlotCount != batch.m_aSlotResults.Count() || !batch.m_bStrategicProjectionHeld)
			return "exact mission convoy held roster batch is invalid";
		if (operation.m_sOperationId != mission.m_sOperationId || operation.m_sManifestId != manifest.m_sManifestId || operation.m_sSpawnResultId != batch.m_sResultId)
			return "exact mission convoy operation backlinks conflict";
		return "";
	}

	protected void LinkPreparedAuthority(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		array<ref HST_ConvoyElementState> elements)
	{
		foreach (HST_ConvoyElementState element : elements)
		{
			HST_MissionAssetState vehicleAsset = state.FindMissionAsset(element.m_sVehicleAssetId);
			HST_ActiveGroupState group = state.FindActiveGroup(element.m_sGroupId);
			vehicleAsset.m_sOperationId = operation.m_sOperationId;
			vehicleAsset.m_sManifestId = manifest.m_sManifestId;
			vehicleAsset.m_sManifestSlotId = element.m_sVehicleSlotId;
			vehicleAsset.m_sAssignedVehicleSlotId = element.m_sVehicleSlotId;
			vehicleAsset.m_sConvoyElementId = element.m_sElementId;

			group.m_sOperationId = operation.m_sOperationId;
			group.m_sManifestId = manifest.m_sManifestId;
			group.m_sSpawnResultId = batch.m_sResultId;
			group.m_sForceId = batch.m_sForceId;
			group.m_sProjectionId = batch.m_sProjectionId;
			group.m_sConvoyElementId = element.m_sElementId;
			group.m_sMissionAssetId = element.m_sVehicleAssetId;
			group.m_iOriginalInfantryCount = element.m_iOriginalCrewCount;
			group.m_iInfantryCount = element.m_iSurvivingCrewCount;
			group.m_iSurvivorInfantryCount = element.m_iSurvivingCrewCount;
			group.m_iLastSeenAliveCount = element.m_iSurvivingCrewCount;
			group.m_iDurableLivingInfantryCount = element.m_iSurvivingCrewCount;
			group.m_bEverHadLivingCrew = true;
			group.m_bEverPopulated = true;
			group.m_bSpawnCompleted = false;
			group.m_bSpawnAttempted = false;
			group.m_bSpawnedEntity = false;
		}

		HST_MissionAssetState cargoAsset = ResolveConvoyCargoAsset(state, mission);
		if (cargoAsset && manifest.m_aAssets.Count() == 1)
		{
			cargoAsset.m_sOperationId = operation.m_sOperationId;
			cargoAsset.m_sManifestId = manifest.m_sManifestId;
			cargoAsset.m_sManifestSlotId = manifest.m_aAssets[0].m_sSlotId;
			cargoAsset.m_sAssignedVehicleSlotId = manifest.m_aAssets[0].m_sAssignedVehicleSlotId;
			cargoAsset.m_sConvoyElementId = elements[0].m_sElementId;
		}
	}

	protected string ResolveAuthority(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		out HST_OperationRecordState operation,
		out HST_ForceManifestState manifest,
		out HST_ForceSpawnResultState batch)
	{
		operation = null;
		manifest = null;
		batch = null;
		if (!state || !mission)
			return "exact mission convoy authority context is missing";
		if (!IsExactMission(mission) || mission.m_sOperationId != BuildOperationId(mission)
			|| mission.m_sManifestId != BuildManifestId(mission) || mission.m_sSpawnResultId != BuildSpawnResultId(mission))
			return "exact mission convoy canonical mission authority links conflict";
		operation = state.FindOperation(mission.m_sOperationId);
		manifest = state.FindForceManifest(mission.m_sManifestId);
		batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		if (!operation || !manifest || !batch)
			return "exact mission convoy operation, manifest, or roster batch is missing";
		if (operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_sManifestId != manifest.m_sManifestId
			|| operation.m_sSpawnResultId != batch.m_sResultId)
			return "exact mission convoy reciprocal authority links conflict";
		if (manifest.m_sOperationId != operation.m_sOperationId || batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sManifestId != manifest.m_sManifestId || batch.m_sManifestHash != manifest.m_sManifestHash)
			return "exact mission convoy manifest or roster backlinks conflict";
		if (operation.m_sIssueRequestId != mission.m_sInstanceId || manifest.m_sCommandRequestId != mission.m_sInstanceId
			|| batch.m_sRequestId != mission.m_sInstanceId || operation.m_sForceId != BuildForceId(mission)
			|| operation.m_sProjectionId != BuildProjectionId(mission) || batch.m_sForceId != operation.m_sForceId
			|| batch.m_sProjectionId != operation.m_sProjectionId)
			return "exact mission convoy force, projection, or request backlinks conflict";
		if (!manifest.m_bFrozen || m_Integrity.BuildManifestHash(manifest) != manifest.m_sManifestHash)
			return "exact mission convoy manifest hash changed after admission";
		if (manifest.m_sForceKind != EXACT_FORCE_KIND || manifest.m_sFactionRole != "mission_convoy"
			|| manifest.m_sIntentId != mission.m_sMissionId || manifest.m_sTargetZoneId != mission.m_sTargetZoneId
			|| manifest.m_sCatalogVersion.IsEmpty() || manifest.m_sPolicyId != EXACT_POLICY_ID
			|| manifest.m_iRequestedVehicleCount != EXACT_VEHICLE_COUNT || manifest.m_iAcceptedVehicleCount != EXACT_VEHICLE_COUNT
			|| manifest.m_aGroups.Count() != EXACT_VEHICLE_COUNT || manifest.m_aVehicles.Count() != EXACT_VEHICLE_COUNT
			|| manifest.m_iRequestedMemberCount != manifest.m_aMembers.Count() || manifest.m_iAcceptedMemberCount != manifest.m_aMembers.Count())
			return "exact mission convoy frozen manifest contract is invalid";
		if (CountOperationClaimants(state, mission) != 1 || CountManifestClaimants(state, mission) != 1
			|| CountBatchClaimants(state, mission) != 1 || CountElementClaimants(state, mission) != EXACT_VEHICLE_COUNT)
			return "exact mission convoy authority identity is ambiguous";
		string routeFailure = ValidateRouteAuthority(state, mission, operation, manifest);
		if (!routeFailure.IsEmpty())
			return routeFailure;
		if (batch.m_iExpectedSlotCount != batch.m_aSlotResults.Count()
			|| batch.m_iExpectedSlotCount != manifest.m_aGroups.Count() + manifest.m_aVehicles.Count() + manifest.m_aMembers.Count() + manifest.m_aAssets.Count())
			return "exact mission convoy held roster slot count conflicts with its frozen manifest";
		if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			if (!operation.m_sSettlementId.IsEmpty() || !mission.m_sSettlementId.IsEmpty()
				|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
				|| !batch.m_bStrategicProjectionHeld
				|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING
				|| batch.m_bCancelRequested)
				return "open exact mission convoy contains terminal or released authority";
		}
		else if (operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			if (operation.m_sSettlementId.IsEmpty() || mission.m_sSettlementId != operation.m_sSettlementId
				|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
				|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN
				|| batch.m_bStrategicProjectionHeld)
				return "settled exact mission convoy terminal receipt is incomplete or conflicting";
		}
		else
			return "exact mission convoy settlement state is unknown";
		int elementSurvivors;
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			HST_MissionAssetState asset = ResolveVehicleAsset(state, mission, i);
			HST_ActiveGroupState group = state.FindActiveGroup(BuildGroupId(mission, i));
			if (!element || !asset || !group)
				return string.Format("exact mission convoy element %1 root is missing", i);
			if (CountElementIdentityClaimants(state, mission, i) != 1
				|| CountAssetIdentityClaimants(state, mission, i) != 1
				|| CountGroupIdentityClaimants(state, mission, i) != 1)
				return string.Format("exact mission convoy element %1 identity is ambiguous", i);
			if (element.m_sOperationId != operation.m_sOperationId || element.m_sManifestId != manifest.m_sManifestId
				|| element.m_sVehicleAssetId != asset.m_sAssetId || element.m_sGroupId != group.m_sGroupId)
				return string.Format("exact mission convoy element %1 authority backlinks conflict", i);
			if (asset.m_sConvoyElementId != element.m_sElementId || asset.m_sOperationId != operation.m_sOperationId
				|| asset.m_sManifestId != manifest.m_sManifestId)
				return string.Format("exact mission convoy asset %1 backlinks conflict", i);
			if (group.m_sConvoyElementId != element.m_sElementId || group.m_sMissionAssetId != asset.m_sAssetId
				|| group.m_sOperationId != operation.m_sOperationId || group.m_sManifestId != manifest.m_sManifestId)
				return string.Format("exact mission convoy group %1 authority backlinks conflict", i);
			if (group.m_sSpawnResultId != batch.m_sResultId || group.m_sForceId != batch.m_sForceId
				|| group.m_sProjectionId != batch.m_sProjectionId)
				return string.Format("exact mission convoy element %1 backlinks conflict", i);
			string elementFailure = ValidateElementAuthority(
				mission,
				manifest,
				batch,
				element,
				asset,
				group,
				i,
				operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN);
			if (!elementFailure.IsEmpty())
				return elementFailure;
			elementSurvivors += Math.Max(0, element.m_iSurvivingCrewCount);
		}
		string cargoFailure = ValidateCargoAuthority(state, mission, operation, manifest, batch);
		if (!cargoFailure.IsEmpty())
			return cargoFailure;
		if (CountLivingMemberSlots(batch) != elementSurvivors)
			return "exact mission convoy element survivors conflict with its durable member-slot roster";
		return "";
	}

	protected string ValidateRouteAuthority(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest)
	{
		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		ref array<vector> positions = BuildRoutePositions(route, ResolveVehicleAsset(state, mission, 0));
		float routeDistance = CalculateRouteDistance(positions);
		if (!route || positions.Count() < 2 || route.m_sRouteId.IsEmpty()
			|| operation.m_sCurrentRouteId != route.m_sRouteId || operation.m_iRouteVersion != EXACT_ROUTE_VERSION)
			return "exact mission convoy route identity or version changed after admission";
		string expectedRouteHash = BuildRouteContractHash(route, positions);
		if (expectedRouteHash.IsEmpty() || operation.m_sRouteContractHash != expectedRouteHash)
			return "exact mission convoy ordered route contract hash changed after admission";
		if (manifest.m_sSourceZoneId != route.m_sSourceZoneId || operation.m_sOriginZoneId != route.m_sSourceZoneId
			|| operation.m_sAssignmentZoneId != mission.m_sTargetZoneId || operation.m_sTacticalTargetZoneId != mission.m_sTargetZoneId)
			return "exact mission convoy route endpoint zones conflict with frozen authority";
		if (!VectorsNear(operation.m_vRouteStartPosition, positions[0], 1.0)
			|| !VectorsNear(operation.m_vRouteEndPosition, positions[positions.Count() - 1], 1.0)
			|| !VectorsNear(operation.m_vOriginPosition, positions[0], 1.0)
			|| !VectorsNear(operation.m_vAssignmentPosition, positions[positions.Count() - 1], 1.0)
			|| Math.AbsFloat(operation.m_fRouteTotalDistanceMeters - routeDistance) > 1.0)
			return "exact mission convoy route geometry changed after admission";
		if (operation.m_fRouteTotalDistanceMeters <= EXACT_ARRIVAL_RADIUS_METERS
			|| operation.m_fRouteProgressMeters < 0.0
			|| operation.m_fRouteProgressMeters > operation.m_fRouteTotalDistanceMeters + 0.1
			|| Math.AbsFloat(operation.m_fStrategicSpeedMetersPerSecond - EXACT_SPEED_METERS_PER_SECOND) > 0.01)
			return "exact mission convoy route cursor or strategic speed is invalid";
		return "";
	}

	protected string ValidateElementAuthority(
		HST_ActiveMissionState mission,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ConvoyElementState element,
		HST_MissionAssetState asset,
		HST_ActiveGroupState group,
		int ordinal,
		bool openAuthority)
	{
		HST_ForceManifestGroupState groupSlot = manifest.m_aGroups[ordinal];
		HST_ForceManifestVehicleState vehicleSlot = manifest.m_aVehicles[ordinal];
		if (!groupSlot || !vehicleSlot
			|| groupSlot.m_iOrdinal != ordinal || vehicleSlot.m_iOrdinal != ordinal
			|| groupSlot.m_sElementId != BuildCrewGroupElementId(mission, ordinal)
			|| vehicleSlot.m_sSlotId != BuildVehicleSlotId(mission, ordinal)
			|| vehicleSlot.m_sGroupElementId != groupSlot.m_sElementId
			|| groupSlot.m_sCatalogEntryId.IsEmpty()
			|| groupSlot.m_sPrefab != group.m_sPrefab || vehicleSlot.m_sPrefab != asset.m_sPrefab
			|| vehicleSlot.m_sPrefab != element.m_sVehiclePrefab || groupSlot.m_sPrefab != element.m_sCrewGroupPrefab
			|| groupSlot.m_iExpectedMemberCount != element.m_iOriginalCrewCount
			|| vehicleSlot.m_iRequiredCrew != element.m_iOriginalCrewCount)
			return string.Format("exact mission convoy element %1 frozen group or vehicle slot conflicts", ordinal);
		if (element.m_sElementId != BuildElementId(mission, ordinal) || element.m_iOrdinal != ordinal
			|| element.m_sVehicleSlotId != vehicleSlot.m_sSlotId || element.m_sCrewGroupElementId != groupSlot.m_sElementId
			|| asset.m_sManifestSlotId != vehicleSlot.m_sSlotId || asset.m_sAssignedVehicleSlotId != vehicleSlot.m_sSlotId)
			return string.Format("exact mission convoy element %1 deterministic slot identity conflicts", ordinal);
		if (Math.AbsFloat(element.m_vFormationOffset[0]) > 0.01 || Math.AbsFloat(element.m_vFormationOffset[1]) > 0.01
			|| Math.AbsFloat(element.m_vFormationOffset[2] + EXACT_FORMATION_SPACING_METERS * ordinal) > 0.1)
			return string.Format("exact mission convoy element %1 formation offset changed", ordinal);
		if (element.m_iOriginalCrewCount <= 0 || element.m_iSurvivingCrewCount < 0
			|| element.m_iSurvivingCrewCount > element.m_iOriginalCrewCount
			|| element.m_fVehicleDamageFraction < 0.0 || element.m_fVehicleDamageFraction > 1.0
			|| element.m_fFuelFraction < 0.0 || element.m_fFuelFraction > 1.0
			|| element.m_fAmmoFraction < 0.0 || element.m_fAmmoFraction > 1.0)
			return string.Format("exact mission convoy element %1 durable counts or vehicle condition are invalid", ordinal);
		string dispositionFailure = ValidateElementDispositionAssetState(element, asset, openAuthority);
		if (!dispositionFailure.IsEmpty())
			return string.Format("exact mission convoy element %1 %2", ordinal, dispositionFailure);
		if (element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE && element.m_bMobile)
			return string.Format("exact mission convoy terminal element %1 is still mobile", ordinal);
		if (CountBatchSlotClaimants(batch, groupSlot.m_sElementId) != 1
			|| CountBatchSlotClaimants(batch, vehicleSlot.m_sSlotId) != 1)
			return string.Format("exact mission convoy element %1 root roster slots are missing or ambiguous", ordinal);
		HST_ForceSpawnSlotResultState groupResult = batch.FindSlotResult(groupSlot.m_sElementId);
		HST_ForceSpawnSlotResultState vehicleResult = batch.FindSlotResult(vehicleSlot.m_sSlotId);
		if (!groupResult || !vehicleResult || groupResult.m_sSlotKind != "group" || vehicleResult.m_sSlotKind != "vehicle"
			|| groupResult.m_sSpawnedPrefab != groupSlot.m_sPrefab || vehicleResult.m_sSpawnedPrefab != vehicleSlot.m_sPrefab)
			return string.Format("exact mission convoy element %1 root roster slots conflict with the manifest", ordinal);

		int elementLivingSlots;
		for (int crewIndex = 0; crewIndex < element.m_iOriginalCrewCount; crewIndex++)
		{
			string memberSlotId = BuildMemberSlotId(mission, ordinal, crewIndex);
			HST_ForceManifestMemberState member = manifest.FindMemberSlot(memberSlotId);
			HST_ForceSpawnSlotResultState memberResult = batch.FindSlotResult(memberSlotId);
			if (!member || !memberResult || CountBatchSlotClaimants(batch, memberSlotId) != 1
				|| member.m_sCatalogSlotId.IsEmpty() || member.m_sPrefab.IsEmpty() || member.m_sRole.IsEmpty()
				|| member.m_sGroupElementId != groupSlot.m_sElementId
				|| member.m_sAssignedVehicleSlotId != vehicleSlot.m_sSlotId || memberResult.m_sSlotKind != "member"
				|| memberResult.m_sSpawnedPrefab != member.m_sPrefab || !member.m_bRequired
				|| member.m_iSeatIndex != crewIndex
				|| (crewIndex == 0 && member.m_sSeatRole != "driver")
				|| (crewIndex > 0 && member.m_sSeatRole != "passenger"))
				return string.Format("exact mission convoy element %1 member slot %2 conflicts with the frozen character roster", ordinal, crewIndex);
			if (openAuthority && !memberResult.m_bCasualtyConfirmed
				&& (memberResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED || !memberResult.m_bAliveVerified))
				return string.Format("exact mission convoy element %1 living member slot %2 is not registered and alive-authorized", ordinal, crewIndex);
			if (memberResult.m_bCasualtyConfirmed
				&& (memberResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED || memberResult.m_bAliveVerified))
				return string.Format("exact mission convoy element %1 casualty member slot %2 has conflicting lifecycle state", ordinal, crewIndex);
			if (memberResult.m_bEverAlive && !memberResult.m_bCasualtyConfirmed)
				elementLivingSlots++;
		}
		if (elementLivingSlots != element.m_iSurvivingCrewCount)
			return string.Format("exact mission convoy element %1 survivor count conflicts with its own durable member slots", ordinal);
		return "";
	}

	protected string ValidateCargoAuthority(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch)
	{
		HST_MissionAssetState cargo = ResolveConvoyCargoAsset(state, mission);
		int cargoAssetCount = CountConvoyCargoAssets(state, mission);
		HST_ConvoyElementState lead = state.FindConvoyElement(BuildElementId(mission, 0));
		if (!cargo)
		{
			if (cargoAssetCount != 0 || manifest.m_aAssets.Count() != 0 || (lead && !lead.m_sCargoAssetId.IsEmpty()))
				return "exact mission convoy cargo authority is partially present";
			return "";
		}
		if (cargoAssetCount != 1 || manifest.m_aAssets.Count() != 1 || !lead
			|| lead.m_sCargoAssetId != cargo.m_sAssetId || cargo.m_sOperationId != operation.m_sOperationId
			|| cargo.m_sManifestId != manifest.m_sManifestId || cargo.m_sConvoyElementId != lead.m_sElementId)
			return "exact mission convoy cargo backlinks are missing or ambiguous";
		HST_ForceManifestAssetState cargoSlot = manifest.m_aAssets[0];
		if (!cargoSlot || cargoSlot.m_sSlotId != BuildCargoSlotId(mission, cargo)
			|| cargoSlot.m_sPrefab != cargo.m_sPrefab || cargoSlot.m_sRole != cargo.m_sRole
			|| cargoSlot.m_sAssignedVehicleSlotId != BuildVehicleSlotId(mission, 0)
			|| cargo.m_sManifestSlotId != cargoSlot.m_sSlotId
			|| cargo.m_sAssignedVehicleSlotId != cargoSlot.m_sAssignedVehicleSlotId
			|| CountCargoIdentityClaimants(state, cargo, cargoSlot) != 1
			|| CountBatchSlotClaimants(batch, cargoSlot.m_sSlotId) != 1)
			return "exact mission convoy cargo carrier or manifest slot changed after admission";
		HST_ForceSpawnSlotResultState cargoResult = batch.FindSlotResult(cargoSlot.m_sSlotId);
		if (!cargoResult || cargoResult.m_sSlotKind != "asset" || cargoResult.m_sSpawnedPrefab != cargoSlot.m_sPrefab)
			return "exact mission convoy cargo roster slot conflicts with its frozen manifest";
		return "";
	}

	protected bool TryBeginMaterialization(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		bool recovery)
	{
		if (!state || !mission || !operation
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
			return false;
		if (recovery)
		{
			if (!IsRecoveryHold(mission) || !HasPendingRecoveryOutcome(state, mission)
				|| !HasMaterializableRecoveryRoot(state, mission))
				return false;
		}
		else if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			|| CountLivingCrews(state, mission) <= 0)
		{
			return false;
		}

		float nearestPlayer = ResolveNearestLivingPlayerDistanceForConvoy(state, mission, operation);
		if (nearestPlayer < 0 || nearestPlayer > EXACT_MATERIALIZE_IN_RADIUS_METERS)
			return false;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
		string projectionKind = "outbound";
		if (recovery)
			projectionKind = "recovery";
		operation.m_sLastProjectionReason = string.Format("living player entered %1m %2 materialization radius at %3m", Math.Round(EXACT_MATERIALIZE_IN_RADIUS_METERS), projectionKind, Math.Round(nearestPlayer));
		operation.m_iRevision++;
		return true;
	}

	protected bool EnterRecoveryHold(HST_CampaignState state, HST_ActiveMissionState mission, HST_OperationRecordState operation)
	{
		if (!state || !mission || !operation || !HasPendingRecoveryOutcome(state, mission))
			return false;

		bool changed;
		if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION
			|| operation.m_eResumeDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
			operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;
			operation.m_iDutyStateEnteredAtSecond = state.m_iElapsedSeconds;
			changed = true;
		}
		if (operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
		{
			operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
			operation.m_iEngagementStateEnteredAtSecond = state.m_iElapsedSeconds;
			changed = true;
		}
		vector recoveryAnchor = ResolveRecoveryAnchor(state, mission);
		if (!IsZeroVector(recoveryAnchor) && DistanceSq2D(operation.m_vStrategicPosition, recoveryAnchor) > 1.0)
		{
			operation.m_vStrategicPosition = recoveryAnchor;
			changed = true;
		}
		mission.m_iRuntimeETASeconds = 0;
		if (!changed)
			return false;
		operation.m_iStrategicLastUpdateSecond = state.m_iElapsedSeconds;
		operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
		operation.m_sLastProjectionReason = "secured convoy assets held for player recovery";
		operation.m_iRevision++;
		return true;
	}

	protected bool IsRecoveryHold(HST_ActiveMissionState mission)
	{
		return mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& mission.m_sRuntimePhase == CONVOY_ELIMINATED;
	}

	protected bool HasPendingRecoveryOutcome(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsRecoveryHold(mission))
			return false;
		if (mission.m_sMissionId == "convoy_money")
			return HasPendingAssetOutcome(state, mission, PAYLOAD_ROLE);
		if (mission.m_sMissionId == "convoy_supplies")
			return HasPendingAssetOutcome(state, mission, PAYLOAD_ROLE);
		if (mission.m_sMissionId == "convoy_prisoners")
			return HasPendingAssetOutcome(state, mission, CAPTIVE_ROLE);
		if (mission.m_sMissionId == "convoy_ammo" || mission.m_sMissionId == "convoy_armored")
		{
			if (mission.m_bConvoyVehicleCapturedOutcomeApplied)
				return false;
			for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
			{
				HST_MissionAssetState vehicle = ResolveVehicleAsset(state, mission, i);
				if (vehicle && !IsVehicleAssetTerminal(vehicle))
					return true;
			}
		}
		return false;
	}

	protected bool HasPendingAssetOutcome(HST_CampaignState state, HST_ActiveMissionState mission, string role)
	{
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role)
				continue;
			if (!asset.m_bDestroyed && (!asset.m_bDelivered || !asset.m_bOutcomeApplied))
				return true;
		}
		return false;
	}

	protected bool IsVehicleAssetTerminal(HST_MissionAssetState asset)
	{
		return !asset || asset.m_bDestroyed || asset.m_bDelivered || asset.m_sLastInteraction == "captured" || asset.m_sLastInteraction == "retired";
	}

	static bool IsRecoverableAbandonedVehicleRoot(HST_MissionAssetState asset, HST_ConvoyElementState element)
	{
		if (!asset || !element || asset.m_bDestroyed || asset.m_bDelivered
			|| asset.m_sLastInteraction == "captured" || asset.m_sLastInteraction == "retired")
			return false;
		return element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED
			&& element.m_iSurvivingCrewCount <= 0 && !element.m_bMobile
			&& element.m_fVehicleDamageFraction >= 0.0 && element.m_fVehicleDamageFraction < 1.0;
	}

	protected bool HasRecoverableAbandonedVehicle(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_MissionAssetState asset = ResolveVehicleAsset(state, mission, i);
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			if (IsRecoverableAbandonedVehicleRoot(asset, element))
				return true;
		}
		return false;
	}

	bool HasMaterializableRecoveryRoot(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		return HasRecoverableAbandonedVehicle(state, mission)
			|| HasUnresolvedTerminalCarrierCargoRoot(state, mission);
	}

	protected bool HasUnresolvedTerminalCarrierCargoRoot(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_MissionAssetState cargo = ResolveConvoyCargoAsset(state, mission);
		if (!cargo || cargo.m_bPickedUp || cargo.m_bDelivered || cargo.m_bDestroyed || cargo.m_bOutcomeApplied
			|| cargo.m_sAssignedVehicleSlotId.IsEmpty())
			return false;
		HST_ConvoyElementState carrier = FindElementByVehicleSlot(state, mission, cargo.m_sAssignedVehicleSlotId);
		if (!carrier || carrier.m_bMobile)
			return false;

		HST_MissionAssetState carrierAsset;
		for (int index = 0; index < EXACT_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState candidate = ResolveVehicleAsset(state, mission, index);
			if (candidate && candidate.m_sManifestSlotId == cargo.m_sAssignedVehicleSlotId)
			{
				carrierAsset = candidate;
				break;
			}
		}
		if (!carrierAsset)
			return false;
		if (carrier.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED)
			return carrierAsset.m_bDestroyed;
		if (carrier.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED)
			return carrierAsset.m_bDelivered || carrierAsset.m_sLastInteraction == "captured";
		return false;
	}

	protected vector ResolveRecoveryAnchor(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_MissionAssetState cargo = ResolveConvoyCargoAsset(state, mission);
		if (cargo && !cargo.m_bDelivered && !cargo.m_bDestroyed && !IsZeroVector(cargo.m_vCurrentPosition))
			return cargo.m_vCurrentPosition;
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_MissionAssetState asset = ResolveVehicleAsset(state, mission, i);
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			if (!asset || !element || IsVehicleAssetTerminal(asset))
				continue;
			if (!IsZeroVector(element.m_vCurrentPosition))
				return element.m_vCurrentPosition;
			if (!IsZeroVector(asset.m_vCurrentPosition))
				return asset.m_vCurrentPosition;
		}
		return "0 0 0";
	}

	protected bool NormalizeRestoredProjection(HST_CampaignState state, HST_ActiveMissionState mission, HST_OperationRecordState operation)
	{
		if (!state.m_bRestoredFromPersistence || !mission || !operation)
			return false;
		bool pendingArrivalOutcome = HasDurableArrivalEvidence(state, mission, operation)
			&& !mission.m_bConvoyArrivalOutcomeApplied;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| (IsTerminalRuntimePhase(mission) && !IsRecoveryHold(mission) && !pendingArrivalOutcome))
			return false;
		int restoreSequence = Math.Max(0, state.m_iPersistenceRestoreSequence);
		if (operation.m_iLastNormalizedRestoreSequence == restoreSequence)
			return false;
		string restoreToken = string.Format("restore sequence %1 normalized", restoreSequence);

		if (!m_PhysicalWar.NormalizeExactMissionConvoyRuntimeForRestore(state, mission))
		{
			FailExactMissionContract(state, mission, "restored exact convoy runtime could not be normalized without changing survivor or element authority");
			return true;
		}
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iStrategicLastUpdateSecond = state.m_iElapsedSeconds;
		operation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
		operation.m_iLastNormalizedRestoreSequence = restoreSequence;
		operation.m_sLastProjectionReason = restoreToken;
		operation.m_iRevision++;
		SetElementsPhysicalized(state, mission, false);
		ProjectStrategicState(state, mission, operation);
		return true;
	}

	protected bool AdvanceVirtualRoute(HST_CampaignState state, HST_ActiveMissionState mission, HST_OperationRecordState operation, int elapsedSeconds)
	{
		if (!state || !mission || !operation || operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			return false;
		if (CountMobileCrewedElements(state, mission) <= 0)
		{
			vector survivorPosition = ResolveFirstLivingElementPosition(state, mission);
			bool changed;
			if (!IsZeroVector(survivorPosition) && DistanceSq2D(operation.m_vStrategicPosition, survivorPosition) > 1.0)
			{
				operation.m_vStrategicPosition = survivorPosition;
				operation.m_iStrategicLastUpdateSecond = state.m_iElapsedSeconds;
				operation.m_iRevision++;
				changed = true;
			}
			if (mission.m_iRuntimeETASeconds != 0)
			{
				mission.m_iRuntimeETASeconds = 0;
				changed = true;
			}
			SyncCargoProjection(state, mission);
			return changed;
		}

		int nowSecond = state.m_iElapsedSeconds;
		int catchupSeconds = nowSecond - operation.m_iStrategicLastUpdateSecond;
		if (catchupSeconds <= 0)
			catchupSeconds = Math.Max(0, elapsedSeconds);
		catchupSeconds = Math.Min(EXACT_MAX_VIRTUAL_CATCHUP_SECONDS, catchupSeconds);
		if (catchupSeconds <= 0)
			return ProjectStrategicState(state, mission, operation);

		float previousProgress = operation.m_fRouteProgressMeters;
		operation.m_fRouteProgressMeters = Math.Min(operation.m_fRouteTotalDistanceMeters,
			operation.m_fRouteProgressMeters + operation.m_fStrategicSpeedMetersPerSecond * catchupSeconds);
		operation.m_iStrategicLastUpdateSecond = nowSecond;
		if (operation.m_fRouteProgressMeters > previousProgress)
			operation.m_iLastProgressAtSecond = nowSecond;
		operation.m_iRevision++;
		ProjectStrategicState(state, mission, operation);
		if (operation.m_fRouteProgressMeters >= operation.m_fRouteTotalDistanceMeters
			&& IsArrivalPositionConfirmed(state, mission, operation))
			MarkConvoyArrived(state, mission, operation, "virtual route cursor reached destination");
		return true;
	}

	protected bool ProjectStrategicState(HST_CampaignState state, HST_ActiveMissionState mission, HST_OperationRecordState operation)
	{
		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		ref array<vector> positions = BuildRoutePositions(route, ResolveVehicleAsset(state, mission, 0));
		if (positions.Count() < 2)
			return false;

		vector leadPosition = PositionAtDistance(positions, operation.m_fRouteProgressMeters);
		operation.m_vStrategicPosition = leadPosition;
		UpdateRuntimeETAFromOperation(state, mission, operation);

		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			if (!element || element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
				|| !element.m_bMobile || element.m_iSurvivingCrewCount <= 0)
				continue;
			float elementDistance = Math.Max(0.0, operation.m_fRouteProgressMeters - EXACT_FORMATION_SPACING_METERS * i);
			vector elementPosition = PositionAtDistance(positions, elementDistance);
			element.m_vCurrentPosition = elementPosition;
			element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			element.m_iRevision++;
			HST_MissionAssetState asset = state.FindMissionAsset(element.m_sVehicleAssetId);
			HST_ActiveGroupState group = state.FindActiveGroup(element.m_sGroupId);
			if (asset)
			{
				asset.m_vCurrentPosition = elementPosition;
				asset.m_vLastKnownPosition = elementPosition;
			}
			if (group)
			{
				group.m_vPosition = elementPosition;
				group.m_vSourcePosition = elementPosition;
				group.m_sRuntimeStatus = CONVOY_MOVING;
			}
		}
		SyncCargoProjection(state, mission);
		return true;
	}

	protected bool SyncPhysicalProjection(HST_CampaignState state, HST_ActiveMissionState mission, HST_OperationRecordState operation)
	{
		bool changed;
		vector leadPosition;
		vector firstActivePosition;
		vector firstLivingPosition;
		int formationReferenceOrdinal = -1;
		int firstActiveOrdinal = -1;
		int firstLivingOrdinal = -1;
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			HST_MissionAssetState asset = ResolveVehicleAsset(state, mission, i);
			HST_ActiveGroupState group = state.FindActiveGroup(BuildGroupId(mission, i));
			if (!element || !asset || !group)
				continue;
			changed = SynchronizeElementSurvivorsFromMemberSlots(state, mission, element) || changed;

			HST_EConvoyElementDisposition previousDisposition = element.m_eDisposition;
			bool previousMobile = element.m_bMobile;
			if (asset.m_bDestroyed)
			{
				element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED;
				element.m_bMobile = false;
				element.m_fVehicleDamageFraction = 1.0;
			}
			else if (asset.m_bDelivered || asset.m_sLastInteraction == "captured")
			{
				element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED;
				element.m_bMobile = false;
			}
			if (element.m_iSurvivingCrewCount <= 0 && element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE)
			{
				element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED;
				element.m_bMobile = false;
				element.m_sTerminalReason = "crew eliminated";
			}
			if (element.m_eDisposition != previousDisposition || element.m_bMobile != previousMobile)
			{
				element.m_iRevision++;
				changed = true;
			}
			vector position = asset.m_vCurrentPosition;
			if (element.m_iSurvivingCrewCount > 0
				&& element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
				&& !IsZeroVector(group.m_vPosition))
				position = group.m_vPosition;
			else if (IsZeroVector(position))
				position = group.m_vPosition;
			if (!IsZeroVector(position))
			{
				element.m_vCurrentPosition = position;
				element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
				if (element.m_iSurvivingCrewCount > 0 && IsZeroVector(firstLivingPosition))
				{
					firstLivingPosition = position;
					firstLivingOrdinal = i;
				}
				if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
					&& IsZeroVector(firstActivePosition))
				{
					firstActivePosition = position;
					firstActiveOrdinal = i;
				}
				if (i == 0 && element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE)
				{
					leadPosition = position;
					formationReferenceOrdinal = 0;
				}
			}
		}

		if (IsZeroVector(leadPosition))
		{
			leadPosition = firstActivePosition;
			formationReferenceOrdinal = firstActiveOrdinal;
		}
		if (IsZeroVector(leadPosition))
		{
			leadPosition = firstLivingPosition;
			formationReferenceOrdinal = firstLivingOrdinal;
		}
		if (!IsZeroVector(leadPosition))
		{
			float formationOffsetMeters = EXACT_FORMATION_SPACING_METERS * Math.Max(0, formationReferenceOrdinal);
			float minimumElementDistance = Math.Max(0.0, operation.m_fRouteProgressMeters - formationOffsetMeters);
			float projectedElementDistance = ProjectPositionToRouteDistance(state, mission, leadPosition, minimumElementDistance);
			float projectedDistance = Math.Min(operation.m_fRouteTotalDistanceMeters, projectedElementDistance + formationOffsetMeters);
			if (projectedDistance > operation.m_fRouteProgressMeters)
			{
				operation.m_fRouteProgressMeters = Math.Min(operation.m_fRouteTotalDistanceMeters, projectedDistance);
				operation.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
				changed = true;
			}
			ref array<vector> routePositions = BuildRoutePositions(ResolveMissionRoute(state, mission), ResolveVehicleAsset(state, mission, 0));
			operation.m_vStrategicPosition = PositionAtDistance(routePositions, operation.m_fRouteProgressMeters);
			operation.m_iStrategicLastUpdateSecond = state.m_iElapsedSeconds;
			operation.m_iRevision++;
		}
		changed = UpdateRuntimeETAFromOperation(state, mission, operation) || changed;
		SyncCargoProjection(state, mission);
		return changed;
	}

	protected bool IsPhysicalCrewSampleAuthoritative(HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		if (group.m_sRuntimeStatus == CONVOY_ELIMINATED || group.m_sRuntimeStatus == "eliminated")
			return true;
		if (group.m_sRuntimeStatus == "spawn_pending_agents"
			|| group.m_sRuntimeStatus == "spawn_deferred_aiworld_budget"
			|| group.m_sRuntimeStatus == "spawn_failed"
			|| group.m_bCrewPopulationTerminallyFailed)
			return false;
		return group.m_bSpawnedEntity;
	}

	protected bool UpdateRuntimeETAFromOperation(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation)
	{
		if (!state || !mission || !operation)
			return false;
		int etaSeconds;
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND
			&& CountMobileCrewedElements(state, mission) > 0
			&& operation.m_fStrategicSpeedMetersPerSecond > 0.01)
		{
			float remaining = Math.Max(0.0, operation.m_fRouteTotalDistanceMeters - operation.m_fRouteProgressMeters);
			etaSeconds = Math.Round(remaining / operation.m_fStrategicSpeedMetersPerSecond);
		}
		if (mission.m_iRuntimeETASeconds == etaSeconds)
			return false;
		mission.m_iRuntimeETASeconds = etaSeconds;
		return true;
	}

	protected bool TryConfirmPhysicalArrival(HST_CampaignState state, HST_ActiveMissionState mission, HST_OperationRecordState operation)
	{
		if (!state || !mission || !operation || operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND)
			return false;
		if (CountMobileCrewedElements(state, mission) <= 0)
			return false;

		float remaining = Math.Max(0.0, operation.m_fRouteTotalDistanceMeters - operation.m_fRouteProgressMeters);
		if (remaining > EXACT_ARRIVAL_RADIUS_METERS || !IsArrivalPositionConfirmed(state, mission, operation))
		{
			if (operation.m_iArrivalConfirmationCount != 0)
			{
				operation.m_iArrivalConfirmationCount = 0;
				operation.m_iLastArrivalConfirmationSecond = 0;
				operation.m_iRevision++;
				return true;
			}
			return false;
		}

		if (operation.m_iLastArrivalConfirmationSecond != state.m_iElapsedSeconds)
		{
			operation.m_iArrivalConfirmationCount++;
			operation.m_iLastArrivalConfirmationSecond = state.m_iElapsedSeconds;
			operation.m_iRevision++;
		}
		if (operation.m_iArrivalConfirmationCount < EXACT_ARRIVAL_CONFIRMATION_SAMPLES)
			return true;

		return MarkConvoyArrived(state, mission, operation, "two distinct physical route-end samples confirmed arrival");
	}

	protected bool MarkConvoyArrived(HST_CampaignState state, HST_ActiveMissionState mission, HST_OperationRecordState operation, string reason)
	{
		if (!state || !mission || !operation || mission.m_sRuntimePhase == CONVOY_FAILED)
			return false;
		if (CountMobileCrewedElements(state, mission) <= 0)
			return false;
		if (!IsArrivalPositionConfirmed(state, mission, operation))
			return false;

		mission.m_sRuntimePhase = CONVOY_FAILED;
		mission.m_sRuntimeFailureReason = "Convoy reached its destination: " + reason;
		mission.m_sLastRuntimeEventKey = CONVOY_FAIL_EVENT;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == mission.m_sInstanceId && !objective.m_bComplete)
				objective.m_bFailed = true;
		}
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			if (element && element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE && element.m_iSurvivingCrewCount > 0)
			{
				element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED;
				element.m_bMobile = false;
				element.m_sTerminalReason = reason;
				element.m_iRevision++;
			}
		}
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision++;
		Print(string.Format("Partisan exact mission convoy | %1 arrived at destination | %2", mission.m_sInstanceId, reason));
		return true;
	}

	protected bool IsArrivalPositionConfirmed(HST_CampaignState state, HST_ActiveMissionState mission, HST_OperationRecordState operation)
	{
		if (!state || !mission || !operation || IsZeroVector(operation.m_vRouteEndPosition))
			return false;
		vector carrierPosition;
		HST_MissionAssetState cargo = ResolveConvoyCargoAsset(state, mission);
		if (cargo)
		{
			if (cargo.m_bDestroyed || cargo.m_bDelivered || cargo.m_bPickedUp || cargo.m_sAssignedVehicleSlotId.IsEmpty())
				return false;
			HST_ConvoyElementState carrier = FindElementByVehicleSlot(state, mission, cargo.m_sAssignedVehicleSlotId);
			if (!carrier || carrier.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
				|| !carrier.m_bMobile || carrier.m_iSurvivingCrewCount <= 0)
				return false;
			carrierPosition = carrier.m_vCurrentPosition;
		}
		else
		{
			carrierPosition = ResolveFirstActiveElementPosition(state, mission);
		}
		if (IsZeroVector(carrierPosition))
			return false;
		return DistanceSq2D(carrierPosition, operation.m_vRouteEndPosition)
			<= EXACT_ARRIVAL_RADIUS_METERS * EXACT_ARRIVAL_RADIUS_METERS;
	}

	protected bool SettleOperation(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_EOperationTerminalResult terminalResult,
		string settlementKind,
		string reason)
	{
		if (!state || !mission || !operation || operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			return false;
		HST_OperationRecordState canonicalOperation;
		HST_ForceManifestState canonicalManifest;
		HST_ForceSpawnResultState canonicalBatch;
		string authorityFailure = ResolveAuthority(state, mission, canonicalOperation, canonicalManifest, canonicalBatch);
		if (!authorityFailure.IsEmpty() || canonicalOperation != operation)
		{
			FailExactMissionContract(state, mission, "settlement authority conflict: " + authorityFailure);
			return false;
		}
		string settlementId = HST_OperationService.BuildSettlementId(operation.m_sOperationId, settlementKind);
		if (settlementId.IsEmpty())
			return false;
		if ((!mission.m_sSettlementId.IsEmpty() && mission.m_sSettlementId != settlementId)
			|| (!operation.m_sSettlementId.IsEmpty() && operation.m_sSettlementId != settlementId))
		{
			FailExactMissionContract(state, mission, "settlement receipt conflicts with the requested terminal outcome");
			return false;
		}

		bool hasPhysicalRuntime = m_PhysicalWar && m_PhysicalWar.HasExactMissionConvoyRuntime(mission);
		mission.m_sSettlementId = settlementId;
		operation.m_sSettlementId = settlementId;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		operation.m_eTerminalResult = terminalResult;
		operation.m_sTerminalReason = reason;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_iDutyStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iSettledAtSecond = state.m_iElapsedSeconds;
		if (hasPhysicalRuntime)
		{
			operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRING;
			operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		}
		else
		{
			operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
			operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		}
		operation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
		operation.m_iRevision++;
		foreach (HST_ConvoyElementState element : state.m_aConvoyElements)
		{
			if (!element || element.m_sOperationId != operation.m_sOperationId)
				continue;
			if (element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
				&& element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED)
				continue;
			element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_RETIRED;
			element.m_bPhysicalized = hasPhysicalRuntime;
			element.m_bMobile = false;
			element.m_sTerminalReason = reason;
			element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			element.m_iRevision++;
		}

		HST_ForceSpawnResultState batch = canonicalBatch;
		if (batch)
		{
			batch.m_bStrategicProjectionHeld = false;
			batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
			batch.m_sTerminalReason = reason;
			batch.m_iCompletedAtSecond = state.m_iElapsedSeconds;
			batch.m_iUpdatedAtSecond = state.m_iElapsedSeconds;
			batch.m_iLifecycleRevision++;
			foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
			{
				if (!slot || slot.m_bCasualtyConfirmed)
					continue;
				slot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
				slot.m_sRetirementReason = reason;
				slot.m_iUpdatedAtSecond = state.m_iElapsedSeconds;
				slot.m_iLifecycleRevision++;
			}
		}
		Print(string.Format("Partisan exact mission convoy | settled %1 | result %2 | settlement %3 | %4", mission.m_sInstanceId, terminalResult, settlementId, reason));
		return true;
	}

	protected bool IsPhysicalProjectionReady(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		if (IsRecoveryHold(mission))
			return m_PhysicalWar.IsExactMissionConvoyRecoveryProjectionReady(state, mission);
		return m_PhysicalWar.IsExactMissionConvoySurvivorProjectionReady(state, mission)
			&& IsCargoProjectionReady(state, mission);
	}

	protected bool IsCargoProjectionReady(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_MissionAssetState cargo = ResolveConvoyCargoAsset(state, mission);
		if (!cargo || cargo.m_bPickedUp || cargo.m_bDelivered || cargo.m_bDestroyed || cargo.m_bOutcomeApplied)
			return true;
		if (!m_MissionRuntime || !m_PhysicalWar)
			return false;
		return m_MissionRuntime.IsExactMissionConvoyCargoProjectionReady(state, mission, m_PhysicalWar);
	}

	protected bool MarkAllCrewsEliminated(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePhase == CONVOY_ELIMINATED)
			return false;
		if (CountLivingCrews(state, mission) > 0)
			return false;
		mission.m_sRuntimePhase = CONVOY_ELIMINATED;
		mission.m_sRuntimeFailureReason = "";
		mission.m_sLastRuntimeEventKey = CONVOY_COMPLETE_EVENT;
		int eliminatedGroups;
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			HST_MissionAssetState asset = ResolveVehicleAsset(state, mission, i);
			HST_ActiveGroupState group = state.FindActiveGroup(BuildGroupId(mission, i));
			if (!group || !element || !asset)
				continue;
			element.m_iSurvivingCrewCount = 0;
			if (asset.m_bDestroyed)
				element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED;
			else if (asset.m_bDelivered || asset.m_sLastInteraction == "captured")
				element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED;
			else
				element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED;
			element.m_bMobile = false;
			element.m_sTerminalReason = "all convoy crew eliminated";
			element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			element.m_iRevision++;
			SynchronizeElementSurvivorsFromMemberSlots(state, mission, element);
			group.m_sRuntimeStatus = CONVOY_ELIMINATED;
			group.m_iInfantryCount = 0;
			group.m_iLastSeenAliveCount = 0;
			group.m_iSurvivorInfantryCount = 0;
			group.m_iDurableLivingInfantryCount = 0;
			group.m_iEliminatedAtSecond = state.m_iElapsedSeconds;
			group.m_iLifecycleRevision++;
			eliminatedGroups++;
		}
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_sTargetId != "convoy" || objective.m_bFailed)
				continue;
			int required = Math.Max(1, eliminatedGroups);
			objective.m_iRequiredCount = required;
			objective.m_iRequiredProgress = required;
			objective.m_iCurrentCount = required;
			objective.m_iCurrentProgress = required;
			objective.m_bComplete = true;
		}
		Print(string.Format("Partisan exact mission convoy | %1 completed from durable zero-survivor roster", mission.m_sInstanceId));
		return true;
	}

	protected bool SetElementsPhysicalized(HST_CampaignState state, HST_ActiveMissionState mission, bool physicalized)
	{
		bool changed;
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			if (!element)
				continue;
			bool desired = physicalized && element.m_iSurvivingCrewCount > 0
				&& (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
					|| element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED
					|| element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED);
			if (physicalized
				&& element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED)
				desired = true;
			if (element.m_bPhysicalized == desired)
				continue;
			element.m_bPhysicalized = desired;
			element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			element.m_iRevision++;
			changed = true;
		}
		return changed;
	}

	protected void SyncCargoProjection(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_MissionAssetState cargo = ResolveConvoyCargoAsset(state, mission);
		if (!cargo || cargo.m_bPickedUp || cargo.m_bDelivered || cargo.m_bDestroyed || cargo.m_sAssignedVehicleSlotId.IsEmpty())
			return;
		HST_ConvoyElementState carrier = FindElementByVehicleSlot(state, mission, cargo.m_sAssignedVehicleSlotId);
		if (!carrier)
			return;
		cargo.m_vCurrentPosition = carrier.m_vCurrentPosition;
		cargo.m_vLastKnownPosition = carrier.m_vCurrentPosition;
	}

	protected HST_ConvoyElementState FindElementByVehicleSlot(HST_CampaignState state, HST_ActiveMissionState mission, string vehicleSlotId)
	{
		if (!state || !mission || vehicleSlotId.IsEmpty())
			return null;
		foreach (HST_ConvoyElementState element : state.m_aConvoyElements)
		{
			if (element && element.m_sMissionInstanceId == mission.m_sInstanceId && element.m_sVehicleSlotId == vehicleSlotId)
				return element;
		}
		return null;
	}

	protected vector ResolveFirstActiveElementPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			if (element && element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE && !IsZeroVector(element.m_vCurrentPosition))
				return element.m_vCurrentPosition;
		}
		return "0 0 0";
	}

	protected vector ResolveFirstLivingElementPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			if (element && element.m_iSurvivingCrewCount > 0 && !IsZeroVector(element.m_vCurrentPosition))
				return element.m_vCurrentPosition;
		}
		return "0 0 0";
	}

	protected int CountLivingCrews(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int count;
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			if (element && element.m_iSurvivingCrewCount > 0)
				count++;
		}
		return count;
	}

	protected int CountMobileCrewedElements(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int count;
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			if (element && element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
				&& element.m_bMobile && element.m_iSurvivingCrewCount > 0)
				count++;
		}
		return count;
	}

	protected bool SynchronizeElementSurvivorsFromMemberSlots(HST_CampaignState state, HST_ActiveMissionState mission, HST_ConvoyElementState element)
	{
		if (!state || !mission || !element)
			return false;
		HST_ForceManifestState manifest = state.FindForceManifest(mission.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		if (!manifest || !manifest.m_bFrozen || !batch
			|| manifest.m_sOperationId != mission.m_sOperationId
			|| batch.m_sOperationId != mission.m_sOperationId
			|| batch.m_sManifestId != manifest.m_sManifestId)
			return false;
		int livingSlots;
		for (int i = 0; i < element.m_iOriginalCrewCount; i++)
		{
			string memberSlotId = BuildMemberSlotId(mission, element.m_iOrdinal, i);
			HST_ForceManifestMemberState member = manifest.FindMemberSlot(memberSlotId);
			HST_ForceSpawnSlotResultState slot = batch.FindSlotResult(memberSlotId);
			if (!member || !slot || !member.m_bRequired || member.m_sGroupElementId != element.m_sCrewGroupElementId
				|| member.m_sAssignedVehicleSlotId != element.m_sVehicleSlotId || slot.m_sSlotKind != "member"
				|| slot.m_sSpawnedPrefab != member.m_sPrefab || !slot.m_bEverAlive)
				return false;
			if (!slot.m_bCasualtyConfirmed)
				livingSlots++;
			else if (slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED || slot.m_bAliveVerified)
				return false;
		}
		if (element.m_iSurvivingCrewCount != livingSlots)
		{
			element.m_iSurvivingCrewCount = livingSlots;
			element.m_iRevision++;
			return true;
		}
		return false;
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

	protected float ProjectPositionToRouteDistance(HST_CampaignState state, HST_ActiveMissionState mission, vector position, float minimumDistance)
	{
		ref array<vector> routePositions = BuildRoutePositions(ResolveMissionRoute(state, mission), ResolveVehicleAsset(state, mission, 0));
		float bestDistanceSq = 999999999.0;
		float bestRouteDistance = minimumDistance;
		float traversed;
		for (int i = 1; i < routePositions.Count(); i++)
		{
			vector start = routePositions[i - 1];
			vector end = routePositions[i];
			float segmentLength = Distance2D(start, end);
			if (segmentLength <= 0.01)
				continue;
			float t;
			vector projected = ClosestPointOnSegment(start, end, position, t);
			float distanceSq = DistanceSq2D(projected, position);
			float routeDistance = traversed + segmentLength * t;
			if (routeDistance + EXACT_FORMATION_SPACING_METERS >= minimumDistance && distanceSq < bestDistanceSq)
			{
				bestDistanceSq = distanceSq;
				bestRouteDistance = Math.Max(minimumDistance, routeDistance);
			}
			traversed += segmentLength;
		}
		return bestRouteDistance;
	}

	protected vector ClosestPointOnSegment(vector start, vector end, vector point, out float t)
	{
		vector result = start;
		float dx = end[0] - start[0];
		float dz = end[2] - start[2];
		float lengthSq = dx * dx + dz * dz;
		t = 0.0;
		if (lengthSq <= 0.01)
			return result;
		t = ((point[0] - start[0]) * dx + (point[2] - start[2]) * dz) / lengthSq;
		if (t < 0.0)
			t = 0.0;
		else if (t > 1.0)
			t = 1.0;
		result[0] = start[0] + dx * t;
		result[1] = start[1] + (end[1] - start[1]) * t;
		result[2] = start[2] + dz * t;
		return result;
	}

	protected vector PositionAtDistance(array<vector> positions, float distanceMeters)
	{
		if (!positions || positions.Count() == 0)
			return "0 0 0";
		if (distanceMeters <= 0.0)
			return positions[0];
		float traversed;
		for (int i = 1; i < positions.Count(); i++)
		{
			vector start = positions[i - 1];
			vector end = positions[i];
			float segmentLength = Distance2D(start, end);
			if (segmentLength <= 0.01)
				continue;
			if (traversed + segmentLength >= distanceMeters)
			{
				float t = (distanceMeters - traversed) / segmentLength;
				vector result = start;
				result[0] = start[0] + (end[0] - start[0]) * t;
				result[1] = start[1] + (end[1] - start[1]) * t;
				result[2] = start[2] + (end[2] - start[2]) * t;
				return result;
			}
			traversed += segmentLength;
		}
		return positions[positions.Count() - 1];
	}

	protected ref array<vector> BuildRoutePositions(HST_GeneratedRouteState route, HST_MissionAssetState leadAsset)
	{
		ref array<vector> positions = {};
		if (leadAsset && !IsZeroVector(leadAsset.m_vSourcePosition))
			AppendRoutePosition(positions, leadAsset.m_vSourcePosition);
		if (route)
		{
			int lastIndex = -1000000;
			while (true)
			{
				HST_RouteWaypointState selected;
				int selectedIndex = 1000000;
				foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
				{
					if (!waypoint || waypoint.m_iIndex <= lastIndex || waypoint.m_iIndex >= selectedIndex)
						continue;
					selected = waypoint;
					selectedIndex = waypoint.m_iIndex;
				}
				if (!selected)
					break;
				AppendRoutePosition(positions, selected.m_vPosition);
				lastIndex = selectedIndex;
			}
			AppendRoutePosition(positions, route.m_vEndPosition);
		}
		if (leadAsset)
			AppendRoutePosition(positions, leadAsset.m_vTargetPosition);
		return positions;
	}

	static string BuildRouteContractHash(HST_GeneratedRouteState route, array<vector> positions)
	{
		if (!route || !positions || positions.Count() < 2)
			return "";
		string canonical = string.Format("%1|%2|%3|%4", route.m_sRouteId, route.m_sSourceZoneId, route.m_sTargetZoneId, positions.Count());
		foreach (vector position : positions)
			canonical = canonical + string.Format("|%1:%2:%3", position[0], position[1], position[2]);
		return string.Format("mcr1_%1_%2", canonical.Hash(), (canonical + "|secondary").Hash());
	}

	protected void AppendRoutePosition(array<vector> positions, vector position)
	{
		if (!positions || IsZeroVector(position))
			return;
		if (positions.Count() > 0 && DistanceSq2D(positions[positions.Count() - 1], position) <= 1.0)
			return;
		positions.Insert(position);
	}

	protected float CalculateRouteDistance(array<vector> positions)
	{
		float result;
		if (!positions)
			return result;
		for (int i = 1; i < positions.Count(); i++)
			result += Distance2D(positions[i - 1], positions[i]);
		return result;
	}

	protected HST_GeneratedRouteState ResolveMissionRoute(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sSiteId.IsEmpty())
			return null;
		HST_GeneratedSiteState site = state.FindGeneratedSite(mission.m_sSiteId);
		if (!site || site.m_sRouteId.IsEmpty())
			return null;
		return state.FindGeneratedRoute(site.m_sRouteId);
	}

	protected HST_MissionAssetState ResolveVehicleAsset(HST_CampaignState state, HST_ActiveMissionState mission, int ordinal)
	{
		if (!state || !mission || ordinal < 0)
			return null;
		return state.FindMissionAsset(string.Format("asset_%1_%2_%3", mission.m_sInstanceId, VEHICLE_ROLE, ordinal));
	}

	protected HST_MissionAssetState ResolveConvoyCargoAsset(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return null;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && (asset.m_sRole == PAYLOAD_ROLE || asset.m_sRole == CAPTIVE_ROLE))
				return asset;
		}
		return null;
	}

	protected void RollbackUncommittedAdmission(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;
		string operationId = mission.m_sOperationId;
		string manifestId = mission.m_sManifestId;
		for (int groupOrdinal = 0; groupOrdinal < EXACT_VEHICLE_COUNT; groupOrdinal++)
		{
			string preparedGroupId = BuildGroupId(mission, groupOrdinal);
			for (int groupIndex = state.m_aActiveGroups.Count() - 1; groupIndex >= 0; groupIndex--)
			{
				HST_ActiveGroupState group = state.m_aActiveGroups[groupIndex];
				if (!group || group.m_sGroupId != preparedGroupId || group.m_sMissionInstanceId != mission.m_sInstanceId || group.m_bSpawnedEntity)
					continue;
				if (!group.m_sOperationId.IsEmpty() && group.m_sOperationId != operationId)
					continue;
				state.m_aActiveGroups.Remove(groupIndex);
			}
		}
		for (int assetOrdinal = 0; assetOrdinal < EXACT_VEHICLE_COUNT; assetOrdinal++)
		{
			HST_MissionAssetState asset = ResolveVehicleAsset(state, mission, assetOrdinal);
			if (!asset)
				continue;
			ClearAttemptedAssetAuthorityLinks(
				asset,
				operationId,
				manifestId,
				BuildVehicleSlotId(mission, assetOrdinal),
				BuildVehicleSlotId(mission, assetOrdinal),
				BuildElementId(mission, assetOrdinal));
		}
		HST_MissionAssetState cargo = ResolveConvoyCargoAsset(state, mission);
		if (cargo)
		{
			ClearAttemptedAssetAuthorityLinks(
				cargo,
				operationId,
				manifestId,
				BuildCargoSlotId(mission, cargo),
				BuildVehicleSlotId(mission, 0),
				BuildElementId(mission, 0));
		}
		ResetUncommittedAdmissionContract(state, mission);
	}

	protected void ResetUncommittedAdmissionContract(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission)
			return;
		if (HasCommittedAdmissionAuthority(state, mission))
			return;
		mission.m_sOperationId = "";
		mission.m_sManifestId = "";
		mission.m_sSpawnResultId = "";
		mission.m_sSettlementId = "";
		mission.m_iOperationContractVersion = 0;
	}

	protected bool HasCommittedAdmissionAuthority(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sOperationId.IsEmpty() || mission.m_sManifestId.IsEmpty()
			|| mission.m_sSpawnResultId.IsEmpty())
			return false;

		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(mission.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		return operation && manifest && batch
			&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
			&& operation.m_iContractVersion == EXACT_CONTRACT_VERSION
			&& operation.m_sMissionInstanceId == mission.m_sInstanceId
			&& operation.m_sManifestId == mission.m_sManifestId
			&& operation.m_sSpawnResultId == mission.m_sSpawnResultId
			&& manifest.m_sOperationId == mission.m_sOperationId
			&& batch.m_sOperationId == mission.m_sOperationId
			&& batch.m_sManifestId == mission.m_sManifestId;
	}

	protected void ClearAttemptedAssetAuthorityLinks(
		HST_MissionAssetState asset,
		string operationId,
		string manifestId,
		string manifestSlotId,
		string assignedVehicleSlotId,
		string convoyElementId)
	{
		if (!asset)
			return;
		if (!operationId.IsEmpty() && asset.m_sOperationId == operationId)
			asset.m_sOperationId = "";
		if (!manifestId.IsEmpty() && asset.m_sManifestId == manifestId)
			asset.m_sManifestId = "";
		if (!manifestSlotId.IsEmpty() && asset.m_sManifestSlotId == manifestSlotId)
			asset.m_sManifestSlotId = "";
		if (!assignedVehicleSlotId.IsEmpty() && asset.m_sAssignedVehicleSlotId == assignedVehicleSlotId)
			asset.m_sAssignedVehicleSlotId = "";
		if (!convoyElementId.IsEmpty() && asset.m_sConvoyElementId == convoyElementId)
			asset.m_sConvoyElementId = "";
	}

	protected bool FailExactMissionContract(HST_CampaignState state, HST_ActiveMissionState mission, string failure)
	{
		if (!state || !mission)
			return false;
		string reason = "Exact mission convoy authority failure: " + failure;
		if (mission.m_sRuntimePhase == CONVOY_FAILED && mission.m_sRuntimeFailureReason == reason)
			return false;
		mission.m_sRuntimePhase = CONVOY_FAILED;
		mission.m_sRuntimeFailureReason = reason;
		mission.m_sLastRuntimeEventKey = CONVOY_FAIL_EVENT;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == mission.m_sInstanceId && !objective.m_bComplete)
				objective.m_bFailed = true;
		}
		Print(string.Format("Partisan exact mission convoy | %1 failed closed: %2", mission.m_sInstanceId, failure), LogLevel.ERROR);
		return true;
	}

	protected bool QuarantineAmbiguousAuthority(HST_CampaignState state, HST_ActiveMissionState mission, string reason)
	{
		if (!state || !mission)
			return false;
		if (reason.IsEmpty())
			reason = "exact mission convoy authority is ambiguous";

		string summary = "authority quarantined without mutating ambiguous operation, manifest, batch, group, asset, or element rows: " + reason;
		if (mission.m_iOperationContractVersion == QUARANTINED_CONTRACT_VERSION && mission.m_sConvoyOutcomeSummary == summary)
			return false;
		mission.m_iOperationContractVersion = QUARANTINED_CONTRACT_VERSION;
		mission.m_sConvoyOutcomeSummary = summary;
		return true;
	}

	protected bool IsTerminalRuntimePhase(HST_ActiveMissionState mission)
	{
		if (!mission)
			return true;
		return mission.m_sRuntimePhase == CONVOY_FAILED || mission.m_sRuntimePhase == CONVOY_ELIMINATED
			|| mission.m_sRuntimePhase == "convoy_arrived" || mission.m_sRuntimePhase == "completed"
			|| mission.m_sRuntimePhase == "expired";
	}

	protected HST_ActiveMissionState ResolveCanonicalExactMissionForInstance(
		HST_CampaignState state,
		string missionInstanceId)
	{
		if (!state || missionInstanceId.IsEmpty())
			return null;

		HST_ActiveMissionState mission;
		int missionClaimants;
		foreach (HST_ActiveMissionState candidateMission : state.m_aActiveMissions)
		{
			if (!candidateMission || candidateMission.m_sInstanceId != missionInstanceId)
				continue;
			mission = candidateMission;
			missionClaimants++;
		}
		if (missionClaimants != 1 || !IsExactMission(mission)
			|| mission.m_sOperationId.IsEmpty() || mission.m_sOperationId != BuildOperationId(mission))
			return null;
		if (CountOperationClaimants(state, mission) != 1)
			return null;

		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		if (!operation || operation.m_sOperationId != mission.m_sOperationId
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
			|| operation.m_iContractVersion != EXACT_CONTRACT_VERSION)
			return null;
		return mission;
	}

	protected int CountOperationClaimants(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int count;
		foreach (HST_OperationRecordState row : state.m_aOperations)
		{
			if (row && (row.m_sOperationId == mission.m_sOperationId || row.m_sMissionInstanceId == mission.m_sInstanceId))
				count++;
		}
		return count;
	}

	protected int CountManifestClaimants(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int count;
		foreach (HST_ForceManifestState row : state.m_aForceManifests)
		{
			if (row && (row.m_sManifestId == mission.m_sManifestId || row.m_sOperationId == mission.m_sOperationId
				|| (row.m_sForceKind == EXACT_FORCE_KIND && row.m_sCommandRequestId == mission.m_sInstanceId)))
				count++;
		}
		return count;
	}

	protected int CountBatchClaimants(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int count;
		foreach (HST_ForceSpawnResultState row : state.m_aForceSpawnResults)
		{
			if (row && (row.m_sResultId == mission.m_sSpawnResultId || row.m_sOperationId == mission.m_sOperationId
				|| row.m_sManifestId == mission.m_sManifestId || (row.m_sRequestId == mission.m_sInstanceId && row.m_sForceId == BuildForceId(mission))))
				count++;
		}
		return count;
	}

	protected int CountElementClaimants(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int count;
		foreach (HST_ConvoyElementState row : state.m_aConvoyElements)
		{
			if (row && (row.m_sOperationId == mission.m_sOperationId || row.m_sMissionInstanceId == mission.m_sInstanceId
				|| row.m_sManifestId == mission.m_sManifestId))
				count++;
		}
		return count;
	}

	protected int CountElementIdentityClaimants(HST_CampaignState state, HST_ActiveMissionState mission, int ordinal)
	{
		int count;
		string elementId = BuildElementId(mission, ordinal);
		HST_MissionAssetState asset = ResolveVehicleAsset(state, mission, ordinal);
		if (!asset)
			return count;
		string assetId = asset.m_sAssetId;
		string groupId = BuildGroupId(mission, ordinal);
		foreach (HST_ConvoyElementState row : state.m_aConvoyElements)
		{
			if (row && (row.m_sElementId == elementId || row.m_sVehicleAssetId == assetId || row.m_sGroupId == groupId))
				count++;
		}
		return count;
	}

	protected int CountAssetIdentityClaimants(HST_CampaignState state, HST_ActiveMissionState mission, int ordinal)
	{
		int count;
		HST_MissionAssetState asset = ResolveVehicleAsset(state, mission, ordinal);
		if (!asset)
			return count;
		string assetId = asset.m_sAssetId;
		string elementId = BuildElementId(mission, ordinal);
		string slotId = BuildVehicleSlotId(mission, ordinal);
		foreach (HST_MissionAssetState row : state.m_aMissionAssets)
		{
			if (row && (row.m_sAssetId == assetId || row.m_sManifestSlotId == slotId || row.m_sConvoyElementId == elementId) && row.m_sRole == VEHICLE_ROLE)
				count++;
		}
		return count;
	}

	protected int CountGroupIdentityClaimants(HST_CampaignState state, HST_ActiveMissionState mission, int ordinal)
	{
		int count;
		string groupId = BuildGroupId(mission, ordinal);
		string elementId = BuildElementId(mission, ordinal);
		HST_MissionAssetState asset = ResolveVehicleAsset(state, mission, ordinal);
		string assetId;
		if (asset)
			assetId = asset.m_sAssetId;
		foreach (HST_ActiveGroupState row : state.m_aActiveGroups)
		{
			if (row && (row.m_sGroupId == groupId || row.m_sConvoyElementId == elementId || (!assetId.IsEmpty() && row.m_sMissionAssetId == assetId)))
				count++;
		}
		return count;
	}

	protected int CountBatchSlotClaimants(HST_ForceSpawnResultState batch, string slotId)
	{
		int count;
		if (!batch || slotId.IsEmpty())
			return count;
		foreach (HST_ForceSpawnSlotResultState row : batch.m_aSlotResults)
		{
			if (row && row.m_sSlotId == slotId)
				count++;
		}
		return count;
	}

	protected int CountConvoyCargoAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int count;
		foreach (HST_MissionAssetState row : state.m_aMissionAssets)
		{
			if (row && row.m_sMissionInstanceId == mission.m_sInstanceId && (row.m_sRole == PAYLOAD_ROLE || row.m_sRole == CAPTIVE_ROLE))
				count++;
		}
		return count;
	}

	protected int CountCargoIdentityClaimants(HST_CampaignState state, HST_MissionAssetState cargo, HST_ForceManifestAssetState cargoSlot)
	{
		int count;
		foreach (HST_MissionAssetState row : state.m_aMissionAssets)
		{
			if (row && (row.m_sAssetId == cargo.m_sAssetId || row.m_sManifestSlotId == cargoSlot.m_sSlotId))
				count++;
		}
		return count;
	}

	protected float ResolveNearestLivingPlayerDistance(vector position)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || IsZeroVector(position))
			return -1.0;
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		float best = -1.0;
		foreach (int playerId : playerIds)
		{
			IEntity entity = playerManager.GetPlayerControlledEntity(playerId);
			if (!entity)
				entity = SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
			if (!IsLivingEntity(entity))
				continue;
			float distance = Distance2D(position, entity.GetOrigin());
			if (best < 0 || distance < best)
				best = distance;
		}
		return best;
	}

	protected float ResolveNearestLivingPlayerDistanceForConvoy(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation)
	{
		if (!state || !mission || !operation)
			return -1.0;

		float best = ResolveNearestLivingPlayerDistance(operation.m_vStrategicPosition);
		for (int i = 0; i < EXACT_VEHICLE_COUNT; i++)
		{
			HST_ConvoyElementState element = state.FindConvoyElement(BuildElementId(mission, i));
			if (!element || IsZeroVector(element.m_vCurrentPosition))
				continue;
			bool relevantRoot = element.m_iSurvivingCrewCount > 0;
			if (!relevantRoot)
			{
				HST_MissionAssetState vehicle = ResolveVehicleAsset(state, mission, i);
				relevantRoot = IsRecoverableAbandonedVehicleRoot(vehicle, element);
			}
			if (!relevantRoot)
				continue;
			float distance = ResolveNearestLivingPlayerDistance(element.m_vCurrentPosition);
			if (distance >= 0 && (best < 0 || distance < best))
				best = distance;
		}
		HST_MissionAssetState cargo = ResolveConvoyCargoAsset(state, mission);
		if (cargo && !cargo.m_bPickedUp && !cargo.m_bDelivered && !cargo.m_bDestroyed
			&& !cargo.m_bOutcomeApplied && !IsZeroVector(cargo.m_vCurrentPosition))
		{
			float cargoDistance = ResolveNearestLivingPlayerDistance(cargo.m_vCurrentPosition);
			if (cargoDistance >= 0 && (best < 0 || cargoDistance < best))
				best = cargoDistance;
		}
		return best;
	}

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity)
			return false;
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected int BuildDeterministicSeed(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		string canonical = string.Format("%1|%2|%3", state.m_iCampaignSeed, mission.m_sInstanceId, mission.m_sMissionId);
		return canonical.Hash();
	}

	static string BuildOperationId(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "";
		return HST_StableIdService.BuildOperationId("mission_convoy", mission.m_sInstanceId);
	}

	static string BuildManifestId(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "";
		return "manifest_mission_convoy_" + mission.m_sInstanceId;
	}

	static string BuildSpawnResultId(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "";
		return "spawn_mission_convoy_" + mission.m_sInstanceId;
	}

	static string BuildForceId(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "";
		return "force_mission_convoy_" + mission.m_sInstanceId;
	}

	static string BuildProjectionId(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "";
		return "projection_mission_convoy_" + mission.m_sInstanceId;
	}

	static string BuildGroupId(HST_ActiveMissionState mission, int ordinal)
	{
		if (!mission)
			return "";
		return string.Format("mission_convoy_%1_%2", mission.m_sInstanceId, ordinal);
	}

	static string BuildElementId(HST_ActiveMissionState mission, int ordinal)
	{
		if (!mission)
			return "";
		return string.Format("convoy_element_%1_%2", mission.m_sInstanceId, ordinal);
	}

	static string BuildCrewGroupElementId(HST_ActiveMissionState mission, int ordinal)
	{
		return BuildElementId(mission, ordinal) + "_crew";
	}

	static string BuildVehicleSlotId(HST_ActiveMissionState mission, int ordinal)
	{
		return BuildElementId(mission, ordinal) + "_vehicle";
	}

	static string BuildMemberSlotId(HST_ActiveMissionState mission, int vehicleOrdinal, int crewOrdinal)
	{
		return string.Format("%1_member_%2", BuildElementId(mission, vehicleOrdinal), crewOrdinal);
	}

	static string BuildCargoSlotId(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset)
			return "";
		return string.Format("convoy_asset_%1_%2", mission.m_sInstanceId, asset.m_sRole);
	}

	protected float Distance2D(vector a, vector b)
	{
		return Math.Sqrt(DistanceSq2D(a, b));
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}

	protected bool VectorsNear(vector left, vector right, float toleranceMeters)
	{
		float tolerance = Math.Max(0.0, toleranceMeters);
		return Math.AbsFloat(left[0] - right[0]) <= tolerance
			&& Math.AbsFloat(left[1] - right[1]) <= tolerance
			&& Math.AbsFloat(left[2] - right[2]) <= tolerance;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}
}
