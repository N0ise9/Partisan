#ifdef ENABLE_DIAG
class HST_CivilianConsequenceProofReport
{
	bool m_bResistanceCasualtyExact;
	bool m_bReplayAndConflictExact;
	bool m_bEnemyAndUnknownExact;
	bool m_bVehicleTheftExact;
	bool m_bCombatEpisodeExact;
	bool m_bMinorLocalityExact;
	bool m_bMalformedInputExact;
	bool m_bPersistenceExact;
	bool m_bAggressionAdmissionExact;
	string m_sResistanceCasualtyEvidence;
	string m_sReplayAndConflictEvidence;
	string m_sEnemyAndUnknownEvidence;
	string m_sVehicleTheftEvidence;
	string m_sCombatEpisodeEvidence;
	string m_sMinorLocalityEvidence;
	string m_sMalformedInputEvidence;
	string m_sPersistenceEvidence;
	string m_sAggressionAdmissionEvidence;

	bool AllExact()
	{
		return m_bResistanceCasualtyExact
			&& m_bReplayAndConflictExact
			&& m_bEnemyAndUnknownExact
			&& m_bVehicleTheftExact
			&& m_bCombatEpisodeExact
			&& m_bMinorLocalityExact
			&& m_bMalformedInputExact
			&& m_bPersistenceExact
			&& m_bAggressionAdmissionExact;
	}

	string BuildReport()
	{
		return string.Format(
			"civilian consequence proof | all exact %1 | casualty %2 | replay/conflict %3 | attribution %4 | theft %5 | combat %6 | minor %7 | malformed %8 | persistence %9",
			AllExact(),
			m_bResistanceCasualtyExact,
			m_bReplayAndConflictExact,
			m_bEnemyAndUnknownExact,
			m_bVehicleTheftExact,
			m_bCombatEpisodeExact,
			m_bMinorLocalityExact,
			m_bMalformedInputExact,
			m_bPersistenceExact)
			+ string.Format(
				" | aggression admission %1",
				m_bAggressionAdmissionExact);
	}
}

class HST_CivilianConsequenceProofFixture
{
	ref HST_CampaignState m_State;
	ref HST_CampaignPreset m_Preset;
	ref HST_EconomyService m_Economy;
	ref HST_StrategicService m_Strategic;
	ref HST_TownInfluenceService m_TownInfluence;
	ref HST_CivilianConsequenceService m_Consequences;
	ref HST_ZoneState m_Zone;
	ref HST_CivilianZoneState m_Legacy;
	ref HST_TownInfluenceRecord m_Record;
	ref HST_FactionPoolState m_ResistancePool;
	ref HST_FactionPoolState m_OccupierPool;
	ref HST_FactionPoolState m_InvaderPool;
}

// Detached proof for exact civilian consequence policy. It is state-only and
// does not create native entities, inspect the world, or perform persistence IO.
class HST_CivilianConsequenceProofService
{
	static const string TOWN_ID = "civilian_consequence_proof_town";
	static const string MINOR_ID = "civilian_consequence_proof_minor";
	static const string RESISTANCE_FACTION = "FIA";
	static const string OCCUPIER_FACTION = "US";
	static const string INVADER_FACTION = "USSR";

	HST_CivilianConsequenceProofReport Run()
	{
		HST_CivilianConsequenceProofReport report
			= new HST_CivilianConsequenceProofReport();
		ProveResistanceCasualtyAndReplay(report);
		ProveEnemyAndUnknownAttribution(report);
		ProveVehicleTheft(report);
		ProveCombatEpisodes(report);
		ProveMinorLocality(report);
		ProveMalformedInput(report);
		ProvePersistence(report);
		ProveAggressionAdmission(report);
		return report;
	}

