#ifdef ENABLE_DIAG
class HST_TownInfluenceProofReport
{
	bool m_bGoldenScalingExact;
	bool m_bStrictThresholdsExact;
	bool m_bIdempotencyRevisionExact;
	bool m_bLegacyProjectionExact;
	bool m_bPopulationMovementExact;
	bool m_bTargetRejectionExact;
	bool m_bAuthorityFailureExact;
	bool m_bExternalCompletionExact;
	bool m_bPre64InvaderRemapExact;
	bool m_bLegacyMigrationExact;
	bool m_bCurrentRestoreValidationExact;
	bool m_bRevisionBoundaryExact;
	string m_sGoldenScalingEvidence;
	string m_sStrictThresholdEvidence;
	string m_sIdempotencyEvidence;
	string m_sLegacyProjectionEvidence;
	string m_sPopulationEvidence;
	string m_sTargetRejectionEvidence;
	string m_sAuthorityFailureEvidence;
	string m_sExternalCompletionEvidence;
	string m_sPre64InvaderRemapEvidence;
	string m_sLegacyMigrationEvidence;
	string m_sCurrentRestoreValidationEvidence;
	string m_sRevisionBoundaryEvidence;

	bool AllExact()
	{
		return m_bGoldenScalingExact
			&& m_bStrictThresholdsExact
			&& m_bIdempotencyRevisionExact
			&& m_bLegacyProjectionExact
			&& m_bPopulationMovementExact
			&& m_bTargetRejectionExact
			&& m_bAuthorityFailureExact
			&& m_bExternalCompletionExact
			&& m_bPre64InvaderRemapExact
			&& m_bLegacyMigrationExact
			&& m_bCurrentRestoreValidationExact
			&& m_bRevisionBoundaryExact;
	}

	string BuildReport()
	{
		string report = string.Format(
			"town influence proof | all exact %1 | scaling %2 | thresholds %3 | idempotency %4 | projection %5 | population %6 | rejection %7 | authority %8 | external completion %9",
			AllExact(),
			m_bGoldenScalingExact,
			m_bStrictThresholdsExact,
			m_bIdempotencyRevisionExact,
			m_bLegacyProjectionExact,
			m_bPopulationMovementExact,
			m_bTargetRejectionExact,
			m_bAuthorityFailureExact,
			m_bExternalCompletionExact);
		return report + string.Format(
			" | pre64 invader %1 | migration %2 | current restore %3 | revision boundary %4",
			m_bPre64InvaderRemapExact,
			m_bLegacyMigrationExact,
			m_bCurrentRestoreValidationExact,
			m_bRevisionBoundaryExact);
	}
}

class HST_TownInfluenceProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_TownInfluenceService m_Service;
	ref HST_ZoneState m_Town;
	ref HST_CivilianZoneState m_Legacy;
	ref HST_TownInfluenceRecord m_Record;
}

// Detached, state-only proof for Schema-64 town political authority. It does
// not create world entities, mutate a live campaign, or require persistence IO.
class HST_TownInfluenceProofService
{
	static const string TOWN_ID = "town_influence_proof";
	static const string RESISTANCE_FACTION = "FIA";
	static const string OCCUPIER_FACTION = "US";
	static const string INVADER_FACTION = "USSR";

	HST_TownInfluenceProofReport Run()
	{
		HST_TownInfluenceProofReport report = new HST_TownInfluenceProofReport();
		ProveGoldenScaling(report);
		ProveStrictThresholds(report);
		ProveIdempotencyAndProjection(report);
		ProvePopulationMovement(report);
		ProveTargetRejection(report);
		ProveAuthorityFailure(report);
		ProveExternalOwnershipCompletion(report);
		ProvePre64InvaderRemap(report);
		ProvePersistenceBoundary(report);
		ProveRevisionBoundaries(report);
		return report;
	}

	protected void ProveRevisionBoundaries(HST_TownInfluenceProofReport report)
	{
		HST_TownInfluenceProofFixture fixture = BuildFixture();
		fixture.m_Record.m_iRevision
			= HST_TownInfluenceService.MAX_MUTABLE_REVISION;
		HST_ZoneState laterTown = BuildPersistenceTown(
			"town_influence_revision_later_proof");
		laterTown.m_iOwnershipContractVersion = 1;
		fixture.m_State.m_aZones.Insert(laterTown);
		HST_CivilianZoneState laterCivilian = new HST_CivilianZoneState();
		laterCivilian.m_sZoneId = laterTown.m_sZoneId;
		laterCivilian.m_iFIASupport = 81;
		laterCivilian.m_iOccupierSupport = 19;
		laterCivilian.m_iPopulationRemaining = 100;
		fixture.m_State.m_aCivilianZones.Insert(laterCivilian);
		fixture.m_Service.EnsureRecords(fixture.m_State);
		HST_TownInfluenceRecord laterRecord = fixture.m_Service.FindValidRecord(
			fixture.m_State,
			laterTown.m_sZoneId);
		if (laterRecord)
		{
			laterRecord.m_iFIASupportBasisPoints = 8100;
			laterRecord.m_iOccupierSupportBasisPoints = 1900;
			laterRecord.m_sHysteresisBand
				= HST_TownInfluenceService.HYSTERESIS_RESISTANCE;
		}
		bool laterChanged = fixture.m_Service.ReconcileTownOwnershipPolicies(
			fixture.m_State,
			fixture.m_Preset,
			true);
		bool exhaustedRowSkipped = laterChanged && laterRecord
			&& laterRecord.m_sHysteresisBand
				== HST_TownInfluenceService.HYSTERESIS_RESISTANCE
			&& laterRecord.m_sPendingOwnerFactionKey == RESISTANCE_FACTION;

		HST_TownInfluenceProofFixture seedFixture = BuildFixture();
		seedFixture.m_Record.m_iRevision
			= HST_TownInfluenceService.MAX_MUTABLE_REVISION - 1;
		int revisionBefore = seedFixture.m_Record.m_iRevision;
		int supportBefore = seedFixture.m_Record.m_iFIASupportBasisPoints;
		bool seedAccepted = seedFixture.m_Service.ApplyDebugSeed(
			seedFixture.m_State,
			TOWN_ID,
			6500,
			3500,
			0,
			120,
			110,
			10,
			"town_influence_revision_headroom_proof");
		bool seedRejectedInert = !seedAccepted
			&& seedFixture.m_Record.m_iRevision == revisionBefore
			&& seedFixture.m_Record.m_iFIASupportBasisPoints == supportBefore;
		report.m_bRevisionBoundaryExact = exhaustedRowSkipped
			&& seedRejectedInert;
		string laterBand;
		string laterPendingOwner;
		if (laterRecord)
		{
			laterBand = laterRecord.m_sHysteresisBand;
			laterPendingOwner = laterRecord.m_sPendingOwnerFactionKey;
		}
		report.m_sRevisionBoundaryEvidence = string.Format(
			"exhausted row skipped %1 | later band/pending %2/%3 | near-ceiling seed inert %4 | revision %5",
			exhaustedRowSkipped,
			laterBand,
			laterPendingOwner,
			seedRejectedInert,
			seedFixture.m_Record.m_iRevision);
	}

