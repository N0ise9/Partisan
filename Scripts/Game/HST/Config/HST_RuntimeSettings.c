class HST_RuntimeSettingsCampaign
{
	string m_sPresetId = "rhs_everon";
	int m_iCampaignSeed = 1985;
	string m_sDefaultHideoutId = "hideout_central_hills";
}

class HST_RuntimeSettingsFactions
{
	string m_sResistanceFactionKey = "FIA";
	string m_sOccupierFactionKey = "RHS_USAF";
	string m_sInvaderFactionKey = "RHS_AFRF";
}

class HST_RuntimeSettingsEconomy
{
	int m_iStartingFactionMoney = 1000;
	int m_iStartingHR = 20;
	int m_iStartingOccupierAttackPool = 100;
	int m_iStartingOccupierSupportPool = 100;
	int m_iStartingInvaderAttackPool = 60;
	int m_iStartingInvaderSupportPool = 60;
	int m_iZoneIncomeIntervalSeconds = 600;
	int m_iWarLevelMaximum = 10;
}

class HST_RuntimeSettingsWorld
{
	int m_iActivationRadiusMeters = 1200;
	int m_iDeactivationRadiusMeters = 1600;
	int m_iMissionDefaultDurationSeconds = 3600;
}

class HST_RuntimeSettingsMembership
{
	bool m_bMembershipEnabled = true;
	bool m_bGuestsCanOpenMenu = false;
	ref array<string> m_aAdminIdentityIds = {};
}

class HST_RuntimeSettingsArsenalLoot
{
	int m_iArsenalUnlockThreshold = 15;
	int m_iMagazineUnlockMultiplier = 3;
	int m_iLootRadiusMeters = 15;
	bool m_bLootOnlyLockedItems = true;
	bool m_bRemoveLootedItems = true;
	bool m_bAllowExplosiveUnlocks;
	bool m_bAllowGuidedLauncherUnlocks;
}

class HST_RuntimeSettingsPersistence
{
	int m_iAutosaveIntervalSeconds = 900;
	int m_iMajorChangeDebounceSeconds = 30;
}

class HST_RuntimeSettingsDebug
{
	bool m_bDebugMenuEnabled = true;
	bool m_bVerboseLogging;
}

class HST_RuntimeSettingsFeatures
{
	bool m_bPhysicalWarEnabled = true;
	bool m_bAreaLootEnabled = true;
	bool m_bSetupUiReadOnly = true;
}

class HST_RuntimeSettings
{
	static const int SCHEMA_VERSION = 1;

	int m_iSchemaVersion = SCHEMA_VERSION;
	ref HST_RuntimeSettingsCampaign m_Campaign = new HST_RuntimeSettingsCampaign();
	ref HST_RuntimeSettingsFactions m_Factions = new HST_RuntimeSettingsFactions();
	ref HST_RuntimeSettingsEconomy m_Economy = new HST_RuntimeSettingsEconomy();
	ref HST_RuntimeSettingsWorld m_World = new HST_RuntimeSettingsWorld();
	ref HST_RuntimeSettingsMembership m_Membership = new HST_RuntimeSettingsMembership();
	ref HST_RuntimeSettingsArsenalLoot m_ArsenalLoot = new HST_RuntimeSettingsArsenalLoot();
	ref HST_RuntimeSettingsPersistence m_Persistence = new HST_RuntimeSettingsPersistence();
	ref HST_RuntimeSettingsDebug m_Debug = new HST_RuntimeSettingsDebug();
	ref HST_RuntimeSettingsFeatures m_Features = new HST_RuntimeSettingsFeatures();

	void Normalize()
	{
		if (m_iSchemaVersion <= 0)
			m_iSchemaVersion = SCHEMA_VERSION;

		m_Economy.m_iStartingFactionMoney = Math.Max(0, m_Economy.m_iStartingFactionMoney);
		m_Economy.m_iStartingHR = Math.Max(0, m_Economy.m_iStartingHR);
		m_Economy.m_iZoneIncomeIntervalSeconds = Math.Max(30, m_Economy.m_iZoneIncomeIntervalSeconds);
		m_Economy.m_iWarLevelMaximum = Math.Max(1, m_Economy.m_iWarLevelMaximum);
		m_World.m_iActivationRadiusMeters = Math.Max(100, m_World.m_iActivationRadiusMeters);
		m_World.m_iDeactivationRadiusMeters = Math.Max(m_World.m_iActivationRadiusMeters, m_World.m_iDeactivationRadiusMeters);
		m_World.m_iMissionDefaultDurationSeconds = Math.Max(300, m_World.m_iMissionDefaultDurationSeconds);
		m_ArsenalLoot.m_iArsenalUnlockThreshold = Math.Max(0, m_ArsenalLoot.m_iArsenalUnlockThreshold);
		m_ArsenalLoot.m_iMagazineUnlockMultiplier = Math.Max(1, m_ArsenalLoot.m_iMagazineUnlockMultiplier);
		m_ArsenalLoot.m_iLootRadiusMeters = Math.Max(1, m_ArsenalLoot.m_iLootRadiusMeters);
		m_Persistence.m_iAutosaveIntervalSeconds = Math.Max(60, m_Persistence.m_iAutosaveIntervalSeconds);
		m_Persistence.m_iMajorChangeDebounceSeconds = Math.Max(1, m_Persistence.m_iMajorChangeDebounceSeconds);
	}

