class HST_ActiveGroupLifecycleProofReport
{
	bool m_bCrewlessMixedGroupTerminalExact;
	bool m_bCapturePresenceCleared;
	bool m_bQRFMarkerRetired;
	bool m_bPersistenceExact;
	bool m_bCombatControlsExact;
	string m_sTerminalEvidence;
	string m_sCaptureEvidence;
	string m_sMarkerEvidence;
	string m_sPersistenceEvidence;
	string m_sControlEvidence;
}

class HST_ActiveGroupLifecyclePhysicalWarProofHarness : HST_PhysicalWarService
{
	bool ApplyPersonnelElimination(HST_CampaignState state, HST_ActiveGroupState activeGroup, string source)
	{
		return ApplyObservedPersonnelElimination(state, activeGroup, source);
	}

	bool IsCombatEffective(HST_ActiveGroupState activeGroup)
	{
		return IsActiveGroupCombatEffective(activeGroup);
	}

	bool ShouldEliminateCrewlessMixedGroup(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		int livingInfantry,
		int deadTrackedMembers,
		bool nativeDelayedPopulationActive,
		bool liveCountGraceActive)
	{
		return ShouldApplyCrewlessMixedPersonnelElimination(
			state,
			activeGroup,
			livingInfantry,
			deadTrackedMembers,
			nativeDelayedPopulationActive,
			liveCountGraceActive);
	}

	bool ShouldPreservePersistentVehicleRecord(HST_RuntimeVehicleState runtimeVehicle)
	{
		return ShouldPreservePersistentDetachedVehicleRecord(runtimeVehicle);
	}
}

class HST_ActiveGroupLifecycleMarkerProofHarness : HST_MapMarkerService
{
	bool IsQRFMarkerVisible(HST_CampaignState state, HST_QRFState qrf)
	{
		return ShouldShowQRFMarker(state, qrf);
	}
}

class HST_ActiveGroupLifecycleProofService
{
	static const string TARGET_ZONE_ID = "lifecycle_target";
	static const string GROUP_ID = "lifecycle_mixed_qrf_group";
	static const string QRF_ID = "lifecycle_mixed_qrf";
	static const string FOLDED_CONTROL_GROUP_ID = "lifecycle_folded_control";
	static const string SESSION_SALVAGE_RUNTIME_ID = "lifecycle_session_salvage";
	static const string PERSISTENT_SALVAGE_RUNTIME_ID = "lifecycle_persistent_salvage";

