[BaseContainerProps()]
class HST_ArsenalItemRule
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Prefab name or path fragment matched by this arsenal rule.", category: "HST Arsenal")]
	string m_sPrefabContains;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Logical arsenal category for matching items.", category: "HST Arsenal")]
	string m_sCategory;

	[Attribute(defvalue: "unlock", uiwidget: UIWidgets.EditBox, desc: "Unlock policy applied to matching items.", category: "HST Arsenal")]
	string m_sPolicy = "unlock";

	[Attribute(defvalue: "-1", uiwidget: UIWidgets.EditBox, desc: "Optional unlock threshold override; -1 uses the balance default.", category: "HST Arsenal")]
	int m_iUnlockThresholdOverride = -1;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Whether this rule applies to area loot.", params: "", category: "HST Arsenal")]
	bool m_bAppliesToAreaLoot = true;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Whether this rule applies to vehicle loot.", params: "", category: "HST Arsenal")]
	bool m_bAppliesToVehicleLoot = true;
}

[BaseContainerProps(configRoot: true)]
class HST_BalanceConfig
{
	[Attribute(defvalue: "900", uiwidget: UIWidgets.EditBox, desc: "Autosave interval in seconds.", category: "HST Balance")]
	int m_iAutosaveIntervalSeconds = 900;

	[Attribute(defvalue: "30", uiwidget: UIWidgets.EditBox, desc: "Major-change save debounce in seconds.", category: "HST Balance")]
	int m_iMajorChangeDebounceSeconds = 30;

