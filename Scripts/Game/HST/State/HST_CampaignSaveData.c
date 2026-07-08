[BaseContainerProps()]
class HST_CampaignSaveData
{
	int m_iSchemaVersion;
	int m_iLastLoadedSchemaVersion;
	string m_sPresetId;
	int m_iCampaignSeed;
	HST_ECampaignPhase m_ePhase;
	int m_iElapsedSeconds;
	int m_iLastSaveSecond;
	int m_iLastRestoreSecond;
	int m_iWarLevel;
	int m_iFactionMoney;
	int m_iHR;
	int m_iTrainingLevel;
	string m_sCampaignEndReason;
	string m_sCampaignEndSummary;
	int m_iCampaignEndedAtSecond;
	int m_iCampaignEndControlPercent;
	int m_iCampaignEndWarLevel;
	int m_iCampaignEndFIAZones;
	int m_iCampaignEndEnemyZones;
	string m_sCampaignEndOutcomeMode;
	int m_iCampaignEndInitialPopulation;
	int m_iCampaignEndRemainingPopulation;
	int m_iCampaignEndKilledPopulation;
	int m_iCampaignEndFIASupportPopulation;
	int m_iCampaignEndSupportPercent;
	int m_iCampaignEndAirfieldsControlled;
	int m_iCampaignEndAirfieldsTotal;
	bool m_bCampaignEndReportGenerated;
	int m_iIncomeAccumulatorSeconds;
	int m_iEnemyResourceAccumulatorSeconds;
	int m_iAggressionAccumulatorSeconds;
	string m_sCommanderIdentityId;
	string m_sHQHideoutId;
	vector m_vHQPosition;
	vector m_vPetrosPosition;
	vector m_vHQCachePosition;
	vector m_vArsenalPosition;
	vector m_vHQTentPosition;
	vector m_vHQSpawnPointPosition;
	bool m_bHQDeployed;
	bool m_bHQRuntimeObjectsSpawned;
	bool m_bPetrosAlive;
	bool m_bRestoredFromPersistence;
	int m_iPetrosDeaths;
	int m_iHQKnowledge;
	int m_iLastHQAttackSecond;
	int m_iHQThreatLevel;
	int m_iHQKnowledgeLastChangedSecond;
	int m_iLastHQActivitySecond;
	int m_iLastHQThreatScanSecond;
	string m_sLastHQKnowledgeReason;
	string m_sLastHQThreatReason;
	bool m_bDefendPetrosActive;
	string m_sDefendPetrosMissionId;
	string m_sDefendPetrosOrderId;
	string m_sDefendPetrosSupportRequestId;
	string m_sDefendPetrosAttackerGroupId;
	string m_sDefendPetrosStatus;
	string m_sDefendPetrosFailureReason;
	int m_iDefendPetrosStartedSecond;
	int m_iDefendPetrosEndsSecond;
	int m_iDefendPetrosLastUpdateSecond;
	int m_iDefendPetrosAttackerCount;
	int m_iDefendPetrosAliveAttackerCount;
	int m_iDefendPetrosKilledCount;
	bool m_bDefendPetrosOutcomeApplied;
	string m_sPetrosPrefab;
	string m_sHQCachePrefab;
	string m_sArsenalPrefab;
	string m_sHQTentPrefab;
	string m_sHQSpawnPointPrefab;
	string m_sLastPersistenceStatus;

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
	ref array<ref HST_RuntimeVehicleState> m_aRuntimeVehicles = {};
	ref array<ref HST_SavedLoadoutState> m_aSavedLoadouts = {};
	ref array<ref HST_IssuedLoadoutItemState> m_aIssuedLoadoutItems = {};
	ref array<ref HST_EmplacementState> m_aCapturedEmplacements = {};
	ref array<ref HST_AmmoPointState> m_aAmmoPoints = {};
	ref array<ref HST_ActiveMissionState> m_aActiveMissions = {};
	ref array<ref HST_GeneratedSiteState> m_aGeneratedSites = {};
	ref array<ref HST_GeneratedRouteState> m_aGeneratedRoutes = {};
	ref array<ref HST_MissionObjectiveState> m_aMissionObjectives = {};
	ref array<ref HST_MissionRuntimeEntityState> m_aMissionRuntimeEntities = {};
	ref array<ref HST_MissionAssetState> m_aMissionAssets = {};
	ref array<ref HST_SupportRequestState> m_aSupportRequests = {};
	ref array<ref HST_EnemyOrderState> m_aEnemyOrders = {};
	ref array<ref HST_EnemySupportLedgerState> m_aEnemySupportLedgers = {};
	ref array<ref HST_CivilianZoneState> m_aCivilianZones = {};
	ref array<ref HST_TownInfluenceEventState> m_aTownInfluenceEvents = {};
	ref array<ref HST_StrategicEventState> m_aStrategicEvents = {};
	ref array<ref HST_PlayerUndercoverState> m_aUndercoverPlayers = {};
	ref array<ref HST_CampaignTaskState> m_aCampaignTasks = {};

	void Capture(HST_CampaignState state)
	{
		if (!state)
			return;

		m_iSchemaVersion = state.m_iSchemaVersion;
		m_iLastLoadedSchemaVersion = state.m_iLastLoadedSchemaVersion;
		m_sPresetId = state.m_sPresetId;
		m_iCampaignSeed = state.m_iCampaignSeed;
		m_ePhase = state.m_ePhase;
		m_iElapsedSeconds = state.m_iElapsedSeconds;
		m_iLastSaveSecond = state.m_iLastSaveSecond;
		m_iLastRestoreSecond = state.m_iLastRestoreSecond;
		m_iWarLevel = state.m_iWarLevel;
		m_iFactionMoney = state.m_iFactionMoney;
		m_iHR = state.m_iHR;
		m_iTrainingLevel = state.m_iTrainingLevel;
		m_sCampaignEndReason = state.m_sCampaignEndReason;
		m_sCampaignEndSummary = state.m_sCampaignEndSummary;
		m_iCampaignEndedAtSecond = state.m_iCampaignEndedAtSecond;
		m_iCampaignEndControlPercent = state.m_iCampaignEndControlPercent;
		m_iCampaignEndWarLevel = state.m_iCampaignEndWarLevel;
		m_iCampaignEndFIAZones = state.m_iCampaignEndFIAZones;
		m_iCampaignEndEnemyZones = state.m_iCampaignEndEnemyZones;
		m_sCampaignEndOutcomeMode = state.m_sCampaignEndOutcomeMode;
		m_iCampaignEndInitialPopulation = state.m_iCampaignEndInitialPopulation;
		m_iCampaignEndRemainingPopulation = state.m_iCampaignEndRemainingPopulation;
		m_iCampaignEndKilledPopulation = state.m_iCampaignEndKilledPopulation;
		m_iCampaignEndFIASupportPopulation = state.m_iCampaignEndFIASupportPopulation;
		m_iCampaignEndSupportPercent = state.m_iCampaignEndSupportPercent;
		m_iCampaignEndAirfieldsControlled = state.m_iCampaignEndAirfieldsControlled;
		m_iCampaignEndAirfieldsTotal = state.m_iCampaignEndAirfieldsTotal;
		m_bCampaignEndReportGenerated = state.m_bCampaignEndReportGenerated;
		m_iIncomeAccumulatorSeconds = state.m_iIncomeAccumulatorSeconds;
		m_iEnemyResourceAccumulatorSeconds = state.m_iEnemyResourceAccumulatorSeconds;
		m_iAggressionAccumulatorSeconds = state.m_iAggressionAccumulatorSeconds;
		m_sCommanderIdentityId = state.m_sCommanderIdentityId;
		m_sHQHideoutId = state.m_sHQHideoutId;
		m_vHQPosition = state.m_vHQPosition;
		m_vPetrosPosition = state.m_vPetrosPosition;
		m_vHQCachePosition = state.m_vHQCachePosition;
		m_vArsenalPosition = state.m_vArsenalPosition;
		m_vHQTentPosition = state.m_vHQTentPosition;
		m_vHQSpawnPointPosition = state.m_vHQSpawnPointPosition;
		m_bHQDeployed = state.m_bHQDeployed;
		m_bHQRuntimeObjectsSpawned = false;
		m_bPetrosAlive = state.m_bPetrosAlive;
		m_bRestoredFromPersistence = state.m_bRestoredFromPersistence;
		m_iPetrosDeaths = state.m_iPetrosDeaths;
		m_iHQKnowledge = state.m_iHQKnowledge;
		m_iLastHQAttackSecond = state.m_iLastHQAttackSecond;
		m_iHQThreatLevel = state.m_iHQThreatLevel;
		m_iHQKnowledgeLastChangedSecond = state.m_iHQKnowledgeLastChangedSecond;
		m_iLastHQActivitySecond = state.m_iLastHQActivitySecond;
		m_iLastHQThreatScanSecond = state.m_iLastHQThreatScanSecond;
		m_sLastHQKnowledgeReason = state.m_sLastHQKnowledgeReason;
		m_sLastHQThreatReason = state.m_sLastHQThreatReason;
		m_bDefendPetrosActive = state.m_bDefendPetrosActive;
		m_sDefendPetrosMissionId = state.m_sDefendPetrosMissionId;
		m_sDefendPetrosOrderId = state.m_sDefendPetrosOrderId;
		m_sDefendPetrosSupportRequestId = state.m_sDefendPetrosSupportRequestId;
		m_sDefendPetrosAttackerGroupId = state.m_sDefendPetrosAttackerGroupId;
		m_sDefendPetrosStatus = state.m_sDefendPetrosStatus;
		m_sDefendPetrosFailureReason = state.m_sDefendPetrosFailureReason;
		m_iDefendPetrosStartedSecond = state.m_iDefendPetrosStartedSecond;
		m_iDefendPetrosEndsSecond = state.m_iDefendPetrosEndsSecond;
		m_iDefendPetrosLastUpdateSecond = state.m_iDefendPetrosLastUpdateSecond;
		m_iDefendPetrosAttackerCount = state.m_iDefendPetrosAttackerCount;
		m_iDefendPetrosAliveAttackerCount = state.m_iDefendPetrosAliveAttackerCount;
		m_iDefendPetrosKilledCount = state.m_iDefendPetrosKilledCount;
		m_bDefendPetrosOutcomeApplied = state.m_bDefendPetrosOutcomeApplied;
		m_sPetrosPrefab = state.m_sPetrosPrefab;
		m_sHQCachePrefab = state.m_sHQCachePrefab;
		m_sArsenalPrefab = state.m_sArsenalPrefab;
		m_sHQTentPrefab = state.m_sHQTentPrefab;
		m_sHQSpawnPointPrefab = state.m_sHQSpawnPointPrefab;
		m_sLastPersistenceStatus = state.m_sLastPersistenceStatus;

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

		m_aVehicleCargoItems.Clear();
		foreach (HST_VehicleCargoItemState cargoItem : state.m_aVehicleCargoItems)
			m_aVehicleCargoItems.Insert(CopyVehicleCargoItem(cargoItem));

		m_aRuntimeVehicles.Clear();
		foreach (HST_RuntimeVehicleState runtimeVehicle : state.m_aRuntimeVehicles)
			m_aRuntimeVehicles.Insert(CopyRuntimeVehicle(runtimeVehicle));

		m_aSavedLoadouts.Clear();
		foreach (HST_SavedLoadoutState loadout : state.m_aSavedLoadouts)
			m_aSavedLoadouts.Insert(CopySavedLoadout(loadout));

		m_aIssuedLoadoutItems.Clear();
		foreach (HST_IssuedLoadoutItemState issuedItem : state.m_aIssuedLoadoutItems)
			m_aIssuedLoadoutItems.Insert(CopyIssuedLoadoutItem(issuedItem));

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

		m_aMissionRuntimeEntities.Clear();
		foreach (HST_MissionRuntimeEntityState runtimeEntity : state.m_aMissionRuntimeEntities)
			m_aMissionRuntimeEntities.Insert(CopyMissionRuntimeEntity(runtimeEntity));

		m_aMissionAssets.Clear();
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
			m_aMissionAssets.Insert(CopyMissionAsset(asset));

		m_aSupportRequests.Clear();
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
			m_aSupportRequests.Insert(CopySupportRequest(request));

		m_aEnemyOrders.Clear();
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
			m_aEnemyOrders.Insert(CopyEnemyOrder(order));

		m_aEnemySupportLedgers.Clear();
		foreach (HST_EnemySupportLedgerState ledger : state.m_aEnemySupportLedgers)
			m_aEnemySupportLedgers.Insert(CopyEnemySupportLedger(ledger));

		m_aCivilianZones.Clear();
		foreach (HST_CivilianZoneState civilianZone : state.m_aCivilianZones)
			m_aCivilianZones.Insert(CopyCivilianZone(civilianZone));

		m_aTownInfluenceEvents.Clear();
		foreach (HST_TownInfluenceEventState influenceEvent : state.m_aTownInfluenceEvents)
			m_aTownInfluenceEvents.Insert(CopyTownInfluenceEvent(influenceEvent));

		m_aStrategicEvents.Clear();
		foreach (HST_StrategicEventState strategicEvent : state.m_aStrategicEvents)
			m_aStrategicEvents.Insert(CopyStrategicEvent(strategicEvent));

		m_aUndercoverPlayers.Clear();
		foreach (HST_PlayerUndercoverState undercover : state.m_aUndercoverPlayers)
			m_aUndercoverPlayers.Insert(CopyUndercoverPlayer(undercover));

		m_aCampaignTasks.Clear();
		foreach (HST_CampaignTaskState task : state.m_aCampaignTasks)
			m_aCampaignTasks.Insert(CopyCampaignTask(task));
	}

