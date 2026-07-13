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

class HST_ActiveGroupRouteProgressStatus
{
	string m_sGroupId;
	string m_sRouteStatus;
	vector m_vTargetPosition;
	vector m_vLastSamplePosition;
	float m_fLastDistanceToTargetMeters = -1.0;
	float m_fBestDistanceToTargetMeters = -1.0;
	int m_iLastSampleSecond = -1;
	int m_iLastProgressSecond = -1;
	int m_iLastRouteReissueSecond = -1;
	int m_iRouteReissueAttemptCount;
	int m_iArrivalSampleCount;
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

class HST_ExactMissionConvoyAssetProjectionSnapshot
{
	ref HST_MissionAssetState m_Asset;
	vector m_vCurrentPosition;
	vector m_vLastKnownPosition;
	string m_sCarriedByVehicleId;
	string m_sLastInteraction;
	bool m_bSpawned;
	bool m_bAlive;
	bool m_bAttachedToCarrier;
	ref HST_MissionRuntimeEntityState m_RuntimeEntity;
	string m_sRuntimeKind;
	string m_sRuntimePrefab;
	vector m_vRuntimePosition;
	vector m_vRuntimeAngles;
	bool m_bRuntimeEntityExisted;
	bool m_bRuntimeSpawned;
	bool m_bRuntimeDestroyed;
	bool m_bRuntimeRecovered;
}

class HST_ExactMissionConvoyRootProjectionSnapshot
{
	ref HST_ActiveGroupState m_Group;
	ref HST_ConvoyElementState m_Element;
	IEntity m_CrewRuntimeBefore;
	IEntity m_VehicleRuntimeBefore;
	string m_sGroupPrefab;
	string m_sGroupSpawnFallbackMode;
	string m_sGroupSpawnFailureReason;
	string m_sGroupRuntimeEntityId;
	string m_sGroupRuntimeStatus;
	string m_sGroupCrewPopulationFailureReason;
	string m_sGroupConvoyRuntimeStage;
	vector m_vGroupPosition;
	vector m_vGroupSourcePosition;
	int m_iGroupInfantryCount;
	int m_iGroupSpawnedAtSecond;
	int m_iGroupLastSeenAliveCount;
	int m_iGroupSurvivorInfantryCount;
	int m_iGroupSurvivorVehicleCount;
	int m_iGroupSpawnedAgentCount;
	int m_iGroupAssignedWaypointCount;
	int m_iGroupMaxObservedCrewAlive;
	int m_iGroupDurableLivingInfantryCount;
	int m_iGroupLifecycleRevision;
	bool m_bGroupEverHadLivingCrew;
	bool m_bGroupEverPopulated;
	bool m_bGroupSpawnCompleted;
	bool m_bGroupCrewPopulationTerminallyFailed;
	bool m_bGroupSpawnAttempted;
	bool m_bGroupSpawnedEntity;
	vector m_vElementPosition;
	string m_sElementTerminalReason;
	int m_iElementSurvivingCrewCount;
	int m_iElementLastUpdatedSecond;
	int m_iElementRevision;
	float m_fElementVehicleDamageFraction;
	float m_fElementFuelFraction;
	float m_fElementAmmoFraction;
	HST_EConvoyElementDisposition m_eElementDisposition;
	bool m_bElementPhysicalized;
	bool m_bElementMobile;
}

class HST_ExactMissionConvoyOutboundProjectionTransaction
{
	ref HST_CampaignState m_State;
	ref HST_ActiveMissionState m_Mission;
	string m_sMissionInstanceId;
	string m_sOriginalMissionFailureReason;
	string m_sTerminalFailureReason;
	int m_iStartedAtSecond;
	bool m_bRolledBackTerminally;
	ref array<ref HST_ExactMissionConvoyAssetProjectionSnapshot> m_aAssetSnapshots = {};
	ref array<ref HST_ExactMissionConvoyRootProjectionSnapshot> m_aRootSnapshots = {};
}

class HST_ExactMissionConvoyRosterMutationPlan
{
	ref HST_CampaignState m_State;
	ref HST_ActiveMissionState m_Mission;
	ref HST_ForceManifestState m_Manifest;
	ref HST_ForceSpawnResultState m_Batch;
	ref array<ref HST_ConvoyElementState> m_aElements = {};
	ref array<ref HST_ActiveGroupState> m_aGroups = {};
	ref array<int> m_aSurvivorCounts = {};
	ref array<ref HST_ForceSpawnSlotResultState> m_aNewCasualtySlots = {};
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
	static const int QRF_CHANCE_REJECT_COOLDOWN_SECONDS = 300;
	static const int ROUTE_STATE_UPDATE_SECONDS = 5;
	static const float HQ_SAFE_RADIUS_METERS = 900;
	static const float HQ_ZONE_ACTIVATION_FALLBACK_RADIUS_METERS = 150.0;
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
	static const int EXACT_MISSION_CONVOY_VEHICLE_COUNT = 3;
	static const string MISSION_PRIMITIVE_CLEAR_AREA = "clear_area";
	static const string MISSION_PRIMITIVE_HOLD_AREA = "hold_area";
	static const string CONVOY_COMPLETE_EVENT_KEY = "convoy_complete";
	static const string CONVOY_FAIL_EVENT_KEY = "convoy_failed";
	static const string CONVOY_CONTACT_CLEAR_EVENT_KEY = "convoy_contact_cleared";
	static const string CONVOY_MOVE_EVENT_PENDING = "convoy_moving_pending";
	static const string CONVOY_MOVE_EVENT_SENT = "convoy_moving_sent";
	static const string FIA_CAMPAIGN_FACTION_CONFIG = "Configs/Factions/FIA_Campaign.conf";
	static const string US_CAMPAIGN_FACTION_CONFIG = "{ADFDBDA163950168}Configs/Factions/US_Campaign.conf";
	static const string USSR_CAMPAIGN_FACTION_CONFIG = "{15B582F8FA0B0940}Configs/Factions/USSR_Campaign.conf";
	static const float CONVOY_CONTACT_RADIUS_METERS = 120.0;
	static const int EXACT_CONVOY_CONTACT_CLEAR_SECONDS = 30;
	static const float CONVOY_DESTINATION_RADIUS_METERS = 50.0;
	static const int CONVOY_PROGRESS_SYNC_SECONDS = 5;
	static const int CONVOY_MARKER_REFRESH_SECONDS = 30;
	static const float CONVOY_PROGRESS_THRESHOLD_METERS = 8.0;
	static const int CONVOY_ROUTE_REISSUE_THRESHOLD_SECONDS = 45;
	static const int CONVOY_HARD_STUCK_THRESHOLD_SECONDS = 120;
	static const int CONVOY_TERMINAL_STUCK_THRESHOLD_SECONDS = 300;
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
	static const float ACTIVE_GROUP_MEMBER_REPAIR_RADIUS_METERS = 160.0;
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
	static const string TOWN_POLICE_PATROL_CYCLE_WAYPOINT_PREFAB = "{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et";
	static const string TOWN_SECURITY_POLICE_PROJECTION_TOKEN = "town_security_police";
	static const int CAMPAIGN_DEBUG_COMBAT_PROBE_SAMPLE_SECONDS = 45;
	static const int CAMPAIGN_DEBUG_COMBAT_PROBE_INFANTRY_COUNT = 4;
	static const int TOWN_SECURITY_POLICE_MIN_INFANTRY = 2;
	static const int TOWN_SECURITY_POLICE_MAX_INFANTRY = 5;
	static const float CAMPAIGN_DEBUG_COMBAT_PROBE_PLAYER_OFFSET_METERS = 90.0;
	static const float CAMPAIGN_DEBUG_COMBAT_PROBE_SEPARATION_METERS = 36.0;
	static const float CAMPAIGN_DEBUG_COMBAT_PROBE_CONTACT_METERS = 70.0;
	static const float CAMPAIGN_DEBUG_COMBAT_WAYPOINT_RADIUS_METERS = 18.0;
	static const float ACTIVE_GROUP_ROUTE_WAYPOINT_RADIUS_METERS = 35.0;
	static const float ACTIVE_GROUP_ROUTE_SWEEP_WAYPOINT_RADIUS_METERS = 55.0;
	static const float ACTIVE_GROUP_ROUTE_ARRIVAL_RADIUS_METERS = 75.0;
	static const float ACTIVE_GROUP_ROUTE_PROGRESS_THRESHOLD_METERS = 8.0;
	static const int ACTIVE_GROUP_ROUTE_STALL_REISSUE_SECONDS = 45;
	static const int ACTIVE_GROUP_ROUTE_REISSUE_COOLDOWN_SECONDS = 30;
	static const int ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS = 3;
	static const int ACTIVE_GROUP_ROUTE_ARRIVAL_SAMPLE_COUNT = 2;
	static const float TOWN_POLICE_PATROL_WAYPOINT_RADIUS_METERS = 12.0;
	static const float TOWN_POLICE_PATROL_FALLBACK_RADIUS_METERS = 75.0;

	protected ref array<string> m_aRuntimeGroupIds = {};
	protected ref array<IEntity> m_aRuntimeGroupEntities = {};
	protected ref map<string, ref array<IEntity>> m_mCombatPresenceRuntimeByGroup
		= new map<string, ref array<IEntity>>();
	protected ref array<string> m_aCombatPresenceRuntimeGroupKeys = {};
	protected ref array<IEntity> m_aCombatPresenceRegisteredMemberScratch = {};
	protected ref array<IEntity> m_aCombatPresenceCountedPlatformScratch = {};
	protected ref array<AIAgent> m_aCombatPresenceAgentScratch = {};
	protected ref HST_CombatPresenceService m_CombatPresence = new HST_CombatPresenceService();
	protected int m_iCombatPresenceRuntimeTopologySignature;
	protected ref array<string> m_aForceSpawnOwnedGroupIds = {};
	protected ref array<string> m_aForceSpawnOwnedResultIds = {};
	protected ref array<IEntity> m_aRuntimeMemberRepairCandidates = {};
	protected ref array<string> m_aRuntimeGroupWaypointIds = {};
	protected ref array<IEntity> m_aRuntimeGroupWaypointEntities = {};
	protected ref array<string> m_aPendingPopulationGroupIds = {};
	protected ref array<string> m_aPendingPopulationRequestedStatuses = {};
	protected ref array<ref HST_ActiveGroupState> m_aPendingPopulationActiveGroups = {};
	protected ref array<ref HST_CampaignState> m_aPendingPopulationStates = {};
	protected ref array<string> m_aRuntimeVehicleGroupIds = {};
	protected ref array<IEntity> m_aRuntimeVehicleEntities = {};
	protected ref array<ref HST_ExactMissionConvoyOutboundProjectionTransaction> m_aExactMissionConvoyOutboundProjectionTransactions = {};
	protected ref array<string> m_aExactMissionConvoyMemberMissionIds = {};
	protected ref array<string> m_aExactMissionConvoyMemberGroupIds = {};
	protected ref array<string> m_aExactMissionConvoyMemberSlotIds = {};
	protected ref array<IEntity> m_aExactMissionConvoyMemberEntities = {};
	protected ref array<string> m_aExactMissionConvoySettledSalvageMissionIds = {};
	protected ref array<string> m_aExactMissionConvoySettledSalvageAssetIds = {};
	protected ref array<string> m_aExactMissionConvoySettledSalvageGroupIds = {};
	protected ref array<IEntity> m_aExactMissionConvoySettledSalvageEntities = {};
	protected ref array<ref HST_ConvoyProgressStatus> m_aConvoyProgressStatuses = {};
	protected ref array<ref HST_ActiveGroupRouteProgressStatus> m_aActiveGroupRouteProgressStatuses = {};
	protected ref array<string> m_aRestoredMissionConvoyRebuildGroupIds = {};
	protected ref array<string> m_aVehicleSpawnBlockedZoneIds = {};
	protected ref array<string> m_aVehicleSpawnBlockedReasons = {};
	protected ref array<string> m_aDebugThrottleKeys = {};
	protected ref array<int> m_aDebugThrottleTicks = {};
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

	void SetCombatPresenceService(HST_CombatPresenceService combatPresence)
	{
		if (combatPresence)
			m_CombatPresence = combatPresence;
	}

	bool HasCombatPresenceRuntimeTopologyChangedSinceLastSample()
	{
		return BuildCombatPresenceRuntimeTopologySignature()
			!= m_iCombatPresenceRuntimeTopologySignature;
	}

	bool RefreshCombatPresenceSamples(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		int sampleSecond = Math.Max(0, state.m_iElapsedSeconds);
		BuildCombatPresenceRuntimeRegistrationIndex();
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup)
				continue;

			if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
			{
				changed = ApplyCombatPresenceSample(activeGroup, 0, 0, 0, -1, false, "terminal_runtime") || changed;
				continue;
			}
			if (!state.IsOperationalActiveGroup(activeGroup))
			{
				changed = ApplyCombatPresenceSample(activeGroup, 0, 0, 0, -1, false, "non_operational") || changed;
				continue;
			}
			if (!activeGroup.m_bSpawnedEntity)
			{
				changed = ApplyCombatPresenceSample(activeGroup, 0, 0, 0, -1, false, "runtime_not_spawned") || changed;
				continue;
			}
			array<IEntity> registeredRuntime = m_mCombatPresenceRuntimeByGroup.Get(activeGroup.m_sGroupId);
			if (!registeredRuntime || registeredRuntime.Count() <= 0)
			{
				changed = ApplyCombatPresenceSample(activeGroup, 0, 0, 0, -1, false, "runtime_unregistered") || changed;
				continue;
			}

			int infantryCount;
			int vehicleCount;
			int staticOperatorCount;
			bool observedCharacterProjection;
			ResolveRegisteredCombatPresenceSample(
				activeGroup,
				registeredRuntime,
				infantryCount,
				vehicleCount,
				staticOperatorCount,
				observedCharacterProjection);
			if (!observedCharacterProjection
				&& IsCombatPresencePopulationAuthorityPending(state, activeGroup))
			{
				changed = ApplyCombatPresenceSample(
					activeGroup,
					0,
					0,
					0,
					-1,
					false,
					"runtime_population_pending") || changed;
				continue;
			}
			changed = ApplyCombatPresenceSample(activeGroup, infantryCount, vehicleCount, staticOperatorCount,
				sampleSecond, true, "registered_runtime_members") || changed;
		}

		m_iCombatPresenceRuntimeTopologySignature = BuildCombatPresenceRuntimeTopologySignature();
		return changed;
	}

	protected int BuildCombatPresenceRuntimeTopologySignature()
	{
		int pairedCount = Math.Min(m_aRuntimeGroupIds.Count(), m_aRuntimeGroupEntities.Count());
		int signature = pairedCount + 17;
		for (int topologyIndex; topologyIndex < pairedCount; topologyIndex++)
		{
			string groupId = m_aRuntimeGroupIds[topologyIndex];
			IEntity entity = m_aRuntimeGroupEntities[topologyIndex];
			signature = signature * 31 + groupId.Hash();
			if (!entity)
			{
				signature = signature * 31 - 1;
				continue;
			}
			signature = signature * 31 + entity.GetID().ToString().Hash();
			if (entity.IsDeleted())
				signature = signature * 31 + 1;
			SCR_AIGroup group = SCR_AIGroup.Cast(entity);
			if (group)
				signature = signature * 31 + group.GetPlayerAndAgentCount();
		}
		return signature;
	}

	protected bool ApplyCombatPresenceSample(
		HST_ActiveGroupState activeGroup,
		int infantryCount,
		int vehicleCount,
		int staticOperatorCount,
		int sampleSecond,
		bool authoritative,
		string reason)
	{
		if (!activeGroup)
			return false;

		infantryCount = Math.Max(0, infantryCount);
		vehicleCount = Math.Max(0, vehicleCount);
		staticOperatorCount = Math.Max(0, staticOperatorCount);
		bool changed = activeGroup.m_iCombatEffectiveInfantryCount != infantryCount
			|| activeGroup.m_iOperationalMannedVehicleCount != vehicleCount
			|| activeGroup.m_iCombatEffectiveStaticOperatorCount != staticOperatorCount
			|| activeGroup.m_bCombatPresenceSampleAuthoritative != authoritative
			|| activeGroup.m_sCombatPresenceSampleReason != reason;

		// Authoritative sample freshness is runtime-only bookkeeping. Refresh it on
		// every sample without turning a timestamp-only refresh into save-dirty state.
		if (authoritative)
			activeGroup.m_iCombatPresenceSampleSecond = Math.Max(0, sampleSecond);
		else
			activeGroup.m_iCombatPresenceSampleSecond = -1;

		if (!changed)
			return false;

		activeGroup.m_iCombatEffectiveInfantryCount = infantryCount;
		activeGroup.m_iOperationalMannedVehicleCount = vehicleCount;
		activeGroup.m_iCombatEffectiveStaticOperatorCount = staticOperatorCount;
		activeGroup.m_bCombatPresenceSampleAuthoritative = authoritative;
		activeGroup.m_sCombatPresenceSampleReason = reason;
		return true;
	}

	protected void BuildCombatPresenceRuntimeRegistrationIndex()
	{
		foreach (string existingGroupId : m_aCombatPresenceRuntimeGroupKeys)
		{
			array<IEntity> existingRuntime = m_mCombatPresenceRuntimeByGroup.Get(existingGroupId);
			if (existingRuntime)
				existingRuntime.Clear();
		}

		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (i >= m_aRuntimeGroupEntities.Count())
				continue;
			string groupId = m_aRuntimeGroupIds[i];
			IEntity entity = m_aRuntimeGroupEntities[i];
			if (groupId.IsEmpty() || !entity || entity.IsDeleted())
				continue;

			array<IEntity> registeredRuntime = m_mCombatPresenceRuntimeByGroup.Get(groupId);
			if (!registeredRuntime)
			{
				registeredRuntime = {};
				m_mCombatPresenceRuntimeByGroup.Set(groupId, registeredRuntime);
				m_aCombatPresenceRuntimeGroupKeys.Insert(groupId);
			}
			if (registeredRuntime.Find(entity) < 0)
				registeredRuntime.Insert(entity);
		}

		for (int keyIndex = m_aCombatPresenceRuntimeGroupKeys.Count() - 1; keyIndex >= 0; keyIndex--)
		{
			string indexedGroupId = m_aCombatPresenceRuntimeGroupKeys[keyIndex];
			array<IEntity> indexedRuntime = m_mCombatPresenceRuntimeByGroup.Get(indexedGroupId);
			if (indexedRuntime && indexedRuntime.Count() > 0)
				continue;
			m_mCombatPresenceRuntimeByGroup.Remove(indexedGroupId);
			m_aCombatPresenceRuntimeGroupKeys.Remove(keyIndex);
		}
	}

	protected void ResolveRegisteredCombatPresenceSample(
		HST_ActiveGroupState activeGroup,
		array<IEntity> registeredRuntime,
		out int infantryCount,
		out int vehicleCount,
		out int staticOperatorCount,
		out bool observedCharacterProjection)
	{
		infantryCount = 0;
		vehicleCount = 0;
		staticOperatorCount = 0;
		observedCharacterProjection = false;
		if (!activeGroup || !registeredRuntime)
			return;

		m_aCombatPresenceRegisteredMemberScratch.Clear();
		m_aCombatPresenceCountedPlatformScratch.Clear();
		CollectRegisteredCombatPresenceMembers(registeredRuntime);
		foreach (IEntity member : m_aCombatPresenceRegisteredMemberScratch)
		{
			SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(member);
			if (!character)
				continue;
			observedCharacterProjection = true;

			CharacterControllerComponent controller = character.GetCharacterController();
			if (!controller || controller.GetLifeState() != ECharacterLifeState.ALIVE || controller.IsUnconscious())
				continue;

			CompartmentAccessComponent access = character.GetCompartmentAccessComponent();
			BaseCompartmentSlot compartment;
			if (access)
				compartment = access.GetCompartment();
			if (!compartment)
			{
				infantryCount++;
				continue;
			}

			if (CargoCompartmentSlot.Cast(compartment))
				continue;

			bool turretOccupant = TurretCompartmentSlot.Cast(compartment) != null;
			bool pilotOccupant = PilotCompartmentSlot.Cast(compartment) != null;
			if (!turretOccupant && !pilotOccupant)
				continue;

			IEntity compartmentOwner = compartment.GetOwner();
			if (!compartmentOwner)
				continue;

			IEntity platform;
			SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.FindOnNearestParent(compartmentOwner, platform);
			if (!usage || !platform || !usage.IsVehicleTypeValid()
				|| m_aCombatPresenceCountedPlatformScratch.Find(platform) >= 0)
				continue;

			EAIVehicleType vehicleType = usage.GetVehicleType();
			bool staticPlatform = vehicleType == EAIVehicleType.STATIC_WEAPON
				|| vehicleType == EAIVehicleType.STATIC_ARTILLERY;
			if (!IsCombatPresencePlatformOperational(platform, !staticPlatform))
				continue;

			if (staticPlatform)
			{
				if (!turretOccupant)
					continue;

				m_aCombatPresenceCountedPlatformScratch.Insert(platform);
				staticOperatorCount++;
				continue;
			}

			bool compositionProvesArmed = activeGroup.m_iCompositionVehicleCount > 0
				&& activeGroup.m_iCompositionArmedVehicleCount >= activeGroup.m_iCompositionVehicleCount;
			bool armedMobilePlatform = turretOccupant || (pilotOccupant && compositionProvesArmed);
			if (!armedMobilePlatform)
				continue;

			m_aCombatPresenceCountedPlatformScratch.Insert(platform);
			vehicleCount++;
		}
	}

	protected bool IsCombatPresencePopulationAuthorityPending(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return false;
		int expectedPersonnel = Math.Max(
			Math.Max(0, activeGroup.m_iInfantryCount),
			Math.Max(0, activeGroup.m_iOriginalInfantryCount));
		expectedPersonnel = Math.Max(expectedPersonnel, Math.Max(0, activeGroup.m_iCompositionManpower));
		if (expectedPersonnel <= 0)
			return false;
		if (activeGroup.m_sRuntimeStatus == "spawn_pending_agents")
			return true;
		if (IsActiveGroupNativeDelayedPopulationActive(activeGroup))
			return true;
		if (IsActiveGroupLiveCountGraceActive(state, activeGroup))
			return true;
		if (state.m_bRestoredFromPersistence
			&& state.m_iElapsedSeconds <= state.m_iLastRestoreSecond + ACTIVE_GROUP_LIVE_COUNT_GRACE_SECONDS
			&& (activeGroup.m_iSpawnedAgentCount > 0
				|| activeGroup.m_iLastSeenAliveCount > 0
				|| activeGroup.m_iDurableLivingInfantryCount > 0))
			return true;
		return !activeGroup.m_bEverPopulated || !activeGroup.m_bSpawnCompleted;
	}

	protected void CollectRegisteredCombatPresenceMembers(
		array<IEntity> registeredRuntime)
	{
		if (!registeredRuntime)
			return;

		foreach (IEntity entity : registeredRuntime)
		{
			if (!entity || entity.IsDeleted())
				continue;

			AIGroup group = AIGroup.Cast(entity);
			if (!group)
			{
				if (m_aCombatPresenceRegisteredMemberScratch.Find(entity) < 0)
					m_aCombatPresenceRegisteredMemberScratch.Insert(entity);
				continue;
			}

			m_aCombatPresenceAgentScratch.Clear();
			group.GetAgents(m_aCombatPresenceAgentScratch);
			foreach (AIAgent agent : m_aCombatPresenceAgentScratch)
			{
				if (!agent)
					continue;

				IEntity controlledEntity = agent.GetControlledEntity();
				if (!controlledEntity || controlledEntity.IsDeleted()
					|| m_aCombatPresenceRegisteredMemberScratch.Find(controlledEntity) >= 0)
					continue;
				m_aCombatPresenceRegisteredMemberScratch.Insert(controlledEntity);
			}
		}
	}

	protected bool IsCombatPresencePlatformOperational(IEntity platform, bool requireMovement)
	{
		if (!platform || platform.IsDeleted())
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.GetDamageManager(platform);
		if (!damageManager || damageManager.GetState() == EDamageState.DESTROYED)
			return false;
		if (SCR_AIVehicleUsability.VehicleIsOnFire(platform, damageManager))
			return false;
		if (requireMovement && !SCR_AIVehicleUsability.VehicleCanMove(platform, damageManager))
			return false;

		return true;
	}

	bool IsForceSpawnQueueManaged(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup || activeGroup.m_sSpawnResultId.IsEmpty())
			return false;
		if (IsMissionConvoyGroup(activeGroup) && !activeGroup.m_sConvoyElementId.IsEmpty())
			return false;

		return true;
	}

	bool ShouldHoldForceSpawnProjection(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!IsForceSpawnQueueManaged(activeGroup))
			return false;

		string identityFailure;
		HST_ForceSpawnResultState batch = ValidateForceSpawnProjectionIdentity(state, activeGroup, identityFailure);
		if (!batch)
			return true;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
			return false;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			return true;

		return !activeGroup.m_bSpawnedEntity || GetForceSpawnGroupRoot(activeGroup) == null;
	}

	bool CanSpawnForceSpawnGroupMember(HST_ActiveGroupState activeGroup, out string reason)
	{
		reason = "";
		if (!IsForceSpawnQueueManaged(activeGroup))
		{
			reason = "active group is not owned by the exact force spawn queue";
			return false;
		}

		return EnsureForceSpawnNextMemberAIWorldBudget(activeGroup, reason);
	}

	bool TryRegisterForceSpawnGroupRoot(HST_CampaignState state, HST_ActiveGroupState activeGroup, SCR_AIGroup root, out string reason)
	{
		reason = "";
		HST_ForceSpawnResultState batch = ValidateForceSpawnProjectionIdentity(state, activeGroup, reason);
		if (!batch)
			return false;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
		{
			reason = "force spawn group root registration requires an in-progress batch";
			return false;
		}
		if (!ValidateForceSpawnGroupRoot(activeGroup, root, reason))
			return false;

		SCR_AIGroup registeredRoot = GetForceSpawnGroupRoot(activeGroup);
		if (registeredRoot)
		{
			if (registeredRoot != root)
			{
				reason = "force spawn projection already owns another runtime group root";
				return false;
			}
			if (!AcquireForceSpawnRuntimeOwnership(activeGroup, reason))
				return false;
			reason = "force spawn group root was already registered";
			return true;
		}
		if (IsRuntimeHandleTrackedByAnotherGroup(activeGroup.m_sGroupId, root))
		{
			reason = "force spawn group root is already registered under another active group";
			return false;
		}
		bool ownershipAlreadyHeld = IsForceSpawnRuntimeOwnershipHeld(activeGroup);
		if (!AcquireForceSpawnRuntimeOwnership(activeGroup, reason))
			return false;

		PrepareForceSpawnGroupRoot(root);
		if (!RegisterRuntimeGroupEntityHandle(activeGroup.m_sGroupId, root))
		{
			if (!ownershipAlreadyHeld)
				ReleaseForceSpawnRuntimeOwnership(activeGroup);
			reason = "force spawn group root could not be registered in physical runtime ownership";
			return false;
		}

		ApplyCampaignDebugEntityName(root, "force_spawn_group", activeGroup.m_sGroupId);
		return true;
	}

	bool TryRegisterForceSpawnGroupMember(HST_CampaignState state, HST_ActiveGroupState activeGroup, SCR_AIGroup root, IEntity member, int ordinal, out string reason)
	{
		reason = "";
		HST_ForceSpawnResultState batch = ValidateForceSpawnProjectionIdentity(state, activeGroup, reason);
		if (!batch)
			return false;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
		{
			reason = "force spawn member registration requires an in-progress batch";
			return false;
		}
		if (!IsForceSpawnRuntimeOwnershipHeld(activeGroup))
		{
			reason = "force spawn runtime ownership was not acquired by the group root";
			return false;
		}
		if (GetForceSpawnGroupRoot(activeGroup) != root)
		{
			reason = "force spawn member does not reference the exact registered group root";
			return false;
		}
		if (IsRuntimeHandleTrackedByAnotherGroup(activeGroup.m_sGroupId, member))
		{
			reason = "force spawn member is already registered under another active group";
			return false;
		}
		if (IsRuntimeGroupEntityHandleTracked(activeGroup.m_sGroupId, member))
		{
			if (!ValidateForceSpawnGroupMember(activeGroup, root, member, ordinal, reason))
				return false;
			reason = "force spawn member was already registered";
			return true;
		}
		if (!AttachForceSpawnGroupMember(activeGroup, root, member, ordinal, reason))
			return false;
		if (!ValidateForceSpawnGroupMember(activeGroup, root, member, ordinal, reason))
		{
			DetachForceSpawnMember(activeGroup, member);
			return false;
		}

		DeactivateForceSpawnMember(member);
		if (!RegisterRuntimeGroupEntityHandle(activeGroup.m_sGroupId, member))
		{
			DetachForceSpawnMember(activeGroup, member);
			reason = "force spawn member could not be registered in physical runtime ownership";
			return false;
		}

		ApplyCampaignDebugEntityName(member, string.Format("force_spawn_member_%1", ordinal), activeGroup.m_sGroupId);
		return true;
	}

	SCR_AIGroup GetForceSpawnGroupRoot(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup || activeGroup.m_sGroupId.IsEmpty())
			return null;

		SCR_AIGroup match;
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != activeGroup.m_sGroupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			SCR_AIGroup candidate = SCR_AIGroup.Cast(m_aRuntimeGroupEntities[i]);
			if (!candidate || candidate.IsDeleted())
				continue;
			if (match && match != candidate)
				return null;
			match = candidate;
		}

		return match;
	}

	bool IsForceSpawnRuntimeHandleRegistered(HST_ActiveGroupState activeGroup, IEntity entity)
	{
		return activeGroup && IsRuntimeGroupEntityHandleTracked(activeGroup.m_sGroupId, entity);
	}

	bool TryUnregisterForceSpawnGroupMember(HST_ActiveGroupState activeGroup, IEntity member, bool deleteEntity, out string reason)
	{
		reason = "";
		if (!ValidateForceSpawnCleanupOwnership(activeGroup, reason))
			return false;
		if (!member)
		{
			reason = "force spawn member was already absent";
			return true;
		}
		if (SCR_AIGroup.Cast(member))
		{
			reason = "force spawn member cleanup received a group root";
			return false;
		}
		if (member.IsDeleted())
		{
			RemoveRuntimeGroupEntityHandleExact(activeGroup.m_sGroupId, member);
			reason = "force spawn member was already deleted";
			return true;
		}
		if (!IsRuntimeGroupEntityHandleTracked(activeGroup.m_sGroupId, member))
		{
			if (IsRuntimeHandleTrackedByAnotherGroup(activeGroup.m_sGroupId, member))
			{
				reason = "force spawn member is owned by another active group";
				return false;
			}
			if (deleteEntity && !member.IsDeleted())
				SCR_EntityHelper.DeleteEntityAndChildren(member);
			reason = "force spawn member was already unregistered";
			return true;
		}

		DetachForceSpawnMember(activeGroup, member);
		RemoveRuntimeGroupEntityHandleExact(activeGroup.m_sGroupId, member);
		if (deleteEntity && member)
			SCR_EntityHelper.DeleteEntityAndChildren(member);
		return true;
	}

	bool TryUnregisterForceSpawnGroupRoot(HST_ActiveGroupState activeGroup, SCR_AIGroup root, bool deleteEntity, out string reason)
	{
		reason = "";
		if (!ValidateForceSpawnCleanupOwnership(activeGroup, reason))
			return false;
		if (CountForceSpawnRuntimeMembers(activeGroup) > 0)
		{
			reason = "force spawn group root cleanup requires all exact members to be removed first";
			return false;
		}
		if (root && root.IsDeleted())
		{
			DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);
			RemoveRuntimeGroupEntityHandleExact(activeGroup.m_sGroupId, root);
			reason = "force spawn group root was already deleted";
			return true;
		}

		SCR_AIGroup registeredRoot = GetForceSpawnGroupRoot(activeGroup);
		if (!root && !registeredRoot)
		{
			reason = "force spawn group root was already absent";
			return true;
		}
		if (registeredRoot && registeredRoot != root)
		{
			reason = "force spawn group root cleanup does not match the registered root";
			return false;
		}
		if (!registeredRoot)
		{
			if (IsRuntimeHandleTrackedByAnotherGroup(activeGroup.m_sGroupId, root))
			{
				reason = "force spawn group root is owned by another active group";
				return false;
			}
			if (deleteEntity && root && !root.IsDeleted())
				SCR_EntityHelper.DeleteEntityAndChildren(root);
			reason = "force spawn group root was already unregistered";
			return true;
		}

		DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);
		RemoveRuntimeGroupEntityHandleExact(activeGroup.m_sGroupId, registeredRoot);
		if (deleteEntity && registeredRoot)
			SCR_EntityHelper.DeleteEntityAndChildren(registeredRoot);
		return true;
	}

	int CountForceSpawnRuntimeMembers(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup || activeGroup.m_sGroupId.IsEmpty())
			return 0;

		array<IEntity> uniqueMembers = {};
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != activeGroup.m_sGroupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity || entity.IsDeleted() || SCR_AIGroup.Cast(entity) || uniqueMembers.Contains(entity))
				continue;
			uniqueMembers.Insert(entity);
		}

		return uniqueMembers.Count();
	}

	bool FinalizeForceSpawnProjection(HST_CampaignState state, HST_ActiveGroupState activeGroup, out string reason)
	{
		reason = "";
		HST_ForceSpawnResultState batch = ValidateForceSpawnProjectionIdentity(state, activeGroup, reason);
		if (!batch)
			return false;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_READY_FOR_HANDOFF)
		{
			reason = "force spawn projection cannot hand off before every exact slot is ready";
			return false;
		}
		bool ownershipHeld = IsForceSpawnRuntimeOwnershipHeld(activeGroup);
		bool alreadyApplied = !ownershipHeld && activeGroup.m_bSpawnedEntity && activeGroup.m_sRuntimeEntityId == activeGroup.m_sGroupId;
		if (!ownershipHeld && !alreadyApplied)
		{
			reason = "force spawn projection has no local runtime ownership";
			return false;
		}

		SCR_AIGroup root = GetForceSpawnGroupRoot(activeGroup);
		if (!root || root.IsDeleted() || CountRegisteredForceSpawnGroupSlots(batch) != 1)
		{
			reason = "force spawn projection requires exactly one registered runtime group root";
			return false;
		}
		int memberCount = CountForceSpawnRuntimeMembers(activeGroup);
		if (memberCount != CountRegisteredForceSpawnMemberSlots(batch))
		{
			reason = string.Format("force spawn runtime member count %1 does not match registered manifest count %2", memberCount, CountRegisteredForceSpawnMemberSlots(batch));
			return false;
		}
		if (!ValidateForceSpawnGroupCardinality(root, memberCount, reason))
			return false;
		if (!ValidateRegisteredForceSpawnMembers(activeGroup, root, reason))
			return false;

		if (!alreadyApplied)
		{
			ActivateRegisteredForceSpawnMembers(activeGroup, root);
			ApplyFinalForceSpawnProjectionState(state, activeGroup, memberCount);
			ReleaseForceSpawnRuntimeOwnership(activeGroup);
			RefreshActiveGroupZoneCounts(state, activeGroup);
			m_bMarkerRefreshNeeded = true;
		}
		return true;
	}

	bool MarkForceSpawnProjectionFailed(HST_CampaignState state, HST_ActiveGroupState activeGroup, string failureReason)
	{
		string identityFailure;
		HST_ForceSpawnResultState batch = ValidateForceSpawnProjectionIdentity(state, activeGroup, identityFailure);
		if (!batch)
			return false;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			&& batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
			return false;
		if (GetForceSpawnGroupRoot(activeGroup) || CountForceSpawnRuntimeMembers(activeGroup) > 0)
			return false;

		activeGroup.m_bSpawnAttempted = true;
		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_sRuntimeEntityId = "";
		activeGroup.m_sRuntimeStatus = "spawn_failed";
		activeGroup.m_sSpawnFailureReason = failureReason;
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_iSurvivorVehicleCount = 0;
		ReleaseForceSpawnRuntimeOwnership(activeGroup);
		RefreshActiveGroupZoneCounts(state, activeGroup);
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	bool PrepareForceSpawnProjectionCleanup(HST_CampaignState state, HST_ActiveGroupState activeGroup, out string reason)
	{
		reason = "";
		HST_ForceSpawnResultState batch = ValidateForceSpawnProjectionIdentity(state, activeGroup, reason);
		if (!batch)
			return false;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			reason = "force spawn runtime retirement requires a successful exact projection";
			return false;
		}
		if (!GetForceSpawnGroupRoot(activeGroup))
		{
			reason = "force spawn runtime retirement has no exact registered group root";
			return false;
		}

		return AcquireForceSpawnRuntimeOwnership(activeGroup, reason);
	}

	bool ReleaseForceSpawnRuntimeOwnership(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		bool removed;
		for (int i = m_aForceSpawnOwnedGroupIds.Count() - 1; i >= 0; i--)
		{
			if (m_aForceSpawnOwnedGroupIds[i] != activeGroup.m_sGroupId || i >= m_aForceSpawnOwnedResultIds.Count())
				continue;
			if (m_aForceSpawnOwnedResultIds[i] != activeGroup.m_sSpawnResultId)
				continue;

			m_aForceSpawnOwnedResultIds.Remove(i);
			m_aForceSpawnOwnedGroupIds.Remove(i);
			removed = true;
		}

		return removed;
	}

	bool IsForceSpawnRuntimeMemberAlive(IEntity member)
	{
		return member && !member.IsDeleted() && !SCR_AIGroup.Cast(member) && IsLivingEntity(member);
	}

	bool DetachConfirmedDeadForceSpawnMember(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		IEntity member,
		out string reason)
	{
		reason = "";
		HST_ForceSpawnResultState batch = ValidateForceSpawnProjectionIdentity(state, activeGroup, reason);
		if (!batch || batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			if (reason.IsEmpty())
				reason = "force casualty detachment requires a successful exact projection";
			return false;
		}
		if (member && !member.IsDeleted() && IsForceSpawnRuntimeMemberAlive(member))
		{
			reason = "force casualty detachment refused for a living member";
			return false;
		}
		if (!AcquireForceSpawnRuntimeOwnership(activeGroup, reason))
			return false;
		bool detached = TryUnregisterForceSpawnGroupMember(activeGroup, member, false, reason);
		ReleaseForceSpawnRuntimeOwnership(activeGroup);
		if (detached)
		{
			RefreshActiveGroupZoneCounts(state, activeGroup);
			m_bMarkerRefreshNeeded = true;
		}
		return detached;
	}

	bool FinalizeEliminatedForceSpawnProjection(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		int nowSecond,
		out string reason)
	{
		reason = "";
		HST_ForceSpawnResultState batch = ValidateForceSpawnProjectionIdentity(state, activeGroup, reason);
		if (!batch)
			return false;
		if (batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_iSuccessfulHandoffCount <= 0)
		{
			reason = "force elimination requires a successfully handed-off projection";
			return false;
		}
		if (!activeGroup.m_bEverPopulated || !activeGroup.m_bSpawnCompleted)
		{
			reason = "force elimination requires ever-populated and spawn-completed evidence";
			return false;
		}

		for (int preflightIndex = 0; preflightIndex < m_aRuntimeGroupEntities.Count(); preflightIndex++)
		{
			if (preflightIndex >= m_aRuntimeGroupIds.Count()
				|| m_aRuntimeGroupIds[preflightIndex] != activeGroup.m_sGroupId)
				continue;
			IEntity runtimeEntity = m_aRuntimeGroupEntities[preflightIndex];
			if (runtimeEntity && !SCR_AIGroup.Cast(runtimeEntity) && IsForceSpawnRuntimeMemberAlive(runtimeEntity))
			{
				reason = "force elimination refused while an exact runtime member is still alive";
				return false;
			}
		}

		if (!AcquireForceSpawnRuntimeOwnership(activeGroup, reason))
			return false;
		for (int memberIndex = m_aRuntimeGroupIds.Count() - 1; memberIndex >= 0; memberIndex--)
		{
			if (m_aRuntimeGroupIds[memberIndex] != activeGroup.m_sGroupId || memberIndex >= m_aRuntimeGroupEntities.Count())
				continue;
			IEntity member = m_aRuntimeGroupEntities[memberIndex];
			if (SCR_AIGroup.Cast(member))
				continue;
			if (member && !member.IsDeleted())
				DetachForceSpawnMember(activeGroup, member);
			m_aRuntimeGroupEntities.Remove(memberIndex);
			m_aRuntimeGroupIds.Remove(memberIndex);
		}

		for (int rootIndex = m_aRuntimeGroupIds.Count() - 1; rootIndex >= 0; rootIndex--)
		{
			if (m_aRuntimeGroupIds[rootIndex] != activeGroup.m_sGroupId || rootIndex >= m_aRuntimeGroupEntities.Count())
				continue;
			IEntity rootEntity = m_aRuntimeGroupEntities[rootIndex];
			SCR_AIGroup root = SCR_AIGroup.Cast(rootEntity);
			if (!root)
				continue;
			DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);
			m_aRuntimeGroupEntities.Remove(rootIndex);
			m_aRuntimeGroupIds.Remove(rootIndex);
			if (!root.IsDeleted())
				SCR_EntityHelper.DeleteEntityAndChildren(root);
		}

		ReleaseForceSpawnRuntimeOwnership(activeGroup);
		activeGroup.m_bSpawnAttempted = true;
		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_sRuntimeEntityId = "";
		activeGroup.m_sRuntimeStatus = "eliminated";
		activeGroup.m_sSpawnFailureReason = "all exact roster members are confirmed dead";
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_iSurvivorVehicleCount = 0;
		activeGroup.m_iDurableLivingInfantryCount = 0;
		activeGroup.m_iEliminatedAtSecond = Math.Max(0, nowSecond);
		activeGroup.m_iLifecycleRevision++;
		RefreshActiveGroupZoneCounts(state, activeGroup);
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected HST_ForceSpawnResultState ValidateForceSpawnProjectionIdentity(HST_CampaignState state, HST_ActiveGroupState activeGroup, out string reason)
	{
		reason = "";
		if (!state || !activeGroup)
		{
			reason = "force spawn projection state or active group is missing";
			return null;
		}
		if (activeGroup.m_sGroupId.IsEmpty() || activeGroup.m_sSpawnResultId.IsEmpty() || activeGroup.m_sManifestId.IsEmpty())
		{
			reason = "force spawn active-group identity is incomplete";
			return null;
		}
		if (activeGroup.m_sForceId.IsEmpty() || activeGroup.m_sProjectionId.IsEmpty() || activeGroup.m_sOperationId.IsEmpty())
		{
			reason = "force spawn active-group force, projection, or operation identity is incomplete";
			return null;
		}
		if (state.FindActiveGroup(activeGroup.m_sGroupId) != activeGroup)
		{
			reason = "force spawn active group is not the canonical campaign projection";
			return null;
		}

		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(activeGroup.m_sSpawnResultId);
		if (!batch)
		{
			reason = "force spawn result is missing";
			return null;
		}
		if (batch.m_sResultId != activeGroup.m_sSpawnResultId || batch.m_sManifestId != activeGroup.m_sManifestId)
		{
			reason = "force spawn result or manifest identity conflicts with active group";
			return null;
		}
		if (batch.m_sForceId != activeGroup.m_sForceId || batch.m_sProjectionId != activeGroup.m_sProjectionId)
		{
			reason = "force spawn force or projection identity conflicts with active group";
			return null;
		}
		if (batch.m_sOperationId != activeGroup.m_sOperationId)
		{
			reason = "force spawn operation identity conflicts with active group";
			return null;
		}

		return batch;
	}

	protected bool ValidateForceSpawnGroupRoot(HST_ActiveGroupState activeGroup, SCR_AIGroup root, out string reason)
	{
		reason = "";
		if (!activeGroup || !root)
		{
			reason = "force spawn group root is missing";
			return false;
		}
		if (root.GetAgentsCount() > 0 || root.GetServerAgentsCount() > 0 || root.GetSpawnQueueSize() > 0 || root.IsInitializing())
		{
			reason = "force spawn group root contains native or queued members before exact registration";
			return false;
		}

		string actualFaction;
		if (!IsRuntimeGroupRootFactionExpected(root, activeGroup, actualFaction))
		{
			reason = string.Format("force spawn group root faction mismatch: expected %1 actual %2", activeGroup.m_sFactionKey, ReportText(actualFaction));
			return false;
		}
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(root.FindComponent(SCR_EditableGroupComponent));
		if (!editableGroup || !editableGroup.GetFaction() || editableGroup.GetFaction().GetFactionKey() != activeGroup.m_sFactionKey)
		{
			reason = "force spawn group root editable faction is missing or mismatched";
			return false;
		}

		return true;
	}

	protected bool ValidateForceSpawnGroupMember(HST_ActiveGroupState activeGroup, SCR_AIGroup root, IEntity member, int ordinal, out string reason)
	{
		reason = "";
		if (!activeGroup || !root || !member || ordinal < 0)
		{
			reason = "force spawn member, group root, or ordinal is invalid";
			return false;
		}
		if (SCR_AIGroup.Cast(member) || IsPlayerControlledRuntimeEntity(member) || !IsLivingEntity(member))
		{
			reason = "force spawn member is a group root, player-controlled, or not alive";
			return false;
		}
		if (ResolveEntityFactionKey(member) != activeGroup.m_sFactionKey)
		{
			reason = "force spawn member faction does not match the active group";
			return false;
		}

		AIAgent agent = ResolveRuntimeMemberAIAgent(member);
		if (!agent || agent.GetParentGroup() != root)
		{
			reason = "force spawn member is not attached to the exact native group root";
			return false;
		}
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(root.FindComponent(SCR_EditableGroupComponent));
		SCR_EditableEntityComponent editableMember = SCR_EditableEntityComponent.GetEditableEntity(member);
		if (!editableGroup || !editableMember || editableMember.GetParentEntity() != editableGroup)
		{
			reason = "force spawn member is not attached to the exact editable group root";
			return false;
		}

		return true;
	}

	protected bool AttachForceSpawnGroupMember(HST_ActiveGroupState activeGroup, SCR_AIGroup root, IEntity member, int ordinal, out string reason)
	{
		reason = "";
		if (!activeGroup || !root || !member || ordinal < 0)
		{
			reason = "force spawn member attach input is invalid";
			return false;
		}
		if (SCR_AIGroup.Cast(member) || IsPlayerControlledRuntimeEntity(member) || !IsLivingEntity(member))
		{
			reason = "force spawn member cannot attach because it is a group root, player-controlled, or not alive";
			return false;
		}
		if (ResolveEntityFactionKey(member) != activeGroup.m_sFactionKey)
		{
			reason = "force spawn member cannot attach with a mismatched faction";
			return false;
		}

		AIAgent agent = ResolveRuntimeMemberAIAgent(member);
		if (!agent)
		{
			reason = "force spawn member has no AI agent";
			return false;
		}
		if (agent.GetParentGroup() && agent.GetParentGroup() != root)
		{
			reason = "force spawn member already belongs to another native group";
			return false;
		}
		if (agent.GetParentGroup() != root)
		{
			root.AddAgentFromControlledEntity(member);
			if (agent.GetParentGroup() != root && !root.AddAIEntityToGroup(member))
				root.AddAgent(agent);
		}
		if (agent.GetParentGroup() != root)
		{
			reason = "force spawn member could not attach to the exact native group";
			return false;
		}

		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(root.FindComponent(SCR_EditableGroupComponent));
		SCR_EditableEntityComponent editableMember = SCR_EditableEntityComponent.GetEditableEntity(member);
		if (!editableGroup || !editableMember)
		{
			root.RemoveAgentFromControlledEntity(member);
			reason = "force spawn member or group is missing editable registration";
			return false;
		}
		if (editableMember.GetParentEntity() && editableMember.GetParentEntity() != editableGroup)
		{
			root.RemoveAgentFromControlledEntity(member);
			reason = "force spawn member already belongs to another editable group";
			return false;
		}
		if (editableMember.GetParentEntity() != editableGroup)
			editableMember.SetParentEntity(editableGroup);
		if (editableMember.GetParentEntity() != editableGroup)
		{
			root.RemoveAgentFromControlledEntity(member);
			editableMember.SetParentEntity(null);
			reason = "force spawn member could not attach to the exact editable group";
			return false;
		}

		DeactivateForceSpawnMember(member);
		return true;
	}

	protected void PrepareForceSpawnGroupRoot(SCR_AIGroup root)
	{
		if (!root)
			return;

		root.SetSpawnImmediately(false);
		root.SetNumberOfMembersToSpawn(0);
		root.SetMaxUnitsToSpawn(0);
		root.SetMemberSpawnDelay(0);
		root.SetDeleteWhenEmpty(false);
		root.DeactivateAI();
	}

	protected void DeactivateForceSpawnMember(IEntity member)
	{
		if (!member)
			return;

		AIControlComponent control = AIControlComponent.Cast(member.FindComponent(AIControlComponent));
		if (control)
			control.DeactivateAI();
		AIAgent agent = ResolveRuntimeMemberAIAgent(member);
		if (agent)
			agent.DeactivateAI();
	}

	protected void DetachForceSpawnMember(HST_ActiveGroupState activeGroup, IEntity member)
	{
		if (!activeGroup || !member)
			return;

		SCR_AIGroup root = GetForceSpawnGroupRoot(activeGroup);
		if (root)
			root.RemoveAgentFromControlledEntity(member);
		SCR_EditableEntityComponent editableMember = SCR_EditableEntityComponent.GetEditableEntity(member);
		if (editableMember)
			editableMember.SetParentEntity(null);
	}

	protected bool AcquireForceSpawnRuntimeOwnership(HST_ActiveGroupState activeGroup, out string reason)
	{
		reason = "";
		if (!activeGroup || activeGroup.m_sGroupId.IsEmpty() || activeGroup.m_sSpawnResultId.IsEmpty())
		{
			reason = "force spawn runtime ownership identity is incomplete";
			return false;
		}

		for (int i = 0; i < m_aForceSpawnOwnedGroupIds.Count(); i++)
		{
			if (m_aForceSpawnOwnedGroupIds[i] != activeGroup.m_sGroupId || i >= m_aForceSpawnOwnedResultIds.Count())
				continue;
			if (m_aForceSpawnOwnedResultIds[i] == activeGroup.m_sSpawnResultId)
				return true;

			reason = "active group is already owned by another force spawn result";
			return false;
		}

		m_aForceSpawnOwnedGroupIds.Insert(activeGroup.m_sGroupId);
		m_aForceSpawnOwnedResultIds.Insert(activeGroup.m_sSpawnResultId);
		return true;
	}

	protected bool IsForceSpawnRuntimeOwnershipHeld(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		for (int i = 0; i < m_aForceSpawnOwnedGroupIds.Count(); i++)
		{
			if (m_aForceSpawnOwnedGroupIds[i] == activeGroup.m_sGroupId && i < m_aForceSpawnOwnedResultIds.Count() && m_aForceSpawnOwnedResultIds[i] == activeGroup.m_sSpawnResultId)
				return true;
		}

		return false;
	}

	protected bool IsForceSpawnRuntimeOwnershipHeldForGroup(string groupId)
	{
		return !groupId.IsEmpty() && m_aForceSpawnOwnedGroupIds.Contains(groupId);
	}

	protected bool ValidateForceSpawnCleanupOwnership(HST_ActiveGroupState activeGroup, out string reason)
	{
		reason = "";
		if (!activeGroup || !IsForceSpawnQueueManaged(activeGroup))
		{
			reason = "force spawn cleanup active group is missing or unmanaged";
			return false;
		}
		if (!IsForceSpawnRuntimeOwnershipHeld(activeGroup))
		{
			reason = "force spawn cleanup does not own the local runtime projection";
			return false;
		}

		return true;
	}

	protected bool IsRuntimeHandleTrackedByAnotherGroup(string groupId, IEntity entity)
	{
		if (!entity)
			return false;

		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (i >= m_aRuntimeGroupEntities.Count() || m_aRuntimeGroupEntities[i] != entity)
				continue;
			if (m_aRuntimeGroupIds[i] != groupId)
				return true;
		}

		return false;
	}

	protected int RemoveRuntimeGroupEntityHandleExact(string groupId, IEntity entity)
	{
		if (groupId.IsEmpty() || !entity)
			return 0;

		int removed;
		for (int i = m_aRuntimeGroupIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count() || m_aRuntimeGroupEntities[i] != entity)
				continue;

			m_aRuntimeGroupEntities.Remove(i);
			m_aRuntimeGroupIds.Remove(i);
			removed++;
		}

		return removed;
	}

	protected int CountRegisteredForceSpawnGroupSlots(HST_ForceSpawnResultState batch)
	{
		return CountRegisteredForceSpawnSlotsByKind(batch, HST_ForceSpawnQueueService.SLOT_KIND_GROUP);
	}

	protected int CountRegisteredForceSpawnMemberSlots(HST_ForceSpawnResultState batch)
	{
		return CountRegisteredForceSpawnSlotsByKind(batch, HST_ForceSpawnQueueService.SLOT_KIND_MEMBER);
	}

	protected int CountRegisteredForceSpawnSlotsByKind(HST_ForceSpawnResultState batch, string slotKind)
	{
		if (!batch)
			return 0;

		int count;
		foreach (HST_ForceSpawnSlotResultState slotResult : batch.m_aSlotResults)
		{
			if (slotResult && slotResult.m_sSlotKind == slotKind && slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED)
				count++;
		}

		return count;
	}

	protected bool ValidateRegisteredForceSpawnMembers(HST_ActiveGroupState activeGroup, SCR_AIGroup root, out string reason)
	{
		reason = "";
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != activeGroup.m_sGroupId || i >= m_aRuntimeGroupEntities.Count())
				continue;
			IEntity member = m_aRuntimeGroupEntities[i];
			if (!member || SCR_AIGroup.Cast(member))
				continue;

			string memberReason;
			if (!ValidateForceSpawnGroupMember(activeGroup, root, member, 0, memberReason))
			{
				reason = "registered force spawn member failed final validation: " + memberReason;
				return false;
			}
		}

		return true;
	}

	protected bool ValidateForceSpawnGroupCardinality(SCR_AIGroup root, int expectedMembers, out string reason)
	{
		reason = "";
		if (!root || root.IsDeleted())
		{
			reason = "force spawn final group root is missing or deleted";
			return false;
		}
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(root.FindComponent(SCR_EditableGroupComponent));
		if (!editableGroup)
		{
			reason = "force spawn final group root has no editable group";
			return false;
		}
		if (root.IsInitializing() || root.GetSpawnQueueSize() != 0)
		{
			reason = "force spawn final group root still has native initialization or queued spawning";
			return false;
		}
		if (root.GetAgentsCount() != expectedMembers || root.GetServerAgentsCount() != expectedMembers
			|| root.GetPlayerAndAgentCount() != expectedMembers || editableGroup.GetSize() != expectedMembers)
		{
			reason = string.Format(
				"force spawn final group cardinality mismatch: expected %1 agents/server/playerEditable %2/%3/%4/%5",
				expectedMembers,
				root.GetAgentsCount(),
				root.GetServerAgentsCount(),
				root.GetPlayerAndAgentCount(),
				editableGroup.GetSize());
			return false;
		}
		return true;
	}

	protected void ActivateRegisteredForceSpawnMembers(HST_ActiveGroupState activeGroup, SCR_AIGroup root)
	{
		if (!activeGroup || !root)
			return;

		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != activeGroup.m_sGroupId || i >= m_aRuntimeGroupEntities.Count())
				continue;
			IEntity member = m_aRuntimeGroupEntities[i];
			if (!member || SCR_AIGroup.Cast(member))
				continue;

			AIControlComponent control = AIControlComponent.Cast(member.FindComponent(AIControlComponent));
			if (control)
				control.ActivateAI();
			AIAgent agent = ResolveRuntimeMemberAIAgent(member);
			if (agent)
				agent.ActivateAI();
		}

		root.SetMaxUnitsToSpawn(Math.Max(0, CountForceSpawnRuntimeMembers(activeGroup)));
		root.ActivateAllMembers();
		root.ActivateAI();
	}

	protected void ApplyFinalForceSpawnProjectionState(HST_CampaignState state, HST_ActiveGroupState activeGroup, int memberCount)
	{
		activeGroup.m_bSpawnAttempted = true;
		activeGroup.m_bSpawnedEntity = true;
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = ResolveSpawnedRuntimeStatus(activeGroup, activeGroup.m_sRuntimeStatus);
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = memberCount;
		activeGroup.m_iLastSeenAliveCount = memberCount;
		activeGroup.m_iSurvivorInfantryCount = Math.Min(Math.Max(0, activeGroup.m_iInfantryCount), memberCount);
		activeGroup.m_iSurvivorVehicleCount = Math.Max(0, activeGroup.m_iSurvivorVehicleCount);
		activeGroup.m_iDurableLivingInfantryCount = memberCount;
		activeGroup.m_bSpawnCompleted = true;
		activeGroup.m_bEverPopulated = activeGroup.m_bEverPopulated || memberCount > 0;
		activeGroup.m_iLifecycleRevision++;
		if (state)
			activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
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
		bool townPolicePatrolChanged = UpdateTownPolicePatrols(state, preset);
		bool zoneGarrisonPatrolChanged = UpdateZoneGarrisonPatrols(state, preset);
		bool routeChanged = UpdateActiveGroupRoutes(state, forceRouteUpdate);
		bool combatProbeChanged = SampleCampaignDebugPhysicalCombatProbe(state);
		bool changed = missionCleanupChanged || runtimeEntityChanged;
		changed = changed || survivorChanged;
		changed = changed || townPolicePatrolChanged;
		changed = changed || zoneGarrisonPatrolChanged;
		changed = changed || routeChanged;
		changed = changed || combatProbeChanged;
		return changed;
	}

	bool RestartExactEnemyQRFInfantryRoute(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		vector targetPosition,
		string reason)
	{
		return RestartExactEnemyOperationInfantryRoute(
			state,
			activeGroup,
			targetPosition,
			reason,
			HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF,
			HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF,
			HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION);
	}

	bool RestartExactEnemyCounterattackInfantryRoute(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		vector targetPosition,
		string reason)
	{
		return RestartExactEnemyOperationInfantryRoute(
			state,
			activeGroup,
			targetPosition,
			reason,
			HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK,
			HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK,
			HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION);
	}

	bool RestartExactEnemyPatrolInfantryRoute(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		vector targetPosition,
		string reason)
	{
		return RestartExactEnemyOperationInfantryRoute(
			state,
			activeGroup,
			targetPosition,
			reason,
			HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL,
			HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL,
			HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION);
	}

	bool RestartExactGarrisonPatrolInfantryRoute(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		vector targetPosition,
		string reason)
	{
		if (!IsExactGarrisonPatrolActiveGroup(state, activeGroup)
			|| !activeGroup.m_bSpawnedEntity || activeGroup.m_iInfantryCount <= 0
			|| IsZeroVector(targetPosition) || IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;

		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		if (!operation
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return false;

		vector livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, false);
		if (IsZeroVector(livePosition))
			livePosition = activeGroup.m_vPosition;
		if (IsZeroVector(livePosition))
			return false;

		DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);
		activeGroup.m_vPosition = livePosition;
		activeGroup.m_vSourcePosition = livePosition;
		activeGroup.m_vTargetPosition = targetPosition;
		activeGroup.m_sRuntimeStatus = "routing";
		activeGroup.m_iAssignedWaypointCount = 0;
		activeGroup.m_iSpawnedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		activeGroup.m_sSpawnFailureReason = reason;
		activeGroup.m_iLifecycleRevision++;
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	bool IsExactEnemyQRFRouteRecoveryExhausted(HST_ActiveGroupState activeGroup, int nowSecond)
	{
		if (!activeGroup || !activeGroup.m_bQRF || activeGroup.m_sEnemyOrderId.IsEmpty()
			|| activeGroup.m_sOperationId.IsEmpty() || activeGroup.m_sProjectionId.IsEmpty())
			return false;
		return IsExactOperationRouteRecoveryExhausted(activeGroup, nowSecond);
	}

	bool IsExactEnemyCounterattackRouteRecoveryExhausted(HST_ActiveGroupState activeGroup, int nowSecond)
	{
		if (!activeGroup || !activeGroup.m_bQRF || activeGroup.m_sEnemyOrderId.IsEmpty()
			|| activeGroup.m_sOperationId.IsEmpty() || activeGroup.m_sProjectionId.IsEmpty())
			return false;
		return IsExactOperationRouteRecoveryExhausted(activeGroup, nowSecond);
	}

	bool IsExactEnemyPatrolRouteRecoveryExhausted(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		int nowSecond)
	{
		if (!IsExactEnemyPatrolActiveGroup(state, activeGroup))
			return false;
		return IsExactOperationRouteRecoveryExhausted(activeGroup, nowSecond);
	}

	bool IsExactGarrisonPatrolRouteRecoveryExhausted(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		int nowSecond)
	{
		if (!IsExactGarrisonPatrolActiveGroup(state, activeGroup))
			return false;
		return IsExactOperationRouteRecoveryExhausted(activeGroup, nowSecond);
	}

	bool TryResolveExactEnemyPatrolLivePosition(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		out vector livePosition,
		out string evidence)
	{
		livePosition = "0 0 0";
		evidence = "missing exact enemy patrol group";
		if (!IsExactEnemyPatrolActiveGroup(state, activeGroup))
			return false;
		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		if (!operation
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
		{
			evidence = "exact enemy patrol does not own physical live-position authority";
			return false;
		}
		if (!activeGroup.m_bSpawnedEntity)
		{
			evidence = "exact enemy patrol is not physically spawned";
			return false;
		}

		livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, false);
		if (IsZeroVector(livePosition))
		{
			evidence = "exact enemy patrol has no living runtime member position";
			return false;
		}
		if (IsZeroVector(activeGroup.m_vPosition) || DistanceSq2D(activeGroup.m_vPosition, livePosition) >= 4.0)
		{
			activeGroup.m_vPosition = livePosition;
			m_bMarkerRefreshNeeded = true;
		}
		evidence = string.Format("exact enemy patrol live position %1", livePosition);
		return true;
	}

	bool HasExactEnemyPatrolLiveContactEvidence(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		float contactRadiusMeters,
		out string evidence)
	{
		vector livePosition;
		if (!TryResolveExactEnemyPatrolLivePosition(state, activeGroup, livePosition, evidence))
			return false;

		float nearestPlayerMeters = ResolveNearestLivingPlayerDistanceMeters(livePosition);
		if (nearestPlayerMeters < 0)
		{
			evidence = "exact enemy patrol has no living player contact candidate";
			return false;
		}
		float acceptedRadius = Math.Max(1.0, contactRadiusMeters);
		evidence = string.Format(
			"exact enemy patrol nearest living player %1m/%2m at %3",
			Math.Round(nearestPlayerMeters),
			Math.Round(acceptedRadius),
			livePosition);
		return nearestPlayerMeters <= acceptedRadius;
	}

	bool TryResolveExactGarrisonPatrolLivePosition(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		out vector livePosition,
		out string evidence)
	{
		livePosition = "0 0 0";
		evidence = "missing exact garrison patrol group";
		if (!IsExactGarrisonPatrolActiveGroup(state, activeGroup))
			return false;
		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		if (!operation
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
		{
			evidence = "exact garrison patrol does not own physical live-position authority";
			return false;
		}
		if (!activeGroup.m_bSpawnedEntity)
		{
			evidence = "exact garrison patrol is not physically spawned";
			return false;
		}

		livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, false);
		if (IsZeroVector(livePosition))
		{
			evidence = "exact garrison patrol has no living runtime member position";
			return false;
		}
		if (IsZeroVector(activeGroup.m_vPosition) || DistanceSq2D(activeGroup.m_vPosition, livePosition) >= 4.0)
		{
			activeGroup.m_vPosition = livePosition;
			m_bMarkerRefreshNeeded = true;
		}
		evidence = string.Format("exact garrison patrol live position %1", livePosition);
		return true;
	}

	bool HasExactGarrisonPatrolLiveContactEvidence(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		float contactRadiusMeters,
		out string evidence)
	{
		vector livePosition;
		if (!TryResolveExactGarrisonPatrolLivePosition(state, activeGroup, livePosition, evidence))
			return false;

		float nearestPlayerMeters = ResolveNearestLivingPlayerDistanceMeters(livePosition);
		if (nearestPlayerMeters < 0)
		{
			evidence = "exact garrison patrol has no living player contact candidate";
			return false;
		}
		float acceptedRadius = Math.Max(1.0, contactRadiusMeters);
		evidence = string.Format(
			"exact garrison patrol nearest living player %1m/%2m at %3",
			Math.Round(nearestPlayerMeters),
			Math.Round(acceptedRadius),
			livePosition);
		return nearestPlayerMeters <= acceptedRadius;
	}

	bool TryResolveExactLocalSecurityPatrolLivePosition(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		out vector livePosition,
		out string evidence)
	{
		livePosition = "0 0 0";
		evidence = "missing exact open physical local-security patrol";
		if (!IsExactOpenPhysicalLocalSecurityPatrolGroup(state, activeGroup))
			return false;

		livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, false);
		if (IsZeroVector(livePosition))
		{
			evidence = "exact local-security patrol has no living runtime member position";
			return false;
		}
		evidence = string.Format("exact local-security patrol live position %1", livePosition);
		return true;
	}

	bool HasExactLocalSecurityPatrolLiveContactEvidence(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		float contactRadiusMeters,
		out string evidence)
	{
		vector livePosition;
		if (!TryResolveExactLocalSecurityPatrolLivePosition(state, activeGroup, livePosition, evidence))
			return false;

		float nearestPlayerMeters = ResolveNearestLivingPlayerDistanceMeters(livePosition);
		if (nearestPlayerMeters < 0)
		{
			evidence = "exact local-security patrol has no living player contact candidate";
			return false;
		}
		float acceptedRadius = Math.Max(1.0, contactRadiusMeters);
		evidence = string.Format(
			"exact local-security patrol nearest living player %1m/%2m at %3",
			Math.Round(nearestPlayerMeters),
			Math.Round(acceptedRadius),
			livePosition);
		return nearestPlayerMeters <= acceptedRadius;
	}

	bool AssignExactLocalSecurityPatrolWaypoints(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		out string evidence)
	{
		evidence = "missing exact open physical local-security patrol";
		if (!IsExactOpenPhysicalLocalSecurityPatrolGroup(state, activeGroup))
			return false;
		if (ShouldHoldForceSpawnProjection(state, activeGroup))
		{
			evidence = "exact local-security patrol physical handoff is not complete";
			return false;
		}
		if (IsTownPolicePatrolAssigned(activeGroup))
		{
			evidence = string.Format(
				"exact local-security patrol already owns %1 cycle waypoints",
				activeGroup.m_iAssignedWaypointCount);
			return true;
		}

		string reason;
		int waypointCount = AssignTownPolicePatrolWaypoints(state, activeGroup, reason);
		if (waypointCount <= 1)
		{
			evidence = reason;
			if (evidence.IsEmpty())
				evidence = "exact local-security patrol could not acquire cycle waypoints";
			return false;
		}

		activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(
			activeGroup.m_sSpawnFallbackMode,
			"town_police_patrol");
		activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(
			activeGroup.m_sSpawnFallbackMode,
			"patrol_cycle");
		activeGroup.m_sSpawnFailureReason = reason;
		evidence = reason;
		return true;
	}

	bool HasExactMissionGuardRuntime(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!HasOpenPhysicalMissionGuardRuntimeAuthority(state, activeGroup))
			return false;

		return HasRuntimeGroupEntity(activeGroup.m_sGroupId);
	}

	bool TryResolveExactMissionGuardLivePosition(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		out vector livePosition,
		out string evidence)
	{
		livePosition = "0 0 0";
		evidence = "missing open physical mission guard authority";
		if (!HasOpenPhysicalMissionGuardRuntimeAuthority(state, activeGroup))
			return false;

		if (!HasRuntimeGroupEntity(activeGroup.m_sGroupId))
		{
			evidence = "mission guard physical runtime is not registered";
			return false;
		}

		livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, false);
		if (IsZeroVector(livePosition))
		{
			evidence = "exact mission guard has no living runtime member position";
			return false;
		}
		if (IsZeroVector(activeGroup.m_vPosition) || DistanceSq2D(activeGroup.m_vPosition, livePosition) >= 4.0)
		{
			activeGroup.m_vPosition = livePosition;
			m_bMarkerRefreshNeeded = true;
		}
		evidence = string.Format("exact mission guard live position %1", livePosition);
		return true;
	}

	bool HasExactMissionGuardLiveContactEvidence(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		float contactRadiusMeters,
		out string evidence)
	{
		vector livePosition;
		if (!TryResolveExactMissionGuardLivePosition(state, activeGroup, livePosition, evidence))
			return false;

		float nearestPlayerMeters = ResolveNearestLivingPlayerDistanceMeters(livePosition);
		if (nearestPlayerMeters < 0)
		{
			evidence = "exact mission guard has no living player contact candidate";
			return false;
		}
		float acceptedRadius = Math.Max(1.0, contactRadiusMeters);
		evidence = string.Format(
			"exact mission guard nearest living player %1m/%2m at %3",
			Math.Round(nearestPlayerMeters),
			Math.Round(acceptedRadius),
			livePosition);
		return nearestPlayerMeters <= acceptedRadius;
	}

	bool RestartExactMissionGuardInfantryAssignment(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		vector assignmentPosition,
		string reason)
	{
		if (!IsExactMissionGuardActiveGroup(state, activeGroup)
			|| !activeGroup.m_bSpawnedEntity || activeGroup.m_iInfantryCount <= 0
			|| IsZeroVector(assignmentPosition) || IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;

		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		if (!operation
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return false;

		vector livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, false);
		if (IsZeroVector(livePosition))
			livePosition = activeGroup.m_vPosition;
		if (IsZeroVector(livePosition) || !Replication.IsServer())
			return false;

		SCR_AIGroup group = SCR_AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId));
		if (!group)
			return false;
		RplComponent groupReplication = RplComponent.Cast(group.FindComponent(RplComponent));
		if (groupReplication && !groupReplication.IsMaster())
			return false;

		bool atAssignment = DistanceSq2D(livePosition, assignmentPosition) < 16.0;
		IEntity waypointEntity;
		AIWaypoint waypoint;
		if (!atAssignment)
		{
			waypointEntity = SpawnActiveGroupRouteWaypoint(activeGroup.m_sGroupId, assignmentPosition, 1, false);
			waypoint = AIWaypoint.Cast(waypointEntity);
			if (!waypoint)
				return false;
		}

		DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);
		if (waypoint)
		{
			group.AddWaypoint(waypoint);
			m_aRuntimeGroupWaypointIds.Insert(activeGroup.m_sGroupId);
			m_aRuntimeGroupWaypointEntities.Insert(waypointEntity);
		}
		group.ActivateAllMembers();
		group.ActivateAI();

		activeGroup.m_vPosition = livePosition;
		activeGroup.m_vSourcePosition = livePosition;
		activeGroup.m_vTargetPosition = assignmentPosition;
		activeGroup.m_sRouteId = "";
		activeGroup.m_sRuntimeStatus = "mission_guard_stationary";
		if (!atAssignment)
			activeGroup.m_sRuntimeStatus = "mission_guard_returning";
		activeGroup.m_iAssignedWaypointCount = 0;
		if (!atAssignment)
			activeGroup.m_iAssignedWaypointCount = 1;
		activeGroup.m_iSpawnedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		activeGroup.m_sSpawnFailureReason = reason;
		activeGroup.m_iLifecycleRevision++;
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected bool RestartExactEnemyOperationInfantryRoute(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		vector targetPosition,
		string reason,
		HST_EOperationType operationType,
		HST_EEnemyOrderType orderType,
		int contractVersion)
	{
		if (!IsExactEnemyOperationActiveGroup(state, activeGroup, operationType, orderType, contractVersion)
			|| !activeGroup.m_bSpawnedEntity || activeGroup.m_iInfantryCount <= 0
			|| IsZeroVector(targetPosition) || IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;

		vector livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, false);
		if (IsZeroVector(livePosition))
			livePosition = activeGroup.m_vPosition;
		if (IsZeroVector(livePosition))
			return false;

		DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);
		activeGroup.m_vPosition = livePosition;
		activeGroup.m_vSourcePosition = livePosition;
		activeGroup.m_vTargetPosition = targetPosition;
		activeGroup.m_sRuntimeStatus = "routing";
		activeGroup.m_iAssignedWaypointCount = 0;
		activeGroup.m_iSpawnedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
		activeGroup.m_sSpawnFailureReason = reason;
		activeGroup.m_iLifecycleRevision++;
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected bool IsExactOperationRouteRecoveryExhausted(HST_ActiveGroupState activeGroup, int nowSecond)
	{
		if (!activeGroup || activeGroup.m_sGroupId.IsEmpty())
			return false;
		foreach (HST_ActiveGroupRouteProgressStatus progress : m_aActiveGroupRouteProgressStatuses)
		{
			if (!progress || progress.m_sGroupId != activeGroup.m_sGroupId)
				continue;
			if (progress.m_iRouteReissueAttemptCount < ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS)
				return false;
			if (progress.m_iLastProgressSecond < 0
				|| nowSecond - progress.m_iLastProgressSecond < ACTIVE_GROUP_ROUTE_STALL_REISSUE_SECONDS)
				return false;
			if (progress.m_fLastDistanceToTargetMeters >= 0
				&& progress.m_fLastDistanceToTargetMeters <= ACTIVE_GROUP_ROUTE_ARRIVAL_RADIUS_METERS)
				return false;
			return true;
		}
		return false;
	}

	bool RecallActiveSupportGroup(HST_CampaignState state, string groupId, vector exitPosition)
	{
		if (!state || groupId.IsEmpty() || IsZeroVector(exitPosition))
			return false;

		HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
		if (!activeGroup || activeGroup.m_sSupportRequestId.IsEmpty())
			return false;
		if (ShouldHoldForceSpawnProjection(state, activeGroup))
			return false;
		if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;

		string livePositionEvidence;
		TryRefreshActiveSupportGroupLivePosition(activeGroup, livePositionEvidence);
		vector sourcePosition = activeGroup.m_vPosition;
		if (IsZeroVector(sourcePosition))
			sourcePosition = activeGroup.m_vTargetPosition;
		if (IsZeroVector(sourcePosition))
			sourcePosition = activeGroup.m_vSourcePosition;

		DeleteRuntimeGroupWaypoints(groupId);
		activeGroup.m_vSourcePosition = sourcePosition;
		activeGroup.m_vTargetPosition = exitPosition;
		activeGroup.m_sRouteId = "";
		activeGroup.m_sRuntimeStatus = "support_recalling";
		activeGroup.m_sSpawnFallbackMode = "support_recall";
		activeGroup.m_iAssignedWaypointCount = 0;
		activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		activeGroup.m_sSpawnFailureReason = "Support recall ordered; moving to exit point.";
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	bool TryRefreshActiveSupportGroupLivePosition(HST_ActiveGroupState activeGroup, out string evidence)
	{
		evidence = "missing active support group";
		if (!activeGroup || activeGroup.m_sSupportRequestId.IsEmpty())
			return false;
		if (!activeGroup.m_bSpawnedEntity)
		{
			evidence = "active support group is not physically spawned";
			return false;
		}

		vector livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, false);
		if (IsZeroVector(livePosition))
		{
			evidence = "no living runtime member position is available";
			return false;
		}

		if (IsZeroVector(activeGroup.m_vPosition) || DistanceSq2D(activeGroup.m_vPosition, livePosition) >= 4.0)
		{
			activeGroup.m_vPosition = livePosition;
			m_bMarkerRefreshNeeded = true;
		}
		evidence = string.Format("live support position refreshed to %1", livePosition);
		return true;
	}

	bool IsActiveSupportGroupPhysicallyWithinDistance(HST_ActiveGroupState activeGroup, vector targetPosition, float radiusMeters, out string evidence)
	{
		evidence = "missing active support group";
		if (!activeGroup || activeGroup.m_sSupportRequestId.IsEmpty())
			return false;
		if (!activeGroup.m_bSpawnedEntity)
		{
			evidence = "active support group is not physically spawned";
			return false;
		}
		if (IsZeroVector(targetPosition))
		{
			evidence = "physical support target is missing";
			return false;
		}

		vector livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, false);
		if (IsZeroVector(livePosition))
		{
			evidence = "no living runtime member position is available";
			return false;
		}
		if (IsZeroVector(activeGroup.m_vPosition) || DistanceSq2D(activeGroup.m_vPosition, livePosition) >= 4.0)
		{
			activeGroup.m_vPosition = livePosition;
			m_bMarkerRefreshNeeded = true;
		}

		float distanceMeters = Math.Sqrt(DistanceSq2D(livePosition, targetPosition));
		float acceptedRadius = Math.Max(1.0, radiusMeters);
		evidence = string.Format("live %1 | target %2 | distance %3m/%4m", livePosition, targetPosition, Math.Round(distanceMeters), Math.Round(acceptedRadius));
		return distanceMeters <= acceptedRadius;
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
			bool shouldBeActive = !IsZoneInsideHQActivationExclusion(state, zone) && (IsAnyLivingPlayerNearZone(playerManager, playerIds, zone, balance) || forceMissionZone);
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

		m_CombatPresence.InvalidateCache();
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
		if (!zone || IsZoneInsideHQActivationExclusion(state, zone))
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

		bool pendingPopulationRemoved
			= ClearPendingActiveGroupPopulationForDebug(groupId);
		bool routeProgressRemoved
			= RemoveActiveGroupRouteProgressForDebug(groupId);
		bool existed = GetRuntimeGroupEntity(groupId) != null;
		DeleteRuntimeGroupEntity(groupId);
		return existed || pendingPopulationRemoved || routeProgressRemoved;
	}

	protected bool ClearPendingActiveGroupPopulationForDebug(string groupId)
	{
		if (groupId.IsEmpty())
			return false;
		SCR_AIGroup group = SCR_AIGroup.Cast(GetRuntimeGroupEntity(groupId));
		if (group)
			group.GetOnAllDelayedEntitySpawned().Remove(
				OnDelayedActiveGroupMembersSpawned);

		bool removed;
		for (int pendingIndex = m_aPendingPopulationGroupIds.Count() - 1; pendingIndex >= 0; pendingIndex--)
		{
			if (m_aPendingPopulationGroupIds[pendingIndex] != groupId)
				continue;
			HST_ActiveGroupState pendingGroup;
			if (pendingIndex < m_aPendingPopulationActiveGroups.Count())
				pendingGroup = m_aPendingPopulationActiveGroups[pendingIndex];
			if (pendingGroup)
			{
				// CallLater population retries retain this object directly. Moving
				// it out of spawn_pending_agents makes queued retries self-cancel.
				pendingGroup.m_bSpawnedEntity = false;
				pendingGroup.m_sRuntimeEntityId = "";
				pendingGroup.m_sRuntimeStatus = "campaign_debug_cleaned";
			}
			if (pendingIndex < m_aPendingPopulationRequestedStatuses.Count())
				m_aPendingPopulationRequestedStatuses.Remove(pendingIndex);
			if (pendingIndex < m_aPendingPopulationActiveGroups.Count())
				m_aPendingPopulationActiveGroups.Remove(pendingIndex);
			if (pendingIndex < m_aPendingPopulationStates.Count())
				m_aPendingPopulationStates.Remove(pendingIndex);
			m_aPendingPopulationGroupIds.Remove(pendingIndex);
			removed = true;
		}
		return removed;
	}

	protected bool RemoveActiveGroupRouteProgressForDebug(string groupId)
	{
		if (groupId.IsEmpty())
			return false;
		bool removed;
		for (int progressIndex = m_aActiveGroupRouteProgressStatuses.Count() - 1; progressIndex >= 0; progressIndex--)
		{
			HST_ActiveGroupRouteProgressStatus progress
				= m_aActiveGroupRouteProgressStatuses[progressIndex];
			if (!progress || progress.m_sGroupId != groupId)
				continue;
			m_aActiveGroupRouteProgressStatuses.Remove(progressIndex);
			removed = true;
		}
		return removed;
	}

	protected bool ShouldForceMissionZoneActive(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return false;

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					mission.m_sTargetZoneId, zone.m_sZoneId))
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

	bool CaptureCampaignDebugMarkerRefreshNeeded()
	{
		return m_bMarkerRefreshNeeded;
	}

	void RestoreCampaignDebugMarkerRefreshNeeded(bool markerRefreshNeeded)
	{
		m_bMarkerRefreshNeeded = markerRefreshNeeded;
	}

	int CaptureCampaignDebugAIWorldLimit()
	{
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return -1;

		return aiWorld.GetAILimit();
	}

	void RestoreCampaignDebugAIWorldLimit(int aiLimit)
	{
		if (aiLimit < 0)
			return;

		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld || aiWorld.GetAILimit() == aiLimit)
			return;

		aiWorld.SetAILimit(aiLimit);
	}

	string BuildConvoyRuntimeReport(HST_CampaignState state)
	{
		if (!state)
			return "Partisan convoy runtime | state not ready";

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

		string report = string.Format("Partisan convoy runtime | missions %1 | groups %2 | crew entities %3 | vehicle entities %4", convoyMissions, convoyGroups, m_aRuntimeGroupIds.Count(), m_aRuntimeVehicleGroupIds.Count());
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
		string report = "Partisan convoy ground vehicle candidates";
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
				string vehicleUsageEvidence;
				bool vehicleUsageRegistered = GetConvoyVehicleControlAdapter().IsVehicleRegisteredWithGroup(crewEntity, vehicleEntity, vehicleUsageEvidence);
				AddConvoyDebugProbeAssertion(probe, "convoy.vehicle_usage." + asset.m_sAssetId, "AI vehicle usage registered for group movement", string.Format("registered %1 | evidence %2 | waypoints %3 | mode %4 | reason %5", ReportBool(vehicleUsageRegistered), ReportText(vehicleUsageEvidence), activeGroup.m_iAssignedWaypointCount, ReportText(activeGroup.m_sSpawnFallbackMode), ReportText(activeGroup.m_sSpawnFailureReason)), ConvoyDebugStatus(vehicleUsageRegistered, vehicleUsageStatus), "convoy vehicle was not registered with the AI vehicle usage layer", groupId, mission.m_sInstanceId);
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
			result = "Partisan campaign debug | physical combat probe blocked: no controlled player entity";
			return false;
		}
		if (!state || !preset)
		{
			result = "Partisan campaign debug | physical combat probe failed: campaign state or preset missing";
			return false;
		}

		vector playerPosition;
		if (!ResolveFirstLivingPlayerDebugPosition(playerPosition))
		{
			result = "Partisan campaign debug | physical combat probe blocked: no living player render-bubble anchor";
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
			result = "Partisan campaign debug | physical combat probe failed: no zone available for active-group ownership";
			return false;
		}
		m_sCampaignDebugCombatProbeZoneId = zone.m_sZoneId;

		HST_ActiveGroupState friendlyGroup = CreateCampaignDebugPhysicalCombatProbeGroup(state, preset, zone, m_sCampaignDebugCombatProbeFriendlyGroupId, m_sCampaignDebugCombatProbeFriendlyFaction, m_vCampaignDebugCombatProbeFriendlyPosition, m_vCampaignDebugCombatProbeEnemyPosition);
		HST_ActiveGroupState enemyGroup = CreateCampaignDebugPhysicalCombatProbeGroup(state, preset, zone, m_sCampaignDebugCombatProbeEnemyGroupId, m_sCampaignDebugCombatProbeEnemyFaction, m_vCampaignDebugCombatProbeEnemyPosition, m_vCampaignDebugCombatProbeFriendlyPosition);
		if (!friendlyGroup || !enemyGroup)
		{
			result = "Partisan campaign debug | physical combat probe failed: could not build temporary active groups";
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
		result = "Partisan campaign debug | physical combat probe started | " + m_sCampaignDebugCombatProbeStartResult;
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
		activeGroup.m_sMissionInstanceId = mission.m_sInstanceId;
		activeGroup.m_sOperationId = HST_StableIdService.BuildOperationId("mission", mission.m_sInstanceId);
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
		activeGroup.m_sMissionInstanceId = mission.m_sInstanceId;
		activeGroup.m_sOperationId = HST_StableIdService.BuildOperationId("mission", mission.m_sInstanceId);
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

		PruneExactMissionConvoySettledSalvageVehicleHandles(state);
		bool changed = ReconcileExactMissionConvoyOutboundProjectionTransactions(state);
		changed = ReconcileExactMissionConvoyAbandonedVehiclePublication(state) || changed;
		changed = CleanupInactiveMissionConvoyRuntime(state) || changed;
		bool hasActiveMissionConvoy;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;
			if (IsTerminalMissionConvoyPhase(mission) && !IsExactMissionConvoyRecoveryHold(mission))
				continue;

			hasActiveMissionConvoy = true;
			changed = NormalizeRestoredMissionConvoyRuntime(state, preset, mission) || changed;
			changed = EnsureMissionConvoyRuntime(state, preset, mission) || changed;
			changed = ReconcileExactMissionConvoyDestroyedVehicleRuntime(state, mission) || changed;
			changed = UpdateMissionConvoyContact(state, mission) || changed;
		}

		if (hasActiveMissionConvoy)
			changed = UpdateRuntimeGroupSurvivors(state, true) || changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;
			if (IsTerminalMissionConvoyPhase(mission) && !IsExactMissionConvoyRecoveryHold(mission))
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
		if (IsPersistenceSmokeMission(mission)
			|| (IsTerminalMissionConvoyPhase(mission) && !IsExactMissionConvoyRecoveryHold(mission)))
			return false;

		bool changed = ReconcileExactMissionConvoyOutboundProjectionTransactions(state);
		changed = ReconcileExactMissionConvoyAbandonedVehiclePublication(state) || changed;
		changed = NormalizeRestoredMissionConvoyRuntime(state, preset, mission) || changed;
		changed = EnsureMissionConvoyRuntime(state, preset, mission) || changed;
		changed = ReconcileExactMissionConvoyDestroyedVehicleRuntime(state, mission) || changed;
		changed = UpdateRuntimeGroupSurvivors(state, true) || changed;
		changed = EnsureMissionConvoyCrewSeating(state, mission) || changed;
		changed = UpdateMissionConvoyContact(state, mission) || changed;
		changed = UpdateMissionConvoyPhase(state, mission, state.m_iElapsedSeconds) || changed;
		changed = UpdateMissionConvoyObjective(state, mission) || changed;
		return changed;
	}

	bool PrepareExactMissionConvoyDurableGroups(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!IsExactActiveMissionConvoy(mission) || !state || !preset)
			return false;
		if (!HasExactMissionConvoyVehicleAssetSet(state, mission))
			return false;

		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			if (!asset)
				return false;

			string groupId = BuildMissionConvoyGroupId(mission, index);
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			if (!activeGroup)
			{
				activeGroup = CreateMissionConvoyGroup(state, preset, mission, asset, index);
				if (!activeGroup)
					return false;
				state.m_aActiveGroups.Insert(activeGroup);
			}
			else if (activeGroup.m_sMissionInstanceId != mission.m_sInstanceId
				|| (!activeGroup.m_sOperationId.IsEmpty() && activeGroup.m_sOperationId != mission.m_sOperationId)
				|| (!activeGroup.m_sMissionAssetId.IsEmpty() && activeGroup.m_sMissionAssetId != asset.m_sAssetId)
				|| activeGroup.m_bSpawnedEntity || GetRuntimeCrewGroupEntity(groupId) || GetRuntimeVehicleEntity(groupId))
			{
				return false;
			}

			BindExactMissionConvoyGroupIdentity(state, activeGroup, mission, asset);
		}

		for (int verifyIndex = 0; verifyIndex < EXACT_MISSION_CONVOY_VEHICLE_COUNT; verifyIndex++)
		{
			if (!state.FindActiveGroup(BuildMissionConvoyGroupId(mission, verifyIndex)))
				return false;
		}

		return true;
	}

	bool HasExactMissionConvoyRuntime(HST_ActiveMissionState mission)
	{
		if (!IsExactMissionConvoyContract(mission))
			return false;
		// Roots created by an open outbound materialization transaction are not
		// published as durable physical evidence until the whole exact set is ready.
		// This prevents partial-root survivor reconciliation while another root is
		// still inside legitimate asynchronous population staging.
		if (FindExactMissionConvoyOutboundProjectionTransaction(mission.m_sInstanceId))
			return false;

		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			string groupId = BuildMissionConvoyGroupId(mission, index);
			if (GetRuntimeCrewGroupEntity(groupId) || GetRuntimeVehicleEntity(groupId))
				return true;
		}

		return false;
	}

	bool IsExactMissionConvoySurvivorProjectionReady(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactActiveMissionConvoy(mission))
			return false;
		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		if (!operation || (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL))
			return false;

		bool hasRequiredProjection;
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !element)
				return false;
			if (IsExactMissionConvoyRecoveryVehicleEligible(asset, element))
			{
				hasRequiredProjection = true;
				if (GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) || !asset.m_bSpawned || !element.m_bPhysicalized)
					return false;
				if (!GetExactMissionConvoyVehicleRuntimeEntity(state, mission, element.m_sVehicleSlotId))
					return false;
				continue;
			}
			if (element.m_iSurvivingCrewCount <= 0)
				continue;

			hasRequiredProjection = true;
			string authorityFailure = ValidateExactMissionConvoyCrewProjectionAuthority(state, mission, asset, activeGroup, element);
			if (!authorityFailure.IsEmpty())
				return false;
			if (!activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_failed"
				|| activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED)
				return false;
			IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
			if (!crewEntity || CountAliveRuntimeCrewAgents(activeGroup) != element.m_iSurvivingCrewCount
				|| !ExactMissionConvoyRuntimeCrewMatchesFrozenSurvivorSlots(state, mission, activeGroup, element))
				return false;

			IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
			if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE)
			{
				if (IsMissionConvoyVehicleAssetResolved(asset) || !asset.m_bSpawned || !vehicleEntity)
					return false;
				continue;
			}
			if (!IsExactMissionConvoyTerminalSurvivingCrew(asset, element))
				return false;
			bool registeredTerminalVehicle = HasRuntimeVehicleRegistration(activeGroup.m_sGroupId);
			bool observedTerminalTransition = vehicleEntity && registeredTerminalVehicle
				&& asset.m_bSpawned && element.m_bPhysicalized;
			if ((asset.m_bSpawned || vehicleEntity || registeredTerminalVehicle) && !observedTerminalTransition)
				return false;
		}

		return hasRequiredProjection;
	}

	IEntity GetExactMissionConvoyVehicleRuntimeEntity(HST_CampaignState state, HST_ActiveMissionState mission, string vehicleSlotId)
	{
		if (!state || !IsExactMissionConvoyContract(mission) || vehicleSlotId.IsEmpty())
			return null;
		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		if (!operation || operation.m_sManifestId.IsEmpty())
			return null;
		HST_ForceManifestState manifest = state.FindForceManifest(operation.m_sManifestId);
		if (!manifest || !manifest.m_bFrozen || manifest.m_sOperationId != operation.m_sOperationId)
			return null;

		HST_ForceManifestVehicleState manifestVehicle;
		int manifestVehicleClaimants;
		foreach (HST_ForceManifestVehicleState candidateVehicle : manifest.m_aVehicles)
		{
			if (!candidateVehicle || candidateVehicle.m_sSlotId != vehicleSlotId)
				continue;
			manifestVehicle = candidateVehicle;
			manifestVehicleClaimants++;
		}
		if (manifestVehicleClaimants != 1 || !manifestVehicle)
			return null;

		HST_ConvoyElementState element;
		int elementClaimants;
		foreach (HST_ConvoyElementState candidateElement : state.m_aConvoyElements)
		{
			if (!candidateElement || candidateElement.m_sOperationId != operation.m_sOperationId
				|| candidateElement.m_sMissionInstanceId != mission.m_sInstanceId || candidateElement.m_sVehicleSlotId != vehicleSlotId)
				continue;
			element = candidateElement;
			elementClaimants++;
		}
		if (elementClaimants != 1 || !element)
			return null;

		HST_MissionAssetState asset;
		int assetClaimants;
		foreach (HST_MissionAssetState candidateAsset : state.m_aMissionAssets)
		{
			if (!candidateAsset || candidateAsset.m_sMissionInstanceId != mission.m_sInstanceId
				|| candidateAsset.m_sOperationId != operation.m_sOperationId || candidateAsset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE
				|| candidateAsset.m_sManifestSlotId != vehicleSlotId)
				continue;
			asset = candidateAsset;
			assetClaimants++;
		}
		if (assetClaimants != 1 || !asset || IsMissionConvoyVehicleAssetResolved(asset))
			return null;

		HST_ActiveGroupState activeGroup;
		int groupClaimants;
		foreach (HST_ActiveGroupState candidateGroup : state.m_aActiveGroups)
		{
			if (!candidateGroup || candidateGroup.m_sMissionInstanceId != mission.m_sInstanceId
				|| candidateGroup.m_sOperationId != operation.m_sOperationId || candidateGroup.m_sConvoyElementId != element.m_sElementId)
				continue;
			activeGroup = candidateGroup;
			groupClaimants++;
		}
		if (groupClaimants != 1 || !activeGroup)
			return null;
		if (element.m_sManifestId != manifest.m_sManifestId || element.m_sVehicleAssetId != asset.m_sAssetId
			|| element.m_sGroupId != activeGroup.m_sGroupId || asset.m_sConvoyElementId != element.m_sElementId
			|| asset.m_sAssignedVehicleSlotId != vehicleSlotId || activeGroup.m_sMissionAssetId != asset.m_sAssetId
			|| manifestVehicle.m_sGroupElementId != element.m_sCrewGroupElementId
			|| manifestVehicle.m_sPrefab != element.m_sVehiclePrefab || manifestVehicle.m_sPrefab != asset.m_sPrefab)
			return null;

		IEntity runtimeVehicle;
		int runtimeClaimants;
		for (int runtimeIndex = 0; runtimeIndex < m_aRuntimeVehicleGroupIds.Count(); runtimeIndex++)
		{
			if (runtimeIndex >= m_aRuntimeVehicleEntities.Count())
				continue;
			IEntity candidateRuntime = m_aRuntimeVehicleEntities[runtimeIndex];
			if (!candidateRuntime)
				continue;
			if (m_aRuntimeVehicleGroupIds[runtimeIndex] == activeGroup.m_sGroupId)
			{
				runtimeVehicle = candidateRuntime;
				runtimeClaimants++;
			}
		}
		if (runtimeClaimants != 1 || !runtimeVehicle)
			return null;
		int runtimeEntityClaimants;
		for (int entityIndex = 0; entityIndex < m_aRuntimeVehicleEntities.Count(); entityIndex++)
		{
			if (m_aRuntimeVehicleEntities[entityIndex] != runtimeVehicle)
				continue;
			if (entityIndex >= m_aRuntimeVehicleGroupIds.Count() || m_aRuntimeVehicleGroupIds[entityIndex] != activeGroup.m_sGroupId)
				return null;
			runtimeEntityClaimants++;
		}
		if (runtimeEntityClaimants != 1)
			return null;
		return runtimeVehicle;
	}

	bool TryHandoffExactMissionConvoyVehicleCapture(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		string vehicleAssetId,
		out IEntity handedOffVehicle,
		out string reason)
	{
		handedOffVehicle = null;
		reason = "";
		if (!state || !IsExactMissionConvoyContract(mission) || vehicleAssetId.IsEmpty())
		{
			reason = "exact convoy capture handoff state, mission, or vehicle asset ID is missing";
			return false;
		}
		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		bool settledSalvageCapture;
		if (!operation)
		{
			operation = ResolveSettledExactMissionConvoyOperationForRetirement(state, mission);
			settledSalvageCapture = operation != null;
		}
		if (!operation || (!settledSalvageCapture
			&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL))
		{
			reason = "exact convoy capture handoff requires an open physical operation or a settled salvage receipt";
			return false;
		}

		HST_MissionAssetState asset;
		int assetClaimants;
		foreach (HST_MissionAssetState candidateAsset : state.m_aMissionAssets)
		{
			if (!candidateAsset || candidateAsset.m_sAssetId != vehicleAssetId)
				continue;
			asset = candidateAsset;
			assetClaimants++;
		}
		if (assetClaimants != 1 || !asset || asset.m_sMissionInstanceId != mission.m_sInstanceId
			|| asset.m_sOperationId != operation.m_sOperationId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE
			|| ResolveExactMissionConvoyVehicleAssetIndex(mission, asset) < 0
			|| IsMissionConvoyVehicleAssetResolved(asset) || asset.m_sManifestSlotId.IsEmpty())
		{
			reason = "exact convoy capture handoff vehicle asset identity is missing, ambiguous, or already terminal";
			return false;
		}

		int vehicleIndex = ResolveExactMissionConvoyVehicleAssetIndex(mission, asset);
		HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, vehicleIndex));
		HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
		if (settledSalvageCapture)
			return TryHandoffExactMissionConvoySettledSalvageVehicleCapture(state, mission, operation, asset, activeGroup, element, handedOffVehicle, reason);
		bool activeCaptureRoot = element
			&& element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE;
		bool abandonedCaptureRoot = IsExactMissionConvoyRecoveryVehicleEligible(asset, element);
		if (!activeGroup || !element || (!activeCaptureRoot && !abandonedCaptureRoot)
			|| !asset.m_bSpawned || !element.m_bPhysicalized
			|| activeGroup.m_sOperationId != operation.m_sOperationId
			|| activeGroup.m_sMissionAssetId != asset.m_sAssetId)
		{
			reason = "exact convoy capture handoff group or element identity is missing, ambiguous, or not an active/recoverable abandoned root";
			return false;
		}
		string authorityFailure = ValidateExactMissionConvoyCrewProjectionAuthority(state, mission, asset, activeGroup, element);
		if (!authorityFailure.IsEmpty())
		{
			reason = "exact convoy capture handoff frozen authority rejected: " + authorityFailure;
			return false;
		}
		if (IsConvoyCrewPopulationPending(state, activeGroup) || IsConvoyCrewControlPending(state, activeGroup))
		{
			reason = "exact convoy capture handoff is blocked by crew population or seating grace";
			return false;
		}
		if (activeCaptureRoot && !activeGroup.m_bEverHadLivingCrew && activeGroup.m_iMaxObservedCrewAlive <= 0)
		{
			reason = "exact convoy capture handoff has no proof that its frozen crew ever materialized";
			return false;
		}
		if (CountAliveRuntimeCrewAgents(activeGroup) > 0)
		{
			reason = "exact convoy capture handoff requires zero living crew";
			return false;
		}

		IEntity vehicleEntity = GetExactMissionConvoyVehicleRuntimeEntity(state, mission, element.m_sVehicleSlotId);
		if (!vehicleEntity)
		{
			reason = "exact convoy capture handoff has no unique tracked authoritative vehicle";
			return false;
		}
		if (IsAnyPlayerInVehicle(vehicleEntity))
		{
			reason = "exact convoy capture handoff is blocked while a living player occupies the vehicle";
			return false;
		}
		if (IsForceSpawnRuntimeOwnershipHeldForGroup(activeGroup.m_sGroupId))
		{
			reason = "exact convoy capture handoff vehicle is held by another runtime lifecycle";
			return false;
		}

		vector position = vehicleEntity.GetOrigin();
		if (IsZeroVector(position))
		{
			reason = "exact convoy capture handoff vehicle has no valid world position";
			return false;
		}
		float damageFraction = element.m_fVehicleDamageFraction;
		float fuelFraction = element.m_fFuelFraction;
		float ammoFraction = element.m_fAmmoFraction;
		TrySampleMissionConvoyVehicleDamageFraction(vehicleEntity, damageFraction);
		TrySampleMissionConvoyVehicleFuelFraction(vehicleEntity, fuelFraction);
		TrySampleMissionConvoyVehicleAmmoFraction(vehicleEntity, ammoFraction);
		if (damageFraction >= 0.999)
		{
			reason = "exact convoy capture handoff rejected a destroyed vehicle";
			return false;
		}

		ref array<ref HST_ConvoyElementState> rosterElements = {};
		ref array<int> rosterSurvivors = {};
		rosterElements.Insert(element);
		rosterSurvivors.Insert(0);
		HST_ExactMissionConvoyRosterMutationPlan rosterPlan;
		string rosterFailure = BuildExactMissionConvoyRosterMutationPlan(state, mission, rosterElements, rosterSurvivors, rosterPlan);
		if (!rosterFailure.IsEmpty())
		{
			reason = "exact convoy capture handoff survivor roster rejected: " + rosterFailure;
			return false;
		}

		int vehicleHandleIndex = -1;
		int vehicleHandleClaimants;
		for (int handleIndex = 0; handleIndex < m_aRuntimeVehicleGroupIds.Count(); handleIndex++)
		{
			if (handleIndex >= m_aRuntimeVehicleEntities.Count()
				|| m_aRuntimeVehicleGroupIds[handleIndex] != activeGroup.m_sGroupId
				|| m_aRuntimeVehicleEntities[handleIndex] != vehicleEntity)
				continue;
			vehicleHandleIndex = handleIndex;
			vehicleHandleClaimants++;
		}
		if (vehicleHandleClaimants != 1 || vehicleHandleIndex < 0)
		{
			reason = "exact convoy capture handoff vehicle handle is missing or ambiguous";
			return false;
		}

		ApplyExactMissionConvoyRosterMutationPlan(rosterPlan);
		m_aRuntimeVehicleEntities.Remove(vehicleHandleIndex);
		m_aRuntimeVehicleGroupIds.Remove(vehicleHandleIndex);
		HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(vehicleEntity);

		activeGroup.m_iInfantryCount = 0;
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_iDurableLivingInfantryCount = 0;
		activeGroup.m_iSurvivorVehicleCount = 0;
		activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
		activeGroup.m_sConvoyRuntimeStage = "CAPTURE_HANDOFF";
		activeGroup.m_iAssignedWaypointCount = 0;
		activeGroup.m_vPosition = position;
		activeGroup.m_vSourcePosition = position;
		activeGroup.m_iLifecycleRevision++;

		asset.m_bSpawned = false;
		asset.m_bPickedUp = true;
		asset.m_bDelivered = true;
		asset.m_bDestroyed = false;
		asset.m_bAlive = true;
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_sLastInteraction = "captured";
		asset.m_vCurrentPosition = position;
		asset.m_vLastKnownPosition = position;

		element.m_vCurrentPosition = position;
		element.m_iSurvivingCrewCount = 0;
		element.m_fVehicleDamageFraction = damageFraction;
		element.m_fFuelFraction = fuelFraction;
		element.m_fAmmoFraction = ammoFraction;
		element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED;
		element.m_bPhysicalized = false;
		element.m_bMobile = false;
		element.m_sTerminalReason = "vehicle captured through exact runtime handoff";
		element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
		element.m_iRevision++;
		SyncExactMissionConvoyRecoveryCarrierAssets(state, mission, element, position);

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bSpawned = false;
			runtimeEntity.m_bDestroyed = false;
			runtimeEntity.m_bRecovered = true;
			runtimeEntity.m_vPosition = position;
		}
		handedOffVehicle = vehicleEntity;
		m_bMarkerRefreshNeeded = true;
		reason = "exact convoy vehicle capture handoff completed without garage mutation";
		Print(string.Format("Partisan exact mission convoy | handed off captured vehicle %1 at %2", asset.m_sAssetId, position));
		return true;
	}

	protected bool TryHandoffExactMissionConvoySettledSalvageVehicleCapture(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_OperationRecordState operation,
		HST_MissionAssetState asset,
		HST_ActiveGroupState activeGroup,
		HST_ConvoyElementState element,
		out IEntity handedOffVehicle,
		out string reason)
	{
		handedOffVehicle = null;
		reason = "";
		if (!operation || operation != ResolveSettledExactMissionConvoyOperationForRetirement(state, mission)
			|| !IsExactMissionConvoySettledSalvageCaptureAuthority(state, mission, asset, activeGroup, element))
		{
			reason = "settled exact convoy salvage capture authority is missing or conflicting";
			return false;
		}

		IEntity vehicleEntity;
		int salvageHandleIndex;
		if (!TryGetExactMissionConvoySettledSalvageVehicleHandle(mission, asset, activeGroup, vehicleEntity, salvageHandleIndex)
			|| !vehicleEntity || vehicleEntity.IsDeleted()
			|| ResolveEntityPrefabName(vehicleEntity) != asset.m_sPrefab)
		{
			reason = "settled exact convoy salvage vehicle has no unique retained entity identity";
			return false;
		}
		if (GetRuntimeVehicleEntity(activeGroup.m_sGroupId) || HasRuntimeVehicleRegistration(activeGroup.m_sGroupId)
			|| GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId)
			|| CountExactMissionConvoyMemberMappings(mission.m_sInstanceId, activeGroup.m_sGroupId) != 0)
		{
			reason = "settled exact convoy salvage vehicle still overlaps an open runtime lifecycle";
			return false;
		}
		if (IsAnyPlayerInVehicle(vehicleEntity))
		{
			reason = "settled exact convoy salvage capture is blocked while a living player occupies the vehicle";
			return false;
		}
		if (IsForceSpawnRuntimeOwnershipHeldForGroup(activeGroup.m_sGroupId))
		{
			reason = "settled exact convoy salvage vehicle is held by another runtime lifecycle";
			return false;
		}

		vector position = vehicleEntity.GetOrigin();
		if (IsZeroVector(position))
		{
			reason = "settled exact convoy salvage vehicle has no valid world position";
			return false;
		}
		float damageFraction = element.m_fVehicleDamageFraction;
		float fuelFraction = element.m_fFuelFraction;
		float ammoFraction = element.m_fAmmoFraction;
		TrySampleMissionConvoyVehicleDamageFraction(vehicleEntity, damageFraction);
		TrySampleMissionConvoyVehicleFuelFraction(vehicleEntity, fuelFraction);
		TrySampleMissionConvoyVehicleAmmoFraction(vehicleEntity, ammoFraction);
		if (damageFraction >= 0.999)
		{
			reason = "settled exact convoy salvage capture rejected a destroyed vehicle";
			return false;
		}

		RemoveExactMissionConvoySettledSalvageVehicleHandleAt(salvageHandleIndex);
		HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(vehicleEntity);
		activeGroup.m_iInfantryCount = 0;
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_iDurableLivingInfantryCount = 0;
		activeGroup.m_iSurvivorVehicleCount = 0;
		activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
		activeGroup.m_sConvoyRuntimeStage = "SETTLED_SALVAGE_HANDOFF";
		activeGroup.m_iAssignedWaypointCount = 0;
		activeGroup.m_vPosition = position;
		activeGroup.m_vSourcePosition = position;
		activeGroup.m_iLifecycleRevision++;

		asset.m_bSpawned = false;
		asset.m_bPickedUp = true;
		asset.m_bDelivered = true;
		asset.m_bDestroyed = false;
		asset.m_bAlive = true;
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_sLastInteraction = "captured";
		asset.m_vCurrentPosition = position;
		asset.m_vLastKnownPosition = position;

		element.m_vCurrentPosition = position;
		element.m_iSurvivingCrewCount = 0;
		element.m_fVehicleDamageFraction = damageFraction;
		element.m_fFuelFraction = fuelFraction;
		element.m_fAmmoFraction = ammoFraction;
		element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED;
		element.m_bPhysicalized = false;
		element.m_bMobile = false;
		element.m_sTerminalReason = "vehicle captured through settled exact salvage handoff";
		element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
		element.m_iRevision++;
		SyncExactMissionConvoyRecoveryCarrierAssets(state, mission, element, position);

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bSpawned = false;
			runtimeEntity.m_bDestroyed = false;
			runtimeEntity.m_bRecovered = true;
			runtimeEntity.m_vPosition = position;
		}
		handedOffVehicle = vehicleEntity;
		m_bMarkerRefreshNeeded = true;
		reason = "settled exact convoy salvage vehicle handoff completed without reopening operation authority";
		Print(string.Format("Partisan exact mission convoy | handed off settled salvage vehicle %1 at %2", asset.m_sAssetId, position));
		return true;
	}

	protected bool ReconcileExactMissionConvoyDestroyedVehicleRuntime(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMissionConvoyContract(mission) || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;
		bool changed;
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !element)
				continue;
			bool durableDestructionResolved = asset.m_bDestroyed
				&& element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED
				&& element.m_fVehicleDamageFraction >= 0.999 && !element.m_bMobile
				&& activeGroup.m_iSurvivorVehicleCount == 0 && !element.m_sTerminalReason.IsEmpty();
			if (durableDestructionResolved)
			{
				vector durablePosition = element.m_vCurrentPosition;
				if (IsZeroVector(durablePosition))
					durablePosition = asset.m_vCurrentPosition;
				if (IsZeroVector(durablePosition))
					durablePosition = asset.m_vLastKnownPosition;
				if (IsZeroVector(durablePosition))
					durablePosition = activeGroup.m_vPosition;
				SyncExactMissionConvoyRecoveryCarrierAssets(state, mission, element, durablePosition);
				continue;
			}
			IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
			float damageFraction = element.m_fVehicleDamageFraction;
			bool runtimeDestroyed;
			if (vehicleEntity)
				runtimeDestroyed = TrySampleMissionConvoyVehicleDamageFraction(vehicleEntity, damageFraction) && damageFraction >= 0.999;
			if (!asset.m_bDestroyed && !runtimeDestroyed)
				continue;

			vector position;
			if (vehicleEntity)
				position = vehicleEntity.GetOrigin();
			if (IsZeroVector(position))
				position = element.m_vCurrentPosition;
			if (IsZeroVector(position))
				position = asset.m_vCurrentPosition;
			if (IsZeroVector(position))
				position = asset.m_vLastKnownPosition;
			if (IsZeroVector(position))
				position = activeGroup.m_vPosition;
			float fuelFraction = element.m_fFuelFraction;
			float ammoFraction = element.m_fAmmoFraction;
			if (vehicleEntity)
			{
				TrySampleMissionConvoyVehicleFuelFraction(vehicleEntity, fuelFraction);
				TrySampleMissionConvoyVehicleAmmoFraction(vehicleEntity, ammoFraction);
			}
			bool assetWasDestroyed = asset.m_bDestroyed;
			if (!assetWasDestroyed)
				MarkMissionConvoyVehicleDestroyed(state, mission, asset, activeGroup.m_sGroupId, position, "vehicle destruction confirmed by exact runtime condition sample");
			else if (!IsZeroVector(position))
			{
				asset.m_vCurrentPosition = position;
				asset.m_vLastKnownPosition = position;
			}
			activeGroup.m_iSurvivorVehicleCount = 0;
			element.m_fVehicleDamageFraction = 1.0;
			element.m_fFuelFraction = fuelFraction;
			element.m_fAmmoFraction = ammoFraction;
			element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED;
			element.m_bMobile = false;
			if (!IsZeroVector(position))
				element.m_vCurrentPosition = position;
			if (assetWasDestroyed)
				element.m_sTerminalReason = "vehicle destruction reconciled from exact durable asset state";
			else
				element.m_sTerminalReason = "vehicle destruction confirmed by exact runtime condition sample";
			element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			element.m_iRevision++;
			SyncExactMissionConvoyRecoveryCarrierAssets(state, mission, element, position);
			m_bMarkerRefreshNeeded = true;
			changed = true;
		}
		return changed;
	}

	bool MaterializeExactMissionConvoyRecoveryVehicles(HST_CampaignState state, HST_ActiveMissionState mission, out string reason)
	{
		reason = "";
		bool recoveryHold = IsExactMissionConvoyRecoveryHold(mission);
		if (!state || (!IsExactActiveMissionConvoy(mission) && !recoveryHold))
		{
			reason = "exact convoy operation is not active";
			return false;
		}
		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		if (!operation || (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL))
		{
			reason = "exact convoy recovery operation is not materializing or physical";
			return false;
		}
		SetExactMissionConvoyAbandonedVehicleProjectionPublished(state, mission, false);
		if (recoveryHold)
		{
			string carrierFailure = ValidateExactMissionConvoyRecoveryCarrierAvailability(state, mission);
			if (!carrierFailure.IsEmpty())
			{
				reason = carrierFailure;
				return false;
			}
			FreezeExactMissionConvoyTerminalRecoveryCargo(state, mission);
		}
		string projectionStage = "ABANDONED_VEHICLE";
		if (recoveryHold)
			projectionStage = "RECOVERY_VEHICLE";

		ref array<ref HST_MissionAssetState> assets = {};
		ref array<ref HST_ActiveGroupState> groups = {};
		ref array<ref HST_ConvoyElementState> elements = {};
		ref array<string> frozenPrefabs = {};
		ref array<vector> originalAssetPositions = {};
		ref array<vector> originalAssetLastPositions = {};
		ref array<vector> originalElementPositions = {};
		ref array<vector> originalGroupPositions = {};
		ref array<vector> originalGroupSourcePositions = {};
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !element)
			{
				reason = string.Format("exact convoy recovery element %1 is missing or ambiguous", index);
				return false;
			}
			if (!IsExactMissionConvoyRecoveryVehicleEligible(asset, element))
				continue;
			if (IsForceSpawnRuntimeOwnershipHeldForGroup(activeGroup.m_sGroupId))
			{
				reason = string.Format("exact convoy abandoned element %1 is owned by another runtime lifecycle", index);
				return false;
			}

			string frozenPrefab;
			if (!TryResolveExactMissionConvoyFrozenVehiclePrefab(state, mission, asset, activeGroup, frozenPrefab))
			{
				reason = string.Format("exact convoy recovery element %1 has no valid frozen vehicle prefab", index);
				return false;
			}
			if (GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId))
			{
				reason = string.Format("exact convoy recovery element %1 unexpectedly owns a crew runtime", index);
				return false;
			}

			IEntity existingVehicle = GetExactMissionConvoyVehicleRuntimeEntity(state, mission, element.m_sVehicleSlotId);
			if (!existingVehicle && (GetRuntimeVehicleEntity(activeGroup.m_sGroupId) || HasRuntimeVehicleRegistration(activeGroup.m_sGroupId)))
			{
				reason = string.Format("exact convoy recovery element %1 has an ambiguous runtime vehicle handle", index);
				return false;
			}

			assets.Insert(asset);
			groups.Insert(activeGroup);
			elements.Insert(element);
			frozenPrefabs.Insert(frozenPrefab);
			originalAssetPositions.Insert(asset.m_vCurrentPosition);
			originalAssetLastPositions.Insert(asset.m_vLastKnownPosition);
			originalElementPositions.Insert(element.m_vCurrentPosition);
			originalGroupPositions.Insert(activeGroup.m_vPosition);
			originalGroupSourcePositions.Insert(activeGroup.m_vSourcePosition);
		}
		if (assets.Count() == 0)
		{
			reason = "exact convoy recovery has no surviving unclaimed vehicle roots";
			return false;
		}

		ref array<string> spawnedGroupIds = {};
		for (int spawnIndex = 0; spawnIndex < assets.Count(); spawnIndex++)
		{
			HST_MissionAssetState asset = assets[spawnIndex];
			HST_ActiveGroupState activeGroup = groups[spawnIndex];
			HST_ConvoyElementState element = elements[spawnIndex];
			IEntity existingVehicle = GetExactMissionConvoyVehicleRuntimeEntity(state, mission, element.m_sVehicleSlotId);
			if (existingVehicle)
			{
				SetExactMissionConvoyProjectionEntityPublished(existingVehicle, false);
				vector existingPosition = existingVehicle.GetOrigin();
				if (!IsZeroVector(existingPosition))
				{
					asset.m_vCurrentPosition = existingPosition;
					asset.m_vLastKnownPosition = existingPosition;
					element.m_vCurrentPosition = existingPosition;
					activeGroup.m_vPosition = existingPosition;
					activeGroup.m_vSourcePosition = existingPosition;
				}
				asset.m_bSpawned = true;
				element.m_bPhysicalized = true;
				activeGroup.m_iSurvivorVehicleCount = 1;
				activeGroup.m_sConvoyRuntimeStage = projectionStage;
				SyncExactMissionConvoyRecoveryCarrierAssets(state, mission, element, element.m_vCurrentPosition);
				continue;
			}
			vector spawnPosition;
			if (!TryResolveMissionConvoyVehicleSpawnPosition(mission, asset, spawnPosition) || HST_WorldPositionService.IsLikelyOpenWater(spawnPosition))
			{
				RollbackExactMissionConvoyRecoveryVehicles(state, mission, assets, groups, elements, spawnedGroupIds, originalAssetPositions, originalAssetLastPositions, originalElementPositions, originalGroupPositions, originalGroupSourcePositions);
				reason = string.Format("exact convoy recovery vehicle %1 has no dry vehicle-safe position", asset.m_sAssetId);
				return false;
			}

			GenericEntity vehicleEntity = SpawnMissionConvoyVehicle(state, null, mission, asset, activeGroup, frozenPrefabs[spawnIndex], spawnPosition, element.m_iOrdinal);
			if (!vehicleEntity)
			{
				RollbackExactMissionConvoyRecoveryVehicles(state, mission, assets, groups, elements, spawnedGroupIds, originalAssetPositions, originalAssetLastPositions, originalElementPositions, originalGroupPositions, originalGroupSourcePositions);
				reason = string.Format("exact convoy recovery vehicle %1 failed to spawn from its frozen prefab", asset.m_sAssetId);
				return false;
			}
			SetExactMissionConvoyProjectionEntityPublished(vehicleEntity, false);

			m_aRuntimeVehicleGroupIds.Insert(activeGroup.m_sGroupId);
			m_aRuntimeVehicleEntities.Insert(vehicleEntity);
			spawnedGroupIds.Insert(activeGroup.m_sGroupId);
			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_bSpawnAttempted = true;
			activeGroup.m_iSpawnedAgentCount = 0;
			activeGroup.m_iAssignedWaypointCount = 0;
			activeGroup.m_iSurvivorVehicleCount = 1;
			activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
			activeGroup.m_sConvoyRuntimeStage = projectionStage;
			activeGroup.m_vPosition = spawnPosition;
			activeGroup.m_vSourcePosition = spawnPosition;
			asset.m_bSpawned = true;
			element.m_vCurrentPosition = spawnPosition;
			element.m_bPhysicalized = true;
			element.m_bMobile = false;
			element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			element.m_iRevision++;
			Print(string.Format("Partisan exact mission convoy | materialized crewless abandoned vehicle %1 at %2", asset.m_sAssetId, spawnPosition));
		}
		for (int carrierIndex = 0; carrierIndex < elements.Count(); carrierIndex++)
			SyncExactMissionConvoyRecoveryCarrierAssets(state, mission, elements[carrierIndex], elements[carrierIndex].m_vCurrentPosition);
		bool publishProjection = operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
		if (publishProjection && recoveryHold)
			publishProjection = IsExactMissionConvoyRecoveryProjectionReady(state, mission);
		else if (publishProjection)
			publishProjection = IsExactMissionConvoySurvivorProjectionReady(state, mission);
		SetExactMissionConvoyAbandonedVehicleProjectionPublished(state, mission, publishProjection);

		m_bMarkerRefreshNeeded = true;
		reason = string.Format("materialized %1 exact crewless abandoned vehicles", spawnedGroupIds.Count());
		return true;
	}

	bool IsExactMissionConvoyRecoveryProjectionReady(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMissionConvoyRecoveryHold(mission))
			return false;
		if (!ValidateExactMissionConvoyRecoveryCarrierAvailability(state, mission).IsEmpty())
			return false;
		bool hasRequiredProjection;
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !element)
				return false;
			if (!IsExactMissionConvoyRecoveryVehicleEligible(asset, element))
				continue;
			hasRequiredProjection = true;
			if (GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) || !asset.m_bSpawned || !element.m_bPhysicalized)
				return false;
			if (!GetExactMissionConvoyVehicleRuntimeEntity(state, mission, element.m_sVehicleSlotId))
				return false;
		}

		foreach (HST_MissionAssetState cargo : state.m_aMissionAssets)
		{
			if (!IsExactMissionConvoyUnresolvedRecoveryCargo(mission, cargo))
				continue;
			HST_ConvoyElementState carrierElement = ResolveExactMissionConvoyRecoveryCarrierElement(state, mission, cargo.m_sAssignedVehicleSlotId);
			if (!carrierElement)
				return false;
			HST_MissionAssetState carrierAsset = state.FindMissionAsset(carrierElement.m_sVehicleAssetId);
			if (!IsExactMissionConvoyRecoveryVehicleEligible(carrierAsset, carrierElement)
				&& !IsExactMissionConvoyTerminalRecoveryCarrier(carrierAsset, carrierElement))
				return false;

			HST_MissionRuntimeEntityState runtimeCargo = state.FindMissionRuntimeEntity(cargo.m_sEntityId);
			if (!cargo.m_bSpawned || !runtimeCargo || !runtimeCargo.m_bSpawned)
				return false;
			if (IsExactMissionConvoyTerminalRecoveryCarrier(carrierAsset, carrierElement)
				&& (cargo.m_bAttachedToCarrier || !cargo.m_sCarriedByVehicleId.IsEmpty()))
				return false;
			hasRequiredProjection = true;
		}
		return hasRequiredProjection;
	}

	protected bool IsExactMissionConvoyRecoveryVehicleEligible(HST_MissionAssetState asset, HST_ConvoyElementState element)
	{
		if (!asset || !element || IsMissionConvoyVehicleAssetResolved(asset))
			return false;
		if (element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED
			|| element.m_iSurvivingCrewCount > 0 || element.m_bMobile)
			return false;
		return element.m_fVehicleDamageFraction >= 0.0 && element.m_fVehicleDamageFraction < 1.0;
	}

	protected string ValidateExactMissionConvoyRecoveryCarrierAvailability(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "exact convoy recovery state or mission is missing";
		foreach (HST_MissionAssetState cargo : state.m_aMissionAssets)
		{
			if (!cargo || cargo.m_sMissionInstanceId != mission.m_sInstanceId || cargo.m_sAssignedVehicleSlotId.IsEmpty())
				continue;
			if (cargo.m_sRole != MISSION_CONVOY_PAYLOAD_ROLE && cargo.m_sRole != MISSION_CONVOY_CAPTIVE_ROLE)
				continue;
			if (cargo.m_bPickedUp || cargo.m_bDelivered || cargo.m_bDestroyed || cargo.m_bOutcomeApplied)
				continue;

			HST_ConvoyElementState carrierElement = ResolveExactMissionConvoyRecoveryCarrierElement(state, mission, cargo.m_sAssignedVehicleSlotId);
			if (!carrierElement)
				return string.Format("exact convoy recovery cargo %1 has a missing or ambiguous carrier slot", cargo.m_sAssetId);
			HST_MissionAssetState carrierAsset = state.FindMissionAsset(carrierElement.m_sVehicleAssetId);
			if (!carrierAsset || carrierAsset.m_sMissionInstanceId != mission.m_sInstanceId
				|| carrierAsset.m_sManifestSlotId != cargo.m_sAssignedVehicleSlotId)
				return string.Format("exact convoy recovery cargo %1 carrier asset backlink is missing or ambiguous", cargo.m_sAssetId);
			if (IsExactMissionConvoyRecoveryVehicleEligible(carrierAsset, carrierElement))
				continue;
			if (!IsExactMissionConvoyTerminalRecoveryCarrier(carrierAsset, carrierElement))
				return string.Format("exact convoy recovery cargo %1 carrier vehicle is not an eligible or terminal resolved exact root", cargo.m_sAssetId);

			vector groundPosition = carrierElement.m_vCurrentPosition;
			if (IsZeroVector(groundPosition))
				groundPosition = carrierAsset.m_vCurrentPosition;
			if (IsZeroVector(groundPosition))
				groundPosition = carrierAsset.m_vLastKnownPosition;
			if (IsZeroVector(groundPosition))
				groundPosition = cargo.m_vCurrentPosition;
			if (IsZeroVector(groundPosition))
				return string.Format("exact convoy recovery cargo %1 terminal carrier has no durable ground position", cargo.m_sAssetId);

		}
		return "";
	}

	protected bool IsExactMissionConvoyUnresolvedRecoveryCargo(HST_ActiveMissionState mission, HST_MissionAssetState cargo)
	{
		if (!mission || !cargo || cargo.m_sMissionInstanceId != mission.m_sInstanceId || cargo.m_sAssignedVehicleSlotId.IsEmpty())
			return false;
		if (cargo.m_sRole != MISSION_CONVOY_PAYLOAD_ROLE && cargo.m_sRole != MISSION_CONVOY_CAPTIVE_ROLE)
			return false;
		return !cargo.m_bPickedUp && !cargo.m_bDelivered && !cargo.m_bDestroyed && !cargo.m_bOutcomeApplied;
	}

	protected HST_ConvoyElementState ResolveExactMissionConvoyRecoveryCarrierElement(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		string vehicleSlotId)
	{
		if (!state || !mission || vehicleSlotId.IsEmpty())
			return null;
		HST_ConvoyElementState carrierElement;
		int carrierClaimants;
		foreach (HST_ConvoyElementState candidate : state.m_aConvoyElements)
		{
			if (!candidate || candidate.m_sOperationId != mission.m_sOperationId
				|| candidate.m_sMissionInstanceId != mission.m_sInstanceId || candidate.m_sVehicleSlotId != vehicleSlotId)
				continue;
			carrierElement = candidate;
			carrierClaimants++;
		}
		if (carrierClaimants != 1)
			return null;
		return carrierElement;
	}

	protected bool IsExactMissionConvoyTerminalRecoveryCarrier(HST_MissionAssetState carrierAsset, HST_ConvoyElementState carrierElement)
	{
		if (!carrierAsset || !carrierElement || !IsMissionConvoyVehicleAssetResolved(carrierAsset)
			|| carrierElement.m_iSurvivingCrewCount > 0 || carrierElement.m_bMobile)
			return false;
		if (carrierElement.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED)
			return carrierAsset.m_bDestroyed;
		if (carrierElement.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED)
			return carrierAsset.m_bDelivered || carrierAsset.m_sLastInteraction == "captured";
		return false;
	}

	protected void FreezeExactMissionConvoyTerminalRecoveryCargo(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;
		foreach (HST_MissionAssetState cargo : state.m_aMissionAssets)
		{
			if (!IsExactMissionConvoyUnresolvedRecoveryCargo(mission, cargo))
				continue;
			HST_ConvoyElementState carrier = ResolveExactMissionConvoyRecoveryCarrierElement(state, mission, cargo.m_sAssignedVehicleSlotId);
			if (!carrier)
				continue;
			HST_MissionAssetState carrierAsset = state.FindMissionAsset(carrier.m_sVehicleAssetId);
			if (!IsExactMissionConvoyTerminalRecoveryCarrier(carrierAsset, carrier))
				continue;
			vector position = carrier.m_vCurrentPosition;
			if (IsZeroVector(position))
				position = carrierAsset.m_vCurrentPosition;
			if (IsZeroVector(position))
				position = carrierAsset.m_vLastKnownPosition;
			if (IsZeroVector(position))
				position = cargo.m_vCurrentPosition;
			if (IsZeroVector(position))
				continue;
			cargo.m_vCurrentPosition = position;
			cargo.m_vLastKnownPosition = position;
			cargo.m_bAttachedToCarrier = false;
			cargo.m_sCarriedByVehicleId = "";
		}
	}

	protected void SyncExactMissionConvoyRecoveryCarrierAssets(HST_CampaignState state, HST_ActiveMissionState mission, HST_ConvoyElementState carrier, vector position)
	{
		if (!state || !mission || !carrier || carrier.m_sVehicleSlotId.IsEmpty() || IsZeroVector(position))
			return;
		HST_MissionAssetState carrierAsset = state.FindMissionAsset(carrier.m_sVehicleAssetId);
		bool terminalResolvedCarrier = carrierAsset && IsMissionConvoyVehicleAssetResolved(carrierAsset)
			&& (carrier.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED
				|| carrier.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED);
		foreach (HST_MissionAssetState cargo : state.m_aMissionAssets)
		{
			if (!cargo || cargo.m_sMissionInstanceId != mission.m_sInstanceId
				|| cargo.m_sAssignedVehicleSlotId != carrier.m_sVehicleSlotId || cargo.m_sAssetId == carrier.m_sVehicleAssetId)
				continue;
			if (cargo.m_bPickedUp || cargo.m_bDelivered || cargo.m_bDestroyed)
				continue;
			cargo.m_vCurrentPosition = position;
			cargo.m_vLastKnownPosition = position;
			cargo.m_bAttachedToCarrier = !terminalResolvedCarrier;
			if (terminalResolvedCarrier)
				cargo.m_sCarriedByVehicleId = "";
			else
				cargo.m_sCarriedByVehicleId = carrier.m_sVehicleAssetId;
		}
	}

	protected bool HasUnsafeExactMissionConvoyCargoCarrierClaim(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState cargo)
	{
		if (!state || !mission || !cargo || cargo.m_sMissionInstanceId != mission.m_sInstanceId)
			return true;
		if (cargo.m_sRole != MISSION_CONVOY_PAYLOAD_ROLE && cargo.m_sRole != MISSION_CONVOY_CAPTIVE_ROLE)
			return false;
		if (!cargo.m_bPickedUp && !cargo.m_bDelivered)
			return false;
		if (cargo.m_bAttachedToCarrier && cargo.m_sCarriedByVehicleId.IsEmpty())
			return true;
		if (cargo.m_sCarriedByVehicleId.IsEmpty())
			return false;

		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState carrierAsset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			if (!carrierAsset)
				return true;
			if (cargo.m_sCarriedByVehicleId == carrierAsset.m_sAssetId
				|| (!carrierAsset.m_sEntityId.IsEmpty() && cargo.m_sCarriedByVehicleId == carrierAsset.m_sEntityId))
				return true;
		}
		return false;
	}

	protected void RollbackExactMissionConvoyRecoveryVehicles(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		array<ref HST_MissionAssetState> assets,
		array<ref HST_ActiveGroupState> groups,
		array<ref HST_ConvoyElementState> elements,
		array<string> spawnedGroupIds,
		array<vector> originalAssetPositions,
		array<vector> originalAssetLastPositions,
		array<vector> originalElementPositions,
		array<vector> originalGroupPositions,
		array<vector> originalGroupSourcePositions)
	{
		if (!state || !mission || !assets || !groups || !elements || !spawnedGroupIds)
			return;
		for (int rollbackIndex = 0; rollbackIndex < groups.Count(); rollbackIndex++)
		{
			HST_ActiveGroupState activeGroup = groups[rollbackIndex];
			if (!activeGroup || spawnedGroupIds.Find(activeGroup.m_sGroupId) < 0)
				continue;
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			RemoveConvoyProgressStatusForGroup(activeGroup.m_sGroupId);
			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_bSpawnAttempted = false;
			activeGroup.m_iSpawnedAgentCount = 0;
			activeGroup.m_iAssignedWaypointCount = 0;
			activeGroup.m_iSurvivorVehicleCount = 1;
			activeGroup.m_sConvoyRuntimeStage = "STRANDED_VIRTUAL";
			if (rollbackIndex < originalGroupPositions.Count())
				activeGroup.m_vPosition = originalGroupPositions[rollbackIndex];
			if (rollbackIndex < originalGroupSourcePositions.Count())
				activeGroup.m_vSourcePosition = originalGroupSourcePositions[rollbackIndex];

			HST_MissionAssetState asset = assets[rollbackIndex];
			if (asset)
			{
				asset.m_bSpawned = false;
				if (rollbackIndex < originalAssetPositions.Count())
					asset.m_vCurrentPosition = originalAssetPositions[rollbackIndex];
				if (rollbackIndex < originalAssetLastPositions.Count())
					asset.m_vLastKnownPosition = originalAssetLastPositions[rollbackIndex];
				HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
				if (runtimeEntity)
				{
					runtimeEntity.m_bSpawned = false;
					runtimeEntity.m_vPosition = asset.m_vCurrentPosition;
				}
			}

			HST_ConvoyElementState element = elements[rollbackIndex];
			if (element)
			{
				element.m_bPhysicalized = false;
				if (rollbackIndex < originalElementPositions.Count())
					element.m_vCurrentPosition = originalElementPositions[rollbackIndex];
				element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
				element.m_iRevision++;
			}
		}
	}

	bool RetireExactMissionConvoyRuntime(HST_CampaignState state, HST_ActiveMissionState mission, out string reason)
	{
		reason = "";
		if (!state || !IsExactMissionConvoyContract(mission))
		{
			reason = "exact mission-convoy state or contract is missing";
			return false;
		}

		ref array<ref HST_ActiveGroupState> groups = {};
		ref array<ref HST_MissionAssetState> assets = {};
		ref array<ref HST_ConvoyElementState> elements = {};
		ref array<bool> preserveVehicles = {};
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			string groupId = BuildMissionConvoyGroupId(mission, index);
			IEntity crewEntity = GetRuntimeCrewGroupEntity(groupId);
			IEntity vehicleEntity = GetRuntimeVehicleEntity(groupId);
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			if (!activeGroup)
			{
				reason = string.Format("exact convoy runtime %1 has no durable group owner", groupId);
				return false;
			}
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			if (!asset || activeGroup.m_sMissionInstanceId != mission.m_sInstanceId || activeGroup.m_sMissionAssetId != asset.m_sAssetId)
			{
				reason = string.Format("exact convoy runtime %1 has conflicting durable ownership", groupId);
				return false;
			}
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!element)
			{
				reason = string.Format("exact convoy runtime %1 has an ambiguous element backlink", groupId);
				return false;
			}
			if (IsForceSpawnRuntimeOwnershipHeldForGroup(groupId))
			{
				reason = string.Format("exact convoy runtime %1 is held by another lifecycle", groupId);
				return false;
			}

			bool preserveVehicle = vehicleEntity && ShouldPreserveInactiveMissionConvoyVehicle(state, activeGroup);
			if (vehicleEntity && !preserveVehicle && IsAnyPlayerInVehicle(vehicleEntity))
			{
				reason = string.Format("a living player occupies terminal convoy vehicle %1", asset.m_sAssetId);
				return false;
			}

			groups.Insert(activeGroup);
			assets.Insert(asset);
			elements.Insert(element);
			preserveVehicles.Insert(preserveVehicle);
		}

		bool requiresNormalization;
		for (int normalizationIndex = 0; normalizationIndex < groups.Count(); normalizationIndex++)
		{
			HST_ActiveGroupState normalizationGroup = groups[normalizationIndex];
			HST_MissionAssetState normalizationAsset = assets[normalizationIndex];
			HST_ConvoyElementState normalizationElement = elements[normalizationIndex];
			HST_MissionRuntimeEntityState normalizationRuntime = state.FindMissionRuntimeEntity(normalizationAsset.m_sEntityId);
			if (GetRuntimeCrewGroupEntity(normalizationGroup.m_sGroupId) || GetRuntimeVehicleEntity(normalizationGroup.m_sGroupId)
				|| normalizationGroup.m_bSpawnedEntity || normalizationGroup.m_bSpawnAttempted
				|| !normalizationGroup.m_sRuntimeEntityId.IsEmpty() || normalizationGroup.m_iSpawnedAgentCount != 0
				|| normalizationGroup.m_iAssignedWaypointCount != 0 || normalizationAsset.m_bSpawned
				|| normalizationElement.m_bPhysicalized || (normalizationRuntime && normalizationRuntime.m_bSpawned)
				|| CountExactMissionConvoyMemberMappings(mission.m_sInstanceId, normalizationGroup.m_sGroupId) != 0)
			{
				requiresNormalization = true;
				break;
			}
		}
		if (!requiresNormalization)
		{
			reason = "exact mission convoy runtime was already retired";
			return false;
		}

		ref array<vector> sampledPositions = {};
		ref array<int> sampledSurvivors = {};
		ref array<float> sampledDamageFractions = {};
		ref array<float> sampledFuelFractions = {};
		ref array<float> sampledAmmoFractions = {};
		ref array<IEntity> sampledVehicles = {};
		for (int sampleIndex = 0; sampleIndex < groups.Count(); sampleIndex++)
		{
			HST_ActiveGroupState sampleGroup = groups[sampleIndex];
			HST_MissionAssetState sampleAsset = assets[sampleIndex];
			HST_ConvoyElementState sampleElement = elements[sampleIndex];
			IEntity sampleCrew = GetRuntimeCrewGroupEntity(sampleGroup.m_sGroupId);
			IEntity sampleVehicle = GetRuntimeVehicleEntity(sampleGroup.m_sGroupId);
			vector samplePosition = sampleGroup.m_vPosition;
			vector terminalCrewPosition;
			if (IsExactMissionConvoyTerminalSurvivingCrew(sampleAsset, sampleElement) && sampleCrew)
				terminalCrewPosition = ResolveActiveGroupLiveRuntimePosition(sampleGroup, true);
			if (!IsZeroVector(terminalCrewPosition))
				samplePosition = terminalCrewPosition;
			else if (sampleVehicle && !IsZeroVector(sampleVehicle.GetOrigin()))
				samplePosition = sampleVehicle.GetOrigin();
			else if (sampleCrew && !IsZeroVector(sampleCrew.GetOrigin()))
				samplePosition = sampleCrew.GetOrigin();

			int survivors = Math.Max(0, Math.Max(sampleGroup.m_iSurvivorInfantryCount, sampleGroup.m_iLastSeenAliveCount));
			if (sampleCrew)
				survivors = CountAliveRuntimeCrewAgents(sampleGroup);
			int originalCrew = Math.Max(sampleGroup.m_iOriginalInfantryCount, sampleGroup.m_iInfantryCount);
			if (sampleElement)
				originalCrew = Math.Max(originalCrew, sampleElement.m_iOriginalCrewCount);
			if (survivors < 0 || survivors > originalCrew)
			{
				reason = string.Format("exact convoy runtime %1 produced an ambiguous living-crew count %2", sampleGroup.m_sGroupId, survivors);
				return false;
			}

			float damageFraction;
			float fuelFraction = 1.0;
			float ammoFraction = 1.0;
			if (sampleElement)
			{
				damageFraction = sampleElement.m_fVehicleDamageFraction;
				fuelFraction = sampleElement.m_fFuelFraction;
				ammoFraction = sampleElement.m_fAmmoFraction;
			}
			if (sampleVehicle)
			{
				TrySampleMissionConvoyVehicleDamageFraction(sampleVehicle, damageFraction);
				TrySampleMissionConvoyVehicleFuelFraction(sampleVehicle, fuelFraction);
				TrySampleMissionConvoyVehicleAmmoFraction(sampleVehicle, ammoFraction);
			}

			sampledPositions.Insert(samplePosition);
			sampledSurvivors.Insert(survivors);
			sampledDamageFractions.Insert(damageFraction);
			sampledFuelFractions.Insert(fuelFraction);
			sampledAmmoFractions.Insert(ammoFraction);
			sampledVehicles.Insert(sampleVehicle);
		}
		for (int salvagePreflightIndex = 0; salvagePreflightIndex < groups.Count(); salvagePreflightIndex++)
		{
			if (!preserveVehicles[salvagePreflightIndex])
				continue;
			if (!IsExactMissionConvoySettledSalvageCaptureAuthority(state, mission, assets[salvagePreflightIndex], groups[salvagePreflightIndex], elements[salvagePreflightIndex]))
			{
				reason = string.Format("exact convoy preserved salvage vehicle %1 has no settled terminal authority", assets[salvagePreflightIndex].m_sAssetId);
				return false;
			}
			if (!CanRegisterExactMissionConvoySettledSalvageVehicleHandle(
				mission,
				assets[salvagePreflightIndex],
				groups[salvagePreflightIndex],
				sampledVehicles[salvagePreflightIndex]))
			{
				reason = string.Format("exact convoy settled salvage vehicle %1 has conflicting process identity", assets[salvagePreflightIndex].m_sAssetId);
				return false;
			}
		}

		HST_ExactMissionConvoyRosterMutationPlan rosterPlan;
		string rosterFailure = BuildExactMissionConvoyRosterMutationPlan(state, mission, elements, sampledSurvivors, rosterPlan, true);
		if (!rosterFailure.IsEmpty())
		{
			reason = rosterFailure;
			return false;
		}
		ApplyExactMissionConvoyRosterMutationPlan(rosterPlan);

		for (int retireIndex = 0; retireIndex < groups.Count(); retireIndex++)
		{
			HST_ActiveGroupState activeGroup = groups[retireIndex];
			HST_MissionAssetState asset = assets[retireIndex];
			HST_ConvoyElementState element = elements[retireIndex];
			bool preserveVehicle = preserveVehicles[retireIndex];
			vector position = sampledPositions[retireIndex];
			int survivors = sampledSurvivors[retireIndex];

			activeGroup.m_vPosition = position;
			activeGroup.m_vSourcePosition = position;
			activeGroup.m_iInfantryCount = survivors;
			activeGroup.m_iSurvivorInfantryCount = survivors;
			activeGroup.m_iLastSeenAliveCount = survivors;
			activeGroup.m_iDurableLivingInfantryCount = survivors;
			activeGroup.m_iSurvivorVehicleCount = 0;
			if (preserveVehicle)
				activeGroup.m_iSurvivorVehicleCount = 1;
			if (!IsZeroVector(position))
			{
				asset.m_vCurrentPosition = position;
				asset.m_vLastKnownPosition = position;
			}
			if (element)
			{
				element.m_fVehicleDamageFraction = sampledDamageFractions[retireIndex];
				element.m_fFuelFraction = sampledFuelFractions[retireIndex];
				element.m_fAmmoFraction = sampledAmmoFractions[retireIndex];
				element.m_vCurrentPosition = position;
				element.m_iSurvivingCrewCount = Math.Min(element.m_iOriginalCrewCount, survivors);
				element.m_bPhysicalized = false;
				element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
				element.m_iRevision++;
			}

			if (preserveVehicle && IsExactMissionConvoySettledSalvageCaptureAuthority(state, mission, asset, activeGroup, element))
				RegisterExactMissionConvoySettledSalvageVehicleHandle(mission, asset, activeGroup, sampledVehicles[retireIndex]);
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId, !preserveVehicle);
			RemoveConvoyProgressStatusForGroup(activeGroup.m_sGroupId);
			RemoveRestoredMissionConvoyRuntimeRebuildAttempt(activeGroup.m_sGroupId);
			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_bSpawnAttempted = false;
			activeGroup.m_sRuntimeEntityId = "";
			activeGroup.m_iSpawnedAgentCount = 0;
			activeGroup.m_iAssignedWaypointCount = 0;
			asset.m_bSpawned = false;
			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity)
				runtimeEntity.m_bSpawned = false;
		}

		m_bMarkerRefreshNeeded = true;
		reason = "exact mission convoy runtime retired";
		return true;
	}

	protected string BuildExactMissionConvoyRosterMutationPlan(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		array<ref HST_ConvoyElementState> elements,
		array<int> sampledSurvivors,
		out HST_ExactMissionConvoyRosterMutationPlan plan,
		bool allowUnmaterializedIdentity = false)
	{
		plan = null;
		if (!state || !mission || !elements || !sampledSurvivors || elements.Count() != sampledSurvivors.Count())
			return "exact convoy sampled survivor roster arrays are missing or misaligned";
		if (IsExactMissionConvoyOutboundProjectionTransactionOpen(mission))
			return "exact convoy survivor mutation is frozen while outbound projection is staged";
		HST_ForceManifestState manifest = state.FindForceManifest(mission.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		if (!manifest || !manifest.m_bFrozen || !batch
			|| manifest.m_sOperationId != mission.m_sOperationId
			|| batch.m_sOperationId != mission.m_sOperationId
			|| batch.m_sManifestId != manifest.m_sManifestId)
			return "exact convoy sampled survivor roster authority is missing or conflicting";
		bool settledRetirementSlotAuthority = allowUnmaterializedIdentity
			&& HasExactMissionConvoySettledRetirementSlotAuthority(state, mission, batch);

		HST_ExactMissionConvoyRosterMutationPlan result = new HST_ExactMissionConvoyRosterMutationPlan();
		result.m_State = state;
		result.m_Mission = mission;
		result.m_Manifest = manifest;
		result.m_Batch = batch;
		for (int elementIndex = 0; elementIndex < elements.Count(); elementIndex++)
		{
			HST_ConvoyElementState element = elements[elementIndex];
			if (!element || element.m_sOperationId != mission.m_sOperationId
				|| element.m_sMissionInstanceId != mission.m_sInstanceId
				|| element.m_sManifestId != manifest.m_sManifestId || result.m_aElements.Contains(element))
				return "exact convoy sampled survivor element authority is missing, duplicate, or conflicting";
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(element.m_sGroupId);
			if (!activeGroup || activeGroup.m_sMissionInstanceId != mission.m_sInstanceId
				|| activeGroup.m_sOperationId != mission.m_sOperationId
				|| activeGroup.m_sManifestId != manifest.m_sManifestId
				|| activeGroup.m_sConvoyElementId != element.m_sElementId)
				return string.Format("exact convoy element %1 has no unique durable group owner", element.m_sElementId);
			int desiredSurvivors = sampledSurvivors[elementIndex];
			if (desiredSurvivors < 0 || desiredSurvivors > element.m_iSurvivingCrewCount)
				return string.Format("exact convoy element %1 sampled an invalid survivor count %2", element.m_sElementId, desiredSurvivors);

			int currentLivingSlots;
			int observedLivingSlots;
			int mappedLivingAuthoritySlots;
			array<IEntity> mappedAuthorityEntities = {};
			SCR_AIGroup exactRoot = SCR_AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId));
			int mappingCount = CountExactMissionConvoyMemberMappings(mission.m_sInstanceId, activeGroup.m_sGroupId);
			if (mappingCount < 0)
				return "exact convoy member identity arrays are corrupt";
			bool hasPhysicalCrewAuthority = exactRoot != null || mappingCount > 0;
			if (hasPhysicalCrewAuthority && !exactRoot)
				return string.Format("exact convoy element %1 lost its canonical crew root while member identities remain", element.m_sElementId);
			for (int seatIndex = 0; seatIndex < element.m_iOriginalCrewCount; seatIndex++)
			{
				HST_ForceManifestMemberState member = ResolveExactMissionConvoyManifestMemberForSeat(manifest, element, seatIndex);
				if (!member || !member.m_bRequired || member.m_sPrefab.IsEmpty()
					|| member.m_sAssignedVehicleSlotId != element.m_sVehicleSlotId)
					return string.Format("exact convoy element %1 frozen member seat %2 is missing or conflicting", element.m_sElementId, seatIndex);
				int resultClaimants;
				foreach (HST_ForceSpawnSlotResultState candidateResult : batch.m_aSlotResults)
				{
					if (candidateResult && candidateResult.m_sSlotId == member.m_sSlotId)
						resultClaimants++;
				}
				HST_ForceSpawnSlotResultState memberResult = batch.FindSlotResult(member.m_sSlotId);
				if (resultClaimants != 1 || !memberResult || memberResult.m_sSlotKind != "member"
					|| memberResult.m_sSpawnedPrefab != member.m_sPrefab || !memberResult.m_bEverAlive)
					return string.Format("exact convoy element %1 held member seat %2 is missing or conflicting", element.m_sElementId, seatIndex);
				bool memberLiving = !memberResult.m_bCasualtyConfirmed;
				if (memberLiving)
					currentLivingSlots++;
				if (memberResult.m_bCasualtyConfirmed
					&& (memberResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED || memberResult.m_bAliveVerified))
					return string.Format("exact convoy element %1 casualty member seat %2 has conflicting lifecycle state", element.m_sElementId, seatIndex);
				if (!memberLiving)
				{
					IEntity retiredMappedEntity;
					if (TryGetExactMissionConvoyMappedMemberEntity(mission.m_sInstanceId, activeGroup.m_sGroupId, member.m_sSlotId, retiredMappedEntity)
						&& retiredMappedEntity && !retiredMappedEntity.IsDeleted() && IsLivingEntity(retiredMappedEntity))
						return string.Format("exact convoy casualty slot %1 is mapped to a living entity", member.m_sSlotId);
					continue;
				}

				if (settledRetirementSlotAuthority)
				{
					if (memberResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED || !memberResult.m_bAliveVerified)
						return string.Format("settled exact convoy living slot %1 has conflicting retirement authority", member.m_sSlotId);
				}
				else if (memberResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED || !memberResult.m_bAliveVerified)
					return string.Format("exact convoy living slot %1 has conflicting lifecycle state", member.m_sSlotId);
				if (!hasPhysicalCrewAuthority)
					continue;

				IEntity mappedEntity;
				if (!TryGetExactMissionConvoyMappedMemberEntity(mission.m_sInstanceId, activeGroup.m_sGroupId, member.m_sSlotId, mappedEntity)
					|| !mappedEntity || mappedEntity.IsDeleted()
					|| ResolveEntityPrefabName(mappedEntity) != member.m_sPrefab
					|| !IsRuntimeEntityRegisteredExactlyOnceForGroup(activeGroup.m_sGroupId, mappedEntity))
					return string.Format("exact convoy living slot %1 has no unique mapped runtime entity", member.m_sSlotId);
				if (mappedAuthorityEntities.Contains(mappedEntity))
					return string.Format("exact convoy living slot %1 reuses another slot's mapped runtime entity", member.m_sSlotId);
				mappedAuthorityEntities.Insert(mappedEntity);
				mappedLivingAuthoritySlots++;
				if (IsLivingEntity(mappedEntity))
				{
					string membershipFailure;
					if (!ValidateForceSpawnGroupMember(activeGroup, exactRoot, mappedEntity, seatIndex, membershipFailure))
						return string.Format("exact convoy living slot %1 lost root membership: %2", member.m_sSlotId, membershipFailure);
					observedLivingSlots++;
				}
				else
				{
					if (settledRetirementSlotAuthority)
						return string.Format("settled exact convoy living slot %1 changed after its terminal roster was frozen", member.m_sSlotId);
					if (!HasExplicitExactMissionConvoyMemberDeathEvidence(mappedEntity))
						return string.Format("exact convoy living slot %1 is missing without explicit death evidence", member.m_sSlotId);
					result.m_aNewCasualtySlots.Insert(memberResult);
				}
			}
			if (currentLivingSlots != element.m_iSurvivingCrewCount)
				return string.Format("exact convoy element %1 durable member slots conflict with its survivor count", element.m_sElementId);
			if (settledRetirementSlotAuthority && desiredSurvivors != currentLivingSlots)
				return string.Format("settled exact convoy element %1 attempted to change its terminal survivor count", element.m_sElementId);
			if (!hasPhysicalCrewAuthority && currentLivingSlots > 0 && !allowUnmaterializedIdentity)
				return string.Format("exact convoy element %1 is missing living mapped entities without death evidence", element.m_sElementId);
			if (!hasPhysicalCrewAuthority)
				observedLivingSlots = currentLivingSlots;
			else if (mappingCount != currentLivingSlots || mappedLivingAuthoritySlots != currentLivingSlots)
				return string.Format("exact convoy element %1 mapping completeness is %2/%3", element.m_sElementId, mappingCount, currentLivingSlots);
			if (desiredSurvivors != observedLivingSlots)
				return string.Format("exact convoy element %1 sampled %2 survivors but identity reconciliation observed %3", element.m_sElementId, desiredSurvivors, observedLivingSlots);

			result.m_aElements.Insert(element);
			result.m_aGroups.Insert(activeGroup);
			result.m_aSurvivorCounts.Insert(observedLivingSlots);
		}
		plan = result;
		return "";
	}

	protected bool HasExactMissionConvoySettledRetirementSlotAuthority(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ForceSpawnResultState batch)
	{
		if (!state || !mission || !batch || mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;
		HST_OperationRecordState operation = ResolveSettledExactMissionConvoyOperationForRetirement(state, mission);
		if (!operation || operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED
			|| mission.m_sSettlementId.IsEmpty() || operation.m_sSettlementId != mission.m_sSettlementId)
			return false;
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRING
			&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
			return false;
		return batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
			&& !batch.m_bStrategicProjectionHeld && batch.m_sTerminalReason == operation.m_sTerminalReason;
	}

	protected void ApplyExactMissionConvoyRosterMutationPlan(HST_ExactMissionConvoyRosterMutationPlan plan)
	{
		if (!plan || !plan.m_State || !plan.m_Mission || !plan.m_Batch
			|| plan.m_aElements.Count() != plan.m_aGroups.Count()
			|| plan.m_aElements.Count() != plan.m_aSurvivorCounts.Count())
			return;

		bool rosterChanged;
		foreach (HST_ForceSpawnSlotResultState casualtySlot : plan.m_aNewCasualtySlots)
		{
			if (!casualtySlot || casualtySlot.m_bCasualtyConfirmed)
				continue;
			casualtySlot.m_bCasualtyConfirmed = true;
			casualtySlot.m_bAliveVerified = false;
			casualtySlot.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED;
			casualtySlot.m_iCasualtyAtSecond = plan.m_State.m_iElapsedSeconds;
			casualtySlot.m_iUpdatedAtSecond = plan.m_State.m_iElapsedSeconds;
			casualtySlot.m_sRetirementReason = "exact mission convoy explicitly mapped physical casualty";
			casualtySlot.m_iLifecycleRevision++;
			RemoveExactMissionConvoyMemberMapping(plan.m_Mission.m_sInstanceId, casualtySlot.m_sSlotId);
			rosterChanged = true;
		}

		for (int elementIndex = 0; elementIndex < plan.m_aElements.Count(); elementIndex++)
		{
			HST_ConvoyElementState element = plan.m_aElements[elementIndex];
			HST_ActiveGroupState activeGroup = plan.m_aGroups[elementIndex];
			int survivors = plan.m_aSurvivorCounts[elementIndex];
			if (element.m_iSurvivingCrewCount != survivors)
			{
				element.m_iSurvivingCrewCount = survivors;
				element.m_iLastUpdatedSecond = plan.m_State.m_iElapsedSeconds;
				element.m_iRevision++;
			}
			if (activeGroup.m_iInfantryCount != survivors
				|| activeGroup.m_iSpawnedAgentCount != survivors
				|| activeGroup.m_iLastSeenAliveCount != survivors
				|| activeGroup.m_iSurvivorInfantryCount != survivors
				|| activeGroup.m_iDurableLivingInfantryCount != survivors)
			{
				activeGroup.m_iInfantryCount = survivors;
				activeGroup.m_iSpawnedAgentCount = survivors;
				activeGroup.m_iLastSeenAliveCount = survivors;
				activeGroup.m_iSurvivorInfantryCount = survivors;
				activeGroup.m_iDurableLivingInfantryCount = survivors;
				activeGroup.m_iLifecycleRevision++;
			}
		}
		if (rosterChanged)
		{
			plan.m_Batch.m_iUpdatedAtSecond = plan.m_State.m_iElapsedSeconds;
			plan.m_Batch.m_iLifecycleRevision++;
		}
	}

	protected bool TrySampleExactMissionConvoyMappedSurvivors(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ActiveGroupState activeGroup,
		HST_ConvoyElementState element,
		out int survivors,
		out string reason)
	{
		survivors = 0;
		reason = "";
		if (!state || !mission || !activeGroup || !element)
		{
			reason = "exact mapped survivor sample root is missing";
			return false;
		}
		HST_ForceManifestState manifest = state.FindForceManifest(mission.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		if (!manifest || !batch)
		{
			reason = "exact mapped survivor authority is missing";
			return false;
		}
		if (element.m_iSurvivingCrewCount <= 0)
		{
			int terminalMappings = CountExactMissionConvoyMemberMappings(mission.m_sInstanceId, activeGroup.m_sGroupId);
			if (terminalMappings != 0)
			{
				reason = string.Format("exact mapped survivor root %1 has zero durable survivors but %2 member mappings", activeGroup.m_sGroupId, terminalMappings);
				return false;
			}
			return true;
		}

		SCR_AIGroup exactRoot = SCR_AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId));
		if (!exactRoot)
		{
			reason = "exact mapped survivor sample has no canonical crew root";
			return false;
		}
		for (int seatIndex = 0; seatIndex < element.m_iOriginalCrewCount; seatIndex++)
		{
			HST_ForceManifestMemberState member = ResolveExactMissionConvoyManifestMemberForSeat(manifest, element, seatIndex);
			HST_ForceSpawnSlotResultState slotResult;
			if (member)
				slotResult = batch.FindSlotResult(member.m_sSlotId);
			if (!member || !slotResult)
			{
				reason = string.Format("exact mapped survivor seat %1 has no durable slot", seatIndex);
				return false;
			}
			if (slotResult.m_bCasualtyConfirmed)
				continue;

			IEntity mappedEntity;
			if (!TryGetExactMissionConvoyMappedMemberEntity(mission.m_sInstanceId, activeGroup.m_sGroupId, member.m_sSlotId, mappedEntity)
				|| !mappedEntity || mappedEntity.IsDeleted())
			{
				reason = string.Format("exact mapped survivor slot %1 is missing without death evidence", member.m_sSlotId);
				return false;
			}
			if (IsLivingEntity(mappedEntity))
			{
				string membershipFailure;
				if (!ValidateForceSpawnGroupMember(activeGroup, exactRoot, mappedEntity, seatIndex, membershipFailure))
				{
					reason = string.Format("exact mapped survivor slot %1 lost root membership: %2", member.m_sSlotId, membershipFailure);
					return false;
				}
				survivors++;
				continue;
			}
			if (!HasExplicitExactMissionConvoyMemberDeathEvidence(mappedEntity))
			{
				reason = string.Format("exact mapped survivor slot %1 is nonliving without explicit death evidence", member.m_sSlotId);
				return false;
			}
		}
		return true;
	}

	bool PrepareExactMissionConvoyAuthorityForPersistence(HST_CampaignState state, out string reason)
	{
		reason = "";
		if (!state || !HasConsistentExactMissionConvoyMemberIdentityArrays())
		{
			reason = "exact convoy persistence boundary has missing state or corrupt member identity arrays";
			return false;
		}
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMissionConvoyContract(mission))
				continue;
			int mappingCount = CountExactMissionConvoyMemberMappings(mission.m_sInstanceId);
			if (mappingCount < 0)
			{
				reason = string.Format("exact convoy %1 persistence member mappings are corrupt", mission.m_sInstanceId);
				return false;
			}
			if (IsExactMissionConvoyOutboundProjectionTransactionOpen(mission))
			{
				reason = string.Format("exact convoy %1 outbound publication transaction is still open", mission.m_sInstanceId);
				return false;
			}
			HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
			bool physicalAuthority = operation
				&& operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;
			if (!physicalAuthority)
			{
				if (mappingCount > 0)
				{
					reason = string.Format("exact convoy %1 retains mapped members without open PHYSICAL authority", mission.m_sInstanceId);
					return false;
				}
				continue;
			}

			bool changed;
			string reconcileFailure;
			if (!TryReconcileExactMissionConvoyMappedSurvivors(state, mission, changed, reconcileFailure))
			{
				reason = string.Format("exact convoy %1 persistence roster reconciliation failed: %2", mission.m_sInstanceId, reconcileFailure);
				return false;
			}
		}
		return true;
	}

	protected bool ReconcileExactMissionConvoyMappedSurvivors(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		bool changed;
		string reason;
		if (!TryReconcileExactMissionConvoyMappedSurvivors(state, mission, changed, reason))
		{
			if (mission && !reason.IsEmpty())
				DebugLogThrottled("exact_member_reconcile_" + mission.m_sInstanceId, reason, 10000);
			return false;
		}
		return changed;
	}

	protected bool TryReconcileExactMissionConvoyMappedSurvivors(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		out bool changed,
		out string reason)
	{
		changed = false;
		reason = "";
		if (!state || !IsExactMissionConvoyContract(mission)
			|| IsExactMissionConvoyOutboundProjectionTransactionOpen(mission))
		{
			reason = "exact mapped survivor reconciliation state is missing or outbound publication remains staged";
			return false;
		}
		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		if (!operation || operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
		{
			reason = "exact mapped survivor reconciliation requires open PHYSICAL authority";
			return false;
		}

		ref array<ref HST_ConvoyElementState> elements = {};
		ref array<int> sampledSurvivors = {};
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !element)
			{
				reason = string.Format("exact mapped survivor root %1 is missing or conflicting", index);
				return false;
			}
			EnsureActiveGroupRuntimeFaction(activeGroup, "exact mapped survivor update");
			EnforceOpposingOccupiedVehicleAccess(state, activeGroup);
			RefreshActiveGroupLivePositionFromRuntime(state, activeGroup, true);
			int survivors;
			string sampleFailure;
			if (!TrySampleExactMissionConvoyMappedSurvivors(state, mission, activeGroup, element, survivors, sampleFailure))
			{
				reason = sampleFailure;
				return false;
			}
			elements.Insert(element);
			sampledSurvivors.Insert(survivors);
		}

		HST_ExactMissionConvoyRosterMutationPlan plan;
		string planFailure = BuildExactMissionConvoyRosterMutationPlan(state, mission, elements, sampledSurvivors, plan);
		if (!planFailure.IsEmpty())
		{
			reason = planFailure;
			return false;
		}
		changed = plan.m_aNewCasualtySlots.Count() > 0;
		for (int planIndex = 0; planIndex < plan.m_aElements.Count(); planIndex++)
		{
			if (plan.m_aElements[planIndex].m_iSurvivingCrewCount != plan.m_aSurvivorCounts[planIndex])
				changed = true;
		}
		ApplyExactMissionConvoyRosterMutationPlan(plan);
		return true;
	}

	protected bool IsValidatedExactMissionConvoyPendingArrivalRestore(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMissionConvoyContract(mission)
			|| mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
			|| mission.m_sRuntimePhase != MISSION_CONVOY_FAILED
			|| mission.m_sLastRuntimeEventKey != CONVOY_FAIL_EVENT_KEY
			|| !mission.m_sRuntimeFailureReason.Contains("Convoy reached its destination:")
			|| mission.m_bConvoyArrivalOutcomeApplied)
			return false;
		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		if (!operation || operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_fRouteTotalDistanceMeters <= 0.0 || IsZeroVector(operation.m_vRouteEndPosition)
			|| operation.m_fRouteProgressMeters < operation.m_fRouteTotalDistanceMeters - HST_MissionConvoyOperationService.EXACT_ARRIVAL_RADIUS_METERS)
			return false;

		int arrivedElements;
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !element
				|| element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE)
				return false;
			if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED)
				arrivedElements++;
		}
		return arrivedElements > 0;
	}

	bool NormalizeExactMissionConvoyRuntimeForRestore(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		bool pendingArrivalRestore = IsValidatedExactMissionConvoyPendingArrivalRestore(state, mission);
		if (!state || (!IsExactActiveMissionConvoy(mission) && !IsExactMissionConvoyRecoveryHold(mission) && !pendingArrivalRestore)
			|| !HasExactMissionConvoyVehicleAssetSet(state, mission))
			return false;

		ref array<ref HST_MissionAssetState> assets = {};
		ref array<ref HST_ActiveGroupState> groups = {};
		ref array<ref HST_ConvoyElementState> elements = {};
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !element || IsForceSpawnRuntimeOwnershipHeldForGroup(activeGroup.m_sGroupId))
				return false;
			IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
			if (vehicleEntity && IsAnyPlayerInVehicle(vehicleEntity))
				return false;
			assets.Insert(asset);
			groups.Insert(activeGroup);
			elements.Insert(element);
		}

		HST_ExactMissionConvoyOutboundProjectionTransaction restoreTransaction = FindExactMissionConvoyOutboundProjectionTransaction(mission.m_sInstanceId);
		if (restoreTransaction)
			RemoveExactMissionConvoyOutboundProjectionTransaction(restoreTransaction);
		for (int cleanupIndex = 0; cleanupIndex < groups.Count(); cleanupIndex++)
		{
			HST_ActiveGroupState cleanupGroup = groups[cleanupIndex];
			ClearPendingActiveGroupPopulation(cleanupGroup);
			DeleteRuntimeGroupEntity(cleanupGroup.m_sGroupId);
			RemoveConvoyProgressStatusForGroup(cleanupGroup.m_sGroupId);
			RemoveRestoredMissionConvoyRuntimeRebuildAttempt(cleanupGroup.m_sGroupId);
		}
		if (FindExactMissionConvoyOutboundProjectionTransaction(mission.m_sInstanceId)
			|| CountExactMissionConvoyMemberMappings(mission.m_sInstanceId) != 0)
			return false;
		foreach (HST_ActiveGroupState verifyGroup : groups)
		{
			if (GetRuntimeCrewGroupEntity(verifyGroup.m_sGroupId) || GetRuntimeVehicleEntity(verifyGroup.m_sGroupId))
				return false;
		}

		for (int normalizeIndex = 0; normalizeIndex < groups.Count(); normalizeIndex++)
		{
			HST_MissionAssetState asset = assets[normalizeIndex];
			HST_ActiveGroupState activeGroup = groups[normalizeIndex];
			HST_ConvoyElementState element = elements[normalizeIndex];

			activeGroup.m_iOriginalInfantryCount = Math.Max(activeGroup.m_iOriginalInfantryCount, element.m_iOriginalCrewCount);
			activeGroup.m_iInfantryCount = Math.Max(0, Math.Min(element.m_iOriginalCrewCount, element.m_iSurvivingCrewCount));
			activeGroup.m_iSurvivorInfantryCount = activeGroup.m_iInfantryCount;
			activeGroup.m_iLastSeenAliveCount = activeGroup.m_iInfantryCount;
			activeGroup.m_iDurableLivingInfantryCount = activeGroup.m_iInfantryCount;
			activeGroup.m_iSpawnedAgentCount = 0;
			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_bSpawnAttempted = false;
			activeGroup.m_sRuntimeEntityId = "";
			activeGroup.m_iAssignedWaypointCount = 0;
			asset.m_bSpawned = false;
			element.m_bPhysicalized = false;
			if (activeGroup.m_iInfantryCount <= 0)
				activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;

			if (IsExactMissionConvoyTerminalSurvivingCrew(asset, element))
			{
				activeGroup.m_iSurvivorVehicleCount = 0;
				activeGroup.m_sRuntimeStatus = ResolveMissionConvoyRuntimeStatus(mission);
				activeGroup.m_sConvoyRuntimeStage = "DISMOUNTED_VIRTUAL";
				HST_MissionRuntimeEntityState terminalRuntimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
				if (terminalRuntimeEntity)
					terminalRuntimeEntity.m_bSpawned = false;
				continue;
			}
			if (IsExactMissionConvoyElementTerminal(element) || IsMissionConvoyVehicleAssetResolved(asset))
				continue;
			if (activeGroup.m_iInfantryCount > 0)
				activeGroup.m_sRuntimeStatus = ResolveMissionConvoyRuntimeStatus(mission);

			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity)
				runtimeEntity.m_bSpawned = false;
		}

		foreach (HST_MissionAssetState cargo : state.m_aMissionAssets)
		{
			if (!cargo || cargo.m_sMissionInstanceId != mission.m_sInstanceId
				|| cargo.m_sAssignedVehicleSlotId.IsEmpty())
				continue;
			if (cargo.m_sRole != MISSION_CONVOY_PAYLOAD_ROLE && cargo.m_sRole != MISSION_CONVOY_CAPTIVE_ROLE)
				continue;
			if (cargo.m_bPickedUp || cargo.m_bDelivered || cargo.m_bDestroyed || cargo.m_bOutcomeApplied)
				continue;

			cargo.m_bSpawned = false;
			cargo.m_bAttachedToCarrier = false;
			cargo.m_sCarriedByVehicleId = "";
			HST_MissionRuntimeEntityState cargoRuntimeEntity;
			if (!cargo.m_sEntityId.IsEmpty())
				cargoRuntimeEntity = state.FindMissionRuntimeEntity(cargo.m_sEntityId);
			if (cargoRuntimeEntity)
				cargoRuntimeEntity.m_bSpawned = false;
		}

		return true;
	}

	bool FoldExactMissionConvoyRuntime(HST_CampaignState state, HST_ActiveMissionState mission, out string reason)
	{
		reason = "";
		if (!state || (!IsExactActiveMissionConvoy(mission) && !IsExactMissionConvoyRecoveryHold(mission)))
		{
			reason = "exact mission-convoy state or active mission is missing";
			return false;
		}

		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		if (!operation)
		{
			reason = "exact mission-convoy operation is missing, closed, or terminal";
			return false;
		}
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
		{
			reason = "exact mission-convoy operation is not physical or dematerializing";
			return false;
		}
		if (mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT
			|| operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT
			|| operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED)
		{
			reason = "exact mission convoy cannot fold while in contact";
			return false;
		}
		if (!HasExactMissionConvoyVehicleAssetSet(state, mission))
		{
			reason = "exact mission convoy does not have its deterministic three-vehicle asset set";
			return false;
		}

		foreach (HST_MissionAssetState missionAsset : state.m_aMissionAssets)
		{
			if (!missionAsset || missionAsset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (missionAsset.m_sRole != MISSION_CONVOY_PAYLOAD_ROLE
				&& missionAsset.m_sRole != MISSION_CONVOY_CAPTIVE_ROLE)
				continue;
			if (HasUnsafeExactMissionConvoyCargoCarrierClaim(state, mission, missionAsset))
			{
				reason = string.Format("convoy cargo or captive asset %1 still claims an unsafe convoy-carrier attachment", missionAsset.m_sAssetId);
				return false;
			}
		}

		ref array<ref HST_MissionAssetState> assets = {};
		ref array<ref HST_ConvoyElementState> elements = {};
		ref array<ref HST_ActiveGroupState> groups = {};
		ref array<ref HST_MissionAssetState> terminalAssets = {};
		ref array<ref HST_ConvoyElementState> terminalElements = {};
		ref array<ref HST_ActiveGroupState> terminalGroups = {};
		ref array<bool> preserveTerminalVehicles = {};
		ref array<bool> terminalCrewRuntimeObserved = {};
		ref array<ref HST_MissionAssetState> crewlessAssets = {};
		ref array<ref HST_ConvoyElementState> crewlessElements = {};
		ref array<ref HST_ActiveGroupState> crewlessGroups = {};
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !element)
			{
				reason = string.Format("exact convoy element mapping %1 is missing or ambiguous", index);
				return false;
			}
			if (activeGroup.m_sOperationId != operation.m_sOperationId || activeGroup.m_sMissionAssetId != asset.m_sAssetId || activeGroup.m_sConvoyElementId != element.m_sElementId)
			{
				reason = string.Format("exact convoy group %1 has conflicting durable ownership", activeGroup.m_sGroupId);
				return false;
			}

			bool assetTerminal = IsMissionConvoyVehicleAssetResolved(asset);
			bool elementTerminal = IsExactMissionConvoyElementTerminal(element);
			bool crewlessRecoveryElement = IsExactMissionConvoyRecoveryHold(mission)
				&& element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED
				&& element.m_iSurvivingCrewCount <= 0 && !assetTerminal;
			if (assetTerminal != elementTerminal)
			{
				reason = string.Format("exact convoy element %1 terminal disposition conflicts with asset %2", element.m_sElementId, asset.m_sAssetId);
				return false;
			}
			if (assetTerminal)
			{
				int terminalLivingCrew = Math.Max(element.m_iSurvivingCrewCount,
					Math.Max(activeGroup.m_iSurvivorInfantryCount, activeGroup.m_iLastSeenAliveCount));
				IEntity terminalCrewRuntime = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
				if (terminalCrewRuntime)
					terminalLivingCrew = CountAliveRuntimeCrewAgents(activeGroup);
				if (terminalLivingCrew > 0 && !IsExactMissionConvoyTerminalSurvivingCrew(asset, element))
				{
					reason = string.Format("exact convoy element %1 has %2 living crew under a terminal disposition that is not crew-only eligible", element.m_sElementId, terminalLivingCrew);
					return false;
				}
				if (IsExactMissionConvoyTerminalSurvivingCrew(asset, element) && !terminalCrewRuntime)
				{
					reason = string.Format("exact convoy terminal element %1 is missing its durable surviving-crew runtime", element.m_sElementId);
					return false;
				}
			}
			if (assetTerminal || crewlessRecoveryElement)
			{
				IEntity terminalCrewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
				IEntity terminalVehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
				if (!terminalCrewEntity && !terminalVehicleEntity)
					continue;
				if (IsForceSpawnRuntimeOwnershipHeldForGroup(activeGroup.m_sGroupId))
				{
					reason = string.Format("terminal exact convoy element %1 runtime ownership is held by another lifecycle", element.m_sElementId);
					return false;
				}
				if (terminalVehicleEntity && IsAnyPlayerInVehicle(terminalVehicleEntity))
				{
					reason = string.Format("a living player occupies terminal convoy vehicle %1", asset.m_sAssetId);
					return false;
				}
				if (IsConvoyCrewPopulationPending(state, activeGroup) || IsConvoyCrewControlPending(state, activeGroup))
				{
					reason = string.Format("terminal convoy element %1 is inside population or seating grace", element.m_sElementId);
					return false;
				}
				terminalAssets.Insert(asset);
				terminalElements.Insert(element);
				terminalGroups.Insert(activeGroup);
				terminalCrewRuntimeObserved.Insert(terminalCrewEntity != null);
				bool preserveTerminalVehicle = terminalVehicleEntity
					&& element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED
					&& IsLivingEntity(terminalVehicleEntity);
				preserveTerminalVehicles.Insert(preserveTerminalVehicle);
				continue;
			}

			IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
			IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
			if (element.m_iSurvivingCrewCount <= 0)
			{
				if (crewEntity && CountAliveRuntimeCrewAgents(activeGroup) > 0)
				{
					reason = string.Format("exact convoy element %1 durable zero crew conflicts with living physical crew", element.m_sElementId);
					return false;
				}
				if (vehicleEntity && IsAnyPlayerInVehicle(vehicleEntity))
				{
					reason = string.Format("a living player occupies crewless convoy vehicle %1", asset.m_sAssetId);
					return false;
				}
				if (IsConvoyCrewPopulationPending(state, activeGroup) || IsConvoyCrewControlPending(state, activeGroup))
				{
					reason = string.Format("crewless convoy element %1 is inside population or seating grace", element.m_sElementId);
					return false;
				}
				if (IsForceSpawnRuntimeOwnershipHeldForGroup(activeGroup.m_sGroupId))
				{
					reason = string.Format("crewless convoy element %1 runtime ownership is held by another lifecycle", element.m_sElementId);
					return false;
				}

				crewlessAssets.Insert(asset);
				crewlessElements.Insert(element);
				crewlessGroups.Insert(activeGroup);
				continue;
			}
			if (!crewEntity || !vehicleEntity)
			{
				reason = string.Format("exact convoy element %1 has incomplete physical runtime handles", element.m_sElementId);
				return false;
			}
			if (IsForceSpawnRuntimeOwnershipHeldForGroup(activeGroup.m_sGroupId))
			{
				reason = string.Format("exact convoy element %1 runtime ownership is held by another lifecycle", element.m_sElementId);
				return false;
			}
			if (IsAnyPlayerInVehicle(vehicleEntity))
			{
				reason = string.Format("a living player occupies convoy vehicle %1", asset.m_sAssetId);
				return false;
			}
			if (IsConvoyCrewPopulationPending(state, activeGroup) || IsConvoyCrewControlPending(state, activeGroup))
			{
				reason = string.Format("convoy element %1 is inside population or seating grace", element.m_sElementId);
				return false;
			}

			assets.Insert(asset);
			elements.Insert(element);
			groups.Insert(activeGroup);
		}

		ref array<vector> sampledCrewlessPositions = {};
		ref array<int> sampledCrewlessSurvivors = {};
		ref array<float> sampledCrewlessDamageFractions = {};
		ref array<float> sampledCrewlessFuelFractions = {};
		ref array<float> sampledCrewlessAmmoFractions = {};
		for (int crewlessSampleIndex = 0; crewlessSampleIndex < crewlessGroups.Count(); crewlessSampleIndex++)
		{
			HST_ActiveGroupState crewlessGroup = crewlessGroups[crewlessSampleIndex];
			HST_ConvoyElementState crewlessElement = crewlessElements[crewlessSampleIndex];
			IEntity crewlessCrew = GetRuntimeCrewGroupEntity(crewlessGroup.m_sGroupId);
			IEntity crewlessVehicle = GetRuntimeVehicleEntity(crewlessGroup.m_sGroupId);
			if (crewlessCrew && CountAliveRuntimeCrewAgents(crewlessGroup) > 0)
			{
				reason = string.Format("crewless convoy group %1 gained living crew during fold preflight", crewlessGroup.m_sGroupId);
				return false;
			}
			vector crewlessPosition = crewlessElement.m_vCurrentPosition;
			if (crewlessVehicle && !IsZeroVector(crewlessVehicle.GetOrigin()))
				crewlessPosition = crewlessVehicle.GetOrigin();
			if (IsZeroVector(crewlessPosition))
			{
				reason = string.Format("crewless convoy group %1 produced an invalid live position", crewlessGroup.m_sGroupId);
				return false;
			}
			float crewlessDamageFraction = crewlessElement.m_fVehicleDamageFraction;
			float crewlessFuelFraction = crewlessElement.m_fFuelFraction;
			float crewlessAmmoFraction = crewlessElement.m_fAmmoFraction;
			if (crewlessVehicle)
			{
				TrySampleMissionConvoyVehicleDamageFraction(crewlessVehicle, crewlessDamageFraction);
				TrySampleMissionConvoyVehicleFuelFraction(crewlessVehicle, crewlessFuelFraction);
				TrySampleMissionConvoyVehicleAmmoFraction(crewlessVehicle, crewlessAmmoFraction);
			}
			sampledCrewlessPositions.Insert(crewlessPosition);
			sampledCrewlessSurvivors.Insert(0);
			sampledCrewlessDamageFractions.Insert(crewlessDamageFraction);
			sampledCrewlessFuelFractions.Insert(crewlessFuelFraction);
			sampledCrewlessAmmoFractions.Insert(crewlessAmmoFraction);
		}

		ref array<vector> sampledTerminalPositions = {};
		ref array<int> sampledTerminalSurvivors = {};
		ref array<float> sampledTerminalDamageFractions = {};
		ref array<float> sampledTerminalFuelFractions = {};
		ref array<float> sampledTerminalAmmoFractions = {};
		for (int terminalSampleIndex = 0; terminalSampleIndex < terminalGroups.Count(); terminalSampleIndex++)
		{
			HST_ActiveGroupState terminalGroup = terminalGroups[terminalSampleIndex];
			HST_ConvoyElementState terminalElement = terminalElements[terminalSampleIndex];
			IEntity terminalCrew = GetRuntimeCrewGroupEntity(terminalGroup.m_sGroupId);
			IEntity terminalVehicle = GetRuntimeVehicleEntity(terminalGroup.m_sGroupId);
			int terminalSurvivors = terminalElement.m_iSurvivingCrewCount;
			if (terminalCrew)
				terminalSurvivors = CountAliveRuntimeCrewAgents(terminalGroup);
			else if (terminalSampleIndex < terminalCrewRuntimeObserved.Count() && terminalCrewRuntimeObserved[terminalSampleIndex])
			{
				bool authoritativeElimination = terminalGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED
					|| terminalGroup.m_sRuntimeStatus == "eliminated"
					|| (terminalGroup.m_iLastSeenAliveCount <= 0
						&& terminalGroup.m_iSurvivorInfantryCount <= 0
						&& terminalGroup.m_iDurableLivingInfantryCount <= 0);
				if (!authoritativeElimination)
				{
					reason = string.Format("terminal convoy group %1 lost its preflighted crew runtime without authoritative elimination", terminalGroup.m_sGroupId);
					return false;
				}
				terminalSurvivors = 0;
			}
			if (terminalSurvivors < 0 || terminalSurvivors > Math.Max(terminalGroup.m_iOriginalInfantryCount, terminalElement.m_iOriginalCrewCount))
			{
				reason = string.Format("terminal convoy group %1 produced an ambiguous living-crew count %2", terminalGroup.m_sGroupId, terminalSurvivors);
				return false;
			}

			vector terminalPosition = terminalElement.m_vCurrentPosition;
			vector terminalCrewPosition;
			if (terminalCrew && terminalSurvivors > 0)
				terminalCrewPosition = ResolveActiveGroupLiveRuntimePosition(terminalGroup, true);
			if (!IsZeroVector(terminalCrewPosition))
				terminalPosition = terminalCrewPosition;
			else if (terminalVehicle && !IsZeroVector(terminalVehicle.GetOrigin()))
				terminalPosition = terminalVehicle.GetOrigin();
			sampledTerminalPositions.Insert(terminalPosition);
			sampledTerminalSurvivors.Insert(terminalSurvivors);

			float terminalDamageFraction = terminalElement.m_fVehicleDamageFraction;
			float terminalFuelFraction = terminalElement.m_fFuelFraction;
			float terminalAmmoFraction = terminalElement.m_fAmmoFraction;
			if (terminalVehicle)
			{
				TrySampleMissionConvoyVehicleDamageFraction(terminalVehicle, terminalDamageFraction);
				TrySampleMissionConvoyVehicleFuelFraction(terminalVehicle, terminalFuelFraction);
				TrySampleMissionConvoyVehicleAmmoFraction(terminalVehicle, terminalAmmoFraction);
			}
			sampledTerminalDamageFractions.Insert(terminalDamageFraction);
			sampledTerminalFuelFractions.Insert(terminalFuelFraction);
			sampledTerminalAmmoFractions.Insert(terminalAmmoFraction);
		}

		ref array<vector> sampledPositions = {};
		ref array<int> sampledSurvivors = {};
		ref array<float> sampledDamageFractions = {};
		ref array<float> sampledFuelFractions = {};
		ref array<float> sampledAmmoFractions = {};
		for (int sampleIndex = 0; sampleIndex < groups.Count(); sampleIndex++)
		{
			HST_ActiveGroupState sampleGroup = groups[sampleIndex];
			IEntity sampleVehicle = GetRuntimeVehicleEntity(sampleGroup.m_sGroupId);
			if (!sampleVehicle)
			{
				reason = string.Format("convoy group %1 lost its vehicle during fold reconciliation", sampleGroup.m_sGroupId);
				return false;
			}

			vector samplePosition = sampleVehicle.GetOrigin();
			if (IsZeroVector(samplePosition))
			{
				reason = string.Format("convoy group %1 produced an invalid live position", sampleGroup.m_sGroupId);
				return false;
			}

			int livingCrew = CountAliveRuntimeCrewAgents(sampleGroup);
			if (livingCrew < 0 || livingCrew > Math.Max(sampleGroup.m_iOriginalInfantryCount, elements[sampleIndex].m_iOriginalCrewCount))
			{
				reason = string.Format("convoy group %1 produced an ambiguous living-crew count %2", sampleGroup.m_sGroupId, livingCrew);
				return false;
			}

			sampledPositions.Insert(samplePosition);
			sampledSurvivors.Insert(livingCrew);
			float damageFraction = elements[sampleIndex].m_fVehicleDamageFraction;
			TrySampleMissionConvoyVehicleDamageFraction(sampleVehicle, damageFraction);
			sampledDamageFractions.Insert(damageFraction);
			float fuelFraction = elements[sampleIndex].m_fFuelFraction;
			TrySampleMissionConvoyVehicleFuelFraction(sampleVehicle, fuelFraction);
			sampledFuelFractions.Insert(fuelFraction);
			float ammoFraction = elements[sampleIndex].m_fAmmoFraction;
			TrySampleMissionConvoyVehicleAmmoFraction(sampleVehicle, ammoFraction);
			sampledAmmoFractions.Insert(ammoFraction);
		}

		ref array<ref HST_ConvoyElementState> allRosterElements = {};
		ref array<int> allRosterSurvivors = {};
		foreach (HST_ConvoyElementState activeRosterElement : elements)
			allRosterElements.Insert(activeRosterElement);
		foreach (int activeRosterSurvivors : sampledSurvivors)
			allRosterSurvivors.Insert(activeRosterSurvivors);
		foreach (HST_ConvoyElementState terminalRosterElement : terminalElements)
			allRosterElements.Insert(terminalRosterElement);
		foreach (int terminalRosterSurvivors : sampledTerminalSurvivors)
			allRosterSurvivors.Insert(terminalRosterSurvivors);
		foreach (HST_ConvoyElementState crewlessRosterElement : crewlessElements)
			allRosterElements.Insert(crewlessRosterElement);
		foreach (int crewlessRosterSurvivors : sampledCrewlessSurvivors)
			allRosterSurvivors.Insert(crewlessRosterSurvivors);
		HST_ExactMissionConvoyRosterMutationPlan rosterPlan;
		string rosterFailure = BuildExactMissionConvoyRosterMutationPlan(state, mission, allRosterElements, allRosterSurvivors, rosterPlan);
		if (!rosterFailure.IsEmpty())
		{
			reason = rosterFailure;
			return false;
		}
		ApplyExactMissionConvoyRosterMutationPlan(rosterPlan);

		for (int crewlessDurableIndex = 0; crewlessDurableIndex < crewlessGroups.Count(); crewlessDurableIndex++)
		{
			HST_ActiveGroupState crewlessGroup = crewlessGroups[crewlessDurableIndex];
			HST_MissionAssetState crewlessAsset = crewlessAssets[crewlessDurableIndex];
			HST_ConvoyElementState crewlessElement = crewlessElements[crewlessDurableIndex];
			vector crewlessPosition = sampledCrewlessPositions[crewlessDurableIndex];
			crewlessGroup.m_vPosition = crewlessPosition;
			crewlessGroup.m_vSourcePosition = crewlessPosition;
			crewlessGroup.m_iInfantryCount = 0;
			crewlessGroup.m_iLastSeenAliveCount = 0;
			crewlessGroup.m_iSurvivorInfantryCount = 0;
			crewlessGroup.m_iDurableLivingInfantryCount = 0;
			crewlessGroup.m_iSurvivorVehicleCount = 1;
			crewlessGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
			crewlessGroup.m_sConvoyRuntimeStage = "STRANDED_VIRTUAL";
			crewlessAsset.m_vCurrentPosition = crewlessPosition;
			crewlessAsset.m_vLastKnownPosition = crewlessPosition;
			crewlessElement.m_vCurrentPosition = crewlessPosition;
			crewlessElement.m_iSurvivingCrewCount = 0;
			crewlessElement.m_fVehicleDamageFraction = sampledCrewlessDamageFractions[crewlessDurableIndex];
			crewlessElement.m_fFuelFraction = sampledCrewlessFuelFractions[crewlessDurableIndex];
			crewlessElement.m_fAmmoFraction = sampledCrewlessAmmoFractions[crewlessDurableIndex];
			crewlessElement.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED;
			crewlessElement.m_bPhysicalized = false;
			crewlessElement.m_bMobile = false;
			crewlessElement.m_sTerminalReason = "crew eliminated; vehicle remains recoverable";
			crewlessElement.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			crewlessElement.m_iRevision++;
			if (crewlessElement.m_fVehicleDamageFraction >= 0.999)
			{
				MarkMissionConvoyVehicleDestroyed(state, mission, crewlessAsset, crewlessGroup.m_sGroupId, crewlessPosition, "vehicle destruction confirmed by crewless exact fold sample");
				crewlessGroup.m_iSurvivorVehicleCount = 0;
				crewlessElement.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED;
				crewlessElement.m_fVehicleDamageFraction = 1.0;
				crewlessElement.m_sTerminalReason = "vehicle destruction confirmed by crewless exact fold sample";
			}
			SyncExactMissionConvoyRecoveryCarrierAssets(state, mission, crewlessElement, crewlessPosition);

			DeleteRuntimeGroupEntity(crewlessGroup.m_sGroupId);
			RemoveConvoyProgressStatusForGroup(crewlessGroup.m_sGroupId);
			RemoveRestoredMissionConvoyRuntimeRebuildAttempt(crewlessGroup.m_sGroupId);
			crewlessGroup.m_bSpawnedEntity = false;
			crewlessGroup.m_bSpawnAttempted = false;
			crewlessGroup.m_sRuntimeEntityId = "";
			crewlessGroup.m_iSpawnedAgentCount = 0;
			crewlessGroup.m_iAssignedWaypointCount = 0;
			crewlessAsset.m_bSpawned = false;
			HST_MissionRuntimeEntityState crewlessRuntimeEntity = state.FindMissionRuntimeEntity(crewlessAsset.m_sEntityId);
			if (crewlessRuntimeEntity)
			{
				crewlessRuntimeEntity.m_bSpawned = false;
				crewlessRuntimeEntity.m_vPosition = crewlessPosition;
			}
		}

		for (int durableIndex = 0; durableIndex < groups.Count(); durableIndex++)
		{
			HST_ActiveGroupState activeGroup = groups[durableIndex];
			HST_MissionAssetState asset = assets[durableIndex];
			HST_ConvoyElementState element = elements[durableIndex];
			vector position = sampledPositions[durableIndex];
			int survivors = sampledSurvivors[durableIndex];

			activeGroup.m_vPosition = position;
			activeGroup.m_vSourcePosition = position;
			activeGroup.m_iInfantryCount = survivors;
			activeGroup.m_iLastSeenAliveCount = survivors;
			activeGroup.m_iSurvivorInfantryCount = survivors;
			activeGroup.m_iDurableLivingInfantryCount = survivors;
			activeGroup.m_iSurvivorVehicleCount = 1;
			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
			element.m_vCurrentPosition = position;
			element.m_iSurvivingCrewCount = survivors;
			element.m_fVehicleDamageFraction = sampledDamageFractions[durableIndex];
			element.m_fFuelFraction = sampledFuelFractions[durableIndex];
			element.m_fAmmoFraction = sampledAmmoFractions[durableIndex];
			element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			element.m_iRevision++;
			bool vehicleDestroyedAtFold = element.m_fVehicleDamageFraction >= 0.999;
			if (vehicleDestroyedAtFold)
			{
				MarkMissionConvoyVehicleDestroyed(state, mission, asset, activeGroup.m_sGroupId, position, "vehicle destruction confirmed by exact fold sample");
				activeGroup.m_iSurvivorVehicleCount = 0;
				element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED;
				element.m_fVehicleDamageFraction = 1.0;
				element.m_bMobile = false;
				element.m_sTerminalReason = "vehicle destruction confirmed by exact fold sample";
				if (survivors > 0)
					activeGroup.m_sConvoyRuntimeStage = "DISMOUNTED_VIRTUAL";
				else
				{
					activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
					activeGroup.m_sConvoyRuntimeStage = "STRANDED_VIRTUAL";
				}
				SyncExactMissionConvoyRecoveryCarrierAssets(state, mission, element, position);
			}
			else if (survivors <= 0)
			{
				activeGroup.m_iSurvivorVehicleCount = 1;
				activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
				activeGroup.m_sConvoyRuntimeStage = "STRANDED_VIRTUAL";
				element.m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED;
				element.m_bMobile = false;
				element.m_sTerminalReason = "crew eliminated during fold reconciliation; vehicle remains recoverable";
				SyncExactMissionConvoyRecoveryCarrierAssets(state, mission, element, position);
			}

			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity)
				runtimeEntity.m_vPosition = position;
		}

		for (int terminalDurableIndex = 0; terminalDurableIndex < terminalGroups.Count(); terminalDurableIndex++)
		{
			HST_ActiveGroupState terminalGroup = terminalGroups[terminalDurableIndex];
			HST_MissionAssetState terminalAsset = terminalAssets[terminalDurableIndex];
			HST_ConvoyElementState terminalElement = terminalElements[terminalDurableIndex];
			vector terminalPosition = sampledTerminalPositions[terminalDurableIndex];
			int terminalSurvivors = sampledTerminalSurvivors[terminalDurableIndex];
			if (!IsZeroVector(terminalPosition))
			{
				terminalGroup.m_vPosition = terminalPosition;
				terminalGroup.m_vSourcePosition = terminalPosition;
				terminalAsset.m_vCurrentPosition = terminalPosition;
				terminalAsset.m_vLastKnownPosition = terminalPosition;
				terminalElement.m_vCurrentPosition = terminalPosition;
			}
			terminalGroup.m_iInfantryCount = terminalSurvivors;
			terminalGroup.m_iLastSeenAliveCount = terminalSurvivors;
			terminalGroup.m_iSurvivorInfantryCount = terminalSurvivors;
			terminalGroup.m_iDurableLivingInfantryCount = terminalSurvivors;
			bool recoverableAbandoned = terminalElement.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED
				&& !IsMissionConvoyVehicleAssetResolved(terminalAsset);
			terminalGroup.m_iSurvivorVehicleCount = 0;
			if (recoverableAbandoned)
			{
				terminalGroup.m_iSurvivorVehicleCount = 1;
				SyncExactMissionConvoyRecoveryCarrierAssets(state, mission, terminalElement, terminalPosition);
			}
			if (terminalSurvivors > 0)
				terminalGroup.m_sConvoyRuntimeStage = "DISMOUNTED_VIRTUAL";
			else
				terminalGroup.m_sConvoyRuntimeStage = "STRANDED_VIRTUAL";
			if (terminalSurvivors <= 0)
				terminalGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
			terminalElement.m_iSurvivingCrewCount = terminalSurvivors;
			terminalElement.m_fVehicleDamageFraction = sampledTerminalDamageFractions[terminalDurableIndex];
			terminalElement.m_fFuelFraction = sampledTerminalFuelFractions[terminalDurableIndex];
			terminalElement.m_fAmmoFraction = sampledTerminalAmmoFractions[terminalDurableIndex];
			terminalElement.m_bMobile = false;
			terminalElement.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			terminalElement.m_iRevision++;

			bool preserveTerminalVehicle = terminalDurableIndex < preserveTerminalVehicles.Count()
				&& preserveTerminalVehicles[terminalDurableIndex];
			if (preserveTerminalVehicle)
			{
				IEntity capturedVehicle = GetRuntimeVehicleEntity(terminalGroup.m_sGroupId);
				if (capturedVehicle)
					HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(capturedVehicle);
			}
			DeleteRuntimeGroupEntity(terminalGroup.m_sGroupId, !preserveTerminalVehicle);
			RemoveConvoyProgressStatusForGroup(terminalGroup.m_sGroupId);
			RemoveRestoredMissionConvoyRuntimeRebuildAttempt(terminalGroup.m_sGroupId);
			terminalGroup.m_bSpawnedEntity = false;
			terminalGroup.m_bSpawnAttempted = false;
			terminalGroup.m_sRuntimeEntityId = "";
			terminalGroup.m_iSpawnedAgentCount = 0;
			terminalGroup.m_iAssignedWaypointCount = 0;
			terminalAsset.m_bSpawned = false;
			terminalElement.m_bPhysicalized = false;
			HST_MissionRuntimeEntityState terminalRuntimeEntity = state.FindMissionRuntimeEntity(terminalAsset.m_sEntityId);
			if (terminalRuntimeEntity)
			{
				terminalRuntimeEntity.m_bSpawned = false;
				terminalRuntimeEntity.m_vPosition = terminalPosition;
			}
		}

		for (int retireIndex = 0; retireIndex < groups.Count(); retireIndex++)
		{
			HST_ActiveGroupState activeGroup = groups[retireIndex];
			HST_MissionAssetState asset = assets[retireIndex];
			HST_ConvoyElementState element = elements[retireIndex];
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			RemoveConvoyProgressStatusForGroup(activeGroup.m_sGroupId);

			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_bSpawnAttempted = false;
			activeGroup.m_sRuntimeEntityId = "";
			activeGroup.m_iSpawnedAgentCount = 0;
			activeGroup.m_iAssignedWaypointCount = 0;
			asset.m_bSpawned = false;
			element.m_bPhysicalized = false;

			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity)
				runtimeEntity.m_bSpawned = false;
		}

		m_bMarkerRefreshNeeded = true;
		reason = "exact mission convoy folded to durable element state";
		return true;
	}

	bool ReconcileInactiveMissionConvoyRuntime(HST_CampaignState state)
	{
		return CleanupInactiveMissionConvoyRuntime(state);
	}

	protected bool NormalizeRestoredMissionConvoyRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !state.m_bRestoredFromPersistence)
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
			return false;
		if (IsExactMissionConvoyContract(mission))
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
		if (IsExactMissionConvoyOutboundProjectionTransactionOpen(mission))
			return false;
		string reason;
		if (TryResolveMissionConvoyContactReasonForUpdate(state, mission, reason))
			return SetMissionConvoyContact(state, mission, reason);

		return TryClearExactMissionConvoyContact(state, mission);
	}

	protected bool TryClearExactMissionConvoyContact(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMissionConvoyContract(mission) || mission.m_sRuntimePhase != MISSION_CONVOY_CONTACT)
			return false;

		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		if (!operation || operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			return false;
		if (state.m_iElapsedSeconds < operation.m_iLastContactAtSecond + EXACT_CONVOY_CONTACT_CLEAR_SECONDS)
			return false;

		foreach (HST_MissionAssetState missionAsset : state.m_aMissionAssets)
		{
			if (!missionAsset || missionAsset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (missionAsset.m_sRole != MISSION_CONVOY_PAYLOAD_ROLE
				&& missionAsset.m_sRole != MISSION_CONVOY_CAPTIVE_ROLE)
				continue;
			if (HasUnsafeExactMissionConvoyCargoCarrierClaim(state, mission, missionAsset))
				return false;
		}

		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !element)
				return false;

			IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
			if (vehicleEntity && IsAnyPlayerInVehicle(vehicleEntity))
				return false;
			if (IsExactMissionConvoyTerminalSurvivingCrew(asset, element))
			{
				if (IsConvoyCrewPopulationPending(state, activeGroup) || IsConvoyCrewControlPending(state, activeGroup))
					return false;
				IEntity terminalCrewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
				if (!terminalCrewEntity || CountAliveRuntimeCrewAgents(activeGroup) != element.m_iSurvivingCrewCount)
					return false;
				continue;
			}
			if (element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
				&& element.m_iSurvivingCrewCount > 0)
				return false;
			if (element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
				|| element.m_iSurvivingCrewCount <= 0 || IsMissionConvoyVehicleAssetResolved(asset))
				continue;
			if (IsConvoyCrewPopulationPending(state, activeGroup) || IsConvoyCrewControlPending(state, activeGroup))
				return false;

			IEntity crewEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
			if (!crewEntity || !vehicleEntity || CountAliveRuntimeCrewAgents(activeGroup) <= 0)
				return false;
			string mobileReason;
			if (!GetConvoyVehicleControlAdapter().IsVehicleMobile(vehicleEntity, mobileReason))
				return false;
		}

		SetMissionConvoyMoving(state, mission);
		mission.m_sRuntimeFailureReason = "";
		mission.m_sLastRuntimeEventKey = CONVOY_CONTACT_CLEAR_EVENT_KEY;
		AssignMissionConvoyWaypoints(state, mission);
		m_bMarkerRefreshNeeded = true;
		Print(string.Format("Partisan exact mission convoy | %1 contact cleared after %2 seconds without new evidence", mission.m_sInstanceId, EXACT_CONVOY_CONTACT_CLEAR_SECONDS));
		return true;
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
		bool exactContract = IsExactMissionConvoyContract(mission);
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			int vehicleIndex = assetIndex;
			if (exactContract)
			{
				vehicleIndex = ResolveExactMissionConvoyVehicleAssetIndex(mission, asset);
				if (vehicleIndex < 0)
					continue;
			}
			else
			{
				assetIndex++;
			}

			string groupId = BuildMissionConvoyGroupId(mission, vehicleIndex);
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
			HST_ConvoyElementState exactElement;
			bool terminalSurvivingCrew;
			bool crewlessAbandonedElement;
			if (exactContract)
			{
				exactElement = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
				terminalSurvivingCrew = IsExactMissionConvoyTerminalSurvivingCrew(asset, exactElement);
				crewlessAbandonedElement = exactElement
					&& exactElement.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED
					&& exactElement.m_iSurvivingCrewCount <= 0 && !IsMissionConvoyVehicleAssetResolved(asset);
				if (!exactElement || (exactElement.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
					&& !terminalSurvivingCrew && !crewlessAbandonedElement))
					continue;
			}
			IEntity earlyVehicleEntity = GetRuntimeVehicleEntity(groupId);
			if (earlyVehicleEntity && !asset.m_bDestroyed && !terminalSurvivingCrew)
			{
				string earlyMobileReason;
				GetConvoyVehicleControlAdapter().IsVehicleMobile(earlyVehicleEntity, earlyMobileReason);
				if (earlyMobileReason == "vehicle is destroyed")
				{
					if (allowStateMutation)
						MarkMissionConvoyVehicleDestroyed(state, mission, asset, groupId, earlyVehicleEntity.GetOrigin(), earlyMobileReason);
					reason = string.Format("Convoy contact: vehicle %1 destroyed.", ReportText(asset.m_sAssetId));
					return true;
				}
			}

			if (asset.m_bDelivered && !terminalSurvivingCrew)
			{
				reason = string.Format("Convoy contact: vehicle %1 captured.", ReportText(asset.m_sAssetId));
				return true;
			}
			if (asset.m_bDestroyed && !terminalSurvivingCrew)
			{
				reason = string.Format("Convoy contact: vehicle %1 destroyed.", ReportText(asset.m_sAssetId));
				return true;
			}

			vector position;
			if (terminalSurvivingCrew && activeGroup)
				position = ResolveActiveGroupLiveRuntimePosition(activeGroup, true);
			else
				position = ResolveMissionConvoyVehiclePosition(asset, groupId);
			if (IsZeroVector(position) && activeGroup)
				position = activeGroup.m_vPosition;

			float nearestPlayer = ResolveNearestLivingPlayerDistanceMeters(position);
			if (nearestPlayer >= 0 && nearestPlayer <= CONVOY_CONTACT_RADIUS_METERS)
			{
				if (terminalSurvivingCrew)
					reason = string.Format("Convoy contact: living player within %1m contact radius at %2m from dismounted crew %3.", Math.Round(CONVOY_CONTACT_RADIUS_METERS), Math.Round(nearestPlayer), ReportText(groupId));
				else
					reason = string.Format("Convoy contact: living player within %1m contact radius at %2m from vehicle %3.", Math.Round(CONVOY_CONTACT_RADIUS_METERS), Math.Round(nearestPlayer), ReportText(asset.m_sAssetId));
				return true;
			}

			if (!activeGroup || !activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED)
				continue;

			int aliveCrew = CountAliveRuntimeCrewAgents(activeGroup);
			if (activeGroup.m_iLastSeenAliveCount > 0 && aliveCrew < activeGroup.m_iLastSeenAliveCount && !IsConvoyCrewControlPending(state, activeGroup))
			{
				if (terminalSurvivingCrew || mission.m_sRuntimePhase == MISSION_CONVOY_CONTACT)
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
			if (terminalSurvivingCrew)
				continue;

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
		if (IsExactMissionConvoyContract(mission))
		{
			HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
			if (operation && operation.m_iLastContactAtSecond != state.m_iElapsedSeconds)
			{
				operation.m_iLastContactAtSecond = state.m_iElapsedSeconds;
				operation.m_iRevision++;
				changed = true;
			}
		}
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
				Print(string.Format("Partisan mission convoy | %1 entered contact: %2", mission.m_sInstanceId, reason));
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
		Print(string.Format("Partisan mission convoy | %1 vehicle %2 marked destroyed: %3", mission.m_sInstanceId, asset.m_sAssetId, ReportText(reason)));
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

	protected bool IsCurrentSchemaExactMissionConvoyDurableClaimant(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return false;
		bool deterministicPrefixAnchor = activeGroup.m_sGroupId.StartsWith(MISSION_CONVOY_GROUP_PREFIX);
		bool elementIdentityAnchor = activeGroup.m_sConvoyElementId.Contains("convoy_element_");
		bool deterministicAuthorityIdAnchor = activeGroup.m_sOperationId.Contains("operation_mission_convoy_")
			|| activeGroup.m_sManifestId.StartsWith("manifest_mission_convoy_")
			|| activeGroup.m_sProjectionId.StartsWith("projection_mission_convoy_")
			|| activeGroup.m_sForceId.StartsWith("force_mission_convoy_")
			|| activeGroup.m_sMissionAssetId.Contains("_convoy_vehicle_");
		bool hasAuthorityBacklink = !activeGroup.m_sOperationId.IsEmpty()
			|| !activeGroup.m_sManifestId.IsEmpty()
			|| !activeGroup.m_sMissionAssetId.IsEmpty();
		if (!deterministicPrefixAnchor && !elementIdentityAnchor && !deterministicAuthorityIdAnchor && !hasAuthorityBacklink)
			return false;
		bool deterministicGroupId;
		if (!activeGroup.m_sMissionInstanceId.IsEmpty())
		{
			for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
			{
				string expectedGroupId = string.Format("%1%2_%3", MISSION_CONVOY_GROUP_PREFIX, activeGroup.m_sMissionInstanceId, index);
				if (activeGroup.m_sGroupId == expectedGroupId)
				{
					deterministicGroupId = true;
					break;
				}
			}
		}

		foreach (HST_OperationRecordState operation : state.m_aOperations)
		{
			if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
				|| operation.m_iContractVersion != HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION)
				continue;
			if ((!activeGroup.m_sOperationId.IsEmpty() && operation.m_sOperationId == activeGroup.m_sOperationId)
				|| operation.m_sMissionInstanceId == activeGroup.m_sMissionInstanceId
				|| (!activeGroup.m_sManifestId.IsEmpty() && operation.m_sManifestId == activeGroup.m_sManifestId))
				return true;
		}
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
		{
			if (!manifest || !manifest.m_bFrozen
				|| manifest.m_sForceKind != HST_MissionConvoyOperationService.EXACT_FORCE_KIND
				|| manifest.m_sPolicyId != HST_MissionConvoyOperationService.EXACT_POLICY_ID)
				continue;
			if ((!activeGroup.m_sManifestId.IsEmpty() && manifest.m_sManifestId == activeGroup.m_sManifestId)
				|| (!activeGroup.m_sOperationId.IsEmpty() && manifest.m_sOperationId == activeGroup.m_sOperationId))
				return true;
		}
		foreach (HST_ConvoyElementState element : state.m_aConvoyElements)
		{
			if (!element || !element.m_sElementId.Contains("convoy_element_")
				|| element.m_sVehicleSlotId.IsEmpty() || element.m_sCrewGroupElementId.IsEmpty())
				continue;
			if ((!activeGroup.m_sConvoyElementId.IsEmpty() && element.m_sElementId == activeGroup.m_sConvoyElementId)
				|| element.m_sGroupId == activeGroup.m_sGroupId
				|| element.m_sMissionInstanceId == activeGroup.m_sMissionInstanceId)
				return true;
		}
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE
				|| !asset.m_sConvoyElementId.Contains("convoy_element_"))
				continue;
			if ((!activeGroup.m_sMissionAssetId.IsEmpty() && asset.m_sAssetId == activeGroup.m_sMissionAssetId)
				|| (!activeGroup.m_sConvoyElementId.IsEmpty() && asset.m_sConvoyElementId == activeGroup.m_sConvoyElementId)
				|| asset.m_sMissionInstanceId == activeGroup.m_sMissionInstanceId)
				return true;
		}
		// Prefix/element/backlink evidence is itself a current-schema claimant.
		// Missing corroboration is corruption to preserve, not permission to let
		// generic cleanup silently delete the only durable evidence row.
		return deterministicPrefixAnchor || deterministicGroupId || elementIdentityAnchor || deterministicAuthorityIdAnchor;
	}

	protected bool CleanupInactiveMissionConvoyRuntime(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		array<string> protectedMissionIds = {};
		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			bool durableExactClaimant = IsCurrentSchemaExactMissionConvoyDurableClaimant(state, activeGroup);
			if (!IsMissionConvoyGroup(activeGroup) && !durableExactClaimant)
				continue;
			HST_ActiveMissionState protectedMission = FindMissionForConvoyGroup(state, activeGroup);
			if (IsProtectedExactMissionConvoyAuthority(protectedMission))
			{
				if (!protectedMissionIds.Contains(protectedMission.m_sInstanceId))
				{
					protectedMissionIds.Insert(protectedMission.m_sInstanceId);
					if (protectedMission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION)
					{
						HST_OperationRecordState protectedOperation = state.FindOperation(protectedMission.m_sOperationId);
						if (protectedOperation
							&& protectedOperation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
							&& protectedMission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
						{
							string retireReason;
							changed = RetireExactMissionConvoyRuntime(state, protectedMission, retireReason) || changed;
						}
					}
					else
					{
						// Quarantine preserves every ambiguous durable row.  Process-local
						// runtime handles may still be discarded by deterministic mission
						// group ID, without rewriting the claimed group/asset/element graph.
						for (int quarantineIndex = 0; quarantineIndex < EXACT_MISSION_CONVOY_VEHICLE_COUNT; quarantineIndex++)
						{
							string quarantineGroupId = BuildMissionConvoyGroupId(protectedMission, quarantineIndex);
							if (!GetRuntimeCrewGroupEntity(quarantineGroupId) && !GetRuntimeVehicleEntity(quarantineGroupId))
								continue;
							DeleteRuntimeGroupEntity(quarantineGroupId);
							RemoveConvoyProgressStatusForGroup(quarantineGroupId);
							changed = true;
						}
					}
				}
				continue;
			}
			if (durableExactClaimant)
			{
				bool hadProcessProjection = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId) != null
					|| GetRuntimeVehicleEntity(activeGroup.m_sGroupId) != null
					|| CountExactMissionConvoyMemberMappings(activeGroup.m_sMissionInstanceId, activeGroup.m_sGroupId) > 0;
				ClearPendingActiveGroupPopulation(activeGroup);
				DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
				RemoveConvoyProgressStatusForGroup(activeGroup.m_sGroupId);
				changed = hadProcessProjection || changed;
				continue;
			}
			if (ShouldHoldForceSpawnProjection(state, activeGroup))
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
				Print(string.Format("Partisan mission convoy | cleaned inactive convoy runtime group %1 and preserved neutralized vehicle", activeGroup.m_sGroupId));
			else
				Print(string.Format("Partisan mission convoy | cleaned inactive convoy runtime group %1", activeGroup.m_sGroupId));
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
		Print(string.Format("Partisan mission convoy | preserving expired engaged convoy runtime %1 until players leave render bubble", activeGroup.m_sGroupId));
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

	protected HST_ExactMissionConvoyOutboundProjectionTransaction FindExactMissionConvoyOutboundProjectionTransaction(string missionInstanceId)
	{
		if (missionInstanceId.IsEmpty())
			return null;
		foreach (HST_ExactMissionConvoyOutboundProjectionTransaction transaction : m_aExactMissionConvoyOutboundProjectionTransactions)
		{
			if (transaction && transaction.m_sMissionInstanceId == missionInstanceId)
				return transaction;
		}
		return null;
	}

	bool IsExactMissionConvoyOutboundProjectionTransactionOpen(HST_ActiveMissionState mission)
	{
		return IsExactMissionConvoyContract(mission)
			&& FindExactMissionConvoyOutboundProjectionTransaction(mission.m_sInstanceId) != null;
	}

	bool CommitExactMissionConvoyOutboundProjectionTransaction(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionRuntimeService missionRuntime,
		out string reason)
	{
		reason = "";
		if (!state || !mission || !missionRuntime)
		{
			reason = "exact outbound publication participants are missing";
			return false;
		}
		HST_ExactMissionConvoyOutboundProjectionTransaction transaction = FindExactMissionConvoyOutboundProjectionTransaction(mission.m_sInstanceId);
		if (!transaction || transaction.m_State != state || transaction.m_Mission != mission)
		{
			reason = "exact outbound publication transaction is missing or conflicting";
			return false;
		}
		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		if (!operation || operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| !IsExactMissionConvoySurvivorProjectionReady(state, mission)
			|| !missionRuntime.IsExactMissionConvoyCargoProjectionReady(state, mission, this))
		{
			reason = "exact outbound publication participants lost physical readiness at commit";
			RollbackExactMissionConvoyOutboundProjectionTransaction(state, transaction, reason, true);
			return false;
		}
		if (!missionRuntime.SetExactMissionConvoyCargoProjectionPublication(state, mission, this, true))
		{
			reason = "exact outbound cargo participant could not publish from current frozen readiness";
			missionRuntime.SetExactMissionConvoyCargoProjectionPublication(state, mission, this, false);
			RollbackExactMissionConvoyOutboundProjectionTransaction(state, transaction, reason, true);
			return false;
		}
		if (!CompleteExactMissionConvoyOutboundProjectionTransaction(transaction))
		{
			reason = "exact outbound root/member participants could not publish atomically";
			missionRuntime.SetExactMissionConvoyCargoProjectionPublication(state, mission, this, false);
			return false;
		}
		return true;
	}

	protected HST_ExactMissionConvoyOutboundProjectionTransaction BeginExactMissionConvoyOutboundProjectionTransaction(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		if (!state || !IsExactActiveMissionConvoy(mission) || IsExactMissionConvoyRecoveryHold(mission))
			return null;
		HST_ExactMissionConvoyOutboundProjectionTransaction existing = FindExactMissionConvoyOutboundProjectionTransaction(mission.m_sInstanceId);
		if (existing)
			return existing;
		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		if (!operation || operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			return null;
		if (CountExactMissionConvoyMemberMappings(mission.m_sInstanceId) != 0)
		{
			DebugLog(string.Format("exact mission convoy outbound transaction rejected stale member identity mappings %1", mission.m_sInstanceId));
			return null;
		}

		HST_ExactMissionConvoyOutboundProjectionTransaction transaction = new HST_ExactMissionConvoyOutboundProjectionTransaction();
		transaction.m_State = state;
		transaction.m_Mission = mission;
		transaction.m_sMissionInstanceId = mission.m_sInstanceId;
		transaction.m_sOriginalMissionFailureReason = mission.m_sRuntimeFailureReason;
		transaction.m_iStartedAtSecond = state.m_iElapsedSeconds;

		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !element || IsForceSpawnRuntimeOwnershipHeldForGroup(activeGroup.m_sGroupId))
				return null;
			IEntity existingCrewRuntime = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
			IEntity existingVehicleRuntime = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
			if (existingCrewRuntime || existingVehicleRuntime || HasRuntimeVehicleRegistration(activeGroup.m_sGroupId)
				|| FindPendingActiveGroupPopulationIndex(activeGroup.m_sGroupId) >= 0)
			{
				DebugLog(string.Format("exact mission convoy outbound transaction rejected preexisting process registration %1 | crew %2 vehicle %3 registered %4", activeGroup.m_sGroupId, existingCrewRuntime != null, existingVehicleRuntime != null, HasRuntimeVehicleRegistration(activeGroup.m_sGroupId)));
				return null;
			}

			HST_ExactMissionConvoyRootProjectionSnapshot rootSnapshot = new HST_ExactMissionConvoyRootProjectionSnapshot();
			rootSnapshot.m_Group = activeGroup;
			rootSnapshot.m_Element = element;
			rootSnapshot.m_CrewRuntimeBefore = existingCrewRuntime;
			rootSnapshot.m_VehicleRuntimeBefore = existingVehicleRuntime;
			rootSnapshot.m_sGroupPrefab = activeGroup.m_sPrefab;
			rootSnapshot.m_sGroupSpawnFallbackMode = activeGroup.m_sSpawnFallbackMode;
			rootSnapshot.m_sGroupSpawnFailureReason = activeGroup.m_sSpawnFailureReason;
			rootSnapshot.m_sGroupRuntimeEntityId = activeGroup.m_sRuntimeEntityId;
			rootSnapshot.m_sGroupRuntimeStatus = activeGroup.m_sRuntimeStatus;
			rootSnapshot.m_sGroupCrewPopulationFailureReason = activeGroup.m_sCrewPopulationFailureReason;
			rootSnapshot.m_sGroupConvoyRuntimeStage = activeGroup.m_sConvoyRuntimeStage;
			rootSnapshot.m_vGroupPosition = activeGroup.m_vPosition;
			rootSnapshot.m_vGroupSourcePosition = activeGroup.m_vSourcePosition;
			rootSnapshot.m_iGroupInfantryCount = activeGroup.m_iInfantryCount;
			rootSnapshot.m_iGroupSpawnedAtSecond = activeGroup.m_iSpawnedAtSecond;
			rootSnapshot.m_iGroupLastSeenAliveCount = activeGroup.m_iLastSeenAliveCount;
			rootSnapshot.m_iGroupSurvivorInfantryCount = activeGroup.m_iSurvivorInfantryCount;
			rootSnapshot.m_iGroupSurvivorVehicleCount = activeGroup.m_iSurvivorVehicleCount;
			rootSnapshot.m_iGroupSpawnedAgentCount = activeGroup.m_iSpawnedAgentCount;
			rootSnapshot.m_iGroupAssignedWaypointCount = activeGroup.m_iAssignedWaypointCount;
			rootSnapshot.m_iGroupMaxObservedCrewAlive = activeGroup.m_iMaxObservedCrewAlive;
			rootSnapshot.m_iGroupDurableLivingInfantryCount = activeGroup.m_iDurableLivingInfantryCount;
			rootSnapshot.m_iGroupLifecycleRevision = activeGroup.m_iLifecycleRevision;
			rootSnapshot.m_bGroupEverHadLivingCrew = activeGroup.m_bEverHadLivingCrew;
			rootSnapshot.m_bGroupEverPopulated = activeGroup.m_bEverPopulated;
			rootSnapshot.m_bGroupSpawnCompleted = activeGroup.m_bSpawnCompleted;
			rootSnapshot.m_bGroupCrewPopulationTerminallyFailed = activeGroup.m_bCrewPopulationTerminallyFailed;
			rootSnapshot.m_bGroupSpawnAttempted = activeGroup.m_bSpawnAttempted;
			rootSnapshot.m_bGroupSpawnedEntity = activeGroup.m_bSpawnedEntity;
			rootSnapshot.m_vElementPosition = element.m_vCurrentPosition;
			rootSnapshot.m_sElementTerminalReason = element.m_sTerminalReason;
			rootSnapshot.m_iElementSurvivingCrewCount = element.m_iSurvivingCrewCount;
			rootSnapshot.m_iElementLastUpdatedSecond = element.m_iLastUpdatedSecond;
			rootSnapshot.m_iElementRevision = element.m_iRevision;
			rootSnapshot.m_fElementVehicleDamageFraction = element.m_fVehicleDamageFraction;
			rootSnapshot.m_fElementFuelFraction = element.m_fFuelFraction;
			rootSnapshot.m_fElementAmmoFraction = element.m_fAmmoFraction;
			rootSnapshot.m_eElementDisposition = element.m_eDisposition;
			rootSnapshot.m_bElementPhysicalized = element.m_bPhysicalized;
			rootSnapshot.m_bElementMobile = element.m_bMobile;
			transaction.m_aRootSnapshots.Insert(rootSnapshot);
		}

		foreach (HST_MissionAssetState missionAsset : state.m_aMissionAssets)
		{
			if (!missionAsset || missionAsset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			HST_ExactMissionConvoyAssetProjectionSnapshot assetSnapshot = new HST_ExactMissionConvoyAssetProjectionSnapshot();
			assetSnapshot.m_Asset = missionAsset;
			assetSnapshot.m_vCurrentPosition = missionAsset.m_vCurrentPosition;
			assetSnapshot.m_vLastKnownPosition = missionAsset.m_vLastKnownPosition;
			assetSnapshot.m_sCarriedByVehicleId = missionAsset.m_sCarriedByVehicleId;
			assetSnapshot.m_sLastInteraction = missionAsset.m_sLastInteraction;
			assetSnapshot.m_bSpawned = missionAsset.m_bSpawned;
			assetSnapshot.m_bAlive = missionAsset.m_bAlive;
			assetSnapshot.m_bAttachedToCarrier = missionAsset.m_bAttachedToCarrier;
			HST_MissionRuntimeEntityState runtimeEntity;
			if (!missionAsset.m_sEntityId.IsEmpty())
				runtimeEntity = state.FindMissionRuntimeEntity(missionAsset.m_sEntityId);
			if (runtimeEntity)
			{
				assetSnapshot.m_RuntimeEntity = runtimeEntity;
				assetSnapshot.m_bRuntimeEntityExisted = true;
				assetSnapshot.m_sRuntimeKind = runtimeEntity.m_sKind;
				assetSnapshot.m_sRuntimePrefab = runtimeEntity.m_sPrefab;
				assetSnapshot.m_vRuntimePosition = runtimeEntity.m_vPosition;
				assetSnapshot.m_vRuntimeAngles = runtimeEntity.m_vAngles;
				assetSnapshot.m_bRuntimeSpawned = runtimeEntity.m_bSpawned;
				assetSnapshot.m_bRuntimeDestroyed = runtimeEntity.m_bDestroyed;
				assetSnapshot.m_bRuntimeRecovered = runtimeEntity.m_bRecovered;
			}
			transaction.m_aAssetSnapshots.Insert(assetSnapshot);
		}

		if (transaction.m_aRootSnapshots.Count() != EXACT_MISSION_CONVOY_VEHICLE_COUNT)
			return null;
		m_aExactMissionConvoyOutboundProjectionTransactions.Insert(transaction);
		DebugLog(string.Format("exact mission convoy outbound materialization transaction began %1 roots %2 assets %3", mission.m_sInstanceId, transaction.m_aRootSnapshots.Count(), transaction.m_aAssetSnapshots.Count()));
		return transaction;
	}

	protected bool IsExactMissionConvoyTerminalRootSpawnFailure(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return true;
		if (activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_bCrewPopulationTerminallyFailed)
			return true;
		return activeGroup.m_sConvoyRuntimeStage == "FAILED"
			|| activeGroup.m_sConvoyRuntimeStage == "TERMINAL_CREW_AUTHORITY_REJECTED"
			|| activeGroup.m_sConvoyRuntimeStage == "TERMINAL_CREW_COUNT_REJECTED"
			|| activeGroup.m_sConvoyRuntimeStage == "TERMINAL_CREW_POSITION_REJECTED"
			|| activeGroup.m_sConvoyRuntimeStage == "TERMINAL_VEHICLE_RESURRECTION_REJECTED"
			|| activeGroup.m_sConvoyRuntimeStage == "TERMINAL_CREW_ROSTER_REJECTED";
	}

	protected string BuildExactMissionConvoyTerminalRootSpawnFailure(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "exact outbound materialization lost a required durable group root";
		string reason = activeGroup.m_sCrewPopulationFailureReason;
		if (reason.IsEmpty())
			reason = activeGroup.m_sSpawnFailureReason;
		if (reason.IsEmpty())
			reason = activeGroup.m_sConvoyRuntimeStage;
		if (reason.IsEmpty())
			reason = "required root entered a terminal spawn state";
		return string.Format("exact outbound root %1 failed: %2", activeGroup.m_sGroupId, reason);
	}

	protected bool HasCompleteExactMissionConvoyOutboundAttemptHandles(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactActiveMissionConvoy(mission))
			return false;
		bool hasRequiredRoot;
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !element)
				return false;
			if (IsExactMissionConvoyRecoveryVehicleEligible(asset, element))
			{
				hasRequiredRoot = true;
				if (GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId)
					|| !GetExactMissionConvoyVehicleRuntimeEntity(state, mission, element.m_sVehicleSlotId))
					return false;
				continue;
			}
			if (element.m_iSurvivingCrewCount <= 0)
				continue;
			hasRequiredRoot = true;
			IEntity crewRuntime = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
			IEntity vehicleRuntime = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
			if (IsExactMissionConvoyTerminalSurvivingCrew(asset, element))
			{
				if (!crewRuntime || vehicleRuntime || HasRuntimeVehicleRegistration(activeGroup.m_sGroupId))
					return false;
				continue;
			}
			if (element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
				|| IsMissionConvoyVehicleAssetResolved(asset) || !crewRuntime || !vehicleRuntime)
				return false;
		}
		return hasRequiredRoot;
	}

	protected bool RollbackExactMissionConvoyOutboundProjectionTransaction(
		HST_CampaignState state,
		HST_ExactMissionConvoyOutboundProjectionTransaction transaction,
		string reason,
		bool terminalFailure)
	{
		if (!state || !transaction || transaction.m_State != state)
			return false;
		if (transaction.m_bRolledBackTerminally)
			return false;

		foreach (HST_ExactMissionConvoyRootProjectionSnapshot rootSnapshot : transaction.m_aRootSnapshots)
		{
			if (!rootSnapshot || !rootSnapshot.m_Group || !rootSnapshot.m_Element)
				continue;
			HST_ActiveGroupState activeGroup = rootSnapshot.m_Group;
			ClearPendingActiveGroupPopulation(activeGroup);
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			RemoveConvoyProgressStatusForGroup(activeGroup.m_sGroupId);
			RemoveRestoredMissionConvoyRuntimeRebuildAttempt(activeGroup.m_sGroupId);

			activeGroup.m_sPrefab = rootSnapshot.m_sGroupPrefab;
			activeGroup.m_sSpawnFallbackMode = rootSnapshot.m_sGroupSpawnFallbackMode;
			activeGroup.m_sSpawnFailureReason = rootSnapshot.m_sGroupSpawnFailureReason;
			activeGroup.m_sRuntimeEntityId = rootSnapshot.m_sGroupRuntimeEntityId;
			activeGroup.m_sRuntimeStatus = rootSnapshot.m_sGroupRuntimeStatus;
			activeGroup.m_sCrewPopulationFailureReason = rootSnapshot.m_sGroupCrewPopulationFailureReason;
			activeGroup.m_sConvoyRuntimeStage = rootSnapshot.m_sGroupConvoyRuntimeStage;
			activeGroup.m_vPosition = rootSnapshot.m_vGroupPosition;
			activeGroup.m_vSourcePosition = rootSnapshot.m_vGroupSourcePosition;
			activeGroup.m_iInfantryCount = rootSnapshot.m_iGroupInfantryCount;
			activeGroup.m_iSpawnedAtSecond = rootSnapshot.m_iGroupSpawnedAtSecond;
			activeGroup.m_iLastSeenAliveCount = rootSnapshot.m_iGroupLastSeenAliveCount;
			activeGroup.m_iSurvivorInfantryCount = rootSnapshot.m_iGroupSurvivorInfantryCount;
			activeGroup.m_iSurvivorVehicleCount = rootSnapshot.m_iGroupSurvivorVehicleCount;
			activeGroup.m_iSpawnedAgentCount = rootSnapshot.m_iGroupSpawnedAgentCount;
			activeGroup.m_iAssignedWaypointCount = rootSnapshot.m_iGroupAssignedWaypointCount;
			activeGroup.m_iMaxObservedCrewAlive = rootSnapshot.m_iGroupMaxObservedCrewAlive;
			activeGroup.m_iDurableLivingInfantryCount = rootSnapshot.m_iGroupDurableLivingInfantryCount;
			activeGroup.m_iLifecycleRevision = rootSnapshot.m_iGroupLifecycleRevision;
			activeGroup.m_bEverHadLivingCrew = rootSnapshot.m_bGroupEverHadLivingCrew;
			activeGroup.m_bEverPopulated = rootSnapshot.m_bGroupEverPopulated;
			activeGroup.m_bSpawnCompleted = rootSnapshot.m_bGroupSpawnCompleted;
			activeGroup.m_bCrewPopulationTerminallyFailed = rootSnapshot.m_bGroupCrewPopulationTerminallyFailed;
			activeGroup.m_bSpawnAttempted = rootSnapshot.m_bGroupSpawnAttempted;
			activeGroup.m_bSpawnedEntity = rootSnapshot.m_bGroupSpawnedEntity;

			HST_ConvoyElementState element = rootSnapshot.m_Element;
			element.m_vCurrentPosition = rootSnapshot.m_vElementPosition;
			element.m_sTerminalReason = rootSnapshot.m_sElementTerminalReason;
			element.m_iSurvivingCrewCount = rootSnapshot.m_iElementSurvivingCrewCount;
			element.m_iLastUpdatedSecond = rootSnapshot.m_iElementLastUpdatedSecond;
			element.m_iRevision = rootSnapshot.m_iElementRevision;
			element.m_fVehicleDamageFraction = rootSnapshot.m_fElementVehicleDamageFraction;
			element.m_fFuelFraction = rootSnapshot.m_fElementFuelFraction;
			element.m_fAmmoFraction = rootSnapshot.m_fElementAmmoFraction;
			element.m_eDisposition = rootSnapshot.m_eElementDisposition;
			element.m_bPhysicalized = rootSnapshot.m_bElementPhysicalized;
			element.m_bMobile = rootSnapshot.m_bElementMobile;
			RefreshActiveGroupZoneCounts(state, activeGroup);
		}

		foreach (HST_ExactMissionConvoyAssetProjectionSnapshot assetSnapshot : transaction.m_aAssetSnapshots)
		{
			if (!assetSnapshot || !assetSnapshot.m_Asset)
				continue;
			HST_MissionAssetState asset = assetSnapshot.m_Asset;
			asset.m_vCurrentPosition = assetSnapshot.m_vCurrentPosition;
			asset.m_vLastKnownPosition = assetSnapshot.m_vLastKnownPosition;
			asset.m_sCarriedByVehicleId = assetSnapshot.m_sCarriedByVehicleId;
			asset.m_sLastInteraction = assetSnapshot.m_sLastInteraction;
			asset.m_bSpawned = assetSnapshot.m_bSpawned;
			asset.m_bAlive = assetSnapshot.m_bAlive;
			asset.m_bAttachedToCarrier = assetSnapshot.m_bAttachedToCarrier;

			HST_MissionRuntimeEntityState runtimeEntity;
			if (!asset.m_sEntityId.IsEmpty())
				runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (!assetSnapshot.m_bRuntimeEntityExisted)
			{
				for (int runtimeIndex = state.m_aMissionRuntimeEntities.Count() - 1; runtimeIndex >= 0; runtimeIndex--)
				{
					HST_MissionRuntimeEntityState candidateRuntime = state.m_aMissionRuntimeEntities[runtimeIndex];
					if (candidateRuntime && candidateRuntime.m_sRuntimeEntityId == asset.m_sEntityId
						&& candidateRuntime.m_sMissionInstanceId == transaction.m_sMissionInstanceId)
						state.m_aMissionRuntimeEntities.Remove(runtimeIndex);
				}
				continue;
			}
			if (!runtimeEntity)
			{
				if (assetSnapshot.m_RuntimeEntity)
				{
					state.m_aMissionRuntimeEntities.Insert(assetSnapshot.m_RuntimeEntity);
					runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
				}
			}
			if (runtimeEntity)
			{
				runtimeEntity.m_sKind = assetSnapshot.m_sRuntimeKind;
				runtimeEntity.m_sPrefab = assetSnapshot.m_sRuntimePrefab;
				runtimeEntity.m_vPosition = assetSnapshot.m_vRuntimePosition;
				runtimeEntity.m_vAngles = assetSnapshot.m_vRuntimeAngles;
				runtimeEntity.m_bSpawned = assetSnapshot.m_bRuntimeSpawned;
				runtimeEntity.m_bDestroyed = assetSnapshot.m_bRuntimeDestroyed;
				runtimeEntity.m_bRecovered = assetSnapshot.m_bRuntimeRecovered;
			}
		}

		if (transaction.m_Mission)
			transaction.m_Mission.m_sRuntimeFailureReason = transaction.m_sOriginalMissionFailureReason;
		transaction.m_sTerminalFailureReason = reason;
		transaction.m_bRolledBackTerminally = terminalFailure;
		if (terminalFailure && transaction.m_Mission)
		{
			HST_OperationRecordState failedOperation = ResolveExactMissionConvoyOperationForRuntime(state, transaction.m_Mission);
			if (failedOperation)
			{
				failedOperation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
				failedOperation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
				failedOperation.m_iMaterializationStateEnteredAtSecond = state.m_iElapsedSeconds;
				failedOperation.m_iLastProjectionDecisionSecond = state.m_iElapsedSeconds;
				failedOperation.m_sLastProjectionReason = "terminal outbound materialization failure: " + reason;
				failedOperation.m_iRevision++;
			}
			SetMissionConvoyFailure(state, transaction.m_Mission, "Exact convoy materialization failed: " + reason);
		}
		m_bMarkerRefreshNeeded = true;
		string rollbackReport = string.Format("Partisan exact mission convoy | rolled back outbound materialization %1 | terminal %2 | %3", transaction.m_sMissionInstanceId, terminalFailure, ReportText(reason));
		if (terminalFailure)
			Print(rollbackReport, LogLevel.WARNING);
		else
			Print(rollbackReport);
		RemoveExactMissionConvoyOutboundProjectionTransaction(transaction);
		return true;
	}

	protected bool CompleteExactMissionConvoyOutboundProjectionTransaction(HST_ExactMissionConvoyOutboundProjectionTransaction transaction)
	{
		if (!transaction)
			return false;
		SetExactMissionConvoyOutboundProjectionTransactionVisible(transaction, true);
		if (!IsExactMissionConvoyOutboundProjectionTransactionPublished(transaction, true))
		{
			SetExactMissionConvoyOutboundProjectionTransactionVisible(transaction, false);
			if (transaction.m_State)
				RollbackExactMissionConvoyOutboundProjectionTransaction(transaction.m_State, transaction, "exact outbound publication did not activate every required root and frozen member", true);
			return false;
		}
		DebugLog(string.Format("exact mission convoy outbound materialization transaction committed %1", transaction.m_sMissionInstanceId));
		RemoveExactMissionConvoyOutboundProjectionTransaction(transaction);
		return true;
	}

	protected void SetExactMissionConvoyOutboundProjectionTransactionVisible(
		HST_ExactMissionConvoyOutboundProjectionTransaction transaction,
		bool visible)
	{
		if (!transaction)
			return;
		foreach (HST_ExactMissionConvoyRootProjectionSnapshot rootSnapshot : transaction.m_aRootSnapshots)
		{
			if (!rootSnapshot || !rootSnapshot.m_Group)
				continue;
			IEntity crewRuntime = GetRuntimeCrewGroupEntity(rootSnapshot.m_Group.m_sGroupId);
			IEntity vehicleRuntime = GetRuntimeVehicleEntity(rootSnapshot.m_Group.m_sGroupId);
			SetExactMissionConvoyProjectionEntityPublished(crewRuntime, visible);
			SetExactMissionConvoyProjectionEntityPublished(vehicleRuntime, visible);
		}
		if (!HasConsistentExactMissionConvoyMemberIdentityArrays())
			return;
		for (int memberIndex = 0; memberIndex < m_aExactMissionConvoyMemberEntities.Count(); memberIndex++)
		{
			if (m_aExactMissionConvoyMemberMissionIds[memberIndex] != transaction.m_sMissionInstanceId)
				continue;
			SetExactMissionConvoyProjectionEntityPublished(m_aExactMissionConvoyMemberEntities[memberIndex], visible);
		}
	}

	protected bool IsExactMissionConvoyOutboundProjectionTransactionPublished(
		HST_ExactMissionConvoyOutboundProjectionTransaction transaction,
		bool published)
	{
		if (!transaction || !HasConsistentExactMissionConvoyMemberIdentityArrays())
			return false;
		EntityFlags publicationFlags = EntityFlags.ACTIVE | EntityFlags.VISIBLE | EntityFlags.TRACEABLE;
		foreach (HST_ExactMissionConvoyRootProjectionSnapshot rootSnapshot : transaction.m_aRootSnapshots)
		{
			if (!rootSnapshot || !rootSnapshot.m_Group)
				return false;
			IEntity crewRuntime = GetRuntimeCrewGroupEntity(rootSnapshot.m_Group.m_sGroupId);
			IEntity vehicleRuntime = GetRuntimeVehicleEntity(rootSnapshot.m_Group.m_sGroupId);
			if (!HasExactMissionConvoyProjectionPublicationState(crewRuntime, publicationFlags, published)
				|| !HasExactMissionConvoyProjectionPublicationState(vehicleRuntime, publicationFlags, published))
				return false;
		}

		int mappedMembers;
		for (int memberIndex = 0; memberIndex < m_aExactMissionConvoyMemberEntities.Count(); memberIndex++)
		{
			if (m_aExactMissionConvoyMemberMissionIds[memberIndex] != transaction.m_sMissionInstanceId)
				continue;
			mappedMembers++;
			if (!HasExactMissionConvoyProjectionPublicationState(m_aExactMissionConvoyMemberEntities[memberIndex], publicationFlags, published))
				return false;
		}
		return mappedMembers > 0
			&& mappedMembers == CountExactMissionConvoyMemberMappings(transaction.m_sMissionInstanceId);
	}

	protected bool HasExactMissionConvoyProjectionPublicationState(
		IEntity entity,
		EntityFlags publicationFlags,
		bool published)
	{
		if (!entity || entity.IsDeleted())
			return false;
		bool hasAllPublicationFlags = (entity.GetFlags() & publicationFlags) == publicationFlags;
		return hasAllPublicationFlags == published;
	}

	protected void SetExactMissionConvoyProjectionEntityPublished(IEntity entity, bool published)
	{
		if (!entity || entity.IsDeleted())
			return;

		EntityFlags publicationFlags = EntityFlags.ACTIVE | EntityFlags.VISIBLE | EntityFlags.TRACEABLE;
		if (published)
			entity.SetFlags(publicationFlags, true);
		else
			entity.ClearFlags(publicationFlags, true);
	}

	protected bool ReconcileExactMissionConvoyAbandonedVehiclePublication(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMissionConvoyContract(mission))
				continue;
			HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
			if (operation && operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
			{
				SetExactMissionConvoyAbandonedVehicleProjectionPublished(state, mission, false);
				continue;
			}
			if (operation && operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
			{
				bool readyToPublish;
				if (IsExactMissionConvoyRecoveryHold(mission))
					readyToPublish = IsExactMissionConvoyRecoveryProjectionReady(state, mission);
				else
					readyToPublish = IsExactMissionConvoySurvivorProjectionReady(state, mission);
				SetExactMissionConvoyAbandonedVehicleProjectionPublished(state, mission, readyToPublish);
				continue;
			}

			changed = RollbackUnpublishedExactMissionConvoyAbandonedVehicleProjection(state, mission) || changed;
		}
		return changed;
	}

	protected void SetExactMissionConvoyAbandonedVehicleProjectionPublished(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		bool published)
	{
		if (!state || !mission)
			return;
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!activeGroup || !IsExactMissionConvoyRecoveryVehicleEligible(asset, element))
				continue;
			IEntity vehicleEntity = GetExactMissionConvoyVehicleRuntimeEntity(state, mission, element.m_sVehicleSlotId);
			SetExactMissionConvoyProjectionEntityPublished(vehicleEntity, published);
		}
	}

	protected bool RollbackUnpublishedExactMissionConvoyAbandonedVehicleProjection(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool changed;
		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(BuildMissionConvoyGroupId(mission, index));
			HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!asset || !activeGroup || !IsExactMissionConvoyRecoveryVehicleEligible(asset, element))
				continue;
			IEntity vehicleEntity = GetExactMissionConvoyVehicleRuntimeEntity(state, mission, element.m_sVehicleSlotId);
			bool ownsProjection = vehicleEntity != null || GetRuntimeVehicleEntity(activeGroup.m_sGroupId) != null
				|| HasRuntimeVehicleRegistration(activeGroup.m_sGroupId) || asset.m_bSpawned || element.m_bPhysicalized;
			if (!ownsProjection)
				continue;

			SetExactMissionConvoyProjectionEntityPublished(vehicleEntity, false);
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			RemoveConvoyProgressStatusForGroup(activeGroup.m_sGroupId);
			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_bSpawnAttempted = false;
			activeGroup.m_sRuntimeEntityId = "";
			activeGroup.m_iSpawnedAgentCount = 0;
			activeGroup.m_iAssignedWaypointCount = 0;
			activeGroup.m_iSurvivorVehicleCount = 1;
			activeGroup.m_sConvoyRuntimeStage = "STRANDED_VIRTUAL";
			asset.m_bSpawned = false;
			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity)
			{
				runtimeEntity.m_bSpawned = false;
				runtimeEntity.m_vPosition = asset.m_vCurrentPosition;
			}
			element.m_bPhysicalized = false;
			element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			element.m_iRevision++;
			changed = true;
			Print(string.Format("Partisan exact mission convoy | rolled back unpublished abandoned vehicle %1 after materialization authority closed", asset.m_sAssetId));
		}
		if (changed)
			m_bMarkerRefreshNeeded = true;
		return changed;
	}

	protected void RemoveExactMissionConvoyOutboundProjectionTransaction(HST_ExactMissionConvoyOutboundProjectionTransaction transaction)
	{
		if (!transaction)
			return;
		for (int index = m_aExactMissionConvoyOutboundProjectionTransactions.Count() - 1; index >= 0; index--)
		{
			if (m_aExactMissionConvoyOutboundProjectionTransactions[index] == transaction)
				m_aExactMissionConvoyOutboundProjectionTransactions.Remove(index);
		}
	}

	protected bool ReconcileExactMissionConvoyOutboundProjectionTransactions(HST_CampaignState state)
	{
		if (!state)
			return false;
		bool changed;
		for (int index = m_aExactMissionConvoyOutboundProjectionTransactions.Count() - 1; index >= 0; index--)
		{
			HST_ExactMissionConvoyOutboundProjectionTransaction transaction = m_aExactMissionConvoyOutboundProjectionTransactions[index];
			if (!transaction || transaction.m_State != state)
			{
				m_aExactMissionConvoyOutboundProjectionTransactions.Remove(index);
				continue;
			}
			HST_ActiveMissionState mission = state.FindActiveMission(transaction.m_sMissionInstanceId);
			HST_OperationRecordState operation;
			if (mission)
				operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
			bool remainsMaterializing = mission && IsExactActiveMissionConvoy(mission) && !IsExactMissionConvoyRecoveryHold(mission)
				&& operation && operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
			bool becamePhysical = mission && IsExactActiveMissionConvoy(mission) && !IsExactMissionConvoyRecoveryHold(mission)
				&& operation && operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL;

			if (transaction.m_bRolledBackTerminally)
			{
				if (!remainsMaterializing)
					m_aExactMissionConvoyOutboundProjectionTransactions.Remove(index);
				continue;
			}
			SetExactMissionConvoyOutboundProjectionTransactionVisible(transaction, false);
			bool terminalRootFailure;
			string terminalReason;
			foreach (HST_ExactMissionConvoyRootProjectionSnapshot rootSnapshot : transaction.m_aRootSnapshots)
			{
				if (!rootSnapshot || !IsExactMissionConvoyTerminalRootSpawnFailure(rootSnapshot.m_Group))
					continue;
				terminalRootFailure = true;
				terminalReason = BuildExactMissionConvoyTerminalRootSpawnFailure(rootSnapshot.m_Group);
				break;
			}
			if (terminalRootFailure)
			{
				changed = RollbackExactMissionConvoyOutboundProjectionTransaction(state, transaction, terminalReason, true) || changed;
				continue;
			}
			// PHYSICAL is only the durable half of commit. MissionConvoyOperation owns
			// the participant seam and revalidates/publishes cargo plus every exact
			// root/member before this process-local transaction may close.
			if (becamePhysical)
				continue;
			if (!remainsMaterializing && !becamePhysical)
				changed = RollbackExactMissionConvoyOutboundProjectionTransaction(state, transaction, "exact outbound materialization ended before every required root became ready", false) || changed;
		}
		return changed;
	}

	protected bool EnsureMissionConvoyRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		if (IsExactMissionConvoyRecoveryHold(mission))
		{
			HST_OperationRecordState recoveryOperation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
			if (!recoveryOperation
				|| (recoveryOperation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
					&& recoveryOperation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL))
				return false;
			if (IsExactMissionConvoyRecoveryProjectionReady(state, mission))
				return false;

			string recoveryReason;
			bool recovered = MaterializeExactMissionConvoyRecoveryVehicles(state, mission, recoveryReason);
			if (!recovered && !recoveryReason.IsEmpty())
				DebugLog(string.Format("exact mission convoy recovery projection pending %1 | %2", mission.m_sInstanceId, recoveryReason));
			return recovered;
		}

		bool changed;
		int vehicleAssets;
		int crewedVehicles;
		bool exactContract = IsExactMissionConvoyContract(mission);
		HST_ExactMissionConvoyOutboundProjectionTransaction outboundTransaction;
		if (exactContract)
		{
			outboundTransaction = FindExactMissionConvoyOutboundProjectionTransaction(mission.m_sInstanceId);
			if (outboundTransaction && outboundTransaction.m_bRolledBackTerminally)
				return false;

			HST_OperationRecordState exactOperation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
			if (!outboundTransaction && exactOperation
				&& exactOperation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				&& !IsExactMissionConvoySurvivorProjectionReady(state, mission))
			{
				outboundTransaction = BeginExactMissionConvoyOutboundProjectionTransaction(state, mission);
				if (!outboundTransaction)
					return false;
			}
		}
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				continue;

			int vehicleIndex = vehicleAssets;
			if (exactContract)
			{
				vehicleIndex = ResolveExactMissionConvoyVehicleAssetIndex(mission, asset);
				if (vehicleIndex < 0)
					continue;
			}
			vehicleAssets++;

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

			if (exactContract)
			{
				HST_ConvoyElementState exactElement = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
				if (IsExactMissionConvoyTerminalSurvivingCrew(asset, exactElement))
				{
					if (activeGroup && activeGroup.m_sRuntimeStatus != "spawn_failed" && ShouldSpawnMissionConvoyRuntime(state, activeGroup))
						changed = TrySpawnMissionConvoyGroup(state, preset, mission, asset, activeGroup, vehicleIndex) || changed;
					if (outboundTransaction)
						SetExactMissionConvoyOutboundProjectionTransactionVisible(outboundTransaction, false);
					if (outboundTransaction && IsExactMissionConvoyTerminalRootSpawnFailure(activeGroup))
					{
						string terminalFailure = BuildExactMissionConvoyTerminalRootSpawnFailure(activeGroup);
						RollbackExactMissionConvoyOutboundProjectionTransaction(state, outboundTransaction, terminalFailure, true);
						return true;
					}
					if (activeGroup && activeGroup.m_bSpawnedEntity && CountAliveRuntimeCrewAgents(activeGroup) > 0)
						crewedVehicles++;
					continue;
				}
			}
			if (IsMissionConvoyVehicleAssetResolved(asset))
				continue;

			if (activeGroup && activeGroup.m_sRuntimeStatus != "spawn_failed" && ShouldSpawnMissionConvoyRuntime(state, activeGroup))
				changed = TrySpawnMissionConvoyGroup(state, preset, mission, asset, activeGroup, vehicleIndex) || changed;
			if (outboundTransaction)
				SetExactMissionConvoyOutboundProjectionTransactionVisible(outboundTransaction, false);
			if (outboundTransaction && IsExactMissionConvoyTerminalRootSpawnFailure(activeGroup))
			{
				string terminalFailure = BuildExactMissionConvoyTerminalRootSpawnFailure(activeGroup);
				RollbackExactMissionConvoyOutboundProjectionTransaction(state, outboundTransaction, terminalFailure, true);
				return true;
			}

			if (activeGroup)
				changed = SyncMissionConvoyVehicleAssetRuntimeState(state, mission, asset, activeGroup, groupId) || changed;

			if (activeGroup && activeGroup.m_bSpawnedEntity && activeGroup.m_sRuntimeStatus != "spawn_failed" && ResolveMissionConvoyRestorableCrewCount(state, activeGroup) > 0)
				crewedVehicles++;
		}

		if (exactContract && outboundTransaction)
		{
			if (!HasCompleteExactMissionConvoyOutboundAttemptHandles(state, mission))
			{
				RollbackExactMissionConvoyOutboundProjectionTransaction(state, outboundTransaction, "exact outbound materialization did not create every required process root in one bounded attempt", false);
				return true;
			}
			// The operation layer commits the transaction with MissionRuntime cargo as
			// a required participant after durable PHYSICAL authority is established.
		}

		if (!exactContract && mission.m_sRuntimePhase != MISSION_CONVOY_CONTACT && vehicleAssets > 0 && vehicleAssets < 3)
		{
			SetMissionConvoyFailure(state, mission, "Convoy needs at least three valid vehicle assets.");
			changed = true;
		}
		else if (!exactContract && mission.m_sRuntimePhase != MISSION_CONVOY_CONTACT && vehicleAssets >= 3 && crewedVehicles < 3 && AllMissionConvoyGroupsAttempted(state, mission, vehicleAssets) && !HasPendingConvoyCrewControl(state, mission, vehicleAssets))
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
				if (travelRecovery && !IsMissionConvoyWaypointAssigned(activeGroup))
					TryAssignCurrentMissionConvoyRoute(state, mission, activeGroup);
			}
			else
			{
				if (!preserveWaypointMode)
				{
					if (seatingReason.Contains("seating pending yes") || seatingReason.Contains("waiting for authoritative seat transition"))
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
		activeGroup.m_sOperationId = HST_StableIdService.BuildOperationId("mission", mission.m_sInstanceId);
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
		if (IsExactMissionConvoyContract(mission))
			BindExactMissionConvoyGroupIdentity(state, activeGroup, mission, asset);
		return activeGroup;
	}

	protected bool TrySpawnMissionConvoyGroup(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, int index)
	{
		if (!state || !mission || !asset || !activeGroup)
			return false;
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
		bool exactContract = IsExactMissionConvoyContract(mission);
		if (exactContract)
		{
			HST_ConvoyElementState exactElement = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (IsExactMissionConvoyTerminalSurvivingCrew(asset, exactElement))
				return TrySpawnExactMissionConvoyTerminalSurvivingCrew(state, preset, mission, asset, activeGroup, exactElement);
			if (IsExactMissionConvoyRecoveryVehicleEligible(asset, exactElement))
				return TrySpawnExactMissionConvoyAbandonedVehicle(state, mission, asset, activeGroup, exactElement);
		}
		if (IsMissionConvoyVehicleAssetResolved(asset))
			return false;
		string vehiclePrefab = asset.m_sPrefab;
		if (exactContract)
		{
			HST_OperationRecordState exactOperation = ResolveOpenExactMissionConvoyOperation(state, mission);
			HST_ConvoyElementState exactElement = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (!exactOperation || !exactElement || IsExactMissionConvoyElementTerminal(exactElement) || exactElement.m_iSurvivingCrewCount <= 0)
				return false;
			if (exactOperation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				&& exactOperation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
				return false;
			if (!TryResolveExactMissionConvoyFrozenVehiclePrefab(state, mission, asset, activeGroup, vehiclePrefab))
			{
				activeGroup.m_bSpawnAttempted = true;
				activeGroup.m_sRuntimeStatus = "spawn_failed";
				activeGroup.m_sSpawnFailureReason = "Exact convoy frozen vehicle manifest is missing, ambiguous, or invalid.";
				activeGroup.m_bCrewPopulationTerminallyFailed = true;
				activeGroup.m_sCrewPopulationFailureReason = activeGroup.m_sSpawnFailureReason;
				activeGroup.m_sConvoyRuntimeStage = "FAILED";
				mission.m_sRuntimeFailureReason = activeGroup.m_sSpawnFailureReason;
				Print(string.Format("Partisan mission convoy | frozen vehicle manifest rejected for %1 group %2 asset %3", mission.m_sInstanceId, activeGroup.m_sGroupId, asset.m_sAssetId), LogLevel.WARNING);
				return true;
			}
		}
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
			Print(string.Format("Partisan mission convoy | spawn failed %1 asset %2 at %3: %4", mission.m_sInstanceId, asset.m_sAssetId, asset.m_vSourcePosition, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
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

		if (!exactContract && vehiclePrefab.IsEmpty())
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
			Print(string.Format("Partisan mission convoy | vehicle spawn failed %1 group %2 prefab %3", mission.m_sInstanceId, activeGroup.m_sGroupId, vehiclePrefab), LogLevel.WARNING);
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
				if (adapterBindReason.Contains("seating pending yes") || adapterBindReason.Contains("waiting for authoritative seat transition"))
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
		if (exactContract)
		{
			HST_ConvoyElementState exactElement = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			if (exactElement)
			{
				exactElement.m_vCurrentPosition = spawnPosition;
				exactElement.m_bPhysicalized = true;
				exactElement.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
				exactElement.m_iRevision++;
			}
		}
		if (IsMissionConvoyTravelPhase(mission) && !IsMissionConvoyWaypointAssigned(activeGroup))
			TryAssignCurrentMissionConvoyRoute(state, mission, activeGroup);
		Print(string.Format("Partisan mission convoy | spawned vehicle %1 and crew group %2 for %3 at %4", vehiclePrefab, activeGroup.m_sGroupId, mission.m_sInstanceId, spawnPosition));
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected bool TrySpawnExactMissionConvoyAbandonedVehicle(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		HST_ActiveGroupState activeGroup,
		HST_ConvoyElementState element)
	{
		if (!state || !mission || !asset || !activeGroup || !IsExactMissionConvoyRecoveryVehicleEligible(asset, element))
			return false;
		string reason;
		bool materialized = MaterializeExactMissionConvoyRecoveryVehicles(state, mission, reason);
		if (!materialized && !reason.IsEmpty())
		{
			activeGroup.m_sSpawnFailureReason = reason;
			activeGroup.m_sConvoyRuntimeStage = "ABANDONED_VEHICLE_PENDING";
		}
		return materialized;
	}

	protected bool TrySpawnExactMissionConvoyTerminalSurvivingCrew(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		HST_ActiveGroupState activeGroup,
		HST_ConvoyElementState element)
	{
		if (!state || !mission || !asset || !activeGroup || !IsExactMissionConvoyTerminalSurvivingCrew(asset, element))
			return false;
		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		if (!operation || (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
			&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL))
			return false;

		string authorityFailure = ValidateExactMissionConvoyCrewProjectionAuthority(state, mission, asset, activeGroup, element);
		if (!authorityFailure.IsEmpty())
		{
			activeGroup.m_sSpawnFailureReason = "Exact terminal convoy crew projection rejected: " + authorityFailure;
			activeGroup.m_sConvoyRuntimeStage = "TERMINAL_CREW_AUTHORITY_REJECTED";
			return true;
		}
		if (GetRuntimeVehicleEntity(activeGroup.m_sGroupId) || HasRuntimeVehicleRegistration(activeGroup.m_sGroupId))
		{
			activeGroup.m_sSpawnFailureReason = "Exact terminal convoy crew projection is waiting for its resolved vehicle runtime to retire.";
			activeGroup.m_sConvoyRuntimeStage = "TERMINAL_VEHICLE_RETIRE_PENDING";
			return true;
		}

		IEntity existingCrew = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		if (existingCrew)
		{
			int existingLiving = CountAliveRuntimeCrewAgents(activeGroup);
			if (existingLiving == element.m_iSurvivingCrewCount
				&& ExactMissionConvoyRuntimeCrewMatchesFrozenSurvivorSlots(state, mission, activeGroup, element))
			{
				activeGroup.m_bSpawnedEntity = true;
				activeGroup.m_iSpawnedAgentCount = existingLiving;
				activeGroup.m_iLastSeenAliveCount = existingLiving;
				activeGroup.m_iSurvivorInfantryCount = existingLiving;
				activeGroup.m_iDurableLivingInfantryCount = existingLiving;
				activeGroup.m_iSurvivorVehicleCount = 0;
				activeGroup.m_sRuntimeStatus = ResolveMissionConvoyRuntimeStatus(mission);
				activeGroup.m_sConvoyRuntimeStage = "DISMOUNTED_SURVIVORS";
				element.m_bPhysicalized = true;
				return false;
			}
			if (activeGroup.m_sRuntimeStatus == "spawn_pending_agents" || IsActiveGroupNativeDelayedPopulationActive(activeGroup))
				return false;
			activeGroup.m_sSpawnFailureReason = string.Format("Exact terminal convoy crew projection count is %1 but durable authority requires %2; survivor resurrection is forbidden.", existingLiving, element.m_iSurvivingCrewCount);
			activeGroup.m_sConvoyRuntimeStage = "TERMINAL_CREW_COUNT_REJECTED";
			return true;
		}

		vector durablePosition = element.m_vCurrentPosition;
		if (IsZeroVector(durablePosition))
			durablePosition = asset.m_vCurrentPosition;
		if (IsZeroVector(durablePosition))
			durablePosition = asset.m_vLastKnownPosition;
		if (IsZeroVector(durablePosition) || HST_WorldPositionService.IsLikelyOpenWater(durablePosition))
		{
			activeGroup.m_sSpawnFailureReason = "Exact terminal convoy surviving crew has no dry durable element position.";
			activeGroup.m_sConvoyRuntimeStage = "TERMINAL_CREW_POSITION_REJECTED";
			return true;
		}
		durablePosition = HST_WorldPositionService.ResolveSafeGroundPosition(durablePosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);

		if (state.m_bRestoredFromPersistence)
			MarkRestoredMissionConvoyRuntimeRebuildAttempted(activeGroup.m_sGroupId);
		activeGroup.m_sPrefab = element.m_sCrewGroupPrefab;
		activeGroup.m_vPosition = durablePosition;
		activeGroup.m_vSourcePosition = durablePosition;
		activeGroup.m_iInfantryCount = element.m_iSurvivingCrewCount;
		activeGroup.m_iSurvivorInfantryCount = element.m_iSurvivingCrewCount;
		activeGroup.m_iLastSeenAliveCount = element.m_iSurvivingCrewCount;
		activeGroup.m_iDurableLivingInfantryCount = element.m_iSurvivingCrewCount;
		activeGroup.m_iSurvivorVehicleCount = 0;
		activeGroup.m_iAssignedWaypointCount = 0;
		activeGroup.m_sRuntimeStatus = ResolveMissionConvoyRuntimeStatus(mission);
		activeGroup.m_sSpawnFallbackMode = "convoy_terminal_crew_only";
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_sConvoyRuntimeStage = "DISMOUNTED_MATERIALIZING";
		asset.m_bSpawned = false;
		element.m_bPhysicalized = false;
		element.m_bMobile = false;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_bSpawned = false;

		if (!TrySpawnActiveGroup(activeGroup, state, preset))
			return true;
		if (GetRuntimeVehicleEntity(activeGroup.m_sGroupId) || HasRuntimeVehicleRegistration(activeGroup.m_sGroupId))
		{
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_sRuntimeEntityId = "";
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = "Exact terminal convoy crew-only materialization unexpectedly created a vehicle runtime.";
			activeGroup.m_sConvoyRuntimeStage = "TERMINAL_VEHICLE_RESURRECTION_REJECTED";
			return true;
		}

		int liveCrew = CountAliveRuntimeCrewAgents(activeGroup);
		if (activeGroup.m_sRuntimeStatus == "spawn_pending_agents" && liveCrew <= 0)
		{
			activeGroup.m_sConvoyRuntimeStage = "DISMOUNTED_POPULATION_PENDING";
			return true;
		}
		if (liveCrew != element.m_iSurvivingCrewCount
			|| !ExactMissionConvoyRuntimeCrewMatchesFrozenSurvivorSlots(state, mission, activeGroup, element))
		{
			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_sRuntimeEntityId = "";
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = string.Format("Exact terminal convoy crew-only materialization produced %1 living members but frozen authority requires %2.", liveCrew, element.m_iSurvivingCrewCount);
			activeGroup.m_sConvoyRuntimeStage = "TERMINAL_CREW_ROSTER_REJECTED";
			return true;
		}

		activeGroup.m_iSpawnedAgentCount = liveCrew;
		activeGroup.m_iLastSeenAliveCount = liveCrew;
		activeGroup.m_iSurvivorInfantryCount = liveCrew;
		activeGroup.m_iDurableLivingInfantryCount = liveCrew;
		activeGroup.m_iSurvivorVehicleCount = 0;
		activeGroup.m_sRuntimeStatus = ResolveMissionConvoyRuntimeStatus(mission);
		activeGroup.m_sConvoyRuntimeStage = "DISMOUNTED_SURVIVORS";
		element.m_vCurrentPosition = durablePosition;
		element.m_bPhysicalized = true;
		element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
		element.m_iRevision++;
		m_bMarkerRefreshNeeded = true;
		Print(string.Format("Partisan exact mission convoy | materialized terminal-vehicle surviving crew %1 count %2 at %3 without vehicle", activeGroup.m_sGroupId, liveCrew, durablePosition));
		return true;
	}

	protected GenericEntity SpawnMissionConvoyVehicle(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, string vehiclePrefab, vector spawnPosition, int index)
	{
		bool exactContract = IsExactMissionConvoyContract(mission);
		if (exactContract)
		{
			string frozenPrefab;
			if (!TryResolveExactMissionConvoyFrozenVehiclePrefab(state, mission, asset, activeGroup, frozenPrefab) || frozenPrefab != vehiclePrefab)
				return null;
			vehiclePrefab = frozenPrefab;
		}
		else
		{
			string selectedPrefab = SelectMissionConvoyVehiclePrefab(state, ResolveMissionConvoyTargetZone(state, mission, asset), activeGroup.m_sFactionKey, mission, index + 13, preset);
			if (!selectedPrefab.IsEmpty())
				vehiclePrefab = selectedPrefab;
			else if (vehiclePrefab.IsEmpty() || !IsValidVehiclePrefabResource(vehiclePrefab, activeGroup.m_sFactionKey))
				vehiclePrefab = SelectMissionConvoyVehiclePrefab(state, ResolveMissionConvoyTargetZone(state, mission, asset), activeGroup.m_sFactionKey, mission, index + 29, preset);
		}
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
		if (!exactContract)
			asset.m_sPrefab = vehiclePrefab;
		else
			ApplyExactMissionConvoyVehicleRuntimeState(state, mission, asset, activeGroup, vehicleEntity);
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
		if (exactContract && FindExactMissionConvoyOutboundProjectionTransaction(mission.m_sInstanceId))
			SetExactMissionConvoyProjectionEntityPublished(vehicleEntity, false);
		return vehicleEntity;
	}

	protected bool TryResolveMissionConvoyVehicleSpawnPosition(HST_ActiveMissionState mission, HST_MissionAssetState asset, out vector spawnPosition)
	{
		spawnPosition = "0 0 0";
		if (!mission || !asset)
			return false;
		if (IsExactMissionConvoyContract(mission) && !IsZeroVector(asset.m_vCurrentPosition))
		{
			if (TryResolveMissionConvoyVehicleSpawnPositionPassAtAnchor(mission, asset, asset.m_vCurrentPosition, spawnPosition, false))
				return true;
			if (TryResolveMissionConvoyVehicleSpawnPositionPassAtAnchor(mission, asset, asset.m_vCurrentPosition, spawnPosition, true))
				return true;
		}

		if (TryResolveMissionConvoyVehicleSpawnPositionPass(mission, asset, spawnPosition, false))
			return true;

		return TryResolveMissionConvoyVehicleSpawnPositionPass(mission, asset, spawnPosition, true);
	}

	protected bool TryResolveMissionConvoyVehicleSpawnPositionPass(HST_ActiveMissionState mission, HST_MissionAssetState asset, out vector spawnPosition, bool preferHeavyVehicleTerrain)
	{
		spawnPosition = "0 0 0";
		if (!mission || !asset)
			return false;

		return TryResolveMissionConvoyVehicleSpawnPositionPassAtAnchor(mission, asset, asset.m_vSourcePosition, spawnPosition, preferHeavyVehicleTerrain);
	}

	protected bool TryResolveMissionConvoyVehicleSpawnPositionPassAtAnchor(HST_ActiveMissionState mission, HST_MissionAssetState asset, vector anchorPosition, out vector spawnPosition, bool preferHeavyVehicleTerrain)
	{
		spawnPosition = "0 0 0";
		if (!mission || !asset || IsZeroVector(anchorPosition))
			return false;

		vector resolved;
		if (TryResolveMissionConvoyVehicleSpawnCandidate(anchorPosition, asset.m_vTargetPosition, resolved, preferHeavyVehicleTerrain) && IsMissionConvoyVehicleSpawnClear(mission, resolved))
		{
			spawnPosition = LiftMissionConvoyVehicleSpawnPosition(resolved);
			return true;
		}

		for (int ring = 1; ring <= 5; ring++)
		{
			float radius = CONVOY_VEHICLE_SPAWN_CLEARANCE_METERS * ring;
			for (int step = 0; step < 8; step++)
			{
				vector candidate = BuildConvoySpawnClearanceCandidate(anchorPosition, step, radius);
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
		if (progress.m_bHardStuck && noProgressSeconds >= CONVOY_TERMINAL_STUCK_THRESHOLD_SECONDS)
		{
			progress.m_sLastProgressReason = string.Format("terminal route stall after %1s without progress", noProgressSeconds);
			SetMissionConvoyFailure(state, mission, string.Format("Convoy route watchdog exhausted recovery for %1 after %2 seconds without progress. Route reissue attempts %3; route snap attempts %4; last recovery: %5", ReportText(asset.m_sAssetId), noProgressSeconds, progress.m_iRouteReissueAttemptCount, progress.m_iRouteSnapAttemptCount, ReportText(progress.m_sLastRecoveryResult)));
			return true;
		}

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
			if (seatingReason.Contains("seating pending yes") || seatingReason.Contains("waiting for authoritative seat transition"))
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
		bool seatingPending = seatingReason.Contains("seating pending yes") || seatingReason.Contains("waiting for authoritative seat transition") || mountedReason.Contains("seating pending yes") || mountedReason.Contains("waiting for authoritative seat transition");
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
		if (HasMissionConvoyControlPending(state, mission))
			return false;

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

			return false;
		}

		return true;
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
		Print(string.Format("Partisan mission convoy | %1 staged as static ambush: %2", mission.m_sInstanceId, mission.m_sRuntimeFailureReason), LogLevel.WARNING);
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
			Print(string.Format("Partisan mission convoy | %1 generic completion: %2/%3 crews eliminated", mission.m_sInstanceId, status.m_iEliminatedCrewGroups, status.m_iRequiredCrewGroups));
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
			if (IsExactMissionConvoyContract(mission))
				return changed;
			SetMissionConvoyFailure(state, mission, "Convoy reached its destination with living crew.");
			return true;
		}

		if (completion.m_bCanComplete && !IsExactMissionConvoyContract(mission))
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
		if (!IsMissionConvoyGroup(activeGroup) || !mission
			|| activeGroup.m_sMissionInstanceId != mission.m_sInstanceId)
			return false;
		if (IsExactMissionConvoyContract(mission))
		{
			if (activeGroup.m_sOperationId != mission.m_sOperationId
				|| activeGroup.m_sManifestId != mission.m_sManifestId
				|| activeGroup.m_sSpawnResultId != mission.m_sSpawnResultId)
				return false;
		}

		return IsMissionConvoyGroupIdForMission(activeGroup.m_sGroupId, mission);
	}

	protected bool IsMissionConvoyGroupIdForMission(string groupId, HST_ActiveMissionState mission)
	{
		if (groupId.IsEmpty() || !mission || mission.m_sInstanceId.IsEmpty())
			return false;
		if (IsExactMissionConvoyContract(mission))
		{
			for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
			{
				if (groupId == BuildMissionConvoyGroupId(mission, index))
					return true;
			}
			return false;
		}

		string prefix = string.Format("%1%2_", MISSION_CONVOY_GROUP_PREFIX, mission.m_sInstanceId);
		return groupId.StartsWith(prefix);
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

		HST_ActiveMissionState exactMission;
		HST_MissionAssetState exactAsset;
		HST_ConvoyElementState exactElement;
		if (state && !activeGroup.m_sMissionInstanceId.IsEmpty())
			exactMission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
		if (IsExactMissionConvoyContract(exactMission))
		{
			exactAsset = FindMissionConvoyAssetForGroup(state, exactMission, activeGroup);
			exactElement = ResolveExactMissionConvoyElement(state, exactMission, exactAsset, activeGroup);
		}
		bool separatedAbandonedVehicle = IsExactMissionConvoyRecoveryVehicleEligible(exactAsset, exactElement);
		if (state && !state.IsOperationalActiveGroup(activeGroup) && !separatedAbandonedVehicle)
			return false;

		if (IsExactMissionConvoyContract(exactMission))
		{
			HST_OperationRecordState operation = ResolveOpenExactMissionConvoyOperation(state, exactMission);
			if (!operation)
				return false;
			if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
				&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
				return false;

			if (!exactAsset || !exactElement)
				return false;
			if (separatedAbandonedVehicle)
			{
				if (activeGroup.m_sRuntimeStatus == "spawn_failed")
					return false;
				if (GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId))
					return true;
				IEntity separatedVehicle = GetExactMissionConvoyVehicleRuntimeEntity(state, exactMission, exactElement.m_sVehicleSlotId);
				if (separatedVehicle)
					return !exactAsset.m_bSpawned || !exactElement.m_bPhysicalized;
				if (GetRuntimeVehicleEntity(activeGroup.m_sGroupId) || HasRuntimeVehicleRegistration(activeGroup.m_sGroupId))
					return true;
				return !activeGroup.m_bSpawnAttempted
					|| (state.m_bRestoredFromPersistence && !WasRestoredMissionConvoyRuntimeRebuildAttempted(activeGroup.m_sGroupId));
			}
			if (exactElement.m_iSurvivingCrewCount <= 0)
				return false;
			if (IsExactMissionConvoyTerminalSurvivingCrew(exactAsset, exactElement))
			{
				if (GetRuntimeVehicleEntity(activeGroup.m_sGroupId) || HasRuntimeVehicleRegistration(activeGroup.m_sGroupId))
					return false;
				if (activeGroup.m_sRuntimeStatus == MISSION_CONVOY_ELIMINATED || activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed")
					return false;
				IEntity terminalCrew = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
				if (!terminalCrew)
					return !activeGroup.m_bSpawnAttempted || (state.m_bRestoredFromPersistence && !WasRestoredMissionConvoyRuntimeRebuildAttempted(activeGroup.m_sGroupId));
				return false;
			}
			if (IsMissionConvoyVehicleAssetResolved(exactAsset) || IsExactMissionConvoyElementTerminal(exactElement))
				return false;
		}
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

	protected bool IsExactMissionConvoyContract(HST_ActiveMissionState mission)
	{
		return mission
			&& mission.m_sRuntimePrimitive == MISSION_CONVOY_PRIMITIVE
			&& mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION;
	}

	protected bool IsExactActiveMissionConvoy(HST_ActiveMissionState mission)
	{
		return IsExactMissionConvoyContract(mission)
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& !IsTerminalMissionConvoyPhase(mission);
	}

	protected bool IsExactMissionConvoyRecoveryHold(HST_ActiveMissionState mission)
	{
		return IsExactMissionConvoyContract(mission)
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& mission.m_sRuntimePhase == MISSION_CONVOY_ELIMINATED;
	}

	protected bool IsExactMissionConvoyTerminalSurvivingCrew(HST_MissionAssetState asset, HST_ConvoyElementState element)
	{
		if (!asset || !element || !IsMissionConvoyVehicleAssetResolved(asset)
			|| element.m_iSurvivingCrewCount <= 0 || element.m_bMobile)
			return false;

		return element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED
			|| element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED;
	}

	protected string ValidateExactMissionConvoyCrewProjectionAuthority(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		HST_ActiveGroupState activeGroup,
		HST_ConvoyElementState element)
	{
		if (!state || !mission || !asset || !activeGroup || !element)
			return "crew projection authority root is missing";
		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		if (!operation || operation.m_sManifestId.IsEmpty() || operation.m_sManifestId != mission.m_sManifestId)
			return "crew projection operation or manifest identity is missing";
		HST_ForceManifestState manifest = state.FindForceManifest(operation.m_sManifestId);
		if (!manifest || !manifest.m_bFrozen || manifest.m_sOperationId != operation.m_sOperationId
			|| manifest.m_sManifestId != element.m_sManifestId)
			return "crew projection frozen manifest is missing or conflicting";

		HST_ForceManifestGroupState groupSlot;
		int groupClaimants;
		foreach (HST_ForceManifestGroupState candidateGroup : manifest.m_aGroups)
		{
			if (!candidateGroup || candidateGroup.m_sElementId != element.m_sCrewGroupElementId)
				continue;
			groupSlot = candidateGroup;
			groupClaimants++;
		}
		if (groupClaimants != 1 || !groupSlot || !groupSlot.m_bRequired
			|| groupSlot.m_sPrefab.IsEmpty() || groupSlot.m_sPrefab != element.m_sCrewGroupPrefab
			|| groupSlot.m_sPrefab != activeGroup.m_sPrefab
			|| groupSlot.m_iExpectedMemberCount != element.m_iOriginalCrewCount
			|| element.m_iSurvivingCrewCount > element.m_iOriginalCrewCount)
			return "crew projection frozen group slot is missing or conflicting";

		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		if (!batch || batch.m_sOperationId != operation.m_sOperationId || batch.m_sManifestId != manifest.m_sManifestId
			|| activeGroup.m_sSpawnResultId != batch.m_sResultId)
			return "crew projection held roster batch is missing or conflicting";
		HST_ForceSpawnSlotResultState groupResult = batch.FindSlotResult(groupSlot.m_sElementId);
		if (!groupResult || groupResult.m_sSlotKind != "group" || groupResult.m_sSpawnedPrefab != groupSlot.m_sPrefab)
			return "crew projection held group roster slot is missing or conflicting";

		int memberCount;
		int livingMemberCount;
		for (int seatIndex = 0; seatIndex < element.m_iOriginalCrewCount; seatIndex++)
		{
			HST_ForceManifestMemberState member = ResolveExactMissionConvoyManifestMemberForSeat(manifest, element, seatIndex);
			if (!member || !member.m_bRequired || member.m_sPrefab.IsEmpty()
				|| member.m_sAssignedVehicleSlotId != element.m_sVehicleSlotId)
				return string.Format("crew projection frozen member seat %1 is missing or conflicting", seatIndex);
			int resultClaimants;
			foreach (HST_ForceSpawnSlotResultState candidateResult : batch.m_aSlotResults)
			{
				if (candidateResult && candidateResult.m_sSlotId == member.m_sSlotId)
					resultClaimants++;
			}
			HST_ForceSpawnSlotResultState memberResult = batch.FindSlotResult(member.m_sSlotId);
			if (resultClaimants != 1 || !memberResult || memberResult.m_sSlotKind != "member"
				|| memberResult.m_sSpawnedPrefab != member.m_sPrefab || !memberResult.m_bEverAlive)
				return string.Format("crew projection held member seat %1 is missing or conflicting", seatIndex);
			bool memberLiving = !memberResult.m_bCasualtyConfirmed;
			if (memberLiving)
				livingMemberCount++;
			memberCount++;
		}
		if (memberCount != element.m_iOriginalCrewCount || livingMemberCount != element.m_iSurvivingCrewCount)
			return "crew projection member-slot survivor count conflicts with its element";
		return "";
	}

	protected HST_ForceManifestMemberState ResolveExactMissionConvoyManifestMemberForSeat(
		HST_ForceManifestState manifest,
		HST_ConvoyElementState element,
		int seatIndex)
	{
		if (!manifest || !element || seatIndex < 0)
			return null;
		HST_ForceManifestMemberState result;
		int claimants;
		foreach (HST_ForceManifestMemberState candidate : manifest.m_aMembers)
		{
			if (!candidate || candidate.m_sGroupElementId != element.m_sCrewGroupElementId
				|| candidate.m_iSeatIndex != seatIndex)
				continue;
			result = candidate;
			claimants++;
		}
		if (claimants != 1)
			return null;
		return result;
	}

	protected bool HasConsistentExactMissionConvoyMemberIdentityArrays()
	{
		int count = m_aExactMissionConvoyMemberMissionIds.Count();
		return m_aExactMissionConvoyMemberGroupIds.Count() == count
			&& m_aExactMissionConvoyMemberSlotIds.Count() == count
			&& m_aExactMissionConvoyMemberEntities.Count() == count;
	}

	protected int CountExactMissionConvoyMemberMappings(string missionInstanceId = "", string groupId = "")
	{
		if (!HasConsistentExactMissionConvoyMemberIdentityArrays())
			return -1;

		int count;
		for (int index = 0; index < m_aExactMissionConvoyMemberEntities.Count(); index++)
		{
			if (!missionInstanceId.IsEmpty() && m_aExactMissionConvoyMemberMissionIds[index] != missionInstanceId)
				continue;
			if (!groupId.IsEmpty() && m_aExactMissionConvoyMemberGroupIds[index] != groupId)
				continue;
			count++;
		}
		return count;
	}

	protected bool TryGetExactMissionConvoyMappedMemberEntity(
		string missionInstanceId,
		string groupId,
		string memberSlotId,
		out IEntity entity)
	{
		entity = null;
		if (missionInstanceId.IsEmpty() || groupId.IsEmpty() || memberSlotId.IsEmpty()
			|| !HasConsistentExactMissionConvoyMemberIdentityArrays())
			return false;

		int claimants;
		for (int index = 0; index < m_aExactMissionConvoyMemberEntities.Count(); index++)
		{
			if (m_aExactMissionConvoyMemberMissionIds[index] != missionInstanceId
				|| m_aExactMissionConvoyMemberGroupIds[index] != groupId
				|| m_aExactMissionConvoyMemberSlotIds[index] != memberSlotId)
				continue;
			entity = m_aExactMissionConvoyMemberEntities[index];
			claimants++;
		}
		return claimants == 1 && entity != null;
	}

	protected bool IsRuntimeEntityRegisteredExactlyOnceForGroup(string groupId, IEntity entity)
	{
		if (groupId.IsEmpty() || !entity)
			return false;
		int claimants;
		for (int index = 0; index < m_aRuntimeGroupEntities.Count(); index++)
		{
			if (index < m_aRuntimeGroupIds.Count() && m_aRuntimeGroupIds[index] == groupId
				&& m_aRuntimeGroupEntities[index] == entity)
				claimants++;
		}
		return claimants == 1;
	}

	protected bool RegisterExactMissionConvoyMemberEntity(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ActiveGroupState activeGroup,
		HST_ForceManifestMemberState member,
		IEntity entity)
	{
		if (!state || !IsExactMissionConvoyContract(mission) || !activeGroup || !member || !entity || entity.IsDeleted()
			|| !HasConsistentExactMissionConvoyMemberIdentityArrays())
			return false;
		if (activeGroup.m_sMissionInstanceId != mission.m_sInstanceId || activeGroup.m_sManifestId != mission.m_sManifestId
			|| member.m_sSlotId.IsEmpty() || member.m_sPrefab.IsEmpty() || ResolveEntityPrefabName(entity) != member.m_sPrefab)
			return false;

		HST_MissionAssetState asset = FindMissionConvoyAssetForGroup(state, mission, activeGroup);
		HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
		if (!element || member.m_sGroupElementId != element.m_sCrewGroupElementId
			|| member.m_sAssignedVehicleSlotId != element.m_sVehicleSlotId)
			return false;
		SCR_AIGroup root = SCR_AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId));
		string membershipFailure;
		if (!root || !ValidateForceSpawnGroupMember(activeGroup, root, entity, member.m_iSeatIndex, membershipFailure))
			return false;

		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		HST_ForceSpawnSlotResultState slotResult;
		if (batch)
			slotResult = batch.FindSlotResult(member.m_sSlotId);
		if (!batch || batch.m_sManifestId != mission.m_sManifestId || batch.m_sOperationId != mission.m_sOperationId
			|| !slotResult || slotResult.m_sSlotKind != "member" || slotResult.m_sSpawnedPrefab != member.m_sPrefab
			|| slotResult.m_bCasualtyConfirmed || slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
			|| !slotResult.m_bAliveVerified || !slotResult.m_bEverAlive)
			return false;

		for (int index = 0; index < m_aExactMissionConvoyMemberEntities.Count(); index++)
		{
			bool sameSlot = m_aExactMissionConvoyMemberMissionIds[index] == mission.m_sInstanceId
				&& m_aExactMissionConvoyMemberGroupIds[index] == activeGroup.m_sGroupId
				&& m_aExactMissionConvoyMemberSlotIds[index] == member.m_sSlotId;
			bool sameEntity = m_aExactMissionConvoyMemberEntities[index] == entity;
			if (sameSlot || sameEntity)
				return sameSlot && sameEntity;
		}

		m_aExactMissionConvoyMemberMissionIds.Insert(mission.m_sInstanceId);
		m_aExactMissionConvoyMemberGroupIds.Insert(activeGroup.m_sGroupId);
		m_aExactMissionConvoyMemberSlotIds.Insert(member.m_sSlotId);
		m_aExactMissionConvoyMemberEntities.Insert(entity);
		return true;
	}

	protected void ClearExactMissionConvoyMemberMappingsForGroup(string groupId)
	{
		if (groupId.IsEmpty())
			return;
		for (int index = m_aExactMissionConvoyMemberEntities.Count() - 1; index >= 0; index--)
		{
			if (index >= m_aExactMissionConvoyMemberGroupIds.Count() || m_aExactMissionConvoyMemberGroupIds[index] != groupId)
				continue;
			m_aExactMissionConvoyMemberEntities.Remove(index);
			m_aExactMissionConvoyMemberSlotIds.Remove(index);
			m_aExactMissionConvoyMemberGroupIds.Remove(index);
			m_aExactMissionConvoyMemberMissionIds.Remove(index);
		}
	}

	protected void ClearExactMissionConvoyMemberMappingsForMission(string missionInstanceId)
	{
		if (missionInstanceId.IsEmpty())
			return;
		for (int index = m_aExactMissionConvoyMemberEntities.Count() - 1; index >= 0; index--)
		{
			if (index >= m_aExactMissionConvoyMemberMissionIds.Count() || m_aExactMissionConvoyMemberMissionIds[index] != missionInstanceId)
				continue;
			m_aExactMissionConvoyMemberEntities.Remove(index);
			m_aExactMissionConvoyMemberSlotIds.Remove(index);
			m_aExactMissionConvoyMemberGroupIds.Remove(index);
			m_aExactMissionConvoyMemberMissionIds.Remove(index);
		}
	}

	protected void RemoveExactMissionConvoyMemberMapping(string missionInstanceId, string memberSlotId)
	{
		if (missionInstanceId.IsEmpty() || memberSlotId.IsEmpty())
			return;
		for (int index = m_aExactMissionConvoyMemberEntities.Count() - 1; index >= 0; index--)
		{
			if (index >= m_aExactMissionConvoyMemberMissionIds.Count()
				|| index >= m_aExactMissionConvoyMemberSlotIds.Count()
				|| m_aExactMissionConvoyMemberMissionIds[index] != missionInstanceId
				|| m_aExactMissionConvoyMemberSlotIds[index] != memberSlotId)
				continue;
			m_aExactMissionConvoyMemberEntities.Remove(index);
			m_aExactMissionConvoyMemberSlotIds.Remove(index);
			m_aExactMissionConvoyMemberGroupIds.Remove(index);
			m_aExactMissionConvoyMemberMissionIds.Remove(index);
		}
	}

	protected bool HasExplicitExactMissionConvoyMemberDeathEvidence(IEntity entity)
	{
		if (!entity || entity.IsDeleted())
			return false;
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (character)
		{
			CharacterControllerComponent controller = character.GetCharacterController();
			if (controller && controller.GetLifeState() == ECharacterLifeState.DEAD)
				return true;
		}
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return damageManager && damageManager.GetState() == EDamageState.DESTROYED;
	}

	protected bool ExactMissionConvoyRuntimeCrewMatchesFrozenSurvivorSlots(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ActiveGroupState activeGroup,
		HST_ConvoyElementState element)
	{
		if (!state || !mission || !activeGroup || !element || element.m_iSurvivingCrewCount <= 0)
			return false;
		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		HST_ForceManifestState manifest;
		if (operation)
			manifest = state.FindForceManifest(operation.m_sManifestId);
		if (!manifest || !manifest.m_bFrozen)
			return false;

		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		if (!batch || !HasConsistentExactMissionConvoyMemberIdentityArrays())
			return false;

		int expectedMappings;
		array<IEntity> mappedEntities = {};
		SCR_AIGroup exactRoot = SCR_AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId));
		if (!exactRoot)
			return false;
		for (int seatIndex = 0; seatIndex < element.m_iOriginalCrewCount; seatIndex++)
		{
			HST_ForceManifestMemberState member = ResolveExactMissionConvoyManifestMemberForSeat(manifest, element, seatIndex);
			HST_ForceSpawnSlotResultState slotResult;
			if (member)
				slotResult = batch.FindSlotResult(member.m_sSlotId);
			if (!member || member.m_sPrefab.IsEmpty() || !slotResult)
				return false;
			if (slotResult.m_bCasualtyConfirmed)
				continue;

			IEntity mappedEntity;
			string membershipFailure;
			if (!TryGetExactMissionConvoyMappedMemberEntity(mission.m_sInstanceId, activeGroup.m_sGroupId, member.m_sSlotId, mappedEntity)
				|| !mappedEntity || mappedEntity.IsDeleted() || !IsLivingEntity(mappedEntity)
				|| ResolveEntityPrefabName(mappedEntity) != member.m_sPrefab
				|| !IsRuntimeEntityRegisteredExactlyOnceForGroup(activeGroup.m_sGroupId, mappedEntity)
				|| !ValidateForceSpawnGroupMember(activeGroup, exactRoot, mappedEntity, seatIndex, membershipFailure)
				|| mappedEntities.Contains(mappedEntity))
				return false;
			mappedEntities.Insert(mappedEntity);
			expectedMappings++;
		}

		return expectedMappings == element.m_iSurvivingCrewCount
			&& CountExactMissionConvoyMemberMappings(mission.m_sInstanceId, activeGroup.m_sGroupId) == expectedMappings;
	}

	protected bool TryResolveExactMissionConvoyFrozenVehiclePrefab(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, out string prefab)
	{
		prefab = "";
		if (!state || !IsExactMissionConvoyContract(mission) || !asset || !activeGroup)
			return false;

		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
		if (!operation || !element || operation.m_sManifestId.IsEmpty() || element.m_sVehicleSlotId.IsEmpty())
			return false;

		HST_ForceManifestState manifest = state.FindForceManifest(operation.m_sManifestId);
		if (!manifest || !manifest.m_bFrozen || manifest.m_sManifestId != element.m_sManifestId || manifest.m_sOperationId != operation.m_sOperationId)
			return false;

		HST_ForceManifestVehicleState frozenVehicle;
		int matches;
		foreach (HST_ForceManifestVehicleState vehicle : manifest.m_aVehicles)
		{
			if (!vehicle || vehicle.m_sSlotId != element.m_sVehicleSlotId)
				continue;
			frozenVehicle = vehicle;
			matches++;
		}
		if (matches != 1 || !frozenVehicle || !frozenVehicle.m_bRequired)
			return false;
		if (frozenVehicle.m_sGroupElementId != element.m_sCrewGroupElementId)
			return false;
		if (frozenVehicle.m_sPrefab.IsEmpty() || frozenVehicle.m_sPrefab != element.m_sVehiclePrefab || frozenVehicle.m_sPrefab != asset.m_sPrefab)
			return false;
		if (element.m_fVehicleDamageFraction < 0.0 || element.m_fVehicleDamageFraction >= 1.0)
			return false;
		if (element.m_fFuelFraction < 0.0 || element.m_fFuelFraction > 1.0)
			return false;
		if (element.m_fAmmoFraction < 0.0 || element.m_fAmmoFraction > 1.0)
			return false;
		if (!IsValidVehiclePrefabResource(frozenVehicle.m_sPrefab, activeGroup.m_sFactionKey))
			return false;

		prefab = frozenVehicle.m_sPrefab;
		return true;
	}

	protected string BuildExactMissionConvoyVehicleAssetId(HST_ActiveMissionState mission, int index)
	{
		if (!mission || mission.m_sInstanceId.IsEmpty())
			return "";

		return string.Format("asset_%1_convoy_vehicle_%2", mission.m_sInstanceId, index);
	}

	protected int ResolveExactMissionConvoyVehicleAssetIndex(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!IsExactMissionConvoyContract(mission) || !asset)
			return -1;

		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			if (asset.m_sAssetId == BuildExactMissionConvoyVehicleAssetId(mission, index))
				return index;
		}

		return -1;
	}

	protected bool HasExactMissionConvoyVehicleAssetSet(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMissionConvoyContract(mission))
			return false;

		int vehicleAssetCount;
		foreach (HST_MissionAssetState candidate : state.m_aMissionAssets)
		{
			if (candidate && candidate.m_sMissionInstanceId == mission.m_sInstanceId && candidate.m_sRole == MISSION_CONVOY_VEHICLE_ROLE)
				vehicleAssetCount++;
		}
		if (vehicleAssetCount != EXACT_MISSION_CONVOY_VEHICLE_COUNT)
			return false;

		for (int index = 0; index < EXACT_MISSION_CONVOY_VEHICLE_COUNT; index++)
		{
			HST_MissionAssetState asset = state.FindMissionAsset(BuildExactMissionConvoyVehicleAssetId(mission, index));
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
				return false;
		}

		return true;
	}

	protected void BindExactMissionConvoyGroupIdentity(HST_CampaignState state, HST_ActiveGroupState activeGroup, HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!state || !activeGroup || !IsExactMissionConvoyContract(mission) || !asset)
			return;

		activeGroup.m_sOperationId = mission.m_sOperationId;
		activeGroup.m_sManifestId = mission.m_sManifestId;
		activeGroup.m_sMissionInstanceId = mission.m_sInstanceId;
		activeGroup.m_sMissionAssetId = asset.m_sAssetId;
		activeGroup.m_sConvoyElementId = asset.m_sConvoyElementId;

		HST_ConvoyElementState element = state.FindConvoyElement(asset.m_sConvoyElementId);
		if (!element)
			return;
		if (element.m_sMissionInstanceId != mission.m_sInstanceId || element.m_sVehicleAssetId != asset.m_sAssetId || element.m_sGroupId != activeGroup.m_sGroupId)
			return;

		activeGroup.m_iOriginalInfantryCount = Math.Max(activeGroup.m_iOriginalInfantryCount, element.m_iOriginalCrewCount);
		activeGroup.m_iInfantryCount = Math.Max(0, element.m_iSurvivingCrewCount);
		activeGroup.m_iSurvivorInfantryCount = activeGroup.m_iInfantryCount;
		activeGroup.m_iLastSeenAliveCount = activeGroup.m_iInfantryCount;
		activeGroup.m_iDurableLivingInfantryCount = activeGroup.m_iInfantryCount;
		if (!IsZeroVector(element.m_vCurrentPosition))
		{
			activeGroup.m_vPosition = element.m_vCurrentPosition;
			activeGroup.m_vSourcePosition = element.m_vCurrentPosition;
		}
	}

	protected HST_OperationRecordState ResolveOpenExactMissionConvoyOperation(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactActiveMissionConvoy(mission) || mission.m_sOperationId.IsEmpty())
			return null;

		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		if (!operation
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
			|| operation.m_iContractVersion != HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
			return null;

		return operation;
	}

	protected HST_OperationRecordState ResolveExactMissionConvoyOperationForRuntime(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMissionConvoyContract(mission) || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sOperationId.IsEmpty())
			return null;

		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		if (!operation
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
			|| operation.m_iContractVersion != HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
			return null;

		if (mission.m_sRuntimePhase == MISSION_CONVOY_ELIMINATED)
		{
			if (operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
				return null;
		}
		else if (IsTerminalMissionConvoyPhase(mission))
		{
			return null;
		}

		return operation;
	}

	protected HST_OperationRecordState ResolveSettledExactMissionConvoyOperationForRetirement(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMissionConvoyContract(mission))
			return null;
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			return null;
		if (mission.m_sOperationId.IsEmpty() || mission.m_sManifestId.IsEmpty())
			return null;
		if (mission.m_sSpawnResultId.IsEmpty() || mission.m_sSettlementId.IsEmpty())
			return null;

		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY)
			return null;
		if (operation.m_iContractVersion != HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION
			|| operation.m_sMissionInstanceId != mission.m_sInstanceId)
			return null;
		if (operation.m_sManifestId != mission.m_sManifestId
			|| operation.m_sSpawnResultId != mission.m_sSpawnResultId
			|| operation.m_sSettlementId != mission.m_sSettlementId)
			return null;
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED
			|| operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| operation.m_eDutyState != HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED)
			return null;

		HST_ForceManifestState manifest = state.FindForceManifest(mission.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		if (!manifest || !manifest.m_bFrozen || manifest.m_sOperationId != operation.m_sOperationId)
			return null;
		if (!batch || batch.m_sOperationId != operation.m_sOperationId)
			return null;
		if (batch.m_sManifestId != manifest.m_sManifestId || batch.m_sResultId != operation.m_sSpawnResultId)
			return null;
		return operation;
	}

	protected bool IsExactMissionConvoySettledSalvageCaptureAuthority(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		HST_ActiveGroupState activeGroup,
		HST_ConvoyElementState element)
	{
		HST_OperationRecordState operation = ResolveSettledExactMissionConvoyOperationForRetirement(state, mission);
		if (!operation || !asset || !activeGroup || !element)
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED
			|| !IsMissionConvoyCrewEliminationCompletion(mission))
			return false;
		if (operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_DESTROYED)
			return false;
		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRING
			&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED)
			return false;
		if (asset.m_sMissionInstanceId != mission.m_sInstanceId
			|| asset.m_sOperationId != operation.m_sOperationId
			|| asset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE)
			return false;
		if (asset.m_sAssetId != element.m_sVehicleAssetId
			|| asset.m_sManifestSlotId != element.m_sVehicleSlotId
			|| asset.m_sPrefab != element.m_sVehiclePrefab)
			return false;
		if (IsMissionConvoyVehicleAssetResolved(asset) || asset.m_bPickedUp || !asset.m_bAlive)
			return false;
		if (activeGroup.m_sMissionInstanceId != mission.m_sInstanceId
			|| activeGroup.m_sOperationId != operation.m_sOperationId)
			return false;
		if (activeGroup.m_sGroupId != element.m_sGroupId
			|| activeGroup.m_sMissionAssetId != asset.m_sAssetId
			|| activeGroup.m_sConvoyElementId != element.m_sElementId)
			return false;
		if (activeGroup.m_iInfantryCount != 0 || activeGroup.m_iSurvivorInfantryCount != 0
			|| activeGroup.m_iDurableLivingInfantryCount != 0)
			return false;
		if (activeGroup.m_iSurvivorVehicleCount != 1
			|| activeGroup.m_sRuntimeStatus != MISSION_CONVOY_ELIMINATED)
			return false;
		if (element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_RETIRED
			|| element.m_iSurvivingCrewCount != 0 || element.m_bMobile
			|| element.m_fVehicleDamageFraction < 0.0 || element.m_fVehicleDamageFraction >= 1.0)
			return false;

		HST_ForceManifestState manifest = state.FindForceManifest(mission.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		if (!manifest || !batch)
			return false;
		HST_ForceManifestVehicleState frozenVehicle;
		int frozenVehicleClaimants;
		foreach (HST_ForceManifestVehicleState candidateVehicle : manifest.m_aVehicles)
		{
			if (!candidateVehicle || candidateVehicle.m_sSlotId != element.m_sVehicleSlotId)
				continue;
			frozenVehicle = candidateVehicle;
			frozenVehicleClaimants++;
		}
		HST_ForceSpawnSlotResultState vehicleResult = batch.FindSlotResult(element.m_sVehicleSlotId);
		if (frozenVehicleClaimants != 1 || !frozenVehicle || !frozenVehicle.m_bRequired)
			return false;
		if (frozenVehicle.m_sGroupElementId != element.m_sCrewGroupElementId
			|| frozenVehicle.m_sPrefab != asset.m_sPrefab || !vehicleResult)
			return false;
		if (vehicleResult.m_sSlotKind != "vehicle"
			|| vehicleResult.m_sSpawnedPrefab != frozenVehicle.m_sPrefab
			|| vehicleResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED)
			return false;
		if (!vehicleResult.m_bEverAlive || !vehicleResult.m_bAliveVerified || vehicleResult.m_bCasualtyConfirmed)
			return false;
		return true;
	}

	protected bool CanRegisterExactMissionConvoySettledSalvageVehicleHandle(
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		HST_ActiveGroupState activeGroup,
		IEntity vehicleEntity)
	{
		if (!mission || !asset || !activeGroup || !vehicleEntity)
			return false;
		if (vehicleEntity.IsDeleted() || ResolveEntityPrefabName(vehicleEntity) != asset.m_sPrefab)
			return false;
		if (!HasConsistentExactMissionConvoySettledSalvageHandleArrays())
			return false;

		for (int index = 0; index < m_aExactMissionConvoySettledSalvageMissionIds.Count(); index++)
		{
			bool exactExisting = m_aExactMissionConvoySettledSalvageMissionIds[index] == mission.m_sInstanceId
				&& m_aExactMissionConvoySettledSalvageAssetIds[index] == asset.m_sAssetId
				&& m_aExactMissionConvoySettledSalvageGroupIds[index] == activeGroup.m_sGroupId
				&& m_aExactMissionConvoySettledSalvageEntities[index] == vehicleEntity;
			if (exactExisting)
				return true;
			if (m_aExactMissionConvoySettledSalvageAssetIds[index] == asset.m_sAssetId
				|| m_aExactMissionConvoySettledSalvageGroupIds[index] == activeGroup.m_sGroupId
				|| m_aExactMissionConvoySettledSalvageEntities[index] == vehicleEntity)
				return false;
		}
		return true;
	}

	protected bool RegisterExactMissionConvoySettledSalvageVehicleHandle(
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		HST_ActiveGroupState activeGroup,
		IEntity vehicleEntity)
	{
		if (!CanRegisterExactMissionConvoySettledSalvageVehicleHandle(mission, asset, activeGroup, vehicleEntity))
			return false;
		for (int index = 0; index < m_aExactMissionConvoySettledSalvageMissionIds.Count(); index++)
		{
			if (m_aExactMissionConvoySettledSalvageMissionIds[index] == mission.m_sInstanceId
				&& m_aExactMissionConvoySettledSalvageAssetIds[index] == asset.m_sAssetId
				&& m_aExactMissionConvoySettledSalvageGroupIds[index] == activeGroup.m_sGroupId
				&& m_aExactMissionConvoySettledSalvageEntities[index] == vehicleEntity)
				return true;
		}
		m_aExactMissionConvoySettledSalvageMissionIds.Insert(mission.m_sInstanceId);
		m_aExactMissionConvoySettledSalvageAssetIds.Insert(asset.m_sAssetId);
		m_aExactMissionConvoySettledSalvageGroupIds.Insert(activeGroup.m_sGroupId);
		m_aExactMissionConvoySettledSalvageEntities.Insert(vehicleEntity);
		return true;
	}

	protected bool TryGetExactMissionConvoySettledSalvageVehicleHandle(
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		HST_ActiveGroupState activeGroup,
		out IEntity vehicleEntity,
		out int handleIndex)
	{
		vehicleEntity = null;
		handleIndex = -1;
		if (!mission || !asset || !activeGroup)
			return false;
		if (!HasConsistentExactMissionConvoySettledSalvageHandleArrays())
			return false;

		int matches;
		for (int index = 0; index < m_aExactMissionConvoySettledSalvageMissionIds.Count(); index++)
		{
			if (m_aExactMissionConvoySettledSalvageMissionIds[index] != mission.m_sInstanceId
				|| m_aExactMissionConvoySettledSalvageAssetIds[index] != asset.m_sAssetId
				|| m_aExactMissionConvoySettledSalvageGroupIds[index] != activeGroup.m_sGroupId)
				continue;
			vehicleEntity = m_aExactMissionConvoySettledSalvageEntities[index];
			handleIndex = index;
			matches++;
		}
		if (matches != 1 || !vehicleEntity)
			return false;
		int entityClaimants;
		foreach (IEntity candidateEntity : m_aExactMissionConvoySettledSalvageEntities)
		{
			if (candidateEntity == vehicleEntity)
				entityClaimants++;
		}
		return entityClaimants == 1;
	}

	protected bool HasConsistentExactMissionConvoySettledSalvageHandleArrays()
	{
		int count = m_aExactMissionConvoySettledSalvageMissionIds.Count();
		return count == m_aExactMissionConvoySettledSalvageAssetIds.Count()
			&& count == m_aExactMissionConvoySettledSalvageGroupIds.Count()
			&& count == m_aExactMissionConvoySettledSalvageEntities.Count();
	}

	protected void PruneExactMissionConvoySettledSalvageVehicleHandles(HST_CampaignState state)
	{
		int alignedCount = m_aExactMissionConvoySettledSalvageMissionIds.Count();
		alignedCount = Math.Min(alignedCount, m_aExactMissionConvoySettledSalvageAssetIds.Count());
		alignedCount = Math.Min(alignedCount, m_aExactMissionConvoySettledSalvageGroupIds.Count());
		alignedCount = Math.Min(alignedCount, m_aExactMissionConvoySettledSalvageEntities.Count());
		while (m_aExactMissionConvoySettledSalvageMissionIds.Count() > alignedCount)
			m_aExactMissionConvoySettledSalvageMissionIds.Remove(m_aExactMissionConvoySettledSalvageMissionIds.Count() - 1);
		while (m_aExactMissionConvoySettledSalvageAssetIds.Count() > alignedCount)
			m_aExactMissionConvoySettledSalvageAssetIds.Remove(m_aExactMissionConvoySettledSalvageAssetIds.Count() - 1);
		while (m_aExactMissionConvoySettledSalvageGroupIds.Count() > alignedCount)
			m_aExactMissionConvoySettledSalvageGroupIds.Remove(m_aExactMissionConvoySettledSalvageGroupIds.Count() - 1);
		while (m_aExactMissionConvoySettledSalvageEntities.Count() > alignedCount)
			m_aExactMissionConvoySettledSalvageEntities.Remove(m_aExactMissionConvoySettledSalvageEntities.Count() - 1);
		if (!state)
			return;

		for (int index = alignedCount - 1; index >= 0; index--)
		{
			HST_ActiveMissionState mission = state.FindActiveMission(m_aExactMissionConvoySettledSalvageMissionIds[index]);
			HST_MissionAssetState asset = state.FindMissionAsset(m_aExactMissionConvoySettledSalvageAssetIds[index]);
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(m_aExactMissionConvoySettledSalvageGroupIds[index]);
			HST_ConvoyElementState element;
			if (mission && asset && activeGroup)
				element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
			IEntity vehicleEntity = m_aExactMissionConvoySettledSalvageEntities[index];
			if (vehicleEntity && !vehicleEntity.IsDeleted()
				&& IsExactMissionConvoySettledSalvageCaptureAuthority(state, mission, asset, activeGroup, element))
				continue;
			RemoveExactMissionConvoySettledSalvageVehicleHandleAt(index);
		}
	}

	protected void RemoveExactMissionConvoySettledSalvageVehicleHandleAt(int index)
	{
		if (index < 0 || index >= m_aExactMissionConvoySettledSalvageMissionIds.Count()
			|| index >= m_aExactMissionConvoySettledSalvageAssetIds.Count()
			|| index >= m_aExactMissionConvoySettledSalvageGroupIds.Count()
			|| index >= m_aExactMissionConvoySettledSalvageEntities.Count())
			return;
		m_aExactMissionConvoySettledSalvageMissionIds.Remove(index);
		m_aExactMissionConvoySettledSalvageAssetIds.Remove(index);
		m_aExactMissionConvoySettledSalvageGroupIds.Remove(index);
		m_aExactMissionConvoySettledSalvageEntities.Remove(index);
	}

	protected HST_ConvoyElementState ResolveExactMissionConvoyElement(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup)
	{
		if (!state || !IsExactMissionConvoyContract(mission) || !asset || !activeGroup || asset.m_sConvoyElementId.IsEmpty())
			return null;

		HST_ConvoyElementState element = state.FindConvoyElement(asset.m_sConvoyElementId);
		if (!element
			|| element.m_sElementId != asset.m_sConvoyElementId
			|| element.m_sOperationId != mission.m_sOperationId
			|| element.m_sMissionInstanceId != mission.m_sInstanceId
			|| element.m_sVehicleAssetId != asset.m_sAssetId
			|| element.m_sGroupId != activeGroup.m_sGroupId)
			return null;
		if (activeGroup.m_sMissionInstanceId != mission.m_sInstanceId
			|| activeGroup.m_sOperationId != mission.m_sOperationId
			|| activeGroup.m_sMissionAssetId != asset.m_sAssetId
			|| activeGroup.m_sConvoyElementId != element.m_sElementId)
			return null;

		return element;
	}

	protected bool IsExactMissionConvoyElementTerminal(HST_ConvoyElementState element)
	{
		if (!element)
			return true;

		return element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
			&& element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED;
	}

	protected void RemoveRestoredMissionConvoyRuntimeRebuildAttempt(string groupId)
	{
		if (groupId.IsEmpty())
			return;

		int index = m_aRestoredMissionConvoyRebuildGroupIds.Find(groupId);
		if (index >= 0)
			m_aRestoredMissionConvoyRebuildGroupIds.Remove(index);
	}

	protected bool IsMissionConvoyGroup(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup || activeGroup.m_sGroupId.IsEmpty() || activeGroup.m_sMissionInstanceId.IsEmpty())
			return false;
		string missionPrefix = string.Format("%1%2_", MISSION_CONVOY_GROUP_PREFIX, activeGroup.m_sMissionInstanceId);
		return activeGroup.m_sGroupId.StartsWith(missionPrefix);
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
			if (TryAssignCurrentMissionConvoyRoute(state, mission, activeGroup, waypoints))
			{
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
					Print(string.Format("Partisan mission convoy | waypoint assignment unavailable for %1: %2", activeGroup.m_sGroupId, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			}
			if (activeGroup.m_sSpawnFallbackMode != previousFallbackMode || activeGroup.m_iAssignedWaypointCount != previousWaypointCount || activeGroup.m_sSpawnFailureReason != previousReason)
				changed = true;
		}

		return changed || (eligibleGroups > 0 && assignedGroups == eligibleGroups);
	}

	protected bool TryAssignCurrentMissionConvoyRoute(HST_CampaignState state, HST_ActiveMissionState mission, HST_ActiveGroupState activeGroup, array<vector> routeWaypoints = null)
	{
		if (!state || !mission || !activeGroup)
			return false;
		if (!routeWaypoints)
			routeWaypoints = BuildMissionConvoyWaypointPositions(ResolveMissionConvoyRoute(state, mission));
		if (!TryAssignConvoyWaypoints(activeGroup, routeWaypoints))
			return false;

		activeGroup.m_sSpawnFallbackMode = "convoy_waypoints";
		activeGroup.m_sConvoyRuntimeStage = "ROUTE_ASSIGNED";
		return true;
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
		vector currentPosition = activeGroup.m_vPosition;
		IEntity vehicleEntity = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
		if (vehicleEntity)
			currentPosition = vehicleEntity.GetOrigin();
		if (IsZeroVector(currentPosition))
			currentPosition = activeGroup.m_vSourcePosition;
		ref array<vector> remainingRouteWaypoints = BuildRemainingMissionConvoyRouteWaypoints(currentPosition, routeWaypoints);

		if (remainingRouteWaypoints && remainingRouteWaypoints.Count() > 0)
		{
			int desiredWaypointCount = Math.Max(CONVOY_RUNTIME_WAYPOINT_MIN_COUNT, Math.Min(CONVOY_RUNTIME_WAYPOINT_MAX_COUNT, remainingRouteWaypoints.Count() + 2));
			int leadInWaypointCount = Math.Max(0, desiredWaypointCount - remainingRouteWaypoints.Count());
			AppendConvoyLeadInWaypoints(result, currentPosition, remainingRouteWaypoints[0], leadInWaypointCount);
			foreach (vector routeWaypoint : remainingRouteWaypoints)
			{
				if (result.Count() >= CONVOY_RUNTIME_WAYPOINT_MAX_COUNT)
					break;

				AppendResolvedConvoyWaypoint(result, routeWaypoint);
			}
		}

		if (result.Count() == 0 && !IsZeroVector(activeGroup.m_vTargetPosition))
			AppendConvoyRoadSegmentWaypoints(result, currentPosition, activeGroup.m_vTargetPosition, activeGroup.m_vTargetPosition);
		if (result.Count() == 1 && !IsZeroVector(activeGroup.m_vTargetPosition))
			AppendConvoyRoadWaypoint(result, activeGroup.m_vTargetPosition, activeGroup.m_vTargetPosition);

		return result;
	}

	protected ref array<vector> BuildRemainingMissionConvoyRouteWaypoints(vector currentPosition, array<vector> routeWaypoints)
	{
		ref array<vector> remaining = {};
		if (!routeWaypoints || routeWaypoints.Count() == 0)
			return remaining;
		if (routeWaypoints.Count() == 1 || IsZeroVector(currentPosition))
		{
			foreach (vector waypoint : routeWaypoints)
				remaining.Insert(waypoint);
			return remaining;
		}

		int firstForwardIndex = 1;
		float bestDistanceSq = 999999999.0;
		for (int index = 1; index < routeWaypoints.Count(); index++)
		{
			vector projected = ClosestPointOnSegment2D(routeWaypoints[index - 1], routeWaypoints[index], currentPosition);
			float distanceSq = DistanceSq2D(projected, currentPosition);
			if (distanceSq > bestDistanceSq + 0.01)
				continue;

			bestDistanceSq = distanceSq;
			firstForwardIndex = index;
		}

		for (int forwardIndex = firstForwardIndex; forwardIndex < routeWaypoints.Count(); forwardIndex++)
			remaining.Insert(routeWaypoints[forwardIndex]);
		return remaining;
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

			int vehicleIndex = index;
			if (IsExactMissionConvoyContract(mission))
			{
				vehicleIndex = ResolveExactMissionConvoyVehicleAssetIndex(mission, asset);
				if (vehicleIndex < 0)
					continue;
			}
			else
			{
				index++;
			}
			vector position = ResolveMissionConvoyVehiclePosition(asset, BuildMissionConvoyGroupId(mission, vehicleIndex));
			UpdateMissionConvoyAssetPosition(state, asset, position);
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
		if (IsExactMissionConvoyContract(mission))
			changed = SampleExactMissionConvoyVehicleRuntimeState(state, mission, asset, activeGroup, vehicleEntity) || changed;

		return changed;
	}

	protected bool IsProtectedExactMissionConvoyAuthority(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sRuntimePrimitive != MISSION_CONVOY_PRIMITIVE)
			return false;
		return mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION
			|| mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION;
	}

	protected void ApplyExactMissionConvoyVehicleRuntimeState(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, IEntity vehicleEntity)
	{
		if (!state || !vehicleEntity || !IsExactMissionConvoyContract(mission) || !asset || !activeGroup)
			return;

		HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
		if (!element)
			return;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(vehicleEntity.FindComponent(SCR_DamageManagerComponent));
		if (damageManager)
		{
			float healthFraction = 1.0 - Math.Max(0.0, Math.Min(1.0, element.m_fVehicleDamageFraction));
			damageManager.SetHealthScaled(healthFraction);
		}

		array<SCR_FuelManagerComponent> fuelManagers = {};
		if (SCR_FuelManagerComponent.GetAllFuelManagers(vehicleEntity, fuelManagers) > 0)
			SCR_FuelManagerComponent.SetTotalFuelPercentageOfFuelManagers(fuelManagers, Math.Max(0.0, Math.Min(1.0, element.m_fFuelFraction)), 0, SCR_EFuelNodeTypeFlag.IS_FUEL_STORAGE);
		ApplyMissionConvoyVehicleAmmoFraction(vehicleEntity, element.m_fAmmoFraction);
	}

	protected bool SampleExactMissionConvoyVehicleRuntimeState(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ActiveGroupState activeGroup, IEntity vehicleEntity)
	{
		if (!state || !vehicleEntity || !IsExactMissionConvoyContract(mission) || !asset || !activeGroup)
			return false;

		HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
		if (!element)
			return false;

		bool changed;
		float damageFraction;
		if (TrySampleMissionConvoyVehicleDamageFraction(vehicleEntity, damageFraction) && Math.AbsFloat(element.m_fVehicleDamageFraction - damageFraction) > 0.001)
		{
			element.m_fVehicleDamageFraction = damageFraction;
			changed = true;
		}

		float fuelFraction;
		if (TrySampleMissionConvoyVehicleFuelFraction(vehicleEntity, fuelFraction) && Math.AbsFloat(element.m_fFuelFraction - fuelFraction) > 0.001)
		{
			element.m_fFuelFraction = fuelFraction;
			changed = true;
		}

		float ammoFraction;
		if (TrySampleMissionConvoyVehicleAmmoFraction(vehicleEntity, ammoFraction) && Math.AbsFloat(element.m_fAmmoFraction - ammoFraction) > 0.001)
		{
			element.m_fAmmoFraction = ammoFraction;
			changed = true;
		}

		if (changed)
		{
			element.m_iLastUpdatedSecond = state.m_iElapsedSeconds;
			element.m_iRevision++;
		}
		return changed;
	}

	protected bool TrySampleMissionConvoyVehicleDamageFraction(IEntity vehicleEntity, out float damageFraction)
	{
		damageFraction = 0.0;
		if (!vehicleEntity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(vehicleEntity.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager)
			return false;

		damageFraction = Math.Max(0.0, Math.Min(1.0, 1.0 - damageManager.GetHealthScaled()));
		return true;
	}

	protected bool TrySampleMissionConvoyVehicleFuelFraction(IEntity vehicleEntity, out float fuelFraction)
	{
		fuelFraction = 0.0;
		if (!vehicleEntity)
			return false;

		array<SCR_FuelManagerComponent> fuelManagers = {};
		if (SCR_FuelManagerComponent.GetAllFuelManagers(vehicleEntity, fuelManagers) <= 0)
			return false;

		float totalFuel;
		float totalMaxFuel;
		SCR_FuelManagerComponent.GetTotalValuesOfFuelNodesOfFuelManagers(fuelManagers, totalFuel, totalMaxFuel, fuelFraction, 0, SCR_EFuelNodeTypeFlag.IS_FUEL_STORAGE);
		if (totalMaxFuel <= 0.0)
			return false;

		fuelFraction = Math.Max(0.0, Math.Min(1.0, fuelFraction));
		return true;
	}

	protected bool ApplyMissionConvoyVehicleAmmoFraction(IEntity vehicleEntity, float ammoFraction)
	{
		if (!vehicleEntity || !Replication.IsServer())
			return false;
		ref array<BaseWeaponComponent> weapons = {};
		ref array<BaseMagazineComponent> magazines = {};
		CollectMissionConvoyVehicleMagazinesRecursive(vehicleEntity, weapons, magazines);
		int writableMagazines;
		float clampedFraction = Math.Max(0.0, Math.Min(1.0, ammoFraction));
		foreach (BaseMagazineComponent magazine : magazines)
		{
			if (!magazine)
				continue;
			int maxAmmo = magazine.GetMaxAmmoCount();
			if (maxAmmo <= 0)
				continue;
			magazine.SetAmmoCount(Math.Round(maxAmmo * clampedFraction));
			writableMagazines++;
		}
		return writableMagazines > 0;
	}

	protected bool TrySampleMissionConvoyVehicleAmmoFraction(IEntity vehicleEntity, out float ammoFraction)
	{
		ammoFraction = 0.0;
		if (!vehicleEntity)
			return false;
		ref array<BaseWeaponComponent> weapons = {};
		ref array<BaseMagazineComponent> magazines = {};
		CollectMissionConvoyVehicleMagazinesRecursive(vehicleEntity, weapons, magazines);
		int totalAmmo;
		int totalMaxAmmo;
		foreach (BaseMagazineComponent magazine : magazines)
		{
			if (!magazine)
				continue;
			int maxAmmo = magazine.GetMaxAmmoCount();
			if (maxAmmo <= 0)
				continue;
			totalMaxAmmo += maxAmmo;
			totalAmmo += Math.Max(0, Math.Min(maxAmmo, magazine.GetAmmoCount()));
		}
		if (totalMaxAmmo <= 0)
			return false;
		ammoFraction = totalAmmo * 1.0 / totalMaxAmmo;
		return true;
	}

	protected void CollectMissionConvoyVehicleMagazinesRecursive(IEntity entity, array<BaseWeaponComponent> weapons, array<BaseMagazineComponent> magazines)
	{
		if (!entity || !weapons || !magazines)
			return;
		array<Managed> components = {};
		entity.FindComponents(BaseWeaponComponent, components);
		foreach (Managed component : components)
		{
			BaseWeaponComponent weapon = BaseWeaponComponent.Cast(component);
			if (!weapon || weapons.Contains(weapon))
				continue;
			weapons.Insert(weapon);
			BaseMagazineComponent magazine = weapon.GetCurrentMagazine();
			if (magazine && !magazines.Contains(magazine))
				magazines.Insert(magazine);
		}

		IEntity child = entity.GetChildren();
		while (child)
		{
			CollectMissionConvoyVehicleMagazinesRecursive(child, weapons, magazines);
			child = child.GetSibling();
		}
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
		{
			string assignedVehicleSlotId;
			HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
			if (IsExactMissionConvoyContract(mission))
			{
				if (asset.m_sManifestSlotId.IsEmpty())
					return;
				assignedVehicleSlotId = asset.m_sManifestSlotId;
			}
			SyncMissionConvoyPayloadPositions(state, asset.m_sMissionInstanceId, position, assignedVehicleSlotId);
		}
	}

	protected void SyncMissionConvoyPayloadPositions(HST_CampaignState state, string missionInstanceId, vector position, string assignedVehicleSlotId = "")
	{
		if (!state || missionInstanceId.IsEmpty() || IsZeroVector(position))
			return;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != missionInstanceId)
				continue;
			if (asset.m_sRole != MISSION_CONVOY_PAYLOAD_ROLE && asset.m_sRole != MISSION_CONVOY_CAPTIVE_ROLE)
				continue;
			if (!assignedVehicleSlotId.IsEmpty() && asset.m_sAssignedVehicleSlotId != assignedVehicleSlotId)
				continue;
			if (!assignedVehicleSlotId.IsEmpty() && !asset.m_bAttachedToCarrier)
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
		if (IsExactMissionConvoyContract(mission))
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
		Print(string.Format("Partisan mission convoy | %1 failed: %2%3", mission.m_sInstanceId, reason, BuildMissionConvoyFailureContext(state, mission)), LogLevel.WARNING);
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
		if (state)
		{
			HST_ActiveMissionState exactMission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
			if (IsExactMissionConvoyContract(exactMission))
			{
				HST_MissionAssetState exactAsset = FindMissionConvoyAssetForGroup(state, exactMission, activeGroup);
				HST_ConvoyElementState exactElement = ResolveExactMissionConvoyElement(state, exactMission, exactAsset, activeGroup);
				if (!exactElement || exactElement.m_iSurvivingCrewCount <= 0)
					return 0;
				if (IsMissionConvoyGroupAssetTerminal(state, activeGroup)
					&& !IsExactMissionConvoyTerminalSurvivingCrew(exactAsset, exactElement))
					return 0;
			}
			else if (IsMissionConvoyGroupAssetTerminal(state, activeGroup))
				return 0;
		}

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
		if (IsExactMissionConvoyContract(mission))
		{
			if (activeGroup.m_sMissionAssetId.IsEmpty())
				return null;

			HST_MissionAssetState exactAsset = state.FindMissionAsset(activeGroup.m_sMissionAssetId);
			if (!exactAsset
				|| exactAsset.m_sMissionInstanceId != mission.m_sInstanceId
				|| exactAsset.m_sRole != MISSION_CONVOY_VEHICLE_ROLE
				|| exactAsset.m_sConvoyElementId != activeGroup.m_sConvoyElementId)
				return null;

			return exactAsset;
		}

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
		if (!groupId.StartsWith(prefix))
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
		if (zone.m_bActive)
		{
			// Authored zone activation overrides retain the configured global
			// hysteresis margin instead of collapsing to Max(override, global exit).
			float deactivationMargin = Math.Max(
				100.0,
				balance.m_iDeactivationRadiusMeters - balance.m_iActivationRadiusMeters);
			radius += deactivationMargin;
		}

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
		if (HasHeldForceSpawnGarrisonProjection(state, zone))
		{
			ApplyActiveZoneCounts(state, zone);
			return changed;
		}

		bool hasActiveGarrisonGroup = HasActiveGarrisonGroup(state, zone);
		if (hasActiveGarrisonGroup && !fullGarrison)
		{
			bool projectionChanged = EnsureTownSecurityPoliceProjection(state, zone, preset, slots, compositions);
			if (projectionChanged)
				changed = true;
			ApplyActiveZoneCounts(state, zone);
			return changed;
		}

		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, zone.m_sOwnerFactionKey);
		if (!garrison)
		{
			bool projectionChanged = EnsureTownSecurityPoliceProjection(state, zone, preset, slots, compositions);
			if (projectionChanged)
				changed = true;
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
		bool policeProjectionChanged = EnsureTownSecurityPoliceProjection(state, zone, preset, slots, compositions);
		if (policeProjectionChanged)
		{
			changed = true;
			ApplyActiveZoneCounts(state, zone);
		}
		if (zone.m_iActiveInfantryCount < infantryCount && infantryCount > 0)
		{
			int pendingInfantry = CountPendingActiveZonePopulationInfantry(state, zone);
			int pendingGroups = CountPendingActiveZonePopulationGroups(state, zone);
			if (pendingInfantry > 0)
			{
				Print(string.Format(
					"Partisan garrison | activation pending native population | zone %1 | requested infantry %2 | active infantry %3 | pending infantry %4 | pending groups %5",
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
					"Partisan garrison | activation partial | zone %1 | requested infantry %2 | active infantry %3 | pending infantry 0 | folded failures may have returned to abstract garrison",
					zone.m_sZoneId,
					infantryCount,
					zone.m_iActiveInfantryCount
				), LogLevel.WARNING);
			}
		}
		if (zone.m_iActiveVehicleCount < vehicleCount && vehicleCount > 0)
		{
			Print(string.Format(
				"Partisan garrison | activation partial | zone %1 | requested vehicles %2 | active vehicles %3 | folded failures may have returned to abstract garrison",
				zone.m_sZoneId,
				vehicleCount,
				zone.m_iActiveVehicleCount
			), LogLevel.WARNING);
		}
		string activationReport = string.Format("Partisan | activated zone %1 | requested infantry %2/%3 vehicles %4/%5 | spawned infantry groups %6 vehicle groups %7", zone.m_sZoneId, infantryCount, garrisonInfantryBefore, vehicleCount, garrisonVehiclesBefore, spawnedInfantryGroups, spawnedVehicleGroups);
		activationReport = activationReport + string.Format(" | active now infantry %1 vehicles %2 | abstract garrison now infantry %3 vehicles %4", zone.m_iActiveInfantryCount, zone.m_iActiveVehicleCount, garrison.m_iInfantryCount, garrison.m_iVehicleCount);
		Print(string.Format("%1", activationReport));
		return true;
	}

	bool CleanupCapturedZoneHostileRuntime(HST_CampaignState state, string zoneId, string resistanceFactionKey)
	{
		return CleanupZoneHostileRuntime(state, zoneId, resistanceFactionKey);
	}

	bool CleanupZoneHostileRuntime(HST_CampaignState state, string zoneId, string controllingFactionKey)
	{
		if (!state || zoneId.IsEmpty() || controllingFactionKey.IsEmpty())
			return false;

		bool changed;
		int removedGroups;
		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			if (!activeGroup || activeGroup.m_sZoneId != zoneId)
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsLocalSecurityPatrolClaimant(state, activeGroup))
				continue;
			if (ShouldHoldForceSpawnProjection(state, activeGroup))
				continue;
			if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;
			if (IsExactPlayerSupportActiveGroup(state, activeGroup))
				continue;
			if (activeGroup.m_bQRF || IsMissionConvoyGroup(activeGroup))
				continue;
			if (IsTownSecurityPoliceProjection(activeGroup))
			{
				DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
				state.m_aActiveGroups.Remove(i);
				removedGroups++;
				changed = true;
				continue;
			}
			if (activeGroup.m_sFactionKey == controllingFactionKey)
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
			Print(string.Format("Partisan capture | cleaned %1 hostile active group(s) after ownership flip at %2", removedGroups, zoneId));
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
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		if (IsLocalSecurityPatrolClaimant(state, activeGroup))
			return false;
		if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
			return false;
		if (IsExactPlayerSupportActiveGroup(state, activeGroup))
			return false;
		if (ShouldHoldForceSpawnProjection(state, activeGroup))
			return false;

		if (activeGroup.m_sRuntimeStatus == "folded")
			return true;
		if (TryEliminateCrewlessMixedActiveGroup(state, activeGroup, "support fold"))
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
			if (!activeGroup || activeGroup.m_sZoneId != zone.m_sZoneId || activeGroup.m_bQRF || IsMissionOwnedActiveGroup(activeGroup)
				|| IsExactPlayerSupportActiveGroup(state, activeGroup))
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsLocalSecurityPatrolClaimant(state, activeGroup))
				continue;
			if (ShouldHoldForceSpawnProjection(state, activeGroup))
				continue;
			if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;

			if (IsTownSecurityPoliceProjection(activeGroup))
			{
				DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
				state.m_aActiveGroups.Remove(i);
				foldedGroups++;
				changed = true;
				continue;
			}

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

			Print(string.Format("Partisan | deactivated zone %1 | folded groups %2 | returned infantry %3 vehicles %4 | abstract garrison now infantry %5 vehicles %6", zone.m_sZoneId, foldedGroups, returnedInfantry, returnedVehicles, garrisonInfantry, garrisonVehicles));
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
		Print(string.Format("Partisan | detached player-used active vehicle %1 from zone cleanup | zone %2 | reason %3 | position %4", activeGroup.m_sGroupId, zone.m_sZoneId, reason, vehicle.GetOrigin()));
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
		Print(string.Format("Partisan | vehicle spawn blocked for %1 until zone unload: %2", zoneId, reason), LogLevel.WARNING);
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

		string prefab = activeGroup.m_sVehiclePrefab;
		if ((prefab.IsEmpty() || !HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab)) && vehicle.GetPrefabData())
			prefab = vehicle.GetPrefabData().GetPrefabName();
		if ((prefab.IsEmpty() || !HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab)) && HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(activeGroup.m_sPrefab))
			prefab = activeGroup.m_sPrefab;
		if (prefab.IsEmpty() || !HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
		{
			Print(string.Format("Partisan | detached active vehicle %1 has no eligible vehicle-root prefab | group prefab %2 | vehicle prefab %3 | reason %4", runtimeId, activeGroup.m_sPrefab, activeGroup.m_sVehiclePrefab, reason), LogLevel.WARNING);
			return;
		}

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(runtimeId);
		bool preservePersistentFieldRecord = ShouldPreservePersistentDetachedVehicleRecord(runtimeVehicle);
		if (!runtimeVehicle)
		{
			runtimeVehicle = new HST_RuntimeVehicleState();
			runtimeVehicle.m_sVehicleRuntimeId = runtimeId;
			state.m_aRuntimeVehicles.Insert(runtimeVehicle);
		}

		runtimeVehicle.m_sVehicleRuntimeId = runtimeId;
		runtimeVehicle.m_sPrefab = prefab;
		runtimeVehicle.m_sDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(prefab, vehicle.GetName());
		runtimeVehicle.m_sFactionKey = activeGroup.m_sFactionKey;
		runtimeVehicle.m_sZoneId = activeGroup.m_sZoneId;
		if (!preservePersistentFieldRecord)
			runtimeVehicle.m_sRuntimeKind = "detached_active_vehicle";
		runtimeVehicle.m_vPosition = vehicle.GetOrigin();
		runtimeVehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(vehicle.GetYawPitchRoll());
		if (runtimeVehicle.m_iSpawnedAtSecond <= 0)
			runtimeVehicle.m_iSpawnedAtSecond = activeGroup.m_iSpawnedAtSecond;
		runtimeVehicle.m_bDetached = !preservePersistentFieldRecord;
		runtimeVehicle.m_bDeleted = false;
		HST_VehicleCapabilityPolicy.ApplyToRuntimeVehicle(runtimeVehicle);
		HST_VehicleCapabilityPolicy.ApplyUndercoverToRuntimeVehicle(runtimeVehicle);
	}

	protected bool ShouldPreservePersistentDetachedVehicleRecord(HST_RuntimeVehicleState runtimeVehicle)
	{
		if (!runtimeVehicle || runtimeVehicle.m_bDeleted || runtimeVehicle.m_bDetached)
			return false;

		return runtimeVehicle.m_sRuntimeKind == "loot_vehicle"
			|| runtimeVehicle.m_sRuntimeKind == "field_vehicle"
			|| runtimeVehicle.m_sRuntimeKind == "garage_redeploy";
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

	protected bool IsAnyPlayerInVehicle(IEntity vehicle)
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
			if (!playerEntity)
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

	protected bool EnsureTownSecurityPoliceProjection(HST_CampaignState state, HST_ZoneState zone, HST_CampaignPreset preset, array<ref HST_ZoneSpawnSlotState> slots, HST_ZoneCompositionService compositions)
	{
		if (!state)
			return false;
		if (!zone)
			return false;

		// Schema-66 exact local-security authority owns all new police patrols.
		// This compatibility seam only retires backlink-free legacy projections;
		// it must never fall back to an aggregate runtime group.
		return CleanupTownSecurityPoliceProjections(
			state,
			zone,
			"legacy town-security projection retired by exact local-security authority");
	}

	protected bool ShouldProjectTownSecurityPolice(HST_CampaignState state, HST_ZoneState zone, HST_CampaignPreset preset, int policePresence)
	{
		if (!state)
			return false;
		if (!zone)
			return false;
		if (policePresence <= 0)
			return false;
		if (zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return false;
		if (!HST_FactionRelationService.IsEnemyFaction(preset, zone.m_sOwnerFactionKey))
			return false;

		return true;
	}

	protected int ResolveTownSecurityPoliceProjectionInfantry(int policePresence)
	{
		if (policePresence <= 0)
			return 0;

		int infantryCount = policePresence + 1;
		if (infantryCount < TOWN_SECURITY_POLICE_MIN_INFANTRY)
			infantryCount = TOWN_SECURITY_POLICE_MIN_INFANTRY;
		if (infantryCount > TOWN_SECURITY_POLICE_MAX_INFANTRY)
			infantryCount = TOWN_SECURITY_POLICE_MAX_INFANTRY;
		return infantryCount;
	}

	protected HST_ActiveGroupState FindTownSecurityPoliceProjection(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state)
			return null;
		if (!zone)
			return null;

		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup)
				continue;
			if (activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;
			if (IsTownSecurityPoliceProjection(activeGroup))
				return activeGroup;
		}

		return null;
	}

	protected int CountTownSecurityPoliceProjections(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state)
			return 0;
		if (!zone)
			return 0;

		int count;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup)
				continue;
			if (activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;
			if (IsTownSecurityPoliceProjection(activeGroup))
				count++;
		}

		return count;
	}

	protected bool CleanupTownSecurityPoliceProjections(HST_CampaignState state, HST_ZoneState zone, string reason)
	{
		if (!state)
			return false;
		if (!zone)
			return false;

		bool changed;
		int removed;
		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			if (!activeGroup)
				continue;
			if (activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;
			if (IsLocalSecurityPatrolClaimant(state, activeGroup))
				continue;
			if (!IsTownSecurityPoliceProjection(activeGroup))
				continue;
			if (ShouldHoldForceSpawnProjection(state, activeGroup))
				continue;

			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			state.m_aActiveGroups.Remove(i);
			removed++;
			changed = true;
		}

		if (changed)
		{
			ApplyActiveZoneCounts(state, zone);
			m_bMarkerRefreshNeeded = true;
			DebugLog(string.Format("town security police projections cleaned %1 | removed %2 | reason %3", zone.m_sZoneId, removed, reason));
		}

		return changed;
	}

	protected bool IsTownSecurityPoliceProjection(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;
		if (!activeGroup.m_sSpawnFallbackMode.Contains(TOWN_SECURITY_POLICE_PROJECTION_TOKEN))
			return false;
		if (!activeGroup.m_sOperationId.IsEmpty() || !activeGroup.m_sManifestId.IsEmpty()
			|| !activeGroup.m_sSpawnResultId.IsEmpty() || !activeGroup.m_sForceId.IsEmpty()
			|| !activeGroup.m_sProjectionId.IsEmpty() || !activeGroup.m_sLocalSecurityPatrolId.IsEmpty()
			|| !activeGroup.m_sMissionInstanceId.IsEmpty() || !activeGroup.m_sSupportRequestId.IsEmpty()
			|| !activeGroup.m_sEnemyOrderId.IsEmpty() || !activeGroup.m_sConvoyElementId.IsEmpty()
			|| !activeGroup.m_sGarrisonZoneId.IsEmpty() || !activeGroup.m_sQRFInstanceId.IsEmpty()
			|| activeGroup.m_bQRF)
			return false;

		return true;
	}

	protected string BuildTownSecurityPoliceGroupId(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state)
			return "";
		if (!zone)
			return "";

		return string.Format("police_%1_%2_%3_%4", zone.m_sZoneId, zone.m_sOwnerFactionKey, state.m_iElapsedSeconds, state.CountOperationalActiveGroups());
	}

	protected bool SpawnActiveZoneGroup(HST_CampaignState state, HST_ZoneState zone, string factionKey, int infantryCount, int vehicleCount, bool qrf, HST_CampaignPreset preset, HST_ZoneSpawnSlotState slot, string runtimeStatus)
	{
		HST_ActiveGroupState activeGroup = CreateActiveGroup(state, zone, factionKey, infantryCount, vehicleCount, qrf, preset);
		ApplySpawnSlot(activeGroup, slot, runtimeStatus);
		if (vehicleCount > 0 && infantryCount <= 0)
		{
			activeGroup.m_sPrefab = SelectVehiclePrefab(state, zone, factionKey, state.CountOperationalActiveGroups(), preset);
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
			garrison.m_sGarrisonId = HST_StableIdService.BuildGarrisonId(zoneId, factionKey);
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
			if (zone.m_sOwnerFactionKey == resistanceFactionKey || zone.m_sQRFRouteId.IsEmpty())
				continue;

			bool verifiedPressure = zone.m_iResistanceCaptureProgress > 0;
			if (!verifiedPressure)
			{
				HST_CombatPresenceResult pressure = m_CombatPresence.QueryZoneHostilePresence(
					state,
					preset,
					zone.m_sOwnerFactionKey,
					zone,
					false);
				if (!pressure || !pressure.m_bQueryValid || !pressure.m_bHasLiveContributors)
					continue;
			}

			if (IsZoneInsideHQSafeArea(state, zone))
				continue;

			if (state.m_iElapsedSeconds < zone.m_iQrfCooldownUntilSecond)
				continue;

			if (state.FindActiveQRF(zone.m_sZoneId, zone.m_sOwnerFactionKey))
				continue;

			if (HasOpenEnemyCommanderQRF(state, zone.m_sZoneId, zone.m_sOwnerFactionKey))
				continue;

			string qrfDecisionReason;
			bool cooldownOnSkip;
			if (!ShouldDispatchLegacyQRF(state, zone, enemyDirector, qrfDecisionReason, cooldownOnSkip))
			{
				if (cooldownOnSkip)
					zone.m_iQrfCooldownUntilSecond = state.m_iElapsedSeconds + QRF_CHANCE_REJECT_COOLDOWN_SECONDS;
				continue;
			}

			HST_QRFState qrf = new HST_QRFState();
			qrf.m_sInstanceId = string.Format("qrf_%1_%2_%3", zone.m_sZoneId, zone.m_sOwnerFactionKey, state.m_iElapsedSeconds);
			qrf.m_sFactionKey = zone.m_sOwnerFactionKey;
			qrf.m_sSourceZoneId = ResolveQRFSourceZoneId(state, zone, zone.m_sOwnerFactionKey);
			qrf.m_sTargetZoneId = zone.m_sZoneId;
			qrf.m_iStartedAtSecond = state.m_iElapsedSeconds;
			qrf.m_iETASeconds = QRF_ETA_SECONDS;
			string qrfSpendReason;
			if (!enemyDirector.TrySpendDefense(
				state,
				zone,
				zone.m_sOwnerFactionKey,
				QRF_ATTACK_RESOURCE_COST,
				QRF_SUPPORT_RESOURCE_COST,
				qrfSpendReason,
				"enemy_resource_debit_" + qrf.m_sInstanceId,
				qrf.m_sInstanceId,
				"",
				HST_StableIdService.BuildOperationId("legacy_qrf", qrf.m_sInstanceId)))
				continue;

			state.m_aQRFs.Insert(qrf);
			zone.m_iQrfCooldownUntilSecond = state.m_iElapsedSeconds + QRF_COOLDOWN_SECONDS;
			m_bMarkerRefreshNeeded = true;
			Print(string.Format("Partisan | dispatched QRF %1 from %2 to pressured zone %3 | physical spawn at T-%4s", qrf.m_sInstanceId, qrf.m_sSourceZoneId, zone.m_sZoneId, QRF_INBOUND_SPAWN_SECONDS));
			NotifyRuntimeEvent(state, "qrf_dispatched_" + qrf.m_sInstanceId, "QRF Dispatched", string.Format("%1 is sending a quick reaction force toward %2.", zone.m_sOwnerFactionKey, ResolveZoneDisplayName(state, zone.m_sZoneId)), zone.m_sZoneId, zone.m_vPosition, 6.0);
			changed = true;
		}

		return changed;
	}

	protected bool HasOpenEnemyCommanderQRF(HST_CampaignState state, string targetZoneId, string factionKey)
	{
		if (!state || targetZoneId.IsEmpty() || factionKey.IsEmpty())
			return false;

		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_eType != HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					order.m_sTargetZoneId, targetZoneId)
				|| order.m_sFactionKey != factionKey)
				continue;
			if (order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED
				&& order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;
			return true;
		}

		return false;
	}

	protected bool ShouldDispatchLegacyQRF(HST_CampaignState state, HST_ZoneState zone, HST_EnemyDirectorService enemyDirector, out string reason, out bool cooldownOnSkip)
	{
		reason = "";
		cooldownOnSkip = false;
		if (!state || !zone || !enemyDirector)
		{
			reason = "qrf context missing";
			return false;
		}

		HST_FactionPoolState pool = state.FindFactionPool(zone.m_sOwnerFactionKey);
		if (!pool || pool.m_iAggression <= 0)
		{
			reason = "no faction aggression";
			return false;
		}

		int threatScore = enemyDirector.GetRecentDamageScore(state, zone.m_sOwnerFactionKey, zone.m_sZoneId);
		if (threatScore <= 0)
		{
			reason = "no recent threat signal";
			return false;
		}

		int chance = ResolveLegacyQRFChance(state, zone, pool.m_iAggression, threatScore);
		int roll = ResolveLegacyQRFRoll(state, zone);
		if (roll >= chance)
		{
			reason = string.Format("qrf chance rejected | roll %1 chance %2 aggression %3 threat %4", roll, chance, pool.m_iAggression, threatScore);
			cooldownOnSkip = true;
			return false;
		}

		reason = string.Format("qrf chance accepted | roll %1 chance %2 aggression %3 threat %4", roll, chance, pool.m_iAggression, threatScore);
		return true;
	}

	protected int ResolveLegacyQRFChance(HST_CampaignState state, HST_ZoneState zone, int aggression, int threatScore)
	{
		int chance = 10;
		chance += Math.Min(35, Math.Max(0, aggression / 2));
		chance += Math.Min(25, Math.Max(0, threatScore * 3));
		if (state)
			chance += Math.Max(0, state.m_iWarLevel - 1) * 3;
		if (zone)
			chance += Math.Min(10, Math.Max(0, zone.m_iPriority / 4));

		return Math.Max(5, Math.Min(85, chance));
	}

	protected int ResolveLegacyQRFRoll(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state)
			return 99;

		int qrfBucket = Math.Max(0, state.m_iElapsedSeconds / QRF_CHANCE_REJECT_COOLDOWN_SECONDS);
		int seed = state.m_iCampaignSeed + qrfBucket * 173 + state.m_iWarLevel * 19 + state.m_aQRFs.Count() * 29;
		if (zone)
			seed += zone.m_sZoneId.Length() * 103 + zone.m_sDisplayName.Length() * 41 + zone.m_iPriority * 23 + Math.Round(zone.m_vPosition[0]) + Math.Round(zone.m_vPosition[2]);

		return HST_DefaultCatalog.PositiveMod(seed, 100);
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
				Print(string.Format("Partisan | QRF %1 failed to stage near objective: missing target zone %2", qrf.m_sInstanceId, qrf.m_sTargetZoneId), LogLevel.WARNING);
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
				Print(string.Format("Partisan | QRF %1 failed to materialize near %2 | prefab %3 | reason %4", qrf.m_sInstanceId, targetZone.m_sZoneId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
				changed = true;
				continue;
			}

			qrf.m_sGroupId = activeGroup.m_sGroupId;
			Print(string.Format("Partisan | QRF %1 spawned near %2 | source %3 | spawn %4 | objective %5 | group %6", qrf.m_sInstanceId, targetZone.m_sZoneId, qrf.m_sSourceZoneId, activeGroup.m_vPosition, targetPosition, activeGroup.m_sGroupId));
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
			if (FailTerminalLinkedQRF(state, qrf, "arrival resolution"))
			{
				changed = true;
				continue;
			}

			if (state.m_iElapsedSeconds < qrf.m_iStartedAtSecond + qrf.m_iETASeconds)
				continue;

			if (qrf.m_sGroupId.IsEmpty())
			{
				qrf.m_bResolved = true;
				qrf.m_bSucceeded = false;
				m_bMarkerRefreshNeeded = true;
				changed = true;
				Print(string.Format("Partisan | QRF %1 failed to reach zone %2 | no physical group materialized", qrf.m_sInstanceId, qrf.m_sTargetZoneId), LogLevel.WARNING);
				continue;
			}

			HST_ActiveGroupState activeGroup = state.FindActiveGroup(qrf.m_sGroupId);
			if (activeGroup && state.m_iElapsedSeconds < activeGroup.m_iSpawnedAtSecond + ROUTE_STATE_UPDATE_SECONDS)
				continue;

			bool combatEffective;
			string combatPresenceReason;
			if (activeGroup && !m_CombatPresence.TryResolveGroupCombatPresence(
				state,
				activeGroup,
				combatEffective,
				combatPresenceReason))
			{
				DebugLog(string.Format(
					"QRF %1 arrival waits for combat-presence authority | group %2 | reason %3",
					qrf.m_sInstanceId,
					qrf.m_sGroupId,
					combatPresenceReason));
				continue;
			}

			qrf.m_bResolved = true;
			if (!activeGroup || !combatEffective)
			{
				qrf.m_bSucceeded = false;
				m_bMarkerRefreshNeeded = true;
				changed = true;
				Print(string.Format("Partisan | QRF %1 failed to reach zone %2 | group %3 status %4 spawned agents %5 alive %6", qrf.m_sInstanceId, qrf.m_sTargetZoneId, qrf.m_sGroupId, ResolveActiveGroupStatus(activeGroup), ResolveSpawnedAgentTotal(activeGroup), ResolveAliveAgentTotal(activeGroup)), LogLevel.WARNING);
				continue;
			}

			qrf.m_bSucceeded = true;
			if (activeGroup)
				activeGroup.m_sRuntimeStatus = "arrived";
			changed = true;
			Print(string.Format("Partisan | QRF %1 active near zone %2", qrf.m_sInstanceId, qrf.m_sTargetZoneId));
		}

		return changed;
	}

	protected bool FailTerminalLinkedQRF(HST_CampaignState state, HST_QRFState qrf, string source)
	{
		if (!state || !qrf || qrf.m_bResolved || qrf.m_sGroupId.IsEmpty())
			return false;

		HST_ActiveGroupState activeGroup = state.FindActiveGroup(qrf.m_sGroupId);
		if (!activeGroup || !IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;

		qrf.m_bResolved = true;
		qrf.m_bSucceeded = false;
		m_bMarkerRefreshNeeded = true;
		Print(string.Format("Partisan | QRF %1 failed because linked group %2 became terminal before completion | status %3 | source %4", qrf.m_sInstanceId, qrf.m_sGroupId, activeGroup.m_sRuntimeStatus, source), LogLevel.WARNING);
		return true;
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

		Print(string.Format("Partisan | QRF %1 could not find dry staging near %2, using objective fallback %3", qrf.m_sInstanceId, targetZone.m_sZoneId, targetPosition), LogLevel.WARNING);
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
			seed += state.m_iCampaignSeed * 17 + state.m_iElapsedSeconds * 5 + state.CountOperationalActiveGroups() * 61;
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

		CleanupActiveGroupRouteProgressStatuses(state);
		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || !state.IsOperationalActiveGroup(activeGroup)
				|| (activeGroup.m_sRuntimeStatus != "routing" && activeGroup.m_sRuntimeStatus != "support_active" && activeGroup.m_sRuntimeStatus != "support_recalling"))
				continue;
			if (ShouldHoldForceSpawnProjection(state, activeGroup))
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;

			ref array<vector> routePositions = BuildActiveGroupRoutePositions(state, ResolveActiveGroupGeneratedRoute(state, activeGroup), activeGroup);
			bool simulateUnspawnedRoute = CanSimulateUnspawnedActiveGroupRoute(activeGroup);
			bool physicalConfirmedRoute = activeGroup.m_bSpawnedEntity
				&& activeGroup.m_iInfantryCount > 0
				&& !IsMissionConvoyGroup(activeGroup)
				&& IsLiveConfirmedOperationRouteGroup(state, activeGroup);
			if (physicalConfirmedRoute)
			{
				changed = UpdatePhysicalSupportActiveGroupRoute(state, activeGroup, routePositions) || changed;
				continue;
			}

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

			if (!activeGroup.m_bSpawnedEntity && !simulateUnspawnedRoute)
				continue;

			if (activeGroup.m_bSpawnedEntity
				&& !IsExactGarrisonPatrolGroup(state, activeGroup)
				&& ApplyResponseGroupMovementSpeed(activeGroup, AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId))))
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
			if (activeGroup.m_bSpawnedEntity && !IsActiveGroupInfantryWaypointAssigned(activeGroup) && activeGroup.m_iAssignedWaypointCount != assignedWaypointCount)
			{
				activeGroup.m_iAssignedWaypointCount = assignedWaypointCount;
				changed = true;
			}
			if (DistanceSq2D(activeGroup.m_vPosition, position) < 25)
			{
				if (progress >= 1.0 && activeGroup.m_sRuntimeStatus != arrivedStatus)
				{
					activeGroup.m_sRuntimeStatus = arrivedStatus;
					if (IsSupportRequestActiveGroup(activeGroup))
						m_bMarkerRefreshNeeded = true;
					changed = true;
				}
				continue;
			}

			activeGroup.m_vPosition = position;
			if (activeGroup.m_bSpawnedEntity && !IsActiveGroupInfantryWaypointAssigned(activeGroup))
				SetRuntimeGroupEntitiesOrigin(activeGroup.m_sGroupId, position);

			if (progress >= 1.0)
				activeGroup.m_sRuntimeStatus = arrivedStatus;
			if (IsSupportRequestActiveGroup(activeGroup))
				m_bMarkerRefreshNeeded = true;

			changed = true;
		}

		return changed;
	}

	protected bool UpdatePhysicalSupportActiveGroupRoute(HST_CampaignState state, HST_ActiveGroupState activeGroup, array<vector> routePositions)
	{
		if (!state || !activeGroup)
			return false;

		vector livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, false);
		if (IsZeroVector(livePosition))
			return false;

		bool changed;
		if (IsZeroVector(activeGroup.m_vPosition) || DistanceSq2D(activeGroup.m_vPosition, livePosition) >= 4.0)
		{
			activeGroup.m_vPosition = livePosition;
			m_bMarkerRefreshNeeded = true;
			changed = true;
		}

		HST_ActiveGroupRouteProgressStatus progress = EnsureActiveGroupRouteProgressStatus(activeGroup);
		if (!progress)
			return changed;
		ResetActiveGroupRouteProgressForCurrentLeg(progress, activeGroup, state.m_iElapsedSeconds);

		float distanceToTargetMeters = Math.Sqrt(DistanceSq2D(livePosition, activeGroup.m_vTargetPosition));
		bool newTimedSample = progress.m_iLastSampleSecond < state.m_iElapsedSeconds;
		if (newTimedSample)
		{
			if (progress.m_iLastSampleSecond < 0)
			{
				progress.m_iLastProgressSecond = state.m_iElapsedSeconds;
				progress.m_fBestDistanceToTargetMeters = distanceToTargetMeters;
			}
			else if (progress.m_fBestDistanceToTargetMeters < 0
				|| progress.m_fBestDistanceToTargetMeters - distanceToTargetMeters >= ACTIVE_GROUP_ROUTE_PROGRESS_THRESHOLD_METERS)
			{
				progress.m_iLastProgressSecond = state.m_iElapsedSeconds;
				progress.m_iRouteReissueAttemptCount = 0;
				progress.m_fBestDistanceToTargetMeters = distanceToTargetMeters;
			}
			progress.m_vLastSamplePosition = livePosition;
			progress.m_fLastDistanceToTargetMeters = distanceToTargetMeters;
			progress.m_iLastSampleSecond = state.m_iElapsedSeconds;
		}

		if (distanceToTargetMeters <= ACTIVE_GROUP_ROUTE_ARRIVAL_RADIUS_METERS)
		{
			if (newTimedSample)
				progress.m_iArrivalSampleCount++;
			if (progress.m_iArrivalSampleCount < ACTIVE_GROUP_ROUTE_ARRIVAL_SAMPLE_COUNT)
				return changed;

			string arrivedStatus = ResolveArrivedRouteStatus(activeGroup);
			if (activeGroup.m_sRuntimeStatus != arrivedStatus)
			{
				activeGroup.m_sRuntimeStatus = arrivedStatus;
				activeGroup.m_sSpawnFailureReason = string.Format("Physical support route completion confirmed by %1 consecutive live-position samples within %2m; final distance %3m.", ACTIVE_GROUP_ROUTE_ARRIVAL_SAMPLE_COUNT, Math.Round(ACTIVE_GROUP_ROUTE_ARRIVAL_RADIUS_METERS), Math.Round(distanceToTargetMeters));
				m_bMarkerRefreshNeeded = true;
				changed = true;
			}
			return changed;
		}
		progress.m_iArrivalSampleCount = 0;

		bool routeAssigned = IsActiveGroupInfantryWaypointAssigned(activeGroup);
		int lastProgressAge = Math.Max(0, state.m_iElapsedSeconds - progress.m_iLastProgressSecond);
		int lastReissueAge = ACTIVE_GROUP_ROUTE_REISSUE_COOLDOWN_SECONDS;
		if (progress.m_iLastRouteReissueSecond >= 0)
			lastReissueAge = Math.Max(0, state.m_iElapsedSeconds - progress.m_iLastRouteReissueSecond);
		bool stalled = lastProgressAge >= ACTIVE_GROUP_ROUTE_STALL_REISSUE_SECONDS;
		bool initialAssignment = !routeAssigned && progress.m_iLastRouteReissueSecond < 0;
		bool canRetry = stalled
			&& lastReissueAge >= ACTIVE_GROUP_ROUTE_REISSUE_COOLDOWN_SECONDS
			&& progress.m_iRouteReissueAttemptCount < ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS;
		if (!initialAssignment && !canRetry)
		{
			if (stalled && progress.m_iRouteReissueAttemptCount >= ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS)
			{
				string cappedReason = string.Format("Physical support route remains en route after %1 bounded waypoint reissue attempts; arrival requires live distance proof.", ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS);
				if (activeGroup.m_sSpawnFailureReason != cappedReason)
				{
					activeGroup.m_sSpawnFailureReason = cappedReason;
					changed = true;
				}
			}
			return changed;
		}

		bool assignedFinalSweepWaypoint;
		int assignedWaypointCount = AssignActiveGroupInfantryRouteWaypoints(activeGroup, routePositions, assignedFinalSweepWaypoint);
		progress.m_iLastRouteReissueSecond = state.m_iElapsedSeconds;
		if (!initialAssignment)
			progress.m_iRouteReissueAttemptCount++;
		if (assignedWaypointCount <= 1)
		{
			activeGroup.m_iAssignedWaypointCount = 0;
			if (initialAssignment)
				progress.m_iRouteReissueAttemptCount = 1;
			activeGroup.m_sSpawnFailureReason = string.Format("Physical support route waypoint assignment pending; attempt %1/%2 and arrival still requires live distance proof.", progress.m_iRouteReissueAttemptCount, ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS);
			return true;
		}

		activeGroup.m_iAssignedWaypointCount = assignedWaypointCount;
		activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "infantry_waypoints");
		if (assignedFinalSweepWaypoint)
			activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "infantry_sweep");
		if (initialAssignment)
			activeGroup.m_sSpawnFailureReason = string.Format("Assigned physical support waypoint chain %1 from the current live position; final sweep %2.", assignedWaypointCount, ReportBool(assignedFinalSweepWaypoint));
		else
			activeGroup.m_sSpawnFailureReason = string.Format("Reissued stalled physical support waypoint chain %1/%2 from the current live position; assigned %3 and final sweep %4.", progress.m_iRouteReissueAttemptCount, ACTIVE_GROUP_ROUTE_MAX_REISSUE_ATTEMPTS, assignedWaypointCount, ReportBool(assignedFinalSweepWaypoint));
		return true;
	}

	protected HST_ActiveGroupRouteProgressStatus EnsureActiveGroupRouteProgressStatus(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup || activeGroup.m_sGroupId.IsEmpty())
			return null;
		foreach (HST_ActiveGroupRouteProgressStatus progress : m_aActiveGroupRouteProgressStatuses)
		{
			if (progress && progress.m_sGroupId == activeGroup.m_sGroupId)
				return progress;
		}

		HST_ActiveGroupRouteProgressStatus created = new HST_ActiveGroupRouteProgressStatus();
		created.m_sGroupId = activeGroup.m_sGroupId;
		m_aActiveGroupRouteProgressStatuses.Insert(created);
		return created;
	}

	protected void ResetActiveGroupRouteProgressForCurrentLeg(HST_ActiveGroupRouteProgressStatus progress, HST_ActiveGroupState activeGroup, int elapsedSeconds)
	{
		if (!progress || !activeGroup)
			return;
		if (progress.m_sRouteStatus == activeGroup.m_sRuntimeStatus
			&& !IsZeroVector(progress.m_vTargetPosition)
			&& DistanceSq2D(progress.m_vTargetPosition, activeGroup.m_vTargetPosition) < 4.0)
			return;

		progress.m_sRouteStatus = activeGroup.m_sRuntimeStatus;
		progress.m_vTargetPosition = activeGroup.m_vTargetPosition;
		progress.m_vLastSamplePosition = "0 0 0";
		progress.m_fLastDistanceToTargetMeters = -1.0;
		progress.m_fBestDistanceToTargetMeters = -1.0;
		progress.m_iLastSampleSecond = -1;
		progress.m_iLastProgressSecond = elapsedSeconds;
		progress.m_iLastRouteReissueSecond = -1;
		progress.m_iRouteReissueAttemptCount = 0;
		progress.m_iArrivalSampleCount = 0;
	}

	protected void CleanupActiveGroupRouteProgressStatuses(HST_CampaignState state)
	{
		for (int i = m_aActiveGroupRouteProgressStatuses.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupRouteProgressStatus progress = m_aActiveGroupRouteProgressStatuses[i];
			HST_ActiveGroupState activeGroup;
			if (state && progress)
				activeGroup = state.FindActiveGroup(progress.m_sGroupId);
			if (activeGroup && IsLiveConfirmedOperationRouteGroup(state, activeGroup)
				&& (activeGroup.m_sRuntimeStatus == "support_active" || activeGroup.m_sRuntimeStatus == "support_recalling"
					|| activeGroup.m_sRuntimeStatus == "routing"))
				continue;
			m_aActiveGroupRouteProgressStatuses.Remove(i);
		}
	}

	protected int ResolveRouteDurationSeconds(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return QRF_ETA_SECONDS;

		if (state && activeGroup.m_sRuntimeStatus == "support_recalling")
		{
			float recallDistance = Math.Sqrt(DistanceSq2D(activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition));
			return Math.Max(45, Math.Round(recallDistance / 8.0));
		}

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
		if (activeGroup && activeGroup.m_sRuntimeStatus == "support_recalling")
			return "support_recall_exited";

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
		if (!Replication.IsServer())
			return 0;

		AIGroup group = AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId));
		if (!group)
			return 0;
		RplComponent groupReplication = RplComponent.Cast(group.FindComponent(RplComponent));
		if (groupReplication && !groupReplication.IsMaster())
			return 0;

		ResourceName waypointResource = ACTIVE_GROUP_ROUTE_WAYPOINT_PREFAB;
		Resource loaded = Resource.Load(waypointResource);
		if (!loaded || !loaded.IsValid())
			return 0;
		bool exactGarrisonPatrol = IsExactGarrisonPatrolGroup(null, activeGroup);
		if (!exactGarrisonPatrol)
		{
			ResourceName sweepWaypointResource = ACTIVE_GROUP_ROUTE_SWEEP_WAYPOINT_PREFAB;
			Resource sweepLoaded = Resource.Load(sweepWaypointResource);
			if (!sweepLoaded || !sweepLoaded.IsValid())
				return 0;
		}

		array<IEntity> preparedEntities = {};
		array<AIWaypoint> preparedWaypoints = {};
		bool preparedFinalSweepWaypoint;
		for (int i = 1; i < routePositions.Count(); i++)
		{
			vector waypointPosition = routePositions[i];
			if (IsZeroVector(waypointPosition))
				continue;
			if (DistanceSq2D(activeGroup.m_vPosition, waypointPosition) < 16.0)
				continue;

			bool finalSweepWaypoint = !exactGarrisonPatrol
				&& i == routePositions.Count() - 1
				&& activeGroup.m_sRuntimeStatus != "support_recalling";
			IEntity waypointEntity = SpawnActiveGroupRouteWaypoint(activeGroup.m_sGroupId, waypointPosition, preparedEntities.Count() + 1, finalSweepWaypoint);
			AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
			if (!waypoint)
				continue;

			preparedEntities.Insert(waypointEntity);
			preparedWaypoints.Insert(waypoint);
			if (finalSweepWaypoint)
				preparedFinalSweepWaypoint = true;
		}

		if (preparedWaypoints.Count() <= 1)
		{
			foreach (IEntity preparedEntity : preparedEntities)
			{
				if (preparedEntity)
					SCR_EntityHelper.DeleteEntityAndChildren(preparedEntity);
			}
			return 0;
		}

		DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);
		for (int preparedIndex = 0; preparedIndex < preparedWaypoints.Count(); preparedIndex++)
		{
			group.AddWaypoint(preparedWaypoints[preparedIndex]);
			m_aRuntimeGroupWaypointIds.Insert(activeGroup.m_sGroupId);
			m_aRuntimeGroupWaypointEntities.Insert(preparedEntities[preparedIndex]);
		}
		assignedFinalSweepWaypoint = preparedFinalSweepWaypoint;

		if (!exactGarrisonPatrol && ApplyResponseGroupMovementSpeed(activeGroup, group))
			activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "response_run");

		return preparedWaypoints.Count();
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
		waypoint.SetCompletionType(EAIWaypointCompletionType.Leader);
		return waypointEntity;
	}

	protected bool IsActiveGroupInfantryWaypointAssigned(HST_ActiveGroupState activeGroup)
	{
		return activeGroup
			&& activeGroup.m_iAssignedWaypointCount > 1
			&& activeGroup.m_sSpawnFallbackMode.Contains("infantry_waypoints")
			&& CountRuntimeGroupWaypointEntities(activeGroup.m_sGroupId) > 1;
	}

	protected int CountRuntimeGroupWaypointEntities(string groupId)
	{
		if (groupId.IsEmpty())
			return 0;

		int count;
		for (int i = 0; i < m_aRuntimeGroupWaypointIds.Count(); i++)
		{
			if (m_aRuntimeGroupWaypointIds[i] != groupId || i >= m_aRuntimeGroupWaypointEntities.Count())
				continue;
			IEntity waypointEntity = m_aRuntimeGroupWaypointEntities[i];
			if (waypointEntity && !waypointEntity.IsDeleted())
				count++;
		}
		return count;
	}

	protected bool UpdateTownPolicePatrols(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!ShouldAssignTownPolicePatrol(state, preset, activeGroup))
				continue;

			string reason;
			if (AssignTownPolicePatrolWaypoints(state, activeGroup, reason) > 1)
			{
				activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "town_police_patrol");
				activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "patrol_cycle");
				activeGroup.m_sSpawnFailureReason = reason;
				changed = true;
			}
			else if (!reason.IsEmpty())
			{
				AppendActiveGroupSpawnFailureNote(activeGroup, reason);
				changed = true;
			}
		}

		return changed;
	}

	protected bool UpdateZoneGarrisonPatrols(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!ShouldAssignZoneGarrisonPatrol(state, preset, activeGroup))
				continue;

			string reason;
			if (AssignTownPolicePatrolWaypoints(state, activeGroup, reason) > 1)
			{
				activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "zone_garrison_patrol");
				activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "patrol_cycle");
				activeGroup.m_sSpawnFailureReason = reason;
				changed = true;
			}
			else if (!reason.IsEmpty())
			{
				AppendActiveGroupSpawnFailureNote(activeGroup, reason);
				changed = true;
			}
		}

		return changed;
	}

	protected bool ShouldAssignZoneGarrisonPatrol(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return false;
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		if (IsLocalSecurityPatrolClaimant(state, activeGroup))
			return false;
		if (ShouldHoldForceSpawnProjection(state, activeGroup))
			return false;
		if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
			return false;
		if (!activeGroup.m_bSpawnedEntity || activeGroup.m_iInfantryCount <= 0 || activeGroup.m_iVehicleCount > 0)
			return false;
		if (activeGroup.m_bQRF || IsMissionConvoyGroup(activeGroup) || IsSupportRequestActiveGroup(activeGroup))
			return false;
		if (!activeGroup.m_sMissionInstanceId.IsEmpty())
			return false;
		if (IsTownPoliceActiveGroup(state, preset, activeGroup))
			return false;
		if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;
		if (IsZoneGarrisonPatrolAssigned(activeGroup))
			return false;
		if (activeGroup.m_sRuntimeStatus == "routing" || activeGroup.m_sRuntimeStatus == "support_active" || activeGroup.m_sRuntimeStatus == "support_recalling")
			return false;
		if (CountAliveRuntimeGroupAgents(activeGroup.m_sGroupId) <= 0)
			return false;

		HST_ZoneState zone = state.FindZone(activeGroup.m_sZoneId);
		if (!zone || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE || zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT)
			return false;

		return true;
	}

	protected bool IsZoneGarrisonPatrolAssigned(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;
		if (activeGroup.m_iAssignedWaypointCount <= 1)
			return false;
		if (!activeGroup.m_sSpawnFallbackMode.Contains("zone_garrison_patrol"))
			return false;
		if (!activeGroup.m_sSpawnFallbackMode.Contains("patrol_cycle"))
			return false;

		return true;
	}

	protected bool ShouldAssignTownPolicePatrol(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveGroupState activeGroup)
	{
		if (!IsTownPoliceActiveGroup(state, preset, activeGroup))
			return false;
		if (ShouldHoldForceSpawnProjection(state, activeGroup))
			return false;
		if (!activeGroup.m_bSpawnedEntity || activeGroup.m_iInfantryCount <= 0)
			return false;
		if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;
		if (IsTownPolicePatrolAssigned(activeGroup))
			return false;
		if (CountAliveRuntimeGroupAgents(activeGroup.m_sGroupId) <= 0)
			return false;

		return true;
	}

	protected bool IsTownPoliceActiveGroup(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveGroupState activeGroup)
	{
		if (!state)
			return false;
		if (!activeGroup)
			return false;
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		if (IsLocalSecurityPatrolClaimant(state, activeGroup)
			&& !IsExactOpenPhysicalLocalSecurityPatrolGroup(state, activeGroup))
			return false;
		if (activeGroup.m_bQRF)
			return false;
		if (IsMissionConvoyGroup(activeGroup))
			return false;
		if (IsSupportRequestActiveGroup(activeGroup))
			return false;
		if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
			return false;
		if (activeGroup.m_iInfantryCount <= 0)
			return false;
		if (activeGroup.m_iVehicleCount > 0)
			return false;

		HST_ZoneState zone = state.FindZone(activeGroup.m_sZoneId);
		if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return false;
		if (!HST_FactionRelationService.IsEnemyFaction(preset, activeGroup.m_sFactionKey))
			return false;

		string prefab = activeGroup.m_sPrefab;
		prefab.ToLower();
		if (prefab.Contains("townpolice"))
			return true;
		if (prefab.Contains("town_police"))
			return true;

		return false;
	}

	protected bool IsTownPolicePatrolAssigned(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;
		if (activeGroup.m_iAssignedWaypointCount <= 1)
			return false;
		if (!activeGroup.m_sSpawnFallbackMode.Contains("town_police_patrol"))
			return false;
		if (!activeGroup.m_sSpawnFallbackMode.Contains("patrol_cycle"))
			return false;

		return true;
	}

	protected int AssignTownPolicePatrolWaypoints(HST_CampaignState state, HST_ActiveGroupState activeGroup, out string reason)
	{
		reason = "";
		if (!state)
			return 0;
		if (!activeGroup)
			return 0;

		AIGroup group = AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId));
		if (!group)
		{
			reason = "Town police patrol pending: runtime AI group missing.";
			return 0;
		}

		ResourceName waypointResource = ACTIVE_GROUP_ROUTE_WAYPOINT_PREFAB;
		Resource loaded = Resource.Load(waypointResource);
		if (!loaded)
		{
			reason = "Town police patrol unavailable: patrol waypoint prefab missing.";
			return 0;
		}
		if (!loaded.IsValid())
		{
			reason = "Town police patrol unavailable: patrol waypoint prefab missing.";
			return 0;
		}

		ResourceName cycleResource = TOWN_POLICE_PATROL_CYCLE_WAYPOINT_PREFAB;
		Resource cycleLoaded = Resource.Load(cycleResource);
		if (!cycleLoaded)
		{
			reason = "Town police patrol unavailable: cycle waypoint prefab missing.";
			return 0;
		}
		if (!cycleLoaded.IsValid())
		{
			reason = "Town police patrol unavailable: cycle waypoint prefab missing.";
			return 0;
		}

		ref array<vector> patrolPositions = BuildTownPolicePatrolPositions(state, activeGroup);
		if (!patrolPositions)
		{
			reason = "Town police patrol unavailable: no patrol positions.";
			return 0;
		}
		if (patrolPositions.Count() < 2)
		{
			reason = "Town police patrol unavailable: no patrol positions.";
			return 0;
		}

		DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);

		array<IEntity> spawnedEntities = {};
		array<AIWaypoint> waypoints = {};
		for (int i = 0; i < patrolPositions.Count(); i++)
		{
			vector waypointPosition = HST_WorldPositionService.ResolveSafeGroundPosition(patrolPositions[i], HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 6.0);
			GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(ACTIVE_GROUP_ROUTE_WAYPOINT_PREFAB, waypointPosition, "0 0 0");
			AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
			if (!waypoint)
			{
				if (waypointEntity)
					SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
				DeleteTownPolicePatrolSpawnedEntities(spawnedEntities);
				reason = "Town police patrol failed while creating patrol waypoint.";
				return 0;
			}

			ApplyCampaignDebugEntityName(waypointEntity, string.Format("town_police_patrol_%1", i + 1), activeGroup.m_sGroupId);
			waypoint.SetCompletionRadius(TOWN_POLICE_PATROL_WAYPOINT_RADIUS_METERS);
			waypoints.Insert(waypoint);
			spawnedEntities.Insert(waypointEntity);
		}

		GenericEntity cycleEntity = HST_WorldPositionService.SpawnPrefab(TOWN_POLICE_PATROL_CYCLE_WAYPOINT_PREFAB, patrolPositions[0], "0 0 0");
		AIWaypointCycle waypointCycle = AIWaypointCycle.Cast(cycleEntity);
		if (!waypointCycle)
		{
			if (cycleEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(cycleEntity);
			DeleteTownPolicePatrolSpawnedEntities(spawnedEntities);
			reason = "Town police patrol failed while creating cycle waypoint.";
			return 0;
		}

		ApplyCampaignDebugEntityName(cycleEntity, "town_police_patrol_cycle", activeGroup.m_sGroupId);
		waypointCycle.SetWaypoints(waypoints);
		foreach (IEntity spawnedEntity : spawnedEntities)
		{
			m_aRuntimeGroupWaypointIds.Insert(activeGroup.m_sGroupId);
			m_aRuntimeGroupWaypointEntities.Insert(spawnedEntity);
		}
		m_aRuntimeGroupWaypointIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeGroupWaypointEntities.Insert(cycleEntity);
		group.AddWaypoint(waypointCycle);
		group.ActivateAllMembers();
		activeGroup.m_iAssignedWaypointCount = waypoints.Count();
		reason = string.Format("Assigned town police patrol cycle waypoints %1.", waypoints.Count());
		return waypoints.Count();
	}

	protected void DeleteTownPolicePatrolSpawnedEntities(array<IEntity> spawnedEntities)
	{
		if (!spawnedEntities)
			return;

		foreach (IEntity spawnedEntity : spawnedEntities)
		{
			if (spawnedEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(spawnedEntity);
		}
	}

	protected ref array<vector> BuildTownPolicePatrolPositions(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		ref array<vector> positions = {};
		if (!state || !activeGroup)
			return positions;

		HST_ZoneState zone = state.FindZone(activeGroup.m_sZoneId);
		if (!zone)
			return positions;

		HST_GeneratedRouteState route = ResolveTownPolicePatrolRoute(state, zone, activeGroup);
		if (route)
		{
			AppendTownPolicePatrolPosition(positions, activeGroup.m_vPosition);
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

				AppendTownPolicePatrolPosition(positions, selectedWaypoint.m_vPosition);
				lastIndex = selectedWaypoint.m_iIndex;
			}
		}

		if (positions.Count() >= 3)
			return positions;

		positions.Clear();
		BuildFallbackTownPolicePatrolPositions(zone, activeGroup, positions);
		return positions;
	}

	protected HST_GeneratedRouteState ResolveTownPolicePatrolRoute(HST_CampaignState state, HST_ZoneState zone, HST_ActiveGroupState activeGroup)
	{
		if (!state)
			return null;
		if (!zone)
			return null;
		if (!activeGroup)
			return null;

		if (!activeGroup.m_sRouteId.IsEmpty())
		{
			HST_GeneratedRouteState activeRoute = state.FindGeneratedRoute(activeGroup.m_sRouteId);
			if (activeRoute && activeRoute.m_aWaypoints.Count() >= 2)
				return activeRoute;
		}

		if (!zone.m_sPatrolRouteId.IsEmpty())
		{
			HST_GeneratedRouteState patrolRoute = state.FindGeneratedRoute(zone.m_sPatrolRouteId);
			if (patrolRoute && patrolRoute.m_aWaypoints.Count() >= 2)
				return patrolRoute;
		}

		return null;
	}

	protected void BuildFallbackTownPolicePatrolPositions(HST_ZoneState zone, HST_ActiveGroupState activeGroup, array<vector> positions)
	{
		if (!zone)
			return;
		if (!activeGroup)
			return;
		if (!positions)
			return;

		int seed = zone.m_sZoneId.Length() * 37 + activeGroup.m_sGroupId.Length() * 13 + Math.Round(activeGroup.m_vPosition[0]) + Math.Round(activeGroup.m_vPosition[2]);
		for (int i = 0; i < 4; i++)
		{
			float angle = (seed + i * 89) * 0.0174533;
			float radius = TOWN_POLICE_PATROL_FALLBACK_RADIUS_METERS;
			if (i == 1)
				radius = radius + 20.0;
			else if (i == 3)
				radius = radius + 20.0;
			vector offset = "0 0 0";
			offset[0] = Math.Sin(angle) * radius;
			offset[2] = Math.Cos(angle) * radius;
			AppendTownPolicePatrolPosition(positions, zone.m_vPosition + offset);
		}
	}

	protected void AppendTownPolicePatrolPosition(array<vector> positions, vector position)
	{
		if (!positions || IsZeroVector(position))
			return;

		vector resolved = HST_WorldPositionService.ResolveSafeGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 6.0);
		if (positions.Count() > 0 && DistanceSq2D(positions[positions.Count() - 1], resolved) < 25.0)
			return;

		positions.Insert(resolved);
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

	protected ref array<vector> BuildActiveGroupRoutePositions(
		HST_CampaignState state,
		HST_GeneratedRouteState route,
		HST_ActiveGroupState activeGroup)
	{
		ref array<vector> positions = {};
		if (!activeGroup)
			return positions;
		if (IsLiveConfirmedOperationRouteGroup(state, activeGroup))
			return BuildDirectSupportRoutePositions(activeGroup);
		if (!route)
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

	protected ref array<vector> BuildDirectSupportRoutePositions(HST_ActiveGroupState activeGroup)
	{
		ref array<vector> positions = {};
		if (!activeGroup)
			return positions;

		vector sourcePosition = activeGroup.m_vPosition;
		if (IsZeroVector(sourcePosition))
			sourcePosition = activeGroup.m_vSourcePosition;
		vector targetPosition = activeGroup.m_vTargetPosition;
		AppendActiveGroupRoutePosition(positions, sourcePosition);
		if (!IsZeroVector(sourcePosition) && !IsZeroVector(targetPosition) && DistanceSq2D(sourcePosition, targetPosition) > 64.0)
		{
			vector midpoint = LerpPosition(sourcePosition, targetPosition, 0.5);
			midpoint = HST_WorldPositionService.ResolveSafeGroundPosition(midpoint, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 6.0);
			AppendActiveGroupRoutePosition(positions, midpoint);
		}
		AppendActiveGroupRoutePosition(positions, targetPosition);
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
		if (qrf)
			activeGroup.m_sOperationId = HST_StableIdService.BuildOperationId("qrf", activeGroup.m_sGroupId);
		else
			activeGroup.m_sOperationId = HST_StableIdService.BuildOperationId("garrison", zone.m_sZoneId);
		activeGroup.m_sZoneId = zone.m_sZoneId;
		activeGroup.m_sFactionKey = factionKey;
		if (!qrf)
			activeGroup.m_sGarrisonZoneId = zone.m_sZoneId;
		activeGroup.m_sPrefab = SelectGroupPrefab(state, zone, factionKey, qrf, preset, infantryCount);
		activeGroup.m_sRouteId = ResolveGroupRouteId(zone, qrf);
		bool townPoliceRouteEligible = !qrf && zone != null;
		if (zone)
			townPoliceRouteEligible = townPoliceRouteEligible && zone.m_eType == HST_EZoneType.HST_ZONE_TOWN;
		townPoliceRouteEligible = townPoliceRouteEligible && infantryCount > 0;
		townPoliceRouteEligible = townPoliceRouteEligible && HST_FactionRelationService.IsEnemyFaction(preset, factionKey);
		if (zone)
			townPoliceRouteEligible = townPoliceRouteEligible && !zone.m_sPatrolRouteId.IsEmpty();
		if (townPoliceRouteEligible)
			activeGroup.m_sRouteId = ResolveTownPoliceGroupRouteId(zone, activeGroup.m_sRouteId);
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
		ApplyTrainingQualitySummaryToActiveGroup(activeGroup, state, preset);
		return activeGroup;
	}

	protected void ApplyTrainingQualitySummaryToActiveGroup(HST_ActiveGroupState activeGroup, HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!activeGroup || !state)
			return;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;
		if (activeGroup.m_sFactionKey != resistanceFactionKey)
			return;

		int qualityBonus = HST_RecruitmentService.ResolveTrainingQualityBonusPercentForLevel(state.m_iTrainingLevel);
		int effectiveInfantry = HST_RecruitmentService.ResolveTrainingEffectiveInfantryStrengthForLevel(activeGroup.m_iInfantryCount, state.m_iTrainingLevel);
		activeGroup.m_iCompositionManpower = Math.Max(activeGroup.m_iCompositionManpower, activeGroup.m_iInfantryCount);
		if (activeGroup.m_sCompositionIntentId.IsEmpty())
			activeGroup.m_sCompositionIntentId = HST_ForceCompositionService.INTENT_GARRISON;
		if (activeGroup.m_sCompositionTier.IsEmpty())
			activeGroup.m_sCompositionTier = string.Format("training_%1", Math.Max(1, state.m_iTrainingLevel));
		string trainingSummary = string.Format("training %1 | quality +%2 pct | effective infantry %3/%4", Math.Max(1, state.m_iTrainingLevel), qualityBonus, effectiveInfantry, Math.Max(0, activeGroup.m_iInfantryCount));
		if (activeGroup.m_sCompositionSummary.IsEmpty())
			activeGroup.m_sCompositionSummary = trainingSummary;
		else if (!activeGroup.m_sCompositionSummary.Contains("quality"))
			activeGroup.m_sCompositionSummary = activeGroup.m_sCompositionSummary + " | " + trainingSummary;
	}

	protected string BuildGroupId(HST_CampaignState state, HST_ZoneState zone, string factionKey, bool qrf)
	{
		string prefix = "grp";
		if (qrf)
			prefix = "qrf";

		return string.Format("%1_%2_%3_%4_%5", prefix, zone.m_sZoneId, factionKey, state.m_iElapsedSeconds, state.CountOperationalActiveGroups());
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

	protected string SelectGroupPrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, bool qrf, HST_CampaignPreset preset = null, int requestedInfantryCount = 0)
	{
		string role = "garrison";
		if (qrf)
			role = "qrf";
		else
		{
			bool townPoliceRole = zone != null;
			townPoliceRole = townPoliceRole && zone.m_eType == HST_EZoneType.HST_ZONE_TOWN;
			townPoliceRole = townPoliceRole && requestedInfantryCount > 0;
			townPoliceRole = townPoliceRole && HST_FactionRelationService.IsEnemyFaction(preset, factionKey);
			if (townPoliceRole)
				role = "town_police";
		}

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

		if (role == "town_police")
		{
			string townPolicePrefab = HST_DefaultCatalog.ResolveTownPoliceGroupPrefab(factionKey, requestedInfantryCount);
			if (!townPolicePrefab.IsEmpty() && IsValidGroupPrefabResource(townPolicePrefab, factionKey))
				return townPolicePrefab;
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

		Print(string.Format("Partisan | no valid %1 prefab found for faction %2", purpose, factionKey), LogLevel.WARNING);
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
			seed += state.m_iCampaignSeed * 19 + state.m_iElapsedSeconds * 3 + state.CountOperationalActiveGroups() * 97;
		if (zone)
			seed += zone.m_iPriority * 41 + zone.m_sZoneId.Length() * 113 + Math.Round(zone.m_vPosition[0]) + Math.Round(zone.m_vPosition[2]);
		if (qrf)
			seed += 7001;
		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected bool TrySpawnActiveGroup(
		HST_ActiveGroupState activeGroup,
		HST_CampaignState state = null,
		HST_CampaignPreset preset = null,
		bool forceCampaignDebugMaterialization = false)
	{
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		if (IsLocalSecurityPatrolClaimant(state, activeGroup))
			return false;
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
		if (!activeGroup || IsTerminalActiveGroupRuntimeStatus(activeGroup) || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;
		HST_ActiveMissionState exactMission;
		if (state && !activeGroup.m_sMissionInstanceId.IsEmpty())
			exactMission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
		if (IsExactMissionConvoyContract(exactMission))
			return TrySpawnExactMissionConvoyFrozenCrewGroup(state, exactMission, activeGroup);

		if (!forceCampaignDebugMaterialization
			&& ShouldDeferActiveGroupRuntimePhysicalization(state, activeGroup))
		{
			MarkActiveGroupRuntimePhysicalizationDeferred(activeGroup, state);
			return false;
		}

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
			Print(string.Format("Partisan | active group prefab spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
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
		if (IsLocalSecurityPatrolClaimant(state, activeGroup))
			return false;
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
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
		if (!IsRoadblockActiveGroup(activeGroup))
		{
			vehicleAnchor[0] = vehicleAnchor[0] + 8.0;
			vehicleAnchor[2] = vehicleAnchor[2] + 6.0;
		}
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

	protected bool IsRoadblockActiveGroup(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;

		return activeGroup.m_sRuntimeStatus.Contains("roadblock") || activeGroup.m_sSpawnFallbackMode.Contains("roadblock");
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

		string evidence = string.Format("Partisan | active group runtime proof | stage %1 | group %2 | zone %3 owner %4 active %5 | expected %6 | prefab %7",
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
			Print(string.Format("Partisan | active group root faction mismatch %1 expected %2 actual %3 prefab %4", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, ReportText(actualGroupFaction), prefab), LogLevel.WARNING);
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
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return;
		if (IsLocalSecurityPatrolClaimant(state, activeGroup))
		{
			ClearPendingActiveGroupPopulation(activeGroup);
			return;
		}
		HST_ActiveMissionState exactMission;
		if (state && activeGroup)
			exactMission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
		if (IsExactMissionConvoyContract(exactMission))
			return;
		if (IsForceSpawnQueueManaged(activeGroup))
		{
			ClearPendingActiveGroupPopulation(activeGroup);
			return;
		}
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
		string finalPopulationFailure = activeGroup.m_sSpawnFailureReason;
		Print(string.Format("Partisan | active group failed %1 prefab %2: zero agents after grace | reason %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, finalPopulationFailure), LogLevel.WARNING);
		if (IsMissionConvoyGroup(activeGroup))
		{
			HST_ExactMissionConvoyOutboundProjectionTransaction outboundTransaction = FindExactMissionConvoyOutboundProjectionTransaction(activeGroup.m_sMissionInstanceId);
			if (outboundTransaction)
				RollbackExactMissionConvoyOutboundProjectionTransaction(state, outboundTransaction, string.Format("exact outbound root %1 failed asynchronous population: %2", activeGroup.m_sGroupId, finalPopulationFailure), true);
		}
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
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		HST_ActiveMissionState exactMission;
		if (state && activeGroup)
			exactMission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
		if (IsExactMissionConvoyContract(exactMission))
			return false;
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
		if (!activeGroup || activeGroup.m_sRuntimeStatus != "spawn_pending_agents")
			return false;

		IEntity groupEntity = GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId);
		SCR_AIGroup group = SCR_AIGroup.Cast(groupEntity);
		ApplyRuntimeGroupFaction(groupEntity, activeGroup, source, true);
		if (group)
			ReconcileRuntimeGroupEditableMembership(group, activeGroup, source);

		int agentCount = CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId);
		if (agentCount <= 0)
			return false;

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
		HST_ExactMissionConvoyOutboundProjectionTransaction outboundTransaction = FindExactMissionConvoyOutboundProjectionTransaction(activeGroup.m_sMissionInstanceId);
		if (outboundTransaction)
			SetExactMissionConvoyOutboundProjectionTransactionVisible(outboundTransaction, false);
		PrintActiveGroupSpawnEvidence(state, activeGroup, source + "_populated");
		DebugLog(string.Format("active group populated %1 live agents %2 expected %3 via %4", activeGroup.m_sGroupId, agentCount, activeGroup.m_iInfantryCount, source));
		return true;
	}

	protected void RegisterPendingActiveGroupPopulation(IEntity entity, HST_ActiveGroupState activeGroup, string requestedStatus, HST_CampaignState state)
	{
		if (!entity || !activeGroup)
			return;
		HST_ActiveMissionState exactMission;
		if (state)
			exactMission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
		if (IsExactMissionConvoyContract(exactMission))
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
		HST_ActiveMissionState exactMission;
		if (state && activeGroup)
			exactMission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
		if (IsExactMissionConvoyContract(exactMission))
			return;
		if (!TryFinalizeSpawnedGroupAgents(activeGroup, requestedStatus, state, "native delayed spawn event"))
		{
			DebugLog(string.Format("active group delayed spawn event fired before durable live agents were countable %1", groupId));
		}
		HST_ExactMissionConvoyOutboundProjectionTransaction outboundTransaction = FindExactMissionConvoyOutboundProjectionTransaction(activeGroup.m_sMissionInstanceId);
		if (outboundTransaction)
			SetExactMissionConvoyOutboundProjectionTransactionVisible(outboundTransaction, false);
	}

	bool CampaignDebugResolvePendingActiveGroupPopulation(HST_ActiveGroupState activeGroup, HST_CampaignState state, string requestedStatus, out string evidence)
	{
		evidence = "missing group";
		if (!activeGroup)
			return false;
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
		{
			evidence = "exact mission guard population is owned by its force-spawn contract";
			return false;
		}

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

	protected bool CampaignDebugUpdateActiveGroupRouteOnly(
		HST_ActiveGroupState activeGroup,
		HST_CampaignState state,
		HST_CampaignPreset preset)
	{
		if (!activeGroup || !state || activeGroup.m_sGroupId.IsEmpty()
			|| state.FindActiveGroup(activeGroup.m_sGroupId) != activeGroup
			|| !state.IsOperationalActiveGroup(activeGroup)
			|| (activeGroup.m_sRuntimeStatus != "routing"
				&& activeGroup.m_sRuntimeStatus != "support_active"
				&& activeGroup.m_sRuntimeStatus != "support_recalling")
			|| ShouldHoldForceSpawnProjection(state, activeGroup)
			|| IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;

		NormalizeStaticActiveGroupRoute(state, preset, activeGroup);
		ref array<vector> routePositions = BuildActiveGroupRoutePositions(
			state,
			ResolveActiveGroupGeneratedRoute(state, activeGroup),
			activeGroup);
		bool physicalConfirmedRoute = activeGroup.m_bSpawnedEntity
			&& activeGroup.m_iInfantryCount > 0
			&& !IsMissionConvoyGroup(activeGroup)
			&& IsLiveConfirmedOperationRouteGroup(state, activeGroup);
		if (physicalConfirmedRoute)
			return UpdatePhysicalSupportActiveGroupRoute(
				state,
				activeGroup,
				routePositions);
		if (!activeGroup.m_bSpawnedEntity
			|| activeGroup.m_iInfantryCount <= 0
			|| IsMissionConvoyGroup(activeGroup)
			|| IsActiveGroupInfantryWaypointAssigned(activeGroup))
			return false;

		bool assignedFinalSweepWaypoint;
		int assignedWaypointCount = AssignActiveGroupInfantryRouteWaypoints(
			activeGroup,
			routePositions,
			assignedFinalSweepWaypoint);
		if (assignedWaypointCount <= 1)
			return false;
		activeGroup.m_iAssignedWaypointCount = assignedWaypointCount;
		activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(
			activeGroup.m_sSpawnFallbackMode,
			"infantry_waypoints");
		if (assignedFinalSweepWaypoint)
			activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(
				activeGroup.m_sSpawnFallbackMode,
				"infantry_sweep");
		activeGroup.m_sSpawnFailureReason = string.Format(
			"Assigned focused Campaign Debug route waypoint chain %1 | final sweep %2.",
			assignedWaypointCount,
			ReportBool(assignedFinalSweepWaypoint));
		return true;
	}

	bool CampaignDebugResolveActiveGroupRouteAssignment(HST_ActiveGroupState activeGroup, HST_CampaignState state, HST_CampaignPreset preset, string requestedStatus, out string evidence)
	{
		evidence = "missing group or state";
		if (!activeGroup || !state || activeGroup.m_sGroupId.IsEmpty()
			|| state.FindActiveGroup(activeGroup.m_sGroupId) != activeGroup
			|| !state.IsOperationalActiveGroup(activeGroup))
			return false;

		if (requestedStatus.IsEmpty() || requestedStatus == "spawn_pending_agents")
			requestedStatus = ResolvePendingActiveGroupRequestedStatus(activeGroup, "guard_center");
		string beforeStatus = activeGroup.m_sRuntimeStatus;
		string beforeMode = activeGroup.m_sSpawnFallbackMode;
		int beforeWaypoints = activeGroup.m_iAssignedWaypointCount;
		string populationEvidence = "not pending";
		bool populationResolved;
		bool directFallbackResolved;
		bool routeUpdateChanged;
		bool manualRouteAssigned;
		bool assignedFinalSweepWaypoint;
		int manualAssignedWaypoints;
		bool materializationAttempted;
		bool materializationChanged;

		if (!activeGroup.m_bSpawnedEntity
			&& !HasRuntimeGroupEntity(activeGroup.m_sGroupId))
		{
			materializationAttempted = true;
			materializationChanged = TrySpawnActiveGroup(
				activeGroup,
				state,
				preset,
				true);
		}
		populationResolved = activeGroup.m_bSpawnedEntity
			&& CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId) > 0;

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
			routeUpdateChanged = CampaignDebugUpdateActiveGroupRouteOnly(
				activeGroup,
				state,
				preset);
			if (!IsActiveGroupInfantryWaypointAssigned(activeGroup) && activeGroup.m_bSpawnedEntity && activeGroup.m_iInfantryCount > 0 && !IsMissionConvoyGroup(activeGroup) && (activeGroup.m_sRuntimeStatus == "routing" || activeGroup.m_sRuntimeStatus == "support_active"))
			{
				ref array<vector> routePositions = BuildActiveGroupRoutePositions(state, ResolveActiveGroupGeneratedRoute(state, activeGroup), activeGroup);
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
		evidence = evidence + string.Format(" | materialization %1/%2 | population %3 | directFallback %4 | routeUpdate %5",
			ReportBool(materializationAttempted),
			ReportBool(materializationChanged),
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

	bool CampaignDebugResolveTownPolicePatrolAssignment(HST_ActiveGroupState activeGroup, HST_CampaignState state, HST_CampaignPreset preset, string requestedStatus, out string evidence)
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
		bool updateChanged;
		bool manualPatrolAssigned;
		int manualAssignedWaypoints;
		string manualReason;

		if (!populationResolved)
		{
			populationResolved = CampaignDebugResolvePendingActiveGroupPopulation(activeGroup, state, requestedStatus, populationEvidence);
			if (!populationResolved && activeGroup.m_sRuntimeStatus == "spawn_pending_agents")
			{
				directFallbackResolved = TryPopulatePendingActiveGroupFromFactionInfantry(activeGroup, requestedStatus, state, "campaign debug town police patrol", true);
				populationResolved = activeGroup.m_sRuntimeStatus != "spawn_pending_agents" && CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId) > 0;
			}
		}

		if (populationResolved)
		{
			updateChanged = UpdateTownPolicePatrols(state, preset);
			if (!IsTownPolicePatrolAssigned(activeGroup) && ShouldAssignTownPolicePatrol(state, preset, activeGroup))
			{
				manualAssignedWaypoints = AssignTownPolicePatrolWaypoints(state, activeGroup, manualReason);
				if (manualAssignedWaypoints > 1)
				{
					activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "town_police_patrol");
					activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "patrol_cycle");
					activeGroup.m_sSpawnFailureReason = manualReason;
					manualPatrolAssigned = true;
					updateChanged = true;
				}
			}
		}

		bool patrolAssigned = IsTownPolicePatrolAssigned(activeGroup);
		evidence = string.Format("status %1 -> %2 | mode %3 -> %4 | waypoints %5 -> %6",
			ReportText(beforeStatus),
			ReportText(activeGroup.m_sRuntimeStatus),
			ReportText(beforeMode),
			ReportText(activeGroup.m_sSpawnFallbackMode),
			beforeWaypoints,
			activeGroup.m_iAssignedWaypointCount);
		evidence = evidence + string.Format(" | population %1 | directFallback %2 | patrolUpdate %3",
			ReportBool(populationResolved),
			ReportBool(directFallbackResolved),
			ReportBool(updateChanged));
		evidence = evidence + string.Format(" | manualPatrol %1/%2 | reason %3 | %4",
			ReportBool(manualPatrolAssigned),
			manualAssignedWaypoints,
			ReportText(manualReason),
			ReportText(populationEvidence));
		return patrolAssigned;
	}

	bool CampaignDebugIsTownSecurityPoliceProjection(HST_ActiveGroupState activeGroup)
	{
		return IsTownSecurityPoliceProjection(activeGroup);
	}

	bool CampaignDebugDeactivateZoneForRuntimeCleanup(HST_CampaignState state, string zoneId, HST_ZoneCompositionService compositions, out string evidence)
	{
		evidence = "missing state or zone";
		if (!state)
			return false;
		if (zoneId.IsEmpty())
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return false;

		HST_GarrisonState garrisonBefore = state.FindGarrison(zoneId, zone.m_sOwnerFactionKey);
		int infantryBefore;
		int vehiclesBefore;
		if (garrisonBefore)
		{
			infantryBefore = garrisonBefore.m_iInfantryCount;
			vehiclesBefore = garrisonBefore.m_iVehicleCount;
		}
		int projectionBefore = CountTownSecurityPoliceProjections(state, zone);
		bool changed = DeactivateZone(state, zone, compositions);
		HST_GarrisonState garrisonAfter = state.FindGarrison(zoneId, zone.m_sOwnerFactionKey);
		int infantryAfter;
		int vehiclesAfter;
		if (garrisonAfter)
		{
			infantryAfter = garrisonAfter.m_iInfantryCount;
			vehiclesAfter = garrisonAfter.m_iVehicleCount;
		}
		int projectionAfter = CountTownSecurityPoliceProjections(state, zone);
		evidence = string.Format("changed %1 | projection %2 -> %3", changed, projectionBefore, projectionAfter);
		evidence = evidence + string.Format(" | garrison infantry %1 -> %2", infantryBefore, infantryAfter);
		evidence = evidence + string.Format(" | vehicles %1 -> %2", vehiclesBefore, vehiclesAfter);

		bool cleanupExpected = changed;
		if (cleanupExpected)
			cleanupExpected = projectionBefore > 0;
		if (cleanupExpected)
			cleanupExpected = projectionAfter == 0;
		if (cleanupExpected)
			cleanupExpected = infantryAfter == infantryBefore;
		if (cleanupExpected)
			cleanupExpected = vehiclesAfter == vehiclesBefore;
		return cleanupExpected;
	}

	bool CampaignDebugIsActiveGroupResponseRunMovement(HST_ActiveGroupState activeGroup, out string actual)
	{
		actual = "group missing";
		if (!activeGroup)
			return false;

		bool responseRunToken = activeGroup.m_sSpawnFallbackMode.Contains("response_run");
		bool groupRun;
		bool formationTight;
		int formationDisplacement = -1;
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
				formationDisplacement = groupMovement.GetFormationDisplacement();
				formationTight = formationDisplacement == 1;
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
		actual = string.Format("response_run %1 | group wanted %2 | groupRun %3 | formation %4 tight %5 | agents run %6/%7", responseRunToken, groupWanted, groupRun, formationDisplacement, formationTight, runAgentCount, liveAgentCount);
		return responseRunToken && formationTight && (groupRun || agentRun);
	}

	protected bool TryKickPendingNativeGroupSpawn(HST_ActiveGroupState activeGroup, string source)
	{
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
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
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
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

	protected bool EnsureForceSpawnNextMemberAIWorldBudget(HST_ActiveGroupState activeGroup, out string failureReason)
	{
		failureReason = "";
		if (!activeGroup)
		{
			failureReason = "AIWorld exact-member budget check failed: active group is missing.";
			return false;
		}
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
		{
			failureReason = "AIWorld exact-member budget check failed: AIWorld is missing.";
			return false;
		}

		int registeredMembers = CountForceSpawnRuntimeMembers(activeGroup);
		int remainingMembers = Math.Max(1, activeGroup.m_iInfantryCount - registeredMembers);
		int currentLimited = aiWorld.GetCurrentAmountOfLimitedAIs();
		int currentLimit = aiWorld.GetAILimit();
		int requiredLimit = currentLimited + remainingMembers + ACTIVE_GROUP_AI_WORLD_REQUIRED_HEADROOM;
		if (requiredLimit < ACTIVE_GROUP_AI_WORLD_MIN_LIMIT)
			requiredLimit = ACTIVE_GROUP_AI_WORLD_MIN_LIMIT;
		if (currentLimit < requiredLimit)
			aiWorld.SetAILimit(requiredLimit);

		currentLimited = aiWorld.GetCurrentAmountOfLimitedAIs();
		currentLimit = aiWorld.GetAILimit();
		if (!aiWorld.CanLimitedAIBeAdded() || (currentLimit > 0 && currentLimited + remainingMembers > currentLimit))
		{
			failureReason = string.Format("AIWorld exact-member budget lacks headroom: remaining %1 | %2", remainingMembers, BuildAIWorldBudgetDebug());
			return false;
		}
		return true;
	}

	protected bool TrySpawnExactMissionConvoyFrozenCrewGroup(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_ActiveGroupState activeGroup)
	{
		if (!state || !IsExactMissionConvoyContract(mission) || !activeGroup
			|| activeGroup.m_sMissionInstanceId != mission.m_sInstanceId || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		HST_MissionAssetState asset = FindMissionConvoyAssetForGroup(state, mission, activeGroup);
		HST_ConvoyElementState element = ResolveExactMissionConvoyElement(state, mission, asset, activeGroup);
		string authorityFailure = ValidateExactMissionConvoyCrewProjectionAuthority(state, mission, asset, activeGroup, element);
		if (!authorityFailure.IsEmpty())
		{
			activeGroup.m_bSpawnAttempted = true;
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = "Exact frozen crew authority rejected: " + authorityFailure;
			return false;
		}
		if (CountExactMissionConvoyMemberMappings(mission.m_sInstanceId, activeGroup.m_sGroupId) != 0)
		{
			activeGroup.m_bSpawnAttempted = true;
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = "Exact frozen crew projection found stale or ambiguous member identity mappings.";
			return false;
		}

		string aiWorldBudgetFailure;
		if (!EnsureActiveGroupAIWorldBudget(activeGroup, "exact frozen convoy crew", aiWorldBudgetFailure))
		{
			MarkActiveGroupAIWorldBudgetDeferred(activeGroup, state, aiWorldBudgetFailure, "exact_frozen_crew_budget");
			return false;
		}

		ResourceName groupResourceName = activeGroup.m_sPrefab;
		Resource groupResource = Resource.Load(groupResourceName);
		BaseWorld world = GetGame().GetWorld();
		if (!world || !groupResource || !groupResource.IsValid()
			|| !IsValidGroupPrefabResource(activeGroup.m_sPrefab, activeGroup.m_sFactionKey))
		{
			activeGroup.m_bSpawnAttempted = true;
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = "Exact frozen crew group prefab is invalid or unavailable.";
			return false;
		}

		activeGroup.m_bSpawnAttempted = true;
		string requestedStatus = activeGroup.m_sRuntimeStatus;
		activeGroup.m_sRuntimeStatus = "spawning";
		activeGroup.m_sSpawnFallbackMode = "convoy_exact_frozen_roster";
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = 0;
		vector spawnPosition = HST_WorldPositionService.ResolveSafeGroundPosition(activeGroup.m_vPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		activeGroup.m_vPosition = spawnPosition;

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = spawnPosition;
		SCR_AIGroup.IgnoreSpawning(true);
		IEntity rootEntity = GetGame().SpawnEntityPrefabEx(groupResourceName, false, world, params);
		SCR_AIGroup.IgnoreSpawning(false);
		SCR_AIGroup rootGroup = SCR_AIGroup.Cast(rootEntity);
		if (!rootGroup)
		{
			if (rootEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(rootEntity);
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = "Exact frozen crew prefab did not create an SCR_AIGroup root.";
			return false;
		}

		StabilizeRuntimeAIGroupRoot(rootEntity, activeGroup, "exact frozen convoy crew root");
		rootGroup.SetSpawnImmediately(false);
		string actualRootFaction;
		if (!IsRuntimeGroupRootFactionExpected(rootEntity, activeGroup, actualRootFaction))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(rootEntity);
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = string.Format("Exact frozen crew root faction mismatch: expected %1 actual %2.", activeGroup.m_sFactionKey, ReportText(actualRootFaction));
			return false;
		}

		ApplyCampaignDebugEntityName(rootEntity, "exact_convoy_group", activeGroup.m_sGroupId);
		m_aRuntimeGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeGroupEntities.Insert(rootEntity);
		HST_ExactMissionConvoyOutboundProjectionTransaction transaction = FindExactMissionConvoyOutboundProjectionTransaction(mission.m_sInstanceId);
		if (transaction)
			SetExactMissionConvoyProjectionEntityPublished(rootEntity, false);

		HST_OperationRecordState operation = ResolveExactMissionConvoyOperationForRuntime(state, mission);
		HST_ForceManifestState manifest;
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		if (operation)
			manifest = state.FindForceManifest(operation.m_sManifestId);
		int spawnedMembers;
		for (int seatIndex = 0; seatIndex < element.m_iOriginalCrewCount; seatIndex++)
		{
			HST_ForceManifestMemberState member = ResolveExactMissionConvoyManifestMemberForSeat(manifest, element, seatIndex);
			HST_ForceSpawnSlotResultState slotResult;
			if (member && batch)
				slotResult = batch.FindSlotResult(member.m_sSlotId);
			if (!member || !slotResult)
			{
				activeGroup.m_sSpawnFailureReason = string.Format("Exact frozen crew seat %1 has no unique durable member authority.", seatIndex);
				break;
			}
			if (slotResult.m_bCasualtyConfirmed)
				continue;
			if (!IsValidInfantryCharacterPrefabResource(member.m_sPrefab, activeGroup.m_sFactionKey))
			{
				activeGroup.m_sSpawnFailureReason = string.Format("Exact frozen crew seat %1 prefab is invalid.", seatIndex);
				break;
			}

			vector memberPosition = ResolveFallbackInfantryMemberPosition(spawnPosition, seatIndex);
			IEntity memberEntity = SpawnFallbackInfantryCharacter(member.m_sPrefab, memberPosition, activeGroup.m_sFactionKey);
			if (!memberEntity || ResolveEntityPrefabName(memberEntity) != member.m_sPrefab
				|| !AttachFactionInfantryMemberToRuntimeGroup(rootGroup, memberEntity, activeGroup, seatIndex))
			{
				if (memberEntity)
					SCR_EntityHelper.DeleteEntityAndChildren(memberEntity);
				activeGroup.m_sSpawnFailureReason = string.Format("Exact frozen crew seat %1 failed synchronous spawn or root attachment.", seatIndex);
				break;
			}

			ApplyCampaignDebugEntityName(memberEntity, "exact_convoy_member", member.m_sSlotId);
			m_aRuntimeGroupIds.Insert(activeGroup.m_sGroupId);
			m_aRuntimeGroupEntities.Insert(memberEntity);
			if (!RegisterExactMissionConvoyMemberEntity(state, mission, activeGroup, member, memberEntity))
			{
				activeGroup.m_sSpawnFailureReason = string.Format("Exact frozen crew seat %1 violated the slot/entity bijection.", seatIndex);
				break;
			}
			if (transaction)
				SetExactMissionConvoyProjectionEntityPublished(memberEntity, false);
			spawnedMembers++;
		}

		if (!activeGroup.m_sSpawnFailureReason.IsEmpty()
			|| spawnedMembers != element.m_iSurvivingCrewCount
			|| CountExactMissionConvoyMemberMappings(mission.m_sInstanceId, activeGroup.m_sGroupId) != spawnedMembers
			|| CountAliveRuntimeCrewAgents(activeGroup) != spawnedMembers)
		{
			string exactFailure = activeGroup.m_sSpawnFailureReason;
			if (exactFailure.IsEmpty())
				exactFailure = string.Format("Exact frozen crew projection completed with %1/%2 mapped living members.", spawnedMembers, element.m_iSurvivingCrewCount);
			DeleteRuntimeCrewEntities(activeGroup.m_sGroupId);
			activeGroup.m_bSpawnedEntity = false;
			activeGroup.m_sRuntimeEntityId = "";
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			activeGroup.m_sSpawnFailureReason = exactFailure;
			activeGroup.m_iSpawnedAgentCount = 0;
			return false;
		}

		ApplyRuntimeGroupFaction(rootEntity, activeGroup, "exact frozen convoy crew", true);
		ReconcileRuntimeGroupEditableMembership(rootGroup, activeGroup, "exact frozen convoy crew");
		rootGroup.SetNumberOfMembersToSpawn(spawnedMembers);
		rootGroup.ActivateAllMembers();
		rootGroup.ActivateAI();
		activeGroup.m_bSpawnedEntity = true;
		activeGroup.m_bSpawnCompleted = true;
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = ResolveSpawnedRuntimeStatus(activeGroup, requestedStatus);
		activeGroup.m_iSpawnedAgentCount = spawnedMembers;
		activeGroup.m_iLastSeenAliveCount = spawnedMembers;
		activeGroup.m_iSurvivorInfantryCount = spawnedMembers;
		activeGroup.m_iDurableLivingInfantryCount = spawnedMembers;
		activeGroup.m_iMaxObservedCrewAlive = Math.Max(activeGroup.m_iMaxObservedCrewAlive, spawnedMembers);
		activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		activeGroup.m_bEverPopulated = true;
		activeGroup.m_bEverHadLivingCrew = spawnedMembers > 0;
		RecordConvoyCrewObservedAlive(activeGroup, spawnedMembers);
		RefreshActiveGroupZoneCounts(state, activeGroup);
		if (transaction)
			SetExactMissionConvoyOutboundProjectionTransactionVisible(transaction, false);
		DebugLog(string.Format("exact frozen convoy crew projected %1 with %2 slot-bound members", activeGroup.m_sGroupId, spawnedMembers));
		return true;
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
			Print(string.Format("Partisan | active group AIWorld limit raised | group %1 | source %2 | limited %3 | limit %4 -> %5 | desiredMembers %6 | headroom %7",
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
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return;
		if (IsLocalSecurityPatrolClaimant(state, activeGroup))
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
		Print(string.Format("Partisan | active group AIWorld native spawn deferred | group %1 | stage %2 | reason %3", activeGroup.m_sGroupId, ReportText(stage), ReportText(failureReason)), LogLevel.WARNING);
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
			Print(string.Format("Partisan | active group editable membership missing group component | group %1 | source %2 | visual %3", activeGroup.m_sGroupId, ReportText(source), ReportText(BuildRuntimeEntityVisualEvidence(group))), LogLevel.WARNING);
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

		int discoveredLiving = DiscoverRuntimeGroupMemberHandles(group, activeGroup, editableGroup, source);
		if (discoveredLiving > livingCount)
			livingCount = discoveredLiving;

		int trackedLiving;
		int trackedReattached;
		int trackedParented;
		if (rawCount <= 0 || playerAndAgentCount <= 0 || livingCount <= 0 || parentedAfter < editableCount)
		{
			trackedLiving = ReconcileTrackedRuntimeMembersWithAIGroup(group, activeGroup, editableGroup, source, trackedReattached, trackedParented);
			if (trackedLiving > livingCount)
				livingCount = trackedLiving;
			parentedAfter += trackedParented;
			repairedParents += trackedParented;
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

		int finalRawCount = group.GetAgentsCount();
		int finalPlayerAndAgentCount = group.GetPlayerAndAgentCount();
		int finalEditableSize = editableGroup.GetSize();
		bool recoveredMembership = livingCount > 0 && (finalRawCount > 0 || finalPlayerAndAgentCount > 0 || finalEditableSize > 0);
		bool suspicious = (!recoveredMembership && finalPlayerAndAgentCount <= 0) || failedParents > 0 || missingEditable > 0 || missingControlled > 0 || leaderChanged;
		if (suspicious)
		{
			string throttleKey = "membership_warn_" + activeGroup.m_sGroupId;
			if (!ShouldEmitThrottled(throttleKey, 30000))
				return;
			string report = string.Format("Partisan | active group editable membership reconciled | group %1 | source %2 | raw %3 server %4 playerAgents %5 living %6 | editableSize %7",
				activeGroup.m_sGroupId,
				ReportText(source),
				finalRawCount,
				group.GetServerAgentsCount(),
				finalPlayerAndAgentCount,
				livingCount,
				finalEditableSize);
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
			if (trackedLiving > 0 || trackedReattached > 0 || trackedParented > 0)
				report = report + string.Format(" | trackedFallback living %1 reattached %2 parented %3", trackedLiving, trackedReattached, trackedParented);
			Print(report);
		}
		else if (m_bDebugLoggingEnabled)
		{
			string debugThrottleKey = "debug_membership_ok_" + activeGroup.m_sGroupId;
			if (!ShouldEmitThrottled(debugThrottleKey, 30000))
				return;
			Print("Partisan physical war debug | " + string.Format("active group editable membership verified %1 via %2 | %3", activeGroup.m_sGroupId, source, BuildRuntimeEntityVisualEvidence(group)));
		}
	}

	protected int DiscoverRuntimeGroupMemberHandles(SCR_AIGroup group, HST_ActiveGroupState activeGroup, SCR_EditableGroupComponent editableGroup, string source)
	{
		if (!group || !activeGroup || activeGroup.m_sGroupId.IsEmpty())
			return 0;

		int registered;
		int attached;
		int parented;
		int valid;
		int deadRegistered;
		array<AIAgent> agents = new array<AIAgent>;
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			bool candidateRegistered;
			bool candidateAttached;
			bool candidateParented;
			bool candidateLiving;
			bool candidateDeadRegistered;
			if (!ReconcileRuntimeGroupMemberCandidate(group, activeGroup, editableGroup, agent.GetControlledEntity(), source + " native agent", candidateRegistered, candidateAttached, candidateParented, candidateLiving, candidateDeadRegistered))
				continue;

			valid++;
			if (candidateRegistered)
				registered++;
			if (candidateAttached)
				attached++;
			if (candidateParented)
				parented++;
			if (candidateDeadRegistered)
				deadRegistered++;
		}

		int expectedHandles = ResolveActiveGroupExpectedRuntimeMemberHandles(activeGroup);
		bool needsWorldScan = false;
		if (needsWorldScan)
		{
			BaseWorld world = GetGame().GetWorld();
			if (world)
			{
				m_aRuntimeMemberRepairCandidates.Clear();
				ScanRuntimeGroupMemberRepairCenter(world, group.GetOrigin());
				ScanRuntimeGroupMemberRepairCenter(world, activeGroup.m_vPosition);
				ScanRuntimeGroupMemberRepairCenter(world, activeGroup.m_vSourcePosition);
				ScanRuntimeGroupMemberRepairCenter(world, activeGroup.m_vTargetPosition);

				foreach (IEntity candidate : m_aRuntimeMemberRepairCandidates)
				{
					if (!candidate)
						continue;
					if (CountTrackedRuntimeMemberHandles(activeGroup.m_sGroupId) >= expectedHandles && group.GetPlayerAndAgentCount() > 0)
						break;

					bool scanRegistered;
					bool scanAttached;
					bool scanParented;
					bool scanLiving;
					bool scanDeadRegistered;
					if (!ReconcileRuntimeGroupMemberCandidate(group, activeGroup, editableGroup, candidate, source + " world scan", scanRegistered, scanAttached, scanParented, scanLiving, scanDeadRegistered))
						continue;

					valid++;
					if (scanRegistered)
						registered++;
					if (scanAttached)
						attached++;
					if (scanParented)
						parented++;
					if (scanDeadRegistered)
						deadRegistered++;
				}
			}
		}

		int living = CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId);
		if (registered > 0 || attached > 0 || parented > 0 || deadRegistered > 0)
		{
			group.ActivateAllMembers();
			group.ActivateAI();
			DebugLogThrottled("member_handles_" + activeGroup.m_sGroupId, string.Format("active group member handles discovered %1 via %2 | valid %3 registered %4 attached %5 parented %6 dead %7 living %8/%9", activeGroup.m_sGroupId, source, valid, registered, attached, parented, deadRegistered, living, expectedHandles), 30000);
		}

		return living;
	}

	protected void ScanRuntimeGroupMemberRepairCenter(BaseWorld world, vector center)
	{
		if (!world || IsZeroVector(center))
			return;

		world.QueryEntitiesBySphere(center, ACTIVE_GROUP_MEMBER_REPAIR_RADIUS_METERS, AddRuntimeMemberRepairCandidate, null, EQueryEntitiesFlags.ALL);
	}

	protected bool AddRuntimeMemberRepairCandidate(IEntity entity)
	{
		if (!entity)
			return true;

		if (m_aRuntimeMemberRepairCandidates.Find(entity) < 0)
			m_aRuntimeMemberRepairCandidates.Insert(entity);

		return true;
	}

	protected bool ReconcileRuntimeGroupMemberCandidate(SCR_AIGroup group, HST_ActiveGroupState activeGroup, SCR_EditableGroupComponent editableGroup, IEntity entity, string source, out bool registered, out bool attached, out bool parented, out bool living, out bool deadRegistered)
	{
		registered = false;
		attached = false;
		parented = false;
		living = false;
		deadRegistered = false;
		if (!group || !activeGroup || !entity || AIGroup.Cast(entity))
			return false;
		if (!IsRuntimeGroupMemberCandidate(group, entity, activeGroup))
			return false;

		living = IsLivingEntity(entity);
		ApplyRuntimeInfantryMemberFaction(entity, activeGroup.m_sFactionKey);
		if (RegisterRuntimeGroupEntityHandle(activeGroup.m_sGroupId, entity))
		{
			registered = true;
			if (!living)
				deadRegistered = true;
		}

		AIAgent agent = ResolveRuntimeMemberAIAgent(entity);
		if (living && agent)
		{
			AIControlComponent control = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
			if (control)
				control.ActivateAI();

			if (agent.GetParentGroup() != group)
			{
				group.AddAgentFromControlledEntity(entity);
				if (agent.GetParentGroup() != group)
				{
					if (!group.AddAIEntityToGroup(entity))
						group.AddAgent(agent);
				}
				if (agent.GetParentGroup() == group)
					attached = true;
			}

			agent.ActivateAI();
		}

		if (editableGroup)
		{
			SCR_EditableEntityComponent editableMember = SCR_EditableEntityComponent.GetEditableEntity(entity);
			if (editableMember && editableMember.GetParentEntity() != editableGroup)
			{
				editableMember.SetParentEntity(editableGroup);
				if (editableMember.GetParentEntity() == editableGroup)
					parented = true;
			}
		}

		return true;
	}

	protected bool ApplyRuntimeInfantryMemberFaction(IEntity entity, string factionKey)
	{
		if (!entity || factionKey.IsEmpty())
			return false;
		if (!ChimeraCharacter.Cast(entity))
			return false;

		return ApplyEntityFaction(entity, factionKey);
	}

	protected bool IsRuntimeGroupMemberCandidate(SCR_AIGroup group, IEntity entity, HST_ActiveGroupState activeGroup)
	{
		if (!entity || !activeGroup || activeGroup.m_sFactionKey.IsEmpty())
			return false;
		if (IsPlayerControlledRuntimeEntity(entity))
			return false;
		AIControlComponent control = AIControlComponent.Cast(entity.FindComponent(AIControlComponent));
		if (!control)
			return false;

		AIAgent agent = control.GetControlAIAgent();
		if (agent && group && agent.GetParentGroup() == group)
			return true;
		if (IsRuntimeGroupEntityHandleTracked(activeGroup.m_sGroupId, entity))
			return true;

		string prefab = ResolveEntityPrefabName(entity);
		if (!IsInfantryCharacterPrefabCatalogFactionMatch(prefab, activeGroup.m_sFactionKey))
		{
			string entityFactionKey = ResolveEntityFactionKey(entity);
			if (entityFactionKey != activeGroup.m_sFactionKey)
				return false;

			if (!agent)
				return true;

			AIGroup parentGroup = agent.GetParentGroup();
			if (parentGroup && parentGroup != group)
				return false;
		}

		return true;
	}

	protected bool IsPlayerControlledRuntimeEntity(IEntity entity)
	{
		if (!entity)
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		return playerManager.GetPlayerIdFromControlledEntity(entity) > 0;
	}

	protected int ResolveActiveGroupExpectedRuntimeMemberHandles(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return 1;

		int expected = Math.Max(0, activeGroup.m_iInfantryCount);
		expected = Math.Max(expected, activeGroup.m_iOriginalInfantryCount);
		expected = Math.Max(expected, activeGroup.m_iSpawnedAgentCount);
		expected = Math.Max(expected, activeGroup.m_iLastSeenAliveCount);
		if (expected <= 0)
			expected = 1;

		return Math.Min(expected, 24);
	}

	protected int CountTrackedRuntimeMemberHandles(string groupId)
	{
		if (groupId.IsEmpty())
			return 0;

		int count;
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;
			if (!m_aRuntimeGroupEntities[i] || AIGroup.Cast(m_aRuntimeGroupEntities[i]))
				continue;

			count++;
		}

		return count;
	}

	protected bool IsRuntimeGroupEntityHandleTracked(string groupId, IEntity entity)
	{
		if (groupId.IsEmpty() || !entity)
			return false;

		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;
			if (m_aRuntimeGroupEntities[i] == entity)
				return true;
		}

		return false;
	}

	protected bool RegisterRuntimeGroupEntityHandle(string groupId, IEntity entity)
	{
		if (groupId.IsEmpty() || !entity)
			return false;
		if (IsRuntimeGroupEntityHandleTracked(groupId, entity))
			return false;

		m_aRuntimeGroupIds.Insert(groupId);
		m_aRuntimeGroupEntities.Insert(entity);
		return true;
	}

	protected int ReconcileTrackedRuntimeMembersWithAIGroup(SCR_AIGroup group, HST_ActiveGroupState activeGroup, SCR_EditableGroupComponent editableGroup, string source, out int reattached, out int parented)
	{
		reattached = 0;
		parented = 0;
		if (!group || !activeGroup)
			return 0;

		int living;
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != activeGroup.m_sGroupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity || AIGroup.Cast(entity))
				continue;
			if (!IsLivingEntity(entity))
				continue;

			living++;
			AIAgent agent = ResolveRuntimeMemberAIAgent(entity);
			if (agent && agent.GetParentGroup() != group)
			{
				group.AddAgentFromControlledEntity(entity);
				if (agent.GetParentGroup() != group)
				{
					if (!group.AddAIEntityToGroup(entity))
						group.AddAgent(agent);
				}
				if (agent.GetParentGroup() == group)
					reattached++;
			}

			if (agent)
				agent.ActivateAI();

			if (!editableGroup)
				continue;

			SCR_EditableEntityComponent editableMember = SCR_EditableEntityComponent.GetEditableEntity(entity);
			if (editableMember && editableMember.GetParentEntity() != editableGroup)
			{
				editableMember.SetParentEntity(editableGroup);
				if (editableMember.GetParentEntity() == editableGroup)
					parented++;
			}
		}

		if (living > 0)
		{
			group.ActivateAllMembers();
			group.ActivateAI();
			if (reattached > 0 || parented > 0)
				DebugLogThrottled("tracked_reconcile_" + activeGroup.m_sGroupId, string.Format("active group tracked member reconcile %1 via %2 | living %3 | reattached %4 | parented %5", activeGroup.m_sGroupId, source, living, reattached, parented), 30000);
		}

		return living;
	}

	protected bool TryPopulatePendingActiveGroupFromNativeSlots(HST_ActiveGroupState activeGroup, string requestedStatus, HST_CampaignState state, string source)
	{
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		HST_ActiveMissionState exactMission;
		if (state && activeGroup)
			exactMission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
		if (IsExactMissionConvoyContract(exactMission))
			return false;
		if (IsExactGarrisonPatrolGroup(state, activeGroup))
			return false;
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
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
		if (activeGroup && CountExactMissionConvoyMemberMappings("", activeGroup.m_sGroupId) >= 0
			&& !activeGroup.m_sConvoyElementId.IsEmpty() && !activeGroup.m_sManifestId.IsEmpty())
			return 0;
		if (IsForceSpawnQueueManaged(activeGroup))
			return 0;
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
				Print(string.Format("Partisan | active group rejected stock slot member %1 | group %2 | expected %3 | source %4", slotPrefabText, activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source), LogLevel.WARNING);
				continue;
			}

			vector position = ResolveFallbackInfantryMemberPosition(activeGroup.m_vPosition, i);
			EntitySpawnParams params = new EntitySpawnParams;
			params.TransformMode = ETransformMode.WORLD;
			params.Transform[3] = position;
			IEntity member = GetGame().SpawnEntityPrefabEx(slotPrefab, true, world, params);
			if (!member)
			{
				Print(string.Format("Partisan | active group stock slot member spawn failed | group %1 | prefab %2 | source %3", activeGroup.m_sGroupId, slotPrefabText, source), LogLevel.WARNING);
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
			Print(string.Format("Partisan | stock group slot member spawned | group %1 | faction %2 | prefab %3 | position %4 | source %5", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, slotPrefabText, position, source));
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
			Print(string.Format("Partisan | active group stock member-slot population failed %1 faction %2 groupPrefab %3 firstSlot %4 source %5", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, activeGroup.m_sPrefab, ReportText(firstPrefab), source), LogLevel.WARNING);
		}

		return spawnedCount;
	}

	protected bool TryPopulatePendingActiveGroupFromFactionInfantry(HST_ActiveGroupState activeGroup, string requestedStatus, HST_CampaignState state, string source, bool allowInitializingFallback = false)
	{
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		HST_ActiveMissionState exactMission;
		if (state && activeGroup)
			exactMission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
		if (IsExactMissionConvoyContract(exactMission))
			return false;
		if (IsExactGarrisonPatrolGroup(state, activeGroup))
			return false;
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
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
			Print(string.Format("Partisan | active group direct fallback failed %1: missing group prefab %2", activeGroup.m_sGroupId, resourceName), LogLevel.WARNING);
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
			Print(string.Format("Partisan | active group direct fallback failed %1: prefab %2 did not spawn an AIGroup", activeGroup.m_sGroupId, resourceName), LogLevel.WARNING);
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
		if (activeGroup && !activeGroup.m_sConvoyElementId.IsEmpty() && !activeGroup.m_sManifestId.IsEmpty())
			return 0;
		if (IsForceSpawnQueueManaged(activeGroup))
			return 0;
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
			Print(string.Format("Partisan | direct infantry spawned | group %1 | faction %2 | prefab %3 | position %4", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, prefab, position));
			spawnedCount++;
		}

		if (spawnedCount > 0)
		{
			ApplyRuntimeGroupFaction(group, activeGroup, "direct infantry fallback", true);
			group.ActivateAI();
		}

		if (spawnedCount <= 0)
			Print(string.Format("Partisan | active group fallback infantry failed %1 faction %2 prefab %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, activeGroup.m_sPrefab), LogLevel.WARNING);

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

		if (agent.GetParentGroup() != group)
		{
			group.AddAgentFromControlledEntity(member);
			if (agent.GetParentGroup() != group)
			{
				if (!group.AddAIEntityToGroup(member))
					group.AddAgent(agent);
			}
		}

		if (agent.GetParentGroup() != group)
		{
			DebugLog(string.Format("active group member attach failed %1 index %2: parent group did not match after attach", activeGroup.m_sGroupId, index));
			return false;
		}

		agent.ActivateAI();
		group.ActivateAllMembers();
		group.ActivateAI();

		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(group.FindComponent(SCR_EditableGroupComponent));
		SCR_EditableEntityComponent editableMember = SCR_EditableEntityComponent.GetEditableEntity(member);
		if (editableGroup && editableMember && editableMember.GetParentEntity() != editableGroup)
			editableMember.SetParentEntity(editableGroup);

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

		Print(string.Format("Partisan | no valid infantry character prefab found for fallback group %1 faction %2", groupId, factionKey), LogLevel.WARNING);
		return "";
	}

	protected bool IsValidInfantryCharacterPrefabResource(string prefab, string factionKey)
	{
		if (prefab.IsEmpty() || !prefab.Contains("{") || !prefab.Contains("}") || !prefab.Contains("Prefabs/Characters/"))
			return false;

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			Print(string.Format("Partisan | rejected missing infantry character prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!IsInfantryCharacterPrefabCatalogFactionMatch(prefab, factionKey))
		{
			Print(string.Format("Partisan | rejected wrong-faction infantry character prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
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
			Print(string.Format("Partisan | runtime faction applied | group %1 | expected %2 | source %3 | changed %4 | mismatches %5 | visual %6 | sample %7", activeGroup.m_sGroupId, factionKey, source, changedCount, mismatchedCount, ReportText(BuildRuntimeEntityVisualEvidence(entity)), ReportText(sample)));
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
				Print(string.Format("Partisan | runtime faction applied | group %1 | expected %2 | source %3 | changed %4 | mismatches %5 | visual %6 | sample %7", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source, changed, mismatches, ReportText(BuildRuntimeEntityVisualEvidence(runtimeEntity)), ReportText(sample)));
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
				Print(string.Format("Partisan | runtime vehicle faction cleared | group %1 | source %2 | changed %3 | remaining claims %4 | sample %5", activeGroup.m_sGroupId, source, vehicleChangedCount, vehicleMismatches, ReportText(vehicleSample)));
		}

		string sample;
		int mismatches = CountActiveGroupRuntimeFactionMismatches(activeGroup, sample);
		if (sample.IsEmpty())
			sample = firstSample;
		if (mismatches > 0)
			Print(string.Format("Partisan | runtime faction mismatch persists | group %1 | expected %2 | source %3 | repaired changed %4 mismatches %5 | audit mismatches %6 | visual %7 | sample %8", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source, totalChanged, totalMismatches, mismatches, ReportText(BuildActiveGroupRuntimeVisualEvidence(activeGroup.m_sGroupId)), ReportText(sample)), LogLevel.WARNING);
	}

	protected bool TryRepairEmptyRuntimeGroupPopulation(HST_CampaignState state, HST_ActiveGroupState activeGroup, string source)
	{
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		if (IsLocalSecurityPatrolClaimant(state, activeGroup))
			return false;
		if (IsExactGarrisonPatrolGroup(state, activeGroup))
			return false;
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
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
			Print(string.Format("Partisan | repaired empty runtime group %1 with stock %2 member slots via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
			return true;
		}

		if (TryPopulatePendingActiveGroupFromFactionInfantry(activeGroup, previousStatus, state, source + " empty runtime shell", true))
		{
			Print(string.Format("Partisan | repaired empty runtime group %1 with direct %2 infantry via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
			return true;
		}

		activeGroup.m_sRuntimeStatus = previousStatus;
		activeGroup.m_bSpawnedEntity = previousSpawned;
		activeGroup.m_sSpawnFailureReason = "Runtime group shell had zero live agents and direct faction infantry repair failed via " + source + ".";
		Print(string.Format("Partisan | empty runtime group repair failed %1 expected %2 via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source), LogLevel.WARNING);
		return false;
	}

	protected bool CanAttemptMissionConvoyCrewPopulationRepair(HST_CampaignState state, HST_ActiveMissionState mission, HST_ActiveGroupState activeGroup)
	{
		if (!state || !mission || !activeGroup)
			return false;
		if (IsForceSpawnQueueManaged(activeGroup))
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
		if (IsExactMissionConvoyContract(mission))
		{
			HST_MissionAssetState exactAsset = FindMissionConvoyAssetForGroup(state, mission, activeGroup);
			HST_ConvoyElementState exactElement = ResolveExactMissionConvoyElement(state, mission, exactAsset, activeGroup);
			if (!exactElement || exactElement.m_iSurvivingCrewCount <= 0)
				return false;
		}
		if (CountAliveRuntimeCrewAgents(activeGroup) > 0)
			return false;
		if (IsConvoyCrewControlPending(state, activeGroup))
			return false;

		return activeGroup.m_bSpawnAttempted || activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "spawn_pending_agents";
	}

	protected bool TryQueuePrimaryActiveGroupRespawnForRepair(HST_ActiveGroupState activeGroup, HST_CampaignState state, string requestedStatus, string source)
	{
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
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

		Print(string.Format("Partisan | active group repair queued primary stock group %1 | status %2 | live %3 | source %4", activeGroup.m_sGroupId, activeGroup.m_sRuntimeStatus, liveCrew, source));
		return true;
	}

	protected bool TryRepairMissionConvoyCrewPopulation(HST_CampaignState state, HST_ActiveMissionState mission, HST_ActiveGroupState activeGroup, string source)
	{
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
		if (IsExactMissionConvoyContract(mission))
			return false;
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
				Print(string.Format("Partisan mission convoy | repaired zero-live crew group %1 with primary stock %2 group via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
			else
				Print(string.Format("Partisan mission convoy | queued primary stock crew group repair %1 expected %2 via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
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
			Print(string.Format("Partisan mission convoy | repaired zero-live crew group %1 with stock %2 member slots via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
			return true;
		}

		if (TryPopulatePendingActiveGroupFromFactionInfantry(activeGroup, ResolveMissionConvoyRuntimeStatus(mission), state, source + " convoy crew repair", true) && CountAliveRuntimeCrewAgents(activeGroup) > 0)
		{
			activeGroup.m_bCrewPopulationTerminallyFailed = false;
			activeGroup.m_sCrewPopulationFailureReason = "";
			Print(string.Format("Partisan mission convoy | repaired zero-live crew group %1 with direct %2 infantry via %3", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source));
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
		Print(string.Format("Partisan mission convoy | zero-live crew repair failed for %1 expected %2 via %3; convoy remains pending instead of eliminated", activeGroup.m_sGroupId, activeGroup.m_sFactionKey, source), LogLevel.WARNING);
		return false;
	}

	bool CampaignDebugHasRuntimeVehicleEntity(string groupId)
	{
		return GetRuntimeVehicleEntity(groupId) != null;
	}

	bool CampaignDebugHasRuntimeGroupEntity(string groupId)
	{
		return HasRuntimeGroupEntity(groupId);
	}

	string CampaignDebugBuildActiveGroupRuntimeVisualEvidence(string groupId)
	{
		return BuildActiveGroupRuntimeVisualEvidence(groupId);
	}

	int CampaignDebugResolveActiveGroupEditableSize(string groupId)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(GetRuntimeCrewGroupEntity(groupId));
		if (!group)
			return -1;

		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(group.FindComponent(SCR_EditableGroupComponent));
		if (!editableGroup)
			return -1;

		return editableGroup.GetSize();
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

		if (seatingReason.Contains("seating pending yes") || seatingReason.Contains("waiting for authoritative seat transition"))
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
		if (activeGroup && (activeGroup.m_bQRF || IsExactPlayerSupportActiveGroup(state, activeGroup)
			|| IsExactEnemyPatrolGroup(state, activeGroup)
			|| IsExactGarrisonPatrolGroup(state, activeGroup)
			|| IsExactOrQuarantinedMissionGuardGroup(state, activeGroup)))
			m_bMarkerRefreshNeeded = true;
		if (!state || !activeGroup)
			return;
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return;
		if (ShouldHoldForceSpawnProjection(state, activeGroup))
			return;
		if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
			return;
		if (IsExactPlayerSupportActiveGroup(state, activeGroup))
			return;

		HST_ZoneState zone = state.FindZone(activeGroup.m_sZoneId);
		if (zone)
			ApplyActiveZoneCounts(state, zone);
	}

	protected bool TrySpawnActiveVehicle(HST_ActiveGroupState activeGroup, HST_CampaignState state = null)
	{
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		if (IsLocalSecurityPatrolClaimant(state, activeGroup))
			return false;
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
		if (!activeGroup || IsTerminalActiveGroupRuntimeStatus(activeGroup) || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		if (ShouldDeferActiveGroupRuntimePhysicalization(state, activeGroup))
		{
			MarkActiveGroupRuntimePhysicalizationDeferred(activeGroup, state);
			return false;
		}

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
			Print(string.Format("Partisan | active vehicle spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
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
			Print(string.Format("Partisan | active vehicle prefab spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
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
		Print(string.Format("Partisan | spawned active vehicle %1 using %2 at %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, spawnPosition));
		return true;
	}

	protected string ResolveSpawnedRuntimeStatus(HST_ActiveGroupState activeGroup, string requestedStatus)
	{
		if (activeGroup && !activeGroup.m_sSupportRequestId.IsEmpty()
			&& (requestedStatus.IsEmpty()
				|| requestedStatus == "queued"
				|| requestedStatus == "spawning"
				|| requestedStatus == "support_arrived"
				|| requestedStatus == "exact_support_spawn_queued"))
			return "support_active";

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

		Print(string.Format("Partisan | no valid %1 group prefab found for faction %2", purpose, factionKey), LogLevel.WARNING);
		return "";
	}

	protected bool IsValidGroupPrefabResource(string prefab, string factionKey)
	{
		if (prefab.IsEmpty())
			return false;

		if (prefab.Contains("_NotSpawned") || prefab.Contains("NotSpawned"))
		{
			Print(string.Format("Partisan | rejected non-spawning group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (prefab.Contains("PlayableGroup.et"))
		{
			Print(string.Format("Partisan | rejected player placeholder group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			Print(string.Format("Partisan | rejected missing group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!IsGroupPrefabCatalogFactionMatch(prefab, factionKey))
		{
			Print(string.Format("Partisan | rejected wrong-faction group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
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
				Print(string.Format("Partisan | rejected aircraft vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!IsGroundVehicleResource(prefab))
		{
			if (logRejection)
				Print(string.Format("Partisan | rejected non-ground vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
		{
			if (logRejection)
				Print(string.Format("Partisan | rejected invalid vehicle root prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (!IsGuidQualifiedVehicleResource(prefab))
		{
			if (logRejection)
				Print(string.Format("Partisan | rejected unverified path-only vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			if (logRejection)
				Print(string.Format("Partisan | rejected missing vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
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
			Print(string.Format("Partisan | spawned prefab %1 for %2 did not create an AIGroup", activeGroup.m_sPrefab, activeGroup.m_sGroupId), LogLevel.WARNING);
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
		if (CountExactMissionConvoyMemberMappings("", activeGroup.m_sGroupId) > 0)
			return false;
		if (IsForceSpawnRuntimeOwnershipHeldForGroup(activeGroup.m_sGroupId))
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
				SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(group.FindComponent(SCR_EditableGroupComponent));
				preservedDeadMembers += DetachDeadRuntimeMembersFromGroupRoot(activeGroup.m_sGroupId, editableGroup);
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

	protected bool IsMixedPersonnelVehicleActiveGroup(HST_ActiveGroupState activeGroup)
	{
		return activeGroup
			&& activeGroup.m_iInfantryCount > 0
			&& activeGroup.m_iVehicleCount > 0
			&& !IsMissionConvoyGroup(activeGroup);
	}

	protected bool HasObservedActiveGroupPersonnel(HST_ActiveGroupState activeGroup, int deadTrackedMembers)
	{
		if (!activeGroup)
			return false;

		return activeGroup.m_bEverPopulated
			|| activeGroup.m_bSpawnCompleted
			|| activeGroup.m_iSpawnedAgentCount > 0
			|| activeGroup.m_iDurableLivingInfantryCount > 0
			|| deadTrackedMembers > 0;
	}

	protected bool ShouldApplyCrewlessMixedPersonnelElimination(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		int livingInfantry,
		int deadTrackedMembers,
		bool nativeDelayedPopulationActive,
		bool liveCountGraceActive)
	{
		if (!state || !activeGroup || !activeGroup.m_bSpawnedEntity)
			return false;
		if (!IsMixedPersonnelVehicleActiveGroup(activeGroup) || IsForceSpawnQueueManaged(activeGroup))
			return false;
		if (IsTerminalActiveGroupRuntimeStatus(activeGroup) || activeGroup.m_sRuntimeStatus == "spawn_pending_agents")
			return false;
		if (livingInfantry > 0 || nativeDelayedPopulationActive || liveCountGraceActive)
			return false;

		return HasObservedActiveGroupPersonnel(activeGroup, deadTrackedMembers);
	}

	protected bool ApplyObservedPersonnelElimination(HST_CampaignState state, HST_ActiveGroupState activeGroup, string source)
	{
		if (!state || !activeGroup || IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;

		bool hadPopulation = activeGroup.m_bEverPopulated
			|| activeGroup.m_bSpawnCompleted
			|| activeGroup.m_iSpawnedAgentCount > 0
			|| activeGroup.m_iDurableLivingInfantryCount > 0;
		activeGroup.m_bSpawnAttempted = true;
		activeGroup.m_bEverPopulated = activeGroup.m_bEverPopulated || hadPopulation;
		activeGroup.m_bSpawnCompleted = activeGroup.m_bSpawnCompleted || hadPopulation;
		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_sRuntimeEntityId = "";
		activeGroup.m_sRuntimeStatus = "eliminated";
		activeGroup.m_sSpawnFallbackMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "personnel_eliminated_vehicle_salvage");
		AppendActiveGroupSpawnFailureNote(activeGroup, "All observed group personnel were eliminated; any intact attached vehicle was released as neutral field salvage.");
		activeGroup.m_iSpawnedAgentCount = 0;
		activeGroup.m_iLastSeenAliveCount = 0;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_iSurvivorVehicleCount = 0;
		activeGroup.m_iDurableLivingInfantryCount = 0;
		activeGroup.m_iAssignedWaypointCount = 0;
		activeGroup.m_iLastCasualtySecond = Math.Max(activeGroup.m_iLastCasualtySecond, state.m_iElapsedSeconds);
		activeGroup.m_iEliminatedAtSecond = Math.Max(activeGroup.m_iEliminatedAtSecond, state.m_iElapsedSeconds);
		activeGroup.m_iLifecycleRevision++;

		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (!qrf || qrf.m_bResolved)
				continue;
			if (qrf.m_sGroupId != activeGroup.m_sGroupId && qrf.m_sInstanceId != activeGroup.m_sQRFInstanceId)
				continue;

			qrf.m_bResolved = true;
			qrf.m_bSucceeded = false;
		}

		m_bMarkerRefreshNeeded = true;
		DebugLog(string.Format("active group personnel terminal %1 | zone %2 | source %3", activeGroup.m_sGroupId, activeGroup.m_sZoneId, source));
		return true;
	}

	protected bool HasRuntimeVehicleRegistration(string groupId)
	{
		if (groupId.IsEmpty())
			return false;

		return m_aRuntimeVehicleGroupIds.Find(groupId) >= 0;
	}

	protected bool CleanupTerminalActiveGroupRuntime(HST_CampaignState state, HST_ActiveGroupState activeGroup, string source)
	{
		if (!activeGroup || !IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		if (IsLocalSecurityPatrolClaimant(state, activeGroup))
			return false;

		bool preserveFoldedSurvivorState = activeGroup.m_sRuntimeStatus == "folded";
		int foldedLastSeenAlive = activeGroup.m_iLastSeenAliveCount;
		int foldedSurvivorInfantry = activeGroup.m_iSurvivorInfantryCount;
		int foldedSurvivorVehicles = activeGroup.m_iSurvivorVehicleCount;
		bool changed = CleanupTerminalActiveGroupRuntimeCrew(activeGroup, source);
		bool hadRuntimeWaypoints = m_aRuntimeGroupWaypointIds.Find(activeGroup.m_sGroupId) >= 0;
		DeleteRuntimeGroupWaypoints(activeGroup.m_sGroupId);
		changed = hadRuntimeWaypoints || changed;
		if (!IsMissionConvoyGroup(activeGroup) && HasRuntimeVehicleRegistration(activeGroup.m_sGroupId))
		{
			IEntity vehicle = GetRuntimeVehicleEntity(activeGroup.m_sGroupId);
			bool preserveVehicle = IsMixedPersonnelVehicleActiveGroup(activeGroup)
				&& activeGroup.m_sRuntimeStatus == "eliminated"
				&& activeGroup.m_sSpawnFallbackMode.Contains("personnel_eliminated_vehicle_salvage")
				&& vehicle
				&& IsLivingEntity(vehicle);
			if (preserveVehicle)
			{
				HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(vehicle);
				HST_ZoneState zone;
				if (state)
					zone = state.FindZone(activeGroup.m_sZoneId);
				RegisterDetachedActiveVehicle(state, zone, activeGroup, vehicle, source);
				changed = DeleteRuntimeGroupEntity(activeGroup.m_sGroupId, false) || changed;
				Print(string.Format("Partisan | released terminal mixed-group vehicle as neutral field salvage | group %1 | zone %2 | position %3 | source %4", activeGroup.m_sGroupId, activeGroup.m_sZoneId, vehicle.GetOrigin(), source));
			}
			else
			{
				changed = DeleteRuntimeGroupEntity(activeGroup.m_sGroupId, true) || changed;
			}
		}

		activeGroup.m_bSpawnedEntity = false;
		activeGroup.m_sRuntimeEntityId = "";
		if (preserveFoldedSurvivorState)
		{
			activeGroup.m_iLastSeenAliveCount = foldedLastSeenAlive;
			activeGroup.m_iSurvivorInfantryCount = foldedSurvivorInfantry;
			activeGroup.m_iSurvivorVehicleCount = foldedSurvivorVehicles;
		}
		else
		{
			activeGroup.m_iLastSeenAliveCount = 0;
			activeGroup.m_iSurvivorInfantryCount = 0;
			activeGroup.m_iSurvivorVehicleCount = 0;
		}
		activeGroup.m_iAssignedWaypointCount = 0;
		return changed;
	}

	protected bool TryEliminateCrewlessMixedActiveGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup, string source)
	{
		if (!state || !activeGroup)
			return false;
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		if (IsLocalSecurityPatrolClaimant(state, activeGroup))
			return false;

		int livingInfantry = CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId);
		int deadTrackedMembers = CountDeadTrackedRuntimeGroupMembers(activeGroup.m_sGroupId);
		bool nativeDelayedPopulationActive = IsActiveGroupNativeDelayedPopulationActive(activeGroup);
		bool liveCountGraceActive = IsActiveGroupLiveCountGraceActive(state, activeGroup);
		if (!ShouldApplyCrewlessMixedPersonnelElimination(
			state,
			activeGroup,
			livingInfantry,
			deadTrackedMembers,
			nativeDelayedPopulationActive,
			liveCountGraceActive))
			return false;

		bool changed = ApplyObservedPersonnelElimination(state, activeGroup, source);
		changed = CleanupTerminalActiveGroupRuntime(state, activeGroup, source) || changed;
		RefreshActiveGroupZoneCounts(state, activeGroup);
		Print(string.Format("Partisan | active mixed group eliminated after personnel loss | group %1 | zone %2 | dead tracked %3 | source %4", activeGroup.m_sGroupId, activeGroup.m_sZoneId, deadTrackedMembers, source));
		return changed;
	}

	protected int UnregisterRuntimeCrewHandlesForRespawn(string groupId, string source)
	{
		if (groupId.IsEmpty())
			return 0;
		if (IsForceSpawnRuntimeOwnershipHeldForGroup(groupId))
			return 0;
		if (CountExactMissionConvoyMemberMappings("", groupId) > 0)
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
				SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(group.FindComponent(SCR_EditableGroupComponent));
				preservedDeadMembers += DetachDeadRuntimeMembersFromGroupRoot(groupId, editableGroup);
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

	protected int DetachDeadRuntimeMembersFromGroupRoot(string groupId, SCR_EditableGroupComponent editableGroup)
	{
		if (groupId.IsEmpty() || !editableGroup)
			return 0;

		int detached;
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity || AIGroup.Cast(entity))
				continue;
			if (IsLivingEntity(entity))
				continue;

			SCR_EditableEntityComponent editableMember = SCR_EditableEntityComponent.GetEditableEntity(entity);
			if (editableMember && editableMember.GetParentEntity() == editableGroup)
				editableMember.SetParentEntity(null);
			detached++;
		}

		return detached;
	}

	protected bool EnsureRuntimeGroupEntities(HST_CampaignState state, HST_CampaignPreset preset = null)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || !state.IsOperationalActiveGroup(activeGroup))
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsLocalSecurityPatrolClaimant(state, activeGroup))
				continue;
			if (IsExactGarrisonPatrolGroup(state, activeGroup) && !IsForceSpawnQueueManaged(activeGroup))
				continue;
			if (IsForceSpawnQueueManaged(activeGroup))
			{
				if (!ShouldHoldForceSpawnProjection(state, activeGroup))
					NormalizeStaticActiveGroupRoute(state, preset, activeGroup);
				continue;
			}

			NormalizeStaticActiveGroupRoute(state, preset, activeGroup);

			if (IsMissionConvoyGroup(activeGroup))
			{
				HST_ActiveMissionState mission = FindMissionForConvoyGroup(state, activeGroup);
				HST_MissionAssetState asset = FindMissionConvoyAssetForGroup(state, mission, activeGroup);
				if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
				{
					changed = CleanupTerminalActiveGroupRuntime(state, activeGroup, "ensure runtime") || changed;
					continue;
				}
				if (mission && asset && ShouldSpawnMissionConvoyRuntime(state, activeGroup))
					changed = TrySpawnMissionConvoyGroup(state, preset, mission, asset, activeGroup, ResolveMissionConvoyGroupIndex(mission, activeGroup.m_sGroupId)) || changed;
				continue;
			}

			if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
			{
				changed = CleanupTerminalActiveGroupRuntime(state, activeGroup, "ensure runtime") || changed;
				continue;
			}

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
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return false;
		if (IsLocalSecurityPatrolClaimant(state, activeGroup))
			return false;
		if (IsExactGarrisonPatrolGroup(state, activeGroup))
			return false;
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
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

		bool changed;
		SCR_AIGroup group = SCR_AIGroup.Cast(GetRuntimeCrewGroupEntity(activeGroup.m_sGroupId));
		if (group)
			ReconcileRuntimeGroupEditableMembership(group, activeGroup, source + " count reconcile");

		int liveInfantry = CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId);
		int liveVehicles = CountAliveRuntimeGroupVehicles(activeGroup.m_sGroupId);
		int liveTotal = liveInfantry + liveVehicles;
		if (liveTotal <= 0)
			return false;
		int strategicLiveCount = liveTotal;
		if (IsMixedPersonnelVehicleActiveGroup(activeGroup))
			strategicLiveCount = liveInfantry;

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

		if (activeGroup.m_iLastSeenAliveCount != strategicLiveCount)
		{
			activeGroup.m_iLastSeenAliveCount = strategicLiveCount;
			changed = true;
		}

		int survivorInfantry = Math.Min(Math.Max(0, activeGroup.m_iInfantryCount), liveInfantry);
		int survivorVehicles = 0;
		if (activeGroup.m_iVehicleCount > 0 && activeGroup.m_iInfantryCount <= 0)
			survivorVehicles = Math.Min(activeGroup.m_iVehicleCount, liveVehicles);
		else if (activeGroup.m_iVehicleCount > 0)
		{
			if (liveInfantry > 0)
				survivorVehicles = Math.Min(activeGroup.m_iVehicleCount, liveVehicles);
		}

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
			DebugLogThrottled(
				"count_reconcile_" + activeGroup.m_sGroupId,
				string.Format("active group count reconcile %1 via %2 | infantry live %3/%4 | total live %5 | spawned %6 | status %7", activeGroup.m_sGroupId, ReportText(source), liveInfantry, activeGroup.m_iInfantryCount, liveTotal, activeGroup.m_iSpawnedAgentCount, activeGroup.m_sRuntimeStatus),
				30000);
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
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsLocalSecurityPatrolClaimant(state, activeGroup))
				continue;
			if (IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;
			if (ShouldHoldForceSpawnProjection(state, activeGroup))
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

	protected void NormalizeStaticActiveGroupRoute(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveGroupState activeGroup)
	{
		if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
			return;
		if (ShouldHoldForceSpawnProjection(state, activeGroup))
			return;
		if (!activeGroup || activeGroup.m_bQRF || IsMissionConvoyGroup(activeGroup))
			return;
		if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
			return;
		if (IsSupportRequestActiveGroup(activeGroup))
			return;
		if (IsTownPoliceActiveGroup(state, preset, activeGroup))
			return;
		if (activeGroup.m_sRuntimeStatus == "routing" || activeGroup.m_sRuntimeStatus == "support_active" || activeGroup.m_sRuntimeStatus == "support_recalling")
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

		return !activeGroup.m_sSupportRequestId.IsEmpty() || activeGroup.m_sSpawnFallbackMode.Contains("support");
	}

	protected bool IsExactPlayerSupportActiveGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup || activeGroup.m_sSupportRequestId.IsEmpty())
			return false;
		HST_SupportRequestState request = state.FindSupportRequest(activeGroup.m_sSupportRequestId);
		if (!request || request.m_sGroupId != activeGroup.m_sGroupId)
			return false;
		return HST_OperationService.RequiresOperation(request)
			&& HST_OperationService.IsExactPlayerSupportType(request.m_eType);
	}

	protected bool IsExactEnemyOperationGroup(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		HST_EOperationType operationType,
		HST_EEnemyOrderType orderType,
		int contractVersion)
	{
		if (!state || !activeGroup || contractVersion <= 0
			|| activeGroup.m_sEnemyOrderId.IsEmpty() || activeGroup.m_sOperationId.IsEmpty()
			|| activeGroup.m_sProjectionId.IsEmpty() || activeGroup.m_sGroupId.IsEmpty())
			return false;

		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		if (!operation || operation.m_eType != operationType || operation.m_iContractVersion != contractVersion
			|| operation.m_sOperationId != activeGroup.m_sOperationId
			|| operation.m_sEnemyOrderId != activeGroup.m_sEnemyOrderId
			|| operation.m_sGroupId != activeGroup.m_sGroupId
			|| operation.m_sProjectionId != activeGroup.m_sProjectionId)
			return false;

		HST_EnemyOrderState order = state.FindEnemyOrder(activeGroup.m_sEnemyOrderId);
		if (!order || order.m_eType != orderType || order.m_iOperationContractVersion != contractVersion
			|| order.m_sOperationId != operation.m_sOperationId
			|| order.m_sOrderId != operation.m_sEnemyOrderId
			|| order.m_sGroupId != activeGroup.m_sGroupId)
			return false;

		if (operationType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF
			|| operationType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK)
			return activeGroup.m_bQRF;
		if (operationType == HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL)
			return !activeGroup.m_bQRF;
		return true;
	}

	protected bool IsExactEnemyOperationActiveGroup(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		HST_EOperationType operationType,
		HST_EEnemyOrderType orderType,
		int contractVersion)
	{
		if (!IsExactEnemyOperationGroup(state, activeGroup, operationType, orderType, contractVersion))
			return false;
		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		return operation
			&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
	}

	protected bool IsExactEnemyQRFActiveGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		return IsExactEnemyOperationActiveGroup(
			state,
			activeGroup,
			HST_EOperationType.HST_OPERATION_TYPE_ENEMY_DEFENSIVE_QRF,
			HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF,
			HST_OperationService.EXACT_ENEMY_DEFENSIVE_QRF_CONTRACT_VERSION);
	}

	protected bool IsExactEnemyCounterattackActiveGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		return IsExactEnemyOperationActiveGroup(
			state,
			activeGroup,
			HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK,
			HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK,
			HST_OperationService.EXACT_ENEMY_COUNTERATTACK_CONTRACT_VERSION);
	}

	bool IsLocalSecurityPatrolClaimant(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;
		if (!activeGroup.m_sLocalSecurityPatrolId.IsEmpty())
			return true;

		string intrinsicHint = activeGroup.m_sGroupId + " "
			+ activeGroup.m_sSpawnFallbackMode + " "
			+ activeGroup.m_sRuntimeStatus;
		intrinsicHint.ToLower();
		if (intrinsicHint.Contains("local_security") || intrinsicHint.Contains("local-security"))
			return true;
		if (!state)
			return false;

		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		if (operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_LOCAL_SECURITY_PATROL)
			return true;

		foreach (HST_LocalSecurityPatrolState patrol : state.m_aLocalSecurityPatrols)
		{
			if (!patrol)
				continue;
			if (!activeGroup.m_sGroupId.IsEmpty() && patrol.m_sGroupId == activeGroup.m_sGroupId)
				return true;
			if (!activeGroup.m_sOperationId.IsEmpty() && patrol.m_sOperationId == activeGroup.m_sOperationId)
				return true;
			if (!activeGroup.m_sManifestId.IsEmpty() && patrol.m_sManifestId == activeGroup.m_sManifestId)
				return true;
			if (!activeGroup.m_sSpawnResultId.IsEmpty() && patrol.m_sSpawnResultId == activeGroup.m_sSpawnResultId)
				return true;
			if (!activeGroup.m_sForceId.IsEmpty() && patrol.m_sForceId == activeGroup.m_sForceId)
				return true;
			if (!activeGroup.m_sProjectionId.IsEmpty() && patrol.m_sProjectionId == activeGroup.m_sProjectionId)
				return true;
		}
		return false;
	}

	bool IsExactOpenPhysicalLocalSecurityPatrolGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup || !IsLocalSecurityPatrolClaimant(state, activeGroup))
			return false;
		if (HST_LocalSecurityOperationService.EXACT_CONTRACT_VERSION <= 0
			|| activeGroup.m_sLocalSecurityPatrolId.IsEmpty()
			|| activeGroup.m_sOperationId.IsEmpty() || activeGroup.m_sManifestId.IsEmpty()
			|| activeGroup.m_sSpawnResultId.IsEmpty() || activeGroup.m_sForceId.IsEmpty()
			|| activeGroup.m_sProjectionId.IsEmpty() || activeGroup.m_sGroupId.IsEmpty()
			|| state.FindActiveGroup(activeGroup.m_sGroupId) != activeGroup)
			return false;

		string mode = activeGroup.m_sSpawnFallbackMode;
		if (mode != HST_LocalSecurityOperationService.EXACT_GROUP_MODE
			&& !mode.StartsWith(HST_LocalSecurityOperationService.EXACT_GROUP_MODE + "_"))
			return false;
		if (!activeGroup.m_bSpawnedEntity || IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;
		if (!activeGroup.m_sEnemyOrderId.IsEmpty() || !activeGroup.m_sMissionInstanceId.IsEmpty()
			|| !activeGroup.m_sSupportRequestId.IsEmpty() || !activeGroup.m_sGarrisonZoneId.IsEmpty()
			|| activeGroup.m_bQRF)
			return false;

		HST_LocalSecurityPatrolState patrol = state.FindLocalSecurityPatrolById(activeGroup.m_sLocalSecurityPatrolId);
		if (!patrol
			|| patrol.m_iContractVersion != HST_LocalSecurityOperationService.EXACT_CONTRACT_VERSION
			|| patrol.m_sPatrolId != activeGroup.m_sLocalSecurityPatrolId
			|| patrol.m_sZoneId.IsEmpty() || patrol.m_sZoneId != activeGroup.m_sZoneId
			|| patrol.m_sFactionKey.IsEmpty() || patrol.m_sFactionKey != activeGroup.m_sFactionKey
			|| patrol.m_sOperationId != activeGroup.m_sOperationId
			|| patrol.m_sManifestId != activeGroup.m_sManifestId
			|| patrol.m_sSpawnResultId != activeGroup.m_sSpawnResultId
			|| patrol.m_sForceId != activeGroup.m_sForceId
			|| patrol.m_sProjectionId != activeGroup.m_sProjectionId
			|| patrol.m_sGroupId != activeGroup.m_sGroupId
			|| patrol.m_sStatus != "active" || patrol.m_iTerminalAtSecond > 0
			|| !patrol.m_sAuthorityFailure.IsEmpty())
			return false;

		HST_ZoneState zone = state.FindZone(patrol.m_sZoneId);
		if (!zone || zone.m_sLocalSecurityPatrolId != patrol.m_sPatrolId
			|| zone.m_sOwnerFactionKey != patrol.m_sFactionKey)
			return false;

		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		if (!operation
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_LOCAL_SECURITY_PATROL
			|| operation.m_iContractVersion != HST_LocalSecurityOperationService.EXACT_CONTRACT_VERSION
			|| operation.m_sLocalSecurityPatrolId != patrol.m_sPatrolId
			|| operation.m_sManifestId != activeGroup.m_sManifestId
			|| operation.m_sSpawnResultId != activeGroup.m_sSpawnResultId
			|| operation.m_sForceId != activeGroup.m_sForceId
			|| operation.m_sProjectionId != activeGroup.m_sProjectionId
			|| operation.m_sGroupId != activeGroup.m_sGroupId
			|| operation.m_sOwnerFactionKey != activeGroup.m_sFactionKey
			|| operation.m_sAssignmentKind != HST_LocalSecurityOperationService.ASSIGNMENT_KIND
			|| operation.m_sAssignmentZoneId != activeGroup.m_sZoneId
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			|| operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
			|| operation.m_ePositionAuthority != HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE)
			return false;

		HST_ForceManifestState manifest = state.FindForceManifest(activeGroup.m_sManifestId);
		if (!manifest || !manifest.m_bFrozen
			|| manifest.m_sManifestId != patrol.m_sManifestId
			|| manifest.m_sManifestHash.IsEmpty() || manifest.m_sManifestHash != patrol.m_sManifestHash
			|| manifest.m_sOperationId != operation.m_sOperationId
			|| manifest.m_sFactionKey != operation.m_sOwnerFactionKey
			|| manifest.m_sTargetZoneId != operation.m_sAssignmentZoneId
			|| manifest.m_sForceKind != HST_LocalSecurityOperationService.EXACT_FORCE_KIND
			|| manifest.m_sPolicyId != HST_LocalSecurityOperationService.EXACT_POLICY_ID
			|| manifest.m_sIntentId != HST_LocalSecurityOperationService.EXACT_MANIFEST_INTENT
			|| manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0]
			|| manifest.m_aGroups[0].m_sPrefab != activeGroup.m_sPrefab)
			return false;

		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(activeGroup.m_sSpawnResultId);
		if (!batch
			|| batch.m_sResultId != patrol.m_sSpawnResultId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sManifestHash != manifest.m_sManifestHash
			|| batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sForceId != operation.m_sForceId
			|| batch.m_sProjectionId != operation.m_sProjectionId
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_bStrategicProjectionHeld || batch.m_iSuccessfulHandoffCount <= 0)
			return false;

		SCR_AIGroup root = GetForceSpawnGroupRoot(activeGroup);
		return root && !root.IsDeleted()
			&& activeGroup.m_sGroupId == activeGroup.m_sProjectionId;
	}

	protected bool IsExactEnemyPatrolGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (IsExactEnemyOperationGroup(
			state,
			activeGroup,
			HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL,
			HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL,
			HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION))
			return true;
		if (!activeGroup)
			return false;
		if (activeGroup.m_sRuntimeStatus == "exact_patrol_quarantined"
			|| activeGroup.m_sRuntimeStatus == "exact_patrol_orphan_quarantined")
			return true;
		if (activeGroup.m_sSpawnFallbackMode == HST_EnemyPatrolOperationService.EXACT_GROUP_MODE
			|| activeGroup.m_sSpawnFallbackMode.StartsWith(HST_EnemyPatrolOperationService.EXACT_GROUP_MODE + "_"))
			return true;
		if (!state || activeGroup.m_sOperationId.IsEmpty())
			return false;
		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL
			|| operation.m_iContractVersion != HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION)
			return false;
		HST_EnemyOrderState order = state.FindEnemyOrder(operation.m_sEnemyOrderId);
		if (order && order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL
			&& order.m_iOperationContractVersion == HST_EnemyPatrolOperationService.QUARANTINED_CONTRACT_VERSION)
			return true;
		return false;
	}

	protected bool IsExactEnemyPatrolActiveGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		return IsExactEnemyOperationActiveGroup(
			state,
			activeGroup,
			HST_EOperationType.HST_OPERATION_TYPE_ENEMY_PATROL,
			HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL,
			HST_EnemyPatrolOperationService.EXACT_CONTRACT_VERSION);
	}

	bool IsExactMissionGuardGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		return HST_MissionGuardOperationService.IsExactMissionGuardGroup(state, activeGroup);
	}

	protected bool IsExactOrQuarantinedMissionGuardGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (HST_MissionGuardOperationService.IsMissionGuardGroupClaimant(state, activeGroup))
			return true;
		if (HST_RescuePOWOperationService.IsMissionRescueGroupClaimant(state, activeGroup))
			return true;
		if (!activeGroup)
			return false;

		// Generic systems must preserve intrinsic exact/quarantine orphan evidence,
		// but this broad isolation hint never authorizes typed runtime mutation.
		return activeGroup.m_sSpawnFallbackMode == HST_MissionGuardOperationService.EXACT_GROUP_MODE
			|| activeGroup.m_sSpawnFallbackMode == HST_MissionGuardOperationService.QUARANTINE_STATUS
			|| activeGroup.m_sRuntimeStatus.StartsWith("mission_guard_")
			|| activeGroup.m_sRuntimeStatus == HST_MissionGuardOperationService.QUARANTINE_STATUS
			|| activeGroup.m_sSpawnFallbackMode == HST_RescuePOWOperationService.EXACT_GROUP_MODE
			|| activeGroup.m_sSpawnFallbackMode == HST_RescuePOWOperationService.QUARANTINE_STATUS
			|| activeGroup.m_sRuntimeStatus.StartsWith("mission_rescue_")
			|| activeGroup.m_sRuntimeStatus == HST_RescuePOWOperationService.QUARANTINE_STATUS;
	}

	protected bool HasOpenPhysicalMissionGuardRuntimeAuthority(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return false;
		bool missionGuardClaim = HST_MissionGuardOperationService.IsMissionGuardGroupClaimant(state, activeGroup);
		bool missionRescueClaim = HST_RescuePOWOperationService.IsMissionRescueGroupClaimant(state, activeGroup);
		if (!missionGuardClaim && !missionRescueClaim)
			return false;
		if (activeGroup.m_sGroupId.IsEmpty() || activeGroup.m_sManifestId.IsEmpty()
			|| activeGroup.m_sSpawnResultId.IsEmpty() || activeGroup.m_sForceId.IsEmpty()
			|| activeGroup.m_sProjectionId.IsEmpty()
			|| state.FindActiveGroup(activeGroup.m_sGroupId) != activeGroup)
			return false;
		HST_ActiveMissionState mission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
		bool missionAuthority = missionGuardClaim
			&& HST_MissionGuardOperationService.IsExactOrQuarantinedMission(mission);
		missionAuthority = missionAuthority || (missionRescueClaim
			&& HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission));
		if (!missionAuthority
			|| mission.m_sOperationId != activeGroup.m_sOperationId
			|| mission.m_sManifestId != activeGroup.m_sManifestId
			|| mission.m_sSpawnResultId != activeGroup.m_sSpawnResultId)
			return false;
		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		bool operationTypeMatches = operation && ((missionGuardClaim
			&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD)
			|| (missionRescueClaim
				&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_RESCUE));
		return operationTypeMatches
			&& operation.m_sOperationId == activeGroup.m_sOperationId
			&& operation.m_sMissionInstanceId == mission.m_sInstanceId
			&& operation.m_sManifestId == activeGroup.m_sManifestId
			&& operation.m_sSpawnResultId == activeGroup.m_sSpawnResultId
			&& operation.m_sForceId == activeGroup.m_sForceId
			&& operation.m_sProjectionId == activeGroup.m_sProjectionId
			&& operation.m_sGroupId == activeGroup.m_sGroupId
			&& operation.m_sOwnerFactionKey == activeGroup.m_sFactionKey
			&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE
			&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
			&& operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
	}

	protected bool IsExactMissionGuardActiveGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		bool exactStationaryMissionGroup = IsExactMissionGuardGroup(state, activeGroup)
			|| HST_RescuePOWOperationService.IsExactMissionRescueGroup(state, activeGroup);
		if (!exactStationaryMissionGroup)
			return false;
		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		HST_ActiveMissionState mission = state.FindActiveMission(activeGroup.m_sMissionInstanceId);
		return operation && mission
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
	}

	bool IsExactGarrisonPatrolGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (IsExactGarrisonPatrolOperationGroup(state, activeGroup))
			return true;
		if (!activeGroup)
			return false;
		if (activeGroup.m_sRuntimeStatus.StartsWith("exact_garrison_patrol_"))
			return true;
		if (activeGroup.m_sSpawnFallbackMode == HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE
			|| activeGroup.m_sSpawnFallbackMode.StartsWith(HST_GarrisonPatrolOperationService.EXACT_GROUP_MODE + "_"))
			return true;
		if (!state || activeGroup.m_sOperationId.IsEmpty())
			return false;
		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		return operation
			&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
			&& operation.m_iContractVersion == HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION;
	}

	protected bool IsExactGarrisonPatrolOperationGroup(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup || HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION <= 0
			|| activeGroup.m_sOperationId.IsEmpty() || activeGroup.m_sManifestId.IsEmpty()
			|| activeGroup.m_sSpawnResultId.IsEmpty() || activeGroup.m_sForceId.IsEmpty()
			|| activeGroup.m_sProjectionId.IsEmpty() || activeGroup.m_sGroupId.IsEmpty()
			|| activeGroup.m_sGarrisonZoneId.IsEmpty())
			return false;

		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		if (!operation
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_GARRISON_PATROL
			|| operation.m_iContractVersion != HST_GarrisonPatrolOperationService.EXACT_CONTRACT_VERSION
			|| operation.m_iProjectionContractVersion != HST_GarrisonPatrolOperationService.EXACT_PROJECTION_CONTRACT_VERSION
			|| operation.m_sOperationId != activeGroup.m_sOperationId
			|| operation.m_sManifestId != activeGroup.m_sManifestId
			|| operation.m_sSpawnResultId != activeGroup.m_sSpawnResultId
			|| operation.m_sForceId != activeGroup.m_sForceId
			|| operation.m_sProjectionId != activeGroup.m_sProjectionId
			|| operation.m_sGroupId != activeGroup.m_sGroupId
			|| operation.m_sOwnerFactionKey.IsEmpty()
			|| operation.m_sOwnerFactionKey != activeGroup.m_sFactionKey
			|| operation.m_sAssignmentKind != HST_GarrisonPatrolOperationService.ASSIGNMENT_KIND
			|| operation.m_sAssignmentZoneId.IsEmpty()
			|| operation.m_sAssignmentZoneId != activeGroup.m_sZoneId
			|| operation.m_sAssignmentZoneId != activeGroup.m_sGarrisonZoneId)
			return false;

		HST_ForceManifestState manifest = state.FindForceManifest(activeGroup.m_sManifestId);
		if (!manifest || !manifest.m_bFrozen
			|| manifest.m_sPolicyId != HST_GarrisonPatrolOperationService.EXACT_POLICY_ID
			|| manifest.m_sForceKind != HST_GarrisonPatrolOperationService.EXACT_FORCE_KIND
			|| manifest.m_sIntentId != HST_GarrisonPatrolOperationService.EXACT_INTENT_ID
			|| manifest.m_sManifestId != operation.m_sManifestId
			|| manifest.m_sOperationId != operation.m_sOperationId
			|| manifest.m_sFactionKey != operation.m_sOwnerFactionKey
			|| manifest.m_sTargetZoneId != operation.m_sAssignmentZoneId
			|| manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0]
			|| manifest.m_aGroups[0].m_sPrefab != activeGroup.m_sPrefab)
			return false;

		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(activeGroup.m_sSpawnResultId);
		if (!batch
			|| batch.m_sResultId != operation.m_sSpawnResultId
			|| batch.m_sManifestId != operation.m_sManifestId
			|| batch.m_sManifestHash != manifest.m_sManifestHash
			|| batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sForceId != operation.m_sForceId
			|| batch.m_sProjectionId != operation.m_sProjectionId)
			return false;

		HST_GarrisonState garrison = state.FindGarrison(operation.m_sAssignmentZoneId, operation.m_sOwnerFactionKey);
		if (!garrison || !garrison.m_aAcceptedManifestIds.Contains(manifest.m_sManifestId))
			return false;

		return activeGroup.m_sGroupId == activeGroup.m_sProjectionId
			&& activeGroup.m_sCompositionIntentId == HST_GarrisonPatrolOperationService.EXACT_INTENT_ID
			&& activeGroup.m_sEnemyOrderId.IsEmpty() && activeGroup.m_sMissionInstanceId.IsEmpty()
			&& activeGroup.m_sSupportRequestId.IsEmpty() && !activeGroup.m_bQRF;
	}

	protected bool IsExactGarrisonPatrolActiveGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!IsExactGarrisonPatrolOperationGroup(state, activeGroup))
			return false;
		HST_OperationRecordState operation = state.FindOperation(activeGroup.m_sOperationId);
		return operation
			&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			&& operation.m_eTerminalResult == HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
	}

	protected bool IsLiveConfirmedOperationRouteGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		return IsSupportRequestActiveGroup(activeGroup)
			|| IsExactEnemyQRFActiveGroup(state, activeGroup)
			|| IsExactEnemyCounterattackActiveGroup(state, activeGroup)
			|| IsExactEnemyPatrolActiveGroup(state, activeGroup)
			|| IsExactGarrisonPatrolActiveGroup(state, activeGroup);
	}

	protected bool CanSimulateUnspawnedActiveGroupRoute(HST_ActiveGroupState activeGroup)
	{
		if (IsForceSpawnQueueManaged(activeGroup))
			return false;
		if (!activeGroup || activeGroup.m_bSpawnedEntity)
			return false;
		if (!IsSupportRequestActiveGroup(activeGroup))
			return false;

		return activeGroup.m_sRuntimeStatus == "support_active" || activeGroup.m_sRuntimeStatus == "support_recalling";
	}

	protected bool ShouldDeferActiveGroupRuntimePhysicalization(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup)
			return false;
		if (!IsSupportRequestActiveGroup(activeGroup))
			return false;
		if (activeGroup.m_bSpawnedEntity || !activeGroup.m_sRuntimeEntityId.IsEmpty())
			return false;
		if (IsTerminalActiveGroupRuntimeStatus(activeGroup))
			return false;
		if (activeGroup.m_sRuntimeStatus == "spawn_pending_agents")
			return false;
		if (HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		vector position = ResolveActiveGroupRuntimePhysicalizationPosition(activeGroup);
		if (IsZeroVector(position))
			return false;

		return !HST_WorldPositionService.IsPositionInsidePlayerEventBubble(position);
	}

	protected vector ResolveActiveGroupRuntimePhysicalizationPosition(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "0 0 0";

		if (!IsZeroVector(activeGroup.m_vPosition))
			return activeGroup.m_vPosition;
		if (!IsZeroVector(activeGroup.m_vTargetPosition))
			return activeGroup.m_vTargetPosition;
		if (!IsZeroVector(activeGroup.m_vSourcePosition))
			return activeGroup.m_vSourcePosition;

		return "0 0 0";
	}

	protected bool MarkActiveGroupRuntimePhysicalizationDeferred(HST_ActiveGroupState activeGroup, HST_CampaignState state)
	{
		if (!activeGroup)
			return false;

		bool changed;
		string deferredMode = AppendActiveGroupSpawnModeToken(activeGroup.m_sSpawnFallbackMode, "runtime_deferred_player_bubble");
		if (activeGroup.m_sSpawnFallbackMode != deferredMode)
		{
			activeGroup.m_sSpawnFallbackMode = deferredMode;
			changed = true;
		}

		string deferredReason = "Runtime physicalization deferred outside the player event bubble; campaign state remains simulated.";
		if (activeGroup.m_sSpawnFailureReason != deferredReason)
		{
			activeGroup.m_sSpawnFailureReason = deferredReason;
			changed = true;
		}

		if (changed)
		{
			m_bMarkerRefreshNeeded = true;
			DebugLog(string.Format("active group runtime deferred %1 at %2 | status %3", activeGroup.m_sGroupId, ResolveActiveGroupRuntimePhysicalizationPosition(activeGroup), activeGroup.m_sRuntimeStatus));
		}

		return changed;
	}

	protected bool UpdateRuntimeGroupSurvivors(HST_CampaignState state, bool missionConvoysOnly = false)
	{
		if (!state)
			return false;

		bool changed;
		ref array<string> reconciledExactMissionIds = {};
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || !state.IsOperationalActiveGroup(activeGroup) || !activeGroup.m_bSpawnedEntity
				|| activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed")
				continue;
			bool missionConvoyGroup = IsMissionConvoyGroup(activeGroup);
			if (missionConvoysOnly && !missionConvoyGroup)
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsLocalSecurityPatrolClaimant(state, activeGroup))
				continue;
			if (ShouldHoldForceSpawnProjection(state, activeGroup))
				continue;
			if (IsExactEnemyPatrolGroup(state, activeGroup)
				|| IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;
			// Exact player support casualties and terminal cleanup are owned by the
			// slot-mapped force adapter. Generic aggregate counting can otherwise
			// delete the root before a dead member is durably retired.
			if (IsExactPlayerSupportActiveGroup(state, activeGroup))
				continue;
			if (missionConvoyGroup && ShouldSpawnMissionConvoyRuntime(state, activeGroup))
				continue;

			HST_ActiveMissionState convoyMission;
			if (missionConvoyGroup)
				convoyMission = FindMissionForConvoyGroup(state, activeGroup);
			if (IsExactMissionConvoyContract(convoyMission))
			{
				if (!reconciledExactMissionIds.Contains(convoyMission.m_sInstanceId))
				{
					reconciledExactMissionIds.Insert(convoyMission.m_sInstanceId);
					changed = ReconcileExactMissionConvoyMappedSurvivors(state, convoyMission) || changed;
				}
				continue;
			}
			if (missionConvoyGroup)
			{
				if (TryRepairMissionConvoyCrewPopulation(state, convoyMission, activeGroup, "survivor update"))
					changed = true;
			}
			else if (TryRepairEmptyRuntimeGroupPopulation(state, activeGroup, "survivor update"))
				changed = true;

			EnsureActiveGroupRuntimeFaction(activeGroup, "survivor update");
			EnforceOpposingOccupiedVehicleAccess(state, activeGroup);
			if (RefreshActiveGroupLivePositionFromRuntime(state, activeGroup, missionConvoyGroup))
				changed = true;
			if (ReconcileActiveGroupRuntimeMemberCounts(state, activeGroup, "survivor update"))
				changed = true;
			if (!missionConvoyGroup && TryEliminateCrewlessMixedActiveGroup(state, activeGroup, "survivor update"))
			{
				changed = true;
				continue;
			}
			int aliveCount;
			if (missionConvoyGroup)
				aliveCount = CountAliveRuntimeCrewAgents(activeGroup);
			else if (activeGroup.m_iVehicleCount > 0 && activeGroup.m_iInfantryCount <= 0)
				aliveCount = CountAliveRuntimeGroupVehicles(activeGroup.m_sGroupId);
			else if (IsMixedPersonnelVehicleActiveGroup(activeGroup))
				aliveCount = CountAliveRuntimeInfantryGroupAgents(activeGroup.m_sGroupId);
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
			int deadTrackedMembers = CountDeadTrackedRuntimeGroupMembers(activeGroup.m_sGroupId);
			if (aliveCount <= 0 && activeGroup.m_iSpawnedAgentCount <= 0 && activeGroup.m_iLastSeenAliveCount <= 0 && deadTrackedMembers <= 0)
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
				activeGroup.m_iSurvivorVehicleCount = Math.Min(activeGroup.m_iVehicleCount, CountAliveRuntimeGroupVehicles(activeGroup.m_sGroupId));
			}
			else if (IsMixedPersonnelVehicleActiveGroup(activeGroup))
			{
				activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, aliveCount);
				activeGroup.m_iSurvivorVehicleCount = Math.Min(activeGroup.m_iVehicleCount, CountAliveRuntimeGroupVehicles(activeGroup.m_sGroupId));
			}
			else
			{
				activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, aliveCount);
				activeGroup.m_iSurvivorVehicleCount = 0;
			}
			if (aliveCount <= 0)
			{
				activeGroup.m_bEverPopulated = activeGroup.m_bEverPopulated || activeGroup.m_iSpawnedAgentCount > 0;
				activeGroup.m_bSpawnCompleted = activeGroup.m_bSpawnCompleted || activeGroup.m_iSpawnedAgentCount > 0;
				if (missionConvoyGroup)
					activeGroup.m_sRuntimeStatus = MISSION_CONVOY_ELIMINATED;
				else
					activeGroup.m_sRuntimeStatus = "eliminated";
				activeGroup.m_iSpawnedAgentCount = 0;
				activeGroup.m_iLastSeenAliveCount = 0;
				activeGroup.m_iSurvivorInfantryCount = 0;
				activeGroup.m_iSurvivorVehicleCount = 0;
				activeGroup.m_iDurableLivingInfantryCount = 0;
				activeGroup.m_iLastCasualtySecond = Math.Max(activeGroup.m_iLastCasualtySecond, state.m_iElapsedSeconds);
				activeGroup.m_iEliminatedAtSecond = Math.Max(activeGroup.m_iEliminatedAtSecond, state.m_iElapsedSeconds);
				activeGroup.m_iLifecycleRevision++;
				CleanupTerminalActiveGroupRuntime(state, activeGroup, "survivor update");
				foreach (HST_QRFState terminalQRF : state.m_aQRFs)
				{
					if (terminalQRF && terminalQRF.m_sGroupId == activeGroup.m_sGroupId)
						FailTerminalLinkedQRF(state, terminalQRF, "survivor update");
				}
				if (activeGroup.m_bQRF || IsSupportRequestActiveGroup(activeGroup)
					|| IsExactEnemyPatrolGroup(state, activeGroup)
					|| IsExactGarrisonPatrolGroup(state, activeGroup))
					m_bMarkerRefreshNeeded = true;
			}
			DebugLogThrottled(
				"survivors_" + activeGroup.m_sGroupId,
				string.Format("active group survivors %1 | zone %2 | alive %3 from %4 | survivors infantry %5/%6 vehicles %7/%8 | status %9", activeGroup.m_sGroupId, activeGroup.m_sZoneId, aliveCount, previousAliveCount, activeGroup.m_iSurvivorInfantryCount, activeGroup.m_iInfantryCount, activeGroup.m_iSurvivorVehicleCount, activeGroup.m_iVehicleCount, activeGroup.m_sRuntimeStatus),
				30000);
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

	protected bool RefreshActiveGroupLivePositionFromRuntime(
		HST_CampaignState state,
		HST_ActiveGroupState activeGroup,
		bool missionConvoyGroup)
	{
		if (!activeGroup)
			return false;

		vector livePosition = ResolveActiveGroupLiveRuntimePosition(activeGroup, missionConvoyGroup);
		if (IsZeroVector(livePosition))
			return false;

		if (!IsZeroVector(activeGroup.m_vPosition) && DistanceSq2D(activeGroup.m_vPosition, livePosition) < 4.0)
			return false;

		activeGroup.m_vPosition = livePosition;
		if (activeGroup.m_bQRF || IsSupportRequestActiveGroup(activeGroup) || missionConvoyGroup
			|| IsExactEnemyPatrolGroup(state, activeGroup)
			|| IsExactGarrisonPatrolGroup(state, activeGroup)
			|| IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
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

	protected int CountDeadTrackedRuntimeGroupMembers(string groupId)
	{
		if (groupId.IsEmpty())
			return 0;

		int deadCount;
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId || i >= m_aRuntimeGroupEntities.Count())
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity || AIGroup.Cast(entity))
				continue;
			if (!IsLivingEntity(entity))
				deadCount++;
		}

		return deadCount;
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

		return livingInfantry.Count() + CountAliveRuntimeGroupVehicles(groupId);
	}

	protected int CountAliveRuntimeGroupVehicles(string groupId)
	{
		if (groupId.IsEmpty())
			return 0;

		int vehicleAliveCount;
		for (int j = 0; j < m_aRuntimeVehicleGroupIds.Count(); j++)
		{
			if (m_aRuntimeVehicleGroupIds[j] != groupId || j >= m_aRuntimeVehicleEntities.Count())
				continue;

			IEntity vehicle = m_aRuntimeVehicleEntities[j];
			if (IsLivingEntity(vehicle))
				vehicleAliveCount++;
		}

		return vehicleAliveCount;
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
			if (!activeGroup)
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsLocalSecurityPatrolClaimant(state, activeGroup))
				continue;
			if (activeGroup.m_bQRF)
				continue;
			if (IsExactPlayerSupportActiveGroup(state, activeGroup))
				continue;
			if (IsMissionOwnedActiveGroup(activeGroup))
				continue;
			if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;
			if (activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;
			if (activeGroup.m_sFactionKey != zone.m_sOwnerFactionKey)
				continue;
			if (IsTownSecurityPoliceProjection(activeGroup))
				continue;
			if (ShouldHoldForceSpawnProjection(state, activeGroup))
				return true;
			if (activeGroup.m_sRuntimeStatus == "eliminated")
				continue;
			if (activeGroup.m_sRuntimeStatus == "spawn_failed")
				continue;
			if (activeGroup.m_sRuntimeStatus == "folded")
				continue;

			if (activeGroup.m_iLastSeenAliveCount > 0)
				return true;
			if (activeGroup.m_iSurvivorInfantryCount > 0)
				return true;
			if (activeGroup.m_iSurvivorVehicleCount > 0)
				return true;
		}

		return false;
	}

	protected bool HasHeldForceSpawnGarrisonProjection(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return false;

		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_bQRF || IsMissionOwnedActiveGroup(activeGroup)
				|| IsExactPlayerSupportActiveGroup(state, activeGroup))
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsLocalSecurityPatrolClaimant(state, activeGroup))
				continue;
			if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;
			if (activeGroup.m_sGarrisonZoneId != zone.m_sZoneId || activeGroup.m_sFactionKey != zone.m_sOwnerFactionKey)
				continue;
			if (ShouldHoldForceSpawnProjection(state, activeGroup))
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
			if (!activeGroup || activeGroup.m_bQRF || IsMissionOwnedActiveGroup(activeGroup) || activeGroup.m_sZoneId != zone.m_sZoneId
				|| IsExactPlayerSupportActiveGroup(state, activeGroup))
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;
			if (ShouldHoldForceSpawnProjection(state, activeGroup))
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
		if (!state)
			return 0;
		if (!zone)
			return 0;

		int infantryCount;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup)
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsLocalSecurityPatrolClaimant(state, activeGroup))
				continue;
			if (activeGroup.m_bQRF)
				continue;
			if (IsExactPlayerSupportActiveGroup(state, activeGroup))
				continue;
			if (IsMissionOwnedActiveGroup(activeGroup))
				continue;
			if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;
			if (activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;
			if (IsTownSecurityPoliceProjection(activeGroup))
				continue;
			if (IsForceSpawnQueueManaged(activeGroup))
				continue;
			if (activeGroup.m_sRuntimeStatus != "spawn_pending_agents")
				continue;

			infantryCount += Math.Max(0, activeGroup.m_iInfantryCount);
		}

		return infantryCount;
	}

	protected int CountPendingActiveZonePopulationGroups(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state)
			return 0;
		if (!zone)
			return 0;

		int groupCount;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup)
				continue;
			if (IsExactOrQuarantinedMissionGuardGroup(state, activeGroup))
				continue;
			if (IsLocalSecurityPatrolClaimant(state, activeGroup))
				continue;
			if (activeGroup.m_bQRF)
				continue;
			if (IsExactPlayerSupportActiveGroup(state, activeGroup))
				continue;
			if (IsMissionOwnedActiveGroup(activeGroup))
				continue;
			if (IsExactEnemyPatrolGroup(state, activeGroup) || IsExactGarrisonPatrolGroup(state, activeGroup))
				continue;
			if (activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;
			if (IsTownSecurityPoliceProjection(activeGroup))
				continue;
			if (IsForceSpawnQueueManaged(activeGroup))
				continue;
			if (activeGroup.m_sRuntimeStatus == "spawn_pending_agents")
				groupCount++;
		}

		return groupCount;
	}

	protected void FoldActiveGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		if (!state || !activeGroup || ShouldHoldForceSpawnProjection(state, activeGroup)
			|| IsExactOrQuarantinedMissionGuardGroup(state, activeGroup)
			|| IsLocalSecurityPatrolClaimant(state, activeGroup)
			|| IsExactEnemyPatrolGroup(state, activeGroup)
			|| IsExactGarrisonPatrolGroup(state, activeGroup)
			|| IsExactPlayerSupportActiveGroup(state, activeGroup))
			return;

		HST_GarrisonState garrison = state.FindGarrison(activeGroup.m_sZoneId, activeGroup.m_sFactionKey);
		if (!garrison)
		{
			garrison = new HST_GarrisonState();
			garrison.m_sGarrisonId = HST_StableIdService.BuildGarrisonId(activeGroup.m_sZoneId, activeGroup.m_sFactionKey);
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

	protected string ResolveTownPoliceGroupRouteId(HST_ZoneState zone, string fallbackRouteId)
	{
		if (zone && !zone.m_sPatrolRouteId.IsEmpty())
			return zone.m_sPatrolRouteId;

		return fallbackRouteId;
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

	protected bool DeleteRuntimeGroupEntity(string groupId, bool deleteVehicle = true)
	{
		if (groupId.IsEmpty() || IsForceSpawnRuntimeOwnershipHeldForGroup(groupId))
			return false;

		bool existed = GetRuntimeGroupEntity(groupId) != null;
		DeleteRuntimeCrewEntities(groupId);

		int removedVehicleHandles;
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
			removedVehicleHandles++;
		}

		return existed || removedVehicleHandles > 0;
	}

	protected void DeleteRuntimeCrewEntities(string groupId)
	{
		if (IsForceSpawnRuntimeOwnershipHeldForGroup(groupId))
			return;
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
		ClearExactMissionConvoyMemberMappingsForGroup(groupId);
	}

	protected void DeleteRuntimeGroupWaypoints(string groupId)
	{
		if (groupId.IsEmpty())
			return;
		AIGroup group = AIGroup.Cast(GetRuntimeCrewGroupEntity(groupId));

		for (int waypointIndex = m_aRuntimeGroupWaypointIds.Count() - 1; waypointIndex >= 0; waypointIndex--)
		{
			if (m_aRuntimeGroupWaypointIds[waypointIndex] != groupId)
				continue;

			IEntity waypointEntity;
			if (waypointIndex < m_aRuntimeGroupWaypointEntities.Count())
				waypointEntity = m_aRuntimeGroupWaypointEntities[waypointIndex];
			AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
			if (group && !group.IsDeleted() && waypoint)
				group.RemoveWaypoint(waypoint);
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

	protected bool IsZoneInsideHQActivationExclusion(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone || !state.m_bHQDeployed)
			return false;

		// The 900 m operation-staging radius must not erase whole nearby towns.
		// Setup already rejects HQ placement inside a location's capture footprint;
		// keep this smaller guard for legacy saves and emergency placement only.
		float radius = Math.Max(HQ_ZONE_ACTIVATION_FALLBACK_RADIUS_METERS, zone.m_iCaptureRadiusMeters);
		return DistanceSq2D(state.m_vHQPosition, zone.m_vPosition) <= radius * radius;
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

		Print("Partisan physical war debug | " + message);
	}

	protected void DebugLogThrottled(string key, string message, int throttleMs)
	{
		if (!m_bDebugLoggingEnabled)
			return;

		PrintThrottled("debug_" + key, "Partisan physical war debug | " + message, throttleMs);
	}

	protected void PrintThrottled(string key, string message, int throttleMs)
	{
		if (!ShouldEmitThrottled(key, throttleMs))
			return;
		Print(message);
	}

	protected bool ShouldEmitThrottled(string key, int throttleMs)
	{
		if (key.IsEmpty())
			return true;

		int now = System.GetTickCount();
		int index = m_aDebugThrottleKeys.Find(key);
		if (index >= 0)
		{
			int lastTick = m_aDebugThrottleTicks[index];
			if (now - lastTick < throttleMs)
				return false;

			m_aDebugThrottleTicks[index] = now;
			return true;
		}

		m_aDebugThrottleKeys.Insert(key);
		m_aDebugThrottleTicks.Insert(now);
		return true;
	}
}
