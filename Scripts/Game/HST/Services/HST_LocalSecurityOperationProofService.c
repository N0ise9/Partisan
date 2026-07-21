#ifdef ENABLE_DIAG
class HST_LocalSecurityOperationProofReport
{
	bool m_bCatalogExact;
	bool m_bAdmissionReplayExact;
	bool m_bEligibilityIsolationExact;
	bool m_bSurvivorRestoreExact;
	bool m_bDestructionReplayExact;
	bool m_bNonLossSettlementExact;
	bool m_bOwnerEpochRearmExact;
	bool m_bConflictQuarantineExact;
	string m_sCatalogEvidence;
	string m_sAdmissionReplayEvidence;
	string m_sEligibilityIsolationEvidence;
	string m_sSurvivorRestoreEvidence;
	string m_sDestructionReplayEvidence;
	string m_sNonLossSettlementEvidence;
	string m_sOwnerEpochRearmEvidence;
	string m_sConflictQuarantineEvidence;
}

class HST_LocalSecurityOperationProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_ForceSpawnQueueService m_Queue;
	ref HST_ForceSpawnAdapterService m_Adapter;
	ref HST_PhysicalWarService m_PhysicalWar;
	ref HST_TownInfluenceService m_TownInfluence;
	ref HST_LocalSecurityOperationService m_Service;
	ref HST_ZoneState m_Zone;
	ref HST_CivilianZoneState m_Civilian;
	ref HST_FactionPoolState m_Pool;
	ref HST_LocalSecurityPatrolState m_Patrol;
	ref HST_OperationRecordState m_Operation;
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
	ref HST_ActiveGroupState m_Group;
	bool m_bInitialTickChanged;
	string m_sFailureReason;
}

class HST_LocalSecurityOperationProofFixtureFactory
{
	HST_LocalSecurityOperationProofFixture Build(
		string suffix,
		HST_EZoneType zoneType,
		string ownerFactionKey,
		int policeStrength)
	{
		HST_LocalSecurityOperationProofFixture fixture = new HST_LocalSecurityOperationProofFixture();
		fixture.m_State = new HST_CampaignState();
		fixture.m_State.m_iCampaignSeed = 666066;
		fixture.m_State.m_iElapsedSeconds = 100;
		fixture.m_State.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		fixture.m_Preset = HST_DefaultCatalog.CreateVanillaEveronPreset();
		fixture.m_Zone = BuildZone(suffix, zoneType, ownerFactionKey);
		fixture.m_Civilian = BuildCivilian(fixture.m_Zone.m_sZoneId, policeStrength);
		fixture.m_Pool = BuildPool(ownerFactionKey);
		fixture.m_State.m_aZones.Insert(fixture.m_Zone);
		fixture.m_State.m_aCivilianZones.Insert(fixture.m_Civilian);
		fixture.m_State.m_aFactionPools.Insert(fixture.m_Pool);

		fixture.m_Queue = new HST_ForceSpawnQueueService();
		fixture.m_Adapter = new HST_ForceSpawnAdapterService();
		fixture.m_PhysicalWar = new HST_PhysicalWarService();
		fixture.m_TownInfluence = new HST_TownInfluenceService();
		fixture.m_TownInfluence.SetCampaignPreset(fixture.m_Preset);
		fixture.m_TownInfluence.EnsureRecords(fixture.m_State);
		fixture.m_Service = new HST_LocalSecurityOperationService();
		fixture.m_Service.SetRuntimeServices(
			fixture.m_Queue,
			fixture.m_Adapter,
			fixture.m_PhysicalWar,
			fixture.m_TownInfluence);
		return fixture;
	}

