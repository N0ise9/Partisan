class HST_ClientMarkerProjectionRegistry
{
	protected ref map<string, ref HST_MapMarkerState> m_mRecords = new map<string, ref HST_MapMarkerState>();
	protected ref map<string, ref HST_MapMarkerState> m_mSnapshotStaging = new map<string, ref HST_MapMarkerState>();
	protected ref array<bool> m_aSnapshotChunksReceived = {};
	protected bool m_bReady;
	protected int m_iEpoch;
	protected int m_iWatermark;
	protected string m_sRegistryHash = "0";
	protected string m_sSnapshotId;
	protected int m_iSnapshotEpoch;
	protected int m_iSnapshotWatermark;
	protected int m_iSnapshotChunkCount;
	protected int m_iSnapshotExpectedRecords;
	protected string m_sSnapshotExpectedHash;
	protected string m_sLastStatus = "not ready";

	HST_MarkerProjectionApplyResult ApplyPacket(string payload)
	{
		HST_MarkerProjectionPacket packet = HST_MarkerProjectionCodec.DecodePacket(payload);
		if (!packet)
			return Reject("malformed marker projection packet", true);
		if (packet.m_sKind == HST_MarkerProjectionCodec.SNAPSHOT_HEADER)
			return ApplySnapshotChunk(packet);
		if (packet.m_sKind == HST_MarkerProjectionCodec.DELTA_HEADER)
			return ApplyDelta(packet);
		return Reject("unsupported marker projection packet", true);
	}

	bool IsReady()
	{
		return m_bReady;
	}

	int GetEpoch()
	{
		return m_iEpoch;
	}

	int GetWatermark()
	{
		return m_iWatermark;
	}

	int GetRecordCount()
	{
		return m_mRecords.Count();
	}

	int GetLiveRecordCount()
	{
		int count;
		foreach (string id, HST_MapMarkerState marker : m_mRecords)
		{
			if (marker && !marker.m_bTombstone)
				count++;
		}
		return count;
	}

	string GetRegistryHash()
	{
		return m_sRegistryHash;
	}

	string GetLastStatus()
	{
		return m_sLastStatus;
	}

	HST_MapMarkerState FindRecord(string markerId)
	{
		return m_mRecords.Get(markerId);
	}

	void CopyRecords(notnull map<string, ref HST_MapMarkerState> target, bool includeTombstones = true)
	{
		target.Clear();
		foreach (string id, HST_MapMarkerState marker : m_mRecords)
		{
			if (!marker || (!includeTombstones && marker.m_bTombstone))
				continue;
			target.Set(id, HST_MarkerProjectionCodec.CopyMarker(marker));
		}
	}

	void Reset(string reason)
	{
		m_mRecords.Clear();
		ResetSnapshotStaging();
		m_bReady = false;
		m_iEpoch = 0;
		m_iWatermark = 0;
		m_sRegistryHash = "0";
		m_sLastStatus = reason;
	}

	void RequireSnapshot(string reason)
	{
		ResetSnapshotStaging();
		m_bReady = false;
		m_sLastStatus = reason;
	}

	protected HST_MarkerProjectionApplyResult ApplySnapshotChunk(HST_MarkerProjectionPacket packet)
	{
		if (m_bReady && packet.m_iEpoch < m_iEpoch)
			return Reject("stale snapshot epoch", true);
		if (m_bReady && packet.m_iEpoch == m_iEpoch && packet.m_iWatermark < m_iWatermark)
			return Reject("stale snapshot watermark", true);

		if (packet.m_sSnapshotId != m_sSnapshotId)
		{
			ResetSnapshotStaging();
			m_sSnapshotId = packet.m_sSnapshotId;
			m_iSnapshotEpoch = packet.m_iEpoch;
			m_iSnapshotWatermark = packet.m_iWatermark;
			m_iSnapshotChunkCount = packet.m_iChunkCount;
			m_iSnapshotExpectedRecords = packet.m_iTotalRecordCount;
			m_sSnapshotExpectedHash = packet.m_sRegistryHash;
			m_aSnapshotChunksReceived.Resize(packet.m_iChunkCount);
		}

		if (packet.m_iEpoch != m_iSnapshotEpoch
			|| packet.m_iWatermark != m_iSnapshotWatermark
			|| packet.m_iChunkCount != m_iSnapshotChunkCount
			|| packet.m_iTotalRecordCount != m_iSnapshotExpectedRecords
			|| packet.m_sRegistryHash != m_sSnapshotExpectedHash)
			return Reject("snapshot chunk metadata mismatch", true);

		if (m_aSnapshotChunksReceived[packet.m_iChunkIndex])
		{
			HST_MarkerProjectionApplyResult duplicate = Accept("duplicate snapshot chunk ignored");
			return duplicate;
		}

		foreach (HST_MapMarkerState marker : packet.m_aRecords)
		{
			if (!marker || marker.m_iStreamSequence > packet.m_iWatermark)
				return Reject("snapshot record exceeds watermark", true);
			HST_MapMarkerState existing = m_mSnapshotStaging.Get(marker.m_sMarkerId);
			if (existing && !HST_MarkerProjectionCodec.TransportEquals(existing, marker))
				return Reject("snapshot contains conflicting duplicate marker id", true);
			m_mSnapshotStaging.Set(marker.m_sMarkerId, HST_MarkerProjectionCodec.CopyMarker(marker));
		}
		m_aSnapshotChunksReceived[packet.m_iChunkIndex] = true;

		for (int i; i < m_aSnapshotChunksReceived.Count(); i++)
		{
			if (!m_aSnapshotChunksReceived[i])
				return Accept("snapshot chunk staged");
		}

		if (m_mSnapshotStaging.Count() != m_iSnapshotExpectedRecords)
			return Reject("snapshot record count mismatch", true);
		string stagedHash = HST_MarkerProjectionCodec.BuildLiveRegistryHash(m_mSnapshotStaging);
		if (stagedHash != m_sSnapshotExpectedHash)
			return Reject("snapshot registry hash mismatch", true);

		m_mRecords.Clear();
		foreach (string id, HST_MapMarkerState stagedMarker : m_mSnapshotStaging)
			m_mRecords.Set(id, HST_MarkerProjectionCodec.CopyMarker(stagedMarker));
		m_bReady = true;
		m_iEpoch = m_iSnapshotEpoch;
		m_iWatermark = m_iSnapshotWatermark;
		m_sRegistryHash = stagedHash;

		HST_MarkerProjectionApplyResult result = Accept("snapshot committed");
		result.m_bSnapshotCommitted = true;
		result.m_bRegistryChanged = true;
		result.m_bNeedsAcknowledge = true;
		result.m_iEpoch = m_iEpoch;
		result.m_iAcknowledgeSequence = m_iWatermark;
		result.m_sSnapshotId = m_sSnapshotId;
		result.m_sRegistryHash = m_sRegistryHash;
		ResetSnapshotStaging();
		return result;
	}

	protected HST_MarkerProjectionApplyResult ApplyDelta(HST_MarkerProjectionPacket packet)
	{
		if (!m_bReady)
			return Reject("delta received before snapshot readiness", true);
		if (packet.m_iEpoch != m_iEpoch)
			return Reject("delta epoch mismatch", true);
		if (packet.m_iToSequence <= m_iWatermark)
		{
			HST_MarkerProjectionApplyResult duplicate = Accept("duplicate delta ignored");
			duplicate.m_bNeedsAcknowledge = !packet.m_sRegistryHash.IsEmpty();
			duplicate.m_iEpoch = m_iEpoch;
			duplicate.m_iAcknowledgeSequence = m_iWatermark;
			if (!packet.m_sRegistryHash.IsEmpty())
				duplicate.m_sRegistryHash = m_sRegistryHash;
			return duplicate;
		}
		if (packet.m_iFromSequence != m_iWatermark + 1)
			return Reject(string.Format("delta sequence gap: expected %1, received %2", m_iWatermark + 1, packet.m_iFromSequence), true);
		if (packet.m_aRecords.Count() != packet.m_iToSequence - packet.m_iFromSequence + 1)
			return Reject("delta event count is not contiguous", true);

		map<string, ref HST_MapMarkerState> working = new map<string, ref HST_MapMarkerState>();
		foreach (string currentId, HST_MapMarkerState currentMarker : m_mRecords)
			working.Set(currentId, HST_MarkerProjectionCodec.CopyMarker(currentMarker));

		int expectedSequence = packet.m_iFromSequence;
		foreach (HST_MapMarkerState marker : packet.m_aRecords)
		{
			if (!marker || marker.m_iStreamSequence != expectedSequence)
				return Reject("delta record sequence mismatch", true);
			expectedSequence++;

			HST_MapMarkerState existing = working.Get(marker.m_sMarkerId);
			if (!existing)
			{
				if (marker.m_iRevision != 1)
					return Reject("new delta marker does not begin at revision one", true);
			}
			else
			{
				if (marker.m_iRevision != existing.m_iRevision + 1)
					return Reject("delta marker revision is not the next revision", true);
			}
			working.Set(marker.m_sMarkerId, HST_MarkerProjectionCodec.CopyMarker(marker));
		}

		string workingHash = HST_MarkerProjectionCodec.BuildLiveRegistryHash(working);
		if (!packet.m_sRegistryHash.IsEmpty() && workingHash != packet.m_sRegistryHash)
			return Reject("delta registry hash mismatch", true);

		m_mRecords.Clear();
		foreach (string id, HST_MapMarkerState updatedMarker : working)
			m_mRecords.Set(id, HST_MarkerProjectionCodec.CopyMarker(updatedMarker));
		m_iWatermark = packet.m_iToSequence;
		m_sRegistryHash = workingHash;

		HST_MarkerProjectionApplyResult result = Accept("delta committed");
		result.m_bRegistryChanged = true;
		result.m_bNeedsAcknowledge = !packet.m_sRegistryHash.IsEmpty();
		result.m_iEpoch = m_iEpoch;
		result.m_iAcknowledgeSequence = m_iWatermark;
		if (!packet.m_sRegistryHash.IsEmpty())
			result.m_sRegistryHash = m_sRegistryHash;
		return result;
	}

	protected HST_MarkerProjectionApplyResult Accept(string reason)
	{
		HST_MarkerProjectionApplyResult result = new HST_MarkerProjectionApplyResult();
		result.m_bAccepted = true;
		result.m_iEpoch = m_iEpoch;
		result.m_iAcknowledgeSequence = m_iWatermark;
		result.m_sReason = reason;
		m_sLastStatus = reason;
		return result;
	}

	protected HST_MarkerProjectionApplyResult Reject(string reason, bool needsResync)
	{
		HST_MarkerProjectionApplyResult result = new HST_MarkerProjectionApplyResult();
		result.m_bNeedsResync = needsResync;
		result.m_iEpoch = m_iEpoch;
		result.m_iAcknowledgeSequence = m_iWatermark;
		result.m_sReason = reason;
		m_sLastStatus = reason;
		return result;
	}

	protected void ResetSnapshotStaging(bool clearIdentity = true)
	{
		m_mSnapshotStaging.Clear();
		m_aSnapshotChunksReceived.Clear();
		m_iSnapshotEpoch = 0;
		m_iSnapshotWatermark = 0;
		m_iSnapshotChunkCount = 0;
		m_iSnapshotExpectedRecords = 0;
		m_sSnapshotExpectedHash = "";
		if (clearIdentity)
			m_sSnapshotId = "";
	}
}

