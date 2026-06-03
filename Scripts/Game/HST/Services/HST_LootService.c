class HST_LootResult
{
	int m_iSourcesScanned;
	int m_iItemsSeen;
	int m_iItemsDeposited;
	int m_iItemsRemoved;
	int m_iItemsSkippedUnlocked;
	int m_iItemsSkippedBlocked;
	int m_iItemsSkippedFriendly;
	ref array<string> m_aDepositedLines = {};

	string BuildSummary()
	{
		string summary = string.Format("h-istasi loot | scanned %1 source(s) | seen %2 item(s) | deposited %3 | removed %4", m_iSourcesScanned, m_iItemsSeen, m_iItemsDeposited, m_iItemsRemoved);
		summary = summary + string.Format(" | skipped unlocked %1 | blocked %2 | friendly/player %3", m_iItemsSkippedUnlocked, m_iItemsSkippedBlocked, m_iItemsSkippedFriendly);
		foreach (string line : m_aDepositedLines)
			summary = summary + "\n" + line;

		return summary;
	}
}

class HST_LootService
{
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

	protected string ResolvePrefabName(IEntity item)
	{
		if (!item || !item.GetPrefabData())
			return "";

		return item.GetPrefabData().GetPrefabName();
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
