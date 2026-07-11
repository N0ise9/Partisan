// Schema-60 restore boundary for exact paid player Search-and-Destroy support.
// Historical Search-and-Destroy requests remain on contract zero. Exact
// authority is accepted only from a complete, reciprocal persisted graph; no
// cost, roster, casualty, arrival, settlement, or refund is inferred here.
class HST_PlayerSearchDestroySaveValidationService
{
	static const int SCHEMA_VERSION = 60;
	static const int QUARANTINED_CONTRACT_VERSION = -60;
	static const string CONFLICT_EVENT_ID = "normalization_schema60_player_search_destroy_conflict";
	static const string MIGRATION_EVENT_ID = "migration_schema60_player_search_destroy";
	static const string QUARANTINE_STATUS = "exact_search_destroy_quarantined";
	static const string QUARANTINE_MODE = "exact_spawn_queue_quarantined";

	protected HST_CampaignSaveData m_SaveData;
	protected ref array<string> m_aHandledRequestIds = {};
	protected ref array<string> m_aHandledQuoteIds = {};
	protected ref array<string> m_aHandledManifestIds = {};
	protected ref array<string> m_aHandledOperationIds = {};
	protected ref array<string> m_aHandledBatchIds = {};
	protected ref array<string> m_aHandledGroupIds = {};
	protected ref array<string> m_aHandledTombstoneQuoteIds = {};

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		if (!m_SaveData)
			return;

		m_aHandledRequestIds.Clear();
		m_aHandledQuoteIds.Clear();
		m_aHandledManifestIds.Clear();
		m_aHandledOperationIds.Clear();
		m_aHandledBatchIds.Clear();
		m_aHandledGroupIds.Clear();
		m_aHandledTombstoneQuoteIds.Clear();

		int legacyCount;
		int validatedCount;
		int conflictCount;
		foreach (HST_SupportRequestState request : m_SaveData.m_aSupportRequests)
		{
			if (!request || request.m_eType != HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
				continue;
			if (!IsSchema60PlayerSearchDestroyRequestClaimant(m_SaveData, request))
			{
				if (restoredSchemaVersion < SCHEMA_VERSION)
					request.m_iOperationContractVersion = 0;
				legacyCount++;
				continue;
			}
			if (m_aHandledRequestIds.Contains(request.m_sRequestId))
				continue;

			if (restoredSchemaVersion < SCHEMA_VERSION)
			{
				QuarantineLinkedGraph(request, null, null, null, null, null,
					"pre-schema-60 save carried an unsupported exact Search-and-Destroy authority claim");
				conflictCount++;
				continue;
			}

			HST_ForceSettlementTombstoneState tombstone = FindUniqueTombstoneForRequest(request);
			if (tombstone && !HasLivePlanningGraph(request))
			{
				string archiveFailure = ValidateArchivedAggregate(request, tombstone);
				if (archiveFailure.IsEmpty())
				{
					NormalizeValidTerminalRequest(request);
					MarkHandled(request, null, null, null, null, null, tombstone);
					validatedCount++;
				}
				else
				{
					QuarantineLinkedGraph(request, null, null, null, null, tombstone, archiveFailure);
					conflictCount++;
				}
				continue;
			}

			HST_ForceQuoteState quote = FindUniqueQuote(request.m_sQuoteId);
			HST_ForceManifestState manifest = FindUniqueManifest(request.m_sManifestId);
			HST_OperationRecordState operation = FindUniqueOperation(request.m_sOperationId);
			HST_ForceSpawnResultState batch = ResolveUniqueBatch(request, operation);
			HST_ActiveGroupState group = ResolveUniqueGroup(request, operation);
			string failure = ValidateAcceptedAggregate(request, quote, manifest, operation, batch, group);
			if (failure.IsEmpty())
			{
				NormalizeValidAggregate(request, operation, batch, group);
				MarkHandled(request, quote, manifest, operation, batch, group, null);
				validatedCount++;
			}
			else
			{
				QuarantineLinkedGraph(request, quote, manifest, operation, batch, null, failure);
				conflictCount++;
			}
		}

		foreach (HST_ForceQuoteState quote : m_SaveData.m_aForceQuotes)
		{
			if (!IsSchema60PlayerSearchDestroyQuoteClaimant(m_SaveData, quote)
				|| m_aHandledQuoteIds.Contains(quote.m_sQuoteId))
				continue;
			HST_ForceManifestState manifest = FindUniqueManifest(quote.m_sManifestId);
			string failure;
			if (restoredSchemaVersion < SCHEMA_VERSION)
				failure = "pre-schema-60 save carried an unsupported exact Search-and-Destroy planning claim";
			else
				failure = ValidateStandalonePlanningGraph(quote, manifest);
			if (failure.IsEmpty())
			{
				MarkHandled(null, quote, manifest, null, null, null, null);
				validatedCount++;
			}
			else
			{
				QuarantineLinkedGraph(null, quote, manifest, null, null, null, failure);
				conflictCount++;
			}
		}

		foreach (HST_ForceSettlementTombstoneState tombstone : m_SaveData.m_aForceSettlementTombstones)
		{
			if (!IsSchema60PlayerSearchDestroyTombstoneClaimant(m_SaveData, tombstone)
				|| m_aHandledTombstoneQuoteIds.Contains(tombstone.m_sQuoteId))
				continue;
			HST_SupportRequestState request = FindUniqueRequest(tombstone.m_sSupportRequestId);
			string failure;
			if (restoredSchemaVersion < SCHEMA_VERSION)
				failure = "pre-schema-60 save carried an unsupported exact Search-and-Destroy archive claim";
			else
				failure = ValidateArchivedAggregate(request, tombstone);
			if (failure.IsEmpty())
			{
				NormalizeValidTerminalRequest(request);
				MarkHandled(request, null, null, null, null, null, tombstone);
				validatedCount++;
			}
			else
			{
				QuarantineLinkedGraph(request, null, null, null, null, tombstone, failure);
				conflictCount++;
			}
		}

		conflictCount += QuarantineOrphanClaimants(restoredSchemaVersion);
		if (restoredSchemaVersion < SCHEMA_VERSION)
			RecordLegacyMigration(legacyCount, conflictCount);
		RecordNormalizationConflict(validatedCount, conflictCount);
		m_SaveData = null;
	}