	protected void ProveGoldenScaling(HST_TownInfluenceProofReport report)
	{
		int population100 = HST_TownInfluenceService.CalculateEffectiveSupportDeltaBasisPoints(1, 100, true);
		int population25 = HST_TownInfluenceService.CalculateEffectiveSupportDeltaBasisPoints(1, 25, true);
		int population400 = HST_TownInfluenceService.CalculateEffectiveSupportDeltaBasisPoints(1, 400, true);
		int unscaledNegative = HST_TownInfluenceService.CalculateEffectiveSupportDeltaBasisPoints(-50, 100, false);
		report.m_bGoldenScalingExact = population100 == 100
			&& population25 == 200
			&& population400 == 50
			&& unscaledNegative == -5000;
		report.m_sGoldenScalingEvidence = string.Format(
			"raw 1 populations 100/25/400 -> %1/%2/%3 bp | raw -50 unscaled -> %4 bp",
			population100,
			population25,
			population400,
			unscaledNegative);
	}

	protected void ProveStrictThresholds(HST_TownInfluenceProofReport report)
	{
		string aboveResistance = HST_TownInfluenceService.ResolveHysteresisBand(8001);
		string equalResistance = HST_TownInfluenceService.ResolveHysteresisBand(8000);
		string equalEnemy = HST_TownInfluenceService.ResolveHysteresisBand(4000);
		string belowEnemy = HST_TownInfluenceService.ResolveHysteresisBand(3999);
		report.m_bStrictThresholdsExact
			= aboveResistance == HST_TownInfluenceService.HYSTERESIS_RESISTANCE
			&& equalResistance == HST_TownInfluenceService.HYSTERESIS_NEUTRAL
			&& equalEnemy == HST_TownInfluenceService.HYSTERESIS_NEUTRAL
			&& belowEnemy == HST_TownInfluenceService.HYSTERESIS_ENEMY
			&& !HST_TownInfluenceService.QualifiesResistanceOwnership(8000)
			&& !HST_TownInfluenceService.QualifiesEnemyOwnership(4000);
		report.m_sStrictThresholdEvidence = string.Format(
			"8001/8000/4000/3999 -> %1/%2/%3/%4",
			aboveResistance,
			equalResistance,
			equalEnemy,
			belowEnemy);
	}

