class HST_LoadoutEditorInsertCallback : ScriptedInventoryOperationCallback
{
	HST_LoadoutEditorService m_Service;
	IEntity m_TemporaryEntity;
	SCR_InventoryStorageManagerComponent m_Inventory;
	IEntity m_PlayerEntity;
	BaseInventoryStorageComponent m_RestoreStorage;
	int m_iRestoreSlotId = -1;
	HST_CampaignState m_State;
	HST_ArsenalService m_Arsenal;
	string m_sIdentityId;
	string m_sReservedPrefab;
	string m_sRestorePrefab;
	bool m_bRefundReservedOnFailure;
	bool m_bAccountRemovedOnComplete;
	bool m_bAccountRemovedOnFailure;
	bool m_bRestoreRemovedOnFailure;
	ref array<string> m_aRemovedPrefabs = {};
	ref array<string> m_aRemovedCategories = {};
	ref array<string> m_aRemovedDisplayNames = {};

	override void OnComplete()
	{
		if (m_Service)
			m_Service.CompleteInsertTransaction(this, true);
	}

	override void OnFailed()
	{
		if (m_Service)
		{
			m_Service.CompleteInsertTransaction(this, false);
			return;
		}

		if (m_TemporaryEntity)
			SCR_EntityHelper.DeleteEntityAndChildren(m_TemporaryEntity);
	}
}

class HST_LoadoutCostEntry
{
	string m_sPrefab;
	string m_sDisplayName;
	string m_sCategory;
	int m_iRequired;
	int m_iAlreadyIssuedFinite;
	int m_iAdditionalFiniteRequired;
	bool m_bInfinite;
}

[BaseContainerProps()]
class HST_PersonalLoadoutFileState
{
	int m_iSchemaVersion = 2;
	string m_sOwnerIdentityId;
	ref array<ref HST_SavedLoadoutState> m_aLoadouts = {};
}

class HST_LoadoutEditorService
{
	static const string PREVIEW_FALLBACK_PREFAB = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	static const string LOADOUT_DIRECTORY = "$profile:h-istasi/loadouts";
	static const string LOADOUT_DIRECTORY_V2 = "$profile:h-istasi/loadouts/v2";
	static const int MAX_AUTO_DRAFT_SLOTS = 12;
	static const int PERSONAL_LOADOUT_SLOT_COUNT = 5;
	static const string NODE_LOADOUT_PREFIX = "live_loadout_";
	static const string NODE_WEAPON_PREFIX = "live_weapon_";
	static const string NODE_ATTACHMENT_PREFIX = "live_attach_";
	static const string NODE_CLOTHING_ATTACHMENT_PREFIX = "live_cloth_attach_";
	static const string NODE_STORAGE_PREFIX = "live_storage_";
	static const string NODE_STORAGE_ITEM_PREFIX = "live_storage_item_";

	protected ref array<string> m_aLoadablePrefabCache = {};
	protected ref array<bool> m_aLoadablePrefabResults = {};
	protected bool m_bDebugLoggingEnabled;

	void SetDebugSettings(HST_RuntimeSettingsDebug debugSettings)
	{
		m_bDebugLoggingEnabled = false;
		if (debugSettings)
			m_bDebugLoggingEnabled = debugSettings.m_bDebugLoggingEnabled;
	}

	protected bool IsDebugLoggingEnabled()
	{
		return m_bDebugLoggingEnabled;
	}

	string OpenEditor(HST_CampaignState state, string identityId, int playerId)
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi loadout editor | failed: campaign/player state not ready";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		int loadedTemplates = LoadPersonalLoadoutsFromFile(state, identityId);
		int purgedExternal = PurgeRemovedExternalLoadoutState(state, identityId);
		if (purgedExternal > 0)
			SavePersonalLoadoutsToFile(state, identityId);
		session.m_sStatus = "open";
		session.m_sLastFailure = "";
		session.m_sPreviewPrefab = PREVIEW_FALLBACK_PREFAB;
		session.m_iOpenedAtSecond = state.m_iElapsedSeconds;
		session.m_iPlayerId = playerId;
		session.m_iSavedLoadoutCount = CountSavedLoadouts(state, identityId);
		RefreshIssuedCounts(state, identityId, session);
		EnsureFixedPersonalLoadoutSlots(state, identityId);
		RefreshLiveDraftFromPlayer(state, identityId, playerId, session);
		session.m_bPreviewSpawned = false;
		session.m_iPreviewItemCount = CountDraftItems(session);
		session.m_sPreviewStatus = "client render preview";

