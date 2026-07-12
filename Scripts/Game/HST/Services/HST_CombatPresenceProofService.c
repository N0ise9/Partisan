class HST_CombatPresenceProofReport
{
	bool m_bEmptyVehicleExcludedExact;
	bool m_bAuthoritativeSamplesExact;
	bool m_bRejectedRowsExact;
	bool m_bHeatLifecycleExact;
	bool m_bSchema62MigrationExact;
	bool m_bSchema63CoolingRestoreExact;
	bool m_bMalformedCurrentHeatExact;
	bool m_bContributorDeterminismExact;
	string m_sEmptyVehicleEvidence;
	string m_sAuthoritativeSampleEvidence;
	string m_sRejectedRowEvidence;
	string m_sHeatLifecycleEvidence;
	string m_sSchema62MigrationEvidence;
	string m_sSchema63CoolingRestoreEvidence;
	string m_sMalformedCurrentHeatEvidence;
	string m_sContributorDeterminismEvidence;

	bool AllExact()
	{
		if (!m_bEmptyVehicleExcludedExact)
			return false;
		if (!m_bAuthoritativeSamplesExact)
			return false;
		if (!m_bRejectedRowsExact)
			return false;
		if (!m_bHeatLifecycleExact)
			return false;
		if (!m_bSchema62MigrationExact)
			return false;
		if (!m_bSchema63CoolingRestoreExact)
			return false;
		if (!m_bMalformedCurrentHeatExact)
			return false;
		return m_bContributorDeterminismExact;
	}

	string BuildReport()
	{
		string report = string.Format(
			"combat presence proof | all exact %1 | empty vehicle %2 | authoritative %3 | rejected rows %4 | heat lifecycle %5",
			AllExact(),
			m_bEmptyVehicleExcludedExact,
			m_bAuthoritativeSamplesExact,
			m_bRejectedRowsExact,
			m_bHeatLifecycleExact);
		return report + string.Format(
			" | schema62 migration %1 | schema63 cooling restore %2 | malformed current %3 | deterministic diagnostics %4",
			m_bSchema62MigrationExact,
			m_bSchema63CoolingRestoreExact,
			m_bMalformedCurrentHeatExact,
			m_bContributorDeterminismExact);
	}
}

// Deterministic state-only proof for the canonical combat-presence predicate,
// its Hot -> Cooling -> Cold hysteresis, and its durable Schema-63 envelope.
class HST_CombatPresenceProofService
{
	static const string RESISTANCE_FACTION = "FIA";
	static const string HOSTILE_FACTION = "US";
	static const string PROOF_ZONE_ID = "combat_presence_proof_zone";
	static const int PROOF_RADIUS_METERS = 250;
	static const int COOLING_SECONDS = 30;

	HST_CombatPresenceProofReport Run()
	{
		HST_CombatPresenceProofReport report = new HST_CombatPresenceProofReport();
		ProveEmptyVehicleExcluded(report);
		ProveAuthoritativeSamples(report);
		ProveRejectedRows(report);
		ProveHeatLifecycle(report);
		ProveSchema62Migration(report);
		ProveSchema63CoolingRestore(report);
		ProveMalformedCurrentHeat(report);
		ProveContributorDeterminism(report);
		return report;
	}

	protected void ProveEmptyVehicleExcluded(HST_CombatPresenceProofReport report)
	{
		HST_CampaignState state = BuildState(100);
		HST_ActiveGroupState emptyVehicle = BuildPhysicalGroup(
			"empty_vehicle_survivor",
			100,
			0,
			0,
			0);
		emptyVehicle.m_iOriginalVehicleCount = 1;
		emptyVehicle.m_iVehicleCount = 1;
		emptyVehicle.m_iSurvivorVehicleCount = 1;
		emptyVehicle.m_bEverPopulated = true;
		state.m_aActiveGroups.Insert(emptyVehicle);

		HST_CombatPresenceService service = new HST_CombatPresenceService();
		HST_CombatPresenceResult result = service.QueryExactFactionPresenceNear(
			state,
			HOSTILE_FACTION,
			vector.Zero,
			PROOF_RADIUS_METERS);

		bool exact = result && result.m_bQueryValid;
		exact = exact && !result.m_bHasLiveContributors;
		exact = exact && !result.m_bBlocksProgress;
		exact = exact && result.GetLivingContributorCount() == 0;
		exact = exact && result.m_iContributorCount == 0;
		report.m_bEmptyVehicleExcludedExact = exact;
		report.m_sEmptyVehicleEvidence = string.Format(
			"survivor vehicles %1 | sampled manned vehicles %2 | living/contributors %3/%4 | blocks %5",
			emptyVehicle.m_iSurvivorVehicleCount,
			emptyVehicle.m_iOperationalMannedVehicleCount,
			result.GetLivingContributorCount(),
			result.m_iContributorCount,
			result.m_bBlocksProgress);
	}

