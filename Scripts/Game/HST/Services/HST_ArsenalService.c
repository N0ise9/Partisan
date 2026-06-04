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

		string resolvedDisplayName = displayName;
		if (resolvedDisplayName.IsEmpty())
			resolvedDisplayName = item.m_sDisplayName;
		item.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, prefab, resolvedDisplayName);

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
		label = HST_DisplayNameService.ResolveItemDisplayName(null, selectedItem.m_sPrefab, label);

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
			label = HST_DisplayNameService.ResolveItemDisplayName(null, item.m_sPrefab, label);

			report = report + string.Format("\n%1 | %2 | count %3", label, item.m_sCategory, CountLabel(item));
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

			report = report + string.Format("\n%1 | %2 | cost %3 | fuel %4 | armed %5 | source %6/%7", vehicle.m_sVehicleId, GarageVehicleDisplayLabel(vehicle), vehicle.m_iRedeployCost, vehicle.m_fFuel, vehicle.m_bArmed, vehicle.m_sSourceZoneId, vehicle.m_sSourceFactionKey);
		}

		return report;
	}

	string RedeployGarageVehicle(HST_CampaignState state, HST_EconomyService economy, string vehicleId, vector deployPosition)
	{
		if (!state || state.m_aGarageVehicles.Count() == 0)
			return "h-istasi garage | failed: no stored vehicle";

		HST_GarageVehicleState vehicle = SelectGarageVehicle(state, vehicleId);
		if (!vehicle || vehicle.m_sPrefab.IsEmpty())
			return "h-istasi garage | failed: selected vehicle not found";

		int cost = vehicle.m_iRedeployCost;
		if (cost <= 0)
			cost = ResolveRedeployCost(vehicle);

		if (economy && state.m_iFactionMoney < cost)
			return string.Format("h-istasi garage | failed: redeploy requires $%1", cost);

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return "h-istasi garage | failed: respawn system not ready";

		GenericEntity entity = respawnSystem.DoSpawn(vehicle.m_sPrefab, deployPosition, vehicle.m_vAngles);
		if (!entity)
			return string.Format("h-istasi garage | failed: could not spawn %1", GarageVehicleDisplayLabel(vehicle));

		if (economy && !economy.SpendFactionMoney(state, cost))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			return string.Format("h-istasi garage | failed: redeploy requires $%1", cost);
		}

		string label = GarageVehicleDisplayLabel(vehicle);
		RemoveVehicle(state, vehicle.m_sVehicleId);
		return string.Format("h-istasi garage | redeployed %1 | complete", label);
	}

	bool RedeployFirstGarageVehicle(HST_CampaignState state, HST_EconomyService economy, vector deployPosition)
	{
		string result = RedeployGarageVehicle(state, economy, "", deployPosition);
		return result.Contains("complete");
	}

	protected HST_GarageVehicleState SelectGarageVehicle(HST_CampaignState state, string vehicleId)
	{
		if (!state)
			return null;

		if (!vehicleId.IsEmpty())
			return state.FindGarageVehicle(vehicleId);

		if (state.m_aGarageVehicles.Count() > 0)
			return state.m_aGarageVehicles[0];

		return null;
	}

	protected string GarageVehicleDisplayLabel(HST_GarageVehicleState vehicle)
	{
		if (!vehicle)
			return "vehicle";

		if (!vehicle.m_sDisplayName.IsEmpty())
			return HST_DisplayNameService.ResolveVehicleDisplayName(vehicle.m_sPrefab, vehicle.m_sDisplayName);

		if (!vehicle.m_sPrefab.IsEmpty())
			return HST_DisplayNameService.ResolveVehicleDisplayName(vehicle.m_sPrefab);

		return vehicle.m_sVehicleId;
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

	protected string CountLabel(HST_ArsenalItemState item)
	{
		if (!item)
			return "0";

		if (item.m_bUnlocked)
			return "INF";

		return string.Format("%1", item.m_iCount);
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
