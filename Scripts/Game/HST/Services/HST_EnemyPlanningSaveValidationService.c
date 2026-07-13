// Schema-68 persistence boundary for per-enemy planning cadence and the latest
// frozen decision. Serialized shape is validated before preset roles are known;
// configured role adoption and fail-closed role checks run after restore.
class HST_EnemyPlanningSaveValidationService
{
	static const int SCHEMA_VERSION = 68;
	static const int CONTRACT_VERSION = 1;
	static const int QUARANTINE_CONTRACT_VERSION = -68;
	static const int MAX_PLANNING_ROWS = 2;
	static const int MAX_ID_CHARACTERS = 192;
	static const int MAX_FINGERPRINT_CHARACTERS = 192;
	static const int MAX_REASON_CHARACTERS = 320;
	protected HST_CampaignSaveData m_SaveData;
	protected bool m_bPrepared;
	protected ref HST_EnemyPlanningAuthorityService m_Authority
		= new HST_EnemyPlanningAuthorityService();

	void PrepareBeforeGenericNormalization(
		HST_CampaignSaveData saveData,
		int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		if (!m_SaveData)
			return;
		EnsureSavedArrays();
		if (m_bPrepared)
			return;
		m_bPrepared = true;
		if (restoredSchemaVersion >= SCHEMA_VERSION)
			return;

		// A pre-68 envelope cannot claim planner authority. Baselines are created
		// only after the live preset identifies the configured enemy roles.
		m_SaveData.m_aEnemyPlanningStates.Clear();
		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
			ClearOrderPlanningAuthority(order);
	}

	void Normalize(
		HST_CampaignSaveData saveData,
		int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		if (!m_SaveData)
			return;
		EnsureSavedArrays();
		if (!m_bPrepared)
			PrepareBeforeGenericNormalization(saveData, restoredSchemaVersion);
		if (restoredSchemaVersion < SCHEMA_VERSION)
			return;

		bool nullRowFound;
		for (int planningIndex = m_SaveData.m_aEnemyPlanningStates.Count() - 1; planningIndex >= 0; planningIndex--)
		{
			if (m_SaveData.m_aEnemyPlanningStates[planningIndex])
				continue;
			m_SaveData.m_aEnemyPlanningStates.Remove(planningIndex);
			nullRowFound = true;
		}
		if (nullRowFound)
			QuarantineAllSavedPlanning("schema68 planning state array contained a null row");
		if (m_SaveData.m_aEnemyPlanningStates.Count() > MAX_PLANNING_ROWS)
			QuarantineAllSavedPlanning("schema68 planning state bound exceeded");

		ValidateSavedIdentitiesAndShapes();
		ValidateSavedOrderPlanningRows();
	}

	bool ValidateRestoredFactionRoles(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		int restoredSchemaVersion = -1)
	{
		if (!state || !preset || !ValidatePresetRoles(preset))
			return false;
		EnsureRuntimeArrays(state);
		int effectiveSchema = restoredSchemaVersion;
		if (effectiveSchema < 0)
			effectiveSchema = state.m_iLastLoadedSchemaVersion;

		if (effectiveSchema < SCHEMA_VERSION)
		{
			state.m_aEnemyPlanningStates.Clear();
			foreach (HST_EnemyOrderState legacyOrder : state.m_aEnemyOrders)
				ClearOrderPlanningAuthority(legacyOrder);
			bool adopted = InsertLegacyBaseline(
				state,
				preset.m_sOccupierFactionKey);
			if (!preset.m_sInvaderFactionKey.IsEmpty())
				adopted = InsertLegacyBaseline(
					state,
					preset.m_sInvaderFactionKey) && adopted;
			return adopted;
		}

		bool valid = true;
		foreach (HST_EnemyPlanningState candidate : state.m_aEnemyPlanningStates)
		{
			if (!candidate || candidate.m_iContractVersion
				== QUARANTINE_CONTRACT_VERSION)
				continue;
			if (!HST_FactionRelationService.IsEnemyFaction(
				preset,
				candidate.m_sFactionKey))
			{
				QuarantinePlanning(
					candidate,
					"schema68 planning authority was attached to a non-enemy faction");
				valid = false;
			}
		}

		valid = ValidateRuntimeRole(
			state,
			preset.m_sOccupierFactionKey) && valid;
		if (!preset.m_sInvaderFactionKey.IsEmpty())
			valid = ValidateRuntimeRole(
				state,
				preset.m_sInvaderFactionKey) && valid;
		valid = ValidateRuntimeOrderPlanningRows(state, preset) && valid;
		return valid;
	}

	protected void EnsureSavedArrays()
	{
		if (!m_SaveData.m_aEnemyPlanningStates)
			m_SaveData.m_aEnemyPlanningStates
				= new array<ref HST_EnemyPlanningState>();
		if (!m_SaveData.m_aEnemyOrders)
			m_SaveData.m_aEnemyOrders = new array<ref HST_EnemyOrderState>();
		if (!m_SaveData.m_aFactionPools)
			m_SaveData.m_aFactionPools = new array<ref HST_FactionPoolState>();
		if (!m_SaveData.m_aEnemyStrategicMutations)
			m_SaveData.m_aEnemyStrategicMutations
				= new array<ref HST_EnemyStrategicMutationState>();
	}