	protected void ProveAuthoritativeSamples(HST_CombatPresenceProofReport report)
	{
		HST_CampaignState state = BuildState(200);
		state.m_aActiveGroups.Insert(BuildPhysicalGroup("sample_infantry", 200, 4, 0, 0));
		state.m_aActiveGroups.Insert(BuildPhysicalGroup("sample_vehicle", 200, 0, 2, 0));
		state.m_aActiveGroups.Insert(BuildPhysicalGroup("sample_static", 200, 0, 0, 3));

		HST_CombatPresenceService service = new HST_CombatPresenceService();
		HST_CombatPresenceResult result = service.QueryExactFactionPresenceNear(
			state,
			HOSTILE_FACTION,
			vector.Zero,
			PROOF_RADIUS_METERS);

		bool countsExact = result && result.m_iInfantryCount == 4;
		countsExact = countsExact && result.m_iMannedVehicleCount == 2;
		countsExact = countsExact && result.m_iStaticOperatorCount == 3;
		countsExact = countsExact && result.GetLivingContributorCount() == 9;
		bool contributorsExact = result && result.m_iContributorCount == 3;
		contributorsExact = contributorsExact && result.m_aContributors.Count() == 3;
		contributorsExact = contributorsExact && HasAuthoritativeContributor(result, "group:sample_infantry");
		contributorsExact = contributorsExact && HasAuthoritativeContributor(result, "group:sample_vehicle");
		contributorsExact = contributorsExact && HasAuthoritativeContributor(result, "group:sample_static");
		report.m_bAuthoritativeSamplesExact = countsExact && contributorsExact;
		report.m_sAuthoritativeSampleEvidence = string.Format(
			"infantry/vehicle/static %1/%2/%3 | living %4 | contributors %5",
			result.m_iInfantryCount,
			result.m_iMannedVehicleCount,
			result.m_iStaticOperatorCount,
			result.GetLivingContributorCount(),
			result.m_iContributorCount);
	}

	protected void ProveRejectedRows(HST_CombatPresenceProofReport report)
	{
		HST_CampaignState state = BuildState(300);
		HST_ActiveGroupState terminal = BuildPhysicalGroup("terminal_row", 300, 2, 1, 1);
		terminal.m_sRuntimeStatus = "eliminated";
		state.m_aActiveGroups.Insert(terminal);

		HST_ActiveGroupState quarantined = BuildPhysicalGroup("quarantined_row", 300, 2, 1, 1);
		quarantined.m_sRuntimeStatus = "exact_runtime_authority_quarantined";
		state.m_aActiveGroups.Insert(quarantined);

		HST_ActiveGroupState stale = BuildPhysicalGroup("stale_row", 297, 2, 1, 1);
		state.m_aActiveGroups.Insert(stale);

		HST_CombatPresenceService service = new HST_CombatPresenceService();
		HST_CombatPresenceResult result = service.QueryExactFactionPresenceNear(
			state,
			HOSTILE_FACTION,
			vector.Zero,
			PROOF_RADIUS_METERS);

		bool exact = result && !result.m_bQueryValid;
		exact = exact && !result.m_bHasLiveContributors;
		exact = exact && result.m_bBlocksProgress;
		exact = exact && result.GetLivingContributorCount() == 0;
		exact = exact && result.m_iContributorCount == 0;
		exact = exact && result.m_aContributors.Count() == 0;
		exact = exact && result.m_iUnresolvedAuthorityCount == 1;
		report.m_bRejectedRowsExact = exact;
		report.m_sRejectedRowEvidence = string.Format(
			"terminal/quarantined/stale inputs 1/1/1 | accepted contributors %1 | living %2 | unresolved %3 | blocks %4",
			result.m_iContributorCount,
			result.GetLivingContributorCount(),
			result.m_iUnresolvedAuthorityCount,
			result.m_bBlocksProgress);
	}

