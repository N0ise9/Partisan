class HST_WorldPositionService
{
	static const float HQ_GROUND_OFFSET = 0.1;
	static const float CHARACTER_GROUND_OFFSET = 0.25;
	static const float PROP_GROUND_OFFSET = 0.05;
	static const float MIN_DRY_SURFACE_Y = 1.0;
	static const float TERRAIN_NOT_READY_SURFACE_Y = 0.01;

	static bool TryResolveGroundPosition(vector source, float verticalOffset, out vector resolved, bool rejectWater = false)
	{
		resolved = source;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		float surfaceY = world.GetSurfaceY(source[0], source[2]);
		if (rejectWater && surfaceY < MIN_DRY_SURFACE_Y)
		{
			if (surfaceY <= TERRAIN_NOT_READY_SURFACE_Y && source[1] <= MIN_DRY_SURFACE_Y)
			{
				resolved[1] = source[1] + verticalOffset;
				return true;
			}

			return false;
		}

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

	static bool IsDryGroundPosition(vector source)
	{
		vector resolved;
		return TryResolveGroundPosition(source, HQ_GROUND_OFFSET, resolved, true);
	}
}