	[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Starting HR.", category: "HST Balance")]
	int m_iStartingHR = 10;

	[Attribute(defvalue: "750", uiwidget: UIWidgets.EditBox, desc: "Starting FIA money.", category: "HST Balance")]
	int m_iStartingFactionMoney = 750;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "Starting FIA training level.", category: "HST Balance")]
	int m_iStartingTrainingLevel = 1;

	[Attribute(defvalue: "70", uiwidget: UIWidgets.EditBox, desc: "Starting occupier attack resource pool.", category: "HST Balance")]
	int m_iStartingOccupierAttackPool = 70;

	[Attribute(defvalue: "80", uiwidget: UIWidgets.EditBox, desc: "Starting occupier support resource pool.", category: "HST Balance")]
	int m_iStartingOccupierSupportPool = 80;

	[Attribute(defvalue: "35", uiwidget: UIWidgets.EditBox, desc: "Starting invader attack resource pool.", category: "HST Balance")]
	int m_iStartingInvaderAttackPool = 35;

	[Attribute(defvalue: "45", uiwidget: UIWidgets.EditBox, desc: "Starting invader support resource pool.", category: "HST Balance")]
	int m_iStartingInvaderSupportPool = 45;

	[Attribute(defvalue: "600", uiwidget: UIWidgets.EditBox, desc: "Zone income interval in seconds.", category: "HST Balance")]
	int m_iZoneIncomeIntervalSeconds = 600;

	[Attribute(defvalue: "3600", uiwidget: UIWidgets.EditBox, desc: "Default mission duration in seconds.", category: "HST Balance")]
	int m_iMissionDefaultDurationSeconds = 3600;

	[Attribute(defvalue: "1200", uiwidget: UIWidgets.EditBox, desc: "Physical activation radius in meters.", category: "HST Balance")]
	int m_iActivationRadiusMeters = 1200;

	[Attribute(defvalue: "1600", uiwidget: UIWidgets.EditBox, desc: "Physical deactivation radius in meters.", category: "HST Balance")]
	int m_iDeactivationRadiusMeters = 1600;

	[Attribute(defvalue: "1800", uiwidget: UIWidgets.EditBox, desc: "Player event/render bubble radius in meters.", category: "HST Balance")]
	int m_iPlayerRenderBubbleRadiusMeters = 1800;

	[Attribute(defvalue: "1800", uiwidget: UIWidgets.EditBox, desc: "Mission category target-selection radius in meters.", category: "HST Balance")]
	int m_iMissionSelectionRadiusMeters = 1800;

	[Attribute(defvalue: "18", uiwidget: UIWidgets.EditBox, desc: "Default arsenal unlock threshold.", category: "HST Loot")]
	int m_iArsenalUnlockThreshold = 18;

	[Attribute(defvalue: "3", uiwidget: UIWidgets.EditBox, desc: "Magazine unlock multiplier.", category: "HST Loot")]
	int m_iMagazineUnlockMultiplier = 3;

	[Attribute(defvalue: "50", uiwidget: UIWidgets.EditBox, desc: "HQ interaction radius in meters.", category: "HST Loot")]
	int m_iHQInteractionRadiusMeters = 50;

	[Attribute(defvalue: "15", uiwidget: UIWidgets.EditBox, desc: "Area loot radius in meters.", category: "HST Loot")]
	int m_iLootRadiusMeters = 15;

	[Attribute(defvalue: "false", uiwidget: UIWidgets.CheckBox, desc: "Only collect locked area loot items.", params: "", category: "HST Loot")]
	bool m_bLootOnlyLockedItems;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Remove collected area loot from the source.", params: "", category: "HST Loot")]
	bool m_bRemoveLootedItems = true;

	[Attribute(defvalue: "false", uiwidget: UIWidgets.CheckBox, desc: "Allow explosives to unlock in the arsenal.", params: "", category: "HST Loot")]
	bool m_bAllowExplosiveUnlocks;

	[Attribute(defvalue: "false", uiwidget: UIWidgets.CheckBox, desc: "Allow guided launchers to unlock in the arsenal.", params: "", category: "HST Loot")]
	bool m_bAllowGuidedLauncherUnlocks;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Enable vehicle loot collection.", params: "", category: "HST Vehicle Loot")]
	bool m_bVehicleLootEnabled = true;

	[Attribute(defvalue: "20", uiwidget: UIWidgets.EditBox, desc: "Vehicle loot radius in meters.", category: "HST Vehicle Loot")]
	int m_iVehicleLootRadiusMeters = 20;

	[Attribute(defvalue: "false", uiwidget: UIWidgets.CheckBox, desc: "Only collect locked vehicle loot items.", params: "", category: "HST Vehicle Loot")]
	bool m_bVehicleLootOnlyLockedItems;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Remove collected vehicle loot from the source.", params: "", category: "HST Vehicle Loot")]
	bool m_bVehicleLootRemoveSource = true;

	[Attribute(defvalue: "48", uiwidget: UIWidgets.EditBox, desc: "Maximum vehicle loot items collected per action.", category: "HST Vehicle Loot")]
	int m_iVehicleLootMaxItemsPerAction = 48;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Enable air support.", params: "", category: "HST Support")]
	bool m_bAirSupportEnabled = true;

	[Attribute(defvalue: "900", uiwidget: UIWidgets.EditBox, desc: "Air support cooldown in seconds.", category: "HST Support")]
	int m_iAirSupportCooldownSeconds = 900;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Enable civilian population simulation.", params: "", category: "HST Civilians")]
	bool m_bCivilianPopulationEnabled = true;

	[Attribute(defvalue: "12", uiwidget: UIWidgets.EditBox, desc: "Maximum active civilians per town.", category: "HST Civilians")]
	int m_iCivilianMaxActivePerTown = 12;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "Minimum civilian vehicles per town.", category: "HST Civilians")]
	int m_iCivilianVehicleMinPerTown = 1;

	[Attribute(defvalue: "5", uiwidget: UIWidgets.EditBox, desc: "Maximum civilian vehicles per town.", category: "HST Civilians")]
	int m_iCivilianVehicleMaxPerTown = 5;

	[Attribute(defvalue: "2", uiwidget: UIWidgets.EditBox, desc: "Number of active civilian-driven ambient traffic vehicles per active town.", category: "HST Civilians")]
	int m_iCivilianDrivingVehicleCountPerTown = 2;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Minimum occupier vehicles per town.", category: "HST Civilians")]
	int m_iOccupierVehicleMinPerTown;

	[Attribute(defvalue: "2", uiwidget: UIWidgets.EditBox, desc: "Maximum occupier vehicles per town.", category: "HST Civilians")]
	int m_iOccupierVehicleMaxPerTown = 2;

	[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Maximum war level.", category: "HST Campaign")]
	int m_iWarLevelMaximum = 10;

	[Attribute(defvalue: "18", uiwidget: UIWidgets.EditBox, desc: "Owned-zone score needed for war level 2.", category: "HST Campaign")]
	int m_iWarLevel2Score = 18;

	[Attribute(defvalue: "38", uiwidget: UIWidgets.EditBox, desc: "Owned-zone score needed for war level 3.", category: "HST Campaign")]
	int m_iWarLevel3Score = 38;

	[Attribute(defvalue: "65", uiwidget: UIWidgets.EditBox, desc: "Owned-zone score needed for war level 4.", category: "HST Campaign")]
	int m_iWarLevel4Score = 65;

	[Attribute(defvalue: "95", uiwidget: UIWidgets.EditBox, desc: "Owned-zone score needed for war level 5.", category: "HST Campaign")]
	int m_iWarLevel5Score = 95;

	[Attribute(defvalue: "130", uiwidget: UIWidgets.EditBox, desc: "Owned-zone score needed for war level 6.", category: "HST Campaign")]
	int m_iWarLevel6Score = 130;

	[Attribute(defvalue: "170", uiwidget: UIWidgets.EditBox, desc: "Owned-zone score needed for war level 7.", category: "HST Campaign")]
	int m_iWarLevel7Score = 170;

	[Attribute(defvalue: "215", uiwidget: UIWidgets.EditBox, desc: "Owned-zone score needed for war level 8.", category: "HST Campaign")]
	int m_iWarLevel8Score = 215;

	[Attribute(defvalue: "265", uiwidget: UIWidgets.EditBox, desc: "Owned-zone score needed for war level 9.", category: "HST Campaign")]
	int m_iWarLevel9Score = 265;

	[Attribute(defvalue: "320", uiwidget: UIWidgets.EditBox, desc: "Owned-zone score needed for war level 10.", category: "HST Campaign")]
	int m_iWarLevel10Score = 320;

	[Attribute(defvalue: "70", uiwidget: UIWidgets.EditBox, desc: "Strategic control percent needed for victory.", category: "HST Campaign")]
	int m_iVictoryControlPercent = 70;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Use population support plus airfield control as the default campaign outcome.", params: "", category: "HST Campaign")]
	bool m_bPopulationOutcomeEnabled = true;

	[Attribute(defvalue: "50", uiwidget: UIWidgets.EditBox, desc: "Percent of remaining population support needed for population victory.", category: "HST Campaign")]
	int m_iVictoryPopulationSupportPercent = 50;

	[Attribute(defvalue: "false", uiwidget: UIWidgets.CheckBox, desc: "Allow legacy strategic-control victory mode when population outcome is disabled.", params: "", category: "HST Campaign")]
	bool m_bLegacyControlVictoryEnabled;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Require all airfields for victory.", params: "", category: "HST Campaign")]
	bool m_bVictoryRequiresAirfields = true;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Require all seaports for victory.", params: "", category: "HST Campaign")]
	bool m_bVictoryRequiresSeaports = true;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Enable campaign loss condition.", params: "", category: "HST Campaign")]
	bool m_bLossConditionEnabled = true;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "HR threshold for loss check.", category: "HST Campaign")]
	int m_iLossHRThreshold;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Money threshold for loss check.", category: "HST Campaign")]
	int m_iLossMoneyThreshold;

	[Attribute(defvalue: "3", uiwidget: UIWidgets.EditBox, desc: "Petros deaths before campaign loss.", category: "HST Campaign")]
	int m_iLossPetrosDeathLimit = 3;

	[Attribute(defvalue: "7200", uiwidget: UIWidgets.EditBox, desc: "Grace period before economic loss can trigger.", category: "HST Campaign")]
	int m_iLossGraceSeconds = 7200;

	[Attribute(defvalue: "8", uiwidget: UIWidgets.EditBox, desc: "Enemy attack income multiplier percent per war level.", category: "HST Balance")]
	int m_iEnemyAttackIncomeWarPercent = 8;

	[Attribute(defvalue: "6", uiwidget: UIWidgets.EditBox, desc: "Enemy support income multiplier percent per war level.", category: "HST Balance")]
	int m_iEnemySupportIncomeWarPercent = 6;

	[Attribute(defvalue: "300", uiwidget: UIWidgets.EditBox, desc: "Aggression decay interval in seconds.", category: "HST Balance")]
	int m_iAggressionDecayIntervalSeconds = 300;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "Aggression decay amount per interval.", category: "HST Balance")]
	int m_iAggressionDecayAmount = 1;

	[Attribute(defvalue: "100", uiwidget: UIWidgets.EditBox, desc: "Capture progress required to flip a zone.", category: "HST Capture")]
	int m_iCaptureProgressRequired = 100;

	[Attribute(defvalue: "2", uiwidget: UIWidgets.EditBox, desc: "Capture progress gained per second.", category: "HST Capture")]
	int m_iCaptureProgressPerSecond = 2;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "Capture progress decay per second.", category: "HST Capture")]
	int m_iCaptureDecayPerSecond = 1;

	[Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Aggression added by capture.", category: "HST Capture")]
	int m_iCaptureAggressionBase = 10;

	[Attribute(defvalue: "45", uiwidget: UIWidgets.EditBox, desc: "Counterattack chance after capture, in percent.", category: "HST Capture")]
	int m_iCaptureCounterattackChancePercent = 45;

	[Attribute(desc: "Arsenal item classification and unlock rules.", category: "HST Loot")]
	ref array<ref HST_ArsenalItemRule> m_aArsenalItemRules = {};

	[Attribute(desc: "Civilian character prefab resources.", category: "HST Civilians")]
	ref array<string> m_aCivilianCharacterPrefabs = {};

	[Attribute(desc: "Civilian vehicle prefab resources.", category: "HST Civilians")]
	ref array<string> m_aCivilianVehiclePrefabs = {};
}