	protected void ProveResistanceCasualtyAndReplay(
		HST_CivilianConsequenceProofReport report)
	{
		HST_CivilianConsequenceProofFixture fixture = BuildFixture(true);
		HST_CivilianConsequenceResult casualty
			= fixture.m_Consequences.RegisterPedestrianCasualty(
				fixture.m_State,
				TOWN_ID,
				"civilian_consequence_resistance_casualty",
				RESISTANCE_FACTION,
				"civilian_actor_receipt_1");
		HST_TownInfluenceEventState influenceEvent;
		if (casualty && casualty.m_TownInfluenceResult)
			influenceEvent = casualty.m_TownInfluenceResult.m_Event;
		HST_StrategicEventState strategic = FindStrategicReceipt(
			fixture.m_State,
			"civilian_consequence_resistance_casualty");
		HST_CivilianConsequenceProofFixture scaled = BuildFixture(true, 400);
		HST_CivilianConsequenceResult scaledCasualty
			= scaled.m_Consequences.RegisterPedestrianCasualty(
				scaled.m_State,
				TOWN_ID,
				"civilian_consequence_scaled_casualty",
				RESISTANCE_FACTION,
				"civilian_actor_receipt_scaled");
		bool scaledExact = scaledCasualty && scaledCasualty.m_bAccepted
			&& scaled.m_Record.m_iFIASupportBasisPoints == 4750
			&& scaled.m_Record.m_iOccupierSupportBasisPoints == 5150
			&& scaled.m_Record.m_iRemainingPopulation == 399
			&& scaledCasualty.m_TownInfluenceResult
			&& scaledCasualty.m_TownInfluenceResult.m_Event
			&& scaledCasualty.m_TownInfluenceResult.m_Event.m_bPopulationScaled;
		bool casualtyStateExact = casualty
			&& casualty.m_bAccepted && casualty.m_bChanged
			&& !casualty.m_bAlreadyApplied && casualty.m_bPanicActive
			&& fixture.m_Record.m_iFIASupportBasisPoints == 4500
			&& fixture.m_Record.m_iOccupierSupportBasisPoints == 5300
			&& fixture.m_Record.m_iRemainingPopulation == 99
			&& fixture.m_Record.m_iDestroyedPopulation == 1
			&& fixture.m_Legacy.m_iReputation == 45
			&& fixture.m_Legacy.m_iWantedHeat == 3
			&& fixture.m_OccupierPool.m_iAggression == 2;
		bool casualtyEventExact = influenceEvent
			&& influenceEvent.m_sKind
				== HST_CivilianConsequenceService.EVENT_CIVILIAN_CASUALTY
			&& influenceEvent.m_iFIASupportDelta == -5
			&& influenceEvent.m_iOccupierSupportDelta == 3
			&& influenceEvent.m_iPopulationDelta == -1
			&& influenceEvent.m_bPopulationScaled
			&& influenceEvent.m_sAggressionFactionKey == OCCUPIER_FACTION
			&& influenceEvent.m_iAggressionDelta == 2
			&& influenceEvent.m_iAggressionBefore == 0
			&& influenceEvent.m_iAggressionAfter == 2;
		bool casualtyStrategicExact = strategic && strategic.m_bApplied
			&& strategic.m_sSourceId == influenceEvent.m_sEventId
			&& strategic.m_sTargetFactionKey == OCCUPIER_FACTION
			&& strategic.m_iAggressionDelta == 2;
		report.m_bResistanceCasualtyExact = casualtyStateExact
			&& casualtyEventExact && casualtyStrategicExact && scaledExact;
		report.m_sResistanceCasualtyEvidence = string.Format(
			"accepted %1 | FIA/occupier %2/%3 bp | population %4/%5 | reputation/heat %6/%7 | aggression %8 | strategic %9",
			casualty && casualty.m_bAccepted,
			fixture.m_Record.m_iFIASupportBasisPoints,
			fixture.m_Record.m_iOccupierSupportBasisPoints,
			fixture.m_Record.m_iRemainingPopulation,
			fixture.m_Record.m_iDestroyedPopulation,
			fixture.m_Legacy.m_iReputation,
			fixture.m_Legacy.m_iWantedHeat,
			fixture.m_OccupierPool.m_iAggression,
			strategic != null);
		report.m_sResistanceCasualtyEvidence
			= report.m_sResistanceCasualtyEvidence + string.Format(
				" | scaled-400 exact %1 FIA/enemy %2/%3 bp",
				scaledExact,
				scaled.m_Record.m_iFIASupportBasisPoints,
				scaled.m_Record.m_iOccupierSupportBasisPoints);

		int recordRevisionBeforeReplay = fixture.m_Record.m_iRevision;
		int zoneRevisionBeforeReplay
			= fixture.m_Zone.m_iCivilianConsequenceRevision;
		fixture.m_OccupierPool.m_iAggression = int.MAX - 1;
		HST_CivilianConsequenceResult replay
			= fixture.m_Consequences.RegisterPedestrianCasualty(
				fixture.m_State,
				TOWN_ID,
				"civilian_consequence_resistance_casualty",
				RESISTANCE_FACTION,
				"civilian_actor_receipt_1");
		bool replayReadOnlyAtHighAggression = replay
			&& replay.m_bAccepted && replay.m_bAlreadyApplied
			&& !replay.m_bChanged
			&& fixture.m_Record.m_iRevision == recordRevisionBeforeReplay
			&& fixture.m_Zone.m_iCivilianConsequenceRevision
				== zoneRevisionBeforeReplay
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 1
			&& fixture.m_State.m_aStrategicEvents.Count() == 1
			&& fixture.m_OccupierPool.m_iAggression == int.MAX - 1;
		HST_CivilianConsequenceResult conflict
			= fixture.m_Consequences.RegisterPedestrianCasualty(
				fixture.m_State,
				TOWN_ID,
				"civilian_consequence_resistance_casualty",
				"",
				"civilian_actor_receipt_1");
		bool conflictExact = conflict && !conflict.m_bAccepted
			&& !conflict.m_sFailureReason.IsEmpty()
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 1
			&& fixture.m_State.m_aStrategicEvents.Count() == 1
			&& fixture.m_Record.m_iRemainingPopulation == 99
			&& fixture.m_OccupierPool.m_iAggression == int.MAX - 1;

		fixture.m_State.m_iElapsedSeconds = 200;
		HST_CivilianConsequenceResult newer
			= fixture.m_Consequences.RegisterPedestrianCasualty(
				fixture.m_State,
				TOWN_ID,
				"civilian_consequence_newer_unknown_casualty",
				"",
				"civilian_actor_receipt_newer");
		string lastEventBeforeOlderReplay
			= fixture.m_Zone.m_sCivilianLastConsequenceEventId;
		int panicBeforeOlderReplay
			= fixture.m_Zone.m_iCivilianPanicUntilSecond;
		int zoneRevisionBeforeOlderReplay
			= fixture.m_Zone.m_iCivilianConsequenceRevision;
		int recordRevisionBeforeOlderReplay = fixture.m_Record.m_iRevision;
		fixture.m_State.m_iElapsedSeconds = 230;
		HST_CivilianConsequenceResult olderReplay
			= fixture.m_Consequences.RegisterPedestrianCasualty(
				fixture.m_State,
				TOWN_ID,
				"civilian_consequence_resistance_casualty",
				RESISTANCE_FACTION,
				"civilian_actor_receipt_1");
		bool olderReplayReadOnly = newer && newer.m_bAccepted
			&& olderReplay && olderReplay.m_bAccepted
			&& olderReplay.m_bAlreadyApplied && !olderReplay.m_bChanged
			&& fixture.m_Zone.m_sCivilianLastConsequenceEventId
				== lastEventBeforeOlderReplay
			&& fixture.m_Zone.m_iCivilianPanicUntilSecond
				== panicBeforeOlderReplay
			&& fixture.m_Zone.m_iCivilianConsequenceRevision
				== zoneRevisionBeforeOlderReplay
			&& fixture.m_Record.m_iRevision
				== recordRevisionBeforeOlderReplay;
		report.m_bReplayAndConflictExact = replayReadOnlyAtHighAggression
			&& conflictExact && olderReplayReadOnly;
		report.m_sReplayAndConflictEvidence = string.Format(
			"high-aggression replay read-only %1 | conflict rejected %2 | older replay read-only %3 | influence/strategic %4/%5 | population %6 | aggression %7",
			replayReadOnlyAtHighAggression,
			conflictExact,
			olderReplayReadOnly,
			fixture.m_State.m_aTownInfluenceEvents.Count(),
			fixture.m_State.m_aStrategicEvents.Count(),
			fixture.m_Record.m_iRemainingPopulation,
			fixture.m_OccupierPool.m_iAggression);
	}

