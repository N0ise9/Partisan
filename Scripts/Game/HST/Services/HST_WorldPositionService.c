class HST_WorldPositionService
{
	static const float HQ_GROUND_OFFSET = 0.1;
	static const float CHARACTER_GROUND_OFFSET = 0.25;
	static const float PROP_GROUND_OFFSET = 0.05;
	static const float VEHICLE_GROUND_OFFSET = 0.35;
	static const float MIN_DRY_SURFACE_Y = 1.0;
	static const float MAX_SAFE_SLOPE_DELTA_METERS = 1.8;
	static const float SAFE_SAMPLE_RADIUS_METERS = 2.5;
	static const float MAX_LARGE_VEHICLE_SLOPE_DELTA_METERS = 0.85;
	static const float LARGE_VEHICLE_SAMPLE_RADIUS_METERS = 6.0;
	static const float MAX_HEAVY_VEHICLE_SLOPE_DELTA_METERS = 0.65;
	static const float HEAVY_VEHICLE_SAMPLE_RADIUS_METERS = 10.0;
	static const float VEHICLE_FOOTPRINT_HALF_WIDTH_METERS = 3.5;
	static const float VEHICLE_FOOTPRINT_HALF_LENGTH_METERS = 6.5;
	static const float MAX_VEHICLE_FOOTPRINT_RANGE_METERS = 1.05;
	static const float MAX_VEHICLE_FOOTPRINT_ROLL_DELTA_METERS = 0.55;
	static const float MAX_VEHICLE_FOOTPRINT_PITCH_DELTA_METERS = 0.95;
	static const float MIN_ROAD_VEHICLE_WIDTH_METERS = 3.5;
	static const float ROAD_VEHICLE_FOOTPRINT_MARGIN_METERS = 0.75;
	static const float ROAD_VEHICLE_FOOTPRINT_HALF_WIDTH_METERS = 1.35;
	static const float ROAD_VEHICLE_FOOTPRINT_HALF_LENGTH_METERS = 3.25;
	static const float OPEN_WATER_SAMPLE_RADIUS_SMALL = 18.0;
	static const float OPEN_WATER_SAMPLE_RADIUS_LARGE = 55.0;
	static const float OPEN_WATER_MAX_DELTA_METERS = 0.08;
	static const float PLAYER_EVENT_BUBBLE_RADIUS_METERS = 1800.0;

	static bool TryResolveGroundPosition(vector source, float verticalOffset, out vector resolved, bool rejectWater = false)
	{
		resolved = source;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		float surfaceY = world.GetSurfaceY(source[0], source[2]);
		if (rejectWater && surfaceY < MIN_DRY_SURFACE_Y)
			return false;

		resolved[1] = surfaceY + verticalOffset;
		if (rejectWater && IsWaterAtPositionInWorld(world, resolved))
			return false;

		return true;
	}

	static vector ResolveGroundPosition(vector source, float verticalOffset = 0.1, bool rejectWater = false)
	{
		vector resolved;
		if (TryResolveGroundPosition(source, verticalOffset, resolved, rejectWater))
			return resolved;

		return source;
	}

	static bool TryResolveSafeGroundPosition(vector preferred, float verticalOffset, out vector resolved, bool rejectWater = true, float clearanceMeters = 2.0)
	{
		resolved = preferred;
		if (TryResolveSafeGroundPositionAt(preferred, verticalOffset, resolved, rejectWater, clearanceMeters))
			return true;

		for (int ring = 1; ring <= 4; ring++)
		{
			float radius = Math.Max(2.0, clearanceMeters) * ring;
			for (int step = 0; step < 8; step++)
			{
				vector candidate = preferred;
				if (step == 0)
					candidate[0] = candidate[0] + radius;
				else if (step == 1)
					candidate[0] = candidate[0] - radius;
				else if (step == 2)
					candidate[2] = candidate[2] + radius;
				else if (step == 3)
					candidate[2] = candidate[2] - radius;
				else if (step == 4)
				{
					candidate[0] = candidate[0] + radius;
					candidate[2] = candidate[2] + radius;
				}
				else if (step == 5)
				{
					candidate[0] = candidate[0] - radius;
					candidate[2] = candidate[2] + radius;
				}
				else if (step == 6)
				{
					candidate[0] = candidate[0] + radius;
					candidate[2] = candidate[2] - radius;
				}
				else
				{
					candidate[0] = candidate[0] - radius;
					candidate[2] = candidate[2] - radius;
				}

				if (TryResolveSafeGroundPositionAt(candidate, verticalOffset, resolved, rejectWater, clearanceMeters))
					return true;
			}
		}

		return false;
	}

	static bool TryResolveDryStagingPosition(vector preferred, float verticalOffset, out vector resolved, float clearanceMeters = 3.0)
	{
		if (!TryResolveSafeGroundPosition(preferred, verticalOffset, resolved, true, clearanceMeters))
			return false;

		if (IsLikelyOpenWater(resolved))
			return false;

		return true;
	}

	static bool IsLikelyOpenWater(vector source)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		if (IsWaterAtPositionInWorld(world, source))
			return true;

		float smallDelta = ResolveSurfaceDelta(world, source, OPEN_WATER_SAMPLE_RADIUS_SMALL);
		if (smallDelta > OPEN_WATER_MAX_DELTA_METERS)
			return false;

		float largeDelta = ResolveSurfaceDelta(world, source, OPEN_WATER_SAMPLE_RADIUS_LARGE);
		return largeDelta <= OPEN_WATER_MAX_DELTA_METERS;
	}

	static vector ResolveSafeGroundPosition(vector preferred, float verticalOffset = 0.1, bool rejectWater = true, float clearanceMeters = 2.0)
	{
		vector resolved;
		if (TryResolveSafeGroundPosition(preferred, verticalOffset, resolved, rejectWater, clearanceMeters))
			return resolved;

		return ResolveGroundPosition(preferred, verticalOffset, rejectWater);
	}

	static bool TryResolveVehicleSpawnPosition(vector preferred, out vector resolved, bool rejectWater = true)
	{
		return TryResolveSafeGroundPosition(preferred, VEHICLE_GROUND_OFFSET, resolved, rejectWater, 5.0);
	}

	static bool TryResolveLargeVehicleSpawnPosition(vector preferred, out vector resolved, bool rejectWater = true)
	{
		return TryResolveSizedVehicleSpawnPosition(preferred, resolved, rejectWater, LARGE_VEHICLE_SAMPLE_RADIUS_METERS, MAX_LARGE_VEHICLE_SLOPE_DELTA_METERS);
	}

	static bool TryResolveHeavyVehicleSpawnPosition(vector preferred, out vector resolved, bool rejectWater = true)
	{
		return TryResolveSizedVehicleSpawnPosition(preferred, resolved, rejectWater, HEAVY_VEHICLE_SAMPLE_RADIUS_METERS, MAX_HEAVY_VEHICLE_SLOPE_DELTA_METERS);
	}

	static bool IsRoadNetworkAvailable(out string reason)
	{
		RoadNetworkManager roadManager;
		return TryResolveRoadNetworkManager(roadManager, reason);
	}

	static bool TryResolveNearestRoadVehiclePosition(vector preferred, float searchRadius, vector destination, out vector roadPosition, out vector roadForward, out float roadWidth, out float roadDistance, out string reason)
	{
		roadPosition = preferred;
		roadForward = BuildRoadForward(preferred, destination);
		roadWidth = 0;
		roadDistance = -1.0;
		reason = "";

		RoadNetworkManager roadManager;
		if (!TryResolveRoadNetworkManager(roadManager, reason))
			return false;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			reason = "world unavailable for road vehicle position";
			return false;
		}

		float radius = Math.Max(1.0, searchRadius);
		array<BaseRoad> roads = {};
		vector aabbMin = preferred;
		aabbMin[0] = aabbMin[0] - radius;
		aabbMin[1] = aabbMin[1] - 50.0;
		aabbMin[2] = aabbMin[2] - radius;
		vector aabbMax = preferred;
		aabbMax[0] = aabbMax[0] + radius;
		aabbMax[1] = aabbMax[1] + 50.0;
		aabbMax[2] = aabbMax[2] + radius;
		roadManager.GetRoadsInAABB(aabbMin, aabbMax, roads);
		if (roads.Count() == 0)
		{
			reason = string.Format("no road network roads found within %1m", Math.Round(radius));
			return false;
		}

		bool skippedNarrowRoad;
		bool skippedInvalidPoints;
		bool skippedUnsafeFootprint;
		bool found;
		float bestDistanceSq = -1.0;
		foreach (BaseRoad road : roads)
		{
			if (!road)
				continue;

			float width = road.GetWidth();
			if (width < MIN_ROAD_VEHICLE_WIDTH_METERS)
			{
				skippedNarrowRoad = true;
				continue;
			}

			array<vector> points = {};
			road.GetPoints(points);
			if (points.Count() < 2)
			{
				skippedInvalidPoints = true;
				continue;
			}

			for (int i = 1; i < points.Count(); i++)
			{
				vector segmentStart = points[i - 1];
				vector segmentEnd = points[i];
				vector candidate = ClosestPointOnSegment2D(segmentStart, segmentEnd, preferred);
				float distanceSq = DistanceSq2D(candidate, preferred);
				if (distanceSq > radius * radius)
					continue;

				vector forward = BuildRoadSegmentForward(segmentStart, segmentEnd, candidate, destination);
				vector grounded;
				if (!TryResolveGroundPosition(candidate, VEHICLE_GROUND_OFFSET, grounded, true))
					continue;
				if (IsLikelyOpenWater(grounded))
					continue;
				if (!IsRoadVehicleFootprintInsideSegmentWidth(grounded, forward, segmentStart, segmentEnd, width))
				{
					skippedUnsafeFootprint = true;
					continue;
				}
				if (!IsVehicleFootprintStableWithForward(grounded, forward))
				{
					skippedUnsafeFootprint = true;
					continue;
				}
				if (bestDistanceSq >= 0 && distanceSq >= bestDistanceSq)
					continue;

				bestDistanceSq = distanceSq;
				roadPosition = grounded;
				roadForward = forward;
				roadWidth = width;
				found = true;
			}
		}

		if (!found)
		{
			if (skippedUnsafeFootprint)
				reason = string.Format("no road point within %1m passed vehicle footprint checks", Math.Round(radius));
			else if (skippedNarrowRoad)
				reason = string.Format("only roads narrower than %1m found within %2m", Math.Round(MIN_ROAD_VEHICLE_WIDTH_METERS), Math.Round(radius));
			else if (skippedInvalidPoints)
				reason = string.Format("road network roads within %1m had insufficient points", Math.Round(radius));
			else
				reason = string.Format("no road segment within %1m of preferred position", Math.Round(radius));
			return false;
		}

		roadDistance = Math.Sqrt(bestDistanceSq);
		reason = string.Format("road-resolved | distance %1m | width %2m", Math.Round(roadDistance), Math.Round(roadWidth));
		return true;
	}

	static bool IsVehicleFootprintStable(vector source)
	{
		return IsVehicleFootprintStableWithBasis(source, 1.0, 0.0, 0.0, 1.0);
	}

	static bool IsVehicleFootprintStableForTravel(vector source, vector targetPosition)
	{
		vector forward = targetPosition;
		forward[0] = targetPosition[0] - source[0];
		forward[1] = 0;
		forward[2] = targetPosition[2] - source[2];
		return IsVehicleFootprintStableWithForward(source, forward);
	}

	static bool IsVehicleFootprintStableWithForward(vector source, vector forward)
	{
		float length = Math.Sqrt(forward[0] * forward[0] + forward[2] * forward[2]);
		if (length <= 0.01)
			return IsVehicleFootprintStable(source);

		float forwardX = forward[0] / length;
		float forwardZ = forward[2] / length;
		return IsVehicleFootprintStableWithBasis(source, -forwardZ, forwardX, forwardX, forwardZ);
	}

	protected static bool IsVehicleFootprintStableWithBasis(vector source, float sideX, float sideZ, float forwardX, float forwardZ)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		float centerY;
		if (!TryReadDrySurfaceY(world, source[0], source[2], centerY))
			return false;

		float halfWidth = VEHICLE_FOOTPRINT_HALF_WIDTH_METERS;
		float halfLength = VEHICLE_FOOTPRINT_HALF_LENGTH_METERS;
		float innerWidth = halfWidth * 0.55;
		float innerLength = halfLength * 0.55;
		float minY = centerY;
		float maxY = centerY;
		float leftY;
		float rightY;
		float frontY;
		float rearY;
		float frontLeftY;
		float frontRightY;
		float rearLeftY;
		float rearRightY;
		float innerLeftY;
		float innerRightY;
		float innerFrontY;
		float innerRearY;
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, -halfWidth, 0, leftY))
			return false;
		minY = Math.Min(minY, leftY);
		maxY = Math.Max(maxY, leftY);
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, halfWidth, 0, rightY))
			return false;
		minY = Math.Min(minY, rightY);
		maxY = Math.Max(maxY, rightY);
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, 0, halfLength, frontY))
			return false;
		minY = Math.Min(minY, frontY);
		maxY = Math.Max(maxY, frontY);
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, 0, -halfLength, rearY))
			return false;
		minY = Math.Min(minY, rearY);
		maxY = Math.Max(maxY, rearY);
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, -halfWidth, halfLength, frontLeftY))
			return false;
		minY = Math.Min(minY, frontLeftY);
		maxY = Math.Max(maxY, frontLeftY);
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, halfWidth, halfLength, frontRightY))
			return false;
		minY = Math.Min(minY, frontRightY);
		maxY = Math.Max(maxY, frontRightY);
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, -halfWidth, -halfLength, rearLeftY))
			return false;
		minY = Math.Min(minY, rearLeftY);
		maxY = Math.Max(maxY, rearLeftY);
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, halfWidth, -halfLength, rearRightY))
			return false;
		minY = Math.Min(minY, rearRightY);
		maxY = Math.Max(maxY, rearRightY);
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, -innerWidth, 0, innerLeftY))
			return false;
		minY = Math.Min(minY, innerLeftY);
		maxY = Math.Max(maxY, innerLeftY);
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, innerWidth, 0, innerRightY))
			return false;
		minY = Math.Min(minY, innerRightY);
		maxY = Math.Max(maxY, innerRightY);
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, 0, innerLength, innerFrontY))
			return false;
		minY = Math.Min(minY, innerFrontY);
		maxY = Math.Max(maxY, innerFrontY);
		if (!TryReadVehicleFootprintSample(world, source, sideX, sideZ, forwardX, forwardZ, 0, -innerLength, innerRearY))
			return false;
		minY = Math.Min(minY, innerRearY);
		maxY = Math.Max(maxY, innerRearY);

		if (maxY - minY > MAX_VEHICLE_FOOTPRINT_RANGE_METERS)
			return false;
		if (AbsFloat(leftY - rightY) > MAX_VEHICLE_FOOTPRINT_ROLL_DELTA_METERS)
			return false;
		if (AbsFloat(frontLeftY - frontRightY) > MAX_VEHICLE_FOOTPRINT_ROLL_DELTA_METERS)
			return false;
		if (AbsFloat(rearLeftY - rearRightY) > MAX_VEHICLE_FOOTPRINT_ROLL_DELTA_METERS)
			return false;
		if (AbsFloat(innerLeftY - innerRightY) > MAX_VEHICLE_FOOTPRINT_ROLL_DELTA_METERS)
			return false;
		if (AbsFloat(frontY - rearY) > MAX_VEHICLE_FOOTPRINT_PITCH_DELTA_METERS)
			return false;
		if (AbsFloat(frontLeftY - rearLeftY) > MAX_VEHICLE_FOOTPRINT_PITCH_DELTA_METERS)
			return false;
		if (AbsFloat(frontRightY - rearRightY) > MAX_VEHICLE_FOOTPRINT_PITCH_DELTA_METERS)
			return false;
		if (AbsFloat(innerFrontY - innerRearY) > MAX_VEHICLE_FOOTPRINT_PITCH_DELTA_METERS)
			return false;

		return true;
	}

	protected static bool TryResolveSizedVehicleSpawnPosition(vector preferred, out vector resolved, bool rejectWater, float sampleRadius, float maxSlopeDelta)
	{
		resolved = preferred;
		if (TryResolveVehicleSpawnPositionAt(preferred, resolved, rejectWater, sampleRadius, maxSlopeDelta))
			return true;

		for (int ring = 1; ring <= 5; ring++)
		{
			float radius = sampleRadius * ring;
			for (int step = 0; step < 8; step++)
			{
				vector candidate = preferred;
				if (step == 0)
					candidate[0] = candidate[0] + radius;
				else if (step == 1)
					candidate[0] = candidate[0] - radius;
				else if (step == 2)
					candidate[2] = candidate[2] + radius;
				else if (step == 3)
					candidate[2] = candidate[2] - radius;
				else if (step == 4)
				{
					candidate[0] = candidate[0] + radius;
					candidate[2] = candidate[2] + radius;
				}
				else if (step == 5)
				{
					candidate[0] = candidate[0] - radius;
					candidate[2] = candidate[2] + radius;
				}
				else if (step == 6)
				{
					candidate[0] = candidate[0] + radius;
					candidate[2] = candidate[2] - radius;
				}
				else
				{
					candidate[0] = candidate[0] - radius;
					candidate[2] = candidate[2] - radius;
				}

				if (TryResolveVehicleSpawnPositionAt(candidate, resolved, rejectWater, sampleRadius, maxSlopeDelta))
					return true;
			}
		}

		return false;
	}

	static GenericEntity SpawnPrefab(string prefab, vector position, vector angles)
	{
		if (prefab.IsEmpty())
			return null;

		ResourceName resourceName = prefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded || !loaded.IsValid())
			return null;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = position;
		IEntity entity = GetGame().SpawnEntityPrefabEx(resourceName, false, world, params);
		GenericEntity genericEntity = GenericEntity.Cast(entity);
		if (genericEntity)
		{
			genericEntity.SetOrigin(position);
			genericEntity.SetAngles(BuildEntitySetAnglesFromYawVector(angles));
		}

		return genericEntity;
	}

	static vector BuildUprightAngles(float yawDegrees)
	{
		vector angles;
		angles[0] = NormalizeYaw(yawDegrees);
		angles[1] = 0;
		angles[2] = 0;
		return angles;
	}

	static vector BuildUprightAnglesFromVector(vector sourceAngles)
	{
		return BuildUprightAngles(sourceAngles[0]);
	}

	static void ApplyUprightEntityTransform(IEntity entity, vector position, vector angles)
	{
		if (!entity)
			return;

		vector uprightAngles = BuildUprightAnglesFromVector(angles);
		entity.SetOrigin(position);
		entity.SetAngles(BuildEntitySetAnglesFromYawVector(uprightAngles));
	}

	static vector BuildEntitySetAnglesFromYawVector(vector sourceAngles)
	{
		return BuildEntitySetAnglesFromYaw(sourceAngles[0]);
	}

	static vector BuildEntitySetAnglesFromYaw(float yawDegrees)
	{
		vector angles;
		angles[0] = 0;
		angles[1] = NormalizeYaw(yawDegrees);
		angles[2] = 0;
		return angles;
	}

	static float NormalizeYaw(float yawDegrees)
	{
		float yaw = yawDegrees;
		while (yaw < 0)
			yaw = yaw + 360.0;
		while (yaw >= 360.0)
			yaw = yaw - 360.0;
		return yaw;
	}

	static bool IsDryGroundPosition(vector source)
	{
		vector resolved;
		return TryResolveGroundPosition(source, HQ_GROUND_OFFSET, resolved, true);
	}

	static bool IsPositionInsidePlayerEventBubble(vector position)
	{
		return IsPositionNearLivingPlayer(position, PLAYER_EVENT_BUBBLE_RADIUS_METERS);
	}

	static bool IsPositionNearLivingPlayer(vector position, float radiusMeters)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		float radiusSq = radiusMeters * radiusMeters;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			if (DistanceSq2D(playerEntity.GetOrigin(), position) <= radiusSq)
				return true;
		}

		return false;
	}

	protected static bool TryResolveSafeGroundPositionAt(vector source, float verticalOffset, out vector resolved, bool rejectWater, float clearanceMeters)
	{
		if (!TryResolveGroundPosition(source, verticalOffset, resolved, rejectWater))
			return false;

		if (!IsSafeSlope(resolved, Math.Max(SAFE_SAMPLE_RADIUS_METERS, clearanceMeters * 0.5)))
			return false;

		return true;
	}

	protected static bool TryResolveVehicleSpawnPositionAt(vector source, out vector resolved, bool rejectWater, float sampleRadius, float maxSlopeDelta)
	{
		if (!TryResolveGroundPosition(source, VEHICLE_GROUND_OFFSET, resolved, rejectWater))
			return false;

		if (IsLikelyOpenWater(resolved))
			return false;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		float slopeDelta = ResolveSurfaceDelta(world, resolved, sampleRadius);
		return slopeDelta <= maxSlopeDelta;
	}

	protected static bool TryResolveRoadNetworkManager(out RoadNetworkManager roadManager, out string reason)
	{
		roadManager = null;
		reason = "";
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
		{
			reason = "AI world unavailable for road network";
			return false;
		}

		roadManager = aiWorld.GetRoadNetworkManager();
		if (!roadManager)
		{
			reason = "RoadNetworkManager unavailable";
			return false;
		}

		reason = "RoadNetworkManager available";
		return true;
	}

	protected static bool IsRoadVehicleFootprintInsideSegmentWidth(vector center, vector forward, vector segmentStart, vector segmentEnd, float roadWidth)
	{
		float halfWidth = roadWidth * 0.5 + ROAD_VEHICLE_FOOTPRINT_MARGIN_METERS;
		if (halfWidth <= 0)
			return false;

		float forwardLength = Math.Sqrt(forward[0] * forward[0] + forward[2] * forward[2]);
		if (forwardLength <= 0.01)
			return false;

		float forwardX = forward[0] / forwardLength;
		float forwardZ = forward[2] / forwardLength;
		float rightX = forwardZ;
		float rightZ = -forwardX;
		if (!IsRoadFootprintSampleInsideWidth(center, segmentStart, segmentEnd, 0, 0, rightX, rightZ, forwardX, forwardZ, halfWidth))
			return false;
		if (!IsRoadFootprintSampleInsideWidth(center, segmentStart, segmentEnd, ROAD_VEHICLE_FOOTPRINT_HALF_WIDTH_METERS, 0, rightX, rightZ, forwardX, forwardZ, halfWidth))
			return false;
		if (!IsRoadFootprintSampleInsideWidth(center, segmentStart, segmentEnd, -ROAD_VEHICLE_FOOTPRINT_HALF_WIDTH_METERS, 0, rightX, rightZ, forwardX, forwardZ, halfWidth))
			return false;
		if (!IsRoadFootprintSampleInsideWidth(center, segmentStart, segmentEnd, 0, ROAD_VEHICLE_FOOTPRINT_HALF_LENGTH_METERS, rightX, rightZ, forwardX, forwardZ, halfWidth))
			return false;
		if (!IsRoadFootprintSampleInsideWidth(center, segmentStart, segmentEnd, 0, -ROAD_VEHICLE_FOOTPRINT_HALF_LENGTH_METERS, rightX, rightZ, forwardX, forwardZ, halfWidth))
			return false;

		return true;
	}

	protected static bool IsRoadFootprintSampleInsideWidth(vector center, vector segmentStart, vector segmentEnd, float sideOffset, float forwardOffset, float rightX, float rightZ, float forwardX, float forwardZ, float allowedDistance)
	{
		vector sample = center;
		sample[0] = center[0] + rightX * sideOffset + forwardX * forwardOffset;
		sample[2] = center[2] + rightZ * sideOffset + forwardZ * forwardOffset;
		vector closest = ClosestPointOnSegment2D(segmentStart, segmentEnd, sample);
		return DistanceSq2D(sample, closest) <= allowedDistance * allowedDistance;
	}

	protected static vector BuildRoadSegmentForward(vector segmentStart, vector segmentEnd, vector roadPosition, vector destination)
	{
		vector forward = BuildRoadForward(segmentStart, segmentEnd);
		float toDestinationX = destination[0] - roadPosition[0];
		float toDestinationZ = destination[2] - roadPosition[2];
		float dot = forward[0] * toDestinationX + forward[2] * toDestinationZ;
		if (dot < 0)
		{
			forward[0] = -forward[0];
			forward[2] = -forward[2];
		}

		return forward;
	}

	protected static vector BuildRoadForward(vector source, vector destination)
	{
		vector forward = "0 0 1";
		float dx = destination[0] - source[0];
		float dz = destination[2] - source[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length <= 0.01)
			return forward;

		forward[0] = dx / length;
		forward[1] = 0;
		forward[2] = dz / length;
		return forward;
	}

	protected static vector ClosestPointOnSegment2D(vector segmentStart, vector segmentEnd, vector point)
	{
		vector result = segmentStart;
		float dx = segmentEnd[0] - segmentStart[0];
		float dz = segmentEnd[2] - segmentStart[2];
		float lengthSq = dx * dx + dz * dz;
		if (lengthSq <= 0.01)
			return result;

		float t = ((point[0] - segmentStart[0]) * dx + (point[2] - segmentStart[2]) * dz) / lengthSq;
		if (t < 0.0)
			t = 0.0;
		else if (t > 1.0)
			t = 1.0;

		result[0] = segmentStart[0] + dx * t;
		result[1] = segmentStart[1] + (segmentEnd[1] - segmentStart[1]) * t;
		result[2] = segmentStart[2] + dz * t;
		return result;
	}

	protected static bool TryReadDrySurfaceY(BaseWorld world, float x, float z, out float y)
	{
		y = 0;
		if (!world)
			return false;

		y = world.GetSurfaceY(x, z);
		if (y < MIN_DRY_SURFACE_Y)
			return false;

		vector waterProbe = Vector(x, y + HQ_GROUND_OFFSET, z);
		return !IsWaterAtPositionInWorld(world, waterProbe);
	}

	protected static bool IsWaterAtPositionInWorld(BaseWorld world, vector position)
	{
		if (!world)
			return false;

		return ChimeraWorldUtils.TryGetWaterSurfaceSimple(world, position);
	}

	protected static bool TryReadVehicleFootprintSample(BaseWorld world, vector source, float sideX, float sideZ, float forwardX, float forwardZ, float sideOffset, float forwardOffset, out float y)
	{
		y = 0;
		float x = source[0] + sideX * sideOffset + forwardX * forwardOffset;
		float z = source[2] + sideZ * sideOffset + forwardZ * forwardOffset;
		return TryReadDrySurfaceY(world, x, z, y);
	}

	protected static IEntity GetBestPlayerEntity(PlayerManager playerManager, int playerId)
	{
		if (!playerManager || playerId <= 0)
			return null;

		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (controlledEntity)
			return controlledEntity;

		return SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
	}

	protected static bool IsLivingPlayerEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected static float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}

	protected static bool IsSafeSlope(vector resolved, float sampleRadius)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		float centerY = world.GetSurfaceY(resolved[0], resolved[2]);
		float maxDelta = 0;
		float y = world.GetSurfaceY(resolved[0] + sampleRadius, resolved[2]);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		y = world.GetSurfaceY(resolved[0] - sampleRadius, resolved[2]);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		y = world.GetSurfaceY(resolved[0], resolved[2] + sampleRadius);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		y = world.GetSurfaceY(resolved[0], resolved[2] - sampleRadius);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		return maxDelta <= MAX_SAFE_SLOPE_DELTA_METERS;
	}

	protected static float ResolveSurfaceDelta(BaseWorld world, vector source, float sampleRadius)
	{
		if (!world)
			return 9999.0;

		float centerY = world.GetSurfaceY(source[0], source[2]);
		float maxDelta = 0;
		float y = world.GetSurfaceY(source[0] + sampleRadius, source[2]);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		y = world.GetSurfaceY(source[0] - sampleRadius, source[2]);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		y = world.GetSurfaceY(source[0], source[2] + sampleRadius);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		y = world.GetSurfaceY(source[0], source[2] - sampleRadius);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		y = world.GetSurfaceY(source[0] + sampleRadius, source[2] + sampleRadius);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		y = world.GetSurfaceY(source[0] - sampleRadius, source[2] + sampleRadius);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		y = world.GetSurfaceY(source[0] + sampleRadius, source[2] - sampleRadius);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		y = world.GetSurfaceY(source[0] - sampleRadius, source[2] - sampleRadius);
		maxDelta = Math.Max(maxDelta, AbsFloat(y - centerY));
		return maxDelta;
	}

	protected static float AbsFloat(float value)
	{
		if (value < 0)
			return -value;

		return value;
	}
}
