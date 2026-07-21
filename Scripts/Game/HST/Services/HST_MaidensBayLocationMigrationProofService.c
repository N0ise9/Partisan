#ifdef ENABLE_DIAG
class HST_MaidensBayLocationMigrationProofReport
{
	bool m_bBothZoneRetirementExact;
	bool m_bLiveReferenceNormalizationExact;
	bool m_bTypedGraphIsolationExact;
	bool m_bLedgerGeneratedContentExact;
	bool m_bOldOnlyConversionExact;
	bool m_bIdempotencyExact;
	bool m_bLookupCompatibilityExact;
	bool m_bAllExact;
	string m_sEvidence;
}

// Deterministic source/runtime proof for the schema-60 location-taxonomy
// migration. The fixtures are deliberately in-memory so Campaign Debug can
// exercise the same normalizer used by persistence without touching a live save.
class HST_MaidensBayLocationMigrationProofService
{
	static const string LEGACY_ZONE_ID = "town_maidens_bay";
	static const string CANONICAL_ZONE_ID = "resource_logistics_warehouse";
	static const string FACTION_KEY = "US";
	static const string GENERIC_GROUP_ID = "maidens_proof_generic_group";
	static const string AMBIENT_GROUP_ID = "maidens_proof_ambient_group";
	static const string GENERIC_REQUEST_ID = "maidens_proof_generic_request";
	static const string GENERIC_ORDER_ID = "maidens_proof_generic_order";
	static const string GENERIC_MISSION_ID = "maidens_proof_generic_mission";
	static const string GENERIC_OPERATION_ID = "maidens_proof_generic_operation";
	static const string TYPED_OPERATION_ID = "maidens_proof_typed_operation";
	static const string TYPED_GROUP_ID = "maidens_proof_typed_group";
	static const string TYPED_REQUEST_ID = "maidens_proof_typed_request";
	static const string TYPED_ORDER_ID = "maidens_proof_typed_order";
	static const string TYPED_MISSION_ID = "maidens_proof_typed_mission";
	static const string TYPED_ROUTE_ID = "route_town_maidens_bay_frozen";
	static const string TYPED_SITE_ID = "site_town_maidens_bay_frozen";
	static const string TYPED_CANONICAL_ROUTE_ID = "route_resource_logistics_warehouse_frozen";
	static const string TYPED_CANONICAL_SITE_ID = "site_resource_logistics_warehouse_frozen";
	static const string CANONICAL_PATROL_ROUTE_ID = "route_resource_logistics_warehouse_patrol";
	static const string CANONICAL_PRIMARY_ROUTE_ID = "route_resource_logistics_warehouse_primary";
	static const string CANONICAL_PRIMARY_SITE_ID = "site_resource_logistics_warehouse_primary";
	static const string CANONICAL_CRASHSITE_ID = "site_resource_logistics_warehouse_crashsite";

	HST_MaidensBayLocationMigrationProofReport Run()
	{
		HST_MaidensBayLocationMigrationProofReport report = new HST_MaidensBayLocationMigrationProofReport();
		HST_MaidensBayLocationSaveValidationService normalizer = new HST_MaidensBayLocationSaveValidationService();

		HST_CampaignSaveData bothZone = BuildBothZoneFixture();
		string typedBefore = BuildTypedGraphSnapshot(bothZone);
		normalizer.Normalize(bothZone, 59);
		string typedAfterFirst = BuildTypedGraphSnapshot(bothZone);
		string bothAfterFirst = BuildMigrationSnapshot(bothZone);

		report.m_bBothZoneRetirementExact = ProveBothZoneRetirement(bothZone);
		report.m_bLiveReferenceNormalizationExact = ProveLiveReferenceNormalization(bothZone);
		report.m_bTypedGraphIsolationExact = typedBefore == typedAfterFirst;
		report.m_bLedgerGeneratedContentExact = ProveLedgerAndGeneratedContent(bothZone);

		normalizer.Normalize(bothZone, 60);
		string typedAfterSecond = BuildTypedGraphSnapshot(bothZone);
		string bothAfterSecond = BuildMigrationSnapshot(bothZone);
		bool bothZoneIdempotent = bothAfterFirst == bothAfterSecond;
		report.m_bTypedGraphIsolationExact = report.m_bTypedGraphIsolationExact
			&& typedBefore == typedAfterSecond;

		HST_CampaignSaveData oldOnly = BuildOldOnlyFixture();
		normalizer.Normalize(oldOnly, 59);
		string oldOnlyAfterFirst = BuildMigrationSnapshot(oldOnly);
		report.m_bOldOnlyConversionExact = ProveOldOnlyConversion(oldOnly);
		normalizer.Normalize(oldOnly, 60);
		string oldOnlyAfterSecond = BuildMigrationSnapshot(oldOnly);
		bool oldOnlyIdempotent = oldOnlyAfterFirst == oldOnlyAfterSecond
			&& ProveOldOnlyConversion(oldOnly);

		report.m_bIdempotencyExact = bothZoneIdempotent && oldOnlyIdempotent;
		report.m_bLookupCompatibilityExact = ProveLookupCompatibility();
		report.m_bAllExact = report.m_bBothZoneRetirementExact
			&& report.m_bLiveReferenceNormalizationExact
			&& report.m_bTypedGraphIsolationExact
			&& report.m_bLedgerGeneratedContentExact
			&& report.m_bOldOnlyConversionExact
			&& report.m_bIdempotencyExact
			&& report.m_bLookupCompatibilityExact;

		report.m_sEvidence = string.Format(
			"both-zone/refs/frozen %1/%2/%3 | ledger+generated %4 | old-only %5 | idempotent %6 | aliases %7",
			report.m_bBothZoneRetirementExact,
			report.m_bLiveReferenceNormalizationExact,
			report.m_bTypedGraphIsolationExact,
			report.m_bLedgerGeneratedContentExact,
			report.m_bOldOnlyConversionExact,
			report.m_bIdempotencyExact,
			report.m_bLookupCompatibilityExact);
		return report;
	}

