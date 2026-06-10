class HST_WorldPositionService
{
	static const float HQ_GROUND_OFFSET = 0.1;
	static const float CHARACTER_GROUND_OFFSET = 0.25;
	static const float PROP_GROUND_OFFSET = 0.05;
	static const float VEHICLE_GROUND_OFFSET = 0.45;
	static const float MIN_DRY_SURFACE_Y = 1.0;
	static const float MAX_SAFE_SLOPE_DELTA_METERS = 1.8;
	static const float SAFE_SAMPLE_RADIUS_METERS = 2.5;

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

	protected static bool TryResolveSafeGroundPositionAt(vector source, float verticalOffset, out vector resolved, bool rejectWater, float clearanceMeters)
	{
		if (!TryResolveGroundPosition(source, verticalOffset, resolved, rejectWater))
			return false;

		if (!IsSafeSlope(resolved, Math.Max(SAFE_SAMPLE_RADIUS_METERS, clearanceMeters * 0.5)))
			return false;

		return true;
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

	protected static float AbsFloat(float value)
	{
		if (value < 0)
			return -value;

		return value;
	}
}
