[BaseContainerProps()]
class HST_FactionPoolState
{
	string m_sFactionKey;
	int m_iAttackResources;
	int m_iSupportResources;
	int m_iMoney;
	int m_iHR;
	int m_iAggression;
}

[BaseContainerProps()]
class HST_PlayerState
{
	string m_sIdentityId;
	string m_sFactionKey = "FIA";
	bool m_bMember;
	bool m_bAdmin;
	bool m_bGuest = true;
	int m_iMoney;
	int m_iRank;
	int m_iLastSeenPlayerId = -1;
	bool m_bHasSpawnRecord;
	int m_iSpawnCount;
	string m_sLastSpawnPrefab;
	vector m_vLastSpawnPosition;
}

[BaseContainerProps()]
class HST_ZoneState
{
	string m_sZoneId;
	string m_sDisplayName;
	string m_sSourceLayoutId;
	string m_sSourceLayerName;
	string m_sMarkerCallsign;
	string m_sMarkerTextColor;
	string m_sMarkerStyle;
	string m_sOwnerFactionKey;
	HST_EZoneType m_eType;
	vector m_vPosition;
	string m_sResourceKind;
	int m_iSupport;
	int m_iResistanceCaptureProgress;
	int m_iIncomeValue;
	int m_iCaptureRadiusMeters;
	int m_iPriority;
	int m_iGarrisonSlots;
	int m_iActivationRadiusMeters;
	string m_sCompositionId;
	string m_sSpawnProfileId;
	bool m_bActive;
	int m_iActiveInfantryCount;
	int m_iActiveVehicleCount;
	string m_sPatrolRouteId;
	string m_sQRFRouteId;
	string m_sMissionSiteId;
	int m_iQrfCooldownUntilSecond;
	ref array<string> m_aLinkedZoneIds = {};
}

[BaseContainerProps()]
class HST_GarrisonState
{
	string m_sZoneId;
	string m_sFactionKey;
	int m_iInfantryCount;
	int m_iVehicleCount;
}

[BaseContainerProps()]
class HST_ActiveGroupState
{
	string m_sGroupId;
	string m_sZoneId;
	string m_sFactionKey;
	string m_sPrefab;
	vector m_vPosition;
	string m_sRouteId;
	vector m_vSourcePosition;
	vector m_vTargetPosition;
	string m_sRuntimeEntityId;
	string m_sRuntimeStatus = "queued";
	int m_iInfantryCount;
	int m_iVehicleCount;
	int m_iSpawnedAtSecond;
	int m_iLastSeenAliveCount;
	int m_iSurvivorInfantryCount;
	int m_iSurvivorVehicleCount;
	bool m_bQRF;
	bool m_bSpawnAttempted;
	bool m_bSpawnedEntity;
}

[BaseContainerProps()]
class HST_QRFState
{
	string m_sInstanceId;
	string m_sFactionKey;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sGroupId;
	int m_iStartedAtSecond;
	int m_iETASeconds;
	bool m_bResolved;
	bool m_bSucceeded;
}

[BaseContainerProps()]
class HST_MapMarkerState
{
	string m_sMarkerId;
	string m_sLinkedId;
	string m_sLabel;
	string m_sCallsign;
	string m_sCategory;
	string m_sOwnerFactionKey;
	string m_sIconHint;
	string m_sColorHint;
	string m_sTextColorHint;
	string m_sStyleHint;
	vector m_vPosition;
	bool m_bVisible = true;
	bool m_bRuntimeNative;
}

[BaseContainerProps()]
class HST_ArsenalItemState
{
	string m_sPrefab;
	string m_sDisplayName;
	string m_sCategory;
	int m_iCount;
	bool m_bUnlocked;
}

[BaseContainerProps()]
class HST_GarageVehicleState
{
	string m_sVehicleId;
	string m_sPrefab;
	string m_sDisplayName;
	string m_sSourceZoneId;
	string m_sSourceFactionKey;
	int m_iStoredAtSecond;
	int m_iRedeployCost;
	vector m_vPosition;
	vector m_vAngles;
	float m_fFuel;
	bool m_bArmed;
	bool m_bUnlocked;
}

