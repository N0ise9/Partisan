class HST_DisplayNameService
{
	static string ResolveItemDisplayName(IEntity item, string prefab, string existingName = "")
	{
		if (!existingName.IsEmpty() && !LooksLikePrefabPath(existingName))
			return existingName;

		if (item)
		{
			InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			if (inventoryItem && inventoryItem.GetUIInfo())
			{
				string itemName = inventoryItem.GetUIInfo().GetName();
				if (!itemName.IsEmpty() && !LooksLikePrefabPath(itemName))
					return itemName;
			}
		}

		return FriendlyPrefabName(prefab, "item");
	}

	static string ResolveVehicleDisplayName(string prefab, string existingName = "")
	{
		if (!existingName.IsEmpty() && !LooksLikePrefabPath(existingName))
			return existingName;

		return FriendlyPrefabName(prefab, "vehicle");
	}

	static string FriendlyPrefabName(string prefab, string fallback = "item")
	{
		if (prefab.IsEmpty())
			return fallback;

		string name = prefab;
		array<string> resourceParts = {};
		name.Split("}", resourceParts, false);
		if (resourceParts.Count() > 1)
			name = resourceParts[resourceParts.Count() - 1];

		array<string> pathParts = {};
		name.Split("/", pathParts, false);
		if (pathParts.Count() > 0)
			name = pathParts[pathParts.Count() - 1];

		name.Replace(".et", "");
		name.Replace("_Random", "");
		name.Replace("_baseLoadout", "");
		name.Replace("_base", "");
		name.Replace("Character_", "");
		name.Replace("Rifle_", "");
		name.Replace("Weapon_", "");
		name.Replace("Magazine_", "");
		name.Replace("Vehicle_", "");
		name.Replace("Car_", "");
		name.Replace("Item_", "");
		name.Replace("Inventory_", "");
		name.Replace("Prefabs", "");
		name.Replace("_", " ");
		name = NormalizeKnownLabel(name.Trim());
		if (name.IsEmpty())
			return fallback;

		return name;
	}

	static bool LooksLikePrefabPath(string value)
	{
		return value.Contains("Prefabs/") || value.Contains(".et") || value.Contains("{");
	}

	protected static string NormalizeKnownLabel(string name)
	{
		name.Replace("M27IAR", "M27 IAR");
		name.Replace("M38SDMR", "M38 SDMR");
		name.Replace("M16A4", "M16A4");
		name.Replace("M4A1", "M4A1");
		name.Replace("AK74", "AK-74");
		name.Replace("AK 74", "AK-74");
		name.Replace("UAZ469", "UAZ-469");
		name.Replace("Ural4320", "Ural-4320");
		name.Replace("M1025", "M1025");
		name.Replace("M1151", "M1151");
		name.Replace("S105", "S105");
		name.Replace("S1203", "S1203");
		name.Replace("RHS RF", "RHS");
		return name;
	}
}
