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
	int m_iDirectRootHits;
	int m_iParentChainRootHits;
	int m_iBoundsRootHits;
	int m_iRuntimeFallbackHits;
	string m_sSelectedPrefab;
	string m_sSelectedRuntimeId;
	string m_sSelectedDisplayName;
	string m_sRejectReason = "not scanned";
	string m_sNearestCandidateDebug;
	string m_sNearestRejectReason;
	float m_fNearestRejectDistanceSq = 999999999.0;
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
	static const int PERSISTENT_FIELD_VEHICLE_SNAPSHOT_RADIUS_METERS = 40;
	static const int PERSISTENT_FIELD_VEHICLE_RESTORE_RADIUS_METERS = 12;
	static const int MAX_SCAN_ENTITIES = 384;
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

	int SnapshotNearbyPersistentVehicles(HST_CampaignState state, int radiusMeters = PERSISTENT_FIELD_VEHICLE_SNAPSHOT_RADIUS_METERS)
	{
		if (!state || radiusMeters <= 0)
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		array<string> snapshottedRuntimeIds = {};
		int snapshottedVehicles;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = playerManager.GetPlayerControlledEntity(playerId);
			if (!playerEntity)
				playerEntity = SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
			if (!IsLivingEntity(playerEntity))
				continue;

			snapshottedVehicles += SnapshotPersistentVehiclesNear(state, playerEntity.GetOrigin(), radiusMeters, playerEntity, snapshottedRuntimeIds);
		}

		if (snapshottedVehicles > 0)
			Print(string.Format("h-istasi vehicle persistence | snapshotted %1 nearby field vehicle(s)", snapshottedVehicles));
		return snapshottedVehicles;
	}

	int RestorePersistentFieldVehicles(HST_CampaignState state)
	{
		if (!state)
			return 0;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		int restoredVehicles;
		foreach (HST_RuntimeVehicleState record : state.m_aRuntimeVehicles)
		{
			if (!ShouldRestorePersistentFieldVehicle(record))
				continue;

			string restoredKind = record.m_sRuntimeKind;
			if (restoredKind.IsEmpty())
				restoredKind = "loot_vehicle";

			int scannedCandidates;
			int resolvedRoots;
			string rejectReason;
			IEntity liveRoot = ResolveRuntimeVehicleRootFromRecord(world, record, scannedCandidates, resolvedRoots, rejectReason);
			if (liveRoot)
			{
				HST_RuntimeVehicleState refreshedRecord = EnsureRuntimeVehicleRecord(state, liveRoot, record, record.m_sPrefab);
				if (refreshedRecord)
					refreshedRecord.m_sRuntimeKind = restoredKind;
				continue;
			}

			string previousRuntimeId = record.m_sVehicleRuntimeId;
			GenericEntity spawnedVehicle = HST_WorldPositionService.SpawnPrefab(record.m_sPrefab, record.m_vPosition, record.m_vAngles);
			if (!spawnedVehicle)
			{
				Print(string.Format("h-istasi vehicle persistence | restore failed for %1 | prefab %2 | position %3", record.m_sVehicleRuntimeId, record.m_sPrefab, record.m_vPosition), LogLevel.WARNING);
				continue;
			}

			HST_WorldPositionService.ApplyUprightEntityTransform(spawnedVehicle, record.m_vPosition, record.m_vAngles);
			HST_RuntimeVehicleState restoredRecord = EnsureRuntimeVehicleRecord(state, spawnedVehicle, record, record.m_sPrefab);
			if (restoredRecord)
			{
				restoredRecord.m_sRuntimeKind = restoredKind;
				restoredRecord.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
				restoredRecord.m_bDetached = false;
				restoredRecord.m_bDeleted = false;
				Print(string.Format("h-istasi vehicle persistence | restored field vehicle %1 | prefab %2 | old id %3 | new id %4", restoredRecord.m_sDisplayName, restoredRecord.m_sPrefab, previousRuntimeId, restoredRecord.m_sVehicleRuntimeId));
			}

			restoredVehicles++;
		}

		return restoredVehicles;
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
		vehicle.m_sVehicleId = BuildUniqueGarageVehicleId(state);
		vehicle.m_sPrefab = ResolveVehiclePrefabFromScan(selectedVehicle, scan);
		if (vehicle.m_sPrefab.IsEmpty() || !IsEligibleSelectedVehicleRoot(selectedVehicle, vehicle.m_sPrefab, scan))
			return "h-istasi garage | failed: nearest candidate was not a top-level vehicle";
		if (!IsLoadableVehiclePrefab(vehicle.m_sPrefab))
			return "h-istasi garage | failed: selected vehicle root has no loadable redeploy prefab | " + vehicle.m_sPrefab;

		string occupiedReason;
		if (IsVehicleOccupied(selectedVehicle, playerEntity, occupiedReason))
			return "h-istasi garage | failed: vehicle is occupied | " + occupiedReason;

		string selectedVehicleRuntimeId = ResolveVehicleRuntimeIdFromScan(selectedVehicle, scan);
		HST_RuntimeVehicleState runtimeVehicle = EnsureRuntimeVehicleRecord(state, selectedVehicle, scan.m_RuntimeVehicle, vehicle.m_sPrefab);
		if (runtimeVehicle)
			selectedVehicleRuntimeId = runtimeVehicle.m_sVehicleRuntimeId;

		vehicle.m_sDisplayName = BuildVehicleDisplayNameFromScan(selectedVehicle, vehicle.m_sPrefab, scan);
		vehicle.m_sSourceZoneId = FindNearestZoneId(state, selectedVehicle.GetOrigin());
		vehicle.m_sSourceFactionKey = ResolveFactionKeyFromScan(selectedVehicle, scan);
		vehicle.m_iStoredAtSecond = state.m_iElapsedSeconds;
		vehicle.m_iRedeployCost = ResolveGarageRedeployCost(vehicle.m_sPrefab);
		vehicle.m_vPosition = selectedVehicle.GetOrigin();
		vehicle.m_vAngles = ResolveStoredVehicleAngles(selectedVehicle, vehicle.m_sPrefab);
		vehicle.m_fFuel = 1.0;
		vehicle.m_sDamageState = ResolveDamageState(selectedVehicle);
		vehicle.m_bArmed = IsLikelyArmedVehicle(vehicle.m_sPrefab);
		HST_VehicleCapabilityPolicy.ApplyToGarageVehicle(vehicle);
		int recoveredLegacyEntries = RekeyLegacyVehiclePartCargo(state, selectedVehicle, selectedVehicleRuntimeId, vehicle.m_sPrefab, vehicle.m_sDisplayName);
		int physicalCargoCount = CaptureVehiclePhysicalCargo(selectedVehicle, vehicle);
		int virtualCargoCount = CopyVirtualVehicleCargoToGarage(state, selectedVehicleRuntimeId, vehicle);
		vehicle.m_bHadPhysicalCargo = physicalCargoCount > 0;

		if (!arsenal.StoreVehicle(state, vehicle))
		{
			state.m_sLastVehicleTargetStatus = "garage capture failed";
			state.m_sLastVehicleTargetReason = "garage record could not be stored before vehicle delete";
			return string.Format("h-istasi garage | failed: garage record could not be stored for %1", vehicle.m_sDisplayName);
		}

		SCR_EntityHelper.DeleteEntityAndChildren(selectedVehicle);
		if (IsVehicleRootStillPresent(state, selectedVehicleRuntimeId, vehicle.m_vPosition))
		{
			arsenal.RemoveVehicle(state, vehicle.m_sVehicleId);
			state.m_sLastVehicleTargetStatus = "garage capture failed";
			state.m_sLastVehicleTargetReason = "selected root still present after delete; garage record rolled back";
			Print(string.Format("h-istasi garage | failed to despawn %1; rolled back garage record | prefab %2", vehicle.m_sDisplayName, vehicle.m_sPrefab), LogLevel.WARNING);
			return string.Format("h-istasi garage | failed: %1 did not despawn, garage record was rolled back", vehicle.m_sDisplayName);
		}

		RemoveVirtualVehicleCargo(state, selectedVehicleRuntimeId);
		MarkRuntimeVehicleDeleted(state, selectedVehicleRuntimeId, vehicle.m_vPosition);
		state.m_sLastVehicleTargetStatus = string.Format("garage captured %1", vehicle.m_sDisplayName);
		state.m_sLastVehicleTargetReason = string.Format("stored garage record before delete; deleted verified root vehicle | physical cargo %1 | virtual cargo %2 | rekeyed legacy %3", physicalCargoCount, virtualCargoCount, recoveredLegacyEntries);
		Print(string.Format("h-istasi garage | captured %1 into %2 and despawned verified root vehicle | prefab %3 | distance %4m | cargo %5 entries/%6 item(s) | legacy %7", vehicle.m_sDisplayName, vehicle.m_sVehicleId, vehicle.m_sPrefab, scan.m_fSelectedDistanceMeters, vehicle.m_aStoredCargoItems.Count(), CountStoredVehicleCargoItems(vehicle), recoveredLegacyEntries));
		return string.Format("h-istasi garage | captured %1 | cargo %2 | complete", vehicle.m_sDisplayName, CountStoredVehicleCargoItems(vehicle));
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
			result.m_aDepositedLines.Insert("stand near the vehicle load area to load loot");
			return result;
		}

		string vehicleId = ResolveVehicleRuntimeIdFromScan(vehicle, vehicleScan);
		string vehiclePrefab = ResolveVehiclePrefabFromScan(vehicle, vehicleScan);
		string vehicleName = BuildVehicleDisplayNameFromScan(vehicle, vehiclePrefab, vehicleScan);
		HST_RuntimeVehicleState runtimeVehicle = EnsureRuntimeVehicleRecord(state, vehicle, vehicleScan.m_RuntimeVehicle, vehiclePrefab);
		if (runtimeVehicle)
		{
			vehicleId = runtimeVehicle.m_sVehicleRuntimeId;
			vehiclePrefab = runtimeVehicle.m_sPrefab;
			vehicleName = HST_DisplayNameService.ResolveVehicleDisplayName(vehiclePrefab, runtimeVehicle.m_sDisplayName);
		}

		RekeyLegacyVehiclePartCargo(state, vehicle, vehicleId, vehiclePrefab, vehicleName);
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
			return "h-istasi vehicle loot | stand near the vehicle load area to unload cargo";

		string vehicleId = ResolveVehicleRuntimeIdFromScan(vehicle, vehicleScan);
		string vehiclePrefab = ResolveVehiclePrefabFromScan(vehicle, vehicleScan);
		string vehicleName = BuildVehicleDisplayNameFromScan(vehicle, vehiclePrefab, vehicleScan);
		HST_RuntimeVehicleState runtimeVehicle = EnsureRuntimeVehicleRecord(state, vehicle, vehicleScan.m_RuntimeVehicle, vehiclePrefab);
		if (runtimeVehicle)
		{
			vehicleId = runtimeVehicle.m_sVehicleRuntimeId;
			vehiclePrefab = runtimeVehicle.m_sPrefab;
			vehicleName = HST_DisplayNameService.ResolveVehicleDisplayName(vehiclePrefab, runtimeVehicle.m_sDisplayName);
		}

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

	protected int RekeyVehicleCargoRuntimeId(HST_CampaignState state, string oldVehicleId, string newVehicleId, string vehiclePrefab, string vehicleName, vector vehiclePosition)
	{
		if (!state || oldVehicleId.IsEmpty() || newVehicleId.IsEmpty() || oldVehicleId == newVehicleId)
			return 0;

		int recoveredEntries;
		for (int i = state.m_aVehicleCargoItems.Count() - 1; i >= 0; i--)
		{
			HST_VehicleCargoItemState cargoItem = state.m_aVehicleCargoItems[i];
			if (!cargoItem || cargoItem.m_sVehicleRuntimeId != oldVehicleId)
				continue;

			HST_VehicleCargoItemState existing = state.FindVehicleCargoItem(newVehicleId, cargoItem.m_sItemPrefab);
			if (existing && existing != cargoItem)
			{
				existing.m_iCount += cargoItem.m_iCount;
				existing.m_iLastStoredAtSecond = Math.Max(existing.m_iLastStoredAtSecond, cargoItem.m_iLastStoredAtSecond);
				existing.m_vLastVehiclePosition = vehiclePosition;
				if (existing.m_sVehiclePrefab.IsEmpty())
					existing.m_sVehiclePrefab = vehiclePrefab;
				if (existing.m_sVehicleDisplayName.IsEmpty())
					existing.m_sVehicleDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(vehiclePrefab, vehicleName);
				if (existing.m_sDisplayName.IsEmpty())
					existing.m_sDisplayName = cargoItem.m_sDisplayName;
				if (existing.m_sCategory.IsEmpty())
					existing.m_sCategory = cargoItem.m_sCategory;
				state.m_aVehicleCargoItems.Remove(i);
			}
			else
			{
				cargoItem.m_sVehicleRuntimeId = newVehicleId;
				cargoItem.m_sVehiclePrefab = vehiclePrefab;
				cargoItem.m_sVehicleDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(vehiclePrefab, vehicleName);
				cargoItem.m_vLastVehiclePosition = vehiclePosition;
			}

			recoveredEntries++;
		}

		if (recoveredEntries > 0)
			Print(string.Format("h-istasi vehicle loot | rekeyed %1 cargo entries from %2 to root %3 | prefab %4", recoveredEntries, oldVehicleId, newVehicleId, vehiclePrefab));

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
			IEntity rootVehicle = ResolveVehicleRoot(candidate, rejectReason);
			if (!rootVehicle)
				continue;

			if (ResolveVehicleRuntimeId(rootVehicle) == vehicleRuntimeId)
				return true;
		}

		return false;
	}

	protected bool AddLootCandidate(IEntity entity)
	{
		if (entity && m_aScanEntities.Count() < MAX_SCAN_ENTITIES)
			m_aScanEntities.Insert(entity);

		return m_aScanEntities.Count() < MAX_SCAN_ENTITIES;
	}

	protected int SnapshotPersistentVehiclesNear(HST_CampaignState state, vector center, int radiusMeters, IEntity playerEntity, array<string> snapshottedRuntimeIds)
	{
		if (!state || !playerEntity || radiusMeters <= 0)
			return 0;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(center, radiusMeters, AddLootCandidate, null, EQueryEntitiesFlags.ALL);
		array<IEntity> checkedRoots = {};
		int snapshottedVehicles;
		foreach (IEntity candidate : m_aScanEntities)
		{
			if (!candidate || candidate == playerEntity)
				continue;

			string rejectReason;
			IEntity rootVehicle = ResolveVehicleRoot(candidate, rejectReason);
			if (!rootVehicle)
				rootVehicle = ResolveVehicleRootByNearbyBounds(candidate, state, rejectReason);
			if (!rootVehicle || checkedRoots.Find(rootVehicle) >= 0)
				continue;

			checkedRoots.Insert(rootVehicle);
			HST_RuntimeVehicleState runtimeCandidate = ResolveRuntimeVehicleRecord(state, rootVehicle);
			if (!CanAdoptPersistentFieldVehicle(runtimeCandidate))
				continue;

			string rootPrefab = ResolveVehiclePrefabFromCandidate(rootVehicle, runtimeCandidate, candidate);
			string protectedReason;
			if (IsProtectedHQEntity(state, rootVehicle, rootPrefab, protectedReason))
				continue;
			if (!IsEligibleLootVehicle(rootVehicle, playerEntity, runtimeCandidate, rootPrefab))
				continue;

			string runtimeId = ResolveVehicleRuntimeId(rootVehicle);
			if (runtimeId.IsEmpty())
				continue;
			if (snapshottedRuntimeIds && snapshottedRuntimeIds.Find(runtimeId) >= 0)
				continue;

			HST_RuntimeVehicleState record = EnsureRuntimeVehicleRecord(state, rootVehicle, runtimeCandidate, rootPrefab);
			if (!record)
				continue;

			if (record.m_sRuntimeKind.IsEmpty() || record.m_sRuntimeKind == "runtime_vehicle")
				record.m_sRuntimeKind = "loot_vehicle";
			record.m_bDetached = false;
			record.m_bDeleted = false;
			if (record.m_iSpawnedAtSecond <= 0)
				record.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
			if (snapshottedRuntimeIds && snapshottedRuntimeIds.Find(record.m_sVehicleRuntimeId) < 0)
				snapshottedRuntimeIds.Insert(record.m_sVehicleRuntimeId);
			snapshottedVehicles++;
		}

		return snapshottedVehicles;
	}

	protected bool CanAdoptPersistentFieldVehicle(HST_RuntimeVehicleState record)
	{
		if (!record)
			return true;

		return record.m_sRuntimeKind == "loot_vehicle"
			|| record.m_sRuntimeKind == "field_vehicle"
			|| record.m_sRuntimeKind == "garage_redeploy";
	}

	protected bool ShouldRestorePersistentFieldVehicle(HST_RuntimeVehicleState record)
	{
		if (!record || record.m_bDeleted || record.m_bDetached)
			return false;
		if (!CanAdoptPersistentFieldVehicle(record))
			return false;
		if (record.m_sVehicleRuntimeId.IsEmpty() || IsZeroVector(record.m_vPosition))
			return false;

		return HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(record.m_sPrefab);
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
		bool protectedByProximity = IsLikelyBuildOrSupplyObject(prefab) || IsHQProtectedPrefab(name);
		if (!protectedByProximity)
			return false;

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
		array<IEntity> visited = {};
		GatherInventoryItemsRecursive(inventory, items, visited, 0);
		foreach (IEntity item : items)
		{
			if (!item)
				continue;

			result.m_iItemsSeen++;
			string prefab;
			string category;
			string displayName;
			if (!TryResolveLootItemForDeposit(item, state, balance, arsenal, result, false, prefab, category, displayName))
				continue;

			HST_ArsenalItemState deposited = arsenal.DepositItem(state, balance, prefab, 1, category, displayName);
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
		array<IEntity> visited = {};
		GatherInventoryItemsRecursive(inventory, items, visited, 0);
		foreach (IEntity item : items)
		{
			if (!item)
				continue;

			if (balance.m_iVehicleLootMaxItemsPerAction > 0 && result.m_iItemsDeposited >= balance.m_iVehicleLootMaxItemsPerAction)
				return;

			result.m_iItemsSeen++;
			string prefab;
			string category;
			string displayName;
			if (!TryResolveLootItemForDeposit(item, state, balance, arsenal, result, true, prefab, category, displayName))
				continue;

			HST_VehicleCargoItemState cargoItem = DepositVehicleCargo(state, vehicle, vehicleId, vehiclePrefab, vehicleName, prefab, category, displayName);
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
		string prefab;
		string category;
		string displayName;
		if (!TryResolveLootItemForDeposit(item, state, balance, arsenal, result, false, prefab, category, displayName))
			return;

		HST_ArsenalItemState deposited = arsenal.DepositItem(state, balance, prefab, 1, category, displayName);
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
		string prefab;
		string category;
		string displayName;
		if (!TryResolveLootItemForDeposit(item, state, balance, arsenal, result, true, prefab, category, displayName))
			return;

		HST_VehicleCargoItemState cargoItem = DepositVehicleCargo(state, vehicle, vehicleId, vehiclePrefab, vehicleName, prefab, category, displayName);
		if (!cargoItem)
			return;

		result.m_iItemsDeposited++;
		result.m_aDepositedLines.Insert(string.Format("+ %1 | %2 | vehicle count %3", cargoItem.m_sDisplayName, cargoItem.m_sCategory, cargoItem.m_iCount));
		if (balance.m_bVehicleLootRemoveSource && RemoveLooseLootItem(item))
			result.m_iItemsRemoved++;
	}

	protected bool TryResolveLootItemForDeposit(IEntity item, HST_CampaignState state, HST_BalanceConfig balance, HST_ArsenalService arsenal, HST_LootResult result, bool vehicleLoot, out string prefab, out string category, out string displayName)
	{
		prefab = "";
		category = "";
		displayName = "";
		if (!item || !balance || !arsenal)
			return false;

		prefab = ResolvePrefabName(item);
		if (prefab.IsEmpty())
		{
			if (result)
				result.m_iItemsSkippedBlocked++;
			return false;
		}

		if (IsRawNonLootAsset(prefab))
		{
			if (result)
			{
				result.m_iItemsSkippedBlocked++;
				result.m_aDepositedLines.Insert("blocked raw non-loot asset: " + prefab);
			}

			return false;
		}

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			if (result)
				result.m_iItemsSkippedBlocked++;
			Print("h-istasi loot | skipped invalid item resource " + prefab);
			return false;
		}

		category = ClassifyItem(item, prefab);
		displayName = BuildDisplayName(item, prefab);

		string depositReason;
		if (!arsenal.CanDepositItem(balance, prefab, category, vehicleLoot, depositReason, displayName))
		{
			if (result)
			{
				result.m_iItemsSkippedBlocked++;
				result.m_aDepositedLines.Insert(string.Format("blocked %1 | %2", HST_DisplayNameService.ResolveItemDisplayName(item, prefab, displayName), depositReason));
			}
			return false;
		}

		bool onlyLockedItems;
		if (vehicleLoot)
			onlyLockedItems = balance.m_bVehicleLootOnlyLockedItems;
		else
			onlyLockedItems = balance.m_bLootOnlyLockedItems;

		if (onlyLockedItems && state && arsenal.IsItemUnlocked(state, prefab))
		{
			if (result)
				result.m_iItemsSkippedUnlocked++;
			return false;
		}

		return true;
	}

	protected bool IsRawNonLootAsset(string prefab)
	{
		if (prefab.IsEmpty())
			return true;

		if (prefab.Contains("Assets/Images/"))
			return true;

		if (prefab.Contains("Assets/Objects/"))
			return true;

		if (prefab.Contains(".png") || prefab.Contains(".edds") || prefab.Contains(".xob") || prefab.Contains(".fbx") || prefab.Contains(".txo"))
			return true;

		return false;
	}

	protected void GatherInventoryItemsRecursive(SCR_InventoryStorageManagerComponent inventory, notnull array<IEntity> outItems, notnull array<IEntity> visited, int depth)
	{
		if (!inventory || depth > 8)
			return;

		array<IEntity> directItems = {};
		inventory.GetItems(directItems, EStoragePurpose.PURPOSE_ANY);
		foreach (IEntity item : directItems)
			GatherInventoryItemRecursive(item, outItems, visited, depth);
	}

	protected void GatherStorageItemsRecursive(BaseInventoryStorageComponent storage, notnull array<IEntity> outItems, notnull array<IEntity> visited, int depth)
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
				GatherInventoryItemRecursive(parentSlot.GetAttachedEntity(), outItems, visited, depth);
		}

		int slotCount = storage.GetSlotsCount();
		for (int slotIndex = 0; slotIndex < slotCount; slotIndex++)
		{
			InventoryStorageSlot slot = storage.GetSlot(slotIndex);
			if (!slot)
				continue;

			GatherInventoryItemRecursive(slot.GetAttachedEntity(), outItems, visited, depth);
		}
	}

	protected void GatherInventoryItemRecursive(IEntity item, notnull array<IEntity> outItems, notnull array<IEntity> visited, int depth)
	{
		if (!item || depth > 8 || visited.Find(item) >= 0)
			return;

		visited.Insert(item);

		SCR_InventoryStorageManagerComponent childInventory = SCR_InventoryStorageManagerComponent.Cast(item.FindComponent(SCR_InventoryStorageManagerComponent));
		if (childInventory)
			GatherInventoryItemsRecursive(childInventory, outItems, visited, depth + 1);

		InventoryItemComponent itemComponent = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		BaseInventoryStorageComponent itemStorage = BaseInventoryStorageComponent.Cast(itemComponent);
		if (itemStorage)
			GatherStorageItemsRecursive(itemStorage, outItems, visited, depth + 1);

		outItems.Insert(item);
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

	protected bool IsVehicleOccupied(IEntity vehicle, IEntity requestingPlayer, out string reason)
	{
		reason = "";
		if (!vehicle)
			return false;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(vehicle.GetOrigin(), 12, AddLootCandidate, null, EQueryEntitiesFlags.ALL);
		foreach (IEntity candidate : m_aScanEntities)
		{
			if (!candidate || candidate == vehicle)
				continue;

			if (!IsPotentialVehicleOccupant(candidate))
				continue;

			if (!IsEntityParentedToRoot(candidate, vehicle))
				continue;

			if (candidate == requestingPlayer)
				reason = "requesting player is still attached to the vehicle";
			else
				reason = "living occupant attached to the vehicle: " + candidate.GetName();

			return true;
		}

		return false;
	}

	protected bool IsPotentialVehicleOccupant(IEntity entity)
	{
		if (!entity || !IsLivingEntity(entity))
			return false;

		string prefab = ResolvePrefabName(entity);
		string name = entity.GetName();
		if (prefab.Contains("Character") || name.Contains("Character"))
			return true;

		if (prefab.Contains("Vehicle") || prefab.Contains("Vehicles"))
			return false;

		if (!entity.FindComponent(FactionAffiliationComponent))
			return false;

		return entity.FindComponent(SCR_InventoryStorageManagerComponent) != null;
	}

	protected bool IsEntityParentedToRoot(IEntity entity, IEntity root)
	{
		IEntity cursor = entity;
		for (int i = 0; i < 16; i++)
		{
			if (!cursor)
				break;

			if (cursor == root)
				return true;

			cursor = cursor.GetParent();
		}

		return false;
	}

	protected string ResolveDamageState(IEntity vehicle)
	{
		if (!vehicle)
			return "unknown";

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(vehicle.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager)
			return "unknown";

		return string.Format("%1", damageManager.GetState());
	}

	protected int CaptureVehiclePhysicalCargo(IEntity vehicle, HST_GarageVehicleState garageVehicle)
	{
		if (!vehicle || !garageVehicle)
			return 0;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(vehicle.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return 0;

		array<IEntity> items = {};
		array<IEntity> visited = {};
		GatherInventoryItemsRecursive(inventory, items, visited, 0);
		int captured;
		foreach (IEntity item : items)
		{
			if (!item)
				continue;

			string prefab = ResolvePrefabName(item);
			if (prefab.IsEmpty())
				continue;

			string category = ClassifyItem(item, prefab);
			string displayName = BuildDisplayName(item, prefab);
			AddStoredVehicleCargo(garageVehicle, prefab, category, displayName, "physical", 1);
			captured++;
		}

		return captured;
	}

	protected int CopyVirtualVehicleCargoToGarage(HST_CampaignState state, string vehicleRuntimeId, HST_GarageVehicleState garageVehicle)
	{
		if (!state || vehicleRuntimeId.IsEmpty() || !garageVehicle)
			return 0;

		int copied;
		foreach (HST_VehicleCargoItemState cargoItem : state.m_aVehicleCargoItems)
		{
			if (!cargoItem || cargoItem.m_sVehicleRuntimeId != vehicleRuntimeId)
				continue;

			int count = Math.Max(1, cargoItem.m_iCount);
			AddStoredVehicleCargo(garageVehicle, cargoItem.m_sItemPrefab, cargoItem.m_sCategory, cargoItem.m_sDisplayName, "virtual", count);
			copied += count;
		}

		return copied;
	}

	protected int RemoveVirtualVehicleCargo(HST_CampaignState state, string vehicleRuntimeId)
	{
		if (!state || vehicleRuntimeId.IsEmpty())
			return 0;

		int removed;
		for (int i = state.m_aVehicleCargoItems.Count() - 1; i >= 0; i--)
		{
			HST_VehicleCargoItemState cargoItem = state.m_aVehicleCargoItems[i];
			if (!cargoItem || cargoItem.m_sVehicleRuntimeId != vehicleRuntimeId)
				continue;

			state.m_aVehicleCargoItems.Remove(i);
			removed++;
		}

		return removed;
	}

	protected HST_StoredVehicleCargoState AddStoredVehicleCargo(HST_GarageVehicleState garageVehicle, string itemPrefab, string category, string displayName, string source, int count)
	{
		if (!garageVehicle || itemPrefab.IsEmpty() || count <= 0)
			return null;

		foreach (HST_StoredVehicleCargoState storedCargo : garageVehicle.m_aStoredCargoItems)
		{
			if (!storedCargo || storedCargo.m_sItemPrefab != itemPrefab)
				continue;

			storedCargo.m_iCount += count;
			if (!category.IsEmpty())
				storedCargo.m_sCategory = category;
			if (!displayName.IsEmpty())
				storedCargo.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, itemPrefab, displayName);
			if (storedCargo.m_sSource != source)
				storedCargo.m_sSource = "mixed";
			return storedCargo;
		}

		HST_StoredVehicleCargoState stored = new HST_StoredVehicleCargoState();
		stored.m_sItemPrefab = itemPrefab;
		stored.m_sCategory = category;
		stored.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, itemPrefab, displayName);
		stored.m_sSource = source;
		stored.m_iCount = count;
		garageVehicle.m_aStoredCargoItems.Insert(stored);
		return stored;
	}

	protected int CountStoredVehicleCargoItems(HST_GarageVehicleState garageVehicle)
	{
		if (!garageVehicle)
			return 0;

		int count;
		foreach (HST_StoredVehicleCargoState storedCargo : garageVehicle.m_aStoredCargoItems)
		{
			if (storedCargo)
				count += Math.Max(1, storedCargo.m_iCount);
		}

		return count;
	}

	protected string BuildUniqueGarageVehicleId(HST_CampaignState state)
	{
		if (!state)
			return "garage_0";

		for (int i = 0; i < 64; i++)
		{
			string candidate = string.Format("garage_%1_%2_%3", state.m_iElapsedSeconds, state.m_aGarageVehicles.Count(), i);
			if (!state.FindGarageVehicle(candidate))
				return candidate;
		}

		return string.Format("garage_%1_%2_fallback", state.m_iElapsedSeconds, state.m_aGarageVehicles.Count());
	}

	protected string ResolvePrefabName(IEntity item)
	{
		if (!item || !item.GetPrefabData())
			return "";

		return item.GetPrefabData().GetPrefabName();
	}

	protected string ResolveVehicleIdentityName(IEntity vehicle)
	{
		string prefab = ResolvePrefabName(vehicle);
		if (!prefab.IsEmpty())
			return prefab;

		if (!vehicle)
			return "";

		string name = vehicle.GetName();
		if (name.IsEmpty())
			return "";

		if (HST_VehicleRootPolicy.IsVehiclePartName(name))
			return "";

		if (HST_VehicleRootPolicy.IsKnownVehicleRootName(name))
			return name;

		return "";
	}

	protected bool IsEligibleGarageVehicle(IEntity entity, IEntity playerEntity, HST_CampaignPreset preset)
	{
		if (!entity || entity == playerEntity)
			return false;

		string prefab = ResolveVehicleIdentityName(entity);
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

		HST_VehicleRootScanResult result = new HST_VehicleRootScanResult();
		if (TrySelectRuntimeRecordVehicle(playerEntity, radiusMeters, state, result, vehicleRuntimeId))
		{
			PublishVehicleTargetDiagnostics(state, "vehicle loot id target selected", result, result.m_fSelectedDistanceMeters * result.m_fSelectedDistanceMeters);
			return result;
		}

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(playerEntity.GetOrigin(), radiusMeters, AddLootCandidate, null, EQueryEntitiesFlags.ALL);
		array<IEntity> checkedRoots = {};
		result.m_iCandidates = m_aScanEntities.Count();
		string lastReject = "target runtime id not found";
		foreach (IEntity candidate : m_aScanEntities)
		{
			string rejectReason;
			IEntity rootVehicle = ResolveVehicleRoot(candidate, rejectReason);
			if (!rootVehicle)
				rootVehicle = ResolveVehicleRootByNearbyBounds(candidate, state, rejectReason);

			if (!rootVehicle)
			{
				TrackNearestVehicleReject(playerEntity, candidate, rejectReason, result);
				lastReject = rejectReason;
				continue;
			}

			RegisterVehicleRootPass(result, rejectReason);

			if (checkedRoots.Find(rootVehicle) >= 0)
				continue;

			checkedRoots.Insert(rootVehicle);
			result.m_iResolvedRoots = checkedRoots.Count();
			string protectedReason;
			HST_RuntimeVehicleState runtimeCandidate = ResolveRuntimeVehicleRecord(state, rootVehicle);
			string rootPrefab = ResolveVehiclePrefabFromCandidate(rootVehicle, runtimeCandidate, candidate);
			if (IsProtectedHQEntity(state, rootVehicle, rootPrefab, protectedReason))
			{
				TrackNearestVehicleReject(playerEntity, rootVehicle, protectedReason, result);
				lastReject = protectedReason;
				continue;
			}

			if (!IsEligibleLootVehicle(rootVehicle, playerEntity, runtimeCandidate, rootPrefab))
			{
				TrackNearestVehicleReject(playerEntity, rootVehicle, "resolved root failed loot vehicle eligibility", result);
				lastReject = "resolved root failed loot vehicle eligibility";
				continue;
			}

			if (DoesRequestedVehicleIdMatch(rootVehicle, candidate, runtimeCandidate, vehicleRuntimeId))
			{
				runtimeCandidate = EnsureRuntimeVehicleRecord(state, rootVehicle, runtimeCandidate, rootPrefab);
				string resolvedRuntimeId = ResolveVehicleRuntimeIdFromRecord(rootVehicle, runtimeCandidate);
				rootPrefab = ResolveVehiclePrefabFromRecord(rootVehicle, runtimeCandidate);
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

		if (!result.m_sNearestRejectReason.IsEmpty())
			result.m_sRejectReason = result.m_sNearestRejectReason;
		else if (result.m_sRejectReason.IsEmpty() || result.m_sRejectReason == "not scanned")
			result.m_sRejectReason = lastReject;

		if (state)
		{
			state.m_iLastVehicleTargetCandidates = result.m_iCandidates;
			state.m_sLastVehicleTargetStatus = "vehicle target not found";
			state.m_sLastVehicleTargetPrefab = "";
			state.m_sLastVehicleTargetReason = result.m_sRejectReason;
			state.m_fLastVehicleTargetDistanceMeters = 0;
			state.m_iLastVehicleTargetCargoEntries = 0;
		}

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

		if (TrySelectRuntimeRecordVehicle(playerEntity, radiusMeters, state, result, ""))
		{
			PublishVehicleTargetDiagnostics(state, purpose, result, result.m_fSelectedDistanceMeters * result.m_fSelectedDistanceMeters);
			return result;
		}

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(playerEntity.GetOrigin(), radiusMeters, AddLootCandidate, null, EQueryEntitiesFlags.ALL);

		float bestDistanceSq = 999999999.0;
		array<IEntity> checkedRoots = {};
		foreach (IEntity candidate : m_aScanEntities)
		{
			result.m_iCandidates++;
			string rejectReason;
			IEntity rootVehicle = ResolveVehicleRoot(candidate, rejectReason);
			if (!rootVehicle)
				rootVehicle = ResolveVehicleRootByNearbyBounds(candidate, state, rejectReason);

			if (!rootVehicle)
			{
				TrackNearestVehicleReject(playerEntity, candidate, rejectReason, result);
				if (!result.m_SelectedRoot)
					result.m_sRejectReason = rejectReason;
				continue;
			}

			RegisterVehicleRootPass(result, rejectReason);

			if (checkedRoots.Find(rootVehicle) >= 0)
				continue;

			checkedRoots.Insert(rootVehicle);
			result.m_iResolvedRoots++;
			string protectedReason;
			HST_RuntimeVehicleState runtimeCandidate = ResolveRuntimeVehicleRecord(state, rootVehicle);
			string rootPrefab = ResolveVehiclePrefabFromCandidate(rootVehicle, runtimeCandidate, candidate);
			if (IsProtectedHQEntity(state, rootVehicle, rootPrefab, protectedReason))
			{
				TrackNearestVehicleReject(playerEntity, rootVehicle, protectedReason, result);
				if (!result.m_SelectedRoot)
					result.m_sRejectReason = protectedReason;
				continue;
			}

			if (!IsEligibleLootVehicle(rootVehicle, playerEntity, runtimeCandidate, rootPrefab))
			{
				TrackNearestVehicleReject(playerEntity, rootVehicle, "resolved root failed loot vehicle eligibility", result);
				if (!result.m_SelectedRoot)
					result.m_sRejectReason = "resolved root failed loot vehicle eligibility";
				continue;
			}

			float distanceSq = DistanceSq2D(playerEntity.GetOrigin(), rootVehicle.GetOrigin());
			if (distanceSq < bestDistanceSq)
			{
				runtimeCandidate = EnsureRuntimeVehicleRecord(state, rootVehicle, runtimeCandidate, rootPrefab);
				rootPrefab = ResolveVehiclePrefabFromRecord(rootVehicle, runtimeCandidate);
				result.m_SelectedRoot = rootVehicle;
				result.m_RuntimeVehicle = runtimeCandidate;
				result.m_sSelectedRuntimeId = ResolveVehicleRuntimeIdFromRecord(rootVehicle, runtimeCandidate);
				result.m_sSelectedPrefab = rootPrefab;
				result.m_sSelectedDisplayName = BuildVehicleDisplayNameFromRecord(rootVehicle, rootPrefab, runtimeCandidate);
				result.m_sRejectReason = "selected nearest eligible root | " + rejectReason;
				result.m_fSelectedDistanceMeters = Math.Sqrt(distanceSq);
				bestDistanceSq = distanceSq;
			}
		}

		if (!result.m_SelectedRoot && !result.m_sNearestRejectReason.IsEmpty())
			result.m_sRejectReason = result.m_sNearestRejectReason;
		else if (!result.m_SelectedRoot && (result.m_sRejectReason.IsEmpty() || result.m_sRejectReason == "not scanned"))
			result.m_sRejectReason = "no vehicle prefab roots found";

		PublishVehicleTargetDiagnostics(state, purpose, result, bestDistanceSq);
		return result;
	}

	protected bool TrySelectRuntimeRecordVehicle(IEntity playerEntity, int radiusMeters, HST_CampaignState state, HST_VehicleRootScanResult result, string requestedRuntimeId)
	{
		if (!playerEntity || !state || !result)
			return false;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			result.m_sRejectReason = "world not ready";
			return false;
		}

		vector playerOrigin = playerEntity.GetOrigin();
		float radiusSq = radiusMeters * radiusMeters;
		float bestDistanceSq = 999999999.0;
		IEntity bestRoot;
		HST_RuntimeVehicleState bestRecord;
		string lastReject = "no registered h-istasi vehicle within radius";
		foreach (HST_RuntimeVehicleState record : state.m_aRuntimeVehicles)
		{
			if (!record || record.m_bDeleted || record.m_sVehicleRuntimeId.IsEmpty())
				continue;

			if (!requestedRuntimeId.IsEmpty() && record.m_sVehicleRuntimeId != requestedRuntimeId)
				continue;

			if (!HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(record.m_sPrefab))
			{
				lastReject = "registered vehicle prefab is not a root vehicle";
				continue;
			}

			if (DistanceSq2D(playerOrigin, record.m_vPosition) > radiusSq)
			{
				lastReject = "registered vehicle record outside interaction radius";
				continue;
			}

			int scannedCandidates;
			int resolvedRoots;
			string recordReject;
			IEntity rootVehicle = ResolveRuntimeVehicleRootFromRecord(world, record, scannedCandidates, resolvedRoots, recordReject);
			result.m_iCandidates += scannedCandidates;
			result.m_iResolvedRoots += resolvedRoots;
			if (!rootVehicle)
			{
				lastReject = recordReject;
				continue;
			}

			string protectedReason;
			string rootPrefab = ResolveVehiclePrefabFromRecord(rootVehicle, record);
			if (IsProtectedHQEntity(state, rootVehicle, rootPrefab, protectedReason))
			{
				lastReject = protectedReason;
				continue;
			}

			if (!IsEligibleLootVehicle(rootVehicle, playerEntity, record))
			{
				lastReject = "registered vehicle root failed eligibility";
				continue;
			}

			float distanceSq = DistanceSq2D(playerOrigin, rootVehicle.GetOrigin());
			if (distanceSq > radiusSq)
			{
				lastReject = "registered vehicle root outside interaction radius";
				continue;
			}

			if (!requestedRuntimeId.IsEmpty())
			{
				FillVehicleScanResult(result, rootVehicle, record, "matched registered h-istasi runtime vehicle", distanceSq);
				return true;
			}

			if (distanceSq < bestDistanceSq)
			{
				bestRoot = rootVehicle;
				bestRecord = record;
				bestDistanceSq = distanceSq;
			}
		}

		if (!bestRoot || !bestRecord)
		{
			if (!lastReject.IsEmpty())
				result.m_sRejectReason = lastReject;
			return false;
		}

		FillVehicleScanResult(result, bestRoot, bestRecord, "selected registered h-istasi runtime vehicle fallback", bestDistanceSq);
		return true;
	}

	protected void FillVehicleScanResult(HST_VehicleRootScanResult result, IEntity rootVehicle, HST_RuntimeVehicleState runtimeVehicle, string reason, float distanceSq)
	{
		if (!result || !rootVehicle)
			return;

		if (runtimeVehicle)
		{
			runtimeVehicle.m_vPosition = rootVehicle.GetOrigin();
			runtimeVehicle.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(rootVehicle.GetYawPitchRoll());
		}

		string prefab = ResolveVehiclePrefabFromRecord(rootVehicle, runtimeVehicle);
		result.m_SelectedRoot = rootVehicle;
		result.m_RuntimeVehicle = runtimeVehicle;
		result.m_sSelectedRuntimeId = ResolveVehicleRuntimeIdFromRecord(rootVehicle, runtimeVehicle);
		result.m_sSelectedPrefab = prefab;
		result.m_sSelectedDisplayName = BuildVehicleDisplayNameFromRecord(rootVehicle, prefab, runtimeVehicle);
		result.m_sRejectReason = reason;
		result.m_fSelectedDistanceMeters = Math.Sqrt(distanceSq);
		if (reason.Contains("runtime") || reason.Contains("registered"))
			result.m_iRuntimeFallbackHits++;
	}

	protected HST_RuntimeVehicleState EnsureRuntimeVehicleRecord(HST_CampaignState state, IEntity rootVehicle, HST_RuntimeVehicleState existingRecord, string prefabHint = "")
	{
		if (!state || !rootVehicle)
			return existingRecord;

		string runtimeId = ResolveVehicleRuntimeId(rootVehicle);
		string prefab = prefabHint;
		if (prefab.IsEmpty() || !HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
			prefab = ResolveVehiclePrefabFromRecord(rootVehicle, existingRecord);
		if (prefab.IsEmpty())
			prefab = ResolveVehicleIdentityName(rootVehicle);
		if (runtimeId.IsEmpty() || prefab.IsEmpty() || !HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
			return existingRecord;

		HST_RuntimeVehicleState record = existingRecord;
		if (!record)
			record = state.FindRuntimeVehicle(runtimeId);
		if (!record)
		{
			record = new HST_RuntimeVehicleState();
			record.m_sVehicleRuntimeId = runtimeId;
			state.m_aRuntimeVehicles.Insert(record);
		}

		string previousRuntimeId = record.m_sVehicleRuntimeId;
		record.m_sVehicleRuntimeId = runtimeId;
		record.m_sPrefab = prefab;
		record.m_sDisplayName = BuildVehicleDisplayName(rootVehicle, prefab);
		record.m_sFactionKey = ResolveFactionKey(rootVehicle);
		record.m_sZoneId = FindNearestZoneId(state, rootVehicle.GetOrigin());
		string previousRuntimeKind = record.m_sRuntimeKind;
		if (previousRuntimeKind.IsEmpty() || previousRuntimeKind == "runtime_vehicle")
			record.m_sRuntimeKind = "loot_vehicle";
		else
			record.m_sRuntimeKind = previousRuntimeKind;
		HST_VehicleCapabilityPolicy.ApplyToRuntimeVehicle(record);
		record.m_vPosition = rootVehicle.GetOrigin();
		record.m_vAngles = HST_WorldPositionService.BuildUprightAnglesFromVector(rootVehicle.GetYawPitchRoll());
		record.m_bDeleted = false;
		if (!previousRuntimeId.IsEmpty() && previousRuntimeId != runtimeId)
			RekeyVehicleCargoRuntimeId(state, previousRuntimeId, runtimeId, prefab, record.m_sDisplayName, rootVehicle.GetOrigin());
		return record;
	}

	protected bool DoesRequestedVehicleIdMatch(IEntity rootVehicle, IEntity sourceCandidate, HST_RuntimeVehicleState runtimeVehicle, string requestedRuntimeId)
	{
		if (requestedRuntimeId.IsEmpty())
			return false;

		if (runtimeVehicle && runtimeVehicle.m_sVehicleRuntimeId == requestedRuntimeId)
			return true;

		if (rootVehicle && ResolveVehicleRuntimeId(rootVehicle) == requestedRuntimeId)
			return true;

		if (sourceCandidate && ResolveVehicleRuntimeId(sourceCandidate) == requestedRuntimeId)
			return true;

		IEntity cursor = sourceCandidate;
		for (int i = 0; i < 16; i++)
		{
			if (!cursor)
				break;

			if (ResolveVehicleRuntimeId(cursor) == requestedRuntimeId)
				return true;
			if (cursor == rootVehicle)
				break;

			cursor = cursor.GetParent();
		}

		return false;
	}

	protected IEntity ResolveRuntimeVehicleRootFromRecord(BaseWorld world, HST_RuntimeVehicleState record, out int scannedCandidates, out int resolvedRoots, out string rejectReason)
	{
		scannedCandidates = 0;
		resolvedRoots = 0;
		rejectReason = "registered vehicle record had no live root nearby";
		if (!world || !record)
			return null;

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(record.m_vPosition, PERSISTENT_FIELD_VEHICLE_RESTORE_RADIUS_METERS, AddLootCandidate, null, EQueryEntitiesFlags.ALL);

		array<IEntity> checkedRoots = {};
		IEntity bestRoot;
		float bestDistanceSq = 999999999.0;
		int bestScore = 999;
		foreach (IEntity candidate : m_aScanEntities)
		{
			scannedCandidates++;
			string rootReject;
			IEntity rootVehicle = ResolveVehicleRoot(candidate, rootReject);
			if (!rootVehicle)
				rootVehicle = ResolveVehicleRootByNearbyBounds(candidate, null, rootReject);

			if (!rootVehicle)
			{
				rejectReason = rootReject;
				continue;
			}

			if (checkedRoots.Find(rootVehicle) >= 0)
				continue;

			checkedRoots.Insert(rootVehicle);
			resolvedRoots++;
			string rootPrefab = ResolveVehicleIdentityName(rootVehicle);
			if (!IsEligibleVehicleRoot(rootVehicle, rootPrefab))
			{
				rejectReason = BuildVehicleRootRejectReason(rootVehicle, rootPrefab);
				continue;
			}

			float distanceSq = DistanceSq2D(record.m_vPosition, rootVehicle.GetOrigin());
			if (distanceSq > PERSISTENT_FIELD_VEHICLE_RESTORE_RADIUS_METERS * PERSISTENT_FIELD_VEHICLE_RESTORE_RADIUS_METERS)
			{
				rejectReason = "resolved root was not near registered vehicle record";
				continue;
			}

			if (!VehiclePrefabsMatch(rootPrefab, record.m_sPrefab))
			{
				rejectReason = string.Format("registered vehicle prefab mismatch | record %1 | live %2", record.m_sPrefab, rootPrefab);
				continue;
			}

			int score = 0;

			if (score > bestScore)
				continue;

			if (score == bestScore && distanceSq >= bestDistanceSq)
				continue;

			bestRoot = rootVehicle;
			bestDistanceSq = distanceSq;
			bestScore = score;
			rejectReason = "resolved live vehicle root near registered record";
		}

		if (bestRoot)
			return bestRoot;

		return null;
	}

	protected bool VehiclePrefabsMatch(string leftPrefab, string rightPrefab)
	{
		if (leftPrefab.IsEmpty() || rightPrefab.IsEmpty())
			return false;

		if (leftPrefab == rightPrefab)
			return true;

		return ResolveResourceBasename(leftPrefab) == ResolveResourceBasename(rightPrefab);
	}

	protected string ResolveResourceBasename(string prefab)
	{
		string basename = prefab;
		int lastSlash = -1;
		for (int index = 0; index < prefab.Length(); index++)
		{
			string current = prefab.Substring(index, 1);
			if (current == "/")
				lastSlash = index;
		}

		if (lastSlash >= 0 && lastSlash + 1 < prefab.Length())
			basename = prefab.Substring(lastSlash + 1, prefab.Length() - lastSlash - 1);

		return basename;
	}

	protected void PublishVehicleTargetDiagnostics(HST_CampaignState state, string status, HST_VehicleRootScanResult scan, float distanceSq)
	{
		if (!state)
			return;

		IEntity vehicle;
		int candidates;
		int resolvedRoots;
		int directRoots;
		int parentRoots;
		int boundsRoots;
		int runtimeRoots;
		string reason = "not scanned";
		string nearestDebug;
		if (scan)
		{
			vehicle = scan.m_SelectedRoot;
			candidates = scan.m_iCandidates;
			resolvedRoots = scan.m_iResolvedRoots;
			directRoots = scan.m_iDirectRootHits;
			parentRoots = scan.m_iParentChainRootHits;
			boundsRoots = scan.m_iBoundsRootHits;
			runtimeRoots = scan.m_iRuntimeFallbackHits;
			reason = scan.m_sRejectReason;
			nearestDebug = scan.m_sNearestCandidateDebug;
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
			string passSummary = string.Format("direct %1 | parent %2 | bounds %3 | runtime %4", directRoots, parentRoots, boundsRoots, runtimeRoots);
			string selectedLog = string.Format("h-istasi vehicle target | %1 | selected %2 | prefab %3 | id %4 | candidates %5 | roots %6 | %7 | distance %8m", status, BuildVehicleDisplayNameFromScan(vehicle, prefab, scan), prefab, vehicleId, candidates, resolvedRoots, passSummary, state.m_fLastVehicleTargetDistanceMeters);
			Print(selectedLog + " | reason " + reason);
			return;
		}

		string noTargetPassSummary = string.Format("direct %1 | parent %2 | bounds %3 | runtime %4", directRoots, parentRoots, boundsRoots, runtimeRoots);
		string noTargetLog = string.Format("h-istasi vehicle target | %1 | no target | candidates %2 | roots %3 | %4", status, candidates, resolvedRoots, noTargetPassSummary);
		Print(noTargetLog + " | reason " + reason + " | " + nearestDebug, LogLevel.WARNING);
	}

	protected bool IsPlayerAtVehicleRear(IEntity playerEntity, IEntity vehicle, int radiusMeters)
	{
		if (!playerEntity || !vehicle)
			return false;

		int gateMeters = Math.Max(5, Math.Min(radiusMeters, 12));
		vector playerOrigin = playerEntity.GetOrigin();
		if (DistanceSq2D(playerOrigin, vehicle.GetOrigin()) <= gateMeters * gateMeters)
			return true;

		vector mins;
		vector maxs;
		vehicle.GetWorldBounds(mins, maxs);
		float clampedX = Math.Clamp(playerOrigin[0], mins[0] - gateMeters, maxs[0] + gateMeters);
		float clampedZ = Math.Clamp(playerOrigin[2], mins[2] - gateMeters, maxs[2] + gateMeters);
		float dx = playerOrigin[0] - clampedX;
		float dz = playerOrigin[2] - clampedZ;
		return (dx * dx + dz * dz) <= gateMeters * gateMeters;
	}

	protected bool IsEligibleLootVehicle(IEntity entity, IEntity playerEntity, HST_RuntimeVehicleState runtimeVehicle, string prefabHint = "")
	{
		if (!entity || entity == playerEntity)
			return false;

		string identity = prefabHint;
		if (identity.IsEmpty() || !HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(identity))
			identity = ResolveVehiclePrefabFromRecord(entity, runtimeVehicle);
		if (identity.IsEmpty())
			identity = ResolveVehicleIdentityName(entity);

		if (identity.IsEmpty())
			return false;

		if (identity.Contains("Character") || identity.Contains("Inventory") || identity.Contains("Magazine"))
			return false;

		if (runtimeVehicle)
			return HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(runtimeVehicle.m_sPrefab);

		return IsEligibleVehicleRoot(entity, identity) || HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(identity);
	}

	protected IEntity ResolveVehicleRoot(IEntity entity, out string rejectReason)
	{
		rejectReason = "entity missing";
		if (!entity)
			return null;

		string directPrefab = ResolveVehicleIdentityName(entity);
		if (IsDirectVehicleRootCandidate(entity, directPrefab))
		{
			rejectReason = BuildDirectVehicleRootReason(entity, directPrefab);
			return entity;
		}

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

			string candidatePrefab = ResolveVehicleIdentityName(candidate);
			if (IsEngineClassifiedVehicleRoot(candidate, candidatePrefab))
			{
				rejectReason = "resolved by base-game vehicle classification";
				return candidate;
			}

			if (IsEligibleVehicleRoot(candidate, candidatePrefab))
			{
				IEntity containerRoot = ResolveVehicleContainerRoot(chain, candidate);
				if (candidatePrefab.Contains("Prefabs/Vehicles/") || candidatePrefab.Contains("/Vehicles/"))
					rejectReason = "resolved canonical vehicle root";
				else
					rejectReason = "resolved known base-game vehicle basename root";
				return containerRoot;
			}

			rejectReason = BuildVehicleRootRejectReason(candidate, candidatePrefab);
		}

		if (rejectReason.IsEmpty() || rejectReason == "entity missing")
			rejectReason = "candidate chain had no eligible vehicle root prefab";

		return null;
	}

	protected IEntity ResolveVehicleRootByNearbyBounds(IEntity entity, HST_CampaignState state, out string rejectReason)
	{
		rejectReason = "candidate had no nearby vehicle bounds root";
		if (!entity)
		{
			rejectReason = "entity missing";
			return null;
		}

		vector origin = entity.GetOrigin();
		IEntity bestRoot;
		float bestDistanceSq = 999999999.0;
		foreach (IEntity candidate : m_aScanEntities)
		{
			if (!candidate || candidate == entity)
				continue;

			string candidatePrefab = ResolveVehicleIdentityName(candidate);
			if (!IsEligibleVehicleRoot(candidate, candidatePrefab))
				continue;

			if (!IsPointNearEntityBounds(origin, candidate, 8.0))
				continue;

			float distanceSq = DistanceSq2D(origin, candidate.GetOrigin());
			if (distanceSq >= bestDistanceSq)
				continue;

			bestRoot = candidate;
			bestDistanceSq = distanceSq;
		}

		if (!bestRoot)
			return null;

		rejectReason = "resolved by nearby vehicle bounds root";
		return bestRoot;
	}

	protected IEntity ResolveVehicleContainerRoot(array<IEntity> chain, IEntity matchedEntity)
	{
		if (!matchedEntity)
			return null;

		if (!chain || chain.Count() == 0)
			return matchedEntity;

		for (int i = chain.Count() - 1; i >= 0; i--)
		{
			IEntity candidate = chain[i];
			if (!candidate)
				continue;

			string candidatePrefab = ResolveVehicleIdentityName(candidate);
			if (candidate == matchedEntity)
				return matchedEntity;

			if (IsEligibleVehicleRoot(candidate, candidatePrefab))
				return candidate;
		}

		return matchedEntity;
	}

	protected bool IsPointNearEntityBounds(vector point, IEntity entity, float marginMeters)
	{
		if (!entity)
			return false;

		vector mins;
		vector maxs;
		entity.GetWorldBounds(mins, maxs);
		float clampedX = Math.Clamp(point[0], mins[0] - marginMeters, maxs[0] + marginMeters);
		float clampedZ = Math.Clamp(point[2], mins[2] - marginMeters, maxs[2] + marginMeters);
		float dx = point[0] - clampedX;
		float dz = point[2] - clampedZ;
		return (dx * dx + dz * dz) <= marginMeters * marginMeters;
	}

	protected void RegisterVehicleRootPass(HST_VehicleRootScanResult result, string reason)
	{
		if (!result)
			return;

		if (reason.Contains("direct"))
		{
			result.m_iDirectRootHits++;
			return;
		}

		if (reason.Contains("bounds"))
		{
			result.m_iBoundsRootHits++;
			return;
		}

		result.m_iParentChainRootHits++;
	}

	protected void TrackNearestVehicleReject(IEntity playerEntity, IEntity candidate, string reason, HST_VehicleRootScanResult result)
	{
		if (!playerEntity || !candidate || !result)
			return;

		float distanceSq = DistanceSq2D(playerEntity.GetOrigin(), candidate.GetOrigin());
		if (distanceSq >= result.m_fNearestRejectDistanceSq)
			return;

		result.m_fNearestRejectDistanceSq = distanceSq;
		result.m_sNearestRejectReason = reason;
		result.m_sNearestCandidateDebug = BuildVehicleCandidateDebug(candidate, reason);
	}

	protected string BuildVehicleCandidateDebug(IEntity entity, string reason)
	{
		if (!entity)
			return "nearest candidate missing";

		string prefab = ResolveVehicleIdentityName(entity);
		string name = entity.GetName();
		if (name.IsEmpty())
			name = "<unnamed>";

		return string.Format("nearest %1 | prefab/name %2 | origin %3 | reject %4", name, prefab, entity.GetOrigin(), reason);
	}

	protected bool IsDirectVehicleRootCandidate(IEntity entity, string prefabOrName)
	{
		if (!entity)
			return false;

		if (entity.GetParent())
			return false;

		if (IsVehiclePartEntity(entity, prefabOrName))
			return false;

		if (IsEngineClassifiedVehicleRoot(entity, prefabOrName))
			return true;

		return IsEligibleVehicleRoot(entity, prefabOrName);
	}

	protected string BuildDirectVehicleRootReason(IEntity entity, string prefabOrName)
	{
		if (IsEngineClassifiedVehicleRoot(entity, prefabOrName))
			return "resolved direct editor/base-game vehicle root";

		if (!prefabOrName.IsEmpty() && HST_VehicleRootPolicy.IsKnownVehicleRootName(prefabOrName))
			return "resolved direct known vehicle basename root";

		return "resolved direct vehicle root";
	}

	protected bool IsEligibleVehicleRoot(IEntity entity, string prefab)
	{
		if (!entity)
			return false;

		if (IsEngineClassifiedVehicleRoot(entity, prefab))
			return true;

		if (!prefab.IsEmpty() && HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab) && !HST_VehicleRootPolicy.IsVehiclePartName(entity.GetName()))
			return true;

		if (HST_VehicleRootPolicy.IsKnownVehicleRootName(entity.GetName()) && !HST_VehicleRootPolicy.IsVehiclePartName(entity.GetName()))
			return true;

		if (prefab.IsEmpty())
			return false;

		return false;
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

		if (HST_VehicleRootPolicy.IsKnownVehicleRootName(name) && !entity.GetParent())
			return true;

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

	protected bool IsLoadableVehiclePrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		Resource loaded = Resource.Load(prefab);
		if (!loaded)
			return false;

		return loaded.IsValid();
	}

	protected string BuildVehicleRootRejectReason(IEntity entity, string prefab)
	{
		if (!entity)
			return "candidate missing";

		if (prefab.IsEmpty() && !HST_VehicleRootPolicy.IsKnownVehicleRootName(entity.GetName()))
			return "candidate had no prefab";

		if (prefab.IsEmpty() && HST_VehicleRootPolicy.IsKnownVehicleRootName(entity.GetName()))
			return "candidate passed known vehicle basename checks";

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

		string prefab = ResolveVehicleIdentityName(entity);
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

		return string.Format("local_%1_%2", ResolveVehicleIdentityName(vehicle), vehicle.GetOrigin());
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

		return ResolveVehicleIdentityName(vehicle);
	}

	protected string ResolveVehiclePrefabFromCandidate(IEntity vehicle, HST_RuntimeVehicleState runtimeVehicle, IEntity sourceCandidate)
	{
		string prefab = ResolveVehiclePrefabFromRecord(vehicle, runtimeVehicle);
		if (!prefab.IsEmpty() && HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
			return prefab;

		prefab = FindVehiclePrefabInParentChain(sourceCandidate, vehicle);
		if (!prefab.IsEmpty())
			return prefab;

		return FindVehiclePrefabNearEntity(vehicle);
	}

	protected string ResolveVehiclePrefabFromRecord(IEntity vehicle, HST_RuntimeVehicleState runtimeVehicle)
	{
		if (runtimeVehicle && !runtimeVehicle.m_sPrefab.IsEmpty())
			return runtimeVehicle.m_sPrefab;

		string prefab = ResolveVehicleIdentityName(vehicle);
		if (!prefab.IsEmpty() && HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
			return prefab;

		string nearbyPrefab = FindVehiclePrefabNearEntity(vehicle);
		if (!nearbyPrefab.IsEmpty())
			return nearbyPrefab;

		return prefab;
	}

	protected string FindVehiclePrefabInParentChain(IEntity entity, IEntity stopAt)
	{
		IEntity cursor = entity;
		for (int i = 0; i < 16; i++)
		{
			if (!cursor)
				break;

			string prefab = ResolveVehicleIdentityName(cursor);
			if (!prefab.IsEmpty() && HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
				return prefab;

			if (cursor == stopAt)
				break;

			cursor = cursor.GetParent();
		}

		return "";
	}

	protected string FindVehiclePrefabNearEntity(IEntity vehicle)
	{
		if (!vehicle)
			return "";

		string chainPrefab = FindVehiclePrefabInParentChain(vehicle, null);
		if (!chainPrefab.IsEmpty())
			return chainPrefab;

		vector origin = vehicle.GetOrigin();
		foreach (IEntity candidate : m_aScanEntities)
		{
			if (!candidate || candidate == vehicle)
				continue;

			string candidatePrefab = ResolveVehicleIdentityName(candidate);
			if (candidatePrefab.IsEmpty() || !HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(candidatePrefab))
				continue;

			if (IsEntityParentedToRoot(candidate, vehicle))
				return candidatePrefab;

			if (DistanceSq2D(origin, candidate.GetOrigin()) <= 10 * 10)
				return candidatePrefab;

			if (IsPointNearEntityBounds(candidate.GetOrigin(), vehicle, 8.0))
				return candidatePrefab;
		}

		return "";
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

		angles = HST_WorldPositionService.BuildUprightAnglesFromVector(vehicle.GetYawPitchRoll());
		if (angles[0] != 0 || angles[1] != 0 || angles[2] != 0)
			return angles;

		vector origin = vehicle.GetOrigin();
		int seed = Math.Round(origin[0]) * 13 + Math.Round(origin[2]) * 17 + prefab.Length() * 23;
		if (seed < 0)
			seed = -seed;

		angles[0] = seed - (seed / 360) * 360;
		return HST_WorldPositionService.BuildUprightAnglesFromVector(angles);
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
		string displayName = BuildDisplayName(item, prefab);
		string wearableCategory = HST_ArsenalItemFilter.ResolveWearableCategory(prefab, displayName);
		if (!wearableCategory.IsEmpty() && !HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(prefab, wearableCategory) && !HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(displayName, wearableCategory))
			return wearableCategory;

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

		if (HST_ArsenalItemFilter.IsMedicalItemToken(prefab, displayName))
			return "medical";

		if (HST_ArsenalItemFilter.IsKnownBackpackToken(prefab, displayName))
			return "backpack";

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

	protected bool RemoveLooseLootItem(IEntity item)
	{
		if (!item)
			return false;

		SCR_EntityHelper.DeleteEntityAndChildren(item);
		return true;
	}
}