	protected HST_CampaignSaveData BuildBothZoneFixture()
	{
		HST_CampaignSaveData saveData = NewSave();
		HST_ZoneState canonical = BuildZone(
			CANONICAL_ZONE_ID,
			"Logistics Warehouse",
			FACTION_KEY,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"5347.874 43.617 10542.923",
			93,
			17);
		canonical.m_iSupport = 37;
		canonical.m_iResistanceCaptureProgress = 23;
		canonical.m_iQrfCooldownUntilSecond = 1337;
		canonical.m_aLinkedZoneIds.Insert(LEGACY_ZONE_ID);
		canonical.m_aLinkedZoneIds.Insert("town_levie");
		saveData.m_aZones.Insert(canonical);

		HST_ZoneState legacy = BuildZone(
			LEGACY_ZONE_ID,
			"Maiden's Bay",
			"USSR",
			HST_EZoneType.HST_ZONE_TOWN,
			"5345.855 43.594 10536.602",
			45,
			10);
		legacy.m_iSupport = 88;
		legacy.m_aLinkedZoneIds.Insert("town_levie");
		legacy.m_aLinkedZoneIds.Insert("town_morton");
		saveData.m_aZones.Insert(legacy);

		HST_GarrisonState canonicalGarrison = BuildGarrison(CANONICAL_ZONE_ID, FACTION_KEY, 4, 1);
		canonicalGarrison.m_aAcceptedManifestIds.Insert("maidens_proof_canonical_manifest");
		saveData.m_aGarrisons.Insert(canonicalGarrison);
		saveData.m_aGarrisons.Insert(BuildGarrison(LEGACY_ZONE_ID, FACTION_KEY, 7, 2));

		HST_ActiveGroupState ambient = new HST_ActiveGroupState();
		ambient.m_sGroupId = AMBIENT_GROUP_ID;
		ambient.m_sZoneId = LEGACY_ZONE_ID;
		ambient.m_sGarrisonZoneId = LEGACY_ZONE_ID;
		ambient.m_sFactionKey = FACTION_KEY;
		ambient.m_sSpawnFallbackMode = "town_police";
		ambient.m_sRuntimeStatus = "active";
		ambient.m_iInfantryCount = 6;
		ambient.m_iSurvivorInfantryCount = 6;
		ambient.m_bSpawnCompleted = true;
		saveData.m_aActiveGroups.Insert(ambient);

		HST_SupportRequestState genericRequest = new HST_SupportRequestState();
		genericRequest.m_sRequestId = GENERIC_REQUEST_ID;
		genericRequest.m_iOperationContractVersion = 0;
		genericRequest.m_sSourceZoneId = LEGACY_ZONE_ID;
		genericRequest.m_sTargetZoneId = LEGACY_ZONE_ID;
		genericRequest.m_sDeploymentRouteId = "route_town_maidens_bay_patrol";
		genericRequest.m_sGroupId = GENERIC_GROUP_ID;
		saveData.m_aSupportRequests.Insert(genericRequest);

		HST_ActiveGroupState genericGroup = new HST_ActiveGroupState();
		genericGroup.m_sGroupId = GENERIC_GROUP_ID;
		genericGroup.m_sSupportRequestId = GENERIC_REQUEST_ID;
		genericGroup.m_sZoneId = LEGACY_ZONE_ID;
		genericGroup.m_sGarrisonZoneId = LEGACY_ZONE_ID;
		genericGroup.m_sRouteId = "route_town_maidens_bay_patrol";
		genericGroup.m_sFactionKey = FACTION_KEY;
		genericGroup.m_sRuntimeStatus = "virtual";
		genericGroup.m_iInfantryCount = 2;
		genericGroup.m_iSurvivorInfantryCount = 2;
		saveData.m_aActiveGroups.Insert(genericGroup);

		HST_QRFState genericQRF = new HST_QRFState();
		genericQRF.m_sInstanceId = "maidens_proof_generic_qrf";
		genericQRF.m_sGroupId = GENERIC_GROUP_ID;
		genericQRF.m_sFactionKey = FACTION_KEY;
		genericQRF.m_sSourceZoneId = LEGACY_ZONE_ID;
		genericQRF.m_sTargetZoneId = LEGACY_ZONE_ID;
		saveData.m_aQRFs.Insert(genericQRF);

		HST_EnemyOrderState genericOrder = new HST_EnemyOrderState();
		genericOrder.m_sOrderId = GENERIC_ORDER_ID;
		genericOrder.m_iOperationContractVersion = 0;
		genericOrder.m_sSourceZoneId = LEGACY_ZONE_ID;
		genericOrder.m_sTargetZoneId = LEGACY_ZONE_ID;
		saveData.m_aEnemyOrders.Insert(genericOrder);

		HST_ActiveMissionState genericMission = new HST_ActiveMissionState();
		genericMission.m_sInstanceId = GENERIC_MISSION_ID;
		genericMission.m_iOperationContractVersion = 0;
		genericMission.m_sTargetZoneId = LEGACY_ZONE_ID;
		genericMission.m_sSiteId = "site_town_maidens_bay_primary";
		saveData.m_aActiveMissions.Insert(genericMission);

		HST_MissionObjectiveState genericObjective = new HST_MissionObjectiveState();
		genericObjective.m_sObjectiveId = "maidens_proof_generic_objective";
		genericObjective.m_sMissionInstanceId = GENERIC_MISSION_ID;
		genericObjective.m_eType = HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA;
		genericObjective.m_sTargetId = LEGACY_ZONE_ID;
		genericObjective.m_sTargetZoneId = LEGACY_ZONE_ID;
		saveData.m_aMissionObjectives.Insert(genericObjective);

		HST_OperationRecordState genericOperation = new HST_OperationRecordState();
		genericOperation.m_sOperationId = GENERIC_OPERATION_ID;
		genericOperation.m_iContractVersion = 0;
		genericOperation.m_sOriginZoneId = LEGACY_ZONE_ID;
		genericOperation.m_sAssignmentZoneId = LEGACY_ZONE_ID;
		genericOperation.m_sTacticalTargetZoneId = LEGACY_ZONE_ID;
		genericOperation.m_sCurrentRouteId = "route_town_maidens_bay_patrol";
		saveData.m_aOperations.Insert(genericOperation);

		HST_GarageVehicleState garageVehicle = new HST_GarageVehicleState();
		garageVehicle.m_sVehicleId = "maidens_proof_garage_vehicle";
		garageVehicle.m_sSourceZoneId = LEGACY_ZONE_ID;
		garageVehicle.m_sLastReporterZoneId = LEGACY_ZONE_ID;
		saveData.m_aGarageVehicles.Insert(garageVehicle);

		HST_RuntimeVehicleState genericVehicle = new HST_RuntimeVehicleState();
		genericVehicle.m_sVehicleRuntimeId = "maidens_proof_generic_vehicle";
		genericVehicle.m_sZoneId = LEGACY_ZONE_ID;
		genericVehicle.m_sLastReporterZoneId = LEGACY_ZONE_ID;
		genericVehicle.m_sRuntimeKind = "mission_transport";
		saveData.m_aRuntimeVehicles.Insert(genericVehicle);
		HST_RuntimeVehicleState ambientVehicle = new HST_RuntimeVehicleState();
		ambientVehicle.m_sVehicleRuntimeId = "maidens_proof_ambient_vehicle";
		ambientVehicle.m_sZoneId = LEGACY_ZONE_ID;
		ambientVehicle.m_sRuntimeKind = "civilian_ambient";
		saveData.m_aRuntimeVehicles.Insert(ambientVehicle);

		HST_PlayerUndercoverState undercover = new HST_PlayerUndercoverState();
		undercover.m_sIdentityId = "maidens_proof_undercover";
		undercover.m_sLastZoneId = LEGACY_ZONE_ID;
		undercover.m_sLastEnforcementZoneId = LEGACY_ZONE_ID;
		saveData.m_aUndercoverPlayers.Insert(undercover);

		HST_CivilianZoneState civilian = new HST_CivilianZoneState();
		civilian.m_sZoneId = LEGACY_ZONE_ID;
		civilian.m_iPopulationRemaining = 48;
		saveData.m_aCivilianZones.Insert(civilian);
		HST_TownInfluenceEventState influence = new HST_TownInfluenceEventState();
		influence.m_sEventId = "maidens_proof_influence";
		influence.m_sZoneId = LEGACY_ZONE_ID;
		saveData.m_aTownInfluenceEvents.Insert(influence);
		HST_TownInfluenceRecord influenceRecord = new HST_TownInfluenceRecord();
		influenceRecord.m_sTownId = LEGACY_ZONE_ID;
		influenceRecord.m_iFIASupportBasisPoints = 7300;
		influenceRecord.m_iOccupierSupportBasisPoints = 2700;
		influenceRecord.m_iInitialPopulation = 48;
		influenceRecord.m_iRemainingPopulation = 48;
		saveData.m_aTownInfluenceRecords.Insert(influenceRecord);

		HST_MapMarkerState zoneMarker = new HST_MapMarkerState();
		zoneMarker.m_sMarkerId = HST_MaidensBayLocationSaveValidationService.LEGACY_ZONE_MARKER_ID;
		zoneMarker.m_sLinkedId = LEGACY_ZONE_ID;
		saveData.m_aMapMarkers.Insert(zoneMarker);
		HST_MapMarkerState ambientMarker = new HST_MapMarkerState();
		ambientMarker.m_sMarkerId = "maidens_proof_ambient_marker";
		ambientMarker.m_sLinkedId = AMBIENT_GROUP_ID;
		saveData.m_aMapMarkers.Insert(ambientMarker);

		BuildLedgerRows(saveData);
		BuildGeneratedRows(saveData);
		BuildTypedFrozenGraph(saveData);
		return saveData;
	}

