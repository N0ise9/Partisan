[BaseContainerProps()]
class HST_ForceManifestMemberState
{
	string m_sSlotId;
	string m_sCatalogSlotId;
	string m_sGroupElementId;
	string m_sPrefab;
	string m_sRole;
	string m_sAssignedVehicleSlotId;
	string m_sSeatRole;
	int m_iSeatIndex = -1;
	int m_iOrdinal;
	int m_iMoneyCost;
	int m_iHRCost = 1;
	int m_iEquipmentCost;
	bool m_bRequired = true;
}

[BaseContainerProps()]
class HST_ForceManifestGroupState
{
	string m_sElementId;
	string m_sCatalogEntryId;
	string m_sPrefab;
	string m_sRole;
	int m_iOrdinal;
	int m_iExpectedMemberCount;
	bool m_bRequired = true;
}

[BaseContainerProps()]
class HST_ForceManifestVehicleState
{
	string m_sSlotId;
	string m_sCatalogEntryId;
	string m_sGroupElementId;
	string m_sPrefab;
	string m_sRole;
	int m_iOrdinal;
	int m_iMoneyCost;
	int m_iRequiredCrew;
	bool m_bArmed;
	bool m_bLightArmor;
	bool m_bHeavyArmor;
	bool m_bRequired = true;
}

[BaseContainerProps()]
class HST_ForceManifestAssetState
{
	string m_sSlotId;
	string m_sKind;
	string m_sPrefab;
	string m_sRole;
	string m_sAssignedVehicleSlotId;
	int m_iQuantity = 1;
	int m_iOrdinal;
	bool m_bRequired = true;
}

[BaseContainerProps()]
class HST_ForceManifestState
{
	string m_sManifestId;
	string m_sManifestHash;
	string m_sOperationId;
	string m_sQuoteId;
	string m_sCommandRequestId;
	string m_sForceKind;
	string m_sFactionRole;
	string m_sFactionKey;
	string m_sIntentId;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sGroupPrefab;
	string m_sCatalogVersion;
	string m_sPolicyId;
	int m_iRequestedMemberCount;
	int m_iAcceptedMemberCount;
	int m_iRequestedVehicleCount;
	int m_iAcceptedVehicleCount;
	int m_iMoneyCost;
	int m_iHRCost;
	int m_iEquipmentCost;
	int m_iAttackResourceCost;
	int m_iSupportResourceCost;
	int m_iDeterministicSeed;
	int m_iCreatedAtSecond;
	bool m_bFrozen = true;
	ref array<ref HST_ForceManifestGroupState> m_aGroups = {};
	ref array<ref HST_ForceManifestMemberState> m_aMembers = {};
	ref array<ref HST_ForceManifestVehicleState> m_aVehicles = {};
	ref array<ref HST_ForceManifestAssetState> m_aAssets = {};

	HST_ForceManifestMemberState FindMemberSlot(string slotId)
	{
		foreach (HST_ForceManifestMemberState member : m_aMembers)
		{
			if (member && member.m_sSlotId == slotId)
				return member;
		}

		return null;
	}
}

[BaseContainerProps()]
class HST_ForceQuoteState
{
	string m_sQuoteId;
	string m_sManifestId;
	string m_sManifestHash;
	string m_sOperationId;
	string m_sCommandRequestId;
	string m_sConfirmationRequestId;
	string m_sActorIdentityId;
	string m_sQuoteKind;
	string m_sFactionKey;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sContextHash;
	string m_sCatalogVersion;
	string m_sPolicyId;
	string m_sMoneyTransactionId;
	string m_sHRTransactionId;
	string m_sAttackTransactionId;
	string m_sSupportTransactionId;
	string m_sRejectionReason;
	vector m_vSourcePosition;
	vector m_vTargetPosition;
	HST_EForceQuoteStatus m_eStatus = HST_EForceQuoteStatus.HST_FORCE_QUOTE_ISSUED;
	int m_iRequestedMemberCount;
	int m_iAcceptedMemberCount;
	int m_iRequestedVehicleCount;
	int m_iAcceptedVehicleCount;
	int m_iMoneyCost;
	int m_iHRCost;
	int m_iEquipmentCost;
	int m_iAttackResourceCost;
	int m_iSupportResourceCost;
	int m_iCreatedAtSecond;
	int m_iExpiresAtSecond;
	int m_iAcceptedAtSecond;
	int m_iRevision = 1;
	int m_iExpectedGarrisonSlots;
	int m_iExpectedAbstractInfantry;
	int m_iExpectedActiveInfantry;
	bool m_bAllOrNothing = true;
}

