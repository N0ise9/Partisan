class HST_EnemyCommanderService
{
	static const int ORDER_TICK_SECONDS = 180;
	static const int ORDER_RESOLVE_SECONDS = 420;
	protected int m_iOrderAccumulatorSeconds;

	bool Tick(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService support, HST_GarrisonService garrisons, int elapsedSeconds)
	{
		if (!state || !preset || !enemyDirector || elapsedSeconds <= 0)
			return false;

		m_iOrderAccumulatorSeconds += elapsedSeconds;
		bool changed = ResolveOrders(state, garrisons);
		if (m_iOrderAccumulatorSeconds < ORDER_TICK_SECONDS)
			return changed;

		m_iOrderAccumulatorSeconds -= ORDER_TICK_SECONDS;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || pool.m_sFactionKey == preset.m_sResistanceFactionKey)
				continue;

			HST_ZoneState targetZone = SelectTargetZone(state, preset, pool.m_sFactionKey);
			if (!targetZone)
				continue;

			if (HasActiveOrderForZone(state, pool.m_sFactionKey, targetZone.m_sZoneId))
				continue;

			HST_EEnemyOrderType orderType = SelectOrderType(state, targetZone, pool);
			if (QueueOrder(state, preset, enemyDirector, support, pool.m_sFactionKey, targetZone, orderType))
				changed = true;
		}

		return changed;
	}

	string BuildEnemyOrderReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi enemy commander | state not ready";

		int queued;
		int active;
		int resolved;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order)
				continue;

			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED)
				queued++;
			else if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				active++;
			else if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED)
				resolved++;
		}

		return string.Format("h-istasi enemy commander | queued %1 | active %2 | resolved %3", queued, active, resolved);
	}

	protected bool QueueOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService support, string factionKey, HST_ZoneState targetZone, HST_EEnemyOrderType orderType)
	{
		int attackCost;
		int supportCost;
		ResolveOrderCosts(orderType, attackCost, supportCost);
		if (!enemyDirector.CanAfford(state, factionKey, attackCost, supportCost))
			return false;

		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = string.Format("order_%1_%2_%3", factionKey, state.m_iElapsedSeconds, state.m_aEnemyOrders.Count());
		order.m_sFactionKey = factionKey;
		order.m_eType = orderType;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED;
		order.m_sTargetZoneId = targetZone.m_sZoneId;
		order.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		order.m_iResolveAtSecond = state.m_iElapsedSeconds + ORDER_RESOLVE_SECONDS;
		order.m_iAttackCost = attackCost;
		order.m_iSupportCost = supportCost;

		HST_ESupportRequestType supportType = SupportTypeForOrder(state, targetZone, orderType);
		if (support && supportType != HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
		{
			HST_SupportRequestState request = support.RequestSupport(state, preset, null, enemyDirector, factionKey, supportType, targetZone.m_sZoneId, false);
			if (request)
				order.m_sSupportRequestId = request.m_sRequestId;
		}
		else if (!enemyDirector.TrySpend(state, factionKey, attackCost, supportCost))
		{
			return false;
		}

		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		state.m_aEnemyOrders.Insert(order);
		Print(string.Format("h-istasi | enemy order %1 active at %2", order.m_sOrderId, targetZone.m_sZoneId));
		return true;
	}

	protected bool ResolveOrders(HST_CampaignState state, HST_GarrisonService garrisons)
	{
		bool changed;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;

			if (state.m_iElapsedSeconds < order.m_iResolveAtSecond)
				continue;

			order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
			ApplyResolvedOrder(state, garrisons, order);
			changed = true;
		}

		return changed;
	}

	protected void ApplyResolvedOrder(HST_CampaignState state, HST_GarrisonService garrisons, HST_EnemyOrderState order)
	{
		if (!garrisons || !order)
			return;

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
			garrisons.AddAbstractForces(state, order.m_sTargetZoneId, order.m_sFactionKey, 2 + state.m_iWarLevel, 0);

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
		{
			HST_CivilianZoneState civilianZone = state.FindCivilianZone(order.m_sTargetZoneId);
			if (civilianZone)
				civilianZone.m_iRoadblockPresence = Math.Min(3, civilianZone.m_iRoadblockPresence + 1);
		}
	}

	protected HST_ZoneState SelectTargetZone(HST_CampaignState state, HST_CampaignPreset preset, string factionKey)
	{
		HST_ZoneState bestZone;
		int bestScore = -9999;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			int score = ScoreTargetZone(state, preset, zone, factionKey);

			if (score > bestScore)
			{
				bestZone = zone;
				bestScore = score;
			}
		}

		return bestZone;
	}

	protected HST_EEnemyOrderType SelectOrderType(HST_CampaignState state, HST_ZoneState targetZone, HST_FactionPoolState pool)
	{
		if (IsHQThreatZone(state, targetZone) && pool.m_iAttackResources >= 25 && pool.m_iSupportResources >= 8 && state.m_iWarLevel >= 4)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK;

		if (targetZone.m_sOwnerFactionKey == "FIA")
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK;

		if (targetZone.m_iResistanceCaptureProgress > 0)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF;

		HST_GarrisonState garrison = state.FindGarrison(targetZone.m_sZoneId, pool.m_sFactionKey);
		if ((!garrison || garrison.m_iInfantryCount < Math.Max(2, state.m_iWarLevel)) && pool.m_iSupportResources >= 10)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON;

		if (targetZone.m_eType == HST_EZoneType.HST_ZONE_TOWN && pool.m_iSupportResources >= 12)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK;

		if (pool.m_iAttackResources >= 20 && state.m_iWarLevel >= 3)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL;

		return HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL;
	}

	protected int ScoreTargetZone(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState zone, string factionKey)
	{
		int score;
		if (zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			score += 22;
		else if (zone.m_sOwnerFactionKey == factionKey)
			score += 10;
		else
			score += 4;

		score += zone.m_iPriority;
		score += zone.m_iResistanceCaptureProgress / 5;
		if (zone.m_bActive)
			score += 12;
		if (zone.m_iSupport > 25)
			score += 4 + zone.m_iSupport / 20;
		if (HasActiveMissionNearZone(state, zone))
			score += 10;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			score += 8;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST || zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			score += 6;
		if (IsHQThreatZone(state, zone))
			score += 9 + state.m_iWarLevel;

		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, factionKey);
		if (!garrison)
			score += 5;
		else if (garrison.m_iInfantryCount < Math.Max(2, state.m_iWarLevel))
			score += 4;

		return score;
	}

	protected bool HasActiveMissionNearZone(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return false;

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (mission.m_sTargetZoneId == zone.m_sZoneId)
				return true;
			if (DistanceSq2D(mission.m_vTargetPosition, zone.m_vPosition) < 450000)
				return true;
		}

		return false;
	}

	protected bool IsHQThreatZone(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, zone.m_vPosition) <= 1440000;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}

	protected HST_ESupportRequestType SupportTypeForOrder(HST_CampaignState state, HST_ZoneState targetZone, HST_EEnemyOrderType orderType)
	{
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF || orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
			return HST_ESupportRequestType.HST_SUPPORT_QRF;

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL || orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
		{
			if (state && state.m_iWarLevel >= 6 && targetZone && (targetZone.m_sOwnerFactionKey == "FIA" || targetZone.m_bActive))
				return HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55;

			if (state && state.m_iWarLevel >= 3)
				return HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK;

			return HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
		}

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
			return HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP;

		return HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP;
	}

	protected void ResolveOrderCosts(HST_EEnemyOrderType orderType, out int attackCost, out int supportCost)
	{
		attackCost = 8;
		supportCost = 4;

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF || orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
		{
			attackCost = 15;
			supportCost = 5;
			return;
		}

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL || orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
		{
			attackCost = 20;
			supportCost = 8;
			return;
		}

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
		{
			attackCost = 4;
			supportCost = 10;
		}
	}

	protected bool HasActiveOrderForZone(HST_CampaignState state, string factionKey, string zoneId)
	{
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_sFactionKey != factionKey || order.m_sTargetZoneId != zoneId)
				continue;

			if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED || order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				return true;
		}

		return false;
	}
}