	protected void ProveIdempotencyAndProjection(HST_TownInfluenceProofReport report)
	{
		HST_TownInfluenceProofFixture fixture = BuildFixture();
		HST_TownInfluenceCommand command = BuildCommand(
			"town_influence_proof_exact",
			"mission_aid",
			1,
			0,
			0);
		command.m_bMarkContacted = true;
		command.m_bMarkResistanceActivity = true;
		HST_TownInfluenceResult first = fixture.m_Service.Execute(
			fixture.m_State,
			command);
		int revisionAfterFirst = fixture.m_Record.m_iRevision;
		HST_TownInfluenceResult replay = fixture.m_Service.Execute(
			fixture.m_State,
			command);

		HST_TownInfluenceCommand conflict = BuildCommand(
			"town_influence_proof_exact",
			"mission_aid",
			2,
			0,
			0);
		conflict.m_bMarkContacted = true;
		conflict.m_bMarkResistanceActivity = true;
		HST_TownInfluenceResult conflictingReplay = fixture.m_Service.Execute(
			fixture.m_State,
			conflict);
		HST_TownInfluenceCommand overflowReplay = BuildCommand(
			"town_influence_proof_exact",
			"mission_aid",
			1,
			0,
			0);
		overflowReplay.m_iHeatDelta = int.MAX;
		HST_TownInfluenceResult rejectedOverflowReplay = fixture.m_Service.Execute(
			fixture.m_State,
			overflowReplay);

		bool firstExact = first && first.m_bAccepted
			&& !first.m_bAlreadyApplied && first.m_bChanged;
		if (firstExact)
			firstExact = first.m_Event
			&& first.m_Event.m_iRequestedFIABasisPointDelta == 100
			&& first.m_Event.m_iEffectiveFIABasisPointDelta == 100;
		if (firstExact)
			firstExact = first.m_Event.m_iRecordRevisionBefore == 1
			&& first.m_Event.m_iRecordRevisionAfter == revisionAfterFirst
			&& revisionAfterFirst == 2;
		bool replayExact = replay && replay.m_bAccepted
			&& replay.m_bAlreadyApplied
			&& !replay.m_bChanged
			&& fixture.m_Record.m_iRevision == revisionAfterFirst
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 1;
		bool rejectionExact = conflictingReplay
			&& !conflictingReplay.m_bAccepted
			&& rejectedOverflowReplay
			&& !rejectedOverflowReplay.m_bAccepted;
		bool idempotencyExact = firstExact && replayExact && rejectionExact;
		report.m_bIdempotencyRevisionExact = idempotencyExact;
		int requestedEvidence;
		int effectiveEvidence;
		if (first && first.m_Event)
		{
			requestedEvidence = first.m_Event.m_iRequestedFIABasisPointDelta;
			effectiveEvidence = first.m_Event.m_iEffectiveFIABasisPointDelta;
		}
		report.m_sIdempotencyEvidence = string.Format(
			"first/replay/conflict/overflow %1/%2/%3/%4 | revision %5 | events %6 | requested/effective %7/%8",
			first && first.m_bAccepted,
			replay && replay.m_bAlreadyApplied,
			conflictingReplay && !conflictingReplay.m_bAccepted,
			rejectedOverflowReplay && !rejectedOverflowReplay.m_bAccepted,
			fixture.m_Record.m_iRevision,
			fixture.m_State.m_aTownInfluenceEvents.Count(),
			requestedEvidence,
			effectiveEvidence);

		bool projectionExact = fixture.m_Record.m_iFIASupportBasisPoints == 5100
			&& fixture.m_Record.m_iOccupierSupportBasisPoints == 5000
			&& fixture.m_Record.m_iInvaderSupportBasisPoints == 0
			&& fixture.m_Legacy.m_iFIASupport == 51
			&& fixture.m_Legacy.m_iOccupierSupport == 50
			&& fixture.m_Legacy.m_iInfluenceEventCount == 1
			&& fixture.m_Legacy.m_sLastInfluenceEventId
				== "town_influence_proof_exact"
			&& fixture.m_Town.m_iSupport == 1
			&& fixture.m_Record.m_bContacted
			&& fixture.m_Record.m_bResistanceActivityStarted;
		report.m_bLegacyProjectionExact = projectionExact;
		report.m_sLegacyProjectionEvidence = string.Format(
			"canonical FIA/enemy %1/%2 bp | legacy FIA/enemy %3/%4 pct | signed %5 | events %6 | contacted/activity %7/%8",
			fixture.m_Record.m_iFIASupportBasisPoints,
			Math.Max(
				fixture.m_Record.m_iOccupierSupportBasisPoints,
				fixture.m_Record.m_iInvaderSupportBasisPoints),
			fixture.m_Legacy.m_iFIASupport,
			fixture.m_Legacy.m_iOccupierSupport,
			fixture.m_Town.m_iSupport,
			fixture.m_Legacy.m_iInfluenceEventCount,
			fixture.m_Record.m_bContacted,
			fixture.m_Record.m_bResistanceActivityStarted);
	}

