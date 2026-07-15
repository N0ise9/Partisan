// Schema-67 persistence boundary for configured enemy strategic resources.
// Save-shape validation proves arithmetic and receipt chains. Faction roles are
// deliberately validated after restore because preset roles are not serialized.
class HST_EnemyStrategicResourceSaveValidationService
{
	static const int SCHEMA_VERSION = 67;
	static const int CONTRACT_VERSION = 1;
	static const int QUARANTINE_CONTRACT_VERSION = -67;
	static const int MAX_OPERATIONAL_MUTATIONS = 4096;
	static const int MAX_TOTAL_OPERATIONAL_MUTATIONS = MAX_OPERATIONAL_MUTATIONS * 2;
	static const int MAX_PERIODIC_MUTATIONS = 4;
	static const int MAX_MUTATION_ROWS = MAX_TOTAL_OPERATIONAL_MUTATIONS + MAX_PERIODIC_MUTATIONS;
	static const int MAX_MUTABLE_REVISION = int.MAX - 16;
	static const int MAX_ID_CHARACTERS = 192;
	static const int MAX_FAILURE_CHARACTERS = 320;
	static const int MAX_CONTRIBUTION_CHARACTERS = 192;
	static const int MAX_DELTA_MAGNITUDE = 1000000000;
	protected HST_CampaignSaveData m_SaveData;
	protected bool m_bPrepared;
	protected int m_iRestoredSchemaVersion;

	void PrepareBeforeGenericNormalization(
		HST_CampaignSaveData saveData,
		int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		m_iRestoredSchemaVersion = restoredSchemaVersion;
		if (!m_SaveData)
			return;
		EnsureArrays();
		if (m_bPrepared)
			return;
		m_bPrepared = true;
		if (restoredSchemaVersion < SCHEMA_VERSION)
		{
			PrepareLegacyPoolSnapshots();
			RejectUnexpectedPre67Receipts();
		}
	}

	void Normalize(
		HST_CampaignSaveData saveData,
		int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		m_iRestoredSchemaVersion = restoredSchemaVersion;
		if (!m_SaveData)
			return;
		EnsureArrays();
		if (!m_bPrepared)
			PrepareBeforeGenericNormalization(saveData, restoredSchemaVersion);
		if (restoredSchemaVersion < SCHEMA_VERSION)
			return;

		bool nullReceiptFound;
		for (int nullIndex = m_SaveData.m_aEnemyStrategicMutations.Count() - 1; nullIndex >= 0; nullIndex--)
		{
			if (m_SaveData.m_aEnemyStrategicMutations[nullIndex])
				continue;
			m_SaveData.m_aEnemyStrategicMutations.Remove(nullIndex);
			nullReceiptFound = true;
		}
		if (nullReceiptFound)
			QuarantineAllExactPools("schema67 enemy strategic receipt array contained a null row");

		ValidatePoolIdentities();
		// Saves produced by an earlier contract-67 build can already contain
		// explicit quarantine tombstones. They are not evidence and must not be
		// allowed to trip the physical bound before current rows are validated.
		PurgePreviouslyQuarantinedSavedReceipts();
		int operationalCount;
		int periodicCount;
		foreach (HST_EnemyStrategicMutationState mutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (!mutation)
				continue;
			if (HST_EnemyStrategicResourceService.IsPeriodicKind(mutation.m_sKind))
				periodicCount++;
			else
				operationalCount++;
		}
		if (operationalCount > MAX_TOTAL_OPERATIONAL_MUTATIONS
			|| periodicCount > MAX_PERIODIC_MUTATIONS
			|| m_SaveData.m_aEnemyStrategicMutations.Count() > MAX_MUTATION_ROWS)
		{
			QuarantineAllExactPools("schema67 enemy strategic receipt bound exceeded");
			QuarantineAllReceipts("schema67 enemy strategic receipt bound exceeded");
			PurgeRejectedSavedReceipts();
			return;
		}

		ValidateReceiptIdentitiesAndShapes();
		ValidatePoolReceiptChains();
		// Quarantine is an attribution decision, not a durable receipt state.
		// Keeping rejected rows in the canonical array would let malformed or
		// orphan evidence consume another faction's bounded admission capacity.
		PurgeRejectedSavedReceipts();
	}

