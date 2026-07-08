class HST_ConvoyOutcomeService
{
	static const string PRIMITIVE_CONVOY_INTERCEPT = "convoy_intercept";
	static const string ROLE_CONVOY_VEHICLE = "convoy_vehicle";
	static const string ROLE_CONVOY_PAYLOAD = "convoy_payload";
	static const string ROLE_CONVOY_CAPTIVE = "convoy_captive";
	static const string MISSION_CONVOY_GROUP_PREFIX = "mission_convoy_";
	static const string PHASE_CONVOY_ELIMINATED = "convoy_eliminated";
	static const string PHASE_CONVOY_ARRIVED = "convoy_arrived";
	static const string PHASE_FAILED = "failed";
	static const string PHASE_CAPTURED = "captured";
	static const string EVENT_CONVOY_COMPLETE = "convoy_complete";
	static const string EVENT_CONVOY_FAILED = "convoy_failed";
	static const int MONEY_CONVOY_FULL_REWARD = 700;
	static const int PRISONER_CONVOY_HR_REWARD = 5;
	static const int PRISONER_CONVOY_SUPPORT_REWARD = 10;
	static const int REINFORCEMENT_CONVOY_INFANTRY_GAIN = 6;
	static const int REINFORCEMENT_CONVOY_VEHICLE_GAIN = 1;
	static const int SUPPLY_CONVOY_OCCUPIER_SUPPORT_GAIN = 10;
	static const int SUPPLY_CONVOY_FIA_SUPPORT_GAIN = 12;

	bool TickConvoyOutcomes(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_EconomyService economy, HST_ArsenalService arsenal, HST_GarrisonService garrisons, HST_TownService towns, HST_StrategicService strategic)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsConvoyMission(mission))
				continue;

			string result;
			if (ShouldApplyConvoyArrivalOutcome(mission))
				changed = OnConvoyArrived(state, preset, balance, mission, garrisons, towns, strategic, result) || changed;
			if (ShouldApplyConvoyCrewEliminatedOutcome(state, mission))
				changed = OnConvoyCrewEliminated(state, preset, balance, mission, economy, strategic, result) || changed;

			foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
			{
				if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bOutcomeApplied)
					continue;

				if (asset.m_sRole == ROLE_CONVOY_VEHICLE && IsAssetCaptured(asset))
				{
					changed = OnConvoyVehicleCaptured(state, preset, balance, mission, asset, arsenal, strategic, result) || changed;
					continue;
				}

				if ((asset.m_sRole == ROLE_CONVOY_PAYLOAD || asset.m_sRole == ROLE_CONVOY_CAPTIVE) && asset.m_bDelivered)
					changed = OnConvoyCargoDelivered(state, preset, balance, mission, asset, economy, towns, strategic, result) || changed;
			}

			if (ShouldApplyConvoyExpiredOutcome(mission))
				changed = OnConvoyMissionExpired(state, preset, balance, mission, garrisons, towns, strategic, result) || changed;
		}

		return changed;
	}

	bool OnConvoyArrived(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ActiveMissionState mission, HST_GarrisonService garrisons, HST_TownService towns, HST_StrategicService strategic, out string result)
	{
		result = "";
		if (!state || !mission || mission.m_bConvoyArrivalOutcomeApplied)
			return false;

		string targetZoneId = ResolveConvoyOutcomeTargetZoneId(state, mission, "convoy_arrived");
		HST_StrategicEventApplyResult strategicEvent = BeginConvoyStrategicEvent(state, preset, strategic, mission, "convoy_arrived", targetZoneId, mission.m_sInstanceId, ResolveConvoyArrivalTargetFactionKey(preset, mission));
		bool applied;
		if (IsReinforcementConvoy(mission))
			applied = ApplyReinforcementArrival(state, preset, mission, garrisons, result);
		else if (IsSupplyConvoy(mission))
			applied = ApplySupplyArrival(state, mission, towns, result);
		else
		{
			mission.m_bConvoyArrivalOutcomeApplied = true;
			result = string.Format("%1 | arrived | no mission-specific arrival outcome", ConvoyLabel(mission));
			SetMissionOutcomeSummary(mission, result);
			applied = true;
		}

		return CompleteConvoyStrategicEvent(state, strategic, strategicEvent, applied, result);
	}

	bool OnConvoyCrewEliminated(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ActiveMissionState mission, HST_EconomyService economy, HST_StrategicService strategic, out string result)
	{
		result = "";
		if (!state || !mission || mission.m_bConvoyCrewEliminatedOutcomeApplied)
			return false;

		HST_StrategicEventApplyResult strategicEvent = BeginConvoyStrategicEvent(state, preset, strategic, mission, "convoy_crew_eliminated", mission.m_sTargetZoneId, mission.m_sInstanceId, ResolveResistanceFactionKey(preset));

		mission.m_bConvoyCrewEliminatedOutcomeApplied = true;
		if (IsMoneyConvoy(mission))
			result = "money convoy | crew eliminated | full payout requires money delivery to HQ";
		else if (IsPrisonerConvoy(mission))
			result = "prisoner convoy | crew eliminated | full reward requires prisoner extraction";
		else if (IsReinforcementConvoy(mission))
			result = "reinforcement convoy | intercepted | target garrison not reinforced";
		else if (IsSupplyConvoy(mission))
			result = "supply convoy | intercepted | occupier support delivery prevented";
		else if (result.IsEmpty())
			result = string.Format("%1 | crew eliminated", ConvoyLabel(mission));

		SetMissionOutcomeSummary(mission, result);
		return CompleteConvoyStrategicEvent(state, strategic, strategicEvent, true, result);
	}

	bool OnConvoyVehicleCaptured(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ActiveMissionState mission, HST_MissionAssetState vehicleAsset, HST_ArsenalService arsenal, HST_StrategicService strategic, out string result)
	{
		result = "";
		if (!state || !mission || !vehicleAsset || vehicleAsset.m_bOutcomeApplied)
			return false;

		HST_StrategicEventApplyResult strategicEvent = BeginConvoyStrategicEvent(state, preset, strategic, mission, "convoy_vehicle_captured", mission.m_sTargetZoneId, vehicleAsset.m_sAssetId, ResolveResistanceFactionKey(preset));

		bool applied;
		if (IsAmmoConvoy(mission))
			applied = ApplyAmmoVehicleCapture(state, mission, vehicleAsset, result);
		else if (IsArmoredConvoy(mission))
			applied = ApplyArmoredVehicleCapture(mission, vehicleAsset, result);
		else
		{
			vehicleAsset.m_bOutcomeApplied = true;
			vehicleAsset.m_sOutcomeKind = "convoy_vehicle_captured";
			mission.m_bConvoyVehicleCapturedOutcomeApplied = true;
			result = string.Format("%1 | vehicle captured", ConvoyLabel(mission));
			SetMissionOutcomeSummary(mission, result);
			applied = true;
		}

		return CompleteConvoyStrategicEvent(state, strategic, strategicEvent, applied, result);
	}

	bool OnConvoyCargoDelivered(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ActiveMissionState mission, HST_MissionAssetState cargoAsset, HST_EconomyService economy, HST_TownService towns, HST_StrategicService strategic, out string result)
	{
		result = "";
		if (!state || !mission || !cargoAsset || cargoAsset.m_bOutcomeApplied)
			return false;

		string targetZoneId = ResolveConvoyOutcomeTargetZoneId(state, mission, "convoy_cargo_delivered");
		HST_StrategicEventApplyResult strategicEvent = BeginConvoyStrategicEvent(state, preset, strategic, mission, "convoy_cargo_delivered", targetZoneId, cargoAsset.m_sAssetId, ResolveResistanceFactionKey(preset));

		bool applied;
		if (IsMoneyConvoy(mission))
			applied = ApplyMoneyDelivery(state, mission, cargoAsset, economy, result);
		else if (IsPrisonerConvoy(mission))
			applied = ApplyPrisonerExtraction(state, mission, cargoAsset, economy, towns, result);
		else if (IsSupplyConvoy(mission))
			applied = ApplySupplyDelivery(state, mission, cargoAsset, towns, result);
		else
		{
			cargoAsset.m_bOutcomeApplied = true;
			cargoAsset.m_sOutcomeKind = "convoy_cargo_delivered";
			mission.m_bConvoyCargoDeliveredOutcomeApplied = true;
			result = string.Format("%1 | cargo delivered", ConvoyLabel(mission));
			SetMissionOutcomeSummary(mission, result);
			applied = true;
		}

		return CompleteConvoyStrategicEvent(state, strategic, strategicEvent, applied, result);
	}

	bool OnConvoyMissionExpired(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ActiveMissionState mission, HST_GarrisonService garrisons, HST_TownService towns, HST_StrategicService strategic, out string result)
	{
		result = "";
		if (!state || !mission || mission.m_bConvoyExpiredOutcomeApplied)
			return false;

		HST_StrategicEventApplyResult strategicEvent = BeginConvoyStrategicEvent(state, preset, strategic, mission, "convoy_expired", mission.m_sTargetZoneId, mission.m_sInstanceId, ResolveOccupierFactionKey(preset));
		mission.m_bConvoyExpiredOutcomeApplied = true;
		result = string.Format("%1 | expired | no mission-specific reward applied", ConvoyLabel(mission));
		SetMissionOutcomeSummary(mission, result);
		return CompleteConvoyStrategicEvent(state, strategic, strategicEvent, true, result);
	}

	protected HST_StrategicEventApplyResult BeginConvoyStrategicEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_ActiveMissionState mission, string kind, string targetZoneId, string sourceId, string targetFactionKey)
	{
		if (!strategic)
			return null;

		return strategic.BeginConvoyOutcomeEvent(state, preset, mission, kind, targetZoneId, sourceId, targetFactionKey);
	}

	protected bool CompleteConvoyStrategicEvent(HST_CampaignState state, HST_StrategicService strategic, HST_StrategicEventApplyResult strategicEvent, bool applied, string result)
	{
		if (!strategic || !strategicEvent)
			return applied;

		if (!applied)
		{
			strategic.DiscardStrategicEvent(state, strategicEvent);
			return false;
		}

		if (strategicEvent.m_Event && !result.IsEmpty())
			strategicEvent.m_Event.m_sReason = result;
		strategic.CompleteStrategicEvent(state, strategicEvent, true, true);
		return true;
	}

	protected string ResolveConvoyOutcomeTargetZoneId(HST_CampaignState state, HST_ActiveMissionState mission, string kind)
	{
		if (!state || !mission)
			return "";

		if (kind == "convoy_cargo_delivered" || (kind == "convoy_arrived" && IsSupplyConvoy(mission)))
		{
			HST_ZoneState supportTown = ResolveSupportTown(state, mission);
			if (supportTown)
				return supportTown.m_sZoneId;
		}

		return mission.m_sTargetZoneId;
	}

	protected string ResolveConvoyArrivalTargetFactionKey(HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (IsSupplyConvoy(mission) || IsReinforcementConvoy(mission))
			return ResolveOccupierFactionKey(preset);

		return ResolveResistanceFactionKey(preset);
	}

	protected string ResolveResistanceFactionKey(HST_CampaignPreset preset)
	{
		if (preset)
			return preset.m_sResistanceFactionKey;

		return "";
	}

	protected string ResolveOccupierFactionKey(HST_CampaignPreset preset)
	{
		if (preset)
			return preset.m_sOccupierFactionKey;

		return "";
	}

	protected bool ApplyAmmoVehicleCapture(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState vehicleAsset, out string result)
	{
		result = "";
		vector position = vehicleAsset.m_vCurrentPosition;
		if (IsZeroVector(position))
			position = vehicleAsset.m_vLastKnownPosition;
		if (IsZeroVector(position))
			position = state.m_vHQPosition;

		string ammoPointId = string.Format("convoy_ammo_%1_%2", mission.m_sInstanceId, vehicleAsset.m_sAssetId);
		HST_AmmoPointState ammoPoint = FindAmmoPoint(state, ammoPointId);
		if (!ammoPoint && !mission.m_bConvoyVehicleCapturedOutcomeApplied)
		{
			ammoPoint = new HST_AmmoPointState();
			ammoPoint.m_sAmmoPointId = ammoPointId;
			ammoPoint.m_vPosition = position;
			state.m_aAmmoPoints.Insert(ammoPoint);
			result = string.Format("ammo convoy | captured ammo vehicle | ammo point %1 created", ammoPointId);
		}
		else
		{
			result = "ammo convoy | captured ammo vehicle | ammo point already recorded";
		}

		vehicleAsset.m_bOutcomeApplied = true;
		vehicleAsset.m_sOutcomeKind = "ammo_point_created";
		mission.m_bConvoyVehicleCapturedOutcomeApplied = true;
		SetMissionOutcomeSummary(mission, result);
		return true;
	}

	protected bool ApplyArmoredVehicleCapture(HST_ActiveMissionState mission, HST_MissionAssetState vehicleAsset, out string result)
	{
		result = "armored convoy | captured vehicle available through garage";
		vehicleAsset.m_bOutcomeApplied = true;
		vehicleAsset.m_sOutcomeKind = "armored_vehicle_garaged";
		mission.m_bConvoyVehicleCapturedOutcomeApplied = true;
		SetMissionOutcomeSummary(mission, result);
		return true;
	}

	protected bool ApplyMoneyDelivery(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState cargoAsset, HST_EconomyService economy, out string result)
	{
		result = "";
		if (economy)
			economy.AddFactionMoney(state, MONEY_CONVOY_FULL_REWARD);
		else
			state.m_iFactionMoney = Math.Max(0, state.m_iFactionMoney + MONEY_CONVOY_FULL_REWARD);

		cargoAsset.m_bOutcomeApplied = true;
		cargoAsset.m_sOutcomeKind = "money_delivered";
		mission.m_bConvoyCargoDeliveredOutcomeApplied = true;
		result = string.Format("money convoy | delivered to HQ | faction money +%1", MONEY_CONVOY_FULL_REWARD);
		SetMissionOutcomeSummary(mission, result);
		return true;
	}

	protected bool ApplyPrisonerExtraction(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState captiveAsset, HST_EconomyService economy, HST_TownService towns, out string result)
	{
		result = "";
		if (economy)
			economy.AddHR(state, PRISONER_CONVOY_HR_REWARD);
		else
			state.m_iHR = Math.Max(0, state.m_iHR + PRISONER_CONVOY_HR_REWARD);

		string supportResult;
		bool supportChanged = ApplyTownSupport(state, mission, towns, PRISONER_CONVOY_SUPPORT_REWARD, supportResult);
		captiveAsset.m_bOutcomeApplied = true;
		captiveAsset.m_sOutcomeKind = "prisoners_extracted";
		mission.m_bConvoyCargoDeliveredOutcomeApplied = true;
		result = string.Format("prisoner convoy | prisoners extracted | HR +%1", PRISONER_CONVOY_HR_REWARD);
		if (supportChanged)
			result = result + string.Format(" | support +%1 at %2", PRISONER_CONVOY_SUPPORT_REWARD, supportResult);
		else
			result = result + " | support unchanged: " + supportResult;
		SetMissionOutcomeSummary(mission, result);
		return true;
	}

	protected bool ApplyReinforcementArrival(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, HST_GarrisonService garrisons, out string result)
	{
		result = "";
		string zoneId = mission.m_sTargetZoneId;
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
		{
			mission.m_bConvoyArrivalOutcomeApplied = true;
			result = "reinforcement convoy | arrival outcome skipped: target zone missing";
			SetMissionOutcomeSummary(mission, result);
			return true;
		}

		string factionKey = ResolveEnemyFactionKey(state, preset, zone);
		bool applied;
		if (garrisons)
			applied = garrisons.AddAbstractForces(state, zoneId, factionKey, REINFORCEMENT_CONVOY_INFANTRY_GAIN, REINFORCEMENT_CONVOY_VEHICLE_GAIN);
		if (!applied)
			applied = AddGarrisonForcesDirect(state, zoneId, factionKey, REINFORCEMENT_CONVOY_INFANTRY_GAIN, REINFORCEMENT_CONVOY_VEHICLE_GAIN);

		mission.m_bConvoyArrivalOutcomeApplied = true;
		if (applied)
			result = string.Format("reinforcement convoy | arrived | garrison +%1 infantry", REINFORCEMENT_CONVOY_INFANTRY_GAIN);
		else
			result = "reinforcement convoy | arrival outcome failed: garrison unavailable";
		result = result + string.Format(" +%1 vehicle at %2", REINFORCEMENT_CONVOY_VEHICLE_GAIN, zoneId);
		SetMissionOutcomeSummary(mission, result);
		return true;
	}

	protected bool ApplySupplyArrival(HST_CampaignState state, HST_ActiveMissionState mission, HST_TownService towns, out string result)
	{
		result = "";
		string supportTarget;
		bool supportChanged = ApplyTownSupport(state, mission, towns, -SUPPLY_CONVOY_OCCUPIER_SUPPORT_GAIN, supportTarget);
		mission.m_bConvoyArrivalOutcomeApplied = true;
		if (supportChanged)
			result = string.Format("supply convoy | arrived | occupier pressure +%1 at %2", SUPPLY_CONVOY_OCCUPIER_SUPPORT_GAIN, supportTarget);
		else
			result = "supply convoy | arrival support unchanged: " + supportTarget;
		SetMissionOutcomeSummary(mission, result);
		return true;
	}

	protected bool ApplySupplyDelivery(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState cargoAsset, HST_TownService towns, out string result)
	{
		result = "";
		string supportTarget;
		bool supportChanged = ApplyTownSupport(state, mission, towns, SUPPLY_CONVOY_FIA_SUPPORT_GAIN, supportTarget);
		cargoAsset.m_bOutcomeApplied = true;
		cargoAsset.m_sOutcomeKind = "supplies_delivered";
		mission.m_bConvoyCargoDeliveredOutcomeApplied = true;
		if (supportChanged)
			result = string.Format("supply convoy | supplies delivered | FIA support +%1 at %2", SUPPLY_CONVOY_FIA_SUPPORT_GAIN, supportTarget);
		else
			result = "supply convoy | delivery support unchanged: " + supportTarget;
		SetMissionOutcomeSummary(mission, result);
		return true;
	}

	protected bool ApplyTownSupport(HST_CampaignState state, HST_ActiveMissionState mission, HST_TownService towns, int amount, out string targetLabel)
	{
		targetLabel = "no town target";
		HST_ZoneState town = ResolveSupportTown(state, mission);
		if (!town)
			return false;

		targetLabel = town.m_sZoneId;
		if (towns)
			return towns.AddSupport(state, town.m_sZoneId, amount);

		town.m_iSupport = Math.Max(-100, Math.Min(100, town.m_iSupport + amount));
		return true;
	}

	protected HST_ZoneState ResolveSupportTown(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return null;

		HST_ZoneState targetZone = state.FindZone(mission.m_sTargetZoneId);
		if (targetZone && targetZone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			return targetZone;

		vector searchPosition = mission.m_vTargetPosition;
		if (IsZeroVector(searchPosition) && targetZone)
			searchPosition = targetZone.m_vPosition;
		if (IsZeroVector(searchPosition))
			searchPosition = state.m_vHQPosition;

		HST_ZoneState bestTown;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
				continue;

			float distanceSq = DistanceSq2D(searchPosition, zone.m_vPosition);
			if (distanceSq >= bestDistanceSq)
				continue;

			bestDistanceSq = distanceSq;
			bestTown = zone;
		}

		return bestTown;
	}

	protected HST_AmmoPointState FindAmmoPoint(HST_CampaignState state, string ammoPointId)
	{
		if (!state || ammoPointId.IsEmpty())
			return null;

		foreach (HST_AmmoPointState ammoPoint : state.m_aAmmoPoints)
		{
			if (ammoPoint && ammoPoint.m_sAmmoPointId == ammoPointId)
				return ammoPoint;
		}

		return null;
	}

	protected bool AddGarrisonForcesDirect(HST_CampaignState state, string zoneId, string factionKey, int infantry, int vehicles)
	{
		if (!state || zoneId.IsEmpty() || factionKey.IsEmpty())
			return false;

		HST_GarrisonState garrison = state.FindGarrison(zoneId, factionKey);
		if (!garrison)
		{
			garrison = new HST_GarrisonState();
			garrison.m_sZoneId = zoneId;
			garrison.m_sFactionKey = factionKey;
			state.m_aGarrisons.Insert(garrison);
		}

		HST_ZoneState zone = state.FindZone(zoneId);
		int nextInfantry = garrison.m_iInfantryCount + Math.Max(0, infantry);
		if (zone && zone.m_iGarrisonSlots > 0)
			nextInfantry = Math.Min(zone.m_iGarrisonSlots, nextInfantry);

		garrison.m_iInfantryCount = nextInfantry;
		garrison.m_iVehicleCount = Math.Max(0, garrison.m_iVehicleCount + vehicles);
		return true;
	}

	protected string ResolveEnemyFactionKey(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState zone)
	{
		if (zone && preset && zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey && !zone.m_sOwnerFactionKey.IsEmpty())
			return zone.m_sOwnerFactionKey;
		if (preset && !preset.m_sOccupierFactionKey.IsEmpty())
			return preset.m_sOccupierFactionKey;
		return "US";
	}

	protected bool ShouldApplyConvoyArrivalOutcome(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_bConvoyArrivalOutcomeApplied)
			return false;
		if (mission.m_sRuntimePhase == PHASE_CONVOY_ARRIVED)
			return true;
		if (mission.m_sRuntimePhase == PHASE_FAILED && mission.m_sRuntimeFailureReason.Contains("destination"))
			return true;
		if (mission.m_sLastRuntimeEventKey == EVENT_CONVOY_FAILED && mission.m_sRuntimeFailureReason.Contains("destination"))
			return true;
		return false;
	}

	protected bool ShouldApplyConvoyCrewEliminatedOutcome(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_bConvoyCrewEliminatedOutcomeApplied)
			return false;
		bool eliminatedEvent = mission.m_sRuntimePhase == PHASE_CONVOY_ELIMINATED || mission.m_sLastRuntimeEventKey == EVENT_CONVOY_COMPLETE;
		if (!eliminatedEvent)
			return false;

		return HasConvoyEliminatedCrewEvidence(state, mission);
	}

	protected bool ShouldApplyConvoyExpiredOutcome(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_bConvoyExpiredOutcomeApplied)
			return false;

		return mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED;
	}

	protected bool IsAssetCaptured(HST_MissionAssetState asset)
	{
		if (!asset)
			return false;

		return asset.m_bDelivered || asset.m_sLastInteraction == PHASE_CAPTURED;
	}

	protected bool IsConvoyMission(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sRuntimePrimitive == PRIMITIVE_CONVOY_INTERCEPT;
	}

	protected bool HasConvoyEliminatedCrewEvidence(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sInstanceId.IsEmpty())
			return false;

		string groupPrefix = string.Format("%1%2_", MISSION_CONVOY_GROUP_PREFIX, mission.m_sInstanceId);
		int convoyGroups = 0;
		int eliminatedGroups = 0;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || !activeGroup.m_sGroupId.Contains(groupPrefix))
				continue;

			convoyGroups++;
			if (activeGroup.m_sRuntimeStatus != PHASE_CONVOY_ELIMINATED && activeGroup.m_sRuntimeStatus != "eliminated")
				return false;
			if (!HasConvoyCrewLiveHistory(activeGroup))
				return false;

			eliminatedGroups++;
		}

		return convoyGroups > 0 && eliminatedGroups == convoyGroups;
	}

	protected bool HasConvoyCrewLiveHistory(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return false;
		if (activeGroup.m_bEverHadLivingCrew)
			return true;
		if (activeGroup.m_iMaxObservedCrewAlive > 0)
			return true;

		return false;
	}

	protected bool IsAmmoConvoy(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == "convoy_ammo";
	}

	protected bool IsArmoredConvoy(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == "convoy_armored";
	}

	protected bool IsMoneyConvoy(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == "convoy_money";
	}

	protected bool IsPrisonerConvoy(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == "convoy_prisoners";
	}

	protected bool IsReinforcementConvoy(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == "convoy_reinforcements";
	}

	protected bool IsSupplyConvoy(HST_ActiveMissionState mission)
	{
		return mission && mission.m_sMissionId == "convoy_supplies";
	}

	protected string ConvoyLabel(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "convoy";

		string label = mission.m_sMissionId;
		label.Replace("_", " ");
		return label;
	}

	protected void SetMissionOutcomeSummary(HST_ActiveMissionState mission, string result)
	{
		if (!mission || result.IsEmpty())
			return;

		mission.m_sConvoyOutcomeSummary = result;
		Print("h-istasi convoy outcome | " + result);
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