	protected HST_CampaignSaveData BuildOldOnlyFixture()
	{
		HST_CampaignSaveData saveData = NewSave();
		HST_ZoneState legacy = BuildZone(
			LEGACY_ZONE_ID,
			"Maiden's Bay",
			"FIA",
			HST_EZoneType.HST_ZONE_TOWN,
			"5345.855 43.594 10536.602",
			50,
			10);
		legacy.m_iSupport = 44;
		legacy.m_iResistanceCaptureProgress = 12;
		legacy.m_aLinkedZoneIds.Insert("town_morton");
		saveData.m_aZones.Insert(legacy);
		saveData.m_aGarrisons.Insert(BuildGarrison(LEGACY_ZONE_ID, "FIA", 5, 1));

		HST_ActiveGroupState linkedGroup = new HST_ActiveGroupState();
		linkedGroup.m_sGroupId = "maidens_proof_old_only_group";
		linkedGroup.m_sSupportRequestId = "maidens_proof_old_only_request";
		linkedGroup.m_sZoneId = LEGACY_ZONE_ID;
		linkedGroup.m_sRouteId = "route_town_maidens_bay_patrol";
		saveData.m_aActiveGroups.Insert(linkedGroup);
		HST_SupportRequestState linkedRequest = new HST_SupportRequestState();
		linkedRequest.m_sRequestId = "maidens_proof_old_only_request";
		linkedRequest.m_sSourceZoneId = LEGACY_ZONE_ID;
		linkedRequest.m_sTargetZoneId = LEGACY_ZONE_ID;
		linkedRequest.m_sDeploymentRouteId = "route_town_maidens_bay_patrol";
		saveData.m_aSupportRequests.Insert(linkedRequest);

		HST_GeneratedSiteState site = BuildSite(
			"site_town_maidens_bay_stash",
			LEGACY_ZONE_ID,
			"route_town_maidens_bay_patrol",
			HST_EGeneratedSiteType.HST_SITE_RESOURCE);
		saveData.m_aGeneratedSites.Insert(site);
		saveData.m_aGeneratedRoutes.Insert(BuildRoute(
			"route_town_maidens_bay_patrol",
			LEGACY_ZONE_ID,
			LEGACY_ZONE_ID,
			"old_only"));
		return saveData;
	}

	protected HST_CampaignSaveData NewSave()
	{
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		saveData.m_iSchemaVersion = 59;
		saveData.m_iLastLoadedSchemaVersion = 59;
		saveData.m_iElapsedSeconds = 1000;
		return saveData;
	}

	protected HST_ZoneState BuildZone(
		string zoneId,
		string displayName,
		string ownerFactionKey,
		HST_EZoneType type,
		vector position,
		int incomeValue,
		int garrisonSlots)
	{
		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = zoneId;
		zone.m_sDisplayName = displayName;
		zone.m_sOwnerFactionKey = ownerFactionKey;
		zone.m_eType = type;
		zone.m_vPosition = position;
		zone.m_iIncomeValue = incomeValue;
		zone.m_iGarrisonSlots = garrisonSlots;
		zone.m_iActivationRadiusMeters = 1200;
		return zone;
	}

	protected HST_GarrisonState BuildGarrison(
		string zoneId,
		string factionKey,
		int infantryCount,
		int vehicleCount)
	{
		HST_GarrisonState garrison = new HST_GarrisonState();
		garrison.m_sGarrisonId = HST_StableIdService.BuildGarrisonId(zoneId, factionKey);
		garrison.m_sZoneId = zoneId;
		garrison.m_sFactionKey = factionKey;
		garrison.m_iInfantryCount = infantryCount;
		garrison.m_iVehicleCount = vehicleCount;
		return garrison;
	}

	protected void BuildLedgerRows(HST_CampaignSaveData saveData)
	{
		HST_EnemySupportLedgerState canonical = new HST_EnemySupportLedgerState();
		canonical.m_sFactionKey = FACTION_KEY;
		canonical.m_sZoneId = CANONICAL_ZONE_ID;
		canonical.m_sLastDecisionReason = "canonical defense";
		canonical.m_iRecentDamageScore = 30;
		canonical.m_iLastDamageSecond = 995;
		canonical.m_iAttackSpent = 10;
		canonical.m_iSupportSpent = 3;
		canonical.m_iLastSpendSecond = 990;
		canonical.m_iCooldownUntilSecond = 1100;
		canonical.m_iRefundedAttackResources = 1;
		canonical.m_iRefundedSupportResources = 2;
		saveData.m_aEnemySupportLedgers.Insert(canonical);

		HST_EnemySupportLedgerState legacy = new HST_EnemySupportLedgerState();
		legacy.m_sFactionKey = FACTION_KEY;
		legacy.m_sZoneId = LEGACY_ZONE_ID;
		legacy.m_sLastDecisionReason = "retired town defense";
		legacy.m_iRecentDamageScore = 45;
		legacy.m_iLastDamageSecond = 998;
		legacy.m_iAttackSpent = 7;
		legacy.m_iSupportSpent = 4;
		legacy.m_iLastSpendSecond = 999;
		legacy.m_iCooldownUntilSecond = 1200;
		legacy.m_iRefundedAttackResources = 2;
		legacy.m_iRefundedSupportResources = 1;
		saveData.m_aEnemySupportLedgers.Insert(legacy);
	}

	protected void BuildGeneratedRows(HST_CampaignSaveData saveData)
	{
		saveData.m_aGeneratedSites.Insert(BuildSite(
			CANONICAL_PRIMARY_SITE_ID,
			CANONICAL_ZONE_ID,
			CANONICAL_PRIMARY_ROUTE_ID,
			HST_EGeneratedSiteType.HST_SITE_RESOURCE));
		saveData.m_aGeneratedSites.Insert(BuildSite(
			"site_town_maidens_bay_primary",
			LEGACY_ZONE_ID,
			"route_town_maidens_bay_primary",
			HST_EGeneratedSiteType.HST_SITE_TOWN_CENTER));
		saveData.m_aGeneratedSites.Insert(BuildSite(
			"site_town_maidens_bay_stash",
			LEGACY_ZONE_ID,
			"route_town_maidens_bay_patrol",
			HST_EGeneratedSiteType.HST_SITE_RESOURCE));

		saveData.m_aGeneratedRoutes.Insert(BuildRoute(
			CANONICAL_PRIMARY_ROUTE_ID,
			CANONICAL_ZONE_ID,
			CANONICAL_ZONE_ID,
			"canonical_primary"));
		saveData.m_aGeneratedRoutes.Insert(BuildRoute(
			"route_town_maidens_bay_primary",
			LEGACY_ZONE_ID,
			LEGACY_ZONE_ID,
			"legacy_duplicate"));
		saveData.m_aGeneratedRoutes.Insert(BuildRoute(
			"route_town_maidens_bay_patrol",
			LEGACY_ZONE_ID,
			LEGACY_ZONE_ID,
			"legacy_unique"));
	}

	protected HST_GeneratedSiteState BuildSite(
		string siteId,
		string zoneId,
		string routeId,
		HST_EGeneratedSiteType type)
	{
		HST_GeneratedSiteState site = new HST_GeneratedSiteState();
		site.m_sSiteId = siteId;
		site.m_sZoneId = zoneId;
		site.m_sRouteId = routeId;
		site.m_sOwnerFactionKey = FACTION_KEY;
		site.m_sSourceLayerName = "proof.layer";
		site.m_sSourceCategory = "proof";
		site.m_sSourceLayoutId = "maidens_proof";
		site.m_eType = type;
		site.m_vPosition = "5348 44 10543";
		site.m_iRadiusMeters = 90;
		return site;
	}

	protected HST_GeneratedRouteState BuildRoute(
		string routeId,
		string sourceZoneId,
		string targetZoneId,
		string hint)
	{
		HST_GeneratedRouteState route = new HST_GeneratedRouteState();
		route.m_sRouteId = routeId;
		route.m_sSourceZoneId = sourceZoneId;
		route.m_sTargetZoneId = targetZoneId;
		route.m_sSourceLayerName = "proof.layer";
		route.m_sSourceCategory = hint;
		route.m_sSourceLayoutId = "maidens_proof";
		route.m_vStartPosition = "5340 44 10530";
		route.m_vMidPosition = "5345 44 10536";
		route.m_vEndPosition = "5350 44 10545";
		route.m_iDistanceMeters = 30;
		route.m_iWaypointCount = 1;
		HST_RouteWaypointState waypoint = new HST_RouteWaypointState();
		waypoint.m_sRouteId = routeId;
		waypoint.m_iIndex = 0;
		waypoint.m_vPosition = route.m_vMidPosition;
		waypoint.m_iRadiusMeters = 12;
		waypoint.m_sHint = hint;
		route.m_aWaypoints.Insert(waypoint);
		return route;
	}