	protected void EnsureRuntimeArrays(HST_CampaignState state)
	{
		if (!state.m_aEnemyPlanningStates)
			state.m_aEnemyPlanningStates
				= new array<ref HST_EnemyPlanningState>();
		if (!state.m_aEnemyOrders)
			state.m_aEnemyOrders = new array<ref HST_EnemyOrderState>();
		if (!state.m_aFactionPools)
			state.m_aFactionPools = new array<ref HST_FactionPoolState>();
		if (!state.m_aEnemyStrategicMutations)
			state.m_aEnemyStrategicMutations
				= new array<ref HST_EnemyStrategicMutationState>();
	}

	protected void ValidateSavedIdentitiesAndShapes()
	{
		foreach (HST_EnemyPlanningState planning : m_SaveData.m_aEnemyPlanningStates)
		{
			if (!planning || planning.m_iContractVersion
				== QUARANTINE_CONTRACT_VERSION)
				continue;
			string failure;
			if (CountSavedPlanningRows(planning.m_sFactionKey) != 1)
				failure = "schema68 planning faction identity is duplicated";
			else if (!planning.m_sDecisionId.IsEmpty()
				&& CountSavedDecisionIds(planning.m_sDecisionId) != 1)
				failure = "schema68 planning decision identity is duplicated";
			else
				failure = ValidatePlanningShape(
					planning,
					m_SaveData.m_iElapsedSeconds);
			if (failure.IsEmpty())
				failure = ValidateSavedPoolLink(planning);
			if (failure.IsEmpty())
				failure = ValidateSavedLatestOrderBacklink(planning, false);
			if (!failure.IsEmpty())
				QuarantinePlanning(planning, failure);
		}
	}

