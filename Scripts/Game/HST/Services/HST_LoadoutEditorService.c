class HST_LoadoutEditorInsertCallback : ScriptedInventoryOperationCallback
{
	IEntity m_TemporaryEntity;

	override void OnFailed()
	{
		if (m_TemporaryEntity)
			SCR_EntityHelper.DeleteEntityAndChildren(m_TemporaryEntity);
	}
}

class HST_LoadoutEditorService
{
	static const string PREVIEW_MANNEQUIN_PREFAB = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	static const string LOADOUT_DIRECTORY = "$profile:h-istasi/loadouts";
	static const int MAX_AUTO_DRAFT_SLOTS = 12;

	protected ref array<string> m_aPreviewIdentityIds = {};
	protected ref array<IEntity> m_aPreviewEntities = {};

	string OpenEditor(HST_CampaignState state, string identityId, int playerId)
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi loadout editor | failed: campaign/player state not ready";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		int loadedTemplates = LoadPersonalLoadoutsFromFile(state, identityId);
		int purgedExternal = PurgeRemovedExternalLoadoutState(state, identityId);
		session.m_sStatus = "open";
		session.m_sLastFailure = "";
		session.m_sPreviewPrefab = PREVIEW_MANNEQUIN_PREFAB;
		session.m_iOpenedAtSecond = state.m_iElapsedSeconds;
		session.m_iSavedLoadoutCount = CountSavedLoadouts(state, identityId);
		RefreshIssuedCounts(state, identityId, session);
		EnsureDraftSlots(state, identityId, session);
		DeletePreviewMannequin(identityId);
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