	protected void ProvePopulationMovement(HST_TownInfluenceProofReport report)
	{
		HST_TownInfluenceProofFixture fixture = BuildFixture();
		HST_TownInfluenceCommand loss = BuildCommand(
			"town_influence_proof_population_loss",
			"incident_civilian_loss",
			0,
			0,
			-6);
		loss.m_bMarkContacted = true;
		HST_TownInfluenceResult lossResult = fixture.m_Service.Execute(
			fixture.m_State,
			loss);
		bool lossExact = lossResult && lossResult.m_bAccepted
			&& fixture.m_Record.m_iInitialPopulation == 100
			&& fixture.m_Record.m_iRemainingPopulation == 94
			&& fixture.m_Record.m_iDestroyedPopulation == 6
			&& lossResult.m_Event
			&& lossResult.m_Event.m_iInitialPopulationBefore == 100
			&& lossResult.m_Event.m_iInitialPopulationAfter == 100
			&& lossResult.m_Event.m_iRemainingPopulationBefore == 100
			&& lossResult.m_Event.m_iRemainingPopulationAfter == 94
			&& lossResult.m_Event.m_iDestroyedPopulationBefore == 0
			&& lossResult.m_Event.m_iDestroyedPopulationAfter == 6;

		HST_TownInfluenceCommand growth = BuildCommand(
			"town_influence_proof_population_growth",
			"mission_population_growth",
			0,
			0,
			4);
		growth.m_bMarkContacted = true;
		growth.m_bMarkResistanceActivity = true;
		HST_TownInfluenceResult growthResult = fixture.m_Service.Execute(
			fixture.m_State,
			growth);
		bool growthExact = growthResult && growthResult.m_bAccepted
			&& fixture.m_Record.m_iInitialPopulation == 104
			&& fixture.m_Record.m_iRemainingPopulation == 98
			&& fixture.m_Record.m_iDestroyedPopulation == 6
			&& fixture.m_Legacy.m_iPopulationRemaining == 98
			&& fixture.m_Legacy.m_iPopulationKilled == 6
			&& growthResult.m_Event
			&& growthResult.m_Event.m_iInitialPopulationBefore == 100
			&& growthResult.m_Event.m_iInitialPopulationAfter == 104
			&& growthResult.m_Event.m_iRemainingPopulationBefore == 94
			&& growthResult.m_Event.m_iRemainingPopulationAfter == 98
			&& growthResult.m_Event.m_iDestroyedPopulationBefore == 6
			&& growthResult.m_Event.m_iDestroyedPopulationAfter == 6;

		bool seedAccepted = fixture.m_Service.ApplyDebugSeed(
			fixture.m_State,
			TOWN_ID,
			6500,
			3500,
			0,
			120,
			110,
			10,
			"town_influence_population_seed_proof");
		int seedRevision = fixture.m_Record.m_iRevision;
		HST_TownInfluenceEventState seedEvent;
		if (fixture.m_State.m_aTownInfluenceEvents.Count() == 3)
			seedEvent = fixture.m_State.m_aTownInfluenceEvents[2];
		bool seedShapeExact = seedAccepted && seedEvent
			&& seedEvent.m_bAbsoluteDebugSeed
			&& seedEvent.m_iPopulationDelta == 16;
		bool seedSupportExact = seedEvent
			&& seedEvent.m_iFIASupportDelta == 1500
			&& seedEvent.m_iOccupierSupportDelta == -1500
			&& seedEvent.m_iRequestedInvaderBasisPointDelta == 0
			&& seedEvent.m_iEffectiveFIABasisPointDelta == 1500
			&& seedEvent.m_iEffectiveOccupierBasisPointDelta == -1500
			&& seedEvent.m_iFIABasisPointsAfter == 6500
			&& seedEvent.m_iOccupierBasisPointsAfter == 3500
			&& seedEvent.m_iInvaderBasisPointsAfter == 0;
		bool seedPopulationExact = seedEvent
			&& seedEvent.m_iInitialPopulationBefore == 104
			&& seedEvent.m_iInitialPopulationAfter == 120
			&& seedEvent.m_iRemainingPopulationBefore == 98
			&& seedEvent.m_iRemainingPopulationAfter == 110
			&& seedEvent.m_iDestroyedPopulationBefore == 6
			&& seedEvent.m_iDestroyedPopulationAfter == 10;
		bool seedEventExact = seedShapeExact
			&& seedSupportExact
			&& seedPopulationExact;
		bool seedReplay = fixture.m_Service.ApplyDebugSeed(
			fixture.m_State,
			TOWN_ID,
			6500,
			3500,
			0,
			120,
			110,
			10,
			"town_influence_population_seed_proof");
		bool seedReplayExact = seedReplay
			&& fixture.m_Record.m_iRevision == seedRevision
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 3;
		bool conflictingSeed = fixture.m_Service.ApplyDebugSeed(
			fixture.m_State,
			TOWN_ID,
			6400,
			3600,
			0,
			120,
			110,
			10,
			"town_influence_population_seed_proof");
		bool seedConflictRejected = !conflictingSeed
			&& fixture.m_Record.m_iRevision == seedRevision
			&& fixture.m_Record.m_iFIASupportBasisPoints == 6500
			&& fixture.m_Record.m_iOccupierSupportBasisPoints == 3500
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 3;

		HST_TownInfluenceProofFixture invaderFixture = BuildFixture();
		invaderFixture.m_Town.m_sOwnerFactionKey = INVADER_FACTION;
		invaderFixture.m_Record.m_iFIASupportBasisPoints = 4321;
		invaderFixture.m_Record.m_iOccupierSupportBasisPoints = 2222;
		invaderFixture.m_Record.m_iInvaderSupportBasisPoints = 3456;
		bool invaderSeedAccepted = invaderFixture.m_Service.ApplyDebugSeed(
			invaderFixture.m_State,
			TOWN_ID,
			6123,
			987,
			2890,
			103,
			100,
			3,
			"town_influence_invader_absolute_seed_proof");
		HST_TownInfluenceEventState invaderSeedEvent;
		if (invaderFixture.m_State.m_aTownInfluenceEvents.Count() == 1)
			invaderSeedEvent = invaderFixture.m_State.m_aTownInfluenceEvents[0];
		bool invaderSeedSupportExact = invaderSeedAccepted && invaderSeedEvent
			&& invaderFixture.m_Record.m_iFIASupportBasisPoints == 6123
			&& invaderFixture.m_Record.m_iOccupierSupportBasisPoints == 987
			&& invaderFixture.m_Record.m_iInvaderSupportBasisPoints == 2890;
		bool invaderSeedDeltaExact = invaderSeedEvent
			&& invaderSeedEvent.m_iFIASupportDelta == 1802
			&& invaderSeedEvent.m_iOccupierSupportDelta == -1235
			&& invaderSeedEvent.m_iRequestedInvaderBasisPointDelta == -566
			&& invaderSeedEvent.m_iEffectiveOccupierBasisPointDelta == -1235
			&& invaderSeedEvent.m_iEffectiveInvaderBasisPointDelta == -566;
		bool invaderSeedExact = invaderSeedSupportExact
			&& invaderSeedDeltaExact;
		report.m_bPopulationMovementExact = lossExact && growthExact
			&& seedEventExact && seedReplayExact
			&& seedConflictRejected && invaderSeedExact;
		report.m_sPopulationEvidence = string.Format(
			"loss/growth/seed/replay/conflict/invader %1/%2/%3/%4/%5/%6 | initial/remaining/destroyed %7/%8/%9",
			lossExact,
			growthExact,
			seedEventExact,
			seedReplayExact,
			seedConflictRejected,
			invaderSeedExact,
			fixture.m_Record.m_iInitialPopulation,
			fixture.m_Record.m_iRemainingPopulation,
			fixture.m_Record.m_iDestroyedPopulation);
	}

	protected void ProveTargetRejection(HST_TownInfluenceProofReport report)
	{
		HST_TownInfluenceProofFixture fixture = BuildFixture();
		HST_TownInfluenceCommand orphan = BuildCommand(
			"town_influence_proof_orphan",
			"mission_aid",
			1,
			0,
			0);
		orphan.m_sTownId = "missing_town";
		HST_TownInfluenceResult orphanResult = fixture.m_Service.Execute(
			fixture.m_State,
			orphan);

		HST_ZoneState resource = new HST_ZoneState();
		resource.m_sZoneId = "town_influence_proof_resource";
		resource.m_sDisplayName = "Proof Resource";
		resource.m_sOwnerFactionKey = OCCUPIER_FACTION;
		resource.m_eType = HST_EZoneType.HST_ZONE_RESOURCE;
		resource.m_iOwnershipContractVersion = 1;
		resource.m_iOwnershipRevision = 1;
		fixture.m_State.m_aZones.Insert(resource);
		HST_TownInfluenceCommand nonTown = BuildCommand(
			"town_influence_proof_non_town",
			"mission_aid",
			1,
			0,
			0);
		nonTown.m_sTownId = resource.m_sZoneId;
		HST_TownInfluenceResult nonTownResult = fixture.m_Service.Execute(
			fixture.m_State,
			nonTown);
		bool noInventedRecord = fixture.m_State.FindTownInfluenceRecord(
			resource.m_sZoneId) == null;
		report.m_bTargetRejectionExact = orphanResult
			&& !orphanResult.m_bAccepted
			&& nonTownResult && !nonTownResult.m_bAccepted
			&& noInventedRecord
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 0;
		report.m_sTargetRejectionEvidence = string.Format(
			"orphan/non-town rejected %1/%2 | invented record %3 | events %4",
			orphanResult && !orphanResult.m_bAccepted,
			nonTownResult && !nonTownResult.m_bAccepted,
			!noInventedRecord,
			fixture.m_State.m_aTownInfluenceEvents.Count());
	}