	protected string ValidatePlanningShape(
		HST_EnemyPlanningState planning,
		int elapsedSeconds)
	{
		if (!planning)
			return "schema68 planning state is missing";
		if (planning.m_iContractVersion != CONTRACT_VERSION
			|| !IsBoundedRequired(planning.m_sFactionKey)
			|| !planning.m_sAuthorityFailure.IsEmpty())
			return "schema68 planning contract, faction, or failure state is invalid";
		if (elapsedSeconds < 0 || planning.m_iDecisionSequence < 0
			|| planning.m_iDecisionSequence >= int.MAX - 1
			|| planning.m_iRevision <= planning.m_iDecisionSequence
			|| planning.m_iRevision >= int.MAX)
			return "schema68 planning revision or decision sequence is invalid";
		if (planning.m_iLastPlanningBucketSecond < 0
			|| planning.m_iLastPlanningBucketSecond > elapsedSeconds
			|| planning.m_iLastPlanningBucketSecond
				> int.MAX - HST_EnemyPlanningAuthorityService.PLANNING_INTERVAL_SECONDS)
			return "schema68 planning cadence checkpoint is invalid";
		if (planning.m_iNextPlanningBucketSecond
			!= planning.m_iLastPlanningBucketSecond
				+ HST_EnemyPlanningAuthorityService.PLANNING_INTERVAL_SECONDS)
			return "schema68 planning cadence interval diverged";
		int retryUpperBound = elapsedSeconds;
		if (elapsedSeconds
			<= int.MAX - HST_EnemyPlanningAuthorityService.RETRY_INTERVAL_SECONDS)
			retryUpperBound = elapsedSeconds
				+ HST_EnemyPlanningAuthorityService.RETRY_INTERVAL_SECONDS;
		if (planning.m_iNextRetrySecond < 0
			|| planning.m_iNextRetrySecond > retryUpperBound)
			return "schema68 planning retry checkpoint exceeds its bounded restore window";
		if (planning.m_iDecisionSequence == 0)
			return ValidateBaselineShape(planning);

		if (planning.m_iDecisionBucketSecond
			!= planning.m_iLastPlanningBucketSecond)
			return "schema68 decision bucket or retry checkpoint is invalid";
		if (!IsDecisionDisposition(planning.m_sDisposition)
			|| !IsBoundedRequired(planning.m_sDecisionId)
			|| !IsBoundedRequired(planning.m_sPlannedOrderId)
			|| !IsBoundedRequired(planning.m_sPlannedOperationId)
			|| !IsBoundedRequired(planning.m_sPlannedDebitMutationId))
			return "schema68 planning decision identity or disposition is invalid";
		if (!IsBoundedOptional(planning.m_sPlannedManifestId)
			|| !IsBoundedOptional(planning.m_sPlannedManifestHash)
			|| !IsBoundedOptional(planning.m_sSelectedTargetZoneId)
			|| !IsBoundedOptional(planning.m_sSelectedSourceZoneId)
			|| !IsBoundedRequired(planning.m_sPlanningCapabilityHash)
			|| !IsBoundedRequired(planning.m_sSpendMode))
			return "schema68 planning decision selection or capability is invalid";
		if (planning.m_sPlannedManifestId.IsEmpty()
			!= planning.m_sPlannedManifestHash.IsEmpty())
			return "schema68 planning manifest identity is incomplete";
		if (!IsBoundedFingerprint(planning.m_sCommitmentFingerprint, true)
			|| !IsBoundedFingerprint(planning.m_sTargetCandidateFingerprint, true)
			|| !IsBoundedFingerprint(planning.m_sSourceCandidateFingerprint, true)
			|| !IsBoundedFingerprint(planning.m_sInputFingerprint, true)
			|| !IsBoundedFingerprint(planning.m_sDecisionFingerprint, true))
			return "schema68 planning fingerprint evidence is invalid";
		if (!IsBoundedReason(planning.m_sFailureReason)
			|| planning.m_iObservedWarLevel < 0
			|| planning.m_iObservedAggression < 0
			|| planning.m_iObservedPoolRevision <= 0
			|| planning.m_iObservedOperationalMutationCount < 0
			|| planning.m_iObservedAttackResources < 0
			|| planning.m_iObservedSupportResources < 0)
			return "schema68 observed planning input is invalid";
		if (planning.m_sDisposition == "committed"
			&& planning.m_iNextRetrySecond == 0
			&& !planning.m_sFailureReason.IsEmpty())
			return "schema68 committed planning decision carries failure residue";
		if (planning.m_sDisposition == "rejected"
			&& planning.m_sFailureReason.IsEmpty())
			return "schema68 rejected planning decision lacks failure evidence";
		if (planning.m_iNextRetrySecond > 0
			&& planning.m_sFailureReason.IsEmpty())
			return "schema68 planning retry checkpoint lacks failure evidence";
		if (planning.m_iCommitmentCount < 0
			|| planning.m_iTargetCandidateCount < 0
			|| planning.m_iSourceCandidateCount < 0
			|| planning.m_iAttackCost < 0
			|| planning.m_iSupportCost < 0)
			return "schema68 planning count or cost is invalid";
		if ((planning.m_iTargetCandidateCount == 0
				&& !planning.m_sSelectedTargetZoneId.IsEmpty())
			|| (planning.m_iTargetCandidateCount > 0
				&& !IsBoundedRequired(planning.m_sSelectedTargetZoneId)))
			return "schema68 selected target conflicts with its candidate set";
		string expectedDecisionId
			= HST_EnemyPlanningAuthorityService.BuildDecisionId(
				planning.m_sFactionKey,
				planning.m_iDecisionSequence,
				planning.m_iDecisionBucketSecond);
		string expectedOrderId
			= HST_EnemyPlanningAuthorityService.BuildOrderId(expectedDecisionId);
		if (planning.m_sDecisionId != expectedDecisionId
			|| planning.m_sPlannedOrderId != expectedOrderId
			|| planning.m_sPlannedOperationId
				!= HST_EnemyPlanningAuthorityService.BuildOperationId(expectedOrderId)
			|| planning.m_sPlannedDebitMutationId
				!= HST_EnemyPlanningAuthorityService.BuildDebitMutationId(
					expectedOrderId))
			return "schema68 planning derived identity diverged";
		if (!PressureArithmeticExact(planning))
			return "schema68 target-pressure arithmetic is invalid";
		if (!planning.m_bTargetPressureApplied
			&& planning.m_sDisposition != "prepared"
			&& (planning.m_iTargetPressureDelta != 0
				|| planning.m_iTargetPressureAfter
					!= planning.m_iTargetPressureBefore))
			return "schema68 unapplied target pressure changed its snapshot";
		if (planning.m_sDisposition == "skipped"
			&& planning.m_bTargetPressureApplied)
			return "schema68 skipped decision applied target pressure";
		if (planning.m_sInputFingerprint
			!= HST_EnemyPlanningAuthorityService.BuildInputFingerprint(planning))
			return "schema68 planning input fingerprint diverged";
		if (planning.m_sDecisionFingerprint
			!= HST_EnemyPlanningAuthorityService.BuildDecisionFingerprint(planning))
			return "schema68 planning decision fingerprint diverged";
		return "";
	}

	protected string ValidateBaselineShape(HST_EnemyPlanningState planning)
	{
		if (planning.m_sDisposition != "idle"
			|| planning.m_iDecisionBucketSecond != 0
			|| planning.m_bTargetPressureApplied)
			return "schema68 baseline planning cadence carries decision state";
		if (!IsBoundedReason(planning.m_sFailureReason))
			return "schema68 baseline planning retry failure is invalid";
		if (planning.m_iNextRetrySecond == 0)
		{
			if (planning.m_iRevision != 1
				|| !planning.m_sFailureReason.IsEmpty())
				return "schema68 baseline planning retry state is invalid";
		}
		else if (planning.m_iRevision <= 1
			|| planning.m_sFailureReason.IsEmpty())
			return "schema68 baseline planning retry state is invalid";
		if (HasObservedBaselineResidue(planning)
			|| HasSelectionBaselineResidue(planning)
			|| HasIdentityBaselineResidue(planning))
			return "schema68 baseline planning state invented a decision";
		return "";
	}