	void ApplyTo(notnull HST_CampaignPreset preset, notnull HST_BalanceConfig balance)
	{
		Normalize();

		preset.m_sPresetId = m_Campaign.m_sPresetId;
		preset.m_sResistanceFactionKey = m_Factions.m_sResistanceFactionKey;
		preset.m_sOccupierFactionKey = m_Factions.m_sOccupierFactionKey;
		preset.m_sInvaderFactionKey = m_Factions.m_sInvaderFactionKey;

		balance.m_iAutosaveIntervalSeconds = m_Persistence.m_iAutosaveIntervalSeconds;
		balance.m_iMajorChangeDebounceSeconds = m_Persistence.m_iMajorChangeDebounceSeconds;
		balance.m_iStartingHR = m_Economy.m_iStartingHR;
		balance.m_iStartingFactionMoney = m_Economy.m_iStartingFactionMoney;
		balance.m_iStartingOccupierAttackPool = m_Economy.m_iStartingOccupierAttackPool;
		balance.m_iStartingOccupierSupportPool = m_Economy.m_iStartingOccupierSupportPool;
		balance.m_iStartingInvaderAttackPool = m_Economy.m_iStartingInvaderAttackPool;
		balance.m_iStartingInvaderSupportPool = m_Economy.m_iStartingInvaderSupportPool;
		balance.m_iZoneIncomeIntervalSeconds = m_Economy.m_iZoneIncomeIntervalSeconds;
		balance.m_iMissionDefaultDurationSeconds = m_World.m_iMissionDefaultDurationSeconds;
		balance.m_iActivationRadiusMeters = m_World.m_iActivationRadiusMeters;
		balance.m_iDeactivationRadiusMeters = m_World.m_iDeactivationRadiusMeters;
		balance.m_iArsenalUnlockThreshold = m_ArsenalLoot.m_iArsenalUnlockThreshold;
		balance.m_iMagazineUnlockMultiplier = m_ArsenalLoot.m_iMagazineUnlockMultiplier;
		balance.m_iLootRadiusMeters = m_ArsenalLoot.m_iLootRadiusMeters;
		balance.m_bLootOnlyLockedItems = m_ArsenalLoot.m_bLootOnlyLockedItems;
		balance.m_bRemoveLootedItems = m_ArsenalLoot.m_bRemoveLootedItems;
		balance.m_bAllowExplosiveUnlocks = m_ArsenalLoot.m_bAllowExplosiveUnlocks;
		balance.m_bAllowGuidedLauncherUnlocks = m_ArsenalLoot.m_bAllowGuidedLauncherUnlocks;
		balance.m_iWarLevelMaximum = m_Economy.m_iWarLevelMaximum;
	}

	string BuildSummary()
	{
		string campaign = string.Format("setup config | schema %1 | preset %2 | hideout %3", m_iSchemaVersion, m_Campaign.m_sPresetId, m_Campaign.m_sDefaultHideoutId);
		string factions = string.Format("\nfactions | resistance %1 | occupier %2 | invader %3", m_Factions.m_sResistanceFactionKey, m_Factions.m_sOccupierFactionKey, m_Factions.m_sInvaderFactionKey);
		string economy = string.Format("\neconomy | money %1 | HR %2 | income %3s | war max %4", m_Economy.m_iStartingFactionMoney, m_Economy.m_iStartingHR, m_Economy.m_iZoneIncomeIntervalSeconds, m_Economy.m_iWarLevelMaximum);
		string world = string.Format("\nworld | activation %1m | deactivation %2m | mission duration %3s", m_World.m_iActivationRadiusMeters, m_World.m_iDeactivationRadiusMeters, m_World.m_iMissionDefaultDurationSeconds);
		string loot = string.Format("\narsenal loot | unlock %1 | mag x%2 | radius %3m | locked only %4 | remove source %5", m_ArsenalLoot.m_iArsenalUnlockThreshold, m_ArsenalLoot.m_iMagazineUnlockMultiplier, m_ArsenalLoot.m_iLootRadiusMeters, m_ArsenalLoot.m_bLootOnlyLockedItems, m_ArsenalLoot.m_bRemoveLootedItems);
		string persistence = string.Format("\npersistence | autosave %1s | debounce %2s", m_Persistence.m_iAutosaveIntervalSeconds, m_Persistence.m_iMajorChangeDebounceSeconds);
		return campaign + factions + economy + world + loot + persistence + "\nsettings source | $profile:h-istasi/HST_Settings.json | config is source of truth for new campaigns";
	}
}
