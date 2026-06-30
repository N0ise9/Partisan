class HST_ArsenalItemFilter
{
	static bool ShouldBlockArsenalPrefab(string prefab, string category = "", string displayName = "")
	{
		if (HasBlockedStructuralContainerToken(prefab, category) || HasBlockedStructuralContainerToken(displayName, category))
			return true;

		if (!ShouldProbeStructuralContainer(category) || IsKnownWearableContainer(prefab, displayName))
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

		if (!entity || !ShouldProbeStructuralContainer(category) || IsKnownWearableContainer(prefab, displayName))
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

		if (value.Contains("scabbard") || value.Contains("sheath"))
			return true;

		if (value.Contains("etool") || value.Contains("e-tool") || value.Contains("entrenching tool"))
			return true;

		if (value.Contains("carrier") && !IsLoadoutClothingCategory(category))
			return true;

		return false;
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
		return prefab.Contains("backpack") || prefab.Contains("fieldpack") || prefab.Contains("field pack") || prefab.Contains("pack_") || displayName.Contains("field pack");
	}

	static bool IsLoadoutClothingCategory(string category)
	{
		return category == "clothing" || category == "headgear" || category == "vest" || category == "pants" || category == "boots" || category == "backpack" || category == "handwear";
	}

	protected static bool ShouldProbeStructuralContainer(string category)
	{
		return category.IsEmpty() || category == "equipment" || category == "utility" || category == "magazine" || category == "attachment" || category == "vest" || category == "backpack";
	}

	protected static bool IsKnownWearableContainer(string prefab, string displayName)
	{
		return IsKnownBackpackToken(prefab, displayName);
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
