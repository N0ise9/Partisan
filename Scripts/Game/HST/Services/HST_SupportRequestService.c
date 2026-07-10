class HST_SupportRequestResult
{
	bool m_bSuccess;
	string m_sFailureReason;
	ref HST_SupportRequestState m_Request;

	string BuildSummary()
	{
		if (!m_bSuccess)
			return "h-istasi support | failed: " + m_sFailureReason;

		if (!m_Request)
			return "h-istasi support | failed: request missing after success";

		string summary = string.Format(
			"h-istasi support | requested %1 | %2 | target %3 at %4 | eta %5s",
			m_Request.m_sRequestId,
			m_Request.m_eType,
			m_Request.m_sTargetZoneId,
			m_Request.m_vTargetPosition,
			m_Request.m_iETASeconds
		);
		summary = summary + string.Format(
			" | cost $%1 HR %2 planned FIA %3 enemy %4/%5 | cooldown %6",
			m_Request.m_iMoneyCost,
			m_Request.m_iHRCost,
			m_Request.m_iPlannedInfantryCount,
			m_Request.m_iAttackCost,
			m_Request.m_iSupportCost,
			m_Request.m_iCooldownUntilSecond
		);
		return summary;
	}
}
class HST_SupportRequestService
{
	static const int PLAYER_SUPPLY_COST = 150;
	static const int PLAYER_QRF_COST = 250;
	static const int PLAYER_FIRE_COST = 350;
	static const int PLAYER_AIRSTRIKE_COST = 750;
	static const int PLAYER_CRUISE_MISSILE_COST = 1200;
	static const int PLAYER_ROADBLOCK_COST = 0;
	static const int DEFAULT_ETA_SECONDS = 120;
	static const int HELICOPTER_STYLE_ETA_SECONDS = 180;
	static const int PLAYER_SUPPORT_COOLDOWN_SECONDS = 600;
	static const int PHYSICAL_SUPPORT_INBOUND_SPAWN_SECONDS = 30;
	static const float PHYSICAL_SUPPORT_MIN_STANDOFF_METERS = 220.0;
	static const float PHYSICAL_SUPPORT_EXTRA_STANDOFF_METERS = 140.0;
	static const float PHYSICAL_SUPPORT_MAX_STANDOFF_METERS = 650.0;
	static const float PETROS_ATTACK_MIN_STANDOFF_METERS = 760.0;
	static const float PETROS_ATTACK_STAGING_MARGIN_METERS = 90.0;
	static const float PETROS_ATTACK_MAX_STAGING_METERS = 1120.0;
	static const float HQ_SAFE_RADIUS_METERS = 900.0;
	static const float SUPPORT_NEAR_HQ_STRATEGIC_RADIUS_METERS = 700.0;
	static const int SUPPORT_NEAR_HQ_KNOWLEDGE_GAIN = 4;
	static const float SUPPORT_RECALL_EXIT_DISTANCE_METERS = 2100.0;
	static const string EXACT_PLAYER_SUPPORT_MODE = "exact_spawn_queue";
	static const string EXACT_PLAYER_SUPPORT_GROUP_STATUS = "exact_support_spawn_queued";
	static const int EXACT_PLAYER_SUPPORT_PRIORITY = 100;
	static const int EXACT_PLAYER_SUPPORT_MAX_RETRIES = 3;
	static const int EXACT_PLAYER_SUPPORT_DEPLOYMENT_GRACE_SECONDS = 120;

	protected bool m_bMarkerRefreshNeeded;
	protected ref HST_ForceCompositionService m_ForceCompositions = new HST_ForceCompositionService();
	protected ref HST_SpawnPlacementService m_SpawnPlacements = new HST_SpawnPlacementService();
	protected ref HST_ForcePlanningIntegrityService m_ForceIntegrity = new HST_ForcePlanningIntegrityService();
	protected ref HST_ForceSpawnQueueService m_ExactForceSpawnQueue;
	protected ref HST_ResourceLedgerService m_ExactResourceLedger;
	protected ref HST_EconomyService m_ExactEconomy;
	protected bool m_bSuccessfulExactPlayerSupportRestorePassComplete;

	void SetForceCompositionService(HST_ForceCompositionService forceCompositions)
	{
		if (forceCompositions)
			m_ForceCompositions = forceCompositions;
	}

	void SetSpawnPlacementService(HST_SpawnPlacementService spawnPlacements)
	{
		if (spawnPlacements)
			m_SpawnPlacements = spawnPlacements;
	}

	void SetExactForceAuthorityServices(HST_ForceSpawnQueueService spawnQueue, HST_ResourceLedgerService resourceLedger, HST_EconomyService economy)
	{
		m_ExactForceSpawnQueue = spawnQueue;
		m_ExactResourceLedger = resourceLedger;
		m_ExactEconomy = economy;
	}

