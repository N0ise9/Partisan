[BaseContainerProps()]
class HST_CampaignSaveData
{
	int m_iSchemaVersion;
	string m_sPresetId;
	int m_iCampaignSeed;
	HST_ECampaignPhase m_ePhase;
	int m_iElapsedSeconds;
	int m_iWarLevel;
	int m_iFactionMoney;
	int m_iHR;
	int m_iTrainingLevel;
	int m_iIncomeAccumulatorSeconds;
	int m_iEnemyResourceAccumulatorSeconds;
	string m_sCommanderIdentityId;
	string m_sHQHideoutId;
	vector m_vHQPosition;
	vector m_vPetrosPosition;
	vector m_vHQCachePosition;
	vector m_vHQTentPosition;
	bool m_bHQDeployed;
	bool m_bHQRuntimeObjectsSpawned;
	bool m_bPetrosAlive;
	int m_iPetrosDeaths;
	string m_sPetrosPrefab;
	string m_sHQCachePrefab;
	string m_sHQTentPrefab;

	ref array<ref HST_FactionPoolState> m_aFactionPools = {};
	ref array<ref HST_PlayerState> m_aPlayers = {};
	ref array<ref HST_ZoneState> m_aZones = {};
	ref array<ref HST_GarrisonState> m_aGarrisons = {};
	ref array<ref HST_ActiveGroupState> m_aActiveGroups = {};
	ref array<ref HST_QRFState> m_aQRFs = {};
	ref array<ref HST_ArsenalItemState> m_aArsenalItems = {};
	ref array<ref HST_GarageVehicleState> m_aGarageVehicles = {};
	ref array<ref HST_EmplacementState> m_aCapturedEmplacements = {};
	ref array<ref HST_AmmoPointState> m_aAmmoPoints = {};
	ref array<ref HST_ActiveMissionState> m_aActiveMissions = {};

	void Capture(HST_CampaignState state)
	{
		if (!state)
			return;

		m_iSchemaVersion = state.m_iSchemaVersion;
		m_sPresetId = state.m_sPresetId;
		m_iCampaignSeed = state.m_iCampaignSeed;
		m_ePhase = state.m_ePhase;
		m_iElapsedSeconds = state.m_iElapsedSeconds;
		m_iWarLevel = state.m_iWarLevel;
		m_iFactionMoney = state.m_iFactionMoney;
		m_iHR = state.m_iHR;
		m_iTrainingLevel = state.m_iTrainingLevel;
		m_iIncomeAccumulatorSeconds = state.m_iIncomeAccumulatorSeconds;
		m_iEnemyResourceAccumulatorSeconds = state.m_iEnemyResourceAccumulatorSeconds;
		m_sCommanderIdentityId = state.m_sCommanderIdentityId;
		m_sHQHideoutId = state.m_sHQHideoutId;
		m_vHQPosition = state.m_vHQPosition;
		m_vPetrosPosition = state.m_vPetrosPosition;
		m_vHQCachePosition = state.m_vHQCachePosition;
		m_vHQTentPosition = state.m_vHQTentPosition;
		m_bHQDeployed = state.m_bHQDeployed;
		m_bHQRuntimeObjectsSpawned = state.m_bHQRuntimeObjectsSpawned;
		m_bPetrosAlive = state.m_bPetrosAlive;
		m_iPetrosDeaths = state.m_iPetrosDeaths;
		m_sPetrosPrefab = state.m_sPetrosPrefab;
		m_sHQCachePrefab = state.m_sHQCachePrefab;
		m_sHQTentPrefab = state.m_sHQTentPrefab;

		m_aFactionPools.Clear();
		foreach (HST_FactionPoolState factionPool : state.m_aFactionPools)
			m_aFactionPools.Insert(CopyFactionPool(factionPool));

		m_aPlayers.Clear();
		foreach (HST_PlayerState player : state.m_aPlayers)
			m_aPlayers.Insert(CopyPlayer(player));

		m_aZones.Clear();
		foreach (HST_ZoneState zone : state.m_aZones)
			m_aZones.Insert(CopyZone(zone));

		m_aGarrisons.Clear();
		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
			m_aGarrisons.Insert(CopyGarrison(garrison));

		m_aActiveGroups.Clear();
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
			m_aActiveGroups.Insert(CopyActiveGroup(activeGroup));

		m_aQRFs.Clear();
		foreach (HST_QRFState qrf : state.m_aQRFs)
			m_aQRFs.Insert(CopyQRF(qrf));

		m_aArsenalItems.Clear();
		foreach (HST_ArsenalItemState arsenalItem : state.m_aArsenalItems)
			m_aArsenalItems.Insert(CopyArsenalItem(arsenalItem));

		m_aGarageVehicles.Clear();
		foreach (HST_GarageVehicleState vehicle : state.m_aGarageVehicles)
			m_aGarageVehicles.Insert(CopyGarageVehicle(vehicle));

		m_aCapturedEmplacements.Clear();
		foreach (HST_EmplacementState emplacement : state.m_aCapturedEmplacements)
			m_aCapturedEmplacements.Insert(CopyEmplacement(emplacement));

		m_aAmmoPoints.Clear();
		foreach (HST_AmmoPointState ammoPoint : state.m_aAmmoPoints)
			m_aAmmoPoints.Insert(CopyAmmoPoint(ammoPoint));

		m_aActiveMissions.Clear();
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
			m_aActiveMissions.Insert(CopyActiveMission(mission));
	}