[BaseContainerProps()]
class HST_ForceSpawnSlotResultState
{
	string m_sSlotId;
	string m_sSlotKind;
	string m_sSpawnedPrefab;
	string m_sEntityId;
	string m_sAssignedVehicleEntityId;
	string m_sNativeGroupId;
	string m_sProjectionId;
	string m_sFailureReason;
	HST_EForceSpawnSlotStatus m_eStatus = HST_EForceSpawnSlotStatus.HST_FORCE_SLOT_QUEUED;
	int m_iAttemptCount;
	int m_iUpdatedAtSecond;
	bool m_bFactionVerified;
	bool m_bGroupVerified;
	bool m_bProjectionVerified;
	bool m_bSeatVerified;
	bool m_bAliveVerified;
}

[BaseContainerProps()]
class HST_ForceSpawnResultState
{
	string m_sResultId;
	string m_sRequestId;
	string m_sManifestId;
	string m_sManifestHash;
	string m_sOperationId;
	string m_sForceId;
	string m_sNativeGroupId;
	string m_sProjectionId;
	string m_sTerminalReason;
	string m_sLastFailureReason;
	HST_EForceSpawnBatchStatus m_eStatus = HST_EForceSpawnBatchStatus.HST_FORCE_SPAWN_PENDING;
	int m_iPriority;
	int m_iRetryCount;
	int m_iMaxRetries;
	int m_iAttemptGeneration;
	int m_iDeadlineSecond;
	int m_iCreatedAtSecond;
	int m_iLastAttemptSecond;
	int m_iNextAttemptSecond;
	int m_iUpdatedAtSecond;
	int m_iCompletedAtSecond;
	int m_iExpectedSlotCount;
	bool m_bCancelRequested;
	ref array<ref HST_ForceSpawnSlotResultState> m_aSlotResults = {};

	HST_ForceSpawnSlotResultState FindSlotResult(string slotId)
	{
		foreach (HST_ForceSpawnSlotResultState slotResult : m_aSlotResults)
		{
			if (slotResult && slotResult.m_sSlotId == slotId)
				return slotResult;
		}

		return null;
	}
}

class HST_ForceQuoteResult
{
	bool m_bSuccess;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_ForceQuoteState m_Quote;
	ref HST_ForceManifestState m_Manifest;

	string BuildSummary()
	{
		if (!m_bSuccess || !m_Quote || !m_Manifest)
			return "h-istasi force quote | failed: " + m_sFailureReason;

		return string.Format(
			"h-istasi garrison quote | %1 fighters at %2 | exact cost $%3 and %4 HR | all-or-nothing | quote %5 | expires at campaign second %6",
			m_Manifest.m_iAcceptedMemberCount,
			m_Quote.m_sTargetZoneId,
			m_Manifest.m_iMoneyCost,
			m_Manifest.m_iHRCost,
			m_Quote.m_sQuoteId,
			m_Quote.m_iExpiresAtSecond
		);
	}
}

class HST_ForceConfirmationResult
{
	bool m_bSuccess;
	bool m_bAlreadyApplied;
	bool m_bStateChanged;
	string m_sFailureReason;
	ref HST_ForceQuoteState m_Quote;
	ref HST_ForceManifestState m_Manifest;

	string BuildSummary()
	{
		if (!m_bSuccess || !m_Quote || !m_Manifest)
			return "h-istasi garrison confirmation | failed: " + m_sFailureReason;

		string disposition = "accepted";
		if (m_bAlreadyApplied)
			disposition = "already accepted";
		return string.Format(
			"h-istasi garrison confirmation | %1 | %2 fighters registered at %3 | charged $%4 and %5 HR | manifest %6",
			disposition,
			m_Manifest.m_iAcceptedMemberCount,
			m_Quote.m_sTargetZoneId,
			m_Manifest.m_iMoneyCost,
			m_Manifest.m_iHRCost,
			m_Manifest.m_sManifestId
		);
	}
}
