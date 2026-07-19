class HST_GunShopCatalogCandidate
{
	string m_sPrefab;
	string m_sDisplayName;
	string m_sCategory;
	string m_sFactionKey;
	int m_iSupplyCost;
}

class HST_MissionRuntimeService
{
	static const string PRIMITIVE_KILL_HVT = "kill_hvt";
	static const string PRIMITIVE_HOLD_AREA = "hold_area";
	static const string PRIMITIVE_CLEAR_AREA = "clear_area";
	static const string PRIMITIVE_DESTROY_TARGET = "destroy_target";
	static const string PRIMITIVE_RECOVER_CARGO = "recover_cargo";
	static const string PRIMITIVE_RESCUE_EXTRACT = "rescue_extract";
	static const string PRIMITIVE_DELIVER_SUPPLIES = "deliver_supplies";
	static const string PRIMITIVE_CONVOY_INTERCEPT = "convoy_intercept";
	static const string PRIMITIVE_GUN_SHOP = "gun_shop";
	static const string PRIMITIVE_ABSTRACT_FALLBACK = "abstract_fallback";
	static const string PHASE_CREATED = "created";
	static const string PHASE_ACTIVE = "active";
	static const string PHASE_CONTACT = "contact";
	static const string PHASE_FREED = "freed";
	static const string PHASE_EXTRACTING = "extracting";
	static const string PHASE_FOLLOWING = "following";
	static const string PHASE_DELIVERING = "delivering";
	static const string PHASE_HOLDING = "holding";
	static const string PHASE_FAILED = "failed";
	static const string PHASE_LOADED = "loaded";
	static const string PHASE_UNLOADED = "unloaded";
	static const string PHASE_DELIVERED = "delivered";
	static const string PHASE_DESTROYED = "destroyed";
	static const string PHASE_CAPTURED = "captured";
	static const string PHASE_CONVOY_STAGING = "convoy_staging";
	static const string PHASE_CONVOY_MOVING = "convoy_moving";
	static const string PHASE_CONVOY_CONTACT = "convoy_contact";
	static const string PHASE_CONVOY_ELIMINATED = "convoy_eliminated";
	static const string PHASE_CONVOY_ARRIVED = "convoy_arrived";
	static const string PROP_HVT = "{6985327711303700}Prefabs/Objects/HST/HST_MissionProp_HVT.et";
	static const string PROP_DESTROY_TARGET = "{6985327711303710}Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et";
	static const string PROP_CARGO = "{6985327711303720}Prefabs/Objects/HST/HST_MissionProp_Cargo.et";
	static const string PROP_CAPTIVES = "{6985327711303730}Prefabs/Objects/HST/HST_MissionProp_Captives.et";
	static const string PROP_HOLD_MARKER = "{6985327711303740}Prefabs/Objects/HST/HST_MissionProp_HoldMarker.et";
	static const string PROP_CITY_SUPPLIES = "{6985327711303750}Prefabs/Objects/HST/HST_MissionProp_CitySupplies.et";
	static const string PROP_CONVOY_PAYLOAD = "{6985327711303760}Prefabs/Objects/HST/HST_MissionProp_ConvoyPayload.et";
	static const string PROP_BANK_MONEY = "{6985327711303770}Prefabs/Objects/HST/HST_MissionProp_BankMoney.et";
	static const string PROP_RESOURCE_CACHE = "{6985327711303780}Prefabs/Objects/HST/HST_MissionProp_ResourceCache.et";
	static const string PROP_CONVOY_VEHICLE = "{4AE9D080927D3CB9}Prefabs/Vehicles/Wheeled/S1203/S1203_base.et";
	static const string PROP_ORDINARY_MIXED_NATIVE_CARRIER = "{B55C6990A6A9411B}Prefabs/Vehicles/Wheeled/M998/M998_covered.et";
	static const string PROP_GUN_SHOP_SELLER = "{6985327711303860}Prefabs/Characters/HST/Character_HST_GunShopCivilian.et";
	static const string PROP_GUN_SHOP_DELIVERY_DRIVER = "{22E43956740A6794}Prefabs/Characters/Factions/CIV/GenericCivilians/Character_CIV_Randomized.et";
	static const string PROP_RUNTIME_EMPTY_GROUP = "{6985327711303910}Prefabs/Groups/HST/HST_RuntimeEmptyGroup.et";
	static const string ASSET_KIND_CHARACTER = "character";
	static const string ASSET_KIND_CARGO = "cargo";
	static const string ASSET_KIND_CAPTIVE = "captive";
	static const string ASSET_KIND_TARGET = "target";
	static const string ASSET_KIND_VEHICLE = "vehicle";
	static const string ASSET_KIND_AREA = "area";
	static const string ROLE_HVT = "hvt";
	static const string ROLE_DESTROY_TARGET = "destroy_target";
	static const string ROLE_HOLD_MARKER = "hold_marker";
	static const string ROLE_LOGISTICS_CARGO = "logistics_cargo";
	static const string ROLE_CITY_SUPPLIES = "city_supplies";
	static const string ROLE_CAPTIVE = "captive";
	static const string ROLE_CONVOY_VEHICLE = "convoy_vehicle";
	static const string ROLE_CONVOY_PAYLOAD = "convoy_payload";
	static const string ROLE_CONVOY_CAPTIVE = "convoy_captive";
	static const string ROLE_GUN_SHOP_SELLER = "gun_shop_seller";
	static const string ROLE_GUN_SHOP_DELIVERY_DRIVER = "gun_shop_delivery_driver";
	static const string ROLE_GUN_SHOP_DELIVERY_VEHICLE = "gun_shop_delivery_vehicle";
	static const float PLAYER_OBJECTIVE_RADIUS_METERS = 35.0;
	static const float PLAYER_ASSET_RADIUS_METERS = 18.0;
	static const float PLAYER_DELIVERY_RADIUS_METERS = 45.0;
	static const float HOSTILE_OBJECTIVE_RADIUS_METERS = 90.0;
	static const float VEHICLE_CARRIER_RADIUS_METERS = 10.0;
	static const float MAX_CONVOY_ZONE_START_DISTANCE_METERS = 5000.0;
	static const int DEFAULT_HOLD_SECONDS = 45;
	static const int DEFAULT_ASSET_INTERACTION_RADIUS_METERS = 18;
	static const int DEFAULT_DELIVERY_RADIUS_METERS = 45;
	static const int DEFAULT_MISSION_CARRIER_CAPACITY = 6;
	static const int MAX_MISSION_VEHICLE_SCAN_ENTITIES = 96;
	static const float MIN_CONVOY_START_DISTANCE_METERS = 2000.0;
	static const float MAX_CONVOY_START_DISTANCE_METERS = 5000.0;
	static const float MIN_CONVOY_STAGING_ZONE_CLEARANCE_METERS = 260.0;
	static const float MIN_CONVOY_STAGING_SITE_CLEARANCE_METERS = 140.0;
	static const int CONVOY_ROUTE_SAMPLE_COUNT = 6;
	static const int CONVOY_START_PROBE_ATTEMPTS = 72;
	static const float CONVOY_DESTINATION_ROAD_SEARCH_RADIUS_METERS = 300.0;
	static const float CONVOY_START_ROAD_SEARCH_RADIUS_METERS = 250.0;
	static const float CONVOY_SLOT_ROAD_SEARCH_RADIUS_METERS = 40.0;
	static const float CONVOY_EXPANDED_START_ROAD_SEARCH_RADIUS_METERS = 700.0;
	static const float CONVOY_EXPANDED_SLOT_ROAD_SEARCH_RADIUS_METERS = 90.0;
	static const float CONVOY_VEHICLE_START_SPACING_METERS = 24.0;
	static const float MIN_CONVOY_VEHICLE_START_SEPARATION_METERS = 18.0;
	static const int CONVOY_ROUTE_SEGMENT_FALLBACK_STEPS = 4;
	static const int MIN_CONVOY_VEHICLES = 3;
	static const int MAX_CONVOY_VEHICLES = 6;
	static const int MIN_CONVOY_IDLE_SECONDS = 300;
	static const int MAX_CONVOY_IDLE_SECONDS = 600;
	static const int GUN_SHOP_MIN_WEAPON_TYPES = 5;
	static const int GUN_SHOP_MAX_WEAPON_TYPES = 20;
	static const int GUN_SHOP_MIN_CLOTHING_TYPES = 5;
	static const int GUN_SHOP_MAX_CLOTHING_TYPES = 20;
	static const int GUN_SHOP_MAX_EXPEDITED_SECONDS = 900;
	static const float GUN_SHOP_DELIVERY_ARRIVAL_RADIUS_METERS = 45.0;
	static const float GUN_SHOP_DELIVERY_SPEED_METERS_PER_SECOND = 16.0;
	static const float GUN_SHOP_DELIVERY_ROAD_SEARCH_RADIUS_METERS = 450.0;
	static const float GUN_SHOP_DELIVERY_SPAWN_EDGE_MARGIN_METERS = 120.0;
	static const int GUN_SHOP_SELLER_INTERACTION_RADIUS_METERS = 22;
	static const float CAPTIVE_FOLLOW_NEAR_DISTANCE_METERS = 5.0;
	static const float CAPTIVE_FOLLOW_BREAK_DISTANCE_METERS = 100.0;
	static const float CAPTIVE_DISEMBARK_RADIUS_METERS = 35.0;
	static const float POST_EXPIRY_PLAYER_ASSET_BUBBLE_METERS = 1800.0;
	static const int CAPTIVE_FOLLOW_WAYPOINT_INTERVAL_SECONDS = 4;
	static const string CAPTIVE_FOLLOW_WAYPOINT_PREFAB = "{A0509D3C4DD4475E}Prefabs/AI/Waypoints/AIWaypoint_Follow.et";
	static const string CAPTIVE_MOVE_WAYPOINT_PREFAB = "{FBA8DC8FDA0E770D}Prefabs/AI/Waypoints/AIWaypoint_Patrol_Hierarchy.et";
	static const string MISSION_CONVOY_GROUP_PREFIX = "mission_convoy_";
	static const string PERSISTENCE_SMOKE_PREFIX = "hst_smoke";
	static const string CAMPAIGN_DEBUG_PREFIX_ROOT = "hst_debug_";
	static const string CAMPAIGN_DEBUG_ENTITY_TAG = "HST_CAMPAIGN_DEBUG";
	static const string MISSION_DESTROY_RADIO_TOWER = "destroy_radio_tower";
	static const string MISSION_STOP_TOWER_REBUILD = "dynamic_stop_tower_rebuild";
	static const float EXISTING_RADIO_TOWER_SEARCH_RADIUS_METERS = 220.0;
	static const string RADIO_TOWER_PREFAB_TOKEN = "TransmitterTower_01";
	static const string EXACT_RESCUE_SEAT_TOKEN_PREFIX = "seat_v2_";
	static const string EXACT_RESCUE_SEAT_UNIQUE_PREFIX = "seat_v2_u_";
	static const string EXACT_RESCUE_SEAT_OBJECT_PREFIX = "seat_v2_o_";
	static const int EXACT_RESCUE_SEAT_DIAGNOSTIC_LIMIT = 8;
	static const int EXACT_RESCUE_COMPARTMENT_DEPTH_LIMIT = 24;

	protected ref HST_ForceCompositionService m_ForceCompositions = new HST_ForceCompositionService();
	protected ref HST_CombatPresenceService m_CombatPresence = new HST_CombatPresenceService();
	protected ref array<string> m_aRuntimeEntityIds = {};
	protected ref array<IEntity> m_aRuntimeEntities = {};
	protected ref array<IEntity> m_aBorrowedWorldEntities = {};
	protected ref array<IEntity> m_aExistingRadioTowerCandidates = {};
	protected ref array<IEntity> m_aMissionVehicleScanEntities = {};
	protected ref array<string> m_aRestoredMissionCarrierRestoreMissionIds = {};
	protected ref array<string> m_aCaptiveFollowWaypointAssetIds = {};
	protected ref array<int> m_aCaptiveFollowWaypointSeconds = {};
	protected ref HST_ArsenalService m_Arsenal;
	protected ref HST_BalanceConfig m_Balance;
	protected ref HST_PhysicalWarService m_PhysicalWar;
	protected bool m_bDebugLoggingEnabled;
	protected IEntity m_OrdinaryCampaignMixedNativeForeignOccupantProof;

	void SetForceCompositionService(HST_ForceCompositionService forceCompositions)
	{
		if (forceCompositions)
			m_ForceCompositions = forceCompositions;
	}

	void SetArsenalService(HST_ArsenalService arsenal)
	{
		m_Arsenal = arsenal;
	}

	void SetBalanceConfig(HST_BalanceConfig balance)
	{
		m_Balance = balance;
	}

	void SetPhysicalWarService(HST_PhysicalWarService physicalWar)
	{
		m_PhysicalWar = physicalWar;
	}

	void SetCombatPresenceService(HST_CombatPresenceService combatPresence)
	{
		if (combatPresence)
			m_CombatPresence = combatPresence;
	}

	void SetDebugLoggingEnabled(bool enabled)
	{
		m_bDebugLoggingEnabled = enabled;
	}

	void CleanupRuntimeEntityForStateRestore(string runtimeEntityId)
	{
		DeleteRuntimeEntity(runtimeEntityId);
	}

	int CountRuntimeEntityHandlesForMission(HST_CampaignState state, string missionInstanceId)
	{
		if (!state || missionInstanceId.IsEmpty())
			return 0;

		int count;
		array<string> countedIds = {};
		foreach (HST_MissionRuntimeEntityState runtimeEntity : state.m_aMissionRuntimeEntities)
		{
			if (!runtimeEntity || runtimeEntity.m_sMissionInstanceId != missionInstanceId || runtimeEntity.m_sRuntimeEntityId.IsEmpty())
				continue;
			if (countedIds.Contains(runtimeEntity.m_sRuntimeEntityId))
				continue;
			if (!GetRuntimeEntity(runtimeEntity.m_sRuntimeEntityId))
				continue;

			countedIds.Insert(runtimeEntity.m_sRuntimeEntityId);
			count++;
		}

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != missionInstanceId || asset.m_sEntityId.IsEmpty())
				continue;
			if (countedIds.Contains(asset.m_sEntityId))
				continue;
			if (!GetRuntimeEntity(asset.m_sEntityId))
				continue;

			countedIds.Insert(asset.m_sEntityId);
			count++;
		}

		return count;
	}

	bool HasMissionAssetRuntimeHandle(HST_MissionAssetState asset)
	{
		return asset && !asset.m_sEntityId.IsEmpty()
			&& GetRuntimeEntity(asset.m_sEntityId) != null;
	}

	bool HasRuntimeEntityHandle(string runtimeEntityId)
	{
		return !runtimeEntityId.IsEmpty() && GetRuntimeEntity(runtimeEntityId) != null;
	}

	bool ResolveOrdinaryCampaignMixedNativeCaptiveProjectionReadOnly(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState captive,
		out IEntity captiveEntity,
		out string evidence)
	{
		captiveEntity = null;
		evidence
			= "ordinary mixed-native captive projection is unavailable";
		if (!state || !mission || !captive
			|| !HST_RescuePOWOperationService.IsExactMission(mission)
			|| !IsExactRescueCaptiveAsset(mission, captive))
			return false;
		string projectionId = captive.m_sRescueProjectionId;
		if (projectionId.IsEmpty())
			projectionId = captive.m_sEntityId;
		IEntity boundEntity;
		if (projectionId.IsEmpty()
			|| (!captive.m_sRescueProjectionId.IsEmpty()
				&& !captive.m_sEntityId.IsEmpty()
				&& captive.m_sRescueProjectionId != captive.m_sEntityId)
			|| !TryResolveRuntimeEntityBindingReadOnly(
				projectionId,
				boundEntity,
				evidence)
			|| !boundEntity || boundEntity.IsDeleted()
			|| !boundEntity.GetWorld())
			return false;
		captiveEntity = boundEntity;
		evidence
			= "ordinary mixed-native captive projection binding is read-only exact";
		return true;
	}

	bool ResolveExactRescueCaptiveProjectionReadOnly(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState captive,
		out IEntity captiveEntity,
		out string evidence)
	{
		captiveEntity = null;
		evidence = "exact rescue captive projection binding is unavailable";
		if (!state || !mission || !captive
			|| !HST_RescuePOWOperationService.IsExactMission(mission)
			|| !IsExactRescueCaptiveAsset(mission, captive))
			return false;

		string projectionId = captive.m_sRescueProjectionId;
		if (projectionId.IsEmpty())
			projectionId = captive.m_sEntityId;
		IEntity boundEntity;
		if (projectionId.IsEmpty()
			|| (!captive.m_sRescueProjectionId.IsEmpty()
				&& !captive.m_sEntityId.IsEmpty()
				&& captive.m_sRescueProjectionId != captive.m_sEntityId)
			|| !TryResolveRuntimeEntityBindingReadOnly(
				projectionId,
				boundEntity,
				evidence)
			|| !boundEntity || boundEntity.IsDeleted()
			|| !boundEntity.GetWorld())
			return false;

		captiveEntity = boundEntity;
		evidence = "exact rescue captive projection binding is read-only exact";
		return true;
	}

	// The ordinary persistence proof uses the same replicated carrier authority as
	// live rescue missions. This seam is intentionally narrow: it creates one
	// production-prefab carrier, requires a replication-backed durable identity,
	// and binds that identity one-to-one before exposing the entity to the proof.
	// The physical-war latch is applied before the rescue latch in the controlled-
	// shutdown transaction, so it is also the earliest safe mutation cutoff here.
	bool PrepareOrdinaryCampaignMixedNativeRescueCarrierProof(
		HST_CampaignState state,
		vector requestedPosition,
		out IEntity carrierEntity,
		out string carrierRuntimeId,
		out string evidence)
	{
		carrierEntity = null;
		carrierRuntimeId = "";
		evidence = "ordinary mixed-native rescue carrier proof is unavailable";
		if (!CanMutateOrdinaryCampaignMixedNativeProof(evidence))
			return false;
		if (!state || IsZeroVector(requestedPosition))
		{
			evidence = "ordinary mixed-native rescue carrier proof requires state and a nonzero position";
			return false;
		}

		vector spawnPosition;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(
			requestedPosition,
			spawnPosition,
			true))
		{
			evidence = "ordinary mixed-native rescue carrier proof could not resolve a safe vehicle position";
			return false;
		}

		GenericEntity spawned = HST_WorldPositionService.SpawnPrefab(
			PROP_ORDINARY_MIXED_NATIVE_CARRIER,
			spawnPosition,
			HST_WorldPositionService.BuildUprightAngles(0.0));
		if (!spawned)
		{
			evidence = "ordinary mixed-native rescue carrier proof could not spawn its production carrier";
			return false;
		}
		HST_WorldPositionService.ApplyUprightEntityTransform(
			spawned,
			spawnPosition,
			HST_WorldPositionService.BuildUprightAngles(0.0));
		HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(spawned);

		BaseRplComponent replication = BaseRplComponent.Cast(
			spawned.FindComponent(BaseRplComponent));
		if (!replication || replication.Id() == RplId.Invalid())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(spawned);
			evidence = "ordinary mixed-native rescue carrier proof lacks a valid replication identity";
			return false;
		}

		string runtimeId = ResolveEntityRuntimeId(spawned);
		if (runtimeId.IsEmpty() || runtimeId.Contains("vehicle_local_"))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(spawned);
			evidence = "ordinary mixed-native rescue carrier proof resolved only a process-local identity";
			return false;
		}

		IEntity existingBinding;
		if (!TryResolveRuntimeEntityBindingReadOnly(
			runtimeId,
			existingBinding,
			evidence)
			|| existingBinding
			|| m_aRuntimeEntities.Contains(spawned))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(spawned);
			if (evidence.IsEmpty())
				evidence = "ordinary mixed-native rescue carrier proof runtime identity is already bound";
			return false;
		}
		if (CountOrdinaryCampaignMixedNativeCarrierRecordsReadOnly(state, runtimeId) != 0)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(spawned);
			evidence = "ordinary mixed-native rescue carrier proof durable identity already exists";
			return false;
		}

		m_aRuntimeEntityIds.Insert(runtimeId);
		m_aRuntimeEntities.Insert(spawned);
		ApplyCampaignDebugEntityName(
			spawned,
			"ordinary_mixed_native_rescue_carrier",
			runtimeId);
		EnsureMissionCarrierVehicleRecord(
			state,
			spawned,
			runtimeId,
			"rescue carrier");

		IEntity verifiedBinding;
		HST_RuntimeVehicleState verifiedRecord;
		if (!TryResolveRuntimeEntityBindingReadOnly(
			runtimeId,
			verifiedBinding,
			evidence)
			|| verifiedBinding != spawned
			|| !TryResolveUniqueMissionCarrierRecordReadOnly(
				state,
				runtimeId,
				verifiedRecord,
				evidence)
			|| !verifiedRecord
			|| verifiedRecord.m_sPrefab.IsEmpty()
			|| verifiedRecord.m_bDeleted)
		{
			DeleteRuntimeEntity(runtimeId);
			RemoveOrdinaryCampaignMixedNativeCarrierRecords(state, runtimeId);
			if (evidence.IsEmpty())
				evidence = "ordinary mixed-native rescue carrier proof registration was not exact";
			return false;
		}

		carrierEntity = spawned;
		carrierRuntimeId = runtimeId;
		evidence = "ordinary mixed-native rescue carrier proof has one replicated binding and one durable mission-carrier row";
		return true;
	}

	// Returns true only when the entity is settled in an exact cargo seat. A
	// false result with a pending reason means the caller should poll again on a
	// later frame; the protected production boarding helper remains authoritative.
	bool TrySettleOrdinaryCampaignMixedNativeProofEntityInCarrier(
		IEntity entity,
		string carrierRuntimeId,
		out string seatToken,
		out string evidence,
		out bool terminalFailure,
		string expectedSeatToken = "")
	{
		seatToken = "";
		evidence = "ordinary mixed-native proof entity is not settled in its carrier";
		terminalFailure = false;
		if (!CanMutateOrdinaryCampaignMixedNativeProof(evidence))
		{
			terminalFailure = true;
			return false;
		}
		if (!entity || carrierRuntimeId.IsEmpty()
			|| carrierRuntimeId.Contains("vehicle_local_"))
		{
			evidence = "ordinary mixed-native proof boarding requires an entity and replicated carrier identity";
			terminalFailure = true;
			return false;
		}

		IEntity carrierEntity;
		if (!TryResolveRuntimeEntityBindingReadOnly(
			carrierRuntimeId,
			carrierEntity,
			evidence)
			|| !carrierEntity
			|| !IsReplicationBackedRuntimeBindingReadOnly(
				carrierRuntimeId,
				carrierEntity))
		{
			if (evidence.IsEmpty())
				evidence = "ordinary mixed-native proof carrier binding is absent or conflicting";
			return false;
		}

		IEntity observedCarrier;
		BaseCompartmentSlot observedSlot;
		if (InspectOrdinaryCampaignMixedNativeProofSeatReadOnly(
			entity,
			carrierRuntimeId,
			observedCarrier,
			observedSlot,
			seatToken,
			evidence))
			return true;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(
			entity.FindComponent(SCR_CompartmentAccessComponent));
		if (access && (access.IsGettingIn()
			|| access.IsGettingOut()
			|| access.IsSwitchingSeatsAnim()))
		{
			evidence = "ordinary mixed-native proof compartment transition is pending";
			return false;
		}
		if (access && access.IsInCompartment())
		{
			evidence = "ordinary mixed-native proof entity occupies a foreign or inexact compartment";
			terminalFailure = true;
			return false;
		}

		string boardingReason;
		if (!TryMoveCaptiveIntoVehicleClassified(
			entity,
			carrierEntity,
			boardingReason,
			terminalFailure,
			expectedSeatToken))
		{
			evidence = "ordinary mixed-native proof boarding was rejected: " + boardingReason;
			return false;
		}
		evidence = "ordinary mixed-native proof boarding was accepted and exact seat settlement is pending: "
			+ boardingReason;
		return false;
	}

	// Returns true only after the entity is fully outside every compartment and
	// no transition animation remains active.
	bool TrySettleOrdinaryCampaignMixedNativeProofEntityOutOfCarrier(
		IEntity entity,
		string expectedCarrierRuntimeId,
		out string evidence)
	{
		evidence = "ordinary mixed-native proof entity has not settled outside its carrier";
		if (!CanMutateOrdinaryCampaignMixedNativeProof(evidence))
			return false;
		if (!entity)
		{
			evidence = "ordinary mixed-native proof disembark entity is missing";
			return false;
		}

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(
			entity.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
		{
			evidence = "ordinary mixed-native proof disembark entity has no compartment access";
			return false;
		}
		if (access.IsGettingIn() || access.IsGettingOut()
			|| access.IsSwitchingSeatsAnim())
		{
			evidence = "ordinary mixed-native proof disembark transition is pending";
			return false;
		}
		if (!access.IsInCompartment())
		{
			evidence = "ordinary mixed-native proof entity is settled outside every compartment";
			return true;
		}

		IEntity occupiedVehicle = access.GetVehicle();
		if (!occupiedVehicle || (!expectedCarrierRuntimeId.IsEmpty()
			&& ResolveEntityRuntimeId(occupiedVehicle) != expectedCarrierRuntimeId))
		{
			evidence = "ordinary mixed-native proof entity occupies an unexpected carrier";
			return false;
		}
		RplComponent occupantReplication = RplComponent.Cast(
			entity.FindComponent(RplComponent));
		if ((!occupantReplication || occupantReplication.IsOwner())
			&& access.GetOutVehicle(
				EGetOutType.TELEPORT,
				-1,
				ECloseDoorAfterActions.INVALID,
				false,
				true))
		{
			evidence
				= "ordinary mixed-native proof authority-local disembark was accepted and exact settlement is pending";
			return false;
		}
		access.AskOwnerToGetOutFromVehicle(
			EGetOutType.TELEPORT,
			-1,
			ECloseDoorAfterActions.INVALID,
			false,
			true);
		evidence
			= "ordinary mixed-native proof owner-routed disembark was accepted and exact settlement is pending";
		return false;
	}

	bool SuspendOrdinaryCampaignMixedNativeOnFootFollowProof(
		IEntity followingEntity,
		IEntity boardingEntity,
		out string evidence)
	{
		evidence
			= "ordinary mixed-native on-foot follow suspension is unavailable";
		if (!CanMutateOrdinaryCampaignMixedNativeProof(evidence))
			return false;
		if (!followingEntity || followingEntity.IsDeleted()
			|| !followingEntity.GetWorld()
			|| !boardingEntity || boardingEntity.IsDeleted()
			|| !boardingEntity.GetWorld())
			return false;

		SCR_CompartmentAccessComponent followingAccess
			= SCR_CompartmentAccessComponent.Cast(
				followingEntity.FindComponent(SCR_CompartmentAccessComponent));
		SCR_CompartmentAccessComponent boardingAccess
			= SCR_CompartmentAccessComponent.Cast(
				boardingEntity.FindComponent(SCR_CompartmentAccessComponent));
		if (!followingAccess || followingAccess.IsGettingIn()
			|| followingAccess.IsGettingOut()
			|| followingAccess.IsSwitchingSeatsAnim()
			|| followingAccess.IsInCompartment()
			|| !boardingAccess || boardingAccess.IsGettingIn()
			|| boardingAccess.IsGettingOut()
			|| boardingAccess.IsSwitchingSeatsAnim()
			|| boardingAccess.IsInCompartment())
		{
			evidence
				= "ordinary mixed-native follow suspension requires two settled on-foot captives";
			return false;
		}
		HST_MissionCaptiveFollowComponent following
			= HST_MissionCaptiveFollowComponent.Cast(
				followingEntity.FindComponent(
					HST_MissionCaptiveFollowComponent));
		HST_MissionCaptiveFollowComponent boarding
			= HST_MissionCaptiveFollowComponent.Cast(
				boardingEntity.FindComponent(
					HST_MissionCaptiveFollowComponent));
		if (!following || !boarding)
		{
			evidence
				= "ordinary mixed-native on-foot captive pair lacks a follow controller";
			return false;
		}
		following.StopFollowing();
		boarding.StopFollowing();
		if (following.IsFollowing() || boarding.IsFollowing())
		{
			evidence
				= "ordinary mixed-native on-foot captive follow controller pair remained active";
			return false;
		}
		evidence
			= "ordinary mixed-native on-foot captive follow controller pair is suspended before player occupancy";
		return true;
	}

	// Read-only exact evidence for the actual engine compartment graph. The
	// reverse occupant link, manager/root ancestry, carrier binding, and stable
	// seat token must all agree before this returns true.
	bool InspectOrdinaryCampaignMixedNativeProofSeatReadOnly(
		IEntity occupant,
		string expectedCarrierRuntimeId,
		out IEntity carrierEntity,
		out BaseCompartmentSlot occupiedSlot,
		out string seatToken,
		out string evidence)
	{
		carrierEntity = null;
		occupiedSlot = null;
		seatToken = "";
		evidence = "ordinary mixed-native proof seat topology is unavailable";
		if (!occupant || occupant.IsDeleted() || !occupant.GetWorld()
			|| expectedCarrierRuntimeId.IsEmpty()
			|| expectedCarrierRuntimeId.Contains("vehicle_local_"))
			return false;

		IEntity boundCarrier;
		if (!TryResolveRuntimeEntityBindingReadOnly(
			expectedCarrierRuntimeId,
			boundCarrier,
			evidence)
			|| !boundCarrier
			|| boundCarrier.IsDeleted()
			|| !boundCarrier.GetWorld()
			|| !IsReplicationBackedRuntimeBindingReadOnly(
				expectedCarrierRuntimeId,
				boundCarrier))
		{
			if (evidence.IsEmpty())
				evidence = "ordinary mixed-native proof carrier binding is absent or conflicting";
			return false;
		}

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(
			occupant.FindComponent(SCR_CompartmentAccessComponent));
		if (!access || access.IsGettingIn() || access.IsGettingOut()
			|| access.IsSwitchingSeatsAnim() || !access.IsInCompartment())
		{
			evidence = "ordinary mixed-native proof occupant is not settled in a compartment";
			return false;
		}

		IEntity observedCarrier = access.GetVehicle();
		BaseCompartmentSlot slot = access.GetCompartment();
		BaseCompartmentManagerComponent manager;
		IEntity managerOwner;
		if (slot)
		{
			manager = slot.GetManager();
			if (manager)
				managerOwner = manager.GetOwner();
		}
		if (!observedCarrier || observedCarrier != boundCarrier
			|| !slot || slot.GetOccupant() != occupant
			|| !manager || !managerOwner
			|| !IsEntityWithinControlledShutdownRoot(
				managerOwner,
				boundCarrier)
			|| !IsEntityWithinControlledShutdownRoot(
				observedCarrier,
				boundCarrier))
		{
			evidence = "ordinary mixed-native proof seat reverse occupant or carrier-root topology is not exact";
			return false;
		}

		string observedSeatToken = BuildExactRescueCarrierSeatToken(slot);
		if (observedSeatToken.IsEmpty())
		{
			evidence = "ordinary mixed-native proof exact seat token is empty";
			return false;
		}

		carrierEntity = boundCarrier;
		occupiedSlot = slot;
		seatToken = observedSeatToken;
		evidence = "ordinary mixed-native proof access, carrier root, reverse occupant, and seat token are read-only exact";
		return true;
	}

	// This entity is deliberately omitted from every HST runtime registry so the
	// rescue preflight can prove that an otherwise-real occupant is foreign. The
	// retained handle exists solely for bounded proof cleanup before any latch.
	bool SpawnOrdinaryCampaignMixedNativeForeignOccupantProof(
		vector requestedPosition,
		out IEntity occupantEntity,
		out string evidence)
	{
		occupantEntity = null;
		evidence = "ordinary mixed-native foreign occupant proof is unavailable";
		if (!CanMutateOrdinaryCampaignMixedNativeProof(evidence))
			return false;
		if (IsZeroVector(requestedPosition))
		{
			evidence = "ordinary mixed-native foreign occupant proof requires a nonzero position";
			return false;
		}
		if (m_OrdinaryCampaignMixedNativeForeignOccupantProof
			&& !m_OrdinaryCampaignMixedNativeForeignOccupantProof.IsDeleted()
			&& m_OrdinaryCampaignMixedNativeForeignOccupantProof.GetWorld())
		{
			evidence = "ordinary mixed-native foreign occupant proof already owns a live entity";
			return false;
		}

		vector spawnPosition = HST_WorldPositionService.ResolveSafeGroundPosition(
			requestedPosition,
			HST_WorldPositionService.CHARACTER_GROUND_OFFSET,
			false,
			2.0);
		GenericEntity spawned = HST_WorldPositionService.SpawnPrefab(
			PROP_GUN_SHOP_DELIVERY_DRIVER,
			spawnPosition,
			"0 0 0");
		if (!spawned)
		{
			evidence = "ordinary mixed-native foreign occupant proof could not spawn a native character";
			return false;
		}
		BaseRplComponent replication = BaseRplComponent.Cast(
			spawned.FindComponent(BaseRplComponent));
		if (!replication || replication.Id() == RplId.Invalid()
			|| m_aRuntimeEntities.Contains(spawned))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(spawned);
			evidence = "ordinary mixed-native foreign occupant proof is not a unique replicated native entity";
			return false;
		}
		string proofRuntimeId = ResolveEntityRuntimeId(spawned);
		IEntity conflictingBinding;
		if (proofRuntimeId.IsEmpty() || proofRuntimeId.Contains("vehicle_local_")
			|| !TryResolveRuntimeEntityBindingReadOnly(
				proofRuntimeId,
				conflictingBinding,
				evidence)
			|| conflictingBinding)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(spawned);
			if (evidence.IsEmpty())
				evidence = "ordinary mixed-native foreign occupant proof identity conflicts with a runtime binding";
			return false;
		}

		ApplyCampaignDebugEntityName(
			spawned,
			"ordinary_mixed_native_foreign_occupant",
			string.Format("foreign_%1", replication.Id()));
		m_OrdinaryCampaignMixedNativeForeignOccupantProof = spawned;
		occupantEntity = spawned;
		evidence = "ordinary mixed-native foreign occupant proof owns one unregistered replicated native character";
		return true;
	}

	bool CleanupOrdinaryCampaignMixedNativeForeignOccupantProof(
		IEntity expectedOccupant,
		string expectedCarrierRuntimeId,
		out string evidence)
	{
		evidence = "ordinary mixed-native foreign occupant proof cleanup is unavailable";
		if (!CanMutateOrdinaryCampaignMixedNativeProof(evidence))
			return false;
		if (!m_OrdinaryCampaignMixedNativeForeignOccupantProof)
		{
			evidence = "ordinary mixed-native foreign occupant proof is already clean";
			return true;
		}
		if (expectedOccupant
			&& expectedOccupant != m_OrdinaryCampaignMixedNativeForeignOccupantProof)
		{
			evidence = "ordinary mixed-native foreign occupant proof cleanup handle conflicts";
			return false;
		}
		if (m_aRuntimeEntities.Contains(
			m_OrdinaryCampaignMixedNativeForeignOccupantProof))
		{
			evidence = "ordinary mixed-native foreign occupant proof was unexpectedly registered";
			return false;
		}
		if (m_OrdinaryCampaignMixedNativeForeignOccupantProof.IsDeleted()
			|| !m_OrdinaryCampaignMixedNativeForeignOccupantProof.GetWorld())
		{
			m_OrdinaryCampaignMixedNativeForeignOccupantProof = null;
			evidence = "ordinary mixed-native foreign occupant proof dead handle was cleared";
			return true;
		}

		string disembarkEvidence;
		if (!TrySettleOrdinaryCampaignMixedNativeProofEntityOutOfCarrier(
			m_OrdinaryCampaignMixedNativeForeignOccupantProof,
			expectedCarrierRuntimeId,
			disembarkEvidence))
		{
			evidence = "ordinary mixed-native foreign occupant proof cleanup pending: "
				+ disembarkEvidence;
			return false;
		}

		SCR_EntityHelper.DeleteEntityAndChildren(
			m_OrdinaryCampaignMixedNativeForeignOccupantProof);
		m_OrdinaryCampaignMixedNativeForeignOccupantProof = null;
		evidence = "ordinary mixed-native foreign occupant proof disembarked and deleted exactly";
		return true;
	}

	protected bool CanMutateOrdinaryCampaignMixedNativeProof(out string evidence)
	{
		if (!m_PhysicalWar)
		{
			evidence = "ordinary mixed-native proof cannot verify the controlled-shutdown latch";
			return false;
		}
		if (m_PhysicalWar.HasControlledShutdownActiveGroupQuiescenceApplied())
		{
			evidence = "ordinary mixed-native proof mutation is locked by controlled-shutdown active-group quiescence";
			return false;
		}
		return true;
	}

	protected int CountOrdinaryCampaignMixedNativeCarrierRecordsReadOnly(
		HST_CampaignState state,
		string carrierRuntimeId)
	{
		if (!state || carrierRuntimeId.IsEmpty())
			return 0;
		int count;
		foreach (HST_RuntimeVehicleState candidate : state.m_aRuntimeVehicles)
		{
			if (candidate
				&& candidate.m_sVehicleRuntimeId == carrierRuntimeId)
				count++;
		}
		return count;
	}

	protected void RemoveOrdinaryCampaignMixedNativeCarrierRecords(
		HST_CampaignState state,
		string carrierRuntimeId)
	{
		if (!state || carrierRuntimeId.IsEmpty())
			return;
		for (int index = state.m_aRuntimeVehicles.Count() - 1; index >= 0; index--)
		{
			HST_RuntimeVehicleState candidate = state.m_aRuntimeVehicles[index];
			if (candidate
				&& candidate.m_sVehicleRuntimeId == carrierRuntimeId)
				state.m_aRuntimeVehicles.Remove(index);
		}
	}

	HST_CampaignDebugCaseResult BuildCampaignDebugCaptiveBoardingProbe(HST_CampaignState state, HST_ActiveMissionState mission, string carrierPrefab, string debugPrefix, bool physicalBlocked)
	{
		HST_CampaignDebugCaseResult probe = CreateCaptiveBoardingDebugProbeCase(state, mission);
		string instanceId = "";
		string zoneId = "";
		if (mission)
		{
			instanceId = mission.m_sInstanceId;
			zoneId = mission.m_sTargetZoneId;
		}

		if (physicalBlocked)
		{
			AddCaptiveBoardingDebugAssertion(probe, "rescue.captive.boarding.player_presence", "physical runtime tests not blocked", "blocked", "BLOCKED", "bootstrap marked physical runtime tests blocked", "", instanceId, zoneId);
			FinalizeCaptiveBoardingDebugProbeCase(state, probe);
			return probe;
		}
		if (!state || !mission)
		{
			AddCaptiveBoardingDebugAssertion(probe, "rescue.captive.boarding.prerequisite", "campaign state and mission exist", "missing", "BLOCKED", "captive boarding probe missing state or mission", "", instanceId, zoneId);
			FinalizeCaptiveBoardingDebugProbeCase(state, probe);
			return probe;
		}
		if (carrierPrefab.IsEmpty())
		{
			AddCaptiveBoardingDebugAssertion(probe, "rescue.captive.boarding.carrier_prefab", "carrier prefab configured", "missing", "BLOCKED", "captive boarding probe missing carrier prefab", "", instanceId, zoneId);
			FinalizeCaptiveBoardingDebugProbeCase(state, probe);
			return probe;
		}

		string sourceId = debugPrefix;
		if (sourceId.IsEmpty())
			sourceId = CAMPAIGN_DEBUG_PREFIX_ROOT + "captive_boarding";
		string assetId = sourceId + "_captive_boarding_asset_" + state.m_iElapsedSeconds;
		string entityId = sourceId + "_captive_boarding_entity_" + state.m_iElapsedSeconds;
		string carrierId = sourceId + "_captive_boarding_carrier_" + state.m_iElapsedSeconds;
		vector basePosition = ResolveCaptiveBoardingDebugBasePosition(state, mission);
		vector captivePosition = HST_WorldPositionService.ResolveSafeGroundPosition(basePosition + "3 0 0", HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 2.0);
		vector carrierPosition;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(basePosition + "8 0 0", carrierPosition, true))
			carrierPosition = HST_WorldPositionService.ResolveSafeGroundPosition(basePosition + "8 0 0", HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true, 5.0);

		HST_MissionAssetState probeAsset = new HST_MissionAssetState();
		probeAsset.m_sAssetId = assetId;
		probeAsset.m_sMissionInstanceId = mission.m_sInstanceId;
		probeAsset.m_sKind = ASSET_KIND_CAPTIVE;
		probeAsset.m_sRole = ROLE_CAPTIVE;
		probeAsset.m_sPrefab = PROP_CAPTIVES;
		probeAsset.m_sEntityId = entityId;
		probeAsset.m_sLastInteraction = PHASE_FREED;
		probeAsset.m_bAlive = true;
		probeAsset.m_bPickedUp = true;
		probeAsset.m_vSourcePosition = captivePosition;
		probeAsset.m_vCurrentPosition = captivePosition;
		probeAsset.m_vLastKnownPosition = captivePosition;
		probeAsset.m_vTargetPosition = mission.m_vTargetPosition;
		state.m_aMissionAssets.Insert(probeAsset);

		GenericEntity carrierEntity = HST_WorldPositionService.SpawnPrefab(carrierPrefab, carrierPosition, HST_WorldPositionService.BuildUprightAngles(0.0));
		if (carrierEntity)
			HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(carrierEntity);
		ApplyCampaignDebugEntityName(carrierEntity, "captive_boarding_carrier", carrierId);
		IEntity captiveEntity = SpawnCaptiveFollowerProjection(state, mission, probeAsset, captivePosition, "0 0 0");
		AddCaptiveBoardingDebugMetric(probe, "rescue.captive.boarding.spawn_distance", string.Format("%1", Math.Round(Math.Sqrt(DistanceSq2D(captivePosition, carrierPosition)))), "meters");
		AddCaptiveBoardingDebugAssertion(probe, "rescue.captive.boarding.temp_asset", "temporary captive mission asset created", BuildCaptiveBoardingDebugAssetActual(probeAsset), CaptiveBoardingDebugStatus(probeAsset != null), "temporary captive asset was not created", assetId, instanceId, zoneId);
		AddCaptiveBoardingDebugAssertion(probe, "rescue.captive.boarding.captive_entity", "temporary captive projection spawned", string.Format("entity %1 | position %2", captiveEntity != null, captivePosition), CaptiveBoardingDebugStatus(captiveEntity != null), "temporary captive projection did not spawn", assetId, instanceId, zoneId);
		AddCaptiveBoardingDebugAssertion(probe, "rescue.captive.boarding.carrier_entity", "temporary carrier spawned", string.Format("entity %1 | prefab %2 | position %3", carrierEntity != null, carrierPrefab, carrierPosition), CaptiveBoardingDebugStatus(carrierEntity != null), "temporary carrier did not spawn", carrierId, instanceId, zoneId);

		string boardingReason;
		bool boardingAccepted = false;
		SCR_CompartmentAccessComponent boardingAccess = null;
		if (captiveEntity && carrierEntity)
		{
			boardingAccepted = TryMoveCaptiveIntoVehicle(captiveEntity, carrierEntity, boardingReason);
			boardingAccess = SCR_CompartmentAccessComponent.Cast(captiveEntity.FindComponent(SCR_CompartmentAccessComponent));
		}
		else
		{
			boardingReason = "captive or carrier missing";
		}

		bool captiveInCarrier = false;
		bool captiveGettingIn = false;
		int boardingStateSamples = 0;
		if (boardingAccess)
		{
			RefreshCaptiveBoardingDebugState(boardingAccess, carrierEntity, captiveInCarrier, captiveGettingIn, boardingStateSamples);
		}
		if (boardingAccepted)
		{
			probeAsset.m_sLastInteraction = PHASE_LOADED;
			probeAsset.m_bAttachedToCarrier = true;
			probeAsset.m_sCarriedByVehicleId = carrierId;
			probeAsset.m_vCurrentPosition = carrierPosition;
			probeAsset.m_vLastKnownPosition = carrierPosition;
		}
		bool boardingAssociated = boardingAccepted && probeAsset.m_sLastInteraction == PHASE_LOADED && probeAsset.m_bAttachedToCarrier && probeAsset.m_sCarriedByVehicleId == carrierId;

		string boardingActual = string.Format("accepted %1 | seated %2 | gettingIn %3 | samples %4 | reason %5", boardingAccepted, captiveInCarrier, captiveGettingIn, boardingStateSamples, boardingReason);
		AddCaptiveBoardingDebugMetric(probe, "rescue.captive.boarding.state_samples", string.Format("%1", boardingStateSamples), "count");
		AddCaptiveBoardingDebugAssertion(probe, "rescue.captive.boarding.command", "runtime captive boarding helper accepts a cargo seat", boardingActual, CaptiveBoardingDebugStatus(boardingAccepted), "captive boarding helper rejected the temporary carrier", assetId, instanceId, zoneId);
		AddCaptiveBoardingDebugAssertion(probe, "rescue.captive.boarding.seat_state", "captive is seated, actively boarding, or authoritatively associated with the temporary carrier after bounded state rescan", boardingActual + string.Format(" | associated %1", boardingAssociated), CaptiveBoardingDebugStatus(captiveInCarrier || captiveGettingIn || boardingAssociated), "captive boarding command did not produce a seated/getting-in state or authoritative loaded association after bounded rescan", assetId, instanceId, zoneId);
		AddCaptiveBoardingDebugAssertion(probe, "rescue.captive.boarding.asset_loaded_state", "temporary captive asset records loaded carrier state", BuildCaptiveBoardingDebugAssetActual(probeAsset), CaptiveBoardingDebugStatus(boardingAssociated), "temporary captive asset did not enter loaded carrier state", assetId, instanceId, zoneId);

		vector transportPosition;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(basePosition + "35 0 0", transportPosition, true))
			transportPosition = HST_WorldPositionService.ResolveSafeGroundPosition(basePosition + "35 0 0", HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true, 5.0);
		bool carrierMoved = false;
		float carrierMoveDistance = 0.0;
		if (carrierEntity)
		{
			vector carrierBeforeMove = carrierEntity.GetOrigin();
			HST_WorldPositionService.ApplyUprightEntityTransform(carrierEntity, transportPosition, carrierEntity.GetYawPitchRoll());
			carrierMoveDistance = Math.Sqrt(DistanceSq2D(carrierBeforeMove, carrierEntity.GetOrigin()));
			carrierMoved = carrierMoveDistance >= 10.0;
		}
		bool captiveStillInCarrier = false;
		bool captiveStillGettingIn = false;
		int transportStateSamples = 0;
		if (boardingAccess)
			RefreshCaptiveBoardingDebugState(boardingAccess, carrierEntity, captiveStillInCarrier, captiveStillGettingIn, transportStateSamples);
		if (carrierMoved && boardingAccepted)
		{
			probeAsset.m_vCurrentPosition = transportPosition;
			probeAsset.m_vLastKnownPosition = transportPosition;
		}
		bool transportAssociated = carrierMoved && boardingAssociated && DistanceSq2D(probeAsset.m_vCurrentPosition, transportPosition) <= 4.0;

		string transportActual = string.Format("moved %1 | distance %2m | still seated %3 | gettingIn %4 | associated %5 | samples %6 | position %7", carrierMoved, Math.Round(carrierMoveDistance), captiveStillInCarrier, captiveStillGettingIn, transportAssociated, transportStateSamples, transportPosition);
		AddCaptiveBoardingDebugMetric(probe, "rescue.captive.boarding.transport_distance", string.Format("%1", Math.Round(carrierMoveDistance)), "meters");
		AddCaptiveBoardingDebugMetric(probe, "rescue.captive.boarding.transport_state_samples", string.Format("%1", transportStateSamples), "count");
		AddCaptiveBoardingDebugAssertion(probe, "rescue.captive.boarding.transport", "loaded captive remains associated with carrier after carrier movement", transportActual, CaptiveBoardingDebugStatus(carrierMoved && (captiveStillInCarrier || captiveStillGettingIn || transportAssociated)), "temporary captive did not remain associated with the carrier after movement", assetId, instanceId, zoneId);

		bool disembarked = false;
		if (boardingAccess && boardingAccess.IsInCompartment())
			disembarked = boardingAccess.GetOutVehicle(EGetOutType.TELEPORT, -1, ECloseDoorAfterActions.INVALID, false);
		DeleteRuntimeEntity(probeAsset.m_sEntityId);
		if (carrierEntity)
			SCR_EntityHelper.DeleteEntityAndChildren(carrierEntity);
		bool recordsRemoved = RemoveCaptiveBoardingDebugRecords(state, assetId, entityId);
		bool runtimeRemoved = !HasRuntimeEntity(entityId);
		string cleanupActual = string.Format("disembarked %1 | records removed %2 | runtime removed %3", disembarked, recordsRemoved, runtimeRemoved);
		AddCaptiveBoardingDebugAssertion(probe, "rescue.captive.boarding.cleanup", "temporary captive boarding entities and records are cleaned", cleanupActual, CaptiveBoardingDebugStatus(recordsRemoved && runtimeRemoved), "temporary captive boarding probe leaked records or runtime entities", assetId, instanceId, zoneId);

		FinalizeCaptiveBoardingDebugProbeCase(state, probe);
		return probe;
	}

	protected void RefreshCaptiveBoardingDebugState(SCR_CompartmentAccessComponent access, IEntity carrierEntity, out bool captiveInCarrier, out bool captiveGettingIn, out int samples)
	{
		captiveInCarrier = false;
		captiveGettingIn = false;
		samples = 0;
		if (!access || !carrierEntity)
			return;

		for (int i = 0; i < 5; i++)
		{
			samples++;
			captiveInCarrier = access.IsInCompartment() && access.GetVehicle() == carrierEntity;
			captiveGettingIn = access.IsGettingIn();
			if (captiveInCarrier || captiveGettingIn)
				return;
		}
	}

	HST_CampaignDebugCaseResult BuildCampaignDebugExpiredPlayerBoundBubbleProbe(HST_CampaignState state, string debugPrefix, bool physicalBlocked)
	{
		HST_CampaignDebugCaseResult probe = CreateExpiredPlayerBoundBubbleDebugCase(state);
		if (physicalBlocked)
		{
			AddExpiredPlayerBoundBubbleDebugAssertion(probe, "render_bubble.mission_asset.player_presence", "physical runtime tests not blocked", "blocked", "BLOCKED", "bootstrap marked physical runtime tests blocked");
			FinalizeExpiredPlayerBoundBubbleDebugCase(state, probe);
			return probe;
		}
		if (!state)
		{
			AddExpiredPlayerBoundBubbleDebugAssertion(probe, "render_bubble.mission_asset.prerequisite", "campaign state exists", "missing", "BLOCKED", "expired mission asset bubble probe missing campaign state");
			FinalizeExpiredPlayerBoundBubbleDebugCase(state, probe);
			return probe;
		}

		int playerId;
		vector playerPosition;
		if (!ResolveFirstLivingPlayerDebugContext(playerId, playerPosition))
		{
			AddExpiredPlayerBoundBubbleDebugAssertion(probe, "render_bubble.mission_asset.player", "living player entity exists", "missing", "BLOCKED", "expired mission asset bubble probe requires a living player");
			FinalizeExpiredPlayerBoundBubbleDebugCase(state, probe);
			return probe;
		}

		string sourceId = debugPrefix;
		if (sourceId.IsEmpty())
			sourceId = CAMPAIGN_DEBUG_PREFIX_ROOT + "expired_asset_bubble";
		string instanceId = sourceId + "_expired_asset_bubble_mission_" + state.m_iElapsedSeconds;
		string assetId = sourceId + "_expired_asset_bubble_asset_" + state.m_iElapsedSeconds;
		string entityId = sourceId + "_expired_asset_bubble_entity_" + state.m_iElapsedSeconds;
		vector nearPosition = HST_WorldPositionService.ResolveSafeGroundPosition(playerPosition + "2 0 2", HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 2.0);
		vector farPosition = ResolveExpiredPlayerBoundBubbleDebugFarPosition(playerPosition);
		float farDistance = Math.Sqrt(DistanceSq2D(playerPosition, farPosition));

		HST_ActiveMissionState mission = new HST_ActiveMissionState();
		mission.m_sInstanceId = instanceId;
		mission.m_sMissionId = "debug_expired_asset_bubble";
		mission.m_sDisplayName = "Debug Expired Asset Bubble";
		mission.m_eStatus = HST_EMissionStatus.HST_MISSION_EXPIRED;
		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_STATE_MACHINE;
		mission.m_sRuntimePrimitive = PRIMITIVE_RESCUE_EXTRACT;
		mission.m_sRuntimeType = PRIMITIVE_RESCUE_EXTRACT;
		mission.m_sRuntimePhase = "expired";
		mission.m_iRuntimeStartedAtSecond = state.m_iElapsedSeconds;
		mission.m_iRemainingSeconds = 0;
		mission.m_iRequiredCaptiveCount = 1;
		mission.m_vTargetPosition = nearPosition;
		state.m_aActiveMissions.Insert(mission);

		HST_MissionAssetState asset = new HST_MissionAssetState();
		asset.m_sAssetId = assetId;
		asset.m_sMissionInstanceId = instanceId;
		asset.m_sKind = ASSET_KIND_CAPTIVE;
		asset.m_sRole = ROLE_CAPTIVE;
		asset.m_sPrefab = PROP_CAPTIVES;
		asset.m_sEntityId = entityId;
		asset.m_sLastInteraction = PHASE_FOLLOWING;
		asset.m_bAlive = true;
		asset.m_bPickedUp = true;
		asset.m_vSourcePosition = nearPosition;
		asset.m_vCurrentPosition = nearPosition;
		asset.m_vLastKnownPosition = nearPosition;
		asset.m_vTargetPosition = playerPosition;
		state.m_aMissionAssets.Insert(asset);

		bool nearContinue = ShouldContinueExpiredPlayerBoundMissionRuntime(state, mission);
		bool nearCanComplete = CanCompleteExpiredPlayerBoundMission(state, mission);
		bool nearFollowAllowed = IsExpiredPlayerBoundMissionInteractionAllowed(state, mission, asset, "mission_captive_follow");
		bool nearExtractAllowed = IsExpiredPlayerBoundMissionInteractionAllowed(state, mission, asset, "mission_captive_extract");
		string nearActual = BuildExpiredPlayerBoundBubbleDebugActual(mission, asset, nearContinue, nearFollowAllowed, nearExtractAllowed, nearCanComplete);

		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_sLastInteraction = PHASE_FOLLOWING;
		asset.m_vCurrentPosition = farPosition;
		asset.m_vLastKnownPosition = farPosition;
		bool farContinue = ShouldContinueExpiredPlayerBoundMissionRuntime(state, mission);
		bool farFollowAllowed = IsExpiredPlayerBoundMissionInteractionAllowed(state, mission, asset, "mission_captive_follow");
		bool farExtractAllowed = IsExpiredPlayerBoundMissionInteractionAllowed(state, mission, asset, "mission_captive_extract");
		bool farCanComplete = CanCompleteExpiredPlayerBoundMission(state, mission);
		string farActual = BuildExpiredPlayerBoundBubbleDebugActual(mission, asset, farContinue, farFollowAllowed, farExtractAllowed, farCanComplete);

		asset.m_bAttachedToCarrier = true;
		asset.m_sCarriedByVehicleId = BuildPlayerCarrierId(playerId);
		asset.m_sLastInteraction = PHASE_LOADED;
		asset.m_vCurrentPosition = farPosition;
		asset.m_vLastKnownPosition = farPosition;
		bool carrierContinue = ShouldContinueExpiredPlayerBoundMissionRuntime(state, mission);
		bool carrierExtractAllowed = IsExpiredPlayerBoundMissionInteractionAllowed(state, mission, asset, "mission_captive_extract");
		bool carrierCanComplete = CanCompleteExpiredPlayerBoundMission(state, mission);
		string carrierActual = BuildExpiredPlayerBoundBubbleDebugActual(mission, asset, carrierContinue, false, carrierExtractAllowed, carrierCanComplete);

		DeleteRuntimeEntity(entityId);
		bool recordsRemoved = RemoveExpiredPlayerBoundBubbleDebugRecords(state, instanceId, assetId, entityId);
		bool runtimeRemoved = !HasRuntimeEntity(entityId);
		string cleanupActual = string.Format("records removed %1 | runtime removed %2 | instance %3 | asset %4", recordsRemoved, runtimeRemoved, instanceId, assetId);

		AddExpiredPlayerBoundBubbleDebugMetric(probe, "render_bubble.mission_asset.radius", string.Format("%1", Math.Round(POST_EXPIRY_PLAYER_ASSET_BUBBLE_METERS)), "m");
		AddExpiredPlayerBoundBubbleDebugMetric(probe, "render_bubble.mission_asset.far_distance", string.Format("%1", Math.Round(farDistance)), "m");
		AddExpiredPlayerBoundBubbleDebugAssertion(probe, "render_bubble.mission_asset.near_continue", "expired picked-up captive inside player bubble continues runtime", nearActual, ExpiredPlayerBoundBubbleDebugStatus(nearContinue), "near expired captive did not remain player-bound", assetId, instanceId);
		AddExpiredPlayerBoundBubbleDebugAssertion(probe, "render_bubble.mission_asset.near_interactions", "expired picked-up captive inside bubble keeps follow/extract interaction eligibility", nearActual, ExpiredPlayerBoundBubbleDebugStatus(nearFollowAllowed && nearExtractAllowed && nearCanComplete), "near expired captive interactions were not allowed", assetId, instanceId);
		AddExpiredPlayerBoundBubbleDebugAssertion(probe, "render_bubble.mission_asset.far_stop", "expired uncarried captive outside player bubble stops runtime and interaction eligibility", farActual, ExpiredPlayerBoundBubbleDebugStatus(!farContinue && !farFollowAllowed && !farExtractAllowed && !farCanComplete), "far expired captive remained eligible outside the player bubble", assetId, instanceId);
		AddExpiredPlayerBoundBubbleDebugAssertion(probe, "render_bubble.mission_asset.player_carrier", "expired captive attached to living player carrier remains player-bound even after position drift", carrierActual, ExpiredPlayerBoundBubbleDebugStatus(carrierContinue && carrierExtractAllowed && carrierCanComplete), "player-carried expired captive was not treated as player-bound", assetId, instanceId);
		AddExpiredPlayerBoundBubbleDebugAssertion(probe, "render_bubble.mission_asset.cleanup", "temporary expired mission and asset records are cleaned", cleanupActual, ExpiredPlayerBoundBubbleDebugStatus(recordsRemoved && runtimeRemoved), "expired mission asset bubble probe leaked temporary records", assetId, instanceId);
		FinalizeExpiredPlayerBoundBubbleDebugCase(state, probe);
		return probe;
	}

	protected HST_CampaignDebugCaseResult CreateCaptiveBoardingDebugProbeCase(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_CampaignDebugCaseResult probe = new HST_CampaignDebugCaseResult();
		string instanceId = "missing";
		string missionId = "missing";
		if (mission)
		{
			instanceId = mission.m_sInstanceId;
			missionId = mission.m_sMissionId;
		}

		probe.m_sCaseId = "rescue_extract.captive_boarding." + SanitizeCampaignDebugEntityToken(missionId) + "." + SanitizeCampaignDebugEntityToken(instanceId);
		probe.m_sCategory = "mission_runtime";
		probe.m_sFeature = "rescue_extract";
		probe.m_sStage = "physical_probe";
		probe.m_sStatus = "PASS";
		if (state)
		{
			probe.m_iStartSecond = state.m_iElapsedSeconds;
			probe.m_iEndSecond = state.m_iElapsedSeconds;
		}
		probe.m_aEvidence.Insert("temporary debug captive asset uses the normal captive boarding helper and is removed before the case is recorded");
		return probe;
	}

	protected HST_CampaignDebugAssertion AddCaptiveBoardingDebugAssertion(HST_CampaignDebugCaseResult probe, string assertionId, string expected, string actual, string status, string failureReason = "", string entityId = "", string missionInstanceId = "", string zoneId = "")
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
		probe.m_aAssertions.Insert(assertion);
		return assertion;
	}

	protected void AddCaptiveBoardingDebugMetric(HST_CampaignDebugCaseResult probe, string metricId, string value, string unit = "")
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
		metric.m_sMissionInstanceId = ResolveCaptiveBoardingDebugMissionInstanceId(probe);
		probe.m_aMetrics.Insert(metric);
	}

	protected string ResolveCaptiveBoardingDebugMissionInstanceId(HST_CampaignDebugCaseResult probe)
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

	protected void FinalizeCaptiveBoardingDebugProbeCase(HST_CampaignState state, HST_CampaignDebugCaseResult probe)
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
			resolvedReason = "temporary captive boarding probe assertions passed";
		probe.m_sReason = resolvedReason;
		if (state)
			probe.m_iEndSecond = state.m_iElapsedSeconds;
	}

	protected string CaptiveBoardingDebugStatus(bool passed, string failStatus = "FAIL")
	{
		if (passed)
			return "PASS";

		return failStatus;
	}

	protected string BuildCaptiveBoardingDebugAssetActual(HST_MissionAssetState asset)
	{
		if (!asset)
			return "asset missing";

		return string.Format("asset %1 | phase %2 | pickedUp %3 | attached %4 | carrier %5 | entity %6", ReportText(asset.m_sAssetId), ReportText(asset.m_sLastInteraction), ReportBool(asset.m_bPickedUp), ReportBool(asset.m_bAttachedToCarrier), ReportText(asset.m_sCarriedByVehicleId), ReportText(asset.m_sEntityId));
	}

	protected vector ResolveCaptiveBoardingDebugBasePosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (mission && !IsZeroVector(mission.m_vTargetPosition))
			return mission.m_vTargetPosition;
		if (state && !IsZeroVector(state.m_vHQPosition))
			return state.m_vHQPosition;
		if (state && !IsZeroVector(state.m_vPetrosPosition))
			return state.m_vPetrosPosition;

		return "0 0 0";
	}

	protected bool RemoveCaptiveBoardingDebugRecords(HST_CampaignState state, string assetId, string entityId)
	{
		if (!state)
			return false;

		bool assetRemoved = false;
		for (int assetIndex = state.m_aMissionAssets.Count() - 1; assetIndex >= 0; assetIndex--)
		{
			HST_MissionAssetState asset = state.m_aMissionAssets[assetIndex];
			if (!asset || asset.m_sAssetId != assetId)
				continue;

			state.m_aMissionAssets.Remove(assetIndex);
			assetRemoved = true;
		}

		bool runtimeRemoved = true;
		for (int runtimeIndex = state.m_aMissionRuntimeEntities.Count() - 1; runtimeIndex >= 0; runtimeIndex--)
		{
			HST_MissionRuntimeEntityState runtimeEntity = state.m_aMissionRuntimeEntities[runtimeIndex];
			if (!runtimeEntity || runtimeEntity.m_sRuntimeEntityId != entityId)
				continue;

			state.m_aMissionRuntimeEntities.Remove(runtimeIndex);
			runtimeRemoved = true;
		}

		return assetRemoved && runtimeRemoved;
	}

	protected HST_CampaignDebugCaseResult CreateExpiredPlayerBoundBubbleDebugCase(HST_CampaignState state)
	{
		HST_CampaignDebugCaseResult probe = new HST_CampaignDebugCaseResult();
		probe.m_sCaseId = "render_bubble.mission_asset.expired_player_bound";
		probe.m_sCategory = "physical_war";
		probe.m_sFeature = "render_bubble";
		probe.m_sStage = "early_mechanics";
		probe.m_sStatus = "PASS";
		if (state)
		{
			probe.m_iStartSecond = state.m_iElapsedSeconds;
			probe.m_iEndSecond = state.m_iElapsedSeconds;
		}
		probe.m_aEvidence.Insert("temporary expired rescue asset probes post-expiry player-bound mission bubble policy and is removed before the case is recorded");
		return probe;
	}

	protected HST_CampaignDebugAssertion AddExpiredPlayerBoundBubbleDebugAssertion(HST_CampaignDebugCaseResult probe, string assertionId, string expected, string actual, string status, string failureReason = "", string entityId = "", string missionInstanceId = "")
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
		probe.m_aAssertions.Insert(assertion);
		return assertion;
	}

	protected void AddExpiredPlayerBoundBubbleDebugMetric(HST_CampaignDebugCaseResult probe, string metricId, string value, string unit = "")
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
		probe.m_aMetrics.Insert(metric);
	}

	protected void FinalizeExpiredPlayerBoundBubbleDebugCase(HST_CampaignState state, HST_CampaignDebugCaseResult probe)
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
			resolvedReason = "expired mission asset player-bound bubble probe assertions passed";
		probe.m_sReason = resolvedReason;
		if (state)
			probe.m_iEndSecond = state.m_iElapsedSeconds;
	}

	protected string ExpiredPlayerBoundBubbleDebugStatus(bool passed, string failStatus = "FAIL")
	{
		if (passed)
			return "PASS";

		return failStatus;
	}

	protected string BuildExpiredPlayerBoundBubbleDebugActual(HST_ActiveMissionState mission, HST_MissionAssetState asset, bool continues, bool followAllowed, bool extractAllowed, bool canComplete)
	{
		if (!mission || !asset)
			return "missing";

		string actual = string.Format("status %1 | phase %2 | asset phase %3", mission.m_eStatus, ReportText(mission.m_sRuntimePhase), ReportText(asset.m_sLastInteraction));
		actual = actual + string.Format(" | continue %1 | follow %2 | extract %3 | complete %4", ReportBool(continues), ReportBool(followAllowed), ReportBool(extractAllowed), ReportBool(canComplete));
		actual = actual + string.Format(" | attached %1 | carrier %2 | position %3", ReportBool(asset.m_bAttachedToCarrier), ReportText(asset.m_sCarriedByVehicleId), asset.m_vCurrentPosition);
		return actual;
	}

	protected bool ResolveFirstLivingPlayerDebugContext(out int playerId, out vector playerPosition)
	{
		playerId = 0;
		playerPosition = "0 0 0";
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int candidateId : playerIds)
		{
			IEntity playerEntity = playerManager.GetPlayerControlledEntity(candidateId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			playerId = candidateId;
			playerPosition = playerEntity.GetOrigin();
			return true;
		}

		return false;
	}

	protected vector ResolveExpiredPlayerBoundBubbleDebugFarPosition(vector playerPosition)
	{
		float minimumDistance = POST_EXPIRY_PLAYER_ASSET_BUBBLE_METERS + 400.0;
		for (int attempt = 0; attempt < 8; attempt++)
		{
			vector offset = "0 0 0";
			if (attempt == 0)
				offset[0] = minimumDistance;
			else if (attempt == 1)
				offset[0] = -minimumDistance;
			else if (attempt == 2)
				offset[2] = minimumDistance;
			else if (attempt == 3)
				offset[2] = -minimumDistance;
			else if (attempt == 4)
			{
				offset[0] = minimumDistance * 0.75;
				offset[2] = minimumDistance * 0.75;
			}
			else if (attempt == 5)
			{
				offset[0] = -minimumDistance * 0.75;
				offset[2] = minimumDistance * 0.75;
			}
			else if (attempt == 6)
			{
				offset[0] = minimumDistance * 0.75;
				offset[2] = -minimumDistance * 0.75;
			}
			else
			{
				offset[0] = -minimumDistance * 0.75;
				offset[2] = -minimumDistance * 0.75;
			}

			vector candidate = HST_WorldPositionService.ResolveSafeGroundPosition(playerPosition + offset, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 8.0);
			if (!IsZeroVector(candidate) && !IsAnyLivingPlayerNearObjective(candidate, POST_EXPIRY_PLAYER_ASSET_BUBBLE_METERS))
				return candidate;
		}

		return playerPosition + Vector(minimumDistance, 0.0, 0.0);
	}

	protected bool RemoveExpiredPlayerBoundBubbleDebugRecords(HST_CampaignState state, string instanceId, string assetId, string entityId)
	{
		if (!state)
			return false;

		bool missionRemoved = false;
		for (int missionIndex = state.m_aActiveMissions.Count() - 1; missionIndex >= 0; missionIndex--)
		{
			HST_ActiveMissionState mission = state.m_aActiveMissions[missionIndex];
			if (!mission || mission.m_sInstanceId != instanceId)
				continue;

			state.m_aActiveMissions.Remove(missionIndex);
			missionRemoved = true;
		}

		bool assetRemoved = false;
		for (int assetIndex = state.m_aMissionAssets.Count() - 1; assetIndex >= 0; assetIndex--)
		{
			HST_MissionAssetState asset = state.m_aMissionAssets[assetIndex];
			if (!asset || asset.m_sAssetId != assetId)
				continue;

			state.m_aMissionAssets.Remove(assetIndex);
			assetRemoved = true;
		}

		for (int runtimeIndex = state.m_aMissionRuntimeEntities.Count() - 1; runtimeIndex >= 0; runtimeIndex--)
		{
			HST_MissionRuntimeEntityState runtimeEntity = state.m_aMissionRuntimeEntities[runtimeIndex];
			if (runtimeEntity && runtimeEntity.m_sRuntimeEntityId == entityId)
				state.m_aMissionRuntimeEntities.Remove(runtimeIndex);
		}

		return missionRemoved && assetRemoved;
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

	bool InitializeMissionRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionDefinition definition, HST_ActiveMissionState mission, HST_GeneratedContentService content)
	{
		if (!state || !definition || !mission)
			return false;

		string primitive = PrimitiveForMission(definition);
		mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_STATE_MACHINE;
		mission.m_sRuntimePrimitive = primitive;
		mission.m_sRuntimeType = definition.m_sRuntimeType;
		if (mission.m_sRuntimeType.IsEmpty())
			mission.m_sRuntimeType = primitive;
		mission.m_sRuntimePhase = PHASE_CREATED;
		mission.m_sRuntimeEntityId = string.Format("runtime_%1_%2", mission.m_sInstanceId, primitive);
		mission.m_iRuntimeStartedAtSecond = state.m_iElapsedSeconds;
		mission.m_iRuntimeHoldSeconds = ResolveHoldSeconds(primitive, definition);
		mission.m_bRuntimeFallback = primitive == PRIMITIVE_ABSTRACT_FALLBACK;
		mission.m_bRuntimeCleanupComplete = false;
		if (primitive == PRIMITIVE_CONVOY_INTERCEPT)
		{
			mission.m_sRuntimePhase = PHASE_CONVOY_STAGING;
			mission.m_iRuntimeCounterA = 0;
			mission.m_iRuntimeCounterB = ResolveConvoyIdleDelaySeconds(state, mission);
			mission.m_iRuntimeCounterC = Math.Max(mission.m_iRuntimeCounterB + 300, ResolveConvoyArrivalSeconds(mission));
			mission.m_iRuntimeETASeconds = mission.m_iRuntimeCounterB;
			mission.m_iRequiredCargoCount = 0;
			mission.m_iRequiredCaptiveCount = 0;
		}
		else if (primitive == PRIMITIVE_GUN_SHOP)
		{
			mission.m_sRuntimePhase = PHASE_ACTIVE;
			mission.m_iRequiredCargoCount = 0;
			mission.m_iRequiredCaptiveCount = 0;
			mission.m_iRequiredVehicleCount = 0;
			EnsureGunShopStockGenerated(state, preset, mission);
		}

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			objective.m_sRuntimePrimitive = primitive;
			if (objective.m_sPhysicalEntityId.IsEmpty())
				objective.m_sPhysicalEntityId = string.Format("%1_%2", mission.m_sRuntimeEntityId, objective.m_sTargetId);
			if (objective.m_sLinkedRuntimeEntityId.IsEmpty())
				objective.m_sLinkedRuntimeEntityId = mission.m_sRuntimeEntityId;
			if (objective.m_iRequiredHoldSeconds <= 0)
				objective.m_iRequiredHoldSeconds = ResolveObjectiveHoldSeconds(objective, primitive, definition);
			objective.m_bAbstractFallback = mission.m_bRuntimeFallback;
		}

		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
		{
			// Exact rescue authority creates and materializes its own captive aggregate.
			// Initialization still establishes the shared primitive/objective envelope,
			// but must not manufacture legacy captives, fallback props, or guard groups.
			mission.m_bRuntimeSpawned = false;
			mission.m_bRuntimeFallback = false;
			return true;
		}
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
		{
			// Exact radio lifecycle owns its durable asset and physical transmitter.
			// Keep only the shared primitive/objective envelope initialized here.
			mission.m_bRuntimeSpawned = false;
			mission.m_bRuntimeFallback = false;
			return true;
		}

		EnsureMissionAssetsInitialized(state, definition, mission);
		LinkObjectivesToMissionAssets(state, mission);
		mission.m_bRuntimeSpawned = TrySpawnMissionRuntimeAssets(state, mission);
		if (!mission.m_bRuntimeSpawned)
		{
			bool convoyWithoutVehicleAssets = primitive == PRIMITIVE_CONVOY_INTERCEPT && state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_VEHICLE) <= 0;
			bool suppressFallbackProp = primitive == PRIMITIVE_GUN_SHOP;
			if (!convoyWithoutVehicleAssets && !suppressFallbackProp)
			{
				mission.m_bRuntimeSpawned = TrySpawnMissionRuntimeProp(state, mission);
				mission.m_bRuntimeFallback = true;
			}
			else
			{
				mission.m_bRuntimeFallback = false;
			}
		}
		else
		{
			mission.m_bRuntimeFallback = false;
		}

		EnsureMissionHostileGroup(state, preset, definition, mission);
		return true;
	}

	bool Tick(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionObjectiveService objectives, int elapsedSeconds)
	{
		if (!state || elapsedSeconds <= 0)
			return false;

		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
			changed = TickMissionRuntime(state, preset, objectives, mission, elapsedSeconds) || changed;

		return changed;
	}

	bool TickCampaignDebugMission(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionObjectiveService objectives, HST_ActiveMissionState mission, int elapsedSeconds)
	{
		if (!state || !mission || elapsedSeconds <= 0 || mission.m_sInstanceId.IsEmpty())
			return false;

		HST_ActiveMissionState authoritativeMission = state.FindActiveMission(mission.m_sInstanceId);
		if (authoritativeMission != mission)
			return false;

		return TickMissionRuntime(state, preset, objectives, mission, elapsedSeconds);
	}

	protected bool TickMissionRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionObjectiveService objectives, HST_ActiveMissionState mission, int elapsedSeconds)
	{
		if (!state || !mission || elapsedSeconds <= 0)
			return false;
		if (IsPersistenceSmokeMission(mission))
			return false;
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
		{
			// Exact rescue owns active actuation, extraction grace, settlement, and
			// process cleanup. Generic expired-runtime and cleanup paths stay isolated.
			return false;
		}
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
		{
			// Lifecycle reconciliation, settlement, and cleanup run before generic runtime.
			return false;
		}

		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
		{
			if (ShouldContinueGunShopRuntime(state, mission))
				return TickGunShopDeliveryRuntime(state, preset, mission, elapsedSeconds);

			if (ShouldContinueExpiredPlayerBoundMissionRuntime(state, mission))
				return TickExpiredPlayerBoundMissionRuntime(state, preset, mission, elapsedSeconds);

			return CleanupMissionRuntime(state, mission);
		}
		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT
			&& mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION)
		{
			// Preserve the rejected aggregate byte-for-byte until the normal failed-
			// mission owner commits status and cleanup. Generic convoy repair must
			// not manufacture or reposition evidence after exact authority failed.
			return false;
		}
		if (mission.m_sRuntimePhase == HST_MissionService.EXPIRY_ADMISSION_PENDING_PHASE
			&& mission.m_sMissionId != "dynamic_defend_petros")
		{
			// The mission remains active only so a rejected strategic receipt can
			// retry after restore. Runtime/objective work must not cross that
			// fail-closed boundary and manufacture a competing terminal outcome.
			return false;
		}

		bool changed;
		changed = RepairActiveMissionRuntimeAfterRestore(state, preset, mission) || changed;
		changed = TickGunShopOpenRuntime(state, preset, mission) || changed;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT
			&& mission.m_iOperationContractVersion != HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION
			&& state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_VEHICLE) <= 0)
			changed = EnsureConvoyMissionAssetsInitialized(state, mission, null) || changed;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT
			&& mission.m_iOperationContractVersion != HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION)
		{
			changed = EnsureConvoyMissionSpecificAssets(state, mission) || changed;
			changed = SyncConvoyPayloadAssetPositions(state, mission) || changed;
		}
		if (mission.m_sRuntimePrimitive != PRIMITIVE_GUN_SHOP)
			changed = EnsureMissionRuntimeProp(state, mission) || changed;
		EnsureMissionCaptivesNeutralized(state, mission);
		changed = UpdateFollowingCaptives(state, mission) || changed;
		changed = SyncMissionAssetRuntimePositions(state, mission) || changed;
		changed = TickDefendPetrosRuntime(state, mission, elapsedSeconds) || changed;
		changed = AdvanceMissionStateMachine(state, preset, mission, elapsedSeconds) || changed;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
				continue;

			if (PollObjective(state, preset, mission, objective, elapsedSeconds))
				changed = true;
		}

		return changed;
	}

	bool TickExactRescueCaptiveActuators(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;
		if (!HST_RescuePOWOperationService.IsExactMission(mission))
			return false;

		bool changed;
		// A restored BOARDING captive has no seat authority and must not claim a
		// cargo slot ahead of a BOARDed captive whose exact durable slot is still
		// being recreated. Keep every tokenless actuator behind this barrier until
		// all durable BOARDed claims are physically exact again.
		bool durableBoardedSeatPending
			= HasUnsettledExactRescueBoardedSeatAuthority(state, mission);
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!IsExactRescueCaptiveAsset(mission, asset))
				continue;

			bool wasSpawned = asset.m_bSpawned;
			vector previousPosition = asset.m_vCurrentPosition;
			changed = HST_RescuePOWOperationService.ApplyCaptiveCompatibilityProjection(asset) || changed;

			if (asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED
				|| asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED)
			{
				bool hadProjection = HasExactRescueCaptiveProjection(asset);
				FoldExactRescueCaptiveProjection(state, mission, asset, "terminal exact rescue disposition");
				changed = hadProjection || changed;
				continue;
			}

			if (asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING)
			{
				string escortEvidence;
				RebindExactRescueEscortProjection(state, mission, asset, escortEvidence);
			}
			else if (asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
				|| asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
			{
				EnsureExactRescueCaptiveProjection(state, mission, asset);
				IEntity carrierEntity;
				string seatToken;
				vector carrierPosition;
				string carrierEvidence;
				if (ResolveExactRescueCarrierEvidence(state, mission, asset, carrierEntity, seatToken, carrierPosition, carrierEvidence))
				{
					IEntity captiveEntity = GetExactRescueCaptiveProjection(asset);
					if (captiveEntity)
					{
						IEntity occupiedVehicle = ResolveEntityVehicle(captiveEntity);
						if (!occupiedVehicle)
						{
							float boardingRadius = Math.Max(8.0,
								asset.m_iInteractionRadiusMeters + 3.0);
							if (DistanceSq2D(captiveEntity.GetOrigin(), carrierEntity.GetOrigin())
								<= boardingRadius * boardingRadius
								&& !(durableBoardedSeatPending
									&& asset.m_eRescueDisposition
										== HST_ERescueCaptiveDisposition
											.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING))
							{
								string boardingReason;
								string expectedSeatToken;
								if (asset.m_eRescueDisposition
									== HST_ERescueCaptiveDisposition
										.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
									expectedSeatToken = asset.m_sRescueCarrierSeatToken;
								TryMoveCaptiveIntoVehicle(
									captiveEntity,
									carrierEntity,
									boardingReason,
									expectedSeatToken);
							}
						}
						else if (occupiedVehicle == carrierEntity)
						{
							asset.m_vCurrentPosition = carrierPosition;
							asset.m_vLastKnownPosition = carrierPosition;
							RegisterAssetRuntimeEntityState(state, asset, carrierPosition, carrierEntity.GetYawPitchRoll());
						}
					}
				}
			}
			else
			{
				IEntity projection = GetExactRescueCaptiveProjection(asset);
				if (projection)
					SyncExactRescueCaptiveProjectionState(state, asset, projection);
			}

			if (asset.m_bSpawned != wasSpawned || DistanceSq2D(previousPosition, asset.m_vCurrentPosition) > 0.5)
				changed = true;
		}

		return changed;
	}

	bool EnsureExactRescueCaptiveProjection(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset)
	{
		if (!state || !mission || !IsExactRescueCaptiveAsset(mission, asset))
			return false;
		if (!HST_RescuePOWOperationService.IsExactMission(mission))
			return false;
		if (!IsExactRescueProjectableDisposition(asset.m_eRescueDisposition))
			return false;

		HST_RescuePOWOperationService.ApplyCaptiveCompatibilityProjection(asset);
		if (!ResolveExactRescueProjectionIdentity(asset))
			return false;

		IEntity existing = GetExactRescueCaptiveProjection(asset);
		if (existing)
		{
			ApplyMissionAssetIdentity(existing, asset);
			SyncExactRescueCaptiveProjectionState(state, asset, existing);
			return true;
		}

		if (asset.m_sPrefab.IsEmpty())
			return false;
		vector position = ResolveRestoredMissionAssetPosition(asset);
		if (IsZeroVector(position))
			position = mission.m_vTargetPosition;
		if (IsZeroVector(position))
			return false;

		IEntity projection = SpawnCaptiveFollowerProjection(state, mission, asset, position, "0 0 0");
		if (!projection)
			return false;

		asset.m_iRescueProjectionGeneration = Math.Max(0, asset.m_iRescueProjectionGeneration) + 1;
		SyncExactRescueCaptiveProjectionState(state, asset, projection);
		return true;
	}

	bool MoveExactRescueCaptiveProjectionForDebug(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		vector position)
	{
		if (!state || !mission || !asset || IsZeroVector(position)
			|| mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
			|| !HST_RescuePOWOperationService.IsExactMission(mission)
			|| !IsExactRescueCaptiveAsset(mission, asset)
			|| asset.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED
			|| asset.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED)
			return false;
		if (!EnsureExactRescueCaptiveProjection(state, mission, asset))
			return false;
		IEntity projection = GetExactRescueCaptiveProjection(asset);
		if (!projection)
			return false;
		HST_WorldPositionService.ApplyUprightEntityTransform(
			projection, position, projection.GetYawPitchRoll());
		SyncExactRescueCaptiveProjectionState(state, asset, projection);
		return DistanceSq2D(projection.GetOrigin(), position) <= 1.0;
	}

	bool FoldExactRescueCaptiveProjection(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		string reason = "")
	{
		return FoldExactRescueCaptiveProjectionInternal(
			state,
			mission,
			asset,
			reason,
			false);
	}

	// Native/profile restore carries durable projection flags across the process
	// boundary, but the old process's IEntity handles do not survive with them.
	// Fold only that stale process-local authority before startup recapture while
	// preserving the captive disposition, carrier identity, seat token, and pose
	// required for the normal materializer to reconstruct the exact topology.
	bool FoldRestoredExactRescueCaptiveProjection(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		string reason = "")
	{
		if (!state || !state.m_bRestoredFromPersistence)
			return false;
		return FoldExactRescueCaptiveProjectionInternal(
			state,
			mission,
			asset,
			reason,
			true);
	}

	protected bool FoldExactRescueCaptiveProjectionInternal(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		string reason,
		bool allowActiveActuatedRestoreFold)
	{
		if (!state || !mission || !IsExactRescueCaptiveAsset(mission, asset))
			return false;
		if (!HST_RescuePOWOperationService.IsExactMission(mission))
			return false;
		bool actuatedDisposition = asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
			|| asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
			|| asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
		if (actuatedDisposition && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& state.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE
			&& !allowActiveActuatedRestoreFold)
			return false;

		HST_RescuePOWOperationService.ApplyCaptiveCompatibilityProjection(asset);
		IEntity projection = GetExactRescueCaptiveProjection(asset);
		if (projection)
		{
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(projection.FindComponent(SCR_DamageManagerComponent));
			if (!damageManager || damageManager.GetState() != EDamageState.DESTROYED)
			{
				vector position = projection.GetOrigin();
				if (!IsZeroVector(position))
				{
					asset.m_vCurrentPosition = position;
					asset.m_vLastKnownPosition = position;
				}
			}
			StopCaptiveFollowController(asset);
			DeleteRuntimeEntity(asset.m_sEntityId);
		}

		asset.m_bSpawned = false;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bSpawned = false;
			runtimeEntity.m_bDestroyed = asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED;
			runtimeEntity.m_bRecovered = asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED;
			runtimeEntity.m_vPosition = asset.m_vCurrentPosition;
		}

		return !HasExactRescueCaptiveProjection(asset);
	}

	bool TryResolveExactRescueCaptiveDeathEvidence(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		out vector position,
		out string evidence)
	{
		position = "0 0 0";
		evidence = "exact rescue captive projection is absent; absence is not death evidence";
		if (!state || !mission || !IsExactRescueCaptiveAsset(mission, asset))
			return false;
		if (!HST_RescuePOWOperationService.IsExactMission(mission))
			return false;

		IEntity projection = GetExactRescueCaptiveProjection(asset);
		if (!projection)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(projection.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager)
		{
			evidence = "exact rescue captive projection has no authoritative damage component";
			return false;
		}
		if (damageManager.GetState() != EDamageState.DESTROYED)
		{
			evidence = "exact rescue captive projection is present and not destroyed";
			return false;
		}

		position = projection.GetOrigin();
		evidence = "exact rescue captive projection damage state is destroyed";
		return true;
	}

	bool RebindExactRescueEscortProjection(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		out string evidence)
	{
		evidence = "exact rescue escort identity is unavailable";
		if (!state || !mission || !IsExactRescueCaptiveAsset(mission, asset))
			return false;
		if (!HST_RescuePOWOperationService.IsExactMission(mission)
			|| asset.m_eRescueDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
			|| asset.m_sRescueEscortIdentityId.IsEmpty())
			return false;

		IEntity escortEntity = ResolveExactRescueEscortEntity(state, asset, evidence);
		if (!escortEntity)
			return false;
		if (!EnsureExactRescueCaptiveProjection(state, mission, asset))
		{
			evidence = "exact rescue captive projection could not be restored for its escort";
			return false;
		}

		IEntity projection = GetExactRescueCaptiveProjection(asset);
		if (!projection)
			return false;
		bool actuated = StartCaptiveFollowController(asset, escortEntity);
		if (!actuated)
			actuated = TryIssueCaptiveFollowWaypoint(state, asset, projection, escortEntity.GetOrigin(), escortEntity);
		if (!actuated)
		{
			evidence = "exact rescue captive projection could not accept its escort actuator";
			return false;
		}

		SyncExactRescueCaptiveProjectionState(state, asset, projection);
		evidence = "exact rescue captive projection rebound to durable escort identity";
		return true;
	}

	bool ResolveExactRescueEscortCarrierEvidence(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		out IEntity carrierEntity,
		out string carrierVehicleId,
		out vector carrierPosition,
		out string evidence)
	{
		carrierEntity = null;
		carrierVehicleId = "";
		carrierPosition = "0 0 0";
		evidence = "exact rescue escort has no observed durable vehicle";
		if (!state || !mission || !IsExactRescueCaptiveAsset(mission, asset))
			return false;
		if (!HST_RescuePOWOperationService.IsExactMission(mission)
			|| asset.m_eRescueDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING)
			return false;

		IEntity escortEntity = ResolveExactRescueEscortEntity(state, asset, evidence);
		if (!escortEntity)
			return false;

		IEntity observedVehicle = ResolveEntityVehicle(escortEntity);
		string prefab;
		IEntity vehicleRoot;
		if (observedVehicle)
			vehicleRoot = ResolveMissionVehicleRoot(observedVehicle, prefab);
		else
			vehicleRoot = ResolveMissionVehicleRoot(escortEntity, prefab);
		if (!vehicleRoot)
		{
			evidence = "exact rescue escort is not occupying a vehicle";
			return false;
		}
		if (!IsExactRescueCarrierOperational(vehicleRoot))
		{
			evidence = "exact rescue escort vehicle is destroyed";
			return false;
		}
		if (prefab.IsEmpty())
		{
			evidence = "exact rescue escort vehicle has no restorable prefab identity";
			return false;
		}

		BaseRplComponent replication = BaseRplComponent.Cast(vehicleRoot.FindComponent(BaseRplComponent));
		if (!replication)
		{
			evidence = "exact rescue escort vehicle has no replication-backed durable identity";
			return false;
		}
		string observedVehicleId = ResolveEntityRuntimeId(vehicleRoot);
		if (observedVehicleId.IsEmpty() || observedVehicleId.Contains("vehicle_local_"))
		{
			evidence = "exact rescue escort vehicle resolved only a process-local identity";
			return false;
		}
		IEntity captiveProjection = GetExactRescueCaptiveProjection(asset);
		float boardingRadius = Math.Max(8.0, asset.m_iInteractionRadiusMeters + 3.0);
		if (!captiveProjection
			|| DistanceSq2D(captiveProjection.GetOrigin(), vehicleRoot.GetOrigin())
				> boardingRadius * boardingRadius)
		{
			evidence = "exact rescue escort vehicle is not within the observed captive boarding radius";
			return false;
		}

		string vehicleName = HST_DisplayNameService.ResolveVehicleDisplayName(prefab, vehicleRoot.GetName());
		EnsureMissionCarrierVehicleRecord(state, vehicleRoot, observedVehicleId, vehicleName);
		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(observedVehicleId);
		if (!runtimeVehicle || runtimeVehicle.m_bDeleted || runtimeVehicle.m_sPrefab.IsEmpty()
			|| runtimeVehicle.m_sRuntimeKind != "mission_carrier")
		{
			evidence = "exact rescue escort vehicle could not be registered as durable mission carrier";
			return false;
		}
		carrierEntity = vehicleRoot;
		carrierVehicleId = observedVehicleId;
		carrierPosition = vehicleRoot.GetOrigin();
		evidence = "exact rescue escort occupies a replication-backed durable mission carrier";
		return true;
	}

	bool TryResolveExactRescueCaptiveInteractionEvidence(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		out vector interactionPosition,
		out string evidence)
	{
		interactionPosition = "0 0 0";
		evidence = "exact rescue captive has no live interaction projection";
		if (!state || !mission || !IsExactRescueCaptiveAsset(mission, asset)
			|| !HST_RescuePOWOperationService.IsExactMission(mission)
			|| mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;
		IEntity projection = GetExactRescueCaptiveProjection(asset);
		if (!projection)
			return false;
		SCR_DamageManagerComponent damage = SCR_DamageManagerComponent.Cast(
			projection.FindComponent(SCR_DamageManagerComponent));
		if (damage && damage.GetState() == EDamageState.DESTROYED)
		{
			evidence = "exact rescue captive projection is destroyed and cannot authorize an interaction";
			return false;
		}
		if (asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
		{
			IEntity carrier;
			string seatToken;
			vector carrierPosition;
			string carrierEvidence;
			if (!ResolveExactRescueCarrierEvidence(
				state, mission, asset, carrier, seatToken, carrierPosition, carrierEvidence)
				|| !carrier || seatToken.IsEmpty())
			{
				evidence = "exact rescue boarded interaction lacks live carrier and seat evidence";
				return false;
			}
			interactionPosition = carrierPosition;
			evidence = "exact rescue captive interaction resolved from live carrier and seat evidence";
			return true;
		}
		interactionPosition = projection.GetOrigin();
		evidence = "exact rescue captive interaction resolved from its live projection";
		return true;
	}

	bool IsExactRescueCaptiveProjectionAbsent(HST_MissionAssetState asset)
	{
		return !asset || GetExactRescueCaptiveProjection(asset) == null;
	}

	bool ResolveExactRescueCarrierEvidence(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		out IEntity carrierEntity,
		out string seatToken,
		out vector carrierPosition,
		out string evidence)
	{
		carrierEntity = null;
		seatToken = "";
		carrierPosition = "0 0 0";
		evidence = "exact rescue carrier projection is absent";
		if (!state || !mission || !IsExactRescueCaptiveAsset(mission, asset))
			return false;
		if (!HST_RescuePOWOperationService.IsExactMission(mission) || asset.m_sRescueCarrierVehicleId.IsEmpty())
			return false;
		HST_RescuePOWOperationService.ApplyCaptiveCompatibilityProjection(asset);

		IEntity captiveEntity = GetExactRescueCaptiveProjection(asset);
		IEntity occupiedVehicle;
		SCR_CompartmentAccessComponent access;
		bool usedDurableSeatToken;
		if (captiveEntity)
		{
			access = SCR_CompartmentAccessComponent.Cast(captiveEntity.FindComponent(SCR_CompartmentAccessComponent));
			if (access && access.IsInCompartment())
				occupiedVehicle = access.GetVehicle();
		}

		if (occupiedVehicle && DoesExactRescueCarrierMatch(asset.m_sRescueCarrierVehicleId, occupiedVehicle))
			carrierEntity = occupiedVehicle;
		if (!carrierEntity)
			carrierEntity = ResolveExactRescueCarrierEntity(state, asset);
		if (!carrierEntity && state.m_bRestoredFromPersistence
			&& asset.m_sCarriedByVehicleId == asset.m_sRescueCarrierVehicleId)
		{
			EnsureRestoredMissionCarrierVehicle(state, asset);
			carrierEntity = ResolveExactRescueCarrierEntity(state, asset);
		}
		if (!carrierEntity)
		{
			evidence = "exact rescue carrier projection is absent; absence is not captive death evidence";
			return false;
		}
		if (!IsExactRescueCarrierOperational(carrierEntity))
		{
			evidence = "exact rescue carrier projection is destroyed; this is not captive death evidence";
			HST_RuntimeVehicleState destroyedCarrier = state.FindRuntimeVehicle(asset.m_sRescueCarrierVehicleId);
			if (destroyedCarrier)
			{
				destroyedCarrier.m_vPosition = carrierEntity.GetOrigin();
				destroyedCarrier.m_bDeleted = true;
			}
			carrierEntity = null;
			return false;
		}

		carrierPosition = carrierEntity.GetOrigin();
		UpdateExactRescueCarrierRecord(state, asset.m_sRescueCarrierVehicleId, carrierEntity);
		if (occupiedVehicle && occupiedVehicle != carrierEntity)
		{
			evidence = "exact rescue captive occupies a different carrier than its durable carrier identity";
			carrierEntity = null;
			carrierPosition = "0 0 0";
			return false;
		}

		if (access && access.IsInCompartment() && occupiedVehicle == carrierEntity)
		{
			BaseCompartmentSlot slot = access.GetCompartment();
			// A live occupied slot is the current runtime authority. The durable
			// token is only a restart fallback when no captive projection exists;
			// preferring it here would make a legitimate seat change impossible to
			// reconcile before the strict persistence pass.
			if (slot)
				seatToken = BuildExactRescueCarrierSeatToken(slot);
		}
		else if (!captiveEntity
			&& asset.m_eRescueDisposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED
			&& !asset.m_sRescueCarrierSeatToken.IsEmpty())
		{
			// Durable boarded authority survives process restart. The exact actuator
			// will reproject/reseat the captive; do not replace its saved seat token
			// with a new session-derived observation while no captive handle exists.
			seatToken = asset.m_sRescueCarrierSeatToken;
			usedDurableSeatToken = true;
		}

		if (seatToken.IsEmpty())
			evidence = "exact rescue carrier projection resolved; captive has no observed occupied seat";
		else if (usedDurableSeatToken)
			evidence = "exact rescue carrier restored with preserved durable boarded seat token";
		else
			evidence = "exact rescue carrier and occupied seat resolved from runtime evidence";
		return true;
	}

	// The normal persistence pass is the authoritative final live-pose sampler for
	// an exact rescue captive. Carrier/seat reconciliation runs immediately before
	// this call; this method then resolves that topology read-only and folds one
	// matching pose into every compatible DTO. Controlled shutdown later validates
	// these values read-only.
	bool SampleExactRescueRuntimePoseForPersistence(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		out string evidence)
	{
		evidence = "exact rescue persistence pose sample is unavailable";
		if (!state || !mission || !asset
			|| !HST_RescuePOWOperationService.IsExactMission(mission)
			|| !IsExactRescueCaptiveAsset(mission, asset))
			return false;
		if (asset.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_EXTRACTED
			|| asset.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_KILLED)
		{
			evidence = "exact rescue terminal captive has no live pose authority to sample";
			return true;
		}

		IEntity captiveEntity;
		IEntity carrierEntity;
		BaseCompartmentSlot captiveSlot;
		string observedSeatToken;
		if (!InspectExactRescueRuntimeTopologyReadOnly(
				state,
				mission,
				asset,
				captiveEntity,
				carrierEntity,
				captiveSlot,
				observedSeatToken,
				evidence,
				false))
			return false;
		if (!captiveEntity)
		{
			if (!asset.m_bSpawned)
			{
				evidence
					= "exact rescue captive has no live pose authority; durable folded pose retained";
				return true;
			}
			return false;
		}
		if (captiveEntity.IsDeleted() || !captiveEntity.GetWorld())
			return false;
		SCR_DamageManagerComponent captiveDamage
			= SCR_DamageManagerComponent.Cast(
				captiveEntity.FindComponent(SCR_DamageManagerComponent));
		if (captiveDamage
			&& captiveDamage.GetState() == EDamageState.DESTROYED)
			return false;

		IEntity poseAuthority = captiveEntity;
		HST_RuntimeVehicleState carrierRecord;
		if (carrierEntity)
		{
			if (!IsExactRescueCarrierOperational(carrierEntity)
				|| !TryResolveUniqueMissionCarrierRecordReadOnly(
					state,
					asset.m_sRescueCarrierVehicleId,
					carrierRecord,
					evidence)
					|| !carrierRecord || carrierRecord.m_bDeleted
				|| carrierRecord.m_sPrefab.IsEmpty())
				return false;
			vector carrierPosition = carrierEntity.GetOrigin();
			vector carrierAngles
				= HST_WorldPositionService.BuildUprightAnglesFromVector(
					carrierEntity.GetYawPitchRoll());
			carrierRecord.m_vPosition = carrierPosition;
			carrierRecord.m_vAngles = carrierAngles;
			// A seatless BOARDING captive is assigned to this carrier but remains
			// physically on foot. Only an actually seated BOARDED captive inherits
			// the carrier-root pose in its durable captive projection.
			if (asset.m_eRescueDisposition
					== HST_ERescueCaptiveDisposition
						.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED
				&& captiveSlot)
			{
				poseAuthority = carrierEntity;
			}
		}

		string projectionId = asset.m_sRescueProjectionId;
		if (projectionId.IsEmpty())
			projectionId = asset.m_sEntityId;
		HST_MissionRuntimeEntityState runtimeProjection;
		int runtimeProjectionMatches;
		foreach (HST_MissionRuntimeEntityState candidateProjection : state.m_aMissionRuntimeEntities)
		{
			if (!candidateProjection
				|| candidateProjection.m_sRuntimeEntityId != projectionId)
				continue;
			runtimeProjectionMatches++;
			if (runtimeProjectionMatches > 1)
				return false;
			runtimeProjection = candidateProjection;
		}
		if (runtimeProjectionMatches != 1 || !runtimeProjection
			|| runtimeProjection.m_sRuntimeEntityId != projectionId
			|| runtimeProjection.m_sMissionInstanceId != mission.m_sInstanceId
			|| runtimeProjection.m_sKind != asset.m_sKind
			|| runtimeProjection.m_sPrefab != asset.m_sPrefab)
			return false;
		vector position = poseAuthority.GetOrigin();
		vector angles = HST_WorldPositionService.BuildUprightAnglesFromVector(
			poseAuthority.GetYawPitchRoll());
		asset.m_bSpawned = true;
		asset.m_vCurrentPosition = position;
		asset.m_vLastKnownPosition = position;
		runtimeProjection.m_vPosition = position;
		runtimeProjection.m_vAngles = angles;
		runtimeProjection.m_bSpawned = true;
		runtimeProjection.m_bDestroyed = false;
		runtimeProjection.m_bRecovered = false;
		evidence = "exact rescue live pose folded into captive and runtime projection DTOs";
		return true;
	}

	// Controlled-shutdown rescue persistence must inspect the live captive and
	// carrier topology without invoking the normal repair-oriented resolvers.
	// Those resolvers can register vehicles, update durable transforms, restore
	// projections, or clean duplicate bindings. This path deliberately performs
	// none of those actions. The optional strict pass is used only after the
	// normal persistence preparation has folded live evidence into durable DTOs.
	bool InspectExactRescueRuntimeTopologyReadOnly(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		out IEntity captiveEntity,
		out IEntity carrierEntity,
		out BaseCompartmentSlot captiveSlot,
		out string observedSeatToken,
		out string evidence,
		bool requirePreparedAuthority = false)
	{
		captiveEntity = null;
		carrierEntity = null;
		captiveSlot = null;
		observedSeatToken = "";
		evidence = "exact rescue controlled-shutdown runtime topology is unavailable";
		if (!state || !mission || !asset
			|| !HST_RescuePOWOperationService.IsExactMission(mission)
			|| !IsExactRescueCaptiveAsset(mission, asset))
			return false;

		string projectionId = asset.m_sRescueProjectionId;
		if (projectionId.IsEmpty())
			projectionId = asset.m_sEntityId;
		if (projectionId.IsEmpty()
			|| (!asset.m_sRescueProjectionId.IsEmpty()
			&& !asset.m_sEntityId.IsEmpty()
			&& asset.m_sRescueProjectionId != asset.m_sEntityId))
		{
			evidence = "exact rescue controlled-shutdown captive projection identity conflicts";
			return false;
		}
		if (!TryResolveRuntimeEntityBindingReadOnly(projectionId, captiveEntity, evidence))
			return false;
		if (captiveEntity
			&& (captiveEntity.IsDeleted() || !captiveEntity.GetWorld()
			|| IsControlledShutdownRuntimeProxyReadOnly(captiveEntity)))
		{
			evidence = "exact rescue controlled-shutdown captive projection is deleted, detached, or proxy-local";
			return false;
		}
		if (!captiveEntity && asset.m_bSpawned)
		{
			evidence = "exact rescue controlled-shutdown durable captive claims an absent live projection";
			return false;
		}
		if (requirePreparedAuthority && captiveEntity)
		{
			SCR_DamageManagerComponent captiveDamage = SCR_DamageManagerComponent.Cast(
				captiveEntity.FindComponent(SCR_DamageManagerComponent));
			if (captiveDamage && captiveDamage.GetState() == EDamageState.DESTROYED)
			{
				evidence = "exact rescue controlled-shutdown prepared captive still has unresolved death evidence";
				return false;
			}
		}

		SCR_CompartmentAccessComponent captiveAccess;
		IEntity occupiedVehicle;
		if (captiveEntity)
		{
			captiveAccess = SCR_CompartmentAccessComponent.Cast(
				captiveEntity.FindComponent(SCR_CompartmentAccessComponent));
			if (captiveAccess && (captiveAccess.IsGettingIn()
				|| captiveAccess.IsGettingOut()
				|| captiveAccess.IsSwitchingSeatsAnim()))
			{
				evidence = "exact rescue controlled-shutdown captive compartment transition is still active";
				return false;
			}
			if (captiveAccess && captiveAccess.IsInCompartment())
			{
				captiveSlot = captiveAccess.GetCompartment();
				occupiedVehicle = captiveAccess.GetVehicle();
				if (!captiveSlot || !occupiedVehicle)
				{
					evidence = "exact rescue controlled-shutdown captive compartment topology is incomplete";
					return false;
				}
				observedSeatToken = BuildExactRescueCarrierSeatToken(captiveSlot);
			}
		}

		bool carrierDisposition
			= asset.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
			|| asset.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
		if (!carrierDisposition)
		{
			if (!asset.m_sRescueCarrierVehicleId.IsEmpty() || occupiedVehicle)
			{
				evidence = "exact rescue controlled-shutdown non-carrier captive retains carrier topology";
				return false;
			}
			if (requirePreparedAuthority && asset.m_bSpawned != (captiveEntity != null))
			{
				evidence = "exact rescue controlled-shutdown prepared captive projection flag is not exact";
				return false;
			}
			if (requirePreparedAuthority && captiveEntity
				&& !IsExactRescuePreparedRuntimePoseExact(
					state,
					mission,
					asset,
					captiveEntity,
					null,
					null))
			{
				evidence = "exact rescue controlled-shutdown prepared on-foot captive pose is not exact";
				return false;
			}
			if (!ValidateExactRescueReadOnlyPlayerTopology(
				state, asset, captiveEntity, null, evidence))
				return false;
			evidence = "exact rescue controlled-shutdown on-foot or folded captive topology is read-only exact";
			return true;
		}

		if (asset.m_sRescueCarrierVehicleId.IsEmpty())
		{
			evidence = "exact rescue controlled-shutdown carrier disposition lacks carrier identity";
			return false;
		}
		HST_RuntimeVehicleState carrierRecord;
		if (!TryResolveUniqueMissionCarrierRecordReadOnly(
			state, asset.m_sRescueCarrierVehicleId, carrierRecord, evidence))
			return false;
		if (!captiveEntity)
		{
			// Restore intentionally folds process-local captive projections before
			// startup recapture. The durable carrier row, disposition, optional seat
			// token, and last pose remain authoritative until the normal materializer
			// recreates that captive. A strict controlled-shutdown pass still requires
			// the complete live captive/carrier topology before publishing its fence.
			if (requirePreparedAuthority || asset.m_bSpawned)
			{
				evidence = "exact rescue controlled-shutdown carrier captive projection is absent";
				return false;
			}
			bool boarded = asset.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
			if ((boarded && asset.m_sRescueCarrierSeatToken.IsEmpty())
				|| (!boarded && !asset.m_sRescueCarrierSeatToken.IsEmpty()))
			{
				evidence = "exact rescue folded carrier captive seat authority conflicts";
				return false;
			}
			if (!ValidateExactRescueFoldedProjectionReadOnly(
				state,
				mission,
				asset,
				projectionId,
				evidence))
				return false;
			if (!ValidateExactRescueReadOnlyPlayerTopology(
				state, asset, null, null, evidence))
				return false;
			evidence = "exact rescue folded carrier captive retains durable carrier, seat, and pose authority";
			return true;
		}
		if (!captiveAccess)
		{
			evidence = "exact rescue controlled-shutdown carrier captive lacks compartment access";
			return false;
		}

		IEntity boundCarrier;
		if (!TryResolveRuntimeEntityBindingReadOnly(
			asset.m_sRescueCarrierVehicleId, boundCarrier, evidence))
			return false;
		if (occupiedVehicle)
		{
			// The persisted carrier key contains the replication ID from the
			// process that created it. Native restart gives the restored root a
			// fresh process-local ID, so accept the exact durable rebind before
			// falling back to same-process replication identity. The independent
			// binding/player/world checks below still reject aliases and ambiguity.
			if (!DoesExactRescueCarrierMatchReadOnly(
				asset.m_sRescueCarrierVehicleId,
				occupiedVehicle,
				boundCarrier))
			{
				evidence = "exact rescue controlled-shutdown captive occupies a foreign carrier";
				return false;
			}
			carrierEntity = occupiedVehicle;
		}

		if (boundCarrier)
		{
			if (!IsReplicationBackedRuntimeBindingReadOnly(
					asset.m_sRescueCarrierVehicleId,
					boundCarrier)
				|| (carrierEntity && carrierEntity != boundCarrier))
			{
				evidence = "exact rescue controlled-shutdown carrier binding identity conflicts";
				return false;
			}
			carrierEntity = boundCarrier;
		}

		IEntity playerCarrier;
		if (!TryResolveExactRescuePlayerCarrierReadOnly(
			asset.m_sRescueCarrierVehicleId, playerCarrier, evidence))
			return false;
		if (playerCarrier)
		{
			if (carrierEntity && carrierEntity != playerCarrier)
			{
				evidence = "exact rescue controlled-shutdown player carrier identity is ambiguous";
				return false;
			}
			carrierEntity = playerCarrier;
		}

		IEntity worldCarrier;
		if (!TryFindMissionVehicleByRuntimeIdReadOnly(
			asset.m_sRescueCarrierVehicleId,
			carrierRecord.m_vPosition,
			worldCarrier,
			evidence))
			return false;
		if (worldCarrier)
		{
			if (carrierEntity && carrierEntity != worldCarrier)
			{
				evidence = "exact rescue controlled-shutdown world carrier identity is ambiguous";
				return false;
			}
			carrierEntity = worldCarrier;
		}
		if (!carrierEntity || carrierEntity.IsDeleted() || !carrierEntity.GetWorld()
			|| IsControlledShutdownRuntimeProxyReadOnly(carrierEntity))
		{
			evidence = "exact rescue controlled-shutdown carrier root is absent, detached, or proxy-local";
			return false;
		}
		if (requirePreparedAuthority
			&& (!IsExactRescueCarrierOperational(carrierEntity)
			|| carrierRecord.m_bDeleted || carrierRecord.m_sPrefab.IsEmpty()))
		{
			evidence = "exact rescue controlled-shutdown prepared carrier authority is not operational";
			return false;
		}
		if (requirePreparedAuthority
			&& !IsExactRescuePreparedRuntimePoseExact(
				state,
				mission,
				asset,
				captiveEntity,
				carrierEntity,
				carrierRecord))
		{
			evidence = "exact rescue controlled-shutdown prepared carrier/captive pose is not exact";
			return false;
		}
		if (occupiedVehicle && occupiedVehicle != carrierEntity)
		{
			evidence = "exact rescue controlled-shutdown captive and durable carrier roots differ";
			return false;
		}
		if (!ValidateExactRescueReadOnlyPlayerTopology(
			state, asset, captiveEntity, carrierEntity, evidence))
			return false;

		if (asset.m_eRescueDisposition
			== HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
		{
			BaseCompartmentManagerComponent captiveManager;
			IEntity captiveManagerOwner;
			if (captiveSlot)
			{
				captiveManager = captiveSlot.GetManager();
				if (captiveManager)
					captiveManagerOwner = captiveManager.GetOwner();
			}
			if (!occupiedVehicle || !captiveSlot || observedSeatToken.IsEmpty()
				|| (requirePreparedAuthority
					&& observedSeatToken != asset.m_sRescueCarrierSeatToken))
			{
				evidence = "exact rescue controlled-shutdown boarded captive seat authority is not exact";
				return false;
			}
			if (captiveSlot.GetOccupant() != captiveEntity
				|| !captiveManager || !captiveManagerOwner
				|| !IsEntityWithinControlledShutdownRoot(
					captiveManagerOwner,
					carrierEntity)
				|| !IsEntityWithinControlledShutdownRoot(
					occupiedVehicle,
					carrierEntity))
			{
				evidence = "exact rescue controlled-shutdown boarded captive reverse seat/root topology is not exact";
				return false;
			}
		}
		else if (requirePreparedAuthority && occupiedVehicle)
		{
			evidence = "exact rescue controlled-shutdown prepared boarding captive has unresolved occupied-seat evidence";
			return false;
		}
		if (requirePreparedAuthority && !asset.m_bSpawned)
		{
			evidence = "exact rescue controlled-shutdown prepared carrier captive is not durably spawned";
			return false;
		}

		evidence = "exact rescue controlled-shutdown captive, carrier, and seat topology is read-only exact";
		return true;
	}

	protected bool IsExactRescuePreparedRuntimePoseExact(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		IEntity captiveEntity,
		IEntity carrierEntity,
		HST_RuntimeVehicleState carrierRecord)
	{
		if (!state || !mission || !asset || !captiveEntity)
			return false;
		IEntity poseAuthority = captiveEntity;
		if (carrierEntity
			&& asset.m_eRescueDisposition
				== HST_ERescueCaptiveDisposition
					.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
			poseAuthority = carrierEntity;
		if (!poseAuthority.GetWorld())
			return false;
		vector position = poseAuthority.GetOrigin();
		vector angles = HST_WorldPositionService.BuildUprightAnglesFromVector(
			poseAuthority.GetYawPitchRoll());
		string projectionId = asset.m_sRescueProjectionId;
		if (projectionId.IsEmpty())
			projectionId = asset.m_sEntityId;
		HST_MissionRuntimeEntityState runtimeProjection
			= state.FindMissionRuntimeEntity(projectionId);
		if (!runtimeProjection
			|| runtimeProjection.m_sRuntimeEntityId != projectionId
			|| runtimeProjection.m_sMissionInstanceId != mission.m_sInstanceId
			|| !runtimeProjection.m_bSpawned
			|| runtimeProjection.m_bDestroyed
			|| runtimeProjection.m_bRecovered
			|| !IsExactRescueControlledShutdownPositionExact(
				asset.m_vCurrentPosition,
				position)
			|| !IsExactRescueControlledShutdownPositionExact(
				asset.m_vLastKnownPosition,
				position)
			|| !IsExactRescueControlledShutdownPositionExact(
				runtimeProjection.m_vPosition,
				position)
			|| !AreExactRescueControlledShutdownAnglesExact(
				runtimeProjection.m_vAngles,
				angles))
			return false;
		if (!carrierEntity)
			return carrierRecord == null;
		vector carrierPosition = carrierEntity.GetOrigin();
		vector carrierAngles
			= HST_WorldPositionService.BuildUprightAnglesFromVector(
				carrierEntity.GetYawPitchRoll());
		return carrierRecord
			&& carrierRecord.m_sVehicleRuntimeId
				== asset.m_sRescueCarrierVehicleId
			&& IsExactRescueControlledShutdownPositionExact(
				carrierRecord.m_vPosition,
				carrierPosition)
			&& AreExactRescueControlledShutdownAnglesExact(
				carrierRecord.m_vAngles,
				carrierAngles);
	}

	protected bool IsEntityWithinControlledShutdownRoot(
		IEntity entity,
		IEntity root)
	{
		if (!entity || !root)
			return false;
		IEntity cursor = entity;
		for (int depth; depth < 64 && cursor; depth++)
		{
			if (cursor == root)
				return true;
			IEntity parent = cursor.GetParent();
			if (parent == cursor)
				return false;
			cursor = parent;
		}
		return false;
	}

	protected bool IsExactRescueControlledShutdownPositionExact(
		vector first,
		vector second)
	{
		float deltaX = first[0] - second[0];
		float deltaY = first[1] - second[1];
		float deltaZ = first[2] - second[2];
		return deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ
			<= 0.0001;
	}

	protected bool AreExactRescueControlledShutdownAnglesExact(
		vector first,
		vector second)
	{
		return IsExactRescueControlledShutdownAngleExact(first[0], second[0])
			&& IsExactRescueControlledShutdownAngleExact(first[1], second[1])
			&& IsExactRescueControlledShutdownAngleExact(first[2], second[2]);
	}

	protected bool IsExactRescueControlledShutdownAngleExact(
		float first,
		float second)
	{
		float difference = Math.AbsFloat(first - second);
		while (difference >= 360.0)
			difference -= 360.0;
		if (difference > 180.0)
			difference = 360.0 - difference;
		return difference <= 0.01;
	}

	protected bool TryResolveRuntimeEntityBindingReadOnly(
		string runtimeEntityId,
		out IEntity entity,
		out string evidence)
	{
		entity = null;
		if (runtimeEntityId.IsEmpty())
		{
			evidence = "exact rescue controlled-shutdown runtime binding identity is empty";
			return false;
		}
		if (m_aRuntimeEntityIds.Count() != m_aRuntimeEntities.Count())
		{
			evidence = "exact rescue controlled-shutdown runtime binding arrays are misaligned";
			return false;
		}
		int matches;
		for (int index = 0; index < m_aRuntimeEntityIds.Count(); index++)
		{
			if (m_aRuntimeEntityIds[index] != runtimeEntityId)
				continue;
			matches++;
			IEntity candidate = m_aRuntimeEntities[index];
			if (!candidate || candidate.IsDeleted() || !candidate.GetWorld())
			{
				evidence = "exact rescue controlled-shutdown runtime binding contains a dead handle";
				return false;
			}
			if (entity && entity != candidate)
			{
				evidence = "exact rescue controlled-shutdown runtime identity maps to multiple live handles";
				return false;
			}
			entity = candidate;
		}
		if (matches > 1)
		{
			evidence = "exact rescue controlled-shutdown runtime identity has duplicate bindings";
			return false;
		}
		if (entity)
		{
			for (int aliasIndex; aliasIndex < m_aRuntimeEntities.Count(); aliasIndex++)
			{
				if (m_aRuntimeEntities[aliasIndex] == entity
					&& m_aRuntimeEntityIds[aliasIndex] != runtimeEntityId)
				{
					evidence
						= "exact rescue controlled-shutdown live handle is aliased by another runtime identity";
					return false;
				}
			}
		}
		return true;
	}

	protected bool DoesExactRescueCarrierMatchReadOnly(
		string carrierId,
		IEntity observedCarrier,
		IEntity boundCarrier)
	{
		if (carrierId.IsEmpty() || !observedCarrier)
			return false;
		if (boundCarrier)
			return boundCarrier == observedCarrier;
		return ResolveEntityRuntimeId(observedCarrier) == carrierId;
	}

	// A durable carrier identity survives process restart while the engine RplId
	// is necessarily process-local. The one-to-one runtime binding is therefore
	// the durable identity authority after restore; a fresh valid replication ID
	// independently proves that the rebound live root can participate in native
	// client topology without rewriting the persisted vehicle_<old RplId> key.
	protected bool IsReplicationBackedRuntimeBindingReadOnly(
		string runtimeEntityId,
		IEntity entity)
	{
		if (runtimeEntityId.IsEmpty() || !entity)
			return false;
		IEntity boundEntity;
		string bindingEvidence;
		if (!TryResolveRuntimeEntityBindingReadOnly(
			runtimeEntityId,
			boundEntity,
			bindingEvidence)
			|| boundEntity != entity)
			return false;
		BaseRplComponent replication = BaseRplComponent.Cast(
			entity.FindComponent(BaseRplComponent));
		return replication && replication.Id() != RplId.Invalid();
	}

	protected bool TryResolveUniqueMissionCarrierRecordReadOnly(
		HST_CampaignState state,
		string carrierId,
		out HST_RuntimeVehicleState record,
		out string evidence)
	{
		record = null;
		if (!state || carrierId.IsEmpty())
			return false;
		int matches;
		foreach (HST_RuntimeVehicleState candidate : state.m_aRuntimeVehicles)
		{
			if (!candidate || candidate.m_sVehicleRuntimeId != carrierId)
				continue;
			matches++;
			record = candidate;
		}
		if (matches != 1 || !record || record.m_sRuntimeKind != "mission_carrier"
			|| record.m_bDeleted || record.m_sPrefab.IsEmpty()
			|| IsZeroVector(record.m_vPosition))
		{
			evidence = "exact rescue controlled-shutdown carrier durable row is absent, duplicate, foreign, or not restorable";
			return false;
		}
		return true;
	}

	protected bool ValidateExactRescueFoldedProjectionReadOnly(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		string projectionId,
		out string evidence)
	{
		if (!state || !mission || !asset || projectionId.IsEmpty()
			|| IsZeroVector(asset.m_vCurrentPosition)
			|| IsZeroVector(asset.m_vLastKnownPosition)
			|| !IsExactRescueControlledShutdownPositionExact(
				asset.m_vCurrentPosition,
				asset.m_vLastKnownPosition))
		{
			evidence = "exact rescue folded captive durable pose conflicts";
			return false;
		}

		HST_MissionRuntimeEntityState runtimeProjection;
		int matches;
		foreach (HST_MissionRuntimeEntityState candidate : state.m_aMissionRuntimeEntities)
		{
			if (!candidate || candidate.m_sRuntimeEntityId != projectionId)
				continue;
			runtimeProjection = candidate;
			matches++;
		}
		// Older compatible rows may not carry a runtime-projection DTO. If one is
		// present, it must be the unique folded mirror of the captive durable pose.
		if (matches == 0)
			return true;
		if (matches != 1 || !runtimeProjection
			|| runtimeProjection.m_sMissionInstanceId != mission.m_sInstanceId
			|| runtimeProjection.m_sKind != asset.m_sKind
			|| runtimeProjection.m_sPrefab != asset.m_sPrefab
			|| runtimeProjection.m_bSpawned
			|| runtimeProjection.m_bDestroyed
			|| runtimeProjection.m_bRecovered
			|| !IsExactRescueControlledShutdownPositionExact(
				runtimeProjection.m_vPosition,
				asset.m_vCurrentPosition))
		{
			evidence = "exact rescue folded captive runtime projection conflicts";
			return false;
		}
		return true;
	}

	protected bool TryResolveExactRescuePlayerCarrierReadOnly(
		string carrierId,
		out IEntity carrierEntity,
		out string evidence)
	{
		carrierEntity = null;
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
		{
			evidence = "exact rescue controlled-shutdown player topology manager is unavailable";
			return false;
		}
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			array<IEntity> playerEntities = {};
			IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
			IEntity mainEntity = SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
			if (controlledEntity)
				playerEntities.Insert(controlledEntity);
			if (mainEntity && mainEntity != controlledEntity)
				playerEntities.Insert(mainEntity);
			foreach (IEntity playerEntity : playerEntities)
			{
				if (!playerEntity || playerEntity.IsDeleted() || !playerEntity.GetWorld())
					continue;
				IEntity vehicle = CompartmentAccessComponent.GetVehicleIn(playerEntity);
				if (!vehicle || ResolveEntityRuntimeId(vehicle) != carrierId)
					continue;
				if (carrierEntity && carrierEntity != vehicle)
				{
					evidence = "exact rescue controlled-shutdown carrier identity resolves through multiple player vehicles";
					return false;
				}
				carrierEntity = vehicle;
			}
		}
		return true;
	}

	protected bool TryFindMissionVehicleByRuntimeIdReadOnly(
		string vehicleId,
		vector nearPosition,
		out IEntity vehicleEntity,
		out string evidence)
	{
		vehicleEntity = null;
		if (vehicleId.IsEmpty() || IsZeroVector(nearPosition))
			return true;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			evidence = "exact rescue controlled-shutdown world is unavailable for carrier inspection";
			return false;
		}
		m_aMissionVehicleScanEntities.Clear();
		world.QueryEntitiesBySphere(
			nearPosition,
			30.0,
			AddMissionVehicleScanCandidate,
			null,
			EQueryEntitiesFlags.ALL);
		array<IEntity> matches = {};
		foreach (IEntity candidate : m_aMissionVehicleScanEntities)
		{
			if (!candidate || candidate.IsDeleted() || !candidate.GetWorld())
				continue;
			string prefab;
			IEntity root = ResolveMissionVehicleRoot(candidate, prefab);
			if (!root || root.IsDeleted() || !root.GetWorld()
				|| ResolveEntityRuntimeId(root) != vehicleId || matches.Contains(root))
				continue;
			matches.Insert(root);
		}
		if (matches.Count() > 1)
		{
			evidence = "exact rescue controlled-shutdown carrier world identity is ambiguous";
			return false;
		}
		if (matches.Count() == 1)
			vehicleEntity = matches[0];
		return true;
	}

	protected bool ValidateExactRescueReadOnlyPlayerTopology(
		HST_CampaignState state,
		HST_MissionAssetState asset,
		IEntity captiveEntity,
		IEntity carrierEntity,
		out string evidence)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
		{
			evidence = "exact rescue controlled-shutdown player topology manager is unavailable";
			return false;
		}
		array<int> playerIds = {};
		array<IEntity> inspectedEntities = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			array<IEntity> playerEntities = {};
			IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
			IEntity mainEntity = SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
			if (controlledEntity)
				playerEntities.Insert(controlledEntity);
			if (mainEntity && mainEntity != controlledEntity)
				playerEntities.Insert(mainEntity);
			foreach (IEntity playerEntity : playerEntities)
			{
				if (!playerEntity || playerEntity.IsDeleted() || !playerEntity.GetWorld()
					|| inspectedEntities.Contains(playerEntity))
					continue;
				inspectedEntities.Insert(playerEntity);
				if (playerEntity == captiveEntity || playerEntity == carrierEntity)
				{
					evidence = "exact rescue controlled-shutdown captive or carrier root is a player-controlled entity";
					return false;
				}
				SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(
					playerEntity.FindComponent(SCR_CompartmentAccessComponent));
				if (!access || (!access.IsGettingIn() && !access.IsGettingOut()
					&& !access.IsSwitchingSeatsAnim()))
					continue;
				// During the first/last transition frames both vehicle accessors can be
				// null. Reject every player compartment transition so a rescue carrier
				// cannot be sampled while its occupancy target is unresolved.
				evidence
					= HST_PhysicalWarService.CONTROLLED_SHUTDOWN_PLAYER_RELEASE_EVIDENCE
						+ ": exact rescue player compartment transition is active";
				return false;
			}
		}
		return true;
	}

	protected bool IsControlledShutdownRuntimeProxyReadOnly(IEntity entity)
	{
		if (!entity)
			return false;
		BaseRplComponent replication = BaseRplComponent.Cast(
			entity.FindComponent(BaseRplComponent));
		return replication && replication.IsProxy();
	}

	protected bool IsExactRescueCaptiveAsset(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
			return false;
		if (!HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return false;
		if (asset.m_sKind != ASSET_KIND_CAPTIVE && asset.m_sRole != ROLE_CAPTIVE)
			return false;
		if (HST_RescuePOWOperationService.IsExactMission(mission))
			return asset.m_iRescueContractVersion == HST_RescuePOWOperationService.EXACT_CONTRACT_VERSION;

		return asset.m_iRescueContractVersion != 0
			|| asset.m_eRescueDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_UNKNOWN;
	}

	protected bool IsExactRescueProjectableDisposition(HST_ERescueCaptiveDisposition disposition)
	{
		return disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_HELD
			|| disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FREED
			|| disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_FOLLOWING
			|| disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDING
			|| disposition == HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED;
	}

	protected IEntity ResolveExactRescueEscortEntity(
		HST_CampaignState state,
		HST_MissionAssetState asset,
		out string evidence)
	{
		evidence = "exact rescue escort identity is unavailable";
		if (!state || !asset || asset.m_sRescueEscortIdentityId.IsEmpty())
			return null;

		HST_PlayerState escort = state.FindPlayer(asset.m_sRescueEscortIdentityId);
		if (!escort || escort.m_iLastSeenPlayerId <= 0)
			return null;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return null;
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		if (playerIds.Find(escort.m_iLastSeenPlayerId) < 0)
		{
			evidence = "exact rescue escort is disconnected; durable identity retained";
			return null;
		}

		IEntity escortEntity = GetBestPlayerEntity(playerManager, escort.m_iLastSeenPlayerId);
		if (!IsLivingPlayerEntity(escortEntity))
		{
			evidence = "exact rescue escort has no living projection; durable identity retained";
			return null;
		}

		evidence = "exact rescue escort resolved from durable identity";
		return escortEntity;
	}

	protected bool ResolveExactRescueProjectionIdentity(HST_MissionAssetState asset)
	{
		if (!asset)
			return false;
		if (asset.m_sRescueProjectionId.IsEmpty() && asset.m_sEntityId.IsEmpty())
			return false;
		if (asset.m_sRescueProjectionId.IsEmpty())
			asset.m_sRescueProjectionId = asset.m_sEntityId;
		else if (asset.m_sEntityId.IsEmpty())
			asset.m_sEntityId = asset.m_sRescueProjectionId;

		return asset.m_sEntityId == asset.m_sRescueProjectionId;
	}

	protected IEntity GetExactRescueCaptiveProjection(HST_MissionAssetState asset)
	{
		if (!asset)
			return null;
		if (!asset.m_sRescueProjectionId.IsEmpty())
			return GetRuntimeEntity(asset.m_sRescueProjectionId);
		return GetRuntimeEntity(asset.m_sEntityId);
	}

	protected bool HasExactRescueCaptiveProjection(HST_MissionAssetState asset)
	{
		return GetExactRescueCaptiveProjection(asset) != null;
	}

	protected void SyncExactRescueCaptiveProjectionState(
		HST_CampaignState state,
		HST_MissionAssetState asset,
		IEntity projection)
	{
		if (!state || !asset || !projection)
			return;

		ApplyMissionAssetIdentity(projection, asset);
		vector position = projection.GetOrigin();
		asset.m_bSpawned = true;
		if (!IsZeroVector(position))
		{
			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
		}
		RegisterAssetRuntimeEntityState(state, asset, asset.m_vCurrentPosition, projection.GetYawPitchRoll());
	}

	protected IEntity ResolveExactRescueCarrierEntity(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!state || !asset || asset.m_sRescueCarrierVehicleId.IsEmpty())
			return null;

		IEntity carrier = GetRuntimeEntity(asset.m_sRescueCarrierVehicleId);
		if (carrier)
			return carrier;

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sRescueCarrierVehicleId);
		if (runtimeVehicle && !runtimeVehicle.m_bDeleted)
		{
			carrier = FindMissionVehicleByRuntimeId(asset.m_sRescueCarrierVehicleId, runtimeVehicle.m_vPosition);
			if (carrier)
				return carrier;
		}

		string escortEvidence;
		IEntity escortEntity = ResolveExactRescueEscortEntity(state, asset, escortEvidence);
		if (escortEntity)
		{
			IEntity observedVehicle = ResolveEntityVehicle(escortEntity);
			string observedPrefab;
			IEntity observedRoot;
			if (observedVehicle)
				observedRoot = ResolveMissionVehicleRoot(observedVehicle, observedPrefab);
			else
				observedRoot = ResolveMissionVehicleRoot(escortEntity, observedPrefab);
			if (observedRoot && DoesExactRescueCarrierMatch(asset.m_sRescueCarrierVehicleId, observedRoot))
			{
				UpdateExactRescueCarrierRecord(state, asset.m_sRescueCarrierVehicleId, observedRoot);
				return observedRoot;
			}
		}

		vector nearPosition = asset.m_vCurrentPosition;
		if (IsZeroVector(nearPosition))
			nearPosition = asset.m_vLastKnownPosition;
		return FindMissionVehicleByRuntimeId(asset.m_sRescueCarrierVehicleId, nearPosition);
	}

	protected bool DoesExactRescueCarrierMatch(string carrierId, IEntity carrierEntity)
	{
		if (carrierId.IsEmpty() || !carrierEntity)
			return false;
		if (GetRuntimeEntity(carrierId) == carrierEntity)
			return true;
		return ResolveEntityRuntimeId(carrierEntity) == carrierId;
	}

	protected bool IsExactRescueCarrierOperational(IEntity carrierEntity)
	{
		if (!carrierEntity)
			return false;
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(
			carrierEntity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected void UpdateExactRescueCarrierRecord(
		HST_CampaignState state,
		string carrierId,
		IEntity carrierEntity)
	{
		if (!state || carrierId.IsEmpty() || !carrierEntity)
			return;
		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(carrierId);
		if (!runtimeVehicle)
			return;
		runtimeVehicle.m_vPosition = carrierEntity.GetOrigin();
		runtimeVehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(
			carrierEntity.GetYawPitchRoll());
		runtimeVehicle.m_bDeleted = false;
	}

	protected bool HasUnsettledExactRescueBoardedSeatAuthority(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!IsExactRescueCaptiveAsset(mission, asset)
				|| asset.m_eRescueDisposition
					!= HST_ERescueCaptiveDisposition
						.HST_RESCUE_CAPTIVE_DISPOSITION_BOARDED)
				continue;
			if (asset.m_sRescueCarrierVehicleId.IsEmpty()
				|| asset.m_sRescueCarrierSeatToken.IsEmpty())
				return true;

			IEntity captiveEntity = GetExactRescueCaptiveProjection(asset);
			SCR_CompartmentAccessComponent access;
			if (captiveEntity)
				access = SCR_CompartmentAccessComponent.Cast(
					captiveEntity.FindComponent(SCR_CompartmentAccessComponent));
			if (!access || access.IsGettingIn() || access.IsGettingOut()
				|| access.IsSwitchingSeatsAnim() || !access.IsInCompartment())
				return true;

			IEntity occupiedVehicle = access.GetVehicle();
			BaseCompartmentSlot occupiedSlot = access.GetCompartment();
			if (!occupiedVehicle || !occupiedSlot
				|| occupiedSlot.GetOccupant() != captiveEntity
				|| !DoesExactRescueCarrierMatch(
					asset.m_sRescueCarrierVehicleId,
					occupiedVehicle))
				return true;

			array<BaseCompartmentSlot> cargoSlots = {};
			if (!CollectExactRescueCarrierCargoSlotsRecursive(
				occupiedVehicle,
				cargoSlots,
				0))
				return true;
			int matchCount;
			string matchMode;
			BaseCompartmentSlot expectedSlot
				= ResolveExpectedExactRescueCarrierSeat(
					cargoSlots,
					asset.m_sRescueCarrierSeatToken,
					matchCount,
					matchMode);
			if (matchCount != 1 || expectedSlot != occupiedSlot)
				return true;
		}
		return false;
	}

	protected string BuildExactRescueCarrierSeatToken(BaseCompartmentSlot slot)
	{
		if (!slot)
			return "";
		string uniqueName = slot.GetCompartmentUniqueName();
		if (!uniqueName.IsEmpty())
			return EXACT_RESCUE_SEAT_UNIQUE_PREFIX + uniqueName;
		string objectName = slot.GetCompartmentName(false);
		if (objectName.IsEmpty())
			return "";
		return EXACT_RESCUE_SEAT_OBJECT_PREFIX + objectName;
	}

	protected bool CollectExactRescueCarrierCargoSlotsRecursive(
		IEntity entity,
		notnull array<BaseCompartmentSlot> cargoSlots,
		int depth)
	{
		if (!entity || depth > EXACT_RESCUE_COMPARTMENT_DEPTH_LIMIT)
			return false;

		array<Managed> managedComponents = {};
		int managerCount
			= entity.FindComponents(BaseCompartmentManagerComponent, managedComponents);
		if (managerCount != managedComponents.Count())
			return false;
		foreach (Managed managedComponent : managedComponents)
		{
			BaseCompartmentManagerComponent manager
				= BaseCompartmentManagerComponent.Cast(managedComponent);
			if (!manager || manager.GetOwner() != entity)
				return false;
			array<BaseCompartmentSlot> managedSlots = {};
			int managedSlotCount = manager.GetCompartments(managedSlots);
			if (managedSlotCount != managedSlots.Count())
				return false;
			foreach (BaseCompartmentSlot slot : managedSlots)
			{
				if (!slot || slot.GetManager() != manager)
					return false;
				if (slot.GetType() != ECompartmentType.CARGO)
					continue;
				if (cargoSlots.Find(slot) >= 0)
					return false;
				cargoSlots.Insert(slot);
			}
		}

		IEntity child = entity.GetChildren();
		while (child)
		{
			if (!CollectExactRescueCarrierCargoSlotsRecursive(
				child,
				cargoSlots,
				depth + 1))
				return false;
			child = child.GetSibling();
		}
		return true;
	}

	protected bool TryParseLegacyExactRescueCarrierSeatToken(
		string seatToken,
		out int managerId,
		out int slotId)
	{
		managerId = -1;
		slotId = -1;
		if (seatToken.IsEmpty()
			|| seatToken.IndexOf(EXACT_RESCUE_SEAT_TOKEN_PREFIX) == 0)
			return false;
		array<string> fields = {};
		seatToken.Split("_", fields, false);
		if (fields.Count() != 3 || fields[0] != "seat")
			return false;
		managerId = fields[1].ToInt();
		slotId = fields[2].ToInt();
		return seatToken == string.Format("seat_%1_%2", managerId, slotId);
	}

	protected BaseCompartmentSlot ResolveExpectedExactRescueCarrierSeat(
		array<BaseCompartmentSlot> cargoSlots,
		string expectedSeatToken,
		out int matchCount,
		out string matchMode)
	{
		matchCount = 0;
		matchMode = "malformed";
		if (!cargoSlots || expectedSeatToken.IsEmpty())
			return null;

		BaseCompartmentSlot matchedSlot;
		if (expectedSeatToken.IndexOf(EXACT_RESCUE_SEAT_TOKEN_PREFIX) == 0)
		{
			matchMode = "authored_name";
			foreach (BaseCompartmentSlot candidate : cargoSlots)
			{
				if (!candidate
					|| BuildExactRescueCarrierSeatToken(candidate)
						!= expectedSeatToken)
					continue;
				matchCount++;
				matchedSlot = candidate;
			}
			if (matchCount == 1)
				return matchedSlot;
			return null;
		}

		int legacyManagerId;
		int legacySlotId;
		if (!TryParseLegacyExactRescueCarrierSeatToken(
			expectedSeatToken,
			legacyManagerId,
			legacySlotId))
			return null;
		// Numeric manager IDs are live locators, not durable prefab identity. A
		// legacy token is recoverable only when its local slot ID identifies one
		// cargo slot across the entire restored carrier hierarchy.
		matchMode = string.Format(
			"legacy_unique_slot_id_from_manager_%1",
			legacyManagerId);
		foreach (BaseCompartmentSlot candidate : cargoSlots)
		{
			if (!candidate || candidate.GetCompartmentSlotID() != legacySlotId)
				continue;
			matchCount++;
			matchedSlot = candidate;
		}
		if (matchCount == 1)
			return matchedSlot;
		return null;
	}

	protected string BuildExactRescueCarrierSeatSelectionEvidence(
		array<BaseCompartmentSlot> cargoSlots,
		IEntity captiveEntity,
		string expectedSeatToken,
		int matchCount,
		string matchMode)
	{
		string candidates;
		int cargoCount;
		int emitted;
		if (cargoSlots)
		{
			foreach (BaseCompartmentSlot candidate : cargoSlots)
			{
				if (!candidate)
					continue;
				cargoCount++;
				if (emitted >= EXACT_RESCUE_SEAT_DIAGNOSTIC_LIMIT)
					continue;
				if (!candidates.IsEmpty())
					candidates += ";";
				candidates += string.Format(
					"%1[a=%2,o=%3,r=%4,l=%5]",
					BuildExactRescueCarrierSeatToken(candidate),
					candidate.IsCompartmentAccessible(),
					candidate.IsOccupied(),
					candidate.IsReserved()
						&& !candidate.IsReservedBy(captiveEntity),
					candidate.IsGetInLockedFor(captiveEntity));
				emitted++;
			}
		}
		if (cargoCount > emitted)
			candidates += string.Format(";+%1_more", cargoCount - emitted);
		return string.Format(
			"expected=%1 mode=%2 matches=%3 cargo=%4 candidates=%5",
			expectedSeatToken,
			matchMode,
			matchCount,
			cargoCount,
			candidates);
	}

	bool TickExactMissionConvoyCargoProjections(HST_CampaignState state, HST_PhysicalWarService physicalWar)
	{
		if (!state || !physicalWar)
			return false;

		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!HST_MissionConvoyOperationService.IsExactMission(mission))
				continue;

			HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
			bool operationOpen = operation
				&& operation.m_sMissionInstanceId == mission.m_sInstanceId
				&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
				&& operation.m_iContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION
				&& operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
			bool operationProjectionActive = operationOpen
				&& (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING
					|| operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL);
			bool operationProjectionPublished = operationOpen
				&& operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				&& !physicalWar.IsExactMissionConvoyOutboundProjectionTransactionOpen(mission);

			foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
			{
				if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || !IsConvoyPayloadOrCaptiveAsset(asset))
					continue;
				if (IsExactMissionConvoyCargoProjectionPlayerBoundOrResolved(asset))
					continue;

				HST_MissionAssetState carrierAsset;
				HST_ConvoyElementState carrierElement;
				bool carrierAuthorityValid = ResolveExactMissionConvoyCargoCarrierAuthority(state, mission, asset, carrierAsset, carrierElement);
				IEntity carrierEntity;
				if (operationProjectionActive && carrierAuthorityValid)
					carrierEntity = physicalWar.GetExactMissionConvoyVehicleRuntimeEntity(state, mission, asset.m_sAssignedVehicleSlotId);

				if (carrierEntity)
				{
					changed = ProjectExactMissionConvoyCargoAtCarrier(state, mission, asset, carrierAsset, carrierEntity) || changed;
					changed = SetExactMissionConvoyCargoProjectionPublished(asset, operationProjectionPublished) || changed;
					continue;
				}

				vector standalonePosition = ResolveExactMissionConvoyStandaloneRecoveryPosition(asset, carrierAsset, carrierElement);
				bool standaloneRecovery = carrierAuthorityValid
					&& operationProjectionActive
					&& IsExactMissionConvoyTerminalCarrierGroundRecovery(mission, asset, carrierAsset, carrierElement)
					&& !IsZeroVector(standalonePosition);
				if (standaloneRecovery)
				{
					changed = ProjectExactMissionConvoyCargoAtDurablePosition(state, mission, asset, carrierAsset, carrierElement) || changed;
					changed = SetExactMissionConvoyCargoProjectionPublished(asset, operationProjectionPublished) || changed;
					continue;
				}

				string assignedCarrierAssetId;
				if (carrierAsset)
					assignedCarrierAssetId = carrierAsset.m_sAssetId;
				changed = DematerializeExactMissionConvoyCargoProjection(state, asset, assignedCarrierAssetId) || changed;
			}
		}

		return changed;
	}

	bool IsExactMissionConvoyCargoProjectionReady(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_PhysicalWarService physicalWar)
	{
		if (!state || !mission || !physicalWar || !HST_MissionConvoyOperationService.IsExactMission(mission))
			return false;
		HST_MissionAssetState cargo = ResolveExactMissionConvoyCargoAsset(state, mission);
		if (!cargo || cargo.m_bPickedUp || cargo.m_bDelivered || cargo.m_bDestroyed || cargo.m_bOutcomeApplied)
			return true;
		if (cargo.m_sEntityId.IsEmpty() || cargo.m_sPrefab.IsEmpty() || cargo.m_sAssignedVehicleSlotId.IsEmpty()
			|| !cargo.m_bSpawned)
			return false;

		HST_MissionAssetState carrierAsset;
		HST_ConvoyElementState carrierElement;
		if (!ResolveExactMissionConvoyCargoCarrierAuthority(state, mission, cargo, carrierAsset, carrierElement)
			|| !carrierAsset)
			return false;
		IEntity cargoEntity = GetRuntimeEntity(cargo.m_sEntityId);
		if (!cargoEntity || !cargoEntity.GetPrefabData()
			|| cargoEntity.GetPrefabData().GetPrefabName() != cargo.m_sPrefab)
			return false;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(cargo.m_sEntityId);
		if (!runtimeEntity || !runtimeEntity.m_bSpawned || runtimeEntity.m_sMissionInstanceId != mission.m_sInstanceId
			|| runtimeEntity.m_sPrefab != cargo.m_sPrefab || runtimeEntity.m_sKind != cargo.m_sRole)
			return false;
		if (IsExactMissionConvoyTerminalCarrierGroundRecovery(mission, cargo, carrierAsset, carrierElement))
			return !cargo.m_bAttachedToCarrier && cargo.m_sCarriedByVehicleId.IsEmpty()
				&& !ResolveEntityVehicle(cargoEntity) && cargoEntity.GetParent() == null;
		if (!cargo.m_bAttachedToCarrier || cargo.m_sCarriedByVehicleId != carrierAsset.m_sAssetId)
			return false;
		IEntity carrierEntity = physicalWar.GetExactMissionConvoyVehicleRuntimeEntity(
			state,
			mission,
			cargo.m_sAssignedVehicleSlotId);
		if (!carrierEntity)
			return false;
		if (cargo.m_sRole == ROLE_CONVOY_CAPTIVE)
			return ResolveEntityVehicle(cargoEntity) == carrierEntity;
		return cargoEntity.GetParent() == carrierEntity;
	}

	bool SetExactMissionConvoyCargoProjectionPublication(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_PhysicalWarService physicalWar,
		bool published)
	{
		if (!state || !mission || !physicalWar || !HST_MissionConvoyOperationService.IsExactMission(mission))
			return false;
		HST_MissionAssetState cargo;
		int cargoClaimants;
		foreach (HST_MissionAssetState candidate : state.m_aMissionAssets)
		{
			if (!candidate || candidate.m_sMissionInstanceId != mission.m_sInstanceId
				|| !IsConvoyPayloadOrCaptiveAsset(candidate))
				continue;
			cargo = candidate;
			cargoClaimants++;
		}
		if (cargoClaimants == 0)
			return true;
		if (cargoClaimants != 1 || !cargo)
			return false;
		if (IsExactMissionConvoyCargoProjectionPlayerBoundOrResolved(cargo))
			return true;

		IEntity cargoEntity = GetRuntimeEntity(cargo.m_sEntityId);
		if (!published)
		{
			if (!cargoEntity || cargoEntity.IsDeleted())
				return true;
			SetExactMissionConvoyCargoProjectionPublished(cargo, false);
			return HasExactMissionConvoyCargoPublicationState(cargoEntity, false);
		}
		if (!IsExactMissionConvoyCargoProjectionReady(state, mission, physicalWar)
			|| !cargoEntity || cargoEntity.IsDeleted())
			return false;
		SetExactMissionConvoyCargoProjectionPublished(cargo, true);
		return HasExactMissionConvoyCargoPublicationState(cargoEntity, true);
	}

	protected bool HasExactMissionConvoyCargoPublicationState(IEntity entity, bool published)
	{
		if (!entity || entity.IsDeleted())
			return false;
		EntityFlags publicationFlags = EntityFlags.ACTIVE | EntityFlags.VISIBLE | EntityFlags.TRACEABLE;
		bool hasAllPublicationFlags = (entity.GetFlags() & publicationFlags) == publicationFlags;
		return hasAllPublicationFlags == published;
	}

	protected HST_MissionAssetState ResolveExactMissionConvoyCargoAsset(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return null;
		HST_MissionAssetState result;
		int claimants;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId
				|| (asset.m_sRole != ROLE_CONVOY_PAYLOAD && asset.m_sRole != ROLE_CONVOY_CAPTIVE))
				continue;
			result = asset;
			claimants++;
		}
		if (claimants != 1)
			return null;
		return result;
	}

	protected bool SetExactMissionConvoyCargoProjectionPublished(HST_MissionAssetState asset, bool published)
	{
		if (!asset || asset.m_sEntityId.IsEmpty())
			return false;
		IEntity entity = GetRuntimeEntity(asset.m_sEntityId);
		if (!entity)
			return false;
		bool active = (entity.GetFlags() & EntityFlags.ACTIVE) != 0;
		bool visible = (entity.GetFlags() & EntityFlags.VISIBLE) != 0;
		bool traceable = (entity.GetFlags() & EntityFlags.TRACEABLE) != 0;
		if (published && active && visible && traceable)
			return false;
		if (!published && !active && !visible && !traceable)
			return false;
		if (published)
			entity.SetFlags(EntityFlags.ACTIVE | EntityFlags.VISIBLE | EntityFlags.TRACEABLE, true);
		else
			entity.ClearFlags(EntityFlags.ACTIVE | EntityFlags.VISIBLE | EntityFlags.TRACEABLE, true);
		return true;
	}

	protected bool TickDefendPetrosRuntime(HST_CampaignState state, HST_ActiveMissionState mission, int elapsedSeconds)
	{
		if (!state || !mission || mission.m_sMissionId != "dynamic_defend_petros" || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;

		if (!state.m_bPetrosAlive)
		{
			MarkRuntimeMissionFailed(state, mission, "Petros was killed during the defense.");
			return true;
		}

		bool changed;
		if (mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == PHASE_CREATED)
		{
			mission.m_sRuntimePhase = PHASE_ACTIVE;
			changed = true;
		}

		int remaining = mission.m_iRemainingSeconds;
		if (state.m_iDefendPetrosEndsSecond > 0)
			remaining = Math.Max(0, state.m_iDefendPetrosEndsSecond - state.m_iElapsedSeconds);
		mission.m_iRuntimeETASeconds = remaining;
		mission.m_iRuntimeCounterA = state.m_iDefendPetrosAliveAttackerCount;
		mission.m_iRuntimeCounterB = state.m_iDefendPetrosKilledCount;
		mission.m_iRuntimeCounterC = state.m_iDefendPetrosAttackerCount;

		if (state.m_iDefendPetrosAliveAttackerCount > 0 && mission.m_sRuntimePhase != PHASE_CONTACT)
		{
			mission.m_sRuntimePhase = PHASE_CONTACT;
			changed = true;
		}
		else if (state.m_iDefendPetrosAliveAttackerCount <= 0 && mission.m_sRuntimePhase == PHASE_CONTACT)
		{
			mission.m_sRuntimePhase = PHASE_HOLDING;
			changed = true;
		}

		if (state.m_iDefendPetrosEndsSecond > 0 && remaining <= 0)
		{
			foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
			{
				if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bFailed)
					continue;

				changed = CompleteWorldObjective(objective) || changed;
			}

			if (mission.m_sRuntimePhase != "defense_secured")
			{
				mission.m_sRuntimePhase = "defense_secured";
				changed = true;
			}
		}

		return changed;
	}

	protected bool TickExpiredPlayerBoundMissionRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, int elapsedSeconds)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_EXPIRED)
			return false;
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;

		bool changed;
		if (ShouldRunRestoredMissionCarrierRestorePass(state, mission))
			changed = EnsureRestoredMissionCarrierVehicles(state, mission) || changed;
		EnsureMissionCaptivesNeutralized(state, mission);
		changed = UpdateFollowingCaptives(state, mission) || changed;
		changed = SyncMissionAssetRuntimePositions(state, mission) || changed;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
				continue;

			if (PollObjective(state, preset, mission, objective, elapsedSeconds))
				changed = true;
		}

		UpdateMissionCountersFromAssets(state, mission);
		return changed;
	}

	bool CanCompleteExpiredPlayerBoundMission(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_EXPIRED)
			return false;
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;

		if (ShouldContinueExpiredPlayerBoundMissionRuntime(state, mission))
			return true;
		if (IsGunShopDeliveryComplete(state, mission))
			return true;

		return AreRuntimeObjectivesComplete(state, mission.m_sInstanceId) && HasDeliveredPlayerBoundMissionAssetAfterExpiry(state, mission);
	}

	protected bool ShouldContinueExpiredPlayerBoundMissionRuntime(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_EXPIRED || mission.m_bRuntimeCleanupComplete)
			return false;
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (IsMissionAssetPlayerBoundAfterExpiry(state, asset))
				return true;
		}

		return false;
	}

	protected bool IsExpiredPlayerBoundMissionInteractionAllowed(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, string commandId)
	{
		if (!state || !mission || !asset || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_EXPIRED)
			return false;
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return false;
		if (asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bDelivered || asset.m_bDestroyed)
			return false;
		if (!IsMissionAssetPlayerBoundAfterExpiry(state, asset))
			return false;

		if (commandId == "mission_asset_deliver" || commandId == "mission_asset_unload")
			return IsTransportableAsset(asset) && asset.m_sKind != ASSET_KIND_CAPTIVE && asset.m_bPickedUp;
		if (commandId == "mission_captive_extract" || commandId == "mission_captive_follow")
			return asset.m_sKind == ASSET_KIND_CAPTIVE && asset.m_bPickedUp;

		return false;
	}

	protected bool IsMissionAssetPlayerBoundAfterExpiry(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!state || !asset || !asset.m_bPickedUp || asset.m_bDelivered || asset.m_bDestroyed)
			return false;

		if (asset.m_bAttachedToCarrier && !asset.m_sCarriedByVehicleId.IsEmpty())
			return IsMissionAssetCarrierNearLivingPlayer(state, asset);

		if (asset.m_sKind != ASSET_KIND_CAPTIVE)
			return false;
		if (asset.m_sLastInteraction != PHASE_FOLLOWING && asset.m_sLastInteraction != PHASE_LOADED && asset.m_sLastInteraction != PHASE_EXTRACTING)
			return false;

		vector position = ResolveCaptiveStopPosition(asset, GetRuntimeEntity(asset.m_sEntityId));
		return !IsZeroVector(position) && IsAnyLivingPlayerNearObjective(position, POST_EXPIRY_PLAYER_ASSET_BUBBLE_METERS);
	}

	protected bool IsMissionAssetCarrierNearLivingPlayer(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!state || !asset || asset.m_sCarriedByVehicleId.IsEmpty())
			return false;

		if (IsPlayerCarrierId(asset.m_sCarriedByVehicleId))
			return IsLivingPlayerEntity(ResolvePlayerEntityForCarrierId(asset.m_sCarriedByVehicleId));

		vector carrierPosition;
		if (!TryResolveCarrierPosition(state, asset.m_sCarriedByVehicleId, asset.m_vCurrentPosition, carrierPosition))
			return false;

		return !IsZeroVector(carrierPosition) && IsAnyLivingPlayerNearObjective(carrierPosition, POST_EXPIRY_PLAYER_ASSET_BUBBLE_METERS);
	}

	protected bool HasDeliveredPlayerBoundMissionAssetAfterExpiry(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_bDelivered && !asset.m_bDestroyed && IsTransportableAsset(asset))
				return true;
		}

		return false;
	}

	protected bool AdvanceMissionStateMachine(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, int elapsedSeconds)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;

		bool changed;
		string previousPhase = mission.m_sRuntimePhase;
		if (mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == PHASE_CREATED)
			mission.m_sRuntimePhase = PHASE_ACTIVE;

		if (mission.m_sRuntimeType == "dynamic_defend_petros" && !state.m_bPetrosAlive)
		{
			MarkRuntimeMissionFailed(state, mission, "Petros was killed during the defense.");
			return true;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
		{
			bool exactConvoy = mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION;
			mission.m_iRuntimeCounterA += elapsedSeconds;
			if (mission.m_iRuntimeCounterB <= 0)
				mission.m_iRuntimeCounterB = ResolveConvoyIdleDelaySeconds(state, mission);
			if (mission.m_iRuntimeCounterC <= mission.m_iRuntimeCounterB)
				mission.m_iRuntimeCounterC = Math.Max(mission.m_iRuntimeCounterB + 300, ResolveConvoyArrivalSeconds(mission));

			if (mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == PHASE_CREATED || mission.m_sRuntimePhase == PHASE_ACTIVE || mission.m_sRuntimePhase == "convoy_static")
				mission.m_sRuntimePhase = PHASE_CONVOY_STAGING;

			if (mission.m_sRuntimePhase == PHASE_CONVOY_STAGING)
				mission.m_iRuntimeETASeconds = Math.Max(0, mission.m_iRuntimeCounterB - mission.m_iRuntimeCounterA);
			else if (!exactConvoy)
				mission.m_iRuntimeETASeconds = Math.Max(0, mission.m_iRuntimeCounterC - mission.m_iRuntimeCounterA);

			if (!exactConvoy && mission.m_sRuntimePhase != PHASE_CONVOY_CONTACT && mission.m_iRuntimeCounterA >= mission.m_iRuntimeCounterC && !AreRuntimeObjectivesComplete(state, mission.m_sInstanceId))
			{
				mission.m_sRuntimePhase = PHASE_CONVOY_ARRIVED;
				MarkRuntimeMissionFailed(state, mission, "Convoy reached its destination.");
				return true;
			}
		}

		UpdateMissionCountersFromObjectives(state, mission);
		UpdateMissionCountersFromAssets(state, mission);
		if (previousPhase != mission.m_sRuntimePhase)
			changed = true;

		return changed;
	}

	protected bool EnsureMissionRuntimeProp(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimeEntityId.IsEmpty())
			return false;
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;

		bool spawned = TrySpawnMissionRuntimeAssets(state, mission);
		bool convoyWithoutVehicleAssets = mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT && state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_VEHICLE) <= 0;
		if (!spawned && !HasRuntimeEntity(mission.m_sRuntimeEntityId) && !convoyWithoutVehicleAssets)
			spawned = TrySpawnMissionRuntimeProp(state, mission);

		if (convoyWithoutVehicleAssets && !spawned)
		{
			if (!mission.m_bRuntimeSpawned && !mission.m_bRuntimeFallback)
				return false;

			mission.m_bRuntimeSpawned = false;
			mission.m_bRuntimeFallback = false;
			return true;
		}

		if (mission.m_bRuntimeSpawned == spawned)
			return spawned;

		mission.m_bRuntimeSpawned = spawned;
		if (spawned)
			mission.m_bRuntimeFallback = false;
		else
			mission.m_bRuntimeFallback = true;

		return true;
	}

	protected void EnsureMissionCaptivesNeutralized(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_sKind != ASSET_KIND_CAPTIVE && asset.m_sRole != ROLE_CAPTIVE && asset.m_sRole != ROLE_CONVOY_CAPTIVE)
				continue;
			if (asset.m_bDestroyed || asset.m_bDelivered)
				continue;

			IEntity entity = GetRuntimeEntity(asset.m_sEntityId);
			if (entity)
				ApplyMissionAssetIdentity(entity, asset);
		}
	}

	protected bool EnsureMissionAssetsInitialized(HST_CampaignState state, HST_MissionDefinition definition, HST_ActiveMissionState mission)
	{
		if (!state || !definition || !mission || mission.m_sRuntimePrimitive == PRIMITIVE_ABSTRACT_FALLBACK)
			return false;
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;

		vector targetPosition = ResolveRuntimePropPosition(state, mission);
		vector hqPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		int cargoCount = Math.Max(1, Math.Max(definition.m_iCargoCount, mission.m_iRequiredCargoCount));
		int captiveCount = Math.Max(1, Math.Max(definition.m_iCaptiveCount, mission.m_iRequiredCaptiveCount));
		int vehicleCount = Math.Max(1, Math.Max(definition.m_iVehicleCount, mission.m_iRequiredVehicleCount));
		bool changed;

		if (mission.m_sRuntimePrimitive == PRIMITIVE_KILL_HVT)
		{
			changed = AddMissionAsset(state, mission, ASSET_KIND_CHARACTER, ROLE_HVT, PROP_HVT, targetPosition, targetPosition, 0) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_GUN_SHOP)
		{
			vector sellerPosition = targetPosition;
			if (!IsZeroVector(mission.m_vGunShopSellerPosition))
				sellerPosition = mission.m_vGunShopSellerPosition;
			else
				mission.m_vGunShopSellerPosition = sellerPosition;
			mission.m_vTargetPosition = sellerPosition;
			if (mission.m_sGunShopSellerAssetId.IsEmpty())
				mission.m_sGunShopSellerAssetId = string.Format("asset_%1_%2_%3", mission.m_sInstanceId, ROLE_GUN_SHOP_SELLER, 0);
			changed = AddMissionAsset(state, mission, ASSET_KIND_CHARACTER, ROLE_GUN_SHOP_SELLER, PROP_GUN_SHOP_SELLER, sellerPosition, sellerPosition, 0) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_DESTROY_TARGET)
		{
			if (mission.m_sMissionId == "destroy_or_steal_armor")
			{
				string vehiclePrefab = ResolveMissionVehiclePrefab(state, mission);
				for (int i = 0; i < vehicleCount; i++)
					changed = AddMissionAsset(state, mission, ASSET_KIND_VEHICLE, ROLE_DESTROY_TARGET, vehiclePrefab, targetPosition, hqPosition, i) || changed;
				return changed;
			}

			changed = AddMissionAsset(state, mission, ASSET_KIND_TARGET, ROLE_DESTROY_TARGET, PROP_DESTROY_TARGET, targetPosition, targetPosition, 0) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_HOLD_AREA || mission.m_sRuntimePrimitive == PRIMITIVE_CLEAR_AREA)
		{
			changed = AddMissionAsset(state, mission, ASSET_KIND_AREA, ROLE_HOLD_MARKER, PROP_HOLD_MARKER, targetPosition, targetPosition, 0) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_DELIVER_SUPPLIES)
		{
			for (int i = 0; i < cargoCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_CITY_SUPPLIES, PROP_CITY_SUPPLIES, hqPosition, targetPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
		{
			for (int i = 0; i < cargoCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_LOGISTICS_CARGO, ResolveLogisticsCargoPrefab(mission), targetPosition, hqPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_RESCUE_EXTRACT)
		{
			for (int i = 0; i < captiveCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CAPTIVE, ROLE_CAPTIVE, PROP_CAPTIVES, targetPosition, hqPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
		{
			return EnsureConvoyMissionAssetsInitialized(state, mission, definition);
		}

		changed = AddMissionAsset(state, mission, ASSET_KIND_AREA, ROLE_HOLD_MARKER, PROP_HOLD_MARKER, targetPosition, targetPosition, 0);
		return changed;
	}

	protected bool EnsureConvoyMissionAssetsInitialized(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionDefinition definition)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;
		if (state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_VEHICLE) > 0)
			return false;

		HST_GeneratedRouteState convoyRoute = ResolveMissionRoute(state, mission);
		vector convoyEnd;
		string convoyDestinationReason;
		if (!TryResolveConvoyEndPosition(state, mission, convoyEnd, convoyDestinationReason))
		{
			mission.m_sRuntimeFailureReason = "No road-resolved convoy destination: " + convoyDestinationReason;
			Print(string.Format("Partisan mission runtime | convoy asset plan failed for %1: %2", mission.m_sInstanceId, mission.m_sRuntimeFailureReason), LogLevel.WARNING);
			return false;
		}
		mission.m_vTargetPosition = convoyEnd;
		string convoyVehiclePrefab = ResolveMissionVehiclePrefab(state, mission);
		int convoyVehicleCount = ResolveConvoyVehicleCount(state, mission, definition);
		vector convoyStart;
		string convoySpawnPlanReason;
		array<vector> convoyStartSlots = {};
		if (!TryResolveConvoySpawnPlan(state, mission, convoyRoute, convoyEnd, convoyVehicleCount, convoyStart, convoyStartSlots, convoySpawnPlanReason))
		{
			mission.m_sRuntimeFailureReason = "No road-resolved convoy spawn plan found in required 2000-5000m band: " + convoySpawnPlanReason;
			Print(string.Format("Partisan mission runtime | convoy asset plan failed for %1: %2", mission.m_sInstanceId, mission.m_sRuntimeFailureReason), LogLevel.WARNING);
			return false;
		}
		if (IsZeroVector(convoyStart) || convoyStartSlots.Count() != convoyVehicleCount)
		{
			mission.m_sRuntimeFailureReason = "No road-resolved convoy spawn plan found in required 2000-5000m band: full-column probe returned incomplete slots.";
			Print(string.Format("Partisan mission runtime | convoy asset plan failed for %1: %2", mission.m_sInstanceId, mission.m_sRuntimeFailureReason), LogLevel.WARNING);
			return false;
		}

		for (int i = 0; i < convoyVehicleCount; i++)
		{
			vector startSlot = convoyStartSlots[i];
			AddMissionAsset(state, mission, ASSET_KIND_VEHICLE, ROLE_CONVOY_VEHICLE, convoyVehiclePrefab, startSlot, convoyEnd, i);
		}

		mission.m_iRequiredVehicleCount = convoyVehicleCount;
		mission.m_iRequiredCargoCount = 0;
		mission.m_iRequiredCaptiveCount = 0;
		EnsureConvoyMissionSpecificAssets(state, mission);
		mission.m_sRuntimeFailureReason = "";
		ConfigureConvoyObjectiveRequirements(state, mission, convoyVehicleCount);
		DeleteRuntimeEntity(mission.m_sRuntimeEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(mission.m_sRuntimeEntityId);
		if (runtimeEntity)
			runtimeEntity.m_bDestroyed = true;
		mission.m_bRuntimeSpawned = true;
		mission.m_bRuntimeFallback = false;
		Print(string.Format("Partisan mission runtime | convoy asset plan ready for %1 | vehicles %2 | start %3 | end %4", mission.m_sInstanceId, convoyVehicleCount, convoyStart, convoyEnd));
		return true;
	}

	protected bool EnsureConvoyMissionSpecificAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;
		if (mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION && !mission.m_sManifestId.IsEmpty())
			return false;
		if (state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_VEHICLE) <= 0)
			return false;

		bool changed;
		vector sourcePosition = ResolveConvoyPayloadSourcePosition(state, mission);
		if (mission.m_sMissionId == "convoy_money")
		{
			mission.m_iRequiredCargoCount = 1;
			if (state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_PAYLOAD) <= 0)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_CONVOY_PAYLOAD, PROP_BANK_MONEY, sourcePosition, ResolveConvoyPayloadDeliveryPosition(state, mission), 0) || changed;
		}
		else if (mission.m_sMissionId == "convoy_supplies")
		{
			mission.m_iRequiredCargoCount = 1;
			if (state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_PAYLOAD) <= 0)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_CONVOY_PAYLOAD, PROP_CONVOY_PAYLOAD, sourcePosition, ResolveConvoyPayloadDeliveryPosition(state, mission), 0) || changed;
		}
		else if (mission.m_sMissionId == "convoy_prisoners")
		{
			mission.m_iRequiredCaptiveCount = 1;
			if (state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_CAPTIVE) <= 0)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CAPTIVE, ROLE_CONVOY_CAPTIVE, PROP_CAPTIVES, sourcePosition, ResolveConvoyPayloadDeliveryPosition(state, mission), 0) || changed;
		}

		return changed;
	}

	protected vector ResolveConvoyPayloadSourcePosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != ROLE_CONVOY_VEHICLE)
				continue;
			if (!IsZeroVector(asset.m_vCurrentPosition))
				return asset.m_vCurrentPosition;
			if (!IsZeroVector(asset.m_vSourcePosition))
				return asset.m_vSourcePosition;
		}

		if (!IsZeroVector(mission.m_vTargetPosition))
			return mission.m_vTargetPosition;
		return state.m_vHQPosition;
	}

	protected vector ResolveConvoyPayloadDeliveryPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		if (mission.m_sMissionId == "convoy_supplies")
		{
			HST_ZoneState supportTown = ResolveConvoySupportTown(state, mission);
			if (supportTown)
				return supportTown.m_vPosition;
		}

		return state.m_vHQPosition;
	}

	protected HST_ZoneState ResolveConvoySupportTown(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return null;

		HST_ZoneState targetZone = state.FindZone(mission.m_sTargetZoneId);
		if (targetZone && targetZone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			return targetZone;

		vector searchPosition = mission.m_vTargetPosition;
		if (IsZeroVector(searchPosition) && targetZone)
			searchPosition = targetZone.m_vPosition;
		if (IsZeroVector(searchPosition))
			searchPosition = state.m_vHQPosition;

		HST_ZoneState bestTown;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
				continue;

			float distanceSq = DistanceSq2D(searchPosition, zone.m_vPosition);
			if (distanceSq >= bestDistanceSq)
				continue;

			bestDistanceSq = distanceSq;
			bestTown = zone;
		}

		return bestTown;
	}

	protected bool SyncConvoyPayloadAssetPositions(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		vector payloadPosition;
		bool exactConvoy = mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION;
		if (exactConvoy)
		{
			HST_MissionAssetState exactCargo;
			foreach (HST_MissionAssetState candidateCargo : state.m_aMissionAssets)
			{
				if (candidateCargo && candidateCargo.m_sMissionInstanceId == mission.m_sInstanceId
					&& (candidateCargo.m_sRole == ROLE_CONVOY_PAYLOAD || candidateCargo.m_sRole == ROLE_CONVOY_CAPTIVE))
				{
					exactCargo = candidateCargo;
					break;
				}
			}
			if (!exactCargo || exactCargo.m_sAssignedVehicleSlotId.IsEmpty())
				return false;
			foreach (HST_MissionAssetState exactVehicle : state.m_aMissionAssets)
			{
				if (!exactVehicle || exactVehicle.m_sMissionInstanceId != mission.m_sInstanceId || exactVehicle.m_sRole != ROLE_CONVOY_VEHICLE)
					continue;
				if (exactVehicle.m_sManifestSlotId != exactCargo.m_sAssignedVehicleSlotId)
					continue;
				payloadPosition = exactVehicle.m_vCurrentPosition;
				if (IsZeroVector(payloadPosition))
					payloadPosition = exactVehicle.m_vLastKnownPosition;
				break;
			}
		}
		else
		{
			payloadPosition = ResolveConvoyPayloadSourcePosition(state, mission);
		}
		if (IsZeroVector(payloadPosition))
			return false;

		bool changed;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_sRole != ROLE_CONVOY_PAYLOAD && asset.m_sRole != ROLE_CONVOY_CAPTIVE)
				continue;
			if (asset.m_bPickedUp || asset.m_bDelivered || asset.m_bDestroyed)
				continue;
			if (DistanceSq2D(asset.m_vCurrentPosition, payloadPosition) <= 1.0)
				continue;

			asset.m_vCurrentPosition = payloadPosition;
			asset.m_vLastKnownPosition = payloadPosition;
			changed = true;
		}

		return changed;
	}

	protected bool IsExactMissionConvoyCargoProjectionPlayerBoundOrResolved(HST_MissionAssetState asset)
	{
		if (!asset)
			return true;
		if (asset.m_bPickedUp || asset.m_bDelivered || asset.m_bDestroyed || asset.m_bOutcomeApplied)
			return true;
		if (asset.m_sRole == ROLE_CONVOY_CAPTIVE && !asset.m_bAlive)
			return true;
		return IsPlayerCarrierId(asset.m_sCarriedByVehicleId);
	}

	protected bool ResolveExactMissionConvoyCargoCarrierAuthority(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState cargo,
		out HST_MissionAssetState carrierAsset,
		out HST_ConvoyElementState carrierElement)
	{
		carrierAsset = null;
		carrierElement = null;
		if (!state || !mission || !cargo || cargo.m_sAssignedVehicleSlotId.IsEmpty()
			|| cargo.m_sOperationId != mission.m_sOperationId || cargo.m_sManifestId != mission.m_sManifestId)
			return false;

		int carrierAssetClaimants;
		foreach (HST_MissionAssetState candidateAsset : state.m_aMissionAssets)
		{
			if (!candidateAsset || candidateAsset.m_sMissionInstanceId != mission.m_sInstanceId
				|| candidateAsset.m_sOperationId != mission.m_sOperationId || candidateAsset.m_sManifestId != mission.m_sManifestId
				|| candidateAsset.m_sRole != ROLE_CONVOY_VEHICLE
				|| candidateAsset.m_sManifestSlotId != cargo.m_sAssignedVehicleSlotId)
				continue;
			carrierAsset = candidateAsset;
			carrierAssetClaimants++;
		}
		if (carrierAssetClaimants != 1 || !carrierAsset)
			return false;

		int carrierElementClaimants;
		foreach (HST_ConvoyElementState candidateElement : state.m_aConvoyElements)
		{
			if (!candidateElement || candidateElement.m_sMissionInstanceId != mission.m_sInstanceId
				|| candidateElement.m_sOperationId != mission.m_sOperationId
				|| candidateElement.m_sManifestId != mission.m_sManifestId
				|| candidateElement.m_sVehicleSlotId != cargo.m_sAssignedVehicleSlotId)
				continue;
			carrierElement = candidateElement;
			carrierElementClaimants++;
		}
		if (carrierElementClaimants != 1 || !carrierElement)
			return false;

		return carrierElement.m_sVehicleAssetId == carrierAsset.m_sAssetId
			&& carrierAsset.m_sConvoyElementId == carrierElement.m_sElementId
			&& cargo.m_sConvoyElementId == carrierElement.m_sElementId
			&& carrierAsset.m_sAssignedVehicleSlotId == cargo.m_sAssignedVehicleSlotId;
	}

	protected bool IsExactMissionConvoyTerminalCarrierGroundRecovery(
		HST_ActiveMissionState mission,
		HST_MissionAssetState cargo,
		HST_MissionAssetState carrierAsset,
		HST_ConvoyElementState carrierElement)
	{
		if (!mission || !cargo || !carrierAsset || !carrierElement)
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;
		if (cargo.m_bPickedUp || cargo.m_bDelivered || cargo.m_bDestroyed || cargo.m_bOutcomeApplied)
			return false;
		if (cargo.m_sRole == ROLE_CONVOY_CAPTIVE && !cargo.m_bAlive)
			return false;

		if (carrierElement.m_bMobile)
			return false;
		if (carrierElement.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED)
			return carrierAsset.m_bDestroyed;
		if (carrierElement.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED)
			return carrierAsset.m_bDelivered || carrierAsset.m_sLastInteraction == "captured";
		return false;
	}

	protected bool ProjectExactMissionConvoyCargoAtCarrier(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		HST_MissionAssetState carrierAsset,
		IEntity carrierEntity)
	{
		if (!state || !mission || !asset || !carrierAsset || !carrierEntity || asset.m_sEntityId.IsEmpty())
			return false;
		if (!asset.m_sCarriedByVehicleId.IsEmpty() && asset.m_sCarriedByVehicleId != carrierAsset.m_sAssetId)
			return false;

		vector carrierPosition = carrierEntity.GetOrigin();
		if (IsZeroVector(carrierPosition))
			return false;
		IEntity existingEntity = GetRuntimeEntity(asset.m_sEntityId);
		if (asset.m_sRole == ROLE_CONVOY_CAPTIVE && existingEntity)
		{
			IEntity seatedVehicle = ResolveEntityVehicle(existingEntity);
			if (seatedVehicle == carrierEntity)
			{
				bool seatedChanged;
				if (!asset.m_bSpawned)
				{
					asset.m_bSpawned = true;
					seatedChanged = true;
				}
				if (!asset.m_bAttachedToCarrier || asset.m_sCarriedByVehicleId != carrierAsset.m_sAssetId)
				{
					asset.m_bAttachedToCarrier = true;
					asset.m_sCarriedByVehicleId = carrierAsset.m_sAssetId;
					seatedChanged = true;
				}
				asset.m_vCurrentPosition = carrierPosition;
				asset.m_vLastKnownPosition = carrierPosition;
				RegisterAssetRuntimeEntityState(state, asset, carrierPosition, carrierEntity.GetYawPitchRoll());
				return seatedChanged;
			}
			if (seatedVehicle)
				return false;
		}
		vector projectionPosition = BuildExactMissionConvoyCarrierProjectionPosition(carrierEntity, asset);
		vector projectionAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(carrierEntity.GetYawPitchRoll());
		bool entityChanged;
		IEntity entity = EnsureExactMissionConvoyCargoProjectionEntity(state, mission, asset, projectionPosition, projectionAngles, entityChanged);
		if (!entity)
			return entityChanged;

		bool changed = entityChanged;
		if (asset.m_sRole == ROLE_CONVOY_CAPTIVE)
		{
			string boardingReason;
			if (!TryMoveCaptiveIntoVehicle(entity, carrierEntity, boardingReason)
				|| ResolveEntityVehicle(entity) != carrierEntity)
				return changed;
			changed = true;
		}
		else if (entity.GetParent() != carrierEntity)
		{
			IEntity previousParent = entity.GetParent();
			if (previousParent)
				previousParent.RemoveChild(entity, true);
			carrierEntity.AddChild(entity, -1, EAddChildFlags.RECALC_LOCAL_TRANSFORM);
			if (entity.GetParent() != carrierEntity)
				return changed;
			changed = true;
		}
		if (vector.Distance(asset.m_vCurrentPosition, carrierPosition) > 0.5
			|| vector.Distance(asset.m_vLastKnownPosition, carrierPosition) > 0.5)
		{
			asset.m_vCurrentPosition = carrierPosition;
			asset.m_vLastKnownPosition = carrierPosition;
			changed = true;
		}
		if (!asset.m_bSpawned)
		{
			asset.m_bSpawned = true;
			changed = true;
		}
		if (!asset.m_bAttachedToCarrier || asset.m_sCarriedByVehicleId != carrierAsset.m_sAssetId)
		{
			asset.m_bAttachedToCarrier = true;
			asset.m_sCarriedByVehicleId = carrierAsset.m_sAssetId;
			changed = true;
		}
		RegisterAssetRuntimeEntityState(state, asset, projectionPosition, projectionAngles);
		return changed;
	}

	protected bool ProjectExactMissionConvoyCargoAtDurablePosition(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		HST_MissionAssetState carrierAsset,
		HST_ConvoyElementState carrierElement)
	{
		if (!state || !mission || !asset || !carrierAsset || !carrierElement || asset.m_sEntityId.IsEmpty())
			return false;
		if (!asset.m_sCarriedByVehicleId.IsEmpty() && asset.m_sCarriedByVehicleId != carrierAsset.m_sAssetId)
			return false;

		vector durablePosition = ResolveExactMissionConvoyStandaloneRecoveryPosition(asset, carrierAsset, carrierElement);
		if (IsZeroVector(durablePosition))
			return false;
		float groundOffset = HST_WorldPositionService.PROP_GROUND_OFFSET;
		if (asset.m_sRole == ROLE_CONVOY_CAPTIVE)
			groundOffset = HST_WorldPositionService.CHARACTER_GROUND_OFFSET;
		vector projectionPosition = HST_WorldPositionService.ResolveSafeGroundPosition(durablePosition, groundOffset, false, 2.0);
		vector projectionAngles = BuildRuntimePropAngles(state, mission);
		IEntity existingEntity = GetRuntimeEntity(asset.m_sEntityId);
		if (existingEntity && existingEntity.GetParent())
			existingEntity.GetParent().RemoveChild(existingEntity, true);
		bool entityChanged;
		IEntity entity = EnsureExactMissionConvoyCargoProjectionEntity(state, mission, asset, projectionPosition, projectionAngles, entityChanged);
		if (!entity)
			return entityChanged;

		bool changed = entityChanged;
		if (vector.Distance(asset.m_vCurrentPosition, projectionPosition) > 0.5
			|| vector.Distance(asset.m_vLastKnownPosition, projectionPosition) > 0.5)
		{
			asset.m_vCurrentPosition = projectionPosition;
			asset.m_vLastKnownPosition = projectionPosition;
			changed = true;
		}
		if (!asset.m_bSpawned)
		{
			asset.m_bSpawned = true;
			changed = true;
		}
		if (asset.m_bAttachedToCarrier || !asset.m_sCarriedByVehicleId.IsEmpty())
		{
			asset.m_bAttachedToCarrier = false;
			asset.m_sCarriedByVehicleId = "";
			changed = true;
		}
		RegisterAssetRuntimeEntityState(state, asset, projectionPosition, projectionAngles);
		return changed;
	}

	protected IEntity EnsureExactMissionConvoyCargoProjectionEntity(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		vector position,
		vector angles,
		out bool changed)
	{
		changed = false;
		if (!state || !mission || !asset || asset.m_sEntityId.IsEmpty() || IsZeroVector(position))
			return null;

		IEntity entity = GetRuntimeEntity(asset.m_sEntityId);
		if (entity && asset.m_sRole == ROLE_CONVOY_CAPTIVE && ResolveEntityVehicle(entity))
		{
			DeleteRuntimeEntity(asset.m_sEntityId);
			entity = null;
			changed = true;
		}
		if (!entity)
		{
			string prefab = asset.m_sPrefab;
			if (prefab.IsEmpty())
			{
				changed = MarkExactMissionConvoyCargoProjectionMissing(state, asset) || changed;
				return null;
			}

			GenericEntity spawned = HST_WorldPositionService.SpawnPrefab(prefab, position, angles);
			if (!spawned)
			{
				changed = MarkExactMissionConvoyCargoProjectionMissing(state, asset) || changed;
				return null;
			}

			entity = spawned;
			m_aRuntimeEntityIds.Insert(asset.m_sEntityId);
			m_aRuntimeEntities.Insert(entity);
			changed = true;
		}
		if (!entity.GetPrefabData() || entity.GetPrefabData().GetPrefabName() != asset.m_sPrefab)
		{
			DeleteRuntimeEntity(asset.m_sEntityId);
			changed = MarkExactMissionConvoyCargoProjectionMissing(state, asset) || changed;
			return null;
		}

		if (vector.Distance(entity.GetOrigin(), position) > 0.25)
		{
			HST_WorldPositionService.ApplyUprightEntityTransform(entity, position, angles);
			changed = true;
		}
		ApplyCampaignDebugEntityName(entity, asset.m_sRole, asset.m_sAssetId);
		ApplyMissionAssetIdentity(entity, asset);
		HST_MissionAssetComponent assetComponent = HST_MissionAssetComponent.Cast(entity.FindComponent(HST_MissionAssetComponent));
		if (assetComponent)
			assetComponent.ConfigureMissionAsset(asset.m_sAssetId, mission.m_sInstanceId, asset.m_sRole);
		return entity;
	}

	protected bool MarkExactMissionConvoyCargoProjectionMissing(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!state || !asset)
			return false;
		bool changed;
		if (asset.m_bSpawned)
		{
			asset.m_bSpawned = false;
			changed = true;
		}
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity && runtimeEntity.m_bSpawned)
		{
			runtimeEntity.m_bSpawned = false;
			changed = true;
		}
		return changed;
	}

	protected vector BuildExactMissionConvoyCarrierProjectionPosition(IEntity carrierEntity, HST_MissionAssetState asset)
	{
		if (!carrierEntity)
			return "0 0 0";

		vector position = carrierEntity.GetOrigin();
		vector angles = carrierEntity.GetYawPitchRoll();
		float radians = angles[0] * Math.DEG2RAD;
		float sideOffset = 1.25;
		float rearOffset = -0.75;
		float heightOffset = 1.0;
		if (asset && asset.m_sRole == ROLE_CONVOY_CAPTIVE)
		{
			sideOffset = 1.75;
			rearOffset = -0.5;
			heightOffset = 0.2;
		}

		position[0] = position[0] + sideOffset * Math.Cos(radians) - rearOffset * Math.Sin(radians);
		position[1] = position[1] + heightOffset;
		position[2] = position[2] + sideOffset * Math.Sin(radians) + rearOffset * Math.Cos(radians);
		return position;
	}

	protected vector ResolveExactMissionConvoyStandaloneRecoveryPosition(
		HST_MissionAssetState asset,
		HST_MissionAssetState carrierAsset,
		HST_ConvoyElementState carrierElement)
	{
		if (asset && !IsZeroVector(asset.m_vLastKnownPosition))
			return asset.m_vLastKnownPosition;
		if (asset && !IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;
		if (carrierAsset && !IsZeroVector(carrierAsset.m_vLastKnownPosition))
			return carrierAsset.m_vLastKnownPosition;
		if (carrierAsset && !IsZeroVector(carrierAsset.m_vCurrentPosition))
			return carrierAsset.m_vCurrentPosition;
		if (carrierElement)
			return carrierElement.m_vCurrentPosition;
		return "0 0 0";
	}

	protected bool DematerializeExactMissionConvoyCargoProjection(
		HST_CampaignState state,
		HST_MissionAssetState asset,
		string assignedCarrierAssetId)
	{
		if (!state || !asset || IsExactMissionConvoyCargoProjectionPlayerBoundOrResolved(asset))
			return false;
		if (!asset.m_sCarriedByVehicleId.IsEmpty() && !assignedCarrierAssetId.IsEmpty()
			&& asset.m_sCarriedByVehicleId != assignedCarrierAssetId)
			return false;

		bool changed;
		IEntity entity = GetRuntimeEntity(asset.m_sEntityId);
		if (entity)
		{
			StopCaptiveFollowController(asset);
			DeleteRuntimeEntity(asset.m_sEntityId);
			changed = true;
		}
		if (asset.m_bSpawned)
		{
			asset.m_bSpawned = false;
			changed = true;
		}
		if (asset.m_bAttachedToCarrier || !asset.m_sCarriedByVehicleId.IsEmpty())
		{
			asset.m_bAttachedToCarrier = false;
			asset.m_sCarriedByVehicleId = "";
			changed = true;
		}

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			if (runtimeEntity.m_bSpawned || runtimeEntity.m_bDestroyed || runtimeEntity.m_bRecovered)
				changed = true;
			runtimeEntity.m_bSpawned = false;
			runtimeEntity.m_bDestroyed = false;
			runtimeEntity.m_bRecovered = false;
			runtimeEntity.m_vPosition = asset.m_vCurrentPosition;
		}
		return changed;
	}

	protected bool AddMissionAsset(HST_CampaignState state, HST_ActiveMissionState mission, string kind, string role, string prefab, vector sourcePosition, vector targetPosition, int index)
	{
		if (!state || !mission || role.IsEmpty())
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;

		string assetId = string.Format("asset_%1_%2_%3", mission.m_sInstanceId, role, index);
		HST_MissionAssetState existingAsset = state.FindMissionAsset(assetId);
		if (existingAsset)
			return RepairMissionAssetRecord(existingAsset, mission, kind, role, prefab, sourcePosition, targetPosition, index);

		HST_MissionAssetState asset = new HST_MissionAssetState();
		asset.m_sAssetId = assetId;
		asset.m_sEntityId = assetId;
		asset.m_sMissionInstanceId = mission.m_sInstanceId;
		asset.m_sKind = kind;
		asset.m_sRole = role;
		asset.m_sPrefab = prefab;
		if (role == ROLE_CONVOY_VEHICLE || role == ROLE_GUN_SHOP_SELLER || role == ROLE_GUN_SHOP_DELIVERY_VEHICLE || role == ROLE_GUN_SHOP_DELIVERY_DRIVER)
			asset.m_vSourcePosition = sourcePosition;
		else
			asset.m_vSourcePosition = OffsetMissionAssetPosition(sourcePosition, index);
		asset.m_vTargetPosition = targetPosition;
		asset.m_vCurrentPosition = asset.m_vSourcePosition;
		asset.m_vLastKnownPosition = asset.m_vSourcePosition;
		asset.m_iDeadlineSecond = mission.m_iActiveUntilSecond;
		asset.m_iCargoCapacityCost = ResolveAssetCapacityCost(kind, role);
		asset.m_iInteractionRadiusMeters = ResolveAssetInteractionRadius(kind, role);
		EnsureDemolitionDefaults(asset);
		asset.m_bAlive = true;
		state.m_aMissionAssets.Insert(asset);
		return true;
	}

	protected bool RepairMissionAssetRecord(HST_MissionAssetState asset, HST_ActiveMissionState mission, string kind, string role, string prefab, vector sourcePosition, vector targetPosition, int index)
	{
		if (!asset || !mission)
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission)
			|| HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
			return false;

		bool changed;
		vector resolvedSourcePosition;
		if (role == ROLE_CONVOY_VEHICLE || role == ROLE_GUN_SHOP_SELLER || role == ROLE_GUN_SHOP_DELIVERY_VEHICLE || role == ROLE_GUN_SHOP_DELIVERY_DRIVER)
			resolvedSourcePosition = sourcePosition;
		else
			resolvedSourcePosition = OffsetMissionAssetPosition(sourcePosition, index);

		if (asset.m_sMissionInstanceId != mission.m_sInstanceId)
		{
			asset.m_sMissionInstanceId = mission.m_sInstanceId;
			changed = true;
		}
		if (asset.m_sKind != kind)
		{
			asset.m_sKind = kind;
			changed = true;
		}
		if (asset.m_sRole != role)
		{
			asset.m_sRole = role;
			changed = true;
		}
		if (asset.m_sPrefab.IsEmpty() || (kind == ASSET_KIND_CAPTIVE && asset.m_sPrefab != prefab))
		{
			asset.m_sPrefab = prefab;
			changed = true;
		}
		if (asset.m_sEntityId.IsEmpty())
		{
			asset.m_sEntityId = asset.m_sAssetId;
			changed = true;
		}
		if (IsZeroVector(asset.m_vSourcePosition) && !IsZeroVector(resolvedSourcePosition))
		{
			asset.m_vSourcePosition = resolvedSourcePosition;
			changed = true;
		}
		if (IsZeroVector(asset.m_vTargetPosition) && !IsZeroVector(targetPosition))
		{
			asset.m_vTargetPosition = targetPosition;
			changed = true;
		}
		if (!asset.m_bPickedUp && !IsTerminalMissionAssetState(asset))
		{
			vector repairedPosition = asset.m_vCurrentPosition;
			if (IsZeroVector(repairedPosition))
				repairedPosition = asset.m_vSourcePosition;
			if (IsZeroVector(repairedPosition))
				repairedPosition = resolvedSourcePosition;
			if (!IsZeroVector(repairedPosition) && DistanceSq2D(asset.m_vCurrentPosition, repairedPosition) > 1.0)
			{
				asset.m_vCurrentPosition = repairedPosition;
				changed = true;
			}
			if (!IsZeroVector(repairedPosition) && IsZeroVector(asset.m_vLastKnownPosition))
			{
				asset.m_vLastKnownPosition = repairedPosition;
				changed = true;
			}
		}
		if (asset.m_iDeadlineSecond <= 0 && mission.m_iActiveUntilSecond > 0)
		{
			asset.m_iDeadlineSecond = mission.m_iActiveUntilSecond;
			changed = true;
		}
		int capacityCost = ResolveAssetCapacityCost(kind, role);
		if (asset.m_iCargoCapacityCost <= 0 && capacityCost > 0)
		{
			asset.m_iCargoCapacityCost = capacityCost;
			changed = true;
		}
		int interactionRadius = ResolveAssetInteractionRadius(kind, role);
		if (asset.m_iInteractionRadiusMeters <= 0 && interactionRadius > 0)
		{
			asset.m_iInteractionRadiusMeters = interactionRadius;
			changed = true;
		}
		if (!IsTerminalMissionAssetState(asset) && !asset.m_bAlive)
		{
			asset.m_bAlive = true;
			changed = true;
		}
		if (EnsureDemolitionDefaults(asset))
			changed = true;

		return changed;
	}

	protected bool EnsureDemolitionDefaults(HST_MissionAssetState asset)
	{
		if (!asset)
			return false;

		if (asset.m_sKind != ASSET_KIND_TARGET && asset.m_sRole != ROLE_DESTROY_TARGET)
			return false;

		float requiredDamage = ResolveDemolitionRequiredDamage(asset);
		if (asset.m_fDemolitionRequiredDamage > 0.0)
			return false;

		asset.m_fDemolitionRequiredDamage = requiredDamage;
		return true;
	}

	protected bool IsTerminalMissionAssetState(HST_MissionAssetState asset)
	{
		if (!asset)
			return true;

		return asset.m_bDestroyed || asset.m_bDelivered || asset.m_sLastInteraction == PHASE_CAPTURED || asset.m_bOutcomeApplied;
	}

	protected void LinkObjectivesToMissionAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			HST_MissionAssetState asset = FindRepresentativeAssetForObjective(state, mission, objective);
			if (!asset)
				continue;

			objective.m_sLinkedRuntimeEntityId = asset.m_sEntityId;
			objective.m_sPhysicalEntityId = asset.m_sEntityId;
			if (objective.m_vPosition[0] == 0 && objective.m_vPosition[1] == 0 && objective.m_vPosition[2] == 0)
				objective.m_vPosition = asset.m_vSourcePosition;
		}
	}

	protected bool TrySpawnMissionRuntimeAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;

		bool foundAsset;
		bool hasUsableAsset;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
				continue;

			foundAsset = true;
			if (asset.m_bDelivered || asset.m_bDestroyed)
			{
				hasUsableAsset = true;
				continue;
			}

			if (asset.m_bPickedUp && !asset.m_bDelivered && asset.m_sKind != ASSET_KIND_CAPTIVE)
			{
				hasUsableAsset = true;
				continue;
			}

			if (asset.m_bPickedUp && !asset.m_bDelivered && asset.m_sKind == ASSET_KIND_CAPTIVE && asset.m_sLastInteraction == PHASE_LOADED)
			{
				hasUsableAsset = true;
				continue;
			}

			if (asset.m_sPrefab.IsEmpty())
				continue;

			if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT && asset.m_sRole == ROLE_CONVOY_VEHICLE)
			{
				hasUsableAsset = true;
				continue;
			}

			if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT && (asset.m_sRole == ROLE_CONVOY_PAYLOAD || asset.m_sRole == ROLE_CONVOY_CAPTIVE))
			{
				hasUsableAsset = true;
				continue;
			}

			if (HasRuntimeEntity(asset.m_sEntityId))
			{
				hasUsableAsset = true;
				continue;
			}

			if (TryBindExistingRadioTower(state, mission, asset))
			{
				hasUsableAsset = true;
				continue;
			}

			vector position;
			if (asset.m_sKind == ASSET_KIND_VEHICLE)
			{
				if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(asset.m_vCurrentPosition, position, true))
				{
					mission.m_sRuntimeFailureReason = "No dry vehicle-safe position for " + asset.m_sRole;
					Print(string.Format("Partisan mission runtime | vehicle asset spawn skipped for %1 at %2: %3", asset.m_sAssetId, asset.m_vCurrentPosition, mission.m_sRuntimeFailureReason), LogLevel.WARNING);
					continue;
				}
			}
			else
			{
				float groundOffset = HST_WorldPositionService.PROP_GROUND_OFFSET;
				if (asset.m_sKind == ASSET_KIND_CAPTIVE)
					groundOffset = HST_WorldPositionService.CHARACTER_GROUND_OFFSET;
				position = HST_WorldPositionService.ResolveSafeGroundPosition(asset.m_vCurrentPosition, groundOffset, false, 2.0);
			}
			vector angles = BuildRuntimePropAngles(state, mission);
			GenericEntity entity = HST_WorldPositionService.SpawnPrefab(asset.m_sPrefab, position, angles);
			if (!entity && asset.m_sKind == ASSET_KIND_CAPTIVE && asset.m_sPrefab != PROP_CAPTIVES)
				entity = HST_WorldPositionService.SpawnPrefab(PROP_CAPTIVES, position, angles);
			if (!entity && asset.m_sPrefab != PROP_HOLD_MARKER)
				entity = HST_WorldPositionService.SpawnPrefab(PROP_HOLD_MARKER, position, angles);

			if (!entity)
			{
				Print(string.Format("Partisan mission runtime | asset spawn failed for %1 using %2", asset.m_sAssetId, asset.m_sPrefab), LogLevel.WARNING);
				continue;
			}
			ApplyCampaignDebugEntityName(entity, asset.m_sRole, asset.m_sAssetId);
			if (asset.m_sKind == ASSET_KIND_VEHICLE || HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(asset.m_sPrefab))
				HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(entity);
			if (ShouldApplyUprightMissionAssetTransform(asset))
				HST_WorldPositionService.ApplyUprightEntityTransform(entity, position, angles);
			ApplyMissionAssetIdentity(entity, asset);

			HST_MissionAssetComponent assetComponent = HST_MissionAssetComponent.Cast(entity.FindComponent(HST_MissionAssetComponent));
			if (assetComponent)
				assetComponent.ConfigureMissionAsset(asset.m_sAssetId, mission.m_sInstanceId, asset.m_sRole);

			asset.m_bSpawned = true;
			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
			m_aRuntimeEntityIds.Insert(asset.m_sEntityId);
			m_aRuntimeEntities.Insert(entity);
			RegisterAssetRuntimeEntityState(state, asset, position, angles);
			if (asset.m_sRole == ROLE_CONVOY_VEHICLE && asset.m_sLastInteraction.IsEmpty())
			{
				asset.m_sLastInteraction = "convoy_static";
				if (mission.m_sRuntimePhase == PHASE_ACTIVE || mission.m_sRuntimePhase == PHASE_CREATED)
					mission.m_sRuntimePhase = "convoy_static";
			}
			hasUsableAsset = true;
			Print(string.Format("Partisan mission runtime | spawned asset %1 role %2 for %3 at %4", asset.m_sPrefab, asset.m_sRole, mission.m_sInstanceId, position));
			LogMissionCaptiveProjection(entity, asset);
		}

		return foundAsset && hasUsableAsset;
	}

	protected bool TryBindExistingRadioTower(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission)
			|| HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
			return false;
		if (!ShouldBindExistingRadioTower(mission, asset))
			return false;

		vector searchCenter = asset.m_vCurrentPosition;
		HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
		if (zone)
			searchCenter = zone.m_vPosition;

		IEntity tower = FindExistingRadioTower(searchCenter);
		if (!tower)
			return false;

		vector position = tower.GetOrigin();
		vector angles = tower.GetYawPitchRoll();
		asset.m_bSpawned = true;
		asset.m_bAlive = true;
		asset.m_vSourcePosition = position;
		asset.m_vCurrentPosition = position;
		asset.m_vLastKnownPosition = position;
		m_aRuntimeEntityIds.Insert(asset.m_sEntityId);
		m_aRuntimeEntities.Insert(tower);
		m_aBorrowedWorldEntities.Insert(tower);
		RegisterAssetRuntimeEntityState(state, asset, position, angles);

		string prefab = ResolveEntityPrefab(tower);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity && !prefab.IsEmpty())
			runtimeEntity.m_sPrefab = prefab;

		Print(string.Format("Partisan mission runtime | bound existing radio tower %1 to asset %2 for %3 at %4", prefab, asset.m_sAssetId, mission.m_sInstanceId, position));
		return true;
	}

	protected bool ShouldBindExistingRadioTower(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset)
			return false;
		if (mission.m_sRuntimePrimitive != PRIMITIVE_DESTROY_TARGET || asset.m_sKind != ASSET_KIND_TARGET || asset.m_sRole != ROLE_DESTROY_TARGET)
			return false;

		return mission.m_sMissionId == MISSION_DESTROY_RADIO_TOWER || mission.m_sMissionId == MISSION_STOP_TOWER_REBUILD;
	}

	protected IEntity FindExistingRadioTower(vector searchCenter)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		m_aExistingRadioTowerCandidates.Clear();
		world.QueryEntitiesBySphere(searchCenter, EXISTING_RADIO_TOWER_SEARCH_RADIUS_METERS, AddExistingRadioTowerCandidate, null, EQueryEntitiesFlags.ALL);

		IEntity nearest;
		float nearestDistanceSq = EXISTING_RADIO_TOWER_SEARCH_RADIUS_METERS * EXISTING_RADIO_TOWER_SEARCH_RADIUS_METERS;
		foreach (IEntity candidate : m_aExistingRadioTowerCandidates)
		{
			float distanceSq = DistanceSq2D(searchCenter, candidate.GetOrigin());
			if (nearest && distanceSq >= nearestDistanceSq)
				continue;

			nearest = candidate;
			nearestDistanceSq = distanceSq;
		}

		m_aExistingRadioTowerCandidates.Clear();
		return nearest;
	}

	protected bool AddExistingRadioTowerCandidate(IEntity entity)
	{
		if (!IsTransmitterTowerEntity(entity))
			return true;
		if (m_aRuntimeEntities.Contains(entity))
			return true;
		if (HST_MissionAssetComponent.Cast(entity.FindComponent(HST_MissionAssetComponent)))
			return true;
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager || damageManager.GetState() == EDamageState.DESTROYED)
			return true;

		m_aExistingRadioTowerCandidates.Insert(entity);
		return true;
	}

	protected bool IsTransmitterTowerEntity(IEntity entity)
	{
		if (!entity)
			return false;

		MapDescriptorComponent descriptor = MapDescriptorComponent.Cast(entity.FindComponent(MapDescriptorComponent));
		if (descriptor && descriptor.GetBaseType() == EMapDescriptorType.MDT_TRANSMITTER)
			return true;

		string prefab = ResolveEntityPrefab(entity);
		if (!prefab.Contains(RADIO_TOWER_PREFAB_TOKEN))
			return false;
		if (prefab.Contains("TransmitterTower_01_light") || prefab.Contains("_base.et"))
			return false;

		return true;
	}

	protected string ResolveEntityPrefab(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "";

		return entity.GetPrefabData().GetPrefabName();
	}

	protected bool TrySpawnMissionRuntimeProp(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimeEntityId.IsEmpty() || mission.m_sRuntimePrimitive == PRIMITIVE_ABSTRACT_FALLBACK)
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT && state.CountMissionAssets(mission.m_sInstanceId, ROLE_CONVOY_VEHICLE) <= 0)
		{
			Print(string.Format("Partisan mission runtime | convoy fallback prop suppressed for %1 because no convoy vehicle assets were planned: %2", mission.m_sInstanceId, ReportText(mission.m_sRuntimeFailureReason)), LogLevel.WARNING);
			return false;
		}

		vector position = ResolveRuntimePropPosition(state, mission);
		string prefab = SelectRuntimePropPrefab(mission.m_sRuntimePrimitive);
		if (prefab.IsEmpty())
			return false;

		vector angles = BuildRuntimePropAngles(state, mission);
		GenericEntity entity = HST_WorldPositionService.SpawnPrefab(prefab, position, angles);
		if (!entity && prefab != PROP_HOLD_MARKER)
			entity = HST_WorldPositionService.SpawnPrefab(PROP_HOLD_MARKER, position, angles);

		if (!entity)
		{
			Print(string.Format("Partisan mission runtime | prop spawn failed for %1 using %2", mission.m_sInstanceId, prefab), LogLevel.WARNING);
			return false;
		}

		HST_WorldPositionService.ApplyUprightEntityTransform(entity, position, angles);
		ApplyCampaignDebugEntityName(entity, mission.m_sRuntimePrimitive, mission.m_sRuntimeEntityId);
		if (HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
			HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(entity);
		m_aRuntimeEntityIds.Insert(mission.m_sRuntimeEntityId);
		m_aRuntimeEntities.Insert(entity);
		RegisterRuntimeEntityState(state, mission, prefab, position, angles);
		Print(string.Format("Partisan mission runtime | spawned prop %1 for %2 at %3", prefab, mission.m_sInstanceId, position));
		return true;
	}

	protected bool ShouldApplyUprightMissionAssetTransform(HST_MissionAssetState asset)
	{
		if (!asset)
			return false;

		return asset.m_sKind == ASSET_KIND_VEHICLE || asset.m_sKind == ASSET_KIND_TARGET || asset.m_sKind == ASSET_KIND_AREA || asset.m_sKind == ASSET_KIND_CARGO;
	}

	protected void RegisterRuntimeEntityState(HST_CampaignState state, HST_ActiveMissionState mission, string prefab, vector position, vector angles)
	{
		if (!state || !mission)
			return;

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(mission.m_sRuntimeEntityId);
		if (!runtimeEntity)
		{
			runtimeEntity = new HST_MissionRuntimeEntityState();
			runtimeEntity.m_sRuntimeEntityId = mission.m_sRuntimeEntityId;
			runtimeEntity.m_sMissionInstanceId = mission.m_sInstanceId;
			state.m_aMissionRuntimeEntities.Insert(runtimeEntity);
		}

		runtimeEntity.m_sKind = mission.m_sRuntimePrimitive;
		runtimeEntity.m_sPrefab = prefab;
		runtimeEntity.m_vPosition = position;
		runtimeEntity.m_vAngles = angles;
		runtimeEntity.m_bSpawned = true;
	}

	protected void RegisterAssetRuntimeEntityState(HST_CampaignState state, HST_MissionAssetState asset, vector position, vector angles)
	{
		if (!state || !asset)
			return;

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtimeEntity)
		{
			runtimeEntity = new HST_MissionRuntimeEntityState();
			runtimeEntity.m_sRuntimeEntityId = asset.m_sEntityId;
			runtimeEntity.m_sMissionInstanceId = asset.m_sMissionInstanceId;
			state.m_aMissionRuntimeEntities.Insert(runtimeEntity);
		}

		runtimeEntity.m_sKind = asset.m_sRole;
		runtimeEntity.m_sPrefab = asset.m_sPrefab;
		runtimeEntity.m_vPosition = position;
		runtimeEntity.m_vAngles = angles;
		runtimeEntity.m_bSpawned = true;
		runtimeEntity.m_bDestroyed = asset.m_bDestroyed;
		runtimeEntity.m_bRecovered = asset.m_bPickedUp || asset.m_bDelivered;
		if (asset.m_sKind == ASSET_KIND_VEHICLE)
			RegisterMissionRuntimeVehicle(state, asset, position, angles);
	}

	protected void RegisterMissionRuntimeVehicle(HST_CampaignState state, HST_MissionAssetState asset, vector position, vector angles)
	{
		if (!state || !asset || asset.m_sEntityId.IsEmpty())
			return;

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
		if (!vehicle)
		{
			vehicle = new HST_RuntimeVehicleState();
			vehicle.m_sVehicleRuntimeId = asset.m_sEntityId;
			state.m_aRuntimeVehicles.Insert(vehicle);
		}

		vehicle.m_sPrefab = asset.m_sPrefab;
		vehicle.m_sDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(asset.m_sPrefab, asset.m_sRole);
		vehicle.m_sRuntimeKind = asset.m_sRole;
		if (asset.m_sRole == ROLE_GUN_SHOP_DELIVERY_VEHICLE)
			vehicle.m_sDisplayName = "Gun Shop Purchases Delivery";
		vehicle.m_vPosition = position;
		vehicle.m_vAngles = angles;
		vehicle.m_bDeleted = asset.m_bDestroyed || asset.m_bDelivered;
	}

	protected void ApplyMissionAssetIdentity(IEntity entity, HST_MissionAssetState asset)
	{
		if (!entity || !asset)
			return;
		if (asset.m_sRole == ROLE_GUN_SHOP_SELLER || asset.m_sRole == ROLE_GUN_SHOP_DELIVERY_DRIVER)
		{
			FactionAffiliationComponent civilianFactionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
			if (civilianFactionComponent)
				civilianFactionComponent.SetAffiliatedFactionByKey("CIV");
			if (asset.m_sRole == ROLE_GUN_SHOP_SELLER)
				StripMissionCaptiveWeapons(entity);
			return;
		}
		if (asset.m_sKind != ASSET_KIND_CAPTIVE && asset.m_sRole != ROLE_CAPTIVE && asset.m_sRole != ROLE_CONVOY_CAPTIVE)
			return;

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (factionComponent)
			factionComponent.SetAffiliatedFactionByKey("CIV");

		int removedWeaponItems = StripMissionCaptiveWeapons(entity);
		if (removedWeaponItems > 0)
			DebugLog(string.Format("disarmed captive projection | asset %1 | removed %2 weapon item(s)", asset.m_sAssetId, removedWeaponItems));
	}

	protected int StripMissionCaptiveWeapons(IEntity entity)
	{
		if (!entity)
			return 0;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(entity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return 0;

		array<IEntity> items = {};
		inventory.GetItems(items, EStoragePurpose.PURPOSE_ANY);
		int removed;
		foreach (IEntity item : items)
		{
			if (!item || !IsMissionCaptiveWeaponItem(item))
				continue;

			if (RemoveMissionCaptiveWeaponItem(inventory, item))
				removed++;
		}

		return removed;
	}

	protected bool IsMissionCaptiveWeaponItem(IEntity item)
	{
		if (!item)
			return false;

		if (BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent)))
			return true;
		if (MagazineComponent.Cast(item.FindComponent(MagazineComponent)) || BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent)))
			return true;
		if (GrenadeMoveComponent.Cast(item.FindComponent(GrenadeMoveComponent)))
			return true;

		string prefab = ResolveEntityPrefabName(item);
		prefab.ToLower();
		if (prefab.Contains("/weapons/") || prefab.Contains("weapon_") || prefab.Contains("rifle") || prefab.Contains("carbine") || prefab.Contains("smg"))
			return true;
		if (prefab.Contains("launcher") || prefab.Contains("m16") || prefab.Contains("ak74") || prefab.Contains("rpg"))
			return true;
		if (prefab.Contains("magazine") || prefab.Contains("mag_") || prefab.Contains("_mag") || prefab.Contains("stanag") || prefab.Contains("pmag") || prefab.Contains("belt"))
			return true;
		if (prefab.Contains("grenade") || prefab.Contains("explosive") || prefab.Contains("mine"))
			return true;

		return false;
	}

	protected bool RemoveMissionCaptiveWeaponItem(SCR_InventoryStorageManagerComponent inventory, IEntity item)
	{
		if (!inventory || !item)
			return false;

		if (inventory.TryDeleteItem(item))
			return true;

		BaseInventoryStorageComponent storage = inventory.FindStorageForItem(item, EStoragePurpose.PURPOSE_ANY);
		if (storage && inventory.TryRemoveItemFromStorage(item, storage))
			return true;

		InventoryItemComponent itemComponent = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (itemComponent && itemComponent.GetParentSlot())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(item);
			return true;
		}

		if (!item.GetParent())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(item);
			return true;
		}

		return false;
	}

	protected void LogMissionCaptiveProjection(IEntity entity, HST_MissionAssetState asset)
	{
		if (!entity || !asset)
			return;
		if (asset.m_sKind != ASSET_KIND_CAPTIVE && asset.m_sRole != ROLE_CAPTIVE && asset.m_sRole != ROLE_CONVOY_CAPTIVE)
			return;

		bool hasActions = ActionsManagerComponent.Cast(entity.FindComponent(ActionsManagerComponent)) != null;
		bool hasRpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent)) != null;
		bool hasMissionAsset = HST_MissionAssetComponent.Cast(entity.FindComponent(HST_MissionAssetComponent)) != null;
		DebugLog(string.Format("captive projection live | asset %1 | entity %2 | origin %3 | actions %4 | rpl %5 | mission asset component %6", asset.m_sAssetId, entity.GetName(), entity.GetOrigin(), hasActions, hasRpl, hasMissionAsset));
	}

	protected bool EnsureRestoredMissionCarrierVehicles(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool changed;
		array<string> restoredCarrierIds = {};
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || !asset.m_bPickedUp || asset.m_bDelivered || asset.m_bDestroyed)
				continue;
			if (!asset.m_bAttachedToCarrier || asset.m_sCarriedByVehicleId.IsEmpty() || IsPlayerCarrierId(asset.m_sCarriedByVehicleId))
				continue;
			if (restoredCarrierIds.Contains(asset.m_sCarriedByVehicleId))
				continue;

			if (EnsureRestoredMissionCarrierVehicle(state, asset))
				changed = true;
			restoredCarrierIds.Insert(asset.m_sCarriedByVehicleId);
		}

		return changed;
	}

	protected bool ShouldRunRestoredMissionCarrierRestorePass(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !state.m_bRestoredFromPersistence || mission.m_sInstanceId.IsEmpty())
			return false;
		if (m_aRestoredMissionCarrierRestoreMissionIds.Contains(mission.m_sInstanceId))
			return false;

		m_aRestoredMissionCarrierRestoreMissionIds.Insert(mission.m_sInstanceId);
		return true;
	}

	protected bool EnsureRestoredMissionCarrierVehicle(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!state || !asset || asset.m_sCarriedByVehicleId.IsEmpty())
			return false;
		if (HasRuntimeEntity(asset.m_sCarriedByVehicleId))
			return false;

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sCarriedByVehicleId);
		if (!runtimeVehicle || runtimeVehicle.m_bDeleted || runtimeVehicle.m_sPrefab.IsEmpty())
			return false;

		vector sourcePosition = runtimeVehicle.m_vPosition;
		if (IsZeroVector(sourcePosition))
			sourcePosition = asset.m_vCurrentPosition;
		if (IsZeroVector(sourcePosition))
			sourcePosition = asset.m_vLastKnownPosition;
		if (IsZeroVector(sourcePosition))
			sourcePosition = asset.m_vSourcePosition;

		vector spawnPosition;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(sourcePosition, spawnPosition, true))
		{
			Print(string.Format("Partisan mission runtime | carrier restore skipped for %1 at %2", asset.m_sCarriedByVehicleId, sourcePosition), LogLevel.WARNING);
			return false;
		}

		vector angles = runtimeVehicle.m_vAngles;
		GenericEntity entity = HST_WorldPositionService.SpawnPrefab(runtimeVehicle.m_sPrefab, spawnPosition, angles);
		if (!entity)
		{
			Print(string.Format("Partisan mission runtime | carrier restore failed for %1 using %2", asset.m_sCarriedByVehicleId, runtimeVehicle.m_sPrefab), LogLevel.WARNING);
			return false;
		}

		HST_WorldPositionService.ApplyUprightEntityTransform(entity, spawnPosition, angles);
		HST_VehicleRootPolicy.ClearVehicleFactionAffiliationRecursive(entity);
		m_aRuntimeEntityIds.Insert(asset.m_sCarriedByVehicleId);
		m_aRuntimeEntities.Insert(entity);
		runtimeVehicle.m_vPosition = spawnPosition;
		runtimeVehicle.m_bDeleted = false;
		Print(string.Format("Partisan mission runtime | restored carrier %1 for mission cargo at %2", asset.m_sCarriedByVehicleId, spawnPosition));
		return true;
	}

	bool HandlePlayerMissionInteraction(HST_CampaignState state, HST_CampaignPreset preset, HST_ArsenalService arsenal, int playerId, IEntity playerEntity, string commandId, string argument, out string result, out string eventType, out string missionInstanceId)
	{
		result = "Partisan mission | no change";
		eventType = "";
		missionInstanceId = "";
		if (!state || !playerEntity)
		{
			result = "Partisan mission | failed: no controlled player entity";
			return false;
		}

		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
		{
			result = "Partisan mission | failed: campaign is not active";
			return false;
		}

		if (!IsLivingPlayerEntity(playerEntity))
		{
			result = "Partisan mission | failed: player is not alive";
			return false;
		}

		vector playerPosition = playerEntity.GetOrigin();
		HST_MissionAssetState asset = ResolveInteractionAsset(state, commandId, argument, playerId, playerPosition);
		if (!asset)
		{
			result = "Partisan mission | failed: no eligible mission asset nearby";
			return false;
		}

		HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset)
			|| HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
		{
			result = "Partisan mission | failed: exact radio interaction requires lifecycle authority";
			return false;
		}
		bool allowPostCompletionConvoyInteraction = IsPostCompletionConvoyInteractionAllowed(state, mission, asset, commandId);
		bool allowExpiredPlayerBoundInteraction = IsExpiredPlayerBoundMissionInteractionAllowed(state, mission, asset, commandId);
		if (!mission || (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE && !allowPostCompletionConvoyInteraction && !allowExpiredPlayerBoundInteraction))
		{
			result = "Partisan mission | failed: mission is no longer active";
			return false;
		}
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
		{
			result = "Partisan mission | failed: exact rescue interaction requires operation authority";
			return false;
		}
		if (AreMissionObjectivesComplete(state, mission) && !allowPostCompletionConvoyInteraction)
		{
			result = "Partisan mission | failed: mission objectives are already complete";
			return false;
		}
		if (IsConvoyPayloadOrCaptiveAsset(asset) && HasLivingConvoyCrewForMission(state, mission))
		{
			result = "Partisan mission | failed: neutralize the convoy crew before recovering this convoy asset";
			return false;
		}

		if (asset.m_bPickedUp && !asset.m_sCarriedByVehicleId.IsEmpty())
		{
			vector carrierPosition;
			if (TryResolveCarrierPosition(state, asset.m_sCarriedByVehicleId, asset.m_vCurrentPosition, carrierPosition))
			{
				asset.m_vCurrentPosition = carrierPosition;
				asset.m_vLastKnownPosition = carrierPosition;
			}
			else if (commandId == "mission_asset_unload" || commandId == "mission_asset_deliver" || commandId == "mission_captive_extract" || commandId == "mission_captive_follow")
			{
				asset.m_vCurrentPosition = playerPosition;
				asset.m_vLastKnownPosition = playerPosition;
			}
		}

		vector validationPosition = ResolveInteractionValidationPosition(asset, commandId);
		vector convoyAccessPosition;
		if (TryResolveConvoyAssetAccessPosition(state, asset, playerPosition, convoyAccessPosition))
		{
			asset.m_vCurrentPosition = convoyAccessPosition;
			asset.m_vLastKnownPosition = convoyAccessPosition;
			validationPosition = convoyAccessPosition;
		}
		float radius = ResolveInteractionRadius(asset, commandId);
		if (DistanceSq2D(playerPosition, validationPosition) > radius * radius)
		{
			if (commandId == "mission_asset_deliver" || (commandId == "mission_captive_extract" && asset.m_bPickedUp))
				result = string.Format("Partisan mission | failed: bring %1 within %2m of the delivery zone", BuildAssetShortLabel(asset), Math.Round(radius));
			else
				result = string.Format("Partisan mission | failed: move within %1m of %2", Math.Round(radius), BuildAssetShortLabel(asset));
			return false;
		}

		bool changed;
		if (commandId == "mission_asset_load")
			changed = ApplyLoadInteraction(state, mission, asset, playerId, playerEntity, playerPosition, result, eventType);
		else if (commandId == "mission_asset_unload")
			changed = ApplyUnloadInteraction(state, mission, asset, playerId, playerPosition, result, eventType);
		else if (commandId == "mission_asset_deliver")
			changed = ApplyDeliverInteraction(state, mission, asset, playerPosition, result, eventType);
		else if (commandId == "mission_captive_extract")
			changed = ApplyCaptiveExtractInteraction(state, mission, asset, playerId, playerEntity, playerPosition, result, eventType);
		else if (commandId == "mission_captive_follow")
			changed = ApplyCaptiveFollowInteraction(state, mission, asset, playerId, playerEntity, playerPosition, result, eventType);
		else if (commandId == "mission_vehicle_capture")
			changed = ApplyVehicleCaptureInteraction(state, mission, asset, arsenal, playerPosition, result, eventType);
		else if (commandId == "mission_asset_sabotage")
			changed = ApplySabotageInteraction(state, mission, asset, playerPosition, result, eventType);
		else
		{
			result = "Partisan mission | failed: unknown mission interaction";
			return false;
		}

		if (!changed)
			return false;

		missionInstanceId = mission.m_sInstanceId;
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE || allowExpiredPlayerBoundInteraction)
		{
			mission.m_sLastRuntimeEventKey = eventType;
			RefreshMissionObjectivesFromAssets(state, mission);
			UpdateMissionCountersFromAssets(state, mission);
		}
		return true;
	}

	bool MarkMissionAssetDestroyedByRuntimeEntity(HST_CampaignState state, string assetId, vector position, out string result, out string eventType, out string missionInstanceId)
	{
		result = "Partisan mission | no change";
		eventType = "";
		missionInstanceId = "";
		if (!state || assetId.IsEmpty())
		{
			result = "Partisan mission | failed: no mission asset supplied";
			return false;
		}

		HST_MissionAssetState asset = state.FindMissionAsset(assetId);
		if (!asset)
		{
			result = "Partisan mission | failed: mission asset not found";
			return false;
		}

		HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset)
			|| HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
		{
			result = "Partisan mission | failed: exact radio destruction requires lifecycle authority";
			return false;
		}
		if (mission && HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
		{
			result = "Partisan mission | failed: exact rescue casualty authority must use the coordinator receipt path";
			return false;
		}
		bool allowPostCompletionConvoyVehicleDestruction = IsPreservedConvoyVehicleAfterCrewElimination(state, mission, asset);
		if (!mission || (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE && !allowPostCompletionConvoyVehicleDestruction))
		{
			result = "Partisan mission | failed: mission is no longer active";
			return false;
		}

		if (asset.m_bDestroyed)
		{
			result = "Partisan mission | already destroyed " + BuildAssetShortLabel(asset);
			return false;
		}

		MarkMissionAssetDestroyed(state, mission, asset, position);
		result = "Partisan mission | destroyed " + BuildAssetShortLabel(asset);
		eventType = "sabotaged";
		if (asset.m_sRole == ROLE_HVT)
			eventType = "neutralized";
		missionInstanceId = mission.m_sInstanceId;
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
		{
			RefreshMissionObjectivesFromAssets(state, mission);
			UpdateMissionCountersFromAssets(state, mission);
		}
		return true;
	}

	bool ApplyMissionAssetExplosiveDamage(HST_CampaignState state, string assetId, vector position, float damage, string sourceLabel, out string result, out string eventType, out string missionInstanceId)
	{
		result = "Partisan mission | no change";
		eventType = "";
		missionInstanceId = "";

		if (!state || assetId.IsEmpty())
		{
			result = "Partisan mission | failed: no mission asset supplied";
			return false;
		}

		HST_MissionAssetState asset = state.FindMissionAsset(assetId);
		if (!asset)
		{
			result = "Partisan mission | failed: mission asset not found";
			return false;
		}

		HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset)
			|| HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
		{
			result = "Partisan mission | failed: exact radio demolition requires lifecycle authority";
			return false;
		}
		if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
		{
			result = "Partisan mission | failed: mission is no longer active";
			return false;
		}

		if (asset.m_bDestroyed)
		{
			result = "Partisan mission | already destroyed " + BuildAssetShortLabel(asset);
			return false;
		}

		if (asset.m_sKind != ASSET_KIND_TARGET && asset.m_sRole != ROLE_DESTROY_TARGET)
		{
			result = "Partisan mission | failed: asset is not a demolition target";
			return false;
		}

		if (damage <= 0.0)
		{
			result = "Partisan mission | ignored non-explosive damage";
			return false;
		}

		if (asset.m_fDemolitionRequiredDamage <= 0.0)
			asset.m_fDemolitionRequiredDamage = ResolveDemolitionRequiredDamage(asset);

		asset.m_fDemolitionDamage = Math.Max(0.0, asset.m_fDemolitionDamage) + damage;
		asset.m_iDemolitionHits++;
		asset.m_sLastDemolitionSource = sourceLabel;
		asset.m_iLastDemolitionSecond = state.m_iElapsedSeconds;

		if (!IsZeroVector(position))
		{
			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
		}

		missionInstanceId = mission.m_sInstanceId;
		if (asset.m_fDemolitionDamage < asset.m_fDemolitionRequiredDamage)
		{
			float remaining = Math.Max(0.0, asset.m_fDemolitionRequiredDamage - asset.m_fDemolitionDamage);
			result = string.Format("Partisan mission | demolition damage %1/%2 on %3 | %4 remaining", Math.Round(asset.m_fDemolitionDamage), Math.Round(asset.m_fDemolitionRequiredDamage), BuildAssetShortLabel(asset), Math.Round(remaining));
			eventType = "demolition_progress";
			return true;
		}

		asset.m_fDemolitionDamage = Math.Max(asset.m_fDemolitionDamage, asset.m_fDemolitionRequiredDamage);
		MarkMissionAssetDestroyed(state, mission, asset, position);
		result = string.Format("Partisan mission | demolished %1 with explosives", BuildAssetShortLabel(asset));
		eventType = "sabotaged";
		RefreshMissionObjectivesFromAssets(state, mission);
		UpdateMissionCountersFromAssets(state, mission);
		return true;
	}

	protected HST_MissionAssetState ResolveInteractionAsset(HST_CampaignState state, string commandId, string argument, int playerId, vector playerPosition)
	{
		if (!argument.IsEmpty())
		{
			HST_MissionAssetState explicitAsset = state.FindMissionAsset(argument);
			if (explicitAsset && IsInteractionAssetEligible(explicitAsset, commandId, playerId))
				return explicitAsset;
		}

		HST_MissionAssetState bestAsset;
		float bestDistanceSq = 999999999.0;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || !IsInteractionAssetEligible(asset, commandId, playerId))
				continue;

			HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
			if (!mission || (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE && !IsPostCompletionConvoyInteractionAllowed(state, mission, asset, commandId) && !IsExpiredPlayerBoundMissionInteractionAllowed(state, mission, asset, commandId)))
				continue;

			vector position = ResolveInteractionValidationPosition(asset, commandId);
			vector convoyAccessPosition;
			if (TryResolveConvoyAssetAccessPosition(state, asset, playerPosition, convoyAccessPosition))
				position = convoyAccessPosition;
			float distanceSq = DistanceSq2D(playerPosition, position);
			if (distanceSq >= bestDistanceSq)
				continue;

			bestDistanceSq = distanceSq;
			bestAsset = asset;
		}

		return bestAsset;
	}

	protected bool IsInteractionAssetEligible(HST_MissionAssetState asset, string commandId, int playerId)
	{
		if (!asset || asset.m_bDelivered)
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
			return false;

		if (commandId == "mission_asset_load")
			return IsTransportableAsset(asset) && !asset.m_bPickedUp && !asset.m_bDestroyed;

		if (commandId == "mission_asset_unload")
			return IsTransportableAsset(asset) && asset.m_bPickedUp && !asset.m_bDestroyed;

		if (commandId == "mission_asset_deliver")
			return IsTransportableAsset(asset) && asset.m_bPickedUp && !asset.m_bDestroyed;

		if (commandId == "mission_captive_extract")
			return asset.m_sKind == ASSET_KIND_CAPTIVE && !asset.m_bDestroyed;

		if (commandId == "mission_captive_follow")
			return asset.m_sKind == ASSET_KIND_CAPTIVE && asset.m_bPickedUp && !asset.m_bDelivered && !asset.m_bDestroyed;

		if (commandId == "mission_vehicle_capture")
		{
			if (asset.m_sRole == ROLE_GUN_SHOP_DELIVERY_VEHICLE)
				return false;

			return asset.m_sKind == ASSET_KIND_VEHICLE && !asset.m_bDestroyed;
		}

		if (commandId == "mission_asset_sabotage")
			return (asset.m_sKind == ASSET_KIND_CHARACTER || asset.m_sKind == ASSET_KIND_VEHICLE) && !asset.m_bDestroyed;

		return false;
	}

	protected bool ApplyLoadInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, int playerId, IEntity playerEntity, vector playerPosition, out string result, out string eventType)
	{
		string carrierId;
		string carrierName;
		vector carrierPosition;
		string failureReason;
		if (!TryResolveMissionAssetCarrier(state, asset, playerId, playerEntity, playerPosition, carrierId, carrierName, carrierPosition, failureReason))
		{
			result = "Partisan mission | failed: " + failureReason;
			return false;
		}

		MarkMissionAssetPickedUp(state, asset);
		asset.m_sCarriedByVehicleId = carrierId;
		asset.m_bAttachedToCarrier = true;
		asset.m_vCurrentPosition = carrierPosition;
		asset.m_vLastKnownPosition = carrierPosition;
		asset.m_sLastInteraction = PHASE_LOADED;
		mission.m_sRuntimePhase = PHASE_LOADED;
		result = string.Format("Partisan mission | loaded %1 into %2", BuildAssetShortLabel(asset), carrierName);
		eventType = "loaded";
		return true;
	}

	protected bool ApplyUnloadInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, int playerId, vector playerPosition, out string result, out string eventType)
	{
		asset.m_bPickedUp = false;
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_vSourcePosition = HST_WorldPositionService.ResolveGroundPosition(playerPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		asset.m_vCurrentPosition = asset.m_vSourcePosition;
		asset.m_vLastKnownPosition = asset.m_vSourcePosition;
		asset.m_sLastInteraction = PHASE_UNLOADED;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_bRecovered = false;
		mission.m_sRuntimePhase = PHASE_UNLOADED;
		result = "Partisan mission | unloaded " + BuildAssetShortLabel(asset);
		eventType = "unloaded";
		return true;
	}

	protected bool ApplyDeliverInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, vector playerPosition, out string result, out string eventType)
	{
		vector deliveryPosition = ResolveDeliveryPosition(asset, playerPosition);
		if (DistanceSq2D(playerPosition, deliveryPosition) > DEFAULT_DELIVERY_RADIUS_METERS * DEFAULT_DELIVERY_RADIUS_METERS)
		{
			result = "Partisan mission | failed: bring " + BuildAssetShortLabel(asset) + " to the delivery zone";
			return false;
		}

		StopCaptiveFollowController(asset);
		MarkMissionAssetDelivered(state, asset, deliveryPosition);
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_sLastInteraction = PHASE_DELIVERED;
		mission.m_sRuntimePhase = PHASE_DELIVERED;
		result = "Partisan mission | delivered " + BuildAssetShortLabel(asset);
		eventType = "delivered";
		return true;
	}

	protected bool ApplyCaptiveExtractInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, int playerId, IEntity playerEntity, vector playerPosition, out string result, out string eventType)
	{
		if (!asset.m_bPickedUp)
		{
			asset.m_bPickedUp = true;
			asset.m_sCarriedByVehicleId = "";
			asset.m_bAttachedToCarrier = false;
			IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
			vector captivePosition = playerPosition;
			if (captiveEntity)
				captivePosition = captiveEntity.GetOrigin();
			asset.m_vCurrentPosition = captivePosition;
			asset.m_vLastKnownPosition = captivePosition;
			asset.m_sLastInteraction = PHASE_FREED;
			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity)
			{
				runtimeEntity.m_bRecovered = false;
				runtimeEntity.m_vPosition = captivePosition;
			}
			mission.m_sRuntimePhase = PHASE_FREED;
			result = "Partisan mission | freed " + BuildAssetShortLabel(asset) + "; order them to follow";
			eventType = "freed";
			return true;
		}

		if (!asset.m_bAttachedToCarrier || asset.m_sCarriedByVehicleId.IsEmpty())
		{
			result = "Partisan mission | failed: order " + BuildAssetShortLabel(asset) + " to follow before extraction";
			return false;
		}

		if (IsEntityInVehicle(playerEntity))
		{
			result = "Partisan mission | failed: dismount before extracting captives at HQ";
			return false;
		}

		return ApplyDeliverInteraction(state, mission, asset, playerPosition, result, eventType);
	}

	protected bool ApplyCaptiveFollowInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, int playerId, IEntity playerEntity, vector playerPosition, out string result, out string eventType)
	{
		if (!asset.m_bPickedUp)
		{
			result = "Partisan mission | failed: free " + BuildAssetShortLabel(asset) + " first";
			return false;
		}
		if (asset.m_bDelivered || asset.m_bDestroyed)
		{
			result = "Partisan mission | failed: captive is no longer available";
			return false;
		}

		int followed;
		foreach (HST_MissionAssetState candidate : state.m_aMissionAssets)
		{
			if (!candidate || candidate.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (candidate.m_sKind != ASSET_KIND_CAPTIVE || !candidate.m_bPickedUp || candidate.m_bDelivered || candidate.m_bDestroyed)
				continue;
			if (DistanceSq2D(playerPosition, ResolveInteractionValidationPosition(candidate, "mission_captive_follow")) > ResolveInteractionRadius(candidate, "mission_captive_follow") * ResolveInteractionRadius(candidate, "mission_captive_follow"))
				continue;

			MarkCaptiveFollowing(state, candidate, playerId, playerEntity);
			followed++;
		}

		if (followed <= 0)
		{
			MarkCaptiveFollowing(state, asset, playerId, playerEntity);
			followed = 1;
		}

		mission.m_sRuntimePhase = PHASE_EXTRACTING;
		if (followed == 1)
			result = "Partisan mission | " + BuildAssetShortLabel(asset) + " is following you";
		else
			result = string.Format("Partisan mission | %1 POWs are following you", followed);
		eventType = "following";
		return true;
	}

	protected void MarkCaptiveFollowing(HST_CampaignState state, HST_MissionAssetState asset, int playerId, IEntity playerEntity)
	{
		if (!state || !asset)
			return;

		vector position = asset.m_vCurrentPosition;
		IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
		if (captiveEntity)
			position = captiveEntity.GetOrigin();

		asset.m_sCarriedByVehicleId = BuildPlayerCarrierId(playerId);
		asset.m_bAttachedToCarrier = true;
		asset.m_vCurrentPosition = position;
		asset.m_vLastKnownPosition = position;
		asset.m_sLastInteraction = PHASE_FOLLOWING;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bRecovered = false;
			runtimeEntity.m_vPosition = position;
		}

		StartCaptiveFollowController(asset, playerEntity);
	}

	protected bool StopCaptiveFollowing(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, vector stopPosition)
	{
		if (!state || !asset)
			return false;

		bool changed;
		IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
		StopCaptiveFollowController(asset);
		IEntity captiveVehicle = ResolveEntityVehicle(captiveEntity);
		if (captiveVehicle)
		{
			stopPosition = captiveVehicle.GetOrigin();
			DeleteRuntimeEntity(asset.m_sEntityId);
			captiveEntity = null;
			changed = true;
		}

		if (IsZeroVector(stopPosition))
			stopPosition = ResolveCaptiveStopPosition(asset, captiveEntity);
		if (IsZeroVector(stopPosition))
			stopPosition = asset.m_vCurrentPosition;

		if (asset.m_bAttachedToCarrier)
		{
			asset.m_bAttachedToCarrier = false;
			changed = true;
		}
		if (!asset.m_sCarriedByVehicleId.IsEmpty())
		{
			asset.m_sCarriedByVehicleId = "";
			changed = true;
		}
		if (asset.m_sLastInteraction != PHASE_FREED)
		{
			asset.m_sLastInteraction = PHASE_FREED;
			changed = true;
		}
		if (!IsZeroVector(stopPosition) && DistanceSq2D(asset.m_vCurrentPosition, stopPosition) > 0.5)
		{
			asset.m_vCurrentPosition = stopPosition;
			asset.m_vLastKnownPosition = stopPosition;
			changed = true;
		}

		if (!captiveEntity && mission && !asset.m_bDelivered && !asset.m_bDestroyed && !IsZeroVector(stopPosition))
		{
			captiveEntity = SpawnCaptiveFollowerProjection(state, mission, asset, stopPosition, "0 0 0");
			if (captiveEntity)
				changed = true;
		}

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			if (!IsZeroVector(stopPosition))
				runtimeEntity.m_vPosition = stopPosition;
			runtimeEntity.m_bRecovered = false;
			runtimeEntity.m_bSpawned = captiveEntity != null;
		}

		return changed;
	}

	protected vector ResolveCaptiveStopPosition(HST_MissionAssetState asset, IEntity captiveEntity)
	{
		if (captiveEntity)
		{
			IEntity captiveVehicle = ResolveEntityVehicle(captiveEntity);
			if (captiveVehicle)
				return captiveVehicle.GetOrigin();

			return captiveEntity.GetOrigin();
		}

		if (asset && !IsZeroVector(asset.m_vLastKnownPosition))
			return asset.m_vLastKnownPosition;
		if (asset)
			return asset.m_vCurrentPosition;

		return "0 0 0";
	}

	protected bool StartCaptiveFollowController(HST_MissionAssetState asset, IEntity targetEntity)
	{
		if (!asset || !targetEntity)
			return false;

		IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
		if (!captiveEntity)
			return false;

		HST_MissionCaptiveFollowComponent follow = HST_MissionCaptiveFollowComponent.Cast(captiveEntity.FindComponent(HST_MissionCaptiveFollowComponent));
		if (!follow)
			return false;

		follow.StartFollowing(targetEntity);
		return true;
	}

	protected void StopCaptiveFollowController(HST_MissionAssetState asset)
	{
		if (!asset)
			return;

		IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
		if (!captiveEntity)
			return;

		HST_MissionCaptiveFollowComponent follow = HST_MissionCaptiveFollowComponent.Cast(captiveEntity.FindComponent(HST_MissionCaptiveFollowComponent));
		if (follow)
			follow.StopFollowing();
	}

	protected void StopCaptiveFollowControllerForEntity(IEntity captiveEntity)
	{
		if (!captiveEntity)
			return;

		HST_MissionCaptiveFollowComponent follow
			= HST_MissionCaptiveFollowComponent.Cast(
				captiveEntity.FindComponent(HST_MissionCaptiveFollowComponent));
		if (follow)
			follow.StopFollowing();
	}

	protected bool ApplyVehicleCaptureInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, HST_ArsenalService arsenal, vector playerPosition, out string result, out string eventType)
	{
		if (asset.m_sRole == ROLE_CONVOY_VEHICLE && HasLivingConvoyCrewForVehicle(state, mission, asset))
		{
			result = "Partisan mission | failed: neutralize the convoy crew before capturing this vehicle";
			return false;
		}
		if (HST_MissionConvoyOperationService.IsExactMission(mission) && asset.m_sRole == ROLE_CONVOY_VEHICLE)
			return ApplyExactMissionConvoyVehicleCaptureInteraction(state, mission, asset, arsenal, playerPosition, result, eventType);

		asset.m_bPickedUp = true;
		asset.m_bDelivered = true;
		asset.m_bAttachedToCarrier = false;
		asset.m_vCurrentPosition = playerPosition;
		asset.m_vLastKnownPosition = playerPosition;
		asset.m_sLastInteraction = PHASE_CAPTURED;
		DeleteRuntimeEntity(asset.m_sEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bRecovered = true;
			runtimeEntity.m_vPosition = playerPosition;
		}

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
		if (runtimeVehicle)
		{
			runtimeVehicle.m_bDeleted = true;
			runtimeVehicle.m_vPosition = playerPosition;
		}

		if (arsenal && !asset.m_sPrefab.IsEmpty())
		{
			HST_GarageVehicleState vehicle = new HST_GarageVehicleState();
			vehicle.m_sVehicleId = "mission_vehicle_" + asset.m_sAssetId;
			vehicle.m_sPrefab = asset.m_sPrefab;
			vehicle.m_sDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(asset.m_sPrefab);
			vehicle.m_sSourceZoneId = mission.m_sTargetZoneId;
			if (runtimeVehicle)
				vehicle.m_sSourceFactionKey = runtimeVehicle.m_sFactionKey;
			vehicle.m_vPosition = playerPosition;
			vehicle.m_vAngles = "0 0 0";
			vehicle.m_fFuel = 1.0;
			vehicle.m_sDamageState = "captured";
			vehicle.m_bUnlocked = true;
			HST_VehicleCapabilityPolicy.CopyRuntimeCoverStateToGarage(runtimeVehicle, vehicle);
			arsenal.StoreVehicle(state, vehicle);
		}

		if (ShouldKeepConvoyContactPhase(mission, asset))
			mission.m_sRuntimePhase = PHASE_CONVOY_CONTACT;
		else if (mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
		{
			mission.m_sRuntimePhase = PHASE_CAPTURED;
		}
		result = "Partisan mission | captured " + BuildAssetShortLabel(asset);
		eventType = "captured";
		return true;
	}

	protected bool ApplyExactMissionConvoyVehicleCaptureInteraction(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		HST_ArsenalService arsenal,
		vector playerPosition,
		out string result,
		out string eventType)
	{
		if (!state || !mission || !asset || !arsenal || !m_PhysicalWar || asset.m_sPrefab.IsEmpty())
		{
			result = "Partisan mission | failed: exact convoy capture services are unavailable";
			return false;
		}
		string garageVehicleId = "mission_vehicle_" + asset.m_sAssetId;
		if (state.FindGarageVehicle(garageVehicleId))
		{
			result = "Partisan mission | failed: captured convoy vehicle already exists in the garage";
			return false;
		}
		HST_MissionAssetState carrierCargo;
		vector carrierCargoGroundPosition;
		string cargoHandoffReason;
		if (!ValidateExactMissionConvoyCarrierCargoForVehicleHandoff(state, mission, asset, carrierCargo, carrierCargoGroundPosition, cargoHandoffReason))
		{
			result = "Partisan mission | failed: " + cargoHandoffReason;
			return false;
		}

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
		HST_GarageVehicleState vehicle = new HST_GarageVehicleState();
		vehicle.m_sVehicleId = garageVehicleId;
		vehicle.m_sPrefab = asset.m_sPrefab;
		vehicle.m_sDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(asset.m_sPrefab);
		vehicle.m_sSourceZoneId = mission.m_sTargetZoneId;
		if (runtimeVehicle)
			vehicle.m_sSourceFactionKey = runtimeVehicle.m_sFactionKey;
		vehicle.m_vPosition = playerPosition;
		vehicle.m_vAngles = "0 0 0";
		vehicle.m_fFuel = 1.0;
		vehicle.m_sDamageState = "captured";
		vehicle.m_bUnlocked = true;
		HST_VehicleCapabilityPolicy.CopyRuntimeCoverStateToGarage(runtimeVehicle, vehicle);
		if (!arsenal.StoreVehicle(state, vehicle))
		{
			result = "Partisan mission | failed: captured convoy vehicle could not be reserved in the garage";
			return false;
		}

		IEntity handedOffVehicle;
		string handoffReason;
		if (!m_PhysicalWar.TryHandoffExactMissionConvoyVehicleCapture(state, mission, asset.m_sAssetId, handedOffVehicle, handoffReason))
		{
			RemoveGarageVehicleRecord(state, garageVehicleId);
			result = "Partisan mission | failed: " + handoffReason;
			return false;
		}

		vehicle.m_vPosition = asset.m_vCurrentPosition;
		if (handedOffVehicle)
			vehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(handedOffVehicle.GetYawPitchRoll());
		HST_ConvoyElementState element = state.FindConvoyElement(asset.m_sConvoyElementId);
		if (element)
			vehicle.m_fFuel = element.m_fFuelFraction;
		if (runtimeVehicle)
		{
			runtimeVehicle.m_bDeleted = true;
			runtimeVehicle.m_vPosition = asset.m_vCurrentPosition;
		}
		bool carrierCargoDetached = CommitExactMissionConvoyCarrierCargoForVehicleHandoff(state, carrierCargo, carrierCargoGroundPosition);
		if (carrierCargoDetached)
		{
			TickExactMissionConvoyCargoProjections(state, m_PhysicalWar);
			if (!IsExactMissionConvoyCargoProjectionReady(state, mission, m_PhysicalWar))
				Print(string.Format("Partisan mission runtime | exact convoy carrier cargo remains durably detached for projection retry after vehicle capture %1", asset.m_sAssetId), LogLevel.WARNING);
		}
		if (handedOffVehicle)
			SCR_EntityHelper.DeleteEntityAndChildren(handedOffVehicle);

		if (ShouldKeepConvoyContactPhase(mission, asset))
			mission.m_sRuntimePhase = PHASE_CONVOY_CONTACT;
		else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
			mission.m_sRuntimePhase = PHASE_CAPTURED;
		result = "Partisan mission | captured " + BuildAssetShortLabel(asset);
		eventType = "captured";
		return true;
	}

	protected bool ValidateExactMissionConvoyCarrierCargoForVehicleHandoff(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState vehicleAsset,
		out HST_MissionAssetState cargo,
		out vector groundPosition,
		out string reason)
	{
		cargo = null;
		groundPosition = "0 0 0";
		reason = "";
		if (!state || !mission || !vehicleAsset || vehicleAsset.m_sManifestSlotId.IsEmpty())
		{
			reason = "exact convoy carrier-cargo handoff authority is missing";
			return false;
		}

		int cargoClaimants;
		foreach (HST_MissionAssetState candidate : state.m_aMissionAssets)
		{
			if (!candidate || candidate.m_sMissionInstanceId != mission.m_sInstanceId
				|| candidate.m_sAssignedVehicleSlotId != vehicleAsset.m_sManifestSlotId
				|| (candidate.m_sRole != ROLE_CONVOY_PAYLOAD && candidate.m_sRole != ROLE_CONVOY_CAPTIVE))
				continue;
			cargo = candidate;
			cargoClaimants++;
		}
		if (cargoClaimants > 1)
		{
			reason = "exact convoy carrier-cargo handoff found ambiguous frozen cargo authority";
			return false;
		}
		if (!cargo || IsExactMissionConvoyCargoProjectionPlayerBoundOrResolved(cargo))
			return true;

		IEntity carrierEntity = m_PhysicalWar.GetExactMissionConvoyVehicleRuntimeEntity(state, mission, vehicleAsset.m_sManifestSlotId);
		if (!carrierEntity)
		{
			reason = "exact convoy carrier-cargo handoff has no authoritative physical carrier";
			return false;
		}
		groundPosition = carrierEntity.GetOrigin();
		if (IsZeroVector(groundPosition))
		{
			reason = "exact convoy carrier-cargo handoff has no valid durable ground position";
			return false;
		}

		IEntity cargoEntity = GetRuntimeEntity(cargo.m_sEntityId);
		if (cargoEntity)
		{
			IEntity occupiedVehicle = ResolveEntityVehicle(cargoEntity);
			IEntity parent = cargoEntity.GetParent();
			if ((occupiedVehicle && occupiedVehicle != carrierEntity)
				|| (!occupiedVehicle && parent && parent != carrierEntity))
			{
				reason = "exact convoy carrier-cargo handoff refused foreign runtime ownership";
				return false;
			}
		}
		return true;
	}

	protected bool CommitExactMissionConvoyCarrierCargoForVehicleHandoff(
		HST_CampaignState state,
		HST_MissionAssetState cargo,
		vector groundPosition)
	{
		if (!state || !cargo || IsZeroVector(groundPosition))
			return false;

		IEntity cargoEntity = GetRuntimeEntity(cargo.m_sEntityId);
		if (cargoEntity)
			DeleteRuntimeEntity(cargo.m_sEntityId);

		cargo.m_bSpawned = false;
		cargo.m_bAttachedToCarrier = false;
		cargo.m_sCarriedByVehicleId = "";
		cargo.m_vCurrentPosition = groundPosition;
		cargo.m_vLastKnownPosition = groundPosition;
		HST_MissionRuntimeEntityState runtimeCargo = state.FindMissionRuntimeEntity(cargo.m_sEntityId);
		if (runtimeCargo)
		{
			runtimeCargo.m_bSpawned = false;
			runtimeCargo.m_bDestroyed = false;
			runtimeCargo.m_bRecovered = false;
			runtimeCargo.m_vPosition = groundPosition;
		}
		return true;
	}

	protected bool RemoveGarageVehicleRecord(HST_CampaignState state, string vehicleId)
	{
		if (!state || vehicleId.IsEmpty())
			return false;
		for (int index = state.m_aGarageVehicles.Count() - 1; index >= 0; index--)
		{
			HST_GarageVehicleState vehicle = state.m_aGarageVehicles[index];
			if (!vehicle || vehicle.m_sVehicleId != vehicleId)
				continue;
			state.m_aGarageVehicles.Remove(index);
			return true;
		}
		return false;
	}

	protected bool IsPostCompletionConvoyInteractionAllowed(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, string commandId)
	{
		if (!IsPreservedConvoyAssetAfterCrewElimination(state, mission, asset))
			return false;

		if (asset.m_sRole == ROLE_CONVOY_VEHICLE)
			return commandId == "mission_vehicle_capture";
		if (asset.m_sRole == ROLE_CONVOY_PAYLOAD)
			return commandId == "mission_asset_load" || commandId == "mission_asset_deliver" || commandId == "mission_asset_unload";
		if (asset.m_sRole == ROLE_CONVOY_CAPTIVE)
			return commandId == "mission_captive_extract" || commandId == "mission_captive_follow";

		return false;
	}

	protected bool IsPreservedConvoyVehicleAfterCrewElimination(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		return IsPreservedConvoyAssetAfterCrewElimination(state, mission, asset) && asset.m_sRole == ROLE_CONVOY_VEHICLE;
	}

	protected bool IsPreservedConvoyAssetAfterCrewElimination(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset)
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			return false;
		if (mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT || !HasConvoyCrewEliminatedForPostCompletion(state, mission))
			return false;
		if (asset.m_sMissionInstanceId != mission.m_sInstanceId)
			return false;

		return !asset.m_bDestroyed && !asset.m_bDelivered;
	}

	protected bool HasConvoyCrewEliminatedForPostCompletion(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;
		if (mission.m_bConvoyCrewEliminatedOutcomeApplied)
			return true;

		return IsConvoyCrewEliminationCompletionEvent(mission.m_sLastRuntimeEventKey) && HasConvoyEliminatedCrewEvidence(state, mission);
	}

	protected bool HasConvoyEliminatedCrewEvidence(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sInstanceId.IsEmpty())
			return false;

		string groupPrefix = "mission_convoy_" + mission.m_sInstanceId + "_";
		int convoyGroups;
		int eliminatedGroups;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsConvoyGroupOwnedByMission(activeGroup, mission, groupPrefix))
				continue;

			convoyGroups++;
			if (activeGroup.m_sRuntimeStatus != "convoy_eliminated" && activeGroup.m_sRuntimeStatus != "eliminated")
				return false;
			if (!HasConvoyCrewLiveHistory(activeGroup))
				return false;

			eliminatedGroups++;
		}

		return convoyGroups > 0 && eliminatedGroups == convoyGroups;
	}

	protected bool HasConvoyCrewLiveHistory(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;
		if (activeGroup.m_bEverHadLivingCrew)
			return true;
		if (activeGroup.m_iMaxObservedCrewAlive > 0)
			return true;

		return false;
	}

	protected bool IsConvoyPayloadOrCaptiveAsset(HST_MissionAssetState asset)
	{
		if (!asset)
			return false;

		return asset.m_sRole == ROLE_CONVOY_PAYLOAD || asset.m_sRole == ROLE_CONVOY_CAPTIVE;
	}

	protected bool IsConvoyCrewEliminationCompletionEvent(string eventKey)
	{
		return eventKey == PHASE_CONVOY_ELIMINATED || eventKey == "convoy_complete" || eventKey == "convoy_secured_sent";
	}

	protected bool ShouldKeepConvoyContactPhase(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT || asset.m_sRole != ROLE_CONVOY_VEHICLE)
			return false;
		if (mission.m_sRuntimePhase == PHASE_FAILED || mission.m_sRuntimePhase == PHASE_CONVOY_ELIMINATED)
			return false;

		return true;
	}

	protected bool HasLivingConvoyCrewForVehicle(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState vehicleAsset)
	{
		if (!state || !mission || !vehicleAsset || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		int vehicleIndex = ResolveMissionConvoyVehicleIndex(state, mission, vehicleAsset);
		if (vehicleIndex < 0)
			return false;

		string groupId = string.Format("mission_convoy_%1_%2", mission.m_sInstanceId, vehicleIndex);
		HST_ActiveGroupState activeGroup = state.FindActiveGroup(groupId);
		if (!activeGroup)
			return false;

		if (activeGroup.m_sRuntimeStatus == "convoy_eliminated" || activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed")
			return false;

		return activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iLastSeenAliveCount > 0;
	}

	protected bool HasLivingConvoyCrewForMission(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		string groupPrefix = string.Format("%1%2_", MISSION_CONVOY_GROUP_PREFIX, mission.m_sInstanceId);
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!IsConvoyGroupOwnedByMission(activeGroup, mission, groupPrefix))
				continue;
			if (activeGroup.m_sRuntimeStatus == "convoy_eliminated" || activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed")
				continue;
			if (activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iLastSeenAliveCount > 0)
				return true;
		}

		return false;
	}

	protected bool IsConvoyGroupOwnedByMission(HST_ActiveGroupState activeGroup, HST_ActiveMissionState mission, string groupPrefix)
	{
		if (!activeGroup || !mission || groupPrefix.IsEmpty() || !activeGroup.m_sGroupId.StartsWith(groupPrefix))
			return false;
		if (!activeGroup.m_sMissionInstanceId.IsEmpty() && activeGroup.m_sMissionInstanceId != mission.m_sInstanceId)
			return false;
		if (!HST_MissionConvoyOperationService.IsExactMission(mission))
			return true;
		return activeGroup.m_sMissionInstanceId == mission.m_sInstanceId
			&& activeGroup.m_sOperationId == mission.m_sOperationId
			&& !activeGroup.m_sConvoyElementId.IsEmpty();
	}

	protected int ResolveMissionConvoyVehicleIndex(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState vehicleAsset)
	{
		if (!state || !mission || !vehicleAsset)
			return -1;

		int index;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != ROLE_CONVOY_VEHICLE)
				continue;
			if (asset == vehicleAsset || asset.m_sAssetId == vehicleAsset.m_sAssetId)
				return index;

			index++;
		}

		return -1;
	}

	protected bool ApplySabotageInteraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, vector playerPosition, out string result, out string eventType)
	{
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission)
			|| HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
		{
			result = "Partisan mission | failed: exact radio sabotage requires lifecycle authority";
			eventType = "";
			return false;
		}

		MarkMissionAssetDestroyed(state, mission, asset, playerPosition);
		if (ShouldKeepConvoyContactPhase(mission, asset))
			mission.m_sRuntimePhase = PHASE_CONVOY_CONTACT;
		else
		{
			mission.m_sRuntimePhase = PHASE_DESTROYED;
		}
		result = "Partisan mission | sabotaged " + BuildAssetShortLabel(asset);
		eventType = "sabotaged";
		return true;
	}

	protected void MarkMissionAssetDestroyed(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, vector position)
	{
		if (!state || !asset)
			return;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission)
			|| HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
			return;

		asset.m_bDestroyed = true;
		asset.m_bAlive = false;
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		if (!IsZeroVector(position))
		{
			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
		}
		asset.m_sLastInteraction = PHASE_DESTROYED;
		DeleteRuntimeEntity(asset.m_sEntityId);
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

		if (mission)
		{
			if (ShouldKeepConvoyContactPhase(mission, asset))
				mission.m_sRuntimePhase = PHASE_CONVOY_CONTACT;
			else if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				mission.m_sRuntimePhase = PHASE_DESTROYED;
		}
	}

	protected vector ResolveInteractionValidationPosition(HST_MissionAssetState asset, string commandId)
	{
		if (!asset)
			return "0 0 0";

		if (commandId == "mission_asset_deliver")
			return ResolveDeliveryPosition(asset, asset.m_vCurrentPosition);

		if (commandId == "mission_captive_extract" && asset.m_bPickedUp)
			return ResolveDeliveryPosition(asset, asset.m_vCurrentPosition);

		if (asset.m_bPickedUp && !IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;

		if (!IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;

		return asset.m_vSourcePosition;
	}

	protected bool TryResolveConvoyAssetAccessPosition(HST_CampaignState state, HST_MissionAssetState asset, vector playerPosition, out vector accessPosition)
	{
		accessPosition = "0 0 0";
		if (!state || !IsConvoyPayloadOrCaptiveAsset(asset) || asset.m_bPickedUp || asset.m_bDelivered || asset.m_bDestroyed)
			return false;

		HST_ActiveMissionState mission = state.FindActiveMission(asset.m_sMissionInstanceId);
		if (!mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;
		if (HasLivingConvoyCrewForMission(state, mission))
			return false;
		if (mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION)
		{
			if (asset.m_sAssignedVehicleSlotId.IsEmpty())
				return false;
			foreach (HST_MissionAssetState assignedVehicle : state.m_aMissionAssets)
			{
				if (!assignedVehicle || assignedVehicle.m_sMissionInstanceId != asset.m_sMissionInstanceId
					|| assignedVehicle.m_sRole != ROLE_CONVOY_VEHICLE
					|| assignedVehicle.m_sManifestSlotId != asset.m_sAssignedVehicleSlotId)
					continue;
				accessPosition = assignedVehicle.m_vCurrentPosition;
				if (IsZeroVector(accessPosition))
					accessPosition = assignedVehicle.m_vLastKnownPosition;
				if (IsZeroVector(accessPosition))
					accessPosition = assignedVehicle.m_vSourcePosition;
				return !IsZeroVector(accessPosition);
			}
			return false;
		}

		bool found;
		float bestDistanceSq = 999999999.0;
		foreach (HST_MissionAssetState vehicleAsset : state.m_aMissionAssets)
		{
			if (!vehicleAsset || vehicleAsset.m_sMissionInstanceId != asset.m_sMissionInstanceId || vehicleAsset.m_sRole != ROLE_CONVOY_VEHICLE)
				continue;
			if (vehicleAsset.m_bDestroyed || vehicleAsset.m_bDelivered)
				continue;

			vector vehiclePosition = vehicleAsset.m_vCurrentPosition;
			if (IsZeroVector(vehiclePosition))
				vehiclePosition = vehicleAsset.m_vLastKnownPosition;
			if (IsZeroVector(vehiclePosition))
				vehiclePosition = vehicleAsset.m_vSourcePosition;
			if (IsZeroVector(vehiclePosition))
				continue;

			float distanceSq = DistanceSq2D(playerPosition, vehiclePosition);
			if (found && distanceSq >= bestDistanceSq)
				continue;

			found = true;
			bestDistanceSq = distanceSq;
			accessPosition = vehiclePosition;
		}

		return found;
	}

	protected vector ResolveDeliveryPosition(HST_MissionAssetState asset, vector fallbackPosition)
	{
		if (asset && !IsZeroVector(asset.m_vTargetPosition))
			return asset.m_vTargetPosition;

		return fallbackPosition;
	}

	protected float ResolveInteractionRadius(HST_MissionAssetState asset, string commandId)
	{
		if (commandId == "mission_asset_deliver" || (commandId == "mission_captive_extract" && asset && asset.m_bPickedUp))
			return DEFAULT_DELIVERY_RADIUS_METERS;

		if (asset && asset.m_iInteractionRadiusMeters > 0)
			return asset.m_iInteractionRadiusMeters;

		return DEFAULT_ASSET_INTERACTION_RADIUS_METERS;
	}

	protected bool IsTransportableAsset(HST_MissionAssetState asset)
	{
		if (!asset)
			return false;

		return asset.m_sKind == ASSET_KIND_CARGO || asset.m_sKind == ASSET_KIND_CAPTIVE;
	}

	protected bool CanCarrierAcceptAsset(HST_CampaignState state, int playerId, HST_MissionAssetState asset)
	{
		return CanCarrierIdAcceptAsset(state, BuildPlayerCarrierId(playerId), asset);
	}

	protected bool CanCarrierIdAcceptAsset(HST_CampaignState state, string carrierId, HST_MissionAssetState asset)
	{
		if (!state || carrierId.IsEmpty() || !asset)
			return false;

		int usedCapacity;
		foreach (HST_MissionAssetState carriedAsset : state.m_aMissionAssets)
		{
			if (!carriedAsset || carriedAsset.m_bDelivered || carriedAsset.m_bDestroyed || carriedAsset.m_sCarriedByVehicleId != carrierId)
				continue;

			usedCapacity += Math.Max(1, carriedAsset.m_iCargoCapacityCost);
		}

		return usedCapacity + Math.Max(1, asset.m_iCargoCapacityCost) <= DEFAULT_MISSION_CARRIER_CAPACITY;
	}

	protected bool TryResolveMissionAssetCarrier(HST_CampaignState state, HST_MissionAssetState asset, int playerId, IEntity playerEntity, vector playerPosition, out string carrierId, out string carrierName, out vector carrierPosition, out string failureReason)
	{
		carrierId = "";
		carrierName = "";
		carrierPosition = playerPosition;
		failureReason = "";
		if (!asset)
		{
			failureReason = "mission asset missing";
			return false;
		}

		IEntity vehicle = FindNearestMissionVehicleCarrier(state, playerEntity, VEHICLE_CARRIER_RADIUS_METERS, carrierId, carrierName);
		if (vehicle)
		{
			carrierPosition = vehicle.GetOrigin();
			EnsureMissionCarrierVehicleRecord(state, vehicle, carrierId, carrierName);
			return true;
		}

		if (TryResolveMissionAssetRuntimeCarrierRecord(state, asset, playerPosition, VEHICLE_CARRIER_RADIUS_METERS, carrierId, carrierName, carrierPosition))
			return true;

		failureReason = "no vehicle nearby";
		return false;
	}

	protected bool TryResolveMissionAssetRuntimeCarrierRecord(HST_CampaignState state, HST_MissionAssetState asset, vector playerPosition, float radiusMeters, out string carrierId, out string carrierName, out vector carrierPosition)
	{
		carrierId = "";
		carrierName = "";
		carrierPosition = playerPosition;
		if (!state || !asset)
			return false;

		float bestDistanceSq = radiusMeters * radiusMeters;
		bool found;
		foreach (HST_RuntimeVehicleState runtimeVehicle : state.m_aRuntimeVehicles)
		{
			if (!runtimeVehicle || runtimeVehicle.m_bDeleted || runtimeVehicle.m_sVehicleRuntimeId.IsEmpty())
				continue;
			if (runtimeVehicle.m_sRuntimeKind != "mission_carrier")
				continue;
			if (IsZeroVector(runtimeVehicle.m_vPosition))
				continue;
			if (!CanCarrierIdAcceptAsset(state, runtimeVehicle.m_sVehicleRuntimeId, asset))
				continue;

			float distanceSq = DistanceSq2D(playerPosition, runtimeVehicle.m_vPosition);
			if (found && distanceSq >= bestDistanceSq)
				continue;
			if (distanceSq > radiusMeters * radiusMeters)
				continue;

			found = true;
			bestDistanceSq = distanceSq;
			carrierId = runtimeVehicle.m_sVehicleRuntimeId;
			carrierName = runtimeVehicle.m_sDisplayName;
			carrierPosition = runtimeVehicle.m_vPosition;
		}

		if (!found)
			return false;
		if (carrierName.IsEmpty())
			carrierName = "vehicle";
		return true;
	}

	protected IEntity FindNearestMissionVehicleCarrier(HST_CampaignState state, IEntity playerEntity, float radiusMeters, out string carrierId, out string carrierName)
	{
		carrierId = "";
		carrierName = "";
		if (!playerEntity)
			return null;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		m_aMissionVehicleScanEntities.Clear();
		world.QueryEntitiesBySphere(playerEntity.GetOrigin(), radiusMeters, AddMissionVehicleScanCandidate, null, EQueryEntitiesFlags.ALL);

		float bestDistanceSq = 999999999.0;
		IEntity bestVehicle;
		string bestId;
		string bestName;
		foreach (IEntity candidate : m_aMissionVehicleScanEntities)
		{
			if (!candidate || candidate == playerEntity)
				continue;

			string prefab;
			IEntity root = ResolveMissionVehicleRoot(candidate, prefab);
			if (!root || root == playerEntity)
				continue;

			float distanceSq = DistanceSq2D(playerEntity.GetOrigin(), root.GetOrigin());
			if (distanceSq >= bestDistanceSq)
				continue;

			bestDistanceSq = distanceSq;
			bestVehicle = root;
			bestId = ResolveEntityRuntimeId(root);
			bestName = HST_DisplayNameService.ResolveVehicleDisplayName(prefab, root.GetName());
		}

		if (!bestVehicle || bestId.IsEmpty())
			return null;

		carrierId = bestId;
		carrierName = bestName;
		if (carrierName.IsEmpty())
			carrierName = "vehicle";
		return bestVehicle;
	}

	protected bool AddMissionVehicleScanCandidate(IEntity entity)
	{
		if (entity && m_aMissionVehicleScanEntities.Count() < MAX_MISSION_VEHICLE_SCAN_ENTITIES)
			m_aMissionVehicleScanEntities.Insert(entity);

		return m_aMissionVehicleScanEntities.Count() < MAX_MISSION_VEHICLE_SCAN_ENTITIES;
	}

	protected IEntity ResolveMissionVehicleRoot(IEntity entity, out string prefab)
	{
		prefab = "";
		IEntity cursor = entity;
		while (cursor)
		{
			prefab = ResolveEntityPrefabName(cursor);
			if (!prefab.IsEmpty() && HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
				return cursor;

			if (HST_VehicleRootPolicy.IsKnownVehicleRootName(cursor.GetName()) && !HST_VehicleRootPolicy.IsVehiclePartName(cursor.GetName()))
				return cursor;

			cursor = cursor.GetParent();
		}

		return null;
	}

	protected void EnsureMissionCarrierVehicleRecord(HST_CampaignState state, IEntity vehicle, string vehicleId, string vehicleName)
	{
		if (!state || !vehicle || vehicleId.IsEmpty())
			return;

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(vehicleId);
		if (!runtimeVehicle)
		{
			runtimeVehicle = new HST_RuntimeVehicleState();
			runtimeVehicle.m_sVehicleRuntimeId = vehicleId;
			state.m_aRuntimeVehicles.Insert(runtimeVehicle);
		}

		string prefab = ResolveEntityPrefabName(vehicle);
		runtimeVehicle.m_sPrefab = prefab;
		runtimeVehicle.m_sDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(prefab, vehicleName);
		runtimeVehicle.m_sRuntimeKind = "mission_carrier";
		runtimeVehicle.m_vPosition = vehicle.GetOrigin();
		runtimeVehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(vehicle.GetYawPitchRoll());
		runtimeVehicle.m_bDeleted = false;
	}

	protected string BuildPlayerCarrierId(int playerId)
	{
		return string.Format("player_%1_mission_carrier", playerId);
	}

	protected bool IsPlayerCarrierId(string carrierId)
	{
		return !carrierId.IsEmpty() && carrierId.Contains("_mission_carrier") && carrierId.Contains("player_");
	}

	protected bool IsPlayerCarrier(HST_MissionAssetState asset, int playerId)
	{
		if (!asset || asset.m_sCarriedByVehicleId.IsEmpty())
			return false;

		return asset.m_sCarriedByVehicleId == BuildPlayerCarrierId(playerId);
	}

	protected string BuildAssetShortLabel(HST_MissionAssetState asset)
	{
		if (!asset)
			return "mission asset";

		string label = asset.m_sRole;
		if (label.IsEmpty())
			label = asset.m_sKind;
		if (label.IsEmpty())
			label = asset.m_sAssetId;
		label.Replace("_", " ");
		return label;
	}

	protected int ResolveAssetCapacityCost(string kind, string role)
	{
		if (kind == ASSET_KIND_CAPTIVE)
			return 2;
		if (kind == ASSET_KIND_VEHICLE)
			return DEFAULT_MISSION_CARRIER_CAPACITY;
		if (role == ROLE_CITY_SUPPLIES || role == ROLE_CONVOY_PAYLOAD)
			return 2;

		return 1;
	}

	protected int ResolveAssetInteractionRadius(string kind, string role)
	{
		if (kind == ASSET_KIND_VEHICLE)
			return 28;
		if (kind == ASSET_KIND_TARGET || kind == ASSET_KIND_CHARACTER)
			return 22;

		return DEFAULT_ASSET_INTERACTION_RADIUS_METERS;
	}

	protected float ResolveDemolitionRequiredDamage(HST_MissionAssetState asset)
	{
		if (!asset)
			return 300.0;

		if (asset.m_sRole == ROLE_DESTROY_TARGET || asset.m_sKind == ASSET_KIND_TARGET)
			return 300.0;

		return 250.0;
	}

	protected void RefreshMissionObjectivesFromAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
				continue;

			string role = ResolveObjectiveAssetRole(mission, objective);
			if (role.IsEmpty())
				continue;

			int satisfied = CountSatisfiedAssetsForObjective(state, mission, objective, role);
			objective.m_iCurrentCount = Math.Min(Math.Max(1, objective.m_iRequiredCount), satisfied);
			objective.m_iCurrentProgress = Math.Min(objective.m_iRequiredProgress, satisfied);
			if (satisfied >= Math.Max(1, objective.m_iRequiredCount))
				CompleteWorldObjective(objective);
		}
	}

	protected bool AreMissionObjectivesComplete(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		if (HasPendingRequiredConvoyOutcome(state, mission))
			return false;

		bool found;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			found = true;
			if (!objective.m_bComplete || objective.m_bFailed)
				return false;
		}

		return found;
	}

	protected bool HasPendingRequiredConvoyOutcome(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		if (mission.m_sMissionId == "convoy_money")
			return HasPendingConvoyAssetOutcome(state, mission, ROLE_CONVOY_PAYLOAD);
		if (mission.m_sMissionId == "convoy_supplies")
			return HasPendingConvoyAssetOutcome(state, mission, ROLE_CONVOY_PAYLOAD);
		if (mission.m_sMissionId == "convoy_prisoners")
			return HasPendingConvoyAssetOutcome(state, mission, ROLE_CONVOY_CAPTIVE);
		if (mission.m_sMissionId == "convoy_ammo" || mission.m_sMissionId == "convoy_armored")
			return HasPendingConvoyVehicleCaptureOutcome(state, mission);

		return false;
	}

	protected bool HasPendingConvoyAssetOutcome(HST_CampaignState state, HST_ActiveMissionState mission, string role)
	{
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role)
				continue;

			if (!asset.m_bDestroyed && (!asset.m_bDelivered || !asset.m_bOutcomeApplied))
				return true;
		}

		return false;
	}

	protected bool HasPendingConvoyVehicleCaptureOutcome(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (mission.m_bConvoyVehicleCapturedOutcomeApplied)
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != ROLE_CONVOY_VEHICLE)
				continue;
			if (!asset.m_bDestroyed && !asset.m_bDelivered)
				return true;
		}

		return false;
	}

	protected int CountSatisfiedAssetsForObjective(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, string role)
	{
		int satisfied;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role)
				continue;

			if (mission.m_sRuntimePrimitive == PRIMITIVE_DESTROY_TARGET && asset.m_sRole == ROLE_DESTROY_TARGET)
			{
				if (asset.m_bDestroyed || (asset.m_sKind == ASSET_KIND_VEHICLE && asset.m_bDelivered))
					satisfied++;
				continue;
			}

			if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
			{
				if (asset.m_bDelivered)
					satisfied++;
				continue;
			}

			if (mission.m_sRuntimePrimitive == PRIMITIVE_DELIVER_SUPPLIES && asset.m_sRole == ROLE_CITY_SUPPLIES)
			{
				if (asset.m_bDelivered)
					satisfied++;
				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
			{
				if (asset.m_bDestroyed || (asset.m_sKind == ASSET_KIND_VEHICLE && asset.m_bDelivered))
					satisfied++;
				continue;
			}

			if (objective.m_sTargetId == "convoy")
			{
				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT)
			{
				if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
				{
					if (asset.m_bDelivered)
						satisfied++;
					continue;
				}

				if (asset.m_bPickedUp || asset.m_bDelivered)
					satisfied++;
				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
			{
				if (asset.m_bDelivered)
					satisfied++;
				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
			{
				if (asset.m_bDelivered)
					satisfied++;
				continue;
			}
		}

		return satisfied;
	}

	protected bool HasSatisfiedConvoyPayload(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		bool hasPayload;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			if (asset.m_sRole != ROLE_CONVOY_PAYLOAD && asset.m_sRole != ROLE_CONVOY_CAPTIVE)
				continue;

			hasPayload = true;
			if (asset.m_sRole == ROLE_CONVOY_CAPTIVE)
			{
				if (!asset.m_bDelivered)
					return false;
			}
			else if (!asset.m_bPickedUp && !asset.m_bDelivered)
			{
				return false;
			}
		}

		return hasPayload;
	}

	protected void UpdateMissionCountersFromAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		int picked;
		int delivered;
		int captives;
		int vehicles;
		int destroyed;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			if (asset.m_bPickedUp)
				picked++;
			if (asset.m_bDelivered)
				delivered++;
			if (asset.m_bDestroyed)
				destroyed++;
			if (asset.m_sKind == ASSET_KIND_CAPTIVE && asset.m_bDelivered)
				captives++;
			if (asset.m_sKind == ASSET_KIND_VEHICLE && asset.m_bDelivered)
				vehicles++;
		}

		mission.m_iRuntimePickupCount = picked;
		mission.m_iRuntimeDeliveryCount = delivered;
		mission.m_iRuntimeDestroyedCount = destroyed;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO || mission.m_sRuntimePrimitive == PRIMITIVE_DELIVER_SUPPLIES)
			mission.m_iRecoveredCargoCount = Math.Min(mission.m_iRequiredCargoCount, delivered);
		else
			mission.m_iRecoveredCargoCount = Math.Min(mission.m_iRequiredCargoCount, picked);
		mission.m_iExtractedCaptiveCount = Math.Min(mission.m_iRequiredCaptiveCount, captives);
		mission.m_iCapturedVehicleCount = Math.Min(mission.m_iRequiredVehicleCount, vehicles);
	}

	protected bool SyncMissionAssetRuntimePositions(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;

		bool changed;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bDelivered || asset.m_bDestroyed)
				continue;
			if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
				continue;

			vector position;
			bool resolved;
			IEntity entity = GetRuntimeEntity(asset.m_sEntityId);
			if (entity)
			{
				position = entity.GetOrigin();
				resolved = true;
			}
			else if (asset.m_bPickedUp && !asset.m_sCarriedByVehicleId.IsEmpty())
			{
				if (asset.m_sKind == ASSET_KIND_CAPTIVE && asset.m_sLastInteraction == PHASE_LOADED && IsPlayerCarrierId(asset.m_sCarriedByVehicleId))
				{
					IEntity playerEntity = ResolvePlayerEntityForCarrierId(asset.m_sCarriedByVehicleId);
					if (!ResolveEntityVehicle(playerEntity))
					{
						if (!IsZeroVector(asset.m_vLastKnownPosition))
							position = asset.m_vLastKnownPosition;
						else
							position = asset.m_vCurrentPosition;
						resolved = true;
					}
				}

				if (!resolved)
					resolved = TryResolveCarrierPosition(state, asset.m_sCarriedByVehicleId, asset.m_vCurrentPosition, position);
				if (!resolved && !IsZeroVector(asset.m_vLastKnownPosition))
				{
					position = asset.m_vLastKnownPosition;
					resolved = true;
				}
			}

			if (!resolved || IsZeroVector(position) || DistanceSq2D(asset.m_vCurrentPosition, position) <= 1.0)
				continue;

			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity)
				runtimeEntity.m_vPosition = position;
			HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
			if (runtimeVehicle)
				runtimeVehicle.m_vPosition = position;
			changed = true;
		}

		return changed;
	}

	protected bool UpdateFollowingCaptives(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		bool changed;
		int followIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_sKind != ASSET_KIND_CAPTIVE || !asset.m_bPickedUp || asset.m_bDelivered || asset.m_bDestroyed)
				continue;
			if (!asset.m_bAttachedToCarrier || asset.m_sCarriedByVehicleId.IsEmpty() || !IsPlayerCarrierId(asset.m_sCarriedByVehicleId))
				continue;

			IEntity playerEntity = ResolvePlayerEntityForCarrierId(asset.m_sCarriedByVehicleId);
			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			IEntity captiveEntity = GetRuntimeEntity(asset.m_sEntityId);
			if (!IsLivingPlayerEntity(playerEntity))
			{
				changed = StopCaptiveFollowing(state, mission, asset, ResolveCaptiveStopPosition(asset, captiveEntity)) || changed;
				continue;
			}

			vector playerPosition = playerEntity.GetOrigin();
			IEntity playerVehicle = ResolveEntityVehicle(playerEntity);
			if (!playerVehicle)
			{
				vector captiveRangePosition = ResolveCaptiveStopPosition(asset, captiveEntity);
				if (!IsZeroVector(captiveRangePosition) && DistanceSq2D(captiveRangePosition, playerPosition) > CAPTIVE_FOLLOW_BREAK_DISTANCE_METERS * CAPTIVE_FOLLOW_BREAK_DISTANCE_METERS)
				{
					changed = StopCaptiveFollowing(state, mission, asset, captiveRangePosition) || changed;
					continue;
				}
			}

			if (playerVehicle)
			{
				vector vehiclePosition = playerVehicle.GetOrigin();
				if (!captiveEntity)
				{
					captiveEntity = SpawnCaptiveFollowerProjection(state, mission, asset, BuildCaptiveFollowPosition(vehiclePosition, followIndex), "0 0 0");
					runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
					if (captiveEntity)
						changed = true;
				}

				string seatReason;
				if (captiveEntity)
				{
					if (TryMoveCaptiveIntoVehicle(captiveEntity, playerVehicle, seatReason))
					{
						StopCaptiveFollowController(asset);
						asset.m_bSpawned = true;
						if (DistanceSq2D(asset.m_vCurrentPosition, vehiclePosition) > 0.5)
							changed = true;
						asset.m_vCurrentPosition = vehiclePosition;
						asset.m_vLastKnownPosition = vehiclePosition;
						if (runtimeEntity)
						{
							runtimeEntity.m_bSpawned = true;
							runtimeEntity.m_bRecovered = false;
							runtimeEntity.m_vPosition = vehiclePosition;
						}
					}
					else
					{
						vector vehicleFollowPosition = BuildCaptiveFollowPosition(vehiclePosition, followIndex);
						if (!StartCaptiveFollowController(asset, playerVehicle))
							TryIssueCaptiveFollowWaypoint(state, asset, captiveEntity, vehicleFollowPosition, playerVehicle);
						vector captivePosition = captiveEntity.GetOrigin();
						if (DistanceSq2D(asset.m_vCurrentPosition, captivePosition) > 0.5)
							changed = true;
						asset.m_vCurrentPosition = captivePosition;
						asset.m_vLastKnownPosition = captivePosition;
						if (runtimeEntity)
						{
							runtimeEntity.m_bSpawned = true;
							runtimeEntity.m_bRecovered = false;
							runtimeEntity.m_vPosition = captivePosition;
						}
					}
				}

				if (asset.m_sLastInteraction != PHASE_LOADED)
				{
					asset.m_sLastInteraction = PHASE_LOADED;
					changed = true;
				}
				followIndex++;
				continue;
			}

			vector followPosition = BuildCaptiveFollowPosition(playerPosition, followIndex);
			followIndex++;
			if (!captiveEntity)
			{
				if (asset.m_sLastInteraction == PHASE_LOADED && DistanceSq2D(asset.m_vCurrentPosition, playerPosition) > CAPTIVE_DISEMBARK_RADIUS_METERS * CAPTIVE_DISEMBARK_RADIUS_METERS)
				{
					if (asset.m_bSpawned)
					{
						asset.m_bSpawned = false;
						changed = true;
					}
					if (runtimeEntity && runtimeEntity.m_bSpawned)
					{
						runtimeEntity.m_bSpawned = false;
						changed = true;
					}
					continue;
				}

				captiveEntity = SpawnCaptiveFollowerProjection(state, mission, asset, followPosition, "0 0 0");
				runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
				if (captiveEntity)
					changed = true;
			}

			if (!captiveEntity)
			{
				if (asset.m_bSpawned)
				{
					asset.m_bSpawned = false;
					changed = true;
				}
				if (runtimeEntity && runtimeEntity.m_bSpawned)
				{
					runtimeEntity.m_bSpawned = false;
					changed = true;
				}
				continue;
			}

			IEntity captiveVehicle = ResolveEntityVehicle(captiveEntity);
			if (captiveVehicle)
			{
				vector captiveVehiclePosition = captiveVehicle.GetOrigin();
				if (DistanceSq2D(playerPosition, captiveVehiclePosition) > CAPTIVE_DISEMBARK_RADIUS_METERS * CAPTIVE_DISEMBARK_RADIUS_METERS)
				{
					if (asset.m_sLastInteraction != PHASE_LOADED)
					{
						asset.m_sLastInteraction = PHASE_LOADED;
						changed = true;
					}
					if (DistanceSq2D(asset.m_vCurrentPosition, captiveVehiclePosition) > 0.5)
					{
						asset.m_vCurrentPosition = captiveVehiclePosition;
						asset.m_vLastKnownPosition = captiveVehiclePosition;
						changed = true;
					}
					if (runtimeEntity)
					{
						runtimeEntity.m_bSpawned = true;
						runtimeEntity.m_bRecovered = false;
						runtimeEntity.m_vPosition = captiveVehiclePosition;
					}
					continue;
				}

				StopCaptiveFollowController(asset);
				DeleteRuntimeEntity(asset.m_sEntityId);
				captiveEntity = SpawnCaptiveFollowerProjection(state, mission, asset, followPosition, "0 0 0");
				runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
				if (!captiveEntity)
					continue;
				changed = true;
			}

			if (asset.m_sLastInteraction != PHASE_FOLLOWING)
			{
				asset.m_sLastInteraction = PHASE_FOLLOWING;
				changed = true;
			}

			vector captivePosition = captiveEntity.GetOrigin();
			float followDistanceSq = DistanceSq2D(captivePosition, followPosition);
			if (followDistanceSq > CAPTIVE_FOLLOW_NEAR_DISTANCE_METERS * CAPTIVE_FOLLOW_NEAR_DISTANCE_METERS)
			{
				if (!StartCaptiveFollowController(asset, playerEntity))
					TryIssueCaptiveFollowWaypoint(state, asset, captiveEntity, followPosition, playerEntity);
			}

			if (DistanceSq2D(asset.m_vCurrentPosition, captivePosition) > 0.5)
			{
				asset.m_vCurrentPosition = captivePosition;
				asset.m_vLastKnownPosition = captivePosition;
				changed = true;
			}
			if (runtimeEntity)
			{
				runtimeEntity.m_bSpawned = true;
				runtimeEntity.m_bRecovered = false;
				runtimeEntity.m_vPosition = captivePosition;
			}
		}

		return changed;
	}

	protected vector BuildCaptiveFollowPosition(vector playerPosition, int followIndex)
	{
		vector offset = "0 0 0";
		if (followIndex == 0)
			offset[0] = 1.5;
		else if (followIndex == 1)
			offset[0] = -1.5;
		else
			offset[0] = 0.0;
		offset[2] = -2.0 - (followIndex * 0.75);

		return HST_WorldPositionService.ResolveSafeGroundPosition(playerPosition + offset, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 1.0);
	}

	protected IEntity SpawnCaptiveFollowerProjection(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset, vector position, vector angles)
	{
		if (!state || !mission || !asset || asset.m_sEntityId.IsEmpty())
			return null;

		IEntity existing = GetRuntimeEntity(asset.m_sEntityId);
		if (existing)
			return existing;

		string prefab = asset.m_sPrefab;
		if (prefab.IsEmpty())
			prefab = PROP_CAPTIVES;

		vector spawnPosition = HST_WorldPositionService.ResolveSafeGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		GenericEntity entity = HST_WorldPositionService.SpawnPrefab(prefab, spawnPosition, angles);
		if (!entity && prefab != PROP_CAPTIVES)
			entity = HST_WorldPositionService.SpawnPrefab(PROP_CAPTIVES, spawnPosition, angles);
		if (!entity)
			return null;

		ApplyCampaignDebugEntityName(entity, asset.m_sRole, asset.m_sAssetId);
		ApplyMissionAssetIdentity(entity, asset);
		HST_MissionAssetComponent assetComponent = HST_MissionAssetComponent.Cast(entity.FindComponent(HST_MissionAssetComponent));
		if (assetComponent)
		{
			assetComponent.ConfigureMissionAsset(asset.m_sAssetId, mission.m_sInstanceId, asset.m_sRole);
			bool actionQuarantined = asset.m_iRescueContractVersion
				== HST_RescuePOWOperationService.QUARANTINED_CONTRACT_VERSION
				|| HST_RescuePOWOperationService.IsQuarantinedMission(mission);
			assetComponent.ConfigureRescueActionProjection(true,
				asset.m_iRescueContractVersion,
				asset.m_eRescueDisposition,
				mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE && !actionQuarantined,
				actionQuarantined,
				asset.m_iRescueRevision);
		}

		asset.m_bSpawned = true;
		asset.m_vCurrentPosition = spawnPosition;
		asset.m_vLastKnownPosition = spawnPosition;
		m_aRuntimeEntityIds.Insert(asset.m_sEntityId);
		m_aRuntimeEntities.Insert(entity);
		RegisterAssetRuntimeEntityState(state, asset, spawnPosition, angles);
		return entity;
	}

	protected bool TryIssueCaptiveFollowWaypoint(HST_CampaignState state, HST_MissionAssetState asset, IEntity captiveEntity, vector followPosition, IEntity followTarget)
	{
		if (!state || !asset || !captiveEntity)
			return false;

		if (!ShouldIssueCaptiveFollowWaypoint(state, asset))
			return false;

		AIGroup group = ResolveCaptiveAIGroup(captiveEntity);
		if (!group)
			return false;

		GenericEntity waypointEntity;
		AIWaypoint waypoint;
		if (followTarget)
		{
			waypointEntity = HST_WorldPositionService.SpawnPrefab(CAPTIVE_FOLLOW_WAYPOINT_PREFAB, followPosition, "0 0 0");
			ApplyCampaignDebugEntityName(waypointEntity, "captive_follow_waypoint", string.Format("%1_%2", asset.m_sAssetId, state.m_iElapsedSeconds));
			SCR_EntityWaypoint entityWaypoint = SCR_EntityWaypoint.Cast(waypointEntity);
			if (entityWaypoint)
			{
				entityWaypoint.SetEntity(followTarget);
				waypoint = entityWaypoint;
			}
		}

		if (!waypoint)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
			waypointEntity = HST_WorldPositionService.SpawnPrefab(CAPTIVE_MOVE_WAYPOINT_PREFAB, followPosition, "0 0 0");
			ApplyCampaignDebugEntityName(waypointEntity, "captive_move_waypoint", string.Format("%1_%2", asset.m_sAssetId, state.m_iElapsedSeconds));
			waypoint = AIWaypoint.Cast(waypointEntity);
		}

		if (!waypoint)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
			return false;
		}

		waypoint.SetCompletionRadius(CAPTIVE_FOLLOW_NEAR_DISTANCE_METERS);
		group.AddWaypoint(waypoint);
		AIGroupMovementComponent groupMovement = AIGroupMovementComponent.Cast(group.FindComponent(AIGroupMovementComponent));
		if (groupMovement)
		{
			groupMovement.SetGroupCharactersWantedMovementType(EMovementType.RUN);
			groupMovement.SetFormationDisplacement(1);
		}
		return true;
	}

	protected bool ShouldIssueCaptiveFollowWaypoint(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!state || !asset || asset.m_sAssetId.IsEmpty())
			return false;

		int index = m_aCaptiveFollowWaypointAssetIds.Find(asset.m_sAssetId);
		if (index >= 0 && index < m_aCaptiveFollowWaypointSeconds.Count())
		{
			if (state.m_iElapsedSeconds - m_aCaptiveFollowWaypointSeconds[index] < CAPTIVE_FOLLOW_WAYPOINT_INTERVAL_SECONDS)
				return false;

			m_aCaptiveFollowWaypointSeconds[index] = state.m_iElapsedSeconds;
			return true;
		}

		m_aCaptiveFollowWaypointAssetIds.Insert(asset.m_sAssetId);
		m_aCaptiveFollowWaypointSeconds.Insert(state.m_iElapsedSeconds);
		return true;
	}

	protected AIGroup ResolveCaptiveAIGroup(IEntity captiveEntity)
	{
		if (!captiveEntity)
			return null;

		IEntity cursor = captiveEntity;
		int depth;
		while (cursor && depth < 6)
		{
			AIGroup group = AIGroup.Cast(cursor);
			if (group)
				return group;

			cursor = cursor.GetParent();
			depth++;
		}

		AIControlComponent controlComponent = AIControlComponent.Cast(captiveEntity.FindComponent(AIControlComponent));
		if (controlComponent)
		{
			controlComponent.ActivateAI();
			AIAgent agent = controlComponent.GetControlAIAgent();
			if (agent)
			{
				AIGroup parentGroup = agent.GetParentGroup();
				if (parentGroup)
					return parentGroup;
			}
		}

		return null;
	}

	protected bool TryMoveCaptiveIntoVehicle(
		IEntity captiveEntity,
		IEntity vehicleEntity,
		out string reason,
		string expectedSeatToken = "")
	{
		bool terminalFailure;
		return TryMoveCaptiveIntoVehicleClassified(
			captiveEntity,
			vehicleEntity,
			reason,
			terminalFailure,
			expectedSeatToken);
	}

	protected bool TryMoveCaptiveIntoVehicleClassified(
		IEntity captiveEntity,
		IEntity vehicleEntity,
		out string reason,
		out bool terminalFailure,
		string expectedSeatToken = "")
	{
		reason = "";
		terminalFailure = false;
		if (!captiveEntity || !vehicleEntity)
		{
			reason = "captive or vehicle missing";
			terminalFailure = true;
			return false;
		}

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(captiveEntity.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
		{
			reason = "captive has no compartment access component";
			terminalFailure = true;
			return false;
		}
		if (access.IsInCompartment() && access.GetVehicle() == vehicleEntity)
		{
			reason = "captive already seated";
			return true;
		}
		if (access.IsGettingIn())
		{
			reason = "captive already boarding";
			return true;
		}

		array<BaseCompartmentSlot> slots = {};
		if (!CollectExactRescueCarrierCargoSlotsRecursive(
			vehicleEntity,
			slots,
			0))
		{
			reason = "vehicle cargo compartment hierarchy is invalid";
			terminalFailure = true;
			return false;
		}

		string seatSelectionEvidence;
		bool structuralSeatFailure;
		BaseCompartmentSlot slot = FindAvailableCaptiveSeat(
			slots,
			captiveEntity,
			seatSelectionEvidence,
			structuralSeatFailure,
			expectedSeatToken);
		if (!slot)
		{
			terminalFailure = structuralSeatFailure;
			if (expectedSeatToken.IsEmpty())
				reason = "vehicle has no accessible cargo seat | "
					+ seatSelectionEvidence;
			else if (structuralSeatFailure)
				reason = "vehicle durable cargo seat identity is absent or ambiguous | "
					+ seatSelectionEvidence;
			else
				reason = "vehicle durable cargo seat is temporarily unavailable | "
					+ seatSelectionEvidence;
			return false;
		}

		IEntity slotOwner = slot.GetOwner();
		if (!slotOwner)
			slotOwner = vehicleEntity;
		RplComponent captiveReplication = RplComponent.Cast(
			captiveEntity.FindComponent(RplComponent));
		if ((!captiveReplication || captiveReplication.IsOwner())
			&& access.GetInVehicle(
				slotOwner,
				slot,
				true,
				-1,
				ECloseDoorAfterActions.INVALID,
				true))
		{
			StopCaptiveFollowControllerForEntity(captiveEntity);
			reason = "authority-local captive cargo move-in accepted";
			return true;
		}

		if (!access.MoveInVehicle(vehicleEntity, ECompartmentType.CARGO, true, slot))
		{
			reason = "captive vehicle boarding order rejected";
			return false;
		}

		StopCaptiveFollowControllerForEntity(captiveEntity);
		reason = "owner-routed captive cargo move-in accepted";
		return true;
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

	protected BaseCompartmentSlot FindAvailableCaptiveSeat(
		array<BaseCompartmentSlot> slots,
		IEntity captiveEntity,
		out string selectionEvidence,
		out bool structuralFailure,
		string expectedSeatToken = "")
	{
		selectionEvidence = "cargo seat selection is unavailable";
		structuralFailure = false;
		if (!slots || !captiveEntity)
		{
			structuralFailure = true;
			return null;
		}
		if (!expectedSeatToken.IsEmpty())
		{
			int matchCount;
			string matchMode;
			BaseCompartmentSlot exactSlot
				= ResolveExpectedExactRescueCarrierSeat(
					slots,
					expectedSeatToken,
					matchCount,
					matchMode);
			selectionEvidence = BuildExactRescueCarrierSeatSelectionEvidence(
				slots,
				captiveEntity,
				expectedSeatToken,
				matchCount,
				matchMode);
			if (!exactSlot || matchCount != 1)
			{
				structuralFailure = true;
				return null;
			}
			if (!exactSlot.IsCompartmentAccessible()
				|| exactSlot.IsOccupied()
				|| (exactSlot.IsReserved() && !exactSlot.IsReservedBy(captiveEntity))
				|| exactSlot.IsGetInLockedFor(captiveEntity))
				return null;
			return exactSlot;
		}

		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;
			if (slot.GetType() != ECompartmentType.CARGO)
				continue;
			if (!slot.IsCompartmentAccessible())
				continue;
			if (slot.IsOccupied())
				continue;
			if (slot.IsReserved() && !slot.IsReservedBy(captiveEntity))
				continue;
			if (slot.IsGetInLockedFor(captiveEntity))
				continue;

			selectionEvidence = BuildExactRescueCarrierSeatSelectionEvidence(
				slots,
				captiveEntity,
				"first_available",
				1,
				"tokenless");
			return slot;
		}

		selectionEvidence = BuildExactRescueCarrierSeatSelectionEvidence(
			slots,
			captiveEntity,
			"first_available",
			0,
			"tokenless");
		return null;
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

	protected IEntity ResolvePlayerEntityForCarrierId(string carrierId)
	{
		if (carrierId.IsEmpty())
			return null;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return null;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (carrierId != BuildPlayerCarrierId(playerId))
				continue;

			return playerManager.GetPlayerControlledEntity(playerId);
		}

		return null;
	}

	protected bool TryResolveCarrierPosition(HST_CampaignState state, string carrierId, vector fallbackPosition, out vector position)
	{
		position = fallbackPosition;
		if (carrierId.IsEmpty())
			return false;

		IEntity runtimeEntity = GetRuntimeEntity(carrierId);
		if (runtimeEntity)
		{
			position = runtimeEntity.GetOrigin();
			return true;
		}

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			array<int> playerIds = {};
			playerManager.GetPlayers(playerIds);
			foreach (int playerId : playerIds)
			{
				if (carrierId != BuildPlayerCarrierId(playerId))
					continue;

				IEntity playerEntity = playerManager.GetPlayerControlledEntity(playerId);
				if (!IsLivingPlayerEntity(playerEntity))
					return false;

				position = playerEntity.GetOrigin();
				return true;
			}
		}

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(carrierId);
		if (runtimeVehicle && !runtimeVehicle.m_bDeleted)
		{
			IEntity vehicle = FindMissionVehicleByRuntimeId(carrierId, runtimeVehicle.m_vPosition);
			if (vehicle)
			{
				position = vehicle.GetOrigin();
				runtimeVehicle.m_vPosition = position;
				return true;
			}

			if (!IsZeroVector(runtimeVehicle.m_vPosition))
			{
				position = runtimeVehicle.m_vPosition;
				return true;
			}
		}

		return false;
	}

	protected IEntity FindMissionVehicleByRuntimeId(string vehicleId, vector nearPosition)
	{
		if (vehicleId.IsEmpty() || IsZeroVector(nearPosition))
			return null;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		m_aMissionVehicleScanEntities.Clear();
		world.QueryEntitiesBySphere(nearPosition, 30.0, AddMissionVehicleScanCandidate, null, EQueryEntitiesFlags.ALL);
		foreach (IEntity candidate : m_aMissionVehicleScanEntities)
		{
			string prefab;
			IEntity root = ResolveMissionVehicleRoot(candidate, prefab);
			if (root && ResolveEntityRuntimeId(root) == vehicleId)
				return root;
		}

		return null;
	}

	protected bool IsConvoyStaticFallback(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		if (mission.m_sRuntimePhase == "convoy_static")
			return true;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == ROLE_CONVOY_VEHICLE && asset.m_sLastInteraction == "convoy_static")
				return true;
		}

		return false;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected string ResolveEntityRuntimeId(IEntity entity)
	{
		if (!entity)
			return "";

		BaseRplComponent rpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("vehicle_%1", rpl.Id());

		return string.Format("vehicle_local_%1_%2", entity.GetName(), entity.GetOrigin());
	}

	protected string ResolveEntityPrefabName(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "";

		return entity.GetPrefabData().GetPrefabName();
	}

	protected HST_MissionAssetState FindRepresentativeAssetForObjective(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!state || !mission || !objective)
			return null;

		string role = ResolveObjectiveAssetRole(mission, objective);
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (!role.IsEmpty() && asset.m_sRole != role)
				continue;

			return asset;
		}

		return null;
	}

	protected string ResolveObjectiveAssetRole(HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!mission || !objective)
			return "";

		if (objective.m_sTargetId == "convoy")
			return ROLE_CONVOY_VEHICLE;
		if (objective.m_sTargetId == "convoy_prisoners")
			return ROLE_CONVOY_CAPTIVE;
		if (objective.m_sTargetId == "convoy_cargo")
			return ROLE_CONVOY_PAYLOAD;
		if (objective.m_sTargetId == "supply_pickup")
			return ROLE_CITY_SUPPLIES;
		if (objective.m_sTargetId == "extract_captives")
			return ROLE_CAPTIVE;
		if (objective.m_sTargetId == "hq_delivery")
			return ROLE_LOGISTICS_CARGO;

		if (mission.m_sRuntimePrimitive == PRIMITIVE_KILL_HVT)
			return ROLE_HVT;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_DESTROY_TARGET)
			return ROLE_DESTROY_TARGET;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_DELIVER_SUPPLIES)
			return ROLE_CITY_SUPPLIES;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_RESCUE_EXTRACT)
			return ROLE_CAPTIVE;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
			return ROLE_LOGISTICS_CARGO;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
			return ROLE_CONVOY_VEHICLE;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_GUN_SHOP)
			return ROLE_GUN_SHOP_SELLER;

		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET)
			return ROLE_HVT;
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
			return ROLE_DESTROY_TARGET;
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT)
			return ROLE_LOGISTICS_CARGO;
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
			return ROLE_CITY_SUPPLIES;
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
			return ROLE_CAPTIVE;
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA)
			return ROLE_HOLD_MARKER;

		return "";
	}

	protected int ResolveConvoyVehicleCount(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionDefinition definition)
	{
		if (mission && mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION)
			return HST_MissionConvoyOperationService.EXACT_VEHICLE_COUNT;

		int seed = BuildConvoyVehicleCountSeed(state, mission, definition);
		int resolved = MIN_CONVOY_VEHICLES + HST_DefaultCatalog.PositiveMod(seed, MAX_CONVOY_VEHICLES - MIN_CONVOY_VEHICLES + 1);
		return Math.Max(MIN_CONVOY_VEHICLES, Math.Min(MAX_CONVOY_VEHICLES, resolved));
	}

	protected int BuildConvoyVehicleCountSeed(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionDefinition definition)
	{
		int seed = 7;
		if (state)
			seed += state.m_iCampaignSeed * 13 + state.m_iElapsedSeconds * 3 + state.m_aActiveMissions.Count() * 101 + state.m_aMissionAssets.Count() * 17;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 41 + mission.m_sMissionId.Length() * 53 + ResolveMissionInstanceNumericSeed(mission.m_sInstanceId) * 127;
		if (definition)
			seed += definition.m_sMissionId.Length() * 17 + definition.m_iRewardMoney;

		return seed;
	}

	protected int ResolveMissionInstanceNumericSeed(string instanceId)
	{
		string prefix = "mission_";
		if (instanceId.Contains(prefix) && instanceId.Length() > prefix.Length())
		{
			string suffix = instanceId.Substring(prefix.Length(), instanceId.Length() - prefix.Length());
			return suffix.ToInt();
		}

		return instanceId.Length();
	}

	protected int ResolveConvoyIdleDelaySeconds(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int seed = 991;
		if (state)
			seed += state.m_iCampaignSeed * 19 + state.m_iElapsedSeconds * 5;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 73 + mission.m_sMissionId.Length() * 37;

		return MIN_CONVOY_IDLE_SECONDS + HST_DefaultCatalog.PositiveMod(seed, MAX_CONVOY_IDLE_SECONDS - MIN_CONVOY_IDLE_SECONDS + 1);
	}

	protected void ConfigureConvoyObjectiveRequirements(HST_CampaignState state, HST_ActiveMissionState mission, int convoyVehicleCount)
	{
		if (!state || !mission)
			return;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_sTargetId != "convoy")
				continue;

			objective.m_sLabel = "Neutralize convoy crew";
			if (mission.m_sMissionId == "convoy_ammo")
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers, then capture a surviving vehicle to establish the ammo point.";
			else if (mission.m_sMissionId == "convoy_armored")
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers, then capture a surviving vehicle for the garage.";
			else if (mission.m_sMissionId == "convoy_money")
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers, then recover and deliver the money payload to HQ.";
			else if (mission.m_sMissionId == "convoy_prisoners")
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers, then free and extract the prisoners.";
			else if (mission.m_sMissionId == "convoy_supplies")
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers, then recover and deliver the supplies.";
			else
				objective.m_sRequirementText = "Ambush the convoy. Kill all convoy soldiers before they reach destination.";
			objective.m_iRequiredCount = Math.Max(1, convoyVehicleCount);
			objective.m_iRequiredProgress = Math.Max(1, convoyVehicleCount);
			objective.m_iCurrentCount = 0;
			objective.m_iCurrentProgress = 0;
		}
	}

	protected bool TryResolveConvoySpawnPlan(HST_CampaignState state, HST_ActiveMissionState mission, HST_GeneratedRouteState route, vector convoyEnd, int vehicleCount, out vector convoyStart, array<vector> convoyStartSlots, out string reason)
	{
		convoyStart = "0 0 0";
		reason = "";
		if (convoyStartSlots)
			convoyStartSlots.Clear();
		if (!state || !mission || IsZeroVector(convoyEnd))
		{
			reason = "mission state or convoy destination missing";
			return false;
		}
		if (vehicleCount <= 0)
		{
			reason = "convoy vehicle count is zero";
			return false;
		}
		if (!convoyStartSlots)
		{
			reason = "convoy start slot buffer missing";
			return false;
		}

		int seed = BuildConvoySpawnPlanSeed(state, mission, vehicleCount);
		for (int attempt = 0; attempt < CONVOY_START_PROBE_ATTEMPTS; attempt++)
		{
			float distanceMeters = ResolveConvoyRandomStartDistanceMeters(seed, attempt);
			vector preferred = BuildConvoySpawnPlanCandidate(route, convoyEnd, distanceMeters, seed, attempt);
			vector leadStart;
			string candidateReason;
			if (!TryResolveConvoyLeadStartCandidate(state, mission, preferred, convoyEnd, leadStart, candidateReason))
			{
				reason = candidateReason;
				continue;
			}

			if (!TryBuildConvoyVehicleStartSlots(leadStart, convoyEnd, vehicleCount, convoyStartSlots, candidateReason))
			{
				reason = candidateReason;
				continue;
			}
			if (convoyStartSlots.Count() != vehicleCount)
			{
				reason = string.Format("convoy spawn plan probed %1/%2 vehicle-safe slots", convoyStartSlots.Count(), vehicleCount);
				convoyStartSlots.Clear();
				continue;
			}

			convoyStart = convoyStartSlots[0];
			return true;
		}

		if (TryResolveConvoySpawnPlanFromGeneratedRouteAnchors(state, mission, convoyEnd, vehicleCount, convoyStart, convoyStartSlots, reason))
			return true;

		if (TryResolveConvoySpawnPlanFromGeneratedRouteSegments(state, mission, convoyEnd, vehicleCount, convoyStart, convoyStartSlots, reason))
			return true;

		if (reason.IsEmpty())
			reason = string.Format("no full-column convoy spawn candidate passed %1 probes", CONVOY_START_PROBE_ATTEMPTS);
		return false;
	}

	protected int BuildConvoySpawnPlanSeed(HST_CampaignState state, HST_ActiveMissionState mission, int vehicleCount)
	{
		int seed = 503;
		if (state)
			seed += state.m_iCampaignSeed * 17 + state.m_iElapsedSeconds * 29 + state.m_aActiveMissions.Count() * 37 + state.m_aMissionAssets.Count() * 41;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 43 + mission.m_sMissionId.Length() * 47 + ResolveMissionInstanceNumericSeed(mission.m_sInstanceId) * 131;

		seed += vehicleCount * 59;
		return seed;
	}

	protected float ResolveConvoyRandomStartDistanceMeters(int seed, int attempt)
	{
		int minDistance = Math.Round(MIN_CONVOY_START_DISTANCE_METERS);
		int maxDistance = Math.Round(MAX_CONVOY_START_DISTANCE_METERS);
		int span = Math.Max(1, maxDistance - minDistance + 1);
		return minDistance + HST_DefaultCatalog.PositiveMod(seed + attempt * 149 + attempt * attempt * 17, span);
	}

	protected vector BuildConvoySpawnPlanCandidate(HST_GeneratedRouteState route, vector convoyEnd, float distanceMeters, int seed, int attempt)
	{
		if (route)
		{
			if (attempt % 5 == 0 && !IsZeroVector(route.m_vStartPosition))
				return BuildConvoyStartCandidateFromAnchor(convoyEnd, route.m_vStartPosition, distanceMeters);
			if (attempt % 5 == 1 && !IsZeroVector(route.m_vMidPosition))
				return BuildConvoyStartCandidateFromAnchor(convoyEnd, route.m_vMidPosition, distanceMeters);
		}

		int direction = HST_DefaultCatalog.PositiveMod(seed + attempt * 3, 8);
		return BuildConvoyStartCandidate(convoyEnd, direction, distanceMeters);
	}

	protected vector BuildConvoyStartCandidateFromAnchor(vector targetPosition, vector anchorPosition, float distanceMeters)
	{
		float dx = anchorPosition[0] - targetPosition[0];
		float dz = anchorPosition[2] - targetPosition[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length <= 1.0)
			return BuildConvoyStartCandidate(targetPosition, 0, distanceMeters);

		vector candidate = targetPosition;
		candidate[0] = candidate[0] + (dx / length) * distanceMeters;
		candidate[2] = candidate[2] + (dz / length) * distanceMeters;
		return candidate;
	}

	protected bool TryResolveConvoySpawnPlanFromGeneratedRouteAnchors(HST_CampaignState state, HST_ActiveMissionState mission, vector convoyEnd, int vehicleCount, out vector convoyStart, array<vector> convoyStartSlots, out string reason)
	{
		convoyStart = "0 0 0";
		reason = "";
		if (!state || !convoyStartSlots)
		{
			reason = "generated route fallback missing state or slot buffer";
			return false;
		}

		int anchorCount;
		foreach (HST_GeneratedRouteState route : state.m_aGeneratedRoutes)
		{
			if (!route)
				continue;

			if (TryResolveConvoySpawnPlanFromAnchor(state, mission, convoyEnd, vehicleCount, route.m_vStartPosition, "generated route " + route.m_sRouteId + " start", convoyStart, convoyStartSlots, reason))
				return true;
			anchorCount++;
			if (TryResolveConvoySpawnPlanFromAnchor(state, mission, convoyEnd, vehicleCount, route.m_vMidPosition, "generated route " + route.m_sRouteId + " midpoint", convoyStart, convoyStartSlots, reason))
				return true;
			anchorCount++;
			if (TryResolveConvoySpawnPlanFromAnchor(state, mission, convoyEnd, vehicleCount, route.m_vEndPosition, "generated route " + route.m_sRouteId + " destination", convoyStart, convoyStartSlots, reason))
				return true;
			anchorCount++;
		}

		if (reason.IsEmpty())
			reason = string.Format("no generated route anchor passed the 2000-5000m convoy start probe (%1 anchors checked)", anchorCount);
		else
			reason = string.Format("%1; generated route fallback checked %2 anchors", reason, anchorCount);
		return false;
	}

	protected bool TryResolveConvoySpawnPlanFromGeneratedRouteSegments(HST_CampaignState state, HST_ActiveMissionState mission, vector convoyEnd, int vehicleCount, out vector convoyStart, array<vector> convoyStartSlots, out string reason)
	{
		convoyStart = "0 0 0";
		reason = "";
		if (!state || !convoyStartSlots)
		{
			reason = "generated route segment fallback missing state or slot buffer";
			return false;
		}

		int sampleCount;
		foreach (HST_GeneratedRouteState route : state.m_aGeneratedRoutes)
		{
			if (!route || route.m_aWaypoints.Count() < 2)
				continue;

			for (int index = 1; index < route.m_aWaypoints.Count(); index++)
			{
				HST_RouteWaypointState previous = route.m_aWaypoints[index - 1];
				HST_RouteWaypointState current = route.m_aWaypoints[index];
				if (!previous || !current)
					continue;

				for (int step = 1; step < CONVOY_ROUTE_SEGMENT_FALLBACK_STEPS; step++)
				{
					float t = step;
					t = t / CONVOY_ROUTE_SEGMENT_FALLBACK_STEPS;
					vector preferred = InterpolatePosition(previous.m_vPosition, current.m_vPosition, t);
					string label = string.Format("generated route %1 segment %2 sample %3/%4", route.m_sRouteId, index, step, CONVOY_ROUTE_SEGMENT_FALLBACK_STEPS);
					if (TryResolveConvoySpawnPlanFromAnchor(state, mission, convoyEnd, vehicleCount, preferred, label, convoyStart, convoyStartSlots, reason))
						return true;

					sampleCount++;
				}
			}
		}

		if (reason.IsEmpty())
			reason = string.Format("no generated route segment sample passed the 2000-5000m convoy start probe (%1 samples checked)", sampleCount);
		else
			reason = string.Format("%1; generated route segment fallback checked %2 samples", reason, sampleCount);
		return false;
	}

	protected bool TryResolveConvoySpawnPlanFromAnchor(HST_CampaignState state, HST_ActiveMissionState mission, vector convoyEnd, int vehicleCount, vector preferred, string label, out vector convoyStart, array<vector> convoyStartSlots, out string reason)
	{
		convoyStart = "0 0 0";
		reason = "";
		if (IsZeroVector(preferred))
		{
			reason = label + " is missing";
			return false;
		}

		float minCandidateDistance = Math.Max(0.0, MIN_CONVOY_START_DISTANCE_METERS - CONVOY_EXPANDED_START_ROAD_SEARCH_RADIUS_METERS);
		float maxCandidateDistance = MAX_CONVOY_START_DISTANCE_METERS + CONVOY_EXPANDED_START_ROAD_SEARCH_RADIUS_METERS;
		float anchorDistanceSq = DistanceSq2D(preferred, convoyEnd);
		if (anchorDistanceSq < minCandidateDistance * minCandidateDistance || anchorDistanceSq > maxCandidateDistance * maxCandidateDistance)
		{
			reason = label + " outside fallback distance window";
			return false;
		}

		vector leadStart;
		string candidateReason;
		if (!TryResolveConvoyLeadStartCandidate(state, mission, preferred, convoyEnd, leadStart, candidateReason))
		{
			reason = label + ": " + candidateReason;
			return false;
		}

		if (!TryBuildConvoyVehicleStartSlots(leadStart, convoyEnd, vehicleCount, convoyStartSlots, candidateReason))
		{
			reason = label + ": " + candidateReason;
			return false;
		}
		if (convoyStartSlots.Count() != vehicleCount)
		{
			reason = string.Format("%1: convoy spawn plan probed %2/%3 vehicle-safe slots", label, convoyStartSlots.Count(), vehicleCount);
			convoyStartSlots.Clear();
			return false;
		}

		convoyStart = convoyStartSlots[0];
		reason = label + " accepted";
		return true;
	}

	protected bool TryResolveConvoyLeadStartCandidate(HST_CampaignState state, HST_ActiveMissionState mission, vector preferred, vector convoyEnd, out vector resolved, out string reason)
	{
		reason = "";
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		if (!TryResolveConvoyRoadVehiclePosition(preferred, CONVOY_START_ROAD_SEARCH_RADIUS_METERS, CONVOY_EXPANDED_START_ROAD_SEARCH_RADIUS_METERS, convoyEnd, resolved, roadForward, roadWidth, roadDistance, roadReason))
		{
			reason = "lead convoy start is not road-resolved: " + roadReason;
			return false;
		}
		if (!HST_WorldPositionService.IsVehicleFootprintStableWithForward(resolved, roadForward))
		{
			reason = "lead road-resolved convoy start failed the flat vehicle footprint check";
			return false;
		}
		if (!IsConvoyStagingPositionClear(state, mission, resolved))
		{
			reason = "lead convoy start is too close to another zone or generated site";
			return false;
		}
		if (!IsUsableConvoyRouteSegment(resolved, convoyEnd))
		{
			reason = "lead convoy start failed the 2000-5000m road endpoint band";
			return false;
		}

		reason = roadReason;
		return true;
	}

	protected bool TryResolveConvoyRoadVehiclePosition(vector preferred, float primarySearchRadius, float expandedSearchRadius, vector destination, out vector resolved, out vector roadForward, out float roadWidth, out float roadDistance, out string reason)
	{
		string primaryReason;
		if (HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(preferred, primarySearchRadius, destination, resolved, roadForward, roadWidth, roadDistance, primaryReason))
		{
			reason = primaryReason;
			return true;
		}

		reason = primaryReason;
		if (expandedSearchRadius <= primarySearchRadius)
			return false;

		string expandedReason;
		if (!HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(preferred, expandedSearchRadius, destination, resolved, roadForward, roadWidth, roadDistance, expandedReason))
		{
			reason = string.Format("%1; expanded %2m search also failed: %3", primaryReason, Math.Round(expandedSearchRadius), expandedReason);
			return false;
		}

		reason = string.Format("%1 after expanded %2m road search; primary %3m search failed: %4", expandedReason, Math.Round(expandedSearchRadius), Math.Round(primarySearchRadius), primaryReason);
		return true;
	}

	protected bool TryBuildConvoyVehicleStartSlots(vector leadStart, vector convoyEnd, int vehicleCount, array<vector> convoyStartSlots, out string reason)
	{
		reason = "";
		if (!convoyStartSlots)
		{
			reason = "convoy start slot buffer missing";
			return false;
		}

		convoyStartSlots.Clear();
		for (int index = 0; index < vehicleCount; index++)
		{
			vector preferred = BuildConvoyColumnSlotPosition(leadStart, convoyEnd, index);
			vector resolved;
			if (!TryResolveConvoyVehicleStartSlot(preferred, convoyEnd, convoyStartSlots, resolved, reason))
			{
				convoyStartSlots.Clear();
				return false;
			}

			convoyStartSlots.Insert(resolved);
		}

		return true;
	}

	protected vector BuildConvoyColumnSlotPosition(vector leadStart, vector convoyEnd, int index)
	{
		vector slot = leadStart;
		if (index <= 0)
			return slot;

		float dx = leadStart[0] - convoyEnd[0];
		float dz = leadStart[2] - convoyEnd[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length <= 1.0)
		{
			slot[2] = slot[2] - CONVOY_VEHICLE_START_SPACING_METERS * index;
			return slot;
		}

		float distanceBehindLead = CONVOY_VEHICLE_START_SPACING_METERS * index;
		slot[0] = slot[0] + (dx / length) * distanceBehindLead;
		slot[2] = slot[2] + (dz / length) * distanceBehindLead;
		return slot;
	}

	protected vector ResolveConvoyEndPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		vector resolved;
		string reason;
		if (TryResolveConvoyEndPosition(state, mission, resolved, reason))
			return resolved;

		return "0 0 0";
	}

	protected bool TryResolveConvoyEndPosition(HST_CampaignState state, HST_ActiveMissionState mission, out vector resolved, out string reason)
	{
		resolved = "0 0 0";
		reason = "";
		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		vector preferred;
		vector forwardTarget;
		if (route)
		{
			preferred = route.m_vEndPosition;
			forwardTarget = route.m_vMidPosition;
		}
		else if (mission)
		{
			preferred = ResolveRuntimePropPosition(state, mission);
			forwardTarget = mission.m_vTargetPosition;
		}

		if (IsZeroVector(preferred))
		{
			reason = "convoy destination preferred position missing";
			return false;
		}
		if (IsZeroVector(forwardTarget))
			forwardTarget = preferred;

		vector roadForward;
		float roadWidth;
		float roadDistance;
		if (!HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(preferred, CONVOY_DESTINATION_ROAD_SEARCH_RADIUS_METERS, forwardTarget, resolved, roadForward, roadWidth, roadDistance, reason))
			return false;

		reason = string.Format("destination road-resolved | distance %1m | width %2m", Math.Round(roadDistance), Math.Round(roadWidth));
		return true;
	}

	protected HST_GeneratedRouteState ResolveMissionRoute(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sSiteId.IsEmpty())
			return null;

		HST_GeneratedSiteState site = state.FindGeneratedSite(mission.m_sSiteId);
		if (!site || site.m_sRouteId.IsEmpty())
			return null;

		return state.FindGeneratedRoute(site.m_sRouteId);
	}

	protected vector BuildConvoyStartCandidate(vector targetPosition, int direction, float distance)
	{
		vector candidate = targetPosition;
		float diagonal = distance * 0.707;
		if (direction == 0)
			candidate[0] = candidate[0] + distance;
		else if (direction == 1)
			candidate[0] = candidate[0] - distance;
		else if (direction == 2)
			candidate[2] = candidate[2] + distance;
		else if (direction == 3)
			candidate[2] = candidate[2] - distance;
		else if (direction == 4)
		{
			candidate[0] = candidate[0] + diagonal;
			candidate[2] = candidate[2] + diagonal;
		}
		else if (direction == 5)
		{
			candidate[0] = candidate[0] - diagonal;
			candidate[2] = candidate[2] + diagonal;
		}
		else if (direction == 6)
		{
			candidate[0] = candidate[0] + diagonal;
			candidate[2] = candidate[2] - diagonal;
		}
		else
		{
			candidate[0] = candidate[0] - diagonal;
			candidate[2] = candidate[2] - diagonal;
		}

		return candidate;
	}

	protected bool IsConvoyStagingPositionClear(HST_CampaignState state, HST_ActiveMissionState mission, vector position)
	{
		if (!state)
			return false;

		float zoneClearanceSq = MIN_CONVOY_STAGING_ZONE_CLEARANCE_METERS * MIN_CONVOY_STAGING_ZONE_CLEARANCE_METERS;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || (mission && zone.m_sZoneId == mission.m_sTargetZoneId))
				continue;

			if (DistanceSq2D(position, zone.m_vPosition) < zoneClearanceSq)
				return false;
		}

		float siteClearanceSq = MIN_CONVOY_STAGING_SITE_CLEARANCE_METERS * MIN_CONVOY_STAGING_SITE_CLEARANCE_METERS;
		foreach (HST_GeneratedSiteState site : state.m_aGeneratedSites)
		{
			if (!site || (mission && site.m_sZoneId == mission.m_sTargetZoneId))
				continue;

			if (DistanceSq2D(position, site.m_vPosition) < siteClearanceSq)
				return false;
		}

		return true;
	}

	protected bool IsUsableConvoyRouteSegment(vector startPosition, vector endPosition)
	{
		if (IsZeroVector(startPosition) || IsZeroVector(endPosition))
			return false;
		if (!IsConvoyDistanceInsideBand(startPosition, endPosition))
			return false;
		return true;
	}

	protected bool IsConvoyDistanceInsideBand(vector startPosition, vector endPosition)
	{
		if (IsZeroVector(startPosition) || IsZeroVector(endPosition))
			return false;

		float distanceSq = DistanceSq2D(startPosition, endPosition);
		return distanceSq >= MIN_CONVOY_START_DISTANCE_METERS * MIN_CONVOY_START_DISTANCE_METERS && distanceSq <= MAX_CONVOY_START_DISTANCE_METERS * MAX_CONVOY_START_DISTANCE_METERS;
	}

	protected bool IsDryConvoyRouteSegment(vector startPosition, vector endPosition)
	{
		for (int i = 1; i <= CONVOY_ROUTE_SAMPLE_COUNT; i++)
		{
			float t = i;
			t = t / (CONVOY_ROUTE_SAMPLE_COUNT + 1);
			vector sample = InterpolatePosition(startPosition, endPosition, t);
			vector resolved;
			if (!HST_WorldPositionService.TryResolveGroundPosition(sample, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, resolved, true))
				return false;
			if (HST_WorldPositionService.IsLikelyOpenWater(resolved))
				return false;
		}

		return true;
	}

	protected vector InterpolatePosition(vector startPosition, vector endPosition, float t)
	{
		vector result = startPosition;
		result[0] = startPosition[0] + (endPosition[0] - startPosition[0]) * t;
		result[1] = startPosition[1] + (endPosition[1] - startPosition[1]) * t;
		result[2] = startPosition[2] + (endPosition[2] - startPosition[2]) * t;
		return result;
	}

	protected vector OffsetConvoyVehicleStartPosition(HST_GeneratedRouteState route, vector startPosition, vector endPosition, int index, array<vector> reservedPositions)
	{
		float distanceAlongRoute = CONVOY_VEHICLE_START_SPACING_METERS * index;
		vector preferred = ResolveConvoyRouteStagingPosition(route, startPosition, endPosition, distanceAlongRoute);
		vector resolved;
		string slotReason;
		if (TryResolveConvoyVehicleStartSlot(preferred, endPosition, reservedPositions, resolved, slotReason))
			return resolved;

		for (int attempt = 1; attempt <= 8; attempt++)
		{
			float forwardDistance = distanceAlongRoute + MIN_CONVOY_VEHICLE_START_SEPARATION_METERS * attempt;
			vector forwardCandidate = ResolveConvoyRouteStagingPosition(route, startPosition, endPosition, forwardDistance);
			if (TryResolveConvoyVehicleStartSlot(forwardCandidate, endPosition, reservedPositions, resolved, slotReason))
				return resolved;

			float backwardDistance = distanceAlongRoute - MIN_CONVOY_VEHICLE_START_SEPARATION_METERS * attempt;
			if (backwardDistance < 0.0)
				continue;

			vector backwardCandidate = ResolveConvoyRouteStagingPosition(route, startPosition, endPosition, backwardDistance);
			if (TryResolveConvoyVehicleStartSlot(backwardCandidate, endPosition, reservedPositions, resolved, slotReason))
				return resolved;
		}

		if (TryResolveConvoyVehicleStartSlot(preferred, endPosition, reservedPositions, resolved, slotReason))
			return resolved;

		return startPosition;
	}

	protected bool TryResolveConvoyVehicleStartSlot(vector preferred, vector convoyEnd, array<vector> reservedPositions, out vector resolved, out string reason)
	{
		reason = "";
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		if (!TryResolveConvoyRoadVehiclePosition(preferred, CONVOY_SLOT_ROAD_SEARCH_RADIUS_METERS, CONVOY_EXPANDED_SLOT_ROAD_SEARCH_RADIUS_METERS, convoyEnd, resolved, roadForward, roadWidth, roadDistance, roadReason))
		{
			reason = "convoy vehicle slot is not road-resolved: " + roadReason;
			return false;
		}
		if (HST_WorldPositionService.IsLikelyOpenWater(resolved))
		{
			reason = "convoy vehicle slot resolved to open water";
			return false;
		}
		if (!HST_WorldPositionService.IsVehicleFootprintStableWithForward(resolved, roadForward))
		{
			reason = "convoy road vehicle slot failed the flat vehicle footprint check";
			return false;
		}
		if (!IsConvoyDistanceInsideBand(resolved, convoyEnd))
		{
			reason = "convoy vehicle slot is outside the required 2000-5000m destination band";
			return false;
		}
		if (!IsConvoyVehicleStartSeparated(resolved, reservedPositions))
		{
			reason = "convoy vehicle slot overlaps another pre-probed convoy vehicle slot";
			return false;
		}

		reason = roadReason;
		return true;
	}

	protected bool TryResolveConvoyVehicleStartCandidate(vector preferred, out vector resolved, bool preferHeavyVehicleTerrain)
	{
		if (preferHeavyVehicleTerrain)
			return HST_WorldPositionService.TryResolveHeavyVehicleSpawnPosition(preferred, resolved, true);

		return HST_WorldPositionService.TryResolveLargeVehicleSpawnPosition(preferred, resolved, true);
	}

	protected vector ResolveConvoyRouteStagingPosition(HST_GeneratedRouteState route, vector startPosition, vector endPosition, float distanceAlongRoute)
	{
		if (distanceAlongRoute <= 0.0)
			return startPosition;

		ref array<vector> routePoints = BuildConvoyRouteStagingPoints(route, startPosition, endPosition);
		if (!routePoints || routePoints.Count() == 0)
			return startPosition;
		if (routePoints.Count() == 1)
			return routePoints[0];

		vector previous = routePoints[0];
		float remaining = distanceAlongRoute;
		for (int i = 1; i < routePoints.Count(); i++)
		{
			vector current = routePoints[i];
			float segmentLength = Math.Sqrt(DistanceSq2D(previous, current));
			if (segmentLength <= 0.1)
			{
				previous = current;
				continue;
			}

			if (remaining <= segmentLength)
			{
				float t = remaining / segmentLength;
				return InterpolatePosition(previous, current, t);
			}

			remaining = remaining - segmentLength;
			previous = current;
		}

		return routePoints[routePoints.Count() - 1];
	}

	protected ref array<vector> BuildConvoyRouteStagingPoints(HST_GeneratedRouteState route, vector startPosition, vector endPosition)
	{
		ref array<vector> points = {};
		AppendConvoyRouteStagingPoint(points, startPosition);

		if (route && route.m_aWaypoints)
		{
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

				AppendConvoyRouteStagingPoint(points, selectedWaypoint.m_vPosition);
				lastIndex = selectedWaypoint.m_iIndex;
			}
		}

		AppendConvoyRouteStagingPoint(points, endPosition);
		return points;
	}

	protected void AppendConvoyRouteStagingPoint(array<vector> points, vector point)
	{
		if (!points || IsZeroVector(point))
			return;
		if (points.Count() > 0 && DistanceSq2D(points[points.Count() - 1], point) < 9.0)
			return;

		points.Insert(point);
	}

	protected bool IsConvoyVehicleStartSeparated(vector candidate, array<vector> reservedPositions)
	{
		if (!reservedPositions)
			return true;

		float minDistanceSq = MIN_CONVOY_VEHICLE_START_SEPARATION_METERS * MIN_CONVOY_VEHICLE_START_SEPARATION_METERS;
		foreach (vector reservedPosition : reservedPositions)
		{
			if (DistanceSq2D(candidate, reservedPosition) < minDistanceSq)
				return false;
		}

		return true;
	}

	protected vector OffsetMissionAssetPosition(vector position, int index)
	{
		vector offsetPosition = position;
		int safeIndex = index;
		if (safeIndex < 0)
			safeIndex = -safeIndex;

		int column = safeIndex - (safeIndex / 3) * 3;
		int row = safeIndex / 3;
		offsetPosition[0] = offsetPosition[0] + (column - 1) * 4.0;
		offsetPosition[2] = offsetPosition[2] + row * 5.0;
		return HST_WorldPositionService.ResolveSafeGroundPosition(offsetPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, true, 2.0);
	}

	protected vector ResolveRuntimePropPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == mission.m_sInstanceId)
				return HST_WorldPositionService.ResolveSafeGroundPosition(objective.m_vPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, true, 2.0);
		}

		if (!mission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
			if (zone)
				return HST_WorldPositionService.ResolveSafeGroundPosition(zone.m_vPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, true, 2.0);
		}

		return HST_WorldPositionService.ResolveSafeGroundPosition(state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, true, 2.0);
	}

	protected string SelectRuntimePropPrefab(string primitive)
	{
		if (primitive == PRIMITIVE_KILL_HVT)
			return PROP_HVT;
		if (primitive == PRIMITIVE_DESTROY_TARGET)
			return PROP_DESTROY_TARGET;
		if (primitive == PRIMITIVE_RECOVER_CARGO || primitive == PRIMITIVE_DELIVER_SUPPLIES)
			return PROP_CARGO;
		if (primitive == PRIMITIVE_RESCUE_EXTRACT)
			return PROP_CAPTIVES;
		if (primitive == PRIMITIVE_CONVOY_INTERCEPT)
			return PROP_CONVOY_VEHICLE;
		if (primitive == PRIMITIVE_HOLD_AREA || primitive == PRIMITIVE_CLEAR_AREA)
			return PROP_HOLD_MARKER;

		return "";
	}

	protected string ResolveLogisticsCargoPrefab(HST_ActiveMissionState mission)
	{
		if (!mission)
			return PROP_CARGO;

		if (mission.m_sMissionId == "logistics_bank")
			return PROP_BANK_MONEY;

		if (mission.m_sMissionId.Contains("resource") || mission.m_sMissionId.Contains("factory") || mission.m_sMissionId.Contains("seaport") || mission.m_sMissionId.Contains("support") || mission.m_sMissionId.Contains("salvage") || mission.m_sMissionId == "dynamic_gun_shop")
			return PROP_RESOURCE_CACHE;

		return PROP_CARGO;
	}

	protected string ResolveMissionVehiclePrefab(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		string factionKey = ResolveMissionOwnerFaction(state, mission);
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		int seed = 0;
		if (state)
			seed += state.m_iCampaignSeed + state.m_iElapsedSeconds * 11;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 29 + mission.m_sMissionId.Length() * 37;

		if (faction && faction.m_aVehiclePrefabs.Count() > 0)
		{
			int startIndex = HST_DefaultCatalog.PositiveMod(seed, faction.m_aVehiclePrefabs.Count());
			for (int i = 0; i < faction.m_aVehiclePrefabs.Count(); i++)
			{
				int index = HST_DefaultCatalog.PositiveMod(startIndex + i, faction.m_aVehiclePrefabs.Count());
				string candidate = faction.m_aVehiclePrefabs[index];
				if (IsValidMissionVehiclePrefab(candidate))
					return candidate;
			}
		}

		if (IsValidMissionVehiclePrefab(PROP_CONVOY_VEHICLE))
			return PROP_CONVOY_VEHICLE;

		return "";
	}

	protected string ResolveMissionOwnerFaction(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sTargetZoneId.IsEmpty())
			return "";

		HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
		if (!zone)
			return "";

		return zone.m_sOwnerFactionKey;
	}

	protected bool IsValidMissionVehiclePrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		ResourceName resourceName = prefab;
		Resource loaded = Resource.Load(resourceName);
		return loaded && loaded.IsValid();
	}

	protected vector BuildRuntimePropAngles(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		vector angles;
		int seed = 313;
		if (state)
			seed += state.m_iCampaignSeed * 19 + state.m_iElapsedSeconds * 3;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 97 + mission.m_sMissionId.Length() * 53;
		if (seed < 0)
			seed = -seed;

		angles[0] = seed - (seed / 360) * 360;
		return HST_WorldPositionService.BuildUprightAnglesFromVector(angles);
	}

	string FindCompletedActiveMissionId(HST_CampaignState state, HST_MissionObjectiveService objectives)
	{
		if (!state || !objectives)
			return "";

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission)
				continue;
			if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
				continue;
			if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
				continue;
			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE && !CanCompleteExpiredPlayerBoundMission(state, mission))
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;

			if (objectives.AreMissionObjectivesComplete(state, mission.m_sInstanceId))
				return mission.m_sInstanceId;
		}

		return "";
	}

	string FindCompletedCampaignDebugMissionId(HST_CampaignState state, HST_MissionObjectiveService objectives, HST_ActiveMissionState mission)
	{
		if (!state || !objectives || !mission || mission.m_sInstanceId.IsEmpty())
			return "";

		HST_ActiveMissionState authoritativeMission = state.FindActiveMission(mission.m_sInstanceId);
		if (authoritativeMission != mission)
			return "";
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return "";
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return "";
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE && !CanCompleteExpiredPlayerBoundMission(state, mission))
			return "";
		if (IsPersistenceSmokeMission(mission))
			return "";
		if (!objectives.AreMissionObjectivesComplete(state, mission.m_sInstanceId))
			return "";

		return mission.m_sInstanceId;
	}

	string FindFailedActiveMissionId(HST_CampaignState state)
	{
		if (!state)
			return "";

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;
			if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
				continue;
			if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
				continue;

			if (mission.m_sRuntimePhase == PHASE_FAILED)
				return mission.m_sInstanceId;
		}

		return "";
	}

	string FindFailedCampaignDebugMissionId(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sInstanceId.IsEmpty())
			return "";

		HST_ActiveMissionState authoritativeMission = state.FindActiveMission(mission.m_sInstanceId);
		if (authoritativeMission != mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return "";
		if (IsPersistenceSmokeMission(mission))
			return "";
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return "";
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return "";
		if (mission.m_sRuntimePhase != PHASE_FAILED)
			return "";

		return mission.m_sInstanceId;
	}

	string BuildRuntimeReport(HST_CampaignState state)
	{
		if (!state)
			return "Partisan mission runtime | state not ready";

		int physical;
		int fallback;
		int spawned;
		int objectiveCount;
		int assetCount;
		string details = "";
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;

			if (mission.m_eRuntimeMode == HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_PHYSICAL_MVP || mission.m_eRuntimeMode == HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_STATE_MACHINE)
				physical++;
			if (mission.m_bRuntimeFallback)
				fallback++;
			if (mission.m_bRuntimeSpawned)
				spawned++;
			objectiveCount += CountMissionObjectivesForReport(state, mission.m_sInstanceId);
			assetCount += CountMissionAssetsForReport(state, mission.m_sInstanceId);

			details = details + BuildMissionRuntimeReport(state, mission);
		}

		return string.Format("Partisan mission runtime | physical %1 | spawned %2 | fallback %3 | objectives %4 | assets %5", physical, spawned, fallback, objectiveCount, assetCount) + details;
	}

	string BuildRuntimeReportForMission(HST_CampaignState state, string instanceId)
	{
		if (!state)
			return "Partisan mission runtime | state not ready";
		if (instanceId.IsEmpty())
			return "Partisan mission runtime | mission instance ID required";

		HST_ActiveMissionState mission = state.FindActiveMission(instanceId);
		if (!mission)
			return "Partisan mission runtime | mission not found: " + instanceId;

		return "Partisan mission runtime | selected mission" + BuildMissionRuntimeReport(state, mission);
	}

	protected string BuildMissionRuntimeReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "";

		int objectiveTotal;
		int objectiveComplete;
		int objectiveFailed;
		CountMissionObjectives(state, mission.m_sInstanceId, objectiveTotal, objectiveComplete, objectiveFailed);

		int assetTotal;
		int assetSpawned;
		int assetDestroyed;
		int assetDelivered;
		CountMissionAssets(state, mission.m_sInstanceId, assetTotal, assetSpawned, assetDestroyed, assetDelivered);

		int runtimeEntityCount = CountMissionRuntimeEntities(state, mission.m_sInstanceId);
		string failureReason = ReportText(mission.m_sRuntimeFailureReason);

		string line = string.Format("\ninstance %1 | mission %2 | name %3 | target zone %4 | site %5", ReportText(mission.m_sInstanceId), ReportText(mission.m_sMissionId), ReportMissionDisplayName(mission), ReportTargetZone(state, mission.m_sTargetZoneId), ReportSite(state, mission.m_sSiteId));
		line = line + string.Format(" | status %1 | primitive %2 | runtime %3 | phase %4 | remaining %5s", mission.m_eStatus, ReportText(mission.m_sRuntimePrimitive), ReportText(mission.m_sRuntimeType), ReportText(mission.m_sRuntimePhase), mission.m_iRemainingSeconds);
		line = line + string.Format(" | objective count %1 | complete objectives %2 | failed objectives %3", objectiveTotal, objectiveComplete, objectiveFailed);
		line = line + string.Format(" | mission asset count %1 | runtime entity count %2 | failure reason %3", assetTotal, runtimeEntityCount, failureReason);
		line = line + string.Format(" | assets spawned %1/%2", assetSpawned, assetTotal);
		line = line + string.Format(" destroyed %1 delivered %2", assetDestroyed, assetDelivered);
		line = line + string.Format(" | fallback %1 | cleanup %2", mission.m_bRuntimeFallback, mission.m_bRuntimeCleanupComplete);

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
			line = line + BuildConvoyRouteReport(state, mission);

		line = line + BuildMissionObjectiveRuntimeReport(state, mission);
		line = line + BuildMissionAssetRuntimeReport(state, mission);
		return line;
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

	protected string ReportMissionDisplayName(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "none";
		if (!mission.m_sDisplayName.IsEmpty())
			return mission.m_sDisplayName;

		return ReportText(mission.m_sMissionId);
	}

	protected string ReportTargetZone(HST_CampaignState state, string zoneId)
	{
		if (zoneId.IsEmpty())
			return "none";
		if (!state)
			return "missing:" + zoneId;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return "missing:" + zoneId;

		return zoneId;
	}

	protected string ReportSite(HST_CampaignState state, string siteId)
	{
		if (siteId.IsEmpty())
			return "none";
		if (!state)
			return "missing:" + siteId;

		HST_GeneratedSiteState site = state.FindGeneratedSite(siteId);
		if (!site)
			return "missing:" + siteId;

		return siteId;
	}

	protected void CountMissionObjectives(HST_CampaignState state, string instanceId, out int total, out int complete, out int failed)
	{
		total = 0;
		complete = 0;
		failed = 0;
		if (!state || instanceId.IsEmpty())
			return;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != instanceId)
				continue;

			total++;
			if (objective.m_bComplete)
				complete++;
			if (objective.m_bFailed)
				failed++;
		}
	}

	protected void CountMissionAssets(HST_CampaignState state, string instanceId, out int total, out int spawned, out int destroyed, out int delivered)
	{
		total = 0;
		spawned = 0;
		destroyed = 0;
		delivered = 0;
		if (!state || instanceId.IsEmpty())
			return;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != instanceId)
				continue;

			total++;
			if (asset.m_bSpawned)
				spawned++;
			if (asset.m_bDestroyed)
				destroyed++;
			if (asset.m_bDelivered)
				delivered++;
		}
	}

	protected int CountMissionRuntimeEntities(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return 0;

		int count;
		foreach (HST_MissionRuntimeEntityState runtimeEntity : state.m_aMissionRuntimeEntities)
		{
			if (runtimeEntity && runtimeEntity.m_sMissionInstanceId == instanceId)
				count++;
		}

		return count;
	}

	protected int CountMissionObjectivesForReport(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return 0;

		int count;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == instanceId)
				count++;
		}

		return count;
	}

	protected int CountMissionAssetsForReport(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return 0;

		int count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == instanceId)
				count++;
		}

		return count;
	}

	protected bool IsPersistenceSmokeMission(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		return mission.m_sInstanceId.Contains(PERSISTENCE_SMOKE_PREFIX) || mission.m_sMissionId.Contains(PERSISTENCE_SMOKE_PREFIX);
	}

	protected string BuildConvoyRouteReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		string routeId = "none";
		vector start;
		vector mid;
		vector end = mission.m_vTargetPosition;
		int distanceMeters;
		int waypointCount;
		bool roadRoute;
		bool vehicleSafe;
		string validationReason = "none";
		string waypointReport = "";
		if (route)
		{
			routeId = route.m_sRouteId;
			start = route.m_vStartPosition;
			mid = route.m_vMidPosition;
			end = route.m_vEndPosition;
			distanceMeters = route.m_iDistanceMeters;
			waypointCount = route.m_iWaypointCount;
			roadRoute = route.m_bRoadRoute;
			vehicleSafe = route.m_bValidatedForVehicles;
			if (!route.m_sValidationFailureReason.IsEmpty())
				validationReason = route.m_sValidationFailureReason;
			waypointReport = BuildConvoyRouteWaypointReport(route);
		}

		vector convoySource = ResolveFirstConvoyAssetSourcePosition(state, mission);
		vector convoyTarget = ResolveFirstConvoyAssetTargetPosition(state, mission);
		int plannedDistanceMeters = Math.Round(Math.Sqrt(DistanceSq2D(convoySource, convoyTarget)));
		bool bandValid = IsConvoyDistanceInsideBand(convoySource, convoyTarget);
		string roadManagerReason;
		bool roadManagerAvailable = HST_WorldPositionService.IsRoadNetworkAvailable(roadManagerReason);
		string report = string.Format("\n  convoy route | route %1 | road %2 | vehicle-safe %3", routeId, roadRoute, vehicleSafe);
		report = report + string.Format(" | waypoint count %1 | distance %2m | validation %3 | road manager %4 %5", waypointCount, distanceMeters, validationReason, ReportBool(roadManagerAvailable), ReportText(roadManagerReason));
		report = report + string.Format(" | start %1 | mid %2 | end %3", start, mid, end);
		report = report + string.Format(" | planned source %1 | planned target %2 | planned distance %3m", convoySource, convoyTarget, plannedDistanceMeters);
		report = report + string.Format(" | required band 2000-5000m | band valid %1", ReportBool(bandValid));
		report = report + string.Format(" | required vehicles %1 | captured %2 | eta %3s", mission.m_iRequiredVehicleCount, mission.m_iCapturedVehicleCount, mission.m_iRuntimeETASeconds);
		report = report + string.Format(" | timers %1/%2/%3", mission.m_iRuntimeCounterA, mission.m_iRuntimeCounterB, mission.m_iRuntimeCounterC);
		report = report + BuildConvoyRoadResolverReport("planned source road", convoySource, convoyTarget, CONVOY_SLOT_ROAD_SEARCH_RADIUS_METERS);
		report = report + BuildConvoyRoadResolverReport("planned target road", convoyTarget, convoySource, CONVOY_DESTINATION_ROAD_SEARCH_RADIUS_METERS);
		return report + waypointReport;
	}

	protected string BuildConvoyRouteWaypointReport(HST_GeneratedRouteState route)
	{
		if (!route)
			return "";

		string report = "";
		foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
		{
			if (!waypoint)
				continue;

			report = report + string.Format("\n    route waypoint template %1 | hint %2 | radius %3m | position %4 | road check assigned-runtime-chain", waypoint.m_iIndex, waypoint.m_sHint, waypoint.m_iRadiusMeters, waypoint.m_vPosition);
		}

		return report;
	}

	protected string BuildConvoyRoadResolverReport(string label, vector position, vector destination, float searchRadius)
	{
		vector roadPosition;
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		bool roadResolved = HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(position, searchRadius, destination, roadPosition, roadForward, roadWidth, roadDistance, roadReason);
		if (!roadResolved)
			return string.Format("\n    %1 | road-resolved no | reason %2", label, ReportText(roadReason));

		return string.Format("\n    %1 | road-resolved yes | road position %2 | forward %3 | width %4m | resolver distance %5m", label, roadPosition, roadForward, Math.Round(roadWidth), Math.Round(roadDistance));
	}

	protected vector ResolveFirstConvoyAssetSourcePosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == ROLE_CONVOY_VEHICLE)
				return asset.m_vSourcePosition;
		}

		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		if (route && !IsZeroVector(route.m_vStartPosition))
			return route.m_vStartPosition;

		return mission.m_vTargetPosition;
	}

	protected vector ResolveFirstConvoyAssetTargetPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == ROLE_CONVOY_VEHICLE)
				return asset.m_vTargetPosition;
		}

		HST_GeneratedRouteState route = ResolveMissionRoute(state, mission);
		if (route && !IsZeroVector(route.m_vEndPosition))
			return route.m_vEndPosition;

		return mission.m_vTargetPosition;
	}

	protected string BuildMissionObjectiveRuntimeReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "";

		string report = "";
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			string status = "active";
			if (objective.m_bComplete)
				status = "complete";
			else if (objective.m_bFailed)
				status = "failed";

			report = report + string.Format("\n  objective | %1 | %2 | target %3 | status %4", objective.m_sObjectiveId, objective.m_sLabel, objective.m_sTargetId, status);
			report = report + string.Format(" | progress %1/%2 | count %3/%4", objective.m_iCurrentProgress, objective.m_iRequiredProgress, objective.m_iCurrentCount, objective.m_iRequiredCount);
			report = report + string.Format(" | hold %1/%2 | entity %3", objective.m_iHoldSeconds, objective.m_iRequiredHoldSeconds, objective.m_sLinkedRuntimeEntityId);
		}

		return report;
	}

	protected string BuildMissionAssetRuntimeReport(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "";

		string report = "";
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			string status = "active";
			if (asset.m_bDestroyed)
				status = "destroyed";
			else if (asset.m_bDelivered)
				status = "delivered";
			else if (asset.m_bPickedUp)
				status = "picked_up";
			else if (!asset.m_bSpawned)
				status = "queued";

			report = report + string.Format("\n  asset | %1 | %2/%3 | status %4", asset.m_sAssetId, asset.m_sKind, asset.m_sRole, status);
			report = report + string.Format(" | source %1 | current %2 | target %3", asset.m_vSourcePosition, asset.m_vCurrentPosition, asset.m_vTargetPosition);
			report = report + string.Format(" | radius %1m | capacity %2 | attached %3 | outcome %4", asset.m_iInteractionRadiusMeters, asset.m_iCargoCapacityCost, ReportBool(asset.m_bAttachedToCarrier), ReportBool(asset.m_bOutcomeApplied));
			if (!asset.m_sCarriedByVehicleId.IsEmpty())
				report = report + " | carrier " + asset.m_sCarriedByVehicleId;
			if (!asset.m_sOutcomeKind.IsEmpty())
				report = report + " | outcome kind " + asset.m_sOutcomeKind;
			if (!IsZeroVector(asset.m_vCurrentPosition) && !IsZeroVector(asset.m_vTargetPosition))
				report = report + string.Format(" | delivery distance %1m", Math.Round(Math.Sqrt(DistanceSq2D(asset.m_vCurrentPosition, asset.m_vTargetPosition))));
			if (!asset.m_sLastInteraction.IsEmpty())
				report = report + " | last " + asset.m_sLastInteraction;
		}

		return report;
	}

	string PrimitiveForMission(HST_MissionDefinition definition)
	{
		if (!definition)
			return PRIMITIVE_ABSTRACT_FALLBACK;

		return PrimitiveForMissionId(definition.m_sMissionId, definition.m_eCategory);
	}

	string PrimitiveForMissionId(string missionId, HST_EMissionCategory category)
	{
		if (missionId == "dynamic_gun_shop")
			return PRIMITIVE_GUN_SHOP;

		if (missionId == "assassinate_officer" || missionId == "assassinate_traitor" || missionId == "assassinate_specops")
			return PRIMITIVE_KILL_HVT;

		if (missionId == "conquest_town" || missionId == "conquest_resource" || missionId == "conquest_factory" || missionId == "conquest_outpost" || missionId == "conquest_airfield" || missionId == "conquest_seaport" || missionId == "dynamic_defend_petros" || missionId == "dynamic_city_flip_battle")
			return PRIMITIVE_HOLD_AREA;

		if (missionId == "destroy_radio_tower" || missionId == "destroy_downed_helicopter" || missionId == "destroy_outpost_cache" || missionId == "destroy_factory_asset" || missionId == "destroy_airfield_asset" || missionId == "destroy_seaport_asset" || missionId == "destroy_or_steal_armor" || missionId == "dynamic_stop_tower_rebuild")
			return PRIMITIVE_DESTROY_TARGET;

		if (missionId == "logistics_bank" || missionId == "logistics_resource_cache" || missionId == "logistics_factory_supplies" || missionId == "logistics_airfield_intel" || missionId == "logistics_seaport_supplies" || missionId == "logistics_support_cache" || missionId == "logistics_salvage_supplies" || missionId == "logistics_ammo_truck" || missionId == "logistics_weapons_truck")
			return PRIMITIVE_RECOVER_CARGO;

		if (missionId == "rescue_pows" || missionId == "rescue_refugees")
			return PRIMITIVE_RESCUE_EXTRACT;

		if (missionId == "support_city_supplies")
			return PRIMITIVE_DELIVER_SUPPLIES;

		if (missionId == "convoy_ammo" || missionId == "convoy_armored" || missionId == "convoy_money" || missionId == "convoy_prisoners" || missionId == "convoy_reinforcements" || missionId == "convoy_supplies")
			return PRIMITIVE_CONVOY_INTERCEPT;

		if (missionId == "dynamic_minor_city_task")
			return PRIMITIVE_CLEAR_AREA;

		if (category == HST_EMissionCategory.HST_MISSION_CONVOY)
			return PRIMITIVE_CONVOY_INTERCEPT;
		if (category == HST_EMissionCategory.HST_MISSION_ASSASSINATION)
			return PRIMITIVE_KILL_HVT;
		if (category == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return PRIMITIVE_HOLD_AREA;
		if (category == HST_EMissionCategory.HST_MISSION_DESTROY)
			return PRIMITIVE_DESTROY_TARGET;
		if (category == HST_EMissionCategory.HST_MISSION_LOGISTICS)
			return PRIMITIVE_RECOVER_CARGO;
		if (category == HST_EMissionCategory.HST_MISSION_RESCUE)
			return PRIMITIVE_RESCUE_EXTRACT;
		if (category == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return PRIMITIVE_DELIVER_SUPPLIES;

		return PRIMITIVE_CLEAR_AREA;
	}

	protected bool PollObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, int elapsedSeconds)
	{
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;
		if (mission && mission.m_sRuntimePrimitive == PRIMITIVE_GUN_SHOP)
			return false;

		return PollPrimitiveObjective(state, preset, mission, objective, elapsedSeconds);
	}

	protected bool PollPrimitiveObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, int elapsedSeconds)
	{
		if (!state || !mission || !objective)
			return false;

		string primitive = objective.m_sRuntimePrimitive;
		if (primitive.IsEmpty())
			primitive = mission.m_sRuntimePrimitive;

		if (primitive == PRIMITIVE_HOLD_AREA)
			return PollHoldAreaObjective(state, preset, mission, objective, elapsedSeconds);
		if (primitive == PRIMITIVE_CLEAR_AREA)
			return PollClearAreaObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_KILL_HVT)
			return PollKillHVTObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_DESTROY_TARGET)
			return PollDestroyTargetObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_RECOVER_CARGO)
			return PollRecoverCargoObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_RESCUE_EXTRACT)
			return PollRescueExtractObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_DELIVER_SUPPLIES)
			return PollDeliverSuppliesObjective(state, preset, mission, objective);
		if (primitive == PRIMITIVE_CONVOY_INTERCEPT)
			return PollConvoyInterceptObjective(state, preset, mission, objective);

		return PollFallbackObjective(state, preset, mission, objective, elapsedSeconds);
	}

	protected bool PollKillHVTObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		if (IsRuntimeEntityDestroyed(objective.m_sLinkedRuntimeEntityId))
			return CompleteWorldObjective(objective);

		return false;
	}

	protected bool PollDestroyTargetObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		if (IsRuntimeEntityDestroyed(objective.m_sLinkedRuntimeEntityId))
			return CompleteWorldObjective(objective);

		return false;
	}

	protected bool PollRecoverCargoObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		return false;
	}

	protected bool PollRescueExtractObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		return false;
	}

	protected bool PollDeliverSuppliesObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		return false;
	}

	protected bool PollConvoyInterceptObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		bool handledByAssets;
		bool assetChanged = PollMissionAssetObjective(state, preset, mission, objective, handledByAssets);
		if (handledByAssets)
			return assetChanged;

		return false;
	}

	protected bool PollHoldAreaObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, int elapsedSeconds)
	{
		float playerRadius = ResolveAreaObjectiveRadiusMeters(state, mission, objective, PLAYER_OBJECTIVE_RADIUS_METERS);
		bool playerNear = IsAnyLivingPlayerNearObjective(objective.m_vPosition, playerRadius);
		if (!playerNear)
			return false;

		objective.m_bWorldDetected = true;
		if (mission.m_sRuntimePhase == PHASE_ACTIVE)
			mission.m_sRuntimePhase = PHASE_CONTACT;

		float hostileRadius = ResolveAreaObjectiveRadiusMeters(state, mission, objective, HOSTILE_OBJECTIVE_RADIUS_METERS);
		if (mission.m_sMissionId != "dynamic_defend_petros" && HasHostileActiveGroupNear(state, preset, objective.m_vPosition, hostileRadius))
		{
			mission.m_sRuntimePhase = PHASE_CONTACT;
			return false;
		}

		mission.m_sRuntimePhase = PHASE_HOLDING;
		return ProgressHoldObjective(objective, elapsedSeconds);
	}

	protected bool PollClearAreaObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		float playerRadius = ResolveAreaObjectiveRadiusMeters(state, mission, objective, PLAYER_OBJECTIVE_RADIUS_METERS);
		bool playerNear = IsAnyLivingPlayerNearObjective(objective.m_vPosition, playerRadius);
		if (!playerNear)
			return false;

		objective.m_bWorldDetected = true;
		float hostileRadius = ResolveAreaObjectiveRadiusMeters(state, mission, objective, HOSTILE_OBJECTIVE_RADIUS_METERS);
		if (HasUnresolvedClearAreaHostiles(state, preset, mission, objective, hostileRadius))
		{
			mission.m_sRuntimePhase = PHASE_CONTACT;
			return false;
		}

		return CompleteWorldObjective(objective);
	}

	protected float ResolveAreaObjectiveRadiusMeters(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, float defaultRadiusMeters)
	{
		float radius = defaultRadiusMeters;
		if (!state || !objective)
			return radius;

		if (objective.m_eType != HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA && objective.m_eType != HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA)
			return radius;

		HST_ZoneState zone = state.FindZone(objective.m_sTargetId);
		if (!zone && mission)
			zone = state.FindZone(mission.m_sTargetZoneId);
		if (zone && zone.m_iCaptureRadiusMeters > 0)
			radius = Math.Max(radius, zone.m_iCaptureRadiusMeters);

		return radius;
	}

	protected bool PollFallbackObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, int elapsedSeconds)
	{
		bool playerNear = IsAnyLivingPlayerNearObjective(objective.m_vPosition, PLAYER_OBJECTIVE_RADIUS_METERS);
		if (!playerNear)
			return false;

		objective.m_bWorldDetected = true;
		if (mission.m_sRuntimePhase == PHASE_ACTIVE)
			mission.m_sRuntimePhase = PHASE_CONTACT;

		objective.m_bAbstractFallback = true;
		return CompleteWorldObjective(objective);
	}

	protected bool PollMissionAssetObjective(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, out bool handled)
	{
		handled = false;
		if (!state || !mission || !objective)
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
		{
			handled = true;
			return false;
		}

		string role = ResolveObjectiveAssetRole(mission, objective);
		if (role.IsEmpty())
			return false;
		if (!HasMatchingObjectiveAssets(state, mission, objective) && mission.m_bRuntimeFallback && HasRuntimeEntity(mission.m_sRuntimeEntityId))
			return false;
		if (objective.m_sTargetId == "convoy")
		{
			handled = true;
			return false;
		}
		if ((objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA) && objective.m_sTargetId != "convoy")
			return false;

		bool changed;
		int satisfiedCount;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role)
				continue;
			if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
				continue;

			handled = true;
			if (SyncMissionAssetDestructionState(state, asset))
				changed = true;

			if (asset.m_bDestroyed)
			{
				if (asset.m_sKind == ASSET_KIND_CAPTIVE)
				{
					MarkRuntimeMissionFailed(state, mission, "Required captives were killed.");
					return true;
				}

				if (mission.m_sRuntimePrimitive == PRIMITIVE_DESTROY_TARGET && asset.m_sRole == ROLE_DESTROY_TARGET)
					satisfiedCount++;
				else if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET || objective.m_sTargetId == "convoy")
					satisfiedCount++;

				continue;
			}

			if (objective.m_sTargetId == "convoy")
			{
				if (asset.m_bDelivered)
					satisfiedCount++;
				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
				continue;

			if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO && asset.m_sRole == ROLE_LOGISTICS_CARGO)
			{
				if (asset.m_bDelivered)
					satisfiedCount++;

				continue;
			}

			if (mission.m_sRuntimePrimitive == PRIMITIVE_DELIVER_SUPPLIES && asset.m_sRole == ROLE_CITY_SUPPLIES)
			{
				if (asset.m_bDelivered)
					satisfiedCount++;

				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT)
			{
				if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
				{
					if (asset.m_bDelivered)
						satisfiedCount++;

					continue;
				}

				if (asset.m_bPickedUp)
					satisfiedCount++;

				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
			{
				if (asset.m_sRole == ROLE_CONVOY_CAPTIVE)
				{
					if (asset.m_bDelivered)
						satisfiedCount++;

					continue;
				}

				if (asset.m_bDelivered)
					satisfiedCount++;

				continue;
			}

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
			{
				if (asset.m_bDelivered)
					satisfiedCount++;
			}
		}

		if (!handled)
			return false;

		if (objective.m_sTargetId == "convoy" && HasSatisfiedConvoyPayload(state, mission))
			satisfiedCount = Math.Max(satisfiedCount, Math.Max(1, objective.m_iRequiredCount));

		objective.m_iCurrentCount = Math.Min(Math.Max(1, objective.m_iRequiredCount), satisfiedCount);
		objective.m_iCurrentProgress = Math.Min(objective.m_iRequiredProgress, satisfiedCount);
		if (satisfiedCount >= Math.Max(1, objective.m_iRequiredCount))
			return CompleteWorldObjective(objective) || changed;

		return changed;
	}

	protected bool HasMatchingObjectiveAssets(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!state || !mission || !objective)
			return false;

		string role = ResolveObjectiveAssetRole(mission, objective);
		if (role.IsEmpty())
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == role)
				return true;
		}

		return false;
	}

	protected bool SyncMissionAssetDestructionState(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!state || !asset || asset.m_bDestroyed)
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
			return false;

		if (!IsRuntimeEntityDestroyed(asset.m_sEntityId))
			return false;

		asset.m_bDestroyed = true;
		asset.m_bAlive = false;
		asset.m_sLastInteraction = PHASE_DESTROYED;
		if (!IsZeroVector(asset.m_vCurrentPosition))
			asset.m_vLastKnownPosition = asset.m_vCurrentPosition;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_bDestroyed = true;

		return true;
	}

	protected void MarkMissionAssetPickedUp(HST_CampaignState state, HST_MissionAssetState asset)
	{
		if (!asset || asset.m_bPickedUp)
			return;

		asset.m_bPickedUp = true;
		asset.m_vCurrentPosition = asset.m_vTargetPosition;
		asset.m_vLastKnownPosition = asset.m_vCurrentPosition;
		asset.m_sLastInteraction = PHASE_LOADED;
		DeleteRuntimeEntity(asset.m_sEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_bRecovered = true;
	}

	protected void MarkMissionAssetDelivered(HST_CampaignState state, HST_MissionAssetState asset, vector deliveryPosition)
	{
		if (!asset || asset.m_bDelivered)
			return;

		asset.m_bPickedUp = true;
		asset.m_bDelivered = true;
		asset.m_vCurrentPosition = deliveryPosition;
		asset.m_vLastKnownPosition = deliveryPosition;
		asset.m_bAttachedToCarrier = false;
		asset.m_sCarriedByVehicleId = "";
		asset.m_sLastInteraction = PHASE_DELIVERED;
		if (asset.m_sKind == ASSET_KIND_CAPTIVE)
			DeleteRuntimeEntity(asset.m_sEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bRecovered = true;
			runtimeEntity.m_vPosition = deliveryPosition;
		}
	}

	protected bool ProgressHoldObjective(HST_MissionObjectiveState objective, int elapsedSeconds)
	{
		int requiredSeconds = objective.m_iRequiredHoldSeconds;
		if (requiredSeconds <= 0)
			requiredSeconds = DEFAULT_HOLD_SECONDS;

		objective.m_iHoldSeconds = Math.Min(requiredSeconds, objective.m_iHoldSeconds + elapsedSeconds);
		if (objective.m_iHoldSeconds < requiredSeconds)
			return true;

		objective.m_iCurrentProgress = objective.m_iRequiredProgress;
		objective.m_iCurrentCount = objective.m_iRequiredCount;
		objective.m_bComplete = true;
		return true;
	}

	protected bool CompleteWorldObjective(HST_MissionObjectiveState objective)
	{
		if (!objective || objective.m_bComplete)
			return false;

		objective.m_iCurrentProgress = objective.m_iRequiredProgress;
		objective.m_bComplete = true;
		return true;
	}

	protected bool TickGunShopOpenRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_GUN_SHOP || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;

		bool changed = EnsureGunShopStockGenerated(state, preset, mission);
		changed = EnsureGunShopSellerAsset(state, mission) || changed;
		if (mission.m_bGunShopPurchaseMade && mission.m_iRemainingSeconds > GUN_SHOP_MAX_EXPEDITED_SECONDS)
		{
			mission.m_iRemainingSeconds = GUN_SHOP_MAX_EXPEDITED_SECONDS;
			mission.m_iActiveUntilSecond = state.m_iElapsedSeconds + GUN_SHOP_MAX_EXPEDITED_SECONDS;
			mission.m_sRuntimePhase = "shop_reserved";
			UpdateGunShopAssetDeadlines(state, mission);
			changed = true;
		}

		return changed;
	}

	protected bool ShouldContinueGunShopRuntime(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_GUN_SHOP)
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_EXPIRED)
			return false;
		if (mission.m_bGunShopDeliveryArrived)
			return false;

		return CountGunShopPurchasedItems(mission) > 0;
	}

	protected bool TickGunShopDeliveryRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, int elapsedSeconds)
	{
		if (!ShouldContinueGunShopRuntime(state, mission))
			return false;

		bool changed = MarkGunShopSellerDeparted(state, mission);
		changed = EnsureGunShopDeliveryAssets(state, mission) || changed;
		changed = TrySpawnMissionRuntimeAssets(state, mission) || changed;
		changed = TrySeatGunShopDeliveryDriver(state, mission) || changed;
		changed = IssueGunShopDeliveryWaypoint(state, mission) || changed;
		changed = AdvanceGunShopDeliveryVehicle(state, mission, elapsedSeconds) || changed;
		return changed;
	}

	protected bool IsGunShopDeliveryComplete(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		return mission && mission.m_sRuntimePrimitive == PRIMITIVE_GUN_SHOP && mission.m_bGunShopDeliveryArrived;
	}

	protected bool EnsureGunShopStockGenerated(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_GUN_SHOP)
			return false;
		if (mission.m_bGunShopStockGenerated)
			return false;

		if (!mission.m_aGunShopItems)
			mission.m_aGunShopItems = {};
		mission.m_aGunShopItems.Clear();

		array<ref HST_GunShopCatalogCandidate> weapons = {};
		array<ref HST_GunShopCatalogCandidate> attachments = {};
		array<ref HST_GunShopCatalogCandidate> clothing = {};
		BuildGunShopCatalogCandidates(state, weapons, attachments, clothing);

		int seed = BuildGunShopSeed(state, mission, 101);
		int weaponCount = ResolveGunShopTypeCount(weapons.Count(), GUN_SHOP_MIN_WEAPON_TYPES, GUN_SHOP_MAX_WEAPON_TYPES, seed);
		int clothingCount = ResolveGunShopTypeCount(clothing.Count(), GUN_SHOP_MIN_CLOTHING_TYPES, GUN_SHOP_MAX_CLOTHING_TYPES, seed + 331);
		int attachmentCount;
		if (attachments.Count() > 0 && weaponCount > 0)
		{
			int desiredAttachmentMin = Math.Max(1, weaponCount);
			int desiredAttachmentMax = Math.Max(desiredAttachmentMin, weaponCount * 3);
			attachmentCount = ResolveGunShopTypeCount(attachments.Count(), desiredAttachmentMin, desiredAttachmentMax, seed + 719);
		}

		AppendGunShopStockItems(mission, weapons, weaponCount, seed + 17);
		AppendGunShopStockItems(mission, attachments, attachmentCount, seed + 41);
		AppendGunShopStockItems(mission, clothing, clothingCount, seed + 83);

		mission.m_bGunShopStockGenerated = true;
		mission.m_iGunShopPurchasedTotal = CountGunShopPurchasedItems(mission);
		return true;
	}

	protected void BuildGunShopCatalogCandidates(HST_CampaignState state, array<ref HST_GunShopCatalogCandidate> weapons, array<ref HST_GunShopCatalogCandidate> attachments, array<ref HST_GunShopCatalogCandidate> clothing)
	{
		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (catalogManager)
		{
			array<SCR_ArsenalItem> arsenalItems = {};
			catalogManager.GetAllArsenalItems(arsenalItems);
			foreach (SCR_ArsenalItem arsenalItem : arsenalItems)
			{
				if (!arsenalItem)
					continue;

				string prefab = arsenalItem.GetItemResourceName();
				string category = ResolveGunShopCatalogCategory(arsenalItem, prefab);
				if (category.IsEmpty())
					continue;

				AppendGunShopCandidateToCategory(state, weapons, attachments, clothing, prefab, HST_DisplayNameService.ResolveItemDisplayName(null, prefab), category, "", arsenalItem.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT, false));
			}
		}

		AppendGunShopArsenalFallbackCandidates(state, weapons, attachments, clothing);
	}

	protected void AppendGunShopArsenalFallbackCandidates(HST_CampaignState state, array<ref HST_GunShopCatalogCandidate> weapons, array<ref HST_GunShopCatalogCandidate> attachments, array<ref HST_GunShopCatalogCandidate> clothing)
	{
		if (!state)
			return;

		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!item || item.m_sPrefab.IsEmpty())
				continue;
			if (item.m_sCategory.IsEmpty())
				continue;
			if (item.m_sCategory == "magazine" || item.m_sCategory == "explosive" || item.m_sCategory == "medical")
				continue;

			string category = NormalizeGunShopCategory(item.m_sCategory, item.m_sPrefab);
			if (category.IsEmpty())
				continue;

			AppendGunShopCandidateToCategory(state, weapons, attachments, clothing, item.m_sPrefab, item.m_sDisplayName, category, "", 0);
		}
	}

	protected void AppendGunShopCandidateToCategory(HST_CampaignState state, array<ref HST_GunShopCatalogCandidate> weapons, array<ref HST_GunShopCatalogCandidate> attachments, array<ref HST_GunShopCatalogCandidate> clothing, string prefab, string displayName, string category, string factionKey, int supplyCost)
	{
		if (category == "attachment")
		{
			AppendGunShopCandidate(state, attachments, prefab, displayName, category, factionKey, supplyCost);
			return;
		}
		if (IsGunShopClothingCategory(category))
		{
			AppendGunShopCandidate(state, clothing, prefab, displayName, category, factionKey, supplyCost);
			return;
		}

		AppendGunShopCandidate(state, weapons, prefab, displayName, category, factionKey, supplyCost);
	}

	protected void AppendGunShopCandidate(HST_CampaignState state, array<ref HST_GunShopCatalogCandidate> candidates, string prefab, string displayName, string category, string factionKey, int supplyCost)
	{
		if (!candidates || prefab.IsEmpty() || category.IsEmpty())
			return;
		if (HasGunShopCandidatePrefab(candidates, prefab))
			return;

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
			return;

		string depositReason;
		if (m_Arsenal && !m_Arsenal.CanDepositItem(m_Balance, prefab, category, false, depositReason, displayName))
			return;

		HST_GunShopCatalogCandidate candidate = new HST_GunShopCatalogCandidate();
		candidate.m_sPrefab = prefab;
		candidate.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, prefab, displayName);
		candidate.m_sCategory = category;
		candidate.m_sFactionKey = factionKey;
		candidate.m_iSupplyCost = supplyCost;
		candidates.Insert(candidate);
	}

	protected bool HasGunShopCandidatePrefab(array<ref HST_GunShopCatalogCandidate> candidates, string prefab)
	{
		if (!candidates || prefab.IsEmpty())
			return false;

		foreach (HST_GunShopCatalogCandidate candidate : candidates)
		{
			if (candidate && candidate.m_sPrefab == prefab)
				return true;
		}

		return false;
	}

	protected string ResolveGunShopCatalogCategory(SCR_ArsenalItem arsenalItem, string prefab)
	{
		if (!arsenalItem || prefab.IsEmpty())
			return "";

		SCR_EArsenalItemType type = arsenalItem.GetItemType();
		SCR_EArsenalItemMode mode = arsenalItem.GetItemMode();
		if (SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.WEAPON_ATTACHMENT) || SCR_Enum.HasPartialFlag(mode, SCR_EArsenalItemMode.ATTACHMENT))
			return "attachment";
		if (SCR_Enum.HasPartialFlag(mode, SCR_EArsenalItemMode.AMMUNITION) || SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.EXPLOSIVES) || SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.LETHAL_THROWABLE) || SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.NON_LETHAL_THROWABLE))
			return "";
		if (SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.RIFLE) || SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.MACHINE_GUN) || SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.SNIPER_RIFLE) || SCR_Enum.HasPartialFlag(mode, SCR_EArsenalItemMode.WEAPON) || SCR_Enum.HasPartialFlag(mode, SCR_EArsenalItemMode.WEAPON_VARIANTS))
			return "weapon";
		if (SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.PISTOL))
			return "sidearm";
		if (SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.ROCKET_LAUNCHER))
			return "launcher";
		if (SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.HEADWEAR))
			return "headgear";
		if (SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.TORSO))
			return "clothing";
		if (SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.VEST_AND_WAIST))
			return "vest";
		if (SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.LEGS))
			return "pants";
		if (SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.FOOTWEAR))
			return "boots";
		if (SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.HANDWEAR))
			return "handwear";
		if (SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.BACKPACK) || SCR_Enum.HasPartialFlag(type, SCR_EArsenalItemType.RADIO_BACKPACK))
			return "backpack";

		return NormalizeGunShopCategory("", prefab);
	}

	protected string NormalizeGunShopCategory(string category, string prefab)
	{
		if (category == "weapon" || category == "sidearm" || category == "launcher" || category == "attachment")
			return category;
		if (IsGunShopClothingCategory(category))
			return category;

		string lower = prefab;
		lower.ToLower();
		if (lower.Contains("attachment") || lower.Contains("optic") || lower.Contains("sight") || lower.Contains("scope") || lower.Contains("muzzle") || lower.Contains("suppressor") || lower.Contains("bipod") || lower.Contains("grip") || lower.Contains("handguard") || lower.Contains("stock"))
			return "attachment";
		if (lower.Contains("/weapons/") || lower.Contains("rifle") || lower.Contains("weapon_"))
			return "weapon";
		if (lower.Contains("headgear") || lower.Contains("helmet") || lower.Contains("cap_"))
			return "headgear";
		if (lower.Contains("vest") || lower.Contains("webbing"))
			return "vest";
		if (lower.Contains("backpack"))
			return "backpack";
		if (lower.Contains("pants") || lower.Contains("trousers"))
			return "pants";
		if (lower.Contains("boots"))
			return "boots";
		if (lower.Contains("gloves") || lower.Contains("handwear"))
			return "handwear";
		if (lower.Contains("/characters/") || lower.Contains("/uniforms/") || lower.Contains("jacket") || lower.Contains("shirt"))
			return "clothing";

		return "";
	}

	protected bool IsGunShopClothingCategory(string category)
	{
		return category == "headgear" || category == "clothing" || category == "vest" || category == "webbing" || category == "pants" || category == "boots" || category == "handwear" || category == "backpack";
	}

	protected int ResolveGunShopTypeCount(int available, int minCount, int maxCount, int seed)
	{
		if (available <= 0)
			return 0;

		int low = Math.Min(available, Math.Max(1, minCount));
		int high = Math.Min(available, Math.Max(low, maxCount));
		if (high <= low)
			return low;

		return low + HST_DefaultCatalog.PositiveMod(seed, high - low + 1);
	}

	protected void AppendGunShopStockItems(HST_ActiveMissionState mission, array<ref HST_GunShopCatalogCandidate> candidates, int count, int seed)
	{
		if (!mission || !candidates || count <= 0 || candidates.Count() <= 0)
			return;

		int startIndex = HST_DefaultCatalog.PositiveMod(seed, candidates.Count());
		int emitted;
		for (int i = 0; i < candidates.Count() && emitted < count; i++)
		{
			int index = HST_DefaultCatalog.PositiveMod(startIndex + i * 7, candidates.Count());
			HST_GunShopCatalogCandidate candidate = candidates[index];
			if (!candidate || HasGunShopStockPrefab(mission, candidate.m_sPrefab))
				continue;

			HST_GunShopItemState item = new HST_GunShopItemState();
			item.m_sItemId = string.Format("shop_%1_%2", mission.m_sInstanceId, mission.m_aGunShopItems.Count());
			item.m_sPrefab = candidate.m_sPrefab;
			item.m_sDisplayName = candidate.m_sDisplayName;
			item.m_sCategory = candidate.m_sCategory;
			item.m_sFactionKey = candidate.m_sFactionKey;
			item.m_iAvailableCount = ResolveGunShopStockQuantity(candidate, seed + i * 53);
			item.m_iBuyCost = ResolveGunShopBuyCost(candidate, seed + i * 97);
			item.m_iSellCost = Math.Max(1, item.m_iBuyCost * (10 + HST_DefaultCatalog.PositiveMod(seed + i * 131, 21)) / 100);
			item.m_bCanSell = true;
			mission.m_aGunShopItems.Insert(item);
			emitted++;
		}
	}

	protected bool HasGunShopStockPrefab(HST_ActiveMissionState mission, string prefab)
	{
		if (!mission || !mission.m_aGunShopItems || prefab.IsEmpty())
			return false;

		foreach (HST_GunShopItemState item : mission.m_aGunShopItems)
		{
			if (item && item.m_sPrefab == prefab)
				return true;
		}

		return false;
	}

	protected int ResolveGunShopStockQuantity(HST_GunShopCatalogCandidate candidate, int seed)
	{
		if (!candidate)
			return 1;

		HST_ArsenalItemState synthetic = new HST_ArsenalItemState();
		synthetic.m_sPrefab = candidate.m_sPrefab;
		synthetic.m_sCategory = candidate.m_sCategory;

		int threshold = -1;
		if (m_Arsenal)
			threshold = m_Arsenal.ResolveUnlockThreshold(synthetic, m_Balance);
		if (threshold <= 0 && m_Balance)
			threshold = Math.Max(1, m_Balance.m_iArsenalUnlockThreshold);
		if (threshold <= 0)
			threshold = 18;

		int cap = Math.Max(1, threshold * 75 / 100);
		return 1 + HST_DefaultCatalog.PositiveMod(seed, cap);
	}

	protected int ResolveGunShopBuyCost(HST_GunShopCatalogCandidate candidate, int seed)
	{
		if (!candidate)
			return 25;

		int baseCost = candidate.m_iSupplyCost;
		if (baseCost <= 0)
		{
			if (candidate.m_sCategory == "weapon")
				baseCost = 160;
			else if (candidate.m_sCategory == "launcher")
				baseCost = 260;
			else if (candidate.m_sCategory == "sidearm")
				baseCost = 80;
			else if (candidate.m_sCategory == "attachment")
				baseCost = 55;
			else
				baseCost = 35;
		}

		int variationPercent = 80 + HST_DefaultCatalog.PositiveMod(seed, 61);
		return Math.Max(5, baseCost * variationPercent / 100);
	}

	protected int CountGunShopPurchasedItems(HST_ActiveMissionState mission)
	{
		if (!mission || !mission.m_aGunShopItems)
			return 0;

		int count;
		foreach (HST_GunShopItemState item : mission.m_aGunShopItems)
		{
			if (item)
				count += Math.Max(0, item.m_iPurchasedCount);
		}

		return count;
	}

	protected bool EnsureGunShopSellerAsset(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		vector sellerPosition = mission.m_vGunShopSellerPosition;
		if (IsZeroVector(sellerPosition))
			sellerPosition = ResolveRuntimePropPosition(state, mission);
		sellerPosition = HST_WorldPositionService.ResolveSafeGroundPosition(sellerPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 2.0);
		mission.m_vGunShopSellerPosition = sellerPosition;
		mission.m_vTargetPosition = sellerPosition;
		if (mission.m_sGunShopSellerAssetId.IsEmpty())
			mission.m_sGunShopSellerAssetId = string.Format("asset_%1_%2_%3", mission.m_sInstanceId, ROLE_GUN_SHOP_SELLER, 0);

		bool changed = AddMissionAsset(state, mission, ASSET_KIND_CHARACTER, ROLE_GUN_SHOP_SELLER, PROP_GUN_SHOP_SELLER, sellerPosition, sellerPosition, 0);
		HST_MissionAssetState seller = state.FindMissionAsset(mission.m_sGunShopSellerAssetId);
		if (seller && seller.m_iInteractionRadiusMeters != GUN_SHOP_SELLER_INTERACTION_RADIUS_METERS)
		{
			seller.m_iInteractionRadiusMeters = GUN_SHOP_SELLER_INTERACTION_RADIUS_METERS;
			changed = true;
		}

		return changed;
	}

	protected bool MarkGunShopSellerDeparted(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sGunShopSellerAssetId.IsEmpty())
			return false;

		HST_MissionAssetState seller = state.FindMissionAsset(mission.m_sGunShopSellerAssetId);
		if (!seller || seller.m_bDestroyed || seller.m_bDelivered)
			return false;

		seller.m_bDestroyed = true;
		seller.m_bAlive = false;
		seller.m_sLastInteraction = "shop_closed";
		DeleteRuntimeEntity(seller.m_sEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(seller.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_bDestroyed = true;

		return true;
	}

	protected bool EnsureGunShopDeliveryAssets(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_bGunShopDeliverySpawned)
			return false;

		vector deliveryStart = mission.m_vGunShopDeliveryPosition;
		if (IsZeroVector(deliveryStart))
			deliveryStart = ResolveGunShopDeliveryStartPosition(state, mission);
		mission.m_vGunShopDeliveryPosition = deliveryStart;

		string vehiclePrefab = SelectGunShopDeliveryVehiclePrefab(state, mission);
		bool changed = AddMissionAsset(state, mission, ASSET_KIND_VEHICLE, ROLE_GUN_SHOP_DELIVERY_VEHICLE, vehiclePrefab, deliveryStart, state.m_vHQPosition, 0);
		changed = AddMissionAsset(state, mission, ASSET_KIND_CHARACTER, ROLE_GUN_SHOP_DELIVERY_DRIVER, PROP_GUN_SHOP_DELIVERY_DRIVER, OffsetGunShopDriverPosition(deliveryStart), state.m_vHQPosition, 0) || changed;
		mission.m_sGunShopDeliveryVehicleAssetId = string.Format("asset_%1_%2_%3", mission.m_sInstanceId, ROLE_GUN_SHOP_DELIVERY_VEHICLE, 0);
		mission.m_sGunShopDeliveryDriverAssetId = string.Format("asset_%1_%2_%3", mission.m_sInstanceId, ROLE_GUN_SHOP_DELIVERY_DRIVER, 0);
		mission.m_bGunShopDeliverySpawned = true;
		mission.m_iGunShopDeliveryStartedAtSecond = state.m_iElapsedSeconds;
		mission.m_sRuntimePhase = "delivery_en_route";
		return true;
	}

	protected vector OffsetGunShopDriverPosition(vector position)
	{
		vector driverPosition = position;
		driverPosition[0] = driverPosition[0] + 2.0;
		return HST_WorldPositionService.ResolveSafeGroundPosition(driverPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 2.0);
	}

	protected vector ResolveGunShopDeliveryStartPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		vector hqPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true);
		vector anchor = ResolveClosestLivingPlayerPositionTo(hqPosition);
		if (IsZeroVector(anchor))
			anchor = hqPosition;

		float radius = Math.Max(450.0, HST_WorldPositionService.GetPlayerEventBubbleRadiusMeters() - GUN_SHOP_DELIVERY_SPAWN_EDGE_MARGIN_METERS);
		vector preferred = BuildGunShopEdgePosition(anchor, hqPosition, radius);
		vector roadPosition;
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		if (HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(preferred, GUN_SHOP_DELIVERY_ROAD_SEARCH_RADIUS_METERS, hqPosition, roadPosition, roadForward, roadWidth, roadDistance, roadReason))
			return roadPosition;

		vector resolved;
		if (HST_WorldPositionService.TryResolveVehicleSpawnPosition(preferred, resolved, true))
			return resolved;

		return HST_WorldPositionService.ResolveSafeGroundPosition(preferred, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true, 8.0);
	}

	protected vector BuildGunShopEdgePosition(vector anchor, vector target, float radius)
	{
		float dx = anchor[0] - target[0];
		float dz = anchor[2] - target[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length <= 1.0)
		{
			dx = 1.0;
			dz = 0.0;
			length = 1.0;
		}

		vector result = anchor;
		result[0] = anchor[0] + dx / length * radius;
		result[2] = anchor[2] + dz / length * radius;
		return result;
	}

	protected vector ResolveClosestLivingPlayerPositionTo(vector target)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return "0 0 0";

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		bool found;
		float bestDistanceSq = 999999999.0;
		vector bestPosition;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			float distanceSq = DistanceSq2D(playerEntity.GetOrigin(), target);
			if (found && distanceSq >= bestDistanceSq)
				continue;

			found = true;
			bestDistanceSq = distanceSq;
			bestPosition = playerEntity.GetOrigin();
		}

		if (!found)
			return "0 0 0";

		return bestPosition;
	}

	protected string SelectGunShopDeliveryVehiclePrefab(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		int seed = BuildGunShopSeed(state, mission, 1709);
		if (m_Balance)
		{
			for (int i = 0; i < m_Balance.m_aCivilianVehiclePrefabs.Count(); i++)
			{
				string candidate = m_Balance.m_aCivilianVehiclePrefabs[HST_DefaultCatalog.PositiveMod(seed + i, m_Balance.m_aCivilianVehiclePrefabs.Count())];
				if (IsGunShopDeliveryVehiclePrefab(candidate))
					return candidate;
			}
		}

		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (catalogManager)
		{
			array<SCR_EntityCatalog> catalogs = {};
			FactionKey key = "CIV";
			if (catalogManager.GetAllFactionEntityCatalogs(catalogs, key) > 0)
			{
				foreach (SCR_EntityCatalog catalog : catalogs)
				{
					if (!catalog)
						continue;

					array<SCR_EntityCatalogEntry> entries = {};
					catalog.GetEntityList(entries);
					for (int i = 0; i < entries.Count(); i++)
					{
						SCR_EntityCatalogEntry entry = entries[HST_DefaultCatalog.PositiveMod(seed + i, entries.Count())];
						if (entry && IsGunShopDeliveryVehiclePrefab(entry.GetPrefab()))
							return entry.GetPrefab();
					}
				}
			}
		}

		return PROP_CONVOY_VEHICLE;
	}

	protected bool IsGunShopDeliveryVehiclePrefab(string prefab)
	{
		if (prefab.IsEmpty() || !prefab.Contains("Prefabs/Vehicles/"))
			return false;
		if (prefab.Contains("Aircraft") || prefab.Contains("Airplane") || prefab.Contains("Plane") || prefab.Contains("Helicopter"))
			return false;

		Resource loaded = Resource.Load(prefab);
		return loaded && loaded.IsValid();
	}

	protected bool TrySeatGunShopDeliveryDriver(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sGunShopDeliveryDriverAssetId.IsEmpty() || mission.m_sGunShopDeliveryVehicleAssetId.IsEmpty())
			return false;

		HST_MissionAssetState driverAsset = state.FindMissionAsset(mission.m_sGunShopDeliveryDriverAssetId);
		HST_MissionAssetState vehicleAsset = state.FindMissionAsset(mission.m_sGunShopDeliveryVehicleAssetId);
		if (!driverAsset || !vehicleAsset)
			return false;
		if (driverAsset.m_bAttachedToCarrier && driverAsset.m_sCarriedByVehicleId == vehicleAsset.m_sEntityId && driverAsset.m_sLastInteraction == "driving")
			return false;

		IEntity driverEntity = GetRuntimeEntity(driverAsset.m_sEntityId);
		IEntity vehicleEntity = GetRuntimeEntity(vehicleAsset.m_sEntityId);
		if (!driverEntity || !vehicleEntity)
			return false;

		string reason;
		if (!TryMoveGunShopDriverIntoVehicle(driverEntity, vehicleEntity, reason))
			return false;

		driverAsset.m_sCarriedByVehicleId = vehicleAsset.m_sEntityId;
		driverAsset.m_bAttachedToCarrier = true;
		driverAsset.m_sLastInteraction = "driving";
		return true;
	}

	protected bool TryMoveGunShopDriverIntoVehicle(IEntity driverEntity, IEntity vehicleEntity, out string reason)
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
			reason = "driver has no compartment access";
			return false;
		}
		if (access.IsInCompartment() && access.GetVehicle() == vehicleEntity)
		{
			reason = "driver already seated";
			return true;
		}
		if (access.IsGettingIn())
		{
			reason = "driver already boarding";
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

	protected BaseCompartmentSlot FindAvailableDriverSlot(array<BaseCompartmentSlot> slots, IEntity driverEntity)
	{
		if (!slots || !driverEntity)
			return null;

		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;
			if (slot.GetType() != ECompartmentType.PILOT)
				continue;
			if (!slot.IsCompartmentAccessible())
				continue;
			if (slot.IsOccupied())
				continue;
			if (slot.IsGetInLockedFor(driverEntity))
				continue;

			return slot;
		}

		return null;
	}

	protected bool IssueGunShopDeliveryWaypoint(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sGunShopDeliveryDriverAssetId.IsEmpty())
			return false;
		if (mission.m_sRuntimePhase == "delivery_waypointed" || mission.m_sRuntimePhase == "delivered")
			return false;

		HST_MissionAssetState driverAsset = state.FindMissionAsset(mission.m_sGunShopDeliveryDriverAssetId);
		HST_MissionAssetState vehicleAsset = state.FindMissionAsset(mission.m_sGunShopDeliveryVehicleAssetId);
		if (!driverAsset || !vehicleAsset)
			return false;

		IEntity driverEntity = GetRuntimeEntity(driverAsset.m_sEntityId);
		IEntity vehicleEntity = GetRuntimeEntity(vehicleAsset.m_sEntityId);
		if (!driverEntity || !vehicleEntity)
			return false;

		AIGroup group = ResolveGunShopDeliveryAIGroup(driverEntity, state.m_vHQPosition);
		if (!group)
			return false;

		SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
		if (utility)
		{
			IEntity usageOwner = vehicleEntity;
			SCR_AIVehicleUsageComponent vehicleUsage = SCR_AIVehicleUsageComponent.FindOnNearestParent(vehicleEntity, usageOwner);
			if (vehicleUsage && vehicleUsage.IsVehicleTypeValid() && vehicleUsage.CanBePiloted())
				utility.AddUsableVehicle(vehicleUsage);
		}

		GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(CAPTIVE_MOVE_WAYPOINT_PREFAB, state.m_vHQPosition, "0 0 0");
		AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
		if (!waypoint)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
			return false;
		}

		waypoint.SetCompletionRadius(GUN_SHOP_DELIVERY_ARRIVAL_RADIUS_METERS);
		group.AddWaypoint(waypoint);
		mission.m_sRuntimePhase = "delivery_waypointed";
		return true;
	}

	protected AIGroup ResolveGunShopDeliveryAIGroup(IEntity driverEntity, vector position)
	{
		if (!driverEntity)
			return null;

		AIControlComponent control = AIControlComponent.Cast(driverEntity.FindComponent(AIControlComponent));
		if (!control)
			return null;

		control.ActivateAI();
		AIAgent agent = control.GetControlAIAgent();
		if (!agent)
			return null;

		AIGroup group = agent.GetParentGroup();
		if (group)
		{
			group.ActivateAllMembers();
			return group;
		}

		GenericEntity groupEntity = HST_WorldPositionService.SpawnPrefab(PROP_RUNTIME_EMPTY_GROUP, position, "0 0 0");
		group = AIGroup.Cast(groupEntity);
		if (!group)
			return null;

		SCR_AIGroup scrGroup = SCR_AIGroup.Cast(group);
		if (scrGroup)
			scrGroup.InitFactionKey("CIV");
		group.AddAgent(agent);
		group.ActivateAllMembers();
		return group;
	}

	protected bool AdvanceGunShopDeliveryVehicle(HST_CampaignState state, HST_ActiveMissionState mission, int elapsedSeconds)
	{
		if (!state || !mission || mission.m_sGunShopDeliveryVehicleAssetId.IsEmpty())
			return false;

		HST_MissionAssetState vehicleAsset = state.FindMissionAsset(mission.m_sGunShopDeliveryVehicleAssetId);
		if (!vehicleAsset || vehicleAsset.m_bDelivered || vehicleAsset.m_bDestroyed)
			return false;

		vector current = vehicleAsset.m_vCurrentPosition;
		IEntity vehicleEntity = GetRuntimeEntity(vehicleAsset.m_sEntityId);
		if (vehicleEntity)
			current = vehicleEntity.GetOrigin();
		if (IsZeroVector(current))
			current = mission.m_vGunShopDeliveryPosition;

		vector hqPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true);
		float distanceSq = DistanceSq2D(current, hqPosition);
		if (distanceSq <= GUN_SHOP_DELIVERY_ARRIVAL_RADIUS_METERS * GUN_SHOP_DELIVERY_ARRIVAL_RADIUS_METERS)
			return CompleteGunShopDelivery(state, mission, hqPosition);

		float distance = Math.Sqrt(distanceSq);
		float step = Math.Max(1.0, elapsedSeconds) * GUN_SHOP_DELIVERY_SPEED_METERS_PER_SECOND;
		float t = Math.Min(1.0, step / distance);
		vector next = InterpolatePosition(current, hqPosition, t);
		vector resolved;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(next, resolved, true))
			resolved = HST_WorldPositionService.ResolveGroundPosition(next, HST_WorldPositionService.VEHICLE_GROUND_OFFSET, true);

		vector angles = BuildVehicleAnglesToward(resolved, hqPosition);
		vehicleAsset.m_vCurrentPosition = resolved;
		vehicleAsset.m_vLastKnownPosition = resolved;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(vehicleAsset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_vPosition = resolved;
			runtimeEntity.m_vAngles = angles;
		}

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(vehicleAsset.m_sEntityId);
		if (runtimeVehicle)
		{
			runtimeVehicle.m_vPosition = resolved;
			runtimeVehicle.m_vAngles = angles;
		}

		if (vehicleEntity)
			HST_WorldPositionService.ApplyUprightEntityTransform(vehicleEntity, resolved, angles);
		UpdateGunShopDriverPosition(state, mission, resolved);

		if (DistanceSq2D(resolved, hqPosition) <= GUN_SHOP_DELIVERY_ARRIVAL_RADIUS_METERS * GUN_SHOP_DELIVERY_ARRIVAL_RADIUS_METERS)
		{
			CompleteGunShopDelivery(state, mission, hqPosition);
			return true;
		}

		return true;
	}

	protected void UpdateGunShopDriverPosition(HST_CampaignState state, HST_ActiveMissionState mission, vector position)
	{
		if (!state || !mission || mission.m_sGunShopDeliveryDriverAssetId.IsEmpty())
			return;

		HST_MissionAssetState driverAsset = state.FindMissionAsset(mission.m_sGunShopDeliveryDriverAssetId);
		if (!driverAsset)
			return;

		driverAsset.m_vCurrentPosition = position;
		driverAsset.m_vLastKnownPosition = position;
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(driverAsset.m_sEntityId);
		if (runtimeEntity)
			runtimeEntity.m_vPosition = position;
	}

	protected bool CompleteGunShopDelivery(HST_CampaignState state, HST_ActiveMissionState mission, vector position)
	{
		if (!state || !mission || mission.m_bGunShopDeliveryArrived)
			return false;

		DepositGunShopPurchases(state, mission);
		CompleteGunShopObjectives(state, mission);
		MarkGunShopDeliveryAssetComplete(state, mission, mission.m_sGunShopDeliveryDriverAssetId, position);
		MarkGunShopDeliveryAssetComplete(state, mission, mission.m_sGunShopDeliveryVehicleAssetId, position);
		mission.m_bGunShopDeliveryArrived = true;
		mission.m_sRuntimePhase = "delivered";
		return true;
	}

	protected void DepositGunShopPurchases(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !mission.m_aGunShopItems)
			return;

		foreach (HST_GunShopItemState item : mission.m_aGunShopItems)
		{
			if (!item || item.m_iPurchasedCount <= 0 || item.m_sPrefab.IsEmpty())
				continue;

			if (m_Arsenal)
			{
				m_Arsenal.DepositItem(state, m_Balance, item.m_sPrefab, item.m_iPurchasedCount, item.m_sCategory, item.m_sDisplayName);
				continue;
			}

			HST_ArsenalItemState arsenalItem = state.FindArsenalItem(item.m_sPrefab);
			if (!arsenalItem)
			{
				arsenalItem = new HST_ArsenalItemState();
				arsenalItem.m_sPrefab = item.m_sPrefab;
				state.m_aArsenalItems.Insert(arsenalItem);
			}
			arsenalItem.m_sCategory = item.m_sCategory;
			arsenalItem.m_sDisplayName = item.m_sDisplayName;
			arsenalItem.m_iCount += item.m_iPurchasedCount;
		}
	}

	protected void CompleteGunShopObjectives(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			objective.m_iCurrentProgress = objective.m_iRequiredProgress;
			objective.m_iCurrentCount = Math.Max(1, objective.m_iRequiredCount);
			objective.m_bComplete = true;
			objective.m_bFailed = false;
		}
	}

	protected void MarkGunShopDeliveryAssetComplete(HST_CampaignState state, HST_ActiveMissionState mission, string assetId, vector position)
	{
		if (!state || !mission || assetId.IsEmpty())
			return;

		HST_MissionAssetState asset = state.FindMissionAsset(assetId);
		if (!asset)
			return;

		asset.m_bDelivered = true;
		asset.m_bDestroyed = false;
		asset.m_bPickedUp = true;
		asset.m_vCurrentPosition = position;
		asset.m_vLastKnownPosition = position;
		asset.m_sLastInteraction = "delivered";
		DeleteRuntimeEntity(asset.m_sEntityId);
		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (runtimeEntity)
		{
			runtimeEntity.m_bRecovered = true;
			runtimeEntity.m_vPosition = position;
		}
		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(asset.m_sEntityId);
		if (runtimeVehicle)
		{
			runtimeVehicle.m_bDeleted = true;
			runtimeVehicle.m_vPosition = position;
		}
	}

	protected void UpdateGunShopAssetDeadlines(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId)
				asset.m_iDeadlineSecond = mission.m_iActiveUntilSecond;
		}
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

	protected int BuildGunShopSeed(HST_CampaignState state, HST_ActiveMissionState mission, int salt)
	{
		int seed = salt;
		if (state)
			seed += state.m_iCampaignSeed * 31 + state.m_iElapsedSeconds * 7;
		if (mission)
			seed += mission.m_sInstanceId.Length() * 97 + mission.m_sMissionId.Length() * 53 + mission.m_sTargetZoneId.Length() * 17;

		return seed;
	}

	protected bool RepairActiveMissionRuntimeAfterRestore(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;

		bool changed;
		string resolvedPrimitive = PrimitiveForMissionId(mission.m_sMissionId, HST_EMissionCategory.HST_MISSION_DYNAMIC);
		if (resolvedPrimitive.IsEmpty())
			resolvedPrimitive = PRIMITIVE_ABSTRACT_FALLBACK;

		if (mission.m_sRuntimePrimitive.IsEmpty() || mission.m_sRuntimePrimitive == PRIMITIVE_ABSTRACT_FALLBACK)
		{
			mission.m_sRuntimePrimitive = resolvedPrimitive;
			changed = true;
		}

		if (mission.m_eRuntimeMode == HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_ABSTRACT && mission.m_sRuntimePrimitive != PRIMITIVE_ABSTRACT_FALLBACK)
		{
			mission.m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_STATE_MACHINE;
			changed = true;
		}

		if (mission.m_sRuntimeType.IsEmpty() || mission.m_sRuntimeType == PRIMITIVE_ABSTRACT_FALLBACK)
		{
			mission.m_sRuntimeType = mission.m_sRuntimePrimitive;
			changed = true;
		}

		if (mission.m_sRuntimePhase.IsEmpty() || mission.m_sRuntimePhase == PHASE_CREATED)
		{
			if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
				mission.m_sRuntimePhase = PHASE_CONVOY_STAGING;
			else
				mission.m_sRuntimePhase = PHASE_ACTIVE;
			changed = true;
		}

		if (mission.m_sRuntimeEntityId.IsEmpty())
		{
			mission.m_sRuntimeEntityId = string.Format("runtime_%1_%2", mission.m_sInstanceId, mission.m_sRuntimePrimitive);
			changed = true;
		}

		if (mission.m_iRuntimeStartedAtSecond <= 0)
		{
			if (mission.m_iStartedAtSecond > 0)
				mission.m_iRuntimeStartedAtSecond = mission.m_iStartedAtSecond;
			else
				mission.m_iRuntimeStartedAtSecond = state.m_iElapsedSeconds;
			changed = true;
		}

		if (mission.m_iRuntimeHoldSeconds <= 0)
		{
			mission.m_iRuntimeHoldSeconds = DEFAULT_HOLD_SECONDS;
			changed = true;
		}

		if (mission.m_bRuntimeCleanupComplete)
		{
			mission.m_bRuntimeCleanupComplete = false;
			changed = true;
		}

		changed = EnsureRestoredMissionAssetsFromState(state, mission) || changed;
		changed = CompleteLegacyRecoverCargoDeliveryObjectives(state, mission) || changed;
		LinkObjectivesToMissionAssets(state, mission);
		if (ShouldRunRestoredMissionCarrierRestorePass(state, mission))
			changed = EnsureRestoredMissionCarrierVehicles(state, mission) || changed;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			changed = EnsureMissionAssetRuntimeEntityRecord(state, mission, asset) || changed;
			if (IsPhysicalConvoyAsset(mission, asset))
				continue;
			if (!state.m_bRestoredFromPersistence)
				continue;
			if (IsTerminalMissionAssetState(asset))
				continue;
			if (asset.m_sEntityId.IsEmpty() || HasRuntimeEntity(asset.m_sEntityId))
				continue;

			if (asset.m_bSpawned)
			{
				asset.m_bSpawned = false;
				changed = true;
			}

			HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtimeEntity)
			{
				if (runtimeEntity.m_bSpawned)
				{
					runtimeEntity.m_bSpawned = false;
					changed = true;
				}
				runtimeEntity.m_vPosition = ResolveRestoredMissionAssetPosition(asset);
			}
		}

		RefreshMissionObjectivesFromAssets(state, mission);
		UpdateMissionCountersFromAssets(state, mission);
		return changed;
	}

	protected bool IsPhysicalConvoyAsset(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT)
			return false;

		return asset.m_sRole == ROLE_CONVOY_VEHICLE || asset.m_sRole == ROLE_CONVOY_PAYLOAD || asset.m_sRole == ROLE_CONVOY_CAPTIVE;
	}

	protected bool CompleteLegacyRecoverCargoDeliveryObjectives(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != PRIMITIVE_RECOVER_CARGO)
			return false;

		bool changed;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (objective.m_sTargetId != "hq_delivery" || objective.m_eType != HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
				continue;

			if (objective.m_bFailed)
			{
				objective.m_bFailed = false;
				changed = true;
			}
			if (!objective.m_bComplete)
			{
				objective.m_iCurrentProgress = objective.m_iRequiredProgress;
				objective.m_iCurrentCount = Math.Max(1, objective.m_iRequiredCount);
				objective.m_bComplete = true;
				changed = true;
			}
		}

		return changed;
	}

	protected bool EnsureRestoredMissionAssetsFromState(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive == PRIMITIVE_ABSTRACT_FALLBACK)
			return false;
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;

		vector targetPosition = ResolveRuntimePropPosition(state, mission);
		vector hqPosition = HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		int cargoCount = Math.Max(1, mission.m_iRequiredCargoCount);
		int captiveCount = Math.Max(1, mission.m_iRequiredCaptiveCount);
		int vehicleCount = Math.Max(1, mission.m_iRequiredVehicleCount);
		bool changed;

		if (mission.m_sRuntimePrimitive == PRIMITIVE_KILL_HVT)
			return AddMissionAsset(state, mission, ASSET_KIND_CHARACTER, ROLE_HVT, PROP_HVT, targetPosition, targetPosition, 0);

		if (mission.m_sRuntimePrimitive == PRIMITIVE_GUN_SHOP)
		{
			vector sellerPosition = targetPosition;
			if (!IsZeroVector(mission.m_vGunShopSellerPosition))
				sellerPosition = mission.m_vGunShopSellerPosition;
			else
				mission.m_vGunShopSellerPosition = sellerPosition;
			mission.m_vTargetPosition = sellerPosition;
			if (mission.m_sGunShopSellerAssetId.IsEmpty())
				mission.m_sGunShopSellerAssetId = string.Format("asset_%1_%2_%3", mission.m_sInstanceId, ROLE_GUN_SHOP_SELLER, 0);
			return AddMissionAsset(state, mission, ASSET_KIND_CHARACTER, ROLE_GUN_SHOP_SELLER, PROP_GUN_SHOP_SELLER, sellerPosition, sellerPosition, 0);
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_DESTROY_TARGET)
		{
			if (mission.m_sMissionId == "destroy_or_steal_armor")
			{
				string vehiclePrefab = ResolveMissionVehiclePrefab(state, mission);
				for (int i = 0; i < vehicleCount; i++)
					changed = AddMissionAsset(state, mission, ASSET_KIND_VEHICLE, ROLE_DESTROY_TARGET, vehiclePrefab, targetPosition, hqPosition, i) || changed;
				return changed;
			}

			return AddMissionAsset(state, mission, ASSET_KIND_TARGET, ROLE_DESTROY_TARGET, PROP_DESTROY_TARGET, targetPosition, targetPosition, 0);
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_HOLD_AREA || mission.m_sRuntimePrimitive == PRIMITIVE_CLEAR_AREA)
			return AddMissionAsset(state, mission, ASSET_KIND_AREA, ROLE_HOLD_MARKER, PROP_HOLD_MARKER, targetPosition, targetPosition, 0);

		if (mission.m_sRuntimePrimitive == PRIMITIVE_DELIVER_SUPPLIES)
		{
			for (int i = 0; i < cargoCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_CITY_SUPPLIES, PROP_CITY_SUPPLIES, hqPosition, targetPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_RECOVER_CARGO)
		{
			for (int i = 0; i < cargoCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CARGO, ROLE_LOGISTICS_CARGO, ResolveLogisticsCargoPrefab(mission), targetPosition, hqPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_RESCUE_EXTRACT)
		{
			for (int i = 0; i < captiveCount; i++)
				changed = AddMissionAsset(state, mission, ASSET_KIND_CAPTIVE, ROLE_CAPTIVE, PROP_CAPTIVES, targetPosition, hqPosition, i) || changed;
			return changed;
		}

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
			return EnsureConvoyMissionSpecificAssets(state, mission);

		return AddMissionAsset(state, mission, ASSET_KIND_AREA, ROLE_HOLD_MARKER, PROP_HOLD_MARKER, targetPosition, targetPosition, 0);
	}

	protected bool EnsureMissionAssetRuntimeEntityRecord(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!state || !mission || !asset)
			return false;
		if (IsTerminalMissionAssetState(asset))
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission)
			|| HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
			return false;

		bool changed;
		if (asset.m_sEntityId.IsEmpty())
		{
			asset.m_sEntityId = asset.m_sAssetId;
			changed = true;
		}
		if (asset.m_sEntityId.IsEmpty())
			return changed;

		HST_MissionRuntimeEntityState runtimeEntity = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtimeEntity)
		{
			runtimeEntity = new HST_MissionRuntimeEntityState();
			runtimeEntity.m_sRuntimeEntityId = asset.m_sEntityId;
			runtimeEntity.m_sMissionInstanceId = mission.m_sInstanceId;
			state.m_aMissionRuntimeEntities.Insert(runtimeEntity);
			changed = true;
		}

		vector position = ResolveRestoredMissionAssetPosition(asset);
		if (runtimeEntity.m_sMissionInstanceId.IsEmpty())
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
		if (DistanceSq2D(runtimeEntity.m_vPosition, position) > 1.0)
		{
			runtimeEntity.m_vPosition = position;
			changed = true;
		}

		runtimeEntity.m_bDestroyed = asset.m_bDestroyed;
		runtimeEntity.m_bRecovered = asset.m_bPickedUp || asset.m_bDelivered;
		return changed;
	}

	protected vector ResolveRestoredMissionAssetPosition(HST_MissionAssetState asset)
	{
		if (!asset)
			return "0 0 0";
		if (!IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;
		if (!IsZeroVector(asset.m_vLastKnownPosition))
			return asset.m_vLastKnownPosition;
		if (!IsZeroVector(asset.m_vSourcePosition))
			return asset.m_vSourcePosition;
		return asset.m_vTargetPosition;
	}

	protected bool CleanupMissionRuntime(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_bRuntimeCleanupComplete)
			return false;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return false;

		DeleteMissionRuntimeEntities(state, mission);
		DeleteRuntimeEntity(mission.m_sRuntimeEntityId);
		CleanupMissionRuntimeRecords(state, mission);
		CleanupMissionAssetRecords(state, mission);
		mission.m_bRuntimeSpawned = false;
		mission.m_bRuntimeCleanupComplete = true;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == mission.m_sInstanceId)
				objective.m_bCleanupComplete = true;
		}

		return true;
	}

	protected void DeleteMissionRuntimeEntities(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sInstanceId.IsEmpty())
			return;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return;

		foreach (HST_MissionRuntimeEntityState runtimeEntity : state.m_aMissionRuntimeEntities)
		{
			if (!runtimeEntity || runtimeEntity.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			HST_MissionAssetState runtimeAsset = FindMissionAssetByEntityId(state, runtimeEntity.m_sRuntimeEntityId);
			if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(runtimeAsset))
				continue;
			if (IsPreservedConvoyAssetAfterCrewElimination(state, mission, runtimeAsset))
				continue;

			DeleteRuntimeEntity(runtimeEntity.m_sRuntimeEntityId);
		}

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
				continue;
			if (IsPreservedConvoyAssetAfterCrewElimination(state, mission, asset))
				continue;

			DeleteRuntimeEntity(asset.m_sEntityId);
		}
	}

	protected bool HasRuntimeEntity(string runtimeEntityId)
	{
		return GetRuntimeEntity(runtimeEntityId) != null;
	}

	protected void DeleteRuntimeEntity(string runtimeEntityId)
	{
		if (runtimeEntityId.IsEmpty())
			return;

		for (int i = m_aRuntimeEntityIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeEntityIds[i] != runtimeEntityId)
				continue;

			if (i < m_aRuntimeEntities.Count())
			{
				IEntity entity = m_aRuntimeEntities[i];
				if (entity && !m_aBorrowedWorldEntities.Contains(entity))
					SCR_EntityHelper.DeleteEntityAndChildren(entity);
				ForgetBorrowedWorldEntity(entity);
				m_aRuntimeEntities.Remove(i);
			}
			m_aRuntimeEntityIds.Remove(i);
		}
	}

	protected void ForgetBorrowedWorldEntity(IEntity entity)
	{
		for (int i = m_aBorrowedWorldEntities.Count() - 1; i >= 0; i--)
		{
			if (m_aBorrowedWorldEntities[i] == entity)
				m_aBorrowedWorldEntities.Remove(i);
		}
	}

	protected void CleanupMissionRuntimeRecords(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sInstanceId.IsEmpty())
			return;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return;

		for (int i = state.m_aMissionRuntimeEntities.Count() - 1; i >= 0; i--)
		{
			HST_MissionRuntimeEntityState runtimeEntity = state.m_aMissionRuntimeEntities[i];
			if (!runtimeEntity || runtimeEntity.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			HST_MissionAssetState runtimeAsset = FindMissionAssetByEntityId(state, runtimeEntity.m_sRuntimeEntityId);
			if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(runtimeAsset))
				continue;
			if (IsPreservedConvoyAssetAfterCrewElimination(state, mission, runtimeAsset))
				continue;
			if (IsExactMissionConvoyDurableAuthorityAsset(state, mission, runtimeAsset))
			{
				// The entity handle is process-local, but the exact asset row is part
				// of the settlement/restore aggregate and must outlive mission cleanup.
				if (runtimeAsset.m_sRole == ROLE_CONVOY_PAYLOAD || runtimeAsset.m_sRole == ROLE_CONVOY_CAPTIVE)
				{
					runtimeEntity.m_bSpawned = false;
					if (mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION)
					{
						runtimeAsset.m_bSpawned = false;
						runtimeAsset.m_bAttachedToCarrier = false;
						runtimeAsset.m_sCarriedByVehicleId = "";
					}
				}
				runtimeEntity.m_vPosition = runtimeAsset.m_vCurrentPosition;
				continue;
			}

			runtimeEntity.m_bDestroyed = true;
			state.m_aMissionRuntimeEntities.Remove(i);
		}
	}

	protected void CleanupMissionAssetRecords(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sInstanceId.IsEmpty())
			return;
		if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedMission(mission))
			return;

		for (int i = state.m_aMissionAssets.Count() - 1; i >= 0; i--)
		{
			HST_MissionAssetState asset = state.m_aMissionAssets[i];
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (HST_RadioSiteLifecycleService.IsManagedOrQuarantinedAsset(asset))
				continue;
			if (IsPreservedConvoyAssetAfterCrewElimination(state, mission, asset))
				continue;
			if (IsExactMissionConvoyDurableAuthorityAsset(state, mission, asset))
				continue;
			if (IsExactRescueDurableAuthorityAsset(mission, asset))
				continue;

			asset.m_bDestroyed = asset.m_bDestroyed || !asset.m_bDelivered;
			state.m_aMissionAssets.Remove(i);
		}
	}

	protected bool IsExactMissionConvoyDurableAuthorityAsset(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset || mission.m_sRuntimePrimitive != PRIMITIVE_CONVOY_INTERCEPT
			|| asset.m_sMissionInstanceId != mission.m_sInstanceId)
			return false;
		if (mission.m_iOperationContractVersion == HST_MissionConvoyOperationService.QUARANTINED_CONTRACT_VERSION)
			return true;
		if (!state || mission.m_iOperationContractVersion != HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION
			|| mission.m_sOperationId.IsEmpty() || mission.m_sManifestId.IsEmpty() || mission.m_sSpawnResultId.IsEmpty())
			return false;
		if (asset.m_sOperationId != mission.m_sOperationId || asset.m_sManifestId != mission.m_sManifestId
			|| asset.m_sManifestSlotId.IsEmpty() || asset.m_sConvoyElementId.IsEmpty())
			return false;

		HST_OperationRecordState operation = state.FindOperation(mission.m_sOperationId);
		HST_ForceManifestState manifest = state.FindForceManifest(mission.m_sManifestId);
		HST_ForceSpawnResultState batch = state.FindForceSpawnResult(mission.m_sSpawnResultId);
		return operation && manifest && batch
			&& operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
			&& operation.m_iContractVersion == HST_MissionConvoyOperationService.EXACT_CONTRACT_VERSION
			&& operation.m_sMissionInstanceId == mission.m_sInstanceId
			&& operation.m_sManifestId == mission.m_sManifestId
			&& operation.m_sSpawnResultId == mission.m_sSpawnResultId
			&& manifest.m_sOperationId == mission.m_sOperationId
			&& batch.m_sOperationId == mission.m_sOperationId
			&& batch.m_sManifestId == mission.m_sManifestId;
	}

	protected bool IsExactRescueDurableAuthorityAsset(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!mission || !asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
			return false;
		if (!HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return false;

		return asset.m_iRescueContractVersion != 0
			|| asset.m_eRescueDisposition != HST_ERescueCaptiveDisposition.HST_RESCUE_CAPTIVE_DISPOSITION_UNKNOWN;
	}

	protected HST_MissionAssetState FindMissionAssetByEntityId(HST_CampaignState state, string entityId)
	{
		if (!state || entityId.IsEmpty())
			return null;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sEntityId == entityId)
				return asset;
		}

		return null;
	}

	protected bool IsRuntimeEntityDestroyed(string runtimeEntityId)
	{
		IEntity entity = GetRuntimeEntity(runtimeEntityId);
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return damageManager && damageManager.GetState() == EDamageState.DESTROYED;
	}

	protected IEntity GetRuntimeEntity(string runtimeEntityId)
	{
		if (runtimeEntityId.IsEmpty())
			return null;

		IEntity resolved;
		for (int i = m_aRuntimeEntityIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeEntityIds[i] != runtimeEntityId)
				continue;

			if (i >= m_aRuntimeEntities.Count())
			{
				m_aRuntimeEntityIds.Remove(i);
				continue;
			}

			IEntity entity = m_aRuntimeEntities[i];
			if (!entity)
			{
				ForgetBorrowedWorldEntity(entity);
				m_aRuntimeEntityIds.Remove(i);
				m_aRuntimeEntities.Remove(i);
				continue;
			}

			if (!resolved)
			{
				resolved = entity;
				continue;
			}

			if (!m_aBorrowedWorldEntities.Contains(entity))
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			ForgetBorrowedWorldEntity(entity);
			m_aRuntimeEntityIds.Remove(i);
			m_aRuntimeEntities.Remove(i);
		}

		return resolved;
	}

	protected int ResolveConvoyArrivalSeconds(HST_ActiveMissionState mission)
	{
		int duration = mission.m_iActiveUntilSecond - mission.m_iStartedAtSecond;
		if (duration <= 0)
			duration = 3600;

		return Math.Max(300, duration * 2 / 3);
	}

	protected bool AreRuntimeObjectivesComplete(HST_CampaignState state, string instanceId)
	{
		bool found;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != instanceId)
				continue;

			found = true;
			if (!objective.m_bComplete || objective.m_bFailed)
				return false;
		}

		return found;
	}

	protected void MarkRuntimeMissionFailed(HST_CampaignState state, HST_ActiveMissionState mission, string reason)
	{
		if (!state || !mission)
			return;

		mission.m_sRuntimePhase = PHASE_FAILED;
		mission.m_sRuntimeFailureReason = reason;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete)
				continue;

			objective.m_bFailed = true;
		}
	}

	protected void UpdateMissionCountersFromObjectives(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return;

		int cargo;
		int captives;
		int vehicles;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || !objective.m_bComplete)
				continue;

			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES)
				cargo += Math.Max(1, objective.m_iRequiredCount);
			if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
				captives += Math.Max(1, objective.m_iRequiredCount);
			if (objective.m_sTargetId.Contains("armor"))
				vehicles += Math.Max(1, objective.m_iRequiredCount);
		}

		mission.m_iRecoveredCargoCount = Math.Min(mission.m_iRequiredCargoCount, cargo);
		mission.m_iExtractedCaptiveCount = Math.Min(mission.m_iRequiredCaptiveCount, captives);
		mission.m_iCapturedVehicleCount = Math.Min(mission.m_iRequiredVehicleCount, vehicles);
	}

	protected void EnsureMissionHostileGroup(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionDefinition definition, HST_ActiveMissionState mission)
	{
		if (!state || !preset || !definition || !mission || mission.m_sTargetZoneId.IsEmpty())
			return;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT || definition.m_sMissionId == "dynamic_minor_city_task")
			return;
		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
			return;
		if (HST_MissionGuardOperationService.IsExactOrQuarantinedMission(mission))
			return;
		if (HST_RescuePOWOperationService.IsExactOrQuarantinedMission(mission))
			return;

		string groupId = "mission_group_" + mission.m_sInstanceId;
		if (state.FindActiveGroup(groupId))
			return;

		HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
		if (!zone)
			return;

		string factionKey = zone.m_sOwnerFactionKey;
		if (factionKey.IsEmpty() || factionKey == preset.m_sResistanceFactionKey)
			factionKey = preset.m_sOccupierFactionKey;

		HST_ForceCompositionResult composition = ComposeMissionGuardForce(state, preset, definition, mission);
		HST_GroupSpawnPlan groupPlan;
		if (composition)
			groupPlan = composition.GetPrimaryGroup();

		HST_ActiveGroupState group = new HST_ActiveGroupState();
		vector guardPosition = ResolveMissionGuardPosition(state, mission);
		group.m_sGroupId = groupId;
		group.m_sOperationId = HST_StableIdService.BuildOperationId("mission", mission.m_sInstanceId);
		group.m_sZoneId = zone.m_sZoneId;
		group.m_sFactionKey = factionKey;
		group.m_sMissionInstanceId = mission.m_sInstanceId;
		if (composition)
			m_ForceCompositions.ApplyCompositionToActiveGroup(group, composition);
		if (composition && composition.m_bSuccess && groupPlan)
			group.m_sPrefab = groupPlan.m_sPrefab;
		else
			group.m_sPrefab = SelectMissionGroupPrefab(state, zone, factionKey, definition);
		group.m_sRouteId = "";
		group.m_vSourcePosition = guardPosition;
		group.m_vTargetPosition = guardPosition;
		group.m_vPosition = HST_WorldPositionService.ResolveSafeGroundPosition(guardPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 4.0);
		group.m_sRuntimeStatus = "mission_guard";
		if (groupPlan)
			group.m_iInfantryCount = Math.Max(1, groupPlan.m_iManpower);
		else
			group.m_iInfantryCount = ResolveMissionGuardInfantryCount(state, definition);
		if (composition && composition.m_bSuccess)
			group.m_iVehicleCount = composition.m_iVehicleCount;
		else
			group.m_iVehicleCount = ResolveMissionGuardVehicleCount(definition);
		group.m_iOriginalInfantryCount = group.m_iInfantryCount;
		group.m_iOriginalVehicleCount = group.m_iVehicleCount;
		group.m_iLastSeenAliveCount = group.m_iInfantryCount + group.m_iVehicleCount;
		group.m_iSurvivorInfantryCount = group.m_iInfantryCount;
		group.m_iSurvivorVehicleCount = group.m_iVehicleCount;
		state.m_aActiveGroups.Insert(group);
	}

	HST_ForceCompositionResult ComposeMissionGuardForce(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionDefinition definition, HST_ActiveMissionState mission)
	{
		if (!m_ForceCompositions)
			m_ForceCompositions = new HST_ForceCompositionService();

		return m_ForceCompositions.Compose(state, preset, BuildMissionGuardForceRequest(state, preset, definition, mission));
	}

	protected HST_ForceRequest BuildMissionGuardForceRequest(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionDefinition definition, HST_ActiveMissionState mission)
	{
		HST_ForceRequest request = new HST_ForceRequest();
		if (!state || !preset || !definition || !mission)
			return request;

		HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
		string factionKey = preset.m_sOccupierFactionKey;
		if (zone && !zone.m_sOwnerFactionKey.IsEmpty() && zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
			factionKey = zone.m_sOwnerFactionKey;

		request.m_sRequestId = "mission_guard_" + mission.m_sInstanceId;
		request.m_sFactionKey = factionKey;
		request.m_sIntentId = HST_ForceCompositionService.INTENT_GARRISON;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			request.m_sIntentId = HST_ForceCompositionService.INTENT_CONVOY_GUARDS;
		else if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			request.m_sIntentId = HST_ForceCompositionService.INTENT_CHECKPOINT;
		request.m_sTargetZoneId = mission.m_sTargetZoneId;
		request.m_vTargetPosition = ResolveMissionGuardPosition(state, mission);
		request.m_iWarLevel = Math.Max(1, state.m_iWarLevel);
		request.m_iMinManpower = 2;
		request.m_iMaxManpower = ResolveMissionGuardInfantryCount(state, definition);
		request.m_iBudget = Math.Max(24, 24 + state.m_iWarLevel * 6 + ResolveMissionGuardVehicleCount(definition) * 14);
		request.m_bAllowInfantry = true;
		request.m_bAllowVehicles = ResolveMissionGuardVehicleCount(definition) > 0;
		request.m_bAllowArmedVehicles = request.m_bAllowVehicles && state.m_iWarLevel >= 5;
		request.m_bAllowLightArmor = request.m_bAllowVehicles && definition.m_sMissionId.Contains("armor");
		request.m_bExplain = true;
		request.m_sReason = "mission guard";
		return request;
	}

	protected vector ResolveMissionGuardPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		if (mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT)
		{
			foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
			{
				if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != ROLE_CONVOY_VEHICLE)
					continue;

				return OffsetMissionAssetPosition(asset.m_vSourcePosition, 3);
			}
		}

		return ResolveRuntimePropPosition(state, mission);
	}

	protected string SelectMissionGroupPrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, HST_MissionDefinition definition)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (!faction)
			return "";

		int seed = state.m_iCampaignSeed + state.m_iElapsedSeconds + zone.m_sZoneId.Length() * 17 + definition.m_sMissionId.Length() * 31;
		array<string> candidates = {};
		AppendUniqueGroupPrefabs(candidates, faction.m_aPatrolGroupPrefabs);
		AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
		return SelectRandomGroupPrefab(candidates, seed);
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

	protected string SelectRandomGroupPrefab(array<string> prefabs, int seed)
	{
		if (!prefabs || prefabs.Count() == 0)
			return "";

		return prefabs[HST_DefaultCatalog.PositiveMod(seed, prefabs.Count())];
	}

	protected int ResolveMissionGuardInfantryCount(HST_CampaignState state, HST_MissionDefinition definition)
	{
		int count = 3 + state.m_iWarLevel;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			count += 3;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			count += 2;
		if (definition.m_sMissionId == "assassinate_specops" || definition.m_sMissionId == "dynamic_defend_petros")
			count += 4;
		return Math.Min(12, count);
	}

	protected int ResolveMissionGuardVehicleCount(HST_MissionDefinition definition)
	{
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return 1;
		if (definition.m_sMissionId == "destroy_or_steal_armor")
			return 1;
		return 0;
	}

	protected int ResolveHoldSeconds(string primitive, HST_MissionDefinition definition)
	{
		if (primitive == PRIMITIVE_HOLD_AREA)
		{
			if (definition && definition.m_sMissionId == "dynamic_defend_petros")
				return 90;

			return DEFAULT_HOLD_SECONDS;
		}

		return 0;
	}

	protected int ResolveObjectiveHoldSeconds(HST_MissionObjectiveState objective, string primitive, HST_MissionDefinition definition)
	{
		if (objective && objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA)
			return ResolveHoldSeconds(primitive, definition);

		return 0;
	}

	protected bool IsAnyLivingPlayerNearObjective(vector position, float radiusMeters)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		float radiusSq = radiusMeters * radiusMeters;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			if (DistanceSq2D(playerEntity.GetOrigin(), position) <= radiusSq)
				return true;
		}

		return false;
	}

	protected bool HasHostileActiveGroupNear(HST_CampaignState state, HST_CampaignPreset preset, vector position, float radiusMeters)
	{
		if (!state)
			return false;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		HST_CombatPresenceResult presence = m_CombatPresence.QueryHostilePresenceNear(
			state,
			preset,
			resistanceFactionKey,
			position,
			radiusMeters);
		return !presence || !presence.m_bQueryValid || presence.m_bBlocksProgress;
	}

	protected bool HasUnresolvedClearAreaHostiles(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_MissionObjectiveState objective, float radiusMeters)
	{
		if (!state || !mission || !objective)
			return false;

		if (HasHostileActiveGroupNear(state, preset, objective.m_vPosition, radiusMeters))
			return true;

		HST_ZoneState targetZone = ResolveObjectiveTargetZone(state, mission, objective);
		if (!targetZone)
			return false;

		if (HasHostileActiveGroupForZone(state, preset, targetZone.m_sZoneId))
			return true;

		return HasPendingTargetZoneHostileProjection(state, preset, targetZone);
	}

	protected HST_ZoneState ResolveObjectiveTargetZone(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!state || !objective)
			return null;

		HST_ZoneState zone;
		if (!objective.m_sTargetZoneId.IsEmpty())
			zone = state.FindZone(objective.m_sTargetZoneId);
		if (!zone && mission && !mission.m_sTargetZoneId.IsEmpty())
			zone = state.FindZone(mission.m_sTargetZoneId);
		if (!zone && !objective.m_sTargetId.IsEmpty())
			zone = state.FindZone(objective.m_sTargetId);

		return zone;
	}

	protected bool HasHostileActiveGroupForZone(HST_CampaignState state, HST_CampaignPreset preset, string zoneId)
	{
		if (!state || zoneId.IsEmpty())
			return false;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
		{
			foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
			{
				if (!group || !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					group.m_sZoneId,
					zoneId))
					continue;
				zone = state.FindZone(group.m_sZoneId);
				if (zone)
					break;
			}
		}
		if (!zone)
			return false;
		HST_CombatPresenceResult presence = m_CombatPresence.QueryZoneHostilePresence(
			state,
			preset,
			resistanceFactionKey,
			zone,
			true);
		return !presence || !presence.m_bQueryValid || presence.m_bBlocksProgress;
	}

	protected bool HasPendingTargetZoneHostileProjection(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone)
	{
		if (!state || !targetZone)
			return false;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;
		if (targetZone.m_sOwnerFactionKey.IsEmpty() || targetZone.m_sOwnerFactionKey == resistanceFactionKey)
			return false;

		if (targetZone.m_iActiveInfantryCount > 0)
			return true;
		if (targetZone.m_bActive)
			return false;

		HST_GarrisonState garrison = state.FindGarrison(targetZone.m_sZoneId, targetZone.m_sOwnerFactionKey);
		if (!garrison)
			return false;

		return garrison.m_iInfantryCount > 0;
	}

	protected IEntity GetBestPlayerEntity(PlayerManager playerManager, int playerId)
	{
		if (!playerManager || playerId <= 0)
			return null;

		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (ChimeraCharacter.Cast(controlledEntity))
			return controlledEntity;

		IEntity mainEntity = SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
		if (ChimeraCharacter.Cast(mainEntity))
			return mainEntity;
		return null;
	}

	protected bool IsLivingPlayerEntity(IEntity entity)
	{
		if (!entity)
			return false;

		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (!character)
			return false;
		CharacterControllerComponent controller = character.GetCharacterController();
		return controller && controller.GetLifeState() == ECharacterLifeState.ALIVE
			&& !controller.IsUnconscious();
	}

	protected bool IsEntityInVehicle(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(entity.FindComponent(SCR_CompartmentAccessComponent));
		return access && access.IsInCompartment() && access.GetVehicle() != null;
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

		Print("Partisan mission runtime debug | " + message);
	}
}