[BaseContainerProps()]
class HST_VehicleCargoItemState
{
	string m_sVehicleRuntimeId;
	string m_sVehiclePrefab;
	string m_sVehicleDisplayName;
	string m_sItemPrefab;
	string m_sDisplayName;
	string m_sCategory;
	int m_iCount;
	int m_iLastStoredAtSecond;
	vector m_vLastVehiclePosition;
}

[BaseContainerProps()]
class HST_EmplacementState
{
	string m_sEmplacementId;
	string m_sPrefab;
	vector m_vPosition;
	vector m_vAngles;
}

[BaseContainerProps()]
class HST_AmmoPointState
{
	string m_sAmmoPointId;
	vector m_vPosition;
}

[BaseContainerProps()]
class HST_ActiveMissionState
{
	string m_sInstanceId;
	string m_sMissionId;
	HST_EMissionStatus m_eStatus;
	HST_EMissionRuntimeMode m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_ABSTRACT;
	int m_iRemainingSeconds;
	string m_sTargetZoneId;
	string m_sSiteId;
	string m_sRuntimePrimitive;
	string m_sRuntimeEntityId;
	int m_iStartedAtSecond;
	int m_iActiveUntilSecond;
	int m_iRuntimeStartedAtSecond;
	int m_iRuntimeHoldSeconds;
	bool m_bDynamic;
	bool m_bRequested;
	bool m_bStatic;
	bool m_bRuntimeSpawned;
	bool m_bRuntimeFallback;
	bool m_bRuntimeCleanupComplete;
}

[BaseContainerProps()]
class HST_GeneratedSiteState
{
	string m_sSiteId;
	string m_sZoneId;
	string m_sRouteId;
	string m_sSourceLayerName;
	string m_sSourceCategory;
	string m_sSourceLayoutId;
	HST_EGeneratedSiteType m_eType;
	vector m_vPosition;
	vector m_vSecondaryPosition;
	int m_iRadiusMeters;
	int m_iWeight = 1;
	bool m_bValid = true;
	bool m_bOccupied;
	string m_sOwnerFactionKey;
}

[BaseContainerProps()]
class HST_GeneratedRouteState
{
	string m_sRouteId;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sSourceLayerName;
	string m_sSourceCategory;
	string m_sSourceLayoutId;
	vector m_vStartPosition;
	vector m_vMidPosition;
	vector m_vEndPosition;
	int m_iDistanceMeters;
	bool m_bRoadRoute = true;
}

[BaseContainerProps()]
class HST_MissionObjectiveState
{
	string m_sObjectiveId;
	string m_sMissionInstanceId;
	HST_EMissionObjectiveType m_eType;
	string m_sTargetId;
	string m_sTargetZoneId;
	string m_sPhysicalEntityId;
	string m_sRuntimePrimitive;
	vector m_vPosition;
	int m_iRequiredProgress = 1;
	int m_iCurrentProgress;
	int m_iHoldSeconds;
	int m_iRequiredHoldSeconds;
	bool m_bComplete;
	bool m_bFailed;
	bool m_bCleanupComplete;
	bool m_bWorldDetected;
	bool m_bAbstractFallback;
}

[BaseContainerProps()]
class HST_SupportRequestState
{
	string m_sRequestId;
	string m_sFactionKey;
	string m_sCapabilityId;
	string m_sAssetProfileId;
	string m_sStrikeKind;
	string m_sStrikeConfigResource;
	HST_ESupportRequestType m_eType;
	HST_ESupportRequestStatus m_eStatus;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sGroupId;
	string m_sRuntimeEntityId;
	vector m_vSourcePosition;
	vector m_vTargetPosition;
	int m_iRequestedAtSecond;
	int m_iETASeconds;
	int m_iAttackCost;
	int m_iSupportCost;
	int m_iMoneyCost;
	int m_iCooldownUntilSecond;
	bool m_bHelicopterStyle;
	bool m_bPlayerRequested;
	bool m_bPhysicalStrikeSpawned;
	bool m_bAbstractResolved;
	string m_sFailureReason;
}

