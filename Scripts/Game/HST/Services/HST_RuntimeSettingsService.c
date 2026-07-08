class HST_RuntimeSettingsService
{
	static const string SETTINGS_DIRECTORY = "$profile:h-istasi";
	static const string SETTINGS_FILE = "$profile:h-istasi/HST_Settings.json";

	static bool LoadDebugLoggingEnabledQuiet()
	{
		bool enabled;
		if (!FileIO.FileExists(SETTINGS_FILE))
			return enabled;

		FileHandle file = FileIO.OpenFile(SETTINGS_FILE, FileMode.READ);
		if (!file)
			return enabled;

		string line;
		while (file.ReadLine(line) >= 0)
		{
			if (LineHasKeyStatic(line, "verboseLogging") || LineHasKeyStatic(line, "debugLoggingEnabled"))
				enabled = ExtractBoolValueStatic(line);
		}

		file.Close();
		return enabled;
	}

	string GetSettingsFilePath()
	{
		return SETTINGS_FILE;
	}

	HST_RuntimeSettings LoadOrCreate()
	{
		HST_RuntimeSettings settings = new HST_RuntimeSettings();

		if (!FileIO.FileExists(SETTINGS_FILE))
		{
			FileIO.MakeDirectory(SETTINGS_DIRECTORY);
			WriteDefault(settings);
			Print("h-istasi | generated first-load settings at " + SETTINGS_FILE);
			return settings;
		}

		array<string> lines = ReadLines(SETTINGS_FILE);
		if (!lines || lines.Count() == 0)
		{
			Print("h-istasi | settings file is empty or unreadable, using built-in defaults", LogLevel.WARNING);
			return settings;
		}

		if (!LooksLikeJson(lines))
		{
			Print("h-istasi | settings file is malformed, using built-in defaults without overwriting it", LogLevel.ERROR);
			return settings;
		}

		ApplyKnownKeys(settings, lines);
		if (MigrateSettings(settings))
		{
			WriteDefault(settings);
			Print("h-istasi | migrated runtime settings to schema " + settings.m_iSchemaVersion);
		}

		settings.Normalize();
		Print("h-istasi | loaded runtime settings from " + SETTINGS_FILE);
		return settings;
	}

	protected bool LooksLikeJson(notnull array<string> lines)
	{
		bool sawOpen;
		bool sawClose;
		foreach (string line : lines)
		{
			if (line.Contains("{"))
				sawOpen = true;
			if (line.Contains("}"))
				sawClose = true;
		}

		return sawOpen && sawClose;
	}

	protected void ApplyKnownKeys(notnull HST_RuntimeSettings settings, notnull array<string> lines)
	{
		foreach (string line : lines)
		{
			ApplyInt(line, "schemaVersion", settings.m_iSchemaVersion);
			ApplyString(line, "presetId", settings.m_Campaign.m_sPresetId);
			ApplyInt(line, "campaignSeed", settings.m_Campaign.m_iCampaignSeed);
			ApplyString(line, "resistanceFactionKey", settings.m_Factions.m_sResistanceFactionKey);
			ApplyString(line, "occupierFactionKey", settings.m_Factions.m_sOccupierFactionKey);
			ApplyString(line, "invaderFactionKey", settings.m_Factions.m_sInvaderFactionKey);
			ApplyInt(line, "startingFactionMoney", settings.m_Economy.m_iStartingFactionMoney);
			ApplyInt(line, "startingHR", settings.m_Economy.m_iStartingHR);
			ApplyInt(line, "startingTrainingLevel", settings.m_Economy.m_iStartingTrainingLevel);
			ApplyInt(line, "startingOccupierAttackPool", settings.m_Economy.m_iStartingOccupierAttackPool);
			ApplyInt(line, "startingOccupierSupportPool", settings.m_Economy.m_iStartingOccupierSupportPool);
			ApplyInt(line, "startingInvaderAttackPool", settings.m_Economy.m_iStartingInvaderAttackPool);
			ApplyInt(line, "startingInvaderSupportPool", settings.m_Economy.m_iStartingInvaderSupportPool);
			ApplyInt(line, "zoneIncomeIntervalSeconds", settings.m_Economy.m_iZoneIncomeIntervalSeconds);
			ApplyInt(line, "warLevelMaximum", settings.m_Economy.m_iWarLevelMaximum);
			ApplyInt(line, "warLevel2Score", settings.m_Economy.m_iWarLevel2Score);
			ApplyInt(line, "warLevel3Score", settings.m_Economy.m_iWarLevel3Score);
			ApplyInt(line, "warLevel4Score", settings.m_Economy.m_iWarLevel4Score);
			ApplyInt(line, "warLevel5Score", settings.m_Economy.m_iWarLevel5Score);
			ApplyInt(line, "warLevel6Score", settings.m_Economy.m_iWarLevel6Score);
			ApplyInt(line, "warLevel7Score", settings.m_Economy.m_iWarLevel7Score);
			ApplyInt(line, "warLevel8Score", settings.m_Economy.m_iWarLevel8Score);
			ApplyInt(line, "warLevel9Score", settings.m_Economy.m_iWarLevel9Score);
			ApplyInt(line, "warLevel10Score", settings.m_Economy.m_iWarLevel10Score);
			ApplyInt(line, "victoryControlPercent", settings.m_Economy.m_iVictoryControlPercent);
			ApplyBool(line, "populationOutcomeEnabled", settings.m_Economy.m_bPopulationOutcomeEnabled);
			ApplyInt(line, "victoryPopulationSupportPercent", settings.m_Economy.m_iVictoryPopulationSupportPercent);
			ApplyBool(line, "legacyControlVictoryEnabled", settings.m_Economy.m_bLegacyControlVictoryEnabled);
			ApplyBool(line, "victoryRequiresAirfields", settings.m_Economy.m_bVictoryRequiresAirfields);
			ApplyBool(line, "victoryRequiresSeaports", settings.m_Economy.m_bVictoryRequiresSeaports);
			ApplyBool(line, "lossConditionEnabled", settings.m_Economy.m_bLossConditionEnabled);
			ApplyInt(line, "lossHRThreshold", settings.m_Economy.m_iLossHRThreshold);
			ApplyInt(line, "lossMoneyThreshold", settings.m_Economy.m_iLossMoneyThreshold);
			ApplyInt(line, "lossPetrosDeathLimit", settings.m_Economy.m_iLossPetrosDeathLimit);
			ApplyInt(line, "lossGraceSeconds", settings.m_Economy.m_iLossGraceSeconds);
			ApplyInt(line, "enemyAttackIncomeWarPercent", settings.m_Economy.m_iEnemyAttackIncomeWarPercent);
			ApplyInt(line, "enemySupportIncomeWarPercent", settings.m_Economy.m_iEnemySupportIncomeWarPercent);
			ApplyInt(line, "aggressionDecayIntervalSeconds", settings.m_Economy.m_iAggressionDecayIntervalSeconds);
			ApplyInt(line, "aggressionDecayAmount", settings.m_Economy.m_iAggressionDecayAmount);
			ApplyInt(line, "captureProgressRequired", settings.m_Capture.m_iProgressRequired);
			ApplyInt(line, "captureProgressPerSecond", settings.m_Capture.m_iProgressPerSecond);
			ApplyInt(line, "captureDecayPerSecond", settings.m_Capture.m_iDecayPerSecond);
			ApplyInt(line, "captureAggressionBase", settings.m_Capture.m_iAggressionBase);
			ApplyInt(line, "captureCounterattackChancePercent", settings.m_Capture.m_iCounterattackChancePercent);
			ApplyInt(line, "activationRadiusMeters", settings.m_World.m_iActivationRadiusMeters);
			ApplyInt(line, "deactivationRadiusMeters", settings.m_World.m_iDeactivationRadiusMeters);
			ApplyInt(line, "playerRenderBubbleRadiusMeters", settings.m_World.m_iPlayerRenderBubbleRadiusMeters);
			ApplyInt(line, "missionSelectionRadiusMeters", settings.m_World.m_iMissionSelectionRadiusMeters);
			ApplyInt(line, "missionDefaultDurationSeconds", settings.m_World.m_iMissionDefaultDurationSeconds);
			ApplyBool(line, "membershipEnabled", settings.m_Membership.m_bMembershipEnabled);
			ApplyBool(line, "guestsCanOpenMenu", settings.m_Membership.m_bGuestsCanOpenMenu);
			ApplyStringArray(line, "adminIdentityIds", settings.m_Membership.m_aAdminIdentityIds);
			ApplyInt(line, "arsenalUnlockThreshold", settings.m_ArsenalLoot.m_iArsenalUnlockThreshold);
			ApplyInt(line, "magazineUnlockMultiplier", settings.m_ArsenalLoot.m_iMagazineUnlockMultiplier);
			ApplyInt(line, "hqInteractionRadiusMeters", settings.m_ArsenalLoot.m_iHQInteractionRadiusMeters);
			ApplyInt(line, "lootRadiusMeters", settings.m_ArsenalLoot.m_iLootRadiusMeters);
			ApplyBool(line, "lootOnlyLockedItems", settings.m_ArsenalLoot.m_bLootOnlyLockedItems);
			ApplyBool(line, "removeLootedItems", settings.m_ArsenalLoot.m_bRemoveLootedItems);
			ApplyBool(line, "allowExplosiveUnlocks", settings.m_ArsenalLoot.m_bAllowExplosiveUnlocks);
			ApplyBool(line, "allowGuidedLauncherUnlocks", settings.m_ArsenalLoot.m_bAllowGuidedLauncherUnlocks);
			ApplyBool(line, "vehicleLootEnabled", settings.m_VehicleLoot.m_bEnabled);
			ApplyInt(line, "vehicleLootRadiusMeters", settings.m_VehicleLoot.m_iRadiusMeters);
			ApplyBool(line, "vehicleLootOnlyLockedItems", settings.m_VehicleLoot.m_bOnlyLockedItems);
			ApplyBool(line, "vehicleLootRemoveSourceItems", settings.m_VehicleLoot.m_bRemoveSourceItems);
			ApplyInt(line, "vehicleLootMaxItemsPerAction", settings.m_VehicleLoot.m_iMaxItemsPerAction);
			ApplyBool(line, "airSupportEnabled", settings.m_AirSupport.m_bEnabled);
			ApplyInt(line, "airSupportCooldownSeconds", settings.m_AirSupport.m_iCooldownSeconds);
			ApplyBool(line, "civilianPopulationEnabled", settings.m_Civilians.m_bEnabled);
			ApplyInt(line, "civilianMaxActivePerTown", settings.m_Civilians.m_iMaxActivePerTown);
			ApplyInt(line, "civilianVehicleMinPerTown", settings.m_Civilians.m_iCivilianVehicleMinPerTown);
			ApplyInt(line, "civilianVehicleMaxPerTown", settings.m_Civilians.m_iCivilianVehicleMaxPerTown);
			ApplyInt(line, "civilianDrivingVehicleCountPerTown", settings.m_Civilians.m_iCivilianDrivingVehicleCountPerTown);
			ApplyInt(line, "occupierVehicleMinPerTown", settings.m_Civilians.m_iOccupierVehicleMinPerTown);
			ApplyInt(line, "occupierVehicleMaxPerTown", settings.m_Civilians.m_iOccupierVehicleMaxPerTown);
			ApplyInt(line, "autosaveIntervalSeconds", settings.m_Persistence.m_iAutosaveIntervalSeconds);
			ApplyInt(line, "majorChangeDebounceSeconds", settings.m_Persistence.m_iMajorChangeDebounceSeconds);
			ApplyBool(line, "debugMenuEnabled", settings.m_Debug.m_bDebugMenuEnabled);
			ApplyBool(line, "verboseLogging", settings.m_Debug.m_bDebugLoggingEnabled);
			ApplyBool(line, "debugLoggingEnabled", settings.m_Debug.m_bDebugLoggingEnabled);
			ApplyBool(line, "physicalWarEnabled", settings.m_Features.m_bPhysicalWarEnabled);
			ApplyBool(line, "areaLootEnabled", settings.m_Features.m_bAreaLootEnabled);
			ApplyBool(line, "setupUiReadOnly", settings.m_Features.m_bSetupUiReadOnly);
			ApplyBool(line, "gameMasterBudgetsEnabled", settings.m_Features.m_bGameMasterBudgetsEnabled);
			ApplyBool(line, "showPlayerMapMarkers", settings.m_Features.m_bShowPlayerMapMarkers);
			ApplyBool(line, "infiniteStaminaEnabled", settings.m_Features.m_bInfiniteStaminaEnabled);
			ApplyBool(line, "trackResistanceSupportGroupsOnMap", settings.m_Features.m_bTrackResistanceSupportGroupsOnMap);
		}

		ApplyStringArrayFromLines(lines, "adminIdentityIds", settings.m_Membership.m_aAdminIdentityIds);
	}

	protected bool MigrateSettings(notnull HST_RuntimeSettings settings)
	{
		bool changed;
		if (settings.m_iSchemaVersion < 3 && settings.m_ArsenalLoot.m_iArsenalUnlockThreshold == 15)
		{
			settings.m_ArsenalLoot.m_iArsenalUnlockThreshold = 25;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 4)
		{
			if (settings.m_Civilians.m_iMaxActivePerTown == 8)
				settings.m_Civilians.m_iMaxActivePerTown = 12;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 5)
		{
			if (settings.m_Civilians.m_iCivilianVehicleMinPerTown == 2 && settings.m_Civilians.m_iCivilianVehicleMaxPerTown == 4)
			{
				settings.m_Civilians.m_iCivilianVehicleMinPerTown = 1;
				settings.m_Civilians.m_iCivilianVehicleMaxPerTown = 5;
				changed = true;
			}
			else if (settings.m_Civilians.m_iCivilianVehicleMinPerTown == 1 && settings.m_Civilians.m_iCivilianVehicleMaxPerTown == 3)
			{
				settings.m_Civilians.m_iCivilianVehicleMaxPerTown = 5;
				changed = true;
			}

			if (settings.m_Civilians.m_iOccupierVehicleMinPerTown == 1 && settings.m_Civilians.m_iOccupierVehicleMaxPerTown == 3)
			{
				settings.m_Civilians.m_iOccupierVehicleMinPerTown = 0;
				settings.m_Civilians.m_iOccupierVehicleMaxPerTown = 2;
				changed = true;
			}
			else if (settings.m_Civilians.m_iOccupierVehicleMinPerTown == 1 && settings.m_Civilians.m_iOccupierVehicleMaxPerTown == 2)
			{
				settings.m_Civilians.m_iOccupierVehicleMinPerTown = 0;
				changed = true;
			}
		}

		if (settings.m_iSchemaVersion < 6)
		{
			settings.m_ArsenalLoot.m_iHQInteractionRadiusMeters = 50;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 7)
		{
			settings.m_ArsenalLoot.m_bLootOnlyLockedItems = false;
			settings.m_VehicleLoot.m_bOnlyLockedItems = false;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 8)
		{
			settings.m_Capture.m_iProgressRequired = 100;
			settings.m_Capture.m_iProgressPerSecond = 2;
			settings.m_Capture.m_iDecayPerSecond = 1;
			settings.m_Capture.m_iAggressionBase = 10;
			settings.m_Capture.m_iCounterattackChancePercent = 45;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 9)
		{
			if (settings.m_Economy.m_iStartingFactionMoney == 1000)
				settings.m_Economy.m_iStartingFactionMoney = 750;
			if (settings.m_Economy.m_iStartingHR == 20)
				settings.m_Economy.m_iStartingHR = 10;
			if (settings.m_Economy.m_iStartingOccupierAttackPool == 100)
				settings.m_Economy.m_iStartingOccupierAttackPool = 70;
			if (settings.m_Economy.m_iStartingOccupierSupportPool == 100)
				settings.m_Economy.m_iStartingOccupierSupportPool = 80;
			if (settings.m_Economy.m_iStartingInvaderAttackPool == 60)
				settings.m_Economy.m_iStartingInvaderAttackPool = 35;
			if (settings.m_Economy.m_iStartingInvaderSupportPool == 60)
				settings.m_Economy.m_iStartingInvaderSupportPool = 45;
			if (settings.m_ArsenalLoot.m_iArsenalUnlockThreshold == 25)
				settings.m_ArsenalLoot.m_iArsenalUnlockThreshold = 18;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 10)
		{
			changed = true;
		}

		if (settings.m_iSchemaVersion < 11)
		{
			settings.m_Features.m_bGameMasterBudgetsEnabled = false;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 12)
		{
			settings.m_Features.m_bShowPlayerMapMarkers = true;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 13)
		{
			settings.m_Features.m_bInfiniteStaminaEnabled = true;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 14)
		{
			settings.m_World.m_iPlayerRenderBubbleRadiusMeters = 1800;
			settings.m_World.m_iMissionSelectionRadiusMeters = 1800;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 15)
		{
			settings.m_Economy.m_bPopulationOutcomeEnabled = true;
			settings.m_Economy.m_iVictoryPopulationSupportPercent = 50;
			settings.m_Economy.m_bLegacyControlVictoryEnabled = false;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 16)
		{
			settings.m_Features.m_bTrackResistanceSupportGroupsOnMap = true;
			changed = true;
		}

		if (settings.m_iSchemaVersion < 17)
		{
			changed = true;
		}

		if (settings.m_iSchemaVersion < 18)
		{
			changed = true;
		}

		if (settings.m_iSchemaVersion < 19)
		{
			if (settings.m_Civilians.m_iCivilianDrivingVehicleCountPerTown <= 0)
				settings.m_Civilians.m_iCivilianDrivingVehicleCountPerTown = 2;
			changed = true;
		}

		if (settings.m_iSchemaVersion < HST_RuntimeSettings.SCHEMA_VERSION)
		{
			settings.m_iSchemaVersion = HST_RuntimeSettings.SCHEMA_VERSION;
			changed = true;
		}

		settings.Normalize();
		return changed;
	}

	protected void ApplyString(string line, string key, out string target)
	{
		if (!LineHasKey(line, key))
			return;

		string value = ExtractValue(line);
		if (!value.IsEmpty())
			target = value;
	}

	protected void ApplyInt(string line, string key, out int target)
	{
		if (!LineHasKey(line, key))
			return;

		target = ExtractValue(line).ToInt();
	}

	protected void ApplyBool(string line, string key, out bool target)
	{
		if (!LineHasKey(line, key))
			return;

		string value = ExtractValue(line);
		value.ToLower();
		target = value == "true" || value == "1";
	}

	protected void ApplyStringArray(string line, string key, notnull array<string> target)
	{
		if (!LineHasKey(line, key))
			return;

		int open = line.IndexOf("[");
		int close = line.IndexOf("]");
		if (open < 0 || close <= open)
			return;

		string values = line.Substring(open + 1, close - open - 1);
		ParseStringArrayValues(values, target);
	}

	protected void ApplyStringArrayFromLines(notnull array<string> lines, string key, notnull array<string> target)
	{
		bool collecting;
		string values;
		foreach (string line : lines)
		{
			if (!collecting)
			{
				if (!LineHasKey(line, key))
					continue;

				int open = line.IndexOf("[");
				if (open < 0)
					return;

				int close = line.IndexOf("]");
				if (close > open)
				{
					values = line.Substring(open + 1, close - open - 1);
					ParseStringArrayValues(values, target);
					return;
				}

				values = line.Substring(open + 1, line.Length() - open - 1);
				collecting = true;
				continue;
			}

			int lineClose = line.IndexOf("]");
			if (lineClose >= 0)
			{
				values = values + "," + line.Substring(0, lineClose);
				ParseStringArrayValues(values, target);
				return;
			}

			values = values + "," + line;
		}
	}

	protected void ParseStringArrayValues(string values, notnull array<string> target)
	{
		target.Clear();
		values.Replace("\"", "");
		values.Replace("[", "");
		values.Replace("]", "");
		array<string> tokens = {};
		values.Split(",", tokens, true);
		foreach (string token : tokens)
		{
			token = token.Trim();
			if (!token.IsEmpty())
				target.Insert(token);
		}
	}

	protected bool LineHasKey(string line, string key)
	{
		return line.Contains("\"" + key + "\"");
	}

	protected static bool LineHasKeyStatic(string line, string key)
	{
		return line.Contains("\"" + key + "\"");
	}

	protected string ExtractValue(string line)
	{
		int colon = line.IndexOf(":");
		if (colon < 0)
			return "";

		string value = line.Substring(colon + 1, line.Length() - colon - 1);
		value.Replace(",", "");
		value.Replace("\"", "");
		value = value.Trim();
		return value;
	}

	protected static bool ExtractBoolValueStatic(string line)
	{
		int colon = line.IndexOf(":");
		if (colon < 0)
			return false;

		string value = line.Substring(colon + 1, line.Length() - colon - 1);
		value.Replace(",", "");
		value.Replace("\"", "");
		value = value.Trim();
		value.ToLower();
		return value == "true" || value == "1";
	}

	protected void WriteDefault(notnull HST_RuntimeSettings settings)
	{
		settings.Normalize();
		array<string> lines = {};
		lines.Insert("{");
		lines.Insert("  \"_comment\": \"Generated by h-istasi. JSON does not support real comments, so _comment fields explain nearby settings and are ignored by the loader.\",");
		lines.Insert("  \"_comment_schemaVersion\": \"Internal settings format version. Leave this at the generated value so migrations can run normally.\",");
		lines.Insert(string.Format("  \"schemaVersion\": %1,", settings.m_iSchemaVersion));
		lines.Insert("  \"campaign\": {");
		lines.Insert("    \"_comment\": \"Campaign defaults used when a new campaign is created.\",");
		lines.Insert("    \"_comment_presetId\": \"Campaign preset id loaded by setup. The current supported preset is vanilla_everon.\",");
		lines.Insert(string.Format("    \"presetId\": \"%1\",", settings.m_Campaign.m_sPresetId));
		lines.Insert("    \"_comment_campaignSeed\": \"Seed used by deterministic generated content when a repeatable random source is needed.\",");
		lines.Insert(string.Format("    \"campaignSeed\": %1", settings.m_Campaign.m_iCampaignSeed));
		lines.Insert("  },");
		lines.Insert("  \"factions\": {");
		lines.Insert("    \"_comment\": \"Faction keys used by campaign systems and spawned forces.\",");
		lines.Insert("    \"_comment_resistanceFactionKey\": \"Playable resistance faction key for player forces, recruits, and support.\",");
		lines.Insert(string.Format("    \"resistanceFactionKey\": \"%1\",", settings.m_Factions.m_sResistanceFactionKey));
		lines.Insert("    \"_comment_occupierFactionKey\": \"Primary occupying enemy faction key.\",");
		lines.Insert(string.Format("    \"occupierFactionKey\": \"%1\",", settings.m_Factions.m_sOccupierFactionKey));
		lines.Insert("    \"_comment_invaderFactionKey\": \"Secondary invading enemy faction key.\",");
		lines.Insert(string.Format("    \"invaderFactionKey\": \"%1\"", settings.m_Factions.m_sInvaderFactionKey));
		lines.Insert("  },");
		lines.Insert("  \"economy\": {");
		lines.Insert("    \"_comment\": \"Starting resources, war-level pacing, outcome rules, enemy resource income, and aggression decay.\",");
		lines.Insert("    \"_comment_startingFactionMoney\": \"Money available to the resistance when a new campaign starts.\",");
		lines.Insert(string.Format("    \"startingFactionMoney\": %1,", settings.m_Economy.m_iStartingFactionMoney));
		lines.Insert("    \"_comment_startingHR\": \"Human resources available for resistance recruiting at campaign start.\",");
		lines.Insert(string.Format("    \"startingHR\": %1,", settings.m_Economy.m_iStartingHR));
		lines.Insert("    \"_comment_startingTrainingLevel\": \"Initial training tier for resistance recruits and force quality.\",");
		lines.Insert(string.Format("    \"startingTrainingLevel\": %1,", settings.m_Economy.m_iStartingTrainingLevel));
		lines.Insert("    \"_comment_startingOccupierAttackPool\": \"Initial occupier resource pool for proactive attacks.\",");
		lines.Insert(string.Format("    \"startingOccupierAttackPool\": %1,", settings.m_Economy.m_iStartingOccupierAttackPool));
		lines.Insert("    \"_comment_startingOccupierSupportPool\": \"Initial occupier resource pool for QRF, support, and defense responses.\",");
		lines.Insert(string.Format("    \"startingOccupierSupportPool\": %1,", settings.m_Economy.m_iStartingOccupierSupportPool));
		lines.Insert("    \"_comment_startingInvaderAttackPool\": \"Initial invader resource pool for proactive attacks.\",");
		lines.Insert(string.Format("    \"startingInvaderAttackPool\": %1,", settings.m_Economy.m_iStartingInvaderAttackPool));
		lines.Insert("    \"_comment_startingInvaderSupportPool\": \"Initial invader resource pool for QRF, support, and defense responses.\",");
		lines.Insert(string.Format("    \"startingInvaderSupportPool\": %1,", settings.m_Economy.m_iStartingInvaderSupportPool));
		lines.Insert("    \"_comment_zoneIncomeIntervalSeconds\": \"Seconds between passive income and enemy resource ticks.\",");
		lines.Insert(string.Format("    \"zoneIncomeIntervalSeconds\": %1,", settings.m_Economy.m_iZoneIncomeIntervalSeconds));
		lines.Insert("    \"_comment_warLevelMaximum\": \"Highest war level the campaign can reach.\",");
		lines.Insert(string.Format("    \"warLevelMaximum\": %1,", settings.m_Economy.m_iWarLevelMaximum));
		lines.Insert("    \"_comment_warLevelScores\": \"Score thresholds for each war level. Each value must be higher than the previous level.\",");
		lines.Insert(string.Format("    \"warLevel2Score\": %1,", settings.m_Economy.m_iWarLevel2Score));
		lines.Insert(string.Format("    \"warLevel3Score\": %1,", settings.m_Economy.m_iWarLevel3Score));
		lines.Insert(string.Format("    \"warLevel4Score\": %1,", settings.m_Economy.m_iWarLevel4Score));
		lines.Insert(string.Format("    \"warLevel5Score\": %1,", settings.m_Economy.m_iWarLevel5Score));
		lines.Insert(string.Format("    \"warLevel6Score\": %1,", settings.m_Economy.m_iWarLevel6Score));
		lines.Insert(string.Format("    \"warLevel7Score\": %1,", settings.m_Economy.m_iWarLevel7Score));
		lines.Insert(string.Format("    \"warLevel8Score\": %1,", settings.m_Economy.m_iWarLevel8Score));
		lines.Insert(string.Format("    \"warLevel9Score\": %1,", settings.m_Economy.m_iWarLevel9Score));
		lines.Insert(string.Format("    \"warLevel10Score\": %1,", settings.m_Economy.m_iWarLevel10Score));
		lines.Insert("    \"_comment_victoryControlPercent\": \"Legacy territory-control victory threshold. Used only when legacy control victory is enabled.\",");
		lines.Insert(string.Format("    \"victoryControlPercent\": %1,", settings.m_Economy.m_iVictoryControlPercent));
		lines.Insert("    \"_comment_populationOutcomeEnabled\": \"Use population support and population-loss rules for win and loss checks.\",");
		lines.Insert(string.Format("    \"populationOutcomeEnabled\": %1,", JsonBool(settings.m_Economy.m_bPopulationOutcomeEnabled)));
		lines.Insert("    \"_comment_victoryPopulationSupportPercent\": \"Minimum share of remaining town population that must support the resistance for population victory.\",");
		lines.Insert(string.Format("    \"victoryPopulationSupportPercent\": %1,", settings.m_Economy.m_iVictoryPopulationSupportPercent));
		lines.Insert("    \"_comment_legacyControlVictoryEnabled\": \"Enables the older control-percent victory path.\",");
		lines.Insert(string.Format("    \"legacyControlVictoryEnabled\": %1,", JsonBool(settings.m_Economy.m_bLegacyControlVictoryEnabled)));
		lines.Insert("    \"_comment_victoryRequiresAirfields\": \"When true, population victory also requires every airfield to be resistance controlled.\",");
		lines.Insert(string.Format("    \"victoryRequiresAirfields\": %1,", JsonBool(settings.m_Economy.m_bVictoryRequiresAirfields)));
		lines.Insert("    \"_comment_victoryRequiresSeaports\": \"When true, population victory also requires every seaport to be resistance controlled.\",");
		lines.Insert(string.Format("    \"victoryRequiresSeaports\": %1,", JsonBool(settings.m_Economy.m_bVictoryRequiresSeaports)));
		lines.Insert("    \"_comment_lossConditionEnabled\": \"Enables campaign loss checks.\",");
		lines.Insert(string.Format("    \"lossConditionEnabled\": %1,", JsonBool(settings.m_Economy.m_bLossConditionEnabled)));
		lines.Insert("    \"_comment_lossHRThreshold\": \"Optional loss threshold for resistance human resources. Zero disables this threshold.\",");
		lines.Insert(string.Format("    \"lossHRThreshold\": %1,", settings.m_Economy.m_iLossHRThreshold));
		lines.Insert("    \"_comment_lossMoneyThreshold\": \"Optional loss threshold for resistance money. Zero disables this threshold.\",");
		lines.Insert(string.Format("    \"lossMoneyThreshold\": %1,", settings.m_Economy.m_iLossMoneyThreshold));
		lines.Insert("    \"_comment_lossPetrosDeathLimit\": \"Number of Petros deaths allowed before Petros-death loss pressure can end the campaign.\",");
		lines.Insert(string.Format("    \"lossPetrosDeathLimit\": %1,", settings.m_Economy.m_iLossPetrosDeathLimit));
		lines.Insert("    \"_comment_lossGraceSeconds\": \"Seconds after campaign start before optional collapse loss thresholds can apply.\",");
		lines.Insert(string.Format("    \"lossGraceSeconds\": %1,", settings.m_Economy.m_iLossGraceSeconds));
		lines.Insert("    \"_comment_enemyAttackIncomeWarPercent\": \"Percent of current war level added to enemy attack pools on each income tick.\",");
		lines.Insert(string.Format("    \"enemyAttackIncomeWarPercent\": %1,", settings.m_Economy.m_iEnemyAttackIncomeWarPercent));
		lines.Insert("    \"_comment_enemySupportIncomeWarPercent\": \"Percent of current war level added to enemy support pools on each income tick.\",");
		lines.Insert(string.Format("    \"enemySupportIncomeWarPercent\": %1,", settings.m_Economy.m_iEnemySupportIncomeWarPercent));
		lines.Insert("    \"_comment_aggressionDecayIntervalSeconds\": \"Seconds between passive global aggression decay checks.\",");
		lines.Insert(string.Format("    \"aggressionDecayIntervalSeconds\": %1,", settings.m_Economy.m_iAggressionDecayIntervalSeconds));
		lines.Insert("    \"_comment_aggressionDecayAmount\": \"Amount of global aggression removed on each decay tick.\",");
		lines.Insert(string.Format("    \"aggressionDecayAmount\": %1", settings.m_Economy.m_iAggressionDecayAmount));
		lines.Insert("  },");
		lines.Insert("  \"capture\": {");
		lines.Insert("    \"_comment\": \"Zone capture progress and counterattack tuning.\",");
		lines.Insert("    \"_comment_captureProgressRequired\": \"Capture progress needed before ownership can flip.\",");
		lines.Insert(string.Format("    \"captureProgressRequired\": %1,", settings.m_Capture.m_iProgressRequired));
		lines.Insert("    \"_comment_captureProgressPerSecond\": \"Progress gained per second while valid resistance pressure is present.\",");
		lines.Insert(string.Format("    \"captureProgressPerSecond\": %1,", settings.m_Capture.m_iProgressPerSecond));
		lines.Insert("    \"_comment_captureDecayPerSecond\": \"Progress lost per second when capture pressure is absent.\",");
		lines.Insert(string.Format("    \"captureDecayPerSecond\": %1,", settings.m_Capture.m_iDecayPerSecond));
		lines.Insert("    \"_comment_captureAggressionBase\": \"Global aggression added by a completed capture.\",");
		lines.Insert(string.Format("    \"captureAggressionBase\": %1,", settings.m_Capture.m_iAggressionBase));
		lines.Insert("    \"_comment_captureCounterattackChancePercent\": \"Percent chance that capture pressure queues enemy counterattack pressure.\",");
		lines.Insert(string.Format("    \"captureCounterattackChancePercent\": %1", settings.m_Capture.m_iCounterattackChancePercent));
		lines.Insert("  },");
		lines.Insert("  \"world\": {");
		lines.Insert("    \"_comment\": \"Runtime activation, render bubble, mission target search, and default mission duration.\",");
		lines.Insert("    \"_comment_activationRadiusMeters\": \"Distance from players where nearby strategic zones become physically active.\",");
		lines.Insert(string.Format("    \"activationRadiusMeters\": %1,", settings.m_World.m_iActivationRadiusMeters));
		lines.Insert("    \"_comment_deactivationRadiusMeters\": \"Distance from players where inactive-zone runtime entities can fold back or despawn.\",");
		lines.Insert(string.Format("    \"deactivationRadiusMeters\": %1,", settings.m_World.m_iDeactivationRadiusMeters));
		lines.Insert("    \"_comment_playerRenderBubbleRadiusMeters\": \"Radius used for player-near runtime mission and event decisions.\",");
		lines.Insert(string.Format("    \"playerRenderBubbleRadiusMeters\": %1,", settings.m_World.m_iPlayerRenderBubbleRadiusMeters));
		lines.Insert("    \"_comment_missionSelectionRadiusMeters\": \"Maximum distance for random category mission target selection near players.\",");
		lines.Insert(string.Format("    \"missionSelectionRadiusMeters\": %1,", settings.m_World.m_iMissionSelectionRadiusMeters));
		lines.Insert("    \"_comment_missionDefaultDurationSeconds\": \"Default mission expiry duration when a mission does not override it.\",");
		lines.Insert(string.Format("    \"missionDefaultDurationSeconds\": %1", settings.m_World.m_iMissionDefaultDurationSeconds));
		lines.Insert("  },");
		lines.Insert("  \"membership\": {");
		lines.Insert("    \"_comment\": \"Command-menu access and runtime admin grants.\",");
		lines.Insert("    \"_comment_membershipEnabled\": \"When true, membership and commander roles gate campaign actions.\",");
		lines.Insert(string.Format("    \"membershipEnabled\": %1,", JsonBool(settings.m_Membership.m_bMembershipEnabled)));
		lines.Insert("    \"_comment_guestsCanOpenMenu\": \"When true, non-members can open the menu for read-only or permitted actions.\",");
		lines.Insert(string.Format("    \"guestsCanOpenMenu\": %1,", JsonBool(settings.m_Membership.m_bGuestsCanOpenMenu)));
		lines.Insert("    \"_comment_adminIdentityIds\": \"SteamID64 strings that should receive admin permissions at runtime.\",");
		lines.Insert(string.Format("    \"adminIdentityIds\": %1", BuildStringArray(settings.m_Membership.m_aAdminIdentityIds)));
		lines.Insert("  },");
		lines.Insert("  \"arsenalLoot\": {");
		lines.Insert("    \"_comment\": \"Area loot collection and arsenal unlock behavior.\",");
		lines.Insert("    \"_comment_arsenalUnlockThreshold\": \"Number of recovered matching items needed before that item becomes unlimited.\",");
		lines.Insert(string.Format("    \"arsenalUnlockThreshold\": %1,", settings.m_ArsenalLoot.m_iArsenalUnlockThreshold));
		lines.Insert("    \"_comment_magazineUnlockMultiplier\": \"Magazine unlock threshold multiplier relative to the base item threshold.\",");
		lines.Insert(string.Format("    \"magazineUnlockMultiplier\": %1,", settings.m_ArsenalLoot.m_iMagazineUnlockMultiplier));
		lines.Insert("    \"_comment_hqInteractionRadiusMeters\": \"Maximum distance from HQ arsenal for HQ arsenal actions.\",");
		lines.Insert(string.Format("    \"hqInteractionRadiusMeters\": %1,", settings.m_ArsenalLoot.m_iHQInteractionRadiusMeters));
		lines.Insert("    \"_comment_lootRadiusMeters\": \"Radius scanned around the player for area loot collection.\",");
		lines.Insert(string.Format("    \"lootRadiusMeters\": %1,", settings.m_ArsenalLoot.m_iLootRadiusMeters));
		lines.Insert("    \"_comment_lootOnlyLockedItems\": \"When true, area loot skips items that are already unlocked in the arsenal.\",");
		lines.Insert(string.Format("    \"lootOnlyLockedItems\": %1,", JsonBool(settings.m_ArsenalLoot.m_bLootOnlyLockedItems)));
		lines.Insert("    \"_comment_removeLootedItems\": \"When true, transferred area-loot items are removed from their source containers.\",");
		lines.Insert(string.Format("    \"removeLootedItems\": %1,", JsonBool(settings.m_ArsenalLoot.m_bRemoveLootedItems)));
		lines.Insert("    \"_comment_allowExplosiveUnlocks\": \"When true, explosive weapons can become unlimited through loot thresholds.\",");
		lines.Insert(string.Format("    \"allowExplosiveUnlocks\": %1,", JsonBool(settings.m_ArsenalLoot.m_bAllowExplosiveUnlocks)));
		lines.Insert("    \"_comment_allowGuidedLauncherUnlocks\": \"When true, guided launchers can become unlimited through loot thresholds.\",");
		lines.Insert(string.Format("    \"allowGuidedLauncherUnlocks\": %1", JsonBool(settings.m_ArsenalLoot.m_bAllowGuidedLauncherUnlocks)));
		lines.Insert("  },");
		lines.Insert("  \"vehicleLoot\": {");
		lines.Insert("    \"_comment\": \"Vehicle cargo looting into campaign storage.\",");
		lines.Insert("    \"_comment_vehicleLootEnabled\": \"Enables nearby vehicle cargo collection.\",");
		lines.Insert(string.Format("    \"vehicleLootEnabled\": %1,", JsonBool(settings.m_VehicleLoot.m_bEnabled)));
		lines.Insert("    \"_comment_vehicleLootRadiusMeters\": \"Radius scanned around the player for eligible vehicles.\",");
		lines.Insert(string.Format("    \"vehicleLootRadiusMeters\": %1,", settings.m_VehicleLoot.m_iRadiusMeters));
		lines.Insert("    \"_comment_vehicleLootOnlyLockedItems\": \"When true, vehicle loot skips items already unlocked in the arsenal.\",");
		lines.Insert(string.Format("    \"vehicleLootOnlyLockedItems\": %1,", JsonBool(settings.m_VehicleLoot.m_bOnlyLockedItems)));
		lines.Insert("    \"_comment_vehicleLootRemoveSourceItems\": \"When true, transferred vehicle-cargo items are removed from the source vehicle.\",");
		lines.Insert(string.Format("    \"vehicleLootRemoveSourceItems\": %1,", JsonBool(settings.m_VehicleLoot.m_bRemoveSourceItems)));
		lines.Insert("    \"_comment_vehicleLootMaxItemsPerAction\": \"Maximum cargo items moved by one vehicle-loot action.\",");
		lines.Insert(string.Format("    \"vehicleLootMaxItemsPerAction\": %1", settings.m_VehicleLoot.m_iMaxItemsPerAction));
		lines.Insert("  },");
		lines.Insert("  \"airSupport\": {");
		lines.Insert("    \"_comment\": \"Player-requested resistance air support availability and cooldown.\",");
		lines.Insert("    \"_comment_airSupportEnabled\": \"Enables resistance air support requests.\",");
		lines.Insert(string.Format("    \"airSupportEnabled\": %1,", JsonBool(settings.m_AirSupport.m_bEnabled)));
		lines.Insert("    \"_comment_airSupportCooldownSeconds\": \"Seconds before another resistance air support request can be made.\",");
		lines.Insert(string.Format("    \"airSupportCooldownSeconds\": %1", settings.m_AirSupport.m_iCooldownSeconds));
		lines.Insert("  },");
		lines.Insert("  \"civilians\": {");
		lines.Insert("    \"_comment\": \"Civilian population and ambient vehicle caps used by town runtime systems.\",");
		lines.Insert("    \"_comment_civilianPopulationEnabled\": \"Enables civilian population and town support simulation.\",");
		lines.Insert(string.Format("    \"civilianPopulationEnabled\": %1,", JsonBool(settings.m_Civilians.m_bEnabled)));
		lines.Insert("    \"_comment_civilianMaxActivePerTown\": \"Maximum active civilian characters per town when the town is active.\",");
		lines.Insert(string.Format("    \"civilianMaxActivePerTown\": %1,", settings.m_Civilians.m_iMaxActivePerTown));
		lines.Insert("    \"_comment_civilianVehicleMinPerTown\": \"Minimum civilian vehicles spawned or tracked per active town.\",");
		lines.Insert(string.Format("    \"civilianVehicleMinPerTown\": %1,", settings.m_Civilians.m_iCivilianVehicleMinPerTown));
		lines.Insert("    \"_comment_civilianVehicleMaxPerTown\": \"Maximum civilian vehicles spawned or tracked per active town.\",");
		lines.Insert(string.Format("    \"civilianVehicleMaxPerTown\": %1,", settings.m_Civilians.m_iCivilianVehicleMaxPerTown));
		lines.Insert("    \"_comment_civilianDrivingVehicleCountPerTown\": \"Number of active civilian-driven traffic vehicles per active town. These vehicles despawn with their drivers after leaving the player render bubble.\",");
		lines.Insert(string.Format("    \"civilianDrivingVehicleCountPerTown\": %1,", settings.m_Civilians.m_iCivilianDrivingVehicleCountPerTown));
		lines.Insert("    \"_comment_occupierVehicleMinPerTown\": \"Minimum occupier security vehicles associated with active towns.\",");
		lines.Insert(string.Format("    \"occupierVehicleMinPerTown\": %1,", settings.m_Civilians.m_iOccupierVehicleMinPerTown));
		lines.Insert("    \"_comment_occupierVehicleMaxPerTown\": \"Maximum occupier security vehicles associated with active towns.\",");
		lines.Insert(string.Format("    \"occupierVehicleMaxPerTown\": %1", settings.m_Civilians.m_iOccupierVehicleMaxPerTown));
		lines.Insert("  },");
		lines.Insert("  \"persistence\": {");
		lines.Insert("    \"_comment\": \"Autosave timing and save debounce behavior.\",");
		lines.Insert("    \"_comment_autosaveIntervalSeconds\": \"Seconds between automatic campaign save attempts.\",");
		lines.Insert(string.Format("    \"autosaveIntervalSeconds\": %1,", settings.m_Persistence.m_iAutosaveIntervalSeconds));
		lines.Insert("    \"_comment_majorChangeDebounceSeconds\": \"Minimum seconds between saves caused by major campaign changes.\",");
		lines.Insert(string.Format("    \"majorChangeDebounceSeconds\": %1", settings.m_Persistence.m_iMajorChangeDebounceSeconds));
		lines.Insert("  },");
		lines.Insert("  \"debug\": {");
		lines.Insert("    \"_comment\": \"Debug menu and logging controls.\",");
		lines.Insert("    \"_comment_debugMenuEnabled\": \"Enables the Admin debug tab and campaign debug actions for admins.\",");
		lines.Insert(string.Format("    \"debugMenuEnabled\": %1,", JsonBool(settings.m_Debug.m_bDebugMenuEnabled)));
		lines.Insert("    \"_comment_debugLoggingEnabled\": \"Enables verbose h-istasi diagnostic logging from runtime systems that check this setting.\",");
		lines.Insert(string.Format("    \"debugLoggingEnabled\": %1", JsonBool(settings.m_Debug.m_bDebugLoggingEnabled)));
		lines.Insert("  },");
		lines.Insert("  \"features\": {");
		lines.Insert("    \"_comment\": \"Runtime feature toggles for broad-alpha systems.\",");
		lines.Insert("    \"_comment_physicalWarEnabled\": \"Enables physical support, enemy response, mission, and garrison runtime spawning.\",");
		lines.Insert(string.Format("    \"physicalWarEnabled\": %1,", JsonBool(settings.m_Features.m_bPhysicalWarEnabled)));
		lines.Insert("    \"_comment_areaLootEnabled\": \"Enables nearby area loot collection into the campaign arsenal.\",");
		lines.Insert(string.Format("    \"areaLootEnabled\": %1,", JsonBool(settings.m_Features.m_bAreaLootEnabled)));
		lines.Insert("    \"_comment_setupUiReadOnly\": \"When true, setup UI reports config values without trying to edit the file in game.\",");
		lines.Insert(string.Format("    \"setupUiReadOnly\": %1,", JsonBool(settings.m_Features.m_bSetupUiReadOnly)));
		lines.Insert("    \"_comment_gameMasterBudgetsEnabled\": \"When false, h-istasi disables Game Master placement budget caps at runtime.\",");
		lines.Insert(string.Format("    \"gameMasterBudgetsEnabled\": %1,", JsonBool(settings.m_Features.m_bGameMasterBudgetsEnabled)));
		lines.Insert("    \"_comment_showPlayerMapMarkers\": \"Shows h-istasi player-facing campaign markers on the map.\",");
		lines.Insert(string.Format("    \"showPlayerMapMarkers\": %1,", JsonBool(settings.m_Features.m_bShowPlayerMapMarkers)));
		lines.Insert("    \"_comment_infiniteStaminaEnabled\": \"Refills local player stamina and suppresses sprint exhaustion visual effects.\",");
		lines.Insert(string.Format("    \"infiniteStaminaEnabled\": %1,", JsonBool(settings.m_Features.m_bInfiniteStaminaEnabled)));
		lines.Insert("    \"_comment_trackResistanceSupportGroupsOnMap\": \"Keeps live markers on player-requested resistance support groups until they end.\",");
		lines.Insert(string.Format("    \"trackResistanceSupportGroupsOnMap\": %1", JsonBool(settings.m_Features.m_bTrackResistanceSupportGroupsOnMap)));
		lines.Insert("  }");
		lines.Insert("}");
		WriteLines(SETTINGS_FILE, lines);
	}

	protected string JsonBool(bool value)
	{
		if (value)
			return "true";

		return "false";
	}

	protected string BuildStringArray(notnull array<string> values)
	{
		string output = "[";
		for (int i = 0; i < values.Count(); i++)
		{
			if (i > 0)
				output = output + ", ";

			output = output + "\"" + values[i] + "\"";
		}

		return output + "]";
	}

	protected array<string> ReadLines(string fileName)
	{
		array<string> lines = {};
		FileHandle file = FileIO.OpenFile(fileName, FileMode.READ);
		if (!file)
			return lines;

		string line;
		while (file.ReadLine(line) >= 0)
			lines.Insert(line);

		file.Close();
		return lines;
	}

	protected bool WriteLines(string fileName, notnull array<string> lines)
	{
		FileHandle file = FileIO.OpenFile(fileName, FileMode.WRITE);
		if (!file)
			return false;

		foreach (string line : lines)
			file.WriteLine(line);

		file.Close();
		return true;
	}
}