	HST_CampaignState Restore()
	{
		MigrateToCurrentSchema();
		HST_CampaignState state = new HST_CampaignState();
		ApplyTo(state);
		return state;
	}

	void ApplyTo(HST_CampaignState state)
	{
		if (!state)
			return;

		MigrateToCurrentSchema();
		state.m_iLastLoadedSchemaVersion = m_iLastLoadedSchemaVersion;
		state.m_iSchemaVersion = m_iSchemaVersion;
		state.m_sPresetId = m_sPresetId;
		state.m_iCampaignSeed = m_iCampaignSeed;
		state.m_ePhase = m_ePhase;
		state.m_iElapsedSeconds = m_iElapsedSeconds;
		state.m_iLastSaveSecond = m_iLastSaveSecond;
		state.m_iLastRestoreSecond = m_iLastRestoreSecond;
		state.m_iWarLevel = m_iWarLevel;
		state.m_iFactionMoney = m_iFactionMoney;
		state.m_iHR = m_iHR;
		state.m_iTrainingLevel = m_iTrainingLevel;
		state.m_sCampaignEndReason = m_sCampaignEndReason;
		state.m_sCampaignEndSummary = m_sCampaignEndSummary;
		state.m_iCampaignEndedAtSecond = m_iCampaignEndedAtSecond;
		state.m_iCampaignEndControlPercent = m_iCampaignEndControlPercent;
		state.m_iCampaignEndWarLevel = m_iCampaignEndWarLevel;
		state.m_iCampaignEndFIAZones = m_iCampaignEndFIAZones;
		state.m_iCampaignEndEnemyZones = m_iCampaignEndEnemyZones;
		state.m_sCampaignEndOutcomeMode = m_sCampaignEndOutcomeMode;
		state.m_iCampaignEndInitialPopulation = m_iCampaignEndInitialPopulation;
		state.m_iCampaignEndRemainingPopulation = m_iCampaignEndRemainingPopulation;
		state.m_iCampaignEndKilledPopulation = m_iCampaignEndKilledPopulation;
		state.m_iCampaignEndFIASupportPopulation = m_iCampaignEndFIASupportPopulation;
		state.m_iCampaignEndSupportPercent = m_iCampaignEndSupportPercent;
		state.m_iCampaignEndAirfieldsControlled = m_iCampaignEndAirfieldsControlled;
		state.m_iCampaignEndAirfieldsTotal = m_iCampaignEndAirfieldsTotal;
		state.m_bCampaignEndReportGenerated = m_bCampaignEndReportGenerated;
		state.m_iIncomeAccumulatorSeconds = m_iIncomeAccumulatorSeconds;
		state.m_iEnemyResourceAccumulatorSeconds = m_iEnemyResourceAccumulatorSeconds;
		state.m_iAggressionAccumulatorSeconds = m_iAggressionAccumulatorSeconds;
		state.m_sCommanderIdentityId = m_sCommanderIdentityId;
		state.m_sHQHideoutId = m_sHQHideoutId;
		state.m_vHQPosition = m_vHQPosition;
		state.m_vPetrosPosition = m_vPetrosPosition;
		state.m_vHQCachePosition = m_vHQCachePosition;
		state.m_vArsenalPosition = m_vArsenalPosition;
		state.m_vHQTentPosition = m_vHQTentPosition;
		state.m_vHQSpawnPointPosition = m_vHQSpawnPointPosition;
		state.m_bHQDeployed = m_bHQDeployed;
		state.m_bHQRuntimeObjectsSpawned = false;
		state.m_bPetrosAlive = m_bPetrosAlive;
		state.m_bRestoredFromPersistence = m_bRestoredFromPersistence;
		state.m_iPetrosDeaths = m_iPetrosDeaths;
		state.m_iHQKnowledge = m_iHQKnowledge;
		state.m_iLastHQAttackSecond = m_iLastHQAttackSecond;
		state.m_iHQThreatLevel = m_iHQThreatLevel;
		state.m_iHQKnowledgeLastChangedSecond = m_iHQKnowledgeLastChangedSecond;
		state.m_iLastHQActivitySecond = m_iLastHQActivitySecond;
		state.m_iLastHQThreatScanSecond = m_iLastHQThreatScanSecond;
		state.m_sLastHQKnowledgeReason = m_sLastHQKnowledgeReason;
		state.m_sLastHQThreatReason = m_sLastHQThreatReason;
		state.m_bDefendPetrosActive = m_bDefendPetrosActive;
		state.m_sDefendPetrosMissionId = m_sDefendPetrosMissionId;
		state.m_sDefendPetrosOrderId = m_sDefendPetrosOrderId;
		state.m_sDefendPetrosSupportRequestId = m_sDefendPetrosSupportRequestId;
		state.m_sDefendPetrosAttackerGroupId = m_sDefendPetrosAttackerGroupId;
		state.m_sDefendPetrosStatus = m_sDefendPetrosStatus;
		state.m_sDefendPetrosFailureReason = m_sDefendPetrosFailureReason;
		state.m_iDefendPetrosStartedSecond = m_iDefendPetrosStartedSecond;
		state.m_iDefendPetrosEndsSecond = m_iDefendPetrosEndsSecond;
		state.m_iDefendPetrosLastUpdateSecond = m_iDefendPetrosLastUpdateSecond;
		state.m_iDefendPetrosAttackerCount = m_iDefendPetrosAttackerCount;
		state.m_iDefendPetrosAliveAttackerCount = m_iDefendPetrosAliveAttackerCount;
		state.m_iDefendPetrosKilledCount = m_iDefendPetrosKilledCount;
		state.m_bDefendPetrosOutcomeApplied = m_bDefendPetrosOutcomeApplied;
		state.m_sPetrosPrefab = m_sPetrosPrefab;
		state.m_sHQCachePrefab = m_sHQCachePrefab;
		state.m_sArsenalPrefab = m_sArsenalPrefab;
		state.m_sHQTentPrefab = m_sHQTentPrefab;
		state.m_sHQSpawnPointPrefab = m_sHQSpawnPointPrefab;
		state.m_sLastPersistenceStatus = m_sLastPersistenceStatus;

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

		state.m_aVehicleCargoItems.Clear();
		foreach (HST_VehicleCargoItemState cargoItem : m_aVehicleCargoItems)
			state.m_aVehicleCargoItems.Insert(CopyVehicleCargoItem(cargoItem));

		state.m_aRuntimeVehicles.Clear();
		foreach (HST_RuntimeVehicleState runtimeVehicle : m_aRuntimeVehicles)
			state.m_aRuntimeVehicles.Insert(CopyRuntimeVehicle(runtimeVehicle));

		state.m_aSavedLoadouts.Clear();
		foreach (HST_SavedLoadoutState loadout : m_aSavedLoadouts)
			state.m_aSavedLoadouts.Insert(CopySavedLoadout(loadout));

		state.m_aIssuedLoadoutItems.Clear();
		foreach (HST_IssuedLoadoutItemState issuedItem : m_aIssuedLoadoutItems)
			state.m_aIssuedLoadoutItems.Insert(CopyIssuedLoadoutItem(issuedItem));

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

		state.m_aMissionRuntimeEntities.Clear();
		foreach (HST_MissionRuntimeEntityState runtimeEntity : m_aMissionRuntimeEntities)
			state.m_aMissionRuntimeEntities.Insert(CopyMissionRuntimeEntity(runtimeEntity));

		state.m_aMissionAssets.Clear();
		foreach (HST_MissionAssetState asset : m_aMissionAssets)
			state.m_aMissionAssets.Insert(CopyMissionAsset(asset));

		state.m_aSupportRequests.Clear();
		foreach (HST_SupportRequestState request : m_aSupportRequests)
			state.m_aSupportRequests.Insert(CopySupportRequest(request));

		state.m_aEnemyOrders.Clear();
		foreach (HST_EnemyOrderState order : m_aEnemyOrders)
			state.m_aEnemyOrders.Insert(CopyEnemyOrder(order));

		state.m_aEnemySupportLedgers.Clear();
		foreach (HST_EnemySupportLedgerState ledger : m_aEnemySupportLedgers)
			state.m_aEnemySupportLedgers.Insert(CopyEnemySupportLedger(ledger));

		state.m_aCivilianZones.Clear();
		foreach (HST_CivilianZoneState civilianZone : m_aCivilianZones)
			state.m_aCivilianZones.Insert(CopyCivilianZone(civilianZone));

		state.m_aTownInfluenceEvents.Clear();
		foreach (HST_TownInfluenceEventState influenceEvent : m_aTownInfluenceEvents)
			state.m_aTownInfluenceEvents.Insert(CopyTownInfluenceEvent(influenceEvent));

		state.m_aStrategicEvents.Clear();
		foreach (HST_StrategicEventState strategicEvent : m_aStrategicEvents)
			state.m_aStrategicEvents.Insert(CopyStrategicEvent(strategicEvent));

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
		target.m_sDisplayName = source.m_sDisplayName;
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
		target.m_sSourceLayoutId = source.m_sSourceLayoutId;
		target.m_sSourceLayerName = source.m_sSourceLayerName;
		target.m_sMarkerCallsign = source.m_sMarkerCallsign;
		target.m_sMarkerTextColor = source.m_sMarkerTextColor;
		target.m_sMarkerStyle = source.m_sMarkerStyle;
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
		target.m_sMissionInstanceId = source.m_sMissionInstanceId;
		target.m_sSupportRequestId = source.m_sSupportRequestId;
		target.m_sGarrisonZoneId = source.m_sGarrisonZoneId;
		target.m_sQRFInstanceId = source.m_sQRFInstanceId;
		target.m_sPrefab = source.m_sPrefab;
		target.m_sVehiclePrefab = source.m_sVehiclePrefab;
		target.m_sCompositionRequestId = source.m_sCompositionRequestId;
		target.m_sCompositionIntentId = source.m_sCompositionIntentId;
		target.m_sCompositionTier = source.m_sCompositionTier;
		target.m_sCompositionSummary = source.m_sCompositionSummary;
		target.m_sSpawnFallbackMode = source.m_sSpawnFallbackMode;
		target.m_sSpawnFailureReason = source.m_sSpawnFailureReason;
		target.m_vPosition = source.m_vPosition;
		target.m_sRouteId = source.m_sRouteId;
		target.m_vSourcePosition = source.m_vSourcePosition;
		target.m_vTargetPosition = source.m_vTargetPosition;
		target.m_sRuntimeEntityId = source.m_sRuntimeEntityId;
		target.m_sRuntimeStatus = source.m_sRuntimeStatus;
		target.m_iInfantryCount = source.m_iInfantryCount;
		target.m_iVehicleCount = source.m_iVehicleCount;
		target.m_iOriginalInfantryCount = source.m_iOriginalInfantryCount;
		target.m_iOriginalVehicleCount = source.m_iOriginalVehicleCount;
		target.m_iCompositionCost = source.m_iCompositionCost;
		target.m_iCompositionManpower = source.m_iCompositionManpower;
		target.m_iCompositionVehicleCount = source.m_iCompositionVehicleCount;
		target.m_iCompositionArmedVehicleCount = source.m_iCompositionArmedVehicleCount;
		target.m_iSpawnedAtSecond = source.m_iSpawnedAtSecond;
		target.m_iLastSeenAliveCount = source.m_iLastSeenAliveCount;
		target.m_iSurvivorInfantryCount = source.m_iSurvivorInfantryCount;
		target.m_iSurvivorVehicleCount = source.m_iSurvivorVehicleCount;
		target.m_iSpawnedAgentCount = source.m_iSpawnedAgentCount;
		target.m_iAssignedWaypointCount = source.m_iAssignedWaypointCount;
		target.m_iMaxObservedCrewAlive = source.m_iMaxObservedCrewAlive;
		target.m_bEverHadLivingCrew = source.m_bEverHadLivingCrew;
		target.m_bCrewPopulationTerminallyFailed = source.m_bCrewPopulationTerminallyFailed;
		target.m_sCrewPopulationFailureReason = source.m_sCrewPopulationFailureReason;
		target.m_sConvoyRuntimeStage = source.m_sConvoyRuntimeStage;
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
		target.m_sCallsign = source.m_sCallsign;
		target.m_sCategory = source.m_sCategory;
		target.m_sOwnerFactionKey = source.m_sOwnerFactionKey;
		target.m_sIconHint = source.m_sIconHint;
		target.m_sColorHint = source.m_sColorHint;
		target.m_sTextColorHint = source.m_sTextColorHint;
		target.m_sStyleHint = source.m_sStyleHint;
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
		target.m_sDamageState = source.m_sDamageState;
		target.m_bArmed = source.m_bArmed;
		target.m_sSourceVehicleKind = source.m_sSourceVehicleKind;
		target.m_bAmmoSource = source.m_bAmmoSource;
		target.m_bRepairSource = source.m_bRepairSource;
		target.m_bFuelSource = source.m_bFuelSource;
		target.m_bReported = source.m_bReported;
		target.m_bCanProvideUndercover = source.m_bCanProvideUndercover;
		target.m_iVehicleHeat = source.m_iVehicleHeat;
		target.m_iLastReportedSecond = source.m_iLastReportedSecond;
		target.m_iReportedUntilSecond = source.m_iReportedUntilSecond;
		target.m_iLastVehicleHeatChangedSecond = source.m_iLastVehicleHeatChangedSecond;
		target.m_iPassengerCompromiseCount = source.m_iPassengerCompromiseCount;
		target.m_sLastReportedReason = source.m_sLastReportedReason;
		target.m_sLastReporterZoneId = source.m_sLastReporterZoneId;
		target.m_bUnlocked = source.m_bUnlocked;
		target.m_bHadPhysicalCargo = source.m_bHadPhysicalCargo;
		foreach (HST_StoredVehicleCargoState cargoItem : source.m_aStoredCargoItems)
			target.m_aStoredCargoItems.Insert(CopyStoredVehicleCargoItem(cargoItem));
		return target;
	}

