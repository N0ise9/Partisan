class HST_DisplayNameService
{
	static string ResolveItemDisplayName(IEntity item, string prefab, string existingName = "")
	{
		if (item)
		{
			InventoryItemComponent inventoryItem = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
			if (inventoryItem && inventoryItem.GetUIInfo())
			{
				string itemName = ResolveReadableDisplayName(inventoryItem.GetUIInfo().GetName());
				if (!itemName.IsEmpty())
					return itemName;
			}
		}

		string resolvedExisting = ResolveReadableDisplayName(existingName);
		if (!resolvedExisting.IsEmpty())
			return resolvedExisting;

		return FriendlyPrefabName(prefab, "item");
	}

	static string ResolveVehicleDisplayName(string prefab, string existingName = "")
	{
		string resolvedExisting = ResolveReadableDisplayName(existingName);
		if (!resolvedExisting.IsEmpty())
			return resolvedExisting;

		return FriendlyPrefabName(prefab, "vehicle");
	}

	static string ResolveShortItemDisplayName(string displayName, string prefab = "")
	{
		string resolved = ResolveReadableDisplayName(displayName);
		if (resolved.IsEmpty())
			resolved = FriendlyPrefabName(prefab, "item");

		resolved.Replace("Headgear", "");
		resolved.Replace("Backpack", "");
		resolved.Replace("Magazine", "Mag");
		resolved.Replace("Inventory", "");
		resolved.Replace("Equipment", "");
		resolved.Replace("Prefab", "");
		resolved.Replace("  ", " ");
		resolved.Replace("  ", " ");
		resolved = resolved.Trim();
		if (resolved.IsEmpty())
			return FriendlyPrefabName(prefab, "item");

		return resolved;
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
		name.Replace("#", "");
		name.Replace("-", " ");
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

	static bool LooksLikeLocalizationKey(string value)
	{
		return value.Length() > 0 && value.Substring(0, 1) == "#";
	}

	static string ResolveReadableDisplayName(string value)
	{
		if (value.IsEmpty() || LooksLikePrefabPath(value))
			return "";

		if (LooksLikeLocalizationKey(value))
		{
			string translated = WidgetManager.Translate(value);
			if (!translated.IsEmpty() && translated != value && !LooksLikeLocalizationKey(translated) && !LooksLikePrefabPath(translated))
				return NormalizeDisplayGlyphs(translated);

			return "";
		}

		return NormalizeDisplayGlyphs(value);
	}

	protected static string NormalizeKnownLabel(string name)
	{
		name = NormalizeDisplayGlyphs(name);
		name.Replace("EQUIPMENT", "");
		name.Replace("Equipment", "");
		name.Replace("Addon", "");
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
		name.Replace("  ", " ");
		name.Replace("  ", " ");
		return name.Trim();
	}

	protected static string NormalizeDisplayGlyphs(string name)
	{
		name.Replace("×", "x");
		return name;
	}
}