	protected bool HasObservedBaselineResidue(HST_EnemyPlanningState planning)
	{
		return planning.m_iObservedWarLevel != 0
			|| planning.m_iObservedAggression != 0
			|| planning.m_iObservedPoolRevision != 0
			|| planning.m_iObservedOperationalMutationCount != 0
			|| planning.m_iObservedAttackResources != 0
			|| planning.m_iObservedSupportResources != 0
			|| planning.m_iCommitmentCount != 0
			|| !planning.m_sCommitmentFingerprint.IsEmpty();
	}

	protected bool HasSelectionBaselineResidue(HST_EnemyPlanningState planning)
	{
		return planning.m_iTargetCandidateCount != 0
			|| !planning.m_sTargetCandidateFingerprint.IsEmpty()
			|| planning.m_iSourceCandidateCount != 0
			|| !planning.m_sSourceCandidateFingerprint.IsEmpty()
			|| !planning.m_sSelectedTargetZoneId.IsEmpty()
			|| !planning.m_sSelectedSourceZoneId.IsEmpty()
			|| planning.m_eSelectedOrderType
				!= HST_EEnemyOrderType.HST_ENEMY_ORDER_PATROL
			|| planning.m_ePlannedSupportType
				!= HST_ESupportRequestType.HST_SUPPORT_QRF
			|| !planning.m_sPlanningCapabilityHash.IsEmpty()
			|| !planning.m_sSpendMode.IsEmpty()
			|| planning.m_iAttackCost != 0
			|| planning.m_iSupportCost != 0;
	}

	protected bool HasIdentityBaselineResidue(HST_EnemyPlanningState planning)
	{
		return planning.m_iTargetPressureBefore != 0
			|| planning.m_iTargetPressureDelta != 0
			|| planning.m_iTargetPressureAfter != 0
			|| !planning.m_sDecisionId.IsEmpty()
			|| !planning.m_sPlannedOrderId.IsEmpty()
			|| !planning.m_sPlannedOperationId.IsEmpty()
			|| !planning.m_sPlannedManifestId.IsEmpty()
			|| !planning.m_sPlannedManifestHash.IsEmpty()
			|| !planning.m_sPlannedDebitMutationId.IsEmpty()
			|| !planning.m_sInputFingerprint.IsEmpty()
			|| !planning.m_sDecisionFingerprint.IsEmpty();
	}

	protected string ValidateSavedPoolLink(HST_EnemyPlanningState planning)
	{
		HST_FactionPoolState pool = FindUniqueSavedPool(planning.m_sFactionKey);
		if (!pool || pool.m_iStrategicContractVersion
			!= HST_EnemyStrategicResourceService.CONTRACT_VERSION
			|| !pool.m_sStrategicAuthorityFailure.IsEmpty())
			return "schema68 planning state lacks exact schema67 pool authority";
		return "";
	}

	protected string ValidateSavedLatestOrderBacklink(
		HST_EnemyPlanningState planning,
		bool adoptPrepared)
	{
		if (!planning || planning.m_iDecisionSequence == 0)
			return "";
		int orderCount = CountSavedOrdersForDecision(planning.m_sDecisionId);
		if (planning.m_sDisposition == "committed")
		{
			if (orderCount != 1)
				return "schema68 committed planning decision lacks one order backlink";
			return ValidateOrderBacklink(
				planning,
				FindSavedOrderForDecision(planning.m_sDecisionId));
		}
		if (planning.m_sDisposition == "prepared")
		{
			if (orderCount > 1)
				return "schema68 prepared planning decision has duplicate orders";
			if (orderCount == 1)
			{
				string failure = ValidateOrderBacklink(
					planning,
					FindSavedOrderForDecision(planning.m_sDecisionId));
				if (!failure.IsEmpty())
					return failure;
				if (adoptPrepared)
				{
					planning.m_sDisposition = "committed";
					planning.m_iNextRetrySecond = 0;
					planning.m_sFailureReason = "";
				}
			}
			return "";
		}
		if (orderCount != 0)
			return "schema68 non-committed planning decision has an order backlink";
		if (HasSavedDebitReceipt(planning.m_sPlannedDebitMutationId))
			return "schema68 non-committed planning decision has a debit receipt";
		return "";
	}

