class HST_EnemyCommanderService
{
	static const int ORDER_TICK_SECONDS = 180;
	static const int ORDER_RESOLVE_SECONDS = 420;
	static const int PHYSICAL_ORDER_TIMEOUT_SECONDS = 300;
	protected int m_iOrderAccumulatorSeconds;

	bool Tick(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService support, HST_GarrisonService garrisons, int elapsedSeconds)
	{
		if (!state || !preset || !enemyDirector || elapsedSeconds <= 0)
			return false;

		m_iOrderAccumulatorSeconds += elapsedSeconds;
		bool changed = TickActiveOrderRuntime(state, preset, support, garrisons, enemyDirector);
		changed = ResolveOrders(state, preset, garrisons) || changed;
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

			RecordTargetPressureSignal(state, preset, enemyDirector, pool.m_sFactionKey, targetZone);
			HST_EEnemyOrderType orderType = SelectOrderType(state, preset, targetZone, pool);
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
		int aborted;
		int physicalized;
		int abstractResolved;
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
			else if (order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED)
				aborted++;

			if (order.m_bPhysicalized)
				physicalized++;
			if (order.m_bAbstractResolved)
				abstractResolved++;
		}

		string report = string.Format(
			"h-istasi enemy commander | queued %1 | active %2 | resolved %3 | aborted %4 | physicalized %5 | abstract %6",
			queued,
			active,
			resolved,
			aborted,
			physicalized,
			abstractResolved
		);

		int emitted;
		for (int i = state.m_aEnemyOrders.Count() - 1; i >= 0; i--)
		{
			HST_EnemyOrderState orderDetail = state.m_aEnemyOrders[i];
			if (!orderDetail)
				continue;

			string targetText = orderDetail.m_sTargetZoneId;
			if (orderDetail.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
				targetText = "resistance_base near " + orderDetail.m_sTargetZoneId;
			string detail = string.Format(
				"\n%1 | %2 | %3 | faction %4 | target %5 | runtime %6 | support %7 | group %8 | resolve %9",
				orderDetail.m_sOrderId,
				orderDetail.m_eType,
				orderDetail.m_eStatus,
				orderDetail.m_sFactionKey,
				targetText,
				orderDetail.m_sRuntimeStatus,
				orderDetail.m_sSupportRequestId,
				orderDetail.m_sGroupId,
				orderDetail.m_iResolveAtSecond
			);
			detail = detail + string.Format(
				" | result %1 | fail %2 | cost %3/%4",
				orderDetail.m_sResolutionKind,
				orderDetail.m_sFailureReason,
				orderDetail.m_iAttackCost,
				orderDetail.m_iSupportCost
			);
			if (!orderDetail.m_sCompositionIntentId.IsEmpty() || orderDetail.m_iCompositionManpower > 0 || orderDetail.m_iCompositionVehicleCount > 0)
				detail = detail + string.Format(" | composition %1 tier %2 compCost %3 manpower %4 vehicles %5 armed %6", orderDetail.m_sCompositionIntentId, orderDetail.m_sCompositionTier, orderDetail.m_iCompositionCost, orderDetail.m_iCompositionManpower, orderDetail.m_iCompositionVehicleCount, orderDetail.m_iCompositionArmedVehicleCount);
			if (!orderDetail.m_sCompositionFailureReason.IsEmpty())
				detail = detail + " | composition failure " + orderDetail.m_sCompositionFailureReason;
			report = report + detail;

			emitted++;
			if (emitted >= 12)
				break;
		}

		return report;
	}

	string BuildPhysicalResponseReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi enemy physical response | state not ready";

