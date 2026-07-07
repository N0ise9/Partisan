class HST_DefaultCatalog
{
	static HST_BalanceConfig CreateBalance()
	{
		HST_BalanceConfig balance = new HST_BalanceConfig();
		EnsureArsenalItemRules(balance);
		EnsureCivilianPools(balance);
		return balance;
	}

	static void EnsureArsenalItemRules(HST_BalanceConfig balance)
	{
		if (!balance || balance.m_aArsenalItemRules.Count() > 0)
			return;

		balance.m_aArsenalItemRules.Insert(NewArsenalItemRule("MissionProp_CitySupplies", "utility", "finite_only", -1));
		balance.m_aArsenalItemRules.Insert(NewArsenalItemRule("MissionProp_ConvoyPayload", "utility", "unlock", 2));
		balance.m_aArsenalItemRules.Insert(NewArsenalItemRule("MissionProp_DestroyTarget", "utility", "blocked", -1));
	}

	static HST_ArsenalItemRule NewArsenalItemRule(string prefabContains, string category, string policy, int thresholdOverride = -1, bool areaLoot = true, bool vehicleLoot = true)
	{
		HST_ArsenalItemRule rule = new HST_ArsenalItemRule();
		rule.m_sPrefabContains = prefabContains;
		rule.m_sCategory = category;
		rule.m_sPolicy = policy;
		rule.m_iUnlockThresholdOverride = thresholdOverride;
		rule.m_bAppliesToAreaLoot = areaLoot;
		rule.m_bAppliesToVehicleLoot = vehicleLoot;
		return rule;
	}

	static void EnsureCivilianPools(HST_BalanceConfig balance)
	{
		if (!balance)
			return;

		if (balance.m_aCivilianCharacterPrefabs.Count() == 0)
		{
			balance.m_aCivilianCharacterPrefabs.Insert("{22E43956740A6794}Prefabs/Characters/Factions/CIV/GenericCivilians/Character_CIV_Randomized.et");
		}

		// Civilian vehicles are resolved from the CIV runtime entity catalog.
	}

	static HST_PrefabPoolEntry NewPrefabPoolEntry(string prefab, int weight, string tag = "")
	{
		HST_PrefabPoolEntry entry = new HST_PrefabPoolEntry();
		entry.m_sPrefab = prefab;
		entry.m_iWeight = Math.Max(1, weight);
		if (!tag.IsEmpty())
			entry.m_aTags.Insert(tag);

		return entry;
	}

	static string SelectWeightedPrefab(array<ref HST_PrefabPoolEntry> pool, int seed)
	{
		if (!pool || pool.Count() == 0)
			return "";

		int totalWeight;
		foreach (HST_PrefabPoolEntry entry : pool)
		{
			if (entry && !entry.m_sPrefab.IsEmpty())
				totalWeight += Math.Max(1, entry.m_iWeight);
		}

		if (totalWeight <= 0)
			return "";

		int roll = PositiveMod(seed, totalWeight);
		foreach (HST_PrefabPoolEntry weightedEntry : pool)
		{
			if (!weightedEntry || weightedEntry.m_sPrefab.IsEmpty())
				continue;

			roll -= Math.Max(1, weightedEntry.m_iWeight);
			if (roll < 0)
				return weightedEntry.m_sPrefab;
		}

		return pool[0].m_sPrefab;
	}

	static int PositiveMod(int value, int divisor)
	{
		if (divisor <= 0)
			return 0;

		int result = value - (value / divisor) * divisor;
		if (result < 0)
			result = result + divisor;

		return result;
	}

	static HST_CampaignPreset CreateVanillaEveronPreset()
	{
		HST_CampaignPreset preset = new HST_CampaignPreset();
		preset.m_sPresetId = "vanilla_everon";
		preset.m_sDisplayName = "h-istasi Everon: FIA vs US vs USSR";
		preset.m_sResistanceFactionKey = "FIA";
		preset.m_sOccupierFactionKey = "US";
		preset.m_sInvaderFactionKey = "USSR";
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
		hideouts.Insert(NewHideout("hideout_north_forest", "North Forest", "6332.167 75.926 8446.294"));
		hideouts.Insert(NewHideout("hideout_central_hills", "Central Hills", "4280.766 14.317 3468.06"));
		hideouts.Insert(NewHideout("hideout_south_woods", "South Woods", "8355.991 237.817 4765.673"));
		return hideouts;
	}

	static HST_FactionTemplate CreateFactionTemplate(string factionKey)
	{
		if (factionKey == "FIA")
			return CreateFiaTemplate();

		if (factionKey == "US")
			return CreateUsTemplate();

		if (factionKey == "USSR")
			return CreateUssrTemplate();

		return null;
	}

	static HST_FactionRuntimeSpawnSpec ResolveRuntimeSpawnSpec(HST_CampaignPreset preset, string factionKey, string role, string sourceContext)
	{
		HST_FactionTemplate faction = CreateFactionTemplate(factionKey);
		if (!faction)
			return null;

		HST_FactionRuntimeSpawnSpec spec = new HST_FactionRuntimeSpawnSpec();
		spec.m_sFactionKey = factionKey;
		spec.m_sExpectedNativeFactionKey = factionKey;
		spec.m_sRole = role;
		spec.m_sSourceContext = sourceContext;
		spec.m_sSourceCatalog = BuildRuntimeSpawnSpecSource(preset, factionKey, role, sourceContext);
		AppendRuntimeSpawnSpecPrefabs(spec.m_aDirectInfantryPrefabs, faction.m_aInfantryPrefabs);
		AppendRuntimeSpawnSpecPrefabs(spec.m_aVehiclePrefabs, faction.m_aVehiclePrefabs);
		if (role == "qrf")
		{
			AppendRuntimeSpawnSpecPrefabs(spec.m_aGroupPrefabs, faction.m_aQRFGroupPrefabs);
			AppendRuntimeSpawnSpecPrefabs(spec.m_aGroupPrefabs, faction.m_aGroupPrefabs);
		}
		else if (role == "patrol")
		{
			AppendRuntimeSpawnSpecPrefabs(spec.m_aGroupPrefabs, faction.m_aPatrolGroupPrefabs);
			AppendRuntimeSpawnSpecPrefabs(spec.m_aGroupPrefabs, faction.m_aGroupPrefabs);
		}
		else if (role == "convoy_crew")
		{
			AppendRuntimeSpawnSpecPrefabs(spec.m_aGroupPrefabs, faction.m_aGroupPrefabs);
			AppendRuntimeSpawnSpecPrefabs(spec.m_aGroupPrefabs, faction.m_aPatrolGroupPrefabs);
		}
		else
		{
			AppendRuntimeSpawnSpecPrefabs(spec.m_aGroupPrefabs, faction.m_aGroupPrefabs);
			AppendRuntimeSpawnSpecPrefabs(spec.m_aGroupPrefabs, faction.m_aPatrolGroupPrefabs);
			AppendRuntimeSpawnSpecPrefabs(spec.m_aGroupPrefabs, faction.m_aQRFGroupPrefabs);
		}

		if (spec.m_aGroupPrefabs.Count() > 0)
			spec.m_sGroupPrefab = spec.m_aGroupPrefabs[0];

		return spec;
	}

	static void AppendRuntimeSpawnSpecPrefabs(array<string> target, array<string> source)
	{
		if (!target || !source)
			return;

		foreach (string prefab : source)
		{
			if (!prefab.IsEmpty() && !target.Contains(prefab))
				target.Insert(prefab);
		}
	}

	static string BuildRuntimeSpawnSpecSource(HST_CampaignPreset preset, string factionKey, string role, string sourceContext)
	{
		string presetId = "default";
		if (preset && !preset.m_sPresetId.IsEmpty())
			presetId = preset.m_sPresetId;

		return string.Format("default catalog %1 | faction %2 | role %3 | source %4", presetId, factionKey, role, sourceContext);
	}

	static void AddDefaultZones(HST_CampaignState state, HST_CampaignPreset preset)
	{
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("airfield_airbase_saint_philippe", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_AIRFIELD, "4915.073 28.594 11855.36", 150, 24, "route_airfield_airbase_saint_philippe", "qrf_airfield_airbase_saint_philippe", "site_airfield_airbase_saint_philippe"), "Airbase Saint-Philippe", "tonka:base:01:MilitaryBaseAirfield", "Bases.layer", "air", 320, 23, "BOSTON", "red", "enemy_base", "comp_airfield_airbase_saint_philippe", "spawn_airfield_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_levie_base", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "7518.644 166.752 4265.595", 50, 16, "route_outpost_levie_base", "qrf_outpost_levie_base", "site_outpost_levie_base"), "Levie Base", "tonka:base:02:MilitaryBaseLevie", "Bases.layer", "security", 240, 8, "PROVIDENCE", "red", "enemy_base", "comp_outpost_levie_base", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_calvary_hill", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "3512.013 181.915 5816.282", 42, 15, "route_outpost_calvary_hill", "qrf_outpost_calvary_hill", "site_outpost_calvary_hill"), "Calvary Hill", "tonka:base:03:MilitaryBaseVilleneuf", "Bases.layer", "security", 240, 7, "NASHVILLE", "red", "enemy_base", "comp_outpost_calvary_hill", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_military_hospital", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "3867.375 14.906 8486.563", 50, 16, "route_outpost_military_hospital", "qrf_outpost_military_hospital", "site_outpost_military_hospital"), "Military Hospital", "tonka:base:04:MilitaryHospital", "Bases.layer", "security", 240, 8, "CHICAGO", "red", "enemy_base", "comp_outpost_military_hospital", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("seaport_coastal_base_chotain", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_SEAPORT, "7462.489 7.867 6700.641", 100, 18, "route_seaport_coastal_base_chotain", "qrf_seaport_coastal_base_chotain", "site_seaport_coastal_base_chotain"), "Coastal Base Chotain", "tonka:base:05:MajorBaseChotain", "Bases.layer", "sea", 320, 10, "MONROE", "red", "enemy_base", "comp_seaport_coastal_base_chotain", "spawn_seaport_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("seaport_coastal_base_lamentin", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_SEAPORT, "1204.166 35.034 5984.738", 100, 18, "route_seaport_coastal_base_lamentin", "qrf_seaport_coastal_base_lamentin", "site_seaport_coastal_base_lamentin"), "Coastal Base Lamentin", "tonka:base:06:MajorBaseLamentin", "Bases.layer", "sea", 320, 10, "HOUSTON", "red", "enemy_base", "comp_seaport_coastal_base_lamentin", "spawn_seaport_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("seaport_coastal_base_morton", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_SEAPORT, "5035.522 15.382 4000.177", 100, 18, "route_seaport_coastal_base_morton", "qrf_seaport_coastal_base_morton", "site_seaport_coastal_base_morton"), "Coastal Base Morton", "tonka:base:07:MajorBaseMorton", "Bases.layer", "sea", 320, 10, "DALLAS", "red", "enemy_base", "comp_seaport_coastal_base_morton", "spawn_seaport_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_north_hq", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "4458.606 8.968 10916.659", 75, 18, "route_outpost_north_hq", "qrf_outpost_north_hq", "site_outpost_north_hq"), "North HQ", "tonka:base:08:MainBaseNorth", "Bases.layer", "security", 285, 11, "PHOENIX", "red", "enemy_base", "comp_outpost_north_hq", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_south_hq", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "10127.863 2.525 1524.779", 58, 17, "route_outpost_south_hq", "qrf_outpost_south_hq", "site_outpost_south_hq"), "South HQ", "tonka:base:09:MainBaseSouth", "Bases.layer", "security", 240, 9, "MEMPHIS", "red", "enemy_base", "comp_outpost_south_hq", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("resource_quarry", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "8819.119 198.157 3909.07", 75, 14, "route_resource_quarry", "qrf_resource_quarry", "site_resource_quarry"), "Quarry", "tonka:base:10:SmallBaseQuarry", "Bases.layer", "materials", 185, 12, "BROOKLYN", "gold", "resource", "comp_resource_quarry", "spawn_resource_guards"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_black_lake", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "9452.499 209.48 2946.152", 42, 15, "route_outpost_black_lake", "qrf_outpost_black_lake", "site_outpost_black_lake"), "Black Lake", "tonka:base:11:SmallBaseBlackLake", "Bases.layer", "security", 240, 7, "OMAHA", "red", "enemy_base", "comp_outpost_black_lake", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_pennants_pass", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "8232.763 223.125 2164.719", 42, 15, "route_outpost_pennants_pass", "qrf_outpost_pennants_pass", "site_outpost_pennants_pass"), "Pennants Pass", "tonka:base:12:SmallBasePennantsPass", "Bases.layer", "security", 240, 7, "CONCORD", "red", "enemy_base", "comp_outpost_pennants_pass", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_simons_wood", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "6311.913 104.423 5226.068", 44, 9, "route_town_simons_wood", "qrf_town_simons_wood", "site_town_simons_wood"), "Simon's Wood", "tonka:base:13:SmallBaseSimonsWood", "Bases.layer", "population", 235, 4, "PRINCETON", "black", "town", "comp_town_simons_wood", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_richemont", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4451.371 64.043 5010.427", 31, 8, "route_town_richemont", "qrf_town_richemont", "site_town_richemont"), "Richemont", "tonka:base:14:SmallBaseMortonValley", "Bases.layer", "population", 220, 3, "DENVER", "black", "town", "comp_town_richemont", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_shepherds_pond", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "3319.675 121.874 4438.134", 58, 17, "route_outpost_shepherds_pond", "qrf_outpost_shepherds_pond", "site_outpost_shepherds_pond"), "Shepherd's Pond", "tonka:base:15:SmallBaseShepherdsPond", "Bases.layer", "security", 240, 9, "TUCSON", "red", "enemy_base", "comp_outpost_shepherds_pond", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_pinewood_lake", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "4509.178 57.291 6231.707", 58, 17, "route_outpost_pinewood_lake", "qrf_outpost_pinewood_lake", "site_outpost_pinewood_lake"), "Pinewood Lake", "tonka:base:16:SmallBasePinewoodLake", "Bases.layer", "security", 240, 9, "SEATTLE", "red", "enemy_base", "comp_outpost_pinewood_lake", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_tillers_find", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "3789.583 59.781 6997.688", 31, 8, "route_town_tillers_find", "qrf_town_tillers_find", "site_town_tillers_find"), "Tiller's Find", "tonka:base:17:SmallBaseTillersFind", "Bases.layer", "population", 220, 3, "TAMPA", "black", "town", "comp_town_tillers_find", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_maidens_bay", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5345.855 43.594 10536.602", 50, 10, "route_town_maidens_bay", "qrf_town_maidens_bay", "site_town_maidens_bay"), "Maiden's Bay", "tonka:base:18:SmallBaseMaidensBay", "Bases.layer", "population", 260, 5, "DAYTON", "black", "town", "comp_town_maidens_bay", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_hornbeam_valley", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5491.385 75.196 7952.735", 31, 8, "route_town_hornbeam_valley", "qrf_town_hornbeam_valley", "site_town_hornbeam_valley"), "Hornbeam Valley", "tonka:base:19:SmallBaseHornbeamValley", "Bases.layer", "population", 220, 3, "AUGUSTA", "black", "town", "comp_town_hornbeam_valley", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("radio_andres_beacon", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "6970.651 86.463 8271.547", 25, 8, "route_radio_andres_beacon", "qrf_radio_andres_beacon", "site_radio_andres_beacon"), "Andre's Beacon", "tonka:base:20:SmallBaseAndresBeacon", "Bases.layer", "communications", 210, 4, "JACKSON", "black", "radio", "comp_radio_andres_beacon", "spawn_radio_patrol"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_goat_bay", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "6635.185 5.902 3119.359", 31, 8, "route_town_goat_bay", "qrf_town_goat_bay", "site_town_goat_bay"), "Goat Bay", "tonka:base:21:SmallBaseGoatbay", "Bases.layer", "population", 220, 3, "RICHMOND", "black", "town", "comp_town_goat_bay", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_chotain", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7084.424 118.776 6017.588", 38, 9, "route_town_chotain", "qrf_town_chotain", "site_town_chotain"), "Chotain", "tonka:base:22:TownBaseChotain", "Bases.layer", "population", 220, 4, "APOLLO", "black", "town", "comp_town_chotain", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_durras", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "8827.942 95.124 2769.363", 31, 8, "route_town_durras", "qrf_town_durras", "site_town_durras"), "Durras", "tonka:base:23:TownBaseDuras", "Bases.layer", "population", 220, 3, "ATLAS", "black", "town", "comp_town_durras", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_entre_deux", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5754.045 217.12 7053.081", 31, 8, "route_town_entre_deux", "qrf_town_entre_deux", "site_town_entre_deux"), "Entre-Deux", "tonka:base:24:TownBaseEntreDeux", "Bases.layer", "population", 220, 3, "TITAN", "black", "town", "comp_town_entre_deux", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_figari", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5281.364 62.97 5361.538", 44, 9, "route_town_figari", "qrf_town_figari", "site_town_figari"), "Figari", "tonka:base:25:TownBaseFigari", "Bases.layer", "population", 235, 4, "STARLIGHT", "black", "town", "comp_town_figari", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_laruns", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7573.111 82.461 5525.154", 38, 9, "route_town_laruns", "qrf_town_laruns", "site_town_laruns"), "Laruns", "tonka:base:26:TownBaseLaruns", "Bases.layer", "population", 220, 4, "COMET", "black", "town", "comp_town_laruns", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_le_moule", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "2570.692 94.634 5423.876", 38, 9, "route_town_le_moule", "qrf_town_le_moule", "site_town_le_moule"), "Le Moule", "tonka:base:27:TownBaseLeMoule", "Bases.layer", "population", 220, 4, "LIBERTY", "black", "town", "comp_town_le_moule", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_levie", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7463.167 137.26 4736.242", 38, 9, "route_town_levie", "qrf_town_levie", "site_town_levie"), "Levie", "tonka:base:28:TownBaseLevie", "Bases.layer", "population", 220, 4, "SPIRIT", "black", "town", "comp_town_levie", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_meaux", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4464.711 13.481 9468.255", 56, 11, "route_town_meaux", "qrf_town_meaux", "site_town_meaux"), "Meaux", "tonka:base:29:TownBaseMeaux", "Bases.layer", "population", 285, 6, "REDHAWK", "black", "town", "comp_town_meaux", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_montignac", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4731.929 162.779 6970.114", 69, 12, "route_town_montignac", "qrf_town_montignac", "site_town_montignac"), "Montignac", "tonka:base:30:TownBaseMontignac", "Bases.layer", "population", 335, 7, "VESPER", "black", "town", "comp_town_montignac", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_provins", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5596.906 96.146 6027.447", 44, 9, "route_town_provins", "qrf_town_provins", "site_town_provins"), "Provins", "tonka:base:31:TownBaseProvins", "Bases.layer", "population", 235, 4, "SUNBURST", "black", "town", "comp_town_provins", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_regina", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7241.147 136.629 2312.1", 56, 11, "route_town_regina", "qrf_town_regina", "site_town_regina"), "Regina", "tonka:base:32:TownBaseRegina", "Bases.layer", "population", 285, 6, "REACTOR", "black", "town", "comp_town_regina", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_tyrone", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4933.47 37.785 9053.985", 31, 8, "route_town_tyrone", "qrf_town_tyrone", "site_town_tyrone"), "Tyrone", "tonka:base:33:TownBaseTyrone", "Bases.layer", "population", 220, 3, "AMBER", "black", "town", "comp_town_tyrone", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_vernon", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "9198.983 64.51 2112.621", 56, 11, "route_town_vernon", "qrf_town_vernon", "site_town_vernon"), "Vernon", "tonka:base:34:TownBaseVernon", "Bases.layer", "population", 285, 6, "JUNKYARD", "black", "town", "comp_town_vernon", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_villeneuve", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "2885.242 85.58 6402.408", 38, 9, "route_town_villeneuve", "qrf_town_villeneuve", "site_town_villeneuve"), "Villeneuve", "tonka:base:35:TownBaseVileneuf", "Bases.layer", "population", 220, 4, "NEW HAMPSHIRE", "black", "town", "comp_town_villeneuve", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_gravette", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4106.324 34.435 7794.186", 38, 9, "route_town_gravette", "qrf_town_gravette", "site_town_gravette"), "Gravette", "tonka:base:36:TownBaseGravette", "Bases.layer", "population", 220, 4, "NEW JERSEY", "black", "town", "comp_town_gravette", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_kermovan", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "6384.869 9.616 9675.053", 44, 9, "route_town_kermovan", "qrf_town_kermovan", "site_town_kermovan"), "Kermovan", "tonka:base:37:TownBaseKermovan", "Bases.layer", "population", 235, 4, "NEW MEXICO", "black", "town", "comp_town_kermovan", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_les_creux", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5244.656 38.613 11354.579", 30, 8, "route_town_les_creux", "qrf_town_les_creux", "site_town_les_creux"), "Les Creux", "tonka:base:38:SmallBaseLesCreux", "Bases.layer", "population", 220, 3, "NORTH CAROLINA", "black", "town", "comp_town_les_creux", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("factory_saint_philippe", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_FACTORY, "4542.439 15.996 10477.004", 110, 14, "route_factory_saint_philippe", "qrf_factory_saint_philippe", "site_factory_saint_philippe"), "St Phillipe Factory", "tonka:base:39:SmallBaseStPhillipeFactory", "Bases.layer", "industry", 240, 17, "NORTH DAKOTA", "gold", "resource", "comp_factory_saint_philippe", "spawn_factory_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_redon", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "2203.623 20.229 7478.579", 30, 8, "route_town_redon", "qrf_town_redon", "site_town_redon"), "Redon", "tonka:base:40:SmallBaseRedon", "Bases.layer", "population", 220, 3, "RHODE ISLAND", "black", "town", "comp_town_redon", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_kervel", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "2482.494 10.406 4232.124", 30, 8, "route_town_kervel", "qrf_town_kervel", "site_town_kervel"), "Kervel", "tonka:base:41:SmallBaseKervel", "Bases.layer", "population", 220, 3, "SOUTH CAROLINA", "black", "town", "comp_town_kervel", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_firing_range", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "7631.984 13.255 8097.089", 43, 15, "route_outpost_firing_range", "qrf_outpost_firing_range", "site_outpost_firing_range"), "Firing Range", "tonka:base:42:SmallBaseFiringRange", "Bases.layer", "security", 240, 7, "SOUTH DAKOTA", "red", "enemy_base", "comp_outpost_firing_range", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_benac", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "6607.746 148.937 7090.993", 30, 8, "route_town_benac", "qrf_town_benac", "site_town_benac"), "Benac", "tonka:base:43:SmallBaseBenac", "Bases.layer", "population", 220, 3, "TENNESSEE", "black", "town", "comp_town_benac", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_scythe", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7463.339 3.435 9524.696", 32, 8, "route_town_scythe", "qrf_town_scythe", "site_town_scythe"), "Scythe", "tonka:base:44:SmallBaseScythe", "Bases.layer", "population", 220, 3, "WEST VIRGINIA", "black", "town", "comp_town_scythe", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_perelle", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "9319.43 7.215 5046.667", 30, 8, "route_town_perelle", "qrf_town_perelle", "site_town_perelle"), "Perelle", "tonka:base:45:SmallBasePerelle", "Bases.layer", "population", 220, 3, "LOS ANGELES", "black", "town", "comp_town_perelle", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_lancre", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "11707.554 7.889 2249.905", 30, 8, "route_town_lancre", "qrf_town_lancre", "site_town_lancre"), "Lancre", "tonka:base:46:SmallBaseLancre", "Bases.layer", "population", 220, 3, "PHILADELPHIA", "black", "town", "comp_town_lancre", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_cave", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "10600.378 188.127 2438.645", 40, 15, "route_outpost_cave", "qrf_outpost_cave", "site_outpost_cave"), "Cave", "tonka:base:47:SmallBaseCave", "Bases.layer", "security", 240, 7, "SAN ANTONIO", "red", "enemy_base", "comp_outpost_cave", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_cheval_gin_lodge", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "9929.094 296.506 2578.768", 33, 14, "route_outpost_cheval_gin_lodge", "qrf_outpost_cheval_gin_lodge", "site_outpost_cheval_gin_lodge"), "Cheval Gin Lodge", "tonka:base:48:SmallBaseChevalGinLodge", "Bases.layer", "security", 240, 6, "SAN DIEGO", "red", "enemy_base", "comp_outpost_cheval_gin_lodge", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_ghost_town", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "11155.366 24.133 1777.733", 30, 8, "route_town_ghost_town", "qrf_town_ghost_town", "site_town_ghost_town"), "Ghost Town", "tonka:base:49:SmallBaseGhostTown", "Bases.layer", "population", 220, 3, "SAN JOSE", "black", "town", "comp_town_ghost_town", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_saint_pierre", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "9716.864 11.392 1526.447", 45, 9, "route_town_saint_pierre", "qrf_town_saint_pierre", "site_town_saint_pierre"), "St Pierre Town", "tonka:base:50:SmallBaseStPierre", "Bases.layer", "population", 240, 4, "AUSTIN", "black", "town", "comp_town_saint_pierre", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_larue_bay", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "9329.601 16.333 317.23", 30, 8, "route_town_larue_bay", "qrf_town_larue_bay", "site_town_larue_bay"), "Larue Bay", "tonka:base:51:SmallBaseLaRoueBay", "Bases.layer", "population", 220, 3, "LAS VEGAS", "black", "town", "comp_town_larue_bay", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_montfort_castle", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "9337.43 197.055 1171.087", 37, 15, "route_outpost_montfort_castle", "qrf_outpost_montfort_castle", "site_outpost_montfort_castle"), "Montfort Castle", "tonka:base:52:SmallBaseMontfortCastle", "Bases.layer", "security", 240, 7, "BALTIMORE", "red", "enemy_base", "comp_outpost_montfort_castle", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_rockweed_cape", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "10355.198 31.18 350.046", 30, 8, "route_town_rockweed_cape", "qrf_town_rockweed_cape", "site_town_rockweed_cape"), "Rockweed Cape", "tonka:base:53:SmallBaseRockweedCape", "Bases.layer", "population", 220, 3, "NEW ORLEANS", "black", "town", "comp_town_rockweed_cape", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_le_bosc", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7896.114 10.121 1456.076", 30, 8, "route_town_le_bosc", "qrf_town_le_bosc", "site_town_le_bosc"), "Le Bosc", "tonka:base:54:SmallBaseLeBosc", "Bases.layer", "population", 220, 3, "PORTLAND", "black", "town", "comp_town_le_bosc", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_thollevast", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "2932.089 7.291 2070.882", 90, 14, "route_town_thollevast", "qrf_town_thollevast", "site_town_thollevast"), "Thollevast", "tonka:base:55:SmallBaseThollevast", "Bases.layer", "population", 435, 9, "FRESNO", "black", "town", "comp_town_thollevast", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_argent", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "10944.771 25.522 8689.265", 56, 11, "route_town_argent", "qrf_town_argent", "site_town_argent"), "Argent", "tonka:base:56:SmallBaseArgent", "Bases.layer", "population", 285, 6, "TULSA", "black", "town", "comp_town_argent", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_erquy", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "10923.335 25.03 11718.213", 38, 9, "route_town_erquy", "qrf_town_erquy", "site_town_erquy"), "Erquy", "tonka:base:57:SmallBaseErquy", "Bases.layer", "population", 220, 4, "", "black", "town", "comp_town_erquy", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("radio_erquy_lighthouse", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "11347.524 62.473 11237.517", 25, 8, "route_radio_erquy_lighthouse", "qrf_radio_erquy_lighthouse", "site_radio_erquy_lighthouse"), "Erquy Lighthouse", "tonka:base:58:SmallBaseErquyLighthouse", "Bases.layer", "communications", 180, 4, "", "black", "radio", "comp_radio_erquy_lighthouse", "spawn_radio_patrol"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("resource_etoupe_gas_station", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "5301.91 20.614 9861.939", 90, 14, "route_resource_etoupe_gas_station", "qrf_resource_etoupe_gas_station", "site_resource_etoupe_gas_station"), "Etoupe Gas Station", "tonka:base:59:SmallBaseEtoupeGasStation", "Bases.layer", "fuel", 180, 13, "", "gold", "resource", "comp_resource_etoupe_gas_station", "spawn_resource_guards"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_smugglers_hole", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "10755.526 35.145 2845.76", 63, 17, "route_outpost_smugglers_hole", "qrf_outpost_smugglers_hole", "site_outpost_smugglers_hole"), "Smugglers Hole", "tonka:base:60:SmallBaseSmugglersHole", "Bases.layer", "security", 250, 9, "", "red", "enemy_base", "comp_outpost_smugglers_hole", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_pins", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "1154.921 42.893 11495.366", 90, 14, "route_town_pins", "qrf_town_pins", "site_town_pins"), "Pins", "tonka:base:61:SmallBasePins", "Bases.layer", "population", 635, 9, "", "black", "town", "comp_town_pins", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_fishermans_cove", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "6053.254 7.852 10159.83", 42, 9, "route_town_fishermans_cove", "qrf_town_fishermans_cove", "site_town_fishermans_cove"), "Fishermans Cove", "tonka:base:62:SmallBaseFishermansCove", "Bases.layer", "population", 230, 4, "", "black", "town", "comp_town_fishermans_cove", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_courbet", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "8422.677 9.357 809.545", 30, 8, "route_town_courbet", "qrf_town_courbet", "site_town_courbet"), "Courbet", "tonka:base:63:SmallBaseCourbet", "Bases.layer", "population", 220, 3, "", "black", "town", "comp_town_courbet", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("factory_power_plant", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_FACTORY, "5848.77 5.245 9654.499", 110, 14, "route_factory_power_plant", "qrf_factory_power_plant", "site_factory_power_plant"), "Power Plant", "tonka:base:64:SmallBasePowerPlant", "Bases.layer", "industry", 240, 17, "", "gold", "resource", "comp_factory_power_plant", "spawn_factory_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_crag_point", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "11667.637 12.049 1446.685", 30, 8, "route_town_crag_point", "qrf_town_crag_point", "site_town_crag_point"), "Crag Point", "tonka:base:65:SmallBaseCragPoint", "Bases.layer", "population", 220, 3, "", "black", "town", "comp_town_crag_point", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("town_skua_point", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "9628.168 16.323 5379.435", 30, 8, "route_town_skua_point", "qrf_town_skua_point", "site_town_skua_point"), "Skua Point", "tonka:base:66:SmallBaseSkuaPoint", "Bases.layer", "population", 220, 3, "", "black", "town", "comp_town_skua_point", "spawn_town_patrols"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("outpost_western_heights", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "7712.864 341.226 3068.306", 50, 16, "route_outpost_western_heights", "qrf_outpost_western_heights", "site_outpost_western_heights"), "Western Heights", "tonka:base:67:SmallBaseWesternHeights", "Bases.layer", "security", 240, 8, "", "red", "enemy_base", "comp_outpost_western_heights", "spawn_outpost_garrison"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("radio_pins_tower", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "1553.824 21.747 11227.977", 25, 8, "route_radio_pins_tower", "qrf_radio_pins_tower", "site_radio_pins_tower"), "Pins Tower", "tonka:base:68:RelayPins", "Bases.layer", "communications", 180, 4, "", "black", "radio", "comp_radio_pins_tower", "spawn_radio_patrol"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("radio_eastern_sea_relay", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "9215.885 27.552 9351.145", 25, 8, "route_radio_eastern_sea_relay", "qrf_radio_eastern_sea_relay", "site_radio_eastern_sea_relay"), "Eastern Sea Relay", "tonka:base:69:RelayEasternSea", "Bases.layer", "communications", 180, 4, "", "black", "radio", "comp_radio_eastern_sea_relay", "spawn_radio_patrol"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("radio_estrapade_tower", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "3402.537 21.509 1621.985", 25, 8, "route_radio_estrapade_tower", "qrf_radio_estrapade_tower", "site_radio_estrapade_tower"), "Estrapade Tower", "tonka:base:70:RelayEstrapade", "Bases.layer", "communications", 180, 4, "", "black", "radio", "comp_radio_estrapade_tower", "spawn_radio_patrol"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("radio_st_phillipe_relay", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "3924.682 31.384 10769.63", 25, 8, "route_radio_st_phillipe_relay", "qrf_radio_st_phillipe_relay", "site_radio_st_phillipe_relay"), "St Phillipe Relay", "tonka:base:71:RelayStPhillipe", "Bases.layer", "communications", 180, 4, "", "black", "radio", "comp_radio_st_phillipe_relay", "spawn_radio_patrol"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("depot_green_valley_supply_depot", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "8504.755 128.823 3303.259", 80, 14, "route_depot_green_valley_supply_depot", "qrf_depot_green_valley_supply_depot", "site_depot_green_valley_supply_depot"), "Green Valley Supply Depot", "tonka:depot:01:GreenValleySupply", "SupplyDepots.layer", "supplies", 180, 12, "", "gold", "depot", "comp_depot_green_valley_supply_depot", "spawn_resource_guards"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("depot_levie_supply_depot", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "6912.457 57.718 4482.109", 80, 14, "route_depot_levie_supply_depot", "qrf_depot_levie_supply_depot", "site_depot_levie_supply_depot"), "Levie Supply Depot", "tonka:depot:02:LevieSupply", "SupplyDepots.layer", "supplies", 180, 12, "", "gold", "depot", "comp_depot_levie_supply_depot", "spawn_resource_guards"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("depot_figari_supply_depot", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "4814.289 53.445 5819.508", 80, 14, "route_depot_figari_supply_depot", "qrf_depot_figari_supply_depot", "site_depot_figari_supply_depot"), "Figari Supply Depot", "tonka:depot:03:FigariSupply", "SupplyDepots.layer", "supplies", 180, 12, "", "gold", "depot", "comp_depot_figari_supply_depot", "spawn_resource_guards"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("depot_farm_supply_depot", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "4995.1 25.473 9943.72", 80, 14, "route_depot_farm_supply_depot", "qrf_depot_farm_supply_depot", "site_depot_farm_supply_depot"), "Farm Supply Depot", "tonka:depot:04:FarmSupply", "SupplyDepots.layer", "supplies", 180, 12, "", "gold", "depot", "comp_depot_farm_supply_depot", "spawn_resource_guards"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("depot_gorey_supply_depot", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "4841.866 112.853 8100.993", 80, 14, "route_depot_gorey_supply_depot", "qrf_depot_gorey_supply_depot", "site_depot_gorey_supply_depot"), "Gorey Supply Depot", "tonka:depot:05:GoreySupply", "SupplyDepots.layer", "supplies", 180, 12, "", "gold", "depot", "comp_depot_gorey_supply_depot", "spawn_resource_guards"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("depot_industrial_supply_depot", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "6458.873 162.267 6496.987", 80, 14, "route_depot_industrial_supply_depot", "qrf_depot_industrial_supply_depot", "site_depot_industrial_supply_depot"), "Industrial Supply Depot", "tonka:depot:06:IndustrialSupply", "SupplyDepots.layer", "supplies", 180, 12, "", "gold", "depot", "comp_depot_industrial_supply_depot", "spawn_resource_guards"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("depot_sawmill_supply_depot", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "3079.382 117.506 5184.799", 80, 14, "route_depot_sawmill_supply_depot", "qrf_depot_sawmill_supply_depot", "site_depot_sawmill_supply_depot"), "Sawmill Supply Depot", "tonka:depot:07:SawmillSupply", "SupplyDepots.layer", "supplies", 180, 12, "", "gold", "depot", "comp_depot_sawmill_supply_depot", "spawn_resource_guards"));
		state.m_aZones.Insert(ConfigureImportedZone(NewZoneState("depot_regina_supply_depot", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "6589.783 7.228 2329.923", 80, 14, "route_depot_regina_supply_depot", "qrf_depot_regina_supply_depot", "site_depot_regina_supply_depot"), "Regina Supply Depot", "tonka:depot:08:ReginaSupply", "SupplyDepots.layer", "supplies", 180, 12, "", "gold", "depot", "comp_depot_regina_supply_depot", "spawn_resource_guards"));
		ApplyEveronLocationPlanOverrides(state, preset);
		ApplyDefaultZoneMetadata(state);
	}

	private static void ApplyEveronLocationPlanOverrides(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return;

		UpsertEveronLocationPlanZone(state, "airfield_airbase_saint_philippe", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_AIRFIELD, "4964.727 28.343 11932.66", 150, 24, "Airbase Saint-Philippe", "MBC_Airport", "bases/base_Airport.layer", "air", 320, 23, "BOSTON", "red", "enemy_base");
		UpsertEveronLocationPlanZone(state, "seaport_coastal_base_chotain", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_SEAPORT, "7461.818 7.814 6729.315", 100, 18, "Coastal Base Chotain", "MBC_Chotain", "bases/base_Barracks.layer", "sea", 320, 10, "MONROE", "red", "enemy_base");
		UpsertEveronLocationPlanZone(state, "seaport_coastal_base_lamentin", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_SEAPORT, "1064.29 5.103 6044.175", 100, 18, "Coastal Base Lamentin", "Port_Lamentin", "ports.layer", "sea", 320, 10, "HOUSTON", "red", "enemy_base");
		UpsertEveronLocationPlanZone(state, "seaport_coastal_base_morton", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_SEAPORT, "4966.436 5.367 3869.599", 100, 18, "Coastal Base Morton", "Port_Morton", "ports.layer", "sea", 320, 10, "DALLAS", "red", "enemy_base");
		UpsertEveronLocationPlanZone(state, "outpost_levie_base", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "7489.05 164.562 4323.926", 50, 16, "Levie Headquarters Base", "MBC_Levie", "bases/base_Levie.layer", "security", 240, 8, "PROVIDENCE", "red", "enemy_base");
		UpsertEveronLocationPlanZone(state, "outpost_military_hospital", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "3884.425 15.219 8463.663", 50, 16, "Field Hospital", "MBC_Hospital", "bases/base_Hospital.layer", "security", 240, 8, "CHICAGO", "red", "enemy_base");
		UpsertEveronLocationPlanZone(state, "outpost_firing_range", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_OUTPOST, "7729.352 13.656 8154.149", 43, 15, "Training Grounds", "MBC_TrainingGrounds", "bases/base_TrainingGrounds.layer", "security", 240, 7, "SOUTH DAKOTA", "red", "enemy_base");
		UpsertEveronLocationPlanZone(state, "resource_logistics_warehouse", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "5347.874 43.617 10542.923", 80, 14, "Logistics Warehouse", "MBC_SaintPhillipe", "bases/base_SaintPhilippe.layer", "supplies", 190, 12, "", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "factory_power_plant", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "5830.083 4.563 9790.85", 90, 14, "Power Plant", "MBC_PowerPlant", "bases/base_PowerPlant.layer", "power", 220, 15, "", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "depot_industrial_supply_depot", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_FACTORY, "6460.508 161.993 6485.497", 110, 14, "Cement Plant", "FC_CementPlant", "factories.layer", "industry", 240, 17, "", "gold", "factory");
		UpsertEveronLocationPlanZone(state, "factory_concrete_plant", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_FACTORY, "5056.617 27.739 10849.919", 110, 14, "Concrete Plant", "FC_ConcretePlant", "factories.layer", "industry", 240, 17, "", "gold", "factory");
		UpsertEveronLocationPlanZone(state, "factory_montignac", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_FACTORY, "4541.109 155.99 6852.202", 110, 14, "Montignac Factory", "FC_MontiW", "factories.layer", "industry", 240, 17, "", "gold", "factory");
		UpsertEveronLocationPlanZone(state, "factory_saint_philippe", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_FACTORY, "4543.267 16.088 10511.486", 110, 14, "VD Almara Factory", "FC_StPhilippeS", "factories.layer", "industry", 240, 17, "NORTH DAKOTA", "gold", "factory");
		UpsertEveronLocationPlanZone(state, "depot_sawmill_supply_depot", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "3080.644 117.548 5212.668", 80, 14, "Le Moule Sawmill", "FC_LeMouleSawmill", "factories.layer", "wood", 180, 12, "", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "depot_regina_supply_depot", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "7306.477 139.031 2518.856", 80, 14, "Regina Sawmill", "FC_ReginaSawmill", "factories.layer", "wood", 180, 12, "", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "resource_quarry", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "8817.028 196.593 3932.52", 75, 14, "Quarry", "FC_Quarry", "factories.layer", "materials", 185, 12, "BROOKLYN", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "resource_chotain_farm", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "6615.087 109.631 5884.541", 65, 12, "Chotain Farm", "FC_ChotainFarm", "factories.layer", "food", 170, 10, "", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "resource_montignac_farm", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "5167.388 176.272 7048.875", 65, 12, "Montignac Farm", "FC_MontiFarm", "factories.layer", "food", 170, 10, "", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "depot_farm_supply_depot", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "4992.117 25.592 9940.01", 65, 12, "Etoupe Farm", "FC_EtoupeFarm", "factories.layer", "food", 170, 10, "", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "resource_saint_pierre_fuel_station", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "9705.849 22.502 1720.1", 55, 10, "Saint-Pierre Fuel Station", "fuel_01", "fuelstations.layer", "fuel", 150, 8, "", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "resource_provins_montignac_fuel_station", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "5342.982 80.641 6313.674", 55, 10, "Provins / Montignac Fuel Station", "fuel_02", "fuelstations.layer", "fuel", 150, 8, "", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "resource_etoupe_gas_station", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "5308.575 21.303 9835.346", 55, 10, "Etoupe / Power Plant Fuel Station", "fuel_03", "fuelstations.layer", "fuel", 150, 8, "", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "resource_lamentin_fuel_station", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "1657.306 71.644 5739.421", 55, 10, "Lamentin Fuel Station", "fuel_04", "fuelstations.layer", "fuel", 150, 8, "", "gold", "resource");
		UpsertEveronLocationPlanZone(state, "resource_saint_philippe_fuel_station", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RESOURCE, "4609.139 18.657 10662.398", 55, 10, "Saint-Philippe Fuel Station", "fuel_05", "fuelstations.layer", "fuel", 150, 8, "", "gold", "resource");

		UpsertEveronLocationPlanZone(state, "town_lamentin", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "1287.587 47.45 6013.407", 38, 8, "Lamentin", "TC_Lamentin", "towns/town_Lamentin.layer", "population", 220, 4, "", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_le_moule", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "2594.323 97.334 5439.671", 38, 9, "Le Moule", "TC_LeMoule", "towns/town_LeMoule.layer", "population", 220, 4, "LIBERTY", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_montignac", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4770.784 174.088 7036.788", 69, 12, "Montignac", "TC_Montignac", "towns/town_Montignac.layer", "population", 335, 7, "VESPER", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_saint_philippe", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4433.775 17.818 10737.896", 45, 9, "Saint-Philippe", "TC_StPhilippe", "towns/town_StPhilippe.layer", "population", 240, 4, "", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_saint_pierre", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "9759.036 11.81 1539.799", 45, 9, "Saint-Pierre", "TC_StPierre", "towns/town_StPierre.layer", "population", 240, 4, "AUSTIN", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_morton", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5082.813 14.852 3954.27", 38, 8, "Morton", "TC_Morton", "towns/town_Morton.layer", "population", 220, 4, "", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_chotain", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7045.688 118.217 6010.328", 38, 9, "Chotain", "TC_Chotain", "towns/town_Chotain.layer", "population", 220, 4, "APOLLO", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_levie", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7482.018 138.035 4725.215", 38, 9, "Levie", "TC_Levie", "towns/town_Levie.layer", "population", 220, 4, "SPIRIT", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_regina", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7198.936 137.438 2336.226", 56, 11, "Regina", "TC_Regina", "towns/town_Regina.layer", "population", 285, 6, "REACTOR", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_gravette", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4091.403 36.236 7788.301", 38, 9, "Gravette", "TC_Gravette", "towns/town_Gravette.layer", "population", 220, 4, "NEW JERSEY", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_meaux", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4490.289 14.235 9502.132", 56, 11, "Meaux", "TC_Meaux", "towns/town_Meaux.layer", "population", 285, 6, "REDHAWK", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_provins", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5546.651 93.654 6003.296", 44, 9, "Provins", "TC_Provins", "towns/town_Provins.layer", "population", 235, 4, "SUNBURST", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_figari", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5239.172 62.582 5356.463", 44, 9, "Figari", "TC_Figari", "towns/town_Figari.layer", "population", 235, 4, "STARLIGHT", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_durras", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "8820.34 94.708 2755.229", 31, 8, "Durras", "TC_Durras", "towns/town_Durras.layer", "population", 220, 3, "ATLAS", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_entre_deux", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "5754.625 217.905 7068.624", 31, 8, "Entre-Deux", "TC_EntreDeux", "towns/town_EntreDeux.layer", "population", 220, 3, "TITAN", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_laruns", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "7565.37 82.81 5500.853", 38, 9, "Laruns", "TC_Laruns", "towns/town_Laruns.layer", "population", 220, 4, "COMET", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_tyrone", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "4928.974 37.441 9061.847", 31, 8, "Tyrone", "TC_Tyrone", "towns/town_Tyrone.layer", "population", 220, 3, "AMBER", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_vernon", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "9263.577 58.307 2059.466", 56, 11, "Vernon", "TC_Vernon", "towns/town_Vernon.layer", "population", 285, 6, "JUNKYARD", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_villeneuve", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "2857.967 86.331 6358.688", 38, 9, "Villeneuve", "TC_Villeneuf", "towns/town_Villeneuf.layer", "population", 220, 4, "NEW HAMPSHIRE", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_kermovan", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_TOWN, "6360.83 5.679 9638.167", 44, 9, "Kermovan", "TC_Kermovan", "towns/town_Kermovan.layer", "population", 235, 4, "NEW MEXICO", "black", "town");
		UpsertEveronLocationPlanZone(state, "town_erquy", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_TOWN, "10911.79 24.79 11731.3", 38, 9, "Erquy", "TC_Erquy", "towns/town_Erquy.layer", "population", 220, 4, "", "black", "town");

		UpsertEveronLocationPlanZone(state, "radio_lamentin_tower", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "1513.3 61.198 5823.788", 25, 8, "Lamentin Radio Tower", "RSC_Lamentin", "POIs/RadioTowers/Lamentin.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_le_moule_tower", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "1933.808 81.757 5137.221", 25, 8, "Le Moule Radio Tower", "RSC_LeMoule", "POIs/RadioTowers/LeMoule.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_st_phillipe_relay", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "4920.975 42.607 10415.15", 25, 8, "Saint-Philippe Radio Tower", "RSC_StPhilippe", "POIs/RadioTowers/StPhilippe.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_saint_pierre_tower", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "9948.328 316.222 2265.166", 25, 8, "Saint-Pierre Radio Tower", "RSC_StPierre", "POIs/RadioTowers/StPierre.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_saules_tower", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "3078.509 22.78 1862.44", 25, 8, "Saules Radio Tower", "RSC_Saules", "POIs/RadioTowers/Saules.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_argent_tower", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "10941.133 26.198 8682.443", 25, 8, "Argent Radio Tower", "RSC_Argent", "POIs/RadioTowers/Argent.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_margarets_mount", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "9406.599 317.376 3891.71", 25, 8, "Margaret's Mount Radio Tower", "RSC_MargaretsMount", "POIs/RadioTowers/MargaretsMount.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_erquy_lighthouse", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "11419.509 63.866 11284.409", 25, 8, "Erquy Island Radio Tower", "RTC_ErquyIsland", "POIs/RadioTowers/ErquyIsland.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_entre_deux_tower", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "5859.855 236.026 7258.213", 25, 8, "Entre-Deux Radio Tower", "RSC_EntreDeux", "POIs/RadioTowers/EntreDeux.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_wolf_hill_tower", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "3845.418 162.777 4308.285", 25, 8, "Wolf Hill Radio Tower", "RSC_WolfHill", "POIs/RadioTowers/WolfHill.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_pins_tower", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "1166.602 40.918 11695.426", 25, 8, "Pins Radio Tower", "RSC_Pins", "POIs/RadioTowers/Pins.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_regina_tower", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "7070.868 156.803 2080.595", 25, 8, "Regina Radio Tower", "RSC_Regina", "POIs/RadioTowers/Regina.layer", "communications", 180, 4, "", "black", "radio");
		UpsertEveronLocationPlanZone(state, "radio_regina2_tower", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_RADIO_TOWER, "7685.4 340.531 3065.327", 25, 8, "Regina2 Radio Tower", "RSC_Regina2", "POIs/RadioTowers/Regina.layer", "communications", 180, 4, "", "black", "radio");

		UpsertEveronLocationPlanZone(state, "mission_radar_airport", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_MISSION_SITE, "4417.224 17.935 11717.371", 0, 0, "Airport Radar Site", "RDR_Airport", "POIs/RadarSites/Airport.layer", "radar", 120, 2, "", "black", "mission_site");
		UpsertEveronLocationPlanZone(state, "outpost_calvary_hill", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_MISSION_SITE, "3497.976 202.983 5817.62", 0, 0, "Calvary Hill Radar Site", "RDR_CalvaryHill", "POIs/RadarSites/CalvaryHill.layer", "radar", 120, 2, "", "black", "mission_site");
		UpsertEveronLocationPlanZone(state, "mission_radar_saules", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_MISSION_SITE, "2855.396 6.706 2363.671", 0, 0, "Saules Radar Site", "RDR_Saules", "POIs/RadarSites/Saules.layer", "radar", 120, 2, "", "black", "mission_site");
		UpsertEveronLocationPlanZone(state, "mission_radar_argent", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_MISSION_SITE, "10894.839 18.222 8818.74", 0, 0, "Argent Radar Site", "RDR_Argent", "POIs/RadarSites/Argent.layer", "radar", 120, 2, "", "black", "mission_site");
		UpsertEveronLocationPlanZone(state, "mission_radar_erquy_island", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_MISSION_SITE, "10454.677 14.339 12041.855", 0, 0, "Erquy Island Radar Site", "RDR_ErquyIsland", "POIs/RadarSites/ErquyIsland.layer", "radar", 120, 2, "", "black", "mission_site");
		UpsertEveronLocationPlanZone(state, "mission_radar_corsairs_cove", preset.m_sInvaderFactionKey, HST_EZoneType.HST_ZONE_MISSION_SITE, "11619.707 14.698 1484.086", 0, 0, "Corsair's Cove Radar Site", "RDR_CorsairsCove", "POIs/RadarSites/CorsairsCove.layer", "radar", 120, 2, "", "black", "mission_site");
		UpsertEveronLocationPlanZone(state, "mission_radar_pins", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_MISSION_SITE, "1410.476 39.425 11424.309", 0, 0, "Pins Radar Site", "RDR_Pins", "POIs/RadarSites/Pins.layer", "radar", 120, 2, "", "black", "mission_site");
		UpsertEveronLocationPlanZone(state, "mission_radar_andres_beacon", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_MISSION_SITE, "6437.238 28.27 8729.806", 0, 0, "Andre's Beacon Radar Site", "RDR_AndresBeacon", "POIs/RadarSites/AndresBeacon.layer", "radar", 120, 2, "", "black", "mission_site");
		UpsertEveronLocationPlanZone(state, "mission_radar_headquarters", preset.m_sOccupierFactionKey, HST_EZoneType.HST_ZONE_MISSION_SITE, "7291.745 144.579 4357.321", 0, 0, "Headquarters Radar Site", "RDR_Headquarters", "POIs/RadarSites/Headquarters.layer", "radar", 120, 2, "", "black", "mission_site");
	}

	private static void UpsertEveronLocationPlanZone(HST_CampaignState state, string zoneId, string ownerFactionKey, HST_EZoneType zoneType, vector position, int incomeValue, int garrisonSlots, string displayName, string sourceLayoutId, string sourceLayerName, string resourceKind, int captureRadius, int priority, string markerCallsign, string markerTextColor, string markerStyle)
	{
		HST_ZoneState zone = FindDefaultCatalogZone(state, zoneId);
		if (!zone)
		{
			zone = NewZoneState(zoneId, ownerFactionKey, zoneType, position, incomeValue, garrisonSlots, "route_" + zoneId, "qrf_" + zoneId, "site_" + zoneId);
			state.m_aZones.Insert(zone);
		}

		zone.m_sOwnerFactionKey = ownerFactionKey;
		zone.m_eType = zoneType;
		zone.m_vPosition = position;
		zone.m_iIncomeValue = incomeValue;
		zone.m_iGarrisonSlots = garrisonSlots;
		ConfigureImportedZone(zone, displayName, sourceLayoutId, sourceLayerName, resourceKind, captureRadius, priority, markerCallsign, markerTextColor, markerStyle, "", GetEveronLocationPlanSpawnProfile(zoneType));
	}

	private static string GetEveronLocationPlanSpawnProfile(HST_EZoneType zoneType)
	{
		if (zoneType == HST_EZoneType.HST_ZONE_MISSION_SITE)
			return "spawn_none";
		return "";
	}

	private static HST_ZoneState FindDefaultCatalogZone(HST_CampaignState state, string zoneId)
	{
		if (!state || zoneId.IsEmpty())
			return null;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_sZoneId == zoneId)
				return zone;
		}

		return null;
	}

	static void AddDefaultGarrisons(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;
			if (zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
				continue;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
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

	static vector GetEmergencySpawnPosition()
	{
		return "4280.766 14.317 3468.06";
	}

static array<ref HST_MissionDefinition> CreateMissionRegistry()
	{
		array<ref HST_MissionDefinition> missions = {};
		missions.Insert(NewMission("assassinate_officer", "Assassinate Officer", HST_EMissionCategory.HST_MISSION_ASSASSINATION, 2700, 400, 2));
		missions.Insert(NewMission("assassinate_traitor", "Assassinate Traitor", HST_EMissionCategory.HST_MISSION_ASSASSINATION, 2700, 350, 2));
		missions.Insert(NewMission("assassinate_specops", "Assassinate Spec Ops", HST_EMissionCategory.HST_MISSION_ASSASSINATION, 3600, 650, 3));
		missions.Insert(NewMission("conquest_town", "Liberate Town", HST_EMissionCategory.HST_MISSION_CONQUEST, 5400, 650, 4));
		missions.Insert(NewMission("conquest_resource", "Capture Resource", HST_EMissionCategory.HST_MISSION_CONQUEST, 5400, 700, 5));
		missions.Insert(NewMission("conquest_factory", "Capture Factory", HST_EMissionCategory.HST_MISSION_CONQUEST, 5400, 850, 5));
		missions.Insert(NewMission("conquest_outpost", "Capture Outpost", HST_EMissionCategory.HST_MISSION_CONQUEST, 5400, 900, 6));
		missions.Insert(NewMission("conquest_airfield", "Capture Airfield", HST_EMissionCategory.HST_MISSION_CONQUEST, 6600, 1200, 7));
		missions.Insert(NewMission("conquest_seaport", "Capture Seaport", HST_EMissionCategory.HST_MISSION_CONQUEST, 6000, 1050, 6));
		missions.Insert(NewMission("convoy_ammo", "Intercept Ammo Convoy", HST_EMissionCategory.HST_MISSION_CONVOY, 3600, 500, 3));
		missions.Insert(NewMission("convoy_armored", "Intercept Armored Convoy", HST_EMissionCategory.HST_MISSION_CONVOY, 4200, 800, 4));
		missions.Insert(NewMission("convoy_money", "Intercept Money Convoy", HST_EMissionCategory.HST_MISSION_CONVOY, 3600, 700, 3));
		missions.Insert(NewMission("convoy_prisoners", "Intercept Prisoner Convoy", HST_EMissionCategory.HST_MISSION_CONVOY, 3600, 450, 3));
		missions.Insert(NewMission("convoy_reinforcements", "Intercept Reinforcements", HST_EMissionCategory.HST_MISSION_CONVOY, 3600, 550, 4));
		missions.Insert(NewMission("convoy_supplies", "Intercept Supply Convoy", HST_EMissionCategory.HST_MISSION_CONVOY, 3600, 500, 3));
		missions.Insert(NewMission("destroy_radio_tower", "Destroy Radio Tower", HST_EMissionCategory.HST_MISSION_DESTROY, 3600, 450, 3));
		missions.Insert(NewMission("destroy_downed_helicopter", "Destroy Downed Helicopter", HST_EMissionCategory.HST_MISSION_DESTROY, 3000, 500, 3));
		missions.Insert(NewMission("destroy_outpost_cache", "Destroy Outpost Cache", HST_EMissionCategory.HST_MISSION_DESTROY, 3600, 550, 3));
		missions.Insert(NewMission("destroy_factory_asset", "Sabotage Factory Asset", HST_EMissionCategory.HST_MISSION_DESTROY, 3600, 600, 3));
		missions.Insert(NewMission("destroy_airfield_asset", "Sabotage Airfield Asset", HST_EMissionCategory.HST_MISSION_DESTROY, 4200, 750, 4));
		missions.Insert(NewMission("destroy_seaport_asset", "Sabotage Seaport Asset", HST_EMissionCategory.HST_MISSION_DESTROY, 4200, 700, 4));
		missions.Insert(NewMission("destroy_or_steal_armor", "Steal or Destroy Armor", HST_EMissionCategory.HST_MISSION_DESTROY, 4200, 700, 4));
		missions.Insert(NewMission("logistics_bank", "Rob Bank", HST_EMissionCategory.HST_MISSION_LOGISTICS, 2400, 550, 2));
		missions.Insert(NewMission("logistics_resource_cache", "Recover Resource Cache", HST_EMissionCategory.HST_MISSION_LOGISTICS, 3000, 500, 2));
		missions.Insert(NewMission("logistics_factory_supplies", "Recover Factory Supplies", HST_EMissionCategory.HST_MISSION_LOGISTICS, 3300, 550, 2));
		missions.Insert(NewMission("logistics_airfield_intel", "Steal Airfield Intel", HST_EMissionCategory.HST_MISSION_LOGISTICS, 3000, 700, 3));
		missions.Insert(NewMission("logistics_seaport_supplies", "Recover Seaport Supplies", HST_EMissionCategory.HST_MISSION_LOGISTICS, 3300, 650, 3));
		missions.Insert(NewMission("logistics_support_cache", "Recover Support Cache", HST_EMissionCategory.HST_MISSION_LOGISTICS, 2700, 400, 1));
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

	private static HST_ZoneState ConfigureImportedZone(HST_ZoneState zone, string displayName, string sourceLayoutId, string sourceLayerName, string resourceKind, int captureRadius, int priority, string markerCallsign, string markerTextColor, string markerStyle, string compositionId, string spawnProfileId)
	{
		zone.m_sDisplayName = displayName;
		zone.m_sSourceLayoutId = sourceLayoutId;
		zone.m_sSourceLayerName = sourceLayerName;
		zone.m_sResourceKind = resourceKind;
		zone.m_iCaptureRadiusMeters = captureRadius;
		zone.m_iPriority = priority;
		zone.m_sMarkerCallsign = markerCallsign;
		zone.m_sMarkerTextColor = markerTextColor;
		zone.m_sMarkerStyle = markerStyle;
		zone.m_sCompositionId = compositionId;
		zone.m_sSpawnProfileId = spawnProfileId;
		return zone;
	}

	private static void ApplyDefaultZoneMetadata(HST_CampaignState state)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			if (zone.m_sDisplayName.IsEmpty())
				zone.m_sDisplayName = GetZoneDisplayName(zone.m_sZoneId);
			if (zone.m_sResourceKind.IsEmpty())
				zone.m_sResourceKind = GetZoneResourceKind(zone);
			if (zone.m_iCaptureRadiusMeters <= 0)
				zone.m_iCaptureRadiusMeters = GetZoneCaptureRadius(zone);
			if (zone.m_iPriority <= 0)
				zone.m_iPriority = GetZonePriority(zone);
			if (zone.m_sCompositionId.IsEmpty())
				zone.m_sCompositionId = "comp_" + zone.m_sZoneId;
			if (zone.m_sSpawnProfileId.IsEmpty())
				zone.m_sSpawnProfileId = GetZoneSpawnProfile(zone);
			AddDefaultLinks(zone);
		}
	}

	static string GetZoneDisplayName(string zoneId)
	{
		if (zoneId == "airfield_airbase_saint_philippe")
			return "Airbase Saint-Philippe";
		if (zoneId == "outpost_levie_base")
			return "Levie Base";
		if (zoneId == "outpost_calvary_hill")
			return "Calvary Hill";
		if (zoneId == "outpost_military_hospital")
			return "Military Hospital";
		if (zoneId == "seaport_coastal_base_chotain")
			return "Coastal Base Chotain";
		if (zoneId == "seaport_coastal_base_lamentin")
			return "Coastal Base Lamentin";
		if (zoneId == "seaport_coastal_base_morton")
			return "Coastal Base Morton";
		if (zoneId == "outpost_north_hq")
			return "North HQ";
		if (zoneId == "outpost_south_hq")
			return "South HQ";
		if (zoneId == "resource_quarry")
			return "Quarry";
		if (zoneId == "outpost_black_lake")
			return "Black Lake";
		if (zoneId == "outpost_pennants_pass")
			return "Pennants Pass";
		if (zoneId == "town_simons_wood")
			return "Simon's Wood";
		if (zoneId == "town_richemont")
			return "Richemont";
		if (zoneId == "outpost_shepherds_pond")
			return "Shepherd's Pond";
		if (zoneId == "outpost_pinewood_lake")
			return "Pinewood Lake";
		if (zoneId == "town_tillers_find")
			return "Tiller's Find";
		if (zoneId == "town_maidens_bay")
			return "Maiden's Bay";
		if (zoneId == "town_hornbeam_valley")
			return "Hornbeam Valley";
		if (zoneId == "radio_andres_beacon")
			return "Andre's Beacon";
		if (zoneId == "town_goat_bay")
			return "Goat Bay";
		if (zoneId == "town_chotain")
			return "Chotain";
		if (zoneId == "town_durras")
			return "Durras";
		if (zoneId == "town_entre_deux")
			return "Entre-Deux";
		if (zoneId == "town_figari")
			return "Figari";
		if (zoneId == "town_laruns")
			return "Laruns";
		if (zoneId == "town_le_moule")
			return "Le Moule";
		if (zoneId == "town_levie")
			return "Levie";
		if (zoneId == "town_meaux")
			return "Meaux";
		if (zoneId == "town_montignac")
			return "Montignac";
		if (zoneId == "town_provins")
			return "Provins";
		if (zoneId == "town_regina")
			return "Regina";
		if (zoneId == "town_tyrone")
			return "Tyrone";
		if (zoneId == "town_vernon")
			return "Vernon";
		if (zoneId == "town_villeneuve")
			return "Villeneuve";
		if (zoneId == "town_gravette")
			return "Gravette";
		if (zoneId == "town_kermovan")
			return "Kermovan";
		if (zoneId == "town_les_creux")
			return "Les Creux";
		if (zoneId == "factory_saint_philippe")
			return "St Phillipe Factory";
		if (zoneId == "town_redon")
			return "Redon";
		if (zoneId == "town_kervel")
			return "Kervel";
		if (zoneId == "outpost_firing_range")
			return "Firing Range";
		if (zoneId == "town_benac")
			return "Benac";
		if (zoneId == "town_scythe")
			return "Scythe";
		if (zoneId == "town_perelle")
			return "Perelle";
		if (zoneId == "town_lancre")
			return "Lancre";
		if (zoneId == "outpost_cave")
			return "Cave";
		if (zoneId == "outpost_cheval_gin_lodge")
			return "Cheval Gin Lodge";
		if (zoneId == "town_ghost_town")
			return "Ghost Town";
		if (zoneId == "town_saint_pierre")
			return "St Pierre Town";
		if (zoneId == "town_larue_bay")
			return "Larue Bay";
		if (zoneId == "outpost_montfort_castle")
			return "Montfort Castle";
		if (zoneId == "town_rockweed_cape")
			return "Rockweed Cape";
		if (zoneId == "town_le_bosc")
			return "Le Bosc";
		if (zoneId == "town_thollevast")
			return "Thollevast";
		if (zoneId == "town_argent")
			return "Argent";
		if (zoneId == "town_erquy")
			return "Erquy";
		if (zoneId == "radio_erquy_lighthouse")
			return "Erquy Lighthouse";
		if (zoneId == "resource_etoupe_gas_station")
			return "Etoupe Gas Station";
		if (zoneId == "outpost_smugglers_hole")
			return "Smugglers Hole";
		if (zoneId == "town_pins")
			return "Pins";
		if (zoneId == "town_fishermans_cove")
			return "Fishermans Cove";
		if (zoneId == "town_courbet")
			return "Courbet";
		if (zoneId == "factory_power_plant")
			return "Power Plant";
		if (zoneId == "town_crag_point")
			return "Crag Point";
		if (zoneId == "town_skua_point")
			return "Skua Point";
		if (zoneId == "outpost_western_heights")
			return "Western Heights";
		if (zoneId == "radio_pins_tower")
			return "Pins Tower";
		if (zoneId == "radio_eastern_sea_relay")
			return "Eastern Sea Relay";
		if (zoneId == "radio_estrapade_tower")
			return "Estrapade Tower";
		if (zoneId == "radio_st_phillipe_relay")
			return "St Phillipe Relay";
		if (zoneId == "depot_green_valley_supply_depot")
			return "Green Valley Supply Depot";
		if (zoneId == "depot_levie_supply_depot")
			return "Levie Supply Depot";
		if (zoneId == "depot_figari_supply_depot")
			return "Figari Supply Depot";
		if (zoneId == "depot_farm_supply_depot")
			return "Farm Supply Depot";
		if (zoneId == "depot_gorey_supply_depot")
			return "Gorey Supply Depot";
		if (zoneId == "depot_industrial_supply_depot")
			return "Industrial Supply Depot";
		if (zoneId == "depot_sawmill_supply_depot")
			return "Sawmill Supply Depot";
		if (zoneId == "depot_regina_supply_depot")
			return "Regina Supply Depot";

		return zoneId;
	}

	private static string GetZoneResourceKind(HST_ZoneState zone)
	{
		if (!zone)
			return "generic";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && zone.m_sZoneId.Contains("fuel"))
			return "fuel";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && (zone.m_sZoneId.Contains("quarry") || zone.m_sZoneId.Contains("mine")))
			return "materials";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			return "supplies";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			return "industry";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return "air";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return "sea";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return "communications";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
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
		return "spawn_outpost_garrison";
	}

	private static void AddDefaultLinks(HST_ZoneState zone)
	{
		if (!zone || zone.m_aLinkedZoneIds.Count() > 0)
			return;

		if (zone.m_sZoneId == "airfield_airbase_saint_philippe")
			AddZoneLinks(zone, "town_les_creux", "outpost_north_hq", "town_maidens_bay");
		if (zone.m_sZoneId == "outpost_levie_base")
			AddZoneLinks(zone, "town_levie", "depot_levie_supply_depot", "outpost_western_heights");
		if (zone.m_sZoneId == "outpost_calvary_hill")
			AddZoneLinks(zone, "depot_sawmill_supply_depot", "town_villeneuve", "town_le_moule");
		if (zone.m_sZoneId == "outpost_military_hospital")
			AddZoneLinks(zone, "town_gravette", "depot_gorey_supply_depot", "town_meaux");
		if (zone.m_sZoneId == "seaport_coastal_base_chotain")
			AddZoneLinks(zone, "town_chotain", "town_benac", "depot_industrial_supply_depot");
		if (zone.m_sZoneId == "seaport_coastal_base_lamentin")
			AddZoneLinks(zone, "town_le_moule", "town_villeneuve", "town_redon");
		if (zone.m_sZoneId == "seaport_coastal_base_morton")
			AddZoneLinks(zone, "town_richemont", "town_figari", "town_simons_wood");
		if (zone.m_sZoneId == "outpost_north_hq")
			AddZoneLinks(zone, "factory_saint_philippe", "radio_st_phillipe_relay", "town_les_creux");
		if (zone.m_sZoneId == "outpost_south_hq")
			AddZoneLinks(zone, "town_saint_pierre", "outpost_montfort_castle", "outpost_cave");
		if (zone.m_sZoneId == "resource_quarry")
			AddZoneLinks(zone, "depot_green_valley_supply_depot", "town_durras", "outpost_black_lake");
		if (zone.m_sZoneId == "outpost_black_lake")
			AddZoneLinks(zone, "outpost_cheval_gin_lodge", "town_durras", "town_vernon");
		if (zone.m_sZoneId == "outpost_pennants_pass")
			AddZoneLinks(zone, "town_le_bosc", "town_durras", "town_vernon");
		if (zone.m_sZoneId == "town_simons_wood")
			AddZoneLinks(zone, "depot_levie_supply_depot", "town_figari", "town_provins");
		if (zone.m_sZoneId == "town_richemont")
			AddZoneLinks(zone, "depot_figari_supply_depot", "town_figari", "seaport_coastal_base_morton");
		if (zone.m_sZoneId == "outpost_shepherds_pond")
			AddZoneLinks(zone, "depot_sawmill_supply_depot", "town_kervel", "town_le_moule");
		if (zone.m_sZoneId == "outpost_pinewood_lake")
			AddZoneLinks(zone, "depot_figari_supply_depot", "town_montignac", "town_tillers_find");
		if (zone.m_sZoneId == "town_tillers_find")
			AddZoneLinks(zone, "town_gravette", "town_montignac", "outpost_pinewood_lake");
		if (zone.m_sZoneId == "town_maidens_bay")
			AddZoneLinks(zone, "resource_etoupe_gas_station", "depot_farm_supply_depot", "town_fishermans_cove");
		if (zone.m_sZoneId == "town_hornbeam_valley")
			AddZoneLinks(zone, "depot_gorey_supply_depot", "town_entre_deux", "town_tyrone");
		if (zone.m_sZoneId == "radio_andres_beacon")
			AddZoneLinks(zone, "outpost_firing_range", "town_benac", "town_scythe");
		if (zone.m_sZoneId == "town_goat_bay")
			AddZoneLinks(zone, "depot_regina_supply_depot", "town_regina", "outpost_western_heights");
		if (zone.m_sZoneId == "town_chotain")
			AddZoneLinks(zone, "town_laruns", "seaport_coastal_base_chotain", "depot_industrial_supply_depot");
		if (zone.m_sZoneId == "town_durras")
			AddZoneLinks(zone, "depot_green_valley_supply_depot", "outpost_black_lake", "town_vernon");
		if (zone.m_sZoneId == "town_entre_deux")
			AddZoneLinks(zone, "town_benac", "depot_industrial_supply_depot", "town_hornbeam_valley");
		if (zone.m_sZoneId == "town_figari")
			AddZoneLinks(zone, "depot_figari_supply_depot", "town_provins", "town_richemont");
		if (zone.m_sZoneId == "town_laruns")
			AddZoneLinks(zone, "town_chotain", "town_levie", "seaport_coastal_base_chotain");
		if (zone.m_sZoneId == "town_le_moule")
			AddZoneLinks(zone, "depot_sawmill_supply_depot", "outpost_calvary_hill", "town_villeneuve");
		if (zone.m_sZoneId == "town_levie")
			AddZoneLinks(zone, "outpost_levie_base", "depot_levie_supply_depot", "town_laruns");
		if (zone.m_sZoneId == "town_meaux")
			AddZoneLinks(zone, "town_tyrone", "depot_farm_supply_depot", "resource_etoupe_gas_station");
		if (zone.m_sZoneId == "town_montignac")
			AddZoneLinks(zone, "outpost_pinewood_lake", "town_tillers_find", "town_entre_deux");
		if (zone.m_sZoneId == "town_provins")
			AddZoneLinks(zone, "town_figari", "depot_figari_supply_depot", "depot_industrial_supply_depot");
		if (zone.m_sZoneId == "town_regina")
			AddZoneLinks(zone, "depot_regina_supply_depot", "outpost_western_heights", "outpost_pennants_pass");
		if (zone.m_sZoneId == "town_tyrone")
			AddZoneLinks(zone, "town_meaux", "resource_etoupe_gas_station", "depot_farm_supply_depot");
		if (zone.m_sZoneId == "town_vernon")
			AddZoneLinks(zone, "town_durras", "town_saint_pierre", "outpost_cheval_gin_lodge");
		if (zone.m_sZoneId == "town_villeneuve")
			AddZoneLinks(zone, "outpost_calvary_hill", "town_le_moule", "town_tillers_find");
		if (zone.m_sZoneId == "town_gravette")
			AddZoneLinks(zone, "outpost_military_hospital", "depot_gorey_supply_depot", "town_tillers_find");
		if (zone.m_sZoneId == "town_kermovan")
			AddZoneLinks(zone, "factory_power_plant", "town_fishermans_cove", "town_scythe");
		if (zone.m_sZoneId == "town_les_creux")
			AddZoneLinks(zone, "airfield_airbase_saint_philippe", "town_maidens_bay", "outpost_north_hq");
		if (zone.m_sZoneId == "factory_saint_philippe")
			AddZoneLinks(zone, "outpost_north_hq", "radio_st_phillipe_relay", "depot_farm_supply_depot");
		if (zone.m_sZoneId == "town_redon")
			AddZoneLinks(zone, "town_villeneuve", "town_tillers_find", "seaport_coastal_base_lamentin");
		if (zone.m_sZoneId == "town_kervel")
			AddZoneLinks(zone, "outpost_shepherds_pond", "depot_sawmill_supply_depot", "town_le_moule");
		if (zone.m_sZoneId == "outpost_firing_range")
			AddZoneLinks(zone, "radio_andres_beacon", "seaport_coastal_base_chotain", "town_benac");
		if (zone.m_sZoneId == "town_benac")
			AddZoneLinks(zone, "depot_industrial_supply_depot", "town_entre_deux", "seaport_coastal_base_chotain");
		if (zone.m_sZoneId == "town_scythe")
			AddZoneLinks(zone, "town_kermovan", "radio_andres_beacon", "outpost_firing_range");
		if (zone.m_sZoneId == "town_perelle")
			AddZoneLinks(zone, "town_skua_point", "resource_quarry", "town_laruns");
		if (zone.m_sZoneId == "town_lancre")
			AddZoneLinks(zone, "town_ghost_town", "town_crag_point", "outpost_smugglers_hole");
		if (zone.m_sZoneId == "outpost_cave")
			AddZoneLinks(zone, "outpost_smugglers_hole", "outpost_cheval_gin_lodge", "town_ghost_town");
		if (zone.m_sZoneId == "outpost_cheval_gin_lodge")
			AddZoneLinks(zone, "outpost_black_lake", "outpost_cave", "town_vernon");
		if (zone.m_sZoneId == "town_ghost_town")
			AddZoneLinks(zone, "town_crag_point", "town_lancre", "outpost_cave");
		if (zone.m_sZoneId == "town_saint_pierre")
			AddZoneLinks(zone, "outpost_south_hq", "outpost_montfort_castle", "town_vernon");
		if (zone.m_sZoneId == "town_larue_bay")
			AddZoneLinks(zone, "outpost_montfort_castle", "town_rockweed_cape", "town_courbet");
		if (zone.m_sZoneId == "outpost_montfort_castle")
			AddZoneLinks(zone, "town_saint_pierre", "town_larue_bay", "outpost_south_hq");
		if (zone.m_sZoneId == "town_rockweed_cape")
			AddZoneLinks(zone, "town_larue_bay", "outpost_south_hq", "outpost_montfort_castle");
		if (zone.m_sZoneId == "town_le_bosc")
			AddZoneLinks(zone, "outpost_pennants_pass", "town_courbet", "town_regina");
		if (zone.m_sZoneId == "town_thollevast")
			AddZoneLinks(zone, "radio_estrapade_tower", "town_kervel", "outpost_shepherds_pond");
		if (zone.m_sZoneId == "town_argent")
			AddZoneLinks(zone, "radio_eastern_sea_relay", "radio_erquy_lighthouse", "town_erquy");
		if (zone.m_sZoneId == "town_erquy")
			AddZoneLinks(zone, "radio_erquy_lighthouse", "radio_eastern_sea_relay", "town_argent");
		if (zone.m_sZoneId == "radio_erquy_lighthouse")
			AddZoneLinks(zone, "town_erquy", "town_argent", "radio_eastern_sea_relay");
		if (zone.m_sZoneId == "resource_etoupe_gas_station")
			AddZoneLinks(zone, "depot_farm_supply_depot", "factory_power_plant", "town_maidens_bay");
		if (zone.m_sZoneId == "outpost_smugglers_hole")
			AddZoneLinks(zone, "outpost_cave", "outpost_cheval_gin_lodge", "town_lancre");
		if (zone.m_sZoneId == "town_pins")
			AddZoneLinks(zone, "radio_pins_tower", "radio_st_phillipe_relay", "outpost_north_hq");
		if (zone.m_sZoneId == "town_fishermans_cove")
			AddZoneLinks(zone, "factory_power_plant", "town_kermovan", "town_maidens_bay");
		if (zone.m_sZoneId == "town_courbet")
			AddZoneLinks(zone, "town_le_bosc", "outpost_montfort_castle", "town_larue_bay");
		if (zone.m_sZoneId == "factory_power_plant")
			AddZoneLinks(zone, "town_kermovan", "town_fishermans_cove", "resource_etoupe_gas_station");
		if (zone.m_sZoneId == "town_crag_point")
			AddZoneLinks(zone, "town_ghost_town", "town_lancre", "outpost_cave");
		if (zone.m_sZoneId == "town_skua_point")
			AddZoneLinks(zone, "town_perelle", "resource_quarry", "town_laruns");
		if (zone.m_sZoneId == "outpost_western_heights")
			AddZoneLinks(zone, "depot_green_valley_supply_depot", "town_regina", "outpost_pennants_pass");
		if (zone.m_sZoneId == "radio_pins_tower")
			AddZoneLinks(zone, "town_pins", "radio_st_phillipe_relay", "outpost_north_hq");
		if (zone.m_sZoneId == "radio_eastern_sea_relay")
			AddZoneLinks(zone, "town_scythe", "town_argent", "outpost_firing_range");
		if (zone.m_sZoneId == "radio_estrapade_tower")
			AddZoneLinks(zone, "town_thollevast", "town_kervel", "outpost_shepherds_pond");
		if (zone.m_sZoneId == "radio_st_phillipe_relay")
			AddZoneLinks(zone, "outpost_north_hq", "factory_saint_philippe", "depot_farm_supply_depot");
		if (zone.m_sZoneId == "depot_green_valley_supply_depot")
			AddZoneLinks(zone, "town_durras", "resource_quarry", "outpost_western_heights");
		if (zone.m_sZoneId == "depot_levie_supply_depot")
			AddZoneLinks(zone, "town_levie", "outpost_levie_base", "town_simons_wood");
		if (zone.m_sZoneId == "depot_figari_supply_depot")
			AddZoneLinks(zone, "outpost_pinewood_lake", "town_figari", "town_provins");
		if (zone.m_sZoneId == "depot_farm_supply_depot")
			AddZoneLinks(zone, "resource_etoupe_gas_station", "town_maidens_bay", "factory_saint_philippe");
		if (zone.m_sZoneId == "depot_gorey_supply_depot")
			AddZoneLinks(zone, "town_hornbeam_valley", "town_gravette", "town_tyrone");
		if (zone.m_sZoneId == "depot_industrial_supply_depot")
			AddZoneLinks(zone, "town_benac", "town_chotain", "town_entre_deux");
		if (zone.m_sZoneId == "depot_sawmill_supply_depot")
			AddZoneLinks(zone, "town_le_moule", "outpost_calvary_hill", "outpost_shepherds_pond");
		if (zone.m_sZoneId == "depot_regina_supply_depot")
			AddZoneLinks(zone, "town_regina", "town_goat_bay", "outpost_western_heights");
	}

	private static void AddZoneLinks(HST_ZoneState zone, string first, string second = "", string third = "")
	{
		if (!first.IsEmpty() && first != zone.m_sZoneId && !zone.m_aLinkedZoneIds.Contains(first))
			zone.m_aLinkedZoneIds.Insert(first);
		if (!second.IsEmpty() && second != zone.m_sZoneId && !zone.m_aLinkedZoneIds.Contains(second))
			zone.m_aLinkedZoneIds.Insert(second);
		if (!third.IsEmpty() && third != zone.m_sZoneId && !zone.m_aLinkedZoneIds.Contains(third))
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
		faction.m_aCapabilities.Insert("air_support_unlockable");
		faction.m_aInfantryPrefabs.Insert("{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et");
		faction.m_aInfantryPrefabs.Insert("{45A02CA25CBA9443}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Medic.et");
		faction.m_aInfantryPrefabs.Insert("{58E47E5A4D599432}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_MG.et");
		faction.m_aInfantryPrefabs.Insert("{C77DFB8546B3F2A2}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_LAT.et");
		faction.m_aVehiclePrefabs.Insert("{16C1F16C9B053801}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport.et");
		faction.m_aVehiclePrefabs.Insert("{0B4DEA8078B78A9B}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_PKM.et");
		faction.m_aGroupPrefabs.Insert("{CE41AF625D05D0F0}Prefabs/Groups/INDFOR/Group_FIA_RifleSquad.et");
		faction.m_aGroupPrefabs.Insert("{5BEA04939D148B1D}Prefabs/Groups/INDFOR/Group_FIA_FireTeam.et");
		faction.m_aPatrolGroupPrefabs.Insert("{6E725D44CA973C24}Prefabs/Groups/INDFOR/Group_FIA_SentryTeam.et");
		faction.m_aPatrolGroupPrefabs.Insert("{5BEA04939D148B1D}Prefabs/Groups/INDFOR/Group_FIA_FireTeam.et");
		faction.m_aQRFGroupPrefabs.Insert("{CE41AF625D05D0F0}Prefabs/Groups/INDFOR/Group_FIA_RifleSquad.et");
		faction.m_aQRFGroupPrefabs.Insert("{22F33D3EC8F281AB}Prefabs/Groups/INDFOR/Group_FIA_MachineGunTeam.et");
		faction.m_aGroupPool.Insert(NewPrefabPoolEntry("{CE41AF625D05D0F0}Prefabs/Groups/INDFOR/Group_FIA_RifleSquad.et", 2, "stock"));
		faction.m_aGroupPool.Insert(NewPrefabPoolEntry("{5BEA04939D148B1D}Prefabs/Groups/INDFOR/Group_FIA_FireTeam.et", 1, "stock"));
		faction.m_aPatrolGroupPool.Insert(NewPrefabPoolEntry("{6E725D44CA973C24}Prefabs/Groups/INDFOR/Group_FIA_SentryTeam.et", 2, "stock"));
		faction.m_aPatrolGroupPool.Insert(NewPrefabPoolEntry("{5BEA04939D148B1D}Prefabs/Groups/INDFOR/Group_FIA_FireTeam.et", 1, "stock"));
		faction.m_aQRFGroupPool.Insert(NewPrefabPoolEntry("{CE41AF625D05D0F0}Prefabs/Groups/INDFOR/Group_FIA_RifleSquad.et", 1, "stock"));
		faction.m_aQRFGroupPool.Insert(NewPrefabPoolEntry("{22F33D3EC8F281AB}Prefabs/Groups/INDFOR/Group_FIA_MachineGunTeam.et", 1, "stock"));
		faction.m_aSupportIds.Insert("support_mortar");
		return faction;
	}

	private static HST_FactionTemplate CreateUsTemplate()
	{
		HST_FactionTemplate faction = new HST_FactionTemplate();
		faction.m_sTemplateId = "us_occupier";
		faction.m_sFactionKey = "US";
		faction.m_sDisplayName = "US Occupiers";
		faction.m_aInfantryPrefabs.Insert("{26A9756790131354}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et");
		faction.m_aInfantryPrefabs.Insert("{84029128FA6F6BB9}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_GL.et");
		faction.m_aInfantryPrefabs.Insert("{1623EA3AEFACA0E4}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_MG.et");
		faction.m_aInfantryPrefabs.Insert("{27BF1FF235DD6036}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_LAT.et");
		faction.m_aVehiclePrefabs.Insert("{4A71F755A4513227}Prefabs/Vehicles/Wheeled/M998/M1025.et");
		faction.m_aVehiclePrefabs.Insert("{B55C6990A6A9411B}Prefabs/Vehicles/Wheeled/M998/M998_covered.et");
		faction.m_aVehiclePrefabs.Insert("{5674FAEB9AB7BDD0}Prefabs/Vehicles/Wheeled/M998/M998_uncovered.et");
		faction.m_aVehiclePrefabs.Insert("{F1FBD0972FA5FE09}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport.et");
		faction.m_aVehiclePrefabs.Insert("{81FDAD5EB644CC3D}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport_covered.et");
		faction.m_aGroupPrefabs.Insert("{DDF3799FA1387848}Prefabs/Groups/BLUFOR/Group_US_RifleSquad.et");
		faction.m_aGroupPrefabs.Insert("{84E5BBAB25EA23E5}Prefabs/Groups/BLUFOR/Group_US_FireTeam.et");
		faction.m_aPatrolGroupPrefabs.Insert("{3BF36BDEEB33AEC9}Prefabs/Groups/BLUFOR/Group_US_SentryTeam.et");
		faction.m_aPatrolGroupPrefabs.Insert("{84E5BBAB25EA23E5}Prefabs/Groups/BLUFOR/Group_US_FireTeam.et");
		faction.m_aQRFGroupPrefabs.Insert("{DDF3799FA1387848}Prefabs/Groups/BLUFOR/Group_US_RifleSquad.et");
		faction.m_aQRFGroupPrefabs.Insert("{958039B857396B7B}Prefabs/Groups/BLUFOR/Group_US_MachineGunTeam.et");
		faction.m_aGroupPool.Insert(NewPrefabPoolEntry("{DDF3799FA1387848}Prefabs/Groups/BLUFOR/Group_US_RifleSquad.et", 4, "stock"));
		faction.m_aGroupPool.Insert(NewPrefabPoolEntry("{84E5BBAB25EA23E5}Prefabs/Groups/BLUFOR/Group_US_FireTeam.et", 2, "stock"));
		faction.m_aPatrolGroupPool.Insert(NewPrefabPoolEntry("{3BF36BDEEB33AEC9}Prefabs/Groups/BLUFOR/Group_US_SentryTeam.et", 3, "stock"));
		faction.m_aPatrolGroupPool.Insert(NewPrefabPoolEntry("{84E5BBAB25EA23E5}Prefabs/Groups/BLUFOR/Group_US_FireTeam.et", 2, "stock"));
		faction.m_aQRFGroupPool.Insert(NewPrefabPoolEntry("{DDF3799FA1387848}Prefabs/Groups/BLUFOR/Group_US_RifleSquad.et", 3, "stock"));
		faction.m_aQRFGroupPool.Insert(NewPrefabPoolEntry("{958039B857396B7B}Prefabs/Groups/BLUFOR/Group_US_MachineGunTeam.et", 2, "stock"));
		faction.m_aSupportIds.Insert("support_mortar");
		return faction;
	}

	private static HST_FactionTemplate CreateUssrTemplate()
	{
		HST_FactionTemplate faction = new HST_FactionTemplate();
		faction.m_sTemplateId = "ussr_invader";
		faction.m_sFactionKey = "USSR";
		faction.m_sDisplayName = "USSR Invaders";
		faction.m_aInfantryPrefabs.Insert("{DCB41B3746FDD1BE}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_Rifleman.et");
		faction.m_aInfantryPrefabs.Insert("{8E0FE664CE7D1CA9}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_GL.et");
		faction.m_aInfantryPrefabs.Insert("{96C784C502AC37DA}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_MG.et");
		faction.m_aInfantryPrefabs.Insert("{BF643BE4ADBDFDD3}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_LAT.et");
		faction.m_aVehiclePrefabs.Insert("{0B4DEA8078B78A9B}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_PKM.et");
		faction.m_aVehiclePrefabs.Insert("{4597626AF36C0858}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320.et");
		faction.m_aVehiclePrefabs.Insert("{16C1F16C9B053801}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport.et");
		faction.m_aVehiclePrefabs.Insert("{C012BB3488BEA0C2}Prefabs/Vehicles/Wheeled/BTR70/BTR70.et");
		faction.m_aGroupPrefabs.Insert("{E552DABF3636C2AD}Prefabs/Groups/OPFOR/Group_USSR_RifleSquad.et");
		faction.m_aGroupPrefabs.Insert("{30ED11AA4F0D41E5}Prefabs/Groups/OPFOR/Group_USSR_FireGroup.et");
		faction.m_aPatrolGroupPrefabs.Insert("{CB58D90EA14430AD}Prefabs/Groups/OPFOR/Group_USSR_SentryTeam.et");
		faction.m_aPatrolGroupPrefabs.Insert("{30ED11AA4F0D41E5}Prefabs/Groups/OPFOR/Group_USSR_FireGroup.et");
		faction.m_aQRFGroupPrefabs.Insert("{E552DABF3636C2AD}Prefabs/Groups/OPFOR/Group_USSR_RifleSquad.et");
		faction.m_aQRFGroupPrefabs.Insert("{A2F75E45C66B1C0A}Prefabs/Groups/OPFOR/Group_USSR_MachineGunTeam.et");
		faction.m_aGroupPool.Insert(NewPrefabPoolEntry("{E552DABF3636C2AD}Prefabs/Groups/OPFOR/Group_USSR_RifleSquad.et", 4, "stock"));
		faction.m_aGroupPool.Insert(NewPrefabPoolEntry("{30ED11AA4F0D41E5}Prefabs/Groups/OPFOR/Group_USSR_FireGroup.et", 2, "stock"));
		faction.m_aPatrolGroupPool.Insert(NewPrefabPoolEntry("{CB58D90EA14430AD}Prefabs/Groups/OPFOR/Group_USSR_SentryTeam.et", 4, "stock"));
		faction.m_aPatrolGroupPool.Insert(NewPrefabPoolEntry("{30ED11AA4F0D41E5}Prefabs/Groups/OPFOR/Group_USSR_FireGroup.et", 2, "stock"));
		faction.m_aQRFGroupPool.Insert(NewPrefabPoolEntry("{E552DABF3636C2AD}Prefabs/Groups/OPFOR/Group_USSR_RifleSquad.et", 3, "stock"));
		faction.m_aQRFGroupPool.Insert(NewPrefabPoolEntry("{A2F75E45C66B1C0A}Prefabs/Groups/OPFOR/Group_USSR_MachineGunTeam.et", 2, "stock"));
		faction.m_aSupportIds.Insert("support_mortar");
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
		ApplyMissionContract(mission);
		return mission;
	}

	private static void ApplyMissionContract(HST_MissionDefinition mission)
	{
		if (!mission)
			return;

		mission.m_sRuntimeType = RuntimeTypeForMission(mission);
		mission.m_sTargetSitePreference = TargetSitePreferenceForMission(mission);
		mission.m_sBriefingText = BriefingForMission(mission);
		mission.m_sRequirementText = RequirementForMission(mission);
		mission.m_sFailureText = FailureTextForMission(mission);
		mission.m_iCargoCount = CargoCountForMission(mission);
		mission.m_iCaptiveCount = CaptiveCountForMission(mission);
		mission.m_iVehicleCount = VehicleCountForMission(mission);
		mission.m_iRewardHR = RewardHRForMission(mission);
		mission.m_iRewardSupport = RewardSupportForMission(mission);
		mission.m_iRewardCaptureProgress = RewardCaptureProgressForMission(mission);
		mission.m_sRewardText = RewardTextForMission(mission);
		mission.m_sFailurePenaltyText = string.Format("Failure increases enemy aggression by %1 and may improve hostile support near the objective.", mission.m_iFailureAggression);
		mission.m_iRequiredWarLevel = RequiredWarLevelForMission(mission);
		ConfigureMissionTargetRules(mission);
	}

	private static void ConfigureMissionTargetRules(HST_MissionDefinition mission)
	{
		if (!mission)
			return;

		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
		{
			mission.m_bAllowFriendlyTarget = true;
			mission.m_bAllowEnemyTarget = false;
		}

		if (mission.m_sMissionId == "conquest_town" || mission.m_sMissionId == "dynamic_city_flip_battle" || mission.m_sMissionId == "dynamic_minor_city_task" || mission.m_sMissionId == "support_city_supplies")
			mission.m_aTargetZoneTypes.Insert("town");
		else if (mission.m_sMissionId == "conquest_resource" || mission.m_sMissionId == "logistics_resource_cache")
			mission.m_aTargetZoneTypes.Insert("resource");
		else if (mission.m_sMissionId == "conquest_factory" || mission.m_sMissionId == "destroy_factory_asset" || mission.m_sMissionId == "logistics_factory_supplies")
			mission.m_aTargetZoneTypes.Insert("factory");
		else if (mission.m_sMissionId == "conquest_airfield" || mission.m_sMissionId == "destroy_airfield_asset" || mission.m_sMissionId == "logistics_airfield_intel")
			mission.m_aTargetZoneTypes.Insert("airfield");
		else if (mission.m_sMissionId == "conquest_seaport" || mission.m_sMissionId == "destroy_seaport_asset" || mission.m_sMissionId == "logistics_seaport_supplies")
			mission.m_aTargetZoneTypes.Insert("seaport");
		else if (mission.m_sMissionId == "conquest_outpost" || mission.m_sMissionId == "destroy_outpost_cache" || mission.m_sMissionId == "destroy_or_steal_armor")
			mission.m_aTargetZoneTypes.Insert("outpost");
		else if (mission.m_sMissionId == "destroy_radio_tower" || mission.m_sMissionId == "dynamic_stop_tower_rebuild")
			mission.m_aTargetZoneTypes.Insert("radio");
		else if (mission.m_sMissionId == "logistics_bank")
			mission.m_aTargetZoneTypes.Insert("town");
		else if (mission.m_sMissionId == "destroy_downed_helicopter" || mission.m_sMissionId == "logistics_salvage_supplies")
			mission.m_aTargetZoneTypes.Insert("crashsite");
		else if (mission.m_sMissionId == "logistics_support_cache")
			mission.m_aTargetZoneTypes.Insert("support");

		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY || mission.m_eCategory == HST_EMissionCategory.HST_MISSION_ASSASSINATION || mission.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE || mission.m_sMissionId == "dynamic_gun_shop")
			mission.m_aTargetZoneTypes.Insert("any");
	}

	private static string RuntimeTypeForMission(HST_MissionDefinition mission)
	{
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_ASSASSINATION)
			return "assassination_hvt";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return "conquest_clear_hold";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return "convoy_route_intercept";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return "destroy_asset";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
			return "logistics_extract";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			return "rescue_extract";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return "support_delivery";

		if (mission.m_sMissionId == "dynamic_defend_petros")
			return "dynamic_defend_petros";
		if (mission.m_sMissionId == "dynamic_stop_tower_rebuild")
			return "dynamic_tower_rebuild";
		if (mission.m_sMissionId == "dynamic_city_flip_battle")
			return "dynamic_city_battle";
		if (mission.m_sMissionId == "dynamic_gun_shop")
			return "dynamic_gun_shop";

		return "dynamic_minor_task";
	}

	private static string TargetSitePreferenceForMission(HST_MissionDefinition mission)
	{
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return "roadblock";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS || mission.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT || mission.m_sMissionId == "dynamic_gun_shop")
			return "support";
		if (mission.m_sMissionId == "destroy_downed_helicopter" || mission.m_sMissionId == "logistics_salvage_supplies")
			return "crashsite";

		return "primary";
	}

	private static string BriefingForMission(HST_MissionDefinition mission)
	{
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_ASSASSINATION)
			return "Locate the marked target, eliminate them, and leave the area before enemy forces reorganize.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return "Attack the objective, clear hostile defenders, and hold the ground long enough for FIA control to stick.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return "Ambush the moving enemy convoy before it reaches destination and neutralize the convoy crew.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return "Reach the objective asset and destroy, disable, or capture it depending on the mission variant.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
			return "Recover the marked cargo or vehicle and get it under FIA control.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			return "Find the captives, make contact, and escort them alive to HQ or a friendly zone.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return "Move FIA supplies from the staging point to the target town to improve local support.";

		return "A dynamic opportunity has appeared. Resolve the objective before the timer expires.";
	}

	private static string RequirementForMission(HST_MissionDefinition mission)
	{
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_ASSASSINATION)
			return "Kill the HVT before the timer expires.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return "Clear defenders, stay inside the objective area, and complete the hold timer.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return "Kill all convoy soldiers before the convoy reaches its destination. Capturing surviving vehicles is optional.";
		if (mission.m_sMissionId == "destroy_or_steal_armor")
			return "Destroy the armor or capture and garage it at HQ.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return "Destroy the marked target asset.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
			return "Recover the marked cargo or vehicle.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			return "Extract all captives alive.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return "Deliver supplies to the target town.";

		return "Complete all displayed objectives.";
	}

	private static string FailureTextForMission(HST_MissionDefinition mission)
	{
		if (mission.m_sMissionId == "assassinate_traitor")
			return "The traitor escapes and can trigger a Defend Petros response.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return "The convoy reaches its destination with living crew.";
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			return "Captives are lost or the extraction timer expires.";

		return "Timer expires or the required mission asset is lost.";
	}

	private static string RewardTextForMission(HST_MissionDefinition mission)
	{
		string reward = string.Format("$%1 FIA funds", mission.m_iRewardMoney);
		if (mission.m_iRewardHR > 0)
			reward = reward + string.Format(", %1 HR", mission.m_iRewardHR);
		if (mission.m_iRewardSupport > 0)
			reward = reward + string.Format(", %1 town support", mission.m_iRewardSupport);
		if (mission.m_iRewardCaptureProgress > 0)
			reward = reward + string.Format(", %1 capture progress", mission.m_iRewardCaptureProgress);
		return reward;
	}

	private static int RewardHRForMission(HST_MissionDefinition mission)
	{
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			return Math.Max(1, mission.m_iCaptiveCount);
		if (mission.m_sMissionId == "convoy_prisoners")
			return 2;
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
			return 1;
		return 0;
	}

	private static int RewardSupportForMission(HST_MissionDefinition mission)
	{
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return 25;
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_RESCUE)
			return 15;
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return 10;
		if (mission.m_sMissionId == "assassinate_specops")
			return 10;
		return 0;
	}

	private static int RewardCaptureProgressForMission(HST_MissionDefinition mission)
	{
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return 60;
		if (mission.m_sMissionId == "dynamic_city_flip_battle")
			return 50;
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return 25;
		return 0;
	}

	private static int CargoCountForMission(HST_MissionDefinition mission)
	{
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS || mission.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
			return 2;
		return 0;
	}

	private static int CaptiveCountForMission(HST_MissionDefinition mission)
	{
		if (mission.m_sMissionId == "rescue_refugees")
			return 4;
		if (mission.m_sMissionId == "rescue_pows")
			return 3;
		return 0;
	}

	private static int VehicleCountForMission(HST_MissionDefinition mission)
	{
		if (mission.m_sMissionId == "destroy_or_steal_armor")
			return 2;
		if (mission.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return 3;
		return 0;
	}

	private static int RequiredWarLevelForMission(HST_MissionDefinition mission)
	{
		if (mission.m_sMissionId == "conquest_airfield" || mission.m_sMissionId == "conquest_seaport")
			return 3;
		if (mission.m_sMissionId == "convoy_armored" || mission.m_sMissionId == "assassinate_specops")
			return 2;
		return 1;
	}
}