	protected void ProveEnemyAndUnknownAttribution(
		HST_CivilianConsequenceProofReport report)
	{
		HST_CivilianConsequenceProofFixture enemy = BuildFixture(true);
		HST_CivilianConsequenceResult enemyResult
			= enemy.m_Consequences.RegisterPedestrianCasualty(
				enemy.m_State,
				TOWN_ID,
				"civilian_consequence_enemy_casualty",
				OCCUPIER_FACTION,
				"civilian_actor_receipt_enemy");
		bool enemyExact = enemyResult && enemyResult.m_bAccepted
			&& enemy.m_Record.m_iFIASupportBasisPoints == 5300
			&& enemy.m_Record.m_iOccupierSupportBasisPoints == 4500
			&& enemy.m_Record.m_iRemainingPopulation == 99
			&& enemy.m_Legacy.m_iReputation == 53
			&& enemy.m_Legacy.m_iWantedHeat == 3
			&& enemy.m_OccupierPool.m_iAggression == 0;

		HST_CivilianConsequenceProofFixture unknown = BuildFixture(true);
		HST_CivilianConsequenceResult unknownResult
			= unknown.m_Consequences.RegisterPedestrianCasualty(
				unknown.m_State,
				TOWN_ID,
				"civilian_consequence_unknown_casualty",
				"",
				"civilian_actor_receipt_unknown");
		bool unknownExact = unknownResult && unknownResult.m_bAccepted
			&& unknown.m_Record.m_iFIASupportBasisPoints == 5000
			&& unknown.m_Record.m_iOccupierSupportBasisPoints == 5000
			&& unknown.m_Record.m_iRemainingPopulation == 99
			&& unknown.m_Legacy.m_iReputation == 50
			&& unknown.m_Legacy.m_iWantedHeat == 2
			&& unknown.m_OccupierPool.m_iAggression == 0;
		report.m_bEnemyAndUnknownExact = enemyExact && unknownExact;
		report.m_sEnemyAndUnknownEvidence = string.Format(
			"enemy exact %1 FIA/enemy %2/%3 rep/heat %4/%5 | unknown exact %6 FIA/enemy %7/%8 rep/heat %9",
			enemyExact,
			enemy.m_Record.m_iFIASupportBasisPoints,
			enemy.m_Record.m_iOccupierSupportBasisPoints,
			enemy.m_Legacy.m_iReputation,
			enemy.m_Legacy.m_iWantedHeat,
			unknownExact,
			unknown.m_Record.m_iFIASupportBasisPoints,
			unknown.m_Record.m_iOccupierSupportBasisPoints,
			string.Format(
				"%1/%2",
				unknown.m_Legacy.m_iReputation,
				unknown.m_Legacy.m_iWantedHeat));
	}

	protected void ProveVehicleTheft(
		HST_CivilianConsequenceProofReport report)
	{
		HST_CivilianConsequenceProofFixture fixture = BuildFixture(true);
		HST_CivilianConsequenceResult theft
			= fixture.m_Consequences.RegisterCivilianVehicleTheft(
				fixture.m_State,
				TOWN_ID,
				"civilian_vehicle_theft_9101",
				RESISTANCE_FACTION,
				"civilian_vehicle_runtime_9101");
		HST_CivilianConsequenceResult replay
			= fixture.m_Consequences.RegisterCivilianVehicleTheft(
				fixture.m_State,
				TOWN_ID,
				"civilian_vehicle_theft_9101",
				RESISTANCE_FACTION,
				"civilian_vehicle_runtime_9101");
		report.m_bVehicleTheftExact = theft && theft.m_bAccepted
			&& replay && replay.m_bAccepted && replay.m_bAlreadyApplied
			&& fixture.m_Record.m_iFIASupportBasisPoints == 4800
			&& fixture.m_Record.m_iOccupierSupportBasisPoints == 5100
			&& fixture.m_Record.m_iRemainingPopulation == 100
			&& fixture.m_Legacy.m_iReputation == 48
			&& fixture.m_Legacy.m_iWantedHeat == 2
			&& fixture.m_OccupierPool.m_iAggression == 1
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 1
			&& fixture.m_State.m_aStrategicEvents.Count() == 1;
		report.m_sVehicleTheftEvidence = string.Format(
			"accepted/replay %1/%2 | FIA/enemy %3/%4 | population %5 | rep/heat %6/%7 | aggression %8 | events %9",
			theft && theft.m_bAccepted,
			replay && replay.m_bAlreadyApplied,
			fixture.m_Record.m_iFIASupportBasisPoints,
			fixture.m_Record.m_iOccupierSupportBasisPoints,
			fixture.m_Record.m_iRemainingPopulation,
			fixture.m_Legacy.m_iReputation,
			fixture.m_Legacy.m_iWantedHeat,
			fixture.m_OccupierPool.m_iAggression,
			fixture.m_State.m_aTownInfluenceEvents.Count());
	}