	protected HST_StoredVehicleCargoState CopyStoredVehicleCargoItem(HST_StoredVehicleCargoState source)
	{
		HST_StoredVehicleCargoState target = new HST_StoredVehicleCargoState();
		if (!source)
			return target;

		target.m_sItemPrefab = source.m_sItemPrefab;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sCategory = source.m_sCategory;
		target.m_sSource = source.m_sSource;
		target.m_iCount = source.m_iCount;
		return target;
	}

	protected HST_VehicleCargoItemState CopyVehicleCargoItem(HST_VehicleCargoItemState source)
	{
		HST_VehicleCargoItemState target = new HST_VehicleCargoItemState();
		target.m_sVehicleRuntimeId = source.m_sVehicleRuntimeId;
		target.m_sVehiclePrefab = source.m_sVehiclePrefab;
		target.m_sVehicleDisplayName = source.m_sVehicleDisplayName;
		target.m_sItemPrefab = source.m_sItemPrefab;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sCategory = source.m_sCategory;
		target.m_iCount = source.m_iCount;
		target.m_iLastStoredAtSecond = source.m_iLastStoredAtSecond;
		target.m_vLastVehiclePosition = source.m_vLastVehiclePosition;
		return target;
	}

	protected HST_RuntimeVehicleState CopyRuntimeVehicle(HST_RuntimeVehicleState source)
	{
		HST_RuntimeVehicleState target = new HST_RuntimeVehicleState();
		if (!source)
			return target;

		target.m_sVehicleRuntimeId = source.m_sVehicleRuntimeId;
		target.m_sPrefab = source.m_sPrefab;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_sZoneId = source.m_sZoneId;
		target.m_sRuntimeKind = source.m_sRuntimeKind;
		target.m_sSourceVehicleKind = source.m_sSourceVehicleKind;
		target.m_vPosition = source.m_vPosition;
		target.m_vAngles = source.m_vAngles;
		target.m_iSpawnedAtSecond = source.m_iSpawnedAtSecond;
		target.m_bDetached = source.m_bDetached;
		target.m_bDeleted = source.m_bDeleted;
		target.m_bAmmoSource = source.m_bAmmoSource;
		target.m_bRepairSource = source.m_bRepairSource;
		target.m_bFuelSource = source.m_bFuelSource;
		target.m_bReported = source.m_bReported;
		target.m_bCanProvideUndercover = source.m_bCanProvideUndercover;
		target.m_iVehicleHeat = source.m_iVehicleHeat;
		target.m_iLastReportedSecond = source.m_iLastReportedSecond;
		target.m_iReportedUntilSecond = source.m_iReportedUntilSecond;
		target.m_iLastVehicleHeatChangedSecond = source.m_iLastVehicleHeatChangedSecond;
		target.m_iPassengerCompromiseCount = source.m_iPassengerCompromiseCount;
		target.m_sLastReportedReason = source.m_sLastReportedReason;
		target.m_sLastReporterZoneId = source.m_sLastReporterZoneId;
		return target;
	}

	protected HST_LoadoutSlotState CopyLoadoutSlot(HST_LoadoutSlotState source)
	{
		HST_LoadoutSlotState target = new HST_LoadoutSlotState();
		if (!source)
			return target;

		target.m_sSlotId = source.m_sSlotId;
		target.m_sItemPrefab = source.m_sItemPrefab;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sCategory = source.m_sCategory;
		target.m_iQuantity = source.m_iQuantity;
		target.m_sWeaponSlotId = source.m_sWeaponSlotId;
		target.m_sAttachmentSlotId = source.m_sAttachmentSlotId;
		target.m_sParentSlotId = source.m_sParentSlotId;
		target.m_sStorageId = source.m_sStorageId;
		target.m_sSlotKind = source.m_sSlotKind;
		return target;
	}

	protected HST_SavedLoadoutState CopySavedLoadout(HST_SavedLoadoutState source)
	{
		HST_SavedLoadoutState target = new HST_SavedLoadoutState();
		if (!source)
			return target;

		target.m_sOwnerIdentityId = source.m_sOwnerIdentityId;
		target.m_sLoadoutId = source.m_sLoadoutId;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sCharacterPrefab = source.m_sCharacterPrefab;
		target.m_sSerializedLoadout = source.m_sSerializedLoadout;
		target.m_sClothingSummary = source.m_sClothingSummary;
		target.m_sWeaponSummary = source.m_sWeaponSummary;
		target.m_sRequiredItemsSummary = source.m_sRequiredItemsSummary;
		target.m_iUpdatedAtSecond = source.m_iUpdatedAtSecond;
		target.m_iSlotIndex = source.m_iSlotIndex;
		foreach (HST_LoadoutSlotState slot : source.m_aSlots)
			target.m_aSlots.Insert(CopyLoadoutSlot(slot));
		return target;
	}

