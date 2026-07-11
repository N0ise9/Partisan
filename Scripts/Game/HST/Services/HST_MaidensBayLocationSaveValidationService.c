// Schema-60 retires the legacy Maiden's Bay town aggregate. The mapped site is
// the Logistics Warehouse resource; a compatibility view keeps frozen records
// that cite the old stable ID resolvable without recreating a town, population,
// marker, garrison manpower, or resource balance.
class HST_MaidensBayLocationSaveValidationService
{
	static const int SCHEMA_VERSION = 60;
	static const string LEGACY_ZONE_ID = "town_maidens_bay";
	static const string CANONICAL_ZONE_ID = "resource_logistics_warehouse";
	static const string MIGRATION_EVENT_ID = "migration_schema60_maidens_bay_location_merge";
	static const string LEGACY_ZONE_MARKER_ID = "hst_zone_town_maidens_bay";

	protected HST_CampaignSaveData m_SaveData;

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		if (!m_SaveData)
			return;
		int canonicalZoneCount = CountCanonicalZones();
		int legacyZoneCountBeforeMigration = CountLegacyZones();
		// No anchor means there is no authoritative row into which auxiliary
		// references can be merged. Leave the save byte-stable for a higher-level
		// repair instead of deleting orphan evidence.
		if (canonicalZoneCount == 0 && legacyZoneCountBeforeMigration == 0)
		{
			m_SaveData = null;
			return;
		}
		// A unique canonical row is authoritative even if several duplicate old
		// rows survived. Without it, however, choosing among multiple old rows
		// would invent an owner/economy winner.
		if (canonicalZoneCount > 1
			|| (canonicalZoneCount == 0 && legacyZoneCountBeforeMigration > 1))
		{
			RecordAmbiguousCanonicalConflict(restoredSchemaVersion);
			m_SaveData = null;
			return;
		}

		array<string> retiredLinks = {};
		int legacyZoneCount = CaptureLegacyZoneLinks(retiredLinks);
		HST_ZoneState canonicalZone = FindCanonicalZone();
		bool convertedLegacyZone;
		if (!canonicalZone)
		{
			canonicalZone = FindLegacyZone();
			if (canonicalZone)
			{
				ConfigureCanonicalWarehouse(canonicalZone, true);
				convertedLegacyZone = true;
			}
		}

		int removedZoneCount = RemoveRemainingLegacyZones();
		if (canonicalZone)
		{
			ConfigureCanonicalWarehouse(canonicalZone, convertedLegacyZone);
			MergeZoneLinks(canonicalZone, retiredLinks);
		}
		int normalizedLinkCount;
		if (canonicalZone)
			normalizedLinkCount = NormalizeZoneLinks();
		array<string> frozenSiteIds = {};
		array<string> frozenRouteIds = {};
		CollectOpenFrozenContentReferences(frozenSiteIds, frozenRouteIds);
		int normalizedLiveReferenceCount;
		int retiredProjectionCount;
		NormalizeLiveZoneReferences(canonicalZone != null, normalizedLiveReferenceCount, retiredProjectionCount);
		int normalizedGeneratedContentCount = NormalizeGeneratedContent(
			canonicalZone != null,
			frozenSiteIds,
			frozenRouteIds);
		int retiredGarrisonCount = NormalizeLegacyGarrisons(convertedLegacyZone, canonicalZone != null);
		int retiredCivilianCount = RemoveLegacyCivilianRows();
		int retiredInfluenceCount = RemoveLegacyTownInfluenceRows();
		int retiredMarkerCount = RemoveLegacyZoneMarkers();
		int normalizedLedgerCount = NormalizeEnemySupportLedgers(canonicalZone != null);

		int changedCount = legacyZoneCount + removedZoneCount + normalizedLinkCount
			+ normalizedLiveReferenceCount + retiredProjectionCount + normalizedGeneratedContentCount
			+ retiredGarrisonCount + retiredCivilianCount + retiredInfluenceCount
			+ retiredMarkerCount + normalizedLedgerCount;
		if (changedCount > 0 && !HasEvent(MIGRATION_EVENT_ID))
		{
			HST_CampaignEventState eventState = new HST_CampaignEventState();
			eventState.m_sEventId = MIGRATION_EVENT_ID;
			eventState.m_sCategory = "migration";
			eventState.m_sAggregateType = "location_taxonomy";
			eventState.m_sAggregateId = "schema60";
			eventState.m_sTransition = "duplicate_town_retired_into_logistics_warehouse";
			eventState.m_sReason = string.Format(
				"restored schema %1; retired %2 legacy town zone rows, %3 duplicate garrison rows, %4 civilian rows, %5 town-influence rows, %6 marker rows, and %9 duplicate live projections; normalized %7 graph links and %8 support ledgers; ",
				restoredSchemaVersion,
				legacyZoneCount,
				retiredGarrisonCount,
				retiredCivilianCount,
				retiredInfluenceCount,
				retiredMarkerCount,
				normalizedLinkCount,
				normalizedLedgerCount,
				retiredProjectionCount);
			eventState.m_sReason += string.Format(
				"normalized %1 mutable live references and %2 generated-content rows; preserved canonical warehouse ownership and economy, preserved no duplicate aggregate manpower, and retained explicitly typed frozen old-ID authority through a detached historical view",
				normalizedLiveReferenceCount,
				normalizedGeneratedContentCount);
			eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
			m_SaveData.m_aCampaignEvents.Insert(eventState);
		}

