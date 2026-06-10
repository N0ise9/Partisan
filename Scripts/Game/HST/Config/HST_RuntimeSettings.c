class HST_RuntimeSettingsCampaign
{
	string m_sPresetId = "vanilla_everon";
	int m_iCampaignSeed = 1985;
	string m_sDefaultHideoutId = "hideout_central_hills";
}

class HST_RuntimeSettingsFactions
{
	string m_sResistanceFactionKey = "FIA";
	string m_sOccupierFactionKey = "US";
	string m_sInvaderFactionKey = "USSR";
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

class HST_RuntimeSettingsCapture
{
	int m_iProgressRequired = 100;
	int m_iProgressPerSecond = 2;
	int m_iDecayPerSecond = 1;
	int m_iAggressionBase = 10;
	int m_iCounterattackChancePercent = 45;
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
	int m_iArsenalUnlockThreshold = 25;
	int m_iMagazineUnlockMultiplier = 3;
	int m_iHQInteractionRadiusMeters = 50;
	int m_iLootRadiusMeters = 15;
	bool m_bLootOnlyLockedItems;
	bool m_bRemoveLootedItems = true;
	bool m_bAllowExplosiveUnlocks;
	bool m_bAllowGuidedLauncherUnlocks;
}

class HST_RuntimeSettingsVehicleLoot
{
	bool m_bEnabled = true;
	int m_iRadiusMeters = 20;
	bool m_bOnlyLockedItems;
	bool m_bRemoveSourceItems = true;
	int m_iMaxItemsPerAction = 48;
}

class HST_RuntimeSettingsAirSupport
{
	bool m_bEnabled = true;
	int m_iCooldownSeconds = 900;
}

class HST_RuntimeSettingsCivilians
{
	bool m_bEnabled = true;
	int m_iMaxActivePerTown = 12;
	int m_iCivilianVehicleMinPerTown = 1;
	int m_iCivilianVehicleMaxPerTown = 5;
	int m_iOccupierVehicleMinPerTown;
	int m_iOccupierVehicleMaxPerTown = 2;
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
	static const int SCHEMA_VERSION = 8;

	int m_iSchemaVersion = SCHEMA_VERSION;
	ref HST_RuntimeSettingsCampaign m_Campaign = new HST_RuntimeSettingsCampaign();
	ref HST_RuntimeSettingsFactions m_Factions = new HST_RuntimeSettingsFactions();
	ref HST_RuntimeSettingsEconomy m_Economy = new HST_RuntimeSettingsEconomy();
	ref HST_RuntimeSettingsCapture m_Capture = new HST_RuntimeSettingsCapture();
	ref HST_RuntimeSettingsWorld m_World = new HST_RuntimeSettingsWorld();
	ref HST_RuntimeSettingsMembership m_Membership = new HST_RuntimeSettingsMembership();
	ref HST_RuntimeSettingsArsenalLoot m_ArsenalLoot = new HST_RuntimeSettingsArsenalLoot();
	ref HST_RuntimeSettingsVehicleLoot m_VehicleLoot = new HST_RuntimeSettingsVehicleLoot();
	ref HST_RuntimeSettingsAirSupport m_AirSupport = new HST_RuntimeSettingsAirSupport();
	ref HST_RuntimeSettingsCivilians m_Civilians = new HST_RuntimeSettingsCivilians();
	ref HST_RuntimeSettingsPersistence m_Persistence = new HST_RuntimeSettingsPersistence();
	ref HST_RuntimeSettingsDebug m_Debug = new HST_RuntimeSettingsDebug();
	ref HST_RuntimeSettingsFeatures m_Features = new HST_RuntimeSettingsFeatures();

