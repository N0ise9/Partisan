class HST_DefaultCatalog
{
	static HST_BalanceConfig CreateBalance()
	{
		return new HST_BalanceConfig();
	}

	static HST_CampaignPreset CreateRhsEveronPreset()
	{
		HST_CampaignPreset preset = new HST_CampaignPreset();
		preset.m_sPresetId = "rhs_everon";
		preset.m_sDisplayName = "h-istasi Everon: FIA vs USMC vs AFRF";
		preset.m_sResistanceFactionKey = "FIA";
		preset.m_sOccupierFactionKey = "RHS_USAF";
		preset.m_sInvaderFactionKey = "RHS_AFRF";
		preset.m_sBalanceConfig = "Configs/HST/Balance/HST_CE311_Balance.conf";
		preset.m_sMapDefinition = "Configs/HST/Maps/HST_Everon.conf";
		preset.m_sMissionRegistry = "Configs/HST/Missions/HST_CE311_Missions.conf";
		preset.m_aUnavailableCapabilities.Insert("fixed_wing_aircraft");
		preset.m_aUnavailableCapabilities.Insert("sead");
		preset.m_aUnavailableCapabilities.Insert("artillery");
		return preset;
	}

	static void AddDefaultFactionPools(HST_CampaignState state, HST_BalanceConfig balance, HST_CampaignPreset preset)
	{
		HST_FactionPoolState resistance = new HST_FactionPoolState();
		resistance.m_sFactionKey = preset.m_sResistanceFactionKey;
		state.m_aFactionPools.Insert(resistance);

		HST_FactionPoolState occupier = new HST_FactionPoolState();
		occupier.m_sFactionKey = preset.m_sOccupierFactionKey;
		occupier.m_iAttackResources = balance.m_iStartingOccupierAttackPool;
		occupier.m_iSupportResources = balance.m_iStartingOccupierSupportPool;
		state.m_aFactionPools.Insert(occupier);

		HST_FactionPoolState invader = new HST_FactionPoolState();
		invader.m_sFactionKey = preset.m_sInvaderFactionKey;
		invader.m_iAttackResources = balance.m_iStartingInvaderAttackPool;
		invader.m_iSupportResources = balance.m_iStartingInvaderSupportPool;
		state.m_aFactionPools.Insert(invader);
	}

	static array<ref HST_HideoutDefinition> CreateHideouts()
	{
		array<ref HST_HideoutDefinition> hideouts = {};
		hideouts.Insert(NewHideout("hideout_north_forest", "North Forest", "3200 0 4100"));
		hideouts.Insert(NewHideout("hideout_central_hills", "Central Hills", "3400 0 4500"));
		hideouts.Insert(NewHideout("hideout_south_woods", "South Woods", "2300 0 8500"));
		return hideouts;
	}

	static HST_FactionTemplate CreateFactionTemplate(string factionKey)
	{
		if (factionKey == "FIA")
			return CreateFiaTemplate();

		if (factionKey == "RHS_USAF")
			return CreateRhsUsmcTemplate();

		if (factionKey == "RHS_AFRF")
			return CreateRhsAfrfTemplate();

		return null;
	}

	static void AddDefaultZones(HST_CampaignState state, HST_CampaignPreset preset)
	{
		state.m_aZones.Insert(NewZoneState("town_saint_pierre", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "9716 0 1526", 80, 12, "route_saint_pierre", "qrf_saint_pierre", "site_saint_pierre"));
		state.m_aZones.Insert(NewZoneState("town_provins", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5596 0 6027", 35, 10, "route_provins", "qrf_provins", "site_provins"));
		state.m_aZones.Insert(NewZoneState("town_entre_deux", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5754 0 7053", 30, 10, "route_entre_deux", "qrf_entre_deux", "site_entre_deux"));
		state.m_aZones.Insert(NewZoneState("town_chotain", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7084 0 6017", 35, 12, "route_chotain", "qrf_chotain", "site_chotain"));
		state.m_aZones.Insert(NewZoneState("town_montignac", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4731 0 6970", 65, 16, "route_montignac", "qrf_montignac", "site_montignac"));
		state.m_aZones.Insert(NewZoneState("town_laruns", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7573 0 5525", 30, 10, "route_laruns", "qrf_laruns", "site_laruns"));
		state.m_aZones.Insert(NewZoneState("town_levie", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7463 0 4736", 30, 10, "route_levie", "qrf_levie", "site_levie"));
		state.m_aZones.Insert(NewZoneState("town_morton", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5035 0 4000", 45, 12, "route_morton", "qrf_morton", "site_morton"));
		state.m_aZones.Insert(NewZoneState("town_meaux", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4464 0 9468", 40, 12, "route_meaux", "qrf_meaux", "site_meaux"));
		state.m_aZones.Insert(NewZoneState("town_tyrone", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4933 0 9053", 35, 10, "route_tyrone", "qrf_tyrone", "site_tyrone"));
		state.m_aZones.Insert(NewZoneState("town_gravette", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4106 0 7794", 35, 10, "route_gravette", "qrf_gravette", "site_gravette"));
		state.m_aZones.Insert(NewZoneState("town_villeneuve", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "2885 0 6402", 45, 12, "route_villeneuve", "qrf_villeneuve", "site_villeneuve"));
		state.m_aZones.Insert(NewZoneState("town_le_moule", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "2570 0 5423", 40, 12, "route_le_moule", "qrf_le_moule", "site_le_moule"));
		state.m_aZones.Insert(NewZoneState("town_lamentin", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "1204 0 5984", 45, 12, "route_lamentin", "qrf_lamentin", "site_lamentin"));
		state.m_aZones.Insert(NewZoneState("town_regina", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7241 0 2312", 45, 12, "route_regina", "qrf_regina", "site_regina"));
		state.m_aZones.Insert(NewZoneState("town_figari", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5281 0 5361", 30, 8, "route_figari", "qrf_figari", "site_figari"));
		state.m_aZones.Insert(NewZoneState("town_durras", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "8827 0 2769", 30, 8, "route_durras", "qrf_durras", "site_durras"));
		state.m_aZones.Insert(NewZoneState("town_saint_philippe", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4542 0 10477", 35, 10, "route_saint_philippe", "qrf_saint_philippe", "site_saint_philippe"));
		state.m_aZones.Insert(NewZoneState("outpost_north", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "4700 0 3500", 25, 16, "route_outpost_north", "qrf_outpost_north", "site_outpost_north"));
		state.m_aZones.Insert(NewZoneState("outpost_south", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "4200 0 8900", 25, 16, "route_outpost_south", "qrf_outpost_south", "site_outpost_south"));
		state.m_aZones.Insert(NewZoneState("airfield_main", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_AIRFIELD, "7500 0 7600", 150, 24, "route_airfield_main", "qrf_airfield_main", "site_airfield_main"));
		state.m_aZones.Insert(NewZoneState("seaport_main", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_SEAPORT, "8650 0 11150", 90, 20, "route_seaport_main", "qrf_seaport_main", "site_seaport_main"));
		state.m_aZones.Insert(NewZoneState("factory_central", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_FACTORY, "6200 0 6200", 120, 12, "route_factory_central", "qrf_factory_central", "site_factory_central"));
		state.m_aZones.Insert(NewZoneState("resource_north", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "5200 0 4200", 75, 10, "route_resource_north", "qrf_resource_north", "site_resource_north"));
		state.m_aZones.Insert(NewZoneState("resource_south", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "5100 0 9300", 75, 10, "route_resource_south", "qrf_resource_south", "site_resource_south"));
		state.m_aZones.Insert(NewZoneState("radio_north", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "1932 0 5139", 20, 8, "route_radio_north", "qrf_radio_north", "site_radio_north"));
		state.m_aZones.Insert(NewZoneState("radio_south", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "10009 0 1550", 20, 8, "route_radio_south", "qrf_radio_south", "site_radio_south"));
	}

	static void AddDefaultGarrisons(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
				continue;

			int infantryCount = Math.Max(2, zone.m_iGarrisonSlots / 2);
			int vehicleCount;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST || zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
				vehicleCount = 1;

			state.m_aGarrisons.Insert(NewGarrisonState(zone.m_sZoneId, zone.m_sOwnerFactionKey, infantryCount, vehicleCount));
		}
	}

	static bool IsKnownHideout(string hideoutId)
	{
		foreach (HST_HideoutDefinition hideout : CreateHideouts())
		{
			if (hideout.m_sHideoutId == hideoutId)
				return true;
		}

		return false;
	}

	static string GetDefaultHideoutId()
	{
		return "hideout_central_hills";
	}

	static vector GetHideoutPosition(string hideoutId)
	{
		foreach (HST_HideoutDefinition hideout : CreateHideouts())
		{
			if (hideout.m_sHideoutId == hideoutId)
				return hideout.m_vPosition;
		}

		return "0 0 0";
	}

	static array<ref HST_MissionDefinition> CreateMissionRegistry()
	{
		array<ref HST_MissionDefinition> missions = {};
		missions.Insert(NewMission("assassinate_officer", "Assassinate Officer", HST_EMissionCategory.HST_MISSION_ASSASSINATION, 2700, 400, 2));
		missions.Insert(NewMission("assassinate_traitor", "Assassinate Traitor", HST_EMissionCategory.HST_MISSION_ASSASSINATION, 2700, 350, 2));
		missions.Insert(NewMission("assassinate_specops", "Assassinate Spec Ops", HST_EMissionCategory.HST_MISSION_ASSASSINATION, 3600, 650, 3));
		missions.Insert(NewMission("conquest_resource", "Capture Resource", HST_EMissionCategory.HST_MISSION_CONQUEST, 5400, 700, 5));
		missions.Insert(NewMission("conquest_outpost", "Capture Outpost", HST_EMissionCategory.HST_MISSION_CONQUEST, 5400, 900, 6));
		missions.Insert(NewMission("convoy_ammo", "Intercept Ammo Convoy", HST_EMissionCategory.HST_MISSION_CONVOY, 3600, 500, 3));
		missions.Insert(NewMission("convoy_armored", "Intercept Armored Convoy", HST_EMissionCategory.HST_MISSION_CONVOY, 4200, 800, 4));
		missions.Insert(NewMission("convoy_money", "Intercept Money Convoy", HST_EMissionCategory.HST_MISSION_CONVOY, 3600, 700, 3));
		missions.Insert(NewMission("convoy_prisoners", "Intercept Prisoner Convoy", HST_EMissionCategory.HST_MISSION_CONVOY, 3600, 450, 3));
		missions.Insert(NewMission("convoy_reinforcements", "Intercept Reinforcements", HST_EMissionCategory.HST_MISSION_CONVOY, 3600, 550, 4));
		missions.Insert(NewMission("convoy_supplies", "Intercept Supply Convoy", HST_EMissionCategory.HST_MISSION_CONVOY, 3600, 500, 3));
		missions.Insert(NewMission("destroy_radio_tower", "Destroy Radio Tower", HST_EMissionCategory.HST_MISSION_DESTROY, 3600, 450, 3));
		missions.Insert(NewMission("destroy_downed_helicopter", "Destroy Downed Helicopter", HST_EMissionCategory.HST_MISSION_DESTROY, 3000, 500, 3));
		missions.Insert(NewMission("destroy_or_steal_armor", "Steal or Destroy Armor", HST_EMissionCategory.HST_MISSION_DESTROY, 4200, 700, 4));
		missions.Insert(NewMission("logistics_bank", "Rob Bank", HST_EMissionCategory.HST_MISSION_LOGISTICS, 2400, 550, 2));
		missions.Insert(NewMission("logistics_salvage_supplies", "Salvage Supplies", HST_EMissionCategory.HST_MISSION_LOGISTICS, 3000, 450, 2));
		missions.Insert(NewMission("logistics_ammo_truck", "Recover Ammo Truck", HST_EMissionCategory.HST_MISSION_LOGISTICS, 3600, 600, 3));
		missions.Insert(NewMission("logistics_weapons_truck", "Recover Weapons Truck", HST_EMissionCategory.HST_MISSION_LOGISTICS, 3600, 650, 3));
		missions.Insert(NewMission("rescue_pows", "Rescue POWs", HST_EMissionCategory.HST_MISSION_RESCUE, 3600, 500, 3));
		missions.Insert(NewMission("rescue_refugees", "Rescue Refugees", HST_EMissionCategory.HST_MISSION_RESCUE, 3600, 400, 2));
		missions.Insert(NewMission("dynamic_defend_petros", "Defend Petros", HST_EMissionCategory.HST_MISSION_DYNAMIC, 2400, 0, 6));
		missions.Insert(NewMission("dynamic_stop_tower_rebuild", "Stop Tower Rebuild", HST_EMissionCategory.HST_MISSION_DYNAMIC, 2400, 350, 3));
		missions.Insert(NewMission("dynamic_minor_city_task", "Minor City Task", HST_EMissionCategory.HST_MISSION_DYNAMIC, 1800, 250, 1));
		missions.Insert(NewMission("dynamic_city_flip_battle", "City Flip Battle", HST_EMissionCategory.HST_MISSION_DYNAMIC, 3600, 500, 4));
		missions.Insert(NewMission("dynamic_gun_shop", "Gun Shop", HST_EMissionCategory.HST_MISSION_DYNAMIC, 3600, 350, 2));
		missions.Insert(NewMission("support_city_supplies", "Deliver City Supplies", HST_EMissionCategory.HST_MISSION_SUPPORT, 3600, 300, 1));
		return missions;
	}

	private static HST_HideoutDefinition NewHideout(string hideoutId, string displayName, vector position)
	{
		HST_HideoutDefinition hideout = new HST_HideoutDefinition();
		hideout.m_sHideoutId = hideoutId;
		hideout.m_sDisplayName = displayName;
		hideout.m_vPosition = position;
		return hideout;
	}

	private static HST_ZoneState NewZoneState(string zoneId, string ownerFactionKey, HST_EZoneType zoneType, vector position, int incomeValue, int garrisonSlots, string patrolRouteId, string qrfRouteId, string missionSiteId)
	{
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = zoneId;
		zone.m_sOwnerFactionKey = ownerFactionKey;
		zone.m_eType = zoneType;
		zone.m_vPosition = position;
		zone.m_iSupport = 0;
		zone.m_iIncomeValue = incomeValue;
		zone.m_iGarrisonSlots = garrisonSlots;
		zone.m_iActivationRadiusMeters = 1200;
		zone.m_sPatrolRouteId = patrolRouteId;
		zone.m_sQRFRouteId = qrfRouteId;
		zone.m_sMissionSiteId = missionSiteId;
		return zone;
	}

	private static HST_GarrisonState NewGarrisonState(string zoneId, string factionKey, int infantryCount, int vehicleCount)
	{
		HST_GarrisonState garrison = new HST_GarrisonState();
		garrison.m_sZoneId = zoneId;
		garrison.m_sFactionKey = factionKey;
		garrison.m_iInfantryCount = infantryCount;
		garrison.m_iVehicleCount = vehicleCount;
		return garrison;
	}

	private static HST_FactionTemplate CreateFiaTemplate()
	{
		HST_FactionTemplate faction = new HST_FactionTemplate();
		faction.m_sTemplateId = "fia_resistance";
		faction.m_sFactionKey = "FIA";
		faction.m_sDisplayName = "FIA Resistance";
		faction.m_bPlayable = true;
		faction.m_aGroupPrefabs.Insert("{8B4D49A9F324E7D5}Prefabs/Groups/PlayableGroup.et");
		faction.m_aPatrolGroupPrefabs.Insert("{8B4D49A9F324E7D5}Prefabs/Groups/PlayableGroup.et");
		faction.m_aQRFGroupPrefabs.Insert("{8B4D49A9F324E7D5}Prefabs/Groups/PlayableGroup.et");
		return faction;
	}

	private static HST_FactionTemplate CreateRhsUsmcTemplate()
	{
		HST_FactionTemplate faction = new HST_FactionTemplate();
		faction.m_sTemplateId = "rhs_usmc_occupier";
		faction.m_sFactionKey = "RHS_USAF";
		faction.m_sDisplayName = "RHS USMC Occupiers";
		faction.m_aGroupPrefabs.Insert("{19843E954790DF28}Prefabs/Groups/BLUFOR/RHS_USAF/RHS_USAF_USMC_MEF/Group_USAF_USMC_MEF_FireTeam.et");
		faction.m_aGroupPrefabs.Insert("{F831DFB4A9B46152}Prefabs/Groups/BLUFOR/RHS_USAF/RHS_USAF_USMC_MEF/Group_USAF_USMC_MEF_RifleSquad.et");
		faction.m_aGroupPrefabs.Insert("{0E3DC6A52A5F959E}Prefabs/Groups/BLUFOR/RHS_USAF/RHS_USAF_USMC_MEF/Group_USAF_USMC_MEF_PlatoonHQ.et");
		faction.m_aPatrolGroupPrefabs.Insert("{19843E954790DF28}Prefabs/Groups/BLUFOR/RHS_USAF/RHS_USAF_USMC_MEF/Group_USAF_USMC_MEF_FireTeam.et");
		faction.m_aPatrolGroupPrefabs.Insert("{7A43006F3F254FB2}Prefabs/Groups/BLUFOR/RHS_USAF/RHS_USAF_MARSOC/Group_USAF_USMC_MARSOC_SentryTeam.et");
		faction.m_aQRFGroupPrefabs.Insert("{F831DFB4A9B46152}Prefabs/Groups/BLUFOR/RHS_USAF/RHS_USAF_USMC_MEF/Group_USAF_USMC_MEF_RifleSquad.et");
		faction.m_aQRFGroupPrefabs.Insert("{D31888A77B1C4310}Prefabs/Groups/BLUFOR/RHS_USAF/RHS_USAF_FORECON/Group_USAF_USMC_FORECON_Squad.et");
		return faction;
	}

	private static HST_FactionTemplate CreateRhsAfrfTemplate()
	{
		HST_FactionTemplate faction = new HST_FactionTemplate();
		faction.m_sTemplateId = "rhs_afrf_invader";
		faction.m_sFactionKey = "RHS_AFRF";
		faction.m_sDisplayName = "RHS AFRF Invaders";
		faction.m_aGroupPrefabs.Insert("{A6A3EDA237E3D336}Prefabs/Groups/OPFOR/RHS_AFRF/MSV/VKPO_Summer/AmbientPatrols/Group_RHS_RF_MSV_VKPO_S_FireGroup_NotSpawned.et");
		faction.m_aGroupPrefabs.Insert("{D9523F26504A8D59}Prefabs/Groups/OPFOR/RHS_AFRF/SPEC/VVRG/Group_RHS_hSPEC_VVRG_TEAM.et");
		faction.m_aPatrolGroupPrefabs.Insert("{A6A3EDA237E3D336}Prefabs/Groups/OPFOR/RHS_AFRF/MSV/VKPO_Summer/AmbientPatrols/Group_RHS_RF_MSV_VKPO_S_FireGroup_NotSpawned.et");
		faction.m_aQRFGroupPrefabs.Insert("{D9523F26504A8D59}Prefabs/Groups/OPFOR/RHS_AFRF/SPEC/VVRG/Group_RHS_hSPEC_VVRG_TEAM.et");
		faction.m_aQRFGroupPrefabs.Insert("{622DDA327B669B67}Prefabs/Groups/OPFOR/RHS_AFRF/SPEC/SSO/Group_RHS_hSPEC_SSO_ATTeam.et");
		return faction;
	}

	private static HST_MissionDefinition NewMission(string missionId, string displayName, HST_EMissionCategory category, int duration, int rewardMoney, int failureAggression)
	{
		HST_MissionDefinition mission = new HST_MissionDefinition();
		mission.m_sMissionId = missionId;
		mission.m_sDisplayName = displayName;
		mission.m_eCategory = category;
		mission.m_iDurationSeconds = duration;
		mission.m_iRewardMoney = rewardMoney;
		mission.m_iFailureAggression = failureAggression;
		return mission;
	}
}
