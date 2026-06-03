class HST_RuntimeSettingsService
{
	static const string SETTINGS_DIRECTORY = "$profile:h-istasi";
	static const string SETTINGS_FILE = "$profile:h-istasi/HST_Settings.json";

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
			ApplyInt(line, "startingOccupierAttackPool", settings.m_Economy.m_iStartingOccupierAttackPool);
			ApplyInt(line, "startingOccupierSupportPool", settings.m_Economy.m_iStartingOccupierSupportPool);
			ApplyInt(line, "startingInvaderAttackPool", settings.m_Economy.m_iStartingInvaderAttackPool);
			ApplyInt(line, "startingInvaderSupportPool", settings.m_Economy.m_iStartingInvaderSupportPool);
			ApplyInt(line, "zoneIncomeIntervalSeconds", settings.m_Economy.m_iZoneIncomeIntervalSeconds);
			ApplyInt(line, "warLevelMaximum", settings.m_Economy.m_iWarLevelMaximum);
			ApplyInt(line, "activationRadiusMeters", settings.m_World.m_iActivationRadiusMeters);
			ApplyInt(line, "deactivationRadiusMeters", settings.m_World.m_iDeactivationRadiusMeters);
			ApplyInt(line, "missionDefaultDurationSeconds", settings.m_World.m_iMissionDefaultDurationSeconds);
			ApplyBool(line, "membershipEnabled", settings.m_Membership.m_bMembershipEnabled);
			ApplyBool(line, "guestsCanOpenMenu", settings.m_Membership.m_bGuestsCanOpenMenu);
			ApplyStringArray(line, "adminIdentityIds", settings.m_Membership.m_aAdminIdentityIds);
			ApplyInt(line, "arsenalUnlockThreshold", settings.m_ArsenalLoot.m_iArsenalUnlockThreshold);
			ApplyInt(line, "magazineUnlockMultiplier", settings.m_ArsenalLoot.m_iMagazineUnlockMultiplier);
			ApplyInt(line, "lootRadiusMeters", settings.m_ArsenalLoot.m_iLootRadiusMeters);
			ApplyBool(line, "lootOnlyLockedItems", settings.m_ArsenalLoot.m_bLootOnlyLockedItems);
			ApplyBool(line, "removeLootedItems", settings.m_ArsenalLoot.m_bRemoveLootedItems);
			ApplyBool(line, "allowExplosiveUnlocks", settings.m_ArsenalLoot.m_bAllowExplosiveUnlocks);
			ApplyBool(line, "allowGuidedLauncherUnlocks", settings.m_ArsenalLoot.m_bAllowGuidedLauncherUnlocks);
			ApplyInt(line, "autosaveIntervalSeconds", settings.m_Persistence.m_iAutosaveIntervalSeconds);
			ApplyInt(line, "majorChangeDebounceSeconds", settings.m_Persistence.m_iMajorChangeDebounceSeconds);
			ApplyBool(line, "debugMenuEnabled", settings.m_Debug.m_bDebugMenuEnabled);
			ApplyBool(line, "verboseLogging", settings.m_Debug.m_bVerboseLogging);
			ApplyBool(line, "physicalWarEnabled", settings.m_Features.m_bPhysicalWarEnabled);
			ApplyBool(line, "areaLootEnabled", settings.m_Features.m_bAreaLootEnabled);
		}
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

	protected void WriteDefault(notnull HST_RuntimeSettings settings)
	{
		array<string> lines = {};
		lines.Insert("{");
		lines.Insert("  \"schemaVersion\": 1,");
		lines.Insert("  \"campaign\": {");
		lines.Insert("    \"presetId\": \"rhs_everon\",");
		lines.Insert("    \"campaignSeed\": 1985,");
		lines.Insert("    \"defaultHideoutId\": \"hideout_central_hills\"");
		lines.Insert("  },");
		lines.Insert("  \"factions\": {");
		lines.Insert("    \"resistanceFactionKey\": \"FIA\",");
		lines.Insert("    \"occupierFactionKey\": \"RHS_USAF\",");
		lines.Insert("    \"invaderFactionKey\": \"RHS_AFRF\"");
		lines.Insert("  },");
		lines.Insert("  \"economy\": {");
		lines.Insert("    \"startingFactionMoney\": 1000,");
		lines.Insert("    \"startingHR\": 20,");
		lines.Insert("    \"startingOccupierAttackPool\": 100,");
		lines.Insert("    \"startingOccupierSupportPool\": 100,");
		lines.Insert("    \"startingInvaderAttackPool\": 60,");
		lines.Insert("    \"startingInvaderSupportPool\": 60,");
		lines.Insert("    \"zoneIncomeIntervalSeconds\": 600,");
		lines.Insert("    \"warLevelMaximum\": 10");
		lines.Insert("  },");
		lines.Insert("  \"world\": {");
		lines.Insert("    \"activationRadiusMeters\": 1200,");
		lines.Insert("    \"deactivationRadiusMeters\": 1600,");
		lines.Insert("    \"missionDefaultDurationSeconds\": 3600");
		lines.Insert("  },");
		lines.Insert("  \"membership\": {");
		lines.Insert("    \"membershipEnabled\": true,");
		lines.Insert("    \"guestsCanOpenMenu\": false,");
		lines.Insert("    \"adminIdentityIds\": []");
		lines.Insert("  },");
		lines.Insert("  \"arsenalLoot\": {");
		lines.Insert("    \"arsenalUnlockThreshold\": 15,");
		lines.Insert("    \"magazineUnlockMultiplier\": 3,");
		lines.Insert("    \"lootRadiusMeters\": 15,");
		lines.Insert("    \"lootOnlyLockedItems\": true,");
		lines.Insert("    \"removeLootedItems\": true,");
		lines.Insert("    \"allowExplosiveUnlocks\": false,");
		lines.Insert("    \"allowGuidedLauncherUnlocks\": false");
		lines.Insert("  },");
		lines.Insert("  \"persistence\": {");
		lines.Insert("    \"autosaveIntervalSeconds\": 900,");
		lines.Insert("    \"majorChangeDebounceSeconds\": 30");
		lines.Insert("  },");
		lines.Insert("  \"debug\": {");
		lines.Insert("    \"debugMenuEnabled\": true,");
		lines.Insert("    \"verboseLogging\": false");
		lines.Insert("  },");
		lines.Insert("  \"features\": {");
		lines.Insert("    \"physicalWarEnabled\": true,");
		lines.Insert("    \"areaLootEnabled\": true,");
		lines.Insert("    \"setupUiReadOnly\": true");
		lines.Insert("  }");
		lines.Insert("}");
		WriteLines(SETTINGS_FILE, lines);
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
