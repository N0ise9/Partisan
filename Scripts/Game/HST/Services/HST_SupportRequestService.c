class HST_SupportRequestService
{
	static const int PLAYER_SUPPLY_COST = 150;
	static const int PLAYER_QRF_COST = 250;
	static const int PLAYER_FIRE_COST = 350;
	static const int PLAYER_AIRSTRIKE_COST = 750;
	static const int PLAYER_CRUISE_MISSILE_COST = 1200;
	static const int DEFAULT_ETA_SECONDS = 120;
	static const int HELICOPTER_STYLE_ETA_SECONDS = 180;
	static const int PLAYER_SUPPORT_COOLDOWN_SECONDS = 600;

	HST_SupportRequestState RequestSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ESupportRequestType supportType, string targetZoneId, bool playerRequested = false, int playerCooldownSeconds = PLAYER_SUPPORT_COOLDOWN_SECONDS)
	{
		if (!state || !preset || factionKey.IsEmpty())
			return null;

		HST_ZoneState targetZone = state.FindZone(targetZoneId);
		if (!targetZone && targetZoneId.IsEmpty())
			targetZone = FindFallbackTargetZone(state, preset);

		if (!targetZone)
			return null;

		int attackCost;
		int supportCost;
		ResolveCosts(supportType, attackCost, supportCost);
		if (factionKey == preset.m_sResistanceFactionKey)
		{
			int moneyCost = ResolvePlayerMoneyCost(supportType);
			if (HasActivePlayerRequest(state, supportType))
				return null;

			if (!economy || !economy.SpendFactionMoney(state, moneyCost))
				return null;
		}
		else if (!enemyDirector || !enemyDirector.TrySpend(state, factionKey, attackCost, supportCost))
		{
			return null;
		}

		HST_SupportRequestState request = new HST_SupportRequestState();
		request.m_sRequestId = BuildRequestId(state, factionKey, supportType);
		request.m_sFactionKey = factionKey;
		request.m_sCapabilityId = CapabilityForSupport(supportType);
		request.m_sAssetProfileId = AssetProfileForSupport(factionKey, supportType, factionKey == preset.m_sResistanceFactionKey);
		request.m_sStrikeKind = StrikeKindForSupport(supportType);
		request.m_sStrikeConfigResource = StrikeConfigForSupport(supportType);
		request.m_eType = supportType;
		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
		request.m_sTargetZoneId = targetZone.m_sZoneId;
		request.m_sSourceZoneId = FindSourceZoneId(state, preset, factionKey, targetZone.m_sZoneId);
		request.m_vTargetPosition = targetZone.m_vPosition;
		request.m_vSourcePosition = ResolveSourcePosition(state, request.m_sSourceZoneId, targetZone.m_vPosition);
		request.m_iRequestedAtSecond = state.m_iElapsedSeconds;
		request.m_iETASeconds = ResolveETA(supportType);
		request.m_iAttackCost = attackCost;
		request.m_iSupportCost = supportCost;
		request.m_iMoneyCost = ResolvePlayerMoneyCost(supportType);
		if (factionKey == preset.m_sResistanceFactionKey)
			request.m_iCooldownUntilSecond = state.m_iElapsedSeconds + Math.Max(60, playerCooldownSeconds);
		request.m_bHelicopterStyle = IsHelicopterStyle(supportType);
		request.m_bPlayerRequested = playerRequested;
		if (IsStrikeSupport(supportType))
			request.m_sRuntimeEntityId = "abstract_strike";
		state.m_aSupportRequests.Insert(request);
		Print(string.Format("h-istasi | support request %1 queued for %2 at %3", request.m_sRequestId, factionKey, targetZone.m_sZoneId));
		return request;
	}

	bool Tick(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons)
	{
		if (!state || !preset)
			return false;

		bool changed;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
				continue;

			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED)
			{
				request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE;
				ApplyActiveSupport(state, preset, request);
				changed = true;
			}

			if (state.m_iElapsedSeconds < request.m_iRequestedAtSecond + request.m_iETASeconds)
				continue;

			ResolveSupport(state, preset, garrisons, request);
			changed = true;
		}

		return changed;
	}

	string BuildSupportReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi support | state not ready";

		int queued;
		int active;
		int resolved;
		int helicopterStyle;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request)
				continue;

			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED)
				queued++;
			else if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				active++;
			else if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED)
				resolved++;

			if (request.m_bHelicopterStyle)
				helicopterStyle++;
		}

		string report = string.Format("h-istasi support | queued %1 | active %2 | resolved %3 | helo-style abstract %4", queued, active, resolved, helicopterStyle);
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request)
				continue;

			string strike = "";
			if (!request.m_sStrikeConfigResource.IsEmpty())
				strike = " | strike " + request.m_sStrikeKind + " | " + request.m_sStrikeConfigResource;

			report = report + string.Format("\n%1 | %2 | %3 | target %4 | eta %5 | asset %6%7", request.m_sRequestId, request.m_eType, request.m_eStatus, request.m_sTargetZoneId, request.m_iETASeconds, request.m_sAssetProfileId, strike);
		}

		return report;
	}

	bool CancelSupportRequest(HST_CampaignState state, string requestId = "", bool playerRequestedOnly = true)
	{
		if (!state)
			return false;

		HST_SupportRequestState request = ResolveCancelableRequest(state, requestId, playerRequestedOnly);
		if (!request)
			return false;

		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED;
		request.m_sFailureReason = "cancelled by commander";
		return true;
	}

	protected void ApplyActiveSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request)
	{
		if (!request || request.m_bHelicopterStyle)
			return;

		if (IsStrikeSupport(request.m_eType))
		{
			ActivateStrikeSupport(request);
			return;
		}

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_EVACUATION)
			return;

		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = string.Format("support_%1", request.m_sRequestId);
		group.m_sZoneId = request.m_sTargetZoneId;
		group.m_sFactionKey = request.m_sFactionKey;
		group.m_sPrefab = SelectGroupPrefab(state, request);
		group.m_sRouteId = request.m_sSourceZoneId + "_to_" + request.m_sTargetZoneId;
		group.m_vSourcePosition = request.m_vSourcePosition;
		group.m_vTargetPosition = request.m_vTargetPosition;
		group.m_vPosition = HST_WorldPositionService.ResolveGroundPosition(group.m_vSourcePosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
		group.m_sRuntimeStatus = "support_active";
		group.m_iInfantryCount = 4 + state.m_iWarLevel;
		group.m_iVehicleCount = 0;
		group.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		group.m_iLastSeenAliveCount = group.m_iInfantryCount;
		group.m_iSurvivorInfantryCount = group.m_iInfantryCount;
		group.m_iSurvivorVehicleCount = group.m_iVehicleCount;
		group.m_bQRF = request.m_eType == HST_ESupportRequestType.HST_SUPPORT_QRF;
		state.m_aActiveGroups.Insert(group);
		request.m_sGroupId = group.m_sGroupId;
	}

	protected void ResolveSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_SupportRequestState request)
	{
		if (!request || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED)
			return;

		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP && request.m_sFactionKey == preset.m_sResistanceFactionKey)
		{
			state.m_iFactionMoney += 75;
			state.m_iHR += 1;
		}

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_QRF && request.m_sFactionKey == preset.m_sResistanceFactionKey && garrisons)
			garrisons.AddAbstractForces(state, request.m_sTargetZoneId, preset.m_sResistanceFactionKey, 3 + state.m_iTrainingLevel, 0);

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE && request.m_sFactionKey == preset.m_sResistanceFactionKey)
		{
			HST_ZoneState targetZone = state.FindZone(request.m_sTargetZoneId);
			if (targetZone && targetZone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				targetZone.m_iResistanceCaptureProgress = Math.Min(HST_ZoneCaptureService.CAPTURE_PROGRESS_REQUIRED - 1, targetZone.m_iResistanceCaptureProgress + 15);
		}

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING && garrisons)
			garrisons.AddAbstractForces(state, request.m_sTargetZoneId, request.m_sFactionKey, 3 + state.m_iWarLevel, 0);

		if (IsStrikeSupport(request.m_eType))
			ResolveStrikeSupport(state, preset, request);
	}

	protected void ResolveCosts(HST_ESupportRequestType supportType, out int attackCost, out int supportCost)
	{
		attackCost = 8;
		supportCost = 4;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF || supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
		{
			attackCost = 15;
			supportCost = 5;
			return;
		}

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING || supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE)
		{
			attackCost = 20;
			supportCost = 8;
			return;
		}

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
		{
			attackCost = 0;
			supportCost = 10;
			return;
		}

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
		{
			attackCost = 24;
			supportCost = 16;
			return;
		}

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
		{
			attackCost = 36;
			supportCost = 28;
		}
	}

	protected int ResolveETA(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return 90;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return 150;

		if (IsHelicopterStyle(supportType))
			return HELICOPTER_STYLE_ETA_SECONDS;

		return DEFAULT_ETA_SECONDS;
	}

	protected int ResolvePlayerMoneyCost(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return PLAYER_QRF_COST;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE || supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return PLAYER_FIRE_COST;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return PLAYER_AIRSTRIKE_COST;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return PLAYER_CRUISE_MISSILE_COST;

		return PLAYER_SUPPLY_COST;
	}

	protected bool HasActivePlayerRequest(HST_CampaignState state, HST_ESupportRequestType supportType)
	{
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || !request.m_bPlayerRequested || request.m_eType != supportType)
				continue;

			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				return true;

			if (state.m_iElapsedSeconds < request.m_iCooldownUntilSecond)
				return true;
		}

		return false;
	}

	protected HST_SupportRequestState ResolveCancelableRequest(HST_CampaignState state, string requestId, bool playerRequestedOnly)
	{
		HST_SupportRequestState fallback;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request)
				continue;

			if (playerRequestedOnly && !request.m_bPlayerRequested)
				continue;

			if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_QUEUED && request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				continue;

			if (!requestId.IsEmpty() && request.m_sRequestId == requestId)
				return request;

			if (requestId.IsEmpty() && !fallback)
				fallback = request;
		}

		return fallback;
	}

	protected string CapabilityForSupport(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
			return "supply_drop";

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return "qrf";

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_TRANSPORT || supportType == HST_ESupportRequestType.HST_SUPPORT_EVACUATION || supportType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING)
			return "rotary_wing";

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE)
			return "suppressive_fire";

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return "airstrike";

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return "cruise_missile";

		return "ground_support";
	}

	protected string AssetProfileForSupport(string factionKey, HST_ESupportRequestType supportType, bool playerRequested)
	{
		if (playerRequested)
		{
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
				return "fia_supply_crate_alpha";
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
				return "fia_qrf_reserve_alpha";
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE)
				return "fia_capability_suppressive_fire_alpha";
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU)
				return "fia_abstract_airstrike_alpha";
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
				return "fia_abstract_airstrike_bravo";
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
				return "fia_abstract_long_range_strike";
		}

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU)
			return factionKey + "_abstract_airstrike_alpha";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return factionKey + "_abstract_airstrike_bravo";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return factionKey + "_abstract_long_range_strike";

		return factionKey + "_" + CapabilityForSupport(supportType);
	}

	protected bool IsHelicopterStyle(HST_ESupportRequestType supportType)
	{
		return supportType == HST_ESupportRequestType.HST_SUPPORT_TRANSPORT || supportType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING || supportType == HST_ESupportRequestType.HST_SUPPORT_EVACUATION;
	}

	protected bool IsStrikeSupport(HST_ESupportRequestType supportType)
	{
		return supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK || supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55;
	}

	protected void ActivateStrikeSupport(HST_SupportRequestState request)
	{
		if (!request)
			return;

		request.m_sRuntimeEntityId = "abstract_strike";
		request.m_bPhysicalStrikeSpawned = false;
		Print(string.Format("h-istasi | abstract strike support %1 active", request.m_sRequestId));
	}

	protected void ResolveStrikeSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request)
	{
		if (!state || !preset || !request)
			return;

		HST_ZoneState targetZone = state.FindZone(request.m_sTargetZoneId);
		if (!targetZone)
			return;

		request.m_bAbstractResolved = true;
		if (request.m_sFactionKey == preset.m_sResistanceFactionKey)
		{
			if (targetZone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				targetZone.m_iResistanceCaptureProgress = Math.Min(HST_ZoneCaptureService.CAPTURE_PROGRESS_REQUIRED - 1, targetZone.m_iResistanceCaptureProgress + StrikeCaptureProgress(request.m_eType));

			return;
		}

		if (targetZone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
		{
			targetZone.m_iSupport = Math.Max(0, targetZone.m_iSupport - StrikeSupportDamage(request.m_eType));
			targetZone.m_iResistanceCaptureProgress = Math.Max(0, targetZone.m_iResistanceCaptureProgress - StrikeCaptureProgress(request.m_eType));
		}
		else
		{
			targetZone.m_iResistanceCaptureProgress = Math.Max(0, targetZone.m_iResistanceCaptureProgress - StrikeSupportDamage(request.m_eType));
		}
	}

	protected int StrikeCaptureProgress(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return 35;

		return 20;
	}

	protected int StrikeSupportDamage(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return 25;

		return 12;
	}

	protected string StrikeKindForSupport(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU)
			return "airstrike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return "heavy_airstrike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return "long_range_strike";

		return "";
	}

	protected string StrikeConfigForSupport(HST_ESupportRequestType supportType)
	{
		return "";
	}

	protected string BuildRequestId(HST_CampaignState state, string factionKey, HST_ESupportRequestType supportType)
	{
		return string.Format("support_%1_%2_%3", factionKey, state.m_iElapsedSeconds, state.m_aSupportRequests.Count());
	}

	protected string FindSourceZoneId(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, string targetZoneId)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sZoneId == targetZoneId)
				continue;

			if (zone.m_sOwnerFactionKey == factionKey)
				return zone.m_sZoneId;
		}

		return targetZoneId;
	}

	protected vector ResolveSourcePosition(HST_CampaignState state, string sourceZoneId, vector fallback)
	{
		HST_ZoneState sourceZone = state.FindZone(sourceZoneId);
		if (sourceZone)
			return sourceZone.m_vPosition;

		return fallback;
	}

	protected HST_ZoneState FindFallbackTargetZone(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				return zone;
		}

		if (state.m_aZones.Count() > 0)
			return state.m_aZones[0];

		return null;
	}

	protected string SelectGroupPrefab(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!request)
			return "";

		string factionKey = request.m_sFactionKey;
		HST_ESupportRequestType supportType = request.m_eType;
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (!faction)
			return "";

		int seed = BuildSupportGroupSeed(state, request);
		bool qrfStyle = supportType == HST_ESupportRequestType.HST_SUPPORT_QRF || supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
		if (qrfStyle && faction.m_aQRFGroupPool.Count() > 0)
			return HST_DefaultCatalog.SelectWeightedPrefab(faction.m_aQRFGroupPool, seed + 701);

		if (faction.m_aPatrolGroupPool.Count() > 0)
			return HST_DefaultCatalog.SelectWeightedPrefab(faction.m_aPatrolGroupPool, seed + 503);

		if (faction.m_aGroupPool.Count() > 0)
			return HST_DefaultCatalog.SelectWeightedPrefab(faction.m_aGroupPool, seed + 307);

		if ((supportType == HST_ESupportRequestType.HST_SUPPORT_QRF || supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY) && faction.m_aQRFGroupPrefabs.Count() > 0)
			return faction.m_aQRFGroupPrefabs[HST_DefaultCatalog.PositiveMod(seed, faction.m_aQRFGroupPrefabs.Count())];

		if (faction.m_aPatrolGroupPrefabs.Count() > 0)
			return faction.m_aPatrolGroupPrefabs[HST_DefaultCatalog.PositiveMod(seed, faction.m_aPatrolGroupPrefabs.Count())];

		if (faction.m_aGroupPrefabs.Count() > 0)
			return faction.m_aGroupPrefabs[HST_DefaultCatalog.PositiveMod(seed, faction.m_aGroupPrefabs.Count())];

		return "";
	}

	protected int BuildSupportGroupSeed(HST_CampaignState state, HST_SupportRequestState request)
	{
		int seed = 613;
		if (state)
			seed += state.m_iCampaignSeed * 17 + state.m_iElapsedSeconds * 5 + state.m_aSupportRequests.Count() * 29;
		if (request)
			seed += request.m_sRequestId.Length() * 47 + request.m_sTargetZoneId.Length() * 89 + request.m_iRequestedAtSecond;
		if (seed < 0)
			seed = -seed;

		return seed;
	}
}
