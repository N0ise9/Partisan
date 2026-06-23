[ComponentEditorProps(category: "h-istasi", description: "Player-owned h-istasi command menu request/RPC bridge")]
class HST_CommandMenuRequestComponentClass : ScriptComponentClass
{
}

class HST_CommandMenuRequestComponent : ScriptComponent
{
	static const int CLIENT_MAP_MARKER_REFRESH_MAX_RETRIES = 10;
	static const int CLIENT_MAP_MARKER_REFRESH_RETRY_MS = 250;
	static const int CLIENT_MAP_MARKER_MAINTENANCE_MS = 500;

	protected static HST_CommandMenuRequestComponent s_LocalOwner;
	protected static HST_CommandMenuRequestComponent s_ServerBroadcaster;

	protected IEntity m_OwnerEntity;
	protected string m_sLastSnapshot;
	protected string m_sLastResult;
	protected bool m_bIsLocalOwner;
	protected bool m_bNativeMapMarkerRefreshBound;
	protected bool m_bNativeMapMarkerRefreshQueued;
	protected bool m_bNativeMapMarkerRefreshForceRecreate;
	protected float m_fOwnerRetryAccumulator;
	protected int m_iNativeMapMarkerRefreshRetries;
	protected ref array<string> m_aRecentLocalNotificationKeys = {};

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		m_bIsLocalOwner = IsLocalOwner(owner);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);

		if (m_bIsLocalOwner)
			BecomeLocalOwner();
		if (Replication.IsServer() && !s_ServerBroadcaster)
			s_ServerBroadcaster = this;
	}

	override void OnDelete(IEntity owner)
	{
		if (m_bIsLocalOwner)
			UnbindNativeMapMarkerRefresh();

		if (s_LocalOwner == this)
			s_LocalOwner = null;
		if (s_ServerBroadcaster == this)
			s_ServerBroadcaster = null;

		super.OnDelete(owner);
	}

	override void EOnInit(IEntity owner)
	{
		RefreshLocalOwner(owner);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_bIsLocalOwner)
			return;

		m_fOwnerRetryAccumulator += timeSlice;
		if (m_fOwnerRetryAccumulator < 0.25)
			return;

		m_fOwnerRetryAccumulator = 0;
		RefreshLocalOwner(owner);
	}

	static HST_CommandMenuRequestComponent GetLocalOwner()
	{
		return s_LocalOwner;
	}

	protected void BecomeLocalOwner()
	{
		if (s_LocalOwner == this && m_bNativeMapMarkerRefreshBound)
			return;

		s_LocalOwner = this;
		BindNativeMapMarkerRefresh();
	}

	protected void BindNativeMapMarkerRefresh()
	{
		if (m_bNativeMapMarkerRefreshBound)
			return;

		SCR_MapEntity.GetOnMapOpenComplete().Insert(OnNativeMapOpenComplete);
		SCR_MapEntity.GetOnMapClose().Insert(OnNativeMapClose);
		SCR_MapEntity.GetOnMapPan().Insert(OnNativeMapPan);
		SCR_MapEntity.GetOnMapZoom().Insert(OnNativeMapZoom);
		m_bNativeMapMarkerRefreshBound = true;
	}

	protected void UnbindNativeMapMarkerRefresh()
	{
		if (!m_bNativeMapMarkerRefreshBound)
			return;

		SCR_MapEntity.GetOnMapOpenComplete().Remove(OnNativeMapOpenComplete);
		SCR_MapEntity.GetOnMapClose().Remove(OnNativeMapClose);
		SCR_MapEntity.GetOnMapPan().Remove(OnNativeMapPan);
		SCR_MapEntity.GetOnMapZoom().Remove(OnNativeMapZoom);
		m_bNativeMapMarkerRefreshBound = false;
		m_bNativeMapMarkerRefreshQueued = false;
		m_bNativeMapMarkerRefreshForceRecreate = false;
	}

	protected void OnNativeMapOpenComplete(MapConfiguration config)
	{
		if (!m_bIsLocalOwner || HST_SetupMapComponent.IsSetupBlocking())
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return;

		m_bNativeMapMarkerRefreshForceRecreate = true;
		m_iNativeMapMarkerRefreshRetries = 0;
		QueueClientNativeMapMarkerRefresh(0);
	}

	protected void OnNativeMapClose(MapConfiguration config)
	{
		m_bNativeMapMarkerRefreshQueued = false;
		m_bNativeMapMarkerRefreshForceRecreate = false;
		m_iNativeMapMarkerRefreshRetries = 0;
	}

	protected void OnNativeMapPan(float panX, float panY, bool adjusted)
	{
		if (!m_bIsLocalOwner || HST_SetupMapComponent.IsSetupBlocking())
			return;

		m_iNativeMapMarkerRefreshRetries = 0;
		QueueClientNativeMapMarkerRefresh(0);
	}

	protected void OnNativeMapZoom(float zoom)
	{
		if (!m_bIsLocalOwner || HST_SetupMapComponent.IsSetupBlocking())
			return;

		m_iNativeMapMarkerRefreshRetries = 0;
		QueueClientNativeMapMarkerRefresh(0);
	}

	protected void QueueClientNativeMapMarkerRefresh(int delayMs)
	{
		if (!m_bIsLocalOwner || m_bNativeMapMarkerRefreshQueued)
			return;

		m_bNativeMapMarkerRefreshQueued = true;
		GetGame().GetCallqueue().CallLater(RefreshClientNativeMapMarkerWidgets, delayMs, false);
	}

	protected void RefreshClientNativeMapMarkerWidgets()
	{
		m_bNativeMapMarkerRefreshQueued = false;
		if (!m_bIsLocalOwner || HST_SetupMapComponent.IsSetupBlocking())
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return;

		string rootName = ResolveCurrentMapRootName(mapEntity);
		bool hasMarkerUI = mapEntity.GetMapUIComponent(SCR_MapMarkersUI) != null;
		int markerUIReady;
		if (hasMarkerUI)
			markerUIReady = 1;
		bool forceRecreate = m_bNativeMapMarkerRefreshForceRecreate && !hasMarkerUI;
		if (!HasOpenMapFrame(mapEntity))
		{
			if (RetryClientNativeMapMarkerRefresh())
				return;

			Print(string.Format("h-istasi map marker client refresh | widgets 0/0 created 0 ui %1 root %2 retry %3", markerUIReady, rootName, m_iNativeMapMarkerRefreshRetries));
			return;
		}

		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
		{
			if (RetryClientNativeMapMarkerRefresh())
				return;

			Print(string.Format("h-istasi map marker client refresh | widgets 0/0 created 0 ui %1 root %2 retry %3", markerUIReady, rootName, m_iNativeMapMarkerRefreshRetries));
			return;
		}

		mapEntity.UpdateViewPort();
		vector markerUpdateMin = "-1000000 0 -1000000";
		vector markerUpdateMax = "1000000 0 1000000";

		int eligible;
		int widgets;
		int visible;
		int created;
		array<SCR_MapMarkerBase> staticMarkers = markerManager.GetStaticMarkers();
		RefreshClientNativeMarkerArray(markerManager, staticMarkers, markerUpdateMin, markerUpdateMax, forceRecreate, eligible, widgets, visible, created);
		array<SCR_MapMarkerBase> disabledMarkers = markerManager.GetDisabledMarkers();
		RefreshClientNativeMarkerArray(markerManager, disabledMarkers, markerUpdateMin, markerUpdateMax, forceRecreate, eligible, widgets, visible, created);
		if (eligible > 0)
			m_bNativeMapMarkerRefreshForceRecreate = false;

		if ((eligible == 0 || widgets == 0 || widgets < eligible) && RetryClientNativeMapMarkerRefresh())
			return;

		if (forceRecreate || created > 0 || widgets < eligible || visible < eligible || m_iNativeMapMarkerRefreshRetries > 0)
			Print(string.Format("h-istasi map marker client refresh | widgets %1/%2 visible %3/%2 created %4 ui %5 root %6 retry %7 force %8", widgets, eligible, visible, created, markerUIReady, rootName, m_iNativeMapMarkerRefreshRetries, forceRecreate));

		QueueClientNativeMapMarkerMaintenance();
	}

	protected void QueueClientNativeMapMarkerMaintenance()
	{
		if (!m_bIsLocalOwner || m_bNativeMapMarkerRefreshQueued || HST_SetupMapComponent.IsSetupBlocking())
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return;

		m_bNativeMapMarkerRefreshQueued = true;
		GetGame().GetCallqueue().CallLater(RefreshClientNativeMapMarkerWidgets, CLIENT_MAP_MARKER_MAINTENANCE_MS, false);
	}

	protected bool RetryClientNativeMapMarkerRefresh()
	{
		if (m_iNativeMapMarkerRefreshRetries >= CLIENT_MAP_MARKER_REFRESH_MAX_RETRIES)
			return false;

		m_iNativeMapMarkerRefreshRetries++;
		QueueClientNativeMapMarkerRefresh(CLIENT_MAP_MARKER_REFRESH_RETRY_MS);
		return true;
	}

	protected void RefreshClientNativeMarkerArray(SCR_MapMarkerManagerComponent markerManager, array<SCR_MapMarkerBase> markers, vector visibleMin, vector visibleMax, bool forceRecreate, inout int totalEligible, inout int totalWidgets, inout int totalVisible, inout int totalCreated)
	{
		if (!markerManager || !markers)
			return;

		foreach (SCR_MapMarkerBase marker : markers)
		{
			if (!IsHSTClientRuntimeMarker(marker))
				continue;

			totalEligible++;
			bool hadWidget = marker.GetRootWidget() != null;
			marker.SetBlocked(false);
			markerManager.SetStaticMarkerDisabled(marker, false);
			if (forceRecreate || !marker.GetRootWidget())
				marker.OnCreateMarker(true);
			if (!marker.GetRootWidget())
				continue;

			if (forceRecreate || !hadWidget)
				totalCreated++;

			marker.OnUpdate(visibleMin, visibleMax);
			marker.SetUpdateDisabled(false);
			marker.SetVisible(true);
			Widget rootWidget = marker.GetRootWidget();
			if (rootWidget)
			{
				totalWidgets++;
				if (rootWidget.IsVisible())
					totalVisible++;
			}
		}
	}

	protected bool IsHSTClientRuntimeMarker(SCR_MapMarkerBase marker)
	{
		if (!marker)
			return false;

		return marker.GetMarkerOwnerID() == -1 && marker.GetType() == SCR_EMapMarkerType.PLACED_CUSTOM;
	}

	protected bool HasOpenMapFrame(SCR_MapEntity mapEntity)
	{
		if (!mapEntity || !mapEntity.IsOpen())
			return false;

		Widget root = mapEntity.GetMapMenuRoot();
		if (!root)
			return false;

		return root.FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME) != null;
	}

	protected string ResolveCurrentMapRootName(SCR_MapEntity mapEntity)
	{
		if (!mapEntity)
			return "none";

		Widget root = mapEntity.GetMapMenuRoot();
		if (!root)
			return "none";

		string rootName = root.GetName();
		if (rootName.IsEmpty())
			return "open";

		return rootName;
	}

	static void BroadcastMissionEvent(string payload, string summary)
	{
		if (!s_ServerBroadcaster)
		{
			Print("h-istasi mission event | no server broadcaster ready", LogLevel.WARNING);
			return;
		}

		s_ServerBroadcaster.BroadcastMissionEvent_I(payload, summary);
	}

	static void BroadcastNotification(string payload, string summary = "")
	{
		if (!s_ServerBroadcaster)
		{
			Print("h-istasi notification | no server broadcaster ready", LogLevel.WARNING);
			return;
		}

		s_ServerBroadcaster.BroadcastNotification_I(payload, summary);
	}

	static void BroadcastMissionIntel(string payload)
	{
		if (!s_ServerBroadcaster)
			return;

		s_ServerBroadcaster.BroadcastMissionIntel_I(payload);
	}

	static void BroadcastSetupRefresh()
	{
		if (!s_ServerBroadcaster)
			return;

		s_ServerBroadcaster.BroadcastSetupRefresh_I();
	}

	string GetLastSnapshot()
	{
		return m_sLastSnapshot;
	}

	string GetLastResult()
	{
		return m_sLastResult;
	}

	int ResolveLocalPlayerId()
	{
		if (!m_OwnerEntity)
			return 0;

		PlayerController controller = PlayerController.Cast(m_OwnerEntity);
		if (controller && controller.GetPlayerId() > 0)
			return controller.GetPlayerId();

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		int controlledPlayerId = playerManager.GetPlayerIdFromControlledEntity(m_OwnerEntity);
		if (controlledPlayerId > 0)
			return controlledPlayerId;

		BaseRplComponent rpl = BaseRplComponent.Cast(m_OwnerEntity.FindComponent(BaseRplComponent));
		if (rpl)
			return playerManager.GetPlayerIdFromEntityRplId(rpl.Id());

		return 0;
	}

	void RequestSnapshot(string selectedTabId, string lastResult = "")
	{
		if (Replication.IsServer())
		{
			SendSnapshotToOwner(selectedTabId, lastResult);
			return;
		}

		Rpc(RpcAsk_RequestSnapshot, selectedTabId, lastResult);
	}

	void RequestAction(string selectedTabId, string commandId, string argument = "")
	{
		if (Replication.IsServer())
		{
			SendActionResultToOwner(selectedTabId, commandId, argument);
			return;
		}

		Rpc(RpcAsk_RequestAction, selectedTabId, commandId, argument);
	}

	void RequestLoadoutEditorAction(string commandId, string argument = "")
	{
		if (Replication.IsServer())
		{
			SendLoadoutEditorActionResultToOwner(commandId, argument);
			return;
		}

		Rpc(RpcAsk_RequestLoadoutEditorAction, commandId, argument);
	}

	void RequestMissionIntel()
	{
		if (Replication.IsServer())
		{
			SendMissionIntelToOwner();
			return;
		}

		Rpc(RpcAsk_RequestMissionIntel);
	}

	void RequestSetupState(string lastResult = "")
	{
		if (Replication.IsServer())
		{
			SendSetupStateToOwner(lastResult);
			return;
		}

		Rpc(RpcAsk_RequestSetupState, lastResult);
	}

	void RequestSetupValidatePosition(float worldX, float worldZ)
	{
		if (Replication.IsServer())
		{
			SendSetupValidateResultToOwner(worldX, worldZ);
			return;
		}

		Rpc(RpcAsk_RequestSetupValidatePosition, worldX, worldZ);
	}

	void RequestSetupConfirmPosition(float worldX, float worldZ)
	{
		if (Replication.IsServer())
		{
			SendSetupConfirmResultToOwner(worldX, worldZ);
			return;
		}

		Rpc(RpcAsk_RequestSetupConfirmPosition, worldX, worldZ);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestSnapshot(string selectedTabId, string lastResult)
	{
		SendSnapshotToOwner(selectedTabId, lastResult);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAction(string selectedTabId, string commandId, string argument)
	{
		SendActionResultToOwner(selectedTabId, commandId, argument);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestLoadoutEditorAction(string commandId, string argument)
	{
		SendLoadoutEditorActionResultToOwner(commandId, argument);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestMissionIntel()
	{
		SendMissionIntelToOwner();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestSetupState(string lastResult)
	{
		SendSetupStateToOwner(lastResult);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestSetupValidatePosition(float worldX, float worldZ)
	{
		SendSetupValidateResultToOwner(worldX, worldZ);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestSetupConfirmPosition(float worldX, float worldZ)
	{
		SendSetupConfirmResultToOwner(worldX, worldZ);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveSnapshot(string payload, string lastResult)
	{
		m_sLastSnapshot = payload;
		m_sLastResult = lastResult;

		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (menu)
			menu.OnServerSnapshot(payload, lastResult);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveLoadoutEditorPayload(string payload, string lastResult)
	{
		HST_LoadoutEditorComponent loadoutEditor = HST_LoadoutEditorComponent.GetLocalInstance();
		if (loadoutEditor)
			loadoutEditor.OnServerActionResult(payload, lastResult);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ReceiveMissionEvent(string payload, string summary)
	{
		HST_MissionClientComponent missionClient = HST_MissionClientComponent.GetLocalInstance();
		if (missionClient)
			missionClient.OnServerMissionEvent(payload, summary);
		else
			Print("h-istasi mission event | " + summary);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ReceiveNotification(string payload, string summary)
	{
		DeliverNotificationLocal(payload, summary);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveMissionIntelOwner(string payload)
	{
		HST_MissionClientComponent missionClient = HST_MissionClientComponent.GetLocalInstance();
		if (missionClient)
			missionClient.OnServerMissionIntel(payload);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ReceiveMissionIntel(string payload)
	{
		HST_MissionClientComponent missionClient = HST_MissionClientComponent.GetLocalInstance();
		if (missionClient)
			missionClient.OnServerMissionIntel(payload);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveSetupState(string payload, string lastResult)
	{
		HST_SetupMapComponent setupMap = HST_SetupMapComponent.GetLocalInstance();
		if (setupMap)
			setupMap.OnServerSetupState(payload, lastResult);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveSetupResult(string payload)
	{
		HST_SetupMapComponent setupMap = HST_SetupMapComponent.GetLocalInstance();
		if (setupMap)
			setupMap.OnServerSetupResult(payload);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_RefreshSetupState()
	{
		HST_SetupMapComponent setupMap = HST_SetupMapComponent.GetLocalInstance();
		if (setupMap)
			setupMap.RequestSetupStateNow();
	}

	protected void SendSnapshotToOwner(string selectedTabId, string lastResult)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSnapshot("HST_MENU|offline|0\nSTATUS|h-istasi menu | coordinator not ready\nEND", lastResult);
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity);
		string payload = coordinator.BuildVisibleMenuPayload(playerId, selectedTabId, lastResult);
		DeliverSnapshot(payload, lastResult);
	}

	protected void BroadcastMissionEvent_I(string payload, string summary)
	{
		Rpc(RpcDo_ReceiveMissionEvent, payload, summary);
	}

	protected void BroadcastNotification_I(string payload, string summary)
	{
		DeliverNotificationLocal(payload, summary);
		Rpc(RpcDo_ReceiveNotification, payload, summary);
	}

	protected void BroadcastMissionIntel_I(string payload)
	{
		Rpc(RpcDo_ReceiveMissionIntel, payload);
	}

	protected void BroadcastSetupRefresh_I()
	{
		RpcDo_RefreshSetupState();
		Rpc(RpcDo_RefreshSetupState);
	}

	protected void DeliverNotificationLocal(string payload, string summary)
	{
		HST_MissionClientComponent missionClient = HST_MissionClientComponent.GetLocalInstance();
		HST_CommandMenuComponent commandMenu = HST_CommandMenuComponent.GetLocalInstance();
		if (!missionClient && !commandMenu)
			return;

		string key = payload;
		if (key.IsEmpty())
			key = summary;
		if (IsDuplicateLocalNotification(key))
			return;

		if (missionClient)
		{
			missionClient.OnServerNotification(payload, summary);
			return;
		}

		Print("h-istasi notification | local fallback | " + summary);
		commandMenu.ShowExternalNotification("h-istasi", summary, 5.0);
	}

	protected bool IsDuplicateLocalNotification(string key)
	{
		if (key.IsEmpty())
			return false;

		if (m_aRecentLocalNotificationKeys.Contains(key))
			return true;

		m_aRecentLocalNotificationKeys.Insert(key);
		while (m_aRecentLocalNotificationKeys.Count() > 32)
			m_aRecentLocalNotificationKeys.Remove(0);

		return false;
	}

	protected void SendActionResultToOwner(string selectedTabId, string commandId, string argument)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSnapshot("HST_MENU|offline|0\nSTATUS|h-istasi command | coordinator not ready\nEND", "coordinator not ready");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity);
		string result = coordinator.RequestVisibleMenuCommand(playerId, selectedTabId, commandId, argument);
		string payload = coordinator.BuildVisibleMenuPayload(playerId, selectedTabId, result);
		DeliverSnapshot(payload, result);
	}

	protected void SendLoadoutEditorActionResultToOwner(string commandId, string argument)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverLoadoutEditorPayload("HST_LOADOUT_EDITOR|offline||false|0|0|0|0\nPREVIEW|false|0 0 0|0|coordinator not ready\nEND", "coordinator not ready");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity);
		string result = coordinator.RequestLoadoutEditorCommand(playerId, commandId, argument);
		string payload;
		if (commandId == "loadout_editor_candidates")
			payload = coordinator.BuildLoadoutEditorCandidatePayload(playerId, argument);
		else
			payload = coordinator.BuildLoadoutEditorPayload(playerId);
		DeliverLoadoutEditorPayload(payload, result);
	}

	protected void SendMissionIntelToOwner()
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverMissionIntel("HST_MISSION_INTEL|offline|0\nEND");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity);
		DeliverMissionIntel(coordinator.BuildMissionIntelPayload(playerId));
	}

	protected void SendSetupStateToOwner(string lastResult = "")
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSetupState("HST_SETUP|offline|0|0|0|0|12800|12800|coordinator not ready|0\nEND", lastResult);
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity);
		DeliverSetupState(coordinator.BuildSetupMapPayload(playerId, lastResult), lastResult);
	}

	protected void SendSetupValidateResultToOwner(float worldX, float worldZ)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSetupResult("HST_SETUP_RESULT|validate|0|0|0|0|coordinator not ready");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity);
		string result = coordinator.RequestSetupValidateHQPosition(playerId, worldX, worldZ);
		DeliverSetupResult(result);
	}

	protected void SendSetupConfirmResultToOwner(float worldX, float worldZ)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSetupResult("HST_SETUP_RESULT|confirm|0|0|0|0|coordinator not ready");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity);
		string result = coordinator.RequestSetupConfirmHQPosition(playerId, worldX, worldZ);
		DeliverSetupResult(result);
		DeliverSetupState(coordinator.BuildSetupMapPayload(playerId, result), result);
	}

	protected void DeliverSnapshot(string payload, string lastResult)
	{
		BaseRplComponent rpl = BaseRplComponent.Cast(m_OwnerEntity.FindComponent(BaseRplComponent));
		if (Replication.IsServer() && (!rpl || rpl.IsOwner()))
		{
			RpcDo_ReceiveSnapshot(payload, lastResult);
			return;
		}

		Rpc(RpcDo_ReceiveSnapshot, payload, lastResult);
	}

	protected void DeliverLoadoutEditorPayload(string payload, string lastResult)
	{
		BaseRplComponent rpl = BaseRplComponent.Cast(m_OwnerEntity.FindComponent(BaseRplComponent));
		if (Replication.IsServer() && (!rpl || rpl.IsOwner()))
		{
			RpcDo_ReceiveLoadoutEditorPayload(payload, lastResult);
			return;
		}

		Rpc(RpcDo_ReceiveLoadoutEditorPayload, payload, lastResult);
	}

	protected void DeliverMissionIntel(string payload)
	{
		BaseRplComponent rpl = BaseRplComponent.Cast(m_OwnerEntity.FindComponent(BaseRplComponent));
		if (Replication.IsServer() && (!rpl || rpl.IsOwner()))
		{
			RpcDo_ReceiveMissionIntelOwner(payload);
			return;
		}

		Rpc(RpcDo_ReceiveMissionIntelOwner, payload);
	}

	protected void DeliverSetupState(string payload, string lastResult)
	{
		BaseRplComponent rpl = BaseRplComponent.Cast(m_OwnerEntity.FindComponent(BaseRplComponent));
		if (Replication.IsServer() && (!rpl || rpl.IsOwner()))
		{
			RpcDo_ReceiveSetupState(payload, lastResult);
			return;
		}

		Rpc(RpcDo_ReceiveSetupState, payload, lastResult);
	}

	protected void DeliverSetupResult(string payload)
	{
		BaseRplComponent rpl = BaseRplComponent.Cast(m_OwnerEntity.FindComponent(BaseRplComponent));
		if (Replication.IsServer() && (!rpl || rpl.IsOwner()))
		{
			RpcDo_ReceiveSetupResult(payload);
			return;
		}

		Rpc(RpcDo_ReceiveSetupResult, payload);
	}

	protected void RefreshLocalOwner(IEntity owner)
	{
		if (m_bIsLocalOwner)
			return;

		if (!IsLocalOwner(owner))
			return;

		m_bIsLocalOwner = true;
		BecomeLocalOwner();
	}

	protected bool IsLocalOwner(IEntity owner)
	{
		if (!owner)
			return false;

		BaseRplComponent rpl = BaseRplComponent.Cast(owner.FindComponent(BaseRplComponent));
		return !rpl || rpl.IsOwner();
	}
}