	protected void ProveAuthorityFailure(HST_TownInfluenceProofReport report)
	{
		HST_TownInfluenceProofFixture fixture = BuildFixture();
		HST_TownInfluenceCommand threshold = BuildCommand(
			"town_influence_proof_authority",
			"mission_aid",
			31,
			0,
			0);
		threshold.m_bMarkContacted = true;
		threshold.m_bMarkResistanceActivity = true;
		threshold.m_bReconcileOwnership = true;
		HST_TownInfluenceResult result = fixture.m_Service.Execute(
			fixture.m_State,
			threshold);
		report.m_bAuthorityFailureExact = result && result.m_bAccepted
			&& result.m_bOwnershipPending
			&& !result.m_sFailureReason.IsEmpty()
			&& fixture.m_Record.m_iFIASupportBasisPoints == 8100
			&& fixture.m_Record.m_sHysteresisBand
				== HST_TownInfluenceService.HYSTERESIS_RESISTANCE
			&& fixture.m_Record.m_sPendingOwnerFactionKey == RESISTANCE_FACTION
			&& fixture.m_Town.m_sOwnerFactionKey == OCCUPIER_FACTION
			&& fixture.m_State.m_aOwnershipTransitions.Count() == 0
			&& fixture.m_Record.m_sAuthorityFailure.IsEmpty();
		string failureEvidence = "missing result";
		if (result)
			failureEvidence = result.m_sFailureReason;
		report.m_sAuthorityFailureEvidence = string.Format(
			"event accepted/pending %1/%2 | support %3 | owner %4 | candidate %5 | receipts %6 | transient failure %7",
			result && result.m_bAccepted,
			result && result.m_bOwnershipPending,
			fixture.m_Record.m_iFIASupportBasisPoints,
			fixture.m_Town.m_sOwnerFactionKey,
			fixture.m_Record.m_sPendingOwnerFactionKey,
			fixture.m_State.m_aOwnershipTransitions.Count(),
			failureEvidence);
	}

	protected void ProveExternalOwnershipCompletion(
		HST_TownInfluenceProofReport report)
	{
		HST_TownInfluenceProofFixture fixture = BuildFixture();
		fixture.m_State.m_iElapsedSeconds = 110;
		fixture.m_Record.m_iFIASupportBasisPoints = 8100;
		fixture.m_Record.m_sHysteresisBand
			= HST_TownInfluenceService.HYSTERESIS_RESISTANCE;
		fixture.m_Record.m_sPendingOwnerFactionKey = RESISTANCE_FACTION;
		fixture.m_Record.m_iPendingOwnerSinceSecond = 100;
		fixture.m_Record.m_sPendingOwnershipRequestId
			= "town_influence_proof_completed_receipt";
		fixture.m_Record.m_iRevision = 2;
		fixture.m_Town.m_sOwnerFactionKey = RESISTANCE_FACTION;
		fixture.m_Town.m_iOwnershipRevision = 2;
		fixture.m_Town.m_sLastOwnershipTransitionRequestId
			= fixture.m_Record.m_sPendingOwnershipRequestId;

		HST_OwnershipTransitionState completed = new HST_OwnershipTransitionState();
		completed.m_sRequestId = fixture.m_Record.m_sPendingOwnershipRequestId;
		completed.m_sZoneId = TOWN_ID;
		completed.m_sNewOwnerFactionKey = RESISTANCE_FACTION;
		completed.m_sCause = "political_support";
		completed.m_sSourceType = "town_influence";
		completed.m_iAppliedRevision = 2;
		completed.m_iCompletedAtSecond = 105;
		completed.m_sStatus = "completed";
		completed.m_bValidated = true;
		completed.m_bOwnerApplied = true;
		completed.m_bCompleted = true;
		fixture.m_State.m_aOwnershipTransitions.Insert(completed);

		bool changed = fixture.m_Service.ReconcileTownOwnershipPolicies(
			fixture.m_State,
			fixture.m_Preset,
			true);
		report.m_bExternalCompletionExact = changed
			&& fixture.m_Record.m_sPendingOwnerFactionKey.IsEmpty()
			&& fixture.m_Record.m_sPendingOwnershipRequestId.IsEmpty()
			&& fixture.m_Record.m_iPendingOwnerSinceSecond == 0
			&& fixture.m_Record.m_sLastFlipOwnerFactionKey == RESISTANCE_FACTION
			&& fixture.m_Record.m_iLastFlipSecond == 105
			&& fixture.m_Record.m_iLastFlipOwnershipRevision == 2
			&& fixture.m_Record.m_sLastMutationId == completed.m_sRequestId;
		report.m_sExternalCompletionEvidence = string.Format(
			"changed %1 | pending owner/request %2/%3 | last flip %4 at %5 revision %6",
			changed,
			fixture.m_Record.m_sPendingOwnerFactionKey,
			fixture.m_Record.m_sPendingOwnershipRequestId,
			fixture.m_Record.m_sLastFlipOwnerFactionKey,
			fixture.m_Record.m_iLastFlipSecond,
			fixture.m_Record.m_iLastFlipOwnershipRevision);
	}

