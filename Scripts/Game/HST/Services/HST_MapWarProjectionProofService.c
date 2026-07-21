#ifdef ENABLE_DIAG
class HST_MapWarProjectionProofReport
{
	bool m_bContactFilterExact;
	bool m_bCurrentTownFirstExact;
	bool m_bSupportSortExact;
	bool m_bTerritoryCompleteExact;
	bool m_bTerritoryOrderExact;
	bool m_bInvalidAuthorityExcluded;
	bool m_bDeferredPublicationExact;
	string m_sEvidence;

	bool AllExact()
	{
		return m_bContactFilterExact
			&& m_bCurrentTownFirstExact
			&& m_bSupportSortExact
			&& m_bTerritoryCompleteExact
			&& m_bTerritoryOrderExact
			&& m_bInvalidAuthorityExcluded
			&& m_bDeferredPublicationExact;
	}
}

class HST_MapWarProjectionProofService
{
	HST_MapWarProjectionProofReport Run()
	{
		HST_MapWarProjectionProofReport report = new HST_MapWarProjectionProofReport();
		HST_CampaignState state = BuildState();
		HST_MapWarProjectionService service = new HST_MapWarProjectionService();

		array<ref HST_ZonePressureProjectionRow> pressure = service.BuildZonePressure(
			state,
			"1000 0 1000");
		report.m_bContactFilterExact = pressure.Count() == 4
			&& !ContainsPressureTown(pressure, "town_hidden");
		report.m_bCurrentTownFirstExact = pressure.Count() >= 1
			&& pressure[0].m_sTownId == "town_current"
			&& pressure[0].m_bCurrentTown;
		report.m_bSupportSortExact = pressure.Count() == 4
			&& pressure[1].m_sTownId == "town_low"
			&& pressure[2].m_sTownId == "town_close"
			&& pressure[3].m_sTownId == "town_high";

		array<ref HST_ResistanceTerritoryProjectionRow> territory = service.BuildResistanceTerritory(
			state,
			"FIA");
		report.m_bTerritoryCompleteExact = territory.Count() == 5
			&& ContainsTerritoryZone(territory, "town_current")
			&& ContainsTerritoryZone(territory, "town_high")
			&& ContainsTerritoryZone(territory, "outpost_alpha")
			&& ContainsTerritoryZone(territory, "outpost_departing")
			&& ContainsTerritoryZone(territory, "resource_alpha")
			&& !ContainsTerritoryZone(territory, "town_low")
			&& !ContainsTerritoryZone(territory, "mission_bookkeeping")
			&& !ContainsTerritoryZone(territory, "outpost_pending")
			&& !ContainsTerritoryZone(territory, "outpost_orphan")
			&& !ContainsTerritoryZone(territory, "outpost_quarantined");
		report.m_bTerritoryOrderExact = territory.Count() == 5
			&& territory[0].m_sZoneId == "town_current"
			&& territory[1].m_sZoneId == "town_high"
			&& territory[2].m_sZoneId == "outpost_alpha"
			&& territory[3].m_sZoneId == "outpost_departing"
			&& territory[3].m_iOwnershipRevision == 1
			&& territory[4].m_sZoneId == "resource_alpha";

		HST_TownInfluenceRecord duplicate = BuildRecord("town_low", 1500, true);
		state.m_aTownInfluenceRecords.Insert(duplicate);
		array<ref HST_ZonePressureProjectionRow> duplicatePressure = service.BuildZonePressure(
			state,
			"1000 0 1000");
		report.m_bInvalidAuthorityExcluded
			= !ContainsPressureTown(duplicatePressure, "town_low")
			&& !ContainsTerritoryZone(territory, "outpost_pending")
			&& !ContainsTerritoryZone(territory, "outpost_orphan")
			&& !ContainsTerritoryZone(territory, "outpost_quarantined")
			&& !ContainsTerritoryZone(territory, "outpost_deferred");
		HST_OwnershipTransitionState deferredTransition
			= state.FindOwnershipTransition("deferred_owner_projection");
		if (deferredTransition)
			deferredTransition.m_bDeferredPublicationReleased = true;
		array<ref HST_ResistanceTerritoryProjectionRow> releasedTerritory
			= service.BuildResistanceTerritory(state, "FIA");
		report.m_bDeferredPublicationExact = deferredTransition
			&& !ContainsTerritoryZone(territory, "outpost_deferred")
			&& ContainsTerritoryZone(releasedTerritory, "outpost_deferred")
			&& releasedTerritory.Count() == territory.Count() + 1;

		report.m_sEvidence = string.Format(
			"pressure %1 current %2 low/close/high %3/%4/%5 | territory %6 | duplicate/deferred exact %7/%8",
			pressure.Count(),
			pressure.Count() > 0 && pressure[0].m_bCurrentTown,
			pressure.Count() > 1 && pressure[1].m_sTownId == "town_low",
			pressure.Count() > 2 && pressure[2].m_sTownId == "town_close",
			pressure.Count() > 3 && pressure[3].m_sTownId == "town_high",
			territory.Count(),
			report.m_bInvalidAuthorityExcluded,
			report.m_bDeferredPublicationExact);
		return report;
	}