	protected void BuildTypedFrozenGraph(HST_CampaignSaveData saveData)
	{
		HST_OperationRecordState operation = new HST_OperationRecordState();
		operation.m_sOperationId = TYPED_OPERATION_ID;
		operation.m_eType = HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_SEARCH_DESTROY;
		operation.m_iContractVersion = -60;
		operation.m_sSupportRequestId = TYPED_REQUEST_ID;
		operation.m_sEnemyOrderId = TYPED_ORDER_ID;
		operation.m_sMissionInstanceId = TYPED_MISSION_ID;
		operation.m_sGroupId = TYPED_GROUP_ID;
		operation.m_sOriginZoneId = LEGACY_ZONE_ID;
		operation.m_sAssignmentZoneId = LEGACY_ZONE_ID;
		operation.m_sTacticalTargetZoneId = LEGACY_ZONE_ID;
		operation.m_sCurrentRouteId = TYPED_ROUTE_ID;
		operation.m_vOriginPosition = "5345 44 10536";
		operation.m_vAssignmentPosition = "5346 44 10537";
		operation.m_vTacticalTargetPosition = "5347 44 10538";
		operation.m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN;
		operation.m_eTerminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE;
		operation.m_iRevision = 9;
		saveData.m_aOperations.Insert(operation);

		HST_SupportRequestState request = new HST_SupportRequestState();
		request.m_sRequestId = TYPED_REQUEST_ID;
		request.m_sOperationId = TYPED_OPERATION_ID;
		request.m_sGroupId = TYPED_GROUP_ID;
		request.m_iOperationContractVersion = -60;
		request.m_sSourceZoneId = LEGACY_ZONE_ID;
		request.m_sTargetZoneId = LEGACY_ZONE_ID;
		request.m_sDeploymentRouteId = TYPED_ROUTE_ID;
		request.m_sRuntimeStatus = "exact_player_support_quarantined";
		request.m_iMoneyCost = 350;
		saveData.m_aSupportRequests.Insert(request);

		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = TYPED_ORDER_ID;
		order.m_sOperationId = TYPED_OPERATION_ID;
		order.m_sGroupId = TYPED_GROUP_ID;
		order.m_iOperationContractVersion = -60;
		order.m_sSourceZoneId = LEGACY_ZONE_ID;
		order.m_sTargetZoneId = LEGACY_ZONE_ID;
		order.m_sRuntimeStatus = "exact_runtime_authority_quarantined";
		saveData.m_aEnemyOrders.Insert(order);

		HST_ActiveMissionState mission = new HST_ActiveMissionState();
		mission.m_sInstanceId = TYPED_MISSION_ID;
		mission.m_sOperationId = TYPED_OPERATION_ID;
		mission.m_iOperationContractVersion = -60;
		mission.m_sTargetZoneId = LEGACY_ZONE_ID;
		mission.m_sSiteId = TYPED_SITE_ID;
		mission.m_sRuntimeFailureReason = "proof quarantine";
		saveData.m_aActiveMissions.Insert(mission);

		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = TYPED_GROUP_ID;
		group.m_sOperationId = TYPED_OPERATION_ID;
		group.m_sSupportRequestId = TYPED_REQUEST_ID;
		group.m_sEnemyOrderId = TYPED_ORDER_ID;
		group.m_sMissionInstanceId = TYPED_MISSION_ID;
		group.m_sQRFInstanceId = "maidens_proof_typed_qrf";
		group.m_sZoneId = LEGACY_ZONE_ID;
		group.m_sGarrisonZoneId = LEGACY_ZONE_ID;
		group.m_sRouteId = TYPED_ROUTE_ID;
		group.m_sRuntimeStatus = "exact_runtime_authority_quarantined";
		group.m_iInfantryCount = 3;
		group.m_iSurvivorInfantryCount = 2;
		saveData.m_aActiveGroups.Insert(group);

		HST_QRFState qrf = new HST_QRFState();
		qrf.m_sInstanceId = "maidens_proof_typed_qrf";
		qrf.m_sGroupId = TYPED_GROUP_ID;
		qrf.m_sFactionKey = FACTION_KEY;
		qrf.m_sSourceZoneId = LEGACY_ZONE_ID;
		qrf.m_sTargetZoneId = LEGACY_ZONE_ID;
		qrf.m_iETASeconds = 77;
		saveData.m_aQRFs.Insert(qrf);

		HST_MissionObjectiveState objective = new HST_MissionObjectiveState();
		objective.m_sObjectiveId = "maidens_proof_typed_objective";
		objective.m_sMissionInstanceId = TYPED_MISSION_ID;
		objective.m_eType = HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA;
		objective.m_sTargetId = LEGACY_ZONE_ID;
		objective.m_sTargetZoneId = LEGACY_ZONE_ID;
		objective.m_iCurrentProgress = 2;
		saveData.m_aMissionObjectives.Insert(objective);

		HST_GeneratedSiteState site = BuildSite(
			TYPED_SITE_ID,
			LEGACY_ZONE_ID,
			TYPED_ROUTE_ID,
			HST_EGeneratedSiteType.HST_SITE_TOWN_CENTER);
		site.m_sSourceCategory = "frozen_typed";
		saveData.m_aGeneratedSites.Insert(site);
		HST_GeneratedRouteState route = BuildRoute(
			TYPED_ROUTE_ID,
			LEGACY_ZONE_ID,
			LEGACY_ZONE_ID,
			"frozen_typed");
		saveData.m_aGeneratedRoutes.Insert(route);
	}

	protected bool ProveBothZoneRetirement(HST_CampaignSaveData saveData)
	{
		HST_ZoneState canonical = FindZone(saveData, CANONICAL_ZONE_ID);
		HST_GarrisonState garrison = FindGarrison(saveData, CANONICAL_ZONE_ID, FACTION_KEY);
		bool canonicalExact = CountZone(saveData, CANONICAL_ZONE_ID) == 1
			&& CountZone(saveData, LEGACY_ZONE_ID) == 0
			&& canonical && canonical.m_sOwnerFactionKey == FACTION_KEY
			&& canonical.m_iSupport == 37
			&& canonical.m_iResistanceCaptureProgress == 23
			&& canonical.m_iIncomeValue == 93
			&& canonical.m_iGarrisonSlots == 17
			&& canonical.m_iQrfCooldownUntilSecond == 1337
			&& canonical.m_aLinkedZoneIds.Contains("town_levie")
			&& canonical.m_aLinkedZoneIds.Contains("town_morton")
			&& !canonical.m_aLinkedZoneIds.Contains(LEGACY_ZONE_ID);
		bool manpowerExact = garrison && garrison.m_iInfantryCount == 4
			&& garrison.m_iVehicleCount == 1
			&& garrison.m_aAcceptedManifestIds.Contains("maidens_proof_canonical_manifest")
			&& CountGarrison(saveData, LEGACY_ZONE_ID, FACTION_KEY) == 0;
		bool projectionRetired = !FindGroup(saveData, AMBIENT_GROUP_ID)
			&& !FindMarker(saveData, "maidens_proof_ambient_marker")
			&& !FindMarker(saveData, HST_MaidensBayLocationSaveValidationService.LEGACY_ZONE_MARKER_ID)
			&& !FindCivilian(saveData, LEGACY_ZONE_ID)
			&& !FindInfluence(saveData, "maidens_proof_influence")
			&& !FindInfluenceRecord(saveData, LEGACY_ZONE_ID)
			&& !FindRuntimeVehicle(saveData, "maidens_proof_ambient_vehicle");
		return canonicalExact && manpowerExact && projectionRetired
			&& CountMigrationEvent(saveData) == 1;
	}