		state.m_sLoadoutEditorStatus = string.Format("open for %1 | preview %2 | file templates %3 | purged external %4", identityId, session.m_bPreviewSpawned, loadedTemplates, purgedExternal);
		state.m_sLastLoadoutEditorFailure = session.m_sLastFailure;
		return "h-istasi loadout editor | opened custom arsenal editor | " + BuildEditorReport(state, identityId);
	}

	string CloseEditor(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi loadout editor | failed: campaign/player state not ready";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		session.m_sStatus = "closed";
		session.m_bPreviewSpawned = false;
		session.m_sLastFailure = "";
		state.m_sLoadoutEditorStatus = "closed";
		state.m_sLastLoadoutEditorFailure = "";
		return "h-istasi loadout editor | closed | live changes kept";
	}

	string BuildEditorPayload(HST_CampaignState state, string identityId, int playerId = 0)
	{
		if (!state || identityId.IsEmpty())
			return "";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		if (playerId > 0)
			session.m_iPlayerId = playerId;
		if (PurgeRemovedExternalLoadoutState(state, identityId) > 0)
			SavePersonalLoadoutsToFile(state, identityId);
		EnsureFixedPersonalLoadoutSlots(state, identityId);
		RefreshLiveDraftFromPlayer(state, identityId, session.m_iPlayerId, session);
		RefreshDraftNodes(state, session);

		int draftItemCount = CountDraftItems(session);
		int finiteRequired;
		int infiniteRequired;
		CountDraftFiniteAndInfinite(state, session, finiteRequired, infiniteRequired);

		string payload = string.Format("HST_LOADOUT_EDITOR|%1|%2|%3|%4|%5|%6|%7", session.m_sStatus, session.m_sCurrentLoadoutId, session.m_bPreviewSpawned, session.m_aDraftSlots.Count(), draftItemCount, finiteRequired, infiniteRequired);
		payload = payload + string.Format("\nPREVIEW|%1|%2|%3|%4", session.m_bPreviewSpawned, session.m_vPreviewPosition, session.m_iPreviewItemCount, SanitizePayloadField(session.m_sPreviewStatus));
		payload = payload + string.Format("\nPREVIEW_PREFAB|%1", session.m_sPreviewPrefab);
		payload = payload + string.Format("\nDEBUG|%1", m_bDebugLoggingEnabled);
		if (!state.m_sLastLoadoutEditorFailure.IsEmpty())
			payload = payload + "\nFAILURE|" + SanitizePayloadField(state.m_sLastLoadoutEditorFailure);

		for (int categoryIndex = 0; categoryIndex < GetEditorCategoryCount(); categoryIndex++)
		{
			string categoryId = GetEditorCategoryId(categoryIndex);
			payload = payload + string.Format("\nCATEGORY|%1|%2|%3", categoryId, SanitizePayloadField(GetEditorCategoryLabel(categoryId)), CountAvailableItemsInCategory(state, categoryId));
		}

		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!IsArsenalItemAvailable(item))
				continue;

			string category = ResolveEditorCategory(item.m_sPrefab, item.m_sCategory);
			string label = HST_DisplayNameService.ResolveItemDisplayName(null, item.m_sPrefab, item.m_sDisplayName);
			string infiniteMarker = "";
			if (item.m_bUnlocked)
				infiniteMarker = "INF";
			payload = payload + BuildEditorItemPayload(category, item.m_sPrefab, label, item.m_iCount, infiniteMarker);
		}

		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (!slot)
				continue;

			string slotCategory = ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory);
			string slotLabel = HST_DisplayNameService.ResolveItemDisplayName(null, slot.m_sItemPrefab, slot.m_sDisplayName);
			payload = payload + BuildEditorSlotPayload(slot, slotCategory, slotLabel);
		}

		foreach (HST_LoadoutNodeState node : session.m_aDraftNodes)
		{
			if (!node)
				continue;

			payload = payload + BuildEditorNodePayload(node, state);
			if (node.m_bCanOpen)
				payload = payload + string.Format("\nSTORAGE|%1|%2|%3|%4|%5|%6|%7|%8", node.m_sNodeId, SanitizePayloadField(node.m_sLabel), node.m_iUsedCapacity, node.m_iTotalCapacity, node.m_bCanDeposit, node.m_fUsedVolume, node.m_fTotalVolume, node.m_fFreeVolume);
			if (node.m_sKind == "attachment")
				payload = payload + string.Format("\nATTACH|%1|%2|%3|%4|%5", node.m_sNodeId, node.m_sParentNodeId, SanitizePayloadField(node.m_sSlotKey), node.m_sItemPrefab, SanitizePayloadField(node.m_sDisplayName));
		}

		for (int loadoutSlotIndex = 0; loadoutSlotIndex < PERSONAL_LOADOUT_SLOT_COUNT; loadoutSlotIndex++)
		{
			HST_SavedLoadoutState loadout = state.FindSavedLoadout(identityId, BuildFixedLoadoutId(loadoutSlotIndex));
			if (!loadout)
				continue;

			payload = payload + string.Format("\nTEMPLATE|%1|%2|%3", loadout.m_sLoadoutId, SanitizePayloadField(loadout.m_sDisplayName), loadout.m_aSlots.Count());
		}

		return payload;
	}

	string BuildEditorCandidatePayload(HST_CampaignState state, string identityId, int playerId, string nodeId)
	{
		if (!state || identityId.IsEmpty() || nodeId.IsEmpty())
			return string.Format("HST_LOADOUT_CANDIDATES|%1|unavailable|0|%2", nodeId, SanitizePayloadField("Loadout editor session unavailable"));

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		if (playerId > 0)
			session.m_iPlayerId = playerId;

		RefreshLiveDraftFromPlayer(state, identityId, session.m_iPlayerId, session);
		RefreshDraftNodes(state, session);
		HST_LoadoutNodeState node = FindDraftNodeById(session, nodeId);
		if (!node)
			return string.Format("HST_LOADOUT_CANDIDATES|%1|missing|0|%2", nodeId, SanitizePayloadField("Selected slot no longer exists"));

		int candidateCount;
		string emptyReason;
		int availableItems;
		int categoryMatches;
		int compatibilityMatches;
		string candidates = BuildCandidatePayloadsForNode(state, identityId, session, node, candidateCount, emptyReason, availableItems, categoryMatches, compatibilityMatches);
		if (candidateCount > 0)
			emptyReason = "";
		else if (emptyReason.IsEmpty())
			emptyReason = "No compatible arsenal items";

		string logReason = emptyReason;
		if (logReason.IsEmpty())
			logReason = "ready";

		if (IsDebugLoggingEnabled())
		{
			string candidateLog = string.Format("h-istasi loadout editor debug | candidates node %1 kind %2 category %3 slot %4 label %5 item %6", nodeId, node.m_sKind, node.m_sCategory, node.m_sSlotKey, ShortenDebugText(node.m_sDisplayName, 64), ShortenDebugText(node.m_sItemPrefab, 96));
			candidateLog = candidateLog + string.Format(" | count %1 arsenal %2 available %3 categoryMatches %4 compatible %5 reason %6", candidateCount, state.m_aArsenalItems.Count(), availableItems, categoryMatches, compatibilityMatches, logReason);
			if (node.m_sKind == "storage")
				candidateLog = candidateLog + string.Format(" | storage used %1 total %2 usedVol %3 totalVol %4 freeVol %5", node.m_iUsedCapacity, node.m_iTotalCapacity, node.m_fUsedVolume, node.m_fTotalVolume, node.m_fFreeVolume);
			Print(candidateLog);
		}
		return string.Format("HST_LOADOUT_CANDIDATES|%1|ready|%2|%3", nodeId, candidateCount, SanitizePayloadField(emptyReason)) + candidates;
	}

	string BuildEditorReport(HST_CampaignState state, string identityId)
	{
		if (!state)
			return "h-istasi loadout editor | campaign state not ready";

		HST_LoadoutEditorSessionState session = state.FindLoadoutEditorSession(identityId);
		string status = state.m_sLoadoutEditorStatus;
		if (session)
			status = string.Format("%1 | preview %2/%3 | saved %4 | issued finite %5 | issued INF %6", session.m_sStatus, session.m_bPreviewSpawned, session.m_iPreviewItemCount, CountSavedLoadouts(state, identityId), session.m_iIssuedFiniteCount, session.m_iIssuedInfiniteCount);

		string report = string.Format("status %1 | arsenal categories %2", status, BuildAvailableCategorySummary(state));
		HST_SavedLoadoutState firstLoadout = state.FindFirstSavedLoadout(identityId);
		if (firstLoadout)
			report = report + string.Format(" | first saved %1 (%2 slots)", firstLoadout.m_sDisplayName, firstLoadout.m_aSlots.Count());
		else
			report = report + " | no saved loadouts";

		if (!state.m_sLastLoadoutEditorFailure.IsEmpty())
			report = report + " | last failure " + state.m_sLastLoadoutEditorFailure;

		return report;
	}

	string SaveCurrentDraft(HST_CampaignState state, string identityId, string loadoutName = "", int playerId = 0)
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi loadout editor | failed: campaign/player state not ready";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		if (playerId > 0)
			session.m_iPlayerId = playerId;
		EnsureFixedPersonalLoadoutSlots(state, identityId);
		IEntity playerEntity = ResolveControlledPlayerEntity(session.m_iPlayerId);
		if (!playerEntity)
			return "h-istasi loadout editor | failed: no live player entity to save";

		string serialized;
		if (!SerializeCharacterLoadout(playerEntity, serialized))
		{
			state.m_sLastLoadoutEditorFailure = "failed to serialize current character loadout";
			return "h-istasi loadout editor | failed: could not serialize current loadout";
		}

		string targetLoadoutId = session.m_sCurrentLoadoutId;
		if (ResolveFixedLoadoutIndex(loadoutName) >= 0)
		{
			targetLoadoutId = loadoutName;
			loadoutName = "";
		}

		int slotIndex = ResolveSaveSlotIndex(state, identityId, targetLoadoutId);
		HST_SavedLoadoutState loadout = FindOrCreateFixedLoadoutSlot(state, identityId, slotIndex);
		string existingDisplayName = loadout.m_sDisplayName;
		bool wasEmptySlot = IsFixedLoadoutSlotEmpty(loadout);
		loadout.m_sSerializedLoadout = serialized;
		loadout.m_sCharacterPrefab = ResolveEntityPrefab(playerEntity);
		loadout.m_iUpdatedAtSecond = state.m_iElapsedSeconds;
		if (loadoutName.IsEmpty())
		{
			if (existingDisplayName.IsEmpty() || wasEmptySlot)
				loadout.m_sDisplayName = string.Format("Field Loadout %1", slotIndex + 1);
			else
				loadout.m_sDisplayName = existingDisplayName;
		}
		else
			loadout.m_sDisplayName = loadoutName;
		loadout.m_aSlots.Clear();
		RefreshLiveDraftFromPlayer(state, identityId, session.m_iPlayerId, session);
		foreach (HST_LoadoutSlotState draftSlot : session.m_aDraftSlots)
			loadout.m_aSlots.Insert(CopySlot(draftSlot));
		BuildSavedLoadoutMetadata(loadout);

		SavePersonalLoadoutsToFile(state, identityId);
		session.m_sCurrentLoadoutId = loadout.m_sLoadoutId;
		session.m_iSavedLoadoutCount = CountSavedLoadouts(state, identityId);
		session.m_sStatus = "saved";
		state.m_sLoadoutEditorStatus = string.Format("saved %1", loadout.m_sDisplayName);
		state.m_sLastLoadoutEditorFailure = "";
		return string.Format("h-istasi loadout editor | saved slot %1 | %2", slotIndex + 1, loadout.m_sDisplayName);
	}

	string ApplySavedLoadout(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId, string loadoutId = "")
	{
		if (!state || !arsenal || identityId.IsEmpty())
			return "h-istasi loadout editor | failed: service not ready";

		HST_SavedLoadoutState loadout = SelectLoadoutForApply(state, identityId, loadoutId);
		if (!loadout)
			return "h-istasi loadout editor | failed: no current or saved personal loadout";
		if (IsFixedLoadoutSlotEmpty(loadout))
			return "h-istasi loadout editor | failed: selected loadout slot is empty";

		array<ref HST_LoadoutCostEntry> costLedger = {};
		string validationFailure;
		if (!ValidateLoadoutTransaction(state, loadout, identityId, costLedger, validationFailure))
		{
			state.m_sLastLoadoutEditorFailure = validationFailure;
			return "h-istasi loadout editor | failed: " + validationFailure;
		}

		bool applied;
		if (!loadout.m_sSerializedLoadout.IsEmpty())
			applied = ApplySerializedLoadoutToPlayerEntity(loadout, playerId, validationFailure);
		else
			applied = ApplyLoadoutToPlayerEntity(loadout, playerId, validationFailure);
		if (!applied)
		{
			state.m_sLastLoadoutEditorFailure = validationFailure;
			return "h-istasi loadout editor | failed: " + validationFailure;
		}

		if (!CommitLoadoutTransaction(state, arsenal, loadout, identityId, costLedger, validationFailure))
		{
			state.m_sLastLoadoutEditorFailure = validationFailure;
			return "h-istasi loadout editor | failed: " + validationFailure;
		}

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		session.m_sStatus = "applied";
		session.m_sCurrentLoadoutId = loadout.m_sLoadoutId;
		session.m_iPlayerId = playerId;
		RefreshLiveDraftFromPlayer(state, identityId, playerId, session);
		RefreshIssuedCounts(state, identityId, session);
		state.m_sLoadoutEditorStatus = string.Format("applied %1", loadout.m_sDisplayName);
		state.m_sLastLoadoutEditorFailure = "";
		return string.Format("h-istasi loadout editor | applied %1 | finite %2 | INF %3", loadout.m_sDisplayName, session.m_iIssuedFiniteCount, session.m_iIssuedInfiniteCount);
	}

	string AddDraftItem(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId, string itemPrefab)
	{
		if (!state || identityId.IsEmpty() || itemPrefab.IsEmpty())
			return "h-istasi loadout editor | failed: missing item";

		HST_ArsenalItemState item = state.FindArsenalItem(itemPrefab);
		if (!IsArsenalItemAvailable(item))
			return "h-istasi loadout editor | failed: item not available in arsenal";
		if (HST_ArsenalItemFilter.ShouldBlockArsenalPrefab(item.m_sPrefab, ResolveEditorCategory(item.m_sPrefab, item.m_sCategory), item.m_sDisplayName))
			return "h-istasi loadout editor | failed: structural inventory containers cannot be added from arsenal";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		session.m_iPlayerId = playerId;

		string failure;
		if (!ReserveArsenalItemForEditor(state, arsenal, identityId, itemPrefab, 1, failure))
			return "h-istasi loadout editor | failed: " + failure;

		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		SCR_InventoryStorageManagerComponent inventory = ResolveInventoryManager(playerEntity, failure);
		if (!inventory || !TryInsertItemIntoInventory(inventory, playerEntity, itemPrefab, failure, state, arsenal, identityId))
		{
			RefundReservedItem(state, arsenal, identityId, itemPrefab, 1);
			state.m_sLastLoadoutEditorFailure = failure;
			return "h-istasi loadout editor | failed: " + failure;
		}

		session.m_sStatus = "live edited";
		RefreshLiveDraftFromPlayer(state, identityId, playerId, session);
		RefreshDraftNodes(state, session);
		RefreshIssuedCounts(state, identityId, session);
		ClearLoadoutEditorFailure(state, identityId);
		return "h-istasi loadout editor | added " + HST_DisplayNameService.ResolveItemDisplayName(null, itemPrefab, item.m_sDisplayName);
	}

	string SetNodeItem(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId, string argument)
	{
		if (!state || identityId.IsEmpty() || argument.IsEmpty())
			return "h-istasi loadout editor | failed: missing node replacement";

		string nodeId;
		string itemPrefab;
		if (!ParseSlotPrefabArgument(argument, nodeId, itemPrefab))
			return "h-istasi loadout editor | failed: malformed node replacement";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		session.m_iPlayerId = playerId;

		HST_ArsenalItemState item = state.FindArsenalItem(itemPrefab);
		if (!IsArsenalItemAvailable(item))
			return "h-istasi loadout editor | failed: item not available in arsenal";

		RefreshLiveDraftFromPlayer(state, identityId, playerId, session);
		RefreshDraftNodes(state, session);
		HST_LoadoutNodeState targetNode = FindDraftNodeById(session, nodeId);
		string itemCategory = ResolveEditorCategory(item.m_sPrefab, item.m_sCategory);
		string itemDisplay = HST_DisplayNameService.ResolveItemDisplayName(null, item.m_sPrefab, item.m_sDisplayName);
		bool isStorageBrowserNode;
		if (targetNode)
			isStorageBrowserNode = targetNode.m_sKind == "storage" || targetNode.m_sKind == "storage_item";
		if (HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(item.m_sPrefab, itemCategory) || HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(itemDisplay, itemCategory) || (isStorageBrowserNode && HST_ArsenalItemFilter.ShouldBlockArsenalPrefab(item.m_sPrefab, itemCategory, itemDisplay)))
			return "h-istasi loadout editor | failed: structural inventory containers cannot be added from arsenal";

		bool ammoMatch;
		if (targetNode && !IsLiveCandidateCompatible(state, playerId, targetNode, item.m_sPrefab, itemCategory, ammoMatch))
			return "h-istasi loadout editor | failed: " + itemDisplay + " is not compatible with " + targetNode.m_sLabel;

		string failure;
		if (!ReserveArsenalItemForEditor(state, arsenal, identityId, itemPrefab, 1, failure))
			return "h-istasi loadout editor | failed: " + failure;

		if (!ApplyLiveNodeItem(state, arsenal, identityId, playerId, nodeId, itemPrefab, failure))
		{
			RefundReservedItem(state, arsenal, identityId, itemPrefab, 1);
			state.m_sLastLoadoutEditorFailure = failure;
			return "h-istasi loadout editor | failed: " + failure;
		}

		session.m_sStatus = "live edited";
		RefreshLiveDraftFromPlayer(state, identityId, playerId, session);
		RefreshDraftNodes(state, session);
		RefreshIssuedCounts(state, identityId, session);
		ClearLoadoutEditorFailure(state, identityId);
		return "h-istasi loadout editor | set " + ResolveNodeLabelFromId(nodeId) + " to " + HST_DisplayNameService.ResolveItemDisplayName(null, itemPrefab, item.m_sDisplayName);
	}

	string RemoveNodeItem(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId, string nodeId)
	{
		if (!state || identityId.IsEmpty() || nodeId.IsEmpty())
			return "h-istasi loadout editor | failed: missing node";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		session.m_iPlayerId = playerId;
		string removedDisplay;
		string failure;
		if (!RemoveLiveNodeItem(state, arsenal, identityId, playerId, nodeId, removedDisplay, failure))
		{
			state.m_sLastLoadoutEditorFailure = failure;
			return "h-istasi loadout editor | failed: " + failure;
		}

		session.m_sStatus = "live edited";
		RefreshLiveDraftFromPlayer(state, identityId, playerId, session);
		RefreshDraftNodes(state, session);
		RefreshIssuedCounts(state, identityId, session);
		ClearLoadoutEditorFailure(state, identityId);
		return "h-istasi loadout editor | removed " + removedDisplay;
	}

	string RemoveDraftSlot(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId, string slotId)
	{
		if (!state || identityId.IsEmpty() || slotId.IsEmpty())
			return "h-istasi loadout editor | failed: missing slot";

		return RemoveNodeItem(state, arsenal, identityId, playerId, slotId);
	}

	string SetDraftSlotQuantity(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId, string argument)
	{
		if (!state || identityId.IsEmpty() || argument.IsEmpty())
			return "h-istasi loadout editor | failed: missing quantity";

		string slotId;
		int quantity;
		if (!ParseSlotQuantityArgument(argument, slotId, quantity))
			return "h-istasi loadout editor | failed: malformed quantity";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		RefreshLiveDraftFromPlayer(state, identityId, playerId, session);
		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (slot && slot.m_sSlotId == slotId && !slot.m_sItemPrefab.IsEmpty())
				return SetNodeItem(state, arsenal, identityId, playerId, slotId + ":" + slot.m_sItemPrefab);
		}

		return "h-istasi loadout editor | failed: slot not found";
	}

	string ReplaceDraftSlotItem(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId, string argument)
	{
		if (!state || identityId.IsEmpty() || argument.IsEmpty())
			return "h-istasi loadout editor | failed: missing replacement";

		string slotId;
		string itemPrefab;
		if (!ParseSlotPrefabArgument(argument, slotId, itemPrefab))
			return "h-istasi loadout editor | failed: malformed replacement";

		return SetNodeItem(state, arsenal, identityId, playerId, slotId + ":" + itemPrefab);
	}

	string SelectSavedLoadout(HST_CampaignState state, string identityId, string loadoutId)
	{
		if (!state || identityId.IsEmpty() || loadoutId.IsEmpty())
			return "h-istasi loadout editor | failed: missing template";

		HST_SavedLoadoutState loadout = state.FindSavedLoadout(identityId, loadoutId);
		if (!loadout)
			return "h-istasi loadout editor | failed: template not found";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		session.m_sCurrentLoadoutId = loadout.m_sLoadoutId;
		session.m_sStatus = "saved slot selected";
		RefreshDraftNodes(state, session);
		ClearLoadoutEditorFailure(state, identityId);
		return "h-istasi loadout editor | selected " + loadout.m_sDisplayName;
	}

	string RenameSavedLoadout(HST_CampaignState state, string identityId, string argument)
	{
		if (!state || identityId.IsEmpty() || argument.IsEmpty())
			return "h-istasi loadout editor | failed: missing template rename";

		string loadoutId;
		string loadoutName;
		if (!ParseLoadoutRenameArgument(argument, loadoutId, loadoutName))
			return "h-istasi loadout editor | failed: malformed template rename";

		HST_SavedLoadoutState loadout = state.FindSavedLoadout(identityId, loadoutId);
		if (!loadout)
			return "h-istasi loadout editor | failed: template not found";
		if (IsFixedLoadoutSlotEmpty(loadout))
			return "h-istasi loadout editor | failed: selected loadout slot is empty";

		loadout.m_sDisplayName = loadoutName;
		loadout.m_iUpdatedAtSecond = state.m_iElapsedSeconds;
		SavePersonalLoadoutsToFile(state, identityId);

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		session.m_sCurrentLoadoutId = loadout.m_sLoadoutId;
		session.m_sStatus = "template renamed";
		ClearLoadoutEditorFailure(state, identityId);
		return "h-istasi loadout editor | renamed " + loadout.m_sDisplayName;
	}

	string ClearDraft(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId)
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi loadout editor | failed: campaign/player state not ready";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		string failure;
		int removed = ClearLiveIssuedItems(state, arsenal, identityId, playerId, failure);
		if (!failure.IsEmpty())
			return "h-istasi loadout editor | failed: " + failure;
		session.m_sStatus = "live cleared";
		RefreshLiveDraftFromPlayer(state, identityId, playerId, session);
		RefreshDraftNodes(state, session);
		RefreshIssuedCounts(state, identityId, session);
		ClearLoadoutEditorFailure(state, identityId);
		return string.Format("h-istasi loadout editor | cleared %1 issued item(s)", removed);
	}

	string DeleteSavedLoadout(HST_CampaignState state, string identityId, string loadoutId)
	{
		if (!state || identityId.IsEmpty() || loadoutId.IsEmpty())
			return "h-istasi loadout editor | failed: missing template";

		for (int i = state.m_aSavedLoadouts.Count() - 1; i >= 0; i--)
		{
			HST_SavedLoadoutState loadout = state.m_aSavedLoadouts[i];
			if (!loadout || loadout.m_sOwnerIdentityId != identityId || loadout.m_sLoadoutId != loadoutId)
				continue;

			string label = loadout.m_sDisplayName;
			ClearFixedLoadoutSlot(loadout);
			SavePersonalLoadoutsToFile(state, identityId);
			HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
			if (session.m_sCurrentLoadoutId == loadoutId)
				session.m_sCurrentLoadoutId = "";
			session.m_iSavedLoadoutCount = CountSavedLoadouts(state, identityId);
			session.m_sStatus = "template deleted";
			RefreshDraftNodes(state, session);
			ClearLoadoutEditorFailure(state, identityId);
			return "h-istasi loadout editor | deleted " + label;
		}

		return "h-istasi loadout editor | failed: template not found";
	}

	string MarkIssuedLoadoutLostOnDeath(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi loadout editor | death ledger clear skipped";

		int cleared;
		for (int i = state.m_aIssuedLoadoutItems.Count() - 1; i >= 0; i--)
		{
			HST_IssuedLoadoutItemState issuedItem = state.m_aIssuedLoadoutItems[i];
			if (!issuedItem || issuedItem.m_sOwnerIdentityId != identityId)
				continue;

			state.m_aIssuedLoadoutItems.Remove(i);
			cleared++;
		}

		HST_LoadoutEditorSessionState session = state.FindLoadoutEditorSession(identityId);
		if (session)
			RefreshIssuedCounts(state, identityId, session);

		return string.Format("h-istasi loadout editor | death cleared %1 issued item ledger entries without refund", cleared);
	}

	protected HST_LoadoutEditorSessionState FindOrCreateSession(HST_CampaignState state, string identityId)
	{
		HST_LoadoutEditorSessionState session = state.FindLoadoutEditorSession(identityId);
		if (session)
			return session;

		session = new HST_LoadoutEditorSessionState();
		session.m_sOwnerIdentityId = identityId;
		state.m_aLoadoutEditorSessions.Insert(session);
		return session;
	}

	protected bool RefreshLiveDraftFromPlayer(HST_CampaignState state, string identityId, int playerId, HST_LoadoutEditorSessionState session)
	{
		if (!session)
			return false;

		session.m_aDraftSlots.Clear();
		session.m_aDraftNodes.Clear();
		session.m_bLiveCharacterAvailable = false;
		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		if (!playerEntity)
		{
			session.m_sPreviewStatus = "live character unavailable";
			return false;
		}

		session.m_bLiveCharacterAvailable = true;
		session.m_sPreviewPrefab = ResolveEntityPrefab(playerEntity);
		if (session.m_sPreviewPrefab.IsEmpty())
			session.m_sPreviewPrefab = PREVIEW_FALLBACK_PREFAB;

		ScanCharacterLoadoutSlots(playerEntity, session);
		ScanCharacterWeaponSlots(playerEntity, session);
		ScanEquippedStorageContainers(state, playerEntity, session);
		session.m_iPreviewItemCount = CountDraftItems(session);
		session.m_sPreviewStatus = "client render preview";
		return true;
	}

	protected void ScanCharacterLoadoutSlots(IEntity playerEntity, HST_LoadoutEditorSessionState session)
	{
		SCR_CharacterInventoryStorageComponent characterStorage = SCR_CharacterInventoryStorageComponent.Cast(playerEntity.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!characterStorage)
			return;

		int slotCount = characterStorage.GetSlotsCount();
		int vestLikeSlotOrdinal;
		for (int slotIndex = 0; slotIndex < slotCount; slotIndex++)
		{
			InventoryStorageSlot slot = characterStorage.GetSlot(slotIndex);
			LoadoutSlotInfo loadoutSlot = LoadoutSlotInfo.Cast(slot);
			if (!slot || !loadoutSlot)
				continue;

			string category = ResolveLoadoutSlotCategory(loadoutSlot, slot.GetAttachedEntity());
			if (IsVestLikeLoadoutSlot(loadoutSlot, slot.GetAttachedEntity()))
			{
				if (vestLikeSlotOrdinal > 0)
					category = "webbing";
				vestLikeSlotOrdinal++;
			}
			string label = ResolveLoadoutSlotLabel(loadoutSlot, category, slot.GetAttachedEntity());
			string focus = ResolveFocusForCategory(category);
			string nodeId = NODE_LOADOUT_PREFIX + string.Format("%1", slotIndex);
			AddLiveNodeFromSlot(session, nodeId, "", "slot", category, label, focus, slot.GetAttachedEntity(), true, true, true);
			ScanClothingAttachmentSlots(session, slot.GetAttachedEntity(), nodeId, slotIndex, focus);
		}
	}

	protected void ScanCharacterWeaponSlots(IEntity playerEntity, HST_LoadoutEditorSessionState session)
	{
		array<BaseInventoryStorageComponent> storages = {};
		FindInventoryStoragesWithSlots(playerEntity, storages);
		int longGunOrdinal;
		for (int storageIndex = 0; storageIndex < storages.Count(); storageIndex++)
		{
			BaseInventoryStorageComponent storage = storages[storageIndex];
			if (!EquipedWeaponStorageComponent.Cast(storage))
				continue;

			int slotCount = storage.GetSlotsCount();
			for (int slotIndex = 0; slotIndex < slotCount; slotIndex++)
			{
				InventoryStorageSlot slot = storage.GetSlot(slotIndex);
				if (!slot)
					continue;

				BaseWeaponComponent weaponSlot = BaseWeaponComponent.Cast(slot.GetParentContainer());
				if (!weaponSlot)
					continue;

				string weaponType = weaponSlot.GetWeaponSlotType();
				weaponType.ToLower();
				string category = "weapon";
				string label = "Primary Weapon";
				string focus = "weapon";
				if (weaponType == "secondary")
				{
					category = "sidearm";
					label = "Sidearm";
				}
				else if (weaponType == "grenade")
				{
					category = "explosive";
					label = "Grenade";
					focus = "hands";
				}
				else if (weaponType == "throwable")
				{
					category = "explosive";
					label = "Throwable";
					focus = "hands";
				}
				else if (weaponType == "launcher")
				{
					category = "launcher";
					label = "Launcher";
				}
				else
				{
					if (longGunOrdinal == 0)
						label = "Primary Weapon";
					else
						label = "Secondary Weapon";
					longGunOrdinal++;
				}

				string nodeId = NODE_WEAPON_PREFIX + string.Format("%1_%2", storageIndex, slotIndex);
				AddLiveNodeFromSlot(session, nodeId, "", "weapon_slot", category, label, focus, slot.GetAttachedEntity(), true, true, false);
				ScanWeaponAttachmentSlots(playerEntity, session, slot.GetAttachedEntity(), nodeId, storageIndex, slotIndex);
			}
		}
	}

	protected void ScanWeaponAttachmentSlots(IEntity playerEntity, HST_LoadoutEditorSessionState session, IEntity weaponEntity, string parentNodeId, int weaponStorageIndex, int weaponSlotIndex)
	{
		if (!weaponEntity)
			return;

		SCR_WeaponAttachmentsStorageComponent attachmentStorage = SCR_WeaponAttachmentsStorageComponent.Cast(weaponEntity.FindComponent(SCR_WeaponAttachmentsStorageComponent));
		if (!attachmentStorage)
			return;

		int slotCount = attachmentStorage.GetSlotsCount();
		for (int slotIndex = 0; slotIndex < slotCount; slotIndex++)
		{
			InventoryStorageSlot slot = attachmentStorage.GetSlot(slotIndex);
			if (!slot)
				continue;

			AttachmentSlotComponent attachmentSlot = AttachmentSlotComponent.Cast(slot.GetParentContainer());
			if (!attachmentSlot || !attachmentSlot.GetAttachmentSlotType() || !attachmentSlot.ShouldShowInInspection())
				continue;

			IEntity attachedEntity = ResolveEditableAttachmentEntity(slot.GetAttachedEntity());
			string slotKey = ResolveAttachmentSlotKeyFromSlot(slot, attachmentSlot, attachedEntity);
			string label = ResolveAttachmentSlotLabel(slotKey, slot.GetSourceName());
			string nodeId = NODE_ATTACHMENT_PREFIX + string.Format("%1_%2_%3", weaponStorageIndex, weaponSlotIndex, slotIndex);
			AddLiveNodeFromSlot(session, nodeId, parentNodeId, "attachment", "attachment", label, "weapon", attachedEntity, true, false, false);
			HST_LoadoutNodeState added = FindDraftNodeById(session, nodeId);
			if (added)
				added.m_sSlotKey = slotKey;
		}
	}

	protected void ScanClothingAttachmentSlots(HST_LoadoutEditorSessionState session, IEntity clothingEntity, string parentNodeId, int loadoutSlotIndex, string focus)
	{
		if (!session || !clothingEntity)
			return;

		array<BaseInventoryStorageComponent> attachmentStorages = {};
		FindEditableAttachmentStorages(clothingEntity, attachmentStorages);
		for (int storageIndex = 0; storageIndex < attachmentStorages.Count(); storageIndex++)
		{
			BaseInventoryStorageComponent attachmentStorage = attachmentStorages[storageIndex];
			if (!attachmentStorage)
				continue;

			int slotCount = attachmentStorage.GetSlotsCount();
			for (int slotIndex = 0; slotIndex < slotCount; slotIndex++)
			{
				InventoryStorageSlot slot = attachmentStorage.GetSlot(slotIndex);
				if (!slot)
					continue;

				AttachmentSlotComponent attachmentSlot = AttachmentSlotComponent.Cast(slot.GetParentContainer());
				if (!attachmentSlot || !attachmentSlot.GetAttachmentSlotType() || !attachmentSlot.ShouldShowInInspection())
					continue;

				IEntity attachedEntity = ResolveEditableAttachmentEntity(slot.GetAttachedEntity());
				string slotKey = ResolveAttachmentSlotKeyFromSlot(slot, attachmentSlot, attachedEntity);
				string label = ResolveAttachmentSlotLabel(slotKey, slot.GetSourceName());
				string nodeId = NODE_CLOTHING_ATTACHMENT_PREFIX + string.Format("%1_%2_%3", loadoutSlotIndex, storageIndex, slotIndex);
				AddLiveNodeFromSlot(session, nodeId, parentNodeId, "attachment", "attachment", label, focus, attachedEntity, true, false, false);
				HST_LoadoutNodeState added = FindDraftNodeById(session, nodeId);
				if (added)
					added.m_sSlotKey = slotKey;
			}
		}
	}

	protected int FindEditableAttachmentStorages(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		if (!entity)
			return 0;

		array<Managed> components = {};
		entity.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed component : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!storage || storage.GetSlotsCount() <= 0 || outStorages.Find(storage) >= 0)
				continue;

			if (!StorageHasInspectableAttachmentSlot(storage))
				continue;

			outStorages.Insert(storage);
		}

		return outStorages.Count();
	}

	protected bool StorageHasInspectableAttachmentSlot(BaseInventoryStorageComponent storage)
	{
		if (!storage)
			return false;

		int slotCount = storage.GetSlotsCount();
		for (int slotIndex = 0; slotIndex < slotCount; slotIndex++)
		{
			InventoryStorageSlot slot = storage.GetSlot(slotIndex);
			if (!slot)
				continue;

			AttachmentSlotComponent attachmentSlot = AttachmentSlotComponent.Cast(slot.GetParentContainer());
			if (attachmentSlot && attachmentSlot.GetAttachmentSlotType() && attachmentSlot.ShouldShowInInspection())
				return true;
		}

		return false;
	}

	protected IEntity ResolveEditableAttachmentEntity(IEntity attachedEntity)
	{
		if (!attachedEntity)
			return null;
		if (!InventoryItemComponent.Cast(attachedEntity.FindComponent(InventoryItemComponent)))
			return null;

		return attachedEntity;
	}

	protected void ScanEquippedStorageContainers(HST_CampaignState state, IEntity playerEntity, HST_LoadoutEditorSessionState session)
	{
		SCR_CharacterInventoryStorageComponent characterStorage = SCR_CharacterInventoryStorageComponent.Cast(playerEntity.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!characterStorage)
			return;

		int slotCount = characterStorage.GetSlotsCount();
		for (int slotIndex = 0; slotIndex < slotCount; slotIndex++)
		{
			InventoryStorageSlot slot = characterStorage.GetSlot(slotIndex);
			if (!slot || !slot.GetAttachedEntity())
				continue;

			IEntity containerEntity = slot.GetAttachedEntity();
			array<BaseInventoryStorageComponent> containerStorages = {};
			FindCargoDepositStorages(containerEntity, containerStorages);
			array<BaseInventoryStorageComponent> insertStorages = {};
			if (FindStorageInsertTargetStorages(containerEntity, insertStorages) <= 0)
				continue;
			array<BaseInventoryStorageComponent> contentStorages = {};
			AddUniqueStorages(insertStorages, contentStorages);

			string containerNodeId = NODE_STORAGE_PREFIX + string.Format("%1", slotIndex);
			string label = ResolveEntityDisplayName(containerEntity, ResolveEntityPrefab(containerEntity));
			if (label.IsEmpty())
				label = "Storage";

			HST_LoadoutNodeState node = new HST_LoadoutNodeState();
			node.m_sNodeId = containerNodeId;
			node.m_sKind = "storage";
			node.m_sSlotKey = "storage";
			node.m_sLabel = label;
			node.m_sDisplayName = label;
			node.m_sItemPrefab = ResolveEntityPrefab(containerEntity);
			node.m_sCategory = "storage";
			node.m_sFocus = ResolveFocusForCategory(ResolveLoadoutSlotCategory(LoadoutSlotInfo.Cast(slot), containerEntity));
			node.m_bCanOpen = true;
			node.m_bCanDeposit = true;
			node.m_bCanRemove = false;
			node.m_iParentSlotIndex = slotIndex;
			float usedVolume;
			float totalVolume;
			float freeVolume;
			CalculateStorageVolume(insertStorages, usedVolume, totalVolume, freeVolume);
			node.m_fUsedVolume = usedVolume;
			node.m_fTotalVolume = totalVolume;
			node.m_fFreeVolume = freeVolume;
			node.m_iUsedCapacity = CountStorageDisplayItems(contentStorages);
			node.m_iTotalCapacity = CountStorageAvailableFitOptions(state, playerEntity, insertStorages);
			session.m_aDraftNodes.Insert(node);
			if (IsDebugLoggingEnabled())
			{
				string storageLog = string.Format("h-istasi loadout editor debug | storage node %1 label %2 slot %3 cargoTargets %4 insertTargets %5", containerNodeId, ShortenDebugText(label, 64), slotIndex, containerStorages.Count(), insertStorages.Count());
				storageLog = storageLog + string.Format(" | usedItems %1 fitOptions %2 usedVol %3 totalVol %4 freeVol %5 prefab %6", node.m_iUsedCapacity, node.m_iTotalCapacity, node.m_fUsedVolume, node.m_fTotalVolume, node.m_fFreeVolume, ShortenDebugText(node.m_sItemPrefab, 96));
				Print(storageLog);
			}

			AddStorageContentNodes(session, contentStorages, containerNodeId, slotIndex);
		}
	}

	protected void AddStorageContentNodes(HST_LoadoutEditorSessionState session, notnull array<BaseInventoryStorageComponent> storages, string parentNodeId, int containerSlotIndex)
	{
		if (storages.Count() == 0)
			return;

		array<IEntity> contents = {};
		array<IEntity> visited = {};
		GatherStorageContentEntitiesFromStorages(storages, contents, visited);
		array<string> groupKeys = {};
		array<int> groupFirstIndexes = {};
		array<int> groupQuantities = {};
		array<IEntity> groupItems = {};
		for (int itemIndex = 0; itemIndex < contents.Count(); itemIndex++)
		{
			IEntity item = contents[itemIndex];
			if (!item)
				continue;

			string prefab = ResolveEntityPrefab(item);
			string category = ResolveEditorCategory(prefab, ResolveCategoryFromEntity(item, prefab));
			string display = ResolveEntityDisplayName(item, prefab);
			if (!ShouldDisplayStorageContentEntity(item, prefab, category, display))
				continue;

			string groupKey = prefab + "|" + category + "|" + display;
			int groupIndex = groupKeys.Find(groupKey);
			if (groupIndex >= 0)
			{
				groupQuantities[groupIndex] = groupQuantities[groupIndex] + 1;
				continue;
			}

			groupKeys.Insert(groupKey);
			groupFirstIndexes.Insert(itemIndex);
			groupQuantities.Insert(1);
			groupItems.Insert(item);
		}

		for (int groupIndex = 0; groupIndex < groupItems.Count(); groupIndex++)
		{
			IEntity item = groupItems[groupIndex];
			if (!item)
				continue;

			string prefab = ResolveEntityPrefab(item);
			string category = ResolveEditorCategory(prefab, ResolveCategoryFromEntity(item, prefab));
			string nodeId = NODE_STORAGE_ITEM_PREFIX + string.Format("%1_%2", containerSlotIndex, groupFirstIndexes[groupIndex]);
			HST_LoadoutNodeState node = new HST_LoadoutNodeState();
			node.m_sNodeId = nodeId;
			node.m_sParentNodeId = parentNodeId;
			node.m_sKind = "storage_item";
			node.m_sSlotKey = category;
			node.m_sLabel = "";
			node.m_sItemPrefab = prefab;
			node.m_sDisplayName = ResolveEntityDisplayName(item, prefab);
			node.m_sCategory = category;
			node.m_sFocus = ResolveFocusForCategory(category);
			node.m_bCanRemove = true;
			node.m_bCanDeposit = true;
			node.m_iParentSlotIndex = containerSlotIndex;
			node.m_iSlotIndex = groupFirstIndexes[groupIndex];
			node.m_iQuantity = Math.Max(1, groupQuantities[groupIndex]);
			session.m_aDraftNodes.Insert(node);

			HST_LoadoutSlotState slotState = BuildSlotSummaryFromEntity(nodeId, item, category, "storage_item");
			slotState.m_iQuantity = node.m_iQuantity;
			slotState.m_sParentSlotId = parentNodeId;
			session.m_aDraftSlots.Insert(slotState);
		}
	}

	protected void AddLiveNodeFromSlot(HST_LoadoutEditorSessionState session, string nodeId, string parentNodeId, string kind, string category, string label, string focus, IEntity attached, bool canRemove, bool canOpen, bool canDeposit)
	{
		HST_LoadoutNodeState node = new HST_LoadoutNodeState();
		node.m_sNodeId = nodeId;
		node.m_sParentNodeId = parentNodeId;
		node.m_sKind = kind;
		node.m_sSlotKey = category;
		node.m_sLabel = label;
		node.m_sCategory = category;
		node.m_sFocus = focus;
		node.m_bCanRemove = canRemove && attached != null;
		node.m_bCanOpen = canOpen && attached != null;
		node.m_bCanDeposit = canDeposit && attached != null;
		if (attached)
		{
			string prefab = ResolveEntityPrefab(attached);
			node.m_sItemPrefab = prefab;
			node.m_sDisplayName = ResolveEntityDisplayName(attached, prefab);
			session.m_aDraftSlots.Insert(BuildSlotSummaryFromEntity(nodeId, attached, category, kind));
		}
		else
		{
			node.m_sDisplayName = "Empty Slot";
		}

		session.m_aDraftNodes.Insert(node);
	}

	protected HST_LoadoutSlotState BuildSlotSummaryFromEntity(string slotId, IEntity entity, string category, string kind)
	{
		HST_LoadoutSlotState slot = new HST_LoadoutSlotState();
		slot.m_sSlotId = slotId;
		slot.m_sItemPrefab = ResolveEntityPrefab(entity);
		slot.m_sDisplayName = ResolveEntityDisplayName(entity, slot.m_sItemPrefab);
		slot.m_sCategory = category;
		slot.m_sSlotKind = kind;
		slot.m_iQuantity = 1;
		return slot;
	}

	protected BaseInventoryStorageComponent ResolveUsableDepositStorage(IEntity entity)
	{
		array<BaseInventoryStorageComponent> storages = {};
		if (FindUsableDepositStorages(entity, storages) <= 0)
			return null;

		return storages[0];
	}

	protected int FindUsableDepositStorages(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		if (!entity)
			return 0;

		array<Managed> components = {};
		entity.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed component : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!IsUsableDepositStorage(storage) || outStorages.Find(storage) >= 0)
				continue;

			outStorages.Insert(storage);
		}

		return outStorages.Count();
	}

	protected int FindInventoryStoragesWithSlots(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		if (!entity)
			return 0;

		array<Managed> components = {};
		int count = entity.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed component : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!storage || storage.GetSlotsCount() <= 0)
				continue;

			outStorages.Insert(storage);
		}

		return outStorages.Count();
	}

	protected bool IsUsableDepositStorage(BaseInventoryStorageComponent storage)
	{
		if (!HasInventoryStorageCapacity(storage))
			return false;

		if (SCR_SalineStorageComponent.Cast(storage) || SCR_TourniquetStorageComponent.Cast(storage))
			return false;

		if (SCR_Enum.HasFlag(storage.GetPurpose(), EStoragePurpose.PURPOSE_DEPOSIT))
			return true;

		if (SCR_UniversalInventoryStorageComponent.Cast(storage))
			return true;

		return false;
	}

	protected bool IsCargoDepositStorage(BaseInventoryStorageComponent storage)
	{
		if (!HasInventoryStorageCapacity(storage))
			return false;

		if (SCR_SalineStorageComponent.Cast(storage) || SCR_TourniquetStorageComponent.Cast(storage))
			return false;

		if (IsStructuralAttachmentStorage(storage))
			return false;

		if (SCR_Enum.HasFlag(storage.GetPurpose(), EStoragePurpose.PURPOSE_DEPOSIT))
			return true;

		if (SCR_UniversalInventoryStorageComponent.Cast(storage))
			return true;

		return false;
	}

	protected int FindCargoDepositStorages(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		array<IEntity> visited = {};
		FindCargoDepositStoragesRecursive(entity, outStorages, visited, 0);
		return outStorages.Count();
	}

	protected void FindCargoDepositStoragesRecursive(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages, notnull array<IEntity> visited, int depth)
	{
		if (!entity || depth > 8 || visited.Find(entity) >= 0)
			return;

		visited.Insert(entity);
		array<Managed> components = {};
		entity.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed component : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!storage)
				continue;

			FindOwnedCargoDepositStorages(storage, outStorages, 0);
			if (!HasInventoryStorageCapacity(storage))
				continue;

			if (IsCargoDepositStorage(storage) && outStorages.Find(storage) < 0)
				outStorages.Insert(storage);

			if (IsStructuralAttachmentStorage(storage))
				FindCargoDepositStoragesInAttachedEntities(storage, outStorages, visited, depth + 1);
		}
	}

	protected void FindOwnedCargoDepositStorages(BaseInventoryStorageComponent storage, notnull array<BaseInventoryStorageComponent> outStorages, int depth)
	{
		if (!storage || depth > 4)
			return;

		array<BaseInventoryStorageComponent> ownedStorages = {};
		storage.GetOwnedStorages(ownedStorages, 1, false);
		foreach (BaseInventoryStorageComponent ownedStorage : ownedStorages)
		{
			if (!ownedStorage || ownedStorage == storage)
				continue;

			if (IsCargoDepositStorage(ownedStorage) && outStorages.Find(ownedStorage) < 0)
				outStorages.Insert(ownedStorage);

			FindOwnedCargoDepositStorages(ownedStorage, outStorages, depth + 1);
		}
	}

	protected bool IsStructuralAttachmentStorage(BaseInventoryStorageComponent storage)
	{
		if (!storage || storage.GetSlotsCount() <= 0)
			return false;

		return SCR_Enum.HasFlag(storage.GetPurpose(), EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT);
	}

	protected bool HasInventoryStorageCapacity(BaseInventoryStorageComponent storage)
	{
		if (!storage)
			return false;

		if (storage.GetSlotsCount() > 0)
			return true;

		return storage.GetMaxVolumeCapacity() > 0.0;
	}

	protected void FindCargoDepositStoragesInAttachedEntities(BaseInventoryStorageComponent storage, notnull array<BaseInventoryStorageComponent> outStorages, notnull array<IEntity> visited, int depth)
	{
		array<IEntity> attached = {};
		array<IEntity> attachedVisited = {};
		GatherStorageContentEntities(storage, attached, attachedVisited);
		foreach (IEntity child : attached)
			FindCargoDepositStoragesRecursive(child, outStorages, visited, depth);
	}

	protected int FindRefundableContentStorages(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		FindCargoDepositStorages(entity, outStorages);
		if (!entity || !BaseWeaponComponent.Cast(entity.FindComponent(BaseWeaponComponent)))
			return outStorages.Count();

		array<Managed> components = {};
		entity.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed component : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!SCR_WeaponAttachmentsStorageComponent.Cast(storage) || outStorages.Find(storage) >= 0)
				continue;

			outStorages.Insert(storage);
		}

		return outStorages.Count();
	}

	protected int FindStorageInsertTargetStorages(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		FindCargoDepositStorages(entity, outStorages);

		return outStorages.Count();
	}

	protected void AddUniqueStorages(notnull array<BaseInventoryStorageComponent> source, notnull array<BaseInventoryStorageComponent> target)
	{
		foreach (BaseInventoryStorageComponent storage : source)
		{
			if (storage && target.Find(storage) < 0)
				target.Insert(storage);
		}
	}

	protected int FindStructuralAttachmentStorages(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		array<IEntity> visited = {};
		FindStructuralAttachmentStoragesRecursive(entity, outStorages, visited, 0);
		return outStorages.Count();
	}

	protected void FindStructuralAttachmentStoragesRecursive(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages, notnull array<IEntity> visited, int depth)
	{
		if (!entity || depth > 8 || visited.Find(entity) >= 0)
			return;

		visited.Insert(entity);
		array<Managed> components = {};
		entity.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed component : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!storage || storage.GetSlotsCount() <= 0 || !IsStructuralAttachmentStorage(storage))
				continue;

			if (outStorages.Find(storage) < 0)
				outStorages.Insert(storage);

			array<IEntity> attached = {};
			array<IEntity> attachedVisited = {};
			GatherStorageContentEntities(storage, attached, attachedVisited);
			foreach (IEntity child : attached)
				FindStructuralAttachmentStoragesRecursive(child, outStorages, visited, depth + 1);
		}
	}

	protected void GatherStorageContentEntitiesFromStorages(notnull array<BaseInventoryStorageComponent> storages, notnull array<IEntity> outItems, notnull array<IEntity> visited)
	{
		foreach (BaseInventoryStorageComponent storage : storages)
			GatherStorageContentEntities(storage, outItems, visited);
	}

	protected void GatherStorageContentEntities(BaseInventoryStorageComponent storage, notnull array<IEntity> outItems, notnull array<IEntity> visited)
	{
		if (!storage)
			return;

		array<InventoryItemComponent> ownedItems = {};
		storage.GetOwnedItems(ownedItems);
		foreach (InventoryItemComponent itemComponent : ownedItems)
		{
			if (!itemComponent)
				continue;

			InventoryStorageSlot parentSlot = itemComponent.GetParentSlot();
			if (parentSlot)
				AddStorageContentEntity(parentSlot.GetAttachedEntity(), outItems, visited);
		}

		int slotCount = storage.GetSlotsCount();
		for (int slotIndex = 0; slotIndex < slotCount; slotIndex++)
		{
			InventoryStorageSlot slot = storage.GetSlot(slotIndex);
			if (slot)
				AddStorageContentEntity(slot.GetAttachedEntity(), outItems, visited);
		}
	}

	protected void AddStorageContentEntity(IEntity item, notnull array<IEntity> outItems, notnull array<IEntity> visited)
	{
		if (!item || visited.Find(item) >= 0)
			return;

		visited.Insert(item);
		outItems.Insert(item);
	}

	protected int CountStorageItems(notnull array<BaseInventoryStorageComponent> storages)
	{
		array<IEntity> contents = {};
		array<IEntity> visited = {};
		GatherStorageContentEntitiesFromStorages(storages, contents, visited);
		return contents.Count();
	}

	protected int CountStorageDisplayItems(notnull array<BaseInventoryStorageComponent> storages)
	{
		array<IEntity> contents = {};
		array<IEntity> visited = {};
		GatherStorageContentEntitiesFromStorages(storages, contents, visited);

		int count;
		foreach (IEntity item : contents)
		{
			if (!item)
				continue;

			string prefab = ResolveEntityPrefab(item);
			string category = ResolveEditorCategory(prefab, ResolveCategoryFromEntity(item, prefab));
			string display = ResolveEntityDisplayName(item, prefab);
			if (ShouldDisplayStorageContentEntity(item, prefab, category, display))
				count++;
		}

		return count;
	}

	protected bool ShouldDisplayStorageContentEntity(IEntity item, string prefab, string category, string display)
	{
		if (!item)
			return false;

		return !HST_ArsenalItemFilter.ShouldBlockArsenalEntity(item, prefab, category, display);
	}

	protected void CalculateStorageVolume(notnull array<BaseInventoryStorageComponent> storages, out float usedVolume, out float totalVolume, out float freeVolume)
	{
		usedVolume = 0.0;
		totalVolume = 0.0;
		freeVolume = 0.0;

		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage)
				continue;

			usedVolume += CalculateStorageOccupiedVolume(storage);
			totalVolume += CalculateStorageMaxVolume(storage);
		}

		freeVolume = Math.Max(0.0, totalVolume - usedVolume);
	}

	protected float CalculateStorageOccupiedVolume(BaseInventoryStorageComponent storage)
	{
		if (!storage)
			return 0.0;

		if (ClothNodeStorageComponent.Cast(storage))
		{
			float occupied;
			array<BaseInventoryStorageComponent> ownedStorages = {};
			storage.GetOwnedStorages(ownedStorages, 1, false);
			foreach (BaseInventoryStorageComponent ownedStorage : ownedStorages)
			{
				if (SCR_UniversalInventoryStorageComponent.Cast(ownedStorage))
					occupied += Math.Max(0.0, ownedStorage.GetOccupiedSpace());
			}

			return occupied;
		}

		return Math.Max(0.0, storage.GetOccupiedSpace());
	}

	protected float CalculateStorageMaxVolume(BaseInventoryStorageComponent storage)
	{
		if (!storage)
			return 0.0;

		if (ClothNodeStorageComponent.Cast(storage))
		{
			float capacity;
			array<BaseInventoryStorageComponent> ownedStorages = {};
			storage.GetOwnedStorages(ownedStorages, 1, false);
			foreach (BaseInventoryStorageComponent ownedStorage : ownedStorages)
			{
				if (SCR_UniversalInventoryStorageComponent.Cast(ownedStorage))
					capacity += Math.Max(0.0, ownedStorage.GetMaxVolumeCapacity());
			}

			return capacity;
		}

		return Math.Max(0.0, storage.GetMaxVolumeCapacity());
	}

	protected int CountStorageAvailableFitOptions(HST_CampaignState state, IEntity playerEntity, notnull array<BaseInventoryStorageComponent> storages)
	{
		if (!state || !playerEntity || storages.Count() == 0)
			return 0;

		string failure;
		SCR_InventoryStorageManagerComponent inventory = ResolveInventoryManager(playerEntity, failure);
		if (!inventory)
			return 0;

		int fitOptions;
		int checkedItems;
		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!IsArsenalItemAvailable(item))
				continue;

			checkedItems++;
			if (checkedItems > 96)
				break;

			bool fits;
			ResourceName resourceName = item.m_sPrefab;
			foreach (BaseInventoryStorageComponent storage : storages)
			{
				if (!storage)
					continue;

				if (inventory.FindStorageForResourceInsert(resourceName, storage, EStoragePurpose.PURPOSE_ANY))
				{
					fits = true;
					break;
				}
			}

			if (!fits)
				continue;

			fitOptions++;
			if (fitOptions >= 24)
				break;
		}

		return fitOptions;
	}

	protected string ResolveEntityPrefab(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "";

		return entity.GetPrefabData().GetPrefabName();
	}

	protected string ResolveEntityDisplayName(IEntity entity, string prefab = "")
	{
		string displayName;
		if (!entity)
			return HST_DisplayNameService.ResolveItemDisplayName(null, prefab, displayName);

		InventoryItemComponent itemComponent = InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
		if (itemComponent && itemComponent.GetUIInfo())
			displayName = itemComponent.GetUIInfo().GetName();

		return HST_DisplayNameService.ResolveItemDisplayName(null, prefab, displayName);
	}

	protected string ResolveCategoryFromEntity(IEntity entity, string prefab)
	{
		string prefabCategory = ResolveCategoryFromPrefab(prefab);
		if (!entity)
			return prefabCategory;

		if (IsLoadoutClothingCategory(prefabCategory))
			return prefabCategory;
		if (BaseWeaponComponent.Cast(entity.FindComponent(BaseWeaponComponent)))
			return "weapon";
		if (MagazineComponent.Cast(entity.FindComponent(MagazineComponent)) || BaseMagazineComponent.Cast(entity.FindComponent(BaseMagazineComponent)))
			return "magazine";
		if (AttachmentSlotComponent.Cast(entity.FindComponent(AttachmentSlotComponent)))
			return "attachment";
		if (BaseLoadoutClothComponent.Cast(entity.FindComponent(BaseLoadoutClothComponent)))
			return "clothing";

		return prefabCategory;
	}

	protected bool IsLoadoutClothingCategory(string category)
	{
		return category == "clothing" || category == "headgear" || category == "vest" || category == "webbing" || category == "pants" || category == "boots" || category == "backpack" || category == "handwear";
	}

	protected string ResolveLoadoutSlotCategory(LoadoutSlotInfo loadoutSlot, IEntity attachedEntity = null)
	{
		string itemCategory = ResolveCategoryFromEntity(attachedEntity, ResolveEntityPrefab(attachedEntity));
		if (IsLoadoutClothingCategory(itemCategory))
			return itemCategory;

		if (!loadoutSlot)
			return "clothing";

		string source = loadoutSlot.GetSourceName();
		string loweredSource = source;
		loweredSource.ToLower();

		LoadoutAreaType area = loadoutSlot.GetAreaType();
		string areaName;
		if (area)
			areaName = area.Type().ToString();
		areaName.ToLower();

		if (areaName.Contains("head") || loweredSource.Contains("hat") || loweredSource.Contains("head"))
			return "headgear";
		if (areaName.Contains("jacket") || areaName.Contains("body") || loweredSource.Contains("jacket") || loweredSource.Contains("blouse") || loweredSource.Contains("uniform"))
			return "clothing";
		if (IsWebbingText(loweredSource) || IsWebbingText(areaName))
			return "webbing";
		if (areaName.Contains("vest") || loweredSource.Contains("vest"))
			return "vest";
		if (areaName.Contains("pants") || areaName.Contains("trouser") || loweredSource.Contains("pants") || loweredSource.Contains("trouser"))
			return "pants";
		if (areaName.Contains("boot") || areaName.Contains("feet") || loweredSource.Contains("boot"))
			return "boots";
		if (areaName.Contains("back") || loweredSource.Contains("back") || loweredSource.Contains("pack"))
			return "backpack";
		if (areaName.Contains("hand") || loweredSource.Contains("hand"))
			return "handwear";

		return "clothing";
	}

	protected string ResolveLoadoutSlotLabel(LoadoutSlotInfo loadoutSlot, string category, IEntity attachedEntity = null)
	{
		if (category == "headgear")
			return "Headgear";
		if (category == "clothing")
			return "Jacket";
		if (category == "vest")
			return "Armored Vest";
		if (category == "webbing")
			return "Chest Rig";
		if (category == "pants")
			return "Pants";
		if (category == "boots")
			return "Boots";
		if (category == "backpack")
			return "Backpack";
		if (category == "handwear")
			return "Handwear";

		if (loadoutSlot && !loadoutSlot.GetSourceName().IsEmpty())
			return loadoutSlot.GetSourceName();

		return "Clothing";
	}

	protected bool IsLoadoutSlotWebbing(LoadoutSlotInfo loadoutSlot, IEntity attachedEntity = null)
	{
		if (IsWebbingText(ResolveEntityDisplayName(attachedEntity, ResolveEntityPrefab(attachedEntity))) || IsWebbingText(ResolveEntityPrefab(attachedEntity)))
			return true;

		if (!loadoutSlot)
			return false;

		if (IsWebbingText(loadoutSlot.GetSourceName()))
			return true;

		LoadoutAreaType area = loadoutSlot.GetAreaType();
		if (!area)
			return false;

		return IsWebbingText(area.Type().ToString());
	}

	protected bool IsVestLikeLoadoutSlot(LoadoutSlotInfo loadoutSlot, IEntity attachedEntity = null)
	{
		string attachedCategory = ResolveCategoryFromEntity(attachedEntity, ResolveEntityPrefab(attachedEntity));
		if (attachedCategory == "vest" || attachedCategory == "webbing")
			return true;

		if (!loadoutSlot)
			return false;

		string source = loadoutSlot.GetSourceName();
		source.ToLower();
		if (source.Contains("vest") || IsWebbingText(source))
			return true;

		LoadoutAreaType area = loadoutSlot.GetAreaType();
		if (!area)
			return false;

		string areaName = area.Type().ToString();
		areaName.ToLower();
		return areaName.Contains("vest") || IsWebbingText(areaName);
	}

	protected bool IsWebbingText(string value)
	{
		if (value.IsEmpty())
			return false;

		value.ToLower();
		return value.Contains("webbing") || value.Contains("belt") || value.Contains("alice") || value.Contains("lbe") || value.Contains("harness") || value.Contains("chest_rig") || value.Contains("chest rig") || value.Contains("load bearing") || value.Contains("load_bearing") || value.Contains("grenadier vest");
	}

	protected string ResolveFocusForCategory(string category)
	{
		if (category == "headgear")
			return "head";
		if (category == "backpack")
			return "back";
		if (category == "boots")
			return "feet";
		if (category == "weapon" || category == "sidearm" || category == "attachment")
			return "weapon";
		if (category == "magazine")
			return "ammo";

		return "torso";
	}

	protected string ResolveAttachmentSlotKeyFromSlot(InventoryStorageSlot slot, AttachmentSlotComponent attachmentSlot, IEntity attachedEntity = null)
	{
		if (attachmentSlot && attachmentSlot.GetAttachmentSlotType())
		{
			typename attachmentType = attachmentSlot.GetAttachmentSlotType().Type();
			if (attachmentType.IsInherited(AttachmentOptics))
				return "optic";
			if (attachmentType.IsInherited(AttachmentUnderBarrel))
				return "underbarrel";
			if (attachmentType.IsInherited(AttachmentBayonet))
				return "bayonet";
			if (attachmentType.IsInherited(AttachmentMuzzle))
				return "muzzle";
			if (attachmentType.IsInherited(AttachmentHandGuard))
				return "handguard";
		}

		string key;
		if (slot)
			key = slot.GetSourceName();
		if (key.IsEmpty() && attachmentSlot && attachmentSlot.GetAttachmentSlotType())
			key = string.Format("%1", attachmentSlot.GetAttachmentSlotType());
		if (attachedEntity)
		{
			string attachedPrefab = ResolveEntityPrefab(attachedEntity);
			string attachedDisplay = ResolveEntityDisplayName(attachedEntity, attachedPrefab);
			key = key + " " + attachedPrefab + " " + attachedDisplay;
		}
		key.ToLower();

		if (key.Contains("optic") || key.Contains("scope") || key.Contains("sight"))
			return "optic";
		if (key.Contains("muzzle") || key.Contains("suppressor") || key.Contains("flash"))
			return "muzzle";
		if (key.Contains("under") || key.Contains("grip") || key.Contains("bipod"))
			return "underbarrel";
		if (key.Contains("bayonet"))
			return "bayonet";
		if (key.Contains("handguard"))
			return "handguard";
		if (key.Contains("stock") || key.Contains("butt"))
			return "stock";
		if (key.Contains("rail") || key.Contains("accessory"))
			return "rail";
		if (key.Contains("ammo") || key.Contains("magazine"))
			return "magazine";
		if (key.Contains("equipment"))
			return "attachment";

		return "attachment";
	}

	protected string ResolveAttachmentSlotLabel(string slotKey, string fallback)
	{
		if (slotKey == "optic")
			return "Optic";
		if (slotKey == "muzzle")
			return "Muzzle";
		if (slotKey == "underbarrel")
			return "Underbarrel";
		if (slotKey == "bayonet")
			return "Bayonet";
		if (slotKey == "handguard")
			return "Handguard";
		if (slotKey == "stock")
			return "Stock";
		if (slotKey == "magazine")
			return "Magazine";
		if (slotKey == "rail")
			return "Accessory Rail";
		if (slotKey == "equipment")
			return "Attachment Slot";
		if (!fallback.IsEmpty() && !IsLikelyRawSlotIdentifier(fallback) && !IsGenericAttachmentSlotLabel(fallback))
			return fallback;

		return "Attachment Slot";
	}

	protected bool IsGenericAttachmentSlotLabel(string value)
	{
		value = value.Trim();
		value.ToLower();
		return value == "attachment" || value == "attachment slot" || value == "equipment" || value == "equipment slot";
	}

	protected bool IsLikelyRawSlotIdentifier(string value)
	{
		value = value.Trim();
		if (value.Length() < 10)
			return false;

		int hexLike;
		for (int i = 0; i < value.Length(); i++)
		{
			string c = value.Substring(i, 1);
			if ("0123456789abcdefABCDEF".Contains(c))
				hexLike++;
		}

		return hexLike >= value.Length() - 1;
	}

	protected bool ReserveArsenalItemForEditor(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, string prefab, int count, out string failure)
	{
		failure = "";
		if (!state || !arsenal || prefab.IsEmpty() || count <= 0)
		{
			failure = "arsenal reserve context missing";
			return false;
		}

		HST_ArsenalItemState item = state.FindArsenalItem(prefab);
		if (!IsArsenalItemAvailable(item))
		{
			failure = "item is not available in the HST arsenal";
			return false;
		}

		string category = ResolveEditorCategory(item.m_sPrefab, item.m_sCategory);
		string displayName = HST_DisplayNameService.ResolveItemDisplayName(null, item.m_sPrefab, item.m_sDisplayName);
		if (!item.m_bUnlocked && !arsenal.WithdrawItem(state, prefab, count))
		{
			failure = string.Format("%1 unavailable; only %2 remain", displayName, item.m_iCount);
			return false;
		}

		SetIssuedItem(state, identityId, prefab, displayName, category, CountIssuedItem(state, identityId, prefab) + count, item.m_bUnlocked);
		return true;
	}

	protected void RefundReservedItem(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, string prefab, int count)
	{
		if (!state || !arsenal || identityId.IsEmpty() || prefab.IsEmpty() || count <= 0)
			return;

		HST_IssuedLoadoutItemState issuedItem = state.FindIssuedLoadoutItem(identityId, prefab);
		if (!issuedItem)
			return;

		int refundCount = Math.Min(count, issuedItem.m_iCount);
		issuedItem.m_iCount -= refundCount;
		if (!issuedItem.m_bInfinite && refundCount > 0)
			arsenal.RefundItem(state, issuedItem.m_sItemPrefab, refundCount, issuedItem.m_sCategory, issuedItem.m_sDisplayName);

		if (issuedItem.m_iCount <= 0)
			RemoveIssuedItemState(state, issuedItem);
	}

	void CompleteInsertTransaction(HST_LoadoutEditorInsertCallback callback, bool success)
	{
		if (!callback)
			return;

		if (success)
		{
			if (callback.m_bAccountRemovedOnComplete)
				ApplyRemovedEntityLedger(callback.m_State, callback.m_Arsenal, callback.m_sIdentityId, callback.m_aRemovedPrefabs, callback.m_aRemovedCategories, callback.m_aRemovedDisplayNames);
			ClearLoadoutEditorFailure(callback.m_State, callback.m_sIdentityId);
			return;
		}

		RecordLoadoutEditorFailure(callback.m_State, callback.m_sIdentityId, "Inventory Full");

		if (callback.m_TemporaryEntity)
			SCR_EntityHelper.DeleteEntityAndChildren(callback.m_TemporaryEntity);

		if (callback.m_bRefundReservedOnFailure)
			RefundReservedItem(callback.m_State, callback.m_Arsenal, callback.m_sIdentityId, callback.m_sReservedPrefab, 1);

		if (callback.m_bAccountRemovedOnFailure)
		{
			ApplyRemovedEntityLedger(callback.m_State, callback.m_Arsenal, callback.m_sIdentityId, callback.m_aRemovedPrefabs, callback.m_aRemovedCategories, callback.m_aRemovedDisplayNames);
			return;
		}

		if (callback.m_bRestoreRemovedOnFailure && !TryRestoreRemovedPrefab(callback))
			ApplyRemovedEntityLedger(callback.m_State, callback.m_Arsenal, callback.m_sIdentityId, callback.m_aRemovedPrefabs, callback.m_aRemovedCategories, callback.m_aRemovedDisplayNames);
	}

	protected void RecordLoadoutEditorFailure(HST_CampaignState state, string identityId, string failure)
	{
		if (!state || failure.IsEmpty())
			return;

		state.m_sLastLoadoutEditorFailure = failure;
		if (identityId.IsEmpty())
			return;

		HST_LoadoutEditorSessionState session = state.FindLoadoutEditorSession(identityId);
		if (session)
			session.m_sLastFailure = failure;
	}

	protected void ClearLoadoutEditorFailure(HST_CampaignState state, string identityId)
	{
		if (!state)
			return;

		state.m_sLastLoadoutEditorFailure = "";
		if (identityId.IsEmpty())
			return;

		HST_LoadoutEditorSessionState session = state.FindLoadoutEditorSession(identityId);
		if (session)
			session.m_sLastFailure = "";
	}

	protected void ConfigureInsertCallback(HST_LoadoutEditorInsertCallback callback, HST_CampaignState state, HST_ArsenalService arsenal, string identityId, string reservedPrefab)
	{
		if (!callback)
			return;

		callback.m_Service = this;
		callback.m_State = state;
		callback.m_Arsenal = arsenal;
		callback.m_sIdentityId = identityId;
		callback.m_sReservedPrefab = reservedPrefab;
		callback.m_bRefundReservedOnFailure = state != null && arsenal != null && !identityId.IsEmpty() && !reservedPrefab.IsEmpty();
	}

	protected bool TryRestoreRemovedPrefab(HST_LoadoutEditorInsertCallback callback)
	{
		if (!callback || !callback.m_Inventory || !callback.m_PlayerEntity || !callback.m_RestoreStorage || callback.m_sRestorePrefab.IsEmpty())
			return false;

		ResourceName resourceName = callback.m_sRestorePrefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded)
			return false;

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = callback.m_PlayerEntity.GetOrigin();
		IEntity itemEntity = GetGame().SpawnEntityPrefabEx(resourceName, false, GetGame().GetWorld(), params);
		if (!itemEntity)
			return false;

		HST_LoadoutEditorInsertCallback restoreCallback = new HST_LoadoutEditorInsertCallback();
		restoreCallback.m_Service = this;
		restoreCallback.m_State = callback.m_State;
		restoreCallback.m_Arsenal = callback.m_Arsenal;
		restoreCallback.m_sIdentityId = callback.m_sIdentityId;
		restoreCallback.m_bAccountRemovedOnFailure = true;
		CopyStringArray(callback.m_aRemovedPrefabs, restoreCallback.m_aRemovedPrefabs);
		CopyStringArray(callback.m_aRemovedCategories, restoreCallback.m_aRemovedCategories);
		CopyStringArray(callback.m_aRemovedDisplayNames, restoreCallback.m_aRemovedDisplayNames);
		restoreCallback.m_TemporaryEntity = itemEntity;
		callback.m_Inventory.TryInsertItemInStorage(itemEntity, callback.m_RestoreStorage, callback.m_iRestoreSlotId, restoreCallback);
		return true;
	}

	protected void CopyStringArray(array<string> source, notnull array<string> target)
	{
		if (!source)
			return;

		foreach (string value : source)
			target.Insert(value);
	}

	protected void AccountRemovedLiveEntity(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, IEntity entity)
	{
		if (!state || !arsenal || !entity)
			return;

		array<string> prefabs = {};
		array<string> categories = {};
		array<string> displayNames = {};
		BuildRemovedEntityLedger(entity, prefabs, categories, displayNames);
		ApplyRemovedEntityLedger(state, arsenal, identityId, prefabs, categories, displayNames);
	}

	protected void BuildRemovedEntityLedger(IEntity entity, notnull array<string> prefabs, notnull array<string> categories, notnull array<string> displayNames)
	{
		array<IEntity> visited = {};
		BuildRemovedEntityLedgerRecursive(entity, prefabs, categories, displayNames, visited, 0);
	}

	protected void BuildRemovedEntityLedgerRecursive(IEntity entity, notnull array<string> prefabs, notnull array<string> categories, notnull array<string> displayNames, notnull array<IEntity> visited, int depth)
	{
		if (!entity || depth > 8 || visited.Find(entity) >= 0)
			return;

		visited.Insert(entity);
		array<BaseInventoryStorageComponent> storages = {};
		FindRefundableContentStorages(entity, storages);
		if (storages.Count() > 0)
		{
			array<IEntity> contents = {};
			array<IEntity> contentVisited = {};
			GatherStorageContentEntitiesFromStorages(storages, contents, contentVisited);
			foreach (IEntity content : contents)
				BuildRemovedEntityLedgerRecursive(content, prefabs, categories, displayNames, visited, depth + 1);
		}

		string prefab = ResolveEntityPrefab(entity);
		if (prefab.IsEmpty())
			return;

		prefabs.Insert(prefab);
		categories.Insert(ResolveEditorCategory(prefab, ResolveCategoryFromEntity(entity, prefab)));
		displayNames.Insert(ResolveEntityDisplayName(entity, prefab));
	}

	protected void ApplyRemovedEntityLedger(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, array<string> prefabs, array<string> categories, array<string> displayNames)
	{
		if (!state || !arsenal || !prefabs)
			return;

		for (int i = 0; i < prefabs.Count(); i++)
		{
			string prefab = prefabs[i];
			if (prefab.IsEmpty())
				continue;

			string category = "";
			if (categories && i < categories.Count())
				category = categories[i];

			string displayName = "";
			if (displayNames && i < displayNames.Count())
				displayName = displayNames[i];

			ApplyRemovedEntityLedgerItem(state, arsenal, identityId, prefab, category, displayName);
		}
	}

	protected void ApplyRemovedEntityLedgerItem(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, string prefab, string category, string displayName)
	{
		if (!state || !arsenal || prefab.IsEmpty())
			return;

		HST_IssuedLoadoutItemState issuedItem = state.FindIssuedLoadoutItem(identityId, prefab);
		if (issuedItem && issuedItem.m_iCount > 0)
		{
			RefundReservedItem(state, arsenal, identityId, prefab, 1);
			return;
		}

		if (category.IsEmpty())
			category = ResolveEditorCategory(prefab, "");
		if (displayName.IsEmpty())
			displayName = HST_DisplayNameService.ResolveItemDisplayName(null, prefab);
		arsenal.RefundItem(state, prefab, 1, category, displayName);
	}

	protected bool ApplyLiveNodeItem(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId, string nodeId, string itemPrefab, out string failure)
	{
		failure = "";
		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		SCR_InventoryStorageManagerComponent inventory = ResolveInventoryManager(playerEntity, failure);
		if (!inventory)
			return false;

		if (nodeId.IndexOf(NODE_STORAGE_PREFIX) == 0 || nodeId.IndexOf(NODE_STORAGE_ITEM_PREFIX) == 0)
			return AddItemToLiveStorage(inventory, playerEntity, nodeId, itemPrefab, failure, state, arsenal, identityId);

		BaseInventoryStorageComponent targetStorage;
		InventoryStorageSlot targetSlot;
		IEntity attachedEntity;
		if (!ResolveLiveSlotTarget(playerEntity, nodeId, targetStorage, targetSlot, attachedEntity, failure))
			return false;

		array<string> removedPrefabs = {};
		array<string> removedCategories = {};
		array<string> removedDisplayNames = {};
		string restorePrefab;
		if (attachedEntity)
		{
			restorePrefab = ResolveEntityPrefab(attachedEntity);
			BuildRemovedEntityLedger(attachedEntity, removedPrefabs, removedCategories, removedDisplayNames);
			if (!inventory.TryDeleteItem(attachedEntity))
			{
				failure = "failed to remove previous item";
				return false;
			}
		}

		return InsertPrefabIntoExactSlot(inventory, playerEntity, targetStorage, targetSlot, itemPrefab, failure, state, arsenal, identityId, removedPrefabs, removedCategories, removedDisplayNames, restorePrefab);
	}

	protected bool RemoveLiveNodeItem(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId, string nodeId, out string removedDisplay, out string failure)
	{
		removedDisplay = "";
		failure = "";
		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		SCR_InventoryStorageManagerComponent inventory = ResolveInventoryManager(playerEntity, failure);
		if (!inventory)
			return false;

		IEntity targetEntity;
		if (nodeId.IndexOf(NODE_STORAGE_ITEM_PREFIX) == 0)
		{
			if (!ResolveLiveStorageItemTarget(playerEntity, nodeId, targetEntity, failure))
				return false;
		}
		else
		{
			BaseInventoryStorageComponent targetStorage;
			InventoryStorageSlot targetSlot;
			if (!ResolveLiveSlotTarget(playerEntity, nodeId, targetStorage, targetSlot, targetEntity, failure))
				return false;
		}

		if (!targetEntity)
		{
			failure = "selected slot is already empty";
			return false;
		}

		string prefab = ResolveEntityPrefab(targetEntity);
		removedDisplay = ResolveEntityDisplayName(targetEntity, prefab);
		array<string> removedPrefabs = {};
		array<string> removedCategories = {};
		array<string> removedDisplayNames = {};
		BuildRemovedEntityLedger(targetEntity, removedPrefabs, removedCategories, removedDisplayNames);
		if (!inventory.TryDeleteItem(targetEntity))
		{
			failure = "failed to delete selected item";
			return false;
		}
		ApplyRemovedEntityLedger(state, arsenal, identityId, removedPrefabs, removedCategories, removedDisplayNames);

		return true;
	}

	protected int ClearLiveIssuedItems(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId, out string failure)
	{
		failure = "";
		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		SCR_InventoryStorageManagerComponent inventory = ResolveInventoryManager(playerEntity, failure);
		if (!inventory)
			return 0;

		array<IEntity> items = {};
		inventory.GetItems(items, EStoragePurpose.PURPOSE_ANY);
		int removed;
		foreach (IEntity item : items)
		{
			string prefab = ResolveEntityPrefab(item);
			if (prefab.IsEmpty() || CountIssuedItem(state, identityId, prefab) <= 0)
				continue;

			array<string> removedPrefabs = {};
			array<string> removedCategories = {};
			array<string> removedDisplayNames = {};
			BuildRemovedEntityLedger(item, removedPrefabs, removedCategories, removedDisplayNames);
			if (inventory.TryDeleteItem(item))
			{
				ApplyRemovedEntityLedger(state, arsenal, identityId, removedPrefabs, removedCategories, removedDisplayNames);
				removed++;
			}
		}

		return removed;
	}

	protected bool AddItemToLiveStorage(SCR_InventoryStorageManagerComponent inventory, IEntity playerEntity, string nodeId, string itemPrefab, out string failure, HST_CampaignState state = null, HST_ArsenalService arsenal = null, string identityId = "")
	{
		failure = "";
		ResourceName resourceName = itemPrefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded)
		{
			failure = "item resource failed to load";
			return false;
		}

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = playerEntity.GetOrigin();
		IEntity itemEntity = GetGame().SpawnEntityPrefabEx(resourceName, false, GetGame().GetWorld(), params);
		if (!itemEntity)
		{
			failure = "item entity failed to spawn";
			return false;
		}

		ClearSpawnedCargoStorageContents(itemEntity);

		array<BaseInventoryStorageComponent> storageTargets = {};
		if (ResolveLiveStorageTargets(playerEntity, nodeId, storageTargets, failure) <= 0)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(itemEntity);
			return false;
		}

		BaseInventoryStorageComponent targetStorage;
		foreach (BaseInventoryStorageComponent candidateStorage : storageTargets)
		{
			targetStorage = inventory.FindStorageForInsert(itemEntity, candidateStorage, EStoragePurpose.PURPOSE_ANY);
			if (targetStorage)
				break;
		}

		if (!targetStorage)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(itemEntity);
			failure = "Inventory Full";
			return false;
		}

		HST_LoadoutEditorInsertCallback callback = new HST_LoadoutEditorInsertCallback();
		ConfigureInsertCallback(callback, state, arsenal, identityId, itemPrefab);
		callback.m_TemporaryEntity = itemEntity;
		inventory.TryInsertItemInStorage(itemEntity, targetStorage, -1, callback);
		return true;
	}

	protected bool InsertPrefabIntoExactSlot(SCR_InventoryStorageManagerComponent inventory, IEntity playerEntity, BaseInventoryStorageComponent storage, InventoryStorageSlot slot, string itemPrefab, out string failure, HST_CampaignState state = null, HST_ArsenalService arsenal = null, string identityId = "", array<string> removedPrefabs = null, array<string> removedCategories = null, array<string> removedDisplayNames = null, string restorePrefab = "")
	{
		failure = "";
		if (!inventory || !playerEntity || !storage || !slot)
		{
			failure = "target slot unavailable";
			return false;
		}

		ResourceName resourceName = itemPrefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded)
		{
			failure = "item resource failed to load";
			return false;
		}

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = playerEntity.GetOrigin();
		IEntity itemEntity = GetGame().SpawnEntityPrefabEx(resourceName, false, GetGame().GetWorld(), params);
		if (!itemEntity)
		{
			failure = "item entity failed to spawn";
			return false;
		}

		ClearSpawnedCargoStorageContents(itemEntity);

		HST_LoadoutEditorInsertCallback callback = new HST_LoadoutEditorInsertCallback();
		ConfigureInsertCallback(callback, state, arsenal, identityId, itemPrefab);
		callback.m_Inventory = inventory;
		callback.m_PlayerEntity = playerEntity;
		callback.m_RestoreStorage = storage;
		callback.m_iRestoreSlotId = slot.GetID();
		callback.m_sRestorePrefab = restorePrefab;
		callback.m_bAccountRemovedOnComplete = removedPrefabs != null && removedPrefabs.Count() > 0;
		callback.m_bRestoreRemovedOnFailure = !restorePrefab.IsEmpty();
		if (removedPrefabs)
			CopyStringArray(removedPrefabs, callback.m_aRemovedPrefabs);
		if (removedCategories)
			CopyStringArray(removedCategories, callback.m_aRemovedCategories);
		if (removedDisplayNames)
			CopyStringArray(removedDisplayNames, callback.m_aRemovedDisplayNames);
		callback.m_TemporaryEntity = itemEntity;
		inventory.TryInsertItemInStorage(itemEntity, storage, slot.GetID(), callback);
		return true;
	}

	protected int ClearSpawnedCargoStorageContents(IEntity itemEntity)
	{
		if (!itemEntity)
			return 0;

		array<BaseInventoryStorageComponent> storages = {};
		if (FindCargoDepositStorages(itemEntity, storages) <= 0)
			return 0;

		array<IEntity> contents = {};
		array<IEntity> visited = {};
		GatherStorageContentEntitiesFromStorages(storages, contents, visited);
		int removed;
		foreach (IEntity content : contents)
		{
			if (!content || content == itemEntity)
				continue;

			SCR_EntityHelper.DeleteEntityAndChildren(content);
			removed++;
		}

		return removed;
	}

	protected SCR_InventoryStorageManagerComponent ResolveInventoryManager(IEntity playerEntity, out string failure)
	{
		failure = "";
		if (!playerEntity)
		{
			failure = "no live player entity";
			return null;
		}

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			failure = "player inventory manager not available";

		return inventory;
	}

	protected void RemoveIssuedItemState(HST_CampaignState state, HST_IssuedLoadoutItemState target)
	{
		if (!state || !target)
			return;

		for (int i = state.m_aIssuedLoadoutItems.Count() - 1; i >= 0; i--)
		{
			if (state.m_aIssuedLoadoutItems[i] == target)
			{
				state.m_aIssuedLoadoutItems.Remove(i);
				return;
			}
		}
	}

	protected bool ResolveLiveSlotTarget(IEntity playerEntity, string nodeId, out BaseInventoryStorageComponent storage, out InventoryStorageSlot slot, out IEntity attachedEntity, out string failure)
	{
		storage = null;
		slot = null;
		attachedEntity = null;
		failure = "";
		if (!playerEntity || nodeId.IsEmpty())
		{
			failure = "missing live slot target";
			return false;
		}

		if (nodeId.IndexOf(NODE_LOADOUT_PREFIX) == 0)
		{
			int slotIndex = ParseSingleNodeIndex(nodeId, NODE_LOADOUT_PREFIX);
			SCR_CharacterInventoryStorageComponent characterStorage = SCR_CharacterInventoryStorageComponent.Cast(playerEntity.FindComponent(SCR_CharacterInventoryStorageComponent));
			if (!characterStorage || slotIndex < 0 || slotIndex >= characterStorage.GetSlotsCount())
			{
				failure = "clothing slot not found";
				return false;
			}

			storage = characterStorage;
			slot = characterStorage.GetSlot(slotIndex);
			attachedEntity = slot.GetAttachedEntity();
			return true;
		}

		if (nodeId.IndexOf(NODE_WEAPON_PREFIX) == 0)
		{
			int storageIndex;
			int slotIndex;
			if (!ParseTwoNodeIndexes(nodeId, NODE_WEAPON_PREFIX, storageIndex, slotIndex))
			{
				failure = "weapon slot id malformed";
				return false;
			}

			array<BaseInventoryStorageComponent> storages = {};
			FindInventoryStoragesWithSlots(playerEntity, storages);
			if (storageIndex < 0 || storageIndex >= storages.Count())
			{
				failure = "weapon storage not found";
				return false;
			}

			storage = storages[storageIndex];
			if (!storage || slotIndex < 0 || slotIndex >= storage.GetSlotsCount())
			{
				failure = "weapon slot not found";
				return false;
			}

			slot = storage.GetSlot(slotIndex);
			attachedEntity = slot.GetAttachedEntity();
			return true;
		}

		if (nodeId.IndexOf(NODE_CLOTHING_ATTACHMENT_PREFIX) == 0)
		{
			int loadoutSlotIndex;
			int attachmentStorageIndex;
			int attachmentSlotIndex;
			if (!ParseThreeNodeIndexes(nodeId, NODE_CLOTHING_ATTACHMENT_PREFIX, loadoutSlotIndex, attachmentStorageIndex, attachmentSlotIndex))
			{
				failure = "clothing attachment slot id malformed";
				return false;
			}

			SCR_CharacterInventoryStorageComponent characterStorage = SCR_CharacterInventoryStorageComponent.Cast(playerEntity.FindComponent(SCR_CharacterInventoryStorageComponent));
			if (!characterStorage || loadoutSlotIndex < 0 || loadoutSlotIndex >= characterStorage.GetSlotsCount())
			{
				failure = "clothing slot not found";
				return false;
			}

			IEntity clothingEntity = characterStorage.GetSlot(loadoutSlotIndex).GetAttachedEntity();
			if (!clothingEntity)
			{
				failure = "clothing slot is empty";
				return false;
			}

			array<BaseInventoryStorageComponent> attachmentStorages = {};
			FindEditableAttachmentStorages(clothingEntity, attachmentStorages);
			if (attachmentStorageIndex < 0 || attachmentStorageIndex >= attachmentStorages.Count())
			{
				failure = "clothing attachment storage not found";
				return false;
			}

			BaseInventoryStorageComponent attachmentStorage = attachmentStorages[attachmentStorageIndex];
			if (!attachmentStorage || attachmentSlotIndex < 0 || attachmentSlotIndex >= attachmentStorage.GetSlotsCount())
			{
				failure = "clothing attachment slot not found";
				return false;
			}

			storage = attachmentStorage;
			slot = attachmentStorage.GetSlot(attachmentSlotIndex);
			attachedEntity = slot.GetAttachedEntity();
			return true;
		}

		if (nodeId.IndexOf(NODE_ATTACHMENT_PREFIX) == 0)
		{
			int weaponStorageIndex;
			int weaponSlotIndex;
			int attachmentSlotIndex;
			if (!ParseThreeNodeIndexes(nodeId, NODE_ATTACHMENT_PREFIX, weaponStorageIndex, weaponSlotIndex, attachmentSlotIndex))
			{
				failure = "attachment slot id malformed";
				return false;
			}

			array<BaseInventoryStorageComponent> storages = {};
			FindInventoryStoragesWithSlots(playerEntity, storages);
			if (weaponStorageIndex < 0 || weaponStorageIndex >= storages.Count())
			{
				failure = "weapon storage not found";
				return false;
			}

			BaseInventoryStorageComponent weaponStorage = storages[weaponStorageIndex];
			if (!weaponStorage || weaponSlotIndex < 0 || weaponSlotIndex >= weaponStorage.GetSlotsCount())
			{
				failure = "weapon slot not found";
				return false;
			}

			IEntity weaponEntity = weaponStorage.GetSlot(weaponSlotIndex).GetAttachedEntity();
			if (!weaponEntity)
			{
				failure = "weapon slot is empty";
				return false;
			}

			SCR_WeaponAttachmentsStorageComponent attachmentStorage = SCR_WeaponAttachmentsStorageComponent.Cast(weaponEntity.FindComponent(SCR_WeaponAttachmentsStorageComponent));
			if (!attachmentStorage || attachmentSlotIndex < 0 || attachmentSlotIndex >= attachmentStorage.GetSlotsCount())
			{
				failure = "attachment storage not found";
				return false;
			}

			storage = attachmentStorage;
			slot = attachmentStorage.GetSlot(attachmentSlotIndex);
			attachedEntity = slot.GetAttachedEntity();
			return true;
		}

		failure = "unknown node target";
		return false;
	}

	protected BaseInventoryStorageComponent ResolveLiveStorageTarget(IEntity playerEntity, string nodeId, out string failure)
	{
		array<BaseInventoryStorageComponent> storages = {};
		if (ResolveLiveStorageTargets(playerEntity, nodeId, storages, failure) <= 0)
			return null;

		return storages[0];
	}

	protected int ResolveLiveStorageTargets(IEntity playerEntity, string nodeId, notnull array<BaseInventoryStorageComponent> outStorages, out string failure)
	{
		IEntity containerEntity = ResolveLiveStorageContainerEntity(playerEntity, nodeId, failure);
		if (!containerEntity)
			return 0;

		if (FindStorageInsertTargetStorages(containerEntity, outStorages) <= 0)
		{
			failure = "selected item has no usable storage";
			return 0;
		}

		return outStorages.Count();
	}

	protected int ResolveLiveStorageContentTargets(IEntity playerEntity, string nodeId, notnull array<BaseInventoryStorageComponent> outStorages, out string failure)
	{
		IEntity containerEntity = ResolveLiveStorageContainerEntity(playerEntity, nodeId, failure);
		if (!containerEntity)
			return 0;

		if (FindStorageInsertTargetStorages(containerEntity, outStorages) <= 0)
		{
			failure = "selected item has no stored cargo";
			return 0;
		}

		return outStorages.Count();
	}

	protected IEntity ResolveLiveStorageContainerEntity(IEntity playerEntity, string nodeId, out string failure)
	{
		failure = "";
		if (!playerEntity || nodeId.IsEmpty())
		{
			failure = "missing storage target";
			return null;
		}

		int containerSlotIndex = -1;
		if (nodeId.IndexOf(NODE_STORAGE_ITEM_PREFIX) == 0)
			containerSlotIndex = ParseSingleNodeIndex(nodeId, NODE_STORAGE_ITEM_PREFIX);
		else if (nodeId.IndexOf(NODE_STORAGE_PREFIX) == 0)
			containerSlotIndex = ParseSingleNodeIndex(nodeId, NODE_STORAGE_PREFIX);

		if (containerSlotIndex < 0)
		{
			failure = "storage target id malformed";
			return null;
		}

		SCR_CharacterInventoryStorageComponent characterStorage = SCR_CharacterInventoryStorageComponent.Cast(playerEntity.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!characterStorage || containerSlotIndex >= characterStorage.GetSlotsCount())
		{
			failure = "storage container slot not found";
			return null;
		}

		IEntity containerEntity = characterStorage.GetSlot(containerSlotIndex).GetAttachedEntity();
		if (!containerEntity)
		{
			failure = "selected storage container is empty";
			return null;
		}

		return containerEntity;
	}

	protected bool ResolveLiveStorageItemTarget(IEntity playerEntity, string nodeId, out IEntity targetEntity, out string failure)
	{
		targetEntity = null;
		failure = "";
		int containerSlotIndex;
		int itemIndex;
		if (!ParseTwoNodeIndexes(nodeId, NODE_STORAGE_ITEM_PREFIX, containerSlotIndex, itemIndex))
		{
			failure = "storage item id malformed";
			return false;
		}

		array<BaseInventoryStorageComponent> storages = {};
		if (ResolveLiveStorageContentTargets(playerEntity, NODE_STORAGE_PREFIX + string.Format("%1", containerSlotIndex), storages, failure) <= 0)
			return false;

		array<IEntity> contents = {};
		array<IEntity> visited = {};
		GatherStorageContentEntitiesFromStorages(storages, contents, visited);
		if (itemIndex < 0 || itemIndex >= contents.Count())
		{
			failure = "stored item not found";
			return false;
		}

		targetEntity = contents[itemIndex];
		return targetEntity != null;
	}

	protected bool IsLiveCandidateCompatible(HST_CampaignState state, int playerId, HST_LoadoutNodeState node, string prefab, string category, out bool ammoMatch)
	{
		ammoMatch = false;
		if (!node || prefab.IsEmpty())
			return false;

		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		if (!playerEntity)
			return false;

		IEntity temp = SpawnTemporaryItem(prefab, playerEntity.GetOrigin());
		if (!temp)
			return false;

		ClearSpawnedCargoStorageContents(temp);

		bool compatible = false;
		if (node.m_sKind == "slot")
			compatible = IsCandidateCompatibleWithLoadoutSlot(playerEntity, node, temp, category);
		else if (node.m_sKind == "weapon_slot")
			compatible = IsCandidateCompatibleWithWeaponSlot(playerEntity, node, temp, prefab);
		else if (node.m_sKind == "attachment")
			compatible = IsCandidateCompatibleWithAttachmentSlot(playerEntity, node.m_sNodeId, temp);
		else if (node.m_sKind == "storage" || node.m_sKind == "storage_item")
			compatible = IsCandidateCompatibleWithStorage(playerEntity, node.m_sNodeId, temp);

		if (category == "magazine")
			ammoMatch = IsMagazineCompatibleWithEquippedWeapons(playerEntity, temp);

		SCR_EntityHelper.DeleteEntityAndChildren(temp);
		return compatible;
	}

	protected bool IsCandidateCompatibleWithLoadoutSlot(IEntity playerEntity, HST_LoadoutNodeState node, IEntity temp, string category)
	{
		if (!node)
			return false;

		BaseInventoryStorageComponent storage;
		InventoryStorageSlot slot;
		IEntity attached;
		string failure;
		if (!ResolveLiveSlotTarget(playerEntity, node.m_sNodeId, storage, slot, attached, failure))
			return false;

		LoadoutSlotInfo loadoutSlot = LoadoutSlotInfo.Cast(slot);
		if (!loadoutSlot || !loadoutSlot.GetAreaType())
			return false;

		if (node && IsLoadoutClothingCategory(node.m_sCategory) && node.m_sCategory == category)
			return true;

		BaseLoadoutClothComponent cloth = BaseLoadoutClothComponent.Cast(temp.FindComponent(BaseLoadoutClothComponent));
		if (!cloth || !cloth.GetAreaType())
			return false;

		return loadoutSlot.GetAreaType().Type().ToString() == cloth.GetAreaType().Type().ToString();
	}

	protected bool IsCandidateCompatibleWithWeaponSlot(IEntity playerEntity, HST_LoadoutNodeState node, IEntity temp, string prefab)
	{
		BaseWeaponComponent weapon = BaseWeaponComponent.Cast(temp.FindComponent(BaseWeaponComponent));
		if (!weapon)
			return false;

		string weaponType = weapon.GetWeaponSlotType();
		weaponType.ToLower();
		if (node.m_sCategory == "explosive")
		{
			if (weaponType != "grenade" && weaponType != "throwable" && !IsExplosiveOrThrowablePrefab(prefab))
				return false;

			return IsCandidateCompatibleWithExactSlot(playerEntity, node.m_sNodeId, temp);
		}

		if (node.m_sCategory == "sidearm")
		{
			if (weaponType != "secondary" && !IsSidearmPrefab(prefab))
				return false;

			return IsCandidateCompatibleWithExactSlot(playerEntity, node.m_sNodeId, temp);
		}

		if (weaponType == "secondary" || IsSidearmPrefab(prefab))
			return false;

		if (weaponType != "primary" && weaponType != "launcher" && !weaponType.IsEmpty())
			return false;

		return IsCandidateCompatibleWithExactSlot(playerEntity, node.m_sNodeId, temp);
	}

	protected bool IsCandidateCompatibleWithExactSlot(IEntity playerEntity, string nodeId, IEntity temp)
	{
		if (!InventoryItemComponent.Cast(temp.FindComponent(InventoryItemComponent)))
			return false;

		BaseInventoryStorageComponent storage;
		InventoryStorageSlot slot;
		IEntity attached;
		string failure;
		if (!ResolveLiveSlotTarget(playerEntity, nodeId, storage, slot, attached, failure))
			return false;

		if (!storage || !slot)
			return false;

		if (attached)
			return storage.CanReplaceItem(temp, slot.GetID());

		return storage.CanStoreItem(temp, slot.GetID());
	}

	protected bool IsCandidateCompatibleWithAttachmentSlot(IEntity playerEntity, string nodeId, IEntity temp)
	{
		BaseInventoryStorageComponent storage;
		InventoryStorageSlot slot;
		IEntity attached;
		string failure;
		if (!ResolveLiveSlotTarget(playerEntity, nodeId, storage, slot, attached, failure))
			return false;

		AttachmentSlotComponent attachmentSlot = AttachmentSlotComponent.Cast(slot.GetParentContainer());
		return attachmentSlot && attachmentSlot.CanSetAttachment(temp);
	}

	protected bool IsCandidateCompatibleWithStorage(IEntity playerEntity, string nodeId, IEntity temp)
	{
		if (!InventoryItemComponent.Cast(temp.FindComponent(InventoryItemComponent)))
			return false;

		string failure;
		array<BaseInventoryStorageComponent> storages = {};
		if (ResolveLiveStorageTargets(playerEntity, nodeId, storages, failure) <= 0)
			return false;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return false;

		foreach (BaseInventoryStorageComponent storage : storages)
		{
			BaseInventoryStorageComponent targetStorage = inventory.FindStorageForInsert(temp, storage, EStoragePurpose.PURPOSE_ANY);
			if (targetStorage)
				return true;
		}

		return false;
	}

	protected bool IsMagazineCompatibleWithEquippedWeapons(IEntity playerEntity, IEntity candidate)
	{
		MagazineComponent magazine = MagazineComponent.Cast(candidate.FindComponent(MagazineComponent));
		if (!magazine || !magazine.GetMagazineWell())
			return false;

		string candidateWell = magazine.GetMagazineWell().Type().ToString();
		ChimeraCharacter character = ChimeraCharacter.Cast(playerEntity);
		if (!character)
			return false;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return false;

		BaseWeaponManagerComponent weaponManager = controller.GetWeaponManagerComponent();
		if (!weaponManager)
			return false;

		array<IEntity> weapons = {};
		weaponManager.GetWeaponsList(weapons);
		foreach (IEntity weaponEntity : weapons)
		{
			BaseMuzzleComponent muzzle = BaseMuzzleComponent.Cast(weaponEntity.FindComponent(BaseMuzzleComponent));
			if (!muzzle || !muzzle.GetMagazineWell())
				continue;

			if (muzzle.GetMagazineWell().Type().ToString() == candidateWell)
				return true;
		}

		return false;
	}

	protected IEntity SpawnTemporaryItem(string prefab, vector origin)
	{
		ResourceName resourceName = prefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded)
			return null;

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = origin;
		return GetGame().SpawnEntityPrefabEx(resourceName, false, GetGame().GetWorld(), params);
	}

	protected bool ParseTwoNodeIndexes(string nodeId, string prefix, out int first, out int second)
	{
		first = -1;
		second = -1;
		if (nodeId.IndexOf(prefix) != 0)
			return false;

		string rest = nodeId.Substring(prefix.Length(), nodeId.Length() - prefix.Length());
		array<string> parts = {};
		rest.Split("_", parts, false);
		if (parts.Count() < 2)
			return false;

		first = parts[0].ToInt();
		second = parts[1].ToInt();
		return true;
	}

	protected bool ParseThreeNodeIndexes(string nodeId, string prefix, out int first, out int second, out int third)
	{
		first = -1;
		second = -1;
		third = -1;
		if (nodeId.IndexOf(prefix) != 0)
			return false;

		string rest = nodeId.Substring(prefix.Length(), nodeId.Length() - prefix.Length());
		array<string> parts = {};
		rest.Split("_", parts, false);
		if (parts.Count() < 3)
			return false;

		first = parts[0].ToInt();
		second = parts[1].ToInt();
		third = parts[2].ToInt();
		return true;
	}

	protected int ParseSingleNodeIndex(string nodeId, string prefix)
	{
		if (nodeId.IndexOf(prefix) != 0)
			return -1;

		string rest = nodeId.Substring(prefix.Length(), nodeId.Length() - prefix.Length());
		array<string> parts = {};
		rest.Split("_", parts, false);
		if (parts.Count() == 0)
			return -1;

		return parts[0].ToInt();
	}

	protected void EnsureDraftSlots(HST_CampaignState state, string identityId, HST_LoadoutEditorSessionState session)
	{
		if (!state || !session)
			return;

		PurgeRemovedExternalDraftSlots(session);
		if (session.m_aDraftSlots.Count() > 0)
			return;

		HST_SavedLoadoutState selected = SelectLoadout(state, identityId, session.m_sCurrentLoadoutId);
		if (selected)
		{
			foreach (HST_LoadoutSlotState selectedSlot : selected.m_aSlots)
			{
				if (IsAllowedLoadoutSlot(selectedSlot))
					session.m_aDraftSlots.Insert(CopySlot(selectedSlot));
			}
			return;
		}

		HST_SavedLoadoutState draft = new HST_SavedLoadoutState();
		BuildDraftSlotsFromIssued(state, identityId, draft);
		if (draft.m_aSlots.Count() == 0)
			BuildStarterDraftSlotsFromArsenal(state, draft);

		foreach (HST_LoadoutSlotState slot : draft.m_aSlots)
		{
			if (IsAllowedLoadoutSlot(slot))
				session.m_aDraftSlots.Insert(CopySlot(slot));
		}
	}

	protected HST_LoadoutSlotState CopySlot(HST_LoadoutSlotState source)
	{
		HST_LoadoutSlotState target = new HST_LoadoutSlotState();
		if (!source)
			return target;

		target.m_sSlotId = source.m_sSlotId;
		target.m_sItemPrefab = source.m_sItemPrefab;
		target.m_sDisplayName = source.m_sDisplayName;
		target.m_sCategory = source.m_sCategory;
		target.m_iQuantity = Math.Max(1, source.m_iQuantity);
		target.m_sWeaponSlotId = source.m_sWeaponSlotId;
		target.m_sAttachmentSlotId = source.m_sAttachmentSlotId;
		target.m_sParentSlotId = source.m_sParentSlotId;
		target.m_sStorageId = source.m_sStorageId;
		target.m_sSlotKind = source.m_sSlotKind;
		return target;
	}

	protected HST_LoadoutSlotState FindDraftSlotByPrefab(HST_LoadoutEditorSessionState session, string prefab)
	{
		if (!session || prefab.IsEmpty())
			return null;

		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (slot && slot.m_sItemPrefab == prefab)
				return slot;
		}

		return null;
	}

	protected int ResolveDraftMaxQuantity(HST_CampaignState state, string identityId, string itemPrefab)
	{
		if (!state || itemPrefab.IsEmpty())
			return 0;

		HST_ArsenalItemState arsenalItem = state.FindArsenalItem(itemPrefab);
		if (!arsenalItem)
			return 0;

		if (arsenalItem.m_bUnlocked)
			return 99;

		return Math.Max(0, arsenalItem.m_iCount) + CountIssuedFiniteItem(state, identityId, itemPrefab);
	}

	protected string BuildUniqueDraftSlotId(HST_LoadoutEditorSessionState session, string category)
	{
		if (!session)
			return BuildSlotId(category, 0);

		for (int index = 0; index < 256; index++)
		{
			string candidate = BuildSlotId(category, index);
			bool found;
			foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
			{
				if (slot && slot.m_sSlotId == candidate)
				{
					found = true;
					break;
				}
			}

			if (!found)
				return candidate;
		}

		return BuildSlotId(category, session.m_aDraftSlots.Count());
	}

	protected void EnsureFixedPersonalLoadoutSlots(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return;

		for (int slotIndex = 0; slotIndex < PERSONAL_LOADOUT_SLOT_COUNT; slotIndex++)
			FindOrCreateFixedLoadoutSlot(state, identityId, slotIndex);
	}

	protected HST_SavedLoadoutState FindOrCreateFixedLoadoutSlot(HST_CampaignState state, string identityId, int slotIndex)
	{
		string loadoutId = BuildFixedLoadoutId(slotIndex);
		HST_SavedLoadoutState existing = state.FindSavedLoadout(identityId, loadoutId);
		if (existing)
		{
			existing.m_iSlotIndex = slotIndex;
			return existing;
		}

		HST_SavedLoadoutState loadout = new HST_SavedLoadoutState();
		loadout.m_sOwnerIdentityId = identityId;
		loadout.m_sLoadoutId = loadoutId;
		loadout.m_iSlotIndex = slotIndex;
		loadout.m_sDisplayName = string.Format("Slot %1 - Empty", slotIndex + 1);
		state.m_aSavedLoadouts.Insert(loadout);
		return loadout;
	}

	protected string BuildFixedLoadoutId(int slotIndex)
	{
		int resolvedIndex = slotIndex;
		if (resolvedIndex < 0)
			resolvedIndex = 0;
		if (resolvedIndex >= PERSONAL_LOADOUT_SLOT_COUNT)
			resolvedIndex = PERSONAL_LOADOUT_SLOT_COUNT - 1;

		return "slot_" + string.Format("%1", resolvedIndex);
	}

	protected bool IsFixedLoadoutSlotEmpty(HST_SavedLoadoutState loadout)
	{
		if (!loadout)
			return true;

		return loadout.m_sSerializedLoadout.IsEmpty() && loadout.m_aSlots.Count() == 0;
	}

	protected void ClearFixedLoadoutSlot(HST_SavedLoadoutState loadout)
	{
		if (!loadout)
			return;

		int slotIndex = loadout.m_iSlotIndex;
		if (slotIndex < 0)
			slotIndex = ResolveFixedLoadoutIndex(loadout.m_sLoadoutId);

		loadout.m_sSerializedLoadout = "";
		loadout.m_sCharacterPrefab = "";
		loadout.m_sClothingSummary = "";
		loadout.m_sWeaponSummary = "";
		loadout.m_sRequiredItemsSummary = "";
		loadout.m_iUpdatedAtSecond = 0;
		loadout.m_aSlots.Clear();
		loadout.m_iSlotIndex = slotIndex;
		loadout.m_sDisplayName = string.Format("Slot %1 - Empty", slotIndex + 1);
	}

	protected int ResolveFixedLoadoutIndex(string loadoutId)
	{
		if (loadoutId.IndexOf("slot_") != 0)
			return -1;

		string slotText = loadoutId.Substring(5, loadoutId.Length() - 5);
		int slotIndex = slotText.ToInt();
		if (slotIndex < 0)
			return -1;
		if (slotIndex >= PERSONAL_LOADOUT_SLOT_COUNT)
			return -1;

		return slotIndex;
	}

	protected int ResolveSaveSlotIndex(HST_CampaignState state, string identityId, string selectedLoadoutId)
	{
		int selectedIndex = ResolveFixedLoadoutIndex(selectedLoadoutId);
		if (selectedIndex >= 0)
			return selectedIndex;

		for (int slotIndex = 0; slotIndex < PERSONAL_LOADOUT_SLOT_COUNT; slotIndex++)
		{
			HST_SavedLoadoutState loadout = state.FindSavedLoadout(identityId, BuildFixedLoadoutId(slotIndex));
			if (IsFixedLoadoutSlotEmpty(loadout))
				return slotIndex;
		}

		return 0;
	}

	protected int MigrateLegacyLoadoutsToFixedSlots(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return 0;

		int migrated;
		for (int existingIndex = 0; existingIndex < state.m_aSavedLoadouts.Count(); existingIndex++)
		{
			HST_SavedLoadoutState legacy = state.m_aSavedLoadouts[existingIndex];
			if (!legacy || legacy.m_sOwnerIdentityId != identityId || ResolveFixedLoadoutIndex(legacy.m_sLoadoutId) >= 0)
				continue;

			int targetIndex = ResolveSaveSlotIndex(state, identityId, "");
			HST_SavedLoadoutState target = FindOrCreateFixedLoadoutSlot(state, identityId, targetIndex);
			target.m_sDisplayName = legacy.m_sDisplayName;
			target.m_iUpdatedAtSecond = legacy.m_iUpdatedAtSecond;
			target.m_aSlots.Clear();
			foreach (HST_LoadoutSlotState legacySlot : legacy.m_aSlots)
				target.m_aSlots.Insert(CopySlot(legacySlot));
			BuildSavedLoadoutMetadata(target);
			migrated++;
		}

		for (int removeIndex = state.m_aSavedLoadouts.Count() - 1; removeIndex >= 0; removeIndex--)
		{
			HST_SavedLoadoutState loadout = state.m_aSavedLoadouts[removeIndex];
			if (loadout && loadout.m_sOwnerIdentityId == identityId && ResolveFixedLoadoutIndex(loadout.m_sLoadoutId) < 0)
				state.m_aSavedLoadouts.Remove(removeIndex);
		}

		return migrated;
	}

	protected bool SerializeCharacterLoadout(IEntity characterEntity, out string serialized)
	{
		serialized = "";
		if (!characterEntity)
			return false;

		GameEntity gameEntity = GameEntity.Cast(characterEntity);
		if (!gameEntity)
			return false;

		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(characterEntity, saveContext))
			return false;

		serialized = saveContext.ExportToString();
		return !serialized.IsEmpty();
	}

	protected bool ApplySerializedLoadoutToPlayerEntity(HST_SavedLoadoutState loadout, int playerId, out string failure)
	{
		failure = "";
		if (!loadout || loadout.m_sSerializedLoadout.IsEmpty())
		{
			failure = "saved slot has no serialized loadout";
			return false;
		}

		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		GameEntity gameEntity = GameEntity.Cast(playerEntity);
		if (!gameEntity)
		{
			failure = "live player entity is not a game entity";
			return false;
		}

		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		if (!loadContext.ImportFromString(loadout.m_sSerializedLoadout))
		{
			failure = "saved loadout data could not be read";
			return false;
		}

		if (!SCR_PlayerArsenalLoadout.ApplyLoadoutString(gameEntity, loadContext))
		{
			failure = "native loadout apply failed";
			return false;
		}

		gameEntity.Update();
		RefreshPlayerControllerMainEntity(playerId, gameEntity);
		return true;
	}

	protected void RefreshPlayerControllerMainEntity(int playerId, IEntity playerEntity)
	{
		if (!playerEntity)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || playerId <= 0)
			return;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(playerManager.GetPlayerController(playerId));
		if (!playerController)
			return;

		playerController.SetInitialMainEntity(playerEntity);
	}

	protected bool ParseLoadoutRenameArgument(string argument, out string loadoutId, out string loadoutName)
	{
		loadoutId = "";
		loadoutName = "";
		int split = argument.IndexOf(":");
		if (split <= 0 || split >= argument.Length() - 1)
			return false;

		loadoutId = argument.Substring(0, split).Trim();
		loadoutName = SanitizeLoadoutDisplayName(argument.Substring(split + 1, argument.Length() - split - 1));
		return !loadoutId.IsEmpty() && !loadoutName.IsEmpty();
	}

	protected string SanitizeLoadoutDisplayName(string value)
	{
		value = value.Trim();
		value.Replace("\n", " ");
		value.Replace("\r", " ");
		value.Replace("|", " ");
		value.Replace(":", " ");
		while (value.Contains("  "))
			value.Replace("  ", " ");
		if (value.Length() > 48)
			value = value.Substring(0, 48).Trim();

		return value;
	}

	protected void BuildSavedLoadoutMetadata(HST_SavedLoadoutState loadout)
	{
		if (!loadout)
			return;

		array<string> clothes = {};
		array<string> weapons = {};
		foreach (HST_LoadoutSlotState slot : loadout.m_aSlots)
		{
			if (!slot)
				continue;

			string category = ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory);
			if (category == "headgear" || category == "clothing" || category == "vest" || category == "webbing" || category == "pants" || category == "boots" || category == "backpack" || category == "handwear")
				clothes.Insert(slot.m_sDisplayName);
			else if (category == "weapon" || category == "sidearm" || category == "launcher")
				weapons.Insert(slot.m_sDisplayName);
		}

		loadout.m_sClothingSummary = SCR_StringHelper.Join(" / ", clothes, false);
		loadout.m_sWeaponSummary = SCR_StringHelper.Join(" / ", weapons, false);
		loadout.m_sRequiredItemsSummary = string.Format("%1 item(s)", loadout.m_aSlots.Count());
	}

	protected HST_SavedLoadoutState SelectLoadoutForApply(HST_CampaignState state, string identityId, string loadoutId)
	{
		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		EnsureFixedPersonalLoadoutSlots(state, identityId);
		if (!loadoutId.IsEmpty())
			return SelectLoadout(state, identityId, loadoutId);
		if (!session.m_sCurrentLoadoutId.IsEmpty())
			return SelectLoadout(state, identityId, session.m_sCurrentLoadoutId);

		return SelectLoadout(state, identityId, loadoutId);
	}

	protected bool IsArsenalItemAvailable(HST_ArsenalItemState item)
	{
		if (!item || item.m_sPrefab.IsEmpty() || IsRemovedExternalItem(item.m_sPrefab, item.m_sDisplayName) || (!item.m_bUnlocked && item.m_iCount <= 0))
			return false;

		string category = ResolveEditorCategory(item.m_sPrefab, item.m_sCategory);
		return !HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(item.m_sPrefab, category) && !HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(item.m_sDisplayName, category);
	}

	protected int CountAvailableItemsInCategory(HST_CampaignState state, string categoryId)
	{
		if (!state || categoryId.IsEmpty())
			return 0;

		int count;
		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!IsArsenalItemAvailable(item))
				continue;

			if (ResolveEditorCategory(item.m_sPrefab, item.m_sCategory) == categoryId)
				count++;
		}

		return count;
	}

	protected int GetEditorCategoryCount()
	{
		return 15;
	}

	protected string GetEditorCategoryId(int index)
	{
		if (index == 0)
			return "headgear";
		if (index == 1)
			return "clothing";
		if (index == 2)
			return "vest";
		if (index == 3)
			return "webbing";
		if (index == 4)
			return "pants";
		if (index == 5)
			return "boots";
		if (index == 6)
			return "backpack";
		if (index == 7)
			return "handwear";
		if (index == 8)
			return "weapon";
		if (index == 9)
			return "sidearm";
		if (index == 10)
			return "magazine";
		if (index == 11)
			return "explosive";
		if (index == 12)
			return "attachment";
		if (index == 13)
			return "medical";

		return "utility";
	}

	protected string GetEditorCategoryLabel(string categoryId)
	{
		if (categoryId == "clothing")
			return "Clothing";
		if (categoryId == "headgear")
			return "Headgear";
		if (categoryId == "vest")
			return "Armored Vest";
		if (categoryId == "webbing")
			return "Chest Rig";
		if (categoryId == "pants")
			return "Pants";
		if (categoryId == "boots")
			return "Boots";
		if (categoryId == "backpack")
			return "Backpack";
		if (categoryId == "handwear")
			return "Handwear";
		if (categoryId == "weapon")
			return "Weapons";
		if (categoryId == "sidearm")
			return "Sidearms";
		if (categoryId == "magazine")
			return "Ammunition";
		if (categoryId == "explosive")
			return "Throwables";
		if (categoryId == "attachment")
			return "Attachments";
		if (categoryId == "medical")
			return "Medical";

		return "Equipment";
	}

	protected string BuildEditorItemPayload(string category, string prefab, string displayName, int count, string infiniteMarker)
	{
		string resolvedDisplay = HST_DisplayNameService.ResolveItemDisplayName(null, prefab, displayName);
		string shortDisplay = HST_DisplayNameService.ResolveShortItemDisplayName(resolvedDisplay, prefab);
		return string.Format("\nITEM|%1|%2|%3|%4|%5|%6|%7|%8|%9", category, prefab, SanitizePayloadField(resolvedDisplay), count, infiniteMarker, SanitizePayloadField(resolvedDisplay), SanitizePayloadField(shortDisplay), SanitizePayloadField(GetEditorSlotLabel(category)), IsPreviewEligibleCategory(category));
	}

	protected string BuildEditorSlotPayload(HST_LoadoutSlotState slot, string category, string displayName)
	{
		string resolvedDisplay = HST_DisplayNameService.ResolveItemDisplayName(null, slot.m_sItemPrefab, displayName);
		string shortDisplay = HST_DisplayNameService.ResolveShortItemDisplayName(resolvedDisplay, slot.m_sItemPrefab);
		return string.Format("\nSLOT|%1|%2|%3|%4|%5|%6|%7|%8|%9", slot.m_sSlotId, category, slot.m_sItemPrefab, SanitizePayloadField(resolvedDisplay), Math.Max(1, slot.m_iQuantity), SanitizePayloadField(resolvedDisplay), SanitizePayloadField(shortDisplay), SanitizePayloadField(GetEditorSlotLabel(category)), IsPreviewEligibleCategory(category));
	}

	protected string BuildEditorNodePayload(HST_LoadoutNodeState node, HST_CampaignState state)
	{
		string display = node.m_sDisplayName;
		if (!node.m_sItemPrefab.IsEmpty())
			display = HST_DisplayNameService.ResolveItemDisplayName(null, node.m_sItemPrefab, node.m_sDisplayName);
		if (display.IsEmpty())
			display = "Empty Slot";

		int count;
		bool infinite;
		ResolveArsenalCountForPrefab(state, node.m_sItemPrefab, count, infinite);
		if (node.m_sKind == "storage_item")
		{
			count = Math.Max(1, node.m_iQuantity);
			infinite = false;
		}
		string payload = string.Format("\nNODE|%1|%2|%3|%4|%5|%6|%7|%8|%9", node.m_sNodeId, node.m_sParentNodeId, node.m_sKind, SanitizePayloadField(node.m_sSlotKey), SanitizePayloadField(node.m_sLabel), node.m_sItemPrefab, SanitizePayloadField(display), count, infinite);
		payload = payload + string.Format("|%1|%2|%3|%4", node.m_bCanOpen, node.m_bCanRemove, node.m_bCanDeposit, node.m_sFocus);
		payload = payload + string.Format("|%1|%2", node.m_iUsedCapacity, node.m_iTotalCapacity);
		return payload;
	}

	protected string BuildCandidatePayloadsForNode(HST_CampaignState state, string identityId, HST_LoadoutEditorSessionState session, HST_LoadoutNodeState node, out int candidateCount, out string emptyReason, out int availableItems, out int categoryMatches, out int compatibilityMatches)
	{
		candidateCount = 0;
		emptyReason = "";
		availableItems = 0;
		categoryMatches = 0;
		compatibilityMatches = 0;
		if (!state || !node)
		{
			emptyReason = "Selected slot unavailable";
			return "";
		}

		string payload;
		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!IsArsenalItemAvailable(item))
				continue;

			availableItems++;
			string category = ResolveEditorCategory(item.m_sPrefab, item.m_sCategory);
			if (!IsCandidateCategoryForNode(node, category, item.m_sPrefab))
				continue;

			string display = HST_DisplayNameService.ResolveItemDisplayName(null, item.m_sPrefab, item.m_sDisplayName);
			string shortDisplay = HST_DisplayNameService.ResolveShortItemDisplayName(display, item.m_sPrefab);
			bool isStorageBrowserNode = node.m_sKind == "storage" || node.m_sKind == "storage_item";
			if (isStorageBrowserNode && IsBlockedStorageBrowserContainerCandidate(session.m_iPlayerId, item.m_sPrefab, display, shortDisplay, category))
				continue;

			categoryMatches++;
			bool ammoMatch;
			bool compatible = IsLiveCandidateCompatible(state, session.m_iPlayerId, node, item.m_sPrefab, category, ammoMatch);
			if (!isStorageBrowserNode && !compatible)
				continue;

			if (compatible)
				compatibilityMatches++;
			int availableCount;
			bool infiniteAvailable;
			ResolveArsenalCountForPrefab(state, item.m_sPrefab, availableCount, infiniteAvailable);

			string infinite = "";
			if (infiniteAvailable)
				infinite = "INF";

			payload = payload + string.Format("\nCANDIDATE|%1|%2|%3|%4|%5|%6|%7|%8|%9", node.m_sNodeId, item.m_sPrefab, SanitizePayloadField(display), SanitizePayloadField(shortDisplay), availableCount, infinite, category, compatible, SanitizePayloadField(BuildCandidateIconHint(category, item.m_sPrefab)));
			payload = payload + string.Format("|%1", ammoMatch);
			candidateCount++;
		}

		if (candidateCount <= 0)
		{
			if (availableItems <= 0)
				emptyReason = "No recovered arsenal items";
			else if (categoryMatches <= 0)
				emptyReason = "No arsenal items match this slot";
			else if (compatibilityMatches <= 0)
				emptyReason = "No compatible items for this slot";
			else
				emptyReason = "No compatible arsenal items";
		}

		return payload;
	}

	protected bool IsBlockedStorageBrowserContainerCandidate(int playerId, string prefab, string display, string shortDisplay, string category)
	{
		if (prefab.IsEmpty())
			return false;

		if (HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(prefab, category) || HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(display, category) || HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(shortDisplay, category))
			return true;

		return HST_ArsenalItemFilter.ShouldBlockArsenalPrefab(prefab, category, display);
	}

	protected bool ContainsStructuralStorageCandidateToken(string value)
	{
		if (value.IsEmpty())
			return false;

		value.ToLower();
		return HST_ArsenalItemFilter.HasBlockedStructuralContainerToken(value);
	}

	protected void RefreshDraftNodes(HST_CampaignState state, HST_LoadoutEditorSessionState session)
	{
		if (!session)
			return;

		if (session.m_bLiveCharacterAvailable && session.m_aDraftNodes.Count() > 0)
			return;

		session.m_aDraftNodes.Clear();
		AddLoadoutNodeForCategory(session, "headgear", "Headgear", "Headgear", "head");
		AddLoadoutNodeForCategory(session, "clothing", "Jacket", "Jacket", "torso");
		AddLoadoutNodeForCategory(session, "vest", "ArmoredVest", "Armored Vest", "torso");
		AddLoadoutNodeForCategory(session, "webbing", "ChestRig", "Chest Rig", "torso");
		AddLoadoutNodeForCategory(session, "pants", "Pants", "Pants", "legs");
		AddLoadoutNodeForCategory(session, "boots", "Boots", "Boots", "feet");
		AddLoadoutNodeForCategory(session, "backpack", "Backpack", "Backpack", "back");
		AddLoadoutNodeForCategory(session, "handwear", "Handwear", "Handwear", "hands");
		AddLoadoutNodeForCategory(session, "weapon", "PrimaryWeapon", "Primary Weapon", "weapon");
		AddLoadoutNodeForCategory(session, "launcher", "SecondaryWeapon", "Secondary Weapon", "weapon");
		AddInventoryNodeRoot(session);
		AddInventoryNodesForCategory(session, "magazine", "Ammunition", "ammo");
		AddInventoryNodesForCategory(session, "explosive", "Throwables", "hands");
		AddInventoryNodesForCategory(session, "medical", "Medical", "torso");
		AddInventoryNodesForCategory(session, "utility", "Equipment", "torso");
		AddAttachmentNodes(session);
	}

	protected void AddLoadoutNodeForCategory(HST_LoadoutEditorSessionState session, string category, string nodeIdSuffix, string label, string focus)
	{
		HST_LoadoutSlotState slot = FindFirstDraftSlotInCategory(session, category);
		HST_LoadoutNodeState node = new HST_LoadoutNodeState();
		if (slot)
			node.m_sNodeId = "node_" + slot.m_sSlotId;
		else
			node.m_sNodeId = "empty_" + category;
		node.m_sKind = "slot";
		node.m_sSlotKey = category;
		node.m_sLabel = label;
		node.m_sCategory = category;
		node.m_sFocus = focus;
		node.m_bCanRemove = slot != null;
		node.m_bCanOpen = slot != null && (category == "vest" || category == "webbing" || category == "backpack" || category == "clothing");
		node.m_bCanDeposit = node.m_bCanOpen;
		node.m_iTotalCapacity = ResolveDisplayCapacityForCategory(category);
		if (slot)
			CopySlotToNode(slot, node);
		session.m_aDraftNodes.Insert(node);
	}

	protected void AddInventoryNodeRoot(HST_LoadoutEditorSessionState session)
	{
		HST_LoadoutNodeState node = new HST_LoadoutNodeState();
		node.m_sNodeId = "storage_inventory";
		node.m_sKind = "storage";
		node.m_sSlotKey = "inventory";
		node.m_sLabel = "Inventory";
		node.m_sDisplayName = "Open carried storage";
		node.m_sCategory = "utility";
		node.m_sFocus = "torso";
		node.m_bCanOpen = true;
		node.m_bCanDeposit = true;
		node.m_iUsedCapacity = CountInventoryDraftSlots(session);
		node.m_iTotalCapacity = Math.Max(12, node.m_iUsedCapacity);
		session.m_aDraftNodes.Insert(node);
	}

	protected void AddInventoryNodesForCategory(HST_LoadoutEditorSessionState session, string category, string label, string focus)
	{
		int emitted;
		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (!slot || IsNestedStorageDraftSlot(slot) || ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory) != category)
				continue;

			HST_LoadoutNodeState node = new HST_LoadoutNodeState();
			node.m_sNodeId = "node_" + slot.m_sSlotId;
			node.m_sParentNodeId = "storage_inventory";
			node.m_sKind = "storage_item";
			node.m_sSlotKey = category;
			node.m_sLabel = label;
			node.m_sCategory = category;
			node.m_sFocus = focus;
			node.m_bCanRemove = true;
			CopySlotToNode(slot, node);
			session.m_aDraftNodes.Insert(node);
			emitted++;
		}

		if (emitted == 0)
		{
			HST_LoadoutNodeState emptyNode = new HST_LoadoutNodeState();
			emptyNode.m_sNodeId = "empty_" + category;
			emptyNode.m_sParentNodeId = "storage_inventory";
			emptyNode.m_sKind = "storage_item";
			emptyNode.m_sSlotKey = category;
			emptyNode.m_sLabel = label;
			emptyNode.m_sCategory = category;
			emptyNode.m_sDisplayName = "Empty Slot";
			emptyNode.m_sFocus = focus;
			session.m_aDraftNodes.Insert(emptyNode);
		}
	}

	protected void AddAttachmentNodes(HST_LoadoutEditorSessionState session)
	{
		HST_LoadoutNodeState weaponNode = FindFirstNodeByCategory(session, "weapon");
		string parentId = "";
		if (weaponNode)
			parentId = weaponNode.m_sNodeId;

		AddAttachmentNode(session, parentId, "attach_optic", "optic", "Optic", "optic");
		AddAttachmentNode(session, parentId, "attach_muzzle", "muzzle", "Muzzle", "muzzle");
		AddAttachmentNode(session, parentId, "attach_underbarrel", "underbarrel", "Underbarrel", "weapon");
		AddAttachmentNode(session, parentId, "attach_bayonet", "bayonet", "Bayonet", "weapon");
		AddAttachmentNode(session, parentId, "attach_handguard", "handguard", "Handguard", "weapon");
	}

	protected void AddAttachmentNode(HST_LoadoutEditorSessionState session, string parentId, string nodeId, string slotKey, string label, string focus)
	{
		HST_LoadoutSlotState slot = FindAttachmentSlot(session, nodeId);
		HST_LoadoutNodeState node = new HST_LoadoutNodeState();
		if (slot)
			node.m_sNodeId = "node_" + slot.m_sSlotId;
		else
			node.m_sNodeId = nodeId;
		node.m_sParentNodeId = parentId;
		node.m_sKind = "attachment";
		node.m_sSlotKey = slotKey;
		node.m_sLabel = label;
		node.m_sCategory = "attachment";
		node.m_sFocus = focus;
		node.m_bCanRemove = slot != null;
		if (slot)
			CopySlotToNode(slot, node);
		else
			node.m_sDisplayName = "Attachment";
		session.m_aDraftNodes.Insert(node);
	}

	protected void CopySlotToNode(HST_LoadoutSlotState slot, HST_LoadoutNodeState node)
	{
		if (!slot || !node)
			return;

		node.m_sItemPrefab = slot.m_sItemPrefab;
		node.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, slot.m_sItemPrefab, slot.m_sDisplayName);
		node.m_sCategory = ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory);
		node.m_iQuantity = Math.Max(1, slot.m_iQuantity);
		node.m_iUsedCapacity = node.m_iQuantity;
	}

	protected HST_LoadoutSlotState FindFirstDraftSlotInCategory(HST_LoadoutEditorSessionState session, string category)
	{
		if (!session)
			return null;

		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (slot && ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory) == category)
				return slot;
		}

		return null;
	}

	protected HST_LoadoutSlotState FindAttachmentSlot(HST_LoadoutEditorSessionState session, string attachmentSlotId)
	{
		if (!session)
			return null;

		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (!slot)
				continue;

			if (slot.m_sAttachmentSlotId == attachmentSlotId)
				return slot;
		}

		return null;
	}

	protected HST_LoadoutNodeState FindFirstNodeByCategory(HST_LoadoutEditorSessionState session, string category)
	{
		if (!session)
			return null;

		foreach (HST_LoadoutNodeState node : session.m_aDraftNodes)
		{
			if (node && node.m_sCategory == category)
				return node;
		}

		return null;
	}

	protected bool IsCandidateCategoryForNode(HST_LoadoutNodeState node, string category, string prefab)
	{
		if (!node)
			return false;

		if (node.m_sKind == "attachment")
			return category == "attachment" && IsAttachmentCandidateForSlot(node.m_sSlotKey, prefab);

		if (node.m_sKind == "storage" || node.m_sKind == "storage_item")
			return IsStorageBrowserCandidateCategory(category);

		if (node.m_sSlotKey == "weapon")
			return category == "weapon" && IsPrimaryWeaponCandidate(prefab);
		if (node.m_sSlotKey == "launcher")
			return category == "launcher" && IsLauncherWeaponCandidate(prefab);

		return category == node.m_sCategory;
	}

	protected bool IsStorageBrowserCandidateCategory(string category)
	{
		return category == "magazine"
			|| category == "explosive"
			|| category == "medical"
			|| category == "utility"
			|| category == "weapon"
			|| category == "launcher"
			|| category == "sidearm";
	}

	protected void ResolveArsenalCountForPrefab(HST_CampaignState state, string prefab, out int count, out bool infinite)
	{
		count = 0;
		infinite = false;
		if (!state || prefab.IsEmpty())
			return;

		HST_ArsenalItemState item = state.FindArsenalItem(prefab);
		if (!item)
			return;

		count = item.m_iCount;
		infinite = item.m_bUnlocked;
	}

	protected string BuildCandidateIconHint(string category, string prefab)
	{
		if (category == "weapon" || category == "sidearm" || category == "launcher")
			return "weapon";
		if (category == "magazine")
			return "ammo";
		if (category == "attachment")
			return "wrench";
		if (category == "vest" || category == "webbing" || category == "backpack" || category == "clothing" || category == "headgear" || category == "pants" || category == "boots" || category == "handwear")
			return "clothing";
		if (category == "explosive")
			return "grenade";
		if (category == "medical")
			return "medical";
		if (category == "utility")
			return "utility";

		return "equipment";
	}

	protected int ResolveDisplayCapacityForCategory(string category)
	{
		if (category == "backpack")
			return 12;
		if (category == "vest")
			return 8;
		if (category == "webbing")
			return 8;
		if (category == "clothing")
			return 4;

		return 1;
	}

	protected int CountInventoryDraftSlots(HST_LoadoutEditorSessionState session)
	{
		if (!session)
			return 0;

		int count;
		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (!slot)
				continue;
			if (IsNestedStorageDraftSlot(slot))
				continue;

			string category = ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory);
			if (category == "magazine" || category == "explosive" || category == "medical" || category == "utility" || category == "attachment")
				count += Math.Max(1, slot.m_iQuantity);
		}

		return count;
	}

	protected bool IsNestedStorageDraftSlot(HST_LoadoutSlotState slot)
	{
		if (!slot)
			return false;
		if (slot.m_sSlotKind == "storage_item")
			return true;
		return !slot.m_sParentSlotId.IsEmpty();
	}

	protected string GetEditorSlotLabel(string categoryId)
	{
		if (categoryId == "clothing")
			return "Jacket";
		if (categoryId == "headgear")
			return "Headgear";
		if (categoryId == "vest")
			return "Armored Vest";
		if (categoryId == "webbing")
			return "Chest Rig";
		if (categoryId == "pants")
			return "Pants";
		if (categoryId == "boots")
			return "Boots";
		if (categoryId == "backpack")
			return "Backpack";
		if (categoryId == "handwear")
			return "Handwear";
		if (categoryId == "weapon")
			return "Weapon";
		if (categoryId == "sidearm")
			return "Sidearm";
		if (categoryId == "launcher")
			return "Launcher";
		if (categoryId == "magazine")
			return "Ammo";
		if (categoryId == "explosive")
			return "Explosive";
		if (categoryId == "attachment")
			return "Attachment";
		if (categoryId == "medical")
			return "Medical";

		return "Gear";
	}

	protected bool IsPreviewEligibleCategory(string categoryId)
	{
		return categoryId == "clothing" || categoryId == "headgear" || categoryId == "vest" || categoryId == "webbing" || categoryId == "pants" || categoryId == "boots" || categoryId == "backpack" || categoryId == "handwear" || categoryId == "weapon" || categoryId == "sidearm" || categoryId == "launcher" || categoryId == "attachment";
	}

	protected bool ParseSlotQuantityArgument(string argument, out string slotId, out int quantity)
	{
		slotId = "";
		quantity = 0;
		int separator = argument.IndexOf(":");
		if (separator <= 0 || separator + 1 >= argument.Length())
			return false;

		slotId = argument.Substring(0, separator);
		string quantityText = argument.Substring(separator + 1, argument.Length() - separator - 1);
		quantity = quantityText.ToInt();
		return !slotId.IsEmpty() && quantity > 0;
	}

	protected bool ParseSlotPrefabArgument(string argument, out string slotId, out string itemPrefab)
	{
		slotId = "";
		itemPrefab = "";
		int separator = argument.IndexOf(":");
		if (separator <= 0 || separator + 1 >= argument.Length())
			return false;

		slotId = argument.Substring(0, separator);
		itemPrefab = argument.Substring(separator + 1, argument.Length() - separator - 1);
		return !slotId.IsEmpty() && !itemPrefab.IsEmpty();
	}

	protected string ResolveDraftSlotIdFromNodeId(string nodeId)
	{
		if (nodeId.IsEmpty())
			return "";

		if (nodeId.IndexOf("node_") != 0)
			return "";

		return nodeId.Substring(5, nodeId.Length() - 5);
	}

	protected HST_LoadoutNodeState FindDraftNodeById(HST_LoadoutEditorSessionState session, string nodeId)
	{
		if (!session || nodeId.IsEmpty())
			return null;

		foreach (HST_LoadoutNodeState node : session.m_aDraftNodes)
		{
			if (node && node.m_sNodeId == nodeId)
				return node;
		}

		return null;
	}

	protected string ResolveNodeCategoryFromId(string nodeId, string fallback)
	{
		if (nodeId.Contains("headgear"))
			return "headgear";
		if (nodeId.Contains("clothing"))
			return "clothing";
		if (nodeId.Contains("webbing") || nodeId.Contains("chest") || nodeId.Contains("rig"))
			return "webbing";
		if (nodeId.Contains("vest"))
			return "vest";
		if (nodeId.Contains("backpack"))
			return "backpack";
		if (nodeId.Contains("weapon") || nodeId.Contains("primary"))
			return "weapon";
		if (nodeId.Contains("launcher") || nodeId.Contains("secondary"))
			return "launcher";
		if (nodeId.Contains("attach"))
			return "attachment";
		if (nodeId.Contains("magazine") || nodeId.Contains("ammo"))
			return "magazine";
		if (nodeId.Contains("explosive") || nodeId.Contains("grenade"))
			return "explosive";
		if (nodeId.Contains("medical"))
			return "medical";
		if (nodeId.Contains("utility") || nodeId.Contains("equipment"))
			return "utility";

		return fallback;
	}

	protected string ResolveNodeLabelFromId(string nodeId)
	{
		if (nodeId.Contains("optic"))
			return "Optic";
		if (nodeId.Contains("muzzle"))
			return "Muzzle";
		if (nodeId.Contains("underbarrel"))
			return "Underbarrel";
		if (nodeId.Contains("headgear"))
			return "Headgear";
		if (nodeId.Contains("webbing") || nodeId.Contains("chest") || nodeId.Contains("rig"))
			return "Chest Rig";
		if (nodeId.Contains("vest"))
			return "Armored Vest";
		if (nodeId.Contains("backpack"))
			return "Backpack";
		if (nodeId.Contains("weapon"))
			return "Weapon";

		return "Slot";
	}

	protected string FindFirstWeaponNodeId(HST_LoadoutEditorSessionState session)
	{
		if (!session)
			return "";

		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (!slot)
				continue;

			string category = ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory);
			if (category == "weapon" || category == "launcher")
				return "node_" + slot.m_sSlotId;
		}

		return "";
	}

	protected IEntity ResolveControlledPlayerEntity(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || playerId <= 0)
			return null;

		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (controlledEntity)
			return controlledEntity;

		return SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
	}

	protected void BuildDraftSlotsFromIssued(HST_CampaignState state, string identityId, HST_SavedLoadoutState loadout)
	{
		foreach (HST_IssuedLoadoutItemState issuedItem : state.m_aIssuedLoadoutItems)
		{
			if (!issuedItem || issuedItem.m_sOwnerIdentityId != identityId || issuedItem.m_sItemPrefab.IsEmpty())
				continue;

			HST_LoadoutSlotState slot = new HST_LoadoutSlotState();
			slot.m_sSlotId = BuildSlotId(issuedItem.m_sCategory, loadout.m_aSlots.Count());
			slot.m_sItemPrefab = issuedItem.m_sItemPrefab;
			slot.m_sDisplayName = issuedItem.m_sDisplayName;
			slot.m_sCategory = issuedItem.m_sCategory;
			slot.m_iQuantity = Math.Max(1, issuedItem.m_iCount);
			loadout.m_aSlots.Insert(slot);
		}
	}

	protected void BuildStarterDraftSlotsFromArsenal(HST_CampaignState state, HST_SavedLoadoutState loadout)
	{
		array<string> usedCategories = {};
		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!item || item.m_sPrefab.IsEmpty())
				continue;

			if (!item.m_bUnlocked && item.m_iCount <= 0)
				continue;

			string category = ResolveEditorCategory(item.m_sPrefab, item.m_sCategory);
			if (usedCategories.Find(category) >= 0 && category != "magazine" && category != "attachment" && category != "medical" && category != "utility")
				continue;

			HST_LoadoutSlotState slot = new HST_LoadoutSlotState();
			slot.m_sSlotId = BuildSlotId(category, loadout.m_aSlots.Count());
			slot.m_sItemPrefab = item.m_sPrefab;
			slot.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, item.m_sPrefab, item.m_sDisplayName);
			slot.m_sCategory = category;
			slot.m_iQuantity = 1;
			loadout.m_aSlots.Insert(slot);
			usedCategories.Insert(category);
			if (loadout.m_aSlots.Count() >= MAX_AUTO_DRAFT_SLOTS)
				return;
		}
	}

	protected HST_SavedLoadoutState SelectLoadout(HST_CampaignState state, string identityId, string loadoutId)
	{
		if (!loadoutId.IsEmpty())
			return state.FindSavedLoadout(identityId, loadoutId);

		HST_LoadoutEditorSessionState session = state.FindLoadoutEditorSession(identityId);
		if (session && !session.m_sCurrentLoadoutId.IsEmpty())
		{
			HST_SavedLoadoutState current = state.FindSavedLoadout(identityId, session.m_sCurrentLoadoutId);
			if (current)
				return current;
		}

		return state.FindFirstSavedLoadout(identityId);
	}

	protected bool ValidateLoadoutTransaction(HST_CampaignState state, HST_SavedLoadoutState loadout, string identityId, notnull array<ref HST_LoadoutCostEntry> costLedger, out string failure)
	{
		failure = "";
		if (!BuildLoadoutCostLedger(state, loadout, identityId, costLedger, failure))
			return false;

		if (!ValidateAttachmentCompatibility(loadout, failure))
			return false;

		return true;
	}

	protected bool BuildLoadoutCostLedger(HST_CampaignState state, HST_SavedLoadoutState loadout, string identityId, notnull array<ref HST_LoadoutCostEntry> costLedger, out string failure)
	{
		failure = "";
		costLedger.Clear();
		if (!state || !loadout || identityId.IsEmpty())
		{
			failure = "loadout transaction state is not ready";
			return false;
		}

		foreach (HST_LoadoutSlotState slot : loadout.m_aSlots)
		{
			if (!slot || slot.m_sItemPrefab.IsEmpty())
			{
				failure = "saved loadout contains an empty slot";
				return false;
			}

			HST_ArsenalItemState arsenalItem = state.FindArsenalItem(slot.m_sItemPrefab);
			if (!arsenalItem)
			{
				failure = "item not present in h-istasi arsenal: " + slot.m_sItemPrefab;
				return false;
			}

			int required = Math.Max(1, slot.m_iQuantity);
			HST_LoadoutCostEntry entry = FindCostEntry(costLedger, slot.m_sItemPrefab);
			if (!entry)
			{
				entry = new HST_LoadoutCostEntry();
				entry.m_sPrefab = slot.m_sItemPrefab;
				entry.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, slot.m_sItemPrefab, slot.m_sDisplayName);
				entry.m_sCategory = ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory);
				entry.m_iAlreadyIssuedFinite = CountIssuedFiniteItem(state, identityId, slot.m_sItemPrefab);
				entry.m_bInfinite = arsenalItem.m_bUnlocked;
				costLedger.Insert(entry);
			}

			entry.m_iRequired += required;
			if (arsenalItem.m_bUnlocked)
				entry.m_bInfinite = true;
			if (entry.m_sDisplayName.IsEmpty())
				entry.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, slot.m_sItemPrefab, slot.m_sDisplayName);
			if (entry.m_sCategory.IsEmpty())
				entry.m_sCategory = ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory);
		}

		foreach (HST_LoadoutCostEntry costEntry : costLedger)
		{
			if (!costEntry)
				continue;

			if (costEntry.m_bInfinite)
			{
				costEntry.m_iAdditionalFiniteRequired = 0;
				continue;
			}

			int additionalRequired = costEntry.m_iRequired - costEntry.m_iAlreadyIssuedFinite;
			if (additionalRequired < 0)
				additionalRequired = 0;
			costEntry.m_iAdditionalFiniteRequired = additionalRequired;
			if (additionalRequired <= 0)
				continue;

			HST_ArsenalItemState arsenalItem = state.FindArsenalItem(costEntry.m_sPrefab);
			int available;
			if (arsenalItem)
				available = arsenalItem.m_iCount;
			if (!arsenalItem || available < additionalRequired)
			{
				failure = string.Format("%1 requires %2 more but only %3 remain", costEntry.m_sDisplayName, additionalRequired, available);
				return false;
			}
		}

		return true;
	}

	protected HST_LoadoutCostEntry FindCostEntry(notnull array<ref HST_LoadoutCostEntry> costLedger, string prefab)
	{
		if (prefab.IsEmpty())
			return null;

		foreach (HST_LoadoutCostEntry entry : costLedger)
		{
			if (entry && HST_ArsenalItemEquivalence.AreEquivalentPrefabs(entry.m_sPrefab, prefab))
				return entry;
		}

		return null;
	}

	protected bool ValidateAttachmentCompatibility(HST_SavedLoadoutState loadout, out string failure)
	{
		failure = "";
		foreach (HST_LoadoutSlotState slot : loadout.m_aSlots)
		{
			if (!slot || slot.m_sAttachmentSlotId.IsEmpty())
				continue;

			if (slot.m_sWeaponSlotId.IsEmpty())
			{
				failure = "attachment slot has no weapon slot relationship: " + slot.m_sDisplayName;
				return false;
			}

			string attachmentCategory = ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory);
			if (attachmentCategory != "attachment" || !IsAttachmentCandidateForSlot(ResolveAttachmentSlotKey(slot.m_sAttachmentSlotId), slot.m_sItemPrefab))
			{
				failure = "attachment is not compatible with weapon slot: " + slot.m_sDisplayName;
				return false;
			}
		}

		return true;
	}

	protected bool ApplyLoadoutToPlayerEntity(HST_SavedLoadoutState loadout, int playerId, out string failure)
	{
		failure = "";
		IEntity playerEntity = ResolveControlledPlayerEntity(playerId);
		if (!playerEntity)
		{
			failure = "no player entity to receive loadout";
			return false;
		}

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(playerEntity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
		{
			failure = "player inventory manager not available";
			return false;
		}

		int inserted;
		foreach (HST_LoadoutSlotState slot : loadout.m_aSlots)
		{
			if (!slot || slot.m_sItemPrefab.IsEmpty())
				continue;

			int quantity = Math.Max(1, slot.m_iQuantity);
			for (int i = 0; i < quantity; i++)
			{
				if (!TryInsertItemIntoPlayerInventory(inventory, playerEntity, slot.m_sItemPrefab, failure))
					return false;

				inserted++;
			}
		}

		if (IsDebugLoggingEnabled())
			Print(string.Format("h-istasi loadout editor debug | requested physical insertion of %1 item(s) for %2 slot custom loadout", inserted, loadout.m_aSlots.Count()));
		return true;
	}

	protected bool TryInsertItemIntoPlayerInventory(SCR_InventoryStorageManagerComponent inventory, IEntity playerEntity, string prefab, out string failure)
	{
		return TryInsertItemIntoInventory(inventory, playerEntity, prefab, failure);
	}

	protected bool TryInsertItemIntoInventory(SCR_InventoryStorageManagerComponent inventory, IEntity ownerEntity, string prefab, out string failure, HST_CampaignState state = null, HST_ArsenalService arsenal = null, string identityId = "")
	{
		failure = "";
		if (!inventory || !ownerEntity || prefab.IsEmpty())
		{
			failure = "missing inventory or item prefab";
			return false;
		}

		ResourceName resourceName = prefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded)
		{
			failure = "item resource failed to load: " + prefab;
			return false;
		}

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = ownerEntity.GetOrigin();
		IEntity itemEntity = GetGame().SpawnEntityPrefabEx(resourceName, false, GetGame().GetWorld(), params);
		if (!itemEntity)
		{
			failure = "item entity failed to spawn: " + prefab;
			return false;
		}

		BaseInventoryStorageComponent storage = inventory.FindStorageForInsert(itemEntity, null, EStoragePurpose.PURPOSE_ANY);
		if (!storage)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(itemEntity);
			failure = "Inventory Full";
			return false;
		}

		HST_LoadoutEditorInsertCallback callback = new HST_LoadoutEditorInsertCallback();
		ConfigureInsertCallback(callback, state, arsenal, identityId, prefab);
		callback.m_TemporaryEntity = itemEntity;
		inventory.TryInsertItemInStorage(itemEntity, storage, -1, callback);
		return true;
	}

	protected bool CommitLoadoutTransaction(HST_CampaignState state, HST_ArsenalService arsenal, HST_SavedLoadoutState loadout, string identityId, notnull array<ref HST_LoadoutCostEntry> costLedger, out string failure)
	{
		failure = "";
		if (!state || !arsenal || !loadout || identityId.IsEmpty())
		{
			failure = "loadout transaction state is not ready";
			return false;
		}

		array<ref HST_LoadoutCostEntry> withdrawnEntries = {};
		array<int> withdrawnCounts = {};
		foreach (HST_LoadoutCostEntry withdrawEntry : costLedger)
		{
			if (!withdrawEntry || withdrawEntry.m_bInfinite || withdrawEntry.m_iAdditionalFiniteRequired <= 0)
				continue;

			if (!arsenal.WithdrawItem(state, withdrawEntry.m_sPrefab, withdrawEntry.m_iAdditionalFiniteRequired))
			{
				RollbackLoadoutWithdrawals(state, arsenal, withdrawnEntries, withdrawnCounts);
				failure = string.Format("%1 became unavailable during loadout apply", withdrawEntry.m_sDisplayName);
				return false;
			}

			withdrawnEntries.Insert(withdrawEntry);
			withdrawnCounts.Insert(withdrawEntry.m_iAdditionalFiniteRequired);
		}

		ReturnUnneededIssuedItems(state, arsenal, loadout, identityId);
		foreach (HST_LoadoutCostEntry costEntry : costLedger)
		{
			if (!costEntry || costEntry.m_sPrefab.IsEmpty())
				continue;

			if (costEntry.m_bInfinite)
			{
				if (costEntry.m_iAlreadyIssuedFinite > 0)
					arsenal.RefundItem(state, costEntry.m_sPrefab, costEntry.m_iAlreadyIssuedFinite, costEntry.m_sCategory, costEntry.m_sDisplayName);

				SetIssuedItem(state, identityId, costEntry.m_sPrefab, costEntry.m_sDisplayName, costEntry.m_sCategory, costEntry.m_iRequired, true);
				continue;
			}

			int surplus = costEntry.m_iAlreadyIssuedFinite - costEntry.m_iRequired;
			if (surplus > 0)
				arsenal.RefundItem(state, costEntry.m_sPrefab, surplus, costEntry.m_sCategory, costEntry.m_sDisplayName);

			SetIssuedItem(state, identityId, costEntry.m_sPrefab, costEntry.m_sDisplayName, costEntry.m_sCategory, costEntry.m_iRequired, false);
		}

		return true;
	}

	protected void RollbackLoadoutWithdrawals(HST_CampaignState state, HST_ArsenalService arsenal, notnull array<ref HST_LoadoutCostEntry> withdrawnEntries, notnull array<int> withdrawnCounts)
	{
		if (!state || !arsenal)
			return;

		for (int i = 0; i < withdrawnEntries.Count(); i++)
		{
			HST_LoadoutCostEntry entry = withdrawnEntries[i];
			if (!entry)
				continue;

			int count;
			if (i < withdrawnCounts.Count())
				count = withdrawnCounts[i];
			if (count <= 0)
				continue;

			arsenal.RefundItem(state, entry.m_sPrefab, count, entry.m_sCategory, entry.m_sDisplayName);
		}
	}

	protected void ReturnUnneededIssuedItems(HST_CampaignState state, HST_ArsenalService arsenal, HST_SavedLoadoutState loadout, string identityId)
	{
		for (int i = state.m_aIssuedLoadoutItems.Count() - 1; i >= 0; i--)
		{
			HST_IssuedLoadoutItemState issuedItem = state.m_aIssuedLoadoutItems[i];
			if (!issuedItem || issuedItem.m_sOwnerIdentityId != identityId)
				continue;

			int required = CountLoadoutItem(loadout, issuedItem.m_sItemPrefab);
			if (required > 0)
				continue;

			if (!issuedItem.m_bInfinite)
				arsenal.RefundItem(state, issuedItem.m_sItemPrefab, issuedItem.m_iCount, issuedItem.m_sCategory, issuedItem.m_sDisplayName);

			state.m_aIssuedLoadoutItems.Remove(i);
		}
	}

	protected void SetIssuedItem(HST_CampaignState state, string identityId, string prefab, string displayName, string category, int count, bool infinite)
	{
		HST_IssuedLoadoutItemState issuedItem = state.FindIssuedLoadoutItem(identityId, prefab);
		if (!issuedItem)
		{
			issuedItem = new HST_IssuedLoadoutItemState();
			issuedItem.m_sOwnerIdentityId = identityId;
			issuedItem.m_sItemPrefab = prefab;
			state.m_aIssuedLoadoutItems.Insert(issuedItem);
		}

		issuedItem.m_sDisplayName = displayName;
		issuedItem.m_sCategory = category;
		issuedItem.m_iCount = count;
		issuedItem.m_bInfinite = infinite;
	}

	protected int CountSavedLoadouts(HST_CampaignState state, string identityId)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_SavedLoadoutState loadout : state.m_aSavedLoadouts)
		{
			if (loadout && loadout.m_sOwnerIdentityId == identityId)
				count++;
		}

		return count;
	}

	protected int CountDraftItems(HST_LoadoutEditorSessionState session)
	{
		if (!session)
			return 0;

		int count;
		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (slot)
				count += Math.Max(1, slot.m_iQuantity);
		}

		return count;
	}

	protected void CountDraftFiniteAndInfinite(HST_CampaignState state, HST_LoadoutEditorSessionState session, out int finiteRequired, out int infiniteRequired)
	{
		finiteRequired = 0;
		infiniteRequired = 0;
		if (!state || !session)
			return;

		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (!slot || slot.m_sItemPrefab.IsEmpty())
				continue;

			HST_ArsenalItemState item = state.FindArsenalItem(slot.m_sItemPrefab);
			int quantity = Math.Max(1, slot.m_iQuantity);
			if (item && item.m_bUnlocked)
				infiniteRequired += quantity;
			else
				finiteRequired += quantity;
		}
	}

	protected int CountIssuedItem(HST_CampaignState state, string identityId, string prefab)
	{
		HST_IssuedLoadoutItemState issuedItem = state.FindIssuedLoadoutItem(identityId, prefab);
		if (!issuedItem)
			return 0;

		return issuedItem.m_iCount;
	}

	protected int CountIssuedFiniteItem(HST_CampaignState state, string identityId, string prefab)
	{
		HST_IssuedLoadoutItemState issuedItem = state.FindIssuedLoadoutItem(identityId, prefab);
		if (!issuedItem || issuedItem.m_bInfinite)
			return 0;

		return issuedItem.m_iCount;
	}

	protected int CountLoadoutItem(HST_SavedLoadoutState loadout, string prefab)
	{
		int count;
		foreach (HST_LoadoutSlotState slot : loadout.m_aSlots)
		{
			if (slot && HST_ArsenalItemEquivalence.AreEquivalentPrefabs(slot.m_sItemPrefab, prefab))
				count += Math.Max(1, slot.m_iQuantity);
		}

		return count;
	}

	protected void RefreshIssuedCounts(HST_CampaignState state, string identityId, HST_LoadoutEditorSessionState session)
	{
		session.m_iIssuedFiniteCount = 0;
		session.m_iIssuedInfiniteCount = 0;
		foreach (HST_IssuedLoadoutItemState issuedItem : state.m_aIssuedLoadoutItems)
		{
			if (!issuedItem || issuedItem.m_sOwnerIdentityId != identityId)
				continue;

			if (issuedItem.m_bInfinite)
				session.m_iIssuedInfiniteCount += issuedItem.m_iCount;
			else
				session.m_iIssuedFiniteCount += issuedItem.m_iCount;
		}
	}

	protected string BuildAvailableCategorySummary(HST_CampaignState state)
	{
		int clothing;
		int headgear;
		int vest;
		int webbing;
		int pants;
		int boots;
		int backpack;
		int handwear;
		int weapon;
		int sidearm;
		int magazine;
		int explosive;
		int attachment;
		int medical;
		int utility;
		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!IsArsenalItemAvailable(item))
				continue;

			string category = ResolveEditorCategory(item.m_sPrefab, item.m_sCategory);
			if (category == "clothing")
				clothing++;
			else if (category == "headgear")
				headgear++;
			else if (category == "vest")
				vest++;
			else if (category == "webbing")
				webbing++;
			else if (category == "pants")
				pants++;
			else if (category == "boots")
				boots++;
			else if (category == "backpack")
				backpack++;
			else if (category == "handwear")
				handwear++;
			else if (category == "weapon" || category == "launcher")
				weapon++;
			else if (category == "sidearm")
				sidearm++;
			else if (category == "magazine")
				magazine++;
			else if (category == "explosive")
				explosive++;
			else if (category == "attachment")
				attachment++;
			else if (category == "medical")
				medical++;
			else
				utility++;
		}

		string firstHalf = string.Format("clothing %1 / head %2 / armor %3 / rig %4 / pants %5 / boots %6 / pack %7 / hands %8 / weapons %9", clothing, headgear, vest, webbing, pants, boots, backpack, handwear, weapon);
		firstHalf = firstHalf + string.Format(" / sidearms %1", sidearm);
		string secondHalf = string.Format("mags %1 / explosives %2 / attachments %3 / medical %4 / utility %5", magazine, explosive, attachment, medical, utility);
		return firstHalf + " / " + secondHalf;
	}

	protected string ResolveEditorCategory(string prefab, string sourceCategory)
	{
		string derivedCategory = ResolveCategoryFromPrefab(prefab);
		if (IsLoadoutClothingCategory(derivedCategory))
			return derivedCategory;

		if (!sourceCategory.IsEmpty() && sourceCategory != "equipment")
		{
			if (sourceCategory == "launcher")
				return "launcher";

			if (sourceCategory == "weapon" && derivedCategory == "sidearm")
				return "sidearm";

			if (sourceCategory == "weapon" && (derivedCategory == "explosive" || derivedCategory == "magazine" || derivedCategory == "attachment" || derivedCategory == "medical" || derivedCategory == "utility"))
				return derivedCategory;

			if (sourceCategory == "attachment" && derivedCategory != "weapon" && derivedCategory != "launcher")
				return "attachment";

			return sourceCategory;
		}

		return derivedCategory;
	}

	protected string ResolveCategoryFromPrefab(string prefab)
	{
		if (HST_ArsenalItemFilter.IsMedicalItemToken(prefab))
			return "medical";
		if (HST_ArsenalItemFilter.IsKnownBackpackToken(prefab))
			return "backpack";
		if (prefab.Contains("Helmet") || prefab.Contains("Hat") || prefab.Contains("Headgear") || prefab.Contains("Cap") || prefab.Contains("Beanie") || prefab.Contains("Boonie") || prefab.Contains("Beret"))
			return "headgear";
		if (prefab.Contains("Pants") || prefab.Contains("Trouser"))
			return "pants";
		if (prefab.Contains("Boot"))
			return "boots";
		if (prefab.Contains("Glove") || prefab.Contains("Handwear"))
			return "handwear";
		if (prefab.Contains("Uniform") || prefab.Contains("Jacket") || prefab.Contains("Shirt") || prefab.Contains("Clothes") || prefab.Contains("Blouse") || prefab.Contains("Parka") || prefab.Contains("Coat"))
			return "clothing";
		if (HST_ArsenalItemFilter.IsKnownWebbingToken(prefab))
			return "webbing";
		if (prefab.Contains("Vest") || prefab.Contains("Carrier") || prefab.Contains("Webbing"))
			return "vest";
		if (prefab.Contains("Backpack") || prefab.Contains("Bag") || prefab.Contains("Pack_"))
			return "backpack";
		if (IsAttachmentPrefab(prefab))
			return "attachment";
		if (IsMagazinePrefab(prefab))
			return "magazine";
		if (IsExplosiveOrThrowablePrefab(prefab))
			return "explosive";
		if (prefab.Contains("Bandage") || prefab.Contains("Morphine") || prefab.Contains("Tourniquet") || prefab.Contains("Medical") || prefab.Contains("Medkit"))
			return "medical";
		if (IsSidearmPrefab(prefab))
			return "sidearm";
		if (IsLauncherWeaponCandidate(prefab))
			return "launcher";
		if (IsPrimaryWeaponCandidate(prefab))
			return "weapon";

		return "utility";
	}

	protected bool IsPrimaryWeaponCandidate(string prefab)
	{
		if (prefab.IsEmpty() || IsExplosiveOrThrowablePrefab(prefab) || IsAttachmentPrefab(prefab) || IsMagazinePrefab(prefab))
			return false;

		if (prefab.Contains("Launcher") || prefab.Contains("RPG") || prefab.Contains("M72") || prefab.Contains("LAW") || prefab.Contains("AT4"))
			return false;

		return prefab.Contains("/Weapons/") || prefab.Contains("Weapon") || prefab.Contains("Rifle") || prefab.Contains("Carbine") || prefab.Contains("MG_") || prefab.Contains("SMG") || prefab.Contains("M16");
	}

	protected bool IsSidearmPrefab(string prefab)
	{
		if (prefab.IsEmpty() || IsAttachmentPrefab(prefab) || IsMagazinePrefab(prefab))
			return false;

		return prefab.Contains("Pistol") || prefab.Contains("Handgun") || prefab.Contains("M9") || prefab.Contains("PM_") || prefab.Contains("_PM");
	}

	protected bool IsLauncherWeaponCandidate(string prefab)
	{
		if (prefab.IsEmpty() || IsAttachmentPrefab(prefab) || IsMagazinePrefab(prefab))
			return false;

		if (prefab.Contains("Grenade") || prefab.Contains("Smoke") || prefab.Contains("Mine") || prefab.Contains("Explosive") || prefab.Contains("RDG") || prefab.Contains("AN-M8"))
			return false;

		return prefab.Contains("Launcher") || prefab.Contains("RPG") || prefab.Contains("M72") || prefab.Contains("LAW") || prefab.Contains("AT4");
	}

	protected bool IsMagazinePrefab(string prefab)
	{
		return prefab.Contains("Magazine") || prefab.Contains("Mag_") || prefab.Contains("_Mag") || prefab.Contains("STANAG") || prefab.Contains("PMAG") || prefab.Contains("Belt") || prefab.Contains("AmmoBox");
	}

	protected bool IsExplosiveOrThrowablePrefab(string prefab)
	{
		if (prefab.Contains("Launcher"))
			return false;

		return prefab.Contains("Grenade") || prefab.Contains("Smoke") || prefab.Contains("Mine") || prefab.Contains("Explosive") || prefab.Contains("Flare") || prefab.Contains("M18") || prefab.Contains("M67") || prefab.Contains("RDG") || prefab.Contains("AN-M8");
	}

	protected bool IsAttachmentPrefab(string prefab)
	{
		return prefab.Contains("Optic") || prefab.Contains("Sight") || prefab.Contains("Scope") || prefab.Contains("Muzzle") || prefab.Contains("Suppressor") || prefab.Contains("Flash_Hider") || prefab.Contains("FlashHider") || prefab.Contains("Compensator") || prefab.Contains("Bipod") || prefab.Contains("Grip") || prefab.Contains("Underbarrel") || prefab.Contains("Bayonet") || prefab.Contains("Handguard") || prefab.Contains("Stock") || prefab.Contains("Attachment");
	}

	protected bool IsAttachmentCandidateForSlot(string slotKey, string prefab)
	{
		if (!IsAttachmentPrefab(prefab))
			return false;

		if (slotKey == "optic")
			return prefab.Contains("Optic") || prefab.Contains("Sight") || prefab.Contains("Scope");
		if (slotKey == "muzzle")
			return prefab.Contains("Muzzle") || prefab.Contains("Suppressor") || prefab.Contains("Flash_Hider") || prefab.Contains("FlashHider") || prefab.Contains("Compensator");
		if (slotKey == "underbarrel")
			return prefab.Contains("Underbarrel") || prefab.Contains("Bipod") || prefab.Contains("Grip");
		if (slotKey == "bayonet")
			return prefab.Contains("Bayonet");
		if (slotKey == "handguard")
			return prefab.Contains("Handguard");
		if (slotKey == "stock")
			return prefab.Contains("Stock");
		if (slotKey == "rail")
			return IsAttachmentPrefab(prefab);
		if (slotKey == "equipment" || slotKey == "attachment")
			return IsAttachmentPrefab(prefab);

		return false;
	}

	protected bool IsReplacementCategoryAllowed(HST_LoadoutSlotState slot, string targetCategory, string replacementCategory, string prefab)
	{
		if (!slot)
			return false;

		if (!slot.m_sAttachmentSlotId.IsEmpty())
			return replacementCategory == "attachment" && IsAttachmentCandidateForSlot(ResolveAttachmentSlotKey(slot.m_sAttachmentSlotId), prefab);

		if (targetCategory == "weapon")
			return replacementCategory == "weapon" && IsPrimaryWeaponCandidate(prefab);
		if (targetCategory == "launcher")
			return replacementCategory == "launcher" && IsLauncherWeaponCandidate(prefab);

		return targetCategory == replacementCategory;
	}

	protected string ResolveAttachmentSlotKey(string attachmentSlotId)
	{
		if (attachmentSlotId.Contains("optic"))
			return "optic";
		if (attachmentSlotId.Contains("muzzle"))
			return "muzzle";
		if (attachmentSlotId.Contains("underbarrel"))
			return "underbarrel";
		if (attachmentSlotId.Contains("bayonet"))
			return "bayonet";
		if (attachmentSlotId.Contains("handguard"))
			return "handguard";
		if (attachmentSlotId.Contains("stock"))
			return "stock";
		if (attachmentSlotId.Contains("rail"))
			return "rail";
		if (attachmentSlotId.Contains("magazine") || attachmentSlotId.Contains("ammo"))
			return "magazine";
		if (attachmentSlotId.Contains("equipment"))
			return "attachment";

		return "attachment";
	}

	protected string SanitizePayloadField(string value)
	{
		if (value.IsEmpty())
			return "";

		value.Replace("|", "/");
		value.Replace("\n", " ");
		value.Replace("\r", " ");
		return value;
	}

	protected string ShortenDebugText(string value, int maxCharacters)
	{
		if (value.IsEmpty() || maxCharacters <= 0)
			return "";

		value.Replace("|", "/");
		value.Replace("\n", " ");
		value.Replace("\r", " ");
		if (value.Length() <= maxCharacters)
			return value;

		if (maxCharacters <= 3)
			return value.Substring(0, maxCharacters);

		return value.Substring(0, maxCharacters - 3) + "...";
	}

	protected bool IsAllowedLoadoutSlot(HST_LoadoutSlotState slot)
	{
		return slot && !slot.m_sSlotId.IsEmpty() && !slot.m_sItemPrefab.IsEmpty() && !IsRemovedExternalItem(slot.m_sItemPrefab, slot.m_sDisplayName);
	}

	protected int PurgeRemovedExternalLoadoutState(HST_CampaignState state, string identityId)
	{
		if (!state)
			return 0;

		int purged;
		for (int arsenalIndex = state.m_aArsenalItems.Count() - 1; arsenalIndex >= 0; arsenalIndex--)
		{
			HST_ArsenalItemState item = state.m_aArsenalItems[arsenalIndex];
			if (!item || !IsRemovedExternalItem(item.m_sPrefab, item.m_sDisplayName))
				continue;

			state.m_aArsenalItems.Remove(arsenalIndex);
			purged++;
		}

		for (int issuedIndex = state.m_aIssuedLoadoutItems.Count() - 1; issuedIndex >= 0; issuedIndex--)
		{
			HST_IssuedLoadoutItemState issuedItem = state.m_aIssuedLoadoutItems[issuedIndex];
			if (!issuedItem || issuedItem.m_sOwnerIdentityId != identityId || !IsRemovedExternalItem(issuedItem.m_sItemPrefab, issuedItem.m_sDisplayName))
				continue;

			state.m_aIssuedLoadoutItems.Remove(issuedIndex);
			purged++;
		}

		foreach (HST_SavedLoadoutState loadout : state.m_aSavedLoadouts)
		{
			if (!loadout || loadout.m_sOwnerIdentityId != identityId)
				continue;

			for (int slotIndex = loadout.m_aSlots.Count() - 1; slotIndex >= 0; slotIndex--)
			{
				HST_LoadoutSlotState slot = loadout.m_aSlots[slotIndex];
				if (!slot || !IsRemovedExternalItem(slot.m_sItemPrefab, slot.m_sDisplayName))
					continue;

				loadout.m_aSlots.Remove(slotIndex);
				purged++;
			}
		}

		foreach (HST_LoadoutEditorSessionState session : state.m_aLoadoutEditorSessions)
		{
			if (!session || session.m_sOwnerIdentityId != identityId)
				continue;

			purged += PurgeRemovedExternalDraftSlots(session);
		}

		if (purged > 0)
			state.m_sLastLoadoutEditorFailure = string.Format("purged %1 removed external loadout/arsenal entrie(s)", purged);

		return purged;
	}

	protected int PurgeRemovedExternalDraftSlots(HST_LoadoutEditorSessionState session)
	{
		if (!session)
			return 0;

		int purged;
		for (int slotIndex = session.m_aDraftSlots.Count() - 1; slotIndex >= 0; slotIndex--)
		{
			HST_LoadoutSlotState slot = session.m_aDraftSlots[slotIndex];
			if (!slot || !IsRemovedExternalItem(slot.m_sItemPrefab, slot.m_sDisplayName))
				continue;

			session.m_aDraftSlots.Remove(slotIndex);
			purged++;
		}

		return purged;
	}

	protected bool IsRemovedExternalPrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		if (prefab.Contains("595F2BF") || prefab.Contains("1337C0DE") || prefab.Contains("BADC0DED") || prefab.Contains("StatusQuo") || prefab.Contains("ContentPack"))
			return true;

		return !IsLoadablePrefabResource(prefab);
	}

	protected bool IsRemovedExternalItem(string prefab, string displayName)
	{
		if (IsRemovedExternalPrefab(prefab))
			return true;

		return HasUnresolvedDisplayKey(prefab, displayName);
	}

	protected bool IsLoadablePrefabResource(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		for (int i = 0; i < m_aLoadablePrefabCache.Count(); i++)
		{
			if (m_aLoadablePrefabCache[i] == prefab)
				return m_aLoadablePrefabResults[i];
		}

		Resource loaded = Resource.Load(prefab);
		bool loadable = false;
		if (loaded)
			loadable = loaded.IsValid();
		m_aLoadablePrefabCache.Insert(prefab);
		m_aLoadablePrefabResults.Insert(loadable);

		return loadable;
	}

	protected bool HasUnresolvedDisplayKey(string prefab, string displayName)
	{
		string resolved = HST_DisplayNameService.ResolveItemDisplayName(null, prefab, displayName);
		if (resolved.IsEmpty() || resolved.Length() < 1)
			return false;

		return resolved.Substring(0, 1) == "#";
	}

	protected string BuildSlotId(string category, int index)
	{
		return string.Format("%1_%2", category, index);
	}

	protected string BuildLoadoutId(string identityId, int elapsedSeconds, int index)
	{
		return string.Format("loadout_%1_%2_%3", identityId.Length(), elapsedSeconds, index);
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected int LoadPersonalLoadoutsFromFile(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return 0;

		int loadedV2 = LoadPersonalLoadoutsFromJsonContext(state, identityId);
		if (loadedV2 > 0)
		{
			EnsureFixedPersonalLoadoutSlots(state, identityId);
			return loadedV2;
		}

		array<string> lines = ReadLines(BuildPersonalLoadoutPath(identityId));
		if (lines.Count() == 0)
			return 0;

		for (int existingIndex = state.m_aSavedLoadouts.Count() - 1; existingIndex >= 0; existingIndex--)
		{
			HST_SavedLoadoutState existingLoadout = state.m_aSavedLoadouts[existingIndex];
			if (existingLoadout && existingLoadout.m_sOwnerIdentityId == identityId)
				state.m_aSavedLoadouts.Remove(existingIndex);
		}

		HST_SavedLoadoutState currentLoadout;
		int loaded;
		foreach (string line : lines)
		{
			if (line.Contains("\"loadoutId\""))
			{
				currentLoadout = new HST_SavedLoadoutState();
				currentLoadout.m_sOwnerIdentityId = identityId;
				currentLoadout.m_sLoadoutId = ExtractJsonString(line, "loadoutId");
				currentLoadout.m_sDisplayName = ExtractJsonString(line, "displayName");
				currentLoadout.m_iUpdatedAtSecond = ExtractJsonInt(line, "updatedAtSecond");
				if (!currentLoadout.m_sLoadoutId.IsEmpty())
				{
					state.m_aSavedLoadouts.Insert(currentLoadout);
					loaded++;
				}
				continue;
			}

			if (!currentLoadout || !line.Contains("\"slotId\""))
				continue;

			HST_LoadoutSlotState slot = new HST_LoadoutSlotState();
			slot.m_sSlotId = ExtractJsonString(line, "slotId");
			slot.m_sItemPrefab = ExtractJsonString(line, "itemPrefab");
			slot.m_sDisplayName = ExtractJsonString(line, "displayName");
			slot.m_sCategory = ExtractJsonString(line, "category");
			slot.m_iQuantity = Math.Max(1, ExtractJsonInt(line, "quantity"));
			slot.m_sWeaponSlotId = ExtractJsonString(line, "weaponSlotId");
			slot.m_sAttachmentSlotId = ExtractJsonString(line, "attachmentSlotId");
			if (!slot.m_sSlotId.IsEmpty() && IsAllowedLoadoutSlot(slot))
				currentLoadout.m_aSlots.Insert(slot);
		}

		MigrateLegacyLoadoutsToFixedSlots(state, identityId);
		EnsureFixedPersonalLoadoutSlots(state, identityId);
		return loaded;
	}

	protected bool SavePersonalLoadoutsToFile(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return false;

		FileIO.MakeDirectory("$profile:h-istasi");
		FileIO.MakeDirectory(LOADOUT_DIRECTORY);
		FileIO.MakeDirectory(LOADOUT_DIRECTORY_V2);

		HST_PersonalLoadoutFileState fileState = new HST_PersonalLoadoutFileState();
		fileState.m_sOwnerIdentityId = identityId;
		for (int slotIndex = 0; slotIndex < PERSONAL_LOADOUT_SLOT_COUNT; slotIndex++)
		{
			HST_SavedLoadoutState loadout = state.FindSavedLoadout(identityId, BuildFixedLoadoutId(slotIndex));
			if (loadout)
				fileState.m_aLoadouts.Insert(loadout);
		}

		JsonSaveContext context = new JsonSaveContext();
		if (context.WriteValue("", fileState) && context.SaveToFile(BuildPersonalLoadoutPathV2(identityId)))
			return true;

		array<string> lines = {};
		lines.Insert("{");
		lines.Insert(string.Format("  \"ownerIdentityId\": \"%1\",", JsonSafe(SafeIdentityId(identityId))));
		lines.Insert("  \"loadouts\": [");
		int emittedLoadouts;
		foreach (HST_SavedLoadoutState loadout : state.m_aSavedLoadouts)
		{
			if (!loadout || loadout.m_sOwnerIdentityId != identityId)
				continue;

			if (emittedLoadouts > 0)
				lines.Insert("    },");

			lines.Insert(string.Format("    { \"loadoutId\": \"%1\", \"displayName\": \"%2\", \"updatedAtSecond\": %3, \"slots\": [", JsonSafe(loadout.m_sLoadoutId), JsonSafe(loadout.m_sDisplayName), loadout.m_iUpdatedAtSecond));
			for (int slotIndex = 0; slotIndex < loadout.m_aSlots.Count(); slotIndex++)
			{
				HST_LoadoutSlotState slot = loadout.m_aSlots[slotIndex];
				if (!slot)
					continue;

				string suffix = ",";
				if (slotIndex == loadout.m_aSlots.Count() - 1)
					suffix = "";

				lines.Insert(string.Format("      { \"slotId\": \"%1\", \"itemPrefab\": \"%2\", \"displayName\": \"%3\", \"category\": \"%4\", \"quantity\": %5, \"weaponSlotId\": \"%6\", \"attachmentSlotId\": \"%7\" }%8", JsonSafe(slot.m_sSlotId), JsonSafe(slot.m_sItemPrefab), JsonSafe(slot.m_sDisplayName), JsonSafe(slot.m_sCategory), Math.Max(1, slot.m_iQuantity), JsonSafe(slot.m_sWeaponSlotId), JsonSafe(slot.m_sAttachmentSlotId), suffix));
			}
			lines.Insert("    ]");
			emittedLoadouts++;
		}

		if (emittedLoadouts > 0)
			lines.Insert("    }");

		lines.Insert("  ]");
		lines.Insert("}");
		return WriteLines(BuildPersonalLoadoutPath(identityId), lines);
	}

	protected int LoadPersonalLoadoutsFromJsonContext(HST_CampaignState state, string identityId)
	{
		string path = BuildPersonalLoadoutPathV2(identityId);
		if (!FileIO.FileExists(path))
			return 0;

		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return 0;

		HST_PersonalLoadoutFileState fileState = new HST_PersonalLoadoutFileState();
		if (!context.ReadValue("", fileState))
			return 0;

		for (int existingIndex = state.m_aSavedLoadouts.Count() - 1; existingIndex >= 0; existingIndex--)
		{
			HST_SavedLoadoutState existingLoadout = state.m_aSavedLoadouts[existingIndex];
			if (existingLoadout && existingLoadout.m_sOwnerIdentityId == identityId)
				state.m_aSavedLoadouts.Remove(existingIndex);
		}

		int loaded;
		foreach (HST_SavedLoadoutState loadout : fileState.m_aLoadouts)
		{
			if (!loadout)
				continue;

			loadout.m_sOwnerIdentityId = identityId;
			if (loadout.m_iSlotIndex < 0)
				loadout.m_iSlotIndex = ResolveFixedLoadoutIndex(loadout.m_sLoadoutId);
			if (loadout.m_sLoadoutId.IsEmpty())
				loadout.m_sLoadoutId = BuildFixedLoadoutId(loadout.m_iSlotIndex);
			if (loadout.m_sDisplayName.IsEmpty())
				loadout.m_sDisplayName = string.Format("Field Loadout %1", loadout.m_iSlotIndex + 1);
			state.m_aSavedLoadouts.Insert(loadout);
			loaded++;
		}

		return loaded;
	}

	protected string BuildPersonalLoadoutPath(string identityId)
	{
		return LOADOUT_DIRECTORY + "/" + SafeIdentityId(identityId) + ".json";
	}

	protected string BuildPersonalLoadoutPathV2(string identityId)
	{
		return LOADOUT_DIRECTORY_V2 + "/" + SafeIdentityId(identityId) + ".json";
	}

	protected string SafeIdentityId(string identityId)
	{
		string safeId = identityId;
		safeId.Replace("/", "_");
		safeId.Replace(":", "_");
		safeId.Replace(" ", "_");
		if (safeId.IsEmpty())
			safeId = "unknown";

		return safeId;
	}

	protected string JsonSafe(string value)
	{
		string safe = value;
		safe.Replace("\n", " ");
		safe.Replace("\r", " ");
		return safe;
	}

	protected string ExtractJsonString(string line, string key)
	{
		string marker = "\"" + key + "\": \"";
		int start = line.IndexOf(marker);
		if (start < 0)
			return "";

		start += marker.Length();
		int end = line.IndexOfFrom(start, "\"");
		if (end < 0)
			return "";

		return line.Substring(start, end - start);
	}

	protected int ExtractJsonInt(string line, string key)
	{
		string marker = "\"" + key + "\": ";
		int start = line.IndexOf(marker);
		if (start < 0)
			return 0;

		start += marker.Length();
		int end = line.IndexOfFrom(start, ",");
		if (end < 0)
			end = line.IndexOfFrom(start, "}");
		if (end < 0)
			end = line.Length();

		string value = line.Substring(start, end - start);
		value.Replace(" ", "");
		return value.ToInt();
	}

	protected array<string> ReadLines(string fileName)
	{
		array<string> lines = {};
		FileHandle file = FileIO.OpenFile(fileName, FileMode.READ);
		if (!file)
			return lines;

		string line;
		while (file.ReadLine(line) >= 0)
			lines.Insert(line);

		file.Close();
		return lines;
	}

	protected bool WriteLines(string fileName, notnull array<string> lines)
	{
		FileHandle file = FileIO.OpenFile(fileName, FileMode.WRITE);
		if (!file)
			return false;

		foreach (string line : lines)
			file.WriteLine(line);

		file.Close();
		return true;
	}
}