	HST_CampaignState Restore()
	{
		HST_CampaignState state = new HST_CampaignState();
		ApplyTo(state);
		return state;
	}

	void ApplyTo(HST_CampaignState state)
	{
		if (!state)
			return;

		state.m_iSchemaVersion = m_iSchemaVersion;
		state.m_sPresetId = m_sPresetId;
		state.m_iCampaignSeed = m_iCampaignSeed;
		state.m_ePhase = m_ePhase;
		state.m_iElapsedSeconds = m_iElapsedSeconds;
		state.m_iWarLevel = m_iWarLevel;
		state.m_iFactionMoney = m_iFactionMoney;
		state.m_iHR = m_iHR;
		state.m_iTrainingLevel = m_iTrainingLevel;
		state.m_iIncomeAccumulatorSeconds = m_iIncomeAccumulatorSeconds;
		state.m_iEnemyResourceAccumulatorSeconds = m_iEnemyResourceAccumulatorSeconds;
		state.m_sCommanderIdentityId = m_sCommanderIdentityId;
		state.m_sHQHideoutId = m_sHQHideoutId;
		state.m_vHQPosition = m_vHQPosition;
		state.m_vPetrosPosition = m_vPetrosPosition;
		state.m_vHQCachePosition = m_vHQCachePosition;
		state.m_vHQTentPosition = m_vHQTentPosition;
		state.m_bHQDeployed = m_bHQDeployed;
		state.m_bHQRuntimeObjectsSpawned = m_bHQRuntimeObjectsSpawned;
		state.m_bPetrosAlive = m_bPetrosAlive;
		state.m_iPetrosDeaths = m_iPetrosDeaths;
		state.m_sPetrosPrefab = m_sPetrosPrefab;
		state.m_sHQCachePrefab = m_sHQCachePrefab;
		state.m_sHQTentPrefab = m_sHQTentPrefab;

		state.m_aFactionPools.Clear();
		foreach (HST_FactionPoolState factionPool : m_aFactionPools)
			state.m_aFactionPools.Insert(CopyFactionPool(factionPool));

		state.m_aPlayers.Clear();
		foreach (HST_PlayerState player : m_aPlayers)
			state.m_aPlayers.Insert(CopyPlayer(player));

		state.m_aZones.Clear();
		foreach (HST_ZoneState zone : m_aZones)
			state.m_aZones.Insert(CopyZone(zone));

		state.m_aGarrisons.Clear();
		foreach (HST_GarrisonState garrison : m_aGarrisons)
			state.m_aGarrisons.Insert(CopyGarrison(garrison));

		state.m_aActiveGroups.Clear();
		foreach (HST_ActiveGroupState activeGroup : m_aActiveGroups)
			state.m_aActiveGroups.Insert(CopyActiveGroup(activeGroup));

		state.m_aQRFs.Clear();
		foreach (HST_QRFState qrf : m_aQRFs)
			state.m_aQRFs.Insert(CopyQRF(qrf));

		state.m_aArsenalItems.Clear();
		foreach (HST_ArsenalItemState arsenalItem : m_aArsenalItems)
			state.m_aArsenalItems.Insert(CopyArsenalItem(arsenalItem));

		state.m_aGarageVehicles.Clear();
		foreach (HST_GarageVehicleState vehicle : m_aGarageVehicles)
			state.m_aGarageVehicles.Insert(CopyGarageVehicle(vehicle));

		state.m_aCapturedEmplacements.Clear();
		foreach (HST_EmplacementState emplacement : m_aCapturedEmplacements)
			state.m_aCapturedEmplacements.Insert(CopyEmplacement(emplacement));

		state.m_aAmmoPoints.Clear();
		foreach (HST_AmmoPointState ammoPoint : m_aAmmoPoints)
			state.m_aAmmoPoints.Insert(CopyAmmoPoint(ammoPoint));

		state.m_aActiveMissions.Clear();
		foreach (HST_ActiveMissionState mission : m_aActiveMissions)
			state.m_aActiveMissions.Insert(CopyActiveMission(mission));
	}