class HST_CampaignDebugMarkerIntegrityProbeResult
{
	bool m_bRan;
	bool m_bPreconditionExact;
	bool m_bSystemOwned;
	bool m_bNonRemovable;
	bool m_bMutationDetected;
	bool m_bMutationRepairApplied;
	bool m_bMutationHealed;
	bool m_bDeleteDetected;
	bool m_bDeleteRepairApplied;
	bool m_bDeleteHealed;
	bool m_bRegistryStable;
	bool m_bSingleInstance;
	bool m_bPlayerMarkerInserted;
	bool m_bPlayerMarkerEditable;
	bool m_bPlayerMarkerIsolated;
	bool m_bPlayerMarkerCleaned;
	string m_sDomainId;
	string m_sFailureReason;
	int m_iStaticCountBefore;
	int m_iStaticCountAfter;
	int m_iTrackedCountBefore;
	int m_iTrackedCountAfter;
	int m_iMutationRepairCreated;
	int m_iMutationRepairUpdated;
	int m_iMutationRepairFailed;
	int m_iDeleteRepairCreated;
	int m_iDeleteRepairUpdated;
	int m_iDeleteRepairFailed;

	string BuildReport()
	{
		string report = string.Format(
			"integrityRan %1 | precondition %2 | domain %3 | systemOwned %4 | nonRemovable %5 | mutationDetected %6 | mutationRepairApplied %7 | mutationHealed %8",
			m_bRan,
			m_bPreconditionExact,
			m_sDomainId,
			m_bSystemOwned,
			m_bNonRemovable,
			m_bMutationDetected,
			m_bMutationRepairApplied,
			m_bMutationHealed);
		report = report + string.Format(
			" | deleteDetected %1 | deleteRepairApplied %2 | deleteHealed %3 | registryStable %4 | singleInstance %5 | playerMarkerInserted %6 | playerMarkerEditable %7 | playerMarkerIsolated %8 | playerMarkerCleaned %9",
			m_bDeleteDetected,
			m_bDeleteRepairApplied,
			m_bDeleteHealed,
			m_bRegistryStable,
			m_bSingleInstance,
			m_bPlayerMarkerInserted,
			m_bPlayerMarkerEditable,
			m_bPlayerMarkerIsolated,
			m_bPlayerMarkerCleaned);
		report = report + string.Format(
			" | mutationRepair %1/%2/%3 | deleteRepair %4/%5/%6",
			m_iMutationRepairCreated,
			m_iMutationRepairUpdated,
			m_iMutationRepairFailed,
			m_iDeleteRepairCreated,
			m_iDeleteRepairUpdated,
			m_iDeleteRepairFailed);
		report = report + string.Format(
			" | static %1/%2 | tracked %3/%4",
			m_iStaticCountAfter,
			m_iStaticCountBefore,
			m_iTrackedCountAfter,
			m_iTrackedCountBefore);
		report = report + " | failure " + m_sFailureReason;
		return report;
	}
}

