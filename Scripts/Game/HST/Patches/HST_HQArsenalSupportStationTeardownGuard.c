modded class SCR_BaseItemSupportStationComponent
{
	protected bool m_bHSTHQArsenalTeardownShield;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!owner)
			return;

		HST_HQArsenalActionFilterComponent filter = HST_HQArsenalActionFilterComponent.Cast(
			owner.FindComponent(HST_HQArsenalActionFilterComponent));
		m_bHSTHQArsenalTeardownShield = filter != null;
	}

	override void OnDelete(IEntity owner)
	{
		if (!m_EntityCatalogManager && m_bHSTHQArsenalTeardownShield)
			return;

		super.OnDelete(owner);
	}
}