	protected void ProveCombatEpisodes(
		HST_CivilianConsequenceProofReport report)
	{
		HST_CivilianConsequenceProofFixture baseline = BuildFixture(true);
		baseline.m_Zone.m_bCivilianCombatDangerActive = true;
		baseline.m_Zone.m_iCivilianCombatEpisodeCount = 1;
		baseline.m_Zone.m_iCivilianAdoptedCombatEpisodeCount = 1;
		baseline.m_Zone.m_iCivilianLastAppliedCombatEpisodeCount = 1;
		baseline.m_Zone.m_iCivilianLastCombatPresenceRevision = 1;
		baseline.m_Zone.m_iCivilianDangerChangedSecond = 100;
		baseline.m_Zone.m_iCivilianPanicUntilSecond = 145;
		PublishCombatFacts(
			baseline,
			1,
			1,
			0,
			"migrated_active_baseline");
		HST_CivilianConsequenceResult baselineObservation
			= baseline.m_Consequences.ObserveNearbyCombat(
				baseline.m_State,
				TOWN_ID,
				1,
				1,
				0,
				"migrated_active_baseline");
		bool baselineExact = baselineObservation
			&& baselineObservation.m_bAccepted
			&& baselineObservation.m_bAlreadyApplied
			&& baseline.m_State.m_aTownInfluenceEvents.Count() == 0
			&& baseline.m_State.m_aStrategicEvents.Count() == 0
			&& baseline.m_Legacy.m_iWantedHeat == 0;

		HST_CivilianConsequenceProofFixture fixture = BuildFixture(true);
		fixture.m_Zone.m_eCombatPresenceState
			= HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT;
		PublishCombatFacts(fixture, 2, 0, 0, "garrison_only");
		HST_CivilianConsequenceResult garrisonOnly
			= fixture.m_Consequences.ObserveNearbyCombat(
				fixture.m_State,
				TOWN_ID,
				2,
				0,
				0,
				"garrison_only");
		bool hotAloneIgnored = garrisonOnly && garrisonOnly.m_bAccepted
			&& !fixture.m_Zone.m_bCivilianCombatDangerActive
			&& fixture.m_Zone.m_iCivilianCombatEpisodeCount == 0
			&& fixture.m_Zone.m_iCivilianLastAppliedCombatEpisodeCount == 0
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 0
			&& fixture.m_Legacy.m_iWantedHeat == 0;

		PublishCombatFacts(fixture, 3, 1, 0, "operation_a");
		HST_CivilianConsequenceResult firstEpisode
			= fixture.m_Consequences.ObserveNearbyCombat(
				fixture.m_State,
				TOWN_ID,
				3,
				1,
				0,
				"operation_a");
		HST_CivilianConsequenceResult firstReplay
			= fixture.m_Consequences.ObserveNearbyCombat(
				fixture.m_State,
				TOWN_ID,
				3,
				1,
				0,
				"operation_a");
		bool firstExact = firstEpisode && firstEpisode.m_bAccepted
			&& firstEpisode.m_bPanicActive
			&& firstReplay && firstReplay.m_bAccepted
			&& firstReplay.m_bAlreadyApplied
			&& fixture.m_Zone.m_bCivilianCombatDangerActive
			&& fixture.m_Zone.m_iCivilianCombatEpisodeCount == 1
			&& fixture.m_Zone.m_iCivilianLastAppliedCombatEpisodeCount == 1
			&& firstEpisode.m_sEventId
				== "civilian_combat_" + TOWN_ID + "_1"
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 1
			&& fixture.m_Legacy.m_iWantedHeat == 4;

		fixture.m_State.m_iElapsedSeconds = 110;
		PublishCombatFacts(fixture, 4, 0, 0, "recovered");
		HST_CivilianConsequenceResult recovery
			= fixture.m_Consequences.ObserveNearbyCombat(
				fixture.m_State,
				TOWN_ID,
				4,
				0,
				0,
				"recovered");
		bool recoveryExact = recovery && recovery.m_bAccepted
			&& !fixture.m_Zone.m_bCivilianCombatDangerActive
			&& fixture.m_Zone.m_iCivilianCombatEpisodeCount == 1
			&& fixture.m_Zone.m_iCivilianPanicUntilSecond == 145
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 1;

		fixture.m_State.m_iElapsedSeconds = 150;
		PublishCombatFacts(fixture, 5, 0, 0, "recovery_expired");
		HST_CivilianConsequenceResult expiredRecovery
			= fixture.m_Consequences.ObserveNearbyCombat(
				fixture.m_State,
				TOWN_ID,
				5,
				0,
				0,
				"recovery_expired");
		bool expiryExact = expiredRecovery && expiredRecovery.m_bAccepted
			&& fixture.m_Zone.m_iCivilianPanicUntilSecond == 0
			&& !fixture.m_Zone.m_bCivilianCombatDangerActive
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 1;

		fixture.m_State.m_iElapsedSeconds = 160;
		PublishCombatFacts(fixture, 6, 0, 1, "recent_fire_b");
		HST_CivilianConsequenceResult secondEpisode
			= fixture.m_Consequences.ObserveNearbyCombat(
				fixture.m_State,
				TOWN_ID,
				6,
				0,
				1,
				"recent_fire_b");
		bool secondExact = secondEpisode && secondEpisode.m_bAccepted
			&& secondEpisode.m_bPanicActive
			&& fixture.m_Zone.m_bCivilianCombatDangerActive
			&& fixture.m_Zone.m_iCivilianCombatEpisodeCount == 2
			&& fixture.m_Zone.m_iCivilianLastAppliedCombatEpisodeCount == 2
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 2
			&& fixture.m_State.m_aStrategicEvents.Count() == 2
			&& fixture.m_Legacy.m_iWantedHeat == 8
			&& fixture.m_Record.m_iRemainingPopulation == 100
			&& fixture.m_OccupierPool.m_iAggression == 0;

		HST_CivilianConsequenceProofFixture pending = BuildFixture(true);
		pending.m_Consequences.SetTownInfluenceService(null);
		PublishCombatFacts(pending, 2, 1, 0, "pending_operation");
		HST_CivilianConsequenceResult rejectedEpisode
			= pending.m_Consequences.ObserveNearbyCombat(
				pending.m_State,
				TOWN_ID,
				2,
				1,
				0,
				"pending_operation");
		pending.m_Consequences.SetTownInfluenceService(
			pending.m_TownInfluence);
		PublishCombatFacts(pending, 3, 0, 0, "pending_cleared");
		HST_CivilianConsequenceResult drainedOnClear
			= pending.m_Consequences.ObserveNearbyCombat(
				pending.m_State,
				TOWN_ID,
				3,
				0,
				0,
				"pending_cleared");
		PublishCombatFacts(pending, 4, 0, 1, "pending_rebound");
		HST_CivilianConsequenceResult reboundEpisode
			= pending.m_Consequences.ObserveNearbyCombat(
				pending.m_State,
				TOWN_ID,
				4,
				0,
				1,
				"pending_rebound");
		bool pendingDrainExact = rejectedEpisode
			&& !rejectedEpisode.m_bAccepted
			&& drainedOnClear && drainedOnClear.m_bAccepted
			&& reboundEpisode && reboundEpisode.m_bAccepted
			&& pending.m_Zone.m_iCivilianCombatEpisodeCount == 2
			&& pending.m_Zone.m_iCivilianLastAppliedCombatEpisodeCount == 2
			&& pending.m_State.m_aTownInfluenceEvents.Count() == 2
			&& pending.m_Legacy.m_iWantedHeat == 8;
		report.m_bCombatEpisodeExact = baselineExact && hotAloneIgnored
			&& firstExact && recoveryExact && expiryExact && secondExact
			&& pendingDrainExact;
		report.m_sCombatEpisodeEvidence = string.Format(
			"HOT ignored %1 | first/replay %2/%3 | recovery/expiry %4/%5 | second %6 | episodes %7 | heat %8 | events %9",
			hotAloneIgnored,
			firstExact,
			firstReplay && firstReplay.m_bAlreadyApplied,
			recoveryExact,
			expiryExact,
			secondExact,
			fixture.m_Zone.m_iCivilianCombatEpisodeCount,
			fixture.m_Legacy.m_iWantedHeat,
			string.Format(
				"%1/%2",
				fixture.m_State.m_aTownInfluenceEvents.Count(),
				fixture.m_State.m_aStrategicEvents.Count()));
		report.m_sCombatEpisodeEvidence
			= report.m_sCombatEpisodeEvidence + string.Format(
				" | migrated baseline no invented heat %1 | pending drain/rebound %2",
				baselineExact,
				pendingDrainExact);
	}

