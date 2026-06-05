class HST_LootResult
{
	int m_iSourcesScanned;
	int m_iItemsSeen;
	int m_iItemsDeposited;
	int m_iItemsRemoved;
	int m_iItemsSkippedUnlocked;
	int m_iItemsSkippedBlocked;
	int m_iItemsSkippedFriendly;
	string m_sVehicleRuntimeId;
	string m_sVehicleDisplayName;
	ref array<string> m_aDepositedLines = {};

	string BuildSummary()
	{
		string target = "arsenal";
		if (!m_sVehicleRuntimeId.IsEmpty())
			target = "vehicle " + m_sVehicleDisplayName;

		string summary = string.Format("h-istasi loot | target %1 | scanned %2 source(s) | seen %3 item(s) | deposited %4 | removed %5", target, m_iSourcesScanned, m_iItemsSeen, m_iItemsDeposited, m_iItemsRemoved);
		summary = summary + string.Format(" | skipped unlocked %1 | blocked %2 | friendly/player %3", m_iItemsSkippedUnlocked, m_iItemsSkippedBlocked, m_iItemsSkippedFriendly);
		foreach (string line : m_aDepositedLines)
			summary = summary + "\n" + line;

		return summary;
	}
}

class HST_VehicleRootScanResult
{
	IEntity m_SelectedRoot;
	HST_RuntimeVehicleState m_RuntimeVehicle;
	int m_iCandidates;
	int m_iResolvedRoots;
	string m_sSelectedPrefab;
	string m_sSelectedRuntimeId;
	string m_sSelectedDisplayName;
	string m_sRejectReason = "not scanned";
	float m_fSelectedDistanceMeters;
	int m_iCargoEntries;

	bool HasTarget()
	{
		return m_SelectedRoot != null;
	}
}

class HST_LootService
{
	static const int GARAGE_CAPTURE_RADIUS_METERS = 24;
	protected ref array<IEntity> m_aScanEntities = {};