	protected HST_CampaignState BuildState()
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		state.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;

		state.m_aZones.Insert(BuildZone("town_current", "Bravo", HST_EZoneType.HST_ZONE_TOWN, "FIA", "1000 0 1000"));
		state.m_aZones.Insert(BuildZone("town_low", "Zulu", HST_EZoneType.HST_ZONE_TOWN, "US", "3000 0 3000"));
		state.m_aZones.Insert(BuildZone("town_close", "Alpha", HST_EZoneType.HST_ZONE_TOWN, "US", "4000 0 4000"));
		state.m_aZones.Insert(BuildZone("town_high", "Charlie", HST_EZoneType.HST_ZONE_TOWN, "FIA", "5000 0 5000"));
		state.m_aZones.Insert(BuildZone("town_hidden", "Hidden", HST_EZoneType.HST_ZONE_TOWN, "US", "7000 0 7000"));
		state.m_aZones.Insert(BuildZone("outpost_alpha", "Delta", HST_EZoneType.HST_ZONE_OUTPOST, "FIA", "6000 0 6000"));
		state.m_aZones.Insert(BuildZone("resource_alpha", "Echo", HST_EZoneType.HST_ZONE_RESOURCE, "FIA", "6500 0 6500"));
		state.m_aZones.Insert(BuildZone("mission_bookkeeping", "Mission", HST_EZoneType.HST_ZONE_MISSION_SITE, "FIA", "6800 0 6800"));
		HST_ZoneState pendingZone = BuildZone("outpost_pending", "Pending", HST_EZoneType.HST_ZONE_OUTPOST, "FIA", "6900 0 6900");
		pendingZone.m_sActiveOwnershipTransitionRequestId = "pending_owner_projection";
		state.m_aZones.Insert(pendingZone);
		HST_ZoneState departingZone = BuildZone("outpost_departing", "Leaving", HST_EZoneType.HST_ZONE_OUTPOST, "US", "6925 0 6925");
		departingZone.m_iOwnershipRevision = 2;
		departingZone.m_sActiveOwnershipTransitionRequestId = "departing_owner_projection";
		state.m_aZones.Insert(departingZone);
		HST_ZoneState orphanZone = BuildZone("outpost_orphan", "Orphan", HST_EZoneType.HST_ZONE_OUTPOST, "FIA", "6950 0 6950");
		orphanZone.m_sActiveOwnershipTransitionRequestId = "missing_owner_projection";
		state.m_aZones.Insert(orphanZone);
		HST_ZoneState quarantinedZone = BuildZone("outpost_quarantined", "Quarantined", HST_EZoneType.HST_ZONE_OUTPOST, "US", "6975 0 6975");
		quarantinedZone.m_iOwnershipContractVersion = HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION;
		quarantinedZone.m_sActiveOwnershipTransitionRequestId = "quarantined_owner_projection";
		quarantinedZone.m_sOwnershipAuthorityFailure = "projection proof quarantine";
		state.m_aZones.Insert(quarantinedZone);
		HST_ZoneState deferredZone = BuildZone(
			"outpost_deferred",
			"Deferred",
			HST_EZoneType.HST_ZONE_OUTPOST,
			"FIA",
			"6990 0 6990");
		deferredZone.m_iOwnershipRevision = 2;
		deferredZone.m_sLastOwnershipTransitionRequestId
			= "deferred_owner_projection";
		state.m_aZones.Insert(deferredZone);
		HST_OwnershipTransitionState pendingTransition = new HST_OwnershipTransitionState();
		pendingTransition.m_sRequestId = pendingZone.m_sActiveOwnershipTransitionRequestId;
		pendingTransition.m_sZoneId = pendingZone.m_sZoneId;
		pendingTransition.m_sPreviousOwnerFactionKey = "US";
		pendingTransition.m_sNewOwnerFactionKey = "FIA";
		pendingTransition.m_bOwnerApplied = true;
		state.m_aOwnershipTransitions.Insert(pendingTransition);
		HST_OwnershipTransitionState departingTransition = new HST_OwnershipTransitionState();
		departingTransition.m_sRequestId = departingZone.m_sActiveOwnershipTransitionRequestId;
		departingTransition.m_sZoneId = departingZone.m_sZoneId;
		departingTransition.m_sPreviousOwnerFactionKey = "FIA";
		departingTransition.m_sNewOwnerFactionKey = "US";
		departingTransition.m_iExpectedRevision = 1;
		departingTransition.m_iAppliedRevision = 2;
		departingTransition.m_bOwnerApplied = true;
		state.m_aOwnershipTransitions.Insert(departingTransition);
		HST_OwnershipTransitionState quarantinedTransition = new HST_OwnershipTransitionState();
		quarantinedTransition.m_iContractVersion = HST_OwnershipTransitionService.QUARANTINED_CONTRACT_VERSION;
		quarantinedTransition.m_sRequestId = quarantinedZone.m_sActiveOwnershipTransitionRequestId;
		quarantinedTransition.m_sZoneId = quarantinedZone.m_sZoneId;
		quarantinedTransition.m_sPreviousOwnerFactionKey = "FIA";
		quarantinedTransition.m_sNewOwnerFactionKey = "US";
		quarantinedTransition.m_iExpectedRevision = 1;
		quarantinedTransition.m_iAppliedRevision = 2;
		quarantinedTransition.m_bOwnerApplied = true;
		quarantinedTransition.m_bQuarantined = true;
		state.m_aOwnershipTransitions.Insert(quarantinedTransition);
		HST_OwnershipTransitionState deferredTransition
			= new HST_OwnershipTransitionState();
		deferredTransition.m_sRequestId
			= deferredZone.m_sLastOwnershipTransitionRequestId;
		deferredTransition.m_sZoneId = deferredZone.m_sZoneId;
		deferredTransition.m_sPreviousOwnerFactionKey = "US";
		deferredTransition.m_sNewOwnerFactionKey = "FIA";
		deferredTransition.m_iExpectedRevision = 1;
		deferredTransition.m_iAppliedRevision = 2;
		deferredTransition.m_bCompleted = true;
		deferredTransition.m_sProjectionParentRequestId
			= "deferred_parent_projection";
		state.m_aOwnershipTransitions.Insert(deferredTransition);