	protected void ProveHeatLifecycle(HST_CombatPresenceProofReport report)
	{
		HST_CampaignState state = BuildState(100);
		HST_ZoneState zone = BuildZone();
		state.m_aZones.Insert(zone);
		HST_ActiveGroupState source = BuildPhysicalGroup("heat_source", 100, 1, 1, 0);
		state.m_aActiveGroups.Insert(source);
		HST_CampaignPreset preset = BuildPreset();
		HST_BalanceConfig balance = BuildBalance();
		HST_CombatPresenceService service = new HST_CombatPresenceService();

		bool hotChanged = service.TickZoneHeat(state, preset, balance, zone, RESISTANCE_FACTION);
		bool hotExact = hotChanged;
		hotExact = hotExact && zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT;
		hotExact = hotExact && zone.m_iCombatPresenceInfantryCount == 1;
		hotExact = hotExact && zone.m_iCombatPresenceMannedVehicleCount == 1;
		hotExact = hotExact && !zone.m_sCombatPresenceContributorHash.IsEmpty();

		SetPhysicalSample(source, 101, 0, 0, 0);
		state.m_iElapsedSeconds = 101;
		HST_CombatPresenceResult hotTransitionGuard = service.QueryZoneHostilePresence(
			state,
			preset,
			RESISTANCE_FACTION,
			zone,
			true);
		bool hotTransitionGuardExact = hotTransitionGuard && hotTransitionGuard.m_bQueryValid;
		hotTransitionGuardExact = hotTransitionGuardExact && hotTransitionGuard.m_bBlocksProgress;
		hotTransitionGuardExact = hotTransitionGuardExact && !hotTransitionGuard.m_bHasLiveContributors;
		hotTransitionGuardExact = hotTransitionGuardExact
			&& hotTransitionGuard.m_eState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT;
		bool firstCoolingChanged = service.TickZoneHeat(state, preset, balance, zone, RESISTANCE_FACTION);
		bool firstCoolingExact = firstCoolingChanged;
		firstCoolingExact = firstCoolingExact && zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING;
		firstCoolingExact = firstCoolingExact && zone.m_iCombatPresenceCoolingUntilSecond == 131;
		firstCoolingExact = firstCoolingExact && zone.m_iCombatPresenceLastHotSecond == 101;
		firstCoolingExact = firstCoolingExact && !zone.m_sCombatPresenceContributorHash.IsEmpty();

		SetPhysicalSample(source, 110, 2, 0, 1);
		state.m_iElapsedSeconds = 110;
		bool reboundChanged = service.TickZoneHeat(state, preset, balance, zone, RESISTANCE_FACTION);
		bool reboundExact = reboundChanged;
		reboundExact = reboundExact && zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT;
		reboundExact = reboundExact && zone.m_iCombatPresenceCoolingUntilSecond == 0;
		reboundExact = reboundExact && zone.m_iCombatPresenceInfantryCount == 2;
		reboundExact = reboundExact && zone.m_iCombatPresenceStaticOperatorCount == 1;

		SetPhysicalSample(source, 111, 0, 0, 0);
		state.m_iElapsedSeconds = 111;
		bool secondCoolingChanged = service.TickZoneHeat(state, preset, balance, zone, RESISTANCE_FACTION);
		bool secondCoolingExact = secondCoolingChanged;
		secondCoolingExact = secondCoolingExact && zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING;
		secondCoolingExact = secondCoolingExact && zone.m_iCombatPresenceCoolingUntilSecond == 141;

		SetPhysicalSample(source, 140, 0, 0, 0);
		state.m_iElapsedSeconds = 140;
		bool prematureColdChanged = service.TickZoneHeat(state, preset, balance, zone, RESISTANCE_FACTION);
		bool deadlineHeldExact = !prematureColdChanged;
		deadlineHeldExact = deadlineHeldExact && zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING;
		deadlineHeldExact = deadlineHeldExact && zone.m_iCombatPresenceCoolingUntilSecond == 141;

		SetPhysicalSample(source, 141, 0, 0, 0);
		state.m_iElapsedSeconds = 141;
		bool coldChanged = service.TickZoneHeat(state, preset, balance, zone, RESISTANCE_FACTION);
		bool coldExact = coldChanged;
		coldExact = coldExact && zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COLD;
		coldExact = coldExact && zone.m_iCombatPresenceCoolingUntilSecond == 0;
		coldExact = coldExact && zone.m_sCombatPresenceContributorHash.IsEmpty();
		coldExact = coldExact && zone.m_aCombatPresenceContributorIds.Count() == 0;

		bool exact = hotExact && hotTransitionGuardExact && firstCoolingExact;
		exact = exact && reboundExact;
		exact = exact && secondCoolingExact;
		exact = exact && deadlineHeldExact;
		exact = exact && coldExact;
		report.m_bHeatLifecycleExact = exact;
		report.m_sHeatLifecycleEvidence = string.Format(
			"hot/transition guard %1/%2 | first deadline %3 | rebound %4 | second deadline %5 | held/cold %6/%7",
			hotExact,
			hotTransitionGuardExact,
			131,
			reboundExact,
			141,
			deadlineHeldExact,
			coldExact);
	}

