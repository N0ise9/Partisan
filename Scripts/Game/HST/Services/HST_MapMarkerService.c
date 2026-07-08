class HST_MapMarkerService
{
	static const string NATIVE_MARKER_MANAGER_COMPONENT = "SCR_MapMarkerManagerComponent";
	static const string NATIVE_MARKER_CONFIG = "{3583D42139D9A10B}Configs/Map/CampaignMapMarkerConfig.conf";
	static const float MARKER_DECONFLICT_RADIUS_METERS = 14.0;
	static const float MARKER_DECONFLICT_STEP_METERS = 22.0;
	static const int MARKER_DECONFLICT_ATTEMPTS = 12;
	static const int MAX_NATIVE_MARKERS = 192;
	static const int MAX_NATIVE_TACTICAL_MARKERS = 48;
	static const int NATIVE_OWNERSHIP_SYNC_INTERVAL_SECONDS = 30;
	static const string PERSISTENCE_SMOKE_PREFIX = "hst_smoke";

	protected ref array<IEntity> m_aNativeMarkerCandidates = {};
	protected ref HST_CampaignMapMarkerDirector m_MarkerDirector = new HST_CampaignMapMarkerDirector();
	protected ref HST_NativeMapMarkerReconciler m_NativeReconciler = new HST_NativeMapMarkerReconciler();
	protected ref map<string, ref HST_MapMarkerRecord> m_mDesiredNativeMarkers = new map<string, ref HST_MapMarkerRecord>();
	protected string m_sNativeMarkerEntityName;
	protected bool m_bNativePublishPending;
	protected bool m_bNativeMapRefreshBound;
	protected float m_fNativePublishRetrySeconds;
	protected string m_sLastNativeMarkerSignature;
	protected int m_iLastNativeEligibleCount;
	protected int m_iLastNativePublishedCount;
	protected int m_iLastNativeSkippedCount;
	protected int m_iLastReportedMarkerRecordCount = -1;
	protected int m_iLastNativeOwnershipSyncSecond = -999999;
	protected int m_iNativeMapWidgetRefreshRetries;
	protected bool m_bDebugLoggingEnabled;
	protected bool m_bTrackResistanceSupportGroupsOnMap = true;

	void SetDebugLoggingEnabled(bool enabled)
	{
		m_bDebugLoggingEnabled = enabled;
		if (m_NativeReconciler)
			m_NativeReconciler.SetDebugLoggingEnabled(enabled);
	}

	void SetTrackResistanceSupportGroupsOnMap(bool enabled)
	{
		m_bTrackResistanceSupportGroupsOnMap = enabled;
	}

	bool IsTrackResistanceSupportGroupsOnMapEnabled()
	{
		return m_bTrackResistanceSupportGroupsOnMap;
	}

	int GetLastNativeEligibleCount()
	{
		return m_iLastNativeEligibleCount;
	}

	int GetLastNativePublishedCount()
	{
		return m_iLastNativePublishedCount;
	}

	int GetLastNativeSkippedCount()
	{
		return m_iLastNativeSkippedCount;
	}

	bool IsNativePublishPending()
	{
		return m_bNativePublishPending;
	}

	bool IsNativeMarkerManagerReady()
	{
		return ResolveNativeMarkerManager() != null;
	}

	int GetLastNativeReconcileFailedCount()
	{
		if (!m_NativeReconciler)
			return 0;

		HST_MapMarkerReconcileResult result = m_NativeReconciler.GetLastResult();
		if (!result)
			return 0;

		return result.m_iFailed;
	}

	int GetTrackedNativeStaticMissingCount()
	{
		if (!m_NativeReconciler)
			return 0;

		SCR_MapMarkerManagerComponent markerManager = ResolveNativeMarkerManager();
		if (!markerManager)
			return 0;

		return m_NativeReconciler.CountTrackedStaticMissing(markerManager);
	}

	bool RebuildAllMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return false;

		state.m_aMapMarkers.Clear();
		if (state.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_SETUP)
		{
			int previousReportedMarkerCount = m_iLastReportedMarkerRecordCount;
			int previousPublishedCount = m_iLastNativePublishedCount;
			int previousEligibleCount = m_iLastNativeEligibleCount;
			int previousSkippedCount = m_iLastNativeSkippedCount;
			bool setupPublished = PublishRuntimeNativeMarkers(state, preset);
			if (ShouldReportMarkerRebuild(state.m_aMapMarkers.Count(), setupPublished, false, previousReportedMarkerCount, previousPublishedCount, previousEligibleCount, previousSkippedCount))
			{
				DebugLog(string.Format("setup phase marker publisher empty, native %1/%2 published, %3 skipped", m_iLastNativePublishedCount, m_iLastNativeEligibleCount, m_iLastNativeSkippedCount));
				m_iLastReportedMarkerRecordCount = state.m_aMapMarkers.Count();
			}
			return setupPublished;
		}

		AddHQMarker(state, preset);
		AddDefendPetrosMarkers(state, preset);
		AddZoneMarkers(state, preset);
		AddMissionMarkers(state, preset);
		AddQRFMarkers(state, preset);
		AddSupportRequestMarkers(state, preset);
		AddResistanceSupportGroupMarkers(state, preset);
		int previousReportedMarkerCount = m_iLastReportedMarkerRecordCount;
		int previousPublishedCount = m_iLastNativePublishedCount;
		int previousEligibleCount = m_iLastNativeEligibleCount;
		int previousSkippedCount = m_iLastNativeSkippedCount;
		bool ownershipSynced = SyncVisibleNativeMarkerOwnershipIfDue(state);
		bool published = PublishRuntimeNativeMarkers(state, preset);
		if (ShouldReportMarkerRebuild(state.m_aMapMarkers.Count(), published, ownershipSynced, previousReportedMarkerCount, previousPublishedCount, previousEligibleCount, previousSkippedCount))
		{
			DebugLog(string.Format("rebuilt %1 campaign map marker record(s), native %2/%3 published, %4 skipped", state.m_aMapMarkers.Count(), m_iLastNativePublishedCount, m_iLastNativeEligibleCount, m_iLastNativeSkippedCount));
			m_iLastReportedMarkerRecordCount = state.m_aMapMarkers.Count();
		}
		return published || ownershipSynced;
	}

	void BindNativeMapRefresh()
	{
		m_bNativeMapRefreshBound = true;
	}

	void UnbindNativeMapRefresh()
	{
		m_bNativeMapRefreshBound = false;
		m_iNativeMapWidgetRefreshRetries = 0;
	}

	bool RefreshHQMarker(HST_CampaignState state, HST_CampaignPreset preset)
	{
		return RebuildAllMarkers(state, preset);
	}

	bool RefreshZoneMarker(HST_CampaignState state, HST_CampaignPreset preset, string zoneId)
	{
		return RebuildAllMarkers(state, preset);
	}

	bool RefreshMissionMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		return RebuildAllMarkers(state, preset);
	}

	void CleanupMarkers(HST_CampaignState state)
	{
		ClearRuntimeNativeMarkers();
		m_sLastNativeMarkerSignature = "";
		m_iLastNativeEligibleCount = 0;
		m_iLastNativePublishedCount = 0;
		m_iLastNativeSkippedCount = 0;
		m_iLastReportedMarkerRecordCount = -1;
		m_iLastNativeOwnershipSyncSecond = -999999;
		m_bNativePublishPending = false;
		m_iNativeMapWidgetRefreshRetries = 0;
		if (!state)
			return;

		state.m_aMapMarkers.Clear();
	}

	bool TickNativePublish(HST_CampaignState state, HST_CampaignPreset preset, float timeSlice)
	{
		if (!m_bNativePublishPending || !state || !preset)
			return false;

		m_fNativePublishRetrySeconds -= timeSlice;
		if (m_fNativePublishRetrySeconds > 0)
			return false;

		m_fNativePublishRetrySeconds = 1.0;
		return PublishRuntimeNativeMarkers(state, preset);
	}

	protected void OnNativeMapOpenComplete(MapConfiguration config)
	{
		// Native marker widgets are created by SCR_MapMarkersUI on map open.
	}

	protected void QueueNativeMapRefresh()
	{
		m_iNativeMapWidgetRefreshRetries = 0;
	}

	protected bool RetryNativeMapRefresh()
	{
		return false;
	}

	string BuildMarkerReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi map markers | campaign state not ready";

		int hqCount;
		int townCount;
		int strategicCount;
		int missionCount;
		int qrfCount;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (!marker || !marker.m_bVisible)
				continue;

			if (marker.m_sCategory == "hq")
				hqCount++;
			else if (marker.m_sCategory == "town")
				townCount++;
			else if (marker.m_sCategory == "mission" || marker.m_sCategory == "mission_objective" || marker.m_sCategory == "mission_asset")
				missionCount++;
			else if (marker.m_sCategory == "qrf" || marker.m_sCategory == "support")
				qrfCount++;
			else
				strategicCount++;
		}

		string summary = string.Format("h-istasi map markers | total %1 | HQ %2 | towns %3", state.m_aMapMarkers.Count(), hqCount, townCount);
		string pending = "ready";
		if (m_bNativePublishPending)
			pending = "pending";
		string tactical = string.Format(" | strategic %1 | missions %2 | QRFs/support %3 | native manager %4 | native %5/%6 visible | widgets %7 | skipped %8 | %9", strategicCount, missionCount, qrfCount, NATIVE_MARKER_MANAGER_COMPONENT, m_iLastNativePublishedCount, m_iLastNativeEligibleCount, BuildNativeWidgetReport(), m_iLastNativeSkippedCount, pending);
		return summary + tactical + BuildMarkerRefreshDiagnostic(state) + BuildMarkerDetailReport(state, 20);
	}

	string BuildMarkerAuditReport(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return "h-istasi phase 23 marker audit | failed: campaign state not ready";

		int activeMissions = CountActiveMissions(state);
		int activeSupport = CountActiveSupportRequests(state);
		int activeQRFs = CountActiveQRFs(state);
		int missionMarkers = CountMarkersByCategory(state, "mission") + CountMarkersByCategory(state, "mission_objective") + CountMarkersByCategory(state, "mission_asset");
		int supportMarkers = CountMarkersByCategory(state, "support");
		int qrfMarkers = CountMarkersByCategory(state, "qrf");
		bool hqMarker = HasMarker(state, "hst_hq");
		bool petrosMarker = HasMarker(state, "hst_petros");
		bool defendMarker = HasMarker(state, "hst_defend_petros");

		string status = "PASS";
		if (state.m_bHQDeployed && !hqMarker)
			status = "WARN";
		if (state.m_bDefendPetrosActive && (!petrosMarker || !defendMarker))
			status = "WARN";
		if (activeMissions > 0 && missionMarkers <= 0)
			status = "WARN";
		if (activeSupport > 0 && supportMarkers <= 0)
			status = "WARN";
		if (activeQRFs > 0 && qrfMarkers <= 0)
			status = "WARN";

		string report = string.Format("h-istasi phase 23 marker audit | %1 | total %2 | native %3/%4 skipped %5", status, state.m_aMapMarkers.Count(), m_iLastNativePublishedCount, m_iLastNativeEligibleCount, m_iLastNativeSkippedCount);
		report = report + string.Format("\ncoverage | HQ %1 | Petros %2 | defend %3 | missions %4/%5 | support %6/%7 | qrf %8/%9", hqMarker, petrosMarker, defendMarker, missionMarkers, activeMissions, supportMarkers, activeSupport, qrfMarkers, activeQRFs);
		report = report + string.Format("\ncategories | hq %1 | town %2 | strategic %3 | mission %4 | objective %5 | asset %6 | support %7 | qrf %8", CountMarkersByCategory(state, "hq"), CountMarkersByCategory(state, "town"), CountMarkersByCategory(state, "strategic"), CountMarkersByCategory(state, "mission"), CountMarkersByCategory(state, "mission_objective"), CountMarkersByCategory(state, "mission_asset"), supportMarkers, qrfMarkers);
		report = report + BuildMarkerRefreshDiagnostic(state);
		return report + BuildMarkerDetailReport(state, 30);
	}

	string BuildNativeMarkerRuntimeReport(HST_CampaignState state)
	{
		SCR_MapMarkerManagerComponent markerManager = ResolveNativeMarkerManager();
		if (!markerManager)
			return "h-istasi native marker report | native marker manager not ready";

		int desiredRecords = m_mDesiredNativeMarkers.Count();
		int nativeStatic = markerManager.GetStaticMarkers().Count();
		int nativeDisabled = markerManager.GetDisabledMarkers().Count();
		int setupPreviewOrphans = CountSetupPreviewNativeMarkers(markerManager);
		int campaignOrphanCandidates = Math.Max(0, CountDevelopmentHSTMarkers(markerManager) - setupPreviewOrphans);
		string pending = "ready";
		if (m_bNativePublishPending)
			pending = "pending";

		string report = "h-istasi native marker report";
		report = report + string.Format("\ndesired records: %1", desiredRecords);
		if (state)
			report = report + string.Format("\nstate records: %1", state.m_aMapMarkers.Count());
		report = report + string.Format("\nnative static array: %1", nativeStatic);
		report = report + string.Format("\nnative disabled array: %1", nativeDisabled);
		report = report + string.Format("\ntracked native handles: %1", m_iLastNativePublishedCount);
		report = report + string.Format("\neligible native records: %1", m_iLastNativeEligibleCount);
		report = report + string.Format("\nskipped/failed native records: %1", m_iLastNativeSkippedCount);
		report = report + string.Format("\nsetup preview orphan candidates: %1", setupPreviewOrphans);
		report = report + string.Format("\ncampaign orphan candidates: %1", campaignOrphanCandidates);
		report = report + string.Format("\nrefresh: %1", pending);
		if (m_NativeReconciler)
		{
			HST_MapMarkerReconcileResult result = m_NativeReconciler.GetLastResult();
			report = report + string.Format("\nreconciler static handles: %1", m_NativeReconciler.GetTrackedStaticHandleCount());
			report = report + string.Format("\nreconciler dynamic handles: %1", m_NativeReconciler.GetTrackedDynamicHandleCount());
			report = report + string.Format("\ntracked static active: %1", m_NativeReconciler.CountTrackedStaticActive(markerManager));
			report = report + string.Format("\ntracked static disabled: %1", m_NativeReconciler.CountTrackedStaticDisabled(markerManager));
			report = report + string.Format("\ntracked static missing: %1", m_NativeReconciler.CountTrackedStaticMissing(markerManager));
			if (result)
			{
				report = report + string.Format("\ncreated last reconcile: %1", result.m_iCreated);
				report = report + string.Format("\nupdated last reconcile: %1", result.m_iUpdated);
				report = report + string.Format("\nremoved last reconcile: %1", result.m_iRemoved);
				report = report + string.Format("\nfailed last reconcile: %1", result.m_iFailed);
			}
			report = report + "\n" + m_NativeReconciler.BuildDetailedRuntimeReport(markerManager);
		}
		return report;
	}

	string AdminPurgeNativeHSTMarkers()
	{
		SCR_MapMarkerManagerComponent markerManager = ResolveNativeMarkerManager();
		if (!markerManager)
			return "h-istasi admin | native marker purge failed: native marker manager not ready";

		int trackedRemoved;
		if (m_NativeReconciler)
		{
			m_NativeReconciler.Clear(markerManager);
			HST_MapMarkerReconcileResult result = m_NativeReconciler.GetLastResult();
			if (result)
				trackedRemoved = result.m_iRemoved;
		}

		array<SCR_MapMarkerBase> candidates = {};
		array<SCR_MapMarkerBase> staticMarkers = markerManager.GetStaticMarkers();
		foreach (SCR_MapMarkerBase marker : staticMarkers)
			candidates.Insert(marker);

		array<SCR_MapMarkerBase> disabledMarkers = markerManager.GetDisabledMarkers();
		foreach (SCR_MapMarkerBase disabledMarker : disabledMarkers)
			candidates.Insert(disabledMarker);

		int orphanRemoved;
		foreach (SCR_MapMarkerBase markerToRemove : candidates)
		{
			if (!IsDevelopmentHSTMarker(markerToRemove))
				continue;

			markerManager.RemoveStaticMarker(markerToRemove);
			orphanRemoved++;
		}

		m_mDesiredNativeMarkers.Clear();
		m_sLastNativeMarkerSignature = "";
		m_iLastNativePublishedCount = 0;
		m_iLastNativeEligibleCount = 0;
		m_iLastNativeSkippedCount = 0;
		m_bNativePublishPending = false;
		m_fNativePublishRetrySeconds = 0;

		return string.Format("h-istasi admin | purged %1 tracked and %2 orphan native HST marker(s)", trackedRemoved, orphanRemoved);
	}

	protected string BuildMarkerRefreshDiagnostic(HST_CampaignState state)
	{
		if (!state)
			return "\nrefresh | state not ready";

		string pending = "ready";
		if (m_bNativePublishPending)
			pending = "pending";

		return string.Format("\nrefresh | state second %1 | records %2 | native %3/%4 | widgets %5 | skipped %6 | ownership sync %7 | %8", state.m_iElapsedSeconds, state.m_aMapMarkers.Count(), m_iLastNativePublishedCount, m_iLastNativeEligibleCount, BuildNativeWidgetReport(), m_iLastNativeSkippedCount, m_iLastNativeOwnershipSyncSecond, pending);
	}

	protected string BuildMarkerDetailReport(HST_CampaignState state, int maxRows)
	{
		if (!state)
			return "";

		string report = "\nmarker detail";
		int emitted;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (!marker || !marker.m_bVisible)
				continue;

			report = report + string.Format("\n%1 | %2 | category %3 | owner %4 | style %5 | native %6 | pos %7", marker.m_sMarkerId, ShortMarkerText(marker.m_sLabel, 70), marker.m_sCategory, marker.m_sOwnerFactionKey, marker.m_sStyleHint, marker.m_bRuntimeNative, marker.m_vPosition);
			emitted++;
			if (emitted >= maxRows)
				break;
		}

		if (emitted == 0)
			report = report + "\nnone";
		else if (state.m_aMapMarkers.Count() > emitted)
			report = report + string.Format("\n... %1 more marker(s)", state.m_aMapMarkers.Count() - emitted);

		return report;
	}

	protected bool HasMarker(HST_CampaignState state, string markerId)
	{
		if (!state || markerId.IsEmpty())
			return false;

		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (marker && marker.m_sMarkerId == markerId && marker.m_bVisible)
				return true;
		}

		return false;
	}

	protected int CountSetupPreviewNativeMarkers(SCR_MapMarkerManagerComponent markerManager)
	{
		if (!markerManager)
			return 0;

		int count;
		array<SCR_MapMarkerBase> staticMarkers = markerManager.GetStaticMarkers();
		foreach (SCR_MapMarkerBase marker : staticMarkers)
		{
			if (IsSetupPreviewNativeMarker(marker))
				count++;
		}

		array<SCR_MapMarkerBase> disabledMarkers = markerManager.GetDisabledMarkers();
		foreach (SCR_MapMarkerBase disabledMarker : disabledMarkers)
		{
			if (IsSetupPreviewNativeMarker(disabledMarker))
				count++;
		}

		return count;
	}

	protected int CountDevelopmentHSTMarkers(SCR_MapMarkerManagerComponent markerManager)
	{
		if (!markerManager)
			return 0;

		int count;
		array<SCR_MapMarkerBase> staticMarkers = markerManager.GetStaticMarkers();
		foreach (SCR_MapMarkerBase marker : staticMarkers)
		{
			if (IsDevelopmentHSTMarker(marker))
				count++;
		}

		array<SCR_MapMarkerBase> disabledMarkers = markerManager.GetDisabledMarkers();
		foreach (SCR_MapMarkerBase disabledMarker : disabledMarkers)
		{
			if (IsDevelopmentHSTMarker(disabledMarker))
				count++;
		}

		return count;
	}

	protected bool IsSetupPreviewNativeMarker(SCR_MapMarkerBase marker)
	{
		if (!marker)
			return false;
		if (marker.GetType() != SCR_EMapMarkerType.PLACED_CUSTOM)
			return false;

		string text = marker.GetCustomText();
		return text == "HQ" || text == "HST_SETUP_HQ_CANDIDATE";
	}

	protected bool IsDevelopmentHSTMarker(SCR_MapMarkerBase marker)
	{
		if (!marker)
			return false;
		if (marker.GetType() != SCR_EMapMarkerType.PLACED_CUSTOM)
			return false;

		string text = marker.GetCustomText();
		if (text == "HQ")
			return true;
		if (text.Contains("FIA HQ"))
			return true;
		if (text.Contains("Petros"))
			return true;
		if (text.Contains("Defend"))
			return true;
		if (text.Contains("QRF"))
			return true;
		if (text.Contains("Mission"))
			return true;
		if (text.Contains("Convoy"))
			return true;

		return false;
	}

	protected int CountMarkersByCategory(HST_CampaignState state, string category)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (marker && marker.m_bVisible && marker.m_sCategory == category)
				count++;
		}

		return count;
	}

	protected int CountMarkersWithPrefix(HST_CampaignState state, string markerPrefix)
	{
		if (!state || markerPrefix.IsEmpty())
			return 0;

		int count;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (marker && marker.m_bVisible && marker.m_sMarkerId.IndexOf(markerPrefix) == 0)
				count++;
		}

		return count;
	}

	protected int CountActiveMissions(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE && !IsPersistenceSmokeMission(mission) && !AreMissionObjectivesComplete(state, mission))
				count++;
		}

		return count;
	}

	protected int CountActiveSupportRequests(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request)
				continue;
			if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_QUEUED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
				count++;
		}

		return count;
	}

	protected int CountActiveQRFs(HST_CampaignState state)
	{
		if (!state)
			return 0;

		int count;
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (ShouldShowQRFMarker(state, qrf))
				count++;
		}

		return count;
	}

	protected void AddHQMarker(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state.m_bHQDeployed)
			return;

		string label = string.Format("FIA HQ | Petros %1 | knowledge %2 | threat %3", state.m_bPetrosAlive, state.m_iHQKnowledge, state.m_iHQThreatLevel);
		AddMarker(state, "hst_hq", state.m_sHQHideoutId, label, "", "hq", preset.m_sResistanceFactionKey, "PICK_UP2", FactionToMarkerColor(preset.m_sResistanceFactionKey, preset), state.m_vHQPosition, true, "green", "support");
	}

	protected void AddDefendPetrosMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return;
		if (!state.m_bHQDeployed)
			return;

		vector petrosPosition = state.m_vPetrosPosition;
		if (IsZeroVector(petrosPosition))
			petrosPosition = state.m_vHQPosition;

		if (state.m_bDefendPetrosActive || !state.m_bPetrosAlive)
		{
			string petrosStatus = "alive";
			if (!state.m_bPetrosAlive)
				petrosStatus = "down";
			AddMarker(state, "hst_petros", "petros", string.Format("Petros | %1 | defense %2", petrosStatus, state.m_sDefendPetrosStatus), "", "hq", preset.m_sResistanceFactionKey, "POINT_OF_INTEREST", FactionToMarkerColor(preset.m_sResistanceFactionKey, preset), petrosPosition, true, "green", "support", true);
		}

		if (!state.m_bDefendPetrosActive)
			return;

		string defenseLabel = string.Format("Defend Petros | attackers %1/%2 | killed %3 | %4", state.m_iDefendPetrosAliveAttackerCount, state.m_iDefendPetrosAttackerCount, state.m_iDefendPetrosKilledCount, state.m_sDefendPetrosStatus);
		AddMarker(state, "hst_defend_petros", state.m_sDefendPetrosMissionId, defenseLabel, "", "mission_objective", preset.m_sResistanceFactionKey, "OBJECTIVE_MARKER", "REFORGER_ORANGE", petrosPosition, true, "gold", "mission_defend_petros", true);

		if (state.m_sDefendPetrosAttackerGroupId.IsEmpty())
			return;

		HST_ActiveGroupState group = state.FindActiveGroup(state.m_sDefendPetrosAttackerGroupId);
		if (!group)
			return;
		if (IsTerminalActiveGroupStatus(group.m_sRuntimeStatus))
			return;

		vector attackerPosition = group.m_vPosition;
		if (IsZeroVector(attackerPosition))
			attackerPosition = group.m_vTargetPosition;
		if (IsZeroVector(attackerPosition))
			attackerPosition = petrosPosition;

		string attackerLabel = string.Format("Petros attackers | %1 | alive %2 | group %3", group.m_sRuntimeStatus, group.m_iLastSeenAliveCount, group.m_sGroupId);
		AddMarker(state, "hst_defend_petros_attackers", state.m_sDefendPetrosAttackerGroupId, attackerLabel, "", "qrf", group.m_sFactionKey, "OBJECTIVE_MARKER", FactionToMarkerColor(group.m_sFactionKey, preset), attackerPosition, true, FactionToMarkerTextColor(group.m_sFactionKey, preset), "enemy_response", true);
	}

	protected bool IsTerminalActiveGroupStatus(string runtimeStatus)
	{
		return runtimeStatus == "eliminated" || runtimeStatus == "folded" || runtimeStatus == "spawn_failed" || runtimeStatus == "despawned" || runtimeStatus == "deleted";
	}

	protected void AddZoneMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			string category = ZoneToMarkerCategory(zone);
			string label = BuildZoneMarkerLabel(zone);
			string color = FactionToMarkerColor(zone.m_sOwnerFactionKey, preset);
			string icon = ZoneToMarkerIcon(zone);
			string textColor = ZoneToMarkerTextColor(zone);
			string style = ZoneToMarkerStyle(zone);
			AddMarker(state, "hst_zone_" + zone.m_sZoneId, zone.m_sZoneId, label, "", category, zone.m_sOwnerFactionKey, icon, color, zone.m_vPosition, true, textColor, style);
		}
	}

	protected void AddMissionMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;
			if (AreMissionObjectivesComplete(state, mission))
				continue;

			bool exactMissionPosition;
			vector markerPosition = ResolveMissionMarkerPosition(state, mission, exactMissionPosition);
			string markerId = mission.m_sMarkerId;
			if (markerId.IsEmpty())
				markerId = "hst_mission_" + mission.m_sInstanceId;

			bool hasSpecificMarker = HasVisibleMissionAssetMarker(state, mission) || HasVisibleMissionObjectiveMarker(state, mission);
			if (!hasSpecificMarker)
				AddMarker(state, markerId, mission.m_sInstanceId, BuildMissionMarkerLabel(state, mission), "", "mission", preset.m_sResistanceFactionKey, MissionToMarkerIcon(mission), MissionToMarkerColor(mission), markerPosition, true, MissionToMarkerTextColor(mission), MissionToMarkerStyle(mission), !exactMissionPosition);
			AddMissionRouteMarkers(state, preset, mission);
			AddMissionObjectiveMarkers(state, preset, mission);
			AddMissionAssetMarkers(state, preset, mission);
		}
	}

	protected bool IsPersistenceSmokeMission(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		return mission.m_sInstanceId.Contains(PERSISTENCE_SMOKE_PREFIX) || mission.m_sMissionId.Contains(PERSISTENCE_SMOKE_PREFIX);
	}

	protected void AddMissionRouteMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		return;
	}

	protected void AddMissionObjectiveMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		bool hasPhysicalAssetMarker = HasVisibleMissionAssetMarker(state, mission);
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
				continue;
			if (mission.m_sRuntimePrimitive == "convoy_intercept" && objective.m_sTargetId == "convoy")
				continue;
			if (hasPhysicalAssetMarker && objective.m_sTargetId != "area" && objective.m_sRuntimePrimitive != "hold_area" && objective.m_sRuntimePrimitive != "clear_area")
				continue;

			HST_MissionAssetState linkedAsset = state.FindMissionAsset(objective.m_sLinkedRuntimeEntityId);
			if (linkedAsset && linkedAsset.m_sKind != "area")
				continue;

			string markerId = string.Format("hst_mission_obj_%1_%2", mission.m_sInstanceId, objective.m_sObjectiveId);
			vector objectivePosition = ResolveMissionObjectiveMarkerPosition(objective, linkedAsset);
			bool deconflictObjective = IsBroadZoneFallbackPosition(state, objective.m_sTargetZoneId, objectivePosition);
			AddMarker(state, markerId, mission.m_sInstanceId, BuildMissionObjectiveMarkerLabel(mission, objective), "", "mission_objective", preset.m_sResistanceFactionKey, MissionObjectiveToMarkerIcon(mission, objective), MissionToMarkerColor(mission), objectivePosition, true, MissionToMarkerTextColor(mission), MissionToMarkerStyle(mission) + "_objective", deconflictObjective);
		}
	}

	protected void AddMissionAssetMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (AreMissionObjectivesComplete(state, mission))
			return;

		if (mission.m_sRuntimePrimitive == "convoy_intercept")
		{
			AddMissionConvoyMarkers(state, preset, mission);
			return;
		}

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bDestroyed || asset.m_bDelivered)
				continue;
			if (asset.m_sKind == "area")
				continue;
			if (asset.m_sRole == "convoy_vehicle" && HasMissionConvoyPayloadSatisfied(state, mission))
				continue;

			vector position = asset.m_vCurrentPosition;
			string icon = MissionAssetToMarkerIcon(mission, asset);
			string style = MissionAssetToMarkerStyle(mission, asset);
			string label = BuildMissionAssetMarkerLabel(mission, asset);
			string color = MissionAssetToMarkerColor(mission, asset);

			if (!asset.m_bPickedUp && (asset.m_sRole == "city_supplies" || asset.m_sRole == "logistics_cargo" || asset.m_sRole == "convoy_payload"))
			{
				position = asset.m_vSourcePosition;
			}
			else if (asset.m_bPickedUp && !asset.m_bDelivered)
			{
				if (!IsZeroVector(asset.m_vTargetPosition))
					position = asset.m_vTargetPosition;
				else
					position = ResolveMissionAssetMarkerPosition(asset);
			}

			AddMarker(state, "hst_mission_asset_" + asset.m_sAssetId, mission.m_sInstanceId, label, "", "mission_asset", preset.m_sResistanceFactionKey, icon, color, position, true, MissionToMarkerTextColor(mission), style, true);
		}
	}

	protected void AddMissionConvoyMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !preset || !mission)
			return;

		HST_MissionAssetState outcomeAsset;
		if (ShouldShowConvoyOutcomeMarkers(mission))
			outcomeAsset = SelectPendingConvoyOutcomeAsset(state, mission);
		if (outcomeAsset && (outcomeAsset.m_sRole == "convoy_payload" || outcomeAsset.m_sRole == "convoy_captive"))
		{
			vector outcomePosition = ResolveConvoyOutcomeMarkerPosition(state, mission, outcomeAsset);
			AddMarker(state, "hst_mission_convoy_outcome_" + outcomeAsset.m_sAssetId, mission.m_sInstanceId, BuildConvoyOutcomeMarkerLabel(mission, outcomeAsset), "", "mission_asset", preset.m_sResistanceFactionKey, MissionAssetToMarkerIcon(mission, outcomeAsset), MissionToMarkerColor(mission), outcomePosition, true, MissionToMarkerTextColor(mission), MissionAssetToMarkerStyle(mission, outcomeAsset), true);
			return;
		}
		if (outcomeAsset && outcomeAsset.m_sRole == "convoy_vehicle" && mission.m_sRuntimePhase == "convoy_eliminated")
		{
			AddMissionConvoyVehicleMarkers(state, preset, mission, MissionMarkerTitle(mission));
			return;
		}

		vector convoyPosition = ResolveMissionConvoyAggregatePosition(state, mission);
		vector destinationPosition = ResolveMissionConvoyDestinationPosition(state, mission);
		string title = MissionMarkerTitle(mission);
		string destinationName = ResolveZoneDisplayNameById(state, mission.m_sTargetZoneId);
		bool hasConvoyVehicleAssets = HasUnresolvedMissionConvoyVehicleAsset(state, mission);
		if (hasConvoyVehicleAssets)
		{
			if (ShouldShowIndividualConvoyVehicleMarkers(mission))
				AddMissionConvoyVehicleMarkers(state, preset, mission, title);
			else
				AddMarker(state, "hst_mission_convoy_current_" + mission.m_sInstanceId, mission.m_sInstanceId, string.Format("Convoy - %1 | %2: neutralize crew", title, MissionRuntimePhaseLabel(mission)), "", "mission_asset", preset.m_sResistanceFactionKey, "POINT_SPECIAL", MissionToMarkerColor(mission), convoyPosition, true, MissionToMarkerTextColor(mission), "mission_convoy_vehicle", true);
		}
		AddMarker(state, "hst_mission_convoy_dest_" + mission.m_sInstanceId, mission.m_sInstanceId, string.Format("Convoy destination - %1 | %2: %3", title, MissionRuntimePhaseLabel(mission), destinationName), "", "mission_objective", preset.m_sResistanceFactionKey, "OBJECTIVE_MARKER", MissionToMarkerColor(mission), destinationPosition, true, MissionToMarkerTextColor(mission), "mission_convoy_destination", true);
	}

	protected bool ShouldShowIndividualConvoyVehicleMarkers(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		return mission.m_sRuntimePhase == "convoy_moving" || mission.m_sRuntimePhase == "convoy_contact" || mission.m_sRuntimePhase == "convoy_eliminated";
	}

	protected bool HasUnresolvedMissionConvoyVehicleAsset(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == "convoy_vehicle" && !asset.m_bDestroyed && !asset.m_bDelivered)
				return true;
		}

		return false;
	}

	protected void AddMissionConvoyVehicleMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, string title)
	{
		int vehicleIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != "convoy_vehicle" || asset.m_bDestroyed || asset.m_bDelivered)
				continue;

			vector position = asset.m_vCurrentPosition;
			if (IsZeroVector(position))
				position = asset.m_vSourcePosition;

			string markerId = "hst_mission_convoy_vehicle_" + asset.m_sAssetId;
			string label;
			if (mission.m_sRuntimePhase == "convoy_eliminated")
				label = string.Format("Convoy vehicle %1 - %2 | %3: capture vehicle", vehicleIndex + 1, title, MissionRuntimePhaseLabel(mission));
			else
				label = string.Format("Convoy vehicle %1 - %2: neutralize crew | %3", vehicleIndex + 1, title, MissionRuntimePhaseLabel(mission));
			AddMarker(state, markerId, mission.m_sInstanceId, label, "", "mission_asset", preset.m_sResistanceFactionKey, "POINT_SPECIAL", MissionToMarkerColor(mission), position, true, MissionToMarkerTextColor(mission), "mission_convoy_vehicle", true);
			vehicleIndex++;
		}
	}

	protected vector ResolveMissionConvoyAggregatePosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		vector aggregate;
		int count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != "convoy_vehicle" || asset.m_bDestroyed || asset.m_bDelivered)
				continue;

			vector position = asset.m_vCurrentPosition;
			if (IsZeroVector(position))
				position = asset.m_vSourcePosition;
			aggregate[0] = aggregate[0] + position[0];
			aggregate[1] = aggregate[1] + position[1];
			aggregate[2] = aggregate[2] + position[2];
			count++;
		}

		if (count > 0)
		{
			aggregate[0] = aggregate[0] / count;
			aggregate[1] = aggregate[1] / count;
			aggregate[2] = aggregate[2] / count;
			return aggregate;
		}

		bool exactMissionPosition;
		return ResolveMissionMarkerPosition(state, mission, exactMissionPosition);
	}

	protected vector ResolveMissionConvoyDestinationPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == "convoy_vehicle" && !IsZeroVector(asset.m_vTargetPosition))
				return asset.m_vTargetPosition;
		}

		if (!IsZeroVector(mission.m_vTargetPosition))
			return mission.m_vTargetPosition;

		return ResolveMissionConvoyAggregatePosition(state, mission);
	}

	protected void AddQRFMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (!ShouldShowQRFMarker(state, qrf))
				continue;

			HST_ZoneState targetZone = state.FindZone(qrf.m_sTargetZoneId);
			if (!targetZone)
				continue;

			vector position = ResolveQRFMarkerPosition(state, qrf, targetZone);
			string label = BuildQRFMarkerLabel(state, qrf, targetZone);
			bool runtimeNative = ShouldPublishNativeTacticalMarkerInPlayerBubble(state, targetZone.m_sZoneId, position);
			AddMarker(state, "hst_qrf_" + qrf.m_sInstanceId, qrf.m_sInstanceId, label, "", "qrf", qrf.m_sFactionKey, "OBJECTIVE_MARKER", FactionToMarkerColor(qrf.m_sFactionKey, preset), position, true, FactionToMarkerTextColor(qrf.m_sFactionKey, preset), "enemy_response", true, runtimeNative);
		}
	}

	protected bool ShouldShowQRFMarker(HST_CampaignState state, HST_QRFState qrf)
	{
		if (!state || !qrf)
			return false;

		if (!qrf.m_bResolved)
			return true;

		if (qrf.m_sGroupId.IsEmpty())
			return false;

		HST_ActiveGroupState group = state.FindActiveGroup(qrf.m_sGroupId);
		if (!group)
			return false;

		return group.m_sRuntimeStatus != "eliminated" && group.m_sRuntimeStatus != "folded" && group.m_sRuntimeStatus != "spawn_failed";
	}

	protected void AddSupportRequestMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
				continue;

			vector position = ResolveSupportMarkerPosition(state, request);
			string color = FactionToMarkerColor(request.m_sFactionKey, preset);
			string label = BuildSupportMarkerLabel(state, request);
			bool runtimeNative = ShouldPublishNativeSupportMarker(request) && ShouldPublishNativeTacticalMarkerInPlayerBubble(state, request.m_sTargetZoneId, position);
			AddMarker(state, "hst_support_" + request.m_sRequestId, request.m_sRequestId, label, "", "support", request.m_sFactionKey, "POINT_OF_INTEREST", color, position, true, FactionToMarkerTextColor(request.m_sFactionKey, preset), "support_incoming", true, runtimeNative);
		}
	}

	protected void AddResistanceSupportGroupMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!m_bTrackResistanceSupportGroupsOnMap || !state || !preset)
			return;

		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			if (!ShouldShowResistanceSupportGroupMarker(state, preset, group))
				continue;

			HST_SupportRequestState request = state.FindSupportRequest(group.m_sSupportRequestId);
			vector position = ResolveResistanceSupportGroupMarkerPosition(request, group);
			string color = FactionToMarkerColor(group.m_sFactionKey, preset);
			string label = BuildResistanceSupportGroupMarkerLabel(state, request, group);
			AddMarker(state, "hst_support_group_" + group.m_sGroupId, group.m_sGroupId, label, "", "support", group.m_sFactionKey, "POINT_SPECIAL", color, position, true, FactionToMarkerTextColor(group.m_sFactionKey, preset), "support_group_live", true, true);
		}
	}

	bool ShouldShowResistanceSupportGroupMarker(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveGroupState group)
	{
		if (!m_bTrackResistanceSupportGroupsOnMap || !state || !preset || !group)
			return false;

		if (group.m_sGroupId.IsEmpty() || group.m_sSupportRequestId.IsEmpty())
			return false;

		if (group.m_sFactionKey != preset.m_sResistanceFactionKey)
			return false;

		if (!group.m_bSpawnedEntity && group.m_sRuntimeEntityId.IsEmpty())
			return false;

		if (IsTerminalActiveGroupStatus(group.m_sRuntimeStatus))
			return false;

		HST_SupportRequestState request = state.FindSupportRequest(group.m_sSupportRequestId);
		if (!request || !request.m_bPlayerRequested || request.m_sFactionKey != preset.m_sResistanceFactionKey || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
			return false;

		int knownAlive = Math.Max(0, group.m_iSurvivorInfantryCount) + Math.Max(0, group.m_iSurvivorVehicleCount);
		knownAlive = Math.Max(knownAlive, Math.Max(0, group.m_iLastSeenAliveCount));
		knownAlive = Math.Max(knownAlive, Math.Max(0, group.m_iSpawnedAgentCount));
		if (knownAlive <= 0 && group.m_iInfantryCount <= 0 && group.m_iVehicleCount <= 0)
			return false;

		return true;
	}

	protected vector ResolveResistanceSupportGroupMarkerPosition(HST_SupportRequestState request, HST_ActiveGroupState group)
	{
		if (!group)
			return "0 0 0";

		if (!IsZeroVector(group.m_vPosition))
			return group.m_vPosition;
		if (!IsZeroVector(group.m_vTargetPosition))
			return group.m_vTargetPosition;
		if (request && !IsZeroVector(request.m_vTargetPosition))
			return request.m_vTargetPosition;
		if (!IsZeroVector(group.m_vSourcePosition))
			return group.m_vSourcePosition;

		return "0 0 0";
	}

	protected string BuildResistanceSupportGroupMarkerLabel(HST_CampaignState state, HST_SupportRequestState request, HST_ActiveGroupState group)
	{
		if (!group)
			return "Resistance support";

		string typeLabel = "Support";
		string targetName = "field";
		if (request)
		{
			typeLabel = SupportRequestTypeLabel(request.m_eType);
			targetName = ResolveZoneDisplayNameById(state, request.m_sTargetZoneId);
			if (IsPetrosAttackSupportMarker(request))
				targetName = "HQ/Petros";
		}

		int knownAlive = Math.Max(0, group.m_iSurvivorInfantryCount) + Math.Max(0, group.m_iSurvivorVehicleCount);
		knownAlive = Math.Max(knownAlive, Math.Max(0, group.m_iLastSeenAliveCount));
		knownAlive = Math.Max(knownAlive, Math.Max(0, group.m_iSpawnedAgentCount));
		if (knownAlive > 0)
			return string.Format("Resistance %1 | live | %2 alive | near %3", typeLabel, knownAlive, targetName);

		return string.Format("Resistance %1 | live | near %2", typeLabel, targetName);
	}

	protected void AddMarker(HST_CampaignState state, string markerId, string linkedId, string label, string callsign, string category, string ownerFactionKey, string iconHint, string colorHint, vector position, bool visible, string textColorHint, string styleHint, bool deconflict = false, bool runtimeNative = true)
	{
		HST_MapMarkerState marker = new HST_MapMarkerState();
		marker.m_sMarkerId = markerId;
		marker.m_sLinkedId = linkedId;
		marker.m_sLabel = label;
		marker.m_sCallsign = callsign;
		marker.m_sCategory = category;
		marker.m_sOwnerFactionKey = ownerFactionKey;
		marker.m_sIconHint = iconHint;
		marker.m_sColorHint = colorHint;
		marker.m_sTextColorHint = textColorHint;
		marker.m_sStyleHint = styleHint;
		marker.m_vPosition = ResolveMarkerPresentationPosition(state, markerId, position, deconflict);
		marker.m_bVisible = visible;
		marker.m_bRuntimeNative = runtimeNative;
		state.m_aMapMarkers.Insert(marker);
	}

	protected bool ShouldPublishNativeSupportMarker(HST_SupportRequestState request)
	{
		if (!request)
			return false;

		return request.m_eType == HST_ESupportRequestType.HST_SUPPORT_QRF || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
	}

	protected bool ShouldPublishNativeTacticalMarkerInPlayerBubble(HST_CampaignState state, string zoneId, vector position)
	{
		if (!state)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (zone && zone.m_bActive)
			return true;

		if (IsZeroVector(position) && zone)
			position = zone.m_vPosition;

		return HST_WorldPositionService.IsPositionInsidePlayerEventBubble(position);
	}

	protected bool PublishRuntimeNativeMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return false;

		SCR_MapMarkerManagerComponent markerManager = ResolveNativeMarkerManager();
		if (!markerManager)
		{
			m_bNativePublishPending = true;
			m_fNativePublishRetrySeconds = 0.25;
			DebugLog("native map marker manager not ready; marker publish pending");
			return false;
		}

		m_MarkerDirector.BuildDesiredMarkers(state, preset, m_mDesiredNativeMarkers);
		bool changed = m_NativeReconciler.Reconcile(markerManager, m_mDesiredNativeMarkers);
		HST_MapMarkerReconcileResult result = m_NativeReconciler.GetLastResult();
		m_iLastNativeEligibleCount = m_MarkerDirector.GetLastEligibleCount();
		m_iLastNativePublishedCount = result.m_iPublishedStatic + result.m_iPublishedDynamic;
		m_iLastNativeSkippedCount = m_MarkerDirector.GetLastSkippedCount() + result.m_iFailed;
		m_bNativePublishPending = false;
		m_fNativePublishRetrySeconds = 0;
		return changed;
	}

	protected bool ShouldReportMarkerRebuild(int markerCount, bool published, bool ownershipSynced, int previousMarkerCount, int previousPublishedCount, int previousEligibleCount, int previousSkippedCount)
	{
		if (!published && !ownershipSynced)
			return false;
		if (markerCount != previousMarkerCount)
			return true;
		if (m_iLastNativePublishedCount != previousPublishedCount)
			return true;
		if (m_iLastNativeEligibleCount != previousEligibleCount)
			return true;
		if (m_iLastNativeSkippedCount != previousSkippedCount)
			return true;

		return false;
	}

	protected bool CreateRuntimeNativeMarker(SCR_MapMarkerManagerComponent markerManager, HST_MapMarkerState marker, HST_CampaignPreset preset)
	{
		return false;
	}

	protected void ClearRuntimeNativeMarkers()
	{
		SCR_MapMarkerManagerComponent markerManager = ResolveNativeMarkerManager();
		if (m_NativeReconciler)
			m_NativeReconciler.Clear(markerManager);
		m_mDesiredNativeMarkers.Clear();
	}

	protected string BuildNativeMarkerSignature(HST_CampaignState state)
	{
		if (!state)
			return "";

		string signature = "";
		int tacticalCount;
		int publishedCount;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (!CanPublishNativeMarker(marker, tacticalCount, publishedCount))
				continue;

			signature = signature + "|" + BuildNativeMarkerSignatureEntry(marker);
			publishedCount++;
			if (IsTacticalNativeMarker(marker))
				tacticalCount++;
		}

		return signature;
	}

	protected string BuildNativeMarkerSignatureEntry(HST_MapMarkerState marker)
	{
		if (!marker)
			return "";

		int x = Math.Round(marker.m_vPosition[0]);
		int z = Math.Round(marker.m_vPosition[2]);
		if (IsTacticalNativeMarker(marker))
		{
			x = Math.Round(marker.m_vPosition[0] / 25.0) * 25;
			z = Math.Round(marker.m_vPosition[2] / 25.0) * 25;
		}
		return string.Format("%1:%2:%3:%4:%5:%6:%7:%8", marker.m_sMarkerId, ResolveNativeMarkerText(marker), marker.m_sCategory, marker.m_sOwnerFactionKey, marker.m_sIconHint, marker.m_sColorHint, x, z);
	}

	protected string ResolveNativeMarkerText(HST_MapMarkerState marker)
	{
		if (!marker)
			return "";

		if (marker.m_sCategory != "mission")
			return ShortMarkerText(marker.m_sLabel, 42);

		int separator = marker.m_sLabel.IndexOf(" |");
		if (separator <= 0)
			return ShortMarkerText(marker.m_sLabel, 42);

		return ShortMarkerText(marker.m_sLabel.Substring(0, separator), 42);
	}

	protected bool HasRuntimeNativeMarkers(SCR_MapMarkerManagerComponent markerManager)
	{
		return markerManager && m_iLastNativePublishedCount > 0;
	}

	protected bool HasRuntimeNativeMarkerWidgets()
	{
		return true;
	}

	protected bool IsNativeMapMarkerUIReady(SCR_MapEntity mapEntity)
	{
		if (!mapEntity || !mapEntity.IsOpen())
			return false;

		if (!mapEntity.GetMapUIComponent(SCR_MapMarkersUI))
			return false;

		Widget root = mapEntity.GetMapMenuRoot();
		if (!root)
			return false;

		return root.FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME) != null;
	}

	protected int CountRuntimeNativeMarkerWidgets()
	{
		return m_iLastNativePublishedCount;
	}

	protected int RecreateRuntimeNativeMarkerWidgets()
	{
		return 0;
	}

	protected string BuildNativeWidgetReport()
	{
		if (!m_NativeReconciler)
			return "native-managed";

		return m_NativeReconciler.BuildRuntimeReport();
	}

	protected bool IsNativeMarkerCandidate(HST_MapMarkerState marker)
	{
		return marker && marker.m_bVisible && marker.m_bRuntimeNative;
	}

	protected bool CanPublishNativeMarker(HST_MapMarkerState marker, int tacticalCount, int publishedCount)
	{
		if (!IsNativeMarkerCandidate(marker))
			return false;

		if (marker.m_sCategory == "mission_route")
			return false;

		if (publishedCount >= MAX_NATIVE_MARKERS)
			return false;

		if (IsTacticalNativeMarker(marker) && tacticalCount >= MAX_NATIVE_TACTICAL_MARKERS)
			return false;

		return true;
	}

	protected bool IsTacticalNativeMarker(HST_MapMarkerState marker)
	{
		if (!marker)
			return false;

		return marker.m_sCategory == "mission" || marker.m_sCategory == "mission_objective" || marker.m_sCategory == "mission_asset" || marker.m_sCategory == "qrf" || marker.m_sCategory == "support";
	}

	protected SCR_MapMarkerManagerComponent ResolveNativeMarkerManager()
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager)
			return markerManager;

		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;

		return SCR_MapMarkerManagerComponent.Cast(gameMode.FindComponent(SCR_MapMarkerManagerComponent));
	}

	protected SCR_EScenarioFrameworkMarkerCustom ResolveNativeIcon(string iconHint, string category, string styleHint)
	{
		if (iconHint == "PICK_UP2")
			return SCR_EScenarioFrameworkMarkerCustom.PICK_UP2;

		if (iconHint == "MINE_SINGLE")
			return SCR_EScenarioFrameworkMarkerCustom.MINE_SINGLE;

		if (iconHint == "FORTIFICATION")
			return SCR_EScenarioFrameworkMarkerCustom.FORTIFICATION;

		if (iconHint == "FORTIFICATION2")
			return SCR_EScenarioFrameworkMarkerCustom.FORTIFICATION2;

		if (iconHint == "POINT_SPECIAL")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_SPECIAL;

		if (iconHint == "POINT_OF_INTEREST")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_OF_INTEREST;

		if (iconHint == "POINT_OF_INTEREST2")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_OF_INTEREST2;

		if (iconHint == "OBSERVATION_POST")
			return SCR_EScenarioFrameworkMarkerCustom.OBSERVATION_POST;

		if (iconHint == "OBSERVATION_POST2")
			return SCR_EScenarioFrameworkMarkerCustom.OBSERVATION_POST2;

		if (iconHint == "RECON_OUTPOST")
			return SCR_EScenarioFrameworkMarkerCustom.RECON_OUTPOST;

		if (iconHint == "RECON_OUTPOST2")
			return SCR_EScenarioFrameworkMarkerCustom.RECON_OUTPOST2;

		if (iconHint == "OBJECTIVE_MARKER" && (styleHint == "town" || category == "town"))
			return SCR_EScenarioFrameworkMarkerCustom.POINT_OF_INTEREST2;

		if (iconHint == "OBJECTIVE_MARKER" && (styleHint == "enemy_base" || styleHint == "stronghold" || category == "enemy_base"))
			return SCR_EScenarioFrameworkMarkerCustom.FORTIFICATION;

		if (iconHint == "OBJECTIVE_MARKER" && (styleHint == "mission_site" || category == "mission_site"))
			return SCR_EScenarioFrameworkMarkerCustom.POINT_SPECIAL;

		if (iconHint == "OBJECTIVE_MARKER")
			return SCR_EScenarioFrameworkMarkerCustom.OBJECTIVE_MARKER;

		if (styleHint == "resource" || styleHint == "depot")
			return SCR_EScenarioFrameworkMarkerCustom.MINE_SINGLE;

		if (styleHint == "town" || category == "town")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_OF_INTEREST2;

		if (styleHint == "radio" || category == "radio")
			return SCR_EScenarioFrameworkMarkerCustom.RECON_OUTPOST;

		if (styleHint == "enemy_base" || styleHint == "stronghold" || category == "enemy_base")
			return SCR_EScenarioFrameworkMarkerCustom.FORTIFICATION;

		if (styleHint == "mission_site" || category == "mission_site")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_SPECIAL;

		if (styleHint == "support" || category == "hq" || category == "hideout")
			return SCR_EScenarioFrameworkMarkerCustom.PICK_UP2;

		return SCR_EScenarioFrameworkMarkerCustom.OBJECTIVE_MARKER;
	}

	protected SCR_EScenarioFrameworkMarkerCustomColor ResolveNativeColor(HST_MapMarkerState marker, HST_CampaignPreset preset)
	{
		if (!marker)
			return SCR_EScenarioFrameworkMarkerCustomColor.WHITE;

		string colorHint = marker.m_sColorHint;
		if (colorHint == "GREEN")
			return SCR_EScenarioFrameworkMarkerCustomColor.GREEN;

		if (colorHint == "BLUFOR")
			return SCR_EScenarioFrameworkMarkerCustomColor.BLUFOR;

		if (colorHint == "BLUE")
			return SCR_EScenarioFrameworkMarkerCustomColor.BLUE;

		if (colorHint == "RED")
			return SCR_EScenarioFrameworkMarkerCustomColor.RED;

		if (colorHint == "OPFOR")
			return SCR_EScenarioFrameworkMarkerCustomColor.OPFOR;

		if (colorHint == "MAGENTA" || marker.m_sTextColorHint == "magenta")
			return SCR_EScenarioFrameworkMarkerCustomColor.MAGENTA;

		if (colorHint == "CIVILIAN")
			return SCR_EScenarioFrameworkMarkerCustomColor.CIVILIAN;

		if (colorHint == "REFORGER_ORANGE" || marker.m_sTextColorHint == "gold")
			return SCR_EScenarioFrameworkMarkerCustomColor.REFORGER_ORANGE;

		if (preset && marker.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			return SCR_EScenarioFrameworkMarkerCustomColor.GREEN;

		if (preset && marker.m_sOwnerFactionKey == preset.m_sOccupierFactionKey)
			return SCR_EScenarioFrameworkMarkerCustomColor.BLUFOR;

		if (preset && marker.m_sOwnerFactionKey == preset.m_sInvaderFactionKey)
			return SCR_EScenarioFrameworkMarkerCustomColor.RED;

		return SCR_EScenarioFrameworkMarkerCustomColor.WHITE;
	}

	protected bool SyncVisibleNativeMarkerOwnershipIfDue(HST_CampaignState state)
	{
		if (!state)
			return false;

		if (state.m_iElapsedSeconds - m_iLastNativeOwnershipSyncSecond < NATIVE_OWNERSHIP_SYNC_INTERVAL_SECONDS)
			return false;

		m_iLastNativeOwnershipSyncSecond = state.m_iElapsedSeconds;
		SyncVisibleNativeMarkerOwnership(state);
		return true;
	}

	protected void SyncVisibleNativeMarkerOwnership(HST_CampaignState state)
	{
		if (!state)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey.IsEmpty())
				continue;

			m_sNativeMarkerEntityName = "HST_ConflictMapMarker_" + zone.m_sZoneId;
			m_aNativeMarkerCandidates.Clear();
			world.QueryEntitiesBySphere(zone.m_vPosition, 4, AddNativeMarkerCandidate, null, EQueryEntitiesFlags.ALL);
			foreach (IEntity markerEntity : m_aNativeMarkerCandidates)
			{
				FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(markerEntity.FindComponent(FactionAffiliationComponent));
				if (factionComponent)
					factionComponent.SetAffiliatedFactionByKey(zone.m_sOwnerFactionKey);
			}
		}

		m_sNativeMarkerEntityName = "";
		m_aNativeMarkerCandidates.Clear();
	}

	protected bool AddNativeMarkerCandidate(IEntity entity)
	{
		if (!entity || m_sNativeMarkerEntityName.IsEmpty())
			return true;

		if (entity.GetName() == m_sNativeMarkerEntityName)
			m_aNativeMarkerCandidates.Insert(entity);

		return true;
	}

	protected string FactionToMarkerColor(string factionKey, HST_CampaignPreset preset)
	{
		if (preset && factionKey == preset.m_sResistanceFactionKey)
			return "GREEN";

		if (preset && factionKey == preset.m_sOccupierFactionKey)
			return "BLUFOR";

		if (preset && factionKey == preset.m_sInvaderFactionKey)
			return "RED";

		return "REFORGER_ORANGE";
	}

	protected string FactionToMarkerTextColor(string factionKey, HST_CampaignPreset preset)
	{
		if (preset && factionKey == preset.m_sResistanceFactionKey)
			return "green";

		if (preset && factionKey == preset.m_sOccupierFactionKey)
			return "blue";

		if (preset && factionKey == preset.m_sInvaderFactionKey)
			return "red";

		return "white";
	}

	protected string ZoneToMarkerCategory(HST_ZoneState zone)
	{
		if (!zone)
			return "strategic";

		if (!zone.m_sMarkerStyle.IsEmpty())
			return zone.m_sMarkerStyle;

		HST_EZoneType zoneType = zone.m_eType;
		if (zoneType == HST_EZoneType.HST_ZONE_TOWN)
			return "town";

		if (zoneType == HST_EZoneType.HST_ZONE_HIDEOUT)
			return "hideout";

		if (zoneType == HST_EZoneType.HST_ZONE_MISSION_SITE)
			return "mission";

		if (zoneType == HST_EZoneType.HST_ZONE_RESOURCE)
			return "resource";

		if (zoneType == HST_EZoneType.HST_ZONE_FACTORY)
			return "factory";

		if (zoneType == HST_EZoneType.HST_ZONE_AIRFIELD || zoneType == HST_EZoneType.HST_ZONE_SEAPORT || zoneType == HST_EZoneType.HST_ZONE_OUTPOST)
			return "enemy_base";

		return "strategic";
	}

	protected string ZoneToMarkerIcon(HST_ZoneState zone)
	{
		if (!zone)
			return "OBJECTIVE_MARKER";

		if (zone && !zone.m_sMarkerStyle.IsEmpty())
		{
			if (zone.m_sMarkerStyle == "resource" || zone.m_sMarkerStyle == "depot")
				return "MINE_SINGLE";
			if (zone.m_sMarkerStyle == "town")
				return "POINT_OF_INTEREST2";
			if (zone.m_sMarkerStyle == "radio")
				return "RECON_OUTPOST";
			if (zone.m_sMarkerStyle == "stronghold")
				return "FORTIFICATION2";
			if (zone.m_sMarkerStyle == "enemy_base")
				return "FORTIFICATION";
			if (zone.m_sMarkerStyle == "mission_site")
				return "POINT_SPECIAL";
			if (zone.m_sMarkerStyle == "support")
				return "PICK_UP2";
		}

		HST_EZoneType zoneType = zone.m_eType;
		if (zoneType == HST_EZoneType.HST_ZONE_RESOURCE || zoneType == HST_EZoneType.HST_ZONE_FACTORY)
			return "MINE_SINGLE";

		if (zoneType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return "RECON_OUTPOST";

		if (zoneType == HST_EZoneType.HST_ZONE_HIDEOUT)
			return "PICK_UP2";

		if (zoneType == HST_EZoneType.HST_ZONE_TOWN)
			return "POINT_OF_INTEREST2";

		if (zoneType == HST_EZoneType.HST_ZONE_MISSION_SITE)
			return "POINT_SPECIAL";

		if (zoneType == HST_EZoneType.HST_ZONE_AIRFIELD || zoneType == HST_EZoneType.HST_ZONE_SEAPORT || zoneType == HST_EZoneType.HST_ZONE_OUTPOST)
			return "FORTIFICATION";

		return "OBJECTIVE_MARKER";
	}

	protected string BuildZoneMarkerLabel(HST_ZoneState zone)
	{
		if (!zone)
			return "unknown";

		string active = "inactive";
		if (zone.m_bActive)
			active = "active";

		return string.Format("%1 | owner %2 | capture %3 | %4", ResolveZoneDisplayName(zone), zone.m_sOwnerFactionKey, zone.m_iResistanceCaptureProgress, active);
	}

	protected string ZoneToMarkerTextColor(HST_ZoneState zone)
	{
		if (!zone || zone.m_sMarkerTextColor.IsEmpty())
			return "black";

		return zone.m_sMarkerTextColor;
	}

	protected string ZoneToMarkerStyle(HST_ZoneState zone)
	{
		if (!zone)
			return "strategic";

		string style = zone.m_sMarkerStyle;
		if (style.IsEmpty())
			style = ZoneToMarkerCategory(zone);
		if (zone.m_iResistanceCaptureProgress > 0)
			style = style + "_capturing";
		if (zone.m_bActive)
			style = style + "_active";

		return style;
	}

	protected string ZoneTypeToLabel(HST_EZoneType zoneType)
	{
		if (zoneType == HST_EZoneType.HST_ZONE_TOWN)
			return "Town";

		if (zoneType == HST_EZoneType.HST_ZONE_OUTPOST)
			return "Outpost";

		if (zoneType == HST_EZoneType.HST_ZONE_RESOURCE)
			return "Resource";

		if (zoneType == HST_EZoneType.HST_ZONE_FACTORY)
			return "Factory";

		if (zoneType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return "Radio";

		if (zoneType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return "Airfield";

		if (zoneType == HST_EZoneType.HST_ZONE_SEAPORT)
			return "Seaport";

		return "Zone";
	}

	protected string ResolveZoneDisplayName(HST_ZoneState zone)
	{
		if (!zone)
			return "unknown";

		if (!zone.m_sDisplayName.IsEmpty())
			return zone.m_sDisplayName;

		return HST_DefaultCatalog.GetZoneDisplayName(zone.m_sZoneId);
	}

	protected vector ResolveMissionMarkerPosition(HST_CampaignState state, HST_ActiveMissionState mission, out bool exactMissionPosition)
	{
		exactMissionPosition = false;
		if (!state || !mission)
			return "0 0 0";

		vector representativeAssetPosition = ResolveRepresentativeMissionAssetPosition(state, mission);
		if (!IsZeroVector(representativeAssetPosition))
		{
			exactMissionPosition = true;
			return representativeAssetPosition;
		}

		vector activeObjectivePosition = ResolveRepresentativeObjectivePosition(state, mission);
		if (!IsZeroVector(activeObjectivePosition))
		{
			exactMissionPosition = !IsBroadZoneFallbackPosition(state, mission.m_sTargetZoneId, activeObjectivePosition);
			return activeObjectivePosition;
		}

		if (!IsZeroVector(mission.m_vTargetPosition))
		{
			exactMissionPosition = !IsBroadZoneFallbackPosition(state, mission.m_sTargetZoneId, mission.m_vTargetPosition);
			return mission.m_vTargetPosition;
		}

		HST_GeneratedSiteState site = state.FindGeneratedSite(mission.m_sSiteId);
		if (site)
		{
			exactMissionPosition = true;
			return site.m_vPosition;
		}

		HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
		if (zone)
			return zone.m_vPosition;

		return state.m_vHQPosition;
	}

	protected vector ResolveRepresentativeMissionAssetPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bDestroyed || asset.m_bDelivered)
				continue;
			if (asset.m_sKind == "area")
				continue;

			return ResolveMissionAssetMarkerPosition(asset);
		}

		return "0 0 0";
	}

	protected vector ResolveRepresentativeObjectivePosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
				continue;

			HST_MissionAssetState linkedAsset = state.FindMissionAsset(objective.m_sLinkedRuntimeEntityId);
			return ResolveMissionObjectiveMarkerPosition(objective, linkedAsset);
		}

		return "0 0 0";
	}

	protected vector ResolveMissionObjectiveMarkerPosition(HST_MissionObjectiveState objective, HST_MissionAssetState linkedAsset)
	{
		if (linkedAsset)
			return ResolveMissionAssetMarkerPosition(linkedAsset);

		if (objective && !IsZeroVector(objective.m_vPosition))
			return objective.m_vPosition;

		return "0 0 0";
	}

	protected vector ResolveMissionAssetMarkerPosition(HST_MissionAssetState asset)
	{
		if (!asset)
			return "0 0 0";

		if (!IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;
		if (!IsZeroVector(asset.m_vSourcePosition))
			return asset.m_vSourcePosition;
		if (!IsZeroVector(asset.m_vTargetPosition))
			return asset.m_vTargetPosition;

		return "0 0 0";
	}

	protected vector ResolveMarkerPresentationPosition(HST_CampaignState state, string markerId, vector position, bool deconflict)
	{
		vector resolved = HST_WorldPositionService.ResolveGroundPosition(position, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		if (!deconflict || !state || !HasMarkerCollision(state, resolved))
			return resolved;

		for (int attempt = 0; attempt < MARKER_DECONFLICT_ATTEMPTS; attempt++)
		{
			vector candidate = BuildMarkerOffsetPosition(resolved, markerId, attempt);
			candidate = HST_WorldPositionService.ResolveGroundPosition(candidate, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
			if (!HasMarkerCollision(state, candidate))
				return candidate;
		}

		return resolved;
	}

	protected bool HasMarkerCollision(HST_CampaignState state, vector position)
	{
		if (!state)
			return false;

		float radiusSq = MARKER_DECONFLICT_RADIUS_METERS * MARKER_DECONFLICT_RADIUS_METERS;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (!marker || !marker.m_bVisible)
				continue;

			if (DistanceSq2D(marker.m_vPosition, position) <= radiusSq)
				return true;
		}

		return false;
	}

	protected vector BuildMarkerOffsetPosition(vector position, string markerId, int attempt)
	{
		vector candidate = position;
		int baseSlot = markerId.Length() + attempt;
		int ring = baseSlot / 8;
		int slot = baseSlot - ring * 8;
		float distance = MARKER_DECONFLICT_STEP_METERS + ring * 10.0;
		float x = 1.0;
		float z = 0.0;

		if (slot == 1)
		{
			x = 0.707;
			z = 0.707;
		}
		else if (slot == 2)
		{
			x = 0.0;
			z = 1.0;
		}
		else if (slot == 3)
		{
			x = -0.707;
			z = 0.707;
		}
		else if (slot == 4)
		{
			x = -1.0;
			z = 0.0;
		}
		else if (slot == 5)
		{
			x = -0.707;
			z = -0.707;
		}
		else if (slot == 6)
		{
			x = 0.0;
			z = -1.0;
		}
		else if (slot == 7)
		{
			x = 0.707;
			z = -0.707;
		}

		candidate[0] = candidate[0] + x * distance;
		candidate[2] = candidate[2] + z * distance;
		return candidate;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected bool IsBroadZoneFallbackPosition(HST_CampaignState state, string zoneId, vector position)
	{
		if (!state || zoneId.IsEmpty() || IsZeroVector(position))
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return false;

		return DistanceSq2D(zone.m_vPosition, position) <= 9.0;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}

	protected string MissionFamilyKey(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "mission";

		string primitive = mission.m_sRuntimePrimitive;
		string missionId = mission.m_sMissionId;

		if (primitive == "convoy_intercept" || missionId.Contains("convoy"))
			return "convoy";
		if (primitive == "rescue_extract" || missionId.Contains("rescue") || missionId.Contains("pow"))
			return "rescue";
		if (primitive == "kill_hvt" || missionId.Contains("assassinate") || missionId.Contains("officer"))
			return "hvt";
		if (primitive == "destroy_target" || missionId.Contains("destroy") || missionId.Contains("radio"))
			return "destroy";
		if (primitive == "deliver_supplies" || missionId.Contains("support_city") || missionId.Contains("supplies"))
			return "support";
		if (primitive == "recover_cargo" || missionId.Contains("logistics") || missionId.Contains("resource") || missionId.Contains("cache"))
			return "logistics";
		if (missionId.Contains("defend_petros"))
			return "support";
		if (primitive == "hold_area" || primitive == "clear_area" || missionId.Contains("conquest"))
			return "conquest";

		return "mission";
	}

	protected string MissionFamilyLabel(HST_ActiveMissionState mission)
	{
		string family = MissionFamilyKey(mission);
		if (family == "convoy")
			return "Convoy";
		if (family == "rescue")
			return "Rescue";
		if (family == "hvt")
			return "HVT";
		if (family == "destroy")
			return "Destroy";
		if (family == "support")
			return "Support";
		if (family == "logistics")
			return "Logistics";
		if (family == "conquest")
			return "Assault";

		return "Mission";
	}

	protected string MissionToMarkerIcon(HST_ActiveMissionState mission)
	{
		string family = MissionFamilyKey(mission);
		if (family == "convoy")
			return "POINT_SPECIAL";
		if (family == "rescue")
			return "PICK_UP2";
		if (family == "hvt")
			return "POINT_OF_INTEREST";
		if (family == "destroy")
			return "MINE_SINGLE";
		if (family == "support")
			return "PICK_UP2";
		if (family == "logistics")
			return "PICK_UP2";
		if (family == "conquest")
			return "OBJECTIVE_MARKER";

		return "OBJECTIVE_MARKER";
	}

	protected string MissionToMarkerColor(HST_ActiveMissionState mission)
	{
		return "REFORGER_ORANGE";
	}

	protected string MissionToMarkerTextColor(HST_ActiveMissionState mission)
	{
		return "gold";
	}

	protected string MissionToMarkerStyle(HST_ActiveMissionState mission)
	{
		return "mission_clickable_" + MissionFamilyKey(mission);
	}

	protected string MissionObjectiveToMarkerIcon(HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!objective)
			return MissionToMarkerIcon(mission);

		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET)
			return "POINT_OF_INTEREST";
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
			return "MINE_SINGLE";
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
			return "PICK_UP2";
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA)
			return "OBJECTIVE_MARKER";

		return MissionToMarkerIcon(mission);
	}

	protected string MissionAssetToMarkerIcon(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!asset)
			return MissionToMarkerIcon(mission);

		if (asset.m_bPickedUp && !asset.m_bDelivered)
			return "OBJECTIVE_MARKER";
		if (asset.m_sRole == "convoy_vehicle")
			return "POINT_SPECIAL";
		if (asset.m_sRole == "hvt")
			return "POINT_OF_INTEREST";
		if (asset.m_sRole == "destroy_target")
			return "MINE_SINGLE";
		if (asset.m_sRole == "city_supplies" || asset.m_sRole == "logistics_cargo" || asset.m_sRole == "convoy_payload" || asset.m_sRole == "captive" || asset.m_sRole == "convoy_captive")
			return "PICK_UP2";
		if (asset.m_sKind == "target")
			return "MINE_SINGLE";
		if (asset.m_sKind == "vehicle")
			return "POINT_SPECIAL";

		return MissionToMarkerIcon(mission);
	}

	protected string MissionAssetToMarkerColor(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		return "REFORGER_ORANGE";
	}

	protected string MissionAssetToMarkerStyle(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!asset)
			return MissionToMarkerStyle(mission) + "_asset";

		if (asset.m_bPickedUp && !asset.m_bDelivered)
			return "mission_delivery";
		if (asset.m_sRole == "convoy_vehicle")
			return "mission_convoy_vehicle";
		if (asset.m_sRole == "convoy_payload")
			return "mission_convoy_payload";
		if (asset.m_sRole == "convoy_captive")
			return "mission_convoy_captive";
		if (asset.m_sRole == "hvt")
			return "mission_hvt_asset";
		if (asset.m_sRole == "destroy_target")
			return "mission_destroy_asset";
		if (asset.m_sRole == "city_supplies")
			return "mission_support_pickup";
		if (asset.m_sRole == "logistics_cargo")
			return "mission_logistics_pickup";
		if (asset.m_sRole == "captive")
			return "mission_rescue_captive";

		return MissionToMarkerStyle(mission) + "_asset";
	}

	protected string BuildMissionMarkerLabel(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission)
			return "Mission";

		string family = MissionFamilyLabel(mission);
		string title = MissionMarkerTitle(mission);
		string targetName = ResolveZoneDisplayNameById(state, mission.m_sTargetZoneId);

		int complete;
		int total;
		if (state)
		{
			foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
			{
				if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
					continue;

				total++;
				if (objective.m_bComplete)
					complete++;
			}
		}

		return string.Format("%1 - %2: %3 | %4s | %5/%6", family, title, targetName, mission.m_iRemainingSeconds, complete, total);
	}

	protected string BuildMissionObjectiveMarkerLabel(HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!objective)
			return "Objective";

		string label = objective.m_sLabel;
		if (label.IsEmpty())
			label = objective.m_sTargetId;
		if (label.IsEmpty())
			label = "Objective";

		return string.Format("%1 - %2: %3", MissionFamilyLabel(mission), MissionMarkerTitle(mission), label);
	}

	protected string BuildMissionAssetMarkerLabel(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!asset)
			return "Mission asset";

		string verb = "Locate";
		if (!asset.m_bPickedUp && (asset.m_sKind == "cargo" || asset.m_sKind == "captive"))
			verb = "Pickup";
		else if (asset.m_bPickedUp && !asset.m_bDelivered)
			verb = "Deliver";
		else if (asset.m_sKind == "target")
			verb = "Destroy";
		else if (asset.m_sKind == "character")
			verb = "Target";
		else if (asset.m_sKind == "vehicle")
			verb = "Intercept";

		if (asset.m_sRole == "convoy_vehicle")
			verb = "Stop";
		else if (asset.m_sRole == "convoy_payload")
			verb = "Recover";
		else if (asset.m_sRole == "convoy_captive")
			verb = "Free";
		else if (asset.m_sRole == "hvt")
			verb = "Kill";

		string role = MissionAssetReadableRole(asset);
		return string.Format("%1 - %2: %3 %4", MissionFamilyLabel(mission), MissionMarkerTitle(mission), verb, role);
	}

	protected bool AreMissionObjectivesComplete(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		if (HasPendingRequiredConvoyOutcome(state, mission))
			return false;

		bool found;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			found = true;
			if (!objective.m_bComplete || objective.m_bFailed)
				return false;
		}

		return found;
	}

	protected bool HasPendingRequiredConvoyOutcome(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != "convoy_intercept")
			return false;

		if (mission.m_sMissionId == "convoy_money")
			return HasPendingConvoyAssetOutcome(state, mission, "convoy_payload");
		if (mission.m_sMissionId == "convoy_supplies")
			return !mission.m_bConvoyCrewEliminatedOutcomeApplied && HasPendingConvoyAssetOutcome(state, mission, "convoy_payload");
		if (mission.m_sMissionId == "convoy_prisoners")
			return HasPendingConvoyAssetOutcome(state, mission, "convoy_captive");
		if (mission.m_sMissionId == "convoy_ammo" || mission.m_sMissionId == "convoy_armored")
			return HasPendingConvoyVehicleCaptureOutcome(state, mission);

		return false;
	}

	protected bool HasPendingConvoyAssetOutcome(HST_CampaignState state, HST_ActiveMissionState mission, string role)
	{
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role)
				continue;

			if (!asset.m_bDelivered || !asset.m_bOutcomeApplied)
				return true;
		}

		return false;
	}

	protected bool HasPendingConvoyVehicleCaptureOutcome(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (mission.m_bConvoyVehicleCapturedOutcomeApplied)
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != "convoy_vehicle")
				continue;
			if (!asset.m_bDestroyed && !asset.m_bDelivered)
				return true;
		}

		return false;
	}

	protected bool ShouldShowConvoyOutcomeMarkers(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sRuntimePrimitive != "convoy_intercept")
			return false;

		if (mission.m_bConvoyCrewEliminatedOutcomeApplied)
			return true;
		if (mission.m_sRuntimePhase == "convoy_eliminated")
			return true;
		if (mission.m_sLastRuntimeEventKey == "convoy_complete" || mission.m_sLastRuntimeEventKey == "convoy_secured_sent")
			return true;

		return false;
	}

	protected HST_MissionAssetState SelectPendingConvoyOutcomeAsset(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != "convoy_intercept")
			return null;

		if (mission.m_sMissionId == "convoy_money" || mission.m_sMissionId == "convoy_supplies")
			return SelectPendingConvoyAssetByRole(state, mission, "convoy_payload");
		if (mission.m_sMissionId == "convoy_prisoners")
			return SelectPendingConvoyAssetByRole(state, mission, "convoy_captive");
		if ((mission.m_sMissionId == "convoy_ammo" || mission.m_sMissionId == "convoy_armored") && !mission.m_bConvoyVehicleCapturedOutcomeApplied)
			return SelectPendingConvoyAssetByRole(state, mission, "convoy_vehicle");

		return null;
	}

	protected HST_MissionAssetState SelectPendingConvoyAssetByRole(HST_CampaignState state, HST_ActiveMissionState mission, string role)
	{
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role || asset.m_bDestroyed)
				continue;
			if (role == "convoy_vehicle" && asset.m_bDelivered)
				continue;
			if ((role == "convoy_payload" || role == "convoy_captive") && asset.m_bDelivered && asset.m_bOutcomeApplied)
				continue;

			return asset;
		}

		return null;
	}

	protected vector ResolveConvoyOutcomeMarkerPosition(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!asset)
			return ResolveMissionConvoyAggregatePosition(state, mission);

		if (asset.m_bPickedUp && !asset.m_bDelivered && !IsZeroVector(asset.m_vTargetPosition))
			return asset.m_vTargetPosition;

		if (!asset.m_bPickedUp && (asset.m_sRole == "convoy_payload" || asset.m_sRole == "convoy_captive"))
			return ResolveMissionConvoyAggregatePosition(state, mission);

		if (!IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;
		if (!IsZeroVector(asset.m_vSourcePosition))
			return asset.m_vSourcePosition;

		return ResolveMissionConvoyAggregatePosition(state, mission);
	}

	protected string BuildConvoyOutcomeMarkerLabel(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		string title = MissionMarkerTitle(mission);
		string role = MissionAssetReadableRole(asset);
		if (asset && asset.m_bPickedUp && !asset.m_bDelivered)
		{
			if (asset.m_sRole == "convoy_captive")
				return string.Format("Extraction - %1: extract %2", title, role);
			return string.Format("Delivery - %1: deliver %2", title, role);
		}
		if (asset && asset.m_sRole == "convoy_captive")
			return string.Format("Secured convoy - %1: free %2", title, role);

		return string.Format("Secured convoy - %1: recover %2", title, role);
	}

	protected bool HasMissionConvoyPayloadSatisfied(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != "convoy_intercept")
			return false;

		bool hasPayload;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_sRole != "convoy_payload" && asset.m_sRole != "convoy_captive")
				continue;

			hasPayload = true;
			if (asset.m_sRole == "convoy_captive")
			{
				if (!asset.m_bDelivered)
					return false;
			}
			else if (!asset.m_bPickedUp && !asset.m_bDelivered)
			{
				return false;
			}
		}

		return hasPayload;
	}

	protected bool HasVisibleMissionAssetMarker(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bDestroyed || asset.m_bDelivered)
				continue;
			if (asset.m_sKind == "area")
				continue;
			if (asset.m_sRole == "convoy_vehicle" && HasMissionConvoyPayloadSatisfied(state, mission))
				continue;

			return true;
		}

		return false;
	}

	protected bool HasVisibleMissionObjectiveMarker(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
				continue;

			HST_MissionAssetState linkedAsset = state.FindMissionAsset(objective.m_sLinkedRuntimeEntityId);
			if (linkedAsset && linkedAsset.m_sKind != "area")
				continue;

			return true;
		}

		return false;
	}

	protected string MissionRuntimePhaseLabel(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sRuntimePhase.IsEmpty())
			return "runtime ready";

		string phase = mission.m_sRuntimePhase;
		phase.Replace("_", " ");
		return phase;
	}

	protected string MissionMarkerTitle(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "Mission";

		if (mission.m_sMissionId == "convoy_ammo")
			return "Ammo Convoy";
		if (mission.m_sMissionId == "convoy_supplies")
			return "Supply Convoy";
		if (mission.m_sMissionId == "convoy_prisoners")
			return "Prisoner Convoy";
		if (mission.m_sMissionId == "rescue_pows")
			return "POW Rescue";
		if (mission.m_sMissionId == "assassinate_officer")
			return "Kill Officer";
		if (mission.m_sMissionId == "destroy_radio_tower")
			return "Radio Tower";
		if (mission.m_sMissionId == "support_city_supplies")
			return "City Supplies";
		if (mission.m_sMissionId == "logistics_resource_cache")
			return "Resource Cache";
		if (mission.m_sMissionId == "dynamic_defend_petros")
			return "Defend Petros";

		string title = mission.m_sDisplayName;
		if (title.IsEmpty())
			title = mission.m_sMissionId;
		title = HST_DisplayNameService.ResolveReadableDisplayName(title);
		return ShortMarkerText(title, 22);
	}

	protected string MissionAssetReadableRole(HST_MissionAssetState asset)
	{
		if (!asset)
			return "asset";

		if (asset.m_sRole == "hvt")
			return "officer";
		if (asset.m_sRole == "destroy_target")
			return "target";
		if (asset.m_sRole == "logistics_cargo")
			return "cargo";
		if (asset.m_sRole == "city_supplies")
			return "supplies";
		if (asset.m_sRole == "captive")
			return "captive";
		if (asset.m_sRole == "convoy_vehicle")
			return "vehicle";
		if (asset.m_sRole == "convoy_payload")
			return "payload";
		if (asset.m_sRole == "convoy_captive")
			return "prisoner";

		string role = asset.m_sRole;
		if (role.IsEmpty())
			role = asset.m_sKind;
		if (role.IsEmpty())
			return "asset";
		role.Replace("_", " ");
		return role;
	}

	protected string ShortMarkerText(string text, int maxCharacters)
	{
		if (text.IsEmpty() || maxCharacters <= 0)
			return "";
		if (text.Length() <= maxCharacters)
			return text;
		if (maxCharacters <= 3)
			return text.Substring(0, maxCharacters);

		return text.Substring(0, maxCharacters - 3) + "...";
	}

	protected vector ResolveQRFMarkerPosition(HST_CampaignState state, HST_QRFState qrf, HST_ZoneState targetZone)
	{
		if (!state || !qrf)
			return "0 0 0";

		if (targetZone)
			return targetZone.m_vPosition;

		return state.m_vHQPosition;
	}

	protected string BuildQRFMarkerLabel(HST_CampaignState state, HST_QRFState qrf, HST_ZoneState targetZone)
	{
		if (!qrf)
			return "Enemy QRF";

		string targetName = ResolveZoneDisplayName(targetZone);
		string status = "mobilizing";
		string groupId = qrf.m_sGroupId;
		if (groupId.IsEmpty())
			groupId = "none";
		string groupRuntime = "no group";
		if (!qrf.m_sGroupId.IsEmpty() && state)
		{
			HST_ActiveGroupState group = state.FindActiveGroup(qrf.m_sGroupId);
			if (group)
				groupRuntime = group.m_sRuntimeStatus;
			if (qrf.m_bResolved && group)
				status = "active";
			else if (group && (group.m_sRuntimeStatus == "arrived" || group.m_sRuntimeStatus == "support_arrived"))
				status = "active";
			else
				status = "nearby";
		}

		string resolved = "open";
		if (qrf.m_bResolved)
			resolved = "resolved";

		int remaining = ResolveQRFRemainingSeconds(state, qrf);
		if (remaining > 0)
			return string.Format("%1 QRF near %2 | %3 | target %4 | %5 | group %6/%7 | ETA %8s", qrf.m_sFactionKey, targetName, status, qrf.m_sTargetZoneId, resolved, groupId, groupRuntime, remaining);

		return string.Format("%1 QRF near %2 | %3 | target %4 | %5 | group %6/%7", qrf.m_sFactionKey, targetName, status, qrf.m_sTargetZoneId, resolved, groupId, groupRuntime);
	}

	protected int ResolveQRFRemainingSeconds(HST_CampaignState state, HST_QRFState qrf)
	{
		if (!state || !qrf)
			return 0;

		return Math.Max(0, qrf.m_iStartedAtSecond + qrf.m_iETASeconds - state.m_iElapsedSeconds);
	}

	protected vector ResolveSupportMarkerPosition(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request)
			return "0 0 0";

		if (!IsZeroVector(request.m_vTargetPosition))
			return request.m_vTargetPosition;

		HST_ZoneState zone = state.FindZone(request.m_sTargetZoneId);
		if (zone)
			return zone.m_vPosition;

		return state.m_vHQPosition;
	}

	protected string BuildSupportMarkerLabel(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!request)
			return "Support";

		string faction = request.m_sFactionKey;
		if (faction.IsEmpty())
			faction = "Enemy";

		string typeLabel = SupportRequestTypeLabel(request.m_eType);
		string targetName = ResolveZoneDisplayNameById(state, request.m_sTargetZoneId);
		if (IsPetrosAttackSupportMarker(request))
			targetName = "HQ/Petros";
		string status = SupportStatusLabel(request.m_eStatus);
		string runtimeStatus = request.m_sRuntimeStatus;
		if (runtimeStatus.IsEmpty())
			runtimeStatus = status;
		string groupId = request.m_sGroupId;
		if (groupId.IsEmpty())
			groupId = "none";
		string groupRuntime = "no group";
		HST_ActiveGroupState group;
		if (state && !request.m_sGroupId.IsEmpty())
			group = state.FindActiveGroup(request.m_sGroupId);
		if (group)
			groupRuntime = group.m_sRuntimeStatus;

		int remaining = ResolveSupportRemainingSeconds(state, request);
		if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
		{
			string deployment = "en route";
			if (!request.m_sGroupId.IsEmpty())
				deployment = "deployed";
			if (group && group.m_sRuntimeStatus == "support_active")
				deployment = "nearby";

			return string.Format("%1 %2 to %3 | %4/%5 | %6 | group %7/%8 | ETA %9s", faction, typeLabel, targetName, status, runtimeStatus, deployment, groupId, groupRuntime, remaining);
		}

		return string.Format("%1 %2 to %3 | %4/%5 | group %6/%7 | ETA %8s", faction, typeLabel, targetName, status, runtimeStatus, groupId, groupRuntime, remaining);
	}

	protected bool IsPetrosAttackSupportMarker(HST_SupportRequestState request)
	{
		if (!request)
			return false;

		return request.m_sAssetProfileId.Contains("_petros_attack") || request.m_sRuntimeStatus.Contains("petros_attack");
	}

	protected int ResolveSupportRemainingSeconds(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!request)
			return 0;

		if (!state)
			return request.m_iETASeconds;

		return Math.Max(0, request.m_iRequestedAtSecond + request.m_iETASeconds - state.m_iElapsedSeconds);
	}

	protected string SupportStatusLabel(HST_ESupportRequestStatus status)
	{
		if (status == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
			return "active";
		if (status == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
			return "cancelled";
		if (status == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED)
			return "resolved";

		return "queued";
	}

	protected string SupportRequestTypeLabel(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return "QRF";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return "search team";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE)
			return "fire support";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING)
			return "troop landing";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return "airstrike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return "missile strike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP)
			return "patrol sweep";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_TRANSPORT)
			return "transport";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
			return "supply drop";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_EVACUATION)
			return "evacuation";

		return "support";
	}

	protected string ResolveZoneDisplayNameById(HST_CampaignState state, string zoneId)
	{
		if (!state || zoneId.IsEmpty())
			return "unknown location";

		HST_ZoneState zone = state.FindZone(zoneId);
		if (zone)
			return ResolveZoneDisplayName(zone);

		string text = HST_DefaultCatalog.GetZoneDisplayName(zoneId);
		if (!text.IsEmpty() && text != zoneId)
			return text;

		text = zoneId;
		text.Replace("_", " ");
		return text;
	}

	protected void DebugLog(string message)
	{
		if (!m_bDebugLoggingEnabled)
			return;

		Print("h-istasi map marker debug | " + message);
	}
}