		state.m_aTownInfluenceRecords.Insert(BuildRecord("town_current", 9000, true));
		state.m_aTownInfluenceRecords.Insert(BuildRecord("town_low", 2001, true));
		state.m_aTownInfluenceRecords.Insert(BuildRecord("town_close", 2049, true));
		state.m_aTownInfluenceRecords.Insert(BuildRecord("town_high", 7000, true));
		state.m_aTownInfluenceRecords.Insert(BuildRecord("town_hidden", 1000, false));
		return state;
	}

	protected HST_ZoneState BuildZone(
		string zoneId,
		string displayName,
		HST_EZoneType zoneType,
		string ownerFactionKey,
		vector position)
	{
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = zoneId;
		zone.m_sDisplayName = displayName;
		zone.m_eType = zoneType;
		zone.m_sOwnerFactionKey = ownerFactionKey;
		zone.m_vPosition = position;
		zone.m_iCaptureRadiusMeters = 180;
		zone.m_iOwnershipRevision = 1;
		return zone;
	}

	protected HST_TownInfluenceRecord BuildRecord(string townId, int fiaSupportBasisPoints, bool contacted)
	{
		HST_TownInfluenceRecord record = new HST_TownInfluenceRecord();
		record.m_sTownId = townId;
		record.m_iRevision = 1;
		record.m_iFIASupportBasisPoints = fiaSupportBasisPoints;
		record.m_iOccupierSupportBasisPoints = 10000 - fiaSupportBasisPoints;
		record.m_iInitialPopulation = 100;
		record.m_iRemainingPopulation = 100;
		record.m_bContacted = contacted;
		record.m_sHysteresisBand = HST_TownInfluenceService.ResolveHysteresisBand(
			fiaSupportBasisPoints);
		if (contacted)
		{
			record.m_sContactSourceId = "map_war_projection_proof_" + townId;
			record.m_sContactReason = "projection proof contact";
		}
		return record;
	}

	protected bool ContainsPressureTown(
		notnull array<ref HST_ZonePressureProjectionRow> rows,
		string townId)
	{
		foreach (HST_ZonePressureProjectionRow row : rows)
		{
			if (row && row.m_sTownId == townId)
				return true;
		}
		return false;
	}

	protected bool ContainsTerritoryZone(
		notnull array<ref HST_ResistanceTerritoryProjectionRow> rows,
		string zoneId)
	{
		foreach (HST_ResistanceTerritoryProjectionRow row : rows)
		{
			if (row && row.m_sZoneId == zoneId)
				return true;
		}
		return false;
	}
}
#endif
