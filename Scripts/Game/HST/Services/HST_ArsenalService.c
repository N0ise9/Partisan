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

	bool WithdrawItem(HST_CampaignState state, string prefab, int amount)
	{
		if (!state || prefab.IsEmpty() || amount <= 0)
			return false;

		HST_ArsenalItemState item = state.FindArsenalItem(prefab);
		if (!item)
			return false;

		if (item.m_bUnlocked)
			return true;

		if (item.m_iCount < amount)
			return false;

		item.m_iCount -= amount;
		return true;
	}

	string WithdrawBestAvailableItem(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi arsenal | campaign state not ready";

		HST_ArsenalItemState selectedItem;
		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!item || item.m_iCount <= 0)
				continue;

			if (!selectedItem || ScoreItemForWithdrawal(item) > ScoreItemForWithdrawal(selectedItem))
				selectedItem = item;
		}

		if (!selectedItem)
			return "h-istasi arsenal | no stored item available";

		if (!WithdrawItem(state, selectedItem.m_sPrefab, 1))
			return "h-istasi arsenal | selected item could not be withdrawn";

		string label = selectedItem.m_sDisplayName;
		if (label.IsEmpty())
			label = selectedItem.m_sPrefab;

		if (selectedItem.m_bUnlocked)
			return string.Format("h-istasi arsenal | issued unlocked item %1", label);

		return string.Format("h-istasi arsenal | issued %1 | remaining %2", label, selectedItem.m_iCount);
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

		if (vehicle.m_iRedeployCost <= 0)
			vehicle.m_iRedeployCost = ResolveRedeployCost(vehicle);

		vehicle.m_bUnlocked = true;
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

	string BuildGarageReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi garage | campaign state not ready";

		string report = string.Format("h-istasi garage | vehicles %1 | emplacements %2 | ammo points %3", state.m_aGarageVehicles.Count(), state.m_aCapturedEmplacements.Count(), state.m_aAmmoPoints.Count());
		foreach (HST_GarageVehicleState vehicle : state.m_aGarageVehicles)
		{
			if (!vehicle)
				continue;

			report = report + string.Format("\n%1 | %2 | fuel %3 | armed %4", vehicle.m_sVehicleId, vehicle.m_sPrefab, vehicle.m_fFuel, vehicle.m_bArmed);
		}

		return report;
	}

	bool RedeployFirstGarageVehicle(HST_CampaignState state, HST_EconomyService economy, vector deployPosition)
	{
		if (!state || state.m_aGarageVehicles.Count() == 0)
			return false;

		HST_GarageVehicleState vehicle = state.m_aGarageVehicles[0];
		if (!vehicle || vehicle.m_sPrefab.IsEmpty())
			return false;

		int cost = vehicle.m_iRedeployCost;
		if (cost <= 0)
			cost = ResolveRedeployCost(vehicle);

		if (economy && state.m_iFactionMoney < cost)
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return false;

		GenericEntity entity = respawnSystem.DoSpawn(vehicle.m_sPrefab, deployPosition, vehicle.m_vAngles);
		if (!entity)
			return false;

		if (economy)
			economy.SpendFactionMoney(state, cost);

		state.m_aGarageVehicles.Remove(0);
		return true;
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

	protected int ScoreItemForWithdrawal(HST_ArsenalItemState item)
	{
		if (!item)
			return -1;

		int score = item.m_iCount;
		if (item.m_bUnlocked)
			score += 10000;
		if (item.m_sCategory == "weapon")
			score += 100;
		else if (item.m_sCategory == "launcher")
			score += 80;
		else if (item.m_sCategory == "magazine")
			score += 30;
		return score;
	}

	protected int ResolveRedeployCost(HST_GarageVehicleState vehicle)
	{
		if (!vehicle)
			return 100;

		if (vehicle.m_bArmed)
			return 250;

		return 100;
	}
}