[BaseContainerProps()]
class HST_PrefabPoolEntry
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Prefab resource.", category: "HST Prefab Pool")]
	string m_sPrefab;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "Selection weight.", category: "HST Prefab Pool")]
	int m_iWeight = 1;

	[Attribute(desc: "Tags for filtering this prefab pool entry.", category: "HST Prefab Pool")]
	ref array<string> m_aTags = {};

	bool HasTag(string tag)
	{
		return !tag.IsEmpty() && m_aTags.Contains(tag);
	}
}

[BaseContainerProps()]
class HST_FactionTemplate
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Template identifier.", category: "HST Faction")]
	string m_sTemplateId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Faction key.", category: "HST Faction")]
	string m_sFactionKey;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Display name.", category: "HST Faction")]
	string m_sDisplayName;

	[Attribute(defvalue: "false", uiwidget: UIWidgets.CheckBox, desc: "Whether this faction can be selected by players.", params: "", category: "HST Faction")]
	bool m_bPlayable;

	[Attribute(desc: "Faction capability ids.", category: "HST Faction")]
	ref array<string> m_aCapabilities = {};

	[Attribute(desc: "Infantry character prefab resources.", category: "HST Faction")]
	ref array<string> m_aInfantryPrefabs = {};

	[Attribute(desc: "Vehicle prefab resources.", category: "HST Faction")]
	ref array<string> m_aVehiclePrefabs = {};

	[Attribute(desc: "Group prefab resources.", category: "HST Faction")]
	ref array<string> m_aGroupPrefabs = {};

	[Attribute(desc: "Patrol group prefab resources.", category: "HST Faction")]
	ref array<string> m_aPatrolGroupPrefabs = {};

	[Attribute(desc: "QRF group prefab resources.", category: "HST Faction")]
	ref array<string> m_aQRFGroupPrefabs = {};

	[Attribute(desc: "Support ids available to this faction.", category: "HST Faction")]
	ref array<string> m_aSupportIds = {};

	[Attribute(desc: "Weighted group prefab pool.", category: "HST Faction")]
	ref array<ref HST_PrefabPoolEntry> m_aGroupPool = {};

	[Attribute(desc: "Weighted patrol group prefab pool.", category: "HST Faction")]
	ref array<ref HST_PrefabPoolEntry> m_aPatrolGroupPool = {};

	[Attribute(desc: "Weighted QRF group prefab pool.", category: "HST Faction")]
	ref array<ref HST_PrefabPoolEntry> m_aQRFGroupPool = {};

	[Attribute(desc: "Weighted rare group prefab pool.", category: "HST Faction")]
	ref array<ref HST_PrefabPoolEntry> m_aRareGroupPool = {};

	bool HasCapability(string capabilityId)
	{
		return m_aCapabilities.Contains(capabilityId);
	}
}

