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
	int m_iPersistenceRestoreSequence;
	int m_iForceSpawnQueueReconciledRestoreSequence;
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
	int m_iNextAuthoritySequence;
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
	ref array<ref HST_OperationRecordState> m_aOperations = {};
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
	ref array<ref HST_CommandReceiptState> m_aCommandReceipts = {};
	ref array<ref HST_ResourceTransactionState> m_aResourceTransactions = {};
	ref array<ref HST_CampaignEventState> m_aCampaignEvents = {};
	ref array<ref HST_ForceManifestState> m_aForceManifests = {};
	ref array<ref HST_ForceQuoteState> m_aForceQuotes = {};
	ref array<ref HST_ForceSettlementTombstoneState> m_aForceSettlementTombstones = {};
	ref array<ref HST_ForceSpawnResultState> m_aForceSpawnResults = {};
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
		m_iPersistenceRestoreSequence = state.m_iPersistenceRestoreSequence;
		m_iForceSpawnQueueReconciledRestoreSequence = state.m_iForceSpawnQueueReconciledRestoreSequence;
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
		m_iNextAuthoritySequence = state.m_iNextAuthoritySequence;
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

		m_aOperations.Clear();
		foreach (HST_OperationRecordState operation : state.m_aOperations)
			m_aOperations.Insert(CopyOperation(operation));

		m_aMapMarkers.Clear();
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
			m_aMapMarkers.Insert(CopyMapMarker(marker));

		m_aArsenalItems.Clear();
		foreach (HST_ArsenalItemState arsenalItem : state.m_aArsenalItems)
			m_aArsenalItems.Insert(CopyArsenalItem(arsenalItem));

		m_aGarageVehicles.Clear();
		foreach (HST_GarageVehicleState vehicle : state.m_aGarageVehicles)
			m_aGarageVehicles.Insert(CopyGarageVehicle(vehicle));

		array<string> sessionOnlyDetachedVehicleIds = {};
		foreach (HST_RuntimeVehicleState sessionRuntimeVehicle : state.m_aRuntimeVehicles)
		{
			if (!IsSessionOnlyDetachedActiveVehicle(sessionRuntimeVehicle) || sessionRuntimeVehicle.m_sVehicleRuntimeId.IsEmpty())
				continue;
			if (sessionOnlyDetachedVehicleIds.Find(sessionRuntimeVehicle.m_sVehicleRuntimeId) < 0)
				sessionOnlyDetachedVehicleIds.Insert(sessionRuntimeVehicle.m_sVehicleRuntimeId);
		}

		m_aVehicleCargoItems.Clear();
		foreach (HST_VehicleCargoItemState cargoItem : state.m_aVehicleCargoItems)
		{
			if (cargoItem && sessionOnlyDetachedVehicleIds.Find(cargoItem.m_sVehicleRuntimeId) >= 0)
				continue;
			m_aVehicleCargoItems.Insert(CopyVehicleCargoItem(cargoItem));
		}

		m_aRuntimeVehicles.Clear();
		foreach (HST_RuntimeVehicleState runtimeVehicle : state.m_aRuntimeVehicles)
		{
			if (IsSessionOnlyDetachedActiveVehicle(runtimeVehicle))
				continue;
			m_aRuntimeVehicles.Insert(CopyRuntimeVehicle(runtimeVehicle));
		}

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

		m_aCommandReceipts.Clear();
		foreach (HST_CommandReceiptState receipt : state.m_aCommandReceipts)
			m_aCommandReceipts.Insert(CopyCommandReceipt(receipt));

		m_aResourceTransactions.Clear();
		foreach (HST_ResourceTransactionState transaction : state.m_aResourceTransactions)
			m_aResourceTransactions.Insert(CopyResourceTransaction(transaction));

		m_aCampaignEvents.Clear();
		foreach (HST_CampaignEventState eventState : state.m_aCampaignEvents)
			m_aCampaignEvents.Insert(CopyCampaignEvent(eventState));

		m_aForceManifests.Clear();
		foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
			m_aForceManifests.Insert(CopyForceManifest(manifest));

		m_aForceQuotes.Clear();
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
			m_aForceQuotes.Insert(CopyForceQuote(quote));

		m_aForceSettlementTombstones.Clear();
		foreach (HST_ForceSettlementTombstoneState tombstone : state.m_aForceSettlementTombstones)
			m_aForceSettlementTombstones.Insert(CopyForceSettlementTombstone(tombstone));

		m_aForceSpawnResults.Clear();
		foreach (HST_ForceSpawnResultState spawnResult : state.m_aForceSpawnResults)
			m_aForceSpawnResults.Insert(CopyForceSpawnResult(spawnResult));

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
		ApplyTo(state, false);
		return state;
	}

	void ApplyTo(HST_CampaignState state, bool migrate = true)
	{
		if (!state)
			return;

		if (migrate)
			MigrateToCurrentSchema();
		state.m_iLastLoadedSchemaVersion = m_iLastLoadedSchemaVersion;
		state.m_iSchemaVersion = m_iSchemaVersion;
		state.m_sPresetId = m_sPresetId;
		state.m_iCampaignSeed = m_iCampaignSeed;
		state.m_ePhase = m_ePhase;
		state.m_iElapsedSeconds = m_iElapsedSeconds;
		state.m_iLastSaveSecond = m_iLastSaveSecond;
		state.m_iLastRestoreSecond = m_iLastRestoreSecond;
		state.m_iPersistenceRestoreSequence = m_iPersistenceRestoreSequence;
		state.m_iForceSpawnQueueReconciledRestoreSequence = m_iForceSpawnQueueReconciledRestoreSequence;
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
		state.m_iNextAuthoritySequence = m_iNextAuthoritySequence;
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

		state.m_aOperations.Clear();
		foreach (HST_OperationRecordState operation : m_aOperations)
			state.m_aOperations.Insert(CopyOperation(operation));

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

		state.m_aCommandReceipts.Clear();
		foreach (HST_CommandReceiptState receipt : m_aCommandReceipts)
			state.m_aCommandReceipts.Insert(CopyCommandReceipt(receipt));

		state.m_aResourceTransactions.Clear();
		foreach (HST_ResourceTransactionState transaction : m_aResourceTransactions)
			state.m_aResourceTransactions.Insert(CopyResourceTransaction(transaction));

		state.m_aCampaignEvents.Clear();
		foreach (HST_CampaignEventState eventState : m_aCampaignEvents)
			state.m_aCampaignEvents.Insert(CopyCampaignEvent(eventState));

		state.m_aForceManifests.Clear();
		foreach (HST_ForceManifestState manifest : m_aForceManifests)
			state.m_aForceManifests.Insert(CopyForceManifest(manifest));

		state.m_aForceQuotes.Clear();
		foreach (HST_ForceQuoteState quote : m_aForceQuotes)
			state.m_aForceQuotes.Insert(CopyForceQuote(quote));

		state.m_aForceSettlementTombstones.Clear();
		foreach (HST_ForceSettlementTombstoneState tombstone : m_aForceSettlementTombstones)
			state.m_aForceSettlementTombstones.Insert(CopyForceSettlementTombstone(tombstone));

		state.m_aForceSpawnResults.Clear();
		foreach (HST_ForceSpawnResultState spawnResult : m_aForceSpawnResults)
			state.m_aForceSpawnResults.Insert(CopyForceSpawnResult(spawnResult));

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
		target.m_sGarrisonId = source.m_sGarrisonId;
		target.m_sZoneId = source.m_sZoneId;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_iInfantryCount = source.m_iInfantryCount;
		target.m_iVehicleCount = source.m_iVehicleCount;
		foreach (string manifestId : source.m_aAcceptedManifestIds)
			target.m_aAcceptedManifestIds.Insert(manifestId);
		return target;
	}

	protected HST_ActiveGroupState CopyActiveGroup(HST_ActiveGroupState source)
	{
		HST_ActiveGroupState target = new HST_ActiveGroupState();
		target.m_sGroupId = source.m_sGroupId;
		target.m_sOperationId = source.m_sOperationId;
		target.m_sManifestId = source.m_sManifestId;
		target.m_sSpawnResultId = source.m_sSpawnResultId;
		target.m_sForceId = source.m_sForceId;
		target.m_sProjectionId = source.m_sProjectionId;
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
		target.m_iDurableLivingInfantryCount = source.m_iDurableLivingInfantryCount;
		target.m_iLastCasualtySecond = source.m_iLastCasualtySecond;
		target.m_iEliminatedAtSecond = source.m_iEliminatedAtSecond;
		target.m_iLifecycleRevision = source.m_iLifecycleRevision;
		target.m_bEverHadLivingCrew = source.m_bEverHadLivingCrew;
		target.m_bEverPopulated = source.m_bEverPopulated;
		target.m_bSpawnCompleted = source.m_bSpawnCompleted;
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

	protected HST_OperationRecordState CopyOperation(HST_OperationRecordState source)
	{
		if (!source)
			return null;

		HST_OperationRecordState target = new HST_OperationRecordState();
		target.m_sOperationId = source.m_sOperationId;
		target.m_eType = source.m_eType;
		target.m_iContractVersion = source.m_iContractVersion;
		target.m_sOwnerFactionKey = source.m_sOwnerFactionKey;
		target.m_sActorIdentityId = source.m_sActorIdentityId;
		target.m_sIssueRequestId = source.m_sIssueRequestId;
		target.m_sConfirmationRequestId = source.m_sConfirmationRequestId;
		target.m_sSupportRequestId = source.m_sSupportRequestId;
		target.m_sQuoteId = source.m_sQuoteId;
		target.m_sManifestId = source.m_sManifestId;
		target.m_sSpawnResultId = source.m_sSpawnResultId;
		target.m_sForceId = source.m_sForceId;
		target.m_sProjectionId = source.m_sProjectionId;
		target.m_sGroupId = source.m_sGroupId;
		target.m_sOriginZoneId = source.m_sOriginZoneId;
		target.m_vOriginPosition = source.m_vOriginPosition;
		target.m_sAssignmentKind = source.m_sAssignmentKind;
		target.m_sAssignmentZoneId = source.m_sAssignmentZoneId;
		target.m_vAssignmentPosition = source.m_vAssignmentPosition;
		target.m_sTacticalTargetZoneId = source.m_sTacticalTargetZoneId;
		target.m_vTacticalTargetPosition = source.m_vTacticalTargetPosition;
		target.m_vStrategicPosition = source.m_vStrategicPosition;
		target.m_sCurrentRouteId = source.m_sCurrentRouteId;
		target.m_sRecallPolicyId = source.m_sRecallPolicyId;
		target.m_sSettlementPolicyId = source.m_sSettlementPolicyId;
		target.m_eDutyState = source.m_eDutyState;
		target.m_eResumeDutyState = source.m_eResumeDutyState;
		target.m_eEngagementMode = source.m_eEngagementMode;
		target.m_eMaterializationState = source.m_eMaterializationState;
		target.m_ePositionAuthority = source.m_ePositionAuthority;
		target.m_eSettlementState = source.m_eSettlementState;
		target.m_eTerminalResult = source.m_eTerminalResult;
		target.m_sSettlementId = source.m_sSettlementId;
		target.m_sTerminalReason = source.m_sTerminalReason;
		target.m_iDeterministicSeed = source.m_iDeterministicSeed;
		target.m_iCreatedAtSecond = source.m_iCreatedAtSecond;
		target.m_iDutyStateEnteredAtSecond = source.m_iDutyStateEnteredAtSecond;
		target.m_iEngagementStateEnteredAtSecond = source.m_iEngagementStateEnteredAtSecond;
		target.m_iMaterializationStateEnteredAtSecond = source.m_iMaterializationStateEnteredAtSecond;
		target.m_iLastContactAtSecond = source.m_iLastContactAtSecond;
		target.m_iLastProgressAtSecond = source.m_iLastProgressAtSecond;
		target.m_iSettledAtSecond = source.m_iSettledAtSecond;
		target.m_iRevision = source.m_iRevision;
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
		target.m_sOperationId = source.m_sOperationId;
		target.m_sQuoteId = source.m_sQuoteId;
		target.m_sManifestId = source.m_sManifestId;
		target.m_sSpawnResultId = source.m_sSpawnResultId;
		target.m_sCommandRequestId = source.m_sCommandRequestId;
		target.m_sMoneyTransactionId = source.m_sMoneyTransactionId;
		target.m_sHRTransactionId = source.m_sHRTransactionId;
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
		target.m_sSelectedGarageVehicleId = source.m_sSelectedGarageVehicleId;
		target.m_sSelectedGarageVehiclePrefab = source.m_sSelectedGarageVehiclePrefab;
		target.m_sSelectedGarageVehicleDisplayName = source.m_sSelectedGarageVehicleDisplayName;
		target.m_vSourcePosition = source.m_vSourcePosition;
		target.m_vTargetPosition = source.m_vTargetPosition;
		target.m_iRequestedAtSecond = source.m_iRequestedAtSecond;
		target.m_iETASeconds = source.m_iETASeconds;
		target.m_iAttackCost = source.m_iAttackCost;
		target.m_iSupportCost = source.m_iSupportCost;
		target.m_iMoneyCost = source.m_iMoneyCost;
		target.m_iHRCost = source.m_iHRCost;
		target.m_iPlannedInfantryCount = source.m_iPlannedInfantryCount;
		target.m_iOperationContractVersion = source.m_iOperationContractVersion;
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
		target.m_bGarageVehicleConsumed = source.m_bGarageVehicleConsumed;
		target.m_sFailureReason = source.m_sFailureReason;
		return target;
	}

	protected HST_EnemyOrderState CopyEnemyOrder(HST_EnemyOrderState source)
	{
		HST_EnemyOrderState target = new HST_EnemyOrderState();
		target.m_sOrderId = source.m_sOrderId;
		target.m_sOperationId = source.m_sOperationId;
		target.m_sManifestId = source.m_sManifestId;
		target.m_sSpawnResultId = source.m_sSpawnResultId;
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

	protected HST_CommandReceiptState CopyCommandReceipt(HST_CommandReceiptState source)
	{
		HST_CommandReceiptState target = new HST_CommandReceiptState();
		target.m_sRequestId = source.m_sRequestId;
		target.m_sActorIdentityId = source.m_sActorIdentityId;
		target.m_sCommandId = source.m_sCommandId;
		target.m_sArgument = source.m_sArgument;
		target.m_sResult = source.m_sResult;
		target.m_sAggregateId = source.m_sAggregateId;
		target.m_eStatus = source.m_eStatus;
		target.m_iReceivedAtSecond = source.m_iReceivedAtSecond;
		target.m_iCompletedAtSecond = source.m_iCompletedAtSecond;
		return target;
	}

	protected HST_ResourceTransactionState CopyResourceTransaction(HST_ResourceTransactionState source)
	{
		HST_ResourceTransactionState target = new HST_ResourceTransactionState();
		target.m_sTransactionId = source.m_sTransactionId;
		target.m_sCommandRequestId = source.m_sCommandRequestId;
		target.m_sOperationId = source.m_sOperationId;
		target.m_sQuoteId = source.m_sQuoteId;
		target.m_sManifestId = source.m_sManifestId;
		target.m_sActorIdentityId = source.m_sActorIdentityId;
		target.m_sResourceType = source.m_sResourceType;
		target.m_sReason = source.m_sReason;
		target.m_sLastSettlementId = source.m_sLastSettlementId;
		target.m_eStatus = source.m_eStatus;
		target.m_iAmount = source.m_iAmount;
		target.m_iRefundedAmount = source.m_iRefundedAmount;
		target.m_iCreatedAtSecond = source.m_iCreatedAtSecond;
		target.m_iSettledAtSecond = source.m_iSettledAtSecond;
		return target;
	}

	protected HST_CampaignEventState CopyCampaignEvent(HST_CampaignEventState source)
	{
		HST_CampaignEventState target = new HST_CampaignEventState();
		target.m_sEventId = source.m_sEventId;
		target.m_sCategory = source.m_sCategory;
		target.m_sAggregateType = source.m_sAggregateType;
		target.m_sAggregateId = source.m_sAggregateId;
		target.m_sCommandRequestId = source.m_sCommandRequestId;
		target.m_sTransition = source.m_sTransition;
		target.m_sReason = source.m_sReason;
		target.m_iCreatedAtSecond = source.m_iCreatedAtSecond;
		return target;
	}

	protected HST_ForceManifestMemberState CopyForceManifestMember(HST_ForceManifestMemberState source)
	{
		if (!source)
			return null;

		HST_ForceManifestMemberState target = new HST_ForceManifestMemberState();
		target.m_sSlotId = source.m_sSlotId;
		target.m_sCatalogSlotId = source.m_sCatalogSlotId;
		target.m_sGroupElementId = source.m_sGroupElementId;
		target.m_sPrefab = source.m_sPrefab;
		target.m_sRole = source.m_sRole;
		target.m_sAssignedVehicleSlotId = source.m_sAssignedVehicleSlotId;
		target.m_sSeatRole = source.m_sSeatRole;
		target.m_iSeatIndex = source.m_iSeatIndex;
		target.m_iOrdinal = source.m_iOrdinal;
		target.m_iMoneyCost = source.m_iMoneyCost;
		target.m_iHRCost = source.m_iHRCost;
		target.m_iEquipmentCost = source.m_iEquipmentCost;
		target.m_bRequired = source.m_bRequired;
		return target;
	}

	protected HST_ForceManifestGroupState CopyForceManifestGroup(HST_ForceManifestGroupState source)
	{
		if (!source)
			return null;

		HST_ForceManifestGroupState target = new HST_ForceManifestGroupState();
		target.m_sElementId = source.m_sElementId;
		target.m_sCatalogEntryId = source.m_sCatalogEntryId;
		target.m_sPrefab = source.m_sPrefab;
		target.m_sRole = source.m_sRole;
		target.m_iOrdinal = source.m_iOrdinal;
		target.m_iExpectedMemberCount = source.m_iExpectedMemberCount;
		target.m_bRequired = source.m_bRequired;
		return target;
	}

	protected HST_ForceManifestVehicleState CopyForceManifestVehicle(HST_ForceManifestVehicleState source)
	{
		if (!source)
			return null;

		HST_ForceManifestVehicleState target = new HST_ForceManifestVehicleState();
		target.m_sSlotId = source.m_sSlotId;
		target.m_sCatalogEntryId = source.m_sCatalogEntryId;
		target.m_sGroupElementId = source.m_sGroupElementId;
		target.m_sPrefab = source.m_sPrefab;
		target.m_sRole = source.m_sRole;
		target.m_iOrdinal = source.m_iOrdinal;
		target.m_iMoneyCost = source.m_iMoneyCost;
		target.m_iRequiredCrew = source.m_iRequiredCrew;
		target.m_bArmed = source.m_bArmed;
		target.m_bLightArmor = source.m_bLightArmor;
		target.m_bHeavyArmor = source.m_bHeavyArmor;
		target.m_bRequired = source.m_bRequired;
		return target;
	}

	protected HST_ForceManifestAssetState CopyForceManifestAsset(HST_ForceManifestAssetState source)
	{
		if (!source)
			return null;

		HST_ForceManifestAssetState target = new HST_ForceManifestAssetState();
		target.m_sSlotId = source.m_sSlotId;
		target.m_sKind = source.m_sKind;
		target.m_sPrefab = source.m_sPrefab;
		target.m_sRole = source.m_sRole;
		target.m_sAssignedVehicleSlotId = source.m_sAssignedVehicleSlotId;
		target.m_iQuantity = source.m_iQuantity;
		target.m_iOrdinal = source.m_iOrdinal;
		target.m_bRequired = source.m_bRequired;
		return target;
	}

	protected HST_ForceManifestState CopyForceManifest(HST_ForceManifestState source)
	{
		if (!source)
			return null;

		HST_ForceManifestState target = new HST_ForceManifestState();
		target.m_sManifestId = source.m_sManifestId;
		target.m_sManifestHash = source.m_sManifestHash;
		target.m_sOperationId = source.m_sOperationId;
		target.m_sQuoteId = source.m_sQuoteId;
		target.m_sCommandRequestId = source.m_sCommandRequestId;
		target.m_sForceKind = source.m_sForceKind;
		target.m_sFactionRole = source.m_sFactionRole;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_sIntentId = source.m_sIntentId;
		target.m_sSourceZoneId = source.m_sSourceZoneId;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sGroupPrefab = source.m_sGroupPrefab;
		target.m_sCatalogVersion = source.m_sCatalogVersion;
		target.m_sPolicyId = source.m_sPolicyId;
		target.m_iRequestedMemberCount = source.m_iRequestedMemberCount;
		target.m_iAcceptedMemberCount = source.m_iAcceptedMemberCount;
		target.m_iRequestedVehicleCount = source.m_iRequestedVehicleCount;
		target.m_iAcceptedVehicleCount = source.m_iAcceptedVehicleCount;
		target.m_iMoneyCost = source.m_iMoneyCost;
		target.m_iHRCost = source.m_iHRCost;
		target.m_iEquipmentCost = source.m_iEquipmentCost;
		target.m_iAttackResourceCost = source.m_iAttackResourceCost;
		target.m_iSupportResourceCost = source.m_iSupportResourceCost;
		target.m_iDeterministicSeed = source.m_iDeterministicSeed;
		target.m_iCreatedAtSecond = source.m_iCreatedAtSecond;
		target.m_bFrozen = source.m_bFrozen;
		foreach (HST_ForceManifestGroupState group : source.m_aGroups)
		{
			HST_ForceManifestGroupState groupCopy = CopyForceManifestGroup(group);
			if (groupCopy)
				target.m_aGroups.Insert(groupCopy);
		}
		foreach (HST_ForceManifestMemberState member : source.m_aMembers)
		{
			HST_ForceManifestMemberState memberCopy = CopyForceManifestMember(member);
			if (memberCopy)
				target.m_aMembers.Insert(memberCopy);
		}
		foreach (HST_ForceManifestVehicleState vehicle : source.m_aVehicles)
		{
			HST_ForceManifestVehicleState vehicleCopy = CopyForceManifestVehicle(vehicle);
			if (vehicleCopy)
				target.m_aVehicles.Insert(vehicleCopy);
		}
		foreach (HST_ForceManifestAssetState asset : source.m_aAssets)
		{
			HST_ForceManifestAssetState assetCopy = CopyForceManifestAsset(asset);
			if (assetCopy)
				target.m_aAssets.Insert(assetCopy);
		}
		return target;
	}

	protected HST_ForceQuoteState CopyForceQuote(HST_ForceQuoteState source)
	{
		if (!source)
			return null;

		HST_ForceQuoteState target = new HST_ForceQuoteState();
		target.m_sQuoteId = source.m_sQuoteId;
		target.m_sManifestId = source.m_sManifestId;
		target.m_sManifestHash = source.m_sManifestHash;
		target.m_sOperationId = source.m_sOperationId;
		target.m_sCommandRequestId = source.m_sCommandRequestId;
		target.m_sConfirmationRequestId = source.m_sConfirmationRequestId;
		target.m_sActorIdentityId = source.m_sActorIdentityId;
		target.m_sQuoteKind = source.m_sQuoteKind;
		target.m_sSupportRequestId = source.m_sSupportRequestId;
		target.m_sCapabilityId = source.m_sCapabilityId;
		target.m_sAssetProfileId = source.m_sAssetProfileId;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_sSourceZoneId = source.m_sSourceZoneId;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sContextHash = source.m_sContextHash;
		target.m_sCatalogVersion = source.m_sCatalogVersion;
		target.m_sPolicyId = source.m_sPolicyId;
		target.m_sMoneyTransactionId = source.m_sMoneyTransactionId;
		target.m_sHRTransactionId = source.m_sHRTransactionId;
		target.m_sAttackTransactionId = source.m_sAttackTransactionId;
		target.m_sSupportTransactionId = source.m_sSupportTransactionId;
		target.m_sRejectionReason = source.m_sRejectionReason;
		target.m_vSourcePosition = source.m_vSourcePosition;
		target.m_vTargetPosition = source.m_vTargetPosition;
		target.m_eSupportType = source.m_eSupportType;
		target.m_eStatus = source.m_eStatus;
		target.m_iRequestedMemberCount = source.m_iRequestedMemberCount;
		target.m_iAcceptedMemberCount = source.m_iAcceptedMemberCount;
		target.m_iRequestedVehicleCount = source.m_iRequestedVehicleCount;
		target.m_iAcceptedVehicleCount = source.m_iAcceptedVehicleCount;
		target.m_iMoneyCost = source.m_iMoneyCost;
		target.m_iHRCost = source.m_iHRCost;
		target.m_iEquipmentCost = source.m_iEquipmentCost;
		target.m_iAttackResourceCost = source.m_iAttackResourceCost;
		target.m_iSupportResourceCost = source.m_iSupportResourceCost;
		target.m_iCreatedAtSecond = source.m_iCreatedAtSecond;
		target.m_iExpiresAtSecond = source.m_iExpiresAtSecond;
		target.m_iAcceptedAtSecond = source.m_iAcceptedAtSecond;
		target.m_iRevision = source.m_iRevision;
		target.m_iETASeconds = source.m_iETASeconds;
		target.m_iCooldownSeconds = source.m_iCooldownSeconds;
		target.m_iExpectedWarLevel = source.m_iExpectedWarLevel;
		target.m_iExpectedGarrisonSlots = source.m_iExpectedGarrisonSlots;
		target.m_iExpectedAbstractInfantry = source.m_iExpectedAbstractInfantry;
		target.m_iExpectedActiveInfantry = source.m_iExpectedActiveInfantry;
		target.m_bAllOrNothing = source.m_bAllOrNothing;
		return target;
	}

	protected HST_ForceSettlementTransactionTombstoneState CopyForceSettlementTransactionTombstone(HST_ForceSettlementTransactionTombstoneState source)
	{
		if (!source)
			return null;
		HST_ForceSettlementTransactionTombstoneState target = new HST_ForceSettlementTransactionTombstoneState();
		target.m_sTransactionId = source.m_sTransactionId;
		target.m_sResourceType = source.m_sResourceType;
		target.m_sLastSettlementId = source.m_sLastSettlementId;
		target.m_eStatus = source.m_eStatus;
		target.m_iAmount = source.m_iAmount;
		target.m_iRefundedAmount = source.m_iRefundedAmount;
		target.m_iSettledAtSecond = source.m_iSettledAtSecond;
		return target;
	}

	protected HST_ForceSettlementTombstoneState CopyForceSettlementTombstone(HST_ForceSettlementTombstoneState source)
	{
		if (!source)
			return null;
		HST_ForceSettlementTombstoneState target = new HST_ForceSettlementTombstoneState();
		target.m_sQuoteId = source.m_sQuoteId;
		target.m_sManifestId = source.m_sManifestId;
		target.m_sManifestHash = source.m_sManifestHash;
		target.m_sOperationId = source.m_sOperationId;
		target.m_sCommandRequestId = source.m_sCommandRequestId;
		target.m_sConfirmationRequestId = source.m_sConfirmationRequestId;
		target.m_sActorIdentityId = source.m_sActorIdentityId;
		target.m_sQuoteKind = source.m_sQuoteKind;
		target.m_sSupportRequestId = source.m_sSupportRequestId;
		target.m_sCapabilityId = source.m_sCapabilityId;
		target.m_sAssetProfileId = source.m_sAssetProfileId;
		target.m_sFactionKey = source.m_sFactionKey;
		target.m_sSourceZoneId = source.m_sSourceZoneId;
		target.m_sTargetZoneId = source.m_sTargetZoneId;
		target.m_sCatalogVersion = source.m_sCatalogVersion;
		target.m_sPolicyId = source.m_sPolicyId;
		target.m_sMoneyTransactionId = source.m_sMoneyTransactionId;
		target.m_sHRTransactionId = source.m_sHRTransactionId;
		target.m_sAttackTransactionId = source.m_sAttackTransactionId;
		target.m_sSupportTransactionId = source.m_sSupportTransactionId;
		target.m_sSettlementKind = source.m_sSettlementKind;
		target.m_sOperationSettlementId = source.m_sOperationSettlementId;
		target.m_vSourcePosition = source.m_vSourcePosition;
		target.m_vTargetPosition = source.m_vTargetPosition;
		target.m_eSupportType = source.m_eSupportType;
		target.m_iRequestedMemberCount = source.m_iRequestedMemberCount;
		target.m_iAcceptedMemberCount = source.m_iAcceptedMemberCount;
		target.m_iRequestedVehicleCount = source.m_iRequestedVehicleCount;
		target.m_iAcceptedVehicleCount = source.m_iAcceptedVehicleCount;
		target.m_iMoneyCost = source.m_iMoneyCost;
		target.m_iHRCost = source.m_iHRCost;
		target.m_iEquipmentCost = source.m_iEquipmentCost;
		target.m_iAttackResourceCost = source.m_iAttackResourceCost;
		target.m_iSupportResourceCost = source.m_iSupportResourceCost;
		target.m_iCreatedAtSecond = source.m_iCreatedAtSecond;
		target.m_iAcceptedAtSecond = source.m_iAcceptedAtSecond;
		target.m_iArchivedAtSecond = source.m_iArchivedAtSecond;
		target.m_iETASeconds = source.m_iETASeconds;
		target.m_iCooldownSeconds = source.m_iCooldownSeconds;
		target.m_iOperationContractVersion = source.m_iOperationContractVersion;
		target.m_iOperationRevision = source.m_iOperationRevision;
		target.m_eOperationTerminalResult = source.m_eOperationTerminalResult;
		target.m_bAllOrNothing = source.m_bAllOrNothing;
		foreach (HST_ForceSettlementTransactionTombstoneState transaction : source.m_aTransactions)
		{
			HST_ForceSettlementTransactionTombstoneState transactionCopy = CopyForceSettlementTransactionTombstone(transaction);
			if (transactionCopy)
				target.m_aTransactions.Insert(transactionCopy);
		}
		return target;
	}

	protected HST_ForceSpawnSlotResultState CopyForceSpawnSlotResult(HST_ForceSpawnSlotResultState source)
	{
		if (!source)
			return null;

		HST_ForceSpawnSlotResultState target = new HST_ForceSpawnSlotResultState();
		target.m_sSlotId = source.m_sSlotId;
		target.m_sSlotKind = source.m_sSlotKind;
		target.m_sSpawnedPrefab = source.m_sSpawnedPrefab;
		target.m_sEntityId = source.m_sEntityId;
		target.m_sAssignedVehicleEntityId = source.m_sAssignedVehicleEntityId;
		target.m_sNativeGroupId = source.m_sNativeGroupId;
		target.m_sProjectionId = source.m_sProjectionId;
		target.m_sFailureReason = source.m_sFailureReason;
		target.m_eStatus = source.m_eStatus;
		target.m_iAttemptCount = source.m_iAttemptCount;
		target.m_iUpdatedAtSecond = source.m_iUpdatedAtSecond;
		target.m_iLifecycleRevision = source.m_iLifecycleRevision;
		target.m_iCasualtyAtSecond = source.m_iCasualtyAtSecond;
		target.m_sRetirementReason = source.m_sRetirementReason;
		target.m_bFactionVerified = source.m_bFactionVerified;
		target.m_bGroupVerified = source.m_bGroupVerified;
		target.m_bGameMasterVerified = source.m_bGameMasterVerified;
		target.m_bProjectionVerified = source.m_bProjectionVerified;
		target.m_bSeatVerified = source.m_bSeatVerified;
		target.m_bAliveVerified = source.m_bAliveVerified;
		target.m_bEverAlive = source.m_bEverAlive;
		target.m_bCasualtyConfirmed = source.m_bCasualtyConfirmed;
		return target;
	}

	protected HST_ForceSpawnResultState CopyForceSpawnResult(HST_ForceSpawnResultState source)
	{
		if (!source)
			return null;

		HST_ForceSpawnResultState target = new HST_ForceSpawnResultState();
		target.m_sResultId = source.m_sResultId;
		target.m_sRequestId = source.m_sRequestId;
		target.m_sManifestId = source.m_sManifestId;
		target.m_sManifestHash = source.m_sManifestHash;
		target.m_sOperationId = source.m_sOperationId;
		target.m_sForceId = source.m_sForceId;
		target.m_sNativeGroupId = source.m_sNativeGroupId;
		target.m_sProjectionId = source.m_sProjectionId;
		target.m_sTerminalReason = source.m_sTerminalReason;
		target.m_sLastFailureReason = source.m_sLastFailureReason;
		target.m_eStatus = source.m_eStatus;
		target.m_iPriority = source.m_iPriority;
		target.m_iRetryCount = source.m_iRetryCount;
		target.m_iMaxRetries = source.m_iMaxRetries;
		target.m_iAttemptGeneration = source.m_iAttemptGeneration;
		target.m_iDeadlineSecond = source.m_iDeadlineSecond;
		target.m_iCreatedAtSecond = source.m_iCreatedAtSecond;
		target.m_iLastAttemptSecond = source.m_iLastAttemptSecond;
		target.m_iNextAttemptSecond = source.m_iNextAttemptSecond;
		target.m_iUpdatedAtSecond = source.m_iUpdatedAtSecond;
		target.m_iCompletedAtSecond = source.m_iCompletedAtSecond;
		target.m_iExpectedSlotCount = source.m_iExpectedSlotCount;
		target.m_iSuccessfulHandoffCount = source.m_iSuccessfulHandoffCount;
		target.m_iReprojectionCount = source.m_iReprojectionCount;
		target.m_iLifecycleRevision = source.m_iLifecycleRevision;
		target.m_iLastLifecycleSecond = source.m_iLastLifecycleSecond;
		target.m_bCancelRequested = source.m_bCancelRequested;
		foreach (HST_ForceSpawnSlotResultState slotResult : source.m_aSlotResults)
		{
			HST_ForceSpawnSlotResultState slotCopy = CopyForceSpawnSlotResult(slotResult);
			if (slotCopy)
				target.m_aSlotResults.Insert(slotCopy);
		}
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

	protected bool IsSessionOnlyDetachedActiveVehicle(HST_RuntimeVehicleState vehicle)
	{
		return vehicle
			&& vehicle.m_bDetached
			&& vehicle.m_sRuntimeKind == "detached_active_vehicle";
	}

	protected void PruneSessionOnlyDetachedActiveVehicles()
	{
		array<string> removedRuntimeIds = {};
		for (int vehicleIndex = m_aRuntimeVehicles.Count() - 1; vehicleIndex >= 0; vehicleIndex--)
		{
			HST_RuntimeVehicleState vehicle = m_aRuntimeVehicles[vehicleIndex];
			if (!IsSessionOnlyDetachedActiveVehicle(vehicle))
				continue;

			if (!vehicle.m_sVehicleRuntimeId.IsEmpty() && removedRuntimeIds.Find(vehicle.m_sVehicleRuntimeId) < 0)
				removedRuntimeIds.Insert(vehicle.m_sVehicleRuntimeId);
			m_aRuntimeVehicles.Remove(vehicleIndex);
		}

		if (removedRuntimeIds.IsEmpty())
			return;

		for (int cargoIndex = m_aVehicleCargoItems.Count() - 1; cargoIndex >= 0; cargoIndex--)
		{
			HST_VehicleCargoItemState cargoItem = m_aVehicleCargoItems[cargoIndex];
			if (!cargoItem || removedRuntimeIds.Find(cargoItem.m_sVehicleRuntimeId) < 0)
				continue;
			m_aVehicleCargoItems.Remove(cargoIndex);
		}
	}

	void MigrateToCurrentSchema()
	{
		int restoredSchemaVersion = m_iSchemaVersion;
		if (restoredSchemaVersion <= 0)
			restoredSchemaVersion = 1;

		m_iLastLoadedSchemaVersion = restoredSchemaVersion;
		m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		m_iPersistenceRestoreSequence = Math.Max(0, m_iPersistenceRestoreSequence);
		m_iForceSpawnQueueReconciledRestoreSequence = Math.Max(0, m_iForceSpawnQueueReconciledRestoreSequence);
		if (m_iNextAuthoritySequence <= 0)
			m_iNextAuthoritySequence = 1;
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
			if (group.m_sOperationId.IsEmpty())
			{
				if (!group.m_sSupportRequestId.IsEmpty())
					group.m_sOperationId = HST_StableIdService.BuildOperationId("support", group.m_sSupportRequestId);
				else if (!group.m_sMissionInstanceId.IsEmpty())
					group.m_sOperationId = HST_StableIdService.BuildOperationId("mission", group.m_sMissionInstanceId);
				else if (!group.m_sQRFInstanceId.IsEmpty())
					group.m_sOperationId = HST_StableIdService.BuildOperationId("qrf", group.m_sQRFInstanceId);
				else if (!group.m_sGarrisonZoneId.IsEmpty())
					group.m_sOperationId = HST_StableIdService.BuildOperationId("garrison", group.m_sGarrisonZoneId);
				else
					group.m_sOperationId = HST_StableIdService.BuildOperationId("group", group.m_sGroupId);
			}
			if (IsZeroVector(group.m_vSourcePosition))
				group.m_vSourcePosition = group.m_vPosition;
			if (IsZeroVector(group.m_vTargetPosition))
				group.m_vTargetPosition = group.m_vPosition;
			bool terminalGroup = group.m_sRuntimeStatus == "eliminated"
				|| group.m_sRuntimeStatus == "convoy_eliminated"
				|| group.m_sRuntimeStatus == "spawn_failed";
			if (terminalGroup)
			{
				group.m_iLastSeenAliveCount = 0;
				group.m_iSurvivorInfantryCount = 0;
				group.m_iSurvivorVehicleCount = 0;
				group.m_bSpawnedEntity = false;
				group.m_sRuntimeEntityId = "";
			}
			else
			{
				if (group.m_iLastSeenAliveCount <= 0)
					group.m_iLastSeenAliveCount = group.m_iInfantryCount + group.m_iVehicleCount;
				if (group.m_iSurvivorInfantryCount <= 0)
					group.m_iSurvivorInfantryCount = group.m_iInfantryCount;
				if (group.m_iSurvivorVehicleCount <= 0)
					group.m_iSurvivorVehicleCount = group.m_iVehicleCount;
			}
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

		PruneSessionOnlyDetachedActiveVehicles();


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
			if (request.m_sOperationId.IsEmpty())
				request.m_sOperationId = HST_StableIdService.BuildOperationId("support", request.m_sRequestId);
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
				request.m_bPhysicalized = request.m_sSpawnResultId.IsEmpty();
				if (request.m_sPhysicalizationMode.IsEmpty())
				{
					if (request.m_sSpawnResultId.IsEmpty())
						request.m_sPhysicalizationMode = "ground_group_legacy";
					else
						request.m_sPhysicalizationMode = "exact_spawn_restore_pending";
				}
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
			if (order.m_sOperationId.IsEmpty())
				order.m_sOperationId = HST_StableIdService.BuildOperationId("enemy_order", order.m_sOrderId);

			if (order.m_iResolvedAtSecond <= 0 && order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED)
				order.m_iResolvedAtSecond = order.m_iResolveAtSecond;

			if (!order.m_sSupportRequestId.IsEmpty())
				order.m_bPhysicalized = true;
		}
		NormalizeActiveGroupSourceLinks();
		NormalizeForceAuthority(restoredSchemaVersion);
		NormalizeOperationAuthority(restoredSchemaVersion);
		NormalizeRestoredOperationProjectionState();
		while (m_aCommandReceipts.Count() > HST_CampaignCommandService.MAX_RECEIPT_ROWS)
			m_aCommandReceipts.Remove(0);
		while (m_aCampaignEvents.Count() > HST_CampaignEventLogService.MAX_EVENT_ROWS)
			m_aCampaignEvents.Remove(0);

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

	protected void NormalizeForceAuthority(int restoredSchemaVersion)
	{
		bool migratedLegacySpawnQueue;
		int migrationSecond = Math.Max(0, m_iElapsedSeconds);
		string legacySpawnFailure = "schema 44 migration finalized pre-schema-44 nonterminal spawn evidence; legacy queue work cannot resume";

		foreach (HST_GarrisonState garrison : m_aGarrisons)
		{
			if (!garrison)
				continue;
			if (garrison.m_sGarrisonId.IsEmpty())
				garrison.m_sGarrisonId = HST_StableIdService.BuildGarrisonId(garrison.m_sZoneId, garrison.m_sFactionKey);
		}

		for (int manifestIndex = m_aForceManifests.Count() - 1; manifestIndex >= 0; manifestIndex--)
		{
			HST_ForceManifestState manifest = m_aForceManifests[manifestIndex];
			if (!manifest)
			{
				m_aForceManifests.Remove(manifestIndex);
				continue;
			}
			for (int groupIndex = manifest.m_aGroups.Count() - 1; groupIndex >= 0; groupIndex--)
			{
				if (!manifest.m_aGroups[groupIndex])
					manifest.m_aGroups.Remove(groupIndex);
			}
			for (int memberIndex = manifest.m_aMembers.Count() - 1; memberIndex >= 0; memberIndex--)
			{
				if (!manifest.m_aMembers[memberIndex])
					manifest.m_aMembers.Remove(memberIndex);
			}
			for (int vehicleIndex = manifest.m_aVehicles.Count() - 1; vehicleIndex >= 0; vehicleIndex--)
			{
				if (!manifest.m_aVehicles[vehicleIndex])
					manifest.m_aVehicles.Remove(vehicleIndex);
			}
			for (int assetIndex = manifest.m_aAssets.Count() - 1; assetIndex >= 0; assetIndex--)
			{
				if (!manifest.m_aAssets[assetIndex])
					manifest.m_aAssets.Remove(assetIndex);
			}
		}

		for (int quoteIndex = m_aForceQuotes.Count() - 1; quoteIndex >= 0; quoteIndex--)
		{
			HST_ForceQuoteState quote = m_aForceQuotes[quoteIndex];
			if (!quote)
			{
				m_aForceQuotes.Remove(quoteIndex);
				continue;
			}
			quote.m_iRequestedMemberCount = Math.Max(0, quote.m_iRequestedMemberCount);
			quote.m_iAcceptedMemberCount = Math.Max(0, quote.m_iAcceptedMemberCount);
			quote.m_iRequestedVehicleCount = Math.Max(0, quote.m_iRequestedVehicleCount);
			quote.m_iAcceptedVehicleCount = Math.Max(0, quote.m_iAcceptedVehicleCount);
			quote.m_iRevision = Math.Max(1, quote.m_iRevision);
		}

		for (int tombstoneIndex = m_aForceSettlementTombstones.Count() - 1; tombstoneIndex >= 0; tombstoneIndex--)
		{
			HST_ForceSettlementTombstoneState tombstone = m_aForceSettlementTombstones[tombstoneIndex];
			if (!tombstone || tombstone.m_sQuoteId.IsEmpty() || tombstone.m_sManifestId.IsEmpty())
			{
				m_aForceSettlementTombstones.Remove(tombstoneIndex);
				continue;
			}
			tombstone.m_iRequestedMemberCount = Math.Max(0, tombstone.m_iRequestedMemberCount);
			tombstone.m_iAcceptedMemberCount = Math.Max(0, tombstone.m_iAcceptedMemberCount);
			tombstone.m_iRequestedVehicleCount = Math.Max(0, tombstone.m_iRequestedVehicleCount);
			tombstone.m_iAcceptedVehicleCount = Math.Max(0, tombstone.m_iAcceptedVehicleCount);
			for (int transactionIndex = tombstone.m_aTransactions.Count() - 1; transactionIndex >= 0; transactionIndex--)
			{
				HST_ForceSettlementTransactionTombstoneState transaction = tombstone.m_aTransactions[transactionIndex];
				if (!transaction || transaction.m_sTransactionId.IsEmpty())
				{
					tombstone.m_aTransactions.Remove(transactionIndex);
					continue;
				}
				transaction.m_iAmount = Math.Max(0, transaction.m_iAmount);
				transaction.m_iRefundedAmount = Math.Max(0, Math.Min(transaction.m_iAmount, transaction.m_iRefundedAmount));
			}
		}

		for (int spawnIndex = m_aForceSpawnResults.Count() - 1; spawnIndex >= 0; spawnIndex--)
		{
			HST_ForceSpawnResultState spawnResult = m_aForceSpawnResults[spawnIndex];
			if (!spawnResult)
			{
				m_aForceSpawnResults.Remove(spawnIndex);
				continue;
			}
			spawnResult.m_iRetryCount = Math.Max(0, spawnResult.m_iRetryCount);
			spawnResult.m_iMaxRetries = Math.Max(0, spawnResult.m_iMaxRetries);
			spawnResult.m_iAttemptGeneration = Math.Max(0, spawnResult.m_iAttemptGeneration);
			spawnResult.m_iDeadlineSecond = Math.Max(0, spawnResult.m_iDeadlineSecond);
			spawnResult.m_iCreatedAtSecond = Math.Max(0, spawnResult.m_iCreatedAtSecond);
			spawnResult.m_iLastAttemptSecond = Math.Max(0, spawnResult.m_iLastAttemptSecond);
			spawnResult.m_iNextAttemptSecond = Math.Max(0, spawnResult.m_iNextAttemptSecond);
			spawnResult.m_iUpdatedAtSecond = Math.Max(0, spawnResult.m_iUpdatedAtSecond);
			spawnResult.m_iCompletedAtSecond = Math.Max(0, spawnResult.m_iCompletedAtSecond);
			spawnResult.m_iExpectedSlotCount = Math.Max(0, spawnResult.m_iExpectedSlotCount);
			spawnResult.m_iSuccessfulHandoffCount = Math.Max(0, spawnResult.m_iSuccessfulHandoffCount);
			spawnResult.m_iReprojectionCount = Math.Max(0, spawnResult.m_iReprojectionCount);
			spawnResult.m_iLifecycleRevision = Math.Max(0, spawnResult.m_iLifecycleRevision);
			spawnResult.m_iLastLifecycleSecond = Math.Max(0, spawnResult.m_iLastLifecycleSecond);
			for (int slotIndex = spawnResult.m_aSlotResults.Count() - 1; slotIndex >= 0; slotIndex--)
			{
				HST_ForceSpawnSlotResultState slotResult = spawnResult.m_aSlotResults[slotIndex];
				if (!slotResult)
				{
					spawnResult.m_aSlotResults.Remove(slotIndex);
					continue;
				}
				slotResult.m_iAttemptCount = Math.Max(0, slotResult.m_iAttemptCount);
				slotResult.m_iUpdatedAtSecond = Math.Max(0, slotResult.m_iUpdatedAtSecond);
				slotResult.m_iLifecycleRevision = Math.Max(0, slotResult.m_iLifecycleRevision);
				slotResult.m_iCasualtyAtSecond = Math.Max(0, slotResult.m_iCasualtyAtSecond);
			}

			if (restoredSchemaVersion >= 44 || IsForceSpawnBatchTerminal(spawnResult.m_eStatus))
				continue;

			FinalizeForceSpawnBatchFailedClosed(spawnResult, legacySpawnFailure, migrationSecond);
			migratedLegacySpawnQueue = true;
		}

		int duplicateSpawnRows = FinalizeDuplicateForceSpawnQueueIdentity(restoredSchemaVersion, migrationSecond);
		MigrateActiveGroupProjectionIdentity(restoredSchemaVersion, migrationSecond);
		MigrateLegacyPlayerQRFTransactions(restoredSchemaVersion, migrationSecond);
		MigrateForceRuntimeLifecycle(restoredSchemaVersion, migrationSecond);
		MigrateForceSettlementArchive(restoredSchemaVersion, migrationSecond);

		if (migratedLegacySpawnQueue && !HasCampaignEventId("migration_schema44_spawn_queue"))
		{
			HST_CampaignEventState spawnMigrationEvent = new HST_CampaignEventState();
			spawnMigrationEvent.m_sEventId = "migration_schema44_spawn_queue";
			spawnMigrationEvent.m_sCategory = "migration";
			spawnMigrationEvent.m_sAggregateType = "spawn_queue";
			spawnMigrationEvent.m_sAggregateId = "schema44";
			spawnMigrationEvent.m_sTransition = "legacy_nonterminal_finalized";
			spawnMigrationEvent.m_sReason = "all pre-schema-44 nonterminal spawn rows were finalized without inventing resumable queue work";
			spawnMigrationEvent.m_iCreatedAtSecond = migrationSecond;
			m_aCampaignEvents.Insert(spawnMigrationEvent);
		}

		if (duplicateSpawnRows > 0 && !HasCampaignEventId("normalization_schema44_spawn_queue_identity_conflict"))
		{
			HST_CampaignEventState duplicateIdentityEvent = new HST_CampaignEventState();
			duplicateIdentityEvent.m_sEventId = "normalization_schema44_spawn_queue_identity_conflict";
			duplicateIdentityEvent.m_sCategory = "normalization";
			duplicateIdentityEvent.m_sAggregateType = "spawn_queue";
			duplicateIdentityEvent.m_sAggregateId = "schema44";
			duplicateIdentityEvent.m_sTransition = "duplicate_identity_failed_closed";
			duplicateIdentityEvent.m_sReason = string.Format("failed closed %1 nonterminal spawn rows with duplicate result, request, or projection identity", duplicateSpawnRows);
			duplicateIdentityEvent.m_iCreatedAtSecond = migrationSecond;
			m_aCampaignEvents.Insert(duplicateIdentityEvent);
		}

		if (restoredSchemaVersion >= 43 || !HasLegacyUnverifiedForces() || HasCampaignEventId("migration_schema43_force_authority"))
			return;

		HST_CampaignEventState migrationEvent = new HST_CampaignEventState();
		migrationEvent.m_sEventId = "migration_schema43_force_authority";
		migrationEvent.m_sCategory = "migration";
		migrationEvent.m_sAggregateType = "force_authority";
		migrationEvent.m_sAggregateId = "schema43";
		migrationEvent.m_sTransition = "legacy_unverified";
		migrationEvent.m_sReason = "legacy force counts preserved without inventing exact manifests, costs, or refunds";
		migrationEvent.m_iCreatedAtSecond = m_iElapsedSeconds;
		m_aCampaignEvents.Insert(migrationEvent);
	}

	protected void NormalizeOperationAuthority(int restoredSchemaVersion)
	{
		if (restoredSchemaVersion >= 49 || HasCampaignEventId("migration_schema49_operation_record"))
			return;

		int migratedCount;
		int conflictCount;
		foreach (HST_SupportRequestState request : m_aSupportRequests)
		{
			if (!request)
				continue;

			// Schema 48 and earlier never opted a request into OperationRecord authority.
			request.m_iOperationContractVersion = 0;
			if (!IsSchema49OperationMigrationCandidate(request))
				continue;

			HST_ForceQuoteState quote;
			HST_ForceManifestState manifest;
			HST_ForceSpawnResultState batch;
			HST_ActiveGroupState group;
			if (!ResolveSchema49OperationMigrationAuthority(request, quote, manifest, batch, group))
			{
				conflictCount++;
				continue;
			}

			HST_OperationRecordState operation = BuildSchema49MigratedOperation(request, quote, manifest, batch, group);
			if (!operation)
			{
				conflictCount++;
				continue;
			}

			request.m_iOperationContractVersion = HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION;
			m_aOperations.Insert(operation);
			migratedCount++;
		}

		HST_CampaignEventState summaryEvent = new HST_CampaignEventState();
		summaryEvent.m_sEventId = "migration_schema49_operation_record";
		summaryEvent.m_sCategory = "migration";
		summaryEvent.m_sAggregateType = "operation_record";
		summaryEvent.m_sAggregateId = "schema49";
		summaryEvent.m_sTransition = "exact_player_qrf_authority_initialized";
		summaryEvent.m_sReason = string.Format("created %1 operation records for uniquely linked accepted nonterminal exact player QRF requests; preserved all other request, economy, ledger, and status authority", migratedCount);
		summaryEvent.m_iCreatedAtSecond = Math.Max(0, m_iElapsedSeconds);
		m_aCampaignEvents.Insert(summaryEvent);

		if (conflictCount <= 0 || HasCampaignEventId("migration_schema49_operation_record_conflict"))
			return;
		HST_CampaignEventState conflictEvent = new HST_CampaignEventState();
		conflictEvent.m_sEventId = "migration_schema49_operation_record_conflict";
		conflictEvent.m_sCategory = "migration";
		conflictEvent.m_sAggregateType = "operation_record";
		conflictEvent.m_sAggregateId = "schema49";
		conflictEvent.m_sTransition = "ambiguous_exact_qrf_preserved";
		conflictEvent.m_sReason = string.Format("left %1 nonterminal exact player QRF candidates on the legacy contract because their persisted authority was incomplete, conflicting, or non-unique", conflictCount);
		conflictEvent.m_iCreatedAtSecond = Math.Max(0, m_iElapsedSeconds);
		m_aCampaignEvents.Insert(conflictEvent);
	}

	protected void NormalizeRestoredOperationProjectionState()
	{
		int restoreSecond = Math.Max(0, m_iElapsedSeconds);
		foreach (HST_OperationRecordState operation : m_aOperations)
		{
			if (!operation || operation.m_eSettlementState == HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
				continue;
			if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL
				&& operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_DEMATERIALIZING)
				continue;

			// Process-local entities never survive persistence. Preserve duty and
			// assignment, but make strategic state own the position until the exact
			// force adapter completes survivor reprojection.
			HST_ActiveGroupState group = FindActiveGroupForMigration(operation.m_sGroupId);
			if (group && group.m_sOperationId == operation.m_sOperationId
				&& group.m_sProjectionId == operation.m_sProjectionId && !IsZeroVector(group.m_vPosition))
				operation.m_vStrategicPosition = group.m_vPosition;
			operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
			operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
			operation.m_iMaterializationStateEnteredAtSecond = restoreSecond;
			operation.m_iLastProgressAtSecond = restoreSecond;
			operation.m_iRevision++;
		}
	}

	protected bool IsSchema49OperationMigrationCandidate(HST_SupportRequestState request)
	{
		if (!request || !request.m_bPlayerRequested || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_QRF)
			return false;
		if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_QUEUED
			&& request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
			return false;

		// Pre-exact requests have neither identity and deliberately remain contract zero.
		return !request.m_sQuoteId.IsEmpty() || !request.m_sManifestId.IsEmpty();
	}

	protected bool ResolveSchema49OperationMigrationAuthority(
		HST_SupportRequestState request,
		out HST_ForceQuoteState quote,
		out HST_ForceManifestState manifest,
		out HST_ForceSpawnResultState batch,
		out HST_ActiveGroupState group)
	{
		quote = null;
		manifest = null;
		batch = null;
		group = null;
		if (!request || request.m_sRequestId.IsEmpty() || request.m_sOperationId.IsEmpty()
			|| request.m_sQuoteId.IsEmpty() || request.m_sManifestId.IsEmpty()
			|| request.m_sSpawnResultId.IsEmpty() || request.m_sCommandRequestId.IsEmpty())
			return false;
		if (request.m_sOperationId != HST_StableIdService.BuildOperationId("support", request.m_sRequestId)
			|| request.m_sSpawnResultId != "spawn_" + request.m_sRequestId)
			return false;
		if (CountSchema49SupportIdentityMatches(request) != 1
			|| CountSchema49OperationIdentityMatches(request) != 0
			|| CountSchema49TombstoneIdentityMatches(request) != 0)
			return false;

		int quoteCount;
		foreach (HST_ForceQuoteState candidateQuote : m_aForceQuotes)
		{
			if (!Schema49QuoteSharesRequestIdentity(candidateQuote, request))
				continue;
			quote = candidateQuote;
			quoteCount++;
		}
		if (quoteCount != 1 || !Schema49QuoteMatchesRequest(quote, request))
			return false;
		if (!Schema49TransactionsMatchQuote(quote))
			return false;

		int manifestCount;
		foreach (HST_ForceManifestState candidateManifest : m_aForceManifests)
		{
			if (!Schema49ManifestSharesRequestIdentity(candidateManifest, request))
				continue;
			manifest = candidateManifest;
			manifestCount++;
		}
		if (manifestCount != 1 || !Schema49ManifestMatchesRequest(manifest, quote, request))
			return false;

		int batchCount;
		foreach (HST_ForceSpawnResultState candidateBatch : m_aForceSpawnResults)
		{
			if (!Schema49BatchSharesRequestIdentity(candidateBatch, request))
				continue;
			batch = candidateBatch;
			batchCount++;
		}
		if (batchCount > 1)
			return false;
		if (batchCount == 1 && !Schema49BatchMatchesRequest(batch, manifest, request))
			return false;

		int groupCount;
		foreach (HST_ActiveGroupState candidateGroup : m_aActiveGroups)
		{
			if (!Schema49GroupSharesRequestIdentity(candidateGroup, request))
				continue;
			group = candidateGroup;
			groupCount++;
		}
		if (batchCount == 0)
			return groupCount == 0 && request.m_sGroupId.IsEmpty();
		if (groupCount != 1)
			return false;
		return Schema49GroupMatchesRequest(group, batch, manifest, request);
	}

	protected bool Schema49QuoteSharesRequestIdentity(HST_ForceQuoteState quote, HST_SupportRequestState request)
	{
		if (!quote || !request)
			return false;
		return quote.m_sQuoteId == request.m_sQuoteId || quote.m_sOperationId == request.m_sOperationId
			|| quote.m_sSupportRequestId == request.m_sRequestId || quote.m_sManifestId == request.m_sManifestId;
	}

	protected bool Schema49QuoteMatchesRequest(HST_ForceQuoteState quote, HST_SupportRequestState request)
	{
		if (!quote || !request || quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED
			|| quote.m_sQuoteKind != HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF
			|| quote.m_eSupportType != HST_ESupportRequestType.HST_SUPPORT_QRF)
			return false;
		if (quote.m_sQuoteId != request.m_sQuoteId || quote.m_sManifestId != request.m_sManifestId
			|| quote.m_sOperationId != request.m_sOperationId || quote.m_sSupportRequestId != request.m_sRequestId
			|| quote.m_sConfirmationRequestId != request.m_sCommandRequestId)
			return false;
		if (quote.m_sActorIdentityId.IsEmpty() || quote.m_sCommandRequestId.IsEmpty()
			|| quote.m_sConfirmationRequestId.IsEmpty() || quote.m_sFactionKey.IsEmpty()
			|| quote.m_sSourceZoneId.IsEmpty() || quote.m_sTargetZoneId.IsEmpty())
			return false;
		if (quote.m_sMoneyTransactionId.IsEmpty() || quote.m_sHRTransactionId.IsEmpty()
			|| quote.m_sMoneyTransactionId != request.m_sMoneyTransactionId
			|| quote.m_sHRTransactionId != request.m_sHRTransactionId)
			return false;
		if (quote.m_sFactionKey != request.m_sFactionKey || quote.m_sCapabilityId != request.m_sCapabilityId
			|| quote.m_sAssetProfileId != request.m_sAssetProfileId || quote.m_sSourceZoneId != request.m_sSourceZoneId
			|| quote.m_sTargetZoneId != request.m_sTargetZoneId
			|| quote.m_vSourcePosition != request.m_vSourcePosition || quote.m_vTargetPosition != request.m_vTargetPosition)
			return false;
		if (quote.m_iMoneyCost != request.m_iMoneyCost || quote.m_iHRCost != request.m_iHRCost
			|| (quote.m_iMoneyCost <= 0 && quote.m_iHRCost <= 0))
			return false;
		return true;
	}

	protected bool Schema49TransactionsMatchQuote(HST_ForceQuoteState quote)
	{
		if (!quote || quote.m_iMoneyCost <= 0 || quote.m_iHRCost <= 0
			|| quote.m_sMoneyTransactionId.IsEmpty() || quote.m_sHRTransactionId.IsEmpty()
			|| quote.m_sMoneyTransactionId == quote.m_sHRTransactionId)
			return false;

		HST_ResourceTransactionState money;
		HST_ResourceTransactionState hr;
		int moneyCount;
		int hrCount;
		foreach (HST_ResourceTransactionState transaction : m_aResourceTransactions)
		{
			if (!transaction)
				continue;
			if (transaction.m_sTransactionId == quote.m_sMoneyTransactionId)
			{
				money = transaction;
				moneyCount++;
			}
			if (transaction.m_sTransactionId == quote.m_sHRTransactionId)
			{
				hr = transaction;
				hrCount++;
			}
		}
		if (moneyCount != 1 || hrCount != 1)
			return false;

		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		return integrity.TransactionMatchesQuote(money, quote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost)
			&& integrity.TransactionMatchesQuote(hr, quote, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost);
	}

	protected bool Schema49ManifestSharesRequestIdentity(HST_ForceManifestState manifest, HST_SupportRequestState request)
	{
		if (!manifest || !request)
			return false;
		return manifest.m_sManifestId == request.m_sManifestId || manifest.m_sOperationId == request.m_sOperationId
			|| manifest.m_sQuoteId == request.m_sQuoteId;
	}

	protected bool Schema49ManifestMatchesRequest(HST_ForceManifestState manifest, HST_ForceQuoteState quote, HST_SupportRequestState request)
	{
		if (!manifest || !quote || !request || !manifest.m_bFrozen || manifest.m_sManifestHash.IsEmpty())
			return false;
		if (manifest.m_sManifestId != request.m_sManifestId || manifest.m_sManifestId != quote.m_sManifestId
			|| manifest.m_sManifestHash != quote.m_sManifestHash || manifest.m_sOperationId != request.m_sOperationId
			|| manifest.m_sQuoteId != request.m_sQuoteId || manifest.m_sCommandRequestId != quote.m_sCommandRequestId)
			return false;
		if (manifest.m_sForceKind != "player_support" || manifest.m_sFactionKey != quote.m_sFactionKey
			|| manifest.m_sSourceZoneId != quote.m_sSourceZoneId || manifest.m_sTargetZoneId != quote.m_sTargetZoneId
			|| manifest.m_sCatalogVersion != quote.m_sCatalogVersion || manifest.m_sPolicyId != quote.m_sPolicyId)
			return false;
		if (manifest.m_iMoneyCost != quote.m_iMoneyCost || manifest.m_iHRCost != quote.m_iHRCost
			|| manifest.m_iAcceptedMemberCount <= 0 || manifest.m_iAcceptedMemberCount != quote.m_iAcceptedMemberCount
			|| manifest.m_iAcceptedMemberCount != request.m_iPlannedInfantryCount
			|| request.m_iCompositionManpower != manifest.m_iAcceptedMemberCount
			|| request.m_iCompositionCost != manifest.m_iMoneyCost)
			return false;
		return true;
	}

	protected bool Schema49BatchSharesRequestIdentity(HST_ForceSpawnResultState batch, HST_SupportRequestState request)
	{
		if (!batch || !request)
			return false;
		return batch.m_sResultId == request.m_sSpawnResultId || batch.m_sRequestId == request.m_sRequestId
			|| batch.m_sOperationId == request.m_sOperationId || batch.m_sManifestId == request.m_sManifestId;
	}

	protected bool Schema49BatchMatchesRequest(HST_ForceSpawnResultState batch, HST_ForceManifestState manifest, HST_SupportRequestState request)
	{
		if (!batch || !manifest || !request)
			return false;
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
			return false;
		return batch.m_sResultId == request.m_sSpawnResultId && batch.m_sRequestId == request.m_sRequestId
			&& batch.m_sManifestId == manifest.m_sManifestId && batch.m_sManifestHash == manifest.m_sManifestHash
			&& batch.m_sOperationId == request.m_sOperationId
			&& batch.m_sForceId == "force_" + request.m_sOperationId
			&& batch.m_sProjectionId == "projection_" + request.m_sOperationId;
	}

	protected bool Schema49GroupSharesRequestIdentity(HST_ActiveGroupState group, HST_SupportRequestState request)
	{
		if (!group || !request)
			return false;
		return (!request.m_sGroupId.IsEmpty() && group.m_sGroupId == request.m_sGroupId)
			|| group.m_sSupportRequestId == request.m_sRequestId || group.m_sOperationId == request.m_sOperationId
			|| group.m_sManifestId == request.m_sManifestId || group.m_sSpawnResultId == request.m_sSpawnResultId;
	}

	protected bool Schema49GroupMatchesRequest(HST_ActiveGroupState group, HST_ForceSpawnResultState batch, HST_ForceManifestState manifest, HST_SupportRequestState request)
	{
		if (!group || !batch || !manifest || !request || request.m_sGroupId.IsEmpty())
			return false;
		if (group.m_sGroupId != request.m_sGroupId || group.m_sGroupId != batch.m_sProjectionId)
			return false;
		if (group.m_sSupportRequestId != request.m_sRequestId || group.m_sOperationId != request.m_sOperationId)
			return false;
		if (group.m_sManifestId != manifest.m_sManifestId || group.m_sSpawnResultId != batch.m_sResultId)
			return false;
		if (group.m_sForceId != batch.m_sForceId || group.m_sProjectionId != batch.m_sProjectionId)
			return false;
		return group.m_sFactionKey == request.m_sFactionKey && group.m_bQRF;
	}

	protected int CountSchema49SupportIdentityMatches(HST_SupportRequestState request)
	{
		int count;
		if (!request)
			return count;
		foreach (HST_SupportRequestState candidate : m_aSupportRequests)
		{
			if (!candidate)
				continue;
			if (candidate.m_sRequestId == request.m_sRequestId || candidate.m_sOperationId == request.m_sOperationId
				|| (!request.m_sQuoteId.IsEmpty() && candidate.m_sQuoteId == request.m_sQuoteId)
				|| (!request.m_sManifestId.IsEmpty() && candidate.m_sManifestId == request.m_sManifestId))
				count++;
		}
		return count;
	}

	protected int CountSchema49OperationIdentityMatches(HST_SupportRequestState request)
	{
		int count;
		if (!request)
			return count;
		foreach (HST_OperationRecordState operation : m_aOperations)
		{
			if (!operation)
				continue;
			if (operation.m_sOperationId == request.m_sOperationId || operation.m_sSupportRequestId == request.m_sRequestId
				|| operation.m_sQuoteId == request.m_sQuoteId || operation.m_sManifestId == request.m_sManifestId)
				count++;
		}
		return count;
	}

	protected int CountSchema49TombstoneIdentityMatches(HST_SupportRequestState request)
	{
		int count;
		if (!request)
			return count;
		foreach (HST_ForceSettlementTombstoneState tombstone : m_aForceSettlementTombstones)
		{
			if (!tombstone)
				continue;
			if (tombstone.m_sOperationId == request.m_sOperationId || tombstone.m_sSupportRequestId == request.m_sRequestId
				|| tombstone.m_sQuoteId == request.m_sQuoteId || tombstone.m_sManifestId == request.m_sManifestId)
				count++;
		}
		return count;
	}

	protected HST_OperationRecordState BuildSchema49MigratedOperation(
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!request || !quote || !manifest)
			return null;

		int migrationSecond = Math.Max(0, m_iElapsedSeconds);
		int createdSecond = Math.Max(0, quote.m_iAcceptedAtSecond);
		if (createdSecond <= 0)
			createdSecond = Math.Max(0, request.m_iRequestedAtSecond);
		HST_EOperationDutyState resumeDuty = HST_EOperationDutyState.HST_OPERATION_DUTY_STAGING;
		if (batch)
			resumeDuty = HST_EOperationDutyState.HST_OPERATION_DUTY_OUTBOUND;
		if (HasSchema49TypedArrivalEvidence(request, batch, group))
			resumeDuty = HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION;

		HST_OperationRecordState operation = new HST_OperationRecordState();
		operation.m_sOperationId = request.m_sOperationId;
		operation.m_eType = HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_QRF;
		operation.m_iContractVersion = HST_OperationService.EXACT_PLAYER_QRF_CONTRACT_VERSION;
		operation.m_sOwnerFactionKey = quote.m_sFactionKey;
		operation.m_sActorIdentityId = quote.m_sActorIdentityId;
		operation.m_sIssueRequestId = quote.m_sCommandRequestId;
		operation.m_sConfirmationRequestId = quote.m_sConfirmationRequestId;
		operation.m_sSupportRequestId = request.m_sRequestId;
		operation.m_sQuoteId = quote.m_sQuoteId;
		operation.m_sManifestId = manifest.m_sManifestId;
		operation.m_sOriginZoneId = quote.m_sSourceZoneId;
		operation.m_vOriginPosition = quote.m_vSourcePosition;
		operation.m_sAssignmentKind = HST_OperationService.EXACT_PLAYER_QRF_ASSIGNMENT_KIND;
		operation.m_sAssignmentZoneId = quote.m_sTargetZoneId;
		operation.m_vAssignmentPosition = quote.m_vTargetPosition;
		operation.m_sTacticalTargetZoneId = quote.m_sTargetZoneId;
		operation.m_vTacticalTargetPosition = quote.m_vTargetPosition;
		operation.m_vStrategicPosition = quote.m_vSourcePosition;
		operation.m_sRecallPolicyId = HST_OperationService.EXACT_PLAYER_QRF_RECALL_POLICY;
		operation.m_sSettlementPolicyId = HST_OperationService.EXACT_PLAYER_QRF_SETTLEMENT_POLICY;
		operation.m_eDutyState = resumeDuty;
		operation.m_eResumeDutyState = resumeDuty;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		operation.m_eTerminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
		operation.m_iDeterministicSeed = manifest.m_iDeterministicSeed;
		operation.m_iCreatedAtSecond = createdSecond;
		operation.m_iDutyStateEnteredAtSecond = createdSecond;
		operation.m_iEngagementStateEnteredAtSecond = migrationSecond;
		operation.m_iMaterializationStateEnteredAtSecond = createdSecond;
		operation.m_iLastProgressAtSecond = migrationSecond;
		operation.m_iRevision = 1;

		if (batch && group)
		{
			operation.m_sSpawnResultId = batch.m_sResultId;
			operation.m_sForceId = batch.m_sForceId;
			operation.m_sProjectionId = batch.m_sProjectionId;
			operation.m_sGroupId = group.m_sGroupId;
			operation.m_sCurrentRouteId = group.m_sRouteId;
			if (!IsZeroVector(group.m_vPosition))
				operation.m_vStrategicPosition = group.m_vPosition;
			operation.m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING;
			operation.m_iMaterializationStateEnteredAtSecond = migrationSecond;
			operation.m_iDutyStateEnteredAtSecond = Math.Max(createdSecond, batch.m_iCreatedAtSecond);
		}

		if (resumeDuty == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
		{
			operation.m_vStrategicPosition = quote.m_vTargetPosition;
			operation.m_iDutyStateEnteredAtSecond = migrationSecond;
		}
		if (request.m_bRecallRequested)
		{
			operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_RECALL_REQUESTED;
			operation.m_iDutyStateEnteredAtSecond = request.m_iRecallRequestedAtSecond;
			if (operation.m_iDutyStateEnteredAtSecond <= 0)
				operation.m_iDutyStateEnteredAtSecond = migrationSecond;
			operation.m_sTacticalTargetZoneId = quote.m_sSourceZoneId;
			operation.m_vTacticalTargetPosition = request.m_vRecallExitPosition;
			if (IsZeroVector(operation.m_vTacticalTargetPosition))
				operation.m_vTacticalTargetPosition = quote.m_vSourcePosition;
		}
		return operation;
	}

	protected bool HasSchema49TypedArrivalEvidence(HST_SupportRequestState request, HST_ForceSpawnResultState batch, HST_ActiveGroupState group)
	{
		if (!request || !batch || !group)
			return false;
		return batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			&& request.m_bPhysicalized && request.m_bAbstractResolved
			&& request.m_sRuntimeStatus == "physical_arrived"
			&& group.m_sRuntimeStatus == "support_arrived";
	}

	protected void MigrateLegacyPlayerQRFTransactions(int restoredSchemaVersion, int migrationSecond)
	{
		if (restoredSchemaVersion >= 46)
			return;

		int importedCount;
		int ambiguousCount;
		foreach (HST_SupportRequestState request : m_aSupportRequests)
		{
			if (!IsLegacyPaidPlayerQRF(request))
				continue;

			string moneyTransactionId;
			bool moneyImported = MigrateLegacyPlayerQRFTransaction(
				request,
				HST_ResourceLedgerService.RESOURCE_FACTION_MONEY,
				Math.Max(0, request.m_iMoneyCost),
				0,
				moneyTransactionId);
			string hrTransactionId;
			bool hrImported = MigrateLegacyPlayerQRFTransaction(
				request,
				HST_ResourceLedgerService.RESOURCE_HR,
				Math.Max(0, request.m_iHRCost),
				Math.Max(0, request.m_iRefundedHR),
				hrTransactionId);
			if (moneyImported && !moneyTransactionId.IsEmpty())
				request.m_sMoneyTransactionId = moneyTransactionId;
			if (hrImported && !hrTransactionId.IsEmpty())
				request.m_sHRTransactionId = hrTransactionId;

			if (moneyImported && hrImported)
				importedCount++;
			else
				ambiguousCount++;
		}

		RecordLegacyPlayerQRFMigrationEvent(importedCount, ambiguousCount, migrationSecond);
	}

	protected bool IsLegacyPaidPlayerQRF(HST_SupportRequestState request)
	{
		if (!request || !request.m_bPlayerRequested || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_QRF)
			return false;
		if (!request.m_sQuoteId.IsEmpty() || !request.m_sManifestId.IsEmpty())
			return false;
		return request.m_iMoneyCost > 0 || request.m_iHRCost > 0;
	}

	protected bool MigrateLegacyPlayerQRFTransaction(
		HST_SupportRequestState request,
		string resourceType,
		int amount,
		int refundedAmount,
		out string transactionId)
	{
		transactionId = "";
		if (!request)
			return false;
		if (amount <= 0)
			return true;

		string expectedId = HST_StableIdService.BuildTransactionId(request.m_sOperationId, resourceType);
		if (expectedId.IsEmpty())
			return false;
		HST_ResourceTransactionState existing = FindResourceTransactionForMigration(expectedId);
		if (existing)
		{
			if (!LegacyPlayerQRFTransactionMatches(existing, request, resourceType, amount))
				return false;
			transactionId = existing.m_sTransactionId;
			return true;
		}

		HST_ResourceTransactionState transaction = new HST_ResourceTransactionState();
		transaction.m_sTransactionId = expectedId;
		transaction.m_sCommandRequestId = request.m_sCommandRequestId;
		if (transaction.m_sCommandRequestId.IsEmpty())
			transaction.m_sCommandRequestId = "migration_schema46_" + request.m_sRequestId;
		transaction.m_sOperationId = request.m_sOperationId;
		transaction.m_sActorIdentityId = "legacy_player_support_unattributed";
		transaction.m_sResourceType = resourceType;
		transaction.m_sReason = "schema 46 imported a proven historical player QRF charge without changing balances";
		transaction.m_iAmount = amount;
		transaction.m_iRefundedAmount = Math.Min(amount, Math.Max(0, refundedAmount));
		transaction.m_iCreatedAtSecond = Math.Max(0, request.m_iRequestedAtSecond);
		transaction.m_iSettledAtSecond = Math.Max(transaction.m_iCreatedAtSecond, request.m_iResolvedAtSecond);
		transaction.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
		if (transaction.m_iRefundedAmount >= transaction.m_iAmount)
			transaction.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED;
		else if (transaction.m_iRefundedAmount > 0)
			transaction.m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED;
		m_aResourceTransactions.Insert(transaction);
		transactionId = transaction.m_sTransactionId;
		return true;
	}

	protected HST_ResourceTransactionState FindResourceTransactionForMigration(string transactionId)
	{
		if (transactionId.IsEmpty())
			return null;
		foreach (HST_ResourceTransactionState transaction : m_aResourceTransactions)
		{
			if (transaction && transaction.m_sTransactionId == transactionId)
				return transaction;
		}
		return null;
	}

	protected bool LegacyPlayerQRFTransactionMatches(
		HST_ResourceTransactionState transaction,
		HST_SupportRequestState request,
		string resourceType,
		int amount)
	{
		if (!transaction || !request)
			return false;
		if (transaction.m_sOperationId != request.m_sOperationId || transaction.m_sResourceType != resourceType)
			return false;
		if (transaction.m_iAmount != amount || !transaction.m_sQuoteId.IsEmpty() || !transaction.m_sManifestId.IsEmpty())
			return false;
		if (transaction.m_iRefundedAmount < 0 || transaction.m_iRefundedAmount > transaction.m_iAmount)
			return false;
		return transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED
			|| transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED
			|| transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED;
	}

	protected void RecordLegacyPlayerQRFMigrationEvent(int importedCount, int ambiguousCount, int migrationSecond)
	{
		if (importedCount > 0 && !HasCampaignEventId("migration_schema46_player_qrf_ledger_imported"))
		{
			HST_CampaignEventState importedEvent = new HST_CampaignEventState();
			importedEvent.m_sEventId = "migration_schema46_player_qrf_ledger_imported";
			importedEvent.m_sCategory = "migration";
			importedEvent.m_sAggregateType = "player_support";
			importedEvent.m_sAggregateId = "schema46";
			importedEvent.m_sTransition = "legacy_qrf_charges_imported";
			importedEvent.m_sReason = string.Format("imported %1 historical player QRF charges into linked ledger rows without changing balances or inventing manifests", importedCount);
			importedEvent.m_iCreatedAtSecond = migrationSecond;
			m_aCampaignEvents.Insert(importedEvent);
		}

		if (ambiguousCount <= 0 || HasCampaignEventId("migration_schema46_player_qrf_ledger_ambiguous"))
			return;
		HST_CampaignEventState ambiguousEvent = new HST_CampaignEventState();
		ambiguousEvent.m_sEventId = "migration_schema46_player_qrf_ledger_ambiguous";
		ambiguousEvent.m_sCategory = "migration";
		ambiguousEvent.m_sAggregateType = "player_support";
		ambiguousEvent.m_sAggregateId = "schema46";
		ambiguousEvent.m_sTransition = "legacy_qrf_charge_ambiguous";
		ambiguousEvent.m_sReason = string.Format("preserved %1 legacy player QRF records without inventing conflicting ledger rows, manifests, or refunds", ambiguousCount);
		ambiguousEvent.m_iCreatedAtSecond = migrationSecond;
		m_aCampaignEvents.Insert(ambiguousEvent);
	}

	protected void MigrateActiveGroupProjectionIdentity(int restoredSchemaVersion, int migrationSecond)
	{
		if (restoredSchemaVersion >= 45)
			return;

		int derivedCount;
		int unresolvedCount;
		foreach (HST_ActiveGroupState group : m_aActiveGroups)
		{
			if (!group || (!group.m_sForceId.IsEmpty() && !group.m_sProjectionId.IsEmpty()))
				continue;

			HST_ForceSpawnResultState linkedBatch;
			int linkedCount;
			foreach (HST_ForceSpawnResultState spawnResult : m_aForceSpawnResults)
			{
				if (!IsActiveGroupSpawnIdentityCandidate(group, spawnResult))
					continue;
				linkedBatch = spawnResult;
				linkedCount++;
				if (linkedCount > 1)
					break;
			}

			if (linkedCount == 1 && linkedBatch)
			{
				if (group.m_sForceId.IsEmpty())
					group.m_sForceId = linkedBatch.m_sForceId;
				if (group.m_sProjectionId.IsEmpty())
					group.m_sProjectionId = linkedBatch.m_sProjectionId;
				derivedCount++;
				continue;
			}

			if (HasActiveGroupSpawnLinkEvidence(group))
				unresolvedCount++;
		}

		RecordSchema45ActiveGroupIdentityEvidence(derivedCount, unresolvedCount, migrationSecond);
	}

	protected bool IsActiveGroupSpawnIdentityCandidate(HST_ActiveGroupState group, HST_ForceSpawnResultState spawnResult)
	{
		if (!group || !spawnResult || spawnResult.m_sForceId.IsEmpty() || spawnResult.m_sProjectionId.IsEmpty())
			return false;
		if (!group.m_sForceId.IsEmpty() && group.m_sForceId != spawnResult.m_sForceId)
			return false;
		if (!group.m_sProjectionId.IsEmpty() && group.m_sProjectionId != spawnResult.m_sProjectionId)
			return false;

		bool resultMatch = !group.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == spawnResult.m_sResultId;
		if (!group.m_sSpawnResultId.IsEmpty() && !resultMatch)
			return false;
		bool manifestMatch = !group.m_sManifestId.IsEmpty() && group.m_sManifestId == spawnResult.m_sManifestId;
		if (!group.m_sManifestId.IsEmpty() && !manifestMatch)
			return false;
		bool operationMatch = !group.m_sOperationId.IsEmpty() && group.m_sOperationId == spawnResult.m_sOperationId;
		if (!group.m_sOperationId.IsEmpty() && !operationMatch)
			return false;

		bool forceMatch = (!group.m_sForceId.IsEmpty() && group.m_sForceId == spawnResult.m_sForceId)
			|| (!group.m_sGroupId.IsEmpty() && group.m_sGroupId == spawnResult.m_sForceId);
		if (!forceMatch)
			return false;
		if (resultMatch)
			return true;
		return manifestMatch && operationMatch;
	}

	protected bool HasActiveGroupSpawnLinkEvidence(HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		if (!group.m_sSpawnResultId.IsEmpty() || !group.m_sForceId.IsEmpty() || !group.m_sProjectionId.IsEmpty())
			return true;

		foreach (HST_ForceSpawnResultState spawnResult : m_aForceSpawnResults)
		{
			if (!spawnResult)
				continue;
			if (!group.m_sManifestId.IsEmpty() && group.m_sManifestId == spawnResult.m_sManifestId)
				return true;
			if (!group.m_sOperationId.IsEmpty() && group.m_sOperationId == spawnResult.m_sOperationId)
				return true;
			if (!group.m_sGroupId.IsEmpty() && group.m_sGroupId == spawnResult.m_sForceId)
				return true;
		}
		return false;
	}

	protected void RecordSchema45ActiveGroupIdentityEvidence(int derivedCount, int unresolvedCount, int migrationSecond)
	{
		if (derivedCount > 0 && !HasCampaignEventId("migration_schema45_active_group_projection_derived"))
		{
			HST_CampaignEventState derivedEvent = new HST_CampaignEventState();
			derivedEvent.m_sEventId = "migration_schema45_active_group_projection_derived";
			derivedEvent.m_sCategory = "migration";
			derivedEvent.m_sAggregateType = "active_group_projection";
			derivedEvent.m_sAggregateId = "schema45";
			derivedEvent.m_sTransition = "unique_spawn_identity_derived";
			derivedEvent.m_sReason = string.Format("derived force and projection identity for %1 uniquely linked active groups", derivedCount);
			derivedEvent.m_iCreatedAtSecond = migrationSecond;
			m_aCampaignEvents.Insert(derivedEvent);
		}

		if (unresolvedCount <= 0 || HasCampaignEventId("migration_schema45_active_group_projection_unresolved"))
			return;
		HST_CampaignEventState unresolvedEvent = new HST_CampaignEventState();
		unresolvedEvent.m_sEventId = "migration_schema45_active_group_projection_unresolved";
		unresolvedEvent.m_sCategory = "migration";
		unresolvedEvent.m_sAggregateType = "active_group_projection";
		unresolvedEvent.m_sAggregateId = "schema45";
		unresolvedEvent.m_sTransition = "linked_identity_unresolved";
		unresolvedEvent.m_sReason = string.Format("left %1 linked active groups unresolved because no unique spawn identity was provable", unresolvedCount);
		unresolvedEvent.m_iCreatedAtSecond = migrationSecond;
		m_aCampaignEvents.Insert(unresolvedEvent);
	}

	protected void MigrateForceRuntimeLifecycle(int restoredSchemaVersion, int migrationSecond)
	{
		if (restoredSchemaVersion >= 47)
			return;

		int migratedBatchCount;
		int migratedMemberCount;
		foreach (HST_ForceSpawnResultState spawnResult : m_aForceSpawnResults)
		{
			if (!spawnResult)
				continue;
			if (spawnResult.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
				&& spawnResult.m_iSuccessfulHandoffCount <= 0)
			{
				spawnResult.m_iSuccessfulHandoffCount = 1;
				migratedBatchCount++;
			}
			spawnResult.m_iLifecycleRevision = Math.Max(spawnResult.m_iLifecycleRevision, spawnResult.m_iSuccessfulHandoffCount);
			spawnResult.m_iLastLifecycleSecond = Math.Max(spawnResult.m_iLastLifecycleSecond, spawnResult.m_iCompletedAtSecond);

			foreach (HST_ForceSpawnSlotResultState slotResult : spawnResult.m_aSlotResults)
			{
				if (!slotResult)
					continue;
				if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
					&& slotResult.m_bAliveVerified)
				{
					slotResult.m_bEverAlive = true;
					slotResult.m_iLifecycleRevision = Math.Max(1, slotResult.m_iLifecycleRevision);
					if (slotResult.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER)
						migratedMemberCount++;
				}
			}
		}

		int migratedGroupCount;
		foreach (HST_ActiveGroupState group : m_aActiveGroups)
		{
			if (!group || group.m_sSpawnResultId.IsEmpty())
				continue;
			HST_ForceSpawnResultState linkedBatch = FindForceSpawnResultForMigration(group.m_sSpawnResultId);
			if (!linkedBatch || linkedBatch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
				continue;

			int livingMembers;
			foreach (HST_ForceSpawnSlotResultState slotResult : linkedBatch.m_aSlotResults)
			{
				if (slotResult && slotResult.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER
					&& slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
					&& slotResult.m_bAliveVerified)
					livingMembers++;
			}
			group.m_bSpawnCompleted = true;
			group.m_bEverPopulated = livingMembers > 0;
			group.m_iDurableLivingInfantryCount = livingMembers;
			group.m_iLifecycleRevision = Math.Max(1, group.m_iLifecycleRevision);
			migratedGroupCount++;
		}

		if (migratedBatchCount <= 0 && migratedMemberCount <= 0 && migratedGroupCount <= 0)
			return;
		if (HasCampaignEventId("migration_schema47_force_runtime_lifecycle"))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = "migration_schema47_force_runtime_lifecycle";
		eventState.m_sCategory = "migration";
		eventState.m_sAggregateType = "force_runtime";
		eventState.m_sAggregateId = "schema47";
		eventState.m_sTransition = "successful_projection_lifecycle_backfilled";
		eventState.m_sReason = string.Format("backfilled lifecycle evidence for %1 successful batches, %2 registered members, and %3 exact active groups without inventing casualties", migratedBatchCount, migratedMemberCount, migratedGroupCount);
		eventState.m_iCreatedAtSecond = migrationSecond;
		m_aCampaignEvents.Insert(eventState);
	}

	protected void MigrateForceSettlementArchive(int restoredSchemaVersion, int migrationSecond)
	{
		if (restoredSchemaVersion >= 48 || HasCampaignEventId("migration_schema48_force_settlement_archive"))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = "migration_schema48_force_settlement_archive";
		eventState.m_sCategory = "migration";
		eventState.m_sAggregateType = "force_settlement_archive";
		eventState.m_sAggregateId = "schema48";
		eventState.m_sTransition = "archive_authority_initialized";
		eventState.m_sReason = "preserved pre-schema-48 accepted quote, manifest, and ledger rows in full without inventing settlement tombstones";
		eventState.m_iCreatedAtSecond = migrationSecond;
		m_aCampaignEvents.Insert(eventState);
	}

	protected HST_ForceSpawnResultState FindForceSpawnResultForMigration(string resultId)
	{
		if (resultId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnResultState spawnResult : m_aForceSpawnResults)
		{
			if (spawnResult && spawnResult.m_sResultId == resultId)
				return spawnResult;
		}
		return null;
	}

	protected bool IsForceSpawnBatchTerminal(HST_EForceSpawnBatchStatus status)
	{
		return status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| status == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
	}

	protected void FinalizeForceSpawnBatchFailedClosed(HST_ForceSpawnResultState spawnResult, string failureReason, int completedAtSecond)
	{
		if (!spawnResult)
			return;

		spawnResult.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL;
		spawnResult.m_sTerminalReason = failureReason;
		spawnResult.m_sLastFailureReason = failureReason;
		spawnResult.m_sNativeGroupId = "";
		spawnResult.m_iCompletedAtSecond = completedAtSecond;
		spawnResult.m_iUpdatedAtSecond = completedAtSecond;
		foreach (HST_ForceSpawnSlotResultState slotResult : spawnResult.m_aSlotResults)
		{
			if (!slotResult)
				continue;
			if (slotResult.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
				&& slotResult.m_bCasualtyConfirmed)
			{
				slotResult.m_sEntityId = "";
				slotResult.m_sAssignedVehicleEntityId = "";
				slotResult.m_sNativeGroupId = "";
				slotResult.m_bAliveVerified = false;
				slotResult.m_iUpdatedAtSecond = completedAtSecond;
				continue;
			}

			if (slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL
				&& slotResult.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_CANCELLED)
			{
				slotResult.m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_FAILED_FINAL;
				slotResult.m_sFailureReason = failureReason;
			}
			slotResult.m_sSpawnedPrefab = "";
			slotResult.m_sEntityId = "";
			slotResult.m_sAssignedVehicleEntityId = "";
			slotResult.m_sNativeGroupId = "";
			slotResult.m_bFactionVerified = false;
			slotResult.m_bGroupVerified = false;
			slotResult.m_bGameMasterVerified = false;
			slotResult.m_bProjectionVerified = false;
			slotResult.m_bSeatVerified = false;
			slotResult.m_bAliveVerified = false;
			slotResult.m_iUpdatedAtSecond = completedAtSecond;
		}
	}

	protected int FinalizeDuplicateForceSpawnQueueIdentity(int restoredSchemaVersion, int normalizationSecond)
	{
		if (restoredSchemaVersion < 44)
			return 0;

		array<int> conflictingIndexes = {};
		for (int firstIndex = 0; firstIndex < m_aForceSpawnResults.Count(); firstIndex++)
		{
			HST_ForceSpawnResultState first = m_aForceSpawnResults[firstIndex];
			if (!first || IsForceSpawnBatchTerminal(first.m_eStatus))
				continue;

			for (int secondIndex = 0; secondIndex < m_aForceSpawnResults.Count(); secondIndex++)
			{
				if (secondIndex == firstIndex)
					continue;

				HST_ForceSpawnResultState second = m_aForceSpawnResults[secondIndex];
				if (!second)
					continue;
				if (!HasDuplicateForceSpawnQueueIdentity(first, second))
					continue;

				if (!conflictingIndexes.Contains(firstIndex))
					conflictingIndexes.Insert(firstIndex);
			}
		}

		string duplicateFailure = "schema 44 normalization failed closed duplicate nonterminal result, request, or projection identity";
		foreach (int conflictingIndex : conflictingIndexes)
			FinalizeForceSpawnBatchFailedClosed(m_aForceSpawnResults[conflictingIndex], duplicateFailure, normalizationSecond);

		return conflictingIndexes.Count();
	}

	protected bool HasDuplicateForceSpawnQueueIdentity(HST_ForceSpawnResultState first, HST_ForceSpawnResultState second)
	{
		if (!first || !second)
			return false;

		return (!first.m_sResultId.IsEmpty() && first.m_sResultId == second.m_sResultId)
			|| (!first.m_sRequestId.IsEmpty() && first.m_sRequestId == second.m_sRequestId)
			|| (!first.m_sProjectionId.IsEmpty() && first.m_sProjectionId == second.m_sProjectionId);
	}

	protected bool HasLegacyUnverifiedForces()
	{
		return m_aGarrisons.Count() > 0 || m_aActiveGroups.Count() > 0 || m_aSupportRequests.Count() > 0 || m_aEnemyOrders.Count() > 0;
	}

	protected bool HasCampaignEventId(string eventId)
	{
		foreach (HST_CampaignEventState eventState : m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
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
			group.m_iDurableLivingInfantryCount = Math.Max(0, group.m_iDurableLivingInfantryCount);
			group.m_iLastCasualtySecond = Math.Max(0, group.m_iLastCasualtySecond);
			group.m_iEliminatedAtSecond = Math.Max(0, group.m_iEliminatedAtSecond);
			group.m_iLifecycleRevision = Math.Max(0, group.m_iLifecycleRevision);
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