	protected bool ProveLiveReferenceNormalization(HST_CampaignSaveData saveData)
	{
		HST_ActiveGroupState group = FindGroup(saveData, GENERIC_GROUP_ID);
		HST_SupportRequestState request = FindRequest(saveData, GENERIC_REQUEST_ID);
		HST_QRFState qrf = FindQRF(saveData, "maidens_proof_generic_qrf");
		HST_EnemyOrderState order = FindOrder(saveData, GENERIC_ORDER_ID);
		HST_ActiveMissionState mission = FindMission(saveData, GENERIC_MISSION_ID);
		HST_MissionObjectiveState objective = FindObjective(saveData, "maidens_proof_generic_objective");
		HST_OperationRecordState operation = FindOperation(saveData, GENERIC_OPERATION_ID);
		HST_GarageVehicleState garageVehicle = FindGarageVehicle(saveData, "maidens_proof_garage_vehicle");
		HST_RuntimeVehicleState runtimeVehicle = FindRuntimeVehicle(saveData, "maidens_proof_generic_vehicle");
		HST_PlayerUndercoverState undercover = FindUndercover(saveData, "maidens_proof_undercover");
		bool groupExact = group && group.m_sZoneId == CANONICAL_ZONE_ID
			&& group.m_sGarrisonZoneId == CANONICAL_ZONE_ID
			&& group.m_sRouteId == CANONICAL_PATROL_ROUTE_ID;
		bool requestExact = request && request.m_sSourceZoneId == CANONICAL_ZONE_ID
			&& request.m_sTargetZoneId == CANONICAL_ZONE_ID
			&& request.m_sDeploymentRouteId == CANONICAL_PATROL_ROUTE_ID;
		bool qrfExact = qrf && qrf.m_sSourceZoneId == CANONICAL_ZONE_ID
			&& qrf.m_sTargetZoneId == CANONICAL_ZONE_ID;
		bool orderExact = order && order.m_sSourceZoneId == CANONICAL_ZONE_ID
			&& order.m_sTargetZoneId == CANONICAL_ZONE_ID;
		bool missionExact = mission && mission.m_sTargetZoneId == CANONICAL_ZONE_ID
			&& mission.m_sSiteId == CANONICAL_PRIMARY_SITE_ID;
		bool objectiveExact = objective && objective.m_sTargetZoneId == CANONICAL_ZONE_ID
			&& objective.m_sTargetId == CANONICAL_ZONE_ID;
		bool operationExact = operation && operation.m_sOriginZoneId == CANONICAL_ZONE_ID
			&& operation.m_sAssignmentZoneId == CANONICAL_ZONE_ID
			&& operation.m_sTacticalTargetZoneId == CANONICAL_ZONE_ID
			&& operation.m_sCurrentRouteId == CANONICAL_PATROL_ROUTE_ID;
		bool garageExact = garageVehicle && garageVehicle.m_sSourceZoneId == CANONICAL_ZONE_ID
			&& garageVehicle.m_sLastReporterZoneId == CANONICAL_ZONE_ID
			;
		bool runtimeVehicleExact = runtimeVehicle && runtimeVehicle.m_sZoneId == CANONICAL_ZONE_ID
			&& runtimeVehicle.m_sLastReporterZoneId == CANONICAL_ZONE_ID
			;
		bool undercoverExact = undercover && undercover.m_sLastZoneId == CANONICAL_ZONE_ID
			&& undercover.m_sLastEnforcementZoneId == CANONICAL_ZONE_ID;
		bool authorityExact = groupExact && requestExact && qrfExact && orderExact;
		bool missionAuthorityExact = missionExact && objectiveExact && operationExact;
		bool vehicleAuthorityExact = garageExact && runtimeVehicleExact && undercoverExact;
		return authorityExact && missionAuthorityExact && vehicleAuthorityExact;
	}

	protected bool ProveLedgerAndGeneratedContent(HST_CampaignSaveData saveData)
	{
		HST_EnemySupportLedgerState ledger = FindLedger(saveData, FACTION_KEY, CANONICAL_ZONE_ID);
		bool ledgerExact = ledger && CountLedger(saveData, FACTION_KEY, CANONICAL_ZONE_ID) == 1
			&& CountLedger(saveData, FACTION_KEY, LEGACY_ZONE_ID) == 0
			&& ledger.m_iAttackSpent == 17
			&& ledger.m_iSupportSpent == 7
			&& ledger.m_iRefundedAttackResources == 3
			&& ledger.m_iRefundedSupportResources == 3
			&& ledger.m_iRecentDamageScore == 45
			&& ledger.m_iLastDamageSecond == 998
			&& ledger.m_iLastSpendSecond == 999
			&& ledger.m_iCooldownUntilSecond == 1200
			&& ledger.m_sLastDecisionReason == "retired town defense";

		HST_GeneratedSiteState crashsite = FindSite(saveData, CANONICAL_CRASHSITE_ID);
		HST_GeneratedSiteState frozenClone = FindSite(saveData, TYPED_CANONICAL_SITE_ID);
		HST_GeneratedRouteState patrol = FindRoute(saveData, CANONICAL_PATROL_ROUTE_ID);
		HST_GeneratedRouteState frozenRouteClone = FindRoute(saveData, TYPED_CANONICAL_ROUTE_ID);
		bool sitesExact = CountSite(saveData, CANONICAL_PRIMARY_SITE_ID) == 1
			&& CountSite(saveData, "site_town_maidens_bay_primary") == 0
			&& crashsite && crashsite.m_sZoneId == CANONICAL_ZONE_ID
			&& crashsite.m_sRouteId == CANONICAL_PATROL_ROUTE_ID
			&& crashsite.m_eType == HST_EGeneratedSiteType.HST_SITE_CRASHSITE
			&& crashsite.m_sSourceLayerName == "SupplyCaches.layer"
			&& crashsite.m_sSourceCategory == "salvage_anchor";
		bool frozenSitesExact = FindSite(saveData, TYPED_SITE_ID)
			&& frozenClone && frozenClone.m_sZoneId == CANONICAL_ZONE_ID
			&& frozenClone.m_sRouteId == TYPED_CANONICAL_ROUTE_ID;
		bool routesExact = CountRoute(saveData, CANONICAL_PRIMARY_ROUTE_ID) == 1
			&& CountRoute(saveData, "route_town_maidens_bay_primary") == 0
			&& patrol && patrol.m_sSourceZoneId == CANONICAL_ZONE_ID
			&& patrol.m_sTargetZoneId == CANONICAL_ZONE_ID
			&& patrol.m_aWaypoints.Count() == 1
			&& patrol.m_aWaypoints[0].m_sRouteId == CANONICAL_PATROL_ROUTE_ID;
		bool frozenRoutesExact = FindRoute(saveData, TYPED_ROUTE_ID)
			&& frozenRouteClone
			&& frozenRouteClone.m_sSourceZoneId == CANONICAL_ZONE_ID
			&& frozenRouteClone.m_sTargetZoneId == CANONICAL_ZONE_ID
			&& frozenRouteClone.m_aWaypoints.Count() == 1;
		if (frozenRoutesExact)
			frozenRoutesExact = frozenRouteClone.m_aWaypoints[0].m_sRouteId == TYPED_CANONICAL_ROUTE_ID;
		return ledgerExact && sitesExact && frozenSitesExact
			&& routesExact && frozenRoutesExact;
	}

	protected bool ProveOldOnlyConversion(HST_CampaignSaveData saveData)
	{
		HST_ZoneState zone = FindZone(saveData, CANONICAL_ZONE_ID);
		HST_GarrisonState garrison = FindGarrison(saveData, CANONICAL_ZONE_ID, "FIA");
		HST_ActiveGroupState group = FindGroup(saveData, "maidens_proof_old_only_group");
		HST_SupportRequestState request = FindRequest(saveData, "maidens_proof_old_only_request");
		HST_GeneratedSiteState site = FindSite(saveData, CANONICAL_CRASHSITE_ID);
		HST_GeneratedRouteState route = FindRoute(saveData, CANONICAL_PATROL_ROUTE_ID);
		bool zoneIdentityExact = zone && CountZone(saveData, CANONICAL_ZONE_ID) == 1
			&& CountZone(saveData, LEGACY_ZONE_ID) == 0
			&& zone.m_sOwnerFactionKey == "FIA";
		bool zoneEconomyExact = zone && zone.m_iSupport == 44
			&& zone.m_iResistanceCaptureProgress == 12
			&& zone.m_iIncomeValue == 50
			&& zone.m_iGarrisonSlots == 10;
		bool garrisonExact = garrison && garrison.m_iInfantryCount == 5
			&& garrison.m_iVehicleCount == 1
			&& CountGarrison(saveData, LEGACY_ZONE_ID, "FIA") == 0;
		bool groupExact = group && group.m_sZoneId == CANONICAL_ZONE_ID
			&& group.m_sRouteId == CANONICAL_PATROL_ROUTE_ID;
		bool requestExact = request && request.m_sSourceZoneId == CANONICAL_ZONE_ID
			&& request.m_sTargetZoneId == CANONICAL_ZONE_ID
			&& request.m_sDeploymentRouteId == CANONICAL_PATROL_ROUTE_ID;
		bool siteExact = site && site.m_sZoneId == CANONICAL_ZONE_ID
			&& site.m_sRouteId == CANONICAL_PATROL_ROUTE_ID;
		bool routeExact = route && route.m_sSourceZoneId == CANONICAL_ZONE_ID
			&& route.m_sTargetZoneId == CANONICAL_ZONE_ID
			&& route.m_aWaypoints.Count() == 1;
		if (routeExact)
			routeExact = route.m_aWaypoints[0].m_sRouteId == CANONICAL_PATROL_ROUTE_ID;
		bool aggregateExact = zoneIdentityExact && zoneEconomyExact && garrisonExact;
		bool referencesExact = groupExact && requestExact && siteExact && routeExact;
		return aggregateExact && referencesExact && CountMigrationEvent(saveData) == 1;
	}

