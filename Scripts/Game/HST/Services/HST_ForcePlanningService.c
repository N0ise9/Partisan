class HST_ForcePlanningService
{
	static const string QUOTE_KIND_GARRISON = "garrison_recruitment";
	static const string QUOTE_KIND_PLAYER_SUPPORT_QRF = "player_support_qrf";
	static const string GARRISON_POLICY_ID = "garrison_exact_all_or_nothing_1";
	static const string SUPPORT_QRF_POLICY_ID = "support_qrf_exact_infantry_1";
	static const int GARRISON_QUOTE_LIFETIME_SECONDS = 120;
	static const int SUPPORT_QRF_QUOTE_LIFETIME_SECONDS = 120;
	static const int SUPPORT_QRF_MONEY_COST = 250;
	static const int SUPPORT_QRF_ETA_SECONDS = 120;
	static const int SUPPORT_QRF_COOLDOWN_SECONDS = 600;
	static const string SUPPORT_QRF_CAPABILITY_ID = "qrf";
	static const string SUPPORT_QRF_ASSET_PROFILE_ID = "fia_qrf_reserve_alpha";
	static const int MAX_RECRUIT_MEMBER_COUNT = 32;
	static const int MAX_OPEN_GARRISON_QUOTES = 64;
	static const int TERMINAL_QUOTE_RETENTION_SECONDS = 600;
	protected ref HST_ForceCatalogService m_Catalog = new HST_ForceCatalogService();
	protected ref HST_ForcePlanningIntegrityService m_Integrity = new HST_ForcePlanningIntegrityService();
	protected ref HST_CampaignEventLogService m_EventLog;

	void SetEventLogService(HST_CampaignEventLogService eventLog)
	{
		m_EventLog = eventLog;
	}

	HST_ForceQuoteResult IssuePlayerSupportQuote(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string actorIdentityId,
		HST_ESupportRequestType supportType,
		string targetZoneId,
		vector targetPosition,
		string commandRequestId,
		bool validateResources = true)
	{
		HST_ForceQuoteResult result = new HST_ForceQuoteResult();
		if (!state || !preset || !m_Catalog || !m_Integrity)
		{
			result.m_sFailureReason = "planning service not ready";
			return result;
		}
		if (actorIdentityId.IsEmpty())
		{
			result.m_sFailureReason = "actor identity missing";
			return result;
		}
		if (supportType != HST_ESupportRequestType.HST_SUPPORT_QRF)
		{
			result.m_sFailureReason = "exact paid support currently accepts QRF only";
			return result;
		}
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE || !state.m_bHQDeployed)
		{
			result.m_sFailureReason = "paid QRF support requires an active campaign and deployed HQ";
			return result;
		}

		result.m_bStateChanged = ExpireIssuedQuotes(state);
		if (PrunePlanningRecords(state))
			result.m_bStateChanged = true;

		HST_ZoneState targetZone = state.FindZone(targetZoneId);
		if (!targetZone)
		{
			result.m_sFailureReason = "target zone not found";
			return result;
		}
		if (IsZeroPosition(targetPosition))
			targetPosition = targetZone.m_vPosition;

		if (!commandRequestId.IsEmpty())
		{
			HST_ForceQuoteState existingQuote = FindQuoteByCommandRequestId(state, commandRequestId);
			if (existingQuote)
			{
				if (existingQuote.m_sActorIdentityId != actorIdentityId
					|| existingQuote.m_sQuoteKind != QUOTE_KIND_PLAYER_SUPPORT_QRF
					|| existingQuote.m_eSupportType != supportType
					|| existingQuote.m_sTargetZoneId != targetZoneId
					|| !PositionsMatch(existingQuote.m_vTargetPosition, targetPosition))
				{
					result.m_sFailureReason = "quote request id conflict";
					return result;
				}

				HST_ForceManifestState existingManifest = state.FindForceManifest(existingQuote.m_sManifestId);
				if (existingQuote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED
					&& existingQuote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
				{
					result.m_sFailureReason = "existing quote request is terminal";
					return result;
				}
				string replayIntegrityFailure;
				bool requireCurrentCatalog = existingQuote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED;
				if (!m_Integrity.ValidateFrozenPlayerSupportQuote(existingManifest, existingQuote, requireCurrentCatalog, replayIntegrityFailure))
				{
					result.m_sFailureReason = "existing support quote integrity conflict: " + replayIntegrityFailure;
					return result;
				}
				if (existingQuote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
				{
					HST_ResourceTransactionState existingMoney = state.FindResourceTransaction(existingQuote.m_sMoneyTransactionId);
					HST_ResourceTransactionState existingHR = state.FindResourceTransaction(existingQuote.m_sHRTransactionId);
					int requestCount;
					HST_SupportRequestState existingRequest = FindUniquePlayerSupportRequestForQuote(state, existingQuote, requestCount);
					if (!m_Integrity.TransactionMatchesAcceptedPlayerSupportQuote(existingMoney, existingQuote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, existingQuote.m_iMoneyCost)
						|| !m_Integrity.TransactionMatchesAcceptedPlayerSupportQuote(existingHR, existingQuote, HST_ResourceLedgerService.RESOURCE_HR, existingQuote.m_iHRCost)
						|| requestCount != 1 || !PlayerSupportRequestMatchesQuote(existingRequest, existingQuote, existingManifest))
					{
						result.m_sFailureReason = "existing accepted support quote authority conflict";
						return result;
					}
				}
				result.m_bSuccess = true;
				result.m_Quote = existingQuote;
				result.m_Manifest = existingManifest;
				return result;
			}
		}

		if (CountOpenPlayerSupportQuotes(state) >= MAX_OPEN_GARRISON_QUOTES && !FindIssuedPlayerSupportQuote(state, actorIdentityId, supportType))
		{
			result.m_sFailureReason = "open player support quote capacity reached";
			return result;
		}
		string blockingReason;
		if (HasBlockingPlayerSupportRequest(state, supportType, blockingReason))
		{
			result.m_sFailureReason = blockingReason;
			return result;
		}

		string factionKey = preset.m_sResistanceFactionKey;
		if (factionKey.IsEmpty())
			factionKey = "FIA";
		if (!state.FindFactionPool(factionKey))
		{
			result.m_sFailureReason = "faction pool unavailable";
			return result;
		}
		HST_ZoneState sourceZone = ResolvePlayerSupportSourceZone(state, factionKey, targetZone);
		if (!sourceZone)
		{
			result.m_sFailureReason = "no resistance-owned QRF source zone is available";
			return result;
		}

		HST_ForceCatalogValidationResult catalogValidation = m_Catalog.ValidateFactionCatalog(factionKey, validateResources);
		if (!catalogValidation || !catalogValidation.m_bValid)
		{
			result.m_sFailureReason = "force group catalog invalid";
			if (catalogValidation && !catalogValidation.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = catalogValidation.m_sFailureReason;
			return result;
		}

		int planningSeed = m_Integrity.BuildDeterministicSeed(state, actorIdentityId + "|" + commandRequestId + "|qrf", targetZoneId);
		HST_ForceGroupCatalogEntry catalogGroup = m_Integrity.SelectPlayerSupportGroup(m_Catalog.BuildGroupCatalog(factionKey), planningSeed, state.m_iWarLevel);
		if (!catalogGroup || catalogGroup.m_aMemberSlots.Count() <= 0)
		{
			result.m_sFailureReason = "deterministic exact QRF group selection failed";
			return result;
		}
		int exactMemberCount = catalogGroup.m_aMemberSlots.Count();
		if (state.m_iFactionMoney < SUPPORT_QRF_MONEY_COST)
		{
			result.m_sFailureReason = string.Format("need $%1, have $%2", SUPPORT_QRF_MONEY_COST, state.m_iFactionMoney);
			return result;
		}
		if (state.m_iHR < exactMemberCount)
		{
			result.m_sFailureReason = string.Format("need %1 HR, have %2", exactMemberCount, state.m_iHR);
			return result;
		}

		if (commandRequestId.IsEmpty())
		{
			commandRequestId = HST_StableIdService.NextId(state, "support_quote_command");
			result.m_bStateChanged = true;
		}
		string quoteId = HST_StableIdService.NextId(state, "support_quote");
		string manifestId = HST_StableIdService.NextId(state, "manifest");
		string supportRequestId = "support_" + quoteId;
		string operationId = HST_StableIdService.BuildOperationId("support", supportRequestId);
		result.m_bStateChanged = true;

		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = manifestId;
		manifest.m_sOperationId = operationId;
		manifest.m_sQuoteId = quoteId;
		manifest.m_sCommandRequestId = commandRequestId;
		manifest.m_sForceKind = "player_support";
		manifest.m_sFactionRole = "resistance";
		manifest.m_sFactionKey = factionKey;
		manifest.m_sIntentId = "hst_qrf_regular";
		manifest.m_sSourceZoneId = sourceZone.m_sZoneId;
		manifest.m_sTargetZoneId = targetZoneId;
		manifest.m_sGroupPrefab = catalogGroup.m_sExecutionPrefab;
		manifest.m_sCatalogVersion = HST_ForceCatalogService.CATALOG_VERSION;
		manifest.m_sPolicyId = SUPPORT_QRF_POLICY_ID;
		manifest.m_iRequestedMemberCount = exactMemberCount;
		manifest.m_iAcceptedMemberCount = exactMemberCount;
		manifest.m_iMoneyCost = SUPPORT_QRF_MONEY_COST;
		manifest.m_iHRCost = exactMemberCount;
		manifest.m_iDeterministicSeed = planningSeed;
		manifest.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		manifest.m_bFrozen = true;

		HST_ForceManifestGroupState groupElement = new HST_ForceManifestGroupState();
		groupElement.m_sElementId = manifestId + "_group_1";
		groupElement.m_sCatalogEntryId = catalogGroup.m_sEntryId;
		groupElement.m_sPrefab = catalogGroup.m_sExecutionPrefab;
		groupElement.m_sRole = catalogGroup.m_sRole;
		groupElement.m_iOrdinal = 0;
		groupElement.m_iExpectedMemberCount = exactMemberCount;
		groupElement.m_bRequired = true;
		manifest.m_aGroups.Insert(groupElement);

		for (int memberIndex = 0; memberIndex < catalogGroup.m_aMemberSlots.Count(); memberIndex++)
		{
			HST_ForceGroupCatalogSlot catalogSlot = catalogGroup.m_aMemberSlots[memberIndex];
			if (!catalogSlot)
			{
				result.m_sFailureReason = "selected QRF group contains a missing catalog slot";
				return result;
			}
			HST_ForceManifestMemberState member = new HST_ForceManifestMemberState();
			member.m_sSlotId = string.Format("%1_member_%2", manifestId, memberIndex + 1);
			member.m_sCatalogSlotId = catalogGroup.m_sEntryId + "/" + catalogSlot.m_sSlotId;
			member.m_sGroupElementId = groupElement.m_sElementId;
			member.m_sPrefab = catalogSlot.m_sPrefab;
			member.m_sRole = catalogSlot.m_sRole;
			member.m_iOrdinal = memberIndex;
			member.m_iMoneyCost = 0;
			member.m_iHRCost = 1;
			member.m_iEquipmentCost = 0;
			member.m_bRequired = true;
			manifest.m_aMembers.Insert(member);
		}

		manifest.m_sManifestHash = m_Integrity.BuildManifestHash(manifest);
		if (manifest.m_sManifestHash.IsEmpty())
		{
			result.m_sFailureReason = "manifest hash failed";
			return result;
		}

		CancelSupersededPlayerSupportQuotes(state, actorIdentityId, supportType, commandRequestId);
		HST_ForceQuoteState quote = new HST_ForceQuoteState();
		quote.m_sQuoteId = quoteId;
		quote.m_sManifestId = manifestId;
		quote.m_sManifestHash = manifest.m_sManifestHash;
		quote.m_sOperationId = operationId;
		quote.m_sCommandRequestId = commandRequestId;
		quote.m_sActorIdentityId = actorIdentityId;
		quote.m_sQuoteKind = QUOTE_KIND_PLAYER_SUPPORT_QRF;
		quote.m_sSupportRequestId = supportRequestId;
		quote.m_sCapabilityId = SUPPORT_QRF_CAPABILITY_ID;
		quote.m_sAssetProfileId = SUPPORT_QRF_ASSET_PROFILE_ID;
		quote.m_sFactionKey = factionKey;
		quote.m_sSourceZoneId = sourceZone.m_sZoneId;
		quote.m_sTargetZoneId = targetZoneId;
		quote.m_sCatalogVersion = manifest.m_sCatalogVersion;
		quote.m_sPolicyId = manifest.m_sPolicyId;
		quote.m_sMoneyTransactionId = HST_StableIdService.BuildTransactionId(operationId, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY);
		quote.m_sHRTransactionId = HST_StableIdService.BuildTransactionId(operationId, HST_ResourceLedgerService.RESOURCE_HR);
		quote.m_vSourcePosition = sourceZone.m_vPosition;
		quote.m_vTargetPosition = targetPosition;
		quote.m_eSupportType = supportType;
		quote.m_iRequestedMemberCount = exactMemberCount;
		quote.m_iAcceptedMemberCount = exactMemberCount;
		quote.m_iMoneyCost = manifest.m_iMoneyCost;
		quote.m_iHRCost = manifest.m_iHRCost;
		quote.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		quote.m_iExpiresAtSecond = state.m_iElapsedSeconds + SUPPORT_QRF_QUOTE_LIFETIME_SECONDS;
		quote.m_iETASeconds = SUPPORT_QRF_ETA_SECONDS;
		quote.m_iCooldownSeconds = SUPPORT_QRF_COOLDOWN_SECONDS;
		quote.m_iExpectedWarLevel = Math.Max(1, state.m_iWarLevel);
		quote.m_bAllOrNothing = true;
		quote.m_sContextHash = m_Integrity.BuildPlayerSupportContextHash(state, quote);
		if (quote.m_sContextHash.IsEmpty())
		{
			result.m_sFailureReason = "support context hash failed";
			return result;
		}

		string integrityFailure;
		if (!m_Integrity.ValidateFrozenPlayerSupportQuote(manifest, quote, false, integrityFailure))
		{
			result.m_sFailureReason = "constructed support quote failed integrity: " + integrityFailure;
			return result;
		}

		state.m_aForceManifests.Insert(manifest);
		state.m_aForceQuotes.Insert(quote);
		result.m_bSuccess = true;
		result.m_bStateChanged = true;
		result.m_Quote = quote;
		result.m_Manifest = manifest;
		AppendQuoteEvent(state, quote, "issued", "exact all-or-nothing player QRF quote issued");
		return result;
	}

	HST_ForceConfirmationResult ConfirmPlayerSupportQuote(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_EconomyService economy,
		HST_SupportRequestService supportRequests,
		HST_ResourceLedgerService ledger,
		string actorIdentityId,
		string quoteId,
		string confirmationRequestId)
	{
		HST_ForceConfirmationResult result = new HST_ForceConfirmationResult();
		if (!state || !preset || !economy || !supportRequests || !ledger)
		{
			result.m_sFailureReason = "authority services not ready";
			return result;
		}

		HST_ForceQuoteState quote = state.FindForceQuote(quoteId);
		result.m_Quote = quote;
		if (!quote || quote.m_sQuoteKind != QUOTE_KIND_PLAYER_SUPPORT_QRF)
		{
			result.m_sFailureReason = "player support quote not found";
			return result;
		}
		if (quote.m_sActorIdentityId != actorIdentityId)
		{
			result.m_sFailureReason = "quote actor conflict";
			return result;
		}

		HST_ForceManifestState manifest = state.FindForceManifest(quote.m_sManifestId);
		result.m_Manifest = manifest;
		if (!manifest)
		{
			if (quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
			{
				RejectQuote(state, quote, "manifest missing", confirmationRequestId);
				result.m_bStateChanged = true;
			}
			result.m_sFailureReason = "manifest missing";
			return result;
		}

		if (quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
		{
			string acceptedIntegrityFailure;
			if (!m_Integrity.ValidateFrozenPlayerSupportQuote(manifest, quote, false, acceptedIntegrityFailure))
			{
				result.m_sFailureReason = "accepted support quote integrity conflict: " + acceptedIntegrityFailure;
				return result;
			}
			HST_ResourceTransactionState acceptedMoney = state.FindResourceTransaction(quote.m_sMoneyTransactionId);
			HST_ResourceTransactionState acceptedHR = state.FindResourceTransaction(quote.m_sHRTransactionId);
			int acceptedRequestCount;
			HST_SupportRequestState acceptedRequest = FindUniquePlayerSupportRequestForQuote(state, quote, acceptedRequestCount);
			if (!m_Integrity.TransactionMatchesAcceptedPlayerSupportQuote(acceptedMoney, quote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost)
				|| !m_Integrity.TransactionMatchesAcceptedPlayerSupportQuote(acceptedHR, quote, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost)
				|| acceptedRequestCount != 1 || !PlayerSupportRequestMatchesQuote(acceptedRequest, quote, manifest))
			{
				result.m_sFailureReason = "accepted support quote authority conflict";
				return result;
			}
			result.m_bSuccess = true;
			result.m_bAlreadyApplied = true;
			result.m_SupportRequest = acceptedRequest;
			return result;
		}
		if (quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
		{
			result.m_sFailureReason = "quote is no longer open";
			return result;
		}
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE || !state.m_bHQDeployed)
		{
			RejectQuote(state, quote, "campaign is no longer active", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		if (confirmationRequestId.IsEmpty())
		{
			confirmationRequestId = HST_StableIdService.NextId(state, "support_confirm_command");
			result.m_bStateChanged = true;
		}
		if (state.m_iElapsedSeconds > quote.m_iExpiresAtSecond)
		{
			quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_EXPIRED;
			quote.m_iRevision++;
			quote.m_sRejectionReason = "quote expired";
			AppendQuoteEvent(state, quote, "expired", quote.m_sRejectionReason, confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		string manifestFailure;
		if (!m_Integrity.ValidateFrozenPlayerSupportQuote(manifest, quote, true, manifestFailure))
		{
			RejectQuote(state, quote, manifestFailure, confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}
		if (m_Integrity.BuildPlayerSupportContextHash(state, quote) != quote.m_sContextHash)
		{
			RejectQuote(state, quote, "support context changed; request a new quote", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}
		string blockingReason;
		if (HasBlockingPlayerSupportRequest(state, quote.m_eSupportType, blockingReason))
		{
			RejectQuote(state, quote, blockingReason, confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}
		if (state.m_iFactionMoney < quote.m_iMoneyCost || state.m_iHR < quote.m_iHRCost)
		{
			RejectQuote(state, quote, "resources changed; request a new quote", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		HST_ResourceTransactionResult moneyReservation = ledger.ReserveCost(
			state,
			economy,
			quote.m_sMoneyTransactionId,
			confirmationRequestId,
			quote.m_sOperationId,
			actorIdentityId,
			HST_ResourceLedgerService.RESOURCE_FACTION_MONEY,
			quote.m_iMoneyCost,
			"exact player QRF support",
			quote.m_sQuoteId,
			quote.m_sManifestId
		);
		if (!moneyReservation || !moneyReservation.m_bSuccess
			|| !m_Integrity.ReservationMatchesQuote(moneyReservation.m_Transaction, quote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost, confirmationRequestId))
		{
			RollbackConfirmationTransactions(state, economy, ledger, quote, "player QRF money reservation integrity failure");
			RejectQuote(state, quote, "money reservation failed", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}
		result.m_bStateChanged = true;

		HST_ResourceTransactionResult hrReservation = ledger.ReserveCost(
			state,
			economy,
			quote.m_sHRTransactionId,
			confirmationRequestId,
			quote.m_sOperationId,
			actorIdentityId,
			HST_ResourceLedgerService.RESOURCE_HR,
			quote.m_iHRCost,
			"exact player QRF support",
			quote.m_sQuoteId,
			quote.m_sManifestId
		);
		if (!hrReservation || !hrReservation.m_bSuccess
			|| !m_Integrity.ReservationMatchesQuote(hrReservation.m_Transaction, quote, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost, confirmationRequestId))
		{
			CancelConfirmationReservations(state, economy, ledger, quote, "player QRF HR reservation failed");
			RejectQuote(state, quote, "HR reservation failed", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}
		result.m_bStateChanged = true;

		quote.m_sConfirmationRequestId = confirmationRequestId;
		string registrationFailure;
		if (!supportRequests.RegisterAcceptedExactPlayerSupport(state, preset, quote, manifest, registrationFailure))
		{
			supportRequests.RemoveAcceptedExactPlayerSupport(state, quote.m_sQuoteId, quote.m_sSupportRequestId);
			RollbackConfirmationTransactions(state, economy, ledger, quote, "exact player QRF registration failed");
			RejectQuote(state, quote, "exact support registration failed: " + registrationFailure, confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		int supportRequestCount;
		HST_SupportRequestState supportRequest = FindUniquePlayerSupportRequestForQuote(state, quote, supportRequestCount);
		if (supportRequestCount != 1 || !PlayerSupportRequestMatchesQuote(supportRequest, quote, manifest))
		{
			supportRequests.RemoveAcceptedExactPlayerSupport(state, quote.m_sQuoteId, quote.m_sSupportRequestId);
			RollbackConfirmationTransactions(state, economy, ledger, quote, "exact player QRF verification failed");
			RejectQuote(state, quote, "exact support registration verification failed", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		bool moneyCommitted = ledger.CommitReserved(state, quote.m_sMoneyTransactionId);
		bool hrCommitted = ledger.CommitReserved(state, quote.m_sHRTransactionId);
		if (!moneyCommitted || !hrCommitted)
		{
			supportRequests.RemoveAcceptedExactPlayerSupport(state, quote.m_sQuoteId, quote.m_sSupportRequestId);
			RollbackConfirmationTransactions(state, economy, ledger, quote, "player QRF ledger commit failed");
			RejectQuote(state, quote, "resource transaction commit failed", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED;
		quote.m_iAcceptedAtSecond = state.m_iElapsedSeconds;
		quote.m_iRevision++;
		result.m_bSuccess = true;
		result.m_bStateChanged = true;
		result.m_SupportRequest = supportRequest;
		AppendQuoteEvent(state, quote, "accepted", "exact player QRF registered and resource transactions committed", confirmationRequestId);
		return result;
	}

	bool CancelPlayerSupportQuote(
		HST_CampaignState state,
		HST_EconomyService economy,
		HST_SupportRequestService supportRequests,
		HST_ResourceLedgerService ledger,
		string actorIdentityId,
		string quoteId,
		string reason = "cancelled by actor",
		string commandRequestId = "")
	{
		if (!state || !economy || !supportRequests || !ledger)
			return false;
		HST_ForceQuoteState quote = state.FindForceQuote(quoteId);
		if (!quote || quote.m_sActorIdentityId != actorIdentityId || quote.m_sQuoteKind != QUOTE_KIND_PLAYER_SUPPORT_QRF)
			return false;
		if (quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_CANCELLED)
			return true;
		if (quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
			return false;

		int supportRequestCount;
		FindUniquePlayerSupportRequestForQuote(state, quote, supportRequestCount);
		if (supportRequestCount > 0 && !supportRequests.RemoveAcceptedExactPlayerSupport(state, quote.m_sQuoteId, quote.m_sSupportRequestId))
			return false;
		RollbackConfirmationTransactions(state, economy, ledger, quote, "open player QRF quote cancelled");
		quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_CANCELLED;
		quote.m_sRejectionReason = reason;
		quote.m_iRevision++;
		AppendQuoteEvent(state, quote, "cancelled", reason, commandRequestId);
		return true;
	}

	int ReconcileInterruptedPlayerSupportConfirmations(
		HST_CampaignState state,
		HST_EconomyService economy,
		HST_SupportRequestService supportRequests,
		HST_ResourceLedgerService ledger)
	{
		if (!state || !economy || !supportRequests || !ledger)
			return 0;
		int reconciled;
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
		{
			if (!quote || quote.m_sQuoteKind != QUOTE_KIND_PLAYER_SUPPORT_QRF || quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
				continue;
			HST_ResourceTransactionState moneyTransaction = state.FindResourceTransaction(quote.m_sMoneyTransactionId);
			HST_ResourceTransactionState hrTransaction = state.FindResourceTransaction(quote.m_sHRTransactionId);
			bool moneyLinked = m_Integrity.TransactionHasQuoteIdentity(moneyTransaction, quote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost);
			bool hrLinked = m_Integrity.TransactionHasQuoteIdentity(hrTransaction, quote, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost);
			int supportRequestCount;
			HST_SupportRequestState supportRequest = FindUniquePlayerSupportRequestForQuote(state, quote, supportRequestCount);
			if (!moneyLinked && !hrLinked && supportRequestCount <= 0)
				continue;

			bool supportRemoved = true;
			if (supportRequestCount > 0)
				supportRemoved = supportRequests.RemoveAcceptedExactPlayerSupport(state, quote.m_sQuoteId, quote.m_sSupportRequestId);
			if (supportRequestCount > 1 || !supportRemoved)
			{
				quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_REJECTED;
				quote.m_sRejectionReason = "interrupted support confirmation has conflicting aggregate links; manual integrity review required";
				quote.m_iRevision++;
				AppendQuoteEvent(state, quote, "restore_reconciled", quote.m_sRejectionReason, quote.m_sConfirmationRequestId);
				reconciled++;
				continue;
			}

			RollbackConfirmationTransactions(state, economy, ledger, quote, "interrupted player QRF confirmation rolled back during restore");
			quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_REJECTED;
			quote.m_sRejectionReason = "interrupted player QRF confirmation rolled back during restore";
			quote.m_iRevision++;
			string causatingRequestId = quote.m_sConfirmationRequestId;
			if (causatingRequestId.IsEmpty() && moneyLinked)
				causatingRequestId = moneyTransaction.m_sCommandRequestId;
			else if (causatingRequestId.IsEmpty() && hrLinked)
				causatingRequestId = hrTransaction.m_sCommandRequestId;
			AppendQuoteEvent(state, quote, "restore_reconciled", quote.m_sRejectionReason, causatingRequestId);
			reconciled++;
		}
		return reconciled;
	}

	HST_ForceQuoteState FindIssuedPlayerSupportQuote(HST_CampaignState state, string actorIdentityId, HST_ESupportRequestType supportType)
	{
		if (!state || actorIdentityId.IsEmpty())
			return null;
		for (int i = state.m_aForceQuotes.Count() - 1; i >= 0; i--)
		{
			HST_ForceQuoteState quote = state.m_aForceQuotes[i];
			if (quote && quote.m_sActorIdentityId == actorIdentityId && quote.m_sQuoteKind == QUOTE_KIND_PLAYER_SUPPORT_QRF
				&& quote.m_eSupportType == supportType && quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
				return quote;
		}
		return null;
	}

	HST_ForceQuoteResult IssueGarrisonQuote(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string actorIdentityId,
		string zoneId,
		int requestedMemberCount,
		string commandRequestId,
		bool validateResources = true)
	{
		HST_ForceQuoteResult result = new HST_ForceQuoteResult();
		if (!state || !preset || !m_Catalog || !m_Integrity)
		{
			result.m_sFailureReason = "planning service not ready";
			return result;
		}
		if (actorIdentityId.IsEmpty())
		{
			result.m_sFailureReason = "actor identity missing";
			return result;
		}
		if (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE || !state.m_bHQDeployed)
		{
			result.m_sFailureReason = "garrison recruitment requires an active campaign and deployed HQ";
			return result;
		}
		if (requestedMemberCount <= 0 || requestedMemberCount > MAX_RECRUIT_MEMBER_COUNT)
		{
			result.m_sFailureReason = string.Format("requested quantity must be between 1 and %1", MAX_RECRUIT_MEMBER_COUNT);
			return result;
		}
		result.m_bStateChanged = ExpireIssuedQuotes(state);
		if (PrunePlanningRecords(state))
			result.m_bStateChanged = true;
		if (!commandRequestId.IsEmpty())
		{
			HST_ForceQuoteState existingQuote = FindQuoteByCommandRequestId(state, commandRequestId);
			if (existingQuote)
			{
				if (existingQuote.m_sActorIdentityId != actorIdentityId || existingQuote.m_sQuoteKind != QUOTE_KIND_GARRISON || existingQuote.m_sTargetZoneId != zoneId || existingQuote.m_iRequestedMemberCount != requestedMemberCount)
				{
					result.m_sFailureReason = "quote request id conflict";
					return result;
				}
				HST_ForceManifestState existingManifest = state.FindForceManifest(existingQuote.m_sManifestId);
				if (existingQuote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED && existingQuote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
				{
					result.m_sFailureReason = "existing quote request is terminal";
					return result;
				}
				string replayIntegrityFailure;
				bool requireCurrentCatalog = existingQuote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED;
				if (!m_Integrity.ValidateFrozenGarrisonQuote(existingManifest, existingQuote, requireCurrentCatalog, replayIntegrityFailure))
				{
					result.m_sFailureReason = "existing quote integrity conflict: " + replayIntegrityFailure;
					return result;
				}
				if (existingQuote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
				{
					HST_ResourceTransactionState existingMoney = state.FindResourceTransaction(existingQuote.m_sMoneyTransactionId);
					HST_ResourceTransactionState existingHR = state.FindResourceTransaction(existingQuote.m_sHRTransactionId);
					if (!m_Integrity.TransactionMatchesQuote(existingMoney, existingQuote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, existingQuote.m_iMoneyCost) || !m_Integrity.TransactionMatchesQuote(existingHR, existingQuote, HST_ResourceLedgerService.RESOURCE_HR, existingQuote.m_iHRCost))
					{
						result.m_sFailureReason = "existing accepted quote transaction integrity conflict";
						return result;
					}
				}
				result.m_bSuccess = true;
				result.m_Quote = existingQuote;
				result.m_Manifest = existingManifest;
				return result;
			}
		}

		if (CountOpenGarrisonQuotes(state) >= MAX_OPEN_GARRISON_QUOTES && !HasOpenGarrisonQuoteForActor(state, actorIdentityId))
		{
			result.m_sFailureReason = "open garrison quote capacity reached";
			return result;
		}
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
		{
			result.m_sFailureReason = "zone not found";
			return result;
		}
		if (zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
		{
			result.m_sFailureReason = "zone does not support strategic garrison recruitment";
			return result;
		}

		string factionKey = preset.m_sResistanceFactionKey;
		if (factionKey.IsEmpty())
			factionKey = "FIA";
		if (zone.m_sOwnerFactionKey != factionKey)
		{
			result.m_sFailureReason = string.Format("zone is owned by %1", zone.m_sOwnerFactionKey);
			return result;
		}
		if (!state.FindFactionPool(factionKey))
		{
			result.m_sFailureReason = "faction pool unavailable";
			return result;
		}

		int abstractInfantry;
		HST_GarrisonState garrison = state.FindGarrison(zoneId, factionKey);
		if (garrison)
			abstractInfantry = Math.Max(0, garrison.m_iInfantryCount);
		int activeInfantry = Math.Max(0, zone.m_iActiveInfantryCount);
		if (zone.m_iGarrisonSlots > 0 && abstractInfantry + activeInfantry + requestedMemberCount > zone.m_iGarrisonSlots)
		{
			int remaining = Math.Max(0, zone.m_iGarrisonSlots - abstractInfantry - activeInfantry);
			result.m_sFailureReason = string.Format("all-or-nothing capacity conflict: requested %1, remaining %2", requestedMemberCount, remaining);
			return result;
		}

		HST_ForceCatalogValidationResult catalogValidation = m_Catalog.ValidateMemberCatalog(factionKey, validateResources);
		if (!catalogValidation || !catalogValidation.m_bValid)
		{
			result.m_sFailureReason = "force member catalog invalid";
			if (catalogValidation && !catalogValidation.m_sFailureReason.IsEmpty())
				result.m_sFailureReason = catalogValidation.m_sFailureReason;
			return result;
		}

		array<ref HST_ForceMemberCatalogEntry> catalog = m_Catalog.BuildMemberCatalog(factionKey);
		if (catalog.Count() == 0)
		{
			result.m_sFailureReason = "member catalog empty";
			return result;
		}
		int planningSeed = m_Integrity.BuildDeterministicSeed(state, actorIdentityId + "|" + commandRequestId, zoneId);
		int preflightMoneyCost;
		int preflightHRCost;
		for (int preflightIndex = 0; preflightIndex < requestedMemberCount; preflightIndex++)
		{
			HST_ForceMemberCatalogEntry preflightEntry = m_Integrity.SelectGarrisonMember(catalog, planningSeed, preflightIndex);
			if (!preflightEntry)
			{
				result.m_sFailureReason = "deterministic member selection failed";
				return result;
			}
			preflightMoneyCost += preflightEntry.m_iMoneyCost;
			preflightHRCost += preflightEntry.m_iHRCost;
		}
		if (state.m_iFactionMoney < preflightMoneyCost)
		{
			result.m_sFailureReason = string.Format("need $%1, have $%2", preflightMoneyCost, state.m_iFactionMoney);
			return result;
		}
		if (state.m_iHR < preflightHRCost)
		{
			result.m_sFailureReason = string.Format("need %1 HR, have %2", preflightHRCost, state.m_iHR);
			return result;
		}

		if (commandRequestId.IsEmpty())
		{
			commandRequestId = HST_StableIdService.NextId(state, "force_quote_command");
			result.m_bStateChanged = true;
		}
		string quoteId = HST_StableIdService.NextId(state, "garrison_quote");
		string manifestId = HST_StableIdService.NextId(state, "manifest");
		result.m_bStateChanged = true;
		string operationId = HST_StableIdService.BuildOperationId("garrison_recruitment", quoteId);

		HST_ForceManifestState manifest = new HST_ForceManifestState();
		manifest.m_sManifestId = manifestId;
		manifest.m_sOperationId = operationId;
		manifest.m_sQuoteId = quoteId;
		manifest.m_sCommandRequestId = commandRequestId;
		manifest.m_sForceKind = "strategic_garrison";
		manifest.m_sFactionRole = "resistance";
		manifest.m_sFactionKey = factionKey;
		manifest.m_sIntentId = "garrison_recruitment";
		manifest.m_sTargetZoneId = zoneId;
		manifest.m_sCatalogVersion = HST_ForceCatalogService.CATALOG_VERSION;
		manifest.m_sPolicyId = GARRISON_POLICY_ID;
		manifest.m_iRequestedMemberCount = requestedMemberCount;
		manifest.m_iAcceptedMemberCount = requestedMemberCount;
		manifest.m_iDeterministicSeed = planningSeed;
		manifest.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		manifest.m_bFrozen = true;

		HST_ForceManifestGroupState groupElement = new HST_ForceManifestGroupState();
		groupElement.m_sElementId = manifestId + "_group_1";
		groupElement.m_sCatalogEntryId = "strategic_garrison_roster";
		groupElement.m_sRole = "garrison";
		groupElement.m_iExpectedMemberCount = requestedMemberCount;
		manifest.m_aGroups.Insert(groupElement);

		for (int memberIndex = 0; memberIndex < requestedMemberCount; memberIndex++)
		{
			HST_ForceMemberCatalogEntry catalogEntry = m_Integrity.SelectGarrisonMember(catalog, manifest.m_iDeterministicSeed, memberIndex);
			if (!catalogEntry)
			{
				result.m_sFailureReason = "deterministic member selection failed";
				return result;
			}

			HST_ForceManifestMemberState member = new HST_ForceManifestMemberState();
			member.m_sSlotId = string.Format("%1_member_%2", manifestId, memberIndex + 1);
			member.m_sCatalogSlotId = catalogEntry.m_sEntryId;
			member.m_sGroupElementId = groupElement.m_sElementId;
			member.m_sPrefab = catalogEntry.m_sPrefab;
			member.m_sRole = catalogEntry.m_sRole;
			member.m_iOrdinal = memberIndex;
			member.m_iMoneyCost = catalogEntry.m_iMoneyCost;
			member.m_iHRCost = catalogEntry.m_iHRCost;
			member.m_iEquipmentCost = catalogEntry.m_iEquipmentCost;
			manifest.m_aMembers.Insert(member);
			manifest.m_iMoneyCost += catalogEntry.m_iMoneyCost;
			manifest.m_iHRCost += catalogEntry.m_iHRCost;
			manifest.m_iEquipmentCost += catalogEntry.m_iEquipmentCost;
		}

		manifest.m_sManifestHash = m_Integrity.BuildManifestHash(manifest);
		if (manifest.m_sManifestHash.IsEmpty())
		{
			result.m_sFailureReason = "manifest hash failed";
			return result;
		}
		if (manifest.m_iMoneyCost != preflightMoneyCost || manifest.m_iHRCost != preflightHRCost)
		{
			result.m_sFailureReason = "garrison preflight cost conflict";
			return result;
		}

		CancelSupersededGarrisonQuotes(state, actorIdentityId, commandRequestId);
		HST_ForceQuoteState quote = new HST_ForceQuoteState();
		quote.m_sQuoteId = quoteId;
		quote.m_sManifestId = manifestId;
		quote.m_sManifestHash = manifest.m_sManifestHash;
		quote.m_sOperationId = operationId;
		quote.m_sCommandRequestId = commandRequestId;
		quote.m_sActorIdentityId = actorIdentityId;
		quote.m_sQuoteKind = QUOTE_KIND_GARRISON;
		quote.m_sFactionKey = factionKey;
		quote.m_sTargetZoneId = zoneId;
		quote.m_sCatalogVersion = manifest.m_sCatalogVersion;
		quote.m_sPolicyId = manifest.m_sPolicyId;
		quote.m_vTargetPosition = zone.m_vPosition;
		quote.m_iRequestedMemberCount = requestedMemberCount;
		quote.m_iAcceptedMemberCount = requestedMemberCount;
		quote.m_iMoneyCost = manifest.m_iMoneyCost;
		quote.m_iHRCost = manifest.m_iHRCost;
		quote.m_iEquipmentCost = manifest.m_iEquipmentCost;
		quote.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		quote.m_iExpiresAtSecond = state.m_iElapsedSeconds + GARRISON_QUOTE_LIFETIME_SECONDS;
		quote.m_iExpectedGarrisonSlots = zone.m_iGarrisonSlots;
		quote.m_iExpectedAbstractInfantry = abstractInfantry;
		quote.m_iExpectedActiveInfantry = activeInfantry;
		quote.m_bAllOrNothing = true;
		quote.m_sMoneyTransactionId = HST_StableIdService.BuildTransactionId(operationId, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY);
		quote.m_sHRTransactionId = HST_StableIdService.BuildTransactionId(operationId, HST_ResourceLedgerService.RESOURCE_HR);
		quote.m_sContextHash = m_Integrity.BuildGarrisonContextHash(state, zone, factionKey);

		state.m_aForceManifests.Insert(manifest);
		state.m_aForceQuotes.Insert(quote);
		result.m_bSuccess = true;
		result.m_bStateChanged = true;
		result.m_Quote = quote;
		result.m_Manifest = manifest;
		AppendQuoteEvent(state, quote, "issued", "exact all-or-nothing garrison quote issued");
		return result;
	}

	HST_ForceConfirmationResult ConfirmGarrisonQuote(
		HST_CampaignState state,
		HST_EconomyService economy,
		HST_GarrisonService garrisons,
		HST_ResourceLedgerService ledger,
		string actorIdentityId,
		string quoteId,
		string confirmationRequestId)
	{
		HST_ForceConfirmationResult result = new HST_ForceConfirmationResult();
		if (!state || !economy || !garrisons || !ledger)
		{
			result.m_sFailureReason = "authority services not ready";
			return result;
		}

		HST_ForceQuoteState quote = state.FindForceQuote(quoteId);
		result.m_Quote = quote;
		if (!quote || quote.m_sQuoteKind != QUOTE_KIND_GARRISON)
		{
			result.m_sFailureReason = "quote not found";
			return result;
		}
		if (quote.m_sActorIdentityId != actorIdentityId)
		{
			result.m_sFailureReason = "quote actor conflict";
			return result;
		}

		bool accepted = quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED;
		if (!accepted && quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
		{
			result.m_sFailureReason = "quote is no longer open";
			return result;
		}
		if (!accepted && (state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE || !state.m_bHQDeployed))
		{
			RejectQuote(state, quote, "campaign is no longer active", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		HST_ForceManifestState manifest = state.FindForceManifest(quote.m_sManifestId);
		result.m_Manifest = manifest;
		if (!manifest)
		{
			if (!accepted)
			{
				RejectQuote(state, quote, "manifest missing", confirmationRequestId);
				result.m_bStateChanged = true;
			}
			result.m_sFailureReason = "manifest missing";
			return result;
		}

		if (accepted)
		{
			string acceptedIntegrityFailure;
			if (!m_Integrity.ValidateFrozenGarrisonQuote(manifest, quote, false, acceptedIntegrityFailure))
			{
				result.m_sFailureReason = "accepted quote integrity conflict: " + acceptedIntegrityFailure;
				return result;
			}
			HST_ResourceTransactionState acceptedMoney = state.FindResourceTransaction(quote.m_sMoneyTransactionId);
			HST_ResourceTransactionState acceptedHR = state.FindResourceTransaction(quote.m_sHRTransactionId);
			if (!m_Integrity.TransactionMatchesQuote(acceptedMoney, quote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost) || !m_Integrity.TransactionMatchesQuote(acceptedHR, quote, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost))
			{
				result.m_sFailureReason = "accepted quote transaction integrity conflict";
				return result;
			}
			result.m_bSuccess = true;
			result.m_bAlreadyApplied = true;
			return result;
		}

		if (confirmationRequestId.IsEmpty())
		{
			confirmationRequestId = HST_StableIdService.NextId(state, "garrison_confirm_command");
			result.m_bStateChanged = true;
		}
		if (state.m_iElapsedSeconds > quote.m_iExpiresAtSecond)
		{
			quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_EXPIRED;
			quote.m_iRevision++;
			quote.m_sRejectionReason = "quote expired";
			AppendQuoteEvent(state, quote, "expired", quote.m_sRejectionReason, confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}
		string manifestFailure;
		if (!m_Integrity.ValidateFrozenGarrisonQuote(manifest, quote, true, manifestFailure))
		{
			RejectQuote(state, quote, manifestFailure, confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		HST_ZoneState zone = state.FindZone(quote.m_sTargetZoneId);
		if (!zone || zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE || zone.m_sOwnerFactionKey != quote.m_sFactionKey || m_Integrity.BuildGarrisonContextHash(state, zone, quote.m_sFactionKey) != quote.m_sContextHash)
		{
			RejectQuote(state, quote, "garrison context changed; request a new quote", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}
		if (state.m_iFactionMoney < quote.m_iMoneyCost || state.m_iHR < quote.m_iHRCost)
		{
			RejectQuote(state, quote, "resources changed; request a new quote", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		HST_ResourceTransactionResult moneyReservation = ledger.ReserveCost(
			state,
			economy,
			quote.m_sMoneyTransactionId,
			confirmationRequestId,
			quote.m_sOperationId,
			actorIdentityId,
			HST_ResourceLedgerService.RESOURCE_FACTION_MONEY,
			quote.m_iMoneyCost,
			"exact garrison recruitment",
			quote.m_sQuoteId,
			quote.m_sManifestId
		);
		if (!moneyReservation || !moneyReservation.m_bSuccess || !m_Integrity.ReservationMatchesQuote(moneyReservation.m_Transaction, quote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost, confirmationRequestId))
		{
			RollbackConfirmationTransactions(state, economy, ledger, quote, "money reservation integrity failure");
			RejectQuote(state, quote, "money reservation failed", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}
		result.m_bStateChanged = true;

		HST_ResourceTransactionResult hrReservation = ledger.ReserveCost(
			state,
			economy,
			quote.m_sHRTransactionId,
			confirmationRequestId,
			quote.m_sOperationId,
			actorIdentityId,
			HST_ResourceLedgerService.RESOURCE_HR,
			quote.m_iHRCost,
			"exact garrison recruitment",
			quote.m_sQuoteId,
			quote.m_sManifestId
		);
		if (!hrReservation || !hrReservation.m_bSuccess || !m_Integrity.ReservationMatchesQuote(hrReservation.m_Transaction, quote, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost, confirmationRequestId))
		{
			CancelConfirmationReservations(state, economy, ledger, quote, "garrison HR reservation failed");
			RejectQuote(state, quote, "HR reservation failed", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}
		result.m_bStateChanged = true;

		HST_GarrisonState beforeGarrison = state.FindGarrison(quote.m_sTargetZoneId, quote.m_sFactionKey);
		int beforeInfantry;
		if (beforeGarrison)
			beforeInfantry = beforeGarrison.m_iInfantryCount;
		if (!garrisons.AddManifestForcesExact(state, quote.m_sTargetZoneId, quote.m_sFactionKey, manifest))
		{
			CancelConfirmationReservations(state, economy, ledger, quote, "exact garrison mutation failed");
			RejectQuote(state, quote, "exact garrison mutation failed", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		HST_GarrisonState afterGarrison = state.FindGarrison(quote.m_sTargetZoneId, quote.m_sFactionKey);
		if (!afterGarrison || afterGarrison.m_iInfantryCount - beforeInfantry != manifest.m_iAcceptedMemberCount || CountString(afterGarrison.m_aAcceptedManifestIds, manifest.m_sManifestId) != 1)
		{
			garrisons.RemoveManifestForcesExact(state, quote.m_sTargetZoneId, quote.m_sFactionKey, manifest);
			CancelConfirmationReservations(state, economy, ledger, quote, "exact garrison verification failed");
			RejectQuote(state, quote, "exact garrison verification failed", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		bool moneyCommitted = ledger.CommitReserved(state, quote.m_sMoneyTransactionId);
		bool hrCommitted = ledger.CommitReserved(state, quote.m_sHRTransactionId);
		if (!moneyCommitted || !hrCommitted)
		{
			garrisons.RemoveManifestForcesExact(state, quote.m_sTargetZoneId, quote.m_sFactionKey, manifest);
			RollbackConfirmationTransactions(state, economy, ledger, quote, "garrison ledger commit failed");
			RejectQuote(state, quote, "resource transaction commit failed", confirmationRequestId);
			result.m_bStateChanged = true;
			result.m_sFailureReason = quote.m_sRejectionReason;
			return result;
		}

		quote.m_sConfirmationRequestId = confirmationRequestId;
		quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED;
		quote.m_iAcceptedAtSecond = state.m_iElapsedSeconds;
		quote.m_iRevision++;
		result.m_bSuccess = true;
		result.m_bStateChanged = true;
		AppendQuoteEvent(state, quote, "accepted", "exact strategic garrison increment registered and resource transactions committed", confirmationRequestId);
		return result;
	}

	bool CancelGarrisonQuote(HST_CampaignState state, string actorIdentityId, string quoteId, string reason = "cancelled by actor", string commandRequestId = "")
	{
		if (!state)
			return false;
		HST_ForceQuoteState quote = state.FindForceQuote(quoteId);
		if (!quote || quote.m_sActorIdentityId != actorIdentityId || quote.m_sQuoteKind != QUOTE_KIND_GARRISON || quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
			return false;
		quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_CANCELLED;
		quote.m_sRejectionReason = reason;
		quote.m_iRevision++;
		AppendQuoteEvent(state, quote, "cancelled", reason, commandRequestId);
		return true;
	}

	int ReconcileInterruptedGarrisonConfirmations(HST_CampaignState state, HST_EconomyService economy, HST_GarrisonService garrisons, HST_ResourceLedgerService ledger)
	{
		if (!state || !economy || !garrisons || !ledger)
			return 0;
		int reconciled;
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
		{
			if (!quote || quote.m_sQuoteKind != QUOTE_KIND_GARRISON || quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
				continue;
			HST_ResourceTransactionState moneyTransaction = state.FindResourceTransaction(quote.m_sMoneyTransactionId);
			HST_ResourceTransactionState hrTransaction = state.FindResourceTransaction(quote.m_sHRTransactionId);
			bool moneyLinked = m_Integrity.TransactionHasQuoteIdentity(moneyTransaction, quote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost);
			bool hrLinked = m_Integrity.TransactionHasQuoteIdentity(hrTransaction, quote, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost);
			HST_GarrisonState garrison = state.FindGarrison(quote.m_sTargetZoneId, quote.m_sFactionKey);
			bool garrisonLinked = garrison && garrison.m_aAcceptedManifestIds.Contains(quote.m_sManifestId);
			if (!moneyLinked && !hrLinked && !garrisonLinked)
				continue;

			HST_ForceManifestState manifest = state.FindForceManifest(quote.m_sManifestId);
			if (garrisonLinked && manifest)
				garrisons.RemoveManifestForcesExact(state, quote.m_sTargetZoneId, quote.m_sFactionKey, manifest);
			RollbackConfirmationTransactions(state, economy, ledger, quote, "interrupted garrison confirmation rolled back during restore");
			quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_REJECTED;
			quote.m_sRejectionReason = "interrupted garrison confirmation rolled back during restore";
			if (garrisonLinked && !manifest)
				quote.m_sRejectionReason = "interrupted confirmation has missing manifest; manual integrity review required";
			quote.m_iRevision++;
			string causatingRequestId;
			if (moneyLinked)
				causatingRequestId = moneyTransaction.m_sCommandRequestId;
			else if (hrLinked)
				causatingRequestId = hrTransaction.m_sCommandRequestId;
			AppendQuoteEvent(state, quote, "restore_reconciled", quote.m_sRejectionReason, causatingRequestId);
			reconciled++;
		}
		return reconciled;
	}

	HST_ForceQuoteState FindIssuedGarrisonQuote(HST_CampaignState state, string actorIdentityId)
	{
		if (!state || actorIdentityId.IsEmpty())
			return null;
		for (int i = state.m_aForceQuotes.Count() - 1; i >= 0; i--)
		{
			HST_ForceQuoteState quote = state.m_aForceQuotes[i];
			if (quote && quote.m_sActorIdentityId == actorIdentityId && quote.m_sQuoteKind == QUOTE_KIND_GARRISON && quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
				return quote;
		}
		return null;
	}

	protected int CountOpenPlayerSupportQuotes(HST_CampaignState state)
	{
		if (!state)
			return 0;
		int count;
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
		{
			if (quote && quote.m_sQuoteKind == QUOTE_KIND_PLAYER_SUPPORT_QRF && quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
				count++;
		}
		return count;
	}

	protected bool HasBlockingPlayerSupportRequest(HST_CampaignState state, HST_ESupportRequestType supportType, out string reason)
	{
		reason = "";
		if (!state)
		{
			reason = "campaign state not ready";
			return true;
		}
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || !request.m_bPlayerRequested || request.m_eType != supportType)
				continue;
			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
			{
				reason = "another player QRF request is already queued or active";
				return true;
			}
			if (IsFullyRefundedExactPlayerSupportRequest(request))
				continue;
			if (state.m_iElapsedSeconds < request.m_iCooldownUntilSecond)
			{
				reason = string.Format("QRF support cooldown active for %1s", request.m_iCooldownUntilSecond - state.m_iElapsedSeconds);
				return true;
			}
		}
		return false;
	}

	protected bool IsFullyRefundedExactPlayerSupportRequest(HST_SupportRequestState request)
	{
		if (!request || request.m_sQuoteId.IsEmpty() || request.m_sManifestId.IsEmpty())
			return false;
		return request.m_sResolutionKind == "exact_deployment_failed_refunded"
			|| request.m_sResolutionKind == "exact_cancelled_refunded"
			|| request.m_sResolutionKind == "restore_projection_unavailable_refunded";
	}

	protected HST_ZoneState ResolvePlayerSupportSourceZone(HST_CampaignState state, string factionKey, HST_ZoneState targetZone)
	{
		if (!state || !targetZone || factionKey.IsEmpty())
			return null;
		HST_ZoneState selected;
		float selectedDistanceSq = 999999999.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey != factionKey)
				continue;
			float distanceSq = DistanceSq2D(zone.m_vPosition, targetZone.m_vPosition);
			if (!selected || distanceSq < selectedDistanceSq
				|| (Math.AbsFloat(distanceSq - selectedDistanceSq) < 0.01 && zone.m_sZoneId.Compare(selected.m_sZoneId) < 0))
			{
				selected = zone;
				selectedDistanceSq = distanceSq;
			}
		}
		return selected;
	}

	protected HST_SupportRequestState FindUniquePlayerSupportRequestForQuote(HST_CampaignState state, HST_ForceQuoteState quote, out int matchCount)
	{
		matchCount = 0;
		if (!state || !quote)
			return null;
		HST_SupportRequestState match;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || request.m_sQuoteId != quote.m_sQuoteId)
				continue;
			matchCount++;
			match = request;
		}
		if (matchCount != 1)
			return null;
		return match;
	}

	protected bool PlayerSupportRequestMatchesQuote(HST_SupportRequestState request, HST_ForceQuoteState quote, HST_ForceManifestState manifest)
	{
		if (!request || !quote || !manifest)
			return false;
		if (request.m_sRequestId != quote.m_sSupportRequestId || request.m_sOperationId != quote.m_sOperationId
			|| request.m_sQuoteId != quote.m_sQuoteId || request.m_sManifestId != quote.m_sManifestId
			|| request.m_sCommandRequestId != quote.m_sConfirmationRequestId)
			return false;
		if (request.m_sMoneyTransactionId != quote.m_sMoneyTransactionId || request.m_sHRTransactionId != quote.m_sHRTransactionId)
			return false;
		if (request.m_sFactionKey != quote.m_sFactionKey || request.m_eType != quote.m_eSupportType
			|| request.m_sCapabilityId != quote.m_sCapabilityId || request.m_sAssetProfileId != quote.m_sAssetProfileId)
			return false;
		if (request.m_sSourceZoneId != quote.m_sSourceZoneId || request.m_sTargetZoneId != quote.m_sTargetZoneId
			|| !PositionsMatch(request.m_vSourcePosition, quote.m_vSourcePosition)
			|| !PositionsMatch(request.m_vTargetPosition, quote.m_vTargetPosition))
			return false;
		if (request.m_iMoneyCost != quote.m_iMoneyCost || request.m_iHRCost != quote.m_iHRCost
			|| request.m_iPlannedInfantryCount != manifest.m_iAcceptedMemberCount
			|| request.m_iCompositionCost != manifest.m_iMoneyCost
			|| request.m_iCompositionManpower != manifest.m_iAcceptedMemberCount
			|| request.m_iETASeconds != quote.m_iETASeconds)
			return false;
		if (request.m_iCooldownUntilSecond - request.m_iRequestedAtSecond != quote.m_iCooldownSeconds)
			return false;
		return request.m_bPlayerRequested;
	}

	protected void CancelSupersededPlayerSupportQuotes(HST_CampaignState state, string actorIdentityId, HST_ESupportRequestType supportType, string causatingRequestId)
	{
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
		{
			if (!quote || quote.m_sActorIdentityId != actorIdentityId || quote.m_sQuoteKind != QUOTE_KIND_PLAYER_SUPPORT_QRF
				|| quote.m_eSupportType != supportType || quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
				continue;
			quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_CANCELLED;
			quote.m_sRejectionReason = "superseded by a newer player QRF quote";
			quote.m_iRevision++;
			AppendQuoteEvent(state, quote, "cancelled", quote.m_sRejectionReason, causatingRequestId);
		}
	}

	protected bool IsZeroPosition(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01 && Math.AbsFloat(value[1]) < 0.01 && Math.AbsFloat(value[2]) < 0.01;
	}

	protected bool PositionsMatch(vector left, vector right)
	{
		return Math.AbsFloat(left[0] - right[0]) < 0.01
			&& Math.AbsFloat(left[1] - right[1]) < 0.01
			&& Math.AbsFloat(left[2] - right[2]) < 0.01;
	}

	protected float DistanceSq2D(vector left, vector right)
	{
		float x = left[0] - right[0];
		float z = left[2] - right[2];
		return x * x + z * z;
	}

	protected int CountOpenGarrisonQuotes(HST_CampaignState state)
	{
		if (!state)
			return 0;
		int count;
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
		{
			if (quote && quote.m_sQuoteKind == QUOTE_KIND_GARRISON && quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
				count++;
		}
		return count;
	}

	protected bool HasOpenGarrisonQuoteForActor(HST_CampaignState state, string actorIdentityId)
	{
		return FindIssuedGarrisonQuote(state, actorIdentityId) != null;
	}

	bool ExpireIssuedQuotes(HST_CampaignState state)
	{
		if (!state)
			return false;
		bool changed;
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
		{
			if (!quote || quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED || state.m_iElapsedSeconds <= quote.m_iExpiresAtSecond)
				continue;
			quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_EXPIRED;
			quote.m_sRejectionReason = "quote expired";
			quote.m_iRevision++;
			AppendQuoteEvent(state, quote, "expired", quote.m_sRejectionReason);
			changed = true;
		}
		return changed;
	}

	bool PrunePlanningRecords(HST_CampaignState state)
	{
		if (!state)
			return false;
		bool changed;
		for (int quoteIndex = 0; quoteIndex < state.m_aForceQuotes.Count(); quoteIndex++)
		{
			HST_ForceQuoteState quote = state.m_aForceQuotes[quoteIndex];
			if (!quote || quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED || quote.m_eStatus == HST_EForceQuoteStatus.HST_FORCE_QUOTE_ACCEPTED)
				continue;
			if (state.m_iElapsedSeconds - quote.m_iCreatedAtSecond < TERMINAL_QUOTE_RETENTION_SECONDS)
				continue;
			if (HasTransactionReference(state, quote.m_sQuoteId, quote.m_sManifestId) || HasManifestAggregateReference(state, quote.m_sManifestId))
				continue;

			string manifestId = quote.m_sManifestId;
			state.m_aForceQuotes.Remove(quoteIndex);
			quoteIndex--;
			RemoveUnreferencedManifest(state, manifestId);
			changed = true;
		}
		return changed;
	}

	protected int CountString(array<string> values, string expected)
	{
		int count;
		foreach (string value : values)
		{
			if (value == expected)
				count++;
		}
		return count;
	}

	protected HST_ForceQuoteState FindQuoteByCommandRequestId(HST_CampaignState state, string commandRequestId)
	{
		if (!state || commandRequestId.IsEmpty())
			return null;
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
		{
			if (quote && quote.m_sCommandRequestId == commandRequestId)
				return quote;
		}
		return null;
	}

	protected bool HasTransactionReference(HST_CampaignState state, string quoteId, string manifestId)
	{
		foreach (HST_ResourceTransactionState transaction : state.m_aResourceTransactions)
		{
			if (transaction && ((!quoteId.IsEmpty() && transaction.m_sQuoteId == quoteId) || (!manifestId.IsEmpty() && transaction.m_sManifestId == manifestId)))
				return true;
		}
		return false;
	}

	protected bool HasManifestAggregateReference(HST_CampaignState state, string manifestId)
	{
		if (manifestId.IsEmpty())
			return false;
		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
		{
			if (garrison && garrison.m_aAcceptedManifestIds.Contains(manifestId))
				return true;
		}
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (group && group.m_sManifestId == manifestId)
				return true;
		}
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (request && request.m_sManifestId == manifestId)
				return true;
		}
		foreach (HST_EnemyOrderState order : state.m_aEnemyOrders)
		{
			if (order && order.m_sManifestId == manifestId)
				return true;
		}
		foreach (HST_ForceSpawnResultState spawnResult : state.m_aForceSpawnResults)
		{
			if (spawnResult && spawnResult.m_sManifestId == manifestId)
				return true;
		}
		return false;
	}

	protected void RemoveUnreferencedManifest(HST_CampaignState state, string manifestId)
	{
		if (!state || manifestId.IsEmpty() || HasManifestAggregateReference(state, manifestId) || HasTransactionReference(state, "", manifestId))
			return;
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
		{
			if (quote && quote.m_sManifestId == manifestId)
				return;
		}
		for (int manifestIndex = state.m_aForceManifests.Count() - 1; manifestIndex >= 0; manifestIndex--)
		{
			HST_ForceManifestState manifest = state.m_aForceManifests[manifestIndex];
			if (manifest && manifest.m_sManifestId == manifestId)
			{
				state.m_aForceManifests.Remove(manifestIndex);
				return;
			}
		}
	}

	protected void CancelSupersededGarrisonQuotes(HST_CampaignState state, string actorIdentityId, string causatingRequestId)
	{
		foreach (HST_ForceQuoteState quote : state.m_aForceQuotes)
		{
			if (!quote || quote.m_sActorIdentityId != actorIdentityId || quote.m_sQuoteKind != QUOTE_KIND_GARRISON || quote.m_eStatus != HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED)
				continue;
			quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_CANCELLED;
			quote.m_sRejectionReason = "superseded by a newer garrison quote";
			quote.m_iRevision++;
			AppendQuoteEvent(state, quote, "cancelled", quote.m_sRejectionReason, causatingRequestId);
		}
	}

	protected void RejectQuote(HST_CampaignState state, HST_ForceQuoteState quote, string reason, string causatingRequestId = "")
	{
		if (!quote)
			return;
		quote.m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_REJECTED;
		quote.m_sRejectionReason = reason;
		quote.m_iRevision++;
		AppendQuoteEvent(state, quote, "rejected", reason, causatingRequestId);
	}

	protected void CancelConfirmationReservations(HST_CampaignState state, HST_EconomyService economy, HST_ResourceLedgerService ledger, HST_ForceQuoteState quote, string reason)
	{
		RollbackQuoteTransaction(state, economy, ledger, quote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost, "cancel_money_" + quote.m_sQuoteId, reason);
		RollbackQuoteTransaction(state, economy, ledger, quote, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost, "cancel_hr_" + quote.m_sQuoteId, reason);
	}

	protected void RollbackConfirmationTransactions(HST_CampaignState state, HST_EconomyService economy, HST_ResourceLedgerService ledger, HST_ForceQuoteState quote, string reason)
	{
		RollbackQuoteTransaction(state, economy, ledger, quote, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY, quote.m_iMoneyCost, "rollback_money_" + quote.m_sQuoteId, reason);
		RollbackQuoteTransaction(state, economy, ledger, quote, HST_ResourceLedgerService.RESOURCE_HR, quote.m_iHRCost, "rollback_hr_" + quote.m_sQuoteId, reason);
	}

	protected void RollbackQuoteTransaction(HST_CampaignState state, HST_EconomyService economy, HST_ResourceLedgerService ledger, HST_ForceQuoteState quote, string resourceType, int amount, string settlementId, string reason)
	{
		if (!quote)
			return;
		string transactionId = quote.m_sMoneyTransactionId;
		if (resourceType == HST_ResourceLedgerService.RESOURCE_HR)
			transactionId = quote.m_sHRTransactionId;
		HST_ResourceTransactionState transaction = state.FindResourceTransaction(transactionId);
		if (!m_Integrity.TransactionHasQuoteIdentity(transaction, quote, resourceType, amount))
			return;
		RollbackTransaction(state, economy, ledger, transactionId, settlementId, reason);
	}

	protected void RollbackTransaction(HST_CampaignState state, HST_EconomyService economy, HST_ResourceLedgerService ledger, string transactionId, string settlementId, string reason)
	{
		HST_ResourceTransactionState transaction = state.FindResourceTransaction(transactionId);
		if (!transaction)
			return;
		if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED)
			ledger.CancelReservation(state, economy, transactionId, settlementId, reason);
		else if (transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED)
			ledger.RefundCommitted(state, economy, transactionId, settlementId, transaction.m_iAmount, reason);
	}

	protected void AppendQuoteEvent(HST_CampaignState state, HST_ForceQuoteState quote, string transition, string reason, string causatingRequestId = "")
	{
		if (!m_EventLog || !state || !quote)
			return;
		if (causatingRequestId.IsEmpty())
			causatingRequestId = quote.m_sCommandRequestId;
		m_EventLog.Append(state, "force", "quote", quote.m_sQuoteId, causatingRequestId, transition, reason);
	}
}
