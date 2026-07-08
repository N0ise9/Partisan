class HST_RuntimeSettingsCampaign
{
	string m_sPresetId = "vanilla_everon";
	int m_iCampaignSeed = 1985;
}

class HST_RuntimeSettingsFactions
{
	string m_sResistanceFactionKey = "FIA";
	string m_sOccupierFactionKey = "US";
	string m_sInvaderFactionKey = "USSR";
}

class HST_RuntimeSettingsEconomy
{
	int m_iStartingFactionMoney = 750;
	int m_iStartingHR = 10;
	int m_iStartingTrainingLevel = 1;
	int m_iStartingOccupierAttackPool = 70;
	int m_iStartingOccupierSupportPool = 80;
	int m_iStartingInvaderAttackPool = 35;
	int m_iStartingInvaderSupportPool = 45;
	int m_iZoneIncomeIntervalSeconds = 600;
	int m_iWarLevelMaximum = 10;
	int m_iWarLevel2Score = 18;
	int m_iWarLevel3Score = 38;
	int m_iWarLevel4Score = 65;
	int m_iWarLevel5Score = 95;
	int m_iWarLevel6Score = 130;
	int m_iWarLevel7Score = 170;
	int m_iWarLevel8Score = 215;
	int m_iWarLevel9Score = 265;
	int m_iWarLevel10Score = 320;
	int m_iVictoryControlPercent = 70;
	bool m_bPopulationOutcomeEnabled = true;
	int m_iVictoryPopulationSupportPercent = 50;
	bool m_bLegacyControlVictoryEnabled;
	bool m_bVictoryRequiresAirfields = true;
	bool m_bVictoryRequiresSeaports = true;
	bool m_bLossConditionEnabled = true;
	int m_iLossHRThreshold;
	int m_iLossMoneyThreshold;
	int m_iLossPetrosDeathLimit = 3;
	int m_iLossGraceSeconds = 7200;
	int m_iEnemyAttackIncomeWarPercent = 8;
	int m_iEnemySupportIncomeWarPercent = 6;
	int m_iAggressionDecayIntervalSeconds = 300;
	int m_iAggressionDecayAmount = 1;
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
	int m_iPlayerRenderBubbleRadiusMeters = 1800;
	int m_iMissionSelectionRadiusMeters = 1800;
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
	int m_iArsenalUnlockThreshold = 18;
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
	bool m_bDebugLoggingEnabled;
}

class HST_RuntimeSettingsFeatures
{
	bool m_bPhysicalWarEnabled = true;
	bool m_bAreaLootEnabled = true;
	bool m_bSetupUiReadOnly = true;
	bool m_bGameMasterBudgetsEnabled;
	bool m_bShowPlayerMapMarkers = true;
	bool m_bInfiniteStaminaEnabled = true;
	bool m_bTrackResistanceSupportGroupsOnMap = true;
}

