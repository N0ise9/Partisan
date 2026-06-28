class HST_PlayerMapMarkerService
{
	static const string PLAYER_MARKER_PREFIX = "hst_player_marker_";
	static const float REFRESH_INTERVAL_SECONDS = 1.5;

	protected ref map<string, ref HST_MapMarkerRecord> m_mDesiredPlayerMarkers = new map<string, ref HST_MapMarkerRecord>();
	protected ref HST_NativeMapMarkerReconciler m_Reconciler = new HST_NativeMapMarkerReconciler();
	protected bool m_bEnabled = true;
	protected bool m_bRefreshRequested = true;
	protected bool m_bDebugLoggingEnabled;
	protected float m_fRefreshAccumulator = REFRESH_INTERVAL_SECONDS;
	protected string m_sLastSignature;
	protected string m_sLastFailureReportSignature;
	protected int m_iRevision;

	void SetDebugLoggingEnabled(bool enabled)
	{
		m_bDebugLoggingEnabled = enabled;
		if (m_Reconciler)
			m_Reconciler.SetDebugLoggingEnabled(enabled);
	}

	void SetEnabled(bool enabled)
	{
		if (m_bEnabled == enabled)
			return;

		m_bEnabled = enabled;
		if (!m_bEnabled)
		{
			ClearAll();
			return;
		}

		RequestRefresh("enabled");
	}

	void RequestRefresh(string reason = "")
	{
		m_bRefreshRequested = true;
		m_fRefreshAccumulator = REFRESH_INTERVAL_SECONDS;
		DebugLog("refresh requested " + reason);
	}

	bool Tick(HST_CampaignState state, float timeSlice)
	{
		if (!Replication.IsServer())
			return false;

		if (!m_bEnabled)
		{
			ClearAll();
			return false;
		}

		m_fRefreshAccumulator += timeSlice;
		if (!m_bRefreshRequested && m_fRefreshAccumulator < REFRESH_INTERVAL_SECONDS)
			return false;

		m_fRefreshAccumulator = 0.0;
		m_bRefreshRequested = false;
		return Refresh(state);
	}

	bool ClearAll()
	{
		bool changed;
		m_mDesiredPlayerMarkers.Clear();
		m_sLastSignature = "";
		m_sLastFailureReportSignature = "";
		m_bRefreshRequested = false;
		if (m_Reconciler)
			changed = m_Reconciler.Clear();

		return changed;
	}

	string BuildRuntimeReport()
	{
		string enabledText = "off";
		if (m_bEnabled)
			enabledText = "on";

		string refreshText = "idle";
		if (m_bRefreshRequested)
			refreshText = "requested";

		int trackedDynamic;
		int liveDynamic;
		int created;
		int updated;
		int removed;
		int unchanged;
		int failed;
		int nativeDynamic;
		bool managerReady;
		bool playerEntryReady;
		if (m_Reconciler)
		{
			trackedDynamic = m_Reconciler.GetTrackedDynamicHandleCount();
			liveDynamic = m_Reconciler.CountTrackedDynamicLive();
			HST_MapMarkerReconcileResult result = m_Reconciler.GetLastResult();
			if (result)
			{
				created = result.m_iCreated;
				updated = result.m_iUpdated;
				removed = result.m_iRemoved;
				unchanged = result.m_iUnchanged;
				failed = result.m_iFailed;
			}
		}

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager)
		{
			managerReady = true;
			nativeDynamic = markerManager.GetDynamicMarkers().Count();
			SCR_MapMarkerConfig markerConfig = markerManager.GetMarkerConfig();
			if (markerConfig && markerConfig.GetMarkerEntryConfigByType(SCR_EMapMarkerType.HST_PLAYER))
				playerEntryReady = true;
		}

		string report = string.Format("h-istasi player marker report | enabled %1 | desired %2 | tracked %3 | live %4 | refresh %5", enabledText, m_mDesiredPlayerMarkers.Count(), trackedDynamic, liveDynamic, refreshText);
		report = report + string.Format("\nnative marker manager | ready %1 | hst player entry %2 | native dynamic %3", managerReady, playerEntryReady, nativeDynamic);
		report = report + string.Format("\nlast reconcile | created %1 | updated %2 | removed %3 | unchanged %4 | failed %5", created, updated, removed, unchanged, failed);

		int detailCount;
		foreach (string id, HST_MapMarkerRecord record : m_mDesiredPlayerMarkers)
		{
			if (detailCount >= 12)
				break;
			if (!record)
				continue;

			report = report + string.Format("\nplayer marker | id %1 | label %2 | target %3 | config %4", id, record.m_sShortLabel, record.m_TargetEntity != null, record.m_iConfigId);
			detailCount++;
		}

		if (m_mDesiredPlayerMarkers.Count() > detailCount)
			report = report + string.Format("\nplayer marker | omitted %1 additional marker(s)", m_mDesiredPlayerMarkers.Count() - detailCount);

		return report;
	}

	protected bool Refresh(HST_CampaignState state)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		m_mDesiredPlayerMarkers.Clear();
		string signature;
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = ResolveControlledPlayerEntity(playerManager, playerId);
			if (!IsLivingEntity(playerEntity))
				continue;

			string playerName = ResolvePlayerName(playerManager, playerId);
			string entitySignature = ResolveEntitySignature(playerEntity);
			signature = signature + string.Format("\n%1|%2|%3", playerId, entitySignature, playerName);

			HST_MapMarkerRecord record = BuildPlayerRecord(playerId, playerEntity, playerName, state);
			if (record && !record.m_sId.IsEmpty())
				m_mDesiredPlayerMarkers.Set(record.m_sId, record);
		}

		if (signature == m_sLastSignature && m_Reconciler && m_Reconciler.GetTrackedDynamicHandleCount() == m_mDesiredPlayerMarkers.Count() && m_Reconciler.CountTrackedDynamicLive() == m_mDesiredPlayerMarkers.Count())
			return false;

		if (!m_Reconciler)
			return false;

		bool changed = m_Reconciler.Reconcile(m_mDesiredPlayerMarkers);
		HST_MapMarkerReconcileResult result = m_Reconciler.GetLastResult();
		bool failed;
		if (result && result.m_iFailed > 0)
		{
			failed = true;
			ReportReconcileFailure(signature, result);
			m_sLastSignature = "";
			m_bRefreshRequested = true;
		}
		else
		{
			m_sLastSignature = signature;
			m_sLastFailureReportSignature = "";
		}

		DebugLog(string.Format("reconciled players=%1 changed=%2 failed=%3 tracked=%4", m_mDesiredPlayerMarkers.Count(), changed, failed, m_Reconciler.GetTrackedDynamicHandleCount()));
		return changed;
	}

	protected void ReportReconcileFailure(string signature, HST_MapMarkerReconcileResult result)
	{
		if (!result)
			return;

		string reportSignature = string.Format("%1|%2|%3|%4", signature, result.m_iFailed, m_mDesiredPlayerMarkers.Count(), m_Reconciler.GetTrackedDynamicHandleCount());
		if (reportSignature == m_sLastFailureReportSignature)
			return;

		m_sLastFailureReportSignature = reportSignature;
		Print(string.Format("h-istasi player map marker | native reconcile failed | desired=%1 failed=%2 trackedDynamic=%3 | verify SCR_MapMarkerManagerComponent uses HST_PlayerMapMarkerConfig.conf and SCR_EMapMarkerType.HST_PLAYER has an active HST_PlayerMapMarkerEntry", m_mDesiredPlayerMarkers.Count(), result.m_iFailed, m_Reconciler.GetTrackedDynamicHandleCount()), LogLevel.WARNING);
	}

	protected HST_MapMarkerRecord BuildPlayerRecord(int playerId, IEntity entity, string playerName, HST_CampaignState state)
	{
		if (playerId <= 0 || !entity)
			return null;

		HST_MapMarkerRecord record = new HST_MapMarkerRecord();
		record.m_sId = PLAYER_MARKER_PREFIX + string.Format("%1", playerId);
		record.m_eRenderMode = HST_EMapMarkerRenderMode.DYNAMIC_ENTITY;
		record.m_TargetEntity = entity;
		record.m_vWorldPosition = entity.GetOrigin();
		record.m_sLabel = playerName;
		record.m_sShortLabel = playerName;
		record.m_sCategory = "player";
		record.m_iPriority = 90;
		record.m_eMarkerType = SCR_EMapMarkerType.HST_PLAYER;
		record.m_iConfigId = playerId;
		record.m_bVisible = true;
		record.m_bCanPlayerRemove = false;
		record.m_bLocalOnly = false;
		record.m_bServerMarker = true;
		m_iRevision++;
		record.m_iRevision = m_iRevision;
		if (state)
			record.m_iLastChangedSecond = state.m_iElapsedSeconds;

		return record;
	}

	protected IEntity ResolveControlledPlayerEntity(PlayerManager playerManager, int playerId)
	{
		if (!playerManager || playerId <= 0)
			return null;

		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (controlledEntity)
			return controlledEntity;

		return SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
	}

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent scrDamage = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		if (scrDamage)
			return scrDamage.GetState() != EDamageState.DESTROYED;

		DamageManagerComponent damage = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
		if (damage)
			return damage.GetState() != EDamageState.DESTROYED;

		return true;
	}

	protected string ResolvePlayerName(PlayerManager playerManager, int playerId)
	{
		string playerName;
		if (playerManager && playerId > 0)
			playerName = playerManager.GetPlayerName(playerId);

		playerName = playerName.Trim();
		if (!playerName.IsEmpty())
			return playerName;

		return string.Format("Player %1", playerId);
	}

	protected string ResolveEntitySignature(IEntity entity)
	{
		if (!entity)
			return "";

		BaseRplComponent rpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("rpl_%1", rpl.Id());

		vector origin = entity.GetOrigin();
		return string.Format("pos_%1_%2", Math.Round(origin[0]), Math.Round(origin[2]));
	}

	protected void DebugLog(string message)
	{
		if (!m_bDebugLoggingEnabled)
			return;

		Print("h-istasi player map marker debug | " + message);
	}
}
