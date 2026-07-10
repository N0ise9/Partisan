class HST_ForcePlanningIntegrityService
{
	static const string GARRISON_POLICY_ID = "garrison_exact_all_or_nothing_1";
	static const string SUPPORT_QRF_POLICY_ID = "support_qrf_exact_infantry_1";
	static const int SUPPORT_QRF_MONEY_COST = 250;
	protected ref HST_ForceCatalogService m_Catalog = new HST_ForceCatalogService();

	string BuildManifestHash(HST_ForceManifestState manifest)
	{
		if (!manifest)
			return "";
		string canonical = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			manifest.m_sManifestId,
			manifest.m_sOperationId,
			manifest.m_sQuoteId,
			manifest.m_sCommandRequestId,
			manifest.m_sForceKind,
			manifest.m_sFactionRole,
			manifest.m_sFactionKey,
			manifest.m_sIntentId,
			manifest.m_sSourceZoneId
		);
		canonical = canonical + string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			manifest.m_sTargetZoneId,
			manifest.m_sGroupPrefab,
			manifest.m_sCatalogVersion,
			manifest.m_sPolicyId,
			manifest.m_iRequestedMemberCount,
			manifest.m_iAcceptedMemberCount,
			manifest.m_iRequestedVehicleCount,
			manifest.m_iAcceptedVehicleCount,
			manifest.m_iMoneyCost
		);
		canonical = canonical + string.Format(
			"|%1|%2|%3|%4|%5|%6|%7",
			manifest.m_iHRCost,
			manifest.m_iEquipmentCost,
			manifest.m_iAttackResourceCost,
			manifest.m_iSupportResourceCost,
			manifest.m_iDeterministicSeed,
			manifest.m_iCreatedAtSecond,
			manifest.m_bFrozen
		);
		foreach (HST_ForceManifestGroupState group : manifest.m_aGroups)
		{
			if (!group)
				continue;
			canonical = canonical + string.Format("|g:%1:%2:%3:%4:%5:%6:%7", group.m_sElementId, group.m_sCatalogEntryId, group.m_sPrefab, group.m_sRole, group.m_iOrdinal, group.m_iExpectedMemberCount, group.m_bRequired);
		}
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!member)
				continue;
			canonical = canonical + string.Format("|m:%1:%2:%3:%4:%5:%6:%7:%8:%9", member.m_sSlotId, member.m_sCatalogSlotId, member.m_sGroupElementId, member.m_sPrefab, member.m_sRole, member.m_sAssignedVehicleSlotId, member.m_sSeatRole, member.m_iSeatIndex, member.m_iOrdinal);
			canonical = canonical + string.Format(":%1:%2:%3:%4", member.m_iMoneyCost, member.m_iHRCost, member.m_iEquipmentCost, member.m_bRequired);
		}
		foreach (HST_ForceManifestVehicleState vehicle : manifest.m_aVehicles)
		{
			if (!vehicle)
				continue;
			canonical = canonical + string.Format("|v:%1:%2:%3:%4:%5:%6:%7:%8:%9", vehicle.m_sSlotId, vehicle.m_sCatalogEntryId, vehicle.m_sGroupElementId, vehicle.m_sPrefab, vehicle.m_sRole, vehicle.m_iOrdinal, vehicle.m_iMoneyCost, vehicle.m_iRequiredCrew, vehicle.m_bArmed);
			canonical = canonical + string.Format(":%1:%2:%3", vehicle.m_bLightArmor, vehicle.m_bHeavyArmor, vehicle.m_bRequired);
		}
		foreach (HST_ForceManifestAssetState asset : manifest.m_aAssets)
		{
			if (!asset)
				continue;
			canonical = canonical + string.Format("|a:%1:%2:%3:%4:%5:%6:%7:%8", asset.m_sSlotId, asset.m_sKind, asset.m_sPrefab, asset.m_sRole, asset.m_sAssignedVehicleSlotId, asset.m_iQuantity, asset.m_iOrdinal, asset.m_bRequired);
		}
		return string.Format("fm1_%1_%2", canonical.Hash(), (canonical + "|secondary").Hash());
	}

	string BuildGarrisonContextHash(HST_CampaignState state, HST_ZoneState zone, string factionKey)
	{
		if (!state || !zone)
			return "";
		int abstractInfantry;
		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, factionKey);
		if (garrison)
			abstractInfantry = Math.Max(0, garrison.m_iInfantryCount);
		string canonical = string.Format("%1|%2|%3|%4|%5|%6|%7|%8", zone.m_sZoneId, zone.m_sOwnerFactionKey, factionKey, zone.m_eType, zone.m_iGarrisonSlots, abstractInfantry, Math.Max(0, zone.m_iActiveInfantryCount), HST_ForceCatalogService.CATALOG_VERSION);
		return string.Format("gc1_%1", canonical.Hash());
	}

	string BuildPlayerSupportContextHash(HST_CampaignState state, HST_ForceQuoteState quote)
	{
		if (!state || !quote)
			return "";
		if (quote.m_iExpectedWarLevel != Math.Max(1, state.m_iWarLevel))
			return "";

		HST_ZoneState sourceZone = state.FindZone(quote.m_sSourceZoneId);
		HST_ZoneState targetZone = state.FindZone(quote.m_sTargetZoneId);
		if (!sourceZone || !targetZone)
			return "";

		int openRequestCount;
		int latestCooldownSecond;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || !request.m_bPlayerRequested || request.m_eType != quote.m_eSupportType)
				continue;
			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				openRequestCount++;
			latestCooldownSecond = Math.Max(latestCooldownSecond, request.m_iCooldownUntilSecond);
		}

		string canonical = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			state.m_ePhase,
			state.m_bHQDeployed,
			quote.m_eSupportType,
			quote.m_sFactionKey,
			quote.m_sSourceZoneId,
			sourceZone.m_sOwnerFactionKey,
			quote.m_sTargetZoneId,
			targetZone.m_sOwnerFactionKey,
			targetZone.m_eType
		);
		canonical = canonical + string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8",
			quote.m_vSourcePosition,
			quote.m_vTargetPosition,
			state.m_iWarLevel,
			openRequestCount,
			latestCooldownSecond,
			HST_ForceCatalogService.CATALOG_VERSION,
			quote.m_sPolicyId,
			quote.m_iExpectedWarLevel
		);
		return string.Format("sc1_%1_%2", canonical.Hash(), (canonical + "|secondary").Hash());
	}

	HST_ForceGroupCatalogEntry SelectPlayerSupportGroup(array<ref HST_ForceGroupCatalogEntry> catalog, int seed, int warLevel)
	{
		if (!catalog || catalog.Count() == 0)
			return null;

		int desiredMemberCount = Math.Max(3, Math.Min(12, 4 + Math.Max(1, warLevel)));
		int closestDistance = 999999;
		array<ref HST_ForceGroupCatalogEntry> closest = {};
		foreach (HST_ForceGroupCatalogEntry entry : catalog)
		{
			if (!entry || entry.m_sEntryId.IsEmpty() || entry.m_sExecutionPrefab.IsEmpty() || entry.m_aMemberSlots.Count() == 0)
				continue;
			int distance = AbsInt(entry.m_aMemberSlots.Count() - desiredMemberCount);
			if (distance < closestDistance)
			{
				closestDistance = distance;
				closest.Clear();
				closest.Insert(entry);
			}
			else if (distance == closestDistance)
			{
				closest.Insert(entry);
			}
		}

		if (closest.Count() == 0)
			return null;
		return closest[PositiveModulo(seed, closest.Count())];
	}

	HST_ForceMemberCatalogEntry SelectGarrisonMember(array<ref HST_ForceMemberCatalogEntry> catalog, int seed, int memberIndex)
	{
		if (!catalog || catalog.Count() == 0)
			return null;
		if (memberIndex == 0)
			return catalog[0];
		int selectedIndex = PositiveModulo(seed + memberIndex * 31, catalog.Count());
		return catalog[selectedIndex];
	}

	int BuildDeterministicSeed(HST_CampaignState state, string requestIdentity, string zoneId)
	{
		return string.Format("%1|force_planning|%2|%3|%4", state.m_iCampaignSeed, requestIdentity, zoneId, HST_ForceCatalogService.CATALOG_VERSION).Hash();
	}

	bool ValidateFrozenGarrisonQuote(HST_ForceManifestState manifest, HST_ForceQuoteState quote, bool requireCurrentCatalog, out string failure)
	{
		failure = "";
		if (!manifest || !quote)
		{
			failure = "quote or manifest missing";
			return false;
		}
		if (requireCurrentCatalog)
		{
			HST_ForceCatalogValidationResult catalogValidation = m_Catalog.ValidateMemberCatalog(quote.m_sFactionKey, true);
			if (!catalogValidation || !catalogValidation.m_bValid)
			{
				failure = "force member catalog no longer validates";
				if (catalogValidation && !catalogValidation.m_sFailureReason.IsEmpty())
					failure = catalogValidation.m_sFailureReason;
				return false;
			}
		}
		if (!manifest.m_bFrozen)
		{
			failure = "manifest is not frozen";
			return false;
		}
		if (manifest.m_sManifestHash != quote.m_sManifestHash || BuildManifestHash(manifest) != quote.m_sManifestHash)
		{
			failure = "manifest hash conflict";
			return false;
		}
		if (manifest.m_sCatalogVersion.IsEmpty() || manifest.m_sCatalogVersion != quote.m_sCatalogVersion)
		{
			failure = "manifest catalog conflict";
			return false;
		}
		if (requireCurrentCatalog && manifest.m_sCatalogVersion != HST_ForceCatalogService.CATALOG_VERSION)
		{
			failure = "quote catalog version is no longer current";
			return false;
		}
		return ValidateGarrisonManifest(manifest, quote, requireCurrentCatalog, failure);
	}

	bool ValidateFrozenPlayerSupportQuote(HST_ForceManifestState manifest, HST_ForceQuoteState quote, bool requireCurrentCatalog, out string failure)
	{
		failure = "";
		if (!manifest || !quote)
		{
			failure = "quote or manifest missing";
			return false;
		}
		if (requireCurrentCatalog)
		{
			HST_ForceCatalogValidationResult catalogValidation = m_Catalog.ValidateFactionCatalog(quote.m_sFactionKey, true);
			if (!catalogValidation || !catalogValidation.m_bValid)
			{
				failure = "force group catalog no longer validates";
				if (catalogValidation && !catalogValidation.m_sFailureReason.IsEmpty())
					failure = catalogValidation.m_sFailureReason;
				return false;
			}
		}
		if (!manifest.m_bFrozen)
		{
			failure = "manifest is not frozen";
			return false;
		}
		if (manifest.m_sManifestHash != quote.m_sManifestHash || BuildManifestHash(manifest) != quote.m_sManifestHash)
		{
			failure = "manifest hash conflict";
			return false;
		}
		if (manifest.m_sCatalogVersion.IsEmpty() || manifest.m_sCatalogVersion != quote.m_sCatalogVersion)
		{
			failure = "manifest catalog conflict";
			return false;
		}
		if (requireCurrentCatalog && manifest.m_sCatalogVersion != HST_ForceCatalogService.CATALOG_VERSION)
		{
			failure = "quote catalog version is no longer current";
			return false;
		}
		return ValidatePlayerSupportManifest(manifest, quote, requireCurrentCatalog, failure);
	}

	bool ValidatePlayerSupportManifest(HST_ForceManifestState manifest, HST_ForceQuoteState quote, bool requireCurrentCatalog, out string failure)
	{
		failure = "";
		if (!manifest || !quote)
		{
			failure = "quote or manifest missing";
			return false;
		}
		if (!ValidateManifestIdentity(manifest, quote) || manifest.m_sSourceZoneId != quote.m_sSourceZoneId)
		{
			failure = "support quote and manifest identity conflict";
			return false;
		}
		if (quote.m_sQuoteKind != HST_ForcePlanningService.QUOTE_KIND_PLAYER_SUPPORT_QRF
			|| quote.m_eSupportType != HST_ESupportRequestType.HST_SUPPORT_QRF
			|| manifest.m_sForceKind != "player_support"
			|| manifest.m_sIntentId != "hst_qrf_regular"
			|| manifest.m_sPolicyId != SUPPORT_QRF_POLICY_ID
			|| quote.m_sPolicyId != manifest.m_sPolicyId
			|| quote.m_sCatalogVersion != manifest.m_sCatalogVersion
			|| !quote.m_bAllOrNothing)
		{
			failure = "support policy or catalog conflict";
			return false;
		}
		if (quote.m_sSupportRequestId != "support_" + quote.m_sQuoteId
			|| quote.m_sCapabilityId != HST_ForcePlanningService.SUPPORT_QRF_CAPABILITY_ID
			|| quote.m_sAssetProfileId != HST_ForcePlanningService.SUPPORT_QRF_ASSET_PROFILE_ID)
		{
			failure = "support execution identity incomplete";
			return false;
		}
		if (quote.m_sOperationId != HST_StableIdService.BuildOperationId("support", quote.m_sSupportRequestId)
			|| quote.m_sMoneyTransactionId != HST_StableIdService.BuildTransactionId(quote.m_sOperationId, HST_ResourceLedgerService.RESOURCE_FACTION_MONEY)
			|| quote.m_sHRTransactionId != HST_StableIdService.BuildTransactionId(quote.m_sOperationId, HST_ResourceLedgerService.RESOURCE_HR))
		{
			failure = "support operation or transaction identity conflict";
			return false;
		}
		if (quote.m_iETASeconds != 120 || quote.m_iCooldownSeconds != 600 || quote.m_iExpectedWarLevel <= 0)
		{
			failure = "support schedule policy conflict";
			return false;
		}
		if (!ValidateManifestCounts(manifest, quote))
		{
			failure = "support quote and manifest force totals conflict";
			return false;
		}
		if (manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0])
		{
			failure = "support must contain exactly one group root";
			return false;
		}

		HST_ForceManifestGroupState group = manifest.m_aGroups[0];
		if (group.m_sElementId.IsEmpty() || group.m_sCatalogEntryId.IsEmpty() || group.m_sPrefab.IsEmpty()
			|| group.m_sPrefab != manifest.m_sGroupPrefab || group.m_iOrdinal != 0 || !group.m_bRequired
			|| group.m_iExpectedMemberCount != manifest.m_iAcceptedMemberCount)
		{
			failure = "support group root conflict";
			return false;
		}

		HST_ForceGroupCatalogEntry catalogGroup;
		if (requireCurrentCatalog)
		{
			catalogGroup = FindGroupCatalogEntry(m_Catalog.BuildGroupCatalog(manifest.m_sFactionKey), group.m_sCatalogEntryId);
			if (!catalogGroup || catalogGroup.m_sFactionKey != manifest.m_sFactionKey || catalogGroup.m_sExecutionPrefab != group.m_sPrefab
				|| catalogGroup.m_sRole != group.m_sRole || catalogGroup.m_aMemberSlots.Count() != manifest.m_aMembers.Count())
			{
				failure = "support group catalog conflict";
				return false;
			}
		}

		array<string> memberIds = {};
		int hrCost;
		for (int memberIndex = 0; memberIndex < manifest.m_aMembers.Count(); memberIndex++)
		{
			HST_ForceManifestMemberState member = manifest.m_aMembers[memberIndex];
			if (!member || member.m_sSlotId.IsEmpty() || memberIds.Contains(member.m_sSlotId)
				|| member.m_sGroupElementId != group.m_sElementId || member.m_iOrdinal != memberIndex
				|| member.m_sPrefab.IsEmpty() || member.m_sRole.IsEmpty() || !member.m_bRequired
				|| !member.m_sAssignedVehicleSlotId.IsEmpty() || member.m_iMoneyCost != 0
				|| member.m_iHRCost != 1 || member.m_iEquipmentCost != 0)
			{
				failure = "support member slot conflict";
				return false;
			}
			memberIds.Insert(member.m_sSlotId);
			hrCost += member.m_iHRCost;

			if (requireCurrentCatalog)
			{
				HST_ForceGroupCatalogSlot catalogSlot = catalogGroup.m_aMemberSlots[memberIndex];
				if (!catalogSlot)
				{
					failure = "support member catalog slot missing";
					return false;
				}
				string expectedCatalogSlotId = catalogGroup.m_sEntryId + "/" + catalogSlot.m_sSlotId;
				if (member.m_sCatalogSlotId != expectedCatalogSlotId
					|| member.m_sPrefab != catalogSlot.m_sPrefab || member.m_sRole != catalogSlot.m_sRole
					|| member.m_iOrdinal != catalogSlot.m_iOrdinal || member.m_bRequired != catalogSlot.m_bRequired)
				{
					failure = "support member catalog conflict";
					return false;
				}
			}
		}

		if (manifest.m_iMoneyCost != SUPPORT_QRF_MONEY_COST || quote.m_iMoneyCost != manifest.m_iMoneyCost
			|| manifest.m_iHRCost != hrCost || quote.m_iHRCost != hrCost
			|| manifest.m_iEquipmentCost != 0 || quote.m_iEquipmentCost != 0
			|| manifest.m_iAttackResourceCost != 0 || quote.m_iAttackResourceCost != 0
			|| manifest.m_iSupportResourceCost != 0 || quote.m_iSupportResourceCost != 0)
		{
			failure = "support resource totals conflict";
			return false;
		}
		return true;
	}

	bool ValidateGarrisonManifest(HST_ForceManifestState manifest, HST_ForceQuoteState quote, bool requireCurrentCatalog, out string failure)
	{
		failure = "";
		if (!manifest || !quote)
		{
			failure = "quote or manifest missing";
			return false;
		}
		if (!ValidateManifestIdentity(manifest, quote))
		{
			failure = "quote and manifest identity conflict";
			return false;
		}
		if (!ValidateManifestPolicy(manifest, quote, requireCurrentCatalog))
		{
			failure = "garrison policy or catalog conflict";
			return false;
		}
		if (!ValidateManifestCounts(manifest, quote))
		{
			failure = "quote and manifest force totals conflict";
			return false;
		}
		if (manifest.m_aGroups.Count() != 1 || !manifest.m_aGroups[0] || manifest.m_aGroups[0].m_iExpectedMemberCount != manifest.m_iAcceptedMemberCount)
		{
			failure = "garrison group element conflict";
			return false;
		}

		array<ref HST_ForceMemberCatalogEntry> memberCatalog = {};
		if (requireCurrentCatalog)
			memberCatalog = m_Catalog.BuildMemberCatalog(manifest.m_sFactionKey);
		array<string> slotIds = {};
		int moneyCost;
		int hrCost;
		int equipmentCost;
		foreach (HST_ForceManifestMemberState member : manifest.m_aMembers)
		{
			if (!ValidateManifestMember(member, manifest.m_aGroups[0], slotIds, memberCatalog, requireCurrentCatalog))
			{
				failure = "garrison member slot or catalog conflict";
				return false;
			}
			slotIds.Insert(member.m_sSlotId);
			moneyCost += member.m_iMoneyCost;
			hrCost += member.m_iHRCost;
			equipmentCost += member.m_iEquipmentCost;
		}
		if (moneyCost != manifest.m_iMoneyCost || hrCost != manifest.m_iHRCost || equipmentCost != manifest.m_iEquipmentCost)
		{
			failure = "manifest resource totals conflict";
			return false;
		}
		if (manifest.m_iMoneyCost != quote.m_iMoneyCost || manifest.m_iHRCost != quote.m_iHRCost || manifest.m_iEquipmentCost != quote.m_iEquipmentCost)
		{
			failure = "quote resource totals conflict";
			return false;
		}
		if (manifest.m_iEquipmentCost != 0 || manifest.m_iAttackResourceCost != 0 || manifest.m_iSupportResourceCost != 0 || quote.m_iAttackResourceCost != 0 || quote.m_iSupportResourceCost != 0)
		{
			failure = "unsupported garrison resource cost";
			return false;
		}
		return true;
	}

	bool TransactionMatchesQuote(HST_ResourceTransactionState transaction, HST_ForceQuoteState quote, string resourceType, int amount)
	{
		if (!TransactionHasQuoteIdentity(transaction, quote, resourceType, amount))
			return false;
		if (quote.m_sConfirmationRequestId.IsEmpty() || transaction.m_sCommandRequestId != quote.m_sConfirmationRequestId)
			return false;
		return transaction.m_iRefundedAmount == 0 && transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED;
	}

	bool TransactionMatchesAcceptedPlayerSupportQuote(HST_ResourceTransactionState transaction, HST_ForceQuoteState quote, string resourceType, int amount)
	{
		if (!TransactionHasQuoteIdentity(transaction, quote, resourceType, amount))
			return false;
		if (quote.m_sConfirmationRequestId.IsEmpty() || transaction.m_sCommandRequestId != quote.m_sConfirmationRequestId)
			return false;
		if (transaction.m_iRefundedAmount < 0 || transaction.m_iRefundedAmount > transaction.m_iAmount)
			return false;
		return transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_COMMITTED
			|| transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_PARTIALLY_REFUNDED
			|| transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_REFUNDED;
	}

	bool ReservationMatchesQuote(HST_ResourceTransactionState transaction, HST_ForceQuoteState quote, string resourceType, int amount, string confirmationRequestId)
	{
		if (!TransactionHasQuoteIdentity(transaction, quote, resourceType, amount))
			return false;
		if (confirmationRequestId.IsEmpty() || transaction.m_sCommandRequestId != confirmationRequestId)
			return false;
		return transaction.m_iRefundedAmount == 0 && transaction.m_eStatus == HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED;
	}

	bool TransactionHasQuoteIdentity(HST_ResourceTransactionState transaction, HST_ForceQuoteState quote, string resourceType, int amount)
	{
		if (!transaction || !quote)
			return false;
		string expectedTransactionId = quote.m_sMoneyTransactionId;
		if (resourceType == HST_ResourceLedgerService.RESOURCE_HR)
			expectedTransactionId = quote.m_sHRTransactionId;
		if (transaction.m_sTransactionId != expectedTransactionId || transaction.m_sQuoteId != quote.m_sQuoteId)
			return false;
		if (transaction.m_sManifestId != quote.m_sManifestId || transaction.m_sOperationId != quote.m_sOperationId)
			return false;
		if (transaction.m_sActorIdentityId != quote.m_sActorIdentityId || transaction.m_sResourceType != resourceType)
			return false;
		return transaction.m_iAmount == amount;
	}

	protected bool ValidateManifestIdentity(HST_ForceManifestState manifest, HST_ForceQuoteState quote)
	{
		if (manifest.m_sQuoteId != quote.m_sQuoteId || manifest.m_sOperationId != quote.m_sOperationId)
			return false;
		if (manifest.m_sCommandRequestId != quote.m_sCommandRequestId || manifest.m_sFactionKey != quote.m_sFactionKey)
			return false;
		return manifest.m_sTargetZoneId == quote.m_sTargetZoneId;
	}

	protected bool ValidateManifestPolicy(HST_ForceManifestState manifest, HST_ForceQuoteState quote, bool requireCurrentCatalog)
	{
		if (manifest.m_sForceKind != "strategic_garrison" || manifest.m_sIntentId != "garrison_recruitment")
			return false;
		if (manifest.m_sPolicyId.IsEmpty() || quote.m_sPolicyId != manifest.m_sPolicyId)
			return false;
		if (quote.m_sCatalogVersion != manifest.m_sCatalogVersion || !quote.m_bAllOrNothing)
			return false;
		return !requireCurrentCatalog || manifest.m_sPolicyId == GARRISON_POLICY_ID;
	}

	protected bool ValidateManifestCounts(HST_ForceManifestState manifest, HST_ForceQuoteState quote)
	{
		if (manifest.m_iRequestedMemberCount <= 0 || manifest.m_iRequestedMemberCount != manifest.m_iAcceptedMemberCount)
			return false;
		if (manifest.m_iAcceptedMemberCount != manifest.m_aMembers.Count())
			return false;
		if (manifest.m_iRequestedMemberCount != quote.m_iRequestedMemberCount || manifest.m_iAcceptedMemberCount != quote.m_iAcceptedMemberCount)
			return false;
		if (manifest.m_iRequestedVehicleCount != 0 || manifest.m_iAcceptedVehicleCount != 0)
			return false;
		if (manifest.m_aVehicles.Count() != 0 || manifest.m_aAssets.Count() != 0)
			return false;
		return quote.m_iRequestedVehicleCount == 0 && quote.m_iAcceptedVehicleCount == 0;
	}

	protected bool ValidateManifestMember(HST_ForceManifestMemberState member, HST_ForceManifestGroupState group, array<string> slotIds, array<ref HST_ForceMemberCatalogEntry> memberCatalog, bool requireCurrentCatalog)
	{
		if (!member || !group || member.m_sSlotId.IsEmpty() || slotIds.Contains(member.m_sSlotId))
			return false;
		if (member.m_sGroupElementId != group.m_sElementId || !member.m_bRequired)
			return false;
		if (member.m_sCatalogSlotId.IsEmpty() || member.m_sPrefab.IsEmpty() || member.m_sRole.IsEmpty())
			return false;
		if (member.m_iMoneyCost < 0 || member.m_iHRCost < 0 || member.m_iEquipmentCost < 0)
			return false;
		if (!requireCurrentCatalog)
			return true;
		HST_ForceMemberCatalogEntry catalogEntry = FindMemberCatalogEntry(memberCatalog, member.m_sCatalogSlotId);
		if (!catalogEntry || catalogEntry.m_sPrefab != member.m_sPrefab || catalogEntry.m_sRole != member.m_sRole)
			return false;
		return catalogEntry.m_iMoneyCost == member.m_iMoneyCost && catalogEntry.m_iHRCost == member.m_iHRCost && catalogEntry.m_iEquipmentCost == member.m_iEquipmentCost;
	}

	protected HST_ForceMemberCatalogEntry FindMemberCatalogEntry(array<ref HST_ForceMemberCatalogEntry> catalog, string entryId)
	{
		foreach (HST_ForceMemberCatalogEntry entry : catalog)
		{
			if (entry && entry.m_sEntryId == entryId)
				return entry;
		}
		return null;
	}

	protected HST_ForceGroupCatalogEntry FindGroupCatalogEntry(array<ref HST_ForceGroupCatalogEntry> catalog, string entryId)
	{
		foreach (HST_ForceGroupCatalogEntry entry : catalog)
		{
			if (entry && entry.m_sEntryId == entryId)
				return entry;
		}
		return null;
	}

	protected int AbsInt(int value)
	{
		if (value < 0)
			return -value;
		return value;
	}

	protected int PositiveModulo(int value, int divisor)
	{
		if (divisor <= 0)
			return 0;
		int result = value % divisor;
		if (result < 0)
			result += divisor;
		return result;
	}
}
