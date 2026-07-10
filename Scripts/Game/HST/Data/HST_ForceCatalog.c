[BaseContainerProps()]
class HST_ForceMemberCatalogEntry
{
	string m_sEntryId;
	string m_sFactionKey;
	string m_sPrefab;
	string m_sRole;
	int m_iHRCost = 1;
	int m_iMoneyCost = 50;
	int m_iEquipmentCost;
	int m_iMinimumWarLevel = 1;
	int m_iWeight = 1;
}

[BaseContainerProps()]
class HST_ForceGroupCatalogSlot
{
	string m_sSlotId;
	string m_sPrefab;
	string m_sRole;
	int m_iOrdinal;
	bool m_bRequired = true;
}

[BaseContainerProps()]
class HST_ForceGroupCatalogEntry
{
	string m_sEntryId;
	string m_sFactionKey;
	string m_sRole;
	string m_sAuthoredPrefab;
	string m_sExecutionPrefab;
	int m_iWeight = 1;
	ref array<ref HST_ForceGroupCatalogSlot> m_aMemberSlots = {};
}

class HST_ForceCatalogValidationResult
{
	bool m_bValid;
	int m_iMemberEntryCount;
	int m_iGroupEntryCount;
	int m_iGroupSlotCount;
	string m_sFailureReason;
	ref array<string> m_aFailures = {};

	string BuildSummary()
	{
		if (m_bValid)
			return string.Format("force catalog %1 | valid | members %2 | groups %3 | group slots %4", HST_ForceCatalogService.CATALOG_VERSION, m_iMemberEntryCount, m_iGroupEntryCount, m_iGroupSlotCount);

		return string.Format("force catalog %1 | invalid | %2", HST_ForceCatalogService.CATALOG_VERSION, m_sFailureReason);
	}
}
