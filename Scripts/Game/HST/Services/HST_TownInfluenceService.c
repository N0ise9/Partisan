class HST_TownInfluenceCommand
{
	string m_sCommandId;
	string m_sEventId;
	string m_sTownId;
	string m_sEventKind;
	string m_sSourceId;
	string m_sReason;
	int m_iRawFIASupportDelta;
	int m_iRawOccupierSupportDelta;
	int m_iRawInvaderSupportDelta;
	int m_iReputationDelta;
	int m_iHeatDelta;
	int m_iPopulationDelta;
	int m_iPoliceDelta;
	int m_iRoadblockDelta;
	string m_sAggressionFactionKey;
	int m_iAggressionDelta;
	int m_iDurationSeconds;
	bool m_bPopulationScaled;
	bool m_bMarkContacted;
	bool m_bMarkResistanceActivity;
	bool m_bReconcileOwnership = true;
#ifdef ENABLE_DIAG
	bool m_bAbsoluteDebugSeed;
	int m_iTargetFIASupportBasisPoints;
	int m_iTargetOccupierSupportBasisPoints;
	int m_iTargetInvaderSupportBasisPoints;
	int m_iTargetInitialPopulation;
	int m_iTargetRemainingPopulation;
	int m_iTargetDestroyedPopulation;
#endif
}

class HST_TownInfluenceResult
{
	bool m_bAccepted;
	bool m_bAlreadyApplied;
	bool m_bChanged;
	bool m_bOwnershipPending;
	string m_sFailureReason;
	ref HST_TownInfluenceEventState m_Event;
	ref HST_TownInfluenceRecord m_Record;

	string BuildReport()
	{
		string townId = "none";
		string eventId = "none";
		int revision;
		if (m_Record)
		{
			townId = m_Record.m_sTownId;
			revision = m_Record.m_iRevision;
		}
		if (m_Event)
			eventId = m_Event.m_sEventId;
		return string.Format(
			"Partisan town influence | accepted %1 | replay %2 | changed %3 | ownership pending %4 | town %5 | event %6 | revision %7 | %8",
			m_bAccepted,
			m_bAlreadyApplied,
			m_bChanged,
			m_bOwnershipPending,
			townId,
			eventId,
			revision,
			m_sFailureReason);
	}
}

class HST_TownPopulationAggregate
{
	bool m_bAuthorityValid;
	int m_iTownCount;
	int m_iInitialPopulation;
	int m_iRemainingPopulation;
	int m_iDestroyedPopulation;
	int m_iFIASupportPopulation;
	string m_sFailureReason;
}

// Schema-64 canonical authority for political support, town population, and
// threshold-driven ownership intent. Compatibility fields are projections only.
class HST_TownInfluenceService
{
	static const int SCHEMA_VERSION = 64;
	static const int EXACT_CONTRACT_VERSION = 1;
	static const int MIN_BASIS_POINTS = 0;
	static const int MAX_BASIS_POINTS = 10000;
	static const int RESISTANCE_OWNERSHIP_THRESHOLD_BASIS_POINTS = 8000;
	static const int ENEMY_OWNERSHIP_THRESHOLD_BASIS_POINTS = 4000;
	static const int OWNERSHIP_POLICY_RECONCILE_SECONDS = 5;
	static const int MAX_TOWN_RECORDS = 512;
	static const int MAX_INFLUENCE_EVENTS = 32768;
	static const int MAX_ID_CHARACTERS = 160;
	static const int MAX_REASON_CHARACTERS = 192;
	static const int MAX_RAW_SUPPORT_DELTA = 10000;
	static const int MAX_POPULATION_DELTA = 1000000;
	static const int MAX_TOWN_POPULATION = 1000000;
	static const int MAX_NON_SUPPORT_DELTA = 1000000;
	static const int MAX_AGGRESSION_DELTA = 1000;
	static const int MAX_LEGACY_PRESSURE = 1000000;
	static const int MAX_MUTABLE_REVISION = int.MAX - 16;
#ifdef ENABLE_DIAG
	static const int DEBUG_SEED_REVISION_HEADROOM = 4;
#endif
	static const int MAX_DURATION_SECONDS = 31536000;
	static const string HYSTERESIS_RESISTANCE = "resistance";
	static const string HYSTERESIS_NEUTRAL = "neutral";
	static const string HYSTERESIS_ENEMY = "enemy";
	static const string FORMULA_REFERENCE_COMMIT = "6e4226d3863ca8673535386c2fff8b6e08a806c4";

	protected HST_CampaignPreset m_Preset;
	protected HST_StrategicService m_Strategic;
	protected HST_EconomyService m_Economy;
	protected HST_OwnershipTransitionService m_OwnershipTransitions;
	protected int m_iNextOwnershipPolicyReconcileSecond;
	protected string m_sLastAuthorityFailure;
	protected ref map<string, ref HST_TownInfluenceRecord> m_mDueExpiryRecords
		= new map<string, ref HST_TownInfluenceRecord>();
	protected ref map<string, int> m_mExpiryTownIdCounts
		= new map<string, int>();
	protected HST_CampaignState m_Pre64NormalizedState;
	protected int m_iPre64NormalizedRestoreSequence = -1;
	protected ref map<string, int> m_mPre64TownIdCounts
		= new map<string, int>();

	void SetCampaignPreset(HST_CampaignPreset preset)
	{
		m_Preset = preset;
	}

	void SetStrategicService(HST_StrategicService strategic)
	{
		m_Strategic = strategic;
	}

	void SetEconomyService(HST_EconomyService economy)
	{
		m_Economy = economy;
	}

	void SetOwnershipTransitionService(HST_OwnershipTransitionService ownershipTransitions)
	{
		m_OwnershipTransitions = ownershipTransitions;
	}

	// Save-shape validation can prove pool uniqueness and receipt arithmetic but
	// the preset faction roles are intentionally not serialized. Finish restore
	// under the live preset before any political reconciliation can consume an
	// aggressive row.
	bool ValidateRestoredAggressionFactionRoles(
		HST_CampaignState state,
		HST_CampaignPreset preset)
	{
		if (!state)
			return false;
		bool changed;
		foreach (HST_TownInfluenceEventState eventState : state.m_aTownInfluenceEvents)
		{
			if (!eventState || !eventState.m_bApplied
				|| eventState.m_iContractVersion != EXACT_CONTRACT_VERSION
				|| eventState.m_iAggressionDelta <= 0)
				continue;
			if (preset && HST_FactionRelationService.IsEnemyFaction(
				preset,
				eventState.m_sAggressionFactionKey))
				continue;
			eventState.m_iContractVersion
				= HST_TownInfluenceSaveValidationService.QUARANTINE_CONTRACT_VERSION;
			eventState.m_bApplied = false;
			foreach (HST_TownInfluenceRecord record : state.m_aTownInfluenceRecords)
			{
				if (!record || record.m_sTownId != eventState.m_sZoneId)
					continue;
				record.m_iContractVersion
					= HST_TownInfluenceSaveValidationService.QUARANTINE_CONTRACT_VERSION;
				record.m_sAuthorityFailure = LimitText(
					"schema65 aggression target is not a configured enemy faction",
					MAX_REASON_CHARACTERS);
			}
			HST_ZoneState zone = state.FindZone(eventState.m_sZoneId);
			if (zone)
			{
				zone.m_iCivilianConsequenceContractVersion
					= HST_CivilianConsequenceSaveValidationService.QUARANTINE_CONTRACT_VERSION;
				zone.m_bCivilianCombatDangerActive = false;
				zone.m_iCivilianPanicUntilSecond = 0;
				zone.m_sCivilianConsequenceAuthorityFailure = LimitText(
					"schema65 aggression target is not a configured enemy faction",
					MAX_REASON_CHARACTERS);
			}
			changed = true;
		}
		return changed;
	}

	string GetLastAuthorityFailure()
	{
		return m_sLastAuthorityFailure;
	}

	static int CalculateRequestedSupportDeltaBasisPoints(int requestedRawDelta)
	{
		return Math.Round(requestedRawDelta * 100.0);
	}

	static string BuildOwnershipRewardEventId(
		string townId,
		string ownershipRequestId)
	{
		if (townId.IsEmpty() || ownershipRequestId.IsEmpty())
			return "";
		return string.Format(
			"town_ownership_%1_%2",
			townId.Hash(),
			ownershipRequestId.Hash());
	}

	static string BuildPoliticalOwnershipRequestId(
		string townId,
		int expectedOwnershipRevision,
		string sourceId)
	{
		if (townId.IsEmpty() || sourceId.IsEmpty()
			|| expectedOwnershipRevision <= 0)
			return "";
		return string.Format(
			"ownership_political_%1_%2_%3",
			townId.Hash(),
			expectedOwnershipRevision,
			sourceId.Hash());
	}

	static int CalculateEffectiveSupportDeltaBasisPoints(
		int requestedRawDelta,
		int initialPopulation,
		bool populationScaled)
	{
		if (!populationScaled)
			return CalculateRequestedSupportDeltaBasisPoints(requestedRawDelta);
		if (initialPopulation <= 0)
			return 0;
		return Math.Round(1000.0 * requestedRawDelta / Math.Sqrt(initialPopulation));
	}

	static string ResolveHysteresisBand(int fiaSupportBasisPoints)
	{
		if (fiaSupportBasisPoints > RESISTANCE_OWNERSHIP_THRESHOLD_BASIS_POINTS)
			return HYSTERESIS_RESISTANCE;
		if (fiaSupportBasisPoints < ENEMY_OWNERSHIP_THRESHOLD_BASIS_POINTS)
			return HYSTERESIS_ENEMY;
		return HYSTERESIS_NEUTRAL;
	}

	static bool QualifiesResistanceOwnership(int fiaSupportBasisPoints)
	{
		return fiaSupportBasisPoints > RESISTANCE_OWNERSHIP_THRESHOLD_BASIS_POINTS;
	}

	static bool QualifiesEnemyOwnership(int fiaSupportBasisPoints)
	{
		return fiaSupportBasisPoints < ENEMY_OWNERSHIP_THRESHOLD_BASIS_POINTS;
	}

	static bool MeetsPopulationSupportThreshold(
		int fiaSupportPopulation,
		int remainingPopulation,
		int requiredPercent)
	{
		if (fiaSupportPopulation < 0 || remainingPopulation <= 0)
			return false;
		int boundedPercent = Math.Max(0, Math.Min(100, requiredPercent));
		int requiredPopulation = (remainingPopulation / 100) * boundedPercent;
		int remainder = remainingPopulation % 100;
		requiredPopulation += (remainder * boundedPercent + 99) / 100;
		return fiaSupportPopulation >= requiredPopulation;
	}

	bool EnsureRecords(HST_CampaignState state)
	{
		if (!state || state.m_iPersistenceRestoreSequence != 0)
			return false;

		bool changed;
		int visited;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN
				|| zone.m_sZoneId.IsEmpty())
				continue;
			if (FindUniqueCanonicalTownZone(state, zone.m_sZoneId) != zone)
				continue;
			visited++;
			if (visited > MAX_TOWN_RECORDS)
				break;

			HST_TownInfluenceRecord existing = FindValidRecord(state, zone.m_sZoneId);
			if (existing)
			{
				ProjectLegacyFields(state, existing);
				continue;
			}
			// Never conceal duplicate or malformed authority by inserting a second row.
			if (HasAnyRecordForTown(state, zone.m_sZoneId))
				continue;
			if (state.m_aTownInfluenceRecords.Count() >= MAX_TOWN_RECORDS)
				break;

