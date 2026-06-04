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
		state.m_aZones.Insert(NewZoneState("outpost_lamentin", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "1810 0 6120", 35, 14, "route_outpost_lamentin", "qrf_outpost_lamentin", "site_outpost_lamentin"));
		state.m_aZones.Insert(NewZoneState("outpost_morton", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "5330 0 3780", 35, 14, "route_outpost_morton", "qrf_outpost_morton", "site_outpost_morton"));
		state.m_aZones.Insert(NewZoneState("outpost_regina", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "7625 0 2575", 35, 14, "route_outpost_regina", "qrf_outpost_regina", "site_outpost_regina"));
		state.m_aZones.Insert(NewZoneState("outpost_airfield_west", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "6900 0 7400", 40, 18, "route_outpost_airfield_west", "qrf_outpost_airfield_west", "site_outpost_airfield_west"));
		state.m_aZones.Insert(NewZoneState("outpost_montignac_hills", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "5220 0 7350", 35, 14, "route_outpost_montignac_hills", "qrf_outpost_montignac_hills", "site_outpost_montignac_hills"));
		state.m_aZones.Insert(NewZoneState("outpost_seaport_guard", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "8250 0 10650", 40, 18, "route_outpost_seaport_guard", "qrf_outpost_seaport_guard", "site_outpost_seaport_guard"));
		state.m_aZones.Insert(NewZoneState("factory_west", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_FACTORY, "2500 0 6100", 95, 12, "route_factory_west", "qrf_factory_west", "site_factory_west"));
		state.m_aZones.Insert(NewZoneState("factory_saint_pierre", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_FACTORY, "9360 0 1880", 110, 14, "route_factory_saint_pierre", "qrf_factory_saint_pierre", "site_factory_saint_pierre"));
		state.m_aZones.Insert(NewZoneState("resource_lumber_lamentin", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "1540 0 5550", 55, 8, "route_resource_lumber_lamentin", "qrf_resource_lumber_lamentin", "site_resource_lumber_lamentin"));
		state.m_aZones.Insert(NewZoneState("resource_lumber_villeneuve", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "3200 0 6750", 55, 8, "route_resource_lumber_villeneuve", "qrf_resource_lumber_villeneuve", "site_resource_lumber_villeneuve"));
		state.m_aZones.Insert(NewZoneState("resource_quarry_levie", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "7120 0 4300", 65, 10, "route_resource_quarry_levie", "qrf_resource_quarry_levie", "site_resource_quarry_levie"));
		state.m_aZones.Insert(NewZoneState("resource_fuel_airfield", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "7160 0 8040", 90, 12, "route_resource_fuel_airfield", "qrf_resource_fuel_airfield", "site_resource_fuel_airfield"));
		state.m_aZones.Insert(NewZoneState("resource_farm_laruns", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "7900 0 6020", 50, 8, "route_resource_farm_laruns", "qrf_resource_farm_laruns", "site_resource_farm_laruns"));
		state.m_aZones.Insert(NewZoneState("resource_farm_provins", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "6000 0 5850", 50, 8, "route_resource_farm_provins", "qrf_resource_farm_provins", "site_resource_farm_provins"));
		state.m_aZones.Insert(NewZoneState("resource_mine_montignac", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "5600 0 6550", 70, 10, "route_resource_mine_montignac", "qrf_resource_mine_montignac", "site_resource_mine_montignac"));
		state.m_aZones.Insert(NewZoneState("resource_port_saint_pierre", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "9900 0 1830", 70, 10, "route_resource_port_saint_pierre", "qrf_resource_port_saint_pierre", "site_resource_port_saint_pierre"));
		state.m_aZones.Insert(NewZoneState("resource_depot_central", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "6060 0 6680", 80, 12, "route_resource_depot_central", "qrf_resource_depot_central", "site_resource_depot_central"));
		state.m_aZones.Insert(NewZoneState("resource_depot_south", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "4550 0 9720", 80, 12, "route_resource_depot_south", "qrf_resource_depot_south", "site_resource_depot_south"));
		state.m_aZones.Insert(NewZoneState("resource_depot_east", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "8720 0 3560", 80, 12, "route_resource_depot_east", "qrf_resource_depot_east", "site_resource_depot_east"));
		state.m_aZones.Insert(NewZoneState("seaport_saint_pierre", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_SEAPORT, "10040 0 1710", 100, 18, "route_seaport_saint_pierre", "qrf_seaport_saint_pierre", "site_seaport_saint_pierre"));
		state.m_aZones.Insert(NewZoneState("radio_central", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "6060 0 6460", 25, 8, "route_radio_central", "qrf_radio_central", "site_radio_central"));
		state.m_aZones.Insert(NewZoneState("radio_west", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "2280 0 5850", 25, 8, "route_radio_west", "qrf_radio_west", "site_radio_west"));
		state.m_aZones.Insert(NewZoneState("bank_saint_pierre", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_BANK, "9729 0 1538", 65, 6, "route_bank_saint_pierre", "qrf_bank_saint_pierre", "site_bank_saint_pierre"));
		state.m_aZones.Insert(NewZoneState("bank_montignac", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_BANK, "4755 0 7005", 55, 6, "route_bank_montignac", "qrf_bank_montignac", "site_bank_montignac"));
		state.m_aZones.Insert(NewZoneState("police_saint_pierre", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_POLICE_STATION, "9703 0 1514", 35, 8, "route_police_saint_pierre", "qrf_police_saint_pierre", "site_police_saint_pierre"));
		state.m_aZones.Insert(NewZoneState("police_morton", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_POLICE_STATION, "5005 0 3975", 30, 8, "route_police_morton", "qrf_police_morton", "site_police_morton"));
		state.m_aZones.Insert(NewZoneState("police_lamentin", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_POLICE_STATION, "1225 0 6020", 30, 8, "route_police_lamentin", "qrf_police_lamentin", "site_police_lamentin"));
		ApplyDefaultZoneMetadata(state);
	}

	static void AddDefaultGarrisons(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
				continue;

			int infantryCount = Math.Max(2, zone.m_iGarrisonSlots / 2 + zone.m_iPriority / 8);
			int vehicleCount;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST || zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT || zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
				vehicleCount = 1;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
				vehicleCount = 2;

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

	private static void ApplyDefaultZoneMetadata(HST_CampaignState state)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			zone.m_sDisplayName = GetZoneDisplayName(zone.m_sZoneId);
			zone.m_sResourceKind = GetZoneResourceKind(zone);
			zone.m_iCaptureRadiusMeters = GetZoneCaptureRadius(zone);
			zone.m_iPriority = GetZonePriority(zone);
			zone.m_sCompositionId = "comp_" + zone.m_sZoneId;
			zone.m_sSpawnProfileId = GetZoneSpawnProfile(zone);
			AddDefaultLinks(zone);
		}
	}

	static string GetZoneDisplayName(string zoneId)
	{
		if (zoneId == "town_saint_pierre")
			return "Saint-Pierre";
		if (zoneId == "town_provins")
			return "Provins";
		if (zoneId == "town_entre_deux")
			return "Entre-Deux";
		if (zoneId == "town_chotain")
			return "Chotain";
		if (zoneId == "town_montignac")
			return "Montignac";
		if (zoneId == "town_laruns")
			return "Laruns";
		if (zoneId == "town_levie")
			return "Levie";
		if (zoneId == "town_morton")
			return "Morton";
		if (zoneId == "town_meaux")
			return "Meaux";
		if (zoneId == "town_tyrone")
			return "Tyrone";
		if (zoneId == "town_gravette")
			return "Gravette";
		if (zoneId == "town_villeneuve")
			return "Villeneuve";
		if (zoneId == "town_le_moule")
			return "Le Moule";
		if (zoneId == "town_lamentin")
			return "Lamentin";
		if (zoneId == "town_regina")
			return "Regina";
		if (zoneId == "town_figari")
			return "Figari";
		if (zoneId == "town_durras")
			return "Durras";
		if (zoneId == "town_saint_philippe")
			return "Saint-Philippe";
		if (zoneId == "outpost_north")
			return "North Outpost";
		if (zoneId == "outpost_south")
			return "South Outpost";
		if (zoneId == "airfield_main")
			return "Everon Airfield";
		if (zoneId == "seaport_main")
			return "Everon Seaport";
		if (zoneId == "factory_central")
			return "Central Factory";
		if (zoneId == "resource_north")
			return "North Resource";
		if (zoneId == "resource_south")
			return "South Resource";
		if (zoneId == "radio_north")
			return "North Radio Tower";
		if (zoneId == "radio_south")
			return "South Radio Tower";
		if (zoneId == "outpost_lamentin")
			return "Lamentin Checkpoint";
		if (zoneId == "outpost_morton")
			return "Morton Checkpoint";
		if (zoneId == "outpost_regina")
			return "Regina Checkpoint";
		if (zoneId == "outpost_airfield_west")
			return "Airfield West Outpost";
		if (zoneId == "outpost_montignac_hills")
			return "Montignac Hills Outpost";
		if (zoneId == "outpost_seaport_guard")
			return "Seaport Guard Post";
		if (zoneId == "factory_west")
			return "West Factory";
		if (zoneId == "factory_saint_pierre")
			return "Saint-Pierre Factory";
		if (zoneId == "resource_lumber_lamentin")
			return "Lamentin Lumber";
		if (zoneId == "resource_lumber_villeneuve")
			return "Villeneuve Lumber";
		if (zoneId == "resource_quarry_levie")
			return "Levie Quarry";
		if (zoneId == "resource_fuel_airfield")
			return "Airfield Fuel Depot";
		if (zoneId == "resource_farm_laruns")
			return "Laruns Farms";
		if (zoneId == "resource_farm_provins")
			return "Provins Farms";
		if (zoneId == "resource_mine_montignac")
			return "Montignac Mine";
		if (zoneId == "resource_port_saint_pierre")
			return "Saint-Pierre Port Stores";
		if (zoneId == "resource_depot_central")
			return "Central Supply Depot";
		if (zoneId == "resource_depot_south")
			return "South Supply Depot";
		if (zoneId == "resource_depot_east")
			return "East Supply Depot";
		if (zoneId == "seaport_saint_pierre")
			return "Saint-Pierre Seaport";
		if (zoneId == "radio_central")
			return "Central Radio Tower";
		if (zoneId == "radio_west")
			return "West Radio Tower";
		if (zoneId == "bank_saint_pierre")
			return "Saint-Pierre Bank";
		if (zoneId == "bank_montignac")
			return "Montignac Bank";
		if (zoneId == "police_saint_pierre")
			return "Saint-Pierre Police";
		if (zoneId == "police_morton")
			return "Morton Police";
		if (zoneId == "police_lamentin")
			return "Lamentin Police";

		return zoneId;
	}

	private static string GetZoneResourceKind(HST_ZoneState zone)
	{
		if (!zone)
			return "generic";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && zone.m_sZoneId.Contains("fuel"))
			return "fuel";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && zone.m_sZoneId.Contains("lumber"))
			return "lumber";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && (zone.m_sZoneId.Contains("mine") || zone.m_sZoneId.Contains("quarry")))
			return "materials";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && zone.m_sZoneId.Contains("farm"))
			return "food";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && zone.m_sZoneId.Contains("depot"))
			return "supplies";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			return "industry";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return "air";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return "sea";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_BANK)
			return "money";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_POLICE_STATION)
			return "security";
		return "population";
	}

	private static int GetZoneCaptureRadius(HST_ZoneState zone)
	{
		if (!zone)
			return 180;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return 380;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY || zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
			return 260;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			return 240;
		return 180;
	}

	private static int GetZonePriority(HST_ZoneState zone)
	{
		if (!zone)
			return 1;

		int priority = Math.Max(1, zone.m_iIncomeValue / 10);
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			priority += 8;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			priority += 6;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			priority += 4;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
			priority += 3;
		return priority;
	}

	private static string GetZoneSpawnProfile(HST_ZoneState zone)
	{
		if (!zone)
			return "spawn_generic";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			return "spawn_town_patrols";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			return "spawn_resource_guards";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			return "spawn_factory_garrison";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return "spawn_airfield_garrison";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return "spawn_seaport_garrison";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return "spawn_radio_patrol";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_POLICE_STATION)
			return "spawn_police_patrol";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_BANK)
			return "spawn_bank_security";
		return "spawn_outpost_garrison";
	}

	private static void AddDefaultLinks(HST_ZoneState zone)
	{
		if (!zone || zone.m_aLinkedZoneIds.Count() > 0)
			return;

		if (zone.m_sZoneId.Contains("saint_pierre"))
			AddZoneLinks(zone, "town_saint_pierre", "seaport_saint_pierre", "factory_saint_pierre");
		else if (zone.m_sZoneId.Contains("montignac"))
			AddZoneLinks(zone, "town_montignac", "factory_central", "resource_mine_montignac");
		else if (zone.m_sZoneId.Contains("lamentin"))
			AddZoneLinks(zone, "town_lamentin", "factory_west", "resource_lumber_lamentin");
		else if (zone.m_sZoneId.Contains("morton"))
			AddZoneLinks(zone, "town_morton", "outpost_north", "resource_north");
		else if (zone.m_sZoneId.Contains("airfield"))
			AddZoneLinks(zone, "airfield_main", "resource_fuel_airfield", "outpost_airfield_west");
		else if (zone.m_sZoneId.Contains("seaport"))
			AddZoneLinks(zone, "seaport_main", "resource_depot_south", "outpost_seaport_guard");
		else if (zone.m_sZoneId.Contains("regina"))
			AddZoneLinks(zone, "town_regina", "radio_south", "resource_depot_east");
	}

	private static void AddZoneLinks(HST_ZoneState zone, string first, string second = "", string third = "")
	{
		if (!first.IsEmpty() && first != zone.m_sZoneId)
			zone.m_aLinkedZoneIds.Insert(first);
		if (!second.IsEmpty() && second != zone.m_sZoneId)
			zone.m_aLinkedZoneIds.Insert(second);
		if (!third.IsEmpty() && third != zone.m_sZoneId)
			zone.m_aLinkedZoneIds.Insert(third);
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
