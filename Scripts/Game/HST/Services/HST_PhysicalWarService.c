class HST_PhysicalWarService
{
	bool UpdateZoneActivation(HST_CampaignState state, HST_BalanceConfig balance)
	{
		if (!state || !balance)
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		bool changed;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			bool shouldBeActive = IsAnyPlayerNearZone(playerManager, playerIds, zone, balance);
			if (zone.m_bActive == shouldBeActive)
				continue;

			zone.m_bActive = shouldBeActive;
			if (shouldBeActive)
				ActivateZone(state, zone);
			else
				DeactivateZone(zone);

			changed = true;
			Print(string.Format("h-istasi | zone %1 physical activation = %2", zone.m_sZoneId, shouldBeActive));
		}

		return changed;
	}

	protected bool IsAnyPlayerNearZone(PlayerManager playerManager, array<int> playerIds, HST_ZoneState zone, HST_BalanceConfig balance)
	{
		if (!zone)
			return false;

		float radius = zone.m_iActivationRadiusMeters;
		if (radius <= 0)
			radius = balance.m_iActivationRadiusMeters;

		float radiusSq = radius * radius;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = playerManager.GetPlayerControlledEntity(playerId);
			if (!playerEntity)
				continue;

			if (DistanceSq2D(playerEntity.GetOrigin(), zone.m_vPosition) <= radiusSq)
				return true;
		}

		return false;
	}

	protected void ActivateZone(HST_CampaignState state, HST_ZoneState zone)
	{
		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, zone.m_sOwnerFactionKey);
		if (!garrison)
		{
			zone.m_iActiveInfantryCount = 0;
			zone.m_iActiveVehicleCount = 0;
			return;
		}

		zone.m_iActiveInfantryCount = garrison.m_iInfantryCount;
		zone.m_iActiveVehicleCount = garrison.m_iVehicleCount;
	}

	protected void DeactivateZone(HST_ZoneState zone)
	{
		zone.m_iActiveInfantryCount = 0;
		zone.m_iActiveVehicleCount = 0;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