	protected void ProvePre64InvaderRemap(HST_TownInfluenceProofReport report)
	{
		HST_TownInfluenceProofFixture fixture = BuildFixture();
		fixture.m_State.m_iLastLoadedSchemaVersion = 63;
		fixture.m_State.m_iPersistenceRestoreSequence = 1;
		fixture.m_Town.m_sOwnerFactionKey = INVADER_FACTION;
		fixture.m_Record.m_iFIASupportBasisPoints = 3500;
		fixture.m_Record.m_iOccupierSupportBasisPoints = 6000;
		fixture.m_Record.m_iInvaderSupportBasisPoints = 0;
		fixture.m_Record.m_iOccupierRadioBasisPoints = 200;
		fixture.m_Record.m_iInvaderRadioBasisPoints = 0;
		fixture.m_Record.m_iOccupierPropagandaBasisPoints = 100;
		fixture.m_Record.m_iInvaderPropagandaBasisPoints = 0;
		fixture.m_Record.m_sHysteresisBand
			= HST_TownInfluenceService.HYSTERESIS_NEUTRAL;
		int revisionBefore = fixture.m_Record.m_iRevision;

		bool changed = fixture.m_Service.ReconcileTownOwnershipPolicies(
			fixture.m_State,
			fixture.m_Preset,
			true);
		int revisionAfterFirst = fixture.m_Record.m_iRevision;
		bool repeated = fixture.m_Service.ReconcileTownOwnershipPolicies(
			fixture.m_State,
			fixture.m_Preset,
			true);
		report.m_bPre64InvaderRemapExact = changed
			&& !repeated
			&& fixture.m_Record.m_iOccupierSupportBasisPoints == 0
			&& fixture.m_Record.m_iInvaderSupportBasisPoints == 6000
			&& fixture.m_Record.m_iOccupierRadioBasisPoints == 0
			&& fixture.m_Record.m_iInvaderRadioBasisPoints == 200
			&& fixture.m_Record.m_iOccupierPropagandaBasisPoints == 0
			&& fixture.m_Record.m_iInvaderPropagandaBasisPoints == 100
			&& fixture.m_Record.m_sHysteresisBand
				== HST_TownInfluenceService.HYSTERESIS_ENEMY
			&& fixture.m_Record.m_sPendingOwnerFactionKey.IsEmpty()
			&& fixture.m_Town.m_sOwnerFactionKey == INVADER_FACTION
			&& revisionAfterFirst == revisionBefore + 1
			&& fixture.m_Record.m_iRevision == revisionAfterFirst;
		report.m_sPre64InvaderRemapEvidence = string.Format(
			"changed/repeated %1/%2 | occupier/invader %3/%4 bp | radio %5/%6 | propaganda %7/%8 | band %9",
			changed,
			repeated,
			fixture.m_Record.m_iOccupierSupportBasisPoints,
			fixture.m_Record.m_iInvaderSupportBasisPoints,
			fixture.m_Record.m_iOccupierRadioBasisPoints,
			fixture.m_Record.m_iInvaderRadioBasisPoints,
			fixture.m_Record.m_iOccupierPropagandaBasisPoints,
			fixture.m_Record.m_iInvaderPropagandaBasisPoints,
			fixture.m_Record.m_sHysteresisBand);
		report.m_sPre64InvaderRemapEvidence
			= report.m_sPre64InvaderRemapEvidence + string.Format(
			" | revisions %1->%2",
			revisionBefore,
			fixture.m_Record.m_iRevision);
	}

