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
			ApplyString(line, "defaultHideoutId", settings.m_Campaign.m_sDefaultHideoutId);
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
		}
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

		target.Clear();
		int open = line.IndexOf("[");
		int close = line.IndexOf("]");
		if (open < 0 || close <= open)
			return;

		string values = line.Substring(open + 1, close - open - 1);
		values.Replace("\"", "");
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
		lines.Insert(string.Format("  \"schemaVersion\": %1,", settings.m_iSchemaVersion));
		lines.Insert("  \"campaign\": {");
		lines.Insert(string.Format("    \"presetId\": \"%1\",", settings.m_Campaign.m_sPresetId));
		lines.Insert(string.Format("    \"campaignSeed\": %1,", settings.m_Campaign.m_iCampaignSeed));
		lines.Insert(string.Format("    \"defaultHideoutId\": \"%1\"", settings.m_Campaign.m_sDefaultHideoutId));
		lines.Insert("  },");
		lines.Insert("  \"factions\": {");
		lines.Insert(string.Format("    \"resistanceFactionKey\": \"%1\",", settings.m_Factions.m_sResistanceFactionKey));
		lines.Insert(string.Format("    \"occupierFactionKey\": \"%1\",", settings.m_Factions.m_sOccupierFactionKey));
		lines.Insert(string.Format("    \"invaderFactionKey\": \"%1\"", settings.m_Factions.m_sInvaderFactionKey));
		lines.Insert("  },");
		lines.Insert("  \"economy\": {");
		lines.Insert(string.Format("    \"startingFactionMoney\": %1,", settings.m_Economy.m_iStartingFactionMoney));
		lines.Insert(string.Format("    \"startingHR\": %1,", settings.m_Economy.m_iStartingHR));
		lines.Insert(string.Format("    \"startingTrainingLevel\": %1,", settings.m_Economy.m_iStartingTrainingLevel));
		lines.Insert(string.Format("    \"startingOccupierAttackPool\": %1,", settings.m_Economy.m_iStartingOccupierAttackPool));
		lines.Insert(string.Format("    \"startingOccupierSupportPool\": %1,", settings.m_Economy.m_iStartingOccupierSupportPool));
		lines.Insert(string.Format("    \"startingInvaderAttackPool\": %1,", settings.m_Economy.m_iStartingInvaderAttackPool));
		lines.Insert(string.Format("    \"startingInvaderSupportPool\": %1,", settings.m_Economy.m_iStartingInvaderSupportPool));
		lines.Insert(string.Format("    \"zoneIncomeIntervalSeconds\": %1,", settings.m_Economy.m_iZoneIncomeIntervalSeconds));
		lines.Insert(string.Format("    \"warLevelMaximum\": %1,", settings.m_Economy.m_iWarLevelMaximum));
		lines.Insert(string.Format("    \"warLevel2Score\": %1,", settings.m_Economy.m_iWarLevel2Score));
		lines.Insert(string.Format("    \"warLevel3Score\": %1,", settings.m_Economy.m_iWarLevel3Score));
		lines.Insert(string.Format("    \"warLevel4Score\": %1,", settings.m_Economy.m_iWarLevel4Score));
		lines.Insert(string.Format("    \"warLevel5Score\": %1,", settings.m_Economy.m_iWarLevel5Score));
		lines.Insert(string.Format("    \"warLevel6Score\": %1,", settings.m_Economy.m_iWarLevel6Score));
		lines.Insert(string.Format("    \"warLevel7Score\": %1,", settings.m_Economy.m_iWarLevel7Score));
		lines.Insert(string.Format("    \"warLevel8Score\": %1,", settings.m_Economy.m_iWarLevel8Score));
		lines.Insert(string.Format("    \"warLevel9Score\": %1,", settings.m_Economy.m_iWarLevel9Score));
		lines.Insert(string.Format("    \"warLevel10Score\": %1,", settings.m_Economy.m_iWarLevel10Score));
		lines.Insert(string.Format("    \"victoryControlPercent\": %1,", settings.m_Economy.m_iVictoryControlPercent));
		lines.Insert(string.Format("    \"victoryRequiresAirfields\": %1,", JsonBool(settings.m_Economy.m_bVictoryRequiresAirfields)));
		lines.Insert(string.Format("    \"victoryRequiresSeaports\": %1,", JsonBool(settings.m_Economy.m_bVictoryRequiresSeaports)));
		lines.Insert(string.Format("    \"lossConditionEnabled\": %1,", JsonBool(settings.m_Economy.m_bLossConditionEnabled)));
		lines.Insert(string.Format("    \"lossHRThreshold\": %1,", settings.m_Economy.m_iLossHRThreshold));
		lines.Insert(string.Format("    \"lossMoneyThreshold\": %1,", settings.m_Economy.m_iLossMoneyThreshold));
		lines.Insert(string.Format("    \"lossPetrosDeathLimit\": %1,", settings.m_Economy.m_iLossPetrosDeathLimit));
		lines.Insert(string.Format("    \"lossGraceSeconds\": %1,", settings.m_Economy.m_iLossGraceSeconds));
		lines.Insert(string.Format("    \"enemyAttackIncomeWarPercent\": %1,", settings.m_Economy.m_iEnemyAttackIncomeWarPercent));
		lines.Insert(string.Format("    \"enemySupportIncomeWarPercent\": %1,", settings.m_Economy.m_iEnemySupportIncomeWarPercent));
		lines.Insert(string.Format("    \"aggressionDecayIntervalSeconds\": %1,", settings.m_Economy.m_iAggressionDecayIntervalSeconds));
		lines.Insert(string.Format("    \"aggressionDecayAmount\": %1", settings.m_Economy.m_iAggressionDecayAmount));
		lines.Insert("  },");
		lines.Insert("  \"capture\": {");
		lines.Insert(string.Format("    \"captureProgressRequired\": %1,", settings.m_Capture.m_iProgressRequired));
		lines.Insert(string.Format("    \"captureProgressPerSecond\": %1,", settings.m_Capture.m_iProgressPerSecond));
		lines.Insert(string.Format("    \"captureDecayPerSecond\": %1,", settings.m_Capture.m_iDecayPerSecond));
		lines.Insert(string.Format("    \"captureAggressionBase\": %1,", settings.m_Capture.m_iAggressionBase));
		lines.Insert(string.Format("    \"captureCounterattackChancePercent\": %1", settings.m_Capture.m_iCounterattackChancePercent));
		lines.Insert("  },");
		lines.Insert("  \"world\": {");
		lines.Insert(string.Format("    \"activationRadiusMeters\": %1,", settings.m_World.m_iActivationRadiusMeters));
		lines.Insert(string.Format("    \"deactivationRadiusMeters\": %1,", settings.m_World.m_iDeactivationRadiusMeters));
		lines.Insert(string.Format("    \"missionDefaultDurationSeconds\": %1", settings.m_World.m_iMissionDefaultDurationSeconds));
		lines.Insert("  },");
		lines.Insert("  \"membership\": {");
		lines.Insert(string.Format("    \"membershipEnabled\": %1,", JsonBool(settings.m_Membership.m_bMembershipEnabled)));
		lines.Insert(string.Format("    \"guestsCanOpenMenu\": %1,", JsonBool(settings.m_Membership.m_bGuestsCanOpenMenu)));
		lines.Insert(string.Format("    \"adminIdentityIds\": %1", BuildStringArray(settings.m_Membership.m_aAdminIdentityIds)));
		lines.Insert("  },");
		lines.Insert("  \"arsenalLoot\": {");
		lines.Insert(string.Format("    \"arsenalUnlockThreshold\": %1,", settings.m_ArsenalLoot.m_iArsenalUnlockThreshold));
		lines.Insert(string.Format("    \"magazineUnlockMultiplier\": %1,", settings.m_ArsenalLoot.m_iMagazineUnlockMultiplier));
		lines.Insert(string.Format("    \"hqInteractionRadiusMeters\": %1,", settings.m_ArsenalLoot.m_iHQInteractionRadiusMeters));
		lines.Insert(string.Format("    \"lootRadiusMeters\": %1,", settings.m_ArsenalLoot.m_iLootRadiusMeters));
		lines.Insert(string.Format("    \"lootOnlyLockedItems\": %1,", JsonBool(settings.m_ArsenalLoot.m_bLootOnlyLockedItems)));
		lines.Insert(string.Format("    \"removeLootedItems\": %1,", JsonBool(settings.m_ArsenalLoot.m_bRemoveLootedItems)));
		lines.Insert(string.Format("    \"allowExplosiveUnlocks\": %1,", JsonBool(settings.m_ArsenalLoot.m_bAllowExplosiveUnlocks)));
		lines.Insert(string.Format("    \"allowGuidedLauncherUnlocks\": %1", JsonBool(settings.m_ArsenalLoot.m_bAllowGuidedLauncherUnlocks)));
		lines.Insert("  },");
		lines.Insert("  \"vehicleLoot\": {");
		lines.Insert(string.Format("    \"vehicleLootEnabled\": %1,", JsonBool(settings.m_VehicleLoot.m_bEnabled)));
		lines.Insert(string.Format("    \"vehicleLootRadiusMeters\": %1,", settings.m_VehicleLoot.m_iRadiusMeters));
		lines.Insert(string.Format("    \"vehicleLootOnlyLockedItems\": %1,", JsonBool(settings.m_VehicleLoot.m_bOnlyLockedItems)));
		lines.Insert(string.Format("    \"vehicleLootRemoveSourceItems\": %1,", JsonBool(settings.m_VehicleLoot.m_bRemoveSourceItems)));
		lines.Insert(string.Format("    \"vehicleLootMaxItemsPerAction\": %1", settings.m_VehicleLoot.m_iMaxItemsPerAction));
		lines.Insert("  },");
		lines.Insert("  \"airSupport\": {");
		lines.Insert(string.Format("    \"airSupportEnabled\": %1,", JsonBool(settings.m_AirSupport.m_bEnabled)));
		lines.Insert(string.Format("    \"airSupportCooldownSeconds\": %1", settings.m_AirSupport.m_iCooldownSeconds));
		lines.Insert("  },");
		lines.Insert("  \"civilians\": {");
		lines.Insert(string.Format("    \"civilianPopulationEnabled\": %1,", JsonBool(settings.m_Civilians.m_bEnabled)));
		lines.Insert(string.Format("    \"civilianMaxActivePerTown\": %1,", settings.m_Civilians.m_iMaxActivePerTown));
		lines.Insert(string.Format("    \"civilianVehicleMinPerTown\": %1,", settings.m_Civilians.m_iCivilianVehicleMinPerTown));
		lines.Insert(string.Format("    \"civilianVehicleMaxPerTown\": %1,", settings.m_Civilians.m_iCivilianVehicleMaxPerTown));
		lines.Insert(string.Format("    \"occupierVehicleMinPerTown\": %1,", settings.m_Civilians.m_iOccupierVehicleMinPerTown));
		lines.Insert(string.Format("    \"occupierVehicleMaxPerTown\": %1", settings.m_Civilians.m_iOccupierVehicleMaxPerTown));
		lines.Insert("  },");
		lines.Insert("  \"persistence\": {");
		lines.Insert(string.Format("    \"autosaveIntervalSeconds\": %1,", settings.m_Persistence.m_iAutosaveIntervalSeconds));
		lines.Insert(string.Format("    \"majorChangeDebounceSeconds\": %1", settings.m_Persistence.m_iMajorChangeDebounceSeconds));
		lines.Insert("  },");
		lines.Insert("  \"debug\": {");
		lines.Insert(string.Format("    \"debugMenuEnabled\": %1,", JsonBool(settings.m_Debug.m_bDebugMenuEnabled)));
		lines.Insert(string.Format("    \"debugLoggingEnabled\": %1", JsonBool(settings.m_Debug.m_bDebugLoggingEnabled)));
		lines.Insert("  },");
		lines.Insert("  \"features\": {");
		lines.Insert(string.Format("    \"physicalWarEnabled\": %1,", JsonBool(settings.m_Features.m_bPhysicalWarEnabled)));
		lines.Insert(string.Format("    \"areaLootEnabled\": %1,", JsonBool(settings.m_Features.m_bAreaLootEnabled)));
		lines.Insert(string.Format("    \"setupUiReadOnly\": %1,", JsonBool(settings.m_Features.m_bSetupUiReadOnly)));
		lines.Insert(string.Format("    \"gameMasterBudgetsEnabled\": %1,", JsonBool(settings.m_Features.m_bGameMasterBudgetsEnabled)));
		lines.Insert(string.Format("    \"showPlayerMapMarkers\": %1", JsonBool(settings.m_Features.m_bShowPlayerMapMarkers)));
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