		DeletePreviewMannequin(identityId);
		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		session.m_sStatus = "closed";
		session.m_bPreviewSpawned = false;
		session.m_sLastFailure = "";
		state.m_sLoadoutEditorStatus = "closed";
		state.m_sLastLoadoutEditorFailure = "";
		return "h-istasi loadout editor | closed without arsenal count changes";
	}

	string BuildEditorPayload(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return "";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		PurgeRemovedExternalLoadoutState(state, identityId);
		EnsureDraftSlots(state, identityId, session);
		RefreshDraftNodes(state, session);

		int draftItemCount = CountDraftItems(session);
		int finiteRequired;
		int infiniteRequired;
		CountDraftFiniteAndInfinite(state, session, finiteRequired, infiniteRequired);

		string payload = string.Format("HST_LOADOUT_EDITOR|%1|%2|%3|%4|%5|%6|%7", session.m_sStatus, session.m_sCurrentLoadoutId, session.m_bPreviewSpawned, session.m_aDraftSlots.Count(), draftItemCount, finiteRequired, infiniteRequired);
		payload = payload + string.Format("\nPREVIEW|%1|%2|%3|%4", session.m_bPreviewSpawned, session.m_vPreviewPosition, session.m_iPreviewItemCount, SanitizePayloadField(session.m_sPreviewStatus));
		payload = payload + string.Format("\nPREVIEW_PREFAB|%1", session.m_sPreviewPrefab);
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
				payload = payload + string.Format("\nSTORAGE|%1|%2|%3|%4|%5", node.m_sNodeId, SanitizePayloadField(node.m_sLabel), node.m_iUsedCapacity, node.m_iTotalCapacity, node.m_bCanDeposit);
			if (node.m_sKind == "attachment")
				payload = payload + string.Format("\nATTACH|%1|%2|%3|%4|%5", node.m_sNodeId, node.m_sParentNodeId, SanitizePayloadField(node.m_sSlotKey), node.m_sItemPrefab, SanitizePayloadField(node.m_sDisplayName));
		}

		foreach (HST_LoadoutNodeState candidateNode : session.m_aDraftNodes)
		{
			if (!candidateNode)
				continue;

			payload = payload + BuildCandidatePayloadsForNode(state, candidateNode);
		}

		foreach (HST_SavedLoadoutState loadout : state.m_aSavedLoadouts)
		{
			if (!loadout || loadout.m_sOwnerIdentityId != identityId)
				continue;

			payload = payload + string.Format("\nTEMPLATE|%1|%2|%3", loadout.m_sLoadoutId, SanitizePayloadField(loadout.m_sDisplayName), loadout.m_aSlots.Count());
		}

		return payload;
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

	string SaveCurrentDraft(HST_CampaignState state, string identityId, string loadoutName = "")
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi loadout editor | failed: campaign/player state not ready";

		if (loadoutName.IsEmpty())
			loadoutName = "Field Loadout " + string.Format("%1", CountSavedLoadouts(state, identityId) + 1);

		HST_SavedLoadoutState loadout = new HST_SavedLoadoutState();
		loadout.m_sOwnerIdentityId = identityId;
		loadout.m_sLoadoutId = BuildLoadoutId(identityId, state.m_iElapsedSeconds, state.m_aSavedLoadouts.Count());
		loadout.m_sDisplayName = loadoutName;
		loadout.m_iUpdatedAtSecond = state.m_iElapsedSeconds;

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		EnsureDraftSlots(state, identityId, session);
		foreach (HST_LoadoutSlotState draftSlot : session.m_aDraftSlots)
			loadout.m_aSlots.Insert(CopySlot(draftSlot));

		if (loadout.m_aSlots.Count() == 0)
		{
			state.m_sLastLoadoutEditorFailure = "no arsenal items available to save into a draft";
			return "h-istasi loadout editor | failed: no issued or arsenal items available for a draft";
		}

		state.m_aSavedLoadouts.Insert(loadout);
		SavePersonalLoadoutsToFile(state, identityId);
		session.m_sCurrentLoadoutId = loadout.m_sLoadoutId;
		session.m_iSavedLoadoutCount = CountSavedLoadouts(state, identityId);
		session.m_sStatus = "draft saved";
		state.m_sLoadoutEditorStatus = string.Format("saved %1", loadout.m_sDisplayName);
		state.m_sLastLoadoutEditorFailure = "";
		return string.Format("h-istasi loadout editor | saved %1 | slots %2", loadout.m_sDisplayName, loadout.m_aSlots.Count());
	}

	string ApplySavedLoadout(HST_CampaignState state, HST_ArsenalService arsenal, string identityId, int playerId, string loadoutId = "")
	{
		if (!state || !arsenal || identityId.IsEmpty())
			return "h-istasi loadout editor | failed: service not ready";

		HST_SavedLoadoutState loadout = SelectLoadoutForApply(state, identityId, loadoutId);
		if (!loadout)
			return "h-istasi loadout editor | failed: no current or saved personal loadout";

		string validationFailure;
		if (!ValidateLoadoutTransaction(state, loadout, identityId, validationFailure))
		{
			state.m_sLastLoadoutEditorFailure = validationFailure;
			return "h-istasi loadout editor | failed: " + validationFailure;
		}

		if (!ApplyLoadoutToPlayerEntity(loadout, playerId, validationFailure))
		{
			state.m_sLastLoadoutEditorFailure = validationFailure;
			return "h-istasi loadout editor | failed: " + validationFailure;
		}

		CommitLoadoutTransaction(state, arsenal, loadout, identityId);
		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		session.m_sStatus = "applied";
		session.m_sCurrentLoadoutId = loadout.m_sLoadoutId;
		RefreshIssuedCounts(state, identityId, session);
		state.m_sLoadoutEditorStatus = string.Format("applied %1", loadout.m_sDisplayName);
		state.m_sLastLoadoutEditorFailure = "";
		return string.Format("h-istasi loadout editor | applied %1 | finite %2 | INF %3", loadout.m_sDisplayName, session.m_iIssuedFiniteCount, session.m_iIssuedInfiniteCount);
	}

	string AddDraftItem(HST_CampaignState state, string identityId, string itemPrefab)
	{
		if (!state || identityId.IsEmpty() || itemPrefab.IsEmpty())
			return "h-istasi loadout editor | failed: missing item";

		HST_ArsenalItemState item = state.FindArsenalItem(itemPrefab);
		if (!IsArsenalItemAvailable(item))
			return "h-istasi loadout editor | failed: item not available in arsenal";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		EnsureDraftSlots(state, identityId, session);
		HST_LoadoutSlotState existing = FindDraftSlotByPrefab(session, itemPrefab);
		if (existing)
		{
			int maxQuantity = ResolveDraftMaxQuantity(state, identityId, itemPrefab);
			if (maxQuantity > 0 && existing.m_iQuantity >= maxQuantity)
				return string.Format("h-istasi loadout editor | failed: only %1 available for %2", maxQuantity, existing.m_sDisplayName);

			existing.m_iQuantity++;
			session.m_sStatus = "draft edited";
			RefreshDraftNodes(state, session);
			RefreshPreviewMannequinLoadout(state, identityId, session);
			return string.Format("h-istasi loadout editor | increased %1 to %2", existing.m_sDisplayName, existing.m_iQuantity);
		}

		HST_LoadoutSlotState slot = new HST_LoadoutSlotState();
		slot.m_sSlotId = BuildUniqueDraftSlotId(session, ResolveEditorCategory(item.m_sPrefab, item.m_sCategory));
		slot.m_sItemPrefab = item.m_sPrefab;
		slot.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, item.m_sPrefab, item.m_sDisplayName);
		slot.m_sCategory = ResolveEditorCategory(item.m_sPrefab, item.m_sCategory);
		slot.m_iQuantity = 1;
		session.m_aDraftSlots.Insert(slot);
		session.m_sStatus = "draft edited";
		RefreshDraftNodes(state, session);
		RefreshPreviewMannequinLoadout(state, identityId, session);
		return "h-istasi loadout editor | added " + slot.m_sDisplayName;
	}

	string SetNodeItem(HST_CampaignState state, string identityId, string argument)
	{
		if (!state || identityId.IsEmpty() || argument.IsEmpty())
			return "h-istasi loadout editor | failed: missing node replacement";

		string nodeId;
		string itemPrefab;
		if (!ParseSlotPrefabArgument(argument, nodeId, itemPrefab))
			return "h-istasi loadout editor | failed: malformed node replacement";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		EnsureDraftSlots(state, identityId, session);
		string slotId = ResolveDraftSlotIdFromNodeId(nodeId);
		if (!slotId.IsEmpty())
			return ReplaceDraftSlotItem(state, identityId, slotId + ":" + itemPrefab);

		HST_ArsenalItemState item = state.FindArsenalItem(itemPrefab);
		if (!IsArsenalItemAvailable(item))
			return "h-istasi loadout editor | failed: item not available in arsenal";

		HST_LoadoutSlotState slot = new HST_LoadoutSlotState();
		string category = ResolveNodeCategoryFromId(nodeId, ResolveEditorCategory(item.m_sPrefab, item.m_sCategory));
		slot.m_sSlotId = BuildUniqueDraftSlotId(session, category);
		slot.m_sItemPrefab = item.m_sPrefab;
		slot.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, item.m_sPrefab, item.m_sDisplayName);
		slot.m_sCategory = category;
		slot.m_iQuantity = 1;
		if (nodeId.Contains("attach_"))
		{
			slot.m_sAttachmentSlotId = nodeId;
			slot.m_sWeaponSlotId = FindFirstWeaponNodeId(session);
		}
		session.m_aDraftSlots.Insert(slot);
		session.m_sStatus = "draft edited";
		RefreshDraftNodes(state, session);
		RefreshPreviewMannequinLoadout(state, identityId, session);
		return "h-istasi loadout editor | set " + ResolveNodeLabelFromId(nodeId) + " to " + slot.m_sDisplayName;
	}

	string RemoveNodeItem(HST_CampaignState state, string identityId, string nodeId)
	{
		if (!state || identityId.IsEmpty() || nodeId.IsEmpty())
			return "h-istasi loadout editor | failed: missing node";

		string slotId = ResolveDraftSlotIdFromNodeId(nodeId);
		if (!slotId.IsEmpty())
			return RemoveDraftSlot(state, identityId, slotId);

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		for (int slotIndex = session.m_aDraftSlots.Count() - 1; slotIndex >= 0; slotIndex--)
		{
			HST_LoadoutSlotState slot = session.m_aDraftSlots[slotIndex];
			if (!slot)
				continue;

			if (slot.m_sAttachmentSlotId != nodeId && slot.m_sWeaponSlotId != nodeId)
				continue;

			string label = slot.m_sDisplayName;
			session.m_aDraftSlots.Remove(slotIndex);
			session.m_sStatus = "draft edited";
			RefreshDraftNodes(state, session);
			RefreshPreviewMannequinLoadout(state, identityId, session);
			return "h-istasi loadout editor | removed " + label;
		}

		return "h-istasi loadout editor | node was already empty";
	}

	string RemoveDraftSlot(HST_CampaignState state, string identityId, string slotId)
	{
		if (!state || identityId.IsEmpty() || slotId.IsEmpty())
			return "h-istasi loadout editor | failed: missing slot";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		for (int i = session.m_aDraftSlots.Count() - 1; i >= 0; i--)
		{
			HST_LoadoutSlotState slot = session.m_aDraftSlots[i];
			if (!slot || slot.m_sSlotId != slotId)
				continue;

			string label = slot.m_sDisplayName;
			session.m_aDraftSlots.Remove(i);
			session.m_sStatus = "draft edited";
			RefreshDraftNodes(state, session);
			RefreshPreviewMannequinLoadout(state, identityId, session);
			return "h-istasi loadout editor | removed " + label;
		}

		return "h-istasi loadout editor | failed: slot not found";
	}

	string SetDraftSlotQuantity(HST_CampaignState state, string identityId, string argument)
	{
		if (!state || identityId.IsEmpty() || argument.IsEmpty())
			return "h-istasi loadout editor | failed: missing quantity";

		string slotId;
		int quantity;
		if (!ParseSlotQuantityArgument(argument, slotId, quantity))
			return "h-istasi loadout editor | failed: malformed quantity";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (!slot || slot.m_sSlotId != slotId)
				continue;

			int maxQuantity = ResolveDraftMaxQuantity(state, identityId, slot.m_sItemPrefab);
			slot.m_iQuantity = Math.Max(1, quantity);
			if (maxQuantity > 0)
				slot.m_iQuantity = Math.Min(slot.m_iQuantity, maxQuantity);
			session.m_sStatus = "draft edited";
			RefreshDraftNodes(state, session);
			RefreshPreviewMannequinLoadout(state, identityId, session);
			return string.Format("h-istasi loadout editor | set %1 x%2", slot.m_sDisplayName, slot.m_iQuantity);
		}

		return "h-istasi loadout editor | failed: slot not found";
	}

	string ReplaceDraftSlotItem(HST_CampaignState state, string identityId, string argument)
	{
		if (!state || identityId.IsEmpty() || argument.IsEmpty())
			return "h-istasi loadout editor | failed: missing replacement";

		string slotId;
		string itemPrefab;
		if (!ParseSlotPrefabArgument(argument, slotId, itemPrefab))
			return "h-istasi loadout editor | failed: malformed replacement";

		HST_ArsenalItemState item = state.FindArsenalItem(itemPrefab);
		if (!IsArsenalItemAvailable(item))
			return "h-istasi loadout editor | failed: replacement item not available in arsenal";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (!slot || slot.m_sSlotId != slotId)
				continue;

			slot.m_sItemPrefab = item.m_sPrefab;
			slot.m_sDisplayName = HST_DisplayNameService.ResolveItemDisplayName(null, item.m_sPrefab, item.m_sDisplayName);
			slot.m_sCategory = ResolveEditorCategory(item.m_sPrefab, item.m_sCategory);
			slot.m_iQuantity = 1;
			session.m_sStatus = "draft edited";
			RefreshDraftNodes(state, session);
			RefreshPreviewMannequinLoadout(state, identityId, session);
			return "h-istasi loadout editor | swapped slot to " + slot.m_sDisplayName;
		}

		return "h-istasi loadout editor | failed: slot not found";
	}

	string SelectSavedLoadout(HST_CampaignState state, string identityId, string loadoutId)
	{
		if (!state || identityId.IsEmpty() || loadoutId.IsEmpty())
			return "h-istasi loadout editor | failed: missing template";

		HST_SavedLoadoutState loadout = state.FindSavedLoadout(identityId, loadoutId);
		if (!loadout)
			return "h-istasi loadout editor | failed: template not found";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		session.m_aDraftSlots.Clear();
		foreach (HST_LoadoutSlotState slot : loadout.m_aSlots)
			session.m_aDraftSlots.Insert(CopySlot(slot));
		session.m_sCurrentLoadoutId = loadout.m_sLoadoutId;
		session.m_sStatus = "template selected";
		RefreshDraftNodes(state, session);
		RefreshPreviewMannequinLoadout(state, identityId, session);
		return "h-istasi loadout editor | selected " + loadout.m_sDisplayName;
	}

	string ClearDraft(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return "h-istasi loadout editor | failed: campaign/player state not ready";

		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		session.m_aDraftSlots.Clear();
		session.m_sCurrentLoadoutId = "";
		session.m_sStatus = "draft cleared";
		RefreshDraftNodes(state, session);
		RefreshPreviewMannequinLoadout(state, identityId, session);
		return "h-istasi loadout editor | cleared current draft";
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
			state.m_aSavedLoadouts.Remove(i);
			SavePersonalLoadoutsToFile(state, identityId);
			HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
			if (session.m_sCurrentLoadoutId == loadoutId)
				session.m_sCurrentLoadoutId = "";
			session.m_iSavedLoadoutCount = CountSavedLoadouts(state, identityId);
			session.m_sStatus = "template deleted";
			RefreshDraftNodes(state, session);
			RefreshPreviewMannequinLoadout(state, identityId, session);
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

	protected HST_SavedLoadoutState SelectLoadoutForApply(HST_CampaignState state, string identityId, string loadoutId)
	{
		HST_LoadoutEditorSessionState session = FindOrCreateSession(state, identityId);
		EnsureDraftSlots(state, identityId, session);
		if (session.m_aDraftSlots.Count() > 0)
		{
			HST_SavedLoadoutState draft = new HST_SavedLoadoutState();
			draft.m_sOwnerIdentityId = identityId;
			draft.m_sLoadoutId = "current_draft";
			draft.m_sDisplayName = "Current Draft";
			draft.m_iUpdatedAtSecond = state.m_iElapsedSeconds;
			foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
				draft.m_aSlots.Insert(CopySlot(slot));
			return draft;
		}

		return SelectLoadout(state, identityId, loadoutId);
	}

	protected bool IsArsenalItemAvailable(HST_ArsenalItemState item)
	{
		return item && !item.m_sPrefab.IsEmpty() && !IsRemovedExternalPrefab(item.m_sPrefab) && (item.m_bUnlocked || item.m_iCount > 0);
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
		return 10;
	}

	protected string GetEditorCategoryId(int index)
	{
		if (index == 0)
			return "clothing";
		if (index == 1)
			return "headgear";
		if (index == 2)
			return "vest";
		if (index == 3)
			return "backpack";
		if (index == 4)
			return "weapon";
		if (index == 5)
			return "magazine";
		if (index == 6)
			return "explosive";
		if (index == 7)
			return "attachment";
		if (index == 8)
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
			return "Vest";
		if (categoryId == "backpack")
			return "Backpack";
		if (categoryId == "weapon")
			return "Weapons";
		if (categoryId == "launcher")
			return "Launchers";
		if (categoryId == "magazine")
			return "Magazines";
		if (categoryId == "explosive")
			return "Explosives";
		if (categoryId == "attachment")
			return "Attachments";
		if (categoryId == "medical")
			return "Medical";

		return "Utility";
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
		string payload = string.Format("\nNODE|%1|%2|%3|%4|%5|%6|%7|%8|%9", node.m_sNodeId, node.m_sParentNodeId, node.m_sKind, SanitizePayloadField(node.m_sSlotKey), SanitizePayloadField(node.m_sLabel), node.m_sItemPrefab, SanitizePayloadField(display), count, infinite);
		payload = payload + string.Format("|%1|%2|%3|%4", node.m_bCanOpen, node.m_bCanRemove, node.m_bCanDeposit, node.m_sFocus);
		return payload;
	}

	protected string BuildCandidatePayloadsForNode(HST_CampaignState state, HST_LoadoutNodeState node)
	{
		if (!state || !node)
			return "";

		string payload;
		foreach (HST_ArsenalItemState item : state.m_aArsenalItems)
		{
			if (!IsArsenalItemAvailable(item))
				continue;

			string category = ResolveEditorCategory(item.m_sPrefab, item.m_sCategory);
			if (!IsCandidateCategoryForNode(node, category, item.m_sPrefab))
				continue;

			string display = HST_DisplayNameService.ResolveItemDisplayName(null, item.m_sPrefab, item.m_sDisplayName);
			string shortDisplay = HST_DisplayNameService.ResolveShortItemDisplayName(display, item.m_sPrefab);
			string infinite = "";
			if (item.m_bUnlocked)
				infinite = "INF";

			payload = payload + string.Format("\nCANDIDATE|%1|%2|%3|%4|%5|%6|%7|%8|%9", node.m_sNodeId, item.m_sPrefab, SanitizePayloadField(display), SanitizePayloadField(shortDisplay), item.m_iCount, infinite, category, true, SanitizePayloadField(BuildCandidateIconHint(category, item.m_sPrefab)));
		}

		return payload;
	}

	protected void RefreshDraftNodes(HST_CampaignState state, HST_LoadoutEditorSessionState session)
	{
		if (!session)
			return;

		session.m_aDraftNodes.Clear();
		AddLoadoutNodeForCategory(session, "headgear", "Hat", "Headgear", "head");
		AddLoadoutNodeForCategory(session, "clothing", "Jacket", "Uniform", "torso");
		AddLoadoutNodeForCategory(session, "vest", "ArmoredVest", "Vest", "torso");
		AddLoadoutNodeForCategory(session, "backpack", "Back", "Backpack", "back");
		AddLoadoutNodeForCategory(session, "weapon", "Primary Weapon", "Primary Weapon", "weapon");
		AddLoadoutNodeForCategory(session, "launcher", "Secondary Weapon", "Launcher", "weapon");
		AddInventoryNodeRoot(session);
		AddInventoryNodesForCategory(session, "magazine", "Ammunition", "ammo");
		AddInventoryNodesForCategory(session, "explosive", "Grenades", "hands");
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
		node.m_bCanOpen = slot != null && (category == "vest" || category == "backpack" || category == "clothing");
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
			if (!slot || ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory) != category)
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

		AddAttachmentNode(session, parentId, "attach_optic", "Optics", "optic");
		AddAttachmentNode(session, parentId, "attach_muzzle", "Muzzle", "muzzle");
		AddAttachmentNode(session, parentId, "attach_underbarrel", "Underbarrel", "weapon");
	}

	protected void AddAttachmentNode(HST_LoadoutEditorSessionState session, string parentId, string nodeId, string label, string focus)
	{
		HST_LoadoutSlotState slot = FindAttachmentSlot(session, nodeId);
		HST_LoadoutNodeState node = new HST_LoadoutNodeState();
		if (slot)
			node.m_sNodeId = "node_" + slot.m_sSlotId;
		else
			node.m_sNodeId = nodeId;
		node.m_sParentNodeId = parentId;
		node.m_sKind = "attachment";
		node.m_sSlotKey = "attachment";
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
			return category == "attachment";

		if (node.m_sKind == "storage" || node.m_sKind == "storage_item")
			return category == node.m_sCategory || category == "magazine" || category == "explosive" || category == "medical" || category == "utility";

		if (node.m_sSlotKey == "weapon" || node.m_sSlotKey == "launcher")
			return category == "weapon";

		return category == node.m_sCategory;
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
		if (category == "weapon")
			return "weapon";
		if (category == "magazine")
			return "ammo";
		if (category == "attachment")
			return "wrench";
		if (category == "vest" || category == "backpack" || category == "clothing" || category == "headgear")
			return "clothing";
		if (category == "explosive")
			return "grenade";

		return "gear";
	}

	protected int ResolveDisplayCapacityForCategory(string category)
	{
		if (category == "backpack")
			return 12;
		if (category == "vest")
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

			string category = ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory);
			if (category == "magazine" || category == "explosive" || category == "medical" || category == "utility" || category == "attachment")
				count += Math.Max(1, slot.m_iQuantity);
		}

		return count;
	}

	protected string GetEditorSlotLabel(string categoryId)
	{
		if (categoryId == "clothing")
			return "Uniform";
		if (categoryId == "headgear")
			return "Head";
		if (categoryId == "vest")
			return "Vest";
		if (categoryId == "backpack")
			return "Back";
		if (categoryId == "weapon")
			return "Weapon";
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
		return categoryId == "clothing" || categoryId == "headgear" || categoryId == "vest" || categoryId == "backpack" || categoryId == "weapon" || categoryId == "launcher" || categoryId == "attachment";
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

	protected string ResolveNodeCategoryFromId(string nodeId, string fallback)
	{
		if (nodeId.Contains("headgear"))
			return "headgear";
		if (nodeId.Contains("clothing"))
			return "clothing";
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
			return "Optics";
		if (nodeId.Contains("muzzle"))
			return "Muzzle";
		if (nodeId.Contains("underbarrel"))
			return "Underbarrel";
		if (nodeId.Contains("headgear"))
			return "Hat";
		if (nodeId.Contains("vest"))
			return "Vest";
		if (nodeId.Contains("backpack"))
			return "Back";
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

	protected bool SpawnPreviewMannequin(HST_CampaignState state, string identityId, int playerId, HST_LoadoutEditorSessionState session)
	{
		DeletePreviewMannequin(identityId);

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
			return false;

		vector previewPosition = ResolvePreviewPosition(state, playerId);
		vector previewAngles = "180 0 0";
		GenericEntity preview = respawnSystem.DoSpawn(PREVIEW_MANNEQUIN_PREFAB, previewPosition, previewAngles);
		if (!preview)
			return false;

		m_aPreviewIdentityIds.Insert(identityId);
		m_aPreviewEntities.Insert(preview);
		session.m_vPreviewPosition = previewPosition;
		session.m_bPreviewSpawned = true;
		session.m_iPreviewItemCount = 0;
		session.m_sPreviewStatus = "preview spawned";
		Print(string.Format("h-istasi loadout editor | preview mannequin spawned for %1 at %2 using %3", identityId, previewPosition, PREVIEW_MANNEQUIN_PREFAB));
		return true;
	}

	protected void DeletePreviewMannequin(string identityId)
	{
		for (int i = m_aPreviewIdentityIds.Count() - 1; i >= 0; i--)
		{
			if (m_aPreviewIdentityIds[i] != identityId)
				continue;

			IEntity preview = m_aPreviewEntities[i];
			if (preview)
				SCR_EntityHelper.DeleteEntityAndChildren(preview);

			m_aPreviewIdentityIds.Remove(i);
			m_aPreviewEntities.Remove(i);
		}
	}

	protected IEntity FindPreviewMannequin(string identityId)
	{
		for (int i = m_aPreviewIdentityIds.Count() - 1; i >= 0; i--)
		{
			if (m_aPreviewIdentityIds[i] == identityId)
				return m_aPreviewEntities[i];
		}

		return null;
	}

	protected bool RefreshPreviewMannequinLoadout(HST_CampaignState state, string identityId, HST_LoadoutEditorSessionState session)
	{
		if (!session)
			return false;

		IEntity preview = FindPreviewMannequin(identityId);
		if (!preview)
		{
			session.m_bPreviewSpawned = false;
			session.m_iPreviewItemCount = CountDraftItems(session);
			session.m_sPreviewStatus = "client render preview";
			return true;
		}

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(preview.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
		{
			session.m_iPreviewItemCount = 0;
			session.m_sPreviewStatus = "preview inventory unavailable";
			return false;
		}

		int removed = ClearInventoryItems(inventory);
		int inserted;
		string lastFailure;
		foreach (HST_LoadoutSlotState slot : session.m_aDraftSlots)
		{
			if (!slot || slot.m_sItemPrefab.IsEmpty())
				continue;

			int previewQuantity = Math.Min(Math.Max(1, slot.m_iQuantity), ResolvePreviewQuantityLimit(slot.m_sCategory));
			for (int index = 0; index < previewQuantity; index++)
			{
				string failure;
				if (TryInsertItemIntoInventory(inventory, preview, slot.m_sItemPrefab, failure))
					inserted++;
				else if (lastFailure.IsEmpty())
					lastFailure = failure;
			}
		}

		session.m_bPreviewSpawned = true;
		session.m_iPreviewItemCount = inserted;
		if (lastFailure.IsEmpty())
			session.m_sPreviewStatus = string.Format("dressed %1 item(s), cleared %2", inserted, removed);
		else
			session.m_sPreviewStatus = string.Format("dressed %1 item(s), skipped %2", inserted, lastFailure);

		Print(string.Format("h-istasi loadout editor | preview mannequin refreshed for %1 | inserted %2 | removed %3 | status %4", identityId, inserted, removed, session.m_sPreviewStatus));
		return lastFailure.IsEmpty();
	}

	protected int ClearInventoryItems(SCR_InventoryStorageManagerComponent inventory)
	{
		if (!inventory)
			return 0;

		array<IEntity> items = {};
		inventory.GetItems(items, EStoragePurpose.PURPOSE_ANY);
		int removed;
		foreach (IEntity item : items)
		{
			if (!item)
				continue;

			if (inventory.TryDeleteItem(item))
				removed++;
		}

		return removed;
	}

	protected int ResolvePreviewQuantityLimit(string category)
	{
		if (category == "magazine" || category == "medical" || category == "explosive")
			return 2;

		return 1;
	}

	protected vector ResolvePreviewPosition(HST_CampaignState state, int playerId)
	{
		if (state && !IsZeroVector(state.m_vArsenalPosition))
		{
			vector hqPreview = state.m_vArsenalPosition;
			hqPreview[0] = hqPreview[0] + 6.0;
			hqPreview[2] = hqPreview[2] + 6.0;
			vector resolvedArsenalPreview;
			if (HST_WorldPositionService.TryResolveGroundPosition(hqPreview, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, resolvedArsenalPreview, true))
				return resolvedArsenalPreview;

			return hqPreview;
		}

		return "0 0 0";
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

	protected bool ValidateLoadoutTransaction(HST_CampaignState state, HST_SavedLoadoutState loadout, string identityId, out string failure)
	{
		failure = "";
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

			if (arsenalItem.m_bUnlocked)
				continue;

			int required = Math.Max(1, slot.m_iQuantity);
			int alreadyIssued = CountIssuedFiniteItem(state, identityId, slot.m_sItemPrefab);
			int additionalRequired = required - alreadyIssued;
			if (additionalRequired <= 0)
				continue;

			if (arsenalItem.m_iCount < additionalRequired)
			{
				failure = string.Format("%1 requires %2 more but only %3 remain", HST_DisplayNameService.ResolveItemDisplayName(null, slot.m_sItemPrefab, slot.m_sDisplayName), additionalRequired, arsenalItem.m_iCount);
				return false;
			}
		}

		if (!ValidateAttachmentCompatibility(loadout, failure))
			return false;

		return true;
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

		Print(string.Format("h-istasi loadout editor | requested physical insertion of %1 item(s) for %2 slot custom loadout", inserted, loadout.m_aSlots.Count()));
		return true;
	}

	protected bool TryInsertItemIntoPlayerInventory(SCR_InventoryStorageManagerComponent inventory, IEntity playerEntity, string prefab, out string failure)
	{
		return TryInsertItemIntoInventory(inventory, playerEntity, prefab, failure);
	}

	protected bool TryInsertItemIntoInventory(SCR_InventoryStorageManagerComponent inventory, IEntity ownerEntity, string prefab, out string failure)
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
			failure = "no storage can accept " + HST_DisplayNameService.ResolveItemDisplayName(null, prefab);
			return false;
		}

		HST_LoadoutEditorInsertCallback callback = new HST_LoadoutEditorInsertCallback();
		callback.m_TemporaryEntity = itemEntity;
		inventory.TryInsertItemInStorage(itemEntity, storage, -1, callback);
		return true;
	}

	protected void CommitLoadoutTransaction(HST_CampaignState state, HST_ArsenalService arsenal, HST_SavedLoadoutState loadout, string identityId)
	{
		ReturnUnneededIssuedItems(state, arsenal, loadout, identityId);
		foreach (HST_LoadoutSlotState slot : loadout.m_aSlots)
		{
			if (!slot || slot.m_sItemPrefab.IsEmpty())
				continue;

			HST_ArsenalItemState arsenalItem = state.FindArsenalItem(slot.m_sItemPrefab);
			if (!arsenalItem)
				continue;

			string category = ResolveEditorCategory(slot.m_sItemPrefab, slot.m_sCategory);
			string displayName = HST_DisplayNameService.ResolveItemDisplayName(null, slot.m_sItemPrefab, slot.m_sDisplayName);
			int required = Math.Max(1, slot.m_iQuantity);
			int alreadyIssued = CountIssuedItem(state, identityId, slot.m_sItemPrefab);

			if (!arsenalItem.m_bUnlocked && required > alreadyIssued)
				arsenal.WithdrawItem(state, slot.m_sItemPrefab, required - alreadyIssued);
			else if (!arsenalItem.m_bUnlocked && alreadyIssued > required)
				arsenal.DepositItem(state, null, slot.m_sItemPrefab, alreadyIssued - required, category, displayName);

			SetIssuedItem(state, identityId, slot.m_sItemPrefab, displayName, category, required, arsenalItem.m_bUnlocked);
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
				arsenal.DepositItem(state, null, issuedItem.m_sItemPrefab, issuedItem.m_iCount, issuedItem.m_sCategory, issuedItem.m_sDisplayName);

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
			if (slot && slot.m_sItemPrefab == prefab)
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
		int backpack;
		int weapon;
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
			else if (category == "backpack")
				backpack++;
			else if (category == "weapon" || category == "launcher")
				weapon++;
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

		string firstHalf = string.Format("clothing %1 / head %2 / vest %3 / pack %4 / weapons %5", clothing, headgear, vest, backpack, weapon);
		string secondHalf = string.Format("mags %1 / explosives %2 / attachments %3 / medical %4 / utility %5", magazine, explosive, attachment, medical, utility);
		return firstHalf + " / " + secondHalf;
	}

	protected string ResolveEditorCategory(string prefab, string sourceCategory)
	{
		if (!sourceCategory.IsEmpty() && sourceCategory != "equipment")
		{
			if (sourceCategory == "launcher")
				return "weapon";

			return sourceCategory;
		}

		if (prefab.Contains("Uniform") || prefab.Contains("Jacket") || prefab.Contains("Pants") || prefab.Contains("Shirt") || prefab.Contains("Clothes"))
			return "clothing";
		if (prefab.Contains("Helmet") || prefab.Contains("Hat") || prefab.Contains("Headgear") || prefab.Contains("Cap"))
			return "headgear";
		if (prefab.Contains("Vest") || prefab.Contains("Carrier") || prefab.Contains("Webbing"))
			return "vest";
		if (prefab.Contains("Backpack") || prefab.Contains("Bag") || prefab.Contains("Pack_"))
			return "backpack";
		if (prefab.Contains("Optic") || prefab.Contains("Sight") || prefab.Contains("Muzzle") || prefab.Contains("Suppressor") || prefab.Contains("Bipod") || prefab.Contains("Attachment"))
			return "attachment";
		if (prefab.Contains("Magazine") || prefab.Contains("magazine"))
			return "magazine";
		if (prefab.Contains("Grenade") || prefab.Contains("Mine") || prefab.Contains("Explosive"))
			return "explosive";
		if (prefab.Contains("Bandage") || prefab.Contains("Morphine") || prefab.Contains("Tourniquet") || prefab.Contains("Medical") || prefab.Contains("Medkit"))
			return "medical";
		if (prefab.Contains("Launcher") || prefab.Contains("RPG") || prefab.Contains("M72") || prefab.Contains("AT4"))
			return "weapon";
		if (prefab.Contains("Weapon") || prefab.Contains("Rifle") || prefab.Contains("Pistol") || prefab.Contains("MG_") || prefab.Contains("SMG"))
			return "weapon";

		return "utility";
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

	protected bool IsAllowedLoadoutSlot(HST_LoadoutSlotState slot)
	{
		return slot && !slot.m_sSlotId.IsEmpty() && !slot.m_sItemPrefab.IsEmpty() && !IsRemovedExternalPrefab(slot.m_sItemPrefab);
	}

	protected int PurgeRemovedExternalLoadoutState(HST_CampaignState state, string identityId)
	{
		if (!state)
			return 0;

		int purged;
		for (int arsenalIndex = state.m_aArsenalItems.Count() - 1; arsenalIndex >= 0; arsenalIndex--)
		{
			HST_ArsenalItemState item = state.m_aArsenalItems[arsenalIndex];
			if (!item || !IsRemovedExternalPrefab(item.m_sPrefab))
				continue;

			state.m_aArsenalItems.Remove(arsenalIndex);
			purged++;
		}

		for (int issuedIndex = state.m_aIssuedLoadoutItems.Count() - 1; issuedIndex >= 0; issuedIndex--)
		{
			HST_IssuedLoadoutItemState issuedItem = state.m_aIssuedLoadoutItems[issuedIndex];
			if (!issuedItem || issuedItem.m_sOwnerIdentityId != identityId || !IsRemovedExternalPrefab(issuedItem.m_sItemPrefab))
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
				if (!slot || !IsRemovedExternalPrefab(slot.m_sItemPrefab))
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
			if (!slot || !IsRemovedExternalPrefab(slot.m_sItemPrefab))
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

		return prefab.Contains("595F2BF") || prefab.Contains("1337C0DE") || prefab.Contains("BADC0DED") || prefab.Contains("StatusQuo") || prefab.Contains("ContentPack");
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

		return loaded;
	}

	protected bool SavePersonalLoadoutsToFile(HST_CampaignState state, string identityId)
	{
		if (!state || identityId.IsEmpty())
			return false;

		FileIO.MakeDirectory("$profile:h-istasi");
		FileIO.MakeDirectory(LOADOUT_DIRECTORY);

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

	protected string BuildPersonalLoadoutPath(string identityId)
	{
		return LOADOUT_DIRECTORY + "/" + SafeIdentityId(identityId) + ".json";
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