	protected HST_IssuedLoadoutItemState CopyIssuedLoadoutItem(HST_IssuedLoadoutItemState source)
	{
		HST_IssuedLoadoutItemState target = new HST_IssuedLoadoutItemState();
		if (!source)
			return target;

		target.m_sOwnerIdentityId = source.m_sOwnerIdentityId;
		target.m_sItemPrefab = source.m_sItemPrefab;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sCategory = source.m_sCategory;
		target.m_iCount = source.m_iCount;
		target.m_bInfinite = source.m_bInfinite;
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
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_eStatus = source.m_eStatus;
		target.m_eRuntimeMode = source.m_eRuntimeMode;
		target.m_iRemainingSeconds = source.m_iRemainingSeconds;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sSiteId = source.m_sSiteId;
		target.m_vTargetPosition = source.m_vTargetPosition;
		target.m_sMarkerId = source.m_sMarkerId;
		target.m_sRuntimePrimitive = source.m_sRuntimePrimitive;
		target.m_sRuntimeType = source.m_sRuntimeType;
		target.m_sRuntimePhase = source.m_sRuntimePhase;
		target.m_sRuntimeFailureReason = source.m_sRuntimeFailureReason;
		target.m_sRuntimeEntityId = source.m_sRuntimeEntityId;
		target.m_sLastRuntimeEventKey = source.m_sLastRuntimeEventKey;
		target.m_iStartedAtSecond = source.m_iStartedAtSecond;
		target.m_iActiveUntilSecond = source.m_iActiveUntilSecond;
		target.m_iRuntimeStartedAtSecond = source.m_iRuntimeStartedAtSecond;
		target.m_iRuntimeHoldSeconds = source.m_iRuntimeHoldSeconds;
		target.m_iRuntimeETASeconds = source.m_iRuntimeETASeconds;
		target.m_iRuntimeCounterA = source.m_iRuntimeCounterA;
		target.m_iRuntimeCounterB = source.m_iRuntimeCounterB;
		target.m_iRuntimeCounterC = source.m_iRuntimeCounterC;
		target.m_iRequiredCargoCount = source.m_iRequiredCargoCount;
		target.m_iRecoveredCargoCount = source.m_iRecoveredCargoCount;
		target.m_iRequiredCaptiveCount = source.m_iRequiredCaptiveCount;
		target.m_iExtractedCaptiveCount = source.m_iExtractedCaptiveCount;
		target.m_iRequiredVehicleCount = source.m_iRequiredVehicleCount;
		target.m_iCapturedVehicleCount = source.m_iCapturedVehicleCount;
		target.m_iRuntimePickupCount = source.m_iRuntimePickupCount;
		target.m_iRuntimeDeliveryCount = source.m_iRuntimeDeliveryCount;
		target.m_iRuntimeDestroyedCount = source.m_iRuntimeDestroyedCount;
		target.m_bDynamic = source.m_bDynamic;
		target.m_bRequested = source.m_bRequested;
		target.m_bStatic = source.m_bStatic;
		target.m_bRuntimeSpawned = source.m_bRuntimeSpawned;
		target.m_bRuntimeFallback = source.m_bRuntimeFallback;
		target.m_bRuntimeCleanupComplete = source.m_bRuntimeCleanupComplete;
		target.m_bCreatedNotificationSent = source.m_bCreatedNotificationSent;
		target.m_bCompletedNotificationSent = source.m_bCompletedNotificationSent;
		target.m_bFailedNotificationSent = source.m_bFailedNotificationSent;
		target.m_bExpiredNotificationSent = source.m_bExpiredNotificationSent;
		target.m_bConvoyArrivalOutcomeApplied = source.m_bConvoyArrivalOutcomeApplied;
		target.m_bConvoyCrewEliminatedOutcomeApplied = source.m_bConvoyCrewEliminatedOutcomeApplied;
		target.m_bConvoyVehicleCapturedOutcomeApplied = source.m_bConvoyVehicleCapturedOutcomeApplied;
		target.m_bConvoyCargoDeliveredOutcomeApplied = source.m_bConvoyCargoDeliveredOutcomeApplied;
		target.m_bConvoyExpiredOutcomeApplied = source.m_bConvoyExpiredOutcomeApplied;
		target.m_sConvoyOutcomeSummary = source.m_sConvoyOutcomeSummary;
		foreach (HST_GunShopItemState shopItem : source.m_aGunShopItems)
			target.m_aGunShopItems.Insert(CopyGunShopItem(shopItem));
		target.m_sGunShopSellerAssetId = source.m_sGunShopSellerAssetId;
		target.m_sGunShopDeliveryDriverAssetId = source.m_sGunShopDeliveryDriverAssetId;
		target.m_sGunShopDeliveryVehicleAssetId = source.m_sGunShopDeliveryVehicleAssetId;
		target.m_sGunShopDeliveryMarkerId = source.m_sGunShopDeliveryMarkerId;
		target.m_vGunShopSellerPosition = source.m_vGunShopSellerPosition;
		target.m_vGunShopDeliveryPosition = source.m_vGunShopDeliveryPosition;
		target.m_bGunShopStockGenerated = source.m_bGunShopStockGenerated;
		target.m_bGunShopPurchaseMade = source.m_bGunShopPurchaseMade;
		target.m_bGunShopPurchaseNoticeSent = source.m_bGunShopPurchaseNoticeSent;
		target.m_bGunShopExpiryNoticeSent = source.m_bGunShopExpiryNoticeSent;
		target.m_bGunShopDeliverySpawned = source.m_bGunShopDeliverySpawned;
		target.m_bGunShopDeliveryNoticeSent = source.m_bGunShopDeliveryNoticeSent;
		target.m_bGunShopDeliveryArrived = source.m_bGunShopDeliveryArrived;
		target.m_iGunShopPurchasedTotal = source.m_iGunShopPurchasedTotal;
		target.m_iGunShopDeliveryStartedAtSecond = source.m_iGunShopDeliveryStartedAtSecond;
		return target;
	}

	protected HST_GunShopItemState CopyGunShopItem(HST_GunShopItemState source)
	{
		HST_GunShopItemState target = new HST_GunShopItemState();
		if (!source)
			return target;

		target.m_sItemId = source.m_sItemId;
		target.m_sPrefab = source.m_sPrefab;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sCategory = source.m_sCategory;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_iAvailableCount = source.m_iAvailableCount;
		target.m_iPurchasedCount = source.m_iPurchasedCount;
		target.m_iBuyCost = source.m_iBuyCost;
		target.m_iSellCost = source.m_iSellCost;
		target.m_bCanSell = source.m_bCanSell;
		return target;
	}

	protected HST_GeneratedSiteState CopyGeneratedSite(HST_GeneratedSiteState source)
	{
		HST_GeneratedSiteState target = new HST_GeneratedSiteState();
		target.m_sSiteId = source.m_sSiteId;
		target.m_sZoneId = source.m_sZoneId;
		target.m_sRouteId = source.m_sRouteId;
		target.m_sSourceLayerName = source.m_sSourceLayerName;
		target.m_sSourceCategory = source.m_sSourceCategory;
		target.m_sSourceLayoutId = source.m_sSourceLayoutId;
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
		if (!source)
			return target;

		target.m_sRouteId = source.m_sRouteId;
		target.m_sSourceZoneId = source.m_sSourceZoneId;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sSourceLayerName = source.m_sSourceLayerName;
		target.m_sSourceCategory = source.m_sSourceCategory;
		target.m_sSourceLayoutId = source.m_sSourceLayoutId;
		target.m_vStartPosition = source.m_vStartPosition;
		target.m_vMidPosition = source.m_vMidPosition;
		target.m_vEndPosition = source.m_vEndPosition;
		target.m_iDistanceMeters = source.m_iDistanceMeters;
		target.m_iWaypointCount = source.m_iWaypointCount;
		target.m_bRoadRoute = source.m_bRoadRoute;
		target.m_bValidatedForVehicles = source.m_bValidatedForVehicles;
		target.m_sValidationFailureReason = source.m_sValidationFailureReason;
		foreach (HST_RouteWaypointState waypoint : source.m_aWaypoints)
			target.m_aWaypoints.Insert(CopyRouteWaypoint(waypoint));
		BackfillGeneratedRouteWaypoints(target);
		return target;
	}

	protected HST_RouteWaypointState CopyRouteWaypoint(HST_RouteWaypointState source)
	{
		HST_RouteWaypointState target = new HST_RouteWaypointState();
		if (!source)
			return target;

		target.m_sRouteId = source.m_sRouteId;
		target.m_iIndex = source.m_iIndex;
		target.m_vPosition = source.m_vPosition;
		target.m_iRadiusMeters = source.m_iRadiusMeters;
		target.m_sHint = source.m_sHint;
		return target;
	}

	protected void BackfillGeneratedRouteWaypoints(HST_GeneratedRouteState route)
	{
		if (!route)
			return;

		if (route.m_aWaypoints.Count() == 0)
		{
			route.m_aWaypoints.Insert(CreateRouteWaypoint(route.m_sRouteId, 0, route.m_vStartPosition, "start"));
			route.m_aWaypoints.Insert(CreateRouteWaypoint(route.m_sRouteId, 1, route.m_vMidPosition, "midpoint"));
			route.m_aWaypoints.Insert(CreateRouteWaypoint(route.m_sRouteId, 2, route.m_vEndPosition, "destination"));
		}

		for (int i = 0; i < route.m_aWaypoints.Count(); i++)
		{
			HST_RouteWaypointState waypoint = route.m_aWaypoints[i];
			if (!waypoint)
				continue;

			waypoint.m_sRouteId = route.m_sRouteId;
			waypoint.m_iIndex = i;
			if (waypoint.m_iRadiusMeters <= 0)
				waypoint.m_iRadiusMeters = 35;
			if (waypoint.m_sHint.IsEmpty())
				waypoint.m_sHint = "route";
		}

		route.m_iWaypointCount = route.m_aWaypoints.Count();
		if (route.m_iDistanceMeters <= 0)
			route.m_iDistanceMeters = CalculateRouteDistanceMeters(route);
		if (route.m_aWaypoints.Count() >= 3)
		{
			route.m_vStartPosition = route.m_aWaypoints[0].m_vPosition;
			route.m_vMidPosition = route.m_aWaypoints[1].m_vPosition;
			route.m_vEndPosition = route.m_aWaypoints[route.m_aWaypoints.Count() - 1].m_vPosition;
		}
	}

	protected HST_RouteWaypointState CreateRouteWaypoint(string routeId, int index, vector position, string hint)
	{
		HST_RouteWaypointState waypoint = new HST_RouteWaypointState();
		waypoint.m_sRouteId = routeId;
		waypoint.m_iIndex = index;
		waypoint.m_vPosition = position;
		waypoint.m_iRadiusMeters = 35;
		waypoint.m_sHint = hint;
		return waypoint;
	}

	protected int CalculateRouteDistanceMeters(HST_GeneratedRouteState route)
	{
		if (!route || route.m_aWaypoints.Count() < 2)
			return 0;

		float distance;
		for (int i = 1; i < route.m_aWaypoints.Count(); i++)
		{
			HST_RouteWaypointState previous = route.m_aWaypoints[i - 1];
			HST_RouteWaypointState current = route.m_aWaypoints[i];
			if (!previous || !current)
				continue;

			distance += Math.Sqrt(DistanceSq2D(previous.m_vPosition, current.m_vPosition));
		}

		return Math.Round(distance);
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}

