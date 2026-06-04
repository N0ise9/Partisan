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
			if (!inventory)
				continue;

			if (!IsEligibleLootSource(source, preset, result))
				continue;

			result.m_iSourcesScanned++;
			CollectInventoryItems(source, inventory, state, balance, arsenal, result);
		}

		if (result.m_iItemsDeposited == 0 && result.m_aDepositedLines.Count() == 0)
			result.m_aDepositedLines.Insert("no eligible loot found nearby");

		return result;
	}

	bool CaptureNearbyVehicleToGarage(HST_CampaignState state, HST_CampaignPreset preset, HST_ArsenalService arsenal, int playerId)
	{
		if (!state || !preset || !arsenal || playerId <= 0)
			return false;

		IEntity playerEntity = ResolveLivingPlayerEntity(playerId);
		if (!playerEntity)
			return false;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(playerEntity.GetOrigin(), GARAGE_CAPTURE_RADIUS_METERS, AddLootCandidate, null, EQueryEntitiesFlags.ALL);

		IEntity selectedVehicle;
		float bestDistanceSq = 999999.0;
		foreach (IEntity candidate : m_aScanEntities)
		{
			if (!IsEligibleGarageVehicle(candidate, playerEntity, preset))
				continue;

			float distanceSq = DistanceSq2D(playerEntity.GetOrigin(), candidate.GetOrigin());
			if (distanceSq < bestDistanceSq)
			{
				selectedVehicle = candidate;
				bestDistanceSq = distanceSq;
			}
		}

		if (!selectedVehicle)
			return false;

		HST_GarageVehicleState vehicle = new HST_GarageVehicleState();
		vehicle.m_sVehicleId = string.Format("garage_%1_%2", state.m_iElapsedSeconds, state.m_aGarageVehicles.Count());
		vehicle.m_sPrefab = ResolvePrefabName(selectedVehicle);
		vehicle.m_sDisplayName = BuildVehicleDisplayName(selectedVehicle, vehicle.m_sPrefab);
		vehicle.m_sSourceZoneId = FindNearestZoneId(state, selectedVehicle.GetOrigin());
		vehicle.m_sSourceFactionKey = ResolveFactionKey(selectedVehicle);
		vehicle.m_iStoredAtSecond = state.m_iElapsedSeconds;
		vehicle.m_iRedeployCost = ResolveGarageRedeployCost(vehicle.m_sPrefab);
		vehicle.m_vPosition = selectedVehicle.GetOrigin();
		vehicle.m_vAngles = "0 0 0";
		vehicle.m_fFuel = 1.0;
		vehicle.m_bArmed = IsLikelyArmedVehicle(vehicle.m_sPrefab);
		if (!arsenal.StoreVehicle(state, vehicle))
			return false;

		SCR_EntityHelper.DeleteEntityAndChildren(selectedVehicle);
		return true;
	}

	HST_LootResult CollectNearbyLootToVehicle(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ArsenalService arsenal, int playerId)
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

		IEntity vehicle = FindNearestLootVehicle(playerEntity, balance.m_iVehicleLootRadiusMeters);
		if (!vehicle)
		{
			result.m_aDepositedLines.Insert("no eligible vehicle nearby");
			return result;
		}

		string vehicleId = ResolveVehicleRuntimeId(vehicle);
		string vehiclePrefab = ResolvePrefabName(vehicle);
		string vehicleName = BuildVehicleDisplayName(vehicle, vehiclePrefab);
		result.m_sVehicleRuntimeId = vehicleId;
		result.m_sVehicleDisplayName = vehicleName;

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
			if (!inventory)
				continue;

			if (!IsEligibleLootSource(source, preset, result))
				continue;

			result.m_iSourcesScanned++;
			CollectInventoryItemsToVehicle(source, inventory, vehicle, vehicleId, vehiclePrefab, vehicleName, state, balance, arsenal, result);
		}

		if (result.m_iItemsDeposited == 0 && result.m_aDepositedLines.Count() == 0)
			result.m_aDepositedLines.Insert("no eligible loot found near vehicle");

		return result;
	}

	string UnloadNearestVehicleCargoToArsenal(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ArsenalService arsenal, int playerId)
	{
		if (!state || !preset || !balance || !arsenal || playerId <= 0)
			return "h-istasi vehicle loot | service not ready";

		IEntity playerEntity = ResolveLivingPlayerEntity(playerId);
		if (!playerEntity)
			return "h-istasi vehicle loot | no living player entity found";

		IEntity vehicle = FindNearestLootVehicle(playerEntity, balance.m_iVehicleLootRadiusMeters);
		if (!vehicle)
			return "h-istasi vehicle loot | no eligible vehicle nearby";

		string vehicleId = ResolveVehicleRuntimeId(vehicle);
		int deposited;
		int entries;
		for (int i = state.m_aVehicleCargoItems.Count() - 1; i >= 0; i--)
		{
			HST_VehicleCargoItemState cargoItem = state.m_aVehicleCargoItems[i];
			if (!cargoItem || cargoItem.m_sVehicleRuntimeId != vehicleId)
				continue;

			HST_ArsenalItemState depositedItem = arsenal.DepositItem(state, balance, cargoItem.m_sItemPrefab, cargoItem.m_iCount, cargoItem.m_sCategory, cargoItem.m_sDisplayName);
			if (depositedItem)
			{
				deposited += cargoItem.m_iCount;
				entries++;
				state.m_aVehicleCargoItems.Remove(i);
			}
		}

		if (deposited <= 0)
			return "h-istasi vehicle loot | no stored cargo found in nearest vehicle";

		return string.Format("h-istasi vehicle loot | unloaded %1 item(s) from %2 cargo entries to arsenal", deposited, entries);
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

			report = report + string.Format("\n%1 | %2 | %3 | count %4", cargoItem.m_sVehicleDisplayName, cargoItem.m_sDisplayName, cargoItem.m_sCategory, cargoItem.m_iCount);
		}

		return report;
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

	protected bool IsEligibleLootSource(IEntity source, HST_CampaignPreset preset, HST_LootResult result)
	{
		if (!source)
			return false;

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
		cargoItem.m_sVehicleDisplayName = vehicleName;
		cargoItem.m_sCategory = category;
		cargoItem.m_sDisplayName = displayName;
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

		if (prefab.Contains("Character") || prefab.Contains("Inventory") || prefab.Contains("Weapon") || prefab.Contains("Magazine"))
			return false;

		if (!prefab.Contains("Vehicles") && !prefab.Contains("Vehicle"))
			return false;

		string factionKey = ResolveFactionKey(entity);
		if (preset && factionKey == preset.m_sResistanceFactionKey)
			return false;

		return true;
	}

	protected IEntity FindNearestLootVehicle(IEntity playerEntity, int radiusMeters)
	{
		if (!playerEntity)
			return null;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		m_aScanEntities.Clear();
		world.QueryEntitiesBySphere(playerEntity.GetOrigin(), radiusMeters, AddLootCandidate, null, EQueryEntitiesFlags.ALL);

		IEntity selectedVehicle;
		float bestDistanceSq = 999999999.0;
		foreach (IEntity candidate : m_aScanEntities)
		{
			if (!IsEligibleLootVehicle(candidate, playerEntity))
				continue;

			float distanceSq = DistanceSq2D(playerEntity.GetOrigin(), candidate.GetOrigin());
			if (distanceSq < bestDistanceSq)
			{
				selectedVehicle = candidate;
				bestDistanceSq = distanceSq;
			}
		}

		return selectedVehicle;
	}

	protected bool IsEligibleLootVehicle(IEntity entity, IEntity playerEntity)
	{
		if (!entity || entity == playerEntity)
			return false;

		string prefab = ResolvePrefabName(entity);
		if (prefab.IsEmpty())
			return false;

		if (prefab.Contains("Character") || prefab.Contains("Inventory") || prefab.Contains("Weapon") || prefab.Contains("Magazine"))
			return false;

		return prefab.Contains("Vehicles") || prefab.Contains("Vehicle");
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

	protected string ResolveFactionKey(IEntity entity)
	{
		if (!entity)
			return "";

		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(entity.FindComponent(FactionAffiliationComponent));
		if (!factionComponent)
			return "";

		return factionComponent.GetAffiliatedFactionKey();
	}

	protected string BuildVehicleDisplayName(IEntity entity, string prefab)
	{
		if (!prefab.IsEmpty())
			return prefab;

		if (entity)
			return entity.GetName();

		return "captured vehicle";
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
		InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (inventoryItem && inventoryItem.GetUIInfo())
		{
			string itemName = inventoryItem.GetUIInfo().GetName();
			if (!itemName.IsEmpty())
				return itemName;
		}

		return prefab;
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
}