	void Normalize()
	{
		if (m_iSchemaVersion <= 0)
			m_iSchemaVersion = SCHEMA_VERSION;
		m_Campaign.m_sPresetId = "vanilla_everon";
		m_Factions.m_sResistanceFactionKey = "FIA";
		m_Factions.m_sOccupierFactionKey = "US";
		m_Factions.m_sInvaderFactionKey = "USSR";

		m_Economy.m_iStartingFactionMoney = Math.Max(0, m_Economy.m_iStartingFactionMoney);
		m_Economy.m_iStartingHR = Math.Max(0, m_Economy.m_iStartingHR);
		m_Economy.m_iZoneIncomeIntervalSeconds = Math.Max(30, m_Economy.m_iZoneIncomeIntervalSeconds);
		m_Economy.m_iWarLevelMaximum = Math.Max(1, m_Economy.m_iWarLevelMaximum);
		m_Capture.m_iProgressRequired = Math.Max(1, m_Capture.m_iProgressRequired);
		m_Capture.m_iProgressPerSecond = Math.Max(1, m_Capture.m_iProgressPerSecond);
		m_Capture.m_iDecayPerSecond = Math.Max(0, m_Capture.m_iDecayPerSecond);
		m_Capture.m_iAggressionBase = Math.Max(0, m_Capture.m_iAggressionBase);
		m_Capture.m_iCounterattackChancePercent = Math.Max(0, Math.Min(100, m_Capture.m_iCounterattackChancePercent));
		m_World.m_iActivationRadiusMeters = Math.Max(100, m_World.m_iActivationRadiusMeters);
		m_World.m_iDeactivationRadiusMeters = Math.Max(m_World.m_iActivationRadiusMeters, m_World.m_iDeactivationRadiusMeters);
		m_World.m_iMissionDefaultDurationSeconds = Math.Max(300, m_World.m_iMissionDefaultDurationSeconds);
		m_ArsenalLoot.m_iArsenalUnlockThreshold = Math.Max(0, m_ArsenalLoot.m_iArsenalUnlockThreshold);
		m_ArsenalLoot.m_iMagazineUnlockMultiplier = Math.Max(1, m_ArsenalLoot.m_iMagazineUnlockMultiplier);
		m_ArsenalLoot.m_iHQInteractionRadiusMeters = Math.Max(1, m_ArsenalLoot.m_iHQInteractionRadiusMeters);
		m_ArsenalLoot.m_iLootRadiusMeters = Math.Max(1, m_ArsenalLoot.m_iLootRadiusMeters);
		m_VehicleLoot.m_iRadiusMeters = Math.Max(1, m_VehicleLoot.m_iRadiusMeters);
		m_VehicleLoot.m_iMaxItemsPerAction = Math.Max(1, m_VehicleLoot.m_iMaxItemsPerAction);
		m_AirSupport.m_iCooldownSeconds = Math.Max(60, m_AirSupport.m_iCooldownSeconds);
		m_Civilians.m_iMaxActivePerTown = Math.Max(0, m_Civilians.m_iMaxActivePerTown);
		m_Civilians.m_iCivilianVehicleMinPerTown = Math.Max(0, m_Civilians.m_iCivilianVehicleMinPerTown);
		m_Civilians.m_iCivilianVehicleMaxPerTown = Math.Max(m_Civilians.m_iCivilianVehicleMinPerTown, m_Civilians.m_iCivilianVehicleMaxPerTown);
		m_Civilians.m_iOccupierVehicleMinPerTown = Math.Max(0, m_Civilians.m_iOccupierVehicleMinPerTown);
		m_Civilians.m_iOccupierVehicleMaxPerTown = Math.Max(m_Civilians.m_iOccupierVehicleMinPerTown, m_Civilians.m_iOccupierVehicleMaxPerTown);
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
		balance.m_iCaptureProgressRequired = m_Capture.m_iProgressRequired;
		balance.m_iCaptureProgressPerSecond = m_Capture.m_iProgressPerSecond;
		balance.m_iCaptureDecayPerSecond = m_Capture.m_iDecayPerSecond;
		balance.m_iCaptureAggressionBase = m_Capture.m_iAggressionBase;
		balance.m_iCaptureCounterattackChancePercent = m_Capture.m_iCounterattackChancePercent;
		balance.m_iMissionDefaultDurationSeconds = m_World.m_iMissionDefaultDurationSeconds;
		balance.m_iActivationRadiusMeters = m_World.m_iActivationRadiusMeters;
		balance.m_iDeactivationRadiusMeters = m_World.m_iDeactivationRadiusMeters;
		balance.m_iArsenalUnlockThreshold = m_ArsenalLoot.m_iArsenalUnlockThreshold;
		balance.m_iMagazineUnlockMultiplier = m_ArsenalLoot.m_iMagazineUnlockMultiplier;
		balance.m_iHQInteractionRadiusMeters = m_ArsenalLoot.m_iHQInteractionRadiusMeters;
		balance.m_iLootRadiusMeters = m_ArsenalLoot.m_iLootRadiusMeters;
		balance.m_bLootOnlyLockedItems = m_ArsenalLoot.m_bLootOnlyLockedItems;
		balance.m_bRemoveLootedItems = m_ArsenalLoot.m_bRemoveLootedItems;
		balance.m_bAllowExplosiveUnlocks = m_ArsenalLoot.m_bAllowExplosiveUnlocks;
		balance.m_bAllowGuidedLauncherUnlocks = m_ArsenalLoot.m_bAllowGuidedLauncherUnlocks;
		balance.m_bVehicleLootEnabled = m_VehicleLoot.m_bEnabled;
		balance.m_iVehicleLootRadiusMeters = m_VehicleLoot.m_iRadiusMeters;
		balance.m_bVehicleLootOnlyLockedItems = m_VehicleLoot.m_bOnlyLockedItems;
		balance.m_bVehicleLootRemoveSource = m_VehicleLoot.m_bRemoveSourceItems;
		balance.m_iVehicleLootMaxItemsPerAction = m_VehicleLoot.m_iMaxItemsPerAction;
		balance.m_bAirSupportEnabled = m_AirSupport.m_bEnabled;
		balance.m_iAirSupportCooldownSeconds = m_AirSupport.m_iCooldownSeconds;
		balance.m_bCivilianPopulationEnabled = m_Civilians.m_bEnabled;
		balance.m_iCivilianMaxActivePerTown = m_Civilians.m_iMaxActivePerTown;
		balance.m_iCivilianVehicleMinPerTown = m_Civilians.m_iCivilianVehicleMinPerTown;
		balance.m_iCivilianVehicleMaxPerTown = m_Civilians.m_iCivilianVehicleMaxPerTown;
		balance.m_iOccupierVehicleMinPerTown = m_Civilians.m_iOccupierVehicleMinPerTown;
		balance.m_iOccupierVehicleMaxPerTown = m_Civilians.m_iOccupierVehicleMaxPerTown;
		balance.m_iWarLevelMaximum = m_Economy.m_iWarLevelMaximum;
	}