	bool Admit(HST_LocalSecurityOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_State || !fixture.m_Service)
			return false;
		fixture.m_bInitialTickChanged = fixture.m_Service.Tick(fixture.m_State, fixture.m_Preset);
		fixture.m_Patrol = fixture.m_State.FindLocalSecurityPatrol(fixture.m_Zone.m_sZoneId);
		if (!fixture.m_Patrol)
		{
			fixture.m_sFailureReason = "local-security proof admission produced no envelope";
			return false;
		}
		fixture.m_Operation = fixture.m_State.FindOperation(fixture.m_Patrol.m_sOperationId);
		fixture.m_Manifest = fixture.m_State.FindForceManifest(fixture.m_Patrol.m_sManifestId);
		fixture.m_Batch = fixture.m_State.FindForceSpawnResult(fixture.m_Patrol.m_sSpawnResultId);
		fixture.m_Group = fixture.m_State.FindActiveGroup(fixture.m_Patrol.m_sGroupId);
		if (!fixture.m_Operation || !fixture.m_Manifest || !fixture.m_Batch || !fixture.m_Group)
		{
			fixture.m_sFailureReason = "local-security proof admission graph is incomplete";
			return false;
		}
		return true;
	}

	protected HST_ZoneState BuildZone(
		string suffix,
		HST_EZoneType zoneType,
		string ownerFactionKey)
	{
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = "local_security_proof_zone_" + suffix;
		zone.m_sDisplayName = "Local Security Proof " + suffix;
		zone.m_sOwnerFactionKey = ownerFactionKey;
		zone.m_iOwnershipContractVersion = HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION;
		zone.m_iOwnershipRevision = 1;
		zone.m_eType = zoneType;
		zone.m_vPosition = "6000 25 6000";
		zone.m_bActive = false;
		return zone;
	}

	protected HST_CivilianZoneState BuildCivilian(string zoneId, int policeStrength)
	{
		HST_CivilianZoneState civilian = new HST_CivilianZoneState();
		civilian.m_sZoneId = zoneId;
		civilian.m_iFIASupport = 30;
		civilian.m_iOccupierSupport = 70;
		civilian.m_iCivilianPresence = 20;
		civilian.m_iPolicePresence = Math.Max(0, policeStrength);
		civilian.m_iPopulationRemaining = 160;
		return civilian;
	}

	protected HST_FactionPoolState BuildPool(string factionKey)
	{
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = factionKey;
		if (factionKey == "USSR" || factionKey == "US")
		{
			pool.m_iStrategicContractVersion = HST_EnemyStrategicResourceService.CONTRACT_VERSION;
			pool.m_iStrategicRevision = 1;
		}
		pool.m_iAttackResources = 100;
		pool.m_iSupportResources = 100;
		pool.m_iMoney = 100;
		pool.m_iHR = 100;
		pool.m_iAggression = 50;
		return pool;
	}
}

class HST_LocalSecurityOperationProofService
{
	protected ref HST_LocalSecurityOperationProofFixtureFactory m_Fixtures
		= new HST_LocalSecurityOperationProofFixtureFactory();

	HST_LocalSecurityOperationProofReport Run()
	{
		HST_LocalSecurityOperationProofReport report = new HST_LocalSecurityOperationProofReport();
		ProveCatalog(report);
		ProveAdmissionReplay(report);
		ProveEligibilityIsolation(report);
		ProveSurvivorRestore(report);
		ProveDestructionReplay(report);
		ProveNonLossSettlements(report);
		ProveOwnerEpochRearm(report);
		ProveConflictQuarantine(report);
		return report;
	}

	protected void ProveCatalog(HST_LocalSecurityOperationProofReport report)
	{
		HST_LocalSecurityCatalogService catalog = new HST_LocalSecurityCatalogService();
		bool exact = true;
		string evidence;
		for (int memberCount = 2; memberCount <= 5; memberCount++)
		{
			HST_LocalSecurityCatalogResult us = catalog.ResolveAuthoredGroup("US", memberCount - 1);
			HST_LocalSecurityCatalogResult ussr = catalog.ResolveAuthoredGroup("USSR", memberCount - 1);
			bool rowExact = IsCatalogRowExact(us, "US", memberCount)
				&& IsCatalogRowExact(ussr, "USSR", memberCount);
			exact = rowExact && exact;
			if (!evidence.IsEmpty())
				evidence = evidence + ", ";
			evidence = evidence + string.Format("%1:%2", memberCount, rowExact);
		}
		HST_LocalSecurityCatalogResult resistance = catalog.ResolveAuthoredGroup("FIA", 3);
		bool resistanceRejected = resistance && !resistance.m_bSuccess && !resistance.m_Group;
		report.m_bCatalogExact = exact && resistanceRejected;
		report.m_sCatalogEvidence = evidence + string.Format(" | resistance rejected %1", resistanceRejected);
	}

