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
			return "h-istasi native markers | manager missing";

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
			"h-istasi native markers | native static %1 | native disabled %2 | tracked static %3 active %4 disabled %5 missing %6 | dynamic %7",
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
		if (handle && IsStaticHandleLive(manager, handle) && handle.m_sRevisionSignature == signature)
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

		SCR_MapMarkerBase nativeMarker = new SCR_MapMarkerBase();
		nativeMarker.SetType(record.m_eMarkerType);
		nativeMarker.SetMarkerConfigID(record.m_iConfigId);
		nativeMarker.SetMarkerOwnerID(-1);
		nativeMarker.SetWorldPos(Math.Round(record.m_vWorldPosition[0]), Math.Round(record.m_vWorldPosition[2]));
		nativeMarker.SetCustomText(record.m_sShortLabel);
		nativeMarker.SetIconEntry(record.m_iIconEntry);
		nativeMarker.SetColorEntry(record.m_iColorEntry);
		nativeMarker.SetMarkerFactionFlags(record.m_iFactionFlags);
		nativeMarker.SetCanBeRemovedByOwner(record.m_bCanPlayerRemove);
		nativeMarker.SetTimestampVisibility(false);

		manager.InsertStaticMarker(nativeMarker, record.m_bLocalOnly, record.m_bServerMarker);

		HST_NativeStaticMarkerHandle handle = new HST_NativeStaticMarkerHandle();
		handle.m_sDomainId = record.m_sId;
		handle.m_iNativeMarkerId = nativeMarker.GetMarkerID();
		handle.m_Marker = nativeMarker;
		handle.m_sRevisionSignature = signature;
		m_mStaticDomainIdToMarker.Set(record.m_sId, handle);
		m_mPublishedRevisionByDomainId.Set(record.m_sId, record.m_iRevision);
		return true;
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
		SCR_MapMarkerEntity markerEntity = m_mDynamicDomainIdToMarkerEntity.Get(id);
		if (!markerEntity)
			return false;

		if (manager)
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

		Print("h-istasi map marker reconciler debug | " + message);
	}
}
