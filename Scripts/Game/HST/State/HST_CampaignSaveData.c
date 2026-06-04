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
	vector m_vArsenalPosition;
	vector m_vHQTentPosition;
	bool m_bHQDeployed;
	bool m_bHQRuntimeObjectsSpawned;
	bool m_bPetrosAlive;
	int m_iPetrosDeaths;
	string m_sPetrosPrefab;
	string m_sHQCachePrefab;
	string m_sArsenalPrefab;
	string m_sHQTentPrefab;

	ref array<ref HST_FactionPoolState> m_aFactionPools = {};
	ref array<ref HST_PlayerState> m_aPlayers = {};
	ref array<ref HST_ZoneState> m_aZones = {};
	ref array<ref HST_GarrisonState> m_aGarrisons = {};
	ref array<ref HST_ActiveGroupState> m_aActiveGroups = {};
	ref array<ref HST_QRFState> m_aQRFs = {};
	ref array<ref HST_MapMarkerState> m_aMapMarkers = {};
	ref array<ref HST_ArsenalItemState> m_aArsenalItems = {};
	ref array<ref HST_GarageVehicleState> m_aGarageVehicles = {};
	ref array<ref HST_EmplacementState> m_aCapturedEmplacements = {};
	ref array<ref HST_AmmoPointState> m_aAmmoPoints = {};
	ref array<ref HST_ActiveMissionState> m_aActiveMissions = {};
	ref array<ref HST_GeneratedSiteState> m_aGeneratedSites = {};
	ref array<ref HST_GeneratedRouteState> m_aGeneratedRoutes = {};
	ref array<ref HST_MissionObjectiveState> m_aMissionObjectives = {};
	ref array<ref HST_SupportRequestState> m_aSupportRequests = {};
	ref array<ref HST_EnemyOrderState> m_aEnemyOrders = {};
	ref array<ref HST_CivilianZoneState> m_aCivilianZones = {};
	ref array<ref HST_PlayerUndercoverState> m_aUndercoverPlayers = {};
	ref array<ref HST_CampaignTaskState> m_aCampaignTasks = {};

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
		m_vArsenalPosition = state.m_vArsenalPosition;
		m_vHQTentPosition = state.m_vHQTentPosition;
		m_bHQDeployed = state.m_bHQDeployed;
		m_bHQRuntimeObjectsSpawned = state.m_bHQRuntimeObjectsSpawned;
		m_bPetrosAlive = state.m_bPetrosAlive;
		m_iPetrosDeaths = state.m_iPetrosDeaths;
		m_sPetrosPrefab = state.m_sPetrosPrefab;
		m_sHQCachePrefab = state.m_sHQCachePrefab;
		m_sArsenalPrefab = state.m_sArsenalPrefab;
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

		m_aMapMarkers.Clear();
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
			m_aMapMarkers.Insert(CopyMapMarker(marker));

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

		m_aGeneratedSites.Clear();
		foreach (HST_GeneratedSiteState site : state.m_aGeneratedSites)
			m_aGeneratedSites.Insert(CopyGeneratedSite(site));

		m_aGeneratedRoutes.Clear();
		foreach (HST_GeneratedRouteState route : state.m_aGeneratedRoutes)
			m_aGeneratedRoutes.Insert(CopyGeneratedRoute(route));

		m_aMissionObjectives.Clear();
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
			m_aMissionObjectives.Insert(CopyMissionObjective(objective));

		m_aSupportRequests.Clear();
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
			m_aSupportRequests.Insert(CopySupportRequest(request));

		m_aEnemyOrders.Clear();
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
			m_aEnemyOrders.Insert(CopyEnemyOrder(order));

		m_aCivilianZones.Clear();
		foreach (HST_CivilianZoneState civilianZone : state.m_aCivilianZones)
			m_aCivilianZones.Insert(CopyCivilianZone(civilianZone));

		m_aUndercoverPlayers.Clear();
		foreach (HST_PlayerUndercoverState undercover : state.m_aUndercoverPlayers)
			m_aUndercoverPlayers.Insert(CopyUndercoverPlayer(undercover));

		m_aCampaignTasks.Clear();
		foreach (HST_CampaignTaskState task : state.m_aCampaignTasks)
			m_aCampaignTasks.Insert(CopyCampaignTask(task));
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
		state.m_vArsenalPosition = m_vArsenalPosition;
		state.m_vHQTentPosition = m_vHQTentPosition;
		state.m_bHQDeployed = m_bHQDeployed;
		state.m_bHQRuntimeObjectsSpawned = m_bHQRuntimeObjectsSpawned;
		state.m_bPetrosAlive = m_bPetrosAlive;
		state.m_iPetrosDeaths = m_iPetrosDeaths;
		state.m_sPetrosPrefab = m_sPetrosPrefab;
		state.m_sHQCachePrefab = m_sHQCachePrefab;
		state.m_sArsenalPrefab = m_sArsenalPrefab;
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

		state.m_aMapMarkers.Clear();
		foreach (HST_MapMarkerState marker : m_aMapMarkers)
			state.m_aMapMarkers.Insert(CopyMapMarker(marker));

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

		state.m_aGeneratedSites.Clear();
		foreach (HST_GeneratedSiteState site : m_aGeneratedSites)
			state.m_aGeneratedSites.Insert(CopyGeneratedSite(site));

		state.m_aGeneratedRoutes.Clear();
		foreach (HST_GeneratedRouteState route : m_aGeneratedRoutes)
			state.m_aGeneratedRoutes.Insert(CopyGeneratedRoute(route));

		state.m_aMissionObjectives.Clear();
		foreach (HST_MissionObjectiveState objective : m_aMissionObjectives)
			state.m_aMissionObjectives.Insert(CopyMissionObjective(objective));

		state.m_aSupportRequests.Clear();
		foreach (HST_SupportRequestState request : m_aSupportRequests)
			state.m_aSupportRequests.Insert(CopySupportRequest(request));

		state.m_aEnemyOrders.Clear();
		foreach (HST_EnemyOrderState order : m_aEnemyOrders)
			state.m_aEnemyOrders.Insert(CopyEnemyOrder(order));

		state.m_aCivilianZones.Clear();
		foreach (HST_CivilianZoneState civilianZone : m_aCivilianZones)
			state.m_aCivilianZones.Insert(CopyCivilianZone(civilianZone));

		state.m_aUndercoverPlayers.Clear();
		foreach (HST_PlayerUndercoverState undercover : m_aUndercoverPlayers)
			state.m_aUndercoverPlayers.Insert(CopyUndercoverPlayer(undercover));

		state.m_aCampaignTasks.Clear();
		foreach (HST_CampaignTaskState task : m_aCampaignTasks)
			state.m_aCampaignTasks.Insert(CopyCampaignTask(task));
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
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sOwnerFactionKey = source.m_sOwnerFactionKey;
		target.m_eType = source.m_eType;
		target.m_vPosition = source.m_vPosition;
		target.m_sResourceKind = source.m_sResourceKind;
		target.m_iSupport = source.m_iSupport;
		target.m_iResistanceCaptureProgress = source.m_iResistanceCaptureProgress;
		target.m_iIncomeValue = source.m_iIncomeValue;
		target.m_iCaptureRadiusMeters = source.m_iCaptureRadiusMeters;
		target.m_iPriority = source.m_iPriority;
		target.m_iGarrisonSlots = source.m_iGarrisonSlots;
		target.m_iActivationRadiusMeters = source.m_iActivationRadiusMeters;
		target.m_sCompositionId = source.m_sCompositionId;
		target.m_sSpawnProfileId = source.m_sSpawnProfileId;
		target.m_bActive = source.m_bActive;
		target.m_iActiveInfantryCount = source.m_iActiveInfantryCount;
		target.m_iActiveVehicleCount = source.m_iActiveVehicleCount;
		target.m_sPatrolRouteId = source.m_sPatrolRouteId;
		target.m_sQRFRouteId = source.m_sQRFRouteId;
		target.m_sMissionSiteId = source.m_sMissionSiteId;
		target.m_iQrfCooldownUntilSecond = source.m_iQrfCooldownUntilSecond;
		foreach (string linkedZoneId : source.m_aLinkedZoneIds)
			target.m_aLinkedZoneIds.Insert(linkedZoneId);
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

	protected HST_MapMarkerState CopyMapMarker(HST_MapMarkerState source)
	{
		HST_MapMarkerState target = new HST_MapMarkerState();
		target.m_sMarkerId = source.m_sMarkerId;
		target.m_sLinkedId = source.m_sLinkedId;
		target.m_sLabel = source.m_sLabel;
		target.m_sCategory = source.m_sCategory;
		target.m_sOwnerFactionKey = source.m_sOwnerFactionKey;
		target.m_sIconHint = source.m_sIconHint;
		target.m_sColorHint = source.m_sColorHint;
		target.m_vPosition = source.m_vPosition;
		target.m_bVisible = source.m_bVisible;
		target.m_bRuntimeNative = source.m_bRuntimeNative;
		return target;
	}

	protected HST_ArsenalItemState CopyArsenalItem(HST_ArsenalItemState source)
	{
		HST_ArsenalItemState target = new HST_ArsenalItemState();
		target.m_sPrefab = source.m_sPrefab;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sCategory = source.m_sCategory;
		target.m_iCount = source.m_iCount;
		target.m_bUnlocked = source.m_bUnlocked;
		return target;
	}

	protected HST_GarageVehicleState CopyGarageVehicle(HST_GarageVehicleState source)
	{
		HST_GarageVehicleState target = new HST_GarageVehicleState();
		target.m_sVehicleId = source.m_sVehicleId;
		target.m_sPrefab = source.m_sPrefab;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sSourceZoneId = source.m_sSourceZoneId;
		target.m_sSourceFactionKey = source.m_sSourceFactionKey;
		target.m_iStoredAtSecond = source.m_iStoredAtSecond;
		target.m_iRedeployCost = source.m_iRedeployCost;
		target.m_vPosition = source.m_vPosition;
		target.m_vAngles = source.m_vAngles;
		target.m_fFuel = source.m_fFuel;
		target.m_bArmed = source.m_bArmed;
		target.m_bUnlocked = source.m_bUnlocked;
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
		target.m_sSiteId = source.m_sSiteId;
		target.m_iStartedAtSecond = source.m_iStartedAtSecond;
		target.m_iActiveUntilSecond = source.m_iActiveUntilSecond;
		target.m_bDynamic = source.m_bDynamic;
		target.m_bRequested = source.m_bRequested;
		target.m_bStatic = source.m_bStatic;
		return target;
	}

	protected HST_GeneratedSiteState CopyGeneratedSite(HST_GeneratedSiteState source)
	{
		HST_GeneratedSiteState target = new HST_GeneratedSiteState();
		target.m_sSiteId = source.m_sSiteId;
		target.m_sZoneId = source.m_sZoneId;
		target.m_sRouteId = source.m_sRouteId;
		target.m_eType = source.m_eType;
		target.m_vPosition = source.m_vPosition;
		target.m_vSecondaryPosition = source.m_vSecondaryPosition;
		target.m_iRadiusMeters = source.m_iRadiusMeters;
		target.m_iWeight = source.m_iWeight;
		target.m_bValid = source.m_bValid;
		target.m_bOccupied = source.m_bOccupied;
		target.m_sOwnerFactionKey = source.m_sOwnerFactionKey;
		return target;
	}

	protected HST_GeneratedRouteState CopyGeneratedRoute(HST_GeneratedRouteState source)
	{
		HST_GeneratedRouteState target = new HST_GeneratedRouteState();
		target.m_sRouteId = source.m_sRouteId;
		target.m_sSourceZoneId = source.m_sSourceZoneId;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_vStartPosition = source.m_vStartPosition;
		target.m_vMidPosition = source.m_vMidPosition;
		target.m_vEndPosition = source.m_vEndPosition;
		target.m_iDistanceMeters = source.m_iDistanceMeters;
		target.m_bRoadRoute = source.m_bRoadRoute;
		return target;
	}

	protected HST_MissionObjectiveState CopyMissionObjective(HST_MissionObjectiveState source)
	{
		HST_MissionObjectiveState target = new HST_MissionObjectiveState();
		target.m_sObjectiveId = source.m_sObjectiveId;
		target.m_sMissionInstanceId = source.m_sMissionInstanceId;
		target.m_eType = source.m_eType;
		target.m_sTargetId = source.m_sTargetId;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sPhysicalEntityId = source.m_sPhysicalEntityId;
		target.m_vPosition = source.m_vPosition;
		target.m_iRequiredProgress = source.m_iRequiredProgress;
		target.m_iCurrentProgress = source.m_iCurrentProgress;
		target.m_bComplete = source.m_bComplete;
		target.m_bFailed = source.m_bFailed;
		target.m_bCleanupComplete = source.m_bCleanupComplete;
		return target;
	}

	protected HST_SupportRequestState CopySupportRequest(HST_SupportRequestState source)
	{
		HST_SupportRequestState target = new HST_SupportRequestState();
		target.m_sRequestId = source.m_sRequestId;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_sCapabilityId = source.m_sCapabilityId;
		target.m_sAssetProfileId = source.m_sAssetProfileId;
		target.m_eType = source.m_eType;
		target.m_eStatus = source.m_eStatus;
		target.m_sSourceZoneId = source.m_sSourceZoneId;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sGroupId = source.m_sGroupId;
		target.m_vSourcePosition = source.m_vSourcePosition;
		target.m_vTargetPosition = source.m_vTargetPosition;
		target.m_iRequestedAtSecond = source.m_iRequestedAtSecond;
		target.m_iETASeconds = source.m_iETASeconds;
		target.m_iAttackCost = source.m_iAttackCost;
		target.m_iSupportCost = source.m_iSupportCost;
		target.m_iMoneyCost = source.m_iMoneyCost;
		target.m_iCooldownUntilSecond = source.m_iCooldownUntilSecond;
		target.m_bHelicopterStyle = source.m_bHelicopterStyle;
		target.m_bPlayerRequested = source.m_bPlayerRequested;
		target.m_sFailureReason = source.m_sFailureReason;
		return target;
	}

	protected HST_EnemyOrderState CopyEnemyOrder(HST_EnemyOrderState source)
	{
		HST_EnemyOrderState target = new HST_EnemyOrderState();
		target.m_sOrderId = source.m_sOrderId;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_eType = source.m_eType;
		target.m_eStatus = source.m_eStatus;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sSupportRequestId = source.m_sSupportRequestId;
		target.m_iCreatedAtSecond = source.m_iCreatedAtSecond;
		target.m_iResolveAtSecond = source.m_iResolveAtSecond;
		target.m_iAttackCost = source.m_iAttackCost;
		target.m_iSupportCost = source.m_iSupportCost;
		return target;
	}

	protected HST_CivilianZoneState CopyCivilianZone(HST_CivilianZoneState source)
	{
		HST_CivilianZoneState target = new HST_CivilianZoneState();
		target.m_sZoneId = source.m_sZoneId;
		target.m_iReputation = source.m_iReputation;
		target.m_iWantedHeat = source.m_iWantedHeat;
		target.m_iCivilianPresence = source.m_iCivilianPresence;
		target.m_iPolicePresence = source.m_iPolicePresence;
		target.m_iRoadblockPresence = source.m_iRoadblockPresence;
		target.m_iLastIncidentSecond = source.m_iLastIncidentSecond;
		target.m_bUndercoverRestricted = source.m_bUndercoverRestricted;
		return target;
	}

	protected HST_PlayerUndercoverState CopyUndercoverPlayer(HST_PlayerUndercoverState source)
	{
		HST_PlayerUndercoverState target = new HST_PlayerUndercoverState();
		target.m_sIdentityId = source.m_sIdentityId;
		target.m_eStatus = source.m_eStatus;
		target.m_iWantedHeat = source.m_iWantedHeat;
		target.m_iCompromisedUntilSecond = source.m_iCompromisedUntilSecond;
		target.m_iLastCheckedSecond = source.m_iLastCheckedSecond;
		target.m_sLastReason = source.m_sLastReason;
		return target;
	}

	protected HST_CampaignTaskState CopyCampaignTask(HST_CampaignTaskState source)
	{
		HST_CampaignTaskState target = new HST_CampaignTaskState();
		target.m_sTaskId = source.m_sTaskId;
		target.m_sLinkedId = source.m_sLinkedId;
		target.m_sTitle = source.m_sTitle;
		target.m_sDescription = source.m_sDescription;
		target.m_sCategory = source.m_sCategory;
		target.m_vPosition = source.m_vPosition;
		target.m_bActive = source.m_bActive;
		target.m_bSucceeded = source.m_bSucceeded;
		target.m_bFailed = source.m_bFailed;
		return target;
	}
}