	protected void ProveSchema62Migration(HST_CombatPresenceProofReport report)
	{
		HST_CampaignState legacyState = BuildState(400);
		legacyState.m_iSchemaVersion = 62;
		legacyState.m_iLastLoadedSchemaVersion = 62;
		HST_ZoneState legacyZone = BuildZone();
		legacyZone.m_eCombatPresenceState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT;
		legacyZone.m_iCombatPresenceLastHotSecond = 399;
		legacyZone.m_iCombatPresenceInfantryCount = 7;
		legacyZone.m_iCombatPresenceMannedVehicleCount = 2;
		legacyZone.m_sCombatPresenceContributorHash = "legacy_inferred_heat";
		legacyZone.m_sCombatPresenceReason = "legacy inferred heat";
		legacyZone.m_aCombatPresenceContributorIds.Insert("group:legacy_vehicle");
		legacyZone.m_aCombatPresenceContributorFacts.Insert("legacy survivor vehicle row");
		legacyState.m_aZones.Insert(legacyZone);
		HST_ActiveGroupState legacyVehicle = BuildVirtualGroup("legacy_vehicle");
		legacyVehicle.m_iDurableLivingInfantryCount = 0;
		legacyVehicle.m_iSurvivorInfantryCount = 0;
		legacyVehicle.m_iSurvivorVehicleCount = 5;
		legacyState.m_aActiveGroups.Insert(legacyVehicle);

		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(legacyState);
		HST_CampaignState restored = save.Restore();
		HST_ZoneState restoredZone = FindZone(restored, PROOF_ZONE_ID);

		bool exact = restored && restored.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION;
		exact = exact && restored.m_iLastLoadedSchemaVersion == 62;
		exact = exact && IsCanonicalCold(restoredZone);
		report.m_bSchema62MigrationExact = exact;
		report.m_sSchema62MigrationEvidence = string.Format(
			"loaded/current schema %1/%2 | state %3 | diagnostics %4/%5",
			restored.m_iLastLoadedSchemaVersion,
			restored.m_iSchemaVersion,
			ResolveZoneStateName(restoredZone),
			ResolveContributorIdCount(restoredZone),
			ResolveContributorFactCount(restoredZone));
	}

	protected void ProveSchema63CoolingRestore(HST_CombatPresenceProofReport report)
	{
		HST_CampaignState state = BuildBoundedState(false, 500);
		HST_ZoneState zone = BuildZone();
		state.m_aZones.Insert(zone);
		HST_CombatPresenceService service = new HST_CombatPresenceService();
		HST_CampaignPreset preset = BuildPreset();
		HST_BalanceConfig balance = BuildBalance();
		service.TickZoneHeat(state, preset, balance, zone, RESISTANCE_FACTION);
		state.m_aActiveGroups.Clear();
		state.m_iElapsedSeconds = 501;
		service.TickZoneHeat(state, preset, balance, zone, RESISTANCE_FACTION);

		int expectedDeadline = zone.m_iCombatPresenceCoolingUntilSecond;
		string expectedHash = zone.m_sCombatPresenceContributorHash;
		array<string> expectedIds = CopyStrings(zone.m_aCombatPresenceContributorIds);
		array<string> expectedFacts = CopyStrings(zone.m_aCombatPresenceContributorFacts);

		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(state);
		HST_CampaignState restored = save.Restore();
		HST_ZoneState restoredZone = FindZone(restored, PROOF_ZONE_ID);

		bool shapeExact = restoredZone;
		shapeExact = shapeExact && restoredZone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING;
		shapeExact = shapeExact && restoredZone.m_iCombatPresenceCoolingUntilSecond == expectedDeadline;
		shapeExact = shapeExact && expectedDeadline == 531;
		shapeExact = shapeExact && restoredZone.m_sCombatPresenceContributorHash == expectedHash;
		bool diagnosticsExact = restoredZone;
		diagnosticsExact = diagnosticsExact
			&& restoredZone.m_aCombatPresenceContributorIds.Count()
			== HST_CombatPresenceService.MAX_PERSISTED_CONTRIBUTORS;
		diagnosticsExact = diagnosticsExact
			&& restoredZone.m_aCombatPresenceContributorFacts.Count()
			== HST_CombatPresenceService.MAX_PERSISTED_CONTRIBUTORS;
		diagnosticsExact = diagnosticsExact && StringArraysEqual(expectedIds, restoredZone.m_aCombatPresenceContributorIds);
		diagnosticsExact = diagnosticsExact && StringArraysEqual(expectedFacts, restoredZone.m_aCombatPresenceContributorFacts);
		diagnosticsExact = diagnosticsExact && FactsBounded(restoredZone.m_aCombatPresenceContributorFacts);
		report.m_bSchema63CoolingRestoreExact = shapeExact && diagnosticsExact;
		report.m_sSchema63CoolingRestoreEvidence = string.Format(
			"deadline expected/restored %1/%2 | ids/facts %3/%4 | bounded %5 | hash stable %6",
			expectedDeadline,
			ResolveCoolingDeadline(restoredZone),
			ResolveContributorIdCount(restoredZone),
			ResolveContributorFactCount(restoredZone),
			FactsBounded(ResolveContributorFacts(restoredZone)),
			restoredZone && restoredZone.m_sCombatPresenceContributorHash == expectedHash);
	}

