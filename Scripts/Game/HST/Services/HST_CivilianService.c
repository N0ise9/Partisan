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
			"Partisan undercover eligibility | %1 | eligible %2 | zone %3 | clothing %4 | weapon %5 | vehicle %6 | offroad %7 | enemy proximity %8 | wanted heat %9",
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
			"Partisan undercover enforcement | %1 | changed %2 | compromised %3 | suspicious %4 | clear %5 | blocked %6 | zone %7 | score %8 | player heat %9",
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

class HST_CivilianProjectionProofSummary
{
	bool m_bProjectionEligible;
	bool m_bTrafficConfigured;
	bool m_bAppearanceDiversityExact;
	bool m_bHornSuppressionExact;
	int m_iExpectedPedestrians;
	int m_iExpectedTrafficVehicles;
	int m_iUniquePedestrianPrefabs;
	int m_iTrafficDriverControllers;
	int m_iTrafficDriversWithHornInput;
	int m_iActorAppearances;
	int m_iUniqueActorPrefabs;
}

// Read-only identity and ordinary-runtime evidence for one ambient root. This
// is intentionally pointer based: Phase 20 may only claim rows created after
// its frozen baseline, and cleanup must never rediscover ownership by count,
// kind, or array tail.
class HST_CampaignDebugAmbientRuntimeRootSnapshot
{
	IEntity m_RootEntity;
	RplId m_RootReplicationId = RplId.Invalid();
	string m_sZoneId;
	string m_sRuntimeKind;
	string m_sFactionKey;
	string m_sVehicleRuntimeId;
	vector m_vSpawnPosition;
	vector m_vCurrentPosition;
	HST_AmbientActorRuntimeRecord m_AmbientRecord;
	HST_RuntimeVehicleState m_RuntimeVehicleRow;
	ref array<IEntity> m_aHelperEntities = {};
	ref array<int> m_aHelperRegistryIndices = {};
	string m_sActorRuntimeId;
	string m_sActorKindId;
	string m_sActorStateId;
	IEntity m_DriverEntity;
	AIGroup m_Group;
	int m_iRuntimeRegistryIndex = -1;
	int m_iActorRecordIndex = -1;
	int m_iProjectionSlot = -1;
	int m_iProjectionSeed;
	int m_iLastSampleAtSecond = -1;
	int m_iLastProgressAtSecond = -1;
	int m_iWaypointCount;
	bool m_bActorAdmitted;
	bool m_bBehaviorReady;
	bool m_bRecovering;
	bool m_bMovementObserved;
	bool m_bActiveWaypoint;
	bool m_bCleanupDeleteRequested;

	bool HasSameImmutableIdentity(
		HST_CampaignDebugAmbientRuntimeRootSnapshot other)
	{
		if (!other || m_aHelperEntities.Count() != other.m_aHelperEntities.Count()
			|| m_aHelperRegistryIndices.Count()
				!= other.m_aHelperRegistryIndices.Count())
			return false;
		for (int helperIndex; helperIndex < m_aHelperEntities.Count(); helperIndex++)
		{
			if (m_aHelperEntities[helperIndex]
				!= other.m_aHelperEntities[helperIndex]
				|| m_aHelperRegistryIndices[helperIndex]
					!= other.m_aHelperRegistryIndices[helperIndex])
				return false;
		}
		if (m_RootEntity != other.m_RootEntity)
			return false;
		if (m_RootReplicationId != other.m_RootReplicationId)
			return false;
		if (m_sZoneId != other.m_sZoneId)
			return false;
		if (m_sRuntimeKind != other.m_sRuntimeKind)
			return false;
		if (m_sFactionKey != other.m_sFactionKey)
			return false;
		if (m_sVehicleRuntimeId != other.m_sVehicleRuntimeId)
			return false;
		if (m_vSpawnPosition != other.m_vSpawnPosition)
			return false;
		if (m_AmbientRecord != other.m_AmbientRecord)
			return false;
		if (m_RuntimeVehicleRow != other.m_RuntimeVehicleRow)
			return false;
		if (m_DriverEntity != other.m_DriverEntity)
			return false;
		if (m_Group != other.m_Group)
			return false;
		if (m_iRuntimeRegistryIndex != other.m_iRuntimeRegistryIndex)
			return false;
		if (m_iActorRecordIndex != other.m_iActorRecordIndex)
			return false;
		if (m_sActorRuntimeId != other.m_sActorRuntimeId)
			return false;
		if (m_sActorKindId != other.m_sActorKindId)
			return false;
		if (m_iProjectionSlot != other.m_iProjectionSlot)
			return false;
		if (m_iProjectionSeed != other.m_iProjectionSeed)
			return false;
		return true;
	}

	bool CollidesWithImmutableIdentity(
		HST_CampaignDebugAmbientRuntimeRootSnapshot other)
	{
		if (!other)
			return false;
		if (m_RootEntity && m_RootEntity == other.m_RootEntity)
			return true;
		if (m_RootReplicationId != RplId.Invalid()
			&& m_RootReplicationId == other.m_RootReplicationId)
			return true;
		if (!m_sActorRuntimeId.IsEmpty()
			&& m_sActorRuntimeId == other.m_sActorRuntimeId)
			return true;
		return !m_sVehicleRuntimeId.IsEmpty()
			&& m_sVehicleRuntimeId == other.m_sVehicleRuntimeId;
	}

	string BuildEvidence()
	{
		string evidence = string.Format(
			"zone %1 | kind %2 | faction %3 | vehicle %4 | actor %5/%6",
			m_sZoneId,
			m_sRuntimeKind,
			m_sFactionKey,
			m_sVehicleRuntimeId,
			m_sActorRuntimeId,
			m_sActorStateId);
		return evidence + string.Format(
			" | slot/seed %1/%2 | admitted/ready/recovering %3/%4/%5 | waypoint/progress %6/%7 | moved %8 | helpers %9",
			m_iProjectionSlot,
			m_iProjectionSeed,
			m_bActorAdmitted,
			m_bBehaviorReady,
			m_bRecovering,
			m_bActiveWaypoint,
			m_iLastProgressAtSecond,
			m_bMovementObserved,
			m_aHelperEntities.Count());
	}
}

// Exact value/pointer snapshot for every mutable Civilian-service registry
// that a staged activation can touch outside the root arrays. Parallel arrays
// are captured as aligned tuples and reset-preserved bindings keep pointer
// identity; cleanup can therefore remove only proven post-baseline entries.
class HST_CampaignDebugAmbientMutableRegistrySnapshot
{
	// This sticky fault is reachable from ordinary consequence flushing while
	// Phase 20 owns ambient actors. It is frozen and compared, never cleared by
	// debug cleanup: a changed global authority fault has no safe local owner.
	bool m_bConsequenceAuthorityFault;
	ref array<string> m_aRetryZoneIds = {};
	ref array<string> m_aRetryKinds = {};
	ref array<int> m_aRetrySeconds = {};
	ref array<string> m_aStaticZoneIds = {};
	ref array<int> m_aStaticCivilianSlots = {};
	ref array<int> m_aStaticMilitarySlots = {};
	ref array<string> m_aStaticMilitaryOwnerKeys = {};
	ref array<string> m_aCasualtyZoneIds = {};
	ref array<string> m_aCasualtyEventIds = {};
	ref array<string> m_aCasualtyFactionKeys = {};
	ref array<string> m_aCasualtySourceIds = {};
	ref array<vector> m_aCasualtyPositions = {};
	ref array<int> m_aCasualtyAttempts = {};
	ref array<int> m_aCasualtyRetrySeconds = {};
	ref array<string> m_aTheftZoneIds = {};
	ref array<string> m_aTheftEventIds = {};
	ref array<string> m_aTheftFactionKeys = {};
	ref array<string> m_aTheftSourceIds = {};
	ref array<int> m_aTheftAttempts = {};
	ref array<int> m_aTheftRetrySeconds = {};
	ref array<string> m_aPanicZoneIds = {};
	ref array<vector> m_aPanicPositions = {};
	ref array<ref HST_RuntimeVehicleState> m_aResetPreservedVehicles = {};
	ref array<ref HST_VehicleCargoItemState> m_aResetPreservedCargo = {};

	string BuildEvidence()
	{
		string evidence = string.Format(
			"retry/static/casualty/theft %1/%2/%3/%4",
			m_aRetryZoneIds.Count(),
			m_aStaticZoneIds.Count(),
			m_aCasualtyEventIds.Count(),
			m_aTheftEventIds.Count());
		return evidence + string.Format(
			" | panic/reset vehicle/reset cargo %1/%2/%3 | consequence fault %4",
			m_aPanicZoneIds.Count(),
			m_aResetPreservedVehicles.Count(),
			m_aResetPreservedCargo.Count(),
			m_bConsequenceAuthorityFault);
	}
}

// Exact transient scheduler/diagnostic authority frozen before Phase 20
// replaces the ordinary ambient budget plan. The plan pointer is borrowed but
// its full deterministic content is fingerprinted, so cleanup cannot restore a
// plan object that was mutated or replaced while the debug owner was active.
class HST_CampaignDebugAmbientServiceStateSnapshot
{
	ref HST_AmbientPopulationBudgetPlan m_BudgetPlan;
	string m_sBudgetPlanFingerprint;
	int m_iNextAmbientRuntimeUpdateSecond;
	int m_iAmbientRotationEpoch;
	int m_iAmbientReconciliationCursor;
	int m_iAmbientRuntimeSequence;
	int m_iAmbientSpawnBudgetRemaining;
	int m_iRuntimeSpawnFailureCount;
	string m_sLastRuntimeSpawnFailurePrefab;
	bool m_bLastAmbientClaimObservationExact;
	bool m_bWarnedMissingCivilianCharacterPool;
	bool m_bWarnedMissingCivilianVehicleCatalog;
	int m_iStateRuntimeCivilianCharacterCount;
	int m_iStateRuntimeCivilianVehicleCount;
	int m_iStateRuntimeMilitaryVehicleCount;
	int m_iStateRuntimeSpawnFailureCount;
	string m_sStateLastRuntimeSpawnFailurePrefab;

	string BuildEvidence()
	{
		string evidence = string.Format(
			"plan %1 | next/epoch/cursor/sequence %2/%3/%4/%5",
			m_sBudgetPlanFingerprint,
			m_iNextAmbientRuntimeUpdateSecond,
			m_iAmbientRotationEpoch,
			m_iAmbientReconciliationCursor,
			m_iAmbientRuntimeSequence);
		evidence = evidence + string.Format(
			" | budget/failures %1/%2 | claim exact %3 | warnings %4/%5",
			m_iAmbientSpawnBudgetRemaining,
			m_iRuntimeSpawnFailureCount,
			m_bLastAmbientClaimObservationExact,
			m_bWarnedMissingCivilianCharacterPool,
			m_bWarnedMissingCivilianVehicleCatalog);
		return evidence + string.Format(
			" | state diagnostics %1/%2/%3/%4",
			m_iStateRuntimeCivilianCharacterCount,
			m_iStateRuntimeCivilianVehicleCount,
			m_iStateRuntimeMilitaryVehicleCount,
			m_iStateRuntimeSpawnFailureCount);
	}
}

// Exclusive service-level authority armed before Phase 20 can call either
// production mutation entrypoint. The baseline is globally empty, while the
// pointer journals are populated at native spawn/row admission time. A failed
// call therefore retains cleanup authority even when the ordinary post-call
// parallel-registry snapshot is malformed and cannot be reconstructed.
class HST_CampaignDebugCivilianMutationLease
{
	string m_sReceiptId;
	string m_sZoneId;
	string m_sFrozenZoneOwnerFactionKey;
	int m_iFrozenZoneOwnershipRevision;
	bool m_bFrozenZoneActive;
	int m_iFrozenZoneActiveInfantry;
	int m_iFrozenZoneActiveVehicles;
	ref HST_CampaignState m_State;
	ref HST_ZoneState m_Zone;
	ref HST_CivilianZoneState m_Town;
	ref HST_CampaignDebugAmbientServiceStateSnapshot m_ServiceBaseline;
	ref HST_CampaignDebugAmbientMutableRegistrySnapshot m_MutableBaseline;
	ref HST_AmbientPopulationBudgetPlan m_OwnedBudgetPlan;
	string m_sOwnedBudgetPlanFingerprint;
	ref array<IEntity> m_aSpawnedEntities = {};
	ref array<ref HST_RuntimeVehicleState> m_aRuntimeVehicleRows = {};
	ref array<IEntity> m_aPlayerClaimCollisionEntities = {};
	ref array<IEntity> m_aForeignCompartmentCollisionEntities = {};
	int m_iMutationAttemptCount;
	string m_sLastMutationKind;
	bool m_bGlobalEmptyBaselineProven;
	bool m_bPlayerClaimCollisionObserved;
	bool m_bForeignCompartmentAuthorityObserved;
	bool m_bForeignVehicleCargoAuthorityObserved;
	bool m_bCleanupStarted;

	string BuildEvidence()
	{
		string evidence = string.Format(
			"receipt %1 | zone %2 | owner/revision %3/%4 | attempts %5 | spawned entities/rows %6/%7",
			m_sReceiptId,
			m_sZoneId,
			m_sFrozenZoneOwnerFactionKey,
			m_iFrozenZoneOwnershipRevision,
			m_iMutationAttemptCount,
			m_aSpawnedEntities.Count(),
			m_aRuntimeVehicleRows.Count());
		return evidence + string.Format(
			" | claim/compartment collisions %1/%2 | foreign cargo %3 | cleanup started %4",
			m_aPlayerClaimCollisionEntities.Count(),
			m_aForeignCompartmentCollisionEntities.Count(),
			m_bForeignVehicleCargoAuthorityObserved,
			m_bCleanupStarted);
	}
}

class HST_CivilianService
{
	static const int MIN_CIVILIAN_CHARACTER_PREFABS = 1;
	static const int MINOR_LOCALITY_CIVILIAN_COUNT = 2;
	static const int CIVILIAN_APPEARANCE_SEED_SALT = 1701;
	static const int AMBIENT_SPAWN_TRANSACTIONS_PER_UPDATE = 4;
	static const float AMBIENT_PROGRESS_DISTANCE_METERS = 2.0;
	static const int AMBIENT_DAY_DENSITY_BASIS_POINTS = 10000;
	static const int AMBIENT_TWILIGHT_DENSITY_BASIS_POINTS = 7000;
	static const int AMBIENT_NIGHT_DENSITY_BASIS_POINTS = 4500;
	static const int AMBIENT_MIN_SAFETY_DENSITY_BASIS_POINTS = 2500;
	static const int AMBIENT_GLOBAL_ACTOR_BUDGET_MAXIMUM = 256;
	static const int AMBIENT_GLOBAL_TRAFFIC_BUDGET_MAXIMUM = 64;
	static const int AMBIENT_MINIMUM_ALLOCATION_LEASE_SECONDS = 120;
	static const string AMBIENT_PEDESTRIAN_RECOVERY_ACK_REASON = "pedestrian recovery awaiting active waypoint";

	static const int HEAT_DECAY_SECONDS = 300;
	static const int VEHICLE_HEAT_DECAY_SECONDS = 300;
	static const int VEHICLE_REPORT_DEFAULT_SECONDS = 300;
	static const int UNDERCOVER_RECHECK_SECONDS = 20;
	static const int UNDERCOVER_ROADBLOCK_SCAN_COOLDOWN_SECONDS = 30;
	static const int UNDERCOVER_POLICE_SCAN_COOLDOWN_SECONDS = 45;
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
	static const float CIVILIAN_PANIC_FLEE_MIN_DISTANCE_METERS = 45.0;
	static const int CIVILIAN_PANIC_FLEE_DISTANCE_VARIANCE_METERS = 25;
	static const float CIVILIAN_PANIC_WAYPOINT_RADIUS_METERS = 5.0;
	static const int CIVILIAN_LOCAL_THREAT_PANIC_SECONDS = 30;
	static const int MAX_PENDING_CIVILIAN_CASUALTIES = 256;
	static const int MAX_PENDING_CIVILIAN_THEFTS = 64;
	static const int MAX_CIVILIAN_CONSEQUENCE_TRANSACTIONS_PER_FRAME = 4;
	static const int MAX_CIVILIAN_CASUALTY_RETRIES = 3;
	static const float CIVILIAN_TRAFFIC_ROUTE_MIN_RADIUS_METERS = 220.0;
	static const int CIVILIAN_TRAFFIC_ROUTE_RADIUS_VARIANCE_METERS = 420;
	static const float CIVILIAN_TRAFFIC_WAYPOINT_RADIUS_METERS = 18.0;
	static const int TOWN_RESISTANCE_FLIP_FIA_SUPPORT = 65;
	static const int TOWN_RESISTANCE_FLIP_SUPPORT_MARGIN = 15;
	static const int TOWN_RESISTANCE_FLIP_MAX_HEAT = 5;
	static const int TOWN_ENEMY_FLIP_OCCUPIER_SUPPORT = 65;
	static const int TOWN_ENEMY_FLIP_SUPPORT_MARGIN = 15;
	static const int TOWN_ENEMY_FLIP_HEAT = 15;
	static const int OWNERSHIP_POLICY_RECONCILE_SECONDS = 5;
	static const string CIVILIAN_FACTION_KEY = "CIV";
	static const string CIVILIAN_TRAFFIC_RUNTIME_KIND = "CIV_TRAFFIC_VEHICLE";
	static const string CIVILIAN_VEHICLE_ENTITY_CATALOG = "{7C53DF3E1349C5B8}Configs/EntityCatalog/CIV/Vehicles_EntityCatalog_CIV.conf";
	static const string CIVILIAN_AI_GROUP_PREFAB = "{6985327711303920}Prefabs/Groups/CIV/HST_CivilianRuntimeEmptyGroup.et";
	static const string CIVILIAN_WANDER_WAYPOINT_PREFAB = "{FBA8DC8FDA0E770D}Prefabs/AI/Waypoints/AIWaypoint_Patrol_Hierarchy.et";
	static const string CIVILIAN_WANDER_CYCLE_WAYPOINT_PREFAB = "{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et";
	static const string CIVILIAN_FLEE_WAYPOINT_PREFAB = "{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et";
	static const string SIMONS_WOOD_ZONE_ID = "town_simons_wood";

	protected ref array<string> m_aRuntimeZoneIds = {};
	protected ref array<string> m_aRuntimeEntityZoneIds = {};
	protected ref array<string> m_aRuntimeEntityKinds = {};
	protected ref array<string> m_aRuntimeEntityFactionKeys = {};
	protected ref array<string> m_aRuntimeEntityVehicleIds = {};
	protected ref array<vector> m_aRuntimeEntitySpawnPositions = {};
	protected ref array<IEntity> m_aRuntimeEntities = {};
	protected ref array<IEntity> m_aRuntimeHelperOwners = {};
	protected ref array<IEntity> m_aRuntimeHelperEntities = {};
	protected ref array<ref HST_AmbientActorRuntimeRecord> m_aAmbientActorRecords = {};
	protected ref array<string> m_aAmbientRetryZoneIds = {};
	protected ref array<string> m_aAmbientRetryKinds = {};
	protected ref array<int> m_aAmbientRetrySeconds = {};
	protected ref array<string> m_aStaticVehicleInitializationZoneIds = {};
	protected ref array<int> m_aStaticCivilianVehicleSlotsCompleted = {};
	protected ref array<int> m_aStaticMilitaryVehicleSlotsCompleted = {};
	protected ref array<string> m_aStaticMilitaryInitializationOwnerKeys = {};
	protected ref array<string> m_aPendingCivilianCasualtyZoneIds = {};
	protected ref array<string> m_aPendingCivilianCasualtyEventIds = {};
	protected ref array<string> m_aPendingCivilianCasualtyFactionKeys = {};
	protected ref array<string> m_aPendingCivilianCasualtySourceIds = {};
	protected ref array<vector> m_aPendingCivilianCasualtyPositions = {};
	protected ref array<int> m_aPendingCivilianCasualtyAttempts = {};
	protected ref array<int> m_aPendingCivilianCasualtyRetrySeconds = {};
	protected ref array<string> m_aPendingCivilianTheftZoneIds = {};
	protected ref array<string> m_aPendingCivilianTheftEventIds = {};
	protected ref array<string> m_aPendingCivilianTheftFactionKeys = {};
	protected ref array<string> m_aPendingCivilianTheftSourceIds = {};
	protected ref array<int> m_aPendingCivilianTheftAttempts = {};
	protected ref array<int> m_aPendingCivilianTheftRetrySeconds = {};
	protected bool m_bPendingCivilianConsequenceAuthorityFault;
	protected ref array<string> m_aCivilianPanicThreatZoneIds = {};
	protected ref array<vector> m_aCivilianPanicThreatPositions = {};
	// Promoted ambient roots leave projection ownership immediately, but retain
	// this lightweight live binding so every checkpoint can snapshot their
	// current transform and terminal destruction state.
	protected ref array<ref HST_RuntimeVehicleState> m_aResetPreservedPlayerVehicles = {};
	protected ref array<ref HST_VehicleCargoItemState> m_aResetPreservedPlayerVehicleCargo = {};
	protected ref HST_CampaignState m_NewCampaignResetPreparedState;
	protected ref HST_PersistentFieldVehicleNewCampaignResetPlan
		m_NewCampaignResetFieldVehiclePlan;
	protected bool m_bNewCampaignResetPrepared;
	protected ref HST_AmbientPopulationBudgetService m_AmbientBudget = new HST_AmbientPopulationBudgetService();
	protected ref HST_AmbientActorRuntimeService m_AmbientRuntime = new HST_AmbientActorRuntimeService();
	protected ref HST_AmbientPopulationBudgetPlan m_LastAmbientBudgetPlan;
	protected int m_iNextAmbientRuntimeUpdateSecond;
	protected int m_iAmbientRotationEpoch;
	protected int m_iAmbientReconciliationCursor;
	protected int m_iAmbientRuntimeSequence;
	protected int m_iAmbientSpawnBudgetRemaining;
	protected int m_iRuntimeSpawnFailureCount;
	protected string m_sLastRuntimeSpawnFailurePrefab;
	protected bool m_bLastAmbientClaimObservationExact = true;
	protected ref HST_CombatPresenceService m_CombatPresence = new HST_CombatPresenceService();
	protected bool m_bWarnedMissingCivilianCharacterPool;
	protected bool m_bWarnedMissingCivilianVehicleCatalog;
	protected ref HST_CampaignDebugCivilianMutationLease
		m_CampaignDebugCivilianMutationLease;
	protected int m_iCampaignDebugCivilianMutationLeaseSequence;
	protected HST_StrategicService m_Strategic;
	protected HST_OwnershipTransitionService m_OwnershipTransitions;
	protected HST_TownInfluenceService m_TownInfluence;
	protected HST_CivilianConsequenceService m_CivilianConsequences;
	protected HST_PersistentFieldVehicleRuntimeService m_PersistentFieldVehicles;
	protected HST_CampaignPreset m_Preset;

	void SetStrategicService(HST_StrategicService strategic)
	{
		m_Strategic = strategic;
	}

	void SetCampaignPreset(HST_CampaignPreset preset)
	{
		m_Preset = preset;
	}

	void SetTownInfluenceService(HST_TownInfluenceService townInfluence)
	{
		m_TownInfluence = townInfluence;
	}

	void SetCivilianConsequenceService(
		HST_CivilianConsequenceService civilianConsequences)
	{
		m_CivilianConsequences = civilianConsequences;
	}

	void SetPersistentFieldVehicleRuntimeService(
		HST_PersistentFieldVehicleRuntimeService persistentFieldVehicles)
	{
		m_PersistentFieldVehicles = persistentFieldVehicles;
	}

	void SetCombatPresenceService(HST_CombatPresenceService combatPresence)
	{
		if (combatPresence)
			m_CombatPresence = combatPresence;
	}

	// The native destruction callback only admits an immutable receipt. Durable
	// town/population mutation is drained at the next server frame before the
	// persistence tick, so native callback re-entry cannot partially apply it.
	bool ObserveAmbientCivilianDestroyed(
		HST_CampaignState state,
		notnull SCR_InstigatorContextData context)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!state || !m_CivilianConsequences)
			return false;
		SCR_ECharacterDeathStatusRelations relation
			= context.GetVictimKillerRelation();
		if (relation == SCR_ECharacterDeathStatusRelations.DELETED
			|| relation == SCR_ECharacterDeathStatusRelations.DELETED_BY_EDITOR)
			return false;
		IEntity victim = context.GetVictimEntity();
		if (!victim)
			return false;
		HST_AmbientActorRuntimeRecord record
			= FindAmbientCivilianRecordForVictim(victim);
		if (!record || record.m_bCasualtyObserved)
			return false;
		RetainPendingCasualtyObservation(
			record,
			ResolveCivilianConsequenceFaction(context),
			victim.GetOrigin());
		TryAdmitRetainedCasualtyObservation(state, record);
		SetCivilianPanicThreat(record.m_sZoneId, victim.GetOrigin());
		return true;
	}

	bool FlushPendingCivilianConsequences(HST_CampaignState state)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!state || !m_CivilianConsequences)
			return false;
		foreach (HST_AmbientActorRuntimeRecord retainedRecord : m_aAmbientActorRecords)
		{
			if (retainedRecord && retainedRecord.m_bCasualtyAdmissionPending)
				TryAdmitRetainedCasualtyObservation(state, retainedRecord);
		}
		if (!HasExactPendingCivilianCasualtyArrays()
			|| !HasExactPendingCivilianTheftArrays())
		{
			if (!m_bPendingCivilianConsequenceAuthorityFault)
			{
				Print(
					"Partisan civilians | pending consequence queue authority is inconsistent",
					LogLevel.ERROR);
			}
			m_bPendingCivilianConsequenceAuthorityFault = true;
			return false;
		}
		bool changed;
		int index;
		int transactionCount;
		while (index < m_aPendingCivilianCasualtyZoneIds.Count()
			&& transactionCount
				< MAX_CIVILIAN_CONSEQUENCE_TRANSACTIONS_PER_FRAME)
		{
			if (state.m_iElapsedSeconds
				< m_aPendingCivilianCasualtyRetrySeconds[index])
			{
				index++;
				continue;
			}
			transactionCount++;
			HST_CivilianConsequenceResult result
				= m_CivilianConsequences.RegisterPedestrianCasualty(
					state,
					m_aPendingCivilianCasualtyZoneIds[index],
					m_aPendingCivilianCasualtyEventIds[index],
					m_aPendingCivilianCasualtyFactionKeys[index],
					m_aPendingCivilianCasualtySourceIds[index]);
			if (result)
				changed = result.m_bChanged || changed;
			bool accepted = result && result.m_bAccepted;
			if (accepted)
			{
				RemovePendingCivilianCasualtyAt(index);
				continue;
			}
			int casualtyAttempt = m_aPendingCivilianCasualtyAttempts[index];
			if (casualtyAttempt < int.MAX - 1)
				casualtyAttempt++;
			m_aPendingCivilianCasualtyAttempts[index] = casualtyAttempt;
			int boundedAttempt = Math.Min(
				MAX_CIVILIAN_CASUALTY_RETRIES,
				m_aPendingCivilianCasualtyAttempts[index]);
			int retryDelay = Math.Max(1, boundedAttempt * 5);
			int retrySecond = int.MAX;
			if (state.m_iElapsedSeconds <= int.MAX - retryDelay)
				retrySecond = state.m_iElapsedSeconds + retryDelay;
			m_aPendingCivilianCasualtyRetrySeconds[index] = retrySecond;
			if (m_aPendingCivilianCasualtyAttempts[index] == 1
				|| m_aPendingCivilianCasualtyAttempts[index]
					== MAX_CIVILIAN_CASUALTY_RETRIES)
			{
				string failure = "unknown";
				if (result && !result.m_sFailureReason.IsEmpty())
					failure = result.m_sFailureReason;
				Print(string.Format(
					"Partisan civilians | casualty consequence retained for retry in %1: %2",
					m_aPendingCivilianCasualtyZoneIds[index],
					failure), LogLevel.ERROR);
			}
			index++;
		}

		index = 0;
		while (index < m_aPendingCivilianTheftZoneIds.Count()
			&& transactionCount
				< MAX_CIVILIAN_CONSEQUENCE_TRANSACTIONS_PER_FRAME)
		{
			if (state.m_iElapsedSeconds
				< m_aPendingCivilianTheftRetrySeconds[index])
			{
				index++;
				continue;
			}
			transactionCount++;
			HST_CivilianConsequenceResult theftResult
				= m_CivilianConsequences.RegisterCivilianVehicleTheft(
					state,
					m_aPendingCivilianTheftZoneIds[index],
					m_aPendingCivilianTheftEventIds[index],
					m_aPendingCivilianTheftFactionKeys[index],
					m_aPendingCivilianTheftSourceIds[index]);
			if (theftResult)
				changed = theftResult.m_bChanged || changed;
			if (theftResult && theftResult.m_bAccepted)
			{
				RemovePendingCivilianTheftAt(index);
				continue;
			}
			int theftAttempt = m_aPendingCivilianTheftAttempts[index];
			if (theftAttempt < int.MAX - 1)
				theftAttempt++;
			m_aPendingCivilianTheftAttempts[index] = theftAttempt;
			int boundedTheftAttempt = Math.Min(
				MAX_CIVILIAN_CASUALTY_RETRIES,
				theftAttempt);
			int theftRetryDelay = Math.Max(1, boundedTheftAttempt * 5);
			int theftRetrySecond = int.MAX;
			if (state.m_iElapsedSeconds <= int.MAX - theftRetryDelay)
				theftRetrySecond = state.m_iElapsedSeconds + theftRetryDelay;
			m_aPendingCivilianTheftRetrySeconds[index] = theftRetrySecond;
			if (theftAttempt == 1
				|| theftAttempt == MAX_CIVILIAN_CASUALTY_RETRIES)
			{
				string theftFailure = "unknown";
				if (theftResult
					&& !theftResult.m_sFailureReason.IsEmpty())
					theftFailure = theftResult.m_sFailureReason;
				Print(string.Format(
					"Partisan civilians | theft consequence retained for retry in %1: %2",
					m_aPendingCivilianTheftZoneIds[index],
					theftFailure), LogLevel.ERROR);
			}
			index++;
		}
		return changed;
	}

	bool TickCivilianCombatConsequences(HST_CampaignState state)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!state || !m_CivilianConsequences)
			return false;
		bool changed;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || !IsCivilianLocality(zone)
				|| zone.m_iCombatPresenceRevision <= 0)
				continue;
			bool dangerFacts = zone.m_iCombatPresenceCurrentOperationCount > 0
				|| zone.m_iCombatPresenceRecentFireCount > 0;
			if (!dangerFacts && !zone.m_bCivilianCombatDangerActive
				&& zone.m_iCivilianLastCombatPresenceRevision
					== zone.m_iCombatPresenceRevision
				&& zone.m_iCivilianLastAppliedCombatEpisodeCount
					== zone.m_iCivilianCombatEpisodeCount
				&& (zone.m_iCivilianPanicUntilSecond == 0
					|| zone.m_iCivilianPanicUntilSecond
						> state.m_iElapsedSeconds))
				continue;
			HST_CivilianConsequenceResult result
				= m_CivilianConsequences.ObserveNearbyCombat(
					state,
					zone.m_sZoneId,
					zone.m_iCombatPresenceRevision,
					zone.m_iCombatPresenceCurrentOperationCount,
					zone.m_iCombatPresenceRecentFireCount,
					zone.m_sCombatPresenceContributorHash);
			if (result)
				changed = result.m_bChanged || changed;
			if (zone.m_bCivilianCombatDangerActive
				|| zone.m_iCivilianPanicUntilSecond > state.m_iElapsedSeconds)
				SetCivilianPanicThreat(zone.m_sZoneId, zone.m_vPosition);
		}
		return changed;
	}

	protected HST_AmbientActorRuntimeRecord FindAmbientCivilianRecordForVictim(
		IEntity victim)
	{
		if (!victim)
			return null;
		foreach (HST_AmbientActorRuntimeRecord record : m_aAmbientActorRecords)
		{
			if (!record)
				continue;
			if (record.m_sKindId
					== HST_AmbientActorRuntimeService.KIND_PEDESTRIAN
				&& record.m_RootEntity == victim)
				return record;
			if (record.m_sKindId
					== HST_AmbientActorRuntimeService.KIND_TRAFFIC
				&& record.m_DriverEntity == victim)
				return record;
		}
		return null;
	}

	protected string BuildAmbientCasualtyEventId(
		HST_CampaignState state,
		HST_AmbientActorRuntimeRecord record)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return "";
		if (!state || !record || record.m_sRuntimeId.IsEmpty())
			return "";
		// The ambient runtime sequence is session-only and may repeat after a
		// restart. Allocate the receipt from persisted campaign authority so a new
		// physical civilian can never alias an older casualty event.
		return HST_StableIdService.NextId(state, "civilian_casualty");
	}

	protected void RetainPendingCasualtyObservation(
		HST_AmbientActorRuntimeRecord record,
		string factionKey,
		vector position)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return;
		if (!record)
			return;
		record.m_bCasualtyObserved = true;
		record.m_bCasualtyAdmissionPending = true;
		record.m_sPendingCasualtyFactionKey = factionKey.Trim();
		record.m_vPendingCasualtyPosition = position;
	}

	protected bool TryAdmitRetainedCasualtyObservation(
		HST_CampaignState state,
		HST_AmbientActorRuntimeRecord record)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!state || !record || !record.m_bCasualtyAdmissionPending)
			return false;
		if (!HasExactPendingCivilianCasualtyArrays())
		{
			m_bPendingCivilianConsequenceAuthorityFault = true;
			return false;
		}
		if (m_aPendingCivilianCasualtyZoneIds.Count()
			>= MAX_PENDING_CIVILIAN_CASUALTIES)
			return false;
		string eventId = BuildAmbientCasualtyEventId(state, record);
		if (eventId.IsEmpty())
		{
			m_bPendingCivilianConsequenceAuthorityFault = true;
			return false;
		}
		m_aPendingCivilianCasualtyZoneIds.Insert(record.m_sZoneId);
		m_aPendingCivilianCasualtyEventIds.Insert(eventId);
		m_aPendingCivilianCasualtyFactionKeys.Insert(
			record.m_sPendingCasualtyFactionKey);
		m_aPendingCivilianCasualtySourceIds.Insert(record.m_sRuntimeId);
		m_aPendingCivilianCasualtyPositions.Insert(
			record.m_vPendingCasualtyPosition);
		m_aPendingCivilianCasualtyAttempts.Insert(0);
		m_aPendingCivilianCasualtyRetrySeconds.Insert(0);
		record.m_bCasualtyAdmissionPending = false;
		record.m_sPendingCasualtyFactionKey = "";
		record.m_sCasualtyReceiptId = eventId;
		return true;
	}

	protected bool QueueAmbientCivilianCasualtyFallback(
		HST_CampaignState state,
		HST_AmbientActorRuntimeRecord record,
		IEntity victim)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!state || !record || record.m_bCasualtyObserved || !victim
			|| !victim.GetWorld() || !ChimeraCharacter.Cast(victim))
			return false;
		CharacterControllerComponent controller
			= ChimeraCharacter.Cast(victim).GetCharacterController();
		if (!controller || controller.GetLifeState() != ECharacterLifeState.DEAD)
			return false;
		RetainPendingCasualtyObservation(record, "", victim.GetOrigin());
		bool admitted = TryAdmitRetainedCasualtyObservation(state, record);
		SetCivilianPanicThreat(record.m_sZoneId, victim.GetOrigin());
		return admitted;
	}

	protected string ResolveCivilianConsequenceFaction(
		notnull SCR_InstigatorContextData context)
	{
		string factionKey;
		int playerId = context.GetKillerPlayerID();
		if (playerId > 0)
		{
			Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(playerId);
			if (playerFaction)
				factionKey = playerFaction.GetFactionKey();
		}
		if (factionKey.IsEmpty())
			factionKey = ResolveEntityFactionKey(context.GetKillerEntity());
		if (factionKey.IsEmpty())
		{
			Instigator instigator = context.GetInstigator();
			if (instigator)
				factionKey = ResolveEntityFactionKey(
					instigator.GetInstigatorEntity());
		}
		factionKey = factionKey.Trim();
		if (!m_Preset)
			return "";
		if (factionKey == m_Preset.m_sResistanceFactionKey
			|| factionKey == m_Preset.m_sOccupierFactionKey
			|| factionKey == m_Preset.m_sInvaderFactionKey)
			return factionKey;
		return "";
	}

	protected string ResolveEntityFactionKey(IEntity entity)
	{
		if (!entity)
			return "";
		FactionAffiliationComponent affiliation
			= FactionAffiliationComponent.Cast(
				entity.FindComponent(FactionAffiliationComponent));
		if (!affiliation)
			return "";
		return affiliation.GetAffiliatedFactionKey();
	}

	protected bool HasExactPendingCivilianCasualtyArrays()
	{
		int count = m_aPendingCivilianCasualtyZoneIds.Count();
		return count == m_aPendingCivilianCasualtyEventIds.Count()
			&& count == m_aPendingCivilianCasualtyFactionKeys.Count()
			&& count == m_aPendingCivilianCasualtySourceIds.Count()
			&& count == m_aPendingCivilianCasualtyPositions.Count()
			&& count == m_aPendingCivilianCasualtyAttempts.Count()
			&& count == m_aPendingCivilianCasualtyRetrySeconds.Count();
	}

	protected bool HasExactPendingCivilianTheftArrays()
	{
		int count = m_aPendingCivilianTheftZoneIds.Count();
		return count == m_aPendingCivilianTheftEventIds.Count()
			&& count == m_aPendingCivilianTheftFactionKeys.Count()
			&& count == m_aPendingCivilianTheftSourceIds.Count()
			&& count == m_aPendingCivilianTheftAttempts.Count()
			&& count == m_aPendingCivilianTheftRetrySeconds.Count();
	}

	protected bool HasPendingCivilianConsequenceWork()
	{
		return m_bPendingCivilianConsequenceAuthorityFault
			|| !HasExactPendingCivilianCasualtyArrays()
			|| !HasExactPendingCivilianTheftArrays()
			|| !m_aPendingCivilianCasualtyZoneIds.IsEmpty()
			|| !m_aPendingCivilianTheftZoneIds.IsEmpty();
	}

	protected bool HasPendingCivilianCasualtyRow(int index)
	{
		return index >= 0
			&& index < m_aPendingCivilianCasualtyZoneIds.Count()
			&& index < m_aPendingCivilianCasualtyEventIds.Count()
			&& index < m_aPendingCivilianCasualtyFactionKeys.Count()
			&& index < m_aPendingCivilianCasualtySourceIds.Count()
			&& index < m_aPendingCivilianCasualtyPositions.Count()
			&& index < m_aPendingCivilianCasualtyAttempts.Count()
			&& index < m_aPendingCivilianCasualtyRetrySeconds.Count();
	}

	protected void RemovePendingCivilianCasualtyAt(int index)
	{
		if (!HasPendingCivilianCasualtyRow(index))
			return;
		m_aPendingCivilianCasualtyZoneIds.Remove(index);
		m_aPendingCivilianCasualtyEventIds.Remove(index);
		m_aPendingCivilianCasualtyFactionKeys.Remove(index);
		m_aPendingCivilianCasualtySourceIds.Remove(index);
		m_aPendingCivilianCasualtyPositions.Remove(index);
		m_aPendingCivilianCasualtyAttempts.Remove(index);
		m_aPendingCivilianCasualtyRetrySeconds.Remove(index);
	}

	protected void ClearPendingCivilianCasualties()
	{
		m_aPendingCivilianCasualtyZoneIds.Clear();
		m_aPendingCivilianCasualtyEventIds.Clear();
		m_aPendingCivilianCasualtyFactionKeys.Clear();
		m_aPendingCivilianCasualtySourceIds.Clear();
		m_aPendingCivilianCasualtyPositions.Clear();
		m_aPendingCivilianCasualtyAttempts.Clear();
		m_aPendingCivilianCasualtyRetrySeconds.Clear();
	}

	protected bool QueuePendingCivilianTheft(
		string zoneId,
		string eventId,
		string factionKey,
		string sourceId)
	{
		if (!HasExactPendingCivilianTheftArrays())
		{
			m_bPendingCivilianConsequenceAuthorityFault = true;
			return false;
		}
		int existingIndex = m_aPendingCivilianTheftEventIds.Find(eventId);
		if (existingIndex >= 0)
		{
			return m_aPendingCivilianTheftZoneIds[existingIndex] == zoneId
				&& m_aPendingCivilianTheftFactionKeys[existingIndex]
					== factionKey
				&& m_aPendingCivilianTheftSourceIds[existingIndex]
					== sourceId;
		}
		if (zoneId.IsEmpty() || eventId.IsEmpty() || factionKey.IsEmpty()
			|| sourceId.IsEmpty()
			|| m_aPendingCivilianTheftZoneIds.Count()
				>= MAX_PENDING_CIVILIAN_THEFTS)
			return false;
		m_aPendingCivilianTheftZoneIds.Insert(zoneId);
		m_aPendingCivilianTheftEventIds.Insert(eventId);
		m_aPendingCivilianTheftFactionKeys.Insert(factionKey);
		m_aPendingCivilianTheftSourceIds.Insert(sourceId);
		m_aPendingCivilianTheftAttempts.Insert(0);
		m_aPendingCivilianTheftRetrySeconds.Insert(0);
		return true;
	}

	protected bool HasPendingCivilianTheftRow(int index)
	{
		return index >= 0
			&& index < m_aPendingCivilianTheftZoneIds.Count()
			&& index < m_aPendingCivilianTheftEventIds.Count()
			&& index < m_aPendingCivilianTheftFactionKeys.Count()
			&& index < m_aPendingCivilianTheftSourceIds.Count()
			&& index < m_aPendingCivilianTheftAttempts.Count()
			&& index < m_aPendingCivilianTheftRetrySeconds.Count();
	}

	protected void RemovePendingCivilianTheftAt(int index)
	{
		if (!HasPendingCivilianTheftRow(index))
			return;
		m_aPendingCivilianTheftZoneIds.Remove(index);
		m_aPendingCivilianTheftEventIds.Remove(index);
		m_aPendingCivilianTheftFactionKeys.Remove(index);
		m_aPendingCivilianTheftSourceIds.Remove(index);
		m_aPendingCivilianTheftAttempts.Remove(index);
		m_aPendingCivilianTheftRetrySeconds.Remove(index);
	}

	protected void ClearPendingCivilianThefts()
	{
		m_aPendingCivilianTheftZoneIds.Clear();
		m_aPendingCivilianTheftEventIds.Clear();
		m_aPendingCivilianTheftFactionKeys.Clear();
		m_aPendingCivilianTheftSourceIds.Clear();
		m_aPendingCivilianTheftAttempts.Clear();
		m_aPendingCivilianTheftRetrySeconds.Clear();
	}

	protected void SetCivilianPanicThreat(string zoneId, vector position)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return;
		zoneId = zoneId.Trim();
		if (zoneId.IsEmpty())
			return;
		int index = m_aCivilianPanicThreatZoneIds.Find(zoneId);
		if (index >= 0)
		{
			if (index < m_aCivilianPanicThreatPositions.Count())
				m_aCivilianPanicThreatPositions[index] = position;
			return;
		}
		m_aCivilianPanicThreatZoneIds.Insert(zoneId);
		m_aCivilianPanicThreatPositions.Insert(position);
	}

	protected vector ResolveCivilianPanicThreat(
		HST_ZoneState zone)
	{
		if (!zone)
			return "0 0 0";
		int index = m_aCivilianPanicThreatZoneIds.Find(zone.m_sZoneId);
		if (index >= 0 && index < m_aCivilianPanicThreatPositions.Count())
			return m_aCivilianPanicThreatPositions[index];
		return zone.m_vPosition;
	}

	// Reset preparation may perform the same reconciliation required by any
	// persistence boundary, but it does not clear civilian runtime caches or
	// delete roots. It freezes every occupied durable vehicle and proves the
	// complete field-vehicle deletion set before another reset authority mutates
	// the world.
	bool PrepareNewCampaignReset(
		HST_CampaignState previousState,
		out string failureReason)
	{
		if (m_CampaignDebugCivilianMutationLease)
		{
			failureReason = "campaign-debug civilian mutation lease is active";
			return false;
		}
		failureReason = "ambient vehicle authority could not be reconciled safely";
		m_bNewCampaignResetPrepared = false;
		m_NewCampaignResetPreparedState = null;
		m_NewCampaignResetFieldVehiclePlan = null;
		m_aResetPreservedPlayerVehicles.Clear();
		m_aResetPreservedPlayerVehicleCargo.Clear();
		if (!previousState)
			return false;
		array<IEntity> resetOccupancyPreflight = {};
		CollectPlayerOccupiedVehicleRoots(resetOccupancyPreflight);
		foreach (IEntity occupiedRoot : resetOccupancyPreflight)
		{
			int occupiedIndex = m_aRuntimeEntities.Find(occupiedRoot);
			if (occupiedIndex >= 0
				&& occupiedIndex < m_aRuntimeEntityKinds.Count()
				&& IsRuntimeVehicle(m_aRuntimeEntityKinds[occupiedIndex])
				&& ResolveExactVehicleClaimantFaction(occupiedRoot).IsEmpty())
				return false;
		}
		if (!PrepareAmbientVehiclePersistence(previousState))
			return false;
		array<string> claimedRuntimeIds = {};
		// A new campaign must not inherit every old field vehicle. Preserve only
		// live roots that a player occupies at the reset boundary, including an
		// ambient root first promoted by this exact observation.
		array<IEntity> occupiedVehicleRoots = {};
		CollectPlayerOccupiedVehicleRoots(occupiedVehicleRoots);
		foreach (IEntity entity : occupiedVehicleRoots)
		{
			if (!IsLivingAmbientEntity(entity))
				continue;
			HST_RuntimeVehicleState vehicle = ResolveRuntimeVehicleRecord(
				previousState,
				entity);
			if (!vehicle && m_PersistentFieldVehicles)
				vehicle = m_PersistentFieldVehicles.ResolveForEntity(
					previousState,
					entity);
			if (vehicle
				&& (vehicle.m_sRuntimeKind == "field_vehicle"
					|| vehicle.m_sRuntimeKind == "loot_vehicle"
					|| vehicle.m_sRuntimeKind == "garage_redeploy")
				&& !vehicle.m_bDeleted
				&& !vehicle.m_sVehicleRuntimeId.IsEmpty()
				&& claimedRuntimeIds.Find(vehicle.m_sVehicleRuntimeId) < 0)
			{
				claimedRuntimeIds.Insert(vehicle.m_sVehicleRuntimeId);
			}
		}
		if (previousState)
		{
			foreach (string claimedRuntimeId : claimedRuntimeIds)
			{
				HST_RuntimeVehicleState claimedVehicle
					= previousState.FindRuntimeVehicle(claimedRuntimeId);
				if (claimedVehicle)
					m_aResetPreservedPlayerVehicles.Insert(claimedVehicle);
				foreach (HST_VehicleCargoItemState cargoItem : previousState.m_aVehicleCargoItems)
				{
					if (cargoItem
						&& cargoItem.m_sVehicleRuntimeId == claimedRuntimeId)
						m_aResetPreservedPlayerVehicleCargo.Insert(cargoItem);
				}
			}
		}
		if (!m_PersistentFieldVehicles)
		{
			m_aResetPreservedPlayerVehicles.Clear();
			m_aResetPreservedPlayerVehicleCargo.Clear();
			return false;
		}
		string cleanupEvidence;
		m_NewCampaignResetFieldVehiclePlan
			= m_PersistentFieldVehicles
				.BuildNewCampaignResetCleanupPlanAfterCapture(
					previousState,
					claimedRuntimeIds,
					cleanupEvidence);
		if (!m_NewCampaignResetFieldVehiclePlan)
		{
			m_aResetPreservedPlayerVehicles.Clear();
			m_aResetPreservedPlayerVehicleCargo.Clear();
			if (!cleanupEvidence.IsEmpty())
				failureReason += " | " + cleanupEvidence;
			return false;
		}
		m_NewCampaignResetPreparedState = previousState;
		m_bNewCampaignResetPrepared = true;
		failureReason = "";
		return true;
	}

	void CancelNewCampaignResetPreparation()
	{
		if (m_CampaignDebugCivilianMutationLease)
			return;
		m_bNewCampaignResetPrepared = false;
		m_NewCampaignResetPreparedState = null;
		m_NewCampaignResetFieldVehiclePlan = null;
		m_aResetPreservedPlayerVehicles.Clear();
		m_aResetPreservedPlayerVehicleCargo.Clear();
	}

	// All fallible persistence, occupancy, and binding checks were completed by
	// PrepareNewCampaignReset. After the prospective reset checkpoint becomes
	// durable, Commit consumes only that frozen plan and performs the no-reject
	// destructive cleanup of old civilian runtime authority.
	void CommitNewCampaignReset()
	{
		if (m_CampaignDebugCivilianMutationLease)
			return;
		if (!m_bNewCampaignResetPrepared
			|| !m_NewCampaignResetPreparedState
			|| !m_NewCampaignResetFieldVehiclePlan)
			return;
		HST_CampaignState previousState = m_NewCampaignResetPreparedState;
		if (m_CivilianConsequences)
			m_CivilianConsequences.ResetRuntimeSession();
		ClearPendingCivilianCasualties();
		ClearPendingCivilianThefts();
		m_bPendingCivilianConsequenceAuthorityFault = false;
		m_aCivilianPanicThreatZoneIds.Clear();
		m_aCivilianPanicThreatPositions.Clear();
		foreach (HST_RuntimeVehicleState preservedVehicle : m_aResetPreservedPlayerVehicles)
		{
			if (preservedVehicle)
				preservedVehicle.m_sRuntimeKind = "field_vehicle";
		}
		m_PersistentFieldVehicles.CommitNewCampaignResetCleanupPlan(
			m_NewCampaignResetFieldVehiclePlan);
		CleanupAllRuntimeEntities(previousState);
		m_LastAmbientBudgetPlan = null;
		m_iNextAmbientRuntimeUpdateSecond = 0;
		m_iAmbientRotationEpoch = 0;
		m_iAmbientReconciliationCursor = 0;
		m_iAmbientRuntimeSequence = 0;
		m_iAmbientSpawnBudgetRemaining = 0;
		m_iRuntimeSpawnFailureCount = 0;
		m_sLastRuntimeSpawnFailurePrefab = "";
		m_bLastAmbientClaimObservationExact = true;
		m_bWarnedMissingCivilianCharacterPool = false;
		m_bWarnedMissingCivilianVehicleCatalog = false;
		m_bNewCampaignResetPrepared = false;
		m_NewCampaignResetPreparedState = null;
		m_NewCampaignResetFieldVehiclePlan = null;
	}

	// Compatibility wrapper for the former one-call reset path.
	bool ResetRuntimeSession(HST_CampaignState previousState)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		string failureReason;
		if (!PrepareNewCampaignReset(previousState, failureReason))
			return false;
		CommitNewCampaignReset();
		return true;
	}

	// Copies the frozen reset-boundary player vehicles into a prospective new
	// campaign without consuming preparation state. The coordinator can therefore
	// capture this state durably while every old-world cleanup remains reversible.
	bool CopyResetPreservedPlayerVehiclesToState(HST_CampaignState newState)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!newState)
			return false;
		foreach (HST_RuntimeVehicleState vehicle : m_aResetPreservedPlayerVehicles)
		{
			if (vehicle
				&& !vehicle.m_sVehicleRuntimeId.IsEmpty()
				&& !newState.FindRuntimeVehicle(vehicle.m_sVehicleRuntimeId))
				newState.m_aRuntimeVehicles.Insert(
					CopyResetPreservedPlayerVehicle(vehicle));
		}
		foreach (HST_VehicleCargoItemState cargoItem : m_aResetPreservedPlayerVehicleCargo)
		{
			if (cargoItem
				&& !newState.FindVehicleCargoItem(
					cargoItem.m_sVehicleRuntimeId,
					cargoItem.m_sItemPrefab))
				newState.m_aVehicleCargoItems.Insert(
					CopyResetPreservedPlayerVehicleCargo(cargoItem));
		}
		return true;
	}

	// Final consume occurs only after the reset checkpoint commits and the
	// destructive civilian cleanup has completed.
	void ClearResetPreservedPlayerVehicles()
	{
		if (m_CampaignDebugCivilianMutationLease)
			return;
		m_aResetPreservedPlayerVehicles.Clear();
		m_aResetPreservedPlayerVehicleCargo.Clear();
	}

	// Compatibility wrapper for the former copy-and-consume operation.
	void ApplyResetPreservedPlayerVehicles(HST_CampaignState newState)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return;
		if (!CopyResetPreservedPlayerVehiclesToState(newState))
			return;
		ClearResetPreservedPlayerVehicles();
	}

	protected HST_RuntimeVehicleState CopyResetPreservedPlayerVehicle(
		HST_RuntimeVehicleState source)
	{
		HST_RuntimeVehicleState target = new HST_RuntimeVehicleState();
		if (!source)
			return target;
		target.m_sVehicleRuntimeId = source.m_sVehicleRuntimeId;
		target.m_sPrefab = source.m_sPrefab;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_sZoneId = source.m_sZoneId;
		target.m_sRuntimeKind = "field_vehicle";
		target.m_sSourceVehicleKind = source.m_sSourceVehicleKind;
		target.m_vPosition = source.m_vPosition;
		target.m_vAngles = source.m_vAngles;
		target.m_iSpawnedAtSecond = source.m_iSpawnedAtSecond;
		target.m_bDetached = source.m_bDetached;
		target.m_bDeleted = source.m_bDeleted;
		target.m_bAmmoSource = source.m_bAmmoSource;
		target.m_bRepairSource = source.m_bRepairSource;
		target.m_bFuelSource = source.m_bFuelSource;
		target.m_bReported = source.m_bReported;
		target.m_bCanProvideUndercover = source.m_bCanProvideUndercover;
		target.m_iVehicleHeat = source.m_iVehicleHeat;
		target.m_iLastReportedSecond = source.m_iLastReportedSecond;
		target.m_iReportedUntilSecond = source.m_iReportedUntilSecond;
		target.m_iLastVehicleHeatChangedSecond
			= source.m_iLastVehicleHeatChangedSecond;
		target.m_iPassengerCompromiseCount
			= source.m_iPassengerCompromiseCount;
		target.m_sLastReportedReason = source.m_sLastReportedReason;
		target.m_sLastReporterZoneId = source.m_sLastReporterZoneId;
		return target;
	}

	protected HST_VehicleCargoItemState CopyResetPreservedPlayerVehicleCargo(
		HST_VehicleCargoItemState source)
	{
		HST_VehicleCargoItemState target = new HST_VehicleCargoItemState();
		if (!source)
			return target;
		target.m_sVehicleRuntimeId = source.m_sVehicleRuntimeId;
		target.m_sVehiclePrefab = source.m_sVehiclePrefab;
		target.m_sVehicleDisplayName = source.m_sVehicleDisplayName;
		target.m_sItemPrefab = source.m_sItemPrefab;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sCategory = source.m_sCategory;
		target.m_iCount = source.m_iCount;
		target.m_iLastStoredAtSecond = source.m_iLastStoredAtSecond;
		target.m_vLastVehiclePosition = source.m_vLastVehiclePosition;
		return target;
	}

	bool EnsureCivilianZones(HST_CampaignState state)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!state)
			return false;

		bool changed;
		int inserted;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			if (NormalizeKnownMinorLocality(zone))
				changed = true;
			if (!IsCivilianLocality(zone))
				continue;

			HST_CivilianZoneState existing = state.FindCivilianZone(zone.m_sZoneId);
			if (existing)
			{
				if (zone.m_sZoneId == SIMONS_WOOD_ZONE_ID && NormalizeMinorLocalityCivilianRecord(existing))
					changed = true;
				continue;
			}

			HST_CivilianZoneState civilianZone = new HST_CivilianZoneState();
			civilianZone.m_sZoneId = zone.m_sZoneId;
			civilianZone.m_iReputation = 50;
			if (IsTrueTownLocation(zone))
			{
				civilianZone.m_iCivilianPresence = Math.Max(4, zone.m_iIncomeValue / 8);
				civilianZone.m_iPolicePresence = Math.Max(1, zone.m_iGarrisonSlots / 6);
				civilianZone.m_iRoadblockPresence = 1;
			}
			else
			{
				civilianZone.m_iCivilianPresence = MINOR_LOCALITY_CIVILIAN_COUNT;
				civilianZone.m_iPolicePresence = 0;
				civilianZone.m_iRoadblockPresence = 0;
			}
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
			inserted++;
			changed = true;
		}

		if (inserted > 0)
			Print(string.Format("Partisan | initialized %1 missing civilian locality record(s); total %2", inserted, state.m_aCivilianZones.Count()));
		return changed;
	}

	void SetOwnershipTransitionService(HST_OwnershipTransitionService ownershipTransitions)
	{
		m_OwnershipTransitions = ownershipTransitions;
	}

	bool IsTrueTownLocation(HST_ZoneState zone)
	{
		// The curated strategic type is the runtime authority. Source-layout
		// metadata remains provenance, but many valid towns were originally
		// imported through base overlays rather than TC_* world objects.
		return zone && zone.m_eType == HST_EZoneType.HST_ZONE_TOWN;
	}

	bool IsMinorCivilianLocality(HST_ZoneState zone)
	{
		if (!zone)
			return false;
		if (zone.m_sZoneId == SIMONS_WOOD_ZONE_ID)
			return true;

		return false;
	}

	bool IsCivilianLocality(HST_ZoneState zone)
	{
		return IsTrueTownLocation(zone) || IsMinorCivilianLocality(zone);
	}

	int ResolveCivilianPedestrianTarget(
		HST_CivilianZoneState civilianZone,
		HST_ZoneState zone,
		HST_BalanceConfig balance,
		HST_CampaignState state = null,
		int civilianPresenceOverride = -1)
	{
		if (!civilianZone || !zone || !balance || !IsCivilianLocality(zone))
			return 0;

		int civilianPresence = civilianZone.m_iCivilianPresence;
		if (civilianPresenceOverride >= 0)
			civilianPresence = civilianPresenceOverride;
		int target = Math.Min(
			civilianPresence,
			balance.m_iCivilianMaxActivePerTown);
		if (IsMinorCivilianLocality(zone))
			target = Math.Min(target, MINOR_LOCALITY_CIVILIAN_COUNT);
		int remainingPopulation = civilianZone.m_iPopulationRemaining;
		int destroyedPopulation = civilianZone.m_iPopulationKilled;
		if (IsTrueTownLocation(zone))
		{
			if (!state || !m_TownInfluence)
				return 0;
			HST_TownInfluenceRecord record = m_TownInfluence.FindValidRecord(
				state,
				zone.m_sZoneId);
			if (!record)
				return 0;
			remainingPopulation = record.m_iRemainingPopulation;
			destroyedPopulation = record.m_iDestroyedPopulation;
		}
		if (remainingPopulation <= 0 && destroyedPopulation > 0)
			return 0;
		if (remainingPopulation > 0)
			target = Math.Min(target, remainingPopulation);

		return Math.Max(0, target);
	}

	int ResolveCivilianTrafficTarget(
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		HST_CivilianZoneState civilianZone = null,
		HST_CampaignState state = null)
	{
		if (!balance || !IsTrueTownLocation(zone))
			return 0;

		int remainingPopulation;
		if (state && m_TownInfluence)
		{
			HST_TownInfluenceRecord record = m_TownInfluence.FindValidRecord(
				state,
				zone.m_sZoneId);
			if (!record)
				return 0;
			remainingPopulation = record.m_iRemainingPopulation;
		}
		else if (civilianZone)
		{
			remainingPopulation = civilianZone.m_iPopulationRemaining;
		}
		else
		{
			return Math.Max(0, balance.m_iCivilianDrivingVehicleCountPerTown);
		}

		if (remainingPopulation <= 0)
			return 0;

		// A traffic vehicle consumes one living civilian driver. Reserve most of
		// a very small surviving population for pedestrians instead of projecting
		// five drivers after the town has nearly emptied.
		int populationTrafficCap = remainingPopulation / 3;
		return Math.Min(
			Math.Max(0, balance.m_iCivilianDrivingVehicleCountPerTown),
			Math.Max(0, populationTrafficCap));
	}

	bool IsCivilianProjectionEligible(HST_ZoneState zone, HST_BalanceConfig balance)
	{
		if (!zone || !balance || !IsCivilianLocality(zone))
			return false;

		float radius = zone.m_iActivationRadiusMeters;
		if (radius <= 0)
			radius = balance.m_iActivationRadiusMeters;
		if (zone.m_bActive)
			radius = Math.Max(radius, balance.m_iPlayerRenderBubbleRadiusMeters);
		return HST_WorldPositionService.IsPositionNearLivingPlayer(zone.m_vPosition, Math.Max(100.0, radius));
	}

	HST_CivilianProjectionProofSummary BuildProjectionProofSummary(
		string zoneId,
		HST_CivilianZoneState civilianZone,
		HST_ZoneState zone,
		HST_BalanceConfig balance,
		int observedPedestrians,
		int observedTrafficVehicles,
		HST_CampaignState state = null,
		int expectedPedestriansOverride = -1,
		int expectedTrafficOverride = -1)
	{
		HST_CivilianProjectionProofSummary summary = new HST_CivilianProjectionProofSummary();
		summary.m_bProjectionEligible = IsCivilianProjectionEligible(zone, balance);
		HST_AmbientPopulationTownAllocation allocation;
		if (m_LastAmbientBudgetPlan)
			allocation = m_LastAmbientBudgetPlan.Find(zoneId);
		if (allocation)
		{
			summary.m_iExpectedPedestrians = allocation.m_iAllocatedPedestrians;
			summary.m_iExpectedTrafficVehicles = allocation.m_iAllocatedTraffic;
		}
		if (expectedPedestriansOverride >= 0
			&& expectedTrafficOverride >= 0)
		{
			summary.m_iExpectedPedestrians = expectedPedestriansOverride;
			summary.m_iExpectedTrafficVehicles = expectedTrafficOverride;
		}
		summary.m_bTrafficConfigured = summary.m_iExpectedTrafficVehicles > 0;
		summary.m_iUniquePedestrianPrefabs = CountUniqueRuntimeEntityPrefabsForZone(zoneId, "CIV_CHARACTER", CIVILIAN_FACTION_KEY);
		summary.m_iTrafficDriverControllers = SuppressAmbientTrafficHornInput(zoneId);
		summary.m_iTrafficDriversWithHornInput = CountAmbientTrafficDriversWithHornInput(zoneId);
		summary.m_iActorAppearances = CountCivilianActorAppearancesForZone(zoneId);
		summary.m_iUniqueActorPrefabs = CountUniqueCivilianActorPrefabsForZone(zoneId);
		summary.m_bAppearanceDiversityExact
			= observedPedestrians == summary.m_iExpectedPedestrians;
		summary.m_bAppearanceDiversityExact = summary.m_bAppearanceDiversityExact
			&& observedTrafficVehicles == summary.m_iExpectedTrafficVehicles;
		summary.m_bAppearanceDiversityExact = summary.m_bAppearanceDiversityExact && summary.m_iUniquePedestrianPrefabs == observedPedestrians;
		summary.m_bAppearanceDiversityExact = summary.m_bAppearanceDiversityExact && summary.m_iActorAppearances == observedPedestrians + summary.m_iTrafficDriverControllers;
		summary.m_bAppearanceDiversityExact = summary.m_bAppearanceDiversityExact && summary.m_iUniqueActorPrefabs == summary.m_iActorAppearances;
		summary.m_bHornSuppressionExact = !summary.m_bTrafficConfigured
			&& observedTrafficVehicles == 0
			&& summary.m_iTrafficDriverControllers == 0
			&& summary.m_iTrafficDriversWithHornInput == 0;
		if (summary.m_bTrafficConfigured)
			summary.m_bHornSuppressionExact = summary.m_iTrafficDriverControllers == observedTrafficVehicles && summary.m_iTrafficDriversWithHornInput == 0;
		return summary;
	}

	protected bool NormalizeKnownMinorLocality(HST_ZoneState zone)
	{
		if (!zone || zone.m_sZoneId != SIMONS_WOOD_ZONE_ID)
			return false;

		bool changed;
		if (zone.m_eType != HST_EZoneType.HST_ZONE_RESOURCE)
		{
			zone.m_eType = HST_EZoneType.HST_ZONE_RESOURCE;
			changed = true;
		}
		if (zone.m_sResourceKind != "food")
		{
			zone.m_sResourceKind = "food";
			changed = true;
		}
		if (zone.m_iCaptureRadiusMeters != 180)
		{
			zone.m_iCaptureRadiusMeters = 180;
			changed = true;
		}
		if (zone.m_iGarrisonSlots != 6)
		{
			zone.m_iGarrisonSlots = 6;
			changed = true;
		}
		if (zone.m_sSpawnProfileId != "spawn_resource_guards")
		{
			zone.m_sSpawnProfileId = "spawn_resource_guards";
			changed = true;
		}
		if (zone.m_sMarkerTextColor != "gold")
		{
			zone.m_sMarkerTextColor = "gold";
			changed = true;
		}
		if (zone.m_sMarkerStyle != "resource")
		{
			zone.m_sMarkerStyle = "resource";
			changed = true;
		}

		return changed;
	}

	protected bool NormalizeMinorLocalityCivilianRecord(HST_CivilianZoneState civilianZone)
	{
		if (!civilianZone)
			return false;

		bool changed;
		int previousPresence = civilianZone.m_iCivilianPresence;
		if (civilianZone.m_iCivilianPresence != MINOR_LOCALITY_CIVILIAN_COUNT)
		{
			civilianZone.m_iCivilianPresence = MINOR_LOCALITY_CIVILIAN_COUNT;
			changed = true;
		}
		if (civilianZone.m_iPopulationKilled <= 0 && civilianZone.m_iPopulationRemaining == Math.Max(20, previousPresence * 8))
		{
			civilianZone.m_iPopulationRemaining = MINOR_LOCALITY_CIVILIAN_COUNT * 8;
			changed = true;
		}
		if (civilianZone.m_iPolicePresence != 0)
		{
			civilianZone.m_iPolicePresence = 0;
			changed = true;
		}
		if (civilianZone.m_iRoadblockPresence != 0)
		{
			civilianZone.m_iRoadblockPresence = 0;
			changed = true;
		}

		return changed;
	}

	bool Tick(HST_CampaignState state, int elapsedSeconds)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
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

		return changed;
	}

	bool UpdatePhysicalTownPopulation(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!state || !balance)
			return false;

		if (!balance.m_bCivilianPopulationEnabled)
		{
			bool durableChanged = PromotePlayerOccupiedRuntimeVehicles(state);
			CleanupAllRuntimeEntities(state);
			m_LastAmbientBudgetPlan = null;
			m_iNextAmbientRuntimeUpdateSecond = 0;
			PublishRuntimeDiagnostics(state);
			return durableChanged;
		}

		int healthInterval = Math.Max(2, balance.m_iCivilianRuntimeHealthIntervalSeconds);
		if (m_iNextAmbientRuntimeUpdateSecond > state.m_iElapsedSeconds)
			return false;
		m_iNextAmbientRuntimeUpdateSecond = state.m_iElapsedSeconds + healthInterval;
		m_iAmbientSpawnBudgetRemaining = AMBIENT_SPAWN_TRANSACTIONS_PER_UPDATE;

		HST_DefaultCatalog.EnsureCivilianPools(balance);
		// Only a player claim changes durable campaign state. Every other value in
		// this pass describes disposable physical projection and must not schedule
		// persistence or mission-intel publication on routine movement samples.
		bool durableChanged = PromotePlayerOccupiedRuntimeVehicles(state);
		PruneDeletedAmbientActorRecords(state);
		PruneDeletedRuntimeEntities(state);
		PruneAmbientTrafficOutsideRenderBubble(state, balance);
		TickAmbientActorRuntime(state, balance);
		m_iAmbientRotationEpoch = m_AmbientBudget.ResolveLeaseEpoch(
			state.m_iElapsedSeconds,
			ResolveAmbientAllocationLeaseSeconds(balance));
		m_LastAmbientBudgetPlan = BuildAmbientPopulationBudgetPlan(state, balance);
		ReconcileAmbientPopulationPlan(state, preset, balance);

		SuppressAmbientTrafficHornInput();
		PublishRuntimeDiagnostics(state);
		return durableChanged;
	}

	protected int ResolveAmbientAllocationLeaseSeconds(HST_BalanceConfig balance)
	{
		if (!balance)
			return AMBIENT_MINIMUM_ALLOCATION_LEASE_SECONDS;
		int recoveryWindow = balance.m_iCivilianRuntimeStartupGraceSeconds
			+ balance.m_iCivilianRuntimeStuckSeconds
			+ balance.m_iCivilianRuntimeRetryBackoffSeconds;
		return Math.Max(
			AMBIENT_MINIMUM_ALLOCATION_LEASE_SECONDS,
			recoveryWindow);
	}

	protected bool ReconcileAmbientTownAllocation(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		HST_AmbientPopulationTownAllocation allocation)
	{
		if (!state || !balance || !allocation)
			return false;
		HST_ZoneState zone = state.FindZone(allocation.m_sZoneId);
		if (!zone)
			return false;
		if (allocation.CountAllocatedActors() <= 0)
			return CleanupZoneRuntimeEntities(state, allocation.m_sZoneId);
		if (!HasRuntimeZone(zone.m_sZoneId))
		{
			return SpawnActiveZoneRuntime(
				state,
				preset,
				balance,
				zone,
				allocation.m_iAllocatedPedestrians,
				allocation.m_iAllocatedTraffic);
		}
		return MaintainActiveZonePopulation(
			state,
			preset,
			balance,
			zone,
			allocation.m_iAllocatedPedestrians,
			allocation.m_iAllocatedTraffic);
	}

	bool UpdatePhysicalTownPopulationForZone(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, string zoneId, bool active)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!state || !balance || zoneId.IsEmpty())
			return false;

		if (!balance.m_bCivilianPopulationEnabled)
			return PublishRuntimeDiagnostics(state) || CleanupZoneRuntimeEntities(state, zoneId);

		HST_DefaultCatalog.EnsureCivilianPools(balance);
		m_iAmbientSpawnBudgetRemaining = AMBIENT_SPAWN_TRANSACTIONS_PER_UPDATE;
		bool changed = PromotePlayerOccupiedRuntimeVehicles(state);
		PruneDeletedAmbientActorRecords(state);
		PruneDeletedRuntimeEntities(state);
		changed = PruneAmbientTrafficOutsideRenderBubble(state, balance) || changed;
		changed = TickAmbientActorRuntime(state, balance) || changed;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return PublishRuntimeDiagnostics(state) || changed;

		if (active)
		{
			m_iAmbientRotationEpoch = m_AmbientBudget.ResolveLeaseEpoch(
				state.m_iElapsedSeconds,
				ResolveAmbientAllocationLeaseSeconds(balance));
			// Campaign Debug must exercise the same complete global plan as
			// production; a one-town full-demand budget could silently exceed the
			// configured actor/traffic caps while other towns remain projected.
			HST_AmbientPopulationBudgetPlan directPlan
				= BuildAmbientPopulationBudgetPlan(state, balance);
			m_LastAmbientBudgetPlan = directPlan;
			changed = ReconcileAmbientPopulationPlan(
				state,
				preset,
				balance,
				zoneId) || changed;
		}
		else
		{
			m_LastAmbientBudgetPlan = null;
			if (CleanupZoneRuntimeEntities(state, zoneId))
				changed = true;
		}

		SuppressAmbientTrafficHornInput();
		return PublishRuntimeDiagnostics(state) || changed;
	}

	protected HST_AmbientPopulationTownAllocation ResolveCampaignDebugSelectedTownAllocation(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		string zoneId,
		int forcedCivilianPresence,
		out HST_AmbientPopulationBudgetPlan resolvedPlan,
		out string evidence)
	{
		resolvedPlan = null;
		evidence = "campaign-debug selected-town allocation rejected";
		if (!state || !balance || !m_AmbientBudget || zoneId.IsEmpty())
			return null;
		int rotationEpoch = m_AmbientBudget.ResolveLeaseEpoch(
			state.m_iElapsedSeconds,
			ResolveAmbientAllocationLeaseSeconds(balance));
		HST_AmbientPopulationBudgetPlan plan
			= BuildAmbientPopulationBudgetPlan(
				state,
				balance,
				zoneId,
				forcedCivilianPresence,
				rotationEpoch);
		if (!plan || !plan.IsWithinBudgets())
			return null;
		resolvedPlan = plan;
		HST_AmbientPopulationTownAllocation selectedAllocation;
		foreach (HST_AmbientPopulationTownAllocation allocation : plan.m_aTownAllocations)
		{
			if (!allocation || allocation.CountAllocatedActors() <= 0)
				continue;
			if (allocation.m_sZoneId != zoneId || selectedAllocation)
			{
				evidence = "global ambient plan has a positive allocation outside the selected town";
				return null;
			}
			selectedAllocation = allocation;
		}
		if (!selectedAllocation)
		{
			evidence = "global ambient plan has no positive selected-town allocation";
			return null;
		}
		evidence = string.Format(
			"selected-only global allocation | zone %1 | pedestrians/traffic %2/%3 | %4",
			zoneId,
			selectedAllocation.m_iAllocatedPedestrians,
			selectedAllocation.m_iAllocatedTraffic,
			plan.BuildReport());
		return selectedAllocation;
	}

	protected string BuildCampaignDebugAmbientBudgetPlanFingerprint(
		HST_AmbientPopulationBudgetPlan plan)
	{
		if (!plan)
			return "<none>";
		string fingerprint = string.Format(
			"plan:%1:%2:%3:%4:%5:%6:%7",
			plan.m_iTotalActorBudget,
			plan.m_iTrafficActorBudget,
			plan.m_iRotationEpoch,
			plan.m_iInputDemandCount,
			plan.m_iRejectedDemandCount,
			plan.m_iMergedDuplicateCount,
			plan.m_iAllocatedPedestrians);
		fingerprint = fingerprint + string.Format(
			":%1:%2",
			plan.m_iAllocatedTraffic,
			plan.m_aTownAllocations.Count());
		foreach (HST_AmbientPopulationTownAllocation allocation : plan.m_aTownAllocations)
		{
			if (!allocation)
			{
				fingerprint = fingerprint + "|<null>";
				continue;
			}
			fingerprint = fingerprint + string.Format(
				"|%1:%2:%3:%4:%5",
				allocation.m_sZoneId,
				allocation.m_iDesiredPedestrians,
				allocation.m_iDesiredTraffic,
				allocation.m_iAllocatedPedestrians,
				allocation.m_iAllocatedTraffic);
		}
		return fingerprint;
	}

	bool CaptureCampaignDebugAmbientServiceState(
		HST_CampaignState state,
		out HST_CampaignDebugAmbientServiceStateSnapshot snapshot,
		out string evidence)
	{
		snapshot = null;
		evidence = "ambient service-state capture rejected";
		if (!state)
			return false;
		snapshot = new HST_CampaignDebugAmbientServiceStateSnapshot();
		snapshot.m_BudgetPlan = m_LastAmbientBudgetPlan;
		snapshot.m_sBudgetPlanFingerprint
			= BuildCampaignDebugAmbientBudgetPlanFingerprint(
				m_LastAmbientBudgetPlan);
		snapshot.m_iNextAmbientRuntimeUpdateSecond
			= m_iNextAmbientRuntimeUpdateSecond;
		snapshot.m_iAmbientRotationEpoch = m_iAmbientRotationEpoch;
		snapshot.m_iAmbientReconciliationCursor
			= m_iAmbientReconciliationCursor;
		snapshot.m_iAmbientRuntimeSequence = m_iAmbientRuntimeSequence;
		snapshot.m_iAmbientSpawnBudgetRemaining
			= m_iAmbientSpawnBudgetRemaining;
		snapshot.m_iRuntimeSpawnFailureCount
			= m_iRuntimeSpawnFailureCount;
		snapshot.m_sLastRuntimeSpawnFailurePrefab
			= m_sLastRuntimeSpawnFailurePrefab;
		snapshot.m_bLastAmbientClaimObservationExact
			= m_bLastAmbientClaimObservationExact;
		snapshot.m_bWarnedMissingCivilianCharacterPool
			= m_bWarnedMissingCivilianCharacterPool;
		snapshot.m_bWarnedMissingCivilianVehicleCatalog
			= m_bWarnedMissingCivilianVehicleCatalog;
		snapshot.m_iStateRuntimeCivilianCharacterCount
			= state.m_iRuntimeCivilianCharacterCount;
		snapshot.m_iStateRuntimeCivilianVehicleCount
			= state.m_iRuntimeCivilianVehicleCount;
		snapshot.m_iStateRuntimeMilitaryVehicleCount
			= state.m_iRuntimeMilitaryVehicleCount;
		snapshot.m_iStateRuntimeSpawnFailureCount
			= state.m_iRuntimeSpawnFailureCount;
		snapshot.m_sStateLastRuntimeSpawnFailurePrefab
			= state.m_sLastRuntimeSpawnFailurePrefab;
		evidence = "exact ambient scheduler and diagnostic baseline captured | "
			+ snapshot.BuildEvidence();
		return true;
	}

	bool IsCampaignDebugAmbientServiceStateExact(
		HST_CampaignState state,
		HST_CampaignDebugAmbientServiceStateSnapshot expected,
		out string evidence)
	{
		evidence = "ambient service-state comparison rejected";
		if (!state || !expected)
			return false;
		string currentPlanFingerprint
			= BuildCampaignDebugAmbientBudgetPlanFingerprint(
				m_LastAmbientBudgetPlan);
		bool exact = true;
		if (m_LastAmbientBudgetPlan != expected.m_BudgetPlan)
			exact = false;
		if (currentPlanFingerprint != expected.m_sBudgetPlanFingerprint)
			exact = false;
		if (m_iNextAmbientRuntimeUpdateSecond
			!= expected.m_iNextAmbientRuntimeUpdateSecond)
			exact = false;
		if (m_iAmbientRotationEpoch != expected.m_iAmbientRotationEpoch)
			exact = false;
		if (m_iAmbientReconciliationCursor
			!= expected.m_iAmbientReconciliationCursor)
			exact = false;
		if (m_iAmbientRuntimeSequence != expected.m_iAmbientRuntimeSequence)
			exact = false;
		if (m_iAmbientSpawnBudgetRemaining
			!= expected.m_iAmbientSpawnBudgetRemaining)
			exact = false;
		if (m_iRuntimeSpawnFailureCount
			!= expected.m_iRuntimeSpawnFailureCount)
			exact = false;
		if (m_sLastRuntimeSpawnFailurePrefab
			!= expected.m_sLastRuntimeSpawnFailurePrefab)
			exact = false;
		if (m_bLastAmbientClaimObservationExact
			!= expected.m_bLastAmbientClaimObservationExact)
			exact = false;
		if (m_bWarnedMissingCivilianCharacterPool
			!= expected.m_bWarnedMissingCivilianCharacterPool)
			exact = false;
		if (m_bWarnedMissingCivilianVehicleCatalog
			!= expected.m_bWarnedMissingCivilianVehicleCatalog)
			exact = false;
		if (state.m_iRuntimeCivilianCharacterCount
			!= expected.m_iStateRuntimeCivilianCharacterCount)
			exact = false;
		if (state.m_iRuntimeCivilianVehicleCount
			!= expected.m_iStateRuntimeCivilianVehicleCount)
			exact = false;
		if (state.m_iRuntimeMilitaryVehicleCount
			!= expected.m_iStateRuntimeMilitaryVehicleCount)
			exact = false;
		if (state.m_iRuntimeSpawnFailureCount
			!= expected.m_iStateRuntimeSpawnFailureCount)
			exact = false;
		if (state.m_sLastRuntimeSpawnFailurePrefab
			!= expected.m_sStateLastRuntimeSpawnFailurePrefab)
			exact = false;
		evidence = string.Format(
			"ambient service baseline exact %1 | current plan %2 | expected %3",
			exact,
			currentPlanFingerprint,
			expected.m_sBudgetPlanFingerprint);
		return exact;
	}

	bool IsCampaignDebugAmbientPlanOwnershipExact(
		HST_AmbientPopulationBudgetPlan expectedPlan,
		string expectedFingerprint,
		out string evidence)
	{
		string currentFingerprint
			= BuildCampaignDebugAmbientBudgetPlanFingerprint(
				m_LastAmbientBudgetPlan);
		bool exact = expectedPlan
			&& m_LastAmbientBudgetPlan == expectedPlan
			&& currentFingerprint == expectedFingerprint;
		evidence = string.Format(
			"owned ambient plan exact %1 | current %2 | expected %3",
			exact,
			currentFingerprint,
			expectedFingerprint);
		return exact;
	}

	bool RestoreCampaignDebugAmbientServiceState(
		HST_CampaignState state,
		HST_CampaignDebugAmbientServiceStateSnapshot baseline,
		HST_AmbientPopulationBudgetPlan ownedPlan,
		string ownedPlanFingerprint,
		out string evidence)
	{
		evidence = "ambient service-state restore rejected";
		if (!state || !baseline)
			return false;
		string frozenBaselineFingerprint
			= BuildCampaignDebugAmbientBudgetPlanFingerprint(
				baseline.m_BudgetPlan);
		if (frozenBaselineFingerprint != baseline.m_sBudgetPlanFingerprint)
		{
			evidence = "borrowed baseline ambient plan content changed before restore";
			return false;
		}
		string currentPlanFingerprint
			= BuildCampaignDebugAmbientBudgetPlanFingerprint(
				m_LastAmbientBudgetPlan);
		bool currentPlanOwned = ownedPlan
			&& m_LastAmbientBudgetPlan == ownedPlan
			&& currentPlanFingerprint == ownedPlanFingerprint;
		bool currentPlanBaseline = !ownedPlan
			&& m_LastAmbientBudgetPlan == baseline.m_BudgetPlan
			&& currentPlanFingerprint == baseline.m_sBudgetPlanFingerprint;
		if (!currentPlanOwned && !currentPlanBaseline)
		{
			evidence = "ambient budget plan was replaced or mutated outside the frozen debug owner";
			return false;
		}

		m_LastAmbientBudgetPlan = baseline.m_BudgetPlan;
		m_iNextAmbientRuntimeUpdateSecond
			= baseline.m_iNextAmbientRuntimeUpdateSecond;
		m_iAmbientRotationEpoch = baseline.m_iAmbientRotationEpoch;
		m_iAmbientReconciliationCursor
			= baseline.m_iAmbientReconciliationCursor;
		m_iAmbientRuntimeSequence = baseline.m_iAmbientRuntimeSequence;
		m_iAmbientSpawnBudgetRemaining
			= baseline.m_iAmbientSpawnBudgetRemaining;
		m_iRuntimeSpawnFailureCount = baseline.m_iRuntimeSpawnFailureCount;
		m_sLastRuntimeSpawnFailurePrefab
			= baseline.m_sLastRuntimeSpawnFailurePrefab;
		m_bLastAmbientClaimObservationExact
			= baseline.m_bLastAmbientClaimObservationExact;
		m_bWarnedMissingCivilianCharacterPool
			= baseline.m_bWarnedMissingCivilianCharacterPool;
		m_bWarnedMissingCivilianVehicleCatalog
			= baseline.m_bWarnedMissingCivilianVehicleCatalog;
		state.m_iRuntimeCivilianCharacterCount
			= baseline.m_iStateRuntimeCivilianCharacterCount;
		state.m_iRuntimeCivilianVehicleCount
			= baseline.m_iStateRuntimeCivilianVehicleCount;
		state.m_iRuntimeMilitaryVehicleCount
			= baseline.m_iStateRuntimeMilitaryVehicleCount;
		state.m_iRuntimeSpawnFailureCount
			= baseline.m_iStateRuntimeSpawnFailureCount;
		state.m_sLastRuntimeSpawnFailurePrefab
			= baseline.m_sStateLastRuntimeSpawnFailurePrefab;
		if (!IsCampaignDebugAmbientServiceStateExact(
			state,
			baseline,
			evidence))
			return false;
		evidence = "exact ambient scheduler and diagnostic baseline restored | "
			+ baseline.BuildEvidence();
		return true;
	}

	protected bool IsCampaignDebugCivilianMutationMutableBaselineEmpty(
		HST_CampaignDebugAmbientMutableRegistrySnapshot baseline)
	{
		return baseline && !baseline.m_bConsequenceAuthorityFault
			&& baseline.m_aRetryZoneIds.IsEmpty()
			&& baseline.m_aStaticZoneIds.IsEmpty()
			&& baseline.m_aCasualtyEventIds.IsEmpty()
			&& baseline.m_aTheftEventIds.IsEmpty()
			&& baseline.m_aPanicZoneIds.IsEmpty()
			&& baseline.m_aResetPreservedVehicles.IsEmpty()
			&& baseline.m_aResetPreservedCargo.IsEmpty();
	}

	protected bool IsCampaignDebugCivilianMutationGlobalEmpty(
		HST_CampaignState state,
		HST_CampaignDebugAmbientMutableRegistrySnapshot expectedMutable,
		out string evidence)
	{
		evidence = "campaign-debug civilian global-empty proof rejected";
		if (!state || !expectedMutable)
			return false;
		array<ref HST_CampaignDebugAmbientRuntimeRootSnapshot> roots = {};
		array<string> zoneIds = {};
		HST_CampaignDebugAmbientMutableRegistrySnapshot currentMutable;
		if (!CaptureCampaignDebugAmbientRuntimeRoots(
			state,
			roots,
			zoneIds,
			evidence)
			|| !CaptureCampaignDebugAmbientMutableRegistries(
				currentMutable,
				evidence)
			|| !AreCampaignDebugAmbientMutableRegistriesExact(
				expectedMutable,
				currentMutable,
				evidence)
			|| !roots.IsEmpty() || !zoneIds.IsEmpty()
			|| !state.m_aRuntimeVehicles.IsEmpty())
			return false;
		evidence = "exact globally empty ambient roots, zones, vehicle rows, and mutable registries";
		return true;
	}

	protected bool IsCampaignDebugCivilianMutationLeaseAuthorityExact(
		HST_CampaignState state,
		string zoneId,
		string receiptId,
		out string evidence)
	{
		evidence = "campaign-debug civilian mutation lease authority rejected";
		HST_CampaignDebugCivilianMutationLease lease
			= m_CampaignDebugCivilianMutationLease;
		if (!lease || !lease.m_bGlobalEmptyBaselineProven
			|| receiptId.IsEmpty() || receiptId != lease.m_sReceiptId
			|| !state || state != lease.m_State
			|| zoneId.IsEmpty() || zoneId != lease.m_sZoneId)
			return false;
		HST_ZoneState zone = state.FindZone(zoneId);
		HST_CivilianZoneState town = state.FindCivilianZone(zoneId);
		if (!zone || zone != lease.m_Zone
			|| !town || town != lease.m_Town
			|| zone.m_sOwnerFactionKey
				!= lease.m_sFrozenZoneOwnerFactionKey
			|| zone.m_iOwnershipRevision
				!= lease.m_iFrozenZoneOwnershipRevision
			|| zone.m_bActive != lease.m_bFrozenZoneActive
			|| zone.m_iActiveInfantryCount
				!= lease.m_iFrozenZoneActiveInfantry
			|| zone.m_iActiveVehicleCount
				!= lease.m_iFrozenZoneActiveVehicles)
		{
			evidence = "selected town/zone pointer, owner, revision, or active-force authority escaped the mutation lease";
			return false;
		}
		evidence = "exact exclusive mutation lease | " + lease.BuildEvidence();
		return true;
	}

	bool BeginCampaignDebugCivilianMutationLease(
		HST_CampaignState state,
		string zoneId,
		HST_CampaignDebugAmbientServiceStateSnapshot expectedServiceBaseline,
		HST_CampaignDebugAmbientMutableRegistrySnapshot expectedMutableBaseline,
		out string receiptId,
		out string evidence)
	{
		receiptId = "";
		evidence = "campaign-debug civilian mutation lease admission rejected";
		if (m_CampaignDebugCivilianMutationLease || !state
			|| zoneId.IsEmpty() || !expectedServiceBaseline
			|| !IsCampaignDebugCivilianMutationMutableBaselineEmpty(
				expectedMutableBaseline))
			return false;
		HST_ZoneState zone = state.FindZone(zoneId);
		HST_CivilianZoneState town = state.FindCivilianZone(zoneId);
		if (!zone || !town || zone.m_sZoneId != town.m_sZoneId)
			return false;
		if (!IsCampaignDebugAmbientServiceStateExact(
			state,
			expectedServiceBaseline,
			evidence))
			return false;
		HST_CampaignDebugAmbientServiceStateSnapshot internalServiceBaseline;
		HST_CampaignDebugAmbientMutableRegistrySnapshot internalMutableBaseline;
		if (!CaptureCampaignDebugAmbientServiceState(
			state,
			internalServiceBaseline,
			evidence)
			|| !CaptureCampaignDebugAmbientMutableRegistries(
				internalMutableBaseline,
				evidence)
			|| !AreCampaignDebugAmbientMutableRegistriesExact(
				expectedMutableBaseline,
				internalMutableBaseline,
				evidence)
			|| !IsCampaignDebugCivilianMutationGlobalEmpty(
				state,
				internalMutableBaseline,
				evidence))
			return false;
		if (m_iCampaignDebugCivilianMutationLeaseSequence >= int.MAX - 1)
		{
			evidence = "campaign-debug civilian mutation lease sequence exhausted";
			return false;
		}
		m_iCampaignDebugCivilianMutationLeaseSequence++;
		HST_CampaignDebugCivilianMutationLease lease
			= new HST_CampaignDebugCivilianMutationLease();
		lease.m_sReceiptId = string.Format(
			"phase20_ambient_mutation_%1_%2_%3",
			zoneId,
			state.m_iElapsedSeconds,
			m_iCampaignDebugCivilianMutationLeaseSequence);
		lease.m_sZoneId = zoneId;
		lease.m_sFrozenZoneOwnerFactionKey = zone.m_sOwnerFactionKey;
		lease.m_iFrozenZoneOwnershipRevision = zone.m_iOwnershipRevision;
		lease.m_bFrozenZoneActive = zone.m_bActive;
		lease.m_iFrozenZoneActiveInfantry = zone.m_iActiveInfantryCount;
		lease.m_iFrozenZoneActiveVehicles = zone.m_iActiveVehicleCount;
		lease.m_State = state;
		lease.m_Zone = zone;
		lease.m_Town = town;
		lease.m_ServiceBaseline = internalServiceBaseline;
		lease.m_MutableBaseline = internalMutableBaseline;
		lease.m_bGlobalEmptyBaselineProven = true;
		m_CampaignDebugCivilianMutationLease = lease;
		receiptId = lease.m_sReceiptId;
		evidence = "exclusive globally empty mutation lease armed before production mutation | "
			+ lease.BuildEvidence();
		return true;
	}

	bool HasCampaignDebugCivilianMutationLease()
	{
		return m_CampaignDebugCivilianMutationLease != null;
	}

	protected bool BeginCampaignDebugCivilianMutationAttempt(
		HST_CampaignState state,
		string zoneId,
		string receiptId,
		string mutationKind,
		out string evidence)
	{
		if (!IsCampaignDebugCivilianMutationLeaseAuthorityExact(
			state,
			zoneId,
			receiptId,
			evidence)
			|| m_CampaignDebugCivilianMutationLease.m_bCleanupStarted
			|| mutationKind.IsEmpty())
			return false;
		if (HasCampaignDebugCivilianMutationForeignClaim(state, evidence))
			return false;
		HST_CampaignDebugCivilianMutationLease lease
			= m_CampaignDebugCivilianMutationLease;
		if (lease.m_iMutationAttemptCount >= int.MAX - 1)
		{
			evidence = "campaign-debug civilian mutation attempt sequence exhausted";
			return false;
		}
		lease.m_iMutationAttemptCount++;
		lease.m_sLastMutationKind = mutationKind;
		evidence = "pre-mutation receipt armed | " + lease.BuildEvidence();
		return true;
	}

	protected bool RecordCampaignDebugCivilianMutationBudgetPlan(
		HST_AmbientPopulationBudgetPlan plan,
		out string evidence)
	{
		evidence = "campaign-debug civilian mutation plan receipt rejected";
		HST_CampaignDebugCivilianMutationLease lease
			= m_CampaignDebugCivilianMutationLease;
		if (!lease || !plan || lease.m_bCleanupStarted)
			return false;
		string fingerprint
			= BuildCampaignDebugAmbientBudgetPlanFingerprint(plan);
		if (fingerprint.IsEmpty())
			return false;
		if (lease.m_OwnedBudgetPlan
			&& (lease.m_OwnedBudgetPlan != plan
				|| lease.m_sOwnedBudgetPlanFingerprint != fingerprint))
			return false;
		lease.m_OwnedBudgetPlan = plan;
		lease.m_sOwnedBudgetPlanFingerprint = fingerprint;
		evidence = "exact mutation-owned ambient budget plan recorded before publication";
		return true;
	}

	protected void TrackCampaignDebugCivilianMutationEntity(IEntity entity)
	{
		if (!entity || !m_CampaignDebugCivilianMutationLease
			|| m_CampaignDebugCivilianMutationLease.m_bCleanupStarted
			|| m_CampaignDebugCivilianMutationLease
				.m_aSpawnedEntities.Contains(entity))
			return;
		m_CampaignDebugCivilianMutationLease.m_aSpawnedEntities.Insert(entity);
	}

	protected void TrackCampaignDebugCivilianMutationRuntimeVehicleRow(
		HST_RuntimeVehicleState row)
	{
		if (!row || !m_CampaignDebugCivilianMutationLease
			|| m_CampaignDebugCivilianMutationLease.m_bCleanupStarted
			|| m_CampaignDebugCivilianMutationLease
				.m_aRuntimeVehicleRows.Contains(row))
			return;
		m_CampaignDebugCivilianMutationLease.m_aRuntimeVehicleRows.Insert(row);
	}

	protected void LatchCampaignDebugCivilianMutationPlayerClaim(
		IEntity claimedEntity)
	{
		HST_CampaignDebugCivilianMutationLease lease
			= m_CampaignDebugCivilianMutationLease;
		if (!lease || !claimedEntity
			|| !lease.m_aSpawnedEntities.Contains(claimedEntity))
			return;
		lease.m_bPlayerClaimCollisionObserved = true;
		if (!lease.m_aPlayerClaimCollisionEntities.Contains(claimedEntity))
			lease.m_aPlayerClaimCollisionEntities.Insert(claimedEntity);
	}

	// Promotion remains held while Phase 20 owns the service, but observation
	// must continue. The collision latch is intentionally one-way: a player who
	// exits before cleanup has still crossed foreign authority, so no debug path
	// may later delete that entity merely because current occupancy is empty.
	protected bool HasCampaignDebugCivilianMutationForeignClaim(
		HST_CampaignState state,
		out string evidence)
	{
		evidence = "";
		HST_CampaignDebugCivilianMutationLease lease
			= m_CampaignDebugCivilianMutationLease;
		if (!lease)
			return false;
		PlayerManager playerManager;
		if (GetGame())
			playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			array<int> playerIds = {};
			playerManager.GetPlayers(playerIds);
			foreach (int playerId : playerIds)
			{
				IEntity controlledEntity
					= playerManager.GetPlayerControlledEntity(playerId);
				IEntity mainEntity
					= SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
				LatchCampaignDebugCivilianMutationPlayerClaim(controlledEntity);
				LatchCampaignDebugCivilianMutationPlayerClaim(
					ResolveEntityVehicle(controlledEntity));
				if (mainEntity != controlledEntity)
				{
					LatchCampaignDebugCivilianMutationPlayerClaim(mainEntity);
					LatchCampaignDebugCivilianMutationPlayerClaim(
						ResolveEntityVehicle(mainEntity));
				}
			}
		}
		foreach (IEntity spawnedEntity : lease.m_aSpawnedEntities)
		{
			BaseCompartmentManagerComponent compartmentManager
				= ResolveCompartmentManager(spawnedEntity);
			if (!compartmentManager)
				continue;
			array<BaseCompartmentSlot> slots = {};
			compartmentManager.GetCompartments(slots);
			foreach (BaseCompartmentSlot slot : slots)
			{
				IEntity occupant;
				if (slot)
					occupant = slot.GetOccupant();
				if (!occupant || lease.m_aSpawnedEntities.Contains(occupant))
					continue;
				lease.m_bForeignCompartmentAuthorityObserved = true;
				if (!lease.m_aForeignCompartmentCollisionEntities.Contains(occupant))
				{
					lease.m_aForeignCompartmentCollisionEntities.Insert(occupant);
				}
			}
		}
		if (state == lease.m_State)
		{
			foreach (HST_VehicleCargoItemState cargoItem : state.m_aVehicleCargoItems)
			{
				if (!cargoItem || cargoItem.m_sVehicleRuntimeId.IsEmpty())
					continue;
				foreach (HST_RuntimeVehicleState trackedVehicle : lease.m_aRuntimeVehicleRows)
				{
					if (trackedVehicle
						&& trackedVehicle.m_sVehicleRuntimeId
							== cargoItem.m_sVehicleRuntimeId)
					{
						lease.m_bForeignVehicleCargoAuthorityObserved = true;
					}
				}
			}
		}
		if (lease.m_bPlayerClaimCollisionObserved)
		{
			evidence = "mutation-leased entity crossed player authority; collision is permanently retained";
			return true;
		}
		if (lease.m_bForeignCompartmentAuthorityObserved)
		{
			evidence = "mutation-leased vehicle gained a foreign compartment occupant; collision is permanently retained";
			return true;
		}
		if (lease.m_bForeignVehicleCargoAuthorityObserved)
		{
			evidence = "mutation-leased vehicle domain gained foreign durable cargo authority";
			return true;
		}
		return false;
	}

	protected bool IsCampaignDebugCivilianMutationKindKnown(string runtimeKind)
	{
		return runtimeKind.IsEmpty() || runtimeKind == "CIV_CHARACTER"
			|| runtimeKind == "CIV_VEHICLE"
			|| runtimeKind == CIVILIAN_TRAFFIC_RUNTIME_KIND
			|| runtimeKind == "MILITARY_VEHICLE";
	}

	protected bool AuditCampaignDebugCivilianMutationLeaseCleanupAuthority(
		HST_CampaignState state,
		string zoneId,
		string receiptId,
		out string evidence)
	{
		if (!IsCampaignDebugCivilianMutationLeaseAuthorityExact(
			state,
			zoneId,
			receiptId,
			evidence))
			return false;
		HST_CampaignDebugCivilianMutationLease lease
			= m_CampaignDebugCivilianMutationLease;
		if (HasCampaignDebugCivilianMutationForeignClaim(state, evidence))
			return false;
		foreach (IEntity spawnedEntity : lease.m_aSpawnedEntities)
		{
			if (!spawnedEntity)
				continue;
			if (IsPlayerControlledEntity(spawnedEntity)
				|| HasPlayerOccupant(spawnedEntity))
			{
				evidence = "mutation-leased entity gained a player claim";
				return false;
			}
		}
		foreach (string currentZoneId : m_aRuntimeZoneIds)
		{
			if (!currentZoneId.IsEmpty() && currentZoneId != zoneId)
			{
				evidence = "runtime-zone authority escaped the mutation lease";
				return false;
			}
		}
		foreach (string rootZoneId : m_aRuntimeEntityZoneIds)
		{
			if (!rootZoneId.IsEmpty() && rootZoneId != zoneId)
			{
				evidence = "ambient root parallel zone authority escaped the mutation lease";
				return false;
			}
		}
		foreach (string rootKind : m_aRuntimeEntityKinds)
		{
			if (!IsCampaignDebugCivilianMutationKindKnown(rootKind))
			{
				evidence = "ambient root parallel kind authority escaped the mutation lease";
				return false;
			}
		}
		foreach (string rootFactionKey : m_aRuntimeEntityFactionKeys)
		{
			if (!rootFactionKey.IsEmpty()
				&& rootFactionKey != CIVILIAN_FACTION_KEY
				&& rootFactionKey != lease.m_sFrozenZoneOwnerFactionKey)
			{
				evidence = "ambient root parallel faction authority escaped the mutation lease";
				return false;
			}
		}
		for (int rootIndex; rootIndex < m_aRuntimeEntities.Count(); rootIndex++)
		{
			IEntity root = m_aRuntimeEntities[rootIndex];
			if (root && !lease.m_aSpawnedEntities.Contains(root))
			{
				evidence = "ambient root pointer is absent from the pre-armed spawn journal";
				return false;
			}
			if (rootIndex < m_aRuntimeEntityZoneIds.Count()
				&& !m_aRuntimeEntityZoneIds[rootIndex].IsEmpty()
				&& m_aRuntimeEntityZoneIds[rootIndex] != zoneId)
			{
				evidence = "ambient root row escaped the selected zone";
				return false;
			}
			if (rootIndex < m_aRuntimeEntityKinds.Count()
				&& !IsCampaignDebugCivilianMutationKindKnown(
					m_aRuntimeEntityKinds[rootIndex]))
			{
				evidence = "ambient root row escaped the known mutation kinds";
				return false;
			}
			if (rootIndex < m_aRuntimeEntityFactionKeys.Count())
			{
				string rootFaction = m_aRuntimeEntityFactionKeys[rootIndex];
				if (!rootFaction.IsEmpty() && rootFaction != CIVILIAN_FACTION_KEY
					&& rootFaction != lease.m_sFrozenZoneOwnerFactionKey)
				{
					evidence = "ambient root faction escaped the selected-zone lease";
					return false;
				}
			}
		}
		for (int helperIndex; helperIndex < m_aRuntimeHelperEntities.Count(); helperIndex++)
		{
			IEntity helper = m_aRuntimeHelperEntities[helperIndex];
			if (helper && !lease.m_aSpawnedEntities.Contains(helper))
			{
				evidence = "ambient helper pointer is absent from the pre-armed spawn journal";
				return false;
			}
		}
		foreach (IEntity helperOwner : m_aRuntimeHelperOwners)
		{
			if (helperOwner && !lease.m_aSpawnedEntities.Contains(helperOwner))
			{
				evidence = "ambient helper owner escaped the pre-armed spawn journal";
				return false;
			}
		}
		foreach (HST_AmbientActorRuntimeRecord record : m_aAmbientActorRecords)
		{
			if (!record)
				continue;
			if ((!record.m_sZoneId.IsEmpty() && record.m_sZoneId != zoneId)
				|| (record.m_RootEntity
					&& !lease.m_aSpawnedEntities.Contains(record.m_RootEntity))
				|| (record.m_DriverEntity
					&& !lease.m_aSpawnedEntities.Contains(record.m_DriverEntity))
				|| (record.m_Group
					&& !lease.m_aSpawnedEntities.Contains(record.m_Group)))
			{
				evidence = "ambient actor record escaped the selected-zone spawn journal";
				return false;
			}
		}
		foreach (HST_RuntimeVehicleState runtimeVehicle : state.m_aRuntimeVehicles)
		{
			if (!runtimeVehicle)
				continue;
			if (!lease.m_aRuntimeVehicleRows.Contains(runtimeVehicle))
			{
				evidence = "runtime-vehicle row is absent from the pre-armed row journal";
				return false;
			}
			if (runtimeVehicle.m_bDetached
				|| (!runtimeVehicle.m_sZoneId.IsEmpty()
					&& runtimeVehicle.m_sZoneId != zoneId)
				|| (!runtimeVehicle.m_sFactionKey.IsEmpty()
					&& runtimeVehicle.m_sFactionKey != CIVILIAN_FACTION_KEY
					&& runtimeVehicle.m_sFactionKey
						!= lease.m_sFrozenZoneOwnerFactionKey))
			{
				evidence = "runtime-vehicle row gained detached, foreign-zone, or foreign-faction authority";
				return false;
			}
		}
		foreach (HST_RuntimeVehicleState trackedRow : lease.m_aRuntimeVehicleRows)
		{
			if (trackedRow && (trackedRow.m_bDetached
				|| (!trackedRow.m_sZoneId.IsEmpty()
					&& trackedRow.m_sZoneId != zoneId)))
			{
				evidence = "tracked runtime-vehicle row escaped lease authority";
				return false;
			}
		}
		foreach (string retryZoneId : m_aAmbientRetryZoneIds)
		{
			if (!retryZoneId.IsEmpty() && retryZoneId != zoneId)
				return false;
		}
		foreach (string retryKind : m_aAmbientRetryKinds)
		{
			if (!IsCampaignDebugCivilianMutationKindKnown(retryKind))
				return false;
		}
		foreach (string staticZoneId : m_aStaticVehicleInitializationZoneIds)
		{
			if (!staticZoneId.IsEmpty() && staticZoneId != zoneId)
				return false;
		}
		foreach (string staticOwnerKey : m_aStaticMilitaryInitializationOwnerKeys)
		{
			if (!staticOwnerKey.IsEmpty() && staticOwnerKey != "<none>"
				&& staticOwnerKey != lease.m_sFrozenZoneOwnerFactionKey)
				return false;
		}
		foreach (string casualtyZoneId : m_aPendingCivilianCasualtyZoneIds)
		{
			if (!casualtyZoneId.IsEmpty() && casualtyZoneId != zoneId)
				return false;
		}
		foreach (string theftZoneId : m_aPendingCivilianTheftZoneIds)
		{
			if (!theftZoneId.IsEmpty() && theftZoneId != zoneId)
				return false;
		}
		foreach (string panicZoneId : m_aCivilianPanicThreatZoneIds)
		{
			if (!panicZoneId.IsEmpty() && panicZoneId != zoneId)
				return false;
		}
		if (!m_aResetPreservedPlayerVehicles.IsEmpty()
			|| !m_aResetPreservedPlayerVehicleCargo.IsEmpty())
		{
			evidence = "player-claim/reset-preserved authority escaped the mutation lease";
			return false;
		}
		string currentPlanFingerprint
			= BuildCampaignDebugAmbientBudgetPlanFingerprint(
				m_LastAmbientBudgetPlan);
		bool baselinePlanExact
			= m_LastAmbientBudgetPlan == lease.m_ServiceBaseline.m_BudgetPlan
			&& currentPlanFingerprint
				== lease.m_ServiceBaseline.m_sBudgetPlanFingerprint;
		bool ownedPlanExact = lease.m_OwnedBudgetPlan
			&& m_LastAmbientBudgetPlan == lease.m_OwnedBudgetPlan
			&& currentPlanFingerprint
				== lease.m_sOwnedBudgetPlanFingerprint;
		if (!baselinePlanExact && !ownedPlanExact)
		{
			evidence = "ambient budget-plan pointer or content escaped the mutation lease";
			return false;
		}
		evidence = "exact lease/state/zone, pointer journals, and globally empty baseline prove emergency cleanup authority | "
			+ lease.BuildEvidence();
		return true;
	}

	protected void ClearCampaignDebugCivilianMutationRegistries(
		HST_CampaignState state)
	{
		array<string> trackedVehicleIds = {};
		foreach (HST_RuntimeVehicleState trackedRow : m_CampaignDebugCivilianMutationLease.m_aRuntimeVehicleRows)
		{
			if (trackedRow && !trackedRow.m_sVehicleRuntimeId.IsEmpty()
				&& !trackedVehicleIds.Contains(trackedRow.m_sVehicleRuntimeId))
				trackedVehicleIds.Insert(trackedRow.m_sVehicleRuntimeId);
		}
		for (int cargoIndex = state.m_aVehicleCargoItems.Count() - 1; cargoIndex >= 0; cargoIndex--)
		{
			HST_VehicleCargoItemState cargo = state.m_aVehicleCargoItems[cargoIndex];
			if (cargo && trackedVehicleIds.Contains(cargo.m_sVehicleRuntimeId))
				state.m_aVehicleCargoItems.Remove(cargoIndex);
		}
		state.m_aRuntimeVehicles.Clear();
		m_aRuntimeZoneIds.Clear();
		m_aRuntimeEntityZoneIds.Clear();
		m_aRuntimeEntityKinds.Clear();
		m_aRuntimeEntityFactionKeys.Clear();
		m_aRuntimeEntityVehicleIds.Clear();
		m_aRuntimeEntitySpawnPositions.Clear();
		m_aRuntimeEntities.Clear();
		m_aRuntimeHelperOwners.Clear();
		m_aRuntimeHelperEntities.Clear();
		m_aAmbientActorRecords.Clear();
		m_aAmbientRetryZoneIds.Clear();
		m_aAmbientRetryKinds.Clear();
		m_aAmbientRetrySeconds.Clear();
		m_aStaticVehicleInitializationZoneIds.Clear();
		m_aStaticCivilianVehicleSlotsCompleted.Clear();
		m_aStaticMilitaryVehicleSlotsCompleted.Clear();
		m_aStaticMilitaryInitializationOwnerKeys.Clear();
		ClearPendingCivilianCasualties();
		ClearPendingCivilianThefts();
		m_bPendingCivilianConsequenceAuthorityFault = false;
		m_aCivilianPanicThreatZoneIds.Clear();
		m_aCivilianPanicThreatPositions.Clear();
	}

	bool EmergencyCleanupCampaignDebugCivilianMutationLease(
		HST_CampaignState state,
		string zoneId,
		string receiptId,
		out string evidence)
	{
		evidence = "campaign-debug civilian emergency mutation cleanup rejected";
		if (!AuditCampaignDebugCivilianMutationLeaseCleanupAuthority(
			state,
			zoneId,
			receiptId,
			evidence))
			return false;
		HST_CampaignDebugCivilianMutationLease lease
			= m_CampaignDebugCivilianMutationLease;
		lease.m_bCleanupStarted = true;
		bool deletionPending;
		foreach (IEntity spawnedEntity : lease.m_aSpawnedEntities)
		{
			if (!spawnedEntity || spawnedEntity.IsDeleted())
				continue;
			SCR_EntityHelper.DeleteEntityAndChildren(spawnedEntity);
			if (!spawnedEntity.IsDeleted())
				deletionPending = true;
		}
		if (deletionPending)
		{
			evidence = "emergency cleanup deletion requested for every leased entity; awaiting exact acknowledgement | "
				+ lease.BuildEvidence();
			return false;
		}
		ClearCampaignDebugCivilianMutationRegistries(state);
		HST_AmbientPopulationBudgetPlan currentOwnedPlan;
		string currentOwnedPlanFingerprint;
		if (m_LastAmbientBudgetPlan != lease.m_ServiceBaseline.m_BudgetPlan)
		{
			currentOwnedPlan = lease.m_OwnedBudgetPlan;
			currentOwnedPlanFingerprint
				= lease.m_sOwnedBudgetPlanFingerprint;
		}
		if (!RestoreCampaignDebugAmbientServiceState(
			state,
			lease.m_ServiceBaseline,
			currentOwnedPlan,
			currentOwnedPlanFingerprint,
			evidence)
			|| !IsCampaignDebugCivilianMutationGlobalEmpty(
				state,
				lease.m_MutableBaseline,
				evidence)
			|| !IsCampaignDebugAmbientServiceStateExact(
				state,
				lease.m_ServiceBaseline,
				evidence))
			return false;
		evidence = "emergency cleanup restored the exact globally empty service baseline and released lease | "
			+ lease.BuildEvidence();
		m_CampaignDebugCivilianMutationLease = null;
		return true;
	}

	bool FinishCampaignDebugCivilianMutationLease(
		HST_CampaignState state,
		string zoneId,
		string receiptId,
		out string evidence)
	{
		if (!IsCampaignDebugCivilianMutationLeaseAuthorityExact(
			state,
			zoneId,
			receiptId,
			evidence))
			return false;
		HST_CampaignDebugCivilianMutationLease lease
			= m_CampaignDebugCivilianMutationLease;
		if (HasCampaignDebugCivilianMutationForeignClaim(state, evidence))
			return false;
		foreach (IEntity spawnedEntity : lease.m_aSpawnedEntities)
		{
			if (spawnedEntity && !spawnedEntity.IsDeleted())
			{
				evidence = "a mutation-leased native entity lacks deletion acknowledgement";
				return false;
			}
		}
		foreach (HST_RuntimeVehicleState trackedRow : lease.m_aRuntimeVehicleRows)
		{
			if (trackedRow && state.m_aRuntimeVehicles.Contains(trackedRow))
			{
				evidence = "a mutation-leased runtime-vehicle row remains live";
				return false;
			}
		}
		if (!IsCampaignDebugCivilianMutationGlobalEmpty(
			state,
			lease.m_MutableBaseline,
			evidence)
			|| !IsCampaignDebugAmbientServiceStateExact(
				state,
				lease.m_ServiceBaseline,
				evidence))
			return false;
		evidence = "normal cleanup restored the exact globally empty service baseline and released lease | "
			+ lease.BuildEvidence();
		m_CampaignDebugCivilianMutationLease = null;
		return true;
	}

	bool CanBeginCampaignDebugCivilianTownAdmission(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		string zoneId,
		int forcedCivilianPresence,
		HST_CampaignDebugAmbientServiceStateSnapshot serviceBaseline,
		out string evidence)
	{
		evidence = "campaign-debug civilian admission preflight rejected";
		if (m_CampaignDebugCivilianMutationLease || !state || !balance
			|| zoneId.IsEmpty() || !serviceBaseline)
			return false;
		if (!IsCampaignDebugAmbientServiceStateExact(
			state,
			serviceBaseline,
			evidence))
			return false;
		array<ref HST_CampaignDebugAmbientRuntimeRootSnapshot> roots = {};
		array<string> runtimeZoneIds = {};
		string snapshotEvidence;
		if (!CaptureCampaignDebugAmbientRuntimeRoots(
			state,
			roots,
			runtimeZoneIds,
			snapshotEvidence)
			|| !roots.IsEmpty() || !runtimeZoneIds.IsEmpty()
			|| !state.m_aRuntimeVehicles.IsEmpty())
		{
			evidence = "global ambient root/zone/actor/helper or runtime-vehicle baseline is not empty | "
				+ snapshotEvidence;
			return false;
		}
		HST_CampaignDebugAmbientMutableRegistrySnapshot mutableRegistries;
		if (!CaptureCampaignDebugAmbientMutableRegistries(
			mutableRegistries,
			snapshotEvidence)
			|| mutableRegistries.m_bConsequenceAuthorityFault
			|| !mutableRegistries.m_aRetryZoneIds.IsEmpty()
			|| !mutableRegistries.m_aStaticZoneIds.IsEmpty()
			|| !mutableRegistries.m_aCasualtyEventIds.IsEmpty()
			|| !mutableRegistries.m_aTheftEventIds.IsEmpty()
			|| !mutableRegistries.m_aPanicZoneIds.IsEmpty()
			|| !mutableRegistries.m_aResetPreservedVehicles.IsEmpty()
			|| !mutableRegistries.m_aResetPreservedCargo.IsEmpty())
		{
			evidence = "global ambient mutable-registry baseline is not empty and fault-free | "
				+ snapshotEvidence;
			return false;
		}
		HST_AmbientPopulationBudgetPlan admissionPlan;
		HST_AmbientPopulationTownAllocation allocation
			= ResolveCampaignDebugSelectedTownAllocation(
				state,
				balance,
				zoneId,
				forcedCivilianPresence,
				admissionPlan,
				snapshotEvidence);
		if (!allocation)
		{
			evidence = snapshotEvidence;
			return false;
		}
		evidence = "globally empty admission baseline | " + snapshotEvidence;
		return true;
	}

	bool AdmitCampaignDebugCivilianTownPopulation(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		string zoneId,
		HST_CampaignDebugAmbientServiceStateSnapshot serviceBaseline,
		string mutationLeaseReceiptId,
		out bool changed,
		out int expectedPedestrians,
		out int expectedTraffic,
		out HST_AmbientPopulationBudgetPlan ownedPlan,
		out string ownedPlanFingerprint,
		out string evidence)
	{
		changed = false;
		expectedPedestrians = 0;
		expectedTraffic = 0;
		ownedPlan = null;
		ownedPlanFingerprint = "";
		evidence = "campaign-debug selected-town admission rejected";
		string mutationReceiptEvidence;
		if (!BeginCampaignDebugCivilianMutationAttempt(
			state,
			zoneId,
			mutationLeaseReceiptId,
			"admit",
			mutationReceiptEvidence))
		{
			evidence = mutationReceiptEvidence;
			return false;
		}
		if (!IsCampaignDebugAmbientServiceStateExact(
			state,
			serviceBaseline,
			evidence))
			return false;
		HST_CivilianZoneState town;
		if (state)
			town = state.FindCivilianZone(zoneId);
		if (!town)
			return false;
		HST_AmbientPopulationBudgetPlan admittedPlan;
		HST_AmbientPopulationTownAllocation allocation
			= ResolveCampaignDebugSelectedTownAllocation(
				state,
				balance,
				zoneId,
				town.m_iCivilianPresence,
				admittedPlan,
				evidence);
		if (!allocation)
			return false;
		if (!RecordCampaignDebugCivilianMutationBudgetPlan(
			admittedPlan,
			mutationReceiptEvidence))
		{
			evidence = mutationReceiptEvidence;
			return false;
		}
		m_LastAmbientBudgetPlan = admittedPlan;
		m_iAmbientRotationEpoch = admittedPlan.m_iRotationEpoch;
		expectedPedestrians = allocation.m_iAllocatedPedestrians;
		expectedTraffic = allocation.m_iAllocatedTraffic;
		int previousSpawnBudget = m_iAmbientSpawnBudgetRemaining;
		m_iAmbientSpawnBudgetRemaining = AMBIENT_SPAWN_TRANSACTIONS_PER_UPDATE;
		changed = ReconcileAmbientTownAllocation(
			state,
			preset,
			balance,
			allocation);
		m_iAmbientSpawnBudgetRemaining = previousSpawnBudget;
		PublishRuntimeDiagnostics(state);
		ownedPlan = admittedPlan;
		ownedPlanFingerprint
			= BuildCampaignDebugAmbientBudgetPlanFingerprint(admittedPlan);
		string ownershipEvidence;
		if (!IsCampaignDebugAmbientPlanOwnershipExact(
			ownedPlan,
			ownedPlanFingerprint,
			ownershipEvidence))
		{
			evidence = ownershipEvidence;
			return false;
		}
		evidence = "selected-town production reconciliation admitted | "
			+ mutationReceiptEvidence + " | " + evidence;
		return true;
	}

	bool TickCampaignDebugCivilianTownPopulation(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		string zoneId,
		string mutationLeaseReceiptId,
		HST_AmbientPopulationBudgetPlan ownedPlan,
		string ownedPlanFingerprint,
		out bool changed,
		out string evidence)
	{
		changed = false;
		evidence = "campaign-debug selected-town tick rejected";
		string mutationReceiptEvidence;
		if (!BeginCampaignDebugCivilianMutationAttempt(
			state,
			zoneId,
			mutationLeaseReceiptId,
			"tick",
			mutationReceiptEvidence))
		{
			evidence = mutationReceiptEvidence;
			return false;
		}
		if (!state || !preset || !balance || zoneId.IsEmpty()
			|| !IsCampaignDebugAmbientPlanOwnershipExact(
				ownedPlan,
				ownedPlanFingerprint,
				evidence))
			return false;
		foreach (string runtimeZoneId : m_aRuntimeEntityZoneIds)
		{
			if (runtimeZoneId != zoneId)
			{
				evidence = "ambient runtime root escaped the selected-town plan owner";
				return false;
			}
		}
		HST_AmbientPopulationTownAllocation allocation
			= m_LastAmbientBudgetPlan.Find(zoneId);
		if (!allocation)
		{
			evidence = "owned ambient plan lost its selected-town allocation";
			return false;
		}
		m_iAmbientSpawnBudgetRemaining = AMBIENT_SPAWN_TRANSACTIONS_PER_UPDATE;
		changed = TickAmbientActorRuntime(state, balance);
		changed = ReconcileAmbientTownAllocation(
			state,
			preset,
			balance,
			allocation) || changed;
		SuppressAmbientTrafficHornInput(zoneId);
		PublishRuntimeDiagnostics(state);
		if (!IsCampaignDebugAmbientPlanOwnershipExact(
			ownedPlan,
			ownedPlanFingerprint,
			evidence))
			return false;
		evidence = "ordinary ambient runtime tick retained exact selected-town plan ownership | "
			+ mutationReceiptEvidence;
		return true;
	}

	protected bool ReconcileAmbientPopulationPlan(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		string priorityZoneId = "")
	{
		if (!state || !balance)
			return false;
		bool changed;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;
			if (m_LastAmbientBudgetPlan
				&& m_LastAmbientBudgetPlan.Find(zone.m_sZoneId))
				continue;
			changed = CleanupZoneRuntimeEntities(state, zone.m_sZoneId) || changed;
		}
		if (!m_LastAmbientBudgetPlan)
			return changed;

		int allocationCount = m_LastAmbientBudgetPlan.m_aTownAllocations.Count();
		int startIndex;
		if (!priorityZoneId.IsEmpty())
		{
			for (int priorityIndex; priorityIndex < allocationCount; priorityIndex++)
			{
				HST_AmbientPopulationTownAllocation priorityAllocation
					= m_LastAmbientBudgetPlan.m_aTownAllocations[priorityIndex];
				if (priorityAllocation
					&& priorityAllocation.m_sZoneId == priorityZoneId)
				{
					startIndex = priorityIndex;
					break;
				}
			}
		}
		else if (allocationCount > 0)
			startIndex = m_iAmbientReconciliationCursor % allocationCount;
		for (int offset; offset < allocationCount; offset++)
		{
			HST_AmbientPopulationTownAllocation allocation
				= m_LastAmbientBudgetPlan.m_aTownAllocations[
					(startIndex + offset) % allocationCount];
			changed = ReconcileAmbientTownAllocation(
				state,
				preset,
				balance,
				allocation) || changed;
		}
		if (allocationCount > 0 && priorityZoneId.IsEmpty())
		{
			m_iAmbientReconciliationCursor
				= (m_iAmbientReconciliationCursor + 1) % allocationCount;
		}
		return changed;
	}

	HST_PlayerUndercoverState EnsurePlayer(HST_CampaignState state, string identityId)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return null;
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
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!state || zoneId.IsEmpty())
			return false;

		int occupierDelta = -reputationDelta / 2;
		if (heatDelta > 0)
			occupierDelta += heatDelta;

		return RegisterInfluenceEvent(state, zoneId, "incident", reputationDelta, occupierDelta, reputationDelta, heatDelta, 0, 0, 0, reason, preset);
	}

	bool RegisterInfluenceEvent(HST_CampaignState state, string zoneId, string eventKind, int fiaSupportDelta, int occupierSupportDelta, int reputationDelta, int heatDelta, int populationDelta, int policeDelta, int roadblockDelta, string reason, HST_CampaignPreset preset = null, int durationSeconds = 0, string sourceId = "")
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!m_TownInfluence)
			return false;
		return m_TownInfluence.RegisterInfluenceEvent(
			state, zoneId, eventKind, fiaSupportDelta, occupierSupportDelta,
			reputationDelta, heatDelta, populationDelta, policeDelta,
			roadblockDelta, reason, ResolveInfluencePreset(preset),
			durationSeconds, sourceId, 0,
			fiaSupportDelta != 0 || occupierSupportDelta != 0);
	}

	bool RegisterInfluenceEventExact(
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
		HST_CampaignPreset preset,
		int durationSeconds,
		string sourceId,
		string exactEventId = "",
		bool reconcileOwnership = true)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!m_TownInfluence)
			return false;
		HST_TownInfluenceCommand command = new HST_TownInfluenceCommand();
		command.m_sCommandId = exactEventId;
		command.m_sEventId = exactEventId;
		command.m_sTownId = zoneId;
		command.m_sEventKind = eventKind;
		command.m_sSourceId = sourceId;
		command.m_sReason = reason;
		command.m_iRawFIASupportDelta = fiaSupportDelta;
		command.m_iRawOccupierSupportDelta = occupierSupportDelta;
		command.m_iReputationDelta = reputationDelta;
		command.m_iHeatDelta = heatDelta;
		command.m_iPopulationDelta = populationDelta;
		command.m_iPoliceDelta = policeDelta;
		command.m_iRoadblockDelta = roadblockDelta;
		command.m_iDurationSeconds = durationSeconds;
		command.m_bPopulationScaled = fiaSupportDelta != 0
			|| occupierSupportDelta != 0;
		command.m_bReconcileOwnership = reconcileOwnership;
		return m_TownInfluence.RegisterInfluenceEventExact(
			state,
			command,
			ResolveInfluencePreset(preset));
	}

	protected void ApplyInfluenceEvent(
		HST_CampaignState state,
		HST_CivilianZoneState civilianZone,
		HST_TownInfluenceEventState influenceEvent,
		HST_CampaignPreset preset = null,
		bool reconcileOwnership = true)
	{
		// Compatibility-only legacy hook. Canonical mutations execute through
		// HST_TownInfluenceService before reaching this obsolete signature.
	}

	protected bool RefreshTownInfluenceAggregates(HST_CampaignState state)
	{
		return false;
	}

	protected void RefreshTownInfluenceAggregatesForZone(HST_CampaignState state, HST_CivilianZoneState civilianZone)
	{
		// Aggregate counters are maintained incrementally by the canonical service.
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

	protected bool ExactInfluenceEventMatches(
		HST_TownInfluenceEventState influenceEvent,
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
		string sourceId,
		int durationSeconds,
		int currentSecond)
	{
		if (!influenceEvent || !influenceEvent.m_bApplied
			|| influenceEvent.m_iCreatedAtSecond > currentSecond)
			return false;
		bool durationMatches = influenceEvent.m_iExpiresAtSecond == 0 && durationSeconds <= 0;
		if (durationSeconds > 0)
			durationMatches = influenceEvent.m_iExpiresAtSecond == influenceEvent.m_iCreatedAtSecond + durationSeconds;
		return durationMatches
			&& influenceEvent.m_sZoneId == zoneId
			&& influenceEvent.m_sKind == eventKind
			&& influenceEvent.m_sSourceId == sourceId
			&& influenceEvent.m_sReason == reason
			&& influenceEvent.m_iFIASupportDelta == fiaSupportDelta
			&& influenceEvent.m_iOccupierSupportDelta == occupierSupportDelta
			&& influenceEvent.m_iReputationDelta == reputationDelta
			&& influenceEvent.m_iHeatDelta == heatDelta
			&& influenceEvent.m_iPopulationDelta == populationDelta
			&& influenceEvent.m_iPoliceDelta == policeDelta
			&& influenceEvent.m_iRoadblockDelta == roadblockDelta;
	}

	protected bool ApplyTownSupportOwnershipPolicy(HST_CampaignState state, HST_CampaignPreset preset, HST_CivilianZoneState civilianZone, HST_ZoneState zone)
	{
		if (!m_TownInfluence || !zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return false;
		m_TownInfluence.ReconcileTownOwnershipPolicies(state, preset, true);
		return m_TownInfluence.FindValidRecord(state, zone.m_sZoneId) != null;
	}

	bool ReconcileTownOwnershipPolicies(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		bool bypassCampaignClockRateLimit = false)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!m_TownInfluence)
			return false;
		return m_TownInfluence.ReconcileTownOwnershipPolicies(
			state,
			ResolveInfluencePreset(preset),
			bypassCampaignClockRateLimit);
	}

	protected bool HasUnresolvedTopLevelOwnershipTransition(HST_CampaignState state)
	{
		if (!state)
			return false;
		foreach (HST_OwnershipTransitionState transition : state.m_aOwnershipTransitions)
		{
			if (transition && !transition.m_bCompleted
				&& transition.m_sProjectionParentRequestId.IsEmpty())
				return true;
		}
		return false;
	}

	protected bool ApplyPoliticalOwnershipTransition(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_CivilianZoneState civilianZone,
		HST_ZoneState zone,
		string newOwnerFactionKey,
		string reason)
	{
		if (!state || !preset || !civilianZone || !zone || !m_OwnershipTransitions)
			return false;
		string sourceId = civilianZone.m_sLastInfluenceEventId;
		if (sourceId.IsEmpty())
			sourceId = string.Format("town_policy_%1_%2", zone.m_sZoneId, state.m_iElapsedSeconds);
		string requestId = string.Format(
			"ownership_political_%1_%2_%3",
			zone.m_sZoneId,
			Math.Max(1, zone.m_iOwnershipRevision),
			sourceId.Hash());
		HST_OwnershipTransitionRequest request = m_OwnershipTransitions.BuildRequest(
			state,
			zone.m_sZoneId,
			newOwnerFactionKey,
			"political_support",
			"town_influence",
			sourceId,
			reason,
			0,
			requestId);
		if (state.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_SETUP
			|| state.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_WON
			|| state.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_LOST)
		{
			// Frozen-phase reconciliation repairs durable political truth only; it
			// must not start fresh retaliation or gameplay notifications after the
			// campaign runtime has stopped.
			request.m_bApplyEnemyConsequences = false;
			request.m_bNotify = false;
		}
		HST_OwnershipTransitionResult result = m_OwnershipTransitions.Apply(state, request);
		if (!result || !result.m_bAccepted || !result.m_bCompleted)
			return false;
		civilianZone.m_sLastSecurityReason = reason;
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
		if (m_CampaignDebugCivilianMutationLease)
			return false;
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
			return "Partisan civilians | state not ready";

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

		string summary = string.Format("Partisan civilians | towns %1 | avg reputation %2 | heat %3 | police %4 | roadblocks %5", townCount, averageReputation, heatTotal, policeTotal, roadblocks);
		return summary + "\n" + BuildTownSupportReport(state);
	}

	string BuildTownSupportReport(HST_CampaignState state, int maxRows = 20)
	{
		if (!state)
			return "Partisan town support | state not ready";

		if (!m_TownInfluence)
			return "Partisan town support | canonical authority unavailable";
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
			HST_TownInfluenceRecord influence = m_TownInfluence.FindValidRecord(
				state,
				civilianZone.m_sZoneId);
			if (!influence)
				continue;

			townCount++;
			fiaSupportTotal += Math.Round(influence.m_iFIASupportBasisPoints / 100.0);
			occupierSupportTotal += Math.Round(Math.Max(
				influence.m_iOccupierSupportBasisPoints,
				influence.m_iInvaderSupportBasisPoints) / 100.0);
			reputationTotal += civilianZone.m_iReputation;
			heatTotal += civilianZone.m_iWantedHeat;
			policeTotal += civilianZone.m_iPolicePresence;
			roadblockTotal += civilianZone.m_iRoadblockPresence;
			populationRemainingTotal += influence.m_iRemainingPopulation;
			populationKilledTotal += influence.m_iDestroyedPopulation;
			influenceEventTotal += influence.m_iInfluenceEventCount;
			activeInfluenceTotal += influence.m_iActiveInfluenceModifierCount;
			expiredInfluenceTotal += influence.m_iExpiredInfluenceModifierCount;
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
			"Partisan town support | towns %1 | avg FIA %2 | avg occupier %3 | avg rep %4 | heat %5 | police %6 | roadblocks %7",
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
			HST_TownInfluenceRecord influence = m_TownInfluence.FindValidRecord(
				state,
				town.m_sZoneId);
			if (!influence)
				continue;

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
				strategicSupport = m_TownInfluence.ResolveSignedSupportPercent(
					state,
					zone.m_sZoneId);
				active = zone.m_bActive;
			}

			int age;
			if (town.m_iLastIncidentSecond > 0)
				age = Math.Max(0, state.m_iElapsedSeconds - town.m_iLastIncidentSecond);

			string line = string.Format(
				"\n%1 | owner %2 | FIA %3 | occupier %4 | rep %5 | heat %6 | police %7 | roadblocks %8 | civs %9",
				label,
				owner,
				Math.Round(influence.m_iFIASupportBasisPoints / 100.0),
				Math.Round(Math.Max(influence.m_iOccupierSupportBasisPoints, influence.m_iInvaderSupportBasisPoints) / 100.0),
				town.m_iReputation,
				town.m_iWantedHeat,
				town.m_iPolicePresence,
				town.m_iRoadblockPresence,
				town.m_iCivilianPresence
			);
			line = line + string.Format(" | restricted %1 | support %2 | active %3 | last %4s %5", town.m_bUndercoverRestricted, strategicSupport, active, age, town.m_sLastIncidentReason);
			line = line + string.Format(" | population %1 killed %2 | influence %3 active %4 expired %5", influence.m_iRemainingPopulation, influence.m_iDestroyedPopulation, influence.m_iInfluenceEventCount, influence.m_iActiveInfluenceModifierCount, influence.m_iExpiredInfluenceModifierCount);
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
			return "Partisan undercover | state not ready";

		if (!identityId.IsEmpty())
		{
			HST_PlayerUndercoverState undercover = state.FindUndercoverPlayer(identityId);
			if (!undercover)
				return "Partisan undercover | no record";

			string report = string.Format(
				"Partisan undercover | %1 | requested %2 | applied %3 | eligible %4 | status %5 | heat %6 | zone %7 | enforcement zone %8 | score %9",
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
			"Partisan undercover | tracked %1 | requested %2 | applied %3 | eligible %4 | clear %5 | suspicious %6 | compromised %7 | wanted %8",
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
		if (m_CampaignDebugCivilianMutationLease)
		{
			result.m_bEligible = false;
			result.m_sSummary = "campaign-debug civilian mutation lease is active";
			return result;
		}

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
		if (m_CampaignDebugCivilianMutationLease)
			return "Partisan undercover | failed: campaign-debug civilian mutation lease is active";
		if (!state || identityId.IsEmpty())
			return "Partisan undercover | failed: state or identity missing";

		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!undercover)
			return "Partisan undercover | failed: could not create player record";

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
				return "Partisan undercover | request denied\n" + eligibility.BuildReport();
			}

			undercover.m_sLastReason = "request denied: no eligibility result";
			return "Partisan undercover | request denied";
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
		return "Partisan undercover | request accepted\n" + eligibility.BuildReport();
	}

	string ClearUndercoverCompromise(HST_CampaignState state, string identityId)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return "Partisan undercover | failed: campaign-debug civilian mutation lease is active";
		if (!state || identityId.IsEmpty())
			return "Partisan undercover | failed: state or identity missing";

		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!undercover)
			return "Partisan undercover | failed: no player record";

		if (undercover.m_iWantedHeat > 0)
			return string.Format("Partisan undercover | failed: wanted heat %1 must cool before clearing", undercover.m_iWantedHeat);

		if (undercover.m_eStatus == HST_EUndercoverStatus.HST_UNDERCOVER_COMPROMISED && state.m_iElapsedSeconds < undercover.m_iCompromisedUntilSecond)
			return string.Format("Partisan undercover | failed: compromised for %1s", undercover.m_iCompromisedUntilSecond - state.m_iElapsedSeconds);

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
		return "Partisan undercover | cleared";
	}

	HST_UndercoverEnforcementResult EnforceUndercoverForPlayer(HST_CampaignState state, HST_CampaignPreset preset, string identityId, IEntity playerEntity)
	{
		HST_UndercoverEnforcementResult result = new HST_UndercoverEnforcementResult();
		result.m_sIdentityId = identityId;
		if (m_CampaignDebugCivilianMutationLease)
		{
			result.m_bBlocked = true;
			result.m_sReason = "campaign-debug civilian mutation lease is active";
			return result;
		}

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
		int score = BuildUndercoverDetectionScore(state, preset, undercover, civilianZone, eligibility, playerEntity, compromiseReason, detectionSource);
		result.m_iDetectionScore = score;
		result.m_sReason = compromiseReason;
		result.m_sDetectionSource = detectionSource;

		undercover.m_iDetectionScore = score;
		undercover.m_sLastDetectionSource = detectionSource;
		undercover.m_sLastEnforcementZoneId = zoneId;

		string scanReason;
		if (TryRoadblockScan(state, preset, undercover, civilianZone, eligibility, scanReason))
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

		if (TryPoliceScan(state, preset, undercover, civilianZone, eligibility, scanReason))
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
		if (m_CampaignDebugCivilianMutationLease)
			return "Partisan undercover | failed: campaign-debug civilian mutation lease is active";
		if (!state || identityId.IsEmpty())
			return "Partisan undercover | failed: state or identity missing";

		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!undercover)
			return "Partisan undercover | failed: no undercover record";

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zoneId);
		CompromiseUndercover(state, undercover, civilianZone, zoneId, reason, "combat", 100);
		return "Partisan undercover | compromised: " + reason;
	}


	string RegisterUndercoverVehicleExposure(HST_CampaignState state, string identityId, string zoneId, string reason, string vehicleRuntimeId = "", HST_StrategicService strategic = null)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return "Partisan undercover | failed: campaign-debug civilian mutation lease is active";
		if (!state || identityId.IsEmpty())
			return "Partisan undercover | failed: state or identity missing";

		HST_PlayerUndercoverState undercover = EnsurePlayer(state, identityId);
		if (!undercover)
			return "Partisan undercover | failed: no undercover record";

		string vehicleReport;
		if (!vehicleRuntimeId.IsEmpty())
			vehicleReport = RegisterVehiclePassengerCompromise(state, vehicleRuntimeId, identityId, zoneId, reason, strategic);

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zoneId);
		CompromiseUndercover(state, undercover, civilianZone, zoneId, reason, "vehicle", 90);
		if (!vehicleReport.IsEmpty())
			return "Partisan undercover | compromised: " + reason + "\n" + vehicleReport;

		return "Partisan undercover | compromised: " + reason;
	}

	string RegisterVehicleHeat(HST_CampaignState state, string vehicleRuntimeId, string zoneId, int heatDelta, int durationSeconds, string reason, bool reported = true, HST_StrategicService strategic = null)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return "Partisan vehicle heat | failed: campaign-debug civilian mutation lease is active";
		if (!state || vehicleRuntimeId.IsEmpty())
			return "Partisan vehicle heat | failed: state or runtime vehicle id missing";

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
		if (!vehicle)
			return "Partisan vehicle heat | failed: runtime vehicle not tracked";

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

		string report = string.Format("Partisan vehicle heat | %1 | heat %2 -> %3 | reported %4 | until %5", vehicleRuntimeId, oldHeat, vehicle.m_iVehicleHeat, vehicle.m_bReported, vehicle.m_iReportedUntilSecond);
		return report + string.Format(" | zone %1 | reason %2", EmptyRuntimeField(zoneId), EmptyRuntimeField(reason));
	}

	string RegisterVehiclePassengerCompromise(HST_CampaignState state, string vehicleRuntimeId, string identityId, string zoneId, string reason, HST_StrategicService strategic = null)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return "Partisan vehicle heat | failed: campaign-debug civilian mutation lease is active";
		if (!state || vehicleRuntimeId.IsEmpty())
			return "Partisan vehicle heat | failed: state or runtime vehicle id missing";

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
		if (!vehicle)
			return "Partisan vehicle heat | failed: runtime vehicle not tracked";

		vehicle.m_iPassengerCompromiseCount = vehicle.m_iPassengerCompromiseCount + 1;
		string report = RegisterVehicleHeat(state, vehicleRuntimeId, zoneId, 4, VEHICLE_REPORT_DEFAULT_SECONDS, reason, true, strategic);
		return report + string.Format(" | passenger compromises %1 | identity %2", vehicle.m_iPassengerCompromiseCount, EmptyRuntimeField(identityId));
	}

	string ClearVehicleHeat(HST_CampaignState state, string vehicleRuntimeId, string reason = "cleared")
	{
		if (m_CampaignDebugCivilianMutationLease)
			return "Partisan vehicle heat | failed: campaign-debug civilian mutation lease is active";
		if (!state || vehicleRuntimeId.IsEmpty())
			return "Partisan vehicle heat | failed: state or runtime vehicle id missing";

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
		if (!vehicle)
			return "Partisan vehicle heat | failed: runtime vehicle not tracked";

		vehicle.m_iVehicleHeat = 0;
		vehicle.m_bReported = false;
		vehicle.m_iReportedUntilSecond = 0;
		vehicle.m_iLastVehicleHeatChangedSecond = state.m_iElapsedSeconds;
		vehicle.m_sLastReportedReason = reason;
		return string.Format("Partisan vehicle heat | %1 | cleared | reason %2", vehicleRuntimeId, EmptyRuntimeField(reason));
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
			return "Partisan vehicle heat | state unavailable";

		string report = string.Format("Partisan vehicle heat | tracked %1", state.m_aRuntimeVehicles.Count());
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

	protected int BuildUndercoverDetectionScore(HST_CampaignState state, HST_CampaignPreset preset, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility, IEntity playerEntity, out string reason, out string source)
	{
		int score;
		reason = "clear";
		source = "none";
		int warLevel = ResolveUndercoverSecurityWarLevel(state);
		int aggression = ResolveUndercoverSecurityAggression(state, preset, civilianZone);

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
			score += Math.Max(0, warLevel) * 2;
			score += Math.Max(0, aggression) / 20;

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

	protected bool TryRoadblockScan(HST_CampaignState state, HST_CampaignPreset preset, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility, out string reason)
	{
		reason = "";

		if (!state || !undercover || !civilianZone || civilianZone.m_iRoadblockPresence <= 0)
			return false;

		if (state.m_iElapsedSeconds < civilianZone.m_iLastRoadblockScanSecond + UNDERCOVER_ROADBLOCK_SCAN_COOLDOWN_SECONDS)
			return false;

		int chance = CalculateRoadblockScanChance(state, preset, undercover, civilianZone, eligibility);
		int roll = BuildUndercoverSecurityScanRoll(state, undercover, civilianZone, "roadblock", undercover.m_iRoadblockScanCount);
		int warLevel = ResolveUndercoverSecurityWarLevel(state);
		int aggression = ResolveUndercoverSecurityAggression(state, preset, civilianZone);

		civilianZone.m_iLastRoadblockScanSecond = state.m_iElapsedSeconds;
		undercover.m_iRoadblockScanCount++;

		if (roll < chance)
		{
			reason = BuildUndercoverSecurityScanReason("roadblock", true, chance, roll, warLevel, aggression, civilianZone.m_iRoadblockPresence, civilianZone.m_iWantedHeat, undercover.m_iWantedHeat);
			civilianZone.m_sLastSecurityReason = reason;
			undercover.m_bLastRoadblockScanFailed = true;
			return true;
		}

		reason = BuildUndercoverSecurityScanReason("roadblock", false, chance, roll, warLevel, aggression, civilianZone.m_iRoadblockPresence, civilianZone.m_iWantedHeat, undercover.m_iWantedHeat);
		civilianZone.m_sLastSecurityReason = reason;
		undercover.m_bLastRoadblockScanFailed = false;
		return false;
	}

	protected bool TryPoliceScan(HST_CampaignState state, HST_CampaignPreset preset, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility, out string reason)
	{
		reason = "";

		if (!state || !undercover || !civilianZone || civilianZone.m_iPolicePresence <= 0)
			return false;

		if (state.m_iElapsedSeconds < civilianZone.m_iLastPoliceScanSecond + UNDERCOVER_POLICE_SCAN_COOLDOWN_SECONDS)
			return false;

		int chance = CalculatePoliceScanChance(state, preset, undercover, civilianZone, eligibility);
		int roll = BuildUndercoverSecurityScanRoll(state, undercover, civilianZone, "police", undercover.m_iPoliceScanCount);
		int warLevel = ResolveUndercoverSecurityWarLevel(state);
		int aggression = ResolveUndercoverSecurityAggression(state, preset, civilianZone);

		civilianZone.m_iLastPoliceScanSecond = state.m_iElapsedSeconds;
		undercover.m_iPoliceScanCount++;

		if (roll < chance)
		{
			reason = BuildUndercoverSecurityScanReason("police", true, chance, roll, warLevel, aggression, civilianZone.m_iPolicePresence, civilianZone.m_iWantedHeat, undercover.m_iWantedHeat);
			civilianZone.m_sLastSecurityReason = reason;
			undercover.m_bLastPoliceScanFailed = true;
			return true;
		}

		reason = BuildUndercoverSecurityScanReason("police", false, chance, roll, warLevel, aggression, civilianZone.m_iPolicePresence, civilianZone.m_iWantedHeat, undercover.m_iWantedHeat);
		civilianZone.m_sLastSecurityReason = reason;
		undercover.m_bLastPoliceScanFailed = false;
		return false;
	}

	int DebugCalculateRoadblockScanChance(HST_CampaignState state, HST_CampaignPreset preset, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility)
	{
		return CalculateRoadblockScanChance(state, preset, undercover, civilianZone, eligibility);
	}

	int DebugCalculatePoliceScanChance(HST_CampaignState state, HST_CampaignPreset preset, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility)
	{
		return CalculatePoliceScanChance(state, preset, undercover, civilianZone, eligibility);
	}

	protected int CalculateRoadblockScanChance(HST_CampaignState state, HST_CampaignPreset preset, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility)
	{
		if (!state || !civilianZone)
			return 0;

		int chance = 20;
		chance += Math.Max(0, civilianZone.m_iRoadblockPresence) * 14;
		chance += Math.Max(0, civilianZone.m_iWantedHeat) * 6;
		chance += ResolveUndercoverSecurityWarLevel(state) * 4;
		chance += ResolveUndercoverSecurityAggression(state, preset, civilianZone) / 8;
		if (undercover)
			chance += Math.Max(0, undercover.m_iWantedHeat) * 10;
		if (eligibility)
		{
			if (eligibility.m_sVehicleReason.Contains("BLOCK"))
				chance += 30;
			if (eligibility.m_sWeaponReason.Contains("BLOCK"))
				chance += 30;
			if (eligibility.m_sClothingReason.Contains("WARN"))
				chance += 10;
			if (eligibility.m_sOffroadReason.Contains("BLOCK") || eligibility.m_sOffroadReason.Contains("WARN"))
				chance += 15;
		}

		return Math.Max(0, Math.Min(100, chance));
	}

	protected int CalculatePoliceScanChance(HST_CampaignState state, HST_CampaignPreset preset, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, HST_UndercoverEligibilityResult eligibility)
	{
		if (!state || !civilianZone)
			return 0;

		int chance = 12;
		chance += Math.Max(0, civilianZone.m_iPolicePresence) * 11;
		chance += Math.Max(0, civilianZone.m_iWantedHeat) * 7;
		chance += ResolveUndercoverSecurityWarLevel(state) * 3;
		chance += ResolveUndercoverSecurityAggression(state, preset, civilianZone) / 10;
		if (undercover)
			chance += Math.Max(0, undercover.m_iWantedHeat) * 10;
		if (eligibility)
		{
			if (eligibility.m_sWeaponReason.Contains("BLOCK"))
				chance += 30;
			if (eligibility.m_sVehicleReason.Contains("BLOCK"))
				chance += 25;
			if (eligibility.m_sClothingReason.Contains("WARN"))
				chance += 8;
		}

		return Math.Max(0, Math.Min(100, chance));
	}

	protected int ResolveUndercoverSecurityWarLevel(HST_CampaignState state)
	{
		if (!state)
			return 0;

		return Math.Max(0, state.m_iWarLevel);
	}

	protected int ResolveUndercoverSecurityAggression(HST_CampaignState state, HST_CampaignPreset preset, HST_CivilianZoneState civilianZone)
	{
		if (!state)
			return 0;

		HST_ZoneState zone;
		if (civilianZone)
			zone = state.FindZone(civilianZone.m_sZoneId);
		if (zone && !zone.m_sOwnerFactionKey.IsEmpty() && (!preset || zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey))
		{
			HST_FactionPoolState ownerPool = state.FindFactionPool(zone.m_sOwnerFactionKey);
			if (ownerPool)
				return Math.Max(0, ownerPool.m_iAggression);
		}

		int aggression;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool)
				continue;
			if (preset && pool.m_sFactionKey == preset.m_sResistanceFactionKey)
				continue;
			if (pool.m_sFactionKey == "CIV")
				continue;

			aggression = Math.Max(aggression, pool.m_iAggression);
		}

		return Math.Max(0, aggression);
	}

	protected int BuildUndercoverSecurityScanRoll(HST_CampaignState state, HST_PlayerUndercoverState undercover, HST_CivilianZoneState civilianZone, string source, int salt)
	{
		int seed = salt * 37;
		if (state)
			seed += state.m_iElapsedSeconds;
		if (undercover)
			seed += undercover.m_sIdentityId.Length() * 17 + undercover.m_iRoadblockScanCount * 11 + undercover.m_iPoliceScanCount * 13;
		if (civilianZone)
			seed += civilianZone.m_sZoneId.Length() * 19 + civilianZone.m_iWantedHeat * 5 + civilianZone.m_iPolicePresence * 3 + civilianZone.m_iRoadblockPresence * 7;
		if (source == "police")
			seed += 29;

		if (seed < 0)
			seed = -seed;
		return seed % 100;
	}

	protected string BuildUndercoverSecurityScanReason(string source, bool failed, int chance, int roll, int warLevel, int aggression, int presence, int townHeat, int playerHeat)
	{
		string result = "passed";
		if (failed)
			result = "failed";

		return string.Format("%1 scan %2 chance %3 roll %4 | war %5 aggression %6 | presence %7 town heat %8 player heat %9", source, result, chance, roll, warLevel, aggression, presence, townHeat, playerHeat);
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

	bool DebugResolveUndercoverIdentityEligibility(string identity, out string clothingReason, out string weaponReason)
	{
		clothingReason = ResolveClothingEligibilityReasonFromIdentity(identity);
		weaponReason = ResolveWeaponEligibilityReasonFromIdentity(identity);
		return !IsBlockingReason(clothingReason) && !IsBlockingReason(weaponReason);
	}

	protected string ResolveClothingEligibilityReason(IEntity playerEntity)
	{
		string blockingEvidence;
		string civilianEvidence;
		int scannedWearables;
		if (TryResolveLiveClothingEligibilityEvidence(playerEntity, blockingEvidence, civilianEvidence, scannedWearables))
			return "BLOCK live military clothing/equipment: " + blockingEvidence;

		if (scannedWearables > 0)
		{
			if (!civilianEvidence.IsEmpty())
				return "OK live civilian clothing: " + civilianEvidence;

			return string.Format("WARN live clothing not recognized | scanned %1", scannedWearables);
		}

		return ResolveClothingEligibilityReasonFromIdentity(ResolveEntityIdentity(playerEntity));
	}

	protected string ResolveClothingEligibilityReasonFromIdentity(string identity)
	{
		if (identity.IsEmpty())
			return "WARN unknown clothing/entity identity";

		string normalized = NormalizeUndercoverIdentity(identity);
		if (IsUndercoverCivilianIdentity(normalized))
			return "OK civilian clothing";

		if (IsUndercoverResistanceIdentity(normalized))
			return "WARN resistance clothing is not civilian";

		if (IsUndercoverMilitaryClothingIdentity(normalized) || IsUndercoverMilitaryRoleIdentity(normalized))
			return "BLOCK military character/equipment identity";

		return "WARN clothing not recognized";
	}

	protected string ResolveWeaponEligibilityReason(IEntity playerEntity)
	{
		string evidence;
		if (TryResolveLiveWeaponEvidence(playerEntity, evidence))
			return "BLOCK live weapon/equipment: " + evidence;

		return ResolveWeaponEligibilityReasonFromIdentity(ResolveEntityIdentity(playerEntity));
	}

	protected string ResolveWeaponEligibilityReasonFromIdentity(string identity)
	{
		if (identity.IsEmpty())
			return "WARN unknown weapon/entity identity";

		string normalized = NormalizeUndercoverIdentity(identity);
		if (IsUndercoverMilitaryRoleIdentity(normalized))
			return "BLOCK military role implies visible/issued weapon";
		if (IsUndercoverWeaponIdentity(normalized))
			return "BLOCK weapon identity detected";

		return "OK no live weapon detected";
	}

	protected bool TryResolveLiveClothingEligibilityEvidence(IEntity playerEntity, out string blockingEvidence, out string civilianEvidence, out int scannedWearables)
	{
		blockingEvidence = "";
		civilianEvidence = "";
		scannedWearables = 0;
		if (!playerEntity)
			return false;

		SCR_CharacterInventoryStorageComponent characterStorage = SCR_CharacterInventoryStorageComponent.Cast(playerEntity.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!characterStorage)
			return false;

		int slotCount = characterStorage.GetSlotsCount();
		for (int slotIndex = 0; slotIndex < slotCount; slotIndex++)
		{
			InventoryStorageSlot slot = characterStorage.GetSlot(slotIndex);
			if (!slot)
				continue;

			IEntity attachedEntity = slot.GetAttachedEntity();
			if (!attachedEntity)
				continue;

			string itemIdentity = ResolveUndercoverItemIdentity(attachedEntity);
			string normalized = NormalizeUndercoverIdentity(itemIdentity);
			string category = ResolveUndercoverWearableCategory(attachedEntity, itemIdentity);
			bool clothingLike = !category.IsEmpty();
			clothingLike = clothingLike || BaseLoadoutClothComponent.Cast(attachedEntity.FindComponent(BaseLoadoutClothComponent)) != null;
			clothingLike = clothingLike || IsUndercoverClothingLikeIdentity(normalized);
			if (!clothingLike)
				continue;

			scannedWearables++;
			if (IsUndercoverMilitaryClothingIdentity(normalized))
			{
				blockingEvidence = BuildUndercoverItemEvidence(attachedEntity, itemIdentity);
				return true;
			}

			if (civilianEvidence.IsEmpty() && IsUndercoverCivilianIdentity(normalized))
				civilianEvidence = BuildUndercoverItemEvidence(attachedEntity, itemIdentity);
		}

		return false;
	}

	protected bool TryResolveLiveWeaponEvidence(IEntity playerEntity, out string evidence)
	{
		evidence = "";
		if (!playerEntity)
			return false;

		BaseWeaponManagerComponent weaponManager = BaseWeaponManagerComponent.Cast(playerEntity.FindComponent(BaseWeaponManagerComponent));
		if (weaponManager)
		{
			BaseWeaponComponent currentWeapon = weaponManager.GetCurrentWeapon();
			if (currentWeapon && IsUndercoverBlockingWeaponItem(currentWeapon.GetOwner()))
			{
				evidence = "current " + BuildUndercoverItemEvidence(currentWeapon.GetOwner());
				return true;
			}

			array<IEntity> weaponEntities = {};
			weaponManager.GetWeaponsList(weaponEntities);
			foreach (IEntity weaponEntity : weaponEntities)
			{
				if (!IsUndercoverBlockingWeaponItem(weaponEntity))
					continue;

				evidence = "equipped " + BuildUndercoverItemEvidence(weaponEntity);
				return true;
			}
		}

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return false;

		array<IEntity> items = {};
		array<IEntity> visited = {};
		GatherUndercoverInventoryItemsRecursive(inventory, items, visited, 0);
		foreach (IEntity item : items)
		{
			if (!IsUndercoverBlockingWeaponItem(item))
				continue;

			evidence = "inventory " + BuildUndercoverItemEvidence(item);
			return true;
		}

		return false;
	}

	protected void GatherUndercoverInventoryItemsRecursive(SCR_InventoryStorageManagerComponent inventory, notnull array<IEntity> outItems, notnull array<IEntity> visited, int depth)
	{
		if (!inventory || depth > 8)
			return;

		array<IEntity> directItems = {};
		inventory.GetItems(directItems, EStoragePurpose.PURPOSE_ANY);
		foreach (IEntity item : directItems)
			GatherUndercoverInventoryItemRecursive(item, outItems, visited, depth);
	}

	protected void GatherUndercoverStorageItemsRecursive(BaseInventoryStorageComponent storage, notnull array<IEntity> outItems, notnull array<IEntity> visited, int depth)
	{
		if (!storage || depth > 8)
			return;

		array<InventoryItemComponent> itemComponents = {};
		storage.GetOwnedItems(itemComponents);
		foreach (InventoryItemComponent itemComponent : itemComponents)
		{
			if (!itemComponent)
				continue;

			InventoryStorageSlot parentSlot = itemComponent.GetParentSlot();
			if (parentSlot)
				GatherUndercoverInventoryItemRecursive(parentSlot.GetAttachedEntity(), outItems, visited, depth);
		}

		int slotCount = storage.GetSlotsCount();
		for (int slotIndex = 0; slotIndex < slotCount; slotIndex++)
		{
			InventoryStorageSlot slot = storage.GetSlot(slotIndex);
			if (!slot)
				continue;

			GatherUndercoverInventoryItemRecursive(slot.GetAttachedEntity(), outItems, visited, depth);
		}
	}

	protected void GatherUndercoverInventoryItemRecursive(IEntity item, notnull array<IEntity> outItems, notnull array<IEntity> visited, int depth)
	{
		if (!item)
			return;
		if (depth > 8)
			return;
		if (visited.Find(item) >= 0)
			return;

		visited.Insert(item);

		SCR_InventoryStorageManagerComponent childInventory = SCR_InventoryStorageManagerComponent.Cast(item.FindComponent(SCR_InventoryStorageManagerComponent));
		if (childInventory)
			GatherUndercoverInventoryItemsRecursive(childInventory, outItems, visited, depth + 1);

		InventoryItemComponent itemComponent = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		BaseInventoryStorageComponent itemStorage = BaseInventoryStorageComponent.Cast(itemComponent);
		if (itemStorage)
			GatherUndercoverStorageItemsRecursive(itemStorage, outItems, visited, depth + 1);

		outItems.Insert(item);
	}

	protected bool IsUndercoverBlockingWeaponItem(IEntity item)
	{
		if (!item)
			return false;

		if (BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent)))
			return true;

		if (GrenadeMoveComponent.Cast(item.FindComponent(GrenadeMoveComponent)))
			return true;

		string itemIdentity = ResolveUndercoverItemIdentity(item);
		bool wearable = BaseLoadoutClothComponent.Cast(item.FindComponent(BaseLoadoutClothComponent)) != null;
		wearable = wearable || !ResolveUndercoverWearableCategory(item, itemIdentity).IsEmpty();
		if (wearable)
			return false;

		return IsUndercoverWeaponIdentity(NormalizeUndercoverIdentity(itemIdentity));
	}

	protected string ResolveUndercoverItemIdentity(IEntity item)
	{
		if (!item)
			return "";

		string prefab = ResolveEntityIdentity(item);
		string displayName = ResolveUndercoverItemDisplayName(item, prefab);
		if (!displayName.IsEmpty() && displayName != prefab)
			return prefab + " " + displayName;

		return prefab;
	}

	protected string ResolveUndercoverItemDisplayName(IEntity item, string prefab = "")
	{
		string displayName;
		if (item)
		{
			InventoryItemComponent itemComponent = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			if (itemComponent && itemComponent.GetUIInfo())
				displayName = itemComponent.GetUIInfo().GetName();
		}

		return HST_DisplayNameService.ResolveItemDisplayName(item, prefab, displayName);
	}

	protected string BuildUndercoverItemEvidence(IEntity item, string identity = "")
	{
		string displayName = ResolveUndercoverItemDisplayName(item, ResolveEntityIdentity(item));
		if (!displayName.IsEmpty())
			return ShortenUndercoverEvidence(displayName);

		if (identity.IsEmpty())
			identity = ResolveEntityIdentity(item);

		return ShortenUndercoverEvidence(identity);
	}

	protected string ResolveUndercoverWearableCategory(IEntity item, string identity)
	{
		string prefab = ResolveEntityIdentity(item);
		string displayName = ResolveUndercoverItemDisplayName(item, prefab);
		string category = HST_ArsenalItemFilter.ResolveWearableCategory(prefab, displayName);
		if (!category.IsEmpty())
			return category;

		return HST_ArsenalItemFilter.ResolveWearableCategory(identity, displayName);
	}

	protected string NormalizeUndercoverIdentity(string value)
	{
		string normalized = value;
		normalized.ToLower();
		normalized.Replace(" ", "_");
		return normalized;
	}

	protected string ShortenUndercoverEvidence(string value)
	{
		if (value.Length() <= 96)
			return value;

		return value.Substring(0, 93) + "...";
	}

	protected bool IsUndercoverCivilianIdentity(string normalized)
	{
		if (normalized.Contains("/civ/"))
			return true;
		if (normalized.Contains("civilian"))
			return true;
		if (normalized.Contains("genericcivilians"))
			return true;
		if (normalized.Contains("citizen"))
			return true;

		return false;
	}

	protected bool IsUndercoverResistanceIdentity(string normalized)
	{
		if (normalized.Contains("/fia/"))
			return true;
		if (normalized.Contains("fia_"))
			return true;
		if (normalized.Contains("_fia"))
			return true;
		if (normalized.Contains("resistance"))
			return true;

		return false;
	}

	protected bool IsUndercoverMilitaryClothingIdentity(string normalized)
	{
		if (IsUndercoverCivilianIdentity(normalized))
			return false;
		if (IsUndercoverResistanceIdentity(normalized))
			return false;

		if (normalized.Contains("us_army"))
			return true;
		if (normalized.Contains("/factions/us/"))
			return true;
		if (normalized.Contains("/factions/ussr/"))
			return true;
		if (normalized.Contains("ussr"))
			return true;
		if (normalized.Contains("soviet"))
			return true;
		if (normalized.Contains("blufor"))
			return true;
		if (normalized.Contains("opfor"))
			return true;
		if (normalized.Contains("military"))
			return true;
		if (normalized.Contains("uniform"))
			return true;
		if (normalized.Contains("helmet"))
			return true;
		if (normalized.Contains("bodyarmor"))
			return true;
		if (normalized.Contains("body_armor"))
			return true;
		if (normalized.Contains("armorvest"))
			return true;
		if (normalized.Contains("armor_vest"))
			return true;
		if (normalized.Contains("platecarrier"))
			return true;
		if (normalized.Contains("plate_carrier"))
			return true;
		if (normalized.Contains("tactical"))
			return true;
		if (normalized.Contains("webbing"))
			return true;
		if (normalized.Contains("chestrig"))
			return true;
		if (normalized.Contains("chest_rig"))
			return true;
		if (normalized.Contains("chest-rig"))
			return true;
		if (normalized.Contains("lbe"))
			return true;
		if (normalized.Contains("alice"))
			return true;
		if (normalized.Contains("loadbearing"))
			return true;
		if (normalized.Contains("load_bearing"))
			return true;
		if (normalized.Contains("camo"))
			return true;
		if (normalized.Contains("camouflage"))
			return true;

		return false;
	}

	protected bool IsUndercoverClothingLikeIdentity(string normalized)
	{
		if (normalized.Contains("clothing"))
			return true;
		if (normalized.Contains("headgear"))
			return true;
		if (normalized.Contains("helmet"))
			return true;
		if (normalized.Contains("hat"))
			return true;
		if (normalized.Contains("vest"))
			return true;
		if (normalized.Contains("webbing"))
			return true;
		if (normalized.Contains("backpack"))
			return true;
		if (normalized.Contains("pants"))
			return true;
		if (normalized.Contains("trouser"))
			return true;
		if (normalized.Contains("boot"))
			return true;
		if (normalized.Contains("glove"))
			return true;
		if (normalized.Contains("uniform"))
			return true;
		if (normalized.Contains("jacket"))
			return true;
		if (normalized.Contains("shirt"))
			return true;

		return false;
	}

	protected bool IsUndercoverMilitaryRoleIdentity(string normalized)
	{
		if (normalized.Contains("rifleman"))
			return true;
		if (normalized.Contains("autorifleman"))
			return true;
		if (normalized.Contains("machinegun"))
			return true;
		if (normalized.Contains("machine_gun"))
			return true;
		if (normalized.Contains("grenadier"))
			return true;
		if (normalized.Contains("marksman"))
			return true;
		if (normalized.Contains("sniper"))
			return true;
		if (normalized.Contains("squadleader"))
			return true;
		if (normalized.Contains("squad_leader"))
			return true;
		if (normalized.Contains("teamleader"))
			return true;
		if (normalized.Contains("team_leader"))
			return true;
		if (normalized.Contains("officer"))
			return true;
		if (normalized.Contains("_lat"))
			return true;
		if (normalized.Contains("/lat"))
			return true;
		if (normalized.Contains("_mg"))
			return true;
		if (normalized.Contains("/mg"))
			return true;

		return false;
	}

	protected bool IsUndercoverWeaponIdentity(string normalized)
	{
		bool weaponAccessory = normalized.Contains("magazine");
		weaponAccessory = weaponAccessory || normalized.Contains("/attachments/");
		weaponAccessory = weaponAccessory || normalized.Contains("attachment_");
		if (weaponAccessory)
			return false;

		if (normalized.Contains("/weapons/"))
			return true;
		if (normalized.Contains("weapon_"))
			return true;
		if (normalized.Contains("/rifles/"))
			return true;
		if (normalized.Contains("rifle"))
			return true;
		if (normalized.Contains("carbine"))
			return true;
		if (normalized.Contains("machinegun"))
			return true;
		if (normalized.Contains("machine_gun"))
			return true;
		if (normalized.Contains("pistol"))
			return true;
		if (normalized.Contains("handgun"))
			return true;
		if (normalized.Contains("sidearm"))
			return true;
		if (normalized.Contains("launcher"))
			return true;
		if (normalized.Contains("rpg"))
			return true;
		if (normalized.Contains("m72"))
			return true;
		if (normalized.Contains("grenade"))
			return true;
		if (normalized.Contains("mine"))
			return true;
		if (normalized.Contains("explosive"))
			return true;

		return false;
	}

	protected string ResolveVehicleEligibilityReason(HST_CampaignState state, IEntity playerEntity)
	{
		IEntity vehicle = ResolveEntityVehicle(playerEntity);
		if (!vehicle)
			return "OK on foot";

		HST_RuntimeVehicleState runtimeVehicle
			= ResolveRuntimeVehicleRecord(state, vehicle);
		if (runtimeVehicle && !runtimeVehicle.m_sVehicleRuntimeId.IsEmpty())
		{
			string runtimeReason = ResolveRuntimeVehicleUndercoverReason(
				state,
				runtimeVehicle.m_sVehicleRuntimeId);
			if (runtimeReason.Contains("BLOCK"))
				return runtimeReason;

			if (runtimeVehicle.m_bCanProvideUndercover)
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
		if (!m_Preset || m_Preset.m_sResistanceFactionKey.IsEmpty())
			return "BLOCK combat-presence authority unavailable";

		vector position = playerEntity.GetOrigin();
		HST_CombatPresenceResult presence = m_CombatPresence.QueryHostilePresenceNear(
			state,
			m_Preset,
			m_Preset.m_sResistanceFactionKey,
			position,
			180.0);
		if (!presence || !presence.m_bQueryValid)
			return "BLOCK combat-presence query unavailable";
		if (presence.m_bHasLiveContributors)
			return string.Format("WARN verified enemy combat presence nearby | contributors %1", presence.m_iContributorCount);

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

	protected HST_AmbientPopulationBudgetPlan BuildAmbientPopulationBudgetPlan(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		string civilianPresenceOverrideZoneId = "",
		int civilianPresenceOverride = -1,
		int rotationEpochOverride = -1)
	{
		if (!state || !balance || !m_AmbientBudget)
			return null;

		array<ref HST_AmbientPopulationTownDemand> demands = {};
		int timeDensityBasisPoints = ResolveAmbientTimeDensityBasisPoints();
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!IsCivilianProjectionEligible(zone, balance))
				continue;
			int zonePresenceOverride = -1;
			if (zone.m_sZoneId == civilianPresenceOverrideZoneId)
				zonePresenceOverride = civilianPresenceOverride;
			HST_AmbientPopulationTownDemand demand = BuildAmbientTownDemand(
				state,
				balance,
				zone,
				timeDensityBasisPoints,
				zonePresenceOverride);
			if (demand)
				demands.Insert(demand);
		}

		int connectedPlayers = HST_CommandMenuRequestComponent.CountConnectedPlayers();
		int totalActorBudget = m_AmbientBudget.ResolveGlobalBudget(
			balance.m_iCivilianGlobalActorBudgetBase,
			balance.m_iCivilianGlobalActorBudgetPerPlayer,
			connectedPlayers,
			state.m_iWarLevel,
			balance.m_iCivilianWarLevelBudgetPenaltyPercent,
			AMBIENT_GLOBAL_ACTOR_BUDGET_MAXIMUM);
		int trafficActorBudget = m_AmbientBudget.ResolveGlobalBudget(
			balance.m_iCivilianGlobalTrafficBudgetBase,
			balance.m_iCivilianGlobalTrafficBudgetPerPlayer,
			connectedPlayers,
			state.m_iWarLevel,
			balance.m_iCivilianWarLevelBudgetPenaltyPercent,
			AMBIENT_GLOBAL_TRAFFIC_BUDGET_MAXIMUM);
		int planRotationEpoch = m_iAmbientRotationEpoch;
		if (rotationEpochOverride >= 0)
			planRotationEpoch = rotationEpochOverride;
		return m_AmbientBudget.BuildPlan(
			demands,
			totalActorBudget,
			trafficActorBudget,
			planRotationEpoch);
	}

	protected HST_AmbientPopulationTownDemand BuildAmbientTownDemand(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		int timeDensityBasisPoints = -1,
		int civilianPresenceOverride = -1)
	{
		if (!state || !balance || !zone || !IsCivilianLocality(zone))
			return null;

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zone.m_sZoneId);
		if (!civilianZone)
			return null;

		int desiredPedestrians = ResolveCivilianPedestrianTarget(
			civilianZone,
			zone,
			balance,
			state,
			civilianPresenceOverride);
		int desiredTraffic = ResolveCivilianTrafficTarget(
			balance,
			zone,
			civilianZone,
			state);

		int remainingPopulation = civilianZone.m_iPopulationRemaining;
		if (IsTrueTownLocation(zone) && m_TownInfluence)
		{
			HST_TownInfluenceRecord record = m_TownInfluence.FindValidRecord(
				state,
				zone.m_sZoneId);
			if (!record)
				return null;
			remainingPopulation = record.m_iRemainingPopulation;
		}
		remainingPopulation = Math.Max(0, remainingPopulation);
		desiredTraffic = Math.Min(desiredTraffic, remainingPopulation / 3);
		desiredPedestrians = Math.Min(
			desiredPedestrians,
			Math.Max(0, remainingPopulation - desiredTraffic));

		int densityBasisPoints = ResolveAmbientDensityBasisPoints(
			civilianZone,
			timeDensityBasisPoints);
		desiredPedestrians = ScaleAmbientDesiredCount(
			desiredPedestrians,
			densityBasisPoints);
		desiredTraffic = ScaleAmbientDesiredCount(
			desiredTraffic,
			densityBasisPoints);
		// Every projected pedestrian and traffic driver needs a distinct concrete
		// appearance inside one locality. Preserve requested traffic first, then
		// fit pedestrians into the remaining unique GUID-qualified pool capacity.
		int appearanceCapacity
			= CountGuidQualifiedCivilianCharacterPrefabs(balance);
		desiredTraffic = Math.Min(desiredTraffic, appearanceCapacity);
		desiredPedestrians = Math.Min(
			desiredPedestrians,
			Math.Max(0, appearanceCapacity - desiredTraffic));
		if (desiredPedestrians <= 0 && desiredTraffic <= 0)
			return null;

		HST_AmbientPopulationTownDemand demand = new HST_AmbientPopulationTownDemand();
		demand.m_sZoneId = zone.m_sZoneId;
		demand.m_iDesiredPedestrians = desiredPedestrians;
		demand.m_iDesiredTraffic = desiredTraffic;
		return demand;
	}

	protected int ResolveAmbientDensityBasisPoints(
		HST_CivilianZoneState civilianZone,
		int timeBasisPoints = -1)
	{
		if (timeBasisPoints <= 0)
			timeBasisPoints = ResolveAmbientTimeDensityBasisPoints();
		int heat = 0;
		if (civilianZone)
			heat = Math.Max(0, civilianZone.m_iWantedHeat);
		int safetyBasisPoints = Math.Max(
			AMBIENT_MIN_SAFETY_DENSITY_BASIS_POINTS,
			10000 - Math.Min(7500, heat * 250));
		return timeBasisPoints * safetyBasisPoints / 10000;
	}

	protected int ResolveAmbientTimeDensityBasisPoints()
	{
		ChimeraWorld world = GetGame().GetWorld();
		if (!world)
			return AMBIENT_DAY_DENSITY_BASIS_POINTS;
		TimeAndWeatherManagerEntity timeManager = world.GetTimeAndWeatherManager();
		if (!timeManager)
			return AMBIENT_DAY_DENSITY_BASIS_POINTS;

		int hour;
		int minute;
		int second;
		timeManager.GetHoursMinutesSeconds(hour, minute, second);
		if (hour >= 7 && hour < 20)
			return AMBIENT_DAY_DENSITY_BASIS_POINTS;
		if ((hour >= 5 && hour < 7) || (hour >= 20 && hour < 22))
			return AMBIENT_TWILIGHT_DENSITY_BASIS_POINTS;
		return AMBIENT_NIGHT_DENSITY_BASIS_POINTS;
	}

	protected int ScaleAmbientDesiredCount(int desired, int densityBasisPoints)
	{
		if (desired <= 0 || densityBasisPoints <= 0)
			return 0;
		return Math.Max(1, (desired * densityBasisPoints + 9999) / 10000);
	}

	protected bool SpawnActiveZoneRuntime(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		int pedestrianTarget,
		int trafficTarget)
	{
		if (!state || !balance || !zone || !IsCivilianLocality(zone))
			return false;

		if (!m_aRuntimeZoneIds.Contains(zone.m_sZoneId))
			m_aRuntimeZoneIds.Insert(zone.m_sZoneId);

		int spawned = SpawnZoneCivilianPedestrians(
			state,
			balance,
			zone,
			Math.Max(0, pedestrianTarget));

		if (IsTrueTownLocation(zone))
		{
			array<vector> occupiedVehiclePositions = {};
			spawned += MaintainTownStaticVehiclePopulation(
				state,
				preset,
				balance,
				zone,
				occupiedVehiclePositions);

			spawned += SpawnZoneCivilianTraffic(
				state,
				balance,
				zone,
				occupiedVehiclePositions,
				occupiedVehiclePositions.Count(),
				Math.Max(0, trafficTarget));
		}

		Print(string.Format("Partisan civilians | zone %1 active | spawned %2 runtime civilian/military ambience entity(s)", zone.m_sZoneId, spawned));
		return true;
	}

	protected bool MaintainActiveZonePopulation(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		int pedestrianTarget,
		int trafficTarget)
	{
		if (!state || !balance || !zone || !IsCivilianLocality(zone))
			return false;

		bool changed = RemoveExcessAmbientActors(
			state,
			zone.m_sZoneId,
			"CIV_CHARACTER",
			Math.Max(0, pedestrianTarget));
		changed = RemoveExcessAmbientActors(
			state,
			zone.m_sZoneId,
			CIVILIAN_TRAFFIC_RUNTIME_KIND,
			Math.Max(0, trafficTarget)) || changed;

		if (SpawnZoneCivilianPedestrians(
			state,
			balance,
			zone,
			Math.Max(0, pedestrianTarget)) > 0)
			changed = true;

		if (!IsTrueTownLocation(zone))
			return changed;

		array<vector> occupiedVehiclePositions = {};
		BuildOccupiedTownVehiclePositions(zone.m_sZoneId, occupiedVehiclePositions);
		if (MaintainTownStaticVehiclePopulation(
			state,
			preset,
			balance,
			zone,
			occupiedVehiclePositions) > 0)
			changed = true;
		if (SpawnZoneCivilianTraffic(
			state,
			balance,
			zone,
			occupiedVehiclePositions,
			occupiedVehiclePositions.Count(),
			Math.Max(0, trafficTarget)) > 0)
			changed = true;
		return changed;
	}

	protected int MaintainTownStaticVehiclePopulation(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		array<vector> occupiedVehiclePositions)
	{
		if (!state || !balance || !zone || !IsTrueTownLocation(zone)
			|| !occupiedVehiclePositions)
			return 0;

		int spawned;
		int civilianTarget = ResolveDeterministicCount(
			balance.m_iCivilianVehicleMinPerTown,
			balance.m_iCivilianVehicleMaxPerTown,
			BuildZoneSeed(state, zone, 101));
		int initializationIndex = EnsureStaticVehicleInitializationIndex(
			zone.m_sZoneId);
		int civilianCompleted = m_aStaticCivilianVehicleSlotsCompleted[initializationIndex];
		for (int civilianIndex = civilianCompleted; civilianIndex < civilianTarget; civilianIndex++)
		{
			if (!CanStartAmbientSpawn(state, zone.m_sZoneId, "CIV_VEHICLE"))
				break;
			int civilianSeed = BuildZoneSeed(state, zone, 200 + civilianIndex);
			string civilianPrefab = SelectCivilianVehiclePrefab(
				balance,
				civilianIndex,
				civilianSeed);
			if (civilianPrefab.IsEmpty())
			{
				SetAmbientSpawnRetry(state, balance, zone.m_sZoneId, "CIV_VEHICLE");
				break;
			}
			m_iAmbientSpawnBudgetRemaining--;
			if (!SpawnTownVehicleRuntimeEntity(
				state,
				zone,
				zone.m_sZoneId,
				civilianPrefab,
				ResolveTownVehicleSpawnPosition(zone, civilianIndex, false),
				BuildSpawnAngles(civilianSeed),
				CIVILIAN_FACTION_KEY,
				"CIV_VEHICLE",
				occupiedVehiclePositions,
				civilianIndex,
				false))
			{
				SetAmbientSpawnRetry(state, balance, zone.m_sZoneId, "CIV_VEHICLE");
				break;
			}
			ClearAmbientSpawnRetry(zone.m_sZoneId, "CIV_VEHICLE");
			m_aStaticCivilianVehicleSlotsCompleted[initializationIndex]
				= m_aStaticCivilianVehicleSlotsCompleted[initializationIndex] + 1;
			spawned++;
		}

		bool projectMilitaryVehicles = ShouldSpawnFactionTownVehicles(preset, zone);
		string militaryPolicyOwner = "<none>";
		if (projectMilitaryVehicles)
			militaryPolicyOwner = zone.m_sOwnerFactionKey;
		if (initializationIndex < m_aStaticMilitaryInitializationOwnerKeys.Count()
			&& m_aStaticMilitaryInitializationOwnerKeys[initializationIndex]
				!= militaryPolicyOwner)
		{
			RecycleStaticMilitaryVehiclesForPolicyChange(state, zone.m_sZoneId);
			m_aStaticMilitaryVehicleSlotsCompleted[initializationIndex] = 0;
			m_aStaticMilitaryInitializationOwnerKeys[initializationIndex]
				= militaryPolicyOwner;
			ClearAmbientSpawnRetry(zone.m_sZoneId, "MILITARY_VEHICLE");
			BuildOccupiedTownVehiclePositions(
				zone.m_sZoneId,
				occupiedVehiclePositions);
		}
		if (!projectMilitaryVehicles)
			return spawned;
		int militaryTarget = ResolveDeterministicCount(
			balance.m_iOccupierVehicleMinPerTown,
			balance.m_iOccupierVehicleMaxPerTown,
			BuildZoneSeed(state, zone, 401));
		int militaryCompleted = m_aStaticMilitaryVehicleSlotsCompleted[initializationIndex];
		for (int militaryIndex = militaryCompleted; militaryIndex < militaryTarget; militaryIndex++)
		{
			if (!CanStartAmbientSpawn(state, zone.m_sZoneId, "MILITARY_VEHICLE"))
				break;
			string ownerPrefab = SelectFactionVehiclePrefab(
				zone.m_sOwnerFactionKey,
				militaryIndex + BuildZoneSeed(state, zone, 503));
			if (ownerPrefab.IsEmpty())
			{
				SetAmbientSpawnRetry(state, balance, zone.m_sZoneId, "MILITARY_VEHICLE");
				break;
			}
			int vehicleSlot = civilianTarget + militaryIndex;
			m_iAmbientSpawnBudgetRemaining--;
			if (!SpawnTownVehicleRuntimeEntity(
				state,
				zone,
				zone.m_sZoneId,
				ownerPrefab,
				ResolveTownVehicleSpawnPosition(zone, vehicleSlot, true),
				BuildSpawnAngles(BuildZoneSeed(state, zone, 700 + militaryIndex)),
				zone.m_sOwnerFactionKey,
				"MILITARY_VEHICLE",
				occupiedVehiclePositions,
				vehicleSlot,
				true))
			{
				SetAmbientSpawnRetry(state, balance, zone.m_sZoneId, "MILITARY_VEHICLE");
				break;
			}
			ClearAmbientSpawnRetry(zone.m_sZoneId, "MILITARY_VEHICLE");
			m_aStaticMilitaryVehicleSlotsCompleted[initializationIndex]
				= m_aStaticMilitaryVehicleSlotsCompleted[initializationIndex] + 1;
			spawned++;
		}
		return spawned;
	}

	protected void RecycleStaticMilitaryVehiclesForPolicyChange(
		HST_CampaignState state,
		string zoneId)
	{
		if (!state || zoneId.IsEmpty())
			return;
		for (int index = m_aRuntimeEntities.Count() - 1; index >= 0; index--)
		{
			if (index >= m_aRuntimeEntityZoneIds.Count()
				|| index >= m_aRuntimeEntityKinds.Count()
				|| m_aRuntimeEntityZoneIds[index] != zoneId
				|| m_aRuntimeEntityKinds[index] != "MILITARY_VEHICLE")
				continue;
			IEntity entity = m_aRuntimeEntities[index];
			if (entity && HasPlayerOccupant(entity))
			{
				if (!PromoteRuntimeVehicleToPersistentField(state, entity))
					continue;
			}
			else if (entity)
			{
				RemoveTransientAmbientRuntimeVehicleRecord(state, entity);
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			}
			else if (index < m_aRuntimeEntityVehicleIds.Count())
			{
				RemoveTransientAmbientRuntimeVehicleRecordById(
					state,
					m_aRuntimeEntityVehicleIds[index]);
			}
			RemoveRuntimeEntityAt(index);
		}
	}

	protected int EnsureStaticVehicleInitializationIndex(string zoneId)
	{
		int index = m_aStaticVehicleInitializationZoneIds.Find(zoneId);
		if (index >= 0)
			return index;
		m_aStaticVehicleInitializationZoneIds.Insert(zoneId);
		m_aStaticCivilianVehicleSlotsCompleted.Insert(0);
		m_aStaticMilitaryVehicleSlotsCompleted.Insert(0);
		m_aStaticMilitaryInitializationOwnerKeys.Insert("");
		return m_aStaticVehicleInitializationZoneIds.Count() - 1;
	}

	protected void ClearStaticVehicleInitialization(string zoneId)
	{
		int index = m_aStaticVehicleInitializationZoneIds.Find(zoneId);
		if (index < 0)
			return;
		m_aStaticVehicleInitializationZoneIds.Remove(index);
		if (index < m_aStaticCivilianVehicleSlotsCompleted.Count())
			m_aStaticCivilianVehicleSlotsCompleted.Remove(index);
		if (index < m_aStaticMilitaryVehicleSlotsCompleted.Count())
			m_aStaticMilitaryVehicleSlotsCompleted.Remove(index);
		if (index < m_aStaticMilitaryInitializationOwnerKeys.Count())
			m_aStaticMilitaryInitializationOwnerKeys.Remove(index);
	}

	protected int SpawnZoneCivilianPedestrians(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		int targetPedestrians)
	{
		if (!state || !balance || !zone || !IsCivilianLocality(zone))
			return 0;

		int currentPedestrians = CountAmbientActorReservationsForZone(
			zone.m_sZoneId,
			"CIV_CHARACTER");
		if (currentPedestrians >= targetPedestrians)
			return 0;
		if (CountGuidQualifiedCivilianCharacterPrefabs(balance) < MIN_CIVILIAN_CHARACTER_PREFABS)
		{
			WarnMissingCivilianCharacterPool(zone, targetPedestrians);
			return 0;
		}

		int spawned;
		int appearanceSeed = BuildZoneSeed(state, zone, CIVILIAN_APPEARANCE_SEED_SALT);
		array<string> usedAppearancePrefabs = {};
		BuildUsedCivilianActorAppearancePrefabs(
			zone.m_sZoneId,
			usedAppearancePrefabs);
		for (int i; i < targetPedestrians; i++)
		{
			if (IsAmbientProjectionSlotReserved(
				zone.m_sZoneId,
				"CIV_CHARACTER",
				i))
				continue;
			if (!CanStartAmbientSpawn(state, zone.m_sZoneId, "CIV_CHARACTER"))
				break;

			int civilianSeed = BuildZoneSeed(state, zone, 1000 + i);
			string civilianPrefab = SelectCivilianCharacterPrefabAvoiding(
				balance,
				i,
				appearanceSeed,
				usedAppearancePrefabs);
			if (civilianPrefab.IsEmpty())
			{
				SetAmbientSpawnRetry(state, balance, zone.m_sZoneId, "CIV_CHARACTER");
				break;
			}

			vector position = ResolveTownCharacterSpawnPosition(zone, i, civilianSeed);
			vector angles = BuildSpawnAngles(civilianSeed);
			int beforeCivilianSpawn = m_aRuntimeEntities.Count();
			m_iAmbientSpawnBudgetRemaining--;
			if (!SpawnRuntimeEntity(
				state,
				zone.m_sZoneId,
				civilianPrefab,
				position,
				angles,
				CIVILIAN_FACTION_KEY,
				"CIV_CHARACTER"))
			{
				SetAmbientSpawnRetry(state, balance, zone.m_sZoneId, "CIV_CHARACTER");
				break;
			}

			if (beforeCivilianSpawn >= m_aRuntimeEntities.Count())
				break;
			IEntity civilianEntity = m_aRuntimeEntities[beforeCivilianSpawn];
			if (!usedAppearancePrefabs.Contains(civilianPrefab))
				usedAppearancePrefabs.Insert(civilianPrefab);
			AIGroup group;
			if (!AssignCivilianPedestrianBehavior(civilianEntity, zone, i, civilianSeed, group)
				|| !RegisterAmbientPedestrianRuntime(state, zone, civilianEntity, group, i, civilianSeed))
			{
				CleanupFailedAmbientRuntimeEntity(state, civilianEntity, zone.m_sZoneId, "CIV_CHARACTER");
				SetAmbientSpawnRetry(state, balance, zone.m_sZoneId, "CIV_CHARACTER");
				break;
			}

			spawned++;
		}
		return spawned;
	}

	protected int SpawnZoneCivilianTraffic(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		array<vector> occupiedVehiclePositions,
		int baseSlotIndex,
		int targetTraffic)
	{
		if (!state || !balance || !zone || !IsTrueTownLocation(zone))
			return 0;

		int currentTraffic = CountAmbientActorReservationsForZone(
			zone.m_sZoneId,
			CIVILIAN_TRAFFIC_RUNTIME_KIND);
		if (currentTraffic >= targetTraffic)
			return 0;

		int spawned;
		for (int i; i < targetTraffic; i++)
		{
			if (IsAmbientProjectionSlotReserved(
				zone.m_sZoneId,
				CIVILIAN_TRAFFIC_RUNTIME_KIND,
				i))
				continue;
			if (!CanStartAmbientSpawn(state, zone.m_sZoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND))
				break;

			int seed = BuildZoneSeed(state, zone, 3000 + i * 17);
			string vehiclePrefab = SelectCivilianVehiclePrefab(balance, i + baseSlotIndex, seed);
			if (vehiclePrefab.IsEmpty())
			{
				SetAmbientSpawnRetry(state, balance, zone.m_sZoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND);
				break;
			}

			int vehicleIndex = baseSlotIndex + i;
			vector vehiclePosition = ResolveTownVehicleSpawnPosition(zone, vehicleIndex, false);
			vector vehicleAngles = BuildSpawnAngles(seed);
			int beforeVehicleSpawn = m_aRuntimeEntities.Count();
			m_iAmbientSpawnBudgetRemaining--;
			if (!SpawnTownVehicleRuntimeEntity(state, zone, zone.m_sZoneId, vehiclePrefab, vehiclePosition, vehicleAngles, "CIV", CIVILIAN_TRAFFIC_RUNTIME_KIND, occupiedVehiclePositions, vehicleIndex, false))
			{
				SetAmbientSpawnRetry(state, balance, zone.m_sZoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND);
				break;
			}

			if (beforeVehicleSpawn >= m_aRuntimeEntities.Count())
				continue;

			IEntity trafficVehicle = m_aRuntimeEntities[beforeVehicleSpawn];
			IEntity driverEntity;
			AIGroup group;
			if (!AssignCivilianTrafficBehavior(
				state,
				balance,
				zone,
				trafficVehicle,
				i,
				seed,
				driverEntity,
				group)
				|| !RegisterAmbientTrafficRuntime(
					state,
					zone,
					trafficVehicle,
					driverEntity,
					group,
					i,
					seed))
			{
				CleanupFailedAmbientRuntimeEntity(state, trafficVehicle, zone.m_sZoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND);
				SetAmbientSpawnRetry(state, balance, zone.m_sZoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND);
				break;
			}

			spawned++;
		}

		return spawned;
	}

	protected bool AssignCivilianPedestrianBehavior(
		IEntity civilianEntity,
		HST_ZoneState zone,
		int index,
		int seed,
		out AIGroup group)
	{
		group = null;
		if (!civilianEntity || !zone)
			return false;

		group = EnsureCivilianAIGroup(
			civilianEntity,
			civilianEntity,
			CIVILIAN_FACTION_KEY,
			false);
		if (!group)
			return false;

		ApplyCivilianMovementSpeed(civilianEntity, group, EMovementType.WALK);
		array<vector> waypoints = {};
		waypoints.Insert(ResolveCivilianWanderPoint(zone, index, seed, 0));
		waypoints.Insert(ResolveCivilianWanderPoint(zone, index, seed, 1));
		return AssignCivilianCycleWaypoints(
			civilianEntity,
			group,
			waypoints,
			CIVILIAN_WANDER_COMPLETION_RADIUS_METERS,
			false) >= 2;
	}

	protected bool AssignCivilianTrafficBehavior(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		IEntity vehicleEntity,
		int index,
		int seed,
		out IEntity driverEntity,
		out AIGroup group)
	{
		driverEntity = null;
		group = null;
		if (!state || !balance || !zone || !vehicleEntity)
			return false;

		driverEntity = SpawnCivilianTrafficDriver(state, balance, zone, vehicleEntity, index, seed);
		if (!driverEntity)
			return false;

		group = EnsureCivilianAIGroup(vehicleEntity, driverEntity, CIVILIAN_FACTION_KEY);
		if (!group)
			return false;

		if (!TryRegisterCivilianVehicleWithGroup(group, vehicleEntity))
		{
			Print(string.Format("Partisan civilians | ambient traffic vehicle registration failed for %1", zone.m_sZoneId), LogLevel.WARNING);
			return false;
		}
		string seatingReason;
		if (!TryMoveCivilianDriverIntoVehicle(driverEntity, vehicleEntity, seatingReason))
		{
			Print(string.Format("Partisan civilians | ambient traffic driver seating failed for %1: %2", zone.m_sZoneId, seatingReason), LogLevel.WARNING);
			return false;
		}
		ClearAmbientTrafficDriverHornInput(driverEntity);

		array<vector> routePositions = {};
		BuildCivilianTrafficRoute(zone, vehicleEntity.GetOrigin(), index, seed, routePositions);
		if (AssignCivilianCycleWaypoints(vehicleEntity, group, routePositions, CIVILIAN_TRAFFIC_WAYPOINT_RADIUS_METERS, true) < 2)
		{
			Print(string.Format("Partisan civilians | ambient traffic route assignment failed for %1", zone.m_sZoneId), LogLevel.WARNING);
			return false;
		}

		return true;
	}

	protected bool RegisterAmbientPedestrianRuntime(
		HST_CampaignState state,
		HST_ZoneState zone,
		IEntity pedestrianEntity,
		AIGroup group,
		int index,
		int seed)
	{
		if (!state || !zone || !pedestrianEntity || !group || !m_AmbientRuntime)
			return false;

		HST_AmbientActorRuntimeRecord record = m_AmbientRuntime.CreateRecord(
			BuildAmbientRuntimeId(zone.m_sZoneId, "ped", state.m_iElapsedSeconds),
			zone.m_sZoneId,
			HST_AmbientActorRuntimeService.KIND_PEDESTRIAN,
			state.m_iElapsedSeconds,
			index,
			seed);
		if (!record)
			return false;
		record.m_RootEntity = pedestrianEntity;
		record.m_Group = group;
		if (!m_AmbientRuntime.TryTransition(
			record,
			HST_AmbientActorRuntimeService.STATE_BEHAVIOR_INITIALIZING,
			"group and wander order created",
			state.m_iElapsedSeconds))
			return false;

		m_aAmbientActorRecords.Insert(record);
		ClearAmbientSpawnRetry(zone.m_sZoneId, "CIV_CHARACTER");
		return true;
	}

	protected bool RegisterAmbientTrafficRuntime(
		HST_CampaignState state,
		HST_ZoneState zone,
		IEntity vehicleEntity,
		IEntity driverEntity,
		AIGroup group,
		int index,
		int seed)
	{
		if (!state || !zone || !vehicleEntity || !driverEntity || !group || !m_AmbientRuntime)
			return false;

		HST_AmbientActorRuntimeRecord record = m_AmbientRuntime.CreateRecord(
			BuildAmbientRuntimeId(zone.m_sZoneId, "traffic", state.m_iElapsedSeconds),
			zone.m_sZoneId,
			HST_AmbientActorRuntimeService.KIND_TRAFFIC,
			state.m_iElapsedSeconds,
			index,
			seed);
		if (!record)
			return false;
		record.m_RootEntity = vehicleEntity;
		record.m_DriverEntity = driverEntity;
		record.m_Group = group;
		if (!m_AmbientRuntime.TryTransition(record, HST_AmbientActorRuntimeService.STATE_VEHICLE_SPAWNED, "vehicle created", state.m_iElapsedSeconds)
			|| !m_AmbientRuntime.TryTransition(record, HST_AmbientActorRuntimeService.STATE_DRIVER_SPAWNED, "driver and CIV group created", state.m_iElapsedSeconds)
			|| !m_AmbientRuntime.TryTransition(record, HST_AmbientActorRuntimeService.STATE_SEATING, "pilot seat request accepted", state.m_iElapsedSeconds))
			return false;

		m_aAmbientActorRecords.Insert(record);
		ClearAmbientSpawnRetry(zone.m_sZoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND);
		return true;
	}

	protected string BuildAmbientRuntimeId(string zoneId, string kind, int elapsedSeconds)
	{
		m_iAmbientRuntimeSequence++;
		return string.Format("ambient_%1_%2_%3_%4", zoneId, kind, elapsedSeconds, m_iAmbientRuntimeSequence);
	}

	protected int CountAmbientActorReservationsForZone(string zoneId, string runtimeKind)
	{
		if (zoneId.IsEmpty() || !m_AmbientRuntime)
			return 0;
		string kindId = ResolveAmbientKindId(runtimeKind);
		if (kindId.IsEmpty())
			return 0;

		int count;
		foreach (HST_AmbientActorRuntimeRecord record : m_aAmbientActorRecords)
		{
			if (record
				&& record.m_sZoneId == zoneId
				&& record.m_sKindId == kindId
				&& m_AmbientRuntime.IsBudgetReservation(record))
				count++;
		}
		return count;
	}

	protected bool IsAmbientProjectionSlotReserved(
		string zoneId,
		string runtimeKind,
		int projectionSlot)
	{
		if (zoneId.IsEmpty() || projectionSlot < 0 || !m_AmbientRuntime)
			return false;
		string kindId = ResolveAmbientKindId(runtimeKind);
		if (kindId.IsEmpty())
			return false;

		foreach (HST_AmbientActorRuntimeRecord record : m_aAmbientActorRecords)
		{
			if (record
				&& record.m_sZoneId == zoneId
				&& record.m_sKindId == kindId
				&& record.m_iProjectionSlot == projectionSlot
				&& m_AmbientRuntime.IsBudgetReservation(record))
				return true;
		}
		return false;
	}

	int CountAmbientBehaviorReadyActorsForZone(string zoneId, string runtimeKind = "")
	{
		if (zoneId.IsEmpty() || !m_AmbientRuntime)
			return 0;
		string kindId = ResolveAmbientKindId(runtimeKind);
		int count;
		foreach (HST_AmbientActorRuntimeRecord record : m_aAmbientActorRecords)
		{
			if (!record || record.m_sZoneId != zoneId)
				continue;
			if (!kindId.IsEmpty() && record.m_sKindId != kindId)
				continue;
			if (m_AmbientRuntime.IsBehaviorReady(record))
				count++;
		}
		return count;
	}

	int CountAmbientAdmittedActorsForZone(string zoneId, string runtimeKind = "")
	{
		if (zoneId.IsEmpty() || !m_AmbientRuntime)
			return 0;
		string kindId = ResolveAmbientKindId(runtimeKind);
		int count;
		foreach (HST_AmbientActorRuntimeRecord record : m_aAmbientActorRecords)
		{
			if (!record || record.m_sZoneId != zoneId || !record.m_bAdmitted)
				continue;
			if (!kindId.IsEmpty() && record.m_sKindId != kindId)
				continue;
			if (m_AmbientRuntime.IsBudgetReservation(record))
				count++;
		}
		return count;
	}

	int CountAmbientRecoveringActorsForZone(string zoneId, string runtimeKind = "")
	{
		if (zoneId.IsEmpty())
			return 0;
		string kindId = ResolveAmbientKindId(runtimeKind);
		int count;
		foreach (HST_AmbientActorRuntimeRecord record : m_aAmbientActorRecords)
		{
			if (!record || record.m_sZoneId != zoneId
				|| record.m_sStateId != HST_AmbientActorRuntimeService.STATE_RECOVERING)
				continue;
			if (kindId.IsEmpty() || record.m_sKindId == kindId)
				count++;
		}
		return count;
	}

	protected string ResolveAmbientKindId(string runtimeKind)
	{
		if (runtimeKind == "CIV_CHARACTER")
			return HST_AmbientActorRuntimeService.KIND_PEDESTRIAN;
		if (runtimeKind == CIVILIAN_TRAFFIC_RUNTIME_KIND)
			return HST_AmbientActorRuntimeService.KIND_TRAFFIC;
		return "";
	}

	protected bool CanStartAmbientSpawn(
		HST_CampaignState state,
		string zoneId,
		string runtimeKind)
	{
		if (!state || m_iAmbientSpawnBudgetRemaining <= 0)
			return false;
		int retryIndex = FindAmbientSpawnRetry(zoneId, runtimeKind);
		return retryIndex < 0
			|| retryIndex >= m_aAmbientRetrySeconds.Count()
			|| state.m_iElapsedSeconds >= m_aAmbientRetrySeconds[retryIndex];
	}

	protected void SetAmbientSpawnRetry(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		string zoneId,
		string runtimeKind)
	{
		if (!state || !balance || zoneId.IsEmpty() || runtimeKind.IsEmpty())
			return;
		int retrySecond = state.m_iElapsedSeconds
			+ Math.Max(
				balance.m_iCivilianRuntimeHealthIntervalSeconds,
				balance.m_iCivilianRuntimeRetryBackoffSeconds);
		int retryIndex = FindAmbientSpawnRetry(zoneId, runtimeKind);
		if (retryIndex >= 0)
		{
			m_aAmbientRetrySeconds[retryIndex] = retrySecond;
			return;
		}
		m_aAmbientRetryZoneIds.Insert(zoneId);
		m_aAmbientRetryKinds.Insert(runtimeKind);
		m_aAmbientRetrySeconds.Insert(retrySecond);
	}

	protected void ClearAmbientSpawnRetry(string zoneId, string runtimeKind)
	{
		int retryIndex = FindAmbientSpawnRetry(zoneId, runtimeKind);
		if (retryIndex < 0)
			return;
		m_aAmbientRetryZoneIds.Remove(retryIndex);
		m_aAmbientRetryKinds.Remove(retryIndex);
		m_aAmbientRetrySeconds.Remove(retryIndex);
	}

	protected int FindAmbientSpawnRetry(string zoneId, string runtimeKind)
	{
		for (int index; index < m_aAmbientRetryZoneIds.Count(); index++)
		{
			if (index >= m_aAmbientRetryKinds.Count())
				return -1;
			if (m_aAmbientRetryZoneIds[index] == zoneId
				&& m_aAmbientRetryKinds[index] == runtimeKind)
				return index;
		}
		return -1;
	}

	protected bool RemoveExcessAmbientActors(
		HST_CampaignState state,
		string zoneId,
		string runtimeKind,
		int targetCount)
	{
		string kindId = ResolveAmbientKindId(runtimeKind);
		int currentCount = CountAmbientActorReservationsForZone(zoneId, runtimeKind);
		bool changed;
		for (int index = m_aAmbientActorRecords.Count() - 1; index >= 0 && currentCount > targetCount; index--)
		{
			HST_AmbientActorRuntimeRecord record = m_aAmbientActorRecords[index];
			if (!record || record.m_sZoneId != zoneId || record.m_sKindId != kindId)
				continue;
			if (RecycleAmbientActorAt(
				state,
				index,
				"budget allocation reduced"))
			{
				currentCount--;
				changed = true;
			}
		}
		return changed;
	}

	protected bool TickAmbientActorRuntime(
		HST_CampaignState state,
		HST_BalanceConfig balance)
	{
		if (!state || !balance || !m_AmbientRuntime)
			return false;

		bool changed;
		for (int index = m_aAmbientActorRecords.Count() - 1; index >= 0; index--)
		{
			HST_AmbientActorRuntimeRecord record = m_aAmbientActorRecords[index];
			if (record && record.m_bCasualtyAdmissionPending
				&& !TryAdmitRetainedCasualtyObservation(state, record))
				continue;
			if (!record || !record.m_RootEntity)
			{
				m_aAmbientActorRecords.Remove(index);
				changed = true;
				continue;
			}
			if (record.m_sStateId == HST_AmbientActorRuntimeService.STATE_DESPAWN_QUEUED)
			{
				changed = RecycleAmbientActorAt(
					state,
					index,
					record.m_sReasonId) || changed;
				continue;
			}

			if (record.m_sKindId == HST_AmbientActorRuntimeService.KIND_PEDESTRIAN)
				changed = TickAmbientPedestrianRecord(state, balance, index, record) || changed;
			else if (record.m_sKindId == HST_AmbientActorRuntimeService.KIND_TRAFFIC)
				changed = TickAmbientTrafficRecord(state, balance, index, record) || changed;
			else
			{
				m_AmbientRuntime.QueueDespawn(record, "unknown ambient actor kind", state.m_iElapsedSeconds);
				RecycleAmbientActorAt(state, index, "unknown ambient actor kind");
				changed = true;
			}
		}
		return changed;
	}

	protected bool TickAmbientPedestrianRecord(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		int recordIndex,
		HST_AmbientActorRuntimeRecord record)
	{
		if (!state || !balance || !record)
			return false;
		int nowSecond = state.m_iElapsedSeconds;
		if (record.m_bCasualtyAdmissionPending
			&& !TryAdmitRetainedCasualtyObservation(state, record))
			return false;
		if (!IsLivingAmbientCharacter(record.m_RootEntity)
			|| !IsExactCivilianFaction(record.m_RootEntity)
			|| !IsExactCivilianGroupMembership(record.m_RootEntity, record.m_Group))
		{
			QueueAmbientCivilianCasualtyFallback(
				state,
				record,
				record.m_RootEntity);
			SetAmbientSpawnRetry(state, balance, record.m_sZoneId, "CIV_CHARACTER");
			m_AmbientRuntime.QueueDespawn(record, "pedestrian authority lost", nowSecond);
			RecycleAmbientActorAt(state, recordIndex, "pedestrian authority lost");
			return true;
		}
		HST_ZoneState panicZone = state.FindZone(record.m_sZoneId);
		bool nativeThreat = IsAmbientPedestrianThreatened(record);
		int panicDurationSeconds;
		if (panicZone
			&& panicZone.m_iCivilianConsequenceContractVersion
				== HST_CivilianConsequenceService.CONTRACT_VERSION
			&& panicZone.m_iCivilianPanicUntilSecond > nowSecond)
		{
			panicDurationSeconds
				= panicZone.m_iCivilianPanicUntilSecond - nowSecond;
		}
		if (nativeThreat)
			panicDurationSeconds = Math.Max(
				panicDurationSeconds,
				CIVILIAN_LOCAL_THREAT_PANIC_SECONDS);
		bool shouldPanic = panicDurationSeconds > 0;

		if (record.m_sStateId == HST_AmbientActorRuntimeService.STATE_BEHAVIOR_INITIALIZING)
		{
			if (HasActiveCivilianWaypoint(record.m_Group))
			{
				bool admitted = m_AmbientRuntime.TryTransition(
					record,
					HST_AmbientActorRuntimeService.STATE_WANDERING,
					"wander behavior acknowledged",
					nowSecond);
				if (admitted)
					m_AmbientRuntime.RecordMovementProgress(record, record.m_RootEntity.GetOrigin(), nowSecond, AMBIENT_PROGRESS_DISTANCE_METERS);
				if (admitted && shouldPanic)
				{
					return BeginOrMaintainAmbientPedestrianPanic(
						state,
						record,
						panicZone,
						panicDurationSeconds,
						balance.m_iCivilianRuntimeMaxRecoveryAttempts,
						"danger active at pedestrian admission") || admitted;
				}
				return admitted;
			}

			if (nowSecond - record.m_iCreatedAtSecond < balance.m_iCivilianRuntimeStartupGraceSeconds)
				return false;
			SetAmbientSpawnRetry(state, balance, record.m_sZoneId, "CIV_CHARACTER");
			m_AmbientRuntime.QueueDespawn(record, "pedestrian behavior acknowledgement timed out", nowSecond);
			RecycleAmbientActorAt(state, recordIndex, "pedestrian behavior acknowledgement timed out");
			return true;
		}

		if (shouldPanic
			&& (record.m_sStateId
					== HST_AmbientActorRuntimeService.STATE_WANDERING
				|| record.m_sStateId
					== HST_AmbientActorRuntimeService.STATE_PANICKED
				|| record.m_sStateId
					== HST_AmbientActorRuntimeService.STATE_RECOVERING))
		{
			string panicReason = "locality combat danger";
			if (nativeThreat)
				panicReason = "native civilian threat";
			bool panicChanged = BeginOrMaintainAmbientPedestrianPanic(
				state,
				record,
				panicZone,
				panicDurationSeconds,
				balance.m_iCivilianRuntimeMaxRecoveryAttempts,
				panicReason);
			if (record.m_sStateId
				== HST_AmbientActorRuntimeService.STATE_PANICKED)
			{
				bool panicMovementChanged = TickPanickedPedestrianMovement(
					state,
					balance,
					record,
					panicZone);
				return panicChanged || panicMovementChanged;
			}
			return panicChanged;
		}

		if (record.m_sStateId == HST_AmbientActorRuntimeService.STATE_PANICKED)
		{
			if (m_AmbientRuntime.ShouldBeginPanicRecovery(record, nowSecond))
			{
				if (!m_AmbientRuntime.BeginPanicRecovery(
					record,
					nowSecond,
					balance.m_iCivilianRuntimeRetryBackoffSeconds))
					return false;
				if (TryRecoverAmbientPedestrian(state, record))
				{
					if (HasActiveCivilianWaypoint(record.m_Group))
					{
						return m_AmbientRuntime.TryTransition(
							record,
							HST_AmbientActorRuntimeService.STATE_WANDERING,
							"panic recovery waypoint acknowledged",
							nowSecond);
					}
					record.m_sReasonId
						= AMBIENT_PEDESTRIAN_RECOVERY_ACK_REASON;
					record.m_iRetryAtSecond = nowSecond
						+ balance.m_iCivilianRuntimeHealthIntervalSeconds;
				}
				return true;
			}
			return TickPanickedPedestrianMovement(
				state,
				balance,
				record,
				panicZone);
		}

		if (record.m_sStateId == HST_AmbientActorRuntimeService.STATE_RECOVERING)
		{
			if (record.m_sReasonId == AMBIENT_PEDESTRIAN_RECOVERY_ACK_REASON
				&& HasActiveCivilianWaypoint(record.m_Group))
			{
				return m_AmbientRuntime.TryTransition(
					record,
					HST_AmbientActorRuntimeService.STATE_WANDERING,
					"pedestrian recovery waypoint acknowledged",
					nowSecond);
			}
			if (!m_AmbientRuntime.IsRetryDue(record, nowSecond))
				return false;
			if (record.m_sReasonId == AMBIENT_PEDESTRIAN_RECOVERY_ACK_REASON)
			{
				if (nowSecond - record.m_iStateChangedAtSecond
					< balance.m_iCivilianRuntimeStartupGraceSeconds
						+ balance.m_iCivilianRuntimeRetryBackoffSeconds)
				{
					record.m_iRetryAtSecond = nowSecond
						+ balance.m_iCivilianRuntimeHealthIntervalSeconds;
					return false;
				}
			}
			else if (TryRecoverAmbientPedestrian(state, record))
			{
				if (HasActiveCivilianWaypoint(record.m_Group))
				{
					return m_AmbientRuntime.TryTransition(
						record,
						HST_AmbientActorRuntimeService.STATE_WANDERING,
						"pedestrian recovery waypoint acknowledged",
						nowSecond);
				}
				record.m_sReasonId = AMBIENT_PEDESTRIAN_RECOVERY_ACK_REASON;
				record.m_iRetryAtSecond = nowSecond
					+ balance.m_iCivilianRuntimeHealthIntervalSeconds;
				return false;
			}

			SetAmbientSpawnRetry(state, balance, record.m_sZoneId, "CIV_CHARACTER");
			m_AmbientRuntime.QueueDespawn(record, "pedestrian recovery acknowledgement failed", nowSecond);
			RecycleAmbientActorAt(state, recordIndex, "pedestrian recovery acknowledgement failed");
			return true;
		}

		if (!m_AmbientRuntime.IsBehaviorReady(record))
			return false;
		if (!HasActiveCivilianWaypoint(record.m_Group))
			return BeginAmbientRecoveryOrRecycle(state, balance, recordIndex, record, "pedestrian waypoint lost");

		bool moved = m_AmbientRuntime.RecordMovementProgress(
			record,
			record.m_RootEntity.GetOrigin(),
			nowSecond,
			AMBIENT_PROGRESS_DISTANCE_METERS);
		if (moved)
			return false;
		if (!m_AmbientRuntime.ShouldRecover(
			record,
			nowSecond,
			balance.m_iCivilianRuntimeStartupGraceSeconds,
			balance.m_iCivilianRuntimeStuckSeconds))
			return false;
		return BeginAmbientRecoveryOrRecycle(state, balance, recordIndex, record, "pedestrian movement stalled");
	}

	protected bool IsAmbientPedestrianThreatened(
		HST_AmbientActorRuntimeRecord record)
	{
		if (!record || !record.m_RootEntity)
			return false;
		AIAgent agent = ResolveCivilianAgentReadOnly(record.m_RootEntity);
		if (!agent)
			return false;
		SCR_AIInfoComponent info = SCR_AIInfoComponent.Cast(
			agent.FindComponent(SCR_AIInfoComponent));
		if (!info)
			return false;
		EAIThreatState threatState = info.GetThreatState();
		return threatState == EAIThreatState.ALERTED
			|| threatState == EAIThreatState.THREATENED;
	}

	protected bool BeginOrMaintainAmbientPedestrianPanic(
		HST_CampaignState state,
		HST_AmbientActorRuntimeRecord record,
		HST_ZoneState zone,
		int durationSeconds,
		int maxRouteRecoveryAttempts,
		string reason)
	{
		if (!state || !record || !zone || durationSeconds <= 0)
			return false;
		string previousState = record.m_sStateId;
		int previousDeadline = record.m_iPanicUntilSecond;
		bool waypointChanged;
		if (record.m_sStateId
				!= HST_AmbientActorRuntimeService.STATE_PANICKED
			|| !HasActiveCivilianWaypoint(record.m_Group))
		{
			waypointChanged = AssignAmbientPedestrianPanicWaypoint(
				state,
				record,
				zone);
			if (!waypointChanged)
			{
				return m_AmbientRuntime.RecordPanicRouteRecoveryAttempt(
					record,
					state.m_iElapsedSeconds,
					maxRouteRecoveryAttempts,
					"panic waypoint admission failed");
			}
		}
		if (waypointChanged
			|| previousState != HST_AmbientActorRuntimeService.STATE_PANICKED)
		{
			ApplyCivilianMovementSpeed(
				record.m_RootEntity,
				record.m_Group,
				EMovementType.RUN);
		}
		if (!m_AmbientRuntime.BeginOrExtendPanic(
			record,
			state.m_iElapsedSeconds,
			durationSeconds,
			reason))
			return waypointChanged;
		return waypointChanged
			|| record.m_sStateId != previousState
			|| record.m_iPanicUntilSecond != previousDeadline;
	}

	protected bool TickPanickedPedestrianMovement(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_AmbientActorRuntimeRecord record,
		HST_ZoneState zone)
	{
		if (!state || !balance || !record || !zone)
			return false;
		if (!HasActiveCivilianWaypoint(record.m_Group))
		{
			if (!m_AmbientRuntime.RecordPanicRouteRecoveryAttempt(
				record,
				state.m_iElapsedSeconds,
				balance.m_iCivilianRuntimeMaxRecoveryAttempts,
				"panic waypoint lost"))
				return false;
			if (record.m_sStateId
				== HST_AmbientActorRuntimeService.STATE_DESPAWN_QUEUED)
				return true;
			AssignAmbientPedestrianPanicWaypoint(state, record, zone);
			return true;
		}
		bool moved = m_AmbientRuntime.RecordMovementProgress(
			record,
			record.m_RootEntity.GetOrigin(),
			state.m_iElapsedSeconds,
			AMBIENT_PROGRESS_DISTANCE_METERS);
		if (moved)
			return false;
		int progressSecond = Math.Max(
			record.m_iStateChangedAtSecond,
			record.m_iLastProgressAtSecond);
		if (state.m_iElapsedSeconds - progressSecond
			< Math.Max(1, balance.m_iCivilianRuntimeStuckSeconds))
			return false;
		if (!m_AmbientRuntime.RecordPanicRouteRecoveryAttempt(
			record,
			state.m_iElapsedSeconds,
			balance.m_iCivilianRuntimeMaxRecoveryAttempts,
			"panic movement stalled"))
			return false;
		if (record.m_sStateId
			== HST_AmbientActorRuntimeService.STATE_DESPAWN_QUEUED)
			return true;
		bool rebuilt = AssignAmbientPedestrianPanicWaypoint(
			state,
			record,
			zone);
		if (rebuilt)
			record.m_iLastProgressAtSecond = state.m_iElapsedSeconds;
		return true;
	}

	protected bool AssignAmbientPedestrianPanicWaypoint(
		HST_CampaignState state,
		HST_AmbientActorRuntimeRecord record,
		HST_ZoneState zone)
	{
		if (!state || !record || !record.m_RootEntity || !record.m_Group
			|| !zone)
			return false;
		vector origin = record.m_RootEntity.GetOrigin();
		vector threat = ResolveCivilianPanicThreat(zone);
		float directionX = origin[0] - threat[0];
		float directionZ = origin[2] - threat[2];
		float lengthSquared = directionX * directionX
			+ directionZ * directionZ;
		if (lengthSquared < 1.0)
		{
			float angle = ModInt(
				record.m_iProjectionSeed
					+ record.m_iPanicCount * 137
					+ record.m_iPanicRouteRecoveryCount * 83
					+ zone.m_iCivilianCombatEpisodeCount * 59,
				360) * 0.0174533;
			directionX = Math.Sin(angle);
			directionZ = Math.Cos(angle);
		}
		else
		{
			float length = Math.Sqrt(lengthSquared);
			directionX = directionX / length;
			directionZ = directionZ / length;
		}
		float fleeDistance = CIVILIAN_PANIC_FLEE_MIN_DISTANCE_METERS
			+ ModInt(
				record.m_iProjectionSeed
					+ record.m_iPanicCount * 43
					+ record.m_iPanicRouteRecoveryCount * 17,
				CIVILIAN_PANIC_FLEE_DISTANCE_VARIANCE_METERS);
		vector target = origin;
		target[0] = target[0] + directionX * fleeDistance;
		target[2] = target[2] + directionZ * fleeDistance;
		target = HST_WorldPositionService.ResolveSafeGroundPosition(
			target,
			HST_WorldPositionService.CHARACTER_GROUND_OFFSET,
			true,
			4.0);
		vector admittedTarget;
		if (!TryResolveCivilianPedestrianWaypoint(
			record.m_Group,
			target,
			admittedTarget))
			return false;
		GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(
			CIVILIAN_FLEE_WAYPOINT_PREFAB,
			admittedTarget,
			"0 0 0");
		TrackCampaignDebugCivilianMutationEntity(waypointEntity);
		AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
		if (!waypoint)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
			return false;
		}
		waypoint.SetCompletionRadius(CIVILIAN_PANIC_WAYPOINT_RADIUS_METERS);
		ClearAmbientMovementHelpers(record);
		RegisterRuntimeHelper(record.m_RootEntity, waypointEntity);
		record.m_Group.AddWaypoint(waypoint);
		record.m_Group.ActivateAllMembers();
		ApplyCivilianMovementSpeed(
			record.m_RootEntity,
			record.m_Group,
			EMovementType.RUN);
		return HasAssignedCivilianWaypoint(record.m_Group);
	}

	protected bool TryRecoverAmbientPedestrian(
		HST_CampaignState state,
		HST_AmbientActorRuntimeRecord record)
	{
		if (!state || !record || !record.m_RootEntity || !record.m_Group)
			return false;
		HST_ZoneState zone = state.FindZone(record.m_sZoneId);
		if (!zone)
			return false;

		int projectionSeedSalt = record.m_iProjectionSeed % 997;
		if (projectionSeedSalt < 0)
			projectionSeedSalt = -projectionSeedSalt;
		int seed = BuildZoneSeed(
			state,
			zone,
			6100
				+ Math.Max(0, record.m_iProjectionSlot) * 193
				+ projectionSeedSalt
				+ record.m_iRecoveryCount * 97);
		array<vector> waypoints = {};
		waypoints.Insert(ResolveCivilianWanderPoint(
			zone,
			record.m_iProjectionSlot,
			seed,
			0));
		waypoints.Insert(ResolveCivilianWanderPoint(
			zone,
			record.m_iProjectionSlot,
			seed,
			1));
		bool assigned = AssignCivilianCycleWaypoints(
			record.m_RootEntity,
			record.m_Group,
			waypoints,
			CIVILIAN_WANDER_COMPLETION_RADIUS_METERS,
			false,
			record) >= 2;
		if (!assigned)
			return false;
		ApplyCivilianMovementSpeed(
			record.m_RootEntity,
			record.m_Group,
			EMovementType.WALK);
		return true;
	}

	protected bool BeginAmbientRecoveryOrRecycle(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		int recordIndex,
		HST_AmbientActorRuntimeRecord record,
		string reason)
	{
		if (!state || !balance || !record)
			return false;
		bool changed = m_AmbientRuntime.BeginRecovery(
			record,
			state.m_iElapsedSeconds,
			balance.m_iCivilianRuntimeMaxRecoveryAttempts,
			balance.m_iCivilianRuntimeRetryBackoffSeconds,
			reason);
		if (!changed)
			return false;
		if (record.m_sStateId != HST_AmbientActorRuntimeService.STATE_DESPAWN_QUEUED)
		{
			ClearAmbientMovementHelpers(record);
			return true;
		}

		string runtimeKind = "CIV_CHARACTER";
		if (record.m_sKindId == HST_AmbientActorRuntimeService.KIND_TRAFFIC)
			runtimeKind = CIVILIAN_TRAFFIC_RUNTIME_KIND;
		SetAmbientSpawnRetry(state, balance, record.m_sZoneId, runtimeKind);
		RecycleAmbientActorAt(state, recordIndex, reason + "; recovery exhausted");
		return true;
	}

	protected bool TickAmbientTrafficRecord(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		int recordIndex,
		HST_AmbientActorRuntimeRecord record)
	{
		if (!state || !balance || !record)
			return false;
		int nowSecond = state.m_iElapsedSeconds;
		if (record.m_bCasualtyAdmissionPending
			&& !TryAdmitRetainedCasualtyObservation(state, record))
			return false;
		if (!IsLivingAmbientEntity(record.m_RootEntity)
			|| !IsLivingAmbientCharacter(record.m_DriverEntity)
			|| !IsExactCivilianFaction(record.m_DriverEntity)
			|| !IsExactCivilianGroupMembership(record.m_DriverEntity, record.m_Group))
		{
			QueueAmbientCivilianCasualtyFallback(
				state,
				record,
				record.m_DriverEntity);
			SetAmbientSpawnRetry(state, balance, record.m_sZoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND);
			m_AmbientRuntime.QueueDespawn(record, "traffic authority lost", nowSecond);
			RecycleAmbientActorAt(state, recordIndex, "traffic authority lost");
			return true;
		}

		if (record.m_sStateId == HST_AmbientActorRuntimeService.STATE_RECOVERING)
			return TickAmbientTrafficRecovery(state, balance, recordIndex, record);

		bool changed;
		if (record.m_sStateId == HST_AmbientActorRuntimeService.STATE_SEATING)
		{
			if (!IsExactAmbientTrafficDriverSeated(record))
				return RetryAmbientTrafficStartup(state, balance, recordIndex, record, "waiting for exact pilot occupant");
			changed = m_AmbientRuntime.TryTransition(
				record,
				HST_AmbientActorRuntimeService.STATE_DRIVER_CONFIRMED,
				"living driver confirmed in pilot seat",
				nowSecond) || changed;
		}

		if (record.m_sStateId == HST_AmbientActorRuntimeService.STATE_DRIVER_CONFIRMED)
		{
			changed = m_AmbientRuntime.TryTransition(
				record,
				HST_AmbientActorRuntimeService.STATE_ENGINE_STARTING,
				"driver authority confirmed",
				nowSecond) || changed;
		}

		if (record.m_sStateId == HST_AmbientActorRuntimeService.STATE_ENGINE_STARTING)
		{
			BaseVehicleControllerComponent controller = ResolveAmbientVehicleController(record.m_RootEntity);
			if (controller && !controller.IsEngineOn())
				controller.ForceStartEngine();
			if (!controller || !controller.IsEngineOn())
				return RetryAmbientTrafficStartup(state, balance, recordIndex, record, "waiting for engine-on acknowledgement") || changed;
			changed = m_AmbientRuntime.TryTransition(
				record,
				HST_AmbientActorRuntimeService.STATE_ENGINE_STARTED,
				"engine-on acknowledged",
				nowSecond) || changed;
		}

		if (record.m_sStateId == HST_AmbientActorRuntimeService.STATE_ENGINE_STARTED)
		{
			if (!HasAssignedCivilianWaypoint(record.m_Group))
			{
				TryRebuildAmbientTrafficRoute(state, record);
			}
			if (!HasActiveCivilianWaypoint(record.m_Group))
				return RetryAmbientTrafficStartup(state, balance, recordIndex, record, "waiting for active traffic waypoint") || changed;
			changed = m_AmbientRuntime.TryTransition(
				record,
				HST_AmbientActorRuntimeService.STATE_ROUTE_FOLLOWING,
				"seat engine and route acknowledged",
				nowSecond) || changed;
			record.m_iRecoveryCount = 0;
			m_AmbientRuntime.RecordMovementProgress(
				record,
				record.m_RootEntity.GetOrigin(),
				nowSecond,
				AMBIENT_PROGRESS_DISTANCE_METERS);
			return true;
		}

		if (!m_AmbientRuntime.IsBehaviorReady(record))
			return changed;
		if (!IsExactAmbientTrafficDriverSeated(record))
			return BeginAmbientRecoveryOrRecycle(state, balance, recordIndex, record, "traffic driver left pilot seat") || changed;
		if (!TryRegisterCivilianVehicleWithGroup(record.m_Group, record.m_RootEntity))
			return BeginAmbientRecoveryOrRecycle(state, balance, recordIndex, record, "traffic usable-vehicle registration lost") || changed;
		BaseVehicleControllerComponent liveController = ResolveAmbientVehicleController(record.m_RootEntity);
		if (!liveController || !liveController.IsEngineOn())
			return BeginAmbientRecoveryOrRecycle(state, balance, recordIndex, record, "traffic engine stopped") || changed;
		if (!HasActiveCivilianWaypoint(record.m_Group))
			return BeginAmbientRecoveryOrRecycle(state, balance, recordIndex, record, "traffic route lost") || changed;

		bool moved = m_AmbientRuntime.RecordMovementProgress(
			record,
			record.m_RootEntity.GetOrigin(),
			nowSecond,
			AMBIENT_PROGRESS_DISTANCE_METERS);
		if (moved)
			return changed;
		if (!m_AmbientRuntime.ShouldRecover(
			record,
			nowSecond,
			balance.m_iCivilianRuntimeStartupGraceSeconds,
			balance.m_iCivilianRuntimeStuckSeconds))
			return changed;
		return BeginAmbientRecoveryOrRecycle(state, balance, recordIndex, record, "traffic movement stalled") || changed;
	}

	protected bool RetryAmbientTrafficStartup(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		int recordIndex,
		HST_AmbientActorRuntimeRecord record,
		string reason)
	{
		if (!state || !balance || !record)
			return false;
		int nowSecond = state.m_iElapsedSeconds;
		if (nowSecond - record.m_iCreatedAtSecond < balance.m_iCivilianRuntimeStartupGraceSeconds)
			return false;
		if (record.m_iRetryAtSecond >= 0 && nowSecond < record.m_iRetryAtSecond)
			return false;
		if (record.m_iRecoveryCount >= balance.m_iCivilianRuntimeMaxRecoveryAttempts)
		{
			SetAmbientSpawnRetry(state, balance, record.m_sZoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND);
			m_AmbientRuntime.QueueDespawn(record, reason + "; startup retries exhausted", nowSecond);
			RecycleAmbientActorAt(state, recordIndex, reason + "; startup retries exhausted");
			return true;
		}

		record.m_iRecoveryCount++;
		record.m_iRetryAtSecond = nowSecond + balance.m_iCivilianRuntimeRetryBackoffSeconds;
		record.m_sReasonId = reason;
		TryRepairAmbientTrafficRuntime(state, record);
		return true;
	}

	protected bool TickAmbientTrafficRecovery(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		int recordIndex,
		HST_AmbientActorRuntimeRecord record)
	{
		if (!m_AmbientRuntime.IsRetryDue(record, state.m_iElapsedSeconds))
			return false;
		if (TryRepairAmbientTrafficRuntime(state, record)
			&& IsExactAmbientTrafficDriverSeated(record)
			&& IsAmbientTrafficEngineOn(record)
			&& HasActiveCivilianWaypoint(record.m_Group))
		{
			return m_AmbientRuntime.TryTransition(
				record,
				HST_AmbientActorRuntimeService.STATE_ROUTE_FOLLOWING,
				"traffic recovery acknowledged",
				state.m_iElapsedSeconds);
		}

		if (state.m_iElapsedSeconds - record.m_iStateChangedAtSecond
			< balance.m_iCivilianRuntimeStartupGraceSeconds + balance.m_iCivilianRuntimeRetryBackoffSeconds)
		{
			record.m_iRetryAtSecond = state.m_iElapsedSeconds
				+ balance.m_iCivilianRuntimeHealthIntervalSeconds;
			return true;
		}

		SetAmbientSpawnRetry(state, balance, record.m_sZoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND);
		m_AmbientRuntime.QueueDespawn(record, "traffic recovery acknowledgement timed out", state.m_iElapsedSeconds);
		RecycleAmbientActorAt(state, recordIndex, "traffic recovery acknowledgement timed out");
		return true;
	}

	protected bool TryRepairAmbientTrafficRuntime(
		HST_CampaignState state,
		HST_AmbientActorRuntimeRecord record)
	{
		if (!state || !record || !record.m_RootEntity || !record.m_DriverEntity || !record.m_Group)
			return false;

		if (!IsExactAmbientTrafficDriverSeated(record))
		{
			string seatingReason;
			TryMoveCivilianDriverIntoVehicle(record.m_DriverEntity, record.m_RootEntity, seatingReason);
		}
		ClearAmbientTrafficDriverHornInput(record.m_DriverEntity);
		bool vehicleRegistered = TryRegisterCivilianVehicleWithGroup(
			record.m_Group,
			record.m_RootEntity);

		BaseVehicleControllerComponent controller = ResolveAmbientVehicleController(record.m_RootEntity);
		if (controller && !controller.IsEngineOn())
			controller.ForceStartEngine();
		if (!HasAssignedCivilianWaypoint(record.m_Group))
			TryRebuildAmbientTrafficRoute(state, record);
		return vehicleRegistered
			&& IsExactAmbientTrafficDriverSeated(record)
			&& controller
			&& controller.IsEngineOn()
			&& HasActiveCivilianWaypoint(record.m_Group);
	}

	protected bool TryRebuildAmbientTrafficRoute(
		HST_CampaignState state,
		HST_AmbientActorRuntimeRecord record)
	{
		if (!state || !record || !record.m_RootEntity || !record.m_Group)
			return false;
		HST_ZoneState zone = state.FindZone(record.m_sZoneId);
		if (!zone)
			return false;

		ClearAmbientMovementHelpers(record);
		int projectionSeedSalt = record.m_iProjectionSeed % 991;
		if (projectionSeedSalt < 0)
			projectionSeedSalt = -projectionSeedSalt;
		int seed = BuildZoneSeed(
			state,
			zone,
			7300
				+ Math.Max(0, record.m_iProjectionSlot) * 211
				+ projectionSeedSalt
				+ record.m_iRecoveryCount * 131);
		array<vector> routePositions = {};
		BuildCivilianTrafficRoute(
			zone,
			record.m_RootEntity.GetOrigin(),
			record.m_iProjectionSlot + record.m_iRecoveryCount * 37,
			seed,
			routePositions);
		return AssignCivilianCycleWaypoints(
			record.m_RootEntity,
			record.m_Group,
			routePositions,
			CIVILIAN_TRAFFIC_WAYPOINT_RADIUS_METERS,
			true) >= 2;
	}

	protected bool IsLivingAmbientEntity(IEntity entity)
	{
		if (!entity || !entity.GetWorld())
			return false;
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (character)
		{
			CharacterControllerComponent controller = character.GetCharacterController();
			return controller && controller.GetLifeState() != ECharacterLifeState.DEAD;
		}
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(
			entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected bool IsLivingAmbientCharacter(IEntity entity)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return false;
		CharacterControllerComponent controller = character.GetCharacterController();
		return controller && controller.GetLifeState() != ECharacterLifeState.DEAD;
	}

	protected bool IsExactCivilianFaction(IEntity entity)
	{
		if (!entity)
			return false;
		FactionAffiliationComponent faction = FactionAffiliationComponent.Cast(
			entity.FindComponent(FactionAffiliationComponent));
		return faction && faction.GetAffiliatedFactionKey() == CIVILIAN_FACTION_KEY;
	}

	protected bool IsExactCivilianGroupMembership(IEntity memberEntity, AIGroup group)
	{
		if (!memberEntity || !group)
			return false;
		AIAgent agent = ResolveCivilianAgentReadOnly(memberEntity);
		if (!agent || agent.GetParentGroup() != group)
			return false;
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		if (agents.Count() != 1 || agents[0] != agent)
			return false;

		SCR_AIGroup scrGroup = SCR_AIGroup.Cast(group);
		if (!scrGroup || scrGroup.GetFactionName() != CIVILIAN_FACTION_KEY)
			return false;
		FactionAffiliationComponent groupFaction = FactionAffiliationComponent.Cast(
			group.FindComponent(FactionAffiliationComponent));
		return !groupFaction
			|| groupFaction.GetAffiliatedFactionKey() == CIVILIAN_FACTION_KEY;
	}

	protected bool HasActiveCivilianWaypoint(AIGroup group)
	{
		if (!group)
			return false;
		AIWaypoint currentWaypoint = group.GetCurrentWaypoint();
		if (!currentWaypoint)
			return false;
		array<AIWaypoint> waypoints = {};
		group.GetWaypoints(waypoints);
		return waypoints.Contains(currentWaypoint);
	}

	protected bool HasAssignedCivilianWaypoint(AIGroup group)
	{
		if (!group)
			return false;
		array<AIWaypoint> waypoints = {};
		group.GetWaypoints(waypoints);
		return !waypoints.IsEmpty();
	}

	protected bool IsExactAmbientTrafficDriverSeated(HST_AmbientActorRuntimeRecord record)
	{
		if (!record || !record.m_RootEntity || !IsLivingAmbientCharacter(record.m_DriverEntity))
			return false;
		BaseCompartmentManagerComponent compartmentManager = ResolveCompartmentManager(record.m_RootEntity);
		if (!compartmentManager)
			return false;

		array<BaseCompartmentSlot> slots = {};
		compartmentManager.GetCompartments(slots);
		bool exactPilot;
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot || (!slot.IsPiloting() && slot.GetType() != ECompartmentType.PILOT))
				continue;
			if (slot.GetOccupant() == record.m_DriverEntity)
			{
				exactPilot = true;
				break;
			}
		}
		if (!exactPilot)
			return false;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(
			record.m_DriverEntity.FindComponent(SCR_CompartmentAccessComponent));
		return access
			&& access.IsInCompartment()
			&& access.GetVehicle() == record.m_RootEntity;
	}

	protected BaseVehicleControllerComponent ResolveAmbientVehicleController(IEntity vehicleEntity)
	{
		if (!vehicleEntity)
			return null;
		BaseVehicleControllerComponent controller = BaseVehicleControllerComponent.Cast(
			vehicleEntity.FindComponent(BaseVehicleControllerComponent));
		if (controller)
			return controller;
		Vehicle vehicle = Vehicle.Cast(vehicleEntity);
		if (!vehicle)
			return null;
		return BaseVehicleControllerComponent.Cast(vehicle.GetVehicleController());
	}

	protected bool IsAmbientTrafficEngineOn(HST_AmbientActorRuntimeRecord record)
	{
		if (!record)
			return false;
		BaseVehicleControllerComponent controller = ResolveAmbientVehicleController(record.m_RootEntity);
		return controller && controller.IsEngineOn();
	}

	protected void ClearAmbientMovementHelpers(HST_AmbientActorRuntimeRecord record)
	{
		if (!record || !record.m_RootEntity)
			return;
		if (record.m_Group)
		{
			array<AIWaypoint> waypoints = {};
			record.m_Group.GetWaypoints(waypoints);
			foreach (AIWaypoint waypoint : waypoints)
			{
				if (waypoint)
					record.m_Group.RemoveWaypoint(waypoint);
			}
		}

		for (int index = m_aRuntimeHelperOwners.Count() - 1; index >= 0; index--)
		{
			if (m_aRuntimeHelperOwners[index] != record.m_RootEntity)
				continue;
			IEntity helper = m_aRuntimeHelperEntities[index];
			if (helper == record.m_DriverEntity || AIGroup.Cast(helper) == record.m_Group)
				continue;
			if (helper)
				SCR_EntityHelper.DeleteEntityAndChildren(helper);
			m_aRuntimeHelperOwners.Remove(index);
			m_aRuntimeHelperEntities.Remove(index);
		}
	}

	protected bool RecycleAmbientActorAt(
		HST_CampaignState state,
		int recordIndex,
		string reason)
	{
		if (recordIndex < 0 || recordIndex >= m_aAmbientActorRecords.Count())
			return false;
		HST_AmbientActorRuntimeRecord record = m_aAmbientActorRecords[recordIndex];
		if (!record)
		{
			m_aAmbientActorRecords.Remove(recordIndex);
			return true;
		}
		if (record.m_bCasualtyAdmissionPending
			&& !TryAdmitRetainedCasualtyObservation(state, record))
			return false;

		IEntity rootEntity = record.m_RootEntity;
		if (record.m_sKindId == HST_AmbientActorRuntimeService.KIND_TRAFFIC
			&& rootEntity && HasPlayerOccupant(rootEntity))
			return false;
		int nowSecond;
		if (state)
			nowSecond = state.m_iElapsedSeconds;
		if (m_AmbientRuntime)
			m_AmbientRuntime.QueueDespawn(record, reason, nowSecond);
		m_aAmbientActorRecords.Remove(recordIndex);
		if (!rootEntity)
			return true;

		int runtimeIndex = m_aRuntimeEntities.Find(rootEntity);
		DeleteRuntimeHelpersForOwner(rootEntity);
		RemoveTransientAmbientRuntimeVehicleRecord(state, rootEntity);
		SCR_EntityHelper.DeleteEntityAndChildren(rootEntity);
		if (runtimeIndex >= 0)
			RemoveRuntimeEntityAt(runtimeIndex);
		return true;
	}

	protected void QueueAndRemoveAmbientActorRecordForEntity(
		IEntity entity,
		HST_CampaignState state,
		string reason)
	{
		if (!entity)
			return;
		for (int index = m_aAmbientActorRecords.Count() - 1; index >= 0; index--)
		{
			HST_AmbientActorRuntimeRecord record = m_aAmbientActorRecords[index];
			if (!record || record.m_RootEntity != entity)
				continue;
			if (record.m_bCasualtyAdmissionPending
				&& !TryAdmitRetainedCasualtyObservation(state, record))
			{
				// The caller may now delete or promote the live root. Retain only
				// the already-cached consequence observation so its later admission
				// cannot recycle a durable vehicle or a replacement entity.
				record.m_RootEntity = null;
				record.m_DriverEntity = null;
				record.m_Group = null;
				continue;
			}
			int nowSecond;
			if (state)
				nowSecond = state.m_iElapsedSeconds;
			if (m_AmbientRuntime)
				m_AmbientRuntime.QueueDespawn(record, reason, nowSecond);
			m_aAmbientActorRecords.Remove(index);
		}
	}

	protected void PruneDeletedAmbientActorRecords(HST_CampaignState state)
	{
		for (int index = m_aAmbientActorRecords.Count() - 1; index >= 0; index--)
		{
			HST_AmbientActorRuntimeRecord record = m_aAmbientActorRecords[index];
			if (record && record.m_bCasualtyAdmissionPending
				&& !TryAdmitRetainedCasualtyObservation(state, record))
				continue;
			if (!record || !record.m_RootEntity)
			{
				if (record && m_AmbientRuntime)
				{
					int nowSecond;
					if (state)
						nowSecond = state.m_iElapsedSeconds;
					m_AmbientRuntime.QueueDespawn(
						record,
						"ambient root lost",
						nowSecond);
				}
				m_aAmbientActorRecords.Remove(index);
			}
		}
	}

	// Called before the persistence tick on every server frame. Player-first
	// discovery avoids scanning every ambient root for occupancy and closes the
	// brief enter/exit gap left by the slower health cadence.
	bool ObservePlayerAmbientVehicleClaims(HST_CampaignState state)
	{
		if (m_CampaignDebugCivilianMutationLease)
		{
			string collisionEvidence;
			HasCampaignDebugCivilianMutationForeignClaim(
				state,
				collisionEvidence);
			return false;
		}
		// A controlled-end binding plan or field fence owns an immutable root set.
		// Coordinator retries may call this discovery hook again, but they must not
		// promote a new root after either one-way latch has been published.
		if (m_PersistentFieldVehicles
			&& (m_PersistentFieldVehicles
				.HasControlledShutdownBindingScopeLocked()
				|| m_PersistentFieldVehicles
					.HasControlledShutdownQuiescenceApplied()))
		{
			m_bLastAmbientClaimObservationExact = true;
			return false;
		}
		if (!state)
		{
			m_bLastAmbientClaimObservationExact = false;
			return false;
		}
		array<IEntity> occupiedVehicleRoots = {};
		CollectPlayerOccupiedVehicleRoots(occupiedVehicleRoots);
		bool changed;
		bool exact = true;
		foreach (IEntity entity : occupiedVehicleRoots)
		{
			int index = m_aRuntimeEntities.Find(entity);
			if (!IsLivingAmbientEntity(entity))
				continue;
			if (index < 0)
			{
				// After process restart, restoration keeps the saved durable ID and
				// binds it to the new live root. Adopt that exact field binding on
				// first player occupancy so later autosaves keep following it.
				HST_RuntimeVehicleState restoredField
					= ResolveRuntimeVehicleRecord(state, entity);
				if (restoredField
					&& (restoredField.m_sRuntimeKind == "field_vehicle"
						|| restoredField.m_sRuntimeKind == "loot_vehicle"
						|| restoredField.m_sRuntimeKind == "garage_redeploy")
					&& !restoredField.m_bDeleted)
				{
					if (!m_PersistentFieldVehicles
						|| !m_PersistentFieldVehicles.Track(
							entity,
							restoredField))
						exact = false;
				}
				continue;
			}
			if (index >= m_aRuntimeEntityKinds.Count()
				|| !IsRuntimeVehicle(m_aRuntimeEntityKinds[index]))
			{
				exact = false;
				continue;
			}

			string runtimeKind = m_aRuntimeEntityKinds[index];
			string zoneId = m_aRuntimeEntityZoneIds[index];
			string claimantFactionKey
				= ResolveExactVehicleClaimantFaction(entity);
			// Passenger occupancy protects the live ambient root from cleanup but
			// is not a theft or durable-claim signal. Wait for exact pilot control.
			if (claimantFactionKey.IsEmpty())
				continue;
			if (!PromoteClaimedRuntimeVehicleWithConsequences(
				state,
				entity,
				runtimeKind,
				zoneId,
				claimantFactionKey))
			{
				Print("Partisan civilians | player-used ambient vehicle could not enter persistent field authority", LogLevel.ERROR);
				exact = false;
				continue;
			}
			QueueAndRemoveAmbientActorRecordForEntity(
				entity,
				state,
				"promoted_player_claim");
			DeleteRuntimeHelpersForOwner(entity);
			RemoveRuntimeEntityAt(index);
			Print(string.Format("Partisan civilians | promoted player-used %1 from %2 to persistent field vehicle", runtimeKind, zoneId));
			changed = true;
		}
		// Per-frame work is claim discovery only. Full transform/destruction/cargo
		// reconciliation runs synchronously at the persistence boundary.
		m_bLastAmbientClaimObservationExact = exact;
		return changed;
	}

	// Persistence calls this synchronously before every capture, including
	// autosave, manual, debug-isolation, and reset checkpoints. A live promoted
	// root without an exact durable record fails closed instead of saving stale
	// or transient authority.
	bool PrepareAmbientVehiclePersistence(HST_CampaignState state)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!state || !m_CivilianConsequences)
			return false;
		if (m_PersistentFieldVehicles
			&& m_PersistentFieldVehicles
				.HasControlledShutdownQuiescenceApplied())
		{
			return m_PersistentFieldVehicles.PrepareForCapture(state);
		}
		ObservePlayerAmbientVehicleClaims(state);
		FlushPendingCivilianConsequences(state);
		if (!m_bLastAmbientClaimObservationExact
			|| HasPendingCivilianConsequenceWork()
			|| !m_PersistentFieldVehicles)
			return false;
		return m_PersistentFieldVehicles.PrepareForCapture(state);
	}

	bool PrepareControlledShutdownVehiclePersistence(
		HST_CampaignState state,
		out string evidence)
	{
		evidence
			= "controlled-shutdown durable field vehicle authority unavailable";
		if (m_CampaignDebugCivilianMutationLease)
		{
			evidence = "campaign-debug civilian mutation lease is active";
			return false;
		}
		if (!state || !m_PersistentFieldVehicles)
			return false;
		// Persistence owns the one normal PrepareStateForCapture pass before any
		// runtime fence is applied. Do not observe/promote or resample here: the
		// strict field preflight immediately before active-group mutation already
		// covered every promoted player-used root in that prepared authority.
		return m_PersistentFieldVehicles.PrepareForControlledShutdown(
			state,
			evidence);
	}

	bool PreflightControlledShutdownVehiclePersistence(
		HST_CampaignState state,
		out string evidence,
		bool requirePreparedAuthority = false)
	{
		evidence
			= "controlled-shutdown durable field vehicle preflight unavailable";
		if (!state || !m_PersistentFieldVehicles)
			return false;
		return m_PersistentFieldVehicles.PreflightControlledShutdownQuiescence(
			state,
			evidence,
			requirePreparedAuthority);
	}

	bool HasControlledShutdownVehiclePersistenceApplied()
	{
		return m_PersistentFieldVehicles
			&& m_PersistentFieldVehicles
				.HasControlledShutdownQuiescenceApplied();
	}

	bool MaintainControlledShutdownVehiclePersistence(
		HST_CampaignState state,
		out string evidence)
	{
		evidence
			= "controlled-shutdown durable field vehicle quiescence maintenance unavailable";
		if (m_CampaignDebugCivilianMutationLease)
		{
			evidence = "campaign-debug civilian mutation lease is active";
			return false;
		}
		if (!state || !m_PersistentFieldVehicles)
			return false;
		return m_PersistentFieldVehicles.MaintainControlledShutdownQuiescence(
			state,
			evidence);
	}

	protected bool PromotePlayerOccupiedRuntimeVehicles(HST_CampaignState state)
	{
		return ObservePlayerAmbientVehicleClaims(state);
	}

	protected void CollectPlayerOccupiedVehicleRoots(array<IEntity> vehicleRoots)
	{
		if (!vehicleRoots)
			return;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			IEntity controlledEntity
				= playerManager.GetPlayerControlledEntity(playerId);
			IEntity mainEntity
				= SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
			CollectPlayerOccupiedVehicleRootFromEntity(
				controlledEntity,
				vehicleRoots);
			if (mainEntity != controlledEntity)
			{
				CollectPlayerOccupiedVehicleRootFromEntity(
					mainEntity,
					vehicleRoots);
			}
		}
	}

	protected void CollectPlayerOccupiedVehicleRootFromEntity(
		IEntity playerEntity,
		array<IEntity> vehicleRoots)
	{
		if (!playerEntity || !vehicleRoots)
			return;
		if (ChimeraCharacter.Cast(playerEntity)
			&& !IsLivingAmbientCharacter(playerEntity))
			return;
		IEntity vehicleEntity = ResolveEntityVehicle(playerEntity);
		if (!vehicleEntity && Vehicle.Cast(playerEntity))
			vehicleEntity = playerEntity;
		if (!IsLivingAmbientEntity(vehicleEntity)
			|| vehicleRoots.Find(vehicleEntity) >= 0)
			return;
		vehicleRoots.Insert(vehicleEntity);
	}

	protected bool HasPlayerOccupant(IEntity vehicleEntity)
	{
		if (!IsLivingAmbientEntity(vehicleEntity))
			return false;
		array<IEntity> occupiedVehicleRoots = {};
		CollectPlayerOccupiedVehicleRoots(occupiedVehicleRoots);
		return occupiedVehicleRoots.Find(vehicleEntity) >= 0;
	}

	protected bool IsPlayerControlledEntity(IEntity entity)
	{
		if (!entity)
			return false;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		return playerManager
			&& playerManager.GetPlayerIdFromControlledEntity(entity) > 0;
	}

	protected void RemoveTransientAmbientRuntimeVehicleRecord(
		HST_CampaignState state,
		IEntity entity)
	{
		HST_RuntimeVehicleState vehicle = ResolveRuntimeVehicleRecord(state, entity);
		if (!state || !vehicle
			|| !IsTransientAmbientVehicleKind(vehicle.m_sRuntimeKind))
			return;
		RemoveTransientAmbientRuntimeVehicleRecordById(
			state,
			vehicle.m_sVehicleRuntimeId);
	}

	protected void RemoveTransientAmbientRuntimeVehicleRecordById(
		HST_CampaignState state,
		string vehicleRuntimeId)
	{
		if (!state || vehicleRuntimeId.IsEmpty())
			return;
		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
		if (!vehicle || !IsTransientAmbientVehicleKind(vehicle.m_sRuntimeKind))
			return;
		for (int cargoIndex = state.m_aVehicleCargoItems.Count() - 1; cargoIndex >= 0; cargoIndex--)
		{
			HST_VehicleCargoItemState cargoItem = state.m_aVehicleCargoItems[cargoIndex];
			if (cargoItem && cargoItem.m_sVehicleRuntimeId == vehicleRuntimeId)
				state.m_aVehicleCargoItems.Remove(cargoIndex);
		}
		state.RemoveRuntimeVehicle(vehicleRuntimeId);
	}

	bool RegisterOwnershipSupportReward(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string zoneId,
		int supportReward,
		string ownershipRequestId,
		bool reconcileOwnership = true)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		return m_TownInfluence && m_TownInfluence.RegisterOwnershipSupportReward(
			state,
			ResolveInfluencePreset(preset),
			zoneId,
			supportReward,
			ownershipRequestId,
			reconcileOwnership);
	}

	protected void CleanupFailedAmbientRuntimeEntity(
		HST_CampaignState state,
		IEntity runtimeEntity,
		string zoneId,
		string runtimeKind)
	{
		if (!runtimeEntity)
			return;

		int runtimeIndex = m_aRuntimeEntities.Find(runtimeEntity);
		QueueAndRemoveAmbientActorRecordForEntity(
			runtimeEntity,
			state,
			"ambient transaction failed");
		DeleteRuntimeHelpersForOwner(runtimeEntity);
		RemoveTransientAmbientRuntimeVehicleRecord(state, runtimeEntity);
		SCR_EntityHelper.DeleteEntityAndChildren(runtimeEntity);
		if (runtimeIndex >= 0)
			RemoveRuntimeEntityAt(runtimeIndex);
		Print(string.Format("Partisan civilians | removed failed ambient %1 projection for %2", runtimeKind, zoneId), LogLevel.WARNING);
	}

	protected IEntity SpawnCivilianTrafficDriver(HST_CampaignState state, HST_BalanceConfig balance, HST_ZoneState zone, IEntity vehicleEntity, int index, int seed)
	{
		if (!state || !balance || !zone || !vehicleEntity)
			return null;

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(zone.m_sZoneId);
		int appearanceSlot = ResolveCivilianPedestrianTarget(
			civilianZone,
			zone,
			balance,
			state) + index;
		int appearanceSeed = BuildZoneSeed(state, zone, CIVILIAN_APPEARANCE_SEED_SALT);
		array<string> usedAppearancePrefabs = {};
		BuildUsedCivilianActorAppearancePrefabs(zone.m_sZoneId, usedAppearancePrefabs);
		string driverPrefab = SelectCivilianCharacterPrefabAvoiding(balance, appearanceSlot, appearanceSeed, usedAppearancePrefabs);
		if (driverPrefab.IsEmpty())
			return null;

		vector driverPosition = vehicleEntity.GetOrigin();
		driverPosition[0] = driverPosition[0] + 1.4;
		driverPosition[2] = driverPosition[2] + 1.4;
		driverPosition = HST_WorldPositionService.ResolveSafeGroundPosition(driverPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 2.0);
		GenericEntity driverEntity = HST_WorldPositionService.SpawnPrefab(driverPrefab, driverPosition, BuildSpawnAngles(seed + 43));
		TrackCampaignDebugCivilianMutationEntity(driverEntity);
		if (!driverEntity)
		{
			RecordSpawnFailure(zone.m_sZoneId, driverPrefab, "CIV_TRAFFIC_DRIVER", driverPosition, BuildSpawnAngles(seed + 43));
			return null;
		}

		ApplyFaction(driverEntity, CIVILIAN_FACTION_KEY, "CIV_CHARACTER");
		RegisterRuntimeHelper(vehicleEntity, driverEntity);
		return driverEntity;
	}

	int SuppressAmbientTrafficHornInput(string zoneId = "")
	{
		int cleared;
		// This runs every server frame. Walk the bounded actor registry directly;
		// helper-owner lookup would multiply helpers by roots at the global caps.
		foreach (HST_AmbientActorRuntimeRecord record : m_aAmbientActorRecords)
		{
			if (!record
				|| record.m_sKindId != HST_AmbientActorRuntimeService.KIND_TRAFFIC
				|| (!zoneId.IsEmpty() && record.m_sZoneId != zoneId))
				continue;
			if (ClearAmbientTrafficDriverHornInput(record.m_DriverEntity))
				cleared++;
		}

		return cleared;
	}

	int CountAmbientTrafficDriversWithHornInput(string zoneId = "")
	{
		int sounding;
		foreach (HST_AmbientActorRuntimeRecord record : m_aAmbientActorRecords)
		{
			if (!record
				|| record.m_sKindId != HST_AmbientActorRuntimeService.KIND_TRAFFIC
				|| (!zoneId.IsEmpty() && record.m_sZoneId != zoneId))
				continue;

			IEntity driver = record.m_DriverEntity;
			if (!driver)
				continue;
			SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(driver.FindComponent(SCR_CharacterControllerComponent));
			if (!controller)
				continue;
			CharacterInputContext inputContext = controller.GetInputContext();
			if (inputContext && inputContext.GetVehicleHorn() != 0)
				sounding++;
		}

		return sounding;
	}

	protected bool ClearAmbientTrafficDriverHornInput(IEntity driverEntity)
	{
		if (!driverEntity)
			return false;

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(driverEntity.FindComponent(SCR_CharacterControllerComponent));
		if (!controller)
			return false;

		CharacterInputContext inputContext = controller.GetInputContext();
		if (!inputContext)
			return false;

		inputContext.SetVehicleHorn(0);
		return true;
	}

	protected AIGroup EnsureCivilianAIGroup(
		IEntity helperOwner,
		IEntity memberEntity,
		string factionKey,
		bool activateMembers = true)
	{
		if (!helperOwner || !memberEntity)
			return null;

		AIAgent agent;
		if (activateMembers)
			agent = ResolveCivilianAgent(memberEntity);
		else
			agent = ResolveCivilianAgentReadOnly(memberEntity);
		if (!agent)
			return null;

		AIGroup group = agent.GetParentGroup();
		if (group)
		{
			if (!IsRuntimeGroupHelperForOwner(helperOwner, group))
				return null;
			ApplyFaction(memberEntity, CIVILIAN_FACTION_KEY, "CIV_CHARACTER");
			ApplyCivilianAIGroupFaction(group, CIVILIAN_FACTION_KEY);
			if (activateMembers)
			{
				group.ActivateAllMembers();
				agent.ActivateAI();
			}
			if (IsExactCivilianGroupMembership(memberEntity, group))
				return group;
			return null;
		}

		IEntity groupEntity = HST_WorldPositionService.SpawnPrefab(CIVILIAN_AI_GROUP_PREFAB, memberEntity.GetOrigin(), "0 0 0");
		TrackCampaignDebugCivilianMutationEntity(groupEntity);
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
			scrGroup.InitFactionKey(CIVILIAN_FACTION_KEY);
			// This is initial ambient AI composition, not a player-group membership
			// change. The stock initial-spawn path attaches through this helper without
			// broadcasting the player-group member-state RPC.
			scrGroup.AddAIEntityToGroup(memberEntity);
			if (agent.GetParentGroup() != group)
				group.AddAgent(agent);
		}
		else
		{
			group.AddAgent(agent);
		}

		ApplyFaction(memberEntity, CIVILIAN_FACTION_KEY, "CIV_CHARACTER");
		ApplyCivilianAIGroupFaction(group, CIVILIAN_FACTION_KEY);
		if (activateMembers)
		{
			group.ActivateAllMembers();
			agent.ActivateAI();
		}
		if (!IsExactCivilianGroupMembership(memberEntity, group))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(groupEntity);
			return null;
		}
		RegisterRuntimeHelper(helperOwner, groupEntity);
		return group;
	}

	protected bool IsRuntimeGroupHelperForOwner(IEntity helperOwner, AIGroup group)
	{
		if (!helperOwner || !group)
			return false;
		for (int index; index < m_aRuntimeHelperOwners.Count(); index++)
		{
			if (m_aRuntimeHelperOwners[index] != helperOwner
				|| index >= m_aRuntimeHelperEntities.Count())
				continue;
			if (AIGroup.Cast(m_aRuntimeHelperEntities[index]) == group)
				return true;
		}
		return false;
	}

	protected string ResolveRuntimeEntityFactionKey(string factionKey, string runtimeKind)
	{
		if (runtimeKind == "CIV_CHARACTER" || runtimeKind == "CIV_VEHICLE" || runtimeKind == CIVILIAN_TRAFFIC_RUNTIME_KIND)
			return CIVILIAN_FACTION_KEY;

		return factionKey;
	}

	protected void ApplyCivilianAIGroupFaction(AIGroup group, string factionKey)
	{
		if (!group || factionKey.IsEmpty())
			return;

		SCR_AIGroup scrGroup = SCR_AIGroup.Cast(group);
		if (scrGroup)
		{
			if (scrGroup.GetFactionName().IsEmpty())
				scrGroup.InitFactionKey(factionKey);
			if (scrGroup.GetFactionName() != factionKey)
			{
				Faction faction = ResolveRuntimeFaction(factionKey);
				if (faction)
					scrGroup.SetFaction(faction);
			}
		}

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(group.FindComponent(FactionAffiliationComponent));
		if (factionComponent)
			factionComponent.SetAffiliatedFactionByKey(factionKey);
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

	protected AIAgent ResolveCivilianAgent(IEntity entity)
	{
		AIAgent agent = ResolveCivilianAgentReadOnly(entity);
		if (agent)
		{
			AIControlComponent control = AIControlComponent.Cast(
				entity.FindComponent(AIControlComponent));
			if (control)
			control.ActivateAI();
		}

		return agent;
	}

	protected AIAgent ResolveCivilianAgentReadOnly(IEntity entity)
	{
		if (!entity)
			return null;
		AIControlComponent control = AIControlComponent.Cast(
			entity.FindComponent(AIControlComponent));
		if (!control)
			return null;
		AIAgent agent = control.GetControlAIAgent();
		if (!agent)
			agent = control.GetAIAgent();
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

		AIAgent agent = ResolveCivilianAgentReadOnly(entity);
		if (!agent)
			return;

		AICharacterMovementComponent movement = AICharacterMovementComponent.Cast(agent.GetMovementComponent());
		if (movement)
			movement.SetMovementTypeWanted(movementType);
	}

	protected int AssignCivilianCycleWaypoints(
		IEntity helperOwner,
		AIGroup group,
		array<vector> positions,
		float completionRadiusMeters,
		bool vehicleRoute,
		HST_AmbientActorRuntimeRecord replacedRecord = null)
	{
		if (!helperOwner || !group || !positions || positions.Count() < 2)
			return 0;

		array<vector> admittedPositions = {};
		if (vehicleRoute)
		{
			foreach (vector vehiclePosition : positions)
			{
				admittedPositions.Insert(vehiclePosition);
			}
		}
		else if (!TryResolveCivilianPedestrianWaypoints(
			group,
			positions,
			admittedPositions))
		{
			return 0;
		}

		array<IEntity> spawnedEntities = {};
		array<AIWaypoint> waypoints = {};
		for (int i = 0; i < admittedPositions.Count(); i++)
		{
			vector waypointPosition = admittedPositions[i];
			if (vehicleRoute)
				waypointPosition = ResolveCivilianTrafficWaypoint(waypointPosition, waypointPosition);

			GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(CIVILIAN_WANDER_WAYPOINT_PREFAB, waypointPosition, "0 0 0");
			TrackCampaignDebugCivilianMutationEntity(waypointEntity);
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

		GenericEntity cycleEntity = HST_WorldPositionService.SpawnPrefab(CIVILIAN_WANDER_CYCLE_WAYPOINT_PREFAB, admittedPositions[0], "0 0 0");
		TrackCampaignDebugCivilianMutationEntity(cycleEntity);
		AIWaypointCycle waypointCycle = AIWaypointCycle.Cast(cycleEntity);
		if (!waypointCycle)
		{
			if (cycleEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(cycleEntity);
			DeleteSpawnedHelperEntities(spawnedEntities);
			return 0;
		}

		waypointCycle.SetWaypoints(waypoints);
		if (replacedRecord)
			ClearAmbientMovementHelpers(replacedRecord);
		foreach (IEntity helper : spawnedEntities)
		{
			RegisterRuntimeHelper(helperOwner, helper);
		}
		RegisterRuntimeHelper(helperOwner, cycleEntity);
		group.AddWaypoint(waypointCycle);
		group.ActivateAllMembers();
		return waypoints.Count();
	}

	protected bool TryResolveCivilianPedestrianWaypoints(
		AIGroup group,
		array<vector> preferredPositions,
		array<vector> resolvedPositions)
	{
		if (!group || !preferredPositions || !resolvedPositions
			|| preferredPositions.IsEmpty())
			return false;

		resolvedPositions.Clear();
		foreach (vector preferredPosition : preferredPositions)
		{
			vector resolvedPosition;
			if (!TryResolveCivilianPedestrianWaypoint(
				group,
				preferredPosition,
				resolvedPosition))
			{
				resolvedPositions.Clear();
				return false;
			}
			resolvedPositions.Insert(resolvedPosition);
		}

		return resolvedPositions.Count() == preferredPositions.Count();
	}

	protected bool TryResolveCivilianPedestrianWaypoint(
		AIGroup group,
		vector preferredPosition,
		out vector resolvedPosition)
	{
		resolvedPosition = preferredPosition;
		if (!group)
			return false;

		AIPathfindingComponent pathfinding = AIPathfindingComponent.Cast(
			group.FindComponent(AIPathfindingComponent));
		if (!pathfinding)
			return false;
		NavmeshWorldComponent navmesh = pathfinding.GetNavmeshComponent();
		if (!navmesh)
			return false;

		if (navmesh.IsTileRequested(preferredPosition))
			return false;
		if (!navmesh.IsTileLoaded(preferredPosition))
		{
			navmesh.LoadTileIn(preferredPosition);
			return false;
		}

		return pathfinding.GetClosestPositionOnNavmesh(
			preferredPosition,
			"10 10 10",
			resolvedPosition);
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

		if (!utility.IsUsableVehicle(vehicleUsage))
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

		IEntity slotOwner = slot.GetOwner();
		if (!slotOwner)
			slotOwner = vehicleEntity;

		RplComponent driverReplication = RplComponent.Cast(driverEntity.FindComponent(RplComponent));
		if ((!driverReplication || driverReplication.IsOwner()) && access.GetInVehicle(slotOwner, slot, true, -1, ECloseDoorAfterActions.INVALID, true))
		{
			reason = "authority-local driver entry accepted";
			return true;
		}

		if (access.MoveInVehicle(vehicleEntity, ECompartmentType.PILOT, true, slot))
		{
			reason = "owner driver move-in request accepted";
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
		TrackCampaignDebugCivilianMutationEntity(entity);
		if (!entity)
		{
			RecordSpawnFailure(zoneId, prefab, runtimeKind, position, angles);
			return false;
		}
		if (IsRuntimeVehicle(runtimeKind))
			HST_WorldPositionService.ApplyUprightEntityTransform(entity, position, angles);

		string runtimeFactionKey = ResolveRuntimeEntityFactionKey(factionKey, runtimeKind);
		ApplyFaction(entity, runtimeFactionKey, runtimeKind);
		string stableVehicleRuntimeId;
		if (IsRuntimeVehicle(runtimeKind))
		{
			stableVehicleRuntimeId = RegisterRuntimeVehicle(
				state,
				entity,
				zoneId,
				prefab,
				position,
				angles,
				runtimeFactionKey,
				runtimeKind);
			if (stableVehicleRuntimeId.IsEmpty())
			{
				RecordSpawnFailure(
					zoneId,
					prefab,
					runtimeKind,
					position,
					angles);
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
				return false;
			}
		}
		m_aRuntimeEntityZoneIds.Insert(zoneId);
		m_aRuntimeEntityKinds.Insert(runtimeKind);
		m_aRuntimeEntityFactionKeys.Insert(runtimeFactionKey);
		m_aRuntimeEntityVehicleIds.Insert(stableVehicleRuntimeId);
		m_aRuntimeEntitySpawnPositions.Insert(position);
		m_aRuntimeEntities.Insert(entity);
		Print(string.Format("Partisan civilians | spawn ok | zone %1 | kind %2 | faction %3 | prefab %4 | pos %5 | yaw %6", zoneId, runtimeKind, runtimeFactionKey, prefab, position, angles[0]));
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
			bool playerClaim = entity
				&& ShouldDetachFromTownCleanup(state, entity, runtimeKind);
			if (playerClaim)
			{
				string claimantFactionKey
					= ResolveExactVehicleClaimantFaction(entity);
				if (claimantFactionKey.IsEmpty())
					continue;
				if (!PromoteClaimedRuntimeVehicleWithConsequences(
					state,
					entity,
					runtimeKind,
					zoneId,
					claimantFactionKey))
				{
					Print("Partisan civilians | town cleanup retained player-used vehicle because persistent field admission failed", LogLevel.ERROR);
					continue;
				}
				QueueAndRemoveAmbientActorRecordForEntity(
					entity,
					state,
					"promoted_player_claim");
				DeleteRuntimeHelpersForOwner(entity);
				Print(string.Format("Partisan civilians | promoted player-used runtime %1 from town cleanup for %2", runtimeKind, zoneId));
			}
			else if (entity)
			{
				QueueAndRemoveAmbientActorRecordForEntity(
					entity,
					state,
					"town projection deactivated");
				DeleteRuntimeHelpersForOwner(entity);
				RemoveTransientAmbientRuntimeVehicleRecord(state, entity);
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			}

			RemoveRuntimeEntityAt(i);
			changed = true;
		}
		if (CountRuntimeEntitiesForZone(zoneId) > 0)
			return changed;

		for (int j = m_aRuntimeZoneIds.Count() - 1; j >= 0; j--)
		{
			if (m_aRuntimeZoneIds[j] == zoneId)
			{
				m_aRuntimeZoneIds.Remove(j);
				changed = true;
			}
		}
		ClearAmbientSpawnRetry(zoneId, "CIV_CHARACTER");
		ClearAmbientSpawnRetry(zoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND);
		ClearAmbientSpawnRetry(zoneId, "CIV_VEHICLE");
		ClearAmbientSpawnRetry(zoneId, "MILITARY_VEHICLE");
		ClearStaticVehicleInitialization(zoneId);

		if (changed)
			Print(string.Format("Partisan civilians | cleaned runtime town population for %1", zoneId));

		return changed;
	}

	protected bool CleanupAllRuntimeEntities(HST_CampaignState state)
	{
		bool changed;
		for (int i = m_aRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			IEntity entity = m_aRuntimeEntities[i];
			string runtimeKind = m_aRuntimeEntityKinds[i];
			if (!entity)
			{
				if (i < m_aRuntimeEntityVehicleIds.Count()
					&& !m_aRuntimeEntityVehicleIds[i].IsEmpty())
				{
					RemoveTransientAmbientRuntimeVehicleRecordById(
						state,
						m_aRuntimeEntityVehicleIds[i]);
				}
				RemoveRuntimeEntityAt(i);
				changed = true;
				continue;
			}
			bool playerClaim = entity
				&& ShouldDetachFromTownCleanup(state, entity, runtimeKind);
			if (playerClaim)
			{
				string runtimeZoneId = "";
				if (i < m_aRuntimeEntityZoneIds.Count())
					runtimeZoneId = m_aRuntimeEntityZoneIds[i];
				string claimantFactionKey
					= ResolveExactVehicleClaimantFaction(entity);
				if (claimantFactionKey.IsEmpty())
					continue;
				if (!PromoteClaimedRuntimeVehicleWithConsequences(
					state,
					entity,
					runtimeKind,
					runtimeZoneId,
					claimantFactionKey))
				{
					Print("Partisan civilians | runtime shutdown retained player-used vehicle because persistent field admission failed", LogLevel.ERROR);
					continue;
				}
				QueueAndRemoveAmbientActorRecordForEntity(
					entity,
					state,
					"promoted_player_claim");
				DeleteRuntimeHelpersForOwner(entity);
				Print(string.Format("Partisan civilians | promoted player-used runtime %1 while civilian runtime disabled", runtimeKind));
			}
			else if (entity)
			{
				QueueAndRemoveAmbientActorRecordForEntity(
					entity,
					state,
					"civilian runtime disabled");
				DeleteRuntimeHelpersForOwner(entity);
				RemoveTransientAmbientRuntimeVehicleRecord(state, entity);
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			}

			RemoveRuntimeEntityAt(i);
			changed = true;
		}

		if (m_aRuntimeEntities.IsEmpty())
		{
			m_aRuntimeZoneIds.Clear();
			m_aAmbientActorRecords.Clear();
			m_aAmbientRetryZoneIds.Clear();
			m_aAmbientRetryKinds.Clear();
			m_aAmbientRetrySeconds.Clear();
			m_aStaticVehicleInitializationZoneIds.Clear();
			m_aStaticCivilianVehicleSlotsCompleted.Clear();
			m_aStaticMilitaryVehicleSlotsCompleted.Clear();
			m_aStaticMilitaryInitializationOwnerKeys.Clear();
			CleanupAllRuntimeHelpers();
		}
		return changed;
	}

	protected void PruneDeletedRuntimeEntities(HST_CampaignState state)
	{
		PruneDeletedRuntimeHelpers();
		for (int i = m_aRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeEntities[i])
				continue;

			if (i < m_aRuntimeEntityVehicleIds.Count()
				&& !m_aRuntimeEntityVehicleIds[i].IsEmpty())
			{
				RemoveTransientAmbientRuntimeVehicleRecordById(
					state,
					m_aRuntimeEntityVehicleIds[i]);
			}
			else
			{
				MarkRuntimeVehicleDeletedBySpawnRecord(
					state,
					m_aRuntimeEntitySpawnPositions[i],
					m_aRuntimeEntityKinds[i]);
			}
			RemoveRuntimeEntityAt(i);
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

			QueueAndRemoveAmbientActorRecordForEntity(
				entity,
				state,
				"outside player render bubble");
			DeleteRuntimeHelpersForOwner(entity);
			RemoveTransientAmbientRuntimeVehicleRecord(state, entity);
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
		if (index < m_aRuntimeEntityVehicleIds.Count())
			m_aRuntimeEntityVehicleIds.Remove(index);
		if (index < m_aRuntimeEntitySpawnPositions.Count())
			m_aRuntimeEntitySpawnPositions.Remove(index);
	}

	protected void RegisterRuntimeHelper(IEntity owner, IEntity helper)
	{
		if (!owner || !helper)
			return;

		TrackCampaignDebugCivilianMutationEntity(helper);
		m_aRuntimeHelperOwners.Insert(owner);
		m_aRuntimeHelperEntities.Insert(helper);
	}

	protected void DeleteRuntimeHelpersForOwner(IEntity owner)
	{
		if (!owner)
			return;
		DetachPlayerControlledRuntimeHelpersFromGroups(owner);

		for (int i = m_aRuntimeHelperOwners.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeHelperOwners[i] != owner)
				continue;

			IEntity helper = m_aRuntimeHelperEntities[i];
			if (helper && !IsPlayerControlledEntity(helper))
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
			if (helper && !IsPlayerControlledEntity(helper))
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

			if (helper && !IsPlayerControlledEntity(helper))
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

	int CountRuntimeEntities()
	{
		int count;
		foreach (IEntity entity : m_aRuntimeEntities)
		{
			if (entity)
				count++;
		}
		return count;
	}

	protected int FindCampaignDebugAmbientRecordIndex(
		IEntity rootEntity,
		out int matchCount)
	{
		matchCount = 0;
		int matchedIndex = -1;
		for (int index; index < m_aAmbientActorRecords.Count(); index++)
		{
			HST_AmbientActorRuntimeRecord record
				= m_aAmbientActorRecords[index];
			if (!record || record.m_RootEntity != rootEntity)
				continue;
			matchedIndex = index;
			matchCount++;
		}
		return matchedIndex;
	}

	protected int FindCampaignDebugRuntimeVehicleRowIndex(
		HST_CampaignState state,
		string vehicleRuntimeId,
		out HST_RuntimeVehicleState matchedRow,
		out int matchCount)
	{
		matchedRow = null;
		matchCount = 0;
		int matchedIndex = -1;
		if (!state || vehicleRuntimeId.IsEmpty())
			return matchedIndex;
		for (int index; index < state.m_aRuntimeVehicles.Count(); index++)
		{
			HST_RuntimeVehicleState row = state.m_aRuntimeVehicles[index];
			if (!row || row.m_sVehicleRuntimeId != vehicleRuntimeId)
				continue;
			matchedRow = row;
			matchedIndex = index;
			matchCount++;
		}
		return matchedIndex;
	}

	protected bool BuildCampaignDebugAmbientRootSnapshot(
		HST_CampaignState state,
		int runtimeIndex,
		out HST_CampaignDebugAmbientRuntimeRootSnapshot snapshot,
		out string evidence)
	{
		snapshot = null;
		evidence = "ambient root snapshot rejected";
		int runtimeCount = m_aRuntimeEntities.Count();
		if (!state || runtimeIndex < 0 || runtimeIndex >= runtimeCount
			|| m_aRuntimeEntityZoneIds.Count() != runtimeCount
			|| m_aRuntimeEntityKinds.Count() != runtimeCount
			|| m_aRuntimeEntityFactionKeys.Count() != runtimeCount
			|| m_aRuntimeEntityVehicleIds.Count() != runtimeCount
			|| m_aRuntimeEntitySpawnPositions.Count() != runtimeCount)
			return false;

		IEntity rootEntity = m_aRuntimeEntities[runtimeIndex];
		if (!rootEntity || rootEntity.IsDeleted())
		{
			evidence = string.Format("runtime index %1 has no live exact root", runtimeIndex);
			return false;
		}
		BaseRplComponent replication = BaseRplComponent.Cast(
			rootEntity.FindComponent(BaseRplComponent));
		if (!replication || replication.Id() == RplId.Invalid())
		{
			evidence = string.Format("runtime index %1 has no stable replication identity", runtimeIndex);
			return false;
		}

		snapshot = new HST_CampaignDebugAmbientRuntimeRootSnapshot();
		snapshot.m_iRuntimeRegistryIndex = runtimeIndex;
		snapshot.m_RootEntity = rootEntity;
		snapshot.m_RootReplicationId = replication.Id();
		snapshot.m_sZoneId = m_aRuntimeEntityZoneIds[runtimeIndex];
		snapshot.m_sRuntimeKind = m_aRuntimeEntityKinds[runtimeIndex];
		snapshot.m_sFactionKey = m_aRuntimeEntityFactionKeys[runtimeIndex];
		snapshot.m_sVehicleRuntimeId
			= m_aRuntimeEntityVehicleIds[runtimeIndex];
		snapshot.m_vSpawnPosition
			= m_aRuntimeEntitySpawnPositions[runtimeIndex];
		snapshot.m_vCurrentPosition = rootEntity.GetOrigin();
		if (snapshot.m_sZoneId.IsEmpty())
		{
			evidence = string.Format("runtime index %1 has no canonical zone", runtimeIndex);
			return false;
		}

		int actorRecordMatches;
		int actorRecordIndex = FindCampaignDebugAmbientRecordIndex(
			rootEntity,
			actorRecordMatches);
		bool expectsActorRecord
			= snapshot.m_sRuntimeKind == "CIV_CHARACTER"
				|| snapshot.m_sRuntimeKind == CIVILIAN_TRAFFIC_RUNTIME_KIND;
		if ((expectsActorRecord && actorRecordMatches != 1)
			|| (!expectsActorRecord && actorRecordMatches != 0))
		{
			evidence = string.Format(
				"runtime index %1 kind %2 has actor record matches %3",
				runtimeIndex,
				snapshot.m_sRuntimeKind,
				actorRecordMatches);
			return false;
		}
		if (expectsActorRecord)
		{
			HST_AmbientActorRuntimeRecord record
				= m_aAmbientActorRecords[actorRecordIndex];
			string expectedActorKind
				= HST_AmbientActorRuntimeService.KIND_PEDESTRIAN;
			if (snapshot.m_sRuntimeKind == CIVILIAN_TRAFFIC_RUNTIME_KIND)
				expectedActorKind = HST_AmbientActorRuntimeService.KIND_TRAFFIC;
			if (!record || record.m_sRuntimeId.IsEmpty()
				|| record.m_sZoneId != snapshot.m_sZoneId
				|| record.m_sKindId != expectedActorKind
				|| record.m_iProjectionSlot < 0)
			{
				evidence = string.Format(
					"runtime index %1 actor identity is incomplete or mismatched",
					runtimeIndex);
				return false;
			}
			snapshot.m_AmbientRecord = record;
			snapshot.m_iActorRecordIndex = actorRecordIndex;
			snapshot.m_sActorRuntimeId = record.m_sRuntimeId;
			snapshot.m_sActorKindId = record.m_sKindId;
			snapshot.m_sActorStateId = record.m_sStateId;
			snapshot.m_DriverEntity = record.m_DriverEntity;
			snapshot.m_Group = record.m_Group;
			snapshot.m_iProjectionSlot = record.m_iProjectionSlot;
			snapshot.m_iProjectionSeed = record.m_iProjectionSeed;
			snapshot.m_iLastSampleAtSecond = record.m_iLastSampleAtSecond;
			snapshot.m_iLastProgressAtSecond = record.m_iLastProgressAtSecond;
			snapshot.m_bActorAdmitted = record.m_bAdmitted;
			snapshot.m_bBehaviorReady
				= m_AmbientRuntime && m_AmbientRuntime.IsBehaviorReady(record);
			snapshot.m_bRecovering
				= record.m_sStateId
					== HST_AmbientActorRuntimeService.STATE_RECOVERING;
			snapshot.m_bMovementObserved = record.m_bMovementObserved;
			if (record.m_Group)
			{
				array<AIWaypoint> waypoints = {};
				record.m_Group.GetWaypoints(waypoints);
				snapshot.m_iWaypointCount = waypoints.Count();
				snapshot.m_bActiveWaypoint
					= HasActiveCivilianWaypoint(record.m_Group);
			}
			bool pedestrianAuthorityExact
				= snapshot.m_sRuntimeKind == "CIV_CHARACTER"
					&& !record.m_DriverEntity
					&& IsExactCivilianFaction(rootEntity)
					&& IsExactCivilianGroupMembership(
						rootEntity,
						record.m_Group);
			bool trafficAuthorityExact
				= snapshot.m_sRuntimeKind == CIVILIAN_TRAFFIC_RUNTIME_KIND
					&& IsExactCivilianFaction(record.m_DriverEntity)
					&& IsExactCivilianGroupMembership(
						record.m_DriverEntity,
						record.m_Group)
					&& IsExactAmbientTrafficDriverSeated(record);
			if (!pedestrianAuthorityExact && !trafficAuthorityExact)
			{
				evidence = string.Format(
					"runtime index %1 actor group, driver, faction, or seating authority is not exact",
					runtimeIndex);
				return false;
			}
		}

		bool expectsVehicleRow = IsRuntimeVehicle(snapshot.m_sRuntimeKind);
		if (expectsVehicleRow != !snapshot.m_sVehicleRuntimeId.IsEmpty())
		{
			evidence = string.Format(
				"runtime index %1 kind %2 has invalid vehicle identity %3",
				runtimeIndex,
				snapshot.m_sRuntimeKind,
				snapshot.m_sVehicleRuntimeId);
			return false;
		}
		if (expectsVehicleRow)
		{
			HST_RuntimeVehicleState vehicleRow;
			int vehicleRowMatches;
			FindCampaignDebugRuntimeVehicleRowIndex(
				state,
				snapshot.m_sVehicleRuntimeId,
				vehicleRow,
				vehicleRowMatches);
			if (vehicleRowMatches != 1 || !vehicleRow
				|| vehicleRow.m_sZoneId != snapshot.m_sZoneId
				|| vehicleRow.m_sRuntimeKind != snapshot.m_sRuntimeKind
				|| vehicleRow.m_sFactionKey != snapshot.m_sFactionKey)
			{
				evidence = string.Format(
					"runtime index %1 vehicle row identity is ambiguous or mismatched",
					runtimeIndex);
				return false;
			}
			snapshot.m_RuntimeVehicleRow = vehicleRow;
		}
		for (int helperIndex; helperIndex < m_aRuntimeHelperOwners.Count(); helperIndex++)
		{
			if (m_aRuntimeHelperOwners[helperIndex] == rootEntity)
			{
				snapshot.m_aHelperEntities.Insert(
					m_aRuntimeHelperEntities[helperIndex]);
				snapshot.m_aHelperRegistryIndices.Insert(helperIndex);
			}
		}
		evidence = snapshot.BuildEvidence();
		return true;
	}

	bool IsCampaignDebugAmbientRuntimeRootOwnedFormExact(
		HST_CampaignDebugAmbientRuntimeRootSnapshot snapshot,
		string ownedZoneId,
		string frozenZoneOwnerFactionKey,
		out string evidence)
	{
		evidence = "ambient owned-root form rejected";
		if (!snapshot || !snapshot.m_RootEntity
			|| snapshot.m_RootEntity.IsDeleted()
			|| snapshot.m_sZoneId != ownedZoneId
			|| snapshot.m_aHelperEntities.Count()
				!= snapshot.m_aHelperRegistryIndices.Count())
			return false;

		bool pedestrian = snapshot.m_sRuntimeKind == "CIV_CHARACTER";
		bool traffic
			= snapshot.m_sRuntimeKind == CIVILIAN_TRAFFIC_RUNTIME_KIND;
		bool civilianVehicle = snapshot.m_sRuntimeKind == "CIV_VEHICLE";
		bool militaryVehicle = snapshot.m_sRuntimeKind == "MILITARY_VEHICLE";
		if (!pedestrian && !traffic && !civilianVehicle && !militaryVehicle)
		{
			evidence = "ambient owned root has an unrecognized runtime kind";
			return false;
		}

		if (pedestrian)
		{
			bool pedestrianExact = snapshot.m_sFactionKey == CIVILIAN_FACTION_KEY
				&& snapshot.m_AmbientRecord
				&& snapshot.m_sActorKindId
					== HST_AmbientActorRuntimeService.KIND_PEDESTRIAN
				&& !snapshot.m_DriverEntity && snapshot.m_Group
				&& snapshot.m_aHelperEntities.Contains(snapshot.m_Group)
				&& IsExactCivilianFaction(snapshot.m_RootEntity)
				&& IsExactCivilianGroupMembership(
					snapshot.m_RootEntity,
					snapshot.m_Group)
				&& !snapshot.m_RuntimeVehicleRow
				&& snapshot.m_sVehicleRuntimeId.IsEmpty();
			if (!pedestrianExact)
			{
				evidence = "ambient pedestrian root lacks exact CIV actor/group authority";
				return false;
			}
		}
		else if (traffic)
		{
			bool trafficExact = snapshot.m_sFactionKey == CIVILIAN_FACTION_KEY
				&& snapshot.m_AmbientRecord
				&& snapshot.m_sActorKindId
					== HST_AmbientActorRuntimeService.KIND_TRAFFIC
				&& snapshot.m_DriverEntity && snapshot.m_Group
				&& snapshot.m_aHelperEntities.Contains(snapshot.m_DriverEntity)
				&& snapshot.m_aHelperEntities.Contains(snapshot.m_Group)
				&& IsExactCivilianFaction(snapshot.m_DriverEntity)
				&& IsExactCivilianGroupMembership(
					snapshot.m_DriverEntity,
					snapshot.m_Group)
				&& IsExactAmbientTrafficDriverSeated(snapshot.m_AmbientRecord)
				&& snapshot.m_RuntimeVehicleRow
				&& snapshot.m_RuntimeVehicleRow.m_sFactionKey
					== CIVILIAN_FACTION_KEY;
			if (!trafficExact)
			{
				evidence = "ambient traffic root lacks exact CIV driver/group/seating authority";
				return false;
			}
		}
		else
		{
			string expectedFactionKey = CIVILIAN_FACTION_KEY;
			if (militaryVehicle)
				expectedFactionKey = frozenZoneOwnerFactionKey;
			bool staticVehicleExact = !expectedFactionKey.IsEmpty()
				&& snapshot.m_sFactionKey == expectedFactionKey
				&& !snapshot.m_AmbientRecord
				&& !snapshot.m_DriverEntity && !snapshot.m_Group
				&& snapshot.m_aHelperEntities.IsEmpty()
				&& snapshot.m_RuntimeVehicleRow
				&& snapshot.m_RuntimeVehicleRow.m_sFactionKey
					== expectedFactionKey
				&& !HasPlayerOccupant(snapshot.m_RootEntity);
			if (!staticVehicleExact)
			{
				evidence = "ambient static vehicle root lacks exact frozen faction and unclaimed authority";
				return false;
			}
		}
		evidence = "ambient owned-root kind, faction, actor, group, driver, and vehicle form is exact";
		return true;
	}

	// Captures every Civilian-service runtime registry, not just the selected
	// town. A caller can therefore compute a true global post-minus-pre owner
	// set while preserving unrelated baseline roots and runtime zones.
	bool CaptureCampaignDebugAmbientRuntimeRoots(
		HST_CampaignState state,
		array<ref HST_CampaignDebugAmbientRuntimeRootSnapshot> snapshots,
		array<string> runtimeZoneIds,
		out string evidence)
	{
		evidence = "ambient runtime snapshot unavailable";
		if (!state || !snapshots || !runtimeZoneIds || !m_AmbientRuntime)
			return false;
		snapshots.Clear();
		runtimeZoneIds.Clear();
		if (m_aRuntimeHelperOwners.Count() != m_aRuntimeHelperEntities.Count())
		{
			evidence = "ambient helper owner/entity registries are misaligned";
			return false;
		}

		foreach (string runtimeZoneId : m_aRuntimeZoneIds)
		{
			if (runtimeZoneId.IsEmpty() || runtimeZoneIds.Contains(runtimeZoneId))
			{
				evidence = "ambient runtime-zone registry contains an empty or duplicate identity";
				return false;
			}
			runtimeZoneIds.Insert(runtimeZoneId);
		}
		for (int rootIndex; rootIndex < m_aRuntimeEntities.Count(); rootIndex++)
		{
			HST_CampaignDebugAmbientRuntimeRootSnapshot snapshot;
			string rootEvidence;
			if (!BuildCampaignDebugAmbientRootSnapshot(
				state,
				rootIndex,
				snapshot,
				rootEvidence))
			{
				evidence = rootEvidence;
				return false;
			}
			if (!runtimeZoneIds.Contains(snapshot.m_sZoneId))
			{
				evidence = "ambient root references a zone absent from the runtime-zone registry";
				return false;
			}
			foreach (HST_CampaignDebugAmbientRuntimeRootSnapshot existing : snapshots)
			{
				if (snapshot.CollidesWithImmutableIdentity(existing))
				{
					evidence = "ambient root snapshot contains a duplicate pointer or stable identity";
					return false;
				}
			}
			snapshots.Insert(snapshot);
		}

		foreach (HST_AmbientActorRuntimeRecord actorRecord : m_aAmbientActorRecords)
		{
			if (!actorRecord || !actorRecord.m_RootEntity
				|| actorRecord.m_sRuntimeId.IsEmpty())
			{
				evidence = "ambient actor registry contains an unbound identity";
				return false;
			}
			int exactActorMatches;
			foreach (HST_CampaignDebugAmbientRuntimeRootSnapshot rootSnapshot : snapshots)
			{
				if (rootSnapshot.m_AmbientRecord == actorRecord)
					exactActorMatches++;
				else if (!rootSnapshot.m_sActorRuntimeId.IsEmpty()
					&& rootSnapshot.m_sActorRuntimeId == actorRecord.m_sRuntimeId)
				{
					evidence = "ambient actor runtime ID aliases a different root";
					return false;
				}
			}
			if (exactActorMatches != 1)
			{
				evidence = "ambient actor registry is not one-to-one with runtime roots";
				return false;
			}
		}
		array<IEntity> capturedHelperEntities = {};
		for (int helperIndex; helperIndex < m_aRuntimeHelperOwners.Count(); helperIndex++)
		{
			IEntity helperOwner = m_aRuntimeHelperOwners[helperIndex];
			IEntity helperEntity = m_aRuntimeHelperEntities[helperIndex];
			if (!helperOwner || !helperEntity || helperEntity.IsDeleted())
			{
				evidence = "ambient helper registry contains a missing exact owner or helper";
				return false;
			}
			if (helperOwner == helperEntity
				|| m_aRuntimeEntities.Contains(helperEntity)
				|| capturedHelperEntities.Contains(helperEntity))
			{
				evidence = "ambient helper identity is shared, duplicated, or aliases a runtime root";
				return false;
			}
			bool ownerFound;
			foreach (HST_CampaignDebugAmbientRuntimeRootSnapshot ownerSnapshot : snapshots)
			{
				if (ownerSnapshot.m_RootEntity == helperOwner)
				{
					ownerFound = true;
					break;
				}
			}
			if (!ownerFound)
			{
				evidence = "ambient helper owner is absent from the runtime-root registry";
				return false;
			}
			capturedHelperEntities.Insert(helperEntity);
		}
		evidence = string.Format(
			"exact ambient snapshot | roots %1 | zones %2 | actors %3 | helpers %4",
			snapshots.Count(),
			runtimeZoneIds.Count(),
			m_aAmbientActorRecords.Count(),
			m_aRuntimeHelperEntities.Count());
		return true;
	}

	bool CaptureCampaignDebugAmbientMutableRegistries(
		out HST_CampaignDebugAmbientMutableRegistrySnapshot snapshot,
		out string evidence)
	{
		snapshot = null;
		evidence = "ambient mutable-registry snapshot rejected";
		int retryCount = m_aAmbientRetryZoneIds.Count();
		int staticCount = m_aStaticVehicleInitializationZoneIds.Count();
		int panicCount = m_aCivilianPanicThreatZoneIds.Count();
		if (retryCount != m_aAmbientRetryKinds.Count()
			|| retryCount != m_aAmbientRetrySeconds.Count()
			|| staticCount != m_aStaticCivilianVehicleSlotsCompleted.Count()
			|| staticCount != m_aStaticMilitaryVehicleSlotsCompleted.Count()
			|| staticCount != m_aStaticMilitaryInitializationOwnerKeys.Count()
			|| !HasExactPendingCivilianCasualtyArrays()
			|| !HasExactPendingCivilianTheftArrays()
			|| panicCount != m_aCivilianPanicThreatPositions.Count())
		{
			evidence = "one or more ambient mutable parallel registries are misaligned";
			return false;
		}

		snapshot = new HST_CampaignDebugAmbientMutableRegistrySnapshot();
		snapshot.m_bConsequenceAuthorityFault
			= m_bPendingCivilianConsequenceAuthorityFault;
		array<string> retryIdentities = {};
		for (int retryIndex; retryIndex < retryCount; retryIndex++)
		{
			string retryIdentity = m_aAmbientRetryZoneIds[retryIndex]
				+ "|" + m_aAmbientRetryKinds[retryIndex];
			if (m_aAmbientRetryZoneIds[retryIndex].IsEmpty()
				|| m_aAmbientRetryKinds[retryIndex].IsEmpty()
				|| retryIdentities.Contains(retryIdentity))
			{
				evidence = "ambient retry registry contains an empty or duplicate identity";
				return false;
			}
			retryIdentities.Insert(retryIdentity);
			snapshot.m_aRetryZoneIds.Insert(m_aAmbientRetryZoneIds[retryIndex]);
			snapshot.m_aRetryKinds.Insert(m_aAmbientRetryKinds[retryIndex]);
			snapshot.m_aRetrySeconds.Insert(m_aAmbientRetrySeconds[retryIndex]);
		}
		for (int staticIndex; staticIndex < staticCount; staticIndex++)
		{
			string staticZoneId
				= m_aStaticVehicleInitializationZoneIds[staticIndex];
			if (staticZoneId.IsEmpty()
				|| snapshot.m_aStaticZoneIds.Contains(staticZoneId))
			{
				evidence = "static initialization registry contains an empty or duplicate zone";
				return false;
			}
			snapshot.m_aStaticZoneIds.Insert(staticZoneId);
			snapshot.m_aStaticCivilianSlots.Insert(
				m_aStaticCivilianVehicleSlotsCompleted[staticIndex]);
			snapshot.m_aStaticMilitarySlots.Insert(
				m_aStaticMilitaryVehicleSlotsCompleted[staticIndex]);
			snapshot.m_aStaticMilitaryOwnerKeys.Insert(
				m_aStaticMilitaryInitializationOwnerKeys[staticIndex]);
		}
		for (int casualtyIndex; casualtyIndex < m_aPendingCivilianCasualtyEventIds.Count(); casualtyIndex++)
		{
			string casualtyEventId
				= m_aPendingCivilianCasualtyEventIds[casualtyIndex];
			if (casualtyEventId.IsEmpty()
				|| snapshot.m_aCasualtyEventIds.Contains(casualtyEventId))
			{
				evidence = "pending casualty registry contains an empty or duplicate event";
				return false;
			}
			snapshot.m_aCasualtyZoneIds.Insert(
				m_aPendingCivilianCasualtyZoneIds[casualtyIndex]);
			snapshot.m_aCasualtyEventIds.Insert(casualtyEventId);
			snapshot.m_aCasualtyFactionKeys.Insert(
				m_aPendingCivilianCasualtyFactionKeys[casualtyIndex]);
			snapshot.m_aCasualtySourceIds.Insert(
				m_aPendingCivilianCasualtySourceIds[casualtyIndex]);
			snapshot.m_aCasualtyPositions.Insert(
				m_aPendingCivilianCasualtyPositions[casualtyIndex]);
			snapshot.m_aCasualtyAttempts.Insert(
				m_aPendingCivilianCasualtyAttempts[casualtyIndex]);
			snapshot.m_aCasualtyRetrySeconds.Insert(
				m_aPendingCivilianCasualtyRetrySeconds[casualtyIndex]);
		}
		for (int theftIndex; theftIndex < m_aPendingCivilianTheftEventIds.Count(); theftIndex++)
		{
			string theftEventId = m_aPendingCivilianTheftEventIds[theftIndex];
			if (theftEventId.IsEmpty()
				|| snapshot.m_aTheftEventIds.Contains(theftEventId))
			{
				evidence = "pending theft registry contains an empty or duplicate event";
				return false;
			}
			snapshot.m_aTheftZoneIds.Insert(
				m_aPendingCivilianTheftZoneIds[theftIndex]);
			snapshot.m_aTheftEventIds.Insert(theftEventId);
			snapshot.m_aTheftFactionKeys.Insert(
				m_aPendingCivilianTheftFactionKeys[theftIndex]);
			snapshot.m_aTheftSourceIds.Insert(
				m_aPendingCivilianTheftSourceIds[theftIndex]);
			snapshot.m_aTheftAttempts.Insert(
				m_aPendingCivilianTheftAttempts[theftIndex]);
			snapshot.m_aTheftRetrySeconds.Insert(
				m_aPendingCivilianTheftRetrySeconds[theftIndex]);
		}
		for (int panicIndex; panicIndex < panicCount; panicIndex++)
		{
			string panicZoneId = m_aCivilianPanicThreatZoneIds[panicIndex];
			if (panicZoneId.IsEmpty()
				|| snapshot.m_aPanicZoneIds.Contains(panicZoneId))
			{
				evidence = "civilian panic registry contains an empty or duplicate zone";
				return false;
			}
			snapshot.m_aPanicZoneIds.Insert(panicZoneId);
			snapshot.m_aPanicPositions.Insert(
				m_aCivilianPanicThreatPositions[panicIndex]);
		}
		foreach (HST_RuntimeVehicleState preservedVehicle : m_aResetPreservedPlayerVehicles)
		{
			if (!preservedVehicle
				|| snapshot.m_aResetPreservedVehicles.Contains(preservedVehicle))
			{
				evidence = "reset-preserved vehicle registry contains a null or duplicate pointer";
				return false;
			}
			snapshot.m_aResetPreservedVehicles.Insert(preservedVehicle);
		}
		foreach (HST_VehicleCargoItemState preservedCargo : m_aResetPreservedPlayerVehicleCargo)
		{
			if (!preservedCargo
				|| snapshot.m_aResetPreservedCargo.Contains(preservedCargo))
			{
				evidence = "reset-preserved cargo registry contains a null or duplicate pointer";
				return false;
			}
			snapshot.m_aResetPreservedCargo.Insert(preservedCargo);
		}
		evidence = "exact mutable registries | " + snapshot.BuildEvidence();
		return true;
	}

	bool BuildCampaignDebugOwnedAmbientMutableRegistries(
		HST_CampaignDebugAmbientMutableRegistrySnapshot baseline,
		HST_CampaignDebugAmbientMutableRegistrySnapshot current,
		out HST_CampaignDebugAmbientMutableRegistrySnapshot ownedSnapshot,
		out string evidence)
	{
		ownedSnapshot = null;
		evidence = "mutable-registry post-minus-pre classification rejected";
		if (!baseline || !current)
			return false;
		ownedSnapshot = new HST_CampaignDebugAmbientMutableRegistrySnapshot();
		if (current.m_bConsequenceAuthorityFault
			!= baseline.m_bConsequenceAuthorityFault)
		{
			evidence = "sticky civilian consequence authority fault changed during the owned window";
			return false;
		}
		ownedSnapshot.m_bConsequenceAuthorityFault
			= baseline.m_bConsequenceAuthorityFault;
		bool mutableRegistryLostBaseline;
		if (current.m_aRetryZoneIds.Count() < baseline.m_aRetryZoneIds.Count())
			mutableRegistryLostBaseline = true;
		if (current.m_aStaticZoneIds.Count() < baseline.m_aStaticZoneIds.Count())
			mutableRegistryLostBaseline = true;
		if (current.m_aCasualtyEventIds.Count() < baseline.m_aCasualtyEventIds.Count())
			mutableRegistryLostBaseline = true;
		if (current.m_aTheftEventIds.Count() < baseline.m_aTheftEventIds.Count())
			mutableRegistryLostBaseline = true;
		if (current.m_aPanicZoneIds.Count() < baseline.m_aPanicZoneIds.Count())
			mutableRegistryLostBaseline = true;
		if (current.m_aResetPreservedVehicles.Count()
			< baseline.m_aResetPreservedVehicles.Count())
			mutableRegistryLostBaseline = true;
		if (current.m_aResetPreservedCargo.Count()
			< baseline.m_aResetPreservedCargo.Count())
			mutableRegistryLostBaseline = true;
		if (mutableRegistryLostBaseline)
		{
			evidence = "one or more mutable registries lost a frozen baseline entry";
			return false;
		}
		for (int retryIndex; retryIndex < baseline.m_aRetryZoneIds.Count(); retryIndex++)
		{
			if (current.m_aRetryZoneIds[retryIndex] != baseline.m_aRetryZoneIds[retryIndex]
				|| current.m_aRetryKinds[retryIndex] != baseline.m_aRetryKinds[retryIndex]
				|| current.m_aRetrySeconds[retryIndex] != baseline.m_aRetrySeconds[retryIndex])
			{
				evidence = "ambient retry baseline identity, content, or order changed";
				return false;
			}
		}
		for (int retryOwned = baseline.m_aRetryZoneIds.Count(); retryOwned < current.m_aRetryZoneIds.Count(); retryOwned++)
		{
			ownedSnapshot.m_aRetryZoneIds.Insert(current.m_aRetryZoneIds[retryOwned]);
			ownedSnapshot.m_aRetryKinds.Insert(current.m_aRetryKinds[retryOwned]);
			ownedSnapshot.m_aRetrySeconds.Insert(current.m_aRetrySeconds[retryOwned]);
		}
		for (int staticIndex; staticIndex < baseline.m_aStaticZoneIds.Count(); staticIndex++)
		{
			if (current.m_aStaticZoneIds[staticIndex] != baseline.m_aStaticZoneIds[staticIndex]
				|| current.m_aStaticCivilianSlots[staticIndex] != baseline.m_aStaticCivilianSlots[staticIndex]
				|| current.m_aStaticMilitarySlots[staticIndex] != baseline.m_aStaticMilitarySlots[staticIndex]
				|| current.m_aStaticMilitaryOwnerKeys[staticIndex]
					!= baseline.m_aStaticMilitaryOwnerKeys[staticIndex])
			{
				evidence = "static initialization baseline identity, content, or order changed";
				return false;
			}
		}
		for (int staticOwned = baseline.m_aStaticZoneIds.Count(); staticOwned < current.m_aStaticZoneIds.Count(); staticOwned++)
		{
			ownedSnapshot.m_aStaticZoneIds.Insert(current.m_aStaticZoneIds[staticOwned]);
			ownedSnapshot.m_aStaticCivilianSlots.Insert(current.m_aStaticCivilianSlots[staticOwned]);
			ownedSnapshot.m_aStaticMilitarySlots.Insert(current.m_aStaticMilitarySlots[staticOwned]);
			ownedSnapshot.m_aStaticMilitaryOwnerKeys.Insert(current.m_aStaticMilitaryOwnerKeys[staticOwned]);
		}
		for (int casualtyIndex; casualtyIndex < baseline.m_aCasualtyEventIds.Count(); casualtyIndex++)
		{
			bool casualtyBaselineExact = true;
			if (current.m_aCasualtyZoneIds[casualtyIndex]
				!= baseline.m_aCasualtyZoneIds[casualtyIndex])
				casualtyBaselineExact = false;
			if (current.m_aCasualtyEventIds[casualtyIndex]
				!= baseline.m_aCasualtyEventIds[casualtyIndex])
				casualtyBaselineExact = false;
			if (current.m_aCasualtyFactionKeys[casualtyIndex]
				!= baseline.m_aCasualtyFactionKeys[casualtyIndex])
				casualtyBaselineExact = false;
			if (current.m_aCasualtySourceIds[casualtyIndex]
				!= baseline.m_aCasualtySourceIds[casualtyIndex])
				casualtyBaselineExact = false;
			if (current.m_aCasualtyPositions[casualtyIndex]
				!= baseline.m_aCasualtyPositions[casualtyIndex])
				casualtyBaselineExact = false;
			if (current.m_aCasualtyAttempts[casualtyIndex]
				!= baseline.m_aCasualtyAttempts[casualtyIndex])
				casualtyBaselineExact = false;
			if (current.m_aCasualtyRetrySeconds[casualtyIndex]
				!= baseline.m_aCasualtyRetrySeconds[casualtyIndex])
				casualtyBaselineExact = false;
			if (!casualtyBaselineExact)
			{
				evidence = "pending casualty baseline identity, content, or order changed";
				return false;
			}
		}
		for (int casualtyOwned = baseline.m_aCasualtyEventIds.Count(); casualtyOwned < current.m_aCasualtyEventIds.Count(); casualtyOwned++)
		{
			ownedSnapshot.m_aCasualtyZoneIds.Insert(current.m_aCasualtyZoneIds[casualtyOwned]);
			ownedSnapshot.m_aCasualtyEventIds.Insert(current.m_aCasualtyEventIds[casualtyOwned]);
			ownedSnapshot.m_aCasualtyFactionKeys.Insert(current.m_aCasualtyFactionKeys[casualtyOwned]);
			ownedSnapshot.m_aCasualtySourceIds.Insert(current.m_aCasualtySourceIds[casualtyOwned]);
			ownedSnapshot.m_aCasualtyPositions.Insert(current.m_aCasualtyPositions[casualtyOwned]);
			ownedSnapshot.m_aCasualtyAttempts.Insert(current.m_aCasualtyAttempts[casualtyOwned]);
			ownedSnapshot.m_aCasualtyRetrySeconds.Insert(current.m_aCasualtyRetrySeconds[casualtyOwned]);
		}
		for (int theftIndex; theftIndex < baseline.m_aTheftEventIds.Count(); theftIndex++)
		{
			bool theftBaselineExact = true;
			if (current.m_aTheftZoneIds[theftIndex]
				!= baseline.m_aTheftZoneIds[theftIndex])
				theftBaselineExact = false;
			if (current.m_aTheftEventIds[theftIndex]
				!= baseline.m_aTheftEventIds[theftIndex])
				theftBaselineExact = false;
			if (current.m_aTheftFactionKeys[theftIndex]
				!= baseline.m_aTheftFactionKeys[theftIndex])
				theftBaselineExact = false;
			if (current.m_aTheftSourceIds[theftIndex]
				!= baseline.m_aTheftSourceIds[theftIndex])
				theftBaselineExact = false;
			if (current.m_aTheftAttempts[theftIndex]
				!= baseline.m_aTheftAttempts[theftIndex])
				theftBaselineExact = false;
			if (current.m_aTheftRetrySeconds[theftIndex]
				!= baseline.m_aTheftRetrySeconds[theftIndex])
				theftBaselineExact = false;
			if (!theftBaselineExact)
			{
				evidence = "pending theft baseline identity, content, or order changed";
				return false;
			}
		}
		for (int theftOwned = baseline.m_aTheftEventIds.Count(); theftOwned < current.m_aTheftEventIds.Count(); theftOwned++)
		{
			ownedSnapshot.m_aTheftZoneIds.Insert(current.m_aTheftZoneIds[theftOwned]);
			ownedSnapshot.m_aTheftEventIds.Insert(current.m_aTheftEventIds[theftOwned]);
			ownedSnapshot.m_aTheftFactionKeys.Insert(current.m_aTheftFactionKeys[theftOwned]);
			ownedSnapshot.m_aTheftSourceIds.Insert(current.m_aTheftSourceIds[theftOwned]);
			ownedSnapshot.m_aTheftAttempts.Insert(current.m_aTheftAttempts[theftOwned]);
			ownedSnapshot.m_aTheftRetrySeconds.Insert(current.m_aTheftRetrySeconds[theftOwned]);
		}
		for (int panicIndex; panicIndex < baseline.m_aPanicZoneIds.Count(); panicIndex++)
		{
			if (current.m_aPanicZoneIds[panicIndex] != baseline.m_aPanicZoneIds[panicIndex]
				|| current.m_aPanicPositions[panicIndex] != baseline.m_aPanicPositions[panicIndex])
			{
				evidence = "civilian panic baseline identity, content, or order changed";
				return false;
			}
		}
		for (int panicOwned = baseline.m_aPanicZoneIds.Count(); panicOwned < current.m_aPanicZoneIds.Count(); panicOwned++)
		{
			ownedSnapshot.m_aPanicZoneIds.Insert(current.m_aPanicZoneIds[panicOwned]);
			ownedSnapshot.m_aPanicPositions.Insert(current.m_aPanicPositions[panicOwned]);
		}
		for (int vehicleIndex; vehicleIndex < baseline.m_aResetPreservedVehicles.Count(); vehicleIndex++)
		{
			if (current.m_aResetPreservedVehicles[vehicleIndex]
				!= baseline.m_aResetPreservedVehicles[vehicleIndex])
			{
				evidence = "reset-preserved vehicle baseline pointer or order changed";
				return false;
			}
		}
		for (int vehicleOwned = baseline.m_aResetPreservedVehicles.Count(); vehicleOwned < current.m_aResetPreservedVehicles.Count(); vehicleOwned++)
		{
			ownedSnapshot.m_aResetPreservedVehicles.Insert(
				current.m_aResetPreservedVehicles[vehicleOwned]);
		}
		for (int cargoIndex; cargoIndex < baseline.m_aResetPreservedCargo.Count(); cargoIndex++)
		{
			if (current.m_aResetPreservedCargo[cargoIndex]
				!= baseline.m_aResetPreservedCargo[cargoIndex])
			{
				evidence = "reset-preserved cargo baseline pointer or order changed";
				return false;
			}
		}
		for (int cargoOwned = baseline.m_aResetPreservedCargo.Count(); cargoOwned < current.m_aResetPreservedCargo.Count(); cargoOwned++)
		{
			ownedSnapshot.m_aResetPreservedCargo.Insert(
				current.m_aResetPreservedCargo[cargoOwned]);
		}
		evidence = "exact mutable post-minus-pre | " + ownedSnapshot.BuildEvidence();
		return true;
	}

	bool AreCampaignDebugAmbientMutableRegistriesExact(
		HST_CampaignDebugAmbientMutableRegistrySnapshot expected,
		HST_CampaignDebugAmbientMutableRegistrySnapshot current,
		out string evidence)
	{
		HST_CampaignDebugAmbientMutableRegistrySnapshot residue;
		if (!BuildCampaignDebugOwnedAmbientMutableRegistries(
			expected,
			current,
			residue,
			evidence))
			return false;
		bool exact = residue.m_aRetryZoneIds.IsEmpty()
			&& residue.m_aStaticZoneIds.IsEmpty()
			&& residue.m_aCasualtyEventIds.IsEmpty()
			&& residue.m_aTheftEventIds.IsEmpty()
			&& residue.m_aPanicZoneIds.IsEmpty()
			&& residue.m_aResetPreservedVehicles.IsEmpty()
			&& residue.m_aResetPreservedCargo.IsEmpty();
		if (!exact)
			evidence = "mutable registry exact comparison found post-expected residue";
		return exact;
	}

	bool CleanupCampaignDebugOwnedAmbientMutableRegistries(
		HST_CampaignDebugAmbientMutableRegistrySnapshot baseline,
		HST_CampaignDebugAmbientMutableRegistrySnapshot ownedSnapshot,
		out string evidence)
	{
		evidence = "exact mutable-registry cleanup rejected";
		HST_CampaignDebugAmbientMutableRegistrySnapshot current;
		string currentEvidence;
		if (!CaptureCampaignDebugAmbientMutableRegistries(current, currentEvidence))
		{
			evidence = currentEvidence;
			return false;
		}
		if (AreCampaignDebugAmbientMutableRegistriesExact(
			baseline,
			current,
			currentEvidence))
		{
			evidence = "exact mutable registries already match the frozen baseline";
			return true;
		}
		HST_CampaignDebugAmbientMutableRegistrySnapshot currentOwned;
		if (!BuildCampaignDebugOwnedAmbientMutableRegistries(
			baseline,
			current,
			currentOwned,
			currentEvidence)
			|| !AreCampaignDebugAmbientMutableRegistriesExact(
				ownedSnapshot,
				currentOwned,
				currentEvidence))
		{
			evidence = currentEvidence;
			return false;
		}

		for (int retryIndex = ownedSnapshot.m_aRetryZoneIds.Count() - 1; retryIndex >= 0; retryIndex--)
		{
			m_aAmbientRetryZoneIds.Remove(m_aAmbientRetryZoneIds.Count() - 1);
			m_aAmbientRetryKinds.Remove(m_aAmbientRetryKinds.Count() - 1);
			m_aAmbientRetrySeconds.Remove(m_aAmbientRetrySeconds.Count() - 1);
		}
		for (int staticIndex = ownedSnapshot.m_aStaticZoneIds.Count() - 1; staticIndex >= 0; staticIndex--)
		{
			m_aStaticVehicleInitializationZoneIds.Remove(m_aStaticVehicleInitializationZoneIds.Count() - 1);
			m_aStaticCivilianVehicleSlotsCompleted.Remove(m_aStaticCivilianVehicleSlotsCompleted.Count() - 1);
			m_aStaticMilitaryVehicleSlotsCompleted.Remove(m_aStaticMilitaryVehicleSlotsCompleted.Count() - 1);
			m_aStaticMilitaryInitializationOwnerKeys.Remove(m_aStaticMilitaryInitializationOwnerKeys.Count() - 1);
		}
		for (int casualtyIndex = ownedSnapshot.m_aCasualtyEventIds.Count() - 1; casualtyIndex >= 0; casualtyIndex--)
			RemovePendingCivilianCasualtyAt(m_aPendingCivilianCasualtyEventIds.Count() - 1);
		for (int theftIndex = ownedSnapshot.m_aTheftEventIds.Count() - 1; theftIndex >= 0; theftIndex--)
			RemovePendingCivilianTheftAt(m_aPendingCivilianTheftEventIds.Count() - 1);
		for (int panicIndex = ownedSnapshot.m_aPanicZoneIds.Count() - 1; panicIndex >= 0; panicIndex--)
		{
			m_aCivilianPanicThreatZoneIds.Remove(m_aCivilianPanicThreatZoneIds.Count() - 1);
			m_aCivilianPanicThreatPositions.Remove(m_aCivilianPanicThreatPositions.Count() - 1);
		}
		for (int vehicleIndex = ownedSnapshot.m_aResetPreservedVehicles.Count() - 1; vehicleIndex >= 0; vehicleIndex--)
			m_aResetPreservedPlayerVehicles.Remove(m_aResetPreservedPlayerVehicles.Count() - 1);
		for (int cargoIndex = ownedSnapshot.m_aResetPreservedCargo.Count() - 1; cargoIndex >= 0; cargoIndex--)
			m_aResetPreservedPlayerVehicleCargo.Remove(m_aResetPreservedPlayerVehicleCargo.Count() - 1);

		HST_CampaignDebugAmbientMutableRegistrySnapshot finalSnapshot;
		if (!CaptureCampaignDebugAmbientMutableRegistries(
			finalSnapshot,
			currentEvidence)
			|| !AreCampaignDebugAmbientMutableRegistriesExact(
				baseline,
				finalSnapshot,
				currentEvidence))
		{
			evidence = currentEvidence;
			return false;
		}
		evidence = "exact mutable registries restored | " + baseline.BuildEvidence();
		return true;
	}

	protected bool AreCampaignDebugOwnedAmbientHelpersExclusive(
		HST_CampaignDebugAmbientRuntimeRootSnapshot expected,
		bool requireLive,
		out string evidence)
	{
		evidence = "owned ambient helper exclusivity rejected";
		if (!expected || !expected.m_RootEntity
			|| m_aRuntimeHelperOwners.Count() != m_aRuntimeHelperEntities.Count()
			|| expected.m_aHelperEntities.Count()
				!= expected.m_aHelperRegistryIndices.Count())
			return false;
		int ownedHelperCount;
		for (int helperIndex; helperIndex < m_aRuntimeHelperEntities.Count(); helperIndex++)
		{
			IEntity helperOwner = m_aRuntimeHelperOwners[helperIndex];
			IEntity helperEntity = m_aRuntimeHelperEntities[helperIndex];
			bool expectedEntity = expected.m_aHelperEntities.Contains(helperEntity);
			if (helperOwner != expected.m_RootEntity)
			{
				if (expectedEntity)
				{
					evidence = "owned ambient helper pointer is shared with another owner";
					return false;
				}
				continue;
			}
			if (ownedHelperCount >= expected.m_aHelperEntities.Count()
				|| helperIndex
					!= expected.m_aHelperRegistryIndices[ownedHelperCount]
				|| helperEntity != expected.m_aHelperEntities[ownedHelperCount]
				|| helperEntity == expected.m_RootEntity
				|| m_aRuntimeEntities.Contains(helperEntity)
				|| (requireLive && (!helperEntity || helperEntity.IsDeleted()
					|| IsPlayerControlledEntity(helperEntity))))
			{
				evidence = "owned ambient helper pointer, order, liveness, or exclusivity changed";
				return false;
			}
			ownedHelperCount++;
		}
		if (ownedHelperCount != expected.m_aHelperEntities.Count())
			return false;
		evidence = "owned ambient helper tuples remain exact and exclusive";
		return true;
	}

	protected bool IsCampaignDebugOwnedAmbientResidueAbsent(
		HST_CampaignState state,
		HST_CampaignDebugAmbientRuntimeRootSnapshot expected,
		out string evidence)
	{
		evidence = "owned ambient residue remains";
		if (!state || !expected)
			return false;
		for (int rootIndex; rootIndex < m_aRuntimeEntities.Count(); rootIndex++)
		{
			if (m_aRuntimeEntities[rootIndex] == expected.m_RootEntity
				|| (!expected.m_sVehicleRuntimeId.IsEmpty()
					&& rootIndex < m_aRuntimeEntityVehicleIds.Count()
					&& m_aRuntimeEntityVehicleIds[rootIndex]
						== expected.m_sVehicleRuntimeId))
				return false;
		}
		foreach (HST_AmbientActorRuntimeRecord record : m_aAmbientActorRecords)
		{
			if (record == expected.m_AmbientRecord
				|| (record && !expected.m_sActorRuntimeId.IsEmpty()
					&& record.m_sRuntimeId == expected.m_sActorRuntimeId))
				return false;
		}
		for (int helperIndex; helperIndex < m_aRuntimeHelperOwners.Count(); helperIndex++)
		{
			if (m_aRuntimeHelperOwners[helperIndex] == expected.m_RootEntity
				|| expected.m_aHelperEntities.Contains(
					m_aRuntimeHelperEntities[helperIndex]))
				return false;
		}
		foreach (HST_RuntimeVehicleState row : state.m_aRuntimeVehicles)
		{
			if (row == expected.m_RuntimeVehicleRow
				|| (row && !expected.m_sVehicleRuntimeId.IsEmpty()
					&& row.m_sVehicleRuntimeId == expected.m_sVehicleRuntimeId))
				return false;
		}
		evidence = "owned ambient root already absent with no pointer, helper, actor, or vehicle residue";
		return true;
	}

	bool ValidateCampaignDebugOwnedAmbientCleanupSet(
		HST_CampaignState state,
		array<ref HST_CampaignDebugAmbientRuntimeRootSnapshot> ownedRoots,
		array<string> ownedZoneIds,
		out string evidence)
	{
		evidence = "owned ambient cleanup-set preflight rejected";
		if (HasCampaignDebugCivilianMutationForeignClaim(state, evidence))
			return false;
		int runtimeCount = m_aRuntimeEntities.Count();
		if (!state || !ownedRoots || !ownedZoneIds
			|| m_aRuntimeEntityZoneIds.Count() != runtimeCount
			|| m_aRuntimeEntityKinds.Count() != runtimeCount
			|| m_aRuntimeEntityFactionKeys.Count() != runtimeCount
			|| m_aRuntimeEntityVehicleIds.Count() != runtimeCount
			|| m_aRuntimeEntitySpawnPositions.Count() != runtimeCount
			|| m_aRuntimeHelperOwners.Count() != m_aRuntimeHelperEntities.Count())
			return false;
		for (int runtimeIndex; runtimeIndex < runtimeCount; runtimeIndex++)
		{
			IEntity currentRoot = m_aRuntimeEntities[runtimeIndex];
			HST_CampaignDebugAmbientRuntimeRootSnapshot exactOwnedRoot;
			int exactOwnedMatches;
			foreach (HST_CampaignDebugAmbientRuntimeRootSnapshot ownedRoot : ownedRoots)
			{
				if (ownedRoot && ownedRoot.m_RootEntity == currentRoot)
				{
					exactOwnedRoot = ownedRoot;
					exactOwnedMatches++;
				}
				else if (ownedRoot && !ownedRoot.m_sVehicleRuntimeId.IsEmpty()
					&& m_aRuntimeEntityVehicleIds[runtimeIndex]
						== ownedRoot.m_sVehicleRuntimeId)
				{
					evidence = "current ambient cleanup root aliases a frozen vehicle identity";
					return false;
				}
			}
			if (exactOwnedMatches != 1 || !exactOwnedRoot
				|| runtimeIndex != exactOwnedRoot.m_iRuntimeRegistryIndex
				|| m_aRuntimeEntityZoneIds[runtimeIndex]
					!= exactOwnedRoot.m_sZoneId
				|| m_aRuntimeEntityKinds[runtimeIndex]
					!= exactOwnedRoot.m_sRuntimeKind
				|| m_aRuntimeEntityFactionKeys[runtimeIndex]
					!= exactOwnedRoot.m_sFactionKey
				|| m_aRuntimeEntityVehicleIds[runtimeIndex]
					!= exactOwnedRoot.m_sVehicleRuntimeId
				|| m_aRuntimeEntitySpawnPositions[runtimeIndex]
					!= exactOwnedRoot.m_vSpawnPosition)
			{
				evidence = "current ambient cleanup root pointer, order, or immutable parallel row is not frozen-owned";
				return false;
			}
			if (currentRoot && currentRoot.IsDeleted())
			{
				if (!exactOwnedRoot.m_bCleanupDeleteRequested)
					return false;
				continue;
			}
			HST_CampaignDebugAmbientRuntimeRootSnapshot currentSnapshot;
			string currentEvidence;
			if (!BuildCampaignDebugAmbientRootSnapshot(
				state,
				runtimeIndex,
				currentSnapshot,
				currentEvidence)
				|| !currentSnapshot.HasSameImmutableIdentity(exactOwnedRoot))
			{
				evidence = currentEvidence;
				return false;
			}
		}
		foreach (HST_CampaignDebugAmbientRuntimeRootSnapshot expectedRoot : ownedRoots)
		{
			if (!expectedRoot)
				return false;
			int currentMatches;
			foreach (IEntity currentEntity : m_aRuntimeEntities)
			{
				if (currentEntity == expectedRoot.m_RootEntity)
					currentMatches++;
			}
			if (currentMatches == 1)
				continue;
			string absenceEvidence;
			if (currentMatches != 0
				|| !expectedRoot.m_bCleanupDeleteRequested
				|| !expectedRoot.m_RootEntity
				|| !expectedRoot.m_RootEntity.IsDeleted()
				|| !IsCampaignDebugOwnedAmbientResidueAbsent(
					state,
					expectedRoot,
					absenceEvidence))
			{
				evidence = "frozen owned ambient root is neither exact-current nor acknowledged absent | "
					+ absenceEvidence;
				return false;
			}
		}
		array<IEntity> seenHelpers = {};
		for (int helperIndex; helperIndex < m_aRuntimeHelperEntities.Count(); helperIndex++)
		{
			IEntity helperOwner = m_aRuntimeHelperOwners[helperIndex];
			IEntity helperEntity = m_aRuntimeHelperEntities[helperIndex];
			HST_CampaignDebugAmbientRuntimeRootSnapshot helperOwnedRoot;
			int helperOwnerMatches;
			foreach (HST_CampaignDebugAmbientRuntimeRootSnapshot ownedHelperRoot : ownedRoots)
			{
				if (ownedHelperRoot
					&& ownedHelperRoot.m_RootEntity == helperOwner
					&& ownedHelperRoot.m_aHelperEntities.Contains(helperEntity)
					&& ownedHelperRoot.m_aHelperRegistryIndices.Contains(helperIndex))
				{
					helperOwnedRoot = ownedHelperRoot;
					helperOwnerMatches++;
				}
			}
			if (helperOwnerMatches != 1 || !helperOwnedRoot
				|| !helperEntity || seenHelpers.Contains(helperEntity)
				|| helperEntity == helperOwner
				|| m_aRuntimeEntities.Contains(helperEntity))
			{
				evidence = "current ambient helper tuple is unowned, shared, reordered, or root-aliased";
				return false;
			}
			seenHelpers.Insert(helperEntity);
		}
		foreach (HST_AmbientActorRuntimeRecord currentActor : m_aAmbientActorRecords)
		{
			int actorMatches;
			foreach (HST_CampaignDebugAmbientRuntimeRootSnapshot actorOwnedRoot : ownedRoots)
			{
				if (actorOwnedRoot
					&& actorOwnedRoot.m_AmbientRecord == currentActor
					&& actorOwnedRoot.m_iActorRecordIndex
						== m_aAmbientActorRecords.Find(currentActor))
					actorMatches++;
			}
			if (actorMatches != 1)
			{
				evidence = "current ambient actor record is unowned or reordered";
				return false;
			}
		}
		array<string> frozenOwnedZoneIds = {};
		foreach (string ownedZoneId : ownedZoneIds)
		{
			if (ownedZoneId.IsEmpty() || frozenOwnedZoneIds.Contains(ownedZoneId))
			{
				evidence = "frozen owned runtime-zone registry is empty or duplicated";
				return false;
			}
			frozenOwnedZoneIds.Insert(ownedZoneId);
		}
		array<string> seenRuntimeZoneIds = {};
		foreach (string currentRuntimeZoneId : m_aRuntimeZoneIds)
		{
			if (!frozenOwnedZoneIds.Contains(currentRuntimeZoneId)
				|| seenRuntimeZoneIds.Contains(currentRuntimeZoneId))
			{
				evidence = "current ambient runtime-zone registry is unowned or duplicated";
				return false;
			}
			seenRuntimeZoneIds.Insert(currentRuntimeZoneId);
		}
		if (seenRuntimeZoneIds.Count() != frozenOwnedZoneIds.Count())
		{
			evidence = "current ambient runtime-zone registry lost a frozen owned zone";
			return false;
		}
		evidence = "all current ambient roots, actor rows, and helper tuples are exact frozen-owned or acknowledged absent";
		return true;
	}

	// Deletes one previously captured post-baseline root only after every
	// parallel registry and linked state-row identity still agrees. Any alias,
	// player claim, pending casualty, or baseline collision retains ownership.
	bool CleanupCampaignDebugOwnedAmbientRuntimeRoot(
		HST_CampaignState state,
		HST_CampaignDebugAmbientRuntimeRootSnapshot expected,
		array<ref HST_CampaignDebugAmbientRuntimeRootSnapshot> baselineRoots,
		out string evidence)
	{
		evidence = "exact owned ambient cleanup rejected";
		if (HasCampaignDebugCivilianMutationForeignClaim(state, evidence))
			return false;
		if (!state || !expected || !baselineRoots)
			return false;
		foreach (HST_CampaignDebugAmbientRuntimeRootSnapshot baseline : baselineRoots)
		{
			if (expected.CollidesWithImmutableIdentity(baseline))
			{
				evidence = "owned ambient identity collides with the frozen baseline";
				return false;
			}
		}

		IEntity rootEntity = expected.m_RootEntity;
		if (!rootEntity)
			return false;
		if (!expected.m_bCleanupDeleteRequested)
		{
			array<ref HST_CampaignDebugAmbientRuntimeRootSnapshot> currentRoots = {};
			array<string> currentZones = {};
			string snapshotEvidence;
			if (!CaptureCampaignDebugAmbientRuntimeRoots(
				state,
				currentRoots,
				currentZones,
				snapshotEvidence))
			{
				evidence = snapshotEvidence;
				return false;
			}
			HST_CampaignDebugAmbientRuntimeRootSnapshot current;
			foreach (HST_CampaignDebugAmbientRuntimeRootSnapshot candidate : currentRoots)
			{
				if (candidate.HasSameImmutableIdentity(expected))
					current = candidate;
				else if (candidate.CollidesWithImmutableIdentity(expected))
				{
					evidence = "owned ambient identity was replaced or aliased";
					return false;
				}
			}
			if (!current || rootEntity.IsDeleted()
				|| IsPlayerControlledEntity(rootEntity)
				|| HasPlayerOccupant(rootEntity)
				|| (current.m_AmbientRecord
					&& current.m_AmbientRecord.m_bCasualtyAdmissionPending)
				|| !AreCampaignDebugOwnedAmbientHelpersExclusive(
					expected,
					true,
					snapshotEvidence))
			{
				evidence = "owned ambient root, actor, helper, or player-claim preflight changed | "
					+ snapshotEvidence;
				return false;
			}
			expected.m_bCleanupDeleteRequested = true;
			SCR_EntityHelper.DeleteEntityAndChildren(rootEntity);
			if (!rootEntity.IsDeleted())
			{
				evidence = "owned ambient root deletion requested; awaiting exact acknowledgement";
				return false;
			}
		}
		if (!rootEntity.IsDeleted())
		{
			evidence = "owned ambient root deletion acknowledgement is still pending";
			return false;
		}
		string absenceEvidence;
		if (IsCampaignDebugOwnedAmbientResidueAbsent(
			state,
			expected,
			absenceEvidence))
		{
			evidence = absenceEvidence;
			return true;
		}

		int runtimeMatches;
		int runtimeIndex = -1;
		for (int rootIndex; rootIndex < m_aRuntimeEntities.Count(); rootIndex++)
		{
			if (m_aRuntimeEntities[rootIndex] != rootEntity)
				continue;
			runtimeMatches++;
			runtimeIndex = rootIndex;
		}
		if (runtimeMatches != 1
			|| runtimeIndex != expected.m_iRuntimeRegistryIndex
			|| runtimeIndex >= m_aRuntimeEntityZoneIds.Count()
			|| runtimeIndex >= m_aRuntimeEntityKinds.Count()
			|| runtimeIndex >= m_aRuntimeEntityFactionKeys.Count()
			|| runtimeIndex >= m_aRuntimeEntityVehicleIds.Count()
			|| runtimeIndex >= m_aRuntimeEntitySpawnPositions.Count()
			|| m_aRuntimeEntityZoneIds[runtimeIndex] != expected.m_sZoneId
			|| m_aRuntimeEntityKinds[runtimeIndex] != expected.m_sRuntimeKind
			|| m_aRuntimeEntityFactionKeys[runtimeIndex] != expected.m_sFactionKey
			|| m_aRuntimeEntityVehicleIds[runtimeIndex]
				!= expected.m_sVehicleRuntimeId
			|| m_aRuntimeEntitySpawnPositions[runtimeIndex]
				!= expected.m_vSpawnPosition
			|| !AreCampaignDebugOwnedAmbientHelpersExclusive(
				expected,
				false,
				absenceEvidence))
		{
			evidence = "acknowledged owned ambient root registries are no longer exact | "
				+ absenceEvidence;
			return false;
		}
		int actorMatches;
		int actorIndex = FindCampaignDebugAmbientRecordIndex(
			rootEntity,
			actorMatches);
		if ((expected.m_AmbientRecord
				&& (actorMatches != 1
					|| actorIndex != expected.m_iActorRecordIndex
					|| m_aAmbientActorRecords[actorIndex]
						!= expected.m_AmbientRecord
					|| expected.m_AmbientRecord.m_DriverEntity
						!= expected.m_DriverEntity
					|| expected.m_AmbientRecord.m_Group != expected.m_Group))
			|| (!expected.m_AmbientRecord && actorMatches != 0))
		{
			evidence = "acknowledged owned ambient actor pointer, order, group, or driver changed";
			return false;
		}
		HST_RuntimeVehicleState vehicleRow;
		int vehicleMatches;
		FindCampaignDebugRuntimeVehicleRowIndex(
			state,
			expected.m_sVehicleRuntimeId,
			vehicleRow,
			vehicleMatches);
		if ((!expected.m_sVehicleRuntimeId.IsEmpty()
				&& (vehicleMatches != 1
					|| vehicleRow != expected.m_RuntimeVehicleRow))
			|| (expected.m_sVehicleRuntimeId.IsEmpty()
				&& vehicleMatches != 0))
		{
			evidence = "acknowledged owned ambient vehicle row changed";
			return false;
		}

		DeleteRuntimeHelpersForOwner(rootEntity);
		if (expected.m_AmbientRecord)
			m_aAmbientActorRecords.Remove(actorIndex);
		if (vehicleRow)
			state.m_aRuntimeVehicles.Remove(
				state.m_aRuntimeVehicles.Find(vehicleRow));
		RemoveRuntimeEntityAt(runtimeIndex);
		if (!IsCampaignDebugOwnedAmbientResidueAbsent(
			state,
			expected,
			absenceEvidence))
		{
			evidence = absenceEvidence;
			return false;
		}
		evidence = "exact acknowledged owned ambient root cleanup complete | "
			+ expected.BuildEvidence();
		return true;
	}

	bool CleanupCampaignDebugOwnedAmbientRuntimeZones(
		array<string> baselineZoneIds,
		array<string> ownedZoneIds,
		out string evidence)
	{
		evidence = "exact owned ambient zone cleanup rejected";
		if (!baselineZoneIds || !ownedZoneIds)
			return false;
		array<string> uniqueOwned = {};
		foreach (string ownedZoneId : ownedZoneIds)
		{
			if (ownedZoneId.IsEmpty() || uniqueOwned.Contains(ownedZoneId)
				|| baselineZoneIds.Contains(ownedZoneId))
			{
				evidence = "owned ambient zone identity is empty, duplicate, or baseline claimed";
				return false;
			}
			if (CountRuntimeEntitiesForZone(ownedZoneId) != 0)
			{
				evidence = "owned ambient zone still contains runtime roots";
				return false;
			}
			int zoneMatches;
			int zoneIndex = -1;
			for (int currentIndex; currentIndex < m_aRuntimeZoneIds.Count(); currentIndex++)
			{
				if (m_aRuntimeZoneIds[currentIndex] != ownedZoneId)
					continue;
				zoneIndex = currentIndex;
				zoneMatches++;
			}
			if (zoneMatches > 1)
			{
				evidence = "owned ambient zone has duplicate registry identities";
				return false;
			}
			if (zoneIndex >= 0)
				m_aRuntimeZoneIds.Remove(zoneIndex);
			uniqueOwned.Insert(ownedZoneId);
		}
		if (m_aRuntimeZoneIds.Count() != baselineZoneIds.Count())
		{
			evidence = "ambient runtime-zone registry did not return to its exact baseline size";
			return false;
		}
		for (int baselineIndex; baselineIndex < baselineZoneIds.Count(); baselineIndex++)
		{
			if (m_aRuntimeZoneIds[baselineIndex] != baselineZoneIds[baselineIndex])
			{
				evidence = "ambient runtime-zone registry did not return to baseline order";
				return false;
			}
		}
		evidence = string.Format(
			"exact ambient zones restored | baseline %1 | owned removed %2",
			baselineZoneIds.Count(),
			ownedZoneIds.Count());
		return true;
	}

	bool CleanupCampaignDebugIsolatedAmbientProjection(
		HST_CampaignState state,
		string ownedZoneId,
		string frozenZoneOwnerFactionKey,
		out string evidence)
	{
		evidence = "isolated ambient projection cleanup rejected";
		if (m_CampaignDebugCivilianMutationLease)
		{
			evidence = "exclusive campaign-debug civilian mutation lease requires receipt-owned cleanup";
			return false;
		}
		int runtimeCount = m_aRuntimeEntities.Count();
		if (!state || ownedZoneId.IsEmpty()
			|| m_aRuntimeEntityZoneIds.Count() != runtimeCount
			|| m_aRuntimeEntityKinds.Count() != runtimeCount
			|| m_aRuntimeEntityFactionKeys.Count() != runtimeCount
			|| m_aRuntimeEntityVehicleIds.Count() != runtimeCount
			|| m_aRuntimeEntitySpawnPositions.Count() != runtimeCount
			|| m_aRuntimeHelperOwners.Count()
				!= m_aRuntimeHelperEntities.Count())
			return false;
		foreach (string runtimeZoneId : m_aRuntimeZoneIds)
		{
			if (runtimeZoneId != ownedZoneId)
			{
				evidence = "isolated ambient runtime-zone registry escaped the selected town";
				return false;
			}
		}
		for (int runtimeIndex; runtimeIndex < runtimeCount; runtimeIndex++)
		{
			IEntity root = m_aRuntimeEntities[runtimeIndex];
			if (m_aRuntimeEntityZoneIds[runtimeIndex] != ownedZoneId
				|| (root && (IsPlayerControlledEntity(root)
					|| HasPlayerOccupant(root))))
			{
				evidence = "isolated ambient root is outside the selected town or player claimed";
				return false;
			}
		}
		foreach (HST_AmbientActorRuntimeRecord actorRecord : m_aAmbientActorRecords)
		{
			if (!actorRecord || !actorRecord.m_RootEntity
				|| !m_aRuntimeEntities.Contains(actorRecord.m_RootEntity)
				|| actorRecord.m_bCasualtyAdmissionPending)
			{
				evidence = "isolated ambient actor is unbound or has pending casualty authority";
				return false;
			}
		}
		foreach (IEntity helperOwner : m_aRuntimeHelperOwners)
		{
			if (!helperOwner || !m_aRuntimeEntities.Contains(helperOwner))
			{
				evidence = "isolated ambient helper owner is not an exact selected-town root";
				return false;
			}
		}
		foreach (HST_RuntimeVehicleState runtimeVehicle : state.m_aRuntimeVehicles)
		{
			if (!runtimeVehicle || runtimeVehicle.m_sZoneId != ownedZoneId
				|| runtimeVehicle.m_bDetached)
			{
				evidence = "isolated runtime-vehicle row is not selected-town transient authority";
				return false;
			}
		}

		HST_CampaignDebugAmbientMutableRegistrySnapshot currentRegistries;
		if (!CaptureCampaignDebugAmbientMutableRegistries(
			currentRegistries,
			evidence)
			|| currentRegistries.m_bConsequenceAuthorityFault
			|| !currentRegistries.m_aCasualtyEventIds.IsEmpty()
			|| !currentRegistries.m_aTheftEventIds.IsEmpty()
			|| !currentRegistries.m_aPanicZoneIds.IsEmpty()
			|| !currentRegistries.m_aResetPreservedVehicles.IsEmpty()
			|| !currentRegistries.m_aResetPreservedCargo.IsEmpty())
			return false;
		for (int retryIndex; retryIndex < currentRegistries.m_aRetryZoneIds.Count(); retryIndex++)
		{
			string retryKind = currentRegistries.m_aRetryKinds[retryIndex];
			if (currentRegistries.m_aRetryZoneIds[retryIndex] != ownedZoneId
				|| (retryKind != "CIV_CHARACTER"
					&& retryKind != "CIV_VEHICLE"
					&& retryKind != "CIV_TRAFFIC_VEHICLE"
					&& retryKind != "MILITARY_VEHICLE"))
			{
				evidence = "isolated ambient retry authority escaped the selected town";
				return false;
			}
		}
		for (int staticIndex; staticIndex < currentRegistries.m_aStaticZoneIds.Count(); staticIndex++)
		{
			string staticOwner
				= currentRegistries.m_aStaticMilitaryOwnerKeys[staticIndex];
			if (currentRegistries.m_aStaticZoneIds[staticIndex] != ownedZoneId
				|| (staticOwner != "<none>"
					&& staticOwner != frozenZoneOwnerFactionKey))
			{
				evidence = "isolated static initialization authority escaped the selected town or frozen owner";
				return false;
			}
		}

		CleanupAllRuntimeEntities(state);
		m_LastAmbientBudgetPlan = null;
		array<ref HST_CampaignDebugAmbientRuntimeRootSnapshot> remainingRoots = {};
		array<string> remainingZones = {};
		HST_CampaignDebugAmbientMutableRegistrySnapshot remainingRegistries;
		if (!CaptureCampaignDebugAmbientRuntimeRoots(
				state,
				remainingRoots,
				remainingZones,
				evidence)
			|| !CaptureCampaignDebugAmbientMutableRegistries(
				remainingRegistries,
				evidence)
			|| !remainingRoots.IsEmpty() || !remainingZones.IsEmpty()
			|| !state.m_aRuntimeVehicles.IsEmpty()
			|| remainingRegistries.m_bConsequenceAuthorityFault
			|| !remainingRegistries.m_aRetryZoneIds.IsEmpty()
			|| !remainingRegistries.m_aStaticZoneIds.IsEmpty()
			|| !remainingRegistries.m_aCasualtyEventIds.IsEmpty()
			|| !remainingRegistries.m_aTheftEventIds.IsEmpty()
			|| !remainingRegistries.m_aPanicZoneIds.IsEmpty()
			|| !remainingRegistries.m_aResetPreservedVehicles.IsEmpty()
			|| !remainingRegistries.m_aResetPreservedCargo.IsEmpty())
		{
			evidence = "isolated ambient projection did not return to the exact global-empty baseline";
			return false;
		}
		evidence = "isolated selected-town ambient projection returned to the exact global-empty baseline";
		return true;
	}

	bool CleanupAmbientProjectionForDebug(HST_CampaignState state)
	{
		if (m_CampaignDebugCivilianMutationLease)
			return false;
		if (!state)
			return false;
		bool changed = PromotePlayerOccupiedRuntimeVehicles(state);
		changed = CleanupAllRuntimeEntities(state) || changed;
		m_LastAmbientBudgetPlan = null;
		m_iNextAmbientRuntimeUpdateSecond = 0;
		PublishRuntimeDiagnostics(state);
		return changed;
	}

	int CountUniqueRuntimeEntityPrefabsForZone(string zoneId, string runtimeKind = "", string factionKey = "")
	{
		if (zoneId.IsEmpty())
			return 0;

		array<string> prefabs = {};
		for (int i = 0; i < m_aRuntimeEntities.Count(); i++)
		{
			if (!MatchesRuntimeEntityFilter(i, zoneId, runtimeKind, factionKey))
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			if (!entity || !entity.GetPrefabData())
				continue;
			string prefab = entity.GetPrefabData().GetPrefabName();
			if (!prefab.IsEmpty() && !prefabs.Contains(prefab))
				prefabs.Insert(prefab);
		}

		return prefabs.Count();
	}

	int CountCivilianActorAppearancesForZone(string zoneId)
	{
		if (zoneId.IsEmpty())
			return 0;

		int count = CountRuntimeEntitiesForZone(zoneId, "CIV_CHARACTER", CIVILIAN_FACTION_KEY);
		for (int i = 0; i < m_aRuntimeHelperEntities.Count(); i++)
		{
			if (IsAmbientTrafficDriverHelper(i, zoneId))
				count++;
		}
		return count;
	}

	int CountUniqueCivilianActorPrefabsForZone(string zoneId)
	{
		array<string> prefabs = {};
		BuildUsedCivilianActorAppearancePrefabs(zoneId, prefabs);
		return prefabs.Count();
	}

	protected void BuildUsedCivilianActorAppearancePrefabs(string zoneId, array<string> prefabs)
	{
		if (zoneId.IsEmpty() || !prefabs)
			return;

		for (int i = 0; i < m_aRuntimeEntities.Count(); i++)
		{
			if (!MatchesRuntimeEntityFilter(i, zoneId, "CIV_CHARACTER", CIVILIAN_FACTION_KEY))
				continue;
			AppendEntityPrefabIfUnique(m_aRuntimeEntities[i], prefabs);
		}
		for (int helperIndex = 0; helperIndex < m_aRuntimeHelperEntities.Count(); helperIndex++)
		{
			if (!IsAmbientTrafficDriverHelper(helperIndex, zoneId))
				continue;
			AppendEntityPrefabIfUnique(m_aRuntimeHelperEntities[helperIndex], prefabs);
		}
	}

	protected void DetachPlayerControlledRuntimeHelpersFromGroups(IEntity owner)
	{
		if (!owner)
			return;
		for (int helperIndex = m_aRuntimeHelperEntities.Count() - 1; helperIndex >= 0; helperIndex--)
		{
			if (helperIndex >= m_aRuntimeHelperOwners.Count()
				|| m_aRuntimeHelperOwners[helperIndex] != owner)
				continue;
			IEntity controlledHelper = m_aRuntimeHelperEntities[helperIndex];
			if (!IsPlayerControlledEntity(controlledHelper))
				continue;
			AIAgent controlledAgent = ResolveCivilianAgentReadOnly(controlledHelper);
			if (!controlledAgent)
				continue;
			AIGroup parentGroup = controlledAgent.GetParentGroup();
			if (parentGroup)
				parentGroup.RemoveAgent(controlledAgent);
		}
	}

	protected bool IsAmbientTrafficDriverHelper(int helperIndex, string zoneId)
	{
		if (helperIndex < 0 || helperIndex >= m_aRuntimeHelperEntities.Count() || helperIndex >= m_aRuntimeHelperOwners.Count())
			return false;
		IEntity owner = m_aRuntimeHelperOwners[helperIndex];
		int runtimeIndex = m_aRuntimeEntities.Find(owner);
		if (runtimeIndex < 0 || runtimeIndex >= m_aRuntimeEntityKinds.Count() || m_aRuntimeEntityKinds[runtimeIndex] != CIVILIAN_TRAFFIC_RUNTIME_KIND)
			return false;
		if (!zoneId.IsEmpty() && (runtimeIndex >= m_aRuntimeEntityZoneIds.Count() || m_aRuntimeEntityZoneIds[runtimeIndex] != zoneId))
			return false;
		IEntity helper = m_aRuntimeHelperEntities[helperIndex];
		return helper && SCR_CharacterControllerComponent.Cast(helper.FindComponent(SCR_CharacterControllerComponent)) != null;
	}

	protected void AppendEntityPrefabIfUnique(IEntity entity, array<string> prefabs)
	{
		if (!entity || !prefabs || !entity.GetPrefabData())
			return;
		string prefab = entity.GetPrefabData().GetPrefabName();
		if (!prefab.IsEmpty() && !prefabs.Contains(prefab))
			prefabs.Insert(prefab);
	}

	int CountRuntimeEntityFactionMismatchesForZone(string zoneId, string runtimeKind, string expectedFactionKey)
	{
		if (zoneId.IsEmpty() || expectedFactionKey.IsEmpty())
			return 0;

		int count;
		for (int i = 0; i < m_aRuntimeEntities.Count(); i++)
		{
			if (!MatchesRuntimeEntityFilter(i, zoneId, runtimeKind, expectedFactionKey))
				continue;

			IEntity entity = m_aRuntimeEntities[i];
			if (!entity)
				continue;

			FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
			if (!factionComponent || factionComponent.GetAffiliatedFactionKey() != expectedFactionKey)
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
		int admittedPedestrians = CountAmbientAdmittedActorsForZone(zoneId, "CIV_CHARACTER");
		int readyPedestrians = CountAmbientBehaviorReadyActorsForZone(zoneId, "CIV_CHARACTER");
		int admittedTraffic = CountAmbientAdmittedActorsForZone(zoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND);
		int readyTraffic = CountAmbientBehaviorReadyActorsForZone(zoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND);
		string report = string.Format("runtime behavior %1 | pedestrians %2/%3 with helpers | pedestrian helpers %4 | traffic %5/%6 with helpers | traffic helpers %7", EmptyRuntimeField(zoneId), pedestrianBehavior, civilianCharacters, pedestrianHelpers, trafficBehavior, trafficVehicles, trafficHelpers);
		return report + string.Format(" | admitted/ready pedestrians %1/%2 | traffic %3/%4", admittedPedestrians, readyPedestrians, admittedTraffic, readyTraffic);
	}

	string BuildRuntimeTownPopulationReport(HST_CampaignState state, string zoneId)
	{
		int civilianCharacters = CountRuntimeEntitiesForZone(zoneId, "CIV_CHARACTER", "CIV");
		int civilianVehicles = CountRuntimeEntitiesForZone(zoneId, "CIV_VEHICLE", "CIV");
		int civilianTrafficVehicles = CountRuntimeEntitiesForZone(zoneId, CIVILIAN_TRAFFIC_RUNTIME_KIND, "CIV");
		int militaryVehicles = CountRuntimeEntitiesForZone(zoneId, "MILITARY_VEHICLE");
		int civilianFactionMismatches = CountRuntimeEntityFactionMismatchesForZone(zoneId, "CIV_CHARACTER", "CIV");
		int total = CountRuntimeEntitiesForZone(zoneId);
		string report = string.Format("runtime town %1 | active %2 | total %3 | civ chars %4 | parked civ vehicles %5 | traffic vehicles %6 | military vehicles %7", EmptyRuntimeField(zoneId), HasRuntimeZone(zoneId), total, civilianCharacters, civilianVehicles, civilianTrafficVehicles, militaryVehicles);
		report = report + string.Format(" | civ character faction mismatches %1", civilianFactionMismatches);
		if (state)
			report = report + string.Format(" | global civ chars %1 | civ vehicles %2 | failures %3 | last failure %4", state.m_iRuntimeCivilianCharacterCount, state.m_iRuntimeCivilianVehicleCount, state.m_iRuntimeSpawnFailureCount, EmptyRuntimeField(state.m_sLastRuntimeSpawnFailurePrefab));

		return report;
	}

	string BuildTownInfluenceReport(HST_CampaignState state, int maxRows = 20)
	{
		if (!state)
			return "Partisan town influence | state not ready";

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

		string report = string.Format("Partisan town influence ledger | events %1 | applied %2 | active modifiers %3 | expired modifiers %4", events, applied, active, expired);
		if (m_TownInfluence)
			report = m_TownInfluence.BuildTownInfluenceReport(state, maxRows) + "\n" + report;
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

	protected HST_CampaignPreset ResolveInfluencePreset(HST_CampaignPreset preset)
	{
		if (preset)
			return preset;
		return m_Preset;
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

	protected string SelectCivilianCharacterPrefabAvoiding(HST_BalanceConfig balance, int index, int seed, array<string> excludedPrefabs)
	{
		if (!balance || CountGuidQualifiedCivilianCharacterPrefabs(balance) < MIN_CIVILIAN_CHARACTER_PREFABS)
			return "";

		for (int i = 0; i < balance.m_aCivilianCharacterPrefabs.Count(); i++)
		{
			string prefab = balance.m_aCivilianCharacterPrefabs[ModInt(seed + index + i, balance.m_aCivilianCharacterPrefabs.Count())];
			if (!IsGuidQualifiedResource(prefab))
				continue;
			if (!excludedPrefabs || !excludedPrefabs.Contains(prefab))
				return prefab;
		}

		// Exhaustion is a capacity/readiness failure, never permission to clone an
		// appearance already projected in the same locality.
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
			Print(string.Format("Partisan civilians | CIV vehicle catalog unavailable or empty (%1); using configured/internal civilian vehicle fallback pool", CIVILIAN_VEHICLE_ENTITY_CATALOG), LogLevel.WARNING);
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

		Print(string.Format("Partisan civilians | no non-aircraft faction vehicle available for %1; skipping owner vehicle", factionKey), LogLevel.WARNING);
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

		array<string> uniquePrefabs = {};
		foreach (string prefab : balance.m_aCivilianCharacterPrefabs)
		{
			if (IsGuidQualifiedResource(prefab)
				&& !uniquePrefabs.Contains(prefab))
				uniquePrefabs.Insert(prefab);
		}

		return uniquePrefabs.Count();
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

	protected string RegisterRuntimeVehicle(HST_CampaignState state, IEntity entity, string zoneId, string prefab, vector position, vector angles, string factionKey, string runtimeKind)
	{
		if (!state || !entity || !IsRuntimeVehicle(runtimeKind) || !HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
			return "";

		string vehicleRuntimeId = ResolveRuntimeVehicleId(entity);
		if (vehicleRuntimeId.IsEmpty() || state.FindRuntimeVehicle(vehicleRuntimeId))
			vehicleRuntimeId = BuildUniqueAmbientRuntimeVehicleId(state, zoneId);
		if (vehicleRuntimeId.IsEmpty())
			return "";

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
		TrackCampaignDebugCivilianMutationRuntimeVehicleRow(vehicle);
		return vehicleRuntimeId;
	}

	protected string BuildUniqueAmbientRuntimeVehicleId(
		HST_CampaignState state,
		string zoneId)
	{
		if (!state)
			return "";
		for (int attempt; attempt < 32; attempt++)
		{
			string candidate = BuildAmbientRuntimeId(
				zoneId,
				"vehicle",
				state.m_iElapsedSeconds);
			if (!state.FindRuntimeVehicle(candidate))
				return candidate;
		}
		return "";
	}

	protected bool PromoteClaimedRuntimeVehicleWithConsequences(
		HST_CampaignState state,
		IEntity entity,
		string originalRuntimeKind,
		string zoneId,
		string claimantFactionKey)
	{
		if (!state || !entity || claimantFactionKey.IsEmpty())
			return false;
		bool requiresTheft = false;
		HST_ZoneState zone;
		HST_RuntimeVehicleState vehicle;
		string eventId;
		if (originalRuntimeKind == CIVILIAN_TRAFFIC_RUNTIME_KIND
			|| originalRuntimeKind == "CIV_VEHICLE")
		{
			zone = state.FindZone(zoneId);
			if (zone && zone.m_eType == HST_EZoneType.HST_ZONE_TOWN
				&& !m_Preset)
				return false;
			if (zone && zone.m_eType == HST_EZoneType.HST_ZONE_TOWN
				&& claimantFactionKey == m_Preset.m_sResistanceFactionKey)
			{
				if (!m_CivilianConsequences
					|| !HasExactPendingCivilianTheftArrays()
					|| m_aPendingCivilianTheftZoneIds.Count()
						>= MAX_PENDING_CIVILIAN_THEFTS)
					return false;
				vehicle = ResolveRuntimeVehicleRecord(state, entity);
				if (!vehicle || vehicle.m_sVehicleRuntimeId.IsEmpty())
					return false;
				eventId = "civilian_theft_" + vehicle.m_sVehicleRuntimeId;
				if (eventId.Length()
					> HST_CivilianConsequenceService.MAX_ID_CHARACTERS)
					return false;
				requiresTheft = true;
			}
		}
		if (!PromoteRuntimeVehicleToPersistentField(state, entity))
			return false;
		if (!requiresTheft)
			return true;
		HST_CivilianConsequenceResult consequence
			= m_CivilianConsequences.RegisterCivilianVehicleTheft(
				state,
				zone.m_sZoneId,
				eventId,
				claimantFactionKey,
				vehicle.m_sVehicleRuntimeId);
		if (consequence && consequence.m_bAccepted)
			return true;
		return QueuePendingCivilianTheft(
			zone.m_sZoneId,
			eventId,
			claimantFactionKey,
			vehicle.m_sVehicleRuntimeId);
	}

	protected string ResolveExactVehicleClaimantFaction(IEntity vehicleEntity)
	{
		if (!vehicleEntity)
			return "";
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return "";
		BaseCompartmentManagerComponent compartmentManager
			= ResolveCompartmentManager(vehicleEntity);
		if (!compartmentManager)
			return "";
		array<BaseCompartmentSlot> slots = {};
		compartmentManager.GetCompartments(slots);
		int selectedPlayerId = int.MAX;
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot
				|| (!slot.IsPiloting()
					&& slot.GetType() != ECompartmentType.PILOT))
				continue;
			IEntity occupant = slot.GetOccupant();
			if (!occupant)
				continue;
			int playerId = playerManager.GetPlayerIdFromControlledEntity(occupant);
			if (playerId <= 0)
				playerId = SCR_PossessingManagerComponent.GetPlayerIdFromMainEntity(
					occupant);
			if (playerId > 0 && playerId < selectedPlayerId)
				selectedPlayerId = playerId;
		}
		if (selectedPlayerId == int.MAX)
			return "";
		Faction faction = SCR_FactionManager.SGetPlayerFaction(selectedPlayerId);
		if (!faction)
			return "";
		return faction.GetFactionKey();
	}

	protected bool PromoteRuntimeVehicleToPersistentField(
		HST_CampaignState state,
		IEntity entity)
	{
		if (!state || !IsLivingAmbientEntity(entity))
			return false;
		// No direct cleanup/policy caller may bypass the controlled-end discovery
		// gate and mutate a durable row after its immutable binding scope is fixed.
		if (m_PersistentFieldVehicles
			&& (m_PersistentFieldVehicles
				.HasControlledShutdownBindingScopeLocked()
				|| m_PersistentFieldVehicles
					.HasControlledShutdownQuiescenceApplied()))
			return false;
		HST_RuntimeVehicleState vehicle = ResolveRuntimeVehicleRecord(state, entity);
		bool insertRecordAfterTrack;
		string previousRuntimeKind;
		bool previousDetached;
		bool previousDeleted;
		if (!vehicle)
		{
			int trackedIndex = m_aRuntimeEntities.Find(entity);
			if (!state || trackedIndex < 0
				|| trackedIndex >= m_aRuntimeEntityVehicleIds.Count()
				|| m_aRuntimeEntityVehicleIds[trackedIndex].IsEmpty()
				|| !entity.GetPrefabData())
				return false;
			string prefab = entity.GetPrefabData().GetPrefabName();
			if (!HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
				return false;
			string vehicleRuntimeId = m_aRuntimeEntityVehicleIds[trackedIndex];
			if (state.FindRuntimeVehicle(vehicleRuntimeId))
				return false;
			vehicle = new HST_RuntimeVehicleState();
			vehicle.m_sVehicleRuntimeId = vehicleRuntimeId;
			vehicle.m_sPrefab = prefab;
			vehicle.m_sDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(prefab);
			if (trackedIndex < m_aRuntimeEntityFactionKeys.Count())
				vehicle.m_sFactionKey = m_aRuntimeEntityFactionKeys[trackedIndex];
			if (trackedIndex < m_aRuntimeEntityZoneIds.Count())
				vehicle.m_sZoneId = m_aRuntimeEntityZoneIds[trackedIndex];
			vehicle.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
			insertRecordAfterTrack = true;
		}
		else
		{
			previousRuntimeKind = vehicle.m_sRuntimeKind;
			previousDetached = vehicle.m_bDetached;
			previousDeleted = vehicle.m_bDeleted;
		}

		// Track accepts an off-ledger durable candidate. Publish only the minimum
		// eligibility fields first, then either restore the pre-promotion row on
		// failure or commit the complete durable DTO after exact binding succeeds.
		// This prevents a transient detach failure from stranding an active,
		// unbound field row that every later preflight would correctly reject.
		vehicle.m_sRuntimeKind = "field_vehicle";
		vehicle.m_bDetached = false;
		vehicle.m_bDeleted = false;
		if (!m_PersistentFieldVehicles
			|| !m_PersistentFieldVehicles.Track(entity, vehicle))
		{
			if (!insertRecordAfterTrack)
			{
				vehicle.m_sRuntimeKind = previousRuntimeKind;
				vehicle.m_bDetached = previousDetached;
				vehicle.m_bDeleted = previousDeleted;
			}
			return false;
		}
		if (insertRecordAfterTrack)
			state.m_aRuntimeVehicles.Insert(vehicle);
		vehicle.m_vPosition = entity.GetOrigin();
		vehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(
			entity.GetYawPitchRoll());
		vehicle.m_bCanProvideUndercover
			= RuntimeVehicleCanProvideCivilianUndercover(vehicle);
		HST_VehicleCapabilityPolicy.NormalizeRuntimeVehicleCoverState(vehicle);
		return true;
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

		for (int index = state.m_aRuntimeVehicles.Count() - 1; index >= 0; index--)
		{
			HST_RuntimeVehicleState vehicle = state.m_aRuntimeVehicles[index];
			if (!vehicle || vehicle.m_bDeleted || vehicle.m_sRuntimeKind != runtimeKind)
				continue;

			if (DistanceSq2D(vehicle.m_vPosition, spawnPosition) > 4.0)
				continue;

			if (!vehicle.m_bDetached && IsTransientAmbientVehicleKind(runtimeKind))
				state.m_aRuntimeVehicles.Remove(index);
			else
				vehicle.m_bDeleted = true;
			return;
		}
	}

	protected bool IsTransientAmbientVehicleKind(string runtimeKind)
	{
		return runtimeKind == CIVILIAN_TRAFFIC_RUNTIME_KIND
			|| runtimeKind == "CIV_VEHICLE"
			|| runtimeKind == "MILITARY_VEHICLE";
	}

	protected HST_RuntimeVehicleState ResolveRuntimeVehicleRecord(HST_CampaignState state, IEntity entity)
	{
		if (!state || !entity)
			return null;

		int trackedIndex = m_aRuntimeEntities.Find(entity);
		if (trackedIndex >= 0 && trackedIndex < m_aRuntimeEntityVehicleIds.Count())
		{
			string stableRuntimeId = m_aRuntimeEntityVehicleIds[trackedIndex];
			if (!stableRuntimeId.IsEmpty())
			{
				HST_RuntimeVehicleState stableRecord = state.FindRuntimeVehicle(stableRuntimeId);
				if (stableRecord)
					return stableRecord;
			}
		}

		// Durable roots use the shared session binding. A current process RPL ID
		// can reuse a saved string after restart and is never exact durable proof.
		if (m_PersistentFieldVehicles)
			return m_PersistentFieldVehicles.ResolveForEntity(state, entity);

		return null;
	}

	protected string ResolveRuntimeVehicleId(IEntity entity)
	{
		if (!entity)
			return "";

		BaseRplComponent rpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("rpl_%1", rpl.Id());

		return "";
	}

	protected void WarnMissingCivilianCharacterPool(HST_ZoneState zone, int requestedCount)
	{
		if (!m_bWarnedMissingCivilianCharacterPool)
		{
			Print(string.Format("Partisan civilians | fewer than %1 GUID-qualified civilian character prefabs configured; skipping civilian character ambience instead of spawning broken CIV group shells", MIN_CIVILIAN_CHARACTER_PREFABS), LogLevel.WARNING);
			m_bWarnedMissingCivilianCharacterPool = true;
		}

		if (zone)
			RecordSpawnFailure(zone.m_sZoneId, "<missing minimum GUID-qualified civilian character pool>", "CIV_CHARACTER", zone.m_vPosition, BuildSpawnAngles(0));
	}

	protected void RecordSpawnFailure(string zoneId, string prefab, string runtimeKind, vector position, vector angles)
	{
		m_iRuntimeSpawnFailureCount++;
		m_sLastRuntimeSpawnFailurePrefab = prefab;
		Print(string.Format("Partisan civilians | spawn failed | zone %1 | kind %2 | prefab %3 | pos %4 | yaw %5", zoneId, runtimeKind, prefab, position, angles[0]), LogLevel.WARNING);
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

	protected bool ShouldDetachFromTownCleanup(
		HST_CampaignState state,
		IEntity entity,
		string runtimeKind)
	{
		if (!entity || !IsRuntimeVehicle(runtimeKind))
			return false;

		HST_RuntimeVehicleState vehicle = ResolveRuntimeVehicleRecord(state, entity);
		return (vehicle && vehicle.m_sRuntimeKind == "field_vehicle")
			|| HasPlayerOccupant(entity);
	}

	protected float DistanceSq2D(vector first, vector second)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz;
	}
}