	protected void ProvePersistenceBoundary(HST_TownInfluenceProofReport report)
	{
		HST_CampaignSaveData legacy = new HST_CampaignSaveData();
		legacy.m_iElapsedSeconds = 100;
		HST_ZoneState legacyTown = BuildPersistenceTown("town_influence_migration_proof");
		legacyTown.m_iSupport = 20;
		legacyTown.m_bActive = true;
		legacy.m_aZones.Insert(legacyTown);
		HST_CivilianZoneState legacyCivilian = new HST_CivilianZoneState();
		legacyCivilian.m_sZoneId = legacyTown.m_sZoneId;
		legacyCivilian.m_iFIASupport = 70;
		legacyCivilian.m_iOccupierSupport = 30;
		legacyCivilian.m_iPopulationRemaining = 75;
		legacyCivilian.m_iPopulationKilled = 25;
		legacy.m_aCivilianZones.Insert(legacyCivilian);
		HST_TownInfluenceSaveValidationService migrationValidator
			= new HST_TownInfluenceSaveValidationService();
		migrationValidator.Normalize(legacy, 63);
		HST_TownInfluenceRecord migrated;
		if (legacy.m_aTownInfluenceRecords.Count() == 1)
			migrated = legacy.m_aTownInfluenceRecords[0];
		report.m_bLegacyMigrationExact = migrated
			&& migrated.m_iContractVersion == HST_TownInfluenceService.EXACT_CONTRACT_VERSION
			&& migrated.m_iFIASupportBasisPoints == 6000
			&& migrated.m_iOccupierSupportBasisPoints == 4000
			&& migrated.m_iInitialPopulation == 100
			&& migrated.m_iRemainingPopulation == 75
			&& migrated.m_iDestroyedPopulation == 25
			&& migrated.m_bContacted
			&& legacyTown.m_iSupport == 20
			&& legacy.m_aCampaignEvents.Count() == 1;
		int migratedFIA;
		int migratedOccupier;
		int migratedInitial;
		int migratedRemaining;
		int migratedDestroyed;
		bool migratedContacted;
		if (migrated)
		{
			migratedFIA = migrated.m_iFIASupportBasisPoints;
			migratedOccupier = migrated.m_iOccupierSupportBasisPoints;
			migratedInitial = migrated.m_iInitialPopulation;
			migratedRemaining = migrated.m_iRemainingPopulation;
			migratedDestroyed = migrated.m_iDestroyedPopulation;
			migratedContacted = migrated.m_bContacted;
		}
		report.m_sLegacyMigrationEvidence = string.Format(
			"record %1 | FIA/occupier %2/%3 bp | population %4/%5/%6 | contacted %7 | conflicts %8",
			migrated != null,
			migratedFIA,
			migratedOccupier,
			migratedInitial,
			migratedRemaining,
			migratedDestroyed,
			migratedContacted,
			legacy.m_aCampaignEvents.Count());

		HST_TownInfluenceRecord reorderedRecord;
		HST_CampaignSaveData reordered = BuildCurrentRestoreFixture(
			"town_influence_reordered_restore_proof",
			reorderedRecord);
		HST_TownInfluenceSaveValidationService currentValidator
			= new HST_TownInfluenceSaveValidationService();
		currentValidator.Normalize(reordered, 64);
		bool reorderedAccepted = reorderedRecord.m_iContractVersion
			== HST_TownInfluenceService.EXACT_CONTRACT_VERSION
			&& reorderedRecord.m_iRevision == 7;
		reorderedRecord.m_iRemainingPopulation = 99;
		reorderedRecord.m_iDestroyedPopulation = 1;
		HST_TownInfluenceSaveValidationService populationValidator
			= new HST_TownInfluenceSaveValidationService();
		populationValidator.Normalize(reordered, 64);
		bool populationTamperQuarantined
			= reorderedRecord.m_iContractVersion == -64
			&& !reorderedRecord.m_sAuthorityFailure.IsEmpty();

		HST_TownInfluenceRecord aggregateRecord;
		HST_CampaignSaveData aggregate = BuildCurrentRestoreFixture(
			"town_influence_aggregate_tamper_proof",
			aggregateRecord);
		aggregateRecord.m_iFIARadioBasisPoints = 201;
		HST_TownInfluenceSaveValidationService aggregateValidator
			= new HST_TownInfluenceSaveValidationService();
		aggregateValidator.Normalize(aggregate, 64);
		bool aggregateTamperQuarantined
			= aggregateRecord.m_iContractVersion == -64
			&& !aggregateRecord.m_sAuthorityFailure.IsEmpty();

		HST_CampaignSaveData orphan = new HST_CampaignSaveData();
		orphan.m_iElapsedSeconds = 100;
		HST_ZoneState orphanTown = BuildPersistenceTown(
			"town_influence_orphan_restore_proof");
		orphan.m_aZones.Insert(orphanTown);
		HST_TownInfluenceRecord orphanRecord = BuildPersistenceRecord(
			orphanTown.m_sZoneId,
			5000,
			2);
		orphanRecord.m_sPendingOwnerFactionKey = RESISTANCE_FACTION;
		orphanRecord.m_iPendingOwnerSinceSecond = 50;
		orphanRecord.m_sPendingOwnershipRequestId = "orphan_political_receipt";
		orphan.m_aTownInfluenceRecords.Insert(orphanRecord);
		HST_TownInfluenceSaveValidationService orphanValidator
			= new HST_TownInfluenceSaveValidationService();
		orphanValidator.Normalize(orphan, 64);
		bool orphanQuarantined = orphanRecord.m_iContractVersion == -64
			&& !orphanRecord.m_sAuthorityFailure.IsEmpty();
		report.m_bCurrentRestoreValidationExact = reorderedAccepted
			&& populationTamperQuarantined
			&& aggregateTamperQuarantined
			&& orphanQuarantined;
		report.m_sCurrentRestoreValidationEvidence = string.Format(
			"reordered exact chain accepted %1 | population tamper quarantined %2 | aggregate tamper quarantined %3 | orphan receipt quarantined %4",
			reorderedAccepted,
			populationTamperQuarantined,
			aggregateTamperQuarantined,
			orphanQuarantined);
	}

	protected HST_CampaignSaveData BuildCurrentRestoreFixture(
		string townId,
		out HST_TownInfluenceRecord record)
	{
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.m_iElapsedSeconds = 100;
		HST_ZoneState town = BuildPersistenceTown(townId);
		saveData.m_aZones.Insert(town);
		record = BuildPersistenceRecord(townId, 5200, 7);
		record.m_iFIARadioBasisPoints = 200;
		record.m_iInfluenceEventCount = 2;
		string firstEventId = townId + "_event_1";
		string secondEventId = townId + "_event_2";
		record.m_sLastInfluenceEventId = secondEventId;
		record.m_sLastInfluenceEventKind = "radio_broadcast";
		record.m_sLastInfluenceEventReason
			= "restore chain event " + secondEventId;
		record.m_iLastInfluenceEventSecond = 20;
		record.m_sLastMutationId = secondEventId;
		saveData.m_aTownInfluenceRecords.Insert(record);
		// Deliberately reverse durable array order, leave two unrelated revisions
		// between exact events, and leave two more after the exact chain. Event
		// revision authority, not JSON order or adjacency, determines continuity.
		saveData.m_aTownInfluenceEvents.Insert(BuildPersistenceEvent(
			townId,
			secondEventId,
			20,
			4,
			5100));
		saveData.m_aTownInfluenceEvents.Insert(BuildPersistenceEvent(
			townId,
			firstEventId,
			10,
			1,
			5000));
		return saveData;
	}

	protected HST_ZoneState BuildPersistenceTown(string townId)
	{
		HST_ZoneState town = new HST_ZoneState();
		town.m_sZoneId = townId;
		town.m_sDisplayName = townId;
		town.m_sOwnerFactionKey = OCCUPIER_FACTION;
		town.m_eType = HST_EZoneType.HST_ZONE_TOWN;
		town.m_iOwnershipContractVersion = 1;
		town.m_iOwnershipRevision = 1;
		return town;
	}