	protected void ProveMalformedCurrentHeat(HST_CombatPresenceProofReport report)
	{
		HST_CampaignState state = BuildState(600);
		HST_ZoneState zone = BuildZone();
		zone.m_eCombatPresenceState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT;
		zone.m_iCombatPresenceLastHotSecond = 600;
		zone.m_iCombatPresenceCoolingUntilSecond = 700;
		zone.m_iCombatPresenceInfantryCount = 1;
		zone.m_sCombatPresenceContributorHash = "malformed_hash";
		zone.m_sCombatPresenceReason = "malformed current heat";
		zone.m_aCombatPresenceContributorIds.Insert("group:malformed");
		state.m_aZones.Insert(zone);
		HST_ZoneState unboundedCooling = BuildZone();
		unboundedCooling.m_sZoneId = "proof_unbounded_cooling";
		unboundedCooling.m_eCombatPresenceState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING;
		unboundedCooling.m_iCombatPresenceLastHotSecond = 600;
		unboundedCooling.m_iCombatPresenceCoolingUntilSecond = 1000;
		unboundedCooling.m_sCombatPresenceReason = "combat area cooling";
		unboundedCooling.m_sCombatPresenceContributorHash = "cp1_0_0";
		unboundedCooling.m_aCombatPresenceContributorIds.Insert("group:unbounded");
		unboundedCooling.m_aCombatPresenceContributorFacts.Insert("unbounded cooling proof");
		state.m_aZones.Insert(unboundedCooling);
		HST_ActiveGroupState sampledGroup = BuildPhysicalGroup(
			"malformed_restore_sample",
			600,
			2,
			1,
			1);
		state.m_aActiveGroups.Insert(sampledGroup);

		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.Capture(state);
		HST_CampaignState restored = save.Restore();
		HST_ZoneState restoredZone = FindZone(restored, PROOF_ZONE_ID);
		HST_ZoneState restoredUnboundedCooling = FindZone(restored, "proof_unbounded_cooling");
		HST_ActiveGroupState restoredSampledGroup;
		if (restored)
			restoredSampledGroup = restored.FindActiveGroup("malformed_restore_sample");
		bool sampleInvalidated = restoredSampledGroup;
		sampleInvalidated = sampleInvalidated
			&& restoredSampledGroup.m_iCombatEffectiveInfantryCount == 0;
		sampleInvalidated = sampleInvalidated
			&& restoredSampledGroup.m_iOperationalMannedVehicleCount == 0;
		sampleInvalidated = sampleInvalidated
			&& restoredSampledGroup.m_iCombatEffectiveStaticOperatorCount == 0;
		sampleInvalidated = sampleInvalidated
			&& restoredSampledGroup.m_iCombatPresenceSampleSecond == -1;
		sampleInvalidated = sampleInvalidated
			&& !restoredSampledGroup.m_bCombatPresenceSampleAuthoritative;
		report.m_bMalformedCurrentHeatExact = IsCanonicalCold(restoredZone)
			&& IsCanonicalCold(restoredUnboundedCooling)
			&& sampleInvalidated;
		report.m_sMalformedCurrentHeatEvidence = string.Format(
			"state %1 | deadline %2 | counts %3/%4/%5 | diagnostics %6/%7 | unbounded/sample %8/%9",
			ResolveZoneStateName(restoredZone),
			ResolveCoolingDeadline(restoredZone),
			ResolveInfantryCount(restoredZone),
			ResolveMannedVehicleCount(restoredZone),
			ResolveStaticOperatorCount(restoredZone),
			ResolveContributorIdCount(restoredZone),
			ResolveContributorFactCount(restoredZone),
			IsCanonicalCold(restoredUnboundedCooling),
			sampleInvalidated);
	}

