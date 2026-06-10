class HST_ZoneCaptureService
{
	static const int CAPTURE_PROGRESS_REQUIRED = 100;
	static const int CAPTURE_PROGRESS_PER_SECOND = 2;
	static const int CAPTURE_DECAY_PER_SECOND = 1;
	static const int TOWN_SUPPORT_CAPTURE_THRESHOLD = 60;

	bool CaptureZone(HST_CampaignState state, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, string factionKey, string resistanceFactionKey = "FIA")
	{
		if (!state || !strategic || !economy || !balance)
			return false;

		return strategic.SetZoneOwner(state, economy, balance, zoneId, factionKey, resistanceFactionKey);
	}

	bool CaptureForResistance(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, int supportReward)
	{
		if (!state || !preset || !strategic || !economy || !balance)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return false;

		string oldOwner = zone.m_sOwnerFactionKey;
		if (!CaptureZone(state, strategic, economy, balance, zoneId, preset.m_sResistanceFactionKey, preset.m_sResistanceFactionKey))
			return false;

		if (!oldOwner.IsEmpty() && oldOwner != preset.m_sResistanceFactionKey)
			economy.AddAggression(state, oldOwner, 10 + Math.Max(0, zone.m_iPriority / 5));

		if (zone && zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			zone.m_iSupport = Math.Max(-100, Math.Min(100, zone.m_iSupport + supportReward));

		return true;
	}

	bool AddResistanceCaptureProgress(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, int amount, int supportReward = 10)
	{
		if (!state || !preset || amount <= 0)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone || zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			return false;

		zone.m_iResistanceCaptureProgress = Math.Min(CAPTURE_PROGRESS_REQUIRED, Math.Max(0, zone.m_iResistanceCaptureProgress + amount));
		Print(string.Format("h-istasi | zone %1 resistance capture progress %2/%3", zoneId, zone.m_iResistanceCaptureProgress, CAPTURE_PROGRESS_REQUIRED));
		if (zone.m_iResistanceCaptureProgress < CAPTURE_PROGRESS_REQUIRED)
			return true;

		return CaptureForResistance(state, preset, strategic, economy, balance, zoneId, supportReward);
	}

	bool TickContestedCapture(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, int elapsedSeconds)
	{
		if (!state || !preset || !strategic || !economy || !balance || elapsedSeconds <= 0)
			return false;

		bool changed;
		string resistanceFactionKey = preset.m_sResistanceFactionKey;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey == resistanceFactionKey || zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
				continue;

			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			{
				if (zone.m_iSupport >= TOWN_SUPPORT_CAPTURE_THRESHOLD)
					changed = CaptureForResistance(state, preset, strategic, economy, balance, zone.m_sZoneId, 0) || changed;
				continue;
			}

			if (!IsDirectlyCapturableZone(zone))
				continue;

			if (!HasLivingPlayerInCaptureRadius(zone, balance))
			{
				changed = DecayCaptureProgress(zone, elapsedSeconds) || changed;
				continue;
			}

			if (HasHostilePresence(state, zone, resistanceFactionKey))
			{
				changed = DecayCaptureProgress(zone, elapsedSeconds) || changed;
				continue;
			}

			changed = AddResistanceCaptureProgress(state, preset, strategic, economy, balance, zone.m_sZoneId, elapsedSeconds * CAPTURE_PROGRESS_PER_SECOND, 5) || changed;
		}

		return changed;
	}

	protected bool DecayCaptureProgress(HST_ZoneState zone, int elapsedSeconds)
	{
		if (!zone || zone.m_iResistanceCaptureProgress <= 0)
			return false;

		int nextProgress = Math.Max(0, zone.m_iResistanceCaptureProgress - elapsedSeconds * CAPTURE_DECAY_PER_SECOND);
		if (nextProgress == zone.m_iResistanceCaptureProgress)
			return false;

		zone.m_iResistanceCaptureProgress = nextProgress;
		return true;
	}

	protected bool IsDirectlyCapturableZone(HST_ZoneState zone)
	{
		if (!zone)
			return false;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_POLICE_STATION)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_BANK)
			return true;

		return false;
	}

	protected bool HasHostilePresence(HST_CampaignState state, HST_ZoneState zone, string resistanceFactionKey)
	{
		if (HasHostileActiveGroup(state, zone, resistanceFactionKey))
			return true;

		if (zone.m_bActive && HadPhysicalHostileGroupAtZone(state, zone, resistanceFactionKey))
			return false;

		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, zone.m_sOwnerFactionKey);
		if (!garrison || garrison.m_sFactionKey == resistanceFactionKey)
			return false;

		return garrison.m_iInfantryCount > 0 || garrison.m_iVehicleCount > 0;
	}

	protected bool HadPhysicalHostileGroupAtZone(HST_CampaignState state, HST_ZoneState zone, string resistanceFactionKey)
	{
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sZoneId != zone.m_sZoneId || activeGroup.m_sFactionKey == resistanceFactionKey)
				continue;
			if (activeGroup.m_bSpawnAttempted)
				return true;
		}

		return false;
	}

	protected bool HasHostileActiveGroup(HST_CampaignState state, HST_ZoneState zone, string resistanceFactionKey)
	{
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sZoneId != zone.m_sZoneId || activeGroup.m_sFactionKey == resistanceFactionKey)
				continue;
			if (activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == "folded")
				continue;
			if (activeGroup.m_iLastSeenAliveCount > 0 || activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iSurvivorVehicleCount > 0)
				return true;
		}

		return false;
	}

	protected bool HasLivingPlayerInCaptureRadius(HST_ZoneState zone, HST_BalanceConfig balance)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		float radius = zone.m_iActivationRadiusMeters;
		if (radius <= 0)
			radius = balance.m_iActivationRadiusMeters;
		float radiusSq = radius * radius;

		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingEntity(playerEntity))
				continue;
			if (DistanceSq2D(playerEntity.GetOrigin(), zone.m_vPosition) <= radiusSq)
				return true;
		}

		return false;
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

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
