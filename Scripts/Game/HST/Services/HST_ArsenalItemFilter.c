class HST_ArsenalItemFilter
{
	static bool ShouldBlockArsenalPrefab(string prefab, string category = "", string displayName = "")
	{
		if (HasBlockedStructuralContainerToken(prefab, category) || HasBlockedStructuralContainerToken(displayName, category))
			return true;

		if (!ShouldProbeStructuralContainer(category) || IsKnownWearableContainer(prefab, displayName) || IsLoadoutClothingCategory(category))
			return false;

		if (!GetGame() || !GetGame().GetWorld())
			return false;

		ResourceName resourceName = prefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded || !loaded.IsValid())
			return false;

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = vector.Zero;
		IEntity temp = GetGame().SpawnEntityPrefabEx(resourceName, false, GetGame().GetWorld(), params);
		if (!temp)
			return false;

		bool blocked = ShouldBlockArsenalEntity(temp, prefab, category, displayName);
		SCR_EntityHelper.DeleteEntityAndChildren(temp);
		return blocked;
	}

	static bool ShouldBlockArsenalEntity(IEntity entity, string prefab = "", string category = "", string displayName = "")
	{
		if (HasBlockedStructuralContainerToken(prefab, category) || HasBlockedStructuralContainerToken(displayName, category))
			return true;

		if (!entity || !ShouldProbeStructuralContainer(category) || IsKnownWearableContainer(prefab, displayName) || IsLoadoutClothingCategory(category))
			return false;

		if (BaseLoadoutClothComponent.Cast(entity.FindComponent(BaseLoadoutClothComponent)))
			return false;

		if (HasStructuralAttachmentStorage(entity))
			return true;

		if ((category.IsEmpty() || category == "equipment" || category == "utility") && HasCargoDepositStorage(entity))
			return true;

		return false;
	}

	static bool HasBlockedStructuralContainerToken(string value, string category = "")
	{
		if (value.IsEmpty())
			return false;

		value.ToLower();
		if (value.Contains("personal belongings") || value.Contains("personalbelongings"))
			return true;

		if (value.Contains("compass case") || value.Contains("compasscase"))
			return true;

		if (value.Contains("suspenders"))
			return true;

		if (value.Contains("pouch") || value.Contains("holster") || value.Contains("bandolier") || value.Contains("bandoleer"))
			return true;

		if (value.Contains("buttpack") || value.Contains("butt pack"))
			return true;

		if (value.Contains("scabbard") || value.Contains("sheath"))
			return true;

		if (HasEntrenchingToolToken(value) && HasStructuralContainerQualifier(value))
			return true;

		if (value.Contains("carrier") && !IsLoadoutClothingCategory(category) && (HasEntrenchingToolToken(value) || value.Contains("compass") || value.Contains("radio")))
			return true;

		return false;
	}

	protected static bool HasEntrenchingToolToken(string value)
	{
		return value.Contains("etool") || value.Contains("e-tool") || value.Contains("e_tool") || value.Contains("e tool") || value.Contains("entrenchingtool") || value.Contains("entrenching_tool") || value.Contains("entrenching tool") || value.Contains("mpl50") || value.Contains("mpl_50") || value.Contains("mpl-50");
	}

	protected static bool HasStructuralContainerQualifier(string value)
	{
		return value.Contains("carrier") || value.Contains("case") || value.Contains("cover") || value.Contains("holder") || value.Contains("sleeve") || value.Contains("mount");
	}

	static bool IsMedicalItemToken(string prefab, string displayName = "")
	{
		prefab.ToLower();
		displayName.ToLower();
		return prefab.Contains("medicine") || prefab.Contains("fielddressing") || prefab.Contains("firstaid") || prefab.Contains("bandage") || prefab.Contains("morphine") || prefab.Contains("tourniquet") || prefab.Contains("medkit") || displayName.Contains("bandage") || displayName.Contains("morphine") || displayName.Contains("tourniquet") || displayName.Contains("medkit");
	}

	static bool IsKnownBackpackToken(string prefab, string displayName = "")
	{
		prefab.ToLower();
		displayName.ToLower();
		if (prefab.Contains("buttpack") || prefab.Contains("butt_pack") || displayName.Contains("buttpack") || displayName.Contains("butt pack"))
			return false;

		return prefab.Contains("backpack")
			|| prefab.Contains("/backpacks/")
			|| prefab.Contains("fieldpack")
			|| prefab.Contains("field_pack")
			|| prefab.Contains("field pack")
			|| prefab.Contains("pack_")
			|| prefab.Contains("ruck")
			|| displayName.Contains("field pack")
			|| displayName.Contains("ruck")
			|| ((prefab.Contains("alice") || displayName.Contains("alice")) && (prefab.Contains("pack") || displayName.Contains("pack")));
	}

	static bool IsKnownWebbingToken(string prefab, string displayName = "")
	{
		prefab.ToLower();
		displayName.ToLower();
		if (IsEntrenchingToolItemToken(prefab, displayName))
			return false;

		return prefab.Contains("webbing")
			|| prefab.Contains("chest_rig")
			|| prefab.Contains("chestrig")
			|| prefab.Contains("chest rig")
			|| prefab.Contains("lbe")
			|| prefab.Contains("loadbearing")
			|| prefab.Contains("load_bearing")
			|| prefab.Contains("harness")
			|| displayName.Contains("webbing")
			|| displayName.Contains("chest rig")
			|| displayName.Contains("lbe")
			|| displayName.Contains("load bearing")
			|| displayName.Contains("harness")
			|| prefab.Contains("alice")
			|| displayName.Contains("alice")
			|| displayName.Contains("grenadier vest")
			|| ((prefab.Contains("grenadier") || displayName.Contains("grenadier")) && (prefab.Contains("vest") || displayName.Contains("vest") || prefab.Contains("webbing") || displayName.Contains("webbing")));
	}

	static bool IsEntrenchingToolItemToken(string prefab, string displayName = "")
	{
		prefab.ToLower();
		displayName.ToLower();
		return prefab.Contains("etool")
			|| prefab.Contains("e_tool")
			|| prefab.Contains("entrenchingtool")
			|| prefab.Contains("entrenching_tool")
			|| prefab.Contains("mpl50")
			|| prefab.Contains("mpl_50")
			|| displayName.Contains("etool")
			|| displayName.Contains("e-tool")
			|| displayName.Contains("e tool")
			|| displayName.Contains("entrenching tool")
			|| displayName.Contains("mpl-50")
			|| displayName.Contains("mpl 50");
	}

	static string ResolveWearableCategory(string prefab, string displayName = "")
	{
		prefab.ToLower();
		displayName.ToLower();

		if (IsEntrenchingToolItemToken(prefab, displayName))
			return "";

		if (IsKnownBackpackToken(prefab, displayName))
			return "backpack";

		if (prefab.Contains("helmet") || prefab.Contains("hat") || prefab.Contains("headgear") || prefab.Contains("cap") || prefab.Contains("beanie") || prefab.Contains("boonie") || prefab.Contains("beret") || displayName.Contains("helmet") || displayName.Contains("hat") || displayName.Contains("cap"))
			return "headgear";

		if (prefab.Contains("pants") || prefab.Contains("trouser") || displayName.Contains("pants") || displayName.Contains("trouser"))
			return "pants";

		if (prefab.Contains("boot") || displayName.Contains("boot"))
			return "boots";

		if (prefab.Contains("glove") || prefab.Contains("handwear") || displayName.Contains("glove"))
			return "handwear";

		if (prefab.Contains("uniform") || prefab.Contains("jacket") || prefab.Contains("shirt") || prefab.Contains("clothes") || prefab.Contains("blouse") || prefab.Contains("parka") || prefab.Contains("coat") || displayName.Contains("uniform") || displayName.Contains("jacket") || displayName.Contains("shirt") || displayName.Contains("blouse") || displayName.Contains("parka") || displayName.Contains("coat"))
			return "clothing";

		if (IsKnownWebbingToken(prefab, displayName))
			return "webbing";

		if (prefab.Contains("vest") || displayName.Contains("vest"))
			return "vest";

		return "";
	}

	static bool IsLoadoutClothingCategory(string category)
	{
		return category == "clothing" || category == "headgear" || category == "vest" || category == "webbing" || category == "pants" || category == "boots" || category == "backpack" || category == "handwear";
	}

	protected static bool ShouldProbeStructuralContainer(string category)
	{
		if (IsLoadoutClothingCategory(category))
			return false;

		return category.IsEmpty() || category == "equipment" || category == "utility" || category == "magazine" || category == "attachment";
	}

	protected static bool IsKnownWearableContainer(string prefab, string displayName)
	{
		return !ResolveWearableCategory(prefab, displayName).IsEmpty();
	}

	protected static bool HasStructuralAttachmentStorage(IEntity entity)
	{
		array<IEntity> visited = {};
		return HasStructuralAttachmentStorageRecursive(entity, visited, 0);
	}

	protected static bool HasStructuralAttachmentStorageRecursive(IEntity entity, notnull array<IEntity> visited, int depth)
	{
		if (!entity || depth > 8 || visited.Find(entity) >= 0)
			return false;

		visited.Insert(entity);
		array<Managed> components = {};
		entity.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed component : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!storage || storage.GetSlotsCount() <= 0 || !IsStructuralAttachmentStorage(storage))
				continue;

			return true;
		}

		return false;
	}

	protected static bool HasCargoDepositStorage(IEntity entity)
	{
		array<IEntity> visited = {};
		return HasCargoDepositStorageRecursive(entity, visited, 0);
	}

	protected static bool HasCargoDepositStorageRecursive(IEntity entity, notnull array<IEntity> visited, int depth)
	{
		if (!entity || depth > 8 || visited.Find(entity) >= 0)
			return false;

		visited.Insert(entity);
		array<Managed> components = {};
		entity.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed component : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!storage || storage.GetSlotsCount() <= 0)
				continue;

			if (IsCargoDepositStorage(storage))
				return true;
		}

		return false;
	}

	protected static bool IsStructuralAttachmentStorage(BaseInventoryStorageComponent storage)
	{
		if (!storage || storage.GetSlotsCount() <= 0)
			return false;

		return SCR_Enum.HasFlag(storage.GetPurpose(), EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT);
	}

	protected static bool IsCargoDepositStorage(BaseInventoryStorageComponent storage)
	{
		if (!storage || storage.GetSlotsCount() <= 0)
			return false;

		if (SCR_SalineStorageComponent.Cast(storage) || SCR_TourniquetStorageComponent.Cast(storage))
			return false;

		if (IsStructuralAttachmentStorage(storage))
			return false;

		if (SCR_Enum.HasFlag(storage.GetPurpose(), EStoragePurpose.PURPOSE_DEPOSIT))
			return true;

		if (SCR_UniversalInventoryStorageComponent.Cast(storage))
			return true;

		return false;
	}
}
