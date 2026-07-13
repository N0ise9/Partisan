class HST_NativeMapMarkerReconciler
{
	protected ref map<string, ref HST_NativeStaticMarkerHandle> m_mStaticDomainIdToMarker = new map<string, ref HST_NativeStaticMarkerHandle>();
	protected ref map<string, SCR_MapMarkerEntity> m_mDynamicDomainIdToMarkerEntity = new map<string, SCR_MapMarkerEntity>();
	protected ref map<string, int> m_mPublishedRevisionByDomainId = new map<string, int>();
	protected ref HST_MapMarkerReconcileResult m_Result = new HST_MapMarkerReconcileResult();
	protected bool m_bDebugLoggingEnabled;

	void SetDebugLoggingEnabled(bool enabled)
	{
		m_bDebugLoggingEnabled = enabled;
	}

	HST_MapMarkerReconcileResult GetLastResult()
	{
		return m_Result;
	}

	int GetTrackedStaticHandleCount()
	{
		return m_mStaticDomainIdToMarker.Count();
	}

	int GetTrackedDynamicHandleCount()
	{
		return m_mDynamicDomainIdToMarkerEntity.Count();
	}

	int CountTrackedDynamicLive()
	{
		return CountTrackedDynamicLive(ResolveMarkerManager());
	}

	int CountTrackedDynamicLive(SCR_MapMarkerManagerComponent manager)
	{
		if (!manager)
			return 0;

		int count;
		foreach (string id, SCR_MapMarkerEntity markerEntity : m_mDynamicDomainIdToMarkerEntity)
		{
			if (IsDynamicMarkerLive(manager, markerEntity))
				count++;
		}

		return count;
	}

	int CountTrackedStaticActive(SCR_MapMarkerManagerComponent manager)
	{
		if (!manager)
			return 0;

		int count;
		foreach (string id, HST_NativeStaticMarkerHandle handle : m_mStaticDomainIdToMarker)
		{
			if (IsHandleInStaticArray(manager, handle))
				count++;
		}

		return count;
	}

	int CountTrackedStaticDisabled(SCR_MapMarkerManagerComponent manager)
	{
		if (!manager)
			return 0;

		int count;
		foreach (string id, HST_NativeStaticMarkerHandle handle : m_mStaticDomainIdToMarker)
		{
			if (!IsHandleInStaticArray(manager, handle) && IsHandleInDisabledArray(manager, handle))
				count++;
		}

		return count;
	}

	int CountTrackedStaticMissing(SCR_MapMarkerManagerComponent manager)
	{
		if (!manager)
			return 0;

		int count;
		foreach (string id, HST_NativeStaticMarkerHandle handle : m_mStaticDomainIdToMarker)
		{
			if (!IsHandleInStaticArray(manager, handle) && !IsHandleInDisabledArray(manager, handle))
				count++;
		}

		return count;
	}

	bool IsDomainIdLive(SCR_MapMarkerManagerComponent manager, string domainId)
	{
		if (!manager || domainId.IsEmpty())
			return false;
		HST_NativeStaticMarkerHandle staticHandle = m_mStaticDomainIdToMarker.Get(domainId);
		if (staticHandle && IsStaticHandleLive(manager, staticHandle))
			return true;
		SCR_MapMarkerEntity dynamicMarker = m_mDynamicDomainIdToMarkerEntity.Get(domainId);
		return dynamicMarker && IsDynamicMarkerLive(manager, dynamicMarker);
	}

	bool IsProjectionIntegrityCurrent(
		SCR_MapMarkerManagerComponent manager,
		notnull map<string, ref HST_MapMarkerRecord> desired)
	{
		if (!manager)
			return false;

		foreach (string staticId, HST_NativeStaticMarkerHandle staleStaticHandle : m_mStaticDomainIdToMarker)
		{
			if (!desired.Contains(staticId))
				return false;
		}
		foreach (string dynamicId, SCR_MapMarkerEntity staleDynamicMarker : m_mDynamicDomainIdToMarkerEntity)
		{
			if (!desired.Contains(dynamicId))
				return false;
		}

		foreach (string id, HST_MapMarkerRecord record : desired)
		{
			if (!record || !record.m_bVisible)
				return false;
			if (record.m_eRenderMode == HST_EMapMarkerRenderMode.AREA_OVERLAY)
				continue;
			if (record.m_eRenderMode == HST_EMapMarkerRenderMode.DYNAMIC_ENTITY)
			{
				SCR_MapMarkerEntity dynamicMarker = m_mDynamicDomainIdToMarkerEntity.Get(id);
				if (!dynamicMarker || !IsDynamicMarkerLive(manager, dynamicMarker))
					return false;
				if (dynamicMarker.GetType() != record.m_eMarkerType
					|| dynamicMarker.GetMarkerConfigID() != record.m_iConfigId
					|| dynamicMarker.GetTarget() != record.m_TargetEntity
					|| dynamicMarker.GetText() != record.m_sShortLabel)
					return false;
				continue;
			}

			HST_NativeStaticMarkerHandle handle = m_mStaticDomainIdToMarker.Get(id);
			SCR_MapMarkerBase liveMarker = ResolveLiveStaticMarker(manager, handle);
			if (!handle
				|| !liveMarker
				|| handle.m_sRevisionSignature != record.BuildNativeSignature()
				|| handle.m_sIntegritySignature != BuildStaticIntegritySignature(liveMarker))
				return false;
		}

		return true;
	}

	bool Reconcile(notnull map<string, ref HST_MapMarkerRecord> desired)
	{
		return Reconcile(ResolveMarkerManager(), desired);
	}

	bool Reconcile(SCR_MapMarkerManagerComponent manager, notnull map<string, ref HST_MapMarkerRecord> desired)
	{
		m_Result.Reset();
		m_Result.m_iDesired = desired.Count();
		if (!manager)
		{
			m_Result.m_iFailed = desired.Count();
			return false;
		}

		RemoveStaleMarkers(manager, desired);

		foreach (string id, HST_MapMarkerRecord record : desired)
		{
			if (!record || !record.m_bVisible)
			{
				RemovePublishedMarker(manager, id);
				continue;
			}

			if (record.m_eRenderMode == HST_EMapMarkerRenderMode.DYNAMIC_ENTITY)
				ReconcileDynamic(manager, record);
			else if (record.m_eRenderMode == HST_EMapMarkerRenderMode.AREA_OVERLAY)
				m_Result.m_iUnchanged++;
			else
				ReconcileStatic(manager, record);
		}

		m_Result.m_iPublishedStatic = CountLiveStaticMarkers(manager);
		m_Result.m_iPublishedDynamic = m_mDynamicDomainIdToMarkerEntity.Count();
		return m_Result.m_bChanged;
	}

	bool Clear()
	{
		return Clear(ResolveMarkerManager());
	}

	bool Clear(SCR_MapMarkerManagerComponent manager)
	{
		m_Result.Reset();
		bool changed;

		array<string> staticIds = {};
		foreach (string id, HST_NativeStaticMarkerHandle handle : m_mStaticDomainIdToMarker)
			staticIds.Insert(id);

		foreach (string staticId : staticIds)
		{
			if (RemoveStatic(manager, staticId))
			{
				m_Result.m_iRemoved++;
				changed = true;
			}
		}

		array<string> dynamicIds = {};
		foreach (string dynamicId, SCR_MapMarkerEntity markerEntity : m_mDynamicDomainIdToMarkerEntity)
			dynamicIds.Insert(dynamicId);

		foreach (string idToRemove : dynamicIds)
		{
			if (RemoveDynamic(manager, idToRemove))
			{
				m_Result.m_iRemoved++;
				changed = true;
			}
		}

		m_mPublishedRevisionByDomainId.Clear();
		m_Result.m_bChanged = changed;
		return changed;
	}

	string BuildRuntimeReport()
	{
		return BuildDetailedRuntimeReport(ResolveMarkerManager());
	}

	string BuildDetailedRuntimeReport(SCR_MapMarkerManagerComponent manager)
	{
		if (!manager)
			return "Partisan native markers | manager missing";

		int nativeStatic = manager.GetStaticMarkers().Count();
		int nativeDisabled = manager.GetDisabledMarkers().Count();
		int trackedStatic;
		int trackedActive;
		int trackedDisabled;
		int trackedMissing;

		foreach (string id, HST_NativeStaticMarkerHandle handle : m_mStaticDomainIdToMarker)
		{
			trackedStatic++;
			if (IsHandleInStaticArray(manager, handle))
				trackedActive++;
			else if (IsHandleInDisabledArray(manager, handle))
				trackedDisabled++;
			else
				trackedMissing++;
		}

		string report = string.Format(
			"Partisan native markers | native static %1 | native disabled %2 | tracked static %3 active %4 disabled %5 missing %6 | dynamic %7",
			nativeStatic,
			nativeDisabled,
			trackedStatic,
			trackedActive,
			trackedDisabled,
			trackedMissing,
			m_mDynamicDomainIdToMarkerEntity.Count());
		return report + string.Format(
			" | created %1 updated %2 removed %3 unchanged %4 failed %5",
			m_Result.m_iCreated,
			m_Result.m_iUpdated,
			m_Result.m_iRemoved,
			m_Result.m_iUnchanged,
			m_Result.m_iFailed);
	}

	bool CampaignDebugResolveTrackedStaticMarker(
		SCR_MapMarkerManagerComponent manager,
		notnull map<string, ref HST_MapMarkerRecord> desired,
		out string domainId,
		out SCR_MapMarkerBase liveMarker,
		out string revisionSignature,
		out string integritySignature)
	{
		domainId = "";
		liveMarker = null;
		revisionSignature = "";
		integritySignature = "";
		if (!manager)
			return false;

		array<string> zoneCandidateIds = {};
		array<string> fallbackCandidateIds = {};
		foreach (string id, HST_NativeStaticMarkerHandle handle : m_mStaticDomainIdToMarker)
		{
			HST_MapMarkerRecord record = desired.Get(id);
			if (!record || !record.m_bVisible || record.m_bCanPlayerRemove
				|| record.m_eRenderMode == HST_EMapMarkerRenderMode.DYNAMIC_ENTITY
				|| record.m_eRenderMode == HST_EMapMarkerRenderMode.AREA_OVERLAY)
				continue;
			// RemoveStaticMarker only removes local markers from the active array.
			// Keep this destructive debug seam away from disabled and player-authored markers.
			if (!IsHandleInStaticArray(manager, handle))
				continue;
			if (id.IndexOf("hst_zone_") == 0)
				zoneCandidateIds.Insert(id);
			else
				fallbackCandidateIds.Insert(id);
		}
		zoneCandidateIds.Sort();
		fallbackCandidateIds.Sort();
		if (!zoneCandidateIds.IsEmpty())
			domainId = zoneCandidateIds[0];
		else if (!fallbackCandidateIds.IsEmpty())
			domainId = fallbackCandidateIds[0];
		else
			return false;

		HST_NativeStaticMarkerHandle selected = m_mStaticDomainIdToMarker.Get(domainId);
		liveMarker = ResolveLiveStaticMarker(manager, selected);
		if (!selected || !liveMarker)
			return false;
		revisionSignature = selected.m_sRevisionSignature;
		integritySignature = selected.m_sIntegritySignature;
		return true;
	}

	bool CampaignDebugResolveTrackedStaticMarkerById(
		SCR_MapMarkerManagerComponent manager,
		string domainId,
		out SCR_MapMarkerBase liveMarker,
		out string revisionSignature,
		out string integritySignature)
	{
		liveMarker = null;
		revisionSignature = "";
		integritySignature = "";
		if (!manager || domainId.IsEmpty())
			return false;

		HST_NativeStaticMarkerHandle handle = m_mStaticDomainIdToMarker.Get(domainId);
		liveMarker = ResolveLiveStaticMarker(manager, handle);
		if (!handle || !liveMarker)
			return false;
		revisionSignature = handle.m_sRevisionSignature;
		integritySignature = handle.m_sIntegritySignature;
		return true;
	}

	bool CampaignDebugMutateTrackedStaticMarker(
		SCR_MapMarkerManagerComponent manager,
		string domainId)
	{
		SCR_MapMarkerBase marker;
		string revisionSignature;
		string integritySignature;
		if (!CampaignDebugResolveTrackedStaticMarkerById(
			manager,
			domainId,
			marker,
			revisionSignature,
			integritySignature))
			return false;

		int position[2];
		marker.GetWorldPos(position);
		marker.SetMarkerOwnerID(1);
		marker.SetCanBeRemovedByOwner(true);
		marker.SetWorldPos(position[0] + 91, position[1] + 73);
		marker.SetCustomText(marker.GetCustomText() + " [integrity probe]");
		return true;
	}

	bool CampaignDebugRemoveTrackedStaticMarkerFromManager(
		SCR_MapMarkerManagerComponent manager,
		string domainId)
	{
		SCR_MapMarkerBase marker;
		string revisionSignature;
		string integritySignature;
		if (!CampaignDebugResolveTrackedStaticMarkerById(
			manager,
			domainId,
			marker,
			revisionSignature,
			integritySignature))
			return false;

		manager.RemoveStaticMarker(marker);
		return ResolveLiveStaticMarker(manager, m_mStaticDomainIdToMarker.Get(domainId)) == null;
	}

	bool CampaignDebugIsTrackedStaticCanonical(
		SCR_MapMarkerManagerComponent manager,
		notnull map<string, ref HST_MapMarkerRecord> desired,
		string domainId)
	{
		if (!manager || domainId.IsEmpty())
			return false;
		HST_MapMarkerRecord record = desired.Get(domainId);
		HST_NativeStaticMarkerHandle handle = m_mStaticDomainIdToMarker.Get(domainId);
		SCR_MapMarkerBase marker = ResolveLiveStaticMarker(manager, handle);
		if (!record || !handle || !marker)
			return false;
		return marker.GetMarkerOwnerID() == -1
			&& !marker.CanBeRemovedByOwner()
			&& handle.m_sRevisionSignature == record.BuildNativeSignature()
			&& handle.m_sIntegritySignature == BuildStaticIntegritySignature(marker);
	}

	int CampaignDebugCountCanonicalStaticMatches(
		SCR_MapMarkerManagerComponent manager,
		string integritySignature)
	{
		if (!manager || integritySignature.IsEmpty())
			return 0;

		int count;
		foreach (SCR_MapMarkerBase marker : manager.GetStaticMarkers())
		{
			if (marker && BuildStaticIntegritySignature(marker) == integritySignature)
				count++;
		}
		foreach (SCR_MapMarkerBase disabledMarker : manager.GetDisabledMarkers())
		{
			if (disabledMarker && BuildStaticIntegritySignature(disabledMarker) == integritySignature)
				count++;
		}
		return count;
	}

	protected void RemoveStaleMarkers(SCR_MapMarkerManagerComponent manager, notnull map<string, ref HST_MapMarkerRecord> desired)
	{
		array<string> staleStaticIds = {};
		foreach (string id, HST_NativeStaticMarkerHandle handle : m_mStaticDomainIdToMarker)
		{
			if (!desired.Contains(id))
				staleStaticIds.Insert(id);
		}

		foreach (string staticId : staleStaticIds)
		{
			if (RemoveStatic(manager, staticId))
			{
				m_Result.m_iRemoved++;
				m_Result.m_bChanged = true;
			}
		}

		array<string> staleDynamicIds = {};
		foreach (string dynamicId, SCR_MapMarkerEntity markerEntity : m_mDynamicDomainIdToMarkerEntity)
		{
			if (!desired.Contains(dynamicId))
				staleDynamicIds.Insert(dynamicId);
		}

		foreach (string idToRemove : staleDynamicIds)
		{
			if (RemoveDynamic(manager, idToRemove))
			{
				m_Result.m_iRemoved++;
				m_Result.m_bChanged = true;
			}
		}
	}

	protected void ReconcileStatic(SCR_MapMarkerManagerComponent manager, HST_MapMarkerRecord record)
	{
		if (!manager || !record)
			return;

		if (m_mDynamicDomainIdToMarkerEntity.Contains(record.m_sId))
		{
			if (RemoveDynamic(manager, record.m_sId))
			{
				m_Result.m_iRemoved++;
				m_Result.m_bChanged = true;
			}
		}

		string signature = record.BuildNativeSignature();
		HST_NativeStaticMarkerHandle handle = m_mStaticDomainIdToMarker.Get(record.m_sId);
		SCR_MapMarkerBase liveMarker;
		if (handle)
			liveMarker = ResolveLiveStaticMarker(manager, handle);
		if (handle
			&& liveMarker
			&& handle.m_sRevisionSignature == signature
			&& handle.m_sIntegritySignature == BuildStaticIntegritySignature(liveMarker))
		{
			m_Result.m_iUnchanged++;
			return;
		}

		if (handle)
		{
			RemoveStatic(manager, record.m_sId);
			m_Result.m_iUpdated++;
			m_Result.m_bChanged = true;
		}

		if (CreateStatic(manager, record, signature))
		{
			m_Result.m_iCreated++;
			m_Result.m_bChanged = true;
			return;
		}

		m_Result.m_iFailed++;
	}

	protected bool CreateStatic(SCR_MapMarkerManagerComponent manager, HST_MapMarkerRecord record, string signature)
	{
		if (!manager || !record)
			return false;

		int iconEntry = ResolveValidIconEntry(manager, record);
		if (iconEntry < 0)
		{
			DebugLog(string.Format("create static rejected id=%1 because no valid placed-marker icon resource is available", record.m_sId));
			return false;
		}

		SCR_MapMarkerBase nativeMarker = new SCR_MapMarkerBase();
		nativeMarker.SetType(record.m_eMarkerType);
		nativeMarker.SetMarkerConfigID(record.m_iConfigId);
		nativeMarker.SetMarkerOwnerID(-1);
		nativeMarker.SetWorldPos(Math.Round(record.m_vWorldPosition[0]), Math.Round(record.m_vWorldPosition[2]));
		nativeMarker.SetCustomText(record.m_sShortLabel);
		nativeMarker.SetIconEntry(iconEntry);
		nativeMarker.SetColorEntry(record.m_iColorEntry);
		nativeMarker.SetMarkerFactionFlags(record.m_iFactionFlags);
		nativeMarker.SetCanBeRemovedByOwner(record.m_bCanPlayerRemove);
		nativeMarker.SetTimestampVisibility(false);

		// The native local insertion path assigns the local player before creating the widget.
		// Protected projection records need system ownership from their first visible frame.
		if (record.m_bLocalOnly && !record.m_bCanPlayerRemove)
			manager.HST_InsertProtectedLocalStaticMarker(nativeMarker);
		else
			manager.InsertStaticMarker(nativeMarker, record.m_bLocalOnly, record.m_bServerMarker);

		if (!record.m_bCanPlayerRemove)
		{
			nativeMarker.SetMarkerOwnerID(-1);
			nativeMarker.SetCanBeRemovedByOwner(false);
		}

		HST_NativeStaticMarkerHandle handle = new HST_NativeStaticMarkerHandle();
		handle.m_sDomainId = record.m_sId;
		handle.m_iNativeMarkerId = nativeMarker.GetMarkerID();
		handle.m_Marker = nativeMarker;
		handle.m_sRevisionSignature = signature;
		handle.m_sIntegritySignature = BuildStaticIntegritySignature(nativeMarker);
		m_mStaticDomainIdToMarker.Set(record.m_sId, handle);
		m_mPublishedRevisionByDomainId.Set(record.m_sId, record.m_iRevision);
		return true;
	}

	protected string BuildStaticIntegritySignature(SCR_MapMarkerBase marker)
	{
		if (!marker)
			return "";

		int position[2];
		marker.GetWorldPos(position);
		string signature = string.Format("%1|%2|%3|%4|%5|%6|%7|%8|%9",
			marker.GetType(),
			marker.GetMarkerConfigID(),
			marker.GetMarkerOwnerID(),
			position[0],
			position[1],
			marker.GetCustomText(),
			marker.GetIconEntry(),
			marker.GetColorEntry(),
			marker.GetMarkerFactionFlags());
		return signature + string.Format("|%1|%2|%3|%4",
			marker.GetFlags(),
			marker.GetRotation(),
			marker.CanBeRemovedByOwner(),
			marker.IsTimestampVisible());
	}

	protected int ResolveValidIconEntry(SCR_MapMarkerManagerComponent manager, HST_MapMarkerRecord record)
	{
		if (!manager || !record)
			return -1;

		int requestedIconEntry = record.m_iIconEntry;
		if (!record.m_sIconQuad.IsEmpty())
		{
			requestedIconEntry = manager.HST_EnsurePlacedIconEntry(
				record.m_sIconImageset,
				record.m_sIconGlowImageset,
				record.m_sIconQuad,
				"general");
		}

		return manager.HST_ResolveValidPlacedIconEntry(requestedIconEntry, SCR_EScenarioFrameworkMarkerCustom.OBJECTIVE_MARKER);
	}

	protected void ReconcileDynamic(SCR_MapMarkerManagerComponent manager, HST_MapMarkerRecord record)
	{
		if (!record.m_TargetEntity)
		{
			ReconcileStatic(manager, record);
			return;
		}

		if (m_mStaticDomainIdToMarker.Contains(record.m_sId))
		{
			if (RemoveStatic(manager, record.m_sId))
			{
				m_Result.m_iRemoved++;
				m_Result.m_bChanged = true;
			}
		}

		SCR_MapMarkerEntity markerEntity = m_mDynamicDomainIdToMarkerEntity.Get(record.m_sId);
		if (markerEntity && !IsDynamicMarkerLive(manager, markerEntity))
		{
			RemoveDynamic(manager, record.m_sId);
			m_Result.m_iRemoved++;
			m_Result.m_bChanged = true;
			markerEntity = null;
		}

		if (!markerEntity)
		{
			markerEntity = manager.InsertDynamicMarker(record.m_eMarkerType, record.m_TargetEntity, record.m_iConfigId);
			if (!markerEntity)
			{
				m_Result.m_iFailed++;
				return;
			}

			m_mDynamicDomainIdToMarkerEntity.Set(record.m_sId, markerEntity);
			m_Result.m_iCreated++;
			m_Result.m_bChanged = true;
		}
		else
		{
			m_Result.m_iUnchanged++;
		}

		markerEntity.SetTarget(record.m_TargetEntity);
		markerEntity.SetText(record.m_sShortLabel);
		markerEntity.SetGlobalVisible(record.m_bVisible);

		Faction markerFaction = ResolveFaction(record.m_sFactionKey);
		if (markerFaction)
			markerEntity.SetFaction(markerFaction);

		m_mPublishedRevisionByDomainId.Set(record.m_sId, record.m_iRevision);
	}

	protected bool IsDynamicMarkerLive(SCR_MapMarkerManagerComponent manager, SCR_MapMarkerEntity markerEntity)
	{
		if (!manager || !markerEntity)
			return false;

		array<SCR_MapMarkerEntity> dynamicMarkers = manager.GetDynamicMarkers();
		return dynamicMarkers.Contains(markerEntity);
	}

	protected bool RemovePublishedMarker(SCR_MapMarkerManagerComponent manager, string id)
	{
		bool removed;
		if (RemoveStatic(manager, id))
			removed = true;
		if (RemoveDynamic(manager, id))
			removed = true;

		if (removed)
		{
			m_Result.m_iRemoved++;
			m_Result.m_bChanged = true;
		}

		return removed;
	}

	protected bool RemoveStatic(SCR_MapMarkerManagerComponent manager, string id)
	{
		HST_NativeStaticMarkerHandle handle = m_mStaticDomainIdToMarker.Get(id);
		if (!handle)
			return false;

		if (manager)
		{
			SCR_MapMarkerBase marker = ResolveLiveStaticMarker(manager, handle);
			if (marker)
				manager.RemoveStaticMarker(marker);
			else
				DebugLog(string.Format("remove static orphan handle id=%1 native=%2 not found in static or disabled arrays", id, handle.m_iNativeMarkerId));
		}

		m_mStaticDomainIdToMarker.Remove(id);
		m_mPublishedRevisionByDomainId.Remove(id);
		return true;
	}

	protected bool RemoveDynamic(SCR_MapMarkerManagerComponent manager, string id)
	{
		if (!m_mDynamicDomainIdToMarkerEntity.Contains(id))
			return false;

		SCR_MapMarkerEntity markerEntity = m_mDynamicDomainIdToMarkerEntity.Get(id);
		if (manager && markerEntity)
			manager.RemoveDynamicMarker(markerEntity);

		m_mDynamicDomainIdToMarkerEntity.Remove(id);
		m_mPublishedRevisionByDomainId.Remove(id);
		return true;
	}

	protected bool IsStaticHandleLive(SCR_MapMarkerManagerComponent manager, HST_NativeStaticMarkerHandle handle)
	{
		return ResolveLiveStaticMarker(manager, handle) != null;
	}

	protected SCR_MapMarkerBase ResolveLiveStaticMarker(SCR_MapMarkerManagerComponent manager, HST_NativeStaticMarkerHandle handle)
	{
		if (!manager || !handle)
			return null;

		if (handle.m_iNativeMarkerId >= 0)
		{
			SCR_MapMarkerBase marker = manager.GetStaticMarkerByID(handle.m_iNativeMarkerId);
			if (marker)
				return marker;

			marker = ResolveDisabledStaticMarkerById(manager, handle.m_iNativeMarkerId);
			if (marker)
				return marker;
		}

		array<SCR_MapMarkerBase> staticMarkers = manager.GetStaticMarkers();
		foreach (SCR_MapMarkerBase marker : staticMarkers)
		{
			if (marker == handle.m_Marker)
				return marker;
		}

		array<SCR_MapMarkerBase> disabledMarkers = manager.GetDisabledMarkers();
		foreach (SCR_MapMarkerBase disabledMarker : disabledMarkers)
		{
			if (disabledMarker == handle.m_Marker)
				return disabledMarker;
		}

		return null;
	}

	protected SCR_MapMarkerBase ResolveDisabledStaticMarkerById(SCR_MapMarkerManagerComponent manager, int markerId)
	{
		if (!manager || markerId < 0)
			return null;

		array<SCR_MapMarkerBase> disabledMarkers = manager.GetDisabledMarkers();
		foreach (SCR_MapMarkerBase marker : disabledMarkers)
		{
			if (marker && marker.GetMarkerID() == markerId)
				return marker;
		}

		return null;
	}

	protected int CountLiveStaticMarkers(SCR_MapMarkerManagerComponent manager)
	{
		if (!manager)
			return 0;

		int count;
		foreach (string id, HST_NativeStaticMarkerHandle handle : m_mStaticDomainIdToMarker)
		{
			if (IsStaticHandleLive(manager, handle))
				count++;
		}

		return count;
	}

	protected bool IsHandleInStaticArray(SCR_MapMarkerManagerComponent manager, HST_NativeStaticMarkerHandle handle)
	{
		if (!manager || !handle)
			return false;

		array<SCR_MapMarkerBase> staticMarkers = manager.GetStaticMarkers();
		foreach (SCR_MapMarkerBase marker : staticMarkers)
		{
			if (!marker)
				continue;
			if (marker == handle.m_Marker)
				return true;
			if (handle.m_iNativeMarkerId >= 0 && marker.GetMarkerID() == handle.m_iNativeMarkerId)
				return true;
		}

		return false;
	}

	protected bool IsHandleInDisabledArray(SCR_MapMarkerManagerComponent manager, HST_NativeStaticMarkerHandle handle)
	{
		if (!manager || !handle)
			return false;

		array<SCR_MapMarkerBase> disabledMarkers = manager.GetDisabledMarkers();
		foreach (SCR_MapMarkerBase marker : disabledMarkers)
		{
			if (!marker)
				continue;
			if (marker == handle.m_Marker)
				return true;
			if (handle.m_iNativeMarkerId >= 0 && marker.GetMarkerID() == handle.m_iNativeMarkerId)
				return true;
		}

		return false;
	}

	protected Faction ResolveFaction(string factionKey)
	{
		if (factionKey.IsEmpty())
			return null;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return null;

		return factionManager.GetFactionByKey(factionKey);
	}

	protected SCR_MapMarkerManagerComponent ResolveMarkerManager()
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager)
			return markerManager;

		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;

		return SCR_MapMarkerManagerComponent.Cast(gameMode.FindComponent(SCR_MapMarkerManagerComponent));
	}

	protected void DebugLog(string message)
	{
		if (!m_bDebugLoggingEnabled)
			return;

		Print("Partisan map marker reconciler debug | " + message);
	}
}
