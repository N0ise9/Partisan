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
	int m_iArsenalUnlockThreshold = 15;
	int m_iWarLevelMaximum = 10;
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
	HST_EZoneType m_eType;
	vector m_vPosition;
	string m_sInitialOwnerFactionKey;
	int m_iGarrisonSlots;
	int m_iActivationRadiusMeters;
	string m_sPatrolRouteId;
	string m_sQRFRouteId;
	string m_sMissionSiteId;
	ref array<string> m_aNearbyTownIds = {};
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
	ref array<string> m_aRequiredCapabilities = {};
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
