class HST_EnemyStrategicMutationCommand
{
	string m_sMutationId;
	string m_sFactionKey;
	string m_sKind;
	string m_sSourceId;
	string m_sOrderId;
	string m_sOperationId;
	string m_sManifestId;
	string m_sZoneId;
	int m_iCreatedAtSecond = -1;
	int m_iAttackDelta;
	int m_iSupportDelta;
	int m_iAggressionDelta;
	string m_sContributionHash;
}

class HST_EnemyStrategicMutationResult
{
	bool m_bAccepted;
	bool m_bAlreadyApplied;
	bool m_bChanged;
	string m_sFailureReason;
	ref HST_EnemyStrategicMutationState m_Mutation;
	ref HST_FactionPoolState m_Pool;

	string BuildReport()
	{
		string mutationId = "none";
		string factionKey = "none";
		int revision;
		if (m_Mutation)
			mutationId = m_Mutation.m_sMutationId;
		if (m_Pool)
		{
			factionKey = m_Pool.m_sFactionKey;
			revision = m_Pool.m_iStrategicRevision;
		}
		return string.Format(
			"Partisan enemy strategic mutation | accepted %1 | replay %2 | changed %3 | faction %4 | mutation %5 | revision %6 | %7",
			m_bAccepted,
			m_bAlreadyApplied,
			m_bChanged,
			factionKey,
			mutationId,
			revision,
			m_sFailureReason);
	}
}

// Schema-67 canonical authority for configured enemy attack resources,
// support resources, and aggression. Operational receipts are never silently
// evicted. Periodic income/decay retain one compact cadence receipt per
// faction/kind; their persisted accumulators are the scheduling authority.
class HST_EnemyStrategicResourceService
{
	static const int SCHEMA_VERSION = 67;
	static const int CONTRACT_VERSION = 1;
	static const int QUARANTINE_CONTRACT_VERSION = -67;
	static const int RESOURCE_INTERVAL_SECONDS = 300;
	static const int MAX_CATCHUP_STEPS_PER_TICK = 24;
	static const int MAX_OPERATIONAL_MUTATIONS = 4096;
	static const int MAX_TOTAL_OPERATIONAL_MUTATIONS = MAX_OPERATIONAL_MUTATIONS * 2;
	static const int MAX_PERIODIC_MUTATIONS = 4;
	static const int MAX_MUTATION_ROWS = MAX_TOTAL_OPERATIONAL_MUTATIONS + MAX_PERIODIC_MUTATIONS;
	static const int MAX_MUTABLE_REVISION = int.MAX - 16;
	static const int MAX_ID_CHARACTERS = 192;
	static const int MAX_FAILURE_CHARACTERS = 320;
	static const int MAX_CONTRIBUTION_CHARACTERS = 192;
	static const int MAX_DELTA_MAGNITUDE = 1000000000;
	static const string EXACT_POLICY_ID = "schema67_enemy_strategic_resource_exact1";
	static const string KIND_PERIODIC_INCOME = "periodic_income";
	static const string KIND_AGGRESSION_DECAY = "aggression_decay";
	static const string KIND_RESOURCE_DEBIT = "resource_debit";
	static const string KIND_RESOURCE_REFUND = "resource_refund";
	static const string KIND_RESOURCE_DELTA = "resource_delta";
	static const string KIND_AGGRESSION_DELTA = "aggression_delta";

	protected HST_TownInfluenceService m_TownInfluence;
	protected bool m_bApplyingPeriodicBucket;

	void SetTownInfluenceService(HST_TownInfluenceService townInfluence)
	{
		m_TownInfluence = townInfluence;
	}

	// Read-only production admission gate for diagnostics and other observers.
	// It intentionally delegates to the same exact pool resolver used before a
	// strategic mutation, without creating a receipt or changing live state.
	bool ResolveExactPool(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string factionKey,
		out HST_FactionPoolState pool,
		out string failure)
	{
		pool = null;
		failure = "";
		factionKey = factionKey.Trim();
		if (!preset || factionKey.IsEmpty()
			|| (factionKey != preset.m_sOccupierFactionKey
				&& factionKey != preset.m_sInvaderFactionKey))
		{
			failure = "enemy strategic pool is not a configured enemy role";
			return false;
		}

		failure = ResolveExactEnemyPool(state, preset, factionKey, pool);
		return failure.IsEmpty();
	}

	static string BuildMutationId(
		string kind,
		string factionKey,
		string sourceId)
	{
		kind = kind.Trim();
		factionKey = factionKey.Trim();
		sourceId = sourceId.Trim();
		if (kind.IsEmpty() || factionKey.IsEmpty() || sourceId.IsEmpty())
			return "";
		string canonical = string.Format(
			"%1|%2|%3|%4",
			EXACT_POLICY_ID,
			kind,
			factionKey,
			sourceId);
		return string.Format(
			"enemy_strategic_%1_%2_%3_%4",
			kind.Hash(),
			factionKey.Hash(),
			canonical.Hash(),
			(canonical + "|secondary").Hash());
	}