	protected HST_MissionObjectiveState CopyMissionObjective(HST_MissionObjectiveState source)
	{
		HST_MissionObjectiveState target = new HST_MissionObjectiveState();
		target.m_sObjectiveId = source.m_sObjectiveId;
		target.m_sMissionInstanceId = source.m_sMissionInstanceId;
		target.m_eType = source.m_eType;
		target.m_sLabel = source.m_sLabel;
		target.m_sRequirementText = source.m_sRequirementText;
		target.m_sTargetId = source.m_sTargetId;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sPhysicalEntityId = source.m_sPhysicalEntityId;
		target.m_sLinkedRuntimeEntityId = source.m_sLinkedRuntimeEntityId;
		target.m_sRuntimePrimitive = source.m_sRuntimePrimitive;
		target.m_vPosition = source.m_vPosition;
		target.m_iRequiredProgress = source.m_iRequiredProgress;
		target.m_iCurrentProgress = source.m_iCurrentProgress;
		target.m_iHoldSeconds = source.m_iHoldSeconds;
		target.m_iRequiredHoldSeconds = source.m_iRequiredHoldSeconds;
		target.m_iCurrentCount = source.m_iCurrentCount;
		target.m_iRequiredCount = source.m_iRequiredCount;
		target.m_bExtractionStarted = source.m_bExtractionStarted;
		target.m_bDeliveryStarted = source.m_bDeliveryStarted;
		target.m_bComplete = source.m_bComplete;
		target.m_bFailed = source.m_bFailed;
		target.m_bCleanupComplete = source.m_bCleanupComplete;
		target.m_bWorldDetected = source.m_bWorldDetected;
		target.m_bAbstractFallback = source.m_bAbstractFallback;
		return target;
	}

	protected HST_MissionRuntimeEntityState CopyMissionRuntimeEntity(HST_MissionRuntimeEntityState source)
	{
		HST_MissionRuntimeEntityState target = new HST_MissionRuntimeEntityState();
		if (!source)
			return target;

		target.m_sRuntimeEntityId = source.m_sRuntimeEntityId;
		target.m_sMissionInstanceId = source.m_sMissionInstanceId;
		target.m_sKind = source.m_sKind;
		target.m_sPrefab = source.m_sPrefab;
		target.m_vPosition = source.m_vPosition;
		target.m_vAngles = source.m_vAngles;
		target.m_bSpawned = source.m_bSpawned;
		target.m_bDestroyed = source.m_bDestroyed;
		target.m_bRecovered = source.m_bRecovered;
		return target;
	}

	protected HST_MissionAssetState CopyMissionAsset(HST_MissionAssetState source)
	{
		HST_MissionAssetState target = new HST_MissionAssetState();
		if (!source)
			return target;

		target.m_sAssetId = source.m_sAssetId;
		target.m_sMissionInstanceId = source.m_sMissionInstanceId;
		target.m_sKind = source.m_sKind;
		target.m_sRole = source.m_sRole;
		target.m_sPrefab = source.m_sPrefab;
		target.m_sEntityId = source.m_sEntityId;
		target.m_sCarriedByVehicleId = source.m_sCarriedByVehicleId;
		target.m_sLastInteraction = source.m_sLastInteraction;
		target.m_bSpawned = source.m_bSpawned;
		target.m_bPickedUp = source.m_bPickedUp;
		target.m_bDelivered = source.m_bDelivered;
		target.m_bDestroyed = source.m_bDestroyed;
		target.m_bAlive = source.m_bAlive;
		target.m_bAttachedToCarrier = source.m_bAttachedToCarrier;
		target.m_bOutcomeApplied = source.m_bOutcomeApplied;
		target.m_sOutcomeKind = source.m_sOutcomeKind;
		target.m_vSourcePosition = source.m_vSourcePosition;
		target.m_vTargetPosition = source.m_vTargetPosition;
		target.m_vCurrentPosition = source.m_vCurrentPosition;
		target.m_vLastKnownPosition = source.m_vLastKnownPosition;
		target.m_iDeadlineSecond = source.m_iDeadlineSecond;
		target.m_iCargoCapacityCost = source.m_iCargoCapacityCost;
		target.m_iInteractionRadiusMeters = source.m_iInteractionRadiusMeters;
		return target;
	}

	protected HST_SupportRequestState CopySupportRequest(HST_SupportRequestState source)
	{
		HST_SupportRequestState target = new HST_SupportRequestState();
		target.m_sRequestId = source.m_sRequestId;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_sCapabilityId = source.m_sCapabilityId;
		target.m_sAssetProfileId = source.m_sAssetProfileId;
		target.m_sCompositionRequestId = source.m_sCompositionRequestId;
		target.m_sCompositionIntentId = source.m_sCompositionIntentId;
		target.m_sCompositionTier = source.m_sCompositionTier;
		target.m_sCompositionSummary = source.m_sCompositionSummary;
		target.m_sCompositionFailureReason = source.m_sCompositionFailureReason;
		target.m_sStrikeKind = source.m_sStrikeKind;
		target.m_sStrikeConfigResource = source.m_sStrikeConfigResource;
		target.m_eType = source.m_eType;
		target.m_eStatus = source.m_eStatus;
		target.m_sSourceZoneId = source.m_sSourceZoneId;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sGroupId = source.m_sGroupId;
		target.m_sRuntimeEntityId = source.m_sRuntimeEntityId;
		target.m_sDeploymentRouteId = source.m_sDeploymentRouteId;
		target.m_sDeploymentPlacementType = source.m_sDeploymentPlacementType;
		target.m_sDeploymentSummary = source.m_sDeploymentSummary;
		target.m_vSourcePosition = source.m_vSourcePosition;
		target.m_vTargetPosition = source.m_vTargetPosition;
		target.m_iRequestedAtSecond = source.m_iRequestedAtSecond;
		target.m_iETASeconds = source.m_iETASeconds;
		target.m_iAttackCost = source.m_iAttackCost;
		target.m_iSupportCost = source.m_iSupportCost;
		target.m_iMoneyCost = source.m_iMoneyCost;
		target.m_iHRCost = source.m_iHRCost;
		target.m_iPlannedInfantryCount = source.m_iPlannedInfantryCount;
		target.m_iRefundedHR = source.m_iRefundedHR;
		target.m_iCompositionCost = source.m_iCompositionCost;
		target.m_iCompositionManpower = source.m_iCompositionManpower;
		target.m_iCompositionVehicleCount = source.m_iCompositionVehicleCount;
		target.m_iCompositionArmedVehicleCount = source.m_iCompositionArmedVehicleCount;
		target.m_iDeploymentTargetDistanceMeters = source.m_iDeploymentTargetDistanceMeters;
		target.m_iDeploymentRoadDistanceMeters = source.m_iDeploymentRoadDistanceMeters;
		target.m_iDeploymentHQDistanceMeters = source.m_iDeploymentHQDistanceMeters;
		target.m_iCooldownUntilSecond = source.m_iCooldownUntilSecond;
		target.m_bHelicopterStyle = source.m_bHelicopterStyle;
		target.m_bPlayerRequested = source.m_bPlayerRequested;
		target.m_bPhysicalStrikeSpawned = source.m_bPhysicalStrikeSpawned;
		target.m_bDeploymentRoadResolved = source.m_bDeploymentRoadResolved;
		target.m_bDeploymentVehicleSafe = source.m_bDeploymentVehicleSafe;
		target.m_bDeploymentVehicleSafeRequired = source.m_bDeploymentVehicleSafeRequired;
		target.m_bAbstractResolved = source.m_bAbstractResolved;
		target.m_sRuntimeStatus = source.m_sRuntimeStatus;
		target.m_sResolutionKind = source.m_sResolutionKind;
		target.m_sPhysicalizationMode = source.m_sPhysicalizationMode;
		target.m_iActivatedAtSecond = source.m_iActivatedAtSecond;
		target.m_iPhysicalizedAtSecond = source.m_iPhysicalizedAtSecond;
		target.m_iResolvedAtSecond = source.m_iResolvedAtSecond;
		target.m_iRecallRequestedAtSecond = source.m_iRecallRequestedAtSecond;
		target.m_vRecallExitPosition = source.m_vRecallExitPosition;
		target.m_bPhysicalized = source.m_bPhysicalized;
		target.m_bOutcomeApplied = source.m_bOutcomeApplied;
		target.m_bRecallRequested = source.m_bRecallRequested;
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
		target.m_sCompositionRequestId = source.m_sCompositionRequestId;
		target.m_sCompositionIntentId = source.m_sCompositionIntentId;
		target.m_sCompositionTier = source.m_sCompositionTier;
		target.m_sCompositionSummary = source.m_sCompositionSummary;
		target.m_sCompositionFailureReason = source.m_sCompositionFailureReason;
		target.m_sSupportRequestId = source.m_sSupportRequestId;
		target.m_sGroupId = source.m_sGroupId;
		target.m_sRuntimeStatus = source.m_sRuntimeStatus;
		target.m_sResolutionKind = source.m_sResolutionKind;
		target.m_sFailureReason = source.m_sFailureReason;
		target.m_vSourcePosition = source.m_vSourcePosition;
		target.m_vTargetPosition = source.m_vTargetPosition;
		target.m_iCreatedAtSecond = source.m_iCreatedAtSecond;
		target.m_iResolveAtSecond = source.m_iResolveAtSecond;
		target.m_iPhysicalizedAtSecond = source.m_iPhysicalizedAtSecond;
		target.m_iResolvedAtSecond = source.m_iResolvedAtSecond;
		target.m_iAttackCost = source.m_iAttackCost;
		target.m_iSupportCost = source.m_iSupportCost;
		target.m_iRefundedAttackResources = source.m_iRefundedAttackResources;
		target.m_iRefundedSupportResources = source.m_iRefundedSupportResources;
		target.m_iCompositionCost = source.m_iCompositionCost;
		target.m_iCompositionManpower = source.m_iCompositionManpower;
		target.m_iCompositionVehicleCount = source.m_iCompositionVehicleCount;
		target.m_iCompositionArmedVehicleCount = source.m_iCompositionArmedVehicleCount;
		target.m_bPhysicalized = source.m_bPhysicalized;
		target.m_bAbstractResolved = source.m_bAbstractResolved;
		target.m_bOutcomeApplied = source.m_bOutcomeApplied;
		target.m_bResourceRefundApplied = source.m_bResourceRefundApplied;
		return target;
	}

	protected HST_EnemySupportLedgerState CopyEnemySupportLedger(HST_EnemySupportLedgerState source)
	{
		HST_EnemySupportLedgerState target = new HST_EnemySupportLedgerState();
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_sZoneId = source.m_sZoneId;
		target.m_sLastDecisionReason = source.m_sLastDecisionReason;
		target.m_iRecentDamageScore = source.m_iRecentDamageScore;
		target.m_iLastDamageSecond = source.m_iLastDamageSecond;
		target.m_iAttackSpent = source.m_iAttackSpent;
		target.m_iSupportSpent = source.m_iSupportSpent;
		target.m_iLastSpendSecond = source.m_iLastSpendSecond;
		target.m_iCooldownUntilSecond = source.m_iCooldownUntilSecond;
		target.m_iRefundedAttackResources = source.m_iRefundedAttackResources;
		target.m_iRefundedSupportResources = source.m_iRefundedSupportResources;
		return target;
	}