	protected void ProveMinorLocality(
		HST_CivilianConsequenceProofReport report)
	{
		HST_CivilianConsequenceProofFixture fixture = BuildFixture(false);
		PublishCombatFacts(fixture, 2, 0, 1, "minor_recent_fire");
		HST_CivilianConsequenceResult combat
			= fixture.m_Consequences.ObserveNearbyCombat(
				fixture.m_State,
				MINOR_ID,
				2,
				0,
				1,
				"minor_recent_fire");
		HST_CivilianConsequenceResult casualty
			= fixture.m_Consequences.RegisterPedestrianCasualty(
				fixture.m_State,
				MINOR_ID,
				"civilian_minor_casualty",
				"",
				"minor_actor_receipt");
		HST_CivilianConsequenceResult casualtyReplay
			= fixture.m_Consequences.RegisterPedestrianCasualty(
				fixture.m_State,
				MINOR_ID,
				"civilian_minor_casualty",
				"",
				"minor_actor_receipt");
		fixture.m_Consequences.ResetRuntimeSession();
		HST_CivilianConsequenceResult newSessionReceipt
			= fixture.m_Consequences.RegisterPedestrianCasualty(
				fixture.m_State,
				MINOR_ID,
				"civilian_minor_casualty",
				"",
				"minor_actor_receipt");
		bool minorEffectsExact = combat && combat.m_bAccepted
			&& combat.m_bPanicOnly && combat.m_bPanicActive
			&& casualty && casualty.m_bAccepted
			&& casualty.m_bPanicOnly && casualty.m_bPanicActive;
		bool minorReceiptExact = casualtyReplay
			&& casualtyReplay.m_bAlreadyApplied
			&& newSessionReceipt && newSessionReceipt.m_bAccepted
			&& !newSessionReceipt.m_bAlreadyApplied;
		bool minorStateExact = fixture.m_Zone.m_bCivilianCombatDangerActive
			&& fixture.m_Zone.m_iCivilianCombatEpisodeCount == 1
			&& fixture.m_Zone.m_iCivilianLastAppliedCombatEpisodeCount == 1;
		bool minorHistoryExact
			= fixture.m_State.m_aTownInfluenceRecords.Count() == 0
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 0
			&& fixture.m_State.m_aStrategicEvents.Count() == 0;
		report.m_bMinorLocalityExact = minorEffectsExact
			&& minorReceiptExact && minorStateExact && minorHistoryExact;
		report.m_sMinorLocalityEvidence = string.Format(
			"combat accepted/panic-only %1/%2 | casualty accepted/panic-only %3/%4 | danger/episodes %5/%6 | influence/strategic %7/%8",
			combat && combat.m_bAccepted,
			combat && combat.m_bPanicOnly,
			casualty && casualty.m_bAccepted,
			casualty && casualty.m_bPanicOnly,
			fixture.m_Zone.m_bCivilianCombatDangerActive,
			fixture.m_Zone.m_iCivilianCombatEpisodeCount,
			fixture.m_State.m_aTownInfluenceEvents.Count(),
			fixture.m_State.m_aStrategicEvents.Count());
		report.m_sMinorLocalityEvidence
			= report.m_sMinorLocalityEvidence + string.Format(
				" | same-session replay/new-session reset %1/%2",
				casualtyReplay && casualtyReplay.m_bAlreadyApplied,
				newSessionReceipt && !newSessionReceipt.m_bAlreadyApplied);
	}

	protected void ProveMalformedInput(
		HST_CivilianConsequenceProofReport report)
	{
		HST_CivilianConsequenceProofFixture fixture = BuildFixture(true);
		int recordRevisionBefore = fixture.m_Record.m_iRevision;
		int zoneRevisionBefore
			= fixture.m_Zone.m_iCivilianConsequenceRevision;
		HST_CivilianConsequenceResult blankId
			= fixture.m_Consequences.RegisterPedestrianCasualty(
				fixture.m_State,
				TOWN_ID,
				"   ",
				RESISTANCE_FACTION);
		HST_CivilianConsequenceResult badFaction
			= fixture.m_Consequences.RegisterPedestrianCasualty(
				fixture.m_State,
				TOWN_ID,
				"civilian_consequence_bad_faction",
				"not_a_campaign_faction");
		PublishCombatFacts(fixture, 2, 0, 0, "");
		HST_CivilianConsequenceResult badCounts
			= fixture.m_Consequences.ObserveNearbyCombat(
				fixture.m_State,
				TOWN_ID,
				2,
				-1,
				0);

		HST_CivilianConsequenceProofFixture corrupt = BuildFixture(true);
		corrupt.m_Zone.m_iCivilianConsequenceContractVersion = 99;
		HST_CivilianConsequenceResult badContract
			= corrupt.m_Consequences.ObserveNearbyCombat(
				corrupt.m_State,
				TOWN_ID,
				2,
				1,
				0);
		report.m_bMalformedInputExact = blankId && !blankId.m_bAccepted
			&& badFaction && !badFaction.m_bAccepted
			&& badCounts && !badCounts.m_bAccepted
			&& badContract && !badContract.m_bAccepted
			&& fixture.m_Record.m_iRevision == recordRevisionBefore
			&& fixture.m_Zone.m_iCivilianConsequenceRevision
				== zoneRevisionBefore
			&& fixture.m_Record.m_iRemainingPopulation == 100
			&& fixture.m_State.m_aTownInfluenceEvents.Count() == 0
			&& fixture.m_State.m_aStrategicEvents.Count() == 0
			&& fixture.m_OccupierPool.m_iAggression == 0
			&& fixture.m_Zone.m_sCivilianConsequenceAuthorityFailure.IsEmpty()
			&& corrupt.m_State.m_aTownInfluenceEvents.Count() == 0;
		report.m_sMalformedInputEvidence = string.Format(
			"blank/faction/counts/contract rejected %1/%2/%3/%4 | record revision %5 | population %6 | events %7/%8 | aggression %9",
			blankId && !blankId.m_bAccepted,
			badFaction && !badFaction.m_bAccepted,
			badCounts && !badCounts.m_bAccepted,
			badContract && !badContract.m_bAccepted,
			fixture.m_Record.m_iRevision,
			fixture.m_Record.m_iRemainingPopulation,
			fixture.m_State.m_aTownInfluenceEvents.Count(),
			fixture.m_State.m_aStrategicEvents.Count(),
			fixture.m_OccupierPool.m_iAggression);
	}