	protected bool IsCatalogRowExact(
		HST_LocalSecurityCatalogResult result,
		string factionKey,
		int memberCount)
	{
		if (!result || !result.m_bSuccess || !result.m_Group
			|| result.m_Group.m_sFactionKey != factionKey
			|| result.m_Group.m_sRole != HST_LocalSecurityCatalogService.ROLE_TOWN_POLICE
			|| result.m_Group.m_aMemberSlots.Count() != memberCount)
			return false;
		for (int slotIndex = 0; slotIndex < result.m_Group.m_aMemberSlots.Count(); slotIndex++)
		{
			HST_ForceGroupCatalogSlot slot = result.m_Group.m_aMemberSlots[slotIndex];
			if (!slot || slot.m_iOrdinal != slotIndex || !slot.m_bRequired || slot.m_sPrefab.IsEmpty())
				return false;
		}
		return true;
	}

	protected void ProveAdmissionReplay(HST_LocalSecurityOperationProofReport report)
	{
		HST_LocalSecurityOperationProofFixture fixture = m_Fixtures.Build(
			"admission_replay", HST_EZoneType.HST_ZONE_TOWN, "US", 3);
		int attackBefore = fixture.m_Pool.m_iAttackResources;
		int supportBefore = fixture.m_Pool.m_iSupportResources;
		if (!m_Fixtures.Admit(fixture))
		{
			report.m_sAdmissionReplayEvidence = fixture.m_sFailureReason;
			return;
		}
		bool admissionExact = fixture.m_bInitialTickChanged && IsAdmissionGraphExact(fixture);
		admissionExact = admissionExact
			&& fixture.m_Pool.m_iAttackResources == attackBefore
			&& fixture.m_Pool.m_iSupportResources == supportBefore;
		int revisionsBefore = fixture.m_Patrol.m_iRevision + fixture.m_Operation.m_iRevision
			+ fixture.m_Batch.m_iLifecycleRevision + fixture.m_Group.m_iLifecycleRevision;
		int eventsBefore = fixture.m_State.m_aTownInfluenceEvents.Count();
		bool replayChanged = fixture.m_Service.Tick(fixture.m_State, fixture.m_Preset);
		int revisionsAfter = fixture.m_Patrol.m_iRevision + fixture.m_Operation.m_iRevision
			+ fixture.m_Batch.m_iLifecycleRevision + fixture.m_Group.m_iLifecycleRevision;
		bool replayExact = !replayChanged && revisionsAfter == revisionsBefore;
		replayExact = replayExact
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == eventsBefore
			&& fixture.m_State.m_aLocalSecurityPatrols.Count() == 1
			&& fixture.m_State.m_aOperations.Count() == 1;
		replayExact = replayExact
			&& fixture.m_State.m_aForceManifests.Count() == 1
			&& fixture.m_State.m_aForceSpawnResults.Count() == 1
			&& fixture.m_State.m_aActiveGroups.Count() == 1;
		report.m_bAdmissionReplayExact = admissionExact && replayExact;
		report.m_sAdmissionReplayEvidence = string.Format(
			"admission/replay %1/%2 | roster %3 | zero cost %4 | held %5",
			admissionExact,
			replayExact,
			fixture.m_Patrol.m_iOriginalInfantryCount,
			ManifestIsZeroCost(fixture.m_Manifest),
			fixture.m_Batch.m_bStrategicProjectionHeld);
	}