class HST_ClientMarkerProjectionService
{
	protected ref HST_ClientMarkerProjectionRegistry m_Registry = new HST_ClientMarkerProjectionRegistry();
	protected ref HST_CampaignMapMarkerDirector m_Director = new HST_CampaignMapMarkerDirector();
	protected ref HST_NativeMapMarkerReconciler m_Reconciler = new HST_NativeMapMarkerReconciler();
	protected ref map<string, ref HST_MapMarkerRecord> m_mDesired = new map<string, ref HST_MapMarkerRecord>();
	protected ref map<string, MapItem> m_mAuthoredZoneMapItemByZoneId = new map<string, MapItem>();
	protected ref map<string, bool> m_mAuthoredZoneLookupAttempted = new map<string, bool>();
	protected ref map<string, bool> m_mSuppressedAuthoredZoneIds = new map<string, bool>();
	protected ref map<string, bool> m_mAuthoredZonePriorVisibility = new map<string, bool>();
	protected int m_iLastAuthoredZoneDescriptorsHidden;
	protected int m_iLastAuthoredZoneDescriptorsMissing;

	HST_MarkerProjectionApplyResult ReceivePacket(string payload)
	{
		HST_MarkerProjectionApplyResult result = m_Registry.ApplyPacket(payload);
		if (result
			&& result.m_bAccepted
			&& result.m_bRegistryChanged
			&& (result.m_bSnapshotCommitted || result.m_bNeedsAcknowledge))
			ReconcileNativeMarkers();
		return result;
	}