	protected string ValidateOrderBacklink(
		HST_EnemyPlanningState planning,
		HST_EnemyOrderState order)
	{
		if (!planning || !order)
			return "schema68 planning order backlink is missing";
		if (order.m_iPlanningContractVersion != CONTRACT_VERSION
			|| order.m_iPlanningDecisionSequence != planning.m_iDecisionSequence
			|| order.m_iPlanningBucketSecond != planning.m_iDecisionBucketSecond
			|| order.m_sPlanningDecisionId != planning.m_sDecisionId
			|| order.m_sPlanningInputFingerprint != planning.m_sInputFingerprint
			|| order.m_sPlanningDecisionFingerprint
				!= planning.m_sDecisionFingerprint)
			return "schema68 planning order authority fields diverged";
		if (order.m_sFactionKey != planning.m_sFactionKey
			|| order.m_sOrderId != planning.m_sPlannedOrderId
			|| order.m_sOperationId != planning.m_sPlannedOperationId
			|| order.m_sManifestId != planning.m_sPlannedManifestId
			|| order.m_sManifestHash != planning.m_sPlannedManifestHash)
			return "schema68 planning order identity backlink diverged";
		if (order.m_sSourceZoneId != planning.m_sSelectedSourceZoneId
			|| order.m_sTargetZoneId != planning.m_sSelectedTargetZoneId
			|| order.m_eType != planning.m_eSelectedOrderType
			|| order.m_ePlannedSupportType != planning.m_ePlannedSupportType
			|| order.m_sPlanningCapabilityHash
				!= planning.m_sPlanningCapabilityHash)
			return "schema68 planning order selection backlink diverged";
		if (order.m_iAttackCost != planning.m_iAttackCost
			|| order.m_iSupportCost != planning.m_iSupportCost
			|| order.m_sResourceDebitMutationId
				!= planning.m_sPlannedDebitMutationId)
			return "schema68 planning order accounting backlink diverged";
		return ValidateSavedDebitBacklink(planning, order);
	}

	protected string ValidateSavedDebitBacklink(
		HST_EnemyPlanningState planning,
		HST_EnemyOrderState order)
	{
		HST_EnemyStrategicMutationState mutation;
		int count;
		foreach (HST_EnemyStrategicMutationState candidate : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (!candidate || candidate.m_sMutationId
				!= planning.m_sPlannedDebitMutationId)
				continue;
			mutation = candidate;
			count++;
		}
		if (count != 1 || !mutation
			|| mutation.m_iContractVersion
				!= HST_EnemyStrategicResourceService.CONTRACT_VERSION
			|| !mutation.m_bApplied)
			return "schema68 committed planning debit receipt is missing or ambiguous";
		if (mutation.m_sFactionKey != planning.m_sFactionKey
			|| mutation.m_sOrderId != order.m_sOrderId
			|| mutation.m_sOperationId != order.m_sOperationId
			|| mutation.m_sManifestId != order.m_sManifestId
			|| mutation.m_sZoneId != order.m_sTargetZoneId)
			return "schema68 committed planning debit receipt backlink diverged";
		return "";
	}

