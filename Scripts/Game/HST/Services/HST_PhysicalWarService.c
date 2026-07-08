class HST_ConvoyReadinessStatus
{
	int m_iVehicleAssetCount;
	int m_iActiveVehicleAssetCount;
	int m_iResolvedVehicleAssetCount;
	int m_iSpawnedVehicleCount;
	int m_iCrewGroupCount;
	int m_iAliveCrewGroupCount;
	int m_iAliveCrewCount;
	int m_iDriverAvailableCount;
	int m_iMobileVehicleCount;
	int m_iRouteAssignedCount;
	int m_iWaypointAssignedCount;
	bool m_bReadyToMove;
	bool m_bStaticFallbackAvailable;
	bool m_bPendingGrace;
	string m_sReason;
}

class HST_ConvoyProgressStatus
{
	string m_sKey;
	string m_sMissionInstanceId;
	string m_sAssetId;
	string m_sGroupId;
	vector m_vLastSamplePosition;
	int m_iLastSampleSecond;
	int m_iLastProgressSecond;
	int m_iRouteReissueAttemptCount;
	int m_iRouteSnapAttemptCount;
	int m_iLastRouteReissueSecond;
	int m_iLastRouteSnapSecond;
	int m_iSampleCount;
	float m_fDistanceToDestinationMeters;
	float m_fPreviousDistanceToDestinationMeters;
	float m_fLastMovementMeters;
	float m_fLastDistanceClosedMeters;
	float m_fMaxMovementMeters;
	float m_fMaxDistanceClosedMeters;
	float m_fNearestPlayerDistanceMeters = -1.0;
	bool m_bNoProgress;
	bool m_bHardStuck;
	string m_sLastProgressReason;
	string m_sLastRecoveryResult;
	string m_sPhaseHistory;
}

class HST_ConvoyCompletionStatus
{
	int m_iTotalVehicleAssets;
	int m_iRequiredCrewGroups;
	int m_iMissingCrewGroups;
	int m_iPendingCrewGroups;
	int m_iEliminatedCrewGroups;
	int m_iLivingCrew;
	int m_iDestroyedVehicles;
	int m_iCapturedVehicles;
	int m_iActiveVehicles;
	bool m_bAllCrewsEliminated;
	bool m_bAnyLiveCrewArrived;
	bool m_bCanComplete;
	bool m_bMustFail;
	string m_sReason;
}

class HST_PhysicalWarService
{
	static const int MAX_ACTIVE_INFANTRY_PER_ZONE = 6;
	static const int MAX_ACTIVE_VEHICLES_PER_ZONE = 1;
	static const int QRF_ATTACK_RESOURCE_COST = 15;
	static const int QRF_SUPPORT_RESOURCE_COST = 5;
	static const int QRF_ETA_SECONDS = 180;
	static const int QRF_INBOUND_SPAWN_SECONDS = 30;
	static const int QRF_COOLDOWN_SECONDS = 900;
	static const int ROUTE_STATE_UPDATE_SECONDS = 5;
	static const float HQ_SAFE_RADIUS_METERS = 900;
	static const float QRF_MIN_STANDOFF_METERS = 220.0;
	static const float QRF_EXTRA_STANDOFF_METERS = 140.0;
	static const float QRF_MAX_STANDOFF_METERS = 650.0;
	static const string MISSION_CONVOY_GROUP_PREFIX = "mission_convoy_";
	static const string MISSION_CONVOY_VEHICLE_ROLE = "convoy_vehicle";
	static const string MISSION_CONVOY_PAYLOAD_ROLE = "convoy_payload";
	static const string MISSION_CONVOY_CAPTIVE_ROLE = "convoy_captive";
	static const string MISSION_CONVOY_PRIMITIVE = "convoy_intercept";
	static const string MISSION_CONVOY_STAGING = "convoy_staging";
	static const string MISSION_CONVOY_MOVING = "convoy_moving";
	static const string MISSION_CONVOY_CONTACT = "convoy_contact";
	static const string MISSION_CONVOY_ELIMINATED = "convoy_eliminated";
	static const string MISSION_CONVOY_ARRIVED = "convoy_arrived";
	static const string MISSION_CONVOY_FAILED = "failed";
	static const string MISSION_PRIMITIVE_CLEAR_AREA = "clear_area";
	static const string MISSION_PRIMITIVE_HOLD_AREA = "hold_area";
	static const string CONVOY_COMPLETE_EVENT_KEY = "convoy_complete";
	static const string CONVOY_FAIL_EVENT_KEY = "convoy_failed";
	static const string CONVOY_MOVE_EVENT_PENDING = "convoy_moving_pending";
	static const string CONVOY_MOVE_EVENT_SENT = "convoy_moving_sent";
	static const string FIA_CAMPAIGN_FACTION_CONFIG = "Configs/Factions/FIA_Campaign.conf";
	static const string US_CAMPAIGN_FACTION_CONFIG = "{ADFDBDA163950168}Configs/Factions/US_Campaign.conf";
	static const string USSR_CAMPAIGN_FACTION_CONFIG = "{15B582F8FA0B0940}Configs/Factions/USSR_Campaign.conf";
	static const float CONVOY_CONTACT_RADIUS_METERS = 120.0;
	static const float CONVOY_DESTINATION_RADIUS_METERS = 50.0;
	static const int CONVOY_PROGRESS_SYNC_SECONDS = 5;
	static const int CONVOY_MARKER_REFRESH_SECONDS = 30;
	static const float CONVOY_PROGRESS_THRESHOLD_METERS = 8.0;
	static const int CONVOY_ROUTE_REISSUE_THRESHOLD_SECONDS = 45;
	static const int CONVOY_HARD_STUCK_THRESHOLD_SECONDS = 120;
	static const float CONVOY_UNSTUCK_MIN_PLAYER_DISTANCE_METERS = 300.0;
	static const float CONVOY_ROUTE_SNAP_SEARCH_RADIUS_METERS = 180.0;
	static const float CONVOY_ROUTE_SNAP_MIN_DESTINATION_DISTANCE_METERS = 250.0;
	static const float CONVOY_ROUTE_SNAP_MAX_ADVANCE_METERS = 160.0;
	static const int CONVOY_CREW_POPULATION_GRACE_SECONDS = 20;
	static const int CONVOY_CREW_SEATING_GRACE_SECONDS = 45;
	static const int CONVOY_READINESS_GRACE_SECONDS = 60;
	static const int ACTIVE_GROUP_AGENT_POPULATION_RETRY_MS = 1000;
	static const int ACTIVE_GROUP_AGENT_POPULATION_MAX_ATTEMPTS = 8;
	static const int ACTIVE_GROUP_AGENT_POPULATION_FORCE_FALLBACK_ATTEMPT = 3;
	static const int ACTIVE_GROUP_AGENT_POPULATION_SLOT_PRIMARY_ATTEMPT = 4;
	static const int ACTIVE_GROUP_AGENT_POPULATION_DIRECT_FALLBACK_ATTEMPT = 4;
	static const int ACTIVE_GROUP_LIVE_COUNT_GRACE_SECONDS = 8;
	static const int ACTIVE_GROUP_AI_WORLD_MIN_LIMIT = 512;
	static const int ACTIVE_GROUP_AI_WORLD_REQUIRED_HEADROOM = 32;
	static const int CONVOY_RUNTIME_WAYPOINT_MIN_COUNT = 3;
	static const int CONVOY_RUNTIME_WAYPOINT_MAX_COUNT = 5;
	static const float EXPIRED_CONVOY_PLAYER_RENDER_BUBBLE_METERS = 1800.0;
	static const float CONVOY_VEHICLE_SPAWN_LIFT_METERS = 1.25;
	static const float CONVOY_VEHICLE_SPAWN_CLEARANCE_METERS = 18.0;
	static const float CONVOY_PHYSICAL_ROAD_SEARCH_RADIUS_METERS = 40.0;
	static const float CONVOY_ROUTE_WAYPOINT_ROAD_SEARCH_RADIUS_METERS = 250.0;
	static const float CONVOY_ROAD_REPORT_SEARCH_RADIUS_METERS = 6.0;
	static const float PLAYER_USED_ACTIVE_VEHICLE_DETACH_DISTANCE_METERS = 35.0;
	static const string PERSISTENCE_SMOKE_PREFIX = "hst_smoke";
	static const string CAMPAIGN_DEBUG_PREFIX_ROOT = "hst_debug_";
	static const string CAMPAIGN_DEBUG_ENTITY_TAG = "HST_CAMPAIGN_DEBUG";
	static const string ACTIVE_GROUP_SPAWN_MODE_GROUP = "group";
	static const string ACTIVE_GROUP_SPAWN_MODE_GROUP_RETRY = "group_spawn_retry";
	static const string ACTIVE_GROUP_SPAWN_MODE_GROUP_NATIVE_IMMEDIATE = "group_native_immediate";
	static const string ACTIVE_GROUP_SPAWN_MODE_GROUP_SLOT_PRIMARY = "group_slot_primary";
	static const string ACTIVE_GROUP_SPAWN_MODE_DIRECT_INFANTRY_FALLBACK = "direct_infantry_fallback";
	static const string ACTIVE_GROUP_SPAWN_MODE_AIWORLD_BUDGET_DEFERRED = "aiworld_budget_deferred";
	static const string ACTIVE_GROUP_RUNTIME_STATUS_AIWORLD_BUDGET_DEFERRED = "spawn_deferred_aiworld_budget";
	static const string DIRECT_INFANTRY_GROUP_PREFAB = "{6985327711303910}Prefabs/Groups/HST/HST_RuntimeEmptyGroup.et";
	static const string DIRECT_INFANTRY_GROUP_PREFAB_US = "{2E3755F24A57D1A0}Prefabs/Groups/HST/HST_RuntimeEmptyGroup_US.et";
	static const string DIRECT_INFANTRY_GROUP_PREFAB_USSR = "{94AA122B0CFB7E40}Prefabs/Groups/HST/HST_RuntimeEmptyGroup_USSR.et";
	static const string CAMPAIGN_DEBUG_TEMP_ENTITY_PREFAB = "{FBA8DC8FDA0E770D}Prefabs/AI/Waypoints/AIWaypoint_Patrol_Hierarchy.et";
	static const string CAMPAIGN_DEBUG_COMBAT_WAYPOINT_PREFAB = "{B3E7B8DC2BAB8ACC}Prefabs/AI/Waypoints/AIWaypoint_SearchAndDestroy.et";
	static const string ACTIVE_GROUP_ROUTE_WAYPOINT_PREFAB = "{FBA8DC8FDA0E770D}Prefabs/AI/Waypoints/AIWaypoint_Patrol_Hierarchy.et";
	static const string ACTIVE_GROUP_ROUTE_SWEEP_WAYPOINT_PREFAB = "{B3E7B8DC2BAB8ACC}Prefabs/AI/Waypoints/AIWaypoint_SearchAndDestroy.et";
	static const int CAMPAIGN_DEBUG_COMBAT_PROBE_SAMPLE_SECONDS = 45;
	static const int CAMPAIGN_DEBUG_COMBAT_PROBE_INFANTRY_COUNT = 4;
	static const float CAMPAIGN_DEBUG_COMBAT_PROBE_PLAYER_OFFSET_METERS = 90.0;
	static const float CAMPAIGN_DEBUG_COMBAT_PROBE_SEPARATION_METERS = 36.0;
	static const float CAMPAIGN_DEBUG_COMBAT_PROBE_CONTACT_METERS = 70.0;
	static const float CAMPAIGN_DEBUG_COMBAT_WAYPOINT_RADIUS_METERS = 18.0;
	static const float ACTIVE_GROUP_ROUTE_WAYPOINT_RADIUS_METERS = 35.0;
	static const float ACTIVE_GROUP_ROUTE_SWEEP_WAYPOINT_RADIUS_METERS = 55.0;

	protected ref array<string> m_aRuntimeGroupIds = {};
	protected ref array<IEntity> m_aRuntimeGroupEntities = {};
	protected ref array<string> m_aRuntimeGroupWaypointIds = {};
	protected ref array<IEntity> m_aRuntimeGroupWaypointEntities = {};
	protected ref array<string> m_aPendingPopulationGroupIds = {};
	protected ref array<string> m_aPendingPopulationRequestedStatuses = {};
	protected ref array<ref HST_ActiveGroupState> m_aPendingPopulationActiveGroups = {};
	protected ref array<ref HST_CampaignState> m_aPendingPopulationStates = {};
	protected ref array<string> m_aRuntimeVehicleGroupIds = {};
	protected ref array<IEntity> m_aRuntimeVehicleEntities = {};
	protected ref array<ref HST_ConvoyProgressStatus> m_aConvoyProgressStatuses = {};
	protected ref array<string> m_aRestoredMissionConvoyRebuildGroupIds = {};
	protected ref array<string> m_aVehicleSpawnBlockedZoneIds = {};
	protected ref array<string> m_aVehicleSpawnBlockedReasons = {};
	protected ref HST_ConvoyVehicleControlAdapter m_ConvoyVehicleControl;
	protected bool m_bMarkerRefreshNeeded;
	protected bool m_bDebugLoggingEnabled;
	protected bool m_bCampaignDebugCombatProbeActive;
	protected string m_sCampaignDebugCombatProbeId;
	protected string m_sCampaignDebugCombatProbeFriendlyGroupId;
	protected string m_sCampaignDebugCombatProbeEnemyGroupId;
	protected string m_sCampaignDebugCombatProbeFriendlyFaction;
	protected string m_sCampaignDebugCombatProbeEnemyFaction;
	protected string m_sCampaignDebugCombatProbeZoneId;
	protected int m_iCampaignDebugCombatProbeStartSecond;
	protected int m_iCampaignDebugCombatProbeLastSampleSecond = -1;
	protected int m_iCampaignDebugCombatProbeFriendlyStartAlive = -1;
	protected int m_iCampaignDebugCombatProbeEnemyStartAlive = -1;
	protected int m_iCampaignDebugCombatProbeFriendlyEndAlive = -1;
	protected int m_iCampaignDebugCombatProbeEnemyEndAlive = -1;
	protected int m_iCampaignDebugCombatProbeFriendlyMinAlive = -1;
	protected int m_iCampaignDebugCombatProbeEnemyMinAlive = -1;
	protected int m_iCampaignDebugCombatProbeSampleCount;
	protected int m_iCampaignDebugCombatProbeHistoryCount;
	protected int m_iCampaignDebugCombatProbeWaypointCount;
	protected float m_fCampaignDebugCombatProbeLastDistance = -1.0;
	protected vector m_vCampaignDebugCombatProbeCenter;
	protected vector m_vCampaignDebugCombatProbeFriendlyPosition;
	protected vector m_vCampaignDebugCombatProbeEnemyPosition;
	protected string m_sCampaignDebugCombatProbeHistory;
	protected string m_sCampaignDebugCombatProbeLastObserved;
	protected string m_sCampaignDebugCombatProbeStartResult;
	protected bool m_bCampaignDebugCombatProbeFriendlyPopulationResolved;
	protected bool m_bCampaignDebugCombatProbeEnemyPopulationResolved;
	protected string m_sCampaignDebugCombatProbeFriendlyPopulationEvidence;
	protected string m_sCampaignDebugCombatProbeEnemyPopulationEvidence;
	protected ref array<IEntity> m_aCampaignDebugCombatProbeWaypointEntities = {};

	void SetDebugLoggingEnabled(bool enabled)
	{
		m_bDebugLoggingEnabled = enabled;
	}

	protected void ApplyCampaignDebugEntityName(IEntity entity, string label, string sourceId)
	{
		if (!entity || sourceId.IsEmpty() || !sourceId.Contains(CAMPAIGN_DEBUG_PREFIX_ROOT))
			return;

		entity.SetName(CAMPAIGN_DEBUG_ENTITY_TAG + "_" + SanitizeCampaignDebugEntityToken(label) + "_" + SanitizeCampaignDebugEntityToken(sourceId));
	}

	protected string SanitizeCampaignDebugEntityToken(string value)
	{
		string safe = value;
		safe.Replace("/", "_");
		safe.Replace(":", "_");
		safe.Replace(" ", "_");
		safe.Replace("@", "_");
		safe.Replace(".", "_");
		if (safe.Length() > 160)
			return safe.Substring(0, 160);

		return safe;
	}

	bool UpdateRoutedActiveGroupsNow(HST_CampaignState state, HST_CampaignPreset preset = null, bool forceRouteUpdate = false)
	{
		if (!state)
			return false;

		bool missionCleanupChanged = CleanupInactiveMissionOwnedActiveGroups(state);
		bool runtimeEntityChanged = EnsureRuntimeGroupEntities(state, preset);
		bool survivorChanged = UpdateRuntimeGroupSurvivors(state);
		bool routeChanged = UpdateActiveGroupRoutes(state, forceRouteUpdate);
		bool combatProbeChanged = SampleCampaignDebugPhysicalCombatProbe(state);
		return missionCleanupChanged || runtimeEntityChanged || survivorChanged || routeChanged || combatProbeChanged;
	}

	int CountRuntimeGroupHandlesForMission(HST_CampaignState state, string missionInstanceId)
	{
		if (!state || missionInstanceId.IsEmpty())
			return 0;

		int count;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sMissionInstanceId != missionInstanceId)
				continue;
			if (GetRuntimeGroupEntity(activeGroup.m_sGroupId))
				count++;
		}

		return count;
	}

	bool UpdateZoneActivation(HST_CampaignState state, HST_BalanceConfig balance, HST_CampaignPreset preset = null, HST_EnemyDirectorService enemyDirector = null, HST_ZoneCompositionService compositions = null)
	{
		if (!state || !balance)
			return false;
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return false;

		bool routedGroupChanged = UpdateRoutedActiveGroupsNow(state, preset);

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return routedGroupChanged;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		bool changed;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			bool forceMissionZone = ShouldForceMissionZoneActive(state, zone);
			bool shouldBeActive = !IsZoneInsideHQSafeArea(state, zone) && (IsAnyLivingPlayerNearZone(playerManager, playerIds, zone, balance) || forceMissionZone);
			if (zone.m_bActive == shouldBeActive)
			{
				if (shouldBeActive)
					changed = ActivateZone(state, zone, preset, compositions, true) || changed;

				continue;
			}

			zone.m_bActive = shouldBeActive;
			if (shouldBeActive)
				changed = ActivateZone(state, zone, preset, compositions, true) || changed;
			else
				changed = DeactivateZone(state, zone, compositions) || changed;

			changed = true;
			m_bMarkerRefreshNeeded = true;
			DebugLog(string.Format("zone %1 physical activation = %2", zone.m_sZoneId, shouldBeActive));
		}

		if (UpdateQRF(state, preset, enemyDirector))
			changed = true;

		return changed || routedGroupChanged;
	}

	bool EnsureMissionTargetZoneActive(HST_CampaignState state, string zoneId, HST_CampaignPreset preset = null, HST_ZoneCompositionService compositions = null)
	{
		if (!state || zoneId.IsEmpty())
			return false;
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone || IsZoneInsideHQSafeArea(state, zone))
			return false;

		bool changed;
		if (!zone.m_bActive)
		{
			zone.m_bActive = true;
			changed = true;
		}

		changed = ActivateZone(state, zone, preset, compositions, true) || changed;
		if (changed)
		{
			m_bMarkerRefreshNeeded = true;
			DebugLog(string.Format("mission target zone %1 forced physical activation", zone.m_sZoneId));
		}

		return changed;
	}

	bool CleanupRuntimeGroupEntityForDebug(string groupId)
	{
		if (groupId.IsEmpty())
			return false;

		bool existed = GetRuntimeGroupEntity(groupId) != null;
		DeleteRuntimeGroupEntity(groupId);
		return existed;
	}

	protected bool ShouldForceMissionZoneActive(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return false;

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sTargetZoneId != zone.m_sZoneId)
				continue;
			if (ShouldForceMissionTargetZonePhysical(mission))
				return true;
		}

		return false;
	}

	protected bool ShouldForceMissionTargetZonePhysical(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sRuntimePrimitive.IsEmpty())
			return false;
		if (mission.m_sRuntimePrimitive == "abstract_fallback")
			return false;
		if (mission.m_sRuntimePrimitive == MISSION_CONVOY_PRIMITIVE)
			return false;

		return true;
	}

	bool ConsumeMarkerRefreshNeeded()
	{
		bool result = m_bMarkerRefreshNeeded;
		m_bMarkerRefreshNeeded = false;
		return result;
	}

	string BuildConvoyRuntimeReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi convoy runtime | state not ready";

		int convoyMissions;
		int convoyGroups;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (mission && mission.m_sRuntimePrimitive == MISSION_CONVOY_PRIMITIVE && !IsPersistenceSmokeMission(mission))
				convoyMissions++;
		}
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (IsMissionConvoyGroup(activeGroup) && !activeGroup.m_sGroupId.Contains(PERSISTENCE_SMOKE_PREFIX))
				convoyGroups++;
		}

		string report = string.Format("h-istasi convoy runtime | missions %1 | groups %2 | crew entities %3 | vehicle entities %4", convoyMissions, convoyGroups, m_aRuntimeGroupIds.Count(), m_aRuntimeVehicleGroupIds.Count());
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;

			report = report + BuildConvoyRuntimeReport(state, mission);
		}

		return report;
	}

	string BuildGroundVehicleCandidateReport()
	{
		string report = "h-istasi convoy ground vehicle candidates";
		report = report + BuildGroundVehicleCandidateFactionReport("FIA");
		report = report + BuildGroundVehicleCandidateFactionReport("US");
		report = report + BuildGroundVehicleCandidateFactionReport("USSR");
		return report;
	}

	string BuildConvoyRuntimeReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state)
			return "\nconvoy mission | state not ready";
		if (!mission)
			return "\nconvoy mission | mission missing";
		if (mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
			return string.Format("\nconvoy mission | instance %1 | mission %2 | not convoy", ReportText(mission.m_sInstanceId), ReportText(mission.m_sMissionId));

		return BuildMissionConvoyRuntimeReport(state, mission);
	}

	HST_CampaignDebugCaseResult BuildCampaignDebugConvoyPhysicalProbe(HST_CampaignState state, HST_ActiveMissionState mission, bool physicalBlocked)
	{
		HST_CampaignDebugCaseResult probe = CreateConvoyDebugProbeCase(state, mission);
		if (physicalBlocked)
		{
			AddConvoyDebugProbeAssertion(probe, "convoy.physical.prerequisite.player", "controlled player entity available", "missing", "BLOCKED", "bootstrap marked physical runtime tests blocked");
			FinalizeConvoyDebugProbeCase(state, probe);
			return probe;
		}

		if (!state || !mission)
		{
			AddConvoyDebugProbeAssertion(probe, "convoy.physical.prerequisite.state", "campaign state and mission exist", "missing", "BLOCKED", "state or mission missing for convoy physical probe");
			FinalizeConvoyDebugProbeCase(state, probe);
			return probe;
		}

		if (mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
		{
			AddConvoyDebugProbeAssertion(probe, "convoy.physical.primitive", "mission primitive convoy_intercept", ReportText(mission.m_sRuntimePrimitive), "SKIPPED", "mission is not a convoy primitive");
			FinalizeConvoyDebugProbeCase(state, probe);
			return probe;
		}

		int populationAttempted;
		int populationResolved;
		int populationUnresolved;
		bool populationSeatingChanged;
		bool populationWaypointChanged;
		string populationEvidence = ResolveCampaignDebugMissionConvoyPopulation(state, mission, populationAttempted, populationResolved, populationUnresolved, populationSeatingChanged, populationWaypointChanged);
		HST_ConvoyReadinessStatus readiness = BuildMissionConvoyReadinessStatus(state, mission);
		AddConvoyDebugProbeMetric(probe, "convoy.assets.vehicle_count", string.Format("%1", readiness.m_iVehicleAssetCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.assets.active_vehicle_count", string.Format("%1", readiness.m_iActiveVehicleAssetCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.assets.resolved_vehicle_count", string.Format("%1", readiness.m_iResolvedVehicleAssetCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.vehicles.spawned", string.Format("%1", readiness.m_iSpawnedVehicleCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.crew.groups", string.Format("%1", readiness.m_iCrewGroupCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.crew.alive", string.Format("%1", readiness.m_iAliveCrewCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.drivers.available", string.Format("%1", readiness.m_iDriverAvailableCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.vehicles.mobile", string.Format("%1", readiness.m_iMobileVehicleCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.routes.assigned", string.Format("%1", readiness.m_iRouteAssignedCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.waypoints.assigned", string.Format("%1", readiness.m_iWaypointAssignedCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.crew.population_attempted", string.Format("%1", populationAttempted), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.crew.population_resolved", string.Format("%1", populationResolved), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.crew.population_unresolved", string.Format("%1", populationUnresolved), "count");

		string pendingGraceStatus = "FAIL";
		if (readiness.m_bPendingGrace)
			pendingGraceStatus = "WARN";
		string populationActual = string.Format("attempted %1 | resolved %2 | unresolved %3 | seating %4 | waypoints %5", populationAttempted, populationResolved, populationUnresolved, ReportBool(populationSeatingChanged), ReportBool(populationWaypointChanged));
		populationActual = populationActual + string.Format(" | evidence %1", ReportText(populationEvidence));
		int activeVehicleAssets = readiness.m_iActiveVehicleAssetCount;
		string assetCountActual = string.Format("planned %1 | active %2 | resolved %3", readiness.m_iVehicleAssetCount, activeVehicleAssets, readiness.m_iResolvedVehicleAssetCount);
		AddConvoyDebugProbeAssertion(probe, "convoy.crew.population", "pending convoy crew groups have durable runtime agents before readiness is judged", populationActual, ConvoyDebugStatus(populationUnresolved == 0), "convoy crew population remained pending or unresolved before readiness assertions", "", mission.m_sInstanceId);
		AddConvoyDebugProbeAssertion(probe, "convoy.assets.vehicle_count", "planned vehicle assets >= 3", assetCountActual, ConvoyDebugStatus(readiness.m_iVehicleAssetCount >= 3), "convoy has fewer than three planned vehicle assets");
		AddConvoyDebugProbeAssertion(probe, "convoy.assets.active_vehicle_count", "active unresolved vehicle assets >= 3 before movement proof", assetCountActual, ConvoyDebugStatus(activeVehicleAssets >= 3), "convoy has fewer than three active unresolved vehicle assets");
		AddConvoyDebugProbeAssertion(probe, "convoy.vehicle_entities.spawned", "spawned vehicle entities == active vehicle asset count", string.Format("%1/%2 active | planned %3 | resolved %4", readiness.m_iSpawnedVehicleCount, activeVehicleAssets, readiness.m_iVehicleAssetCount, readiness.m_iResolvedVehicleAssetCount), ConvoyDebugStatus(activeVehicleAssets >= 3 && readiness.m_iSpawnedVehicleCount >= activeVehicleAssets), "not every active convoy vehicle asset has a runtime vehicle entity");
		AddConvoyDebugProbeAssertion(probe, "convoy.crew.groups", "crew group count >= active vehicle count", string.Format("%1/%2 active | pending grace %3", readiness.m_iCrewGroupCount, activeVehicleAssets, readiness.m_bPendingGrace), ConvoyDebugStatus(activeVehicleAssets >= 3 && readiness.m_iCrewGroupCount >= activeVehicleAssets, pendingGraceStatus), "not every active convoy vehicle has a crew group");
		AddConvoyDebugProbeAssertion(probe, "convoy.crew.alive", "alive crew count > 0", string.Format("%1 | pending grace %2", readiness.m_iAliveCrewCount, readiness.m_bPendingGrace), ConvoyDebugStatus(readiness.m_iAliveCrewCount > 0, pendingGraceStatus), "convoy crew groups have no living crew");
		AddConvoyDebugProbeAssertion(probe, "convoy.crew.driver_seated", "driver available for every moving vehicle", string.Format("%1/%2 active | pending grace %3", readiness.m_iDriverAvailableCount, activeVehicleAssets, readiness.m_bPendingGrace), ConvoyDebugStatus(activeVehicleAssets >= 3 && readiness.m_iDriverAvailableCount >= activeVehicleAssets, pendingGraceStatus), "not every convoy vehicle has a seated living driver");
		AddConvoyDebugProbeAssertion(probe, "convoy.vehicle_entities.mobile", "mobile vehicle count == active vehicle asset count", string.Format("%1/%2 active", readiness.m_iMobileVehicleCount, activeVehicleAssets), ConvoyDebugStatus(activeVehicleAssets >= 3 && readiness.m_iMobileVehicleCount >= activeVehicleAssets), "not every active convoy vehicle is mobile");
		AddConvoyDebugProbeAssertion(probe, "convoy.route.assigned", "route assigned count == active vehicle asset count", string.Format("%1/%2 active | pending grace %3", readiness.m_iRouteAssignedCount, activeVehicleAssets, readiness.m_bPendingGrace), ConvoyDebugStatus(activeVehicleAssets >= 3 && readiness.m_iRouteAssignedCount >= activeVehicleAssets, pendingGraceStatus), "not every convoy group has the mission route assigned");
		AddConvoyDebugProbeAssertion(probe, "convoy.route.waypoints", "waypoint assigned count == active vehicle asset count", string.Format("%1/%2 active | pending grace %3", readiness.m_iWaypointAssignedCount, activeVehicleAssets, readiness.m_bPendingGrace), ConvoyDebugStatus(activeVehicleAssets >= 3 && readiness.m_iWaypointAssignedCount >= activeVehicleAssets, pendingGraceStatus), "not every convoy group has runtime waypoints assigned");
		AddConvoyDebugProbeAssertion(probe, "convoy.readiness.ready_to_move", "convoy ready-to-move state true", readiness.m_sReason, ConvoyDebugStatus(readiness.m_bReadyToMove, "WARN"), "convoy readiness is not complete yet");

		HST_GeneratedRouteState route = ResolveMissionConvoyRoute(state, mission);
		AddConvoyDebugProbeAssertion(probe, "convoy.route.template", "generated route exists and has >= 3 waypoints", BuildConvoyDebugRouteActual(route), ConvoyDebugStatus(route && route.m_iWaypointCount >= CONVOY_RUNTIME_WAYPOINT_MIN_COUNT), "convoy route template is missing or too short");

		int assetIndex;
		int progressSampledCount;
		int repeatedProgressSampledCount;
		int distanceDecreaseSampledCount;
		int hardStuckCount;
		int noProgressCount;
		int missingProgressCount;
		int movementWindowCount;
		int routeTimeoutCount;
		int pendingStallWindowCount;
		int recoveryAttemptCount;
		int factionCheckedCount;
		int factionMismatchGroupCount;
		int factionUncheckableCount;
		int maxRouteTimeoutSeconds;
		float bestMovementMeters;
		float bestDistanceClosedMeters;
		string convoyPhaseHistory;
		string routeTimeoutEvidence;
		string factionMismatchEvidence;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, assetIndex);
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			IEntity vehicleEntity = GetRuntimeVehicleEntity(groupId);
			IEntity crewEntity = GetRuntimeCrewGroupEntity(groupId);
			string crewEntityMissingStatus = "FAIL";
			if (readiness.m_bPendingGrace || IsConvoyCrewControlPending(state, activeGroup))
				crewEntityMissingStatus = "WARN";
			AddConvoyDebugProbeAssertion(probe, "convoy.vehicle_entity." + asset.m_sAssetId, "vehicle entity exists for asset", string.Format("group %1 | vehicle %2", groupId, vehicleEntity != null), ConvoyDebugStatus(vehicleEntity != null), "convoy vehicle entity missing", groupId, mission.m_sInstanceId);
			AddConvoyDebugProbeAssertion(probe, "convoy.crew_entity." + asset.m_sAssetId, "crew entity exists for asset", string.Format("group %1 | crew %2 | pending grace %3", groupId, crewEntity != null, readiness.m_bPendingGrace), ConvoyDebugStatus(crewEntity != null, crewEntityMissingStatus), "convoy crew entity missing", groupId, mission.m_sInstanceId);
			if (activeGroup)
			{
				string groupWaypointStatus = "FAIL";
				if (readiness.m_bPendingGrace || activeGroup.m_sSpawnFallbackMode == "convoy_seating_pending")
					groupWaypointStatus = "WARN";
				AddConvoyDebugProbeAssertion(probe, "convoy.group_waypoints." + asset.m_sAssetId, "assigned waypoint count >= 2", string.Format("%1 | mode %2 | reason %3", activeGroup.m_iAssignedWaypointCount, ReportText(activeGroup.m_sSpawnFallbackMode), ReportText(activeGroup.m_sSpawnFailureReason)), ConvoyDebugStatus(activeGroup.m_iAssignedWaypointCount >= 2, groupWaypointStatus), "convoy group has too few assigned waypoints", groupId, mission.m_sInstanceId);
				string vehicleUsageStatus = "FAIL";
				if (readiness.m_bPendingGrace || activeGroup.m_sSpawnFallbackMode == "convoy_seating_pending")
					vehicleUsageStatus = "WARN";
				bool vehicleUsageRegistered = activeGroup.m_iAssignedWaypointCount >= 2 && activeGroup.m_sSpawnFailureReason.Contains("vehicle usage registered");
				AddConvoyDebugProbeAssertion(probe, "convoy.vehicle_usage." + asset.m_sAssetId, "AI vehicle usage registered for group movement", string.Format("waypoints %1 | mode %2 | reason %3", activeGroup.m_iAssignedWaypointCount, ReportText(activeGroup.m_sSpawnFallbackMode), ReportText(activeGroup.m_sSpawnFailureReason)), ConvoyDebugStatus(vehicleUsageRegistered, vehicleUsageStatus), "convoy vehicle was not registered with the AI vehicle usage layer", groupId, mission.m_sInstanceId);
				EnsureActiveGroupRuntimeFaction(activeGroup, "convoy physical probe");
				string factionSample;
				int factionMismatches = CountActiveGroupRuntimeFactionMismatches(activeGroup, factionSample);
				bool factionCheckable = crewEntity != null || vehicleEntity != null;
				int liveFactionMembers = CountRuntimeGroupControlledEntities(activeGroup.m_sGroupId);
				bool liveMemberProof = activeGroup.m_iInfantryCount <= 0 || liveFactionMembers > 0;
				string factionStatus = "FAIL";
				if (factionCheckable && factionMismatches <= 0 && liveMemberProof)
				{
					factionStatus = "PASS";
					factionCheckedCount++;
				}
				else if (factionCheckable && !liveMemberProof && (readiness.m_bPendingGrace || IsConvoyCrewControlPending(state, activeGroup)))
				{
					factionStatus = "WARN";
					factionUncheckableCount++;
				}
				else if (!factionCheckable)
				{
					factionStatus = crewEntityMissingStatus;
					factionUncheckableCount++;
				}
				else
				{
					factionMismatchGroupCount++;
					factionMismatchEvidence = AppendConvoyDebugEvidence(factionMismatchEvidence, string.Format("%1 expected %2 mismatches %3 liveMembers %4 sample %5", groupId, activeGroup.m_sFactionKey, factionMismatches, liveFactionMembers, ReportText(factionSample)));
				}
				AddConvoyDebugProbeAssertion(probe, "convoy.faction." + asset.m_sAssetId, "crew runtime faction matches the active-group faction and vehicle is unclaimed", BuildActiveGroupRuntimeFactionActual(activeGroup, factionMismatches, factionSample, crewEntity != null, vehicleEntity != null), factionStatus, "convoy crew faction mismatch or vehicle still claimed", groupId, mission.m_sInstanceId);
			}
			else
			{
				factionUncheckableCount++;
				AddConvoyDebugProbeAssertion(probe, "convoy.group_waypoints." + asset.m_sAssetId, "assigned waypoint count >= 2", "group missing", "FAIL", "convoy active group missing", groupId, mission.m_sInstanceId);
			}

			vector vehiclePosition = ResolveMissionConvoyVehiclePosition(asset, groupId);
			HST_ConvoyProgressStatus progress = FindConvoyProgressStatus(mission, asset, groupId);
			if (!progress)
			{
				missingProgressCount++;
				AddConvoyDebugProbeAssertion(probe, "convoy.movement.progress." + asset.m_sAssetId, "progress sampled", string.Format("distance %1m | result not sampled", Math.Round(ResolveMissionConvoyDistanceToDestinationMeters(mission, asset, vehiclePosition))), "WARN", "convoy movement progress has not been sampled yet", groupId, mission.m_sInstanceId);
			}
			else
			{
				progressSampledCount++;
				bool progressRepeatedSamples = progress.m_iSampleCount >= 2;
				bool progressDistanceClosed = progress.m_fMaxDistanceClosedMeters >= 25.0;
				bool progressRecoveryAttempted = progress.m_iRouteReissueAttemptCount > 0 || progress.m_iRouteSnapAttemptCount > 0;
				bool progressContactOrTerminal = mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT || mission.m_sRuntimePhase == MISSION_CONVOY_ARRIVED || mission.m_sRuntimePhase == MISSION_CONVOY_ELIMINATED;
				bool progressAnyMovement = progress.m_fMaxMovementMeters > 0.5 || progress.m_fMaxDistanceClosedMeters > 0.5;
				int progressRouteWindowSeconds = 0;
				if (progress.m_iSampleCount > 1)
					progressRouteWindowSeconds = (progress.m_iSampleCount - 1) * CONVOY_PROGRESS_SYNC_SECONDS;
				bool progressStallWindowComplete = progressRouteWindowSeconds >= CONVOY_ROUTE_REISSUE_THRESHOLD_SECONDS;
				bool progressRouteTimedOut = progressRepeatedSamples && progressStallWindowComplete && !progressAnyMovement && !progressRecoveryAttempted && !progressContactOrTerminal;
				bool progressWindowProved = progressDistanceClosed || progressRecoveryAttempted || progressContactOrTerminal;
				bool progressWindowPending = progressRepeatedSamples && !progressWindowProved && !progressRouteTimedOut;

				if (progressRepeatedSamples)
				{
					movementWindowCount++;
					repeatedProgressSampledCount++;
				}
				if (progressDistanceClosed)
					distanceDecreaseSampledCount++;
				if (progressRecoveryAttempted)
					recoveryAttemptCount++;
				if (progressWindowPending)
					pendingStallWindowCount++;
				if (progressRouteTimedOut)
				{
					routeTimeoutCount++;
					routeTimeoutEvidence = AppendConvoyDebugEvidence(routeTimeoutEvidence, string.Format("%1 timeout %2s | samples %3 | reason %4 | recovery %5", asset.m_sAssetId, progressRouteWindowSeconds, progress.m_iSampleCount, ReportText(progress.m_sLastProgressReason), ReportText(progress.m_sLastRecoveryResult)));
				}
				if (progressRouteWindowSeconds > maxRouteTimeoutSeconds)
					maxRouteTimeoutSeconds = progressRouteWindowSeconds;
				bestMovementMeters = Math.Max(bestMovementMeters, progress.m_fMaxMovementMeters);
				bestDistanceClosedMeters = Math.Max(bestDistanceClosedMeters, progress.m_fMaxDistanceClosedMeters);
				convoyPhaseHistory = AppendConvoyDebugPhaseHistory(convoyPhaseHistory, progress.m_sPhaseHistory);
				if (progress.m_bHardStuck)
					hardStuckCount++;
				if (progress.m_bNoProgress)
					noProgressCount++;

				string progressActual = string.Format("distance %1m | sampled %2m | no-progress %3 | hard-stuck %4 | reason %5 | recovery %6", Math.Round(ResolveMissionConvoyDistanceToDestinationMeters(mission, asset, vehiclePosition)), Math.Round(progress.m_fDistanceToDestinationMeters), ReportBool(progress.m_bNoProgress), ReportBool(progress.m_bHardStuck), ReportText(progress.m_sLastProgressReason), ReportText(progress.m_sLastRecoveryResult));
				AddConvoyDebugProbeAssertion(probe, "convoy.movement.progress." + asset.m_sAssetId, "progress sampled with no hard stuck", progressActual, ConvoyDebugStatus(!progress.m_bHardStuck, "WARN"), "convoy progress reports hard-stuck recovery state", groupId, mission.m_sInstanceId);
				string sampleWindowActual = string.Format("samples %1 | window %2/%3s | last moved %4m | last closed %5m | max moved %6m | max closed %7m", progress.m_iSampleCount, progressRouteWindowSeconds, CONVOY_ROUTE_REISSUE_THRESHOLD_SECONDS, Math.Round(progress.m_fLastMovementMeters), Math.Round(progress.m_fLastDistanceClosedMeters), Math.Round(progress.m_fMaxMovementMeters), Math.Round(progress.m_fMaxDistanceClosedMeters));
				sampleWindowActual = sampleWindowActual + string.Format(" | reissue %1 | snap %2 | timeout %3 | phases %4", progress.m_iRouteReissueAttemptCount, progress.m_iRouteSnapAttemptCount, ReportBool(progressRouteTimedOut), ReportText(progress.m_sPhaseHistory));
				string movementWindowStatus = "WARN";
				string movementWindowReason = "convoy movement window did not prove movement, recovery, or phase progress for this vehicle";
				if (progressRouteTimedOut)
					movementWindowStatus = "FAIL";
				else if (progressRepeatedSamples && progressWindowProved)
					movementWindowStatus = "PASS";
				else if (progressWindowPending && !progressStallWindowComplete)
					movementWindowReason = "convoy movement window remains inside stall grace before recovery threshold";
				AddConvoyDebugProbeAssertion(probe, "convoy.movement.window." + asset.m_sAssetId, "repeated samples prove distance closure, recovery, contact, or terminal progress", sampleWindowActual, movementWindowStatus, movementWindowReason, groupId, mission.m_sInstanceId);
			}
			assetIndex++;
		}

		AddConvoyDebugProbeMetric(probe, "convoy.progress.sampled", string.Format("%1", progressSampledCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.repeated_sampled", string.Format("%1", repeatedProgressSampledCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.distance_decrease_sampled", string.Format("%1", distanceDecreaseSampledCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.missing", string.Format("%1", missingProgressCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.no_progress", string.Format("%1", noProgressCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.hard_stuck", string.Format("%1", hardStuckCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.movement_window", string.Format("%1", movementWindowCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.recovery_attempted", string.Format("%1", recoveryAttemptCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.pending_stall_window", string.Format("%1", pendingStallWindowCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.stall_timeout", string.Format("%1", routeTimeoutCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.stall_timeout_seconds", string.Format("%1", maxRouteTimeoutSeconds), "seconds");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.best_movement", string.Format("%1", Math.Round(bestMovementMeters)), "meters");
		AddConvoyDebugProbeMetric(probe, "convoy.progress.best_distance_closed", string.Format("%1", Math.Round(bestDistanceClosedMeters)), "meters");
		AddConvoyDebugProbeMetric(probe, "convoy.faction.checked", string.Format("%1", factionCheckedCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.faction.mismatch_groups", string.Format("%1", factionMismatchGroupCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.faction.uncheckable", string.Format("%1", factionUncheckableCount), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.phase.final", ReportText(mission.m_sRuntimePhase), "phase");
		AddConvoyDebugProbeMetric(probe, "convoy.phase.history", ReportText(convoyPhaseHistory), "phase");
		if (!routeTimeoutEvidence.IsEmpty())
			probe.m_aEvidence.Insert("convoy route timeout | " + routeTimeoutEvidence);
		if (!factionMismatchEvidence.IsEmpty())
			probe.m_aEvidence.Insert("convoy crew/vehicle claim mismatch | " + factionMismatchEvidence);
		AddConvoyDebugProbeAssertion(probe, "convoy.movement.sample_count", "progress sample exists for each active convoy vehicle", string.Format("%1/%2 sampled | missing %3", progressSampledCount, activeVehicleAssets, missingProgressCount), ConvoyDebugStatus(activeVehicleAssets >= 3 && progressSampledCount >= activeVehicleAssets && missingProgressCount == 0, "WARN"), "one or more active convoy vehicles have no movement sample yet");
		AddConvoyDebugProbeAssertion(probe, "convoy.movement.repeated_sample_count", "at least two progress samples exist for each active convoy vehicle", string.Format("%1/%2 repeated | phases %3", repeatedProgressSampledCount, activeVehicleAssets, ReportText(convoyPhaseHistory)), ConvoyDebugStatus(activeVehicleAssets >= 3 && repeatedProgressSampledCount >= activeVehicleAssets, "WARN"), "one or more active convoy vehicles lack a repeated movement-window sample");
		AddConvoyDebugProbeAssertion(probe, "convoy.movement.distance_decrease_count", "each active convoy vehicle closes at least 25m toward destination during sampled movement window", string.Format("%1/%2 closed >= 25m | best closed %3m | best moved %4m", distanceDecreaseSampledCount, activeVehicleAssets, Math.Round(bestDistanceClosedMeters), Math.Round(bestMovementMeters)), ConvoyDebugStatus(activeVehicleAssets >= 3 && distanceDecreaseSampledCount >= activeVehicleAssets, "WARN"), "one or more active convoy vehicles did not prove a 25m destination-distance decrease");
		string convoyPhaseActual = string.Format("mission phase %1 | sampled phases %2", ReportText(mission.m_sRuntimePhase), ReportText(convoyPhaseHistory));
		bool convoyTravelPhaseObserved = ConvoyDebugPhaseHistoryHasTravelPhase(convoyPhaseHistory) || ConvoyDebugPhaseIsTravelOrTerminal(mission.m_sRuntimePhase);
		bool convoyContactOrTerminalObserved = ConvoyDebugPhaseHistoryHasPhase(convoyPhaseHistory, MISSION_CONVOY_CONTACT) || ConvoyDebugPhaseHistoryHasTerminalPhase(convoyPhaseHistory) || mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT || ConvoyDebugPhaseIsTerminal(mission.m_sRuntimePhase);
		bool convoyTerminalObserved = ConvoyDebugPhaseHistoryHasTerminalPhase(convoyPhaseHistory) || ConvoyDebugPhaseIsTerminal(mission.m_sRuntimePhase);
		AddConvoyDebugProbeAssertion(probe, "convoy.phase.travel_history", "sample history or final phase includes moving/contact/terminal convoy phase", convoyPhaseActual, ConvoyDebugStatus(convoyTravelPhaseObserved, "WARN"), "convoy movement samples did not observe a travel/contact/terminal phase");
		AddConvoyDebugProbeAssertion(probe, "convoy.phase.contact_or_terminal_history", "sample history or final phase includes contact, arrival, or elimination", convoyPhaseActual, ConvoyDebugStatus(convoyContactOrTerminalObserved, "WARN"), "convoy probe did not observe contact, arrival, or elimination phase evidence");
		AddConvoyDebugProbeAssertion(probe, "convoy.phase.terminal_history", "sample history or final phase includes arrival or elimination when a terminal condition is driven", convoyPhaseActual, ConvoyDebugStatus(convoyTerminalObserved, "WARN"), "convoy probe has not yet proven arrival/elimination terminal phase evidence");
		AddConvoyDebugProbeAssertion(probe, "convoy.movement.hard_stuck_count", "hard-stuck count 0", string.Format("%1", hardStuckCount), ConvoyDebugStatus(hardStuckCount == 0), "one or more convoy vehicles are hard-stuck");
		string routeTimeoutStatus = "PASS";
		if (movementWindowCount < activeVehicleAssets || activeVehicleAssets <= 0)
			routeTimeoutStatus = "WARN";
		if (routeTimeoutCount > 0)
			routeTimeoutStatus = "FAIL";
		else if (pendingStallWindowCount > 0)
			routeTimeoutStatus = "WARN";
		string routeTimeoutActual = string.Format("timed out %1/%2 active | pending %3 | sampled windows %4 | window %5/%6s | evidence %7", routeTimeoutCount, activeVehicleAssets, pendingStallWindowCount, movementWindowCount, maxRouteTimeoutSeconds, CONVOY_ROUTE_REISSUE_THRESHOLD_SECONDS, ReportText(routeTimeoutEvidence));
		AddConvoyDebugProbeAssertion(probe, "convoy.movement.stall_timeout", "fully sampled convoy movement windows do not stall without movement, distance closure, recovery, contact, or arrival", routeTimeoutActual, routeTimeoutStatus, "one or more convoy vehicles timed out across the controlled movement window", "", mission.m_sInstanceId);
		string factionAggregateStatus = "PASS";
		if (factionMismatchGroupCount > 0)
			factionAggregateStatus = "FAIL";
		else if (factionUncheckableCount > 0 || factionCheckedCount < activeVehicleAssets)
			factionAggregateStatus = "WARN";
		string factionAggregateActual = string.Format("checked %1/%2 active | mismatched groups %3 | uncheckable %4 | evidence %5", factionCheckedCount, activeVehicleAssets, factionMismatchGroupCount, factionUncheckableCount, ReportText(factionMismatchEvidence));
		AddConvoyDebugProbeAssertion(probe, "convoy.faction.runtime_entities", "every checkable convoy crew entity is factioned and convoy vehicles are unclaimed", factionAggregateActual, factionAggregateStatus, "one or more convoy crews used the wrong faction or vehicles were claimed", "", mission.m_sInstanceId);

		probe.m_aEvidence.Insert(BuildConvoyRuntimeReport(state, mission));
		FinalizeConvoyDebugProbeCase(state, probe);
		return probe;
	}

	protected string ResolveCampaignDebugMissionConvoyPopulation(HST_CampaignState state, HST_ActiveMissionState mission, out int attempted, out int resolved, out int unresolved, out bool seatingChanged, out bool waypointChanged)
	{
		attempted = 0;
		resolved = 0;
		unresolved = 0;
		seatingChanged = false;
		waypointChanged = false;
		if (!state || !mission)
			return "missing state or mission";

		string evidence;
		int assetIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, assetIndex);
			assetIndex++;
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			if (!activeGroup)
				continue;
			int liveBefore = CountAliveRuntimeCrewAgents(activeGroup);
			bool pendingPopulation = activeGroup.m_sRuntimeStatus == "spawn_pending_agents";
			bool repairPopulation = !pendingPopulation && liveBefore <= 0 && CanAttemptMissionConvoyCrewPopulationRepair(state, mission, activeGroup);
			if (!pendingPopulation && !repairPopulation)
				continue;

			attempted++;
			string groupEvidence;
			bool groupResolved;
			if (pendingPopulation)
				groupResolved = CampaignDebugResolvePendingActiveGroupPopulation(activeGroup, state, ResolveMissionConvoyRuntimeStatus(mission), groupEvidence);
			else
			{
				groupResolved = TryRepairMissionConvoyCrewPopulation(state, mission, activeGroup, "campaign debug pre-route");
				groupEvidence = "zero-live convoy crew repair";
			}
			int liveAfter = CountAliveRuntimeCrewAgents(activeGroup);
			if (groupResolved && liveAfter > 0)
				resolved++;
			else
				unresolved++;

			string sample = string.Format("%1 live %2 -> %3 | resolved %4", ReportText(groupId), liveBefore, liveAfter, ReportBool(groupResolved));
			if (repairPopulation)
				sample = sample + " | repair yes";
			sample = sample + string.Format(" | %1", ReportText(groupEvidence));
			evidence = AppendConvoyDebugEvidence(evidence, sample);
		}

		if (attempted > 0)
		{
			seatingChanged = EnsureMissionConvoyCrewSeating(state, mission);
			HST_ConvoyReadinessStatus readiness = BuildMissionConvoyReadinessStatus(state, mission);
			if (CanAttemptMissionConvoyWaypointAssignment(readiness))
				waypointChanged = AssignMissionConvoyWaypoints(state, mission);
		}

		if (evidence.IsEmpty())
			evidence = "no pending convoy crew population";
		return evidence;
	}

	HST_CampaignDebugCaseResult BuildCampaignDebugConvoyPhaseChainProbe(HST_CampaignState state, string debugPrefix)
	{
		HST_CampaignDebugCaseResult probe = CreateConvoyPhaseChainDebugCase(state);
		if (!state)
		{
			AddConvoyDebugProbeAssertion(probe, "convoy.phase_chain.prerequisite.state", "campaign state exists", "missing", "BLOCKED", "controlled convoy phase-chain probe missing campaign state");
			FinalizeConvoyPhaseChainDebugCase(state, probe);
			return probe;
		}

		string prefix = ResolveConvoyPhaseChainDebugPrefix(debugPrefix);
		string instanceId = prefix + "_convoy_phase_chain";
		RemoveConvoyPhaseChainDebugRecords(state, instanceId);

		HST_ActiveMissionState mission = CreateConvoyPhaseChainDebugMission(state, instanceId);
		HST_MissionObjectiveState objective = CreateConvoyPhaseChainDebugObjective(mission);
		state.m_aActiveMissions.Insert(mission);
		state.m_aMissionObjectives.Insert(objective);
		for (int i = 0; i < 3; i++)
		{
			HST_MissionAssetState asset = CreateConvoyPhaseChainDebugVehicleAsset(mission, i);
			HST_ActiveGroupState activeGroup = CreateConvoyPhaseChainDebugGroup(mission, asset, i, state.m_iElapsedSeconds);
			state.m_aMissionAssets.Insert(asset);
			state.m_aActiveGroups.Insert(activeGroup);
		}

		string observedPhases;
		UpdateMissionConvoyPhase(state, mission, state.m_iElapsedSeconds);
		observedPhases = AppendConvoyPhaseChainObservedPhase(observedPhases, mission.m_sRuntimePhase);
		AddConvoyDebugProbeAssertion(probe, "convoy.phase_chain.staging", "UpdateMissionConvoyPhase initializes empty convoy phase to convoy_staging", BuildConvoyPhaseChainActual(mission, observedPhases), ConvoyDebugStatus(mission.m_sRuntimePhase == MISSION_CONVOY_STAGING), "controlled convoy phase-chain did not enter staging", "", instanceId);

		SetMissionConvoyMoving(state, mission);
		observedPhases = AppendConvoyPhaseChainObservedPhase(observedPhases, mission.m_sRuntimePhase);
		bool groupsMoving = CountConvoyPhaseChainDebugGroupsWithStatus(state, mission, MISSION_CONVOY_MOVING) == 3;
		bool movingEvent = mission.m_sLastRuntimeEventKey == CONVOY_MOVE_EVENT_PENDING;
		AddConvoyDebugProbeAssertion(probe, "convoy.phase_chain.moving", "SetMissionConvoyMoving sets phase, event key, and convoy group runtime status", BuildConvoyPhaseChainGroupActual(state, mission, observedPhases, MISSION_CONVOY_MOVING), ConvoyDebugStatus(mission.m_sRuntimePhase == MISSION_CONVOY_MOVING && groupsMoving && movingEvent), "controlled convoy phase-chain did not enter moving with group status propagation", "", instanceId);

		SetMissionConvoyContact(state, mission, "Campaign debug controlled convoy contact.");
		observedPhases = AppendConvoyPhaseChainObservedPhase(observedPhases, mission.m_sRuntimePhase);
		bool groupsContact = CountConvoyPhaseChainDebugGroupsWithStatus(state, mission, MISSION_CONVOY_CONTACT) == 3;
		AddConvoyDebugProbeAssertion(probe, "convoy.phase_chain.contact", "SetMissionConvoyContact sets contact phase, reason, and group runtime status", BuildConvoyPhaseChainGroupActual(state, mission, observedPhases, MISSION_CONVOY_CONTACT), ConvoyDebugStatus(mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT && groupsContact && !mission.m_sRuntimeFailureReason.IsEmpty()), "controlled convoy phase-chain did not enter contact with reason and group status propagation", "", instanceId);

		MarkConvoyPhaseChainDebugGroupsEliminated(state, mission);
		UpdateMissionConvoyObjective(state, mission);
		observedPhases = AppendConvoyPhaseChainObservedPhase(observedPhases, mission.m_sRuntimePhase);
		HST_ConvoyCompletionStatus completion = BuildMissionConvoyCompletionStatus(state, mission);
		bool objectiveComplete = objective.m_bComplete && objective.m_iCurrentCount == objective.m_iRequiredCount && objective.m_iCurrentProgress == objective.m_iRequiredProgress;
		bool groupsEliminated = CountConvoyPhaseChainDebugGroupsWithStatus(state, mission, MISSION_CONVOY_ELIMINATED) == 3;
		bool eliminatedPhase = mission.m_sRuntimePhase == MISSION_CONVOY_ELIMINATED && mission.m_sLastRuntimeEventKey == CONVOY_COMPLETE_EVENT_KEY;
		string eliminatedActual = BuildConvoyPhaseChainGroupActual(state, mission, observedPhases, MISSION_CONVOY_ELIMINATED);
		eliminatedActual = eliminatedActual + string.Format(" | objective %1/%2 | progress %3/%4 | completion %5/%6 | reason %7", objective.m_iCurrentCount, objective.m_iRequiredCount, objective.m_iCurrentProgress, objective.m_iRequiredProgress, completion.m_iEliminatedCrewGroups, completion.m_iRequiredCrewGroups, ReportText(completion.m_sReason));
		AddConvoyDebugProbeAssertion(probe, "convoy.phase_chain.eliminated", "UpdateMissionConvoyObjective detects eliminated crews and completes convoy objective through convoy_eliminated", eliminatedActual, ConvoyDebugStatus(eliminatedPhase && groupsEliminated && objectiveComplete && completion.m_bCanComplete), "controlled convoy phase-chain did not complete through eliminated crew objective path", "", instanceId);

		bool exactSequence = observedPhases == "convoy_staging -> convoy_moving -> convoy_contact -> convoy_eliminated";
		AddConvoyDebugProbeMetric(probe, "convoy.phase_chain.vehicle_assets", string.Format("%1", CountConvoyPhaseChainDebugVehicleAssets(state, instanceId)), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.phase_chain.crew_groups", string.Format("%1", CountConvoyPhaseChainDebugGroups(state, mission)), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.phase_chain.eliminated_groups", string.Format("%1", completion.m_iEliminatedCrewGroups), "count");
		AddConvoyDebugProbeMetric(probe, "convoy.phase_chain.observed", observedPhases, "phase");
		AddConvoyDebugProbeAssertion(probe, "convoy.phase_chain.sequence", "controlled helper sequence is staging -> moving -> contact -> eliminated", observedPhases, ConvoyDebugStatus(exactSequence), "controlled convoy phase-chain sequence did not include every expected phase in order", "", instanceId);
		probe.m_aEvidence.Insert("controlled state-machine probe uses temporary debug convoy records and real HST_PhysicalWarService transition helpers; natural physical driving remains covered by convoy_physical movement assertions");

		bool cleaned = RemoveConvoyPhaseChainDebugRecords(state, instanceId);
		AddConvoyDebugProbeAssertion(probe, "convoy.phase_chain.cleanup", "temporary convoy phase-chain mission, objective, assets, groups, and progress records are removed", BuildConvoyPhaseChainCleanupActual(cleaned, state, instanceId), ConvoyDebugStatus(cleaned), "controlled convoy phase-chain probe leaked temporary records");
		FinalizeConvoyPhaseChainDebugCase(state, probe);
		return probe;
	}

	HST_CampaignDebugCaseResult BuildCampaignDebugExpiredConvoyRenderBubbleProbe(HST_CampaignState state, string debugPrefix, bool physicalBlocked)
	{
		HST_CampaignDebugCaseResult probe = CreateExpiredConvoyRenderBubbleDebugCase(state);
		if (physicalBlocked)
		{
			AddConvoyDebugProbeAssertion(probe, "render_bubble.convoy.prerequisite.player", "controlled player entity available", "missing", "BLOCKED", "bootstrap marked physical runtime tests blocked");
			FinalizeExpiredConvoyRenderBubbleDebugCase(state, probe);
			return probe;
		}

		if (!state)
		{
			AddConvoyDebugProbeAssertion(probe, "render_bubble.convoy.prerequisite.state", "campaign state exists", "missing", "BLOCKED", "expired convoy render-bubble probe missing campaign state");
			FinalizeExpiredConvoyRenderBubbleDebugCase(state, probe);
			return probe;
		}

		vector playerPosition;
		if (!ResolveFirstLivingPlayerDebugPosition(playerPosition))
		{
			AddConvoyDebugProbeAssertion(probe, "render_bubble.convoy.player", "living player entity exists", "missing", "BLOCKED", "expired convoy render-bubble probe requires a living player");
			FinalizeExpiredConvoyRenderBubbleDebugCase(state, probe);
			return probe;
		}

		string prefix = ResolveExpiredConvoyRenderBubbleDebugPrefix(debugPrefix);
		vector nearPosition = playerPosition;
		nearPosition[0] = nearPosition[0] + 10.0;
		vector farPosition = ResolveExpiredConvoyRenderBubbleDebugFarPosition(playerPosition);

		string nearInstanceId = prefix + "_expired_convoy_near";
		HST_ActiveMissionState nearMission = CreateExpiredConvoyRenderBubbleDebugMission(nearInstanceId, nearPosition);
		HST_ActiveGroupState nearGroup = CreateExpiredConvoyRenderBubbleDebugGroup(nearMission, nearPosition);
		RemoveExpiredConvoyRenderBubbleDebugRecords(state, nearMission.m_sInstanceId, nearGroup.m_sGroupId);
		IEntity nearCrewEntity = SpawnExpiredConvoyRenderBubbleDebugCrewEntity(nearPosition, nearGroup.m_sGroupId);
		if (!nearCrewEntity)
		{
			AddConvoyDebugProbeAssertion(probe, "render_bubble.convoy.temp_entity.near", "temporary live crew entity spawns", "missing", "BLOCKED", "expired convoy render-bubble probe could not spawn temporary crew entity", nearGroup.m_sGroupId, nearMission.m_sInstanceId);
			FinalizeExpiredConvoyRenderBubbleDebugCase(state, probe);
			return probe;
		}

		RegisterExpiredConvoyRenderBubbleDebugRuntime(state, nearMission, nearGroup, nearCrewEntity);
		int nearAliveCrewBefore = CountAliveRuntimeCrewAgents(nearGroup);
		float nearDistance = ResolveNearestLivingPlayerDistanceMeters(nearPosition);
		bool nearCleanupChanged = CleanupInactiveMissionConvoyRuntimeGroupForDebug(state, nearGroup);
		HST_ActiveGroupState nearObservedGroup = state.FindActiveGroup(nearGroup.m_sGroupId);
		bool nearGroupPreserved = nearObservedGroup != null;
		bool nearRuntimePreserved = GetRuntimeCrewGroupEntity(nearGroup.m_sGroupId) != null;
		bool nearMarkedPreserved = nearObservedGroup != null && nearObservedGroup.m_sSpawnFallbackMode == "expired_combat_preserved";
		string nearActual = BuildExpiredConvoyRenderBubbleDebugActual(nearGroup.m_sGroupId, nearCleanupChanged, nearGroupPreserved, nearRuntimePreserved, nearAliveCrewBefore, nearDistance, nearGroup.m_sSpawnFallbackMode, nearGroup.m_sSpawnFailureReason);
		AddConvoyDebugProbeMetric(probe, "render_bubble.convoy.radius", string.Format("%1", Math.Round(EXPIRED_CONVOY_PLAYER_RENDER_BUBBLE_METERS)), "m");
		AddConvoyDebugProbeMetric(probe, "render_bubble.convoy.near_distance", string.Format("%1", Math.Round(nearDistance)), "m");
		AddConvoyDebugProbeAssertion(probe, "render_bubble.convoy.near_preserve", "expired contact convoy runtime inside player bubble is preserved and marked", nearActual, ConvoyDebugStatus(nearAliveCrewBefore > 0 && nearDistance >= 0 && nearDistance <= EXPIRED_CONVOY_PLAYER_RENDER_BUBBLE_METERS && nearGroupPreserved && nearRuntimePreserved && nearMarkedPreserved), "near expired convoy contact runtime was not preserved", nearGroup.m_sGroupId, nearMission.m_sInstanceId);
		bool nearRecordsRemoved = RemoveExpiredConvoyRenderBubbleDebugRecords(state, nearMission.m_sInstanceId, nearGroup.m_sGroupId);

		string farInstanceId = prefix + "_expired_convoy_far";
		HST_ActiveMissionState farMission = CreateExpiredConvoyRenderBubbleDebugMission(farInstanceId, farPosition);
		HST_ActiveGroupState farGroup = CreateExpiredConvoyRenderBubbleDebugGroup(farMission, farPosition);
		RemoveExpiredConvoyRenderBubbleDebugRecords(state, farMission.m_sInstanceId, farGroup.m_sGroupId);
		IEntity farCrewEntity = SpawnExpiredConvoyRenderBubbleDebugCrewEntity(farPosition, farGroup.m_sGroupId);
		if (!farCrewEntity)
		{
			AddConvoyDebugProbeAssertion(probe, "render_bubble.convoy.temp_entity.far", "temporary live crew entity spawns outside bubble", "missing", "BLOCKED", "expired convoy render-bubble probe could not spawn far temporary crew entity", farGroup.m_sGroupId, farMission.m_sInstanceId);
			bool farFailedRecordsRemoved = RemoveExpiredConvoyRenderBubbleDebugRecords(state, farMission.m_sInstanceId, farGroup.m_sGroupId);
			AddConvoyDebugProbeAssertion(probe, "render_bubble.convoy.cleanup", "temporary expired convoy records are cleaned", BuildExpiredConvoyRenderBubbleCleanupActual(nearRecordsRemoved, farFailedRecordsRemoved), ConvoyDebugStatus(nearRecordsRemoved && farFailedRecordsRemoved), "expired convoy render-bubble probe leaked temporary records");
			FinalizeExpiredConvoyRenderBubbleDebugCase(state, probe);
			return probe;
		}

		RegisterExpiredConvoyRenderBubbleDebugRuntime(state, farMission, farGroup, farCrewEntity);
		int farAliveCrewBefore = CountAliveRuntimeCrewAgents(farGroup);
		float farDistance = ResolveNearestLivingPlayerDistanceMeters(farPosition);
		bool farCleanupChanged = CleanupInactiveMissionConvoyRuntimeGroupForDebug(state, farGroup);
		bool farGroupRemoved = state.FindActiveGroup(farGroup.m_sGroupId) == null;
		bool farRuntimeRemoved = GetRuntimeCrewGroupEntity(farGroup.m_sGroupId) == null;
		string farActual = BuildExpiredConvoyRenderBubbleDebugActual(farGroup.m_sGroupId, farCleanupChanged, !farGroupRemoved, !farRuntimeRemoved, farAliveCrewBefore, farDistance, farGroup.m_sSpawnFallbackMode, farGroup.m_sSpawnFailureReason);
		AddConvoyDebugProbeMetric(probe, "render_bubble.convoy.far_distance", string.Format("%1", Math.Round(farDistance)), "m");
		AddConvoyDebugProbeAssertion(probe, "render_bubble.convoy.far_cleanup", "expired contact convoy runtime outside player bubble is deleted", farActual, ConvoyDebugStatus(farAliveCrewBefore > 0 && farDistance > EXPIRED_CONVOY_PLAYER_RENDER_BUBBLE_METERS && farGroupRemoved && farRuntimeRemoved), "far expired convoy contact runtime was not cleaned up", farGroup.m_sGroupId, farMission.m_sInstanceId);
		bool farRecordsRemoved = RemoveExpiredConvoyRenderBubbleDebugRecords(state, farMission.m_sInstanceId, farGroup.m_sGroupId);

		AddConvoyDebugProbeAssertion(probe, "render_bubble.convoy.cleanup", "temporary expired convoy records are cleaned", BuildExpiredConvoyRenderBubbleCleanupActual(nearRecordsRemoved, farRecordsRemoved), ConvoyDebugStatus(nearRecordsRemoved && farRecordsRemoved), "expired convoy render-bubble probe leaked temporary records");
		FinalizeExpiredConvoyRenderBubbleDebugCase(state, probe);
		return probe;
	}

	bool StartCampaignDebugPhysicalCombatProbe(HST_CampaignState state, HST_CampaignPreset preset, string debugPrefix, bool physicalBlocked, out string result)
	{
		result = "";
		CleanupCampaignDebugPhysicalCombatProbeRuntime(state);
		ResetCampaignDebugPhysicalCombatProbeState();

		if (physicalBlocked)
		{
			result = "h-istasi campaign debug | physical combat probe blocked: no controlled player entity";
			return false;
		}
		if (!state || !preset)
		{
			result = "h-istasi campaign debug | physical combat probe failed: campaign state or preset missing";
			return false;
		}

		vector playerPosition;
		if (!ResolveFirstLivingPlayerDebugPosition(playerPosition))
		{
			result = "h-istasi campaign debug | physical combat probe blocked: no living player render-bubble anchor";
			return false;
		}

		string prefix = ResolveCampaignDebugPhysicalCombatProbePrefix(debugPrefix);
		m_sCampaignDebugCombatProbeId = prefix + "_physical_combat";
		m_sCampaignDebugCombatProbeFriendlyGroupId = m_sCampaignDebugCombatProbeId + "_resistance";
		m_sCampaignDebugCombatProbeEnemyGroupId = m_sCampaignDebugCombatProbeId + "_enemy";
		m_sCampaignDebugCombatProbeFriendlyFaction = ResolveCampaignDebugPhysicalCombatResistanceFaction(preset);
		m_sCampaignDebugCombatProbeEnemyFaction = ResolveCampaignDebugPhysicalCombatEnemyFaction(preset, m_sCampaignDebugCombatProbeFriendlyFaction);

		m_vCampaignDebugCombatProbeCenter = playerPosition;
		m_vCampaignDebugCombatProbeCenter[0] = m_vCampaignDebugCombatProbeCenter[0] + CAMPAIGN_DEBUG_COMBAT_PROBE_PLAYER_OFFSET_METERS;
		m_vCampaignDebugCombatProbeCenter = HST_WorldPositionService.ResolveSafeGroundPosition(m_vCampaignDebugCombatProbeCenter, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		m_vCampaignDebugCombatProbeFriendlyPosition = m_vCampaignDebugCombatProbeCenter;
		m_vCampaignDebugCombatProbeFriendlyPosition[0] = m_vCampaignDebugCombatProbeFriendlyPosition[0] - (CAMPAIGN_DEBUG_COMBAT_PROBE_SEPARATION_METERS * 0.5);
		m_vCampaignDebugCombatProbeEnemyPosition = m_vCampaignDebugCombatProbeCenter;
		m_vCampaignDebugCombatProbeEnemyPosition[0] = m_vCampaignDebugCombatProbeEnemyPosition[0] + (CAMPAIGN_DEBUG_COMBAT_PROBE_SEPARATION_METERS * 0.5);
		m_vCampaignDebugCombatProbeFriendlyPosition = HST_WorldPositionService.ResolveSafeGroundPosition(m_vCampaignDebugCombatProbeFriendlyPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		m_vCampaignDebugCombatProbeEnemyPosition = HST_WorldPositionService.ResolveSafeGroundPosition(m_vCampaignDebugCombatProbeEnemyPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);

		HST_ZoneState zone = SelectCampaignDebugPhysicalCombatProbeZone(state, m_vCampaignDebugCombatProbeCenter);
		if (!zone)
		{
			result = "h-istasi campaign debug | physical combat probe failed: no zone available for active-group ownership";
			return false;
		}
		m_sCampaignDebugCombatProbeZoneId = zone.m_sZoneId;

		HST_ActiveGroupState friendlyGroup = CreateCampaignDebugPhysicalCombatProbeGroup(state, preset, zone, m_sCampaignDebugCombatProbeFriendlyGroupId, m_sCampaignDebugCombatProbeFriendlyFaction, m_vCampaignDebugCombatProbeFriendlyPosition, m_vCampaignDebugCombatProbeEnemyPosition);
		HST_ActiveGroupState enemyGroup = CreateCampaignDebugPhysicalCombatProbeGroup(state, preset, zone, m_sCampaignDebugCombatProbeEnemyGroupId, m_sCampaignDebugCombatProbeEnemyFaction, m_vCampaignDebugCombatProbeEnemyPosition, m_vCampaignDebugCombatProbeFriendlyPosition);
		if (!friendlyGroup || !enemyGroup)
		{
			result = "h-istasi campaign debug | physical combat probe failed: could not build temporary active groups";
			return false;
		}

		state.m_aActiveGroups.Insert(friendlyGroup);
		state.m_aActiveGroups.Insert(enemyGroup);
		bool friendlySpawned = TrySpawnActiveGroup(friendlyGroup, state, preset);
		bool enemySpawned = TrySpawnActiveGroup(enemyGroup, state, preset);
		m_bCampaignDebugCombatProbeFriendlyPopulationResolved = CampaignDebugResolvePendingActiveGroupPopulation(friendlyGroup, state, "campaign_debug_combat", m_sCampaignDebugCombatProbeFriendlyPopulationEvidence);
		m_bCampaignDebugCombatProbeEnemyPopulationResolved = CampaignDebugResolvePendingActiveGroupPopulation(enemyGroup, state, "campaign_debug_combat", m_sCampaignDebugCombatProbeEnemyPopulationEvidence);
		if (AssignCampaignDebugPhysicalCombatWaypoint(m_sCampaignDebugCombatProbeFriendlyGroupId, m_vCampaignDebugCombatProbeEnemyPosition))
			m_iCampaignDebugCombatProbeWaypointCount++;
		if (AssignCampaignDebugPhysicalCombatWaypoint(m_sCampaignDebugCombatProbeEnemyGroupId, m_vCampaignDebugCombatProbeFriendlyPosition))
			m_iCampaignDebugCombatProbeWaypointCount++;

		m_iCampaignDebugCombatProbeStartSecond = state.m_iElapsedSeconds;
		m_bCampaignDebugCombatProbeActive = true;
		SampleCampaignDebugPhysicalCombatProbe(state, true);
		m_sCampaignDebugCombatProbeStartResult = string.Format("friendly %1 spawned %2 population %3 | enemy %4 spawned %5 population %6", m_sCampaignDebugCombatProbeFriendlyGroupId, friendlySpawned, m_bCampaignDebugCombatProbeFriendlyPopulationResolved, m_sCampaignDebugCombatProbeEnemyGroupId, enemySpawned, m_bCampaignDebugCombatProbeEnemyPopulationResolved);
		m_sCampaignDebugCombatProbeStartResult = m_sCampaignDebugCombatProbeStartResult + string.Format(" | waypoints %1 | center %2", m_iCampaignDebugCombatProbeWaypointCount, m_vCampaignDebugCombatProbeCenter);
		result = "h-istasi campaign debug | physical combat probe started | " + m_sCampaignDebugCombatProbeStartResult;
		return true;
	}

	HST_CampaignDebugCaseResult FinishCampaignDebugPhysicalCombatProbe(HST_CampaignState state, bool physicalBlocked)
	{
		HST_CampaignDebugCaseResult probe = CreatePhysicalCombatDebugCase(state);
		if (physicalBlocked)
		{
			AddConvoyDebugProbeAssertion(probe, "physical_combat.prerequisite.player", "controlled player entity available", "missing", "BLOCKED", "bootstrap marked physical runtime tests blocked");
			FinalizePhysicalCombatDebugCase(state, probe);
			return probe;
		}
		if (!state)
		{
			AddConvoyDebugProbeAssertion(probe, "physical_combat.prerequisite.state", "campaign state exists", "missing", "BLOCKED", "physical combat probe missing campaign state");
			FinalizePhysicalCombatDebugCase(state, probe);
			return probe;
		}
		if (!m_bCampaignDebugCombatProbeActive)
		{
			AddConvoyDebugProbeAssertion(probe, "physical_combat.started", "physical combat probe was started before result collection", "not active", "BLOCKED", "physical combat probe was not active when result step ran");
			FinalizePhysicalCombatDebugCase(state, probe);
			return probe;
		}

		SampleCampaignDebugPhysicalCombatProbe(state, true);
		int elapsedSeconds = Math.Max(0, state.m_iElapsedSeconds - m_iCampaignDebugCombatProbeStartSecond);
		HST_ActiveGroupState friendlyGroup = state.FindActiveGroup(m_sCampaignDebugCombatProbeFriendlyGroupId);
		HST_ActiveGroupState enemyGroup = state.FindActiveGroup(m_sCampaignDebugCombatProbeEnemyGroupId);
		bool friendlyRuntimeExists = GetRuntimeGroupEntity(m_sCampaignDebugCombatProbeFriendlyGroupId) != null;
		bool enemyRuntimeExists = GetRuntimeGroupEntity(m_sCampaignDebugCombatProbeEnemyGroupId) != null;
		bool factionSplit = !m_sCampaignDebugCombatProbeFriendlyFaction.IsEmpty() && !m_sCampaignDebugCombatProbeEnemyFaction.IsEmpty() && m_sCampaignDebugCombatProbeFriendlyFaction != m_sCampaignDebugCombatProbeEnemyFaction;
		string factionRelationActual;
		bool factionsHostile = AreRuntimeFactionKeysHostile(m_sCampaignDebugCombatProbeFriendlyFaction, m_sCampaignDebugCombatProbeEnemyFaction, factionRelationActual);
		bool friendlyAliveObserved = m_iCampaignDebugCombatProbeFriendlyStartAlive > 0 || m_iCampaignDebugCombatProbeFriendlyEndAlive > 0;
		bool enemyAliveObserved = m_iCampaignDebugCombatProbeEnemyStartAlive > 0 || m_iCampaignDebugCombatProbeEnemyEndAlive > 0;
		bool friendlyLossObserved = m_iCampaignDebugCombatProbeFriendlyStartAlive > 0 && m_iCampaignDebugCombatProbeFriendlyMinAlive >= 0 && m_iCampaignDebugCombatProbeFriendlyMinAlive < m_iCampaignDebugCombatProbeFriendlyStartAlive;
		bool enemyLossObserved = m_iCampaignDebugCombatProbeEnemyStartAlive > 0 && m_iCampaignDebugCombatProbeEnemyMinAlive >= 0 && m_iCampaignDebugCombatProbeEnemyMinAlive < m_iCampaignDebugCombatProbeEnemyStartAlive;
		bool casualtyObserved = friendlyLossObserved || enemyLossObserved;
		bool contactObserved = casualtyObserved || (m_fCampaignDebugCombatProbeLastDistance >= 0 && m_fCampaignDebugCombatProbeLastDistance <= CAMPAIGN_DEBUG_COMBAT_PROBE_CONTACT_METERS);
		bool sampleWindowCovered = elapsedSeconds >= CAMPAIGN_DEBUG_COMBAT_PROBE_SAMPLE_SECONDS - 1 && m_iCampaignDebugCombatProbeSampleCount >= 2;
		string friendlyFactionSample;
		string enemyFactionSample;
		int friendlyFactionMismatches = -1;
		int enemyFactionMismatches = -1;
		if (friendlyGroup)
		{
			EnsureActiveGroupRuntimeFaction(friendlyGroup, "physical combat probe");
			friendlyFactionMismatches = CountActiveGroupRuntimeFactionMismatches(friendlyGroup, friendlyFactionSample);
		}
		if (enemyGroup)
		{
			EnsureActiveGroupRuntimeFaction(enemyGroup, "physical combat probe");
			enemyFactionMismatches = CountActiveGroupRuntimeFactionMismatches(enemyGroup, enemyFactionSample);
		}
		bool friendlyFactionOk = friendlyGroup && friendlyRuntimeExists && friendlyAliveObserved && friendlyFactionMismatches == 0;
		bool enemyFactionOk = enemyGroup && enemyRuntimeExists && enemyAliveObserved && enemyFactionMismatches == 0;

		AddConvoyDebugProbeMetric(probe, "physical_combat.elapsed_seconds", string.Format("%1", elapsedSeconds), "seconds");
		AddConvoyDebugProbeMetric(probe, "physical_combat.samples", string.Format("%1", m_iCampaignDebugCombatProbeSampleCount), "count");
		AddConvoyDebugProbeMetric(probe, "physical_combat.friendly_alive_start", string.Format("%1", m_iCampaignDebugCombatProbeFriendlyStartAlive), "count");
		AddConvoyDebugProbeMetric(probe, "physical_combat.enemy_alive_start", string.Format("%1", m_iCampaignDebugCombatProbeEnemyStartAlive), "count");
		AddConvoyDebugProbeMetric(probe, "physical_combat.friendly_alive_end", string.Format("%1", m_iCampaignDebugCombatProbeFriendlyEndAlive), "count");
		AddConvoyDebugProbeMetric(probe, "physical_combat.enemy_alive_end", string.Format("%1", m_iCampaignDebugCombatProbeEnemyEndAlive), "count");
		AddConvoyDebugProbeMetric(probe, "physical_combat.final_distance", string.Format("%1", Math.Round(m_fCampaignDebugCombatProbeLastDistance)), "meters");
		AddConvoyDebugProbeMetric(probe, "physical_combat.friendly_population_resolved", string.Format("%1", m_bCampaignDebugCombatProbeFriendlyPopulationResolved), "bool");
		AddConvoyDebugProbeMetric(probe, "physical_combat.enemy_population_resolved", string.Format("%1", m_bCampaignDebugCombatProbeEnemyPopulationResolved), "bool");

		probe.m_aEvidence.Insert("start | " + ReportText(m_sCampaignDebugCombatProbeStartResult));
		probe.m_aEvidence.Insert("population | friendly " + ReportText(m_sCampaignDebugCombatProbeFriendlyPopulationEvidence) + " | enemy " + ReportText(m_sCampaignDebugCombatProbeEnemyPopulationEvidence));
		probe.m_aEvidence.Insert("samples | " + ReportText(m_sCampaignDebugCombatProbeHistory));
		AddConvoyDebugProbeAssertion(probe, "physical_combat.faction_split", "temporary groups use resistance and enemy factions, not all FIA", string.Format("friendly %1 | enemy %2", ReportText(m_sCampaignDebugCombatProbeFriendlyFaction), ReportText(m_sCampaignDebugCombatProbeEnemyFaction)), ConvoyDebugStatus(factionSplit), "physical combat probe factions were not split between resistance and enemy", "", "", m_sCampaignDebugCombatProbeZoneId);
		AddConvoyDebugProbeAssertion(probe, "physical_combat.faction_hostility", "temporary group factions are mutually hostile in native faction relations", factionRelationActual, ConvoyDebugStatus(factionsHostile), "physical combat probe selected factions that are not mutually hostile", "", "", m_sCampaignDebugCombatProbeZoneId);
		AddConvoyDebugProbeAssertion(probe, "physical_combat.friendly_runtime_faction", "friendly runtime group/entities are stamped with the resistance faction", BuildActiveGroupRuntimeFactionActual(friendlyGroup, friendlyFactionMismatches, friendlyFactionSample, friendlyRuntimeExists, false), ConvoyDebugStatus(friendlyFactionOk), "friendly combat probe runtime faction mismatch", m_sCampaignDebugCombatProbeFriendlyGroupId, "", m_sCampaignDebugCombatProbeZoneId);
		AddConvoyDebugProbeAssertion(probe, "physical_combat.enemy_runtime_faction", "enemy runtime group/entities are stamped with the enemy faction", BuildActiveGroupRuntimeFactionActual(enemyGroup, enemyFactionMismatches, enemyFactionSample, enemyRuntimeExists, false), ConvoyDebugStatus(enemyFactionOk), "enemy combat probe runtime faction mismatch", m_sCampaignDebugCombatProbeEnemyGroupId, "", m_sCampaignDebugCombatProbeZoneId);
		AddConvoyDebugProbeAssertion(probe, "physical_combat.active_group_records", "temporary active-group records exist for both sides during sample window", string.Format("friendly %1 | enemy %2", friendlyGroup != null, enemyGroup != null), ConvoyDebugStatus(friendlyGroup != null && enemyGroup != null), "temporary active-group records missing before cleanup", "", "", m_sCampaignDebugCombatProbeZoneId);
		string populationActual = string.Format("friendly %1 | enemy %2", ReportText(m_sCampaignDebugCombatProbeFriendlyPopulationEvidence), ReportText(m_sCampaignDebugCombatProbeEnemyPopulationEvidence));
		AddConvoyDebugProbeAssertion(probe, "physical_combat.population", "both temporary groups have durable runtime agents before waypoints and samples are judged", populationActual, ConvoyDebugStatus(m_bCampaignDebugCombatProbeFriendlyPopulationResolved && m_bCampaignDebugCombatProbeEnemyPopulationResolved), "physical combat probe could not prove durable AI population for both sides", "", "", m_sCampaignDebugCombatProbeZoneId);
		AddConvoyDebugProbeAssertion(probe, "physical_combat.runtime_entities", "both temporary groups spawn runtime AI group entities", string.Format("friendly runtime %1 alive %2 | enemy runtime %3 alive %4", friendlyRuntimeExists, m_iCampaignDebugCombatProbeFriendlyEndAlive, enemyRuntimeExists, m_iCampaignDebugCombatProbeEnemyEndAlive), ConvoyDebugStatus(friendlyRuntimeExists && enemyRuntimeExists && friendlyAliveObserved && enemyAliveObserved), "one or both combat probe groups did not spawn living runtime AI", "", "", m_sCampaignDebugCombatProbeZoneId);
		AddConvoyDebugProbeAssertion(probe, "physical_combat.search_waypoints", "both groups receive search-and-destroy waypoints toward the opposing side", string.Format("%1/2", m_iCampaignDebugCombatProbeWaypointCount), ConvoyDebugStatus(m_iCampaignDebugCombatProbeWaypointCount >= 2), "combat probe did not assign opposing search-and-destroy waypoints", "", "", m_sCampaignDebugCombatProbeZoneId);
		AddConvoyDebugProbeAssertion(probe, "physical_combat.sample_window", "normal server tick samples the spawned groups across the configured combat window", string.Format("elapsed %1/%2 | samples %3 | last %4", elapsedSeconds, CAMPAIGN_DEBUG_COMBAT_PROBE_SAMPLE_SECONDS, m_iCampaignDebugCombatProbeSampleCount, ReportText(m_sCampaignDebugCombatProbeLastObserved)), ConvoyDebugStatus(sampleWindowCovered), "physical combat probe did not collect enough timed samples", "", "", m_sCampaignDebugCombatProbeZoneId);
		AddConvoyDebugProbeAssertion(probe, "physical_combat.contact_distance", "opposing groups reach direct contact distance or produce casualty evidence", string.Format("distance %1m | casualty %2", Math.Round(m_fCampaignDebugCombatProbeLastDistance), casualtyObserved), ConvoyDebugStatus(contactObserved), "opposing groups did not reach contact distance or casualty evidence", "", "", m_sCampaignDebugCombatProbeZoneId);
		AddConvoyDebugProbeAssertion(probe, "physical_combat.casualty_resolution", "natural AI combat changes at least one side's live-count during the sample window after contact is proven", string.Format("friendly %1 -> min %2 -> %3 | enemy %4 -> min %5 -> %6 | contact %7", m_iCampaignDebugCombatProbeFriendlyStartAlive, m_iCampaignDebugCombatProbeFriendlyMinAlive, m_iCampaignDebugCombatProbeFriendlyEndAlive, m_iCampaignDebugCombatProbeEnemyStartAlive, m_iCampaignDebugCombatProbeEnemyMinAlive, m_iCampaignDebugCombatProbeEnemyEndAlive, contactObserved), ConvoyDebugStatus(casualtyObserved, "WARN"), "no live-count loss was observed during the physical combat window after contact proof", "", "", m_sCampaignDebugCombatProbeZoneId);

		bool cleaned = CleanupCampaignDebugPhysicalCombatProbeRuntime(state);
		AddConvoyDebugProbeAssertion(probe, "physical_combat.cleanup", "temporary combat groups and waypoints are removed after result collection", BuildPhysicalCombatCleanupActual(state, cleaned), ConvoyDebugStatus(cleaned), "physical combat probe leaked temporary runtime or state records", "", "", m_sCampaignDebugCombatProbeZoneId);
		FinalizePhysicalCombatDebugCase(state, probe);
		ResetCampaignDebugPhysicalCombatProbeState();
		return probe;
	}

	protected HST_CampaignDebugCaseResult CreatePhysicalCombatDebugCase(HST_CampaignState state)
	{
		HST_CampaignDebugCaseResult probe = new HST_CampaignDebugCaseResult();
		probe.m_sCaseId = "physical_combat.ai_contact";
		probe.m_sCategory = "physical_war";
		probe.m_sFeature = "ai_combat_contact";
		probe.m_sStage = "physical_probe";
		probe.m_sStatus = "PASS";
		if (state)
		{
			probe.m_iStartSecond = m_iCampaignDebugCombatProbeStartSecond;
			probe.m_iEndSecond = state.m_iElapsedSeconds;
		}
		probe.m_aEvidence.Insert("temporary resistance/enemy active groups spawn inside the player render bubble and use search-and-destroy waypoints toward one another");
		return probe;
	}

	protected void FinalizePhysicalCombatDebugCase(HST_CampaignState state, HST_CampaignDebugCaseResult probe)
	{
		FinalizeConvoyDebugProbeCase(state, probe);
		if (probe && probe.m_sStatus == "PASS")
			probe.m_sReason = "physical AI combat/contact probe assertions passed";
	}

	protected string ResolveCampaignDebugPhysicalCombatProbePrefix(string debugPrefix)
	{
		if (debugPrefix.IsEmpty())
			return CAMPAIGN_DEBUG_PREFIX_ROOT + "combat";
		if (debugPrefix.Contains(CAMPAIGN_DEBUG_PREFIX_ROOT))
			return debugPrefix;

		return CAMPAIGN_DEBUG_PREFIX_ROOT + debugPrefix;
	}

	protected string ResolveCampaignDebugPhysicalCombatResistanceFaction(HST_CampaignPreset preset)
	{
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			return preset.m_sResistanceFactionKey;

		return "FIA";
	}

	protected string ResolveCampaignDebugPhysicalCombatEnemyFaction(HST_CampaignPreset preset, string resistanceFaction)
	{
		if (preset && !preset.m_sOccupierFactionKey.IsEmpty() && preset.m_sOccupierFactionKey != resistanceFaction && AreRuntimeFactionKeysHostileOrUnknown(resistanceFaction, preset.m_sOccupierFactionKey))
			return preset.m_sOccupierFactionKey;
		if (preset && !preset.m_sInvaderFactionKey.IsEmpty() && preset.m_sInvaderFactionKey != resistanceFaction && AreRuntimeFactionKeysHostileOrUnknown(resistanceFaction, preset.m_sInvaderFactionKey))
			return preset.m_sInvaderFactionKey;
		if (preset && !preset.m_sOccupierFactionKey.IsEmpty() && preset.m_sOccupierFactionKey != resistanceFaction)
			return preset.m_sOccupierFactionKey;
		if (preset && !preset.m_sInvaderFactionKey.IsEmpty() && preset.m_sInvaderFactionKey != resistanceFaction)
			return preset.m_sInvaderFactionKey;
		if (resistanceFaction != "USSR")
			return "USSR";

		return "US";
	}

	protected bool AreRuntimeFactionKeysHostileOrUnknown(string factionKeyA, string factionKeyB)
	{
		string actual;
		if (AreRuntimeFactionKeysHostile(factionKeyA, factionKeyB, actual))
			return true;

		return actual.Contains("missing");
	}

	protected bool AreRuntimeFactionKeysHostile(string factionKeyA, string factionKeyB, out string actual)
	{
		actual = string.Format("friendly %1 | enemy %2 | relation missing", ReportText(factionKeyA), ReportText(factionKeyB));
		if (factionKeyA.IsEmpty() || factionKeyB.IsEmpty() || GetGame() == null)
			return false;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return false;

		Faction factionA = factionManager.GetFactionByKey(factionKeyA);
		Faction factionB = factionManager.GetFactionByKey(factionKeyB);
		if (!factionA || !factionB)
			return false;

		bool aHostileB = factionA.IsFactionEnemy(factionB);
		bool bHostileA = factionB.IsFactionEnemy(factionA);
		actual = string.Format("friendly %1 hostileToEnemy %2 | enemy %3 hostileToFriendly %4", ReportText(factionKeyA), ReportBool(aHostileB), ReportText(factionKeyB), ReportBool(bHostileA));
		return aHostileB && bHostileA;
	}

	protected HST_ZoneState SelectCampaignDebugPhysicalCombatProbeZone(HST_CampaignState state, vector centerPosition)
	{
		HST_ZoneState nearestZone = FindNearestZone(state, centerPosition);
		if (nearestZone)
			return nearestZone;
		if (!state)
			return null;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone)
				return zone;
		}

		return null;
	}

	protected HST_ActiveGroupState CreateCampaignDebugPhysicalCombatProbeGroup(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState zone, string groupId, string factionKey, vector position, vector targetPosition)
	{
		if (!state || !zone || groupId.IsEmpty() || factionKey.IsEmpty())
			return null;

		HST_ActiveGroupState activeGroup = CreateActiveGroup(state, zone, factionKey, CAMPAIGN_DEBUG_COMBAT_PROBE_INFANTRY_COUNT, 0, false, preset);
		activeGroup.m_sGroupId = groupId;
		activeGroup.m_sZoneId = zone.m_sZoneId;
		activeGroup.m_sFactionKey = factionKey;
		activeGroup.m_sPrefab = SelectGroupPrefab(state, zone, factionKey, false, preset);
		activeGroup.m_sRouteId = "";
		activeGroup.m_vSourcePosition = position;
		activeGroup.m_vTargetPosition = targetPosition;
		activeGroup.m_vPosition = position;
		activeGroup.m_sRuntimeStatus = "campaign_debug_combat";
		activeGroup.m_iInfantryCount = CAMPAIGN_DEBUG_COMBAT_PROBE_INFANTRY_COUNT;
		activeGroup.m_iVehicleCount = 0;
		activeGroup.m_iLastSeenAliveCount = CAMPAIGN_DEBUG_COMBAT_PROBE_INFANTRY_COUNT;
		activeGroup.m_iSurvivorInfantryCount = CAMPAIGN_DEBUG_COMBAT_PROBE_INFANTRY_COUNT;
		activeGroup.m_iSurvivorVehicleCount = 0;
		activeGroup.m_bQRF = false;
		return activeGroup;
	}

	protected bool AssignCampaignDebugPhysicalCombatWaypoint(string groupId, vector targetPosition)
	{
		if (groupId.IsEmpty())
			return false;

		IEntity groupEntity = GetRuntimeCrewGroupEntity(groupId);
		AIGroup group = AIGroup.Cast(groupEntity);
		if (!group)
			return false;

		GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(CAMPAIGN_DEBUG_COMBAT_WAYPOINT_PREFAB, targetPosition, "0 0 0");
		AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
		if (!waypoint)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
			return false;
		}

		ApplyCampaignDebugEntityName(waypointEntity, "combat_waypoint", groupId);
		waypoint.SetCompletionRadius(CAMPAIGN_DEBUG_COMBAT_WAYPOINT_RADIUS_METERS);
		group.AddWaypoint(waypoint);
		m_aCampaignDebugCombatProbeWaypointEntities.Insert(waypointEntity);
		return true;
	}

	protected bool SampleCampaignDebugPhysicalCombatProbe(HST_CampaignState state, bool force = false)
	{
		if (!m_bCampaignDebugCombatProbeActive || !state)
			return false;
		if (!force && m_iCampaignDebugCombatProbeLastSampleSecond == state.m_iElapsedSeconds)
			return false;

		int previousFriendlyAlive = m_iCampaignDebugCombatProbeFriendlyEndAlive;
		int previousEnemyAlive = m_iCampaignDebugCombatProbeEnemyEndAlive;
		int friendlyAlive = CountAliveRuntimeGroupAgents(m_sCampaignDebugCombatProbeFriendlyGroupId);
		int enemyAlive = CountAliveRuntimeGroupAgents(m_sCampaignDebugCombatProbeEnemyGroupId);
		if (m_iCampaignDebugCombatProbeFriendlyStartAlive < 0 && friendlyAlive > 0)
			m_iCampaignDebugCombatProbeFriendlyStartAlive = friendlyAlive;
		if (m_iCampaignDebugCombatProbeEnemyStartAlive < 0 && enemyAlive > 0)
			m_iCampaignDebugCombatProbeEnemyStartAlive = enemyAlive;
		if (m_iCampaignDebugCombatProbeFriendlyStartAlive >= 0)
		{
			if (m_iCampaignDebugCombatProbeFriendlyMinAlive < 0 || friendlyAlive < m_iCampaignDebugCombatProbeFriendlyMinAlive)
				m_iCampaignDebugCombatProbeFriendlyMinAlive = friendlyAlive;
		}
		if (m_iCampaignDebugCombatProbeEnemyStartAlive >= 0)
		{
			if (m_iCampaignDebugCombatProbeEnemyMinAlive < 0 || enemyAlive < m_iCampaignDebugCombatProbeEnemyMinAlive)
				m_iCampaignDebugCombatProbeEnemyMinAlive = enemyAlive;
		}
		m_iCampaignDebugCombatProbeFriendlyEndAlive = friendlyAlive;
		m_iCampaignDebugCombatProbeEnemyEndAlive = enemyAlive;

		vector friendlyPosition = ResolveCampaignDebugPhysicalCombatGroupPosition(state, m_sCampaignDebugCombatProbeFriendlyGroupId);
		vector enemyPosition = ResolveCampaignDebugPhysicalCombatGroupPosition(state, m_sCampaignDebugCombatProbeEnemyGroupId);
		if (!IsZeroVector(friendlyPosition) && !IsZeroVector(enemyPosition))
			m_fCampaignDebugCombatProbeLastDistance = Math.Sqrt(DistanceSq2D(friendlyPosition, enemyPosition));

		m_iCampaignDebugCombatProbeSampleCount++;
		m_iCampaignDebugCombatProbeLastSampleSecond = state.m_iElapsedSeconds;
		string sampleText = string.Format("#%1 t+%2s friendly %3 enemy %4 distance %5m", m_iCampaignDebugCombatProbeSampleCount, Math.Max(0, state.m_iElapsedSeconds - m_iCampaignDebugCombatProbeStartSecond), friendlyAlive, enemyAlive, Math.Round(m_fCampaignDebugCombatProbeLastDistance));
		AppendCampaignDebugPhysicalCombatProbeHistory(sampleText);
		return previousFriendlyAlive != friendlyAlive || previousEnemyAlive != enemyAlive;
	}

	protected vector ResolveCampaignDebugPhysicalCombatGroupPosition(HST_CampaignState state, string groupId)
	{
		IEntity runtimeEntity = GetRuntimeGroupEntity(groupId);
		if (runtimeEntity)
			return runtimeEntity.GetOrigin();

		HST_ActiveGroupState activeGroup;
		if (state)
			activeGroup = state.FindActiveGroup(groupId);
		if (activeGroup)
			return activeGroup.m_vPosition;

		return "0 0 0";
	}

	protected void AppendCampaignDebugPhysicalCombatProbeHistory(string sampleText)
	{
		if (sampleText.IsEmpty())
			return;

		m_sCampaignDebugCombatProbeLastObserved = sampleText;
		if (m_iCampaignDebugCombatProbeHistoryCount >= 10)
			return;

		if (m_sCampaignDebugCombatProbeHistory.IsEmpty())
			m_sCampaignDebugCombatProbeHistory = sampleText;
		else
			m_sCampaignDebugCombatProbeHistory = m_sCampaignDebugCombatProbeHistory + " | " + sampleText;
		m_iCampaignDebugCombatProbeHistoryCount++;
	}

	protected bool CleanupCampaignDebugPhysicalCombatProbeRuntime(HST_CampaignState state)
	{
		foreach (IEntity waypointEntity : m_aCampaignDebugCombatProbeWaypointEntities)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
		}
		m_aCampaignDebugCombatProbeWaypointEntities.Clear();

		if (!m_sCampaignDebugCombatProbeFriendlyGroupId.IsEmpty())
		{
			DeleteRuntimeGroupEntity(m_sCampaignDebugCombatProbeFriendlyGroupId);
			RemoveActiveGroupStateForDebug(state, m_sCampaignDebugCombatProbeFriendlyGroupId);
		}
		if (!m_sCampaignDebugCombatProbeEnemyGroupId.IsEmpty())
		{
			DeleteRuntimeGroupEntity(m_sCampaignDebugCombatProbeEnemyGroupId);
			RemoveActiveGroupStateForDebug(state, m_sCampaignDebugCombatProbeEnemyGroupId);
		}

		bool friendlyClean = m_sCampaignDebugCombatProbeFriendlyGroupId.IsEmpty() || (!GetRuntimeGroupEntity(m_sCampaignDebugCombatProbeFriendlyGroupId) && (!state || !state.FindActiveGroup(m_sCampaignDebugCombatProbeFriendlyGroupId)));
		bool enemyClean = m_sCampaignDebugCombatProbeEnemyGroupId.IsEmpty() || (!GetRuntimeGroupEntity(m_sCampaignDebugCombatProbeEnemyGroupId) && (!state || !state.FindActiveGroup(m_sCampaignDebugCombatProbeEnemyGroupId)));
		return friendlyClean && enemyClean && m_aCampaignDebugCombatProbeWaypointEntities.Count() == 0;
	}

	protected string BuildPhysicalCombatCleanupActual(HST_CampaignState state, bool cleaned)
	{
		bool friendlyGroupRemaining = state && !m_sCampaignDebugCombatProbeFriendlyGroupId.IsEmpty() && state.FindActiveGroup(m_sCampaignDebugCombatProbeFriendlyGroupId) != null;
		bool enemyGroupRemaining = state && !m_sCampaignDebugCombatProbeEnemyGroupId.IsEmpty() && state.FindActiveGroup(m_sCampaignDebugCombatProbeEnemyGroupId) != null;
		bool friendlyRuntimeRemaining = !m_sCampaignDebugCombatProbeFriendlyGroupId.IsEmpty() && GetRuntimeGroupEntity(m_sCampaignDebugCombatProbeFriendlyGroupId) != null;
		bool enemyRuntimeRemaining = !m_sCampaignDebugCombatProbeEnemyGroupId.IsEmpty() && GetRuntimeGroupEntity(m_sCampaignDebugCombatProbeEnemyGroupId) != null;
		return string.Format("cleaned %1 | friendly state %2 runtime %3 | enemy state %4 runtime %5 | waypoints %6", ReportBool(cleaned), ReportBool(friendlyGroupRemaining), ReportBool(friendlyRuntimeRemaining), ReportBool(enemyGroupRemaining), ReportBool(enemyRuntimeRemaining), m_aCampaignDebugCombatProbeWaypointEntities.Count());
	}

	protected void ResetCampaignDebugPhysicalCombatProbeState()
	{
		m_bCampaignDebugCombatProbeActive = false;
		m_sCampaignDebugCombatProbeId = "";
		m_sCampaignDebugCombatProbeFriendlyGroupId = "";
		m_sCampaignDebugCombatProbeEnemyGroupId = "";
		m_sCampaignDebugCombatProbeFriendlyFaction = "";
		m_sCampaignDebugCombatProbeEnemyFaction = "";
		m_sCampaignDebugCombatProbeZoneId = "";
		m_iCampaignDebugCombatProbeStartSecond = 0;
		m_iCampaignDebugCombatProbeLastSampleSecond = -1;
		m_iCampaignDebugCombatProbeFriendlyStartAlive = -1;
		m_iCampaignDebugCombatProbeEnemyStartAlive = -1;
		m_iCampaignDebugCombatProbeFriendlyEndAlive = -1;
		m_iCampaignDebugCombatProbeEnemyEndAlive = -1;
		m_iCampaignDebugCombatProbeFriendlyMinAlive = -1;
		m_iCampaignDebugCombatProbeEnemyMinAlive = -1;
		m_iCampaignDebugCombatProbeSampleCount = 0;
		m_iCampaignDebugCombatProbeHistoryCount = 0;
		m_iCampaignDebugCombatProbeWaypointCount = 0;
		m_fCampaignDebugCombatProbeLastDistance = -1.0;
		m_vCampaignDebugCombatProbeCenter = "0 0 0";
		m_vCampaignDebugCombatProbeFriendlyPosition = "0 0 0";
		m_vCampaignDebugCombatProbeEnemyPosition = "0 0 0";
		m_sCampaignDebugCombatProbeHistory = "";
		m_sCampaignDebugCombatProbeLastObserved = "";
		m_sCampaignDebugCombatProbeStartResult = "";
		m_bCampaignDebugCombatProbeFriendlyPopulationResolved = false;
		m_bCampaignDebugCombatProbeEnemyPopulationResolved = false;
		m_sCampaignDebugCombatProbeFriendlyPopulationEvidence = "";
		m_sCampaignDebugCombatProbeEnemyPopulationEvidence = "";
	}

	protected HST_CampaignDebugCaseResult CreateConvoyDebugProbeCase(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_CampaignDebugCaseResult probe = new HST_CampaignDebugCaseResult();
		string instanceId = "missing";
		string missionId = "missing";
		if (mission)
		{
			instanceId = mission.m_sInstanceId;
			missionId = mission.m_sMissionId;
		}

		probe.m_sCaseId = "convoy_physical." + missionId + "." + instanceId;
		probe.m_sCategory = "mission_runtime";
		probe.m_sFeature = "convoy_intercept";
		probe.m_sStage = "physical_probe";
		probe.m_sStatus = "PASS";
		if (state)
		{
			probe.m_iStartSecond = state.m_iElapsedSeconds;
			probe.m_iEndSecond = state.m_iElapsedSeconds;
		}

		return probe;
	}

	protected HST_CampaignDebugAssertion AddConvoyDebugProbeAssertion(HST_CampaignDebugCaseResult probe, string assertionId, string expected, string actual, string status, string failureReason = "", string entityId = "", string missionInstanceId = "", string zoneId = "", string orderId = "")
	{
		if (!probe)
			return null;

		HST_CampaignDebugAssertion assertion = new HST_CampaignDebugAssertion();
		assertion.m_sAssertionId = assertionId;
		assertion.m_sExpected = expected;
		assertion.m_sActual = actual;
		assertion.m_sStatus = status;
		assertion.m_sFailureReason = failureReason;
		assertion.m_sEntityId = entityId;
		assertion.m_sMissionInstanceId = missionInstanceId;
		assertion.m_sZoneId = zoneId;
		assertion.m_sOrderId = orderId;
		probe.m_aAssertions.Insert(assertion);
		return assertion;
	}

	protected void AddConvoyDebugProbeMetric(HST_CampaignDebugCaseResult probe, string metricId, string value, string unit = "")
	{
		if (!probe)
			return;

		HST_CampaignDebugMetric metric = new HST_CampaignDebugMetric();
		metric.m_sMetricId = metricId;
		metric.m_sName = metricId;
		metric.m_sValue = value;
		metric.m_sUnit = unit;
		metric.m_sFeature = probe.m_sFeature;
		metric.m_sStage = probe.m_sStage;
		metric.m_sMissionInstanceId = ResolveConvoyDebugProbeMissionInstanceId(probe);
		probe.m_aMetrics.Insert(metric);
	}

	protected string ResolveConvoyDebugProbeMissionInstanceId(HST_CampaignDebugCaseResult probe)
	{
		if (!probe)
			return "";

		foreach (HST_CampaignDebugAssertion assertion : probe.m_aAssertions)
		{
			if (assertion && !assertion.m_sMissionInstanceId.IsEmpty())
				return assertion.m_sMissionInstanceId;
		}

		return "";
	}

	protected void FinalizeConvoyDebugProbeCase(HST_CampaignState state, HST_CampaignDebugCaseResult probe)
	{
		if (!probe)
			return;

		string resolvedStatus = "PASS";
		string resolvedReason;
		foreach (HST_CampaignDebugAssertion assertion : probe.m_aAssertions)
		{
			if (!assertion)
				continue;

			if (assertion.m_sStatus == "FAIL")
			{
				resolvedStatus = "FAIL";
				resolvedReason = assertion.m_sFailureReason;
				break;
			}

			if (assertion.m_sStatus == "BLOCKED" && resolvedStatus != "FAIL")
			{
				resolvedStatus = "BLOCKED";
				if (resolvedReason.IsEmpty())
					resolvedReason = assertion.m_sFailureReason;
			}
			else if (assertion.m_sStatus == "WARN" && resolvedStatus == "PASS")
			{
				resolvedStatus = "WARN";
				if (resolvedReason.IsEmpty())
					resolvedReason = assertion.m_sFailureReason;
			}
			else if (assertion.m_sStatus == "SKIPPED" && resolvedStatus == "PASS")
			{
				resolvedStatus = "SKIPPED";
				if (resolvedReason.IsEmpty())
					resolvedReason = assertion.m_sFailureReason;
			}
		}

		probe.m_sStatus = resolvedStatus;
		if (resolvedReason.IsEmpty())
			resolvedReason = "convoy physical probe assertions passed";
		probe.m_sReason = resolvedReason;
		if (state)
			probe.m_iEndSecond = state.m_iElapsedSeconds;
	}

	protected string ConvoyDebugStatus(bool passed, string failStatus = "FAIL")
	{
		if (passed)
			return "PASS";

		return failStatus;
	}

	protected HST_CampaignDebugCaseResult CreateExpiredConvoyRenderBubbleDebugCase(HST_CampaignState state)
	{
		HST_CampaignDebugCaseResult probe = new HST_CampaignDebugCaseResult();
		probe.m_sCaseId = "render_bubble.convoy.expired_contact";
		probe.m_sCategory = "physical_war";
		probe.m_sFeature = "render_bubble";
		probe.m_sStage = "early_mechanics";
		probe.m_sStatus = "PASS";
		if (state)
		{
			probe.m_iStartSecond = state.m_iElapsedSeconds;
			probe.m_iEndSecond = state.m_iElapsedSeconds;
		}
		probe.m_aEvidence.Insert("temporary expired convoy contact groups exercise player-bubble preserve/delete cleanup policy and are removed before the case is recorded");
		return probe;
	}

	protected void FinalizeExpiredConvoyRenderBubbleDebugCase(HST_CampaignState state, HST_CampaignDebugCaseResult probe)
	{
		FinalizeConvoyDebugProbeCase(state, probe);
		if (probe && probe.m_sStatus == "PASS")
			probe.m_sReason = "expired convoy render-bubble probe assertions passed";
	}

	protected string ResolveExpiredConvoyRenderBubbleDebugPrefix(string debugPrefix)
	{
		if (debugPrefix.IsEmpty())
			return CAMPAIGN_DEBUG_PREFIX_ROOT + "render_bubble";
		if (debugPrefix.Contains(CAMPAIGN_DEBUG_PREFIX_ROOT))
			return debugPrefix;

		return CAMPAIGN_DEBUG_PREFIX_ROOT + debugPrefix;
	}

	protected HST_CampaignDebugCaseResult CreateConvoyPhaseChainDebugCase(HST_CampaignState state)
	{
		HST_CampaignDebugCaseResult probe = new HST_CampaignDebugCaseResult();
		probe.m_sCaseId = "convoy_phase_chain.controlled_state_machine";
		probe.m_sCategory = "mission_runtime";
		probe.m_sFeature = "convoy_intercept";
		probe.m_sStage = "controlled_phase_chain";
		probe.m_sStatus = "PASS";
		if (state)
		{
			probe.m_iStartSecond = state.m_iElapsedSeconds;
			probe.m_iEndSecond = state.m_iElapsedSeconds;
		}
		probe.m_aEvidence.Insert("controlled phase-chain probe proves convoy transition helpers on temporary debug records only; it does not replace natural movement evidence");
		return probe;
	}

	protected void FinalizeConvoyPhaseChainDebugCase(HST_CampaignState state, HST_CampaignDebugCaseResult probe)
	{
		FinalizeConvoyDebugProbeCase(state, probe);
		if (probe && probe.m_sStatus == "PASS")
			probe.m_sReason = "controlled convoy phase-chain assertions passed";
	}

	protected string ResolveConvoyPhaseChainDebugPrefix(string debugPrefix)
	{
		if (debugPrefix.IsEmpty())
			return CAMPAIGN_DEBUG_PREFIX_ROOT + "convoy_phase_chain";
		if (debugPrefix.Contains(CAMPAIGN_DEBUG_PREFIX_ROOT))
			return debugPrefix;

		return CAMPAIGN_DEBUG_PREFIX_ROOT + debugPrefix;
	}

	protected HST_ActiveMissionState CreateConvoyPhaseChainDebugMission(HST_CampaignState state, string instanceId)
	{
		vector targetPosition = "100 0 100";
		if (state && !IsZeroVector(state.m_vHQPosition))
		{
			targetPosition = state.m_vHQPosition;
			targetPosition[0] = targetPosition[0] + 250.0;
			targetPosition[2] = targetPosition[2] + 250.0;
		}

		HST_ActiveMissionState mission = new HST_ActiveMissionState();
		mission.m_sInstanceId = instanceId;
		mission.m_sMissionId = "campaign_debug_convoy_phase_chain";
		mission.m_sDisplayName = "Campaign Debug Convoy Phase Chain";
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_ACTIVE;
		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_PHYSICAL_MVP;
		mission.m_sRuntimePrimitive = MISSION_CONVOY_PRIMITIVE;
		mission.m_sRuntimeType = "convoy_intercept";
		mission.m_vTargetPosition = targetPosition;
		mission.m_iRuntimeCounterA = 0;
		mission.m_iRuntimeCounterB = 30;
		mission.m_iRequiredVehicleCount = 3;
		mission.m_bRuntimeSpawned = true;
		return mission;
	}

	protected HST_MissionObjectiveState CreateConvoyPhaseChainDebugObjective(HST_ActiveMissionState mission)
	{
		HST_MissionObjectiveState objective = new HST_MissionObjectiveState();
		objective.m_sObjectiveId = "objective_" + mission.m_sInstanceId + "_convoy";
		objective.m_sMissionInstanceId = mission.m_sInstanceId;
		objective.m_eType = HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA;
		objective.m_sLabel = "Neutralize convoy crew";
		objective.m_sRequirementText = "Neutralize the controlled debug convoy crew records.";
		objective.m_sTargetId = "convoy";
		objective.m_sRuntimePrimitive = MISSION_CONVOY_PRIMITIVE;
		objective.m_vPosition = mission.m_vTargetPosition;
		objective.m_iRequiredProgress = 3;
		objective.m_iRequiredCount = 3;
		return objective;
	}

	protected HST_MissionAssetState CreateConvoyPhaseChainDebugVehicleAsset(HST_ActiveMissionState mission, int index)
	{
		vector sourcePosition = mission.m_vTargetPosition;
		sourcePosition[0] = sourcePosition[0] + 300.0 + (index * 18.0);
		sourcePosition[2] = sourcePosition[2] + 90.0 + (index * 12.0);

		HST_MissionAssetState asset = new HST_MissionAssetState();
		asset.m_sAssetId = string.Format("%1_vehicle_%2", mission.m_sInstanceId, index);
		asset.m_sMissionInstanceId = mission.m_sInstanceId;
		asset.m_sKind = "vehicle";
		asset.m_sRole = MISSION_CONVOY_VEHICLE_ROLE;
		asset.m_sPrefab = "campaign_debug_convoy_vehicle";
		asset.m_bSpawned = true;
		asset.m_bAlive = true;
		asset.m_vSourcePosition = sourcePosition;
		asset.m_vCurrentPosition = sourcePosition;
		asset.m_vLastKnownPosition = sourcePosition;
		asset.m_vTargetPosition = mission.m_vTargetPosition;
		return asset;
	}

	protected HST_ActiveGroupState CreateConvoyPhaseChainDebugGroup(HST_ActiveMissionState mission, HST_MissionAssetState asset, int index, int elapsedSeconds)
	{
		HST_ActiveGroupState activeGroup = new HST_ActiveGroupState();
		activeGroup.m_sGroupId = BuildMissionConvoyGroupId(mission, index);
		activeGroup.m_sFactionKey = "USSR";
		activeGroup.m_sPrefab = "campaign_debug_convoy_crew";
		activeGroup.m_vPosition = asset.m_vCurrentPosition;
		activeGroup.m_vSourcePosition = asset.m_vSourcePosition;
		activeGroup.m_vTargetPosition = asset.m_vTargetPosition;
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = MISSION_CONVOY_STAGING;
		activeGroup.m_sRouteId = mission.m_sInstanceId + "_route";
		activeGroup.m_sSpawnFallbackMode = "convoy_waypoints";
		activeGroup.m_iInfantryCount = 1;
		activeGroup.m_iVehicleCount = 1;
		activeGroup.m_iSpawnedAtSecond = elapsedSeconds;
		activeGroup.m_iLastSeenAliveCount = 1;
		activeGroup.m_iSurvivorInfantryCount = 1;
		activeGroup.m_iSpawnedAgentCount = 1;
		activeGroup.m_iAssignedWaypointCount = 3;
		activeGroup.m_bSpawnAttempted = true;
		activeGroup.m_bSpawnedEntity = true;
		return activeGroup;
	}

	protected string AppendConvoyPhaseChainObservedPhase(string observedPhases, string phase)
	{
		if (phase.IsEmpty())
			phase = "none";
		if (observedPhases.IsEmpty())
			return phase;

		return observedPhases + " -> " + phase;
	}

	protected string BuildConvoyPhaseChainActual(HST_ActiveMissionState mission, string observedPhases)
	{
		if (!mission)
			return "mission missing";

		return string.Format("phase %1 | event %2 | reason %3 | observed %4", ReportText(mission.m_sRuntimePhase), ReportText(mission.m_sLastRuntimeEventKey), ReportText(mission.m_sRuntimeFailureReason), ReportText(observedPhases));
	}

	protected string BuildConvoyPhaseChainGroupActual(HST_CampaignState state, HST_ActiveMissionState mission, string observedPhases, string expectedStatus)
	{
		return string.Format("%1 | groups %2/3 %3", BuildConvoyPhaseChainActual(mission, observedPhases), CountConvoyPhaseChainDebugGroupsWithStatus(state, mission, expectedStatus), ReportText(expectedStatus));
	}

	protected void MarkConvoyPhaseChainDebugGroupsEliminated(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroupForMission(activeGroup, mission))
				continue;

			activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
			activeGroup.m_iLastSeenAliveCount = 0;
			activeGroup.m_iSurvivorInfantryCount = 0;
			activeGroup.m_iSpawnedAgentCount = Math.Max(1, activeGroup.m_iSpawnedAgentCount);
			activeGroup.m_bEverHadLivingCrew = true;
			activeGroup.m_iMaxObservedCrewAlive = Math.Max(activeGroup.m_iMaxObservedCrewAlive, 1);
		}
	}

	protected int CountConvoyPhaseChainDebugVehicleAssets(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return 0;

		int count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == instanceId && asset.m_sRole == MISSION_CONVOY_VEHICLE_ROLE)
				count++;
		}

		return count;
	}

	protected int CountConvoyPhaseChainDebugGroups(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return 0;

		int count;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (IsMissionConvoyGroupForMission(activeGroup, mission))
				count++;
		}

		return count;
	}

	protected int CountConvoyPhaseChainDebugGroupsWithStatus(HST_CampaignState state, HST_ActiveMissionState mission, string status)
	{
		if (!state || !mission || status.IsEmpty())
			return 0;

		int count;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (IsMissionConvoyGroupForMission(activeGroup, mission) && activeGroup.m_sRuntimeStatus == status)
				count++;
		}

		return count;
	}

	protected bool RemoveConvoyPhaseChainDebugRecords(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return false;

		HST_ActiveMissionState mission = state.FindActiveMission(instanceId);
		for (int groupNumber = 0; groupNumber < 3; groupNumber++)
		{
			string groupId;
			if (mission)
				groupId = BuildMissionConvoyGroupId(mission, groupNumber);
			else
				groupId = string.Format("%1%2_%3", MISSION_CONVOY_GROUP_PREFIX, instanceId, groupNumber);

			DeleteRuntimeGroupEntity(groupId);
			RemoveConvoyProgressStatusForGroup(groupId);
			RemoveActiveGroupStateForDebug(state, groupId);
		}

		for (int missionIndex = state.m_aActiveMissions.Count() - 1; missionIndex >= 0; missionIndex--)
		{
			HST_ActiveMissionState activeMission = state.m_aActiveMissions[missionIndex];
			if (activeMission && activeMission.m_sInstanceId == instanceId)
				state.m_aActiveMissions.Remove(missionIndex);
		}

		for (int objectiveIndex = state.m_aMissionObjectives.Count() - 1; objectiveIndex >= 0; objectiveIndex--)
		{
			HST_MissionObjectiveState objective = state.m_aMissionObjectives[objectiveIndex];
			if (objective && objective.m_sMissionInstanceId == instanceId)
				state.m_aMissionObjectives.Remove(objectiveIndex);
		}

		for (int runtimeIndex = state.m_aMissionRuntimeEntities.Count() - 1; runtimeIndex >= 0; runtimeIndex--)
		{
			HST_MissionRuntimeEntityState runtimeEntity = state.m_aMissionRuntimeEntities[runtimeIndex];
			if (runtimeEntity && runtimeEntity.m_sMissionInstanceId == instanceId)
				state.m_aMissionRuntimeEntities.Remove(runtimeIndex);
		}

		for (int assetIndex = state.m_aMissionAssets.Count() - 1; assetIndex >= 0; assetIndex--)
		{
			HST_MissionAssetState asset = state.m_aMissionAssets[assetIndex];
			if (asset && asset.m_sMissionInstanceId == instanceId)
				state.m_aMissionAssets.Remove(assetIndex);
		}

		return AreConvoyPhaseChainDebugRecordsRemoved(state, instanceId);
	}

	protected bool AreConvoyPhaseChainDebugRecordsRemoved(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return false;
		if (state.FindActiveMission(instanceId))
			return false;
		if (CountConvoyPhaseChainDebugVehicleAssets(state, instanceId) > 0)
			return false;

		for (int groupNumber = 0; groupNumber < 3; groupNumber++)
		{
			string groupId = string.Format("%1%2_%3", MISSION_CONVOY_GROUP_PREFIX, instanceId, groupNumber);
			if (state.FindActiveGroup(groupId) || GetRuntimeGroupEntity(groupId) || GetRuntimeVehicleEntity(groupId))
				return false;
		}

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == instanceId)
				return false;
		}

		foreach (HST_MissionRuntimeEntityState runtimeEntity : state.m_aMissionRuntimeEntities)
		{
			if (runtimeEntity && runtimeEntity.m_sMissionInstanceId == instanceId)
				return false;
		}

		return true;
	}

	protected string BuildConvoyPhaseChainCleanupActual(bool cleaned, HST_CampaignState state, string instanceId)
	{
		bool missionRemaining = state && state.FindActiveMission(instanceId) != null;
		return string.Format("cleaned %1 | mission remaining %2 | assets %3", ReportBool(cleaned), ReportBool(missionRemaining), CountConvoyPhaseChainDebugVehicleAssets(state, instanceId));
	}

	protected HST_ActiveMissionState CreateExpiredConvoyRenderBubbleDebugMission(string instanceId, vector position)
	{
		HST_ActiveMissionState mission = new HST_ActiveMissionState();
		mission.m_sInstanceId = instanceId;
		mission.m_sMissionId = "campaign_debug_expired_convoy";
		mission.m_sDisplayName = "Campaign Debug Expired Convoy";
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_PHYSICAL_MVP;
		mission.m_sRuntimePrimitive = MISSION_CONVOY_PRIMITIVE;
		mission.m_sRuntimeType = "convoy_intercept";
		mission.m_sRuntimePhase = MISSION_CONVOY_CONTACT;
		mission.m_vTargetPosition = position;
		return mission;
	}

	protected HST_ActiveGroupState CreateExpiredConvoyRenderBubbleDebugGroup(HST_ActiveMissionState mission, vector position)
	{
		HST_ActiveGroupState activeGroup = new HST_ActiveGroupState();
		activeGroup.m_sGroupId = BuildMissionConvoyGroupId(mission, 0);
		activeGroup.m_sFactionKey = "USSR";
		activeGroup.m_sPrefab = CAMPAIGN_DEBUG_TEMP_ENTITY_PREFAB;
		activeGroup.m_vPosition = position;
		activeGroup.m_vSourcePosition = position;
		activeGroup.m_vTargetPosition = position;
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = MISSION_CONVOY_CONTACT;
		activeGroup.m_iInfantryCount = 1;
		activeGroup.m_iLastSeenAliveCount = 1;
		activeGroup.m_iSurvivorInfantryCount = 1;
		activeGroup.m_iSpawnedAgentCount = 1;
		activeGroup.m_bSpawnAttempted = true;
		activeGroup.m_bSpawnedEntity = true;
		return activeGroup;
	}

	protected void RegisterExpiredConvoyRenderBubbleDebugRuntime(HST_CampaignState state, HST_ActiveMissionState mission, HST_ActiveGroupState activeGroup, IEntity crewEntity)
	{
		if (!state || !mission || !activeGroup || !crewEntity)
			return;

		state.m_aActiveMissions.Insert(mission);
		state.m_aActiveGroups.Insert(activeGroup);
		m_aRuntimeGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeGroupEntities.Insert(crewEntity);
	}

	protected IEntity SpawnExpiredConvoyRenderBubbleDebugCrewEntity(vector position, string groupId)
	{
		GenericEntity tempEntity = HST_WorldPositionService.SpawnPrefab(CAMPAIGN_DEBUG_TEMP_ENTITY_PREFAB, position, "0 0 0");
		if (!tempEntity)
			return null;

		ApplyCampaignDebugEntityName(tempEntity, "expired_convoy_crew", groupId);
		return tempEntity;
	}

	protected bool CleanupInactiveMissionConvoyRuntimeGroupForDebug(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return false;
		if (!IsMissionConvoyGroup(activeGroup))
			return false;
		if (HasActiveMissionForConvoyGroup(state, activeGroup))
			return false;

		if (ShouldKeepExpiredEngagedConvoyRuntime(state, activeGroup))
			return MarkExpiredEngagedConvoyRuntimePreserved(activeGroup);

		DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
		RemoveConvoyProgressStatusForGroup(activeGroup.m_sGroupId);
		RemoveActiveGroupStateForDebug(state, activeGroup.m_sGroupId);
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected bool RemoveExpiredConvoyRenderBubbleDebugRecords(HST_CampaignState state, string instanceId, string groupId)
	{
		if (!state)
			return false;

		if (!groupId.IsEmpty())
		{
			DeleteRuntimeGroupEntity(groupId);
			RemoveConvoyProgressStatusForGroup(groupId);
			RemoveActiveGroupStateForDebug(state, groupId);
		}

		for (int missionIndex = state.m_aActiveMissions.Count() - 1; missionIndex >= 0; missionIndex--)
		{
			HST_ActiveMissionState mission = state.m_aActiveMissions[missionIndex];
			if (mission && mission.m_sInstanceId == instanceId)
				state.m_aActiveMissions.Remove(missionIndex);
		}

		bool groupClear = groupId.IsEmpty() || state.FindActiveGroup(groupId) == null;
		bool missionClear = instanceId.IsEmpty() || state.FindActiveMission(instanceId) == null;
		bool runtimeClear = groupId.IsEmpty() || GetRuntimeGroupEntity(groupId) == null;
		return groupClear && missionClear && runtimeClear;
	}

	protected void RemoveActiveGroupStateForDebug(HST_CampaignState state, string groupId)
	{
		if (!state || groupId.IsEmpty())
			return;

		for (int groupIndex = state.m_aActiveGroups.Count() - 1; groupIndex >= 0; groupIndex--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[groupIndex];
			if (activeGroup && activeGroup.m_sGroupId == groupId)
				state.m_aActiveGroups.Remove(groupIndex);
		}
	}

	protected bool ResolveFirstLivingPlayerDebugPosition(out vector playerPosition)
	{
		playerPosition = "0 0 0";
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			playerPosition = playerEntity.GetOrigin();
			return true;
		}

		return false;
	}

	protected vector ResolveExpiredConvoyRenderBubbleDebugFarPosition(vector playerPosition)
	{
		float minimumDistance = EXPIRED_CONVOY_PLAYER_RENDER_BUBBLE_METERS + 500.0;
		vector farPosition = playerPosition;
		farPosition[0] = farPosition[0] + minimumDistance;
		return farPosition;
	}

	protected string BuildExpiredConvoyRenderBubbleDebugActual(string groupId, bool cleanupChanged, bool groupPreserved, bool runtimePreserved, int aliveCrew, float distanceMeters, string fallbackMode, string reason)
	{
		string actual = string.Format("group %1 | changed %2 | group present %3 | runtime present %4", ReportText(groupId), ReportBool(cleanupChanged), ReportBool(groupPreserved), ReportBool(runtimePreserved));
		actual = actual + string.Format(" | alive crew %1 | distance %2m | mode %3 | reason %4", aliveCrew, Math.Round(distanceMeters), ReportText(fallbackMode), ReportText(reason));
		return actual;
	}

	protected string BuildExpiredConvoyRenderBubbleCleanupActual(bool nearRecordsRemoved, bool farRecordsRemoved)
	{
		return string.Format("near clear %1 | far clear %2", ReportBool(nearRecordsRemoved), ReportBool(farRecordsRemoved));
	}

	protected string BuildConvoyDebugRouteActual(HST_GeneratedRouteState route)
	{
		if (!route)
			return "route missing";

		return string.Format("route %1 | waypoints %2 | distance %3m | road %4 | vehicle-safe %5", ReportText(route.m_sRouteId), route.m_iWaypointCount, route.m_iDistanceMeters, ReportBool(route.m_bRoadRoute), ReportBool(route.m_bValidatedForVehicles));
	}

	protected string BuildActiveGroupRuntimeFactionActual(HST_ActiveGroupState activeGroup, int mismatchCount, string sample, bool groupEntityPresent, bool vehicleEntityPresent)
	{
		if (!activeGroup)
			return "group missing";

		string groupRootFaction = ResolveActiveGroupRuntimeRootFactionKey(activeGroup);
		int liveMemberCount = CountRuntimeGroupControlledEntities(activeGroup.m_sGroupId);
		string actual = string.Format("group %1 | expected %2 | prefab %3 | group root faction %4 | live members %5 | group entity %6 | vehicle entity %7 | mismatches %8 | visual %9", ReportText(activeGroup.m_sGroupId), ReportText(activeGroup.m_sFactionKey), ReportText(activeGroup.m_sPrefab), ReportText(groupRootFaction), liveMemberCount, ReportBool(groupEntityPresent), ReportBool(vehicleEntityPresent), mismatchCount, ReportText(BuildActiveGroupRuntimeVisualEvidence(activeGroup.m_sGroupId)));
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		if (vehicleEntity)
			actual = actual + string.Format(" | vehicle claimed faction %1", ReportText(HST_VehicleRootPolicy.ResolveVehicleFactionKey(vehicleEntity)));
		return actual + string.Format(" | sample %1", ReportText(sample));
	}

	int CountCampaignDebugRuntimeFactionMismatches(HST_CampaignState state, out string evidence)
	{
		evidence = "state missing";
		if (!state)
			return 0;

		int runtimeGroupCount;
		int checkedGroupCount;
		int pendingLiveCountGroups;
		int skippedTerminalEmptyGroups;
		int skippedPendingPopulationGroups;
		int mismatchCount;
		string firstMismatch;
		string firstPending;
		string firstTerminalEmpty;
		string firstSkippedPendingPopulation;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sGroupId.IsEmpty() || activeGroup.m_sFactionKey.IsEmpty())
				continue;

			bool groupEntityPresent = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) != null;
			bool vehicleEntityPresent = GetRuntimeVehicleEntity(activeGroup.m_sGroupId) != null;
			if (!groupEntityPresent && !vehicleEntityPresent)
				continue;

			if (activeGroup.m_sRuntimeStatus == "spawn_pending_agents" || activeGroup.m_sRuntimeStatus == ACTIVE_GROUP_RUNTIME_STATUS_AIWORLD_BUDGET_DEFERRED)
			{
				skippedPendingPopulationGroups++;
				if (firstSkippedPendingPopulation.IsEmpty())
					firstSkippedPendingPopulation = activeGroup.m_sGroupId;
				continue;
			}

			int liveControlledMembers = CountRuntimeGroupControlledEntities(activeGroup.m_sGroupId);
			int liveRuntimeEntities = CountAliveRuntimeGroupAgents(activeGroup.m_sGroupId);
			if (IsTerminalActiveGroupRuntimeStatus(activeGroup) && liveControlledMembers <= 0 && liveRuntimeEntities <= 0)
			{
				skippedTerminalEmptyGroups++;
				if (firstTerminalEmpty.IsEmpty())
					firstTerminalEmpty = activeGroup.m_sGroupId;
				continue;
			}

			runtimeGroupCount++;
			EnsureActiveGroupRuntimeFaction(activeGroup, "campaign debug faction audit");

			string sample;
			int activeGroupMismatches = CountActiveGroupRuntimeFactionMismatches(activeGroup, sample);
			if (activeGroup.m_iInfantryCount > 0 && groupEntityPresent && liveControlledMembers <= 0)
			{
				if (IsActiveGroupLiveCountGraceActive(state, activeGroup))
				{
					pendingLiveCountGroups++;
					if (firstPending.IsEmpty())
						firstPending = activeGroup.m_sGroupId;

					activeGroupMismatches++;
					if (sample.IsEmpty())
						sample = "pending live controlled members for infantry group";
				}
				else
				{
					activeGroupMismatches++;
					if (sample.IsEmpty())
						sample = "zero live controlled members for infantry group";
				}
			}
			checkedGroupCount++;
			mismatchCount += activeGroupMismatches;
			if (activeGroupMismatches > 0 && firstMismatch.IsEmpty())
				firstMismatch = BuildActiveGroupRuntimeFactionActual(activeGroup, activeGroupMismatches, sample, groupEntityPresent, vehicleEntityPresent);
		}

		evidence = string.Format("runtime groups %1 | checked %2 | mismatches %3 | pending live-count %4 | skipped terminal empty %5 | skipped pending population %6", runtimeGroupCount, checkedGroupCount, mismatchCount, pendingLiveCountGroups, skippedTerminalEmptyGroups, skippedPendingPopulationGroups);
		evidence = evidence + string.Format(" | first %1", ReportText(firstMismatch));
		if (!firstPending.IsEmpty() || !firstTerminalEmpty.IsEmpty() || !firstSkippedPendingPopulation.IsEmpty())
			evidence = evidence + string.Format(" | first pending %1 | first terminal empty %2 | first skipped pending %3", ReportText(firstPending), ReportText(firstTerminalEmpty), ReportText(firstSkippedPendingPopulation));
		return mismatchCount;
	}

	int CountCampaignDebugDirectFallbackActiveGroups(HST_CampaignState state, out string evidence)
	{
		evidence = "state missing";
		if (!state)
			return -1;

		int checkedCount;
		int fallbackCount;
		int terminalCount;
		string firstFallback;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sGroupId.IsEmpty() || activeGroup.m_iInfantryCount <= 0)
				continue;

			if (!IsDirectInfantryFallbackMode(activeGroup.m_sSpawnFallbackMode))
				continue;

			checkedCount++;
			if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
				terminalCount++;

			fallbackCount++;
			if (firstFallback.IsEmpty())
				firstFallback = BuildActiveGroupDirectFallbackActual(activeGroup);
		}

		evidence = string.Format("checked fallback-tagged %1 | direct fallback groups %2 | terminal fallback groups %3 | first %4", checkedCount, fallbackCount, terminalCount, ReportText(firstFallback));
		return fallbackCount;
	}

	int CountCampaignDebugPendingPopulationActiveGroups(HST_CampaignState state, out string evidence)
	{
		evidence = "state missing";
		if (!state)
			return -1;

		int pendingCount;
		int nativePendingCount;
		int aiWorldDeferredCount;
		string firstPending;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sGroupId.IsEmpty() || activeGroup.m_iInfantryCount <= 0)
				continue;
			if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
				continue;

			bool nativePending = activeGroup.m_sRuntimeStatus == "spawn_pending_agents";
			bool aiWorldDeferred = activeGroup.m_sRuntimeStatus == ACTIVE_GROUP_RUNTIME_STATUS_AIWORLD_BUDGET_DEFERRED;
			if (!nativePending && !aiWorldDeferred)
				continue;

			pendingCount++;
			if (nativePending)
				nativePendingCount++;
			if (aiWorldDeferred)
				aiWorldDeferredCount++;
			if (firstPending.IsEmpty())
				firstPending = BuildActiveGroupPendingPopulationActual(activeGroup);
		}

		evidence = string.Format("pending population groups %1 | native pending %2 | aiworld deferred %3 | first %4", pendingCount, nativePendingCount, aiWorldDeferredCount, ReportText(firstPending));
		return pendingCount;
	}

	int CampaignDebugResolvePendingPopulationActiveGroups(HST_CampaignState state, out string evidence)
	{
		evidence = "state missing";
		if (!state)
			return -1;

		int attemptedCount;
		int resolvedCount;
		int unresolvedCount;
		int skippedDeferredCount;
		string firstResolved;
		string firstUnresolved;
		string firstDeferred;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sGroupId.IsEmpty() || activeGroup.m_iInfantryCount <= 0)
				continue;
			if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
				continue;
			if (activeGroup.m_sRuntimeStatus == ACTIVE_GROUP_RUNTIME_STATUS_AIWORLD_BUDGET_DEFERRED)
			{
				skippedDeferredCount++;
				if (firstDeferred.IsEmpty())
					firstDeferred = activeGroup.m_sGroupId;
				continue;
			}
			if (activeGroup.m_sRuntimeStatus != "spawn_pending_agents")
				continue;

			attemptedCount++;
			string requestedStatus = ResolvePendingActiveGroupRequestedStatus(activeGroup, "");
			string groupEvidence;
			bool resolved = CampaignDebugResolvePendingActiveGroupPopulation(activeGroup, state, requestedStatus, groupEvidence);
			if (resolved)
			{
				resolvedCount++;
				if (firstResolved.IsEmpty())
					firstResolved = string.Format("%1 | %2", activeGroup.m_sGroupId, groupEvidence);
			}
			else
			{
				unresolvedCount++;
				if (firstUnresolved.IsEmpty())
					firstUnresolved = string.Format("%1 | %2", activeGroup.m_sGroupId, groupEvidence);
			}
		}

		evidence = string.Format("attempted %1 | resolved %2 | unresolved %3 | deferred %4", attemptedCount, resolvedCount, unresolvedCount, skippedDeferredCount);
		evidence = evidence + string.Format(" | first resolved %1 | first unresolved %2", ReportText(firstResolved), ReportText(firstUnresolved));
		if (!firstDeferred.IsEmpty())
			evidence = evidence + string.Format(" | first deferred %1", ReportText(firstDeferred));
		return resolvedCount;
	}

	protected string BuildActiveGroupPendingPopulationActual(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "missing";

		bool groupEntityPresent = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) != null;
		int liveMembers = CountRuntimeGroupControlledEntities(activeGroup.m_sGroupId);
		string actual = string.Format("group %1 | zone %2 | faction %3 | prefab %4 | status %5 | mode %6 | spawned %7 | root %8 | live %9",
			ReportText(activeGroup.m_sGroupId),
			ReportText(activeGroup.m_sZoneId),
			ReportText(activeGroup.m_sFactionKey),
			ReportText(activeGroup.m_sPrefab),
			ReportText(activeGroup.m_sRuntimeStatus),
			ReportText(activeGroup.m_sSpawnFallbackMode),
			ReportBool(activeGroup.m_bSpawnedEntity),
			ReportBool(groupEntityPresent),
			liveMembers);
		return actual + string.Format("/%1 | reason %2 | visual %3",
			activeGroup.m_iInfantryCount,
			ReportText(activeGroup.m_sSpawnFailureReason),
			ReportText(BuildActiveGroupRuntimeVisualEvidence(activeGroup.m_sGroupId)));
	}

	protected string ResolvePendingActiveGroupRequestedStatus(HST_ActiveGroupState activeGroup, string fallbackStatus)
	{
		if (!activeGroup)
			return fallbackStatus;

		int pendingIndex = FindPendingActiveGroupPopulationIndex(activeGroup.m_sGroupId);
		if (pendingIndex >= 0 && pendingIndex < m_aPendingPopulationRequestedStatuses.Count())
		{
			string requestedStatus = m_aPendingPopulationRequestedStatuses[pendingIndex];
			if (!requestedStatus.IsEmpty())
				return requestedStatus;
		}

		if (!fallbackStatus.IsEmpty())
			return fallbackStatus;
		if (!activeGroup.m_sSupportRequestId.IsEmpty())
			return "support_active";
		if (activeGroup.m_bQRF || !activeGroup.m_sQRFInstanceId.IsEmpty())
			return "routing";
		if (!activeGroup.m_sMissionInstanceId.IsEmpty())
			return "active";

		return "active";
	}

	protected string BuildActiveGroupDirectFallbackActual(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "missing";

		int liveMembers = CountRuntimeGroupControlledEntities(activeGroup.m_sGroupId);
		string actual = string.Format("group %1 | zone %2 | faction %3 | prefab %4 | mode %5 | status %6 | spawned %7 | agents %8/%9",
			ReportText(activeGroup.m_sGroupId),
			ReportText(activeGroup.m_sZoneId),
			ReportText(activeGroup.m_sFactionKey),
			ReportText(activeGroup.m_sPrefab),
			ReportText(activeGroup.m_sSpawnFallbackMode),
			ReportText(activeGroup.m_sRuntimeStatus),
			ReportBool(activeGroup.m_bSpawnedEntity),
			liveMembers,
			activeGroup.m_iInfantryCount);
		return actual + " | visual " + ReportText(BuildActiveGroupRuntimeVisualEvidence(activeGroup.m_sGroupId));
	}

	protected string AppendConvoyDebugPhaseHistory(string summary, string phaseHistory)
	{
		if (phaseHistory.IsEmpty())
			return summary;
		if (summary.IsEmpty())
			return phaseHistory;
		if (summary.Contains(phaseHistory))
			return summary;

		return summary + " | " + phaseHistory;
	}

	protected string AppendConvoyDebugEvidence(string summary, string evidence)
	{
		if (evidence.IsEmpty())
			return summary;
		if (summary.IsEmpty())
			return evidence;

		return summary + " | " + evidence;
	}

	protected bool ConvoyDebugPhaseHistoryHasTravelPhase(string phaseHistory)
	{
		return phaseHistory.Contains(MISSION_CONVOY_MOVING) || phaseHistory.Contains(MISSION_CONVOY_CONTACT) || phaseHistory.Contains(MISSION_CONVOY_ARRIVED) || phaseHistory.Contains(MISSION_CONVOY_ELIMINATED);
	}

	protected bool ConvoyDebugPhaseHistoryHasTerminalPhase(string phaseHistory)
	{
		return phaseHistory.Contains(MISSION_CONVOY_ARRIVED) || phaseHistory.Contains(MISSION_CONVOY_ELIMINATED);
	}

	protected bool ConvoyDebugPhaseHistoryHasPhase(string phaseHistory, string phase)
	{
		if (phase.IsEmpty())
			return false;

		return phaseHistory.Contains(phase);
	}

	protected bool ConvoyDebugPhaseIsTravelOrTerminal(string phase)
	{
		return phase == MISSION_CONVOY_MOVING || phase == MISSION_CONVOY_CONTACT || ConvoyDebugPhaseIsTerminal(phase);
	}

	protected bool ConvoyDebugPhaseIsTerminal(string phase)
	{
		return phase == MISSION_CONVOY_ARRIVED || phase == MISSION_CONVOY_ELIMINATED;
	}

	bool UpdateMissionConvoys(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, int elapsedSeconds)
	{
		if (!state)
			return false;
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return false;

		bool changed = CleanupInactiveMissionConvoyRuntime(state);
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;
			if (IsTerminalMissionConvoyPhase(mission))
				continue;

			changed = NormalizeRestoredMissionConvoyRuntime(state, preset, mission) || changed;
			changed = EnsureMissionConvoyRuntime(state, preset, mission) || changed;
			changed = UpdateMissionConvoyContact(state, mission) || changed;
		}

		changed = UpdateRuntimeGroupSurvivors(state) || changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;
			if (IsTerminalMissionConvoyPhase(mission))
				continue;

			changed = EnsureMissionConvoyCrewSeating(state, mission) || changed;
			changed = UpdateMissionConvoyContact(state, mission) || changed;
			changed = UpdateMissionConvoyPhase(state, mission, elapsedSeconds) || changed;
			changed = UpdateMissionConvoyObjective(state, mission) || changed;
		}

		return changed;
	}

	bool EnsureMissionConvoyRuntimeNow(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
			return false;
		if (IsPersistenceSmokeMission(mission) || IsTerminalMissionConvoyPhase(mission))
			return false;

		bool changed = NormalizeRestoredMissionConvoyRuntime(state, preset, mission);
		changed = EnsureMissionConvoyRuntime(state, preset, mission) || changed;
		changed = UpdateRuntimeGroupSurvivors(state) || changed;
		changed = EnsureMissionConvoyCrewSeating(state, mission) || changed;
		changed = UpdateMissionConvoyContact(state, mission) || changed;
		changed = UpdateMissionConvoyPhase(state, mission, state.m_iElapsedSeconds) || changed;
		changed = UpdateMissionConvoyObjective(state, mission) || changed;
		return changed;
	}

	protected bool NormalizeRestoredMissionConvoyRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !state.m_bRestoredFromPersistence)
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
			return false;

		bool changed;
		if (mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == "created" || mission.m_sRuntimePhase == "active" || mission.m_sRuntimePhase == "convoy_static")
		{
			mission.m_sRuntimePhase = MISSION_CONVOY_STAGING;
			changed = true;
		}

		int vehicleIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, vehicleIndex);
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			if (IsMissionConvoyVehicleAssetResolved(asset))
			{
				if (activeGroup)
				{
					if (activeGroup.m_sRuntimeStatus != MISSION_CONVOY_ELIMINATED)
					{
						activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
						changed = true;
					}
					if (activeGroup.m_iSurvivorVehicleCount != 0 || activeGroup.m_iSurvivorInfantryCount != 0 || activeGroup.m_iLastSeenAliveCount != 0 || activeGroup.m_bSpawnedEntity || activeGroup.m_iAssignedWaypointCount != 0)
					{
						activeGroup.m_iSurvivorVehicleCount = 0;
						activeGroup.m_iSurvivorInfantryCount = 0;
						activeGroup.m_iLastSeenAliveCount = 0;
						activeGroup.m_bSpawnedEntity = false;
						activeGroup.m_iAssignedWaypointCount = 0;
						changed = true;
					}
				}

				DeleteRuntimeGroupEntity(groupId);
				HST_RuntimeVehicleState terminalVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
				if (terminalVehicle && !terminalVehicle.m_bDeleted)
				{
					terminalVehicle.m_bDeleted = true;
					changed = true;
				}

				vehicleIndex++;
				continue;
			}

			if (!activeGroup)
			{
				activeGroup = CreateMissionConvoyGroup(state, preset, mission, asset, vehicleIndex);
				if (activeGroup)
				{
					vector createdPosition = ResolveRestoredConvoyGroupPosition(asset);
					if (!IsZeroVector(createdPosition))
					{
						activeGroup.m_vPosition = createdPosition;
						activeGroup.m_vSourcePosition = createdPosition;
					}
					activeGroup.m_vTargetPosition = asset.m_vTargetPosition;
					state.m_aActiveGroups.Insert(activeGroup);
					changed = true;
				}
			}

			if (activeGroup)
			{
				changed = ClampRestoredConvoyCrewCounts(activeGroup) || changed;
				if (activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_iInfantryCount <= 0)
				{
					if (activeGroup.m_bSpawnedEntity || activeGroup.m_iAssignedWaypointCount != 0)
					{
						activeGroup.m_bSpawnedEntity = false;
						activeGroup.m_iAssignedWaypointCount = 0;
						changed = true;
					}
					DeleteRuntimeGroupEntity(groupId, false);
					vehicleIndex++;
					continue;
				}

				bool missingCrewRuntime = GetRuntimeCrewGroupEntity(groupId) == null;
				bool missingVehicleRuntime = GetRuntimeVehicleEntity(groupId) == null;
				if (activeGroup.m_bSpawnedEntity && (missingCrewRuntime || missingVehicleRuntime))
				{
					if (WasRestoredMissionConvoyRuntimeRebuildAttempted(groupId))
					{
						changed = RecordRestoredMissionConvoyRuntimeHandleLoss(activeGroup, missingCrewRuntime, missingVehicleRuntime) || changed;
						vehicleIndex++;
						continue;
					}

					MarkRestoredMissionConvoyRuntimeRebuildAttempted(groupId);
					DeleteRuntimeGroupEntity(groupId);
					vector restoredPosition = ResolveRestoredConvoyGroupPosition(asset);
					if (!IsZeroVector(restoredPosition))
					{
						activeGroup.m_vPosition = restoredPosition;
						activeGroup.m_vSourcePosition = restoredPosition;
						asset.m_vSourcePosition = restoredPosition;
						asset.m_vCurrentPosition = restoredPosition;
						asset.m_vLastKnownPosition = restoredPosition;
					}

					activeGroup.m_vTargetPosition = asset.m_vTargetPosition;
					activeGroup.m_bSpawnedEntity = false;
					activeGroup.m_bSpawnAttempted = false;
					activeGroup.m_iAssignedWaypointCount = 0;
					activeGroup.m_sSpawnFallbackMode = "restore_rebuild";
					activeGroup.m_sSpawnFailureReason = "Runtime handles were not persisted; rebuilding convoy group from active mission state.";
					asset.m_bSpawned = false;

					HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
					if (runtimeEntity)
					{
						runtimeEntity.m_bSpawned = false;
						runtimeEntity.m_vPosition = asset.m_vCurrentPosition;
					}

					HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
					if (runtimeVehicle)
					{
						runtimeVehicle.m_bDeleted = false;
						runtimeVehicle.m_vPosition = asset.m_vCurrentPosition;
					}

					changed = true;
				}
			}

			vehicleIndex++;
		}

		return changed;
	}

	protected bool ClampRestoredConvoyCrewCounts(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		bool changed;
		int restoredAlive = Math.Max(activeGroup.m_iSurvivorInfantryCount, activeGroup.m_iLastSeenAliveCount);
		if (restoredAlive <= 0)
		{
			bool explicitlyEliminated = activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_sRuntimeStatus == "eliminated";
			if (!explicitlyEliminated && activeGroup.m_iInfantryCount > 0)
			{
				restoredAlive = activeGroup.m_iInfantryCount;
			}
			else if (activeGroup.m_iInfantryCount != 0 || activeGroup.m_iSurvivorInfantryCount != 0 || activeGroup.m_iLastSeenAliveCount != 0 || activeGroup.m_sRuntimeStatus != MISSION_CONVOY_ELIMINATED)
			{
				activeGroup.m_iInfantryCount = 0;
				activeGroup.m_iSurvivorInfantryCount = 0;
				activeGroup.m_iLastSeenAliveCount = 0;
				activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
				changed = true;
				return changed;
			}
		}

		if (activeGroup.m_iInfantryCount <= 0)
		{
			activeGroup.m_iInfantryCount = restoredAlive;
			changed = true;
		}

		int clampedAlive = Math.Min(activeGroup.m_iInfantryCount, restoredAlive);
		if (activeGroup.m_iSurvivorInfantryCount != clampedAlive)
		{
			activeGroup.m_iSurvivorInfantryCount = clampedAlive;
			changed = true;
		}
		if (activeGroup.m_iLastSeenAliveCount != clampedAlive)
		{
			activeGroup.m_iLastSeenAliveCount = clampedAlive;
			changed = true;
		}

		return changed;
	}

	protected vector ResolveRestoredConvoyGroupPosition(HST_MissionAssetState asset)
	{
		if (!asset)
			return "0 0 0";
		if (!IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;
		if (!IsZeroVector(asset.m_vLastKnownPosition))
			return asset.m_vLastKnownPosition;
		return asset.m_vSourcePosition;
	}

	protected bool UpdateMissionConvoyContact(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		string reason;
		if (!TryResolveMissionConvoyContactReasonForUpdate(state, mission, reason))
			return false;

		return SetMissionConvoyContact(state, mission, reason);
	}

	protected bool TryResolveMissionConvoyContactReason(HST_CampaignState state, HST_ActiveMissionState mission, out string reason)
	{
		return TryResolveMissionConvoyContactReasonInternal(state, mission, false, reason);
	}

	protected bool TryResolveMissionConvoyContactReasonForUpdate(HST_CampaignState state, HST_ActiveMissionState mission, out string reason)
	{
		return TryResolveMissionConvoyContactReasonInternal(state, mission, true, reason);
	}

	protected bool TryResolveMissionConvoyContactReasonInternal(HST_CampaignState state, HST_ActiveMissionState mission, bool allowStateMutation, out string reason)
	{
		reason = "";
		if (!state || !IsMissionConvoyContactEligible(mission))
			return false;

		int assetIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, assetIndex);
			assetIndex++;

			if (asset.m_bDelivered)
			{
				reason = string.Format("Convoy contact: vehicle %1 captured.", ReportText(asset.m_sAssetId));
				return true;
			}
			if (asset.m_bDestroyed)
			{
				reason = string.Format("Convoy contact: vehicle %1 destroyed.", ReportText(asset.m_sAssetId));
				return true;
			}

			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			vector position = ResolveMissionConvoyVehiclePosition(asset, groupId);
			if (IsZeroVector(position) && activeGroup)
				position = activeGroup.m_vPosition;

			float nearestPlayer = ResolveNearestLivingPlayerDistanceMeters(position);
			if (nearestPlayer >= 0 && nearestPlayer <= CONVOY_CONTACT_RADIUS_METERS)
			{
				reason = string.Format("Convoy contact: living player within %1m contact radius at %2m from vehicle %3.", Math.Round(CONVOY_CONTACT_RADIUS_METERS), Math.Round(nearestPlayer), ReportText(asset.m_sAssetId));
				return true;
			}

			if (!activeGroup || !activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED)
				continue;

			int aliveCrew = CountAliveRuntimeCrewAgents(activeGroup);
			if (activeGroup.m_iLastSeenAliveCount > 0 && aliveCrew < activeGroup.m_iLastSeenAliveCount && !IsConvoyCrewControlPending(state, activeGroup))
			{
				if (mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT)
				{
					reason = string.Format("Convoy contact: crew count decreased for %1 from %2 to %3.", ReportText(groupId), activeGroup.m_iLastSeenAliveCount, aliveCrew);
					return true;
				}
				if (allowStateMutation && TryRepairMissionConvoyCrewPopulation(state, mission, activeGroup, "contact gate"))
					continue;
				activeGroup.m_sSpawnFailureReason = string.Format("Convoy contact pending: crew count decreased before explicit contact for %1 from %2 to %3.", ReportText(groupId), activeGroup.m_iLastSeenAliveCount, aliveCrew);
				activeGroup.m_sConvoyRuntimeStage = "CREW_UNOBSERVED";
				continue;
			}

			IEntity vehicleEntity = GetRuntimeVehicleEntity(groupId);
			if (!vehicleEntity)
				continue;

			string mobileReason;
			if (GetConvoyVehicleControlAdapter().IsVehicleMobile(vehicleEntity, mobileReason))
				continue;

			if (mobileReason == "vehicle is destroyed" && allowStateMutation)
				MarkMissionConvoyVehicleDestroyed(state, mission, asset, groupId, vehicleEntity.GetOrigin(), mobileReason);

			reason = string.Format("Convoy contact: vehicle %1 is no longer mobile: %2.", ReportText(asset.m_sAssetId), ReportText(mobileReason));
			return true;
		}

		return false;
	}

	protected bool IsMissionConvoyContactEligible(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
			return false;
		if (mission.m_sRuntimePhase == MISSION_CONVOY_FAILED || mission.m_sRuntimePhase == MISSION_CONVOY_ELIMINATED || mission.m_sRuntimePhase == MISSION_CONVOY_ARRIVED)
			return false;

		return true;
	}

	protected bool SetMissionConvoyContact(HST_CampaignState state, HST_ActiveMissionState mission, string reason)
	{
		if (!state || !mission)
			return false;
		if (reason.IsEmpty())
			reason = "Convoy contact: ambush started.";

		bool changed;
		bool enteredContact;
		if (mission.m_sRuntimePhase != MISSION_CONVOY_CONTACT)
		{
			mission.m_sRuntimePhase = MISSION_CONVOY_CONTACT;
			changed = true;
			enteredContact = true;
		}
		if (mission.m_sRuntimeFailureReason.IsEmpty())
		{
			mission.m_sRuntimeFailureReason = reason;
			changed = true;
		}

		changed = ApplyMissionConvoyStatusToGroups(state, mission, MISSION_CONVOY_CONTACT) || changed;
		if (changed)
		{
			m_bMarkerRefreshNeeded = true;
			if (enteredContact)
				Print(string.Format("h-istasi mission convoy | %1 entered contact: %2", mission.m_sInstanceId, reason));
		}

		return changed;
	}

	protected void MarkMissionConvoyVehicleDestroyed(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, string groupId, vector position, string reason)
	{
		if (!state || !mission || !asset || asset.m_bDestroyed)
			return;

		asset.m_bDestroyed = true;
		asset.m_bAlive = false;
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_sLastInteraction = "destroyed";
		if (!IsZeroVector(position))
		{
			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
		}

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bDestroyed = true;
			runtimeEntity.m_vPosition = asset.m_vCurrentPosition;
		}

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
		if (runtimeVehicle)
		{
			runtimeVehicle.m_bDeleted = true;
			runtimeVehicle.m_vPosition = asset.m_vCurrentPosition;
		}

		HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
		if (activeGroup)
		{
			activeGroup.m_iSurvivorVehicleCount = 0;
			int aliveCrew = CountAliveRuntimeCrewAgents(activeGroup);
			if (aliveCrew <= 0 && !IsConvoyCrewControlPending(state, activeGroup))
			{
				activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
				activeGroup.m_iLastSeenAliveCount = 0;
				activeGroup.m_iSurvivorInfantryCount = 0;
			}
		}

		mission.m_iRuntimeDestroyedCount++;
		Print(string.Format("h-istasi mission convoy | %1 vehicle %2 marked destroyed: %3", mission.m_sInstanceId, asset.m_sAssetId, ReportText(reason)));
	}

	protected string BuildMissionConvoyContactReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		string contactActive = "no";
		if (mission && mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT)
			contactActive = "yes";

		string reason = "";
		if (mission)
			reason = mission.m_sRuntimeFailureReason;
		if (reason.IsEmpty())
		{
			string pendingReason;
			if (TryResolveMissionConvoyContactReason(state, mission, pendingReason))
				reason = pendingReason;
			else
				reason = "none";
		}

		return string.Format("\n  convoy contact | active %1 | radius %2m | reason %3", contactActive, Math.Round(CONVOY_CONTACT_RADIUS_METERS), ReportText(reason));
	}

	protected bool CleanupInactiveMissionConvoyRuntime(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			if (!IsMissionConvoyGroup(activeGroup))
				continue;
			if (HasActiveMissionForConvoyGroup(state, activeGroup))
				continue;

			if (ShouldKeepExpiredEngagedConvoyRuntime(state, activeGroup))
			{
				changed = MarkExpiredEngagedConvoyRuntimePreserved(activeGroup) || changed;
				continue;
			}

			bool preserveVehicle = ShouldPreserveInactiveMissionConvoyVehicle(state, activeGroup);
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId, !preserveVehicle);
			RemoveConvoyProgressStatusForGroup(activeGroup.m_sGroupId);
			if (preserveVehicle)
				Print(string.Format("h-istasi mission convoy | cleaned inactive convoy runtime group %1 and preserved neutralized vehicle", activeGroup.m_sGroupId));
			else
				Print(string.Format("h-istasi mission convoy | cleaned inactive convoy runtime group %1", activeGroup.m_sGroupId));
			state.m_aActiveGroups.Remove(i);
			changed = true;
		}

		for (int j = m_aConvoyProgressStatuses.Count() - 1; j >= 0; j--)
		{
			HST_ConvoyProgressStatus progress = m_aConvoyProgressStatuses[j];
			if (!progress)
			{
				m_aConvoyProgressStatuses.Remove(j);
				continue;
			}
			if (HasActiveConvoyMissionById(state, progress.m_sMissionInstanceId))
				continue;

			m_aConvoyProgressStatuses.Remove(j);
		}

		if (changed)
			m_bMarkerRefreshNeeded = true;

		return changed;
	}

	protected bool ShouldKeepExpiredEngagedConvoyRuntime(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return false;
		if (activeGroup.m_sRuntimeStatus != MISSION_CONVOY_CONTACT)
			return false;

		HST_ActiveMissionState mission = FindMissionForConvoyGroup(state, activeGroup);
		if (!mission || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_EXPIRED)
			return false;
		if (CountAliveRuntimeCrewAgents(activeGroup) <= 0)
			return false;

		vector runtimePosition = ResolveMissionConvoyRuntimePosition(activeGroup);
		float nearestPlayerDistance = ResolveNearestLivingPlayerDistanceMeters(runtimePosition);
		return nearestPlayerDistance >= 0 && nearestPlayerDistance <= EXPIRED_CONVOY_PLAYER_RENDER_BUBBLE_METERS;
	}

	protected bool MarkExpiredEngagedConvoyRuntimePreserved(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		string mode = "expired_combat_preserved";
		string reason = "Expired convoy combat preserved until no living player remains inside render bubble.";
		if (activeGroup.m_sSpawnFallbackMode == mode && activeGroup.m_sSpawnFailureReason == reason)
			return false;

		activeGroup.m_sSpawnFallbackMode = mode;
		activeGroup.m_sSpawnFailureReason = reason;
		Print(string.Format("h-istasi mission convoy | preserving expired engaged convoy runtime %1 until players leave render bubble", activeGroup.m_sGroupId));
		return true;
	}

	protected vector ResolveMissionConvoyRuntimePosition(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return vector.Zero;

		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		if (vehicleEntity)
			return vehicleEntity.GetOrigin();

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		if (crewEntity)
			return crewEntity.GetOrigin();

		return activeGroup.m_vPosition;
	}

	protected bool ShouldPreserveInactiveMissionConvoyVehicle(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		HST_ActiveMissionState mission = FindMissionForConvoyGroup(state, activeGroup);
		if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			return false;
		if (!IsMissionConvoyCrewEliminationCompletion(mission))
			return false;

		HST_MissionAssetState asset = FindMissionConvoyAssetForGroup(state, mission, activeGroup);
		if (!asset || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
			return false;

		return !asset.m_bDestroyed && !asset.m_bDelivered;
	}

	protected bool IsMissionConvoyCrewEliminationCompletion(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;
		if (mission.m_bConvoyCrewEliminatedOutcomeApplied)
			return true;

		return mission.m_sLastRuntimeEventKey == CONVOY_COMPLETE_EVENT_KEY || mission.m_sLastRuntimeEventKey == MISSION_CONVOY_ELIMINATED || mission.m_sLastRuntimeEventKey == "convoy_secured_sent";
	}

	protected bool HasActiveMissionForConvoyGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		HST_ActiveMissionState mission = FindMissionForConvoyGroup(state, activeGroup);
		return mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE;
	}

	protected bool HasActiveConvoyMissionById(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return false;

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (mission && mission.m_sInstanceId == instanceId && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE && mission.m_sRuntimePrimitive == MISSION_CONVOY_PRIMITIVE)
				return true;
		}

		return false;
	}

	protected void RemoveConvoyProgressStatusForGroup(string groupId)
	{
		if (groupId.IsEmpty())
			return;

		for (int i = m_aConvoyProgressStatuses.Count() - 1; i >= 0; i--)
		{
			HST_ConvoyProgressStatus progress = m_aConvoyProgressStatuses[i];
			if (progress && progress.m_sGroupId == groupId)
				m_aConvoyProgressStatuses.Remove(i);
		}
	}

	protected bool EnsureMissionConvoyRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool changed;
		int vehicleAssets;
		int crewedVehicles;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			int vehicleIndex = vehicleAssets;
			vehicleAssets++;
			if (IsMissionConvoyVehicleAssetResolved(asset))
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, vehicleIndex);
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			if (!activeGroup)
			{
				activeGroup = CreateMissionConvoyGroup(state, preset, mission, asset, vehicleIndex);
				if (activeGroup)
				{
					state.m_aActiveGroups.Insert(activeGroup);
					changed = true;
				}
			}

			if (activeGroup && activeGroup.m_sRuntimeStatus != "spawn_failed" && ShouldSpawnMissionConvoyRuntime(state, activeGroup))
				changed = TrySpawnMissionConvoyGroup(state, preset, mission, asset, activeGroup, vehicleIndex) || changed;

			if (activeGroup)
				changed = SyncMissionConvoyVehicleAssetRuntimeState(state, mission, asset, activeGroup, groupId) || changed;

			if (activeGroup && activeGroup.m_bSpawnedEntity && activeGroup.m_sRuntimeStatus != "spawn_failed" && ResolveMissionConvoyRestorableCrewCount(state, activeGroup) > 0)
				crewedVehicles++;
		}

		if (mission.m_sRuntimePhase != MISSION_CONVOY_CONTACT && vehicleAssets > 0 && vehicleAssets < 3)
		{
			SetMissionConvoyFailure(state, mission, "Convoy needs at least three valid vehicle assets.");
			changed = true;
		}
		else if (mission.m_sRuntimePhase != MISSION_CONVOY_CONTACT && vehicleAssets >= 3 && crewedVehicles < 3 && AllMissionConvoyGroupsAttempted(state, mission, vehicleAssets) && !HasPendingConvoyCrewControl(state, mission, vehicleAssets))
		{
			SetMissionConvoyFailure(state, mission, "Convoy could not spawn three crewed vehicles.");
			changed = true;
		}

		return changed;
	}

	protected bool EnsureMissionConvoyCrewSeating(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !ShouldRetryMissionConvoyCrewSeating(state, mission))
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroupForMission(activeGroup, mission) || !activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED)
				continue;

			string previousFallbackMode = activeGroup.m_sSpawnFallbackMode;
			string previousReason = activeGroup.m_sSpawnFailureReason;
			bool travelRecovery = IsMissionConvoyTravelPhase(mission);
			if (travelRecovery && !IsMissionConvoyCrewControlDegraded(activeGroup) && IsMissionConvoyWaypointAssigned(activeGroup))
				continue;

			IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
			IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
			if (!crewEntity || !vehicleEntity)
			{
				if (activeGroup.m_sSpawnFallbackMode != previousFallbackMode || activeGroup.m_sSpawnFailureReason != previousReason)
					changed = true;
				continue;
			}

			string reseatBlockReason;
			if (ShouldSuppressMissionConvoyCrewReseat(mission, activeGroup, crewEntity, vehicleEntity, reseatBlockReason))
			{
				activeGroup.m_sSpawnFailureReason = "Convoy crew reseat blocked: " + reseatBlockReason;
				if (mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT)
					activeGroup.m_sConvoyRuntimeStage = "CONTACT";
				if (activeGroup.m_sSpawnFallbackMode != previousFallbackMode || activeGroup.m_sSpawnFailureReason != previousReason)
					changed = true;
				continue;
			}

			string seatingReason;
			bool preserveWaypointMode = activeGroup.m_sSpawnFallbackMode == "convoy_waypoints";
			string recoveryLabel = "Convoy moving recovery";
			if (mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT)
				recoveryLabel = "Convoy contact recovery";
			GetConvoyVehicleControlAdapter().MoveUnseatedLivingCrewNearVehicle(crewEntity, vehicleEntity, vehicleEntity.GetOrigin());
			if (GetConvoyVehicleControlAdapter().TryBindCrewToVehicle(activeGroup, crewEntity, vehicleEntity, seatingReason))
			{
				if (!preserveWaypointMode)
					activeGroup.m_sSpawnFallbackMode = "convoy_driver_available";
				if (travelRecovery)
					activeGroup.m_sSpawnFailureReason = recoveryLabel + " succeeded: " + seatingReason;
				else
					activeGroup.m_sSpawnFailureReason = seatingReason;
				RefreshMissionConvoyCrewCount(activeGroup);
				activeGroup.m_sConvoyRuntimeStage = "DRIVER_BOUND";
			}
			else
			{
				if (!preserveWaypointMode)
				{
					if (seatingReason.Contains("seating pending yes") || seatingReason.Contains("waiting for animated AI boarding"))
						activeGroup.m_sSpawnFallbackMode = "convoy_seating_pending";
					else
						activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_control_unavailable";
				}

				if (travelRecovery)
					activeGroup.m_sSpawnFailureReason = recoveryLabel + " pending: " + seatingReason;
				else
					activeGroup.m_sSpawnFailureReason = seatingReason;
				if (activeGroup.m_sSpawnFailureReason.IsEmpty())
					activeGroup.m_sSpawnFailureReason = "Convoy crew seating has not confirmed a seated AI driver yet.";
				if (CountAliveRuntimeCrewAgents(activeGroup) > 0)
					activeGroup.m_sConvoyRuntimeStage = "CREW_POPULATED";
			}

			if (activeGroup.m_sSpawnFallbackMode != previousFallbackMode || activeGroup.m_sSpawnFailureReason != previousReason)
				changed = true;
		}

		return changed;
	}

	protected bool ShouldRetryMissionConvoyCrewSeating(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;
		if (mission.m_sRuntimePhase == MISSION_CONVOY_STAGING)
			return true;
		if (mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT)
			return false;
		if (!state)
			return false;

		return IsMissionConvoyTravelPhase(mission) && state.m_iElapsedSeconds % CONVOY_PROGRESS_SYNC_SECONDS == 0;
	}

	protected bool ShouldSuppressMissionConvoyCrewReseat(HST_ActiveMissionState mission, HST_ActiveGroupState activeGroup, IEntity crewEntity, IEntity vehicleEntity, out string reason)
	{
		reason = "";
		if (!mission || !activeGroup)
			return false;
		if (!activeGroup.m_bEverHadLivingCrew && activeGroup.m_iMaxObservedCrewAlive <= 0 && activeGroup.m_iLastSeenAliveCount <= 0)
			return false;

		if (mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT)
		{
			reason = "convoy is in contact; AI dismounts are combat behavior";
			return true;
		}

		if (!IsMissionConvoyTravelPhase(mission))
			return false;

		string dismountReason;
		if (GetConvoyVehicleControlAdapter().AreAllLivingCrewDismounted(crewEntity, vehicleEntity, dismountReason))
		{
			reason = "all living crew are dismounted";
			if (!dismountReason.IsEmpty())
				reason = reason + ": " + dismountReason;
			return true;
		}

		return false;
	}

	protected bool IsMissionConvoyTravelPhase(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		return mission.m_sRuntimePhase == MISSION_CONVOY_MOVING || mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT;
	}

	protected HST_ActiveGroupState CreateMissionConvoyGroup(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionAssetState asset, int index)
	{
		HST_ZoneState targetZone = ResolveMissionConvoyTargetZone(state, mission, asset);
		if (!targetZone)
			return null;

		string factionKey = ResolveMissionConvoyFactionKey(state, preset, mission, targetZone);
		HST_ActiveGroupState activeGroup = new HST_ActiveGroupState();
		activeGroup.m_sGroupId = BuildMissionConvoyGroupId(mission, index);
		activeGroup.m_sZoneId = targetZone.m_sZoneId;
		activeGroup.m_sFactionKey = factionKey;
		activeGroup.m_sMissionInstanceId = mission.m_sInstanceId;
		activeGroup.m_sPrefab = SelectConvoyCrewGroupPrefab(state, preset, targetZone, factionKey, index);
		activeGroup.m_sRouteId = ResolveMissionConvoyRouteId(state, mission);
		activeGroup.m_vSourcePosition = asset.m_vSourcePosition;
		activeGroup.m_vTargetPosition = asset.m_vTargetPosition;
		activeGroup.m_vPosition = asset.m_vSourcePosition;
		activeGroup.m_sRuntimeStatus = ResolveMissionConvoyRuntimeStatus(mission);
		activeGroup.m_iInfantryCount = ResolveMissionConvoyCrewCount(state, mission, index);
		activeGroup.m_iVehicleCount = 1;
		activeGroup.m_iOriginalInfantryCount = activeGroup.m_iInfantryCount;
		activeGroup.m_iOriginalVehicleCount = activeGroup.m_iVehicleCount;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_iSurvivorVehicleCount = 1;
		activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		activeGroup.m_sConvoyRuntimeStage = "PLANNED";
		return activeGroup;
	}

	protected bool TrySpawnMissionConvoyGroup(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, int index)
	{
		if (!state || !mission || !asset || !activeGroup)
			return false;
		if (IsMissionConvoyVehicleAssetResolved(asset))
			return false;
		if (state.m_bRestoredFromPersistence)
			MarkRestoredMissionConvoyRuntimeRebuildAttempted(activeGroup.m_sGroupId);

		bool hasCrewRuntime = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) != null;
		bool hasVehicleRuntime = GetRuntimeVehicleEntity(activeGroup.m_sGroupId) != null;
		if (hasCrewRuntime && hasVehicleRuntime)
			return false;
		if (hasCrewRuntime || hasVehicleRuntime)
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);

		vector spawnPosition;
		if (!TryResolveMissionConvoyVehicleSpawnPosition(mission, asset, spawnPosition) || HST_WorldPositionService.IsLikelyOpenWater(spawnPosition))
		{
			activeGroup.m_bSpawnAttempted = true;
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = "No dry vehicle-safe convoy staging position.";
			activeGroup.m_bCrewPopulationTerminallyFailed = true;
			activeGroup.m_sCrewPopulationFailureReason = activeGroup.m_sSpawnFailureReason;
			activeGroup.m_sConvoyRuntimeStage = "FAILED";
			mission.m_sRuntimeFailureReason = activeGroup.m_sSpawnFailureReason;
			Print(string.Format("h-istasi mission convoy | spawn failed %1 asset %2 at %3: %4", mission.m_sInstanceId, asset.m_sAssetId, asset.m_vSourcePosition, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			return true;
		}

		activeGroup.m_vPosition = OffsetConvoyCrewSpawnPosition(spawnPosition, asset.m_vTargetPosition, index);
		activeGroup.m_vSourcePosition = activeGroup.m_vPosition;
		if (!TrySpawnActiveGroup(activeGroup, state, preset))
		{
			activeGroup.m_bCrewPopulationTerminallyFailed = true;
			activeGroup.m_sCrewPopulationFailureReason = activeGroup.m_sSpawnFailureReason;
			activeGroup.m_sConvoyRuntimeStage = "FAILED";
			mission.m_sRuntimeFailureReason = activeGroup.m_sSpawnFailureReason;
			return true;
		}
		activeGroup.m_sConvoyRuntimeStage = "CREW_GROUP_CREATED";

		string vehiclePrefab = asset.m_sPrefab;
		if (vehiclePrefab.IsEmpty())
			vehiclePrefab = SelectMissionConvoyVehiclePrefab(state, ResolveMissionConvoyTargetZone(state, mission, asset), activeGroup.m_sFactionKey, mission, index, preset);

		GenericEntity vehicleEntity = SpawnMissionConvoyVehicle(state, preset, mission, asset, activeGroup, vehiclePrefab, spawnPosition, index);
		if (!vehicleEntity)
		{
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = "Convoy vehicle prefab failed to spawn.";
			activeGroup.m_bCrewPopulationTerminallyFailed = true;
			activeGroup.m_sCrewPopulationFailureReason = activeGroup.m_sSpawnFailureReason;
			activeGroup.m_sConvoyRuntimeStage = "FAILED";
			mission.m_sRuntimeFailureReason = activeGroup.m_sSpawnFailureReason;
			Print(string.Format("h-istasi mission convoy | vehicle spawn failed %1 group %2 prefab %3", mission.m_sInstanceId, activeGroup.m_sGroupId, vehiclePrefab), LogLevel.WARNING);
			return true;
		}
		activeGroup.m_sConvoyRuntimeStage = "VEHICLE_SPAWNED";

		m_aRuntimeVehicleGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeVehicleEntities.Insert(vehicleEntity);
		bool crewPopulationPending = activeGroup.m_sRuntimeStatus == "spawn_pending_agents" && activeGroup.m_iSpawnedAgentCount <= 0;
		if (!crewPopulationPending)
			activeGroup.m_sRuntimeStatus = ResolveMissionConvoyRuntimeStatus(mission);
		activeGroup.m_iAssignedWaypointCount = 0;
		string adapterBindReason;
		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		if (crewPopulationPending)
		{
			activeGroup.m_sSpawnFallbackMode = "convoy_crew_population_pending";
			activeGroup.m_sSpawnFailureReason = "Convoy crew group is awaiting AI agent population.";
			activeGroup.m_sConvoyRuntimeStage = "CREW_GROUP_CREATED";
		}
		else
		{
			GetConvoyVehicleControlAdapter().MoveUnseatedLivingCrewNearVehicle(crewEntity, vehicleEntity, vehicleEntity.GetOrigin());
			if (GetConvoyVehicleControlAdapter().TryBindCrewToVehicle(activeGroup, crewEntity, vehicleEntity, adapterBindReason))
			{
				activeGroup.m_sSpawnFallbackMode = "convoy_driver_available";
				activeGroup.m_sSpawnFailureReason = adapterBindReason;
				RefreshMissionConvoyCrewCount(activeGroup);
				activeGroup.m_sConvoyRuntimeStage = "DRIVER_BOUND";
			}
			else
			{
				if (adapterBindReason.Contains("seating pending yes") || adapterBindReason.Contains("waiting for animated AI boarding"))
					activeGroup.m_sSpawnFallbackMode = "convoy_seating_pending";
				else
					activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_control_unavailable";
				activeGroup.m_sSpawnFailureReason = adapterBindReason;
				if (activeGroup.m_sSpawnFailureReason.IsEmpty())
					activeGroup.m_sSpawnFailureReason = "Convoy crew seating has not confirmed a seated AI driver yet.";
				if (CountAliveRuntimeCrewAgents(activeGroup) > 0)
					activeGroup.m_sConvoyRuntimeStage = "CREW_POPULATED";
			}
		}
		int liveCrew = CountAliveRuntimeCrewAgents(activeGroup);
		int preservedCrew = ResolveMissionConvoyRestorableCrewCount(state, activeGroup);
		if (liveCrew > 0)
		{
			RecordConvoyCrewObservedAlive(activeGroup, liveCrew);
			activeGroup.m_iLastSeenAliveCount = liveCrew;
			activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, liveCrew);
		}
		else if (preservedCrew > 0)
		{
			activeGroup.m_iLastSeenAliveCount = preservedCrew;
			activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, preservedCrew);
		}
		else
		{
			activeGroup.m_iLastSeenAliveCount = 0;
			activeGroup.m_iSurvivorInfantryCount = 0;
		}
		activeGroup.m_iSurvivorVehicleCount = 1;
		UpdateMissionConvoyAssetPosition(state, asset, spawnPosition);
		Print(string.Format("h-istasi mission convoy | spawned vehicle %1 and crew group %2 for %3 at %4", vehiclePrefab, activeGroup.m_sGroupId, mission.m_sInstanceId, spawnPosition));
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected GenericEntity SpawnMissionConvoyVehicle(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, string vehiclePrefab, vector spawnPosition, int index)
	{
		string selectedPrefab = SelectMissionConvoyVehiclePrefab(state, ResolveMissionConvoyTargetZone(state, mission, asset), activeGroup.m_sFactionKey, mission, index + 13, preset);
		if (!selectedPrefab.IsEmpty())
			vehiclePrefab = selectedPrefab;
		else if (vehiclePrefab.IsEmpty() || !IsValidVehiclePrefabResource(vehiclePrefab, activeGroup.m_sFactionKey))
			vehiclePrefab = SelectMissionConvoyVehiclePrefab(state, ResolveMissionConvoyTargetZone(state, mission, asset), activeGroup.m_sFactionKey, mission, index + 29, preset);
		if (vehiclePrefab.IsEmpty())
			return null;

		vector angles = BuildConvoyVehicleAngles(spawnPosition, asset.m_vTargetPosition);
		vector roadPosition;
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		if (HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(spawnPosition, CONVOY_ROAD_REPORT_SEARCH_RADIUS_METERS, asset.m_vTargetPosition, roadPosition, roadForward, roadWidth, roadDistance, roadReason))
			angles = BuildConvoyVehicleAnglesFromForward(spawnPosition, roadForward, asset.m_vTargetPosition);
		GenericEntity vehicleEntity = HST_WorldPositionService.SpawnPrefab(vehiclePrefab, spawnPosition, angles);
		if (!vehicleEntity)
			return null;

		ApplyCampaignDebugEntityName(vehicleEntity, MISSION_CONVOY_VEHICLE_ROLE, asset.m_sAssetId);
		HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(vehicleEntity);
		HST_WorldPositionService.ApplyUprightEntityTransform(vehicleEntity, spawnPosition, angles);
		asset.m_sPrefab = vehiclePrefab;
		asset.m_bSpawned = true;
		asset.m_bAlive = true;
		asset.m_vCurrentPosition = spawnPosition;
		asset.m_vLastKnownPosition = spawnPosition;

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtimeEntity)
		{
			runtimeEntity = new HST_MissionRuntimeEntityState();
			runtimeEntity.m_sRuntimeEntityId = asset.m_sEntityId;
			runtimeEntity.m_sMissionInstanceId = mission.m_sInstanceId;
			state.m_aMissionRuntimeEntities.Insert(runtimeEntity);
		}

		runtimeEntity.m_sKind = asset.m_sRole;
		runtimeEntity.m_sPrefab = vehiclePrefab;
		runtimeEntity.m_vPosition = spawnPosition;
		runtimeEntity.m_vAngles = angles;
		runtimeEntity.m_bSpawned = true;
		return vehicleEntity;
	}

	protected bool TryResolveMissionConvoyVehicleSpawnPosition(HST_ActiveMissionState mission, HST_MissionAssetState asset, out vector spawnPosition)
	{
		spawnPosition = "0 0 0";
		if (!mission || !asset)
			return false;

		if (TryResolveMissionConvoyVehicleSpawnPositionPass(mission, asset, spawnPosition, false))
			return true;

		return TryResolveMissionConvoyVehicleSpawnPositionPass(mission, asset, spawnPosition, true);
	}

	protected bool TryResolveMissionConvoyVehicleSpawnPositionPass(HST_ActiveMissionState mission, HST_MissionAssetState asset, out vector spawnPosition, bool preferHeavyVehicleTerrain)
	{
		spawnPosition = "0 0 0";
		if (!mission || !asset)
			return false;

		vector resolved;
		if (TryResolveMissionConvoyVehicleSpawnCandidate(asset.m_vSourcePosition, asset.m_vTargetPosition, resolved, preferHeavyVehicleTerrain) && IsMissionConvoyVehicleSpawnClear(mission, resolved))
		{
			spawnPosition = LiftMissionConvoyVehicleSpawnPosition(resolved);
			return true;
		}

		for (int ring = 1; ring <= 5; ring++)
		{
			float radius = CONVOY_VEHICLE_SPAWN_CLEARANCE_METERS * ring;
			for (int step = 0; step < 8; step++)
			{
				vector candidate = BuildConvoySpawnClearanceCandidate(asset.m_vSourcePosition, step, radius);
				if (!TryResolveMissionConvoyVehicleSpawnCandidate(candidate, asset.m_vTargetPosition, resolved, preferHeavyVehicleTerrain))
					continue;
				if (!IsMissionConvoyVehicleSpawnClear(mission, resolved))
					continue;

				spawnPosition = LiftMissionConvoyVehicleSpawnPosition(resolved);
				return true;
			}
		}

		return false;
	}

	protected bool TryResolveMissionConvoyVehicleSpawnCandidate(vector position, vector targetPosition, out vector resolved, bool preferHeavyVehicleTerrain)
	{
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		if (!HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(position, CONVOY_PHYSICAL_ROAD_SEARCH_RADIUS_METERS, targetPosition, resolved, roadForward, roadWidth, roadDistance, roadReason))
			return false;

		return HST_WorldPositionService.IsVehicleFootprintStableWithForward(resolved, roadForward);
	}

	protected vector LiftMissionConvoyVehicleSpawnPosition(vector position)
	{
		vector lifted;
		if (HST_WorldPositionService.TryResolveGroundPosition(position, HST_WorldPositionService.VEHICLE_GROUND_OFFSET + CONVOY_VEHICLE_SPAWN_LIFT_METERS, lifted, true))
			return lifted;

		lifted = position;
		lifted[1] = lifted[1] + CONVOY_VEHICLE_SPAWN_LIFT_METERS;
		return lifted;
	}

	protected vector BuildConvoySpawnClearanceCandidate(vector origin, int step, float radius)
	{
		vector candidate = origin;
		if (step == 0)
			candidate[0] = candidate[0] + radius;
		else if (step == 1)
			candidate[0] = candidate[0] - radius;
		else if (step == 2)
			candidate[2] = candidate[2] + radius;
		else if (step == 3)
			candidate[2] = candidate[2] - radius;
		else if (step == 4)
		{
			candidate[0] = candidate[0] + radius;
			candidate[2] = candidate[2] + radius;
		}
		else if (step == 5)
		{
			candidate[0] = candidate[0] - radius;
			candidate[2] = candidate[2] + radius;
		}
		else if (step == 6)
		{
			candidate[0] = candidate[0] + radius;
			candidate[2] = candidate[2] - radius;
		}
		else
		{
			candidate[0] = candidate[0] - radius;
			candidate[2] = candidate[2] - radius;
		}

		return candidate;
	}

	protected bool IsMissionConvoyVehicleSpawnClear(HST_ActiveMissionState mission, vector position)
	{
		if (!mission)
			return false;
		if (HST_WorldPositionService.IsLikelyOpenWater(position))
			return false;

		float minDistanceSq = CONVOY_VEHICLE_SPAWN_CLEARANCE_METERS * CONVOY_VEHICLE_SPAWN_CLEARANCE_METERS;
		for (int i = 0; i < m_aRuntimeVehicleGroupIds.Count(); i++)
		{
			string groupId = m_aRuntimeVehicleGroupIds[i];
			if (!IsMissionConvoyGroupIdForMission(groupId, mission))
				continue;
			if (i >= m_aRuntimeVehicleEntities.Count())
				continue;

			IEntity vehicle = m_aRuntimeVehicleEntities[i];
			if (!vehicle)
				continue;
			if (DistanceSq2D(vehicle.GetOrigin(), position) < minDistanceSq)
				return false;
		}

		return true;
	}

	protected bool UpdateMissionConvoyPhase(HST_CampaignState state, HST_ActiveMissionState mission, int elapsedSeconds)
	{
		if (!state || !mission)
			return false;

		bool changed;
		if (mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == "created" || mission.m_sRuntimePhase == "active" || mission.m_sRuntimePhase == "convoy_static")
		{
			mission.m_sRuntimePhase = MISSION_CONVOY_STAGING;
			changed = true;
		}

		if (mission.m_sRuntimePhase == MISSION_CONVOY_STAGING && mission.m_iRuntimeCounterB > 0 && mission.m_iRuntimeCounterA >= mission.m_iRuntimeCounterB)
		{
			changed = TryAdvanceMissionConvoyFromStaging(state, mission) || changed;
		}

		if (mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT && state.m_iElapsedSeconds % CONVOY_PROGRESS_SYNC_SECONDS == 0)
			changed = AssignMissionConvoyWaypoints(state, mission) || changed;

		if (TryResolveMissionConvoyDestinationArrival(state, mission))
			return true;

		if (mission.m_sRuntimePhase == MISSION_CONVOY_MOVING && IsMissionConvoyMovementInterrupted(state, mission))
		{
			SyncMissionConvoyAssetPositions(state, mission);
			SetMissionConvoyStaticFallback(state, mission, "Convoy movement interrupted because every moving convoy group lost vehicle control or waypoint assignment.");
			return true;
		}

		if (IsMissionConvoyTravelPhase(mission))
			changed = UpdateMissionConvoyProgress(state, mission) || changed;

		if (IsMissionConvoyTravelPhase(mission) && state.m_iElapsedSeconds % CONVOY_MARKER_REFRESH_SECONDS == 0)
		{
			m_bMarkerRefreshNeeded = true;
			changed = true;
		}

		return changed;
	}

	protected bool UpdateMissionConvoyProgress(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !IsMissionConvoyTravelPhase(mission))
			return false;
		if (state.m_iElapsedSeconds % CONVOY_PROGRESS_SYNC_SECONDS != 0)
			return false;

		bool changed;
		int assetIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, assetIndex);
			assetIndex++;
			if (IsMissionConvoyVehicleAssetResolved(asset))
				continue;

			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			vector position = ResolveMissionConvoyVehiclePosition(asset, groupId);
			bool positionChanged = !IsZeroVector(position) && DistanceSq2D(asset.m_vCurrentPosition, position) > 1.0;
			UpdateMissionConvoyAssetPosition(state, asset, position);
			if (activeGroup && !IsZeroVector(position))
				activeGroup.m_vPosition = position;

			HST_ConvoyProgressStatus progress = EnsureConvoyProgressStatus(mission, asset, groupId);
			changed = positionChanged || UpdateConvoyVehicleProgressStatus(state, mission, asset, activeGroup, progress, position) || changed;
		}

		return changed;
	}

	protected bool UpdateConvoyVehicleProgressStatus(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, HST_ConvoyProgressStatus progress, vector position)
	{
		if (!state || !mission || !asset || !progress || IsZeroVector(position))
			return false;

		int now = state.m_iElapsedSeconds;
		float distanceToDestination = ResolveMissionConvoyDistanceToDestinationMeters(mission, asset, position);
		progress.m_fNearestPlayerDistanceMeters = ResolveNearestLivingPlayerDistanceMeters(position);
		progress.m_fDistanceToDestinationMeters = distanceToDestination;
		RememberConvoyProgressPhase(progress, mission.m_sRuntimePhase);
		if (progress.m_iLastSampleSecond <= 0)
		{
			progress.m_vLastSamplePosition = position;
			progress.m_iLastSampleSecond = now;
			progress.m_iLastProgressSecond = now;
			progress.m_fPreviousDistanceToDestinationMeters = distanceToDestination;
			progress.m_iSampleCount = 1;
			progress.m_fLastMovementMeters = 0.0;
			progress.m_fLastDistanceClosedMeters = 0.0;
			progress.m_bNoProgress = false;
			progress.m_bHardStuck = false;
			progress.m_sLastProgressReason = "progress initialized";
			return true;
		}

		float movementMeters = Math.Sqrt(DistanceSq2D(progress.m_vLastSamplePosition, position));
		float distanceImprovedMeters = progress.m_fPreviousDistanceToDestinationMeters - distanceToDestination;
		bool madeProgress = movementMeters >= CONVOY_PROGRESS_THRESHOLD_METERS || distanceImprovedMeters >= CONVOY_PROGRESS_THRESHOLD_METERS;
		progress.m_vLastSamplePosition = position;
		progress.m_iLastSampleSecond = now;
		progress.m_fPreviousDistanceToDestinationMeters = distanceToDestination;
		progress.m_iSampleCount++;
		progress.m_fLastMovementMeters = movementMeters;
		progress.m_fLastDistanceClosedMeters = distanceImprovedMeters;
		progress.m_fMaxMovementMeters = Math.Max(progress.m_fMaxMovementMeters, movementMeters);
		progress.m_fMaxDistanceClosedMeters = Math.Max(progress.m_fMaxDistanceClosedMeters, distanceImprovedMeters);
		if (madeProgress)
		{
			progress.m_iLastProgressSecond = now;
			progress.m_bNoProgress = false;
			progress.m_bHardStuck = false;
			progress.m_sLastProgressReason = string.Format("progressing: moved %1m, closed %2m", Math.Round(movementMeters), Math.Round(distanceImprovedMeters));
			if (activeGroup)
				activeGroup.m_sConvoyRuntimeStage = "MOVING";
			return true;
		}

		int noProgressSeconds = now - progress.m_iLastProgressSecond;
		bool previousNoProgress = progress.m_bNoProgress;
		bool previousHardStuck = progress.m_bHardStuck;
		progress.m_bNoProgress = noProgressSeconds >= CONVOY_ROUTE_REISSUE_THRESHOLD_SECONDS;
		progress.m_bHardStuck = noProgressSeconds >= CONVOY_HARD_STUCK_THRESHOLD_SECONDS;
		progress.m_sLastProgressReason = string.Format("no progress for %1s: moved %2m, closed %3m", noProgressSeconds, Math.Round(movementMeters), Math.Round(distanceImprovedMeters));

		bool changed = previousNoProgress != progress.m_bNoProgress || previousHardStuck != progress.m_bHardStuck;
		if (progress.m_bNoProgress && progress.m_iRouteReissueAttemptCount < 1)
			changed = TryReissueMissionConvoyRouteForProgress(state, mission, activeGroup, progress, "stuck threshold") || changed;

		if (progress.m_bHardStuck && progress.m_iRouteSnapAttemptCount < 1)
			changed = TrySnapMissionConvoyVehicleToRoute(state, mission, asset, activeGroup, progress) || changed;

		return changed;
	}

	protected void RememberConvoyProgressPhase(HST_ConvoyProgressStatus progress, string phase)
	{
		if (!progress)
			return;
		if (phase.IsEmpty())
			phase = "unknown";
		if (progress.m_sPhaseHistory.IsEmpty())
		{
			progress.m_sPhaseHistory = phase;
			return;
		}
		if (progress.m_sPhaseHistory.Contains(phase))
			return;

		progress.m_sPhaseHistory = progress.m_sPhaseHistory + ">" + phase;
	}

	protected bool TryReissueMissionConvoyRouteForProgress(HST_CampaignState state, HST_ActiveMissionState mission, HST_ActiveGroupState activeGroup, HST_ConvoyProgressStatus progress, string reason)
	{
		if (!state || !mission || !activeGroup || !progress)
			return false;

		if (!IsZeroVector(activeGroup.m_vPosition))
			activeGroup.m_vSourcePosition = activeGroup.m_vPosition;

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		string reseatBlockReason;
		if (ShouldSuppressMissionConvoyCrewReseat(mission, activeGroup, crewEntity, vehicleEntity, reseatBlockReason) && !GetConvoyVehicleControlAdapter().HasLivingDriver(crewEntity, vehicleEntity))
		{
			progress.m_sLastRecoveryResult = "route reissue blocked: " + ReportText(reseatBlockReason);
			return true;
		}

		string driverReason;
		if (!TryEnsureMissionConvoyDriverForRouteReissue(activeGroup, driverReason))
		{
			progress.m_sLastRecoveryResult = string.Format("route reissue waiting for reseated driver after %1: %2", reason, ReportText(driverReason));
			return true;
		}

		ref array<vector> waypoints = BuildMissionConvoyWaypointPositions(ResolveMissionConvoyRoute(state, mission));
		bool reissued = TryAssignConvoyWaypoints(activeGroup, waypoints);
		progress.m_iRouteReissueAttemptCount++;
		progress.m_iLastRouteReissueSecond = state.m_iElapsedSeconds;
		if (reissued)
			progress.m_sLastRecoveryResult = string.Format("route reissued after %1: %2", reason, ReportText(activeGroup.m_sSpawnFailureReason));
		else
			progress.m_sLastRecoveryResult = string.Format("route reissue failed after %1: %2", reason, ReportText(activeGroup.m_sSpawnFailureReason));

		return true;
	}

	protected bool TryEnsureMissionConvoyDriverForRouteReissue(HST_ActiveGroupState activeGroup, out string reason)
	{
		reason = "";
		if (!activeGroup)
		{
			reason = "active convoy group missing";
			return false;
		}

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		if (GetConvoyVehicleControlAdapter().HasLivingDriver(crewEntity, vehicleEntity))
		{
			reason = "driver already seated";
			return true;
		}

		bool preserveWaypointMode = activeGroup.m_sSpawnFallbackMode == "convoy_waypoints";
		string seatingReason;
		GetConvoyVehicleControlAdapter().MoveUnseatedLivingCrewNearVehicle(crewEntity, vehicleEntity, vehicleEntity.GetOrigin());
		if (GetConvoyVehicleControlAdapter().TryBindCrewToVehicle(activeGroup, crewEntity, vehicleEntity, seatingReason))
		{
			if (!preserveWaypointMode)
				activeGroup.m_sSpawnFallbackMode = "convoy_driver_available";
			activeGroup.m_sSpawnFailureReason = "Convoy moving recovery succeeded before route reissue: " + seatingReason;
			RefreshMissionConvoyCrewCount(activeGroup);
			reason = seatingReason;
			return true;
		}

		if (!preserveWaypointMode)
		{
			if (seatingReason.Contains("seating pending yes") || seatingReason.Contains("waiting for animated AI boarding"))
				activeGroup.m_sSpawnFallbackMode = "convoy_seating_pending";
			else
				activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_control_unavailable";
		}
		activeGroup.m_sSpawnFailureReason = "Convoy moving recovery pending before route reissue: " + seatingReason;
		if (activeGroup.m_sSpawnFailureReason.IsEmpty())
			activeGroup.m_sSpawnFailureReason = "Convoy moving recovery pending before route reissue: no seated living AI driver is assigned to the vehicle.";
		reason = seatingReason;
		return false;
	}

	protected bool TrySnapMissionConvoyVehicleToRoute(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, HST_ConvoyProgressStatus progress)
	{
		if (!state || !mission || !asset || !activeGroup || !progress)
			return false;

		progress.m_iLastRouteSnapSecond = state.m_iElapsedSeconds;
		if (mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT)
		{
			progress.m_sLastRecoveryResult = "route snap blocked: convoy is in contact";
			return false;
		}

		if (progress.m_fNearestPlayerDistanceMeters >= 0 && progress.m_fNearestPlayerDistanceMeters < CONVOY_UNSTUCK_MIN_PLAYER_DISTANCE_METERS)
		{
			progress.m_sLastRecoveryResult = string.Format("route snap blocked: nearest player %1m is inside %2m gate", Math.Round(progress.m_fNearestPlayerDistanceMeters), Math.Round(CONVOY_UNSTUCK_MIN_PLAYER_DISTANCE_METERS));
			return false;
		}

		progress.m_iRouteSnapAttemptCount++;
		IEntity vehicle = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		if (!vehicle)
		{
			progress.m_sLastRecoveryResult = "route snap failed: convoy vehicle entity missing";
			return true;
		}

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		string reseatBlockReason;
		if (ShouldSuppressMissionConvoyCrewReseat(mission, activeGroup, crewEntity, vehicle, reseatBlockReason) && !GetConvoyVehicleControlAdapter().HasLivingDriver(crewEntity, vehicle))
		{
			progress.m_sLastRecoveryResult = "route snap blocked: " + ReportText(reseatBlockReason);
			return true;
		}

		vector snapPosition;
		vector snapForward;
		string snapReason;
		if (!TryResolveNearestConvoyRouteSnapPosition(state, mission, vehicle.GetOrigin(), snapPosition, snapForward, snapReason))
		{
			progress.m_sLastRecoveryResult = "route snap failed: " + ReportText(snapReason);
			return true;
		}

		vector angles = BuildConvoyVehicleAnglesFromForward(snapPosition, snapForward, asset.m_vTargetPosition);
		HST_WorldPositionService.ApplyUprightEntityTransform(vehicle, snapPosition, angles);
		SetRuntimeGroupEntitiesOrigin(activeGroup.m_sGroupId, snapPosition);

		int movedCrew = GetConvoyVehicleControlAdapter().MoveUnseatedLivingCrewNearVehicle(crewEntity, vehicle, snapPosition);
		string seatingReason;
		bool driverReady = GetConvoyVehicleControlAdapter().TryBindCrewToVehicle(activeGroup, crewEntity, vehicle, seatingReason);
		RefreshMissionConvoyCrewCount(activeGroup);
		string mountedReason;
		bool crewMounted = GetConvoyVehicleControlAdapter().AreLivingCrewMounted(crewEntity, vehicle, mountedReason);
		bool seatingPending = seatingReason.Contains("seating pending yes") || seatingReason.Contains("waiting for animated AI boarding") || mountedReason.Contains("seating pending yes") || mountedReason.Contains("waiting for animated AI boarding");
		if (!driverReady)
		{
			activeGroup.m_sSpawnFallbackMode = "convoy_seating_pending";
			activeGroup.m_sSpawnFailureReason = "Convoy route snap recovery pending: " + seatingReason;
			progress.m_sLastRecoveryResult = string.Format("route snap pending: moved %1 unseated crew near vehicle but no seated driver confirmed: %2", movedCrew, ReportText(seatingReason));
			return true;
		}
		if (!crewMounted && !seatingPending)
		{
			activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_control_unavailable";
			activeGroup.m_sSpawnFailureReason = "Convoy route snap recovery failed: " + mountedReason;
			progress.m_sLastRecoveryResult = string.Format("route snap failed: moved %1 unseated crew near vehicle but crew not mounted: %2", movedCrew, ReportText(mountedReason));
			return true;
		}

		activeGroup.m_sSpawnFallbackMode = "convoy_driver_available";
		activeGroup.m_sSpawnFailureReason = "Convoy route snap recovery seated crew: " + seatingReason;
		activeGroup.m_vPosition = snapPosition;
		activeGroup.m_vSourcePosition = snapPosition;
		UpdateMissionConvoyAssetPosition(state, asset, snapPosition);

		float distanceToDestination = ResolveMissionConvoyDistanceToDestinationMeters(mission, asset, snapPosition);
		progress.m_vLastSamplePosition = snapPosition;
		progress.m_iLastSampleSecond = state.m_iElapsedSeconds;
		progress.m_iLastProgressSecond = state.m_iElapsedSeconds;
		progress.m_fDistanceToDestinationMeters = distanceToDestination;
		progress.m_fPreviousDistanceToDestinationMeters = distanceToDestination;
		progress.m_bNoProgress = false;
		progress.m_bHardStuck = false;
		progress.m_sLastProgressReason = "progress reset after route snap";
		string snapResult = string.Format("route snap succeeded to road point %1 | distance to destination %2m | forward %3 | moved unseated crew %4 | reseat %5", snapPosition, Math.Round(distanceToDestination), snapForward, movedCrew, ReportText(seatingReason));
		progress.m_sLastRecoveryResult = snapResult;

		if (progress.m_iRouteReissueAttemptCount < 2)
		{
			TryReissueMissionConvoyRouteForProgress(state, mission, activeGroup, progress, "route snap");
			progress.m_sLastRecoveryResult = snapResult + "; " + progress.m_sLastRecoveryResult;
		}

		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected bool TryResolveNearestConvoyRouteSnapPosition(HST_CampaignState state, HST_ActiveMissionState mission, vector currentPosition, out vector snapPosition, out vector snapForward, out string reason)
	{
		snapPosition = "0 0 0";
		snapForward = "0 0 1";
		reason = "";
		if (!state || !mission || IsZeroVector(currentPosition) || IsZeroVector(mission.m_vTargetPosition))
		{
			reason = "mission, current position, or destination missing";
			return false;
		}

		float roadWidth;
		float roadDistance;
		if (!HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(currentPosition, CONVOY_ROUTE_SNAP_SEARCH_RADIUS_METERS, mission.m_vTargetPosition, snapPosition, snapForward, roadWidth, roadDistance, reason))
		{
			reason = "no road-resolved snap point near stuck vehicle: " + reason;
			return false;
		}

		float snapDistanceFromVehicle = Math.Sqrt(DistanceSq2D(currentPosition, snapPosition));
		if (snapDistanceFromVehicle > CONVOY_ROUTE_SNAP_SEARCH_RADIUS_METERS)
		{
			reason = string.Format("road snap point %1m from vehicle exceeds %2m recovery search radius", Math.Round(snapDistanceFromVehicle), Math.Round(CONVOY_ROUTE_SNAP_SEARCH_RADIUS_METERS));
			return false;
		}

		float snapDistanceToDestination = Math.Sqrt(DistanceSq2D(snapPosition, mission.m_vTargetPosition));
		if (snapDistanceToDestination <= CONVOY_DESTINATION_RADIUS_METERS + CONVOY_ROUTE_SNAP_MIN_DESTINATION_DISTANCE_METERS)
		{
			reason = string.Format("road snap point is too close to destination for recovery snap: %1m", Math.Round(snapDistanceToDestination));
			return false;
		}

		float currentDistanceToDestination = Math.Sqrt(DistanceSq2D(currentPosition, mission.m_vTargetPosition));
		if (currentDistanceToDestination - snapDistanceToDestination > CONVOY_ROUTE_SNAP_MAX_ADVANCE_METERS)
		{
			reason = string.Format("road snap point would advance convoy %1m toward destination, above %2m snap limit", Math.Round(currentDistanceToDestination - snapDistanceToDestination), Math.Round(CONVOY_ROUTE_SNAP_MAX_ADVANCE_METERS));
			return false;
		}

		reason = string.Format("road snap point resolved | road distance %1m | road width %2m | destination distance %3m", Math.Round(roadDistance), Math.Round(roadWidth), Math.Round(snapDistanceToDestination));
		return true;
	}

	protected void AppendConvoyRouteSnapCandidate(array<vector> candidates, vector position)
	{
		if (!candidates || IsZeroVector(position))
			return;
		if (candidates.Count() > 0 && DistanceSq2D(candidates[candidates.Count() - 1], position) < 9.0)
			return;

		candidates.Insert(position);
	}

	protected vector ClosestPointOnSegment2D(vector segmentStart, vector segmentEnd, vector point)
	{
		vector result = segmentStart;
		float dx = segmentEnd[0] - segmentStart[0];
		float dz = segmentEnd[2] - segmentStart[2];
		float lengthSq = dx * dx + dz * dz;
		if (lengthSq <= 0.01)
			return result;

		float t = ((point[0] - segmentStart[0]) * dx + (point[2] - segmentStart[2]) * dz) / lengthSq;
		if (t < 0.0)
			t = 0.0;
		else if (t > 1.0)
			t = 1.0;

		result[0] = segmentStart[0] + dx * t;
		result[1] = segmentStart[1] + (segmentEnd[1] - segmentStart[1]) * t;
		result[2] = segmentStart[2] + dz * t;
		return result;
	}

	protected vector BuildConvoyRouteSnapForward(vector sourcePosition, vector targetPosition)
	{
		vector forward = "0 0 1";
		float dx = targetPosition[0] - sourcePosition[0];
		float dz = targetPosition[2] - sourcePosition[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length <= 0.01)
			return forward;

		forward[0] = dx / length;
		forward[1] = 0;
		forward[2] = dz / length;
		return forward;
	}

	protected bool TryAdvanceMissionConvoyFromStaging(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_ConvoyReadinessStatus readiness = BuildMissionConvoyReadinessStatus(state, mission);
		if (CanAttemptMissionConvoyWaypointAssignment(readiness) && AssignMissionConvoyWaypoints(state, mission))
			readiness = BuildMissionConvoyReadinessStatus(state, mission);

		if (readiness.m_bReadyToMove)
		{
			SetMissionConvoyMoving(state, mission);
			return true;
		}

		if (readiness.m_bPendingGrace)
			return false;

		if (readiness.m_bStaticFallbackAvailable)
		{
			SetMissionConvoyStaticFallback(state, mission, readiness.m_sReason);
			return true;
		}

		SetMissionConvoyFailure(state, mission, readiness.m_sReason);
		return true;
	}

	protected bool IsMissionConvoyMovementInterrupted(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePhase != MISSION_CONVOY_MOVING)
			return false;

		ref array<vector> routeWaypoints = BuildMissionConvoyWaypointPositions(ResolveMissionConvoyRoute(state, mission));
		if (!routeWaypoints || routeWaypoints.Count() <= 1)
			return true;

		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroupForMission(activeGroup, mission) || !activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED)
				continue;
			if (!IsMissionConvoyWaypointAssigned(activeGroup))
			{
				if (activeGroup.m_sSpawnFailureReason.IsEmpty() || !activeGroup.m_sSpawnFailureReason.Contains("Convoy moving recovery"))
					activeGroup.m_sSpawnFailureReason = "Convoy moving recovery pending: waypoint-chain assignment is missing.";
				continue;
			}
			if (IsMissionConvoyGroupFullyDismounted(activeGroup))
				continue;
			if (IsMissionConvoyCrewControlDegraded(activeGroup))
				continue;
		}

		return false;
	}

	protected bool IsMissionConvoyGroupFullyDismounted(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		string reason;
		bool dismounted = GetConvoyVehicleControlAdapter().AreAllLivingCrewDismounted(crewEntity, vehicleEntity, reason);
		if (dismounted && !reason.IsEmpty())
			activeGroup.m_sSpawnFailureReason = "Convoy moving recovery pending: all living crew dismounted; " + reason;

		return dismounted;
	}

	protected bool IsMissionConvoyCrewControlDegraded(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		string reason;
		if (GetConvoyVehicleControlAdapter().AreAllLivingCrewDismounted(crewEntity, vehicleEntity, reason))
		{
			if (!reason.IsEmpty())
				activeGroup.m_sSpawnFailureReason = "Convoy moving recovery pending: all living crew dismounted; " + reason;
			return true;
		}

		if (!GetConvoyVehicleControlAdapter().HasLivingDriver(crewEntity, vehicleEntity))
		{
			activeGroup.m_sSpawnFailureReason = "Convoy moving recovery pending: no seated living AI driver is assigned to the vehicle.";
			return true;
		}

		return false;
	}

	protected void SetMissionConvoyMoving(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		mission.m_sRuntimePhase = MISSION_CONVOY_MOVING;
		mission.m_sLastRuntimeEventKey = CONVOY_MOVE_EVENT_PENDING;
		ApplyMissionConvoyStatusToGroups(state, mission, MISSION_CONVOY_MOVING);
		m_bMarkerRefreshNeeded = true;
	}

	protected void SetMissionConvoyStaticFallback(HST_CampaignState state, HST_ActiveMissionState mission, string reason)
	{
		if (!state || !mission)
			return;

		if (reason.IsEmpty())
			reason = "Convoy readiness failed; convoy staged as a static ambush.";
		else if (!reason.Contains("static ambush"))
			reason = reason + " Convoy staged as a static ambush.";

		mission.m_sRuntimePhase = MISSION_CONVOY_CONTACT;
		mission.m_sRuntimeFailureReason = reason;
		ApplyMissionConvoyStatusToGroups(state, mission, MISSION_CONVOY_CONTACT);
		m_bMarkerRefreshNeeded = true;
		Print(string.Format("h-istasi mission convoy | %1 staged as static ambush: %2", mission.m_sInstanceId, mission.m_sRuntimeFailureReason), LogLevel.WARNING);
	}

	protected HST_ConvoyCompletionStatus BuildMissionConvoyCompletionStatus(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_ConvoyCompletionStatus status = new HST_ConvoyCompletionStatus();
		if (!state || !mission)
		{
			status.m_sReason = "state or mission missing";
			return status;
		}

		int assetIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, assetIndex);
			assetIndex++;
			status.m_iTotalVehicleAssets++;
			status.m_iRequiredCrewGroups++;

			bool resolved = IsMissionConvoyVehicleAssetResolved(asset);
			if (asset.m_bDestroyed)
				status.m_iDestroyedVehicles++;
			if (asset.m_bDelivered || asset.m_sLastInteraction == "captured")
				status.m_iCapturedVehicles++;
			if (!resolved)
				status.m_iActiveVehicles++;

			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			if (!activeGroup)
			{
				status.m_iMissingCrewGroups++;
				continue;
			}

			if (!IsMissionConvoyCrewEliminationObservable(state, activeGroup))
			{
				status.m_iPendingCrewGroups++;
				status.m_iLivingCrew += Math.Max(0, ResolveMissionConvoyRestorableCrewCount(state, activeGroup));
				continue;
			}

			int aliveCrew = CountAliveRuntimeCrewAgents(activeGroup);
			if (IsMissionConvoyCrewGroupEliminated(state, activeGroup, aliveCrew))
			{
				status.m_iEliminatedCrewGroups++;
			}
			else
			{
				int knownLivingCrew = Math.Max(aliveCrew, Math.Max(activeGroup.m_iLastSeenAliveCount, activeGroup.m_iSurvivorInfantryCount));
				status.m_iLivingCrew += Math.Max(0, knownLivingCrew);
			}

			if (resolved || aliveCrew <= 0)
				continue;

			vector position = ResolveMissionConvoyVehiclePosition(asset, groupId);
			if (!IsMissionConvoyAtDestination(mission, position))
				continue;

			status.m_bAnyLiveCrewArrived = true;
			status.m_sReason = "live-crewed convoy vehicle reached destination";
		}

		status.m_bAllCrewsEliminated = status.m_iRequiredCrewGroups > 0 && status.m_iMissingCrewGroups == 0 && status.m_iPendingCrewGroups == 0 && status.m_iEliminatedCrewGroups >= status.m_iRequiredCrewGroups;
		status.m_bCanComplete = status.m_bAllCrewsEliminated;
		status.m_bMustFail = status.m_bAnyLiveCrewArrived;
		if (status.m_sReason.IsEmpty())
		{
			if (status.m_bCanComplete)
				status.m_sReason = "all convoy crews eliminated";
			else if (status.m_iMissingCrewGroups > 0 || status.m_iPendingCrewGroups > 0)
				status.m_sReason = string.Format("crew runtime pending | missing %1 | pending %2 | eliminated %3/%4 | living crew %5", status.m_iMissingCrewGroups, status.m_iPendingCrewGroups, status.m_iEliminatedCrewGroups, status.m_iRequiredCrewGroups, status.m_iLivingCrew);
			else
				status.m_sReason = string.Format("crew progress %1/%2 eliminated | living crew %3", status.m_iEliminatedCrewGroups, status.m_iRequiredCrewGroups, status.m_iLivingCrew);
		}

		return status;
	}

	protected string BuildMissionConvoyCompletionReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_ConvoyCompletionStatus status = BuildMissionConvoyCompletionStatus(state, mission);
		string report = string.Format("\n  convoy completion | can complete %1 | must fail %2 | reason %3", ReportBool(status.m_bCanComplete), ReportBool(status.m_bMustFail), ReportText(status.m_sReason));
		report = report + string.Format("\n    convoy completion groups %1 | eliminated %2 | missing %3 | pending %4 | living crew %5", status.m_iRequiredCrewGroups, status.m_iEliminatedCrewGroups, status.m_iMissingCrewGroups, status.m_iPendingCrewGroups, status.m_iLivingCrew);
		report = report + string.Format("\n    convoy completion vehicles total %1 | active %2 | destroyed %3 | captured %4", status.m_iTotalVehicleAssets, status.m_iActiveVehicles, status.m_iDestroyedVehicles, status.m_iCapturedVehicles);
		return report;
	}

	protected bool IsMissionConvoyCrewEliminationObservable(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;
		if (activeGroup.m_sRuntimeStatus == "spawn_failed")
			return false;
		if (!HasMissionConvoyCrewEverBeenObservedAlive(activeGroup))
			return false;
		if (activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_sRuntimeStatus == "eliminated")
			return HasMissionConvoyExplicitEliminationContext(state, activeGroup);
		if (IsConvoyCrewControlPending(state, activeGroup))
			return false;
		if (!activeGroup.m_bSpawnedEntity)
			return false;
		if (!GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId))
			return false;
		if (activeGroup.m_iSpawnedAgentCount <= 0 && activeGroup.m_iLastSeenAliveCount <= 0 && activeGroup.m_iSurvivorInfantryCount <= 0)
			return false;

		return HasMissionConvoyExplicitEliminationContext(state, activeGroup);
	}

	protected bool ApplyMissionConvoyEliminatedGroupStatuses(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroupForMission(activeGroup, mission))
				continue;
			if (activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED)
				continue;

			int aliveCrew = CountAliveRuntimeCrewAgents(activeGroup);
			if (!IsMissionConvoyCrewGroupEliminated(state, activeGroup, aliveCrew))
				continue;

			activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
			activeGroup.m_iLastSeenAliveCount = 0;
			activeGroup.m_iSurvivorInfantryCount = 0;
			changed = true;
		}

		return changed;
	}

	protected bool CompleteGenericMissionConvoy(HST_CampaignState state, HST_ActiveMissionState mission, HST_ConvoyCompletionStatus status)
	{
		if (!state || !mission || !status)
			return false;

		bool changed;
		if (mission.m_sRuntimePhase != MISSION_CONVOY_ELIMINATED)
		{
			mission.m_sRuntimePhase = MISSION_CONVOY_ELIMINATED;
			mission.m_sLastRuntimeEventKey = CONVOY_COMPLETE_EVENT_KEY;
			mission.m_sRuntimeFailureReason = "";
			changed = true;
		}

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_sTargetId != "convoy" || objective.m_bFailed)
				continue;

			int required = Math.Max(1, status.m_iRequiredCrewGroups);
			if (objective.m_iRequiredCount != required || objective.m_iRequiredProgress != required)
			{
				objective.m_iRequiredCount = required;
				objective.m_iRequiredProgress = required;
				changed = true;
			}
			if (objective.m_iCurrentCount != required || objective.m_iCurrentProgress != required)
			{
				objective.m_iCurrentCount = required;
				objective.m_iCurrentProgress = required;
				changed = true;
			}
			if (!objective.m_bComplete)
			{
				objective.m_bComplete = true;
				changed = true;
			}
		}

		changed = ApplyMissionConvoyStatusToGroups(state, mission, MISSION_CONVOY_ELIMINATED) || changed;
		if (changed)
		{
			m_bMarkerRefreshNeeded = true;
			Print(string.Format("h-istasi mission convoy | %1 generic completion: %2/%3 crews eliminated", mission.m_sInstanceId, status.m_iEliminatedCrewGroups, status.m_iRequiredCrewGroups));
		}

		return changed;
	}

	protected bool UpdateMissionConvoyObjective(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool changed = ApplyMissionConvoyEliminatedGroupStatuses(state, mission);
		HST_ConvoyCompletionStatus completion = BuildMissionConvoyCompletionStatus(state, mission);
		changed = ApplyMissionConvoyObjectiveProgress(state, mission, completion.m_iEliminatedCrewGroups, completion.m_iRequiredCrewGroups) || changed;

		if (completion.m_bMustFail)
		{
			SetMissionConvoyFailure(state, mission, "Convoy reached its destination with living crew.");
			return true;
		}

		if (completion.m_bCanComplete)
			return CompleteGenericMissionConvoy(state, mission, completion) || changed;

		return changed;
	}

	protected string BuildMissionConvoyRuntimeReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "";

		int vehicleAssetCount = CountMissionConvoyVehicleAssets(state, mission.m_sInstanceId);
		vector sourcePosition = ResolveMissionConvoySourcePosition(state, mission);
		vector targetPosition = ResolveMissionConvoyTargetPosition(state, mission);
		int travelDistanceMeters = Math.Round(Math.Sqrt(DistanceSq2D(sourcePosition, targetPosition)));
		HST_GeneratedRouteState route = ResolveMissionConvoyRoute(state, mission);
		string routeId = ResolveMissionConvoyRouteId(state, mission);
		string report = string.Format("\nconvoy mission | instance %1 | mission %2 | status %3 | phase %4", ReportText(mission.m_sInstanceId), ReportText(mission.m_sMissionId), mission.m_eStatus, ReportText(mission.m_sRuntimePhase));
		report = report + string.Format(" | ETA %1 | source position %2 | target position %3 | travel distance %4m", mission.m_iRuntimeETASeconds, sourcePosition, targetPosition, travelDistanceMeters);
		report = report + string.Format(" | route/site ID %1 | route ID %2", ReportRouteSite(state, mission.m_sSiteId), ReportRouteSite(state, routeId));
		report = report + string.Format(" | vehicle asset count %1 | mission failure reason %2", vehicleAssetCount, ReportText(mission.m_sRuntimeFailureReason));
		report = report + BuildMissionConvoyRouteReport(route);
		report = report + BuildMissionConvoyReadinessReport(state, mission);
		report = report + BuildMissionConvoyCompletionReport(state, mission);
		report = report + BuildMissionConvoyContactReport(state, mission);
		report = report + BuildMissionConvoyOutcomeReport(state, mission);

		int assetIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, assetIndex);
			report = report + BuildConvoyVehicleAssetReport(state, mission, asset, groupId);
			assetIndex++;
		}

		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroupForMission(activeGroup, mission))
				continue;

			report = report + BuildConvoyActiveGroupReport(activeGroup, route);
		}

		return report;
	}

	protected string BuildMissionConvoyOutcomeReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "";

		string report = string.Format("\n  convoy outcome | summary %1", ReportText(mission.m_sConvoyOutcomeSummary));
		report = report + string.Format(" | arrival %1 | crew %2 | vehicle %3", ReportBool(mission.m_bConvoyArrivalOutcomeApplied), ReportBool(mission.m_bConvoyCrewEliminatedOutcomeApplied), ReportBool(mission.m_bConvoyVehicleCapturedOutcomeApplied));
		report = report + string.Format(" | cargo %1 | expired %2", ReportBool(mission.m_bConvoyCargoDeliveredOutcomeApplied), ReportBool(mission.m_bConvoyExpiredOutcomeApplied));
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE && asset.m_sRole != MISSION_CONVOY_PAYLOAD_ROLE && asset.m_sRole != MISSION_CONVOY_CAPTIVE_ROLE)
				continue;

			report = report + string.Format("\n    convoy asset outcome | asset %1 | role %2", ReportText(asset.m_sAssetId), ReportText(asset.m_sRole));
			report = report + string.Format(" | delivered %1 | destroyed %2 | applied %3", ReportBool(asset.m_bDelivered), ReportBool(asset.m_bDestroyed), ReportBool(asset.m_bOutcomeApplied));
			report = report + " | kind " + ReportText(asset.m_sOutcomeKind);
			if ((asset.m_sRole == MISSION_CONVOY_PAYLOAD_ROLE || asset.m_sRole == MISSION_CONVOY_CAPTIVE_ROLE) && !asset.m_bPickedUp && !asset.m_bDelivered && !asset.m_bDestroyed)
				report = report + " | access any surviving convoy vehicle";
		}

		return report;
	}

	protected string BuildConvoyVehicleAssetReport(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, string groupId)
	{
		if (!asset)
			return "\n  convoy vehicle asset | missing";

		vector vehiclePosition = ResolveMissionConvoyVehiclePosition(asset, groupId);
		bool spawned = asset.m_bSpawned || GetRuntimeVehicleEntity(groupId) != null;
		string report = string.Format("\n  convoy vehicle asset | asset %1 | prefab %2 | source position %3", ReportText(asset.m_sAssetId), ReportText(asset.m_sPrefab), asset.m_vSourcePosition);
		report = report + string.Format(" | current position %1 | target position %2 | spawned %3", vehiclePosition, asset.m_vTargetPosition, ReportBool(spawned));
		report = report + string.Format(" | resolved %1 | destroyed %2 | delivered/captured %3 | last interaction %4", ReportBool(IsMissionConvoyVehicleAssetResolved(asset)), ReportBool(asset.m_bDestroyed), ReportBool(asset.m_bDelivered), ReportText(asset.m_sLastInteraction));
		report = report + BuildConvoyRoadResolverReport("source road", asset.m_vSourcePosition, asset.m_vTargetPosition, CONVOY_ROAD_REPORT_SEARCH_RADIUS_METERS);
		report = report + BuildConvoyRoadResolverReport("current road", vehiclePosition, asset.m_vTargetPosition, CONVOY_ROAD_REPORT_SEARCH_RADIUS_METERS);
		report = report + BuildConvoyRoadResolverReport("target road", asset.m_vTargetPosition, asset.m_vSourcePosition, CONVOY_ROAD_REPORT_SEARCH_RADIUS_METERS);
		report = report + BuildConvoyVehicleProgressReport(state, mission, asset, groupId, vehiclePosition);
		return report;
	}

	protected string BuildConvoyVehicleProgressReport(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, string groupId, vector vehiclePosition)
	{
		float distanceToDestination = ResolveMissionConvoyDistanceToDestinationMeters(mission, asset, vehiclePosition);
		HST_ConvoyProgressStatus progress = FindConvoyProgressStatus(mission, asset, groupId);
		if (!progress)
			return string.Format("\n    convoy progress | distance to destination %1m | last progress not sampled | no-progress no | hard stuck no | route reissue 0 | route snap 0 | nearest player unknown | result not sampled", Math.Round(distanceToDestination));

		int lastProgressAge;
		if (state && progress.m_iLastProgressSecond > 0)
			lastProgressAge = Math.Max(0, state.m_iElapsedSeconds - progress.m_iLastProgressSecond);

		string nearestPlayer = "none";
		if (progress.m_fNearestPlayerDistanceMeters >= 0)
			nearestPlayer = string.Format("%1m", Math.Round(progress.m_fNearestPlayerDistanceMeters));

		string report = string.Format("\n    convoy progress | distance to destination %1m | sampled distance %2m | last progress age %3s", Math.Round(distanceToDestination), Math.Round(progress.m_fDistanceToDestinationMeters), lastProgressAge);
		report = report + string.Format(" | no-progress %1 | hard stuck %2", ReportBool(progress.m_bNoProgress), ReportBool(progress.m_bHardStuck));
		report = report + string.Format(" | route reissue %1 | route snap %2", progress.m_iRouteReissueAttemptCount, progress.m_iRouteSnapAttemptCount);
		report = report + string.Format(" | nearest player %1 | snap gate %2m", nearestPlayer, Math.Round(CONVOY_UNSTUCK_MIN_PLAYER_DISTANCE_METERS));
		report = report + string.Format(" | progress reason %1 | recovery %2", ReportText(progress.m_sLastProgressReason), ReportText(progress.m_sLastRecoveryResult));
		return report;
	}

	protected string BuildConvoyActiveGroupReport(HST_ActiveGroupState activeGroup, HST_GeneratedRouteState route)
	{
		if (!activeGroup)
			return "\n  convoy group | missing";

		bool crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) != null;
		bool vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId) != null;
		int aliveCrew = CountAliveRuntimeCrewAgents(activeGroup);
		string report = string.Format("\n  convoy group | group %1 | faction %2 | prefab %3", ReportText(activeGroup.m_sGroupId), ReportText(activeGroup.m_sFactionKey), ReportText(activeGroup.m_sPrefab));
		report = report + string.Format(" | spawned entity %1 | crew entity %2 | vehicle entity %3", ReportBool(activeGroup.m_bSpawnedEntity), ReportBool(crewEntity), ReportBool(vehicleEntity));
		report = report + string.Format(" | runtime status %1 | crew count %2 | alive crew count %3", ReportText(activeGroup.m_sRuntimeStatus), activeGroup.m_iInfantryCount, aliveCrew);
		report = report + string.Format(" | route ID %1 | source position %2 | target position %3", ReportText(activeGroup.m_sRouteId), activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition);
		report = report + string.Format(" | fallback mode %1 | assigned waypoint count %2", ReportText(activeGroup.m_sSpawnFallbackMode), activeGroup.m_iAssignedWaypointCount);
		report = report + " | spawn failure reason " + ReportText(ResolveConvoyAdapterReportReason(activeGroup));
		report = report + BuildConvoyAssignedWaypointChainReport(activeGroup, route);
		report = report + BuildConvoyVehicleControlAdapterReport(activeGroup);
		return report;
	}

	protected string BuildConvoyAssignedWaypointChainReport(HST_ActiveGroupState activeGroup, HST_GeneratedRouteState route)
	{
		if (!activeGroup)
			return "\n    assigned road waypoint chain | group missing";

		ref array<vector> routeWaypoints = BuildMissionConvoyWaypointPositions(route);
		ref array<vector> groupWaypoints = BuildMissionConvoyGroupWaypointPositions(activeGroup, routeWaypoints);
		int waypointCount = groupWaypoints.Count();
		if (waypointCount == 0)
			return "\n    assigned road waypoint chain | computed count 0 | road-resolved no | reason no road-resolved runtime waypoint chain";

		int roadResolvedCount;
		string firstFailure;
		foreach (vector waypointPosition : groupWaypoints)
		{
			vector roadPosition;
			vector roadForward;
			float roadWidth;
			float roadDistance;
			string roadReason;
			if (HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(waypointPosition, CONVOY_ROAD_REPORT_SEARCH_RADIUS_METERS, activeGroup.m_vTargetPosition, roadPosition, roadForward, roadWidth, roadDistance, roadReason))
			{
				roadResolvedCount++;
				continue;
			}

			if (firstFailure.IsEmpty())
				firstFailure = roadReason;
		}

		bool allRoadResolved = roadResolvedCount == waypointCount;
		int middleIndex = waypointCount / 2;
		int lastIndex = waypointCount - 1;
		string report = string.Format("\n    assigned road waypoint chain | assigned count %1 | computed count %2 | road-resolved %3 | road points %4/%5", activeGroup.m_iAssignedWaypointCount, waypointCount, ReportBool(allRoadResolved), roadResolvedCount, waypointCount);
		report = report + string.Format(" | first %1 | mid %2 | final %3", groupWaypoints[0], groupWaypoints[middleIndex], groupWaypoints[lastIndex]);
		if (!firstFailure.IsEmpty())
			report = report + " | first failure " + ReportText(firstFailure);

		return report;
	}

	protected string BuildConvoyVehicleControlAdapterReport(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "\n    convoy adapter | group missing";

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		string mobileReason;
		bool mobile = GetConvoyVehicleControlAdapter().IsVehicleMobile(vehicleEntity, mobileReason);
		bool driver = GetConvoyVehicleControlAdapter().HasLivingDriver(crewEntity, vehicleEntity);
		int adapterCrew = GetConvoyVehicleControlAdapter().CountLivingCrew(crewEntity);
		string seatingReport = GetConvoyVehicleControlAdapter().BuildCrewSeatingReport(crewEntity, vehicleEntity);
		string result = activeGroup.m_sSpawnFallbackMode;
		if (result.IsEmpty())
			result = "none";
		string reason = ResolveConvoyAdapterReportReason(activeGroup);
		if (reason.IsEmpty())
			reason = mobileReason;

		string report = string.Format("\n    convoy adapter | living crew %1 | driver assigned %2 | vehicle mobile %3", adapterCrew, ReportBool(driver), ReportBool(mobile));
		report = report + " | " + seatingReport;
		report = report + string.Format(" | mobile reason %1 | last result %2 | reason %3", ReportText(mobileReason), ReportText(result), ReportText(reason));
		return report;
	}

	protected string ResolveConvoyAdapterReportReason(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "";

		string reason = activeGroup.m_sSpawnFailureReason;
		if (!IsStaleConvoyCrewPopulationReason(reason))
			return reason;

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		int adapterCrew = GetConvoyVehicleControlAdapter().CountLivingCrew(crewEntity);
		if (adapterCrew <= 0)
			return reason;

		string mobileReason;
		if (!GetConvoyVehicleControlAdapter().IsVehicleMobile(vehicleEntity, mobileReason))
			return "Convoy adapter cannot bind crew: " + mobileReason;

		if (!GetConvoyVehicleControlAdapter().HasLivingDriver(crewEntity, vehicleEntity))
			return "Convoy adapter cannot bind crew: no seated living AI driver is assigned to the vehicle.";

		return "Convoy adapter found seated driver and mobile vehicle.";
	}

	protected bool IsStaleConvoyCrewPopulationReason(string reason)
	{
		return reason == "Convoy adapter cannot bind crew: no living crew agents." || reason == "Convoy adapter cannot bind crew yet: crew group has not populated living agents." || reason.Contains("planned for Phase") || reason.Contains("planned for later convoy phases");
	}

	protected string BuildMissionConvoyRouteReport(HST_GeneratedRouteState route)
	{
		if (!route)
			return "\n  convoy route | route none | road no | vehicle-safe no | waypoint count 0 | distance 0m | validation missing generated route";

		string validation = route.m_sValidationFailureReason;
		if (validation.IsEmpty())
			validation = "none";
		string roadManagerReason;
		bool roadManagerAvailable = HST_WorldPositionService.IsRoadNetworkAvailable(roadManagerReason);
		string report = string.Format("\n  convoy route | route %1 | road %2 | vehicle-safe %3", ReportText(route.m_sRouteId), ReportBool(route.m_bRoadRoute), ReportBool(route.m_bValidatedForVehicles));
		report = report + string.Format(" | waypoint count %1 | distance %2m | validation %3 | road manager %4 %5", route.m_iWaypointCount, route.m_iDistanceMeters, validation, ReportBool(roadManagerAvailable), ReportText(roadManagerReason));
		foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
		{
			if (!waypoint)
				continue;

			report = report + string.Format("\n    active route waypoint template %1 | hint %2 | radius %3m | position %4 | road check assigned-runtime-chain", waypoint.m_iIndex, ReportText(waypoint.m_sHint), waypoint.m_iRadiusMeters, waypoint.m_vPosition);
		}

		return report;
	}

	protected string BuildConvoyRoadResolverReport(string label, vector position, vector destination, float searchRadius)
	{
		string roadManagerReason;
		bool roadManagerAvailable = HST_WorldPositionService.IsRoadNetworkAvailable(roadManagerReason);
		vector roadPosition;
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		bool roadResolved = HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(position, searchRadius, destination, roadPosition, roadForward, roadWidth, roadDistance, roadReason);
		if (!roadResolved)
			return string.Format("\n    %1 | road manager %2 %3 | road-resolved no | reason %4", label, ReportBool(roadManagerAvailable), ReportText(roadManagerReason), ReportText(roadReason));

		return string.Format("\n    %1 | road manager %2 %3 | road-resolved yes | road position %4 | forward %5 | width %6m | resolver distance %7m", label, ReportBool(roadManagerAvailable), ReportText(roadManagerReason), roadPosition, roadForward, Math.Round(roadWidth), Math.Round(roadDistance));
	}

	protected string BuildMissionConvoyReadinessReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_ConvoyReadinessStatus readiness = BuildMissionConvoyReadinessStatus(state, mission);
		string report = string.Format("\n  convoy readiness | ready %1 | pending grace %2 | static fallback %3 | reason %4", ReportBool(readiness.m_bReadyToMove), ReportBool(readiness.m_bPendingGrace), ReportBool(readiness.m_bStaticFallbackAvailable), ReportText(readiness.m_sReason));
		report = report + string.Format("\n    convoy readiness vehicle assets planned %1 | active %2 | resolved %3 | spawned vehicles %4", readiness.m_iVehicleAssetCount, readiness.m_iActiveVehicleAssetCount, readiness.m_iResolvedVehicleAssetCount, readiness.m_iSpawnedVehicleCount);
		report = report + string.Format("\n    convoy readiness crew groups %1 | alive crew groups %2 | alive crew count %3", readiness.m_iCrewGroupCount, readiness.m_iAliveCrewGroupCount, readiness.m_iAliveCrewCount);
		report = report + string.Format("\n    convoy readiness driver availability %1 | mobile vehicles %2 | route assignment %3 | waypoint assignment %4", readiness.m_iDriverAvailableCount, readiness.m_iMobileVehicleCount, readiness.m_iRouteAssignedCount, readiness.m_iWaypointAssignedCount);
		return report;
	}

	protected HST_ConvoyReadinessStatus BuildMissionConvoyReadinessStatus(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_ConvoyReadinessStatus readiness = new HST_ConvoyReadinessStatus();
		if (!state || !mission)
		{
			readiness.m_sReason = "Convoy readiness state or mission missing.";
			return readiness;
		}

		HST_GeneratedRouteState route = ResolveMissionConvoyRoute(state, mission);
		string routeId = ResolveMissionConvoyRouteId(state, mission);
		int assetIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, assetIndex);
			assetIndex++;
			readiness.m_iVehicleAssetCount++;
			if (IsMissionConvoyVehicleAssetResolved(asset))
			{
				readiness.m_iResolvedVehicleAssetCount++;
				continue;
			}

			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			readiness.m_iActiveVehicleAssetCount++;
			if (GetRuntimeVehicleEntity(groupId) != null)
				readiness.m_iSpawnedVehicleCount++;
			if (IsMissionConvoyCrewGroupSpawned(activeGroup))
				readiness.m_iCrewGroupCount++;

			int aliveCrew = CountAliveRuntimeCrewAgents(activeGroup);
			if (aliveCrew > 0)
				readiness.m_iAliveCrewGroupCount++;
			readiness.m_iAliveCrewCount += Math.Max(0, aliveCrew);

			if (HasMissionConvoyDriverAvailable(activeGroup))
				readiness.m_iDriverAvailableCount++;
			if (IsMissionConvoyVehicleMobile(activeGroup))
				readiness.m_iMobileVehicleCount++;
			if (IsMissionConvoyRouteAssigned(route, routeId, activeGroup))
				readiness.m_iRouteAssignedCount++;
			if (IsMissionConvoyWaypointAssigned(activeGroup))
				readiness.m_iWaypointAssignedCount++;
		}

		readiness.m_bReadyToMove = IsMissionConvoyReadyToMove(readiness);
		readiness.m_bStaticFallbackAvailable = IsMissionConvoyStaticFallbackAvailable(readiness);
		readiness.m_bPendingGrace = IsMissionConvoyReadinessGraceActive(mission) || HasMissionConvoyControlPending(state, mission);
		readiness.m_sReason = ResolveMissionConvoyReadinessReason(readiness);
		return readiness;
	}

	protected bool IsMissionConvoyCrewGroupSpawned(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup || !activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_failed")
			return false;

		return GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) != null;
	}

	protected bool HasMissionConvoyDriverAvailable(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		return GetConvoyVehicleControlAdapter().HasLivingDriver(crewEntity, vehicleEntity);
	}

	protected bool IsMissionConvoyVehicleMobile(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		string reason;
		return GetConvoyVehicleControlAdapter().IsVehicleMobile(GetRuntimeVehicleEntity(activeGroup.m_sGroupId), reason);
	}

	protected bool IsMissionConvoyRouteAssigned(HST_GeneratedRouteState route, string routeId, HST_ActiveGroupState activeGroup)
	{
		if (!route || routeId.IsEmpty() || !activeGroup || activeGroup.m_sRouteId.IsEmpty())
			return false;
		if (route.m_iWaypointCount < 3)
			return false;

		return activeGroup.m_sRouteId == routeId;
	}

	protected bool IsMissionConvoyWaypointAssigned(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		return activeGroup.m_sSpawnFallbackMode == "convoy_waypoints" && activeGroup.m_iAssignedWaypointCount > 1;
	}

	protected bool IsMissionConvoyReadyToMove(HST_ConvoyReadinessStatus readiness)
	{
		if (!readiness || readiness.m_iActiveVehicleAssetCount < 3)
			return false;

		int required = readiness.m_iActiveVehicleAssetCount;
		return readiness.m_iSpawnedVehicleCount >= required && readiness.m_iCrewGroupCount >= required && readiness.m_iAliveCrewGroupCount >= required && readiness.m_iMobileVehicleCount >= required && readiness.m_iDriverAvailableCount >= required && readiness.m_iRouteAssignedCount >= required && readiness.m_iWaypointAssignedCount >= required;
	}

	protected bool IsMissionConvoyStaticFallbackAvailable(HST_ConvoyReadinessStatus readiness)
	{
		if (!readiness || readiness.m_iActiveVehicleAssetCount < 3)
			return false;

		int required = readiness.m_iActiveVehicleAssetCount;
		return readiness.m_iSpawnedVehicleCount >= required && readiness.m_iCrewGroupCount >= required;
	}

	protected bool CanAttemptMissionConvoyWaypointAssignment(HST_ConvoyReadinessStatus readiness)
	{
		if (!readiness || readiness.m_iActiveVehicleAssetCount < 3)
			return false;

		int required = readiness.m_iActiveVehicleAssetCount;
		return readiness.m_iSpawnedVehicleCount >= required && readiness.m_iCrewGroupCount >= required && readiness.m_iAliveCrewGroupCount >= required && readiness.m_iMobileVehicleCount >= required && readiness.m_iDriverAvailableCount >= required && readiness.m_iRouteAssignedCount >= required && readiness.m_iWaypointAssignedCount < required;
	}

	protected bool IsMissionConvoyReadinessGraceActive(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sRuntimePhase != MISSION_CONVOY_STAGING || mission.m_iRuntimeCounterB <= 0)
			return false;

		int elapsedPastStaging = mission.m_iRuntimeCounterA - mission.m_iRuntimeCounterB;
		return elapsedPastStaging >= 0 && elapsedPastStaging < CONVOY_READINESS_GRACE_SECONDS;
	}

	protected string ResolveMissionConvoyReadinessReason(HST_ConvoyReadinessStatus readiness)
	{
		if (!readiness)
			return "Convoy readiness state missing.";
		if (readiness.m_iVehicleAssetCount <= 0)
			return "No planned convoy vehicle assets exist; convoy cannot move.";
		if (readiness.m_iVehicleAssetCount < 3)
			return "Convoy needs at least three planned vehicle assets for readiness.";
		if (readiness.m_iActiveVehicleAssetCount <= 0)
			return string.Format("No active convoy vehicle assets remain; planned %1 resolved %2.", readiness.m_iVehicleAssetCount, readiness.m_iResolvedVehicleAssetCount);
		if (readiness.m_iActiveVehicleAssetCount < 3)
			return string.Format("Convoy needs at least three active vehicle assets for movement; active %1 planned %2 resolved %3.", readiness.m_iActiveVehicleAssetCount, readiness.m_iVehicleAssetCount, readiness.m_iResolvedVehicleAssetCount);
		if (readiness.m_iSpawnedVehicleCount <= 0)
			return "No convoy vehicles spawned; convoy cannot move.";
		if (readiness.m_iSpawnedVehicleCount < readiness.m_iActiveVehicleAssetCount)
			return "Not all convoy vehicle assets spawned physical vehicles.";
		if (readiness.m_iCrewGroupCount <= 0)
			return "Convoy crew groups failed to spawn.";
		if (readiness.m_iCrewGroupCount < readiness.m_iActiveVehicleAssetCount)
			return "Not all convoy crew groups spawned.";
		if (readiness.m_iAliveCrewCount <= 0)
			return "Convoy crew groups have no living crew.";
		if (readiness.m_iAliveCrewGroupCount < readiness.m_iActiveVehicleAssetCount)
			return "Not all convoy crew groups have living crew.";
		if (readiness.m_iMobileVehicleCount < readiness.m_iActiveVehicleAssetCount)
			return "Not all convoy vehicles are mobile according to the vehicle-control adapter.";
		if (readiness.m_iDriverAvailableCount < readiness.m_iActiveVehicleAssetCount)
			return "Convoy has no seated living AI driver yet.";
		if (readiness.m_iRouteAssignedCount < readiness.m_iActiveVehicleAssetCount)
			return "Convoy route assignment failed.";
		if (readiness.m_iWaypointAssignedCount < readiness.m_iActiveVehicleAssetCount)
			return "Convoy waypoint assignment failed.";

		return "ready";
	}

	protected int CountMissionConvoyVehicleAssets(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return 0;

		int count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == instanceId && asset.m_sRole == MISSION_CONVOY_VEHICLE_ROLE)
				count++;
		}

		return count;
	}

	protected vector ResolveMissionConvoySourcePosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == MISSION_CONVOY_VEHICLE_ROLE)
				return asset.m_vSourcePosition;
		}

		return mission.m_vTargetPosition;
	}

	protected vector ResolveMissionConvoyTargetPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == MISSION_CONVOY_VEHICLE_ROLE)
				return asset.m_vTargetPosition;
		}

		return mission.m_vTargetPosition;
	}

	protected string ResolveMissionConvoyRouteId(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_GeneratedRouteState route = ResolveMissionConvoyRoute(state, mission);
		if (route)
			return route.m_sRouteId;

		return "";
	}

	protected HST_ConvoyProgressStatus EnsureConvoyProgressStatus(HST_ActiveMissionState mission, HST_MissionAssetState asset, string groupId)
	{
		string key = BuildConvoyProgressKey(mission, asset, groupId);
		foreach (HST_ConvoyProgressStatus progress : m_aConvoyProgressStatuses)
		{
			if (progress && progress.m_sKey == key)
				return progress;
		}

		HST_ConvoyProgressStatus created = new HST_ConvoyProgressStatus();
		created.m_sKey = key;
		if (mission)
			created.m_sMissionInstanceId = mission.m_sInstanceId;
		if (asset)
			created.m_sAssetId = asset.m_sAssetId;
		created.m_sGroupId = groupId;
		created.m_sLastProgressReason = "not sampled";
		created.m_sLastRecoveryResult = "none";
		m_aConvoyProgressStatuses.Insert(created);
		return created;
	}

	protected HST_ConvoyProgressStatus FindConvoyProgressStatus(HST_ActiveMissionState mission, HST_MissionAssetState asset, string groupId)
	{
		string key = BuildConvoyProgressKey(mission, asset, groupId);
		foreach (HST_ConvoyProgressStatus progress : m_aConvoyProgressStatuses)
		{
			if (progress && progress.m_sKey == key)
				return progress;
		}

		return null;
	}

	protected string BuildConvoyProgressKey(HST_ActiveMissionState mission, HST_MissionAssetState asset, string groupId)
	{
		string missionId;
		if (mission)
			missionId = mission.m_sInstanceId;
		string assetId;
		if (asset)
			assetId = asset.m_sAssetId;

		return string.Format("%1|%2|%3", missionId, assetId, groupId);
	}

	protected float ResolveMissionConvoyDistanceToDestinationMeters(HST_ActiveMissionState mission, HST_MissionAssetState asset, vector position)
	{
		vector target;
		if (asset && !IsZeroVector(asset.m_vTargetPosition))
			target = asset.m_vTargetPosition;
		else if (mission)
			target = mission.m_vTargetPosition;

		if (IsZeroVector(position) || IsZeroVector(target))
			return 0;

		return Math.Sqrt(DistanceSq2D(position, target));
	}

	protected HST_GeneratedRouteState ResolveMissionConvoyRoute(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sSiteId.IsEmpty())
			return null;

		HST_GeneratedSiteState site = state.FindGeneratedSite(mission.m_sSiteId);
		if (!site || site.m_sRouteId.IsEmpty())
			return null;

		return state.FindGeneratedRoute(site.m_sRouteId);
	}

	protected bool IsMissionConvoyGroupForMission(HST_ActiveGroupState activeGroup, HST_ActiveMissionState mission)
	{
		if (!IsMissionConvoyGroup(activeGroup))
			return false;

		return IsMissionConvoyGroupIdForMission(activeGroup.m_sGroupId, mission);
	}

	protected bool IsMissionConvoyGroupIdForMission(string groupId, HST_ActiveMissionState mission)
	{
		if (groupId.IsEmpty() || !mission || mission.m_sInstanceId.IsEmpty())
			return false;

		string prefix = string.Format("%1%2_", MISSION_CONVOY_GROUP_PREFIX, mission.m_sInstanceId);
		return groupId.Contains(prefix);
	}

	protected bool IsMissionConvoyVehicleAssetResolved(HST_MissionAssetState asset)
	{
		if (!asset)
			return true;

		return asset.m_bDestroyed || asset.m_bDelivered || asset.m_sLastInteraction == "captured";
	}

	protected bool IsMissionConvoyCrewGroupEliminated(HST_CampaignState state, HST_ActiveGroupState activeGroup, int aliveCrew)
	{
		if (!activeGroup || activeGroup.m_sRuntimeStatus == "spawn_failed")
			return false;
		if (aliveCrew > 0)
			return false;
		if (!HasMissionConvoyCrewEverBeenObservedAlive(activeGroup))
			return false;
		if (activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_sRuntimeStatus == "eliminated")
			return HasMissionConvoyExplicitEliminationContext(state, activeGroup);
		if (!IsMissionConvoyCrewEliminationObservable(state, activeGroup))
			return false;

		return true;
	}

	protected bool HasMissionConvoyExplicitEliminationContext(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return false;

		HST_ActiveMissionState mission = FindMissionForConvoyGroup(state, activeGroup);
		if (!mission)
			return false;
		if (mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT || mission.m_sRuntimePhase == MISSION_CONVOY_ELIMINATED)
			return true;
		if (mission.m_sLastRuntimeEventKey == CONVOY_COMPLETE_EVENT_KEY || mission.m_sLastRuntimeEventKey == MISSION_CONVOY_ELIMINATED || mission.m_sLastRuntimeEventKey == "convoy_secured_sent")
			return true;

		HST_MissionAssetState asset = FindMissionConvoyAssetForGroup(state, mission, activeGroup);
		if (asset && IsMissionConvoyVehicleAssetResolved(asset))
			return true;

		return false;
	}

	protected bool HasMissionConvoyCrewEverBeenObservedAlive(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;
		if (activeGroup.m_bEverHadLivingCrew)
			return true;
		if (activeGroup.m_iMaxObservedCrewAlive > 0)
			return true;

		return false;
	}

	protected string ReportText(string value)
	{
		if (value.IsEmpty())
			return "none";

		return value;
	}

	protected string ReportBool(bool value)
	{
		if (value)
			return "yes";

		return "no";
	}

	protected string ReportRouteSite(HST_CampaignState state, string routeOrSiteId)
	{
		if (routeOrSiteId.IsEmpty())
			return "none";
		if (!state)
			return "missing:" + routeOrSiteId;

		if (state.FindGeneratedSite(routeOrSiteId))
			return routeOrSiteId;
		if (state.FindGeneratedRoute(routeOrSiteId))
			return routeOrSiteId;

		return "missing:" + routeOrSiteId;
	}

	protected bool HasPendingConvoyCrewPopulation(HST_CampaignState state, HST_ActiveMissionState mission, int vehicleAssets)
	{
		if (!state || !mission)
			return false;

		for (int index = 0; index < vehicleAssets; index++)
		{
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			if (IsConvoyCrewPopulationPending(state, activeGroup))
				return true;
		}

		return false;
	}

	protected bool HasPendingConvoyCrewControl(HST_CampaignState state, HST_ActiveMissionState mission, int vehicleAssets)
	{
		if (!state || !mission)
			return false;

		for (int index = 0; index < vehicleAssets; index++)
		{
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			if (IsConvoyCrewControlPending(state, activeGroup))
				return true;
		}

		return false;
	}

	protected bool HasMissionConvoyControlPending(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		int assetIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, assetIndex));
			assetIndex++;
			if (IsConvoyCrewControlPending(state, activeGroup))
				return true;
		}

		return false;
	}

	protected bool IsConvoyCrewPopulationPending(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup || activeGroup.m_iSpawnedAgentCount > 0)
			return false;
		if (state.m_iElapsedSeconds >= activeGroup.m_iSpawnedAtSecond + CONVOY_CREW_POPULATION_GRACE_SECONDS)
			return false;
		if (activeGroup.m_sRuntimeStatus == "spawn_pending_agents")
			return true;
		if (activeGroup.m_bSpawnAttempted && GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId))
			return true;

		return false;
	}

	protected bool IsConvoyCrewControlPending(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (IsConvoyCrewPopulationPending(state, activeGroup))
			return true;
		if (!state || !activeGroup)
			return false;
		if (activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == "eliminated")
			return false;
		if (activeGroup.m_sSpawnFallbackMode != "convoy_seating_pending")
			return false;
		if (activeGroup.m_iSpawnedAgentCount <= 0 && activeGroup.m_iLastSeenAliveCount <= 0 && activeGroup.m_iSurvivorInfantryCount <= 0)
			return false;

		return state.m_iElapsedSeconds < activeGroup.m_iSpawnedAtSecond + CONVOY_CREW_SEATING_GRACE_SECONDS;
	}

	protected bool IsRestoredMissionConvoyRuntimeRebindPending(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup || !state.m_bRestoredFromPersistence || !activeGroup.m_bSpawnedEntity)
			return false;
		if (state.m_iElapsedSeconds > state.m_iLastRestoreSecond + CONVOY_CREW_POPULATION_GRACE_SECONDS)
			return false;

		return activeGroup.m_iSpawnedAgentCount > 0 || activeGroup.m_iLastSeenAliveCount > 0 || activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iSurvivorVehicleCount > 0;
	}

	protected bool ShouldSpawnMissionConvoyRuntime(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;
		if (activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed")
			return false;
		if (!activeGroup.m_bSpawnedEntity)
			return true;
		if (state && IsMissionConvoyGroupAssetTerminal(state, activeGroup))
			return false;

		bool missingCrewRuntime = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) == null;
		bool missingVehicleRuntime = GetRuntimeVehicleEntity(activeGroup.m_sGroupId) == null;
		if (!missingCrewRuntime && !missingVehicleRuntime)
			return false;
		if (state && state.m_bRestoredFromPersistence && (missingCrewRuntime || missingVehicleRuntime))
			return !WasRestoredMissionConvoyRuntimeRebuildAttempted(activeGroup.m_sGroupId);
		if (activeGroup.m_iSpawnedAgentCount <= 0 && activeGroup.m_iLastSeenAliveCount <= 0 && activeGroup.m_iSurvivorInfantryCount <= 0)
			return true;

		return false;
	}

	protected bool WasRestoredMissionConvoyRuntimeRebuildAttempted(string groupId)
	{
		if (groupId.IsEmpty())
			return false;

		return m_aRestoredMissionConvoyRebuildGroupIds.Find(groupId) >= 0;
	}

	protected void MarkRestoredMissionConvoyRuntimeRebuildAttempted(string groupId)
	{
		if (groupId.IsEmpty())
			return;
		if (m_aRestoredMissionConvoyRebuildGroupIds.Find(groupId) >= 0)
			return;

		m_aRestoredMissionConvoyRebuildGroupIds.Insert(groupId);
	}

	protected bool RecordRestoredMissionConvoyRuntimeHandleLoss(HST_ActiveGroupState activeGroup, bool missingCrewRuntime, bool missingVehicleRuntime)
	{
		if (!activeGroup)
			return false;

		string missing = "";
		if (missingCrewRuntime)
			missing = "crew";
		if (missingVehicleRuntime)
		{
			if (!missing.IsEmpty())
				missing = missing + "+";
			missing = missing + "vehicle";
		}
		if (missing.IsEmpty())
			missing = "runtime";

		string reason = string.Format("Restore repair already rebuilt this convoy group once; suppressing repeat %1 respawn.", missing);
		if (activeGroup.m_sSpawnFailureReason == reason)
			return false;

		activeGroup.m_sSpawnFailureReason = reason;
		return true;
	}

	protected bool IsMissionConvoyGroupAssetTerminal(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		HST_ActiveMissionState mission = FindMissionForConvoyGroup(state, activeGroup);
		HST_MissionAssetState asset = FindMissionConvoyAssetForGroup(state, mission, activeGroup);
		if (!asset)
			return false;

		return IsMissionConvoyVehicleAssetResolved(asset);
	}

	protected string BuildMissionConvoyGroupId(HST_ActiveMissionState mission, int index)
	{
		if (!mission)
			return "";

		return string.Format("%1%2_%3", MISSION_CONVOY_GROUP_PREFIX, mission.m_sInstanceId, index);
	}

	protected bool IsMissionConvoyGroup(HST_ActiveGroupState activeGroup)
	{
		return activeGroup && activeGroup.m_sGroupId.Contains(MISSION_CONVOY_GROUP_PREFIX);
	}

	protected bool IsMissionOwnedActiveGroup(HST_ActiveGroupState activeGroup)
	{
		return activeGroup && !activeGroup.m_sMissionInstanceId.IsEmpty();
	}

	protected bool IsTerminalMissionConvoyPhase(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		return mission.m_sRuntimePhase == MISSION_CONVOY_FAILED || mission.m_sRuntimePhase == MISSION_CONVOY_ELIMINATED || mission.m_sRuntimePhase == MISSION_CONVOY_ARRIVED || mission.m_sRuntimePhase == "completed" || mission.m_sRuntimePhase == "expired";
	}

	protected bool IsPersistenceSmokeMission(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		return mission.m_sInstanceId.Contains(PERSISTENCE_SMOKE_PREFIX) || mission.m_sMissionId.Contains(PERSISTENCE_SMOKE_PREFIX);
	}

	protected string ResolveMissionConvoyRuntimeStatus(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == "created" || mission.m_sRuntimePhase == "active" || mission.m_sRuntimePhase == "convoy_static")
			return MISSION_CONVOY_STAGING;

		return mission.m_sRuntimePhase;
	}

	protected HST_ZoneState ResolveMissionConvoyTargetZone(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!state)
			return null;

		if (mission && !mission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
			if (zone)
				return zone;
		}

		if (asset)
			return FindNearestZone(state, asset.m_vTargetPosition);

		return null;
	}

	protected HST_ZoneState FindNearestZone(HST_CampaignState state, vector position)
	{
		if (!state)
			return null;

		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			float distanceSq = DistanceSq2D(zone.m_vPosition, position);
			if (distanceSq < bestDistanceSq)
			{
				bestDistanceSq = distanceSq;
				bestZone = zone;
			}
		}

		return bestZone;
	}

	protected string ResolveMissionConvoyFactionKey(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_ZoneState targetZone)
	{
		if (targetZone && !targetZone.m_sOwnerFactionKey.IsEmpty())
		{
			if (!preset || targetZone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				return targetZone.m_sOwnerFactionKey;
		}

		if (preset && !preset.m_sOccupierFactionKey.IsEmpty())
			return preset.m_sOccupierFactionKey;

		return "US";
	}

	protected string SelectConvoyCrewGroupPrefab(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState zone, string factionKey, int index)
	{
		HST_FactionRuntimeSpawnSpec spec = HST_DefaultCatalog.ResolveRuntimeSpawnSpec(preset, factionKey, "convoy_crew", "mission convoy crew selection");
		if (!spec)
			return "";

		array<string> candidates = {};
		AppendUniqueGroupPrefabs(candidates, spec.m_aGroupPrefabs);
		int seed = BuildGroupSelectionSeed(state, zone, false) + index * 71 + 9001;
		string compactPrefab = SelectPreferredCompactGroupPrefab(candidates, seed, factionKey, "mission convoy crew");
		if (!compactPrefab.IsEmpty())
			return compactPrefab;

		return SelectValidGroupPrefabFromList(candidates, seed, factionKey, "mission convoy crew");
	}

	protected string SelectPreferredCompactGroupPrefab(array<string> prefabs, int seed, string factionKey, string purpose)
	{
		if (!prefabs || prefabs.Count() == 0)
			return "";

		int startIndex = HST_DefaultCatalog.PositiveMod(seed, prefabs.Count());
		for (int offset = 0; offset < prefabs.Count(); offset++)
		{
			int index = HST_DefaultCatalog.PositiveMod(startIndex + offset, prefabs.Count());
			string prefab = prefabs[index];
			if (!IsCompactCrewGroupPrefab(prefab))
				continue;
			if (!IsValidGroupPrefabResource(prefab, factionKey))
				continue;

			return prefab;
		}

		return "";
	}

	protected bool IsCompactCrewGroupPrefab(string prefab)
	{
		return prefab.Contains("SentryTeam");
	}

	protected int ResolveMissionConvoyCrewCount(HST_CampaignState state, HST_ActiveMissionState mission, int index)
	{
		return 2;
	}

	protected vector OffsetConvoyCrewSpawnPosition(vector vehiclePosition, vector targetPosition, int index)
	{
		vector result = vehiclePosition;
		float dx = targetPosition[0] - vehiclePosition[0];
		float dz = targetPosition[2] - vehiclePosition[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		float sideX = 0.0;
		float sideZ = 1.0;
		if (length > 1.0)
		{
			sideX = -dz / length;
			sideZ = dx / length;
		}

		float sideOffset = 5.5;
		if ((index - (index / 2) * 2) == 1)
			sideOffset = -5.5;

		result[0] = result[0] + sideX * sideOffset;
		result[2] = result[2] + sideZ * sideOffset;
		return HST_WorldPositionService.ResolveSafeGroundPosition(result, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 3.0);
	}

	protected vector BuildConvoyVehicleAngles(vector sourcePosition, vector targetPosition)
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

	protected vector BuildConvoyVehicleAnglesFromForward(vector sourcePosition, vector forward, vector fallbackTargetPosition)
	{
		float length = Math.Sqrt(forward[0] * forward[0] + forward[2] * forward[2]);
		if (length <= 0.01)
			return BuildConvoyVehicleAngles(sourcePosition, fallbackTargetPosition);

		vector travelTarget = sourcePosition;
		travelTarget[0] = travelTarget[0] + forward[0] * 10.0;
		travelTarget[2] = travelTarget[2] + forward[2] * 10.0;
		return BuildConvoyVehicleAngles(sourcePosition, travelTarget);
	}

	protected bool AllMissionConvoyGroupsAttempted(HST_CampaignState state, HST_ActiveMissionState mission, int vehicleAssets)
	{
		if (!state || !mission || vehicleAssets <= 0)
			return false;

		for (int i = 0; i < vehicleAssets; i++)
		{
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, i));
			if (!activeGroup || (!activeGroup.m_bSpawnAttempted && !activeGroup.m_bSpawnedEntity))
				return false;
		}

		return true;
	}

	protected bool ApplyMissionConvoyStatusToGroups(HST_CampaignState state, HST_ActiveMissionState mission, string status)
	{
		if (!state || !mission || status.IsEmpty())
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroupForMission(activeGroup, mission))
				continue;
			if (activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_sRuntimeStatus == "spawn_failed")
				continue;
			if (activeGroup.m_sRuntimeStatus == status)
				continue;

			activeGroup.m_sRuntimeStatus = status;
			if (status == MISSION_CONVOY_CONTACT)
				activeGroup.m_sConvoyRuntimeStage = "CONTACT";
			else if (status == MISSION_CONVOY_ARRIVED)
				activeGroup.m_sConvoyRuntimeStage = "ARRIVED";
			else if (status == MISSION_CONVOY_ELIMINATED)
				activeGroup.m_sConvoyRuntimeStage = "INTERCEPTED";
			changed = true;
		}

		return changed;
	}

	protected ref array<vector> BuildMissionConvoyWaypointPositions(HST_GeneratedRouteState route)
	{
		ref array<vector> waypoints = {};
		if (!route)
			return waypoints;

		if (!route.m_aWaypoints || route.m_aWaypoints.Count() == 0)
		{
			if (!IsZeroVector(route.m_vEndPosition))
				AppendConvoyRoadWaypoint(waypoints, route.m_vEndPosition, route.m_vEndPosition);

			return waypoints;
		}

		ref array<vector> preferredWaypoints = {};
		int lastIndex = -1000000;
		while (true)
		{
			HST_RouteWaypointState selectedWaypoint;
			int selectedIndex = 1000000;
			foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
			{
				if (!waypoint)
					continue;
				if (waypoint.m_iIndex <= lastIndex)
					continue;
				if (waypoint.m_iIndex >= selectedIndex)
					continue;

				selectedWaypoint = waypoint;
				selectedIndex = waypoint.m_iIndex;
			}

			if (!selectedWaypoint)
				break;

			preferredWaypoints.Insert(selectedWaypoint.m_vPosition);

			lastIndex = selectedWaypoint.m_iIndex;
		}

		AppendSparseConvoyRouteWaypoints(waypoints, preferredWaypoints, route.m_vEndPosition, CONVOY_RUNTIME_WAYPOINT_MAX_COUNT);
		return waypoints;
	}

	protected void AppendSparseConvoyRouteWaypoints(array<vector> waypoints, array<vector> preferredWaypoints, vector destination, int maxWaypointCount)
	{
		if (!waypoints || !preferredWaypoints || preferredWaypoints.Count() == 0 || maxWaypointCount <= 0)
			return;

		int preferredCount = preferredWaypoints.Count();
		if (preferredCount <= maxWaypointCount)
		{
			foreach (vector preferredPosition : preferredWaypoints)
			{
				AppendConvoyRoadWaypoint(waypoints, preferredPosition, destination);
			}
			return;
		}

		int lastSelectedIndex = -1;
		for (int i = 0; i < maxWaypointCount; i++)
		{
			float t = 0.0;
			if (maxWaypointCount > 1)
				t = i * 1.0 / (maxWaypointCount - 1);

			int selectedIndex = Math.Round(t * (preferredCount - 1));
			if (selectedIndex <= lastSelectedIndex)
				selectedIndex = lastSelectedIndex + 1;
			if (selectedIndex >= preferredCount)
				selectedIndex = preferredCount - 1;

			AppendConvoyRoadWaypoint(waypoints, preferredWaypoints[selectedIndex], destination);
			lastSelectedIndex = selectedIndex;
		}
	}

	protected void AppendConvoyRoadSegmentWaypoints(array<vector> waypoints, vector startPosition, vector endPosition, vector destination)
	{
		if (!waypoints || IsZeroVector(startPosition) || IsZeroVector(endPosition))
			return;

		float distanceMeters = Math.Sqrt(DistanceSq2D(startPosition, endPosition));
		int segmentCount = Math.Max(1, Math.Round(distanceMeters / 90.0));
		segmentCount = Math.Min(segmentCount, CONVOY_RUNTIME_WAYPOINT_MAX_COUNT);
		for (int i = 1; i <= segmentCount; i++)
		{
			float t = i;
			t = t / segmentCount;
			vector candidate = startPosition;
			candidate[0] = startPosition[0] + (endPosition[0] - startPosition[0]) * t;
			candidate[1] = startPosition[1] + (endPosition[1] - startPosition[1]) * t;
			candidate[2] = startPosition[2] + (endPosition[2] - startPosition[2]) * t;
			AppendConvoyRoadWaypoint(waypoints, candidate, destination);
		}
	}

	protected void AppendConvoyRoadWaypoint(array<vector> waypoints, vector preferred, vector destination)
	{
		if (!waypoints || IsZeroVector(preferred))
			return;

		vector resolved;
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		if (!HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(preferred, CONVOY_ROUTE_WAYPOINT_ROAD_SEARCH_RADIUS_METERS, destination, resolved, roadForward, roadWidth, roadDistance, roadReason))
			return;

		if (waypoints.Count() > 0 && DistanceSq2D(waypoints[waypoints.Count() - 1], resolved) < 9.0)
			return;

		waypoints.Insert(resolved);
	}

	protected bool AssignMissionConvoyWaypoints(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		ref array<vector> waypoints = BuildMissionConvoyWaypointPositions(ResolveMissionConvoyRoute(state, mission));
		int eligibleGroups;
		int assignedGroups;
		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsMissionConvoyGroupForMission(activeGroup, mission) || !activeGroup.m_bSpawnedEntity)
				continue;
			if (activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED)
				continue;
			eligibleGroups++;
			if (IsMissionConvoyWaypointAssigned(activeGroup))
			{
				assignedGroups++;
				continue;
			}

			string previousFallbackMode = activeGroup.m_sSpawnFallbackMode;
			int previousWaypointCount = activeGroup.m_iAssignedWaypointCount;
			string previousReason = activeGroup.m_sSpawnFailureReason;
			if (TryAssignConvoyWaypoints(activeGroup, waypoints))
			{
				activeGroup.m_sSpawnFallbackMode = "convoy_waypoints";
				activeGroup.m_sConvoyRuntimeStage = "ROUTE_ASSIGNED";
				assignedGroups++;
			}
			else
			{
				if (activeGroup.m_sSpawnFallbackMode.IsEmpty() || activeGroup.m_sSpawnFallbackMode == "convoy_crew_near_vehicle" || activeGroup.m_sSpawnFallbackMode == "convoy_seating_pending")
					activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_control_unavailable";
				if (activeGroup.m_sSpawnFailureReason.IsEmpty())
					activeGroup.m_sSpawnFailureReason = "Convoy has no seated living AI driver yet.";
				if (mission.m_sRuntimeFailureReason.IsEmpty())
					mission.m_sRuntimeFailureReason = activeGroup.m_sSpawnFailureReason;
				if (activeGroup.m_sSpawnFailureReason != previousReason)
					Print(string.Format("h-istasi mission convoy | waypoint assignment unavailable for %1: %2", activeGroup.m_sGroupId, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			}
			if (activeGroup.m_sSpawnFallbackMode != previousFallbackMode || activeGroup.m_iAssignedWaypointCount != previousWaypointCount || activeGroup.m_sSpawnFailureReason != previousReason)
				changed = true;
		}

		return changed || (eligibleGroups > 0 && assignedGroups == eligibleGroups);
	}

	protected bool TryAssignConvoyWaypoints(HST_ActiveGroupState activeGroup, array<vector> waypoints)
	{
		if (!activeGroup)
			return false;

		ref array<vector> groupWaypoints = BuildMissionConvoyGroupWaypointPositions(activeGroup, waypoints);
		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicle = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		int assignedWaypointCount;
		string adapterReason;
		if (GetConvoyVehicleControlAdapter().TryAssignVehicleRoute(activeGroup, crewEntity, vehicle, groupWaypoints, assignedWaypointCount, adapterReason) && assignedWaypointCount > 1)
		{
			activeGroup.m_iAssignedWaypointCount = assignedWaypointCount;
			activeGroup.m_sSpawnFailureReason = adapterReason;
			return true;
		}

		activeGroup.m_iAssignedWaypointCount = 0;
		if (!vehicle)
			activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_missing";
		else
			activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_control_unavailable";
		activeGroup.m_sSpawnFailureReason = adapterReason;
		if (activeGroup.m_sSpawnFailureReason.IsEmpty())
			activeGroup.m_sSpawnFailureReason = "Convoy has no seated living AI driver yet.";
		return false;
	}

	protected ref array<vector> BuildMissionConvoyGroupWaypointPositions(HST_ActiveGroupState activeGroup, array<vector> routeWaypoints)
	{
		ref array<vector> result = {};
		if (!activeGroup)
			return result;

		if (routeWaypoints && routeWaypoints.Count() > 0)
		{
			int desiredWaypointCount = Math.Max(CONVOY_RUNTIME_WAYPOINT_MIN_COUNT, Math.Min(CONVOY_RUNTIME_WAYPOINT_MAX_COUNT, routeWaypoints.Count() + 2));
			int leadInWaypointCount = Math.Max(0, desiredWaypointCount - routeWaypoints.Count());
			AppendConvoyLeadInWaypoints(result, activeGroup.m_vSourcePosition, routeWaypoints[0], leadInWaypointCount);
			foreach (vector routeWaypoint : routeWaypoints)
			{
				if (result.Count() >= CONVOY_RUNTIME_WAYPOINT_MAX_COUNT)
					break;

				AppendResolvedConvoyWaypoint(result, routeWaypoint);
			}
		}

		if (result.Count() == 0 && !IsZeroVector(activeGroup.m_vTargetPosition))
			AppendConvoyRoadSegmentWaypoints(result, activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition, activeGroup.m_vTargetPosition);
		if (result.Count() == 1 && !IsZeroVector(activeGroup.m_vTargetPosition))
			AppendConvoyRoadWaypoint(result, activeGroup.m_vTargetPosition, activeGroup.m_vTargetPosition);

		return result;
	}

	protected void AppendConvoyLeadInWaypoints(array<vector> waypoints, vector sourcePosition, vector routeStartPosition, int maxWaypointCount)
	{
		if (!waypoints || maxWaypointCount <= 0 || IsZeroVector(sourcePosition) || IsZeroVector(routeStartPosition))
			return;

		for (int i = 1; i <= maxWaypointCount; i++)
		{
			float t = i;
			t = t / (maxWaypointCount + 1);
			vector candidate = sourcePosition;
			candidate[0] = sourcePosition[0] + (routeStartPosition[0] - sourcePosition[0]) * t;
			candidate[1] = sourcePosition[1] + (routeStartPosition[1] - sourcePosition[1]) * t;
			candidate[2] = sourcePosition[2] + (routeStartPosition[2] - sourcePosition[2]) * t;
			AppendConvoyRoadWaypoint(waypoints, candidate, routeStartPosition);
		}
	}

	protected void AppendResolvedConvoyWaypoint(array<vector> waypoints, vector resolved)
	{
		if (!waypoints || IsZeroVector(resolved))
			return;
		if (waypoints.Count() > 0 && DistanceSq2D(waypoints[waypoints.Count() - 1], resolved) < 9.0)
			return;

		waypoints.Insert(resolved);
	}

	protected void SyncMissionConvoyAssetPositions(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		int index;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			vector position = ResolveMissionConvoyVehiclePosition(asset, BuildMissionConvoyGroupId(mission, index));
			UpdateMissionConvoyAssetPosition(state, asset, position);
			index++;
		}
	}

	protected bool SyncMissionConvoyVehicleAssetRuntimeState(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, string groupId)
	{
		if (!state || !mission || !asset || !activeGroup || groupId.IsEmpty())
			return false;
		if (IsMissionConvoyVehicleAssetResolved(asset))
			return false;
		if (!activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED)
			return false;

		IEntity vehicleEntity = GetRuntimeVehicleEntity(groupId);
		if (!vehicleEntity)
			return false;

		bool changed;
		vector position = vehicleEntity.GetOrigin();
		if (asset.m_sEntityId.IsEmpty())
		{
			asset.m_sEntityId = asset.m_sAssetId;
			changed = true;
		}
		if (!asset.m_bSpawned)
		{
			asset.m_bSpawned = true;
			changed = true;
		}
		if (!asset.m_bAlive)
		{
			asset.m_bAlive = true;
			changed = true;
		}
		if (!IsZeroVector(position))
		{
			if (DistanceSq2D(asset.m_vCurrentPosition, position) > 1.0)
			{
				asset.m_vCurrentPosition = position;
				changed = true;
			}
			if (DistanceSq2D(asset.m_vLastKnownPosition, position) > 1.0)
			{
				asset.m_vLastKnownPosition = position;
				changed = true;
			}
		}

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtimeEntity)
		{
			runtimeEntity = new HST_MissionRuntimeEntityState();
			runtimeEntity.m_sRuntimeEntityId = asset.m_sEntityId;
			runtimeEntity.m_sMissionInstanceId = mission.m_sInstanceId;
			state.m_aMissionRuntimeEntities.Insert(runtimeEntity);
			changed = true;
		}
		if (runtimeEntity.m_sMissionInstanceId != mission.m_sInstanceId)
		{
			runtimeEntity.m_sMissionInstanceId = mission.m_sInstanceId;
			changed = true;
		}
		if (runtimeEntity.m_sKind != asset.m_sRole)
		{
			runtimeEntity.m_sKind = asset.m_sRole;
			changed = true;
		}
		if (runtimeEntity.m_sPrefab != asset.m_sPrefab)
		{
			runtimeEntity.m_sPrefab = asset.m_sPrefab;
			changed = true;
		}
		if (!runtimeEntity.m_bSpawned)
		{
			runtimeEntity.m_bSpawned = true;
			changed = true;
		}
		if (runtimeEntity.m_bDestroyed)
		{
			runtimeEntity.m_bDestroyed = false;
			changed = true;
		}
		if (!IsZeroVector(position) && DistanceSq2D(runtimeEntity.m_vPosition, position) > 1.0)
		{
			runtimeEntity.m_vPosition = position;
			changed = true;
		}

		return changed;
	}

	protected vector ResolveMissionConvoyVehiclePosition(HST_MissionAssetState asset, string groupId)
	{
		IEntity vehicle = GetRuntimeVehicleEntity(groupId);
		if (vehicle)
			return vehicle.GetOrigin();

		if (asset)
			return asset.m_vCurrentPosition;

		return "0 0 0";
	}

	protected void UpdateMissionConvoyAssetPosition(HST_CampaignState state, HST_MissionAssetState asset, vector position)
	{
		if (!state || !asset || IsZeroVector(position))
			return;

		asset.m_vCurrentPosition = position;
		asset.m_vLastKnownPosition = position;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_vPosition = position;
		if (asset.m_sRole == MISSION_CONVOY_VEHICLE_ROLE)
			SyncMissionConvoyPayloadPositions(state, asset.m_sMissionInstanceId, position);
	}

	protected void SyncMissionConvoyPayloadPositions(HST_CampaignState state, string missionInstanceId, vector position)
	{
		if (!state || missionInstanceId.IsEmpty() || IsZeroVector(position))
			return;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != missionInstanceId)
				continue;
			if (asset.m_sRole != MISSION_CONVOY_PAYLOAD_ROLE && asset.m_sRole != MISSION_CONVOY_CAPTIVE_ROLE)
				continue;
			if (asset.m_bPickedUp || asset.m_bDelivered || asset.m_bDestroyed)
				continue;

			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
		}
	}

	protected bool ApplyMissionConvoyObjectiveProgress(HST_CampaignState state, HST_ActiveMissionState mission, int eliminatedGroups, int totalGroups)
	{
		if (!state || !mission || totalGroups <= 0)
			return false;

		bool changed;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_sTargetId != "convoy")
				continue;

			int required = Math.Max(1, totalGroups);
			int current = Math.Min(required, Math.Max(0, eliminatedGroups));
			if (objective.m_iRequiredCount != required || objective.m_iRequiredProgress != required)
			{
				objective.m_iRequiredCount = required;
				objective.m_iRequiredProgress = required;
				changed = true;
			}

			if (objective.m_iCurrentCount != current || objective.m_iCurrentProgress != current)
			{
				objective.m_iCurrentCount = current;
				objective.m_iCurrentProgress = current;
				changed = true;
			}

			if (current >= required && !objective.m_bComplete)
			{
				objective.m_bComplete = true;
				changed = true;
			}
		}

		return changed;
	}

	protected bool TryResolveMissionConvoyDestinationArrival(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !CanResolveMissionConvoyDestinationArrival(mission))
			return false;

		int assetIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, assetIndex);
			assetIndex++;
			if (IsMissionConvoyVehicleAssetResolved(asset))
				continue;

			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			if (!activeGroup || CountAliveRuntimeCrewAgents(activeGroup) <= 0)
				continue;

			vector position = ResolveMissionConvoyVehiclePosition(asset, groupId);
			if (!IsMissionConvoyAtDestination(mission, position))
				continue;

			UpdateMissionConvoyAssetPosition(state, asset, position);
			if (!IsZeroVector(position))
				activeGroup.m_vPosition = position;
			SetMissionConvoyFailure(state, mission, "Convoy reached its destination with living crew.");
			return true;
		}

		return false;
	}

	protected bool CanResolveMissionConvoyDestinationArrival(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;
		if (mission.m_sRuntimePhase == MISSION_CONVOY_MOVING || mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT)
			return true;

		return false;
	}

	protected bool IsMissionConvoyAtDestination(HST_ActiveMissionState mission, vector position)
	{
		if (!mission || IsZeroVector(position) || IsZeroVector(mission.m_vTargetPosition))
			return false;

		return DistanceSq2D(position, mission.m_vTargetPosition) <= CONVOY_DESTINATION_RADIUS_METERS * CONVOY_DESTINATION_RADIUS_METERS;
	}

	protected void SetMissionConvoyFailure(HST_CampaignState state, HST_ActiveMissionState mission, string reason)
	{
		if (!state || !mission || mission.m_sRuntimePhase == MISSION_CONVOY_FAILED)
			return;

		mission.m_sRuntimePhase = MISSION_CONVOY_FAILED;
		mission.m_sRuntimeFailureReason = reason;
		mission.m_sLastRuntimeEventKey = CONVOY_FAIL_EVENT_KEY;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete)
				continue;

			objective.m_bFailed = true;
		}

		m_bMarkerRefreshNeeded = true;
		Print(string.Format("h-istasi mission convoy | %1 failed: %2%3", mission.m_sInstanceId, reason, BuildMissionConvoyFailureContext(state, mission)), LogLevel.WARNING);
	}

	protected string BuildMissionConvoyFailureContext(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "";

		int vehicleAssets;
		int attemptedGroups;
		int spawnedGroups;
		int pendingControlGroups;
		int aliveCrewGroups;
		int aliveCrew;
		int crewRuntimeEntities;
		int vehicleRuntimeEntities;
		string sample = "";
		int assetIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			string groupId = BuildMissionConvoyGroupId(mission, assetIndex);
			assetIndex++;
			vehicleAssets++;
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			if (!activeGroup)
			{
				if (sample.IsEmpty())
					sample = string.Format(" | sample group %1 missing", ReportText(groupId));
				continue;
			}

			if (activeGroup.m_bSpawnAttempted)
				attemptedGroups++;
			if (activeGroup.m_bSpawnedEntity)
				spawnedGroups++;
			if (IsConvoyCrewControlPending(state, activeGroup))
				pendingControlGroups++;
			if (GetRuntimeCrewGroupEntity(groupId))
				crewRuntimeEntities++;
			if (GetRuntimeVehicleEntity(groupId))
				vehicleRuntimeEntities++;

			int groupAliveCrew = CountAliveRuntimeCrewAgents(activeGroup);
			if (groupAliveCrew > 0)
			{
				aliveCrewGroups++;
				aliveCrew += groupAliveCrew;
			}

			if (sample.IsEmpty())
				sample = string.Format(" | sample group %1 status %2 spawned %3 agents %4 alive %5 maxObserved %6 everAlive %7 stage %8 mode %9", ReportText(groupId), ReportText(activeGroup.m_sRuntimeStatus), ReportBool(activeGroup.m_bSpawnedEntity), activeGroup.m_iSpawnedAgentCount, groupAliveCrew, activeGroup.m_iMaxObservedCrewAlive, ReportBool(activeGroup.m_bEverHadLivingCrew), ReportText(activeGroup.m_sConvoyRuntimeStage), ReportText(activeGroup.m_sSpawnFallbackMode)) + string.Format(" reason %1 crewFailure %2", ReportText(activeGroup.m_sSpawnFailureReason), ReportText(activeGroup.m_sCrewPopulationFailureReason));
		}

		return string.Format(" | context assets %1 | attempted groups %2 | spawned groups %3 | pending control %4 | alive crew groups %5 | alive crew %6 | crew runtime entities %7 | vehicle runtime entities %8%9", vehicleAssets, attemptedGroups, spawnedGroups, pendingControlGroups, aliveCrewGroups, aliveCrew, crewRuntimeEntities, vehicleRuntimeEntities, sample);
	}

	protected IEntity GetRuntimeCrewGroupEntity(string groupId)
	{
		IEntity fallbackEntity;
		for (int i = m_aRuntimeGroupIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity)
				continue;
			if (AIGroup.Cast(entity))
				return entity;
			if (!fallbackEntity)
				fallbackEntity = entity;
		}

		return fallbackEntity;
	}

	protected IEntity GetRuntimeVehicleEntity(string groupId)
	{
		for (int i = m_aRuntimeVehicleGroupIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeVehicleGroupIds[i] != groupId || i >= m_aRuntimeVehicleEntities.Count())
				continue;

			IEntity entity = m_aRuntimeVehicleEntities[i];
			if (entity)
				return entity;
		}

		return null;
	}

	protected int CountAliveRuntimeCrewAgents(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return 0;

		int aliveCount = CountAliveRuntimeCrewAgents(activeGroup.m_sGroupId);
		if (activeGroup.m_iInfantryCount <= 0)
			return aliveCount;

		return Math.Min(activeGroup.m_iInfantryCount, aliveCount);
	}

	protected int ResolveMissionConvoyRestorableCrewCount(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return 0;
		if (activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed")
			return 0;
		if (activeGroup.m_bCrewPopulationTerminallyFailed)
			return 0;
		if (state && IsMissionConvoyGroupAssetTerminal(state, activeGroup))
			return 0;

		int liveCrew = CountAliveRuntimeCrewAgents(activeGroup);
		if (liveCrew > 0)
			return liveCrew;

		int preservedCrew = Math.Max(activeGroup.m_iSpawnedAgentCount, Math.Max(activeGroup.m_iSurvivorInfantryCount, activeGroup.m_iLastSeenAliveCount));
		if (preservedCrew <= 0)
			return 0;
		if (activeGroup.m_iInfantryCount <= 0)
			return preservedCrew;

		return Math.Min(activeGroup.m_iInfantryCount, preservedCrew);
	}

	protected void RefreshMissionConvoyCrewCount(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return;

		int livingCrew = CountAliveRuntimeCrewAgents(activeGroup.m_sGroupId);
		if (livingCrew <= 0)
			return;

		RecordConvoyCrewObservedAlive(activeGroup, livingCrew);
		activeGroup.m_iInfantryCount = livingCrew;
		activeGroup.m_iSurvivorInfantryCount = livingCrew;
		activeGroup.m_iLastSeenAliveCount = livingCrew;
		activeGroup.m_iSpawnedAgentCount = Math.Max(activeGroup.m_iSpawnedAgentCount, livingCrew);
	}

	protected void RecordConvoyCrewObservedAlive(HST_ActiveGroupState activeGroup, int aliveCrew)
	{
		if (!activeGroup || aliveCrew <= 0)
			return;

		activeGroup.m_bEverHadLivingCrew = true;
		activeGroup.m_iMaxObservedCrewAlive = Math.Max(activeGroup.m_iMaxObservedCrewAlive, aliveCrew);
		activeGroup.m_bCrewPopulationTerminallyFailed = false;
		activeGroup.m_sCrewPopulationFailureReason = "";
		if (activeGroup.m_sConvoyRuntimeStage.IsEmpty() || activeGroup.m_sConvoyRuntimeStage == "PLANNED" || activeGroup.m_sConvoyRuntimeStage == "VEHICLE_SPAWNED" || activeGroup.m_sConvoyRuntimeStage == "CREW_GROUP_CREATED")
			activeGroup.m_sConvoyRuntimeStage = "CREW_POPULATED";
	}

	protected int CountAliveRuntimeCrewAgents(string groupId)
	{
		array<IEntity> livingCrew = {};
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity)
				continue;

			AIGroup group = AIGroup.Cast(entity);
			if (group)
			{
				CollectLivingNativeAIGroupEntities(group, livingCrew);
				continue;
			}

			if (IsLivingEntity(entity) && livingCrew.Find(entity) < 0)
				livingCrew.Insert(entity);
		}

		return livingCrew.Count();
	}

	protected HST_ActiveMissionState FindMissionForConvoyGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return null;

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
				continue;
			if (IsMissionConvoyGroupForMission(activeGroup, mission))
				return mission;
		}

		return null;
	}

	protected HST_MissionAssetState FindMissionConvoyAssetForGroup(HST_CampaignState state, HST_ActiveMissionState mission, HST_ActiveGroupState activeGroup)
	{
		if (!state || !mission || !activeGroup)
			return null;

		int targetIndex = ResolveMissionConvoyGroupIndex(mission, activeGroup.m_sGroupId);
		int index;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;
			if (index == targetIndex)
				return asset;

			index++;
		}

		return null;
	}

	protected int ResolveMissionConvoyGroupIndex(HST_ActiveMissionState mission, string groupId)
	{
		if (!mission || groupId.IsEmpty())
			return 0;

		string prefix = string.Format("%1%2_", MISSION_CONVOY_GROUP_PREFIX, mission.m_sInstanceId);
		if (!groupId.Contains(prefix))
			return 0;

		string suffix = groupId.Substring(prefix.Length(), groupId.Length() - prefix.Length());
		return suffix.ToInt();
	}


	protected bool IsAnyLivingPlayerNearZone(PlayerManager playerManager, array<int> playerIds, HST_ZoneState zone, HST_BalanceConfig balance)
	{
		if (!zone)
			return false;

		float radius = zone.m_iActivationRadiusMeters;
		if (radius <= 0)
			radius = balance.m_iActivationRadiusMeters;

		float radiusSq = radius * radius;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			if (DistanceSq2D(playerEntity.GetOrigin(), zone.m_vPosition) <= radiusSq)
				return true;
		}

		return false;
	}

	protected bool ActivateZone(HST_CampaignState state, HST_ZoneState zone, HST_CampaignPreset preset = null, HST_ZoneCompositionService compositions = null, bool fullGarrison = false)
	{
		bool changed;
		array<ref HST_ZoneSpawnSlotState> slots = {};
		if (compositions)
		{
			changed = compositions.EnsureZoneComposition(state, zone) || changed;
			slots = compositions.BuildZoneSpawnSlots(state, zone);
		}

		bool hasActiveGarrisonGroup = HasActiveGarrisonGroup(state, zone);
		if (hasActiveGarrisonGroup && !fullGarrison)
		{
			ApplyActiveZoneCounts(state, zone);
			return changed;
		}

		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, zone.m_sOwnerFactionKey);
		if (!garrison)
		{
			ApplyActiveZoneCounts(state, zone);
			return changed;
		}

		int garrisonInfantryBefore = garrison.m_iInfantryCount;
		int garrisonVehiclesBefore = garrison.m_iVehicleCount;
		int infantryCount;
		int vehicleCount;
		if (fullGarrison)
		{
			infantryCount = Math.Max(0, garrison.m_iInfantryCount);
			vehicleCount = Math.Max(0, garrison.m_iVehicleCount);
		}
		else
		{
			infantryCount = Math.Min(garrison.m_iInfantryCount, ResolveActiveInfantryCap(zone));
			vehicleCount = Math.Min(garrison.m_iVehicleCount, ResolveActiveVehicleCap(zone));
		}
		if (vehicleCount > 0 && IsActiveVehicleSpawnBlocked(zone.m_sZoneId))
			vehicleCount = 0;

		if (infantryCount <= 0 && vehicleCount <= 0)
		{
			ApplyActiveZoneCounts(state, zone);
			return changed;
		}

		garrison.m_iInfantryCount = Math.Max(0, garrison.m_iInfantryCount - infantryCount);
		garrison.m_iVehicleCount = Math.Max(0, garrison.m_iVehicleCount - vehicleCount);

		int spawnedInfantryGroups = SpawnZoneInfantryGroups(state, zone, preset, slots, infantryCount, compositions);
		int spawnedVehicleGroups = SpawnZoneVehicleGroups(state, zone, preset, slots, vehicleCount);
		ApplyActiveZoneCounts(state, zone);
		if (zone.m_iActiveInfantryCount < infantryCount && infantryCount > 0)
		{
			int pendingInfantry = CountPendingActiveZonePopulationInfantry(state, zone);
			int pendingGroups = CountPendingActiveZonePopulationGroups(state, zone);
			if (pendingInfantry > 0)
			{
				Print(string.Format(
					"h-istasi garrison | activation pending native population | zone %1 | requested infantry %2 | active infantry %3 | pending infantry %4 | pending groups %5",
					zone.m_sZoneId,
					infantryCount,
					zone.m_iActiveInfantryCount,
					pendingInfantry,
					pendingGroups
				));
			}
			else
			{
				Print(string.Format(
					"h-istasi garrison | activation partial | zone %1 | requested infantry %2 | active infantry %3 | pending infantry 0 | folded failures may have returned to abstract garrison",
					zone.m_sZoneId,
					infantryCount,
					zone.m_iActiveInfantryCount
				), LogLevel.WARNING);
			}
		}
		if (zone.m_iActiveVehicleCount < vehicleCount && vehicleCount > 0)
		{
			Print(string.Format(
				"h-istasi garrison | activation partial | zone %1 | requested vehicles %2 | active vehicles %3 | folded failures may have returned to abstract garrison",
				zone.m_sZoneId,
				vehicleCount,
				zone.m_iActiveVehicleCount
			), LogLevel.WARNING);
		}
		string activationReport = string.Format("h-istasi | activated zone %1 | requested infantry %2/%3 vehicles %4/%5 | spawned infantry groups %6 vehicle groups %7", zone.m_sZoneId, infantryCount, garrisonInfantryBefore, vehicleCount, garrisonVehiclesBefore, spawnedInfantryGroups, spawnedVehicleGroups);
		activationReport = activationReport + string.Format(" | active now infantry %1 vehicles %2 | abstract garrison now infantry %3 vehicles %4", zone.m_iActiveInfantryCount, zone.m_iActiveVehicleCount, garrison.m_iInfantryCount, garrison.m_iVehicleCount);
		Print(string.Format("%1", activationReport));
		return true;
	}

	bool CleanupCapturedZoneHostileRuntime(HST_CampaignState state, string zoneId, string resistanceFactionKey)
	{
		if (!state || zoneId.IsEmpty() || resistanceFactionKey.IsEmpty())
			return false;

		bool changed;
		int removedGroups;
		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			if (!activeGroup || activeGroup.m_sZoneId != zoneId)
				continue;
			if (activeGroup.m_bQRF || IsMissionConvoyGroup(activeGroup))
				continue;
			if (activeGroup.m_sFactionKey == resistanceFactionKey)
				continue;

			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			state.m_aActiveGroups.Remove(i);
			removedGroups++;
			changed = true;
		}

		if (changed)
		{
			HST_ZoneState zone = state.FindZone(zoneId);
			if (zone)
				ApplyActiveZoneCounts(state, zone);

			ClearActiveVehicleSpawnBlocked(zoneId);
			m_bMarkerRefreshNeeded = true;
			Print(string.Format("h-istasi capture | cleaned %1 hostile active group(s) after ownership flip at %2", removedGroups, zoneId));
		}

		return changed;
	}

	bool FoldActiveSupportGroup(HST_CampaignState state, string groupId)
	{
		if (!state || groupId.IsEmpty())
			return false;

		HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
		if (!activeGroup)
			return false;

		if (activeGroup.m_sRuntimeStatus == "folded")
			return true;

		FoldActiveGroup(state, activeGroup);
		DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected bool DeactivateZone(HST_CampaignState state, HST_ZoneState zone, HST_ZoneCompositionService compositions = null)
	{
		bool changed;
		int foldedGroups;
		int returnedInfantry;
		int returnedVehicles;
		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			if (!activeGroup || activeGroup.m_sZoneId != zone.m_sZoneId || activeGroup.m_bQRF || IsMissionOwnedActiveGroup(activeGroup))
				continue;

			if (TryDetachPlayerUsedActiveVehicleFromZoneCleanup(state, zone, activeGroup))
			{
				state.m_aActiveGroups.Remove(i);
				foldedGroups++;
				changed = true;
				continue;
			}

			int beforeInfantry;
			int beforeVehicles;
			HST_GarrisonState beforeGarrison = state.FindGarrison(activeGroup.m_sZoneId, activeGroup.m_sFactionKey);
			if (beforeGarrison)
			{
				beforeInfantry = beforeGarrison.m_iInfantryCount;
				beforeVehicles = beforeGarrison.m_iVehicleCount;
			}

			FoldActiveGroup(state, activeGroup);
			HST_GarrisonState afterGarrison = state.FindGarrison(activeGroup.m_sZoneId, activeGroup.m_sFactionKey);
			if (afterGarrison)
			{
				returnedInfantry += Math.Max(0, afterGarrison.m_iInfantryCount - beforeInfantry);
				returnedVehicles += Math.Max(0, afterGarrison.m_iVehicleCount - beforeVehicles);
			}

			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			state.m_aActiveGroups.Remove(i);
			foldedGroups++;
			changed = true;
		}

		zone.m_iActiveInfantryCount = 0;
		zone.m_iActiveVehicleCount = 0;
		ClearActiveVehicleSpawnBlocked(zone.m_sZoneId);
		if (compositions)
			changed = compositions.CleanupZoneComposition(zone.m_sZoneId) || changed;
		if (changed)
		{
			HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, zone.m_sOwnerFactionKey);
			int garrisonInfantry;
			int garrisonVehicles;
			if (garrison)
			{
				garrisonInfantry = garrison.m_iInfantryCount;
				garrisonVehicles = garrison.m_iVehicleCount;
			}

			Print(string.Format("h-istasi | deactivated zone %1 | folded groups %2 | returned infantry %3 vehicles %4 | abstract garrison now infantry %5 vehicles %6", zone.m_sZoneId, foldedGroups, returnedInfantry, returnedVehicles, garrisonInfantry, garrisonVehicles));
		}
		return changed;
	}

	protected bool TryDetachPlayerUsedActiveVehicleFromZoneCleanup(HST_CampaignState state, HST_ZoneState zone, HST_ActiveGroupState activeGroup)
	{
		if (!state || !zone || !activeGroup || activeGroup.m_iVehicleCount <= 0)
			return false;

		IEntity vehicle = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		if (!vehicle || !IsLivingEntity(vehicle))
			return false;

		string reason;
		if (!ShouldDetachActiveVehicleFromZoneCleanup(vehicle, activeGroup, reason))
			return false;

		RegisterDetachedActiveVehicle(state, zone, activeGroup, vehicle, reason);
		DeleteRuntimeGroupEntity(activeGroup.m_sGroupId, false);
		Print(string.Format("h-istasi | detached player-used active vehicle %1 from zone cleanup | zone %2 | reason %3 | position %4", activeGroup.m_sGroupId, zone.m_sZoneId, reason, vehicle.GetOrigin()));
		return true;
	}

	protected bool ShouldDetachActiveVehicleFromZoneCleanup(IEntity vehicle, HST_ActiveGroupState activeGroup, out string reason)
	{
		reason = "";
		if (!vehicle || !activeGroup)
			return false;

		if (IsAnyLivingPlayerInVehicle(vehicle))
		{
			reason = "player occupied";
			return true;
		}

		float distanceSq = DistanceSq2D(vehicle.GetOrigin(), activeGroup.m_vPosition);
		if (distanceSq >= PLAYER_USED_ACTIVE_VEHICLE_DETACH_DISTANCE_METERS * PLAYER_USED_ACTIVE_VEHICLE_DETACH_DISTANCE_METERS)
		{
			reason = "moved from active-zone spawn";
			return true;
		}

		return false;
	}

	protected bool IsActiveVehicleSpawnBlocked(string zoneId)
	{
		return !zoneId.IsEmpty() && m_aVehicleSpawnBlockedZoneIds.Contains(zoneId);
	}

	protected void RecordActiveVehicleSpawnBlocked(HST_ZoneState zone, string reason)
	{
		if (!zone)
			return;

		string zoneId = zone.m_sZoneId;
		if (zoneId.IsEmpty())
			return;

		int index = m_aVehicleSpawnBlockedZoneIds.Find(zoneId);
		if (index >= 0)
		{
			if (index < m_aVehicleSpawnBlockedReasons.Count())
				m_aVehicleSpawnBlockedReasons[index] = reason;
			return;
		}

		m_aVehicleSpawnBlockedZoneIds.Insert(zoneId);
		m_aVehicleSpawnBlockedReasons.Insert(reason);
		Print(string.Format("h-istasi | vehicle spawn blocked for %1 until zone unload: %2", zoneId, reason), LogLevel.WARNING);
	}

	protected void ClearActiveVehicleSpawnBlocked(string zoneId)
	{
		if (zoneId.IsEmpty())
			return;

		for (int i = m_aVehicleSpawnBlockedZoneIds.Count() - 1; i >= 0; i--)
		{
			if (m_aVehicleSpawnBlockedZoneIds[i] != zoneId)
				continue;

			if (i < m_aVehicleSpawnBlockedReasons.Count())
				m_aVehicleSpawnBlockedReasons.Remove(i);
			m_aVehicleSpawnBlockedZoneIds.Remove(i);
		}
	}

	protected void RegisterDetachedActiveVehicle(HST_CampaignState state, HST_ZoneState zone, HST_ActiveGroupState activeGroup, IEntity vehicle, string reason)
	{
		if (!state || !activeGroup || !vehicle)
			return;

		string runtimeId = ResolveActiveVehicleRuntimeId(vehicle);
		if (runtimeId.IsEmpty())
			return;

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(runtimeId);
		if (!runtimeVehicle)
		{
			runtimeVehicle = new HST_RuntimeVehicleState();
			runtimeVehicle.m_sVehicleRuntimeId = runtimeId;
			state.m_aRuntimeVehicles.Insert(runtimeVehicle);
		}

		string prefab = activeGroup.m_sPrefab;
		if (prefab.IsEmpty() && vehicle.GetPrefabData())
			prefab = vehicle.GetPrefabData().GetPrefabName();

		runtimeVehicle.m_sVehicleRuntimeId = runtimeId;
		runtimeVehicle.m_sPrefab = prefab;
		runtimeVehicle.m_sDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(prefab, vehicle.GetName());
		runtimeVehicle.m_sFactionKey = activeGroup.m_sFactionKey;
		runtimeVehicle.m_sZoneId = activeGroup.m_sZoneId;
		runtimeVehicle.m_sRuntimeKind = "detached_active_vehicle";
		runtimeVehicle.m_vPosition = vehicle.GetOrigin();
		runtimeVehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(vehicle.GetYawPitchRoll());
		runtimeVehicle.m_iSpawnedAtSecond = activeGroup.m_iSpawnedAtSecond;
		runtimeVehicle.m_bDetached = true;
		runtimeVehicle.m_bDeleted = false;
	}

	protected bool IsAnyLivingPlayerInVehicle(IEntity vehicle)
	{
		if (!vehicle)
			return false;

		string vehicleRuntimeId = ResolveActiveVehicleRuntimeId(vehicle);
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			IEntity playerVehicle = ResolveEntityVehicle(playerEntity);
			if (!playerVehicle)
				continue;
			if (playerVehicle == vehicle)
				return true;

			string playerVehicleRuntimeId = ResolveActiveVehicleRuntimeId(playerVehicle);
			if (!vehicleRuntimeId.IsEmpty() && playerVehicleRuntimeId == vehicleRuntimeId)
				return true;
		}

		return false;
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

	protected string ResolveActiveVehicleRuntimeId(IEntity vehicle)
	{
		if (!vehicle)
			return "";

		BaseRplComponent rpl = BaseRplComponent.Cast(vehicle.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("rpl_%1", rpl.Id());

		string prefab = "";
		if (vehicle.GetPrefabData())
			prefab = vehicle.GetPrefabData().GetPrefabName();

		return string.Format("local_%1_%2", prefab, vehicle.GetOrigin());
	}

	protected int SpawnZoneInfantryGroups(HST_CampaignState state, HST_ZoneState zone, HST_CampaignPreset preset, array<ref HST_ZoneSpawnSlotState> slots, int infantryCount, HST_ZoneCompositionService compositions)
	{
		if (infantryCount <= 0)
			return 0;

		int groupCount = ResolveInfantryGroupCount(infantryCount);
		int remainingInfantry = infantryCount;
		int spawnedGroups;
		bool distributedGuardAssigned;
		for (int groupIndex = 0; groupIndex < groupCount; groupIndex++)
		{
			int remainingGroups = groupCount - groupIndex;
			int groupInfantry = Math.Max(1, remainingInfantry / remainingGroups);
			remainingInfantry = Math.Max(0, remainingInfantry - groupInfantry);

			HST_ZoneSpawnSlotState slot;
			string runtimeStatus = "guard_center";
			if (compositions)
			{
				if (groupIndex == 0)
				{
					slot = compositions.SelectSlot(slots, HST_ZoneCompositionService.SLOT_INFANTRY, 0);
				}
				else
				{
					slot = compositions.SelectSlot(slots, HST_ZoneCompositionService.SLOT_PATROL, groupIndex - 1);
					runtimeStatus = "guard_distributed";
					distributedGuardAssigned = true;
				}

				if (!slot)
					slot = compositions.SelectSlot(slots, HST_ZoneCompositionService.SLOT_INFANTRY, groupIndex);
			}

			if (SpawnActiveZoneGroup(state, zone, zone.m_sOwnerFactionKey, groupInfantry, 0, false, preset, slot, runtimeStatus))
				spawnedGroups++;
		}

		if (distributedGuardAssigned && compositions)
			compositions.ReportDistributedGuardSlots(zone.m_sZoneId);

		return spawnedGroups;
	}

	protected int SpawnZoneVehicleGroups(HST_CampaignState state, HST_ZoneState zone, HST_CampaignPreset preset, array<ref HST_ZoneSpawnSlotState> slots, int vehicleCount)
	{
		if (vehicleCount <= 0)
			return 0;

		int spawnedVehicles;
		for (int vehicleIndex = 0; vehicleIndex < vehicleCount; vehicleIndex++)
		{
			HST_ZoneSpawnSlotState slot;
			if (slots)
			{
				foreach (HST_ZoneSpawnSlotState candidate : slots)
				{
					if (!candidate || candidate.m_sKind != HST_ZoneCompositionService.SLOT_VEHICLE || candidate.m_bOccupied)
						continue;

					candidate.m_bOccupied = true;
					slot = candidate;
					break;
				}
			}

			if (!slot)
			{
				FoldUnspawnedForces(state, zone.m_sZoneId, zone.m_sOwnerFactionKey, 0, 1);
				RecordActiveVehicleSpawnBlocked(zone, "no safe vehicle slot");
				continue;
			}

			if (SpawnActiveZoneGroup(state, zone, zone.m_sOwnerFactionKey, 0, 1, false, preset, slot, "vehicle_guard"))
				spawnedVehicles++;
		}

		return spawnedVehicles;
	}

	protected bool SpawnActiveZoneGroup(HST_CampaignState state, HST_ZoneState zone, string factionKey, int infantryCount, int vehicleCount, bool qrf, HST_CampaignPreset preset, HST_ZoneSpawnSlotState slot, string runtimeStatus)
	{
		HST_ActiveGroupState activeGroup = CreateActiveGroup(state, zone, factionKey, infantryCount, vehicleCount, qrf, preset);
		ApplySpawnSlot(activeGroup, slot, runtimeStatus);
		if (vehicleCount > 0 && infantryCount <= 0)
		{
			activeGroup.m_sPrefab = SelectVehiclePrefab(state, zone, factionKey, state.m_aActiveGroups.Count(), preset);
			activeGroup.m_sSpawnFallbackMode = "vehicle";
		}

		state.m_aActiveGroups.Insert(activeGroup);
		bool spawned;
		if (vehicleCount > 0 && infantryCount <= 0)
			spawned = TrySpawnActiveVehicle(activeGroup, state);
		else
			spawned = TrySpawnActiveGroup(activeGroup, state, preset);

		if (!spawned)
		{
			FoldActiveGroup(state, activeGroup);
			state.m_aActiveGroups.Remove(state.m_aActiveGroups.Count() - 1);
			ApplyActiveZoneCounts(state, zone);
			NotifyRuntimeEvent(state, "ai_spawn_failed_" + activeGroup.m_sGroupId, "Enemy Spawn Failed", string.Format("%1 could not spawn at %2. %3", zone.m_sDisplayName, zone.m_sZoneId, activeGroup.m_sSpawnFailureReason), zone.m_sZoneId, activeGroup.m_vPosition, 6.0);
			return false;
		}

		return true;
	}

	protected void ApplySpawnSlot(HST_ActiveGroupState activeGroup, HST_ZoneSpawnSlotState slot, string runtimeStatus)
	{
		if (!activeGroup)
			return;

		if (slot)
		{
			activeGroup.m_vPosition = slot.m_vPosition;
			activeGroup.m_vSourcePosition = slot.m_vPosition;
			activeGroup.m_vTargetPosition = slot.m_vPosition;
			activeGroup.m_sSpawnFallbackMode = slot.m_sSlotId;
		}

		if (!runtimeStatus.IsEmpty())
			activeGroup.m_sRuntimeStatus = runtimeStatus;
	}

	protected int ResolveInfantryGroupCount(int infantryCount)
	{
		if (infantryCount <= 4)
			return 1;
		if (infantryCount <= 8)
			return 2;
		if (infantryCount <= 12)
			return 3;

		return 4;
	}

	protected void FoldUnspawnedForces(HST_CampaignState state, string zoneId, string factionKey, int infantryCount, int vehicleCount)
	{
		if (!state || zoneId.IsEmpty() || factionKey.IsEmpty())
			return;

		HST_GarrisonState garrison = state.FindGarrison(zoneId, factionKey);
		if (!garrison)
		{
			garrison = new HST_GarrisonState();
			garrison.m_sZoneId = zoneId;
			garrison.m_sFactionKey = factionKey;
			state.m_aGarrisons.Insert(garrison);
		}

		garrison.m_iInfantryCount += Math.Max(0, infantryCount);
		garrison.m_iVehicleCount += Math.Max(0, vehicleCount);
	}

	protected bool UpdateQRF(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector)
	{
		bool changed = SpawnPendingQRFs(state, preset);
		changed = ResolveArrivedQRFs(state) || changed;
		if (!enemyDirector)
			return changed;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone.m_bActive || zone.m_sOwnerFactionKey == resistanceFactionKey || zone.m_sQRFRouteId.IsEmpty())
				continue;

			if (IsZoneInsideHQSafeArea(state, zone))
				continue;

			if (state.m_iElapsedSeconds < zone.m_iQrfCooldownUntilSecond)
				continue;

			if (state.FindActiveQRF(zone.m_sZoneId, zone.m_sOwnerFactionKey))
				continue;

			string qrfSpendReason;
			if (!enemyDirector.TrySpendDefense(state, zone, zone.m_sOwnerFactionKey, QRF_ATTACK_RESOURCE_COST, QRF_SUPPORT_RESOURCE_COST, qrfSpendReason))
				continue;

			HST_QRFState qrf = new HST_QRFState();
			qrf.m_sInstanceId = string.Format("qrf_%1_%2_%3", zone.m_sZoneId, zone.m_sOwnerFactionKey, state.m_iElapsedSeconds);
			qrf.m_sFactionKey = zone.m_sOwnerFactionKey;
			qrf.m_sSourceZoneId = ResolveQRFSourceZoneId(state, zone, zone.m_sOwnerFactionKey);
			qrf.m_sTargetZoneId = zone.m_sZoneId;
			qrf.m_iStartedAtSecond = state.m_iElapsedSeconds;
			qrf.m_iETASeconds = QRF_ETA_SECONDS;
			state.m_aQRFs.Insert(qrf);
			zone.m_iQrfCooldownUntilSecond = state.m_iElapsedSeconds + QRF_COOLDOWN_SECONDS;
			m_bMarkerRefreshNeeded = true;
			Print(string.Format("h-istasi | dispatched QRF %1 from %2 to active zone %3 | physical spawn at T-%4s", qrf.m_sInstanceId, qrf.m_sSourceZoneId, zone.m_sZoneId, QRF_INBOUND_SPAWN_SECONDS));
			NotifyRuntimeEvent(state, "qrf_dispatched_" + qrf.m_sInstanceId, "QRF Dispatched", string.Format("%1 is sending a quick reaction force toward %2.", zone.m_sOwnerFactionKey, ResolveZoneDisplayName(state, zone.m_sZoneId)), zone.m_sZoneId, zone.m_vPosition, 6.0);
			changed = true;
		}

		return changed;
	}

	protected bool SpawnPendingQRFs(HST_CampaignState state, HST_CampaignPreset preset = null)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (!qrf || qrf.m_bResolved || !qrf.m_sGroupId.IsEmpty())
				continue;

			int spawnAtSecond = qrf.m_iStartedAtSecond + Math.Max(0, qrf.m_iETASeconds - QRF_INBOUND_SPAWN_SECONDS);
			if (state.m_iElapsedSeconds < spawnAtSecond)
				continue;

			HST_ZoneState targetZone = state.FindZone(qrf.m_sTargetZoneId);
			if (!targetZone)
			{
				qrf.m_bResolved = true;
				qrf.m_bSucceeded = false;
				m_bMarkerRefreshNeeded = true;
				Print(string.Format("h-istasi | QRF %1 failed to stage near objective: missing target zone %2", qrf.m_sInstanceId, qrf.m_sTargetZoneId), LogLevel.WARNING);
				changed = true;
				continue;
			}

			if (qrf.m_sSourceZoneId.IsEmpty())
				qrf.m_sSourceZoneId = ResolveQRFSourceZoneId(state, targetZone, qrf.m_sFactionKey);

			int infantryCount = Math.Min(ResolveActiveInfantryCap(targetZone), Math.Max(2, targetZone.m_iGarrisonSlots / 3 + state.m_iWarLevel / 2));
			int vehicleCount = ResolveActiveVehicleCap(targetZone);
			HST_ActiveGroupState activeGroup = CreateActiveGroup(state, targetZone, qrf.m_sFactionKey, infantryCount, vehicleCount, true, preset);
			vector targetPosition = HST_WorldPositionService.ResolveGroundPosition(targetZone.m_vPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
			vector sourcePosition = ResolveQRFStagingPosition(state, qrf, targetZone, targetPosition);
			string fallbackRouteId = qrf.m_sSourceZoneId + "_to_" + qrf.m_sTargetZoneId;
			activeGroup.m_sRouteId = ResolvePhysicalResponseRouteId(state, targetZone, fallbackRouteId);
			activeGroup.m_sQRFInstanceId = qrf.m_sInstanceId;
			activeGroup.m_vSourcePosition = sourcePosition;
			activeGroup.m_vTargetPosition = targetPosition;
			activeGroup.m_vPosition = sourcePosition;
			activeGroup.m_sRuntimeStatus = "routing";
			activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
			state.m_aActiveGroups.Insert(activeGroup);
			if (!TrySpawnActiveGroup(activeGroup, state, preset))
			{
				state.m_aActiveGroups.Remove(state.m_aActiveGroups.Count() - 1);
				qrf.m_bResolved = true;
				qrf.m_bSucceeded = false;
				m_bMarkerRefreshNeeded = true;
				NotifyRuntimeEvent(state, "qrf_spawn_failed_" + qrf.m_sInstanceId, "QRF Spawn Failed", string.Format("%1 QRF could not spawn for %2. %3", qrf.m_sFactionKey, ResolveZoneDisplayName(state, targetZone.m_sZoneId), activeGroup.m_sSpawnFailureReason), targetZone.m_sZoneId, activeGroup.m_vPosition, 6.0);
				Print(string.Format("h-istasi | QRF %1 failed to materialize near %2 | prefab %3 | reason %4", qrf.m_sInstanceId, targetZone.m_sZoneId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
				changed = true;
				continue;
			}

			qrf.m_sGroupId = activeGroup.m_sGroupId;
			Print(string.Format("h-istasi | QRF %1 spawned near %2 | source %3 | spawn %4 | objective %5 | group %6", qrf.m_sInstanceId, targetZone.m_sZoneId, qrf.m_sSourceZoneId, activeGroup.m_vPosition, targetPosition, activeGroup.m_sGroupId));
			changed = true;
		}

		return changed;
	}

	protected bool ResolveArrivedQRFs(HST_CampaignState state)
	{
		bool changed;
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (!qrf || qrf.m_bResolved)
				continue;

			if (state.m_iElapsedSeconds < qrf.m_iStartedAtSecond + qrf.m_iETASeconds)
				continue;

			if (qrf.m_sGroupId.IsEmpty())
			{
				qrf.m_bResolved = true;
				qrf.m_bSucceeded = false;
				m_bMarkerRefreshNeeded = true;
				changed = true;
				Print(string.Format("h-istasi | QRF %1 failed to reach zone %2 | no physical group materialized", qrf.m_sInstanceId, qrf.m_sTargetZoneId), LogLevel.WARNING);
				continue;
			}

			HST_ActiveGroupState activeGroup = state.FindActiveGroup(qrf.m_sGroupId);
			if (activeGroup && state.m_iElapsedSeconds < activeGroup.m_iSpawnedAtSecond + ROUTE_STATE_UPDATE_SECONDS)
				continue;

			qrf.m_bResolved = true;
			if (!IsActiveGroupCombatEffective(activeGroup))
			{
				qrf.m_bSucceeded = false;
				m_bMarkerRefreshNeeded = true;
				changed = true;
				Print(string.Format("h-istasi | QRF %1 failed to reach zone %2 | group %3 status %4 spawned agents %5 alive %6", qrf.m_sInstanceId, qrf.m_sTargetZoneId, qrf.m_sGroupId, ResolveActiveGroupStatus(activeGroup), ResolveSpawnedAgentTotal(activeGroup), ResolveAliveAgentTotal(activeGroup)), LogLevel.WARNING);
				continue;
			}

			qrf.m_bSucceeded = true;
			if (activeGroup)
				activeGroup.m_sRuntimeStatus = "arrived";
			changed = true;
			Print(string.Format("h-istasi | QRF %1 active near zone %2", qrf.m_sInstanceId, qrf.m_sTargetZoneId));
		}

		return changed;
	}

	protected string ResolveQRFSourceZoneId(HST_CampaignState state, HST_ZoneState targetZone, string factionKey)
	{
		if (!state || !targetZone || factionKey.IsEmpty())
			return "";

		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sZoneId == targetZone.m_sZoneId || zone.m_sOwnerFactionKey != factionKey)
				continue;

			float distanceSq = DistanceSq2D(zone.m_vPosition, targetZone.m_vPosition);
			if (distanceSq < bestDistanceSq)
			{
				bestDistanceSq = distanceSq;
				bestZone = zone;
			}
		}

		if (bestZone)
			return bestZone.m_sZoneId;

		return targetZone.m_sZoneId;
	}

	protected vector ResolveQRFSourcePosition(HST_CampaignState state, HST_QRFState qrf, HST_ZoneState targetZone)
	{
		if (state && qrf && !qrf.m_sSourceZoneId.IsEmpty())
		{
			HST_ZoneState sourceZone = state.FindZone(qrf.m_sSourceZoneId);
			if (sourceZone)
				return sourceZone.m_vPosition;
		}

		if (targetZone)
			return targetZone.m_vPosition;

		return "0 0 0";
	}

	protected vector ResolveQRFStagingPosition(HST_CampaignState state, HST_QRFState qrf, HST_ZoneState targetZone, vector targetPosition)
	{
		vector sourcePosition = ResolveQRFSourcePosition(state, qrf, targetZone);
		float standoff = ResolveQRFStagingDistanceMeters(targetZone);
		int seed = BuildQRFInboundSeed(state, qrf, targetZone);
		for (int attempt = 0; attempt < 32; attempt++)
		{
			vector candidate = BuildQRFApproachCandidate(targetPosition, sourcePosition, seed, attempt, standoff);
			vector resolved;
			if (!HST_WorldPositionService.TryResolveDryStagingPosition(candidate, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, resolved, 3.0))
				continue;
			if (IsInsideHQSafeRadius(state, resolved))
				continue;

			return resolved;
		}

		vector fallback;
		if (HST_WorldPositionService.TryResolveDryStagingPosition(sourcePosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, fallback, 3.0) && !IsInsideHQSafeRadius(state, fallback))
			return fallback;

		if (HST_WorldPositionService.TryResolveDryStagingPosition(targetPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, fallback, 3.0) && !IsInsideHQSafeRadius(state, fallback))
			return fallback;

		Print(string.Format("h-istasi | QRF %1 could not find dry staging near %2, using objective fallback %3", qrf.m_sInstanceId, targetZone.m_sZoneId, targetPosition), LogLevel.WARNING);
		return HST_WorldPositionService.ResolveSafeGroundPosition(targetPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 8.0);
	}

	protected float ResolveQRFStagingDistanceMeters(HST_ZoneState targetZone)
	{
		return Math.Max(QRF_MIN_STANDOFF_METERS, ResolveQRFStandoffMeters(targetZone) * 0.5);
	}

	protected float ResolveQRFStandoffMeters(HST_ZoneState targetZone)
	{
		float radius;
		if (targetZone)
			radius = targetZone.m_iCaptureRadiusMeters;
		if (radius <= 0 && targetZone)
			radius = targetZone.m_iActivationRadiusMeters * 0.35;
		if (radius <= 0)
			radius = QRF_MIN_STANDOFF_METERS;

		return Math.Min(QRF_MAX_STANDOFF_METERS, Math.Max(QRF_MIN_STANDOFF_METERS, radius + QRF_EXTRA_STANDOFF_METERS));
	}

	protected int BuildQRFInboundSeed(HST_CampaignState state, HST_QRFState qrf, HST_ZoneState targetZone)
	{
		int seed = 3371;
		if (state)
			seed += state.m_iCampaignSeed * 17 + state.m_iElapsedSeconds * 5 + state.m_aActiveGroups.Count() * 61;
		if (qrf)
			seed += qrf.m_sInstanceId.Length() * 43 + qrf.m_sFactionKey.Length() * 19 + qrf.m_sSourceZoneId.Length() * 29;
		if (targetZone)
			seed += targetZone.m_iPriority * 37 + targetZone.m_sZoneId.Length() * 83 + Math.Round(targetZone.m_vPosition[0]) + Math.Round(targetZone.m_vPosition[2]);
		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected vector BuildQRFApproachCandidate(vector target, vector source, int seed, int attempt, float standoffMeters)
	{
		vector candidate = target;
		float dx = target[0] - source[0];
		float dz = target[2] - source[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length > 1.0 && attempt < 2)
		{
			float distance = standoffMeters + attempt * 45.0;
			candidate[0] = target[0] - dx / length * distance;
			candidate[2] = target[2] - dz / length * distance;
			return candidate;
		}

		int slot = HST_DefaultCatalog.PositiveMod(seed + attempt, 8);
		float distanceBySlot = standoffMeters + (attempt / 8) * 55.0;
		float x = 1.0;
		float z = 0.0;
		if (slot == 1)
		{
			x = 0.707;
			z = 0.707;
		}
		else if (slot == 2)
		{
			x = 0.0;
			z = 1.0;
		}
		else if (slot == 3)
		{
			x = -0.707;
			z = 0.707;
		}
		else if (slot == 4)
		{
			x = -1.0;
			z = 0.0;
		}
		else if (slot == 5)
		{
			x = -0.707;
			z = -0.707;
		}
		else if (slot == 6)
		{
			x = 0.0;
			z = -1.0;
		}
		else if (slot == 7)
		{
			x = 0.707;
			z = -0.707;
		}

		candidate[0] = target[0] + x * distanceBySlot;
		candidate[2] = target[2] + z * distanceBySlot;
		return candidate;
	}

	protected bool IsActiveGroupCombatEffective(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		if (activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed")
			return false;

		if (activeGroup.m_iSpawnedAgentCount <= 0)
			return false;

		if (activeGroup.m_iLastSeenAliveCount > 0)
			return true;

		return activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iSurvivorVehicleCount > 0;
	}

	protected string ResolveActiveGroupStatus(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "missing";

		return activeGroup.m_sRuntimeStatus;
	}

	protected int ResolveSpawnedAgentTotal(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return 0;

		return activeGroup.m_iSpawnedAgentCount;
	}

	protected int ResolveAliveAgentTotal(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return 0;

		return activeGroup.m_iLastSeenAliveCount;
	}

	protected bool UpdateActiveGroupRoutes(HST_CampaignState state, bool forceRouteUpdate = false)
	{
		if (!state || (!forceRouteUpdate && state.m_iElapsedSeconds % ROUTE_STATE_UPDATE_SECONDS != 0))
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || (activeGroup.m_sRuntimeStatus != "routing" && activeGroup.m_sRuntimeStatus != "support_active"))
				continue;

			ref array<vector> routePositions = BuildActiveGroupRoutePositions(ResolveActiveGroupGeneratedRoute(state, activeGroup), activeGroup);
			if (activeGroup.m_bSpawnedEntity && activeGroup.m_iInfantryCount > 0 && !IsMissionConvoyGroup(activeGroup) && !IsActiveGroupInfantryWaypointAssigned(activeGroup))
			{
				bool assignedFinalSweepWaypoint;
				int assignedInfantryWaypoints = AssignActiveGroupInfantryRouteWaypoints(activeGroup, routePositions, assignedFinalSweepWaypoint);
				if (assignedInfantryWaypoints > 1)
				{
					activeGroup.m_iAssignedWaypointCount = assignedInfantryWaypoints;
					activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "infantry_waypoints");
					if (assignedFinalSweepWaypoint)
						activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "infantry_sweep");
					activeGroup.m_sSpawnFailureReason = string.Format("Assigned infantry route waypoint chain %1 | final sweep %2.", assignedInfantryWaypoints, ReportBool(assignedFinalSweepWaypoint));
					changed = true;
				}
			}

			if (!activeGroup.m_bSpawnedEntity)
				continue;

			if (ApplyResponseGroupMovementSpeed(activeGroup, AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId))))
			{
				string responseRunMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "response_run");
				if (activeGroup.m_sSpawnFallbackMode != responseRunMode)
				{
					activeGroup.m_sSpawnFallbackMode = responseRunMode;
					changed = true;
				}
			}

			int duration = ResolveRouteDurationSeconds(state, activeGroup);
			int elapsed = state.m_iElapsedSeconds - activeGroup.m_iSpawnedAtSecond;
			if (elapsed < 0)
				elapsed = 0;

			float progress = Math.Min(1.0, elapsed * 1.0 / duration);
			int assignedWaypointCount;
			vector position = ResolveActiveGroupRoutePosition(routePositions, activeGroup, progress, assignedWaypointCount);
			position = HST_WorldPositionService.ResolveSafeGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
			string arrivedStatus = ResolveArrivedRouteStatus(activeGroup);
			if (!IsActiveGroupInfantryWaypointAssigned(activeGroup) && activeGroup.m_iAssignedWaypointCount != assignedWaypointCount)
			{
				activeGroup.m_iAssignedWaypointCount = assignedWaypointCount;
				changed = true;
			}
			if (DistanceSq2D(activeGroup.m_vPosition, position) < 25)
			{
				if (progress >= 1.0 && activeGroup.m_sRuntimeStatus != arrivedStatus)
				{
					activeGroup.m_sRuntimeStatus = arrivedStatus;
					changed = true;
				}
				continue;
			}

			activeGroup.m_vPosition = position;
			if (!IsActiveGroupInfantryWaypointAssigned(activeGroup))
				SetRuntimeGroupEntitiesOrigin(activeGroup.m_sGroupId, position);

			if (progress >= 1.0)
				activeGroup.m_sRuntimeStatus = arrivedStatus;

			changed = true;
		}

		return changed;
	}

	protected int ResolveRouteDurationSeconds(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return QRF_ETA_SECONDS;

		if (state && activeGroup.m_sRuntimeStatus == "support_active")
		{
			HST_SupportRequestState request = FindSupportRequestByGroupId(state, activeGroup.m_sGroupId);
			if (request)
				return Math.Max(5, request.m_iRequestedAtSecond + request.m_iETASeconds - activeGroup.m_iSpawnedAtSecond);
		}

		if (activeGroup.m_bQRF)
		{
			HST_QRFState qrf = FindQRFByGroupId(state, activeGroup.m_sGroupId);
			if (qrf)
				return Math.Max(5, qrf.m_iStartedAtSecond + qrf.m_iETASeconds - activeGroup.m_iSpawnedAtSecond);

			return QRF_ETA_SECONDS;
		}

		float distance = Math.Sqrt(DistanceSq2D(activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition));
		return Math.Max(120, Math.Round(distance / 6.0));
	}

	protected string ResolveArrivedRouteStatus(HST_ActiveGroupState activeGroup)
	{
		if (activeGroup && activeGroup.m_sRuntimeStatus == "support_active")
			return "support_arrived";

		return "arrived";
	}

	protected vector ResolveActiveGroupRoutePosition(array<vector> routePositions, HST_ActiveGroupState activeGroup, float progress, out int assignedWaypointCount)
	{
		assignedWaypointCount = 0;
		if (!activeGroup)
			return "0 0 0";

		if (!routePositions || routePositions.Count() < 2)
			return LerpPosition(activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition, progress);

		assignedWaypointCount = routePositions.Count();
		return ResolveRoutePolylinePosition(routePositions, progress);
	}

	protected int AssignActiveGroupInfantryRouteWaypoints(HST_ActiveGroupState activeGroup, array<vector> routePositions, out bool assignedFinalSweepWaypoint)
	{
		assignedFinalSweepWaypoint = false;
		if (!activeGroup || activeGroup.m_sGroupId.IsEmpty() || !routePositions || routePositions.Count() < 2)
			return 0;

		AIGroup group = AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId));
		if (!group)
			return 0;

		ResourceName waypointResource = ACTIVE_GROUP_ROUTE_WAYPOINT_PREFAB;
		Resource loaded = Resource.Load(waypointResource);
		if (!loaded || !loaded.IsValid())
			return 0;
		ResourceName sweepWaypointResource = ACTIVE_GROUP_ROUTE_SWEEP_WAYPOINT_PREFAB;
		Resource sweepLoaded = Resource.Load(sweepWaypointResource);
		if (!sweepLoaded || !sweepLoaded.IsValid())
			return 0;

		DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);

		int assignedCount;
		for (int i = 1; i < routePositions.Count(); i++)
		{
			vector waypointPosition = routePositions[i];
			if (IsZeroVector(waypointPosition))
				continue;
			if (DistanceSq2D(activeGroup.m_vPosition, waypointPosition) < 16.0)
				continue;

			bool finalSweepWaypoint = i == routePositions.Count() - 1;
			IEntity waypointEntity = SpawnActiveGroupRouteWaypoint(activeGroup.m_sGroupId, waypointPosition, assignedCount + 1, finalSweepWaypoint);
			AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
			if (!waypoint)
				continue;

			group.AddWaypoint(waypoint);
			m_aRuntimeGroupWaypointIds.Insert(activeGroup.m_sGroupId);
			m_aRuntimeGroupWaypointEntities.Insert(waypointEntity);
			if (finalSweepWaypoint)
				assignedFinalSweepWaypoint = true;
			assignedCount++;
		}

		if (assignedCount <= 1)
		{
			DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);
			return 0;
		}

		if (ApplyResponseGroupMovementSpeed(activeGroup, group))
			activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "response_run");

		return assignedCount;
	}

	protected IEntity SpawnActiveGroupRouteWaypoint(string groupId, vector position, int waypointIndex, bool finalSweepWaypoint = false)
	{
		string waypointPrefab = ACTIVE_GROUP_ROUTE_WAYPOINT_PREFAB;
		float waypointRadius = ACTIVE_GROUP_ROUTE_WAYPOINT_RADIUS_METERS;
		string waypointName = "active_route_waypoint";
		if (finalSweepWaypoint)
		{
			waypointPrefab = ACTIVE_GROUP_ROUTE_SWEEP_WAYPOINT_PREFAB;
			waypointRadius = ACTIVE_GROUP_ROUTE_SWEEP_WAYPOINT_RADIUS_METERS;
			waypointName = "active_route_sweep_waypoint";
		}

		vector waypointPosition = HST_WorldPositionService.ResolveSafeGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 8.0);
		GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(waypointPrefab, waypointPosition, "0 0 0");
		AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
		if (!waypoint)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
			return null;
		}

		ApplyCampaignDebugEntityName(waypointEntity, string.Format("%1_%2", waypointName, waypointIndex), groupId);
		waypoint.SetCompletionRadius(waypointRadius);
		return waypointEntity;
	}

	protected bool IsActiveGroupInfantryWaypointAssigned(HST_ActiveGroupState activeGroup)
	{
		return activeGroup && activeGroup.m_iAssignedWaypointCount > 1 && activeGroup.m_sSpawnFallbackMode.Contains("infantry_waypoints");
	}

	protected bool ApplyResponseGroupMovementSpeed(HST_ActiveGroupState activeGroup, AIGroup group)
	{
		if (!ShouldUseResponseMovementSpeed(activeGroup))
			return false;

		bool applied;

		if (group)
		{
			AIGroupMovementComponent groupMovement = AIGroupMovementComponent.Cast(group.GetMovementComponent());
			if (!groupMovement)
				groupMovement = AIGroupMovementComponent.Cast(group.FindComponent(AIGroupMovementComponent));
			if (groupMovement)
			{
				groupMovement.SetGroupCharactersWantedMovementType(EMovementType.RUN);
				groupMovement.SetFormationDisplacement(1);
				applied = true;
			}
		}

		int agentAppliedCount = ApplyResponseAgentMovementSpeed(activeGroup.m_sGroupId);
		return applied || agentAppliedCount > 0;
	}

	protected int ApplyResponseAgentMovementSpeed(string groupId)
	{
		if (groupId.IsEmpty())
			return 0;

		int appliedCount;
		array<IEntity> livingEntities = {};
		CollectLivingRuntimeGroupEntities(groupId, livingEntities);
		foreach (IEntity entity : livingEntities)
		{
			if (!entity)
				continue;

			AIControlComponent control = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
			if (!control)
				continue;

			AIAgent agent = control.GetControlAIAgent();
			if (!agent)
				continue;

			AICharacterMovementComponent movement = AICharacterMovementComponent.Cast(agent.GetMovementComponent());
			if (!movement)
				continue;

			movement.SetMovementTypeWanted(EMovementType.RUN);
			appliedCount++;
		}

		return appliedCount;
	}

	protected bool ShouldUseResponseMovementSpeed(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		if (!activeGroup.m_sSupportRequestId.IsEmpty())
			return true;
		if (!activeGroup.m_sQRFInstanceId.IsEmpty() || activeGroup.m_bQRF)
			return true;
		if (activeGroup.m_sSpawnFallbackMode.Contains("petros_attack_support"))
			return true;

		return false;
	}

	protected string AppendActiveGroupSpawnModeToken(string mode, string token)
	{
		if (token.IsEmpty())
			return mode;
		if (mode.IsEmpty())
			return token;
		if (mode.Contains(token))
			return mode;

		return mode + "_" + token;
	}

	protected void AppendActiveGroupSpawnFailureNote(HST_ActiveGroupState activeGroup, string note)
	{
		if (!activeGroup || note.IsEmpty())
			return;
		if (activeGroup.m_sSpawnFailureReason.IsEmpty())
		{
			activeGroup.m_sSpawnFailureReason = note;
			return;
		}
		if (activeGroup.m_sSpawnFailureReason.Contains(note))
			return;

		activeGroup.m_sSpawnFailureReason = activeGroup.m_sSpawnFailureReason + " " + note;
	}

	protected HST_GeneratedRouteState ResolveActiveGroupGeneratedRoute(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return null;

		if (!activeGroup.m_sRouteId.IsEmpty())
		{
			HST_GeneratedRouteState directRoute = state.FindGeneratedRoute(activeGroup.m_sRouteId);
			if (directRoute)
				return directRoute;
		}

		HST_ZoneState zone = state.FindZone(activeGroup.m_sZoneId);
		return ResolveGeneratedResponseRouteForZone(state, zone);
	}

	protected HST_GeneratedRouteState ResolveGeneratedResponseRouteForZone(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return null;

		if (!zone.m_sQRFRouteId.IsEmpty())
		{
			HST_GeneratedRouteState qrfRoute = state.FindGeneratedRoute(zone.m_sQRFRouteId);
			if (qrfRoute)
				return qrfRoute;
		}

		if (!zone.m_sPatrolRouteId.IsEmpty())
		{
			HST_GeneratedRouteState patrolRoute = state.FindGeneratedRoute(zone.m_sPatrolRouteId);
			if (patrolRoute)
				return patrolRoute;
		}

		string generatedRouteId = "route_" + zone.m_sZoneId + "_alpha";
		return state.FindGeneratedRoute(generatedRouteId);
	}

	protected string ResolvePhysicalResponseRouteId(HST_CampaignState state, HST_ZoneState targetZone, string fallbackRouteId)
	{
		HST_GeneratedRouteState route = ResolveGeneratedResponseRouteForZone(state, targetZone);
		if (route)
			return route.m_sRouteId;

		return fallbackRouteId;
	}

	protected ref array<vector> BuildActiveGroupRoutePositions(HST_GeneratedRouteState route, HST_ActiveGroupState activeGroup)
	{
		ref array<vector> positions = {};
		if (!route || !activeGroup)
			return positions;

		AppendActiveGroupRoutePosition(positions, activeGroup.m_vSourcePosition);
		int lastIndex = -1000000;
		while (true)
		{
			HST_RouteWaypointState selectedWaypoint;
			int selectedIndex = 1000000;
			foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
			{
				if (!waypoint)
					continue;
				if (waypoint.m_iIndex <= lastIndex)
					continue;
				if (waypoint.m_iIndex >= selectedIndex)
					continue;

				selectedWaypoint = waypoint;
				selectedIndex = waypoint.m_iIndex;
			}

			if (!selectedWaypoint)
				break;

			AppendActiveGroupRoutePosition(positions, selectedWaypoint.m_vPosition);
			lastIndex = selectedWaypoint.m_iIndex;
		}

		AppendActiveGroupRoutePosition(positions, activeGroup.m_vTargetPosition);
		return positions;
	}

	protected void AppendActiveGroupRoutePosition(array<vector> positions, vector position)
	{
		if (!positions || IsZeroVector(position))
			return;
		if (positions.Count() > 0 && DistanceSq2D(positions[positions.Count() - 1], position) < 9.0)
			return;

		positions.Insert(position);
	}

	protected vector ResolveRoutePolylinePosition(array<vector> positions, float progress)
	{
		if (!positions || positions.Count() == 0)
			return "0 0 0";
		if (positions.Count() == 1)
			return positions[0];

		float totalDistance;
		for (int i = 1; i < positions.Count(); i++)
			totalDistance += Math.Sqrt(DistanceSq2D(positions[i - 1], positions[i]));

		if (totalDistance <= 1.0)
			return positions[positions.Count() - 1];

		float targetDistance = Math.Max(0.0, Math.Min(1.0, progress)) * totalDistance;
		float traversedDistance;
		for (int segmentIndex = 1; segmentIndex < positions.Count(); segmentIndex++)
		{
			vector fromPosition = positions[segmentIndex - 1];
			vector toPosition = positions[segmentIndex];
			float segmentDistance = Math.Sqrt(DistanceSq2D(fromPosition, toPosition));
			if (segmentDistance <= 0.1)
				continue;

			if (traversedDistance + segmentDistance >= targetDistance)
			{
				float segmentProgress = (targetDistance - traversedDistance) / segmentDistance;
				return LerpPosition(fromPosition, toPosition, segmentProgress);
			}

			traversedDistance += segmentDistance;
		}

		return positions[positions.Count() - 1];
	}

	protected HST_QRFState FindQRFByGroupId(HST_CampaignState state, string groupId)
	{
		if (!state || groupId.IsEmpty())
			return null;

		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (qrf && qrf.m_sGroupId == groupId)
				return qrf;
		}

		return null;
	}

	protected HST_SupportRequestState FindSupportRequestByGroupId(HST_CampaignState state, string groupId)
	{
		if (!state || groupId.IsEmpty())
			return null;

		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (request && request.m_sGroupId == groupId)
				return request;
		}

		return null;
	}

	protected vector LerpPosition(vector sourcePosition, vector targetPosition, float progress)
	{
		vector result;
		result[0] = sourcePosition[0] + (targetPosition[0] - sourcePosition[0]) * progress;
		result[1] = sourcePosition[1] + (targetPosition[1] - sourcePosition[1]) * progress;
		result[2] = sourcePosition[2] + (targetPosition[2] - sourcePosition[2]) * progress;
		return result;
	}

	protected HST_ActiveGroupState CreateActiveGroup(HST_CampaignState state, HST_ZoneState zone, string factionKey, int infantryCount, int vehicleCount, bool qrf, HST_CampaignPreset preset = null)
	{
		HST_ActiveGroupState activeGroup = new HST_ActiveGroupState();
		activeGroup.m_sGroupId = BuildGroupId(state, zone, factionKey, qrf);
		activeGroup.m_sZoneId = zone.m_sZoneId;
		activeGroup.m_sFactionKey = factionKey;
		if (!qrf)
			activeGroup.m_sGarrisonZoneId = zone.m_sZoneId;
		activeGroup.m_sPrefab = SelectGroupPrefab(state, zone, factionKey, qrf, preset);
		activeGroup.m_sRouteId = ResolveGroupRouteId(zone, qrf);
		activeGroup.m_vSourcePosition = ResolveGroupSourcePosition(state, zone, activeGroup.m_sRouteId, qrf);
		activeGroup.m_vTargetPosition = ResolveGroupTargetPosition(state, zone, activeGroup.m_sRouteId, qrf);
		activeGroup.m_vPosition = HST_WorldPositionService.ResolveSafeGroundPosition(activeGroup.m_vSourcePosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		activeGroup.m_sRuntimeStatus = "queued";
		activeGroup.m_iInfantryCount = infantryCount;
		activeGroup.m_iVehicleCount = vehicleCount;
		activeGroup.m_iOriginalInfantryCount = infantryCount;
		activeGroup.m_iOriginalVehicleCount = vehicleCount;
		activeGroup.m_iLastSeenAliveCount = infantryCount + vehicleCount;
		activeGroup.m_iSurvivorInfantryCount = infantryCount;
		activeGroup.m_iSurvivorVehicleCount = vehicleCount;
		activeGroup.m_bQRF = qrf;
		return activeGroup;
	}

	protected string BuildGroupId(HST_CampaignState state, HST_ZoneState zone, string factionKey, bool qrf)
	{
		string prefix = "grp";
		if (qrf)
			prefix = "qrf";

		return string.Format("%1_%2_%3_%4_%5", prefix, zone.m_sZoneId, factionKey, state.m_iElapsedSeconds, state.m_aActiveGroups.Count());
	}

	protected string SelectTrainedResistanceGroupPrefab(HST_CampaignState state, HST_ZoneState zone, HST_FactionTemplate faction, int seed)
	{
		if (!state || !faction)
			return "";

		array<string> candidates = {};
		int training = Math.Max(1, state.m_iTrainingLevel);

		if (training <= 2)
		{
			AppendUniqueGroupPrefabs(candidates, faction.m_aPatrolGroupPrefabs);
			AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
		}
		else if (training <= 5)
		{
			AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
			AppendUniqueGroupPrefabs(candidates, faction.m_aPatrolGroupPrefabs);
		}
		else
		{
			AppendUniqueGroupPrefabs(candidates, faction.m_aQRFGroupPrefabs);
			AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
			AppendUniqueGroupPrefabs(candidates, faction.m_aPatrolGroupPrefabs);
		}

		return SelectValidGroupPrefabFromList(candidates, seed, faction.m_sFactionKey, "trained FIA garrison");
	}

	protected string SelectGroupPrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, bool qrf, HST_CampaignPreset preset = null)
	{
		string role = "garrison";
		if (qrf)
			role = "qrf";
		HST_FactionRuntimeSpawnSpec spec = HST_DefaultCatalog.ResolveRuntimeSpawnSpec(preset, factionKey, role, "physical war group selection");
		if (!spec)
			return "";

		int seed = BuildGroupSelectionSeed(state, zone, qrf);
		array<string> candidates = {};
		if (qrf)
		{
			AppendUniqueGroupPrefabs(candidates, spec.m_aGroupPrefabs);
			return SelectValidGroupPrefabFromList(candidates, seed, factionKey, "qrf");
		}

		if (factionKey == "FIA")
		{
			HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
			string trainedPrefab = SelectTrainedResistanceGroupPrefab(state, zone, faction, seed);
			if (!trainedPrefab.IsEmpty())
				return trainedPrefab;
		}

		AppendUniqueGroupPrefabs(candidates, spec.m_aGroupPrefabs);
		return SelectValidGroupPrefabFromList(candidates, seed, factionKey, "garrison");
	}

	protected string SelectVehiclePrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, int index, HST_CampaignPreset preset = null)
	{
		HST_FactionRuntimeSpawnSpec spec = HST_DefaultCatalog.ResolveRuntimeSpawnSpec(preset, factionKey, "vehicle", "physical war vehicle selection");
		if (!spec || spec.m_aVehiclePrefabs.Count() == 0)
			return "";

		int seed = BuildGroupSelectionSeed(state, zone, false) + index * 47;
		return SelectValidVehiclePrefabFromList(spec.m_aVehiclePrefabs, seed, factionKey, "active vehicle");
	}

	protected string SelectMissionConvoyVehiclePrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, HST_ActiveMissionState mission, int index, HST_CampaignPreset preset = null)
	{
		array<string> candidates = {};
		BuildConvoyVehicleCandidates(candidates, factionKey, mission, preset);
		array<string> usableCandidates = {};
		BuildUsableConvoyVehicleCandidates(usableCandidates, candidates, factionKey);
		if (usableCandidates.Count() == 0)
			return "";

		int seed = BuildGroupSelectionSeed(state, zone, false) + index * 53 + ResolveMissionVehicleSelectionSalt(mission);
		return usableCandidates[HST_DefaultCatalog.PositiveMod(seed, usableCandidates.Count())];
	}

	protected void BuildConvoyVehicleCandidates(array<string> candidates, string factionKey, HST_ActiveMissionState mission, HST_CampaignPreset preset = null)
	{
		if (!candidates)
			return;

		HST_FactionRuntimeSpawnSpec spec = HST_DefaultCatalog.ResolveRuntimeSpawnSpec(preset, factionKey, "convoy_vehicle", "mission convoy vehicle selection");
		if (spec)
			AppendUniqueVehiclePrefabs(candidates, spec.m_aVehiclePrefabs);
		AppendRuntimeFactionCatalogVehiclePrefabs(candidates, factionKey);

		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (faction)
			AppendUniqueVehiclePrefabs(candidates, faction.m_aVehiclePrefabs);
		AppendFallbackConvoyVehiclePrefabs(candidates, factionKey, mission);
	}

	protected string BuildGroundVehicleCandidateFactionReport(string factionKey)
	{
		array<string> candidates = {};
		BuildConvoyVehicleCandidates(candidates, factionKey, null);

		array<string> usableCandidates = {};
		BuildUsableConvoyVehicleCandidates(usableCandidates, candidates, factionKey);
		string usableList = "";
		foreach (string prefab : usableCandidates)
		{
			if (usableList.IsEmpty())
				usableList = prefab;
			else
				usableList = usableList + ", " + prefab;
		}

		if (usableList.IsEmpty())
			usableList = "none";

		int catalogCandidates = CountRuntimeFactionCatalogVehicleCandidates(factionKey);
		string catalogSource = BuildFactionCampaignCatalogSource(factionKey);
		if (catalogCandidates <= 0)
			catalogSource = catalogSource + " unavailable; local GUID fallback active";
		string report = string.Format("\n  faction %1 convoy ground vehicle candidates | usable %2/%3", factionKey, usableCandidates.Count(), candidates.Count());
		report = report + string.Format(" | catalog source %1 | catalog candidates %2", catalogSource, catalogCandidates);
		report = report + string.Format(" | unverified path-only %1", CountUnverifiedVehicleCandidates(candidates));
		report = report + " | usable prefabs " + usableList;
		return report;
	}

	protected void BuildUsableConvoyVehicleCandidates(array<string> usableCandidates, array<string> candidates, string factionKey)
	{
		if (!usableCandidates || !candidates)
			return;

		foreach (string prefab : candidates)
		{
			if (!IsValidVehiclePrefabResource(prefab, factionKey, false))
				continue;
			if (usableCandidates.Contains(prefab))
				continue;

			usableCandidates.Insert(prefab);
		}
	}

	protected int AppendRuntimeFactionCatalogVehiclePrefabs(array<string> candidates, string factionKey)
	{
		if (!candidates || factionKey.IsEmpty())
			return 0;

		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalogManager)
			return 0;

		array<SCR_EntityCatalog> catalogs = {};
		FactionKey key = factionKey;
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

				string prefab = entry.GetPrefab();
				if (!IsGroundVehicleResource(prefab))
					continue;

				AppendUniqueVehiclePrefab(candidates, prefab);
			}
		}

		return candidates.Count() - before;
	}

	protected int CountRuntimeFactionCatalogVehicleCandidates(string factionKey)
	{
		array<string> candidates = {};
		return AppendRuntimeFactionCatalogVehiclePrefabs(candidates, factionKey);
	}

	protected string BuildFactionCampaignCatalogSource(string factionKey)
	{
		string source = "runtime SCR_EntityCatalog";
		if (factionKey == "US")
			source = source + " via " + US_CAMPAIGN_FACTION_CONFIG;
		else if (factionKey == "USSR")
			source = source + " via " + USSR_CAMPAIGN_FACTION_CONFIG;
		else if (factionKey == "FIA")
			source = source + " via " + FIA_CAMPAIGN_FACTION_CONFIG;

		return source;
	}

	protected void AppendFallbackConvoyVehiclePrefabs(array<string> candidates, string factionKey, HST_ActiveMissionState mission)
	{
		if (!candidates)
			return;

		if (factionKey == "US")
		{
			AppendUniqueVehiclePrefab(candidates, "{4A71F755A4513227}Prefabs/Vehicles/Wheeled/M998/M1025.et");
			AppendUniqueVehiclePrefab(candidates, "{B55C6990A6A9411B}Prefabs/Vehicles/Wheeled/M998/M998_covered.et");
			AppendUniqueVehiclePrefab(candidates, "{5674FAEB9AB7BDD0}Prefabs/Vehicles/Wheeled/M998/M998_uncovered.et");
			AppendUniqueVehiclePrefab(candidates, "{F1FBD0972FA5FE09}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport.et");
			AppendUniqueVehiclePrefab(candidates, "{81FDAD5EB644CC3D}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport_covered.et");
			return;
		}

		if (factionKey == "USSR")
		{
			AppendUniqueVehiclePrefab(candidates, "{0B4DEA8078B78A9B}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_PKM.et");
			AppendUniqueVehiclePrefab(candidates, "{4597626AF36C0858}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320.et");
			AppendUniqueVehiclePrefab(candidates, "{16C1F16C9B053801}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport.et");
			AppendUniqueVehiclePrefab(candidates, "{C012BB3488BEA0C2}Prefabs/Vehicles/Wheeled/BTR70/BTR70.et");
			return;
		}

		if (factionKey == "FIA")
		{
			AppendUniqueVehiclePrefab(candidates, "{16C1F16C9B053801}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport.et");
			AppendUniqueVehiclePrefab(candidates, "{B47110AA1A806556}Prefabs/Vehicles/Wheeled/BTR70/BTR70_FIA.et");
			AppendUniqueVehiclePrefab(candidates, "{0B4DEA8078B78A9B}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_PKM.et");
		}
	}

	protected int ResolveMissionVehicleSelectionSalt(HST_ActiveMissionState mission)
	{
		if (!mission)
			return 0;

		int salt = mission.m_sInstanceId.Length() * 89 + mission.m_sMissionId.Length() * 131;
		if (mission.m_sMissionId == "convoy_armored")
			salt += 7001;
		if (mission.m_sMissionId == "convoy_reinforcements")
			salt += 3001;
		return salt;
	}

	protected string SelectValidVehiclePrefabFromList(array<string> prefabs, int seed, string factionKey, string purpose)
	{
		if (!prefabs || prefabs.Count() == 0)
			return "";

		int startIndex = HST_DefaultCatalog.PositiveMod(seed, prefabs.Count());
		for (int offset = 0; offset < prefabs.Count(); offset++)
		{
			int index = HST_DefaultCatalog.PositiveMod(startIndex + offset, prefabs.Count());
			string prefab = prefabs[index];
			if (!IsValidVehiclePrefabResource(prefab, factionKey, false))
				continue;

			return prefab;
		}

		Print(string.Format("h-istasi | no valid %1 prefab found for faction %2", purpose, factionKey), LogLevel.WARNING);
		return "";
	}

	protected void AppendUniqueGroupPrefabs(array<string> candidates, array<string> source)
	{
		if (!candidates || !source)
			return;

		foreach (string prefab : source)
		{
			if (prefab.IsEmpty() || candidates.Contains(prefab))
				continue;

			candidates.Insert(prefab);
		}
	}

	protected void AppendUniqueVehiclePrefabs(array<string> candidates, array<string> source)
	{
		if (!candidates || !source)
			return;

		foreach (string prefab : source)
		{
			AppendUniqueVehiclePrefab(candidates, prefab);
		}
	}

	protected void AppendUniqueVehiclePrefab(array<string> candidates, string prefab)
	{
		if (!candidates || prefab.IsEmpty() || candidates.Contains(prefab))
			return;
		if (IsAircraftVehicleResource(prefab))
			return;
		if (!IsGroundVehicleResource(prefab))
			return;

		candidates.Insert(prefab);
	}

	protected int CountUnverifiedVehicleCandidates(array<string> candidates)
	{
		if (!candidates)
			return 0;

		int count;
		foreach (string prefab : candidates)
		{
			if (!IsGuidQualifiedVehicleResource(prefab))
				count++;
		}

		return count;
	}

	protected int BuildGroupSelectionSeed(HST_CampaignState state, HST_ZoneState zone, bool qrf)
	{
		int seed = 271;
		if (state)
			seed += state.m_iCampaignSeed * 19 + state.m_iElapsedSeconds * 3 + state.m_aActiveGroups.Count() * 97;
		if (zone)
			seed += zone.m_iPriority * 41 + zone.m_sZoneId.Length() * 113 + Math.Round(zone.m_vPosition[0]) + Math.Round(zone.m_vPosition[2]);
		if (qrf)
			seed += 7001;
		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected bool TrySpawnActiveGroup(HST_ActiveGroupState activeGroup, HST_CampaignState state = null, HST_CampaignPreset preset = null)
	{
		if (!activeGroup || IsTerminalActiveGroupRuntimeStatus(activeGroup) || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		if (activeGroup.m_bSpawnAttempted && !activeGroup.m_bSpawnedEntity)
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
		{
			activeGroup.m_sSpawnFailureReason = "Respawn system is unavailable.";
			return false;
		}

		string aiWorldBudgetFailure;
		if (!EnsureActiveGroupAIWorldBudget(activeGroup, "pre-spawn native group", aiWorldBudgetFailure))
		{
			MarkActiveGroupAIWorldBudgetDeferred(activeGroup, state, aiWorldBudgetFailure, "pre_spawn_budget");
			return false;
		}

		activeGroup.m_bSpawnAttempted = true;
		string requestedStatus = activeGroup.m_sRuntimeStatus;
		activeGroup.m_sRuntimeStatus = "spawning";
		activeGroup.m_sSpawnFallbackMode = ResolveActiveGroupPrimarySpawnMode(activeGroup.m_sSpawnFallbackMode, ACTIVE_GROUP_SPAWN_MODE_GROUP);
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = 0;
		PrintActiveGroupSpawnEvidence(state, activeGroup, "request");

		vector spawnPosition = HST_WorldPositionService.ResolveSafeGroundPosition(activeGroup.m_vPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		activeGroup.m_vPosition = spawnPosition;

		GenericEntity entity;
		int agentCount = 0;
		string failureReason = "";
		if (activeGroup.m_sPrefab.IsEmpty())
		{
			failureReason = string.Format("No group prefab configured for faction %1.", activeGroup.m_sFactionKey);
		}
		else if (IsValidGroupPrefabResource(activeGroup.m_sPrefab, activeGroup.m_sFactionKey))
		{
			entity = SpawnControlledNativeActiveGroupPrefab(activeGroup.m_sPrefab, spawnPosition, activeGroup, failureReason);
			if (!entity)
			{
				if (failureReason.IsEmpty())
					failureReason = string.Format("Group prefab did not spawn: %1.", activeGroup.m_sPrefab);
			}
			else
			{
				agentCount = ResolveSpawnedAgentCount(entity, activeGroup);
			}
		}
		else
		{
			failureReason = string.Format("Invalid group prefab: %1.", activeGroup.m_sPrefab);
		}

		if (!entity)
		{
			activeGroup.m_sSpawnFailureReason = failureReason;
			if (activeGroup.m_sSpawnFailureReason.IsEmpty())
				activeGroup.m_sSpawnFailureReason = string.Format("Group prefab spawn failed for faction %1.", activeGroup.m_sFactionKey);
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			Print(string.Format("h-istasi | active group prefab spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			PrintActiveGroupSpawnEvidence(state, activeGroup, "failed");
			return false;
		}

		ApplyCampaignDebugEntityName(entity, "active_group", activeGroup.m_sGroupId);
		if (agentCount <= 0)
		{
			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_sRuntimeEntityId = "";
			activeGroup.m_sRuntimeStatus = "spawn_pending_agents";
			activeGroup.m_sSpawnFailureReason = "AIGroup spawned but is awaiting agent population.";
			activeGroup.m_iSpawnedAgentCount = 0;
			activeGroup.m_iLastSeenAliveCount = 0;
			activeGroup.m_iSurvivorInfantryCount = 0;
			activeGroup.m_iSurvivorVehicleCount = 0;
			if (state)
				activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
			m_aRuntimeGroupIds.Insert(activeGroup.m_sGroupId);
			m_aRuntimeGroupEntities.Insert(entity);
			RegisterPendingActiveGroupPopulation(entity, activeGroup, requestedStatus, state);
			PrintActiveGroupSpawnEvidence(state, activeGroup, "pending_agents");
			GetGame().GetCallqueue().CallLater(ConfirmSpawnedGroupAgents, ACTIVE_GROUP_AGENT_POPULATION_RETRY_MS, false, activeGroup, requestedStatus, state, 1);
			DebugLog(string.Format("active group pending agent population %1 prefab %2", activeGroup.m_sGroupId, activeGroup.m_sPrefab));
			return true;
		}

		activeGroup.m_bSpawnedEntity = true;
		ApplyRuntimeGroupFaction(entity, activeGroup, "native immediate populated", true);
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = ResolveSpawnedRuntimeStatus(activeGroup, requestedStatus);
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = agentCount;
		activeGroup.m_iLastSeenAliveCount = Math.Max(0, agentCount);
		activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, agentCount);
		if (IsMissionConvoyGroup(activeGroup))
			RecordConvoyCrewObservedAlive(activeGroup, agentCount);
		if (state)
			activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		m_aRuntimeGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeGroupEntities.Insert(entity);
		ReconcileRuntimeGroupEditableMembership(SCR_AIGroup.Cast(entity), activeGroup, "native immediate populated");
		TrySpawnActiveGroupAttachedVehicle(state, preset, activeGroup, "native immediate populated");
		PrintActiveGroupSpawnEvidence(state, activeGroup, "spawned");
		DebugLog(string.Format("spawned active group %1 using %2 (%3 agents)", activeGroup.m_sGroupId, activeGroup.m_sSpawnFallbackMode, agentCount));
		return true;
	}

	protected bool TrySpawnActiveGroupAttachedVehicle(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveGroupState activeGroup, string source)
	{
		if (!activeGroup || activeGroup.m_iVehicleCount <= 0 || activeGroup.m_iInfantryCount <= 0 || IsMissionConvoyGroup(activeGroup))
			return false;
		if (GetRuntimeVehicleEntity(activeGroup.m_sGroupId))
			return true;

		HST_ZoneState zone;
		if (state && !activeGroup.m_sZoneId.IsEmpty())
			zone = state.FindZone(activeGroup.m_sZoneId);

		string vehiclePrefab = activeGroup.m_sVehiclePrefab;
		if (vehiclePrefab.IsEmpty())
			vehiclePrefab = SelectVehiclePrefab(state, zone, activeGroup.m_sFactionKey, activeGroup.m_sGroupId.Length() + activeGroup.m_iSpawnedAtSecond, preset);
		if (vehiclePrefab.IsEmpty() || !IsValidVehiclePrefabResource(vehiclePrefab, activeGroup.m_sFactionKey))
		{
			activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "vehicle_attach_missing");
			AppendActiveGroupSpawnFailureNote(activeGroup, "Attached vehicle prefab unavailable for mixed active group.");
			return false;
		}

		vector vehicleAnchor = activeGroup.m_vPosition;
		vehicleAnchor[0] = vehicleAnchor[0] + 8.0;
		vehicleAnchor[2] = vehicleAnchor[2] + 6.0;
		vector spawnPosition;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(vehicleAnchor, spawnPosition, true) && !HST_WorldPositionService.TryResolveVehicleSpawnPosition(activeGroup.m_vPosition, spawnPosition, true))
		{
			activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "vehicle_attach_blocked");
			AppendActiveGroupSpawnFailureNote(activeGroup, "Attached vehicle spawn position unavailable for mixed active group.");
			return false;
		}

		vector spawnAngles = HST_WorldPositionService.BuildUprightAngles(0);
		GenericEntity vehicleEntity = HST_WorldPositionService.SpawnPrefab(vehiclePrefab, spawnPosition, spawnAngles);
		if (!vehicleEntity)
		{
			activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "vehicle_attach_failed");
			AppendActiveGroupSpawnFailureNote(activeGroup, "Attached vehicle prefab spawn failed for mixed active group.");
			return false;
		}

		HST_WorldPositionService.ApplyUprightEntityTransform(vehicleEntity, spawnPosition, spawnAngles);
		HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(vehicleEntity);
		ApplyCampaignDebugEntityName(vehicleEntity, "active_group_vehicle", activeGroup.m_sGroupId);
		m_aRuntimeVehicleGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeVehicleEntities.Insert(vehicleEntity);
		activeGroup.m_sVehiclePrefab = vehiclePrefab;
		activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "vehicle_attached");
		activeGroup.m_iSurvivorVehicleCount = Math.Max(1, activeGroup.m_iVehicleCount);
		activeGroup.m_iLastSeenAliveCount = Math.Max(activeGroup.m_iLastSeenAliveCount, activeGroup.m_iSpawnedAgentCount + activeGroup.m_iVehicleCount);
		DebugLog(string.Format("spawned attached active group vehicle %1 via %2 | prefab %3 | source %4", activeGroup.m_sGroupId, activeGroup.m_sSpawnFallbackMode, vehiclePrefab, source));
		return true;
	}

	protected void PrintActiveGroupSpawnEvidence(HST_CampaignState state, HST_ActiveGroupState activeGroup, string stage)
	{
		if (!activeGroup)
			return;

		HST_ZoneState zone;
		if (state && !activeGroup.m_sZoneId.IsEmpty())
			zone = state.FindZone(activeGroup.m_sZoneId);

		string zoneOwner = "missing";
		bool zoneActive;
		int zoneActiveInfantry;
		int zoneActiveVehicles;
		if (zone)
		{
			zoneOwner = zone.m_sOwnerFactionKey;
			zoneActive = zone.m_bActive;
			zoneActiveInfantry = zone.m_iActiveInfantryCount;
			zoneActiveVehicles = zone.m_iActiveVehicleCount;
		}

		bool catalogMatch = IsGroupPrefabCatalogFactionMatch(activeGroup.m_sPrefab, activeGroup.m_sFactionKey);
		bool resourceLoaded;
		if (!activeGroup.m_sPrefab.IsEmpty())
		{
			Resource loaded = Resource.Load(activeGroup.m_sPrefab);
			if (loaded && loaded.IsValid())
				resourceLoaded = true;
		}

		string evidence = string.Format("h-istasi | active group runtime proof | stage %1 | group %2 | zone %3 owner %4 active %5 | expected %6 | prefab %7",
			ReportText(stage),
			ReportText(activeGroup.m_sGroupId),
			ReportText(activeGroup.m_sZoneId),
			ReportText(zoneOwner),
			ReportBool(zoneActive),
			ReportText(activeGroup.m_sFactionKey),
			ReportText(activeGroup.m_sPrefab));
		evidence = evidence + string.Format(" | catalog %1 resource %2 | status %3 mode %4 | agents %5/%6 | zone counts %7/%8 | visual %9",
			ReportBool(catalogMatch),
			ReportBool(resourceLoaded),
			ReportText(activeGroup.m_sRuntimeStatus),
			ReportText(activeGroup.m_sSpawnFallbackMode),
			activeGroup.m_iSpawnedAgentCount,
			activeGroup.m_iLastSeenAliveCount,
			zoneActiveInfantry,
			zoneActiveVehicles,
			ReportText(BuildActiveGroupRuntimeVisualEvidence(activeGroup.m_sGroupId)));
		evidence = evidence + " | " + BuildAIWorldBudgetDebug();
		Print(evidence);
	}

	protected GenericEntity SpawnControlledNativeActiveGroupPrefab(string prefab, vector position, HST_ActiveGroupState activeGroup, out string failureReason)
	{
		failureReason = "";
		if (prefab.IsEmpty())
		{
			failureReason = "Group prefab path is empty.";
			return null;
		}

		ResourceName resourceName = prefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded || !loaded.IsValid())
		{
			failureReason = "Group prefab resource did not load: " + prefab;
			return null;
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			failureReason = "World is unavailable for group prefab spawn.";
			return null;
		}

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = position;

		IEntity spawnedEntity = GetGame().SpawnEntityPrefabEx(resourceName, false, world, params);
		GenericEntity entity = GenericEntity.Cast(spawnedEntity);
		if (!entity)
		{
			if (spawnedEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(spawnedEntity);
			failureReason = "Group prefab did not create a GenericEntity: " + prefab;
			return null;
		}

		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if (!group)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			failureReason = "Group prefab did not create an SCR_AIGroup: " + prefab;
			return null;
		}

		vector zeroAngles = HST_WorldPositionService.BuildUprightAngles(0);
		entity.SetOrigin(position);
		entity.SetAngles(HST_WorldPositionService.BuildEntitySetAnglesFromYawVector(zeroAngles));
		StabilizeRuntimeAIGroupRoot(entity, activeGroup, "controlled group prefab spawn");
		string actualGroupFaction;
		if (!IsRuntimeGroupRootFactionExpected(entity, activeGroup, actualGroupFaction))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			failureReason = string.Format("Group prefab root faction mismatch before native member spawn: expected %1 actual %2 prefab %3.", activeGroup.m_sFactionKey, ReportText(actualGroupFaction), prefab);
			Print(string.Format("h-istasi | active group root faction mismatch %1 expected %2 actual %3 prefab %4", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, ReportText(actualGroupFaction), prefab), LogLevel.WARNING);
			return null;
		}
		if (activeGroup)
			DebugLog(string.Format("active group primary stock root faction verified %1 expected %2 prefab %3 | %4", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, prefab, BuildNativeGroupPopulationDebug(group)));
		if (activeGroup && !EnsureActiveGroupAIWorldBudget(activeGroup, "controlled group prefab SpawnUnits", failureReason))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			return null;
		}
		if (!group.GetSpawnImmediately())
			group.SpawnUnits();
		if (activeGroup)
			DebugLog(string.Format("active group requested controlled native member spawn %1 expected infantry %2 prefab %3 | %4", activeGroup.m_sGroupId, activeGroup.m_iInfantryCount, prefab, BuildNativeGroupPopulationDebug(group)));
		if (activeGroup && CountLivingNativeAIGroupAgents(group) <= 0 && group.GetSpawnQueueSize() <= 0)
		{
			int slotSpawned = SpawnNativeSlotMembersIntoRuntimeGroup(group, activeGroup, "controlled group prefab empty native queue");
			if (slotSpawned > 0)
				activeGroup.m_sSpawnFallbackMode = ACTIVE_GROUP_SPAWN_MODE_GROUP_SLOT_PRIMARY;
		}
		return entity;
	}

	protected void StabilizeRuntimeAIGroupRoot(IEntity entity, HST_ActiveGroupState activeGroup, string source)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if (!group)
			return;

		group.SetDeleteWhenEmpty(false);
		group.SetMemberSpawnDelay(0);
		if (activeGroup && activeGroup.m_iInfantryCount > 0)
			group.SetMaxUnitsToSpawn(activeGroup.m_iInfantryCount);
		if (activeGroup)
			DebugLog(string.Format("active group stabilized native AIGroup root %1 deleteWhenEmpty false expected infantry %2 via %3", activeGroup.m_sGroupId, activeGroup.m_iInfantryCount, source));
	}

	protected void ConfirmSpawnedGroupAgents(HST_ActiveGroupState activeGroup, string requestedStatus, HST_CampaignState state, int attempt)
	{
		if (!activeGroup || activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus != "spawn_pending_agents")
		{
			ClearPendingActiveGroupPopulation(activeGroup);
			return;
		}

		if (TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, "retry"))
			return;
		string aiWorldBudgetFailure;
		if (!EnsureActiveGroupAIWorldBudget(activeGroup, string.Format("pending population retry %1", attempt), aiWorldBudgetFailure))
		{
			MarkActiveGroupAIWorldBudgetDeferred(activeGroup, state, aiWorldBudgetFailure, "pending_budget");
			return;
		}

		bool forceFallback = attempt >= ACTIVE_GROUP_AGENT_POPULATION_FORCE_FALLBACK_ATTEMPT;
		if (forceFallback && TryKickPendingNativeGroupSpawn(activeGroup, "retry"))
		{
			GetGame().GetCallqueue().CallLater(ConfirmSpawnedGroupAgents, ACTIVE_GROUP_AGENT_POPULATION_RETRY_MS, false, activeGroup, requestedStatus, state, attempt + 1);
			return;
		}

		bool forceSlotPrimary = attempt >= ACTIVE_GROUP_AGENT_POPULATION_SLOT_PRIMARY_ATTEMPT;
		if (forceSlotPrimary && TryFlushPendingNativeGroupSpawnImmediately(activeGroup, requestedStatus, state, "retry"))
			return;
		if (forceSlotPrimary && TryPopulatePendingActiveGroupFromNativeSlots(activeGroup, requestedStatus, state, "retry"))
			return;

		bool forceDirectFallback = attempt >= ACTIVE_GROUP_AGENT_POPULATION_DIRECT_FALLBACK_ATTEMPT;
		if (forceDirectFallback && IsActiveGroupNativeDelayedPopulationActive(activeGroup))
			DebugLog(string.Format("active group forcing direct infantry fallback while native delayed population remains active %1 attempt %2/%3 | %4", activeGroup.m_sGroupId, attempt, ACTIVE_GROUP_AGENT_POPULATION_MAX_ATTEMPTS, BuildActiveGroupRuntimeVisualEvidence(activeGroup.m_sGroupId)));
		if (forceDirectFallback && TryPopulatePendingActiveGroupFromFactionInfantry(activeGroup, requestedStatus, state, "retry", true))
			return;

		if (attempt < ACTIVE_GROUP_AGENT_POPULATION_MAX_ATTEMPTS)
		{
			if (forceDirectFallback)
				activeGroup.m_sSpawnFailureReason = string.Format("AIGroup stock member-slot population or direct faction infantry fallback attempted but still has zero agents (%1/%2).", attempt, ACTIVE_GROUP_AGENT_POPULATION_MAX_ATTEMPTS);
			else
				activeGroup.m_sSpawnFailureReason = string.Format("AIGroup spawned but is still awaiting agent population (%1/%2).", attempt, ACTIVE_GROUP_AGENT_POPULATION_MAX_ATTEMPTS);

			GetGame().GetCallqueue().CallLater(ConfirmSpawnedGroupAgents, ACTIVE_GROUP_AGENT_POPULATION_RETRY_MS, false, activeGroup, requestedStatus, state, attempt + 1);
			DebugLog(string.Format("active group still pending agent population %1 attempt %2/%3 prefab %4", activeGroup.m_sGroupId, attempt, ACTIVE_GROUP_AGENT_POPULATION_MAX_ATTEMPTS, activeGroup.m_sPrefab));
			return;
		}

		string previousFailureReason = activeGroup.m_sSpawnFailureReason;
		ClearPendingActiveGroupPopulation(activeGroup);
		DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_sRuntimeEntityId = "";
		activeGroup.m_sRuntimeStatus = "spawn_failed";
		activeGroup.m_sSpawnFailureReason = BuildFinalActiveGroupPopulationFailureReason(previousFailureReason);
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_iSurvivorVehicleCount = 0;
		if (IsMissionConvoyGroup(activeGroup))
		{
			activeGroup.m_bCrewPopulationTerminallyFailed = true;
			activeGroup.m_sCrewPopulationFailureReason = activeGroup.m_sSpawnFailureReason;
			activeGroup.m_sConvoyRuntimeStage = "FAILED";
		}
		RefreshActiveGroupZoneCounts(state, activeGroup);
		Print(string.Format("h-istasi | active group failed %1 prefab %2: zero agents after grace | reason %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
	}

	protected string BuildFinalActiveGroupPopulationFailureReason(string previousFailureReason)
	{
		string finalReason = string.Format("AIGroup prefab produced zero agents after %1 population attempts.", ACTIVE_GROUP_AGENT_POPULATION_MAX_ATTEMPTS);
		if (previousFailureReason.IsEmpty() || previousFailureReason == finalReason)
			return finalReason;

		return previousFailureReason + " Final zero-agent failure: " + finalReason;
	}

	protected bool TryFinalizeSpawnedGroupAgents(HST_ActiveGroupState activeGroup, string requestedStatus, HST_CampaignState state, string source)
	{
		if (!activeGroup || activeGroup.m_sRuntimeStatus != "spawn_pending_agents")
			return false;

		int agentCount = CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId);
		if (agentCount <= 0)
			return false;

		IEntity groupEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		ApplyRuntimeGroupFaction(groupEntity, activeGroup, source, true);
		ReconcileRuntimeGroupEditableMembership(SCR_AIGroup.Cast(groupEntity), activeGroup, source);
		ClearPendingActiveGroupPopulation(activeGroup);
		activeGroup.m_bSpawnedEntity = true;
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = ResolveSpawnedRuntimeStatus(activeGroup, requestedStatus);
		if (!IsDirectInfantryFallbackMode(activeGroup.m_sSpawnFallbackMode) && !IsActiveGroupSpawnMode(activeGroup.m_sSpawnFallbackMode, ACTIVE_GROUP_SPAWN_MODE_GROUP_SLOT_PRIMARY) && !ShouldPreserveActiveGroupSemanticSpawnMode(activeGroup.m_sSpawnFallbackMode))
			activeGroup.m_sSpawnFallbackMode = ACTIVE_GROUP_SPAWN_MODE_GROUP;
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = agentCount;
		activeGroup.m_iLastSeenAliveCount = agentCount;
		activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, agentCount);
		if (IsMissionConvoyGroup(activeGroup))
			RecordConvoyCrewObservedAlive(activeGroup, agentCount);
		if (state)
			activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		RefreshActiveGroupZoneCounts(state, activeGroup);
		TryBindPopulatedMissionConvoyGroup(state, activeGroup);
		TrySpawnActiveGroupAttachedVehicle(state, null, activeGroup, source);
		PrintActiveGroupSpawnEvidence(state, activeGroup, source + "_populated");
		DebugLog(string.Format("active group populated %1 live agents %2 expected %3 via %4", activeGroup.m_sGroupId, agentCount, activeGroup.m_iInfantryCount, source));
		return true;
	}

	protected void RegisterPendingActiveGroupPopulation(IEntity entity, HST_ActiveGroupState activeGroup, string requestedStatus, HST_CampaignState state)
	{
		if (!entity || !activeGroup)
			return;

		ClearPendingActiveGroupPopulation(activeGroup);
		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if (!group)
			return;

		StabilizeRuntimeAIGroupRoot(entity, activeGroup, "pending population registration");
		group.GetOnAllDelayedEntitySpawned().Remove(OnDelayedActiveGroupMembersSpawned);
		group.GetOnAllDelayedEntitySpawned().Insert(OnDelayedActiveGroupMembersSpawned);
		m_aPendingPopulationGroupIds.Insert(activeGroup.m_sGroupId);
		m_aPendingPopulationRequestedStatuses.Insert(requestedStatus);
		m_aPendingPopulationActiveGroups.Insert(activeGroup);
		m_aPendingPopulationStates.Insert(state);
	}

	protected void OnDelayedActiveGroupMembersSpawned(SCR_AIGroup group)
	{
		if (!group)
			return;

		string groupId = ResolveRuntimeGroupIdForEntity(group);
		int index = FindPendingActiveGroupPopulationIndex(groupId);
		if (index < 0)
			return;

		HST_ActiveGroupState activeGroup = m_aPendingPopulationActiveGroups[index];
		string requestedStatus = m_aPendingPopulationRequestedStatuses[index];
		HST_CampaignState state = m_aPendingPopulationStates[index];
		if (!TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, "native delayed spawn event"))
		{
			DebugLog(string.Format("active group delayed spawn event fired before durable live agents were countable %1", groupId));
		}
	}

	bool CampaignDebugResolvePendingActiveGroupPopulation(HST_ActiveGroupState activeGroup, HST_CampaignState state, string requestedStatus, out string evidence)
	{
		evidence = "missing group";
		if (!activeGroup)
			return false;

		string beforeStatus = activeGroup.m_sRuntimeStatus;
		int beforeAgents = activeGroup.m_iSpawnedAgentCount;
		bool wasPending = activeGroup.m_sRuntimeStatus == "spawn_pending_agents";
		bool finalized = false;
		bool kickedNativeSpawn = false;
		bool flushedNativeImmediate = false;
		bool populatedSlotPrimary = false;
		bool populatedDirectFallback = false;
		bool nativeDelayedActiveBeforeDirectFallback = false;

		if (requestedStatus.IsEmpty())
			requestedStatus = activeGroup.m_sRuntimeStatus;
		if (requestedStatus == "spawn_pending_agents")
			requestedStatus = "active";

		if (wasPending)
		{
			finalized = TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, "campaign debug pre-route");
			if (!finalized)
				kickedNativeSpawn = TryKickPendingNativeGroupSpawn(activeGroup, "campaign debug pre-route");
			if (!finalized && kickedNativeSpawn)
				finalized = TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, "campaign debug pre-route native retry");
			if (!finalized)
			{
				flushedNativeImmediate = TryFlushPendingNativeGroupSpawnImmediately(activeGroup, requestedStatus, state, "campaign debug pre-route");
				if (flushedNativeImmediate)
					finalized = true;
			}
			if (!finalized)
				populatedSlotPrimary = TryPopulatePendingActiveGroupFromNativeSlots(activeGroup, requestedStatus, state, "campaign debug pre-route");
			if (!finalized && populatedSlotPrimary)
				finalized = true;
			if (!finalized)
			{
				nativeDelayedActiveBeforeDirectFallback = IsActiveGroupNativeDelayedPopulationActive(activeGroup);
				activeGroup.m_sSpawnFailureReason = "Campaign debug pre-route proof left group pending because primary native/stock-slot population has not completed; direct faction-infantry fallback is not certification proof.";
				DebugLog(string.Format("active group campaign debug pre-route kept primary population pending %1 | nativeDelayed %2 | status %3", activeGroup.m_sGroupId, nativeDelayedActiveBeforeDirectFallback, activeGroup.m_sRuntimeStatus));
			}
		}

		int liveAfter = CountAliveRuntimeGroupAgents(activeGroup.m_sGroupId);
		bool resolved = activeGroup.m_sRuntimeStatus != "spawn_pending_agents" && liveAfter > 0;
		evidence = string.Format("pending %1 | finalized %2 | nativeRetry %3 | nativeImmediate %4 | slotPrimary %5 | directFallback %6 | nativeDelayedBeforeDirect %7",
			wasPending,
			finalized,
			kickedNativeSpawn,
			flushedNativeImmediate,
			populatedSlotPrimary,
			populatedDirectFallback,
			nativeDelayedActiveBeforeDirectFallback);
		evidence = evidence + string.Format(" | status %1 -> %2 | agents %3",
			ReportText(beforeStatus),
			ReportText(activeGroup.m_sRuntimeStatus),
			beforeAgents);
		evidence = evidence + string.Format(" -> %1 | live %2", activeGroup.m_iSpawnedAgentCount, liveAfter);
		evidence = evidence + string.Format(" | lastAlive %1 | reason %2", activeGroup.m_iLastSeenAliveCount, ReportText(activeGroup.m_sSpawnFailureReason));
		return resolved;
	}

	bool CampaignDebugResolveActiveGroupRouteAssignment(HST_ActiveGroupState activeGroup, HST_CampaignState state, HST_CampaignPreset preset, string requestedStatus, out string evidence)
	{
		evidence = "missing group or state";
		if (!activeGroup || !state)
			return false;

		string beforeStatus = activeGroup.m_sRuntimeStatus;
		string beforeMode = activeGroup.m_sSpawnFallbackMode;
		int beforeWaypoints = activeGroup.m_iAssignedWaypointCount;
		string populationEvidence = "not pending";
		bool populationResolved = activeGroup.m_sRuntimeStatus != "spawn_pending_agents";
		bool directFallbackResolved;
		bool routeUpdateChanged;
		bool manualRouteAssigned;
		bool assignedFinalSweepWaypoint;
		int manualAssignedWaypoints;

		if (!populationResolved)
		{
			populationResolved = CampaignDebugResolvePendingActiveGroupPopulation(activeGroup, state, requestedStatus, populationEvidence);
			if (!populationResolved && activeGroup.m_sRuntimeStatus == "spawn_pending_agents")
			{
				directFallbackResolved = TryPopulatePendingActiveGroupFromFactionInfantry(activeGroup, requestedStatus, state, "campaign debug route assignment", true);
				populationResolved = activeGroup.m_sRuntimeStatus != "spawn_pending_agents" && CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId) > 0;
			}
		}

		if (populationResolved)
		{
			routeUpdateChanged = UpdateRoutedActiveGroupsNow(state, preset, true);
			if (!IsActiveGroupInfantryWaypointAssigned(activeGroup) && activeGroup.m_bSpawnedEntity && activeGroup.m_iInfantryCount > 0 && !IsMissionConvoyGroup(activeGroup) && (activeGroup.m_sRuntimeStatus == "routing" || activeGroup.m_sRuntimeStatus == "support_active"))
			{
				ref array<vector> routePositions = BuildActiveGroupRoutePositions(ResolveActiveGroupGeneratedRoute(state, activeGroup), activeGroup);
				manualAssignedWaypoints = AssignActiveGroupInfantryRouteWaypoints(activeGroup, routePositions, assignedFinalSweepWaypoint);
				if (manualAssignedWaypoints > 1)
				{
					activeGroup.m_iAssignedWaypointCount = manualAssignedWaypoints;
					activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "infantry_waypoints");
					if (assignedFinalSweepWaypoint)
						activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "infantry_sweep");
					activeGroup.m_sSpawnFailureReason = string.Format("Assigned infantry route waypoint chain %1 via campaign debug route resolver | final sweep %2.", manualAssignedWaypoints, ReportBool(assignedFinalSweepWaypoint));
					manualRouteAssigned = true;
					routeUpdateChanged = true;
				}
			}
		}

		bool routeAssigned = IsActiveGroupInfantryWaypointAssigned(activeGroup) && activeGroup.m_sSpawnFallbackMode.Contains("infantry_sweep");
		evidence = string.Format("status %1 -> %2 | mode %3 -> %4 | waypoints %5 -> %6",
			ReportText(beforeStatus),
			ReportText(activeGroup.m_sRuntimeStatus),
			ReportText(beforeMode),
			ReportText(activeGroup.m_sSpawnFallbackMode),
			beforeWaypoints,
			activeGroup.m_iAssignedWaypointCount);
		evidence = evidence + string.Format(" | population %1 | directFallback %2 | routeUpdate %3",
			ReportBool(populationResolved),
			ReportBool(directFallbackResolved),
			ReportBool(routeUpdateChanged));
		evidence = evidence + string.Format(" | manualRoute %1/%2 finalSweep %3 | %4",
			ReportBool(manualRouteAssigned),
			manualAssignedWaypoints,
			ReportBool(assignedFinalSweepWaypoint),
			ReportText(populationEvidence));
		return routeAssigned;
	}

	bool CampaignDebugIsActiveGroupResponseRunMovement(HST_ActiveGroupState activeGroup, out string actual)
	{
		actual = "group missing";
		if (!activeGroup)
			return false;

		bool responseRunToken = activeGroup.m_sSpawnFallbackMode.Contains("response_run");
		bool groupRun;
		int liveAgentCount;
		int runAgentCount;
		string groupWanted = "none";
		AIGroup group = AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId));
		if (group)
		{
			AIGroupMovementComponent groupMovement = AIGroupMovementComponent.Cast(group.GetMovementComponent());
			if (!groupMovement)
				groupMovement = AIGroupMovementComponent.Cast(group.FindComponent(AIGroupMovementComponent));
			if (groupMovement)
			{
				EMovementType wanted = groupMovement.GetGroupCharactersMovementTypeWanted();
				groupRun = wanted == EMovementType.RUN;
				groupWanted = typename.EnumToString(EMovementType, wanted);
			}
		}

		array<IEntity> livingEntities = {};
		CollectLivingRuntimeGroupEntities(activeGroup.m_sGroupId, livingEntities);
		foreach (IEntity entity : livingEntities)
		{
			if (!entity)
				continue;

			AIControlComponent control = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
			if (!control)
				continue;

			AIAgent agent = control.GetControlAIAgent();
			if (!agent)
				continue;

			AICharacterMovementComponent movement = AICharacterMovementComponent.Cast(agent.GetMovementComponent());
			if (!movement)
				continue;

			liveAgentCount++;
			if (movement.GetMovementTypeWanted() == EMovementType.RUN)
				runAgentCount++;
		}

		bool agentRun = liveAgentCount > 0 && runAgentCount >= liveAgentCount;
		actual = string.Format("response_run %1 | group wanted %2 | groupRun %3 | agents run %4/%5", responseRunToken, groupWanted, groupRun, runAgentCount, liveAgentCount);
		return responseRunToken && (groupRun || agentRun);
	}

	protected bool TryKickPendingNativeGroupSpawn(HST_ActiveGroupState activeGroup, string source)
	{
		if (!activeGroup || activeGroup.m_sRuntimeStatus != "spawn_pending_agents")
			return false;
		if (IsActiveGroupSpawnMode(activeGroup.m_sSpawnFallbackMode, ACTIVE_GROUP_SPAWN_MODE_GROUP_RETRY))
		{
			activeGroup.m_sSpawnFailureReason = "Native SCR_AIGroup.SpawnUnits retry already queued via earlier attempt.";
			DebugLog(string.Format("active group native SpawnUnits retry skipped %1: already queued via %2", activeGroup.m_sGroupId, source));
			return false;
		}

		IEntity runtimeGroupEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		if (!runtimeGroupEntity)
		{
			activeGroup.m_sSpawnFailureReason = "Native SCR_AIGroup.SpawnUnits retry skipped: missing runtime group entity.";
			DebugLog(string.Format("active group native SpawnUnits retry skipped %1: missing runtime group entity via %2", activeGroup.m_sGroupId, source));
			return false;
		}

		SCR_AIGroup group = SCR_AIGroup.Cast(runtimeGroupEntity);
		if (!group)
		{
			activeGroup.m_sSpawnFailureReason = "Native SCR_AIGroup.SpawnUnits retry skipped: runtime entity is not SCR_AIGroup.";
			DebugLog(string.Format("active group native SpawnUnits retry skipped %1: runtime entity is not SCR_AIGroup via %2", activeGroup.m_sGroupId, source));
			return false;
		}
		if (group.IsInitializing())
		{
			activeGroup.m_sSpawnFailureReason = "Native SCR_AIGroup.SpawnUnits retry skipped: native delayed spawn is still initializing.";
			DebugLog(string.Format("active group native SpawnUnits retry skipped %1: native group still initializing via %2 | %3", activeGroup.m_sGroupId, source, BuildNativeGroupPopulationDebug(group)));
			return false;
		}

		int existingAgents = CountLivingNativeAIGroupAgents(group);
		if (existingAgents > 0)
		{
			activeGroup.m_sSpawnFailureReason = string.Format("Native SCR_AIGroup.SpawnUnits retry skipped: already has %1 live agents.", existingAgents);
			DebugLog(string.Format("active group native SpawnUnits retry skipped %1: already has live agents %2 via %3 | %4", activeGroup.m_sGroupId, existingAgents, source, BuildNativeGroupPopulationDebug(group)));
			return false;
		}

		string aiWorldBudgetFailure;
		if (!EnsureActiveGroupAIWorldBudget(activeGroup, "native SpawnUnits retry " + source, aiWorldBudgetFailure))
		{
			activeGroup.m_sSpawnFailureReason = "Native SCR_AIGroup.SpawnUnits retry deferred: " + aiWorldBudgetFailure;
			DebugLog(string.Format("active group native SpawnUnits retry deferred %1 via %2 | %3", activeGroup.m_sGroupId, source, aiWorldBudgetFailure));
			return false;
		}

		group.SetDeleteWhenEmpty(false);
		group.SetMaxUnitsToSpawn(Math.Max(1, activeGroup.m_iInfantryCount));
		group.SetMemberSpawnDelay(0);
		group.SpawnUnits();
		activeGroup.m_sSpawnFallbackMode = ResolveActiveGroupPrimarySpawnMode(activeGroup.m_sSpawnFallbackMode, ACTIVE_GROUP_SPAWN_MODE_GROUP_RETRY);
		activeGroup.m_sSpawnFailureReason = "Queued native SCR_AIGroup.SpawnUnits retry via " + source;
		DebugLog(string.Format("active group native SpawnUnits retry queued %1 expected infantry %2 via %3 | %4", activeGroup.m_sGroupId, activeGroup.m_iInfantryCount, source, BuildNativeGroupPopulationDebug(group)));
		return true;
	}

	protected bool TryFlushPendingNativeGroupSpawnImmediately(HST_ActiveGroupState activeGroup, string requestedStatus, HST_CampaignState state, string source)
	{
		if (!activeGroup || activeGroup.m_sRuntimeStatus != "spawn_pending_agents")
			return false;

		IEntity runtimeGroupEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		SCR_AIGroup group = SCR_AIGroup.Cast(runtimeGroupEntity);
		if (!group)
		{
			activeGroup.m_sSpawnFailureReason = "Native SCR_AIGroup immediate flush skipped: runtime entity is not SCR_AIGroup.";
			DebugLog(string.Format("active group native immediate flush skipped %1: runtime entity is not SCR_AIGroup via %2", activeGroup.m_sGroupId, source));
			return false;
		}

		int existingAgents = CountLivingNativeAIGroupAgents(group);
		if (existingAgents > 0)
			return TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, source + " existing native members");

		int queueBefore = group.GetSpawnQueueSize();
		if (queueBefore <= 0 && !group.IsInitializing())
		{
			activeGroup.m_sSpawnFailureReason = "Native SCR_AIGroup immediate flush skipped: delayed spawn queue is empty.";
			DebugLog(string.Format("active group native immediate flush skipped %1: empty queue via %2 | %3", activeGroup.m_sGroupId, source, BuildNativeGroupPopulationDebug(group)));
			return false;
		}

		string aiWorldBudgetFailure;
		if (!EnsureActiveGroupAIWorldBudget(activeGroup, "native immediate queue flush " + source, aiWorldBudgetFailure))
		{
			activeGroup.m_sSpawnFailureReason = "Native SCR_AIGroup immediate flush deferred: " + aiWorldBudgetFailure;
			DebugLog(string.Format("active group native immediate flush deferred %1 via %2 | %3", activeGroup.m_sGroupId, source, aiWorldBudgetFailure));
			return false;
		}

		group.SetDeleteWhenEmpty(false);
		group.SetMaxUnitsToSpawn(Math.Max(1, activeGroup.m_iInfantryCount));
		group.SetMemberSpawnDelay(0);
		group.SpawnAllImmediately();
		activeGroup.m_sSpawnFallbackMode = ResolveActiveGroupPrimarySpawnMode(activeGroup.m_sSpawnFallbackMode, ACTIVE_GROUP_SPAWN_MODE_GROUP_NATIVE_IMMEDIATE);

		int queueAfter = group.GetSpawnQueueSize();
		int liveAfter = CountLivingNativeAIGroupAgents(group);
		activeGroup.m_sSpawnFailureReason = string.Format("Native SCR_AIGroup delayed queue flushed immediately via %1; queue %2 -> %3 live %4.", source, queueBefore, queueAfter, liveAfter);
		DebugLog(string.Format("active group native immediate flush %1 expected infantry %2 via %3 | queue %4 -> %5 live %6 | %7", activeGroup.m_sGroupId, activeGroup.m_iInfantryCount, source, queueBefore, queueAfter, liveAfter, BuildNativeGroupPopulationDebug(group)));
		if (liveAfter <= 0)
			return false;

		return TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, source + " native immediate flush");
	}

	protected bool IsActiveGroupNativeDelayedPopulationActive(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		return IsNativeGroupDelayedPopulationActive(SCR_AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId)));
	}

	protected bool IsNativeGroupDelayedPopulationActive(SCR_AIGroup group)
	{
		if (!group)
			return false;

		return group.IsInitializing() || group.GetSpawnQueueSize() > 0;
	}

	protected int CountNativeGroupMemberSlots(SCR_AIGroup group)
	{
		if (!group || !group.m_aUnitPrefabSlots)
			return 0;

		return group.m_aUnitPrefabSlots.Count();
	}

	protected string BuildAIWorldBudgetDebug()
	{
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return "aiWorld missing";

		return string.Format("aiWorld limited %1/%2 canAdd %3 active %4/%5 canActivate %6",
			aiWorld.GetCurrentAmountOfLimitedAIs(),
			aiWorld.GetAILimit(),
			ReportBool(aiWorld.CanLimitedAIBeAdded()),
			aiWorld.GetCurrentNumOfActiveAIs(),
			aiWorld.GetLimitOfActiveAIs(),
			ReportBool(aiWorld.CanAIBeActivated()));
	}

	protected int ResolveActiveGroupAIWorldDesiredMemberCount(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return 1;

		if (activeGroup.m_iInfantryCount > 0)
			return Math.Max(1, activeGroup.m_iInfantryCount);

		return 1;
	}

	protected bool EnsureActiveGroupAIWorldBudget(HST_ActiveGroupState activeGroup, string source, out string failureReason)
	{
		failureReason = "";
		if (!activeGroup)
		{
			failureReason = "AIWorld budget check failed: active group is missing.";
			return false;
		}
		if (activeGroup.m_iInfantryCount <= 0)
			return true;

		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
		{
			failureReason = "AIWorld budget check failed: AIWorld is missing.";
			return false;
		}

		int desiredMembers = ResolveActiveGroupAIWorldDesiredMemberCount(activeGroup);
		int currentLimited = aiWorld.GetCurrentAmountOfLimitedAIs();
		int currentLimit = aiWorld.GetAILimit();
		int requiredLimit = currentLimited + desiredMembers + ACTIVE_GROUP_AI_WORLD_REQUIRED_HEADROOM;
		if (requiredLimit < ACTIVE_GROUP_AI_WORLD_MIN_LIMIT)
			requiredLimit = ACTIVE_GROUP_AI_WORLD_MIN_LIMIT;

		if (currentLimit < requiredLimit)
		{
			aiWorld.SetAILimit(requiredLimit);
			Print(string.Format("h-istasi | active group AIWorld limit raised | group %1 | source %2 | limited %3 | limit %4 -> %5 | desiredMembers %6 | headroom %7",
				activeGroup.m_sGroupId,
				ReportText(source),
				currentLimited,
				currentLimit,
				requiredLimit,
				desiredMembers,
				ACTIVE_GROUP_AI_WORLD_REQUIRED_HEADROOM));
		}

		currentLimited = aiWorld.GetCurrentAmountOfLimitedAIs();
		currentLimit = aiWorld.GetAILimit();
		if (!aiWorld.CanLimitedAIBeAdded())
		{
			failureReason = string.Format("AIWorld native member budget denied for primary active-group spawn via %1: expected members %2 | %3", ReportText(source), desiredMembers, BuildAIWorldBudgetDebug());
			return false;
		}
		if (currentLimit > 0 && currentLimited + desiredMembers > currentLimit)
		{
			failureReason = string.Format("AIWorld native member budget lacks headroom for primary active-group spawn via %1: expected members %2 | %3", ReportText(source), desiredMembers, BuildAIWorldBudgetDebug());
			return false;
		}

		return true;
	}

	protected void MarkActiveGroupAIWorldBudgetDeferred(HST_ActiveGroupState activeGroup, HST_CampaignState state, string failureReason, string stage)
	{
		if (!activeGroup)
			return;

		ClearPendingActiveGroupPopulation(activeGroup);
		DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
		activeGroup.m_bSpawnAttempted = false;
		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_sRuntimeEntityId = "";
		activeGroup.m_sRuntimeStatus = ACTIVE_GROUP_RUNTIME_STATUS_AIWORLD_BUDGET_DEFERRED;
		activeGroup.m_sSpawnFallbackMode = ACTIVE_GROUP_SPAWN_MODE_AIWORLD_BUDGET_DEFERRED;
		activeGroup.m_sSpawnFailureReason = failureReason;
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_iSurvivorVehicleCount = 0;
		if (IsMissionConvoyGroup(activeGroup))
		{
			activeGroup.m_sCrewPopulationFailureReason = failureReason;
			activeGroup.m_sConvoyRuntimeStage = "CREW_AIWORLD_BUDGET_DEFERRED";
		}
		RefreshActiveGroupZoneCounts(state, activeGroup);
		Print(string.Format("h-istasi | active group AIWorld native spawn deferred | group %1 | stage %2 | reason %3", activeGroup.m_sGroupId, ReportText(stage), ReportText(failureReason)), LogLevel.WARNING);
		PrintActiveGroupSpawnEvidence(state, activeGroup, stage);
	}

	protected string BuildNativeGroupPopulationDebug(SCR_AIGroup group)
	{
		if (!group)
			return "native group missing | " + BuildAIWorldBudgetDebug();

		string evidence = string.Format("raw %1 living %2 slots %3 queue %4 initializing %5 membersToSpawn %6 | %7",
			group.GetAgentsCount(),
			CountLivingNativeAIGroupAgents(group),
			CountNativeGroupMemberSlots(group),
			group.GetSpawnQueueSize(),
			ReportBool(group.IsInitializing()),
			group.GetNumberOfMembersToSpawn(),
			BuildAIWorldBudgetDebug());
		return evidence + " | " + BuildEditableGroupRuntimeEvidence(group);
	}

	protected string BuildEditableGroupRuntimeEvidence(SCR_AIGroup group)
	{
		if (!group)
			return "editable missing";

		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(group.FindComponent(SCR_EditableGroupComponent));
		if (!editableGroup)
			return "editable missing";

		string factionKey = "";
		Faction faction = editableGroup.GetFaction();
		if (faction)
			factionKey = faction.GetFactionKey();

		int editableMembers;
		int parentedMembers;
		int missingEditableMembers;
		int missingControlledEntities;
		array<AIAgent> agents = new array<AIAgent>;
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity controlledEntity = agent.GetControlledEntity();
			if (!controlledEntity)
			{
				missingControlledEntities++;
				continue;
			}

			SCR_EditableEntityComponent editableMember = SCR_EditableEntityComponent.GetEditableEntity(controlledEntity);
			if (!editableMember)
			{
				missingEditableMembers++;
				continue;
			}

			editableMembers++;
			if (editableMember.GetParentEntity() == editableGroup)
				parentedMembers++;
		}

		return string.Format("editableSize %1 editableFaction %2 serverAgents %3 leader %4 editableParented %5/%6 editableMissing %7 missingControlled %8",
			editableGroup.GetSize(),
			ReportText(factionKey),
			group.GetServerAgentsCount(),
			ReportBool(group.GetLeaderAgent() != null),
			parentedMembers,
			editableMembers,
			missingEditableMembers,
			missingControlledEntities);
	}

	protected void ReconcileRuntimeGroupEditableMembership(SCR_AIGroup group, HST_ActiveGroupState activeGroup, string source)
	{
		if (!group || !activeGroup)
			return;

		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(group.FindComponent(SCR_EditableGroupComponent));
		if (!editableGroup)
		{
			Print(string.Format("h-istasi | active group editable membership missing group component | group %1 | source %2 | visual %3", activeGroup.m_sGroupId, ReportText(source), ReportText(BuildRuntimeEntityVisualEvidence(group))), LogLevel.WARNING);
			return;
		}

		int rawCount = group.GetAgentsCount();
		int serverCount = group.GetServerAgentsCount();
		int playerAndAgentCount = group.GetPlayerAndAgentCount();
		int livingCount;
		int editableCount;
		int parentedBefore;
		int parentedAfter;
		int repairedParents;
		int failedParents;
		int missingEditable;
		int missingControlled;
		int deadControlled;
		AIAgent firstLivingAgent;

		array<AIAgent> agents = new array<AIAgent>;
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity controlledEntity = agent.GetControlledEntity();
			if (!controlledEntity)
			{
				missingControlled++;
				continue;
			}

			if (IsLivingEntity(controlledEntity))
			{
				livingCount++;
				if (!firstLivingAgent)
					firstLivingAgent = agent;
			}
			else
			{
				deadControlled++;
			}

			SCR_EditableEntityComponent editableMember = SCR_EditableEntityComponent.GetEditableEntity(controlledEntity);
			if (!editableMember)
			{
				missingEditable++;
				continue;
			}

			editableCount++;
			if (editableMember.GetParentEntity() == editableGroup)
			{
				parentedBefore++;
				parentedAfter++;
				continue;
			}

			editableMember.SetParentEntity(editableGroup);
			if (editableMember.GetParentEntity() == editableGroup)
			{
				repairedParents++;
				parentedAfter++;
			}
			else
			{
				failedParents++;
			}
		}

		bool leaderChanged;
		AIAgent currentLeader = group.GetLeaderAgent();
		bool leaderLiving;
		if (currentLeader && IsLivingEntity(currentLeader.GetControlledEntity()))
			leaderLiving = true;
		if (firstLivingAgent && !leaderLiving)
		{
			group.SetNewLeader(firstLivingAgent);
			leaderChanged = true;
		}

		bool suspicious = editableGroup.GetSize() != playerAndAgentCount || playerAndAgentCount <= 0 || parentedAfter < editableCount || repairedParents > 0 || failedParents > 0 || missingEditable > 0 || missingControlled > 0 || leaderChanged;
		if (suspicious)
		{
			string report = string.Format("h-istasi | active group editable membership reconciled | group %1 | source %2 | raw %3 server %4 playerAgents %5 living %6 | editableSize %7",
				activeGroup.m_sGroupId,
				ReportText(source),
				rawCount,
				serverCount,
				playerAndAgentCount,
				livingCount,
				editableGroup.GetSize());
			report = report + string.Format(" | parented %1 -> %2/%3 repaired %4 failed %5 | missingEditable %6 missingControlled %7 deadControlled %8",
				parentedBefore,
				parentedAfter,
				editableCount,
				repairedParents,
				failedParents,
				missingEditable,
				missingControlled,
				deadControlled);
			report = report + string.Format(" | leaderChanged %1 | visual %2",
				ReportBool(leaderChanged),
				ReportText(BuildRuntimeEntityVisualEvidence(group)));
			Print(report);
		}
		else
		{
			DebugLog(string.Format("active group editable membership verified %1 via %2 | %3", activeGroup.m_sGroupId, source, BuildRuntimeEntityVisualEvidence(group)));
		}
	}

	protected bool TryPopulatePendingActiveGroupFromNativeSlots(HST_ActiveGroupState activeGroup, string requestedStatus, HST_CampaignState state, string source)
	{
		if (!activeGroup || activeGroup.m_sRuntimeStatus != "spawn_pending_agents")
			return false;
		if (activeGroup.m_iInfantryCount <= 0)
		{
			activeGroup.m_sSpawnFailureReason = "Stock group member-slot population skipped: active group has no infantry count.";
			DebugLog(string.Format("active group stock member-slot population skipped %1: no infantry count via %2", activeGroup.m_sGroupId, source));
			return false;
		}

		IEntity runtimeGroupEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		SCR_AIGroup group = SCR_AIGroup.Cast(runtimeGroupEntity);
		if (!group)
		{
			activeGroup.m_sSpawnFailureReason = "Stock group member-slot population skipped: runtime entity is not SCR_AIGroup.";
			DebugLog(string.Format("active group stock member-slot population skipped %1: runtime entity is not SCR_AIGroup via %2", activeGroup.m_sGroupId, source));
			return false;
		}

		if (IsNativeGroupDelayedPopulationActive(group))
		{
			activeGroup.m_sSpawnFailureReason = "Stock group member-slot population waiting: native delayed spawn queue is still active.";
			DebugLog(string.Format("active group stock member-slot population waiting %1 via %2 | %3", activeGroup.m_sGroupId, source, BuildNativeGroupPopulationDebug(group)));
			return false;
		}

		int existingCount = CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId);
		if (existingCount > 0)
			return TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, source + " existing stock members");

		string aiWorldBudgetFailure;
		if (!EnsureActiveGroupAIWorldBudget(activeGroup, "stock member-slot population " + source, aiWorldBudgetFailure))
		{
			activeGroup.m_sSpawnFailureReason = "Stock group member-slot population deferred: " + aiWorldBudgetFailure;
			DebugLog(string.Format("active group stock member-slot population deferred %1 via %2 | %3", activeGroup.m_sGroupId, source, aiWorldBudgetFailure));
			return false;
		}

		int spawnedCount = SpawnNativeSlotMembersIntoRuntimeGroup(group, activeGroup, source);
		if (spawnedCount <= 0)
		{
			activeGroup.m_sSpawnFailureReason = "Stock group member-slot population failed: spawned zero valid members.";
			DebugLog(string.Format("active group stock member-slot population failed %1 via %2 | %3", activeGroup.m_sGroupId, source, BuildNativeGroupPopulationDebug(group)));
			return false;
		}

		activeGroup.m_sSpawnFallbackMode = ResolveActiveGroupPrimarySpawnMode(activeGroup.m_sSpawnFallbackMode, ACTIVE_GROUP_SPAWN_MODE_GROUP_SLOT_PRIMARY);
		activeGroup.m_sSpawnFailureReason = "Native group prefab populated from its stock member slots via " + source + ".";
		DebugLog(string.Format("active group populated %1 with %2 stock %3 slot members via %4 | %5", activeGroup.m_sGroupId, spawnedCount, activeGroup.m_sFactionKey, source, BuildNativeGroupPopulationDebug(group)));
		return TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, source + " stock member slots");
	}

	protected int SpawnNativeSlotMembersIntoRuntimeGroup(SCR_AIGroup group, HST_ActiveGroupState activeGroup, string source)
	{
		if (!group || !activeGroup || activeGroup.m_iInfantryCount <= 0)
			return 0;
		if (!group.m_aUnitPrefabSlots || group.m_aUnitPrefabSlots.Count() <= 0)
		{
			DebugLog(string.Format("active group stock member-slot population failed %1: group prefab has zero member slots via %2", activeGroup.m_sGroupId, source));
			return 0;
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		int desiredCount = Math.Min(Math.Max(1, activeGroup.m_iInfantryCount), group.m_aUnitPrefabSlots.Count());
		int spawnedCount;
		string firstPrefab;
		for (int i = 0; i < desiredCount; i++)
		{
			ResourceName slotPrefab = group.m_aUnitPrefabSlots[i];
			string slotPrefabText = slotPrefab;
			if (slotPrefabText.IsEmpty())
				continue;
			if (firstPrefab.IsEmpty())
				firstPrefab = slotPrefabText;
			if (!IsValidInfantryCharacterPrefabResource(slotPrefabText, activeGroup.m_sFactionKey))
			{
				Print(string.Format("h-istasi | active group rejected stock slot member %1 | group %2 | expected %3 | source %4", slotPrefabText, activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source), LogLevel.WARNING);
				continue;
			}

			vector position = ResolveFallbackInfantryMemberPosition(activeGroup.m_vPosition, i);
			EntitySpawnParams params = new EntitySpawnParams;
			params.TransformMode = ETransformMode.WORLD;
			params.Transform[3] = position;
			IEntity member = GetGame().SpawnEntityPrefabEx(slotPrefab, true, world, params);
			if (!member)
			{
				Print(string.Format("h-istasi | active group stock slot member spawn failed | group %1 | prefab %2 | source %3", activeGroup.m_sGroupId, slotPrefabText, source), LogLevel.WARNING);
				continue;
			}

			member.SetOrigin(position);
			ApplyEntityFaction(member, activeGroup.m_sFactionKey);
			if (!AttachFactionInfantryMemberToRuntimeGroup(group, member, activeGroup, i))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(member);
				continue;
			}
			ApplyEntityFaction(member, activeGroup.m_sFactionKey);
			m_aRuntimeGroupIds.Insert(activeGroup.m_sGroupId);
			m_aRuntimeGroupEntities.Insert(member);
			Print(string.Format("h-istasi | stock group slot member spawned | group %1 | faction %2 | prefab %3 | position %4 | source %5", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, slotPrefabText, position, source));
			spawnedCount++;
		}

		if (spawnedCount > 0)
		{
			group.SetNumberOfMembersToSpawn(spawnedCount);
			ApplyRuntimeGroupFaction(group, activeGroup, "stock group member slots", true);
			group.ActivateAI();
		}
		else
		{
			Print(string.Format("h-istasi | active group stock member-slot population failed %1 faction %2 groupPrefab %3 firstSlot %4 source %5", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, activeGroup.m_sPrefab, ReportText(firstPrefab), source), LogLevel.WARNING);
		}

		return spawnedCount;
	}

	protected bool TryPopulatePendingActiveGroupFromFactionInfantry(HST_ActiveGroupState activeGroup, string requestedStatus, HST_CampaignState state, string source, bool allowInitializingFallback = false)
	{
		if (!activeGroup || activeGroup.m_sRuntimeStatus != "spawn_pending_agents")
			return false;
		if (activeGroup.m_iInfantryCount <= 0)
		{
			activeGroup.m_sSpawnFailureReason = "Direct faction infantry fallback skipped: active group has no infantry count.";
			DebugLog(string.Format("active group direct infantry fallback skipped %1: no infantry count via %2", activeGroup.m_sGroupId, source));
			return false;
		}

		string aiWorldBudgetFailure;
		if (!EnsureActiveGroupAIWorldBudget(activeGroup, "direct faction infantry fallback " + source, aiWorldBudgetFailure))
		{
			activeGroup.m_sSpawnFailureReason = "Direct faction infantry fallback deferred: " + aiWorldBudgetFailure;
			DebugLog(string.Format("active group direct infantry fallback deferred %1 via %2 | %3", activeGroup.m_sGroupId, source, aiWorldBudgetFailure));
			return false;
		}

		IEntity runtimeGroupEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		SCR_AIGroup group;
		if (!runtimeGroupEntity)
		{
			activeGroup.m_sSpawnFailureReason = "Direct faction infantry fallback replacing missing runtime group entity.";
			DebugLog(string.Format("active group direct infantry fallback replacing missing runtime group entity %1 via %2", activeGroup.m_sGroupId, source));
			group = ReplaceEmptyNativeGroupForDirectInfantry(activeGroup, null, source + " missing runtime group");
		}
		else
		{
			group = SCR_AIGroup.Cast(runtimeGroupEntity);
			if (!group)
			{
				activeGroup.m_sSpawnFailureReason = "Direct faction infantry fallback skipped: runtime entity is not SCR_AIGroup.";
				DebugLog(string.Format("active group direct infantry fallback skipped %1: runtime entity is not SCR_AIGroup via %2", activeGroup.m_sGroupId, source));
				return false;
			}
			if (group.IsInitializing() && !allowInitializingFallback)
			{
				activeGroup.m_sSpawnFailureReason = "Direct faction infantry fallback skipped: native group is still initializing.";
				DebugLog(string.Format("active group direct infantry fallback skipped %1: native group still initializing via %2", activeGroup.m_sGroupId, source));
				return false;
			}
			if (group.IsInitializing() && allowInitializingFallback)
				DebugLog(string.Format("active group forcing direct infantry fallback while native group is still initializing %1 via %2", activeGroup.m_sGroupId, source));

			int existingCount = CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId);
			if (existingCount > 0)
				return TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, source + " existing agents");

			group = ReplaceEmptyNativeGroupForDirectInfantry(activeGroup, group, source);
		}
		if (!group)
		{
			activeGroup.m_sSpawnFailureReason = "Direct faction infantry fallback failed: replacement group could not be created.";
			DebugLog(string.Format("active group direct infantry fallback failed %1: replacement group could not be created via %2", activeGroup.m_sGroupId, source));
			return false;
		}

		int spawnedCount = SpawnFactionInfantryIntoRuntimeGroup(group, activeGroup);
		if (spawnedCount <= 0)
		{
			activeGroup.m_sSpawnFailureReason = "Direct faction infantry fallback failed: spawned zero valid infantry members.";
			DebugLog(string.Format("active group direct infantry fallback failed %1: spawned zero valid infantry members via %2", activeGroup.m_sGroupId, source));
			return false;
		}

		activeGroup.m_sSpawnFallbackMode = ResolveDirectInfantryFallbackMode(activeGroup.m_sSpawnFallbackMode);
		activeGroup.m_sSpawnFailureReason = "Native group prefab produced zero live controlled members; direct faction infantry fallback populated via " + source + ".";
		DebugLog(string.Format("active group populated %1 with %2 direct %3 infantry after empty group prefab via %4", activeGroup.m_sGroupId, spawnedCount, activeGroup.m_sFactionKey, source));
		return TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, source + " direct infantry fallback");
	}

	protected string ResolveDirectInfantryFallbackMode(string previousMode)
	{
		if (IsDirectInfantryFallbackMode(previousMode))
			return previousMode;
		if (previousMode.IsEmpty() || previousMode == ACTIVE_GROUP_SPAWN_MODE_GROUP || previousMode == ACTIVE_GROUP_SPAWN_MODE_GROUP_RETRY)
			return ACTIVE_GROUP_SPAWN_MODE_DIRECT_INFANTRY_FALLBACK;

		return previousMode + "_" + ACTIVE_GROUP_SPAWN_MODE_DIRECT_INFANTRY_FALLBACK;
	}

	protected bool IsDirectInfantryFallbackMode(string mode)
	{
		return mode.Contains(ACTIVE_GROUP_SPAWN_MODE_DIRECT_INFANTRY_FALLBACK);
	}

	protected bool IsActiveGroupSpawnMode(string mode, string token)
	{
		if (mode.IsEmpty() || token.IsEmpty())
			return false;

		return mode == token || mode.Contains(token);
	}

	protected bool ShouldPreserveActiveGroupSemanticSpawnMode(string mode)
	{
		if (mode.IsEmpty())
			return false;

		return mode.Contains("support") || mode.Contains("convoy");
	}

	protected string ResolveActiveGroupPrimarySpawnMode(string previousMode, string primaryMode)
	{
		if (primaryMode.IsEmpty())
			return previousMode;
		if (previousMode.IsEmpty())
			return primaryMode;
		if (IsActiveGroupSpawnMode(previousMode, primaryMode))
			return previousMode;
		if (ShouldPreserveActiveGroupSemanticSpawnMode(previousMode))
			return previousMode + "_" + primaryMode;

		return primaryMode;
	}

	protected SCR_AIGroup ReplaceEmptyNativeGroupForDirectInfantry(HST_ActiveGroupState activeGroup, SCR_AIGroup nativeGroup, string source)
	{
		if (!activeGroup)
			return null;

		if (nativeGroup)
			nativeGroup.GetOnAllDelayedEntitySpawned().Remove(OnDelayedActiveGroupMembersSpawned);

		DeleteRuntimeCrewEntities(activeGroup.m_sGroupId);

		ResourceName resourceName = ResolveDirectInfantryGroupPrefab(activeGroup.m_sFactionKey);
		Resource loaded = Resource.Load(resourceName);
		if (!loaded || !loaded.IsValid())
		{
			Print(string.Format("h-istasi | active group direct fallback failed %1: missing group prefab %2", activeGroup.m_sGroupId, resourceName), LogLevel.WARNING);
			return null;
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = activeGroup.m_vPosition;
		SCR_AIGroup.IgnoreSpawning(true);
		IEntity entity = GetGame().SpawnEntityPrefabEx(resourceName, true, world, params);
		SCR_AIGroup.IgnoreSpawning(false);
		SCR_AIGroup replacementGroup = SCR_AIGroup.Cast(entity);
		if (!replacementGroup)
		{
			if (entity)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			Print(string.Format("h-istasi | active group direct fallback failed %1: prefab %2 did not spawn an AIGroup", activeGroup.m_sGroupId, resourceName), LogLevel.WARNING);
			return null;
		}

		ApplyCampaignDebugEntityName(entity, "direct_group", activeGroup.m_sGroupId);
		replacementGroup.SetSpawnImmediately(false);
		replacementGroup.SetDeleteWhenEmpty(false);
		ApplyRuntimeGroupFaction(entity, activeGroup, source + " direct infantry group replacement", true);
		m_aRuntimeGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeGroupEntities.Insert(entity);
		DebugLog(string.Format("active group replaced empty native AIGroup for direct infantry fallback %1 via %2", activeGroup.m_sGroupId, source));
		return replacementGroup;
	}

	protected string ResolveDirectInfantryGroupPrefab(string factionKey)
	{
		if (factionKey == "US")
			return DIRECT_INFANTRY_GROUP_PREFAB_US;
		if (factionKey == "USSR")
			return DIRECT_INFANTRY_GROUP_PREFAB_USSR;

		return DIRECT_INFANTRY_GROUP_PREFAB;
	}

	protected int SpawnFactionInfantryIntoRuntimeGroup(SCR_AIGroup group, HST_ActiveGroupState activeGroup)
	{
		if (!group || !activeGroup)
			return 0;

		int desiredCount = Math.Max(1, activeGroup.m_iInfantryCount);
		int spawnedCount;
		for (int i = 0; i < desiredCount; i++)
		{
			string prefab = SelectFactionInfantryCharacterPrefab(activeGroup.m_sFactionKey, activeGroup.m_sGroupId, i);
			if (prefab.IsEmpty())
				continue;

			vector position = ResolveFallbackInfantryMemberPosition(activeGroup.m_vPosition, i);
			IEntity member = SpawnFallbackInfantryCharacter(prefab, position, activeGroup.m_sFactionKey);
			if (!member)
				continue;

			ApplyEntityFaction(member, activeGroup.m_sFactionKey);
			if (!AttachFactionInfantryMemberToRuntimeGroup(group, member, activeGroup, i))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(member);
				continue;
			}
			ApplyEntityFaction(member, activeGroup.m_sFactionKey);

			m_aRuntimeGroupIds.Insert(activeGroup.m_sGroupId);
			m_aRuntimeGroupEntities.Insert(member);
			Print(string.Format("h-istasi | direct infantry spawned | group %1 | faction %2 | prefab %3 | position %4", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, prefab, position));
			spawnedCount++;
		}

		if (spawnedCount > 0)
		{
			ApplyRuntimeGroupFaction(group, activeGroup, "direct infantry fallback", true);
			group.ActivateAI();
		}

		if (spawnedCount <= 0)
			Print(string.Format("h-istasi | active group fallback infantry failed %1 faction %2 prefab %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, activeGroup.m_sPrefab), LogLevel.WARNING);

		return spawnedCount;
	}

	protected bool AttachFactionInfantryMemberToRuntimeGroup(SCR_AIGroup group, IEntity member, HST_ActiveGroupState activeGroup, int index)
	{
		if (!group || !member || !activeGroup)
			return false;

		AIAgent agent = ResolveRuntimeMemberAIAgent(member);
		if (!agent)
		{
			DebugLog(string.Format("active group member attach failed %1 index %2: missing AI agent", activeGroup.m_sGroupId, index));
			return false;
		}

		if (agent.GetParentGroup() == group)
			return true;

		group.AddAgentFromControlledEntity(member);
		if (agent.GetParentGroup() != group)
		{
			if (!group.AddAIEntityToGroup(member))
				group.AddAgent(agent);
		}

		if (agent.GetParentGroup() != group)
		{
			DebugLog(string.Format("active group member attach failed %1 index %2: parent group did not match after attach", activeGroup.m_sGroupId, index));
			return false;
		}

		agent.ActivateAI();
		return true;
	}

	protected AIAgent ResolveRuntimeMemberAIAgent(IEntity member)
	{
		if (!member)
			return null;

		AIControlComponent control = AIControlComponent.Cast(member.FindComponent(AIControlComponent));
		if (!control)
			return null;

		AIAgent agent = control.GetControlAIAgent();
		if (agent)
			return agent;

		return control.GetAIAgent();
	}

	protected string SelectFactionInfantryCharacterPrefab(string factionKey, string groupId, int index)
	{
		HST_FactionRuntimeSpawnSpec spec = HST_DefaultCatalog.ResolveRuntimeSpawnSpec(null, factionKey, "direct_infantry", "direct fallback infantry");
		if (!spec || spec.m_aDirectInfantryPrefabs.Count() == 0)
			return "";

		int seed = groupId.Length() * 17 + factionKey.Length() * 31 + index * 43;
		int startIndex = HST_DefaultCatalog.PositiveMod(seed, spec.m_aDirectInfantryPrefabs.Count());
		for (int offset = 0; offset < spec.m_aDirectInfantryPrefabs.Count(); offset++)
		{
			int candidateIndex = HST_DefaultCatalog.PositiveMod(startIndex + offset, spec.m_aDirectInfantryPrefabs.Count());
			string prefab = spec.m_aDirectInfantryPrefabs[candidateIndex];
			if (IsValidInfantryCharacterPrefabResource(prefab, factionKey))
				return prefab;
		}

		Print(string.Format("h-istasi | no valid infantry character prefab found for fallback group %1 faction %2", groupId, factionKey), LogLevel.WARNING);
		return "";
	}

	protected bool IsValidInfantryCharacterPrefabResource(string prefab, string factionKey)
	{
		if (prefab.IsEmpty() || !prefab.Contains("{") || !prefab.Contains("}") || !prefab.Contains("Prefabs/Characters/"))
			return false;

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			Print(string.Format("h-istasi | rejected missing infantry character prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!IsInfantryCharacterPrefabCatalogFactionMatch(prefab, factionKey))
		{
			Print(string.Format("h-istasi | rejected wrong-faction infantry character prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		return true;
	}

	protected bool IsInfantryCharacterPrefabCatalogFactionMatch(string prefab, string factionKey)
	{
		if (factionKey.IsEmpty())
			return true;

		if (factionKey == "FIA")
			return prefab.Contains("/INDFOR/FIA/") || prefab.Contains("Character_FIA_");

		if (factionKey == "US")
			return prefab.Contains("/BLUFOR/US_Army/") || prefab.Contains("Character_US_");

		if (factionKey == "USSR")
			return prefab.Contains("/OPFOR/USSR_Army/") || prefab.Contains("Character_USSR_");

		return true;
	}

	protected IEntity SpawnFallbackInfantryCharacter(string prefab, vector position, string factionKey)
	{
		ResourceName resourceName = prefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded || !loaded.IsValid())
			return null;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = position;
		IEntity entity = GetGame().SpawnEntityPrefabEx(resourceName, true, world, params);
		if (!entity)
			return null;

		entity.SetOrigin(position);
		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (factionComponent && !factionKey.IsEmpty())
			factionComponent.SetAffiliatedFactionByKey(factionKey);

		return entity;
	}

	protected bool IsRuntimeGroupRootFactionExpected(IEntity entity, HST_ActiveGroupState activeGroup, out string actualFaction)
	{
		actualFaction = "";
		if (!activeGroup || activeGroup.m_sFactionKey.IsEmpty())
			return true;

		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if (!group)
			return true;

		actualFaction = group.GetFactionName();
		return actualFaction == activeGroup.m_sFactionKey;
	}

	protected void ApplyRuntimeGroupFaction(IEntity entity, HST_ActiveGroupState activeGroup, string source, bool forceGroupSetFaction = false)
	{
		if (!entity || !activeGroup || activeGroup.m_sFactionKey.IsEmpty())
			return;

		string factionKey = activeGroup.m_sFactionKey;
		int changedCount;
		int mismatchedCount;
		string sample;
		EnsureRuntimeFactionRecursive(entity, factionKey, changedCount, mismatchedCount, sample, forceGroupSetFaction);

		if (changedCount > 0 || mismatchedCount > 0)
			Print(string.Format("h-istasi | runtime faction applied | group %1 | expected %2 | source %3 | changed %4 | mismatches %5 | visual %6 | sample %7", activeGroup.m_sGroupId, factionKey, source, changedCount, mismatchedCount, ReportText(BuildRuntimeEntityVisualEvidence(entity)), ReportText(sample)));
	}

	protected bool EnsureRuntimeFactionRecursive(IEntity root, string factionKey, out int changed, out int mismatches, out string sample, bool forceGroupSetFaction = false)
	{
		changed = 0;
		mismatches = 0;
		sample = "";
		if (!root || factionKey.IsEmpty())
			return false;

		int vehicleChangedCount;
		if (HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursiveCount(root, vehicleChangedCount))
			changed += vehicleChangedCount;

		if (HST_VehicleRootPolicy.IsVehicleRootLikeEntity(root))
		{
			mismatches += CountRuntimeVehicleClaimMismatch(root, sample);
			return mismatches == 0;
		}

		SCR_AIGroup group = SCR_AIGroup.Cast(root);
		if (group)
		{
			if (ApplyEntityFaction(root, factionKey))
			{
				changed++;
				if (sample.IsEmpty())
					sample = string.Format("group root affiliation changed to %1", ReportText(factionKey));
			}

			string rootFactionKey = ResolveEntityFactionKey(root);
			if (!rootFactionKey.IsEmpty() && rootFactionKey != factionKey)
			{
				mismatches++;
				if (sample.IsEmpty())
					sample = string.Format("group root affiliation pos %1 actual %2", root.GetOrigin(), ReportText(rootFactionKey));
			}

			string groupFactionKey = group.GetFactionName();
			if (groupFactionKey != factionKey)
			{
				string groupFactionMethod;
				if (ApplyAIGroupFaction(group, factionKey, groupFactionMethod, forceGroupSetFaction))
				{
					changed++;
					if (sample.IsEmpty())
						sample = string.Format("group root faction changed %1 -> %2 via %3", ReportText(groupFactionKey), ReportText(group.GetFactionName()), ReportText(groupFactionMethod));
				}
				groupFactionKey = group.GetFactionName();
			}
			else if (forceGroupSetFaction)
			{
				string groupFactionMethod;
				if (ApplyAIGroupFaction(group, factionKey, groupFactionMethod, forceGroupSetFaction))
				{
					changed++;
					if (sample.IsEmpty())
						sample = string.Format("group root faction rebroadcast %1 via %2", ReportText(groupFactionKey), ReportText(groupFactionMethod));
				}
			}
			if (groupFactionKey != factionKey)
			{
				mismatches++;
				if (sample.IsEmpty())
					sample = string.Format("group root pos %1 actual group %2", root.GetOrigin(), ReportText(groupFactionKey));
			}

			array<AIAgent> agents = new array<AIAgent>;
			group.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;

				IEntity controlledEntity = agent.GetControlledEntity();
				if (!controlledEntity)
					continue;

				if (ApplyEntityFaction(controlledEntity, factionKey))
					changed++;
				if (ResolveEntityFactionKey(controlledEntity) != factionKey)
				{
					mismatches++;
					if (sample.IsEmpty())
						sample = string.Format("member pos %1 actual %2", controlledEntity.GetOrigin(), ReportText(ResolveEntityFactionKey(controlledEntity)));
				}
			}

			return mismatches == 0;
		}

		if (ApplyEntityFaction(root, factionKey))
			changed++;
		if (ResolveEntityFactionKey(root) != factionKey)
		{
			mismatches++;
			sample = string.Format("entity pos %1 actual %2", root.GetOrigin(), ReportText(ResolveEntityFactionKey(root)));
		}

		return mismatches == 0;
	}

	protected bool ApplyAIGroupFaction(SCR_AIGroup group, string factionKey, out string method, bool forceSetFaction = false)
	{
		method = "";
		if (!group || factionKey.IsEmpty())
			return false;

		string before = group.GetFactionName();
		if (before == factionKey && !forceSetFaction)
			return false;

		bool changed = false;
		if (before.IsEmpty() && group.InitFactionKey(factionKey))
		{
			changed = true;
			method = "InitFactionKey";
		}

		if (group.GetFactionName() != factionKey)
		{
			Faction faction = ResolveRuntimeFaction(factionKey);
			if (faction && group.SetFaction(faction))
			{
				changed = true;
				if (method.IsEmpty())
					method = "SetFaction";
				else
					method = method + "+SetFaction";
			}
		}
		else if (forceSetFaction)
		{
			Faction broadcastFaction = ResolveRuntimeFaction(factionKey);
			if (broadcastFaction && group.SetFaction(broadcastFaction))
			{
				changed = true;
				if (method.IsEmpty())
					method = "SetFactionBroadcast";
				else
					method = method + "+SetFactionBroadcast";
			}
		}

		return changed;
	}

	protected void EnsureActiveGroupRuntimeFaction(HST_ActiveGroupState activeGroup, string source)
	{
		if (!activeGroup || activeGroup.m_sFactionKey.IsEmpty())
			return;

		int totalChanged = 0;
		int totalMismatches = 0;
		string firstSample = "";
		array<IEntity> checkedEntities = {};
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != activeGroup.m_sGroupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity runtimeEntity = m_aRuntimeGroupEntities[i];
			if (!runtimeEntity || checkedEntities.Find(runtimeEntity) >= 0)
				continue;

			checkedEntities.Insert(runtimeEntity);
			int changed;
			int mismatches;
			string sample;
			EnsureRuntimeFactionRecursive(runtimeEntity, activeGroup.m_sFactionKey, changed, mismatches, sample);
			totalChanged += changed;
			totalMismatches += mismatches;
			if (firstSample.IsEmpty() && !sample.IsEmpty())
				firstSample = sample;
			if (changed > 0 || mismatches > 0)
			{
				Print(string.Format("h-istasi | runtime faction applied | group %1 | expected %2 | source %3 | changed %4 | mismatches %5 | visual %6 | sample %7", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source, changed, mismatches, ReportText(BuildRuntimeEntityVisualEvidence(runtimeEntity)), ReportText(sample)));
			}
		}

		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		if (vehicleEntity)
		{
			int vehicleChangedCount;
			bool vehicleChanged = HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursiveCount(vehicleEntity, vehicleChangedCount);
			string vehicleSample;
			int vehicleMismatches = CountRuntimeVehicleClaimMismatch(vehicleEntity, vehicleSample);
			if (vehicleChanged)
				totalChanged += Math.Max(1, vehicleChangedCount);
			totalMismatches += vehicleMismatches;
			if (firstSample.IsEmpty() && !vehicleSample.IsEmpty())
				firstSample = vehicleSample;
			if (vehicleChanged || vehicleMismatches > 0)
				Print(string.Format("h-istasi | runtime vehicle faction cleared | group %1 | source %2 | changed %3 | remaining claims %4 | sample %5", activeGroup.m_sGroupId, source, vehicleChangedCount, vehicleMismatches, ReportText(vehicleSample)));
		}

		string sample;
		int mismatches = CountActiveGroupRuntimeFactionMismatches(activeGroup, sample);
		if (sample.IsEmpty())
			sample = firstSample;
		if (mismatches > 0)
			Print(string.Format("h-istasi | runtime faction mismatch persists | group %1 | expected %2 | source %3 | repaired changed %4 mismatches %5 | audit mismatches %6 | visual %7 | sample %8", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source, totalChanged, totalMismatches, mismatches, ReportText(BuildActiveGroupRuntimeVisualEvidence(activeGroup.m_sGroupId)), ReportText(sample)), LogLevel.WARNING);
	}

	protected bool TryRepairEmptyRuntimeGroupPopulation(HST_CampaignState state, HST_ActiveGroupState activeGroup, string source)
	{
		if (!activeGroup || !activeGroup.m_bSpawnedEntity || activeGroup.m_iInfantryCount <= 0)
			return false;
		if (activeGroup.m_sRuntimeStatus == "spawn_pending_agents" || IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;
		if (activeGroup.m_iSpawnedAgentCount > 0 || activeGroup.m_iLastSeenAliveCount > 0 || activeGroup.m_iSurvivorInfantryCount > 0)
			return false;

		IEntity groupEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		if (!groupEntity)
			return false;
		if (CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId) > 0)
			return false;

		string previousStatus = activeGroup.m_sRuntimeStatus;
		bool previousSpawned = activeGroup.m_bSpawnedEntity;
		activeGroup.m_sRuntimeStatus = "spawn_pending_agents";
		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_sSpawnFailureReason = "Runtime group shell had zero live agents; forcing stock group member-slot population via " + source + ".";
		if (IsMissionConvoyGroup(activeGroup) && activeGroup.m_sConvoyRuntimeStage.IsEmpty())
			activeGroup.m_sConvoyRuntimeStage = "CREW_GROUP_CREATED";

		if (TryPopulatePendingActiveGroupFromNativeSlots(activeGroup, previousStatus, state, source + " empty runtime shell"))
		{
			Print(string.Format("h-istasi | repaired empty runtime group %1 with stock %2 member slots via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
			return true;
		}

		if (TryPopulatePendingActiveGroupFromFactionInfantry(activeGroup, previousStatus, state, source + " empty runtime shell", true))
		{
			Print(string.Format("h-istasi | repaired empty runtime group %1 with direct %2 infantry via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
			return true;
		}

		activeGroup.m_sRuntimeStatus = previousStatus;
		activeGroup.m_bSpawnedEntity = previousSpawned;
		activeGroup.m_sSpawnFailureReason = "Runtime group shell had zero live agents and direct faction infantry repair failed via " + source + ".";
		Print(string.Format("h-istasi | empty runtime group repair failed %1 expected %2 via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source), LogLevel.WARNING);
		return false;
	}

	protected bool CanAttemptMissionConvoyCrewPopulationRepair(HST_CampaignState state, HST_ActiveMissionState mission, HST_ActiveGroupState activeGroup)
	{
		if (!state || !mission || !activeGroup)
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
			return false;
		if (IsPersistenceSmokeMission(mission) || IsPersistenceSmokeActiveGroup(activeGroup))
			return false;
		if (!IsMissionConvoyGroupForMission(activeGroup, mission))
			return false;
		if (IsTerminalMissionConvoyPhase(mission) || mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT)
			return false;
		if (activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed")
			return false;
		if (activeGroup.m_bCrewPopulationTerminallyFailed || activeGroup.m_iInfantryCount <= 0)
			return false;
		if (IsMissionConvoyGroupAssetTerminal(state, activeGroup))
			return false;
		if (CountAliveRuntimeCrewAgents(activeGroup) > 0)
			return false;
		if (IsConvoyCrewControlPending(state, activeGroup))
			return false;

		return activeGroup.m_bSpawnAttempted || activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_pending_agents";
	}

	protected bool TryQueuePrimaryActiveGroupRespawnForRepair(HST_ActiveGroupState activeGroup, HST_CampaignState state, string requestedStatus, string source)
	{
		if (!activeGroup || activeGroup.m_iInfantryCount <= 0)
			return false;

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		if (SCR_AIGroup.Cast(crewEntity))
			return false;
		if (crewEntity)
			UnregisterRuntimeCrewHandlesForRespawn(activeGroup.m_sGroupId, source);

		activeGroup.m_bSpawnAttempted = false;
		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_sRuntimeEntityId = "";
		activeGroup.m_sRuntimeStatus = requestedStatus;
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_sSpawnFailureReason = "Queued primary stock group respawn via " + source + ".";

		if (!TrySpawnActiveGroup(activeGroup, state))
			return false;

		int liveCrew = CountAliveRuntimeCrewAgents(activeGroup);
		if (liveCrew > 0)
			RecordConvoyCrewObservedAlive(activeGroup, liveCrew);

		Print(string.Format("h-istasi | active group repair queued primary stock group %1 | status %2 | live %3 | source %4", activeGroup.m_sGroupId, activeGroup.m_sRuntimeStatus, liveCrew, source));
		return true;
	}

	protected bool TryRepairMissionConvoyCrewPopulation(HST_CampaignState state, HST_ActiveMissionState mission, HST_ActiveGroupState activeGroup, string source)
	{
		if (!CanAttemptMissionConvoyCrewPopulationRepair(state, mission, activeGroup))
			return false;

		string previousStatus = activeGroup.m_sRuntimeStatus;
		string previousMode = activeGroup.m_sSpawnFallbackMode;
		string previousReason = activeGroup.m_sSpawnFailureReason;
		string previousStage = activeGroup.m_sConvoyRuntimeStage;

		activeGroup.m_sRuntimeStatus = "spawn_pending_agents";
		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_sSpawnFallbackMode = "convoy_crew_population_repair";
		activeGroup.m_sSpawnFailureReason = "Convoy crew became unobserved before explicit contact; forcing stock group member-slot population via " + source + ".";
		activeGroup.m_sConvoyRuntimeStage = "CREW_REPAIR";

		if (TryQueuePrimaryActiveGroupRespawnForRepair(activeGroup, state, ResolveMissionConvoyRuntimeStatus(mission), source + " convoy crew repair"))
		{
			activeGroup.m_bCrewPopulationTerminallyFailed = false;
			activeGroup.m_sCrewPopulationFailureReason = "";
			if (CountAliveRuntimeCrewAgents(activeGroup) > 0)
				Print(string.Format("h-istasi mission convoy | repaired zero-live crew group %1 with primary stock %2 group via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
			else
				Print(string.Format("h-istasi mission convoy | queued primary stock crew group repair %1 expected %2 via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
			return true;
		}

		activeGroup.m_sRuntimeStatus = "spawn_pending_agents";
		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_sSpawnFallbackMode = "convoy_crew_population_repair";
		activeGroup.m_sSpawnFailureReason = "Convoy crew became unobserved before explicit contact; forcing stock group member-slot population via " + source + ".";
		activeGroup.m_sConvoyRuntimeStage = "CREW_REPAIR";

		if (TryPopulatePendingActiveGroupFromNativeSlots(activeGroup, ResolveMissionConvoyRuntimeStatus(mission), state, source + " convoy crew repair") && CountAliveRuntimeCrewAgents(activeGroup) > 0)
		{
			activeGroup.m_bCrewPopulationTerminallyFailed = false;
			activeGroup.m_sCrewPopulationFailureReason = "";
			Print(string.Format("h-istasi mission convoy | repaired zero-live crew group %1 with stock %2 member slots via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
			return true;
		}

		if (TryPopulatePendingActiveGroupFromFactionInfantry(activeGroup, ResolveMissionConvoyRuntimeStatus(mission), state, source + " convoy crew repair", true) && CountAliveRuntimeCrewAgents(activeGroup) > 0)
		{
			activeGroup.m_bCrewPopulationTerminallyFailed = false;
			activeGroup.m_sCrewPopulationFailureReason = "";
			Print(string.Format("h-istasi mission convoy | repaired zero-live crew group %1 with direct %2 infantry via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
			return true;
		}

		activeGroup.m_sRuntimeStatus = previousStatus;
		activeGroup.m_bSpawnAttempted = false;
		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_sSpawnFallbackMode = "convoy_crew_population_repair_failed";
		activeGroup.m_sSpawnFailureReason = "Convoy crew population repair failed via " + source + ". Previous mode " + previousMode + " reason " + previousReason + ".";
		activeGroup.m_sCrewPopulationFailureReason = activeGroup.m_sSpawnFailureReason;
		activeGroup.m_sConvoyRuntimeStage = "CREW_REPAIR_PENDING";
		if (!previousStage.IsEmpty() && previousStage == "CREW_UNOBSERVED")
			activeGroup.m_sConvoyRuntimeStage = previousStage;
		Print(string.Format("h-istasi mission convoy | zero-live crew repair failed for %1 expected %2 via %3; convoy remains pending instead of eliminated", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source), LogLevel.WARNING);
		return false;
	}

	bool CampaignDebugHasRuntimeVehicleEntity(string groupId)
	{
		return GetRuntimeVehicleEntity(groupId) != null;
	}

	string CampaignDebugBuildActiveGroupRuntimeVisualEvidence(string groupId)
	{
		return BuildActiveGroupRuntimeVisualEvidence(groupId);
	}

	protected string BuildActiveGroupRuntimeVisualEvidence(string groupId)
	{
		if (groupId.IsEmpty())
			return "group missing";

		string rootEvidence;
		string memberHandleEvidence;
		int rootCount;
		int memberHandleCount;
		int livingMemberHandleCount;
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity)
				continue;

			string entityEvidence = BuildRuntimeEntityVisualEvidence(entity);
			if (SCR_AIGroup.Cast(entity))
			{
				rootCount++;
				if (rootEvidence.IsEmpty())
					rootEvidence = entityEvidence;
				continue;
			}

			memberHandleCount++;
			if (IsLivingEntity(entity))
				livingMemberHandleCount++;
			if (memberHandleEvidence.IsEmpty())
				memberHandleEvidence = entityEvidence;
		}

		string evidence = string.Format("roots %1 memberHandles %2 livingMemberHandles %3", rootCount, memberHandleCount, livingMemberHandleCount);
		if (!memberHandleEvidence.IsEmpty())
			evidence = evidence + " | memberHandleSample " + memberHandleEvidence;
		if (!rootEvidence.IsEmpty())
			evidence = evidence + " | rootSample " + rootEvidence;

		IEntity vehicleEntity = GetRuntimeVehicleEntity(groupId);
		if (vehicleEntity)
		{
			string vehicleEvidence = "vehicle " + BuildRuntimeEntityVisualEvidence(vehicleEntity);
			evidence = evidence + " | " + vehicleEvidence;
		}

		if (rootCount <= 0 && memberHandleCount <= 0 && !vehicleEntity)
			return "no runtime entities";

		return evidence;
	}

	protected string BuildRuntimeEntityVisualEvidence(IEntity entity)
	{
		if (!entity)
			return "missing";

		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if (group)
		{
			string memberPrefab = "";
			string memberFaction = "";
			array<AIAgent> agents = new array<AIAgent>;
			group.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;

				IEntity controlledEntity = agent.GetControlledEntity();
				if (!controlledEntity)
					continue;

				memberPrefab = ResolveEntityPrefabName(controlledEntity);
				memberFaction = ResolveEntityFactionKey(controlledEntity);
				break;
			}

			string evidence = string.Format("root %1 groupFaction %2 rootFaction %3 raw %4 living %5 slots %6 queue %7 member %8 memberFaction %9", ReportText(ResolveEntityPrefabName(entity)), ReportText(group.GetFactionName()), ReportText(ResolveEntityFactionKey(entity)), group.GetAgentsCount(), CountLivingNativeAIGroupAgents(group), CountNativeGroupMemberSlots(group), group.GetSpawnQueueSize(), ReportText(memberPrefab), ReportText(memberFaction));
			return evidence + " | " + BuildEditableGroupRuntimeEvidence(group);
		}

		return string.Format("entity %1 faction %2 name %3", ReportText(ResolveEntityPrefabName(entity)), ReportText(ResolveEntityFactionKey(entity)), ReportText(entity.GetName()));
	}

	protected int CountActiveGroupRuntimeFactionMismatches(HST_ActiveGroupState activeGroup, out string sample)
	{
		sample = "";
		if (!activeGroup || activeGroup.m_sFactionKey.IsEmpty())
			return 0;

		string expectedFactionKey = activeGroup.m_sFactionKey;
		int mismatches;
		array<IEntity> checkedMemberEntities = {};
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != activeGroup.m_sGroupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity)
				continue;

			SCR_AIGroup group = SCR_AIGroup.Cast(entity);
			if (group)
			{
				mismatches += CountRuntimeGroupRootAffiliationMismatch(entity, expectedFactionKey, sample);

				string groupFactionKey = group.GetFactionName();
				if (groupFactionKey != expectedFactionKey)
				{
					if (sample.IsEmpty())
						sample = string.Format("group root pos %1 actual group %2", entity.GetOrigin(), ReportText(groupFactionKey));

					mismatches++;
				}

				array<AIAgent> agents = new array<AIAgent>;
				group.GetAgents(agents);
				foreach (AIAgent agent : agents)
				{
					if (!agent)
						continue;

					IEntity controlledEntity = agent.GetControlledEntity();
					if (!controlledEntity)
						continue;

					if (checkedMemberEntities.Find(controlledEntity) >= 0)
						continue;

					checkedMemberEntities.Insert(controlledEntity);
					mismatches += CountRuntimeEntityFactionMismatch(controlledEntity, expectedFactionKey, sample);
				}

				continue;
			}

			if (checkedMemberEntities.Find(entity) >= 0)
				continue;
			if (!IsLivingEntity(entity) && !IsInfantryCharacterPrefabVisualMismatch(ResolveEntityPrefabName(entity), expectedFactionKey))
				continue;

			checkedMemberEntities.Insert(entity);
			mismatches += CountRuntimeEntityFactionMismatch(entity, expectedFactionKey, sample);
		}

		for (int j = 0; j < m_aRuntimeVehicleGroupIds.Count(); j++)
		{
			if (m_aRuntimeVehicleGroupIds[j] != activeGroup.m_sGroupId || j >= m_aRuntimeVehicleEntities.Count())
				continue;

			mismatches += CountRuntimeVehicleClaimMismatch(m_aRuntimeVehicleEntities[j], sample);
		}

		return mismatches;
	}

	protected int CountRuntimeGroupRootAffiliationMismatch(IEntity entity, string expectedFactionKey, out string sample)
	{
		if (!entity || expectedFactionKey.IsEmpty())
			return 0;

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionComponent)
			return 0;

		string actualFactionKey = factionComponent.GetAffiliatedFactionKey();
		if (actualFactionKey == expectedFactionKey)
			return 0;

		if (sample.IsEmpty())
			sample = string.Format("group root affiliation pos %1 actual %2 expected %3", entity.GetOrigin(), ReportText(actualFactionKey), ReportText(expectedFactionKey));

		return 1;
	}

	protected string ResolveActiveGroupRuntimeRootFactionKey(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup || activeGroup.m_sGroupId.IsEmpty())
			return "";

		IEntity groupEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		SCR_AIGroup group = SCR_AIGroup.Cast(groupEntity);
		if (!group)
			return "";

		return group.GetFactionName();
	}

	protected int CountRuntimeGroupControlledEntities(string groupId)
	{
		if (groupId.IsEmpty())
			return 0;

		return CountAliveRuntimeInfantryGroupAgents(groupId);
	}

	protected int CountRuntimeEntityFactionMismatch(IEntity entity, string expectedFactionKey, out string sample)
	{
		if (!entity || expectedFactionKey.IsEmpty())
			return 0;

		if (HST_VehicleRootPolicy.IsVehicleRootLikeEntity(entity))
			return CountRuntimeVehicleClaimMismatch(entity, sample);

		string actualFactionKey = ResolveEntityFactionKey(entity);
		string prefab = ResolveEntityPrefabName(entity);
		bool factionMismatch = actualFactionKey != expectedFactionKey;
		bool visualMismatch = IsInfantryCharacterPrefabVisualMismatch(prefab, expectedFactionKey);
		if (!factionMismatch && !visualMismatch)
			return 0;

		if (sample.IsEmpty())
		{
			if (visualMismatch)
				sample = string.Format("pos %1 actual %2 prefab %3 visual mismatch expected %4", entity.GetOrigin(), ReportText(actualFactionKey), ReportText(prefab), ReportText(expectedFactionKey));
			else
				sample = string.Format("pos %1 actual %2 prefab %3", entity.GetOrigin(), ReportText(actualFactionKey), ReportText(prefab));
		}

		return 1;
	}

	protected int CountRuntimeVehicleClaimMismatch(IEntity entity, out string sample)
	{
		if (!entity)
			return 0;

		string claimSample;
		int claimCount = HST_VehicleRootPolicy.CountVehicleFactionClaimsRecursive(entity, claimSample);
		if (claimCount <= 0)
			return 0;

		if (sample.IsEmpty())
			sample = claimSample;

		return claimCount;
	}

	protected bool IsInfantryCharacterPrefabVisualMismatch(string prefab, string factionKey)
	{
		if (prefab.IsEmpty() || !prefab.Contains("Prefabs/Characters/"))
			return false;

		return !IsInfantryCharacterPrefabCatalogFactionMatch(prefab, factionKey);
	}

	protected Faction ResolveRuntimeFaction(string factionKey)
	{
		if (factionKey.IsEmpty())
			return null;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return null;

		return factionManager.GetFactionByKey(factionKey);
	}

	protected bool ApplyEntityFaction(IEntity entity, string factionKey)
	{
		if (!entity || factionKey.IsEmpty())
			return false;

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionComponent)
			return false;
		if (factionComponent.GetAffiliatedFactionKey() == factionKey)
			return false;

		factionComponent.SetAffiliatedFactionByKey(factionKey);
		return true;
	}

	protected string ResolveEntityFactionKey(IEntity entity)
	{
		if (!entity)
			return "";

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionComponent)
			return "";

		return factionComponent.GetAffiliatedFactionKey();
	}

	protected string ResolveEntityPrefabName(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "";

		return entity.GetPrefabData().GetPrefabName();
	}

	protected vector ResolveFallbackInfantryMemberPosition(vector groupPosition, int index)
	{
		vector candidate = groupPosition;
		float distance = 1.5 + (index / 8) * 1.25;
		int slot = index % 8;
		if (slot == 0)
			candidate[0] = candidate[0] + distance;
		else if (slot == 1)
			candidate[2] = candidate[2] + distance;
		else if (slot == 2)
			candidate[0] = candidate[0] - distance;
		else if (slot == 3)
			candidate[2] = candidate[2] - distance;
		else if (slot == 4)
		{
			candidate[0] = candidate[0] + distance;
			candidate[2] = candidate[2] + distance;
		}
		else if (slot == 5)
		{
			candidate[0] = candidate[0] - distance;
			candidate[2] = candidate[2] + distance;
		}
		else if (slot == 6)
		{
			candidate[0] = candidate[0] + distance;
			candidate[2] = candidate[2] - distance;
		}
		else
		{
			candidate[0] = candidate[0] - distance;
			candidate[2] = candidate[2] - distance;
		}

		return HST_WorldPositionService.ResolveSafeGroundPosition(candidate, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 1.0);
	}

	protected void ClearPendingActiveGroupPopulation(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return;

		string groupId = activeGroup.m_sGroupId;
		SCR_AIGroup group = SCR_AIGroup.Cast(GetRuntimeGroupEntity(groupId));
		if (group)
			group.GetOnAllDelayedEntitySpawned().Remove(OnDelayedActiveGroupMembersSpawned);

		for (int i = m_aPendingPopulationGroupIds.Count() - 1; i >= 0; i--)
		{
			if (m_aPendingPopulationGroupIds[i] != groupId)
				continue;

			if (i < m_aPendingPopulationRequestedStatuses.Count())
				m_aPendingPopulationRequestedStatuses.Remove(i);
			if (i < m_aPendingPopulationActiveGroups.Count())
				m_aPendingPopulationActiveGroups.Remove(i);
			if (i < m_aPendingPopulationStates.Count())
				m_aPendingPopulationStates.Remove(i);
			m_aPendingPopulationGroupIds.Remove(i);
		}
	}

	protected int FindPendingActiveGroupPopulationIndex(string groupId)
	{
		if (groupId.IsEmpty())
			return -1;

		for (int i = 0; i < m_aPendingPopulationGroupIds.Count(); i++)
		{
			if (m_aPendingPopulationGroupIds[i] == groupId)
				return i;
		}

		return -1;
	}

	protected string ResolveRuntimeGroupIdForEntity(IEntity entity)
	{
		if (!entity)
			return "";

		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (i >= m_aRuntimeGroupEntities.Count())
				continue;
			if (m_aRuntimeGroupEntities[i] == entity)
				return m_aRuntimeGroupIds[i];
		}

		return "";
	}

	protected bool TryBindPopulatedMissionConvoyGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!IsMissionConvoyGroup(activeGroup))
			return false;

		HST_ActiveMissionState mission = FindMissionForConvoyGroup(state, activeGroup);
		if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
			return false;
		if (mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT && activeGroup.m_bEverHadLivingCrew)
			return false;

		IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		if (!crewEntity || !vehicleEntity)
			return false;

		string seatingReason;
		GetConvoyVehicleControlAdapter().MoveUnseatedLivingCrewNearVehicle(crewEntity, vehicleEntity, vehicleEntity.GetOrigin());
		if (GetConvoyVehicleControlAdapter().TryBindCrewToVehicle(activeGroup, crewEntity, vehicleEntity, seatingReason))
		{
			activeGroup.m_sSpawnFallbackMode = "convoy_driver_available";
			activeGroup.m_sSpawnFailureReason = "Convoy crew populated and bound: " + seatingReason;
			RefreshMissionConvoyCrewCount(activeGroup);
			activeGroup.m_sConvoyRuntimeStage = "DRIVER_BOUND";
			return true;
		}

		if (seatingReason.Contains("seating pending yes") || seatingReason.Contains("waiting for animated AI boarding"))
			activeGroup.m_sSpawnFallbackMode = "convoy_seating_pending";
		else
			activeGroup.m_sSpawnFallbackMode = "convoy_vehicle_control_unavailable";

		activeGroup.m_sSpawnFailureReason = "Convoy crew populated; binding pending: " + seatingReason;
		if (activeGroup.m_sSpawnFailureReason.IsEmpty())
			activeGroup.m_sSpawnFailureReason = "Convoy crew populated; binding pending.";
		if (CountAliveRuntimeCrewAgents(activeGroup) > 0)
			activeGroup.m_sConvoyRuntimeStage = "CREW_POPULATED";
		return true;
	}

	protected void RefreshActiveGroupZoneCounts(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (activeGroup && activeGroup.m_bQRF)
			m_bMarkerRefreshNeeded = true;
		if (!state || !activeGroup)
			return;

		HST_ZoneState zone = state.FindZone(activeGroup.m_sZoneId);
		if (zone)
			ApplyActiveZoneCounts(state, zone);
	}

	protected bool TrySpawnActiveVehicle(HST_ActiveGroupState activeGroup, HST_CampaignState state = null)
	{
		if (!activeGroup || IsTerminalActiveGroupRuntimeStatus(activeGroup) || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		if (activeGroup.m_bSpawnAttempted && !activeGroup.m_bSpawnedEntity)
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
		{
			activeGroup.m_sSpawnFailureReason = "Respawn system is unavailable.";
			return false;
		}

		activeGroup.m_bSpawnAttempted = true;
		string requestedStatus = activeGroup.m_sRuntimeStatus;
		activeGroup.m_sRuntimeStatus = "spawning";
		activeGroup.m_sSpawnFallbackMode = "vehicle";
		activeGroup.m_sSpawnFailureReason = "";

		vector spawnPosition;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(activeGroup.m_vPosition, spawnPosition, true))
		{
			activeGroup.m_sSpawnFailureReason = "No dry vehicle-safe ground at assigned slot.";
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			Print(string.Format("h-istasi | active vehicle spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			return false;
		}

		activeGroup.m_vPosition = spawnPosition;
		string failureReason = "";
		GenericEntity entity;
		if (activeGroup.m_sPrefab.IsEmpty())
		{
			failureReason = string.Format("No vehicle prefab configured for faction %1.", activeGroup.m_sFactionKey);
		}
		else if (IsValidVehiclePrefabResource(activeGroup.m_sPrefab, activeGroup.m_sFactionKey))
		{
			vector spawnAngles = HST_WorldPositionService.BuildUprightAngles(0);
			entity = HST_WorldPositionService.SpawnPrefab(activeGroup.m_sPrefab, spawnPosition, spawnAngles);
			if (!entity)
				failureReason = string.Format("Vehicle prefab did not spawn: %1.", activeGroup.m_sPrefab);
			else
				HST_WorldPositionService.ApplyUprightEntityTransform(entity, spawnPosition, spawnAngles);
		}
		else
		{
			failureReason = string.Format("Invalid vehicle prefab: %1.", activeGroup.m_sPrefab);
		}

		if (!entity)
		{
			activeGroup.m_sSpawnFailureReason = failureReason;
			if (activeGroup.m_sSpawnFailureReason.IsEmpty())
				activeGroup.m_sSpawnFailureReason = string.Format("Vehicle prefab spawn failed for faction %1.", activeGroup.m_sFactionKey);
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			Print(string.Format("h-istasi | active vehicle prefab spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			return false;
		}

		activeGroup.m_bSpawnedEntity = true;
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = ResolveSpawnedRuntimeStatus(activeGroup, requestedStatus);
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = 1;
		activeGroup.m_iLastSeenAliveCount = 1;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_iSurvivorVehicleCount = Math.Max(1, activeGroup.m_iVehicleCount);
		if (state)
			activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(entity);
		m_aRuntimeVehicleGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeVehicleEntities.Insert(entity);
		EnsureActiveGroupRuntimeFaction(activeGroup, "vehicle spawn");
		Print(string.Format("h-istasi | spawned active vehicle %1 using %2 at %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, spawnPosition));
		return true;
	}

	protected string ResolveSpawnedRuntimeStatus(HST_ActiveGroupState activeGroup, string requestedStatus)
	{
		if (!requestedStatus.IsEmpty() && requestedStatus != "queued" && requestedStatus != "spawning")
			return requestedStatus;

		if (activeGroup && activeGroup.m_bQRF)
			return "staged";

		return "guard_center";
	}

	protected string SelectValidGroupPrefabFromList(array<string> prefabs, int seed, string factionKey, string purpose)
	{
		if (!prefabs || prefabs.Count() == 0)
			return "";

		int startIndex = HST_DefaultCatalog.PositiveMod(seed, prefabs.Count());
		for (int offset = 0; offset < prefabs.Count(); offset++)
		{
			int index = HST_DefaultCatalog.PositiveMod(startIndex + offset, prefabs.Count());
			string prefab = prefabs[index];
			if (!IsValidGroupPrefabResource(prefab, factionKey))
				continue;

			return prefab;
		}

		Print(string.Format("h-istasi | no valid %1 group prefab found for faction %2", purpose, factionKey), LogLevel.WARNING);
		return "";
	}

	protected bool IsValidGroupPrefabResource(string prefab, string factionKey)
	{
		if (prefab.IsEmpty())
			return false;

		if (prefab.Contains("_NotSpawned") || prefab.Contains("NotSpawned"))
		{
			Print(string.Format("h-istasi | rejected non-spawning group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (prefab.Contains("PlayableGroup.et"))
		{
			Print(string.Format("h-istasi | rejected player placeholder group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			Print(string.Format("h-istasi | rejected missing group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!IsGroupPrefabCatalogFactionMatch(prefab, factionKey))
		{
			Print(string.Format("h-istasi | rejected wrong-faction group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		return true;
	}

	protected bool IsGroupPrefabCatalogFactionMatch(string prefab, string factionKey)
	{
		if (factionKey.IsEmpty())
			return true;

		if (factionKey == "FIA")
			return prefab.Contains("/INDFOR/") || prefab.Contains("Group_FIA_");

		if (factionKey == "US")
			return prefab.Contains("/BLUFOR/") || prefab.Contains("Group_US_");

		if (factionKey == "USSR")
			return prefab.Contains("/OPFOR/") || prefab.Contains("Group_USSR_");

		return true;
	}

	protected bool IsValidVehiclePrefabResource(string prefab, string factionKey, bool logRejection = true)
	{
		if (prefab.IsEmpty())
			return false;

		if (IsAircraftVehicleResource(prefab))
		{
			if (logRejection)
				Print(string.Format("h-istasi | rejected aircraft vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!IsGroundVehicleResource(prefab))
		{
			if (logRejection)
				Print(string.Format("h-istasi | rejected non-ground vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
		{
			if (logRejection)
				Print(string.Format("h-istasi | rejected invalid vehicle root prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!IsGuidQualifiedVehicleResource(prefab))
		{
			if (logRejection)
				Print(string.Format("h-istasi | rejected unverified path-only vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			if (logRejection)
				Print(string.Format("h-istasi | rejected missing vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		return true;
	}

	protected bool IsGuidQualifiedVehicleResource(string prefab)
	{
		return !prefab.IsEmpty() && prefab.Contains("{") && prefab.Contains("}") && prefab.Contains("Prefabs/Vehicles/");
	}

	protected bool IsGroundVehicleResource(string prefab)
	{
		return prefab.Contains("Prefabs/Vehicles/Wheeled/") || prefab.Contains("Prefabs/Vehicles/Tracked/");
	}

	protected bool IsAircraftVehicleResource(string prefab)
	{
		return prefab.Contains("Aircraft") || prefab.Contains("Airplane") || prefab.Contains("Plane") || prefab.Contains("Helicopter") || prefab.Contains("Helicopters") || prefab.Contains("/UH") || prefab.Contains("/AH") || prefab.Contains("/Mi") || prefab.Contains("/KA") || prefab.Contains("/Ka");
	}

	protected int ResolveSpawnedAgentCount(GenericEntity entity, HST_ActiveGroupState activeGroup)
	{
		if (!entity || !activeGroup)
			return 0;

		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if (!group)
		{
			Print(string.Format("h-istasi | spawned prefab %1 for %2 did not create an AIGroup", activeGroup.m_sPrefab, activeGroup.m_sGroupId), LogLevel.WARNING);
			return 0;
		}

		int rawAgentCount = group.GetAgentsCount();
		int livingAgentCount = CountLivingNativeAIGroupAgents(group);
		if (livingAgentCount <= 0)
		{
			DebugLog(string.Format("spawned AIGroup %1 for %2 has %3 raw agents but no living controlled agents yet; awaiting population grace | %4", activeGroup.m_sPrefab, activeGroup.m_sGroupId, rawAgentCount, BuildNativeGroupPopulationDebug(group)));
			return 0;
		}

		return livingAgentCount;
	}

	protected void NotifyRuntimeEvent(HST_CampaignState state, string eventId, string title, string message, string zoneId, vector position, float durationSeconds)
	{
		if (!ShouldBroadcastRuntimeEvent(state, zoneId, position))
			return;

		string payload = string.Format("HST_NOTIFICATION|%1|enemy|warning|%2|%3|%4||%5|%6", eventId, PayloadText(title), PayloadText(message), zoneId, position, durationSeconds);
		HST_CommandMenuRequestComponent.BroadcastNotification(payload, title + ": " + message);
	}

	protected bool ShouldBroadcastRuntimeEvent(HST_CampaignState state, string zoneId, vector position)
	{
		if (!state)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (zone && zone.m_bActive)
			return true;

		if (IsZeroVector(position) && zone)
			position = zone.m_vPosition;

		return HST_WorldPositionService.IsPositionInsidePlayerEventBubble(position);
	}

	protected string ResolveZoneDisplayName(HST_CampaignState state, string zoneId)
	{
		if (!state || zoneId.IsEmpty())
			return "unknown location";

		HST_ZoneState zone = state.FindZone(zoneId);
		if (zone)
		{
			if (!zone.m_sDisplayName.IsEmpty())
				return zone.m_sDisplayName;
			return HST_DefaultCatalog.GetZoneDisplayName(zone.m_sZoneId);
		}

		string label = HST_DefaultCatalog.GetZoneDisplayName(zoneId);
		if (!label.IsEmpty() && label != zoneId)
			return label;

		label = zoneId;
		label.Replace("_", " ");
		return label;
	}

	protected string PayloadText(string value)
	{
		if (value.IsEmpty())
			return "";

		string result = value;
		result.Replace("|", "/");
		result.Replace("\n", " ");
		return result;
	}

	protected int ResolveActiveInfantryCap(HST_ZoneState zone)
	{
		if (!zone)
			return MAX_ACTIVE_INFANTRY_PER_ZONE;

		int cap = MAX_ACTIVE_INFANTRY_PER_ZONE + Math.Max(0, zone.m_iPriority / 12);
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			cap += 2;
		return Math.Min(12, cap);
	}

	protected int ResolveActiveVehicleCap(HST_ZoneState zone)
	{
		if (!zone)
			return MAX_ACTIVE_VEHICLES_PER_ZONE;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return 2;

		return MAX_ACTIVE_VEHICLES_PER_ZONE;
	}

	protected bool IsTerminalActiveGroupRuntimeStatus(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return true;

		return activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed";
	}

	protected bool IsPersistenceSmokeActiveGroup(HST_ActiveGroupState activeGroup)
	{
		return activeGroup && activeGroup.m_sGroupId.Contains(PERSISTENCE_SMOKE_PREFIX);
	}

	protected bool CleanupTerminalActiveGroupRuntimeCrew(HST_ActiveGroupState activeGroup, string source)
	{
		if (!activeGroup || activeGroup.m_sGroupId.IsEmpty() || !IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;

		int removedHandles;
		int deletedGroupRoots;
		int deletedLivingMembers;
		int preservedDeadMembers;
		for (int i = m_aRuntimeGroupIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeGroupIds[i] != activeGroup.m_sGroupId)
				continue;

			IEntity entity;
			if (i < m_aRuntimeGroupEntities.Count())
				entity = m_aRuntimeGroupEntities[i];

			SCR_AIGroup group = SCR_AIGroup.Cast(entity);
			if (group)
			{
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
				deletedGroupRoots++;
			}
			else if (entity && IsLivingEntity(entity))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
				deletedLivingMembers++;
			}
			else if (entity)
			{
				preservedDeadMembers++;
			}

			if (i < m_aRuntimeGroupEntities.Count())
				m_aRuntimeGroupEntities.Remove(i);
			m_aRuntimeGroupIds.Remove(i);
			removedHandles++;
		}

		if (removedHandles <= 0)
			return false;

		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_sRuntimeEntityId = "";
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		DebugLog(string.Format("active group terminal crew cleanup %1 | status %2 | removed handles %3 | deleted roots %4 | deleted living %5 | preserved dead %6 | source %7", activeGroup.m_sGroupId, activeGroup.m_sRuntimeStatus, removedHandles, deletedGroupRoots, deletedLivingMembers, preservedDeadMembers, source));
		return true;
	}

	protected int UnregisterRuntimeCrewHandlesForRespawn(string groupId, string source)
	{
		if (groupId.IsEmpty())
			return 0;

		int removedHandles;
		int deletedGroupRoots;
		int deletedLivingMembers;
		int preservedDeadMembers;
		for (int i = m_aRuntimeGroupIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeGroupIds[i] != groupId)
				continue;

			IEntity entity;
			if (i < m_aRuntimeGroupEntities.Count())
				entity = m_aRuntimeGroupEntities[i];

			SCR_AIGroup group = SCR_AIGroup.Cast(entity);
			if (group)
			{
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
				deletedGroupRoots++;
			}
			else if (entity && IsLivingEntity(entity))
			{
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
				deletedLivingMembers++;
			}
			else if (entity)
			{
				preservedDeadMembers++;
			}

			if (i < m_aRuntimeGroupEntities.Count())
				m_aRuntimeGroupEntities.Remove(i);
			m_aRuntimeGroupIds.Remove(i);
			removedHandles++;
		}

		if (removedHandles > 0)
			DebugLog(string.Format("active group respawn crew handle cleanup %1 | removed handles %2 | deleted roots %3 | deleted living %4 | preserved dead %5 | source %6", groupId, removedHandles, deletedGroupRoots, deletedLivingMembers, preservedDeadMembers, source));
		return removedHandles;
	}

	protected bool EnsureRuntimeGroupEntities(HST_CampaignState state, HST_CampaignPreset preset = null)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup)
				continue;

			NormalizeStaticActiveGroupRoute(activeGroup);

			if (IsMissionConvoyGroup(activeGroup))
			{
				HST_ActiveMissionState mission = FindMissionForConvoyGroup(state, activeGroup);
				HST_MissionAssetState asset = FindMissionConvoyAssetForGroup(state, mission, activeGroup);
				if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
				{
					changed = CleanupTerminalActiveGroupRuntimeCrew(activeGroup, "ensure runtime") || changed;
					continue;
				}
				if (mission && asset && ShouldSpawnMissionConvoyRuntime(state, activeGroup))
					changed = TrySpawnMissionConvoyGroup(state, preset, mission, asset, activeGroup, ResolveMissionConvoyGroupIndex(mission, activeGroup.m_sGroupId)) || changed;
				continue;
			}

			if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
			{
				changed = CleanupTerminalActiveGroupRuntimeCrew(activeGroup, "ensure runtime") || changed;
				continue;
			}

			changed = ReconcileActiveGroupRuntimeMemberCounts(state, activeGroup, "ensure runtime") || changed;
			if (HasRuntimeGroupEntity(activeGroup.m_sGroupId))
				continue;

			if (activeGroup.m_iVehicleCount > 0 && activeGroup.m_iInfantryCount <= 0)
				changed = TrySpawnActiveVehicle(activeGroup, state) || changed;
			else
				changed = TrySpawnActiveGroup(activeGroup, state, preset) || changed;
		}

		return changed;
	}

	protected bool ReconcileActiveGroupRuntimeMemberCounts(HST_CampaignState state, HST_ActiveGroupState activeGroup, string source)
	{
		if (!state || !activeGroup || IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;
		if (!HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		if (activeGroup.m_sRuntimeStatus == "spawn_pending_agents")
		{
			string requestedStatus = ResolvePendingActiveGroupRequestedStatus(activeGroup, "");
			if (TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, source + " count reconcile"))
				return true;
		}

		int liveInfantry = CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId);
		int liveTotal = CountAliveRuntimeGroupAgents(activeGroup.m_sGroupId);
		if (liveTotal <= 0)
			return false;

		bool changed;
		SCR_AIGroup group = SCR_AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId));
		if (group)
			ReconcileRuntimeGroupEditableMembership(group, activeGroup, source + " count reconcile");

		if (!activeGroup.m_bSpawnedEntity)
		{
			activeGroup.m_bSpawnedEntity = true;
			activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
			if (activeGroup.m_sRuntimeStatus == "spawn_pending_agents")
				activeGroup.m_sRuntimeStatus = ResolveSpawnedRuntimeStatus(activeGroup, ResolvePendingActiveGroupRequestedStatus(activeGroup, ""));
			changed = true;
		}

		if (liveInfantry > activeGroup.m_iSpawnedAgentCount)
		{
			activeGroup.m_iSpawnedAgentCount = liveInfantry;
			changed = true;
		}

		if (activeGroup.m_iLastSeenAliveCount != liveTotal)
		{
			activeGroup.m_iLastSeenAliveCount = liveTotal;
			changed = true;
		}

		int survivorInfantry = Math.Min(Math.Max(0, activeGroup.m_iInfantryCount), liveInfantry);
		int survivorVehicles = 0;
		if (activeGroup.m_iVehicleCount > 0 && activeGroup.m_iInfantryCount <= 0)
			survivorVehicles = Math.Min(activeGroup.m_iVehicleCount, liveTotal);
		else if (activeGroup.m_iVehicleCount > 0)
			survivorVehicles = Math.Max(0, Math.Min(activeGroup.m_iVehicleCount, liveTotal - liveInfantry));

		if (activeGroup.m_iSurvivorInfantryCount != survivorInfantry)
		{
			activeGroup.m_iSurvivorInfantryCount = survivorInfantry;
			changed = true;
		}
		if (activeGroup.m_iSurvivorVehicleCount != survivorVehicles)
		{
			activeGroup.m_iSurvivorVehicleCount = survivorVehicles;
			changed = true;
		}

		if (changed)
		{
			RefreshActiveGroupZoneCounts(state, activeGroup);
			DebugLog(string.Format("active group count reconcile %1 via %2 | infantry live %3/%4 | total live %5 | spawned %6 | status %7", activeGroup.m_sGroupId, ReportText(source), liveInfantry, activeGroup.m_iInfantryCount, liveTotal, activeGroup.m_iSpawnedAgentCount, activeGroup.m_sRuntimeStatus));
		}

		return changed;
	}

	protected bool CleanupInactiveMissionOwnedActiveGroups(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			if (!activeGroup || activeGroup.m_sMissionInstanceId.IsEmpty() || IsMissionConvoyGroup(activeGroup))
				continue;

			HST_ActiveMissionState mission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
			if (mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			state.m_aActiveGroups.Remove(i);
			changed = true;
			DebugLog(string.Format("mission-owned active group %1 cleaned after mission %2 stopped being active", activeGroup.m_sGroupId, activeGroup.m_sMissionInstanceId));
		}

		return changed;
	}

	protected void NormalizeStaticActiveGroupRoute(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup || activeGroup.m_bQRF || IsMissionConvoyGroup(activeGroup))
			return;
		if (IsSupportRequestActiveGroup(activeGroup))
			return;
		if (activeGroup.m_sRuntimeStatus == "routing" || activeGroup.m_sRuntimeStatus == "support_active")
			return;
		if (activeGroup.m_sRouteId.IsEmpty())
			return;

		activeGroup.m_sRouteId = "";
		activeGroup.m_vSourcePosition = activeGroup.m_vPosition;
		activeGroup.m_vTargetPosition = activeGroup.m_vPosition;
	}

	protected bool IsPetrosAttackSupportGroup(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		return activeGroup.m_sSpawnFallbackMode.Contains("petros_attack_support");
	}

	protected bool IsSupportRequestActiveGroup(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		return activeGroup.m_sSpawnFallbackMode.Contains("support");
	}

	protected bool UpdateRuntimeGroupSurvivors(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || !activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed")
				continue;
			bool missionConvoyGroup = IsMissionConvoyGroup(activeGroup);
			if (missionConvoyGroup && ShouldSpawnMissionConvoyRuntime(state, activeGroup))
				continue;

			HST_ActiveMissionState convoyMission;
			if (missionConvoyGroup)
				convoyMission = FindMissionForConvoyGroup(state, activeGroup);
			if (missionConvoyGroup)
			{
				if (TryRepairMissionConvoyCrewPopulation(state, convoyMission, activeGroup, "survivor update"))
					changed = true;
			}
			else if (TryRepairEmptyRuntimeGroupPopulation(state, activeGroup, "survivor update"))
				changed = true;

			EnsureActiveGroupRuntimeFaction(activeGroup, "survivor update");
			EnforceOpposingOccupiedVehicleAccess(state, activeGroup);
			if (RefreshActiveGroupLivePositionFromRuntime(activeGroup, missionConvoyGroup))
				changed = true;
			int aliveCount;
			if (missionConvoyGroup)
				aliveCount = CountAliveRuntimeCrewAgents(activeGroup);
			else
				aliveCount = CountAliveRuntimeGroupAgents(activeGroup.m_sGroupId);
			if (missionConvoyGroup && aliveCount > 0)
				RecordConvoyCrewObservedAlive(activeGroup, aliveCount);
			if (missionConvoyGroup && aliveCount <= 0 && IsConvoyCrewControlPending(state, activeGroup))
				continue;
			if (aliveCount <= 0 && IsActiveGroupNativeDelayedPopulationActive(activeGroup))
			{
				DebugLog(string.Format("active group survivor update waiting for native delayed population %1 | zone %2 | status %3 | visual %4", activeGroup.m_sGroupId, activeGroup.m_sZoneId, activeGroup.m_sRuntimeStatus, BuildActiveGroupRuntimeVisualEvidence(activeGroup.m_sGroupId)));
				continue;
			}
			if (missionConvoyGroup && aliveCount <= 0 && activeGroup.m_iSpawnedAgentCount <= 0 && ResolveMissionConvoyRestorableCrewCount(state, activeGroup) > 0)
				continue;
			if (aliveCount <= 0 && IsActiveGroupLiveCountGraceActive(state, activeGroup))
			{
				DebugLog(string.Format("active group live-count grace %1 | zone %2 | spawned agents %3 | last alive %4 | status %5", activeGroup.m_sGroupId, activeGroup.m_sZoneId, activeGroup.m_iSpawnedAgentCount, activeGroup.m_iLastSeenAliveCount, activeGroup.m_sRuntimeStatus));
				continue;
			}
			if (aliveCount <= 0 && activeGroup.m_iSpawnedAgentCount <= 0 && (!missionConvoyGroup || activeGroup.m_iLastSeenAliveCount <= 0))
				continue;
			if (missionConvoyGroup && aliveCount <= 0 && !HasMissionConvoyExplicitEliminationContext(state, activeGroup))
			{
				string previousReason = activeGroup.m_sSpawnFailureReason;
				activeGroup.m_sSpawnFailureReason = "Convoy crew unobserved before explicit contact; awaiting population repair or physical contact proof.";
				activeGroup.m_sConvoyRuntimeStage = "CREW_UNOBSERVED";
				if (previousReason != activeGroup.m_sSpawnFailureReason)
					changed = true;
				continue;
			}
			if (aliveCount > 0 && activeGroup.m_iSpawnedAgentCount <= 0)
			{
				activeGroup.m_iSpawnedAgentCount = aliveCount;
				changed = true;
				DebugLog(string.Format("active group populated %1 | zone %2 | live agents %3 | expected infantry %4 vehicles %5 | status %6", activeGroup.m_sGroupId, activeGroup.m_sZoneId, aliveCount, activeGroup.m_iInfantryCount, activeGroup.m_iVehicleCount, activeGroup.m_sRuntimeStatus));
			}
			if (aliveCount == activeGroup.m_iLastSeenAliveCount && aliveCount > 0)
				continue;

			int previousAliveCount = activeGroup.m_iLastSeenAliveCount;
			activeGroup.m_iLastSeenAliveCount = aliveCount;
			if (missionConvoyGroup)
			{
				activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, CountAliveRuntimeCrewAgents(activeGroup));
				if (IsMissionConvoyGroupAssetTerminal(state, activeGroup))
					activeGroup.m_iSurvivorVehicleCount = 0;
				else
					activeGroup.m_iSurvivorVehicleCount = 1;
			}
			else if (activeGroup.m_iVehicleCount > 0 && activeGroup.m_iInfantryCount <= 0)
			{
				activeGroup.m_iSurvivorInfantryCount = 0;
				activeGroup.m_iSurvivorVehicleCount = Math.Min(activeGroup.m_iVehicleCount, aliveCount);
			}
			else
			{
				activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, aliveCount);
				activeGroup.m_iSurvivorVehicleCount = 0;
			}
			if (aliveCount <= 0)
			{
				if (missionConvoyGroup)
					activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
				else
					activeGroup.m_sRuntimeStatus = "eliminated";
				CleanupTerminalActiveGroupRuntimeCrew(activeGroup, "survivor update");
				if (activeGroup.m_bQRF || IsSupportRequestActiveGroup(activeGroup))
					m_bMarkerRefreshNeeded = true;
			}
			DebugLog(string.Format("active group survivors %1 | zone %2 | alive %3 from %4 | survivors infantry %5/%6 vehicles %7/%8 | status %9", activeGroup.m_sGroupId, activeGroup.m_sZoneId, aliveCount, previousAliveCount, activeGroup.m_iSurvivorInfantryCount, activeGroup.m_iInfantryCount, activeGroup.m_iSurvivorVehicleCount, activeGroup.m_iVehicleCount, activeGroup.m_sRuntimeStatus));
			changed = true;
		}

		if (changed)
		{
			foreach (HST_ZoneState zone : state.m_aZones)
			{
				if (zone && zone.m_bActive)
					ApplyActiveZoneCounts(state, zone);
			}
		}

		return changed;
	}

	protected bool RefreshActiveGroupLivePositionFromRuntime(HST_ActiveGroupState activeGroup, bool missionConvoyGroup)
	{
		if (!activeGroup)
			return false;

		vector livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, missionConvoyGroup);
		if (IsZeroVector(livePosition))
			return false;

		if (!IsZeroVector(activeGroup.m_vPosition) && DistanceSq2D(activeGroup.m_vPosition, livePosition) < 4.0)
			return false;

		activeGroup.m_vPosition = livePosition;
		if (activeGroup.m_bQRF || IsSupportRequestActiveGroup(activeGroup) || missionConvoyGroup)
			m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected vector ResolveActiveGroupLiveRuntimePosition(HST_ActiveGroupState activeGroup, bool missionConvoyGroup)
	{
		if (!activeGroup)
			return "0 0 0";

		array<IEntity> livingEntities = {};
		CollectLivingRuntimeGroupEntities(activeGroup.m_sGroupId, livingEntities);
		if (livingEntities.Count() > 0)
		{
			vector sum = "0 0 0";
			foreach (IEntity entity : livingEntities)
			{
				if (!entity)
					continue;
				sum = sum + entity.GetOrigin();
			}

			vector average = sum * (1.0 / livingEntities.Count());
			return HST_WorldPositionService.ResolveGroundPosition(average, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
		}

		IEntity vehicle = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		if (vehicle && IsLivingEntity(vehicle))
			return vehicle.GetOrigin();

		return "0 0 0";
	}

	protected void CollectLivingRuntimeGroupEntities(string groupId, array<IEntity> livingEntities)
	{
		if (groupId.IsEmpty() || !livingEntities)
			return;

		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity)
				continue;

			AIGroup group = AIGroup.Cast(entity);
			if (group)
			{
				CollectLivingNativeAIGroupEntities(group, livingEntities);
				continue;
			}

			if (IsLivingEntity(entity) && livingEntities.Find(entity) < 0)
				livingEntities.Insert(entity);
		}
	}

	protected bool EnforceOpposingOccupiedVehicleAccess(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup || IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;
		if (activeGroup.m_sFactionKey == "FIA" || activeGroup.m_sFactionKey == "CIV")
			return false;

		IEntity vehicle = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		if (!vehicle)
			return false;
		if (!IsLivingEntity(vehicle))
			return false;
		if (!HasLivingRuntimeGroupMemberInVehicle(activeGroup.m_sGroupId, vehicle))
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		bool ejected;
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;
			if (ResolveEntityVehicle(playerEntity) != vehicle)
				continue;

			SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(playerEntity.FindComponent(SCR_CompartmentAccessComponent));
			if (!access)
				continue;

			if (access.GetOutVehicle(EGetOutType.TELEPORT, -1, ECloseDoorAfterActions.INVALID, false, true))
			{
				ejected = true;
				DebugLog(string.Format("blocked player %1 from riding in occupied opposing vehicle for group %2 faction %3", playerId, activeGroup.m_sGroupId, activeGroup.m_sFactionKey));
			}
		}

		return ejected;
	}

	protected bool HasLivingRuntimeGroupMemberInVehicle(string groupId, IEntity vehicle)
	{
		if (groupId.IsEmpty() || !vehicle)
			return false;

		array<IEntity> livingEntities = {};
		CollectLivingRuntimeGroupEntities(groupId, livingEntities);
		foreach (IEntity entity : livingEntities)
		{
			if (!entity)
				continue;
			if (ResolveEntityVehicle(entity) == vehicle)
				return true;
		}

		return false;
	}

	protected bool IsActiveGroupLiveCountGraceActive(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return false;
		if (activeGroup.m_iSpawnedAgentCount <= 0 && activeGroup.m_iLastSeenAliveCount <= 0)
			return false;
		if (activeGroup.m_iSpawnedAtSecond < 0)
			return false;

		return state.m_iElapsedSeconds < activeGroup.m_iSpawnedAtSecond + ACTIVE_GROUP_LIVE_COUNT_GRACE_SECONDS;
	}

	protected int CountAliveRuntimeGroupAgents(string groupId)
	{
		array<IEntity> livingInfantry = {};
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity)
				continue;

			AIGroup group = AIGroup.Cast(entity);
			if (group)
			{
				CollectLivingNativeAIGroupEntities(group, livingInfantry);
				continue;
			}

			if (IsLivingEntity(entity) && livingInfantry.Find(entity) < 0)
				livingInfantry.Insert(entity);
		}

		int vehicleAliveCount;
		for (int j = 0; j < m_aRuntimeVehicleGroupIds.Count(); j++)
		{
			if (m_aRuntimeVehicleGroupIds[j] != groupId || j >= m_aRuntimeVehicleEntities.Count())
				continue;

			IEntity vehicle = m_aRuntimeVehicleEntities[j];
			if (IsLivingEntity(vehicle))
				vehicleAliveCount++;
		}

		return livingInfantry.Count() + vehicleAliveCount;
	}

	protected int CountAliveRuntimeInfantryGroupAgents(string groupId)
	{
		array<IEntity> livingInfantry = {};
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity)
				continue;

			AIGroup group = AIGroup.Cast(entity);
			if (group)
			{
				CollectLivingNativeAIGroupEntities(group, livingInfantry);
				continue;
			}

			if (IsLivingEntity(entity) && livingInfantry.Find(entity) < 0)
				livingInfantry.Insert(entity);
		}

		return livingInfantry.Count();
	}

	protected int CountAliveRuntimeNativeGroupAgents(string groupId)
	{
		int aliveCount;
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			AIGroup group = AIGroup.Cast(m_aRuntimeGroupEntities[i]);
			if (!group)
				continue;

			aliveCount += CountLivingNativeAIGroupAgents(group);
		}

		return aliveCount;
	}

	protected int CountLivingNativeAIGroupAgents(AIGroup group)
	{
		array<IEntity> livingEntities = {};
		CollectLivingNativeAIGroupEntities(group, livingEntities);
		return livingEntities.Count();
	}

	protected void CollectLivingNativeAIGroupEntities(AIGroup group, array<IEntity> livingEntities)
	{
		if (!group || !livingEntities)
			return;

		array<AIAgent> agents = new array<AIAgent>;
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity controlledEntity = agent.GetControlledEntity();
			if (IsLivingEntity(controlledEntity) && livingEntities.Find(controlledEntity) < 0)
				livingEntities.Insert(controlledEntity);
		}
	}

	protected bool HasActiveGarrisonGroup(HST_CampaignState state, HST_ZoneState zone)
	{
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_bQRF || IsMissionOwnedActiveGroup(activeGroup) || activeGroup.m_sZoneId != zone.m_sZoneId || activeGroup.m_sFactionKey != zone.m_sOwnerFactionKey)
				continue;
			if (activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == "folded")
				continue;

			if (activeGroup.m_iLastSeenAliveCount > 0 || activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iSurvivorVehicleCount > 0)
				return true;
		}

		return false;
	}

	protected void ApplyActiveZoneCounts(HST_CampaignState state, HST_ZoneState zone)
	{
		int infantryCount;
		int vehicleCount;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_bQRF || IsMissionOwnedActiveGroup(activeGroup) || activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;

			if (activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == "folded")
				continue;

			infantryCount += Math.Max(0, activeGroup.m_iSurvivorInfantryCount);
			vehicleCount += Math.Max(0, activeGroup.m_iSurvivorVehicleCount);
		}

		zone.m_iActiveInfantryCount = infantryCount;
		zone.m_iActiveVehicleCount = vehicleCount;
	}

	protected int CountPendingActiveZonePopulationInfantry(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return 0;

		int infantryCount;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_bQRF || IsMissionOwnedActiveGroup(activeGroup) || activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;
			if (activeGroup.m_sRuntimeStatus != "spawn_pending_agents")
				continue;

			infantryCount += Math.Max(0, activeGroup.m_iInfantryCount);
		}

		return infantryCount;
	}

	protected int CountPendingActiveZonePopulationGroups(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return 0;

		int groupCount;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_bQRF || IsMissionOwnedActiveGroup(activeGroup) || activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;
			if (activeGroup.m_sRuntimeStatus == "spawn_pending_agents")
				groupCount++;
		}

		return groupCount;
	}

	protected void FoldActiveGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		HST_GarrisonState garrison = state.FindGarrison(activeGroup.m_sZoneId, activeGroup.m_sFactionKey);
		if (!garrison)
		{
			garrison = new HST_GarrisonState();
			garrison.m_sZoneId = activeGroup.m_sZoneId;
			garrison.m_sFactionKey = activeGroup.m_sFactionKey;
			state.m_aGarrisons.Insert(garrison);
		}

		bool forceLost = activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED;
		int survivorInfantry;
		int survivorVehicles;
		if (!forceLost)
		{
			survivorInfantry = activeGroup.m_iSurvivorInfantryCount;
			survivorVehicles = activeGroup.m_iSurvivorVehicleCount;
			if (!activeGroup.m_bSpawnedEntity && survivorInfantry <= 0 && activeGroup.m_iInfantryCount > 0)
				survivorInfantry = activeGroup.m_iInfantryCount;
			if (!activeGroup.m_bSpawnedEntity && survivorVehicles <= 0 && activeGroup.m_iVehicleCount > 0)
				survivorVehicles = activeGroup.m_iVehicleCount;
		}

		string previousStatus = activeGroup.m_sRuntimeStatus;
		activeGroup.m_sRuntimeStatus = "folded";
		if (activeGroup.m_bQRF || IsSupportRequestActiveGroup(activeGroup))
			m_bMarkerRefreshNeeded = true;
		int returnedInfantry = Math.Max(0, survivorInfantry);
		HST_ZoneState zone = state.FindZone(activeGroup.m_sZoneId);
		if (zone && zone.m_iGarrisonSlots > 0)
			returnedInfantry = Math.Min(returnedInfantry, Math.Max(0, zone.m_iGarrisonSlots - garrison.m_iInfantryCount));
		int returnedVehicles = Math.Max(0, survivorVehicles);
		garrison.m_iInfantryCount += returnedInfantry;
		garrison.m_iVehicleCount += returnedVehicles;
		string foldedCounts = string.Format("returned infantry %1/%2 requested %3 vehicles %4/%5 | last alive %6 | spawned agents %7", returnedInfantry, activeGroup.m_iInfantryCount, survivorInfantry, returnedVehicles, activeGroup.m_iVehicleCount, activeGroup.m_iLastSeenAliveCount, activeGroup.m_iSpawnedAgentCount);
		DebugLog(string.Format("folded active group %1 | zone %2 | status %3 | %4", activeGroup.m_sGroupId, activeGroup.m_sZoneId, previousStatus, foldedCounts));
	}

	protected string ResolveGroupRouteId(HST_ZoneState zone, bool qrf)
	{
		if (!zone)
			return "";
		if (!qrf)
			return "";

		if (qrf && !zone.m_sQRFRouteId.IsEmpty())
			return zone.m_sQRFRouteId;

		return "";
	}

	protected vector ResolveGroupSourcePosition(HST_CampaignState state, HST_ZoneState zone, string routeId, bool qrf)
	{
		HST_GeneratedRouteState route;
		if (state && !routeId.IsEmpty())
			route = state.FindGeneratedRoute(routeId);

		if (route)
		{
			if (qrf)
				return route.m_vStartPosition;

			return route.m_vMidPosition;
		}

		if (zone)
			return zone.m_vPosition;

		return "0 0 0";
	}

	protected vector ResolveGroupTargetPosition(HST_CampaignState state, HST_ZoneState zone, string routeId, bool qrf)
	{
		HST_GeneratedRouteState route;
		if (state && !routeId.IsEmpty())
			route = state.FindGeneratedRoute(routeId);

		if (route)
			return route.m_vEndPosition;

		if (zone)
			return zone.m_vPosition;

		return "0 0 0";
	}

	protected bool HasRuntimeGroupEntity(string groupId)
	{
		return GetRuntimeGroupEntity(groupId) != null;
	}

	protected IEntity GetRuntimeGroupEntity(string groupId)
	{
		IEntity groupEntity = GetRuntimeCrewGroupEntity(groupId);
		if (groupEntity)
			return groupEntity;

		return GetRuntimeVehicleEntity(groupId);
	}

	protected void SetRuntimeGroupEntitiesOrigin(string groupId, vector position)
	{
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (entity)
				entity.SetOrigin(position);
		}
	}

	protected void DeleteRuntimeGroupEntity(string groupId, bool deleteVehicle = true)
	{
		DeleteRuntimeCrewEntities(groupId);

		for (int j = m_aRuntimeVehicleGroupIds.Count() - 1; j >= 0; j--)
		{
			if (m_aRuntimeVehicleGroupIds[j] != groupId)
				continue;

			IEntity vehicle;
			if (j < m_aRuntimeVehicleEntities.Count())
				vehicle = m_aRuntimeVehicleEntities[j];
			if (deleteVehicle && vehicle)
				SCR_EntityHelper.DeleteEntityAndChildren(vehicle);

			if (j < m_aRuntimeVehicleEntities.Count())
				m_aRuntimeVehicleEntities.Remove(j);
			m_aRuntimeVehicleGroupIds.Remove(j);
		}
	}

	protected void DeleteRuntimeCrewEntities(string groupId)
	{
		DeleteRuntimeGroupWaypoints(groupId);

		for (int i = m_aRuntimeGroupIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeGroupIds[i] != groupId)
				continue;

			IEntity entity;
			if (i < m_aRuntimeGroupEntities.Count())
				entity = m_aRuntimeGroupEntities[i];
			if (entity)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);

			if (i < m_aRuntimeGroupEntities.Count())
				m_aRuntimeGroupEntities.Remove(i);
			m_aRuntimeGroupIds.Remove(i);
		}
	}

	protected void DeleteRuntimeGroupWaypoints(string groupId)
	{
		if (groupId.IsEmpty())
			return;

		for (int waypointIndex = m_aRuntimeGroupWaypointIds.Count() - 1; waypointIndex >= 0; waypointIndex--)
		{
			if (m_aRuntimeGroupWaypointIds[waypointIndex] != groupId)
				continue;

			IEntity waypointEntity;
			if (waypointIndex < m_aRuntimeGroupWaypointEntities.Count())
				waypointEntity = m_aRuntimeGroupWaypointEntities[waypointIndex];
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);

			if (waypointIndex < m_aRuntimeGroupWaypointEntities.Count())
				m_aRuntimeGroupWaypointEntities.Remove(waypointIndex);
			m_aRuntimeGroupWaypointIds.Remove(waypointIndex);
		}
	}

	protected bool IsZoneInsideHQSafeArea(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, zone.m_vPosition) <= HQ_SAFE_RADIUS_METERS * HQ_SAFE_RADIUS_METERS;
	}

	protected bool IsInsideHQSafeRadius(HST_CampaignState state, vector position)
	{
		if (!state || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, position) <= HQ_SAFE_RADIUS_METERS * HQ_SAFE_RADIUS_METERS;
	}

	protected float ResolveNearestLivingPlayerDistanceMeters(vector position)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || IsZeroVector(position))
			return -1.0;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		float bestDistance = -1.0;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			float distance = Math.Sqrt(DistanceSq2D(playerEntity.GetOrigin(), position));
			if (bestDistance < 0 || distance < bestDistance)
				bestDistance = distance;
		}

		return bestDistance;
	}

	protected IEntity GetBestPlayerEntity(PlayerManager playerManager, int playerId)
	{
		if (!playerManager || playerId <= 0)
			return null;

		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (controlledEntity)
			return controlledEntity;

		return SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
	}

	protected bool IsLivingPlayerEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected HST_ConvoyVehicleControlAdapter GetConvoyVehicleControlAdapter()
	{
		if (!m_ConvoyVehicleControl)
			m_ConvoyVehicleControl = new HST_ConvoyVehicleControlAdapter();

		return m_ConvoyVehicleControl;
	}

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity)
			return false;

		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (character)
		{
			CharacterControllerComponent controller = character.GetCharacterController();
			if (controller)
				return controller.GetLifeState() != ECharacterLifeState.DEAD;
		}

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}

	protected void DebugLog(string message)
	{
		if (!m_bDebugLoggingEnabled)
			return;

		Print("h-istasi physical war debug | " + message);
	}
}