		int linkedSupport;
		int linkedGroups;
		int activePhysicalGroups;
		int abstractPending;
		int physicalPending;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order)
				continue;

			if (!order.m_sSupportRequestId.IsEmpty())
				linkedSupport++;
			if (!order.m_sGroupId.IsEmpty())
				linkedGroups++;
			if (order.m_bPhysicalized && order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				physicalPending++;
			if (!order.m_bPhysicalized && order.m_eStatus == HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				abstractPending++;
		}

		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!group)
				continue;

			if (group.m_sRuntimeStatus == "support_active" || group.m_sRuntimeStatus == "support_arrived")
				activePhysicalGroups++;
		}

		return string.Format(
			"h-istasi enemy physical response | support links %1 | group links %2 | support-active groups %3 | abstract pending %4 | physical pending %5",
			linkedSupport,
			linkedGroups,
			activePhysicalGroups,
			abstractPending,
			physicalPending
		);
	}

	bool TryQueueImmediateCounterattack(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService support, string factionKey, HST_ZoneState capturedZone, int chancePercent)
	{
		if (!state || !preset || !enemyDirector || !capturedZone || factionKey.IsEmpty())
			return false;

		if (HasActiveOrderForZone(state, factionKey, capturedZone.m_sZoneId))
		{
			Print(string.Format("h-istasi capture | counterattack skipped for %1 at %2 | active order already exists", factionKey, capturedZone.m_sZoneId));
			return false;
		}

		int attackCost;
		int supportCost;
		ResolveOrderCosts(HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK, attackCost, supportCost);
		string spendReason;
		if (!enemyDirector.CanSpendDefense(state, capturedZone, factionKey, attackCost, supportCost, spendReason))
		{
			Print(string.Format("h-istasi capture | counterattack skipped for %1 at %2 | %3", factionKey, capturedZone.m_sZoneId, spendReason));
			return false;
		}

		int chance = Math.Max(0, Math.Min(100, chancePercent));
		int rollSeed = state.m_iCampaignSeed + state.m_iElapsedSeconds * 17 + capturedZone.m_sZoneId.Length() * 101 + capturedZone.m_sDisplayName.Length() * 29 + factionKey.Length() * 31 + state.m_aEnemyOrders.Count() * 13 + state.m_iWarLevel * 19 + capturedZone.m_iPriority * 23;
		rollSeed += Math.Round(capturedZone.m_vPosition[0]) + Math.Round(capturedZone.m_vPosition[2]);
		int roll = HST_DefaultCatalog.PositiveMod(rollSeed, 100);
		if (roll >= chance)
		{
			Print(string.Format("h-istasi capture | counterattack skipped for %1 at %2 | roll %3 chance %4", factionKey, capturedZone.m_sZoneId, roll, chance));
			return false;
		}

		bool queued = QueueOrder(state, preset, enemyDirector, support, factionKey, capturedZone, HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK);
		if (queued)
			Print(string.Format("h-istasi capture | counterattack queued for %1 at %2 | roll %3 chance %4", factionKey, capturedZone.m_sZoneId, roll, chance));
		else
			Print(string.Format("h-istasi capture | counterattack failed for %1 at %2 after chance pass", factionKey, capturedZone.m_sZoneId));

		return queued;
	}

	HST_EnemyOrderState QueueDebugOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ZoneState targetZone, HST_EEnemyOrderType orderType)
	{
		if (!state || !preset || !enemyDirector || !targetZone || factionKey.IsEmpty())
			return null;

		if (!state.FindFactionPool(factionKey))
			return null;

		enemyDirector.AddResources(state, factionKey, 100, 100);
		int beforeCount = state.m_aEnemyOrders.Count();
		if (!QueueOrder(state, preset, enemyDirector, null, factionKey, targetZone, orderType))
			return null;

		if (state.m_aEnemyOrders.Count() <= beforeCount)
			return null;

		return state.m_aEnemyOrders[state.m_aEnemyOrders.Count() - 1];
	}

	HST_EnemyOrderState QueueDebugPetrosAttack(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, string factionKey)
	{
		if (!state || !preset || !enemyDirector || factionKey.IsEmpty())
			return null;

		enemyDirector.AddResources(state, factionKey, 100, 100);
		return QueuePetrosAttack(state, preset, enemyDirector, factionKey);
	}

	HST_EnemyOrderState QueuePetrosAttack(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, string factionKey)
	{
		if (!state || !preset || !enemyDirector || factionKey.IsEmpty())
			return null;

		HST_ZoneState targetZone = ResolvePetrosAttackTargetZone(state, preset);
		if (!targetZone)
			return null;

		if (HasActiveOrderForZone(state, factionKey, targetZone.m_sZoneId))
			return null;

		int beforeCount = state.m_aEnemyOrders.Count();
		if (!QueueOrder(state, preset, enemyDirector, null, factionKey, targetZone, HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK))
			return null;
		if (state.m_aEnemyOrders.Count() <= beforeCount)
			return null;

		HST_EnemyOrderState order = state.m_aEnemyOrders[state.m_aEnemyOrders.Count() - 1];
		order.m_vTargetPosition = ResolvePetrosAttackTargetPosition(state);
		order.m_sRuntimeStatus = "petros_attack_ordered";
		return order;
	}

	int DebugResolveDueOrdersNow(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons)
	{
		if (!state)
			return 0;

		int resolved;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;

			order.m_iResolveAtSecond = state.m_iElapsedSeconds;
			resolved++;
		}

		ResolveOrders(state, preset, garrisons);
		return resolved;
	}

	bool DebugResolveOrderNow(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, string orderId)
	{
		HST_EnemyOrderState order = FindOrderForDebug(state, orderId);
		if (!order || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return false;

		order.m_iResolveAtSecond = state.m_iElapsedSeconds;
		if (order.m_bPhysicalized)
			order.m_iResolveAtSecond = Math.Max(0, state.m_iElapsedSeconds - PHYSICAL_ORDER_TIMEOUT_SECONDS);

		return ResolveOrderNow(state, preset, garrisons, order);
	}

	bool DebugApplySurvivorRefund(HST_CampaignState state, HST_EnemyDirectorService enemyDirector, HST_EnemyOrderState order, HST_ActiveGroupState group)
	{
		return ApplySurvivorRefund(state, enemyDirector, order, group);
	}

	protected bool QueueOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService support, string factionKey, HST_ZoneState targetZone, HST_EEnemyOrderType orderType)
	{
		if (!state || !preset || !enemyDirector || !targetZone || factionKey.IsEmpty())
			return false;

		int attackCost;
		int supportCost;
		ResolveOrderCosts(orderType, attackCost, supportCost);
		string spendReason;
		if (!enemyDirector.TrySpendDefense(state, targetZone, factionKey, attackCost, supportCost, spendReason))
		{
			Print(string.Format("h-istasi enemy commander | order skipped for %1 at %2 type %3 | %4", factionKey, targetZone.m_sZoneId, orderType, spendReason));
			return false;
		}

		HST_EnemyOrderState order = new HST_EnemyOrderState();
		order.m_sOrderId = string.Format("order_%1_%2_%3", factionKey, state.m_iElapsedSeconds, state.m_aEnemyOrders.Count());
		order.m_sFactionKey = factionKey;
		order.m_eType = orderType;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_QUEUED;
		order.m_sTargetZoneId = targetZone.m_sZoneId;
		order.m_sRuntimeStatus = "active_abstract_pending";
		order.m_vTargetPosition = targetZone.m_vPosition;
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			order.m_vTargetPosition = ResolvePetrosAttackTargetPosition(state);
		order.m_vSourcePosition = ResolveOrderSourcePosition(state, preset, factionKey, targetZone);
		order.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		order.m_iResolveAtSecond = state.m_iElapsedSeconds + ORDER_RESOLVE_SECONDS;
		order.m_iAttackCost = attackCost;
		order.m_iSupportCost = supportCost;
		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE;
		state.m_aEnemyOrders.Insert(order);
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			state.m_iLastHQAttackSecond = state.m_iElapsedSeconds;
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			Print(string.Format("h-istasi | enemy order %1 active against Petros/HQ near %2 at %3", order.m_sOrderId, targetZone.m_sZoneId, order.m_vTargetPosition));
		else
			Print(string.Format("h-istasi | enemy order %1 active at %2", order.m_sOrderId, targetZone.m_sZoneId));
		return true;
	}

	protected bool TickActiveOrderRuntime(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestService support, HST_GarrisonService garrisons, HST_EnemyDirectorService enemyDirector)
	{
		bool changed;
		if (!state)
			return false;

		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;

			if (ShouldPhysicalizeOrder(state, preset, order))
				changed = TryPhysicalizeOrder(state, preset, support, order) || changed;

			changed = SyncPhysicalizedOrder(state, order, enemyDirector) || changed;
		}

		return changed;
	}

	protected bool IsPhysicalizableOrderType(HST_EEnemyOrderType orderType)
	{
		return orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF
			|| orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK
			|| orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL
			|| orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK;
	}

	protected bool ShouldPhysicalizeOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyOrderState order)
	{
		if (!state || !preset || !order || order.m_bPhysicalized || !IsPhysicalizableOrderType(order.m_eType))
			return false;

		if (order.m_sRuntimeStatus == "physicalize_failed")
			return false;

		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		if (!targetZone)
			return false;

		if (targetZone.m_bActive)
			return true;

		if (HasActiveMissionNearZone(state, targetZone))
			return true;

		if (HasActiveObjectiveNearZone(state, targetZone))
			return true;

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK && IsPetrosAttackTargetInsidePlayerEventBubble(state))
			return true;

		vector targetPosition = order.m_vTargetPosition;
		if (targetPosition[0] == 0 && targetPosition[1] == 0 && targetPosition[2] == 0)
			targetPosition = targetZone.m_vPosition;

		return HST_WorldPositionService.IsPositionInsidePlayerEventBubble(targetPosition);
	}

	protected bool TryPhysicalizeOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestService support, HST_EnemyOrderState order)
	{
		if (!state || !preset || !support || !order || order.m_bPhysicalized)
			return false;

		HST_ESupportRequestType supportType = SupportTypeForOrder(state, preset, state.FindZone(order.m_sTargetZoneId), order.m_eType);
		HST_SupportRequestState request = support.RequestPrepaidEnemySupport(
			state,
			preset,
			order.m_sFactionKey,
			supportType,
			order.m_sTargetZoneId,
			order.m_vSourcePosition,
			order.m_vTargetPosition
		);

		if (!request)
		{
			order.m_sFailureReason = "physical support request could not be created";
			order.m_sRuntimeStatus = "physicalize_failed";
			return true;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
		{
			request.m_sAssetProfileId = request.m_sAssetProfileId + "_petros_attack";
			request.m_sRuntimeStatus = "petros_attack_support_queued";
		}

		order.m_sSupportRequestId = request.m_sRequestId;
		order.m_bPhysicalized = true;
		order.m_iPhysicalizedAtSecond = state.m_iElapsedSeconds;
		order.m_sRuntimeStatus = "physical_support_queued";
		Print(string.Format("h-istasi enemy commander | order %1 physicalized via support %2", order.m_sOrderId, order.m_sSupportRequestId));
		return true;
	}

	protected bool SyncPhysicalizedOrder(HST_CampaignState state, HST_EnemyOrderState order, HST_EnemyDirectorService enemyDirector)
	{
		if (!state || !order || !order.m_bPhysicalized || order.m_sSupportRequestId.IsEmpty())
			return false;

		HST_SupportRequestState request = FindSupportRequest(state, order.m_sSupportRequestId);
		if (!request)
		{
			if (order.m_sRuntimeStatus == "support_missing")
				return false;

			order.m_sFailureReason = "linked support request missing";
			order.m_sRuntimeStatus = "support_missing";
			return true;
		}

		bool changed;
		changed = SyncOrderCompositionFromSupportRequest(order, request) || changed;

		if (order.m_sGroupId != request.m_sGroupId && !request.m_sGroupId.IsEmpty())
		{
			order.m_sGroupId = request.m_sGroupId;
			order.m_sRuntimeStatus = "physical_group_linked";
			changed = true;
		}

		if (!request.m_sFailureReason.IsEmpty() && order.m_sFailureReason != request.m_sFailureReason)
		{
			order.m_sFailureReason = request.m_sFailureReason;
			changed = true;
		}

		if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
		{
			order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ABORTED;
			order.m_iResolvedAtSecond = state.m_iElapsedSeconds;
			order.m_sRuntimeStatus = "aborted_support_cancelled";
			order.m_sResolutionKind = "aborted";
			changed = true;
		}
		else if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED)
		{
			order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
			order.m_iResolvedAtSecond = state.m_iElapsedSeconds;
			order.m_sRuntimeStatus = "resolved_physical_support";
			order.m_sResolutionKind = "physical";
			changed = true;
		}

		if (!order.m_sGroupId.IsEmpty())
		{
			HST_ActiveGroupState group = state.FindActiveGroup(order.m_sGroupId);
			if (group && (group.m_sRuntimeStatus == "eliminated" || group.m_sRuntimeStatus == "folded" || group.m_sRuntimeStatus == "spawn_failed"))
			{
				changed = ApplySurvivorRefund(state, enemyDirector, order, group) || changed;
				order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
				order.m_iResolvedAtSecond = state.m_iElapsedSeconds;
				order.m_sRuntimeStatus = "resolved_group_" + group.m_sRuntimeStatus;
				order.m_sResolutionKind = "physical_group_terminal";
				changed = true;
			}
		}

		return changed;
	}

	protected bool ApplySurvivorRefund(HST_CampaignState state, HST_EnemyDirectorService enemyDirector, HST_EnemyOrderState order, HST_ActiveGroupState group)
	{
		if (!state || !enemyDirector || !order || !group || order.m_bResourceRefundApplied)
			return false;

		int survivorCount = Math.Max(0, group.m_iSurvivorInfantryCount) + Math.Max(0, group.m_iSurvivorVehicleCount);
		if (group.m_sRuntimeStatus != "folded" && survivorCount <= 0)
			return false;

		int attackRefund;
		int supportRefund;
		if (order.m_iAttackCost > 0)
			attackRefund = Math.Max(1, Math.Round(order.m_iAttackCost * 0.35));
		if (order.m_iSupportCost > 0)
			supportRefund = Math.Max(1, Math.Round(order.m_iSupportCost * 0.35));

		attackRefund = Math.Min(order.m_iAttackCost, attackRefund);
		supportRefund = Math.Min(order.m_iSupportCost, supportRefund);
		if (attackRefund <= 0 && supportRefund <= 0)
			return false;

		enemyDirector.RefundDefenseResources(state, order.m_sFactionKey, order.m_sTargetZoneId, attackRefund, supportRefund, "survivor fold-back");
		order.m_iRefundedAttackResources = attackRefund;
		order.m_iRefundedSupportResources = supportRefund;
		order.m_bResourceRefundApplied = true;
		return true;
	}

	protected bool SyncOrderCompositionFromSupportRequest(HST_EnemyOrderState order, HST_SupportRequestState request)
	{
		if (!order || !request)
			return false;

		bool changed;
		if (!request.m_sCompositionRequestId.IsEmpty() && order.m_sCompositionRequestId != request.m_sCompositionRequestId)
		{
			order.m_sCompositionRequestId = request.m_sCompositionRequestId;
			changed = true;
		}
		if (!request.m_sCompositionIntentId.IsEmpty() && order.m_sCompositionIntentId != request.m_sCompositionIntentId)
		{
			order.m_sCompositionIntentId = request.m_sCompositionIntentId;
			changed = true;
		}
		if (!request.m_sCompositionTier.IsEmpty() && order.m_sCompositionTier != request.m_sCompositionTier)
		{
			order.m_sCompositionTier = request.m_sCompositionTier;
			changed = true;
		}
		if (!request.m_sCompositionSummary.IsEmpty() && order.m_sCompositionSummary != request.m_sCompositionSummary)
		{
			order.m_sCompositionSummary = request.m_sCompositionSummary;
			changed = true;
		}
		if (!request.m_sCompositionFailureReason.IsEmpty() && order.m_sCompositionFailureReason != request.m_sCompositionFailureReason)
		{
			order.m_sCompositionFailureReason = request.m_sCompositionFailureReason;
			changed = true;
		}
		if (order.m_iCompositionCost != request.m_iCompositionCost)
		{
			order.m_iCompositionCost = request.m_iCompositionCost;
			changed = true;
		}
		if (order.m_iCompositionManpower != request.m_iCompositionManpower)
		{
			order.m_iCompositionManpower = request.m_iCompositionManpower;
			changed = true;
		}
		if (order.m_iCompositionVehicleCount != request.m_iCompositionVehicleCount)
		{
			order.m_iCompositionVehicleCount = request.m_iCompositionVehicleCount;
			changed = true;
		}
		if (order.m_iCompositionArmedVehicleCount != request.m_iCompositionArmedVehicleCount)
		{
			order.m_iCompositionArmedVehicleCount = request.m_iCompositionArmedVehicleCount;
			changed = true;
		}

		return changed;
	}

	protected HST_SupportRequestState FindSupportRequest(HST_CampaignState state, string requestId)
	{
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (request && request.m_sRequestId == requestId)
				return request;
		}

		return null;
	}

	protected bool ResolveOrders(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons)
	{
		bool changed;
		if (!state)
			return false;

		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
				continue;

			if (state.m_iElapsedSeconds < order.m_iResolveAtSecond)
				continue;

			if (order.m_bPhysicalized && !IsPhysicalizedOrderTimedOut(state, order))
				continue;

			changed = ResolveOrderNow(state, preset, garrisons, order) || changed;
		}

		return changed;
	}

	protected bool ResolveOrderNow(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_EnemyOrderState order)
	{
		if (!state || !order || order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_ACTIVE)
			return false;

		order.m_eStatus = HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED;
		order.m_iResolvedAtSecond = state.m_iElapsedSeconds;

		if (order.m_bPhysicalized)
		{
			order.m_sResolutionKind = "physical_timeout";
			order.m_sRuntimeStatus = "resolved_physical_timeout";
		}
		else
		{
			order.m_bAbstractResolved = true;
			order.m_sResolutionKind = "abstract";
			order.m_sRuntimeStatus = "resolved_abstract";
			ApplyResolvedOrder(state, preset, garrisons, order);
		}

		return true;
	}

	protected bool IsPhysicalizedOrderTimedOut(HST_CampaignState state, HST_EnemyOrderState order)
	{
		if (!state || !order)
			return true;

		int timeoutSecond = order.m_iResolveAtSecond + PHYSICAL_ORDER_TIMEOUT_SECONDS;
		return state.m_iElapsedSeconds >= timeoutSecond;
	}

	protected void ApplyResolvedOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_EnemyOrderState order)
	{
		if (!state || !order || order.m_bOutcomeApplied)
			return;

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON)
		{
			if (garrisons)
				garrisons.AddAbstractForces(state, order.m_sTargetZoneId, order.m_sFactionKey, 2 + state.m_iWarLevel, 0);

			order.m_sResolutionKind = "abstract_rebuild_garrison";
			order.m_bOutcomeApplied = true;
			return;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
		{
			HST_CivilianZoneState civilianZone = state.FindCivilianZone(order.m_sTargetZoneId);
			if (!civilianZone)
			{
				civilianZone = new HST_CivilianZoneState();
				civilianZone.m_sZoneId = order.m_sTargetZoneId;
				state.m_aCivilianZones.Insert(civilianZone);
			}

			civilianZone.m_iRoadblockPresence = Math.Min(3, civilianZone.m_iRoadblockPresence + 1);
			order.m_sResolutionKind = "abstract_roadblock";
			order.m_bOutcomeApplied = true;
			return;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF)
		{
			HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
			if (targetZone)
				targetZone.m_iResistanceCaptureProgress = Math.Max(0, targetZone.m_iResistanceCaptureProgress - 20);

			if (garrisons)
				garrisons.AddAbstractForces(state, order.m_sTargetZoneId, order.m_sFactionKey, 1 + state.m_iWarLevel / 2, 0);

			order.m_sResolutionKind = "abstract_qrf_pressure";
			order.m_bOutcomeApplied = true;
			return;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
		{
			ApplyAbstractCounterattack(state, preset, garrisons, order);
			order.m_bOutcomeApplied = true;
			return;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL)
		{
			ApplyAbstractSupportPressure(state, preset, order);
			order.m_bOutcomeApplied = true;
			return;
		}

		if (order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
		{
			ApplyAbstractPetrosPressure(state, preset, order);
			order.m_bOutcomeApplied = true;
			return;
		}

		order.m_sResolutionKind = "abstract_patrol_completed";
		order.m_bOutcomeApplied = true;
	}

	protected void ApplyAbstractCounterattack(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_EnemyOrderState order)
	{
		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		if (!targetZone)
		{
			order.m_sResolutionKind = "abstract_counterattack_missing_zone";
			return;
		}

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		HST_GarrisonState fiaGarrison = state.FindGarrison(targetZone.m_sZoneId, resistanceFactionKey);
		if (fiaGarrison && fiaGarrison.m_iInfantryCount > 0)
		{
			int casualties = Math.Max(1, state.m_iWarLevel / 2);
			fiaGarrison.m_iInfantryCount = Math.Max(0, fiaGarrison.m_iInfantryCount - casualties);
			order.m_sResolutionKind = string.Format("abstract_counterattack_fia_casualties_%1", casualties);
			return;
		}

		targetZone.m_iSupport = Math.Max(-100, targetZone.m_iSupport - 8);
		order.m_sResolutionKind = "abstract_counterattack_support_damage";
	}

	protected void ApplyAbstractSupportPressure(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyOrderState order)
	{
		HST_ZoneState targetZone = state.FindZone(order.m_sTargetZoneId);
		if (!targetZone)
		{
			order.m_sResolutionKind = "abstract_support_missing_zone";
			return;
		}

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		if (targetZone.m_sOwnerFactionKey == resistanceFactionKey)
			targetZone.m_iSupport = Math.Max(-100, targetZone.m_iSupport - 6);
		else
			targetZone.m_iResistanceCaptureProgress = Math.Max(0, targetZone.m_iResistanceCaptureProgress - 12);

		order.m_sResolutionKind = "abstract_support_pressure";
	}

	protected void ApplyAbstractPetrosPressure(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyOrderState order)
	{
		state.m_iHQKnowledge = Math.Min(100, state.m_iHQKnowledge + 10);
		state.m_iHQThreatLevel = Math.Max(state.m_iHQThreatLevel, state.m_iHQKnowledge);
		state.m_iHQKnowledgeLastChangedSecond = state.m_iElapsedSeconds;
		state.m_iLastHQActivitySecond = state.m_iElapsedSeconds;
		state.m_sLastHQKnowledgeReason = "abstract Petros pressure";
		state.m_sLastHQThreatReason = "abstract Petros pressure";
		state.m_iLastHQAttackSecond = state.m_iElapsedSeconds;
		order.m_sResolutionKind = "abstract_petros_pressure";
	}

	protected HST_ZoneState ResolvePetrosAttackTargetZone(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return null;

		vector targetPosition = state.m_vPetrosPosition;
		if (IsZeroVector(targetPosition))
			targetPosition = state.m_vHQPosition;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
				continue;
			if (zone.m_sOwnerFactionKey == resistanceFactionKey)
				continue;

			float distanceSq = DistanceSq2D(targetPosition, zone.m_vPosition);
			if (distanceSq < bestDistanceSq)
			{
				bestZone = zone;
				bestDistanceSq = distanceSq;
			}
		}

		if (bestZone)
			return bestZone;

		foreach (HST_ZoneState fallback : state.m_aZones)
		{
			if (!fallback)
				continue;
			if (fallback.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || fallback.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
				continue;

			float fallbackDistanceSq = DistanceSq2D(targetPosition, fallback.m_vPosition);
			if (!bestZone || fallbackDistanceSq < bestDistanceSq)
			{
				bestZone = fallback;
				bestDistanceSq = fallbackDistanceSq;
			}
		}

		return bestZone;
	}

	protected HST_ZoneState SelectTargetZone(HST_CampaignState state, HST_CampaignPreset preset, string factionKey)
	{
		HST_ZoneState bestZone;
		int bestScore = -9999;
		int totalWeight;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			int score = ScoreTargetZone(state, preset, zone, factionKey);
			totalWeight += Math.Max(1, score + 25);

			if (score > bestScore)
			{
				bestZone = zone;
				bestScore = score;
			}
		}

		if (totalWeight <= 0)
			return bestZone;

		int rollSeed = state.m_iCampaignSeed + state.m_iElapsedSeconds * 13 + factionKey.Length() * 97 + state.m_aEnemyOrders.Count() * 43;
		int roll = HST_DefaultCatalog.PositiveMod(rollSeed, totalWeight);
		int cumulative;
		foreach (HST_ZoneState weightedZone : state.m_aZones)
		{
			if (!weightedZone)
				continue;

			cumulative += Math.Max(1, ScoreTargetZone(state, preset, weightedZone, factionKey) + 25);
			if (roll < cumulative)
				return weightedZone;
		}

		return bestZone;
	}

	protected HST_EEnemyOrderType SelectOrderType(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone, HST_FactionPoolState pool)
	{
		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		if (state.m_iHQKnowledge >= 100 && IsHQThreatZone(state, targetZone) && pool.m_iAttackResources >= 20 && pool.m_iSupportResources >= 8 && state.m_iElapsedSeconds > state.m_iLastHQAttackSecond + 1800)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK;

		if (IsHQThreatZone(state, targetZone) && pool.m_iAttackResources >= 25 && pool.m_iSupportResources >= 8 && state.m_iWarLevel >= 4)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK;

		if (targetZone.m_sOwnerFactionKey == resistanceFactionKey)
			return HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK;

		int recentDamageScore;
		if (state && pool)
		{
			HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(pool.m_sFactionKey, targetZone.m_sZoneId);
			if (ledger)
				recentDamageScore = ledger.m_iRecentDamageScore;
		}
		if (targetZone.m_iResistanceCaptureProgress > 0 || recentDamageScore > 0)
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

	protected void RecordTargetPressureSignal(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ZoneState targetZone)
	{
		if (!state || !enemyDirector || !targetZone || factionKey.IsEmpty())
			return;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;
		if (targetZone.m_sOwnerFactionKey == resistanceFactionKey)
			return;

		int damageScore = Math.Max(0, targetZone.m_iResistanceCaptureProgress / 5);
		if (targetZone.m_bActive)
			damageScore += 2;
		if (HasActiveMissionNearZone(state, targetZone))
			damageScore += 3;
		if (HasActiveObjectiveNearZone(state, targetZone))
			damageScore += 3;

		if (damageScore <= 0)
			return;

		enemyDirector.RecordZoneDamageSignal(state, factionKey, targetZone, damageScore, "target pressure signal");
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
		if (state.m_iHQKnowledge > 0 && IsHQThreatZone(state, zone))
			score += state.m_iHQKnowledge / 10;

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

	protected bool HasActiveObjectiveNearZone(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return false;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_bComplete || objective.m_bFailed)
				continue;

			HST_ActiveMissionState mission = state.FindActiveMission(objective.m_sMissionInstanceId);
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			if (objective.m_sTargetZoneId == zone.m_sZoneId)
				return true;
			if (DistanceSq2D(objective.m_vPosition, zone.m_vPosition) < 450000)
				return true;
		}

		return false;
	}

	protected bool IsPetrosAttackTargetInsidePlayerEventBubble(HST_CampaignState state)
	{
		if (!state)
			return false;

		vector target = ResolvePetrosAttackTargetPosition(state);
		if (IsZeroVector(target))
			return false;

		return HST_WorldPositionService.IsPositionInsidePlayerEventBubble(target);
	}

	protected vector ResolvePetrosAttackTargetPosition(HST_CampaignState state)
	{
		if (!state)
			return "0 0 0";

		vector target = state.m_vPetrosPosition;
		if (IsZeroVector(target))
			target = state.m_vHQPosition;

		return target;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected bool IsHQThreatZone(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, zone.m_vPosition) <= 1440000;
	}

	protected vector ResolveOrderSourcePosition(HST_CampaignState state, HST_CampaignPreset preset, string factionKey, HST_ZoneState targetZone)
	{
		if (!state || !targetZone)
			return "0 0 0";

		HST_ZoneState bestZone;
		float bestDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sZoneId == targetZone.m_sZoneId)
				continue;

			if (zone.m_sOwnerFactionKey != factionKey)
				continue;

			float distanceSq = DistanceSq2D(zone.m_vPosition, targetZone.m_vPosition);
			if (distanceSq < bestDistanceSq)
			{
				bestZone = zone;
				bestDistanceSq = distanceSq;
			}
		}

		if (bestZone)
			return bestZone.m_vPosition;

		return targetZone.m_vPosition;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}

	protected HST_ESupportRequestType SupportTypeForOrder(HST_CampaignState state, HST_CampaignPreset preset, HST_ZoneState targetZone, HST_EEnemyOrderType orderType)
	{
		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			return HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_QRF || orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
			return HST_ESupportRequestType.HST_SUPPORT_QRF;

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL)
		{
			int roll = ResolveSupportTypeRoll(state, targetZone, orderType);
			if (state && state.m_iWarLevel >= 6 && targetZone && (targetZone.m_sOwnerFactionKey == resistanceFactionKey || targetZone.m_bActive) && roll < 20)
				return HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55;

			if (state && state.m_iWarLevel >= 3 && roll < 70)
				return HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK;

			return HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
		}

		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_ROADBLOCK)
			return HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP;

		return HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP;
	}

	protected int ResolveSupportTypeRoll(HST_CampaignState state, HST_ZoneState targetZone, HST_EEnemyOrderType orderType)
	{
		if (!state)
			return 0;

		int zoneHash;
		if (targetZone)
			zoneHash = targetZone.m_sZoneId.Length() * 31 + targetZone.m_iPriority * 17 + targetZone.m_iResistanceCaptureProgress * 7;
		int orderTypeScore = 1;
		if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_SUPPORT_CALL)
			orderTypeScore = 5;
		else if (orderType == HST_EEnemyOrderType.HST_ENEMY_ORDER_PETROS_ATTACK)
			orderTypeScore = 9;
		int seed = state.m_iCampaignSeed + state.m_iElapsedSeconds * 3 + state.m_aEnemyOrders.Count() * 41 + state.m_iWarLevel * 19 + zoneHash + orderTypeScore * 11;
		return HST_DefaultCatalog.PositiveMod(seed, 100);
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

	protected HST_EnemyOrderState FindOrderForDebug(HST_CampaignState state, string orderId)
	{
		if (!state || orderId.IsEmpty())
			return null;

		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				return order;
		}

		return null;
	}
}