	protected HST_FactionPoolState CopyFactionPool(HST_FactionPoolState source)
	{
		HST_FactionPoolState target = new HST_FactionPoolState();
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_iAttackResources = source.m_iAttackResources;
		target.m_iSupportResources = source.m_iSupportResources;
		target.m_iMoney = source.m_iMoney;
		target.m_iHR = source.m_iHR;
		target.m_iAggression = source.m_iAggression;
		return target;
	}

	protected HST_PlayerState CopyPlayer(HST_PlayerState source)
	{
		HST_PlayerState target = new HST_PlayerState();
		target.m_sIdentityId = source.m_sIdentityId;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_bMember = source.m_bMember;
		target.m_bAdmin = source.m_bAdmin;
		target.m_bGuest = source.m_bGuest;
		target.m_iMoney = source.m_iMoney;
		target.m_iRank = source.m_iRank;
		target.m_iLastSeenPlayerId = source.m_iLastSeenPlayerId;
		target.m_bHasSpawnRecord = source.m_bHasSpawnRecord;
		target.m_iSpawnCount = source.m_iSpawnCount;
		target.m_sLastSpawnPrefab = source.m_sLastSpawnPrefab;
		target.m_vLastSpawnPosition = source.m_vLastSpawnPosition;
		return target;
	}

	protected HST_ZoneState CopyZone(HST_ZoneState source)
	{
		HST_ZoneState target = new HST_ZoneState();
		target.m_sZoneId = source.m_sZoneId;
		target.m_sOwnerFactionKey = source.m_sOwnerFactionKey;
		target.m_eType = source.m_eType;
		target.m_vPosition = source.m_vPosition;
		target.m_iSupport = source.m_iSupport;
		target.m_iResistanceCaptureProgress = source.m_iResistanceCaptureProgress;
		target.m_iIncomeValue = source.m_iIncomeValue;
		target.m_iGarrisonSlots = source.m_iGarrisonSlots;
		target.m_iActivationRadiusMeters = source.m_iActivationRadiusMeters;
		target.m_bActive = source.m_bActive;
		target.m_iActiveInfantryCount = source.m_iActiveInfantryCount;
		target.m_iActiveVehicleCount = source.m_iActiveVehicleCount;
		target.m_sPatrolRouteId = source.m_sPatrolRouteId;
		target.m_sQRFRouteId = source.m_sQRFRouteId;
		target.m_sMissionSiteId = source.m_sMissionSiteId;
		target.m_iQrfCooldownUntilSecond = source.m_iQrfCooldownUntilSecond;
		return target;
	}