	protected bool IsAdmissionGraphExact(HST_LocalSecurityOperationProofFixture fixture)
	{
		if (!fixture || !fixture.m_Patrol || !fixture.m_Operation || !fixture.m_Manifest
			|| !fixture.m_Batch || !fixture.m_Group)
			return false;
		string patrolId = HST_LocalSecuritySaveValidationService.BuildPatrolId(
			fixture.m_Zone.m_sZoneId,
			fixture.m_Zone.m_sOwnerFactionKey,
			fixture.m_Zone.m_iOwnershipRevision,
			1);
		bool exact = fixture.m_Patrol.m_sPatrolId == patrolId
			&& fixture.m_Zone.m_sLocalSecurityPatrolId == patrolId;
		exact = exact && fixture.m_Patrol.m_sOperationId
			== HST_LocalSecuritySaveValidationService.BuildOperationId(patrolId);
		exact = exact && fixture.m_Patrol.m_sManifestId
			== HST_LocalSecuritySaveValidationService.BuildManifestId(
				fixture.m_Patrol.m_sOperationId);
		exact = exact && fixture.m_Operation.m_eType
			== HST_EOperationType.HST_OPERATION_TYPE_LOCAL_SECURITY_PATROL;
		exact = exact && fixture.m_Operation.m_sCurrentRouteId.IsEmpty()
			&& fixture.m_Operation.m_sRouteContractHash.IsEmpty()
			&& fixture.m_Operation.m_iRouteVersion == 0
			&& fixture.m_Operation.m_iRouteWaypointIndex == -1;
		exact = exact && fixture.m_Manifest.m_bFrozen
			&& ManifestIsZeroCost(fixture.m_Manifest)
			&& fixture.m_Batch.m_bStrategicProjectionHeld;
		exact = exact && fixture.m_Group.m_sSpawnFallbackMode
			== HST_LocalSecurityOperationService.EXACT_GROUP_MODE;
		return exact && fixture.m_Group.m_sLocalSecurityPatrolId == patrolId
			&& fixture.m_Patrol.m_iLivingInfantryCount
				== fixture.m_Manifest.m_iAcceptedMemberCount;
	}

	protected bool ManifestIsZeroCost(HST_ForceManifestState manifest)
	{
		return manifest && manifest.m_iMoneyCost == 0 && manifest.m_iHRCost == 0
			&& manifest.m_iEquipmentCost == 0 && manifest.m_iAttackResourceCost == 0
			&& manifest.m_iSupportResourceCost == 0;
	}

	protected void ProveEligibilityIsolation(HST_LocalSecurityOperationProofReport report)
	{
		HST_LocalSecurityOperationProofFixture minor = m_Fixtures.Build(
			"minor_reject", HST_EZoneType.HST_ZONE_RESOURCE, "US", 4);
		bool minorChanged = minor.m_Service.Tick(minor.m_State, minor.m_Preset);
		HST_LocalSecurityOperationProofFixture resistance = m_Fixtures.Build(
			"resistance_reject", HST_EZoneType.HST_ZONE_TOWN, "FIA", 4);
		bool resistanceChanged = resistance.m_Service.Tick(resistance.m_State, resistance.m_Preset);
		bool minorRejected = !minorChanged && minor.m_State.m_aLocalSecurityPatrols.IsEmpty()
			&& minor.m_State.m_aOperations.IsEmpty() && minor.m_State.m_aForceManifests.IsEmpty();
		bool resistanceRejected = !resistanceChanged
			&& resistance.m_State.m_aLocalSecurityPatrols.IsEmpty()
			&& resistance.m_State.m_aOperations.IsEmpty()
			&& resistance.m_State.m_aForceManifests.IsEmpty();
		report.m_bEligibilityIsolationExact = minorRejected && resistanceRejected;
		report.m_sEligibilityIsolationEvidence = string.Format(
			"minor/resistance rejected %1/%2", minorRejected, resistanceRejected);
	}