	protected void ProvePersistence(
		HST_CivilianConsequenceProofReport report)
	{
		HST_CivilianConsequenceProofFixture fixture = BuildFixture(true);
		HST_CivilianConsequenceResult casualty
			= fixture.m_Consequences.RegisterPedestrianCasualty(
				fixture.m_State,
				TOWN_ID,
				"civilian_consequence_persistence_casualty",
				RESISTANCE_FACTION,
				"civilian_persistence_actor");
		PublishCombatFacts(fixture, 2, 0, 1, "persistence_recent_fire");
		HST_CivilianConsequenceResult combat
			= fixture.m_Consequences.ObserveNearbyCombat(
				fixture.m_State,
				TOWN_ID,
				2,
				0,
				1,
				"persistence_recent_fire");
		HST_CampaignSaveData current = new HST_CampaignSaveData();
		current.Capture(fixture.m_State);
		HST_TownInfluenceSaveValidationService townValidator
			= new HST_TownInfluenceSaveValidationService();
		townValidator.ValidateCurrentAuthorityBeforeOwnership(
			current,
			HST_CampaignState.SCHEMA_VERSION);
		townValidator.ValidateAfterOwnership(current);
		HST_CivilianConsequenceSaveValidationService consequenceValidator
			= new HST_CivilianConsequenceSaveValidationService();
		consequenceValidator.Normalize(
			current,
			HST_CampaignState.SCHEMA_VERSION);
		HST_ZoneState savedZone;
		if (!current.m_aZones.IsEmpty())
			savedZone = current.m_aZones[0];
		HST_TownInfluenceEventState savedEvent;
		if (!current.m_aTownInfluenceEvents.IsEmpty())
			savedEvent = current.m_aTownInfluenceEvents[0];
		bool currentExact = casualty && casualty.m_bAccepted
			&& combat && combat.m_bAccepted
			&& savedZone
			&& savedZone.m_iCivilianConsequenceContractVersion
				== HST_CivilianConsequenceService.CONTRACT_VERSION
			&& savedZone.m_iCivilianAdoptedCombatEpisodeCount == 0
			&& savedZone.m_iCivilianLastAppliedCombatEpisodeCount == 1
			&& savedZone.m_sCivilianLastConsequenceEventId
				== "civilian_combat_" + TOWN_ID + "_1"
			&& savedEvent
			&& savedEvent.m_iContractVersion
				== HST_TownInfluenceService.EXACT_CONTRACT_VERSION
			&& savedEvent.m_iAggressionBefore == 0
			&& savedEvent.m_iAggressionAfter == 2;

		HST_CivilianConsequenceProofFixture migration = BuildFixture(true);
		PublishCombatFacts(migration, 5, 1, 0, "migration_contact");
		HST_CampaignSaveData legacy = new HST_CampaignSaveData();
		legacy.Capture(migration.m_State);
		legacy.m_iSchemaVersion = 64;
		consequenceValidator.Normalize(legacy, 64);
		HST_ZoneState migratedZone;
		if (!legacy.m_aZones.IsEmpty())
			migratedZone = legacy.m_aZones[0];
		bool migrationExact = migratedZone
			&& migratedZone.m_bCivilianCombatDangerActive
			&& migratedZone.m_iCivilianCombatEpisodeCount == 1
			&& migratedZone.m_iCivilianAdoptedCombatEpisodeCount == 1
			&& migratedZone.m_iCivilianLastAppliedCombatEpisodeCount == 1
			&& migratedZone.m_iCivilianLastCombatPresenceRevision == 5
			&& migratedZone.m_iCivilianPanicUntilSecond
				> legacy.m_iElapsedSeconds
			&& legacy.m_aTownInfluenceEvents.IsEmpty()
			&& migration.m_Legacy.m_iWantedHeat == 0;

		HST_CampaignSaveData malformed = new HST_CampaignSaveData();
		malformed.Capture(migration.m_State);
		HST_ZoneState malformedZone = malformed.m_aZones[0];
		malformedZone.m_iCivilianCombatEpisodeCount = 1;
		malformedZone.m_iCivilianLastAppliedCombatEpisodeCount = 2;
		consequenceValidator.Normalize(
			malformed,
			HST_CampaignState.SCHEMA_VERSION);
		bool malformedExact = malformedZone.m_iCivilianConsequenceContractVersion
			== HST_CivilianConsequenceSaveValidationService.QUARANTINE_CONTRACT_VERSION
			&& !malformedZone.m_bCivilianCombatDangerActive;

		HST_CampaignSaveData tampered = new HST_CampaignSaveData();
		tampered.Capture(fixture.m_State);
		HST_TownInfluenceEventState tamperedEvent
			= tampered.m_aTownInfluenceEvents[0];
		tamperedEvent.m_iAggressionAfter
			= tamperedEvent.m_iAggressionAfter + 1;
		HST_TownInfluenceSaveValidationService tamperValidator
			= new HST_TownInfluenceSaveValidationService();
		tamperValidator.ValidateCurrentAuthorityBeforeOwnership(
			tampered,
			HST_CampaignState.SCHEMA_VERSION);
		bool tamperExact = tamperedEvent.m_iContractVersion
			== HST_TownInfluenceSaveValidationService.QUARANTINE_CONTRACT_VERSION
			&& !tamperedEvent.m_bApplied;

		HST_CampaignSaveData missingCombatReceipt = new HST_CampaignSaveData();
		missingCombatReceipt.Capture(fixture.m_State);
		if (missingCombatReceipt.m_aTownInfluenceEvents.Count() > 1)
			missingCombatReceipt.m_aTownInfluenceEvents.Remove(1);
		HST_ZoneState missingCombatZone = missingCombatReceipt.m_aZones[0];
		missingCombatZone.m_sCivilianLastConsequenceEventId
			= "civilian_consequence_persistence_casualty";
		consequenceValidator.Normalize(
			missingCombatReceipt,
			HST_CampaignState.SCHEMA_VERSION);
		bool missingCombatReceiptExact
			= missingCombatZone.m_iCivilianConsequenceContractVersion
				== HST_CivilianConsequenceSaveValidationService.QUARANTINE_CONTRACT_VERSION;

		HST_CampaignSaveData wrongCombatFingerprint
			= new HST_CampaignSaveData();
		wrongCombatFingerprint.Capture(fixture.m_State);
		HST_TownInfluenceEventState wrongCombatEvent;
		if (wrongCombatFingerprint.m_aTownInfluenceEvents.Count() > 1)
			wrongCombatEvent = wrongCombatFingerprint.m_aTownInfluenceEvents[1];
		if (wrongCombatEvent)
			wrongCombatEvent.m_iHeatDelta = 0;
		HST_ZoneState wrongCombatZone = wrongCombatFingerprint.m_aZones[0];
		consequenceValidator.Normalize(
			wrongCombatFingerprint,
			HST_CampaignState.SCHEMA_VERSION);
		bool wrongCombatFingerprintExact = wrongCombatEvent
			&& wrongCombatZone.m_iCivilianConsequenceContractVersion
				== HST_CivilianConsequenceSaveValidationService.QUARANTINE_CONTRACT_VERSION;

		report.m_bPersistenceExact = currentExact
			&& migrationExact
			&& malformedExact
			&& tamperExact
			&& missingCombatReceiptExact
			&& wrongCombatFingerprintExact;
		int savedAggressionBefore;
		int savedAggressionAfter;
		if (savedEvent)
		{
			savedAggressionBefore = savedEvent.m_iAggressionBefore;
			savedAggressionAfter = savedEvent.m_iAggressionAfter;
		}
		int migratedEpisode;
		int migratedAppliedEpisode;
		if (migratedZone)
		{
			migratedEpisode = migratedZone.m_iCivilianCombatEpisodeCount;
			migratedAppliedEpisode
				= migratedZone.m_iCivilianLastAppliedCombatEpisodeCount;
		}
		report.m_sPersistenceEvidence = string.Format(
			"current/migration/malformed/tamper %1/%2/%3/%4 | aggression %5->%6 | migrated episode/applied %7/%8 | quarantine %9",
			currentExact,
			migrationExact,
			malformedExact,
			tamperExact,
			savedAggressionBefore,
			savedAggressionAfter,
			migratedEpisode,
			migratedAppliedEpisode,
			malformedZone.m_iCivilianConsequenceContractVersion);
		report.m_sPersistenceEvidence
			= report.m_sPersistenceEvidence + string.Format(
				" | missing/wrong live combat receipt quarantined %1/%2",
				missingCombatReceiptExact,
				wrongCombatFingerprintExact);
	}