			HST_TownInfluenceRecord record = BuildInitialRecord(state, zone);
			if (!record)
				continue;
			state.m_aTownInfluenceRecords.Insert(record);
			ProjectLegacyFields(state, record);
			changed = true;
		}
		return changed;
	}

	HST_TownInfluenceRecord FindValidRecord(HST_CampaignState state, string townId)
	{
		if (!state || townId.IsEmpty())
			return null;
		HST_ZoneState zone = FindUniqueCanonicalTownZone(state, townId);
		if (!zone)
			return null;

		HST_TownInfluenceRecord match;
		foreach (HST_TownInfluenceRecord record : state.m_aTownInfluenceRecords)
		{
			if (!record || record.m_sTownId != zone.m_sZoneId)
				continue;
			if (match)
				return null;
			match = record;
		}
		if (!IsRecordContractValid(state, match, state.m_iElapsedSeconds))
			return null;
		return match;
	}

	int ResolveFIASupportPercent(HST_CampaignState state, string townId)
	{
		HST_TownInfluenceRecord record = FindValidRecord(state, townId);
		if (!record)
			return 0;
		return BasisPointsToPercent(record.m_iFIASupportBasisPoints);
	}

	int ResolveSignedSupportPercent(HST_CampaignState state, string townId)
	{
		HST_TownInfluenceRecord record = FindValidRecord(state, townId);
		if (!record)
			return 0;
		int enemySupport = Math.Max(
			record.m_iOccupierSupportBasisPoints,
			record.m_iInvaderSupportBasisPoints);
		return ClampSignedPercent(Math.Round(
			(record.m_iFIASupportBasisPoints - enemySupport) / 100.0));
	}

	HST_TownPopulationAggregate BuildPopulationAggregate(
		HST_CampaignState state)
	{
		HST_TownPopulationAggregate aggregate
			= new HST_TownPopulationAggregate();
		if (!state)
		{
			aggregate.m_sFailureReason = "campaign state is unavailable";
			return aggregate;
		}

		map<string, bool> visitedTownIds = new map<string, bool>();
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
				continue;
			if (zone.m_sZoneId.IsEmpty()
				|| visitedTownIds.Contains(zone.m_sZoneId))
			{
				aggregate.m_sFailureReason
					= "town population zone identity is ambiguous";
				return aggregate;
			}
			visitedTownIds.Set(zone.m_sZoneId, true);
			if (visitedTownIds.Count() > MAX_TOWN_RECORDS)
			{
				aggregate.m_sFailureReason
					= "town population authority exceeds the bounded town count";
				return aggregate;
			}

			HST_TownInfluenceRecord record = FindValidRecord(
				state,
				zone.m_sZoneId);
			if (!record)
			{
				aggregate.m_sFailureReason
					= "canonical town population authority is unavailable";
				return aggregate;
			}
			aggregate.m_iTownCount++;
			aggregate.m_iInitialPopulation += record.m_iInitialPopulation;
			aggregate.m_iRemainingPopulation += record.m_iRemainingPopulation;
			aggregate.m_iDestroyedPopulation += record.m_iDestroyedPopulation;
			aggregate.m_iFIASupportPopulation += Math.Round(
				record.m_iRemainingPopulation
					* (record.m_iFIASupportBasisPoints / 10000.0));
		}
		aggregate.m_bAuthorityValid = true;
		return aggregate;
	}

	bool MarkContacted(
		HST_CampaignState state,
		string townId,
		string sourceId = "",
		string reason = "town contacted",
		bool resistanceActivityStarted = false)
	{
		HST_TownInfluenceRecord record = FindValidRecord(state, townId);
		if (!record)
			return false;
		if (record.m_iRevision >= MAX_MUTABLE_REVISION)
			return false;
		if (sourceId.IsEmpty())
			sourceId = record.m_sTownId;
		sourceId = LimitText(sourceId.Trim(), MAX_ID_CHARACTERS);
		reason = LimitText(reason.Trim(), MAX_REASON_CHARACTERS);
		if (sourceId.IsEmpty() || reason.IsEmpty())
			return false;

		bool changed;
		if (!record.m_bContacted)
		{
			record.m_bContacted = true;
			record.m_iContactedAtSecond = state.m_iElapsedSeconds;
			record.m_sContactSourceId = sourceId;
			record.m_sContactReason = reason;
			changed = true;
		}
		if (resistanceActivityStarted && !record.m_bResistanceActivityStarted)
		{
			record.m_bResistanceActivityStarted = true;
			changed = true;
		}
		if (!changed)
			return true;
		record.m_sLastMutationId = sourceId;
		IncrementRevision(record);
		ProjectLegacyFields(state, record);
		return true;
	}

	HST_TownInfluenceResult Execute(
		HST_CampaignState state,
		HST_TownInfluenceCommand command)
	{
		return ExecuteWithPreset(state, command, m_Preset, false);
	}

	bool RegisterInfluenceEvent(
		HST_CampaignState state,
		string zoneId,
		string eventKind,
		int fiaSupportDelta,
		int occupierSupportDelta,
		int reputationDelta,
		int heatDelta,
		int populationDelta,
		int policeDelta,
		int roadblockDelta,
		string reason,
		HST_CampaignPreset preset = null,
		int durationSeconds = 0,
		string sourceId = "",
		int invaderSupportDelta = 0,
		bool populationScaled = false)
	{
		HST_TownInfluenceCommand command = new HST_TownInfluenceCommand();
		command.m_sTownId = zoneId;
		command.m_sEventKind = eventKind;
		command.m_sSourceId = sourceId;
		command.m_sReason = reason;
		command.m_iRawFIASupportDelta = fiaSupportDelta;
		command.m_iRawOccupierSupportDelta = occupierSupportDelta;
		command.m_iRawInvaderSupportDelta = invaderSupportDelta;
		command.m_iReputationDelta = reputationDelta;
		command.m_iHeatDelta = heatDelta;
		command.m_iPopulationDelta = populationDelta;
		command.m_iPoliceDelta = policeDelta;
		command.m_iRoadblockDelta = roadblockDelta;
		command.m_iDurationSeconds = durationSeconds;
		command.m_bPopulationScaled = populationScaled;
		return RegisterInfluenceEventExact(
			state,
			command,
			preset);
	}

	bool RegisterInfluenceEventExact(
		HST_CampaignState state,
		HST_TownInfluenceCommand command,
		HST_CampaignPreset preset = null)
	{
		if (!command)
			return false;
		command.m_bMarkContacted = IsExplicitContactEvent(
			command.m_sEventKind,
			command.m_sSourceId);
		command.m_bMarkResistanceActivity = command.m_bMarkContacted
			&& command.m_iRawFIASupportDelta != 0;
		HST_CampaignPreset effectivePreset = preset;
		if (!effectivePreset)
			effectivePreset = m_Preset;
		HST_TownInfluenceResult result = ExecuteWithPreset(
			state,
			command,
			effectivePreset,
			false);
		return result && result.m_bAccepted;
	}

	bool AddSupportChange(
		HST_CampaignState state,
		string townId,
		int requestedRawDelta,
		string reason = "town support change",
		HST_CampaignPreset preset = null,
		string sourceId = "",
		string exactEventId = "",
		bool populationScaled = false,
		bool reconcileOwnership = true)
	{
		HST_CampaignPreset effectivePreset = preset;
		if (!effectivePreset)
			effectivePreset = m_Preset;
		HST_TownInfluenceCommand command = new HST_TownInfluenceCommand();
		command.m_sCommandId = exactEventId;
		command.m_sEventId = exactEventId;
		command.m_sTownId = townId;
		command.m_sEventKind = "support_change";
		command.m_sSourceId = sourceId;
		command.m_sReason = reason;
		command.m_iRawFIASupportDelta = requestedRawDelta;
		command.m_bPopulationScaled = populationScaled;
		command.m_bReconcileOwnership = reconcileOwnership;
		return RegisterInfluenceEventExact(state, command, effectivePreset);
	}

#ifdef ENABLE_DIAG
	// Explicit absolute seed used only by authorized campaign-debug fixtures.
	// Keeping it here preserves the same revision, projection, and ownership
	// policy boundary as ordinary event-driven influence mutations.
	bool ApplyDebugSeed(
		HST_CampaignState state,
		string townId,
		int fiaSupportBasisPoints,
		int occupierSupportBasisPoints,
		int invaderSupportBasisPoints,
		int initialPopulation,
		int remainingPopulation,
		int destroyedPopulation,
		string mutationId)
	{
		HST_TownInfluenceRecord record = FindValidRecord(state, townId);
		if (!state || !record || !IsBasisPoints(fiaSupportBasisPoints)
			|| !IsBasisPoints(occupierSupportBasisPoints)
			|| !IsBasisPoints(invaderSupportBasisPoints)
			|| initialPopulation <= 0
			|| initialPopulation > MAX_TOWN_POPULATION
			|| remainingPopulation < 0
			|| remainingPopulation > initialPopulation
			|| destroyedPopulation < 0
			|| destroyedPopulation != initialPopulation - remainingPopulation)
			return false;
		mutationId = LimitText(mutationId.Trim(), MAX_ID_CHARACTERS);
		if (mutationId.IsEmpty())
			return false;

		HST_TownInfluenceCommand command = new HST_TownInfluenceCommand();
		command.m_sCommandId = string.Format(
			"town_debug_seed_%1_%2",
			townId.Hash(),
			mutationId.Hash());
		command.m_sEventId = command.m_sCommandId;
		command.m_sTownId = townId;
		command.m_sEventKind = "admin_debug_seed";
		command.m_sSourceId = mutationId;
		command.m_sReason = "authorized campaign debug town seed";
		command.m_bAbsoluteDebugSeed = true;
		command.m_iTargetFIASupportBasisPoints = fiaSupportBasisPoints;
		command.m_iTargetOccupierSupportBasisPoints = occupierSupportBasisPoints;
		command.m_iTargetInvaderSupportBasisPoints = invaderSupportBasisPoints;
		command.m_iTargetInitialPopulation = initialPopulation;
		command.m_iTargetRemainingPopulation = remainingPopulation;
		command.m_iTargetDestroyedPopulation = destroyedPopulation;
		command.m_bReconcileOwnership = true;
		HST_TownInfluenceEventState existing;
		if (!FindUniqueEvent(state, command.m_sEventId, existing))
			return false;
		if (existing)
		{
			command.m_iRawFIASupportDelta = existing.m_iFIASupportDelta;
			command.m_iRawOccupierSupportDelta = existing.m_iOccupierSupportDelta;
			command.m_iRawInvaderSupportDelta
				= existing.m_iRequestedInvaderBasisPointDelta;
			command.m_iPopulationDelta = existing.m_iPopulationDelta;
			command.m_bPopulationScaled = existing.m_bPopulationScaled;
		}
		else
		{
			if (record.m_iRevision
				> MAX_MUTABLE_REVISION - DEBUG_SEED_REVISION_HEADROOM)
				return false;
			command.m_iRawFIASupportDelta = fiaSupportBasisPoints
				- record.m_iFIASupportBasisPoints;
			command.m_iRawOccupierSupportDelta = occupierSupportBasisPoints
				- record.m_iOccupierSupportBasisPoints;
			command.m_iRawInvaderSupportDelta = invaderSupportBasisPoints
				- record.m_iInvaderSupportBasisPoints;
			command.m_iPopulationDelta = initialPopulation
				- record.m_iInitialPopulation;
		}
		HST_TownInfluenceResult eventResult = ExecuteWithPreset(
			state,
			command,
			m_Preset,
			true);
		return eventResult && eventResult.m_bAccepted;
	}