[BaseContainerProps()]
class HST_EnemyOrderState
{
	string m_sOrderId;
	string m_sFactionKey;
	HST_EEnemyOrderType m_eType;
	HST_EEnemyOrderStatus m_eStatus;
	string m_sTargetZoneId;
	string m_sSupportRequestId;
	int m_iCreatedAtSecond;
	int m_iResolveAtSecond;
	int m_iAttackCost;
	int m_iSupportCost;
}

[BaseContainerProps()]
class HST_CivilianZoneState
{
	string m_sZoneId;
	int m_iReputation;
	int m_iWantedHeat;
	int m_iCivilianPresence;
	int m_iPolicePresence;
	int m_iRoadblockPresence;
	int m_iLastIncidentSecond;
	bool m_bUndercoverRestricted;
}

[BaseContainerProps()]
class HST_PlayerUndercoverState
{
	string m_sIdentityId;
	HST_EUndercoverStatus m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
	int m_iWantedHeat;
	int m_iCompromisedUntilSecond;
	int m_iLastCheckedSecond;
	string m_sLastReason;
}

[BaseContainerProps()]
class HST_CampaignTaskState
{
	string m_sTaskId;
	string m_sLinkedId;
	string m_sTitle;
	string m_sDescription;
	string m_sCategory;
	vector m_vPosition;
	bool m_bActive = true;
	bool m_bSucceeded;
	bool m_bFailed;
}

[BaseContainerProps()]
class HST_CampaignState
{
	static const int SCHEMA_VERSION = 10;

	int m_iSchemaVersion = SCHEMA_VERSION;
	int m_iLastLoadedSchemaVersion = SCHEMA_VERSION;
	string m_sPresetId = "rhs_everon";
	int m_iCampaignSeed = 1985;
	HST_ECampaignPhase m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_SETUP;
	int m_iElapsedSeconds;
	int m_iLastSaveSecond;
	int m_iLastRestoreSecond;
	int m_iWarLevel = 1;
	int m_iFactionMoney = 1000;
	int m_iHR = 20;
	int m_iTrainingLevel = 1;
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
	bool m_bPetrosAlive = true;
	bool m_bRestoredFromPersistence;
	int m_iPetrosDeaths;
	string m_sPetrosPrefab;
	string m_sHQCachePrefab;
	string m_sArsenalPrefab;
	string m_sHQTentPrefab;
	string m_sLastPersistenceStatus = "not tracked";

	ref array<ref HST_FactionPoolState> m_aFactionPools = {};
	ref array<ref HST_PlayerState> m_aPlayers = {};
	ref array<ref HST_ZoneState> m_aZones = {};
	ref array<ref HST_GarrisonState> m_aGarrisons = {};
	ref array<ref HST_ActiveGroupState> m_aActiveGroups = {};
	ref array<ref HST_QRFState> m_aQRFs = {};
	ref array<ref HST_MapMarkerState> m_aMapMarkers = {};
	ref array<ref HST_ArsenalItemState> m_aArsenalItems = {};
	ref array<ref HST_GarageVehicleState> m_aGarageVehicles = {};
	ref array<ref HST_VehicleCargoItemState> m_aVehicleCargoItems = {};
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

	HST_FactionPoolState FindFactionPool(string factionKey)
	{
		foreach (HST_FactionPoolState pool : m_aFactionPools)
		{
			if (pool.m_sFactionKey == factionKey)
				return pool;
		}

		return null;
	}

	HST_ZoneState FindZone(string zoneId)
	{
		foreach (HST_ZoneState zone : m_aZones)
		{
			if (zone.m_sZoneId == zoneId)
				return zone;
		}

		return null;
	}

	HST_PlayerState FindPlayer(string identityId)
	{
		foreach (HST_PlayerState player : m_aPlayers)
		{
			if (player.m_sIdentityId == identityId)
				return player;
		}

		return null;
	}

