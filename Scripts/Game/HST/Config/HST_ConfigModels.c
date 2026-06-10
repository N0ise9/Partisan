[BaseContainerProps(configRoot: true)]
class HST_BalanceConfig
{
	int m_iAutosaveIntervalSeconds = 900;
	int m_iMajorChangeDebounceSeconds = 30;
	int m_iStartingHR = 20;
	int m_iStartingFactionMoney = 1000;
	int m_iStartingOccupierAttackPool = 100;
	int m_iStartingOccupierSupportPool = 100;
	int m_iStartingInvaderAttackPool = 60;
	int m_iStartingInvaderSupportPool = 60;
	int m_iZoneIncomeIntervalSeconds = 600;
	int m_iMissionDefaultDurationSeconds = 3600;
	int m_iActivationRadiusMeters = 1200;
	int m_iDeactivationRadiusMeters = 1600;
	int m_iArsenalUnlockThreshold = 25;
	int m_iMagazineUnlockMultiplier = 3;
	int m_iHQInteractionRadiusMeters = 50;
	int m_iLootRadiusMeters = 15;
	bool m_bLootOnlyLockedItems;
	bool m_bRemoveLootedItems = true;
	bool m_bAllowExplosiveUnlocks;
	bool m_bAllowGuidedLauncherUnlocks;
	bool m_bVehicleLootEnabled = true;
	int m_iVehicleLootRadiusMeters = 20;
	bool m_bVehicleLootOnlyLockedItems;
	bool m_bVehicleLootRemoveSource = true;
	int m_iVehicleLootMaxItemsPerAction = 48;
	bool m_bAirSupportEnabled = true;
	int m_iAirSupportCooldownSeconds = 900;
	bool m_bCivilianPopulationEnabled = true;
	int m_iCivilianMaxActivePerTown = 12;
	int m_iCivilianVehicleMinPerTown = 1;
	int m_iCivilianVehicleMaxPerTown = 5;
	int m_iOccupierVehicleMinPerTown;
	int m_iOccupierVehicleMaxPerTown = 2;
	int m_iWarLevelMaximum = 10;
	int m_iCaptureProgressRequired = 100;
	int m_iCaptureProgressPerSecond = 2;
	int m_iCaptureDecayPerSecond = 1;
	int m_iCaptureAggressionBase = 10;
	int m_iCaptureCounterattackChancePercent = 45;
	ref array<string> m_aCivilianCharacterPrefabs = {};
	ref array<string> m_aCivilianVehiclePrefabs = {};
}

[BaseContainerProps()]
class HST_PrefabPoolEntry
{
	string m_sPrefab;
	int m_iWeight = 1;
	ref array<string> m_aTags = {};

	bool HasTag(string tag)
	{
		return !tag.IsEmpty() && m_aTags.Contains(tag);
	}
}

[BaseContainerProps()]
class HST_FactionTemplate
{
	string m_sTemplateId;
	string m_sFactionKey;
	string m_sDisplayName;
	bool m_bPlayable;
	ref array<string> m_aCapabilities = {};
	ref array<string> m_aInfantryPrefabs = {};
	ref array<string> m_aVehiclePrefabs = {};
	ref array<string> m_aGroupPrefabs = {};
	ref array<string> m_aPatrolGroupPrefabs = {};
	ref array<string> m_aQRFGroupPrefabs = {};
	ref array<string> m_aSupportIds = {};
	ref array<ref HST_PrefabPoolEntry> m_aGroupPool = {};
	ref array<ref HST_PrefabPoolEntry> m_aPatrolGroupPool = {};
	ref array<ref HST_PrefabPoolEntry> m_aQRFGroupPool = {};
	ref array<ref HST_PrefabPoolEntry> m_aRareGroupPool = {};

	bool HasCapability(string capabilityId)
	{
		return m_aCapabilities.Contains(capabilityId);
	}
}

[BaseContainerProps()]
class HST_ZoneDefinition
{
	string m_sZoneId;
	string m_sDisplayName;
	string m_sSourceLayoutId;
	string m_sSourceLayerName;
	HST_EZoneType m_eType;
	vector m_vPosition;
	string m_sInitialOwnerFactionKey;
	int m_iIncomeValue;
	string m_sResourceKind;
	int m_iCaptureRadiusMeters;
	int m_iPriority;
	string m_sCompositionId;
	string m_sSpawnProfileId;
	int m_iSupport;
	int m_iGarrisonSlots;
	int m_iActivationRadiusMeters;
	string m_sPatrolRouteId;
	string m_sQRFRouteId;
	string m_sMissionSiteId;
	bool m_bShowMapMarker = true;
	string m_sMarkerLabel;
	string m_sMarkerCallsign;
	string m_sMarkerIcon;
	string m_sMarkerColor;
	string m_sMarkerTextColor;
	string m_sMarkerStyle;
	ref array<string> m_aNearbyTownIds = {};
	ref array<string> m_aLinkedZoneIds = {};
	ref array<string> m_aCapabilities = {};
}

[BaseContainerProps()]
class HST_HideoutDefinition
{
	string m_sHideoutId;
	string m_sDisplayName;
	vector m_vPosition;
}

[BaseContainerProps(configRoot: true)]
class HST_MapDefinition
{
	string m_sMapId;
	string m_sDisplayName;
	ref array<ref HST_ZoneDefinition> m_aZones = {};
	ref array<ref HST_HideoutDefinition> m_aHideouts = {};
}

[BaseContainerProps()]
class HST_MissionDefinition
{
	string m_sMissionId;
	string m_sDisplayName;
	HST_EMissionCategory m_eCategory;
	int m_iDurationSeconds = 3600;
	int m_iRewardMoney;
	int m_iRewardHR;
	int m_iFailureAggression;
	string m_sBriefingText;
	string m_sRequirementText;
	string m_sFailureText;
	string m_sRuntimeType;
	string m_sTargetSitePreference;
	string m_sRewardText;
	string m_sFailurePenaltyText;
	int m_iRewardSupport;
	int m_iRewardCaptureProgress;
	int m_iCargoCount = 1;
	int m_iCaptiveCount = 1;
	int m_iVehicleCount = 1;
	int m_iRequiredWarLevel = 1;
	bool m_bAllowEnemyTarget = true;
	bool m_bAllowFriendlyTarget;
	ref array<string> m_aRequiredCapabilities = {};
	ref array<string> m_aTargetZoneTypes = {};
}

[BaseContainerProps(configRoot: true)]
class HST_MissionRegistryConfig
{
	ref array<ref HST_MissionDefinition> m_aMissions = {};
}

[BaseContainerProps(configRoot: true)]
class HST_CampaignPreset
{
	string m_sPresetId;
	string m_sDisplayName;
	string m_sResistanceFactionKey;
	string m_sOccupierFactionKey;
	string m_sInvaderFactionKey;
	string m_sBalanceConfig;
	string m_sMapDefinition;
	string m_sMissionRegistry;
	ref array<string> m_aUnavailableCapabilities = {};

	bool HasCapability(string capabilityId)
	{
		return !m_aUnavailableCapabilities.Contains(capabilityId);
	}
}
