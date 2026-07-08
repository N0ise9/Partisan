[ComponentEditorProps(category: "h-istasi", description: "Player-owned h-istasi command menu request/RPC bridge")]
class HST_CommandMenuRequestComponentClass : ScriptComponentClass
{
}

class HST_CommandMenuRequestComponent : ScriptComponent
{
	static const int CLIENT_MAP_MARKER_REFRESH_MAX_RETRIES = 10;
	static const int CLIENT_MAP_MARKER_REFRESH_RETRY_MS = 250;
	static const float RUNTIME_FEATURE_SETTINGS_RETRY_SECONDS = 0.5;
	static const float INFINITE_STAMINA_REFILL_INTERVAL_SECONDS = 0.1;
	static const float INFINITE_STAMINA_TARGET = 0.98;
	static const ResourceName PLAYER_MARKER_CONFIG = "{6985327711306212}Configs/Map/HST_PlayerMapMarkerConfig.conf";

	protected static HST_CommandMenuRequestComponent s_LocalOwner;
	protected static HST_CommandMenuRequestComponent s_ServerBroadcaster;
	protected static bool s_bLocalPetrosRelocationActive;

	protected IEntity m_OwnerEntity;
	protected string m_sLastSnapshot;
	protected string m_sLastResult;
	protected bool m_bIsLocalOwner;
	protected bool m_bNativeMapMarkerRefreshBound;
	protected bool m_bNativeMapMarkerRefreshQueued;
	protected bool m_bDebugLoggingEnabled;
	protected bool m_bRuntimeFeatureSettingsSynced;
	protected bool m_bInfiniteStaminaEnabled;
	protected float m_fOwnerRetryAccumulator;
	protected float m_fRuntimeFeatureSettingsRetryAccumulator;
	protected float m_fInfiniteStaminaAccumulator;
	protected int m_iNativeMapMarkerRefreshRetries;
	protected int m_iInfiniteStaminaRefillCount;
	protected ref array<string> m_aRecentLocalNotificationKeys = {};

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		m_bIsLocalOwner = IsLocalOwner(owner);
		m_bDebugLoggingEnabled = HST_RuntimeSettingsService.LoadDebugLoggingEnabledQuiet();
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);

		if (m_bIsLocalOwner)
			BecomeLocalOwner();
		if (Replication.IsServer() && !s_ServerBroadcaster)
			s_ServerBroadcaster = this;
	}

	override void OnDelete(IEntity owner)
	{
		if (m_bIsLocalOwner)
		{
			UnbindNativeMapMarkerRefresh();
			SCR_StaminaBlurEffect.SetHistasiInfiniteStaminaVisualSuppressed(false);
		}

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
		{
			TickRuntimeFeatureSettingsRequest(timeSlice);
			TickInfiniteStamina(timeSlice);
			return;
		}

		m_fOwnerRetryAccumulator += timeSlice;
		if (m_fOwnerRetryAccumulator < 0.25)
			return;

		m_fOwnerRetryAccumulator = 0;
		RefreshLocalOwner(owner);
	}

	static HST_CommandMenuRequestComponent GetLocalOwner()
	{
		if (!s_LocalOwner)
			RecoverLocalOwnerFromController("static lookup");

		return s_LocalOwner;
	}

	static bool IsLocalPetrosRelocationActive()
	{
		return s_bLocalPetrosRelocationActive;
	}

	protected void BecomeLocalOwner()
	{
		if (s_LocalOwner == this)
			return;

		s_LocalOwner = this;
		BindNativeMapMarkerRefresh();
		RequestRuntimeFeatureSettingsNow("local owner");
	}

	protected static void RecoverLocalOwnerFromController(string reason)
	{
		PlayerController localController = GetGame().GetPlayerController();
		if (!localController)
			return;

		HST_CommandMenuRequestComponent localComponent = HST_CommandMenuRequestComponent.Cast(localController.FindComponent(HST_CommandMenuRequestComponent));
		if (!localComponent)
			return;

		localComponent.ClaimRecoveredLocalOwner(reason);
	}

	protected void ClaimRecoveredLocalOwner(string reason)
	{
		if (!m_bIsLocalOwner)
			m_bIsLocalOwner = true;

		DebugLog("recovering local request bridge via " + reason);
		BecomeLocalOwner();
	}

	protected void BindNativeMapMarkerRefresh()
	{
		if (m_bNativeMapMarkerRefreshBound)
			return;

		SCR_MapEntity.GetOnMapOpenComplete().Insert(OnNativeMapOpenComplete);
		SCR_MapEntity.GetOnMapClose().Insert(OnNativeMapClose);
		m_bNativeMapMarkerRefreshBound = true;
	}

	protected void UnbindNativeMapMarkerRefresh()
	{
		if (m_bNativeMapMarkerRefreshBound)
		{
			SCR_MapEntity.GetOnMapOpenComplete().Remove(OnNativeMapOpenComplete);
			SCR_MapEntity.GetOnMapClose().Remove(OnNativeMapClose);
		}

		m_bNativeMapMarkerRefreshBound = false;
		m_bNativeMapMarkerRefreshQueued = false;
	}

	protected void OnNativeMapOpenComplete(MapConfiguration config)
	{
		if (!m_bIsLocalOwner || HST_SetupMapComponent.IsSetupBlocking())
			return;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
			return;

		m_iNativeMapMarkerRefreshRetries = 0;
		QueueClientNativeMapMarkerRefresh(0);
	}

	protected void OnNativeMapClose(MapConfiguration config)
	{
		m_bNativeMapMarkerRefreshQueued = false;
		m_iNativeMapMarkerRefreshRetries = 0;
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
		bool hasToolMenuUI = mapEntity.GetMapUIComponent(SCR_MapToolMenuUI) != null;
		bool hasToolInteractionUI = mapEntity.GetMapUIComponent(SCR_MapToolInteractionUI) != null;
		bool hasToolMenuWidget = HasMapWidget(mapEntity, "ToolMenu");
		bool hasToolMenuBarWidget = HasMapWidget(mapEntity, "ToolMenuBar");
		int markerUIReady;
		if (hasMarkerUI)
			markerUIReady = 1;
		int toolMenuUIReady;
		if (hasToolMenuUI)
			toolMenuUIReady = 1;
		int toolInteractionUIReady;
		if (hasToolInteractionUI)
			toolInteractionUIReady = 1;
		int toolMenuWidgetReady;
		if (hasToolMenuWidget)
			toolMenuWidgetReady = 1;
		int toolMenuBarWidgetReady;
		if (hasToolMenuBarWidget)
			toolMenuBarWidgetReady = 1;
		if (!HasOpenMapFrame(mapEntity))
		{
			if (RetryClientNativeMapMarkerRefresh())
				return;

			DebugLog(string.Format("map marker client refresh | map frame unavailable ui %1 root %2 retry %3", markerUIReady, rootName, m_iNativeMapMarkerRefreshRetries));
			return;
		}

		DebugLog(string.Format("map ui ready | root %1 markerUI %2 toolMenuUI %3 toolInteractionUI %4 toolMenuWidget %5 toolMenuBarWidget %6 retry %7", rootName, markerUIReady, toolMenuUIReady, toolInteractionUIReady, toolMenuWidgetReady, toolMenuBarWidgetReady, m_iNativeMapMarkerRefreshRetries));
		if (!RefreshHSTPlayerMarkerWidgets())
			RetryClientNativeMapMarkerRefresh();
	}

	protected bool RefreshHSTPlayerMarkerWidgets()
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!markerManager)
			return true;

		markerManager.EnsureHSTMarkerConfig(PLAYER_MARKER_CONFIG);
		array<SCR_MapMarkerEntity> dynamicMarkers = markerManager.GetDynamicMarkers();
		int playerMarkers;
		int readyMarkers;
		foreach (SCR_MapMarkerEntity marker : dynamicMarkers)
		{
			if (!marker || marker.GetType() != SCR_EMapMarkerType.HST_PLAYER)
				continue;

			playerMarkers++;
			if (marker.EnsureHSTPlayerMarkerWidget())
				readyMarkers++;
		}

		if (playerMarkers == 0)
		{
			DebugLog(string.Format("player marker widget refresh | no dynamic player markers retry %1", m_iNativeMapMarkerRefreshRetries));
			return false;
		}

		if (playerMarkers > 0)
			DebugLog(string.Format("player marker widget refresh | ready %1/%2 retry %3", readyMarkers, playerMarkers, m_iNativeMapMarkerRefreshRetries));

		return readyMarkers >= playerMarkers;
	}

	protected bool RetryClientNativeMapMarkerRefresh()
	{
		if (m_iNativeMapMarkerRefreshRetries >= CLIENT_MAP_MARKER_REFRESH_MAX_RETRIES)
			return false;

		m_iNativeMapMarkerRefreshRetries++;
		QueueClientNativeMapMarkerRefresh(CLIENT_MAP_MARKER_REFRESH_RETRY_MS);
		return true;
	}

	protected bool HasOpenMapFrame(SCR_MapEntity mapEntity)
	{
		if (!mapEntity || !mapEntity.IsOpen())
			return false;

		return HasMapWidget(mapEntity, SCR_MapConstants.MAP_FRAME_NAME);
	}

	protected bool HasMapWidget(SCR_MapEntity mapEntity, string widgetName)
	{
		if (!mapEntity || !mapEntity.IsOpen() || widgetName.IsEmpty())
			return false;

		Widget root = mapEntity.GetMapMenuRoot();
		if (!root)
			return false;

		return root.FindAnyWidget(widgetName) != null;
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

	protected void ApplyDebugLoggingFromMenuPayload(string payload)
	{
		string debugLine = ExtractPayloadLine(payload, "DEBUG|");
		if (debugLine.IsEmpty())
			return;

		m_bDebugLoggingEnabled = ParsePayloadBool(ExtractPipeField(debugLine, 1));
	}

	protected void ApplyRuntimeFeatureSettingsFromMenuPayload(string payload)
	{
		string featureLine = ExtractPayloadLine(payload, "FEATURE|infiniteStamina|");
		if (featureLine.IsEmpty())
			return;

		ApplyInfiniteStaminaSetting(ParsePayloadBool(ExtractPipeField(featureLine, 2)), "menu payload");
	}

	protected string ExtractPayloadLine(string payload, string prefix)
	{
		if (payload.IsEmpty() || prefix.IsEmpty())
			return "";

		int start = payload.IndexOf(prefix);
		if (start < 0)
			return "";

		int end = payload.IndexOfFrom(start, "\n");
		if (end < 0)
			end = payload.Length();

		return payload.Substring(start, end - start);
	}

	protected string ExtractPipeField(string line, int fieldIndex)
	{
		int start;
		for (int i = 0; i < fieldIndex; i++)
		{
			start = line.IndexOfFrom(start, "|");
			if (start < 0)
				return "";

			start++;
		}

		int end = line.IndexOfFrom(start, "|");
		if (end < 0)
			end = line.Length();

		return line.Substring(start, end - start);
	}

	protected bool ParsePayloadBool(string value)
	{
		return value == "true" || value == "1";
	}

	protected void DebugLog(string message)
	{
		if (!m_bDebugLoggingEnabled)
			return;

		Print("h-istasi request bridge debug | " + message);
	}

	static void BroadcastMissionEvent(string payload, string summary)
	{
		if (BroadcastMissionEventToConnectedOwners(payload, summary) > 0)
			return;

		if (!s_ServerBroadcaster)
		{
			Print("h-istasi mission event | no server broadcaster ready", LogLevel.WARNING);
			return;
		}

		s_ServerBroadcaster.BroadcastMissionEvent_I(payload, summary);
	}

	static void BroadcastNotification(string payload, string summary = "")
	{
		if (BroadcastNotificationToConnectedOwners(payload, summary) > 0)
			return;

		if (!s_ServerBroadcaster)
		{
			Print("h-istasi notification | no server broadcaster ready", LogLevel.WARNING);
			return;
		}

		s_ServerBroadcaster.BroadcastNotification_I(payload, summary);
	}

	static bool SendOwnerNotification(int playerId, string payload, string summary = "")
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		PlayerController controller = playerManager.GetPlayerController(playerId);
		if (!controller)
		{
			Print(string.Format("h-istasi notification | owner RPC unavailable: player controller missing player=%1", playerId), LogLevel.WARNING);
			return false;
		}

		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.Cast(controller.FindComponent(HST_CommandMenuRequestComponent));
		if (!request)
		{
			Print(string.Format("h-istasi notification | owner RPC unavailable: request bridge missing player=%1", playerId), LogLevel.WARNING);
			return false;
		}

		request.DeliverNotificationOwner(payload, summary);
		return true;
	}

	static int CountConnectedPlayers()
	{
		if (!Replication.IsServer())
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		return playerIds.Count();
	}

	static int CountConnectedRequestBridges()
	{
		if (!Replication.IsServer())
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		int bridgeCount;
		foreach (int playerId : playerIds)
		{
			if (ResolvePlayerRequestBridge(playerManager, playerId))
				bridgeCount++;
		}

		return bridgeCount;
	}

	static bool SendPetrosRelocationStateOwner(int playerId, bool active)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		PlayerController controller = playerManager.GetPlayerController(playerId);
		if (!controller)
		{
			Print(string.Format("h-istasi Petros relocation | owner state RPC unavailable: player controller missing player=%1 active=%2", playerId, active), LogLevel.WARNING);
			return false;
		}

		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.Cast(controller.FindComponent(HST_CommandMenuRequestComponent));
		if (!request)
		{
			Print(string.Format("h-istasi Petros relocation | owner state RPC unavailable: request bridge missing player=%1 active=%2", playerId, active), LogLevel.WARNING);
			return false;
		}

		request.DeliverPetrosRelocationState(active);
		return true;
	}

	static void BroadcastMissionIntel(string payload)
	{
		if (BroadcastMissionIntelToConnectedOwners(payload) > 0)
			return;

		if (!s_ServerBroadcaster)
			return;

		s_ServerBroadcaster.BroadcastMissionIntel_I(payload);
	}

	protected static int BroadcastMissionEventToConnectedOwners(string payload, string summary)
	{
		if (!Replication.IsServer())
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		int delivered;
		foreach (int playerId : playerIds)
		{
			HST_CommandMenuRequestComponent request = ResolvePlayerRequestBridge(playerManager, playerId);
			if (!request)
				continue;

			request.DeliverMissionEventOwner(payload, summary);
			delivered++;
		}

		return delivered;
	}

	protected static int BroadcastNotificationToConnectedOwners(string payload, string summary)
	{
		if (!Replication.IsServer())
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		int delivered;
		foreach (int playerId : playerIds)
		{
			HST_CommandMenuRequestComponent request = ResolvePlayerRequestBridge(playerManager, playerId);
			if (!request)
				continue;

			request.DeliverNotificationOwner(payload, summary);
			delivered++;
		}

		return delivered;
	}

	protected static int BroadcastMissionIntelToConnectedOwners(string payload)
	{
		if (!Replication.IsServer())
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		int delivered;
		foreach (int playerId : playerIds)
		{
			HST_CommandMenuRequestComponent request = ResolvePlayerRequestBridge(playerManager, playerId);
			if (!request)
				continue;

			request.DeliverMissionIntel(payload);
			delivered++;
		}

		return delivered;
	}

	protected static HST_CommandMenuRequestComponent ResolvePlayerRequestBridge(PlayerManager playerManager, int playerId)
	{
		if (!playerManager || playerId <= 0)
			return null;

		PlayerController controller = playerManager.GetPlayerController(playerId);
		if (!controller)
			return null;

		return HST_CommandMenuRequestComponent.Cast(controller.FindComponent(HST_CommandMenuRequestComponent));
	}

	static void BroadcastSetupRefresh()
	{
		if (!s_ServerBroadcaster)
			return;

		s_ServerBroadcaster.BroadcastSetupRefresh_I();
	}

	static bool SendCampaignDebugTeleportOwner(int playerId, vector position, string reason)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		PlayerController controller = playerManager.GetPlayerController(playerId);
		if (!controller)
		{
			Print(string.Format("h-istasi campaign debug teleport | owner RPC unavailable: player controller missing player=%1 reason=%2 target=%3", playerId, reason, position), LogLevel.WARNING);
			return false;
		}

		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.Cast(controller.FindComponent(HST_CommandMenuRequestComponent));
		if (!request)
		{
			Print(string.Format("h-istasi campaign debug teleport | owner RPC unavailable: request bridge missing player=%1 reason=%2 target=%3", playerId, reason, position), LogLevel.WARNING);
			return false;
		}

		request.DeliverCampaignDebugTeleport(position, reason);
		return true;
	}

	static bool SendCampaignDebugCommandMenuProofOwner(int playerId, string requestId, string selectedTabId)
	{
		if (!Replication.IsServer() || playerId <= 0 || requestId.IsEmpty())
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		PlayerController controller = playerManager.GetPlayerController(playerId);
		if (!controller)
		{
			Print(string.Format("h-istasi campaign debug command menu | owner RPC unavailable: player controller missing player=%1 request=%2", playerId, requestId), LogLevel.WARNING);
			return false;
		}

		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.Cast(controller.FindComponent(HST_CommandMenuRequestComponent));
		if (!request)
		{
			Print(string.Format("h-istasi campaign debug command menu | owner RPC unavailable: request bridge missing player=%1 request=%2", playerId, requestId), LogLevel.WARNING);
			return false;
		}

		request.DeliverCampaignDebugCommandMenuProof(requestId, selectedTabId);
		return true;
	}

	static bool SendCampaignDebugMapProofOwner(int playerId, string requestId)
	{
		if (!Replication.IsServer() || playerId <= 0 || requestId.IsEmpty())
			return false;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		PlayerController controller = playerManager.GetPlayerController(playerId);
		if (!controller)
		{
			Print(string.Format("h-istasi campaign debug map | owner RPC unavailable: player controller missing player=%1 request=%2", playerId, requestId), LogLevel.WARNING);
			return false;
		}

		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.Cast(controller.FindComponent(HST_CommandMenuRequestComponent));
		if (!request)
		{
			Print(string.Format("h-istasi campaign debug map | owner RPC unavailable: request bridge missing player=%1 request=%2", playerId, requestId), LogLevel.WARNING);
			return false;
		}

		request.DeliverCampaignDebugMapProof(requestId);
		return true;
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
			return ResolveNativeLocalPlayerId();

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

		return ResolveNativeLocalPlayerId();
	}

	void RequestSnapshot(string selectedTabId, string lastResult = "")
	{
		int clientPlayerId = ResolveLocalPlayerId();
		if (Replication.IsServer())
		{
			SendSnapshotToOwner(selectedTabId, lastResult, clientPlayerId);
			return;
		}

		Rpc(RpcAsk_RequestSnapshot, selectedTabId, lastResult, clientPlayerId);
	}

	void RequestRuntimeFeatureSettings()
	{
		RequestRuntimeFeatureSettingsNow("manual request");
	}

	void RequestAction(string selectedTabId, string commandId, string argument = "")
	{
		int clientPlayerId = ResolveLocalPlayerId();
		if (Replication.IsServer())
		{
			SendActionResultToOwner(selectedTabId, commandId, argument, clientPlayerId);
			return;
		}

		Rpc(RpcAsk_RequestAction, selectedTabId, commandId, argument, clientPlayerId);
	}

	void RequestLoadoutEditorAction(string commandId, string argument = "")
	{
		int clientPlayerId = ResolveLocalPlayerId();
		if (Replication.IsServer())
		{
			SendLoadoutEditorActionResultToOwner(commandId, argument, clientPlayerId);
			return;
		}

		Rpc(RpcAsk_RequestLoadoutEditorAction, commandId, argument, clientPlayerId);
	}

	void RequestMissionIntel()
	{
		int clientPlayerId = ResolveLocalPlayerId();
		if (Replication.IsServer())
		{
			SendMissionIntelToOwner(clientPlayerId);
			return;
		}

		Rpc(RpcAsk_RequestMissionIntel, clientPlayerId);
	}

	void RequestSetupState(string lastResult = "")
	{
		int clientPlayerId = ResolveLocalPlayerId();
		if (Replication.IsServer())
		{
			SendSetupStateToOwner(lastResult, clientPlayerId);
			return;
		}

		Rpc(RpcAsk_RequestSetupState, lastResult, clientPlayerId);
	}

	void RequestSetupValidatePosition(float worldX, float worldZ)
	{
		int clientPlayerId = ResolveLocalPlayerId();
		if (Replication.IsServer())
		{
			SendSetupValidateResultToOwner(worldX, worldZ, clientPlayerId);
			return;
		}

		Rpc(RpcAsk_RequestSetupValidatePosition, worldX, worldZ, clientPlayerId);
	}

	void RequestSetupConfirmPosition(float worldX, float worldZ)
	{
		int clientPlayerId = ResolveLocalPlayerId();
		if (Replication.IsServer())
		{
			SendSetupConfirmResultToOwner(worldX, worldZ, clientPlayerId);
			return;
		}

		Rpc(RpcAsk_RequestSetupConfirmPosition, worldX, worldZ, clientPlayerId);
	}

	void ReportCampaignDebugCommandMenuProof(string requestId, string report)
	{
		int clientPlayerId = ResolveLocalPlayerId();
		if (Replication.IsServer())
		{
			ReceiveCampaignDebugCommandMenuProofReport(requestId, report, clientPlayerId);
			return;
		}

		Rpc(RpcAsk_ReportCampaignDebugCommandMenuProof, requestId, report, clientPlayerId);
	}

	void ReportCampaignDebugMapProof(string requestId, string report)
	{
		int clientPlayerId = ResolveLocalPlayerId();
		if (Replication.IsServer())
		{
			ReceiveCampaignDebugMapProofReport(requestId, report, clientPlayerId);
			return;
		}

		Rpc(RpcAsk_ReportCampaignDebugMapProof, requestId, report, clientPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestSnapshot(string selectedTabId, string lastResult, int clientPlayerId)
	{
		SendSnapshotToOwner(selectedTabId, lastResult, clientPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestRuntimeFeatureSettings(int clientPlayerId)
	{
		SendRuntimeFeatureSettingsToOwner(clientPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAction(string selectedTabId, string commandId, string argument, int clientPlayerId)
	{
		SendActionResultToOwner(selectedTabId, commandId, argument, clientPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestLoadoutEditorAction(string commandId, string argument, int clientPlayerId)
	{
		SendLoadoutEditorActionResultToOwner(commandId, argument, clientPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestMissionIntel(int clientPlayerId)
	{
		SendMissionIntelToOwner(clientPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestSetupState(string lastResult, int clientPlayerId)
	{
		SendSetupStateToOwner(lastResult, clientPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestSetupValidatePosition(float worldX, float worldZ, int clientPlayerId)
	{
		SendSetupValidateResultToOwner(worldX, worldZ, clientPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestSetupConfirmPosition(float worldX, float worldZ, int clientPlayerId)
	{
		SendSetupConfirmResultToOwner(worldX, worldZ, clientPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ReportCampaignDebugCommandMenuProof(string requestId, string report, int clientPlayerId)
	{
		ReceiveCampaignDebugCommandMenuProofReport(requestId, report, clientPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_ReportCampaignDebugMapProof(string requestId, string report, int clientPlayerId)
	{
		ReceiveCampaignDebugMapProofReport(requestId, report, clientPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveSnapshot(string payload, string lastResult)
	{
		m_sLastSnapshot = payload;
		m_sLastResult = lastResult;
		ApplyDebugLoggingFromMenuPayload(payload);
		ApplyRuntimeFeatureSettingsFromMenuPayload(payload);

		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (menu)
			menu.OnServerSnapshot(payload, lastResult);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveRuntimeFeatureSettings(bool infiniteStaminaEnabled, string source)
	{
		ApplyInfiniteStaminaSetting(infiniteStaminaEnabled, source);
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

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveMissionEventOwner(string payload, string summary)
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
	protected void RpcDo_ReceiveNotificationOwner(string payload, string summary)
	{
		DeliverNotificationLocal(payload, summary);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_PetrosRelocationStateOwner(bool active)
	{
		s_bLocalPetrosRelocationActive = active;
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

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_CampaignDebugTeleport(vector position, string reason)
	{
		int localPlayerId = ResolveLocalPlayerId();
		IEntity playerEntity = ResolveLocalControlledEntity(localPlayerId);
		bool nativeTeleported = false;
		bool forcedEntityOrigin = false;
		bool confirmed = false;
		if (localPlayerId > 0)
			nativeTeleported = SCR_Global.TeleportPlayer(localPlayerId, position, SCR_EPlayerTeleportedReason.DEFAULT);

		playerEntity = ResolveLocalControlledEntity(localPlayerId);
		if (playerEntity)
		{
			float confirmRadiusSq = 15.0 * 15.0;
			confirmed = DistanceSq2D(playerEntity.GetOrigin(), position) <= confirmRadiusSq;
			if (!confirmed)
			{
				playerEntity.SetOrigin(position);
				forcedEntityOrigin = true;
				confirmed = DistanceSq2D(playerEntity.GetOrigin(), position) <= confirmRadiusSq;
			}
		}

		string actual = "missing";
		if (playerEntity)
			actual = string.Format("pos %1", playerEntity.GetOrigin());

		Print(string.Format("h-istasi campaign debug teleport owner | reason %1 | player %2 | target %3 | native %4 | forced %5 | confirmed %6 | actual %7", reason, localPlayerId, position, nativeTeleported, forcedEntityOrigin, confirmed, actual));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_CampaignDebugCommandMenuProof(string requestId, string selectedTabId)
	{
		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (!menu)
		{
			ReportCampaignDebugCommandMenuProof(requestId, string.Format("request %1 | player %2 | menu component missing", requestId, ResolveLocalPlayerId()));
			return;
		}

		menu.RunCampaignDebugRenderedProof(requestId, selectedTabId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_CampaignDebugMapProof(string requestId)
	{
		RunCampaignDebugMapProof(requestId);
	}

	protected void SendSnapshotToOwner(string selectedTabId, string lastResult, int clientPlayerId = 0)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSnapshot("HST_MENU|offline|0\nSTATUS|h-istasi menu | coordinator not ready\nEND", lastResult);
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity, clientPlayerId, "snapshot");
		DebugLog(string.Format("snapshot request | selected=%1 ownerPlayer=%2 clientHint=%3", selectedTabId, playerId, clientPlayerId));
		string payload = coordinator.BuildVisibleMenuPayload(playerId, selectedTabId, lastResult);
		DeliverSnapshot(payload, lastResult);
	}

	protected void SendRuntimeFeatureSettingsToOwner(int clientPlayerId = 0)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
			return;

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity, clientPlayerId, "runtime feature settings");
		DeliverRuntimeFeatureSettings(coordinator.IsInfiniteStaminaEnabled(), string.Format("server settings player %1", playerId));
	}

	protected void BroadcastMissionEvent_I(string payload, string summary)
	{
		Rpc(RpcDo_ReceiveMissionEvent, payload, summary);
	}

	protected void DeliverMissionEventOwner(string payload, string summary)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_ReceiveMissionEventOwner(payload, summary);
			return;
		}

		Rpc(RpcDo_ReceiveMissionEventOwner, payload, summary);
	}

	protected void BroadcastNotification_I(string payload, string summary)
	{
		DeliverNotificationLocal(payload, summary);
		Rpc(RpcDo_ReceiveNotification, payload, summary);
	}

	protected void DeliverNotificationOwner(string payload, string summary)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_ReceiveNotificationOwner(payload, summary);
			return;
		}

		Rpc(RpcDo_ReceiveNotificationOwner, payload, summary);
	}

	protected void DeliverPetrosRelocationState(bool active)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_PetrosRelocationStateOwner(active);
			return;
		}

		Rpc(RpcDo_PetrosRelocationStateOwner, active);
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

	protected void SendActionResultToOwner(string selectedTabId, string commandId, string argument, int clientPlayerId = 0)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSnapshot("HST_MENU|offline|0\nSTATUS|h-istasi command | coordinator not ready\nEND", "coordinator not ready");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity, clientPlayerId, "action");
		DebugLog(string.Format("action request | command=%1 ownerPlayer=%2 clientHint=%3", commandId, playerId, clientPlayerId));
		string result = coordinator.RequestVisibleMenuCommand(playerId, selectedTabId, commandId, argument);
		string payload = coordinator.BuildVisibleMenuPayload(playerId, selectedTabId, result);
		DeliverSnapshot(payload, result);
	}

	protected void SendLoadoutEditorActionResultToOwner(string commandId, string argument, int clientPlayerId = 0)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverLoadoutEditorPayload("HST_LOADOUT_EDITOR|offline||false|0|0|0|0\nPREVIEW|false|0 0 0|0|coordinator not ready\nEND", "coordinator not ready");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity, clientPlayerId, "loadout");
		string result = coordinator.RequestLoadoutEditorCommand(playerId, commandId, argument);
		string payload;
		if (commandId == "loadout_editor_candidates")
			payload = coordinator.BuildLoadoutEditorCandidatePayload(playerId, argument);
		else
			payload = coordinator.BuildLoadoutEditorPayload(playerId);
		DeliverLoadoutEditorPayload(payload, result);
	}

	protected void SendMissionIntelToOwner(int clientPlayerId = 0)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverMissionIntel("HST_MISSION_INTEL|offline|0\nEND");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity, clientPlayerId, "mission intel");
		DeliverMissionIntel(coordinator.BuildMissionIntelPayload(playerId));
	}

	protected void SendSetupStateToOwner(string lastResult = "", int clientPlayerId = 0)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSetupState("HST_SETUP|offline|0|0|0|0|12800|12800|coordinator not ready|0\nEND", lastResult);
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity, clientPlayerId, "setup state");
		DeliverSetupState(coordinator.BuildSetupMapPayload(playerId, lastResult), lastResult);
	}

	protected void SendSetupValidateResultToOwner(float worldX, float worldZ, int clientPlayerId = 0)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSetupResult("HST_SETUP_RESULT|validate|0|0|0|0|coordinator not ready");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity, clientPlayerId, "setup validate");
		string result = coordinator.RequestSetupValidateHQPosition(playerId, worldX, worldZ);
		DeliverSetupResult(result);
	}

	protected void SendSetupConfirmResultToOwner(float worldX, float worldZ, int clientPlayerId = 0)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSetupResult("HST_SETUP_RESULT|confirm|0|0|0|0|coordinator not ready");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity, clientPlayerId, "setup confirm");
		string result = coordinator.RequestSetupConfirmHQPosition(playerId, worldX, worldZ);
		DeliverSetupResult(result);
		DeliverSetupState(coordinator.BuildSetupMapPayload(playerId, result), result);
	}

	protected void DeliverSnapshot(string payload, string lastResult)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_ReceiveSnapshot(payload, lastResult);
			return;
		}

		Rpc(RpcDo_ReceiveSnapshot, payload, lastResult);
	}

	protected void DeliverRuntimeFeatureSettings(bool infiniteStaminaEnabled, string source)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_ReceiveRuntimeFeatureSettings(infiniteStaminaEnabled, source);
			return;
		}

		Rpc(RpcDo_ReceiveRuntimeFeatureSettings, infiniteStaminaEnabled, source);
	}

	protected void DeliverLoadoutEditorPayload(string payload, string lastResult)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_ReceiveLoadoutEditorPayload(payload, lastResult);
			return;
		}

		Rpc(RpcDo_ReceiveLoadoutEditorPayload, payload, lastResult);
	}

	protected void DeliverMissionIntel(string payload)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_ReceiveMissionIntelOwner(payload);
			return;
		}

		Rpc(RpcDo_ReceiveMissionIntelOwner, payload);
	}

	protected void DeliverSetupState(string payload, string lastResult)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_ReceiveSetupState(payload, lastResult);
			return;
		}

		Rpc(RpcDo_ReceiveSetupState, payload, lastResult);
	}

	protected void DeliverSetupResult(string payload)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_ReceiveSetupResult(payload);
			return;
		}

		Rpc(RpcDo_ReceiveSetupResult, payload);
	}

	protected void DeliverCampaignDebugTeleport(vector position, string reason)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_CampaignDebugTeleport(position, reason);
			return;
		}

		Rpc(RpcDo_CampaignDebugTeleport, position, reason);
	}

	protected void DeliverCampaignDebugCommandMenuProof(string requestId, string selectedTabId)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_CampaignDebugCommandMenuProof(requestId, selectedTabId);
			return;
		}

		Rpc(RpcDo_CampaignDebugCommandMenuProof, requestId, selectedTabId);
	}

	protected void DeliverCampaignDebugMapProof(string requestId)
	{
		if (Replication.IsServer() && IsLocalOwner(m_OwnerEntity))
		{
			RpcDo_CampaignDebugMapProof(requestId);
			return;
		}

		Rpc(RpcDo_CampaignDebugMapProof, requestId);
	}

	protected void ReceiveCampaignDebugCommandMenuProofReport(string requestId, string report, int clientPlayerId = 0)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
			return;

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity, clientPlayerId, "command menu rendered proof");
		coordinator.ReceiveCampaignDebugCommandMenuProofReport(playerId, requestId, report);
	}

	protected void ReceiveCampaignDebugMapProofReport(string requestId, string report, int clientPlayerId = 0)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
			return;

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity, clientPlayerId, "map marker rendered proof");
		coordinator.ReceiveCampaignDebugMapProofReport(playerId, requestId, report);
	}

	protected void RunCampaignDebugMapProof(string requestId)
	{
		if (requestId.IsEmpty())
			return;

		GetGame().GetCallqueue().CallLater(DispatchCampaignDebugMapProofReport, 120, false, requestId, 0);
		GetGame().GetCallqueue().CallLater(DispatchCampaignDebugMapProofReport, 650, false, requestId, 1);
	}

	protected void DispatchCampaignDebugMapProofReport(string requestId, int passIndex)
	{
		string report = BuildCampaignDebugMapProofReport(requestId, passIndex);
		ReportCampaignDebugMapProof(requestId, report);
		Print("h-istasi map | campaign debug rendered proof | " + report);
	}

	protected string BuildCampaignDebugMapProofReport(string requestId, int passIndex)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		bool mapOpen = mapEntity && mapEntity.IsOpen();
		Widget rootWidget;
		if (mapEntity)
			rootWidget = mapEntity.GetMapMenuRoot();
		bool rootVisible = rootWidget && rootWidget.IsVisibleInHierarchy();
		bool hasFrame = HasOpenMapFrame(mapEntity);
		bool markerUI = mapEntity && mapEntity.GetMapUIComponent(SCR_MapMarkersUI) != null;
		bool toolMenuUI = mapEntity && mapEntity.GetMapUIComponent(SCR_MapToolMenuUI) != null;
		bool toolInteractionUI = mapEntity && mapEntity.GetMapUIComponent(SCR_MapToolInteractionUI) != null;
		bool toolMenuWidget = HasMapWidget(mapEntity, "ToolMenu");
		bool toolMenuBarWidget = HasMapWidget(mapEntity, "ToolMenuBar");
		int staticMarkers;
		int dynamicMarkers;
		int playerMarkers;
		int readyPlayerMarkers;
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager)
		{
			staticMarkers = markerManager.GetStaticMarkers().Count();
			dynamicMarkers = markerManager.GetDynamicMarkers().Count();
			if (mapOpen)
				RefreshHSTPlayerMarkerWidgets();
			foreach (SCR_MapMarkerEntity marker : markerManager.GetDynamicMarkers())
			{
				if (!marker || marker.GetType() != SCR_EMapMarkerType.HST_PLAYER)
					continue;

				playerMarkers++;
				if (mapOpen && marker.EnsureHSTPlayerMarkerWidget())
					readyPlayerMarkers++;
			}
		}

		bool markersReady = markerManager != null && (staticMarkers + dynamicMarkers) > 0;
		bool playerReady = playerMarkers > 0 && readyPlayerMarkers >= playerMarkers;
		string rootSummary = "none";
		if (rootWidget)
			rootSummary = HST_UIDebug.WidgetSummary(rootWidget);

		string report = string.Format("request %1 | player %2 | pass %3 | mapOpen %4 | root %5 | frame %6 | markerUI %7 | markersReady %8 | playerReady %9", requestId, ResolveLocalPlayerId(), passIndex, mapOpen, rootVisible, hasFrame, markerUI, markersReady, playerReady);
		report = report + string.Format(" | static %1 | dynamic %2 | playerMarkers %3/%4", staticMarkers, dynamicMarkers, readyPlayerMarkers, playerMarkers);
		report = report + string.Format(" | toolMenuUI %1 | toolInteractionUI %2 | toolMenuWidget %3 | toolMenuBarWidget %4", toolMenuUI, toolInteractionUI, toolMenuWidget, toolMenuBarWidget);
		report = report + " | rootSummary " + ShortenText(rootSummary, 140);
		return report;
	}

	protected string ShortenText(string text, int maxCharacters)
	{
		if (text.IsEmpty() || maxCharacters <= 0)
			return "";
		if (text.Length() <= maxCharacters)
			return text;
		if (maxCharacters <= 3)
			return text.Substring(0, maxCharacters);

		return text.Substring(0, maxCharacters - 3) + "...";
	}

	protected void TickRuntimeFeatureSettingsRequest(float timeSlice)
	{
		if (m_bRuntimeFeatureSettingsSynced)
			return;

		m_fRuntimeFeatureSettingsRetryAccumulator += timeSlice;
		if (m_fRuntimeFeatureSettingsRetryAccumulator < RUNTIME_FEATURE_SETTINGS_RETRY_SECONDS)
			return;

		m_fRuntimeFeatureSettingsRetryAccumulator = 0;
		RequestRuntimeFeatureSettingsNow("retry");
	}

	protected void RequestRuntimeFeatureSettingsNow(string reason)
	{
		if (!m_bIsLocalOwner)
			return;

		int clientPlayerId = ResolveLocalPlayerId();
		if (Replication.IsServer())
		{
			SendRuntimeFeatureSettingsToOwner(clientPlayerId);
			return;
		}

		Rpc(RpcAsk_RequestRuntimeFeatureSettings, clientPlayerId);
		DebugLog(string.Format("runtime feature settings requested | reason %1 | player %2", reason, clientPlayerId));
	}

	protected void ApplyInfiniteStaminaSetting(bool enabled, string source)
	{
		bool changed = !m_bRuntimeFeatureSettingsSynced || m_bInfiniteStaminaEnabled != enabled;
		m_bRuntimeFeatureSettingsSynced = true;
		m_bInfiniteStaminaEnabled = enabled;
		m_fRuntimeFeatureSettingsRetryAccumulator = 0;
		SCR_StaminaBlurEffect.SetHistasiInfiniteStaminaVisualSuppressed(enabled);
		if (changed)
			Print(string.Format("h-istasi stamina | owner setting sync | infinite stamina %1 | sprint vignette suppressed %2 | source %3 | player %4", enabled, enabled, source, ResolveLocalPlayerId()));
	}

	protected void TickInfiniteStamina(float timeSlice)
	{
		SCR_StaminaBlurEffect.SetHistasiInfiniteStaminaVisualSuppressed(m_bRuntimeFeatureSettingsSynced && m_bInfiniteStaminaEnabled);
		if (!m_bRuntimeFeatureSettingsSynced || !m_bInfiniteStaminaEnabled)
			return;

		m_fInfiniteStaminaAccumulator += timeSlice;
		if (m_fInfiniteStaminaAccumulator < INFINITE_STAMINA_REFILL_INTERVAL_SECONDS)
			return;

		m_fInfiniteStaminaAccumulator = 0;
		int localPlayerId = ResolveLocalPlayerId();
		IEntity playerEntity = ResolveLocalControlledEntity(localPlayerId);
		if (!playerEntity)
			return;

		CharacterStaminaComponent stamina = CharacterStaminaComponent.Cast(playerEntity.FindComponent(CharacterStaminaComponent));
		if (!stamina)
			return;

		float currentStamina = stamina.GetStamina();
		if (currentStamina >= INFINITE_STAMINA_TARGET)
			return;

		stamina.AddStamina(1.0 - currentStamina);
		m_iInfiniteStaminaRefillCount++;
		if (m_bDebugLoggingEnabled && (m_iInfiniteStaminaRefillCount <= 3 || (m_iInfiniteStaminaRefillCount % 50) == 0))
			DebugLog(string.Format("infinite stamina refill | player %1 | before %2 | count %3", localPlayerId, currentStamina, m_iInfiniteStaminaRefillCount));
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

		IEntity localControlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (localControlledEntity && owner == localControlledEntity)
			return true;

		PlayerController localController = GetGame().GetPlayerController();
		PlayerController ownerController = PlayerController.Cast(owner);
		if (localController)
		{
			if (ownerController && ownerController == localController)
				return true;

			HST_CommandMenuRequestComponent localComponent = HST_CommandMenuRequestComponent.Cast(localController.FindComponent(HST_CommandMenuRequestComponent));
			if (localComponent == this)
				return true;
		}

		int localPlayerId = ResolveNativeLocalPlayerId();
		if (localPlayerId <= 0)
			return false;

		if (ownerController && ownerController.GetPlayerId() == localPlayerId)
			return true;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		int ownerPlayerId = playerManager.GetPlayerIdFromControlledEntity(owner);
		if (ownerPlayerId == localPlayerId)
			return true;

		BaseRplComponent rpl = BaseRplComponent.Cast(owner.FindComponent(BaseRplComponent));
		return rpl && playerManager.GetPlayerIdFromEntityRplId(rpl.Id()) == localPlayerId;
	}

	protected int ResolveNativeLocalPlayerId()
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId > 0)
			return localPlayerId;

		PlayerController localController = GetGame().GetPlayerController();
		if (localController && localController.GetPlayerId() > 0)
			return localController.GetPlayerId();

		IEntity controlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (!controlledEntity)
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		return playerManager.GetPlayerIdFromControlledEntity(controlledEntity);
	}

	protected IEntity ResolveLocalControlledEntity(int localPlayerId)
	{
		IEntity controlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (controlledEntity)
			return controlledEntity;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || localPlayerId <= 0)
			return null;

		return playerManager.GetPlayerControlledEntity(localPlayerId);
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return dx * dx + dz * dz;
	}
}
