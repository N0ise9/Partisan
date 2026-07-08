class HST_ForceCompositionService
{
	static const string INTENT_PATROL = "hst_patrol";
	static const string INTENT_SEARCH_PATROL = "hst_search_patrol";
	static const string INTENT_ROADBLOCK = "hst_roadblock";
	static const string INTENT_CHECKPOINT = "hst_checkpoint";
	static const string INTENT_GARRISON = "hst_garrison";
	static const string INTENT_REBUILD_GARRISON = "hst_rebuild_garrison";
	static const string INTENT_QRF_REGULAR = "hst_qrf_regular";
	static const string INTENT_QRF_ELITE = "hst_qrf_elite";
	static const string INTENT_COUNTERATTACK = "hst_counterattack";
	static const string INTENT_PETROS_ATTACK = "hst_defend_petros_attack";
	static const string INTENT_CONVOY_GUARDS = "hst_convoy_guards";
	static const string INTENT_CONVOY_ARMORED = "hst_convoy_armored";
	static const string INTENT_TOWN_POLICE = "hst_town_police";

	HST_ForceCompositionResult Compose(HST_CampaignState state, HST_CampaignPreset preset, HST_ForceRequest request)
	{
		HST_ForceCompositionResult result = new HST_ForceCompositionResult();
		if (!request)
			return Fail(result, "force request missing");

		result.m_sRequestId = request.m_sRequestId;
		result.m_sFactionKey = request.m_sFactionKey;
		result.m_sIntentId = NormalizeIntent(request.m_sIntentId);
		result.m_sSourceZoneId = request.m_sSourceZoneId;
		result.m_sTargetZoneId = request.m_sTargetZoneId;

		if (result.m_sRequestId.IsEmpty())
			result.m_sRequestId = BuildRequestId(state, result.m_sFactionKey, result.m_sIntentId);
		if (result.m_sFactionKey.IsEmpty())
			return Fail(result, "faction key missing");

		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(result.m_sFactionKey);
		if (!faction)
			return Fail(result, "faction template missing for " + result.m_sFactionKey);

		result.m_iWarLevel = ResolveWarLevel(state, request);
		result.m_iBudget = ResolveBudget(request, result.m_iWarLevel);
		result.m_sSelectedTier = ResolveTier(result.m_iWarLevel);

		string capabilityFailure = ResolveCapabilityFailure(preset, faction, request, result.m_sIntentId);
		if (!capabilityFailure.IsEmpty())
			return Fail(result, capabilityFailure);

		bool allowInfantry = request.m_bAllowInfantry;
		if (!allowInfantry && !request.m_bAllowVehicles && !request.m_bAllowStatics)
			return Fail(result, "request disables infantry, vehicles, and statics");

		int minManpower = ResolveMinManpower(request, result.m_sIntentId, result.m_iWarLevel);
		int maxManpower = ResolveMaxManpower(request, result.m_sIntentId, result.m_iWarLevel, minManpower);
		int desiredManpower = ResolveDesiredManpower(request, result.m_sIntentId, result.m_iWarLevel, minManpower, maxManpower);

		if (allowInfantry)
			BuildGroupPlans(faction, request, result, minManpower, maxManpower, desiredManpower);

		if (request.m_bAllowVehicles)
			BuildVehiclePlans(faction, request, result);

		bool hasGroup = result.GetPrimaryGroup() != null;
		bool hasVehicle = result.GetPrimaryVehicle() != null;
		if (allowInfantry && !hasGroup)
			return Fail(result, BuildNoGroupFailure(result));
		if (!allowInfantry && request.m_bAllowVehicles && !hasVehicle)
			return Fail(result, BuildNoVehicleFailure(result));

		result.m_bSuccess = true;
		result.m_sDebugSummary = BuildDebugSummary(request, result);
		return result;
	}

	HST_ForceRequest BuildSupportForceRequest(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState supportRequest)
	{
		HST_ForceRequest request = new HST_ForceRequest();
		if (!supportRequest)
			return request;

		request.m_sRequestId = supportRequest.m_sRequestId;
		request.m_sFactionKey = supportRequest.m_sFactionKey;
		request.m_sIntentId = IntentForSupport(supportRequest.m_eType, supportRequest.m_sAssetProfileId);
		request.m_sSourceZoneId = supportRequest.m_sSourceZoneId;
		request.m_sTargetZoneId = supportRequest.m_sTargetZoneId;
		request.m_vSourcePosition = supportRequest.m_vSourcePosition;
		request.m_vTargetPosition = supportRequest.m_vTargetPosition;
		request.m_iWarLevel = ResolveWarLevel(state, null);
		request.m_iBudget = Math.Max(10 + request.m_iWarLevel * 4, supportRequest.m_iAttackCost + supportRequest.m_iSupportCost);
		request.m_iMinManpower = 3;
		request.m_iMaxManpower = 12 + request.m_iWarLevel;
		request.m_bAllowInfantry = true;
		request.m_bAllowVehicles = request.m_iWarLevel >= 4 || supportRequest.m_iAttackCost >= 18 || supportRequest.m_iSupportCost >= 18;
		if (request.m_bAllowVehicles)
			request.m_iBudget = Math.Max(request.m_iBudget, 28 + request.m_iWarLevel * 5);
		request.m_bAllowArmedVehicles = request.m_iWarLevel >= 5;
		request.m_bAllowLightArmor = request.m_iWarLevel >= 6;
		request.m_bAllowHeavyArmor = false;
		request.m_fMechanizedShare = 0.0;
		if (request.m_bAllowVehicles)
			request.m_fMechanizedShare = 0.25;
		request.m_sReason = "support request";
		request.m_bExplain = true;
		return request;
	}

	static string IntentForSupport(HST_ESupportRequestType supportType, string assetProfileId = "")
	{
		if (assetProfileId.Contains("_petros_attack"))
			return INTENT_PETROS_ATTACK;
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return INTENT_QRF_REGULAR;
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return INTENT_SEARCH_PATROL;
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE)
			return INTENT_CHECKPOINT;

		return INTENT_PATROL;
	}

	void ApplyCompositionToSupportRequest(HST_SupportRequestState request, HST_ForceCompositionResult result)
	{
		if (!request || !result)
			return;

		request.m_sCompositionRequestId = result.m_sRequestId;
		request.m_sCompositionIntentId = result.m_sIntentId;
		request.m_sCompositionTier = result.m_sSelectedTier;
		request.m_sCompositionSummary = result.m_sDebugSummary;
		request.m_sCompositionFailureReason = result.m_sFailureReason;
		request.m_iCompositionCost = result.m_iTotalCost;
		request.m_iCompositionManpower = result.m_iManpower;
		request.m_iCompositionVehicleCount = result.m_iVehicleCount;
		request.m_iCompositionArmedVehicleCount = result.m_iArmedVehicleCount;
	}

	void ApplyCompositionToActiveGroup(HST_ActiveGroupState group, HST_ForceCompositionResult result)
	{
		if (!group || !result)
			return;

		group.m_sCompositionRequestId = result.m_sRequestId;
		group.m_sCompositionIntentId = result.m_sIntentId;
		group.m_sCompositionTier = result.m_sSelectedTier;
		group.m_sCompositionSummary = result.m_sDebugSummary;
		HST_VehicleSpawnPlan vehicle = result.GetPrimaryVehicle();
		if (vehicle)
			group.m_sVehiclePrefab = vehicle.m_sPrefab;
		group.m_iCompositionCost = result.m_iTotalCost;
		group.m_iCompositionManpower = result.m_iManpower;
		group.m_iCompositionVehicleCount = result.m_iVehicleCount;
		group.m_iCompositionArmedVehicleCount = result.m_iArmedVehicleCount;
	}

	void ApplyCompositionToEnemyOrder(HST_EnemyOrderState order, HST_ForceCompositionResult result)
	{
		if (!order || !result)
			return;

		order.m_sCompositionRequestId = result.m_sRequestId;
		order.m_sCompositionIntentId = result.m_sIntentId;
		order.m_sCompositionTier = result.m_sSelectedTier;
		order.m_sCompositionSummary = result.m_sDebugSummary;
		order.m_sCompositionFailureReason = result.m_sFailureReason;
		order.m_iCompositionCost = result.m_iTotalCost;
		order.m_iCompositionManpower = result.m_iManpower;
		order.m_iCompositionVehicleCount = result.m_iVehicleCount;
		order.m_iCompositionArmedVehicleCount = result.m_iArmedVehicleCount;
	}

	string BuildCompositionReport(HST_CampaignState state, HST_CampaignPreset preset)
	{
		string report = "h-istasi force composition";
		report = report + "\n" + Compose(state, preset, BuildDebugRequest(state, preset, "US", INTENT_QRF_REGULAR, 1, true, false)).m_sDebugSummary;
		report = report + "\n" + Compose(state, preset, BuildDebugRequest(state, preset, "US", INTENT_QRF_REGULAR, 5, true, true)).m_sDebugSummary;
		report = report + "\n" + Compose(state, preset, BuildDebugRequest(state, preset, "USSR", INTENT_COUNTERATTACK, 5, true, true)).m_sDebugSummary;
		report = report + "\n" + Compose(state, preset, BuildDebugRequest(state, preset, "FIA", INTENT_GARRISON, 2, true, false)).m_sDebugSummary;
		return report;
	}

	HST_ForceRequest BuildDebugRequest(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, string intentId, int warLevel, bool allowInfantry, bool allowVehicles)
	{
		HST_ForceRequest request = new HST_ForceRequest();
		request.m_sRequestId = string.Format("debug_force_%1_%2_wl%3", factionKey, intentId, warLevel);
		request.m_sFactionKey = factionKey;
		request.m_sIntentId = intentId;
		request.m_iWarLevel = warLevel;
		request.m_iBudget = 60 + warLevel * 8;
		request.m_iMinManpower = 2;
		request.m_iMaxManpower = 16;
		request.m_bAllowInfantry = allowInfantry;
		request.m_bAllowVehicles = allowVehicles;
		request.m_bAllowArmedVehicles = allowVehicles;
		request.m_bAllowLightArmor = allowVehicles && warLevel >= 5;
		request.m_bAllowHeavyArmor = false;
		request.m_bExplain = true;
		request.m_sReason = "debug";
		return request;
	}

	protected HST_ForceCompositionResult Fail(HST_ForceCompositionResult result, string reason)
	{
		if (!result)
		{
			HST_ForceCompositionResult failedResult = new HST_ForceCompositionResult();
			failedResult.m_bSuccess = false;
			failedResult.m_sFailureReason = reason;
			failedResult.m_sDebugSummary = BuildDebugSummary(null, failedResult);
			return failedResult;
		}

		result.m_bSuccess = false;
		result.m_sFailureReason = reason;
		result.m_sDebugSummary = BuildDebugSummary(null, result);
		return result;
	}

	protected string NormalizeIntent(string intentId)
	{
		if (intentId.IsEmpty())
			return INTENT_PATROL;

		return intentId;
	}

	protected string BuildRequestId(HST_CampaignState state, string factionKey, string intentId)
	{
		int elapsed;
		int seed;
		if (state)
		{
			elapsed = state.m_iElapsedSeconds;
			seed = state.m_iCampaignSeed;
		}

		return string.Format("force_%1_%2_%3_%4", factionKey, intentId, elapsed, seed);
	}

	protected int ResolveWarLevel(HST_CampaignState state, HST_ForceRequest request)
	{
		if (request && request.m_iWarLevel > 0)
			return Math.Max(1, request.m_iWarLevel);
		if (state)
			return Math.Max(1, state.m_iWarLevel);

		return 1;
	}

	protected int ResolveBudget(HST_ForceRequest request, int warLevel)
	{
		if (request && request.m_iBudget > 0)
			return request.m_iBudget;

		return 30 + Math.Max(1, warLevel) * 8;
	}

	protected string ResolveTier(int warLevel)
	{
		if (warLevel >= 8)
			return "heavy";
		if (warLevel >= 5)
			return "medium";
		if (warLevel >= 3)
			return "light";

		return "early";
	}

	protected string ResolveCapabilityFailure(HST_CampaignPreset preset, HST_FactionTemplate faction, HST_ForceRequest request, string intentId)
	{
		if (!request || !faction)
			return "";

		if (request.m_bAllowHelicopter && !faction.HasCapability("helicopter_transport") && !faction.HasCapability("helicopter_qrf"))
			return "helicopter composition disabled: faction has no helicopter transport capability";

		if ((intentId.Contains("artillery") || intentId.Contains("mortar")) && IsPresetCapabilityUnavailable(preset, "artillery"))
			return "artillery composition disabled by campaign preset";

		if (request.m_bAllowHeavyArmor && !faction.HasCapability("heavy_armor") && request.m_iWarLevel < 8)
			return "heavy armor composition disabled: faction template has no heavy armor capability at this war level";

		return "";
	}

	protected bool IsPresetCapabilityUnavailable(HST_CampaignPreset preset, string capabilityId)
	{
		if (!preset || capabilityId.IsEmpty())
			return false;

		return preset.m_aUnavailableCapabilities.Contains(capabilityId);
	}

	protected int ResolveMinManpower(HST_ForceRequest request, string intentId, int warLevel)
	{
		if (request && request.m_iMinManpower > 0)
			return request.m_iMinManpower;
		if (intentId == INTENT_QRF_REGULAR || intentId == INTENT_COUNTERATTACK || intentId == INTENT_PETROS_ATTACK)
			return 4;
		if (intentId == INTENT_GARRISON || intentId == INTENT_REBUILD_GARRISON)
			return 3;
		if (intentId == INTENT_TOWN_POLICE || intentId == INTENT_PATROL || intentId == INTENT_ROADBLOCK)
			return 2;

		return Math.Max(2, warLevel);
	}

	protected int ResolveMaxManpower(HST_ForceRequest request, string intentId, int warLevel, int minManpower)
	{
		if (request && request.m_iMaxManpower > 0)
			return Math.Max(minManpower, request.m_iMaxManpower);
		if (intentId == INTENT_COUNTERATTACK || intentId == INTENT_PETROS_ATTACK)
			return 18 + warLevel;
		if (intentId == INTENT_QRF_ELITE)
			return 14 + warLevel;
		if (intentId == INTENT_QRF_REGULAR)
			return 12 + warLevel;

		return Math.Max(minManpower, 8 + warLevel);
	}

	protected int ResolveDesiredManpower(HST_ForceRequest request, string intentId, int warLevel, int minManpower, int maxManpower)
	{
		int desired = minManpower + warLevel;
		if (intentId == INTENT_QRF_REGULAR)
			desired = 4 + warLevel;
		else if (intentId == INTENT_QRF_ELITE || intentId == INTENT_COUNTERATTACK || intentId == INTENT_PETROS_ATTACK)
			desired = 6 + warLevel * 2;
		else if (intentId == INTENT_GARRISON || intentId == INTENT_REBUILD_GARRISON)
			desired = 4 + warLevel;
		else if (intentId == INTENT_TOWN_POLICE || intentId == INTENT_PATROL)
			desired = 2 + Math.Max(0, warLevel / 2);

		return Math.Max(minManpower, Math.Min(maxManpower, desired));
	}

	protected void BuildGroupPlans(HST_FactionTemplate faction, HST_ForceRequest request, HST_ForceCompositionResult result, int minManpower, int maxManpower, int desiredManpower)
	{
		array<ref HST_PrefabPoolEntry> candidates = {};
		AppendForcedGroupCandidates(candidates, request);
		if (!request.m_bRestrictToForcedPrefabs)
			AppendFactionGroupCandidates(candidates, faction, result.m_sIntentId);

		int seed = BuildSelectionSeed(request, result, 101);
		int guard;
		while (result.m_iManpower < desiredManpower && guard < 6)
		{
			HST_PrefabPoolEntry entry = SelectWeightedEntry(candidates, seed + guard * 173);
			if (!entry)
				break;

			HST_GroupSpawnPlan plan = BuildGroupPlan(entry.m_sPrefab, entry.m_iWeight, result.m_sIntentId, result.m_sSelectedTier);
			if (!IsPrefabResourceValid(plan.m_sPrefab))
			{
				plan.m_bSkipped = true;
				plan.m_sFailureReason = "invalid group prefab resource";
				result.m_iSkippedPrefabCount++;
				result.m_aGroups.Insert(plan);
				RemoveCandidate(candidates, entry);
				guard++;
				continue;
			}

			if (result.m_iManpower + plan.m_iManpower > maxManpower && result.m_iManpower >= minManpower)
				break;

			if (result.m_iTotalCost + plan.m_iCost > result.m_iBudget && result.m_iManpower >= minManpower)
				break;

			result.m_aGroups.Insert(plan);
			result.m_iManpower += plan.m_iManpower;
			result.m_iTotalCost += plan.m_iCost;
			guard++;
		}
	}

	protected void BuildVehiclePlans(HST_FactionTemplate faction, HST_ForceRequest request, HST_ForceCompositionResult result)
	{
		if (result.m_iWarLevel < 4 && request.m_fMechanizedShare <= 0.0 && result.m_sIntentId != INTENT_CONVOY_ARMORED)
			return;

		array<ref HST_PrefabPoolEntry> candidates = {};
		AppendForcedVehicleCandidates(candidates, request);
		if (!request.m_bRestrictToForcedPrefabs)
			AppendVehicleCandidates(candidates, faction, request, result);

		HST_PrefabPoolEntry entry = SelectWeightedEntry(candidates, BuildSelectionSeed(request, result, 907));
		if (!entry)
			return;

		HST_VehicleSpawnPlan plan = BuildVehiclePlan(entry.m_sPrefab, result.m_sIntentId, result.m_sSelectedTier, request);
		if (!IsPrefabResourceValid(plan.m_sPrefab))
		{
			plan.m_bSkipped = true;
			plan.m_sFailureReason = "invalid vehicle prefab resource";
			result.m_iSkippedPrefabCount++;
			result.m_aVehicles.Insert(plan);
			return;
		}

		if (result.m_iTotalCost + plan.m_iCost > result.m_iBudget && result.m_iManpower > 0)
			return;

		result.m_aVehicles.Insert(plan);
		result.m_iVehicleCount++;
		if (plan.m_bArmed)
			result.m_iArmedVehicleCount++;
		result.m_iTotalCost += plan.m_iCost;
	}

	protected void AppendForcedGroupCandidates(array<ref HST_PrefabPoolEntry> candidates, HST_ForceRequest request)
	{
		if (!candidates || !request)
			return;

		foreach (string prefab : request.m_aForcedGroupPrefabs)
			AppendCandidate(candidates, prefab, 1);
	}

	protected void AppendFactionGroupCandidates(array<ref HST_PrefabPoolEntry> candidates, HST_FactionTemplate faction, string intentId)
	{
		if (!candidates || !faction)
			return;

		if (intentId == INTENT_QRF_REGULAR || intentId == INTENT_QRF_ELITE || intentId == INTENT_COUNTERATTACK || intentId == INTENT_PETROS_ATTACK)
		{
			AppendPool(candidates, faction.m_aQRFGroupPool);
			AppendPool(candidates, faction.m_aGroupPool);
			AppendArray(candidates, faction.m_aQRFGroupPrefabs, 1);
			AppendArray(candidates, faction.m_aGroupPrefabs, 1);
			return;
		}

		if (intentId == INTENT_PATROL || intentId == INTENT_SEARCH_PATROL || intentId == INTENT_TOWN_POLICE || intentId == INTENT_ROADBLOCK)
		{
			AppendPool(candidates, faction.m_aPatrolGroupPool);
			AppendPool(candidates, faction.m_aGroupPool);
			AppendArray(candidates, faction.m_aPatrolGroupPrefabs, 1);
			AppendArray(candidates, faction.m_aGroupPrefabs, 1);
			return;
		}

		AppendPool(candidates, faction.m_aGroupPool);
		AppendArray(candidates, faction.m_aGroupPrefabs, 1);
		AppendPool(candidates, faction.m_aPatrolGroupPool);
		AppendArray(candidates, faction.m_aPatrolGroupPrefabs, 1);
	}

	protected void AppendForcedVehicleCandidates(array<ref HST_PrefabPoolEntry> candidates, HST_ForceRequest request)
	{
		if (!candidates || !request)
			return;

		foreach (string prefab : request.m_aForcedVehiclePrefabs)
			AppendCandidate(candidates, prefab, 1);
	}

	protected void AppendVehicleCandidates(array<ref HST_PrefabPoolEntry> candidates, HST_FactionTemplate faction, HST_ForceRequest request, HST_ForceCompositionResult result)
	{
		if (!candidates || !faction)
			return;

		foreach (string prefab : faction.m_aVehiclePrefabs)
		{
			if (prefab.IsEmpty())
				continue;

			bool armed = IsArmedVehiclePrefab(prefab);
			bool armor = IsLightArmorPrefab(prefab) || IsHeavyArmorPrefab(prefab);
			if (armed && !request.m_bAllowArmedVehicles)
				continue;
			if (armor && !request.m_bAllowLightArmor && !request.m_bAllowHeavyArmor)
				continue;

			int weight = 1;
			if (armed && result.m_iWarLevel >= 5)
				weight += 2;
			if (!armed && result.m_iWarLevel <= 4)
				weight += 1;
			AppendCandidate(candidates, prefab, weight);
		}
	}

	protected void AppendPool(array<ref HST_PrefabPoolEntry> candidates, array<ref HST_PrefabPoolEntry> pool)
	{
		if (!candidates || !pool)
			return;

		foreach (HST_PrefabPoolEntry entry : pool)
		{
			if (entry)
				AppendCandidate(candidates, entry.m_sPrefab, entry.m_iWeight);
		}
	}

	protected void AppendArray(array<ref HST_PrefabPoolEntry> candidates, array<string> prefabs, int weight)
	{
		if (!candidates || !prefabs)
			return;

		foreach (string prefab : prefabs)
			AppendCandidate(candidates, prefab, weight);
	}

	protected void AppendCandidate(array<ref HST_PrefabPoolEntry> candidates, string prefab, int weight)
	{
		if (!candidates || prefab.IsEmpty())
			return;

		foreach (HST_PrefabPoolEntry existing : candidates)
		{
			if (existing && existing.m_sPrefab == prefab)
			{
				existing.m_iWeight = Math.Max(1, existing.m_iWeight + Math.Max(1, weight));
				return;
			}
		}

		HST_PrefabPoolEntry entry = new HST_PrefabPoolEntry();
		entry.m_sPrefab = prefab;
		entry.m_iWeight = Math.Max(1, weight);
		candidates.Insert(entry);
	}

	protected void RemoveCandidate(array<ref HST_PrefabPoolEntry> candidates, HST_PrefabPoolEntry entry)
	{
		if (!candidates || !entry)
			return;

		int index = candidates.Find(entry);
		if (index >= 0)
			candidates.Remove(index);
	}

	protected HST_PrefabPoolEntry SelectWeightedEntry(array<ref HST_PrefabPoolEntry> candidates, int seed)
	{
		if (!candidates || candidates.Count() == 0)
			return null;

		int totalWeight;
		foreach (HST_PrefabPoolEntry entry : candidates)
		{
			if (entry && !entry.m_sPrefab.IsEmpty())
				totalWeight += Math.Max(1, entry.m_iWeight);
		}

		if (totalWeight <= 0)
			return null;

		int roll = HST_DefaultCatalog.PositiveMod(seed, totalWeight);
		foreach (HST_PrefabPoolEntry weightedEntry : candidates)
		{
			if (!weightedEntry || weightedEntry.m_sPrefab.IsEmpty())
				continue;

			roll -= Math.Max(1, weightedEntry.m_iWeight);
			if (roll < 0)
				return weightedEntry;
		}

		return candidates[0];
	}

	protected HST_GroupSpawnPlan BuildGroupPlan(string prefab, int weight, string intentId, string tier)
	{
		HST_GroupSpawnPlan plan = new HST_GroupSpawnPlan();
		plan.m_sPrefab = prefab;
		plan.m_sRole = intentId;
		plan.m_sTier = tier;
		plan.m_iWeight = Math.Max(1, weight);
		plan.m_iManpower = EstimateGroupManpower(prefab);
		plan.m_iCost = Math.Max(2, plan.m_iManpower * 2);
		plan.m_iCost += ResolveTierCostBonus(tier, plan.m_iCost);
		return plan;
	}

	protected HST_VehicleSpawnPlan BuildVehiclePlan(string prefab, string intentId, string tier, HST_ForceRequest request)
	{
		HST_VehicleSpawnPlan plan = new HST_VehicleSpawnPlan();
		plan.m_sPrefab = prefab;
		plan.m_sRole = intentId;
		plan.m_sTier = tier;
		plan.m_iCrew = 1;
		plan.m_bArmed = IsArmedVehiclePrefab(prefab);
		plan.m_bLightArmor = IsLightArmorPrefab(prefab);
		plan.m_bHeavyArmor = IsHeavyArmorPrefab(prefab);
		plan.m_iCost = 10;
		if (plan.m_bArmed)
			plan.m_iCost += 8;
		if (plan.m_bLightArmor)
			plan.m_iCost += 12;
		if (plan.m_bHeavyArmor)
			plan.m_iCost += 24;
		plan.m_iCost += ResolveTierCostBonus(tier, plan.m_iCost);
		return plan;
	}

	protected int ResolveTierCostBonus(string tier, int baseCost)
	{
		if (tier == "heavy")
			return Math.Max(8, baseCost / 2);
		if (tier == "medium")
			return Math.Max(4, baseCost / 3);
		if (tier == "light")
			return Math.Max(2, baseCost / 4);

		return 0;
	}

	protected int EstimateGroupManpower(string prefab)
	{
		string value = prefab;
		value.ToLower();
		if (value.Contains("riflesquad") || value.Contains("rifle_squad"))
			return 8;
		if (value.Contains("squad"))
			return 8;
		if (value.Contains("fireteam") || value.Contains("fire_group") || value.Contains("firegroup"))
			return 4;
		if (value.Contains("machinegunteam") || value.Contains("machinegun_team"))
			return 3;
		if (value.Contains("sentry"))
			return 2;

		return 4;
	}

	protected bool IsPrefabResourceValid(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		ResourceName resourceName = prefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded || !loaded.IsValid())
			return false;

		return true;
	}

	protected bool IsArmedVehiclePrefab(string prefab)
	{
		string value = prefab;
		value.ToLower();
		return value.Contains("_pkm") || value.Contains("m1025") || value.Contains("btr") || value.Contains("armed");
	}

	protected bool IsLightArmorPrefab(string prefab)
	{
		string value = prefab;
		value.ToLower();
		return value.Contains("btr") || value.Contains("lav") || value.Contains("brdm");
	}

	protected bool IsHeavyArmorPrefab(string prefab)
	{
		string value = prefab;
		value.ToLower();
		return value.Contains("tank") || value.Contains("m1a1") || value.Contains("t72");
	}

	protected int BuildSelectionSeed(HST_ForceRequest request, HST_ForceCompositionResult result, int salt)
	{
		int seed = salt;
		if (result)
		{
			seed += result.m_iWarLevel * 97 + result.m_iBudget * 13;
			seed += result.m_sFactionKey.Length() * 41 + result.m_sIntentId.Length() * 53;
			seed += result.m_sSourceZoneId.Length() * 17 + result.m_sTargetZoneId.Length() * 19;
		}
		if (request)
			seed += Math.Round(request.m_vTargetPosition[0]) * 3 + Math.Round(request.m_vTargetPosition[2]) * 5 + request.m_sReason.Length() * 23;
		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected string BuildNoGroupFailure(HST_ForceCompositionResult result)
	{
		string failure = "no valid group prefab for " + result.m_sFactionKey + " intent " + result.m_sIntentId;
		if (result.m_iSkippedPrefabCount > 0)
			failure = failure + string.Format("; skipped invalid prefabs %1", result.m_iSkippedPrefabCount);
		return failure;
	}

	protected string BuildNoVehicleFailure(HST_ForceCompositionResult result)
	{
		string failure = "no valid vehicle prefab for " + result.m_sFactionKey + " intent " + result.m_sIntentId;
		if (result.m_iSkippedPrefabCount > 0)
			failure = failure + string.Format("; skipped invalid prefabs %1", result.m_iSkippedPrefabCount);
		return failure;
	}

	protected string BuildDebugSummary(HST_ForceRequest request, HST_ForceCompositionResult result)
	{
		if (!result)
			return "force composition | missing result";

		string summary = string.Format(
			"force composition | request %1 | faction %2 | intent %3 | war %4 | tier %5 | success %6 | cost %7/%8 | manpower %9",
			EmptyField(result.m_sRequestId),
			EmptyField(result.m_sFactionKey),
			EmptyField(result.m_sIntentId),
			result.m_iWarLevel,
			EmptyField(result.m_sSelectedTier),
			result.m_bSuccess,
			result.m_iTotalCost,
			result.m_iBudget,
			result.m_iManpower
		);
		summary = summary + string.Format(
			" | vehicles %1 armed %2 | skipped %3",
			result.m_iVehicleCount,
			result.m_iArmedVehicleCount,
			result.m_iSkippedPrefabCount
		);

		if (!result.m_sFailureReason.IsEmpty())
			summary = summary + " | failure " + result.m_sFailureReason;

		HST_GroupSpawnPlan group = result.GetPrimaryGroup();
		if (group)
			summary = summary + string.Format(" | group %1 manpower %2", group.m_sPrefab, group.m_iManpower);
		HST_VehicleSpawnPlan vehicle = result.GetPrimaryVehicle();
		if (vehicle)
			summary = summary + string.Format(" | vehicle %1 armed %2", vehicle.m_sPrefab, vehicle.m_bArmed);

		if (request && request.m_bExplain && !request.m_sReason.IsEmpty())
			summary = summary + " | reason " + request.m_sReason;

		return summary;
	}

	protected string EmptyField(string value)
	{
		if (value.IsEmpty())
			return "none";

		return value;
	}
}