[BaseContainerProps()]
class HST_FactionRuntimeSpawnSpec
{
	string m_sFactionKey;
	string m_sGroupPrefab;
	string m_sExpectedNativeFactionKey;
	string m_sSourceCatalog;
	string m_sRole;
	string m_sSourceContext;
	ref array<string> m_aDirectInfantryPrefabs = {};
	ref array<string> m_aVehiclePrefabs = {};
	ref array<string> m_aGroupPrefabs = {};
}

[BaseContainerProps()]
class HST_ZoneDefinition
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Zone identifier.", category: "HST Zone")]
	string m_sZoneId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Display name.", category: "HST Zone")]
	string m_sDisplayName;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Source layout identifier.", category: "HST Zone")]
	string m_sSourceLayoutId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Source layer name.", category: "HST Zone")]
	string m_sSourceLayerName;

	[Attribute(defvalue: "HST_ZONE_TOWN", uiwidget: UIWidgets.ComboBox, desc: "Strategic zone type.", params: "", enums: ParamEnumArray.FromEnum(HST_EZoneType), category: "HST Zone")]
	HST_EZoneType m_eType;

	[Attribute(defvalue: "0 0 0", desc: "World position.", category: "HST Zone")]
	vector m_vPosition;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Initial owner faction key.", category: "HST Zone")]
	string m_sInitialOwnerFactionKey;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Income value.", category: "HST Zone")]
	int m_iIncomeValue;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Resource kind.", category: "HST Zone")]
	string m_sResourceKind;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Capture radius in meters.", category: "HST Zone")]
	int m_iCaptureRadiusMeters;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Strategic priority.", category: "HST Zone")]
	int m_iPriority;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Composition identifier.", category: "HST Zone")]
	string m_sCompositionId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Spawn profile identifier.", category: "HST Zone")]
	string m_sSpawnProfileId;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Town support value.", category: "HST Zone")]
	int m_iSupport;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Garrison slot count.", category: "HST Zone")]
	int m_iGarrisonSlots;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Zone activation radius in meters.", category: "HST Zone")]
	int m_iActivationRadiusMeters;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Patrol route identifier.", category: "HST Zone")]
	string m_sPatrolRouteId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "QRF route identifier.", category: "HST Zone")]
	string m_sQRFRouteId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Mission site identifier.", category: "HST Zone")]
	string m_sMissionSiteId;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Show this zone on the map.", params: "", category: "HST Zone Marker")]
	bool m_bShowMapMarker = true;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Map marker label.", category: "HST Zone Marker")]
	string m_sMarkerLabel;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Map marker callsign.", category: "HST Zone Marker")]
	string m_sMarkerCallsign;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Map marker icon id.", category: "HST Zone Marker")]
	string m_sMarkerIcon;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Map marker color id.", category: "HST Zone Marker")]
	string m_sMarkerColor;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Map marker text color id.", category: "HST Zone Marker")]
	string m_sMarkerTextColor;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Map marker style id.", category: "HST Zone Marker")]
	string m_sMarkerStyle;

	[Attribute(desc: "Nearby town zone ids.", category: "HST Zone")]
	ref array<string> m_aNearbyTownIds = {};

	[Attribute(desc: "Linked strategic zone ids.", category: "HST Zone")]
	ref array<string> m_aLinkedZoneIds = {};

	[Attribute(desc: "Zone capability ids.", category: "HST Zone")]
	ref array<string> m_aCapabilities = {};
}