	string BuildSummary()
	{
		string campaign = string.Format("setup config | schema %1 | preset %2 | hideout %3", m_iSchemaVersion, m_Campaign.m_sPresetId, m_Campaign.m_sDefaultHideoutId);
		string factions = string.Format("\nfactions | resistance %1 | occupier %2 | invader %3", m_Factions.m_sResistanceFactionKey, m_Factions.m_sOccupierFactionKey, m_Factions.m_sInvaderFactionKey);
		string economy = string.Format("\neconomy | money %1 | HR %2 | income %3s | war max %4", m_Economy.m_iStartingFactionMoney, m_Economy.m_iStartingHR, m_Economy.m_iZoneIncomeIntervalSeconds, m_Economy.m_iWarLevelMaximum);
		string capture = string.Format("\ncapture | required %1 | progress %2/s | decay %3/s | aggression %4 | counterattack %5 pct", m_Capture.m_iProgressRequired, m_Capture.m_iProgressPerSecond, m_Capture.m_iDecayPerSecond, m_Capture.m_iAggressionBase, m_Capture.m_iCounterattackChancePercent);
		string world = string.Format("\nworld | activation %1m | deactivation %2m | mission duration %3s", m_World.m_iActivationRadiusMeters, m_World.m_iDeactivationRadiusMeters, m_World.m_iMissionDefaultDurationSeconds);
		string loot = string.Format("\narsenal loot | unlock %1 | mag x%2 | HQ radius %3m | loot radius %4m | locked only %5 | remove source %6", m_ArsenalLoot.m_iArsenalUnlockThreshold, m_ArsenalLoot.m_iMagazineUnlockMultiplier, m_ArsenalLoot.m_iHQInteractionRadiusMeters, m_ArsenalLoot.m_iLootRadiusMeters, m_ArsenalLoot.m_bLootOnlyLockedItems, m_ArsenalLoot.m_bRemoveLootedItems);
		string vehicleLoot = string.Format("\nvehicle loot | enabled %1 | radius %2m | locked only %3 | remove source %4 | max %5", m_VehicleLoot.m_bEnabled, m_VehicleLoot.m_iRadiusMeters, m_VehicleLoot.m_bOnlyLockedItems, m_VehicleLoot.m_bRemoveSourceItems, m_VehicleLoot.m_iMaxItemsPerAction);
		string airSupport = string.Format("\nair support | enabled %1 | cooldown %2s", m_AirSupport.m_bEnabled, m_AirSupport.m_iCooldownSeconds);
		string civilians = string.Format("\ncivilians | enabled %1 | max %2 per town | civ vehicles %3-%4 | occupier vehicles %5-%6", m_Civilians.m_bEnabled, m_Civilians.m_iMaxActivePerTown, m_Civilians.m_iCivilianVehicleMinPerTown, m_Civilians.m_iCivilianVehicleMaxPerTown, m_Civilians.m_iOccupierVehicleMinPerTown, m_Civilians.m_iOccupierVehicleMaxPerTown);
		string persistence = string.Format("\npersistence | autosave %1s | debounce %2s", m_Persistence.m_iAutosaveIntervalSeconds, m_Persistence.m_iMajorChangeDebounceSeconds);
		return campaign + factions + economy + capture + world + loot + vehicleLoot + airSupport + civilians + persistence + "\nsettings source | $profile:h-istasi/HST_Settings.json | config is source of truth for new campaigns";
	}
}