	protected void ProveAggressionAdmission(
		HST_CivilianConsequenceProofReport report)
	{
		HST_CivilianConsequenceProofFixture missingStrategic
			= BuildFixture(true);
		missingStrategic.m_TownInfluence.SetStrategicService(null);
		HST_TownInfluenceResult missingStrategicResult
			= missingStrategic.m_TownInfluence.Execute(
				missingStrategic.m_State,
				BuildAggressionAdmissionCommand(
					"civilian_admission_missing_strategic",
					OCCUPIER_FACTION,
					1));

		HST_CivilianConsequenceProofFixture missingEconomy
			= BuildFixture(true);
		missingEconomy.m_TownInfluence.SetEconomyService(null);
		HST_TownInfluenceResult missingEconomyResult
			= missingEconomy.m_TownInfluence.Execute(
				missingEconomy.m_State,
				BuildAggressionAdmissionCommand(
					"civilian_admission_missing_economy",
					OCCUPIER_FACTION,
					1));

		HST_CivilianConsequenceProofFixture duplicatePool
			= BuildFixture(true);
		duplicatePool.m_State.m_aFactionPools.Insert(
			BuildFactionPool(OCCUPIER_FACTION));
		HST_TownInfluenceResult duplicatePoolResult
			= duplicatePool.m_TownInfluence.Execute(
				duplicatePool.m_State,
				BuildAggressionAdmissionCommand(
					"civilian_admission_duplicate_pool",
					OCCUPIER_FACTION,
					1));

		HST_CivilianConsequenceProofFixture nonEnemy = BuildFixture(true);
		HST_TownInfluenceResult nonEnemyResult
			= nonEnemy.m_TownInfluence.Execute(
				nonEnemy.m_State,
				BuildAggressionAdmissionCommand(
					"civilian_admission_non_enemy",
					RESISTANCE_FACTION,
					1));

		HST_CivilianConsequenceProofFixture negativePool = BuildFixture(true);
		negativePool.m_OccupierPool.m_iAggression = -1;
		HST_TownInfluenceResult negativePoolResult
			= negativePool.m_TownInfluence.Execute(
				negativePool.m_State,
				BuildAggressionAdmissionCommand(
					"civilian_admission_negative_pool",
					OCCUPIER_FACTION,
					1));

		HST_CivilianConsequenceProofFixture overflow = BuildFixture(true);
		overflow.m_OccupierPool.m_iAggression = int.MAX;
		HST_TownInfluenceResult overflowResult
			= overflow.m_TownInfluence.Execute(
				overflow.m_State,
				BuildAggressionAdmissionCommand(
					"civilian_admission_overflow",
					OCCUPIER_FACTION,
					1));

		HST_CivilianConsequenceProofFixture collision = BuildFixture(true);
		HST_StrategicEventState claimant = new HST_StrategicEventState();
		claimant.m_sEventId = "civilian_admission_existing_strategic";
		claimant.m_sSourceType = "town_influence";
		claimant.m_sSourceId = "civilian_admission_source_collision";
		collision.m_State.m_aStrategicEvents.Insert(claimant);
		HST_TownInfluenceResult collisionResult
			= collision.m_TownInfluence.Execute(
				collision.m_State,
				BuildAggressionAdmissionCommand(
					"civilian_admission_source_collision",
					OCCUPIER_FACTION,
					1));

		HST_CivilianConsequenceProofFixture idExhaustion = BuildFixture(true);
		idExhaustion.m_State.m_iNextAuthoritySequence = int.MAX;
		HST_TownInfluenceResult idExhaustionResult
			= idExhaustion.m_TownInfluence.Execute(
				idExhaustion.m_State,
				BuildAggressionAdmissionCommand(
					"civilian_admission_id_exhaustion",
					OCCUPIER_FACTION,
					1));

		HST_CivilianConsequenceProofFixture restoredRoleTamper
			= BuildFixture(true);
		HST_TownInfluenceResult restoredRoleSeed
			= restoredRoleTamper.m_TownInfluence.Execute(
				restoredRoleTamper.m_State,
				BuildAggressionAdmissionCommand(
					"civilian_admission_restore_role_tamper",
					OCCUPIER_FACTION,
					1));
		HST_TownInfluenceEventState restoredRoleEvent;
		if (restoredRoleSeed)
			restoredRoleEvent = restoredRoleSeed.m_Event;
		if (restoredRoleEvent)
			restoredRoleEvent.m_sAggressionFactionKey = RESISTANCE_FACTION;
		bool restoredRoleChanged
			= restoredRoleTamper.m_TownInfluence
				.ValidateRestoredAggressionFactionRoles(
					restoredRoleTamper.m_State,
					restoredRoleTamper.m_Preset);
		bool restoredRoleQuarantined = restoredRoleChanged
			&& restoredRoleEvent
			&& restoredRoleEvent.m_iContractVersion
				== HST_TownInfluenceSaveValidationService.QUARANTINE_CONTRACT_VERSION
			&& !restoredRoleEvent.m_bApplied
			&& restoredRoleTamper.m_Record.m_iContractVersion
				== HST_TownInfluenceSaveValidationService.QUARANTINE_CONTRACT_VERSION
			&& restoredRoleTamper.m_Zone.m_iCivilianConsequenceContractVersion
				== HST_CivilianConsequenceSaveValidationService.QUARANTINE_CONTRACT_VERSION;

		bool missingAuthorityExact = IsRejectedAggressionAdmissionInert(
				missingStrategic,
				missingStrategicResult,
				0)
			&& IsRejectedAggressionAdmissionInert(
				missingEconomy,
				missingEconomyResult,
				0);
		bool poolRoleBoundsExact = IsRejectedAggressionAdmissionInert(
				duplicatePool,
				duplicatePoolResult,
				0)
			&& IsRejectedAggressionAdmissionInert(
				nonEnemy,
				nonEnemyResult,
				0)
			&& IsRejectedAggressionAdmissionInert(
				negativePool,
				negativePoolResult,
				-1)
			&& IsRejectedAggressionAdmissionInert(
				overflow,
				overflowResult,
				int.MAX);
		bool sourceAndIdBoundsExact = IsRejectedAggressionAdmissionInert(
				collision,
				collisionResult,
				0)
			&& collision.m_State.m_aStrategicEvents.Count() == 1
			&& IsRejectedAggressionAdmissionInert(
				idExhaustion,
				idExhaustionResult,
				0);
		report.m_bAggressionAdmissionExact = missingAuthorityExact
			&& poolRoleBoundsExact && sourceAndIdBoundsExact
			&& restoredRoleQuarantined;
		report.m_sAggressionAdmissionEvidence = string.Format(
			"missing strategic/economy %1/%2 | duplicate/non-enemy %3/%4 | negative/overflow %5/%6 | source collision %7 | exact %8",
			missingStrategicResult && !missingStrategicResult.m_bAccepted,
			missingEconomyResult && !missingEconomyResult.m_bAccepted,
			duplicatePoolResult && !duplicatePoolResult.m_bAccepted,
			nonEnemyResult && !nonEnemyResult.m_bAccepted,
			negativePoolResult && !negativePoolResult.m_bAccepted,
			overflowResult && !overflowResult.m_bAccepted,
			collisionResult && !collisionResult.m_bAccepted,
			report.m_bAggressionAdmissionExact);
		report.m_sAggressionAdmissionEvidence
			= report.m_sAggressionAdmissionEvidence + string.Format(
				" | id exhaustion inert %1 | restored role quarantine %2",
				idExhaustionResult && !idExhaustionResult.m_bAccepted,
				restoredRoleQuarantined);
	}

