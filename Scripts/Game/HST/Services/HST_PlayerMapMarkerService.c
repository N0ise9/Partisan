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
		m_bRefreshRequested = false;
		if (m_Reconciler)
			changed = m_Reconciler.Clear();

		return changed;
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

		if (signature == m_sLastSignature && m_Reconciler && m_Reconciler.GetTrackedDynamicHandleCount() == m_mDesiredPlayerMarkers.Count())
			return false;

		if (!m_Reconciler)
			return false;

		bool changed = m_Reconciler.Reconcile(m_mDesiredPlayerMarkers);
		HST_MapMarkerReconcileResult result = m_Reconciler.GetLastResult();
		bool failed;
		if (result && result.m_iFailed > 0)
		{
			failed = true;
			m_sLastSignature = "";
			m_bRefreshRequested = true;
		}
		else
		{
			m_sLastSignature = signature;
		}

		DebugLog(string.Format("reconciled players=%1 changed=%2 failed=%3 tracked=%4", m_mDesiredPlayerMarkers.Count(), changed, failed, m_Reconciler.GetTrackedDynamicHandleCount()));
		return changed;
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
		record.m_iConfigId = -1;
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
