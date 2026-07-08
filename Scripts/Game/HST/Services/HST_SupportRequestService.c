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

		return string.Format(
			"h-istasi support | requested %1 | %2 | target %3 at %4 | eta %5s | cost $%6 enemy %7/%8 | cooldown %9",
			m_Request.m_sRequestId,
			m_Request.m_eType,
			m_Request.m_sTargetZoneId,
			m_Request.m_vTargetPosition,
			m_Request.m_iETASeconds,
			m_Request.m_iMoneyCost,
			m_Request.m_iAttackCost,
			m_Request.m_iSupportCost,
			m_Request.m_iCooldownUntilSecond
		);
	}
}
class HST_SupportRequestService
{
	static const int PLAYER_SUPPLY_COST = 150;
	static const int PLAYER_QRF_COST = 250;
	static const int PLAYER_FIRE_COST = 350;
	static const int PLAYER_AIRSTRIKE_COST = 750;
	static const int PLAYER_CRUISE_MISSILE_COST = 1200;
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

	protected bool m_bMarkerRefreshNeeded;
	protected ref HST_ForceCompositionService m_ForceCompositions = new HST_ForceCompositionService();
	protected ref HST_SpawnPlacementService m_SpawnPlacements = new HST_SpawnPlacementService();

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

	HST_SupportRequestState RequestSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ESupportRequestType supportType, string targetZoneId, bool playerRequested = false, int playerCooldownSeconds = PLAYER_SUPPORT_COOLDOWN_SECONDS)
	{
		HST_SupportRequestResult result = RequestSupportDetailed(state, preset, economy, enemyDirector, factionKey, supportType, targetZoneId, playerRequested, playerCooldownSeconds);
		if (!result || !result.m_bSuccess)
			return null;

		return result.m_Request;
	}

	HST_SupportRequestResult RequestSupportDetailed(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ESupportRequestType supportType, string targetZoneId, bool playerRequested = false, int playerCooldownSeconds = PLAYER_SUPPORT_COOLDOWN_SECONDS)
	{
		return RequestSupportDetailedInternal(state, preset, economy, enemyDirector, factionKey, supportType, targetZoneId, "0 0 0", false, playerRequested, playerCooldownSeconds);
	}

	HST_SupportRequestResult RequestSupportAtPositionDetailed(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ESupportRequestType supportType, string targetZoneId, vector targetPosition, bool playerRequested = false, int playerCooldownSeconds = PLAYER_SUPPORT_COOLDOWN_SECONDS)
	{
		return RequestSupportDetailedInternal(state, preset, economy, enemyDirector, factionKey, supportType, targetZoneId, targetPosition, true, playerRequested, playerCooldownSeconds);
	}

	protected HST_SupportRequestResult RequestSupportDetailedInternal(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ESupportRequestType supportType, string targetZoneId, vector targetPosition, bool useExplicitTargetPosition, bool playerRequested, int playerCooldownSeconds)
	{
		HST_SupportRequestResult result = new HST_SupportRequestResult();

		if (!state || !preset)
		{
			result.m_sFailureReason = "campaign state or preset not ready";
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

		int moneyCost = ResolvePlayerMoneyCost(supportType);
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

			if (state.m_iFactionMoney < moneyCost)
			{
				result.m_sFailureReason = string.Format("need $%1, have $%2", moneyCost, state.m_iFactionMoney);
				return result;
			}

			if (!economy.SpendFactionMoney(state, moneyCost))
			{
				result.m_sFailureReason = "money spend failed";
				return result;
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

		string sourceZoneId = FindSourceZoneId(state, preset, factionKey, targetZone.m_sZoneId);
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

	protected HST_SupportRequestState BuildSupportRequestRecord(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, HST_ESupportRequestType supportType, HST_ZoneState targetZone, vector targetPosition, vector sourcePosition, bool playerRequested, int moneyCost, int attackCost, int supportCost, int playerCooldownSeconds)
	{
		HST_SupportRequestState request = new HST_SupportRequestState();
		request.m_sRequestId = BuildRequestId(state, factionKey, supportType);
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

	bool ConsumeMarkerRefreshNeeded()
	{
		bool result = m_bMarkerRefreshNeeded;
		m_bMarkerRefreshNeeded = false;
		return result;
	}

	bool Tick(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_PhysicalWarService physicalWar = null, HST_StrategicService strategic = null, HST_HQService hq = null)
	{
		if (!state || !preset)
			return false;

		bool changed;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
				continue;

			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED)
				changed = ActivateSupportRequest(state, preset, request) || changed;

			if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				continue;

			changed = TickActiveSupportRequest(state, preset, garrisons, physicalWar, request, strategic, hq) || changed;
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

	protected bool TickActiveSupportRequest(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_PhysicalWarService physicalWar, HST_SupportRequestState request, HST_StrategicService strategic = null, HST_HQService hq = null)
	{
		if (!state || !request)
			return false;

		if (IsPhysicalGroundSupport(request))
			return TickPhysicalGroundSupport(state, preset, garrisons, physicalWar, request, strategic, hq);

		int arrivalAtSecond = request.m_iRequestedAtSecond + Math.Max(0, request.m_iETASeconds);
		if (state.m_iElapsedSeconds < arrivalAtSecond)
			return false;

		return ResolveSupport(state, preset, garrisons, request, strategic, hq);
	}

	protected bool TickPhysicalGroundSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_PhysicalWarService physicalWar, HST_SupportRequestState request, HST_StrategicService strategic = null, HST_HQService hq = null)
	{
		if (!state || !request)
			return false;

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
			line = line + string.Format(" | abstract %1 | physicalStrike %2%3%4", request.m_bAbstractResolved, request.m_bPhysicalStrikeSpawned, compositionDetail, detail);
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

		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED;
		request.m_sFailureReason = "cancelled by commander";
		request.m_sRuntimeStatus = "cancelled";
		request.m_sResolutionKind = "cancelled";
		request.m_iResolvedAtSecond = state.m_iElapsedSeconds;
		m_bMarkerRefreshNeeded = true;
		return true;
	}

	protected bool ApplyActiveSupport(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request, bool arrivedAtTarget = false)
	{
		if (!state || !request || request.m_bHelicopterStyle)
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

		if (!m_SpawnPlacements)
			m_SpawnPlacements = new HST_SpawnPlacementService();

		bool requireVehicleSafe = composition.m_iVehicleCount > 0;
		HST_SpawnPlacementResult placement = m_SpawnPlacements.ResolvePlacement(state, preset, m_SpawnPlacements.BuildSupportPlacementRequest(state, preset, request, arrivedAtTarget, requireVehicleSafe));
		if (requireVehicleSafe && placement && !placement.m_bSuccess && placement.m_sFailureReason.Contains("vehicle-safe"))
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
			placement = m_SpawnPlacements.ResolvePlacement(state, preset, m_SpawnPlacements.BuildSupportPlacementRequest(state, preset, request, arrivedAtTarget, false));
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
		if (arrivedAtTarget)
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
		group.m_sZoneId = request.m_sTargetZoneId;
		group.m_sFactionKey = request.m_sFactionKey;
		group.m_sSupportRequestId = request.m_sRequestId;
		group.m_sPrefab = prefab;
		m_ForceCompositions.ApplyCompositionToActiveGroup(group, composition);
		group.m_sSpawnFallbackMode = "support";
		if (IsPetrosAttackSupport(request))
			group.m_sSpawnFallbackMode = "petros_attack_support";
		group.m_sRouteId = request.m_sDeploymentRouteId;
		group.m_vSourcePosition = sourcePosition;
		group.m_vTargetPosition = targetPosition;
		group.m_vPosition = sourcePosition;
		group.m_sRuntimeStatus = "support_arrived";
		if (!arrivedAtTarget)
			group.m_sRuntimeStatus = "support_active";
		group.m_iInfantryCount = Math.Max(1, groupPlan.m_iManpower);
		group.m_iVehicleCount = Math.Max(0, composition.m_iVehicleCount);
		group.m_iOriginalInfantryCount = group.m_iInfantryCount;
		group.m_iOriginalVehicleCount = group.m_iVehicleCount;
		group.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		group.m_iLastSeenAliveCount = 0;
		group.m_iSurvivorInfantryCount = group.m_iInfantryCount;
		group.m_iSurvivorVehicleCount = group.m_iVehicleCount;
		group.m_bQRF = request.m_eType == HST_ESupportRequestType.HST_SUPPORT_QRF;
		state.m_aActiveGroups.Insert(group);

		request.m_sGroupId = group.m_sGroupId;
		request.m_bPhysicalized = true;
		request.m_iPhysicalizedAtSecond = state.m_iElapsedSeconds;
		request.m_sPhysicalizationMode = "ground_group";
		request.m_sRuntimeStatus = "physical_group_linked";
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

		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_QRF || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
		{
			ApplyQRFOutcome(state, preset, garrisons, request);
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

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF || supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
		{
			attackCost = 15;
			supportCost = 5;
			return;
		}

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING || supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE)
		{
			attackCost = 20;
			supportCost = 8;
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

		if (IsHelicopterStyle(supportType))
			return HELICOPTER_STYLE_ETA_SECONDS;

		return DEFAULT_ETA_SECONDS;
	}

	protected int ResolvePlayerMoneyCost(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return PLAYER_QRF_COST;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE || supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return PLAYER_FIRE_COST;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return PLAYER_AIRSTRIKE_COST;

		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return PLAYER_CRUISE_MISSILE_COST;

		return PLAYER_SUPPLY_COST;
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

			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
			{
				reason = string.Format("support type %1 already has active request %2", supportType, request.m_sRequestId);
				return true;
			}

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

		return request.m_eType == HST_ESupportRequestType.HST_SUPPORT_QRF || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
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

		HST_ZoneState targetZone = state.FindZone(request.m_sTargetZoneId);
		if (targetZone && targetZone.m_bActive)
			return true;

		if (HasActiveMissionNearSupportTarget(state, request))
			return true;

		if (HasActiveObjectiveNearSupportTarget(state, request))
			return true;

		vector targetPosition = ResolvePhysicalSupportTargetPosition(state, request);
		return HST_WorldPositionService.IsPositionInsidePlayerEventBubble(targetPosition);
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
		bool qrfStyle = supportType == HST_ESupportRequestType.HST_SUPPORT_QRF || supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
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
