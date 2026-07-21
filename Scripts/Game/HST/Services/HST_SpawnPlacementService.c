class HST_SpawnPlacementService
{
	static const string TYPE_GENERIC = "generic";
	static const string TYPE_QRF_STAGING = "qrf_staging";
	static const string TYPE_PETROS_ATTACK_STAGING = "petros_attack_staging";
	static const string TYPE_CONVOY_ENDPOINT = "convoy_endpoint";
	static const string TYPE_ROADBLOCK = "roadblock";
	static const float HQ_SAFE_RADIUS_METERS = 900.0;
	static const float PETROS_ATTACK_MIN_STANDOFF_METERS = 760.0;
	static const float PETROS_ATTACK_STAGING_MARGIN_METERS = 90.0;
	static const float PETROS_ATTACK_MAX_STANDOFF_METERS = 1120.0;
	static const float DEFAULT_MIN_STANDOFF_METERS = 220.0;
	static const float DEFAULT_MAX_STANDOFF_METERS = 650.0;
	static const float SUPPORT_MIN_PLAYER_CLEARANCE_METERS = 180.0;
	static const float SUPPORT_MIN_ACTIVE_GROUP_CLEARANCE_METERS = 90.0;

	HST_SpawnPlacementResult ResolvePlacement(HST_CampaignState state, HST_CampaignPreset preset, HST_SpawnPlacementRequest request)
	{
		HST_SpawnPlacementResult result = new HST_SpawnPlacementResult();
		if (!request)
			return Fail(result, "spawn placement request missing");

		result.m_sRequestId = request.m_sRequestId;
		result.m_sPlacementType = NormalizePlacementType(request.m_sPlacementType);
		result.m_sSourceZoneId = request.m_sSourceZoneId;
		result.m_sTargetZoneId = request.m_sTargetZoneId;
		result.m_vTargetPosition = ResolveTargetPosition(state, request);
		if (IsZeroVector(result.m_vTargetPosition))
			return Fail(result, "target position missing");

		vector preferredSource = ResolvePreferredSourcePosition(state, request, result.m_vTargetPosition);
		vector selected = result.m_vTargetPosition;
		string failureReason;
		if (result.m_sPlacementType == TYPE_CONVOY_ENDPOINT)
		{
			if (!ResolveConvoyEndpoint(request, result, failureReason))
				return Fail(result, failureReason);
			selected = result.m_vSpawnPosition;
		}
		else if (result.m_sPlacementType == TYPE_ROADBLOCK)
		{
			if (!ResolveRoadblockPlacement(state, request, result, failureReason))
				return Fail(result, failureReason);
			selected = result.m_vSpawnPosition;
		}
		else
		{
			if (!ResolveStandoffPlacement(state, request, result, preferredSource, selected, failureReason))
				return Fail(result, failureReason);
		}

		result.m_vSpawnPosition = selected;
		ApplyClearanceMetrics(state, request, result, selected);
		result.m_bDryGround = HST_WorldPositionService.IsDryGroundPosition(selected);
		if (result.m_sPlacementType == TYPE_ROADBLOCK)
			result.m_bVehicleSafe = !request.m_bRequireVehicleSafe || HST_WorldPositionService.IsVehicleFootprintStableWithForward(selected, result.m_vRoadForward);
		else
			result.m_bVehicleSafe = !request.m_bRequireVehicleSafe || HST_WorldPositionService.IsVehicleFootprintStableForTravel(selected, result.m_vTargetPosition);
		result.m_fTargetDistanceMeters = Distance2D(selected, result.m_vTargetPosition);
		if (state && state.m_bHQDeployed)
			result.m_fHQDistanceMeters = Distance2D(selected, state.m_vHQPosition);
		result.m_bHQStandoffSatisfied = IsHQStandoffSatisfied(state, request, selected);
		if (request.m_bRequireDryGround && !result.m_bDryGround)
			return Fail(result, "resolved placement is not dry ground");
		if (request.m_bRequireVehicleSafe && !result.m_bVehicleSafe)
			return Fail(result, "resolved placement is not vehicle-safe");
		if (!result.m_bHQStandoffSatisfied)
			return Fail(result, "resolved placement violates HQ standoff");

		result.m_bSuccess = true;
		result.m_sDebugSummary = BuildDebugSummary(request, result);
		return result;
	}

	HST_SpawnPlacementRequest BuildSupportPlacementRequest(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState supportRequest, bool arrivedAtTarget = false, bool requireVehicleSafe = false)
	{
		HST_SpawnPlacementRequest request = new HST_SpawnPlacementRequest();
		if (!supportRequest)
			return request;

		request.m_sRequestId = "support_place_" + supportRequest.m_sRequestId;
		request.m_sPlacementType = TYPE_QRF_STAGING;
		if (supportRequest.m_sAssetProfileId.Contains("_petros_attack") || supportRequest.m_sRuntimeStatus.Contains("petros_attack"))
			request.m_sPlacementType = TYPE_PETROS_ATTACK_STAGING;
		request.m_sSourceZoneId = supportRequest.m_sSourceZoneId;
		request.m_sTargetZoneId = supportRequest.m_sTargetZoneId;
		request.m_vPreferredSourcePosition = supportRequest.m_vSourcePosition;
		request.m_vTargetPosition = supportRequest.m_vTargetPosition;
		request.m_fMinStandoffMeters = DEFAULT_MIN_STANDOFF_METERS;
		request.m_fMaxStandoffMeters = DEFAULT_MAX_STANDOFF_METERS;
		if (request.m_sPlacementType == TYPE_PETROS_ATTACK_STAGING)
		{
			request.m_bUseHQAsTarget = true;
			request.m_bAvoidHQSafeRadius = true;
			request.m_fMinStandoffMeters = PETROS_ATTACK_MIN_STANDOFF_METERS + PETROS_ATTACK_STAGING_MARGIN_METERS;
			request.m_fMaxStandoffMeters = PETROS_ATTACK_MAX_STANDOFF_METERS;
		}
		if (arrivedAtTarget && request.m_sPlacementType != TYPE_PETROS_ATTACK_STAGING)
		{
			request.m_fMinStandoffMeters = Math.Max(80.0, request.m_fMinStandoffMeters * 0.35);
			request.m_fMaxStandoffMeters = Math.Max(request.m_fMinStandoffMeters + 30.0, request.m_fMaxStandoffMeters * 0.55);
		}
		request.m_bRequireDryGround = true;
		request.m_bRequireVehicleSafe = requireVehicleSafe;
		request.m_bPreferRoadSource = true;
		request.m_bRequireRoadSource = false;
		request.m_bExplain = true;
		request.m_sReason = "support physicalization";
		request.m_bAvoidLivingPlayers = true;
		request.m_bAvoidActiveGroups = true;
		request.m_fMinPlayerDistanceMeters = SUPPORT_MIN_PLAYER_CLEARANCE_METERS;
		request.m_fMinActiveGroupDistanceMeters = SUPPORT_MIN_ACTIVE_GROUP_CLEARANCE_METERS;
		return request;
	}

	HST_SpawnPlacementRequest BuildRoadblockPlacementRequest(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState supportRequest)
	{
		HST_SpawnPlacementRequest request = new HST_SpawnPlacementRequest();
		if (!supportRequest)
			return request;

		request.m_sRequestId = "roadblock_place_" + supportRequest.m_sRequestId;
		request.m_sPlacementType = TYPE_ROADBLOCK;
		request.m_sSourceZoneId = supportRequest.m_sSourceZoneId;
		request.m_sTargetZoneId = supportRequest.m_sTargetZoneId;
		request.m_vPreferredSourcePosition = supportRequest.m_vSourcePosition;
		request.m_vTargetPosition = supportRequest.m_vTargetPosition;
		request.m_fRoadSearchRadiusMeters = 450.0;
		request.m_bRequireDryGround = true;
		request.m_bRequireVehicleSafe = true;
		request.m_bRequireRoadTarget = true;
		request.m_bExplain = true;
		request.m_sReason = "roadblock support placement";
		return request;
	}

	HST_SpawnPlacementRequest BuildConvoyEndpointRequest(string requestId, vector sourcePosition, vector targetPosition, bool sourceEndpoint)
	{
		HST_SpawnPlacementRequest request = new HST_SpawnPlacementRequest();
		request.m_sRequestId = requestId;
		request.m_sPlacementType = TYPE_CONVOY_ENDPOINT;
		request.m_vPreferredSourcePosition = sourcePosition;
		request.m_vTargetPosition = targetPosition;
		request.m_bRequireDryGround = true;
		request.m_bRequireVehicleSafe = true;
		request.m_bPreferRoadSource = sourceEndpoint;
		request.m_bRequireRoadSource = sourceEndpoint;
		request.m_bPreferRoadTarget = !sourceEndpoint;
		request.m_bRequireRoadTarget = !sourceEndpoint;
		request.m_fRoadSearchRadiusMeters = 350.0;
		request.m_bAvoidLivingPlayers = true;
		request.m_fMinPlayerDistanceMeters = SUPPORT_MIN_PLAYER_CLEARANCE_METERS;
		request.m_bExplain = true;
		request.m_sReason = "convoy endpoint";
		return request;
	}

#ifdef ENABLE_DIAG
	string BuildPlacementReport(HST_CampaignState state, HST_CampaignPreset preset)
	{
		string report = "Partisan spawn placement";
		HST_ZoneState zone = FindDebugTargetZone(state, preset);
		if (!zone)
			return report + "\nno target zone";

		HST_SpawnPlacementRequest qrf = new HST_SpawnPlacementRequest();
		qrf.m_sRequestId = "debug_place_qrf";
		qrf.m_sPlacementType = TYPE_QRF_STAGING;
		qrf.m_sTargetZoneId = zone.m_sZoneId;
		qrf.m_vTargetPosition = zone.m_vPosition;
		qrf.m_vPreferredSourcePosition = ResolveDebugSourcePosition(state, preset, zone);
		qrf.m_bRequireDryGround = true;
		qrf.m_bPreferRoadSource = true;
		qrf.m_bRequireRoadSource = false;
		qrf.m_sReason = "debug qrf";
		qrf.m_bExplain = true;
		report = report + "\n" + ResolvePlacement(state, preset, qrf).m_sDebugSummary;

		HST_SpawnPlacementRequest petros = new HST_SpawnPlacementRequest();
		petros.m_sRequestId = "debug_place_petros_attack";
		petros.m_sPlacementType = TYPE_PETROS_ATTACK_STAGING;
		petros.m_sTargetZoneId = zone.m_sZoneId;
		petros.m_vPreferredSourcePosition = ResolveDebugSourcePosition(state, preset, zone);
		petros.m_bUseHQAsTarget = true;
		petros.m_bAvoidHQSafeRadius = true;
		petros.m_fMinStandoffMeters = PETROS_ATTACK_MIN_STANDOFF_METERS + PETROS_ATTACK_STAGING_MARGIN_METERS;
		petros.m_fMaxStandoffMeters = PETROS_ATTACK_MAX_STANDOFF_METERS;
		petros.m_bRequireDryGround = true;
		petros.m_sReason = "debug petros attack";
		petros.m_bExplain = true;
		report = report + "\n" + ResolvePlacement(state, preset, petros).m_sDebugSummary;

		HST_SpawnPlacementRequest convoy = BuildConvoyEndpointRequest("debug_place_convoy_source", ResolveDebugSourcePosition(state, preset, zone), zone.m_vPosition, true);
		report = report + "\n" + ResolvePlacement(state, preset, convoy).m_sDebugSummary;
		return report;
	}
#endif

	protected bool ResolveStandoffPlacement(HST_CampaignState state, HST_SpawnPlacementRequest request, HST_SpawnPlacementResult result, vector preferredSource, out vector selected, out string failureReason)
	{
		float minStandoff = ResolveMinStandoff(request);
		float maxStandoff = ResolveMaxStandoff(request, minStandoff);
		int seed = BuildPlacementSeed(state, request, result);
		for (int attempt = 0; attempt < 48; attempt++)
		{
			vector candidate = BuildApproachCandidate(result.m_vTargetPosition, preferredSource, seed, attempt, minStandoff, maxStandoff);
			if (TryAcceptCandidate(state, request, result, candidate, selected, failureReason))
				return true;
		}

		if (TryAcceptCandidate(state, request, result, preferredSource, selected, failureReason))
			return true;

		failureReason = "no dry spawn placement satisfied standoff constraints";
		return false;
	}

	protected bool ResolveConvoyEndpoint(HST_SpawnPlacementRequest request, HST_SpawnPlacementResult result, out string failureReason)
	{
		vector preferred = result.m_vTargetPosition;
		vector destination = result.m_vTargetPosition;
		if (request.m_bRequireRoadSource || request.m_bPreferRoadSource)
		{
			preferred = request.m_vPreferredSourcePosition;
			destination = result.m_vTargetPosition;
		}

		vector roadPosition;
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		float roadSearchRadius = Math.Max(1.0, request.m_fRoadSearchRadiusMeters);
		if (!HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(
			preferred,
			roadSearchRadius,
			destination,
			roadPosition,
			roadForward,
			roadWidth,
			roadDistance,
			roadReason
		))
		{
			failureReason = "convoy endpoint road resolution failed: " + roadReason;
			return false;
		}

		if (!HST_WorldPositionService.IsVehicleFootprintStableWithForward(roadPosition, roadForward))
		{
			failureReason = "convoy endpoint road point failed vehicle-safe dry-ground validation";
			return false;
		}

		result.m_bRoadResolved = true;
		result.m_vRoadForward = roadForward;
		result.m_fRoadDistanceMeters = roadDistance;
		result.m_vSpawnPosition = roadPosition;
		return true;
	}

	protected bool ResolveRoadblockPlacement(HST_CampaignState state, HST_SpawnPlacementRequest request, HST_SpawnPlacementResult result, out string failureReason)
	{
		vector preferred = result.m_vTargetPosition;
		vector destination = request.m_vPreferredSourcePosition;
		bool hqReady = state && state.m_bHQDeployed;
		hqReady = hqReady && !IsZeroVector(state.m_vHQPosition);
		if (hqReady)
			destination = state.m_vHQPosition;
		if (IsZeroVector(destination))
			destination = result.m_vTargetPosition;

		vector roadPosition;
		vector roadForward;
		float roadWidth;
		float roadDistance;
		string roadReason;
		float roadSearchRadius = Math.Max(1.0, request.m_fRoadSearchRadiusMeters);
		if (!HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(
			preferred,
			roadSearchRadius,
			destination,
			roadPosition,
			roadForward,
			roadWidth,
			roadDistance,
			roadReason
		))
		{
			failureReason = "roadblock road resolution failed: " + roadReason;
			return false;
		}

		if (!HST_WorldPositionService.IsVehicleFootprintStableWithForward(roadPosition, roadForward))
		{
			failureReason = "roadblock road point failed vehicle-safe dry-ground validation";
			return false;
		}

		result.m_bRoadResolved = true;
		result.m_vRoadForward = roadForward;
		result.m_fRoadDistanceMeters = roadDistance;
		result.m_vSpawnPosition = roadPosition;
		return true;
	}

	protected bool TryAcceptCandidate(HST_CampaignState state, HST_SpawnPlacementRequest request, HST_SpawnPlacementResult result, vector candidate, out vector selected, out string failureReason)
	{
		vector resolved;
		if (!HST_WorldPositionService.TryResolveDryStagingPosition(candidate, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, resolved, 3.0))
		{
			failureReason = "candidate was not dry ground";
			return false;
		}
		if (!IsWithinStandoff(request, result.m_vTargetPosition, resolved))
		{
			failureReason = "candidate outside standoff range";
			return false;
		}
		if (!IsHQStandoffSatisfied(state, request, resolved))
		{
			failureReason = "candidate violates HQ standoff";
			return false;
		}

		vector accepted = resolved;
		bool acceptedRoadResolved = false;
		vector acceptedRoadForward = "0 0 0";
		float acceptedRoadDistance = 0.0;
		if (request.m_bPreferRoadSource || request.m_bRequireRoadSource)
		{
			vector roadPosition;
			vector roadForward;
			float roadWidth;
			float roadDistance;
			string roadReason;
			float roadSearchRadius = Math.Max(1.0, request.m_fRoadSearchRadiusMeters);
			if (HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(
				resolved,
				roadSearchRadius,
				result.m_vTargetPosition,
				roadPosition,
				roadForward,
				roadWidth,
				roadDistance,
				roadReason
			))
			{
				accepted = roadPosition;
				acceptedRoadResolved = true;
				acceptedRoadForward = roadForward;
				acceptedRoadDistance = roadDistance;
			}
			else if (request.m_bRequireRoadSource)
			{
				failureReason = "candidate road resolution failed: " + roadReason;
				return false;
			}
		}

		if (!IsWithinStandoff(request, result.m_vTargetPosition, accepted))
		{
			failureReason = "candidate outside standoff range after road adjustment";
			return false;
		}
		if (!IsHQStandoffSatisfied(state, request, accepted))
		{
			failureReason = "candidate violates HQ standoff after road adjustment";
			return false;
		}
		if (!IsClearOfLivingPlayers(request, accepted, result))
		{
			failureReason = "candidate too close to living player";
			return false;
		}
		if (!IsClearOfActiveGroups(state, request, accepted, result))
		{
			failureReason = "candidate too close to active AI group";
			return false;
		}

		selected = accepted;
		result.m_bRoadResolved = acceptedRoadResolved;
		result.m_vRoadForward = acceptedRoadForward;
		result.m_fRoadDistanceMeters = acceptedRoadDistance;
		return true;
	}

	protected HST_SpawnPlacementResult Fail(HST_SpawnPlacementResult result, string reason)
	{
		if (!result)
		{
			HST_SpawnPlacementResult failedResult = new HST_SpawnPlacementResult();
			failedResult.m_bSuccess = false;
			failedResult.m_sFailureReason = reason;
			failedResult.m_sDebugSummary = BuildDebugSummary(null, failedResult);
			return failedResult;
		}

		result.m_bSuccess = false;
		result.m_sFailureReason = reason;
		result.m_sDebugSummary = BuildDebugSummary(null, result);
		return result;
	}

	protected string BuildDebugSummary(HST_SpawnPlacementRequest request, HST_SpawnPlacementResult result)
	{
		if (!result)
			return "spawn placement | missing result";

		string summary = string.Format(
			"spawn placement | request %1 | type %2 | success %3 | target %4 | spawn %5 | targetDistance %6m | hqDistance %7m",
			EmptyField(result.m_sRequestId),
			EmptyField(result.m_sPlacementType),
			result.m_bSuccess,
			result.m_vTargetPosition,
			result.m_vSpawnPosition,
			Math.Round(result.m_fTargetDistanceMeters),
			Math.Round(result.m_fHQDistanceMeters)
		);
		summary = summary + string.Format(
			" | dry %1 | vehicleSafe %2 | road %3 roadDistance %4m | hqStandoff %5",
			result.m_bDryGround,
			result.m_bVehicleSafe,
			result.m_bRoadResolved,
			Math.Round(result.m_fRoadDistanceMeters),
			result.m_bHQStandoffSatisfied
		);
		summary = summary + string.Format(
			" | playerClear %1 nearestPlayer %2m | activeGroupClear %3 nearestGroup %4m",
			result.m_bPlayerClearanceSatisfied,
			Math.Round(result.m_fNearestPlayerDistanceMeters),
			result.m_bActiveGroupClearanceSatisfied,
			Math.Round(result.m_fNearestActiveGroupDistanceMeters)
		);
		if (!result.m_sFailureReason.IsEmpty())
			summary = summary + " | failure " + result.m_sFailureReason;
		if (request && request.m_bExplain && !request.m_sReason.IsEmpty())
			summary = summary + " | reason " + request.m_sReason;
		return summary;
	}

	protected string NormalizePlacementType(string value)
	{
		if (value.IsEmpty())
			return TYPE_GENERIC;

		return value;
	}

	protected vector ResolveTargetPosition(HST_CampaignState state, HST_SpawnPlacementRequest request)
	{
		if (request.m_bUseHQAsTarget && state && state.m_bHQDeployed)
			return HST_WorldPositionService.ResolveGroundPosition(state.m_vHQPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
		if (!IsZeroVector(request.m_vTargetPosition))
			return HST_WorldPositionService.ResolveGroundPosition(request.m_vTargetPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
		if (state && !request.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState targetZone = state.FindZone(request.m_sTargetZoneId);
			if (targetZone)
				return HST_WorldPositionService.ResolveGroundPosition(targetZone.m_vPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false);
		}

		return "0 0 0";
	}

	protected vector ResolvePreferredSourcePosition(HST_CampaignState state, HST_SpawnPlacementRequest request, vector fallback)
	{
		if (!IsZeroVector(request.m_vPreferredSourcePosition))
			return request.m_vPreferredSourcePosition;
		if (state && !request.m_sSourceZoneId.IsEmpty())
		{
			HST_ZoneState sourceZone = state.FindZone(request.m_sSourceZoneId);
			if (sourceZone)
				return sourceZone.m_vPosition;
		}

		return fallback;
	}

	protected float ResolveMinStandoff(HST_SpawnPlacementRequest request)
	{
		if (request && request.m_fMinStandoffMeters > 0)
			return request.m_fMinStandoffMeters;
		if (request && request.m_sPlacementType == TYPE_PETROS_ATTACK_STAGING)
			return PETROS_ATTACK_MIN_STANDOFF_METERS + PETROS_ATTACK_STAGING_MARGIN_METERS;

		return DEFAULT_MIN_STANDOFF_METERS;
	}

	protected float ResolveMaxStandoff(HST_SpawnPlacementRequest request, float minStandoff)
	{
		if (request && request.m_fMaxStandoffMeters > minStandoff)
			return request.m_fMaxStandoffMeters;

		return Math.Max(minStandoff + 60.0, DEFAULT_MAX_STANDOFF_METERS);
	}

	protected bool IsWithinStandoff(HST_SpawnPlacementRequest request, vector target, vector position)
	{
		float distance = Distance2D(target, position);
		float minStandoff = ResolveMinStandoff(request);
		float maxStandoff = ResolveMaxStandoff(request, minStandoff);
		return distance >= minStandoff && distance <= maxStandoff;
	}

	protected bool IsHQStandoffSatisfied(HST_CampaignState state, HST_SpawnPlacementRequest request, vector position)
	{
		if (!request || !request.m_bAvoidHQSafeRadius || !state || !state.m_bHQDeployed)
			return true;

		if (request.m_sPlacementType == TYPE_PETROS_ATTACK_STAGING)
			return Distance2D(state.m_vHQPosition, position) >= PETROS_ATTACK_MIN_STANDOFF_METERS;

		return Distance2D(state.m_vHQPosition, position) >= HQ_SAFE_RADIUS_METERS;
	}

	protected void ApplyClearanceMetrics(HST_CampaignState state, HST_SpawnPlacementRequest request, HST_SpawnPlacementResult result, vector position)
	{
		if (!result)
			return;

		result.m_fNearestPlayerDistanceMeters = ResolveNearestLivingPlayerDistanceMeters(position);
		result.m_fNearestActiveGroupDistanceMeters = ResolveNearestActiveGroupDistanceMeters(state, position);
		result.m_bPlayerClearanceSatisfied = IsClearOfLivingPlayers(request, position, result);
		result.m_bActiveGroupClearanceSatisfied = IsClearOfActiveGroups(state, request, position, result);
	}

	protected bool IsClearOfLivingPlayers(HST_SpawnPlacementRequest request, vector position, HST_SpawnPlacementResult result = null)
	{
		if (!request || !request.m_bAvoidLivingPlayers)
			return true;

		float minDistance = Math.Max(1.0, request.m_fMinPlayerDistanceMeters);
		float nearest = ResolveNearestLivingPlayerDistanceMeters(position);
		if (result)
		{
			result.m_fNearestPlayerDistanceMeters = nearest;
			result.m_bPlayerClearanceSatisfied = nearest < 0 || nearest >= minDistance;
		}

		return nearest < 0 || nearest >= minDistance;
	}

	protected bool IsClearOfActiveGroups(HST_CampaignState state, HST_SpawnPlacementRequest request, vector position, HST_SpawnPlacementResult result = null)
	{
		if (!request || !request.m_bAvoidActiveGroups)
			return true;

		float minDistance = Math.Max(1.0, request.m_fMinActiveGroupDistanceMeters);
		float nearest = ResolveNearestActiveGroupDistanceMeters(state, position);
		if (result)
		{
			result.m_fNearestActiveGroupDistanceMeters = nearest;
			result.m_bActiveGroupClearanceSatisfied = nearest < 0 || nearest >= minDistance;
		}

		return nearest < 0 || nearest >= minDistance;
	}

	protected float ResolveNearestLivingPlayerDistanceMeters(vector position)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return -1.0;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		float nearest = -1.0;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			float distance = Distance2D(playerEntity.GetOrigin(), position);
			if (nearest < 0 || distance < nearest)
				nearest = distance;
		}

		return nearest;
	}

	protected float ResolveNearestActiveGroupDistanceMeters(HST_CampaignState state, vector position)
	{
		if (!state)
			return -1.0;

		float nearest = -1.0;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || !state.IsOperationalActiveGroup(activeGroup) || IsTerminalActiveGroupStatus(activeGroup))
				continue;

			vector groupPosition = ResolveActiveGroupClearancePosition(activeGroup);
			if (IsZeroVector(groupPosition))
				continue;

			float distance = Distance2D(groupPosition, position);
			if (nearest < 0 || distance < nearest)
				nearest = distance;
		}

		return nearest;
	}

	protected vector ResolveActiveGroupClearancePosition(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return "0 0 0";
		if (!IsZeroVector(activeGroup.m_vPosition))
			return activeGroup.m_vPosition;
		if (!IsZeroVector(activeGroup.m_vSourcePosition))
			return activeGroup.m_vSourcePosition;
		return activeGroup.m_vTargetPosition;
	}

	protected bool IsTerminalActiveGroupStatus(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return true;

		return activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed";
	}

	protected IEntity GetBestPlayerEntity(PlayerManager playerManager, int playerId)
	{
		if (!playerManager || playerId <= 0)
			return null;

		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (controlledEntity)
			return controlledEntity;

		return SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
	}

	protected bool IsLivingPlayerEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected vector BuildApproachCandidate(vector target, vector source, int seed, int attempt, float minStandoff, float maxStandoff)
	{
		vector candidate = target;
		float dx = target[0] - source[0];
		float dz = target[2] - source[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		float range = Math.Max(0.0, maxStandoff - minStandoff);
		float distance = minStandoff + HST_DefaultCatalog.PositiveMod(seed + attempt * 37, Math.Max(1, Math.Round(range + 1.0)));
		if (length > 1.0 && attempt < 3)
		{
			candidate[0] = target[0] - dx / length * distance;
			candidate[2] = target[2] - dz / length * distance;
			return candidate;
		}

		int slot = HST_DefaultCatalog.PositiveMod(seed + attempt, 8);
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

		candidate[0] = target[0] + x * distance;
		candidate[2] = target[2] + z * distance;
		return candidate;
	}

	protected int BuildPlacementSeed(HST_CampaignState state, HST_SpawnPlacementRequest request, HST_SpawnPlacementResult result)
	{
		int seed = 7193;
		if (state)
			seed += state.m_iCampaignSeed * 17 + state.m_iElapsedSeconds * 5;
		if (request)
			seed += request.m_sRequestId.Length() * 43 + request.m_sPlacementType.Length() * 31 + request.m_sTargetZoneId.Length() * 19;
		if (result)
			seed += Math.Round(result.m_vTargetPosition[0]) + Math.Round(result.m_vTargetPosition[2]);
		if (seed < 0)
			seed = -seed;

		return seed;
	}

#ifdef ENABLE_DIAG
	protected HST_ZoneState FindDebugTargetZone(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return null;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && preset && zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
				return zone;
		}

		foreach (HST_ZoneState fallbackZone : state.m_aZones)
		{
			if (fallbackZone)
				return fallbackZone;
		}

		return null;
	}

	protected vector ResolveDebugSourcePosition(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone)
	{
		if (!state || !targetZone)
			return "0 0 0";

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone != targetZone && (!preset || zone.m_sOwnerFactionKey == targetZone.m_sOwnerFactionKey))
				return zone.m_vPosition;
		}

		return targetZone.m_vPosition;
	}
#endif

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected float Distance2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return Math.Sqrt(x * x + z * z);
	}

	protected string EmptyField(string value)
	{
		if (value.IsEmpty())
			return "none";

		return value;
	}
}