	protected void ValidateSavedOrderPlanningRows()
	{
		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (!order || order.m_iPlanningContractVersion == 0
				|| order.m_iPlanningContractVersion
					== QUARANTINE_CONTRACT_VERSION)
				continue;
			string failure = ValidateOrderPlanningShape(order);
			HST_EnemyPlanningState planning = FindUniqueSavedPlanning(
				order.m_sFactionKey);
			if (failure.IsEmpty() && (!planning
				|| planning.m_iContractVersion != CONTRACT_VERSION
				|| order.m_iPlanningDecisionSequence
					> planning.m_iDecisionSequence
				|| order.m_iPlanningBucketSecond
					> planning.m_iLastPlanningBucketSecond))
				failure = "schema68 planned order exceeds its faction planning authority";
			if (failure.IsEmpty()
				&& CountSavedOrdersForDecision(order.m_sPlanningDecisionId) != 1)
				failure = "schema68 planned order decision identity is duplicated";
			if (failure.IsEmpty())
				continue;
			QuarantineOrderPlanning(order);
			if (planning)
				QuarantinePlanning(planning, failure);
		}
	}

	protected string ValidateOrderPlanningShape(HST_EnemyOrderState order)
	{
		if (!order || order.m_iPlanningContractVersion != CONTRACT_VERSION
			|| order.m_iPlanningDecisionSequence <= 0
			|| order.m_iPlanningBucketSecond < 0
			|| !IsBoundedRequired(order.m_sPlanningDecisionId)
			|| !IsBoundedFingerprint(order.m_sPlanningInputFingerprint, true)
			|| !IsBoundedFingerprint(
				order.m_sPlanningDecisionFingerprint,
				true)
			|| !IsBoundedRequired(order.m_sPlanningCapabilityHash))
			return "schema68 planned order authority shape is invalid";
		return "";
	}

	protected bool ValidateRuntimeRole(
		HST_CampaignState state,
		string factionKey)
	{
		HST_EnemyPlanningState planning;
		if (!FindUniqueRuntimePlanning(state, factionKey, planning))
		{
			string failure = "schema68 configured enemy planning state is missing or duplicated";
			if (CountRuntimePlanningRows(state, factionKey) == 0)
				InsertRuntimePlaceholder(state, factionKey, failure);
			else
				QuarantineRuntimePlanningByKey(state, factionKey, failure);
			return false;
		}
		if (planning.m_iContractVersion == QUARANTINE_CONTRACT_VERSION)
			return false;
		string failure = ValidatePlanningShape(planning, state.m_iElapsedSeconds);
		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		if (failure.IsEmpty() && (!pool
			|| pool.m_iStrategicContractVersion
				!= HST_EnemyStrategicResourceService.CONTRACT_VERSION
			|| !pool.m_sStrategicAuthorityFailure.IsEmpty()))
			failure = "schema68 configured enemy planning state lacks exact schema67 pool authority";
		if (failure.IsEmpty())
			failure = ValidateRuntimeLatestOrderBacklink(state, planning);
		if (failure.IsEmpty())
			return true;
		QuarantinePlanning(planning, failure);
		return false;
	}

	protected string ValidateRuntimeLatestOrderBacklink(
		HST_CampaignState state,
		HST_EnemyPlanningState planning)
	{
		if (!state || !planning || planning.m_iDecisionSequence == 0)
			return "";
		int count;
		HST_EnemyOrderState order;
		foreach (HST_EnemyOrderState candidate : state.m_aEnemyOrders)
		{
			if (!candidate || candidate.m_sPlanningDecisionId
				!= planning.m_sDecisionId)
				continue;
			order = candidate;
			count++;
		}
		if (planning.m_sDisposition == "committed")
		{
			if (count != 1)
				return "schema68 committed runtime planning order is missing or duplicated";
			return ValidateRuntimeOrderBacklink(state, planning, order);
		}
		if (planning.m_sDisposition == "prepared")
		{
			if (count > 1)
				return "schema68 prepared runtime planning order is duplicated";
			if (count == 0)
				return "";
			string failure = ValidateRuntimeOrderBacklink(state, planning, order);
			if (!failure.IsEmpty())
				return failure;
			planning.m_sDisposition = "committed";
			planning.m_iNextRetrySecond = 0;
			planning.m_sFailureReason = "";
			return "";
		}
		if (count != 0)
			return "schema68 non-committed runtime decision has an order backlink";
		if (state.FindEnemyStrategicMutation(planning.m_sPlannedDebitMutationId))
			return "schema68 non-committed runtime decision has a debit receipt";
		return "";
	}

	protected string ValidateRuntimeOrderBacklink(
		HST_CampaignState state,
		HST_EnemyPlanningState planning,
		HST_EnemyOrderState order)
	{
		string failure = ValidateRuntimeOrderFields(planning, order);
		if (!failure.IsEmpty())
			return failure;
		HST_EnemyStrategicMutationState mutation
			= state.FindEnemyStrategicMutation(planning.m_sPlannedDebitMutationId);
		if (!mutation || mutation.m_iContractVersion
			!= HST_EnemyStrategicResourceService.CONTRACT_VERSION
			|| !mutation.m_bApplied)
			return "schema68 runtime planning debit receipt is missing";
		if (mutation.m_sFactionKey != planning.m_sFactionKey
			|| mutation.m_sOrderId != order.m_sOrderId
			|| mutation.m_sOperationId != order.m_sOperationId
			|| mutation.m_sManifestId != order.m_sManifestId
			|| mutation.m_sZoneId != order.m_sTargetZoneId)
			return "schema68 runtime planning debit receipt backlink diverged";
		return "";
	}

	protected string ValidateRuntimeOrderFields(
		HST_EnemyPlanningState planning,
		HST_EnemyOrderState order)
	{
		if (!planning || !order)
			return "schema68 runtime planning order backlink is missing";
		if (order.m_iPlanningContractVersion != CONTRACT_VERSION
			|| order.m_iPlanningDecisionSequence != planning.m_iDecisionSequence
			|| order.m_iPlanningBucketSecond != planning.m_iDecisionBucketSecond
			|| order.m_sPlanningDecisionId != planning.m_sDecisionId
			|| order.m_sPlanningInputFingerprint != planning.m_sInputFingerprint
			|| order.m_sPlanningDecisionFingerprint
				!= planning.m_sDecisionFingerprint)
			return "schema68 runtime planning order authority diverged";
		if (order.m_sFactionKey != planning.m_sFactionKey
			|| order.m_sOrderId != planning.m_sPlannedOrderId
			|| order.m_sOperationId != planning.m_sPlannedOperationId
			|| order.m_sManifestId != planning.m_sPlannedManifestId
			|| order.m_sManifestHash != planning.m_sPlannedManifestHash)
			return "schema68 runtime planning order identity diverged";
		if (order.m_sSourceZoneId != planning.m_sSelectedSourceZoneId
			|| order.m_sTargetZoneId != planning.m_sSelectedTargetZoneId
			|| order.m_eType != planning.m_eSelectedOrderType
			|| order.m_ePlannedSupportType != planning.m_ePlannedSupportType
			|| order.m_sPlanningCapabilityHash
				!= planning.m_sPlanningCapabilityHash)
			return "schema68 runtime planning order selection diverged";
		if (order.m_iAttackCost != planning.m_iAttackCost
			|| order.m_iSupportCost != planning.m_iSupportCost
			|| order.m_sResourceDebitMutationId
				!= planning.m_sPlannedDebitMutationId)
			return "schema68 runtime planning order accounting diverged";
		return "";
	}

	protected bool ValidateRuntimeOrderPlanningRows(
		HST_CampaignState state,
		HST_CampaignPreset preset)
	{
		bool valid = true;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (!order || order.m_iPlanningContractVersion == 0
				|| order.m_iPlanningContractVersion
					== QUARANTINE_CONTRACT_VERSION)
				continue;
			HST_EnemyPlanningState planning = state.FindEnemyPlanningState(
				order.m_sFactionKey);
			string failure = ValidateOrderPlanningShape(order);
			if (failure.IsEmpty() && !HST_FactionRelationService.IsEnemyFaction(
				preset,
				order.m_sFactionKey))
				failure = "schema68 planned order belongs to a non-enemy faction";
			if (failure.IsEmpty() && (!planning
				|| planning.m_iContractVersion != CONTRACT_VERSION
				|| order.m_iPlanningDecisionSequence
					> planning.m_iDecisionSequence
				|| order.m_iPlanningBucketSecond
					> planning.m_iLastPlanningBucketSecond))
				failure = "schema68 planned order exceeds live planning authority";
			if (failure.IsEmpty()
				&& CountRuntimeOrdersForDecision(
					state,
					order.m_sPlanningDecisionId) != 1)
				failure = "schema68 runtime planned order decision identity is duplicated";
			if (failure.IsEmpty())
				continue;
			QuarantineOrderPlanning(order);
			if (planning)
				QuarantinePlanning(planning, failure);
			valid = false;
		}
		return valid;
	}

	protected bool InsertLegacyBaseline(
		HST_CampaignState state,
		string factionKey)
	{
		if (!state || factionKey.IsEmpty())
			return false;
		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		if (!pool || pool.m_iStrategicContractVersion
			!= HST_EnemyStrategicResourceService.CONTRACT_VERSION
			|| !pool.m_sStrategicAuthorityFailure.IsEmpty()
			|| state.m_iElapsedSeconds < 0
			|| state.m_iElapsedSeconds
				> int.MAX
					- HST_EnemyPlanningAuthorityService.PLANNING_INTERVAL_SECONDS)
		{
			InsertRuntimePlaceholder(
				state,
				factionKey,
				"schema68 migration requires one exact schema67 enemy pool");
			return false;
		}
		HST_EnemyPlanningState planning = new HST_EnemyPlanningState();
		planning.m_iContractVersion = CONTRACT_VERSION;
		planning.m_iRevision = 1;
		planning.m_sFactionKey = factionKey;
		planning.m_iLastPlanningBucketSecond = state.m_iElapsedSeconds;
		planning.m_iNextPlanningBucketSecond = state.m_iElapsedSeconds
			+ HST_EnemyPlanningAuthorityService.PLANNING_INTERVAL_SECONDS;
		planning.m_sDisposition = "idle";
		state.m_aEnemyPlanningStates.Insert(planning);
		return true;
	}

	protected bool PressureArithmeticExact(HST_EnemyPlanningState planning)
	{
		if (!planning || planning.m_iTargetPressureBefore < 0
			|| planning.m_iTargetPressureDelta < 0
			|| planning.m_iTargetPressureAfter < 0)
			return false;
		if (planning.m_iTargetPressureDelta > 0
			&& planning.m_iTargetPressureBefore
				> int.MAX - planning.m_iTargetPressureDelta)
			return false;
		return planning.m_iTargetPressureAfter
			== planning.m_iTargetPressureBefore
				+ planning.m_iTargetPressureDelta;
	}

	protected bool IsDecisionDisposition(string disposition)
	{
		return disposition == "prepared" || disposition == "committed"
			|| disposition == "skipped" || disposition == "rejected";
	}

	protected bool IsBoundedRequired(string value)
	{
		return !value.IsEmpty() && value == value.Trim()
			&& value.Length() <= MAX_ID_CHARACTERS;
	}

	protected bool IsBoundedOptional(string value)
	{
		return value == value.Trim() && value.Length() <= MAX_ID_CHARACTERS;
	}

	protected bool IsBoundedFingerprint(string value, bool required)
	{
		if (required && value.IsEmpty())
			return false;
		return value == value.Trim()
			&& value.Length() <= MAX_FINGERPRINT_CHARACTERS;
	}

	protected bool IsBoundedReason(string value)
	{
		return value == value.Trim() && value.Length() <= MAX_REASON_CHARACTERS;
	}

	protected bool ValidatePresetRoles(HST_CampaignPreset preset)
	{
		if (!preset || preset.m_sOccupierFactionKey.IsEmpty())
			return false;
		return preset.m_sInvaderFactionKey.IsEmpty()
			|| preset.m_sInvaderFactionKey != preset.m_sOccupierFactionKey;
	}

	protected int CountSavedPlanningRows(string factionKey)
	{
		int count;
		foreach (HST_EnemyPlanningState planning : m_SaveData.m_aEnemyPlanningStates)
		{
			if (planning && planning.m_sFactionKey == factionKey)
				count++;
		}
		return count;
	}

	protected int CountSavedDecisionIds(string decisionId)
	{
		int count;
		foreach (HST_EnemyPlanningState planning : m_SaveData.m_aEnemyPlanningStates)
		{
			if (planning && planning.m_sDecisionId == decisionId)
				count++;
		}
		return count;
	}

	protected HST_EnemyPlanningState FindUniqueSavedPlanning(string factionKey)
	{
		HST_EnemyPlanningState match;
		foreach (HST_EnemyPlanningState planning : m_SaveData.m_aEnemyPlanningStates)
		{
			if (!planning || planning.m_sFactionKey != factionKey)
				continue;
			if (match)
				return null;
			match = planning;
		}
		return match;
	}

	protected HST_FactionPoolState FindUniqueSavedPool(string factionKey)
	{
		HST_FactionPoolState match;
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

	protected int CountSavedOrdersForDecision(string decisionId)
	{
		int count;
		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (order && order.m_sPlanningDecisionId == decisionId)
				count++;
		}
		return count;
	}

	protected HST_EnemyOrderState FindSavedOrderForDecision(string decisionId)
	{
		HST_EnemyOrderState match;
		foreach (HST_EnemyOrderState order : m_SaveData.m_aEnemyOrders)
		{
			if (!order || order.m_sPlanningDecisionId != decisionId)
				continue;
			if (match)
				return null;
			match = order;
		}
		return match;
	}

	protected bool HasSavedDebitReceipt(string mutationId)
	{
		if (mutationId.IsEmpty())
			return false;
		foreach (HST_EnemyStrategicMutationState mutation : m_SaveData.m_aEnemyStrategicMutations)
		{
			if (mutation && mutation.m_sMutationId == mutationId
				&& mutation.m_iContractVersion
					== HST_EnemyStrategicResourceService.CONTRACT_VERSION
				&& mutation.m_bApplied)
				return true;
		}
		return false;
	}

	protected bool FindUniqueRuntimePlanning(
		HST_CampaignState state,
		string factionKey,
		out HST_EnemyPlanningState match)
	{
		match = null;
		foreach (HST_EnemyPlanningState planning : state.m_aEnemyPlanningStates)
		{
			if (!planning || planning.m_sFactionKey != factionKey)
				continue;
			if (match)
				return false;
			match = planning;
		}
		return match != null;
	}

	protected int CountRuntimePlanningRows(
		HST_CampaignState state,
		string factionKey)
	{
		int count;
		foreach (HST_EnemyPlanningState planning : state.m_aEnemyPlanningStates)
		{
			if (planning && planning.m_sFactionKey == factionKey)
				count++;
		}
		return count;
	}

	protected int CountRuntimeOrdersForDecision(
		HST_CampaignState state,
		string decisionId)
	{
		int count;
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sPlanningDecisionId == decisionId)
				count++;
		}
		return count;
	}

	protected void QuarantineAllSavedPlanning(string failure)
	{
		foreach (HST_EnemyPlanningState planning : m_SaveData.m_aEnemyPlanningStates)
		{
			if (planning && planning.m_iContractVersion != 0)
				QuarantinePlanning(planning, failure);
		}
	}

	protected void QuarantineRuntimePlanningByKey(
		HST_CampaignState state,
		string factionKey,
		string failure)
	{
		foreach (HST_EnemyPlanningState planning : state.m_aEnemyPlanningStates)
		{
			if (planning && planning.m_sFactionKey == factionKey)
				QuarantinePlanning(planning, failure);
		}
	}

	protected void InsertRuntimePlaceholder(
		HST_CampaignState state,
		string factionKey,
		string failure)
	{
		if (!state || factionKey.IsEmpty())
			return;
		HST_EnemyPlanningState placeholder = new HST_EnemyPlanningState();
		placeholder.m_sFactionKey = factionKey;
		m_Authority.Quarantine(placeholder, LimitText(failure));
		state.m_aEnemyPlanningStates.Insert(placeholder);
	}

	protected void QuarantinePlanning(
		HST_EnemyPlanningState planning,
		string failure)
	{
		if (!planning)
			return;
		m_Authority.Quarantine(planning, LimitText(failure));
	}

	protected void QuarantineOrderPlanning(HST_EnemyOrderState order)
	{
		if (order)
			order.m_iPlanningContractVersion = QUARANTINE_CONTRACT_VERSION;
	}

	protected void ClearOrderPlanningAuthority(HST_EnemyOrderState order)
	{
		if (!order)
			return;
		order.m_iPlanningContractVersion = 0;
		order.m_iPlanningDecisionSequence = 0;
		order.m_iPlanningBucketSecond = 0;
		order.m_sPlanningDecisionId = "";
		order.m_sPlanningInputFingerprint = "";
		order.m_sPlanningDecisionFingerprint = "";
		order.m_ePlannedSupportType
			= HST_ESupportRequestType.HST_SUPPORT_QRF;
		order.m_sPlanningCapabilityHash = "";
	}

	protected string LimitText(string value)
	{
		string trimmed = value.Trim();
		if (trimmed.Length() <= MAX_REASON_CHARACTERS)
			return trimmed;
		return trimmed.Substring(0, MAX_REASON_CHARACTERS);
	}
}