	protected HST_GarrisonState CopyGarrison(HST_GarrisonState source)
	{
		HST_GarrisonState target = new HST_GarrisonState();
		target.m_sZoneId = source.m_sZoneId;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_iInfantryCount = source.m_iInfantryCount;
		target.m_iVehicleCount = source.m_iVehicleCount;
		return target;
	}

	protected HST_ActiveGroupState CopyActiveGroup(HST_ActiveGroupState source)
	{
		HST_ActiveGroupState target = new HST_ActiveGroupState();
		target.m_sGroupId = source.m_sGroupId;
		target.m_sZoneId = source.m_sZoneId;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_sPrefab = source.m_sPrefab;
		target.m_vPosition = source.m_vPosition;
		target.m_iInfantryCount = source.m_iInfantryCount;
		target.m_iVehicleCount = source.m_iVehicleCount;
		target.m_bQRF = source.m_bQRF;
		target.m_bSpawnAttempted = source.m_bSpawnAttempted;
		target.m_bSpawnedEntity = source.m_bSpawnedEntity;
		return target;
	}

	protected HST_QRFState CopyQRF(HST_QRFState source)
	{
		HST_QRFState target = new HST_QRFState();
		target.m_sInstanceId = source.m_sInstanceId;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_sSourceZoneId = source.m_sSourceZoneId;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sGroupId = source.m_sGroupId;
		target.m_iStartedAtSecond = source.m_iStartedAtSecond;
		target.m_iETASeconds = source.m_iETASeconds;
		target.m_bResolved = source.m_bResolved;
		target.m_bSucceeded = source.m_bSucceeded;
		return target;
	}

	protected HST_ArsenalItemState CopyArsenalItem(HST_ArsenalItemState source)
	{
		HST_ArsenalItemState target = new HST_ArsenalItemState();
		target.m_sPrefab = source.m_sPrefab;
		target.m_iCount = source.m_iCount;
		target.m_bUnlocked = source.m_bUnlocked;
		return target;
	}

	protected HST_GarageVehicleState CopyGarageVehicle(HST_GarageVehicleState source)
	{
		HST_GarageVehicleState target = new HST_GarageVehicleState();
		target.m_sVehicleId = source.m_sVehicleId;
		target.m_sPrefab = source.m_sPrefab;
		target.m_vPosition = source.m_vPosition;
		target.m_vAngles = source.m_vAngles;
		target.m_fFuel = source.m_fFuel;
		target.m_bArmed = source.m_bArmed;
		return target;
	}

	protected HST_EmplacementState CopyEmplacement(HST_EmplacementState source)
	{
		HST_EmplacementState target = new HST_EmplacementState();
		target.m_sEmplacementId = source.m_sEmplacementId;
		target.m_sPrefab = source.m_sPrefab;
		target.m_vPosition = source.m_vPosition;
		target.m_vAngles = source.m_vAngles;
		return target;
	}

	protected HST_AmmoPointState CopyAmmoPoint(HST_AmmoPointState source)
	{
		HST_AmmoPointState target = new HST_AmmoPointState();
		target.m_sAmmoPointId = source.m_sAmmoPointId;
		target.m_vPosition = source.m_vPosition;
		return target;
	}

	protected HST_ActiveMissionState CopyActiveMission(HST_ActiveMissionState source)
	{
		HST_ActiveMissionState target = new HST_ActiveMissionState();
		target.m_sInstanceId = source.m_sInstanceId;
		target.m_sMissionId = source.m_sMissionId;
		target.m_eStatus = source.m_eStatus;
		target.m_iRemainingSeconds = source.m_iRemainingSeconds;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		return target;
	}
}