	bool ReconcileNativeMarkers()
	{
		if (!m_Registry.IsReady())
			return false;

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
			return false;

		HST_CampaignState projectionState = new HST_CampaignState();
		map<string, ref HST_MapMarkerState> records = new map<string, ref HST_MapMarkerState>();
		m_Registry.CopyRecords(records, false);
		array<string> markerIds = {};
		foreach (string id, HST_MapMarkerState marker : records)
			markerIds.Insert(id);
		markerIds.Sort();
		foreach (string markerId : markerIds)
			projectionState.m_aMapMarkers.Insert(records.Get(markerId));

		m_Director.BuildDesiredMarkers(projectionState, null, m_mDesired);
		foreach (string markerId, HST_MapMarkerRecord record : m_mDesired)
		{
			if (!record)
				continue;
			record.m_bLocalOnly = true;
			record.m_bServerMarker = false;
		}

		m_Reconciler.Reconcile(markerManager, m_mDesired);
		bool authoredDescriptorsReady = ReconcileAuthoredZoneDescriptors(markerManager, records);
		return m_Reconciler.GetLastResult().m_iFailed == 0 && authoredDescriptorsReady;
	}

	bool RepairNativeMarkersIfNeeded()
	{
		if (!m_Registry.IsReady())
			return false;

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
			return false;
		if (m_Registry.GetLiveRecordCount() > 0 && m_mDesired.Count() == 0)
			return ReconcileNativeMarkers();
		if (m_Reconciler.IsProjectionIntegrityCurrent(markerManager, m_mDesired))
			return true;

		return ReconcileNativeMarkers();
	}