	protected HST_TownInfluenceRecord BuildPersistenceRecord(
		string townId,
		int fiaSupportBasisPoints,
		int revision)
	{
		HST_TownInfluenceRecord record = new HST_TownInfluenceRecord();
		record.m_sTownId = townId;
		record.m_iRevision = revision;
		record.m_iFIASupportBasisPoints = fiaSupportBasisPoints;
		record.m_iOccupierSupportBasisPoints = 5000;
		record.m_iInitialPopulation = 100;
		record.m_iRemainingPopulation = 100;
		record.m_sHysteresisBand = HST_TownInfluenceService.ResolveHysteresisBand(
			fiaSupportBasisPoints);
		return record;
	}

	protected HST_TownInfluenceEventState BuildPersistenceEvent(
		string townId,
		string eventId,
		int createdAtSecond,
		int revisionBefore,
		int fiaBefore)
	{
		HST_TownInfluenceEventState eventState
			= new HST_TownInfluenceEventState();
		eventState.m_sEventId = eventId;
		eventState.m_sZoneId = townId;
		eventState.m_sKind = "radio_broadcast";
		eventState.m_sSourceId = "restore_chain_radio";
		eventState.m_sReason = "restore chain event " + eventId;
		eventState.m_iCreatedAtSecond = createdAtSecond;
		eventState.m_iFIASupportDelta = 1;
		eventState.m_iRequestedFIABasisPointDelta = 100;
		eventState.m_iEffectiveFIABasisPointDelta = 100;
		eventState.m_iPopulationUsed = 100;
		eventState.m_iRecordRevisionBefore = revisionBefore;
		eventState.m_iRecordRevisionAfter = revisionBefore + 1;
		eventState.m_iFIABasisPointsBefore = fiaBefore;
		eventState.m_iFIABasisPointsAfter = fiaBefore + 100;
		eventState.m_iOccupierBasisPointsBefore = 5000;
		eventState.m_iOccupierBasisPointsAfter = 5000;
		eventState.m_iInitialPopulationBefore = 100;
		eventState.m_iInitialPopulationAfter = 100;
		eventState.m_iRemainingPopulationBefore = 100;
		eventState.m_iRemainingPopulationAfter = 100;
		eventState.m_iDestroyedPopulationBefore = 0;
		eventState.m_iDestroyedPopulationAfter = 0;
		eventState.m_bApplied = true;
		return eventState;
	}

	protected HST_TownInfluenceProofFixture BuildFixture()
	{
		HST_TownInfluenceProofFixture fixture = new HST_TownInfluenceProofFixture();
		fixture.m_State = new HST_CampaignState();
		fixture.m_State.m_iElapsedSeconds = 100;
		fixture.m_State.m_iPersistenceRestoreSequence = 0;
		fixture.m_Preset = new HST_CampaignPreset();
		fixture.m_Preset.m_sPresetId = "town_influence_proof";
		fixture.m_Preset.m_sResistanceFactionKey = RESISTANCE_FACTION;
		fixture.m_Preset.m_sOccupierFactionKey = OCCUPIER_FACTION;
		fixture.m_Preset.m_sInvaderFactionKey = INVADER_FACTION;
		fixture.m_State.m_aFactionPools.Insert(BuildFactionPool(RESISTANCE_FACTION));
		fixture.m_State.m_aFactionPools.Insert(BuildFactionPool(OCCUPIER_FACTION));
		fixture.m_State.m_aFactionPools.Insert(BuildFactionPool(INVADER_FACTION));

		fixture.m_Town = new HST_ZoneState();
		fixture.m_Town.m_sZoneId = TOWN_ID;
		fixture.m_Town.m_sDisplayName = "Town Influence Proof";
		fixture.m_Town.m_sOwnerFactionKey = OCCUPIER_FACTION;
		fixture.m_Town.m_eType = HST_EZoneType.HST_ZONE_TOWN;
		fixture.m_Town.m_iOwnershipContractVersion = 1;
		fixture.m_Town.m_iOwnershipRevision = 1;
		fixture.m_State.m_aZones.Insert(fixture.m_Town);

		fixture.m_Legacy = new HST_CivilianZoneState();
		fixture.m_Legacy.m_sZoneId = TOWN_ID;
		fixture.m_Legacy.m_iReputation = 50;
		fixture.m_Legacy.m_iFIASupport = 50;
		fixture.m_Legacy.m_iOccupierSupport = 50;
		fixture.m_Legacy.m_iCivilianPresence = 10;
		fixture.m_Legacy.m_iPopulationRemaining = 100;
		fixture.m_State.m_aCivilianZones.Insert(fixture.m_Legacy);

		fixture.m_Service = new HST_TownInfluenceService();
		fixture.m_Service.SetCampaignPreset(fixture.m_Preset);
		fixture.m_Service.EnsureRecords(fixture.m_State);
		fixture.m_Record = fixture.m_Service.FindValidRecord(
			fixture.m_State,
			TOWN_ID);
		return fixture;
	}

	protected HST_TownInfluenceCommand BuildCommand(
		string eventId,
		string eventKind,
		int fiaDelta,
		int occupierDelta,
		int populationDelta)
	{
		HST_TownInfluenceCommand command = new HST_TownInfluenceCommand();
		command.m_sCommandId = eventId;
		command.m_sEventId = eventId;
		command.m_sTownId = TOWN_ID;
		command.m_sEventKind = eventKind;
		command.m_sSourceId = "town_influence_proof_source";
		command.m_sReason = "detached town influence proof";
		command.m_iRawFIASupportDelta = fiaDelta;
		command.m_iRawOccupierSupportDelta = occupierDelta;
		command.m_iPopulationDelta = populationDelta;
		command.m_bReconcileOwnership = false;
		return command;
	}

	protected HST_FactionPoolState BuildFactionPool(string factionKey)
	{
		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = factionKey;
		if (factionKey == "USSR" || factionKey == "US")
		{
			pool.m_iStrategicContractVersion = HST_EnemyStrategicResourceService.CONTRACT_VERSION;
			pool.m_iStrategicRevision = 1;
		}
		return pool;
	}
}
#endif