	static bool IsSchema60PlayerSearchDestroyRequestClaimant(
		HST_CampaignSaveData saveData,
		HST_SupportRequestState request)
	{
		if (!saveData || !request)
			return false;
		if (request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY
			&& request.m_iOperationContractVersion != 0)
			return true;
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (IsDirectOperationClaimant(operation)
				&& ((!request.m_sOperationId.IsEmpty() && request.m_sOperationId == operation.m_sOperationId)
					|| (!request.m_sRequestId.IsEmpty() && request.m_sRequestId == operation.m_sSupportRequestId)))
				return true;
		}
		foreach (HST_ForceQuoteState quote : saveData.m_aForceQuotes)
		{
			if (IsDirectQuoteClaimant(quote)
				&& ((!request.m_sQuoteId.IsEmpty() && request.m_sQuoteId == quote.m_sQuoteId)
					|| (!request.m_sRequestId.IsEmpty() && request.m_sRequestId == quote.m_sSupportRequestId)))
				return true;
		}
		return false;
	}

	static bool IsSchema60PlayerSearchDestroyQuoteClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceQuoteState quote)
	{
		if (!saveData || !quote)
			return false;
		if (IsDirectQuoteClaimant(quote))
			return true;
		foreach (HST_SupportRequestState request : saveData.m_aSupportRequests)
		{
			if (IsSchema60PlayerSearchDestroyRequestClaimant(saveData, request)
				&& ((!quote.m_sQuoteId.IsEmpty() && quote.m_sQuoteId == request.m_sQuoteId)
					|| (!quote.m_sSupportRequestId.IsEmpty() && quote.m_sSupportRequestId == request.m_sRequestId)))
				return true;
		}
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (IsDirectOperationClaimant(operation)
				&& ((!quote.m_sOperationId.IsEmpty() && quote.m_sOperationId == operation.m_sOperationId)
					|| (!quote.m_sQuoteId.IsEmpty() && quote.m_sQuoteId == operation.m_sQuoteId)))
				return true;
		}
		return false;
	}

	static bool IsSchema60PlayerSearchDestroyManifestClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceManifestState manifest)
	{
		if (!saveData || !manifest)
			return false;
		if (manifest.m_sPolicyId == HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_POLICY_ID
			|| manifest.m_sIntentId == "hst_search_destroy_regular")
			return true;
		foreach (HST_SupportRequestState request : saveData.m_aSupportRequests)
		{
			if (!IsSchema60PlayerSearchDestroyRequestClaimant(saveData, request))
				continue;
			if ((!manifest.m_sManifestId.IsEmpty() && manifest.m_sManifestId == request.m_sManifestId)
				|| (!manifest.m_sQuoteId.IsEmpty() && manifest.m_sQuoteId == request.m_sQuoteId)
				|| (!manifest.m_sOperationId.IsEmpty() && manifest.m_sOperationId == request.m_sOperationId))
				return true;
		}
		foreach (HST_ForceQuoteState quote : saveData.m_aForceQuotes)
		{
			if (IsSchema60PlayerSearchDestroyQuoteClaimant(saveData, quote)
				&& ((!manifest.m_sManifestId.IsEmpty() && manifest.m_sManifestId == quote.m_sManifestId)
					|| (!manifest.m_sQuoteId.IsEmpty() && manifest.m_sQuoteId == quote.m_sQuoteId)
					|| (!manifest.m_sOperationId.IsEmpty() && manifest.m_sOperationId == quote.m_sOperationId)))
				return true;
		}
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (IsDirectOperationClaimant(operation)
				&& ((!manifest.m_sManifestId.IsEmpty() && manifest.m_sManifestId == operation.m_sManifestId)
					|| (!manifest.m_sOperationId.IsEmpty() && manifest.m_sOperationId == operation.m_sOperationId)))
				return true;
		}
		return false;
	}

	static bool IsSchema60PlayerSearchDestroyOperationClaimant(
		HST_CampaignSaveData saveData,
		HST_OperationRecordState operation)
	{
		if (!saveData || !operation)
			return false;
		if (IsDirectOperationClaimant(operation))
			return true;
		foreach (HST_SupportRequestState request : saveData.m_aSupportRequests)
		{
			if (request && request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY
				&& request.m_iOperationContractVersion != 0
				&& ((!operation.m_sOperationId.IsEmpty() && operation.m_sOperationId == request.m_sOperationId)
					|| (!operation.m_sSupportRequestId.IsEmpty() && operation.m_sSupportRequestId == request.m_sRequestId)))
				return true;
		}
		foreach (HST_ForceQuoteState quote : saveData.m_aForceQuotes)
		{
			if (IsDirectQuoteClaimant(quote)
				&& ((!operation.m_sOperationId.IsEmpty() && operation.m_sOperationId == quote.m_sOperationId)
					|| (!operation.m_sQuoteId.IsEmpty() && operation.m_sQuoteId == quote.m_sQuoteId)))
				return true;
		}
		return false;
	}

	static bool IsSchema60PlayerSearchDestroyTransactionClaimant(
		HST_CampaignSaveData saveData,
		HST_ResourceTransactionState transaction)
	{
		if (!saveData || !transaction)
			return false;
		foreach (HST_SupportRequestState request : saveData.m_aSupportRequests)
		{
			if (!IsSchema60PlayerSearchDestroyRequestClaimant(saveData, request))
				continue;
			if ((!transaction.m_sQuoteId.IsEmpty() && transaction.m_sQuoteId == request.m_sQuoteId)
				|| (!transaction.m_sManifestId.IsEmpty() && transaction.m_sManifestId == request.m_sManifestId)
				|| (!transaction.m_sOperationId.IsEmpty() && transaction.m_sOperationId == request.m_sOperationId)
				|| (!transaction.m_sTransactionId.IsEmpty()
					&& (transaction.m_sTransactionId == request.m_sMoneyTransactionId
						|| transaction.m_sTransactionId == request.m_sHRTransactionId)))
				return true;
		}
		foreach (HST_ForceQuoteState quote : saveData.m_aForceQuotes)
		{
			if (!IsSchema60PlayerSearchDestroyQuoteClaimant(saveData, quote))
				continue;
			if ((!transaction.m_sQuoteId.IsEmpty() && transaction.m_sQuoteId == quote.m_sQuoteId)
				|| (!transaction.m_sManifestId.IsEmpty() && transaction.m_sManifestId == quote.m_sManifestId)
				|| (!transaction.m_sOperationId.IsEmpty() && transaction.m_sOperationId == quote.m_sOperationId)
				|| transaction.m_sTransactionId == quote.m_sMoneyTransactionId
				|| transaction.m_sTransactionId == quote.m_sHRTransactionId)
				return true;
		}
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (IsDirectOperationClaimant(operation) && !transaction.m_sOperationId.IsEmpty()
				&& transaction.m_sOperationId == operation.m_sOperationId)
				return true;
		}
		return false;
	}

	static bool IsSchema60PlayerSearchDestroyBatchClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceSpawnResultState batch)
	{
		if (!saveData || !batch)
			return false;
		foreach (HST_SupportRequestState request : saveData.m_aSupportRequests)
		{
			if (!IsSchema60PlayerSearchDestroyRequestClaimant(saveData, request))
				continue;
			if ((!batch.m_sResultId.IsEmpty() && batch.m_sResultId == request.m_sSpawnResultId)
				|| (!batch.m_sRequestId.IsEmpty() && batch.m_sRequestId == request.m_sRequestId)
				|| (!batch.m_sOperationId.IsEmpty() && batch.m_sOperationId == request.m_sOperationId)
				|| (!batch.m_sManifestId.IsEmpty() && batch.m_sManifestId == request.m_sManifestId))
				return true;
		}
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!IsDirectOperationClaimant(operation))
				continue;
			if ((!batch.m_sResultId.IsEmpty() && batch.m_sResultId == operation.m_sSpawnResultId)
				|| (!batch.m_sOperationId.IsEmpty() && batch.m_sOperationId == operation.m_sOperationId)
				|| (!batch.m_sProjectionId.IsEmpty() && batch.m_sProjectionId == operation.m_sProjectionId))
				return true;
		}
		return false;
	}

	static bool IsSchema60PlayerSearchDestroyGroupClaimant(
		HST_CampaignSaveData saveData,
		HST_ActiveGroupState group)
	{
		if (!saveData || !group)
			return false;
		foreach (HST_SupportRequestState request : saveData.m_aSupportRequests)
		{
			if (!IsSchema60PlayerSearchDestroyRequestClaimant(saveData, request))
				continue;
			if ((!group.m_sSupportRequestId.IsEmpty() && group.m_sSupportRequestId == request.m_sRequestId)
				|| (!group.m_sOperationId.IsEmpty() && group.m_sOperationId == request.m_sOperationId)
				|| (!group.m_sManifestId.IsEmpty() && group.m_sManifestId == request.m_sManifestId)
				|| (!group.m_sSpawnResultId.IsEmpty() && group.m_sSpawnResultId == request.m_sSpawnResultId))
				return true;
		}
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!IsDirectOperationClaimant(operation))
				continue;
			if ((!group.m_sGroupId.IsEmpty() && group.m_sGroupId == operation.m_sGroupId)
				|| (!group.m_sOperationId.IsEmpty() && group.m_sOperationId == operation.m_sOperationId)
				|| (!group.m_sProjectionId.IsEmpty() && group.m_sProjectionId == operation.m_sProjectionId))
				return true;
		}
		return false;
	}

	static bool IsSchema60PlayerSearchDestroyTombstoneClaimant(
		HST_CampaignSaveData saveData,
		HST_ForceSettlementTombstoneState tombstone)
	{
		if (!saveData || !tombstone)
			return false;
		bool direct = tombstone.m_sQuoteKind == HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_SEARCH_DESTROY
			|| tombstone.m_sPolicyId == HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_POLICY_ID
			|| (tombstone.m_eSupportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY
				&& tombstone.m_iOperationContractVersion != 0);
		if (direct)
			return true;
		foreach (HST_SupportRequestState request : saveData.m_aSupportRequests)
		{
			if (!IsSchema60PlayerSearchDestroyRequestClaimant(saveData, request))
				continue;
			if ((!tombstone.m_sSupportRequestId.IsEmpty() && tombstone.m_sSupportRequestId == request.m_sRequestId)
				|| (!tombstone.m_sQuoteId.IsEmpty() && tombstone.m_sQuoteId == request.m_sQuoteId)
				|| (!tombstone.m_sManifestId.IsEmpty() && tombstone.m_sManifestId == request.m_sManifestId)
				|| (!tombstone.m_sOperationId.IsEmpty() && tombstone.m_sOperationId == request.m_sOperationId))
				return true;
		}
		foreach (HST_ForceQuoteState quote : saveData.m_aForceQuotes)
		{
			if (!IsDirectQuoteClaimant(quote))
				continue;
			if ((!tombstone.m_sSupportRequestId.IsEmpty() && tombstone.m_sSupportRequestId == quote.m_sSupportRequestId)
				|| (!tombstone.m_sQuoteId.IsEmpty() && tombstone.m_sQuoteId == quote.m_sQuoteId)
				|| (!tombstone.m_sManifestId.IsEmpty() && tombstone.m_sManifestId == quote.m_sManifestId)
				|| (!tombstone.m_sOperationId.IsEmpty() && tombstone.m_sOperationId == quote.m_sOperationId))
				return true;
		}
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (!IsDirectOperationClaimant(operation))
				continue;
			if ((!tombstone.m_sSupportRequestId.IsEmpty() && tombstone.m_sSupportRequestId == operation.m_sSupportRequestId)
				|| (!tombstone.m_sQuoteId.IsEmpty() && tombstone.m_sQuoteId == operation.m_sQuoteId)
				|| (!tombstone.m_sManifestId.IsEmpty() && tombstone.m_sManifestId == operation.m_sManifestId)
				|| (!tombstone.m_sOperationId.IsEmpty() && tombstone.m_sOperationId == operation.m_sOperationId))
				return true;
		}
		return false;
	}

	static bool HasSchema60PlayerSearchDestroyClaimant(HST_CampaignSaveData saveData)
	{
		if (!saveData)
			return false;
		foreach (HST_SupportRequestState request : saveData.m_aSupportRequests)
		{
			if (IsSchema60PlayerSearchDestroyRequestClaimant(saveData, request))
				return true;
		}
		foreach (HST_ForceQuoteState quote : saveData.m_aForceQuotes)
		{
			if (IsSchema60PlayerSearchDestroyQuoteClaimant(saveData, quote))
				return true;
		}
		foreach (HST_OperationRecordState operation : saveData.m_aOperations)
		{
			if (IsSchema60PlayerSearchDestroyOperationClaimant(saveData, operation))
				return true;
		}
		foreach (HST_ForceSettlementTombstoneState tombstone : saveData.m_aForceSettlementTombstones)
		{
			if (IsSchema60PlayerSearchDestroyTombstoneClaimant(saveData, tombstone))
				return true;
		}
		return false;
	}

	protected static bool IsDirectQuoteClaimant(HST_ForceQuoteState quote)
	{
		return quote && (quote.m_sQuoteKind == HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_SEARCH_DESTROY
			|| quote.m_sPolicyId == HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_POLICY_ID);
	}

	protected static bool IsDirectOperationClaimant(HST_OperationRecordState operation)
	{
		return operation && operation.m_eType
			== HST_EOperationType.HST_OPERATION_TYPE_PLAYER_SUPPORT_SEARCH_DESTROY;
	}

	protected string ValidateAcceptedAggregate(
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!request || !quote || !manifest || !operation)
			return "exact Search-and-Destroy restore authority is incomplete or ambiguous";
		if (CountRequestsByAnyIdentity(request) != 1 || CountQuotesByAnyIdentity(request, quote) != 1
			|| CountManifestsByAnyIdentity(request, manifest) != 1
			|| CountOperationsByAnyIdentity(request, operation) != 1)
			return "exact Search-and-Destroy planning or operation identity is ambiguous";
		if (FindUniqueTombstoneForRequest(request))
			return "live exact Search-and-Destroy graph collides with archived authority";

		string failure = ValidateFrozenPlanning(quote, manifest, true);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateRequestLinks(request, quote, manifest, operation);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateCommittedLedger(request, quote, operation);
		if (!failure.IsEmpty())
			return failure;

		HST_CampaignState validationState = BuildValidationState();
		HST_OperationService operations = new HST_OperationService();
		failure = operations.ValidateExactPlayerSearchDestroy(
			validationState,
			operation,
			request,
			quote,
			manifest);
		if (!failure.IsEmpty())
			return "exact Search-and-Destroy operation conflicts: " + failure;

		bool settled = operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED;
		if (settled)
		{
			if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED
				&& request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
				return "settled exact Search-and-Destroy operation has a nonterminal request";
			if (request.m_sResolutionKind.IsEmpty()
				|| operation.m_sSettlementId != HST_OperationService.BuildSettlementId(
					operation.m_sOperationId,
					request.m_sResolutionKind))
				return "settled exact Search-and-Destroy receipt is not reciprocal";
		}
		else if (request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_QUEUED
			&& request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
			return "open exact Search-and-Destroy operation has a terminal request";

		return ValidateExecutionGraph(request, quote, manifest, operation, batch, group, settled);
	}

	protected string ValidateFrozenPlanning(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		bool requireAccepted)
	{
		if (!quote || !manifest)
			return "exact Search-and-Destroy quote or manifest is missing";
		if (quote.m_eSupportType != HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY
			|| quote.m_sQuoteKind != HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_SEARCH_DESTROY
			|| quote.m_sPolicyId != HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_POLICY_ID
			|| manifest.m_sPolicyId != HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_POLICY_ID
			|| manifest.m_sIntentId != "hst_search_destroy_regular")
			return "exact Search-and-Destroy planning family identity conflicts";
		if (requireAccepted && quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
			return "exact Search-and-Destroy quote is not accepted";
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		string integrityFailure;
		if (!integrity.ValidateFrozenPlayerSupportQuote(manifest, quote, false, integrityFailure))
			return "exact Search-and-Destroy frozen planning conflicts: " + integrityFailure;
		return "";
	}

	protected string ValidateRequestLinks(
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation)
	{
		if (request.m_eType != HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY
			|| request.m_iOperationContractVersion != HST_OperationService.EXACT_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION
			|| !request.m_bPlayerRequested)
			return "exact Search-and-Destroy request contract conflicts";
		if (request.m_sRequestId.IsEmpty()
			|| request.m_sRequestId != quote.m_sSupportRequestId
			|| request.m_sOperationId != quote.m_sOperationId
			|| request.m_sOperationId != manifest.m_sOperationId
			|| request.m_sOperationId != operation.m_sOperationId
			|| request.m_sQuoteId != quote.m_sQuoteId
			|| request.m_sManifestId != manifest.m_sManifestId)
			return "exact Search-and-Destroy reciprocal aggregate links conflict";
		if (request.m_sSpawnResultId != "spawn_" + request.m_sRequestId
			|| request.m_sCommandRequestId != quote.m_sConfirmationRequestId
			|| request.m_sMoneyTransactionId != quote.m_sMoneyTransactionId
			|| request.m_sHRTransactionId != quote.m_sHRTransactionId)
			return "exact Search-and-Destroy execution or ledger identity conflicts";
		if (request.m_sFactionKey != quote.m_sFactionKey
			|| request.m_sCapabilityId != HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_CAPABILITY_ID
			|| request.m_sAssetProfileId != HST_ForcePlanningService.SUPPORT_SEARCH_DESTROY_ASSET_PROFILE_ID
			|| request.m_sSourceZoneId != quote.m_sSourceZoneId
			|| request.m_sTargetZoneId != quote.m_sTargetZoneId
			|| request.m_vSourcePosition != quote.m_vSourcePosition
			|| request.m_vTargetPosition != quote.m_vTargetPosition)
			return "exact Search-and-Destroy owner, capability, or assignment conflicts";
		if (request.m_sCompositionRequestId != manifest.m_sManifestId
			|| request.m_sCompositionIntentId != manifest.m_sIntentId
			|| request.m_sCompositionTier != "exact"
			|| request.m_sPhysicalizationMode != HST_SupportRequestService.EXACT_PLAYER_SUPPORT_MODE)
			return "exact Search-and-Destroy composition or projection policy conflicts";
		if (request.m_iMoneyCost != manifest.m_iMoneyCost
			|| request.m_iHRCost != manifest.m_iHRCost
			|| request.m_iPlannedInfantryCount != manifest.m_iAcceptedMemberCount
			|| request.m_iCompositionCost != manifest.m_iMoneyCost
			|| request.m_iCompositionManpower != manifest.m_iAcceptedMemberCount
			|| request.m_iCompositionVehicleCount != 0
			|| request.m_iCompositionArmedVehicleCount != 0
			|| request.m_iAttackCost != 0 || request.m_iSupportCost != 0)
			return "exact Search-and-Destroy cost or frozen roster conflicts";
		if (request.m_iETASeconds != quote.m_iETASeconds
			|| request.m_iCooldownUntilSecond - request.m_iRequestedAtSecond != quote.m_iCooldownSeconds
			|| request.m_iRefundedHR < 0 || request.m_iRefundedHR > request.m_iHRCost)
			return "exact Search-and-Destroy schedule or refund evidence conflicts";
		if (request.m_bHelicopterStyle || request.m_bPhysicalStrikeSpawned
			|| request.m_bGarageVehicleConsumed || !request.m_sSelectedGarageVehicleId.IsEmpty()
			|| !request.m_sSelectedGarageVehiclePrefab.IsEmpty())
			return "exact Search-and-Destroy infantry-only request contains foreign asset authority";
		return "";
	}

	protected string ValidateCommittedLedger(
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_OperationRecordState operation)
	{
		HST_ResourceTransactionState money = FindUniqueTransaction(quote.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = FindUniqueTransaction(quote.m_sHRTransactionId);
		HST_ForcePlanningIntegrityService integrity = new HST_ForcePlanningIntegrityService();
		if (!money || !hr || money == hr
			|| !integrity.TransactionMatchesAcceptedPlayerSupportQuote(
				money, quote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost)
			|| !integrity.TransactionMatchesAcceptedPlayerSupportQuote(
				hr, quote, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost))
			return "exact Search-and-Destroy committed ledger receipts conflict";
		if (!TransactionSettlementIsConsistent(money) || !TransactionSettlementIsConsistent(hr)
			|| request.m_iRefundedHR != hr.m_iRefundedAmount)
			return "exact Search-and-Destroy refund evidence conflicts";
		if (operation && operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
		{
			bool openLedgerExact = money.m_eStatus
				== HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
			openLedgerExact = openLedgerExact && hr.m_eStatus
				== HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
			openLedgerExact = openLedgerExact && money.m_iRefundedAmount == 0;
			openLedgerExact = openLedgerExact && hr.m_iRefundedAmount == 0;
			openLedgerExact = openLedgerExact && request.m_iRefundedHR == 0;
			if (!openLedgerExact)
				return "open exact Search-and-Destroy operation contains settled or refunded ledger authority";
		}
		if (CountTransactionsByPlanningIdentity(quote) != 2)
			return "exact Search-and-Destroy ledger identity is ambiguous";
		return "";
	}

	protected string ValidateExecutionGraph(
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		bool settled)
	{
		bool hasExecution = !operation.m_sSpawnResultId.IsEmpty() || !operation.m_sForceId.IsEmpty()
			|| !operation.m_sProjectionId.IsEmpty() || !operation.m_sGroupId.IsEmpty();
		if (!hasExecution)
		{
			if (batch || group || !request.m_sGroupId.IsEmpty()
				|| CountBatchesByPlanningIdentity(request) > 0
				|| CountGroupsByPlanningIdentity(request) > 0)
				return "unlinked exact Search-and-Destroy execution claimant is present";
			return "";
		}
		if (!settled && (!batch || !group))
			return "exact Search-and-Destroy execution graph is incomplete or ambiguous";

		string expectedResultId = "spawn_" + request.m_sRequestId;
		string expectedForceId = "force_" + operation.m_sOperationId;
		string expectedProjectionId = "projection_" + operation.m_sOperationId;
		if (operation.m_sSpawnResultId != expectedResultId
			|| operation.m_sForceId != expectedForceId
			|| operation.m_sProjectionId != expectedProjectionId
			|| operation.m_sGroupId != expectedProjectionId)
			return "exact Search-and-Destroy deterministic execution identity conflicts";
		if (!settled && request.m_sGroupId != expectedProjectionId)
			return "open exact Search-and-Destroy request lost its projection backlink";
		if (settled && !request.m_sGroupId.IsEmpty()
			&& request.m_sGroupId != expectedProjectionId)
			return "settled exact Search-and-Destroy request has a foreign projection backlink";

		string failure = ValidateOptionalExecutionBatch(
			request,
			manifest,
			operation,
			batch,
			expectedResultId,
			expectedForceId,
			expectedProjectionId,
			settled);
		if (!failure.IsEmpty())
			return failure;
		return ValidateOptionalExecutionGroup(
			request,
			manifest,
			operation,
			batch,
			group,
			expectedResultId,
			expectedForceId,
			expectedProjectionId,
			settled);
	}

	protected string ValidateOptionalExecutionBatch(
		HST_SupportRequestState request,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		string expectedResultId,
		string expectedForceId,
		string expectedProjectionId,
		bool settled)
	{
		if (!batch)
		{
			if (!settled || CountBatchesByPlanningIdentity(request) > 0)
				return "exact Search-and-Destroy execution batch is missing or ambiguous";
			return "";
		}
		if (batch.m_sResultId != expectedResultId || batch.m_sRequestId != request.m_sRequestId
			|| batch.m_sManifestId != manifest.m_sManifestId
			|| batch.m_sManifestHash != manifest.m_sManifestHash
			|| batch.m_sOperationId != operation.m_sOperationId
			|| batch.m_sForceId != expectedForceId
			|| batch.m_sProjectionId != expectedProjectionId)
			return "exact Search-and-Destroy batch backlinks conflict";
		if (CountBatchesByAnyIdentity(request, batch) != 1)
			return "exact Search-and-Destroy execution batch identity is ambiguous";
		return ValidateBatchSlots(manifest, batch, settled);
	}

	protected string ValidateOptionalExecutionGroup(
		HST_SupportRequestState request,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		string expectedResultId,
		string expectedForceId,
		string expectedProjectionId,
		bool settled)
	{
		if (!group)
		{
			if (!settled || CountGroupsByPlanningIdentity(request) > 0)
				return "exact Search-and-Destroy execution group is missing or ambiguous";
			return "";
		}
		if (!batch)
			return "settled exact Search-and-Destroy group outlived its pinned batch";
		if (group.m_sGroupId != expectedProjectionId || group.m_sProjectionId != expectedProjectionId
			|| group.m_sForceId != expectedForceId || group.m_sSpawnResultId != expectedResultId
			|| group.m_sOperationId != operation.m_sOperationId
			|| group.m_sManifestId != manifest.m_sManifestId
			|| group.m_sSupportRequestId != request.m_sRequestId)
			return "exact Search-and-Destroy group backlinks conflict";
		if (CountGroupsByAnyIdentity(request, group) != 1)
			return "exact Search-and-Destroy execution group identity is ambiguous";
		return ValidateGroup(manifest, request, operation, batch, group, settled);
	}

	protected string ValidateBatchSlots(
		HST_ForceManifestState manifest,
		HST_ForceSpawnResultState batch,
		bool settled)
	{
		if (!manifest || !batch || manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
			return "exact Search-and-Destroy batch or manifest root is missing";
		int expectedSlots = manifest.m_aMembers.Count() + 1;
		if (batch.m_iExpectedSlotCount != expectedSlots || batch.m_aSlotResults.Count() != expectedSlots
			|| batch.m_bExternalAssetAuthority)
			return "exact Search-and-Destroy batch slot count or ownership conflicts";
		string rootSlotId = manifest.m_aGroups[0].m_sElementId;
		if (CountBatchSlots(batch, rootSlotId, HST_ForceSpawnQueueService.SLOT_KIND_GROUP) != 1)
			return "exact Search-and-Destroy group-root slot conflicts";
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member || CountBatchSlots(batch, member.m_sSlotId,
				HST_ForceSpawnQueueService.SLOT_KIND_MEMBER) != 1)
				return "exact Search-and-Destroy member-slot bijection conflicts";
		}
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot || slot.m_sSlotId.IsEmpty() || slot.m_sProjectionId != batch.m_sProjectionId)
				return "exact Search-and-Destroy batch slot identity conflicts";
			bool rootSlot = slot.m_sSlotId == rootSlotId
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_GROUP;
			bool memberSlot = manifest.FindMemberSlot(slot.m_sSlotId) != null
				&& slot.m_sSlotKind == HST_ForceSpawnQueueService.SLOT_KIND_MEMBER;
			if (!rootSlot && !memberSlot)
				return "exact Search-and-Destroy batch contains a foreign slot";
			if (slot.m_bCasualtyConfirmed)
			{
				if (!memberSlot || !slot.m_bEverAlive
					|| slot.m_eStatus != HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED
					|| slot.m_iCasualtyAtSecond < batch.m_iCreatedAtSecond)
					return "exact Search-and-Destroy casualty evidence conflicts";
				continue;
			}
			if (slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_RETIRED)
				return "exact Search-and-Destroy batch contains an unproven retired slot";
			if (memberSlot && slot.m_eStatus == HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_REGISTERED
				&& !slot.m_bEverAlive)
				return "registered exact Search-and-Destroy member lacks durable living evidence";
		}
		if (!settled && (batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED
			|| batch.m_eStatus == HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_FAILED_FINAL)
			&& batch.m_sTerminalReason.IsEmpty() && batch.m_sLastFailureReason.IsEmpty())
			return "terminal exact Search-and-Destroy batch lacks failure evidence";
		return "";
	}

	protected string ValidateGroup(
		HST_ForceManifestState manifest,
		HST_SupportRequestState request,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		bool settled)
	{
		HST_ForceManifestGroupState root = manifest.m_aGroups[0];
		if (group.m_sZoneId != request.m_sTargetZoneId || group.m_sFactionKey != request.m_sFactionKey
			|| group.m_sPrefab != root.m_sPrefab
			|| group.m_sCompositionRequestId != manifest.m_sManifestId
			|| group.m_sCompositionIntentId != manifest.m_sIntentId
			|| group.m_sCompositionTier != "exact")
			return "exact Search-and-Destroy group role or composition conflicts";
		if (group.m_sSpawnFallbackMode != HST_SupportRequestService.EXACT_PLAYER_SUPPORT_MODE
			|| group.m_bQRF || group.m_iVehicleCount != 0 || group.m_iOriginalVehicleCount != 0
			|| !group.m_sEnemyOrderId.IsEmpty() || !group.m_sMissionInstanceId.IsEmpty())
			return "exact Search-and-Destroy infantry-only group contains foreign authority";
		int accepted = manifest.m_iAcceptedMemberCount;
		if (group.m_iOriginalInfantryCount != accepted
			|| group.m_iCompositionManpower != accepted
			|| group.m_iCompositionCost != manifest.m_iMoneyCost)
			return "exact Search-and-Destroy original roster or cost conflicts";
		if (!CountWithin(group.m_iInfantryCount, accepted)
			|| !CountWithin(group.m_iLastSeenAliveCount, accepted)
			|| !CountWithin(group.m_iSurvivorInfantryCount, accepted)
			|| !CountWithin(group.m_iDurableLivingInfantryCount, accepted))
			return "exact Search-and-Destroy survivor aggregate is out of bounds";

		HST_ForceSpawnQueueService queue = new HST_ForceSpawnQueueService();
		int casualties = queue.CountConfirmedCasualtyMemberSlots(batch);
		int living = accepted - casualties;
		if (living < 0)
			return "exact Search-and-Destroy casualty count exceeds its frozen roster";
		if (batch.m_iSuccessfulHandoffCount > 0
			&& (group.m_iDurableLivingInfantryCount != living
				|| group.m_iSurvivorInfantryCount != living
				|| group.m_iLastSeenAliveCount != living))
			return "exact Search-and-Destroy survivor aggregate conflicts with slot evidence";

		if (settled)
			return "";
		bool live = operation.m_ePositionAuthority
			== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		if (live && (IsZeroVector(group.m_vPosition) || !group.m_bSpawnedEntity
			|| batch.m_eStatus != HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_SUCCEEDED))
			return "live exact Search-and-Destroy projection lacks physical handoff evidence";
		if (!live && operation.m_ePositionAuthority
			== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& !PositionsMatch(group.m_vPosition, operation.m_vStrategicPosition, 2.0))
			return "strategic exact Search-and-Destroy group position conflicts";
		return "";
	}

	protected string ValidateStandalonePlanningGraph(
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest)
	{
		string failure = ValidateFrozenPlanning(quote, manifest, false);
		if (!failure.IsEmpty())
			return failure;
		if (CountStandaloneQuotes(quote) != 1 || CountStandaloneManifests(quote, manifest) != 1)
			return "standalone exact Search-and-Destroy planning identity is ambiguous";
		if (CountRequestsForQuote(quote) > 0 || CountOperationsForQuote(quote) > 0
			|| CountBatchesForQuote(quote) > 0 || CountGroupsForQuote(quote) > 0
			|| CountTombstonesForQuote(quote.m_sQuoteId) > 0)
			return "standalone exact Search-and-Destroy planning row has executable or archived backlinks";
		int transactionCount = CountTransactionsByPlanningIdentity(quote);
		if (quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
		{
			if (transactionCount != 0)
				return "issued exact Search-and-Destroy quote has an interrupted ledger graph";
			return "";
		}
		if (quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
			return "accepted exact Search-and-Destroy quote has no reciprocal request graph";
		if (transactionCount == 0)
			return "";
		if (transactionCount != 2)
			return "terminal exact Search-and-Destroy planning ledger is ambiguous";
		HST_ResourceTransactionState money = FindUniqueTransaction(quote.m_sMoneyTransactionId);
		HST_ResourceTransactionState hr = FindUniqueTransaction(quote.m_sHRTransactionId);
		if (!ValidateTerminalPlanningTransaction(money, quote,
				HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost)
			|| !ValidateTerminalPlanningTransaction(hr, quote,
				HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost))
			return "terminal exact Search-and-Destroy planning ledger conflicts";
		return "";
	}

	protected string ValidateArchivedAggregate(
		HST_SupportRequestState request,
		HST_ForceSettlementTombstoneState tombstone)
	{
		if (!request || !tombstone)
			return "archived exact Search-and-Destroy request or tombstone is missing";
		HST_ForceSettlementArchiveService archive = new HST_ForceSettlementArchiveService();
		if (!archive.IsReplayTombstoneValid(tombstone)
			|| tombstone.m_iOperationContractVersion
				!= HST_OperationService.EXACT_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION)
			return "archived exact Search-and-Destroy tombstone conflicts";
		if (CountTombstonesForQuote(tombstone.m_sQuoteId) != 1
			|| CountRequestsByAnyIdentity(request) != 1)
			return "archived exact Search-and-Destroy identity is ambiguous";
		if (request.m_eType != HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY
			|| request.m_iOperationContractVersion
				!= HST_OperationService.EXACT_PLAYER_SEARCH_DESTROY_CONTRACT_VERSION
			|| !request.m_bPlayerRequested
			|| request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED
				&& request.m_eStatus != HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
			return "archived exact Search-and-Destroy request contract conflicts";
		if (request.m_sRequestId != tombstone.m_sSupportRequestId
			|| request.m_sOperationId != tombstone.m_sOperationId
			|| request.m_sQuoteId != tombstone.m_sQuoteId
			|| request.m_sManifestId != tombstone.m_sManifestId
			|| request.m_sMoneyTransactionId != tombstone.m_sMoneyTransactionId
			|| request.m_sHRTransactionId != tombstone.m_sHRTransactionId)
			return "archived exact Search-and-Destroy reciprocal links conflict";
		if (request.m_sFactionKey != tombstone.m_sFactionKey
			|| request.m_sCapabilityId != tombstone.m_sCapabilityId
			|| request.m_sAssetProfileId != tombstone.m_sAssetProfileId
			|| request.m_sSourceZoneId != tombstone.m_sSourceZoneId
			|| request.m_sTargetZoneId != tombstone.m_sTargetZoneId
			|| request.m_iMoneyCost != tombstone.m_iMoneyCost
			|| request.m_iHRCost != tombstone.m_iHRCost
			|| request.m_iPlannedInfantryCount != tombstone.m_iAcceptedMemberCount)
			return "archived exact Search-and-Destroy frozen authority conflicts";
		string requestFailure = ValidateArchivedRequestAuthority(request, tombstone);
		if (!requestFailure.IsEmpty())
			return requestFailure;
		HST_ForceSettlementTransactionTombstoneState hr = tombstone.FindTransaction(tombstone.m_sHRTransactionId);
		if (!hr || request.m_iRefundedHR != hr.m_iRefundedAmount)
			return "archived exact Search-and-Destroy refund evidence conflicts";
		if (HasLivePlanningGraph(request) || CountLiveTransactionsForRequest(request) > 0
			|| CountBatchesByPlanningIdentity(request) > 0 || CountGroupsByPlanningIdentity(request) > 0)
			return "archived exact Search-and-Destroy authority retains a live backlink";
		return "";
	}

	protected string ValidateArchivedRequestAuthority(
		HST_SupportRequestState request,
		HST_ForceSettlementTombstoneState tombstone)
	{
		if (!request || !tombstone)
			return "archived exact Search-and-Destroy request authority is missing";
		if (request.m_sResolutionKind != tombstone.m_sSettlementKind
			|| request.m_sCommandRequestId != tombstone.m_sConfirmationRequestId
			|| request.m_sSpawnResultId != "spawn_" + request.m_sRequestId)
			return "archived exact Search-and-Destroy terminal receipt is not reciprocal";
		if (request.m_sCompositionRequestId != tombstone.m_sManifestId
			|| request.m_sCompositionIntentId != "hst_search_destroy_regular"
			|| request.m_sCompositionTier != "exact"
			|| request.m_sPhysicalizationMode != HST_SupportRequestService.EXACT_PLAYER_SUPPORT_MODE)
			return "archived exact Search-and-Destroy composition identity conflicts";
		if (request.m_vSourcePosition != tombstone.m_vSourcePosition
			|| request.m_vTargetPosition != tombstone.m_vTargetPosition
			|| request.m_iRequestedAtSecond != tombstone.m_iAcceptedAtSecond)
			return "archived exact Search-and-Destroy assignment or acceptance time conflicts";
		if (request.m_iMoneyCost != tombstone.m_iMoneyCost
			|| request.m_iHRCost != tombstone.m_iHRCost
			|| request.m_iCompositionCost != tombstone.m_iMoneyCost
			|| request.m_iCompositionManpower != tombstone.m_iAcceptedMemberCount)
			return "archived exact Search-and-Destroy cost or roster authority conflicts";
		if (request.m_iCompositionVehicleCount != 0
			|| request.m_iCompositionArmedVehicleCount != 0
			|| request.m_iAttackCost != 0 || request.m_iSupportCost != 0)
			return "archived exact Search-and-Destroy request contains foreign force authority";
		if (request.m_iETASeconds != tombstone.m_iETASeconds
			|| request.m_iCooldownUntilSecond - request.m_iRequestedAtSecond
				!= tombstone.m_iCooldownSeconds)
			return "archived exact Search-and-Destroy schedule authority conflicts";
		if (request.m_bHelicopterStyle || request.m_bPhysicalStrikeSpawned
			|| request.m_bGarageVehicleConsumed
			|| !request.m_sSelectedGarageVehicleId.IsEmpty()
			|| !request.m_sSelectedGarageVehiclePrefab.IsEmpty())
			return "archived exact Search-and-Destroy request contains foreign asset authority";
		return "";
	}

	protected void NormalizeValidAggregate(
		HST_SupportRequestState request,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group)
	{
		if (!request || !operation)
			return;
		if (operation.m_eSettlementState
			== HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_SETTLED)
		{
			NormalizeValidTerminalRequest(request);
			ClearBatchProcessBindings(batch);
			ClearGroupProcessBindings(group, "retired");
			return;
		}

		bool savedLive = operation.m_ePositionAuthority
			== HST_EOperationPositionAuthority.HST_OPERATION_POSITION_LIVE;
		if (savedLive && group && !IsZeroVector(group.m_vPosition))
		{
			HST_StrategicMovementService movement = new HST_StrategicMovementService();
			movement.SyncRouteProgressFromPosition(operation, group.m_vPosition);
			HST_OperationService operations = new HST_OperationService();
			operations.ApplyExactPlayerSupportReturnToAssignment(
				operation,
				group,
				Math.Max(0, m_SaveData.m_iElapsedSeconds));
		}
		operation.m_eMaterializationState
			= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL;
		operation.m_ePositionAuthority
			= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_iArrivalConfirmationCount = 0;
		operation.m_iLastArrivalConfirmationSecond = 0;
		operation.m_sLastProjectionReason
			= "restored exact Search-and-Destroy with strategic position authority";
		operation.m_iRevision++;

		request.m_bPhysicalized = false;
		request.m_sRuntimeEntityId = "";
		if (operation.m_eDutyState == HST_EOperationDutyState.HST_OPERATION_DUTY_ON_STATION)
			request.m_sRuntimeStatus = "exact_virtual_on_station";
		else
			request.m_sRuntimeStatus = "exact_restore_virtual";
		ClearBatchProcessBindings(batch);
		ClearGroupProcessBindings(group, "support_virtual");
		if (group)
		{
			group.m_vPosition = operation.m_vStrategicPosition;
			group.m_vSourcePosition = operation.m_vStrategicPosition;
			group.m_iLifecycleRevision++;
		}
	}

	protected void NormalizeValidTerminalRequest(HST_SupportRequestState request)
	{
		if (!request)
			return;
		request.m_bPhysicalized = false;
		request.m_sRuntimeEntityId = "";
	}

	protected void ClearBatchProcessBindings(HST_ForceSpawnResultState batch)
	{
		if (!batch)
			return;
		batch.m_sNativeGroupId = "";
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (!slot)
				continue;
			slot.m_sEntityId = "";
			slot.m_sAssignedVehicleEntityId = "";
			slot.m_sNativeGroupId = "";
			slot.m_bAliveVerified = false;
			slot.m_bFactionVerified = false;
			slot.m_bGroupVerified = false;
			slot.m_bGameMasterVerified = false;
			slot.m_bProjectionVerified = false;
			slot.m_bSeatVerified = false;
		}
	}

	protected void ClearGroupProcessBindings(HST_ActiveGroupState group, string runtimeStatus)
	{
		if (!group)
			return;
		group.m_bSpawnedEntity = false;
		group.m_bSpawnAttempted = false;
		group.m_sRuntimeEntityId = "";
		group.m_iSpawnedAgentCount = 0;
		group.m_iAssignedWaypointCount = 0;
		group.m_sRuntimeStatus = runtimeStatus;
	}

	protected void QuarantineLinkedGraph(
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ForceSettlementTombstoneState tombstone,
		string reason)
	{
		if (reason.IsEmpty())
			reason = "schema-60 exact Search-and-Destroy authority conflict";
		foreach (HST_SupportRequestState candidateRequest : m_SaveData.m_aSupportRequests)
		{
			if (candidateRequest && GraphIdentityMatches(
				candidateRequest.m_sRequestId,
				candidateRequest.m_sQuoteId,
				candidateRequest.m_sManifestId,
				candidateRequest.m_sOperationId,
				request, quote, manifest, operation, batch, tombstone))
				QuarantineRequest(candidateRequest, reason);
		}
		foreach (HST_ForceQuoteState candidateQuote : m_SaveData.m_aForceQuotes)
		{
			if (candidateQuote && GraphIdentityMatches(
				candidateQuote.m_sSupportRequestId,
				candidateQuote.m_sQuoteId,
				candidateQuote.m_sManifestId,
				candidateQuote.m_sOperationId,
				request, quote, manifest, operation, batch, tombstone))
				QuarantineQuote(candidateQuote, reason);
		}
		foreach (HST_OperationRecordState candidateOperation : m_SaveData.m_aOperations)
		{
			if (candidateOperation && GraphIdentityMatches(
				candidateOperation.m_sSupportRequestId,
				candidateOperation.m_sQuoteId,
				candidateOperation.m_sManifestId,
				candidateOperation.m_sOperationId,
				request, quote, manifest, operation, batch, tombstone))
				QuarantineOperation(candidateOperation, reason);
		}
		foreach (HST_ForceSpawnResultState candidateBatch : m_SaveData.m_aForceSpawnResults)
		{
			if (candidateBatch && GraphIdentityMatches(
				candidateBatch.m_sRequestId,
				"",
				candidateBatch.m_sManifestId,
				candidateBatch.m_sOperationId,
				request, quote, manifest, operation, batch, tombstone))
				QuarantineBatch(candidateBatch, reason);
		}
		foreach (HST_ActiveGroupState candidateGroup : m_SaveData.m_aActiveGroups)
		{
			if (candidateGroup && GraphIdentityMatches(
				candidateGroup.m_sSupportRequestId,
				"",
				candidateGroup.m_sManifestId,
				candidateGroup.m_sOperationId,
				request, quote, manifest, operation, batch, tombstone))
				QuarantineGroup(candidateGroup, reason);
		}
		foreach (HST_ForceSettlementTombstoneState candidateTombstone : m_SaveData.m_aForceSettlementTombstones)
		{
			if (candidateTombstone && GraphIdentityMatches(
				candidateTombstone.m_sSupportRequestId,
				candidateTombstone.m_sQuoteId,
				candidateTombstone.m_sManifestId,
				candidateTombstone.m_sOperationId,
				request, quote, manifest, operation, batch, tombstone))
			{
				candidateTombstone.m_iOperationContractVersion = QUARANTINED_CONTRACT_VERSION;
				AppendUnique(m_aHandledTombstoneQuoteIds, candidateTombstone.m_sQuoteId);
			}
		}
		MarkHandled(request, quote, manifest, operation, batch, null, tombstone);
	}

	protected void QuarantineRequest(HST_SupportRequestState request, string reason)
	{
		request.m_iOperationContractVersion = QUARANTINED_CONTRACT_VERSION;
		request.m_eStatus = HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED;
		request.m_sRuntimeStatus = QUARANTINE_STATUS;
		request.m_sPhysicalizationMode = QUARANTINE_MODE;
		request.m_sRuntimeEntityId = "";
		request.m_bPhysicalized = false;
		request.m_sFailureReason = reason;
		AppendUnique(m_aHandledRequestIds, request.m_sRequestId);
	}

	protected void QuarantineQuote(HST_ForceQuoteState quote, string reason)
	{
		quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_CANCELLED;
		quote.m_sRejectionReason = reason;
		quote.m_iRevision = Math.Max(1, quote.m_iRevision) + 1;
		AppendUnique(m_aHandledQuoteIds, quote.m_sQuoteId);
	}

	protected void QuarantineOperation(HST_OperationRecordState operation, string reason)
	{
		operation.m_iContractVersion = QUARANTINED_CONTRACT_VERSION;
		operation.m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_SETTLED;
		operation.m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CLEAR;
		operation.m_eMaterializationState
			= HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_RETIRED;
		operation.m_ePositionAuthority
			= HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC;
		operation.m_sLastProjectionReason = reason;
		operation.m_iRevision = Math.Max(1, operation.m_iRevision) + 1;
		AppendUnique(m_aHandledOperationIds, operation.m_sOperationId);
	}

	protected void QuarantineBatch(HST_ForceSpawnResultState batch, string reason)
	{
		batch.m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_CANCELLED;
		batch.m_bCancelRequested = true;
		batch.m_sTerminalReason = reason;
		batch.m_sLastFailureReason = reason;
		ClearBatchProcessBindings(batch);
		AppendUnique(m_aHandledBatchIds, batch.m_sResultId);
	}

	protected void QuarantineGroup(HST_ActiveGroupState group, string reason)
	{
		ClearGroupProcessBindings(group, QUARANTINE_STATUS);
		group.m_sSpawnFallbackMode = QUARANTINE_MODE;
		group.m_sSpawnFailureReason = reason;
		AppendUnique(m_aHandledGroupIds, group.m_sGroupId);
	}

	protected int QuarantineOrphanClaimants(int restoredSchemaVersion)
	{
		int count;
		foreach (HST_SupportRequestState request : m_SaveData.m_aSupportRequests)
		{
			if (!IsSchema60PlayerSearchDestroyRequestClaimant(m_SaveData, request)
				|| m_aHandledRequestIds.Contains(request.m_sRequestId))
				continue;
			QuarantineLinkedGraph(request, FindUniqueQuote(request.m_sQuoteId),
				FindUniqueManifest(request.m_sManifestId), FindUniqueOperation(request.m_sOperationId),
				ResolveUniqueBatch(request, FindUniqueOperation(request.m_sOperationId)), null,
				"exact Search-and-Destroy request has no validated reciprocal authority graph");
			count++;
		}
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!IsSchema60PlayerSearchDestroyOperationClaimant(m_SaveData, operation)
				|| m_aHandledOperationIds.Contains(operation.m_sOperationId))
				continue;
			HST_SupportRequestState request = FindUniqueRequest(operation.m_sSupportRequestId);
			QuarantineLinkedGraph(request, FindUniqueQuote(operation.m_sQuoteId),
				FindUniqueManifest(operation.m_sManifestId), operation,
				FindUniqueBatch(operation.m_sSpawnResultId), null,
				"exact Search-and-Destroy operation has no validated reciprocal authority graph");
			count++;
		}
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!IsSchema60PlayerSearchDestroyManifestClaimant(m_SaveData, manifest)
				|| m_aHandledManifestIds.Contains(manifest.m_sManifestId))
				continue;
			QuarantineLinkedGraph(null, FindUniqueQuote(manifest.m_sQuoteId), manifest,
				FindUniqueOperation(manifest.m_sOperationId), null, null,
				"exact Search-and-Destroy manifest has no validated reciprocal authority graph");
			count++;
		}
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!IsSchema60PlayerSearchDestroyBatchClaimant(m_SaveData, batch)
				|| m_aHandledBatchIds.Contains(batch.m_sResultId))
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(batch.m_sOperationId);
			HST_SupportRequestState request = FindUniqueRequest(batch.m_sRequestId);
			QuarantineLinkedGraph(request, null, FindUniqueManifest(batch.m_sManifestId),
				operation, batch, null,
				"exact Search-and-Destroy batch has no validated reciprocal authority graph");
			count++;
		}
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!IsSchema60PlayerSearchDestroyGroupClaimant(m_SaveData, group)
				|| m_aHandledGroupIds.Contains(group.m_sGroupId))
				continue;
			HST_OperationRecordState operation = FindUniqueOperation(group.m_sOperationId);
			HST_SupportRequestState request = FindUniqueRequest(group.m_sSupportRequestId);
			QuarantineLinkedGraph(request, null, FindUniqueManifest(group.m_sManifestId),
				operation, FindUniqueBatch(group.m_sSpawnResultId), null,
				"exact Search-and-Destroy group has no validated reciprocal authority graph");
			count++;
		}
		return count;
	}

	protected void MarkHandled(
		HST_SupportRequestState request,
		HST_ForceQuoteState quote,
		HST_ForceManifestState manifest,
		HST_OperationRecordState operation,
		HST_ForceSpawnResultState batch,
		HST_ActiveGroupState group,
		HST_ForceSettlementTombstoneState tombstone)
	{
		if (request)
			AppendUnique(m_aHandledRequestIds, request.m_sRequestId);
		if (quote)
			AppendUnique(m_aHandledQuoteIds, quote.m_sQuoteId);
		if (manifest)
			AppendUnique(m_aHandledManifestIds, manifest.m_sManifestId);
		if (operation)
			AppendUnique(m_aHandledOperationIds, operation.m_sOperationId);
		if (batch)
			AppendUnique(m_aHandledBatchIds, batch.m_sResultId);
		if (group)
			AppendUnique(m_aHandledGroupIds, group.m_sGroupId);
		if (tombstone)
			AppendUnique(m_aHandledTombstoneQuoteIds, tombstone.m_sQuoteId);
	}

	protected bool GraphIdentityMatches(
		string requestId,
		string quoteId,
		string manifestId,
		string operationId,
		HST_SupportRequestState seedRequest,
		HST_ForceQuoteState seedQuote,
		HST_ForceManifestState seedManifest,
		HST_OperationRecordState seedOperation,
		HST_ForceSpawnResultState seedBatch,
		HST_ForceSettlementTombstoneState seedTombstone)
	{
		if (seedRequest && (NonEmptyMatch(requestId, seedRequest.m_sRequestId)
			|| NonEmptyMatch(quoteId, seedRequest.m_sQuoteId)
			|| NonEmptyMatch(manifestId, seedRequest.m_sManifestId)
			|| NonEmptyMatch(operationId, seedRequest.m_sOperationId)))
			return true;
		if (seedQuote && (NonEmptyMatch(requestId, seedQuote.m_sSupportRequestId)
			|| NonEmptyMatch(quoteId, seedQuote.m_sQuoteId)
			|| NonEmptyMatch(manifestId, seedQuote.m_sManifestId)
			|| NonEmptyMatch(operationId, seedQuote.m_sOperationId)))
			return true;
		if (seedManifest && (NonEmptyMatch(quoteId, seedManifest.m_sQuoteId)
			|| NonEmptyMatch(manifestId, seedManifest.m_sManifestId)
			|| NonEmptyMatch(operationId, seedManifest.m_sOperationId)))
			return true;
		if (seedOperation && (NonEmptyMatch(requestId, seedOperation.m_sSupportRequestId)
			|| NonEmptyMatch(quoteId, seedOperation.m_sQuoteId)
			|| NonEmptyMatch(manifestId, seedOperation.m_sManifestId)
			|| NonEmptyMatch(operationId, seedOperation.m_sOperationId)))
			return true;
		if (seedBatch && (NonEmptyMatch(requestId, seedBatch.m_sRequestId)
			|| NonEmptyMatch(manifestId, seedBatch.m_sManifestId)
			|| NonEmptyMatch(operationId, seedBatch.m_sOperationId)))
			return true;
		return seedTombstone && (NonEmptyMatch(requestId, seedTombstone.m_sSupportRequestId)
			|| NonEmptyMatch(quoteId, seedTombstone.m_sQuoteId)
			|| NonEmptyMatch(manifestId, seedTombstone.m_sManifestId)
			|| NonEmptyMatch(operationId, seedTombstone.m_sOperationId));
	}

	protected bool NonEmptyMatch(string left, string right)
	{
		return !left.IsEmpty() && !right.IsEmpty() && left == right;
	}

	protected void AppendUnique(array<string> values, string value)
	{
		if (!value.IsEmpty() && !values.Contains(value))
			values.Insert(value);
	}

	protected bool CountWithin(int value, int maximum)
	{
		return value >= 0 && value <= maximum;
	}

	protected bool TransactionSettlementIsConsistent(HST_ResourceTransactionState transaction)
	{
		if (!transaction || transaction.m_iAmount < 0 || transaction.m_iRefundedAmount < 0
			|| transaction.m_iRefundedAmount > transaction.m_iAmount)
			return false;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED)
			return transaction.m_iRefundedAmount == 0;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED)
			return transaction.m_iRefundedAmount > 0
				&& transaction.m_iRefundedAmount < transaction.m_iAmount;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED)
			return transaction.m_iRefundedAmount == transaction.m_iAmount;
		return false;
	}

	protected bool ValidateTerminalPlanningTransaction(
		HST_ResourceTransactionState transaction,
		HST_ForceQuoteState quote,
		string resourceType,
		int amount)
	{
		if (!transaction || !quote || transaction.m_sOperationId != quote.m_sOperationId
			|| transaction.m_sQuoteId != quote.m_sQuoteId
			|| transaction.m_sManifestId != quote.m_sManifestId
			|| transaction.m_sActorIdentityId != quote.m_sActorIdentityId
			|| transaction.m_sResourceType != resourceType || transaction.m_iAmount != amount)
			return false;
		return (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_CANCELLED
			|| transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED)
			&& transaction.m_iRefundedAmount == transaction.m_iAmount;
	}

	protected HST_CampaignState BuildValidationState()
	{
		HST_CampaignState state = new HST_CampaignState();
		state.m_iElapsedSeconds = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		state.m_aOperations.Clear();
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (operation)
				state.m_aOperations.Insert(operation);
		}
		return state;
	}

	protected HST_SupportRequestState FindUniqueRequest(string requestId)
	{
		HST_SupportRequestState match;
		if (requestId.IsEmpty())
			return null;
		foreach (HST_SupportRequestState request : m_SaveData.m_aSupportRequests)
		{
			if (!request || request.m_sRequestId != requestId)
				continue;
			if (match)
				return null;
			match = request;
		}
		return match;
	}

	protected HST_ForceQuoteState FindUniqueQuote(string quoteId)
	{
		HST_ForceQuoteState match;
		if (quoteId.IsEmpty())
			return null;
		foreach (HST_ForceQuoteState quote : m_SaveData.m_aForceQuotes)
		{
			if (!quote || quote.m_sQuoteId != quoteId)
				continue;
			if (match)
				return null;
			match = quote;
		}
		return match;
	}

	protected HST_ForceManifestState FindUniqueManifest(string manifestId)
	{
		HST_ForceManifestState match;
		if (manifestId.IsEmpty())
			return null;
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!manifest || manifest.m_sManifestId != manifestId)
				continue;
			if (match)
				return null;
			match = manifest;
		}
		return match;
	}

	protected HST_OperationRecordState FindUniqueOperation(string operationId)
	{
		HST_OperationRecordState match;
		if (operationId.IsEmpty())
			return null;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation || operation.m_sOperationId != operationId)
				continue;
			if (match)
				return null;
			match = operation;
		}
		return match;
	}

	protected HST_ForceSpawnResultState FindUniqueBatch(string resultId)
	{
		HST_ForceSpawnResultState match;
		if (resultId.IsEmpty())
			return null;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch || batch.m_sResultId != resultId)
				continue;
			if (match)
				return null;
			match = batch;
		}
		return match;
	}

	protected HST_ActiveGroupState FindUniqueGroup(string groupId)
	{
		HST_ActiveGroupState match;
		if (groupId.IsEmpty())
			return null;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!group || group.m_sGroupId != groupId)
				continue;
			if (match)
				return null;
			match = group;
		}
		return match;
	}

	protected HST_ResourceTransactionState FindUniqueTransaction(string transactionId)
	{
		HST_ResourceTransactionState match;
		if (transactionId.IsEmpty())
			return null;
		foreach (HST_ResourceTransactionState transaction : m_SaveData.m_aResourceTransactions)
		{
			if (!transaction || transaction.m_sTransactionId != transactionId)
				continue;
			if (match)
				return null;
			match = transaction;
		}
		return match;
	}

	protected HST_ForceSettlementTombstoneState FindUniqueTombstoneForRequest(
		HST_SupportRequestState request)
	{
		HST_ForceSettlementTombstoneState match;
		if (!request)
			return null;
		foreach (HST_ForceSettlementTombstoneState tombstone : m_SaveData.m_aForceSettlementTombstones)
		{
			if (!tombstone || (tombstone.m_sQuoteId != request.m_sQuoteId
				&& tombstone.m_sSupportRequestId != request.m_sRequestId))
				continue;
			if (match)
				return null;
			match = tombstone;
		}
		return match;
	}

	protected HST_ForceSpawnResultState ResolveUniqueBatch(
		HST_SupportRequestState request,
		HST_OperationRecordState operation)
	{
		string resultId;
		if (operation)
			resultId = operation.m_sSpawnResultId;
		if (resultId.IsEmpty() && request)
			resultId = request.m_sSpawnResultId;
		HST_ForceSpawnResultState batch = FindUniqueBatch(resultId);
		if (!operation || operation.m_sSpawnResultId.IsEmpty())
			return batch;
		return batch;
	}

	protected HST_ActiveGroupState ResolveUniqueGroup(
		HST_SupportRequestState request,
		HST_OperationRecordState operation)
	{
		string groupId;
		if (operation)
			groupId = operation.m_sGroupId;
		if (groupId.IsEmpty() && request)
			groupId = request.m_sGroupId;
		return FindUniqueGroup(groupId);
	}

	protected bool HasLivePlanningGraph(HST_SupportRequestState request)
	{
		if (!request)
			return false;
		return FindUniqueQuote(request.m_sQuoteId) != null
			|| FindUniqueManifest(request.m_sManifestId) != null
			|| FindUniqueOperation(request.m_sOperationId) != null;
	}

	protected int CountRequestsByAnyIdentity(HST_SupportRequestState expected)
	{
		int count;
		foreach (HST_SupportRequestState request : m_SaveData.m_aSupportRequests)
		{
			if (!request || !expected)
				continue;
			if (NonEmptyMatch(request.m_sRequestId, expected.m_sRequestId)
				|| NonEmptyMatch(request.m_sQuoteId, expected.m_sQuoteId)
				|| NonEmptyMatch(request.m_sManifestId, expected.m_sManifestId)
				|| NonEmptyMatch(request.m_sOperationId, expected.m_sOperationId))
				count++;
		}
		return count;
	}

	protected int CountQuotesByAnyIdentity(
		HST_SupportRequestState request,
		HST_ForceQuoteState expected)
	{
		int count;
		foreach (HST_ForceQuoteState quote : m_SaveData.m_aForceQuotes)
		{
			if (!quote || !request || !expected)
				continue;
			if (NonEmptyMatch(quote.m_sQuoteId, expected.m_sQuoteId)
				|| NonEmptyMatch(quote.m_sSupportRequestId, request.m_sRequestId)
				|| NonEmptyMatch(quote.m_sManifestId, request.m_sManifestId)
				|| NonEmptyMatch(quote.m_sOperationId, request.m_sOperationId))
				count++;
		}
		return count;
	}

	protected int CountManifestsByAnyIdentity(
		HST_SupportRequestState request,
		HST_ForceManifestState expected)
	{
		int count;
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (!manifest || !request || !expected)
				continue;
			if (NonEmptyMatch(manifest.m_sManifestId, expected.m_sManifestId)
				|| NonEmptyMatch(manifest.m_sQuoteId, request.m_sQuoteId)
				|| NonEmptyMatch(manifest.m_sOperationId, request.m_sOperationId))
				count++;
		}
		return count;
	}

	protected int CountOperationsByAnyIdentity(
		HST_SupportRequestState request,
		HST_OperationRecordState expected)
	{
		int count;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (!operation || !request || !expected)
				continue;
			if (NonEmptyMatch(operation.m_sOperationId, expected.m_sOperationId)
				|| NonEmptyMatch(operation.m_sSupportRequestId, request.m_sRequestId)
				|| NonEmptyMatch(operation.m_sQuoteId, request.m_sQuoteId)
				|| NonEmptyMatch(operation.m_sManifestId, request.m_sManifestId))
				count++;
		}
		return count;
	}

	protected int CountBatchesByAnyIdentity(
		HST_SupportRequestState request,
		HST_ForceSpawnResultState expected)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (!batch || !request || !expected)
				continue;
			if (NonEmptyMatch(batch.m_sResultId, expected.m_sResultId)
				|| NonEmptyMatch(batch.m_sRequestId, request.m_sRequestId)
				|| NonEmptyMatch(batch.m_sManifestId, request.m_sManifestId)
				|| NonEmptyMatch(batch.m_sOperationId, request.m_sOperationId)
				|| NonEmptyMatch(batch.m_sProjectionId, expected.m_sProjectionId))
				count++;
		}
		return count;
	}

	protected int CountGroupsByAnyIdentity(
		HST_SupportRequestState request,
		HST_ActiveGroupState expected)
	{
		int count;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (!group || !request || !expected)
				continue;
			if (NonEmptyMatch(group.m_sGroupId, expected.m_sGroupId)
				|| NonEmptyMatch(group.m_sSupportRequestId, request.m_sRequestId)
				|| NonEmptyMatch(group.m_sManifestId, request.m_sManifestId)
				|| NonEmptyMatch(group.m_sOperationId, request.m_sOperationId)
				|| NonEmptyMatch(group.m_sProjectionId, expected.m_sProjectionId))
				count++;
		}
		return count;
	}

	protected int CountBatchesByPlanningIdentity(HST_SupportRequestState request)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (batch && request && (NonEmptyMatch(batch.m_sRequestId, request.m_sRequestId)
				|| NonEmptyMatch(batch.m_sManifestId, request.m_sManifestId)
				|| NonEmptyMatch(batch.m_sOperationId, request.m_sOperationId)))
				count++;
		}
		return count;
	}

	protected int CountGroupsByPlanningIdentity(HST_SupportRequestState request)
	{
		int count;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (group && request && (NonEmptyMatch(group.m_sSupportRequestId, request.m_sRequestId)
				|| NonEmptyMatch(group.m_sManifestId, request.m_sManifestId)
				|| NonEmptyMatch(group.m_sOperationId, request.m_sOperationId)))
				count++;
		}
		return count;
	}

	protected int CountTransactionsByPlanningIdentity(HST_ForceQuoteState quote)
	{
		int count;
		foreach (HST_ResourceTransactionState transaction : m_SaveData.m_aResourceTransactions)
		{
			if (transaction && quote && (NonEmptyMatch(transaction.m_sQuoteId, quote.m_sQuoteId)
				|| NonEmptyMatch(transaction.m_sManifestId, quote.m_sManifestId)
				|| NonEmptyMatch(transaction.m_sOperationId, quote.m_sOperationId)
				|| NonEmptyMatch(transaction.m_sTransactionId, quote.m_sMoneyTransactionId)
				|| NonEmptyMatch(transaction.m_sTransactionId, quote.m_sHRTransactionId)))
				count++;
		}
		return count;
	}

	protected int CountLiveTransactionsForRequest(HST_SupportRequestState request)
	{
		int count;
		foreach (HST_ResourceTransactionState transaction : m_SaveData.m_aResourceTransactions)
		{
			if (transaction && request && (NonEmptyMatch(transaction.m_sQuoteId, request.m_sQuoteId)
				|| NonEmptyMatch(transaction.m_sManifestId, request.m_sManifestId)
				|| NonEmptyMatch(transaction.m_sOperationId, request.m_sOperationId)
				|| NonEmptyMatch(transaction.m_sTransactionId, request.m_sMoneyTransactionId)
				|| NonEmptyMatch(transaction.m_sTransactionId, request.m_sHRTransactionId)))
				count++;
		}
		return count;
	}

	protected int CountStandaloneQuotes(HST_ForceQuoteState expected)
	{
		int count;
		foreach (HST_ForceQuoteState quote : m_SaveData.m_aForceQuotes)
		{
			if (quote && expected && (NonEmptyMatch(quote.m_sQuoteId, expected.m_sQuoteId)
				|| NonEmptyMatch(quote.m_sManifestId, expected.m_sManifestId)
				|| NonEmptyMatch(quote.m_sOperationId, expected.m_sOperationId)
				|| NonEmptyMatch(quote.m_sSupportRequestId, expected.m_sSupportRequestId)))
				count++;
		}
		return count;
	}

	protected int CountStandaloneManifests(
		HST_ForceQuoteState quote,
		HST_ForceManifestState expected)
	{
		int count;
		foreach (HST_ForceManifestState manifest : m_SaveData.m_aForceManifests)
		{
			if (manifest && quote && expected && (NonEmptyMatch(manifest.m_sManifestId, expected.m_sManifestId)
				|| NonEmptyMatch(manifest.m_sQuoteId, quote.m_sQuoteId)
				|| NonEmptyMatch(manifest.m_sOperationId, quote.m_sOperationId)))
				count++;
		}
		return count;
	}

	protected int CountRequestsForQuote(HST_ForceQuoteState quote)
	{
		int count;
		foreach (HST_SupportRequestState request : m_SaveData.m_aSupportRequests)
		{
			if (request && quote && (NonEmptyMatch(request.m_sQuoteId, quote.m_sQuoteId)
				|| NonEmptyMatch(request.m_sRequestId, quote.m_sSupportRequestId)
				|| NonEmptyMatch(request.m_sOperationId, quote.m_sOperationId)))
				count++;
		}
		return count;
	}

	protected int CountOperationsForQuote(HST_ForceQuoteState quote)
	{
		int count;
		foreach (HST_OperationRecordState operation : m_SaveData.m_aOperations)
		{
			if (operation && quote && (NonEmptyMatch(operation.m_sQuoteId, quote.m_sQuoteId)
				|| NonEmptyMatch(operation.m_sOperationId, quote.m_sOperationId)
				|| NonEmptyMatch(operation.m_sManifestId, quote.m_sManifestId)))
				count++;
		}
		return count;
	}

	protected int CountBatchesForQuote(HST_ForceQuoteState quote)
	{
		int count;
		foreach (HST_ForceSpawnResultState batch : m_SaveData.m_aForceSpawnResults)
		{
			if (batch && quote && (NonEmptyMatch(batch.m_sOperationId, quote.m_sOperationId)
				|| NonEmptyMatch(batch.m_sManifestId, quote.m_sManifestId)
				|| NonEmptyMatch(batch.m_sRequestId, quote.m_sSupportRequestId)))
				count++;
		}
		return count;
	}

	protected int CountGroupsForQuote(HST_ForceQuoteState quote)
	{
		int count;
		foreach (HST_ActiveGroupState group : m_SaveData.m_aActiveGroups)
		{
			if (group && quote && (NonEmptyMatch(group.m_sOperationId, quote.m_sOperationId)
				|| NonEmptyMatch(group.m_sManifestId, quote.m_sManifestId)
				|| NonEmptyMatch(group.m_sSupportRequestId, quote.m_sSupportRequestId)))
				count++;
		}
		return count;
	}

	protected int CountTombstonesForQuote(string quoteId)
	{
		int count;
		foreach (HST_ForceSettlementTombstoneState tombstone : m_SaveData.m_aForceSettlementTombstones)
		{
			if (tombstone && tombstone.m_sQuoteId == quoteId)
				count++;
		}
		return count;
	}

	protected int CountBatchSlots(
		HST_ForceSpawnResultState batch,
		string slotId,
		string slotKind)
	{
		int count;
		foreach (HST_ForceSpawnSlotResultState slot : batch.m_aSlotResults)
		{
			if (slot && slot.m_sSlotId == slotId && slot.m_sSlotKind == slotKind)
				count++;
		}
		return count;
	}

	protected bool PositionsMatch(vector first, vector second, float toleranceMeters)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz <= toleranceMeters * toleranceMeters;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected bool HasEvent(string eventId)
	{
		foreach (HST_CampaignEventState eventState : m_SaveData.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
	}

	protected void RecordLegacyMigration(int legacyCount, int conflictCount)
	{
		if (HasEvent(MIGRATION_EVENT_ID))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = MIGRATION_EVENT_ID;
		eventState.m_sCategory = "migration";
		eventState.m_sAggregateType = "player_search_destroy_authority";
		eventState.m_sAggregateId = "schema60";
		eventState.m_sTransition = "legacy_search_destroy_preserved";
		eventState.m_sReason = string.Format(
			"preserved %1 historical Search-and-Destroy requests on contract zero and quarantined %2 unsupported strong claims without creating an exact operation, roster, casualty, arrival, cost, settlement, or refund",
			legacyCount,
			conflictCount);
		eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(eventState);
	}

	protected void RecordNormalizationConflict(int validatedCount, int conflictCount)
	{
		if (conflictCount <= 0 || HasEvent(CONFLICT_EVENT_ID))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = CONFLICT_EVENT_ID;
		eventState.m_sCategory = "normalization";
		eventState.m_sAggregateType = "player_search_destroy_authority";
		eventState.m_sAggregateId = "schema60";
		eventState.m_sTransition = "malformed_exact_search_destroy_quarantined";
		eventState.m_sReason = string.Format(
			"validated %1 exact Search-and-Destroy graphs and quarantined %2 incomplete, duplicate, malformed, stale, or non-reciprocal claims while preserving all economic evidence and creating no refund",
			validatedCount,
			conflictCount);
		eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(eventState);
	}
}
