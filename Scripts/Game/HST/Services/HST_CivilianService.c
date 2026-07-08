class HST_UndercoverEligibilityResult
{
	bool m_bEligible;
	string m_sIdentityId;
	string m_sZoneId;
	string m_sSummary;
	string m_sClothingReason;
	string m_sWeaponReason;
	string m_sVehicleReason;
	string m_sOffroadReason;
	string m_sEnemyProximityReason;
	string m_sWantedHeatReason;

	string BuildReport()
	{
		string report = string.Format(
			"h-istasi undercover eligibility | %1 | eligible %2 | zone %3 | clothing %4 | weapon %5 | vehicle %6 | offroad %7 | enemy proximity %8 | wanted heat %9",
			m_sIdentityId,
			m_bEligible,
			m_sZoneId,
			m_sClothingReason,
			m_sWeaponReason,
			m_sVehicleReason,
			m_sOffroadReason,
			m_sEnemyProximityReason,
			m_sWantedHeatReason
		);
		return report + string.Format(" | %1", m_sSummary);
	}
}

class HST_UndercoverEnforcementResult
{
	bool m_bChanged;
	bool m_bCompromised;
	bool m_bSuspicious;
	bool m_bCleared;
	bool m_bBlocked;
	string m_sIdentityId;
	string m_sZoneId;
	string m_sReason;
	string m_sDetectionSource;
	int m_iDetectionScore;
	int m_iWantedHeat;
	int m_iTownHeat;
	int m_iPolicePresence;
	int m_iRoadblockPresence;

	string BuildReport()
	{
		string report = string.Format(
			"h-istasi undercover enforcement | %1 | changed %2 | compromised %3 | suspicious %4 | clear %5 | blocked %6 | zone %7 | score %8 | player heat %9",
			m_sIdentityId,
			m_bChanged,
			m_bCompromised,
			m_bSuspicious,
			m_bCleared,
			m_bBlocked,
			m_sZoneId,
			m_iDetectionScore,
			m_iWantedHeat
		);
		return report + string.Format(" | town heat %1 | police %2 | roadblocks %3 | source %4 | reason %5", m_iTownHeat, m_iPolicePresence, m_iRoadblockPresence, m_sDetectionSource, m_sReason);
	}
}

class HST_CivilianService
{
	static const int MIN_CIVILIAN_CHARACTER_PREFABS = 1;

	static const int HEAT_DECAY_SECONDS = 300;
	static const int VEHICLE_HEAT_DECAY_SECONDS = 300;
	static const int VEHICLE_REPORT_DEFAULT_SECONDS = 300;
	static const int UNDERCOVER_RECHECK_SECONDS = 20;
	static const float PLAYER_USED_VEHICLE_DETACH_DISTANCE_METERS = 35.0;
	static const float TOWN_VEHICLE_ROAD_SEARCH_RADIUS_METERS = 140.0;
	static const float TOWN_VEHICLE_ROAD_FORWARD_TARGET_METERS = 25.0;
	static const float TOWN_VEHICLE_MIN_SEPARATION_METERS = 12.0;
	static const float TOWN_CIVILIAN_VEHICLE_BASE_RADIUS_METERS = 24.0;
	static const int TOWN_CIVILIAN_VEHICLE_RADIUS_VARIANCE_METERS = 37;
	static const float TOWN_MILITARY_VEHICLE_BASE_RADIUS_METERS = 36.0;
	static const int TOWN_MILITARY_VEHICLE_RADIUS_VARIANCE_METERS = 43;
	static const int TOWN_VEHICLE_POSITION_ATTEMPTS = 10;
	static const float CIVILIAN_WANDER_MIN_RADIUS_METERS = 16.0;
	static const int CIVILIAN_WANDER_RADIUS_VARIANCE_METERS = 36;
	static const float CIVILIAN_WANDER_COMPLETION_RADIUS_METERS = 3.0;
	static const float CIVILIAN_TRAFFIC_ROUTE_MIN_RADIUS_METERS = 220.0;
	static const int CIVILIAN_TRAFFIC_ROUTE_RADIUS_VARIANCE_METERS = 420;
	static const float CIVILIAN_TRAFFIC_WAYPOINT_RADIUS_METERS = 18.0;
	static const int TOWN_RESISTANCE_FLIP_FIA_SUPPORT = 65;
	static const int TOWN_RESISTANCE_FLIP_SUPPORT_MARGIN = 15;
	static const int TOWN_RESISTANCE_FLIP_MAX_HEAT = 5;
	static const int TOWN_ENEMY_FLIP_OCCUPIER_SUPPORT = 65;
	static const int TOWN_ENEMY_FLIP_SUPPORT_MARGIN = 15;
	static const int TOWN_ENEMY_FLIP_HEAT = 15;
	static const string CIVILIAN_FACTION_KEY = "CIV";
	static const string CIVILIAN_TRAFFIC_RUNTIME_KIND = "CIV_TRAFFIC_VEHICLE";
	static const string CIVILIAN_VEHICLE_ENTITY_CATALOG = "{7C53DF3E1349C5B8}Configs/EntityCatalog/CIV/Vehicles_EntityCatalog_CIV.conf";
	static const string CIVILIAN_AI_GROUP_PREFAB = "{6985327711303910}Prefabs/Groups/HST/HST_RuntimeEmptyGroup.et";
	static const string CIVILIAN_WANDER_WAYPOINT_PREFAB = "{FBA8DC8FDA0E770D}Prefabs/AI/Waypoints/AIWaypoint_Patrol_Hierarchy.et";
	static const string CIVILIAN_WANDER_CYCLE_WAYPOINT_PREFAB = "{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et";

	protected ref array<string> m_aRuntimeZoneIds = {};
	protected ref array<string> m_aRuntimeEntityZoneIds = {};
	protected ref array<string> m_aRuntimeEntityKinds = {};
	protected ref array<string> m_aRuntimeEntityFactionKeys = {};
	protected ref array<vector> m_aRuntimeEntitySpawnPositions = {};
	protected ref array<IEntity> m_aRuntimeEntities = {};
	protected ref array<IEntity> m_aRuntimeHelperOwners = {};
	protected ref array<IEntity> m_aRuntimeHelperEntities = {};
	protected int m_iRuntimeSpawnFailureCount;
	protected string m_sLastRuntimeSpawnFailurePrefab;
	protected bool m_bWarnedMissingCivilianCharacterPool;
	protected bool m_bWarnedMissingCivilianVehicleCatalog;
	protected HST_StrategicService m_Strategic;

	void SetStrategicService(HST_StrategicService strategic)
	{
		m_Strategic = strategic;
	}

	bool EnsureCivilianZones(HST_CampaignState state)
	{
		if (!state || state.m_aCivilianZones.Count() > 0)
			return false;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
				continue;

			HST_CivilianZoneState civilianZone = new HST_CivilianZoneState();
			civilianZone.m_sZoneId = zone.m_sZoneId;
			civilianZone.m_iReputation = 50;
			civilianZone.m_iCivilianPresence = Math.Max(4, zone.m_iIncomeValue / 8);
			civilianZone.m_iPolicePresence = Math.Max(1, zone.m_iGarrisonSlots / 6);
			civilianZone.m_iRoadblockPresence = 1;
			civilianZone.m_iFIASupport = Math.Max(0, Math.Min(100, 50 + zone.m_iSupport / 2));
			civilianZone.m_iOccupierSupport = Math.Max(0, Math.Min(100, 50 - zone.m_iSupport / 2 + civilianZone.m_iPolicePresence * 2 + civilianZone.m_iRoadblockPresence * 3));
			civilianZone.m_sLastIncidentReason = "initialized";
			civilianZone.m_sLastSecurityReason = "initialized";
			civilianZone.m_iLastSupportChangeSecond = state.m_iElapsedSeconds;
			civilianZone.m_bUndercoverRestricted = zone.m_iSupport < 25;
			civilianZone.m_iPopulationRemaining = Math.Max(20, civilianZone.m_iCivilianPresence * 8);
			civilianZone.m_sLastInfluenceKind = "initialized";
			civilianZone.m_sLastInfluenceReason = "initialized";
			state.m_aCivilianZones.Insert(civilianZone);
		}