	HST_SupportRequestState RequestSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ESupportRequestType supportType, string targetZoneId, bool playerRequested = false, int playerCooldownSeconds = PLAYER_SUPPORT_COOLDOWN_SECONDS)
	{
		HST_SupportRequestResult result = RequestSupportDetailed(state, preset, economy, enemyDirector, factionKey, supportType, targetZoneId, playerRequested, playerCooldownSeconds);
		if (!result || !result.m_bSuccess)
			return null;

		return result.m_Request;
	}

	HST_SupportRequestResult RequestSupportDetailed(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EconomyService economy,
		HST_EnemyDirectorService enemyDirector,
		string factionKey,
		HST_ESupportRequestType supportType,
		string targetZoneId,
		bool playerRequested = false,
		int playerCooldownSeconds = PLAYER_SUPPORT_COOLDOWN_SECONDS)
	{
		return RequestSupportDetailedInternal(
			state,
			preset,
			economy,
			enemyDirector,
			factionKey,
			supportType,
			targetZoneId,
			"0 0 0",
			false,
			playerRequested,
			playerCooldownSeconds
		);
	}

	HST_SupportRequestResult RequestSupportAtPositionDetailed(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EconomyService economy,
		HST_EnemyDirectorService enemyDirector,
		string factionKey,
		HST_ESupportRequestType supportType,
		string targetZoneId,
		vector targetPosition,
		bool playerRequested = false,
		int playerCooldownSeconds = PLAYER_SUPPORT_COOLDOWN_SECONDS)
	{
		return RequestSupportDetailedInternal(
			state,
			preset,
			economy,
			enemyDirector,
			factionKey,
			supportType,
			targetZoneId,
			targetPosition,
			true,
			playerRequested,
			playerCooldownSeconds
		);
	}

	HST_SupportRequestResult RequestCampaignDebugLegacyPlayerSupportDetailed(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EconomyService economy,
		HST_EnemyDirectorService enemyDirector,
		string factionKey,
		HST_ESupportRequestType supportType,
		string targetZoneId,
		bool allowLegacyPlayerSupport,
		int playerCooldownSeconds = PLAYER_SUPPORT_COOLDOWN_SECONDS)
	{
		return RequestSupportDetailedInternal(
			state,
			preset,
			economy,
			enemyDirector,
			factionKey,
			supportType,
			targetZoneId,
			"0 0 0",
			false,
			true,
			playerCooldownSeconds,
			"",
			allowLegacyPlayerSupport
		);
	}

	HST_SupportRequestResult RequestCampaignDebugLegacyPlayerSupportAtPositionDetailed(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EconomyService economy,
		HST_EnemyDirectorService enemyDirector,
		string factionKey,
		HST_ESupportRequestType supportType,
		string targetZoneId,
		vector targetPosition,
		bool allowLegacyPlayerSupport,
		int playerCooldownSeconds = PLAYER_SUPPORT_COOLDOWN_SECONDS)
	{
		return RequestSupportDetailedInternal(
			state,
			preset,
			economy,
			enemyDirector,
			factionKey,
			supportType,
			targetZoneId,
			targetPosition,
			true,
			true,
			playerCooldownSeconds,
			"",
			allowLegacyPlayerSupport
		);
	}

	HST_SupportRequestResult RequestRoadblockAtPositionDetailed(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EconomyService economy,
		HST_EnemyDirectorService enemyDirector,
		string targetZoneId,
		vector targetPosition,
		string garageVehicleId,
		bool playerRequested = true,
		int playerCooldownSeconds = PLAYER_SUPPORT_COOLDOWN_SECONDS)
	{
		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		return RequestSupportDetailedInternal(
			state,
			preset,
			economy,
			enemyDirector,
			resistanceFactionKey,
			HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK,
			targetZoneId,
			targetPosition,
			true,
			playerRequested,
			playerCooldownSeconds,
			garageVehicleId
		);
	}

	protected HST_SupportRequestResult RequestSupportDetailedInternal(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EconomyService economy,
		HST_EnemyDirectorService enemyDirector,
		string factionKey,
		HST_ESupportRequestType supportType,
		string targetZoneId,
		vector targetPosition,
		bool useExplicitTargetPosition,
		bool playerRequested,
		int playerCooldownSeconds,
		string selectedGarageVehicleId = "",
		bool allowCampaignDebugLegacyPlayerSupport = false)
	{
		HST_SupportRequestResult result = new HST_SupportRequestResult();

		if (!state || !preset)
		{
			result.m_sFailureReason = "campaign state or preset not ready";
			return result;
		}

		string resistanceFactionKey = preset.m_sResistanceFactionKey;
		if (resistanceFactionKey.IsEmpty())
			resistanceFactionKey = "FIA";

		bool exactPlayerQRF = playerRequested && supportType == HST_ESupportRequestType.HST_SUPPORT_QRF;
		exactPlayerQRF = exactPlayerQRF && factionKey == resistanceFactionKey;
		if (exactPlayerQRF && !allowCampaignDebugLegacyPlayerSupport)
		{
			result.m_sFailureReason = "exact player QRF requires an accepted server quote";
			return result;
		}

		if (factionKey.IsEmpty())
		{
			result.m_sFailureReason = "faction key missing";
			return result;
		}

		HST_ZoneState targetZone = state.FindZone(targetZoneId);
		if (!targetZone && targetZoneId.IsEmpty())
			targetZone = FindFallbackTargetZone(state, preset);

		if (!targetZone)
		{
			result.m_sFailureReason = "target zone not found";
			return result;
		}

		vector resolvedTargetPosition = targetZone.m_vPosition;
		if (useExplicitTargetPosition && !IsZeroVector(targetPosition))
			resolvedTargetPosition = HST_WorldPositionService.ResolveGroundPosition(targetPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);

		int attackCost;
		int supportCost;
		ResolveCosts(supportType, attackCost, supportCost);

		string sourceZoneId = FindSourceZoneId(state, preset, factionKey, targetZone.m_sZoneId);
		int moneyCost = ResolvePlayerMoneyCost(supportType);
		HST_SupportRequestState request = BuildSupportRequestRecord(
			state,
			preset,
			factionKey,
			supportType,
			targetZone,
			resolvedTargetPosition,
			ResolveSourcePosition(state, sourceZoneId, resolvedTargetPosition),
			playerRequested,
			moneyCost,
			attackCost,
			supportCost,
			playerCooldownSeconds
		);

		HST_GarageVehicleState selectedRoadblockVehicle;
		if (factionKey == preset.m_sResistanceFactionKey && supportType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
		{
			selectedRoadblockVehicle = ResolveRoadblockGarageVehicle(state, selectedGarageVehicleId);
			if (!selectedRoadblockVehicle)
			{
				result.m_sFailureReason = "no stored HQ vehicle selected for roadblock";
				if (!selectedGarageVehicleId.IsEmpty())
					result.m_sFailureReason = "stored HQ vehicle not found for roadblock: " + selectedGarageVehicleId;
				return result;
			}

			ApplyGarageVehicleToRoadblockRequest(request, selectedRoadblockVehicle);
		}

		if (factionKey == preset.m_sResistanceFactionKey)
		{
			string cooldownReason;
			if (HasActivePlayerRequestDetailed(state, supportType, cooldownReason))
			{
				result.m_sFailureReason = cooldownReason;
				return result;
			}

			if (!economy)
			{
				result.m_sFailureReason = "economy service not ready";
				return result;
			}

			string hrFailureReason;
			int hrCost = ResolvePlayerHRCost(state, preset, request, hrFailureReason);
			if (!hrFailureReason.IsEmpty())
			{
				result.m_sFailureReason = hrFailureReason;
				return result;
			}

			request.m_iHRCost = hrCost;
			if (request.m_iPlannedInfantryCount <= 0 && hrCost > 0)
				request.m_iPlannedInfantryCount = hrCost;

			if (state.m_iFactionMoney < moneyCost)
			{
				result.m_sFailureReason = string.Format("need $%1, have $%2", moneyCost, state.m_iFactionMoney);
				return result;
			}

			if (state.m_iHR < hrCost)
			{
				result.m_sFailureReason = string.Format("need %1 HR, have %2", hrCost, state.m_iHR);
				return result;
			}

			if (!economy.SpendFactionMoney(state, moneyCost))
			{
				result.m_sFailureReason = "money spend failed";
				return result;
			}

			if (!economy.SpendHR(state, hrCost))
			{
				economy.AddFactionMoney(state, moneyCost);
				result.m_sFailureReason = "HR spend failed";
				return result;
			}

			if (supportType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
			{
				HST_GarageVehicleState consumedVehicle = ConsumeRoadblockGarageVehicle(state, request.m_sSelectedGarageVehicleId);
				if (!consumedVehicle)
				{
					economy.AddFactionMoney(state, moneyCost);
					economy.AddHR(state, hrCost);
					result.m_sFailureReason = "selected HQ garage vehicle could not be consumed";
					return result;
				}

				request.m_bGarageVehicleConsumed = true;
			}
		}
		else
		{
			if (!enemyDirector)
			{
				result.m_sFailureReason = "enemy director not ready";
				return result;
			}

			attackCost = 0;
			string spendReason;
			if (!enemyDirector.CanSpendDefense(state, targetZone, factionKey, attackCost, supportCost, spendReason))
			{
				result.m_sFailureReason = "enemy support denied: " + spendReason;
				return result;
			}

			if (!enemyDirector.TrySpendDefense(state, targetZone, factionKey, attackCost, supportCost, spendReason))
			{
				result.m_sFailureReason = "enemy support spend failed: " + spendReason;
				return result;
			}
		}

		state.m_aSupportRequests.Insert(request);
		m_bMarkerRefreshNeeded = true;
		result.m_Request = request;
		result.m_bSuccess = true;

		Print(string.Format("h-istasi | support request %1 queued for %2 at %3 target %4", request.m_sRequestId, factionKey, targetZone.m_sZoneId, resolvedTargetPosition));
		return result;
	}

	HST_SupportRequestState RequestPrepaidEnemySupport(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, HST_ESupportRequestType supportType, string targetZoneId, vector sourcePosition, vector targetPosition)
	{
		if (!state || !preset || factionKey.IsEmpty())
			return null;

		HST_ZoneState targetZone = state.FindZone(targetZoneId);
		if (!targetZone)
			return null;

		vector resolvedTargetPosition = targetPosition;
		if (IsZeroVector(resolvedTargetPosition))
			resolvedTargetPosition = targetZone.m_vPosition;

		string sourceZoneId = FindSourceZoneId(state, preset, factionKey, targetZone.m_sZoneId);
		vector resolvedSourcePosition = sourcePosition;
		if (IsZeroVector(resolvedSourcePosition))
			resolvedSourcePosition = ResolveSourcePosition(state, sourceZoneId, resolvedTargetPosition);

		int attackCost;
		int supportCost;
		ResolveCosts(supportType, attackCost, supportCost);

		HST_SupportRequestState request = BuildSupportRequestRecord(state, preset, factionKey, supportType, targetZone, resolvedTargetPosition, resolvedSourcePosition, false, 0, attackCost, supportCost, 0);
		state.m_aSupportRequests.Insert(request);
		m_bMarkerRefreshNeeded = true;
		Print(string.Format("h-istasi | prepaid enemy support %1 linked to order for %2 at %3", request.m_sRequestId, factionKey, targetZone.m_sZoneId));
		return request;
	}

	protected HST_GarageVehicleState ResolveRoadblockGarageVehicle(HST_CampaignState state, string garageVehicleId)
	{
		if (!state || state.m_aGarageVehicles.Count() == 0)
			return null;

		if (!garageVehicleId.IsEmpty())
			return state.FindGarageVehicle(garageVehicleId);

		foreach (HST_GarageVehicleState vehicle : state.m_aGarageVehicles)
		{
			if (vehicle && !vehicle.m_sPrefab.IsEmpty())
				return vehicle;
		}

		return null;
	}

	protected void ApplyGarageVehicleToRoadblockRequest(HST_SupportRequestState request, HST_GarageVehicleState vehicle)
	{
		if (!request || !vehicle)
			return;

		request.m_sSelectedGarageVehicleId = vehicle.m_sVehicleId;
		request.m_sSelectedGarageVehiclePrefab = vehicle.m_sPrefab;
		request.m_sSelectedGarageVehicleDisplayName = vehicle.m_sDisplayName;
		if (request.m_sSelectedGarageVehicleDisplayName.IsEmpty())
			request.m_sSelectedGarageVehicleDisplayName = HST_DisplayNameService.ResolveVehicleDisplayName(vehicle.m_sPrefab, vehicle.m_sVehicleId);
		request.m_sDeploymentSummary = "HQ garage vehicle selected: " + request.m_sSelectedGarageVehicleDisplayName;
		request.m_sPhysicalizationMode = "roadblock_vehicle_selected";
	}

	protected HST_GarageVehicleState ConsumeRoadblockGarageVehicle(HST_CampaignState state, string garageVehicleId)
	{
		if (!state || garageVehicleId.IsEmpty())
			return null;

		for (int i = 0; i < state.m_aGarageVehicles.Count(); i++)
		{
			HST_GarageVehicleState vehicle = state.m_aGarageVehicles[i];
			if (!vehicle || vehicle.m_sVehicleId != garageVehicleId)
				continue;

			state.m_aGarageVehicles.Remove(i);
			return vehicle;
		}

		return null;
	}

	protected HST_SupportRequestState BuildSupportRequestRecord(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, HST_ESupportRequestType supportType, HST_ZoneState targetZone, vector targetPosition, vector sourcePosition, bool playerRequested, int moneyCost, int attackCost, int supportCost, int playerCooldownSeconds)
	{
		HST_SupportRequestState request = new HST_SupportRequestState();
		request.m_sRequestId = BuildRequestId(state, factionKey, supportType);
		request.m_sOperationId = HST_StableIdService.BuildOperationId("support", request.m_sRequestId);
		request.m_sFactionKey = factionKey;
		request.m_sCapabilityId = CapabilityForSupport(supportType);
		request.m_sAssetProfileId = AssetProfileForSupport(factionKey, supportType, factionKey == preset.m_sResistanceFactionKey);
		request.m_sCompositionIntentId = HST_ForceCompositionService.IntentForSupport(supportType, request.m_sAssetProfileId);
		request.m_sStrikeKind = StrikeKindForSupport(supportType);
		request.m_sStrikeConfigResource = StrikeConfigForSupport(supportType);
		request.m_eType = supportType;
		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
		request.m_sRuntimeStatus = "queued";
		request.m_sPhysicalizationMode = "none";
		request.m_sTargetZoneId = targetZone.m_sZoneId;
		request.m_sSourceZoneId = FindSourceZoneId(state, preset, factionKey, targetZone.m_sZoneId);
		request.m_vTargetPosition = targetPosition;
		request.m_vSourcePosition = sourcePosition;
		request.m_iRequestedAtSecond = state.m_iElapsedSeconds;
		request.m_iETASeconds = ResolveETA(supportType);
		request.m_iAttackCost = attackCost;
		request.m_iSupportCost = supportCost;
		request.m_iMoneyCost = moneyCost;
		request.m_iHRCost = 0;
		request.m_iPlannedInfantryCount = EstimatePlayerSupportHRCost(supportType, state.m_iWarLevel);
		if (factionKey == preset.m_sResistanceFactionKey && playerCooldownSeconds > 0)
			request.m_iCooldownUntilSecond = state.m_iElapsedSeconds + Math.Max(60, playerCooldownSeconds);
		request.m_bHelicopterStyle = IsHelicopterStyle(supportType);
		request.m_bPlayerRequested = playerRequested;

		if (IsStrikeSupport(supportType))
		{
			request.m_sRuntimeEntityId = "abstract_strike";
			request.m_sPhysicalizationMode = "abstract_strike";
		}

		return request;
	}

	bool RegisterAcceptedExactPlayerSupport(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		out string failure)
	{
		failure = ValidateAcceptedExactPlayerSupportRegistration(state, preset, quote, manifest);
		if (!failure.IsEmpty())
			return false;

		HST_SupportRequestState existing = FindExactPlayerSupportByQuote(state, quote.m_sQuoteId);
		if (existing)
		{
			failure = ValidateExactPlayerSupportRequestLinks(existing, quote, manifest, false);
			return failure.IsEmpty();
		}
		if (state.FindSupportRequest(quote.m_sSupportRequestId))
		{
			failure = "support request id is already owned by another request";
			return false;
		}

		HST_ZoneState targetZone = state.FindZone(quote.m_sTargetZoneId);
		if (!targetZone)
		{
			failure = "exact support target zone is missing";
			return false;
		}

		HST_SupportRequestState request = new HST_SupportRequestState();
		request.m_sRequestId = quote.m_sSupportRequestId;
		request.m_sOperationId = quote.m_sOperationId;
		request.m_sQuoteId = quote.m_sQuoteId;
		request.m_sManifestId = manifest.m_sManifestId;
		request.m_sSpawnResultId = BuildExactSupportSpawnResultId(request.m_sRequestId);
		request.m_sCommandRequestId = quote.m_sConfirmationRequestId;
		request.m_sMoneyTransactionId = quote.m_sMoneyTransactionId;
		request.m_sHRTransactionId = quote.m_sHRTransactionId;
		request.m_sFactionKey = quote.m_sFactionKey;
		request.m_sCapabilityId = quote.m_sCapabilityId;
		request.m_sAssetProfileId = quote.m_sAssetProfileId;
		request.m_sCompositionRequestId = manifest.m_sManifestId;
		request.m_sCompositionIntentId = manifest.m_sIntentId;
		request.m_sCompositionTier = "exact";
		request.m_sCompositionSummary = string.Format("exact accepted QRF manifest %1 | members %2 | root %3", manifest.m_sManifestId, manifest.m_iAcceptedMemberCount, ResolveExactManifestGroupPrefab(manifest));
		request.m_eType = HST_ESupportRequestType.HST_SUPPORT_QRF;
		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_QUEUED;
		request.m_sSourceZoneId = quote.m_sSourceZoneId;
		request.m_sTargetZoneId = quote.m_sTargetZoneId;
		request.m_sRuntimeStatus = "exact_waiting_eta";
		request.m_sPhysicalizationMode = EXACT_PLAYER_SUPPORT_MODE;
		request.m_vSourcePosition = quote.m_vSourcePosition;
		request.m_vTargetPosition = quote.m_vTargetPosition;
		if (IsZeroVector(request.m_vSourcePosition))
			request.m_vSourcePosition = ResolveSourcePosition(state, request.m_sSourceZoneId, request.m_vTargetPosition);
		if (IsZeroVector(request.m_vTargetPosition))
			request.m_vTargetPosition = targetZone.m_vPosition;
		request.m_iRequestedAtSecond = state.m_iElapsedSeconds;
		request.m_iETASeconds = quote.m_iETASeconds;
		request.m_iMoneyCost = manifest.m_iMoneyCost;
		request.m_iHRCost = manifest.m_iHRCost;
		request.m_iPlannedInfantryCount = manifest.m_iAcceptedMemberCount;
		request.m_iCompositionCost = manifest.m_iMoneyCost;
		request.m_iCompositionManpower = manifest.m_iAcceptedMemberCount;
		request.m_iCooldownUntilSecond = state.m_iElapsedSeconds + Math.Max(0, quote.m_iCooldownSeconds);
		request.m_bPlayerRequested = true;

		state.m_aSupportRequests.Insert(request);
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	bool RemoveAcceptedExactPlayerSupport(HST_CampaignState state, string quoteId, string supportRequestId)
	{
		if (!state || quoteId.IsEmpty() || supportRequestId.IsEmpty())
			return false;

		for (int index = state.m_aSupportRequests.Count() - 1; index >= 0; index--)
		{
			HST_SupportRequestState request = state.m_aSupportRequests[index];
			if (!request || request.m_sRequestId != supportRequestId || request.m_sQuoteId != quoteId)
				continue;
			if (!request.m_sGroupId.IsEmpty() || FindExactSupportSpawnBatch(state, request))
				return false;

			state.m_aSupportRequests.Remove(index);
			m_bMarkerRefreshNeeded = true;
			return true;
		}
		return false;
	}

	HST_ForceSpawnQueueEnqueueResult EnqueueAcceptedExactPlayerSupportProjection(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_SupportRequestState request,
		vector sourcePosition,
		vector targetPosition)
	{
		HST_ForceSpawnQueueEnqueueResult result = new HST_ForceSpawnQueueEnqueueResult();
		if (!m_ExactForceSpawnQueue)
		{
			result.m_sFailureReason = "exact force spawn queue is not ready";
			return result;
		}

		HST_ForceQuoteState quote;
		HST_ForceManifestState manifest;
		string contextFailure = ResolveAcceptedExactPlayerSupportContext(state, preset, request, quote, manifest, false);
		if (!contextFailure.IsEmpty())
		{
			result.m_sFailureReason = contextFailure;
			return result;
		}
		if (IsZeroVector(sourcePosition) || IsZeroVector(targetPosition))
		{
			result.m_sFailureReason = "exact support projection placement is incomplete";
			return result;
		}

		int deadlineSecond = ResolveExactSupportDeploymentDeadline(request);
		if (deadlineSecond <= state.m_iElapsedSeconds)
		{
			result.m_sFailureReason = "exact support projection admission deadline expired";
			return result;
		}

		string resultId = BuildExactSupportSpawnResultId(request.m_sRequestId);
		string forceId = BuildExactSupportForceId(request.m_sOperationId);
		string projectionId = BuildExactSupportProjectionId(request.m_sOperationId);
		if (request.m_sSpawnResultId != resultId)
		{
			result.m_sFailureReason = "exact support spawn result identity conflict";
			return result;
		}

		HST_ActiveGroupState activeGroup = state.FindActiveGroup(projectionId);
		bool createdGroup;
		if (!activeGroup)
		{
			activeGroup = BuildExactSupportActiveGroup(state, request, manifest, sourcePosition, targetPosition, resultId, forceId, projectionId);
			if (!activeGroup)
			{
				result.m_sFailureReason = "exact support active-group projection could not be built";
				return result;
			}
			state.m_aActiveGroups.Insert(activeGroup);
			createdGroup = true;
		}
		else
		{
			string groupFailure = ValidateExactSupportActiveGroup(activeGroup, request, manifest, resultId, forceId, projectionId);
			if (!groupFailure.IsEmpty())
			{
				result.m_sFailureReason = groupFailure;
				return result;
			}
		}

		HST_ForceSpawnQueueRequest spawnRequest = new HST_ForceSpawnQueueRequest();
		spawnRequest.m_sResultId = resultId;
		spawnRequest.m_sRequestId = request.m_sRequestId;
		spawnRequest.m_sForceId = forceId;
		spawnRequest.m_sProjectionId = projectionId;
		spawnRequest.m_iPriority = EXACT_PLAYER_SUPPORT_PRIORITY;
		spawnRequest.m_iMaxRetries = EXACT_PLAYER_SUPPORT_MAX_RETRIES;
		spawnRequest.m_iDeadlineSecond = deadlineSecond;
		result = m_ExactForceSpawnQueue.Enqueue(state.m_aForceSpawnResults, manifest, spawnRequest, state.m_iElapsedSeconds);
		if (!result || !result.m_bSuccess)
		{
			if (createdGroup)
				RemoveExactSupportActiveGroup(state, projectionId);
			return result;
		}

		request.m_sGroupId = activeGroup.m_sGroupId;
		request.m_sDeploymentRouteId = activeGroup.m_sRouteId;
		request.m_sRuntimeStatus = EXACT_PLAYER_SUPPORT_GROUP_STATUS;
		request.m_sPhysicalizationMode = EXACT_PLAYER_SUPPORT_MODE;
		m_bMarkerRefreshNeeded = true;
		return result;
	}

	bool ConsumeMarkerRefreshNeeded()
	{
		bool result = m_bMarkerRefreshNeeded;
		m_bMarkerRefreshNeeded = false;
		return result;
	}

	bool Tick(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_PhysicalWarService physicalWar = null, HST_StrategicService strategic = null, HST_HQService hq = null, HST_EconomyService economy = null, HST_ForceSpawnAdapterService forceSpawnAdapter = null)
	{
		if (!state || !preset)
			return false;

		bool changed = ReconcileSuccessfulExactPlayerSupportRuntimeAfterRestore(state, physicalWar, forceSpawnAdapter);
		changed = TickExactPlayerSupportSettlements(state, physicalWar, forceSpawnAdapter) || changed;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
				continue;

			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED)
				changed = ActivateSupportRequest(state, preset, request) || changed;

			if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				continue;

			changed = TickActiveSupportRequest(state, preset, garrisons, physicalWar, request, strategic, hq, economy, forceSpawnAdapter) || changed;
		}

		return changed;
	}

	protected bool ActivateSupportRequest(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request)
	{
		if (!state || !request)
			return false;

		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE;
		request.m_iActivatedAtSecond = state.m_iElapsedSeconds;
		request.m_sRuntimeStatus = "active_waiting_eta";

		if (IsStrikeSupport(request.m_eType))
		{
			ActivateStrikeSupport(request);
			request.m_bPhysicalized = true;
			request.m_iPhysicalizedAtSecond = state.m_iElapsedSeconds;
			request.m_sPhysicalizationMode = "abstract_strike";
			request.m_sRuntimeStatus = "abstract_strike_active";
		}

		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected bool TickActiveSupportRequest(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_PhysicalWarService physicalWar, HST_SupportRequestState request, HST_StrategicService strategic = null, HST_HQService hq = null, HST_EconomyService economy = null, HST_ForceSpawnAdapterService forceSpawnAdapter = null)
	{
		if (!state || !request)
			return false;
		if (HasExactPlayerSupportAuthorityIdentity(request))
			return TickAcceptedExactPlayerSupport(state, preset, physicalWar, forceSpawnAdapter, request);

		if (IsPhysicalGroundSupport(request))
			return TickPhysicalGroundSupport(state, preset, garrisons, physicalWar, request, strategic, hq, economy);

		int arrivalAtSecond = request.m_iRequestedAtSecond + Math.Max(0, request.m_iETASeconds);
		if (state.m_iElapsedSeconds < arrivalAtSecond)
			return false;

		return ResolveSupport(state, preset, garrisons, request, strategic, hq);
	}

	protected bool TickPhysicalGroundSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_PhysicalWarService physicalWar, HST_SupportRequestState request, HST_StrategicService strategic = null, HST_HQService hq = null, HST_EconomyService economy = null)
	{
		if (!state || !request)
			return false;

		if (request.m_bRecallRequested)
			return TickRecalledPhysicalGroundSupport(state, economy, request);

		int arrivalAtSecond = request.m_iRequestedAtSecond + Math.Max(0, request.m_iETASeconds);
		int spawnAtSecond = arrivalAtSecond - ResolveInboundSpawnLeadSeconds(request);

		if (request.m_sGroupId.IsEmpty() && state.m_iElapsedSeconds >= spawnAtSecond)
		{
			if (ShouldPhysicalizeSupport(state, preset, request))
			{
				bool physicalized = ApplyActiveSupport(state, preset, request, false);
				if (physicalized)
					return true;
			}
			else
			{
				request.m_sRuntimeStatus = "active_waiting_abstract_eta";
			}
		}

		if (state.m_iElapsedSeconds < arrivalAtSecond)
			return false;

		if (!request.m_sGroupId.IsEmpty())
		{
			if (!request.m_bAbstractResolved)
			{
				if (!ShouldPhysicalizeSupport(state, preset, request))
				{
					if (FoldPhysicalSupportOutsideBubble(state, garrisons, physicalWar, request))
						return ResolveSupportAsPhysicalComplete(state, request);

					request.m_sRuntimeStatus = "active_waiting_abstract_eta";
					return ResolveSupport(state, preset, garrisons, request, strategic, hq);
				}

				request.m_bAbstractResolved = true;
				MarkPhysicalSupportArrived(state, request);
				if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
					request.m_sRuntimeStatus = "roadblock_established";
				else
					request.m_sRuntimeStatus = "physical_arrived";
				m_bMarkerRefreshNeeded = true;
				return true;
			}

			if (!ShouldPhysicalizeSupport(state, preset, request))
			{
				if (FoldPhysicalSupportOutsideBubble(state, garrisons, physicalWar, request))
					return ResolveSupportAsPhysicalComplete(state, request);
			}

			if (IsPhysicalSupportFinished(state, request))
			{
				HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
				if (!group)
					request.m_sFailureReason = "physical support group missing";
				else
					request.m_sFailureReason = "physical support group terminal: " + group.m_sRuntimeStatus;

				return ResolveSupportAsPhysicalComplete(state, request);
			}

			return false;
		}

		return ResolveSupport(state, preset, garrisons, request);
	}

	protected bool TickAcceptedExactPlayerSupport(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_PhysicalWarService physicalWar,
		HST_ForceSpawnAdapterService forceSpawnAdapter,
		HST_SupportRequestState request)
	{
		HST_ForceQuoteState quote;
		HST_ForceManifestState manifest;
		string contextFailure = ResolveAcceptedExactPlayerSupportContext(state, preset, request, quote, manifest, false);
		HST_ForceSpawnResultState batch = FindExactSupportSpawnBatch(state, request);
		if (!contextFailure.IsEmpty())
			return FailClosedExactPlayerSupport(state, request, batch, contextFailure);

		if (request.m_bRecallRequested)
			return TickRecalledExactPlayerSupport(state, physicalWar, forceSpawnAdapter, request, batch);

		if (batch)
		{
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL)
				return SettleExactPlayerSupportDeploymentTerminal(state, request, batch, "exact QRF deployment failed: " + batch.m_sTerminalReason, false);
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
				return SettleExactPlayerSupportDeploymentTerminal(state, request, batch, "exact QRF deployment cancelled: " + batch.m_sTerminalReason, true);
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
				return TickSucceededExactPlayerSupport(state, request);

			return UpdateExactSupportQueueRuntimeStatus(request, batch);
		}

		int spawnAtSecond = request.m_iRequestedAtSecond + Math.Max(0, request.m_iETASeconds) - ResolveInboundSpawnLeadSeconds(request);
		if (state.m_iElapsedSeconds < spawnAtSecond)
			return SetExactSupportRuntimeStatus(request, "exact_waiting_eta");

		return TryStartExactPlayerSupportProjection(state, preset, request);
	}

	protected bool TryStartExactPlayerSupportProjection(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request)
	{
		if (!m_SpawnPlacements)
			m_SpawnPlacements = new HST_SpawnPlacementService();
		HST_SpawnPlacementRequest placementRequest = m_SpawnPlacements.BuildSupportPlacementRequest(state, preset, request, false, false);
		HST_SpawnPlacementResult placement = m_SpawnPlacements.ResolvePlacement(state, preset, placementRequest);
		if (!placement || !placement.m_bSuccess)
		{
			string reason = "exact QRF placement unavailable";
			if (placement && !placement.m_sFailureReason.IsEmpty())
				reason = placement.m_sFailureReason;
			request.m_sFailureReason = reason;
			if (state.m_iElapsedSeconds >= ResolveExactSupportDeploymentDeadline(request))
				return SettleExactPlayerSupportFullRefund(state, request, "exact QRF placement deadline expired: " + reason, false);
			return SetExactSupportRuntimeStatus(request, "exact_waiting_placement");
		}

		ApplyExactSupportPlacementEvidence(request, placement);
		HST_ForceSpawnQueueEnqueueResult enqueue = EnqueueAcceptedExactPlayerSupportProjection(
			state,
			preset,
			request,
			placement.m_vSpawnPosition,
			placement.m_vTargetPosition);
		if (enqueue && enqueue.m_bSuccess)
			return true;

		string failure = "exact QRF spawn admission failed";
		if (enqueue && !enqueue.m_sFailureReason.IsEmpty())
			failure = enqueue.m_sFailureReason;
		request.m_sFailureReason = failure;
		if (state.m_iElapsedSeconds < ResolveExactSupportDeploymentDeadline(request) && IsRetryableExactSupportAdmissionFailure(failure))
			return SetExactSupportRuntimeStatus(request, "exact_waiting_spawn_capacity");
		return SettleExactPlayerSupportFullRefund(state, request, "exact QRF spawn admission failed: " + failure, false);
	}

	protected bool TickSucceededExactPlayerSupport(HST_CampaignState state, HST_SupportRequestState request)
	{
		HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
		if (request.m_sRuntimeStatus == "exact_restore_waiting_queue_capacity"
			|| (group && group.m_sRuntimeStatus == "exact_restore_waiting_queue_capacity"))
			return false;
		if (!group || !group.m_bSpawnedEntity)
		{
			request.m_sFailureReason = "successful exact QRF has no live active-group projection";
			return SetExactSupportRuntimeStatus(request, "exact_success_projection_missing");
		}

		bool changed;
		if (!request.m_bPhysicalized)
		{
			request.m_bPhysicalized = true;
			request.m_iPhysicalizedAtSecond = state.m_iElapsedSeconds;
			request.m_sPhysicalizationMode = EXACT_PLAYER_SUPPORT_MODE;
			request.m_sRuntimeStatus = "physical_group_linked";
			request.m_sFailureReason = "";
			changed = true;
			m_bMarkerRefreshNeeded = true;
		}

		int arrivalAtSecond = request.m_iRequestedAtSecond + Math.Max(0, request.m_iETASeconds);
		if (state.m_iElapsedSeconds >= arrivalAtSecond && !request.m_bAbstractResolved)
		{
			request.m_bAbstractResolved = true;
			request.m_sRuntimeStatus = "physical_arrived";
			MarkPhysicalSupportArrived(state, request);
			changed = true;
			m_bMarkerRefreshNeeded = true;
		}

		if (IsPhysicalSupportFinished(state, request))
			return ResolveSupportAsPhysicalComplete(state, request) || changed;
		return changed;
	}

	protected bool TickRecalledExactPlayerSupport(
		HST_CampaignState state,
		HST_PhysicalWarService physicalWar,
		HST_ForceSpawnAdapterService forceSpawnAdapter,
		HST_SupportRequestState request,
		HST_ForceSpawnResultState batch)
	{
		if (batch && batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL)
			return SettleExactPlayerSupportDeploymentTerminal(state, request, batch, "exact QRF deployment failed: " + batch.m_sTerminalReason, false);
		if (!batch || batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			if (batch && !IsTerminalExactSupportBatch(batch))
			{
				if (!m_ExactForceSpawnQueue)
					return false;
				HST_ForceSpawnQueueCallbackResult cancel = m_ExactForceSpawnQueue.RequestCancel(state.m_aForceSpawnResults, batch.m_sResultId, state.m_iElapsedSeconds, "exact QRF recalled before deployment");
				if (!cancel || !cancel.m_bAccepted)
					return false;
				return SetExactSupportRuntimeStatus(request, "exact_recall_waiting_cleanup") || cancel.m_bStateChanged;
			}

			int eligibleInfantry = request.m_iHRCost;
			if (batch && batch.m_iSuccessfulHandoffCount > 0)
			{
				if (!m_ExactForceSpawnQueue)
					return SetExactSupportRuntimeStatus(request, "exact_recall_settlement_queue_unavailable");
				eligibleInfantry = m_ExactForceSpawnQueue.CountDurableLivingMemberSlots(batch);
			}
			return SettleExactPlayerSupportHRRefund(state, request, eligibleInfantry, "exact QRF recalled before deployment", "recalled_before_deploy");
		}

		HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
		if (!group)
		{
			request.m_sFailureReason = "exact QRF recall projection is missing";
			return SetExactSupportRuntimeStatus(request, "exact_recall_projection_missing");
		}
		if (group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "spawn_failed")
		{
			if (!m_ExactForceSpawnQueue)
				return SetExactSupportRuntimeStatus(request, "exact_recall_settlement_queue_unavailable");
			int lostSurvivors = m_ExactForceSpawnQueue.CountDurableLivingMemberSlots(batch);
			return SettleExactPlayerSupportHRRefund(state, request, lostSurvivors, "exact QRF recall group was lost", "recalled_group_lost");
		}
		if (group.m_sRuntimeStatus == "support_recalling")
			return SetExactSupportRuntimeStatus(request, "recall_routing");
		if (group.m_sRuntimeStatus != "support_recall_exited" && group.m_sRuntimeStatus != "folded")
			return false;

		if (!m_ExactForceSpawnQueue)
			return SetExactSupportRuntimeStatus(request, "exact_recall_settlement_queue_unavailable");
		int survivorInfantry = m_ExactForceSpawnQueue.CountDurableLivingMemberSlots(batch);
		if (!RetireExactSupportProjectionRuntime(state, physicalWar, forceSpawnAdapter, group))
			return SetExactSupportRuntimeStatus(request, "exact_recall_waiting_retirement");
		group.m_bSpawnedEntity = false;
		group.m_sRuntimeEntityId = "";
		group.m_sRuntimeStatus = "folded";
		return SettleExactPlayerSupportHRRefund(state, request, survivorInfantry, "exact QRF recalled survivors", "recalled_refund_hr");
	}

	protected bool TickRecalledPhysicalGroundSupport(HST_CampaignState state, HST_EconomyService economy, HST_SupportRequestState request)
	{
		if (!state || !request)
			return false;

		if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE && request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_QUEUED)
			return false;

		if (request.m_sGroupId.IsEmpty())
		{
			int fullRefund = ApplySupportRecallHRRefund(state, economy, request, Math.Max(0, request.m_iHRCost - request.m_iRefundedHR));
			request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
			request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
			request.m_sResolutionKind = "recalled_before_deploy";
			request.m_sRuntimeStatus = "resolved_recalled_before_deploy";
			request.m_sFailureReason = string.Format("recalled before deployment; refunded HR %1", fullRefund);
			m_bMarkerRefreshNeeded = true;
			return true;
		}

		HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
		if (!group)
		{
			request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
			request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
			request.m_sResolutionKind = "recalled_missing_group";
			request.m_sRuntimeStatus = "resolved_recalled_missing_group";
			request.m_sFailureReason = "recalled group missing";
			m_bMarkerRefreshNeeded = true;
			return true;
		}

		if (group.m_sRuntimeStatus == "support_recalling")
		{
			request.m_sRuntimeStatus = "recall_routing";
			return false;
		}

		if (group.m_sRuntimeStatus == "support_recall_exited" || group.m_sRuntimeStatus == "folded")
		{
			int survivorInfantry = Math.Max(0, group.m_iSurvivorInfantryCount);
			if (!group.m_bSpawnedEntity && survivorInfantry <= 0 && group.m_iInfantryCount > 0)
				survivorInfantry = group.m_iInfantryCount;
			int refunded = ApplySupportRecallHRRefund(state, economy, request, survivorInfantry);
			group.m_sRuntimeStatus = "folded";
			request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
			request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
			request.m_sResolutionKind = "recalled_refund_hr";
			request.m_sRuntimeStatus = "resolved_recalled";
			request.m_sFailureReason = string.Format("recalled survivors refunded HR %1/%2", refunded, request.m_iHRCost);
			m_bMarkerRefreshNeeded = true;
			return true;
		}

		if (group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "spawn_failed")
		{
			request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
			request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
			request.m_sResolutionKind = "recalled_group_lost";
			request.m_sRuntimeStatus = "resolved_recalled_group_lost";
			request.m_sFailureReason = "recalled group terminal: " + group.m_sRuntimeStatus;
			m_bMarkerRefreshNeeded = true;
			return true;
		}

		return false;
	}

	protected bool FoldPhysicalSupportOutsideBubble(HST_CampaignState state, HST_GarrisonService garrisons, HST_PhysicalWarService physicalWar, HST_SupportRequestState request)
	{
		if (!state || !request || request.m_sGroupId.IsEmpty())
			return false;

		HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
		if (!group)
			return false;

		if (group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "spawn_failed")
			return false;

		bool folded;
		if (physicalWar)
			folded = physicalWar.FoldActiveSupportGroup(state, group.m_sGroupId);

		if (!folded)
		{
			FoldSupportGroupDataOnly(state, garrisons, group);
			folded = true;
		}

		if (folded)
		{
			request.m_sFailureReason = "physical support folded after leaving event bubble";
			request.m_sRuntimeStatus = "physical_folded_outside_bubble";
			request.m_sResolutionKind = "physical_group_folded";
		}

		return folded;
	}

	protected void FoldSupportGroupDataOnly(HST_CampaignState state, HST_GarrisonService garrisons, HST_ActiveGroupState group)
	{
		if (!state || !group)
			return;

		if (group.m_sRuntimeStatus == "folded")
			return;

		int survivorInfantry = Math.Max(0, group.m_iSurvivorInfantryCount);
		int survivorVehicles = Math.Max(0, group.m_iSurvivorVehicleCount);
		if (!group.m_bSpawnedEntity && survivorInfantry <= 0 && group.m_iInfantryCount > 0)
			survivorInfantry = group.m_iInfantryCount;
		if (!group.m_bSpawnedEntity && survivorVehicles <= 0 && group.m_iVehicleCount > 0)
			survivorVehicles = group.m_iVehicleCount;

		if (garrisons && (survivorInfantry > 0 || survivorVehicles > 0))
			garrisons.AddAbstractForces(state, group.m_sZoneId, group.m_sFactionKey, survivorInfantry, survivorVehicles);

		group.m_sRuntimeStatus = "folded";
	}

	bool TickExactPlayerSupportSettlements(
		HST_CampaignState state,
		HST_PhysicalWarService physicalWar = null,
		HST_ForceSpawnAdapterService forceSpawnAdapter = null)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!HasExactPlayerSupportAuthorityIdentity(request))
				continue;
			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
				continue;
			HST_ForceSpawnResultState batch = FindExactSupportSpawnBatch(state, request);
			if (request.m_bRecallRequested)
			{
				changed = TickRecalledExactPlayerSupport(state, physicalWar, forceSpawnAdapter, request, batch) || changed;
				continue;
			}
			if (!batch)
			{
				if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
					changed = SettleExactPlayerSupportFullRefund(state, request, "exact QRF cancelled because campaign phase is no longer active", true) || changed;
				continue;
			}
			if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL)
				changed = SettleExactPlayerSupportDeploymentTerminal(state, request, batch, "exact QRF deployment failed: " + batch.m_sTerminalReason, false) || changed;
			else if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED)
				changed = SettleExactPlayerSupportDeploymentTerminal(state, request, batch, "exact QRF deployment cancelled: " + batch.m_sTerminalReason, true) || changed;
			else if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			{
				HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
				if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
				{
					int terminalSurvivors = m_ExactForceSpawnQueue.CountDurableLivingMemberSlots(batch);
					if (group && group.m_bSpawnedEntity)
					{
						if (!RetireExactSupportProjectionRuntime(state, physicalWar, forceSpawnAdapter, group))
							continue;
						group.m_bSpawnedEntity = false;
						group.m_sRuntimeEntityId = "";
						group.m_sRuntimeStatus = "folded";
					}
					changed = SettleExactPlayerSupportHRRefund(state, request, terminalSurvivors, "successful exact QRF retired because campaign phase is no longer active", "terminal_phase_survivors_refunded") || changed;
					continue;
				}
				if (group && group.m_bSpawnCompleted && group.m_bEverPopulated
					&& group.m_iDurableLivingInfantryCount <= 0 && group.m_sRuntimeStatus == "eliminated")
					changed = SettleEliminatedExactPlayerSupport(state, request, group) || changed;
			}
		}
		return changed;
	}

	bool ReconcileSuccessfulExactPlayerSupportRuntimeAfterRestore(
		HST_CampaignState state,
		HST_PhysicalWarService physicalWar,
		HST_ForceSpawnAdapterService forceSpawnAdapter)
	{
		if (!state || !state.m_bRestoredFromPersistence || !physicalWar || !forceSpawnAdapter || !m_ExactForceSpawnQueue)
			return false;

		bool changed;
		bool initialRestorePass = !m_bSuccessfulExactPlayerSupportRestorePassComplete;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!HasExactPlayerSupportAuthorityIdentity(request))
				continue;
			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
				continue;
			if (!initialRestorePass && request.m_sRuntimeStatus != "exact_restore_waiting_queue_capacity")
				continue;
			HST_ForceSpawnResultState batch = FindExactSupportSpawnBatch(state, request);
			if (!batch || batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
				continue;

			HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
			bool localProjectionReady;
			if (group)
			{
				SCR_AIGroup root = physicalWar.GetForceSpawnGroupRoot(group);
				localProjectionReady = root && !root.IsDeleted();
				localProjectionReady = localProjectionReady && forceSpawnAdapter.CountHandlesForProjection(group.m_sProjectionId) > 0;
			}
			if (localProjectionReady)
				continue;

			int livingMembers = m_ExactForceSpawnQueue.CountDurableLivingMemberSlots(batch);
			if (livingMembers <= 0)
			{
				if (group)
				{
					group.m_bSpawnedEntity = false;
					group.m_sRuntimeEntityId = "";
					group.m_sRuntimeStatus = "eliminated";
					group.m_iDurableLivingInfantryCount = 0;
				}
				changed = SettleEliminatedExactPlayerSupport(state, request, group) || changed;
				continue;
			}
			if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			{
				if (group)
				{
					group.m_bSpawnedEntity = false;
					group.m_sRuntimeEntityId = "";
					group.m_sRuntimeStatus = "folded";
				}
				changed = SettleExactPlayerSupportHRRefund(state, request, livingMembers, "successful exact QRF retired because campaign phase is no longer active", "terminal_phase_survivors_refunded") || changed;
				continue;
			}
			if (!group)
			{
				changed = SettleExactPlayerSupportHRRefund(state, request, livingMembers, "successful exact QRF projection owner is missing after restore", "restore_owner_missing_survivors_refunded") || changed;
				continue;
			}

			int deadlineSecond = state.m_iElapsedSeconds + Math.Max(EXACT_PLAYER_SUPPORT_DEPLOYMENT_GRACE_SECONDS, request.m_iETASeconds);
			HST_ForceSpawnQueueCallbackResult requeue = m_ExactForceSpawnQueue.RequeueSuccessfulProjectionAfterRestore(
				state.m_aForceSpawnResults,
				state.FindForceManifest(request.m_sManifestId),
				batch.m_sResultId,
				batch.m_sProjectionId,
				state.m_iElapsedSeconds,
				deadlineSecond);
			if (!requeue || !requeue.m_bAccepted)
			{
				string failure = "successful exact QRF survivor reprojection was rejected";
				if (requeue && !requeue.m_sFailureReason.IsEmpty())
					failure = failure + ": " + requeue.m_sFailureReason;
				if (failure.Contains("queue capacity"))
				{
					group.m_bSpawnedEntity = false;
					group.m_sRuntimeEntityId = "";
					group.m_sRuntimeStatus = "exact_restore_waiting_queue_capacity";
					request.m_bPhysicalized = false;
					request.m_sRuntimeStatus = "exact_restore_waiting_queue_capacity";
					request.m_sFailureReason = failure;
					changed = true;
					continue;
				}
				changed = SettleExactPlayerSupportHRRefund(state, request, livingMembers, failure, "restore_reprojection_failed_survivors_refunded") || changed;
				continue;
			}

			group.m_bSpawnedEntity = false;
			group.m_sRuntimeEntityId = "";
			group.m_sRuntimeStatus = EXACT_PLAYER_SUPPORT_GROUP_STATUS;
			group.m_sSpawnFailureReason = "";
			group.m_iSpawnedAgentCount = 0;
			group.m_iLastSeenAliveCount = livingMembers;
			group.m_iSurvivorInfantryCount = livingMembers;
			group.m_iDurableLivingInfantryCount = livingMembers;
			group.m_iLifecycleRevision++;
			request.m_bPhysicalized = false;
			request.m_sRuntimeStatus = "exact_restore_survivor_reprojection_queued";
			request.m_sFailureReason = "";
			changed = true;
			m_bMarkerRefreshNeeded = true;
		}
		m_bSuccessfulExactPlayerSupportRestorePassComplete = true;
		return changed;
	}

	protected string ValidateAcceptedExactPlayerSupportRegistration(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		if (!state || !preset || !quote || !manifest || !m_ForceIntegrity)
			return "exact support registration authority is not ready";
		if (quote.m_sQuoteKind != HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF
			|| quote.m_eSupportType != HST_ESupportRequestType.HST_SUPPORT_QRF)
			return "quote is not an exact player QRF quote";
		if (quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED
			&& quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
			return "exact player QRF quote is not open or accepted";
		if (quote.m_sSupportRequestId.IsEmpty() || quote.m_sConfirmationRequestId.IsEmpty())
			return "exact player QRF support or confirmation identity is incomplete";
		if (quote.m_sFactionKey != preset.m_sResistanceFactionKey)
			return "exact player QRF faction does not match the resistance";
		string integrityFailure;
		bool requireCurrentCatalog = quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED;
		if (!m_ForceIntegrity.ValidateFrozenPlayerSupportQuote(manifest, quote, requireCurrentCatalog, integrityFailure))
			return "exact player QRF quote integrity conflict: " + integrityFailure;
		if (quote.m_sMoneyTransactionId.IsEmpty() || quote.m_sHRTransactionId.IsEmpty())
			return "exact player QRF transaction identity is incomplete";
		return "";
	}

	protected string ResolveAcceptedExactPlayerSupportContext(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_SupportRequestState request,
		out HST_ForceQuoteState quote,
		out HST_ForceManifestState manifest,
		bool requireCurrentCatalog)
	{
		quote = null;
		manifest = null;
		if (!state || !preset || !request || !m_ForceIntegrity)
			return "exact support execution authority is not ready";
		quote = state.FindForceQuote(request.m_sQuoteId);
		manifest = state.FindForceManifest(request.m_sManifestId);
		if (!quote || !manifest)
			return "accepted exact support quote or manifest is missing";
		if (quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
			return "exact support quote is not accepted";
		string linkFailure = ValidateExactPlayerSupportRequestLinks(request, quote, manifest, true);
		if (!linkFailure.IsEmpty())
			return linkFailure;
		string integrityFailure;
		if (!m_ForceIntegrity.ValidateFrozenPlayerSupportQuote(manifest, quote, requireCurrentCatalog, integrityFailure))
			return "accepted exact support integrity conflict: " + integrityFailure;
		if (quote.m_sFactionKey != preset.m_sResistanceFactionKey)
			return "accepted exact support faction conflict";
		return "";
	}

	protected string ValidateExactPlayerSupportRequestLinks(
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		bool requireAcceptedQuote)
	{
		if (!request || !quote || !manifest)
			return "exact support request, quote, or manifest is missing";
		if (requireAcceptedQuote && quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
			return "exact support quote is not accepted";
		if (request.m_sRequestId != quote.m_sSupportRequestId || request.m_sQuoteId != quote.m_sQuoteId)
			return "exact support request or quote identity conflict";
		if (request.m_sManifestId != quote.m_sManifestId || request.m_sManifestId != manifest.m_sManifestId)
			return "exact support manifest identity conflict";
		if (request.m_sOperationId != quote.m_sOperationId || request.m_sOperationId != manifest.m_sOperationId)
			return "exact support operation identity conflict";
		if (request.m_sCommandRequestId != quote.m_sConfirmationRequestId)
			return "exact support confirmation request identity conflict";
		if (request.m_sMoneyTransactionId != quote.m_sMoneyTransactionId || request.m_sHRTransactionId != quote.m_sHRTransactionId)
			return "exact support transaction identity conflict";
		if (request.m_sSpawnResultId != BuildExactSupportSpawnResultId(request.m_sRequestId))
			return "exact support spawn result identity conflict";
		if (request.m_sFactionKey != manifest.m_sFactionKey || request.m_sTargetZoneId != manifest.m_sTargetZoneId
			|| request.m_sSourceZoneId != manifest.m_sSourceZoneId)
			return "exact support faction or zone identity conflict";
		if (request.m_eType != HST_ESupportRequestType.HST_SUPPORT_QRF || !request.m_bPlayerRequested)
			return "exact support request type conflict";
		if (request.m_iMoneyCost != manifest.m_iMoneyCost || request.m_iHRCost != manifest.m_iHRCost
			|| request.m_iPlannedInfantryCount != manifest.m_iAcceptedMemberCount
			|| request.m_iCompositionCost != manifest.m_iMoneyCost
			|| request.m_iCompositionManpower != manifest.m_iAcceptedMemberCount)
			return "exact support request cost or member count conflict";
		return "";
	}

	protected HST_SupportRequestState FindExactPlayerSupportByQuote(HST_CampaignState state, string quoteId)
	{
		if (!state || quoteId.IsEmpty())
			return null;
		HST_SupportRequestState match;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || request.m_sQuoteId != quoteId)
				continue;
			if (match)
				return null;
			match = request;
		}
		return match;
	}

	protected bool HasExactPlayerSupportAuthorityIdentity(HST_SupportRequestState request)
	{
		if (!request || !request.m_bPlayerRequested || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_QRF)
			return false;
		return !request.m_sQuoteId.IsEmpty() || !request.m_sManifestId.IsEmpty()
			|| request.m_sPhysicalizationMode == EXACT_PLAYER_SUPPORT_MODE;
	}

	protected HST_ForceSpawnResultState FindExactSupportSpawnBatch(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request)
			return null;
		if (!request.m_sSpawnResultId.IsEmpty())
			return state.FindForceSpawnResult(request.m_sSpawnResultId);
		return state.FindForceSpawnResultByRequest(request.m_sRequestId);
	}

	protected HST_ActiveGroupState BuildExactSupportActiveGroup(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ForceManifestState manifest,
		vector sourcePosition,
		vector targetPosition,
		string resultId,
		string forceId,
		string projectionId)
	{
		string groupPrefab = ResolveExactManifestGroupPrefab(manifest);
		if (!state || !request || !manifest || groupPrefab.IsEmpty())
			return null;

		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = projectionId;
		group.m_sOperationId = request.m_sOperationId;
		group.m_sManifestId = manifest.m_sManifestId;
		group.m_sSpawnResultId = resultId;
		group.m_sForceId = forceId;
		group.m_sProjectionId = projectionId;
		group.m_sZoneId = request.m_sTargetZoneId;
		group.m_sFactionKey = request.m_sFactionKey;
		group.m_sSupportRequestId = request.m_sRequestId;
		group.m_sPrefab = groupPrefab;
		group.m_sCompositionRequestId = manifest.m_sManifestId;
		group.m_sCompositionIntentId = manifest.m_sIntentId;
		group.m_sCompositionTier = "exact";
		group.m_sCompositionSummary = request.m_sCompositionSummary;
		group.m_sSpawnFallbackMode = EXACT_PLAYER_SUPPORT_MODE;
		group.m_sRouteId = ResolveSupportDeploymentRouteId(state, request);
		group.m_vSourcePosition = sourcePosition;
		group.m_vTargetPosition = targetPosition;
		group.m_vPosition = sourcePosition;
		group.m_sRuntimeStatus = EXACT_PLAYER_SUPPORT_GROUP_STATUS;
		group.m_iInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iOriginalInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iCompositionCost = manifest.m_iMoneyCost;
		group.m_iCompositionManpower = manifest.m_iAcceptedMemberCount;
		group.m_iLastSeenAliveCount = manifest.m_iAcceptedMemberCount;
		group.m_iSurvivorInfantryCount = manifest.m_iAcceptedMemberCount;
		group.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		group.m_bQRF = true;
		return group;
	}

	protected string ValidateExactSupportActiveGroup(
		HST_ActiveGroupState group,
		HST_SupportRequestState request,
		HST_ForceManifestState manifest,
		string resultId,
		string forceId,
		string projectionId)
	{
		if (!group || !request || !manifest)
			return "exact support active-group context is missing";
		if (group.m_sGroupId != projectionId || group.m_sProjectionId != projectionId || group.m_sForceId != forceId)
			return "exact support active-group force or projection identity conflict";
		if (group.m_sSpawnResultId != resultId || group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sOperationId != manifest.m_sOperationId)
			return "exact support active-group result, manifest, or operation identity conflict";
		if (group.m_sSupportRequestId != request.m_sRequestId || group.m_sFactionKey != manifest.m_sFactionKey)
			return "exact support active-group request or faction identity conflict";
		if (group.m_sPrefab != ResolveExactManifestGroupPrefab(manifest)
			|| group.m_iInfantryCount != manifest.m_iAcceptedMemberCount || group.m_iVehicleCount != 0
			|| group.m_iCompositionCost != manifest.m_iMoneyCost || group.m_iCompositionManpower != manifest.m_iAcceptedMemberCount)
			return "exact support active-group prefab or force count conflict";
		return "";
	}

	protected string ResolveExactManifestGroupPrefab(HST_ForceManifestState manifest)
	{
		if (!manifest || manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return "";
		string prefab = manifest.m_aGroups[0].m_sPrefab;
		if (prefab.IsEmpty())
			prefab = manifest.m_sGroupPrefab;
		return prefab;
	}

	protected string BuildExactSupportSpawnResultId(string supportRequestId)
	{
		if (supportRequestId.IsEmpty())
			return "";
		return "spawn_" + supportRequestId;
	}

	protected string BuildExactSupportForceId(string operationId)
	{
		if (operationId.IsEmpty())
			return "";
		return "force_" + operationId;
	}

	protected string BuildExactSupportProjectionId(string operationId)
	{
		if (operationId.IsEmpty())
			return "";
		return "projection_" + operationId;
	}

	protected int ResolveExactSupportDeploymentDeadline(HST_SupportRequestState request)
	{
		if (!request)
			return 0;
		return request.m_iRequestedAtSecond + Math.Max(0, request.m_iETASeconds) + EXACT_PLAYER_SUPPORT_DEPLOYMENT_GRACE_SECONDS;
	}

	protected bool IsTerminalExactSupportBatch(HST_ForceSpawnResultState batch)
	{
		return batch && (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED);
	}

	protected bool IsRetryableExactSupportAdmissionFailure(string failure)
	{
		return failure.Contains("capacity") || failure.Contains("would exceed")
			|| failure.Contains("retention") || failure.Contains("not ready");
	}

	protected bool UpdateExactSupportQueueRuntimeStatus(HST_SupportRequestState request, HST_ForceSpawnResultState batch)
	{
		if (!request || !batch)
			return false;
		string status = "exact_spawn_pending";
		if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_IN_PROGRESS)
			status = "exact_spawn_in_progress";
		else if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_DEFERRED)
			status = "exact_spawn_deferred";
		else if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_RETRYABLE)
			status = "exact_spawn_retryable";
		else if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CLEANUP_PENDING)
			status = "exact_spawn_cleanup_pending";
		else if (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_READY_FOR_HANDOFF)
			status = "exact_spawn_ready_for_handoff";
		return SetExactSupportRuntimeStatus(request, status);
	}

	protected bool SetExactSupportRuntimeStatus(HST_SupportRequestState request, string status)
	{
		if (!request || request.m_sRuntimeStatus == status)
			return false;
		request.m_sRuntimeStatus = status;
		return true;
	}

	protected void ApplyExactSupportPlacementEvidence(HST_SupportRequestState request, HST_SpawnPlacementResult placement)
	{
		if (!request || !placement)
			return;
		request.m_sDeploymentPlacementType = placement.m_sPlacementType;
		request.m_sDeploymentSummary = placement.m_sDebugSummary;
		request.m_iDeploymentTargetDistanceMeters = Math.Round(placement.m_fTargetDistanceMeters);
		request.m_iDeploymentRoadDistanceMeters = Math.Round(placement.m_fRoadDistanceMeters);
		request.m_iDeploymentHQDistanceMeters = Math.Round(placement.m_fHQDistanceMeters);
		request.m_bDeploymentRoadResolved = placement.m_bRoadResolved;
		request.m_bDeploymentVehicleSafe = placement.m_bVehicleSafe;
		request.m_bDeploymentVehicleSafeRequired = false;
	}

	protected bool FailClosedExactPlayerSupport(HST_CampaignState state, HST_SupportRequestState request, HST_ForceSpawnResultState batch, string reason)
	{
		request.m_sFailureReason = reason;
		if (batch && !IsTerminalExactSupportBatch(batch))
		{
			if (!m_ExactForceSpawnQueue)
				return SetExactSupportRuntimeStatus(request, "exact_integrity_failure_queue_unavailable");
			HST_ForceSpawnQueueCallbackResult cancel = m_ExactForceSpawnQueue.RequestCancel(state.m_aForceSpawnResults, batch.m_sResultId, state.m_iElapsedSeconds, reason);
			return SetExactSupportRuntimeStatus(request, "exact_integrity_failure_cleanup_pending") || (cancel && cancel.m_bStateChanged);
		}
		if (batch && batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
			return SetExactSupportRuntimeStatus(request, "exact_integrity_failure_after_success");
		return SettleExactPlayerSupportDeploymentTerminal(state, request, batch, reason, false);
	}

	protected bool SettleExactPlayerSupportDeploymentTerminal(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ForceSpawnResultState batch,
		string reason,
		bool cancelled)
	{
		if (!batch || batch.m_iSuccessfulHandoffCount <= 0)
			return SettleExactPlayerSupportFullRefund(state, request, reason, cancelled);
		if (!m_ExactForceSpawnQueue)
			return SetExactSupportRuntimeStatus(request, "exact_terminal_settlement_queue_unavailable");
		int survivingInfantry = m_ExactForceSpawnQueue.CountDurableLivingMemberSlots(batch);
		string resolutionKind = "reprojection_failed_survivors_refunded";
		if (cancelled)
			resolutionKind = "reprojection_cancelled_survivors_refunded";
		return SettleExactPlayerSupportHRRefund(state, request, survivingInfantry, reason, resolutionKind);
	}

	protected bool SettleEliminatedExactPlayerSupport(
		HST_CampaignState state,
		HST_SupportRequestState request,
		HST_ActiveGroupState group)
	{
		if (!state || !request)
			return false;
		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
		request.m_sResolutionKind = "force_eliminated";
		request.m_sRuntimeStatus = "resolved_force_eliminated";
		request.m_sFailureReason = "all exact QRF roster members were confirmed dead";
		request.m_bPhysicalized = false;
		if (group)
			RemoveExactSupportActiveGroup(state, group.m_sGroupId);
		request.m_sGroupId = "";
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected bool SettleExactPlayerSupportFullRefund(HST_CampaignState state, HST_SupportRequestState request, string reason, bool cancelled)
	{
		if (!state || !request || !m_ExactResourceLedger || !m_ExactEconomy)
			return false;
		string settlementPrefix = "exact_support_failure";
		if (cancelled)
			settlementPrefix = "exact_support_cancel";
		bool moneySettled = SettleExactSupportTransaction(state, request, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, request.m_iMoneyCost, settlementPrefix + "_money_" + request.m_sRequestId, reason);
		bool hrSettled = SettleExactSupportTransaction(state, request, HST_ResourceLedgerService.RESOURCE_HR, request.m_iHRCost, settlementPrefix + "_hr_" + request.m_sRequestId, reason);
		if (!moneySettled || !hrSettled)
			return false;

		HST_ResourceTransactionState hrTransaction = state.FindResourceTransaction(request.m_sHRTransactionId);
		if (hrTransaction)
			request.m_iRefundedHR = Math.Min(request.m_iHRCost, Math.Max(0, hrTransaction.m_iRefundedAmount));
		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		if (cancelled)
			request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED;
		request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
		request.m_sFailureReason = reason;
		request.m_sResolutionKind = "exact_deployment_failed_refunded";
		request.m_sRuntimeStatus = "resolved_exact_deployment_failed_refunded";
		if (cancelled)
		{
			request.m_sResolutionKind = "exact_cancelled_refunded";
			request.m_sRuntimeStatus = "cancelled_exact_refunded";
		}
		request.m_bPhysicalized = false;
		HST_ActiveGroupState linkedGroup = state.FindActiveGroup(request.m_sGroupId);
		if (linkedGroup && !linkedGroup.m_bSpawnedEntity)
		{
			RemoveExactSupportActiveGroup(state, linkedGroup.m_sGroupId);
			request.m_sGroupId = "";
		}
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected bool SettleExactPlayerSupportHRRefund(HST_CampaignState state, HST_SupportRequestState request, int eligibleInfantry, string reason, string resolutionKind)
	{
		if (!state || !request || !m_ExactResourceLedger || !m_ExactEconomy)
			return false;
		int targetRefund = Math.Min(request.m_iHRCost, Math.Max(0, eligibleInfantry));
		if (!SettleExactSupportTransaction(state, request, HST_ResourceLedgerService.RESOURCE_HR, targetRefund, "exact_support_recall_hr_" + request.m_sRequestId, reason))
			return false;
		HST_ResourceTransactionState transaction = state.FindResourceTransaction(request.m_sHRTransactionId);
		if (transaction)
			request.m_iRefundedHR = Math.Min(request.m_iHRCost, Math.Max(0, transaction.m_iRefundedAmount));
		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
		request.m_sResolutionKind = resolutionKind;
		request.m_sRuntimeStatus = "resolved_" + resolutionKind;
		request.m_sFailureReason = reason;
		request.m_bPhysicalized = false;
		HST_ActiveGroupState linkedGroup = state.FindActiveGroup(request.m_sGroupId);
		if (linkedGroup && !linkedGroup.m_bSpawnedEntity)
		{
			RemoveExactSupportActiveGroup(state, linkedGroup.m_sGroupId);
			request.m_sGroupId = "";
		}
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected bool SettleExactSupportTransaction(
		HST_CampaignState state,
		HST_SupportRequestState request,
		string resourceType,
		int targetRefund,
		string settlementId,
		string reason)
	{
		string transactionId = request.m_sMoneyTransactionId;
		int transactionAmount = request.m_iMoneyCost;
		if (resourceType == HST_ResourceLedgerService.RESOURCE_HR)
		{
			transactionId = request.m_sHRTransactionId;
			transactionAmount = request.m_iHRCost;
		}
		HST_ResourceTransactionState transaction = state.FindResourceTransaction(transactionId);
		if (!ValidateExactSupportTransactionIdentity(transaction, request, resourceType, transactionAmount))
			return false;
		targetRefund = Math.Min(transactionAmount, Math.Max(0, targetRefund));
		if (transaction.m_iRefundedAmount >= targetRefund)
			return true;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED)
		{
			if (targetRefund != transactionAmount)
				return false;
			m_ExactResourceLedger.CancelReservation(state, m_ExactEconomy, transactionId, settlementId, reason);
		}
		else if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED
			|| transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED)
		{
			int remainingTarget = Math.Max(0, targetRefund - transaction.m_iRefundedAmount);
			m_ExactResourceLedger.RefundCommitted(state, m_ExactEconomy, transactionId, settlementId, remainingTarget, reason);
		}
		return transaction.m_iRefundedAmount >= targetRefund;
	}

	protected bool ValidateExactSupportTransactionIdentity(HST_ResourceTransactionState transaction, HST_SupportRequestState request, string resourceType, int amount)
	{
		if (!transaction || !request)
			return false;
		string expectedId = request.m_sMoneyTransactionId;
		if (resourceType == HST_ResourceLedgerService.RESOURCE_HR)
			expectedId = request.m_sHRTransactionId;
		return transaction.m_sTransactionId == expectedId
			&& transaction.m_sCommandRequestId == request.m_sCommandRequestId
			&& transaction.m_sOperationId == request.m_sOperationId
			&& transaction.m_sQuoteId == request.m_sQuoteId
			&& transaction.m_sManifestId == request.m_sManifestId
			&& transaction.m_sResourceType == resourceType
			&& transaction.m_iAmount == amount;
	}

	protected bool RetireExactSupportProjectionRuntime(
		HST_CampaignState state,
		HST_PhysicalWarService physicalWar,
		HST_ForceSpawnAdapterService forceSpawnAdapter,
		HST_ActiveGroupState group)
	{
		if (!state || !physicalWar || !forceSpawnAdapter || !group)
			return false;
		HST_ForceSpawnAdapterRetireResult retirement = forceSpawnAdapter.RetireProjectionRuntime(state, physicalWar, group.m_sProjectionId);
		if (retirement && retirement.m_bSuccess)
			return true;
		forceSpawnAdapter.PruneDeletedProjectionBindings(group.m_sProjectionId);
		return forceSpawnAdapter.CountHandlesForProjection(group.m_sProjectionId) == 0
			&& physicalWar.GetForceSpawnGroupRoot(group) == null;
	}

	protected void RemoveExactSupportActiveGroup(HST_CampaignState state, string groupId)
	{
		if (!state || groupId.IsEmpty())
			return;
		for (int index = state.m_aActiveGroups.Count() - 1; index >= 0; index--)
		{
			HST_ActiveGroupState group = state.m_aActiveGroups[index];
			if (group && group.m_sGroupId == groupId)
				state.m_aActiveGroups.Remove(index);
		}
	}

	string BuildSupportReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi support | state not ready";

		int queued;
		int active;
		int resolved;
		int cancelled;
		int physicalized;
		int abstractResolved;
		int helicopterStyle;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request)
				continue;

			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED)
				queued++;
			else if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				active++;
			else if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED)
				resolved++;
			else if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
				cancelled++;

			if (request.m_bPhysicalized)
				physicalized++;
			if (request.m_bAbstractResolved)
				abstractResolved++;
			if (request.m_bHelicopterStyle)
				helicopterStyle++;
		}

		string report = string.Format("h-istasi support | queued %1 | active %2 | resolved %3 | cancelled %4 | physical %5 | abstract %6 | helo-style %7", queued, active, resolved, cancelled, physicalized, abstractResolved, helicopterStyle);
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request)
				continue;

			string detail = "";
			if (!request.m_sStrikeConfigResource.IsEmpty())
				detail = " | strike " + request.m_sStrikeKind + " | " + request.m_sStrikeConfigResource;
			if (!request.m_sFailureReason.IsEmpty())
				detail = detail + " | fail " + request.m_sFailureReason;
			if (request.m_bOutcomeApplied)
				detail = detail + " | outcome applied";
			if (request.m_bRecallRequested)
				detail = detail + string.Format(" | recall at %1 exit %2 refund HR %3/%4", request.m_iRecallRequestedAtSecond, request.m_vRecallExitPosition, request.m_iRefundedHR, request.m_iHRCost);
			if (!request.m_sSelectedGarageVehicleId.IsEmpty())
				detail = detail + string.Format(" | garage vehicle %1 %2 consumed %3", request.m_sSelectedGarageVehicleId, request.m_sSelectedGarageVehicleDisplayName, request.m_bGarageVehicleConsumed);

			string compositionDetail = "";
			if (!request.m_sCompositionIntentId.IsEmpty() || request.m_iCompositionManpower > 0 || request.m_iCompositionVehicleCount > 0)
				compositionDetail = string.Format(" | composition %1 tier %2 cost %3 manpower %4 vehicles %5 armed %6", request.m_sCompositionIntentId, request.m_sCompositionTier, request.m_iCompositionCost, request.m_iCompositionManpower, request.m_iCompositionVehicleCount, request.m_iCompositionArmedVehicleCount);
			if (!request.m_sCompositionFailureReason.IsEmpty())
				compositionDetail = compositionDetail + " | composition failure " + request.m_sCompositionFailureReason;

			string line = string.Format(
				"\n%1 | %2 | %3 | runtime %4 | target %5 | eta %6 | requested %7 | active %8 | physical %9",
				request.m_sRequestId,
				request.m_eType,
				request.m_eStatus,
				request.m_sRuntimeStatus,
				request.m_sTargetZoneId,
				request.m_iETASeconds,
				request.m_iRequestedAtSecond,
				request.m_iActivatedAtSecond,
				request.m_iPhysicalizedAtSecond
			);
			line = line + string.Format(" | resolved %1 | asset %2 | group %3 | mode %4 | result %5", request.m_iResolvedAtSecond, request.m_sAssetProfileId, request.m_sGroupId, request.m_sPhysicalizationMode, request.m_sResolutionKind);
			line = line + string.Format(" | cost $%1 HR %2 planned FIA %3 refunded HR %4 | abstract %5 | physicalStrike %6%7%8", request.m_iMoneyCost, request.m_iHRCost, request.m_iPlannedInfantryCount, request.m_iRefundedHR, request.m_bAbstractResolved, request.m_bPhysicalStrikeSpawned, compositionDetail, detail);
			report = report + line;
		}

		return report;
	}

	string BuildSupportCooldownReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi support cooldowns | state not ready";

		string report = "h-istasi support cooldowns";
		int emitted;

		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || !request.m_bPlayerRequested)
				continue;

			int remaining = Math.Max(0, request.m_iCooldownUntilSecond - state.m_iElapsedSeconds);
			if (remaining <= 0 && request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_QUEUED && request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				continue;

			report = report + string.Format("\n%1 | %2 | status %3 | cooldown remaining %4s", request.m_sRequestId, request.m_eType, request.m_eStatus, remaining);
			emitted++;
		}

		if (emitted == 0)
			report = report + "\nnone";

		return report;
	}

	bool CancelSupportRequest(HST_CampaignState state, string requestId = "", bool playerRequestedOnly = true)
	{
		if (!state)
			return false;

		HST_SupportRequestState request = ResolveCancelableRequest(state, requestId, playerRequestedOnly);
		if (!request)
			return false;
		if (HasExactPlayerSupportAuthorityIdentity(request))
		{
			HST_ForceSpawnResultState batch = FindExactSupportSpawnBatch(state, request);
			if (batch && batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
				return false;
			if (batch && !IsTerminalExactSupportBatch(batch))
			{
				if (!m_ExactForceSpawnQueue)
					return false;
				HST_ForceSpawnQueueCallbackResult cancel = m_ExactForceSpawnQueue.RequestCancel(state.m_aForceSpawnResults, batch.m_sResultId, state.m_iElapsedSeconds, "exact QRF cancelled by commander");
				if (!cancel || !cancel.m_bAccepted)
					return false;
				request.m_sFailureReason = "cancelled by commander; exact cleanup pending";
				request.m_sRuntimeStatus = "exact_cancel_waiting_cleanup";
				m_bMarkerRefreshNeeded = true;
				return true;
			}
			if (batch && batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL)
				return SettleExactPlayerSupportDeploymentTerminal(state, request, batch, "exact QRF deployment failed before commander cancellation", false);
			return SettleExactPlayerSupportDeploymentTerminal(state, request, batch, "exact QRF cancelled by commander before deployment", true);
		}

		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED;
		request.m_sFailureReason = "cancelled by commander";
		request.m_sRuntimeStatus = "cancelled";
		request.m_sResolutionKind = "cancelled";
		request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	string RecallSupportRequestReport(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_PhysicalWarService physicalWar, string requestId, bool playerRequestedOnly = true)
	{
		if (!state || !preset)
			return "h-istasi support recall | failed: campaign state or preset not ready";

		HST_SupportRequestState request = ResolveRecallableRequest(state, requestId, playerRequestedOnly);
		if (!request)
			return "h-istasi support recall | failed: no recallable support team";

		if (request.m_bRecallRequested)
			return "h-istasi support recall | failed: recall already ordered for " + request.m_sRequestId;

		request.m_bRecallRequested = true;
		request.m_iRecallRequestedAtSecond = state.m_iElapsedSeconds;
		request.m_sRuntimeStatus = "recall_ordered";
		if (HasExactPlayerSupportAuthorityIdentity(request))
			return BeginExactPlayerSupportRecall(state, physicalWar, request);

		if (request.m_sGroupId.IsEmpty())
		{
			int refunded = ApplySupportRecallHRRefund(state, economy, request, Math.Max(0, request.m_iHRCost - request.m_iRefundedHR));
			request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
			request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
			request.m_sRuntimeStatus = "resolved_recalled_before_deploy";
			request.m_sResolutionKind = "recalled_before_deploy";
			m_bMarkerRefreshNeeded = true;
			return string.Format("h-istasi support recall | %1 recalled before deployment | refunded HR %2/%3", request.m_sRequestId, refunded, request.m_iHRCost);
		}

		HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
		if (!group)
		{
			request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
			request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
			request.m_sRuntimeStatus = "resolved_recalled_missing_group";
			request.m_sResolutionKind = "recalled_missing_group";
			request.m_sFailureReason = "recall group missing";
			m_bMarkerRefreshNeeded = true;
			return string.Format("h-istasi support recall | %1 resolved: group missing | refunded HR 0/%2", request.m_sRequestId, request.m_iHRCost);
		}

		if (group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "spawn_failed")
		{
			request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
			request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
			request.m_sRuntimeStatus = "resolved_recalled_group_lost";
			request.m_sResolutionKind = "recalled_group_lost";
			request.m_sFailureReason = "recall group terminal: " + group.m_sRuntimeStatus;
			m_bMarkerRefreshNeeded = true;
			return string.Format("h-istasi support recall | %1 could not recall %2 | group %3 | refunded HR 0/%4", request.m_sRequestId, group.m_sGroupId, group.m_sRuntimeStatus, request.m_iHRCost);
		}

		vector exitPosition = ResolveSupportRecallExitPosition(state, request, group);
		request.m_vRecallExitPosition = exitPosition;
		bool routed;
		if (physicalWar)
			routed = physicalWar.RecallActiveSupportGroup(state, group.m_sGroupId, exitPosition);
		if (!routed)
			routed = ApplySupportRecallRouteDataOnly(state, request, group, exitPosition);

		m_bMarkerRefreshNeeded = true;
		if (!routed)
		{
			request.m_bRecallRequested = false;
			request.m_iRecallRequestedAtSecond = 0;
			request.m_sRuntimeStatus = "active_recall_route_failed";
			request.m_sFailureReason = "recall route failed";
			return string.Format("h-istasi support recall | failed: could not route %1 out of area", request.m_sRequestId);
		}

		return string.Format("h-istasi support recall | ordered %1 | group %2 | survivors %3/%4 | exit %5 | HR refund pending", request.m_sRequestId, group.m_sGroupId, Math.Max(0, group.m_iSurvivorInfantryCount), request.m_iHRCost, exitPosition);
	}

	bool RecallSupportRequest(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_PhysicalWarService physicalWar, string requestId, bool playerRequestedOnly = true)
	{
		string report = RecallSupportRequestReport(state, preset, economy, physicalWar, requestId, playerRequestedOnly);
		return !report.Contains("failed");
	}

	protected string BeginExactPlayerSupportRecall(HST_CampaignState state, HST_PhysicalWarService physicalWar, HST_SupportRequestState request)
	{
		HST_ForceSpawnResultState batch = FindExactSupportSpawnBatch(state, request);
		if (batch && batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL)
		{
			bool failedSettled = SettleExactPlayerSupportDeploymentTerminal(state, request, batch, "exact QRF deployment failed before recall: " + batch.m_sTerminalReason, false);
			if (!failedSettled)
				return "h-istasi support recall | failed: exact deployment failure refund could not settle";
			if (batch.m_iSuccessfulHandoffCount > 0)
				return string.Format("h-istasi support recall | %1 reprojection failed after prior handoff | retained money | refunded survivor HR %2", request.m_sRequestId, request.m_iRefundedHR);
			return string.Format("h-istasi support recall | %1 deployment failed before recall | refunded $%2 and HR %3", request.m_sRequestId, request.m_iMoneyCost, request.m_iRefundedHR);
		}
		if (!batch || batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED)
		{
			if (batch && !IsTerminalExactSupportBatch(batch))
			{
				if (!m_ExactForceSpawnQueue)
				{
					request.m_bRecallRequested = false;
					request.m_iRecallRequestedAtSecond = 0;
					return "h-istasi support recall | failed: exact spawn queue not ready";
				}
				HST_ForceSpawnQueueCallbackResult cancel = m_ExactForceSpawnQueue.RequestCancel(state.m_aForceSpawnResults, batch.m_sResultId, state.m_iElapsedSeconds, "exact QRF recalled before deployment");
				if (!cancel || !cancel.m_bAccepted)
				{
					request.m_bRecallRequested = false;
					request.m_iRecallRequestedAtSecond = 0;
					return "h-istasi support recall | failed: exact predeployment cancellation was rejected";
				}
				request.m_sRuntimeStatus = "exact_recall_waiting_cleanup";
				m_bMarkerRefreshNeeded = true;
				return string.Format("h-istasi support recall | %1 cancellation ordered before deployment | HR refund pending exact cleanup", request.m_sRequestId);
			}

			int eligibleInfantry = request.m_iHRCost;
			if (batch && batch.m_iSuccessfulHandoffCount > 0)
			{
				if (!m_ExactForceSpawnQueue)
					return "h-istasi support recall | failed: exact settlement queue is unavailable";
				eligibleInfantry = m_ExactForceSpawnQueue.CountDurableLivingMemberSlots(batch);
			}
			bool settled = SettleExactPlayerSupportHRRefund(state, request, eligibleInfantry, "exact QRF recalled before deployment", "recalled_before_deploy");
			if (!settled)
				return "h-istasi support recall | failed: exact predeployment HR refund could not settle";
			return string.Format("h-istasi support recall | %1 recalled before deployment | retained money | refunded HR %2/%3", request.m_sRequestId, request.m_iRefundedHR, request.m_iHRCost);
		}

		HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
		if (!group)
		{
			request.m_sRuntimeStatus = "exact_recall_projection_missing";
			request.m_sFailureReason = "exact QRF recall projection is missing";
			return "h-istasi support recall | failed: exact deployed group is missing";
		}
		if (group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "spawn_failed")
		{
			if (!m_ExactForceSpawnQueue)
				return "h-istasi support recall | failed: exact settlement queue is unavailable";
			int lostSurvivors = m_ExactForceSpawnQueue.CountDurableLivingMemberSlots(batch);
			SettleExactPlayerSupportHRRefund(state, request, lostSurvivors, "exact QRF recall group was lost", "recalled_group_lost");
			return string.Format("h-istasi support recall | %1 could not recall %2 | group %3 | refunded HR %4/%5", request.m_sRequestId, group.m_sGroupId, group.m_sRuntimeStatus, request.m_iRefundedHR, request.m_iHRCost);
		}

		vector exitPosition = ResolveSupportRecallExitPosition(state, request, group);
		request.m_vRecallExitPosition = exitPosition;
		bool routed;
		if (physicalWar)
			routed = physicalWar.RecallActiveSupportGroup(state, group.m_sGroupId, exitPosition);
		if (!routed)
			routed = ApplySupportRecallRouteDataOnly(state, request, group, exitPosition);
		if (!routed)
		{
			request.m_bRecallRequested = false;
			request.m_iRecallRequestedAtSecond = 0;
			request.m_sRuntimeStatus = "exact_recall_route_failed";
			request.m_sFailureReason = "exact recall route failed";
			return string.Format("h-istasi support recall | failed: could not route exact QRF %1 out of area", request.m_sRequestId);
		}

		m_bMarkerRefreshNeeded = true;
		int durableSurvivors = Math.Max(0, group.m_iSurvivorInfantryCount);
		if (m_ExactForceSpawnQueue)
			durableSurvivors = m_ExactForceSpawnQueue.CountDurableLivingMemberSlots(batch);
		return string.Format("h-istasi support recall | ordered %1 | group %2 | survivors %3/%4 | exit %5 | ledger HR refund pending", request.m_sRequestId, group.m_sGroupId, durableSurvivors, request.m_iHRCost, exitPosition);
	}

	protected bool ApplyActiveSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request, bool arrivedAtTarget = false)
	{
		if (!state || !request || request.m_bHelicopterStyle)
			return false;
		if (HasExactPlayerSupportAuthorityIdentity(request))
			return false;

		if (IsStrikeSupport(request.m_eType))
		{
			ActivateStrikeSupport(request);
			request.m_bPhysicalized = true;
			request.m_iPhysicalizedAtSecond = state.m_iElapsedSeconds;
			request.m_sPhysicalizationMode = "abstract_strike";
			request.m_sRuntimeStatus = "abstract_strike_active";
			m_bMarkerRefreshNeeded = true;
			return true;
		}

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_EVACUATION)
			return false;

		if (!request.m_sGroupId.IsEmpty())
			return false;

		if (!m_ForceCompositions)
			m_ForceCompositions = new HST_ForceCompositionService();

		HST_ForceRequest forceRequest = m_ForceCompositions.BuildSupportForceRequest(state, preset, request);
		HST_ForceCompositionResult composition = m_ForceCompositions.Compose(state, preset, forceRequest);
		m_ForceCompositions.ApplyCompositionToSupportRequest(request, composition);
		HST_GroupSpawnPlan groupPlan;
		if (composition)
			groupPlan = composition.GetPrimaryGroup();

		if (!composition || !composition.m_bSuccess || !groupPlan)
		{
			request.m_sFailureReason = "force composition failed";
			if (composition && !composition.m_sFailureReason.IsEmpty())
				request.m_sFailureReason = composition.m_sFailureReason;
			request.m_sRuntimeStatus = "physicalize_failed_composition";
			request.m_sPhysicalizationMode = "ground_group_blocked";
			return false;
		}

		string prefab = groupPlan.m_sPrefab;
		bool roadblockSupport = request.m_eType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK;
		if (roadblockSupport)
		{
			composition.m_iVehicleCount = Math.Max(1, composition.m_iVehicleCount);
			request.m_iCompositionVehicleCount = Math.Max(1, request.m_iCompositionVehicleCount);
		}

		if (!m_SpawnPlacements)
			m_SpawnPlacements = new HST_SpawnPlacementService();

		bool requireVehicleSafe = composition.m_iVehicleCount > 0;
		HST_SpawnPlacementRequest placementRequest;
		if (roadblockSupport)
			placementRequest = m_SpawnPlacements.BuildRoadblockPlacementRequest(state, preset, request);
		else
			placementRequest = m_SpawnPlacements.BuildSupportPlacementRequest(state, preset, request, arrivedAtTarget, requireVehicleSafe);
		HST_SpawnPlacementResult placement = m_SpawnPlacements.ResolvePlacement(state, preset, placementRequest);
		bool vehicleSafeDowngradeAllowed = !roadblockSupport && requireVehicleSafe;
		vehicleSafeDowngradeAllowed = vehicleSafeDowngradeAllowed && placement;
		vehicleSafeDowngradeAllowed = vehicleSafeDowngradeAllowed && !placement.m_bSuccess;
		vehicleSafeDowngradeAllowed = vehicleSafeDowngradeAllowed && placement.m_sFailureReason.Contains("vehicle-safe");
		if (vehicleSafeDowngradeAllowed)
		{
			requireVehicleSafe = false;
			composition.m_iVehicleCount = 0;
			composition.m_iArmedVehicleCount = 0;
			request.m_iCompositionVehicleCount = 0;
			request.m_iCompositionArmedVehicleCount = 0;
			string vehicleDowngradeSummary = "vehicle-safe staging unavailable; deployed infantry-only";
			if (!composition.m_sDebugSummary.IsEmpty())
				composition.m_sDebugSummary = composition.m_sDebugSummary + " | " + vehicleDowngradeSummary;
			else
				composition.m_sDebugSummary = vehicleDowngradeSummary;
			request.m_sCompositionSummary = composition.m_sDebugSummary;
			HST_SpawnPlacementRequest infantryPlacementRequest = m_SpawnPlacements.BuildSupportPlacementRequest(
				state,
				preset,
				request,
				arrivedAtTarget,
				false
			);
			placement = m_SpawnPlacements.ResolvePlacement(state, preset, infantryPlacementRequest);
			if (placement && placement.m_bSuccess)
				placement.m_sDebugSummary = placement.m_sDebugSummary + " | " + vehicleDowngradeSummary;
		}
		if (!placement || !placement.m_bSuccess)
		{
			request.m_sFailureReason = "spawn placement failed";
			if (placement && !placement.m_sFailureReason.IsEmpty())
			{
				request.m_sFailureReason = placement.m_sFailureReason;
				request.m_sDeploymentPlacementType = placement.m_sPlacementType;
				request.m_sDeploymentSummary = placement.m_sDebugSummary;
				request.m_bDeploymentVehicleSafeRequired = requireVehicleSafe;
			}
			request.m_sRuntimeStatus = "physicalize_failed_placement";
			request.m_sPhysicalizationMode = "ground_group_blocked";
			return false;
		}

		string routeId = ResolveSupportDeploymentRouteId(state, request);

		request.m_sDeploymentRouteId = routeId;
		request.m_sDeploymentPlacementType = placement.m_sPlacementType;
		request.m_sDeploymentSummary = placement.m_sDebugSummary;
		request.m_iDeploymentTargetDistanceMeters = Math.Round(placement.m_fTargetDistanceMeters);
		request.m_iDeploymentRoadDistanceMeters = Math.Round(placement.m_fRoadDistanceMeters);
		request.m_iDeploymentHQDistanceMeters = Math.Round(placement.m_fHQDistanceMeters);
		request.m_bDeploymentRoadResolved = placement.m_bRoadResolved;
		request.m_bDeploymentVehicleSafe = placement.m_bVehicleSafe;
		request.m_bDeploymentVehicleSafeRequired = requireVehicleSafe;

		vector objectivePosition = placement.m_vTargetPosition;
		vector targetPosition = objectivePosition;
		vector sourcePosition;
		string phase = "staged";
		if (roadblockSupport)
		{
			sourcePosition = placement.m_vSpawnPosition;
			targetPosition = sourcePosition;
			phase = "established";
		}
		else if (arrivedAtTarget)
		{
			sourcePosition = placement.m_vSpawnPosition;
			targetPosition = sourcePosition;
			phase = "arrived";
		}
		else
		{
			sourcePosition = placement.m_vSpawnPosition;
			targetPosition = objectivePosition;
		}

		if (IsPetrosAttackSupport(request) && IsInsidePetrosAttackStandoff(state, sourcePosition))
		{
			request.m_sFailureReason = "Petros attack staging inside HQ exclusion radius";
			request.m_sRuntimeStatus = "physicalize_failed_hq_standoff";
			request.m_sPhysicalizationMode = "ground_group_blocked";
			return false;
		}

		HST_ActiveGroupState group = new HST_ActiveGroupState();
		group.m_sGroupId = string.Format("support_%1", request.m_sRequestId);
		group.m_sOperationId = request.m_sOperationId;
		group.m_sZoneId = request.m_sTargetZoneId;
		group.m_sFactionKey = request.m_sFactionKey;
		group.m_sSupportRequestId = request.m_sRequestId;
		group.m_sPrefab = prefab;
		m_ForceCompositions.ApplyCompositionToActiveGroup(group, composition);
		group.m_sSpawnFallbackMode = "support";
		if (IsPetrosAttackSupport(request))
			group.m_sSpawnFallbackMode = "petros_attack_support";
		if (roadblockSupport)
			group.m_sSpawnFallbackMode = "roadblock_support";
		group.m_sRouteId = request.m_sDeploymentRouteId;
		group.m_vSourcePosition = sourcePosition;
		group.m_vTargetPosition = targetPosition;
		group.m_vPosition = sourcePosition;
		group.m_sRuntimeStatus = "support_arrived";
		if (!arrivedAtTarget)
			group.m_sRuntimeStatus = "support_active";
		if (roadblockSupport)
			group.m_sRuntimeStatus = "roadblock_established";
		group.m_iInfantryCount = Math.Max(1, groupPlan.m_iManpower);
		group.m_iVehicleCount = Math.Max(0, composition.m_iVehicleCount);
		if (roadblockSupport)
			group.m_iVehicleCount = Math.Max(1, group.m_iVehicleCount);
		group.m_iOriginalInfantryCount = group.m_iInfantryCount;
		group.m_iOriginalVehicleCount = group.m_iVehicleCount;
		group.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		group.m_iLastSeenAliveCount = 0;
		group.m_iSurvivorInfantryCount = group.m_iInfantryCount;
		group.m_iSurvivorVehicleCount = group.m_iVehicleCount;
		group.m_bQRF = request.m_eType == HST_ESupportRequestType.HST_SUPPORT_QRF;
		if (roadblockSupport && !request.m_sSelectedGarageVehiclePrefab.IsEmpty())
			group.m_sVehiclePrefab = request.m_sSelectedGarageVehiclePrefab;
		state.m_aActiveGroups.Insert(group);

		request.m_sGroupId = group.m_sGroupId;
		request.m_bPhysicalized = true;
		request.m_iPhysicalizedAtSecond = state.m_iElapsedSeconds;
		request.m_sPhysicalizationMode = "ground_group";
		request.m_sRuntimeStatus = "physical_group_linked";
		if (roadblockSupport)
		{
			request.m_sPhysicalizationMode = "roadblock_group";
			request.m_sRuntimeStatus = "roadblock_established";
			ApplyRoadblockOutcome(state, request);
			request.m_bOutcomeApplied = true;
			request.m_sResolutionKind = "physical_roadblock_established";
		}
		m_bMarkerRefreshNeeded = true;
		Print(string.Format("h-istasi | physical support %1 %2 near %3 | spawn %4 | objective %5 | group %6 | prefab %7", request.m_sRequestId, phase, request.m_sTargetZoneId, group.m_vPosition, objectivePosition, group.m_sGroupId, group.m_sPrefab));
		return true;
	}

	protected string ResolveSupportDeploymentRouteId(HST_CampaignState state, HST_SupportRequestState request)
	{
		string fallbackRouteId;
		if (request)
		{
			fallbackRouteId = request.m_sSourceZoneId + "_to_" + request.m_sTargetZoneId;
			if (request.m_sSourceZoneId.IsEmpty() || request.m_sTargetZoneId.IsEmpty())
				fallbackRouteId = "support_" + request.m_sRequestId;
		}

		if (!state || !request)
			return fallbackRouteId;

		HST_ZoneState targetZone = state.FindZone(request.m_sTargetZoneId);
		HST_GeneratedRouteState route = ResolveGeneratedSupportRouteForZone(state, targetZone);
		if (route)
			return route.m_sRouteId;

		return fallbackRouteId;
	}

	protected HST_GeneratedRouteState ResolveGeneratedSupportRouteForZone(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return null;

		if (!zone.m_sQRFRouteId.IsEmpty())
		{
			HST_GeneratedRouteState qrfRoute = state.FindGeneratedRoute(zone.m_sQRFRouteId);
			if (qrfRoute)
				return qrfRoute;
		}

		if (!zone.m_sPatrolRouteId.IsEmpty())
		{
			HST_GeneratedRouteState patrolRoute = state.FindGeneratedRoute(zone.m_sPatrolRouteId);
			if (patrolRoute)
				return patrolRoute;
		}

		string generatedRouteId = "route_" + zone.m_sZoneId + "_alpha";
		return state.FindGeneratedRoute(generatedRouteId);
	}

	protected void MarkPhysicalSupportArrived(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request || request.m_sGroupId.IsEmpty())
			return;

		HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
		if (group && group.m_sRuntimeStatus == "support_active")
			group.m_sRuntimeStatus = "support_arrived";
	}

	protected bool ResolveSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_SupportRequestState request, HST_StrategicService strategic = null, HST_HQService hq = null)
	{
		if (!state || !preset || !request || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED)
			return false;

		if (!request.m_bOutcomeApplied)
		{
			ApplySupportOutcome(state, preset, garrisons, request);
			ApplySupportNearHQStrategicEvent(state, preset, strategic, hq, request);
		}

		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
		request.m_bAbstractResolved = request.m_sGroupId.IsEmpty() || request.m_sResolutionKind.Contains("abstract");
		if (request.m_sRuntimeStatus.IsEmpty() || request.m_sRuntimeStatus == "active_waiting_eta" || request.m_sRuntimeStatus == "active_waiting_abstract_eta" || request.m_sRuntimeStatus == "abstract_strike_active" || request.m_sRuntimeStatus.Contains("physicalize_failed"))
			request.m_sRuntimeStatus = "resolved_" + request.m_sResolutionKind;

		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected void ApplySupportOutcome(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_SupportRequestState request)
	{
		if (!state || !preset || !request || request.m_bOutcomeApplied)
			return;

		if (request.m_sPhysicalizationMode.IsEmpty() || request.m_sPhysicalizationMode == "none")
			request.m_sPhysicalizationMode = ResolveAbstractPhysicalizationMode(request);

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
		{
			ApplySupplyOutcome(state, preset, request);
			request.m_bOutcomeApplied = true;
			return;
		}

		bool qrfPressureSupport = request.m_eType == HST_ESupportRequestType.HST_SUPPORT_QRF;
		qrfPressureSupport = qrfPressureSupport || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
		if (qrfPressureSupport)
		{
			ApplyQRFOutcome(state, preset, garrisons, request);
			request.m_bOutcomeApplied = true;
			return;
		}

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
		{
			ApplyRoadblockOutcome(state, request);
			request.m_bOutcomeApplied = true;
			return;
		}

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE)
		{
			ApplySuppressiveFireOutcome(state, preset, request);
			request.m_bOutcomeApplied = true;
			return;
		}

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING)
		{
			if (garrisons)
				garrisons.AddAbstractForces(state, request.m_sTargetZoneId, request.m_sFactionKey, 3 + state.m_iWarLevel, 0);
			request.m_sResolutionKind = "abstract_troop_landing";
			request.m_bOutcomeApplied = true;
			return;
		}

		if (IsStrikeSupport(request.m_eType))
		{
			ResolveStrikeSupport(state, preset, request);
			request.m_sResolutionKind = "abstract_strike";
			request.m_bOutcomeApplied = true;
			return;
		}

		request.m_sResolutionKind = "abstract_no_effect";
		request.m_bOutcomeApplied = true;
	}

	protected void ApplyRoadblockOutcome(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request)
			return;

		HST_CivilianZoneState civilianZone = state.FindCivilianZone(request.m_sTargetZoneId);
		if (!civilianZone)
		{
			civilianZone = new HST_CivilianZoneState();
			civilianZone.m_sZoneId = request.m_sTargetZoneId;
			state.m_aCivilianZones.Insert(civilianZone);
		}

		civilianZone.m_iRoadblockPresence = Math.Min(3, civilianZone.m_iRoadblockPresence + 1);
		if (!request.m_sGroupId.IsEmpty())
			request.m_sResolutionKind = "physical_roadblock";
		else
			request.m_sResolutionKind = "abstract_roadblock";
	}

	protected bool ApplySupportNearHQStrategicEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_HQService hq, HST_SupportRequestState request)
	{
		if (!state || !preset || !strategic || !hq || !request)
			return false;
		if (request.m_sFactionKey == preset.m_sResistanceFactionKey)
			return false;
		if (!IsSupportRequestNearHQ(state, request))
			return false;

		HST_StrategicEventApplyResult result = strategic.BeginSupportNearHQEvent(state, preset, request);
		bool changed = hq.AddHQKnowledge(state, SUPPORT_NEAR_HQ_KNOWLEDGE_GAIN, "hostile support near HQ: " + request.m_sRequestId);
		if (result && result.m_Event)
		{
			result.m_Event.m_sReason = string.Format("hostile support near HQ: %1 | %2", request.m_sRequestId, request.m_sResolutionKind);
			strategic.CompleteStrategicEvent(state, result, true, changed);
			return result.m_bChanged || changed;
		}

		return changed;
	}

	protected bool IsSupportRequestNearHQ(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request)
			return false;

		vector targetPosition = ResolveSupportNearHQPosition(state, request);
		if (IsZeroVector(targetPosition))
			return false;

		return DistanceSq2D(state.m_vHQPosition, targetPosition) <= SUPPORT_NEAR_HQ_STRATEGIC_RADIUS_METERS * SUPPORT_NEAR_HQ_STRATEGIC_RADIUS_METERS;
	}

	protected vector ResolveSupportNearHQPosition(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request)
			return "0 0 0";

		if (!IsZeroVector(request.m_vTargetPosition))
			return request.m_vTargetPosition;

		HST_ZoneState targetZone = state.FindZone(request.m_sTargetZoneId);
		if (targetZone)
			return targetZone.m_vPosition;

		HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
		if (group)
			return group.m_vTargetPosition;

		return "0 0 0";
	}

	protected string ResolveAbstractPhysicalizationMode(HST_SupportRequestState request)
	{
		if (!request)
			return "none";

		if (IsStrikeSupport(request.m_eType))
			return "abstract_strike";
		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
			return "supply_abstract";
		if (request.m_bHelicopterStyle)
			return "helicopter_abstract";
		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
			return "roadblock_abstract";
		if (IsPhysicalGroundSupport(request))
			return "ground_abstract";

		return "none";
	}

	protected void ApplySupplyOutcome(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request)
	{
		if (request.m_sFactionKey == preset.m_sResistanceFactionKey)
		{
			state.m_iFactionMoney += 75;
			state.m_iHR += 1;
			request.m_sResolutionKind = "abstract_fia_supply";
			return;
		}

		HST_GarrisonState enemyGarrison = state.FindGarrison(request.m_sTargetZoneId, request.m_sFactionKey);
		if (!enemyGarrison)
		{
			enemyGarrison = new HST_GarrisonState();
			enemyGarrison.m_sGarrisonId = HST_StableIdService.BuildGarrisonId(request.m_sTargetZoneId, request.m_sFactionKey);
			enemyGarrison.m_sZoneId = request.m_sTargetZoneId;
			enemyGarrison.m_sFactionKey = request.m_sFactionKey;
			state.m_aGarrisons.Insert(enemyGarrison);
		}

		enemyGarrison.m_iInfantryCount += 1;
		ClampGarrisonToZoneCapacity(state, enemyGarrison);
		request.m_sResolutionKind = "abstract_enemy_supply";
	}

	protected void ApplyQRFOutcome(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_SupportRequestState request)
	{
		if (!state || !preset || !request)
			return;

		if (request.m_sFactionKey == preset.m_sResistanceFactionKey)
		{
			request.m_sResolutionKind = "abstract_fia_qrf_pressure";
			return;
		}

		request.m_sResolutionKind = "abstract_enemy_qrf_pressure";
	}

	protected void ClampGarrisonToZoneCapacity(HST_CampaignState state, HST_GarrisonState garrison)
	{
		if (!state || !garrison)
			return;

		HST_ZoneState zone = state.FindZone(garrison.m_sZoneId);
		if (!zone || zone.m_iGarrisonSlots <= 0)
			return;

		garrison.m_iInfantryCount = Math.Min(zone.m_iGarrisonSlots, Math.Max(0, garrison.m_iInfantryCount));
	}

	protected void ApplySuppressiveFireOutcome(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request)
	{
		HST_ZoneState targetZone = state.FindZone(request.m_sTargetZoneId);
		if (!targetZone)
		{
			request.m_sResolutionKind = "abstract_suppressive_fire_missing_zone";
			return;
		}

		if (request.m_sFactionKey == preset.m_sResistanceFactionKey)
		{
			if (targetZone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				targetZone.m_iResistanceCaptureProgress = Math.Min(HST_ZoneCaptureService.CAPTURE_PROGRESS_REQUIRED - 1, targetZone.m_iResistanceCaptureProgress + 15);

			request.m_sResolutionKind = "abstract_fia_suppressive_fire";
			return;
		}

		if (targetZone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			targetZone.m_iSupport = Math.Max(-100, targetZone.m_iSupport - 6);
		else
			targetZone.m_iResistanceCaptureProgress = Math.Max(0, targetZone.m_iResistanceCaptureProgress - 12);

		request.m_sResolutionKind = "abstract_enemy_suppressive_fire";
	}

	protected bool ResolveSupportAsPhysicalComplete(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request)
			return false;

		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED;
		request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
		request.m_sResolutionKind = "physical_group_terminal";
		request.m_sRuntimeStatus = "resolved_physical_group_terminal";
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected void ResolveCosts(HST_ESupportRequestType supportType, out int attackCost, out int supportCost)
	{
		attackCost = 8;
		supportCost = 4;

		bool qrfStyleCost = supportType == HST_ESupportRequestType.HST_SUPPORT_QRF;
		qrfStyleCost = qrfStyleCost || supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
		if (qrfStyleCost)
		{
			attackCost = 15;
			supportCost = 5;
			return;
		}

		bool highPressureCost = supportType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING;
		highPressureCost = highPressureCost || supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE;
		if (highPressureCost)
		{
			attackCost = 20;
			supportCost = 8;
			return;
		}

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
		{
			attackCost = 8;
			supportCost = 6;
			return;
		}

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
		{
			attackCost = 0;
			supportCost = 10;
			return;
		}

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
		{
			attackCost = 24;
			supportCost = 16;
			return;
		}

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
		{
			attackCost = 36;
			supportCost = 28;
		}
	}

	protected int ResolveETA(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return 90;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return 150;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
			return 45;

		if (IsHelicopterStyle(supportType))
			return HELICOPTER_STYLE_ETA_SECONDS;

		return DEFAULT_ETA_SECONDS;
	}

	protected int ResolvePlayerMoneyCost(HST_ESupportRequestType supportType)
	{
		return GetPlayerMoneyCost(supportType);
	}

	static int GetPlayerMoneyCost(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return PLAYER_QRF_COST;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE || supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return PLAYER_FIRE_COST;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return PLAYER_AIRSTRIKE_COST;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return PLAYER_CRUISE_MISSILE_COST;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
			return PLAYER_ROADBLOCK_COST;

		return PLAYER_SUPPLY_COST;
	}

	static int EstimatePlayerSupportHRCost(HST_ESupportRequestType supportType, int warLevel)
	{
		warLevel = Math.Max(1, warLevel);
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return Math.Max(3, Math.Min(12 + warLevel, 4 + warLevel));
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return Math.Max(3, Math.Min(12 + warLevel, 3 + warLevel));
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
			return Math.Max(2, Math.Min(6, 2 + warLevel / 2));

		return 0;
	}

	protected int ResolvePlayerHRCost(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request, out string failureReason)
	{
		failureReason = "";
		if (!state || !preset || !request)
			return 0;
		if (!IsPhysicalGroundSupport(request))
			return 0;

		if (!m_ForceCompositions)
			m_ForceCompositions = new HST_ForceCompositionService();

		HST_ForceRequest forceRequest = m_ForceCompositions.BuildSupportForceRequest(state, preset, request);
		HST_ForceCompositionResult composition = m_ForceCompositions.Compose(state, preset, forceRequest);
		m_ForceCompositions.ApplyCompositionToSupportRequest(request, composition);
		HST_GroupSpawnPlan groupPlan;
		if (composition)
			groupPlan = composition.GetPrimaryGroup();

		if (!composition || !composition.m_bSuccess || !groupPlan)
		{
			failureReason = "force composition failed for support HR cost";
			if (composition && !composition.m_sFailureReason.IsEmpty())
				failureReason = composition.m_sFailureReason;
			return 0;
		}

		request.m_iPlannedInfantryCount = Math.Max(1, groupPlan.m_iManpower);
		return request.m_iPlannedInfantryCount;
	}

	protected bool HasActivePlayerRequestDetailed(HST_CampaignState state, HST_ESupportRequestType supportType, out string reason)
	{
		reason = "";
		if (!state)
		{
			reason = "campaign state not ready";
			return true;
		}

		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || !request.m_bPlayerRequested || request.m_eType != supportType)
				continue;

			if (state.m_iElapsedSeconds < request.m_iCooldownUntilSecond)
			{
				reason = string.Format("support type %1 cooldown active for %2s", supportType, request.m_iCooldownUntilSecond - state.m_iElapsedSeconds);
				return true;
			}
		}

		return false;
	}

	protected bool HasActivePlayerRequest(HST_CampaignState state, HST_ESupportRequestType supportType)
	{
		string reason;
		return HasActivePlayerRequestDetailed(state, supportType, reason);
	}

	protected HST_SupportRequestState ResolveCancelableRequest(HST_CampaignState state, string requestId, bool playerRequestedOnly)
	{
		HST_SupportRequestState fallback;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request)
				continue;

			if (playerRequestedOnly && !request.m_bPlayerRequested)
				continue;

			if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_QUEUED && request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				continue;

			if (!requestId.IsEmpty() && request.m_sRequestId == requestId)
				return request;

			if (requestId.IsEmpty() && !fallback)
				fallback = request;
		}

		return fallback;
	}

	protected HST_SupportRequestState ResolveRecallableRequest(HST_CampaignState state, string requestId, bool playerRequestedOnly)
	{
		if (!state)
			return null;

		HST_SupportRequestState fallback;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request)
				continue;
			if (playerRequestedOnly && !request.m_bPlayerRequested)
				continue;
			if (!IsRecallableSupportRequest(state, request))
				continue;

			if (!requestId.IsEmpty() && request.m_sRequestId == requestId)
				return request;

			if (requestId.IsEmpty() && !fallback)
				fallback = request;
		}

		return fallback;
	}

	protected bool IsRecallableSupportRequest(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request || request.m_bRecallRequested)
			return false;
		if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_QUEUED && request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
			return false;
		if (request.m_iHRCost <= 0 && request.m_sGroupId.IsEmpty())
			return false;
		if (!IsPhysicalGroundSupport(request))
			return false;

		return true;
	}

	protected bool ApplySupportRecallRouteDataOnly(HST_CampaignState state, HST_SupportRequestState request, HST_ActiveGroupState group, vector exitPosition)
	{
		if (!state || !request || !group || IsZeroVector(exitPosition))
			return false;

		group.m_vSourcePosition = group.m_vPosition;
		if (IsZeroVector(group.m_vSourcePosition))
			group.m_vSourcePosition = request.m_vTargetPosition;
		group.m_vTargetPosition = exitPosition;
		group.m_sRouteId = "";
		group.m_sRuntimeStatus = "support_recalling";
		group.m_sSpawnFallbackMode = "support_recall";
		group.m_iAssignedWaypointCount = 0;
		group.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		group.m_sSpawnFailureReason = "Support recall ordered; moving to exit point.";
		request.m_sRuntimeStatus = "recall_routing";
		return true;
	}

	protected vector ResolveSupportRecallExitPosition(HST_CampaignState state, HST_SupportRequestState request, HST_ActiveGroupState group)
	{
		vector currentPosition = group.m_vPosition;
		if (IsZeroVector(currentPosition))
			currentPosition = request.m_vTargetPosition;
		if (IsZeroVector(currentPosition))
			currentPosition = request.m_vSourcePosition;

		vector sourcePosition = group.m_vSourcePosition;
		if (IsZeroVector(sourcePosition))
			sourcePosition = request.m_vSourcePosition;

		vector targetPosition = request.m_vTargetPosition;
		if (IsZeroVector(targetPosition))
			targetPosition = currentPosition;

		float dx = sourcePosition[0] - targetPosition[0];
		float dz = sourcePosition[2] - targetPosition[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length <= 1.0)
		{
			dx = currentPosition[0] - targetPosition[0];
			dz = currentPosition[2] - targetPosition[2];
			length = Math.Sqrt(dx * dx + dz * dz);
		}
		if (length <= 1.0)
		{
			dx = 1.0;
			dz = 0.0;
			length = 1.0;
		}

		vector exitPosition = currentPosition;
		exitPosition[0] = currentPosition[0] + dx / length * SUPPORT_RECALL_EXIT_DISTANCE_METERS;
		exitPosition[2] = currentPosition[2] + dz / length * SUPPORT_RECALL_EXIT_DISTANCE_METERS;
		return HST_WorldPositionService.ResolveSafeGroundPosition(exitPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 12.0);
	}

	protected int ApplySupportRecallHRRefund(HST_CampaignState state, HST_EconomyService economy, HST_SupportRequestState request, int survivorInfantry)
	{
		if (!state || !request)
			return 0;

		int remainingRefund = Math.Max(0, request.m_iHRCost - request.m_iRefundedHR);
		int refund = Math.Min(remainingRefund, Math.Max(0, survivorInfantry));
		if (refund <= 0)
			return 0;

		if (economy)
			economy.AddHR(state, refund);
		else
			state.m_iHR = Math.Max(0, state.m_iHR + refund);
		request.m_iRefundedHR += refund;
		return refund;
	}

	protected string CapabilityForSupport(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
			return "supply_drop";

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return "qrf";

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_TRANSPORT || supportType == HST_ESupportRequestType.HST_SUPPORT_EVACUATION || supportType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING)
			return "rotary_wing";

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE)
			return "suppressive_fire";

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
			return "roadblock";

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return "airstrike";

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return "cruise_missile";

		return "ground_support";
	}

	protected string AssetProfileForSupport(string factionKey, HST_ESupportRequestType supportType, bool playerRequested)
	{
		if (playerRequested)
		{
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
				return "fia_supply_crate_alpha";
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
				return "fia_qrf_reserve_alpha";
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE)
				return "fia_capability_suppressive_fire_alpha";
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
				return "fia_roadblock_alpha";
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU)
				return "fia_abstract_airstrike_alpha";
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
				return "fia_abstract_airstrike_bravo";
			if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
				return "fia_abstract_long_range_strike";
		}

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU)
			return factionKey + "_abstract_airstrike_alpha";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return factionKey + "_abstract_airstrike_bravo";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return factionKey + "_abstract_long_range_strike";

		return factionKey + "_" + CapabilityForSupport(supportType);
	}

	protected bool IsHelicopterStyle(HST_ESupportRequestType supportType)
	{
		return supportType == HST_ESupportRequestType.HST_SUPPORT_TRANSPORT || supportType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING || supportType == HST_ESupportRequestType.HST_SUPPORT_EVACUATION;
	}

	protected bool IsStrikeSupport(HST_ESupportRequestType supportType)
	{
		return supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK || supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55;
	}

	protected bool IsPhysicalGroundSupport(HST_SupportRequestState request)
	{
		if (!request || request.m_bHelicopterStyle)
			return false;

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return true;
		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return true;
		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_ROADBLOCK)
			return true;

		return false;
	}

	protected bool IsPetrosAttackSupport(HST_SupportRequestState request)
	{
		if (!request)
			return false;

		return request.m_sAssetProfileId.Contains("_petros_attack") || request.m_sRuntimeStatus.Contains("petros_attack");
	}

	protected bool ShouldPhysicalizeSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request)
	{
		if (!state || !request)
			return false;

		if (!IsPhysicalGroundSupport(request))
			return false;

		if (request.m_sRuntimeStatus == "physicalize_failed_no_prefab")
			return false;

		return true;
	}

	protected bool HasActiveMissionNearSupportTarget(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request)
			return false;

		vector targetPosition = ResolvePhysicalSupportTargetPosition(state, request);
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			if (!request.m_sTargetZoneId.IsEmpty() && mission.m_sTargetZoneId == request.m_sTargetZoneId)
				return true;

			if (DistanceSq2D(mission.m_vTargetPosition, targetPosition) <= 700 * 700)
				return true;
		}

		return false;
	}

	protected bool HasActiveObjectiveNearSupportTarget(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request)
			return false;

		vector targetPosition = ResolvePhysicalSupportTargetPosition(state, request);
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_bComplete || objective.m_bFailed)
				continue;

			HST_ActiveMissionState mission = state.FindActiveMission(objective.m_sMissionInstanceId);
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			if (!request.m_sTargetZoneId.IsEmpty() && objective.m_sTargetZoneId == request.m_sTargetZoneId)
				return true;

			if (DistanceSq2D(objective.m_vPosition, targetPosition) <= 700 * 700)
				return true;
		}

		return false;
	}

	protected bool IsPhysicalSupportFinished(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request || request.m_sGroupId.IsEmpty())
			return false;

		HST_ActiveGroupState group = state.FindActiveGroup(request.m_sGroupId);
		if (!group)
			return true;

		return group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "folded" || group.m_sRuntimeStatus == "spawn_failed";
	}

	protected int ResolveInboundSpawnLeadSeconds(HST_SupportRequestState request)
	{
		if (!request || request.m_iETASeconds <= 0)
			return 0;

		int halfEta = Math.Max(5, request.m_iETASeconds / 2);
		return Math.Min(PHYSICAL_SUPPORT_INBOUND_SPAWN_SECONDS, halfEta);
	}

	protected vector ResolvePhysicalSupportTargetPosition(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!request)
			return "0 0 0";

		vector target = request.m_vTargetPosition;
		if (target[0] != 0 || target[1] != 0 || target[2] != 0)
			return HST_WorldPositionService.ResolveGroundPosition(target, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);

		if (state)
		{
			HST_ZoneState targetZone = state.FindZone(request.m_sTargetZoneId);
			if (targetZone)
				return HST_WorldPositionService.ResolveGroundPosition(targetZone.m_vPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
		}

		return "0 0 0";
	}

	protected vector ResolvePhysicalSupportStagingPosition(HST_CampaignState state, HST_SupportRequestState request)
	{
		vector target = ResolvePhysicalSupportTargetPosition(state, request);
		HST_ZoneState targetZone;
		if (state && request)
			targetZone = state.FindZone(request.m_sTargetZoneId);

		float standoff = ResolvePhysicalSupportStagingDistanceMeters(targetZone, request);
		int seed = BuildSupportGroupSeed(state, request);
		for (int attempt = 0; attempt < 32; attempt++)
		{
			vector candidate = BuildSupportApproachCandidate(target, request.m_vSourcePosition, seed, attempt, standoff);
			vector resolved;
			if (!HST_WorldPositionService.TryResolveDryStagingPosition(candidate, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, resolved, 3.0))
				continue;
			if (!IsSupportStagingPositionAllowed(state, request, resolved))
				continue;

			return resolved;
		}

		vector fallback;
		if (HST_WorldPositionService.TryResolveDryStagingPosition(request.m_vSourcePosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, fallback, 3.0) && IsSupportStagingPositionAllowed(state, request, fallback))
			return fallback;

		if (HST_WorldPositionService.TryResolveDryStagingPosition(target, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, fallback, 3.0) && IsSupportStagingPositionAllowed(state, request, fallback))
			return fallback;

		if (TryResolvePetrosAttackStagingPosition(state, request, fallback))
			return fallback;

		return ResolvePhysicalSupportFallbackPosition(state, request, target);
	}

	protected float ResolvePhysicalSupportStagingDistanceMeters(HST_ZoneState targetZone, HST_SupportRequestState request = null)
	{
		if (IsPetrosAttackSupport(request))
			return Math.Max(PETROS_ATTACK_MIN_STANDOFF_METERS + PETROS_ATTACK_STAGING_MARGIN_METERS, ResolvePhysicalSupportStandoffMeters(targetZone));

		return Math.Max(PHYSICAL_SUPPORT_MIN_STANDOFF_METERS, ResolvePhysicalSupportStandoffMeters(targetZone) * 0.5);
	}

	protected vector ResolvePhysicalSupportArrivalPosition(HST_CampaignState state, HST_SupportRequestState request)
	{
		vector target = ResolvePhysicalSupportTargetPosition(state, request);
		HST_ZoneState targetZone;
		if (state && request)
			targetZone = state.FindZone(request.m_sTargetZoneId);

		float standoff = ResolvePhysicalSupportStagingDistanceMeters(targetZone, request);
		int seed = BuildSupportGroupSeed(state, request);
		for (int attempt = 0; attempt < 24; attempt++)
		{
			vector candidate = BuildSupportApproachCandidate(target, request.m_vSourcePosition, seed, attempt, standoff);
			vector resolved;
			if (!HST_WorldPositionService.TryResolveDryStagingPosition(candidate, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, resolved, 3.0))
				continue;
			if (!IsSupportStagingPositionAllowed(state, request, resolved))
				continue;

			return resolved;
		}

		vector fallback;
		if (HST_WorldPositionService.TryResolveDryStagingPosition(target, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, fallback, 3.0) && IsSupportStagingPositionAllowed(state, request, fallback))
			return fallback;

		if (TryResolvePetrosAttackStagingPosition(state, request, fallback))
			return fallback;

		return ResolvePhysicalSupportFallbackPosition(state, request, target);
	}

	protected bool TryResolvePetrosAttackStagingPosition(HST_CampaignState state, HST_SupportRequestState request, out vector resolved)
	{
		resolved = "0 0 0";
		if (!IsPetrosAttackSupport(request) || !state || !state.m_bHQDeployed)
			return false;

		int seed = BuildSupportGroupSeed(state, request) + 1703;
		float standoff = PETROS_ATTACK_MIN_STANDOFF_METERS + PETROS_ATTACK_STAGING_MARGIN_METERS;
		for (int attempt = 0; attempt < 48; attempt++)
		{
			vector candidate = BuildSupportApproachCandidate(state.m_vHQPosition, request.m_vSourcePosition, seed, attempt, standoff);
			if (!HST_WorldPositionService.TryResolveDryStagingPosition(candidate, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, resolved, 3.0))
				continue;
			if (IsInsidePetrosAttackStandoff(state, resolved))
				continue;
			if (IsOutsidePetrosAttackMaxStaging(state, resolved))
				continue;

			return true;
		}

		return false;
	}

	protected vector ResolvePhysicalSupportFallbackPosition(HST_CampaignState state, HST_SupportRequestState request, vector target)
	{
		if (!IsPetrosAttackSupport(request) || !state || !state.m_bHQDeployed)
			return HST_WorldPositionService.ResolveSafeGroundPosition(target, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 8.0);

		float standoff = PETROS_ATTACK_MIN_STANDOFF_METERS + PETROS_ATTACK_STAGING_MARGIN_METERS;
		vector candidate = BuildSupportApproachCandidate(state.m_vHQPosition, request.m_vSourcePosition, BuildSupportGroupSeed(state, request) + 2909, 0, standoff);
		vector resolved = HST_WorldPositionService.ResolveSafeGroundPosition(candidate, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 8.0);
		if (IsSupportStagingPositionAllowed(state, request, resolved))
			return resolved;

		return HST_WorldPositionService.ResolveGroundPosition(candidate, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
	}

	protected bool IsSupportStagingPositionAllowed(HST_CampaignState state, HST_SupportRequestState request, vector position)
	{
		if (IsPetrosAttackSupport(request))
		{
			if (IsInsidePetrosAttackStandoff(state, position))
				return false;
			if (IsOutsidePetrosAttackMaxStaging(state, position))
				return false;
		}
		else if (IsInsideHQSafeRadius(state, position))
		{
			return false;
		}

		return true;
	}

	protected float ResolvePhysicalSupportStandoffMeters(HST_ZoneState targetZone)
	{
		float radius;
		if (targetZone)
			radius = targetZone.m_iCaptureRadiusMeters;
		if (radius <= 0 && targetZone)
			radius = targetZone.m_iActivationRadiusMeters * 0.35;
		if (radius <= 0)
			radius = PHYSICAL_SUPPORT_MIN_STANDOFF_METERS;

		return Math.Min(PHYSICAL_SUPPORT_MAX_STANDOFF_METERS, Math.Max(PHYSICAL_SUPPORT_MIN_STANDOFF_METERS, radius + PHYSICAL_SUPPORT_EXTRA_STANDOFF_METERS));
	}

	protected vector BuildSupportApproachCandidate(vector target, vector source, int seed, int attempt, float standoffMeters)
	{
		vector candidate = target;
		float dx = target[0] - source[0];
		float dz = target[2] - source[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length > 1.0 && attempt < 2)
		{
			float distance = standoffMeters + attempt * 45.0;
			candidate[0] = target[0] - dx / length * distance;
			candidate[2] = target[2] - dz / length * distance;
			return candidate;
		}

		int slot = HST_DefaultCatalog.PositiveMod(seed + attempt, 8);
		float distanceBySlot = standoffMeters + (attempt / 8) * 55.0;
		float x = 1.0;
		float z = 0.0;
		if (slot == 1)
		{
			x = 0.707;
			z = 0.707;
		}
		else if (slot == 2)
		{
			x = 0.0;
			z = 1.0;
		}
		else if (slot == 3)
		{
			x = -0.707;
			z = 0.707;
		}
		else if (slot == 4)
		{
			x = -1.0;
			z = 0.0;
		}
		else if (slot == 5)
		{
			x = -0.707;
			z = -0.707;
		}
		else if (slot == 6)
		{
			x = 0.0;
			z = -1.0;
		}
		else if (slot == 7)
		{
			x = 0.707;
			z = -0.707;
		}

		candidate[0] = target[0] + x * distanceBySlot;
		candidate[2] = target[2] + z * distanceBySlot;
		return candidate;
	}

	protected bool IsInsideHQSafeRadius(HST_CampaignState state, vector position)
	{
		if (!state || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, position) <= HQ_SAFE_RADIUS_METERS * HQ_SAFE_RADIUS_METERS;
	}

	protected bool IsInsidePetrosAttackStandoff(HST_CampaignState state, vector position)
	{
		if (!state || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, position) < PETROS_ATTACK_MIN_STANDOFF_METERS * PETROS_ATTACK_MIN_STANDOFF_METERS;
	}

	protected bool IsOutsidePetrosAttackMaxStaging(HST_CampaignState state, vector position)
	{
		if (!state || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, position) > PETROS_ATTACK_MAX_STAGING_METERS * PETROS_ATTACK_MAX_STAGING_METERS;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}

	protected void ActivateStrikeSupport(HST_SupportRequestState request)
	{
		if (!request)
			return;

		request.m_sRuntimeEntityId = "abstract_strike";
		request.m_bPhysicalStrikeSpawned = false;
		Print(string.Format("h-istasi | abstract strike support %1 active", request.m_sRequestId));
	}

	protected void ResolveStrikeSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request)
	{
		if (!state || !preset || !request)
			return;

		HST_ZoneState targetZone = state.FindZone(request.m_sTargetZoneId);
		if (!targetZone)
			return;

		request.m_bAbstractResolved = true;
		if (request.m_sFactionKey == preset.m_sResistanceFactionKey)
		{
			if (targetZone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				targetZone.m_iResistanceCaptureProgress = Math.Min(HST_ZoneCaptureService.CAPTURE_PROGRESS_REQUIRED - 1, targetZone.m_iResistanceCaptureProgress + StrikeCaptureProgress(request.m_eType));

			return;
		}

		if (targetZone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
		{
			targetZone.m_iSupport = Math.Max(0, targetZone.m_iSupport - StrikeSupportDamage(request.m_eType));
			targetZone.m_iResistanceCaptureProgress = Math.Max(0, targetZone.m_iResistanceCaptureProgress - StrikeCaptureProgress(request.m_eType));
		}
		else
		{
			targetZone.m_iResistanceCaptureProgress = Math.Max(0, targetZone.m_iResistanceCaptureProgress - StrikeSupportDamage(request.m_eType));
		}
	}

	protected int StrikeCaptureProgress(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return 35;

		return 20;
	}

	protected int StrikeSupportDamage(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return 25;

		return 12;
	}

	protected string StrikeKindForSupport(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU)
			return "airstrike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return "heavy_airstrike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return "long_range_strike";

		return "";
	}

	protected string StrikeConfigForSupport(HST_ESupportRequestType supportType)
	{
		return "";
	}

	protected string BuildRequestId(HST_CampaignState state, string factionKey, HST_ESupportRequestType supportType)
	{
		return string.Format("support_%1_%2_%3", factionKey, state.m_iElapsedSeconds, state.m_aSupportRequests.Count());
	}

	protected string FindSourceZoneId(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, string targetZoneId)
	{
		HST_ZoneState targetZone = state.FindZone(targetZoneId);
		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sZoneId == targetZoneId)
				continue;

			if (zone.m_sOwnerFactionKey != factionKey)
				continue;

			if (!targetZone)
				return zone.m_sZoneId;

			float distanceSq = DistanceSq2D(zone.m_vPosition, targetZone.m_vPosition);
			if (distanceSq < bestDistanceSq)
			{
				bestDistanceSq = distanceSq;
				bestZone = zone;
			}
		}

		if (bestZone)
			return bestZone.m_sZoneId;

		return targetZoneId;
	}

	protected vector ResolveSourcePosition(HST_CampaignState state, string sourceZoneId, vector fallback)
	{
		HST_ZoneState sourceZone = state.FindZone(sourceZoneId);
		if (sourceZone)
			return sourceZone.m_vPosition;

		return fallback;
	}

	protected HST_ZoneState FindFallbackTargetZone(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				return zone;
		}

		if (state.m_aZones.Count() > 0)
			return state.m_aZones[0];

		return null;
	}

	protected string SelectGroupPrefab(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!request)
			return "";

		string factionKey = request.m_sFactionKey;
		HST_ESupportRequestType supportType = request.m_eType;
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (!faction)
			return "";

		int seed = BuildSupportGroupSeed(state, request);
		bool qrfStyle = supportType == HST_ESupportRequestType.HST_SUPPORT_QRF;
		qrfStyle = qrfStyle || supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
		array<string> candidates = {};
		if (qrfStyle)
		{
			AppendUniqueGroupPrefabs(candidates, faction.m_aQRFGroupPrefabs);
			AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
			return SelectRandomGroupPrefab(candidates, seed + 701);
		}

		AppendUniqueGroupPrefabs(candidates, faction.m_aPatrolGroupPrefabs);
		AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
		return SelectRandomGroupPrefab(candidates, seed + 503);
	}

	protected void AppendUniqueGroupPrefabs(array<string> candidates, array<string> source)
	{
		if (!candidates || !source)
			return;

		foreach (string prefab : source)
		{
			if (prefab.IsEmpty() || candidates.Contains(prefab))
				continue;

			candidates.Insert(prefab);
		}
	}

	protected string SelectRandomGroupPrefab(array<string> prefabs, int seed)
	{
		if (!prefabs || prefabs.Count() == 0)
			return "";

		return prefabs[HST_DefaultCatalog.PositiveMod(seed, prefabs.Count())];
	}

	protected int BuildSupportGroupSeed(HST_CampaignState state, HST_SupportRequestState request)
	{
		int seed = 613;
		if (state)
			seed += state.m_iCampaignSeed * 17 + state.m_iElapsedSeconds * 5 + state.m_aSupportRequests.Count() * 29;
		if (request)
			seed += request.m_sRequestId.Length() * 47 + request.m_sTargetZoneId.Length() * 89 + request.m_iRequestedAtSecond;
		if (seed < 0)
			seed = -seed;

		return seed;
	}
}
