class HST_SpawnPlacementService
{
	static const string TYPE_GENERIC = "generic";
	static const string TYPE_QRF_STAGING = "qrf_staging";
	static const string TYPE_PETROS_ATTACK_STAGING = "petros_attack_staging";
	static const string TYPE_CONVOY_ENDPOINT = "convoy_endpoint";
	static const float HQ_SAFE_RADIUS_METERS = 900.0;
	static const float PETROS_ATTACK_MIN_STANDOFF_METERS = 950.0;
	static const float DEFAULT_MIN_STANDOFF_METERS = 220.0;
	static const float DEFAULT_MAX_STANDOFF_METERS = 650.0;

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
		else
		{
			if (!ResolveStandoffPlacement(state, request, result, preferredSource, selected, failureReason))
				return Fail(result, failureReason);
		}

		result.m_vSpawnPosition = selected;
		result.m_bDryGround = HST_WorldPositionService.IsDryGroundPosition(selected);
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

	HST_SpawnPlacementRequest BuildSupportPlacementRequest(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState supportRequest, bool arrivedAtTarget = false)
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
			request.m_fMinStandoffMeters = PETROS_ATTACK_MIN_STANDOFF_METERS + 140.0;
			request.m_fMaxStandoffMeters = PETROS_ATTACK_MIN_STANDOFF_METERS + 450.0;
		}
		if (arrivedAtTarget)
		{
			request.m_fMinStandoffMeters = Math.Max(80.0, request.m_fMinStandoffMeters * 0.35);
			request.m_fMaxStandoffMeters = Math.Max(request.m_fMinStandoffMeters + 30.0, request.m_fMaxStandoffMeters * 0.55);
		}
		request.m_bRequireDryGround = true;
		request.m_bPreferRoadSource = true;
		request.m_bRequireRoadSource = false;
		request.m_bExplain = true;
		request.m_sReason = "support physicalization";
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
		request.m_bExplain = true;
		request.m_sReason = "convoy endpoint";
		return request;
	}

	string BuildPlacementReport(HST_CampaignState state, HST_CampaignPreset preset)
	{
		string report = "h-istasi spawn placement";
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
		petros.m_fMinStandoffMeters = PETROS_ATTACK_MIN_STANDOFF_METERS + 140.0;
		petros.m_bRequireDryGround = true;
		petros.m_sReason = "debug petros attack";
		petros.m_bExplain = true;
		report = report + "\n" + ResolvePlacement(state, preset, petros).m_sDebugSummary;

		HST_SpawnPlacementRequest convoy = BuildConvoyEndpointRequest("debug_place_convoy_source", ResolveDebugSourcePosition(state, preset, zone), zone.m_vPosition, true);
		report = report + "\n" + ResolvePlacement(state, preset, convoy).m_sDebugSummary;
		return report;
	}

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
		if (!HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(preferred, Math.Max(1.0, request.m_fRoadSearchRadiusMeters), destination, roadPosition, roadForward, roadWidth, roadDistance, roadReason))
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

		if (request.m_bPreferRoadSource || request.m_bRequireRoadSource)
		{
			vector roadPosition;
			vector roadForward;
			float roadWidth;
			float roadDistance;
			string roadReason;
			if (HST_WorldPositionService.TryResolveNearestRoadVehiclePosition(resolved, Math.Max(1.0, request.m_fRoadSearchRadiusMeters), result.m_vTargetPosition, roadPosition, roadForward, roadWidth, roadDistance, roadReason))
			{
				selected = roadPosition;
				result.m_bRoadResolved = true;
				result.m_vRoadForward = roadForward;
				result.m_fRoadDistanceMeters = roadDistance;
				return true;
			}
			if (request.m_bRequireRoadSource)
			{
				failureReason = "candidate road resolution failed: " + roadReason;
				return false;
			}
		}

		selected = resolved;
		return true;
	}

	protected HST_SpawnPlacementResult Fail(HST_SpawnPlacementResult result, string reason)
	{
		if (!result)
			result = new HST_SpawnPlacementResult();

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
			"spawn placement | request %1 | type %2 | success %3 | target %4 | spawn %5 | targetDistance %6m | hqDistance %7m | dry %8 | vehicleSafe %9 | road %10 roadDistance %11m | hqStandoff %12",
			EmptyField(result.m_sRequestId),
			EmptyField(result.m_sPlacementType),
			result.m_bSuccess,
			result.m_vTargetPosition,
			result.m_vSpawnPosition,
			Math.Round(result.m_fTargetDistanceMeters),
			Math.Round(result.m_fHQDistanceMeters),
			result.m_bDryGround,
			result.m_bVehicleSafe,
			result.m_bRoadResolved,
			Math.Round(result.m_fRoadDistanceMeters),
			result.m_bHQStandoffSatisfied
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
			return PETROS_ATTACK_MIN_STANDOFF_METERS + 140.0;

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

		return Distance2D(state.m_vHQPosition, position) >= HQ_SAFE_RADIUS_METERS;
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