class HST_RuntimeSettings
{
	static const int SCHEMA_VERSION = 17;

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
		m_Economy.m_iStartingTrainingLevel = Math.Max(1, m_Economy.m_iStartingTrainingLevel);
		m_Economy.m_iStartingOccupierAttackPool = Math.Max(0, m_Economy.m_iStartingOccupierAttackPool);
		m_Economy.m_iStartingOccupierSupportPool = Math.Max(0, m_Economy.m_iStartingOccupierSupportPool);
		m_Economy.m_iStartingInvaderAttackPool = Math.Max(0, m_Economy.m_iStartingInvaderAttackPool);
		m_Economy.m_iStartingInvaderSupportPool = Math.Max(0, m_Economy.m_iStartingInvaderSupportPool);
		m_Economy.m_iZoneIncomeIntervalSeconds = Math.Max(30, m_Economy.m_iZoneIncomeIntervalSeconds);
		m_Economy.m_iWarLevelMaximum = Math.Max(1, m_Economy.m_iWarLevelMaximum);
		m_Economy.m_iWarLevel2Score = Math.Max(1, m_Economy.m_iWarLevel2Score);
		m_Economy.m_iWarLevel3Score = Math.Max(m_Economy.m_iWarLevel2Score + 1, m_Economy.m_iWarLevel3Score);
		m_Economy.m_iWarLevel4Score = Math.Max(m_Economy.m_iWarLevel3Score + 1, m_Economy.m_iWarLevel4Score);
		m_Economy.m_iWarLevel5Score = Math.Max(m_Economy.m_iWarLevel4Score + 1, m_Economy.m_iWarLevel5Score);
		m_Economy.m_iWarLevel6Score = Math.Max(m_Economy.m_iWarLevel5Score + 1, m_Economy.m_iWarLevel6Score);
		m_Economy.m_iWarLevel7Score = Math.Max(m_Economy.m_iWarLevel6Score + 1, m_Economy.m_iWarLevel7Score);
		m_Economy.m_iWarLevel8Score = Math.Max(m_Economy.m_iWarLevel7Score + 1, m_Economy.m_iWarLevel8Score);
		m_Economy.m_iWarLevel9Score = Math.Max(m_Economy.m_iWarLevel8Score + 1, m_Economy.m_iWarLevel9Score);
		m_Economy.m_iWarLevel10Score = Math.Max(m_Economy.m_iWarLevel9Score + 1, m_Economy.m_iWarLevel10Score);
		m_Economy.m_iVictoryControlPercent = Math.Max(1, Math.Min(100, m_Economy.m_iVictoryControlPercent));
		m_Economy.m_iVictoryPopulationSupportPercent = Math.Max(1, Math.Min(100, m_Economy.m_iVictoryPopulationSupportPercent));
		m_Economy.m_iLossHRThreshold = Math.Max(0, m_Economy.m_iLossHRThreshold);
		m_Economy.m_iLossMoneyThreshold = Math.Max(0, m_Economy.m_iLossMoneyThreshold);
		m_Economy.m_iLossPetrosDeathLimit = Math.Max(1, m_Economy.m_iLossPetrosDeathLimit);
		m_Economy.m_iLossGraceSeconds = Math.Max(0, m_Economy.m_iLossGraceSeconds);
		m_Economy.m_iEnemyAttackIncomeWarPercent = Math.Max(0, m_Economy.m_iEnemyAttackIncomeWarPercent);
		m_Economy.m_iEnemySupportIncomeWarPercent = Math.Max(0, m_Economy.m_iEnemySupportIncomeWarPercent);
		m_Economy.m_iAggressionDecayIntervalSeconds = Math.Max(60, m_Economy.m_iAggressionDecayIntervalSeconds);
		m_Economy.m_iAggressionDecayAmount = Math.Max(0, m_Economy.m_iAggressionDecayAmount);
		m_Capture.m_iProgressRequired = Math.Max(1, m_Capture.m_iProgressRequired);
		m_Capture.m_iProgressPerSecond = Math.Max(1, m_Capture.m_iProgressPerSecond);
		m_Capture.m_iDecayPerSecond = Math.Max(0, m_Capture.m_iDecayPerSecond);
		m_Capture.m_iAggressionBase = Math.Max(0, m_Capture.m_iAggressionBase);
		m_Capture.m_iCounterattackChancePercent = Math.Max(0, Math.Min(100, m_Capture.m_iCounterattackChancePercent));
		m_World.m_iActivationRadiusMeters = Math.Max(100, m_World.m_iActivationRadiusMeters);
		m_World.m_iDeactivationRadiusMeters = Math.Max(m_World.m_iActivationRadiusMeters, m_World.m_iDeactivationRadiusMeters);
		m_World.m_iPlayerRenderBubbleRadiusMeters = Math.Max(100, m_World.m_iPlayerRenderBubbleRadiusMeters);
		if (m_World.m_iMissionSelectionRadiusMeters <= 0)
			m_World.m_iMissionSelectionRadiusMeters = m_World.m_iPlayerRenderBubbleRadiusMeters;
		else
			m_World.m_iMissionSelectionRadiusMeters = Math.Max(100, m_World.m_iMissionSelectionRadiusMeters);
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
		balance.m_iStartingTrainingLevel = m_Economy.m_iStartingTrainingLevel;
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
		balance.m_iPlayerRenderBubbleRadiusMeters = m_World.m_iPlayerRenderBubbleRadiusMeters;
		balance.m_iMissionSelectionRadiusMeters = m_World.m_iMissionSelectionRadiusMeters;
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
		balance.m_iWarLevel2Score = m_Economy.m_iWarLevel2Score;
		balance.m_iWarLevel3Score = m_Economy.m_iWarLevel3Score;
		balance.m_iWarLevel4Score = m_Economy.m_iWarLevel4Score;
		balance.m_iWarLevel5Score = m_Economy.m_iWarLevel5Score;
		balance.m_iWarLevel6Score = m_Economy.m_iWarLevel6Score;
		balance.m_iWarLevel7Score = m_Economy.m_iWarLevel7Score;
		balance.m_iWarLevel8Score = m_Economy.m_iWarLevel8Score;
		balance.m_iWarLevel9Score = m_Economy.m_iWarLevel9Score;
		balance.m_iWarLevel10Score = m_Economy.m_iWarLevel10Score;
		balance.m_iVictoryControlPercent = m_Economy.m_iVictoryControlPercent;
		balance.m_bPopulationOutcomeEnabled = m_Economy.m_bPopulationOutcomeEnabled;
		balance.m_iVictoryPopulationSupportPercent = m_Economy.m_iVictoryPopulationSupportPercent;
		balance.m_bLegacyControlVictoryEnabled = m_Economy.m_bLegacyControlVictoryEnabled;
		balance.m_bVictoryRequiresAirfields = m_Economy.m_bVictoryRequiresAirfields;
		balance.m_bVictoryRequiresSeaports = m_Economy.m_bVictoryRequiresSeaports;
		balance.m_bLossConditionEnabled = m_Economy.m_bLossConditionEnabled;
		balance.m_iLossHRThreshold = m_Economy.m_iLossHRThreshold;
		balance.m_iLossMoneyThreshold = m_Economy.m_iLossMoneyThreshold;
		balance.m_iLossPetrosDeathLimit = m_Economy.m_iLossPetrosDeathLimit;
		balance.m_iLossGraceSeconds = m_Economy.m_iLossGraceSeconds;
		balance.m_iEnemyAttackIncomeWarPercent = m_Economy.m_iEnemyAttackIncomeWarPercent;
		balance.m_iEnemySupportIncomeWarPercent = m_Economy.m_iEnemySupportIncomeWarPercent;
		balance.m_iAggressionDecayIntervalSeconds = m_Economy.m_iAggressionDecayIntervalSeconds;
		balance.m_iAggressionDecayAmount = m_Economy.m_iAggressionDecayAmount;
	}

	string BuildSummary()
	{
		string campaign = string.Format("setup config | schema %1 | preset %2 | seed %3", m_iSchemaVersion, m_Campaign.m_sPresetId, m_Campaign.m_iCampaignSeed);
		string factions = string.Format("\nfactions | resistance %1 | occupier %2 | invader %3", m_Factions.m_sResistanceFactionKey, m_Factions.m_sOccupierFactionKey, m_Factions.m_sInvaderFactionKey);
		string economy = string.Format("\neconomy | money %1 | HR %2 | training %3 | income %4s | war max %5", m_Economy.m_iStartingFactionMoney, m_Economy.m_iStartingHR, m_Economy.m_iStartingTrainingLevel, m_Economy.m_iZoneIncomeIntervalSeconds, m_Economy.m_iWarLevelMaximum);
		string pacing = string.Format("\npacing | WL2 %1 | WL3 %2 | WL4 %3 | WL5 %4 | WL6 %5 | WL7 %6 | WL8 %7 | WL9 %8 | WL10 %9", m_Economy.m_iWarLevel2Score, m_Economy.m_iWarLevel3Score, m_Economy.m_iWarLevel4Score, m_Economy.m_iWarLevel5Score, m_Economy.m_iWarLevel6Score, m_Economy.m_iWarLevel7Score, m_Economy.m_iWarLevel8Score, m_Economy.m_iWarLevel9Score, m_Economy.m_iWarLevel10Score);
		pacing = pacing + string.Format(" | population outcome %1 | support victory %2 pct | legacy control %3 pct enabled %4", m_Economy.m_bPopulationOutcomeEnabled, m_Economy.m_iVictoryPopulationSupportPercent, m_Economy.m_iVictoryControlPercent, m_Economy.m_bLegacyControlVictoryEnabled);
		string loss = string.Format("\nloss | enabled %1 | population catastrophe when more than one third killed | HR %2 | money %3 | Petros deaths %4 | grace %5s", m_Economy.m_bLossConditionEnabled, m_Economy.m_iLossHRThreshold, m_Economy.m_iLossMoneyThreshold, m_Economy.m_iLossPetrosDeathLimit, m_Economy.m_iLossGraceSeconds);
		string capture = string.Format("\ncapture | required %1 | progress %2/s | decay %3/s | aggression %4 | counterattack %5 pct", m_Capture.m_iProgressRequired, m_Capture.m_iProgressPerSecond, m_Capture.m_iDecayPerSecond, m_Capture.m_iAggressionBase, m_Capture.m_iCounterattackChancePercent);
		string world = string.Format("\nworld | activation %1m | deactivation %2m | render bubble %3m | mission selection %4m | mission duration %5s", m_World.m_iActivationRadiusMeters, m_World.m_iDeactivationRadiusMeters, m_World.m_iPlayerRenderBubbleRadiusMeters, m_World.m_iMissionSelectionRadiusMeters, m_World.m_iMissionDefaultDurationSeconds);
		string loot = string.Format("\narsenal loot | unlock %1 | mag x%2 | HQ radius %3m | loot radius %4m | locked only %5 | remove source %6", m_ArsenalLoot.m_iArsenalUnlockThreshold, m_ArsenalLoot.m_iMagazineUnlockMultiplier, m_ArsenalLoot.m_iHQInteractionRadiusMeters, m_ArsenalLoot.m_iLootRadiusMeters, m_ArsenalLoot.m_bLootOnlyLockedItems, m_ArsenalLoot.m_bRemoveLootedItems);
		string vehicleLoot = string.Format("\nvehicle loot | enabled %1 | radius %2m | locked only %3 | remove source %4 | max %5", m_VehicleLoot.m_bEnabled, m_VehicleLoot.m_iRadiusMeters, m_VehicleLoot.m_bOnlyLockedItems, m_VehicleLoot.m_bRemoveSourceItems, m_VehicleLoot.m_iMaxItemsPerAction);
		string airSupport = string.Format("\nair support | enabled %1 | cooldown %2s", m_AirSupport.m_bEnabled, m_AirSupport.m_iCooldownSeconds);
		string civilians = string.Format("\ncivilians | enabled %1 | max %2 per town | civ vehicles %3-%4 | occupier vehicles %5-%6", m_Civilians.m_bEnabled, m_Civilians.m_iMaxActivePerTown, m_Civilians.m_iCivilianVehicleMinPerTown, m_Civilians.m_iCivilianVehicleMaxPerTown, m_Civilians.m_iOccupierVehicleMinPerTown, m_Civilians.m_iOccupierVehicleMaxPerTown);
		string persistence = string.Format("\npersistence | autosave %1s | debounce %2s", m_Persistence.m_iAutosaveIntervalSeconds, m_Persistence.m_iMajorChangeDebounceSeconds);
		string features = string.Format("\nfeatures | physical war %1 | area loot %2 | setup read only %3 | GM budgets %4 | player markers %5 | infinite stamina %6 | resistance support tracking %7", m_Features.m_bPhysicalWarEnabled, m_Features.m_bAreaLootEnabled, m_Features.m_bSetupUiReadOnly, m_Features.m_bGameMasterBudgetsEnabled, m_Features.m_bShowPlayerMapMarkers, m_Features.m_bInfiniteStaminaEnabled, m_Features.m_bTrackResistanceSupportGroupsOnMap);
		return campaign + factions + economy + pacing + loss + capture + world + loot + vehicleLoot + airSupport + civilians + persistence + features + "\nsettings source | $profile:h-istasi/HST_Settings.json | config is source of truth for new campaigns";
	}
}