	HST_LootResult LootNearbyToArsenal(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ArsenalService arsenal, int playerId)
	{
		HST_LootResult result = new HST_LootResult();
		if (!state || !preset || !balance || !arsenal || playerId <= 0)
			return result;

		IEntity playerEntity = ResolveLivingPlayerEntity(playerId);
		if (!playerEntity)
		{
			result.m_aDepositedLines.Insert("no living player entity found for area loot");
			return result;
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return result;

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(playerEntity.GetOrigin(), balance.m_iLootRadiusMeters, AddLootCandidate, null, EQueryEntitiesFlags.ALL);
		foreach (IEntity source : m_aScanEntities)
		{
			if (!source || source == playerEntity)
				continue;

			SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(source.FindComponent(SCR_InventoryStorageManagerComponent));
			if (inventory)
			{
				if (!IsEligibleLootSource(source, state, preset, result))
					continue;

				result.m_iSourcesScanned++;
				CollectInventoryItems(source, inventory, state, balance, arsenal, result);
				continue;
			}

			if (!IsEligibleLooseLootEntity(source, playerEntity, null))
				continue;

			result.m_iSourcesScanned++;
			CollectLooseItemToArsenal(source, state, balance, arsenal, result);
		}

		if (result.m_iItemsDeposited == 0 && result.m_aDepositedLines.Count() == 0)
			result.m_aDepositedLines.Insert("no eligible loot found nearby");

		return result;
	}

	string CaptureNearbyVehicleToGarage(HST_CampaignState state, HST_CampaignPreset preset, HST_ArsenalService arsenal, int playerId)
	{
		if (!state || !preset || !arsenal || playerId <= 0)
			return "h-istasi garage | failed: service not ready";

		IEntity playerEntity = ResolveLivingPlayerEntity(playerId);
		if (!playerEntity)
			return "h-istasi garage | failed: no living player entity found";

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return "h-istasi garage | failed: world not ready";

		HST_VehicleRootScanResult scan = FindNearestVehicleRoot(playerEntity, GARAGE_CAPTURE_RADIUS_METERS, state, "garage capture");
		IEntity selectedVehicle = scan.m_SelectedRoot;

		if (!selectedVehicle)
			return string.Format("h-istasi garage | failed: no safe root vehicle nearby | candidates %1 | reason %2", scan.m_iCandidates, scan.m_sRejectReason);

		HST_GarageVehicleState vehicle = new HST_GarageVehicleState();
		vehicle.m_sVehicleId = string.Format("garage_%1_%2", state.m_iElapsedSeconds, state.m_aGarageVehicles.Count());
		vehicle.m_sPrefab = ResolveVehiclePrefabFromScan(selectedVehicle, scan);
		if (vehicle.m_sPrefab.IsEmpty() || !IsEligibleSelectedVehicleRoot(selectedVehicle, vehicle.m_sPrefab, scan))
			return "h-istasi garage | failed: nearest candidate was not a top-level vehicle";

		string selectedVehicleRuntimeId = ResolveVehicleRuntimeIdFromScan(selectedVehicle, scan);
		vehicle.m_sDisplayName = BuildVehicleDisplayNameFromScan(selectedVehicle, vehicle.m_sPrefab, scan);
		vehicle.m_sSourceZoneId = FindNearestZoneId(state, selectedVehicle.GetOrigin());
		vehicle.m_sSourceFactionKey = ResolveFactionKeyFromScan(selectedVehicle, scan);
		vehicle.m_iStoredAtSecond = state.m_iElapsedSeconds;
		vehicle.m_iRedeployCost = ResolveGarageRedeployCost(vehicle.m_sPrefab);
		vehicle.m_vPosition = selectedVehicle.GetOrigin();
		vehicle.m_vAngles = ResolveStoredVehicleAngles(selectedVehicle, vehicle.m_sPrefab);
		vehicle.m_fFuel = 1.0;
		vehicle.m_bArmed = IsLikelyArmedVehicle(vehicle.m_sPrefab);

		SCR_EntityHelper.DeleteEntityAndChildren(selectedVehicle);
		if (IsVehicleRootStillPresent(state, selectedVehicleRuntimeId, vehicle.m_vPosition))
		{
			state.m_sLastVehicleTargetStatus = "garage capture failed";
			state.m_sLastVehicleTargetReason = "selected root still present after delete";
			Print(string.Format("h-istasi garage | failed to despawn %1; no garage record kept | prefab %2", vehicle.m_sDisplayName, vehicle.m_sPrefab), LogLevel.WARNING);
			return string.Format("h-istasi garage | failed: %1 did not despawn, garage record was not kept", vehicle.m_sDisplayName);
		}

		if (!arsenal.StoreVehicle(state, vehicle))
			return "h-istasi garage | failed: vehicle despawned but garage record could not be stored";

		MarkRuntimeVehicleDeleted(state, selectedVehicleRuntimeId, vehicle.m_vPosition);
		state.m_sLastVehicleTargetStatus = string.Format("garage captured %1", vehicle.m_sDisplayName);
		state.m_sLastVehicleTargetReason = "deleted verified root vehicle then stored garage record";
		Print(string.Format("h-istasi garage | captured %1 into %2 and despawned verified root vehicle | prefab %3 | distance %4m", vehicle.m_sDisplayName, vehicle.m_sVehicleId, vehicle.m_sPrefab, scan.m_fSelectedDistanceMeters));
		return string.Format("h-istasi garage | captured %1 | complete", vehicle.m_sDisplayName);
	}

	HST_LootResult CollectNearbyLootToVehicle(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ArsenalService arsenal, int playerId, string vehicleRuntimeId = "")
	{
		HST_LootResult result = new HST_LootResult();
		if (!state || !preset || !balance || !arsenal || playerId <= 0)
			return result;

		if (!balance.m_bVehicleLootEnabled)
		{
			result.m_aDepositedLines.Insert("vehicle loot disabled by config");
			return result;
		}

		IEntity playerEntity = ResolveLivingPlayerEntity(playerId);
		if (!playerEntity)
		{
			result.m_aDepositedLines.Insert("no living player entity found for vehicle loot");
			return result;
		}

		HST_VehicleRootScanResult vehicleScan;
		if (!vehicleRuntimeId.IsEmpty())
			vehicleScan = FindLootVehicleScanByRuntimeId(playerEntity, balance.m_iVehicleLootRadiusMeters, vehicleRuntimeId, state);
		else
			vehicleScan = FindNearestVehicleRoot(playerEntity, balance.m_iVehicleLootRadiusMeters, state, "vehicle loot");

		IEntity vehicle = vehicleScan.m_SelectedRoot;
		if (!vehicle)
		{
			if (!vehicleRuntimeId.IsEmpty())
				result.m_aDepositedLines.Insert(string.Format("target vehicle not nearby or invalid | %1", state.m_sLastVehicleTargetReason));
			else
				result.m_aDepositedLines.Insert(string.Format("no eligible vehicle nearby | candidates %1 | reason %2", state.m_iLastVehicleTargetCandidates, state.m_sLastVehicleTargetReason));
			return result;
		}

		if (!IsPlayerAtVehicleRear(playerEntity, vehicle, balance.m_iVehicleLootRadiusMeters))
		{
			result.m_aDepositedLines.Insert("stand near the rear/load area of the vehicle to load loot");
			return result;
		}

		string vehicleId = ResolveVehicleRuntimeIdFromScan(vehicle, vehicleScan);
		string vehiclePrefab = ResolveVehiclePrefabFromScan(vehicle, vehicleScan);
		string vehicleName = BuildVehicleDisplayNameFromScan(vehicle, vehiclePrefab, vehicleScan);
		result.m_sVehicleRuntimeId = vehicleId;
		result.m_sVehicleDisplayName = vehicleName;
		state.m_sLastVehicleTargetStatus = string.Format("vehicle loot target %1", vehicleName);
		state.m_sLastVehicleTargetPrefab = vehiclePrefab;
		state.m_sLastVehicleTargetReason = "target selected; scanning nearby loot";

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return result;

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(vehicle.GetOrigin(), balance.m_iVehicleLootRadiusMeters, AddLootCandidate, null, EQueryEntitiesFlags.ALL);
		foreach (IEntity source : m_aScanEntities)
		{
			if (!source || source == playerEntity || source == vehicle)
				continue;

			if (balance.m_iVehicleLootMaxItemsPerAction > 0 && result.m_iItemsDeposited >= balance.m_iVehicleLootMaxItemsPerAction)
				break;

			SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(source.FindComponent(SCR_InventoryStorageManagerComponent));
			if (inventory)
			{
				if (!IsEligibleLootSource(source, state, preset, result))
					continue;

				result.m_iSourcesScanned++;
				CollectInventoryItemsToVehicle(source, inventory, vehicle, vehicleId, vehiclePrefab, vehicleName, state, balance, arsenal, result);
				continue;
			}

			if (!IsEligibleLooseLootEntity(source, playerEntity, vehicle))
				continue;

			result.m_iSourcesScanned++;
			CollectLooseItemToVehicle(source, vehicle, vehicleId, vehiclePrefab, vehicleName, state, balance, arsenal, result);
		}

		if (result.m_iItemsDeposited == 0 && result.m_aDepositedLines.Count() == 0)
			result.m_aDepositedLines.Insert("no eligible loot found near vehicle");

		state.m_iLastVehicleTargetCargoEntries = CountVehicleCargoEntries(state, vehicleId);
		Print(string.Format("h-istasi vehicle loot | target %1 | prefab %2 | scanned %3 | deposited %4 | cargo entries for vehicle %5", vehicleName, vehiclePrefab, result.m_iSourcesScanned, result.m_iItemsDeposited, state.m_iLastVehicleTargetCargoEntries));

		return result;
	}

	string UnloadNearestVehicleCargoToArsenal(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ArsenalService arsenal, int playerId, string vehicleRuntimeId = "")
	{
		if (!state || !preset || !balance || !arsenal || playerId <= 0)
			return "h-istasi vehicle loot | service not ready";

		IEntity playerEntity = ResolveLivingPlayerEntity(playerId);
		if (!playerEntity)
			return "h-istasi vehicle loot | no living player entity found";

		HST_VehicleRootScanResult vehicleScan;
		if (!vehicleRuntimeId.IsEmpty())
			vehicleScan = FindLootVehicleScanByRuntimeId(playerEntity, balance.m_iVehicleLootRadiusMeters, vehicleRuntimeId, state);
		else
			vehicleScan = FindNearestVehicleRoot(playerEntity, balance.m_iVehicleLootRadiusMeters, state, "vehicle unload");

		IEntity vehicle = vehicleScan.m_SelectedRoot;
		if (!vehicle)
		{
			if (!vehicleRuntimeId.IsEmpty())
				return string.Format("h-istasi vehicle loot | target vehicle not nearby or invalid | %1", state.m_sLastVehicleTargetReason);

			return string.Format("h-istasi vehicle loot | no eligible vehicle nearby | candidates %1 | reason %2", state.m_iLastVehicleTargetCandidates, state.m_sLastVehicleTargetReason);
		}

		if (!IsPlayerAtVehicleRear(playerEntity, vehicle, balance.m_iVehicleLootRadiusMeters))
			return "h-istasi vehicle loot | stand near the rear/load area of the vehicle to unload cargo";

		string vehicleId = ResolveVehicleRuntimeIdFromScan(vehicle, vehicleScan);
		string vehiclePrefab = ResolveVehiclePrefabFromScan(vehicle, vehicleScan);
		string vehicleName = BuildVehicleDisplayNameFromScan(vehicle, vehiclePrefab, vehicleScan);
		int recoveredLegacyEntries = RekeyLegacyVehiclePartCargo(state, vehicle, vehicleId, vehiclePrefab, vehicleName);
		int deposited;
		int entries;
		for (int i = state.m_aVehicleCargoItems.Count() - 1; i >= 0; i--)
		{
			HST_VehicleCargoItemState cargoItem = state.m_aVehicleCargoItems[i];
			if (!cargoItem || cargoItem.m_sVehicleRuntimeId != vehicleId)
				continue;

			string cargoDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, cargoItem.m_sItemPrefab, cargoItem.m_sDisplayName);
			HST_ArsenalItemState depositedItem = arsenal.DepositItem(state, balance, cargoItem.m_sItemPrefab, cargoItem.m_iCount, cargoItem.m_sCategory, cargoDisplayName);
			if (depositedItem)
			{
				deposited += cargoItem.m_iCount;
				entries++;
				state.m_aVehicleCargoItems.Remove(i);
			}
		}

		if (deposited <= 0)
			return string.Format("h-istasi vehicle loot | no stored cargo found in nearest vehicle | recovered legacy entries %1", recoveredLegacyEntries);

		state.m_iLastVehicleTargetCargoEntries = CountVehicleCargoEntriesIncludingLegacy(state, vehicle);
		Print(string.Format("h-istasi vehicle loot | unloaded %1 item(s) from %2 cargo entries to arsenal | target %3 | prefab %4 | recovered legacy entries %5 | remaining cargo entries %6", deposited, entries, vehicleName, vehiclePrefab, recoveredLegacyEntries, state.m_iLastVehicleTargetCargoEntries));
		return string.Format("h-istasi vehicle loot | unloaded %1 item(s) from %2 cargo entries to arsenal | recovered legacy entries %3", deposited, entries, recoveredLegacyEntries);
	}

	string BuildVehicleCargoReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi vehicle cargo | campaign state not ready";

		string report = string.Format("h-istasi vehicle cargo | entries %1", state.m_aVehicleCargoItems.Count());
		foreach (HST_VehicleCargoItemState cargoItem : state.m_aVehicleCargoItems)
		{
			if (!cargoItem)
				continue;

			string vehicleLabel = HST_DisplayNameService.ResolveVehicleDisplayName(cargoItem.m_sVehiclePrefab, cargoItem.m_sVehicleDisplayName);
			string itemLabel = HST_DisplayNameService.ResolveItemDisplayName(null, cargoItem.m_sItemPrefab, cargoItem.m_sDisplayName);
			report = report + string.Format("\n%1 | %2 | %3 | count %4", vehicleLabel, itemLabel, cargoItem.m_sCategory, cargoItem.m_iCount);
		}

