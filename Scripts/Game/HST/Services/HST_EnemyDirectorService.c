class HST_EnemyDirectorService
{
	static const int RESOURCE_TICK_SECONDS = 300;
	static const int SUPPORT_STACK_COOLDOWN_SECONDS = 600;
	static const int SUPPORT_SPEND_WINDOW_SECONDS = 1800;
	static const int RECENT_DAMAGE_WINDOW_SECONDS = 900;
	static const int MAX_DEFENCE_SPEND_BASE = 28;
	static const int MAX_DEFENCE_SPEND_PER_WAR_LEVEL = 7;

	bool TickResources(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, int elapsedSeconds)
	{
		if (!state || !preset || !balance || elapsedSeconds <= 0)
			return false;

		state.m_iEnemyResourceAccumulatorSeconds += elapsedSeconds;
		if (state.m_iEnemyResourceAccumulatorSeconds < RESOURCE_TICK_SECONDS)
			return false;

		int resourceSteps = state.m_iEnemyResourceAccumulatorSeconds / RESOURCE_TICK_SECONDS;
		state.m_iEnemyResourceAccumulatorSeconds = state.m_iEnemyResourceAccumulatorSeconds % RESOURCE_TICK_SECONDS;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
				continue;

			int attackIncome = ResolveEnemyAttackIncome(state, balance, zone) * resourceSteps;
			int supportIncome = ResolveEnemySupportIncome(state, balance, zone) * resourceSteps;
			AddResources(state, zone.m_sOwnerFactionKey, attackIncome, supportIncome);
		}

		return true;
	}

	bool CanAfford(HST_CampaignState state, string factionKey, int attackCost, int supportCost)
	{
		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		return pool && pool.m_iAttackResources >= attackCost && pool.m_iSupportResources >= supportCost;
	}

	bool TrySpend(HST_CampaignState state, string factionKey, int attackCost, int supportCost)
	{
		if (attackCost < 0 || supportCost < 0 || !CanAfford(state, factionKey, attackCost, supportCost))
			return false;

		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		pool.m_iAttackResources -= attackCost;
		pool.m_iSupportResources -= supportCost;
		return true;
	}

	bool CanSpendProactiveAttack(HST_CampaignState state, string factionKey, int attackCost, out string reason)
	{
		reason = "";
		if (!state || factionKey.IsEmpty())
		{
			reason = "state or faction missing";
			return false;
		}

		if (attackCost < 0)
		{
			reason = "negative proactive attack spend requested";
			return false;
		}

		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		if (!pool)
		{
			reason = "faction resource pool missing";
			return false;
		}

		if (pool.m_iAttackResources < attackCost)
		{
			reason = string.Format("cannot afford proactive attack %1/%2", pool.m_iAttackResources, attackCost);
			return false;
		}

		reason = string.Format("proactive attack spend allowed | attack %1/%2", pool.m_iAttackResources, attackCost);
		return true;
	}

	bool TrySpendProactiveAttack(HST_CampaignState state, string factionKey, int attackCost, out string reason)
	{
		if (!CanSpendProactiveAttack(state, factionKey, attackCost, reason))
			return false;

		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		if (!pool)
		{
			reason = "faction resource pool missing after proactive gate";
			return false;
		}

		pool.m_iAttackResources = Math.Max(0, pool.m_iAttackResources - Math.Max(0, attackCost));
		return true;
	}

	bool CanSpendDefense(HST_CampaignState state, HST_ZoneState targetZone, string factionKey, int attackCost, int supportCost, out string reason)
	{
		reason = "";
		if (!state || !targetZone || factionKey.IsEmpty())
		{
			reason = "state, target zone, or faction missing";
			return false;
		}

		if (attackCost < 0 || supportCost < 0)
		{
			reason = "negative defense spend requested";
			return false;
		}

		HST_EnemySupportLedgerState ledger = FindOrCreateSupportLedger(state, factionKey, targetZone.m_sZoneId);
		if (!ledger)
		{
			reason = "support ledger unavailable";
			return false;
		}

		UpdateLedgerWindows(state, ledger);
		if (ledger.m_iCooldownUntilSecond > state.m_iElapsedSeconds)
		{
			reason = string.Format("support stack cooldown active for %1s", ledger.m_iCooldownUntilSecond - state.m_iElapsedSeconds);
			ledger.m_sLastDecisionReason = reason;
			return false;
		}

		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		if (!pool)
		{
			reason = "faction resource pool missing";
			ledger.m_sLastDecisionReason = reason;
			return false;
		}

		if (pool.m_iAttackResources < attackCost || pool.m_iSupportResources < supportCost)
		{
			reason = string.Format("cannot afford attack %1/%2 support %3/%4", pool.m_iAttackResources, attackCost, pool.m_iSupportResources, supportCost);
			ledger.m_sLastDecisionReason = reason;
			return false;
		}

		int maxSpend = ResolveMaxDefenseSpend(state, targetZone, ledger);
		int projectedSpend = ledger.m_iAttackSpent + ledger.m_iSupportSpent + attackCost + supportCost;
		if (projectedSpend > maxSpend)
		{
			reason = string.Format("max defense spend reached for zone %1 | projected %2 cap %3", targetZone.m_sZoneId, projectedSpend, maxSpend);
			ledger.m_sLastDecisionReason = reason;
			return false;
		}

		reason = string.Format("defense spend allowed | projected %1 cap %2 damage %3", projectedSpend, maxSpend, ResolveRecentDamageScore(state, ledger));
		ledger.m_sLastDecisionReason = reason;
		return true;
	}

	bool TrySpendDefense(HST_CampaignState state, HST_ZoneState targetZone, string factionKey, int attackCost, int supportCost, out string reason)
	{
		if (!CanSpendDefense(state, targetZone, factionKey, attackCost, supportCost, reason))
			return false;

		if (!TrySpend(state, factionKey, attackCost, supportCost))
		{
			reason = "enemy resource spend failed after defense gate";
			HST_EnemySupportLedgerState failedLedger = FindOrCreateSupportLedger(state, factionKey, targetZone.m_sZoneId);
			if (failedLedger)
				failedLedger.m_sLastDecisionReason = reason;
			return false;
		}

		HST_EnemySupportLedgerState ledger = FindOrCreateSupportLedger(state, factionKey, targetZone.m_sZoneId);
		if (ledger)
		{
			ledger.m_iAttackSpent += attackCost;
			ledger.m_iSupportSpent += supportCost;
			ledger.m_iLastSpendSecond = state.m_iElapsedSeconds;
			ledger.m_iCooldownUntilSecond = state.m_iElapsedSeconds + SUPPORT_STACK_COOLDOWN_SECONDS;
			ledger.m_sLastDecisionReason = string.Format("spent attack %1 support %2 | cooldown until %3", attackCost, supportCost, ledger.m_iCooldownUntilSecond);
		}

		return true;
	}

	void RecordZoneDamageSignal(HST_CampaignState state, string factionKey, HST_ZoneState targetZone, int damageScore, string reason)
	{
		if (!state || !targetZone || factionKey.IsEmpty() || damageScore <= 0)
			return;

		HST_EnemySupportLedgerState ledger = FindOrCreateSupportLedger(state, factionKey, targetZone.m_sZoneId);
		if (!ledger)
			return;

		UpdateLedgerWindows(state, ledger);
		ledger.m_iRecentDamageScore = Math.Min(100, ledger.m_iRecentDamageScore + damageScore);
		ledger.m_iLastDamageSecond = state.m_iElapsedSeconds;
		if (reason.IsEmpty())
			reason = "recent resistance pressure";
		ledger.m_sLastDecisionReason = string.Format("%1 | damage score %2", reason, ledger.m_iRecentDamageScore);
	}

	int GetRecentDamageScore(HST_CampaignState state, string factionKey, string zoneId)
	{
		if (!state)
			return 0;

		HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(factionKey, zoneId);
		if (!ledger)
			return 0;

		return ResolveRecentDamageScore(state, ledger);
	}

	void RefundDefenseResources(HST_CampaignState state, string factionKey, string zoneId, int attackRefund, int supportRefund, string reason)
	{
		if (!state || factionKey.IsEmpty())
			return;

		int clampedAttack = Math.Max(0, attackRefund);
		int clampedSupport = Math.Max(0, supportRefund);
		if (clampedAttack <= 0 && clampedSupport <= 0)
			return;

		AddResources(state, factionKey, clampedAttack, clampedSupport);
		HST_EnemySupportLedgerState ledger = FindOrCreateSupportLedger(state, factionKey, zoneId);
		if (ledger)
		{
			ledger.m_iRefundedAttackResources += clampedAttack;
			ledger.m_iRefundedSupportResources += clampedSupport;
			ledger.m_iAttackSpent = Math.Max(0, ledger.m_iAttackSpent - clampedAttack);
			ledger.m_iSupportSpent = Math.Max(0, ledger.m_iSupportSpent - clampedSupport);
			if (reason.IsEmpty())
				reason = "defense refund";
			ledger.m_sLastDecisionReason = string.Format("%1 | refund attack %2 support %3", reason, clampedAttack, clampedSupport);
		}
	}

	void AddResources(HST_CampaignState state, string factionKey, int attackResources, int supportResources)
	{
		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		if (!pool)
			return;

		pool.m_iAttackResources = Math.Max(0, pool.m_iAttackResources + attackResources);
		pool.m_iSupportResources = Math.Max(0, pool.m_iSupportResources + supportResources);
	}

	string BuildEnemyResourceReport(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance)
	{
		if (!state || !preset || !balance)
			return "h-istasi enemy resources | state/preset/balance not ready";

		string report = string.Format(
			"h-istasi enemy resources | tick %1s | timer %2s | war %3 | attack war pct %4 | support war pct %5",
			RESOURCE_TICK_SECONDS,
			state.m_iEnemyResourceAccumulatorSeconds,
			state.m_iWarLevel,
			balance.m_iEnemyAttackIncomeWarPercent,
			balance.m_iEnemySupportIncomeWarPercent
		);
		report = report + "\nspend policy | proactive attacks use attack pool | reactive defense/support uses support ledger";

		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || !HST_FactionRelationService.IsEnemyFaction(preset, pool.m_sFactionKey))
				continue;

			int attackIncome;
			int supportIncome;
			CalculateProjectedIncomeForFaction(state, balance, pool.m_sFactionKey, attackIncome, supportIncome);
			report = report + string.Format(
				"\n%1 | current attack %2 support %3 aggression %4 | next attack +%5 support +%6",
				pool.m_sFactionKey,
				pool.m_iAttackResources,
				pool.m_iSupportResources,
				pool.m_iAggression,
				attackIncome,
				supportIncome
			);
		}

		string ledgerReport = BuildEnemySupportLedgerReport(state, preset);
		if (!ledgerReport.IsEmpty())
			report = report + "\n" + ledgerReport;

		return report;
	}

	string BuildEnemySupportLedgerReport(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return "";

		int count;
		string report = "h-istasi enemy support ledgers";
		foreach (HST_EnemySupportLedgerState ledger : state.m_aEnemySupportLedgers)
		{
			if (!ledger || !HST_FactionRelationService.IsEnemyFaction(preset, ledger.m_sFactionKey))
				continue;

			UpdateLedgerWindows(state, ledger);
			if (ledger.m_iAttackSpent <= 0 && ledger.m_iSupportSpent <= 0 && ledger.m_iRecentDamageScore <= 0 && ledger.m_iCooldownUntilSecond <= state.m_iElapsedSeconds && ledger.m_sLastDecisionReason.IsEmpty())
				continue;

			HST_ZoneState zone = state.FindZone(ledger.m_sZoneId);
			int maxSpend = ResolveMaxDefenseSpend(state, zone, ledger);
			int recentDamage = ResolveRecentDamageScore(state, ledger);
			int cooldownRemaining = Math.Max(0, ledger.m_iCooldownUntilSecond - state.m_iElapsedSeconds);
			string line = string.Format(
				"\n%1/%2 | spent %3/%4 cap %5 | damage %6 | cooldown %7s | refunds %8/%9",
				ledger.m_sFactionKey,
				ledger.m_sZoneId,
				ledger.m_iAttackSpent,
				ledger.m_iSupportSpent,
				maxSpend,
				recentDamage,
				cooldownRemaining,
				ledger.m_iRefundedAttackResources,
				ledger.m_iRefundedSupportResources
			);
			report = report + line + string.Format(" | reason %1", ledger.m_sLastDecisionReason);
			count++;
			if (count >= 12)
				break;
		}

		if (count <= 0)
			return "h-istasi enemy support ledgers | none";

		return report;
	}

	protected void CalculateProjectedIncomeForFaction(HST_CampaignState state, HST_BalanceConfig balance, string factionKey, out int attackIncome, out int supportIncome)
	{
		attackIncome = 0;
		supportIncome = 0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != factionKey)
				continue;

			attackIncome += ResolveEnemyAttackIncome(state, balance, zone);
			supportIncome += ResolveEnemySupportIncome(state, balance, zone);
		}
	}

	protected HST_EnemySupportLedgerState FindOrCreateSupportLedger(HST_CampaignState state, string factionKey, string zoneId)
	{
		if (!state || factionKey.IsEmpty() || zoneId.IsEmpty())
			return null;

		HST_EnemySupportLedgerState ledger = state.FindEnemySupportLedger(factionKey, zoneId);
		if (ledger)
			return ledger;

		ledger = new HST_EnemySupportLedgerState();
		ledger.m_sFactionKey = factionKey;
		ledger.m_sZoneId = zoneId;
		state.m_aEnemySupportLedgers.Insert(ledger);
		return ledger;
	}

	protected void UpdateLedgerWindows(HST_CampaignState state, HST_EnemySupportLedgerState ledger)
	{
		if (!state || !ledger)
			return;

		if ((ledger.m_iAttackSpent > 0 || ledger.m_iSupportSpent > 0) && state.m_iElapsedSeconds - ledger.m_iLastSpendSecond > SUPPORT_SPEND_WINDOW_SECONDS)
		{
			ledger.m_iAttackSpent = 0;
			ledger.m_iSupportSpent = 0;
		}

		if (ledger.m_iRecentDamageScore > 0 && state.m_iElapsedSeconds - ledger.m_iLastDamageSecond > RECENT_DAMAGE_WINDOW_SECONDS)
			ledger.m_iRecentDamageScore = 0;
	}

	protected int ResolveRecentDamageScore(HST_CampaignState state, HST_EnemySupportLedgerState ledger)
	{
		if (!state || !ledger)
			return 0;

		if (ledger.m_iRecentDamageScore <= 0)
			return Math.Max(0, ledger.m_iRecentDamageScore);

		int age = state.m_iElapsedSeconds - ledger.m_iLastDamageSecond;
		if (age <= 0)
			return Math.Max(0, ledger.m_iRecentDamageScore);

		if (age >= RECENT_DAMAGE_WINDOW_SECONDS)
			return 0;

		int remainingPercent = Math.Max(0, 100 - Math.Round(age * 100.0 / RECENT_DAMAGE_WINDOW_SECONDS));
		return Math.Max(0, Math.Round(ledger.m_iRecentDamageScore * remainingPercent / 100.0));
	}

	protected int ResolveMaxDefenseSpend(HST_CampaignState state, HST_ZoneState zone, HST_EnemySupportLedgerState ledger)
	{
		if (!state)
			return MAX_DEFENCE_SPEND_BASE;

		int maxSpend = MAX_DEFENCE_SPEND_BASE + Math.Max(0, state.m_iWarLevel) * MAX_DEFENCE_SPEND_PER_WAR_LEVEL;
		if (zone)
		{
			maxSpend += Math.Max(0, zone.m_iPriority / 2);
			maxSpend += Math.Max(0, zone.m_iResistanceCaptureProgress / 4);
			if (zone.m_bActive)
				maxSpend += 8;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
				maxSpend += 10;
			else if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST || zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
				maxSpend += 6;
		}

		maxSpend += ResolveRecentDamageScore(state, ledger) / 3;
		return Math.Max(20, maxSpend);
	}

	protected int ResolveEnemyAttackIncome(HST_CampaignState state, HST_BalanceConfig balance, HST_ZoneState zone)
	{
		int income = Math.Max(1, zone.m_iIncomeValue / 60) + Math.Max(0, zone.m_iPriority / 10);
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			income += 3;
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			income += 2;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && zone.m_sResourceKind == "fuel")
			income += 2;

		int multiplier = 100 + state.m_iWarLevel * Math.Max(0, balance.m_iEnemyAttackIncomeWarPercent);
		income = Math.Max(1, Math.Round(income * multiplier / 100.0));
		return income;
	}

	protected int ResolveEnemySupportIncome(HST_CampaignState state, HST_BalanceConfig balance, HST_ZoneState zone)
	{
		int income = 1;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE && zone.m_sResourceKind == "supplies")
			income += 2;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER || zone.m_eType == HST_EZoneType.HST_ZONE_POLICE_STATION)
			income += 1;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			income += Math.Max(0, 1 - zone.m_iSupport / 50);

		int multiplier = 100 + state.m_iWarLevel * Math.Max(0, balance.m_iEnemySupportIncomeWarPercent);
		income = Math.Max(1, Math.Round(income * multiplier / 100.0));
		return income;
	}
}
