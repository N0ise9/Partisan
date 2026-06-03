class HST_ArsenalService
{
	HST_ArsenalItemState DepositItem(HST_CampaignState state, HST_BalanceConfig balance, string prefab, int amount, string category = "equipment", string displayName = "")
	{
		if (prefab.IsEmpty() || amount <= 0)
			return null;

		HST_ArsenalItemState item = state.FindArsenalItem(prefab);
		if (!item)
		{
			item = new HST_ArsenalItemState();
			item.m_sPrefab = prefab;
			state.m_aArsenalItems.Insert(item);
		}

		if (!category.IsEmpty())
			item.m_sCategory = category;

		if (!displayName.IsEmpty())
			item.m_sDisplayName = displayName;
		else if (item.m_sDisplayName.IsEmpty())
			item.m_sDisplayName = prefab;

		item.m_iCount += amount;
		item.m_bUnlocked = ShouldUnlock(item, balance);
		return item;
	}

	bool IsItemUnlocked(HST_CampaignState state, string prefab)
	{
		if (!state || prefab.IsEmpty())
			return false;

		HST_ArsenalItemState item = state.FindArsenalItem(prefab);
		return item && item.m_bUnlocked;
	}

	string BuildArsenalReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi arsenal | campaign state not ready";

		string report = string.Format("h-istasi arsenal | tracked items %1", state.m_aArsenalItems.Count());
		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!item)
				continue;

			string label = item.m_sDisplayName;
			if (label.IsEmpty())
				label = item.m_sPrefab;

			report = report + string.Format("\n%1 | %2 | count %3 | unlocked %4", label, item.m_sCategory, item.m_iCount, item.m_bUnlocked);
		}

		return report;
	}

	bool StoreVehicle(HST_CampaignState state, HST_GarageVehicleState vehicle)
	{
		if (!vehicle || vehicle.m_sVehicleId.IsEmpty() || vehicle.m_sPrefab.IsEmpty() || state.FindGarageVehicle(vehicle.m_sVehicleId))
			return false;

		state.m_aGarageVehicles.Insert(vehicle);
		return true;
	}

	HST_GarageVehicleState RemoveVehicle(HST_CampaignState state, string vehicleId)
	{
		for (int i = 0; i < state.m_aGarageVehicles.Count(); i++)
		{
			HST_GarageVehicleState vehicle = state.m_aGarageVehicles[i];
			if (vehicle.m_sVehicleId != vehicleId)
				continue;

			state.m_aGarageVehicles.Remove(i);
			return vehicle;
		}

		return null;
	}

	protected bool ShouldUnlock(HST_ArsenalItemState item, HST_BalanceConfig balance)
	{
		if (!item || !balance)
			return false;

		int threshold = balance.m_iArsenalUnlockThreshold;
		if (item.m_sCategory == "magazine")
			threshold = threshold * Math.Max(1, balance.m_iMagazineUnlockMultiplier);

		return threshold <= 0 || item.m_iCount >= threshold;
	}
}
