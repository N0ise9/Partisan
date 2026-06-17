class HST_ArsenalService
{
	HST_ArsenalItemState DepositItem(HST_CampaignState state, HST_BalanceConfig balance, string prefab, int amount, string category = "equipment", string displayName = "")
	{
		if (!state || prefab.IsEmpty() || amount <= 0)
			return null;

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			Print("h-istasi arsenal | skipped invalid item resource deposit " + prefab);
			return null;
		}

		string depositReason;
		if (!CanDepositItem(balance, prefab, category, false, depositReason))
		{
			Print(string.Format("h-istasi arsenal | blocked item deposit %1 | %2", prefab, depositReason));
			return null;
		}

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
		if (!item.m_bUnlocked && balance)
			item.m_bUnlocked = ShouldUnlock(item, balance);
		return item;
	}

	HST_ArsenalItemState RefundItem(HST_CampaignState state, string prefab, int amount, string category = "equipment", string displayName = "")
	{
		if (!state || prefab.IsEmpty() || amount <= 0)
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
		return item;
	}

	bool CanDepositItem(HST_BalanceConfig balance, string prefab, string category, bool vehicleLoot, out string reason)
	{
		reason = "";
		if (prefab.IsEmpty())
		{
			reason = "empty item prefab";
			return false;
		}

		if (IsRawNonLootAsset(prefab))
		{
			reason = "raw visual/support asset is not loot";
			return false;
		}

		if (!balance)
			return true;

		HST_ArsenalItemRule rule = FindItemRule(balance, prefab, category, vehicleLoot);
		if (rule && ResolveUnlockPolicy(rule) == "blocked")
		{
			reason = "blocked by arsenal item rule";
			return false;
		}

		if (category == "explosive" && !balance.m_bAllowExplosiveUnlocks)
		{
			reason = "explosive deposits disabled by balance";
			return false;
		}

		if (IsGuidedLauncher(prefab) && !balance.m_bAllowGuidedLauncherUnlocks)
		{
			reason = "guided launcher deposits disabled by balance";
			return false;
		}

		return true;
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

	string BuildArsenalReport(HST_CampaignState state, HST_BalanceConfig balance = null)
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
			string policy = ResolveUnlockPolicy(FindItemRule(balance, item.m_sPrefab, item.m_sCategory, false));
			int threshold = ResolveUnlockThreshold(item, balance);
			string thresholdLabel = "n/a";
			if (threshold >= 0)
				thresholdLabel = string.Format("%1", threshold);

			report = report + string.Format("\n%1 | %2 | count %3 | policy %4 | threshold %5", label, item.m_sCategory, CountLabel(item), policy, thresholdLabel);
		}

		if (balance && balance.m_aArsenalItemRules.Count() > 0)
		{
			foreach (HST_ArsenalItemRule rule : balance.m_aArsenalItemRules)
			{
				if (!rule)
					continue;

				string policy = ResolveUnlockPolicy(rule);
				if (policy != "blocked" && policy != "finite_only")
					continue;

				string thresholdLabel = "n/a";
				if (rule.m_iUnlockThresholdOverride >= 0)
					thresholdLabel = string.Format("%1", rule.m_iUnlockThresholdOverride);

				report = report + string.Format("\npolicy hint | contains %1 | category %2 | policy %3 | threshold %4", rule.m_sPrefabContains, rule.m_sCategory, policy, thresholdLabel);
			}
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

	int CleanupInvalidGarageRecords(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int removed;
		for (int garageIndex = state.m_aGarageVehicles.Count() - 1; garageIndex >= 0; garageIndex--)
		{
			HST_GarageVehicleState vehicle = state.m_aGarageVehicles[garageIndex];
			if (vehicle && HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(vehicle.m_sPrefab))
				continue;

			string garagePrefab = "";
			string garageId = "";
			if (vehicle)
			{
				garagePrefab = vehicle.m_sPrefab;
				garageId = vehicle.m_sVehicleId;
			}

			state.m_aGarageVehicles.Remove(garageIndex);
			removed++;
			Print(string.Format("h-istasi garage | removed invalid stored vehicle record %1 | prefab %2", garageId, garagePrefab), LogLevel.WARNING);
		}

		for (int cargoIndex = state.m_aVehicleCargoItems.Count() - 1; cargoIndex >= 0; cargoIndex--)
		{
			HST_VehicleCargoItemState cargoItem = state.m_aVehicleCargoItems[cargoIndex];
			if (cargoItem && HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(cargoItem.m_sVehiclePrefab))
				continue;

			string cargoPrefab = "";
			string cargoId = "";
			if (cargoItem)
			{
				cargoPrefab = cargoItem.m_sVehiclePrefab;
				cargoId = cargoItem.m_sVehicleRuntimeId;
			}

			state.m_aVehicleCargoItems.Remove(cargoIndex);
			removed++;
			Print(string.Format("h-istasi vehicle cargo | removed invalid vehicle cargo record %1 | prefab %2", cargoId, cargoPrefab), LogLevel.WARNING);
		}

		return removed;
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

			report = report + string.Format("\n%1 | %2 | cost %3 | fuel %4 | damage %5 | armed %6 | cargo %7 | source %8/%9", vehicle.m_sVehicleId, GarageVehicleDisplayLabel(vehicle), vehicle.m_iRedeployCost, vehicle.m_fFuel, vehicle.m_sDamageState, vehicle.m_bArmed, CountStoredVehicleCargoItems(vehicle), vehicle.m_sSourceZoneId, vehicle.m_sSourceFactionKey);
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

		if (!HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(vehicle.m_sPrefab))
		{
			Print(string.Format("h-istasi garage | refused redeploy of invalid stored vehicle prefab %1 | vehicle %2", vehicle.m_sPrefab, vehicle.m_sVehicleId), LogLevel.WARNING);
			return string.Format("h-istasi garage | failed: stored prefab is not a valid vehicle root (%1)", GarageVehicleDisplayLabel(vehicle));
		}

		int cost = vehicle.m_iRedeployCost;
		if (cost <= 0)
			cost = ResolveRedeployCost(vehicle);

		if (economy && state.m_iFactionMoney < cost)
			return string.Format("h-istasi garage | failed: redeploy requires $%1", cost);

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return "h-istasi garage | failed: respawn system not ready";

		vector resolvedDeployPosition;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(deployPosition, resolvedDeployPosition, true))
			return string.Format("h-istasi garage | failed: no dry redeploy ground near %1", deployPosition);

		vector deployAngles = ResolveRedeployAngles(vehicle, resolvedDeployPosition);
		GenericEntity entity = HST_WorldPositionService.SpawnPrefab(vehicle.m_sPrefab, resolvedDeployPosition, deployAngles);
		if (!entity)
		{
			Print(string.Format("h-istasi garage | failed to spawn garage vehicle %1 | prefab %2 | position %3 | yaw %4", GarageVehicleDisplayLabel(vehicle), vehicle.m_sPrefab, resolvedDeployPosition, deployAngles[0]), LogLevel.WARNING);
			return string.Format("h-istasi garage | failed: could not spawn %1", GarageVehicleDisplayLabel(vehicle));
		}
		HST_WorldPositionService.ApplyUprightEntityTransform(entity, resolvedDeployPosition, deployAngles);

		RegisterRedeployedRuntimeVehicle(state, entity, vehicle, resolvedDeployPosition, deployAngles);
		string restoredRuntimeId;
		string restoreFailure;
		if (!RestoreStoredVehicleCargo(state, entity, vehicle, restoredRuntimeId, restoreFailure))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			return "h-istasi garage | failed: cargo restore failed | " + restoreFailure;
		}

		if (economy && !economy.SpendFactionMoney(state, cost))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			RemoveVehicleCargoByRuntimeId(state, restoredRuntimeId);
			return string.Format("h-istasi garage | failed: redeploy requires $%1", cost);
		}

		string label = GarageVehicleDisplayLabel(vehicle);
		RemoveVehicle(state, vehicle.m_sVehicleId);
		Print(string.Format("h-istasi garage | redeployed %1 | prefab %2 | position %3 | yaw %4 | cost %5 | restored cargo %6 to %7", label, vehicle.m_sPrefab, resolvedDeployPosition, deployAngles[0], cost, CountStoredVehicleCargoItems(vehicle), restoredRuntimeId));
		return string.Format("h-istasi garage | redeployed %1 | restored cargo %2 | complete", label, CountStoredVehicleCargoItems(vehicle));
	}

	bool RedeployFirstGarageVehicle(HST_CampaignState state, HST_EconomyService economy, vector deployPosition)
	{
		string result = RedeployGarageVehicle(state, economy, "", deployPosition);
		return result.Contains("complete");
	}

	protected bool RestoreStoredVehicleCargo(HST_CampaignState state, IEntity spawnedVehicle, HST_GarageVehicleState garageVehicle, out string restoredRuntimeId, out string failure)
	{
		restoredRuntimeId = "";
		failure = "";
		if (!state || !spawnedVehicle || !garageVehicle)
		{
			failure = "missing spawned vehicle or garage state";
			return false;
		}

		restoredRuntimeId = ResolveSpawnedVehicleRuntimeId(spawnedVehicle);
		if (restoredRuntimeId.IsEmpty())
		{
			failure = "spawned vehicle runtime id not available";
			return false;
		}

		foreach (HST_StoredVehicleCargoState storedCargo : garageVehicle.m_aStoredCargoItems)
		{
			if (!storedCargo || storedCargo.m_sItemPrefab.IsEmpty() || storedCargo.m_iCount <= 0)
				continue;

			HST_VehicleCargoItemState cargoItem = state.FindVehicleCargoItem(restoredRuntimeId, storedCargo.m_sItemPrefab);
			if (!cargoItem)
			{
				cargoItem = new HST_VehicleCargoItemState();
				cargoItem.m_sVehicleRuntimeId = restoredRuntimeId;
				cargoItem.m_sItemPrefab = storedCargo.m_sItemPrefab;
				state.m_aVehicleCargoItems.Insert(cargoItem);
			}

			cargoItem.m_sVehiclePrefab = garageVehicle.m_sPrefab;
			cargoItem.m_sVehicleDisplayName = GarageVehicleDisplayLabel(garageVehicle);
			cargoItem.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, storedCargo.m_sItemPrefab, storedCargo.m_sDisplayName);
			cargoItem.m_sCategory = storedCargo.m_sCategory;
			cargoItem.m_iCount += storedCargo.m_iCount;
			cargoItem.m_iLastStoredAtSecond = state.m_iElapsedSeconds;
			cargoItem.m_vLastVehiclePosition = spawnedVehicle.GetOrigin();
		}

		return true;
	}

	protected string ResolveSpawnedVehicleRuntimeId(IEntity vehicle)
	{
		if (!vehicle)
			return "";

		BaseRplComponent rpl = BaseRplComponent.Cast(vehicle.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("rpl_%1", rpl.Id());

		return string.Format("local_%1_%2", ResolvePrefabName(vehicle), vehicle.GetOrigin());
	}

	protected string ResolvePrefabName(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "";

		return entity.GetPrefabData().GetPrefabName();
	}

	protected int RemoveVehicleCargoByRuntimeId(HST_CampaignState state, string vehicleRuntimeId)
	{
		if (!state || vehicleRuntimeId.IsEmpty())
			return 0;

		int removed;
		for (int i = state.m_aVehicleCargoItems.Count() - 1; i >= 0; i--)
		{
			HST_VehicleCargoItemState cargoItem = state.m_aVehicleCargoItems[i];
			if (!cargoItem || cargoItem.m_sVehicleRuntimeId != vehicleRuntimeId)
				continue;

			state.m_aVehicleCargoItems.Remove(i);
			removed++;
		}

		return removed;
	}

	protected int CountStoredVehicleCargoItems(HST_GarageVehicleState vehicle)
	{
		if (!vehicle)
			return 0;

		int count;
		foreach (HST_StoredVehicleCargoState storedCargo : vehicle.m_aStoredCargoItems)
		{
			if (storedCargo)
				count += Math.Max(1, storedCargo.m_iCount);
		}

		return count;
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

	protected vector ResolveRedeployAngles(HST_GarageVehicleState vehicle, vector deployPosition)
	{
		if (!vehicle)
			return BuildFallbackRedeployAngles("", deployPosition);

		if (!IsZeroAngles(vehicle.m_vAngles))
			return HST_WorldPositionService.BuildUprightAnglesFromVector(vehicle.m_vAngles);

		return BuildFallbackRedeployAngles(vehicle.m_sVehicleId + vehicle.m_sPrefab, deployPosition);
	}

	protected bool IsZeroAngles(vector angles)
	{
		return angles[0] == 0 && angles[1] == 0 && angles[2] == 0;
	}

	protected vector BuildFallbackRedeployAngles(string salt, vector deployPosition)
	{
		vector angles;
		int seed = salt.Length() * 41 + Math.Round(deployPosition[0]) * 11 + Math.Round(deployPosition[2]) * 17;
		if (seed < 0)
			seed = -seed;

		angles[0] = seed - (seed / 360) * 360;
		return HST_WorldPositionService.BuildUprightAnglesFromVector(angles);
	}

	protected void RegisterRedeployedRuntimeVehicle(HST_CampaignState state, IEntity entity, HST_GarageVehicleState garageVehicle, vector position, vector angles)
	{
		if (!state || !entity || !garageVehicle)
			return;

		string runtimeId = ResolveSpawnedVehicleRuntimeId(entity);
		if (runtimeId.IsEmpty())
			return;

		HST_RuntimeVehicleState runtimeVehicle = state.FindRuntimeVehicle(runtimeId);
		if (!runtimeVehicle)
		{
			runtimeVehicle = new HST_RuntimeVehicleState();
			runtimeVehicle.m_sVehicleRuntimeId = runtimeId;
			state.m_aRuntimeVehicles.Insert(runtimeVehicle);
		}

		runtimeVehicle.m_sPrefab = garageVehicle.m_sPrefab;
		runtimeVehicle.m_sDisplayName = GarageVehicleDisplayLabel(garageVehicle);
		runtimeVehicle.m_sFactionKey = garageVehicle.m_sSourceFactionKey;
		runtimeVehicle.m_sZoneId = garageVehicle.m_sSourceZoneId;
		runtimeVehicle.m_sRuntimeKind = "garage_redeploy";
		runtimeVehicle.m_vPosition = position;
		runtimeVehicle.m_vAngles = angles;
		runtimeVehicle.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		runtimeVehicle.m_bDeleted = false;
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

	HST_ArsenalItemRule FindItemRule(HST_BalanceConfig balance, string prefab, string category = "", bool vehicleLoot = false)
	{
		if (!balance || prefab.IsEmpty())
			return null;

		foreach (HST_ArsenalItemRule rule : balance.m_aArsenalItemRules)
		{
			if (!rule)
				continue;

			if (vehicleLoot && !rule.m_bAppliesToVehicleLoot)
				continue;

			if (!vehicleLoot && !rule.m_bAppliesToAreaLoot)
				continue;

			if (!rule.m_sPrefabContains.IsEmpty() && !prefab.Contains(rule.m_sPrefabContains))
				continue;

			if (!rule.m_sCategory.IsEmpty())
			{
				if (category.IsEmpty() || rule.m_sCategory != category)
					continue;
			}

			return rule;
		}

		return null;
	}

	string ResolveUnlockPolicy(HST_ArsenalItemRule rule)
	{
		if (!rule || rule.m_sPolicy.IsEmpty())
			return "unlock";

		if (rule.m_sPolicy == "blocked" || rule.m_sPolicy == "finite_only" || rule.m_sPolicy == "unlock")
			return rule.m_sPolicy;

		return "unlock";
	}

	int ResolveUnlockThreshold(HST_ArsenalItemState item, HST_BalanceConfig balance)
	{
		if (!item || !balance)
			return -1;

		HST_ArsenalItemRule rule = FindItemRule(balance, item.m_sPrefab, item.m_sCategory, false);
		if (rule && rule.m_iUnlockThresholdOverride >= 0)
			return rule.m_iUnlockThresholdOverride;

		int threshold = balance.m_iArsenalUnlockThreshold;
		if (item.m_sCategory == "magazine")
			threshold = threshold * Math.Max(1, balance.m_iMagazineUnlockMultiplier);

		return threshold;
	}

	protected bool ShouldUnlock(HST_ArsenalItemState item, HST_BalanceConfig balance)
	{
		if (!item || !balance)
			return false;

		string policy = ResolveUnlockPolicy(FindItemRule(balance, item.m_sPrefab, item.m_sCategory, false));
		if (policy == "blocked" || policy == "finite_only")
			return false;

		int threshold = ResolveUnlockThreshold(item, balance);
		return threshold <= 0 || item.m_iCount >= threshold;
	}

	protected bool IsRawNonLootAsset(string prefab)
	{
		if (prefab.IsEmpty())
			return true;

		if (prefab.Contains("Assets/Images/"))
			return true;

		if (prefab.Contains("Assets/Objects/"))
			return true;

		if (prefab.Contains(".png") || prefab.Contains(".edds") || prefab.Contains(".xob") || prefab.Contains(".fbx") || prefab.Contains(".txo"))
			return true;

		return false;
	}

	protected bool IsGuidedLauncher(string prefab)
	{
		return prefab.Contains("ATGM") || prefab.Contains("Javelin") || prefab.Contains("Stinger") || prefab.Contains("Igla") || prefab.Contains("Metis") || prefab.Contains("guided") || prefab.Contains("Guided");
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