		m_SaveData = null;
	}

	static string ResolveCanonicalZoneId(string zoneId)
	{
		if (zoneId == LEGACY_ZONE_ID)
			return CANONICAL_ZONE_ID;
		return zoneId;
	}

	static bool IsLegacyZoneId(string zoneId)
	{
		return zoneId == LEGACY_ZONE_ID;
	}

	static bool AreEquivalentZoneIds(string leftZoneId, string rightZoneId)
	{
		if (leftZoneId.IsEmpty() || rightZoneId.IsEmpty())
			return leftZoneId == rightZoneId;
		return ResolveCanonicalZoneId(leftZoneId) == ResolveCanonicalZoneId(rightZoneId);
	}

	// Frozen pre-schema-60 operations retain the old ID and exact old position.
	// This detached view is deliberately never inserted into m_aZones, so it
	// cannot produce a marker, civilian population, income tick, or new order.
	static HST_ZoneState BuildLegacyCompatibilityZone(HST_ZoneState canonicalZone)
	{
		return BuildFrozenHistoricalZoneView(canonicalZone);
	}

	static HST_ZoneState BuildFrozenHistoricalZoneView(HST_ZoneState canonicalZone)
	{
		if (!canonicalZone || canonicalZone.m_sZoneId != CANONICAL_ZONE_ID)
			return null;

		HST_ZoneState view = new HST_ZoneState();
		view.m_sZoneId = LEGACY_ZONE_ID;
		view.m_sDisplayName = "Logistics Warehouse";
		view.m_sSourceLayoutId = canonicalZone.m_sSourceLayoutId;
		view.m_sSourceLayerName = canonicalZone.m_sSourceLayerName;
		view.m_sMarkerCallsign = "";
		view.m_sMarkerTextColor = "gold";
		view.m_sMarkerStyle = "resource";
		view.m_sOwnerFactionKey = canonicalZone.m_sOwnerFactionKey;
		view.m_eType = HST_EZoneType.HST_ZONE_RESOURCE;
		view.m_vPosition = "5345.855 43.594 10536.602";
		view.m_sResourceKind = "supplies";
		view.m_iSupport = canonicalZone.m_iSupport;
		view.m_iResistanceCaptureProgress = canonicalZone.m_iResistanceCaptureProgress;
		view.m_iIncomeValue = canonicalZone.m_iIncomeValue;
		view.m_iCaptureRadiusMeters = 190;
		view.m_iPriority = canonicalZone.m_iPriority;
		view.m_iGarrisonSlots = 10;
		view.m_iActivationRadiusMeters = canonicalZone.m_iActivationRadiusMeters;
		view.m_sCompositionId = "comp_town_maidens_bay";
		view.m_sSpawnProfileId = "spawn_resource_guards";
		view.m_bActive = canonicalZone.m_bActive;
		view.m_iActiveInfantryCount = canonicalZone.m_iActiveInfantryCount;
		view.m_iActiveVehicleCount = canonicalZone.m_iActiveVehicleCount;
		view.m_sPatrolRouteId = "route_town_maidens_bay";
		view.m_sQRFRouteId = "qrf_town_maidens_bay";
		view.m_sMissionSiteId = "site_town_maidens_bay";
		view.m_iQrfCooldownUntilSecond = canonicalZone.m_iQrfCooldownUntilSecond;
		foreach (string linkedZoneId : canonicalZone.m_aLinkedZoneIds)
			view.m_aLinkedZoneIds.Insert(linkedZoneId);
		return view;
	}

	protected int CaptureLegacyZoneLinks(array<string> retiredLinks)
	{
		int count;
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (!zone || zone.m_sZoneId != LEGACY_ZONE_ID)
				continue;
			count++;
			foreach (string linkedZoneId : zone.m_aLinkedZoneIds)
			{
				string canonicalLink = ResolveCanonicalZoneId(linkedZoneId);
				if (!canonicalLink.IsEmpty() && canonicalLink != CANONICAL_ZONE_ID
					&& !retiredLinks.Contains(canonicalLink))
					retiredLinks.Insert(canonicalLink);
			}
		}
		return count;
	}

	protected HST_ZoneState FindCanonicalZone()
	{
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (zone && zone.m_sZoneId == CANONICAL_ZONE_ID)
				return zone;
		}
		return null;
	}

	protected int CountCanonicalZones()
	{
		int count;
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (zone && zone.m_sZoneId == CANONICAL_ZONE_ID)
				count++;
		}
		return count;
	}

	protected int CountLegacyZones()
	{
		int count;
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (zone && zone.m_sZoneId == LEGACY_ZONE_ID)
				count++;
		}
		return count;
	}

	protected void RecordAmbiguousCanonicalConflict(int restoredSchemaVersion)
	{
		string eventId = MIGRATION_EVENT_ID + "_ambiguous_canonical";
		if (HasEvent(eventId))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = eventId;
		eventState.m_sCategory = "normalization";
		eventState.m_sAggregateType = "location_taxonomy";
		eventState.m_sAggregateId = "schema60";
		eventState.m_sTransition = "ambiguous_location_authority_preserved_fail_closed";
		eventState.m_sReason = string.Format(
			"restored schema %1 contains ambiguous Logistics Warehouse/Maiden's Bay zone authority; no links, projections, ledgers, or generated content were rewritten",
			restoredSchemaVersion);
		eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(eventState);
	}

	protected HST_ZoneState FindLegacyZone()
	{
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (zone && zone.m_sZoneId == LEGACY_ZONE_ID)
				return zone;
		}
		return null;
	}

	protected void ConfigureCanonicalWarehouse(HST_ZoneState zone, bool preserveLegacyEconomy)
	{
		if (!zone)
			return;
		zone.m_sZoneId = CANONICAL_ZONE_ID;
		zone.m_sDisplayName = "Logistics Warehouse";
		zone.m_sSourceLayoutId = "MBC_SaintPhillipe";
		zone.m_sSourceLayerName = "bases/base_SaintPhilippe.layer";
		zone.m_eType = HST_EZoneType.HST_ZONE_RESOURCE;
		zone.m_vPosition = "5347.874 43.617 10542.923";
		zone.m_sResourceKind = "supplies";
		zone.m_iCaptureRadiusMeters = 190;
		zone.m_iPriority = 12;
		zone.m_sMarkerCallsign = "";
		zone.m_sMarkerTextColor = "gold";
		zone.m_sMarkerStyle = "resource";
		zone.m_sPatrolRouteId = "route_resource_logistics_warehouse";
		zone.m_sQRFRouteId = "qrf_resource_logistics_warehouse";
		zone.m_sMissionSiteId = "site_resource_logistics_warehouse";
		zone.m_sCompositionId = "comp_resource_logistics_warehouse";
		zone.m_sSpawnProfileId = "spawn_resource_guards";
		if (zone.m_iActivationRadiusMeters <= 0)
			zone.m_iActivationRadiusMeters = 1200;
		// An existing canonical row owns its durable economy values. A lone
		// legacy row also retains its prior values across every later restore;
		// only absent canonical definition values receive warehouse defaults.
		if (!preserveLegacyEconomy)
		{
			if (zone.m_iIncomeValue <= 0)
				zone.m_iIncomeValue = 80;
			if (zone.m_iGarrisonSlots <= 0)
				zone.m_iGarrisonSlots = 14;
		}
	}

	protected int RemoveRemainingLegacyZones()
	{
		int removed;
		for (int zoneIndex = m_SaveData.m_aZones.Count() - 1; zoneIndex >= 0; zoneIndex--)
		{
			HST_ZoneState zone = m_SaveData.m_aZones[zoneIndex];
			if (!zone || zone.m_sZoneId != LEGACY_ZONE_ID)
				continue;
			m_SaveData.m_aZones.Remove(zoneIndex);
			removed++;
		}
		return removed;
	}

	protected void MergeZoneLinks(HST_ZoneState zone, array<string> links)
	{
		if (!zone)
			return;
		foreach (string linkedZoneId : links)
		{
			if (!linkedZoneId.IsEmpty() && linkedZoneId != zone.m_sZoneId
				&& !zone.m_aLinkedZoneIds.Contains(linkedZoneId))
				zone.m_aLinkedZoneIds.Insert(linkedZoneId);
		}
	}

	protected int NormalizeZoneLinks()
	{
		int changed;
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (!zone)
				continue;
			array<string> normalizedLinks = {};
			foreach (string linkedZoneId : zone.m_aLinkedZoneIds)
			{
				string canonicalLink = ResolveCanonicalZoneId(linkedZoneId);
				if (canonicalLink != linkedZoneId)
					changed++;
				if (canonicalLink.IsEmpty() || canonicalLink == zone.m_sZoneId
					|| normalizedLinks.Contains(canonicalLink))
				{
					changed++;
					continue;
				}
				normalizedLinks.Insert(canonicalLink);
			}
			zone.m_aLinkedZoneIds.Clear();
			foreach (string normalizedLink : normalizedLinks)
				zone.m_aLinkedZoneIds.Insert(normalizedLink);
		}
		return changed;
	}

	// Mutable generic campaign state always moves to the canonical warehouse ID.
	// Any nonzero typed claimant, including malformed/quarantined authority, keeps
	// its frozen identity fail-closed for its owning validator to settle.
	protected void NormalizeLiveZoneReferences(
		bool hasCanonicalZone,
		out int normalizedCount,
		out int retiredProjectionCount)
	{
		normalizedCount = 0;
		retiredProjectionCount = 0;
		if (!hasCanonicalZone)
			return;

		array<string> retiredGroupIds = {};
		for (int groupIndex = m_SaveData.m_aActiveGroups.Count() - 1; groupIndex >= 0; groupIndex--)
		{
			HST_ActiveGroupState group = m_SaveData.m_aActiveGroups[groupIndex];
			if (!group || (group.m_sZoneId != LEGACY_ZONE_ID
				&& group.m_sGarrisonZoneId != LEGACY_ZONE_ID))
				continue;
			if (IsOpenFrozenTypedGroup(group))
				continue;

			if (IsDuplicateLegacyProjection(group))
			{
				if (!group.m_sGroupId.IsEmpty() && !retiredGroupIds.Contains(group.m_sGroupId))
					retiredGroupIds.Insert(group.m_sGroupId);
				m_SaveData.m_aActiveGroups.Remove(groupIndex);
				retiredProjectionCount++;
				continue;
			}

			if (group.m_sZoneId == LEGACY_ZONE_ID)
			{
				group.m_sZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
			if (group.m_sGarrisonZoneId == LEGACY_ZONE_ID)
			{
				group.m_sGarrisonZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
			string canonicalRouteId = CanonicalizeGeneratedRouteId(group.m_sRouteId);
			if (canonicalRouteId != group.m_sRouteId)
			{
				group.m_sRouteId = canonicalRouteId;
				normalizedCount++;
			}
		}

		foreach (HST_QRFState qrf : m_SaveData.m_aQRFs)
		{
			if (!qrf || IsOpenFrozenTypedGroupId(qrf.m_sGroupId))
				continue;
			normalizedCount += NormalizeQRFZoneReferences(qrf);
		}

		foreach (HST_SupportRequestState request : m_SaveData.m_aSupportRequests)
		{
			if (!request || IsTypedSupportRequestClaimant(request, false))
				continue;
			if (request.m_sSourceZoneId == LEGACY_ZONE_ID)
			{
				request.m_sSourceZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
			if (request.m_sTargetZoneId == LEGACY_ZONE_ID)
			{
				request.m_sTargetZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
			string canonicalDeploymentRouteId = CanonicalizeGeneratedRouteId(request.m_sDeploymentRouteId);
			if (canonicalDeploymentRouteId != request.m_sDeploymentRouteId)
			{
				request.m_sDeploymentRouteId = canonicalDeploymentRouteId;
				normalizedCount++;
			}
		}

		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (!order || IsTypedEnemyOrderClaimant(order, false))
				continue;
			if (order.m_sSourceZoneId == LEGACY_ZONE_ID)
			{
				order.m_sSourceZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
			if (order.m_sTargetZoneId == LEGACY_ZONE_ID)
			{
				order.m_sTargetZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
		}

		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!mission || IsTypedMissionClaimant(mission, false))
				continue;
			if (mission.m_sTargetZoneId == LEGACY_ZONE_ID)
			{
				mission.m_sTargetZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
			string canonicalSiteId = CanonicalizeGeneratedSiteId(mission.m_sSiteId);
			if (canonicalSiteId != mission.m_sSiteId)
			{
				mission.m_sSiteId = canonicalSiteId;
				normalizedCount++;
			}
		}

		foreach (HST_MissionObjectiveState objective : m_SaveData.m_aMissionObjectives)
		{
			if (!objective || objective.m_sTargetZoneId != LEGACY_ZONE_ID)
				continue;
			if (HasTypedMissionClaimant(objective.m_sMissionInstanceId))
				continue;
			if (ShouldCanonicalizeObjectiveTargetId(objective))
			{
				objective.m_sTargetId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
			objective.m_sTargetZoneId = CANONICAL_ZONE_ID;
			normalizedCount++;
		}

		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation || IsTypedOperationClaimant(operation, false))
				continue;
			normalizedCount += NormalizeOperationZoneReferences(operation);
		}

		foreach (HST_GarageVehicleState garageVehicle : m_SaveData.m_aGarageVehicles)
		{
			if (!garageVehicle)
				continue;
			if (garageVehicle.m_sSourceZoneId == LEGACY_ZONE_ID)
			{
				garageVehicle.m_sSourceZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
			if (garageVehicle.m_sLastReporterZoneId == LEGACY_ZONE_ID)
			{
				garageVehicle.m_sLastReporterZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
		}

		for (int vehicleIndex = m_SaveData.m_aRuntimeVehicles.Count() - 1; vehicleIndex >= 0; vehicleIndex--)
		{
			HST_RuntimeVehicleState runtimeVehicle = m_SaveData.m_aRuntimeVehicles[vehicleIndex];
			if (!runtimeVehicle)
				continue;
			if (runtimeVehicle.m_sZoneId == LEGACY_ZONE_ID && IsLegacyAmbientRuntimeVehicle(runtimeVehicle))
			{
				m_SaveData.m_aRuntimeVehicles.Remove(vehicleIndex);
				retiredProjectionCount++;
				continue;
			}
			if (runtimeVehicle.m_sZoneId == LEGACY_ZONE_ID)
			{
				runtimeVehicle.m_sZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
			if (runtimeVehicle.m_sLastReporterZoneId == LEGACY_ZONE_ID)
			{
				runtimeVehicle.m_sLastReporterZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
		}

		foreach (HST_PlayerUndercoverState undercover : m_SaveData.m_aUndercoverPlayers)
		{
			if (!undercover)
				continue;
			if (undercover.m_sLastEnforcementZoneId == LEGACY_ZONE_ID)
			{
				undercover.m_sLastEnforcementZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
			if (undercover.m_sLastZoneId == LEGACY_ZONE_ID)
			{
				undercover.m_sLastZoneId = CANONICAL_ZONE_ID;
				normalizedCount++;
			}
		}

		RemoveMarkersForRetiredGroups(retiredGroupIds);
	}

	protected int NormalizeQRFZoneReferences(HST_QRFState qrf)
	{
		int changed;
		if (qrf.m_sSourceZoneId == LEGACY_ZONE_ID)
		{
			qrf.m_sSourceZoneId = CANONICAL_ZONE_ID;
			changed++;
		}
		if (qrf.m_sTargetZoneId == LEGACY_ZONE_ID)
		{
			qrf.m_sTargetZoneId = CANONICAL_ZONE_ID;
			changed++;
		}
		return changed;
	}

	protected int NormalizeOperationZoneReferences(HST_OperationRecordState operation)
	{
		int changed;
		if (operation.m_sOriginZoneId == LEGACY_ZONE_ID)
		{
			operation.m_sOriginZoneId = CANONICAL_ZONE_ID;
			changed++;
		}
		if (operation.m_sAssignmentZoneId == LEGACY_ZONE_ID)
		{
			operation.m_sAssignmentZoneId = CANONICAL_ZONE_ID;
			changed++;
		}
		if (operation.m_sTacticalTargetZoneId == LEGACY_ZONE_ID)
		{
			operation.m_sTacticalTargetZoneId = CANONICAL_ZONE_ID;
			changed++;
		}
		string canonicalRouteId = CanonicalizeGeneratedRouteId(operation.m_sCurrentRouteId);
		if (canonicalRouteId != operation.m_sCurrentRouteId)
		{
			operation.m_sCurrentRouteId = canonicalRouteId;
			changed++;
		}
		return changed;
	}

	protected bool IsDuplicateLegacyProjection(HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		if (HasLiveLinkedAuthority(group))
			return false;
		if (!group.m_sGarrisonZoneId.IsEmpty())
			return group.m_sGarrisonZoneId == LEGACY_ZONE_ID;
		string projectionText = group.m_sSpawnFallbackMode + " " + group.m_sRuntimeStatus
			+ " " + group.m_sPrefab + " " + group.m_sOperationId;
		projectionText.ToLower();
		if (projectionText.Contains("town_police") || projectionText.Contains("townpolice")
			|| projectionText.Contains("garrison"))
			return true;
		return !group.m_bQRF;
	}

	protected bool ShouldCanonicalizeObjectiveTargetId(HST_MissionObjectiveState objective)
	{
		if (!objective || objective.m_sTargetId != LEGACY_ZONE_ID)
			return false;
		return objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA
			|| objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT
			|| objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES
			|| objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA
			|| objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_FIND_SITE;
	}

	protected bool HasLiveLinkedAuthority(HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		if (!group.m_sMissionInstanceId.IsEmpty() && FindMission(group.m_sMissionInstanceId))
			return true;
		if (!group.m_sSupportRequestId.IsEmpty() && FindSupportRequest(group.m_sSupportRequestId))
			return true;
		if (!group.m_sEnemyOrderId.IsEmpty() && FindEnemyOrder(group.m_sEnemyOrderId))
			return true;
		if (!group.m_sQRFInstanceId.IsEmpty() && FindQRF(group.m_sQRFInstanceId))
			return true;
		if (!group.m_sOperationId.IsEmpty() && FindOperation(group.m_sOperationId))
			return true;
		return false;
	}

	protected bool IsLegacyAmbientRuntimeVehicle(HST_RuntimeVehicleState vehicle)
	{
		if (!vehicle || vehicle.m_bDetached)
			return false;
		string runtimeKind = vehicle.m_sRuntimeKind;
		runtimeKind.ToLower();
		return runtimeKind.Contains("civ") || runtimeKind.Contains("ambient")
			|| runtimeKind.Contains("military_vehicle");
	}

	protected void RemoveMarkersForRetiredGroups(array<string> retiredGroupIds)
	{
		if (!retiredGroupIds || retiredGroupIds.IsEmpty())
			return;
		for (int markerIndex = m_SaveData.m_aMapMarkers.Count() - 1; markerIndex >= 0; markerIndex--)
		{
			HST_MapMarkerState marker = m_SaveData.m_aMapMarkers[markerIndex];
			if (!marker || !retiredGroupIds.Contains(marker.m_sLinkedId))
				continue;
			m_SaveData.m_aMapMarkers.Remove(markerIndex);
		}
	}

	protected HST_ActiveMissionState FindMission(string missionInstanceId)
	{
		if (missionInstanceId.IsEmpty())
			return null;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (mission && mission.m_sInstanceId == missionInstanceId)
				return mission;
		}
		return null;
	}

	protected bool HasTypedMissionClaimant(string missionInstanceId)
	{
		if (missionInstanceId.IsEmpty())
			return false;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (mission && mission.m_sInstanceId == missionInstanceId
				&& IsTypedMissionClaimant(mission, false))
				return true;
		}
		return false;
	}

	protected HST_SupportRequestState FindSupportRequest(string requestId)
	{
		if (requestId.IsEmpty())
			return null;
		foreach (HST_SupportRequestState request : m_SaveData.m_aSupportRequests)
		{
			if (request && request.m_sRequestId == requestId)
				return request;
		}
		return null;
	}

	protected HST_EnemyOrderState FindEnemyOrder(string orderId)
	{
		if (orderId.IsEmpty())
			return null;
		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				return order;
		}
		return null;
	}

	protected HST_QRFState FindQRF(string qrfInstanceId)
	{
		if (qrfInstanceId.IsEmpty())
			return null;
		foreach (HST_QRFState qrf : m_SaveData.m_aQRFs)
		{
			if (qrf && qrf.m_sInstanceId == qrfInstanceId)
				return qrf;
		}
		return null;
	}

	protected HST_OperationRecordState FindOperation(string operationId)
	{
		if (operationId.IsEmpty())
			return null;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (operation && operation.m_sOperationId == operationId)
				return operation;
		}
		return null;
	}

	protected bool IsOpenFrozenTypedGroup(HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		if (group.m_sSpawnFallbackMode == HST_EnemyQRFOperationService.EXACT_QRF_GROUP_MODE
			|| group.m_sSpawnFallbackMode == HST_RescuePOWOperationService.EXACT_GROUP_MODE
			|| group.m_sSpawnFallbackMode == HST_RescuePOWOperationService.QUARANTINE_STATUS
			|| group.m_sRuntimeStatus == HST_RescuePOWOperationService.QUARANTINE_STATUS
			|| group.m_sSpawnFallbackMode == HST_PlayerSearchDestroySaveValidationService.QUARANTINE_MODE
			|| group.m_sRuntimeStatus == HST_PlayerSearchDestroySaveValidationService.QUARANTINE_STATUS
			|| group.m_sSpawnFallbackMode == HST_SupportRequestService.EXACT_PLAYER_SUPPORT_MODE
			|| group.m_sRuntimeStatus == HST_SupportRequestService.EXACT_PLAYER_SUPPORT_GROUP_STATUS
			|| group.m_sSpawnFallbackMode == HST_EnemyPatrolOperationService.EXACT_GROUP_MODE
			|| group.m_sSpawnFallbackMode.StartsWith(HST_EnemyPatrolOperationService.EXACT_GROUP_MODE + "_")
			|| group.m_sRuntimeStatus.StartsWith("enemy_patrol_")
			|| group.m_sRuntimeStatus == "exact_patrol_quarantined"
			|| group.m_sRuntimeStatus == "exact_patrol_orphan_quarantined"
			|| group.m_sRuntimeStatus == "exact_runtime_authority_quarantined")
			return true;
		if (HST_PlayerSearchDestroySaveValidationService.IsSchema60PlayerSearchDestroyGroupClaimant(m_SaveData, group)
			|| HST_MissionConvoySaveValidationService.IsSchema52MissionConvoyGroupClaimant(m_SaveData, group)
			|| HST_GarrisonPatrolSaveValidationService.IsSchema54GarrisonPatrolGroupClaimant(m_SaveData, group)
			|| HST_AssassinationGuardSaveValidationService.IsSchema57MissionGuardGroupClaimant(m_SaveData, group)
			|| HST_RescuePOWSaveValidationService.IsSchema58RescuePOWGroupClaimant(m_SaveData, group))
			return true;
		foreach (HST_SupportRequestState request : m_SaveData.m_aSupportRequests)
		{
			if (request && request.m_iOperationContractVersion != 0
				&& ((!group.m_sSupportRequestId.IsEmpty() && group.m_sSupportRequestId == request.m_sRequestId)
					|| (!group.m_sOperationId.IsEmpty() && group.m_sOperationId == request.m_sOperationId)))
				return true;
		}
		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (order && order.m_iOperationContractVersion != 0
				&& ((!group.m_sEnemyOrderId.IsEmpty() && group.m_sEnemyOrderId == order.m_sOrderId)
					|| (!group.m_sOperationId.IsEmpty() && group.m_sOperationId == order.m_sOperationId)))
				return true;
		}
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (mission && mission.m_iOperationContractVersion != 0
				&& ((!group.m_sMissionInstanceId.IsEmpty() && group.m_sMissionInstanceId == mission.m_sInstanceId)
					|| (!group.m_sOperationId.IsEmpty() && group.m_sOperationId == mission.m_sOperationId)))
				return true;
		}
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!IsTypedOperationClaimant(operation, false))
				continue;
			if (MatchesOperationGroupIdentity(operation, group))
				return true;
		}
		if (HasTypedManifestAuthority(group) || HasTypedBatchAuthority(group)
			|| HasTypedTombstoneAuthority(group))
			return true;
		return false;
	}

	protected bool HasTypedManifestAuthority(HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!manifest)
				continue;
			bool matches = (!group.m_sManifestId.IsEmpty()
				&& group.m_sManifestId == manifest.m_sManifestId)
				|| (!group.m_sOperationId.IsEmpty()
					&& group.m_sOperationId == manifest.m_sOperationId);
			if (!matches)
				continue;
			if (IsTypedManifestClaimant(manifest))
				return true;
		}
		return false;
	}

	protected bool IsTypedManifestClaimant(HST_ForceManifestState manifest)
	{
		if (!manifest)
			return false;
		string policyId = manifest.m_sPolicyId;
		policyId.ToLower();
		if (policyId.Contains("exact"))
			return true;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (IsTypedOperationClaimant(operation, false)
				&& ((!manifest.m_sOperationId.IsEmpty() && manifest.m_sOperationId == operation.m_sOperationId)
					|| (!manifest.m_sManifestId.IsEmpty() && manifest.m_sManifestId == operation.m_sManifestId)))
				return true;
		}
		foreach (HST_SupportRequestState request : m_SaveData.m_aSupportRequests)
		{
			if (request && IsTypedSupportRequestClaimant(request, false)
				&& ((!manifest.m_sOperationId.IsEmpty() && manifest.m_sOperationId == request.m_sOperationId)
					|| (!manifest.m_sManifestId.IsEmpty() && manifest.m_sManifestId == request.m_sManifestId)))
				return true;
		}
		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (order && IsTypedEnemyOrderClaimant(order, false)
				&& !manifest.m_sOperationId.IsEmpty() && manifest.m_sOperationId == order.m_sOperationId)
				return true;
		}
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (mission && IsTypedMissionClaimant(mission, false)
				&& ((!manifest.m_sOperationId.IsEmpty() && manifest.m_sOperationId == mission.m_sOperationId)
					|| (!manifest.m_sManifestId.IsEmpty() && manifest.m_sManifestId == mission.m_sManifestId)))
				return true;
		}
		foreach (HST_ForceSettlementTombstoneState tombstone : m_SaveData.m_aForceSettlementTombstones)
		{
			if (tombstone && tombstone.m_iOperationContractVersion != 0
				&& ((!manifest.m_sOperationId.IsEmpty() && manifest.m_sOperationId == tombstone.m_sOperationId)
					|| (!manifest.m_sManifestId.IsEmpty() && manifest.m_sManifestId == tombstone.m_sManifestId)))
				return true;
		}
		return false;
	}

	protected bool HasTypedBatchAuthority(HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch)
				continue;
			bool matches = (!group.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == batch.m_sResultId)
				|| (!group.m_sOperationId.IsEmpty() && group.m_sOperationId == batch.m_sOperationId)
				|| (!group.m_sManifestId.IsEmpty() && group.m_sManifestId == batch.m_sManifestId)
				|| (!group.m_sForceId.IsEmpty() && group.m_sForceId == batch.m_sForceId)
				|| (!group.m_sProjectionId.IsEmpty() && group.m_sProjectionId == batch.m_sProjectionId);
			if (!matches)
				continue;
			if (HST_PlayerSearchDestroySaveValidationService.IsSchema60PlayerSearchDestroyBatchClaimant(m_SaveData, batch)
				|| HST_MissionConvoySaveValidationService.IsSchema52MissionConvoyBatchClaimant(m_SaveData, batch)
				|| HST_GarrisonPatrolSaveValidationService.IsSchema54GarrisonPatrolBatchClaimant(m_SaveData, batch)
				|| HST_AssassinationGuardSaveValidationService.IsSchema57MissionGuardBatchClaimant(m_SaveData, batch)
				|| HST_RescuePOWSaveValidationService.IsSchema58RescuePOWBatchClaimant(m_SaveData, batch))
				return true;
			foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
			{
				if (manifest && manifest.m_sManifestId == batch.m_sManifestId
					&& IsTypedManifestClaimant(manifest))
					return true;
			}
		}
		return false;
	}

	protected bool HasTypedTombstoneAuthority(HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		foreach (HST_ForceSettlementTombstoneState tombstone : m_SaveData.m_aForceSettlementTombstones)
		{
			if (!tombstone || tombstone.m_iOperationContractVersion == 0)
				continue;
			if ((!group.m_sOperationId.IsEmpty() && group.m_sOperationId == tombstone.m_sOperationId)
				|| (!group.m_sManifestId.IsEmpty() && group.m_sManifestId == tombstone.m_sManifestId))
				return true;
		}
		return false;
	}

	protected bool IsOpenFrozenTypedGroupId(string groupId)
	{
		if (groupId.IsEmpty())
			return false;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (group && group.m_sGroupId == groupId && IsOpenFrozenTypedGroup(group))
				return true;
		}
		return false;
	}

	protected bool IsTypedOperationClaimant(HST_OperationRecordState operation, bool requireOpen)
	{
		if (!operation || operation.m_iContractVersion == 0)
			return false;
		if (requireOpen
			&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
			return false;
		return true;
	}

	protected bool MatchesOperationGroupIdentity(
		HST_OperationRecordState operation,
		HST_ActiveGroupState group)
	{
		if (!operation || !group)
			return false;
		if (!group.m_sOperationId.IsEmpty() && group.m_sOperationId == operation.m_sOperationId)
			return true;
		if (!group.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)
			return true;
		if (!group.m_sManifestId.IsEmpty() && group.m_sManifestId == operation.m_sManifestId)
			return true;
		if (!group.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == operation.m_sSpawnResultId)
			return true;
		if (!group.m_sForceId.IsEmpty() && group.m_sForceId == operation.m_sForceId)
			return true;
		if (!group.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId)
			return true;
		return false;
	}

	protected bool IsTypedSupportRequestClaimant(HST_SupportRequestState request, bool requireOpen)
	{
		if (!request)
			return false;
		if (request.m_iOperationContractVersion != 0)
			return true;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!IsTypedOperationClaimant(operation, requireOpen))
				continue;
			if ((!request.m_sOperationId.IsEmpty() && request.m_sOperationId == operation.m_sOperationId)
				|| (!request.m_sRequestId.IsEmpty() && request.m_sRequestId == operation.m_sSupportRequestId))
				return true;
		}
		return false;
	}

	protected bool IsTypedEnemyOrderClaimant(HST_EnemyOrderState order, bool requireOpen)
	{
		if (!order)
			return false;
		if (order.m_iOperationContractVersion != 0)
			return true;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!IsTypedOperationClaimant(operation, requireOpen))
				continue;
			if ((!order.m_sOperationId.IsEmpty() && order.m_sOperationId == operation.m_sOperationId)
				|| (!order.m_sOrderId.IsEmpty() && order.m_sOrderId == operation.m_sEnemyOrderId))
				return true;
		}
		return false;
	}

	protected bool IsTypedMissionClaimant(HST_ActiveMissionState mission, bool requireOpen)
	{
		if (!mission)
			return false;
		if (mission.m_iOperationContractVersion != 0)
			return true;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!IsTypedOperationClaimant(operation, requireOpen))
				continue;
			if ((!mission.m_sOperationId.IsEmpty() && mission.m_sOperationId == operation.m_sOperationId)
				|| (!mission.m_sInstanceId.IsEmpty() && mission.m_sInstanceId == operation.m_sMissionInstanceId))
				return true;
		}
		return false;
	}

	protected void CollectOpenFrozenContentReferences(
		array<string> siteIds,
		array<string> routeIds)
	{
		if (!siteIds || !routeIds)
			return;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!IsTypedOperationClaimant(operation, false))
				continue;
			AppendUniqueString(routeIds, operation.m_sCurrentRouteId);
			foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
			{
				if (group && MatchesOperationGroupIdentity(operation, group))
					AppendUniqueString(routeIds, group.m_sRouteId);
			}
			foreach (HST_SupportRequestState request : m_SaveData.m_aSupportRequests)
			{
				if (request && ((!request.m_sOperationId.IsEmpty()
					&& request.m_sOperationId == operation.m_sOperationId)
					|| (!request.m_sRequestId.IsEmpty()
						&& request.m_sRequestId == operation.m_sSupportRequestId)))
					AppendUniqueString(routeIds, request.m_sDeploymentRouteId);
			}
			foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
			{
				if (mission && ((!mission.m_sOperationId.IsEmpty()
					&& mission.m_sOperationId == operation.m_sOperationId)
					|| (!mission.m_sInstanceId.IsEmpty()
						&& mission.m_sInstanceId == operation.m_sMissionInstanceId)))
					AppendUniqueString(siteIds, mission.m_sSiteId);
			}
		}
		foreach (HST_ActiveGroupState typedGroup : m_SaveData.m_aActiveGroups)
		{
			if (IsOpenFrozenTypedGroup(typedGroup))
				AppendUniqueString(routeIds, typedGroup.m_sRouteId);
		}
		foreach (HST_SupportRequestState typedRequest : m_SaveData.m_aSupportRequests)
		{
			if (typedRequest && typedRequest.m_iOperationContractVersion != 0)
				AppendUniqueString(routeIds, typedRequest.m_sDeploymentRouteId);
		}
		foreach (HST_ActiveMissionState typedMission : m_SaveData.m_aActiveMissions)
		{
			if (typedMission && typedMission.m_iOperationContractVersion != 0)
				AppendUniqueString(siteIds, typedMission.m_sSiteId);
		}

		foreach (HST_GeneratedSiteState site : m_SaveData.m_aGeneratedSites)
		{
			if (site && siteIds.Contains(site.m_sSiteId))
				AppendUniqueString(routeIds, site.m_sRouteId);
		}
	}

	protected int NormalizeGeneratedContent(
		bool hasCanonicalZone,
		array<string> frozenSiteIds,
		array<string> frozenRouteIds)
	{
		int changed;
		if (!hasCanonicalZone)
			return changed;

		for (int siteIndex = m_SaveData.m_aGeneratedSites.Count() - 1; siteIndex >= 0; siteIndex--)
		{
			HST_GeneratedSiteState site = m_SaveData.m_aGeneratedSites[siteIndex];
			if (!site)
				continue;
			bool legacyOwned = site.m_sZoneId == LEGACY_ZONE_ID
				|| site.m_sSiteId.StartsWith("site_" + LEGACY_ZONE_ID + "_");
			if (!legacyOwned)
				continue;
			if (frozenSiteIds && frozenSiteIds.Contains(site.m_sSiteId))
			{
				changed += EnsureCanonicalGeneratedSiteClone(site);
				continue;
			}
			string canonicalSiteId = CanonicalizeGeneratedSiteId(site.m_sSiteId);
			HST_GeneratedSiteState canonicalSite = FindOtherGeneratedSite(canonicalSiteId, site);
			if (!canonicalSite)
			{
				RekeyGeneratedSite(site, canonicalSiteId);
				changed++;
				continue;
			}
			m_SaveData.m_aGeneratedSites.Remove(siteIndex);
			changed++;
		}

		for (int routeIndex = m_SaveData.m_aGeneratedRoutes.Count() - 1; routeIndex >= 0; routeIndex--)
		{
			HST_GeneratedRouteState route = m_SaveData.m_aGeneratedRoutes[routeIndex];
			if (!route)
				continue;
			if (frozenRouteIds && frozenRouteIds.Contains(route.m_sRouteId))
			{
				changed += EnsureCanonicalGeneratedRouteClone(route);
				continue;
			}
			bool legacyOwned = route.m_sRouteId.StartsWith("route_" + LEGACY_ZONE_ID + "_");
			legacyOwned = legacyOwned || (route.m_sSourceZoneId == LEGACY_ZONE_ID
				&& route.m_sTargetZoneId == LEGACY_ZONE_ID);
			if (legacyOwned && (!frozenRouteIds || !frozenRouteIds.Contains(route.m_sRouteId)))
			{
				string canonicalRouteId = CanonicalizeGeneratedRouteId(route.m_sRouteId);
				HST_GeneratedRouteState canonicalRoute = FindOtherGeneratedRoute(canonicalRouteId, route);
				if (!canonicalRoute)
				{
					RekeyGeneratedRoute(route, canonicalRouteId);
					changed++;
					continue;
				}
				m_SaveData.m_aGeneratedRoutes.Remove(routeIndex);
				changed++;
				continue;
			}
			if (legacyOwned)
				continue;
			if (route.m_sSourceZoneId == LEGACY_ZONE_ID)
			{
				route.m_sSourceZoneId = CANONICAL_ZONE_ID;
				changed++;
			}
			if (route.m_sTargetZoneId == LEGACY_ZONE_ID)
			{
				route.m_sTargetZoneId = CANONICAL_ZONE_ID;
				changed++;
			}
		}
		return changed;
	}

	protected int EnsureCanonicalGeneratedSiteClone(HST_GeneratedSiteState source)
	{
		if (!source)
			return 0;
		string canonicalSiteId = CanonicalizeGeneratedSiteId(source.m_sSiteId);
		if (canonicalSiteId.IsEmpty() || canonicalSiteId == source.m_sSiteId
			|| FindOtherGeneratedSite(canonicalSiteId, source))
			return 0;

		HST_GeneratedSiteState clone = new HST_GeneratedSiteState();
		clone.m_sSiteId = source.m_sSiteId;
		clone.m_sZoneId = source.m_sZoneId;
		clone.m_sRouteId = source.m_sRouteId;
		clone.m_sSourceLayerName = source.m_sSourceLayerName;
		clone.m_sSourceCategory = source.m_sSourceCategory;
		clone.m_sSourceLayoutId = source.m_sSourceLayoutId;
		clone.m_eType = source.m_eType;
		clone.m_vPosition = source.m_vPosition;
		clone.m_vSecondaryPosition = source.m_vSecondaryPosition;
		clone.m_iRadiusMeters = source.m_iRadiusMeters;
		clone.m_iWeight = source.m_iWeight;
		clone.m_bValid = source.m_bValid;
		clone.m_bOccupied = source.m_bOccupied;
		clone.m_sOwnerFactionKey = source.m_sOwnerFactionKey;
		RekeyGeneratedSite(clone, canonicalSiteId);
		m_SaveData.m_aGeneratedSites.Insert(clone);
		return 1;
	}

	protected int EnsureCanonicalGeneratedRouteClone(HST_GeneratedRouteState source)
	{
		if (!source)
			return 0;
		string canonicalRouteId = CanonicalizeGeneratedRouteId(source.m_sRouteId);
		if (canonicalRouteId.IsEmpty() || canonicalRouteId == source.m_sRouteId
			|| FindOtherGeneratedRoute(canonicalRouteId, source))
			return 0;

		HST_GeneratedRouteState clone = new HST_GeneratedRouteState();
		clone.m_sRouteId = source.m_sRouteId;
		clone.m_sSourceZoneId = source.m_sSourceZoneId;
		clone.m_sTargetZoneId = source.m_sTargetZoneId;
		clone.m_sSourceLayerName = source.m_sSourceLayerName;
		clone.m_sSourceCategory = source.m_sSourceCategory;
		clone.m_sSourceLayoutId = source.m_sSourceLayoutId;
		clone.m_vStartPosition = source.m_vStartPosition;
		clone.m_vMidPosition = source.m_vMidPosition;
		clone.m_vEndPosition = source.m_vEndPosition;
		clone.m_iDistanceMeters = source.m_iDistanceMeters;
		clone.m_iWaypointCount = source.m_iWaypointCount;
		clone.m_bRoadRoute = source.m_bRoadRoute;
		clone.m_bValidatedForVehicles = source.m_bValidatedForVehicles;
		clone.m_sValidationFailureReason = source.m_sValidationFailureReason;
		foreach (HST_RouteWaypointState sourceWaypoint : source.m_aWaypoints)
		{
			if (!sourceWaypoint)
				continue;
			HST_RouteWaypointState waypoint = new HST_RouteWaypointState();
			waypoint.m_sRouteId = sourceWaypoint.m_sRouteId;
			waypoint.m_iIndex = sourceWaypoint.m_iIndex;
			waypoint.m_vPosition = sourceWaypoint.m_vPosition;
			waypoint.m_iRadiusMeters = sourceWaypoint.m_iRadiusMeters;
			waypoint.m_sHint = sourceWaypoint.m_sHint;
			clone.m_aWaypoints.Insert(waypoint);
		}
		RekeyGeneratedRoute(clone, canonicalRouteId);
		m_SaveData.m_aGeneratedRoutes.Insert(clone);
		return 1;
	}

	protected HST_GeneratedSiteState FindOtherGeneratedSite(
		string siteId,
		HST_GeneratedSiteState excluded)
	{
		if (siteId.IsEmpty())
			return null;
		foreach (HST_GeneratedSiteState site : m_SaveData.m_aGeneratedSites)
		{
			if (site && site != excluded && site.m_sSiteId == siteId)
				return site;
		}
		return null;
	}

	protected HST_GeneratedRouteState FindOtherGeneratedRoute(
		string routeId,
		HST_GeneratedRouteState excluded)
	{
		if (routeId.IsEmpty())
			return null;
		foreach (HST_GeneratedRouteState route : m_SaveData.m_aGeneratedRoutes)
		{
			if (route && route != excluded && route.m_sRouteId == routeId)
				return route;
		}
		return null;
	}

	protected void RekeyGeneratedSite(HST_GeneratedSiteState site, string canonicalSiteId)
	{
		if (!site)
			return;
		string oldSiteId = site.m_sSiteId;
		site.m_sSiteId = canonicalSiteId;
		site.m_sZoneId = CANONICAL_ZONE_ID;
		site.m_sRouteId = CanonicalizeGeneratedRouteId(site.m_sRouteId);
		HST_ZoneState canonicalZone = FindCanonicalZone();
		if (canonicalZone)
			site.m_sOwnerFactionKey = canonicalZone.m_sOwnerFactionKey;
		if (oldSiteId == "site_" + LEGACY_ZONE_ID + "_primary")
			site.m_eType = HST_EGeneratedSiteType.HST_SITE_RESOURCE;
		else if (oldSiteId == "site_" + LEGACY_ZONE_ID + "_stash")
		{
			site.m_eType = HST_EGeneratedSiteType.HST_SITE_CRASHSITE;
			site.m_sSourceLayerName = "SupplyCaches.layer";
			site.m_sSourceCategory = "salvage_anchor";
		}
	}

	protected void RekeyGeneratedRoute(HST_GeneratedRouteState route, string canonicalRouteId)
	{
		if (!route)
			return;
		route.m_sRouteId = canonicalRouteId;
		route.m_sSourceZoneId = ResolveCanonicalZoneId(route.m_sSourceZoneId);
		route.m_sTargetZoneId = ResolveCanonicalZoneId(route.m_sTargetZoneId);
		foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
		{
			if (waypoint)
				waypoint.m_sRouteId = canonicalRouteId;
		}
	}

	protected string CanonicalizeGeneratedSiteId(string siteId)
	{
		string prefix = "site_" + LEGACY_ZONE_ID;
		if (!siteId.StartsWith(prefix))
			return siteId;
		string suffix = siteId.Substring(prefix.Length(), siteId.Length() - prefix.Length());
		if (suffix == "_stash")
			suffix = "_crashsite";
		return "site_" + CANONICAL_ZONE_ID + suffix;
	}

	protected string CanonicalizeGeneratedRouteId(string routeId)
	{
		string routePrefix = "route_" + LEGACY_ZONE_ID;
		if (routeId.StartsWith(routePrefix))
			return "route_" + CANONICAL_ZONE_ID
				+ routeId.Substring(routePrefix.Length(), routeId.Length() - routePrefix.Length());
		string qrfPrefix = "qrf_" + LEGACY_ZONE_ID;
		if (routeId.StartsWith(qrfPrefix))
			return "qrf_" + CANONICAL_ZONE_ID
				+ routeId.Substring(qrfPrefix.Length(), routeId.Length() - qrfPrefix.Length());
		return routeId;
	}

	protected void AppendUniqueString(array<string> values, string value)
	{
		if (values && !value.IsEmpty() && !values.Contains(value))
			values.Insert(value);
	}

	protected int NormalizeLegacyGarrisons(bool convertedLegacyZone, bool hasCanonicalZone)
	{
		int changed;
		if (!hasCanonicalZone)
			return changed;

		for (int garrisonIndex = m_SaveData.m_aGarrisons.Count() - 1; garrisonIndex >= 0; garrisonIndex--)
		{
			HST_GarrisonState garrison = m_SaveData.m_aGarrisons[garrisonIndex];
			if (!garrison || garrison.m_sZoneId != LEGACY_ZONE_ID)
				continue;

			bool hasExactBacklink = garrison.m_aAcceptedManifestIds.Count() > 0;
			if (convertedLegacyZone && (garrison.m_iInfantryCount > 0 || garrison.m_iVehicleCount > 0))
				PreserveConvertedAggregateGarrison(garrison);

			if (!hasExactBacklink)
			{
				m_SaveData.m_aGarrisons.Remove(garrisonIndex);
				changed++;
				continue;
			}

			HST_GarrisonState keeper = FindLegacyCompatibilityGarrison(garrison.m_sFactionKey);
			if (keeper && keeper != garrison)
			{
				AppendUniqueStrings(keeper.m_aAcceptedManifestIds, garrison.m_aAcceptedManifestIds);
				m_SaveData.m_aGarrisons.Remove(garrisonIndex);
				changed++;
				continue;
			}

			garrison.m_sGarrisonId = HST_StableIdService.BuildGarrisonId(LEGACY_ZONE_ID, garrison.m_sFactionKey);
			garrison.m_iInfantryCount = 0;
			garrison.m_iVehicleCount = 0;
			changed++;
			RemoveCompatibilityBacklinksFromCanonicalGarrison(garrison);
		}
		return changed;
	}

	protected void PreserveConvertedAggregateGarrison(HST_GarrisonState legacyGarrison)
	{
		HST_GarrisonState canonicalGarrison = FindCanonicalGarrison(legacyGarrison.m_sFactionKey);
		if (canonicalGarrison)
			return;

		canonicalGarrison = new HST_GarrisonState();
		canonicalGarrison.m_sZoneId = CANONICAL_ZONE_ID;
		canonicalGarrison.m_sFactionKey = legacyGarrison.m_sFactionKey;
		canonicalGarrison.m_sGarrisonId = HST_StableIdService.BuildGarrisonId(CANONICAL_ZONE_ID, legacyGarrison.m_sFactionKey);
		canonicalGarrison.m_iInfantryCount = Math.Max(0, legacyGarrison.m_iInfantryCount);
		canonicalGarrison.m_iVehicleCount = Math.Max(0, legacyGarrison.m_iVehicleCount);
		m_SaveData.m_aGarrisons.Insert(canonicalGarrison);
	}

	protected HST_GarrisonState FindCanonicalGarrison(string factionKey)
	{
		foreach (HST_GarrisonState garrison : m_SaveData.m_aGarrisons)
		{
			if (garrison && garrison.m_sZoneId == CANONICAL_ZONE_ID
				&& garrison.m_sFactionKey == factionKey)
				return garrison;
		}
		return null;
	}

	protected HST_GarrisonState FindLegacyCompatibilityGarrison(string factionKey)
	{
		foreach (HST_GarrisonState garrison : m_SaveData.m_aGarrisons)
		{
			if (garrison && garrison.m_sZoneId == LEGACY_ZONE_ID
				&& garrison.m_sFactionKey == factionKey
				&& garrison.m_aAcceptedManifestIds.Count() > 0)
				return garrison;
		}
		return null;
	}

	protected void RemoveCompatibilityBacklinksFromCanonicalGarrison(HST_GarrisonState compatibilityGarrison)
	{
		HST_GarrisonState canonicalGarrison = FindCanonicalGarrison(compatibilityGarrison.m_sFactionKey);
		if (!canonicalGarrison)
			return;
		foreach (string manifestId : compatibilityGarrison.m_aAcceptedManifestIds)
			RemoveString(canonicalGarrison.m_aAcceptedManifestIds, manifestId);
	}

	protected void AppendUniqueStrings(array<string> target, array<string> source)
	{
		foreach (string value : source)
		{
			if (!value.IsEmpty() && !target.Contains(value))
				target.Insert(value);
		}
	}

	protected void RemoveString(array<string> values, string value)
	{
		for (int valueIndex = values.Count() - 1; valueIndex >= 0; valueIndex--)
		{
			if (values[valueIndex] == value)
				values.Remove(valueIndex);
		}
	}

	protected int RemoveLegacyCivilianRows()
	{
		int removed;
		for (int index = m_SaveData.m_aCivilianZones.Count() - 1; index >= 0; index--)
		{
			HST_CivilianZoneState civilian = m_SaveData.m_aCivilianZones[index];
			if (!civilian || civilian.m_sZoneId != LEGACY_ZONE_ID)
				continue;
			m_SaveData.m_aCivilianZones.Remove(index);
			removed++;
		}
		return removed;
	}

	protected int RemoveLegacyTownInfluenceRows()
	{
		int removed;
		for (int index = m_SaveData.m_aTownInfluenceEvents.Count() - 1; index >= 0; index--)
		{
			HST_TownInfluenceEventState influence = m_SaveData.m_aTownInfluenceEvents[index];
			if (!influence || influence.m_sZoneId != LEGACY_ZONE_ID)
				continue;
			m_SaveData.m_aTownInfluenceEvents.Remove(index);
			removed++;
		}
		return removed;
	}

	protected int RemoveLegacyZoneMarkers()
	{
		int removed;
		for (int index = m_SaveData.m_aMapMarkers.Count() - 1; index >= 0; index--)
		{
			HST_MapMarkerState marker = m_SaveData.m_aMapMarkers[index];
			if (!marker || (marker.m_sMarkerId != LEGACY_ZONE_MARKER_ID
				&& marker.m_sLinkedId != LEGACY_ZONE_ID))
				continue;
			m_SaveData.m_aMapMarkers.Remove(index);
			removed++;
		}
		return removed;
	}

	protected int NormalizeEnemySupportLedgers(bool hasCanonicalZone)
	{
		int changed;
		if (!hasCanonicalZone)
			return changed;
		for (int index = m_SaveData.m_aEnemySupportLedgers.Count() - 1; index >= 0; index--)
		{
			HST_EnemySupportLedgerState ledger = m_SaveData.m_aEnemySupportLedgers[index];
			if (!ledger || ledger.m_sZoneId != LEGACY_ZONE_ID)
				continue;
			HST_EnemySupportLedgerState canonicalLedger = FindCanonicalLedger(ledger.m_sFactionKey);
			if (canonicalLedger)
			{
				MergeEnemySupportLedger(canonicalLedger, ledger);
				m_SaveData.m_aEnemySupportLedgers.Remove(index);
			}
			else
				ledger.m_sZoneId = CANONICAL_ZONE_ID;
			changed++;
		}
		return changed;
	}

	protected HST_EnemySupportLedgerState FindCanonicalLedger(string factionKey)
	{
		foreach (HST_EnemySupportLedgerState ledger : m_SaveData.m_aEnemySupportLedgers)
		{
			if (ledger && ledger.m_sZoneId == CANONICAL_ZONE_ID
				&& ledger.m_sFactionKey == factionKey)
				return ledger;
		}
		return null;
	}

	protected void MergeEnemySupportLedger(
		HST_EnemySupportLedgerState canonicalLedger,
		HST_EnemySupportLedgerState legacyLedger)
	{
		if (!canonicalLedger || !legacyLedger || canonicalLedger == legacyLedger)
			return;

		ExpireStaleLedgerWindows(canonicalLedger);
		ExpireStaleLedgerWindows(legacyLedger);
		int canonicalDecisionSecond = Math.Max(canonicalLedger.m_iLastDamageSecond,
			Math.Max(canonicalLedger.m_iLastSpendSecond, canonicalLedger.m_iCooldownUntilSecond));
		int legacyDecisionSecond = Math.Max(legacyLedger.m_iLastDamageSecond,
			Math.Max(legacyLedger.m_iLastSpendSecond, legacyLedger.m_iCooldownUntilSecond));
		if (!legacyLedger.m_sLastDecisionReason.IsEmpty()
			&& (canonicalLedger.m_sLastDecisionReason.IsEmpty()
				|| legacyDecisionSecond > canonicalDecisionSecond))
			canonicalLedger.m_sLastDecisionReason = legacyLedger.m_sLastDecisionReason;
		canonicalLedger.m_iAttackSpent = Math.Max(0, canonicalLedger.m_iAttackSpent)
			+ Math.Max(0, legacyLedger.m_iAttackSpent);
		canonicalLedger.m_iSupportSpent = Math.Max(0, canonicalLedger.m_iSupportSpent)
			+ Math.Max(0, legacyLedger.m_iSupportSpent);
		canonicalLedger.m_iRefundedAttackResources = Math.Max(0, canonicalLedger.m_iRefundedAttackResources)
			+ Math.Max(0, legacyLedger.m_iRefundedAttackResources);
		canonicalLedger.m_iRefundedSupportResources = Math.Max(0, canonicalLedger.m_iRefundedSupportResources)
			+ Math.Max(0, legacyLedger.m_iRefundedSupportResources);
		canonicalLedger.m_iRecentDamageScore = Math.Min(100, Math.Max(
			Math.Max(0, canonicalLedger.m_iRecentDamageScore),
			Math.Max(0, legacyLedger.m_iRecentDamageScore)));
		canonicalLedger.m_iLastDamageSecond = Math.Max(
			canonicalLedger.m_iLastDamageSecond,
			legacyLedger.m_iLastDamageSecond);
		canonicalLedger.m_iLastSpendSecond = Math.Max(
			canonicalLedger.m_iLastSpendSecond,
			legacyLedger.m_iLastSpendSecond);
		canonicalLedger.m_iCooldownUntilSecond = Math.Max(
			canonicalLedger.m_iCooldownUntilSecond,
			legacyLedger.m_iCooldownUntilSecond);
	}

	protected void ExpireStaleLedgerWindows(HST_EnemySupportLedgerState ledger)
	{
		if (!ledger)
			return;
		int nowSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		if (nowSecond - ledger.m_iLastSpendSecond > HST_EnemyDirectorService.SUPPORT_SPEND_WINDOW_SECONDS)
		{
			ledger.m_iAttackSpent = 0;
			ledger.m_iSupportSpent = 0;
		}
		if (nowSecond - ledger.m_iLastDamageSecond > HST_EnemyDirectorService.RECENT_DAMAGE_WINDOW_SECONDS)
			ledger.m_iRecentDamageScore = 0;
	}

	protected bool HasEvent(string eventId)
	{
		foreach (HST_CampaignEventState eventState : m_SaveData.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
	}
}