	// Pre-schema-67 saves carry only a pool snapshot. Adoption happens here,
	// after the live preset supplies the configured enemy roles. No historical
	// mutation receipt is invented for that snapshot.
	bool ValidateRestoredFactionRoles(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		int restoredSchemaVersion = -1)
	{
		if (!state || !preset || !ValidatePresetRoles(preset))
			return false;
		if (!state.m_aEnemyStrategicMutations)
			state.m_aEnemyStrategicMutations
				= new array<ref HST_EnemyStrategicMutationState>();
		int effectiveSchema = restoredSchemaVersion;
		if (effectiveSchema < 0)
			effectiveSchema = state.m_iLastLoadedSchemaVersion;

		bool valid = true;
		if (effectiveSchema < SCHEMA_VERSION)
		{
			if (!state.m_aEnemyStrategicMutations.IsEmpty())
			{
				QuarantineRuntimeEnemyPools(
					state,
					preset,
					"pre-schema67 restore carried unsupported strategic receipts");
				return false;
			}
			valid = AdoptLegacyEnemyPool(
				state,
				preset.m_sOccupierFactionKey)
				&& valid;
			if (!preset.m_sInvaderFactionKey.IsEmpty())
				valid = AdoptLegacyEnemyPool(
					state,
					preset.m_sInvaderFactionKey)
					&& valid;
		}

		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool)
				continue;
			bool enemy = HST_FactionRelationService.IsEnemyFaction(
				preset,
				pool.m_sFactionKey);
			if (!enemy && pool.m_iStrategicContractVersion == CONTRACT_VERSION)
			{
				QuarantineRuntimePool(
					pool,
					"schema67 strategic authority was attached to a non-enemy faction");
				valid = false;
			}
		}

		valid = ValidateRuntimeEnemyPool(
			state,
			preset.m_sOccupierFactionKey)
			&& valid;
		if (!preset.m_sInvaderFactionKey.IsEmpty())
			valid = ValidateRuntimeEnemyPool(
				state,
				preset.m_sInvaderFactionKey)
				&& valid;

		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (!mutation || mutation.m_iContractVersion
				== QUARANTINE_CONTRACT_VERSION)
				continue;
			if (!HST_FactionRelationService.IsEnemyFaction(
				preset,
				mutation.m_sFactionKey))
			{
				QuarantineRuntimeMutation(mutation);
				valid = false;
			}
		}
		PurgeRejectedRuntimeReceipts(state);
		return valid;
	}

	static bool ValidateMutationShape(
		HST_EnemyStrategicMutationState mutation,
		out string failure)
	{
		failure = "";
		if (!mutation)
		{
			failure = "strategic mutation row is null";
			return false;
		}
		if (mutation.m_iContractVersion != CONTRACT_VERSION
			|| !mutation.m_bApplied)
		{
			failure = "strategic mutation contract or applied flag is invalid";
			return false;
		}
		if (!IsBoundedRequiredId(mutation.m_sMutationId)
			|| !IsBoundedRequiredId(mutation.m_sFactionKey)
			|| !IsBoundedRequiredId(mutation.m_sKind)
			|| !IsBoundedRequiredId(mutation.m_sSourceId)
			|| !IsBoundedOptionalId(mutation.m_sOrderId)
			|| !IsBoundedOptionalId(mutation.m_sOperationId)
			|| !IsBoundedOptionalId(mutation.m_sManifestId)
			|| !IsBoundedOptionalId(mutation.m_sZoneId))
		{
			failure = "strategic mutation identity or link is invalid";
			return false;
		}
		if (mutation.m_sContributionHash != mutation.m_sContributionHash.Trim()
			|| mutation.m_sContributionHash.Length() > MAX_CONTRIBUTION_CHARACTERS)
		{
			failure = "strategic mutation contribution hash is invalid";
			return false;
		}
		if (HST_EnemyStrategicResourceService.IsPeriodicKind(mutation.m_sKind)
			&& mutation.m_sContributionHash.IsEmpty())
		{
			failure = "periodic strategic mutation contribution hash is missing";
			return false;
		}
		if (mutation.m_sKind
			== HST_EnemyStrategicResourceService.KIND_PERIODIC_INCOME)
		{
			if (mutation.m_iAttackDelta < 0 || mutation.m_iSupportDelta < 0
				|| (mutation.m_iAttackDelta == 0
					&& mutation.m_iSupportDelta == 0)
				|| mutation.m_iAggressionDelta != 0
				|| !mutation.m_sOrderId.IsEmpty()
				|| !mutation.m_sOperationId.IsEmpty()
				|| !mutation.m_sManifestId.IsEmpty()
				|| !mutation.m_sZoneId.IsEmpty())
			{
				failure = "periodic income strategic mutation shape is invalid";
				return false;
			}
		}
		else if (mutation.m_sKind
			== HST_EnemyStrategicResourceService.KIND_AGGRESSION_DECAY)
		{
			if (mutation.m_iAttackDelta != 0 || mutation.m_iSupportDelta != 0
				|| mutation.m_iAggressionDelta >= 0
				|| !mutation.m_sOrderId.IsEmpty()
				|| !mutation.m_sOperationId.IsEmpty()
				|| !mutation.m_sManifestId.IsEmpty()
				|| !mutation.m_sZoneId.IsEmpty())
			{
				failure = "aggression-decay strategic mutation shape is invalid";
				return false;
			}
		}
		if ((mutation.m_sKind == "defense_support_debit"
			|| mutation.m_sKind == "defense_support_refund"
			|| mutation.m_sKind == "town_influence"
			|| mutation.m_sKind == "ownership_transition")
			&& mutation.m_sZoneId.IsEmpty())
		{
			failure = "linked strategic mutation target zone is missing";
			return false;
		}
		bool periodic = HST_EnemyStrategicResourceService.IsPeriodicKind(
			mutation.m_sKind);
		if ((periodic && mutation.m_iOperationalSequence != 0)
			|| (!periodic && (mutation.m_iOperationalSequence <= 0
				|| mutation.m_iOperationalSequence > MAX_OPERATIONAL_MUTATIONS)))
		{
			failure = "strategic mutation operational sequence is invalid";
			return false;
		}
		if (mutation.m_iCreatedAtSecond < 0
			|| mutation.m_iPoolRevisionBefore <= 0
			|| mutation.m_iPoolRevisionBefore >= MAX_MUTABLE_REVISION
			|| mutation.m_iPoolRevisionAfter
				!= mutation.m_iPoolRevisionBefore + 1)
		{
			failure = "strategic mutation time or revision is invalid";
			return false;
		}
		if (!IsBoundedDelta(mutation.m_iAttackDelta)
			|| !IsBoundedDelta(mutation.m_iSupportDelta)
			|| !IsBoundedDelta(mutation.m_iAggressionDelta)
			|| (periodic && mutation.m_iAttackDelta == 0
				&& mutation.m_iSupportDelta == 0
				&& mutation.m_iAggressionDelta == 0))
		{
			failure = "strategic mutation delta is invalid";
			return false;
		}
		if (!ArithmeticExact(
			mutation.m_iAttackBefore,
			mutation.m_iAttackDelta,
			mutation.m_iAttackAfter)
			|| !ArithmeticExact(
				mutation.m_iSupportBefore,
				mutation.m_iSupportDelta,
				mutation.m_iSupportAfter)
			|| !ArithmeticExact(
				mutation.m_iAggressionBefore,
				mutation.m_iAggressionDelta,
				mutation.m_iAggressionAfter))
		{
			failure = "strategic mutation arithmetic is invalid";
			return false;
		}
		if (mutation.m_sFingerprint.IsEmpty()
			|| mutation.m_sFingerprint
				!= HST_EnemyStrategicResourceService.BuildMutationFingerprint(
					mutation))
		{
			failure = "strategic mutation structural fingerprint is invalid";
			return false;
		}
		return true;
	}

	protected void EnsureArrays()
	{
		if (!m_SaveData.m_aFactionPools)
			m_SaveData.m_aFactionPools = new array<ref HST_FactionPoolState>();
		if (!m_SaveData.m_aEnemyStrategicMutations)
			m_SaveData.m_aEnemyStrategicMutations
				= new array<ref HST_EnemyStrategicMutationState>();
		if (!m_SaveData.m_aEnemyOrders)
			m_SaveData.m_aEnemyOrders = new array<ref HST_EnemyOrderState>();
		if (!m_SaveData.m_aEnemySupportLedgers)
			m_SaveData.m_aEnemySupportLedgers
				= new array<ref HST_EnemySupportLedgerState>();
		if (!m_SaveData.m_aTownInfluenceEvents)
			m_SaveData.m_aTownInfluenceEvents
				= new array<ref HST_TownInfluenceEventState>();
		if (!m_SaveData.m_aOwnershipTransitions)
			m_SaveData.m_aOwnershipTransitions
				= new array<ref HST_OwnershipTransitionState>();
	}

	protected void RejectUnexpectedPre67Receipts()
	{
		if (!m_SaveData || m_SaveData.m_aEnemyStrategicMutations.IsEmpty())
			return;
		foreach (HST_EnemyStrategicMutationState mutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (!mutation)
				continue;
			QuarantineSavedPool(
				FindUniqueSavedPool(mutation.m_sFactionKey),
				"pre-schema67 save carried unsupported strategic receipts");
			QuarantineSavedMutation(mutation);
		}
	}

	protected void PrepareLegacyPoolSnapshots()
	{
		if (!m_SaveData)
			return;
		foreach (HST_FactionPoolState pool : m_SaveData.m_aFactionPools)
		{
			if (!pool)
				continue;
			pool.m_iStrategicContractVersion = 0;
			pool.m_iStrategicRevision = 0;
			pool.m_iStrategicOperationalMutationCount = 0;
			pool.m_iResourceAccumulatorSeconds = 0;
			pool.m_iAggressionAccumulatorSeconds = 0;
			pool.m_iLastResourceBucketSecond = 0;
			pool.m_iLastAggressionBucketSecond = 0;
			pool.m_sLastStrategicMutationId = "";
			pool.m_sStrategicAuthorityFailure = "";
		}
	}

	protected void ValidatePoolIdentities()
	{
		foreach (HST_FactionPoolState pool : m_SaveData.m_aFactionPools)
		{
			if (!pool)
				continue;
			string key = pool.m_sFactionKey.Trim();
			if (key.IsEmpty() || key.Length() > MAX_ID_CHARACTERS
				|| key != pool.m_sFactionKey)
			{
				QuarantineSavedPool(pool, "schema67 faction pool identity is invalid");
				continue;
			}
			if (CountSavedPools(key) != 1)
			{
				QuarantineSavedPoolsByKey(
					key,
					"schema67 faction pool identity is duplicated");
				continue;
			}
			if (pool.m_iStrategicContractVersion == QUARANTINE_CONTRACT_VERSION)
				continue;
			if (pool.m_iStrategicContractVersion == 0)
				continue;
			string failure = ValidatePoolShape(pool);
			if (!failure.IsEmpty())
				QuarantineSavedPool(pool, failure);
		}
	}

	protected void ValidateReceiptIdentitiesAndShapes()
	{
		foreach (HST_EnemyStrategicMutationState mutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (!mutation || mutation.m_iContractVersion
				== QUARANTINE_CONTRACT_VERSION)
				continue;
			string failure;
			if (CountSavedMutations(mutation.m_sMutationId) != 1)
				failure = "schema67 strategic mutation identity is duplicated";
			else if (HST_EnemyStrategicResourceService.IsPeriodicKind(
					mutation.m_sKind)
				&& CountPeriodicSavedMutations(
					mutation.m_sFactionKey,
					mutation.m_sKind) != 1)
				failure = "schema67 periodic cadence evidence is duplicated";
			else if (!ValidateMutationShape(mutation, failure))
			{
			}
			else
			{
				HST_FactionPoolState pool = FindUniqueSavedPool(
					mutation.m_sFactionKey);
				if (!pool || pool.m_iStrategicContractVersion != CONTRACT_VERSION)
					failure = "schema67 strategic mutation pool authority is unavailable";
				else if (mutation.m_iCreatedAtSecond > m_SaveData.m_iElapsedSeconds)
					failure = "schema67 strategic mutation was created in the future";
				else if (mutation.m_iPoolRevisionAfter > pool.m_iStrategicRevision)
					failure = "schema67 strategic mutation revision exceeds its pool";
				else
					failure = ValidateMutationBacklinks(mutation);
			}
			if (failure.IsEmpty())
				continue;
			QuarantineSavedPoolsForMutation(mutation, failure);
			QuarantineSavedMutation(mutation);
		}
	}

	protected void QuarantineSavedPoolsForMutation(
		HST_EnemyStrategicMutationState mutation,
		string failure)
	{
		if (!m_SaveData || !mutation)
			return;

		// Attribute by the serialized key first. A malformed key with only
		// boundary whitespace can still name an existing pool, so fail that
		// canonical pool closed as well instead of treating the row as harmless.
		string factionKey = mutation.m_sFactionKey;
		if (!factionKey.IsEmpty())
			QuarantineSavedPoolsByKey(factionKey, failure);
		string canonicalFactionKey = factionKey.Trim();
		if (!canonicalFactionKey.IsEmpty()
			&& canonicalFactionKey != factionKey)
			QuarantineSavedPoolsByKey(canonicalFactionKey, failure);
	}

	protected string ValidateMutationBacklinks(
		HST_EnemyStrategicMutationState mutation)
	{
		if (!mutation)
			return "schema67 strategic mutation backlink row is missing";
		if (!mutation.m_sOrderId.IsEmpty())
		{
			string orderFailure = ValidateOrderBacklink(mutation);
			if (!orderFailure.IsEmpty())
				return orderFailure;
		}
		if ((mutation.m_sKind == "defense_support_debit"
			|| mutation.m_sKind == "defense_support_refund")
			&& !mutation.m_sZoneId.IsEmpty()
			&& CountSavedSupportLedgers(
				mutation.m_sFactionKey,
				mutation.m_sZoneId) != 1)
			return "schema67 defense resource mutation support ledger is missing or duplicated";
		if (mutation.m_sKind == "town_influence")
			return ValidateTownInfluenceBacklink(mutation);
		if (mutation.m_sKind == "ownership_transition")
			return ValidateOwnershipTransitionBacklink(mutation);
		return "";
	}

	protected string ValidateOrderBacklink(
		HST_EnemyStrategicMutationState mutation)
	{
		HST_EnemyOrderState order;
		int orderCount;
		foreach (HST_EnemyOrderState candidate : m_SaveData.m_aEnemyOrders)
		{
			if (!candidate || candidate.m_sOrderId != mutation.m_sOrderId)
				continue;
			order = candidate;
			orderCount++;
		}
		if (orderCount != 1 || !order)
			return "schema67 strategic mutation enemy-order backlink is missing or duplicated";
		string mutationZoneId
			= HST_MaidensBayLocationSaveValidationService.ResolveCanonicalZoneId(
				mutation.m_sZoneId);
		string orderZoneId
			= HST_MaidensBayLocationSaveValidationService.ResolveCanonicalZoneId(
				order.m_sTargetZoneId);
		if (order.m_sFactionKey != mutation.m_sFactionKey
			|| order.m_sOperationId != mutation.m_sOperationId
			|| order.m_sManifestId != mutation.m_sManifestId
			|| orderZoneId != mutationZoneId)
			return "schema67 strategic mutation enemy-order links diverged";
		if (mutation.m_sKind == "proactive_attack_debit"
			|| mutation.m_sKind == "defense_support_debit")
		{
			if (order.m_iAttackCost < 0 || order.m_iSupportCost < 0
				|| order.m_sResourceDebitMutationId != mutation.m_sMutationId
				|| mutation.m_iAttackDelta != -order.m_iAttackCost
				|| mutation.m_iSupportDelta != -order.m_iSupportCost
				|| mutation.m_iAggressionDelta != 0
				|| mutation.m_sSourceId != order.m_sOrderId)
				return "schema67 strategic debit enemy-order backlink diverged";
		}
		else if (mutation.m_sKind == "proactive_attack_refund"
			|| mutation.m_sKind == "defense_support_refund")
		{
			if (m_iRestoredSchemaVersion >= 70
				&& HST_EnemyQRFSaveValidationService.HasPreparedSettlementIntent(
				m_SaveData,
				order))
			{
				// A Schema-70 exact defensive QRF may persist after staging its
				// deterministic terminal tuple and before or after the matching
				// refund mutation. Accept only the complete PREPARED aggregate;
				// ordinary partial QRF receipts remain on the fail-closed path.
				string qrfPendingFailure
					= HST_EnemyQRFSaveValidationService.ValidatePreparedSaveAuthority(
						m_SaveData,
						order);
				if (qrfPendingFailure.IsEmpty())
					return "";
				return "schema67 exact enemy defensive QRF pending refund authority conflicts: "
					+ qrfPendingFailure;
			}
			bool exactCounterattackPending = order.m_eType
				== HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK;
			exactCounterattackPending = exactCounterattackPending
				&& order.m_iOperationContractVersion
					== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION;
			exactCounterattackPending = exactCounterattackPending
				&& !order.m_bResourceSettlementApplied
				&& !order.m_sResourceSettlementId.IsEmpty()
				&& !order.m_sResourceSettlementKind.IsEmpty()
				&& order.m_sResourceSettlementId
					== HST_OperationService.BuildSettlementId(
						order.m_sOperationId,
						order.m_sResourceSettlementKind);
			exactCounterattackPending = exactCounterattackPending
				&& order.m_sResourceRefundMutationId == mutation.m_sMutationId;
			if (exactCounterattackPending)
			{
				// Schema 69 permits exactly one refund linked to a durable PREPARED
				// terminal intent (or an uncommitted full rollback). Delegate the
				// graph check; ordinary Schema-67 refunds remain unchanged.
				string pendingFailure
					= HST_EnemyCounterattackSaveValidationService.ValidatePendingResourceRefundAuthority(
						m_SaveData.m_aEnemyStrategicMutations,
						order);
				if (pendingFailure.IsEmpty())
				{
					HST_OperationRecordState pendingOperation;
					HST_ForceManifestState pendingManifest;
					HST_ForceSpawnResultState pendingBatch;
					HST_ActiveGroupState pendingGroup;
					int operationCount;
					int manifestCount;
					int batchCount;
					int groupCount;
					string expectedResultId;
					if (!order.m_sOrderId.IsEmpty())
						expectedResultId = "spawn_" + order.m_sOrderId;
					string expectedProjectionId;
					string expectedForceId;
					if (!order.m_sOperationId.IsEmpty())
					{
						expectedProjectionId = "projection_" + order.m_sOperationId;
						expectedForceId = "force_" + order.m_sOperationId;
					}
					foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
					{
						if (!operation)
							continue;
						bool claimant = operation.m_sOperationId == order.m_sOperationId
							|| operation.m_sEnemyOrderId == order.m_sOrderId
							|| operation.m_sManifestId == order.m_sManifestId;
						if (!claimant && !order.m_sSpawnResultId.IsEmpty())
							claimant = operation.m_sSpawnResultId == order.m_sSpawnResultId;
						if (!claimant && !order.m_sGroupId.IsEmpty())
							claimant = operation.m_sGroupId == order.m_sGroupId;
						if (!claimant && !expectedResultId.IsEmpty())
							claimant = operation.m_sSpawnResultId == expectedResultId;
						if (!claimant && !expectedProjectionId.IsEmpty())
							claimant = operation.m_sGroupId == expectedProjectionId
								|| operation.m_sProjectionId == expectedProjectionId;
						if (!claimant && !expectedForceId.IsEmpty())
							claimant = operation.m_sForceId == expectedForceId;
						if (!claimant)
							continue;
						pendingOperation = operation;
						operationCount++;
					}
					foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
					{
						if (!manifest || (manifest.m_sManifestId != order.m_sManifestId
							&& manifest.m_sOperationId != order.m_sOperationId))
							continue;
						pendingManifest = manifest;
						manifestCount++;
					}
					foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
					{
						if (!batch)
							continue;
						bool claimant = batch.m_sRequestId == order.m_sOrderId
							|| batch.m_sOperationId == order.m_sOperationId
							|| batch.m_sManifestId == order.m_sManifestId;
						if (!claimant && !expectedResultId.IsEmpty())
							claimant = batch.m_sResultId == expectedResultId;
						if (!claimant && !expectedProjectionId.IsEmpty())
							claimant = batch.m_sProjectionId == expectedProjectionId;
						if (!claimant && !expectedForceId.IsEmpty())
							claimant = batch.m_sForceId == expectedForceId;
						if (!claimant)
							continue;
						pendingBatch = batch;
						batchCount++;
					}
					foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
					{
						if (!group)
							continue;
						bool claimant = group.m_sEnemyOrderId == order.m_sOrderId
							|| group.m_sOperationId == order.m_sOperationId
							|| group.m_sManifestId == order.m_sManifestId;
						if (!claimant && !expectedProjectionId.IsEmpty())
							claimant = group.m_sGroupId == expectedProjectionId
								|| group.m_sProjectionId == expectedProjectionId;
						if (!claimant && !expectedResultId.IsEmpty())
							claimant = group.m_sSpawnResultId == expectedResultId;
						if (!claimant && !expectedForceId.IsEmpty())
							claimant = group.m_sForceId == expectedForceId;
						if (!claimant)
							continue;
						pendingGroup = group;
						groupCount++;
					}
					bool fullRefund
						= HST_EnemyCounterattackSaveValidationService.IsFullRefundSettlementKind(
							order.m_sResourceSettlementKind);
					bool aggregateCountsExact = operationCount <= 1
						&& manifestCount <= 1 && batchCount <= 1 && groupCount <= 1;
					if (!fullRefund)
						aggregateCountsExact = operationCount == 1
							&& manifestCount == 1 && batchCount == 1 && groupCount == 1;
					if (!aggregateCountsExact)
						return "schema67 exact counterattack pending refund aggregate is incomplete or ambiguous";
					pendingFailure
						= HST_EnemyCounterattackSaveValidationService.ValidatePendingResourceRefundAggregateAuthority(
							m_SaveData.m_aEnemyStrategicMutations,
							order,
							pendingOperation,
							pendingManifest,
							pendingBatch,
							pendingGroup);
					if (pendingFailure.IsEmpty())
						return "";
				}
				return "schema67 exact counterattack pending refund authority conflicts: "
					+ pendingFailure;
			}
			bool exactGarrisonRebuildPending = order.m_eType
				== HST_EEnemyOrderType.HST_ENEMY_ORDER_REBUILD_GARRISON;
			exactGarrisonRebuildPending = exactGarrisonRebuildPending
				&& order.m_iOperationContractVersion
					== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_CONTRACT_VERSION;
			exactGarrisonRebuildPending = exactGarrisonRebuildPending
				&& !order.m_bResourceSettlementApplied
				&& !order.m_sResourceSettlementId.IsEmpty()
				&& !order.m_sResourceSettlementKind.IsEmpty()
				&& order.m_sResourceSettlementId
					== HST_OperationService.BuildSettlementId(
						order.m_sOperationId,
						order.m_sResourceSettlementKind);
			exactGarrisonRebuildPending = exactGarrisonRebuildPending
				&& order.m_sResourceRefundMutationId == mutation.m_sMutationId;
			if (exactGarrisonRebuildPending)
			{
				// Schema 70 permits exactly one refund linked to a durable PREPARED
				// terminal intent or the staged zero-refund delivery receipt. Delegate
				// only that exact graph; historical rebuild rows remain on Schema 67.
				string pendingFailure
					= HST_EnemyGarrisonRebuildSaveValidationService.ValidatePendingResourceRefundAuthority(
						m_SaveData.m_aEnemyStrategicMutations,
						order);
				if (pendingFailure.IsEmpty())
				{
					HST_OperationRecordState pendingOperation;
					HST_ForceManifestState pendingManifest;
					HST_ForceSpawnResultState pendingBatch;
					HST_ActiveGroupState pendingGroup;
					int operationCount;
					int manifestCount;
					int batchCount;
					int groupCount;
					string expectedResultId;
					if (!order.m_sOrderId.IsEmpty())
						expectedResultId = "spawn_" + order.m_sOrderId;
					string expectedProjectionId;
					string expectedForceId;
					if (!order.m_sOperationId.IsEmpty())
					{
						expectedProjectionId = "projection_" + order.m_sOperationId;
						expectedForceId = "force_" + order.m_sOperationId;
					}
					foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
					{
						if (!operation)
							continue;
						bool claimant = operation.m_sOperationId == order.m_sOperationId
							|| operation.m_sEnemyOrderId == order.m_sOrderId
							|| operation.m_sManifestId == order.m_sManifestId;
						if (!claimant && !order.m_sSpawnResultId.IsEmpty())
							claimant = operation.m_sSpawnResultId == order.m_sSpawnResultId;
						if (!claimant && !order.m_sGroupId.IsEmpty())
							claimant = operation.m_sGroupId == order.m_sGroupId;
						if (!claimant && !expectedResultId.IsEmpty())
							claimant = operation.m_sSpawnResultId == expectedResultId;
						if (!claimant && !expectedProjectionId.IsEmpty())
							claimant = operation.m_sGroupId == expectedProjectionId
								|| operation.m_sProjectionId == expectedProjectionId;
						if (!claimant && !expectedForceId.IsEmpty())
							claimant = operation.m_sForceId == expectedForceId;
						if (!claimant)
							continue;
						pendingOperation = operation;
						operationCount++;
					}
					foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
					{
						if (!manifest || (manifest.m_sManifestId != order.m_sManifestId
							&& manifest.m_sOperationId != order.m_sOperationId))
							continue;
						pendingManifest = manifest;
						manifestCount++;
					}
					foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
					{
						if (!batch)
							continue;
						bool claimant = batch.m_sRequestId == order.m_sOrderId
							|| batch.m_sOperationId == order.m_sOperationId
							|| batch.m_sManifestId == order.m_sManifestId;
						if (!claimant && !expectedResultId.IsEmpty())
							claimant = batch.m_sResultId == expectedResultId;
						if (!claimant && !expectedProjectionId.IsEmpty())
							claimant = batch.m_sProjectionId == expectedProjectionId;
						if (!claimant && !expectedForceId.IsEmpty())
							claimant = batch.m_sForceId == expectedForceId;
						if (!claimant)
							continue;
						pendingBatch = batch;
						batchCount++;
					}
					foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
					{
						if (!group)
							continue;
						bool claimant = group.m_sEnemyOrderId == order.m_sOrderId
							|| group.m_sOperationId == order.m_sOperationId
							|| group.m_sManifestId == order.m_sManifestId;
						if (!claimant && !expectedProjectionId.IsEmpty())
							claimant = group.m_sGroupId == expectedProjectionId
								|| group.m_sProjectionId == expectedProjectionId;
						if (!claimant && !expectedResultId.IsEmpty())
							claimant = group.m_sSpawnResultId == expectedResultId;
						if (!claimant && !expectedForceId.IsEmpty())
							claimant = group.m_sForceId == expectedForceId;
						if (!claimant)
							continue;
						pendingGroup = group;
						groupCount++;
					}
					bool fullRefund
						= HST_EnemyGarrisonRebuildSaveValidationService.IsFullRefundSettlementKind(
							order.m_sResourceSettlementKind);
					bool graphlessFullRefund = fullRefund
						&& !order.m_bStrategicServiceCommitted;
					bool aggregateCountsExact = operationCount <= 1
						&& manifestCount <= 1 && batchCount <= 1 && groupCount <= 1;
					if (!graphlessFullRefund)
						aggregateCountsExact = operationCount == 1
							&& manifestCount == 1 && batchCount == 1 && groupCount == 1;
					if (!aggregateCountsExact)
						return "schema67 exact enemy garrison rebuild pending refund aggregate is incomplete or ambiguous";
					pendingFailure
						= HST_EnemyGarrisonRebuildSaveValidationService.ValidatePendingResourceRefundAggregateAuthority(
							m_SaveData.m_aEnemyStrategicMutations,
							order,
							pendingOperation,
							pendingManifest,
							pendingBatch,
							pendingGroup);
					if (pendingFailure.IsEmpty())
						return "";
				}
				return "schema67 exact enemy garrison rebuild pending refund authority conflicts: "
					+ pendingFailure;
			}
			if (order.m_iRefundedAttackResources < 0
				|| order.m_iRefundedSupportResources < 0
				|| order.m_sResourceRefundMutationId != mutation.m_sMutationId
				|| mutation.m_iAttackDelta != order.m_iRefundedAttackResources
				|| mutation.m_iSupportDelta != order.m_iRefundedSupportResources
				|| mutation.m_iAggressionDelta != 0)
				return "schema67 strategic refund enemy-order backlink diverged";
			if (!order.m_sResourceSettlementId.IsEmpty()
				&& mutation.m_sSourceId != order.m_sResourceSettlementId)
				return "schema67 strategic refund settlement backlink diverged";
			if (order.m_sResourceSettlementId.IsEmpty()
				&& mutation.m_sSourceId != order.m_sOrderId)
				return "schema67 strategic refund source backlink diverged";
		}
		return "";
	}

	protected int CountSavedSupportLedgers(
		string factionKey,
		string zoneId)
	{
		int count;
		string canonicalZoneId
			= HST_MaidensBayLocationSaveValidationService.ResolveCanonicalZoneId(zoneId);
		foreach (HST_EnemySupportLedgerState ledger : m_SaveData.m_aEnemySupportLedgers)
		{
			if (!ledger || ledger.m_sFactionKey != factionKey)
				continue;
			if (HST_MaidensBayLocationSaveValidationService.ResolveCanonicalZoneId(
				ledger.m_sZoneId) == canonicalZoneId)
				count++;
		}
		return count;
	}

	protected string ValidateTownInfluenceBacklink(
		HST_EnemyStrategicMutationState mutation)
	{
		HST_TownInfluenceEventState eventState;
		int count;
		foreach (HST_TownInfluenceEventState candidate : m_SaveData.m_aTownInfluenceEvents)
		{
			if (!candidate || candidate.m_sEventId != mutation.m_sSourceId)
				continue;
			eventState = candidate;
			count++;
		}
		if (count != 1 || !eventState
			|| eventState.m_iContractVersion
				!= HST_TownInfluenceService.EXACT_CONTRACT_VERSION
			|| !eventState.m_bApplied)
			return "schema67 town-influence strategic backlink is missing or duplicated";
		if (eventState.m_sAggressionFactionKey != mutation.m_sFactionKey
			|| eventState.m_iAggressionDelta != mutation.m_iAggressionDelta
			|| eventState.m_iAggressionBefore != mutation.m_iAggressionBefore
			|| eventState.m_iAggressionAfter != mutation.m_iAggressionAfter
			|| HST_MaidensBayLocationSaveValidationService.ResolveCanonicalZoneId(
				eventState.m_sZoneId)
				!= HST_MaidensBayLocationSaveValidationService.ResolveCanonicalZoneId(
					mutation.m_sZoneId))
			return "schema67 town-influence strategic backlink diverged";
		return "";
	}

	protected string ValidateOwnershipTransitionBacklink(
		HST_EnemyStrategicMutationState mutation)
	{
		HST_OwnershipTransitionState transition;
		int count;
		foreach (HST_OwnershipTransitionState candidate : m_SaveData.m_aOwnershipTransitions)
		{
			if (!candidate || candidate.m_sRequestId != mutation.m_sSourceId)
				continue;
			transition = candidate;
			count++;
		}
		bool pendingPreOwnerAdmission = transition
			&& !transition.m_bEnemyConsequencesApplied
			&& !transition.m_bOwnerApplied && !transition.m_bCompleted
			&& transition.m_sStatus == "applying"
			&& transition.m_iAggressionApplied > 0;
		if (count != 1 || !transition
			|| transition.m_iContractVersion
				!= HST_OwnershipTransitionService.EXACT_CONTRACT_VERSION
			|| transition.m_bQuarantined
			|| (!transition.m_bEnemyConsequencesApplied
				&& !pendingPreOwnerAdmission))
			return "schema67 ownership-transition strategic backlink is missing or duplicated";
		if (transition.m_sPreviousOwnerFactionKey != mutation.m_sFactionKey
			|| transition.m_iAggressionApplied != mutation.m_iAggressionDelta
			|| HST_MaidensBayLocationSaveValidationService.ResolveCanonicalZoneId(
				transition.m_sZoneId)
				!= HST_MaidensBayLocationSaveValidationService.ResolveCanonicalZoneId(
					mutation.m_sZoneId))
			return "schema67 ownership-transition strategic backlink diverged";
		return "";
	}

	protected void ValidatePoolReceiptChains()
	{
		foreach (HST_FactionPoolState pool : m_SaveData.m_aFactionPools)
		{
			if (!pool || pool.m_iStrategicContractVersion != CONTRACT_VERSION)
				continue;
			string failure = ValidatePoolReceiptChain(pool);
			if (!failure.IsEmpty())
				QuarantineSavedPool(pool, failure);
		}
	}

	protected string ValidatePoolShape(HST_FactionPoolState pool)
	{
		if (!pool)
			return "schema67 faction pool row is missing";
		if (pool.m_iStrategicContractVersion != CONTRACT_VERSION)
			return "schema67 faction pool contract is invalid";
		if (pool.m_iStrategicRevision <= 0
			|| pool.m_iStrategicRevision >= MAX_MUTABLE_REVISION)
			return "schema67 faction pool revision is invalid";
		if (pool.m_iStrategicOperationalMutationCount < 0
			|| pool.m_iStrategicOperationalMutationCount > MAX_OPERATIONAL_MUTATIONS)
			return "schema67 faction pool operational sequence count is invalid";
		if (pool.m_iAttackResources < 0 || pool.m_iSupportResources < 0
			|| pool.m_iAggression < 0)
			return "schema67 faction pool projection is negative";
		if (m_SaveData.m_iElapsedSeconds < 0)
			return "schema67 faction pool campaign clock is invalid";
		bool resourceCadenceExact = pool.m_iLastResourceBucketSecond
			== m_SaveData.m_iElapsedSeconds - pool.m_iResourceAccumulatorSeconds;
		bool aggressionCadenceExact = pool.m_iLastAggressionBucketSecond
			== m_SaveData.m_iElapsedSeconds - pool.m_iAggressionAccumulatorSeconds;
		int latestResourceReceiptSecond = FindLatestSavedPeriodicSecond(
			pool.m_sFactionKey,
			HST_EnemyStrategicResourceService.KIND_PERIODIC_INCOME);
		int latestAggressionReceiptSecond = FindLatestSavedPeriodicSecond(
			pool.m_sFactionKey,
			HST_EnemyStrategicResourceService.KIND_AGGRESSION_DECAY);
		if (pool.m_iResourceAccumulatorSeconds < 0
			|| pool.m_iAggressionAccumulatorSeconds < 0
			|| pool.m_iResourceAccumulatorSeconds > m_SaveData.m_iElapsedSeconds
			|| pool.m_iAggressionAccumulatorSeconds > m_SaveData.m_iElapsedSeconds
			|| pool.m_iLastResourceBucketSecond < 0
			|| pool.m_iLastAggressionBucketSecond < 0
			|| latestResourceReceiptSecond > pool.m_iLastResourceBucketSecond
			|| latestAggressionReceiptSecond > pool.m_iLastAggressionBucketSecond
			|| (!resourceCadenceExact && !IsSavedCadenceUninitialized(
				pool,
				HST_EnemyStrategicResourceService.KIND_PERIODIC_INCOME))
			|| (!aggressionCadenceExact && !IsSavedCadenceUninitialized(
				pool,
				HST_EnemyStrategicResourceService.KIND_AGGRESSION_DECAY)))
			return "schema67 faction pool cadence accumulator is invalid";
		if (pool.m_sLastStrategicMutationId.Length() > MAX_ID_CHARACTERS
			|| pool.m_sLastStrategicMutationId
				!= pool.m_sLastStrategicMutationId.Trim()
			|| !pool.m_sStrategicAuthorityFailure.IsEmpty())
			return "schema67 faction pool identity or failure state is invalid";
		return "";
	}

	protected string ValidatePoolReceiptChain(HST_FactionPoolState pool)
	{
		if (!pool)
			return "schema67 faction pool receipt chain is unavailable";
		int operationalCount;
		array<int> sequenceCounts = {};
		sequenceCounts.Resize(pool.m_iStrategicOperationalMutationCount + 1);
		foreach (HST_EnemyStrategicMutationState sequenceMutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (!sequenceMutation
				|| sequenceMutation.m_iContractVersion != CONTRACT_VERSION
				|| !sequenceMutation.m_bApplied
				|| sequenceMutation.m_sFactionKey != pool.m_sFactionKey
				|| HST_EnemyStrategicResourceService.IsPeriodicKind(
					sequenceMutation.m_sKind))
				continue;
			operationalCount++;
			if (sequenceMutation.m_iOperationalSequence <= 0
				|| sequenceMutation.m_iOperationalSequence
					> pool.m_iStrategicOperationalMutationCount)
				return "schema67 faction operational receipt sequence is outside its pool count";
			int currentSequenceCount
				= sequenceCounts[sequenceMutation.m_iOperationalSequence];
			sequenceCounts[sequenceMutation.m_iOperationalSequence]
				= currentSequenceCount + 1;
		}
		if (operationalCount != pool.m_iStrategicOperationalMutationCount)
			return "schema67 faction operational receipt count diverged";
		for (int sequence = 1; sequence <= pool.m_iStrategicOperationalMutationCount; sequence++)
		{
			if (sequenceCounts[sequence] != 1)
				return "schema67 faction operational receipt sequence is missing or duplicated";
		}
		int latestCount;
		HST_EnemyStrategicMutationState latest;
		foreach (HST_EnemyStrategicMutationState mutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (!mutation || mutation.m_iContractVersion != CONTRACT_VERSION
				|| mutation.m_sFactionKey != pool.m_sFactionKey)
				continue;
			if (CountSavedRevision(
				pool.m_sFactionKey,
				mutation.m_iPoolRevisionAfter) != 1)
				return "schema67 strategic pool receipt revision is duplicated";
			HST_EnemyStrategicMutationState predecessor
				= FindSavedRevision(
					pool.m_sFactionKey,
					mutation.m_iPoolRevisionBefore);
			if (predecessor
				&& (predecessor.m_iAttackAfter != mutation.m_iAttackBefore
					|| predecessor.m_iSupportAfter != mutation.m_iSupportBefore
					|| predecessor.m_iAggressionAfter
						!= mutation.m_iAggressionBefore))
				return "schema67 strategic pool receipt chain arithmetic diverged";
			if (mutation.m_iPoolRevisionAfter == pool.m_iStrategicRevision)
			{
				latest = mutation;
				latestCount++;
			}
		}
		if (pool.m_iStrategicRevision == 1)
		{
			if (latestCount != 0 || !pool.m_sLastStrategicMutationId.IsEmpty())
				return "schema67 baseline pool carries an impossible last receipt";
			return "";
		}
		if (latestCount != 1 || !latest)
			return "schema67 strategic pool latest receipt is missing or ambiguous";
		if (pool.m_sLastStrategicMutationId != latest.m_sMutationId
			|| pool.m_iAttackResources != latest.m_iAttackAfter
			|| pool.m_iSupportResources != latest.m_iSupportAfter
			|| pool.m_iAggression != latest.m_iAggressionAfter)
			return "schema67 strategic pool projection diverged from its latest receipt";
		return "";
	}

	protected bool AdoptLegacyEnemyPool(
		HST_CampaignState state,
		string factionKey)
	{
		HST_FactionPoolState pool;
		if (!FindUniqueRuntimePool(state, factionKey, pool))
		{
			string failure = "schema67 migration requires one configured enemy pool";
			if (CountRuntimePools(state, factionKey) == 0)
				InsertMissingRuntimePoolPlaceholder(state, factionKey, failure);
			else
				QuarantineRuntimePoolsByKey(state, factionKey, failure);
			return false;
		}
		if (pool.m_iAttackResources < 0 || pool.m_iSupportResources < 0
			|| pool.m_iAggression < 0
			|| state.m_iEnemyResourceAccumulatorSeconds < 0
			|| state.m_iAggressionAccumulatorSeconds < 0
			|| state.m_iEnemyResourceAccumulatorSeconds > state.m_iElapsedSeconds
			|| state.m_iAggressionAccumulatorSeconds > state.m_iElapsedSeconds)
		{
			QuarantineRuntimePool(
				pool,
				"schema67 migration baseline snapshot is invalid");
			return false;
		}
		if (pool.m_iStrategicContractVersion == QUARANTINE_CONTRACT_VERSION)
			return false;
		pool.m_iStrategicContractVersion = CONTRACT_VERSION;
		pool.m_iStrategicRevision = 1;
		pool.m_iStrategicOperationalMutationCount = 0;
		pool.m_iResourceAccumulatorSeconds
			= state.m_iEnemyResourceAccumulatorSeconds;
		pool.m_iAggressionAccumulatorSeconds
			= state.m_iAggressionAccumulatorSeconds;
		pool.m_iLastResourceBucketSecond
			= state.m_iElapsedSeconds - pool.m_iResourceAccumulatorSeconds;
		pool.m_iLastAggressionBucketSecond
			= state.m_iElapsedSeconds - pool.m_iAggressionAccumulatorSeconds;
		pool.m_sLastStrategicMutationId = "";
		pool.m_sStrategicAuthorityFailure = "";
		return true;
	}

	protected bool ValidateRuntimeEnemyPool(
		HST_CampaignState state,
		string factionKey)
	{
		HST_FactionPoolState pool;
		if (!FindUniqueRuntimePool(state, factionKey, pool))
		{
			string failure = "schema67 configured enemy pool is missing or duplicated";
			if (CountRuntimePools(state, factionKey) == 0)
				InsertMissingRuntimePoolPlaceholder(state, factionKey, failure);
			else
				QuarantineRuntimePoolsByKey(state, factionKey, failure);
			return false;
		}
		if (state.m_iElapsedSeconds < 0)
		{
			QuarantineRuntimePool(
				pool,
				"schema67 configured enemy pool campaign clock is invalid");
			return false;
		}
		bool resourceCadenceExact = pool.m_iLastResourceBucketSecond
			== state.m_iElapsedSeconds - pool.m_iResourceAccumulatorSeconds;
		bool aggressionCadenceExact = pool.m_iLastAggressionBucketSecond
			== state.m_iElapsedSeconds - pool.m_iAggressionAccumulatorSeconds;
		if (pool.m_iStrategicContractVersion != CONTRACT_VERSION
			|| pool.m_iStrategicRevision <= 0
			|| pool.m_iStrategicRevision >= MAX_MUTABLE_REVISION
			|| pool.m_iStrategicOperationalMutationCount < 0
			|| pool.m_iStrategicOperationalMutationCount > MAX_OPERATIONAL_MUTATIONS
			|| pool.m_iAttackResources < 0 || pool.m_iSupportResources < 0
			|| pool.m_iAggression < 0
			|| pool.m_iResourceAccumulatorSeconds < 0
			|| pool.m_iAggressionAccumulatorSeconds < 0
			|| pool.m_iResourceAccumulatorSeconds > state.m_iElapsedSeconds
			|| pool.m_iAggressionAccumulatorSeconds > state.m_iElapsedSeconds
			|| (!resourceCadenceExact && !IsRuntimeCadenceUninitialized(
				state,
				pool,
				HST_EnemyStrategicResourceService.KIND_PERIODIC_INCOME))
			|| (!aggressionCadenceExact && !IsRuntimeCadenceUninitialized(
				state,
				pool,
				HST_EnemyStrategicResourceService.KIND_AGGRESSION_DECAY))
			|| !pool.m_sStrategicAuthorityFailure.IsEmpty())
		{
			QuarantineRuntimePool(
				pool,
				"schema67 configured enemy pool failed live-role validation");
			return false;
		}
		return true;
	}

	protected bool IsSavedCadenceUninitialized(
		HST_FactionPoolState pool,
		string kind)
	{
		if (!pool)
			return false;
		if (kind == HST_EnemyStrategicResourceService.KIND_PERIODIC_INCOME)
		{
			if (pool.m_iResourceAccumulatorSeconds != 0
				|| pool.m_iLastResourceBucketSecond != 0)
				return false;
		}
		else if (pool.m_iAggressionAccumulatorSeconds != 0
			|| pool.m_iLastAggressionBucketSecond != 0)
			return false;
		return CountPeriodicSavedMutations(pool.m_sFactionKey, kind) == 0;
	}

	protected bool IsRuntimeCadenceUninitialized(
		HST_CampaignState state,
		HST_FactionPoolState pool,
		string kind)
	{
		if (!state || !pool)
			return false;
		if (kind == HST_EnemyStrategicResourceService.KIND_PERIODIC_INCOME)
		{
			if (pool.m_iResourceAccumulatorSeconds != 0
				|| pool.m_iLastResourceBucketSecond != 0)
				return false;
		}
		else if (pool.m_iAggressionAccumulatorSeconds != 0
			|| pool.m_iLastAggressionBucketSecond != 0)
			return false;
		foreach (HST_EnemyStrategicMutationState mutation : state.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_iContractVersion == CONTRACT_VERSION
				&& mutation.m_sFactionKey == pool.m_sFactionKey
				&& mutation.m_sKind == kind)
				return false;
		}
		return true;
	}

	protected HST_FactionPoolState FindUniqueSavedPool(string factionKey)
	{
		HST_FactionPoolState match;
		if (!m_SaveData || factionKey.IsEmpty())
			return null;
		foreach (HST_FactionPoolState pool : m_SaveData.m_aFactionPools)
		{
			if (!pool || pool.m_sFactionKey != factionKey)
				continue;
			if (match)
				return null;
			match = pool;
		}
		return match;
	}

	protected bool FindUniqueRuntimePool(
		HST_CampaignState state,
		string factionKey,
		out HST_FactionPoolState match)
	{
		match = null;
		if (!state || factionKey.IsEmpty())
			return false;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || pool.m_sFactionKey != factionKey)
				continue;
			if (match)
				return false;
			match = pool;
		}
		return match != null;
	}

	protected int CountRuntimePools(HST_CampaignState state, string factionKey)
	{
		int count;
		if (!state || factionKey.IsEmpty())
			return count;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (pool && pool.m_sFactionKey == factionKey)
				count++;
		}
		return count;
	}

	protected void InsertMissingRuntimePoolPlaceholder(
		HST_CampaignState state,
		string factionKey,
		string failure)
	{
		if (!state || factionKey.IsEmpty()
			|| CountRuntimePools(state, factionKey) != 0)
			return;
		HST_FactionPoolState placeholder = new HST_FactionPoolState();
		placeholder.m_sFactionKey = factionKey;
		placeholder.m_iStrategicContractVersion = QUARANTINE_CONTRACT_VERSION;
		placeholder.m_sStrategicAuthorityFailure = LimitText(
			failure.Trim(),
			MAX_FAILURE_CHARACTERS);
		state.m_aFactionPools.Insert(placeholder);
	}

	protected int CountSavedPools(string factionKey)
	{
		int count;
		if (!m_SaveData || factionKey.IsEmpty())
			return count;
		foreach (HST_FactionPoolState pool : m_SaveData.m_aFactionPools)
		{
			if (pool && pool.m_sFactionKey == factionKey)
				count++;
		}
		return count;
	}

	protected int CountSavedMutations(string mutationId)
	{
		int count;
		if (!m_SaveData || mutationId.IsEmpty())
			return count;
		foreach (HST_EnemyStrategicMutationState mutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_sMutationId == mutationId)
				count++;
		}
		return count;
	}

	protected int CountPeriodicSavedMutations(
		string factionKey,
		string kind)
	{
		int count;
		if (!m_SaveData || factionKey.IsEmpty() || kind.IsEmpty())
			return count;
		foreach (HST_EnemyStrategicMutationState mutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_sFactionKey == factionKey
				&& mutation.m_sKind == kind)
				count++;
		}
		return count;
	}

	protected int FindLatestSavedPeriodicSecond(
		string factionKey,
		string kind)
	{
		int latest = -1;
		foreach (HST_EnemyStrategicMutationState mutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_iContractVersion == CONTRACT_VERSION
				&& mutation.m_bApplied
				&& mutation.m_sFactionKey == factionKey
				&& mutation.m_sKind == kind)
				latest = Math.Max(latest, mutation.m_iCreatedAtSecond);
		}
		return latest;
	}

	protected int CountSavedRevision(string factionKey, int revisionAfter)
	{
		int count;
		foreach (HST_EnemyStrategicMutationState mutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_iContractVersion == CONTRACT_VERSION
				&& mutation.m_sFactionKey == factionKey
				&& mutation.m_iPoolRevisionAfter == revisionAfter)
				count++;
		}
		return count;
	}

	protected HST_EnemyStrategicMutationState FindSavedRevision(
		string factionKey,
		int revisionAfter)
	{
		HST_EnemyStrategicMutationState match;
		foreach (HST_EnemyStrategicMutationState mutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (!mutation || mutation.m_iContractVersion != CONTRACT_VERSION
				|| mutation.m_sFactionKey != factionKey
				|| mutation.m_iPoolRevisionAfter != revisionAfter)
				continue;
			if (match)
				return null;
			match = mutation;
		}
		return match;
	}

	protected void QuarantineAllExactPools(string failure)
	{
		if (!m_SaveData)
			return;
		foreach (HST_FactionPoolState pool : m_SaveData.m_aFactionPools)
		{
			if (pool && pool.m_iStrategicContractVersion != 0)
				QuarantineSavedPool(pool, failure);
		}
	}

	protected void QuarantineAllReceipts(string failure)
	{
		if (!m_SaveData)
			return;
		foreach (HST_EnemyStrategicMutationState mutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (mutation)
				QuarantineSavedMutation(mutation);
		}
	}

	protected void PurgeRejectedSavedReceipts()
	{
		if (!m_SaveData || !m_SaveData.m_aEnemyStrategicMutations)
			return;
		for (int mutationIndex = m_SaveData.m_aEnemyStrategicMutations.Count() - 1; mutationIndex >= 0; mutationIndex--)
		{
			HST_EnemyStrategicMutationState mutation
				= m_SaveData.m_aEnemyStrategicMutations[mutationIndex];
			HST_FactionPoolState pool;
			if (mutation && mutation.m_iContractVersion == CONTRACT_VERSION
				&& mutation.m_bApplied)
				pool = FindUniqueSavedPool(mutation.m_sFactionKey);
			if (pool && pool.m_iStrategicContractVersion == CONTRACT_VERSION
				&& pool.m_sStrategicAuthorityFailure.IsEmpty())
				continue;
			m_SaveData.m_aEnemyStrategicMutations.Remove(mutationIndex);
		}
	}

	protected void PurgePreviouslyQuarantinedSavedReceipts()
	{
		if (!m_SaveData || !m_SaveData.m_aEnemyStrategicMutations)
			return;
		for (int mutationIndex = m_SaveData.m_aEnemyStrategicMutations.Count() - 1; mutationIndex >= 0; mutationIndex--)
		{
			HST_EnemyStrategicMutationState mutation
				= m_SaveData.m_aEnemyStrategicMutations[mutationIndex];
			if (!mutation
				|| mutation.m_iContractVersion != QUARANTINE_CONTRACT_VERSION
				|| mutation.m_bApplied)
				continue;
			QuarantineSavedPoolsForMutation(
				mutation,
				"schema67 save retained a previously quarantined strategic receipt");
			m_SaveData.m_aEnemyStrategicMutations.Remove(mutationIndex);
		}
	}

	protected void QuarantineSavedPoolsByKey(
		string factionKey,
		string failure)
	{
		if (!m_SaveData || factionKey.IsEmpty())
			return;
		foreach (HST_FactionPoolState pool : m_SaveData.m_aFactionPools)
		{
			if (pool && pool.m_sFactionKey == factionKey)
				QuarantineSavedPool(pool, failure);
		}
	}

	protected void QuarantineSavedPool(
		HST_FactionPoolState pool,
		string failure)
	{
		if (!pool)
			return;
		pool.m_iStrategicContractVersion = QUARANTINE_CONTRACT_VERSION;
		pool.m_sStrategicAuthorityFailure = LimitText(
			failure.Trim(),
			MAX_FAILURE_CHARACTERS);
	}

	protected void QuarantineSavedMutation(
		HST_EnemyStrategicMutationState mutation)
	{
		if (!mutation)
			return;
		mutation.m_iContractVersion = QUARANTINE_CONTRACT_VERSION;
		mutation.m_bApplied = false;
	}

	protected void QuarantineRuntimeEnemyPools(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string failure)
	{
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (pool && HST_FactionRelationService.IsEnemyFaction(
				preset,
				pool.m_sFactionKey))
				QuarantineRuntimePool(pool, failure);
		}
	}

	protected void QuarantineRuntimePoolsByKey(
		HST_CampaignState state,
		string factionKey,
		string failure)
	{
		if (!state || factionKey.IsEmpty())
			return;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (pool && pool.m_sFactionKey == factionKey)
				QuarantineRuntimePool(pool, failure);
		}
	}

	protected void QuarantineRuntimePool(
		HST_FactionPoolState pool,
		string failure)
	{
		if (!pool)
			return;
		pool.m_iStrategicContractVersion = QUARANTINE_CONTRACT_VERSION;
		pool.m_sStrategicAuthorityFailure = LimitText(
			failure.Trim(),
			MAX_FAILURE_CHARACTERS);
	}

	protected void QuarantineRuntimeMutation(
		HST_EnemyStrategicMutationState mutation)
	{
		if (!mutation)
			return;
		mutation.m_iContractVersion = QUARANTINE_CONTRACT_VERSION;
		mutation.m_bApplied = false;
	}

	protected void PurgeRejectedRuntimeReceipts(HST_CampaignState state)
	{
		if (!state || !state.m_aEnemyStrategicMutations)
			return;
		for (int mutationIndex = state.m_aEnemyStrategicMutations.Count() - 1; mutationIndex >= 0; mutationIndex--)
		{
			HST_EnemyStrategicMutationState mutation
				= state.m_aEnemyStrategicMutations[mutationIndex];
			HST_FactionPoolState pool;
			bool uniquePool;
			if (mutation && mutation.m_iContractVersion == CONTRACT_VERSION
				&& mutation.m_bApplied)
				uniquePool = FindUniqueRuntimePool(
					state,
					mutation.m_sFactionKey,
					pool);
			if (uniquePool && pool
				&& pool.m_iStrategicContractVersion == CONTRACT_VERSION
				&& pool.m_sStrategicAuthorityFailure.IsEmpty())
				continue;
			state.m_aEnemyStrategicMutations.Remove(mutationIndex);
		}
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

	protected static bool ArithmeticExact(int before, int delta, int after)
	{
		if (before < 0 || after < 0 || delta == int.MIN)
			return false;
		if (delta > 0 && before > int.MAX - delta)
			return false;
		if (delta < 0 && before < -delta)
			return false;
		return before + delta == after;
	}

	protected static bool IsBoundedRequiredId(string value)
	{
		return !value.IsEmpty() && value == value.Trim()
			&& value.Length() <= MAX_ID_CHARACTERS;
	}

	protected static bool IsBoundedOptionalId(string value)
	{
		return value == value.Trim() && value.Length() <= MAX_ID_CHARACTERS;
	}

	protected static bool IsBoundedDelta(int value)
	{
		return value != int.MIN
			&& value >= -MAX_DELTA_MAGNITUDE
			&& value <= MAX_DELTA_MAGNITUDE;
	}

	protected string LimitText(string value, int maxCharacters)
	{
		if (value.Length() <= maxCharacters)
			return value;
		return value.Substring(0, maxCharacters);
	}
}