	protected HST_CivilianZoneState CopyCivilianZone(HST_CivilianZoneState source)
	{
		HST_CivilianZoneState target = new HST_CivilianZoneState();
		target.m_sZoneId = source.m_sZoneId;
		target.m_iReputation = source.m_iReputation;
		target.m_iFIASupport = source.m_iFIASupport;
		target.m_iOccupierSupport = source.m_iOccupierSupport;
		target.m_iWantedHeat = source.m_iWantedHeat;
		target.m_iCivilianPresence = source.m_iCivilianPresence;
		target.m_iPolicePresence = source.m_iPolicePresence;
		target.m_iRoadblockPresence = source.m_iRoadblockPresence;
		target.m_iLastIncidentSecond = source.m_iLastIncidentSecond;
		target.m_sLastIncidentReason = source.m_sLastIncidentReason;
		target.m_iLastSupportChangeSecond = source.m_iLastSupportChangeSecond;
		target.m_iLastRoadblockScanSecond = source.m_iLastRoadblockScanSecond;
		target.m_iLastPoliceScanSecond = source.m_iLastPoliceScanSecond;
		target.m_sLastSecurityReason = source.m_sLastSecurityReason;
		target.m_bUndercoverRestricted = source.m_bUndercoverRestricted;
		target.m_iPopulationRemaining = source.m_iPopulationRemaining;
		target.m_iPopulationKilled = source.m_iPopulationKilled;
		target.m_iInfluenceEventCount = source.m_iInfluenceEventCount;
		target.m_iActiveInfluenceModifierCount = source.m_iActiveInfluenceModifierCount;
		target.m_iExpiredInfluenceModifierCount = source.m_iExpiredInfluenceModifierCount;
		target.m_iLastInfluenceEventSecond = source.m_iLastInfluenceEventSecond;
		target.m_sLastInfluenceEventId = source.m_sLastInfluenceEventId;
		target.m_sLastInfluenceKind = source.m_sLastInfluenceKind;
		target.m_sLastInfluenceReason = source.m_sLastInfluenceReason;
		return target;
	}

	protected HST_TownInfluenceEventState CopyTownInfluenceEvent(HST_TownInfluenceEventState source)
	{
		HST_TownInfluenceEventState target = new HST_TownInfluenceEventState();
		target.m_sEventId = source.m_sEventId;
		target.m_sZoneId = source.m_sZoneId;
		target.m_sKind = source.m_sKind;
		target.m_sSourceId = source.m_sSourceId;
		target.m_sReason = source.m_sReason;
		target.m_iCreatedAtSecond = source.m_iCreatedAtSecond;
		target.m_iExpiresAtSecond = source.m_iExpiresAtSecond;
		target.m_iFIASupportDelta = source.m_iFIASupportDelta;
		target.m_iOccupierSupportDelta = source.m_iOccupierSupportDelta;
		target.m_iReputationDelta = source.m_iReputationDelta;
		target.m_iHeatDelta = source.m_iHeatDelta;
		target.m_iPopulationDelta = source.m_iPopulationDelta;
		target.m_iPoliceDelta = source.m_iPoliceDelta;
		target.m_iRoadblockDelta = source.m_iRoadblockDelta;
		target.m_bApplied = source.m_bApplied;
		return target;
	}

