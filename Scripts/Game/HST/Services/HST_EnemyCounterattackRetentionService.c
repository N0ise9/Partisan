// Preserves terminal spawn evidence claimed by a quarantined Schema-69 enemy
// counterattack. The queue compactor is intentionally aggregate-agnostic, so
// its caller supplies these durable backlink pins before removing old rows.
class HST_EnemyCounterattackRetentionService
{
	static void AddQuarantinedSpawnPins(
		HST_CampaignState state,
		HST_ForceSpawnQueueRetentionPins pins)
	{
		if (!state || !pins)
			return;

		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!IsQuarantinedCounterattack(order))
				continue;

			AddOrderPins(pins, order);
			foreach (HST_OperationRecordState operation : state.m_aOperations)
			{
				if (OperationClaimsOrder(operation, order))
					AddOperationPins(pins, operation);
			}
			foreach (HST_ForceManifestState manifest : state.m_aForceManifests)
			{
				if (ManifestClaimsPinnedAuthority(manifest, pins))
					AddManifestPins(pins, manifest);
			}
			foreach (HST_ForceSpawnResultState batch : state.m_aForceSpawnResults)
			{
				if (BatchClaimsOrderOrPinnedAuthority(batch, order, pins))
					AddBatchPins(pins, batch);
			}
		}
	}

	protected static bool IsQuarantinedCounterattack(HST_EnemyOrderState order)
	{
		return order
			&& order.m_eType == HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK
			&& order.m_iOperationContractVersion
				== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION;
	}

	protected static bool OperationClaimsOrder(
		HST_OperationRecordState operation,
		HST_EnemyOrderState order)
	{
		if (!operation || !order
			|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_ENEMY_COUNTERATTACK)
			return false;
		return (!order.m_sOrderId.IsEmpty()
				&& operation.m_sEnemyOrderId == order.m_sOrderId)
			|| (!order.m_sOperationId.IsEmpty()
				&& operation.m_sOperationId == order.m_sOperationId)
			|| (!order.m_sManifestId.IsEmpty()
				&& operation.m_sManifestId == order.m_sManifestId)
			|| (!order.m_sSpawnResultId.IsEmpty()
				&& operation.m_sSpawnResultId == order.m_sSpawnResultId)
			|| (!order.m_sGroupId.IsEmpty()
				&& operation.m_sGroupId == order.m_sGroupId);
	}

	protected static bool ManifestClaimsPinnedAuthority(
		HST_ForceManifestState manifest,
		HST_ForceSpawnQueueRetentionPins pins)
	{
		if (!manifest || !pins)
			return false;
		return (!manifest.m_sManifestId.IsEmpty()
				&& pins.m_aManifestIds.Contains(manifest.m_sManifestId))
			|| (!manifest.m_sOperationId.IsEmpty()
				&& pins.m_aOperationIds.Contains(manifest.m_sOperationId));
	}

	protected static bool BatchClaimsOrderOrPinnedAuthority(
		HST_ForceSpawnResultState batch,
		HST_EnemyOrderState order,
		HST_ForceSpawnQueueRetentionPins pins)
	{
		if (!batch || !order || !pins)
			return false;
		if (!order.m_sOrderId.IsEmpty()
			&& batch.m_sRequestId == order.m_sOrderId)
			return true;
		return pins.Contains(batch);
	}

	protected static void AddOrderPins(
		HST_ForceSpawnQueueRetentionPins pins,
		HST_EnemyOrderState order)
	{
		AddPin(pins.m_aRequestIds, order.m_sOrderId);
		AddPin(pins.m_aResultIds, order.m_sSpawnResultId);
		AddPin(pins.m_aManifestIds, order.m_sManifestId);
		AddPin(pins.m_aOperationIds, order.m_sOperationId);
	}

	protected static void AddOperationPins(
		HST_ForceSpawnQueueRetentionPins pins,
		HST_OperationRecordState operation)
	{
		AddPin(pins.m_aResultIds, operation.m_sSpawnResultId);
		AddPin(pins.m_aManifestIds, operation.m_sManifestId);
		AddPin(pins.m_aOperationIds, operation.m_sOperationId);
		AddPin(pins.m_aForceIds, operation.m_sForceId);
		AddPin(pins.m_aProjectionIds, operation.m_sProjectionId);
	}

	protected static void AddManifestPins(
		HST_ForceSpawnQueueRetentionPins pins,
		HST_ForceManifestState manifest)
	{
		AddPin(pins.m_aManifestIds, manifest.m_sManifestId);
		AddPin(pins.m_aOperationIds, manifest.m_sOperationId);
	}

	protected static void AddBatchPins(
		HST_ForceSpawnQueueRetentionPins pins,
		HST_ForceSpawnResultState batch)
	{
		AddPin(pins.m_aResultIds, batch.m_sResultId);
		AddPin(pins.m_aRequestIds, batch.m_sRequestId);
		AddPin(pins.m_aManifestIds, batch.m_sManifestId);
		AddPin(pins.m_aOperationIds, batch.m_sOperationId);
		AddPin(pins.m_aForceIds, batch.m_sForceId);
		AddPin(pins.m_aProjectionIds, batch.m_sProjectionId);
	}

	protected static void AddPin(array<string> values, string value)
	{
		if (values && !value.IsEmpty() && !values.Contains(value))
			values.Insert(value);
	}
}