	protected HST_TownInfluenceCommand BuildAggressionAdmissionCommand(
		string eventId,
		string targetFactionKey,
		int aggressionDelta)
	{
		HST_TownInfluenceCommand command = new HST_TownInfluenceCommand();
		command.m_sCommandId = eventId;
		command.m_sEventId = eventId;
		command.m_sTownId = TOWN_ID;
		command.m_sEventKind
			= HST_CivilianConsequenceService.EVENT_CIVILIAN_CASUALTY;
		command.m_sSourceId = eventId;
		command.m_sReason = "civilian aggression admission proof";
		command.m_iRawFIASupportDelta = -1;
		command.m_sAggressionFactionKey = targetFactionKey;
		command.m_iAggressionDelta = aggressionDelta;
		command.m_bPopulationScaled = true;
		command.m_bReconcileOwnership = false;
		return command;
	}

	protected bool IsRejectedAggressionAdmissionInert(
		HST_CivilianConsequenceProofFixture fixture,
		HST_TownInfluenceResult result,
		int expectedAggression)
	{
		return fixture && result && !result.m_bAccepted
			&& fixture.m_State.m_aTownInfluenceEvents.IsEmpty()
			&& fixture.m_Record.m_iRevision == 1
			&& fixture.m_OccupierPool.m_iAggression == expectedAggression;
	}

	protected HST_CivilianConsequenceProofFixture BuildFixture(
		bool canonicalTown,
		int population = 100)
	{
		HST_CivilianConsequenceProofFixture fixture
			= new HST_CivilianConsequenceProofFixture();
		fixture.m_State = new HST_CampaignState();
		fixture.m_State.m_iElapsedSeconds = 100;
		fixture.m_State.m_iPersistenceRestoreSequence = 0;
		fixture.m_Preset = new HST_CampaignPreset();
		fixture.m_Preset.m_sPresetId = "civilian_consequence_proof";
		fixture.m_Preset.m_sResistanceFactionKey = RESISTANCE_FACTION;
		fixture.m_Preset.m_sOccupierFactionKey = OCCUPIER_FACTION;
		fixture.m_Preset.m_sInvaderFactionKey = INVADER_FACTION;
		fixture.m_ResistancePool = BuildFactionPool(RESISTANCE_FACTION);
		fixture.m_OccupierPool = BuildFactionPool(OCCUPIER_FACTION);
		fixture.m_InvaderPool = BuildFactionPool(INVADER_FACTION);
		fixture.m_State.m_aFactionPools.Insert(fixture.m_ResistancePool);
		fixture.m_State.m_aFactionPools.Insert(fixture.m_OccupierPool);
		fixture.m_State.m_aFactionPools.Insert(fixture.m_InvaderPool);

		fixture.m_Zone = new HST_ZoneState();
		if (canonicalTown)
			fixture.m_Zone.m_sZoneId = TOWN_ID;
		else
			fixture.m_Zone.m_sZoneId = MINOR_ID;
		fixture.m_Zone.m_sDisplayName = fixture.m_Zone.m_sZoneId;
		fixture.m_Zone.m_sOwnerFactionKey = OCCUPIER_FACTION;
		fixture.m_Zone.m_iOwnershipContractVersion = 1;
		fixture.m_Zone.m_iOwnershipRevision = 1;
		if (canonicalTown)
			fixture.m_Zone.m_eType = HST_EZoneType.HST_ZONE_TOWN;
		else
			fixture.m_Zone.m_eType = HST_EZoneType.HST_ZONE_RESOURCE;
		fixture.m_State.m_aZones.Insert(fixture.m_Zone);

		if (canonicalTown)
		{
			fixture.m_Legacy = new HST_CivilianZoneState();
			fixture.m_Legacy.m_sZoneId = fixture.m_Zone.m_sZoneId;
			fixture.m_Legacy.m_iReputation = 50;
			fixture.m_Legacy.m_iFIASupport = 50;
			fixture.m_Legacy.m_iOccupierSupport = 50;
			fixture.m_Legacy.m_iCivilianPresence = 10;
			fixture.m_Legacy.m_iPopulationRemaining = Math.Max(1, population);
			fixture.m_State.m_aCivilianZones.Insert(fixture.m_Legacy);
		}

		fixture.m_Economy = new HST_EconomyService();
		fixture.m_Strategic = new HST_StrategicService();
		fixture.m_TownInfluence = new HST_TownInfluenceService();
		fixture.m_TownInfluence.SetCampaignPreset(fixture.m_Preset);
		fixture.m_TownInfluence.SetEconomyService(fixture.m_Economy);
		fixture.m_TownInfluence.SetStrategicService(fixture.m_Strategic);
		fixture.m_Strategic.SetTownInfluenceService(fixture.m_TownInfluence);
		if (canonicalTown)
		{
			fixture.m_TownInfluence.EnsureRecords(fixture.m_State);
			fixture.m_Record = fixture.m_TownInfluence.FindValidRecord(
				fixture.m_State,
				fixture.m_Zone.m_sZoneId);
		}
		fixture.m_Consequences = new HST_CivilianConsequenceService();
		fixture.m_Consequences.SetCampaignPreset(fixture.m_Preset);
		fixture.m_Consequences.SetTownInfluenceService(
			fixture.m_TownInfluence);
		return fixture;
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

	protected void PublishCombatFacts(
		HST_CivilianConsequenceProofFixture fixture,
		int revision,
		int currentOperationCount,
		int recentFireCount,
		string contributorHash)
	{
		if (!fixture || !fixture.m_Zone)
			return;
		fixture.m_Zone.m_iCombatPresenceRevision = revision;
		fixture.m_Zone.m_iCombatPresenceCurrentOperationCount
			= currentOperationCount;
		fixture.m_Zone.m_iCombatPresenceRecentFireCount = recentFireCount;
		fixture.m_Zone.m_sCombatPresenceContributorHash = contributorHash;
	}

	protected HST_StrategicEventState FindStrategicReceipt(
		HST_CampaignState state,
		string sourceId)
	{
		HST_StrategicEventState match;
		if (!state || sourceId.IsEmpty())
			return null;
		foreach (HST_StrategicEventState eventState : state.m_aStrategicEvents)
		{
			if (!eventState || eventState.m_sSourceType != "town_influence"
				|| eventState.m_sSourceId != sourceId)
				continue;
			if (match)
				return null;
			match = eventState;
		}
		return match;
	}
}
#endif