	static string BuildMutationFingerprint(
		HST_EnemyStrategicMutationState mutation)
	{
		if (!mutation)
			return "";
		string canonical = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			mutation.m_iContractVersion,
			mutation.m_sMutationId,
			mutation.m_sFactionKey,
			mutation.m_sKind,
			mutation.m_sSourceId,
			mutation.m_sOrderId,
			mutation.m_sOperationId,
			mutation.m_sManifestId,
			mutation.m_sZoneId);
		canonical = canonical + string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			mutation.m_iCreatedAtSecond,
			mutation.m_iPoolRevisionBefore,
			mutation.m_iPoolRevisionAfter,
			mutation.m_iOperationalSequence,
			mutation.m_iAttackBefore,
			mutation.m_iAttackDelta,
			mutation.m_iAttackAfter,
			mutation.m_iSupportBefore,
			mutation.m_iSupportDelta);
		canonical = canonical + string.Format(
			"|%1|%2|%3|%4|%5|%6|%7",
			mutation.m_iSupportAfter,
			mutation.m_iAggressionBefore,
			mutation.m_iAggressionDelta,
			mutation.m_iAggressionAfter,
			mutation.m_sContributionHash,
			mutation.m_bApplied,
			EXACT_POLICY_ID);
		return string.Format(
			"esr1_%1_%2",
			canonical.Hash(),
			(canonical + "|secondary").Hash());
	}

	HST_EnemyStrategicMutationResult ApplyMutation(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyStrategicMutationCommand command)
	{
		HST_EnemyStrategicMutationResult result
			= new HST_EnemyStrategicMutationResult();
		string failure = NormalizeAndValidateCommand(state, preset, command);
		if (!failure.IsEmpty())
			return Reject(result, failure);

		HST_FactionPoolState pool;
		failure = ResolveExactEnemyPool(state, preset, command.m_sFactionKey, pool);
		result.m_Pool = pool;
		if (!failure.IsEmpty())
		{
			if (failure.Contains("duplicated") || failure.Contains("corrupt"))
				QuarantineFactionPools(state, command.m_sFactionKey, failure);
			return Reject(result, failure, pool);
		}

		HST_EnemyStrategicMutationState existing;
		bool uniqueMutation = FindUniqueMutation(
			state,
			command.m_sMutationId,
			existing);
		if (!uniqueMutation)
		{
			failure = "enemy strategic mutation id is duplicated";
			QuarantinePool(pool, failure);
			QuarantineMutationClaimantPools(
				state,
				command.m_sMutationId,
				failure);
			return Reject(result, failure, pool);
		}
		if (existing)
		{
			result.m_Mutation = existing;
			string existingFailure;
			if (!HST_EnemyStrategicResourceSaveValidationService.ValidateMutationShape(
				existing,
				existingFailure)
				|| !ExistingMatchesCommand(existing, command)
				|| !ExistingMatchesPool(existing, pool))
			{
				failure = "enemy strategic mutation id was reused with a different fingerprint";
				if (!existingFailure.IsEmpty())
					failure = failure + ": " + existingFailure;
				QuarantinePool(pool, failure);
				QuarantineFactionPools(
					state,
					existing.m_sFactionKey,
					failure);
				return Reject(result, failure, pool, existing);
			}
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			return result;
		}

		failure = ValidateNewMutationAdmission(state, pool, command);
		if (!failure.IsEmpty())
		{
			if (failure.Contains("corrupt"))
				QuarantinePool(pool, failure);
			return Reject(result, failure, pool);
		}

		int attackAfter;
		int supportAfter;
		int aggressionAfter;
		if (!TryApplyNonnegativeDelta(
			pool.m_iAttackResources,
			command.m_iAttackDelta,
			attackAfter))
			return Reject(result, "enemy attack-resource mutation would underflow or overflow", pool);
		if (!TryApplyNonnegativeDelta(
			pool.m_iSupportResources,
			command.m_iSupportDelta,
			supportAfter))
			return Reject(result, "enemy support-resource mutation would underflow or overflow", pool);
		if (!TryApplyNonnegativeDelta(
			pool.m_iAggression,
			command.m_iAggressionDelta,
			aggressionAfter))
			return Reject(result, "enemy aggression mutation would underflow or overflow", pool);

		HST_EnemyStrategicMutationState mutation
			= BuildMutation(
				state,
				pool,
				command,
				attackAfter,
				supportAfter,
				aggressionAfter);
		if (!mutation)
			return Reject(result, "enemy strategic mutation receipt could not be built", pool);
		string shapeFailure;
		if (!HST_EnemyStrategicResourceSaveValidationService.ValidateMutationShape(
			mutation,
			shapeFailure))
			return Reject(
				result,
				"enemy strategic mutation receipt shape is invalid: " + shapeFailure,
				pool);

		int compactedPeriodicIndex = -1;
		if (IsPeriodicKind(command.m_sKind))
		{
			if (CountOperationalMutationsForFaction(state, pool.m_sFactionKey)
				!= pool.m_iStrategicOperationalMutationCount)
				return Reject(
					result,
					"faction strategic operational receipt sequence is corrupt",
					pool);
			failure = ResolvePeriodicCompactionIndex(
				state,
				command,
				compactedPeriodicIndex);
			if (!failure.IsEmpty())
			{
				QuarantinePool(pool, failure);
				return Reject(result, failure, pool);
			}
		}

		// Every validation above is side-effect free. Commit the projection and
		// its receipt together; no fallible operation follows this point.
		pool.m_iAttackResources = attackAfter;
		pool.m_iSupportResources = supportAfter;
		pool.m_iAggression = aggressionAfter;
		pool.m_iStrategicRevision = mutation.m_iPoolRevisionAfter;
		if (!IsPeriodicKind(mutation.m_sKind))
			pool.m_iStrategicOperationalMutationCount
				= mutation.m_iOperationalSequence;
		pool.m_sLastStrategicMutationId = mutation.m_sMutationId;
		pool.m_sStrategicAuthorityFailure = "";
		state.m_aEnemyStrategicMutations.Insert(mutation);
		if (compactedPeriodicIndex >= 0)
			state.m_aEnemyStrategicMutations.Remove(compactedPeriodicIndex);

		result.m_bAccepted = true;
		result.m_bChanged = true;
		result.m_Mutation = mutation;
		return result;
	}

	HST_EnemyStrategicMutationResult DebitResources(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string mutationId,
		string factionKey,
		int attackCost,
		int supportCost,
		string kind,
		string sourceId,
		string orderId = "",
		string operationId = "",
		string manifestId = "",
		string zoneId = "")
	{
		if (attackCost < 0 || supportCost < 0)
			return BuildRejectedResult("enemy strategic debit cost is negative");
		if (kind.Trim().IsEmpty())
			kind = KIND_RESOURCE_DEBIT;
		return ApplyResourceDelta(
			state,
			preset,
			mutationId,
			factionKey,
			-attackCost,
			-supportCost,
			kind,
			sourceId,
			orderId,
			operationId,
			manifestId,
			zoneId);
	}

	HST_EnemyStrategicMutationResult RefundResources(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string mutationId,
		string factionKey,
		int attackRefund,
		int supportRefund,
		string kind,
		string sourceId,
		string orderId = "",
		string operationId = "",
		string manifestId = "",
		string zoneId = "")
	{
		if (attackRefund < 0 || supportRefund < 0)
			return BuildRejectedResult("enemy strategic refund amount is negative");
		if (kind.Trim().IsEmpty())
			kind = KIND_RESOURCE_REFUND;
		return ApplyResourceDelta(
			state,
			preset,
			mutationId,
			factionKey,
			attackRefund,
			supportRefund,
			kind,
			sourceId,
			orderId,
			operationId,
			manifestId,
			zoneId);
	}

	HST_EnemyStrategicMutationResult ApplyResourceDelta(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string mutationId,
		string factionKey,
		int attackDelta,
		int supportDelta,
		string kind,
		string sourceId,
		string orderId = "",
		string operationId = "",
		string manifestId = "",
		string zoneId = "")
	{
		if (kind.Trim().IsEmpty())
			kind = KIND_RESOURCE_DELTA;
		if (sourceId.Trim().IsEmpty())
			sourceId = mutationId;
		HST_EnemyStrategicMutationCommand command
			= NewCommand(
				mutationId,
				factionKey,
				kind,
				sourceId,
				orderId,
				operationId,
				manifestId,
				zoneId);
		command.m_iAttackDelta = attackDelta;
		command.m_iSupportDelta = supportDelta;
		return ApplyMutation(state, preset, command);
	}

	HST_EnemyStrategicMutationResult ApplyAggressionDelta(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string mutationId,
		string factionKey,
		int aggressionDelta,
		string kind,
		string sourceId,
		string orderId = "",
		string operationId = "",
		string manifestId = "",
		string zoneId = "")
	{
		if (kind.Trim().IsEmpty())
			kind = KIND_AGGRESSION_DELTA;
		if (sourceId.Trim().IsEmpty())
			sourceId = mutationId;
		HST_EnemyStrategicMutationCommand command
			= NewCommand(
				mutationId,
				factionKey,
				kind,
				sourceId,
				orderId,
				operationId,
				manifestId,
				zoneId);
		command.m_iAggressionDelta = aggressionDelta;
		return ApplyMutation(state, preset, command);
	}

	bool TickIncome(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		int elapsedSeconds)
	{
		if (!state || !preset || !balance || elapsedSeconds <= 0
			|| !ValidatePresetRoles(preset))
			return false;

		bool processed;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || !HST_FactionRelationService.IsEnemyFaction(
				preset,
				pool.m_sFactionKey))
				continue;
			HST_FactionPoolState uniquePool;
			string poolFailure = ResolveExactEnemyPool(
				state,
				preset,
				pool.m_sFactionKey,
				uniquePool);
			if (!poolFailure.IsEmpty() || uniquePool != pool)
			{
				if (pool.m_iStrategicContractVersion
					!= QUARANTINE_CONTRACT_VERSION)
				{
					if (poolFailure.Contains("duplicated"))
						QuarantineFactionPools(state, pool.m_sFactionKey, poolFailure);
					else
						QuarantinePool(pool, poolFailure);
				}
				continue;
			}
			int previousElapsedSecond = state.m_iElapsedSeconds - elapsedSeconds;
			if (pool.m_iLastResourceBucketSecond == 0
				&& pool.m_iResourceAccumulatorSeconds == 0
				&& !HasPeriodicMutation(
					state,
					pool.m_sFactionKey,
					KIND_PERIODIC_INCOME))
				pool.m_iLastResourceBucketSecond = previousElapsedSecond;
			if (previousElapsedSecond < 0
				|| pool.m_iLastResourceBucketSecond
					!= previousElapsedSecond - pool.m_iResourceAccumulatorSeconds)
			{
				QuarantinePool(pool, "enemy resource cadence checkpoint diverged");
				continue;
			}
			string checkpointFailure = ValidatePeriodicCheckpoint(
				state,
				pool.m_sFactionKey,
				KIND_PERIODIC_INCOME,
				pool.m_iLastResourceBucketSecond);
			if (!checkpointFailure.IsEmpty())
			{
				QuarantinePool(pool, checkpointFailure);
				continue;
			}
			int nextResourceAccumulator;
			if (!TryAccumulate(
				pool.m_iResourceAccumulatorSeconds,
				elapsedSeconds,
				nextResourceAccumulator))
			{
				QuarantinePool(pool, "enemy resource cadence accumulator overflow");
				continue;
			}
			pool.m_iResourceAccumulatorSeconds = nextResourceAccumulator;
			int due = pool.m_iResourceAccumulatorSeconds / RESOURCE_INTERVAL_SECONDS;
			int steps = Math.Min(due, MAX_CATCHUP_STEPS_PER_TICK);
			for (int stepIndex = 0; stepIndex < steps; stepIndex++)
			{
				int bucketEndSecond
					= ResolveOldestPendingBucketEnd(
						state,
						pool.m_iResourceAccumulatorSeconds,
						RESOURCE_INTERVAL_SECONDS);
				if (bucketEndSecond < 0)
				{
					QuarantinePool(pool, "enemy resource cadence precedes campaign clock");
					break;
				}
				int attackIncome;
				int supportIncome;
				string contributionHash;
				string calculationFailure;
				if (!CalculateIncomeForFaction(
					state,
					balance,
					pool.m_sFactionKey,
					attackIncome,
					supportIncome,
					contributionHash,
					calculationFailure))
				{
					QuarantinePool(pool, calculationFailure);
					break;
				}
				bool bucketAccepted = true;
				string bucketFailure;
				if (attackIncome > 0 || supportIncome > 0)
				{
					string sourceId = string.Format(
						"enemy_income_bucket_%1_%2",
						pool.m_sFactionKey.Hash(),
						bucketEndSecond);
					HST_EnemyStrategicMutationCommand command = NewCommand(
						BuildMutationId(
							KIND_PERIODIC_INCOME,
							pool.m_sFactionKey,
							sourceId),
						pool.m_sFactionKey,
						KIND_PERIODIC_INCOME,
						sourceId,
						"",
						"",
						"",
						"");
					command.m_iCreatedAtSecond = bucketEndSecond;
					command.m_iAttackDelta = attackIncome;
					command.m_iSupportDelta = supportIncome;
					command.m_sContributionHash = contributionHash;
					m_bApplyingPeriodicBucket = true;
					HST_EnemyStrategicMutationResult result
						= ApplyMutation(state, preset, command);
					m_bApplyingPeriodicBucket = false;
					bucketAccepted = result && result.m_bAccepted;
					if (result)
						bucketFailure = result.m_sFailureReason;
				}
				if (!bucketAccepted)
				{
					QuarantinePool(
						pool,
						"enemy resource periodic bucket rejected: " + bucketFailure);
					break;
				}
				pool.m_iResourceAccumulatorSeconds
					-= RESOURCE_INTERVAL_SECONDS;
				pool.m_iLastResourceBucketSecond = bucketEndSecond;
				processed = true;
			}
			if (pool.m_iStrategicContractVersion == CONTRACT_VERSION
				&& pool.m_iLastResourceBucketSecond
					!= state.m_iElapsedSeconds - pool.m_iResourceAccumulatorSeconds)
				QuarantinePool(pool, "enemy resource cadence checkpoint did not close");
		}
		return processed;
	}

	bool TickAggressionDecay(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		int elapsedSeconds)
	{
		if (!state || !preset || !balance || elapsedSeconds <= 0
			|| !ValidatePresetRoles(preset))
			return false;
		int interval = Math.Max(
			60,
			balance.m_iAggressionDecayIntervalSeconds);
		int decayAmount = Math.Max(0, balance.m_iAggressionDecayAmount);

		bool processed;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || !HST_FactionRelationService.IsEnemyFaction(
				preset,
				pool.m_sFactionKey))
				continue;
			HST_FactionPoolState uniquePool;
			string poolFailure = ResolveExactEnemyPool(
				state,
				preset,
				pool.m_sFactionKey,
				uniquePool);
			if (!poolFailure.IsEmpty() || uniquePool != pool)
			{
				if (pool.m_iStrategicContractVersion
					!= QUARANTINE_CONTRACT_VERSION)
				{
					if (poolFailure.Contains("duplicated"))
						QuarantineFactionPools(state, pool.m_sFactionKey, poolFailure);
					else
						QuarantinePool(pool, poolFailure);
				}
				continue;
			}
			int previousElapsedSecond = state.m_iElapsedSeconds - elapsedSeconds;
			if (pool.m_iLastAggressionBucketSecond == 0
				&& pool.m_iAggressionAccumulatorSeconds == 0
				&& !HasPeriodicMutation(
					state,
					pool.m_sFactionKey,
					KIND_AGGRESSION_DECAY))
				pool.m_iLastAggressionBucketSecond = previousElapsedSecond;
			if (previousElapsedSecond < 0
				|| pool.m_iLastAggressionBucketSecond
					!= previousElapsedSecond - pool.m_iAggressionAccumulatorSeconds)
			{
				QuarantinePool(pool, "enemy aggression cadence checkpoint diverged");
				continue;
			}
			string checkpointFailure = ValidatePeriodicCheckpoint(
				state,
				pool.m_sFactionKey,
				KIND_AGGRESSION_DECAY,
				pool.m_iLastAggressionBucketSecond);
			if (!checkpointFailure.IsEmpty())
			{
				QuarantinePool(pool, checkpointFailure);
				continue;
			}
			int nextAggressionAccumulator;
			if (!TryAccumulate(
				pool.m_iAggressionAccumulatorSeconds,
				elapsedSeconds,
				nextAggressionAccumulator))
			{
				QuarantinePool(pool, "enemy aggression cadence accumulator overflow");
				continue;
			}
			pool.m_iAggressionAccumulatorSeconds = nextAggressionAccumulator;
			int due = pool.m_iAggressionAccumulatorSeconds / interval;
			int steps = Math.Min(due, MAX_CATCHUP_STEPS_PER_TICK);
			for (int stepIndex = 0; stepIndex < steps; stepIndex++)
			{
				int bucketEndSecond = ResolveOldestPendingBucketEnd(
					state,
					pool.m_iAggressionAccumulatorSeconds,
					interval);
				if (bucketEndSecond < 0)
				{
					QuarantinePool(pool, "enemy aggression cadence precedes campaign clock");
					break;
				}
				int appliedDecay = Math.Min(pool.m_iAggression, decayAmount);
				bool bucketAccepted = true;
				string bucketFailure;
				if (appliedDecay > 0)
				{
					string sourceId = string.Format(
						"enemy_aggression_decay_%1_%2_%3",
						pool.m_sFactionKey.Hash(),
						bucketEndSecond,
						interval);
					HST_EnemyStrategicMutationCommand command = NewCommand(
						BuildMutationId(
							KIND_AGGRESSION_DECAY,
							pool.m_sFactionKey,
							sourceId),
						pool.m_sFactionKey,
						KIND_AGGRESSION_DECAY,
						sourceId,
						"",
						"",
						"",
						"");
					command.m_iCreatedAtSecond = bucketEndSecond;
					command.m_iAggressionDelta = -appliedDecay;
					command.m_sContributionHash = BuildDecayContributionHash(
						interval,
						decayAmount);
					m_bApplyingPeriodicBucket = true;
					HST_EnemyStrategicMutationResult result
						= ApplyMutation(state, preset, command);
					m_bApplyingPeriodicBucket = false;
					bucketAccepted = result && result.m_bAccepted;
					if (result)
						bucketFailure = result.m_sFailureReason;
				}
				if (!bucketAccepted)
				{
					QuarantinePool(
						pool,
						"enemy aggression periodic bucket rejected: " + bucketFailure);
					break;
				}
				pool.m_iAggressionAccumulatorSeconds -= interval;
				pool.m_iLastAggressionBucketSecond = bucketEndSecond;
				processed = true;
			}
			if (pool.m_iStrategicContractVersion == CONTRACT_VERSION
				&& pool.m_iLastAggressionBucketSecond
					!= state.m_iElapsedSeconds - pool.m_iAggressionAccumulatorSeconds)
				QuarantinePool(pool, "enemy aggression cadence checkpoint did not close");
		}
		return processed;
	}

	string BuildReport(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		int maxRows = 16)
	{
		if (!state || !preset)
			return "Partisan enemy strategic resources | state or preset unavailable";
		int exactPools;
		int quarantinedPools;
		int operationalReceipts;
		int periodicReceipts;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (!mutation)
				continue;
			if (IsPeriodicKind(mutation.m_sKind))
				periodicReceipts++;
			else
				operationalReceipts++;
		}
		string rows = "";
		int rowCount;
		int boundedRows = Math.Max(0, Math.Min(32, maxRows));
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || !HST_FactionRelationService.IsEnemyFaction(
				preset,
				pool.m_sFactionKey))
				continue;
			if (pool.m_iStrategicContractVersion == CONTRACT_VERSION)
				exactPools++;
			else if (pool.m_iStrategicContractVersion
				== QUARANTINE_CONTRACT_VERSION)
				quarantinedPools++;
			if (rowCount >= boundedRows)
				continue;
			rows = rows + string.Format(
				"\n%1 | contract/revision %2/%3 | attack/support/aggression %4/%5/%6 | cadence %7/%8 | last %9",
				pool.m_sFactionKey,
				pool.m_iStrategicContractVersion,
				pool.m_iStrategicRevision,
				pool.m_iAttackResources,
				pool.m_iSupportResources,
				pool.m_iAggression,
				pool.m_iResourceAccumulatorSeconds,
				pool.m_iAggressionAccumulatorSeconds,
				pool.m_sLastStrategicMutationId);
			rows = rows + string.Format(
				" | operational %1/%2 | bucket resource/aggression %3/%4",
				pool.m_iStrategicOperationalMutationCount,
				MAX_OPERATIONAL_MUTATIONS,
				pool.m_iLastResourceBucketSecond,
				pool.m_iLastAggressionBucketSecond);
			if (!pool.m_sStrategicAuthorityFailure.IsEmpty())
				rows = rows + " | failure " + pool.m_sStrategicAuthorityFailure;
			rowCount++;
		}
		return string.Format(
			"Partisan enemy strategic resources | policy %1 | exact/quarantined pools %2/%3 | operational/periodic receipts %4/%5",
			EXACT_POLICY_ID,
			exactPools,
			quarantinedPools,
			operationalReceipts,
			periodicReceipts) + rows;
	}

	protected HST_EnemyStrategicMutationCommand NewCommand(
		string mutationId,
		string factionKey,
		string kind,
		string sourceId,
		string orderId,
		string operationId,
		string manifestId,
		string zoneId)
	{
		HST_EnemyStrategicMutationCommand command
			= new HST_EnemyStrategicMutationCommand();
		command.m_sMutationId = mutationId;
		command.m_sFactionKey = factionKey;
		command.m_sKind = kind;
		command.m_sSourceId = sourceId;
		if (command.m_sSourceId.Trim().IsEmpty())
			command.m_sSourceId = mutationId;
		command.m_sOrderId = orderId;
		command.m_sOperationId = operationId;
		command.m_sManifestId = manifestId;
		command.m_sZoneId = zoneId;
		return command;
	}

	protected string NormalizeAndValidateCommand(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EnemyStrategicMutationCommand command)
	{
		if (!state || !preset || !command)
			return "enemy strategic mutation authority is unavailable";
		if (!ValidatePresetRoles(preset))
			return "configured faction roles are invalid";
		if (!state.m_aEnemyStrategicMutations)
			state.m_aEnemyStrategicMutations
				= new array<ref HST_EnemyStrategicMutationState>();
		command.m_sMutationId = command.m_sMutationId.Trim();
		command.m_sFactionKey = command.m_sFactionKey.Trim();
		command.m_sKind = command.m_sKind.Trim();
		command.m_sSourceId = command.m_sSourceId.Trim();
		command.m_sOrderId = command.m_sOrderId.Trim();
		command.m_sOperationId = command.m_sOperationId.Trim();
		command.m_sManifestId = command.m_sManifestId.Trim();
		command.m_sZoneId = command.m_sZoneId.Trim();
		command.m_sContributionHash = command.m_sContributionHash.Trim();
		if (!IsBoundedRequiredId(command.m_sMutationId)
			|| !IsBoundedRequiredId(command.m_sFactionKey)
			|| !IsBoundedRequiredId(command.m_sKind)
			|| !IsBoundedRequiredId(command.m_sSourceId))
			return "enemy strategic mutation identity is invalid";
		if (!IsBoundedOptionalId(command.m_sOrderId)
			|| !IsBoundedOptionalId(command.m_sOperationId)
			|| !IsBoundedOptionalId(command.m_sManifestId)
			|| !IsBoundedOptionalId(command.m_sZoneId)
			|| command.m_sContributionHash.Length()
				> MAX_CONTRIBUTION_CHARACTERS)
			return "enemy strategic mutation link or contribution identity is invalid";
		if (!HST_FactionRelationService.IsEnemyFaction(
			preset,
			command.m_sFactionKey))
			return "enemy strategic mutation target is not a configured enemy faction";
		if (IsPeriodicKind(command.m_sKind) && !m_bApplyingPeriodicBucket)
			return "periodic enemy strategic kinds are reserved for cadence authority";
		if (!IsBoundedDelta(command.m_iAttackDelta)
			|| !IsBoundedDelta(command.m_iSupportDelta)
			|| !IsBoundedDelta(command.m_iAggressionDelta))
			return "enemy strategic mutation delta is outside the bounded range";
		if (IsPeriodicKind(command.m_sKind)
			&& command.m_iAttackDelta == 0
			&& command.m_iSupportDelta == 0
			&& command.m_iAggressionDelta == 0)
			return "periodic enemy strategic mutation has no effect";
		if (command.m_iCreatedAtSecond > state.m_iElapsedSeconds)
			return "enemy strategic mutation creation time is in the future";
		return "";
	}

	protected string ResolveExactEnemyPool(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string factionKey,
		out HST_FactionPoolState pool)
	{
		pool = null;
		if (!state || !preset || factionKey.IsEmpty())
			return "enemy strategic pool identity is unavailable";
		int count;
		foreach (HST_FactionPoolState candidate : state.m_aFactionPools)
		{
			if (!candidate || candidate.m_sFactionKey != factionKey)
				continue;
			pool = candidate;
			count++;
		}
		if (count == 0)
			return "configured enemy strategic pool is missing";
		if (count != 1)
			return "configured enemy strategic pool is duplicated";
		if (pool.m_iStrategicContractVersion == QUARANTINE_CONTRACT_VERSION
			|| !pool.m_sStrategicAuthorityFailure.IsEmpty())
			return "configured enemy strategic pool is quarantined";
		if (pool.m_iStrategicContractVersion != CONTRACT_VERSION)
			return "configured enemy strategic pool has not been adopted";
		if (pool.m_iStrategicRevision <= 0
			|| pool.m_iStrategicRevision >= MAX_MUTABLE_REVISION
			|| pool.m_iStrategicOperationalMutationCount < 0
			|| pool.m_iStrategicOperationalMutationCount > MAX_OPERATIONAL_MUTATIONS
			|| pool.m_iAttackResources < 0
			|| pool.m_iSupportResources < 0
			|| pool.m_iAggression < 0
			|| pool.m_iResourceAccumulatorSeconds < 0
			|| pool.m_iAggressionAccumulatorSeconds < 0
			|| pool.m_iLastResourceBucketSecond < 0
			|| pool.m_iLastAggressionBucketSecond < 0)
			return "configured enemy strategic pool is corrupt";
		return "";
	}

	protected string ValidateNewMutationAdmission(
		HST_CampaignState state,
		HST_FactionPoolState pool,
		HST_EnemyStrategicMutationCommand command)
	{
		if (!state || !pool || !command)
			return "enemy strategic mutation admission authority is unavailable";
		if (pool.m_iStrategicRevision >= MAX_MUTABLE_REVISION)
			return "enemy strategic pool revision authority is exhausted";
		if (IsPeriodicKind(command.m_sKind))
		{
			if (CountOperationalMutations(state) > MAX_TOTAL_OPERATIONAL_MUTATIONS)
				return "enemy strategic operational receipt history is corrupt";
			if (CountPeriodicMutations(state) > MAX_PERIODIC_MUTATIONS)
				return "enemy strategic periodic cadence evidence is corrupt";
			if (state.m_aEnemyStrategicMutations.Count() > MAX_MUTATION_ROWS
				|| (state.m_aEnemyStrategicMutations.Count() == MAX_MUTATION_ROWS
					&& !HasPeriodicMutation(
						state,
						pool.m_sFactionKey,
						command.m_sKind)))
				return "bounded enemy strategic mutation history is full";
			return "";
		}
		if (pool.m_iStrategicOperationalMutationCount >= MAX_OPERATIONAL_MUTATIONS)
			return "bounded faction strategic operational receipt history is full";
		if (CountOperationalMutationsForFaction(state, pool.m_sFactionKey)
			!= pool.m_iStrategicOperationalMutationCount)
			return "faction strategic operational receipt sequence is corrupt";
		if (state.m_aEnemyStrategicMutations.Count() >= MAX_MUTATION_ROWS)
			return "bounded enemy strategic mutation history is corrupt";
		return "";
	}

	protected HST_EnemyStrategicMutationState BuildMutation(
		HST_CampaignState state,
		HST_FactionPoolState pool,
		HST_EnemyStrategicMutationCommand command,
		int attackAfter,
		int supportAfter,
		int aggressionAfter)
	{
		if (!state || !pool || !command)
			return null;
		HST_EnemyStrategicMutationState mutation
			= new HST_EnemyStrategicMutationState();
		mutation.m_iContractVersion = CONTRACT_VERSION;
		mutation.m_sMutationId = command.m_sMutationId;
		mutation.m_sFactionKey = command.m_sFactionKey;
		mutation.m_sKind = command.m_sKind;
		mutation.m_sSourceId = command.m_sSourceId;
		mutation.m_sOrderId = command.m_sOrderId;
		mutation.m_sOperationId = command.m_sOperationId;
		mutation.m_sManifestId = command.m_sManifestId;
		mutation.m_sZoneId = command.m_sZoneId;
		mutation.m_iCreatedAtSecond = command.m_iCreatedAtSecond;
		if (mutation.m_iCreatedAtSecond < 0)
			mutation.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		mutation.m_iPoolRevisionBefore = pool.m_iStrategicRevision;
		mutation.m_iPoolRevisionAfter = pool.m_iStrategicRevision + 1;
		if (IsPeriodicKind(command.m_sKind))
			mutation.m_iOperationalSequence = 0;
		else
			mutation.m_iOperationalSequence
				= pool.m_iStrategicOperationalMutationCount + 1;
		mutation.m_iAttackBefore = pool.m_iAttackResources;
		mutation.m_iAttackDelta = command.m_iAttackDelta;
		mutation.m_iAttackAfter = attackAfter;
		mutation.m_iSupportBefore = pool.m_iSupportResources;
		mutation.m_iSupportDelta = command.m_iSupportDelta;
		mutation.m_iSupportAfter = supportAfter;
		mutation.m_iAggressionBefore = pool.m_iAggression;
		mutation.m_iAggressionDelta = command.m_iAggressionDelta;
		mutation.m_iAggressionAfter = aggressionAfter;
		mutation.m_sContributionHash = command.m_sContributionHash;
		mutation.m_bApplied = true;
		mutation.m_sFingerprint = BuildMutationFingerprint(mutation);
		return mutation;
	}

	protected bool ExistingMatchesCommand(
		HST_EnemyStrategicMutationState existing,
		HST_EnemyStrategicMutationCommand command)
	{
		if (!existing || !command)
			return false;
		if (existing.m_sMutationId != command.m_sMutationId
			|| existing.m_sFactionKey != command.m_sFactionKey
			|| existing.m_sKind != command.m_sKind
			|| existing.m_sSourceId != command.m_sSourceId)
			return false;
		if (existing.m_sOrderId != command.m_sOrderId
			|| existing.m_sOperationId != command.m_sOperationId
			|| existing.m_sManifestId != command.m_sManifestId
			|| existing.m_sZoneId != command.m_sZoneId)
			return false;
		if (existing.m_iAttackDelta != command.m_iAttackDelta
			|| existing.m_iSupportDelta != command.m_iSupportDelta
			|| existing.m_iAggressionDelta != command.m_iAggressionDelta)
			return false;
		return existing.m_sContributionHash == command.m_sContributionHash;
	}

	protected bool ExistingMatchesPool(
		HST_EnemyStrategicMutationState existing,
		HST_FactionPoolState pool)
	{
		if (!existing || !pool || existing.m_sFactionKey != pool.m_sFactionKey
			|| existing.m_iPoolRevisionAfter > pool.m_iStrategicRevision)
			return false;
		if (!IsPeriodicKind(existing.m_sKind)
			&& (existing.m_iOperationalSequence <= 0
				|| existing.m_iOperationalSequence
					> pool.m_iStrategicOperationalMutationCount))
			return false;
		if (existing.m_iPoolRevisionAfter != pool.m_iStrategicRevision)
			return true;
		return pool.m_sLastStrategicMutationId == existing.m_sMutationId
			&& pool.m_iAttackResources == existing.m_iAttackAfter
			&& pool.m_iSupportResources == existing.m_iSupportAfter
			&& pool.m_iAggression == existing.m_iAggressionAfter;
	}

	protected bool FindUniqueMutation(
		HST_CampaignState state,
		string mutationId,
		out HST_EnemyStrategicMutationState match)
	{
		match = null;
		if (!state || mutationId.IsEmpty())
			return false;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (!mutation || mutation.m_sMutationId != mutationId)
				continue;
			if (match)
				return false;
			match = mutation;
		}
		return true;
	}

	protected string ResolvePeriodicCompactionIndex(
		HST_CampaignState state,
		HST_EnemyStrategicMutationCommand command,
		out int compactedIndex)
	{
		compactedIndex = -1;
		if (!state || !command || !IsPeriodicKind(command.m_sKind))
			return "periodic cadence compaction authority is unavailable";
		for (int mutationIndex = 0; mutationIndex < state.m_aEnemyStrategicMutations.Count(); mutationIndex++)
		{
			HST_EnemyStrategicMutationState candidate
				= state.m_aEnemyStrategicMutations[mutationIndex];
			if (!candidate || candidate.m_sFactionKey != command.m_sFactionKey
				|| candidate.m_sKind != command.m_sKind)
				continue;
			if (compactedIndex >= 0)
				return "periodic cadence evidence is duplicated";
			if (candidate.m_iCreatedAtSecond >= command.m_iCreatedAtSecond)
				return "periodic cadence evidence would regress";
			compactedIndex = mutationIndex;
		}
		return "";
	}

	protected bool CalculateIncomeForFaction(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		string factionKey,
		out int attackIncome,
		out int supportIncome,
		out string contributionHash,
		out string failure)
	{
		attackIncome = 0;
		supportIncome = 0;
		contributionHash = "";
		failure = "";
		if (!state || !balance || factionKey.IsEmpty())
		{
			failure = "enemy income calculation authority is unavailable";
			return false;
		}
		array<string> contributions = {};
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != factionKey)
				continue;
			int zoneAttack;
			int zoneSupport;
			if (!TryResolveEnemyAttackIncome(state, balance, zone, zoneAttack)
				|| !TryResolveEnemySupportIncome(state, balance, zone, zoneSupport))
			{
				failure = "enemy income scaling arithmetic is invalid or overflowed";
				return false;
			}
			if (!TryAddNonnegative(attackIncome, zoneAttack, attackIncome)
				|| !TryAddNonnegative(supportIncome, zoneSupport, supportIncome))
			{
				failure = "enemy income contribution would overflow";
				return false;
			}
			int signedSupport;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && m_TownInfluence)
				signedSupport = m_TownInfluence.ResolveSignedSupportPercent(
					state,
					zone.m_sZoneId);
			contributions.Insert(string.Format(
				"%1|%2|%3|%4|%5|%6|%7|%8",
				zone.m_sZoneId,
				zone.m_eType,
				zone.m_iIncomeValue,
				zone.m_iPriority,
				zone.m_sResourceKind,
				signedSupport,
				zoneAttack,
				zoneSupport));
		}
		contributions.Sort();
		string canonical = string.Format(
			"%1|%2|%3|%4|%5|%6",
			EXACT_POLICY_ID,
			factionKey,
			state.m_iWarLevel,
			balance.m_iEnemyAttackIncomeWarPercent,
			balance.m_iEnemySupportIncomeWarPercent,
			contributions.Count());
		foreach (string contribution : contributions)
			canonical = canonical + "|" + contribution;
		contributionHash = string.Format(
			"eic1_%1_%2",
			canonical.Hash(),
			(canonical + "|secondary").Hash());
		return true;
	}

	protected bool TryResolveEnemyAttackIncome(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		out int resolvedIncome)
	{
		resolvedIncome = 0;
		int income;
		if (!TryAddNonnegative(
			Math.Max(1, zone.m_iIncomeValue / 60),
			Math.Max(0, zone.m_iPriority / 10),
			income))
			return false;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD
			|| zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
		{
			if (!TryAddNonnegative(income, 3, income))
				return false;
		}
		else if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
		{
			if (!TryAddNonnegative(income, 2, income))
				return false;
		}
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE
			&& zone.m_sResourceKind == "fuel")
		{
			if (!TryAddNonnegative(income, 2, income))
				return false;
		}
		return TryScaleIncome(
			income,
			state.m_iWarLevel,
			balance.m_iEnemyAttackIncomeWarPercent,
			resolvedIncome);
	}

	protected bool TryResolveEnemySupportIncome(
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		out int resolvedIncome)
	{
		resolvedIncome = 0;
		int income = 1;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE
			&& zone.m_sResourceKind == "supplies")
		{
			if (!TryAddNonnegative(income, 2, income))
				return false;
		}
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER
			|| zone.m_eType == HST_EZoneType.HST_ZONE_POLICE_STATION)
		{
			if (!TryAddNonnegative(income, 1, income))
				return false;
		}
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && m_TownInfluence)
		{
			int signedSupport = m_TownInfluence.ResolveSignedSupportPercent(
				state,
				zone.m_sZoneId);
			if (signedSupport < -100 || signedSupport > 100)
				return false;
			if (!TryAddNonnegative(
				income,
				Math.Max(0, 1 - signedSupport / 50),
				income))
				return false;
		}
		return TryScaleIncome(
			income,
			state.m_iWarLevel,
			balance.m_iEnemySupportIncomeWarPercent,
			resolvedIncome);
	}

	protected bool TryScaleIncome(
		int baseIncome,
		int warLevel,
		int warPercent,
		out int scaledIncome)
	{
		scaledIncome = 0;
		int boundedWarLevel = Math.Max(0, warLevel);
		int boundedWarPercent = Math.Max(0, warPercent);
		if (boundedWarLevel > 0
			&& boundedWarPercent > int.MAX / boundedWarLevel)
			return false;
		int warBonus = boundedWarLevel * boundedWarPercent;
		if (warBonus > int.MAX - 100)
			return false;
		int multiplier = 100 + warBonus;
		if (baseIncome <= 0 || baseIncome > int.MAX / multiplier)
			return false;
		int scaledNumerator = baseIncome * multiplier;
		scaledIncome = scaledNumerator / 100;
		if (scaledNumerator % 100 >= 50)
		{
			if (scaledIncome == int.MAX)
				return false;
			scaledIncome++;
		}
		scaledIncome = Math.Max(1, scaledIncome);
		return scaledIncome > 0;
	}

	protected string BuildDecayContributionHash(int interval, int amount)
	{
		string canonical = string.Format(
			"%1|%2|%3|%4",
			EXACT_POLICY_ID,
			KIND_AGGRESSION_DECAY,
			interval,
			amount);
		return string.Format(
			"ead1_%1_%2",
			canonical.Hash(),
			(canonical + "|secondary").Hash());
	}

	protected int ResolveOldestPendingBucketEnd(
		HST_CampaignState state,
		int accumulatorSeconds,
		int intervalSeconds)
	{
		if (!state || accumulatorSeconds < intervalSeconds
			|| state.m_iElapsedSeconds < accumulatorSeconds)
			return -1;
		return state.m_iElapsedSeconds - accumulatorSeconds + intervalSeconds;
	}

	protected bool TryAccumulate(
		int accumulator,
		int elapsedSeconds,
		out int accumulated)
	{
		accumulated = accumulator;
		if (elapsedSeconds <= 0 || accumulator < 0
			|| accumulator > int.MAX - elapsedSeconds)
			return false;
		accumulated = accumulator + elapsedSeconds;
		return true;
	}

	protected bool TryApplyNonnegativeDelta(
		int before,
		int delta,
		out int after)
	{
		after = before;
		if (before < 0 || delta == int.MIN)
			return false;
		if (delta > 0 && before > int.MAX - delta)
			return false;
		if (delta < 0 && before < -delta)
			return false;
		after = before + delta;
		return after >= 0;
	}

	protected bool TryAddNonnegative(int left, int right, out int total)
	{
		total = left;
		if (left < 0 || right < 0 || left > int.MAX - right)
			return false;
		total = left + right;
		return true;
	}

	protected bool FindPeriodicMutation(
		HST_CampaignState state,
		string factionKey,
		string kind,
		out HST_EnemyStrategicMutationState match)
	{
		match = null;
		if (!state || factionKey.IsEmpty() || !IsPeriodicKind(kind))
			return false;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (!mutation || mutation.m_sFactionKey != factionKey
				|| mutation.m_sKind != kind)
				continue;
			if (match)
				return false;
			match = mutation;
		}
		return true;
	}

	protected bool HasPeriodicMutation(
		HST_CampaignState state,
		string factionKey,
		string kind)
	{
		HST_EnemyStrategicMutationState ignored;
		return FindPeriodicMutation(state, factionKey, kind, ignored) && ignored;
	}

	protected string ValidatePeriodicCheckpoint(
		HST_CampaignState state,
		string factionKey,
		string kind,
		int lastBucketSecond)
	{
		int count;
		HST_EnemyStrategicMutationState retained;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (!mutation || mutation.m_iContractVersion != CONTRACT_VERSION
				|| !mutation.m_bApplied
				|| mutation.m_sFactionKey != factionKey
				|| mutation.m_sKind != kind)
				continue;
			retained = mutation;
			count++;
		}
		if (count > 1)
			return "enemy periodic cadence evidence is duplicated";
		if (retained && retained.m_iCreatedAtSecond > lastBucketSecond)
			return "enemy periodic cadence checkpoint precedes retained evidence";
		return "";
	}

	protected int CountOperationalMutations(HST_CampaignState state)
	{
		int count;
		if (!state)
			return count;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_iContractVersion == CONTRACT_VERSION
				&& mutation.m_bApplied
				&& !IsPeriodicKind(mutation.m_sKind))
				count++;
		}
		return count;
	}

	protected int CountOperationalMutationsForFaction(
		HST_CampaignState state,
		string factionKey)
	{
		int count;
		if (!state || factionKey.IsEmpty())
			return count;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_iContractVersion == CONTRACT_VERSION
				&& mutation.m_bApplied
				&& mutation.m_sFactionKey == factionKey
				&& !IsPeriodicKind(mutation.m_sKind))
				count++;
		}
		return count;
	}

	protected int CountPeriodicMutations(HST_CampaignState state)
	{
		int count;
		if (!state)
			return count;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_iContractVersion == CONTRACT_VERSION
				&& mutation.m_bApplied
				&& IsPeriodicKind(mutation.m_sKind))
				count++;
		}
		return count;
	}

	static bool IsPeriodicKind(string kind)
	{
		return kind == KIND_PERIODIC_INCOME || kind == KIND_AGGRESSION_DECAY;
	}

	protected bool ValidatePresetRoles(HST_CampaignPreset preset)
	{
		if (!preset || preset.m_sResistanceFactionKey.Trim().IsEmpty()
			|| preset.m_sOccupierFactionKey.Trim().IsEmpty())
			return false;
		if (preset.m_sResistanceFactionKey != preset.m_sResistanceFactionKey.Trim()
			|| preset.m_sOccupierFactionKey != preset.m_sOccupierFactionKey.Trim()
			|| preset.m_sInvaderFactionKey != preset.m_sInvaderFactionKey.Trim()
			|| preset.m_sResistanceFactionKey.Length() > MAX_ID_CHARACTERS
			|| preset.m_sOccupierFactionKey.Length() > MAX_ID_CHARACTERS
			|| preset.m_sInvaderFactionKey.Length() > MAX_ID_CHARACTERS)
			return false;
		if (preset.m_sResistanceFactionKey == preset.m_sOccupierFactionKey)
			return false;
		if (!preset.m_sInvaderFactionKey.IsEmpty()
			&& (preset.m_sInvaderFactionKey == preset.m_sResistanceFactionKey
				|| preset.m_sInvaderFactionKey == preset.m_sOccupierFactionKey))
			return false;
		return true;
	}

	protected bool IsBoundedRequiredId(string value)
	{
		return !value.IsEmpty() && value.Length() <= MAX_ID_CHARACTERS;
	}

	protected bool IsBoundedOptionalId(string value)
	{
		return value.Length() <= MAX_ID_CHARACTERS;
	}

	protected bool IsBoundedDelta(int value)
	{
		return value != int.MIN
			&& value >= -MAX_DELTA_MAGNITUDE
			&& value <= MAX_DELTA_MAGNITUDE;
	}

	protected void QuarantineFactionPools(
		HST_CampaignState state,
		string factionKey,
		string failure)
	{
		if (!state || factionKey.IsEmpty())
			return;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (pool && pool.m_sFactionKey == factionKey)
				QuarantinePool(pool, failure);
		}
	}

	protected void QuarantineMutationClaimantPools(
		HST_CampaignState state,
		string mutationId,
		string failure)
	{
		if (!state || mutationId.IsEmpty())
			return;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_sMutationId == mutationId)
				QuarantineFactionPools(
					state,
					mutation.m_sFactionKey,
					failure);
		}
	}

	protected void QuarantinePool(HST_FactionPoolState pool, string failure)
	{
		if (!pool)
			return;
		pool.m_iStrategicContractVersion = QUARANTINE_CONTRACT_VERSION;
		pool.m_sStrategicAuthorityFailure = LimitText(
			failure.Trim(),
			MAX_FAILURE_CHARACTERS);
	}

	protected string LimitText(string value, int maxCharacters)
	{
		if (value.Length() <= maxCharacters)
			return value;
		return value.Substring(0, maxCharacters);
	}

	protected HST_EnemyStrategicMutationResult BuildNoopResult(
		HST_CampaignState state,
		string factionKey)
	{
		HST_EnemyStrategicMutationResult result
			= new HST_EnemyStrategicMutationResult();
		result.m_bAccepted = true;
		if (state)
			result.m_Pool = state.FindFactionPool(factionKey);
		return result;
	}

	protected HST_EnemyStrategicMutationResult BuildRejectedResult(string failure)
	{
		HST_EnemyStrategicMutationResult result
			= new HST_EnemyStrategicMutationResult();
		result.m_sFailureReason = failure;
		return result;
	}

	protected HST_EnemyStrategicMutationResult Reject(
		HST_EnemyStrategicMutationResult result,
		string failure,
		HST_FactionPoolState pool = null,
		HST_EnemyStrategicMutationState mutation = null)
	{
		if (!result)
			return BuildRejectedResult(failure);
		result.m_Pool = pool;
		result.m_Mutation = mutation;
		result.m_sFailureReason = LimitText(
			failure.Trim(),
			MAX_FAILURE_CHARACTERS);
		return result;
	}
}