	HST_CampaignDebugMarkerIntegrityProbeResult CampaignDebugRunNativeMarkerIntegrityProbe()
	{
		HST_CampaignDebugMarkerIntegrityProbeResult result
			= new HST_CampaignDebugMarkerIntegrityProbeResult();
		result.m_bRan = true;
		if (!m_Registry || !m_Registry.IsReady())
		{
			result.m_sFailureReason = "client registry is not ready";
			return result;
		}

		SCR_MapMarkerManagerComponent markerManager
			= SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
		{
			result.m_sFailureReason = "native marker manager is unavailable";
			return result;
		}

		RepairNativeMarkersIfNeeded();
		int epochBefore = m_Registry.GetEpoch();
		int watermarkBefore = m_Registry.GetWatermark();
		int registryCountBefore = m_Registry.GetRecordCount();
		int liveCountBefore = m_Registry.GetLiveRecordCount();
		string registryHashBefore = m_Registry.GetRegistryHash();
		int desiredCountBefore = m_mDesired.Count();
		result.m_iTrackedCountBefore = m_Reconciler.GetTrackedStaticHandleCount();
		result.m_iStaticCountBefore = CampaignDebugCountAllStaticMarkers(markerManager);

		SCR_MapMarkerBase campaignMarker;
		string revisionSignature;
		string canonicalIntegritySignature;
		bool selected = m_Reconciler.CampaignDebugResolveTrackedStaticMarker(
			markerManager,
			m_mDesired,
			result.m_sDomainId,
			campaignMarker,
			revisionSignature,
			canonicalIntegritySignature);
		if (!selected || !campaignMarker)
		{
			result.m_sFailureReason = "no protected tracked static campaign marker is available";
			return result;
		}

		result.m_bSystemOwned = campaignMarker.GetMarkerOwnerID() == -1;
		result.m_bNonRemovable = !campaignMarker.CanBeRemovedByOwner();
		result.m_bPreconditionExact = result.m_bSystemOwned
			&& result.m_bNonRemovable
			&& m_Reconciler.CampaignDebugIsTrackedStaticCanonical(
				markerManager,
				m_mDesired,
				result.m_sDomainId)
			&& m_Reconciler.CampaignDebugCountCanonicalStaticMatches(
				markerManager,
				canonicalIntegritySignature) == 1;

		bool mutated = m_Reconciler.CampaignDebugMutateTrackedStaticMarker(
			markerManager,
			result.m_sDomainId);
		result.m_bMutationDetected = mutated
			&& !m_Reconciler.IsProjectionIntegrityCurrent(markerManager, m_mDesired);
		RepairNativeMarkersIfNeeded();
		HST_MapMarkerReconcileResult mutationRepairResult = m_Reconciler.GetLastResult();
		if (mutationRepairResult)
		{
			result.m_iMutationRepairCreated = mutationRepairResult.m_iCreated;
			result.m_iMutationRepairUpdated = mutationRepairResult.m_iUpdated;
			result.m_iMutationRepairFailed = mutationRepairResult.m_iFailed;
		}
		result.m_bMutationRepairApplied = result.m_iMutationRepairCreated > 0
			&& result.m_iMutationRepairUpdated > 0
			&& result.m_iMutationRepairFailed == 0;
		SCR_MapMarkerBase mutationRepairedMarker;
		string mutationRevisionSignature;
		string mutationIntegritySignature;
		bool mutationRepairedResolved
			= m_Reconciler.CampaignDebugResolveTrackedStaticMarkerById(
				markerManager,
				result.m_sDomainId,
				mutationRepairedMarker,
				mutationRevisionSignature,
				mutationIntegritySignature);
		result.m_bMutationHealed = result.m_bMutationDetected
			&& result.m_bMutationRepairApplied
			&& mutationRepairedResolved
			&& mutationRevisionSignature == revisionSignature
			&& mutationIntegritySignature == canonicalIntegritySignature
			&& m_Reconciler.CampaignDebugIsTrackedStaticCanonical(
				markerManager,
				m_mDesired,
				result.m_sDomainId);

		bool removed = m_Reconciler.CampaignDebugRemoveTrackedStaticMarkerFromManager(
			markerManager,
			result.m_sDomainId);
		result.m_bDeleteDetected = removed
			&& !m_Reconciler.IsProjectionIntegrityCurrent(markerManager, m_mDesired);
		RepairNativeMarkersIfNeeded();
		HST_MapMarkerReconcileResult deleteRepairResult = m_Reconciler.GetLastResult();
		if (deleteRepairResult)
		{
			result.m_iDeleteRepairCreated = deleteRepairResult.m_iCreated;
			result.m_iDeleteRepairUpdated = deleteRepairResult.m_iUpdated;
			result.m_iDeleteRepairFailed = deleteRepairResult.m_iFailed;
		}
		result.m_bDeleteRepairApplied = result.m_iDeleteRepairCreated > 0
			&& result.m_iDeleteRepairUpdated > 0
			&& result.m_iDeleteRepairFailed == 0;
		SCR_MapMarkerBase deleteRepairedMarker;
		string deleteRevisionSignature;
		string deleteIntegritySignature;
		bool deleteRepairedResolved
			= m_Reconciler.CampaignDebugResolveTrackedStaticMarkerById(
				markerManager,
				result.m_sDomainId,
				deleteRepairedMarker,
				deleteRevisionSignature,
				deleteIntegritySignature);
		result.m_bDeleteHealed = result.m_bDeleteDetected
			&& result.m_bDeleteRepairApplied
			&& deleteRepairedResolved
			&& deleteRevisionSignature == revisionSignature
			&& deleteIntegritySignature == canonicalIntegritySignature
			&& m_Reconciler.CampaignDebugIsTrackedStaticCanonical(
				markerManager,
				m_mDesired,
				result.m_sDomainId);

		// Always make one final production repair attempt before returning from the
		// destructive probe. A failed assertion must never be allowed to leave the
		// real client map in its deliberately mutated or deleted state.
		RepairNativeMarkersIfNeeded();
		HST_MapMarkerReconcileResult finalRepairResult = m_Reconciler.GetLastResult();
		if (finalRepairResult)
		{
			result.m_iDeleteRepairCreated = finalRepairResult.m_iCreated;
			result.m_iDeleteRepairUpdated = finalRepairResult.m_iUpdated;
			result.m_iDeleteRepairFailed = finalRepairResult.m_iFailed;
		}
		result.m_bDeleteRepairApplied = result.m_iDeleteRepairCreated > 0
			&& result.m_iDeleteRepairUpdated > 0
			&& result.m_iDeleteRepairFailed == 0;
		deleteRepairedResolved
			= m_Reconciler.CampaignDebugResolveTrackedStaticMarkerById(
				markerManager,
				result.m_sDomainId,
				deleteRepairedMarker,
				deleteRevisionSignature,
				deleteIntegritySignature);
		result.m_bDeleteHealed = result.m_bDeleteDetected
			&& result.m_bDeleteRepairApplied
			&& deleteRepairedResolved
			&& deleteRevisionSignature == revisionSignature
			&& deleteIntegritySignature == canonicalIntegritySignature
			&& m_Reconciler.CampaignDebugIsTrackedStaticCanonical(
				markerManager,
				m_mDesired,
				result.m_sDomainId);

		if (deleteRepairedMarker)
			CampaignDebugProbePlayerMarkerIsolation(markerManager, deleteRepairedMarker, result);
		else
			result.m_sFailureReason = "campaign marker did not resolve after final deletion repair";
		RepairNativeMarkersIfNeeded();

		result.m_iTrackedCountAfter = m_Reconciler.GetTrackedStaticHandleCount();
		result.m_iStaticCountAfter = CampaignDebugCountAllStaticMarkers(markerManager);
		result.m_bRegistryStable = m_Registry.GetEpoch() == epochBefore
			&& m_Registry.GetWatermark() == watermarkBefore
			&& m_Registry.GetRecordCount() == registryCountBefore
			&& m_Registry.GetLiveRecordCount() == liveCountBefore
			&& m_Registry.GetRegistryHash() == registryHashBefore
			&& m_mDesired.Count() == desiredCountBefore
			&& result.m_iTrackedCountAfter == result.m_iTrackedCountBefore;
		result.m_bSingleInstance = result.m_bDeleteHealed
			&& m_Reconciler.CampaignDebugCountCanonicalStaticMatches(
				markerManager,
				canonicalIntegritySignature) == 1
			&& result.m_iStaticCountAfter == result.m_iStaticCountBefore;
		if (result.m_sFailureReason.IsEmpty())
		{
			if (!result.m_bPreconditionExact)
				result.m_sFailureReason = "selected campaign marker was not canonical before the probe";
			else if (!result.m_bMutationDetected)
				result.m_sFailureReason = "campaign marker mutation was not detected";
			else if (!result.m_bMutationHealed)
				result.m_sFailureReason = "campaign marker mutation was not repaired canonically";
			else if (!result.m_bDeleteDetected)
				result.m_sFailureReason = "campaign marker deletion was not detected";
			else if (!result.m_bDeleteHealed)
				result.m_sFailureReason = "campaign marker deletion was not repaired canonically";
			else if (!result.m_bRegistryStable)
				result.m_sFailureReason = "marker projection registry changed during native repair";
			else if (!result.m_bSingleInstance)
				result.m_sFailureReason = "campaign marker repair did not restore one exact instance";
			else if (!result.m_bPlayerMarkerInserted || !result.m_bPlayerMarkerEditable
				|| !result.m_bPlayerMarkerIsolated || !result.m_bPlayerMarkerCleaned)
				result.m_sFailureReason = "ordinary player marker isolation was not preserved";
		}
		return result;
	}

