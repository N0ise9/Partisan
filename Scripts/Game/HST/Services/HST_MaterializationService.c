enum HST_EOperationProjectionDecision
{
	HST_OPERATION_PROJECTION_RETAIN,
	HST_OPERATION_PROJECTION_MATERIALIZE,
	HST_OPERATION_PROJECTION_DEMATERIALIZE
}

class HST_OperationProjectionDecision
{
	HST_EOperationProjectionDecision m_eDecision = HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_RETAIN;
	float m_fMaterializeInDistanceMeters;
	float m_fMaterializeOutDistanceMeters;
	bool m_bPlayerInsideInDistance;
	bool m_bPlayerInsideOutDistance;
	string m_sReason;
}

class HST_MaterializationService
{
	static const float MIN_HYSTERESIS_METERS = 350.0;
	static const float HYSTERESIS_RATIO = 0.2;

	HST_OperationProjectionDecision EvaluateExactPlayerQRF(HST_OperationRecordState operation, vector position)
	{
		float inDistance = Math.Max(100.0, HST_WorldPositionService.GetPlayerEventBubbleRadiusMeters());
		float outDistance = inDistance + Math.Max(MIN_HYSTERESIS_METERS, inDistance * HYSTERESIS_RATIO);
		bool insideIn = HST_WorldPositionService.IsPositionNearLivingPlayer(position, inDistance);
		bool insideOut = HST_WorldPositionService.IsPositionNearLivingPlayer(position, outDistance);
		return EvaluateExactPlayerQRFForProximity(operation, insideIn, insideOut, inDistance, outDistance);
	}

	HST_OperationProjectionDecision EvaluateExactPlayerQRFForProximity(
		HST_OperationRecordState operation,
		bool playerInsideInDistance,
		bool playerInsideOutDistance,
		float inDistance,
		float outDistance)
	{
		HST_OperationProjectionDecision result = new HST_OperationProjectionDecision();
		result.m_fMaterializeInDistanceMeters = inDistance;
		result.m_fMaterializeOutDistanceMeters = outDistance;
		if (!operation || operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			result.m_sReason = "terminal or missing operation retained";
			return result;
		}

		result.m_bPlayerInsideInDistance = playerInsideInDistance;
		result.m_bPlayerInsideOutDistance = playerInsideOutDistance;
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
		{
			if (result.m_bPlayerInsideInDistance)
			{
				result.m_eDecision = HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE;
				result.m_sReason = "living player entered materialize-in distance";
			}
			else
				result.m_sReason = "virtual operation remains outside materialize-in distance";
			return result;
		}

		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
		{
			result.m_sReason = "projection transition already in progress";
			return result;
		}
		if (result.m_bPlayerInsideOutDistance)
		{
			result.m_sReason = "physical operation retained inside materialize-out distance";
			return result;
		}
		if (operation.m_eEngagementMode != HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR)
		{
			result.m_sReason = "physical operation retained during authoritative contact";
			return result;
		}

		result.m_eDecision = HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE;
		result.m_sReason = "all living players left materialize-out distance";
		return result;
	}

	HST_OperationProjectionDecision EvaluateExactEnemyCounterattack(
		HST_OperationRecordState operation,
		vector position)
	{
		float inDistance = Math.Max(100.0, HST_WorldPositionService.GetPlayerEventBubbleRadiusMeters());
		float outDistance = inDistance + Math.Max(MIN_HYSTERESIS_METERS, inDistance * HYSTERESIS_RATIO);
		bool insideIn = HST_WorldPositionService.IsPositionNearLivingPlayer(position, inDistance);
		bool insideOut = HST_WorldPositionService.IsPositionNearLivingPlayer(position, outDistance);
		return EvaluateExactEnemyCounterattackForProximity(
			operation,
			insideIn,
			insideOut,
			inDistance,
			outDistance);
	}

	HST_OperationProjectionDecision EvaluateExactEnemyCounterattackForProximity(
		HST_OperationRecordState operation,
		bool playerInsideInDistance,
		bool playerInsideOutDistance,
		float inDistance,
		float outDistance)
	{
		HST_OperationProjectionDecision result = new HST_OperationProjectionDecision();
		result.m_fMaterializeInDistanceMeters = inDistance;
		result.m_fMaterializeOutDistanceMeters = outDistance;
		if (!operation || operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			result.m_sReason = "terminal or missing counterattack retained";
			return result;
		}

		result.m_bPlayerInsideInDistance = playerInsideInDistance;
		result.m_bPlayerInsideOutDistance = playerInsideOutDistance;
		if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
		{
			if (result.m_bPlayerInsideInDistance)
			{
				result.m_eDecision = HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_MATERIALIZE;
				result.m_sReason = "living player entered counterattack materialize-in distance";
			}
			else
				result.m_sReason = "virtual counterattack remains outside materialize-in distance";
			return result;
		}

		if (operation.m_eMaterializationState != HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_PHYSICAL)
		{
			result.m_sReason = "counterattack projection transition already in progress";
			return result;
		}
		if (result.m_bPlayerInsideOutDistance)
		{
			result.m_sReason = "physical counterattack retained inside materialize-out distance";
			return result;
		}

		// Enemy-on-enemy contact must not pin a remote physical projection after
		// every player leaves. The operation service samples its exact survivor
		// roster before accepting this handoff, then strategic combat resumes from
		// the same durable slots.
		result.m_eDecision = HST_EOperationProjectionDecision.HST_OPERATION_PROJECTION_DEMATERIALIZE;
		result.m_sReason = "all living players left counterattack materialize-out distance; strategic combat resumes";
		return result;
	}
}