	HST_ActiveGroupLifecycleProofReport Run()
	{
		HST_ActiveGroupLifecycleProofReport report = new HST_ActiveGroupLifecycleProofReport();
		HST_CampaignState state = BuildState();
		HST_ActiveGroupState group = state.FindActiveGroup(GROUP_ID);
		HST_QRFState qrf = FindQRF(state, QRF_ID);
		HST_CampaignPreset preset = BuildPreset();
		HST_BalanceConfig balance = new HST_BalanceConfig();
		HST_ZoneCaptureService capture = new HST_ZoneCaptureService();
		HST_ActiveGroupLifecyclePhysicalWarProofHarness physicalWar = new HST_ActiveGroupLifecyclePhysicalWarProofHarness();
		HST_ActiveGroupLifecycleMarkerProofHarness markers = new HST_ActiveGroupLifecycleMarkerProofHarness();

		HST_ZoneCaptureStatus captureBefore = capture.BuildCaptureStatus(state, preset, balance, state.FindZone(TARGET_ZONE_ID));
		bool markerBefore = markers.IsQRFMarkerVisible(state, qrf);
		HST_ActiveGroupState eligibilityLive = BuildEligibilityControl("eligibility_live");
		HST_ActiveGroupState eligibilityPending = BuildEligibilityControl("eligibility_pending");
		eligibilityPending.m_sRuntimeStatus = "spawn_pending_agents";
		HST_ActiveGroupState eligibilityQueueOwned = BuildEligibilityControl("eligibility_queue_owned");
		eligibilityQueueOwned.m_sSpawnResultId = "eligibility_queue_result";
		HST_ActiveGroupState eligibilityConvoy = BuildEligibilityControl("mission_convoy_eligibility_control");
		HST_ActiveGroupState eligibilityUnobserved = BuildEligibilityControl("eligibility_unobserved");
		eligibilityUnobserved.m_iSpawnedAgentCount = 0;
		eligibilityUnobserved.m_iDurableLivingInfantryCount = 0;
		eligibilityUnobserved.m_bEverPopulated = false;
		eligibilityUnobserved.m_bSpawnCompleted = false;
		bool eligibilityAccepted = physicalWar.ShouldEliminateCrewlessMixedGroup(state, group, 0, 0, false, false);
		bool eligibilityRejectedLive = !physicalWar.ShouldEliminateCrewlessMixedGroup(state, eligibilityLive, 1, 0, false, false);
		bool eligibilityRejectedPending = !physicalWar.ShouldEliminateCrewlessMixedGroup(state, eligibilityPending, 0, 0, false, false);
		bool eligibilityRejectedQueue = !physicalWar.ShouldEliminateCrewlessMixedGroup(state, eligibilityQueueOwned, 0, 0, false, false);
		bool eligibilityRejectedConvoy = !physicalWar.ShouldEliminateCrewlessMixedGroup(state, eligibilityConvoy, 0, 0, false, false);
		bool eligibilityRejectedUnobserved = !physicalWar.ShouldEliminateCrewlessMixedGroup(state, eligibilityUnobserved, 0, 0, false, false);
		bool eligibilityRejectedDelayed = !physicalWar.ShouldEliminateCrewlessMixedGroup(state, eligibilityLive, 0, 0, true, false);
		bool eligibilityRejectedGrace = !physicalWar.ShouldEliminateCrewlessMixedGroup(state, eligibilityLive, 0, 0, false, true);
		HST_RuntimeVehicleState persistentVehicleControl = BuildVehicleRecordControl("field_vehicle", false, false);
		HST_RuntimeVehicleState sessionVehicleControl = BuildVehicleRecordControl("detached_active_vehicle", true, false);
		bool persistentVehiclePolicyPreserved = physicalWar.ShouldPreservePersistentVehicleRecord(persistentVehicleControl);
		bool sessionVehiclePolicyNotPersistent = !physicalWar.ShouldPreservePersistentVehicleRecord(sessionVehicleControl);
		bool firstApply = physicalWar.ApplyPersonnelElimination(state, group, "deterministic lifecycle proof");
		bool secondApply = physicalWar.ApplyPersonnelElimination(state, group, "deterministic lifecycle proof replay");
		HST_ZoneCaptureStatus captureAfter = capture.BuildCaptureStatus(state, preset, balance, state.FindZone(TARGET_ZONE_ID));

		bool terminalAppliedOnce = eligibilityAccepted && firstApply && !secondApply && group && qrf;
		bool terminalIdentityExact = group && group.m_sRuntimeStatus == "eliminated"
			&& !group.m_bSpawnedEntity && group.m_sRuntimeEntityId.IsEmpty();
		bool terminalCountsExact = group && group.m_iSpawnedAgentCount == 0
			&& group.m_iLastSeenAliveCount == 0
			&& group.m_iSurvivorInfantryCount == 0
			&& group.m_iSurvivorVehicleCount == 0;
		bool terminalHistoryExact = group && group.m_iEliminatedAtSecond == state.m_iElapsedSeconds
			&& group.m_bEverPopulated && group.m_bSpawnCompleted;
		bool terminalQRFExact = qrf && qrf.m_bResolved && !qrf.m_bSucceeded;
		report.m_bCrewlessMixedGroupTerminalExact = terminalAppliedOnce
			&& terminalIdentityExact
			&& terminalCountsExact
			&& terminalHistoryExact
			&& terminalQRFExact;
		report.m_sTerminalEvidence = string.Format(
			"apply/replay %1/%2 | status %3 | spawned/last/infantry/vehicle %4/%5/%6/%7",
			firstApply,
			secondApply,
			ResolveGroupStatus(group),
			ResolveSpawnedCount(group),
			ResolveLastSeenCount(group),
			ResolveSurvivorInfantry(group),
			ResolveSurvivorVehicles(group));
		report.m_sTerminalEvidence = report.m_sTerminalEvidence + string.Format(
			" | populated/completed %1/%2 | QRF resolved/succeeded %3/%4",
			group && group.m_bEverPopulated,
			group && group.m_bSpawnCompleted,
			qrf && qrf.m_bResolved,
			qrf && qrf.m_bSucceeded);

		report.m_bCapturePresenceCleared = captureBefore && captureAfter
			&& captureBefore.m_iEnemyVehicleCountNearby == 1
			&& captureBefore.m_sBlockedReason == "hostile vehicles remain"
			&& captureAfter.m_iEnemyVehicleCountNearby == 0
			&& captureAfter.m_iEnemyCountNearby == 0
			&& captureAfter.m_sBlockedReason.IsEmpty();
		report.m_sCaptureEvidence = string.Format(
			"enemy infantry/vehicle %1/%2 -> %3/%4 | blocked %5 -> %6",
			ResolveEnemyInfantry(captureBefore),
			ResolveEnemyVehicles(captureBefore),
			ResolveEnemyInfantry(captureAfter),
			ResolveEnemyVehicles(captureAfter),
			ResolveBlockedReason(captureBefore),
			ResolveBlockedReason(captureAfter));

		bool markerAfterResolved = markers.IsQRFMarkerVisible(state, qrf);
		qrf.m_bResolved = false;
		bool markerAfterUnresolvedReplay = markers.IsQRFMarkerVisible(state, qrf);
		qrf.m_bResolved = true;
		report.m_bQRFMarkerRetired = markerBefore && !markerAfterResolved && !markerAfterUnresolvedReplay;
		report.m_sMarkerEvidence = string.Format(
			"active unresolved %1 | terminal resolved %2 | terminal unresolved replay %3",
			markerBefore,
			markerAfterResolved,
			markerAfterUnresolvedReplay);

		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.Capture(state);
		HST_CampaignState restored = saveData.Restore();
		HST_ActiveGroupState restoredGroup;
		HST_ActiveGroupState restoredFoldedGroup;
		HST_QRFState restoredQRF;
		if (restored)
		{
			restoredGroup = restored.FindActiveGroup(GROUP_ID);
			restoredFoldedGroup = restored.FindActiveGroup(FOLDED_CONTROL_GROUP_ID);
			restoredQRF = FindQRF(restored, QRF_ID);
		}
		bool restoredIdentityExact = restored && restoredGroup && restoredFoldedGroup && restoredQRF
			&& restored.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION;
		bool restoredTerminalExact = restoredGroup && restoredGroup.m_sRuntimeStatus == "eliminated"
			&& !restoredGroup.m_bSpawnedEntity && restoredGroup.m_sRuntimeEntityId.IsEmpty();
		bool restoredTerminalCountsExact = restoredGroup && restoredGroup.m_iLastSeenAliveCount == 0
			&& restoredGroup.m_iSurvivorInfantryCount == 0
			&& restoredGroup.m_iSurvivorVehicleCount == 0;
		bool restoredQRFExact = restoredQRF && restoredQRF.m_bResolved && !restoredQRF.m_bSucceeded;
		bool restoredFoldedExact = restoredFoldedGroup && restoredFoldedGroup.m_sRuntimeStatus == "folded"
			&& restoredFoldedGroup.m_iSurvivorInfantryCount == 2
			&& restoredFoldedGroup.m_iSurvivorVehicleCount == 1;
		bool sessionSalvagePruned = restored
			&& restored.FindRuntimeVehicle(SESSION_SALVAGE_RUNTIME_ID) == null
			&& CountVehicleCargoForRuntime(restored, SESSION_SALVAGE_RUNTIME_ID) == 0;
		bool persistentSalvageRetained = restored
			&& restored.FindRuntimeVehicle(PERSISTENT_SALVAGE_RUNTIME_ID) != null
			&& CountVehicleCargoForRuntime(restored, PERSISTENT_SALVAGE_RUNTIME_ID) == 1;
		report.m_bPersistenceExact = restoredIdentityExact
			&& restoredTerminalExact
			&& restoredTerminalCountsExact
			&& restoredQRFExact
			&& restoredFoldedExact
			&& sessionSalvagePruned
			&& persistentSalvageRetained;
		report.m_sPersistenceEvidence = string.Format(
			"schema %1 | status %2 | spawned %3 | last/infantry/vehicle %4/%5/%6 | QRF resolved/succeeded %7/%8",
			ResolveSchema(restored),
			ResolveGroupStatus(restoredGroup),
			restoredGroup && restoredGroup.m_bSpawnedEntity,
			ResolveLastSeenCount(restoredGroup),
			ResolveSurvivorInfantry(restoredGroup),
			ResolveSurvivorVehicles(restoredGroup),
			restoredQRF && restoredQRF.m_bResolved,
			restoredQRF && restoredQRF.m_bSucceeded);
		report.m_sPersistenceEvidence = report.m_sPersistenceEvidence + string.Format(
			" | folded survivors %1/%2 | session salvage/cargo retained %3/%4",
			ResolveSurvivorInfantry(restoredFoldedGroup),
			ResolveSurvivorVehicles(restoredFoldedGroup),
			restored && restored.FindRuntimeVehicle(SESSION_SALVAGE_RUNTIME_ID) != null,
			CountVehicleCargoForRuntime(restored, SESSION_SALVAGE_RUNTIME_ID));
		report.m_sPersistenceEvidence = report.m_sPersistenceEvidence + string.Format(
			" | durable salvage/cargo retained %1/%2",
			restored && restored.FindRuntimeVehicle(PERSISTENT_SALVAGE_RUNTIME_ID) != null,
			CountVehicleCargoForRuntime(restored, PERSISTENT_SALVAGE_RUNTIME_ID));

		HST_ActiveGroupState liveMixed = BuildCombatControl("control_live_mixed", 2, 1, 1, 1, 1);
		HST_ActiveGroupState crewlessMixed = BuildCombatControl("control_crewless_mixed", 2, 1, 1, 0, 1);
		HST_ActiveGroupState vehicleOnly = BuildCombatControl("control_vehicle_only", 0, 1, 1, 0, 1);
		bool liveMixedEffective = physicalWar.IsCombatEffective(liveMixed);
		bool crewlessMixedEffective = physicalWar.IsCombatEffective(crewlessMixed);
		bool vehicleOnlyEffective = physicalWar.IsCombatEffective(vehicleOnly);
		bool combatControlsExact = liveMixedEffective && !crewlessMixedEffective && vehicleOnlyEffective;
		bool eligibilityIdentityControlsExact = eligibilityAccepted
			&& eligibilityRejectedLive
			&& eligibilityRejectedPending
			&& eligibilityRejectedQueue
			&& eligibilityRejectedConvoy;
		bool eligibilityTimingControlsExact = eligibilityRejectedUnobserved
			&& eligibilityRejectedDelayed
			&& eligibilityRejectedGrace;
		bool vehicleRecordPolicyExact = persistentVehiclePolicyPreserved && sessionVehiclePolicyNotPersistent;
		report.m_bCombatControlsExact = combatControlsExact
			&& eligibilityIdentityControlsExact
			&& eligibilityTimingControlsExact
			&& vehicleRecordPolicyExact;
		report.m_sControlEvidence = string.Format(
			"live mixed %1 | crewless mixed vehicle survivor %2 | vehicle-only projection %3",
			liveMixedEffective,
			crewlessMixedEffective,
			vehicleOnlyEffective);
		report.m_sControlEvidence = report.m_sControlEvidence + string.Format(
			" | eligible %1 | rejected live/pending/queue/convoy %2/%3/%4/%5",
			eligibilityAccepted,
			eligibilityRejectedLive,
			eligibilityRejectedPending,
			eligibilityRejectedQueue,
			eligibilityRejectedConvoy);
		report.m_sControlEvidence = report.m_sControlEvidence + string.Format(
			" | rejected unobserved/delayed/grace %1/%2/%3",
			eligibilityRejectedUnobserved,
			eligibilityRejectedDelayed,
			eligibilityRejectedGrace);
		report.m_sControlEvidence = report.m_sControlEvidence + string.Format(
			" | durable/session salvage policy %1/%2",
			persistentVehiclePolicyPreserved,
			sessionVehiclePolicyNotPersistent);
		return report;
	}