	protected void CampaignDebugProbePlayerMarkerIsolation(
		SCR_MapMarkerManagerComponent markerManager,
		SCR_MapMarkerBase canonicalCampaignMarker,
		HST_CampaignDebugMarkerIntegrityProbeResult result)
	{
		if (!markerManager || !canonicalCampaignMarker || !result)
			return;
		PlayerController localController = GetGame().GetPlayerController();
		if (!localController)
			return;
		int localPlayerId = localController.GetPlayerId();

		int campaignPosition[2];
		canonicalCampaignMarker.GetWorldPos(campaignPosition);
		SCR_MapMarkerBase playerMarker = new SCR_MapMarkerBase();
		playerMarker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		playerMarker.SetWorldPos(campaignPosition[0] + 41, campaignPosition[1] + 29);
		playerMarker.SetCustomText("HST marker integrity player probe");
		playerMarker.SetIconEntry(canonicalCampaignMarker.GetIconEntry());
		playerMarker.SetColorEntry(canonicalCampaignMarker.GetColorEntry());
		playerMarker.SetCanBeRemovedByOwner(true);
		markerManager.InsertStaticMarker(playerMarker, true, false);
		result.m_bPlayerMarkerInserted
			= CampaignDebugIsStaticMarkerLive(markerManager, playerMarker);
		result.m_bPlayerMarkerEditable = result.m_bPlayerMarkerInserted
			&& playerMarker.GetMarkerOwnerID() == localPlayerId
			&& playerMarker.CanBeRemovedByOwner();

		int changedX = campaignPosition[0] + 53;
		int changedZ = campaignPosition[1] + 47;
		string changedText = "HST marker integrity player probe changed";
		playerMarker.SetWorldPos(changedX, changedZ);
		playerMarker.SetCustomText(changedText);
		RepairNativeMarkersIfNeeded();
		int observedPosition[2];
		playerMarker.GetWorldPos(observedPosition);
		result.m_bPlayerMarkerIsolated = result.m_bPlayerMarkerEditable
			&& CampaignDebugIsStaticMarkerLive(markerManager, playerMarker)
			&& playerMarker.CanBeRemovedByOwner()
			&& observedPosition[0] == changedX
			&& observedPosition[1] == changedZ
			&& playerMarker.GetCustomText() == changedText;

		if (result.m_bPlayerMarkerInserted)
			markerManager.RemoveStaticMarker(playerMarker);
		if (CampaignDebugIsStaticMarkerLive(markerManager, playerMarker))
			markerManager.RemoveStaticMarker(playerMarker);
		result.m_bPlayerMarkerCleaned
			= !CampaignDebugIsStaticMarkerLive(markerManager, playerMarker);
	}