	protected void ProveSurvivorRestore(HST_LocalSecurityOperationProofReport report)
	{
		HST_LocalSecurityOperationProofFixture fixture = m_Fixtures.Build(
			"survivor_restore", HST_EZoneType.HST_ZONE_TOWN, "US", 4);
		if (!m_Fixtures.Admit(fixture))
		{
			report.m_sSurvivorRestoreEvidence = fixture.m_sFailureReason;
			return;
		}
		fixture.m_State.m_iElapsedSeconds = 120;
		bool casualtiesAccepted = ConfirmCasualties(
			fixture, fixture.m_Manifest.m_aMembers.Count() - 1, "proof survivor roster");
		fixture.m_Service.Tick(fixture.m_State, fixture.m_Preset);
		bool oneSurvivor = RosterEquals(fixture, 1);
		int eventsBefore = fixture.m_State.m_aTownInfluenceEvents.Count();
		fixture.m_State.m_iElapsedSeconds = 130;
		fixture.m_Service.ReconcileAfterRestore(fixture.m_State, fixture.m_Preset);
		bool restoreExact = RosterEquals(fixture, 1)
			&& fixture.m_Batch.m_bStrategicProjectionHeld
			&& fixture.m_Operation.m_eMaterializationState
				== HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == eventsBefore;
		fixture.m_State.m_iElapsedSeconds = 140;
		fixture.m_Service.Tick(fixture.m_State, fixture.m_Preset);
		bool noRefill = RosterEquals(fixture, 1);
		report.m_bSurvivorRestoreExact = casualtiesAccepted && oneSurvivor && restoreExact && noRefill;
		report.m_sSurvivorRestoreEvidence = string.Format(
			"casualties/one/restore/no-refill %1/%2/%3/%4",
			casualtiesAccepted,
			oneSurvivor,
			restoreExact,
			noRefill);
	}

	protected bool ConfirmCasualties(
		HST_LocalSecurityOperationProofFixture fixture,
		int casualtyCount,
		string reason)
	{
		if (!fixture || !fixture.m_Manifest || !fixture.m_Batch)
			return false;
		int accepted;
		for (int memberIndex = 0; memberIndex < casualtyCount; memberIndex++)
		{
			HST_ForceManifestMemberState member = fixture.m_Manifest.m_aMembers[memberIndex];
			HST_ForceSpawnQueueCallbackResult casualty = fixture.m_Queue.ConfirmStrategicMemberCasualty(
				fixture.m_State.m_aForceSpawnResults,
				fixture.m_Manifest,
				fixture.m_Batch.m_sResultId,
				fixture.m_Batch.m_sProjectionId,
				member.m_sSlotId,
				fixture.m_State.m_iElapsedSeconds,
				reason);
			if (casualty && casualty.m_bAccepted)
				accepted++;
		}
		return accepted == casualtyCount;
	}

	protected bool RosterEquals(HST_LocalSecurityOperationProofFixture fixture, int living)
	{
		return fixture && fixture.m_Patrol && fixture.m_Operation && fixture.m_Group
			&& fixture.m_Patrol.m_iLivingInfantryCount == living
			&& fixture.m_Operation.m_iLastVirtualFriendlyCount == living
			&& fixture.m_Group.m_iInfantryCount == living
			&& fixture.m_Group.m_iDurableLivingInfantryCount == living
			&& fixture.m_Queue.CountStrategicLivingMemberSlots(fixture.m_Batch) == living;
	}