	protected HST_CampaignState BuildState()
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_iElapsedSeconds = 420;
		state.m_iTrainingLevel = 1;

		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = TARGET_ZONE_ID;
		zone.m_sDisplayName = "Lifecycle Target";
		zone.m_sOwnerFactionKey = "US";
		zone.m_eType = HST_EZoneType.HST_ZONE_OUTPOST;
		zone.m_vPosition = "100 0 100";
		zone.m_iCaptureRadiusMeters = 500;
		state.m_aZones.Insert(zone);

		HST_ActiveGroupState friendly = new HST_ActiveGroupState();
		friendly.m_sGroupId = "lifecycle_friendly";
		friendly.m_sZoneId = TARGET_ZONE_ID;
		friendly.m_sFactionKey = "FIA";
		friendly.m_sRuntimeStatus = "active";
		friendly.m_vPosition = zone.m_vPosition;
		friendly.m_iInfantryCount = 1;
		friendly.m_iOriginalInfantryCount = 1;
		friendly.m_iSpawnedAgentCount = 1;
		friendly.m_iLastSeenAliveCount = 1;
		friendly.m_iSurvivorInfantryCount = 1;
		friendly.m_bSpawnedEntity = true;
		state.m_aActiveGroups.Insert(friendly);

		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = GROUP_ID;
		group.m_sQRFInstanceId = QRF_ID;
		group.m_sZoneId = TARGET_ZONE_ID;
		group.m_sFactionKey = "US";
		group.m_sRuntimeEntityId = GROUP_ID;
		group.m_sRuntimeStatus = "arrived";
		group.m_vPosition = zone.m_vPosition;
		group.m_iInfantryCount = 2;
		group.m_iVehicleCount = 1;
		group.m_iOriginalInfantryCount = 2;
		group.m_iOriginalVehicleCount = 1;
		group.m_iSpawnedAgentCount = 2;
		group.m_iLastSeenAliveCount = 1;
		group.m_iSurvivorInfantryCount = 0;
		group.m_iSurvivorVehicleCount = 1;
		group.m_iSpawnedAtSecond = 100;
		group.m_bQRF = true;
		group.m_bSpawnAttempted = true;
		group.m_bSpawnedEntity = true;
		group.m_bEverPopulated = true;
		group.m_bSpawnCompleted = true;
		state.m_aActiveGroups.Insert(group);

