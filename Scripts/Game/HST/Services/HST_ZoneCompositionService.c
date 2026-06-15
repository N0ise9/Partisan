class HST_ZoneCompositionService
{
	static const string SLOT_INFANTRY = "infantry";
	static const string SLOT_VEHICLE = "vehicle";
	static const string SLOT_PROP = "prop";
	static const string SLOT_STATIC = "static";
	static const string SLOT_PATROL = "patrol";
	static const string PROP_TENT = "{01AE5FD77A9A4C21}Prefabs/Structures/Military/Camps/TentSmallUS_01/TentSmallUS_01.et";
	static const string PROP_SUPPLY_CACHE = "{AB1A97B1BAE8C395}Prefabs/Compositions/Slotted/SlotFlatSmall/SupplyCache_S_FIA_01.et";
	static const string PROP_CARGO = "{2C303FA30DF3D73F}Prefabs/Props/Military/AmmoBoxes/US/EquipmentBoxWooden_Ammunition_01_US.et";
	static const string PROP_HOLD_MARKER = "{2C303FA30DF3D73F}Prefabs/Props/Military/AmmoBoxes/US/EquipmentBoxWooden_Ammunition_01_US.et";
	static const string PROP_DESTROY_TARGET = "{7E2380494811A5FB}Prefabs/Structures/Infrastructure/Towers/TransmitterTower_01/TransmitterTower_01_medium.et";
	static const string PROP_RESOURCE_CACHE = "{2C303FA30DF3D73F}Prefabs/Props/Military/AmmoBoxes/US/EquipmentBoxWooden_Ammunition_01_US.et";
	static const string PROP_SANDBAG_CANDIDATE = "Prefabs/Structures/Military/Sandbags/SandbagWall_01.et";
	static const string MISSION_DESTROY_RADIO_TOWER = "destroy_radio_tower";
	static const string MISSION_STOP_TOWER_REBUILD = "dynamic_stop_tower_rebuild";
	static const string ROLE_DESTROY_TARGET = "destroy_target";
	static const float HQ_SAFE_RADIUS_METERS = 900.0;

	protected ref array<string> m_aRuntimeZoneIds = {};
	protected ref array<string> m_aRuntimeSlotIds = {};
	protected ref array<string> m_aRuntimePrefabs = {};
	protected ref array<IEntity> m_aRuntimeEntities = {};
	protected ref array<string> m_aReportedSkipKeys = {};
	protected int m_iLastSpawnedCount;
	protected int m_iLastSkippedPrefabCount;
	protected string m_sLastFailedZoneId;
	protected string m_sLastFailedSlotId;
	protected string m_sLastFailedPrefab;
	protected string m_sLastFailedSlotReason;

	array<ref HST_ZoneSpawnSlotState> BuildZoneSpawnSlots(HST_CampaignState state, HST_ZoneState zone)
	{
		array<ref HST_ZoneSpawnSlotState> slots = {};
		if (!state || !zone)
			return slots;

		HST_ZoneCompositionDefinition definition = BuildDefinition(zone);
		vector anchor = ResolvePrimaryAnchor(state, zone);
		vector secondaryAnchor = ResolveSecondaryAnchor(state, zone, anchor);
		float captureRadius = ResolveCaptureRadius(zone);
		float infantryRadius = Math.Max(12.0, Math.Min(55.0, captureRadius * 0.35));
		float vehicleRadius = Math.Max(22.0, Math.Min(85.0, captureRadius * 0.55));
		float propRadius = Math.Max(8.0, Math.Min(30.0, captureRadius * 0.22));
		float patrolRadius = Math.Max(35.0, Math.Min(110.0, captureRadius * 0.75));

		AddSlotsForKind(state, zone, slots, SLOT_INFANTRY, definition.m_iInfantrySlotCount, anchor, infantryRadius, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 11);
		AddSlotsForKind(state, zone, slots, SLOT_VEHICLE, definition.m_iVehicleSlotCount, secondaryAnchor, vehicleRadius, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true, 23);
		AddSlotsForKind(state, zone, slots, SLOT_PROP, definition.m_iPropSlotCount, anchor, propRadius, HST_WorldPositionService.PROP_GROUND_OFFSET, false, 37);
		AddSlotsForKind(state, zone, slots, SLOT_STATIC, 1, anchor, propRadius + 6.0, HST_WorldPositionService.PROP_GROUND_OFFSET, false, 41);
		AddSlotsForKind(state, zone, slots, SLOT_PATROL, definition.m_iPatrolPointCount, anchor, patrolRadius, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 53);
		return slots;
	}

	bool EnsureZoneComposition(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return false;

		if (HasRuntimeZone(zone.m_sZoneId))
			return false;

		m_iLastSpawnedCount = 0;
		m_iLastSkippedPrefabCount = 0;
		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
		{
			RecordFailure(zone.m_sZoneId, "composition", "", zone.m_vPosition, "respawn system unavailable");
			return false;
		}

		array<ref HST_ZoneSpawnSlotState> slots = BuildZoneSpawnSlots(state, zone);
		int propIndex;
		foreach (HST_ZoneSpawnSlotState slot : slots)
		{
			if (!slot || (slot.m_sKind != SLOT_PROP && slot.m_sKind != SLOT_STATIC))
				continue;

			string prefab = SelectPropPrefab(zone, slot, propIndex);
			propIndex++;
			if (prefab.IsEmpty())
				continue;

			if (ShouldSkipRadioTowerStaticForMissionTarget(state, zone, slot, prefab))
			{
				RecordSkip(zone.m_sZoneId, slot.m_sSlotId, prefab, slot.m_vPosition, "active mission target owns radio tower");
				continue;
			}

			if (!IsValidPrefab(prefab))
			{
				RecordFailure(zone.m_sZoneId, slot.m_sSlotId, prefab, slot.m_vPosition, "optional prefab missing or invalid");
				continue;
			}

			GenericEntity entity = HST_WorldPositionService.SpawnPrefab(prefab, slot.m_vPosition, slot.m_vAngles);
			if (!entity)
			{
				RecordFailure(zone.m_sZoneId, slot.m_sSlotId, prefab, slot.m_vPosition, "prefab spawn returned null");
				continue;
			}

			HST_WorldPositionService.ApplyUprightEntityTransform(entity, slot.m_vPosition, slot.m_vAngles);
			m_aRuntimeZoneIds.Insert(zone.m_sZoneId);
			m_aRuntimeSlotIds.Insert(slot.m_sSlotId);
			m_aRuntimePrefabs.Insert(prefab);
			m_aRuntimeEntities.Insert(entity);
			m_iLastSpawnedCount++;
			Print(string.Format("h-istasi zone composition | spawned %1 at %2 slot %3 prefab %4", zone.m_sZoneId, slot.m_vPosition, slot.m_sSlotId, prefab));
		}

		if (m_iLastSpawnedCount <= 0)
			RecordFailure(zone.m_sZoneId, "composition", "", zone.m_vPosition, "no composition props spawned");

		return m_iLastSpawnedCount > 0;
	}

	bool CleanupZoneComposition(string zoneId)
	{
		if (zoneId.IsEmpty())
			return false;

		bool changed;
		for (int i = m_aRuntimeZoneIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeZoneIds[i] != zoneId)
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			if (entity)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);

			m_aRuntimeZoneIds.Remove(i);
			m_aRuntimeSlotIds.Remove(i);
			m_aRuntimePrefabs.Remove(i);
			m_aRuntimeEntities.Remove(i);
			changed = true;
		}

		if (changed)
			Print(string.Format("h-istasi zone composition | cleaned runtime props for %1", zoneId));

		return changed;
	}

	HST_ZoneSpawnSlotState SelectSlot(array<ref HST_ZoneSpawnSlotState> slots, string kind, int preferredIndex)
	{
		if (!slots || kind.IsEmpty())
			return null;

		int seen;
		foreach (HST_ZoneSpawnSlotState slot : slots)
		{
			if (!slot || slot.m_sKind != kind || slot.m_bOccupied)
				continue;

			if (seen >= preferredIndex)
			{
				slot.m_bOccupied = true;
				return slot;
			}

			seen++;
		}

		foreach (HST_ZoneSpawnSlotState fallback : slots)
		{
			if (!fallback || fallback.m_sKind != kind || fallback.m_bOccupied)
				continue;

			fallback.m_bOccupied = true;
			return fallback;
		}

		return null;
	}

	void ReportPatrolWaypointUnavailable(string zoneId)
	{
		m_sLastFailedZoneId = zoneId;
		m_sLastFailedSlotId = zoneId + "_patrol";
		m_sLastFailedPrefab = "";
		m_sLastFailedSlotReason = "patrol waypoint API unavailable";
		Print(string.Format("h-istasi zone composition | %1 | patrol waypoint API unavailable; distributed guard placement retained", zoneId), LogLevel.WARNING);
	}

	string BuildCompositionReport(HST_CampaignState state)
	{
		int activeZoneCount = CountUniqueRuntimeZones();
		string lastPrefab = m_sLastFailedPrefab;
		if (lastPrefab.IsEmpty())
			lastPrefab = "none";
		string lastSlot = m_sLastFailedSlotId;
		if (lastSlot.IsEmpty())
			lastSlot = "none";
		string lastReason = m_sLastFailedSlotReason;
		if (lastReason.IsEmpty())
			lastReason = "none";

		return string.Format("h-istasi zone composition | active zones %1 | runtime props %2 | last spawned %3 | skipped prefabs %4 | last failed prefab %5 | last failed slot %6 | last reason %7", activeZoneCount, m_aRuntimeEntities.Count(), m_iLastSpawnedCount, m_iLastSkippedPrefabCount, lastPrefab, lastSlot, lastReason);
	}

	int GetLastSpawnedCount()
	{
		return m_iLastSpawnedCount;
	}

	int GetRuntimePropCount()
	{
		return m_aRuntimeEntities.Count();
	}

	int GetActiveRuntimeZoneCount()
	{
		return CountUniqueRuntimeZones();
	}

	int GetLastSkippedPrefabCount()
	{
		return m_iLastSkippedPrefabCount;
	}

	string GetLastFailedPrefab()
	{
		return m_sLastFailedPrefab;
	}

	string GetLastFailedSlotReason()
	{
		return m_sLastFailedSlotReason;
	}

	protected HST_ZoneCompositionDefinition BuildDefinition(HST_ZoneState zone)
	{
		HST_ZoneCompositionDefinition definition = new HST_ZoneCompositionDefinition();
		if (!zone)
			return definition;

		definition.m_sCompositionId = zone.m_sCompositionId;
		definition.m_sSpawnProfileId = zone.m_sSpawnProfileId;
		definition.m_iInfantrySlotCount = Math.Max(2, Math.Min(8, zone.m_iGarrisonSlots));
		definition.m_iVehicleSlotCount = ResolveVehicleSlotCount(zone);
		definition.m_iPropSlotCount = ResolvePropSlotCount(zone);
		definition.m_iPatrolPointCount = 4;
		AddPropEntry(definition, SLOT_PROP, PROP_TENT, 1, true);
		AddPropEntry(definition, SLOT_PROP, PROP_SUPPLY_CACHE, 1, true);
		AddPropEntry(definition, SLOT_PROP, PROP_SANDBAG_CANDIDATE, 1, true);
		return definition;
	}

	protected void AddPropEntry(HST_ZoneCompositionDefinition definition, string kind, string prefab, int count, bool optional)
	{
		if (!definition || prefab.IsEmpty())
			return;

		HST_ZoneCompositionPropEntry entry = new HST_ZoneCompositionPropEntry();
		entry.m_sKind = kind;
		entry.m_sPrefab = prefab;
		entry.m_iCount = Math.Max(1, count);
		entry.m_bOptional = optional;
		definition.m_aProps.Insert(entry);
	}

	protected void AddSlotsForKind(HST_CampaignState state, HST_ZoneState zone, array<ref HST_ZoneSpawnSlotState> slots, string kind, int count, vector anchor, float radius, float verticalOffset, bool vehicleSlot, int salt)
	{
		for (int i = 0; i < count; i++)
		{
			HST_ZoneSpawnSlotState slot;
			if (TryBuildSlot(state, zone, kind, i, anchor, radius, verticalOffset, vehicleSlot, salt, slot))
				slots.Insert(slot);
		}
	}

	protected bool TryBuildSlot(HST_CampaignState state, HST_ZoneState zone, string kind, int index, vector anchor, float radius, float verticalOffset, bool vehicleSlot, int salt, out HST_ZoneSpawnSlotState slot)
	{
		slot = null;
		if (!zone)
			return false;

		int seed = BuildZoneSeed(state, zone, salt + index * 17);
		vector candidate = anchor + BuildOffset(seed, radius, index);
		vector resolved;
		bool resolvedSafe;
		if (vehicleSlot)
			resolvedSafe = HST_WorldPositionService.TryResolveVehicleSpawnPosition(candidate, resolved, true);
		else
			resolvedSafe = HST_WorldPositionService.TryResolveSafeGroundPosition(candidate, verticalOffset, resolved, true, 2.0);

		string slotId = string.Format("%1_%2_%3", zone.m_sZoneId, kind, index);
		if (!resolvedSafe)
		{
			RecordFailure(zone.m_sZoneId, slotId, "", candidate, "water or unsafe slope");
			return false;
		}

		if (IsInsideHQSafeRadius(state, resolved))
		{
			RecordFailure(zone.m_sZoneId, slotId, "", resolved, "inside HQ safe radius");
			return false;
		}

		slot = new HST_ZoneSpawnSlotState();
		slot.m_sZoneId = zone.m_sZoneId;
		slot.m_sSlotId = slotId;
		slot.m_sKind = kind;
		slot.m_vPosition = resolved;
		slot.m_vAngles = HST_WorldPositionService.BuildUprightAngles(PositiveMod(seed * 29 + index * 43, 360));
		slot.m_bOccupied = false;
		return true;
	}

	protected string SelectPropPrefab(HST_ZoneState zone, HST_ZoneSpawnSlotState slot, int propIndex)
	{
		if (!zone || !slot)
			return "";

		if (slot.m_sKind == SLOT_STATIC)
		{
			if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
				return PROP_DESTROY_TARGET;

			return PROP_HOLD_MARKER;
		}

		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE || zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
		{
			if (propIndex == 0)
				return PROP_RESOURCE_CACHE;
			if (propIndex == 1)
				return PROP_CARGO;
		}

		if (propIndex == 0)
			return PROP_TENT;
		if (propIndex == 1)
			return PROP_SUPPLY_CACHE;
		if (propIndex == 2)
			return PROP_SANDBAG_CANDIDATE;

		return PROP_HOLD_MARKER;
	}

	protected bool ShouldSkipRadioTowerStaticForMissionTarget(HST_CampaignState state, HST_ZoneState zone, HST_ZoneSpawnSlotState slot, string prefab)
	{
		if (!state || !zone || !slot || prefab != PROP_DESTROY_TARGET)
			return false;
		if (zone.m_eType != HST_EZoneType.HST_ZONE_RADIO_TOWER || slot.m_sKind != SLOT_STATIC)
			return false;

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_sTargetZoneId != zone.m_sZoneId)
				continue;
			if (mission.m_sMissionId != MISSION_DESTROY_RADIO_TOWER && mission.m_sMissionId != MISSION_STOP_TOWER_REBUILD)
				continue;
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE && HasLiveMissionDestroyTarget(state, mission.m_sInstanceId))
				return true;
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED && HasCompletedMissionDestroyTargetObjective(state, mission.m_sInstanceId))
				return true;
		}

		return false;
	}

	protected bool HasLiveMissionDestroyTarget(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != instanceId || asset.m_sRole != ROLE_DESTROY_TARGET)
				continue;
			if (!asset.m_bDestroyed && !asset.m_bDelivered)
				return true;
		}

		return false;
	}

	protected bool HasCompletedMissionDestroyTargetObjective(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return false;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != instanceId)
				continue;
			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET && objective.m_bComplete && !objective.m_bFailed)
				return true;
		}

		return false;
	}

	protected vector ResolvePrimaryAnchor(HST_CampaignState state, HST_ZoneState zone)
	{
		HST_GeneratedSiteState site = ResolvePrimarySite(state, zone);
		if (site)
			return site.m_vPosition;

		return zone.m_vPosition;
	}

	protected vector ResolveSecondaryAnchor(HST_CampaignState state, HST_ZoneState zone, vector fallback)
	{
		HST_GeneratedSiteState site = ResolvePrimarySite(state, zone);
		if (site && HasPosition(site.m_vSecondaryPosition))
			return site.m_vSecondaryPosition;

		if (zone && !zone.m_sPatrolRouteId.IsEmpty())
		{
			HST_GeneratedRouteState route = state.FindGeneratedRoute(zone.m_sPatrolRouteId);
			if (route && HasPosition(route.m_vMidPosition))
				return route.m_vMidPosition;
		}

		return fallback;
	}

	protected HST_GeneratedSiteState ResolvePrimarySite(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return null;

		if (!zone.m_sMissionSiteId.IsEmpty())
		{
			HST_GeneratedSiteState direct = state.FindGeneratedSite(zone.m_sMissionSiteId);
			if (direct)
				return direct;
		}

		foreach (HST_GeneratedSiteState site : state.m_aGeneratedSites)
		{
			if (!site || site.m_sZoneId != zone.m_sZoneId)
				continue;

			if (site.m_eType == HST_EGeneratedSiteType.HST_SITE_OUTPOST || site.m_eType == HST_EGeneratedSiteType.HST_SITE_RESOURCE || site.m_eType == HST_EGeneratedSiteType.HST_SITE_FACTORY || site.m_eType == HST_EGeneratedSiteType.HST_SITE_RADIO || site.m_eType == HST_EGeneratedSiteType.HST_SITE_AIRFIELD || site.m_eType == HST_EGeneratedSiteType.HST_SITE_SEAPORT || site.m_eType == HST_EGeneratedSiteType.HST_SITE_TOWN_CENTER)
				return site;
		}

		return null;
	}

	protected int ResolveVehicleSlotCount(HST_ZoneState zone)
	{
		if (!zone)
			return 1;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return 2;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST || zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY || zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			return 1;

		return 1;
	}

	protected int ResolvePropSlotCount(HST_ZoneState zone)
	{
		if (!zone)
			return 2;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT || zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
			return 3;

		return 2;
	}

	protected float ResolveCaptureRadius(HST_ZoneState zone)
	{
		if (!zone || zone.m_iCaptureRadiusMeters <= 0)
			return 80.0;

		return zone.m_iCaptureRadiusMeters;
	}

	protected vector BuildOffset(int seed, float radius, int index)
	{
		float angle = PositiveMod(seed + index * 67, 360) * 0.0174533;
		vector offset = "0 0 0";
		offset[0] = Math.Sin(angle) * radius;
		offset[2] = Math.Cos(angle) * radius;
		return offset;
	}

	protected int BuildZoneSeed(HST_CampaignState state, HST_ZoneState zone, int salt)
	{
		int seed = salt * 31;
		if (state)
			seed += state.m_iCampaignSeed * 17;
		if (zone)
		{
			seed += zone.m_iPriority * 37;
			seed += zone.m_iGarrisonSlots * 23;
			seed += zone.m_iCaptureRadiusMeters * 5;
			seed += zone.m_iActivationRadiusMeters * 3;
			seed += zone.m_sZoneId.Length() * 101;
			seed += zone.m_sCompositionId.Length() * 43;
			seed += zone.m_sSpawnProfileId.Length() * 47;
			seed += Math.Round(zone.m_vPosition[0]) + Math.Round(zone.m_vPosition[2]);
		}

		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected bool IsInsideHQSafeRadius(HST_CampaignState state, vector position)
	{
		if (!state || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, position) <= HQ_SAFE_RADIUS_METERS * HQ_SAFE_RADIUS_METERS;
	}

	protected bool IsValidPrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
			return false;

		return true;
	}

	protected void RecordFailure(string zoneId, string slotId, string prefab, vector position, string reason)
	{
		m_iLastSkippedPrefabCount++;
		m_sLastFailedZoneId = zoneId;
		m_sLastFailedSlotId = slotId;
		m_sLastFailedPrefab = prefab;
		m_sLastFailedSlotReason = reason;
		if (ShouldPrintSkipLog(zoneId, slotId, prefab, reason))
			Print(string.Format("h-istasi zone composition | spawn skipped | zone %1 | slot %2 | prefab %3 | pos %4 | reason %5", zoneId, slotId, prefab, position, reason), LogLevel.WARNING);
	}

	protected void RecordSkip(string zoneId, string slotId, string prefab, vector position, string reason)
	{
		m_iLastSkippedPrefabCount++;
		m_sLastFailedZoneId = zoneId;
		m_sLastFailedSlotId = slotId;
		m_sLastFailedPrefab = prefab;
		m_sLastFailedSlotReason = reason;
		if (ShouldPrintSkipLog(zoneId, slotId, prefab, reason))
			Print(string.Format("h-istasi zone composition | spawn skipped | zone %1 | slot %2 | prefab %3 | pos %4 | reason %5", zoneId, slotId, prefab, position, reason));
	}

	protected bool ShouldPrintSkipLog(string zoneId, string slotId, string prefab, string reason)
	{
		string key = string.Format("%1|%2|%3|%4", zoneId, slotId, prefab, reason);
		if (m_aReportedSkipKeys.Contains(key))
			return false;

		m_aReportedSkipKeys.Insert(key);
		return true;
	}

	protected bool HasRuntimeZone(string zoneId)
	{
		return m_aRuntimeZoneIds.Contains(zoneId);
	}

	protected int CountUniqueRuntimeZones()
	{
		array<string> seen = {};
		foreach (string zoneId : m_aRuntimeZoneIds)
		{
			if (!seen.Contains(zoneId))
				seen.Insert(zoneId);
		}

		return seen.Count();
	}

	protected bool HasPosition(vector position)
	{
		return position[0] != 0 || position[1] != 0 || position[2] != 0;
	}

	protected int PositiveMod(int value, int divisor)
	{
		if (divisor <= 0)
			return 0;

		int result = value - (value / divisor) * divisor;
		if (result < 0)
			result = result + divisor;

		return result;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
