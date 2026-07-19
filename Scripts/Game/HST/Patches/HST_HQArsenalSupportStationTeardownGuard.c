modded class SCR_BaseItemSupportStationComponent
{
	override void OnDelete(IEntity owner)
	{
		if (!m_EntityCatalogManager
			&& owner
			&& HST_HQArsenalActionFilterComponent.Cast(
				owner.FindComponent(HST_HQArsenalActionFilterComponent)))
			return;

		super.OnDelete(owner);
	}
}