	protected void ProveContributorDeterminism(HST_CombatPresenceProofReport report)
	{
		HST_CampaignState forwardState = BuildBoundedState(false, 700);
		HST_CampaignState reverseState = BuildBoundedState(true, 700);
		HST_CombatPresenceService service = new HST_CombatPresenceService();
		HST_CombatPresenceResult forward = service.QueryExactFactionPresenceNear(
			forwardState,
			HOSTILE_FACTION,
			vector.Zero,
			PROOF_RADIUS_METERS);
		HST_CombatPresenceResult reverse = service.QueryExactFactionPresenceNear(
			reverseState,
			HOSTILE_FACTION,
			vector.Zero,
			PROOF_RADIUS_METERS);

		bool capExact = forward && forward.m_iContributorCount == 26;
		capExact = capExact && forward.m_aContributors.Count() == HST_CombatPresenceService.MAX_PERSISTED_CONTRIBUTORS;
		capExact = capExact && forward.m_iOmittedContributorCount == 2;
		capExact = capExact && forward.m_aContributors[0].m_sContributorId == "group:alpha";
		capExact = capExact && forward.m_aContributors[23].m_sContributorId == "group:xray";
		bool deterministic = forward && reverse;
		deterministic = deterministic && forward.m_sContributorHash == reverse.m_sContributorHash;
		deterministic = deterministic && ContributorProjectionsEqual(forward, reverse);
		deterministic = deterministic && !forward.m_sContributorHash.IsEmpty();
		report.m_bContributorDeterminismExact = capExact && deterministic;
		report.m_sContributorDeterminismEvidence = string.Format(
			"contributors/persisted/omitted %1/%2/%3 | first/last %4/%5 | hash equal %6",
			ResolveContributorCount(forward),
			ResolvePersistedContributorCount(forward),
			ResolveOmittedContributorCount(forward),
			ResolveContributorId(forward, 0),
			ResolveContributorId(forward, 23),
			forward && reverse && forward.m_sContributorHash == reverse.m_sContributorHash);
	}

