modded class SCR_BaseItemSupportStationComponent
{
	protected const ResourceName HQ_ARSENAL_PREFAB = "{6985327711303400}Prefabs/Objects/HST/HST_HQArsenal.et";
	protected bool m_bHSTHQArsenalTeardownShield;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!owner || !owner.GetPrefabData())
			return;

		m_bHSTHQArsenalTeardownShield = owner.GetPrefabData().GetPrefabName() == HQ_ARSENAL_PREFAB;
	}

	override void OnDelete(IEntity owner)
	{
		if (!m_EntityCatalogManager && m_bHSTHQArsenalTeardownShield)
			return;

		super.OnDelete(owner);
	}
}