[BaseContainerProps()]
class HST_HideoutDefinition
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Hideout identifier.", category: "HST Hideout")]
	string m_sHideoutId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Display name.", category: "HST Hideout")]
	string m_sDisplayName;

	[Attribute(defvalue: "0 0 0", desc: "World position.", category: "HST Hideout")]
	vector m_vPosition;
}

[BaseContainerProps(configRoot: true)]
class HST_MapDefinition
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Map identifier.", category: "HST Map")]
	string m_sMapId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Display name.", category: "HST Map")]
	string m_sDisplayName;

	[Attribute(defvalue: "0 0 0", desc: "World-space map minimum bounds.", category: "HST Map")]
	vector m_vWorldMin;

	[Attribute(defvalue: "12800 0 12800", desc: "World-space map maximum bounds.", category: "HST Map")]
	vector m_vWorldMax;

	[Attribute(desc: "Strategic zone definitions.", category: "HST Map")]
	ref array<ref HST_ZoneDefinition> m_aZones = {};

	[Attribute(desc: "Resistance hideout definitions.", category: "HST Map")]
	ref array<ref HST_HideoutDefinition> m_aHideouts = {};
}

[BaseContainerProps()]
class HST_MissionDefinition
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Mission identifier.", category: "HST Mission")]
	string m_sMissionId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Display name.", category: "HST Mission")]
	string m_sDisplayName;

	[Attribute(defvalue: "HST_MISSION_ASSASSINATION", uiwidget: UIWidgets.ComboBox, desc: "Mission category.", params: "", enums: ParamEnumArray.FromEnum(HST_EMissionCategory), category: "HST Mission")]
	HST_EMissionCategory m_eCategory;

	[Attribute(defvalue: "3600", uiwidget: UIWidgets.EditBox, desc: "Duration in seconds.", category: "HST Mission")]
	int m_iDurationSeconds = 3600;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Money reward.", category: "HST Mission Rewards")]
	int m_iRewardMoney;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "HR reward.", category: "HST Mission Rewards")]
	int m_iRewardHR;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Aggression added on failure.", category: "HST Mission")]
	int m_iFailureAggression;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Briefing text.", category: "HST Mission Text")]
	string m_sBriefingText;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Requirement text.", category: "HST Mission Text")]
	string m_sRequirementText;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Failure text.", category: "HST Mission Text")]
	string m_sFailureText;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Runtime type id.", category: "HST Mission Runtime")]
	string m_sRuntimeType;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Preferred target site type.", category: "HST Mission Runtime")]
	string m_sTargetSitePreference;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Reward text.", category: "HST Mission Text")]
	string m_sRewardText;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Failure penalty text.", category: "HST Mission Text")]
	string m_sFailurePenaltyText;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Town support reward.", category: "HST Mission Rewards")]
	int m_iRewardSupport;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Capture progress reward.", category: "HST Mission Rewards")]
	int m_iRewardCaptureProgress;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Required cargo count.", category: "HST Mission Runtime")]
	int m_iCargoCount;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Required captive count.", category: "HST Mission Runtime")]
	int m_iCaptiveCount;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Vehicle count.", category: "HST Mission Runtime")]
	int m_iVehicleCount;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "Required war level.", category: "HST Mission")]
	int m_iRequiredWarLevel = 1;

	[Attribute(defvalue: "true", uiwidget: UIWidgets.CheckBox, desc: "Allow hostile target zones.", params: "", category: "HST Mission Targeting")]
	bool m_bAllowEnemyTarget = true;

	[Attribute(defvalue: "false", uiwidget: UIWidgets.CheckBox, desc: "Allow friendly target zones.", params: "", category: "HST Mission Targeting")]
	bool m_bAllowFriendlyTarget;

	[Attribute(desc: "Capabilities required by this mission.", category: "HST Mission Targeting")]
	ref array<string> m_aRequiredCapabilities = {};

	[Attribute(desc: "Allowed target zone type ids.", category: "HST Mission Targeting")]
	ref array<string> m_aTargetZoneTypes = {};
}