#endif

	bool RegisterOwnershipSupportReward(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string zoneId,
		int supportReward,
		string ownershipRequestId,
		bool reconcileOwnership = true)
	{
		if (!state || !preset || zoneId.IsEmpty()
			|| ownershipRequestId.IsEmpty() || supportReward == 0)
			return false;
		int fiaDelta;
		int occupierDelta;
		if (supportReward > 0)
			fiaDelta = supportReward;
		else
			occupierDelta = -supportReward;
		string eventId = BuildOwnershipRewardEventId(
			zoneId,
			ownershipRequestId);

		HST_TownInfluenceCommand command = new HST_TownInfluenceCommand();
		command.m_sCommandId = eventId;
		command.m_sEventId = eventId;
		command.m_sTownId = zoneId;
		command.m_sEventKind = "ownership_support";
		command.m_sSourceId = ownershipRequestId;
		command.m_sReason = "linked support from ownership transition";
		command.m_iRawFIASupportDelta = fiaDelta;
		command.m_iRawOccupierSupportDelta = occupierDelta;
		command.m_iReputationDelta = supportReward / 2;
		command.m_bPopulationScaled = true;
		command.m_bReconcileOwnership = false;
		HST_TownInfluenceResult result = ExecuteWithPreset(
			state,
			command,
			preset,
			true);
		if (!result || !result.m_bAccepted || !reconcileOwnership)
			return result && result.m_bAccepted;
		return ReconcileRecordOwnership(state, preset, result.m_Record, true);
	}

	bool ReconcileTownOwnershipPolicies(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		bool bypassCampaignClockRateLimit = false)
	{
		if (!state)
			return false;
		m_sLastAuthorityFailure = "";
		HST_CampaignPreset effectivePreset = preset;
		if (!effectivePreset)
			effectivePreset = m_Preset;
		bool changed = NormalizePre64TownRecords(state, effectivePreset);
		changed = ProcessDueInfluenceExpiries(state) || changed;
		if (!effectivePreset)
		{
			m_sLastAuthorityFailure = "campaign preset is unavailable";
			return changed;
		}
		if (!bypassCampaignClockRateLimit
			&& state.m_iElapsedSeconds < m_iNextOwnershipPolicyReconcileSecond)
			return changed;
		if (state.m_iElapsedSeconds
			> int.MAX - OWNERSHIP_POLICY_RECONCILE_SECONDS)
			m_iNextOwnershipPolicyReconcileSecond = int.MAX;
		else
			m_iNextOwnershipPolicyReconcileSecond = state.m_iElapsedSeconds
				+ OWNERSHIP_POLICY_RECONCILE_SECONDS;

		int visited;
		foreach (HST_TownInfluenceRecord record : state.m_aTownInfluenceRecords)
		{
			if (!record)
				continue;
			visited++;
			if (visited > MAX_TOWN_RECORDS)
				break;
			HST_TownInfluenceRecord valid = FindValidRecord(state, record.m_sTownId);
			if (!valid || valid != record)
				continue;
			if (record.m_iRevision >= MAX_MUTABLE_REVISION)
				continue;
			bool candidateChanged = UpdateOwnershipCandidate(
				state,
				effectivePreset,
				record);
			if (!m_sLastAuthorityFailure.IsEmpty())
				break;
			if (candidateChanged)
			{
				IncrementRevision(record);
				ProjectLegacyFields(state, record);
				changed = true;
			}
			if (record.m_sPendingOwnerFactionKey.IsEmpty())
				continue;
			if (HasDifferentUnresolvedTopLevelTransition(
					state,
					record.m_sPendingOwnershipRequestId))
				break;
			int revisionBefore = record.m_iRevision;
			bool reconciled = ReconcileRecordOwnership(
				state,
				effectivePreset,
				record,
				false);
			if (record.m_iRevision != revisionBefore)
				changed = true;
			if (!reconciled)
				break;
			// One political ownership command per reconciliation pass keeps the
			// global transition order deterministic.
			break;
		}
		return changed;
	}

	bool ProjectLegacyFields(HST_CampaignState state, HST_TownInfluenceRecord record)
	{
		if (!state || !record || record.m_iContractVersion != EXACT_CONTRACT_VERSION)
			return false;
		HST_ZoneState zone = FindUniqueCanonicalTownZone(
			state,
			record.m_sTownId);
		if (!zone)
			return false;
		HST_CivilianZoneState civilian = state.FindCivilianZone(record.m_sTownId);
		if (!civilian)
		{
			civilian = new HST_CivilianZoneState();
			civilian.m_sZoneId = record.m_sTownId;
			civilian.m_iReputation = 50;
			state.m_aCivilianZones.Insert(civilian);
		}

		int enemySupport = Math.Max(
			record.m_iOccupierSupportBasisPoints,
			record.m_iInvaderSupportBasisPoints);
		civilian.m_iFIASupport = BasisPointsToPercent(
			record.m_iFIASupportBasisPoints);
		civilian.m_iOccupierSupport = BasisPointsToPercent(enemySupport);
		civilian.m_iPopulationRemaining = record.m_iRemainingPopulation;
		civilian.m_iPopulationKilled = record.m_iDestroyedPopulation;
		civilian.m_iInfluenceEventCount = record.m_iInfluenceEventCount;
		civilian.m_iActiveInfluenceModifierCount = record.m_iActiveInfluenceModifierCount;
		civilian.m_iExpiredInfluenceModifierCount = record.m_iExpiredInfluenceModifierCount;
		civilian.m_sLastInfluenceEventId = record.m_sLastInfluenceEventId;
		civilian.m_sLastInfluenceKind = record.m_sLastInfluenceEventKind;
		civilian.m_sLastInfluenceReason = record.m_sLastInfluenceEventReason;
		civilian.m_iLastInfluenceEventSecond = record.m_iLastInfluenceEventSecond;
		civilian.m_iLastSupportChangeSecond = record.m_iLastInfluenceEventSecond;
		zone.m_iSupport = ClampSignedPercent(Math.Round(
			(record.m_iFIASupportBasisPoints - enemySupport) / 100.0));
		civilian.m_bUndercoverRestricted = zone.m_iSupport < 25;
		return true;
	}

	string BuildTownInfluenceReport(HST_CampaignState state, int maxRows = 20)
	{
		if (!state)
			return "Partisan town influence | state not ready";
		int boundedRows = Math.Max(0, Math.Min(40, maxRows));
		int validCount;
		int fiaTotal;
		int occupierTotal;
		int invaderTotal;
		int remainingTotal;
		int destroyedTotal;
		int contactedCount;
		string rows = "";
		foreach (HST_TownInfluenceRecord record : state.m_aTownInfluenceRecords)
		{
			if (!record || FindValidRecord(state, record.m_sTownId) != record)
				continue;
			validCount++;
			fiaTotal += record.m_iFIASupportBasisPoints;
			occupierTotal += record.m_iOccupierSupportBasisPoints;
			invaderTotal += record.m_iInvaderSupportBasisPoints;
			remainingTotal += record.m_iRemainingPopulation;
			destroyedTotal += record.m_iDestroyedPopulation;
			if (record.m_bContacted)
				contactedCount++;
			if (validCount <= boundedRows)
			{
				string pendingOwner = record.m_sPendingOwnerFactionKey;
				if (pendingOwner.IsEmpty())
					pendingOwner = "none";
				string lastEventId = record.m_sLastInfluenceEventId;
				if (lastEventId.IsEmpty())
					lastEventId = "none";
				string row = string.Format(
					"\n%1 | FIA/occupier/invader %2/%3/%4 bp | signed %5 pct | population %6/%7 | band %8",
					record.m_sTownId,
					record.m_iFIASupportBasisPoints,
					record.m_iOccupierSupportBasisPoints,
					record.m_iInvaderSupportBasisPoints,
					ResolveSignedSupportPercent(state, record.m_sTownId),
					record.m_iRemainingPopulation,
					record.m_iInitialPopulation,
					record.m_sHysteresisBand);
				row = row + string.Format(
					" | pending %1 | revision %2 | last %3",
					pendingOwner,
					record.m_iRevision,
					lastEventId);
				rows = rows + row;
			}
		}

		int averageFIA;
		int averageOccupier;
		int averageInvader;
		if (validCount > 0)
		{
			averageFIA = fiaTotal / validCount;
			averageOccupier = occupierTotal / validCount;
			averageInvader = invaderTotal / validCount;
		}
		return string.Format(
			"Partisan town influence | records %1 | contacted %2 | average FIA/occupier/invader %3/%4/%5 bp | population remaining/destroyed %6/%7 | formula source %8",
			validCount,
			contactedCount,
			averageFIA,
			averageOccupier,
			averageInvader,
			remainingTotal,
			destroyedTotal,
			FORMULA_REFERENCE_COMMIT) + rows;
	}

	protected HST_TownInfluenceResult ExecuteWithPreset(
		HST_CampaignState state,
		HST_TownInfluenceCommand command,
		HST_CampaignPreset preset,
		bool allowNestedOwnership)
	{
		HST_TownInfluenceResult result = new HST_TownInfluenceResult();
		string failure = ValidateCommand(state, command, preset);
		if (!failure.IsEmpty())
			return Reject(result, failure);

		HST_ZoneState zone = FindUniqueCanonicalTownZone(
			state,
			command.m_sTownId);
		HST_TownInfluenceRecord record = FindValidRecord(state, zone.m_sZoneId);
		if (!record)
			return Reject(result, "canonical town influence authority is unavailable");
		result.m_Record = record;
		HST_TownInfluenceEventState existing;
		if (!command.m_sEventId.IsEmpty())
		{
			if (!FindUniqueEvent(state, command.m_sEventId, existing))
				return Reject(result, "town influence event id is duplicated");
			if (existing)
			{
				result.m_Event = existing;
				if (!ExactEventMatches(existing, command))
					return Reject(result, "town influence event id was reused with a different fingerprint");
				result.m_bAccepted = true;
				result.m_bAlreadyApplied = true;
				if (command.m_bReconcileOwnership)
				{
					int revisionBeforeReconcile = record.m_iRevision;
					if (!ReconcileRecordOwnership(state, preset, record, allowNestedOwnership))
						result.m_sFailureReason = m_sLastAuthorityFailure;
					if (record.m_iRevision != revisionBeforeReconcile)
						result.m_bChanged = true;
				}
				result.m_bOwnershipPending = !record.m_sPendingOwnerFactionKey.IsEmpty();
				return result;
			}
		}
		failure = ValidateNewEventAdmission(state, command, record);
		if (!failure.IsEmpty())
			return Reject(result, failure);
		if (state.m_aTownInfluenceEvents.Count() >= MAX_INFLUENCE_EVENTS)
			return Reject(result, "bounded town influence event history is full");

		HST_TownInfluenceEventState eventState = BuildEvent(
			state,
			zone,
			record,
			command,
			preset);
		if (!eventState)
			return Reject(result, "town influence event could not be built");
		state.m_aTownInfluenceEvents.Insert(eventState);
		result.m_Event = eventState;

		HST_StrategicEventApplyResult strategicEvent;
		if (m_Strategic)
			strategicEvent = m_Strategic.BeginTownInfluenceEvent(state, preset, eventState);
		if (command.m_iAggressionDelta > 0
			&& (!strategicEvent || !strategicEvent.m_bRecorded
				|| !strategicEvent.m_Event
				|| strategicEvent.m_Event.m_sEventId.IsEmpty()))
		{
			if (strategicEvent && strategicEvent.m_Event)
				m_Strategic.DiscardStrategicEvent(state, strategicEvent);
			state.m_aTownInfluenceEvents.Remove(
				state.m_aTownInfluenceEvents.Count() - 1);
			result.m_Event = null;
			return Reject(
				result,
				"town influence aggression strategic receipt could not be admitted");
		}
		if (eventState.m_iAggressionDelta != 0
			&& !m_Economy.AddAggression(
				state,
				eventState.m_sAggressionFactionKey,
				eventState.m_iAggressionDelta,
				"enemy_aggression_town_" + eventState.m_sEventId,
				"town_influence",
				eventState.m_sEventId,
				"",
				"",
				"",
				eventState.m_sZoneId))
		{
			if (m_Strategic && strategicEvent && strategicEvent.m_Event)
				m_Strategic.DiscardStrategicEvent(state, strategicEvent);
			state.m_aTownInfluenceEvents.Remove(
				state.m_aTownInfluenceEvents.Count() - 1);
			result.m_Event = null;
			return Reject(
				result,
				"town influence aggression resource mutation could not be admitted");
		}
		if (eventState.m_iAggressionDelta != 0)
		{
			HST_FactionPoolState aggressionPool
				= state.FindFactionPool(eventState.m_sAggressionFactionKey);
			if (aggressionPool)
				eventState.m_iAggressionAfter = aggressionPool.m_iAggression;
		}
		ApplyEvent(state, zone, record, eventState, command, preset);
		result.m_bAccepted = true;
		result.m_bChanged = true;
		// Close the town receipt before ownership reconciliation. A threshold
		// transition owns a separate strategic receipt and may apply retaliation
		// aggression; folding it into this event would destroy exact attribution.
		if (m_Strategic && strategicEvent && strategicEvent.m_Event)
			m_Strategic.CompleteStrategicEvent(state, strategicEvent, true, true);

		if (command.m_bReconcileOwnership)
		{
			if (!ReconcileRecordOwnership(state, preset, record, allowNestedOwnership))
				result.m_sFailureReason = m_sLastAuthorityFailure;
		}
		result.m_bOwnershipPending = !record.m_sPendingOwnerFactionKey.IsEmpty();
		return result;
	}

	protected HST_TownInfluenceEventState BuildEvent(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_TownInfluenceRecord record,
		HST_TownInfluenceCommand command,
		HST_CampaignPreset preset)
	{
		HST_TownInfluenceEventState eventState = new HST_TownInfluenceEventState();
		eventState.m_iContractVersion = EXACT_CONTRACT_VERSION;
		eventState.m_sEventId = command.m_sEventId;
		if (eventState.m_sEventId.IsEmpty())
			eventState.m_sEventId = HST_StableIdService.NextId(
				state,
				"town_influence");
		if (eventState.m_sEventId.IsEmpty())
			return null;
		eventState.m_sZoneId = zone.m_sZoneId;
		eventState.m_sKind = command.m_sEventKind;
		eventState.m_sSourceId = command.m_sSourceId;
		eventState.m_sReason = command.m_sReason;
		eventState.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		if (command.m_iDurationSeconds > 0)
			eventState.m_iExpiresAtSecond = state.m_iElapsedSeconds
				+ command.m_iDurationSeconds;
		eventState.m_iFIASupportDelta = command.m_iRawFIASupportDelta;
		eventState.m_iOccupierSupportDelta = command.m_iRawOccupierSupportDelta;
		eventState.m_iReputationDelta = command.m_iReputationDelta;
		eventState.m_iHeatDelta = command.m_iHeatDelta;
		eventState.m_iPopulationDelta = command.m_iPopulationDelta;
		eventState.m_iPoliceDelta = command.m_iPoliceDelta;
		eventState.m_iRoadblockDelta = command.m_iRoadblockDelta;
		eventState.m_sAggressionFactionKey = command.m_sAggressionFactionKey;
		eventState.m_iAggressionDelta = command.m_iAggressionDelta;
		if (eventState.m_iAggressionDelta > 0)
		{
			HST_FactionPoolState aggressionPool
				= state.FindFactionPool(eventState.m_sAggressionFactionKey);
			if (aggressionPool)
				eventState.m_iAggressionBefore = aggressionPool.m_iAggression;
		}
		eventState.m_iPopulationUsed = record.m_iInitialPopulation;
		eventState.m_bPopulationScaled = command.m_bPopulationScaled;
#ifdef ENABLE_DIAG
		eventState.m_bAbsoluteDebugSeed = command.m_bAbsoluteDebugSeed;
#endif
		eventState.m_iInitialPopulationBefore = record.m_iInitialPopulation;
		eventState.m_iRemainingPopulationBefore = record.m_iRemainingPopulation;
		eventState.m_iDestroyedPopulationBefore = record.m_iDestroyedPopulation;

#ifdef ENABLE_DIAG
		if (command.m_bAbsoluteDebugSeed)
		{
			eventState.m_iRequestedFIABasisPointDelta
				= command.m_iRawFIASupportDelta;
			eventState.m_iRequestedOccupierBasisPointDelta
				= command.m_iRawOccupierSupportDelta;
			eventState.m_iRequestedInvaderBasisPointDelta
				= command.m_iRawInvaderSupportDelta;
			eventState.m_iEffectiveFIABasisPointDelta
				= command.m_iRawFIASupportDelta;
			eventState.m_iEffectiveOccupierBasisPointDelta
				= command.m_iRawOccupierSupportDelta;
			eventState.m_iEffectiveInvaderBasisPointDelta
				= command.m_iRawInvaderSupportDelta;
		}
		else
#endif
		{
			int occupierRaw = command.m_iRawOccupierSupportDelta;
			int invaderRaw = command.m_iRawInvaderSupportDelta;
			if (preset && !preset.m_sInvaderFactionKey.IsEmpty()
				&& zone.m_sOwnerFactionKey == preset.m_sInvaderFactionKey)
			{
				invaderRaw += occupierRaw;
				occupierRaw = 0;
			}
			eventState.m_iRequestedFIABasisPointDelta
				= CalculateRequestedSupportDeltaBasisPoints(
					command.m_iRawFIASupportDelta);
			eventState.m_iRequestedOccupierBasisPointDelta
				= CalculateRequestedSupportDeltaBasisPoints(occupierRaw);
			eventState.m_iRequestedInvaderBasisPointDelta
				= CalculateRequestedSupportDeltaBasisPoints(invaderRaw);
			eventState.m_iEffectiveFIABasisPointDelta
				= CalculateEffectiveSupportDeltaBasisPoints(
					command.m_iRawFIASupportDelta,
					record.m_iInitialPopulation,
					command.m_bPopulationScaled);
			eventState.m_iEffectiveOccupierBasisPointDelta
				= CalculateEffectiveSupportDeltaBasisPoints(
					occupierRaw,
					record.m_iInitialPopulation,
					command.m_bPopulationScaled);
			eventState.m_iEffectiveInvaderBasisPointDelta
				= CalculateEffectiveSupportDeltaBasisPoints(
					invaderRaw,
					record.m_iInitialPopulation,
					command.m_bPopulationScaled);
		}
		eventState.m_iRecordRevisionBefore = record.m_iRevision;
		eventState.m_iFIABasisPointsBefore = record.m_iFIASupportBasisPoints;
		eventState.m_iOccupierBasisPointsBefore = record.m_iOccupierSupportBasisPoints;
		eventState.m_iInvaderBasisPointsBefore = record.m_iInvaderSupportBasisPoints;
		return eventState;
	}

	protected void ApplyEvent(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_TownInfluenceRecord record,
		HST_TownInfluenceEventState eventState,
		HST_TownInfluenceCommand command,
		HST_CampaignPreset preset)
	{
		int fiaBefore = record.m_iFIASupportBasisPoints;
		int occupierBefore = record.m_iOccupierSupportBasisPoints;
		int invaderBefore = record.m_iInvaderSupportBasisPoints;
#ifdef ENABLE_DIAG
		if (eventState.m_bAbsoluteDebugSeed)
		{
			record.m_iFIASupportBasisPoints
				= command.m_iTargetFIASupportBasisPoints;
			record.m_iOccupierSupportBasisPoints
				= command.m_iTargetOccupierSupportBasisPoints;
			record.m_iInvaderSupportBasisPoints
				= command.m_iTargetInvaderSupportBasisPoints;
		}
		else
#endif
		{
			record.m_iFIASupportBasisPoints = ClampBasisPoints(
				fiaBefore + eventState.m_iEffectiveFIABasisPointDelta);
			record.m_iOccupierSupportBasisPoints = ClampBasisPoints(
				occupierBefore + eventState.m_iEffectiveOccupierBasisPointDelta);
			record.m_iInvaderSupportBasisPoints = ClampBasisPoints(
				invaderBefore + eventState.m_iEffectiveInvaderBasisPointDelta);
		}

#ifdef ENABLE_DIAG
		if (eventState.m_bAbsoluteDebugSeed)
		{
			record.m_iInitialPopulation = command.m_iTargetInitialPopulation;
			record.m_iRemainingPopulation = command.m_iTargetRemainingPopulation;
			record.m_iDestroyedPopulation = command.m_iTargetDestroyedPopulation;
		}
		else if (eventState.m_iPopulationDelta < 0)
#else
		if (eventState.m_iPopulationDelta < 0)
#endif
		{
			int destroyed = Math.Min(
				record.m_iRemainingPopulation,
				-eventState.m_iPopulationDelta);
			record.m_iRemainingPopulation -= destroyed;
			record.m_iDestroyedPopulation += destroyed;
		}
		else if (eventState.m_iPopulationDelta > 0)
		{
			int growth = eventState.m_iPopulationDelta;
			if (record.m_iInitialPopulation <= int.MAX - growth
				&& record.m_iRemainingPopulation <= int.MAX - growth)
			{
				record.m_iInitialPopulation += growth;
				record.m_iRemainingPopulation += growth;
			}
		}

		HST_CivilianZoneState civilian = EnsureLegacyCivilianProjection(state, record);
		if (civilian)
		{
			civilian.m_iReputation = SafeAddClamped(
				civilian.m_iReputation,
				eventState.m_iReputationDelta,
				0,
				100);
			civilian.m_iWantedHeat = SafeAddClamped(
				civilian.m_iWantedHeat,
				eventState.m_iHeatDelta,
				0,
				MAX_LEGACY_PRESSURE);
			civilian.m_iPolicePresence = SafeAddClamped(
				civilian.m_iPolicePresence,
				eventState.m_iPoliceDelta,
				0,
				MAX_LEGACY_PRESSURE);
			civilian.m_iRoadblockPresence = SafeAddClamped(
				civilian.m_iRoadblockPresence,
				eventState.m_iRoadblockDelta,
				0,
				MAX_LEGACY_PRESSURE);
			civilian.m_iLastIncidentSecond = state.m_iElapsedSeconds;
			civilian.m_sLastIncidentReason = eventState.m_sReason;
		}

		if (command.m_bMarkContacted || command.m_bMarkResistanceActivity)
			ApplyContactEvidence(record, state, eventState, command.m_bMarkResistanceActivity);
		ApplyEventKindAggregates(
			record,
			eventState,
			record.m_iFIASupportBasisPoints - fiaBefore,
			record.m_iOccupierSupportBasisPoints - occupierBefore,
			record.m_iInvaderSupportBasisPoints - invaderBefore);

		record.m_iInfluenceEventCount++;
		if (eventState.m_iExpiresAtSecond > state.m_iElapsedSeconds)
		{
			record.m_iActiveInfluenceModifierCount++;
			if (record.m_iNextInfluenceExpirySecond <= 0
				|| eventState.m_iExpiresAtSecond < record.m_iNextInfluenceExpirySecond)
				record.m_iNextInfluenceExpirySecond = eventState.m_iExpiresAtSecond;
		}
		record.m_sLastInfluenceEventId = eventState.m_sEventId;
		record.m_sLastInfluenceEventKind = eventState.m_sKind;
		record.m_sLastInfluenceEventReason = eventState.m_sReason;
		record.m_iLastInfluenceEventSecond = eventState.m_iCreatedAtSecond;
		record.m_sLastMutationId = eventState.m_sEventId;
		UpdateOwnershipCandidate(state, preset, record);
		record.m_iLastHysteresisEvaluationSecond = state.m_iElapsedSeconds;
		IncrementRevision(record);

		eventState.m_iRecordRevisionAfter = record.m_iRevision;
		eventState.m_iFIABasisPointsAfter = record.m_iFIASupportBasisPoints;
		eventState.m_iOccupierBasisPointsAfter = record.m_iOccupierSupportBasisPoints;
		eventState.m_iInvaderBasisPointsAfter = record.m_iInvaderSupportBasisPoints;
		eventState.m_iInitialPopulationAfter = record.m_iInitialPopulation;
		eventState.m_iRemainingPopulationAfter = record.m_iRemainingPopulation;
		eventState.m_iDestroyedPopulationAfter = record.m_iDestroyedPopulation;
		eventState.m_bApplied = true;
		ProjectLegacyFields(state, record);
	}

	protected bool UpdateOwnershipCandidate(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_TownInfluenceRecord record)
	{
		if (!state || !record)
			return false;
		string previousBand = record.m_sHysteresisBand;
		string previousOwner = record.m_sPendingOwnerFactionKey;
		int previousSince = record.m_iPendingOwnerSinceSecond;
		string previousRequest = record.m_sPendingOwnershipRequestId;
		string previousLastFlipOwner = record.m_sLastFlipOwnerFactionKey;
		int previousLastFlipSecond = record.m_iLastFlipSecond;
		int previousLastFlipRevision = record.m_iLastFlipOwnershipRevision;
		if (!FinalizeCompletedPendingOwnership(state, record))
			return false;
		record.m_sHysteresisBand = ResolveHysteresisBand(
			record.m_iFIASupportBasisPoints);
		// Once accepted, an ownership receipt owns its frozen political intent.
		// Later support movement cannot abandon that receipt before completion.
		if (!record.m_sPendingOwnershipRequestId.IsEmpty())
		{
			HST_OwnershipTransitionState pendingTransition
				= state.FindOwnershipTransition(record.m_sPendingOwnershipRequestId);
			if (pendingTransition && !pendingTransition.m_bCompleted)
			{
				bool frozenBandChanged
					= record.m_sHysteresisBand != previousBand;
				if (frozenBandChanged)
					record.m_iLastHysteresisEvaluationSecond
						= state.m_iElapsedSeconds;
				return frozenBandChanged;
			}
		}

		string candidateOwner;
		if (record.m_sHysteresisBand == HYSTERESIS_RESISTANCE && preset)
			candidateOwner = preset.m_sResistanceFactionKey;
		else if (record.m_sHysteresisBand == HYSTERESIS_ENEMY && preset)
			candidateOwner = ResolveEnemyOwnershipCandidate(state, preset, record);
		HST_ZoneState zone = FindUniqueCanonicalTownZone(state, record.m_sTownId);
		if (!zone || candidateOwner.IsEmpty()
			|| zone.m_sOwnerFactionKey == candidateOwner)
		{
			ClearPendingOwner(record);
		}
		else if (record.m_sPendingOwnerFactionKey != candidateOwner)
		{
			record.m_sPendingOwnerFactionKey = candidateOwner;
			record.m_iPendingOwnerSinceSecond = state.m_iElapsedSeconds;
			record.m_sPendingOwnershipRequestId = "";
		}

		bool policyChanged = record.m_sHysteresisBand != previousBand
			|| record.m_sPendingOwnerFactionKey != previousOwner
			|| record.m_iPendingOwnerSinceSecond != previousSince
			|| record.m_sPendingOwnershipRequestId != previousRequest
			|| record.m_sLastFlipOwnerFactionKey != previousLastFlipOwner
			|| record.m_iLastFlipSecond != previousLastFlipSecond
			|| record.m_iLastFlipOwnershipRevision != previousLastFlipRevision;
		if (policyChanged)
			record.m_iLastHysteresisEvaluationSecond = state.m_iElapsedSeconds;
		return policyChanged;
	}

	protected bool ReconcileRecordOwnership(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_TownInfluenceRecord record,
		bool allowNestedOwnership)
	{
		m_sLastAuthorityFailure = "";
		if (!state || !preset || !record)
			return FailAuthority("town ownership state, preset, or record is unavailable");
		if (record.m_iRevision >= MAX_MUTABLE_REVISION)
			return FailAuthority("town influence revision authority is exhausted");
		bool policyChanged = UpdateOwnershipCandidate(state, preset, record);
		if (!m_sLastAuthorityFailure.IsEmpty())
			return false;
		if (policyChanged)
		{
			IncrementRevision(record);
			ProjectLegacyFields(state, record);
		}
		if (record.m_sPendingOwnerFactionKey.IsEmpty())
			return true;
		HST_ZoneState zone = FindUniqueCanonicalTownZone(state, record.m_sTownId);
		if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return FailAuthority("town ownership zone is unavailable");
		if (zone.m_iOwnershipContractVersion
			!= HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			|| !zone.m_sOwnershipAuthorityFailure.IsEmpty())
			return FailAuthority("town ownership authority is invalid");
		if (!state.FindFactionPool(zone.m_sOwnerFactionKey)
			|| !state.FindFactionPool(record.m_sPendingOwnerFactionKey))
			return FailAuthority("town ownership faction authority is unavailable");
		if (!m_OwnershipTransitions)
			return FailAuthority("ownership transition service is unavailable");

		string sourceId = record.m_sLastInfluenceEventId;
		if (sourceId.IsEmpty())
			sourceId = string.Format(
				"town_policy_%1_%2",
				record.m_sTownId,
				record.m_iRevision);
		string requestId = record.m_sPendingOwnershipRequestId;
		bool preparedRequestId = requestId.IsEmpty();
		if (requestId.IsEmpty())
		{
			requestId = BuildPoliticalOwnershipRequestId(
				zone.m_sZoneId,
				Math.Max(1, zone.m_iOwnershipRevision),
				sourceId);
		}
		string policyReason = "support flipped town to enemy";
		if (record.m_sPendingOwnerFactionKey == preset.m_sResistanceFactionKey)
			policyReason = "support flipped town to resistance";
		HST_OwnershipTransitionRequest request = m_OwnershipTransitions.BuildRequest(
			state,
			zone.m_sZoneId,
			record.m_sPendingOwnerFactionKey,
			"political_support",
			"town_influence",
			sourceId,
			policyReason,
			0,
			requestId);
		if (state.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_SETUP
			|| state.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_WON
			|| state.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_LOST)
		{
			request.m_bApplyEnemyConsequences = false;
			request.m_bNotify = false;
		}
		HST_OwnershipTransitionResult transitionResult
			= m_OwnershipTransitions.Apply(state, request);
		if (!transitionResult || !transitionResult.m_bAccepted)
		{
			if (transitionResult)
				return FailAuthority(transitionResult.m_sFailureReason);
			return FailAuthority("political ownership transition was rejected");
		}
		if (!transitionResult.m_bCompleted)
		{
			if (preparedRequestId)
			{
				record.m_sPendingOwnershipRequestId = requestId;
				IncrementRevision(record);
				ProjectLegacyFields(state, record);
			}
			return FailAuthority("political ownership transition remains unresolved");
		}

		record.m_sLastFlipOwnerFactionKey = record.m_sPendingOwnerFactionKey;
		record.m_iLastFlipSecond = state.m_iElapsedSeconds;
		record.m_iLastFlipOwnershipRevision = zone.m_iOwnershipRevision;
		record.m_iOwnershipCooldownUntilSecond = 0;
		ClearPendingOwner(record);
		record.m_sLastMutationId = requestId;
		IncrementRevision(record);
		ProjectLegacyFields(state, record);
		return true;
	}

	protected bool ProcessDueInfluenceExpiries(HST_CampaignState state)
	{
		if (!state)
			return false;
		if (state.m_aTownInfluenceEvents.Count() > MAX_INFLUENCE_EVENTS)
		{
			m_sLastAuthorityFailure = "town influence event history exceeds the bounded expiry scan";
			return false;
		}
		m_mDueExpiryRecords.Clear();
		m_mExpiryTownIdCounts.Clear();
		foreach (HST_TownInfluenceRecord record : state.m_aTownInfluenceRecords)
		{
			if (!record || record.m_iNextInfluenceExpirySecond <= 0
				|| record.m_iNextInfluenceExpirySecond > state.m_iElapsedSeconds)
				continue;
			if (!m_mExpiryTownIdCounts.Contains(record.m_sTownId)
				&& m_mExpiryTownIdCounts.Count() >= MAX_TOWN_RECORDS)
				break;
			m_mExpiryTownIdCounts.Set(record.m_sTownId, 0);
		}
		if (m_mExpiryTownIdCounts.Count() == 0)
			return false;

		foreach (HST_TownInfluenceRecord countedRecord : state.m_aTownInfluenceRecords)
		{
			if (!countedRecord)
				continue;
			int townCount;
			if (!m_mExpiryTownIdCounts.Find(countedRecord.m_sTownId, townCount))
				continue;
			m_mExpiryTownIdCounts.Set(countedRecord.m_sTownId, townCount + 1);
		}
		foreach (HST_TownInfluenceRecord dueRecord : state.m_aTownInfluenceRecords)
		{
			if (!dueRecord || dueRecord.m_iNextInfluenceExpirySecond <= 0
				|| dueRecord.m_iNextInfluenceExpirySecond > state.m_iElapsedSeconds)
				continue;
			int authorityCount;
			if (!m_mExpiryTownIdCounts.Find(dueRecord.m_sTownId, authorityCount)
				|| authorityCount != 1
				|| dueRecord.m_iContractVersion != EXACT_CONTRACT_VERSION
				|| dueRecord.m_iRevision >= MAX_MUTABLE_REVISION
				|| !dueRecord.m_sAuthorityFailure.IsEmpty())
				continue;
			dueRecord.m_iActiveInfluenceModifierCount = 0;
			dueRecord.m_iExpiredInfluenceModifierCount = 0;
			dueRecord.m_iNextInfluenceExpirySecond = 0;
			m_mDueExpiryRecords.Set(dueRecord.m_sTownId, dueRecord);
		}
		if (m_mDueExpiryRecords.Count() == 0)
			return false;

		int visited;
		foreach (HST_TownInfluenceEventState eventState : state.m_aTownInfluenceEvents)
		{
			visited++;
			if (visited > MAX_INFLUENCE_EVENTS)
				break;
			if (!eventState || !eventState.m_bApplied
				|| eventState.m_iExpiresAtSecond <= 0)
				continue;
			HST_TownInfluenceRecord dueRecord;
			if (!m_mDueExpiryRecords.Find(eventState.m_sZoneId, dueRecord)
				|| !dueRecord)
				continue;
			if (eventState.m_iExpiresAtSecond > state.m_iElapsedSeconds)
			{
				dueRecord.m_iActiveInfluenceModifierCount++;
				if (dueRecord.m_iNextInfluenceExpirySecond <= 0
					|| eventState.m_iExpiresAtSecond
					< dueRecord.m_iNextInfluenceExpirySecond)
					dueRecord.m_iNextInfluenceExpirySecond
						= eventState.m_iExpiresAtSecond;
			}
			else
				dueRecord.m_iExpiredInfluenceModifierCount++;
		}

		foreach (string townId, HST_TownInfluenceRecord changedRecord : m_mDueExpiryRecords)
		{
			changedRecord.m_iActiveInfluenceModifierCount = Math.Min(
				changedRecord.m_iInfluenceEventCount,
				changedRecord.m_iActiveInfluenceModifierCount);
			changedRecord.m_iExpiredInfluenceModifierCount = Math.Min(
				Math.Max(0, changedRecord.m_iInfluenceEventCount
					- changedRecord.m_iActiveInfluenceModifierCount),
				changedRecord.m_iExpiredInfluenceModifierCount);
			IncrementRevision(changedRecord);
			ProjectLegacyFields(state, changedRecord);
		}
		return true;
	}

	protected HST_TownInfluenceRecord BuildInitialRecord(
		HST_CampaignState state,
		HST_ZoneState zone)
	{
		if (!state || !zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return null;
		HST_CivilianZoneState civilian = state.FindCivilianZone(zone.m_sZoneId);
		int fiaPercent = Math.Max(0, Math.Min(100, 50 + zone.m_iSupport / 2));
		int enemyPercent = Math.Max(0, Math.Min(100, 50 - zone.m_iSupport / 2));
		if (civilian && civilian.m_iFIASupport >= 0
			&& civilian.m_iFIASupport <= 100
			&& civilian.m_iOccupierSupport >= 0
			&& civilian.m_iOccupierSupport <= 100)
		{
			fiaPercent = civilian.m_iFIASupport;
			enemyPercent = civilian.m_iOccupierSupport;
		}

		HST_TownInfluenceRecord record = new HST_TownInfluenceRecord();
		record.m_iContractVersion = EXACT_CONTRACT_VERSION;
		record.m_sTownId = zone.m_sZoneId;
		record.m_iRevision = 1;
		record.m_iFIASupportBasisPoints = fiaPercent * 100;
		bool invaderOwned = m_Preset && !m_Preset.m_sInvaderFactionKey.IsEmpty()
			&& zone.m_sOwnerFactionKey == m_Preset.m_sInvaderFactionKey;
		if (invaderOwned)
			record.m_iInvaderSupportBasisPoints = enemyPercent * 100;
		else
			record.m_iOccupierSupportBasisPoints = enemyPercent * 100;

		int remaining;
		int destroyed;
		int presence;
		if (civilian)
		{
			remaining = Math.Max(0, civilian.m_iPopulationRemaining);
			destroyed = Math.Max(0, civilian.m_iPopulationKilled);
			presence = Math.Max(0, civilian.m_iCivilianPresence);
		}
		int initial = Math.Min(
			MAX_TOWN_POPULATION,
			SafePopulationSum(remaining, destroyed));
		remaining = Math.Min(initial, remaining);
		if (initial <= 0)
		{
			if (presence > MAX_TOWN_POPULATION / 8)
				initial = MAX_TOWN_POPULATION;
			else
				initial = Math.Max(20, Math.Max(1, presence) * 8);
			remaining = initial;
			destroyed = 0;
		}
		record.m_iInitialPopulation = initial;
		record.m_iRemainingPopulation = remaining;
		record.m_iDestroyedPopulation = Math.Min(
			destroyed,
			initial - remaining);
		record.m_sHysteresisBand = ResolveHysteresisBand(
			record.m_iFIASupportBasisPoints);
		record.m_iLastHysteresisEvaluationSecond = state.m_iElapsedSeconds;
		record.m_sLastMutationId = LimitText(
			"town_influence_initialized_" + zone.m_sZoneId,
			MAX_ID_CHARACTERS);
		return record;
	}

	protected bool NormalizePre64TownRecords(
		HST_CampaignState state,
		HST_CampaignPreset preset)
	{
		if (!state || !preset || state.m_iLastLoadedSchemaVersion >= SCHEMA_VERSION)
			return false;
		if (m_Pre64NormalizedState == state
			&& m_iPre64NormalizedRestoreSequence
				== state.m_iPersistenceRestoreSequence)
			return false;

		m_mPre64TownIdCounts.Clear();
		foreach (HST_TownInfluenceRecord counted : state.m_aTownInfluenceRecords)
		{
			if (!counted || counted.m_sTownId.IsEmpty())
				continue;
			int count;
			if (m_mPre64TownIdCounts.Find(counted.m_sTownId, count))
				m_mPre64TownIdCounts.Set(counted.m_sTownId, count + 1);
			else if (m_mPre64TownIdCounts.Count() < MAX_TOWN_RECORDS)
				m_mPre64TownIdCounts.Set(counted.m_sTownId, 1);
		}

		bool changed;
		foreach (HST_TownInfluenceRecord record : state.m_aTownInfluenceRecords)
		{
			if (!record || record.m_iContractVersion != EXACT_CONTRACT_VERSION
				|| !record.m_sAuthorityFailure.IsEmpty()
				|| record.m_iRevision >= MAX_MUTABLE_REVISION)
				continue;
			int authorityCount;
			if (!m_mPre64TownIdCounts.Find(record.m_sTownId, authorityCount)
				|| authorityCount != 1)
				continue;
			HST_ZoneState zone = FindUniqueCanonicalTownZone(
				state,
				record.m_sTownId);
			if (!zone)
				continue;

			bool recordChanged;
			if (!preset.m_sInvaderFactionKey.IsEmpty()
				&& zone.m_sOwnerFactionKey == preset.m_sInvaderFactionKey)
			{
				if (record.m_iOccupierSupportBasisPoints != 0)
				{
					record.m_iInvaderSupportBasisPoints = ClampBasisPoints(
						record.m_iInvaderSupportBasisPoints
							+ record.m_iOccupierSupportBasisPoints);
					record.m_iOccupierSupportBasisPoints = 0;
					recordChanged = true;
				}
				if (record.m_iOccupierRadioBasisPoints != 0)
				{
					record.m_iInvaderRadioBasisPoints = ClampBasisPoints(
						record.m_iInvaderRadioBasisPoints
							+ record.m_iOccupierRadioBasisPoints);
					record.m_iOccupierRadioBasisPoints = 0;
					recordChanged = true;
				}
				if (record.m_iOccupierPropagandaBasisPoints != 0)
				{
					record.m_iInvaderPropagandaBasisPoints = ClampBasisPoints(
						record.m_iInvaderPropagandaBasisPoints
							+ record.m_iOccupierPropagandaBasisPoints);
					record.m_iOccupierPropagandaBasisPoints = 0;
					recordChanged = true;
				}
			}
			string expectedBand = ResolveHysteresisBand(
				record.m_iFIASupportBasisPoints);
			if (record.m_sHysteresisBand != expectedBand)
			{
				record.m_sHysteresisBand = expectedBand;
				recordChanged = true;
			}
			if (!recordChanged)
				continue;
			record.m_iLastHysteresisEvaluationSecond = state.m_iElapsedSeconds;
			record.m_sLastMutationId = LimitText(
				"schema64_pre64_town_normalization_" + record.m_sTownId,
				MAX_ID_CHARACTERS);
			IncrementRevision(record);
			ProjectLegacyFields(state, record);
			changed = true;
		}
		m_Pre64NormalizedState = state;
		m_iPre64NormalizedRestoreSequence = state.m_iPersistenceRestoreSequence;
		return changed;
	}

	protected bool IsRecordContractValid(
		HST_CampaignState state,
		HST_TownInfluenceRecord record,
		int elapsedSecond)
	{
		if (!record || record.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| record.m_sTownId.IsEmpty() || record.m_iRevision <= 0
			|| record.m_iRevision > MAX_MUTABLE_REVISION
			|| !record.m_sAuthorityFailure.IsEmpty())
			return false;
		if (!IsBasisPoints(record.m_iFIASupportBasisPoints)
			|| !IsBasisPoints(record.m_iOccupierSupportBasisPoints)
			|| !IsBasisPoints(record.m_iInvaderSupportBasisPoints)
			|| !IsBasisPoints(record.m_iFIARadioBasisPoints)
			|| !IsBasisPoints(record.m_iOccupierRadioBasisPoints)
			|| !IsBasisPoints(record.m_iInvaderRadioBasisPoints)
			|| !IsBasisPoints(record.m_iFIAPropagandaBasisPoints)
			|| !IsBasisPoints(record.m_iOccupierPropagandaBasisPoints)
			|| !IsBasisPoints(record.m_iInvaderPropagandaBasisPoints))
			return false;
		if (record.m_iInitialPopulation <= 0
			|| record.m_iInitialPopulation > MAX_TOWN_POPULATION
			|| record.m_iRemainingPopulation < 0
			|| record.m_iRemainingPopulation > MAX_TOWN_POPULATION
			|| record.m_iRemainingPopulation > record.m_iInitialPopulation
			|| record.m_iDestroyedPopulation < 0
			|| record.m_iDestroyedPopulation
				!= record.m_iInitialPopulation - record.m_iRemainingPopulation)
			return false;
		if (record.m_bResistanceActivityStarted && !record.m_bContacted)
			return false;
		int now = Math.Max(0, elapsedSecond);
		if (record.m_bContacted)
		{
			if (record.m_iContactedAtSecond < 0
				|| record.m_iContactedAtSecond > now
				|| record.m_sContactSourceId.IsEmpty()
				|| record.m_sContactReason.IsEmpty())
				return false;
		}
		else if (record.m_iContactedAtSecond != 0
			|| !record.m_sContactSourceId.IsEmpty()
			|| !record.m_sContactReason.IsEmpty())
			return false;
		if (record.m_sHysteresisBand != HYSTERESIS_RESISTANCE
			&& record.m_sHysteresisBand != HYSTERESIS_NEUTRAL
			&& record.m_sHysteresisBand != HYSTERESIS_ENEMY)
			return false;
		if (record.m_sHysteresisBand
			!= ResolveHysteresisBand(record.m_iFIASupportBasisPoints))
			return false;
		if (record.m_iPendingOwnerSinceSecond < 0
			|| record.m_iPendingOwnerSinceSecond > now
			|| record.m_iOwnershipCooldownUntilSecond < 0
			|| record.m_iLastHysteresisEvaluationSecond < 0
			|| record.m_iLastHysteresisEvaluationSecond > now
			|| record.m_iLastFlipSecond < 0
			|| record.m_iLastFlipSecond > now
			|| record.m_iLastFlipOwnershipRevision < 0)
			return false;
		if (record.m_sPendingOwnerFactionKey.IsEmpty())
		{
			if (record.m_iPendingOwnerSinceSecond != 0
				|| !record.m_sPendingOwnershipRequestId.IsEmpty())
				return false;
		}
		else if (record.m_iPendingOwnerSinceSecond > now)
			return false;
		if (!HasValidPendingOwnershipReceiptLink(state, record))
			return false;
		if (record.m_sLastFlipOwnerFactionKey.IsEmpty())
		{
			if (record.m_iLastFlipSecond != 0
				|| record.m_iLastFlipOwnershipRevision != 0)
				return false;
		}
		else if (record.m_iLastFlipOwnershipRevision <= 0)
			return false;
		if (record.m_iInfluenceEventCount < 0
			|| record.m_iInfluenceEventCount > MAX_INFLUENCE_EVENTS
			|| record.m_iActiveInfluenceModifierCount < 0
			|| record.m_iActiveInfluenceModifierCount > MAX_INFLUENCE_EVENTS
			|| record.m_iExpiredInfluenceModifierCount < 0
			|| record.m_iExpiredInfluenceModifierCount > MAX_INFLUENCE_EVENTS
			|| record.m_iActiveInfluenceModifierCount > record.m_iInfluenceEventCount
			|| record.m_iExpiredInfluenceModifierCount
				> record.m_iInfluenceEventCount
					- record.m_iActiveInfluenceModifierCount
			|| record.m_iNextInfluenceExpirySecond < 0
			|| (record.m_iNextInfluenceExpirySecond > 0
				&& record.m_iNextInfluenceExpirySecond <= now))
			return false;
		if (record.m_sLastInfluenceEventId.IsEmpty())
		{
			if (!record.m_sLastInfluenceEventKind.IsEmpty()
				|| !record.m_sLastInfluenceEventReason.IsEmpty()
				|| record.m_iLastInfluenceEventSecond != 0)
				return false;
		}
		else if (record.m_sLastInfluenceEventKind.IsEmpty()
			|| record.m_sLastInfluenceEventReason.IsEmpty()
			|| record.m_iLastInfluenceEventSecond < 0
			|| record.m_iLastInfluenceEventSecond > now)
			return false;
		return true;
	}

	protected bool IsValidPopulationTriple(
		int initialPopulation,
		int remainingPopulation,
		int destroyedPopulation)
	{
		return initialPopulation > 0
			&& initialPopulation <= MAX_TOWN_POPULATION
			&& remainingPopulation >= 0
			&& remainingPopulation <= initialPopulation
			&& destroyedPopulation >= 0
			&& destroyedPopulation == initialPopulation - remainingPopulation;
	}

	protected string ValidateCommand(
		HST_CampaignState state,
		HST_TownInfluenceCommand command,
		HST_CampaignPreset preset)
	{
		if (!state || !command)
			return "state or town influence command is unavailable";
		if (command.m_sTownId.IsEmpty())
			return "town influence command requires a town id";
		command.m_sCommandId = command.m_sCommandId.Trim();
		command.m_sEventId = command.m_sEventId.Trim();
		if (command.m_sEventId.IsEmpty() && !command.m_sCommandId.IsEmpty())
			command.m_sEventId = command.m_sCommandId;
		HST_ZoneState zone = FindUniqueCanonicalTownZone(
			state,
			command.m_sTownId);
		if (!zone)
			return "town influence command does not target a canonical town";
		command.m_sTownId = zone.m_sZoneId;
		if (command.m_sEventKind.Trim().IsEmpty())
			command.m_sEventKind = "event";
		command.m_sEventKind = LimitText(command.m_sEventKind.Trim(), MAX_ID_CHARACTERS);
		command.m_sSourceId = LimitText(command.m_sSourceId.Trim(), MAX_ID_CHARACTERS);
		command.m_sReason = LimitText(command.m_sReason.Trim(), MAX_REASON_CHARACTERS);
		command.m_sAggressionFactionKey
			= command.m_sAggressionFactionKey.Trim();
		if (command.m_sReason.IsEmpty())
			command.m_sReason = "town influence " + command.m_sEventKind;
		if (command.m_sEventId.Length() > MAX_ID_CHARACTERS
			|| command.m_sCommandId.Length() > MAX_ID_CHARACTERS
			|| command.m_sAggressionFactionKey.Length()
				> MAX_ID_CHARACTERS)
			return "town influence exact id or faction key is too long";
		if (command.m_iDurationSeconds < 0
			|| command.m_iDurationSeconds > MAX_DURATION_SECONDS)
			return "town influence duration is invalid";
		if (!IsRawSupportDelta(command.m_iRawFIASupportDelta)
			|| !IsRawSupportDelta(command.m_iRawOccupierSupportDelta)
			|| !IsRawSupportDelta(command.m_iRawInvaderSupportDelta))
			return "town influence raw support delta is outside the bounded range";
		if (!IsNonSupportDelta(command.m_iReputationDelta)
			|| !IsNonSupportDelta(command.m_iHeatDelta)
			|| !IsNonSupportDelta(command.m_iPoliceDelta)
			|| !IsNonSupportDelta(command.m_iRoadblockDelta))
			return "town influence ambient or security delta is outside the bounded range";
		if (command.m_iAggressionDelta < 0
			|| command.m_iAggressionDelta > MAX_AGGRESSION_DELTA)
			return "town influence aggression delta is outside the bounded range";
		if (command.m_sAggressionFactionKey.IsEmpty()
			!= (command.m_iAggressionDelta == 0))
			return "town influence aggression target and delta are inconsistent";
		if (command.m_iAggressionDelta > 0
			&& command.m_sEventId.IsEmpty())
			return "town influence aggression requires an exact event id";
		if (!command.m_sAggressionFactionKey.IsEmpty()
			&& (!preset || !HST_FactionRelationService.IsEnemyFaction(
				preset,
				command.m_sAggressionFactionKey)))
			return "town influence aggression target is not an enemy faction";
		HST_TownInfluenceRecord record = FindValidRecord(state, zone.m_sZoneId);
		if (!record)
			return "canonical town influence record is unavailable";
#ifdef ENABLE_DIAG
		if (command.m_bAbsoluteDebugSeed)
		{
			if (command.m_sEventKind != "admin_debug_seed")
				return "absolute town seed is not an authorized debug event";
			if (!IsBasisPoints(command.m_iTargetFIASupportBasisPoints)
				|| !IsBasisPoints(command.m_iTargetOccupierSupportBasisPoints)
				|| !IsBasisPoints(command.m_iTargetInvaderSupportBasisPoints)
				|| !IsValidPopulationTriple(
					command.m_iTargetInitialPopulation,
					command.m_iTargetRemainingPopulation,
					command.m_iTargetDestroyedPopulation))
				return "absolute town seed target is invalid";
			if (command.m_bPopulationScaled || command.m_iDurationSeconds != 0
				|| command.m_iReputationDelta != 0
				|| command.m_iHeatDelta != 0
				|| command.m_iPoliceDelta != 0
				|| command.m_iRoadblockDelta != 0
				|| !command.m_sAggressionFactionKey.IsEmpty()
				|| command.m_iAggressionDelta != 0)
				return "absolute town seed carries non-seed modifiers";
		}
		else if (command.m_iTargetFIASupportBasisPoints != 0
			|| command.m_iTargetOccupierSupportBasisPoints != 0
			|| command.m_iTargetInvaderSupportBasisPoints != 0
			|| command.m_iTargetInitialPopulation != 0
			|| command.m_iTargetRemainingPopulation != 0
			|| command.m_iTargetDestroyedPopulation != 0)
			return "non-seed town influence command carries absolute target fields";
#endif
		if (command.m_iPopulationDelta < -MAX_POPULATION_DELTA
			|| command.m_iPopulationDelta > MAX_POPULATION_DELTA)
			return "town population delta is invalid";
		return "";
	}

	protected string ValidateNewEventAdmission(
		HST_CampaignState state,
		HST_TownInfluenceCommand command,
		HST_TownInfluenceRecord record)
	{
		if (!state || !command || !record)
			return "new town influence admission authority is unavailable";
		if (command.m_iAggressionDelta != 0)
		{
			if (!m_Economy || !m_Strategic)
				return "town influence aggression authority is unavailable";
			HST_FactionPoolState aggressionPool;
			if (!FindUniqueFactionPool(
				state,
				command.m_sAggressionFactionKey,
				aggressionPool))
				return "town influence aggression target authority is not unique";
			if (aggressionPool.m_iAggression < 0)
				return "town influence aggression target authority is invalid";
			if (aggressionPool.m_iAggression
				> int.MAX - command.m_iAggressionDelta)
				return "town influence aggression delta would overflow authority";
			if (HasStrategicTownInfluenceSourceClaim(
				state,
				command.m_sEventId))
				return "town influence aggression strategic source is already claimed";
			string strategicIdentityFailure;
			if (!m_Strategic.CanBuildStrategicEventId(
				state,
				"town_influence",
				strategicIdentityFailure))
			{
				return "town influence aggression strategic receipt is unavailable: "
					+ strategicIdentityFailure;
			}
		}
		if (command.m_iDurationSeconds > 0
			&& state.m_iElapsedSeconds
				> int.MAX - command.m_iDurationSeconds)
			return "town influence expiry would overflow campaign time";
		if (record.m_iRevision >= MAX_MUTABLE_REVISION)
			return "town influence revision authority is exhausted";
		if (record.m_iInfluenceEventCount >= MAX_INFLUENCE_EVENTS
			|| record.m_iActiveInfluenceModifierCount >= MAX_INFLUENCE_EVENTS
			|| record.m_iExpiredInfluenceModifierCount >= MAX_INFLUENCE_EVENTS)
			return "town influence aggregate authority is exhausted";
		if (command.m_bPopulationScaled && record.m_iInitialPopulation <= 0)
			return "population-scaled influence requires positive initial population";
#ifdef ENABLE_DIAG
		if (command.m_bAbsoluteDebugSeed
			&& (command.m_iRawFIASupportDelta
					!= command.m_iTargetFIASupportBasisPoints
						- record.m_iFIASupportBasisPoints
				|| command.m_iRawOccupierSupportDelta
					!= command.m_iTargetOccupierSupportBasisPoints
						- record.m_iOccupierSupportBasisPoints
				|| command.m_iRawInvaderSupportDelta
					!= command.m_iTargetInvaderSupportBasisPoints
						- record.m_iInvaderSupportBasisPoints
				|| command.m_iPopulationDelta
					!= command.m_iTargetInitialPopulation
						- record.m_iInitialPopulation))
			return "absolute town seed delta is inconsistent";
		if (!command.m_bAbsoluteDebugSeed
			&& command.m_iPopulationDelta > 0
#else
		if (command.m_iPopulationDelta > 0
#endif
			&& (record.m_iInitialPopulation
					> MAX_TOWN_POPULATION - command.m_iPopulationDelta
				|| record.m_iRemainingPopulation
					> MAX_TOWN_POPULATION - command.m_iPopulationDelta))
			return "town population growth would overflow authority";
		return "";
	}

	protected bool FindUniqueFactionPool(
		HST_CampaignState state,
		string factionKey,
		out HST_FactionPoolState match)
	{
		match = null;
		if (!state || factionKey.IsEmpty())
			return false;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || pool.m_sFactionKey != factionKey)
				continue;
			if (match)
				return false;
			match = pool;
		}
		return match != null;
	}

	protected bool HasStrategicTownInfluenceSourceClaim(
		HST_CampaignState state,
		string influenceEventId)
	{
		if (!state || influenceEventId.IsEmpty())
			return false;
		foreach (HST_StrategicEventState strategic : state.m_aStrategicEvents)
		{
			if (strategic
				&& strategic.m_sSourceType == "town_influence"
				&& strategic.m_sSourceId == influenceEventId)
				return true;
		}
		return false;
	}

	protected bool ExactEventMatches(
		HST_TownInfluenceEventState eventState,
		HST_TownInfluenceCommand command)
	{
		// Contact/activity flags are insertion directives, not event payload:
		// replay never reapplies them. Reconcile is deliberately a retry directive
		// so an exact durable event may resume its still-pending ownership policy.
		if (!eventState || !command || !eventState.m_bApplied
			|| eventState.m_iContractVersion != EXACT_CONTRACT_VERSION)
			return false;
		bool durationMatches = eventState.m_iExpiresAtSecond == 0
			&& command.m_iDurationSeconds <= 0;
		if (command.m_iDurationSeconds > 0)
			durationMatches = eventState.m_iExpiresAtSecond
				== eventState.m_iCreatedAtSecond + command.m_iDurationSeconds;
		if (!durationMatches)
			return false;
		if (eventState.m_sZoneId != command.m_sTownId
			|| eventState.m_sKind != command.m_sEventKind
			|| eventState.m_sSourceId != command.m_sSourceId
			|| eventState.m_sReason != command.m_sReason)
			return false;
		if (eventState.m_iFIASupportDelta != command.m_iRawFIASupportDelta
			|| eventState.m_iOccupierSupportDelta != command.m_iRawOccupierSupportDelta
			|| eventState.m_iReputationDelta != command.m_iReputationDelta
			|| eventState.m_iHeatDelta != command.m_iHeatDelta)
			return false;
		if (eventState.m_iPopulationDelta != command.m_iPopulationDelta
			|| eventState.m_iPoliceDelta != command.m_iPoliceDelta
			|| eventState.m_iRoadblockDelta != command.m_iRoadblockDelta
			|| eventState.m_bPopulationScaled != command.m_bPopulationScaled
			|| eventState.m_iPopulationUsed <= 0)
			return false;
		if (eventState.m_sAggressionFactionKey
				!= command.m_sAggressionFactionKey
			|| eventState.m_iAggressionDelta != command.m_iAggressionDelta)
			return false;
#ifdef ENABLE_DIAG
		if (eventState.m_bAbsoluteDebugSeed
			!= command.m_bAbsoluteDebugSeed)
			return false;
		if (command.m_bAbsoluteDebugSeed
			&& (eventState.m_iFIABasisPointsAfter
					!= command.m_iTargetFIASupportBasisPoints
				|| eventState.m_iOccupierBasisPointsAfter
					!= command.m_iTargetOccupierSupportBasisPoints
				|| eventState.m_iInvaderBasisPointsAfter
					!= command.m_iTargetInvaderSupportBasisPoints
				|| eventState.m_iInitialPopulationAfter
					!= command.m_iTargetInitialPopulation
				|| eventState.m_iRemainingPopulationAfter
					!= command.m_iTargetRemainingPopulation
				|| eventState.m_iDestroyedPopulationAfter
					!= command.m_iTargetDestroyedPopulation))
			return false;
		if (command.m_bAbsoluteDebugSeed)
		{
			return eventState.m_iRequestedFIABasisPointDelta
					== command.m_iRawFIASupportDelta
				&& eventState.m_iRequestedOccupierBasisPointDelta
					== command.m_iRawOccupierSupportDelta
				&& eventState.m_iRequestedInvaderBasisPointDelta
					== command.m_iRawInvaderSupportDelta
				&& eventState.m_iEffectiveFIABasisPointDelta
					== command.m_iRawFIASupportDelta
				&& eventState.m_iEffectiveOccupierBasisPointDelta
					== command.m_iRawOccupierSupportDelta
				&& eventState.m_iEffectiveInvaderBasisPointDelta
					== command.m_iRawInvaderSupportDelta;
		}
#endif

		int requestedFIA = CalculateRequestedSupportDeltaBasisPoints(
			command.m_iRawFIASupportDelta);
		if (eventState.m_iRequestedFIABasisPointDelta != requestedFIA)
			return false;
		int requestedOccupier = CalculateRequestedSupportDeltaBasisPoints(
			command.m_iRawOccupierSupportDelta);
		int requestedInvader = CalculateRequestedSupportDeltaBasisPoints(
			command.m_iRawInvaderSupportDelta);
		bool directEnemyShape
			= eventState.m_iRequestedOccupierBasisPointDelta == requestedOccupier
			&& eventState.m_iRequestedInvaderBasisPointDelta == requestedInvader;
		bool redirectedEnemyShape
			= eventState.m_iRequestedOccupierBasisPointDelta == 0
			&& eventState.m_iRequestedInvaderBasisPointDelta
				== requestedInvader + requestedOccupier;
		if (!directEnemyShape && !redirectedEnemyShape)
			return false;
		if (eventState.m_iEffectiveFIABasisPointDelta
			!= CalculateEffectiveSupportDeltaBasisPoints(
				command.m_iRawFIASupportDelta,
				eventState.m_iPopulationUsed,
				command.m_bPopulationScaled))
			return false;
		int effectiveOccupierRaw = command.m_iRawOccupierSupportDelta;
		int effectiveInvaderRaw = command.m_iRawInvaderSupportDelta;
		if (redirectedEnemyShape)
		{
			effectiveInvaderRaw += effectiveOccupierRaw;
			effectiveOccupierRaw = 0;
		}
		return eventState.m_iEffectiveOccupierBasisPointDelta
				== CalculateEffectiveSupportDeltaBasisPoints(
					effectiveOccupierRaw,
					eventState.m_iPopulationUsed,
					command.m_bPopulationScaled)
			&& eventState.m_iEffectiveInvaderBasisPointDelta
				== CalculateEffectiveSupportDeltaBasisPoints(
					effectiveInvaderRaw,
					eventState.m_iPopulationUsed,
					command.m_bPopulationScaled);
	}

	protected void ApplyContactEvidence(
		HST_TownInfluenceRecord record,
		HST_CampaignState state,
		HST_TownInfluenceEventState eventState,
		bool resistanceActivity)
	{
		if (!record || !state || !eventState)
			return;
		if (!record.m_bContacted)
		{
			record.m_bContacted = true;
			record.m_iContactedAtSecond = state.m_iElapsedSeconds;
			record.m_sContactSourceId = eventState.m_sSourceId;
			if (record.m_sContactSourceId.IsEmpty())
				record.m_sContactSourceId = eventState.m_sEventId;
			record.m_sContactReason = eventState.m_sReason;
		}
		if (resistanceActivity)
			record.m_bResistanceActivityStarted = true;
	}

	protected void ApplyEventKindAggregates(
		HST_TownInfluenceRecord record,
		HST_TownInfluenceEventState eventState,
		int fiaApplied,
		int occupierApplied,
		int invaderApplied)
	{
		if (!record || !eventState)
			return;
		string kind = eventState.m_sKind;
		if (kind.Contains("radio") || kind.Contains("broadcast"))
		{
			record.m_iFIARadioBasisPoints = ClampBasisPoints(
				record.m_iFIARadioBasisPoints + fiaApplied);
			record.m_iOccupierRadioBasisPoints = ClampBasisPoints(
				record.m_iOccupierRadioBasisPoints + occupierApplied);
			record.m_iInvaderRadioBasisPoints = ClampBasisPoints(
				record.m_iInvaderRadioBasisPoints + invaderApplied);
		}
		if (kind.Contains("propaganda"))
		{
			record.m_iFIAPropagandaBasisPoints = ClampBasisPoints(
				record.m_iFIAPropagandaBasisPoints + fiaApplied);
			record.m_iOccupierPropagandaBasisPoints = ClampBasisPoints(
				record.m_iOccupierPropagandaBasisPoints + occupierApplied);
			record.m_iInvaderPropagandaBasisPoints = ClampBasisPoints(
				record.m_iInvaderPropagandaBasisPoints + invaderApplied);
		}
	}

	protected string ResolveEnemyOwnershipCandidate(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_TownInfluenceRecord record)
	{
		if (!preset || !record)
			return "";
		string occupier = preset.m_sOccupierFactionKey;
		string invader = preset.m_sInvaderFactionKey;
		if (occupier.IsEmpty())
			return invader;
		if (invader.IsEmpty() || invader == occupier)
			return occupier;
		if (record.m_iInvaderSupportBasisPoints
			> record.m_iOccupierSupportBasisPoints)
			return invader;
		if (record.m_iInvaderSupportBasisPoints
			< record.m_iOccupierSupportBasisPoints)
			return occupier;
		HST_ZoneState zone;
		if (state)
			zone = FindUniqueCanonicalTownZone(state, record.m_sTownId);
		if (zone && zone.m_sOwnerFactionKey == invader)
			return invader;
		return occupier;
	}

	protected bool HasDifferentUnresolvedTopLevelTransition(
		HST_CampaignState state,
		string allowedRequestId)
	{
		if (!state)
			return false;
		foreach (HST_OwnershipTransitionState transition : state.m_aOwnershipTransitions)
		{
			if (!transition || transition.m_bCompleted
				|| !transition.m_sProjectionParentRequestId.IsEmpty())
				continue;
			if (!allowedRequestId.IsEmpty()
				&& transition.m_sRequestId == allowedRequestId)
				continue;
			return true;
		}
		return false;
	}

	protected bool HasValidPendingOwnershipReceiptLink(
		HST_CampaignState state,
		HST_TownInfluenceRecord record)
	{
		if (!state || !record || record.m_sPendingOwnershipRequestId.IsEmpty())
			return true;
		HST_OwnershipTransitionState match;
		foreach (HST_OwnershipTransitionState transition : state.m_aOwnershipTransitions)
		{
			if (!transition
				|| transition.m_sRequestId != record.m_sPendingOwnershipRequestId)
				continue;
			if (match)
				return false;
			match = transition;
		}
		if (!match)
			return false;
		if (match.m_bCompleted)
		{
			HST_ZoneState completedZone = FindUniqueCanonicalTownZone(
				state,
				record.m_sTownId);
			if (!completedZone
				|| completedZone.m_sOwnerFactionKey != match.m_sNewOwnerFactionKey
				|| completedZone.m_iOwnershipRevision != match.m_iAppliedRevision
				|| completedZone.m_sLastOwnershipTransitionRequestId
					!= match.m_sRequestId)
				return false;
		}
		return match.m_iContractVersion
				== HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			&& !match.m_bQuarantined
			&& match.m_bValidated
			&& match.m_sZoneId == record.m_sTownId
			&& match.m_sNewOwnerFactionKey == record.m_sPendingOwnerFactionKey
			&& match.m_sCause == "political_support"
			&& match.m_sSourceType == "town_influence"
			&& ((!match.m_bCompleted
					&& (match.m_sStatus == "validated"
						|| match.m_sStatus == "applying"
						|| match.m_sStatus == "projecting"))
				|| (match.m_bCompleted && match.m_bOwnerApplied
					&& match.m_sStatus == "completed"));
	}

	protected bool FinalizeCompletedPendingOwnership(
		HST_CampaignState state,
		HST_TownInfluenceRecord record)
	{
		if (!state || !record)
			return false;
		if (record.m_sPendingOwnershipRequestId.IsEmpty())
			return true;
		HST_OwnershipTransitionState transition = state.FindOwnershipTransition(
			record.m_sPendingOwnershipRequestId);
		if (!transition || !transition.m_bCompleted)
			return true;
		HST_ZoneState zone = FindUniqueCanonicalTownZone(state, record.m_sTownId);
		if (!zone || transition.m_sZoneId != record.m_sTownId
			|| transition.m_sNewOwnerFactionKey != record.m_sPendingOwnerFactionKey
			|| zone.m_sOwnerFactionKey != record.m_sPendingOwnerFactionKey)
		{
			m_sLastAuthorityFailure
				= "completed political ownership receipt does not match pending town authority";
			return false;
		}
		record.m_sLastFlipOwnerFactionKey = transition.m_sNewOwnerFactionKey;
		record.m_iLastFlipSecond = Math.Max(
			0,
			Math.Min(state.m_iElapsedSeconds, transition.m_iCompletedAtSecond));
		record.m_iLastFlipOwnershipRevision = transition.m_iAppliedRevision;
		if (record.m_iLastFlipOwnershipRevision <= 0)
			record.m_iLastFlipOwnershipRevision = zone.m_iOwnershipRevision;
		record.m_iOwnershipCooldownUntilSecond = 0;
		record.m_sLastMutationId = transition.m_sRequestId;
		ClearPendingOwner(record);
		return true;
	}

	protected void ClearPendingOwner(HST_TownInfluenceRecord record)
	{
		if (!record)
			return;
		record.m_sPendingOwnerFactionKey = "";
		record.m_iPendingOwnerSinceSecond = 0;
		record.m_sPendingOwnershipRequestId = "";
	}

	protected HST_CivilianZoneState EnsureLegacyCivilianProjection(
		HST_CampaignState state,
		HST_TownInfluenceRecord record)
	{
		if (!state || !record)
			return null;
		HST_CivilianZoneState civilian = state.FindCivilianZone(record.m_sTownId);
		if (civilian)
			return civilian;
		civilian = new HST_CivilianZoneState();
		civilian.m_sZoneId = record.m_sTownId;
		civilian.m_iReputation = 50;
		state.m_aCivilianZones.Insert(civilian);
		return civilian;
	}

	protected bool FindUniqueEvent(
		HST_CampaignState state,
		string eventId,
		out HST_TownInfluenceEventState match)
	{
		match = null;
		if (!state || eventId.IsEmpty())
			return true;
		foreach (HST_TownInfluenceEventState eventState : state.m_aTownInfluenceEvents)
		{
			if (!eventState || eventState.m_sEventId != eventId)
				continue;
			if (match)
				return false;
			match = eventState;
		}
		return true;
	}

	protected bool HasAnyRecordForTown(HST_CampaignState state, string townId)
	{
		if (!state || townId.IsEmpty())
			return false;
		foreach (HST_TownInfluenceRecord record : state.m_aTownInfluenceRecords)
		{
			if (record && record.m_sTownId == townId)
				return true;
		}
		return false;
	}

	protected HST_ZoneState FindUniqueCanonicalTownZone(
		HST_CampaignState state,
		string townId)
	{
		if (!state || townId.IsEmpty())
			return null;
		HST_ZoneState match;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sZoneId != townId)
				continue;
			if (match)
				return null;
			match = zone;
		}
		if (!match || match.m_eType != HST_EZoneType.HST_ZONE_TOWN
			|| match.m_iOwnershipContractVersion
				!= HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			|| match.m_iOwnershipRevision <= 0
			|| !match.m_sOwnershipAuthorityFailure.IsEmpty())
			return null;
		return match;
	}

	protected bool IsExplicitContactEvent(string eventKind, string sourceId)
	{
		if (eventKind.IsEmpty() || eventKind == "radio_broadcast"
			|| eventKind == "security_pressure")
			return false;
		return eventKind == "support_change"
			|| eventKind.Contains("incident")
			|| eventKind.Contains("mission")
			|| eventKind.Contains("resistance")
			|| eventKind.Contains("player")
			|| eventKind.Contains("aid")
			|| eventKind.Contains("supplies")
			|| sourceId.Contains("player");
	}

	protected void IncrementRevision(HST_TownInfluenceRecord record)
	{
		if (!record)
			return;
		if (record.m_iRevision < int.MAX - 1)
			record.m_iRevision++;
	}

	protected bool FailAuthority(string failure)
	{
		m_sLastAuthorityFailure = failure;
		return false;
	}

	protected HST_TownInfluenceResult Reject(
		HST_TownInfluenceResult result,
		string failure)
	{
		result.m_sFailureReason = failure;
		return result;
	}

	protected bool IsRawSupportDelta(int value)
	{
		return value >= -MAX_RAW_SUPPORT_DELTA
			&& value <= MAX_RAW_SUPPORT_DELTA;
	}

	protected bool IsNonSupportDelta(int value)
	{
		return value >= -MAX_NON_SUPPORT_DELTA
			&& value <= MAX_NON_SUPPORT_DELTA;
	}

	protected bool IsBasisPoints(int value)
	{
		return value >= MIN_BASIS_POINTS && value <= MAX_BASIS_POINTS;
	}

	protected int ClampBasisPoints(int value)
	{
		return Math.Max(MIN_BASIS_POINTS, Math.Min(MAX_BASIS_POINTS, value));
	}

	protected int BasisPointsToPercent(int basisPoints)
	{
		return Math.Max(0, Math.Min(100, Math.Round(
			ClampBasisPoints(basisPoints) / 100.0)));
	}

	protected int ClampSignedPercent(int value)
	{
		return Math.Max(-100, Math.Min(100, value));
	}

	protected int SafeAddClamped(
		int current,
		int delta,
		int minimum,
		int maximum)
	{
		int boundedCurrent = Math.Max(minimum, Math.Min(maximum, current));
		if (delta > 0 && boundedCurrent > maximum - delta)
			return maximum;
		if (delta < 0 && boundedCurrent < minimum - delta)
			return minimum;
		return Math.Max(minimum, Math.Min(maximum, boundedCurrent + delta));
	}

	protected int SafePopulationSum(int remaining, int destroyed)
	{
		int boundedRemaining = Math.Max(0, remaining);
		int boundedDestroyed = Math.Max(0, destroyed);
		if (boundedRemaining > int.MAX - boundedDestroyed)
			return int.MAX;
		return boundedRemaining + boundedDestroyed;
	}

	protected string LimitText(string value, int maxCharacters)
	{
		if (maxCharacters <= 0 || value.IsEmpty())
			return "";
		if (value.Length() <= maxCharacters)
			return value;
		return value.Substring(0, maxCharacters);
	}
}