		Print(string.Format("h-istasi | initialized %1 civilian town record(s)", state.m_aCivilianZones.Count()));
		return true;
	}

	bool Tick(HST_CampaignState state, int elapsedSeconds)
	{
		if (!state || elapsedSeconds <= 0)
			return false;

		bool changed;
		foreach (HST_CivilianZoneState civilianZone : state.m_aCivilianZones)
		{
			if (!civilianZone || civilianZone.m_iWantedHeat <= 0)
				continue;

			if (state.m_iElapsedSeconds < civilianZone.m_iLastIncidentSecond + HEAT_DECAY_SECONDS)
				continue;

			civilianZone.m_iWantedHeat = Math.Max(0, civilianZone.m_iWantedHeat - 1);
			civilianZone.m_iLastIncidentSecond = state.m_iElapsedSeconds;
			changed = true;
		}

		foreach (HST_PlayerUndercoverState undercover : state.m_aUndercoverPlayers)
		{
			if (!undercover || undercover.m_iWantedHeat <= 0)
				continue;

			if (undercover.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_COMPROMISED && state.m_iElapsedSeconds < undercover.m_iCompromisedUntilSecond)
				continue;

			undercover.m_iWantedHeat = Math.Max(0, undercover.m_iWantedHeat - 1);
			if (undercover.m_iWantedHeat == 0)
			{
				undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
				undercover.m_sLastReason = "heat cooled";
			}

			changed = true;
		}

		foreach (HST_RuntimeVehicleState vehicle : state.m_aRuntimeVehicles)
		{
			if (TickRuntimeVehicleHeat(state, vehicle))
				changed = true;
		}

		return RefreshTownInfluenceAggregates(state) || changed;
	}

	bool UpdatePhysicalTownPopulation(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance)
	{
		if (!state || !balance)
			return false;

		HST_DefaultCatalog.EnsureCivilianPools(balance);
		PruneDeletedRuntimeEntities(state);
		bool changed = PruneAmbientTrafficOutsideRenderBubble(state, balance);

		if (!balance.m_bCivilianPopulationEnabled)
		{
			bool cleaned = CleanupAllRuntimeEntities(state);
			return PublishRuntimeDiagnostics(state) || cleaned || changed;
		}

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			if (zone.m_bActive)
			{
				if (!HasRuntimeZone(zone.m_sZoneId))
					changed = SpawnActiveZoneRuntime(state, preset, balance, zone) || changed;
				else if (MaintainActiveZoneTraffic(state, preset, balance, zone))
					changed = true;

				continue;
			}

			if (CleanupZoneRuntimeEntities(state, zone.m_sZoneId))
				changed = true;
		}

		return PublishRuntimeDiagnostics(state) || changed;
	}

	bool UpdatePhysicalTownPopulationForZone(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, string zoneId, bool active)
	{
		if (!state || !balance || zoneId.IsEmpty())
			return false;

		HST_DefaultCatalog.EnsureCivilianPools(balance);
		PruneDeletedRuntimeEntities(state);
		bool changed = PruneAmbientTrafficOutsideRenderBubble(state, balance);

		if (!balance.m_bCivilianPopulationEnabled)
			return PublishRuntimeDiagnostics(state) || CleanupZoneRuntimeEntities(state, zoneId) || changed;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return PublishRuntimeDiagnostics(state) || changed;

		if (active)
		{
			if (!HasRuntimeZone(zoneId))
				changed = SpawnActiveZoneRuntime(state, preset, balance, zone) || changed;
			else if (MaintainActiveZoneTraffic(state, preset, balance, zone))
				changed = true;
		}
		else if (CleanupZoneRuntimeEntities(state, zoneId))
		{
			changed = true;
		}

		return PublishRuntimeDiagnostics(state) || changed;
	}

	HST_PlayerUndercoverState EnsurePlayer(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return null;

		HST_PlayerUndercoverState undercover = state.FindUndercoverPlayer(identityId);
		if (undercover)
			return undercover;

		undercover = new HST_PlayerUndercoverState();
		undercover.m_sIdentityId = identityId;
		state.m_aUndercoverPlayers.Insert(undercover);
		return undercover;
	}

	bool RegisterIncident(HST_CampaignState state, string zoneId, int reputationDelta, int heatDelta, string reason, HST_CampaignPreset preset = null)
	{
		if (!state || zoneId.IsEmpty())
			return false;

		int occupierDelta = -reputationDelta / 2;
		if (heatDelta > 0)
			occupierDelta += heatDelta;

		return RegisterInfluenceEvent(state, zoneId, "incident", reputationDelta, occupierDelta, reputationDelta, heatDelta, 0, 0, 0, reason, preset);
	}

	bool RegisterInfluenceEvent(HST_CampaignState state, string zoneId, string eventKind, int fiaSupportDelta, int occupierSupportDelta, int reputationDelta, int heatDelta, int populationDelta, int policeDelta, int roadblockDelta, string reason, HST_CampaignPreset preset = null, int durationSeconds = 0, string sourceId = "")
	{
		if (!state || zoneId.IsEmpty())
			return false;

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zoneId);
		if (!civilianZone)
			return false;

		HST_TownInfluenceEventState influenceEvent = new HST_TownInfluenceEventState();
		influenceEvent.m_sEventId = BuildTownInfluenceEventId(state, zoneId, eventKind);
		influenceEvent.m_sZoneId = zoneId;
		influenceEvent.m_sKind = eventKind;
		influenceEvent.m_sSourceId = sourceId;
		influenceEvent.m_sReason = reason;
		influenceEvent.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		if (durationSeconds > 0)
			influenceEvent.m_iExpiresAtSecond = state.m_iElapsedSeconds + durationSeconds;
		influenceEvent.m_iFIASupportDelta = fiaSupportDelta;
		influenceEvent.m_iOccupierSupportDelta = occupierSupportDelta;
		influenceEvent.m_iReputationDelta = reputationDelta;
		influenceEvent.m_iHeatDelta = heatDelta;
		influenceEvent.m_iPopulationDelta = populationDelta;
		influenceEvent.m_iPoliceDelta = policeDelta;
		influenceEvent.m_iRoadblockDelta = roadblockDelta;
		state.m_aTownInfluenceEvents.Insert(influenceEvent);

		HST_StrategicEventApplyResult strategicEvent;
		if (m_Strategic)
			strategicEvent = m_Strategic.BeginTownInfluenceEvent(state, preset, influenceEvent);

		ApplyInfluenceEvent(state, civilianZone, influenceEvent, preset);
		RefreshTownInfluenceAggregatesForZone(state, civilianZone);
		if (m_Strategic && strategicEvent && strategicEvent.m_Event)
			m_Strategic.CompleteStrategicEvent(state, strategicEvent, true, true);
		return true;
	}

	protected void ApplyInfluenceEvent(HST_CampaignState state, HST_CivilianZoneState civilianZone, HST_TownInfluenceEventState influenceEvent, HST_CampaignPreset preset = null)
	{
		if (!state || !civilianZone || !influenceEvent)
			return;

		EnsureTownPopulationFields(civilianZone);
		civilianZone.m_iReputation = Math.Max(0, Math.Min(100, civilianZone.m_iReputation + influenceEvent.m_iReputationDelta));
		civilianZone.m_iWantedHeat = Math.Max(0, civilianZone.m_iWantedHeat + influenceEvent.m_iHeatDelta);
		civilianZone.m_iLastIncidentSecond = state.m_iElapsedSeconds;
		civilianZone.m_sLastIncidentReason = influenceEvent.m_sReason;
		civilianZone.m_iLastSupportChangeSecond = state.m_iElapsedSeconds;
		civilianZone.m_iFIASupport = Math.Max(0, Math.Min(100, civilianZone.m_iFIASupport + influenceEvent.m_iFIASupportDelta));
		civilianZone.m_iOccupierSupport = Math.Max(0, Math.Min(100, civilianZone.m_iOccupierSupport + influenceEvent.m_iOccupierSupportDelta));
		civilianZone.m_iPolicePresence = Math.Max(0, civilianZone.m_iPolicePresence + influenceEvent.m_iPoliceDelta);
		civilianZone.m_iRoadblockPresence = Math.Max(0, civilianZone.m_iRoadblockPresence + influenceEvent.m_iRoadblockDelta);

		if (influenceEvent.m_iPopulationDelta < 0)
		{
			int killed = Math.Min(civilianZone.m_iPopulationRemaining, -influenceEvent.m_iPopulationDelta);
			civilianZone.m_iPopulationRemaining = Math.Max(0, civilianZone.m_iPopulationRemaining - killed);
			civilianZone.m_iPopulationKilled += killed;
		}
		else if (influenceEvent.m_iPopulationDelta > 0)
		{
			civilianZone.m_iPopulationRemaining += influenceEvent.m_iPopulationDelta;
		}

		influenceEvent.m_bApplied = true;
		civilianZone.m_iLastInfluenceEventSecond = influenceEvent.m_iCreatedAtSecond;
		civilianZone.m_sLastInfluenceEventId = influenceEvent.m_sEventId;
		civilianZone.m_sLastInfluenceKind = influenceEvent.m_sKind;
		civilianZone.m_sLastInfluenceReason = influenceEvent.m_sReason;

		HST_ZoneState zone = state.FindZone(influenceEvent.m_sZoneId);
		if (zone)
		{
			zone.m_iSupport = Math.Max(-100, Math.Min(100, civilianZone.m_iFIASupport - civilianZone.m_iOccupierSupport));
			civilianZone.m_bUndercoverRestricted = zone.m_iSupport < 25;
			ApplyTownSupportOwnershipPolicy(state, preset, civilianZone, zone);
		}
	}

	protected bool RefreshTownInfluenceAggregates(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_CivilianZoneState civilianZone : state.m_aCivilianZones)
		{
			if (!civilianZone)
				continue;

			int beforeEvents = civilianZone.m_iInfluenceEventCount;
			int beforeActive = civilianZone.m_iActiveInfluenceModifierCount;
			int beforeExpired = civilianZone.m_iExpiredInfluenceModifierCount;
			string beforeLast = civilianZone.m_sLastInfluenceEventId;
			RefreshTownInfluenceAggregatesForZone(state, civilianZone);
			if (civilianZone.m_iInfluenceEventCount != beforeEvents || civilianZone.m_iActiveInfluenceModifierCount != beforeActive || civilianZone.m_iExpiredInfluenceModifierCount != beforeExpired || civilianZone.m_sLastInfluenceEventId != beforeLast)
				changed = true;
		}

		return changed;
	}

	protected void RefreshTownInfluenceAggregatesForZone(HST_CampaignState state, HST_CivilianZoneState civilianZone)
	{
		if (!state || !civilianZone)
			return;

		EnsureTownPopulationFields(civilianZone);
		int eventCount;
		int activeCount;
		int expiredCount;
		int lastSecond = civilianZone.m_iLastInfluenceEventSecond;
		string lastEventId = civilianZone.m_sLastInfluenceEventId;
		string lastKind = civilianZone.m_sLastInfluenceKind;
		string lastReason = civilianZone.m_sLastInfluenceReason;

		foreach (HST_TownInfluenceEventState influenceEvent : state.m_aTownInfluenceEvents)
		{
			if (!influenceEvent || influenceEvent.m_sZoneId != civilianZone.m_sZoneId)
				continue;

			eventCount++;
			if (influenceEvent.m_iExpiresAtSecond > 0)
			{
				if (influenceEvent.m_iExpiresAtSecond > state.m_iElapsedSeconds)
					activeCount++;
				else
					expiredCount++;
			}

			if (influenceEvent.m_iCreatedAtSecond >= lastSecond)
			{
				lastSecond = influenceEvent.m_iCreatedAtSecond;
				lastEventId = influenceEvent.m_sEventId;
				lastKind = influenceEvent.m_sKind;
				lastReason = influenceEvent.m_sReason;
			}
		}

		civilianZone.m_iInfluenceEventCount = eventCount;
		civilianZone.m_iActiveInfluenceModifierCount = activeCount;
		civilianZone.m_iExpiredInfluenceModifierCount = expiredCount;
		civilianZone.m_iLastInfluenceEventSecond = lastSecond;
		civilianZone.m_sLastInfluenceEventId = lastEventId;
		civilianZone.m_sLastInfluenceKind = lastKind;
		civilianZone.m_sLastInfluenceReason = lastReason;
	}

	protected void EnsureTownPopulationFields(HST_CivilianZoneState civilianZone)
	{
		if (!civilianZone)
			return;

		if (civilianZone.m_iPopulationRemaining <= 0 && civilianZone.m_iPopulationKilled <= 0)
			civilianZone.m_iPopulationRemaining = Math.Max(20, Math.Max(1, civilianZone.m_iCivilianPresence) * 8);
	}

	protected string BuildTownInfluenceEventId(HST_CampaignState state, string zoneId, string eventKind)
	{
		string safeKind = eventKind;
		if (safeKind.IsEmpty())
			safeKind = "event";
		return string.Format("town_influence_%1_%2_%3_%4", zoneId, safeKind, state.m_iElapsedSeconds, state.m_aTownInfluenceEvents.Count());
	}

	protected bool ApplyTownSupportOwnershipPolicy(HST_CampaignState state, HST_CampaignPreset preset, HST_CivilianZoneState civilianZone, HST_ZoneState zone)
	{
		if (!state || !preset || !civilianZone || !zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return false;

		string resistanceFactionKey = preset.m_sResistanceFactionKey;
		if (resistanceFactionKey.IsEmpty() || !state.FindFactionPool(resistanceFactionKey))
			return false;

		int supportMargin = civilianZone.m_iFIASupport - civilianZone.m_iOccupierSupport;
		if (zone.m_sOwnerFactionKey != resistanceFactionKey)
		{
			if (civilianZone.m_iFIASupport < TOWN_RESISTANCE_FLIP_FIA_SUPPORT)
				return false;
			if (supportMargin < TOWN_RESISTANCE_FLIP_SUPPORT_MARGIN)
				return false;
			if (civilianZone.m_iWantedHeat > TOWN_RESISTANCE_FLIP_MAX_HEAT)
				return false;

			zone.m_sOwnerFactionKey = resistanceFactionKey;
			zone.m_iResistanceCaptureProgress = 0;
			civilianZone.m_sLastSecurityReason = "support flipped town to resistance";
			return true;
		}

		bool occupierSupportDominant = civilianZone.m_iOccupierSupport >= TOWN_ENEMY_FLIP_OCCUPIER_SUPPORT && supportMargin <= -TOWN_ENEMY_FLIP_SUPPORT_MARGIN;
		bool heatedOccupierPressure = civilianZone.m_iWantedHeat >= TOWN_ENEMY_FLIP_HEAT && civilianZone.m_iOccupierSupport > civilianZone.m_iFIASupport;
		if (!occupierSupportDominant && !heatedOccupierPressure)
			return false;

		string enemyFactionKey = ResolveTownSupportEnemyFaction(state, preset, zone.m_sOwnerFactionKey);
		if (enemyFactionKey.IsEmpty())
			return false;

		zone.m_sOwnerFactionKey = enemyFactionKey;
		zone.m_iResistanceCaptureProgress = 0;
		civilianZone.m_sLastSecurityReason = "support flipped town to enemy";
		return true;
	}

	protected string ResolveTownSupportEnemyFaction(HST_CampaignState state, HST_CampaignPreset preset, string currentOwner)
	{
		if (!currentOwner.IsEmpty() && preset && currentOwner != preset.m_sResistanceFactionKey && state.FindFactionPool(currentOwner))
			return currentOwner;
		if (preset && !preset.m_sOccupierFactionKey.IsEmpty() && state.FindFactionPool(preset.m_sOccupierFactionKey))
			return preset.m_sOccupierFactionKey;
		if (preset && !preset.m_sInvaderFactionKey.IsEmpty() && state.FindFactionPool(preset.m_sInvaderFactionKey))
			return preset.m_sInvaderFactionKey;

		return "";
	}

	bool CheckUndercover(HST_CampaignState state, string identityId, string zoneId, bool visiblyArmed, bool suspiciousVehicle, bool recentCombat)
	{
		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!undercover)
			return false;

		if (state.m_iElapsedSeconds < undercover.m_iLastCheckedSecond + UNDERCOVER_RECHECK_SECONDS)
			return false;

		undercover.m_iLastCheckedSecond = state.m_iElapsedSeconds;
		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zoneId);
		if (!civilianZone)
		{
			undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
			undercover.m_sLastReason = "outside civilian zone";
			return true;
		}

		if (recentCombat || visiblyArmed)
		{
			undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_COMPROMISED;
			undercover.m_iWantedHeat = Math.Max(undercover.m_iWantedHeat, 4);
			undercover.m_iCompromisedUntilSecond = state.m_iElapsedSeconds + 180;
			undercover.m_sLastReason = "armed or combat exposure";
			civilianZone.m_iWantedHeat = Math.Max(civilianZone.m_iWantedHeat, 3);
			return true;
		}

		if (suspiciousVehicle || civilianZone.m_bUndercoverRestricted || civilianZone.m_iWantedHeat >= 4)
		{
			undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_SUSPICIOUS;
			undercover.m_iWantedHeat = Math.Max(undercover.m_iWantedHeat, 1);
			undercover.m_sLastReason = "suspicious presence";
			return true;
		}

		undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
		undercover.m_sLastReason = "clear";
		return true;
	}

	string BuildCivilianReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi civilians | state not ready";

		int reputationTotal;
		int heatTotal;
		int policeTotal;
		int roadblocks;
		int townCount;
		foreach (HST_CivilianZoneState civilianZone : state.m_aCivilianZones)
		{
			if (!civilianZone)
				continue;

			townCount++;
			reputationTotal += civilianZone.m_iReputation;
			heatTotal += civilianZone.m_iWantedHeat;
			policeTotal += civilianZone.m_iPolicePresence;
			roadblocks += civilianZone.m_iRoadblockPresence;
		}

		int averageReputation;
		if (townCount > 0)
			averageReputation = reputationTotal / townCount;

		string summary = string.Format("h-istasi civilians | towns %1 | avg reputation %2 | heat %3 | police %4 | roadblocks %5", townCount, averageReputation, heatTotal, policeTotal, roadblocks);
		return summary + "\n" + BuildTownSupportReport(state);
	}

	string BuildTownSupportReport(HST_CampaignState state, int maxRows = 20)
	{
		if (!state)
			return "h-istasi town support | state not ready";

		RefreshTownInfluenceAggregates(state);
		int townCount;
		int fiaSupportTotal;
		int occupierSupportTotal;
		int reputationTotal;
		int heatTotal;
		int policeTotal;
		int roadblockTotal;
		int populationRemainingTotal;
		int populationKilledTotal;
		int influenceEventTotal;
		int activeInfluenceTotal;
		int expiredInfluenceTotal;

		foreach (HST_CivilianZoneState civilianZone : state.m_aCivilianZones)
		{
			if (!civilianZone)
				continue;

			EnsureTownPopulationFields(civilianZone);
			townCount++;
			fiaSupportTotal += civilianZone.m_iFIASupport;
			occupierSupportTotal += civilianZone.m_iOccupierSupport;
			reputationTotal += civilianZone.m_iReputation;
			heatTotal += civilianZone.m_iWantedHeat;
			policeTotal += civilianZone.m_iPolicePresence;
			roadblockTotal += civilianZone.m_iRoadblockPresence;
			populationRemainingTotal += civilianZone.m_iPopulationRemaining;
			populationKilledTotal += civilianZone.m_iPopulationKilled;
			influenceEventTotal += civilianZone.m_iInfluenceEventCount;
			activeInfluenceTotal += civilianZone.m_iActiveInfluenceModifierCount;
			expiredInfluenceTotal += civilianZone.m_iExpiredInfluenceModifierCount;
		}

		int avgFIA;
		int avgOccupier;
		int avgRep;
		if (townCount > 0)
		{
			avgFIA = fiaSupportTotal / townCount;
			avgOccupier = occupierSupportTotal / townCount;
			avgRep = reputationTotal / townCount;
		}

		string report = string.Format(
			"h-istasi town support | towns %1 | avg FIA %2 | avg occupier %3 | avg rep %4 | heat %5 | police %6 | roadblocks %7",
			townCount,
			avgFIA,
			avgOccupier,
			avgRep,
			heatTotal,
			policeTotal,
			roadblockTotal
		);
		report = report + string.Format(" | population %1 killed %2 | influence events %3 active %4 expired %5", populationRemainingTotal, populationKilledTotal, influenceEventTotal, activeInfluenceTotal, expiredInfluenceTotal);

		int emitted;
		foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
		{
			if (!town || emitted >= maxRows)
				continue;

			EnsureTownPopulationFields(town);
			HST_ZoneState zone = state.FindZone(town.m_sZoneId);
			string label = town.m_sZoneId;
			string owner = "";
			int strategicSupport;
			bool active;

			if (zone)
			{
				if (!zone.m_sDisplayName.IsEmpty())
					label = zone.m_sDisplayName;

				owner = zone.m_sOwnerFactionKey;
				strategicSupport = zone.m_iSupport;
				active = zone.m_bActive;
			}

			int age;
			if (town.m_iLastIncidentSecond > 0)
				age = Math.Max(0, state.m_iElapsedSeconds - town.m_iLastIncidentSecond);

			string line = string.Format(
				"\n%1 | owner %2 | FIA %3 | occupier %4 | rep %5 | heat %6 | police %7 | roadblocks %8 | civs %9",
				label,
				owner,
				town.m_iFIASupport,
				town.m_iOccupierSupport,
				town.m_iReputation,
				town.m_iWantedHeat,
				town.m_iPolicePresence,
				town.m_iRoadblockPresence,
				town.m_iCivilianPresence
			);
			line = line + string.Format(" | restricted %1 | support %2 | active %3 | last %4s %5", town.m_bUndercoverRestricted, strategicSupport, active, age, town.m_sLastIncidentReason);
			line = line + string.Format(" | population %1 killed %2 | influence %3 active %4 expired %5", town.m_iPopulationRemaining, town.m_iPopulationKilled, town.m_iInfluenceEventCount, town.m_iActiveInfluenceModifierCount, town.m_iExpiredInfluenceModifierCount);
			line = line + string.Format(" | roadScan %1 | policeScan %2 | security %3", town.m_iLastRoadblockScanSecond, town.m_iLastPoliceScanSecond, town.m_sLastSecurityReason);
			line = line + string.Format(" | lastInfluence %1 %2", town.m_sLastInfluenceKind, town.m_sLastInfluenceReason);
			report = report + line;

			emitted++;
		}

		if (emitted == 0)
			report = report + "\nno civilian town records";

		return report;
	}

	string BuildUndercoverReport(HST_CampaignState state, string identityId = "")
	{
		if (!state)
			return "h-istasi undercover | state not ready";

		if (!identityId.IsEmpty())
		{
			HST_PlayerUndercoverState undercover = state.FindUndercoverPlayer(identityId);
			if (!undercover)
				return "h-istasi undercover | no record";

			string report = string.Format(
				"h-istasi undercover | %1 | requested %2 | applied %3 | eligible %4 | status %5 | heat %6 | zone %7 | enforcement zone %8 | score %9",
				identityId,
				undercover.m_bUndercoverRequested,
				undercover.m_bUndercoverApplied,
				undercover.m_bLastEligibilityResult,
				undercover.m_eStatus,
				undercover.m_iWantedHeat,
				undercover.m_sLastZoneId,
				undercover.m_sLastEnforcementZoneId,
				undercover.m_iDetectionScore
			);
			report = report + string.Format(" | source %1 | reason %2 | compromise %3\nclothing: %4\nweapon: %5\nvehicle: %6\noffroad: %7\nenemy proximity: %8\nwanted heat: %9", undercover.m_sLastDetectionSource, undercover.m_sLastReason, undercover.m_sLastCompromiseReason, undercover.m_sClothingReason, undercover.m_sWeaponReason, undercover.m_sVehicleReason, undercover.m_sOffroadReason, undercover.m_sEnemyProximityReason, undercover.m_sWantedHeatReason);
			report = report + string.Format("\nroadblock scans: %1 failed %2 | police scans: %3 failed %4 | checked: %5 | enforced: %6 | mode: %7", undercover.m_iRoadblockScanCount, undercover.m_bLastRoadblockScanFailed, undercover.m_iPoliceScanCount, undercover.m_bLastPoliceScanFailed, undercover.m_iLastEligibilityCheckSecond, undercover.m_iLastEnforcementSecond, undercover.m_sAppliedMode);
			return report;
		}

		int tracked;
		int requested;
		int applied;
		int eligible;
		int clear;
		int suspicious;
		int compromised;
		int wanted;
		foreach (HST_PlayerUndercoverState undercoverPlayer : state.m_aUndercoverPlayers)
		{
			if (!undercoverPlayer)
				continue;

			tracked++;
			if (undercoverPlayer.m_bUndercoverRequested)
				requested++;
			if (undercoverPlayer.m_bUndercoverApplied)
				applied++;
			if (undercoverPlayer.m_bLastEligibilityResult)
				eligible++;

			if (undercoverPlayer.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR)
				clear++;
			else if (undercoverPlayer.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_SUSPICIOUS)
				suspicious++;
			else if (undercoverPlayer.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_COMPROMISED)
				compromised++;
			else if (undercoverPlayer.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_WANTED)
				wanted++;
		}

		return string.Format(
			"h-istasi undercover | tracked %1 | requested %2 | applied %3 | eligible %4 | clear %5 | suspicious %6 | compromised %7 | wanted %8",
			tracked,
			requested,
			applied,
			eligible,
			clear,
			suspicious,
			compromised,
			wanted
		);
	}

	HST_UndercoverEligibilityResult BuildUndercoverEligibility(HST_CampaignState state, string identityId, IEntity playerEntity, bool checkCurrentEntity = true)
	{
		HST_UndercoverEligibilityResult result = new HST_UndercoverEligibilityResult();
		result.m_sIdentityId = identityId;
		result.m_bEligible = true;

		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!state || !undercover)
		{
			result.m_bEligible = false;
			result.m_sClothingReason = "WARN player entity unavailable";
			result.m_sWeaponReason = "WARN player entity unavailable";
			result.m_sVehicleReason = "WARN player entity unavailable";
			result.m_sOffroadReason = "WARN player entity unavailable";
			result.m_sEnemyProximityReason = "WARN state/player missing";
			result.m_sWantedHeatReason = "BLOCK state or undercover record missing";
			result.m_sSummary = "state or undercover record not ready";
			return result;
		}

		string zoneId = "";
		if (checkCurrentEntity)
			zoneId = ResolveNearestCivilianZoneId(state, playerEntity);
		else
			zoneId = undercover.m_sLastZoneId;
		if (zoneId.IsEmpty())
			zoneId = ResolveNearestCivilianZoneId(state, playerEntity);
		if (zoneId.IsEmpty())
		{
			foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
			{
				if (!town)
					continue;

				zoneId = town.m_sZoneId;
				break;
			}
		}
		result.m_sZoneId = zoneId;

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zoneId);
		if (checkCurrentEntity)
		{
			result.m_sClothingReason = ResolveClothingEligibilityReason(playerEntity);
			result.m_sWeaponReason = ResolveWeaponEligibilityReason(playerEntity);
			result.m_sVehicleReason = ResolveVehicleEligibilityReason(state, playerEntity);
			result.m_sOffroadReason = ResolveOffroadEligibilityReason(playerEntity);
			result.m_sEnemyProximityReason = ResolveEnemyProximityReason(state, playerEntity);
		}
		else
		{
			result.m_sClothingReason = "OK prechecked civilian clothing";
			result.m_sWeaponReason = "OK prechecked no visible military weapon";
			result.m_sVehicleReason = "OK prechecked civilian vehicle/on foot";
			result.m_sOffroadReason = "OK prechecked off-road state";
			result.m_sEnemyProximityReason = "OK prechecked no enemy nearby";
		}
		result.m_sWantedHeatReason = ResolveWantedHeatReason(state, undercover, civilianZone);

		if (IsBlockingReason(result.m_sClothingReason)
			|| IsBlockingReason(result.m_sWeaponReason)
			|| IsBlockingReason(result.m_sVehicleReason)
			|| IsBlockingReason(result.m_sOffroadReason)
			|| IsBlockingReason(result.m_sEnemyProximityReason)
			|| IsBlockingReason(result.m_sWantedHeatReason))
		{
			result.m_bEligible = false;
		}

		if (result.m_bEligible)
			result.m_sSummary = "eligible for undercover request";
		else
			result.m_sSummary = "not eligible for undercover request";

		StoreEligibilityResult(state, undercover, result);
		return result;
	}

	string RequestUndercover(HST_CampaignState state, string identityId, IEntity playerEntity, bool checkCurrentEntity = true)
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi undercover | failed: state or identity missing";

		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!undercover)
			return "h-istasi undercover | failed: could not create player record";

		HST_UndercoverEligibilityResult eligibility = BuildUndercoverEligibility(state, identityId, playerEntity, checkCurrentEntity);
		if (!eligibility || !eligibility.m_bEligible)
		{
			if (undercover.m_iWantedHeat >= 4)
				undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_WANTED;
			undercover.m_bUndercoverRequested = false;
			undercover.m_bUndercoverApplied = false;
			undercover.m_sAppliedMode = "request_denied";
			if (eligibility)
			{
				undercover.m_sLastReason = "request denied: " + eligibility.m_sSummary;
				return "h-istasi undercover | request denied\n" + eligibility.BuildReport();
			}

			undercover.m_sLastReason = "request denied: no eligibility result";
			return "h-istasi undercover | request denied";
		}

		undercover.m_bUndercoverRequested = true;
		undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
		undercover.m_sLastReason = "undercover requested and eligible";
		undercover.m_bUndercoverApplied = true;
		undercover.m_bEnforcementEnabled = true;
		undercover.m_sAppliedMode = "undercover_active";
		undercover.m_iDetectionScore = 0;
		undercover.m_sLastCompromiseReason = "";
		undercover.m_sLastDetectionSource = "request";
		undercover.m_iLastEnforcementSecond = state.m_iElapsedSeconds - UNDERCOVER_RECHECK_SECONDS;
		return "h-istasi undercover | request accepted\n" + eligibility.BuildReport();
	}

	string ClearUndercoverCompromise(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi undercover | failed: state or identity missing";

		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!undercover)
			return "h-istasi undercover | failed: no player record";

		if (undercover.m_iWantedHeat > 0)
			return string.Format("h-istasi undercover | failed: wanted heat %1 must cool before clearing", undercover.m_iWantedHeat);

		if (undercover.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_COMPROMISED && state.m_iElapsedSeconds < undercover.m_iCompromisedUntilSecond)
			return string.Format("h-istasi undercover | failed: compromised for %1s", undercover.m_iCompromisedUntilSecond - state.m_iElapsedSeconds);

		undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
		undercover.m_bUndercoverRequested = false;
		undercover.m_bUndercoverApplied = false;
		undercover.m_sAppliedMode = "cleared";
		undercover.m_iDetectionScore = 0;
		undercover.m_sLastCompromiseReason = "";
		undercover.m_sLastDetectionSource = "player_clear";
		undercover.m_bLastRoadblockScanFailed = false;
		undercover.m_bLastPoliceScanFailed = false;
		undercover.m_iCompromisedUntilSecond = 0;
		undercover.m_sLastReason = "cleared by player";
		return "h-istasi undercover | cleared";
	}

	HST_UndercoverEnforcementResult EnforceUndercoverForPlayer(HST_CampaignState state, HST_CampaignPreset preset, string identityId, IEntity playerEntity)
	{
		HST_UndercoverEnforcementResult result = new HST_UndercoverEnforcementResult();
		result.m_sIdentityId = identityId;

		if (!state || identityId.IsEmpty())
		{
			result.m_bBlocked = true;
			result.m_sReason = "state or identity missing";
			return result;
		}

		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!undercover)
		{
			result.m_bBlocked = true;
			result.m_sReason = "undercover record unavailable";
			return result;
		}

		if (!undercover.m_bEnforcementEnabled)
		{
			result.m_bBlocked = true;
			result.m_sReason = "enforcement disabled";
			return result;
		}

		if (!undercover.m_bUndercoverRequested && !undercover.m_bUndercoverApplied)
		{
			result.m_bBlocked = true;
			result.m_sReason = "undercover not requested";
			return result;
		}

		if (undercover.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_COMPROMISED && state.m_iElapsedSeconds < undercover.m_iCompromisedUntilSecond)
		{
			result.m_bBlocked = true;
			result.m_sReason = string.Format("compromised for %1s", undercover.m_iCompromisedUntilSecond - state.m_iElapsedSeconds);
			result.m_iWantedHeat = undercover.m_iWantedHeat;
			result.m_iDetectionScore = undercover.m_iDetectionScore;
			result.m_sDetectionSource = undercover.m_sLastDetectionSource;
			return result;
		}

		if (state.m_iElapsedSeconds < undercover.m_iLastEnforcementSecond + UNDERCOVER_RECHECK_SECONDS)
		{
			result.m_bBlocked = true;
			result.m_sReason = "recheck cooldown";
			return result;
		}

		undercover.m_iLastEnforcementSecond = state.m_iElapsedSeconds;

		HST_UndercoverEligibilityResult eligibility = BuildUndercoverEligibility(state, identityId, playerEntity);
		string zoneId;
		if (eligibility)
			zoneId = eligibility.m_sZoneId;
		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zoneId);

		result.m_sZoneId = zoneId;
		result.m_iWantedHeat = undercover.m_iWantedHeat;
		if (civilianZone)
		{
			result.m_iTownHeat = civilianZone.m_iWantedHeat;
			result.m_iPolicePresence = civilianZone.m_iPolicePresence;
			result.m_iRoadblockPresence = civilianZone.m_iRoadblockPresence;
		}

		string compromiseReason;
		string detectionSource;
		int score = BuildUndercoverDetectionScore(state, undercover, civilianZone, eligibility, playerEntity, compromiseReason, detectionSource);
		result.m_iDetectionScore = score;
		result.m_sReason = compromiseReason;
		result.m_sDetectionSource = detectionSource;

		undercover.m_iDetectionScore = score;
		undercover.m_sLastDetectionSource = detectionSource;
		undercover.m_sLastEnforcementZoneId = zoneId;

		string scanReason;
		if (TryRoadblockScan(state, undercover, civilianZone, eligibility, scanReason))
		{
			CompromiseUndercover(state, undercover, civilianZone, zoneId, scanReason, "roadblock", score);
			result.m_bChanged = true;
			result.m_bCompromised = true;
			result.m_sReason = scanReason;
			result.m_sDetectionSource = "roadblock";
			result.m_iWantedHeat = undercover.m_iWantedHeat;
			if (civilianZone)
				result.m_iTownHeat = civilianZone.m_iWantedHeat;
			return result;
		}

		if (TryPoliceScan(state, undercover, civilianZone, eligibility, scanReason))
		{
			CompromiseUndercover(state, undercover, civilianZone, zoneId, scanReason, "police", score);
			result.m_bChanged = true;
			result.m_bCompromised = true;
			result.m_sReason = scanReason;
			result.m_sDetectionSource = "police";
			result.m_iWantedHeat = undercover.m_iWantedHeat;
			if (civilianZone)
				result.m_iTownHeat = civilianZone.m_iWantedHeat;
			return result;
		}

		if (ShouldCompromiseUndercover(state, civilianZone, eligibility, score, compromiseReason))
		{
			CompromiseUndercover(state, undercover, civilianZone, zoneId, compromiseReason, detectionSource, score);
			result.m_bChanged = true;
			result.m_bCompromised = true;
			result.m_iWantedHeat = undercover.m_iWantedHeat;
			if (civilianZone)
				result.m_iTownHeat = civilianZone.m_iWantedHeat;
			return result;
		}

		if (ShouldMarkSuspicious(civilianZone, eligibility, score))
		{
			if (undercover.m_eStatus != HST_EUndercoverStatus.HST_UNDERCOVER_SUSPICIOUS || !undercover.m_bUndercoverApplied)
				result.m_bChanged = true;

			undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_SUSPICIOUS;
			undercover.m_bUndercoverApplied = true;
			undercover.m_sAppliedMode = "undercover_active";
			undercover.m_sLastReason = compromiseReason;
			result.m_bSuspicious = true;
			return result;
		}

		if (undercover.m_eStatus != HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR || !undercover.m_bUndercoverApplied)
			result.m_bChanged = true;

		undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
		undercover.m_bUndercoverApplied = true;
		undercover.m_sAppliedMode = "undercover_active";
		undercover.m_sLastReason = "undercover enforcement clear";
		result.m_bCleared = true;
		return result;
	}

	string RegisterUndercoverCombatExposure(HST_CampaignState state, string identityId, string zoneId, string reason)
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi undercover | failed: state or identity missing";

		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!undercover)
			return "h-istasi undercover | failed: no undercover record";

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zoneId);
		CompromiseUndercover(state, undercover, civilianZone, zoneId, reason, "combat", 100);
		return "h-istasi undercover | compromised: " + reason;
	}


	string RegisterUndercoverVehicleExposure(HST_CampaignState state, string identityId, string zoneId, string reason, string vehicleRuntimeId = "", HST_StrategicService strategic = null)
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi undercover | failed: state or identity missing";

		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!undercover)
			return "h-istasi undercover | failed: no undercover record";

		string vehicleReport;
		if (!vehicleRuntimeId.IsEmpty())
			vehicleReport = RegisterVehiclePassengerCompromise(state, vehicleRuntimeId, identityId, zoneId, reason, strategic);

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zoneId);
		CompromiseUndercover(state, undercover, civilianZone, zoneId, reason, "vehicle", 90);
		if (!vehicleReport.IsEmpty())
			return "h-istasi undercover | compromised: " + reason + "\n" + vehicleReport;

		return "h-istasi undercover | compromised: " + reason;
	}

	string RegisterVehicleHeat(HST_CampaignState state, string vehicleRuntimeId, string zoneId, int heatDelta, int durationSeconds, string reason, bool reported = true, HST_StrategicService strategic = null)
	{
		if (!state || vehicleRuntimeId.IsEmpty())
			return "h-istasi vehicle heat | failed: state or runtime vehicle id missing";

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
		if (!vehicle)
			return "h-istasi vehicle heat | failed: runtime vehicle not tracked";

		int oldHeat = vehicle.m_iVehicleHeat;
		bool reportedBefore = vehicle.m_bReported;
		int reportedUntilBefore = vehicle.m_iReportedUntilSecond;
		HST_StrategicService eventStrategic = strategic;
		if (!eventStrategic)
			eventStrategic = m_Strategic;
		HST_StrategicEventApplyResult eventResult;
		if (eventStrategic && reported)
			eventResult = eventStrategic.BeginVehicleReportEvent(state, vehicle, zoneId, reason);

		int appliedDelta = Math.Max(0, heatDelta);
		vehicle.m_iVehicleHeat = Math.Max(0, Math.Min(10, vehicle.m_iVehicleHeat + appliedDelta));
		vehicle.m_iLastVehicleHeatChangedSecond = state.m_iElapsedSeconds;
		vehicle.m_sLastReportedReason = reason;
		vehicle.m_sLastReporterZoneId = zoneId;
		if (reported || vehicle.m_iVehicleHeat > 0)
		{
			vehicle.m_bReported = vehicle.m_iVehicleHeat > 0;
			vehicle.m_iLastReportedSecond = state.m_iElapsedSeconds;
			int reportDuration = durationSeconds;
			if (reportDuration <= 0)
				reportDuration = VEHICLE_REPORT_DEFAULT_SECONDS;
			vehicle.m_iReportedUntilSecond = Math.Max(vehicle.m_iReportedUntilSecond, state.m_iElapsedSeconds + reportDuration);
		}

		if (vehicle.m_iVehicleHeat == 0)
		{
			vehicle.m_bReported = false;
			vehicle.m_iReportedUntilSecond = 0;
		}

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zoneId);
		if (civilianZone && vehicle.m_iVehicleHeat > 0)
		{
			civilianZone.m_iLastIncidentSecond = state.m_iElapsedSeconds;
			civilianZone.m_sLastSecurityReason = "vehicle reported: " + reason;
		}

		if (eventResult && eventResult.m_Event)
		{
			bool changed = oldHeat != vehicle.m_iVehicleHeat || reportedBefore != vehicle.m_bReported || reportedUntilBefore != vehicle.m_iReportedUntilSecond;
			eventStrategic.CompleteStrategicEvent(state, eventResult, true, changed);
		}

		string report = string.Format("h-istasi vehicle heat | %1 | heat %2 -> %3 | reported %4 | until %5", vehicleRuntimeId, oldHeat, vehicle.m_iVehicleHeat, vehicle.m_bReported, vehicle.m_iReportedUntilSecond);
		return report + string.Format(" | zone %1 | reason %2", EmptyRuntimeField(zoneId), EmptyRuntimeField(reason));
	}

	string RegisterVehiclePassengerCompromise(HST_CampaignState state, string vehicleRuntimeId, string identityId, string zoneId, string reason, HST_StrategicService strategic = null)
	{
		if (!state || vehicleRuntimeId.IsEmpty())
			return "h-istasi vehicle heat | failed: state or runtime vehicle id missing";

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
		if (!vehicle)
			return "h-istasi vehicle heat | failed: runtime vehicle not tracked";

		vehicle.m_iPassengerCompromiseCount = vehicle.m_iPassengerCompromiseCount + 1;
		string report = RegisterVehicleHeat(state, vehicleRuntimeId, zoneId, 4, VEHICLE_REPORT_DEFAULT_SECONDS, reason, true, strategic);
		return report + string.Format(" | passenger compromises %1 | identity %2", vehicle.m_iPassengerCompromiseCount, EmptyRuntimeField(identityId));
	}

	string ClearVehicleHeat(HST_CampaignState state, string vehicleRuntimeId, string reason = "cleared")
	{
		if (!state || vehicleRuntimeId.IsEmpty())
			return "h-istasi vehicle heat | failed: state or runtime vehicle id missing";

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
		if (!vehicle)
			return "h-istasi vehicle heat | failed: runtime vehicle not tracked";

		vehicle.m_iVehicleHeat = 0;
		vehicle.m_bReported = false;
		vehicle.m_iReportedUntilSecond = 0;
		vehicle.m_iLastVehicleHeatChangedSecond = state.m_iElapsedSeconds;
		vehicle.m_sLastReportedReason = reason;
		return string.Format("h-istasi vehicle heat | %1 | cleared | reason %2", vehicleRuntimeId, EmptyRuntimeField(reason));
	}

	bool IsRuntimeVehicleReportedForUndercover(HST_CampaignState state, string vehicleRuntimeId)
	{
		if (!state || vehicleRuntimeId.IsEmpty())
			return false;

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
		if (!vehicle || vehicle.m_bDeleted)
			return false;

		if (vehicle.m_iReportedUntilSecond > 0 && state.m_iElapsedSeconds >= vehicle.m_iReportedUntilSecond && vehicle.m_iVehicleHeat <= 0)
			return false;

		return vehicle.m_bReported || vehicle.m_iVehicleHeat > 0;
	}

	string ResolveRuntimeVehicleUndercoverReason(HST_CampaignState state, string vehicleRuntimeId)
	{
		if (vehicleRuntimeId.IsEmpty())
			return "OK no runtime vehicle";
		if (!state)
			return "WARN runtime vehicle state unavailable";

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
		if (!vehicle)
			return "WARN runtime vehicle not tracked";
		if (vehicle.m_bDeleted)
			return "BLOCK deleted runtime vehicle";
		if (IsRuntimeVehicleReportedForUndercover(state, vehicleRuntimeId))
			return string.Format("BLOCK reported vehicle heat %1 reason %2", vehicle.m_iVehicleHeat, EmptyRuntimeField(vehicle.m_sLastReportedReason));
		if (!vehicle.m_bCanProvideUndercover)
			return "BLOCK vehicle cannot provide civilian undercover";

		return string.Format("OK runtime civilian vehicle heat %1", vehicle.m_iVehicleHeat);
	}

	string BuildVehicleHeatReport(HST_CampaignState state, int limit = 12)
	{
		if (!state)
			return "h-istasi vehicle heat | state unavailable";

		string report = string.Format("h-istasi vehicle heat | tracked %1", state.m_aRuntimeVehicles.Count());
		int shown;
		foreach (HST_RuntimeVehicleState vehicle : state.m_aRuntimeVehicles)
		{
			if (!vehicle)
				continue;
			if (!vehicle.m_bReported && vehicle.m_iVehicleHeat <= 0 && vehicle.m_iPassengerCompromiseCount <= 0)
				continue;

			report = report + string.Format("\n- %1 | heat %2 | reported %3 | until %4 | cover %5", EmptyRuntimeField(vehicle.m_sVehicleRuntimeId), vehicle.m_iVehicleHeat, vehicle.m_bReported, vehicle.m_iReportedUntilSecond, vehicle.m_bCanProvideUndercover);
			report = report + string.Format(" | zone %1 | passengers %2 | reason %3", EmptyRuntimeField(vehicle.m_sLastReporterZoneId), vehicle.m_iPassengerCompromiseCount, EmptyRuntimeField(vehicle.m_sLastReportedReason));
			shown++;
			if (shown >= limit)
				break;
		}

		if (shown == 0)
			report = report + "\n- none";
		return report;
	}

	protected int BuildUndercoverDetectionScore(HST_CampaignState state, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility, IEntity playerEntity, out string reason, out string source)
	{
		int score;
		reason = "clear";
		source = "none";

		if (!eligibility || !eligibility.m_bEligible)
		{
			score += 20;
			reason = "eligibility failed: " + LastBlockingEligibilityReason(eligibility);
			source = "eligibility";
		}

		if (eligibility)
		{
			if (eligibility.m_sWeaponReason.Contains("BLOCK"))
			{
				score += 70;
				reason = eligibility.m_sWeaponReason;
				source = "weapon";
			}

			if (eligibility.m_sVehicleReason.Contains("BLOCK"))
			{
				score += 55;
				reason = eligibility.m_sVehicleReason;
				source = "vehicle";
			}

			if (eligibility.m_sClothingReason.Contains("BLOCK"))
			{
				score += 45;
				reason = eligibility.m_sClothingReason;
				source = "clothing";
			}

			if (eligibility.m_sWantedHeatReason.Contains("BLOCK"))
			{
				score += 80;
				reason = eligibility.m_sWantedHeatReason;
				source = "wanted_heat";
			}

			if (eligibility.m_sEnemyProximityReason.Contains("WARN"))
			{
				score += 20;
				reason = eligibility.m_sEnemyProximityReason;
				source = "enemy_proximity";
			}
		}

		if (civilianZone)
		{
			score += Math.Max(0, civilianZone.m_iWantedHeat) * 10;
			score += Math.Max(0, civilianZone.m_iPolicePresence) * 6;
			score += Math.Max(0, civilianZone.m_iRoadblockPresence) * 8;

			if (civilianZone.m_bUndercoverRestricted)
			{
				score += 15;
				reason = "restricted town";
				source = "restricted_zone";
			}
		}

		if (undercover)
			score += Math.Max(0, undercover.m_iWantedHeat) * 12;

		string roadReason;
		if (IsLikelyOffroadNearSecurity(state, civilianZone, playerEntity, roadReason))
		{
			score += 25;
			reason = roadReason;
			source = "offroad";
		}

		return score;
	}

	protected string LastBlockingEligibilityReason(HST_UndercoverEligibilityResult eligibility)
	{
		if (!eligibility)
			return "eligibility missing";

		if (eligibility.m_sWantedHeatReason.Contains("BLOCK"))
			return eligibility.m_sWantedHeatReason;
		if (eligibility.m_sWeaponReason.Contains("BLOCK"))
			return eligibility.m_sWeaponReason;
		if (eligibility.m_sVehicleReason.Contains("BLOCK"))
			return eligibility.m_sVehicleReason;
		if (eligibility.m_sClothingReason.Contains("BLOCK"))
			return eligibility.m_sClothingReason;
		if (eligibility.m_sOffroadReason.Contains("BLOCK"))
			return eligibility.m_sOffroadReason;
		if (eligibility.m_sEnemyProximityReason.Contains("BLOCK"))
			return eligibility.m_sEnemyProximityReason;

		return eligibility.m_sSummary;
	}

	protected bool ShouldCompromiseUndercover(HST_CampaignState state, HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility, int score, string reason)
	{
		if (reason.Contains("weapon") || reason.Contains("visible/issued weapon"))
			return true;

		if (reason.Contains("military vehicle"))
			return true;

		if (eligibility && eligibility.m_sVehicleReason.Contains("BLOCK"))
			return true;

		if (eligibility && eligibility.m_sWantedHeatReason.Contains("BLOCK"))
			return true;

		if (civilianZone && civilianZone.m_iRoadblockPresence > 0 && score >= 55)
			return true;

		if (civilianZone && civilianZone.m_iPolicePresence > 0 && score >= 65)
			return true;

		return score >= 80;
	}

	protected bool ShouldMarkSuspicious(HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility, int score)
	{
		if (score >= 25)
			return true;

		if (eligibility && (eligibility.m_sClothingReason.Contains("WARN") || eligibility.m_sVehicleReason.Contains("WARN") || eligibility.m_sEnemyProximityReason.Contains("WARN")))
			return true;

		if (civilianZone && (civilianZone.m_iPolicePresence > 0 || civilianZone.m_iRoadblockPresence > 0) && score >= 15)
			return true;

		return false;
	}

	protected void CompromiseUndercover(HST_CampaignState state, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, string zoneId, string reason, string source, int score)
	{
		if (!state || !undercover)
			return;

		undercover.m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_COMPROMISED;
		undercover.m_bUndercoverApplied = false;
		undercover.m_sAppliedMode = "compromised";
		undercover.m_iWantedHeat = Math.Max(undercover.m_iWantedHeat, 4);
		undercover.m_iCompromisedUntilSecond = state.m_iElapsedSeconds + 180;
		undercover.m_iLastCompromisedSecond = state.m_iElapsedSeconds;
		undercover.m_sLastReason = reason;
		undercover.m_sLastCompromiseReason = reason;
		undercover.m_sLastDetectionSource = source;
		undercover.m_sLastEnforcementZoneId = zoneId;
		undercover.m_iDetectionScore = score;

		if (civilianZone)
		{
			civilianZone.m_iWantedHeat = Math.Max(civilianZone.m_iWantedHeat, 3);
			civilianZone.m_iLastIncidentSecond = state.m_iElapsedSeconds;
			civilianZone.m_sLastIncidentReason = "undercover compromised: " + reason;
			civilianZone.m_sLastSecurityReason = source + ": " + reason;

			if (source == "roadblock")
			{
				civilianZone.m_iLastRoadblockScanSecond = state.m_iElapsedSeconds;
				undercover.m_bLastRoadblockScanFailed = true;
			}

			if (source == "police" || source == "wanted_heat" || source == "restricted_zone")
			{
				civilianZone.m_iLastPoliceScanSecond = state.m_iElapsedSeconds;
				undercover.m_bLastPoliceScanFailed = true;
			}
		}
	}

	protected bool TryRoadblockScan(HST_CampaignState state, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility, out string reason)
	{
		reason = "";

		if (!state || !undercover || !civilianZone || civilianZone.m_iRoadblockPresence <= 0)
			return false;

		if (state.m_iElapsedSeconds < civilianZone.m_iLastRoadblockScanSecond + 30)
			return false;

		int score = 20 + civilianZone.m_iRoadblockPresence * 15 + undercover.m_iWantedHeat * 10;
		if (eligibility)
		{
			if (eligibility.m_sVehicleReason.Contains("BLOCK"))
				score += 40;
			if (eligibility.m_sWeaponReason.Contains("BLOCK"))
				score += 40;
			if (eligibility.m_sClothingReason.Contains("WARN"))
				score += 10;
		}

		civilianZone.m_iLastRoadblockScanSecond = state.m_iElapsedSeconds;
		undercover.m_iRoadblockScanCount++;

		if (score >= 55)
		{
			reason = string.Format("roadblock scan failed score %1", score);
			civilianZone.m_sLastSecurityReason = reason;
			undercover.m_bLastRoadblockScanFailed = true;
			return true;
		}

		reason = string.Format("roadblock scan passed score %1", score);
		civilianZone.m_sLastSecurityReason = reason;
		undercover.m_bLastRoadblockScanFailed = false;
		return false;
	}

	protected bool TryPoliceScan(HST_CampaignState state, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility, out string reason)
	{
		reason = "";

		if (!state || !undercover || !civilianZone || civilianZone.m_iPolicePresence <= 0)
			return false;

		if (state.m_iElapsedSeconds < civilianZone.m_iLastPoliceScanSecond + 45)
			return false;

		int score = 10 + civilianZone.m_iPolicePresence * 12 + civilianZone.m_iWantedHeat * 8 + undercover.m_iWantedHeat * 10;
		if (eligibility)
		{
			if (eligibility.m_sWeaponReason.Contains("BLOCK"))
				score += 35;
			if (eligibility.m_sVehicleReason.Contains("BLOCK"))
				score += 30;
		}

		civilianZone.m_iLastPoliceScanSecond = state.m_iElapsedSeconds;
		undercover.m_iPoliceScanCount++;

		if (score >= 65)
		{
			reason = string.Format("police scan failed score %1", score);
			civilianZone.m_sLastSecurityReason = reason;
			undercover.m_bLastPoliceScanFailed = true;
			return true;
		}

		reason = string.Format("police scan passed score %1", score);
		civilianZone.m_sLastSecurityReason = reason;
		undercover.m_bLastPoliceScanFailed = false;
		return false;
	}

	protected bool IsLikelyOffroadNearSecurity(HST_CampaignState state, HST_CivilianZoneState civilianZone, IEntity playerEntity, out string reason)
	{
		reason = "";

		if (!state || !civilianZone || !playerEntity)
			return false;

		if (civilianZone.m_iPolicePresence <= 0 && civilianZone.m_iRoadblockPresence <= 0)
			return false;

		IEntity vehicle = ResolveEntityVehicle(playerEntity);
		if (!vehicle)
			return false;

		vector position = vehicle.GetOrigin();
		vector roadDestination = position;
		roadDestination[2] = roadDestination[2] + 1.0;
		vector roadPosition;
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;

		bool roadFound = HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(position, 35.0, roadDestination, roadPosition, roadForward, roadWidth, roadDistance, roadReason);
		if (!roadFound || roadDistance > 30.0)
		{
			reason = "off-road vehicle near police/roadblock";
			return true;
		}

		return false;
	}
	protected bool IsBlockingReason(string reason)
	{
		if (reason.IsEmpty())
			return false;

		return reason.Contains("BLOCK");
	}

	protected string ResolveClothingEligibilityReason(IEntity playerEntity)
	{
		string identity = ResolveEntityIdentity(playerEntity);
		if (identity.IsEmpty())
			return "WARN unknown clothing/entity identity";

		if (identity.Contains("CIV") || identity.Contains("Civilian") || identity.Contains("GenericCivilians"))
			return "OK civilian clothing";

		if (identity.Contains("US_Army") || identity.Contains("USSR") || identity.Contains("BLUFOR") || identity.Contains("OPFOR") || identity.Contains("Rifleman") || identity.Contains("MG") || identity.Contains("LAT"))
			return "BLOCK military character/equipment identity";

		if (identity.Contains("FIA"))
			return "WARN FIA clothing is not civilian";

		return "WARN clothing not recognized";
	}

	protected string ResolveWeaponEligibilityReason(IEntity playerEntity)
	{
		string identity = ResolveEntityIdentity(playerEntity);
		if (identity.Contains("Rifleman") || identity.Contains("MG") || identity.Contains("LAT") || identity.Contains("GL") || identity.Contains("MachineGun"))
			return "BLOCK military role implies visible/issued weapon";

		return "OK no visible military weapon detected by Phase 20 heuristic";
	}

	protected string ResolveVehicleEligibilityReason(HST_CampaignState state, IEntity playerEntity)
	{
		IEntity vehicle = ResolveEntityVehicle(playerEntity);
		if (!vehicle)
			return "OK on foot";

		string vehicleRuntimeId = ResolveRuntimeVehicleId(vehicle);
		if (state && !vehicleRuntimeId.IsEmpty())
		{
			string runtimeReason = ResolveRuntimeVehicleUndercoverReason(state, vehicleRuntimeId);
			if (runtimeReason.Contains("BLOCK"))
				return runtimeReason;

			HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
			if (runtimeVehicle && runtimeVehicle.m_bCanProvideUndercover)
				return runtimeReason;
		}

		string identity = ResolveEntityIdentity(vehicle);
		if (identity.Contains("M998") || identity.Contains("M1025") || identity.Contains("UAZ") || identity.Contains("BTR") || identity.Contains("BMP") || identity.Contains("APC") || identity.Contains("PKM") || identity.Contains("Armed"))
			return "BLOCK military vehicle";

		if (identity.Contains("S105") || identity.Contains("S1203") || identity.Contains("CIV"))
			return "OK civilian vehicle";

		return "WARN vehicle not recognized";
	}

	protected string ResolveOffroadEligibilityReason(IEntity playerEntity)
	{
		if (!playerEntity)
			return "WARN player entity missing";

		return "OK off-road enforcement active in Phase 21 tick";
	}

	protected string ResolveEnemyProximityReason(HST_CampaignState state, IEntity playerEntity)
	{
		if (!state || !playerEntity)
			return "WARN state/player missing";

		vector position = playerEntity.GetOrigin();
		float dangerRadiusSq = 180 * 180;
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!group)
				continue;
			if (group.m_sFactionKey == "FIA" || group.m_sFactionKey == "CIV")
				continue;
			if (group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "folded" || group.m_sRuntimeStatus == "spawn_failed")
				continue;
			if (DistanceSq2D(group.m_vPosition, position) <= dangerRadiusSq)
				return "WARN enemy active group nearby";
		}

		return "OK no nearby enemy active group";
	}

	protected string ResolveWantedHeatReason(HST_CampaignState state, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone)
	{
		if (!undercover)
			return "BLOCK undercover record missing";

		if (undercover.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_COMPROMISED && state.m_iElapsedSeconds < undercover.m_iCompromisedUntilSecond)
			return string.Format("BLOCK compromised for %1s", undercover.m_iCompromisedUntilSecond - state.m_iElapsedSeconds);

		if (undercover.m_iWantedHeat >= 4)
			return string.Format("BLOCK player wanted heat %1", undercover.m_iWantedHeat);

		if (civilianZone && civilianZone.m_iWantedHeat >= 4)
			return string.Format("BLOCK town wanted heat %1", civilianZone.m_iWantedHeat);

		if (civilianZone && civilianZone.m_bUndercoverRestricted)
			return "WARN town restricts undercover";

		return "OK wanted heat clear";
	}

	protected void StoreEligibilityResult(HST_CampaignState state, HST_PlayerUndercoverState undercover, HST_UndercoverEligibilityResult result)
	{
		if (!state || !undercover || !result)
			return;

		undercover.m_bLastEligibilityResult = result.m_bEligible;
		undercover.m_sLastZoneId = result.m_sZoneId;
		undercover.m_sLastEligibilitySummary = result.m_sSummary;
		undercover.m_sClothingReason = result.m_sClothingReason;
		undercover.m_sWeaponReason = result.m_sWeaponReason;
		undercover.m_sVehicleReason = result.m_sVehicleReason;
		undercover.m_sOffroadReason = result.m_sOffroadReason;
		undercover.m_sEnemyProximityReason = result.m_sEnemyProximityReason;
		undercover.m_sWantedHeatReason = result.m_sWantedHeatReason;
		undercover.m_iLastEligibilityCheckSecond = state.m_iElapsedSeconds;
	}

	protected string ResolveNearestCivilianZoneId(HST_CampaignState state, IEntity playerEntity)
	{
		if (!state || !playerEntity)
			return "";

		vector position = playerEntity.GetOrigin();
		HST_CivilianZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_CivilianZoneState civilianZone : state.m_aCivilianZones)
		{
			if (!civilianZone)
				continue;

			HST_ZoneState zone = state.FindZone(civilianZone.m_sZoneId);
			if (!zone)
				continue;

			float distanceSq = DistanceSq2D(zone.m_vPosition, position);
			if (distanceSq < bestDistanceSq)
			{
				bestZone = civilianZone;
				bestDistanceSq = distanceSq;
			}
		}

		if (bestZone)
			return bestZone.m_sZoneId;

		return "";
	}

	protected string ResolveEntityIdentity(IEntity entity)
	{
		if (!entity)
			return "";

		if (entity.GetPrefabData())
		{
			string prefab = entity.GetPrefabData().GetPrefabName();
			if (!prefab.IsEmpty())
				return prefab;
		}

		return entity.GetName();
	}

	protected IEntity ResolveEntityVehicle(IEntity entity)
	{
		if (!entity)
			return null;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(entity.FindComponent(SCR_CompartmentAccessComponent));
		if (!access || !access.IsInCompartment())
			return null;

		return access.GetVehicle();
	}

	protected bool SpawnActiveZoneRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ZoneState zone)
	{
		if (!state || !balance || !zone)
			return false;

		m_aRuntimeZoneIds.Insert(zone.m_sZoneId);

		int spawned;
		int civilianVehicleCount;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
		{
			HST_CivilianZoneState civilianZone = state.FindCivilianZone(zone.m_sZoneId);
			if (civilianZone)
			{
				int civilianCount = Math.Min(civilianZone.m_iCivilianPresence, balance.m_iCivilianMaxActivePerTown);
				if (civilianCount > 0 && CountGuidQualifiedCivilianCharacterPrefabs(balance) < MIN_CIVILIAN_CHARACTER_PREFABS)
					WarnMissingCivilianCharacterPool(zone, civilianCount);

				for (int i = 0; i < civilianCount; i++)
				{
					int civilianSeed = BuildZoneSeed(state, zone, 1000 + i);
					string civilianPrefab = SelectCivilianCharacterPrefab(balance, i, civilianSeed);
					if (civilianPrefab.IsEmpty())
						continue;

					vector position = ResolveTownCharacterSpawnPosition(zone, i, civilianSeed);
					vector angles = BuildSpawnAngles(civilianSeed);
					int beforeCivilianSpawn = m_aRuntimeEntities.Count();
					if (SpawnRuntimeEntity(state, zone.m_sZoneId, civilianPrefab, position, angles, "CIV", "CIV_CHARACTER"))
					{
						spawned++;
						if (beforeCivilianSpawn < m_aRuntimeEntities.Count())
							AssignCivilianPedestrianBehavior(m_aRuntimeEntities[beforeCivilianSpawn], zone, i, civilianSeed);
					}
				}
			}

			civilianVehicleCount = ResolveDeterministicCount(balance.m_iCivilianVehicleMinPerTown, balance.m_iCivilianVehicleMaxPerTown, BuildZoneSeed(state, zone, 101));
			array<vector> occupiedVehiclePositions = {};
			for (int j = 0; j < civilianVehicleCount; j++)
			{
				int civilianVehicleSeed = BuildZoneSeed(state, zone, 200 + j);
				vector vehiclePosition = ResolveTownVehicleSpawnPosition(zone, j, false);
				vector vehicleAngles = BuildSpawnAngles(civilianVehicleSeed);
				if (SpawnTownVehicleRuntimeEntity(state, zone, zone.m_sZoneId, SelectCivilianVehiclePrefab(balance, j, civilianVehicleSeed), vehiclePosition, vehicleAngles, "CIV", "CIV_VEHICLE", occupiedVehiclePositions, j, false))
					spawned++;
			}

			if (ShouldSpawnFactionTownVehicles(preset, zone))
			{
				int militaryVehicleCount = ResolveDeterministicCount(balance.m_iOccupierVehicleMinPerTown, balance.m_iOccupierVehicleMaxPerTown, BuildZoneSeed(state, zone, 401));
				for (int k = 0; k < militaryVehicleCount; k++)
				{
					string ownerPrefab = SelectFactionVehiclePrefab(zone.m_sOwnerFactionKey, k + BuildZoneSeed(state, zone, 503));
					if (ownerPrefab.IsEmpty())
						continue;

					vector ownerPosition = ResolveTownVehicleSpawnPosition(zone, civilianVehicleCount + k, true);
					vector ownerAngles = BuildSpawnAngles(BuildZoneSeed(state, zone, 700 + k));
					if (SpawnTownVehicleRuntimeEntity(state, zone, zone.m_sZoneId, ownerPrefab, ownerPosition, ownerAngles, zone.m_sOwnerFactionKey, "MILITARY_VEHICLE", occupiedVehiclePositions, civilianVehicleCount + k, true))
						spawned++;
				}
			}

			spawned += SpawnZoneCivilianTraffic(state, balance, zone, occupiedVehiclePositions, occupiedVehiclePositions.Count());
		}

		Print(string.Format("h-istasi civilians | zone %1 active | spawned %2 runtime civilian/military ambience entity(s)", zone.m_sZoneId, spawned));
		return true;
	}

	protected bool MaintainActiveZoneTraffic(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ZoneState zone)
	{
		if (!state || !balance || !zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return false;

		int targetTraffic = Math.Max(0, balance.m_iCivilianDrivingVehicleCountPerTown);
		int currentTraffic = CountRuntimeEntitiesForZone(zone.m_sZoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND, "CIV");
		if (currentTraffic >= targetTraffic)
			return false;

		array<vector> occupiedVehiclePositions = {};
		BuildOccupiedTownVehiclePositions(zone.m_sZoneId, occupiedVehiclePositions);
		int spawned = SpawnZoneCivilianTraffic(state, balance, zone, occupiedVehiclePositions, occupiedVehiclePositions.Count());
		return spawned > 0;
	}

	protected int SpawnZoneCivilianTraffic(HST_CampaignState state, HST_BalanceConfig balance, HST_ZoneState zone, array<vector> occupiedVehiclePositions, int baseSlotIndex)
	{
		if (!state || !balance || !zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return 0;

		int targetTraffic = Math.Max(0, balance.m_iCivilianDrivingVehicleCountPerTown);
		int currentTraffic = CountRuntimeEntitiesForZone(zone.m_sZoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND, "CIV");
		if (currentTraffic >= targetTraffic)
			return 0;

		int spawned;
		for (int i = currentTraffic; i < targetTraffic; i++)
		{
			int seed = BuildZoneSeed(state, zone, 3000 + i * 17);
			string vehiclePrefab = SelectCivilianVehiclePrefab(balance, i + baseSlotIndex, seed);
			if (vehiclePrefab.IsEmpty())
				continue;

			int vehicleIndex = baseSlotIndex + i;
			vector vehiclePosition = ResolveTownVehicleSpawnPosition(zone, vehicleIndex, false);
			vector vehicleAngles = BuildSpawnAngles(seed);
			int beforeVehicleSpawn = m_aRuntimeEntities.Count();
			if (!SpawnTownVehicleRuntimeEntity(state, zone, zone.m_sZoneId, vehiclePrefab, vehiclePosition, vehicleAngles, "CIV", CIVILIAN_TRAFFIC_RUNTIME_KIND, occupiedVehiclePositions, vehicleIndex, false))
				continue;

			spawned++;
			if (beforeVehicleSpawn < m_aRuntimeEntities.Count())
				AssignCivilianTrafficBehavior(state, balance, zone, m_aRuntimeEntities[beforeVehicleSpawn], i, seed);
		}

		return spawned;
	}

	protected void AssignCivilianPedestrianBehavior(IEntity civilianEntity, HST_ZoneState zone, int index, int seed)
	{
		if (!civilianEntity || !zone)
			return;

		AIGroup group = EnsureCivilianAIGroup(civilianEntity, civilianEntity, CIVILIAN_FACTION_KEY);
		if (!group)
			return;

		ApplyCivilianMovementSpeed(civilianEntity, group, EMovementType.WALK);
		array<vector> waypoints = {};
		waypoints.Insert(ResolveCivilianWanderPoint(zone, index, seed, 0));
		waypoints.Insert(ResolveCivilianWanderPoint(zone, index, seed, 1));
		AssignCivilianCycleWaypoints(civilianEntity, group, waypoints, CIVILIAN_WANDER_COMPLETION_RADIUS_METERS, false);
	}

	protected void AssignCivilianTrafficBehavior(HST_CampaignState state, HST_BalanceConfig balance, HST_ZoneState zone, IEntity vehicleEntity, int index, int seed)
	{
		if (!state || !balance || !zone || !vehicleEntity)
			return;

		IEntity driverEntity = SpawnCivilianTrafficDriver(state, balance, zone, vehicleEntity, index, seed);
		if (!driverEntity)
			return;

		AIGroup group = EnsureCivilianAIGroup(vehicleEntity, driverEntity, CIVILIAN_FACTION_KEY);
		if (!group)
			return;

		TryRegisterCivilianVehicleWithGroup(group, vehicleEntity);
		string seatingReason;
		if (!TryMoveCivilianDriverIntoVehicle(driverEntity, vehicleEntity, seatingReason))
		{
			Print(string.Format("h-istasi civilians | ambient traffic driver seating failed for %1: %2", zone.m_sZoneId, seatingReason), LogLevel.WARNING);
			return;
		}

		array<vector> routePositions = {};
		BuildCivilianTrafficRoute(zone, vehicleEntity.GetOrigin(), index, seed, routePositions);
		AssignCivilianCycleWaypoints(vehicleEntity, group, routePositions, CIVILIAN_TRAFFIC_WAYPOINT_RADIUS_METERS, true);
	}

	protected IEntity SpawnCivilianTrafficDriver(HST_CampaignState state, HST_BalanceConfig balance, HST_ZoneState zone, IEntity vehicleEntity, int index, int seed)
	{
		if (!state || !balance || !zone || !vehicleEntity)
			return null;

		string driverPrefab = SelectCivilianCharacterPrefab(balance, index, seed + 19);
		if (driverPrefab.IsEmpty())
			return null;

		vector driverPosition = vehicleEntity.GetOrigin();
		driverPosition[0] = driverPosition[0] + 1.4;
		driverPosition[2] = driverPosition[2] + 1.4;
		driverPosition = HST_WorldPositionService.ResolveSafeGroundPosition(driverPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 2.0);
		GenericEntity driverEntity = HST_WorldPositionService.SpawnPrefab(driverPrefab, driverPosition, BuildSpawnAngles(seed + 43));
		if (!driverEntity)
		{
			RecordSpawnFailure(zone.m_sZoneId, driverPrefab, "CIV_TRAFFIC_DRIVER", driverPosition, BuildSpawnAngles(seed + 43));
			return null;
		}

		ApplyFaction(driverEntity, CIVILIAN_FACTION_KEY, "CIV_CHARACTER");
		RegisterRuntimeHelper(vehicleEntity, driverEntity);
		return driverEntity;
	}

	protected AIGroup EnsureCivilianAIGroup(IEntity helperOwner, IEntity memberEntity, string factionKey)
	{
		if (!helperOwner || !memberEntity)
			return null;

		AIAgent agent = ResolveCivilianAgent(memberEntity);
		if (!agent)
			return null;

		AIGroup group = agent.GetParentGroup();
		if (group)
		{
			group.ActivateAllMembers();
			agent.ActivateAI();
			return group;
		}

		IEntity groupEntity = HST_WorldPositionService.SpawnPrefab(CIVILIAN_AI_GROUP_PREFAB, memberEntity.GetOrigin(), "0 0 0");
		group = AIGroup.Cast(groupEntity);
		if (!group)
		{
			if (groupEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(groupEntity);
			return null;
		}

		SCR_AIGroup scrGroup = SCR_AIGroup.Cast(group);
		if (scrGroup)
		{
			scrGroup.InitFactionKey(factionKey);
			scrGroup.AddAgentFromControlledEntity(memberEntity);
		}
		else
		{
			group.AddAgent(agent);
		}

		group.ActivateAllMembers();
		agent.ActivateAI();
		RegisterRuntimeHelper(helperOwner, groupEntity);
		return group;
	}

	protected AIAgent ResolveCivilianAgent(IEntity entity)
	{
		if (!entity)
			return null;

		AIControlComponent control = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
		if (!control)
			return null;

		AIAgent agent = control.GetControlAIAgent();
		if (!agent)
			agent = control.GetAIAgent();
		if (agent)
			control.ActivateAI();

		return agent;
	}

	protected void ApplyCivilianMovementSpeed(IEntity entity, AIGroup group, EMovementType movementType)
	{
		if (group)
		{
			AIGroupMovementComponent groupMovement = AIGroupMovementComponent.Cast(group.GetMovementComponent());
			if (!groupMovement)
				groupMovement = AIGroupMovementComponent.Cast(group.FindComponent(AIGroupMovementComponent));
			if (groupMovement)
			{
				groupMovement.SetGroupCharactersWantedMovementType(movementType);
				groupMovement.SetFormationDisplacement(1);
			}
		}

		AIAgent agent = ResolveCivilianAgent(entity);
		if (!agent)
			return;

		AICharacterMovementComponent movement = AICharacterMovementComponent.Cast(agent.GetMovementComponent());
		if (movement)
			movement.SetMovementTypeWanted(movementType);
	}

	protected int AssignCivilianCycleWaypoints(IEntity helperOwner, AIGroup group, array<vector> positions, float completionRadiusMeters, bool vehicleRoute)
	{
		if (!helperOwner || !group || !positions || positions.Count() < 2)
			return 0;

		array<IEntity> spawnedEntities = {};
		array<AIWaypoint> waypoints = {};
		for (int i = 0; i < positions.Count(); i++)
		{
			vector waypointPosition = positions[i];
			if (vehicleRoute)
				waypointPosition = ResolveCivilianTrafficWaypoint(waypointPosition, waypointPosition);
			else
				waypointPosition = HST_WorldPositionService.ResolveSafeGroundPosition(waypointPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 3.0);

			GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(CIVILIAN_WANDER_WAYPOINT_PREFAB, waypointPosition, "0 0 0");
			AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
			if (!waypoint)
			{
				if (waypointEntity)
					SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
				DeleteSpawnedHelperEntities(spawnedEntities);
				return 0;
			}

			waypoint.SetCompletionRadius(completionRadiusMeters);
			waypoints.Insert(waypoint);
			spawnedEntities.Insert(waypointEntity);
		}

		GenericEntity cycleEntity = HST_WorldPositionService.SpawnPrefab(CIVILIAN_WANDER_CYCLE_WAYPOINT_PREFAB, positions[0], "0 0 0");
		AIWaypointCycle waypointCycle = AIWaypointCycle.Cast(cycleEntity);
		if (!waypointCycle)
		{
			if (cycleEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(cycleEntity);
			DeleteSpawnedHelperEntities(spawnedEntities);
			return 0;
		}

		waypointCycle.SetWaypoints(waypoints);
		foreach (IEntity helper : spawnedEntities)
		{
			RegisterRuntimeHelper(helperOwner, helper);
		}
		RegisterRuntimeHelper(helperOwner, cycleEntity);
		group.AddWaypoint(waypointCycle);
		group.ActivateAllMembers();
		return waypoints.Count();
	}

	protected vector ResolveCivilianWanderPoint(HST_ZoneState zone, int index, int seed, int legIndex)
	{
		if (!zone)
			return "0 0 0";

		float radius = CIVILIAN_WANDER_MIN_RADIUS_METERS + ModInt(seed + legIndex * 31, CIVILIAN_WANDER_RADIUS_VARIANCE_METERS);
		float angle = ModInt(seed * 23 + index * 41 + legIndex * 137, 360) * 0.0174533;
		vector offset = "0 0 0";
		offset[0] = Math.Sin(angle) * radius;
		offset[2] = Math.Cos(angle) * radius;
		return HST_WorldPositionService.ResolveSafeGroundPosition(zone.m_vPosition + offset, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 3.0);
	}

	protected void BuildCivilianTrafficRoute(HST_ZoneState zone, vector vehiclePosition, int index, int seed, array<vector> routePositions)
	{
		if (!routePositions)
			return;

		routePositions.Clear();
		if (!zone)
			return;

		routePositions.Insert(ResolveCivilianTrafficWaypoint(vehiclePosition, zone.m_vPosition));
		for (int leg = 0; leg < 3; leg++)
		{
			float radius = CIVILIAN_TRAFFIC_ROUTE_MIN_RADIUS_METERS + ModInt(seed + leg * 73, CIVILIAN_TRAFFIC_ROUTE_RADIUS_VARIANCE_METERS);
			float angle = ModInt(seed * 19 + index * 57 + leg * 119, 360) * 0.0174533;
			vector offset = "0 0 0";
			offset[0] = Math.Sin(angle) * radius;
			offset[2] = Math.Cos(angle) * radius;
			vector preferred = zone.m_vPosition + offset;
			routePositions.Insert(ResolveCivilianTrafficWaypoint(preferred, zone.m_vPosition));
		}
	}

	protected vector ResolveCivilianTrafficWaypoint(vector preferredPosition, vector destination)
	{
		vector roadPosition;
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		if (HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(preferredPosition, TOWN_VEHICLE_ROAD_SEARCH_RADIUS_METERS, destination, roadPosition, roadForward, roadWidth, roadDistance, roadReason))
			return roadPosition;

		return HST_WorldPositionService.ResolveSafeGroundPosition(preferredPosition, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true, 8.0);
	}

	protected bool TryRegisterCivilianVehicleWithGroup(AIGroup group, IEntity vehicleEntity)
	{
		if (!group || !vehicleEntity)
			return false;

		SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
		if (!utility)
			return false;

		IEntity vehicleUsageOwner = vehicleEntity;
		SCR_AIVehicleUsageComponent vehicleUsage = SCR_AIVehicleUsageComponent.FindOnNearestParent(vehicleEntity, vehicleUsageOwner);
		if (!vehicleUsage || !vehicleUsage.IsVehicleTypeValid() || !vehicleUsage.CanBePiloted())
			return false;

		utility.AddUsableVehicle(vehicleUsage);
		return utility.IsUsableVehicle(vehicleUsage);
	}

	protected bool TryMoveCivilianDriverIntoVehicle(IEntity driverEntity, IEntity vehicleEntity, out string reason)
	{
		reason = "";
		if (!driverEntity || !vehicleEntity)
		{
			reason = "driver or vehicle missing";
			return false;
		}

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(driverEntity.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
		{
			reason = "driver has no compartment access component";
			return false;
		}
		if (access.IsInCompartment() && access.GetVehicle() == vehicleEntity)
		{
			reason = "driver already seated";
			return true;
		}
		if (access.IsGettingIn())
		{
			reason = "driver already getting in";
			return true;
		}

		BaseCompartmentManagerComponent compartmentManager = ResolveCompartmentManager(vehicleEntity);
		if (!compartmentManager)
		{
			reason = "vehicle has no compartment manager";
			return false;
		}

		array<BaseCompartmentSlot> slots = {};
		compartmentManager.GetCompartments(slots);
		BaseCompartmentSlot slot = FindAvailableDriverSlot(slots, driverEntity);
		if (!slot)
		{
			reason = "vehicle has no available driver slot";
			return false;
		}

		if (access.MoveInVehicle(vehicleEntity, ECompartmentType.PILOT, true, slot))
		{
			reason = "driver moved into pilot slot";
			return true;
		}

		IEntity slotOwner = slot.GetOwner();
		if (!slotOwner)
			slotOwner = vehicleEntity;
		if (access.GetInVehicle(slotOwner, slot, true, -1, ECloseDoorAfterActions.INVALID, true))
		{
			reason = "driver get-in order accepted";
			return true;
		}

		reason = "driver seating request rejected";
		return false;
	}

	protected BaseCompartmentManagerComponent ResolveCompartmentManager(IEntity vehicleEntity)
	{
		if (!vehicleEntity)
			return null;

		SCR_BaseCompartmentManagerComponent scrManager = SCR_BaseCompartmentManagerComponent.Cast(vehicleEntity.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (scrManager)
			return scrManager;

		return BaseCompartmentManagerComponent.Cast(vehicleEntity.FindComponent(BaseCompartmentManagerComponent));
	}

	protected BaseCompartmentSlot FindAvailableDriverSlot(array<BaseCompartmentSlot> slots, IEntity driverEntity)
	{
		if (!slots || !driverEntity)
			return null;

		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;
			if (!slot.IsPiloting() && slot.GetType() != ECompartmentType.PILOT)
				continue;
			if (!slot.IsCompartmentAccessible())
				continue;
			if (slot.IsOccupied())
				continue;
			if (slot.IsReserved() && !slot.IsReservedBy(driverEntity))
				continue;
			if (slot.IsGetInLockedFor(driverEntity))
				continue;

			return slot;
		}

		return null;
	}

	protected void BuildOccupiedTownVehiclePositions(string zoneId, array<vector> occupiedVehiclePositions)
	{
		if (!occupiedVehiclePositions)
			return;

		occupiedVehiclePositions.Clear();
		if (zoneId.IsEmpty())
			return;

		for (int i = 0; i < m_aRuntimeEntities.Count(); i++)
		{
			if (i >= m_aRuntimeEntityZoneIds.Count() || m_aRuntimeEntityZoneIds[i] != zoneId)
				continue;
			if (i >= m_aRuntimeEntityKinds.Count() || !IsRuntimeVehicle(m_aRuntimeEntityKinds[i]))
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			if (entity)
				occupiedVehiclePositions.Insert(entity.GetOrigin());
		}
	}

	protected bool SpawnRuntimeEntity(HST_CampaignState state, string zoneId, string prefab, vector position, vector angles, string factionKey, string runtimeKind)
	{
		if (zoneId.IsEmpty() || prefab.IsEmpty())
			return false;

		if (IsRuntimeVehicle(runtimeKind))
		{
			vector vehiclePosition;
			vector vehicleAngles;
			ResolveTownVehicleSpawnTransform(position, angles, vehiclePosition, vehicleAngles);
			position = vehiclePosition;
			angles = vehicleAngles;
		}
		else
		{
			position = HST_WorldPositionService.ResolveSafeGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 2.0);
		}

		return SpawnResolvedRuntimeEntity(state, zoneId, prefab, position, angles, factionKey, runtimeKind);
	}

	protected bool SpawnTownVehicleRuntimeEntity(HST_CampaignState state, HST_ZoneState zone, string zoneId, string prefab, vector preferredPosition, vector preferredAngles, string factionKey, string runtimeKind, array<vector> occupiedVehiclePositions, int vehicleSlotIndex, bool militaryVehicle)
	{
		if (zoneId.IsEmpty() || prefab.IsEmpty())
			return false;

		if (!IsRuntimeVehicle(runtimeKind) || !occupiedVehiclePositions)
			return SpawnRuntimeEntity(state, zoneId, prefab, preferredPosition, preferredAngles, factionKey, runtimeKind);

		vector bestPosition;
		vector bestAngles;
		float bestDistanceSq = -1.0;
		bool hasBest;
		for (int attempt = 0; attempt < TOWN_VEHICLE_POSITION_ATTEMPTS; attempt++)
		{
			vector attemptPosition = preferredPosition;
			vector attemptAngles = preferredAngles;
			if (attempt > 0 && zone)
			{
				int alternateIndex = vehicleSlotIndex + attempt * 17;
				attemptPosition = ResolveTownVehicleSpawnPosition(zone, alternateIndex, militaryVehicle);
				attemptAngles = BuildSpawnAngles(BuildTownSpawnSeed(zone, alternateIndex, militaryVehicle) + attempt * 31);
			}

			vector resolvedPosition;
			vector resolvedAngles;
			ResolveTownVehicleSpawnTransform(attemptPosition, attemptAngles, resolvedPosition, resolvedAngles);
			float nearestDistanceSq = ResolveNearestVehicleSpawnDistanceSq(resolvedPosition, occupiedVehiclePositions);
			if (!hasBest || nearestDistanceSq > bestDistanceSq)
			{
				bestPosition = resolvedPosition;
				bestAngles = resolvedAngles;
				bestDistanceSq = nearestDistanceSq;
				hasBest = true;
			}

			if (!IsVehicleSpawnSeparated(resolvedPosition, occupiedVehiclePositions))
				continue;

			if (!SpawnResolvedRuntimeEntity(state, zoneId, prefab, resolvedPosition, resolvedAngles, factionKey, runtimeKind))
				return false;

			occupiedVehiclePositions.Insert(resolvedPosition);
			return true;
		}

		if (!hasBest)
			return false;

		if (!SpawnResolvedRuntimeEntity(state, zoneId, prefab, bestPosition, bestAngles, factionKey, runtimeKind))
			return false;

		occupiedVehiclePositions.Insert(bestPosition);
		return true;
	}

	protected bool SpawnResolvedRuntimeEntity(HST_CampaignState state, string zoneId, string prefab, vector position, vector angles, string factionKey, string runtimeKind)
	{
		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return false;

		GenericEntity entity = HST_WorldPositionService.SpawnPrefab(prefab, position, angles);
		if (!entity)
		{
			RecordSpawnFailure(zoneId, prefab, runtimeKind, position, angles);
			return false;
		}
		if (IsRuntimeVehicle(runtimeKind))
			HST_WorldPositionService.ApplyUprightEntityTransform(entity, position, angles);

		ApplyFaction(entity, factionKey, runtimeKind);
		m_aRuntimeEntityZoneIds.Insert(zoneId);
		m_aRuntimeEntityKinds.Insert(runtimeKind);
		m_aRuntimeEntityFactionKeys.Insert(factionKey);
		m_aRuntimeEntitySpawnPositions.Insert(position);
		m_aRuntimeEntities.Insert(entity);
		RegisterRuntimeVehicle(state, entity, zoneId, prefab, position, angles, factionKey, runtimeKind);
		Print(string.Format("h-istasi civilians | spawn ok | zone %1 | kind %2 | faction %3 | prefab %4 | pos %5 | yaw %6", zoneId, runtimeKind, factionKey, prefab, position, angles[0]));
		return true;
	}

	protected bool CleanupZoneRuntimeEntities(HST_CampaignState state, string zoneId)
	{
		if (zoneId.IsEmpty())
			return false;

		bool changed;
		for (int i = m_aRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeEntityZoneIds[i] != zoneId)
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			string runtimeKind = m_aRuntimeEntityKinds[i];
			vector spawnPosition = m_aRuntimeEntitySpawnPositions[i];
			DeleteRuntimeHelpersForOwner(entity);
			if (entity && ShouldDetachFromTownCleanup(entity, runtimeKind, spawnPosition))
			{
				MarkRuntimeVehicleDetached(state, entity);
				Print(string.Format("h-istasi civilians | detached player-used runtime %1 from town cleanup for %2", runtimeKind, zoneId));
			}
			else if (entity)
			{
				MarkRuntimeVehicleDeleted(state, entity);
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			}

			m_aRuntimeEntities.Remove(i);
			m_aRuntimeEntityZoneIds.Remove(i);
			m_aRuntimeEntityKinds.Remove(i);
			if (i < m_aRuntimeEntityFactionKeys.Count())
				m_aRuntimeEntityFactionKeys.Remove(i);
			m_aRuntimeEntitySpawnPositions.Remove(i);
			changed = true;
		}

		for (int j = m_aRuntimeZoneIds.Count() - 1; j >= 0; j--)
		{
			if (m_aRuntimeZoneIds[j] == zoneId)
				m_aRuntimeZoneIds.Remove(j);
		}

		if (changed)
			Print(string.Format("h-istasi civilians | cleaned runtime town population for %1", zoneId));

		return changed;
	}

	protected bool CleanupAllRuntimeEntities(HST_CampaignState state)
	{
		bool changed;
		for (int i = m_aRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			IEntity entity = m_aRuntimeEntities[i];
			string runtimeKind = m_aRuntimeEntityKinds[i];
			vector spawnPosition = m_aRuntimeEntitySpawnPositions[i];
			DeleteRuntimeHelpersForOwner(entity);
			if (entity && ShouldDetachFromTownCleanup(entity, runtimeKind, spawnPosition))
			{
				MarkRuntimeVehicleDetached(state, entity);
				Print(string.Format("h-istasi civilians | detached player-used runtime %1 while civilian runtime disabled", runtimeKind));
			}
			else if (entity)
			{
				MarkRuntimeVehicleDeleted(state, entity);
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			}

			changed = true;
		}

		m_aRuntimeEntities.Clear();
		m_aRuntimeEntityZoneIds.Clear();
		m_aRuntimeEntityKinds.Clear();
		m_aRuntimeEntityFactionKeys.Clear();
		m_aRuntimeEntitySpawnPositions.Clear();
		m_aRuntimeZoneIds.Clear();
		CleanupAllRuntimeHelpers();
		return changed;
	}

	protected void PruneDeletedRuntimeEntities(HST_CampaignState state)
	{
		PruneDeletedRuntimeHelpers();
		for (int i = m_aRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeEntities[i])
				continue;

			DeleteRuntimeHelpersForOwner(m_aRuntimeEntities[i]);
			MarkRuntimeVehicleDeletedBySpawnRecord(state, m_aRuntimeEntitySpawnPositions[i], m_aRuntimeEntityKinds[i]);
			m_aRuntimeEntities.Remove(i);
			m_aRuntimeEntityZoneIds.Remove(i);
			m_aRuntimeEntityKinds.Remove(i);
			if (i < m_aRuntimeEntityFactionKeys.Count())
				m_aRuntimeEntityFactionKeys.Remove(i);
			m_aRuntimeEntitySpawnPositions.Remove(i);
		}
	}

	protected bool PruneAmbientTrafficOutsideRenderBubble(HST_CampaignState state, HST_BalanceConfig balance)
	{
		if (!state || !balance)
			return false;

		bool changed;
		float renderRadius = Math.Max(100.0, balance.m_iPlayerRenderBubbleRadiusMeters);
		for (int i = m_aRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			if (i >= m_aRuntimeEntityKinds.Count() || m_aRuntimeEntityKinds[i] != CIVILIAN_TRAFFIC_RUNTIME_KIND)
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			if (!entity)
				continue;

			if (HST_WorldPositionService.IsPositionNearLivingPlayer(entity.GetOrigin(), renderRadius))
				continue;

			DeleteRuntimeHelpersForOwner(entity);
			MarkRuntimeVehicleDeleted(state, entity);
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			RemoveRuntimeEntityAt(i);
			changed = true;
		}

		return changed;
	}

	protected void RemoveRuntimeEntityAt(int index)
	{
		if (index < 0 || index >= m_aRuntimeEntities.Count())
			return;

		m_aRuntimeEntities.Remove(index);
		if (index < m_aRuntimeEntityZoneIds.Count())
			m_aRuntimeEntityZoneIds.Remove(index);
		if (index < m_aRuntimeEntityKinds.Count())
			m_aRuntimeEntityKinds.Remove(index);
		if (index < m_aRuntimeEntityFactionKeys.Count())
			m_aRuntimeEntityFactionKeys.Remove(index);
		if (index < m_aRuntimeEntitySpawnPositions.Count())
			m_aRuntimeEntitySpawnPositions.Remove(index);
	}

	protected void RegisterRuntimeHelper(IEntity owner, IEntity helper)
	{
		if (!owner || !helper)
			return;

		m_aRuntimeHelperOwners.Insert(owner);
		m_aRuntimeHelperEntities.Insert(helper);
	}

	protected void DeleteRuntimeHelpersForOwner(IEntity owner)
	{
		if (!owner)
			return;

		for (int i = m_aRuntimeHelperOwners.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeHelperOwners[i] != owner)
				continue;

			IEntity helper = m_aRuntimeHelperEntities[i];
			if (helper)
				SCR_EntityHelper.DeleteEntityAndChildren(helper);
			m_aRuntimeHelperOwners.Remove(i);
			m_aRuntimeHelperEntities.Remove(i);
		}
	}

	protected void CleanupAllRuntimeHelpers()
	{
		for (int i = m_aRuntimeHelperEntities.Count() - 1; i >= 0; i--)
		{
			IEntity helper = m_aRuntimeHelperEntities[i];
			if (helper)
				SCR_EntityHelper.DeleteEntityAndChildren(helper);
		}

		m_aRuntimeHelperOwners.Clear();
		m_aRuntimeHelperEntities.Clear();
	}

	protected void PruneDeletedRuntimeHelpers()
	{
		for (int i = m_aRuntimeHelperEntities.Count() - 1; i >= 0; i--)
		{
			IEntity owner;
			if (i < m_aRuntimeHelperOwners.Count())
				owner = m_aRuntimeHelperOwners[i];
			IEntity helper = m_aRuntimeHelperEntities[i];
			if (owner && helper)
				continue;

			if (helper)
				SCR_EntityHelper.DeleteEntityAndChildren(helper);
			if (i < m_aRuntimeHelperOwners.Count())
				m_aRuntimeHelperOwners.Remove(i);
			m_aRuntimeHelperEntities.Remove(i);
		}
	}

	protected void DeleteSpawnedHelperEntities(array<IEntity> helperEntities)
	{
		if (!helperEntities)
			return;

		foreach (IEntity helper : helperEntities)
		{
			if (helper)
				SCR_EntityHelper.DeleteEntityAndChildren(helper);
		}
	}

	protected bool HasRuntimeZone(string zoneId)
	{
		return m_aRuntimeZoneIds.Contains(zoneId);
	}

	bool HasRuntimeTownZone(string zoneId)
	{
		return HasRuntimeZone(zoneId);
	}

	int CountRuntimeEntitiesForZone(string zoneId, string runtimeKind = "", string factionKey = "")
	{
		if (zoneId.IsEmpty())
			return 0;

		int count;
		for (int i = 0; i < m_aRuntimeEntities.Count(); i++)
		{
			if (m_aRuntimeEntityZoneIds[i] != zoneId)
				continue;
			if (!runtimeKind.IsEmpty() && m_aRuntimeEntityKinds[i] != runtimeKind)
				continue;
			string runtimeFactionKey;
			if (i < m_aRuntimeEntityFactionKeys.Count())
				runtimeFactionKey = m_aRuntimeEntityFactionKeys[i];
			if (!factionKey.IsEmpty() && runtimeFactionKey != factionKey)
				continue;
			if (!m_aRuntimeEntities[i])
				continue;

			count++;
		}

		return count;
	}

	int CountRuntimeEntitiesForZoneWithHelpers(string zoneId, string runtimeKind = "", string factionKey = "", int minHelperCount = 1)
	{
		if (zoneId.IsEmpty())
			return 0;

		int count;
		for (int i = 0; i < m_aRuntimeEntities.Count(); i++)
		{
			if (!MatchesRuntimeEntityFilter(i, zoneId, runtimeKind, factionKey))
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			if (!entity)
				continue;

			if (CountRuntimeHelpersForOwner(entity) >= Math.Max(1, minHelperCount))
				count++;
		}

		return count;
	}

	int CountRuntimeHelpersForZone(string zoneId, string runtimeKind = "", string factionKey = "")
	{
		if (zoneId.IsEmpty())
			return 0;

		int helperCount;
		for (int i = 0; i < m_aRuntimeEntities.Count(); i++)
		{
			if (!MatchesRuntimeEntityFilter(i, zoneId, runtimeKind, factionKey))
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			if (entity)
				helperCount += CountRuntimeHelpersForOwner(entity);
		}

		return helperCount;
	}

	int CountRuntimeHelpersForOwner(IEntity owner)
	{
		if (!owner)
			return 0;

		int count;
		for (int i = 0; i < m_aRuntimeHelperOwners.Count(); i++)
		{
			if (m_aRuntimeHelperOwners[i] != owner)
				continue;
			if (i >= m_aRuntimeHelperEntities.Count() || !m_aRuntimeHelperEntities[i])
				continue;

			count++;
		}

		return count;
	}

	int CountRuntimeEntitiesForZoneOutsideRadius(string zoneId, vector center, float radiusMeters, string runtimeKind = "", string factionKey = "")
	{
		if (zoneId.IsEmpty())
			return 0;

		float radiusSq = radiusMeters * radiusMeters;
		int count;
		for (int i = 0; i < m_aRuntimeEntities.Count(); i++)
		{
			if (m_aRuntimeEntityZoneIds[i] != zoneId)
				continue;
			if (!runtimeKind.IsEmpty() && m_aRuntimeEntityKinds[i] != runtimeKind)
				continue;
			string runtimeFactionKey;
			if (i < m_aRuntimeEntityFactionKeys.Count())
				runtimeFactionKey = m_aRuntimeEntityFactionKeys[i];
			if (!factionKey.IsEmpty() && runtimeFactionKey != factionKey)
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			if (!entity)
				continue;
			if (DistanceSq2D(entity.GetOrigin(), center) > radiusSq)
				count++;
		}

		return count;
	}

	int CountRuntimeEntitiesForZoneMovedFromSpawn(string zoneId, float minDistanceMeters, string runtimeKind = "", string factionKey = "")
	{
		if (zoneId.IsEmpty())
			return 0;

		float minDistanceSq = minDistanceMeters * minDistanceMeters;
		int count;
		for (int i = 0; i < m_aRuntimeEntities.Count(); i++)
		{
			if (!MatchesRuntimeEntityFilter(i, zoneId, runtimeKind, factionKey))
				continue;
			if (i >= m_aRuntimeEntitySpawnPositions.Count())
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			if (!entity)
				continue;
			if (DistanceSq2D(entity.GetOrigin(), m_aRuntimeEntitySpawnPositions[i]) >= minDistanceSq)
				count++;
		}

		return count;
	}

	float ResolveRuntimeEntitiesForZoneMaxMovementFromSpawn(string zoneId, string runtimeKind = "", string factionKey = "")
	{
		if (zoneId.IsEmpty())
			return 0.0;

		float maxMovementSq;
		for (int i = 0; i < m_aRuntimeEntities.Count(); i++)
		{
			if (!MatchesRuntimeEntityFilter(i, zoneId, runtimeKind, factionKey))
				continue;
			if (i >= m_aRuntimeEntitySpawnPositions.Count())
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			if (!entity)
				continue;

			float movementSq = DistanceSq2D(entity.GetOrigin(), m_aRuntimeEntitySpawnPositions[i]);
			if (movementSq > maxMovementSq)
				maxMovementSq = movementSq;
		}

		return Math.Sqrt(maxMovementSq);
	}

	string BuildRuntimeMovementSampleReport(string zoneId, float minDistanceMeters, string runtimeKind = "", string factionKey = "")
	{
		int total = CountRuntimeEntitiesForZone(zoneId, runtimeKind, factionKey);
		int moved = CountRuntimeEntitiesForZoneMovedFromSpawn(zoneId, minDistanceMeters, runtimeKind, factionKey);
		float maxMovement = ResolveRuntimeEntitiesForZoneMaxMovementFromSpawn(zoneId, runtimeKind, factionKey);
		string report = string.Format("runtime movement %1 | kind %2 | faction %3 | moved %4/%5", EmptyRuntimeField(zoneId), EmptyRuntimeField(runtimeKind), EmptyRuntimeField(factionKey), moved, total);
		return report + string.Format(" | threshold %1m | max %2m", Math.Round(minDistanceMeters), Math.Round(maxMovement));
	}

	string BuildRuntimeBehaviorReport(string zoneId)
	{
		int civilianCharacters = CountRuntimeEntitiesForZone(zoneId, "CIV_CHARACTER", "CIV");
		int pedestrianHelpers = CountRuntimeHelpersForZone(zoneId, "CIV_CHARACTER", "CIV");
		int pedestrianBehavior = CountRuntimeEntitiesForZoneWithHelpers(zoneId, "CIV_CHARACTER", "CIV", 3);
		int trafficVehicles = CountRuntimeEntitiesForZone(zoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND, "CIV");
		int trafficHelpers = CountRuntimeHelpersForZone(zoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND, "CIV");
		int trafficBehavior = CountRuntimeEntitiesForZoneWithHelpers(zoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND, "CIV", 5);
		return string.Format("runtime behavior %1 | pedestrians %2/%3 with helpers | pedestrian helpers %4 | traffic %5/%6 with helpers | traffic helpers %7", EmptyRuntimeField(zoneId), pedestrianBehavior, civilianCharacters, pedestrianHelpers, trafficBehavior, trafficVehicles, trafficHelpers);
	}

	string BuildRuntimeTownPopulationReport(HST_CampaignState state, string zoneId)
	{
		int civilianCharacters = CountRuntimeEntitiesForZone(zoneId, "CIV_CHARACTER", "CIV");
		int civilianVehicles = CountRuntimeEntitiesForZone(zoneId, "CIV_VEHICLE", "CIV");
		int civilianTrafficVehicles = CountRuntimeEntitiesForZone(zoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND, "CIV");
		int militaryVehicles = CountRuntimeEntitiesForZone(zoneId, "MILITARY_VEHICLE");
		int total = CountRuntimeEntitiesForZone(zoneId);
		string report = string.Format("runtime town %1 | active %2 | total %3 | civ chars %4 | parked civ vehicles %5 | traffic vehicles %6 | military vehicles %7", EmptyRuntimeField(zoneId), HasRuntimeZone(zoneId), total, civilianCharacters, civilianVehicles, civilianTrafficVehicles, militaryVehicles);
		if (state)
			report = report + string.Format(" | global civ chars %1 | civ vehicles %2 | failures %3 | last failure %4", state.m_iRuntimeCivilianCharacterCount, state.m_iRuntimeCivilianVehicleCount, state.m_iRuntimeSpawnFailureCount, EmptyRuntimeField(state.m_sLastRuntimeSpawnFailurePrefab));

		return report;
	}

	string BuildTownInfluenceReport(HST_CampaignState state, int maxRows = 20)
	{
		if (!state)
			return "h-istasi town influence | state not ready";

		RefreshTownInfluenceAggregates(state);
		int events;
		int applied;
		int active;
		int expired;
		foreach (HST_TownInfluenceEventState influenceEvent : state.m_aTownInfluenceEvents)
		{
			if (!influenceEvent)
				continue;

			events++;
			if (influenceEvent.m_bApplied)
				applied++;
			if (influenceEvent.m_iExpiresAtSecond > 0)
			{
				if (influenceEvent.m_iExpiresAtSecond > state.m_iElapsedSeconds)
					active++;
				else
					expired++;
			}
		}

		string report = string.Format("h-istasi town influence | events %1 | applied %2 | active modifiers %3 | expired modifiers %4", events, applied, active, expired);
		int emitted;
		for (int i = state.m_aTownInfluenceEvents.Count() - 1; i >= 0; i--)
		{
			if (emitted >= maxRows)
				break;

			HST_TownInfluenceEventState influenceEvent = state.m_aTownInfluenceEvents[i];
			if (!influenceEvent)
				continue;

			int remaining;
			if (influenceEvent.m_iExpiresAtSecond > 0)
				remaining = influenceEvent.m_iExpiresAtSecond - state.m_iElapsedSeconds;

			string line = string.Format("\n%1 | zone %2 | kind %3 | FIA %4 occ %5 rep %6 heat %7 pop %8",
				influenceEvent.m_sEventId,
				influenceEvent.m_sZoneId,
				influenceEvent.m_sKind,
				influenceEvent.m_iFIASupportDelta,
				influenceEvent.m_iOccupierSupportDelta,
				influenceEvent.m_iReputationDelta,
				influenceEvent.m_iHeatDelta,
				influenceEvent.m_iPopulationDelta);
			line = line + string.Format(" | police %1 road %2 | applied %3 | remaining %4 | %5",
				influenceEvent.m_iPoliceDelta,
				influenceEvent.m_iRoadblockDelta,
				influenceEvent.m_bApplied,
				remaining,
				influenceEvent.m_sReason);
			report = report + line;
			emitted++;
		}

		if (emitted == 0)
			report = report + "\nno town influence events";

		return report;
	}

	protected bool MatchesRuntimeEntityFilter(int index, string zoneId, string runtimeKind = "", string factionKey = "")
	{
		if (index < 0 || index >= m_aRuntimeEntities.Count())
			return false;
		if (index >= m_aRuntimeEntityZoneIds.Count() || m_aRuntimeEntityZoneIds[index] != zoneId)
			return false;
		if (!runtimeKind.IsEmpty())
		{
			if (index >= m_aRuntimeEntityKinds.Count() || m_aRuntimeEntityKinds[index] != runtimeKind)
				return false;
		}

		if (!factionKey.IsEmpty())
		{
			string runtimeFactionKey;
			if (index < m_aRuntimeEntityFactionKeys.Count())
				runtimeFactionKey = m_aRuntimeEntityFactionKeys[index];
			if (runtimeFactionKey != factionKey)
				return false;
		}

		return true;
	}

	protected string EmptyRuntimeField(string value)
	{
		if (value.IsEmpty())
			return "none";

		return value;
	}

	protected vector ResolveTownCharacterSpawnPosition(HST_ZoneState zone, int index, int seed)
	{
		if (!zone)
			return "0 0 0";

		vector offset = "0 0 0";
		float radius = 8.0 + ModInt(seed, 24);
		float angle = ModInt(seed * 29 + index * 43, 360) * 0.0174533;
		offset[0] = Math.Sin(angle) * radius;
		offset[2] = Math.Cos(angle) * radius;
		return HST_WorldPositionService.ResolveGroundPosition(zone.m_vPosition + offset, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true);
	}

	protected vector ResolveTownVehicleSpawnPosition(HST_ZoneState zone, int index, bool militaryVehicle)
	{
		if (!zone)
			return "0 0 0";

		int seed = BuildTownSpawnSeed(zone, index, militaryVehicle);
		float radius = TOWN_CIVILIAN_VEHICLE_BASE_RADIUS_METERS + ModInt(seed, TOWN_CIVILIAN_VEHICLE_RADIUS_VARIANCE_METERS);
		if (militaryVehicle)
			radius = TOWN_MILITARY_VEHICLE_BASE_RADIUS_METERS + ModInt(seed, TOWN_MILITARY_VEHICLE_RADIUS_VARIANCE_METERS);

		float angle = ModInt(seed * 37, 360) * 0.0174533;
		vector offset = "0 0 0";
		offset[0] = Math.Sin(angle) * radius;
		offset[2] = Math.Cos(angle) * radius;
		return HST_WorldPositionService.ResolveGroundPosition(zone.m_vPosition + offset, HST_WorldPositionService.PROP_GROUND_OFFSET, true);
	}

	protected int BuildTownSpawnSeed(HST_ZoneState zone, int index, bool militaryVehicle)
	{
		int seed = 17 + index * 53;
		if (zone)
		{
			seed = seed + zone.m_iPriority * 31 + zone.m_sZoneId.Length() * 19;
			seed = seed + Math.Round(zone.m_vPosition[0]) + Math.Round(zone.m_vPosition[2]);
		}

		if (militaryVehicle)
			seed = seed + 997;

		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected int BuildZoneSeed(HST_CampaignState state, HST_ZoneState zone, int salt)
	{
		int seed = salt * 31;
		if (state)
			seed += state.m_iCampaignSeed * 17;
		if (zone)
		{
			seed += zone.m_iPriority * 37;
			seed += zone.m_iIncomeValue * 13;
			seed += zone.m_sZoneId.Length() * 101;
			seed += Math.Round(zone.m_vPosition[0]) + Math.Round(zone.m_vPosition[2]);
		}

		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected int ResolveDeterministicCount(int minCount, int maxCount, int seed)
	{
		if (maxCount <= 0)
			return 0;

		if (maxCount <= minCount)
			return Math.Max(0, minCount);

		int span = maxCount - minCount + 1;
		int value = ModInt(seed, span);

		return minCount + value;
	}

	protected int ModInt(int value, int divisor)
	{
		if (divisor <= 0)
			return 0;

		int result = value - (value / divisor) * divisor;
		if (result < 0)
			result = result + divisor;

		return result;
	}

	protected vector BuildSpawnAngles(int seed)
	{
		vector angles;
		angles[0] = ModInt(seed * 47 + 91, 360);
		return HST_WorldPositionService.BuildUprightAnglesFromVector(angles);
	}

	protected void ResolveTownVehicleSpawnTransform(vector preferredPosition, vector preferredAngles, out vector resolvedPosition, out vector resolvedAngles)
	{
		resolvedPosition = preferredPosition;
		resolvedAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(preferredAngles);

		vector roadPosition;
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		vector roadDestination = BuildVehicleForwardTarget(preferredPosition, resolvedAngles);
		if (HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(preferredPosition, TOWN_VEHICLE_ROAD_SEARCH_RADIUS_METERS, roadDestination, roadPosition, roadForward, roadWidth, roadDistance, roadReason))
		{
			resolvedPosition = roadPosition;
			resolvedAngles = BuildVehicleAnglesFromForward(roadPosition, roadForward, resolvedAngles);
			return;
		}

		vector safeVehiclePosition;
		if (HST_WorldPositionService.TryResolveVehicleSpawnPosition(preferredPosition, safeVehiclePosition, true))
			resolvedPosition = safeVehiclePosition;
		else
			resolvedPosition = HST_WorldPositionService.ResolveGroundPosition(preferredPosition, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true);
	}

	protected bool IsVehicleSpawnSeparated(vector position, array<vector> occupiedVehiclePositions)
	{
		return ResolveNearestVehicleSpawnDistanceSq(position, occupiedVehiclePositions) >= TOWN_VEHICLE_MIN_SEPARATION_METERS * TOWN_VEHICLE_MIN_SEPARATION_METERS;
	}

	protected float ResolveNearestVehicleSpawnDistanceSq(vector position, array<vector> occupiedVehiclePositions)
	{
		if (!occupiedVehiclePositions || occupiedVehiclePositions.Count() == 0)
			return TOWN_VEHICLE_MIN_SEPARATION_METERS * TOWN_VEHICLE_MIN_SEPARATION_METERS;

		float nearestDistanceSq = 999999999.0;
		foreach (vector occupiedPosition : occupiedVehiclePositions)
		{
			float distanceSq = DistanceSq2D(position, occupiedPosition);
			if (distanceSq < nearestDistanceSq)
				nearestDistanceSq = distanceSq;
		}

		return nearestDistanceSq;
	}

	protected vector BuildVehicleForwardTarget(vector sourcePosition, vector angles)
	{
		vector target = sourcePosition;
		float yaw = angles[0] * 0.017453292;
		target[0] = target[0] + Math.Sin(yaw) * TOWN_VEHICLE_ROAD_FORWARD_TARGET_METERS;
		target[2] = target[2] + Math.Cos(yaw) * TOWN_VEHICLE_ROAD_FORWARD_TARGET_METERS;
		return target;
	}

	protected vector BuildVehicleAnglesFromForward(vector sourcePosition, vector forward, vector fallbackAngles)
	{
		float length = Math.Sqrt(forward[0] * forward[0] + forward[2] * forward[2]);
		if (length <= 0.01)
			return HST_WorldPositionService.BuildUprightAnglesFromVector(fallbackAngles);

		vector travelTarget = sourcePosition;
		travelTarget[0] = travelTarget[0] + forward[0] * 10.0;
		travelTarget[2] = travelTarget[2] + forward[2] * 10.0;
		return BuildVehicleAnglesToward(sourcePosition, travelTarget);
	}

	protected vector BuildVehicleAnglesToward(vector sourcePosition, vector targetPosition)
	{
		float dx = targetPosition[0] - sourcePosition[0];
		float dz = targetPosition[2] - sourcePosition[2];
		float yaw;
		float absDx = dx;
		if (absDx < 0)
			absDx = -absDx;
		float absDz = dz;
		if (absDz < 0)
			absDz = -absDz;
		if (absDx > absDz)
		{
			if (dx >= 0)
				yaw = 90;
			else
				yaw = 270;
		}
		else if (dz < 0)
		{
			yaw = 180;
		}

		return HST_WorldPositionService.BuildUprightAngles(yaw);
	}

	protected string SelectCivilianCharacterPrefab(HST_BalanceConfig balance, int index, int seed)
	{
		if (!balance || CountGuidQualifiedCivilianCharacterPrefabs(balance) < MIN_CIVILIAN_CHARACTER_PREFABS)
			return "";

		for (int i = 0; i < balance.m_aCivilianCharacterPrefabs.Count(); i++)
		{
			string prefab = balance.m_aCivilianCharacterPrefabs[ModInt(seed + index + i, balance.m_aCivilianCharacterPrefabs.Count())];
			if (IsGuidQualifiedResource(prefab))
				return prefab;
		}

		return "";
	}

	protected string SelectCivilianVehiclePrefab(HST_BalanceConfig balance, int index, int seed)
	{
		array<string> candidates = {};
		BuildCivilianVehiclePrefabCandidates(balance, candidates);
		if (candidates.Count() > 0)
			return candidates[ModInt((seed / 7) + index * 3, candidates.Count())];

		return "{DA79E34823120087}Prefabs/Vehicles/Wheeled/S105/S105_base.et";
	}

	protected void BuildCivilianVehiclePrefabCandidates(HST_BalanceConfig balance, array<string> candidates)
	{
		if (!candidates)
			return;

		int catalogCount = AppendRuntimeCivilianVehicleCatalogPrefabs(candidates);
		if (catalogCount <= 0 && !m_bWarnedMissingCivilianVehicleCatalog)
		{
			Print(string.Format("h-istasi civilians | CIV vehicle catalog unavailable or empty (%1); using configured/internal civilian vehicle fallback pool", CIVILIAN_VEHICLE_ENTITY_CATALOG), LogLevel.WARNING);
			m_bWarnedMissingCivilianVehicleCatalog = true;
		}

		if (!balance)
			return;

		foreach (string prefab : balance.m_aCivilianVehiclePrefabs)
		{
			AppendUniqueCivilianVehiclePrefab(candidates, prefab);
		}
	}

	protected int AppendRuntimeCivilianVehicleCatalogPrefabs(array<string> candidates)
	{
		if (!candidates)
			return 0;

		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalogManager)
			return 0;

		array<SCR_EntityCatalog> catalogs = {};
		FactionKey key = CIVILIAN_FACTION_KEY;
		if (catalogManager.GetAllFactionEntityCatalogs(catalogs, key) <= 0)
			return 0;

		int before = candidates.Count();
		foreach (SCR_EntityCatalog catalog : catalogs)
		{
			if (!catalog)
				continue;

			array<SCR_EntityCatalogEntry> entries = {};
			catalog.GetEntityList(entries);
			foreach (SCR_EntityCatalogEntry entry : entries)
			{
				if (!entry)
					continue;

				AppendUniqueCivilianVehiclePrefab(candidates, entry.GetPrefab());
			}
		}

		return candidates.Count() - before;
	}

	protected void AppendUniqueCivilianVehiclePrefab(array<string> candidates, string prefab)
	{
		if (!candidates || prefab.IsEmpty() || candidates.Contains(prefab))
			return;
		if (!IsTownGroundVehicleResource(prefab) || IsKnownInvalidVehicleResource(prefab))
			return;

		candidates.Insert(prefab);
	}

	protected bool ShouldSpawnFactionTownVehicles(HST_CampaignPreset preset, HST_ZoneState zone)
	{
		if (!preset || !zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return false;

		return zone.m_sOwnerFactionKey == preset.m_sOccupierFactionKey || zone.m_sOwnerFactionKey == preset.m_sInvaderFactionKey;
	}

	protected string SelectFactionVehiclePrefab(string factionKey, int index)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (faction && faction.m_aVehiclePrefabs.Count() > 0)
		{
			for (int i = 0; i < faction.m_aVehiclePrefabs.Count(); i++)
			{
				string factionVehicle = faction.m_aVehiclePrefabs[ModInt(index + i, faction.m_aVehiclePrefabs.Count())];
				if (IsTownGroundVehicleResource(factionVehicle) && !IsKnownInvalidVehicleResource(factionVehicle))
					return factionVehicle;
			}
		}

		Print(string.Format("h-istasi civilians | no non-aircraft faction vehicle available for %1; skipping owner vehicle", factionKey), LogLevel.WARNING);
		return "";
	}

	protected bool IsTownGroundVehicleResource(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		if (!prefab.Contains("Prefabs/Vehicles/"))
			return false;

		return !IsAircraftVehicleResource(prefab);
	}

	protected bool IsGuidQualifiedResource(string prefab)
	{
		return !prefab.IsEmpty() && prefab.Contains("{") && prefab.Contains("}") && prefab.Contains("Prefabs/");
	}

	protected int CountGuidQualifiedCivilianCharacterPrefabs(HST_BalanceConfig balance)
	{
		if (!balance)
			return 0;

		int validCount;
		foreach (string prefab : balance.m_aCivilianCharacterPrefabs)
		{
			if (IsGuidQualifiedResource(prefab))
				validCount++;
		}

		return validCount;
	}

	protected bool IsKnownInvalidVehicleResource(string prefab)
	{
		return HST_VehicleRootPolicy.IsKnownBrokenVehiclePrefab(prefab);
	}

	protected bool IsAircraftVehicleResource(string prefab)
	{
		return prefab.Contains("Aircraft") || prefab.Contains("Airplane") || prefab.Contains("Plane") || prefab.Contains("Helicopter") || prefab.Contains("Helicopters") || prefab.Contains("/UH") || prefab.Contains("/AH") || prefab.Contains("/Mi") || prefab.Contains("/KA") || prefab.Contains("/Ka");
	}

	protected void ApplyFaction(IEntity entity, string factionKey, string runtimeKind)
	{
		if (!entity)
			return;

		if (IsRuntimeVehicle(runtimeKind))
		{
			ClearRuntimeVehicleFaction(entity);
			return;
		}

		if (factionKey.IsEmpty())
			return;

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionComponent)
			return;

		if (runtimeKind == "CIV_CHARACTER")
		{
			factionComponent.SetAffiliatedFactionByKey("CIV");
			return;
		}

		factionComponent.SetAffiliatedFactionByKey(factionKey);
	}

	protected void ClearRuntimeVehicleFaction(IEntity entity)
	{
		HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(entity);
	}

	protected bool TickRuntimeVehicleHeat(HST_CampaignState state, HST_RuntimeVehicleState vehicle)
	{
		if (!state || !vehicle || vehicle.m_iVehicleHeat <= 0)
			return false;

		if (vehicle.m_iReportedUntilSecond > 0 && state.m_iElapsedSeconds >= vehicle.m_iReportedUntilSecond)
		{
			vehicle.m_iVehicleHeat = 0;
			vehicle.m_bReported = false;
			vehicle.m_iReportedUntilSecond = 0;
			vehicle.m_iLastVehicleHeatChangedSecond = state.m_iElapsedSeconds;
			vehicle.m_sLastReportedReason = "vehicle heat expired";
			return true;
		}

		if (state.m_iElapsedSeconds < vehicle.m_iLastVehicleHeatChangedSecond + VEHICLE_HEAT_DECAY_SECONDS)
			return false;

		vehicle.m_iVehicleHeat = Math.Max(0, vehicle.m_iVehicleHeat - 1);
		vehicle.m_iLastVehicleHeatChangedSecond = state.m_iElapsedSeconds;
		if (vehicle.m_iVehicleHeat == 0)
		{
			vehicle.m_bReported = false;
			vehicle.m_iReportedUntilSecond = 0;
			vehicle.m_sLastReportedReason = "vehicle heat cooled";
		}

		return true;
	}

	protected bool RuntimeVehicleCanProvideCivilianUndercover(HST_RuntimeVehicleState vehicle)
	{
		return HST_VehicleCapabilityPolicy.CanRuntimeVehicleProvideCivilianUndercover(vehicle);
	}

	protected bool IsRuntimeVehicle(string runtimeKind)
	{
		return runtimeKind.Contains("VEHICLE");
	}

	protected void RegisterRuntimeVehicle(HST_CampaignState state, IEntity entity, string zoneId, string prefab, vector position, vector angles, string factionKey, string runtimeKind)
	{
		if (!state || !entity || !IsRuntimeVehicle(runtimeKind) || !HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
			return;

		string vehicleRuntimeId = ResolveRuntimeVehicleId(entity);
		if (vehicleRuntimeId.IsEmpty())
			return;

		state.RemoveRuntimeVehicle(vehicleRuntimeId);
		HST_RuntimeVehicleState vehicle = new HST_RuntimeVehicleState();
		vehicle.m_sVehicleRuntimeId = vehicleRuntimeId;
		vehicle.m_sPrefab = prefab;
		vehicle.m_sDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(prefab);
		vehicle.m_sFactionKey = factionKey;
		vehicle.m_sZoneId = zoneId;
		vehicle.m_sRuntimeKind = runtimeKind;
		vehicle.m_vPosition = position;
		vehicle.m_vAngles = angles;
		vehicle.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		vehicle.m_bCanProvideUndercover = RuntimeVehicleCanProvideCivilianUndercover(vehicle);
		HST_VehicleCapabilityPolicy.NormalizeRuntimeVehicleCoverState(vehicle);
		state.m_aRuntimeVehicles.Insert(vehicle);
	}

	protected void MarkRuntimeVehicleDetached(HST_CampaignState state, IEntity entity)
	{
		HST_RuntimeVehicleState vehicle = ResolveRuntimeVehicleRecord(state, entity);
		if (!vehicle)
			return;

		vehicle.m_bDetached = true;
		vehicle.m_vPosition = entity.GetOrigin();
		vehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(entity.GetYawPitchRoll());
	}

	protected void MarkRuntimeVehicleDeleted(HST_CampaignState state, IEntity entity)
	{
		HST_RuntimeVehicleState vehicle = ResolveRuntimeVehicleRecord(state, entity);
		if (!vehicle)
			return;

		vehicle.m_bDeleted = true;
		vehicle.m_vPosition = entity.GetOrigin();
		vehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(entity.GetYawPitchRoll());
	}

	protected void MarkRuntimeVehicleDeletedBySpawnRecord(HST_CampaignState state, vector spawnPosition, string runtimeKind)
	{
		if (!state || !IsRuntimeVehicle(runtimeKind))
			return;

		foreach (HST_RuntimeVehicleState vehicle : state.m_aRuntimeVehicles)
		{
			if (!vehicle || vehicle.m_bDeleted || vehicle.m_sRuntimeKind != runtimeKind)
				continue;

			if (DistanceSq2D(vehicle.m_vPosition, spawnPosition) > 4.0)
				continue;

			vehicle.m_bDeleted = true;
			return;
		}
	}

	protected HST_RuntimeVehicleState ResolveRuntimeVehicleRecord(HST_CampaignState state, IEntity entity)
	{
		if (!state || !entity)
			return null;

		string vehicleRuntimeId = ResolveRuntimeVehicleId(entity);
		if (!vehicleRuntimeId.IsEmpty())
			return state.FindRuntimeVehicle(vehicleRuntimeId);

		return null;
	}

	protected string ResolveRuntimeVehicleId(IEntity entity)
	{
		if (!entity)
			return "";

		BaseRplComponent rpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("rpl_%1", rpl.Id());

		string prefab = "";
		if (entity.GetPrefabData())
			prefab = entity.GetPrefabData().GetPrefabName();

		return string.Format("local_%1_%2", prefab, entity.GetOrigin());
	}

	protected void WarnMissingCivilianCharacterPool(HST_ZoneState zone, int requestedCount)
	{
		if (!m_bWarnedMissingCivilianCharacterPool)
		{
			Print(string.Format("h-istasi civilians | fewer than %1 GUID-qualified civilian character prefabs configured; skipping civilian character ambience instead of spawning broken CIV group shells", MIN_CIVILIAN_CHARACTER_PREFABS), LogLevel.WARNING);
			m_bWarnedMissingCivilianCharacterPool = true;
		}

		if (zone)
			RecordSpawnFailure(zone.m_sZoneId, "<missing minimum GUID-qualified civilian character pool>", "CIV_CHARACTER", zone.m_vPosition, BuildSpawnAngles(0));
	}

	protected void RecordSpawnFailure(string zoneId, string prefab, string runtimeKind, vector position, vector angles)
	{
		m_iRuntimeSpawnFailureCount++;
		m_sLastRuntimeSpawnFailurePrefab = prefab;
		Print(string.Format("h-istasi civilians | spawn failed | zone %1 | kind %2 | prefab %3 | pos %4 | yaw %5", zoneId, runtimeKind, prefab, position, angles[0]), LogLevel.WARNING);
	}

	protected bool PublishRuntimeDiagnostics(HST_CampaignState state)
	{
		if (!state)
			return false;

		int civilianCharacters;
		int civilianVehicles;
		int militaryVehicles;
		for (int i = 0; i < m_aRuntimeEntityKinds.Count(); i++)
		{
			string runtimeKind = m_aRuntimeEntityKinds[i];
			if (runtimeKind == "CIV_CHARACTER")
				civilianCharacters++;
			else if (runtimeKind == "CIV_VEHICLE" || runtimeKind == CIVILIAN_TRAFFIC_RUNTIME_KIND)
				civilianVehicles++;
			else if (runtimeKind == "MILITARY_VEHICLE")
				militaryVehicles++;
		}

		bool changed;
		if (state.m_iRuntimeCivilianCharacterCount != civilianCharacters)
		{
			state.m_iRuntimeCivilianCharacterCount = civilianCharacters;
			changed = true;
		}

		if (state.m_iRuntimeCivilianVehicleCount != civilianVehicles)
		{
			state.m_iRuntimeCivilianVehicleCount = civilianVehicles;
			changed = true;
		}

		if (state.m_iRuntimeMilitaryVehicleCount != militaryVehicles)
		{
			state.m_iRuntimeMilitaryVehicleCount = militaryVehicles;
			changed = true;
		}

		if (state.m_iRuntimeSpawnFailureCount != m_iRuntimeSpawnFailureCount)
		{
			state.m_iRuntimeSpawnFailureCount = m_iRuntimeSpawnFailureCount;
			changed = true;
		}

		if (state.m_sLastRuntimeSpawnFailurePrefab != m_sLastRuntimeSpawnFailurePrefab)
		{
			state.m_sLastRuntimeSpawnFailurePrefab = m_sLastRuntimeSpawnFailurePrefab;
			changed = true;
		}

		return changed;
	}

	protected bool ShouldDetachFromTownCleanup(IEntity entity, string runtimeKind, vector spawnPosition)
	{
		if (!entity || !IsRuntimeVehicle(runtimeKind))
			return false;

		float distanceSq = DistanceSq2D(entity.GetOrigin(), spawnPosition);
		return distanceSq >= PLAYER_USED_VEHICLE_DETACH_DISTANCE_METERS * PLAYER_USED_VEHICLE_DETACH_DISTANCE_METERS;
	}

	protected float DistanceSq2D(vector first, vector second)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz;
	}
}