		return report;
	}

	protected int CountVehicleCargoEntries(HST_CampaignState state, string vehicleRuntimeId)
	{
		if (!state || vehicleRuntimeId.IsEmpty())
			return 0;

		int entries;
		foreach (HST_VehicleCargoItemState cargoItem : state.m_aVehicleCargoItems)
		{
			if (cargoItem && cargoItem.m_sVehicleRuntimeId == vehicleRuntimeId)
				entries++;
		}

		return entries;
	}

	protected int CountVehicleCargoEntriesIncludingLegacy(HST_CampaignState state, IEntity vehicle)
	{
		if (!state || !vehicle)
			return 0;

		string vehicleId = ResolveVehicleRuntimeId(vehicle);
		int entries = CountVehicleCargoEntries(state, vehicleId);
		entries += CountLegacyVehiclePartCargoNear(state, vehicle, vehicleId);
		return entries;
	}

	protected int CountLegacyVehiclePartCargoNear(HST_CampaignState state, IEntity vehicle, string vehicleId)
	{
		if (!state || !vehicle)
			return 0;

		int entries;
		foreach (HST_VehicleCargoItemState cargoItem : state.m_aVehicleCargoItems)
		{
			if (!cargoItem || cargoItem.m_sVehicleRuntimeId == vehicleId)
				continue;

			if (!IsLegacyVehiclePartCargoNear(cargoItem, vehicle))
				continue;

			entries++;
		}

		return entries;
	}

	protected int RekeyLegacyVehiclePartCargo(HST_CampaignState state, IEntity vehicle, string vehicleId, string vehiclePrefab, string vehicleName)
	{
		if (!state || !vehicle || vehicleId.IsEmpty())
			return 0;

		int recoveredEntries;
		for (int i = state.m_aVehicleCargoItems.Count() - 1; i >= 0; i--)
		{
			HST_VehicleCargoItemState cargoItem = state.m_aVehicleCargoItems[i];
			if (!cargoItem || cargoItem.m_sVehicleRuntimeId == vehicleId)
				continue;

			if (!IsLegacyVehiclePartCargoNear(cargoItem, vehicle))
				continue;

			HST_VehicleCargoItemState existing = state.FindVehicleCargoItem(vehicleId, cargoItem.m_sItemPrefab);
			if (existing && existing != cargoItem)
			{
				existing.m_iCount += cargoItem.m_iCount;
				existing.m_iLastStoredAtSecond = Math.Max(existing.m_iLastStoredAtSecond, cargoItem.m_iLastStoredAtSecond);
				existing.m_vLastVehiclePosition = vehicle.GetOrigin();
				if (existing.m_sDisplayName.IsEmpty())
					existing.m_sDisplayName = cargoItem.m_sDisplayName;
				if (existing.m_sCategory.IsEmpty())
					existing.m_sCategory = cargoItem.m_sCategory;
				state.m_aVehicleCargoItems.Remove(i);
			}
			else
			{
				cargoItem.m_sVehicleRuntimeId = vehicleId;
				cargoItem.m_sVehiclePrefab = vehiclePrefab;
				cargoItem.m_sVehicleDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(vehiclePrefab, vehicleName);
				cargoItem.m_vLastVehiclePosition = vehicle.GetOrigin();
			}

			recoveredEntries++;
		}

		if (recoveredEntries > 0)
			Print(string.Format("h-istasi vehicle loot | rekeyed %1 legacy vehicle-part cargo entries to root %2 | prefab %3", recoveredEntries, vehicleId, vehiclePrefab));

		return recoveredEntries;
	}

	protected bool IsLegacyVehiclePartCargoNear(HST_VehicleCargoItemState cargoItem, IEntity vehicle)
	{
		if (!cargoItem || !vehicle)
			return false;

		if (!HST_VehicleRootPolicy.IsVehiclePartPrefab(cargoItem.m_sVehiclePrefab) && !HST_VehicleRootPolicy.IsVehiclePartName(cargoItem.m_sVehicleDisplayName))
			return false;

		vector cargoPosition = cargoItem.m_vLastVehiclePosition;
		if (IsZeroVector(cargoPosition))
			return false;

		return DistanceSq2D(cargoPosition, vehicle.GetOrigin()) <= 12 * 12;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected bool IsVehicleRootStillPresent(HST_CampaignState state, string vehicleRuntimeId, vector position)
	{
		if (vehicleRuntimeId.IsEmpty())
			return false;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(position, 8, AddLootCandidate, null, EQueryEntitiesFlags.ALL);
		foreach (IEntity candidate : m_aScanEntities)
		{
			string rejectReason;
			HST_RuntimeVehicleState runtimeCandidate;
			IEntity rootVehicle = ResolveVehicleRootWithRuntimeFallback(state, candidate, runtimeCandidate, rejectReason);
			if (!rootVehicle)
				continue;

			if (ResolveVehicleRuntimeIdFromRecord(rootVehicle, runtimeCandidate) == vehicleRuntimeId)
				return true;
		}

		return false;
	}

	protected bool AddLootCandidate(IEntity entity)
	{
		if (entity)
			m_aScanEntities.Insert(entity);

		return true;
	}

	protected IEntity ResolveLivingPlayerEntity(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return null;

		IEntity playerEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (!playerEntity)
			playerEntity = SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);

		if (!IsLivingEntity(playerEntity))
			return null;

		return playerEntity;
	}

	protected bool IsEligibleLootSource(IEntity source, HST_CampaignState state, HST_CampaignPreset preset, HST_LootResult result)
	{
		if (!source)
			return false;

		string prefab = ResolvePrefabName(source);
		if (IsProtectedHQLootSource(state, source, prefab, result))
			return false;

		if (IsEligibleVehicleRoot(source, prefab))
		{
			result.m_iItemsSkippedBlocked++;
			result.m_aDepositedLines.Insert("skipped vehicle root loot source: " + BuildVehicleDisplayName(source, prefab));
			return false;
		}

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(source.FindComponent(FactionAffiliationComponent));
		if (factionComponent)
		{
			string factionKey = factionComponent.GetAffiliatedFactionKey();
			if (factionKey == preset.m_sResistanceFactionKey || factionKey == "FIA")
			{
				result.m_iItemsSkippedFriendly++;
				return false;
			}

			if (IsLivingEntity(source))
				return false;
		}

		return true;
	}

	protected bool IsProtectedHQLootSource(HST_CampaignState state, IEntity source, string prefab, HST_LootResult result)
	{
		string reason;
		if (!IsProtectedHQEntity(state, source, prefab, reason))
			return false;

		if (result)
		{
			result.m_iItemsSkippedBlocked++;
			result.m_aDepositedLines.Insert("skipped protected HQ source: " + reason);
		}

		return true;
	}

	protected bool IsProtectedHQEntity(HST_CampaignState state, IEntity entity, string prefab, out string reason)
	{
		reason = "";
		if (!state || !entity)
			return false;

		string name = entity.GetName();
		if (IsHQProtectedPrefab(prefab) || IsHQProtectedPrefab(name))
		{
			reason = BuildProtectedHQReason(entity, prefab, "protected prefab/name");
			return true;
		}

		vector origin = entity.GetOrigin();
		if (IsNearHQObject(origin, state.m_vPetrosPosition, 4.0))
		{
			reason = BuildProtectedHQReason(entity, prefab, "near Petros");
			return true;
		}

		if (IsNearHQObject(origin, state.m_vHQCachePosition, 5.0))
		{
			reason = BuildProtectedHQReason(entity, prefab, "near HQ cache");
			return true;
		}

		if (IsNearHQObject(origin, state.m_vArsenalPosition, 5.0))
		{
			reason = BuildProtectedHQReason(entity, prefab, "near HQ arsenal");
			return true;
		}

		if (IsNearHQObject(origin, state.m_vHQTentPosition, 5.0) && IsLikelyBuildOrSupplyObject(prefab))
		{
			reason = BuildProtectedHQReason(entity, prefab, "near HQ tent");
			return true;
		}

		return false;
	}

	protected bool IsHQProtectedPrefab(string prefabOrName)
	{
		if (prefabOrName.IsEmpty())
			return false;

		if (prefabOrName.Contains("HST_HQArsenal") || prefabOrName.Contains("Character_HST_Petros"))
			return true;

		if (prefabOrName.Contains("Arsenal") || prefabOrName.Contains("arsenal"))
			return true;

		if (prefabOrName.Contains("SupplyCache") || prefabOrName.Contains("Supply") || prefabOrName.Contains("Cache") || prefabOrName.Contains("TentSmallUS"))
			return true;

		if (prefabOrName.Contains("ArsenalBox_FIA") || prefabOrName.Contains("ArsenalBoxes/FIA"))
			return true;

		return false;
	}

	protected bool IsLikelyBuildOrSupplyObject(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		return prefab.Contains("Props/") || prefab.Contains("/Props") || prefab.Contains("Supply") || prefab.Contains("Cache") || prefab.Contains("Tent") || prefab.Contains("Box") || prefab.Contains("Crate");
	}

	protected bool IsNearHQObject(vector origin, vector hqObjectPosition, float radiusMeters)
	{
		if (IsZeroVector(hqObjectPosition))
			return false;

		return DistanceSq2D(origin, hqObjectPosition) <= radiusMeters * radiusMeters;
	}

	protected string BuildProtectedHQReason(IEntity entity, string prefab, string reason)
	{
		string label = reason;
		if (entity)
			label = label + " | " + entity.GetName();
		if (!prefab.IsEmpty())
			label = label + " | " + prefab;

		return label;
	}

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected void CollectInventoryItems(IEntity source, SCR_InventoryStorageManagerComponent inventory, HST_CampaignState state, HST_BalanceConfig balance, HST_ArsenalService arsenal, HST_LootResult result)
	{
		array<IEntity> items = {};
		inventory.GetItems(items, EStoragePurpose.PURPOSE_ANY);
		foreach (IEntity item : items)
		{
			if (!item)
				continue;

			result.m_iItemsSeen++;
			string prefab = ResolvePrefabName(item);
			if (prefab.IsEmpty())
			{
				result.m_iItemsSkippedBlocked++;
				continue;
			}

			string category = ClassifyItem(item, prefab);
			if (!IsAllowedByLootRules(prefab, category, balance))
			{
				result.m_iItemsSkippedBlocked++;
				continue;
			}

			if (balance.m_bLootOnlyLockedItems && arsenal.IsItemUnlocked(state, prefab))
			{
				result.m_iItemsSkippedUnlocked++;
				continue;
			}

			HST_ArsenalItemState deposited = arsenal.DepositItem(state, balance, prefab, 1, category, BuildDisplayName(item, prefab));
			if (!deposited)
				continue;

			result.m_iItemsDeposited++;
			result.m_aDepositedLines.Insert(string.Format("+ %1 | %2 | count %3 | unlocked %4", deposited.m_sDisplayName, deposited.m_sCategory, deposited.m_iCount, deposited.m_bUnlocked));
			if (balance.m_bRemoveLootedItems && RemoveLootedItem(inventory, item))
				result.m_iItemsRemoved++;
		}
	}

	protected void CollectInventoryItemsToVehicle(IEntity source, SCR_InventoryStorageManagerComponent inventory, IEntity vehicle, string vehicleId, string vehiclePrefab, string vehicleName, HST_CampaignState state, HST_BalanceConfig balance, HST_ArsenalService arsenal, HST_LootResult result)
	{
		array<IEntity> items = {};
		inventory.GetItems(items, EStoragePurpose.PURPOSE_ANY);
		foreach (IEntity item : items)
		{
			if (!item)
				continue;

			if (balance.m_iVehicleLootMaxItemsPerAction > 0 && result.m_iItemsDeposited >= balance.m_iVehicleLootMaxItemsPerAction)
				return;

			result.m_iItemsSeen++;
			string prefab = ResolvePrefabName(item);
			if (prefab.IsEmpty())
			{
				result.m_iItemsSkippedBlocked++;
				continue;
			}

			string category = ClassifyItem(item, prefab);
			if (!IsAllowedByLootRules(prefab, category, balance))
			{
				result.m_iItemsSkippedBlocked++;
				continue;
			}

			if (balance.m_bVehicleLootOnlyLockedItems && arsenal.IsItemUnlocked(state, prefab))
			{
				result.m_iItemsSkippedUnlocked++;
				continue;
			}

			HST_VehicleCargoItemState cargoItem = DepositVehicleCargo(state, vehicle, vehicleId, vehiclePrefab, vehicleName, prefab, category, BuildDisplayName(item, prefab));
			if (!cargoItem)
				continue;

			result.m_iItemsDeposited++;
			result.m_aDepositedLines.Insert(string.Format("+ %1 | %2 | vehicle count %3", cargoItem.m_sDisplayName, cargoItem.m_sCategory, cargoItem.m_iCount));
			if (balance.m_bVehicleLootRemoveSource && RemoveLootedItem(inventory, item))
				result.m_iItemsRemoved++;
		}
	}

	protected void CollectLooseItemToArsenal(IEntity item, HST_CampaignState state, HST_BalanceConfig balance, HST_ArsenalService arsenal, HST_LootResult result)
	{
		result.m_iItemsSeen++;
		string prefab = ResolvePrefabName(item);
		if (prefab.IsEmpty())
		{
			result.m_iItemsSkippedBlocked++;
			return;
		}

		string category = ClassifyItem(item, prefab);
		if (!IsAllowedByLootRules(prefab, category, balance))
		{
			result.m_iItemsSkippedBlocked++;
			return;
		}

		if (balance.m_bLootOnlyLockedItems && arsenal.IsItemUnlocked(state, prefab))
		{
			result.m_iItemsSkippedUnlocked++;
			return;
		}

		HST_ArsenalItemState deposited = arsenal.DepositItem(state, balance, prefab, 1, category, BuildDisplayName(item, prefab));
		if (!deposited)
			return;

		result.m_iItemsDeposited++;
		result.m_aDepositedLines.Insert(string.Format("+ %1 | %2 | count %3 | unlocked %4", deposited.m_sDisplayName, deposited.m_sCategory, deposited.m_iCount, deposited.m_bUnlocked));
		if (balance.m_bRemoveLootedItems && RemoveLooseLootItem(item))
			result.m_iItemsRemoved++;
	}

	protected void CollectLooseItemToVehicle(IEntity item, IEntity vehicle, string vehicleId, string vehiclePrefab, string vehicleName, HST_CampaignState state, HST_BalanceConfig balance, HST_ArsenalService arsenal, HST_LootResult result)
	{
		if (balance.m_iVehicleLootMaxItemsPerAction > 0 && result.m_iItemsDeposited >= balance.m_iVehicleLootMaxItemsPerAction)
			return;

		result.m_iItemsSeen++;
		string prefab = ResolvePrefabName(item);
		if (prefab.IsEmpty())
		{
			result.m_iItemsSkippedBlocked++;
			return;
		}

		string category = ClassifyItem(item, prefab);
		if (!IsAllowedByLootRules(prefab, category, balance))
		{
			result.m_iItemsSkippedBlocked++;
			return;
		}

		if (balance.m_bVehicleLootOnlyLockedItems && arsenal.IsItemUnlocked(state, prefab))
		{
			result.m_iItemsSkippedUnlocked++;
			return;
		}

		HST_VehicleCargoItemState cargoItem = DepositVehicleCargo(state, vehicle, vehicleId, vehiclePrefab, vehicleName, prefab, category, BuildDisplayName(item, prefab));
		if (!cargoItem)
			return;

		result.m_iItemsDeposited++;
		result.m_aDepositedLines.Insert(string.Format("+ %1 | %2 | vehicle count %3", cargoItem.m_sDisplayName, cargoItem.m_sCategory, cargoItem.m_iCount));
		if (balance.m_bVehicleLootRemoveSource && RemoveLooseLootItem(item))
			result.m_iItemsRemoved++;
	}

	protected HST_VehicleCargoItemState DepositVehicleCargo(HST_CampaignState state, IEntity vehicle, string vehicleId, string vehiclePrefab, string vehicleName, string itemPrefab, string category, string displayName)
	{
		if (!state || vehicleId.IsEmpty() || itemPrefab.IsEmpty())
			return null;

		HST_VehicleCargoItemState cargoItem = state.FindVehicleCargoItem(vehicleId, itemPrefab);
		if (!cargoItem)
		{
			cargoItem = new HST_VehicleCargoItemState();
			cargoItem.m_sVehicleRuntimeId = vehicleId;
			cargoItem.m_sItemPrefab = itemPrefab;
			state.m_aVehicleCargoItems.Insert(cargoItem);
		}

		cargoItem.m_sVehiclePrefab = vehiclePrefab;
		cargoItem.m_sVehicleDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(vehiclePrefab, vehicleName);
		cargoItem.m_sCategory = category;
		cargoItem.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, itemPrefab, displayName);
		cargoItem.m_iCount++;
		cargoItem.m_iLastStoredAtSecond = state.m_iElapsedSeconds;
		if (vehicle)
			cargoItem.m_vLastVehiclePosition = vehicle.GetOrigin();

		return cargoItem;
	}

	protected string ResolvePrefabName(IEntity item)
	{
		if (!item || !item.GetPrefabData())
			return "";

		return item.GetPrefabData().GetPrefabName();
	}

	protected bool IsEligibleGarageVehicle(IEntity entity, IEntity playerEntity, HST_CampaignPreset preset)
	{
		if (!entity || entity == playerEntity)
			return false;

		string prefab = ResolvePrefabName(entity);
		if (prefab.IsEmpty())
			return false;

		if (prefab.Contains("Character") || prefab.Contains("Inventory") || prefab.Contains("Magazine"))
			return false;

		if (!IsEligibleVehicleRoot(entity, prefab))
			return false;

		return true;
	}

	protected bool IsEligibleLooseLootEntity(IEntity entity, IEntity playerEntity, IEntity targetVehicle)
	{
		if (!entity || entity == playerEntity || entity == targetVehicle)
			return false;

		string prefab = ResolvePrefabName(entity);
		if (prefab.IsEmpty())
			return false;

		if (IsHQProtectedPrefab(prefab) || IsHQProtectedPrefab(entity.GetName()))
			return false;

		if (prefab.Contains("Character") || prefab.Contains("Vehicle") || prefab.Contains("Vehicles"))
			return false;

		if (entity.GetParent())
			return false;

		if (entity.FindComponent(SCR_InventoryStorageManagerComponent))
			return false;

		if (entity.FindComponent(BaseWeaponComponent) || entity.FindComponent(BaseMagazineComponent) || entity.FindComponent(GrenadeMoveComponent) || entity.FindComponent(InventoryItemComponent))
			return true;

		return prefab.Contains("Weapons") || prefab.Contains("Weapon") || prefab.Contains("Magazines") || prefab.Contains("Magazine") || prefab.Contains("Grenade") || prefab.Contains("Items") || prefab.Contains("Equipment");
	}

	protected IEntity FindNearestLootVehicle(IEntity playerEntity, int radiusMeters, HST_CampaignState state)
	{
		HST_VehicleRootScanResult scan = FindNearestVehicleRoot(playerEntity, radiusMeters, state, "vehicle loot");
		return scan.m_SelectedRoot;
	}

	protected IEntity FindLootVehicleByRuntimeId(IEntity playerEntity, int radiusMeters, string vehicleRuntimeId, HST_CampaignState state)
	{
		HST_VehicleRootScanResult scan = FindLootVehicleScanByRuntimeId(playerEntity, radiusMeters, vehicleRuntimeId, state);
		return scan.m_SelectedRoot;
	}

	protected HST_VehicleRootScanResult FindLootVehicleScanByRuntimeId(IEntity playerEntity, int radiusMeters, string vehicleRuntimeId, HST_CampaignState state)
	{
		if (!playerEntity || vehicleRuntimeId.IsEmpty())
		{
			HST_VehicleRootScanResult emptyResult = new HST_VehicleRootScanResult();
			emptyResult.m_sRejectReason = "missing player or vehicle runtime id";
			return emptyResult;
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			HST_VehicleRootScanResult emptyWorldResult = new HST_VehicleRootScanResult();
			emptyWorldResult.m_sRejectReason = "world not ready";
			return emptyWorldResult;
		}

		HST_VehicleRootScanResult scan = FindNearestVehicleRoot(playerEntity, radiusMeters, state, "vehicle loot id " + vehicleRuntimeId);
		if (scan.m_SelectedRoot && ResolveVehicleRuntimeIdFromScan(scan.m_SelectedRoot, scan) == vehicleRuntimeId)
			return scan;

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(playerEntity.GetOrigin(), radiusMeters, AddLootCandidate, null, EQueryEntitiesFlags.ALL);
		array<IEntity> checkedRoots = {};
		HST_VehicleRootScanResult result = new HST_VehicleRootScanResult();
		result.m_iCandidates = m_aScanEntities.Count();
		string lastReject = "target runtime id not found";
		foreach (IEntity candidate : m_aScanEntities)
		{
			string rejectReason;
			HST_RuntimeVehicleState runtimeCandidate;
			IEntity rootVehicle = ResolveVehicleRootWithRuntimeFallback(state, candidate, runtimeCandidate, rejectReason);
			if (!rootVehicle)
			{
				lastReject = rejectReason;
				continue;
			}

			if (checkedRoots.Find(rootVehicle) >= 0)
				continue;

			checkedRoots.Insert(rootVehicle);
			result.m_iResolvedRoots = checkedRoots.Count();
			string protectedReason;
			string rootPrefab = ResolveVehiclePrefabFromRecord(rootVehicle, runtimeCandidate);
			if (IsProtectedHQEntity(state, rootVehicle, rootPrefab, protectedReason))
			{
				lastReject = protectedReason;
				continue;
			}

			if (!IsEligibleLootVehicle(rootVehicle, playerEntity, runtimeCandidate))
			{
				lastReject = "resolved root failed loot vehicle eligibility";
				continue;
			}

			string resolvedRuntimeId = ResolveVehicleRuntimeIdFromRecord(rootVehicle, runtimeCandidate);
			if (resolvedRuntimeId == vehicleRuntimeId)
			{
				result.m_SelectedRoot = rootVehicle;
				result.m_RuntimeVehicle = runtimeCandidate;
				result.m_sSelectedRuntimeId = resolvedRuntimeId;
				result.m_sSelectedPrefab = rootPrefab;
				result.m_sSelectedDisplayName = BuildVehicleDisplayNameFromRecord(rootVehicle, rootPrefab, runtimeCandidate);
				result.m_sRejectReason = "matched runtime id";
				result.m_fSelectedDistanceMeters = Math.Sqrt(DistanceSq2D(playerEntity.GetOrigin(), rootVehicle.GetOrigin()));
				PublishVehicleTargetDiagnostics(state, "vehicle loot id target selected", result, DistanceSq2D(playerEntity.GetOrigin(), rootVehicle.GetOrigin()));
				return result;
			}
		}

		if (state)
		{
			state.m_iLastVehicleTargetCandidates = m_aScanEntities.Count();
			state.m_sLastVehicleTargetStatus = "vehicle target not found";
			state.m_sLastVehicleTargetPrefab = "";
			state.m_sLastVehicleTargetReason = lastReject;
			state.m_fLastVehicleTargetDistanceMeters = 0;
			state.m_iLastVehicleTargetCargoEntries = 0;
		}

		result.m_sRejectReason = lastReject;
		return result;
	}

	protected HST_VehicleRootScanResult FindNearestVehicleRoot(IEntity playerEntity, int radiusMeters, HST_CampaignState state, string purpose)
	{
		HST_VehicleRootScanResult result = new HST_VehicleRootScanResult();
		if (!playerEntity)
		{
			result.m_sRejectReason = "no player entity";
			PublishVehicleTargetDiagnostics(state, purpose + " failed", result, 0);
			return result;
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			result.m_sRejectReason = "world not ready";
			PublishVehicleTargetDiagnostics(state, purpose + " failed", result, 0);
			return result;
		}

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(playerEntity.GetOrigin(), radiusMeters, AddLootCandidate, null, EQueryEntitiesFlags.ALL);

		float bestDistanceSq = 999999999.0;
		float bestRegisteredDistanceSq = 999999999.0;
		bool selectedRegisteredRuntimeVehicle;
		array<IEntity> checkedRoots = {};
		foreach (IEntity candidate : m_aScanEntities)
		{
			result.m_iCandidates++;
			string rejectReason;
			HST_RuntimeVehicleState runtimeCandidate;
			IEntity rootVehicle = ResolveVehicleRootWithRuntimeFallback(state, candidate, runtimeCandidate, rejectReason);
			if (!rootVehicle)
			{
				if (!result.m_SelectedRoot)
					result.m_sRejectReason = rejectReason;
				continue;
			}

			if (checkedRoots.Find(rootVehicle) >= 0)
				continue;

			checkedRoots.Insert(rootVehicle);
			result.m_iResolvedRoots++;
			string protectedReason;
			string rootPrefab = ResolveVehiclePrefabFromRecord(rootVehicle, runtimeCandidate);
			if (IsProtectedHQEntity(state, rootVehicle, rootPrefab, protectedReason))
			{
				if (!result.m_SelectedRoot)
					result.m_sRejectReason = protectedReason;
				continue;
			}

			if (!IsEligibleLootVehicle(rootVehicle, playerEntity, runtimeCandidate))
			{
				if (!result.m_SelectedRoot)
					result.m_sRejectReason = "resolved root failed loot vehicle eligibility";
				continue;
			}

			float distanceSq = DistanceSq2D(playerEntity.GetOrigin(), rootVehicle.GetOrigin());
			HST_RuntimeVehicleState runtimeVehicle = runtimeCandidate;
			if (!runtimeVehicle)
				runtimeVehicle = ResolveRuntimeVehicleRecord(state, rootVehicle);

			if (runtimeVehicle)
			{
				runtimeVehicle.m_vPosition = rootVehicle.GetOrigin();
				runtimeVehicle.m_vAngles = rootVehicle.GetYawPitchRoll();
				if (distanceSq < bestRegisteredDistanceSq)
				{
					result.m_SelectedRoot = rootVehicle;
					result.m_RuntimeVehicle = runtimeVehicle;
					result.m_sSelectedRuntimeId = runtimeVehicle.m_sVehicleRuntimeId;
					result.m_sSelectedPrefab = ResolveVehiclePrefabFromRecord(rootVehicle, runtimeVehicle);
					result.m_sSelectedDisplayName = BuildVehicleDisplayNameFromRecord(rootVehicle, result.m_sSelectedPrefab, runtimeVehicle);
					result.m_sRejectReason = "selected registered h-istasi runtime vehicle";
					result.m_fSelectedDistanceMeters = Math.Sqrt(distanceSq);
					bestRegisteredDistanceSq = distanceSq;
					selectedRegisteredRuntimeVehicle = true;
				}

				continue;
			}

			if (!selectedRegisteredRuntimeVehicle && distanceSq < bestDistanceSq)
			{
				result.m_SelectedRoot = rootVehicle;
				result.m_RuntimeVehicle = null;
				result.m_sSelectedRuntimeId = ResolveVehicleRuntimeId(rootVehicle);
				result.m_sSelectedPrefab = rootPrefab;
				result.m_sSelectedDisplayName = BuildVehicleDisplayName(rootVehicle, rootPrefab);
				result.m_sRejectReason = "selected nearest eligible root";
				result.m_fSelectedDistanceMeters = Math.Sqrt(distanceSq);
				bestDistanceSq = distanceSq;
			}
		}

		if (!result.m_SelectedRoot && result.m_sRejectReason.IsEmpty())
			result.m_sRejectReason = "no vehicle prefab roots found";

		float selectedDistanceSq = bestDistanceSq;
		if (selectedRegisteredRuntimeVehicle)
			selectedDistanceSq = bestRegisteredDistanceSq;

		PublishVehicleTargetDiagnostics(state, purpose, result, selectedDistanceSq);
		return result;
	}

	protected void PublishVehicleTargetDiagnostics(HST_CampaignState state, string status, HST_VehicleRootScanResult scan, float distanceSq)
	{
		if (!state)
			return;

		IEntity vehicle;
		int candidates;
		int resolvedRoots;
		string reason = "not scanned";
		if (scan)
		{
			vehicle = scan.m_SelectedRoot;
			candidates = scan.m_iCandidates;
			resolvedRoots = scan.m_iResolvedRoots;
			reason = scan.m_sRejectReason;
		}

		state.m_sLastVehicleTargetStatus = status;
		state.m_sLastVehicleTargetReason = reason;
		state.m_iLastVehicleTargetCandidates = candidates;
		state.m_iLastVehicleTargetCargoEntries = 0;
		state.m_fLastVehicleTargetDistanceMeters = 0;
		state.m_sLastVehicleTargetPrefab = "";

		if (vehicle)
		{
			string prefab = ResolveVehiclePrefabFromScan(vehicle, scan);
			string vehicleId = ResolveVehicleRuntimeIdFromScan(vehicle, scan);
			state.m_sLastVehicleTargetPrefab = prefab;
			state.m_fLastVehicleTargetDistanceMeters = Math.Sqrt(distanceSq);
			state.m_iLastVehicleTargetCargoEntries = CountVehicleCargoEntries(state, vehicleId) + CountLegacyVehiclePartCargoNear(state, vehicle, vehicleId);
			Print(string.Format("h-istasi vehicle target | %1 | selected %2 | prefab %3 | id %4 | candidates %5 | roots %6 | distance %7m | reason %8", status, BuildVehicleDisplayNameFromScan(vehicle, prefab, scan), prefab, vehicleId, candidates, resolvedRoots, state.m_fLastVehicleTargetDistanceMeters, reason));
			return;
		}

		Print(string.Format("h-istasi vehicle target | %1 | no target | candidates %2 | roots %3 | reason %4", status, candidates, resolvedRoots, reason), LogLevel.WARNING);
	}

	protected bool IsPlayerAtVehicleRear(IEntity playerEntity, IEntity vehicle, int radiusMeters)
	{
		if (!playerEntity || !vehicle)
			return false;

		int gateMeters = Math.Max(3, Math.Min(radiusMeters, 8));
		return DistanceSq2D(playerEntity.GetOrigin(), vehicle.GetOrigin()) <= gateMeters * gateMeters;
	}

	protected bool IsEligibleLootVehicle(IEntity entity, IEntity playerEntity, HST_RuntimeVehicleState runtimeVehicle)
	{
		if (!entity || entity == playerEntity)
			return false;

		string prefab = ResolveVehiclePrefabFromRecord(entity, runtimeVehicle);
		if (prefab.IsEmpty())
			return false;

		if (prefab.Contains("Character") || prefab.Contains("Inventory") || prefab.Contains("Magazine"))
			return false;

		if (runtimeVehicle)
			return HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(runtimeVehicle.m_sPrefab);

		return IsEligibleVehicleRoot(entity, prefab);
	}

	protected IEntity ResolveVehicleRoot(IEntity entity, out string rejectReason)
	{
		rejectReason = "entity missing";
		if (!entity)
			return null;

		array<IEntity> chain = {};
		IEntity cursor = entity;
		for (int i = 0; i < 16; i++)
		{
			if (!cursor)
				break;

			chain.Insert(cursor);
			cursor = cursor.GetParent();
		}

		for (int topDown = chain.Count() - 1; topDown >= 0; topDown--)
		{
			IEntity candidate = chain[topDown];
			if (!candidate)
				continue;

			string candidatePrefab = ResolvePrefabName(candidate);
			if (IsEngineClassifiedVehicleRoot(candidate, candidatePrefab))
			{
				rejectReason = "resolved by base-game vehicle classification";
				return candidate;
			}

			if (candidatePrefab.IsEmpty())
				continue;

			if (IsEligibleVehicleRoot(candidate, candidatePrefab))
			{
				rejectReason = "resolved canonical vehicle root";
				return candidate;
			}

			rejectReason = BuildVehicleRootRejectReason(candidate, candidatePrefab);
		}

		if (rejectReason.IsEmpty() || rejectReason == "entity missing")
			rejectReason = "candidate chain had no eligible vehicle root prefab";

		return null;
	}

	protected IEntity ResolveVehicleRootWithRuntimeFallback(HST_CampaignState state, IEntity entity, out HST_RuntimeVehicleState runtimeVehicle, out string rejectReason)
	{
		runtimeVehicle = null;
		IEntity rootVehicle = ResolveVehicleRoot(entity, rejectReason);
		if (rootVehicle)
		{
			runtimeVehicle = ResolveRuntimeVehicleRecord(state, rootVehicle);
			return rootVehicle;
		}

		rootVehicle = ResolveRegisteredRuntimeVehicleRootFromCandidate(state, entity, runtimeVehicle, rejectReason);
		if (rootVehicle)
			return rootVehicle;

		return null;
	}

	protected IEntity ResolveRegisteredRuntimeVehicleRootFromCandidate(HST_CampaignState state, IEntity candidate, out HST_RuntimeVehicleState runtimeVehicle, out string rejectReason)
	{
		runtimeVehicle = null;
		rejectReason = "no registered h-istasi vehicle near candidate";
		if (!state || !candidate)
			return null;

		IEntity rootCandidate = ResolveTopmostEntity(candidate);
		if (!rootCandidate)
			return null;

		string rootPrefab = ResolvePrefabName(rootCandidate);
		if (!rootPrefab.IsEmpty() && (HST_VehicleRootPolicy.IsRejectedWorldPrefab(rootPrefab) || IsHQProtectedPrefab(rootPrefab)))
		{
			rejectReason = "runtime fallback rejected top-level scenery/prop near vehicle record";
			return null;
		}

		if (HST_VehicleRootPolicy.IsVehiclePartName(rootCandidate.GetName()))
		{
			rejectReason = "runtime fallback rejected top-level vehicle part";
			return null;
		}

		string rootRuntimeId = ResolveVehicleRuntimeId(rootCandidate);
		bool hasVehicleChainSignal = DoesCandidateChainHaveVehicleSignal(candidate);
		vector candidateOrigin = candidate.GetOrigin();
		vector rootOrigin = rootCandidate.GetOrigin();
		HST_RuntimeVehicleState bestRecord;
		float bestDistanceSq = 999999999.0;
		foreach (HST_RuntimeVehicleState record : state.m_aRuntimeVehicles)
		{
			if (!record || record.m_bDeleted || record.m_sVehicleRuntimeId.IsEmpty())
				continue;

			if (!HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(record.m_sPrefab))
				continue;

			if (!rootRuntimeId.IsEmpty() && rootRuntimeId == record.m_sVehicleRuntimeId)
			{
				runtimeVehicle = record;
				rejectReason = "resolved by matching registered h-istasi vehicle runtime id";
				return rootCandidate;
			}

			if (!hasVehicleChainSignal)
				continue;

			float rootDistanceSq = DistanceSq2D(record.m_vPosition, rootOrigin);
			float candidateDistanceSq = DistanceSq2D(record.m_vPosition, candidateOrigin);
			float distanceSq = rootDistanceSq;
			if (candidateDistanceSq < distanceSq)
				distanceSq = candidateDistanceSq;

			if (distanceSq > 12 * 12 || distanceSq >= bestDistanceSq)
				continue;

			bestRecord = record;
			bestDistanceSq = distanceSq;
		}

		if (!bestRecord)
			return null;

		runtimeVehicle = bestRecord;
		rejectReason = "resolved by registered h-istasi vehicle spawn record";
		return rootCandidate;
	}

	protected IEntity ResolveTopmostEntity(IEntity entity)
	{
		IEntity topmost = entity;
		IEntity cursor = entity;
		for (int i = 0; i < 16; i++)
		{
			if (!cursor)
				break;

			topmost = cursor;
			cursor = cursor.GetParent();
		}

		return topmost;
	}

	protected bool DoesCandidateChainHaveVehicleSignal(IEntity entity)
	{
		IEntity cursor = entity;
		for (int i = 0; i < 16; i++)
		{
			if (!cursor)
				break;

			string prefab = ResolvePrefabName(cursor);
			if (HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab) || HST_VehicleRootPolicy.IsVehiclePartPrefab(prefab) || HST_VehicleRootPolicy.IsVehiclePartName(cursor.GetName()))
				return true;

			if (IsEngineClassifiedVehicleRoot(cursor, prefab))
				return true;

			cursor = cursor.GetParent();
		}

		return false;
	}

	protected bool IsEligibleVehicleRoot(IEntity entity, string prefab)
	{
		if (!entity)
			return false;

		if (IsEngineClassifiedVehicleRoot(entity, prefab))
			return true;

		if (prefab.IsEmpty())
			return false;

		return HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab) && !HST_VehicleRootPolicy.IsVehiclePartName(entity.GetName());
	}

	protected bool IsEngineClassifiedVehicleRoot(IEntity entity, string prefab)
	{
		if (!entity)
			return false;

		string name = entity.GetName();
		if (HST_VehicleRootPolicy.IsVehiclePartName(name))
			return false;

		if (!prefab.IsEmpty())
		{
			if (HST_VehicleRootPolicy.IsVehiclePartPrefab(prefab) || HST_VehicleRootPolicy.IsRejectedWorldPrefab(prefab))
				return false;

			if (prefab.Contains("Arsenal") || prefab.Contains("arsenal") || prefab.Contains("Supply") || prefab.Contains("Cache") || prefab.Contains("Crate") || prefab.Contains("Box"))
				return false;
		}

		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
		if (editableEntity && editableEntity.GetEntityType() == EEditableEntityType.VEHICLE)
			return true;

		return false;
	}

	protected bool IsEligibleSelectedVehicleRoot(IEntity entity, string prefab, HST_VehicleRootScanResult scan)
	{
		if (!entity || prefab.IsEmpty())
			return false;

		if (scan && scan.m_RuntimeVehicle)
			return HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(scan.m_RuntimeVehicle.m_sPrefab);

		return IsEligibleVehicleRoot(entity, prefab);
	}

	protected string BuildVehicleRootRejectReason(IEntity entity, string prefab)
	{
		if (!entity)
			return "candidate missing";

		if (prefab.IsEmpty())
			return "candidate had no prefab";

		string policyReason = HST_VehicleRootPolicy.BuildRejectReason(entity, prefab);
		if (policyReason != "candidate passed prefab checks")
			return policyReason;

		return "candidate passed vehicle root checks";
	}

	protected HST_RuntimeVehicleState ResolveRuntimeVehicleRecord(HST_CampaignState state, IEntity entity)
	{
		if (!state || !entity)
			return null;

		string vehicleId = ResolveVehicleRuntimeId(entity);
		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(vehicleId);
		if (runtimeVehicle && !runtimeVehicle.m_bDeleted)
			return runtimeVehicle;

		string prefab = ResolvePrefabName(entity);
		vector origin = entity.GetOrigin();
		foreach (HST_RuntimeVehicleState candidate : state.m_aRuntimeVehicles)
		{
			if (!candidate || candidate.m_bDeleted || candidate.m_sPrefab != prefab)
				continue;

			if (DistanceSq2D(candidate.m_vPosition, origin) > 64.0)
				continue;

			return candidate;
		}

		return null;
	}

	protected void MarkRuntimeVehicleDeleted(HST_CampaignState state, string vehicleRuntimeId, vector deletedPosition)
	{
		if (!state || vehicleRuntimeId.IsEmpty())
			return;

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(vehicleRuntimeId);
		if (!runtimeVehicle)
			return;

		runtimeVehicle.m_bDeleted = true;
		runtimeVehicle.m_vPosition = deletedPosition;
	}

	protected bool IsLikelyVehicleRootPrefab(string prefab)
	{
		return HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab);
	}

	protected bool IsRejectedVehicleRootPrefab(string prefab)
	{
		return HST_VehicleRootPolicy.IsRejectedVehicleRootPrefab(prefab);
	}

	protected bool IsVehiclePartEntity(IEntity entity, string prefab)
	{
		if (HST_VehicleRootPolicy.IsVehiclePartPrefab(prefab))
			return true;

		if (!entity)
			return false;

		return HST_VehicleRootPolicy.IsVehiclePartName(entity.GetName());
	}

	protected bool IsVehiclePartName(string name)
	{
		return HST_VehicleRootPolicy.IsVehiclePartName(name);
	}

	protected string ResolveVehicleRuntimeId(IEntity vehicle)
	{
		if (!vehicle)
			return "";

		BaseRplComponent rpl = BaseRplComponent.Cast(vehicle.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("rpl_%1", rpl.Id());

		return string.Format("local_%1_%2", ResolvePrefabName(vehicle), vehicle.GetOrigin());
	}

	protected string ResolveVehicleRuntimeIdFromScan(IEntity vehicle, HST_VehicleRootScanResult scan)
	{
		if (scan && !scan.m_sSelectedRuntimeId.IsEmpty())
			return scan.m_sSelectedRuntimeId;

		if (scan && scan.m_RuntimeVehicle && !scan.m_RuntimeVehicle.m_sVehicleRuntimeId.IsEmpty())
			return scan.m_RuntimeVehicle.m_sVehicleRuntimeId;

		return ResolveVehicleRuntimeId(vehicle);
	}

	protected string ResolveVehicleRuntimeIdFromRecord(IEntity vehicle, HST_RuntimeVehicleState runtimeVehicle)
	{
		if (runtimeVehicle && !runtimeVehicle.m_sVehicleRuntimeId.IsEmpty())
			return runtimeVehicle.m_sVehicleRuntimeId;

		return ResolveVehicleRuntimeId(vehicle);
	}

	protected string ResolveVehiclePrefabFromScan(IEntity vehicle, HST_VehicleRootScanResult scan)
	{
		if (scan && !scan.m_sSelectedPrefab.IsEmpty())
			return scan.m_sSelectedPrefab;

		if (scan && scan.m_RuntimeVehicle && !scan.m_RuntimeVehicle.m_sPrefab.IsEmpty())
			return scan.m_RuntimeVehicle.m_sPrefab;

		return ResolvePrefabName(vehicle);
	}

	protected string ResolveVehiclePrefabFromRecord(IEntity vehicle, HST_RuntimeVehicleState runtimeVehicle)
	{
		if (runtimeVehicle && !runtimeVehicle.m_sPrefab.IsEmpty())
			return runtimeVehicle.m_sPrefab;

		return ResolvePrefabName(vehicle);
	}

	protected string ResolveFactionKey(IEntity entity)
	{
		if (!entity)
			return "";

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionComponent)
			return "";

		return factionComponent.GetAffiliatedFactionKey();
	}

	protected string ResolveFactionKeyFromScan(IEntity entity, HST_VehicleRootScanResult scan)
	{
		if (scan && scan.m_RuntimeVehicle && !scan.m_RuntimeVehicle.m_sFactionKey.IsEmpty())
			return scan.m_RuntimeVehicle.m_sFactionKey;

		return ResolveFactionKey(entity);
	}

	protected string BuildVehicleDisplayName(IEntity entity, string prefab)
	{
		if (!prefab.IsEmpty())
			return HST_DisplayNameService.ResolveVehicleDisplayName(prefab);

		if (entity)
			return entity.GetName();

		return "captured vehicle";
	}

	protected string BuildVehicleDisplayNameFromScan(IEntity entity, string prefab, HST_VehicleRootScanResult scan)
	{
		if (scan && !scan.m_sSelectedDisplayName.IsEmpty())
			return scan.m_sSelectedDisplayName;

		if (scan && scan.m_RuntimeVehicle && !scan.m_RuntimeVehicle.m_sDisplayName.IsEmpty())
			return HST_DisplayNameService.ResolveVehicleDisplayName(scan.m_RuntimeVehicle.m_sPrefab, scan.m_RuntimeVehicle.m_sDisplayName);

		return BuildVehicleDisplayName(entity, prefab);
	}

	protected string BuildVehicleDisplayNameFromRecord(IEntity entity, string prefab, HST_RuntimeVehicleState runtimeVehicle)
	{
		if (runtimeVehicle && !runtimeVehicle.m_sDisplayName.IsEmpty())
			return HST_DisplayNameService.ResolveVehicleDisplayName(runtimeVehicle.m_sPrefab, runtimeVehicle.m_sDisplayName);

		return BuildVehicleDisplayName(entity, prefab);
	}

	protected string FindNearestZoneId(HST_CampaignState state, vector position)
	{
		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			float distanceSq = DistanceSq2D(position, zone.m_vPosition);
			if (distanceSq < bestDistanceSq)
			{
				bestZone = zone;
				bestDistanceSq = distanceSq;
			}
		}

		if (bestZone)
			return bestZone.m_sZoneId;

		return "";
	}

	protected float DistanceSq2D(vector first, vector second)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz;
	}

	protected vector ResolveStoredVehicleAngles(IEntity vehicle, string prefab)
	{
		vector angles;
		if (!vehicle)
			return angles;

		angles = vehicle.GetYawPitchRoll();
		if (angles[0] != 0 || angles[1] != 0 || angles[2] != 0)
			return angles;

		vector origin = vehicle.GetOrigin();
		int seed = Math.Round(origin[0]) * 13 + Math.Round(origin[2]) * 17 + prefab.Length() * 23;
		if (seed < 0)
			seed = -seed;

		angles[0] = seed - (seed / 360) * 360;
		return angles;
	}

	protected int ResolveGarageRedeployCost(string prefab)
	{
		if (IsLikelyArmedVehicle(prefab))
			return 250;

		return 100;
	}

	protected bool IsLikelyArmedVehicle(string prefab)
	{
		return prefab.Contains("APC") || prefab.Contains("BTR") || prefab.Contains("BMP") || prefab.Contains("M2") || prefab.Contains("DShK") || prefab.Contains("PKM") || prefab.Contains("Armed") || prefab.Contains("armed");
	}

	protected string BuildDisplayName(IEntity item, string prefab)
	{
		return HST_DisplayNameService.ResolveItemDisplayName(item, prefab);
	}

	protected string ClassifyItem(IEntity item, string prefab)
	{
		if (item.FindComponent(BaseMagazineComponent))
			return "magazine";

		if (item.FindComponent(BaseWeaponComponent))
			return "weapon";

		if (item.FindComponent(GrenadeMoveComponent))
			return "explosive";

		if (prefab.Contains("Magazine") || prefab.Contains("magazine"))
			return "magazine";

		if (prefab.Contains("Grenade") || prefab.Contains("Mine") || prefab.Contains("Explosive"))
			return "explosive";

		if (prefab.Contains("Launcher") || prefab.Contains("RPG") || prefab.Contains("M72") || prefab.Contains("AT4"))
			return "launcher";

		return "equipment";
	}

	protected bool IsAllowedByLootRules(string prefab, string category, HST_BalanceConfig balance)
	{
		if (category == "explosive" && !balance.m_bAllowExplosiveUnlocks)
			return false;

		if (IsGuidedLauncher(prefab) && !balance.m_bAllowGuidedLauncherUnlocks)
			return false;

		return true;
	}

	protected bool IsGuidedLauncher(string prefab)
	{
		return prefab.Contains("ATGM") || prefab.Contains("Javelin") || prefab.Contains("Stinger") || prefab.Contains("Igla") || prefab.Contains("Metis") || prefab.Contains("guided") || prefab.Contains("Guided");
	}

	protected bool RemoveLootedItem(SCR_InventoryStorageManagerComponent inventory, IEntity item)
	{
		if (!inventory || !item)
			return false;

		if (inventory.TryDeleteItem(item))
			return true;

		BaseInventoryStorageComponent storage = inventory.FindStorageForItem(item, EStoragePurpose.PURPOSE_ANY);
		return storage && inventory.TryRemoveItemFromStorage(item, storage);
	}

	protected bool RemoveLooseLootItem(IEntity item)
	{
		if (!item)
			return false;

		SCR_EntityHelper.DeleteEntityAndChildren(item);
		return true;
	}
}
