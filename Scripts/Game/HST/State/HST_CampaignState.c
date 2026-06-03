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
	string m_sOwnerFactionKey;
	HST_EZoneType m_eType;
	vector m_vPosition;
	int m_iSupport;
	int m_iResistanceCaptureProgress;
	int m_iIncomeValue;
	int m_iGarrisonSlots;
	int m_iActivationRadiusMeters;
	bool m_bActive;
	int m_iActiveInfantryCount;
	int m_iActiveVehicleCount;
	string m_sPatrolRouteId;
	string m_sQRFRouteId;
	string m_sMissionSiteId;
	int m_iQrfCooldownUntilSecond;
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
	int m_iInfantryCount;
	int m_iVehicleCount;
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
class HST_ArsenalItemState
{
	string m_sPrefab;
	int m_iCount;
	bool m_bUnlocked;
}

[BaseContainerProps()]
class HST_GarageVehicleState
{
	string m_sVehicleId;
	string m_sPrefab;
	vector m_vPosition;
	vector m_vAngles;
	float m_fFuel;
	bool m_bArmed;
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
	int m_iRemainingSeconds;
	string m_sTargetZoneId;
}

[BaseContainerProps()]
class HST_CampaignState
{
	static const int SCHEMA_VERSION = 3;

	int m_iSchemaVersion = SCHEMA_VERSION;
	string m_sPresetId = "rhs_everon";
	int m_iCampaignSeed = 1985;
	HST_ECampaignPhase m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_SETUP;
	int m_iElapsedSeconds;
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
	vector m_vHQTentPosition;
	bool m_bHQDeployed;
	bool m_bHQRuntimeObjectsSpawned;
	bool m_bPetrosAlive = true;
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
}