	protected bool ProveLookupCompatibility()
	{
		HST_CampaignState state = new HST_CampaignState();
		HST_ZoneState canonical = BuildZone(
			CANONICAL_ZONE_ID,
			"Logistics Warehouse",
			FACTION_KEY,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"5347.874 43.617 10542.923",
			93,
			17);
		canonical.m_iSupport = 37;
		state.m_aZones.Insert(canonical);
		HST_GarrisonState garrison = BuildGarrison(CANONICAL_ZONE_ID, FACTION_KEY, 4, 1);
		state.m_aGarrisons.Insert(garrison);
		HST_EnemySupportLedgerState ledger = new HST_EnemySupportLedgerState();
		ledger.m_sFactionKey = FACTION_KEY;
		ledger.m_sZoneId = CANONICAL_ZONE_ID;
		ledger.m_iAttackSpent = 17;
		state.m_aEnemySupportLedgers.Insert(ledger);

		HST_ZoneState liveAlias = state.FindZone(LEGACY_ZONE_ID);
		bool liveAliasExact = liveAlias == canonical;
		if (liveAlias)
			liveAlias.m_iSupport = 64;
		HST_ZoneState historical = state.FindFrozenHistoricalZoneView(LEGACY_ZONE_ID);
		bool detachedExact = historical && historical != canonical
			&& historical.m_sZoneId == LEGACY_ZONE_ID
			&& historical.m_iSupport == 64;
		if (historical)
			historical.m_iSupport = 1;
		detachedExact = detachedExact && canonical.m_iSupport == 64;

		HST_GarrisonState garrisonAlias = state.FindGarrison(LEGACY_ZONE_ID, FACTION_KEY);
		HST_EnemySupportLedgerState ledgerAlias = state.FindEnemySupportLedger(FACTION_KEY, LEGACY_ZONE_ID);
		bool equivalenceExact = HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
			LEGACY_ZONE_ID,
			CANONICAL_ZONE_ID)
			&& HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds("", "")
			&& !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(LEGACY_ZONE_ID, "")
			&& !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
				LEGACY_ZONE_ID,
				"town_morton");

		HST_CampaignState ambiguousState = new HST_CampaignState();
		ambiguousState.m_aZones.Insert(BuildZone(
			CANONICAL_ZONE_ID,
			"Logistics Warehouse A",
			FACTION_KEY,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"5347 44 10542",
			80,
			14));
		ambiguousState.m_aZones.Insert(BuildZone(
			CANONICAL_ZONE_ID,
			"Logistics Warehouse B",
			FACTION_KEY,
			HST_EZoneType.HST_ZONE_RESOURCE,
			"5348 44 10543",
			80,
			14));
		bool ambiguousHistoricalRejected
			= !ambiguousState.FindFrozenHistoricalZoneView(LEGACY_ZONE_ID);
		bool aggregateAliasesExact = garrisonAlias == garrison && ledgerAlias == ledger;
		return liveAliasExact && detachedExact && aggregateAliasesExact
			&& equivalenceExact && ambiguousHistoricalRejected;
	}

	protected string BuildTypedGraphSnapshot(HST_CampaignSaveData saveData)
	{
		HST_OperationRecordState operation = FindOperation(saveData, TYPED_OPERATION_ID);
		HST_SupportRequestState request = FindRequest(saveData, TYPED_REQUEST_ID);
		HST_EnemyOrderState order = FindOrder(saveData, TYPED_ORDER_ID);
		HST_ActiveMissionState mission = FindMission(saveData, TYPED_MISSION_ID);
		HST_ActiveGroupState group = FindGroup(saveData, TYPED_GROUP_ID);
		HST_QRFState qrf = FindQRF(saveData, "maidens_proof_typed_qrf");
		HST_MissionObjectiveState objective = FindObjective(saveData, "maidens_proof_typed_objective");
		HST_GeneratedSiteState site = FindSite(saveData, TYPED_SITE_ID);
		HST_GeneratedRouteState route = FindRoute(saveData, TYPED_ROUTE_ID);
		string snapshot = "operation=" + OperationSnapshot(operation);
		snapshot += "\nrequest=" + RequestSnapshot(request);
		snapshot += "\norder=" + OrderSnapshot(order);
		snapshot += "\nmission=" + MissionSnapshot(mission);
		snapshot += "\ngroup=" + GroupSnapshot(group);
		snapshot += "\nqrf=" + QRFSnapshot(qrf);
		snapshot += "\nobjective=" + ObjectiveSnapshot(objective);
		snapshot += "\nsite=" + SiteSnapshot(site);
		snapshot += "\nroute=" + RouteSnapshot(route);
		return snapshot;
	}

	protected string BuildMigrationSnapshot(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return "missing";
		string snapshot = string.Format(
			"counts %1/%2/%3/%4/%5/%6/%7/%8/%9",
			saveData.m_aZones.Count(),
			saveData.m_aGarrisons.Count(),
			saveData.m_aActiveGroups.Count(),
			saveData.m_aQRFs.Count(),
			saveData.m_aMapMarkers.Count(),
			saveData.m_aCivilianZones.Count(),
			saveData.m_aGeneratedSites.Count(),
			saveData.m_aGeneratedRoutes.Count(),
			saveData.m_aCampaignEvents.Count());
		snapshot += string.Format(
			"|extra %1/%2/%3/%4/%5/%6",
			saveData.m_aGarageVehicles.Count(),
			saveData.m_aRuntimeVehicles.Count(),
			saveData.m_aTownInfluenceEvents.Count(),
			saveData.m_aTownInfluenceRecords.Count(),
			saveData.m_aUndercoverPlayers.Count(),
			saveData.m_aEnemySupportLedgers.Count());
		foreach (HST_ZoneState zone : saveData.m_aZones)
			snapshot += "\nZ " + ZoneSnapshot(zone);
		foreach (HST_GarrisonState garrison : saveData.m_aGarrisons)
			snapshot += "\nG " + GarrisonSnapshot(garrison);
		foreach (HST_ActiveGroupState group : saveData.m_aActiveGroups)
			snapshot += "\nA " + GroupSnapshot(group);
		foreach (HST_QRFState qrf : saveData.m_aQRFs)
			snapshot += "\nQ " + QRFSnapshot(qrf);
		foreach (HST_SupportRequestState request : saveData.m_aSupportRequests)
			snapshot += "\nR " + RequestSnapshot(request);
		foreach (HST_EnemyOrderState order : saveData.m_aEnemyOrders)
			snapshot += "\nE " + OrderSnapshot(order);
		foreach (HST_ActiveMissionState mission : saveData.m_aActiveMissions)
			snapshot += "\nM " + MissionSnapshot(mission);
		foreach (HST_MissionObjectiveState objective : saveData.m_aMissionObjectives)
			snapshot += "\nO " + ObjectiveSnapshot(objective);
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
			snapshot += "\nP " + OperationSnapshot(operation);
		foreach (HST_GeneratedSiteState site : saveData.m_aGeneratedSites)
			snapshot += "\nS " + SiteSnapshot(site);
		foreach (HST_GeneratedRouteState route : saveData.m_aGeneratedRoutes)
			snapshot += "\nT " + RouteSnapshot(route);
		foreach (HST_EnemySupportLedgerState ledger : saveData.m_aEnemySupportLedgers)
			snapshot += "\nL " + LedgerSnapshot(ledger);
		foreach (HST_MapMarkerState marker : saveData.m_aMapMarkers)
		{
			if (marker)
				snapshot += "\nK " + marker.m_sMarkerId + "|" + marker.m_sLinkedId;
		}
		foreach (HST_GarageVehicleState garageVehicle : saveData.m_aGarageVehicles)
		{
			if (garageVehicle)
			{
				snapshot += "\nV " + string.Format(
					"%1|%2|%3",
					garageVehicle.m_sVehicleId,
					garageVehicle.m_sSourceZoneId,
					garageVehicle.m_sLastReporterZoneId);
			}
		}
		foreach (HST_RuntimeVehicleState runtimeVehicle : saveData.m_aRuntimeVehicles)
		{
			if (runtimeVehicle)
			{
				snapshot += "\nW " + string.Format(
					"%1|%2|%3|%4",
					runtimeVehicle.m_sVehicleRuntimeId,
					runtimeVehicle.m_sZoneId,
					runtimeVehicle.m_sLastReporterZoneId,
					runtimeVehicle.m_sRuntimeKind);
			}
		}
		foreach (HST_CivilianZoneState civilian : saveData.m_aCivilianZones)
		{
			if (civilian)
				snapshot += "\nI " + civilian.m_sZoneId + "|" + civilian.m_iPopulationRemaining;
		}
		foreach (HST_TownInfluenceEventState influence : saveData.m_aTownInfluenceEvents)
		{
			if (influence)
				snapshot += "\nN " + influence.m_sEventId + "|" + influence.m_sZoneId;
		}
		foreach (HST_TownInfluenceRecord influenceRecord : saveData.m_aTownInfluenceRecords)
		{
			if (influenceRecord)
				snapshot += "\nJ " + influenceRecord.m_sTownId;
		}
		foreach (HST_PlayerUndercoverState undercover : saveData.m_aUndercoverPlayers)
			snapshot += "\nU " + string.Format(
				"%1|%2|%3",
				undercover.m_sIdentityId,
				undercover.m_sLastZoneId,
				undercover.m_sLastEnforcementZoneId);
		foreach (HST_CampaignEventState eventState : saveData.m_aCampaignEvents)
			snapshot += "\nC " + string.Format(
				"%1|%2|%3|%4",
				eventState.m_sEventId,
				eventState.m_sTransition,
				eventState.m_sReason,
				eventState.m_iCreatedAtSecond);
		return snapshot;
	}

	protected string ZoneSnapshot(HST_ZoneState zone)
	{
		if (!zone)
			return "missing";
		string snapshot = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			zone.m_sZoneId,
			zone.m_sDisplayName,
			zone.m_sOwnerFactionKey,
			zone.m_eType,
			zone.m_vPosition,
			zone.m_iSupport,
			zone.m_iResistanceCaptureProgress,
			zone.m_iIncomeValue,
			zone.m_iGarrisonSlots);
		foreach (string linkedZoneId : zone.m_aLinkedZoneIds)
			snapshot += "|link=" + linkedZoneId;
		return snapshot;
	}

	protected string GarrisonSnapshot(HST_GarrisonState garrison)
	{
		if (!garrison)
			return "missing";
		string snapshot = string.Format(
			"%1|%2|%3|%4|%5",
			garrison.m_sGarrisonId,
			garrison.m_sZoneId,
			garrison.m_sFactionKey,
			garrison.m_iInfantryCount,
			garrison.m_iVehicleCount);
		foreach (string manifestId : garrison.m_aAcceptedManifestIds)
			snapshot += "|manifest=" + manifestId;
		return snapshot;
	}

	protected string GroupSnapshot(HST_ActiveGroupState group)
	{
		if (!group)
			return "missing";
		string snapshot = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			group.m_sGroupId,
			group.m_sOperationId,
			group.m_sMissionInstanceId,
			group.m_sSupportRequestId,
			group.m_sEnemyOrderId,
			group.m_sQRFInstanceId,
			group.m_sZoneId,
			group.m_sGarrisonZoneId,
			group.m_sRouteId);
		return snapshot + string.Format(
			"|%1|%2|%3|%4",
			group.m_sRuntimeStatus,
			group.m_sSpawnFallbackMode,
			group.m_iInfantryCount,
			group.m_iSurvivorInfantryCount);
	}

	protected string QRFSnapshot(HST_QRFState qrf)
	{
		if (!qrf)
			return "missing";
		return string.Format(
			"%1|%2|%3|%4|%5|%6|%7",
			qrf.m_sInstanceId,
			qrf.m_sGroupId,
			qrf.m_sFactionKey,
			qrf.m_sSourceZoneId,
			qrf.m_sTargetZoneId,
			qrf.m_iETASeconds,
			qrf.m_bResolved);
	}

	protected string RequestSnapshot(HST_SupportRequestState request)
	{
		if (!request)
			return "missing";
		return string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			request.m_sRequestId,
			request.m_sOperationId,
			request.m_sGroupId,
			request.m_iOperationContractVersion,
			request.m_sSourceZoneId,
			request.m_sTargetZoneId,
			request.m_sDeploymentRouteId,
			request.m_sRuntimeStatus,
			request.m_iMoneyCost);
	}

	protected string OrderSnapshot(HST_EnemyOrderState order)
	{
		if (!order)
			return "missing";
		return string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8",
			order.m_sOrderId,
			order.m_sOperationId,
			order.m_sGroupId,
			order.m_iOperationContractVersion,
			order.m_sSourceZoneId,
			order.m_sTargetZoneId,
			order.m_sRuntimeStatus,
			order.m_sFailureReason);
	}

	protected string MissionSnapshot(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "missing";
		return string.Format(
			"%1|%2|%3|%4|%5|%6|%7",
			mission.m_sInstanceId,
			mission.m_sOperationId,
			mission.m_iOperationContractVersion,
			mission.m_sTargetZoneId,
			mission.m_sSiteId,
			mission.m_sRuntimePrimitive,
			mission.m_sRuntimeFailureReason);
	}

	protected string ObjectiveSnapshot(HST_MissionObjectiveState objective)
	{
		if (!objective)
			return "missing";
		return string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8",
			objective.m_sObjectiveId,
			objective.m_sMissionInstanceId,
			objective.m_eType,
			objective.m_sTargetId,
			objective.m_sTargetZoneId,
			objective.m_iCurrentProgress,
			objective.m_bComplete,
			objective.m_bFailed);
	}

	protected string OperationSnapshot(HST_OperationRecordState operation)
	{
		if (!operation)
			return "missing";
		string snapshot = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			operation.m_sOperationId,
			operation.m_eType,
			operation.m_iContractVersion,
			operation.m_sSupportRequestId,
			operation.m_sEnemyOrderId,
			operation.m_sMissionInstanceId,
			operation.m_sGroupId,
			operation.m_sOriginZoneId,
			operation.m_sAssignmentZoneId);
		return snapshot + string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8",
			operation.m_sTacticalTargetZoneId,
			operation.m_sCurrentRouteId,
			operation.m_vOriginPosition,
			operation.m_vAssignmentPosition,
			operation.m_vTacticalTargetPosition,
			operation.m_eSettlementState,
			operation.m_eTerminalResult,
			operation.m_iRevision);
	}

	protected string SiteSnapshot(HST_GeneratedSiteState site)
	{
		if (!site)
			return "missing";
		return string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			site.m_sSiteId,
			site.m_sZoneId,
			site.m_sRouteId,
			site.m_sSourceLayerName,
			site.m_sSourceCategory,
			site.m_sSourceLayoutId,
			site.m_eType,
			site.m_vPosition,
			site.m_iRadiusMeters);
	}

	protected string RouteSnapshot(HST_GeneratedRouteState route)
	{
		if (!route)
			return "missing";
		string snapshot = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			route.m_sRouteId,
			route.m_sSourceZoneId,
			route.m_sTargetZoneId,
			route.m_sSourceLayerName,
			route.m_sSourceCategory,
			route.m_sSourceLayoutId,
			route.m_vStartPosition,
			route.m_vMidPosition,
			route.m_vEndPosition);
		snapshot += string.Format(
			"|%1|%2",
			route.m_iDistanceMeters,
			route.m_iWaypointCount);
		foreach (HST_RouteWaypointState waypoint : route.m_aWaypoints)
		{
			if (!waypoint)
			{
				snapshot += "|waypoint=missing";
				continue;
			}
			snapshot += string.Format(
				"|waypoint=%1/%2/%3/%4/%5",
				waypoint.m_sRouteId,
				waypoint.m_iIndex,
				waypoint.m_vPosition,
				waypoint.m_iRadiusMeters,
				waypoint.m_sHint);
		}
		return snapshot;
	}

	protected string LedgerSnapshot(HST_EnemySupportLedgerState ledger)
	{
		if (!ledger)
			return "missing";
		string snapshot = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			ledger.m_sFactionKey,
			ledger.m_sZoneId,
			ledger.m_sLastDecisionReason,
			ledger.m_iRecentDamageScore,
			ledger.m_iLastDamageSecond,
			ledger.m_iAttackSpent,
			ledger.m_iSupportSpent,
			ledger.m_iLastSpendSecond,
			ledger.m_iCooldownUntilSecond);
		return snapshot + string.Format(
			"|%1|%2",
			ledger.m_iRefundedAttackResources,
			ledger.m_iRefundedSupportResources);
	}

	protected HST_ZoneState FindZone(HST_CampaignSaveData saveData, string zoneId)
	{
		if (!saveData)
			return null;
		foreach (HST_ZoneState zone : saveData.m_aZones)
		{
			if (zone && zone.m_sZoneId == zoneId)
				return zone;
		}
		return null;
	}

	protected int CountZone(HST_CampaignSaveData saveData, string zoneId)
	{
		int count;
		if (!saveData)
			return count;
		foreach (HST_ZoneState zone : saveData.m_aZones)
		{
			if (zone && zone.m_sZoneId == zoneId)
				count++;
		}
		return count;
	}

	protected HST_GarrisonState FindGarrison(HST_CampaignSaveData saveData, string zoneId, string factionKey)
	{
		if (!saveData)
			return null;
		foreach (HST_GarrisonState garrison : saveData.m_aGarrisons)
		{
			if (garrison && garrison.m_sZoneId == zoneId && garrison.m_sFactionKey == factionKey)
				return garrison;
		}
		return null;
	}

	protected int CountGarrison(HST_CampaignSaveData saveData, string zoneId, string factionKey)
	{
		int count;
		if (!saveData)
			return count;
		foreach (HST_GarrisonState garrison : saveData.m_aGarrisons)
		{
			if (garrison && garrison.m_sZoneId == zoneId && garrison.m_sFactionKey == factionKey)
				count++;
		}
		return count;
	}

	protected HST_ActiveGroupState FindGroup(HST_CampaignSaveData saveData, string groupId)
	{
		if (!saveData)
			return null;
		foreach (HST_ActiveGroupState group : saveData.m_aActiveGroups)
		{
			if (group && group.m_sGroupId == groupId)
				return group;
		}
		return null;
	}

	protected HST_QRFState FindQRF(HST_CampaignSaveData saveData, string instanceId)
	{
		if (!saveData)
			return null;
		foreach (HST_QRFState qrf : saveData.m_aQRFs)
		{
			if (qrf && qrf.m_sInstanceId == instanceId)
				return qrf;
		}
		return null;
	}

	protected HST_SupportRequestState FindRequest(HST_CampaignSaveData saveData, string requestId)
	{
		if (!saveData)
			return null;
		foreach (HST_SupportRequestState request : saveData.m_aSupportRequests)
		{
			if (request && request.m_sRequestId == requestId)
				return request;
		}
		return null;
	}

	protected HST_EnemyOrderState FindOrder(HST_CampaignSaveData saveData, string orderId)
	{
		if (!saveData)
			return null;
		foreach (HST_EnemyOrderState order : saveData.m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				return order;
		}
		return null;
	}

	protected HST_ActiveMissionState FindMission(HST_CampaignSaveData saveData, string instanceId)
	{
		if (!saveData)
			return null;
		foreach (HST_ActiveMissionState mission : saveData.m_aActiveMissions)
		{
			if (mission && mission.m_sInstanceId == instanceId)
				return mission;
		}
		return null;
	}

	protected HST_MissionObjectiveState FindObjective(HST_CampaignSaveData saveData, string objectiveId)
	{
		if (!saveData)
			return null;
		foreach (HST_MissionObjectiveState objective : saveData.m_aMissionObjectives)
		{
			if (objective && objective.m_sObjectiveId == objectiveId)
				return objective;
		}
		return null;
	}

	protected HST_OperationRecordState FindOperation(HST_CampaignSaveData saveData, string operationId)
	{
		if (!saveData)
			return null;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (operation && operation.m_sOperationId == operationId)
				return operation;
		}
		return null;
	}

	protected HST_MapMarkerState FindMarker(HST_CampaignSaveData saveData, string markerId)
	{
		if (!saveData)
			return null;
		foreach (HST_MapMarkerState marker : saveData.m_aMapMarkers)
		{
			if (marker && !marker.m_bTombstone && marker.m_sMarkerId == markerId)
				return marker;
		}
		return null;
	}

	protected HST_CivilianZoneState FindCivilian(HST_CampaignSaveData saveData, string zoneId)
	{
		if (!saveData)
			return null;
		foreach (HST_CivilianZoneState civilian : saveData.m_aCivilianZones)
		{
			if (civilian && civilian.m_sZoneId == zoneId)
				return civilian;
		}
		return null;
	}

	protected HST_TownInfluenceEventState FindInfluence(HST_CampaignSaveData saveData, string eventId)
	{
		if (!saveData)
			return null;
		foreach (HST_TownInfluenceEventState influence : saveData.m_aTownInfluenceEvents)
		{
			if (influence && influence.m_sEventId == eventId)
				return influence;
		}
		return null;
	}

	protected HST_TownInfluenceRecord FindInfluenceRecord(HST_CampaignSaveData saveData, string townId)
	{
		if (!saveData)
			return null;
		foreach (HST_TownInfluenceRecord record : saveData.m_aTownInfluenceRecords)
		{
			if (record && record.m_sTownId == townId)
				return record;
		}
		return null;
	}

	protected HST_GarageVehicleState FindGarageVehicle(HST_CampaignSaveData saveData, string vehicleId)
	{
		if (!saveData)
			return null;
		foreach (HST_GarageVehicleState vehicle : saveData.m_aGarageVehicles)
		{
			if (vehicle && vehicle.m_sVehicleId == vehicleId)
				return vehicle;
		}
		return null;
	}

	protected HST_RuntimeVehicleState FindRuntimeVehicle(HST_CampaignSaveData saveData, string vehicleId)
	{
		if (!saveData)
			return null;
		foreach (HST_RuntimeVehicleState vehicle : saveData.m_aRuntimeVehicles)
		{
			if (vehicle && vehicle.m_sVehicleRuntimeId == vehicleId)
				return vehicle;
		}
		return null;
	}

	protected HST_PlayerUndercoverState FindUndercover(HST_CampaignSaveData saveData, string identityId)
	{
		if (!saveData)
			return null;
		foreach (HST_PlayerUndercoverState undercover : saveData.m_aUndercoverPlayers)
		{
			if (undercover && undercover.m_sIdentityId == identityId)
				return undercover;
		}
		return null;
	}

	protected HST_EnemySupportLedgerState FindLedger(HST_CampaignSaveData saveData, string factionKey, string zoneId)
	{
		if (!saveData)
			return null;
		foreach (HST_EnemySupportLedgerState ledger : saveData.m_aEnemySupportLedgers)
		{
			if (ledger && ledger.m_sFactionKey == factionKey && ledger.m_sZoneId == zoneId)
				return ledger;
		}
		return null;
	}

	protected int CountLedger(HST_CampaignSaveData saveData, string factionKey, string zoneId)
	{
		int count;
		if (!saveData)
			return count;
		foreach (HST_EnemySupportLedgerState ledger : saveData.m_aEnemySupportLedgers)
		{
			if (ledger && ledger.m_sFactionKey == factionKey && ledger.m_sZoneId == zoneId)
				count++;
		}
		return count;
	}

	protected HST_GeneratedSiteState FindSite(HST_CampaignSaveData saveData, string siteId)
	{
		if (!saveData)
			return null;
		foreach (HST_GeneratedSiteState site : saveData.m_aGeneratedSites)
		{
			if (site && site.m_sSiteId == siteId)
				return site;
		}
		return null;
	}

	protected int CountSite(HST_CampaignSaveData saveData, string siteId)
	{
		int count;
		if (!saveData)
			return count;
		foreach (HST_GeneratedSiteState site : saveData.m_aGeneratedSites)
		{
			if (site && site.m_sSiteId == siteId)
				count++;
		}
		return count;
	}

	protected HST_GeneratedRouteState FindRoute(HST_CampaignSaveData saveData, string routeId)
	{
		if (!saveData)
			return null;
		foreach (HST_GeneratedRouteState route : saveData.m_aGeneratedRoutes)
		{
			if (route && route.m_sRouteId == routeId)
				return route;
		}
		return null;
	}

	protected int CountRoute(HST_CampaignSaveData saveData, string routeId)
	{
		int count;
		if (!saveData)
			return count;
		foreach (HST_GeneratedRouteState route : saveData.m_aGeneratedRoutes)
		{
			if (route && route.m_sRouteId == routeId)
				count++;
		}
		return count;
	}

	protected int CountMigrationEvent(HST_CampaignSaveData saveData)
	{
		int count;
		if (!saveData)
			return count;
		foreach (HST_CampaignEventState eventState : saveData.m_aCampaignEvents)
		{
			if (eventState
				&& eventState.m_sEventId == HST_MaidensBayLocationSaveValidationService.MIGRATION_EVENT_ID)
				count++;
		}
		return count;
	}
}
#endif