	protected int CampaignDebugCountAllStaticMarkers(
		SCR_MapMarkerManagerComponent markerManager)
	{
		if (!markerManager)
			return 0;
		return markerManager.GetStaticMarkers().Count()
			+ markerManager.GetDisabledMarkers().Count();
	}

	protected bool CampaignDebugIsStaticMarkerLive(
		SCR_MapMarkerManagerComponent markerManager,
		SCR_MapMarkerBase marker)
	{
		if (!markerManager || !marker)
			return false;
		foreach (SCR_MapMarkerBase activeMarker : markerManager.GetStaticMarkers())
		{
			if (activeMarker == marker)
				return true;
		}
		foreach (SCR_MapMarkerBase disabledMarker : markerManager.GetDisabledMarkers())
		{
			if (disabledMarker == marker)
				return true;
		}
		return false;
	}

	protected bool ReconcileAuthoredZoneDescriptors(
		SCR_MapMarkerManagerComponent markerManager,
		notnull map<string, ref HST_MapMarkerState> liveRecords)
	{
		m_iLastAuthoredZoneDescriptorsHidden = 0;
		m_iLastAuthoredZoneDescriptorsMissing = 0;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		array<string> suppressedZoneIds = {};
		foreach (string suppressedZoneId, bool suppressed : m_mSuppressedAuthoredZoneIds)
			suppressedZoneIds.Insert(suppressedZoneId);
		foreach (string suppressedZoneId : suppressedZoneIds)
		{
			string suppressedMarkerId = "hst_zone_" + suppressedZoneId;
			if (m_mDesired.Contains(suppressedMarkerId)
				&& m_Reconciler.IsDomainIdLive(markerManager, suppressedMarkerId))
				continue;
			RestoreAuthoredZoneDescriptor(suppressedZoneId);
		}

		foreach (string markerId, HST_MapMarkerState marker : liveRecords)
		{
			if (!marker || marker.m_bTombstone || markerId.IndexOf("hst_zone_") != 0 || marker.m_sLinkedId.IsEmpty())
				continue;
			if (!m_mDesired.Contains(markerId) || !m_Reconciler.IsDomainIdLive(markerManager, markerId))
				continue;

			MapItem mapItem = ResolveAuthoredZoneMapItem(marker.m_sLinkedId, world);
			if (!mapItem)
			{
				m_iLastAuthoredZoneDescriptorsMissing++;
				continue;
			}

			if (!m_mSuppressedAuthoredZoneIds.Contains(marker.m_sLinkedId))
				m_mAuthoredZonePriorVisibility.Set(marker.m_sLinkedId, mapItem.IsVisible());
			if (mapItem.IsVisible())
				mapItem.SetVisible(false);
			m_mSuppressedAuthoredZoneIds.Set(marker.m_sLinkedId, true);
		}
		m_iLastAuthoredZoneDescriptorsHidden = m_mSuppressedAuthoredZoneIds.Count();
		return m_iLastAuthoredZoneDescriptorsMissing == 0;
	}