	HST_ArsenalItemState FindArsenalItem(string prefab)
	{
		foreach (HST_ArsenalItemState item : m_aArsenalItems)
		{
			if (item.m_sPrefab == prefab)
				return item;
		}

		return null;
	}

	HST_GarageVehicleState FindGarageVehicle(string vehicleId)
	{
		foreach (HST_GarageVehicleState vehicle : m_aGarageVehicles)
		{
			if (vehicle.m_sVehicleId == vehicleId)
				return vehicle;
		}

		return null;
	}

	HST_VehicleCargoItemState FindVehicleCargoItem(string vehicleRuntimeId, string itemPrefab)
	{
		foreach (HST_VehicleCargoItemState cargoItem : m_aVehicleCargoItems)
		{
			if (cargoItem.m_sVehicleRuntimeId == vehicleRuntimeId && cargoItem.m_sItemPrefab == itemPrefab)
				return cargoItem;
		}

		return null;
	}

	HST_GarrisonState FindGarrison(string zoneId, string factionKey)
	{
		foreach (HST_GarrisonState garrison : m_aGarrisons)
		{
			if (garrison.m_sZoneId == zoneId && garrison.m_sFactionKey == factionKey)
				return garrison;
		}

		return null;
	}

	HST_ActiveGroupState FindActiveGroup(string groupId)
	{
		foreach (HST_ActiveGroupState group : m_aActiveGroups)
		{
			if (group.m_sGroupId == groupId)
				return group;
		}

		return null;
	}

	HST_QRFState FindActiveQRF(string targetZoneId, string factionKey)
	{
		foreach (HST_QRFState qrf : m_aQRFs)
		{
			if (!qrf.m_bResolved && qrf.m_sTargetZoneId == targetZoneId && qrf.m_sFactionKey == factionKey)
				return qrf;
		}

		return null;
	}

	HST_ActiveMissionState FindActiveMission(string instanceId)
	{
		foreach (HST_ActiveMissionState mission : m_aActiveMissions)
		{
			if (mission.m_sInstanceId == instanceId)
				return mission;
		}

		return null;
	}

	HST_MapMarkerState FindMapMarker(string markerId)
	{
		foreach (HST_MapMarkerState marker : m_aMapMarkers)
		{
			if (marker.m_sMarkerId == markerId)
				return marker;
		}

		return null;
	}

	HST_GeneratedSiteState FindGeneratedSite(string siteId)
	{
		foreach (HST_GeneratedSiteState site : m_aGeneratedSites)
		{
			if (site.m_sSiteId == siteId)
				return site;
		}

		return null;
	}

	HST_GeneratedRouteState FindGeneratedRoute(string routeId)
	{
		foreach (HST_GeneratedRouteState route : m_aGeneratedRoutes)
		{
			if (route.m_sRouteId == routeId)
				return route;
		}

		return null;
	}

	HST_SupportRequestState FindSupportRequest(string requestId)
	{
		foreach (HST_SupportRequestState request : m_aSupportRequests)
		{
			if (request.m_sRequestId == requestId)
				return request;
		}

		return null;
	}

	HST_CivilianZoneState FindCivilianZone(string zoneId)
	{
		foreach (HST_CivilianZoneState civilianZone : m_aCivilianZones)
		{
			if (civilianZone.m_sZoneId == zoneId)
				return civilianZone;
		}

		return null;
	}

	HST_PlayerUndercoverState FindUndercoverPlayer(string identityId)
	{
		foreach (HST_PlayerUndercoverState undercover : m_aUndercoverPlayers)
		{
			if (undercover.m_sIdentityId == identityId)
				return undercover;
		}

		return null;
	}

	HST_CampaignTaskState FindCampaignTask(string taskId)
	{
		foreach (HST_CampaignTaskState task : m_aCampaignTasks)
		{
			if (task.m_sTaskId == taskId)
				return task;
		}

		return null;
	}
}
