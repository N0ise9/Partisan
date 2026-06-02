class HST_ArsenalService
{
	HST_ArsenalItemState DepositItem(HST_CampaignState state, HST_BalanceConfig balance, string prefab, int amount)
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

		item.m_iCount += amount;
		item.m_bUnlocked = item.m_iCount >= balance.m_iArsenalUnlockThreshold;
		return item;
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
}