	protected HST_StrategicEventState CopyStrategicEvent(HST_StrategicEventState source)
	{
		HST_StrategicEventState target = new HST_StrategicEventState();
		target.m_sEventId = source.m_sEventId;
		target.m_sKind = source.m_sKind;
		target.m_sSourceType = source.m_sSourceType;
		target.m_sSourceId = source.m_sSourceId;
		target.m_sMissionId = source.m_sMissionId;
		target.m_sMissionInstanceId = source.m_sMissionInstanceId;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sTargetFactionKey = source.m_sTargetFactionKey;
		target.m_sReason = source.m_sReason;
		target.m_sSummary = source.m_sSummary;
		target.m_iCreatedAtSecond = source.m_iCreatedAtSecond;
		target.m_iFactionMoneyDelta = source.m_iFactionMoneyDelta;
		target.m_iHRDelta = source.m_iHRDelta;
		target.m_iAggressionDelta = source.m_iAggressionDelta;
		target.m_iAttackResourceDelta = source.m_iAttackResourceDelta;
		target.m_iSupportResourceDelta = source.m_iSupportResourceDelta;
		target.m_iTownSupportDelta = source.m_iTownSupportDelta;
		target.m_iCaptureProgressDelta = source.m_iCaptureProgressDelta;
		target.m_iHQKnowledgeDelta = source.m_iHQKnowledgeDelta;
		target.m_sOwnerBefore = source.m_sOwnerBefore;
		target.m_sOwnerAfter = source.m_sOwnerAfter;
		target.m_iSupportBefore = source.m_iSupportBefore;
		target.m_iSupportAfter = source.m_iSupportAfter;
		target.m_iCaptureProgressBefore = source.m_iCaptureProgressBefore;
		target.m_iCaptureProgressAfter = source.m_iCaptureProgressAfter;
		target.m_iHQKnowledgeBefore = source.m_iHQKnowledgeBefore;
		target.m_iHQKnowledgeAfter = source.m_iHQKnowledgeAfter;
		target.m_sVehicleRuntimeId = source.m_sVehicleRuntimeId;
		target.m_iVehicleHeatBefore = source.m_iVehicleHeatBefore;
		target.m_iVehicleHeatAfter = source.m_iVehicleHeatAfter;
		target.m_iVehicleHeatDelta = source.m_iVehicleHeatDelta;
		target.m_bVehicleReportedBefore = source.m_bVehicleReportedBefore;
		target.m_bVehicleReportedAfter = source.m_bVehicleReportedAfter;
		target.m_iVehicleReportedUntilBefore = source.m_iVehicleReportedUntilBefore;
		target.m_iVehicleReportedUntilAfter = source.m_iVehicleReportedUntilAfter;
		target.m_iVehicleReportedUntilDelta = source.m_iVehicleReportedUntilDelta;
		target.m_bApplied = source.m_bApplied;
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
		target.m_bUndercoverRequested = source.m_bUndercoverRequested;
		target.m_bUndercoverApplied = source.m_bUndercoverApplied;
		target.m_bEnforcementEnabled = source.m_bEnforcementEnabled;
		target.m_sAppliedMode = source.m_sAppliedMode;
		target.m_sLastCompromiseReason = source.m_sLastCompromiseReason;
		target.m_sLastDetectionSource = source.m_sLastDetectionSource;
		target.m_sLastEnforcementZoneId = source.m_sLastEnforcementZoneId;
		target.m_iLastEnforcementSecond = source.m_iLastEnforcementSecond;
		target.m_iLastCompromisedSecond = source.m_iLastCompromisedSecond;
		target.m_iDetectionScore = source.m_iDetectionScore;
		target.m_iRoadblockScanCount = source.m_iRoadblockScanCount;
		target.m_iPoliceScanCount = source.m_iPoliceScanCount;
		target.m_bLastRoadblockScanFailed = source.m_bLastRoadblockScanFailed;
		target.m_bLastPoliceScanFailed = source.m_bLastPoliceScanFailed;
		target.m_bLastEligibilityResult = source.m_bLastEligibilityResult;
		target.m_sLastZoneId = source.m_sLastZoneId;
		target.m_sLastEligibilitySummary = source.m_sLastEligibilitySummary;
		target.m_sClothingReason = source.m_sClothingReason;
		target.m_sWeaponReason = source.m_sWeaponReason;
		target.m_sVehicleReason = source.m_sVehicleReason;
		target.m_sOffroadReason = source.m_sOffroadReason;
		target.m_sEnemyProximityReason = source.m_sEnemyProximityReason;
		target.m_sWantedHeatReason = source.m_sWantedHeatReason;
		target.m_iLastEligibilityCheckSecond = source.m_iLastEligibilityCheckSecond;
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

	void MigrateToCurrentSchema()
	{
		int restoredSchemaVersion = m_iSchemaVersion;
		if (restoredSchemaVersion <= 0)
			restoredSchemaVersion = 1;

		m_iLastLoadedSchemaVersion = restoredSchemaVersion;
		m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		if (m_sLastPersistenceStatus.IsEmpty())
			m_sLastPersistenceStatus = "migrated local save data";
		m_iHQKnowledge = Math.Max(0, Math.Min(100, m_iHQKnowledge));
		if (m_sDefendPetrosStatus.IsEmpty())
			m_sDefendPetrosStatus = "inactive";
		if (m_iHQThreatLevel <= 0 && m_iHQKnowledge > 0)
			m_iHQThreatLevel = Math.Min(100, m_iHQKnowledge);
		if (m_sLastHQKnowledgeReason.IsEmpty())
			m_sLastHQKnowledgeReason = "legacy/backfilled";
		if (m_sLastHQThreatReason.IsEmpty())
			m_sLastHQThreatReason = "legacy/backfilled";
		if (!m_sDefendPetrosMissionId.IsEmpty())
			m_bDefendPetrosActive = true;
		if (restoredSchemaVersion < 26)
		{
			if (m_bHQDeployed && !IsZeroVector(m_vHQPosition))
			{
				if (IsZeroVector(m_vHQSpawnPointPosition))
					m_vHQSpawnPointPosition = m_vHQPosition + "12 0 0";
				if (m_sHQSpawnPointPrefab.IsEmpty())
					m_sHQSpawnPointPrefab = HST_HQService.HQ_SPAWN_POINT_PREFAB;
				m_bHQRuntimeObjectsSpawned = false;
			}
			else
			{
				m_vHQSpawnPointPosition = "0 0 0";
				m_sHQSpawnPointPrefab = "";
			}
		}

		if (restoredSchemaVersion < 25 && (m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_WON || m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_LOST))
		{
			int fiaZones;
			int enemyZones;
			foreach (HST_ZoneState zone : m_aZones)
			{
				if (!zone || zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
					continue;

				if (zone.m_sOwnerFactionKey == "FIA")
					fiaZones++;
				else
					enemyZones++;
			}

			int totalZones = fiaZones + enemyZones;
			if (totalZones > 0 && m_iCampaignEndControlPercent <= 0)
				m_iCampaignEndControlPercent = Math.Round(fiaZones * 100.0 / totalZones);
			if (m_iCampaignEndWarLevel <= 0)
				m_iCampaignEndWarLevel = Math.Max(1, m_iWarLevel);
			if (m_iCampaignEndFIAZones <= 0)
				m_iCampaignEndFIAZones = fiaZones;
			if (m_iCampaignEndEnemyZones <= 0)
				m_iCampaignEndEnemyZones = enemyZones;
			if (m_iCampaignEndedAtSecond <= 0)
				m_iCampaignEndedAtSecond = m_iElapsedSeconds;
			if (m_sCampaignEndReason.IsEmpty())
				m_sCampaignEndReason = "legacy campaign end/backfilled";
			if (m_sCampaignEndSummary.IsEmpty())
				m_sCampaignEndSummary = string.Format("Migrated schema %1 campaign end with FIA zones %2 and enemy zones %3.", restoredSchemaVersion, fiaZones, enemyZones);
			if (m_sCampaignEndOutcomeMode.IsEmpty())
				m_sCampaignEndOutcomeMode = "legacy_control";
			m_bCampaignEndReportGenerated = true;
		}
		foreach (HST_ActiveMissionState mission : m_aActiveMissions)
		{
			if (!mission)
				continue;

			if (mission.m_sRuntimePrimitive.IsEmpty())
				mission.m_sRuntimePrimitive = "abstract_fallback";
			if (!mission.m_bRuntimeSpawned && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
				mission.m_bRuntimeFallback = true;
			if (mission.m_iRuntimeStartedAtSecond <= 0)
				mission.m_iRuntimeStartedAtSecond = mission.m_iStartedAtSecond;
			if (mission.m_iRuntimeETASeconds <= 0 && mission.m_iRuntimeCounterB > 0)
				mission.m_iRuntimeETASeconds = Math.Max(0, mission.m_iRuntimeCounterB - mission.m_iRuntimeCounterA);
			if (mission.m_sLastRuntimeEventKey.IsEmpty())
				mission.m_sLastRuntimeEventKey = mission.m_sRuntimePhase;
		}

		foreach (HST_GeneratedRouteState route : m_aGeneratedRoutes)
			BackfillGeneratedRouteWaypoints(route);

		foreach (HST_MissionObjectiveState objective : m_aMissionObjectives)
		{
			if (!objective)
				continue;

			if (objective.m_sRuntimePrimitive.IsEmpty())
				objective.m_sRuntimePrimitive = "abstract_fallback";
			if (objective.m_iRequiredHoldSeconds <= 0 && objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA)
				objective.m_iRequiredHoldSeconds = 60;
			if (!objective.m_bWorldDetected && !objective.m_bComplete)
				objective.m_bAbstractFallback = true;
		}

		foreach (HST_MissionAssetState asset : m_aMissionAssets)
		{
			if (!asset)
				continue;

			if (asset.m_sAssetId.IsEmpty())
				asset.m_sAssetId = asset.m_sEntityId;
			if (asset.m_sEntityId.IsEmpty())
				asset.m_sEntityId = asset.m_sAssetId;
			if (asset.m_sKind.IsEmpty())
				asset.m_sKind = "mission_asset";
			if (asset.m_sRole.IsEmpty())
				asset.m_sRole = asset.m_sKind;
			if (!asset.m_bAlive && !asset.m_bDestroyed)
				asset.m_bAlive = true;
			if (IsZeroVector(asset.m_vCurrentPosition))
				asset.m_vCurrentPosition = asset.m_vSourcePosition;
			if (IsZeroVector(asset.m_vLastKnownPosition))
				asset.m_vLastKnownPosition = asset.m_vCurrentPosition;
			if (asset.m_iCargoCapacityCost <= 0)
				asset.m_iCargoCapacityCost = 1;
			if (asset.m_iInteractionRadiusMeters <= 0)
				asset.m_iInteractionRadiusMeters = 18;
			if (!asset.m_sCarriedByVehicleId.IsEmpty())
				asset.m_bAttachedToCarrier = true;
		}

		if (m_aMissionAssets.Count() == 0 && m_aMissionRuntimeEntities.Count() > 0)
			MigrateRuntimeEntitiesToMissionAssets();

		foreach (HST_ActiveGroupState group : m_aActiveGroups)
		{
			if (!group)
				continue;

			if (group.m_sRuntimeStatus.IsEmpty())
				group.m_sRuntimeStatus = "restored";
			if (IsZeroVector(group.m_vSourcePosition))
				group.m_vSourcePosition = group.m_vPosition;
			if (IsZeroVector(group.m_vTargetPosition))
				group.m_vTargetPosition = group.m_vPosition;
			if (group.m_iLastSeenAliveCount <= 0)
				group.m_iLastSeenAliveCount = group.m_iInfantryCount + group.m_iVehicleCount;
			if (group.m_iSurvivorInfantryCount <= 0)
				group.m_iSurvivorInfantryCount = group.m_iInfantryCount;
			if (group.m_iSurvivorVehicleCount <= 0)
				group.m_iSurvivorVehicleCount = group.m_iVehicleCount;
			if (group.m_iOriginalInfantryCount <= 0)
				group.m_iOriginalInfantryCount = group.m_iInfantryCount;
			if (group.m_iOriginalVehicleCount <= 0)
				group.m_iOriginalVehicleCount = group.m_iVehicleCount;
			if (group.m_iAssignedWaypointCount <= 0 && group.m_sSpawnFallbackMode == "convoy_waypoints")
				group.m_iAssignedWaypointCount = ResolveGeneratedRouteWaypointCount(FindGeneratedRouteForMigration(group.m_sRouteId));
		}

		foreach (HST_VehicleCargoItemState cargoItem : m_aVehicleCargoItems)
		{
			if (!cargoItem)
				continue;

			if (cargoItem.m_sVehicleRuntimeId.IsEmpty())
				cargoItem.m_sVehicleRuntimeId = cargoItem.m_sVehiclePrefab;
			if (cargoItem.m_sDisplayName.IsEmpty())
				cargoItem.m_sDisplayName = cargoItem.m_sItemPrefab;
			if (cargoItem.m_sCategory.IsEmpty())
				cargoItem.m_sCategory = "equipment";
		}


		bool backfillVehicleCapabilities = restoredSchemaVersion < 19;
		bool backfillGarageVehicleCover = restoredSchemaVersion < 32;
		foreach (HST_GarageVehicleState garageVehicle : m_aGarageVehicles)
		{
			if (!garageVehicle)
				continue;

			if (backfillVehicleCapabilities)
				HST_VehicleCapabilityPolicy.ApplyToGarageVehicle(garageVehicle);
			else if (garageVehicle.m_sSourceVehicleKind.IsEmpty())
				garageVehicle.m_sSourceVehicleKind = HST_VehicleCapabilityPolicy.ResolveSourceVehicleKindFromState(garageVehicle.m_sPrefab, garageVehicle.m_bAmmoSource, garageVehicle.m_bRepairSource, garageVehicle.m_bFuelSource);
			if (backfillGarageVehicleCover || !garageVehicle.m_bCanProvideUndercover)
				garageVehicle.m_bCanProvideUndercover = HST_VehicleCapabilityPolicy.CanGarageVehicleProvideCivilianUndercover(garageVehicle);
			HST_VehicleCapabilityPolicy.NormalizeGarageVehicleCoverState(garageVehicle);
		}
		foreach (HST_RuntimeVehicleState vehicle : m_aRuntimeVehicles)
		{
			if (!vehicle)
				continue;

			if (vehicle.m_sVehicleRuntimeId.IsEmpty())
				vehicle.m_sVehicleRuntimeId = vehicle.m_sPrefab;
			if (vehicle.m_sDisplayName.IsEmpty())
				vehicle.m_sDisplayName = vehicle.m_sRuntimeKind;
			if (vehicle.m_sRuntimeKind.IsEmpty())
				vehicle.m_sRuntimeKind = "runtime_vehicle";
			if (backfillVehicleCapabilities)
				HST_VehicleCapabilityPolicy.ApplyToRuntimeVehicle(vehicle);
			else if (vehicle.m_sSourceVehicleKind.IsEmpty())
				vehicle.m_sSourceVehicleKind = HST_VehicleCapabilityPolicy.ResolveSourceVehicleKindFromState(vehicle.m_sPrefab, vehicle.m_bAmmoSource, vehicle.m_bRepairSource, vehicle.m_bFuelSource);

			if (restoredSchemaVersion < 31 || !vehicle.m_bCanProvideUndercover)
				vehicle.m_bCanProvideUndercover = HST_VehicleCapabilityPolicy.CanRuntimeVehicleProvideCivilianUndercover(vehicle);
			HST_VehicleCapabilityPolicy.NormalizeRuntimeVehicleCoverState(vehicle);
		}

		foreach (HST_SupportRequestState request : m_aSupportRequests)
		{
			if (!request)
				continue;

			if (request.m_sStrikeKind.IsEmpty())
				request.m_sStrikeKind = StrikeKindFromType(request.m_eType);
			request.m_sStrikeConfigResource = "";
			if (request.m_sRuntimeEntityId.IsEmpty() && !request.m_sStrikeKind.IsEmpty())
				request.m_sRuntimeEntityId = "abstract_strike";
			if (request.m_iPlannedInfantryCount <= 0 && request.m_iCompositionManpower > 0)
				request.m_iPlannedInfantryCount = request.m_iCompositionManpower;
			if (request.m_iHRCost < 0)
				request.m_iHRCost = 0;
			if (request.m_iRefundedHR < 0)
				request.m_iRefundedHR = 0;
			if (request.m_iRefundedHR > request.m_iHRCost && request.m_iHRCost > 0)
				request.m_iRefundedHR = request.m_iHRCost;

			if (request.m_sRuntimeStatus.IsEmpty())
			{
				if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED)
					request.m_sRuntimeStatus = "queued_legacy";
				else if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
					request.m_sRuntimeStatus = "active_legacy";
				else if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED)
					request.m_sRuntimeStatus = "resolved_legacy";
				else if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
					request.m_sRuntimeStatus = "cancelled_legacy";
			}

			if (!request.m_sGroupId.IsEmpty())
			{
				request.m_bPhysicalized = true;
				if (request.m_sPhysicalizationMode.IsEmpty())
					request.m_sPhysicalizationMode = "ground_group_legacy";
			}

			if (!request.m_sRuntimeEntityId.IsEmpty() && request.m_sRuntimeEntityId == "abstract_strike")
			{
				request.m_bPhysicalized = true;
				if (request.m_sPhysicalizationMode.IsEmpty())
					request.m_sPhysicalizationMode = "abstract_strike_legacy";
			}

			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED && request.m_iResolvedAtSecond <= 0)
				request.m_iResolvedAtSecond = request.m_iRequestedAtSecond + Math.Max(0, request.m_iETASeconds);
		}

		foreach (HST_EnemyOrderState order : m_aEnemyOrders)
		{
			if (!order)
				continue;

			if (order.m_sRuntimeStatus.IsEmpty())
			{
				if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED)
					order.m_sRuntimeStatus = "resolved_legacy";
				else if (!order.m_sSupportRequestId.IsEmpty())
					order.m_sRuntimeStatus = "support_linked_legacy";
				else
					order.m_sRuntimeStatus = "active_legacy";
			}

			if (order.m_iResolvedAtSecond <= 0 && order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED)
				order.m_iResolvedAtSecond = order.m_iResolveAtSecond;

			if (!order.m_sSupportRequestId.IsEmpty())
				order.m_bPhysicalized = true;
		}
		NormalizeActiveGroupSourceLinks();