	protected HST_CampaignState BuildState(int nowSecond)
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE;
		state.m_iElapsedSeconds = nowSecond;
		state.m_sPresetId = "combat_presence_proof";
		return state;
	}

	protected HST_CampaignState BuildBoundedState(bool reverseOrder, int nowSecond)
	{
		HST_CampaignState state = BuildState(nowSecond);
		array<string> names = BuildContributorNames();
		if (reverseOrder)
		{
			for (int reverseIndex = names.Count() - 1; reverseIndex >= 0; reverseIndex--)
				state.m_aActiveGroups.Insert(BuildVirtualGroup(names[reverseIndex]));
		}
		else
		{
			foreach (string name : names)
				state.m_aActiveGroups.Insert(BuildVirtualGroup(name));
		}
		return state;
	}

	protected HST_ZoneState BuildZone()
	{
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = PROOF_ZONE_ID;
		zone.m_sDisplayName = "Combat Presence Proof";
		zone.m_sOwnerFactionKey = HOSTILE_FACTION;
		zone.m_eType = HST_EZoneType.HST_ZONE_OUTPOST;
		zone.m_vPosition = vector.Zero;
		zone.m_iCaptureRadiusMeters = PROOF_RADIUS_METERS;
		return zone;
	}

	protected HST_CampaignPreset BuildPreset()
	{
		HST_CampaignPreset preset = new HST_CampaignPreset();
		preset.m_sPresetId = "combat_presence_proof";
		preset.m_sResistanceFactionKey = RESISTANCE_FACTION;
		preset.m_sOccupierFactionKey = HOSTILE_FACTION;
		preset.m_sInvaderFactionKey = "USSR";
		return preset;
	}

	protected HST_BalanceConfig BuildBalance()
	{
		HST_BalanceConfig balance = new HST_BalanceConfig();
		balance.m_iCombatPresenceCoolingSeconds = COOLING_SECONDS;
		return balance;
	}

	protected HST_ActiveGroupState BuildPhysicalGroup(
		string groupId,
		int sampleSecond,
		int infantryCount,
		int mannedVehicleCount,
		int staticOperatorCount)
	{
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = groupId;
		group.m_sFactionKey = HOSTILE_FACTION;
		group.m_sRuntimeStatus = "active";
		group.m_vPosition = vector.Zero;
		group.m_bSpawnAttempted = true;
		group.m_bSpawnedEntity = true;
		group.m_bEverPopulated = true;
		group.m_bSpawnCompleted = true;
		SetPhysicalSample(group, sampleSecond, infantryCount, mannedVehicleCount, staticOperatorCount);
		return group;
	}

	protected void SetPhysicalSample(
		HST_ActiveGroupState group,
		int sampleSecond,
		int infantryCount,
		int mannedVehicleCount,
		int staticOperatorCount)
	{
		if (!group)
			return;
		group.m_bCombatPresenceSampleAuthoritative = true;
		group.m_iCombatPresenceSampleSecond = sampleSecond;
		group.m_iCombatEffectiveInfantryCount = Math.Max(0, infantryCount);
		group.m_iOperationalMannedVehicleCount = Math.Max(0, mannedVehicleCount);
		group.m_iCombatEffectiveStaticOperatorCount = Math.Max(0, staticOperatorCount);
		group.m_sCombatPresenceSampleReason = "deterministic proof sample";
	}

	protected HST_ActiveGroupState BuildVirtualGroup(string groupId)
	{
		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = groupId;
		group.m_sFactionKey = HOSTILE_FACTION;
		group.m_sRuntimeStatus = "virtual";
		group.m_vPosition = vector.Zero;
		group.m_iOriginalInfantryCount = 1;
		group.m_iInfantryCount = 1;
		group.m_iDurableLivingInfantryCount = 1;
		group.m_iSurvivorInfantryCount = 1;
		return group;
	}

	protected array<string> BuildContributorNames()
	{
		array<string> names = {};
		names.Insert("alpha");
		names.Insert("bravo");
		names.Insert("charlie");
		names.Insert("delta");
		names.Insert("echo");
		names.Insert("foxtrot");
		names.Insert("golf");
		names.Insert("hotel");
		names.Insert("india");
		names.Insert("juliet");
		names.Insert("kilo");
		names.Insert("lima");
		names.Insert("mike");
		names.Insert("november");
		names.Insert("oscar");
		names.Insert("papa");
		names.Insert("quebec");
		names.Insert("romeo");
		names.Insert("sierra");
		names.Insert("tango");
		names.Insert("uniform");
		names.Insert("victor");
		names.Insert("whiskey");
		names.Insert("xray");
		names.Insert("yankee");
		names.Insert("zulu");
		return names;
	}

	protected bool HasAuthoritativeContributor(HST_CombatPresenceResult result, string contributorId)
	{
		if (!result || contributorId.IsEmpty())
			return false;
		foreach (HST_CombatPresenceContribution contribution : result.m_aContributors)
		{
			if (contribution && contribution.m_sContributorId == contributorId)
				return contribution.m_bAuthoritativePhysicalSample;
		}
		return false;
	}

	protected bool ContributorProjectionsEqual(
		HST_CombatPresenceResult left,
		HST_CombatPresenceResult right)
	{
		if (!left || !right)
			return false;
		if (left.m_aContributors.Count() != right.m_aContributors.Count())
			return false;
		for (int index; index < left.m_aContributors.Count(); index++)
		{
			HST_CombatPresenceContribution leftContribution = left.m_aContributors[index];
			HST_CombatPresenceContribution rightContribution = right.m_aContributors[index];
			if (!leftContribution || !rightContribution)
				return false;
			if (leftContribution.m_sContributorId != rightContribution.m_sContributorId)
				return false;
			if (leftContribution.m_sFact != rightContribution.m_sFact)
				return false;
		}
		return true;
	}

	protected bool IsCanonicalCold(HST_ZoneState zone)
	{
		if (!zone)
			return false;
		if (zone.m_eCombatPresenceState != HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COLD)
			return false;
		if (zone.m_iCombatPresenceLastHotSecond != 0 || zone.m_iCombatPresenceCoolingUntilSecond != 0)
			return false;
		if (zone.m_iCombatPresenceInfantryCount != 0 || zone.m_iCombatPresenceMannedVehicleCount != 0)
			return false;
		if (zone.m_iCombatPresenceStaticOperatorCount != 0)
			return false;
		if (zone.m_iCombatPresenceCurrentOperationCount != 0 || zone.m_iCombatPresenceRecentFireCount != 0)
			return false;
		if (!zone.m_sCombatPresenceContributorHash.IsEmpty())
			return false;
		if (!zone.m_aCombatPresenceContributorIds || !zone.m_aCombatPresenceContributorFacts)
			return false;
		return zone.m_aCombatPresenceContributorIds.Count() == 0
			&& zone.m_aCombatPresenceContributorFacts.Count() == 0;
	}

	protected bool FactsBounded(array<string> facts)
	{
		if (!facts || facts.Count() > HST_CombatPresenceService.MAX_PERSISTED_CONTRIBUTORS)
			return false;
		foreach (string fact : facts)
		{
			if (fact.Length() > HST_CombatPresenceService.MAX_DIAGNOSTIC_FACT_CHARACTERS)
				return false;
		}
		return true;
	}

	protected array<string> CopyStrings(array<string> source)
	{
		array<string> copy = {};
		if (!source)
			return copy;
		foreach (string value : source)
			copy.Insert(value);
		return copy;
	}

	protected bool StringArraysEqual(array<string> left, array<string> right)
	{
		if (!left || !right)
			return false;
		if (left.Count() != right.Count())
			return false;
		for (int index; index < left.Count(); index++)
		{
			if (left[index] != right[index])
				return false;
		}
		return true;
	}

	protected HST_ZoneState FindZone(HST_CampaignState state, string zoneId)
	{
		if (!state)
			return null;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_sZoneId == zoneId)
				return zone;
		}
		return null;
	}

	protected string ResolveZoneStateName(HST_ZoneState zone)
	{
		if (!zone)
			return "missing";
		return HST_CombatPresenceService.StateName(zone.m_eCombatPresenceState);
	}

	protected int ResolveCoolingDeadline(HST_ZoneState zone)
	{
		if (!zone)
			return -1;
		return zone.m_iCombatPresenceCoolingUntilSecond;
	}

	protected int ResolveInfantryCount(HST_ZoneState zone)
	{
		if (!zone)
			return -1;
		return zone.m_iCombatPresenceInfantryCount;
	}

	protected int ResolveMannedVehicleCount(HST_ZoneState zone)
	{
		if (!zone)
			return -1;
		return zone.m_iCombatPresenceMannedVehicleCount;
	}

	protected int ResolveStaticOperatorCount(HST_ZoneState zone)
	{
		if (!zone)
			return -1;
		return zone.m_iCombatPresenceStaticOperatorCount;
	}

	protected int ResolveContributorIdCount(HST_ZoneState zone)
	{
		if (!zone || !zone.m_aCombatPresenceContributorIds)
			return -1;
		return zone.m_aCombatPresenceContributorIds.Count();
	}

	protected int ResolveContributorFactCount(HST_ZoneState zone)
	{
		if (!zone || !zone.m_aCombatPresenceContributorFacts)
			return -1;
		return zone.m_aCombatPresenceContributorFacts.Count();
	}

	protected array<string> ResolveContributorFacts(HST_ZoneState zone)
	{
		if (!zone)
			return null;
		return zone.m_aCombatPresenceContributorFacts;
	}

	protected int ResolveContributorCount(HST_CombatPresenceResult result)
	{
		if (!result)
			return -1;
		return result.m_iContributorCount;
	}

	protected int ResolvePersistedContributorCount(HST_CombatPresenceResult result)
	{
		if (!result || !result.m_aContributors)
			return -1;
		return result.m_aContributors.Count();
	}

	protected int ResolveOmittedContributorCount(HST_CombatPresenceResult result)
	{
		if (!result)
			return -1;
		return result.m_iOmittedContributorCount;
	}

	protected string ResolveContributorId(HST_CombatPresenceResult result, int index)
	{
		if (!result)
			return "missing";
		if (index < 0 || index >= result.m_aContributors.Count())
			return "missing";
		HST_CombatPresenceContribution contribution = result.m_aContributors[index];
		if (!contribution)
			return "missing";
		return contribution.m_sContributorId;
	}
}