[BaseContainerProps(configRoot: true)]
class HST_MissionRegistryConfig
{
	[Attribute(desc: "Mission definitions available to the campaign.", category: "HST Missions")]
	ref array<ref HST_MissionDefinition> m_aMissions = {};
}

[BaseContainerProps(configRoot: true)]
class HST_CampaignPreset
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Campaign preset identifier.", category: "HST Preset")]
	string m_sPresetId;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Display name.", category: "HST Preset")]
	string m_sDisplayName;

	[Attribute(defvalue: "FIA", uiwidget: UIWidgets.EditBox, desc: "Resistance faction key.", category: "HST Preset")]
	string m_sResistanceFactionKey;

	[Attribute(defvalue: "US", uiwidget: UIWidgets.EditBox, desc: "Occupier faction key.", category: "HST Preset")]
	string m_sOccupierFactionKey;

	[Attribute(defvalue: "USSR", uiwidget: UIWidgets.EditBox, desc: "Invader faction key.", category: "HST Preset")]
	string m_sInvaderFactionKey;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Balance config resource.", category: "HST Preset")]
	string m_sBalanceConfig;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Map definition config resource.", category: "HST Preset")]
	string m_sMapDefinition;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Mission registry config resource.", category: "HST Preset")]
	string m_sMissionRegistry;

	[Attribute(desc: "Capabilities unavailable in this preset.", category: "HST Preset")]
	ref array<string> m_aUnavailableCapabilities = {};

	bool HasCapability(string capabilityId)
	{
		return !m_aUnavailableCapabilities.Contains(capabilityId);
	}
}
