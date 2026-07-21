// Schema-64 persistence boundary for canonical town influence. Migration reads
// legacy zone/civilian facts once, resolves their signed-support disagreement
// deterministically, and projects the resulting canonical record back to the
// compatibility fields. Current-schema restore never reapplies event effects.
class HST_TownInfluenceSaveValidationService
{
	static const int SCHEMA_VERSION = 64;
	static const int AGGRESSION_SCHEMA_VERSION = 65;
	static const int CONTRACT_VERSION = 1;
	static const int QUARANTINE_CONTRACT_VERSION = -64;
	static const int MIN_BASIS_POINTS = 0;
	static const int MAX_BASIS_POINTS = 10000;
	static const int MAX_ID_CHARACTERS = 160;
	static const int MAX_REASON_CHARACTERS = 192;
	static const string MIGRATION_CONFLICT_EVENT_ID = "migration_schema64_town_influence_conflict";
	protected ref map<string, bool> m_mInvalidCurrentEventTownIds
		= new map<string, bool>();

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		if (!saveData)
			return;
		EnsureArrays(saveData);
		if (restoredSchemaVersion < AGGRESSION_SCHEMA_VERSION)
			ClearPreSchema65AggressionEvidence(saveData);
		if (restoredSchemaVersion < SCHEMA_VERSION)
		{
			MigrateLegacyTownInfluence(saveData);
			ValidateCurrentTownInfluence(saveData);
			return;
		}
		ValidateCurrentTownInfluence(saveData);
	}

	void ValidateCurrentAuthorityBeforeOwnership(
		HST_CampaignSaveData saveData,
		int restoredSchemaVersion)
	{
		if (!saveData)
			return;
		EnsureArrays(saveData);
		if (restoredSchemaVersion < AGGRESSION_SCHEMA_VERSION)
			ClearPreSchema65AggressionEvidence(saveData);
		// Pre-schema-64 event rows deserialize with the new field defaults. Relabel
		// and materialize their canonical town records before schema-62 ownership
		// validates completed support receipts against this authority.
		if (restoredSchemaVersion < SCHEMA_VERSION)
			MigrateLegacyTownInfluence(saveData);
		m_mInvalidCurrentEventTownIds.Clear();
		ValidateCurrentInfluenceEvents(saveData);
		RemoveNullRecords(saveData.m_aTownInfluenceRecords);
		if (saveData.m_aTownInfluenceRecords.Count()
			> HST_TownInfluenceService.MAX_TOWN_RECORDS)
		{
			foreach (HST_TownInfluenceRecord overBoundRecord : saveData.m_aTownInfluenceRecords)
			{
				Quarantine(
					overBoundRecord,
					"schema64 town influence record bound exceeded");
			}
			while (saveData.m_aTownInfluenceRecords.Count()
				> HST_TownInfluenceService.MAX_TOWN_RECORDS)
			{
				saveData.m_aTownInfluenceRecords.Remove(
					saveData.m_aTownInfluenceRecords.Count() - 1);
			}
		}
		map<string, int> townIdCounts = new map<string, int>();
		foreach (HST_TownInfluenceRecord countedRecord : saveData.m_aTownInfluenceRecords)
		{
			if (!countedRecord)
				continue;
			int count;
			if (townIdCounts.Find(countedRecord.m_sTownId, count))
				townIdCounts.Set(countedRecord.m_sTownId, count + 1);
			else
				townIdCounts.Set(countedRecord.m_sTownId, 1);
		}
		foreach (HST_TownInfluenceRecord record : saveData.m_aTownInfluenceRecords)
		{
			if (!record || record.m_iContractVersion == QUARANTINE_CONTRACT_VERSION)
				continue;
			int duplicateCount;
			townIdCounts.Find(record.m_sTownId, duplicateCount);
			string failure;
			if (duplicateCount != 1
				|| !FindCuratedTown(saveData, record.m_sTownId)
				|| CountCivilianZones(saveData, record.m_sTownId) > 1
				|| !ValidateCanonicalRecord(
					record,
					saveData.m_iElapsedSeconds,
					failure)
				|| m_mInvalidCurrentEventTownIds.Contains(record.m_sTownId)
				|| !HasValidEventAggregateConsistency(saveData, record))
			{
				if (failure.IsEmpty())
					failure = "schema64 pre-ownership town authority is inconsistent";
				Quarantine(record, failure);
			}
		}
	}

	void ValidateAfterOwnership(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return;
		EnsureArrays(saveData);
		ValidateCurrentTownInfluence(saveData);
	}

	protected void MigrateLegacyTownInfluence(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return;
		MarkLegacyInfluenceEvents(saveData);
		saveData.m_aTownInfluenceRecords.Clear();
		map<string, bool> migratedTownIds = new map<string, bool>();
		int supportConflictCount;
		foreach (HST_ZoneState zone : saveData.m_aZones)
		{
			if (!IsCuratedTown(zone) || zone.m_sZoneId.IsEmpty())
				continue;
			HST_ZoneState uniqueTown = FindCuratedTown(saveData, zone.m_sZoneId);
			if (!uniqueTown || uniqueTown != zone
				|| CountCivilianZones(saveData, zone.m_sZoneId) > 1)
				continue;
			if (migratedTownIds.Contains(zone.m_sZoneId))
				continue;
			migratedTownIds.Set(zone.m_sZoneId, true);

			HST_CivilianZoneState civilian = FindCivilianZone(saveData, zone.m_sZoneId);
			bool hasValidCivilianPair = civilian
				&& civilian.m_iFIASupport >= 0 && civilian.m_iFIASupport <= 100
				&& civilian.m_iOccupierSupport >= 0 && civilian.m_iOccupierSupport <= 100;
			int legacyFIA = 50;
			int legacyOccupier = 50;
			if (hasValidCivilianPair)
			{
				legacyFIA = civilian.m_iFIASupport;
				legacyOccupier = civilian.m_iOccupierSupport;
			}
			int signedSupport = ClampSignedPercent(zone.m_iSupport);
			int canonicalFIA = legacyFIA;
			int canonicalOccupier = legacyOccupier;
			if (!hasValidCivilianPair)
			{
				ProjectSupportPair(
					50,
					50,
					signedSupport,
					canonicalFIA,
					canonicalOccupier);
			}
			else if (legacyFIA - legacyOccupier != signedSupport)
			{
				ProjectSupportPair(
					legacyFIA,
					legacyOccupier,
					signedSupport,
					canonicalFIA,
					canonicalOccupier);
				supportConflictCount++;
			}

			HST_TownInfluenceRecord record = new HST_TownInfluenceRecord();
			record.m_iContractVersion = CONTRACT_VERSION;
			record.m_sTownId = zone.m_sZoneId;
			record.m_iRevision = 1;
			record.m_iFIASupportBasisPoints = canonicalFIA * 100;
			record.m_iOccupierSupportBasisPoints = canonicalOccupier * 100;
			record.m_iInvaderSupportBasisPoints = 0;
			MigratePopulation(record, civilian);
			HST_TownInfluenceEventState latestEvent = FindLatestInfluenceEvent(
				saveData,
				zone.m_sZoneId);
			HST_TownInfluenceEventState contactEvent = FindFirstContactInfluenceEvent(
				saveData,
				zone.m_sZoneId);
			bool hasRealInfluenceEvent = contactEvent != null;
			record.m_bContacted = zone.m_bActive
				|| zone.m_iResistanceCaptureProgress > 0
				|| hasRealInfluenceEvent;
			record.m_bResistanceActivityStarted = zone.m_iResistanceCaptureProgress > 0
				|| hasRealInfluenceEvent;
			MigrateContactEvidence(record, zone, contactEvent, saveData.m_iElapsedSeconds);
			record.m_sHysteresisBand = ResolveHysteresisBand(
				record.m_iFIASupportBasisPoints);
			MigrateInfluenceAggregates(record, civilian, saveData, zone.m_sZoneId);
			MigrateLastInfluenceEvidence(record, civilian, latestEvent, saveData.m_iElapsedSeconds);
			record.m_sLastMutationId = record.m_sLastInfluenceEventId;
			record.m_sAuthorityFailure = "";
			saveData.m_aTownInfluenceRecords.Insert(record);
			ProjectLegacyFields(saveData, zone, record);
		}

		if (supportConflictCount > 0)
			RecordMigrationConflict(saveData, supportConflictCount);
	}

	protected void MarkLegacyInfluenceEvents(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return;
		foreach (HST_TownInfluenceEventState eventState : saveData.m_aTownInfluenceEvents)
		{
			if (!eventState)
				continue;
			eventState.m_iContractVersion = 0;
			eventState.m_iRequestedFIABasisPointDelta = 0;
			eventState.m_iRequestedOccupierBasisPointDelta = 0;
			eventState.m_iRequestedInvaderBasisPointDelta = 0;
			eventState.m_iEffectiveFIABasisPointDelta = 0;
			eventState.m_iEffectiveOccupierBasisPointDelta = 0;
			eventState.m_iEffectiveInvaderBasisPointDelta = 0;
			eventState.m_iPopulationUsed = 0;
			eventState.m_bPopulationScaled = false;
			eventState.m_iRecordRevisionBefore = 0;
			eventState.m_iRecordRevisionAfter = 0;
			eventState.m_iFIABasisPointsBefore = 0;
			eventState.m_iFIABasisPointsAfter = 0;
			eventState.m_iOccupierBasisPointsBefore = 0;
			eventState.m_iOccupierBasisPointsAfter = 0;
			eventState.m_iInvaderBasisPointsBefore = 0;
			eventState.m_iInvaderBasisPointsAfter = 0;
#ifdef ENABLE_DIAG
			eventState.m_bAbsoluteDebugSeed = false;
#endif
			eventState.m_iInitialPopulationBefore = 0;
			eventState.m_iInitialPopulationAfter = 0;
			eventState.m_iRemainingPopulationBefore = 0;
			eventState.m_iRemainingPopulationAfter = 0;
			eventState.m_iDestroyedPopulationBefore = 0;
			eventState.m_iDestroyedPopulationAfter = 0;
			eventState.m_sAggressionFactionKey = "";
			eventState.m_iAggressionDelta = 0;
			eventState.m_iAggressionBefore = 0;
			eventState.m_iAggressionAfter = 0;
		}
	}

	protected void ClearPreSchema65AggressionEvidence(
		HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return;
		foreach (HST_TownInfluenceEventState eventState : saveData.m_aTownInfluenceEvents)
		{
			if (!eventState)
				continue;
			eventState.m_sAggressionFactionKey = "";
			eventState.m_iAggressionDelta = 0;
			eventState.m_iAggressionBefore = 0;
			eventState.m_iAggressionAfter = 0;
		}
	}

	protected void ValidateCurrentTownInfluence(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return;
		m_mInvalidCurrentEventTownIds.Clear();
		ValidateCurrentInfluenceEvents(saveData);
		RemoveNullRecords(saveData.m_aTownInfluenceRecords);
		if (saveData.m_aTownInfluenceRecords.Count()
			> HST_TownInfluenceService.MAX_TOWN_RECORDS)
		{
			foreach (HST_TownInfluenceRecord overBoundRecord : saveData.m_aTownInfluenceRecords)
			{
				Quarantine(
					overBoundRecord,
					"schema64 town influence record bound exceeded");
			}
			while (saveData.m_aTownInfluenceRecords.Count()
				> HST_TownInfluenceService.MAX_TOWN_RECORDS)
			{
				saveData.m_aTownInfluenceRecords.Remove(
					saveData.m_aTownInfluenceRecords.Count() - 1);
			}
		}
		map<string, int> townIdCounts = new map<string, int>();
		foreach (HST_TownInfluenceRecord countedRecord : saveData.m_aTownInfluenceRecords)
		{
			if (!countedRecord)
				continue;
			int count;
			if (townIdCounts.Find(countedRecord.m_sTownId, count))
				townIdCounts.Set(countedRecord.m_sTownId, count + 1);
			else
				townIdCounts.Set(countedRecord.m_sTownId, 1);
		}

		foreach (HST_TownInfluenceRecord record : saveData.m_aTownInfluenceRecords)
		{
			if (!record)
				continue;
			int duplicateCount;
			townIdCounts.Find(record.m_sTownId, duplicateCount);
			if (duplicateCount > 1)
			{
				Quarantine(record, "schema64 duplicate town influence authority");
				continue;
			}
			if (record.m_iContractVersion == QUARANTINE_CONTRACT_VERSION)
			{
				if (record.m_sAuthorityFailure.IsEmpty())
					record.m_sAuthorityFailure = "schema64 quarantined town influence authority";
				continue;
			}

			HST_ZoneState zone = FindCuratedTown(saveData, record.m_sTownId);
			if (!zone)
			{
				Quarantine(record, "schema64 town id does not resolve to a curated town");
				continue;
			}
			if (CountCivilianZones(saveData, record.m_sTownId) > 1)
			{
				Quarantine(record, "schema64 duplicate town compatibility projection");
				continue;
			}
			string failure;
			if (!ValidateCanonicalRecord(record, saveData.m_iElapsedSeconds, failure))
			{
				Quarantine(record, failure);
				continue;
			}
			if (m_mInvalidCurrentEventTownIds.Contains(record.m_sTownId))
			{
				Quarantine(record, "schema64 town has quarantined influence event authority");
				continue;
			}
			if (!HasValidPendingOwnershipReceipt(saveData, record))
			{
				Quarantine(record, "schema64 pending political ownership receipt is invalid");
				continue;
			}
			if (!HasValidEventAggregateConsistency(saveData, record))
			{
				Quarantine(record, "schema64 influence event aggregate or revision is inconsistent");
				continue;
			}
			ProjectLegacyFields(saveData, zone, record);
		}

		map<string, bool> visitedTownIds = new map<string, bool>();
		foreach (HST_ZoneState curatedTown : saveData.m_aZones)
		{
			if (!IsCuratedTown(curatedTown) || curatedTown.m_sZoneId.IsEmpty())
				continue;
			if (visitedTownIds.Contains(curatedTown.m_sZoneId))
				continue;
			visitedTownIds.Set(curatedTown.m_sZoneId, true);
			if (HasTownInfluenceRecord(saveData, curatedTown.m_sZoneId))
				continue;
			if (saveData.m_aTownInfluenceRecords.Count()
				>= HST_TownInfluenceService.MAX_TOWN_RECORDS)
				break;

			HST_TownInfluenceRecord placeholder = new HST_TownInfluenceRecord();
			placeholder.m_iContractVersion = QUARANTINE_CONTRACT_VERSION;
			placeholder.m_sTownId = curatedTown.m_sZoneId;
			placeholder.m_iRevision = 1;
			placeholder.m_sHysteresisBand = "neutral";
			placeholder.m_sAuthorityFailure = "schema64 missing canonical town influence authority";
			saveData.m_aTownInfluenceRecords.Insert(placeholder);
		}
	}

	protected bool ValidateCanonicalRecord(
		HST_TownInfluenceRecord record,
		int elapsedSecond,
		out string failure)
	{
		failure = "";
		if (!record)
		{
			failure = "schema64 null town influence authority";
			return false;
		}
		if (record.m_iContractVersion != CONTRACT_VERSION)
		{
			failure = "schema64 unsupported town influence contract";
			return false;
		}
		if (!IsValidId(record.m_sTownId))
		{
			failure = "schema64 invalid town influence id";
			return false;
		}
		if (record.m_iRevision <= 0
			|| record.m_iRevision > HST_TownInfluenceService.MAX_MUTABLE_REVISION)
		{
			failure = "schema64 invalid town influence revision";
			return false;
		}
		if (!HasBoundedBasisPoints(record))
		{
			failure = "schema64 town influence basis points out of bounds";
			return false;
		}
		if (!HasValidPopulation(record))
		{
			failure = "schema64 invalid town population authority";
			return false;
		}
		if (record.m_bResistanceActivityStarted && !record.m_bContacted)
		{
			failure = "schema64 activity-started town is not contacted";
			return false;
		}
		if (!HasValidContactEvidence(record, Math.Max(0, elapsedSecond)))
		{
			failure = "schema64 invalid town contact evidence";
			return false;
		}
		if (!HasValidInfluenceAggregates(record, Math.Max(0, elapsedSecond)))
		{
			failure = "schema64 invalid town influence aggregates";
			return false;
		}
		if (!HasValidHysteresis(record, Math.Max(0, elapsedSecond)))
		{
			failure = "schema64 invalid town ownership hysteresis";
			return false;
		}
		if (!HasValidEventEvidence(record, Math.Max(0, elapsedSecond)))
		{
			failure = "schema64 invalid town influence event evidence";
			return false;
		}
		if (!record.m_sLastMutationId.IsEmpty() && !IsValidId(record.m_sLastMutationId))
		{
			failure = "schema64 invalid town influence mutation id";
			return false;
		}
		if (!record.m_sAuthorityFailure.IsEmpty())
		{
			failure = "schema64 canonical town influence row carries authority failure";
			return false;
		}
		return true;
	}

	protected void ValidateCurrentInfluenceEvents(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return;
		for (int nullIndex = saveData.m_aTownInfluenceEvents.Count() - 1; nullIndex >= 0; nullIndex--)
		{
			if (!saveData.m_aTownInfluenceEvents[nullIndex])
				saveData.m_aTownInfluenceEvents.Remove(nullIndex);
		}
		if (saveData.m_aTownInfluenceEvents.Count()
			> HST_TownInfluenceService.MAX_INFLUENCE_EVENTS)
		{
			foreach (HST_TownInfluenceEventState overBoundEvent : saveData.m_aTownInfluenceEvents)
			{
				if (overBoundEvent && !overBoundEvent.m_sZoneId.IsEmpty())
					m_mInvalidCurrentEventTownIds.Set(
						overBoundEvent.m_sZoneId,
						true);
			}
			while (saveData.m_aTownInfluenceEvents.Count()
				> HST_TownInfluenceService.MAX_INFLUENCE_EVENTS)
			{
				saveData.m_aTownInfluenceEvents.Remove(
					saveData.m_aTownInfluenceEvents.Count() - 1);
			}
		}

		map<string, int> eventIdCounts = new map<string, int>();
		foreach (HST_TownInfluenceEventState countedEvent : saveData.m_aTownInfluenceEvents)
		{
			if (!countedEvent)
				continue;
			int count;
			if (eventIdCounts.Find(countedEvent.m_sEventId, count))
				eventIdCounts.Set(countedEvent.m_sEventId, count + 1);
			else
				eventIdCounts.Set(countedEvent.m_sEventId, 1);
		}
		map<string, ref HST_StrategicEventState> strategicByInfluenceSourceId
			= new map<string, ref HST_StrategicEventState>();
		map<string, int> strategicInfluenceSourceIdCounts
			= new map<string, int>();
		map<string, int> strategicEventIdCounts = new map<string, int>();
		BuildAggressionStrategicReceiptIndex(
			saveData,
			strategicByInfluenceSourceId,
			strategicInfluenceSourceIdCounts,
			strategicEventIdCounts);

		foreach (HST_TownInfluenceEventState eventState : saveData.m_aTownInfluenceEvents)
		{
			if (!eventState)
				continue;
			bool valid;
			int duplicateCount;
			eventIdCounts.Find(eventState.m_sEventId, duplicateCount);
			if (duplicateCount == 1)
			{
				if (eventState.m_iContractVersion == CONTRACT_VERSION)
				{
					valid = IsValidCurrentExactEvent(
						saveData,
						eventState,
						strategicByInfluenceSourceId,
						strategicInfluenceSourceIdCounts,
						strategicEventIdCounts);
				}
				else if (eventState.m_iContractVersion == 0)
					valid = IsValidCurrentLegacyEvent(saveData, eventState);
			}
			if (valid)
				continue;

			if (!eventState.m_sZoneId.IsEmpty())
				m_mInvalidCurrentEventTownIds.Set(eventState.m_sZoneId, true);
			QuarantineCurrentEvent(eventState);
		}
	}

	protected bool IsValidCurrentLegacyEvent(
		HST_CampaignSaveData saveData,
		HST_TownInfluenceEventState eventState)
	{
		if (!saveData || !eventState || !eventState.m_bApplied
			|| !IsValidId(eventState.m_sEventId)
			|| !FindCuratedTown(saveData, eventState.m_sZoneId))
			return false;
		if (eventState.m_sKind.Trim().IsEmpty()
			|| eventState.m_sKind.Length() > MAX_ID_CHARACTERS
			|| eventState.m_sSourceId.Length() > MAX_ID_CHARACTERS
			|| eventState.m_sReason.Length() > MAX_REASON_CHARACTERS)
			return false;
		if (eventState.m_iCreatedAtSecond < 0
			|| eventState.m_iCreatedAtSecond > saveData.m_iElapsedSeconds)
			return false;
		if (!eventState.m_sAggressionFactionKey.IsEmpty()
			|| eventState.m_iAggressionDelta != 0
			|| eventState.m_iAggressionBefore != 0
			|| eventState.m_iAggressionAfter != 0)
			return false;
		return eventState.m_iExpiresAtSecond == 0
			|| eventState.m_iExpiresAtSecond > eventState.m_iCreatedAtSecond;
	}

	protected bool IsValidCurrentExactEvent(
		HST_CampaignSaveData saveData,
		HST_TownInfluenceEventState eventState,
		map<string, ref HST_StrategicEventState> strategicByInfluenceSourceId,
		map<string, int> strategicInfluenceSourceIdCounts,
		map<string, int> strategicEventIdCounts)
	{
		if (!saveData || !eventState || !eventState.m_bApplied
			|| !IsValidId(eventState.m_sEventId)
			|| !FindCuratedTown(saveData, eventState.m_sZoneId))
			return false;
		if (eventState.m_sEventId != eventState.m_sEventId.Trim()
			|| eventState.m_sKind != eventState.m_sKind.Trim()
			|| eventState.m_sSourceId != eventState.m_sSourceId.Trim()
			|| eventState.m_sReason != eventState.m_sReason.Trim()
			|| eventState.m_sKind.Trim().IsEmpty()
			|| eventState.m_sKind.Length() > MAX_ID_CHARACTERS
			|| eventState.m_sSourceId.Length() > MAX_ID_CHARACTERS
			|| eventState.m_sReason.Trim().IsEmpty()
			|| eventState.m_sReason.Length() > MAX_REASON_CHARACTERS)
			return false;
		if (eventState.m_iCreatedAtSecond < 0
			|| eventState.m_iCreatedAtSecond > saveData.m_iElapsedSeconds)
			return false;
		if (eventState.m_iExpiresAtSecond != 0
			&& eventState.m_iExpiresAtSecond <= eventState.m_iCreatedAtSecond)
			return false;
		if (eventState.m_iExpiresAtSecond > 0
			&& eventState.m_iExpiresAtSecond - eventState.m_iCreatedAtSecond
				> HST_TownInfluenceService.MAX_DURATION_SECONDS)
			return false;
		if (!IsBoundedRawSupportDelta(eventState.m_iFIASupportDelta)
			|| !IsBoundedRawSupportDelta(eventState.m_iOccupierSupportDelta)
			|| !IsBoundedNonSupportDelta(eventState.m_iReputationDelta)
			|| !IsBoundedNonSupportDelta(eventState.m_iHeatDelta)
			|| !IsBoundedNonSupportDelta(eventState.m_iPoliceDelta)
			|| !IsBoundedNonSupportDelta(eventState.m_iRoadblockDelta)
			|| eventState.m_iPopulationDelta < -HST_TownInfluenceService.MAX_POPULATION_DELTA
			|| eventState.m_iPopulationDelta > HST_TownInfluenceService.MAX_POPULATION_DELTA)
			return false;
		if (!HasValidAggressionEventShape(
			saveData,
			eventState,
			strategicByInfluenceSourceId,
			strategicInfluenceSourceIdCounts,
			strategicEventIdCounts))
			return false;
		if (eventState.m_iPopulationUsed <= 0
			|| eventState.m_iPopulationUsed
				> HST_TownInfluenceService.MAX_TOWN_POPULATION
			|| (eventState.m_iPopulationDelta > 0
				&& eventState.m_iPopulationUsed
					> HST_TownInfluenceService.MAX_TOWN_POPULATION
						- eventState.m_iPopulationDelta)
			|| eventState.m_iRecordRevisionBefore <= 0
			|| eventState.m_iRecordRevisionBefore
				>= HST_TownInfluenceService.MAX_MUTABLE_REVISION
			|| eventState.m_iRecordRevisionAfter
				!= eventState.m_iRecordRevisionBefore + 1)
			return false;
		if (eventState.m_iPopulationUsed
				!= eventState.m_iInitialPopulationBefore
			|| !IsValidPopulationTriple(
				eventState.m_iInitialPopulationBefore,
				eventState.m_iRemainingPopulationBefore,
				eventState.m_iDestroyedPopulationBefore)
			|| !IsValidPopulationTriple(
				eventState.m_iInitialPopulationAfter,
				eventState.m_iRemainingPopulationAfter,
				eventState.m_iDestroyedPopulationAfter))
			return false;
#ifdef ENABLE_DIAG
		if (eventState.m_bAbsoluteDebugSeed)
		{
			if (eventState.m_sKind != "admin_debug_seed"
				|| eventState.m_iPopulationDelta
					!= eventState.m_iInitialPopulationAfter
						- eventState.m_iInitialPopulationBefore
				|| eventState.m_bPopulationScaled
				|| eventState.m_iExpiresAtSecond != 0
				|| eventState.m_iReputationDelta != 0
				|| eventState.m_iHeatDelta != 0
				|| eventState.m_iPoliceDelta != 0
				|| eventState.m_iRoadblockDelta != 0)
				return false;
		}
		else if (!HasExactPopulationTransition(eventState))
#else
		if (!HasExactPopulationTransition(eventState))
#endif
			return false;
		if (!HasValidSupportEventShape(eventState))
			return false;
		if (!IsBasisPoints(eventState.m_iFIABasisPointsBefore)
			|| !IsBasisPoints(eventState.m_iFIABasisPointsAfter)
			|| !IsBasisPoints(eventState.m_iOccupierBasisPointsBefore)
			|| !IsBasisPoints(eventState.m_iOccupierBasisPointsAfter)
			|| !IsBasisPoints(eventState.m_iInvaderBasisPointsBefore)
			|| !IsBasisPoints(eventState.m_iInvaderBasisPointsAfter))
			return false;
		return eventState.m_iFIABasisPointsAfter == ClampBasisPoints(
				eventState.m_iFIABasisPointsBefore
					+ eventState.m_iEffectiveFIABasisPointDelta)
			&& eventState.m_iOccupierBasisPointsAfter == ClampBasisPoints(
				eventState.m_iOccupierBasisPointsBefore
					+ eventState.m_iEffectiveOccupierBasisPointDelta)
			&& eventState.m_iInvaderBasisPointsAfter == ClampBasisPoints(
				eventState.m_iInvaderBasisPointsBefore
					+ eventState.m_iEffectiveInvaderBasisPointDelta);
	}

	protected bool HasValidAggressionEventShape(
		HST_CampaignSaveData saveData,
		HST_TownInfluenceEventState eventState,
		map<string, ref HST_StrategicEventState> strategicByInfluenceSourceId,
		map<string, int> strategicInfluenceSourceIdCounts,
		map<string, int> strategicEventIdCounts)
	{
		if (!saveData || !eventState)
			return false;
		if (eventState.m_sAggressionFactionKey
				!= eventState.m_sAggressionFactionKey.Trim()
			|| eventState.m_sAggressionFactionKey.Length()
				> MAX_ID_CHARACTERS
			|| eventState.m_iAggressionDelta < 0
			|| eventState.m_iAggressionDelta
				> HST_TownInfluenceService.MAX_AGGRESSION_DELTA)
			return false;
		bool hasTarget = !eventState.m_sAggressionFactionKey.IsEmpty();
		if (hasTarget != (eventState.m_iAggressionDelta > 0))
			return false;
		if (!hasTarget)
		{
			return eventState.m_iAggressionBefore == 0
				&& eventState.m_iAggressionAfter == 0;
		}
#ifdef ENABLE_DIAG
		if (eventState.m_bAbsoluteDebugSeed
			|| eventState.m_iAggressionBefore < 0
#else
		if (eventState.m_iAggressionBefore < 0
#endif
			|| eventState.m_iAggressionBefore
				> int.MAX - eventState.m_iAggressionDelta
			|| eventState.m_iAggressionAfter
				!= eventState.m_iAggressionBefore
					+ eventState.m_iAggressionDelta)
			return false;
		HST_FactionPoolState targetPool;
		if (!FindUniqueFactionPool(
			saveData,
			eventState.m_sAggressionFactionKey,
			targetPool)
			|| targetPool.m_iAggression < 0)
			return false;
		return HasExactAggressionStrategicReceipt(
			eventState,
			strategicByInfluenceSourceId,
			strategicInfluenceSourceIdCounts,
			strategicEventIdCounts);
	}

	protected bool FindUniqueFactionPool(
		HST_CampaignSaveData saveData,
		string factionKey,
		out HST_FactionPoolState match)
	{
		match = null;
		if (!saveData || factionKey.IsEmpty())
			return false;
		foreach (HST_FactionPoolState pool : saveData.m_aFactionPools)
		{
			if (!pool || pool.m_sFactionKey != factionKey)
				continue;
			if (match)
				return false;
			match = pool;
		}
		return match != null;
	}

	protected void BuildAggressionStrategicReceiptIndex(
		HST_CampaignSaveData saveData,
		map<string, ref HST_StrategicEventState> strategicByInfluenceSourceId,
		map<string, int> strategicInfluenceSourceIdCounts,
		map<string, int> strategicEventIdCounts)
	{
		if (!saveData || !strategicByInfluenceSourceId
			|| !strategicInfluenceSourceIdCounts || !strategicEventIdCounts)
			return;
		foreach (HST_StrategicEventState strategic : saveData.m_aStrategicEvents)
		{
			if (!strategic)
				continue;
			int eventIdCount;
			if (strategicEventIdCounts.Find(
				strategic.m_sEventId,
				eventIdCount))
			{
				strategicEventIdCounts.Set(strategic.m_sEventId, 2);
			}
			else
				strategicEventIdCounts.Set(strategic.m_sEventId, 1);

			if (strategic.m_sSourceType != "town_influence")
				continue;
			int sourceIdCount;
			if (strategicInfluenceSourceIdCounts.Find(
				strategic.m_sSourceId,
				sourceIdCount))
			{
				// A saturated duplicate marker is enough to fail every claimant
				// closed without allowing an attacker-sized counter to overflow.
				strategicInfluenceSourceIdCounts.Set(
					strategic.m_sSourceId,
					2);
				continue;
			}
			strategicInfluenceSourceIdCounts.Set(strategic.m_sSourceId, 1);
			strategicByInfluenceSourceId.Set(
				strategic.m_sSourceId,
				strategic);
		}
	}

	protected bool HasExactAggressionStrategicReceipt(
		HST_TownInfluenceEventState influenceEvent,
		map<string, ref HST_StrategicEventState> strategicByInfluenceSourceId,
		map<string, int> strategicInfluenceSourceIdCounts,
		map<string, int> strategicEventIdCounts)
	{
		if (!influenceEvent || !strategicByInfluenceSourceId
			|| !strategicInfluenceSourceIdCounts || !strategicEventIdCounts)
			return false;
		int sourceIdCount;
		if (!strategicInfluenceSourceIdCounts.Find(
			influenceEvent.m_sEventId,
			sourceIdCount)
			|| sourceIdCount != 1)
			return false;
		HST_StrategicEventState match
			= strategicByInfluenceSourceId.Get(influenceEvent.m_sEventId);
		int strategicEventIdCount;
		if (match)
			strategicEventIdCounts.Find(match.m_sEventId, strategicEventIdCount);
		return match
			&& strategicEventIdCount == 1
			&& IsValidId(match.m_sEventId)
			&& match.m_sKind == "town_influence"
			&& match.m_sTargetZoneId == influenceEvent.m_sZoneId
			&& match.m_sTargetFactionKey
				== influenceEvent.m_sAggressionFactionKey
			&& match.m_iCreatedAtSecond
				== influenceEvent.m_iCreatedAtSecond
			&& match.m_iAggressionDelta
				== influenceEvent.m_iAggressionDelta
			&& match.m_bApplied
			&& !match.m_sSummary.Trim().IsEmpty();
	}

	protected bool HasValidSupportEventShape(
		HST_TownInfluenceEventState eventState)
	{
		if (!eventState)
			return false;
#ifdef ENABLE_DIAG
		if (eventState.m_bAbsoluteDebugSeed)
		{
			return eventState.m_iRequestedFIABasisPointDelta
					== eventState.m_iFIASupportDelta
				&& eventState.m_iRequestedOccupierBasisPointDelta
					== eventState.m_iOccupierSupportDelta
				&& IsBoundedRawSupportDelta(
					eventState.m_iRequestedInvaderBasisPointDelta)
				&& eventState.m_iEffectiveFIABasisPointDelta
					== eventState.m_iRequestedFIABasisPointDelta
				&& eventState.m_iEffectiveOccupierBasisPointDelta
					== eventState.m_iRequestedOccupierBasisPointDelta
				&& eventState.m_iEffectiveInvaderBasisPointDelta
					== eventState.m_iRequestedInvaderBasisPointDelta;
		}
#endif
		if (eventState.m_iRequestedFIABasisPointDelta
			!= eventState.m_iFIASupportDelta * 100)
			return false;
		if (!IsBoundedRequestedBasisPointDelta(
				eventState.m_iRequestedOccupierBasisPointDelta)
			|| !IsBoundedRequestedBasisPointDelta(
				eventState.m_iRequestedInvaderBasisPointDelta))
			return false;
		if (eventState.m_iRequestedOccupierBasisPointDelta % 100 != 0
			|| eventState.m_iRequestedInvaderBasisPointDelta % 100 != 0)
			return false;
		int requestedOccupierRaw
			= eventState.m_iRequestedOccupierBasisPointDelta / 100;
		int requestedInvaderRaw
			= eventState.m_iRequestedInvaderBasisPointDelta / 100;
		bool directEnemyShape
			= requestedOccupierRaw == eventState.m_iOccupierSupportDelta
			&& IsBoundedRawSupportDelta(requestedInvaderRaw);
		bool redirectedEnemyShape = requestedOccupierRaw == 0
			&& IsBoundedRawSupportDelta(
				requestedInvaderRaw - eventState.m_iOccupierSupportDelta);
		if (!directEnemyShape && !redirectedEnemyShape)
			return false;
		if (eventState.m_iEffectiveFIABasisPointDelta
			!= CalculateEffectiveBasisPointDelta(
				eventState.m_iRequestedFIABasisPointDelta / 100,
				eventState.m_iPopulationUsed,
				eventState.m_bPopulationScaled))
			return false;
		if (eventState.m_iEffectiveOccupierBasisPointDelta
			!= CalculateEffectiveBasisPointDelta(
				eventState.m_iRequestedOccupierBasisPointDelta / 100,
				eventState.m_iPopulationUsed,
				eventState.m_bPopulationScaled))
			return false;
		return eventState.m_iEffectiveInvaderBasisPointDelta
			== CalculateEffectiveBasisPointDelta(
				eventState.m_iRequestedInvaderBasisPointDelta / 100,
				eventState.m_iPopulationUsed,
				eventState.m_bPopulationScaled);
	}

	protected bool HasValidPendingOwnershipReceipt(
		HST_CampaignSaveData saveData,
		HST_TownInfluenceRecord record)
	{
		if (!saveData || !record)
			return false;
		if (record.m_sPendingOwnershipRequestId.IsEmpty())
			return true;
		HST_OwnershipTransitionState match;
		foreach (HST_OwnershipTransitionState transition : saveData.m_aOwnershipTransitions)
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
			HST_ZoneState completedZone = FindCuratedTown(
				saveData,
				record.m_sTownId);
			if (!completedZone
				|| completedZone.m_sOwnerFactionKey != match.m_sNewOwnerFactionKey
				|| completedZone.m_iOwnershipRevision != match.m_iAppliedRevision
				|| completedZone.m_sLastOwnershipTransitionRequestId
					!= match.m_sRequestId)
				return false;
		}
		return match.m_iContractVersion == HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
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

	protected bool HasValidEventAggregateConsistency(
		HST_CampaignSaveData saveData,
		HST_TownInfluenceRecord record)
	{
		if (!saveData || !record)
			return false;
		int eventCount;
		int activeCount;
		int expiredCount;
		int nextExpirySecond;
		array<int> exactRevisions = {};
		map<int, ref HST_TownInfluenceEventState> exactByRevision
			= new map<int, ref HST_TownInfluenceEventState>();
		foreach (HST_TownInfluenceEventState eventState : saveData.m_aTownInfluenceEvents)
		{
			if (!eventState || !eventState.m_bApplied
				|| eventState.m_sZoneId != record.m_sTownId)
				continue;
			if (eventState.m_iContractVersion != 0
				&& eventState.m_iContractVersion != CONTRACT_VERSION)
				return false;
			eventCount++;
			if (eventState.m_iExpiresAtSecond > saveData.m_iElapsedSeconds)
			{
				activeCount++;
				if (nextExpirySecond <= 0
					|| eventState.m_iExpiresAtSecond < nextExpirySecond)
					nextExpirySecond = eventState.m_iExpiresAtSecond;
			}
			else if (eventState.m_iExpiresAtSecond > 0)
				expiredCount++;
			if (eventState.m_iContractVersion != CONTRACT_VERSION)
				continue;
			if (exactByRevision.Contains(eventState.m_iRecordRevisionAfter))
				return false;
			exactByRevision.Set(eventState.m_iRecordRevisionAfter, eventState);
			exactRevisions.Insert(eventState.m_iRecordRevisionAfter);
		}
		if (record.m_iInfluenceEventCount != eventCount
			|| record.m_iActiveInfluenceModifierCount != activeCount
			|| record.m_iExpiredInfluenceModifierCount != expiredCount)
			return false;
		if (nextExpirySecond > 0
			&& record.m_iNextInfluenceExpirySecond != nextExpirySecond)
			return false;
		if (nextExpirySecond == 0
			&& record.m_iNextInfluenceExpirySecond != 0)
			return false;
		if (exactRevisions.Count() == 0)
		{
			return record.m_iFIARadioBasisPoints == 0
				&& record.m_iOccupierRadioBasisPoints == 0
				&& record.m_iInvaderRadioBasisPoints == 0
				&& record.m_iFIAPropagandaBasisPoints == 0
				&& record.m_iOccupierPropagandaBasisPoints == 0
				&& record.m_iInvaderPropagandaBasisPoints == 0;
		}
		exactRevisions.Sort();
		HST_TownInfluenceEventState previousExact;
		HST_TownInfluenceEventState lastExact;
		int expectedFIARadio;
		int expectedOccupierRadio;
		int expectedInvaderRadio;
		int expectedFIAPropaganda;
		int expectedOccupierPropaganda;
		int expectedInvaderPropaganda;
		foreach (int exactRevision : exactRevisions)
		{
			HST_TownInfluenceEventState exactEvent;
			if (!exactByRevision.Find(exactRevision, exactEvent) || !exactEvent)
				return false;
			if (previousExact)
			{
				if (exactEvent.m_iRecordRevisionBefore
						< previousExact.m_iRecordRevisionAfter
					|| exactEvent.m_iCreatedAtSecond
						< previousExact.m_iCreatedAtSecond)
					return false;
				if (exactEvent.m_iFIABasisPointsBefore
						!= previousExact.m_iFIABasisPointsAfter
					|| exactEvent.m_iOccupierBasisPointsBefore
						!= previousExact.m_iOccupierBasisPointsAfter
					|| exactEvent.m_iInvaderBasisPointsBefore
						!= previousExact.m_iInvaderBasisPointsAfter)
					return false;
				if (exactEvent.m_iInitialPopulationBefore
						!= previousExact.m_iInitialPopulationAfter
					|| exactEvent.m_iRemainingPopulationBefore
						!= previousExact.m_iRemainingPopulationAfter
					|| exactEvent.m_iDestroyedPopulationBefore
						!= previousExact.m_iDestroyedPopulationAfter)
					return false;
			}
			int fiaApplied = exactEvent.m_iFIABasisPointsAfter
				- exactEvent.m_iFIABasisPointsBefore;
			int occupierApplied = exactEvent.m_iOccupierBasisPointsAfter
				- exactEvent.m_iOccupierBasisPointsBefore;
			int invaderApplied = exactEvent.m_iInvaderBasisPointsAfter
				- exactEvent.m_iInvaderBasisPointsBefore;
			if (exactEvent.m_sKind.Contains("radio")
				|| exactEvent.m_sKind.Contains("broadcast"))
			{
				expectedFIARadio = ClampBasisPoints(expectedFIARadio + fiaApplied);
				expectedOccupierRadio = ClampBasisPoints(
					expectedOccupierRadio + occupierApplied);
				expectedInvaderRadio = ClampBasisPoints(
					expectedInvaderRadio + invaderApplied);
			}
			if (exactEvent.m_sKind.Contains("propaganda"))
			{
				expectedFIAPropaganda = ClampBasisPoints(
					expectedFIAPropaganda + fiaApplied);
				expectedOccupierPropaganda = ClampBasisPoints(
					expectedOccupierPropaganda + occupierApplied);
				expectedInvaderPropaganda = ClampBasisPoints(
					expectedInvaderPropaganda + invaderApplied);
			}
			previousExact = exactEvent;
			lastExact = exactEvent;
		}
		if (record.m_iFIARadioBasisPoints != expectedFIARadio
			|| record.m_iOccupierRadioBasisPoints != expectedOccupierRadio
			|| record.m_iInvaderRadioBasisPoints != expectedInvaderRadio
			|| record.m_iFIAPropagandaBasisPoints != expectedFIAPropaganda
			|| record.m_iOccupierPropagandaBasisPoints
				!= expectedOccupierPropaganda
			|| record.m_iInvaderPropagandaBasisPoints
				!= expectedInvaderPropaganda)
			return false;
		if (record.m_iRevision < lastExact.m_iRecordRevisionAfter
			|| record.m_sLastInfluenceEventId != lastExact.m_sEventId
			|| record.m_sLastInfluenceEventKind != lastExact.m_sKind
			|| record.m_sLastInfluenceEventReason != lastExact.m_sReason
			|| record.m_iLastInfluenceEventSecond != lastExact.m_iCreatedAtSecond)
			return false;
		if (record.m_iFIASupportBasisPoints != lastExact.m_iFIABasisPointsAfter
			|| record.m_iOccupierSupportBasisPoints
				!= lastExact.m_iOccupierBasisPointsAfter
			|| record.m_iInvaderSupportBasisPoints
				!= lastExact.m_iInvaderBasisPointsAfter)
			return false;
		return record.m_iInitialPopulation
				== lastExact.m_iInitialPopulationAfter
			&& record.m_iRemainingPopulation
				== lastExact.m_iRemainingPopulationAfter
			&& record.m_iDestroyedPopulation
				== lastExact.m_iDestroyedPopulationAfter;
	}

	protected void QuarantineCurrentEvent(HST_TownInfluenceEventState eventState)
	{
		if (!eventState)
			return;
		eventState.m_iContractVersion = QUARANTINE_CONTRACT_VERSION;
		eventState.m_bApplied = false;
	}

	protected bool HasBoundedBasisPoints(HST_TownInfluenceRecord record)
	{
		if (!record)
			return false;
		return IsBasisPoints(record.m_iFIASupportBasisPoints)
			&& IsBasisPoints(record.m_iOccupierSupportBasisPoints)
			&& IsBasisPoints(record.m_iInvaderSupportBasisPoints)
			&& IsBasisPoints(record.m_iFIARadioBasisPoints)
			&& IsBasisPoints(record.m_iOccupierRadioBasisPoints)
			&& IsBasisPoints(record.m_iInvaderRadioBasisPoints)
			&& IsBasisPoints(record.m_iFIAPropagandaBasisPoints)
			&& IsBasisPoints(record.m_iOccupierPropagandaBasisPoints)
			&& IsBasisPoints(record.m_iInvaderPropagandaBasisPoints);
	}

	protected bool HasValidPopulation(HST_TownInfluenceRecord record)
	{
		return record && IsValidPopulationTriple(
			record.m_iInitialPopulation,
			record.m_iRemainingPopulation,
			record.m_iDestroyedPopulation);
	}

	protected bool IsValidPopulationTriple(
		int initialPopulation,
		int remainingPopulation,
		int destroyedPopulation)
	{
		if (initialPopulation <= 0
			|| initialPopulation > HST_TownInfluenceService.MAX_TOWN_POPULATION)
			return false;
		if (remainingPopulation < 0 || remainingPopulation > initialPopulation)
			return false;
		return destroyedPopulation >= 0
			&& destroyedPopulation == initialPopulation - remainingPopulation;
	}

	protected bool HasExactPopulationTransition(
		HST_TownInfluenceEventState eventState)
	{
		if (!eventState)
			return false;
		int expectedInitial = eventState.m_iInitialPopulationBefore;
		int expectedRemaining = eventState.m_iRemainingPopulationBefore;
		int expectedDestroyed = eventState.m_iDestroyedPopulationBefore;
		if (eventState.m_iPopulationDelta < 0)
		{
			int destroyed = Math.Min(
				expectedRemaining,
				-eventState.m_iPopulationDelta);
			expectedRemaining -= destroyed;
			expectedDestroyed += destroyed;
		}
		else if (eventState.m_iPopulationDelta > 0)
		{
			expectedInitial += eventState.m_iPopulationDelta;
			expectedRemaining += eventState.m_iPopulationDelta;
		}
		if (eventState.m_iInitialPopulationAfter != expectedInitial)
			return false;
		return eventState.m_iRemainingPopulationAfter == expectedRemaining
			&& eventState.m_iDestroyedPopulationAfter == expectedDestroyed;
	}

	protected bool HasValidHysteresis(HST_TownInfluenceRecord record, int elapsedSecond)
	{
		if (!record || !IsKnownHysteresisBand(record.m_sHysteresisBand))
			return false;
		if (record.m_sHysteresisBand
			!= ResolveHysteresisBand(record.m_iFIASupportBasisPoints))
			return false;
		if (record.m_iPendingOwnerSinceSecond < 0
			|| record.m_iOwnershipCooldownUntilSecond < 0
			|| record.m_iLastHysteresisEvaluationSecond < 0
			|| record.m_iLastFlipSecond < 0
			|| record.m_iLastFlipOwnershipRevision < 0)
			return false;
		if (record.m_iPendingOwnerSinceSecond > elapsedSecond
			|| record.m_iLastHysteresisEvaluationSecond > elapsedSecond
			|| record.m_iLastFlipSecond > elapsedSecond)
			return false;
		if (record.m_sPendingOwnerFactionKey.IsEmpty())
		{
			if (record.m_iPendingOwnerSinceSecond != 0)
				return false;
		}
		else if (!IsValidId(record.m_sPendingOwnerFactionKey))
			return false;
		if (!record.m_sPendingOwnershipRequestId.IsEmpty())
		{
			if (record.m_sPendingOwnerFactionKey.IsEmpty()
				|| !IsValidId(record.m_sPendingOwnershipRequestId))
				return false;
		}
		if (record.m_sLastFlipOwnerFactionKey.IsEmpty())
			return record.m_iLastFlipSecond == 0
				&& record.m_iLastFlipOwnershipRevision == 0;
		return IsValidId(record.m_sLastFlipOwnerFactionKey)
			&& record.m_iLastFlipOwnershipRevision > 0;
	}

	protected bool HasValidContactEvidence(HST_TownInfluenceRecord record, int elapsedSecond)
	{
		if (!record || record.m_iContactedAtSecond < 0
			|| record.m_iContactedAtSecond > elapsedSecond)
			return false;
		if (!record.m_bContacted)
		{
			return record.m_iContactedAtSecond == 0
				&& record.m_sContactSourceId.IsEmpty()
				&& record.m_sContactReason.IsEmpty();
		}
		return IsValidId(record.m_sContactSourceId)
			&& !record.m_sContactReason.Trim().IsEmpty()
			&& record.m_sContactReason.Length() <= MAX_REASON_CHARACTERS;
	}

	protected bool HasValidInfluenceAggregates(HST_TownInfluenceRecord record, int elapsedSecond)
	{
		if (!record || record.m_iInfluenceEventCount < 0
			|| record.m_iInfluenceEventCount > HST_TownInfluenceService.MAX_INFLUENCE_EVENTS
			|| record.m_iActiveInfluenceModifierCount < 0
			|| record.m_iActiveInfluenceModifierCount > HST_TownInfluenceService.MAX_INFLUENCE_EVENTS
			|| record.m_iExpiredInfluenceModifierCount < 0
			|| record.m_iExpiredInfluenceModifierCount > HST_TownInfluenceService.MAX_INFLUENCE_EVENTS
			|| record.m_iNextInfluenceExpirySecond < 0)
			return false;
		if (record.m_iActiveInfluenceModifierCount > record.m_iInfluenceEventCount)
			return false;
		if (record.m_iExpiredInfluenceModifierCount
			> record.m_iInfluenceEventCount - record.m_iActiveInfluenceModifierCount)
			return false;
		if (record.m_iNextInfluenceExpirySecond > 0
			&& record.m_iActiveInfluenceModifierCount <= 0)
			return false;
		return record.m_iNextInfluenceExpirySecond == 0
			|| record.m_iNextInfluenceExpirySecond > elapsedSecond;
	}

	protected bool HasValidEventEvidence(HST_TownInfluenceRecord record, int elapsedSecond)
	{
		if (!record || record.m_iLastInfluenceEventSecond < 0
			|| record.m_iLastInfluenceEventSecond > elapsedSecond)
			return false;
		if (record.m_sLastInfluenceEventId.IsEmpty())
		{
			return record.m_sLastInfluenceEventKind.IsEmpty()
				&& record.m_sLastInfluenceEventReason.IsEmpty()
				&& record.m_iLastInfluenceEventSecond == 0;
		}
		if (!IsValidId(record.m_sLastInfluenceEventId))
			return false;
		if (record.m_sLastInfluenceEventKind.Trim().IsEmpty()
			|| record.m_sLastInfluenceEventKind.Length() > MAX_ID_CHARACTERS)
			return false;
		return !record.m_sLastInfluenceEventReason.Trim().IsEmpty()
			&& record.m_sLastInfluenceEventReason.Length() <= MAX_REASON_CHARACTERS;
	}

	protected void MigratePopulation(
		HST_TownInfluenceRecord record,
		HST_CivilianZoneState civilian)
	{
		if (!record)
			return;
		int remaining;
		int destroyed;
		int civilianPresence;
		if (civilian)
		{
			remaining = Math.Max(0, civilian.m_iPopulationRemaining);
			destroyed = Math.Max(0, civilian.m_iPopulationKilled);
			civilianPresence = Math.Max(0, civilian.m_iCivilianPresence);
		}
		int initial = Math.Min(
			HST_TownInfluenceService.MAX_TOWN_POPULATION,
			SafePopulationSum(remaining, destroyed));
		remaining = Math.Min(initial, remaining);
		if (initial <= 0)
		{
			if (civilianPresence
				> HST_TownInfluenceService.MAX_TOWN_POPULATION / 8)
				initial = HST_TownInfluenceService.MAX_TOWN_POPULATION;
			else
				initial = Math.Max(20, civilianPresence * 8);
			remaining = initial;
		}
		record.m_iInitialPopulation = initial;
		record.m_iRemainingPopulation = remaining;
		record.m_iDestroyedPopulation = Math.Min(
			destroyed,
			Math.Max(0, initial - remaining));
	}

	protected void MigrateLastInfluenceEvidence(
		HST_TownInfluenceRecord record,
		HST_CivilianZoneState civilian,
		HST_TownInfluenceEventState latestEvent,
		int elapsedSecond)
	{
		if (!record)
			return;
		if (latestEvent)
		{
			record.m_sLastInfluenceEventId = LimitText(latestEvent.m_sEventId, MAX_ID_CHARACTERS);
			if (!IsValidId(record.m_sLastInfluenceEventId))
				record.m_sLastInfluenceEventId = BuildLegacyEvidenceId(
					record.m_sTownId,
					latestEvent.m_iCreatedAtSecond);
			record.m_sLastInfluenceEventKind = LimitText(latestEvent.m_sKind, MAX_ID_CHARACTERS);
			record.m_sLastInfluenceEventReason = LimitText(latestEvent.m_sReason, MAX_REASON_CHARACTERS);
			if (record.m_sLastInfluenceEventKind.Trim().IsEmpty())
				record.m_sLastInfluenceEventKind = "legacy_event";
			if (record.m_sLastInfluenceEventReason.Trim().IsEmpty())
				record.m_sLastInfluenceEventReason = "legacy influence event evidence";
			record.m_iLastInfluenceEventSecond = Math.Max(
				0,
				Math.Min(Math.Max(0, elapsedSecond), latestEvent.m_iCreatedAtSecond));
			return;
		}
		if (!civilian || civilian.m_sLastInfluenceEventId.Trim().IsEmpty())
			return;
		record.m_sLastInfluenceEventId = LimitText(civilian.m_sLastInfluenceEventId, MAX_ID_CHARACTERS);
		if (!IsValidId(record.m_sLastInfluenceEventId))
			record.m_sLastInfluenceEventId = BuildLegacyEvidenceId(
				record.m_sTownId,
				civilian.m_iLastInfluenceEventSecond);
		record.m_sLastInfluenceEventKind = LimitText(civilian.m_sLastInfluenceKind, MAX_ID_CHARACTERS);
		record.m_sLastInfluenceEventReason = LimitText(civilian.m_sLastInfluenceReason, MAX_REASON_CHARACTERS);
		if (record.m_sLastInfluenceEventKind.Trim().IsEmpty())
			record.m_sLastInfluenceEventKind = "legacy_event";
		if (record.m_sLastInfluenceEventReason.Trim().IsEmpty())
			record.m_sLastInfluenceEventReason = "legacy influence event evidence";
		record.m_iLastInfluenceEventSecond = Math.Max(
			0,
			Math.Min(Math.Max(0, elapsedSecond), civilian.m_iLastInfluenceEventSecond));
	}

	protected void MigrateContactEvidence(
		HST_TownInfluenceRecord record,
		HST_ZoneState zone,
		HST_TownInfluenceEventState contactEvent,
		int elapsedSecond)
	{
		if (!record || !record.m_bContacted || !zone)
			return;
		if (contactEvent)
		{
			record.m_iContactedAtSecond = Math.Max(
				0,
				Math.Min(Math.Max(0, elapsedSecond), contactEvent.m_iCreatedAtSecond));
			record.m_sContactSourceId = LimitText(contactEvent.m_sSourceId, MAX_ID_CHARACTERS);
			if (!IsValidId(record.m_sContactSourceId))
				record.m_sContactSourceId = LimitText(contactEvent.m_sEventId, MAX_ID_CHARACTERS);
			if (!IsValidId(record.m_sContactSourceId))
				record.m_sContactSourceId = BuildLegacyEvidenceId(
					record.m_sTownId,
					contactEvent.m_iCreatedAtSecond);
			record.m_sContactReason = LimitText(contactEvent.m_sReason, MAX_REASON_CHARACTERS);
			if (record.m_sContactReason.Trim().IsEmpty())
				record.m_sContactReason = "legacy town influence event contact";
			return;
		}
		record.m_iContactedAtSecond = 0;
		record.m_sContactSourceId = LimitText(zone.m_sZoneId, MAX_ID_CHARACTERS);
		if (zone.m_iResistanceCaptureProgress > 0)
			record.m_sContactReason = "legacy capture progress evidence";
		else
			record.m_sContactReason = "legacy active-zone contact evidence";
	}

	protected void MigrateInfluenceAggregates(
		HST_TownInfluenceRecord record,
		HST_CivilianZoneState civilian,
		HST_CampaignSaveData saveData,
		string townId)
	{
		if (!record || !saveData || townId.IsEmpty())
			return;
		int eventCount;
		int activeCount;
		int expiredCount;
		int nextExpirySecond;
		foreach (HST_TownInfluenceEventState eventState : saveData.m_aTownInfluenceEvents)
		{
			if (!eventState || !eventState.m_bApplied
				|| eventState.m_sZoneId != townId)
				continue;
			eventCount++;
			if (eventState.m_iExpiresAtSecond <= 0)
				continue;
			if (eventState.m_iExpiresAtSecond > saveData.m_iElapsedSeconds)
			{
				activeCount++;
				if (nextExpirySecond <= 0
					|| eventState.m_iExpiresAtSecond < nextExpirySecond)
					nextExpirySecond = eventState.m_iExpiresAtSecond;
			}
			else
				expiredCount++;
		}
		eventCount = Math.Max(eventCount, SafePopulationSum(activeCount, expiredCount));
		eventCount = Math.Min(
			HST_TownInfluenceService.MAX_INFLUENCE_EVENTS,
			eventCount);
		activeCount = Math.Min(eventCount, activeCount);
		expiredCount = Math.Min(
			Math.Max(0, eventCount - activeCount),
			expiredCount);
		record.m_iInfluenceEventCount = eventCount;
		record.m_iActiveInfluenceModifierCount = activeCount;
		record.m_iExpiredInfluenceModifierCount = expiredCount;
		record.m_iNextInfluenceExpirySecond = nextExpirySecond;
	}

	protected HST_TownInfluenceEventState FindLatestInfluenceEvent(
		HST_CampaignSaveData saveData,
		string townId)
	{
		HST_TownInfluenceEventState latest;
		if (!saveData || townId.IsEmpty())
			return latest;
		foreach (HST_TownInfluenceEventState eventState : saveData.m_aTownInfluenceEvents)
		{
			if (!eventState || eventState.m_sZoneId != townId
				|| eventState.m_sEventId.Trim().IsEmpty() || !eventState.m_bApplied)
				continue;
			if (!latest || eventState.m_iCreatedAtSecond > latest.m_iCreatedAtSecond
				|| (eventState.m_iCreatedAtSecond == latest.m_iCreatedAtSecond
					&& eventState.m_sEventId.Compare(latest.m_sEventId) > 0))
				latest = eventState;
		}
		return latest;
	}

	protected HST_TownInfluenceEventState FindFirstContactInfluenceEvent(
		HST_CampaignSaveData saveData,
		string townId)
	{
		HST_TownInfluenceEventState first;
		if (!saveData || townId.IsEmpty())
			return first;
		foreach (HST_TownInfluenceEventState eventState : saveData.m_aTownInfluenceEvents)
		{
			if (!eventState || eventState.m_sZoneId != townId
				|| eventState.m_sEventId.Trim().IsEmpty() || !eventState.m_bApplied)
				continue;
			if (!IsContactInfluenceKind(eventState.m_sKind))
				continue;
			if (!first || eventState.m_iCreatedAtSecond < first.m_iCreatedAtSecond
				|| (eventState.m_iCreatedAtSecond == first.m_iCreatedAtSecond
					&& eventState.m_sEventId.Compare(first.m_sEventId) < 0))
				first = eventState;
		}
		return first;
	}

	protected bool IsContactInfluenceKind(string kind)
	{
		if (kind.Trim().IsEmpty() || kind == "radio_broadcast" || kind == "security_pressure")
			return false;
		return kind != "initialized" && kind != "legacy/backfilled"
			&& kind != "legacy_backfilled" && kind != "legacy_backfill";
	}

	protected void ProjectLegacyFields(
		HST_CampaignSaveData saveData,
		HST_ZoneState zone,
		HST_TownInfluenceRecord record)
	{
		if (!saveData || !zone || !record || record.m_iContractVersion != CONTRACT_VERSION)
			return;
		HST_CivilianZoneState civilian = FindCivilianZone(saveData, record.m_sTownId);
		if (!civilian)
		{
			civilian = new HST_CivilianZoneState();
			civilian.m_sZoneId = record.m_sTownId;
			saveData.m_aCivilianZones.Insert(civilian);
		}
		civilian.m_iFIASupport = BasisPointsToPercent(record.m_iFIASupportBasisPoints);
		int enemySupportBasisPoints = Math.Max(
			record.m_iOccupierSupportBasisPoints,
			record.m_iInvaderSupportBasisPoints);
		civilian.m_iOccupierSupport = BasisPointsToPercent(enemySupportBasisPoints);
		civilian.m_iPopulationRemaining = record.m_iRemainingPopulation;
		civilian.m_iPopulationKilled = record.m_iDestroyedPopulation;
		civilian.m_iInfluenceEventCount = record.m_iInfluenceEventCount;
		civilian.m_iActiveInfluenceModifierCount = record.m_iActiveInfluenceModifierCount;
		civilian.m_iExpiredInfluenceModifierCount = record.m_iExpiredInfluenceModifierCount;
		civilian.m_sLastInfluenceEventId = record.m_sLastInfluenceEventId;
		civilian.m_sLastInfluenceKind = record.m_sLastInfluenceEventKind;
		civilian.m_sLastInfluenceReason = record.m_sLastInfluenceEventReason;
		civilian.m_iLastInfluenceEventSecond = record.m_iLastInfluenceEventSecond;
		zone.m_iSupport = ClampSignedPercent(
			civilian.m_iFIASupport - civilian.m_iOccupierSupport);
	}

	protected void ProjectSupportPair(
		int legacyFIA,
		int legacyOccupier,
		int signedSupport,
		out int canonicalFIA,
		out int canonicalOccupier)
	{
		int targetFIA = ClampPercent(legacyFIA);
		int targetOccupier = ClampPercent(legacyOccupier);
		canonicalFIA = targetFIA;
		canonicalOccupier = targetOccupier;
		int boundedSignedSupport = ClampSignedPercent(signedSupport);
		int minimumFIA = Math.Max(0, boundedSignedSupport);
		int maximumFIA = Math.Min(100, 100 + boundedSignedSupport);
		int bestScore = int.MAX;
		for (int candidateFIA = minimumFIA; candidateFIA <= maximumFIA; candidateFIA++)
		{
			int candidateOccupier = candidateFIA - boundedSignedSupport;
			int fiaDistance = candidateFIA - targetFIA;
			int occupierDistance = candidateOccupier - targetOccupier;
			int score = fiaDistance * fiaDistance + occupierDistance * occupierDistance;
			if (score >= bestScore)
				continue;
			bestScore = score;
			canonicalFIA = candidateFIA;
			canonicalOccupier = candidateOccupier;
		}
	}

	protected void RecordMigrationConflict(HST_CampaignSaveData saveData, int conflictCount)
	{
		if (!saveData || conflictCount <= 0 || HasCampaignEvent(saveData, MIGRATION_CONFLICT_EVENT_ID))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = MIGRATION_CONFLICT_EVENT_ID;
		eventState.m_sCategory = "migration";
		eventState.m_sAggregateType = "town_influence";
		eventState.m_sAggregateId = "schema64";
		eventState.m_sTransition = "legacy_support_conflicts_resolved";
		eventState.m_sReason = string.Format(
			"resolved %1 legacy town support conflict(s) with signed zone support precedence and minimal bounded pair adjustment",
			conflictCount);
		eventState.m_iCreatedAtSecond = Math.Max(0, saveData.m_iElapsedSeconds);
		saveData.m_aCampaignEvents.Insert(eventState);
	}

	protected void Quarantine(HST_TownInfluenceRecord record, string failure)
	{
		if (!record)
			return;
		bool alreadyQuarantined = record.m_iContractVersion == QUARANTINE_CONTRACT_VERSION;
		record.m_iContractVersion = QUARANTINE_CONTRACT_VERSION;
		if (record.m_sAuthorityFailure.IsEmpty())
			record.m_sAuthorityFailure = LimitText(failure, MAX_REASON_CHARACTERS);
		else if (!alreadyQuarantined)
			record.m_sAuthorityFailure = LimitText(
				record.m_sAuthorityFailure + " | " + failure,
				MAX_REASON_CHARACTERS);
	}

	protected void RemoveNullRecords(array<ref HST_TownInfluenceRecord> records)
	{
		if (!records)
			return;
		for (int index = records.Count() - 1; index >= 0; index--)
		{
			if (!records[index])
				records.Remove(index);
		}
	}

	protected HST_ZoneState FindCuratedTown(HST_CampaignSaveData saveData, string townId)
	{
		if (!saveData || townId.IsEmpty())
			return null;
		HST_ZoneState match;
		foreach (HST_ZoneState zone : saveData.m_aZones)
		{
			if (!zone || zone.m_sZoneId != townId)
				continue;
			if (match)
				return null;
			match = zone;
		}
		if (!IsCuratedTown(match)
			|| match.m_iOwnershipContractVersion
				!= HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			|| match.m_iOwnershipRevision <= 0
			|| !match.m_sOwnershipAuthorityFailure.IsEmpty())
			return null;
		return match;
	}

	protected HST_CivilianZoneState FindCivilianZone(HST_CampaignSaveData saveData, string townId)
	{
		if (!saveData || townId.IsEmpty())
			return null;
		HST_CivilianZoneState match;
		foreach (HST_CivilianZoneState civilian : saveData.m_aCivilianZones)
		{
			if (!civilian || civilian.m_sZoneId != townId)
				continue;
			if (match)
				return null;
			match = civilian;
		}
		return match;
	}

	protected int CountCivilianZones(HST_CampaignSaveData saveData, string townId)
	{
		if (!saveData || townId.IsEmpty())
			return 0;
		int count;
		foreach (HST_CivilianZoneState civilian : saveData.m_aCivilianZones)
		{
			if (civilian && civilian.m_sZoneId == townId)
				count++;
		}
		return count;
	}

	protected bool HasTownInfluenceRecord(HST_CampaignSaveData saveData, string townId)
	{
		if (!saveData || townId.IsEmpty())
			return false;
		foreach (HST_TownInfluenceRecord record : saveData.m_aTownInfluenceRecords)
		{
			if (record && record.m_sTownId == townId)
				return true;
		}
		return false;
	}

	protected bool HasCampaignEvent(HST_CampaignSaveData saveData, string eventId)
	{
		if (!saveData || eventId.IsEmpty())
			return false;
		foreach (HST_CampaignEventState eventState : saveData.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
	}

	protected bool IsCuratedTown(HST_ZoneState zone)
	{
		return zone && zone.m_eType == HST_EZoneType.HST_ZONE_TOWN;
	}

	protected bool IsKnownHysteresisBand(string band)
	{
		return band == "resistance" || band == "neutral" || band == "enemy";
	}

	protected string ResolveHysteresisBand(int fiaSupportBasisPoints)
	{
		if (fiaSupportBasisPoints
			> HST_TownInfluenceService.RESISTANCE_OWNERSHIP_THRESHOLD_BASIS_POINTS)
			return "resistance";
		if (fiaSupportBasisPoints
			< HST_TownInfluenceService.ENEMY_OWNERSHIP_THRESHOLD_BASIS_POINTS)
			return "enemy";
		return "neutral";
	}

	protected bool IsBoundedRawSupportDelta(int value)
	{
		return value >= -HST_TownInfluenceService.MAX_RAW_SUPPORT_DELTA
			&& value <= HST_TownInfluenceService.MAX_RAW_SUPPORT_DELTA;
	}

	protected bool IsBoundedNonSupportDelta(int value)
	{
		return value >= -HST_TownInfluenceService.MAX_NON_SUPPORT_DELTA
			&& value <= HST_TownInfluenceService.MAX_NON_SUPPORT_DELTA;
	}

	protected bool IsBoundedRequestedBasisPointDelta(int value)
	{
		int bound = HST_TownInfluenceService.MAX_RAW_SUPPORT_DELTA * 200;
		return value >= -bound && value <= bound;
	}

	protected int CalculateEffectiveBasisPointDelta(
		int rawDelta,
		int population,
		bool populationScaled)
	{
		if (!populationScaled)
			return Math.Round(rawDelta * 100.0);
		if (population <= 0)
			return 0;
		return Math.Round(1000.0 * rawDelta / Math.Sqrt(population));
	}

	protected bool IsBasisPoints(int value)
	{
		return value >= MIN_BASIS_POINTS && value <= MAX_BASIS_POINTS;
	}

	protected int ClampBasisPoints(int value)
	{
		return Math.Max(MIN_BASIS_POINTS, Math.Min(MAX_BASIS_POINTS, value));
	}

	protected bool IsValidId(string value)
	{
		return !value.IsEmpty() && value.Length() <= MAX_ID_CHARACTERS
			&& !value.Trim().IsEmpty();
	}

	protected string BuildLegacyEvidenceId(string townId, int eventSecond)
	{
		return string.Format(
			"schema64_legacy_%1_%2",
			townId.Hash(),
			Math.Max(0, eventSecond));
	}

	protected int SafePopulationSum(int remaining, int destroyed)
	{
		int boundedRemaining = Math.Max(0, remaining);
		int boundedDestroyed = Math.Max(0, destroyed);
		if (boundedRemaining > int.MAX - boundedDestroyed)
			return int.MAX;
		return boundedRemaining + boundedDestroyed;
	}

	protected int BasisPointsToPercent(int basisPoints)
	{
		return ClampPercent(Math.Round(Math.Max(0, Math.Min(10000, basisPoints)) / 100.0));
	}

	protected int ClampPercent(int value)
	{
		return Math.Max(0, Math.Min(100, value));
	}

	protected int ClampSignedPercent(int value)
	{
		return Math.Max(-100, Math.Min(100, value));
	}

	protected string LimitText(string value, int maxCharacters)
	{
		if (maxCharacters <= 0 || value.IsEmpty())
			return "";
		if (value.Length() <= maxCharacters)
			return value;
		return value.Substring(0, maxCharacters);
	}

	protected void EnsureArrays(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return;
		if (!saveData.m_aTownInfluenceRecords)
			saveData.m_aTownInfluenceRecords = new array<ref HST_TownInfluenceRecord>();
		if (!saveData.m_aTownInfluenceEvents)
			saveData.m_aTownInfluenceEvents = new array<ref HST_TownInfluenceEventState>();
		if (!saveData.m_aCivilianZones)
			saveData.m_aCivilianZones = new array<ref HST_CivilianZoneState>();
		if (!saveData.m_aCampaignEvents)
			saveData.m_aCampaignEvents = new array<ref HST_CampaignEventState>();
		if (!saveData.m_aFactionPools)
			saveData.m_aFactionPools = new array<ref HST_FactionPoolState>();
		if (!saveData.m_aStrategicEvents)
			saveData.m_aStrategicEvents = new array<ref HST_StrategicEventState>();
	}
}