		HST_QRFState qrf = new HST_QRFState();
		qrf.m_sInstanceId = QRF_ID;
		qrf.m_sFactionKey = "US";
		qrf.m_sTargetZoneId = TARGET_ZONE_ID;
		qrf.m_sGroupId = GROUP_ID;
		qrf.m_iStartedAtSecond = 100;
		qrf.m_iETASeconds = 600;
		state.m_aQRFs.Insert(qrf);

		HST_ActiveGroupState foldedControl = new HST_ActiveGroupState();
		foldedControl.m_sGroupId = FOLDED_CONTROL_GROUP_ID;
		foldedControl.m_sZoneId = TARGET_ZONE_ID;
		foldedControl.m_sFactionKey = "US";
		foldedControl.m_sRuntimeStatus = "folded";
		foldedControl.m_iInfantryCount = 4;
		foldedControl.m_iVehicleCount = 1;
		foldedControl.m_iLastSeenAliveCount = 3;
		foldedControl.m_iSurvivorInfantryCount = 2;
		foldedControl.m_iSurvivorVehicleCount = 1;
		state.m_aActiveGroups.Insert(foldedControl);

		HST_RuntimeVehicleState sessionSalvage = new HST_RuntimeVehicleState();
		sessionSalvage.m_sVehicleRuntimeId = SESSION_SALVAGE_RUNTIME_ID;
		sessionSalvage.m_sPrefab = "session_salvage_vehicle";
		sessionSalvage.m_sRuntimeKind = "detached_active_vehicle";
		sessionSalvage.m_bDetached = true;
		state.m_aRuntimeVehicles.Insert(sessionSalvage);