	protected void ProveDestructionReplay(HST_LocalSecurityOperationProofReport report)
	{
		HST_LocalSecurityOperationProofFixture fixture = m_Fixtures.Build(
			"destruction_replay", HST_EZoneType.HST_ZONE_TOWN, "US", 2);
		if (!m_Fixtures.Admit(fixture))
		{
			report.m_sDestructionReplayEvidence = fixture.m_sFailureReason;
			return;
		}
		fixture.m_State.m_iElapsedSeconds = 150;
		int policeBefore = fixture.m_Civilian.m_iPolicePresence;
		bool casualtiesAccepted = ConfirmCasualties(
			fixture, fixture.m_Manifest.m_aMembers.Count(), "proof full destruction");
		fixture.m_Service.Tick(fixture.m_State, fixture.m_Preset);
		HST_TownInfluenceEventState loss = FindEvent(
			fixture.m_State,
			HST_LocalSecuritySaveValidationService.BuildLossEventId(fixture.m_Patrol.m_sPatrolId));
		bool destroyedExact = fixture.m_Patrol.m_sStatus == "terminal"
			&& fixture.m_Patrol.m_bLossEventApplied
			&& fixture.m_Patrol.m_iLivingInfantryCount == 0
			&& fixture.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED
			&& !fixture.m_State.FindForceSpawnResult(fixture.m_Patrol.m_sSpawnResultId)
			&& !fixture.m_State.FindActiveGroup(fixture.m_Patrol.m_sGroupId)
			&& LossEventIsExact(loss, fixture)
			&& fixture.m_Civilian.m_iPolicePresence == policeBefore - 1;
		int eventCount = fixture.m_State.m_aTownInfluenceEvents.Count();
		int policeAfter = fixture.m_Civilian.m_iPolicePresence;
		fixture.m_State.m_iElapsedSeconds = 160;
		bool replayChanged = fixture.m_Service.Tick(fixture.m_State, fixture.m_Preset);
		bool replayExact = !replayChanged
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == eventCount
			&& fixture.m_Civilian.m_iPolicePresence == policeAfter
			&& fixture.m_Patrol.m_sStatus == "terminal";
		report.m_bDestructionReplayExact = casualtiesAccepted && destroyedExact && replayExact;
		report.m_sDestructionReplayEvidence = string.Format(
			"casualties/destroyed/replay %1/%2/%3 | police %4 -> %5",
			casualtiesAccepted,
			destroyedExact,
			replayExact,
			policeBefore,
			policeAfter);
	}

	protected HST_TownInfluenceEventState FindEvent(HST_CampaignState state, string eventId)
	{
		HST_TownInfluenceEventState match;
		if (!state || eventId.IsEmpty())
			return null;
		foreach (HST_TownInfluenceEventState eventState : state.m_aTownInfluenceEvents)
		{
			if (!eventState || eventState.m_sEventId != eventId)
				continue;
			if (match)
				return null;
			match = eventState;
		}
		return match;
	}

	protected bool LossEventIsExact(
		HST_TownInfluenceEventState loss,
		HST_LocalSecurityOperationProofFixture fixture)
	{
		if (!loss || !fixture || !loss.m_bApplied)
			return false;
		if (loss.m_sZoneId != fixture.m_Zone.m_sZoneId
			|| loss.m_sKind != "local_security_patrol_destroyed"
			|| loss.m_sSourceId != fixture.m_Operation.m_sOperationId
			|| loss.m_iPoliceDelta != -1)
			return false;
		if (loss.m_iFIASupportDelta != 0 || loss.m_iOccupierSupportDelta != 0
			|| loss.m_iReputationDelta != 0 || loss.m_iHeatDelta != 0
			|| loss.m_iPopulationDelta != 0 || loss.m_iRoadblockDelta != 0)
			return false;
		return loss.m_sAggressionFactionKey.IsEmpty()
			&& loss.m_iAggressionDelta == 0
			&& loss.m_iExpiresAtSecond == 0
			&& !loss.m_bPopulationScaled;
	}

	protected void ProveNonLossSettlements(HST_LocalSecurityOperationProofReport report)
	{
		string ownerEvidence;
		string stopEvidence;
		string spawnEvidence;
		bool ownerExact = ProveOwnerSettlementNoLoss(ownerEvidence);
		bool stopExact = ProveStopSettlementNoLoss(stopEvidence);
		bool spawnExact = ProveSpawnFailureNoLoss(spawnEvidence);
		report.m_bNonLossSettlementExact = ownerExact && stopExact && spawnExact;
		report.m_sNonLossSettlementEvidence = "owner " + ownerEvidence
			+ " | stop " + stopEvidence + " | spawn " + spawnEvidence;
	}

	protected bool ProveOwnerSettlementNoLoss(out string evidence)
	{
		HST_LocalSecurityOperationProofFixture fixture = m_Fixtures.Build(
			"owner_no_loss", HST_EZoneType.HST_ZONE_TOWN, "US", 3);
		if (!m_Fixtures.Admit(fixture))
		{
			evidence = fixture.m_sFailureReason;
			return false;
		}
		bool stateChanged;
		string failure;
		bool accepted = fixture.m_Service.ReconcileZoneOwnershipChange(
			fixture.m_State,
			fixture.m_Zone.m_sZoneId,
			"USSR",
			stateChanged,
			failure);
		bool exact = accepted && stateChanged && fixture.m_Patrol.m_sStatus == "terminal"
			&& fixture.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_INVALIDATED
			&& !fixture.m_Patrol.m_bLossEventApplied
			&& fixture.m_State.m_aTownInfluenceEvents.IsEmpty();
		evidence = string.Format("accepted/changed/exact %1/%2/%3 %4", accepted, stateChanged, exact, failure);
		return exact;
	}