		foreach (HST_CivilianZoneState civilianZone : m_aCivilianZones)
		{
			if (!civilianZone)
				continue;

			if (civilianZone.m_iFIASupport == 0 && civilianZone.m_iReputation > 0)
				civilianZone.m_iFIASupport = Math.Max(0, Math.Min(100, civilianZone.m_iReputation));

			if (civilianZone.m_iOccupierSupport == 0)
				civilianZone.m_iOccupierSupport = Math.Max(0, Math.Min(100, 100 - civilianZone.m_iFIASupport / 2 + civilianZone.m_iPolicePresence * 2 + civilianZone.m_iRoadblockPresence * 3));

			if (civilianZone.m_sLastIncidentReason.IsEmpty())
				civilianZone.m_sLastIncidentReason = "legacy/backfilled";
			if (civilianZone.m_iLastSupportChangeSecond <= 0)
				civilianZone.m_iLastSupportChangeSecond = civilianZone.m_iLastIncidentSecond;
			if (civilianZone.m_sLastSecurityReason.IsEmpty())
				civilianZone.m_sLastSecurityReason = civilianZone.m_sLastIncidentReason;

			if (civilianZone.m_iPopulationRemaining <= 0)
				civilianZone.m_iPopulationRemaining = Math.Max(20, Math.Max(1, civilianZone.m_iCivilianPresence) * 8);
			if (civilianZone.m_sLastInfluenceReason.IsEmpty())
				civilianZone.m_sLastInfluenceReason = civilianZone.m_sLastIncidentReason;
			if (civilianZone.m_sLastInfluenceKind.IsEmpty())
				civilianZone.m_sLastInfluenceKind = "legacy/backfilled";
			if (civilianZone.m_iLastInfluenceEventSecond <= 0)
				civilianZone.m_iLastInfluenceEventSecond = civilianZone.m_iLastSupportChangeSecond;

			int influenceCount;
			int activeInfluenceCount;
			int expiredInfluenceCount;
			string lastInfluenceEventId = civilianZone.m_sLastInfluenceEventId;
			int lastInfluenceSecond = civilianZone.m_iLastInfluenceEventSecond;
			foreach (HST_TownInfluenceEventState influenceEvent : m_aTownInfluenceEvents)
			{
				if (!influenceEvent || influenceEvent.m_sZoneId != civilianZone.m_sZoneId)
					continue;

				influenceCount++;
				if (influenceEvent.m_iExpiresAtSecond > 0)
				{
					if (influenceEvent.m_iExpiresAtSecond > m_iElapsedSeconds)
						activeInfluenceCount++;
					else
						expiredInfluenceCount++;
				}

				if (influenceEvent.m_iCreatedAtSecond >= lastInfluenceSecond)
				{
					lastInfluenceSecond = influenceEvent.m_iCreatedAtSecond;
					lastInfluenceEventId = influenceEvent.m_sEventId;
					civilianZone.m_sLastInfluenceKind = influenceEvent.m_sKind;
					civilianZone.m_sLastInfluenceReason = influenceEvent.m_sReason;
				}
			}

			civilianZone.m_iInfluenceEventCount = influenceCount;
			civilianZone.m_iActiveInfluenceModifierCount = activeInfluenceCount;
			civilianZone.m_iExpiredInfluenceModifierCount = expiredInfluenceCount;
			civilianZone.m_iLastInfluenceEventSecond = lastInfluenceSecond;
			civilianZone.m_sLastInfluenceEventId = lastInfluenceEventId;
		}

		if ((m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_WON || m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_LOST) && m_bCampaignEndReportGenerated)
			BackfillCampaignEndPopulationMetadata();

		foreach (HST_PlayerUndercoverState undercover : m_aUndercoverPlayers)
		{
			if (!undercover)
				continue;

			if (undercover.m_sLastEligibilitySummary.IsEmpty())
				undercover.m_sLastEligibilitySummary = undercover.m_sLastReason;

			if (undercover.m_sWantedHeatReason.IsEmpty())
				undercover.m_sWantedHeatReason = "legacy/backfilled";

			if (undercover.m_sAppliedMode.IsEmpty())
				undercover.m_sAppliedMode = "legacy/not_applied";
			if (undercover.m_sLastCompromiseReason.IsEmpty())
				undercover.m_sLastCompromiseReason = undercover.m_sLastReason;
			if (undercover.m_sLastDetectionSource.IsEmpty())
				undercover.m_sLastDetectionSource = "legacy/backfilled";
			if (restoredSchemaVersion < 23 && !undercover.m_bEnforcementEnabled)
				undercover.m_bEnforcementEnabled = true;
		}
	}

	protected void NormalizeActiveGroupSourceLinks()
	{
		foreach (HST_ActiveGroupState group : m_aActiveGroups)
		{
			if (!group)
				continue;

			if (group.m_iOriginalInfantryCount <= 0)
				group.m_iOriginalInfantryCount = group.m_iInfantryCount;
			if (group.m_iOriginalVehicleCount <= 0)
				group.m_iOriginalVehicleCount = group.m_iVehicleCount;
		}

		foreach (HST_SupportRequestState request : m_aSupportRequests)
		{
			if (!request || request.m_sGroupId.IsEmpty())
				continue;

			HST_ActiveGroupState group = FindActiveGroupForMigration(request.m_sGroupId);
			if (group)
				group.m_sSupportRequestId = request.m_sRequestId;
		}

		foreach (HST_QRFState qrf : m_aQRFs)
		{
			if (!qrf || qrf.m_sGroupId.IsEmpty())
				continue;

			HST_ActiveGroupState group = FindActiveGroupForMigration(qrf.m_sGroupId);
			if (group)
				group.m_sQRFInstanceId = qrf.m_sInstanceId;
		}

		foreach (HST_ActiveMissionState mission : m_aActiveMissions)
		{
			if (!mission || mission.m_sInstanceId.IsEmpty())
				continue;

			string guardGroupId = "mission_group_" + mission.m_sInstanceId;
			string convoyGroupToken = "mission_convoy_" + mission.m_sInstanceId + "_";
			foreach (HST_ActiveGroupState group : m_aActiveGroups)
			{
				if (!group)
					continue;
				if (group.m_sGroupId == guardGroupId || group.m_sGroupId.Contains(convoyGroupToken))
					group.m_sMissionInstanceId = mission.m_sInstanceId;
			}
		}

		foreach (HST_ActiveGroupState group : m_aActiveGroups)
		{
			if (!group || group.m_sZoneId.IsEmpty())
				continue;
			if (!group.m_sSupportRequestId.IsEmpty() || !group.m_sMissionInstanceId.IsEmpty() || !group.m_sQRFInstanceId.IsEmpty())
				continue;
			if (group.m_sGarrisonZoneId.IsEmpty())
				group.m_sGarrisonZoneId = group.m_sZoneId;
		}
	}

	protected HST_ActiveGroupState FindActiveGroupForMigration(string groupId)
	{
		if (groupId.IsEmpty())
			return null;

		foreach (HST_ActiveGroupState group : m_aActiveGroups)
		{
			if (group && group.m_sGroupId == groupId)
				return group;
		}

		return null;
	}

	protected void BackfillCampaignEndPopulationMetadata()
	{
		if (m_sCampaignEndOutcomeMode.IsEmpty())
			m_sCampaignEndOutcomeMode = "legacy_control";

		int initialPopulation;
		int remainingPopulation;
		int killedPopulation;
		int fiaSupportPopulation;
		foreach (HST_CivilianZoneState civilianZone : m_aCivilianZones)
		{
			if (!civilianZone)
				continue;

			int townRemaining = Math.Max(0, civilianZone.m_iPopulationRemaining);
			int townKilled = Math.Max(0, civilianZone.m_iPopulationKilled);
			remainingPopulation += townRemaining;
			killedPopulation += townKilled;
			initialPopulation += townRemaining + townKilled;
			int townSupport = Math.Max(0, Math.Min(100, civilianZone.m_iFIASupport));
			fiaSupportPopulation += Math.Round(townRemaining * townSupport / 100.0);
		}

		if (m_iCampaignEndInitialPopulation <= 0)
			m_iCampaignEndInitialPopulation = initialPopulation;
		if (m_iCampaignEndRemainingPopulation <= 0)
			m_iCampaignEndRemainingPopulation = remainingPopulation;
		if (m_iCampaignEndKilledPopulation <= 0)
			m_iCampaignEndKilledPopulation = killedPopulation;
		if (m_iCampaignEndFIASupportPopulation <= 0)
			m_iCampaignEndFIASupportPopulation = fiaSupportPopulation;
		if (m_iCampaignEndSupportPercent <= 0 && remainingPopulation > 0)
			m_iCampaignEndSupportPercent = Math.Round(fiaSupportPopulation * 100.0 / remainingPopulation);

		int airfieldsTotal;
		int airfieldsControlled;
		foreach (HST_ZoneState zone : m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_AIRFIELD)
				continue;

			airfieldsTotal++;
			if (zone.m_sOwnerFactionKey == "FIA")
				airfieldsControlled++;
		}

		if (m_iCampaignEndAirfieldsTotal <= 0)
			m_iCampaignEndAirfieldsTotal = airfieldsTotal;
		if (m_iCampaignEndAirfieldsControlled <= 0)
			m_iCampaignEndAirfieldsControlled = airfieldsControlled;
	}

	protected bool RuntimeVehicleCanProvideCivilianUndercover(HST_RuntimeVehicleState vehicle)
	{
		return HST_VehicleCapabilityPolicy.CanRuntimeVehicleProvideCivilianUndercover(vehicle);
	}

	protected void MigrateRuntimeEntitiesToMissionAssets()
	{
		foreach (HST_MissionRuntimeEntityState runtimeEntity : m_aMissionRuntimeEntities)
		{
			if (!runtimeEntity || runtimeEntity.m_sRuntimeEntityId.IsEmpty())
				continue;

			HST_MissionAssetState asset = new HST_MissionAssetState();
			asset.m_sAssetId = runtimeEntity.m_sRuntimeEntityId;
			asset.m_sEntityId = runtimeEntity.m_sRuntimeEntityId;
			asset.m_sMissionInstanceId = runtimeEntity.m_sMissionInstanceId;
			asset.m_sKind = runtimeEntity.m_sKind;
			asset.m_sRole = runtimeEntity.m_sKind;
			asset.m_sPrefab = runtimeEntity.m_sPrefab;
			asset.m_vSourcePosition = runtimeEntity.m_vPosition;
			asset.m_vCurrentPosition = runtimeEntity.m_vPosition;
			asset.m_vTargetPosition = runtimeEntity.m_vPosition;
			asset.m_vLastKnownPosition = runtimeEntity.m_vPosition;
			asset.m_bSpawned = runtimeEntity.m_bSpawned;
			asset.m_bDestroyed = runtimeEntity.m_bDestroyed;
			asset.m_bPickedUp = runtimeEntity.m_bRecovered;
			asset.m_bDelivered = runtimeEntity.m_bRecovered;
			asset.m_bAlive = !runtimeEntity.m_bDestroyed;
			asset.m_iCargoCapacityCost = 1;
			asset.m_iInteractionRadiusMeters = 18;
			m_aMissionAssets.Insert(asset);
		}
	}

	protected HST_GeneratedRouteState FindGeneratedRouteForMigration(string routeId)
	{
		if (routeId.IsEmpty())
			return null;

		foreach (HST_GeneratedRouteState route : m_aGeneratedRoutes)
		{
			if (route && route.m_sRouteId == routeId)
				return route;
		}

		return null;
	}

	protected int ResolveGeneratedRouteWaypointCount(HST_GeneratedRouteState route)
	{
		if (!route)
			return 0;
		if (route.m_iWaypointCount > 0)
			return route.m_iWaypointCount;
		if (route.m_aWaypoints)
			return route.m_aWaypoints.Count();

		return 0;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected string StrikeKindFromType(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU)
			return "airstrike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return "heavy_airstrike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return "long_range_strike";

		return "";
	}
}