		HST_VehicleCargoItemState sessionCargo = new HST_VehicleCargoItemState();
		sessionCargo.m_sVehicleRuntimeId = SESSION_SALVAGE_RUNTIME_ID;
		sessionCargo.m_sItemPrefab = "session_salvage_cargo";
		sessionCargo.m_iCount = 1;
		state.m_aVehicleCargoItems.Insert(sessionCargo);

		HST_RuntimeVehicleState persistentSalvage = new HST_RuntimeVehicleState();
		persistentSalvage.m_sVehicleRuntimeId = PERSISTENT_SALVAGE_RUNTIME_ID;
		persistentSalvage.m_sPrefab = "persistent_salvage_vehicle";
		persistentSalvage.m_sRuntimeKind = "field_vehicle";
		persistentSalvage.m_bDetached = false;
		state.m_aRuntimeVehicles.Insert(persistentSalvage);

		HST_VehicleCargoItemState persistentCargo = new HST_VehicleCargoItemState();
		persistentCargo.m_sVehicleRuntimeId = PERSISTENT_SALVAGE_RUNTIME_ID;
		persistentCargo.m_sItemPrefab = "persistent_salvage_cargo";
		persistentCargo.m_iCount = 1;
		state.m_aVehicleCargoItems.Insert(persistentCargo);
		return state;
	}

	protected HST_CampaignPreset BuildPreset()
	{
		HST_CampaignPreset preset = new HST_CampaignPreset();
		preset.m_sPresetId = "lifecycle_proof";
		preset.m_sResistanceFactionKey = "FIA";
		preset.m_sOccupierFactionKey = "US";
		preset.m_sInvaderFactionKey = "USSR";
		return preset;
	}

	protected HST_ActiveGroupState BuildCombatControl(string groupId, int infantryCount, int vehicleCount, int spawnedCount, int survivorInfantry, int survivorVehicles)
	{
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = groupId;
		group.m_sRuntimeStatus = "active";
		group.m_iInfantryCount = infantryCount;
		group.m_iVehicleCount = vehicleCount;
		group.m_iSpawnedAgentCount = spawnedCount;
		group.m_iLastSeenAliveCount = survivorInfantry;
		if (infantryCount <= 0)
			group.m_iLastSeenAliveCount = survivorVehicles;
		group.m_iSurvivorInfantryCount = survivorInfantry;
		group.m_iSurvivorVehicleCount = survivorVehicles;
		return group;
	}

	protected HST_ActiveGroupState BuildEligibilityControl(string groupId)
	{
		HST_ActiveGroupState group = BuildCombatControl(groupId, 2, 1, 2, 1, 1);
		group.m_bSpawnAttempted = true;
		group.m_bSpawnedEntity = true;
		group.m_bEverPopulated = true;
		group.m_bSpawnCompleted = true;
		group.m_iDurableLivingInfantryCount = 1;
		return group;
	}

	protected HST_RuntimeVehicleState BuildVehicleRecordControl(string runtimeKind, bool detached, bool deleted)
	{
		HST_RuntimeVehicleState vehicle = new HST_RuntimeVehicleState();
		vehicle.m_sRuntimeKind = runtimeKind;
		vehicle.m_bDetached = detached;
		vehicle.m_bDeleted = deleted;
		return vehicle;
	}

	protected HST_QRFState FindQRF(HST_CampaignState state, string instanceId)
	{
		if (!state)
			return null;

		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (qrf && qrf.m_sInstanceId == instanceId)
				return qrf;
		}

		return null;
	}

	protected string ResolveGroupStatus(HST_ActiveGroupState group)
	{
		if (!group)
			return "missing";
		return group.m_sRuntimeStatus;
	}

	protected int ResolveSpawnedCount(HST_ActiveGroupState group)
	{
		if (!group)
			return -1;
		return group.m_iSpawnedAgentCount;
	}

	protected int ResolveLastSeenCount(HST_ActiveGroupState group)
	{
		if (!group)
			return -1;
		return group.m_iLastSeenAliveCount;
	}

	protected int ResolveSurvivorInfantry(HST_ActiveGroupState group)
	{
		if (!group)
			return -1;
		return group.m_iSurvivorInfantryCount;
	}

	protected int ResolveSurvivorVehicles(HST_ActiveGroupState group)
	{
		if (!group)
			return -1;
		return group.m_iSurvivorVehicleCount;
	}

	protected int ResolveEnemyInfantry(HST_ZoneCaptureStatus status)
	{
		if (!status)
			return -1;
		return status.m_iEnemyCountNearby;
	}

	protected int ResolveEnemyVehicles(HST_ZoneCaptureStatus status)
	{
		if (!status)
			return -1;
		return status.m_iEnemyVehicleCountNearby;
	}

	protected string ResolveBlockedReason(HST_ZoneCaptureStatus status)
	{
		if (!status)
			return "missing";
		return status.m_sBlockedReason;
	}

	protected int ResolveSchema(HST_CampaignState state)
	{
		if (!state)
			return -1;
		return state.m_iSchemaVersion;
	}

	protected int CountVehicleCargoForRuntime(HST_CampaignState state, string runtimeId)
	{
		if (!state || runtimeId.IsEmpty())
			return 0;

		int count;
		foreach (HST_VehicleCargoItemState cargoItem : state.m_aVehicleCargoItems)
		{
			if (cargoItem && cargoItem.m_sVehicleRuntimeId == runtimeId)
				count++;
		}
		return count;
	}
}