	protected bool ProveStopSettlementNoLoss(out string evidence)
	{
		HST_LocalSecurityOperationProofFixture fixture = m_Fixtures.Build(
			"stop_no_loss", HST_EZoneType.HST_ZONE_TOWN, "US", 3);
		if (!m_Fixtures.Admit(fixture))
		{
			evidence = fixture.m_sFailureReason;
			return false;
		}
		bool changed = fixture.m_Service.SettleOpenOperationsForCampaignStop(
			fixture.m_State, "proof campaign stop");
		bool exact = changed && fixture.m_Patrol.m_sStatus == "terminal"
			&& fixture.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_CANCELLED
			&& !fixture.m_Patrol.m_bLossEventApplied
			&& fixture.m_State.m_aTownInfluenceEvents.IsEmpty();
		evidence = string.Format("changed/exact %1/%2", changed, exact);
		return exact;
	}

	protected bool ProveSpawnFailureNoLoss(out string evidence)
	{
		HST_LocalSecurityOperationProofFixture fixture = m_Fixtures.Build(
			"spawn_no_loss", HST_EZoneType.HST_ZONE_TOWN, "US", 3);
		if (!m_Fixtures.Admit(fixture))
		{
			evidence = fixture.m_sFailureReason;
			return false;
		}
		fixture.m_Batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
		fixture.m_State.m_iElapsedSeconds = 125;
		bool changed = fixture.m_Service.Tick(fixture.m_State, fixture.m_Preset);
		bool exact = changed && fixture.m_Patrol.m_sStatus == "terminal"
			&& fixture.m_Operation.m_eTerminalResult
				== HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_SPAWN_FAILED
			&& !fixture.m_Patrol.m_bLossEventApplied
			&& fixture.m_State.m_aTownInfluenceEvents.IsEmpty();
		evidence = string.Format("changed/exact %1/%2", changed, exact);
		return exact;
	}

	protected void ProveOwnerEpochRearm(HST_LocalSecurityOperationProofReport report)
	{
		HST_LocalSecurityOperationProofFixture fixture = m_Fixtures.Build(
			"owner_epoch_rearm", HST_EZoneType.HST_ZONE_TOWN, "US", 3);
		if (!m_Fixtures.Admit(fixture))
		{
			report.m_sOwnerEpochRearmEvidence = fixture.m_sFailureReason;
			return;
		}
		string oldPatrolId = fixture.m_Patrol.m_sPatrolId;
		string oldOperationId = fixture.m_Patrol.m_sOperationId;
		string oldManifestId = fixture.m_Patrol.m_sManifestId;
		bool settled = fixture.m_Service.SettleOpenOperationsForCampaignStop(
			fixture.m_State, "proof owner epoch boundary");
		fixture.m_Zone.m_iOwnershipRevision++;
		fixture.m_Zone.m_sLastOwnershipTransitionRequestId = "proof_owner_epoch_transition";
		fixture.m_State.m_iElapsedSeconds = 180;
		bool rearmed = fixture.m_Service.Tick(fixture.m_State, fixture.m_Preset);
		HST_LocalSecurityPatrolState current = fixture.m_State.FindLocalSecurityPatrol(fixture.m_Zone.m_sZoneId);
		int currentEpoch;
		if (current)
			currentEpoch = current.m_iEpoch;
		bool exact = settled && rearmed && current && current.m_sStatus == "active"
			&& current.m_iEpoch == 2 && current.m_iOwnershipRevision == 2
			&& current.m_sPatrolId != oldPatrolId
			&& !fixture.m_State.FindOperation(oldOperationId)
			&& !fixture.m_State.FindForceManifest(oldManifestId)
			&& fixture.m_State.m_aLocalSecurityPatrols.Count() == 1
			&& fixture.m_State.m_aOperations.Count() == 1
			&& fixture.m_State.m_aForceManifests.Count() == 1
			&& fixture.m_State.m_aForceSpawnResults.Count() == 1
			&& fixture.m_State.m_aActiveGroups.Count() == 1
			&& fixture.m_State.m_aTownInfluenceEvents.IsEmpty();
		report.m_bOwnerEpochRearmExact = exact;
		report.m_sOwnerEpochRearmEvidence = string.Format(
			"settled/rearmed/exact %1/%2/%3 | epoch %4",
			settled,
			rearmed,
			exact,
			currentEpoch);
	}