	protected MapItem ResolveAuthoredZoneMapItem(string zoneId, BaseWorld world)
	{
		MapItem cached = m_mAuthoredZoneMapItemByZoneId.Get(zoneId);
		if (cached)
			return cached;
		if (!world || zoneId.IsEmpty() || m_mAuthoredZoneLookupAttempted.Contains(zoneId))
			return null;

		m_mAuthoredZoneLookupAttempted.Set(zoneId, true);
		string entityName = "HST_ConflictMapMarker_" + zoneId;
		IEntity markerEntity = world.FindEntityByName(entityName);
		if (!markerEntity || markerEntity.GetName() != entityName)
			return null;
		SCR_CampaignMilitaryBaseMapDescriptorComponent descriptor = SCR_CampaignMilitaryBaseMapDescriptorComponent.Cast(markerEntity.FindComponent(SCR_CampaignMilitaryBaseMapDescriptorComponent));
		if (!descriptor)
			return null;
		MapItem mapItem = descriptor.Item();
		if (mapItem)
			m_mAuthoredZoneMapItemByZoneId.Set(zoneId, mapItem);
		return mapItem;
	}

	protected void RestoreAuthoredZoneDescriptor(string zoneId)
	{
		MapItem mapItem = m_mAuthoredZoneMapItemByZoneId.Get(zoneId);
		if (mapItem && m_mAuthoredZonePriorVisibility.Contains(zoneId))
			mapItem.SetVisible(m_mAuthoredZonePriorVisibility.Get(zoneId));
		m_mSuppressedAuthoredZoneIds.Remove(zoneId);
		m_mAuthoredZonePriorVisibility.Remove(zoneId);
	}

	protected void RestoreAllAuthoredZoneDescriptors()
	{
		array<string> suppressedZoneIds = {};
		foreach (string zoneId, bool suppressed : m_mSuppressedAuthoredZoneIds)
			suppressedZoneIds.Insert(zoneId);
		foreach (string zoneId : suppressedZoneIds)
			RestoreAuthoredZoneDescriptor(zoneId);
		m_iLastAuthoredZoneDescriptorsHidden = 0;
	}

	void RetryMissingAuthoredZoneDescriptorBindings()
	{
		array<string> missingZoneIds = {};
		foreach (string zoneId, bool attempted : m_mAuthoredZoneLookupAttempted)
		{
			if (!m_mAuthoredZoneMapItemByZoneId.Contains(zoneId))
				missingZoneIds.Insert(zoneId);
		}
		foreach (string zoneId : missingZoneIds)
			m_mAuthoredZoneLookupAttempted.Remove(zoneId);
	}

	void Clear()
	{
		RestoreAllAuthoredZoneDescriptors();
		m_Reconciler.Clear();
		m_mDesired.Clear();
		m_Registry.Reset("client projection cleared");
	}

	void RequireSnapshot(string reason)
	{
		m_Registry.RequireSnapshot(reason);
	}

	HST_ClientMarkerProjectionRegistry GetRegistry()
	{
		return m_Registry;
	}

	string BuildReport()
	{
		string report = string.Format(
			"Partisan client marker projection | ready %1 | epoch %2 | watermark %3 | records %4 live %5 | hash %6 | status %7",
			m_Registry.IsReady(),
			m_Registry.GetEpoch(),
			m_Registry.GetWatermark(),
			m_Registry.GetRecordCount(),
			m_Registry.GetLiveRecordCount(),
			m_Registry.GetRegistryHash(),
			m_Registry.GetLastStatus());
		report = report + string.Format(
			" | authored zones hidden/missing %1/%2 | native %3",
			m_iLastAuthoredZoneDescriptorsHidden,
			m_iLastAuthoredZoneDescriptorsMissing,
			m_Reconciler.BuildRuntimeReport());
		return report;
	}
}