	protected void ProveConflictQuarantine(HST_LocalSecurityOperationProofReport report)
	{
		HST_LocalSecurityOperationProofFixture fixture = m_Fixtures.Build(
			"conflict_quarantine", HST_EZoneType.HST_ZONE_TOWN, "US", 2);
		if (!m_Fixtures.Admit(fixture))
		{
			report.m_sConflictQuarantineEvidence = fixture.m_sFailureReason;
			return;
		}
		fixture.m_State.m_iElapsedSeconds = 145;
		HST_TownInfluenceCommand conflicting = new HST_TownInfluenceCommand();
		conflicting.m_sCommandId = HST_LocalSecuritySaveValidationService.BuildLossEventId(
			fixture.m_Patrol.m_sPatrolId);
		conflicting.m_sEventId = conflicting.m_sCommandId;
		conflicting.m_sTownId = fixture.m_Zone.m_sZoneId;
		conflicting.m_sEventKind = "foreign_local_security_collision";
		conflicting.m_sSourceId = "proof_foreign_authority";
		conflicting.m_sReason = "proof conflicting local-security receipt";
		conflicting.m_iPoliceDelta = 1;
		conflicting.m_bReconcileOwnership = false;
		HST_TownInfluenceResult inserted = fixture.m_TownInfluence.Execute(fixture.m_State, conflicting);
		bool casualtiesAccepted = ConfirmCasualties(
			fixture, fixture.m_Manifest.m_aMembers.Count(), "proof conflict casualties");
		fixture.m_Service.Tick(fixture.m_State, fixture.m_Preset);
		string persistenceFailure;
		bool persistenceReady = fixture.m_Service.PrepareQuarantinedAuthorityForPersistence(
			fixture.m_State,
			persistenceFailure);
		bool runtimeRetired = fixture.m_Adapter.CountHandlesForProjection(
			fixture.m_Patrol.m_sProjectionId) == 0;
		runtimeRetired = runtimeRetired
			&& !fixture.m_PhysicalWar.GetForceSpawnGroupRoot(fixture.m_Group)
			&& fixture.m_PhysicalWar.CountForceSpawnRuntimeMembers(fixture.m_Group) == 0
			&& !fixture.m_Group.m_bSpawnedEntity
			&& fixture.m_Group.m_sRuntimeEntityId.IsEmpty();
		bool exact = inserted && inserted.m_bAccepted && casualtiesAccepted;
		exact = exact && fixture.m_Patrol.m_iContractVersion
			== HST_LocalSecurityOperationService.QUARANTINED_CONTRACT_VERSION;
		exact = exact && fixture.m_Patrol.m_sStatus == "quarantined"
			&& !fixture.m_Patrol.m_bLossEventApplied
			&& fixture.m_Patrol.m_sAuthorityFailure.Contains("fingerprint");
		exact = exact && fixture.m_State.m_aTownInfluenceEvents.Count() == 1
			&& persistenceReady && runtimeRetired;
		report.m_bConflictQuarantineExact = exact;
		report.m_sConflictQuarantineEvidence = string.Format(
			"collision/casualties/quarantine %1/%2/%3 | persistence %4 | runtime retired %5 | failure %6",
			inserted && inserted.m_bAccepted,
			casualtiesAccepted,
			exact,
			persistenceReady,
			runtimeRetired,
			persistenceFailure);
	}
}
#endif
