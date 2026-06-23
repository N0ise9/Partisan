class HST_SetupMapWidgetHandler : ScriptedWidgetEventHandler
{
	protected HST_SetupMapComponent m_Component;

	void Bind(HST_SetupMapComponent component)
	{
		m_Component = component;
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (!m_Component)
			return true;

		return m_Component.OnSetupOverlayClicked(w.GetUserID());
	}
}

class HST_SetupMapDrawCommandSet
{
	ref array<ref CanvasWidgetCommand> m_aCommands = {};
}

[ComponentEditorProps(category: "h-istasi", description: "Client setup tactical map for initial HQ placement")]
class HST_SetupMapComponentClass : ScriptComponentClass
{
}

class HST_SetupMapComponent : ScriptComponent
{
	static const int CONFIRM_YES_WIDGET_ID = 71002;
	static const int CONFIRM_NO_WIDGET_ID = 71003;
	static const int CONFIRM_BLOCKER_WIDGET_ID = 71004;
	static const int SETUP_Z_ORDER = 50000;
	static const int SETUP_OVERLAY_Z_ORDER = 51000;
	static const int SETUP_MODAL_Z_ORDER = 52000;
	static const float SETUP_STATE_REQUEST_INTERVAL_SECONDS = 2.5;
	static const float SETUP_SERVER_REQUEST_TIMEOUT_SECONDS = 5.0;
	static const float SETUP_VALIDATION_RESULT_TOLERANCE_METERS = 8.0;
	static const ResourceName SETUP_NATIVE_MAP_LAYOUT = "{6985327711306200}UI/layouts/HST_SetupHQMap.layout";
	static const ResourceName SETUP_NATIVE_MAP_CONFIG = "{6985327711306210}Configs/Map/HST_SetupHQMap.conf";
	static const ResourceName MENU_FONT = "";
	static const string SETUP_INPUT_CONTEXT = "InGameMenuContext";
	static const string SETUP_CURSOR_CONTEXT = "InventoryContext";

	protected static HST_SetupMapComponent s_LocalInstance;

	protected IEntity m_OwnerEntity;
	protected bool m_bIsLocalOwner;
	protected bool m_bSetupActive;
	protected bool m_bIsCommander;
	protected bool m_bConfirmOpen;
	protected bool m_bCandidateValid;
	protected bool m_bAwaitingServer;
	protected bool m_bDebugLoggingEnabled;
	protected bool m_bHasAuthoritativeSetupState;
	protected bool m_bResolvedLocalPlayerId;
	protected bool m_bNativeMapOpen;
	protected bool m_bNativeInvokersBound;
	protected bool m_bOverlayDirty = true;
	protected string m_sPhase = "setup";
	protected string m_sStatusText = "Waiting for setup state...";
	protected string m_sLastResult;
	protected vector m_vWorldMin = "0 0 0";
	protected vector m_vWorldMax = "12800 0 12800";
	protected vector m_vCandidatePosition = "0 0 0";
	protected vector m_vRequestedValidationPosition = "0 0 0";
	protected ref array<string> m_aZoneIds = {};
	protected ref array<string> m_aZoneLabels = {};
	protected ref array<string> m_aZoneTypes = {};
	protected ref array<string> m_aZoneOwners = {};
	protected ref array<float> m_aZoneXs = {};
	protected ref array<float> m_aZoneZs = {};
	protected ref array<float> m_aZoneRadii = {};
	protected ref array<string> m_aZoneTones = {};
	protected ref array<Widget> m_aWidgets = {};
	protected ref array<Widget> m_aOverlayWidgets = {};
	protected ref array<Widget> m_aModalWidgets = {};
	protected ref array<ref HST_SetupMapDrawCommandSet> m_aChromeDrawCommandSets = {};
	protected ref array<ref HST_SetupMapDrawCommandSet> m_aOverlayDrawCommandSets = {};
	protected ref array<ref HST_SetupMapDrawCommandSet> m_aModalDrawCommandSets = {};
	protected ref HST_SetupMapWidgetHandler m_WidgetHandler;
	protected SCR_MapEntity m_MapEntity;
	protected Widget m_wSetupRoot;
	protected Widget m_wOverlayRoot;
	protected TextWidget m_wPromptText;
	protected float m_fOwnerRetryAccumulator;
	protected float m_fRequestAccumulator;
	protected float m_fAwaitingServerAccumulator;
	protected float m_fDebugHeartbeatAccumulator;
	protected int m_iScreenW;
	protected int m_iScreenH;
	protected int m_iSetupStateRequestCount;
	protected int m_iSetupPayloadCount;
	protected int m_iSetupResultCount;
	protected int m_iOverlayRenderCount;
	protected bool m_bLoggedLocalReady;
	protected bool m_bLoggedBridgeMissing;
	protected bool m_bLoggedBridgeRecovered;
	protected float m_fScale = 1.0;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		m_bIsLocalOwner = IsLocalOwner(owner);
		m_WidgetHandler = new HST_SetupMapWidgetHandler();
		m_WidgetHandler.Bind(this);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);

		if (m_bIsLocalOwner)
		{
			s_LocalInstance = this;
			LogLocalReady();
			ActivatePendingSetupOverlay("Waiting for setup state...");
			RequestSetupStateNow();
		}
	}

	override void OnDelete(IEntity owner)
	{
		if (s_LocalInstance == this)
			s_LocalInstance = null;

		CloseNativeSetupMap();
		super.OnDelete(owner);
	}

	override void EOnInit(IEntity owner)
	{
		RefreshLocalOwner(owner);
		if (m_bIsLocalOwner)
		{
			ActivatePendingSetupOverlay("Waiting for setup state...");
			RequestSetupStateNow();
		}
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bIsLocalOwner)
		{
			m_fOwnerRetryAccumulator += timeSlice;
			if (m_fOwnerRetryAccumulator >= 0.25)
			{
				m_fOwnerRetryAccumulator = 0;
				RefreshLocalOwner(owner);
			}

			return;
		}

		m_fRequestAccumulator += timeSlice;
		if (m_fRequestAccumulator >= SETUP_STATE_REQUEST_INTERVAL_SECONDS)
		{
			m_fRequestAccumulator = 0;
			RequestSetupStateNow();
		}

		if (!m_bSetupActive)
			return;

		RefreshResolvedLocalPlayerId();
		ActivateSetupInput();
		ClearBlockedSetupKeys();
		SCR_RespawnSystemComponent.CloseRespawnMenu();
		CloseCommandMenuIfOpen();
		TickServerRequestTimeout(timeSlice);
		RefreshScreenMetrics();
		EnsureNativeSetupMapOpen();
		UpdateSetupPrompt();
		UpdateConfirmationVisibility();
		if (m_bOverlayDirty)
		{
			m_bOverlayDirty = false;
			RedrawNativeMapOverlay();
		}

		DebugHeartbeat(timeSlice);
	}

	static HST_SetupMapComponent GetLocalInstance()
	{
		return s_LocalInstance;
	}

	static bool IsSetupBlocking()
	{
		return s_LocalInstance && s_LocalInstance.m_bSetupActive;
	}

	void RequestSetupStateNow()
	{
		if (!m_bIsLocalOwner)
			return;

		m_iSetupStateRequestCount++;
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (!request)
		{
			m_sStatusText = "Waiting for setup request bridge...";
			m_bOverlayDirty = true;
			if (!m_bLoggedBridgeMissing)
			{
				m_bLoggedBridgeMissing = true;
				Print("h-istasi setup ui | request bridge not ready; setup shield is visible");
			}

			return;
		}

		if (m_bLoggedBridgeMissing && !m_bLoggedBridgeRecovered)
		{
			m_bLoggedBridgeRecovered = true;
			Print("h-istasi setup ui | request bridge recovered");
		}

		request.RequestSetupState(m_sLastResult);
	}

	void OnServerSetupState(string payload, string lastResult = "")
	{
		m_iSetupPayloadCount++;
		m_bHasAuthoritativeSetupState = true;
		m_sLastResult = lastResult;
		ParseSetupStatePayload(payload);
		DebugLog(string.Format("state payload #%1 phase=%2 active=%3 commander=%4 zones=%5 status=%6", m_iSetupPayloadCount, m_sPhase, m_bSetupActive, m_bIsCommander, m_aZoneIds.Count(), ShortenText(m_sStatusText, 96)));
		if (!m_bSetupActive)
		{
			CloseNativeSetupMap();
			DebugLog("setup inactive; native setup map closed");
			return;
		}

		m_bOverlayDirty = true;
	}

	void OnServerSetupResult(string payload)
	{
		if (payload.IsEmpty() || !payload.Contains("HST_SETUP_RESULT|"))
			return;

		m_iSetupResultCount++;
		string action = ExtractPipeField(payload, 1);
		bool accepted = ParseBool(ExtractPipeField(payload, 2));
		vector resolved = "0 0 0";
		resolved[0] = ExtractPipeField(payload, 3).ToFloat();
		resolved[1] = ExtractPipeField(payload, 4).ToFloat();
		resolved[2] = ExtractPipeField(payload, 5).ToFloat();
		string message = ExtractPipeField(payload, 6);
		if (action == "validate" && !m_bAwaitingServer && !IsCoordinatorSetupFailure(message))
		{
			DebugLog(string.Format("ignored unexpected validate result resolved=%1 message=%2", resolved, ShortenText(message, 96)));
			return;
		}

		if (action == "validate" && !IsLatestValidationResult(resolved) && !IsCoordinatorSetupFailure(message))
		{
			DebugLog(string.Format("ignored stale validate result resolved=%1 latest=%2 message=%3", resolved, m_vRequestedValidationPosition, ShortenText(message, 96)));
			return;
		}

		m_sLastResult = message;
		m_sStatusText = message;
		m_bAwaitingServer = false;
		m_fAwaitingServerAccumulator = 0;
		DebugLog(string.Format("result payload #%1 action=%2 accepted=%3 resolved=%4 message=%5", m_iSetupResultCount, action, accepted, resolved, ShortenText(message, 96)));

		if (action == "validate" && accepted)
		{
			m_vCandidatePosition = resolved;
			m_bCandidateValid = true;
			m_bConfirmOpen = true;
		}
		else if (action == "validate")
		{
			m_bCandidateValid = false;
			m_bConfirmOpen = false;
		}
		else if (action == "confirm" && accepted)
		{
			m_bConfirmOpen = false;
			RequestSetupStateNow();
		}
		else if (action == "confirm")
		{
			m_bConfirmOpen = false;
		}

		m_bOverlayDirty = true;
		UpdateConfirmationVisibility();
	}

	bool OnSetupOverlayClicked(int widgetId)
	{
		if (!m_bSetupActive)
			return true;

		if (widgetId == CONFIRM_NO_WIDGET_ID)
		{
			m_bConfirmOpen = false;
			m_sStatusText = "Select a location on the map to place the HQ";
			m_bOverlayDirty = true;
			UpdateConfirmationVisibility();
			return true;
		}

		if (widgetId == CONFIRM_YES_WIDGET_ID)
		{
			if (m_bCandidateValid && !m_bAwaitingServer)
				RequestConfirmPosition();
			return true;
		}

		return true;
	}

	protected void ParseSetupStatePayload(string payload)
	{
		ClearZonePayload();
		if (payload.IsEmpty() || !payload.Contains("HST_SETUP|"))
		{
			m_bSetupActive = false;
			return;
		}

		int lineEnd = payload.IndexOf("\n");
		if (lineEnd < 0)
			lineEnd = payload.Length();

		string header = payload.Substring(0, lineEnd);
		m_sPhase = ExtractPipeField(header, 1);
		m_bSetupActive = ParseBool(ExtractPipeField(header, 2));
		m_bIsCommander = ParseBool(ExtractPipeField(header, 3));
		m_vWorldMin[0] = ExtractPipeField(header, 4).ToFloat();
		m_vWorldMin[2] = ExtractPipeField(header, 5).ToFloat();
		m_vWorldMax[0] = ExtractPipeField(header, 6).ToFloat();
		m_vWorldMax[2] = ExtractPipeField(header, 7).ToFloat();
		m_sStatusText = ExtractPipeField(header, 8);
		m_bDebugLoggingEnabled = ParseBool(ExtractPipeField(header, 9));
		if (m_sStatusText.IsEmpty())
		{
			if (m_bIsCommander)
				m_sStatusText = "Select a location on the map to place the HQ";
			else
				m_sStatusText = "Please wait, the commander is selecting the HQ location...";
		}

		int cursor;
		while (true)
		{
			int zoneStart = payload.IndexOfFrom(cursor, "ZONE|");
			if (zoneStart < 0)
				break;

			int zoneEnd = payload.IndexOfFrom(zoneStart, "\n");
			if (zoneEnd < 0)
				zoneEnd = payload.Length();

			string line = payload.Substring(zoneStart, zoneEnd - zoneStart);
			m_aZoneIds.Insert(ExtractPipeField(line, 1));
			m_aZoneLabels.Insert(ExtractPipeField(line, 2));
			m_aZoneTypes.Insert(ExtractPipeField(line, 3));
			m_aZoneOwners.Insert(ExtractPipeField(line, 4));
			m_aZoneXs.Insert(ExtractPipeField(line, 5).ToFloat());
			m_aZoneZs.Insert(ExtractPipeField(line, 6).ToFloat());
			m_aZoneRadii.Insert(Math.Max(50.0, ExtractPipeField(line, 7).ToFloat()));
			m_aZoneTones.Insert(ExtractPipeField(line, 8));
			cursor = zoneEnd + 1;
		}
	}

	protected void EnsureNativeSetupMapOpen()
	{
		if (!m_bSetupActive)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		if (!m_wSetupRoot)
		{
			m_wSetupRoot = workspace.CreateWidgets(SETUP_NATIVE_MAP_LAYOUT);
			if (!m_wSetupRoot)
			{
				m_sStatusText = "setup map layout failed to load";
				DebugLog("native setup map layout failed to load");
				return;
			}

			m_wSetupRoot.SetZOrder(SETUP_Z_ORDER);
			m_aWidgets.Insert(m_wSetupRoot);
			m_wOverlayRoot = m_wSetupRoot.FindAnyWidget("HST_SetupOverlayRoot");
			if (!m_wOverlayRoot)
				m_wOverlayRoot = m_wSetupRoot;

			m_wPromptText = CreatePromptText(workspace, m_wOverlayRoot);
			m_bOverlayDirty = true;
		}

		if (!m_MapEntity)
			m_MapEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapEntity)
		{
			m_sStatusText = "native map entity not ready";
			DebugLog("native setup map entity not ready");
			return;
		}

		BindNativeMapInvokers();
		if (m_MapEntity.IsOpen())
		{
			if (m_MapEntity.GetMapMenuRoot() == m_wSetupRoot)
			{
				m_bNativeMapOpen = true;
				return;
			}

			m_MapEntity.CloseMap();
		}

		MapConfiguration mapConfig = m_MapEntity.SetupMapConfig(EMapEntityMode.FULLSCREEN, SETUP_NATIVE_MAP_CONFIG, m_wSetupRoot);
		if (!mapConfig)
		{
			m_sStatusText = "setup map configuration failed";
			DebugLog("native setup map configuration failed");
			return;
		}

		m_MapEntity.OpenMap(mapConfig);
		m_bNativeMapOpen = true;
		m_bOverlayDirty = true;
		DebugLog("native setup map opened");
	}

	protected void BindNativeMapInvokers()
	{
		if (m_bNativeInvokersBound)
			return;

		SCR_MapEntity.GetOnSelection().Insert(OnNativeMapSelection);
		SCR_MapEntity.GetOnMapPan().Insert(OnNativeMapPan);
		SCR_MapEntity.GetOnMapZoom().Insert(OnNativeMapZoom);
		SCR_MapEntity.GetOnMapOpenComplete().Insert(OnNativeMapOpenComplete);
		m_bNativeInvokersBound = true;
	}

	protected void CloseNativeSetupMap()
	{
		if (m_bNativeInvokersBound)
		{
			SCR_MapEntity.GetOnSelection().Remove(OnNativeMapSelection);
			SCR_MapEntity.GetOnMapPan().Remove(OnNativeMapPan);
			SCR_MapEntity.GetOnMapZoom().Remove(OnNativeMapZoom);
			SCR_MapEntity.GetOnMapOpenComplete().Remove(OnNativeMapOpenComplete);
			m_bNativeInvokersBound = false;
		}

		if (m_MapEntity && m_MapEntity.IsOpen() && (!m_wSetupRoot || m_MapEntity.GetMapMenuRoot() == m_wSetupRoot))
			m_MapEntity.CloseMap();

		m_bNativeMapOpen = false;
		ClearModalWidgets();
		ClearOverlayWidgets();
		ClearWidgets();
		m_wSetupRoot = null;
		m_wOverlayRoot = null;
		m_wPromptText = null;
	}

	protected void OnNativeMapSelection(vector screenPos)
	{
		if (!m_bSetupActive || !m_bIsCommander || m_bConfirmOpen || m_bAwaitingServer)
			return;
		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;

		float screenX = screenPos[0];
		float screenY = screenPos[2];
		if (screenY == 0.0 && screenPos[1] != 0.0)
			screenY = screenPos[1];

		float worldX;
		float worldZ;
		m_MapEntity.ScreenToWorld(Math.Round(screenX), Math.Round(screenY), worldX, worldZ);

		vector worldPosition = "0 0 0";
		worldPosition[0] = Math.Clamp(worldX, m_vWorldMin[0], m_vWorldMax[0]);
		worldPosition[2] = Math.Clamp(worldZ, m_vWorldMin[2], m_vWorldMax[2]);

		m_vCandidatePosition = worldPosition;
		m_bCandidateValid = false;
		RequestValidatePosition(worldPosition);
		m_bOverlayDirty = true;
	}

	protected void OnNativeMapPan(float panX, float panY, bool adjusted)
	{
		m_bOverlayDirty = true;
	}

	protected void OnNativeMapZoom(float zoom)
	{
		m_bOverlayDirty = true;
	}

	protected void OnNativeMapOpenComplete(MapConfiguration config)
	{
		m_bOverlayDirty = true;
	}

	protected void RequestValidatePosition(vector worldPosition)
	{
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (!request)
		{
			m_sStatusText = "setup request bridge not ready";
			m_bOverlayDirty = true;
			DebugLog("validate request aborted: request bridge missing");
			return;
		}

		m_bAwaitingServer = true;
		m_fAwaitingServerAccumulator = 0;
		m_vRequestedValidationPosition = worldPosition;
		m_sStatusText = "Checking HQ location...";
		m_bOverlayDirty = true;
		DebugLog(string.Format("validate request %1", worldPosition));
		request.RequestSetupValidatePosition(worldPosition[0], worldPosition[2]);
	}

	protected void RequestConfirmPosition()
	{
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (!request)
		{
			m_sStatusText = "setup request bridge not ready";
			m_bOverlayDirty = true;
			DebugLog("confirm request aborted: request bridge missing");
			return;
		}

		m_bAwaitingServer = true;
		m_fAwaitingServerAccumulator = 0;
		m_sStatusText = "Placing HQ...";
		m_bOverlayDirty = true;
		DebugLog(string.Format("confirm request %1", m_vCandidatePosition));
		request.RequestSetupConfirmPosition(m_vCandidatePosition[0], m_vCandidatePosition[2]);
	}

	protected void RefreshScreenMetrics()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		int screenW = Math.Max(1, Math.Round(workspace.GetWidth()));
		int screenH = Math.Max(1, Math.Round(workspace.GetHeight()));
		if (screenW != m_iScreenW || screenH != m_iScreenH)
		{
			m_iScreenW = screenW;
			m_iScreenH = screenH;
			m_bOverlayDirty = true;
			float sx = m_iScreenW / 1920.0;
			float sy = m_iScreenH / 1080.0;
			m_fScale = Math.Clamp(Math.Min(sx, sy), 0.70, 1.15);
		}
	}

	protected TextWidget CreatePromptText(WorkspaceWidget workspace, Widget parent)
	{
		CreateRectWidgetAtZ(workspace, parent, 0, 0, Math.Max(1, m_iScreenW), ScalePx(76), 0xF20D1318, 1.0, 0, SETUP_OVERLAY_Z_ORDER, m_aChromeDrawCommandSets);
		CreateRectWidgetAtZ(workspace, parent, 0, ScalePx(72), Math.Max(1, m_iScreenW), ScalePx(4), 0xFFC4953B, 1.0, 0, SETUP_OVERLAY_Z_ORDER + 1, m_aChromeDrawCommandSets);
		return CreateWrappedTextWidgetAtZ(workspace, parent, "", ScalePx(36), ScalePx(18), Math.Max(1, m_iScreenW - ScalePx(72)), ScalePx(42), ScaleFont(22), 0xFFF2E6CA, 0, true, SETUP_OVERLAY_Z_ORDER + 2);
	}

	protected void UpdateSetupPrompt()
	{
		if (!m_wPromptText)
			return;

		string prompt = m_sStatusText;
		if (m_bSetupActive && m_bHasAuthoritativeSetupState && !m_bIsCommander)
			prompt = "Please wait, the commander is selecting the HQ location...";
		else if (m_bSetupActive && m_bIsCommander)
			prompt = "Select a location on the map to place the HQ";
		else if (m_bSetupActive && m_sStatusText.IsEmpty())
			prompt = "Select a location on the map to place the HQ";

		m_wPromptText.SetText(prompt);
	}

	protected void UpdateConfirmationVisibility()
	{
		if (!m_bConfirmOpen)
		{
			ClearModalWidgets();
			return;
		}

		if (m_aModalWidgets.Count() > 0)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace || !m_wSetupRoot)
			return;

		int modalW = Math.Min(ScalePx(560), m_iScreenW - ScalePx(80));
		int modalH = ScalePx(210);
		int left = (m_iScreenW - modalW) / 2;
		int top = (m_iScreenH - modalH) / 2;
		CreateModalRect(workspace, 0, 0, m_iScreenW, m_iScreenH, 0xA8000000, 1.0, CONFIRM_BLOCKER_WIDGET_ID, SETUP_MODAL_Z_ORDER + 1);
		CreateModalRect(workspace, left - ScalePx(16), top - ScalePx(16), modalW + ScalePx(32), modalH + ScalePx(32), 0xE0000000, 1.0, CONFIRM_BLOCKER_WIDGET_ID, SETUP_MODAL_Z_ORDER + 2);
		CreateModalRect(workspace, left, top, modalW, modalH, 0xFF1D2932, 1.0, CONFIRM_BLOCKER_WIDGET_ID, SETUP_MODAL_Z_ORDER + 3);
		CreateModalRect(workspace, left, top, modalW, ScalePx(4), 0xFFC4953B, 1.0, CONFIRM_BLOCKER_WIDGET_ID, SETUP_MODAL_Z_ORDER + 4);
		CreateModalText(workspace, "Are you sure you want to place the HQ here?", left + ScalePx(34), top + ScalePx(34), modalW - ScalePx(68), ScalePx(72), ScaleFont(22), 0xFFF2E6CA, CONFIRM_BLOCKER_WIDGET_ID, true, SETUP_MODAL_Z_ORDER + 5);
		int buttonW = ScalePx(160);
		int buttonH = ScalePx(50);
		int buttonTop = top + modalH - ScalePx(74);
		int noLeft = left + modalW / 2 - buttonW - ScalePx(12);
		int yesLeft = left + modalW / 2 + ScalePx(12);
		CreateModalRect(workspace, noLeft, buttonTop, buttonW, buttonH, 0xFF46535D, 1.0, CONFIRM_NO_WIDGET_ID, SETUP_MODAL_Z_ORDER + 5);
		CreateModalText(workspace, "No", noLeft + ScalePx(20), buttonTop + ScalePx(13), buttonW - ScalePx(40), ScalePx(26), ScaleFont(17), 0xFFF4EBD6, CONFIRM_NO_WIDGET_ID, true, SETUP_MODAL_Z_ORDER + 6);
		CreateModalRect(workspace, yesLeft, buttonTop, buttonW, buttonH, 0xFFC4953B, 1.0, CONFIRM_YES_WIDGET_ID, SETUP_MODAL_Z_ORDER + 5);
		CreateModalText(workspace, "Yes", yesLeft + ScalePx(20), buttonTop + ScalePx(13), buttonW - ScalePx(40), ScalePx(26), ScaleFont(17), 0xFF111820, CONFIRM_YES_WIDGET_ID, true, SETUP_MODAL_Z_ORDER + 6);
	}

	protected void RedrawNativeMapOverlay()
	{
		ClearOverlayWidgets();
		if (!m_bSetupActive || !m_wOverlayRoot || !m_MapEntity || !m_MapEntity.IsOpen())
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		m_iOverlayRenderCount++;
		if (m_iOverlayRenderCount <= 3 || (m_iOverlayRenderCount % 20) == 0)
			DebugLog(string.Format("overlay render #%1 commander=%2 confirm=%3 awaiting=%4 zones=%5", m_iOverlayRenderCount, m_bIsCommander, m_bConfirmOpen, m_bAwaitingServer, m_aZoneIds.Count()));

		for (int i = 0; i < m_aZoneLabels.Count(); i++)
			DrawNativeZoneMarker(workspace, i);

		if (m_bCandidateValid || m_bAwaitingServer)
			DrawNativeCandidateMarker(workspace);
	}

	protected void DrawNativeZoneMarker(WorkspaceWidget workspace, int index)
	{
		vector pos = "0 0 0";
		pos[0] = m_aZoneXs[index];
		pos[2] = m_aZoneZs[index];
		int sx;
		int sy;
		if (!WorldToNativeMapScreen(pos, sx, sy))
			return;

		int radiusPx = Math.Max(4, ResolveNativeRadiusPixels(pos, m_aZoneRadii[index]));
		if (!IsScreenRectVisible(sx - radiusPx, sy - radiusPx, radiusPx * 2, radiusPx * 2))
			return;

		CreateOverlayCircle(workspace, sx - radiusPx, sy - radiusPx, radiusPx * 2, ZoneFillColor(m_aZoneTones[index]), 0.44);
		CreateOverlayRect(workspace, sx - ScalePx(3), sy - ScalePx(3), ScalePx(6), ScalePx(6), ZonePointColor(m_aZoneTones[index]), 1.0);
		if (radiusPx >= ScalePx(9))
			CreateOverlayLabel(workspace, ShortenText(m_aZoneLabels[index], 24), sx + ScalePx(6), sy - ScalePx(8), 0xFFE5ECEE);
	}

	protected void DrawNativeCandidateMarker(WorkspaceWidget workspace)
	{
		int sx;
		int sy;
		if (!WorldToNativeMapScreen(m_vCandidatePosition, sx, sy))
			return;

		int size = ScalePx(22);
		int color = 0xFFFFD166;
		if (m_bAwaitingServer)
			color = 0xFF9FD3FF;

		CreateOverlayRect(workspace, sx - size / 2, sy - ScalePx(2), size, ScalePx(4), color, 1.0);
		CreateOverlayRect(workspace, sx - ScalePx(2), sy - size / 2, ScalePx(4), size, color, 1.0);
		CreateOverlayLabel(workspace, "HQ", sx + ScalePx(10), sy + ScalePx(8), color);
	}

	protected bool WorldToNativeMapScreen(vector worldPos, out int screenX, out int screenY)
	{
		screenX = 0;
		screenY = 0;
		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return false;

		m_MapEntity.WorldToScreen(worldPos[0], worldPos[2], screenX, screenY, true);
		return true;
	}

	protected int ResolveNativeRadiusPixels(vector center, float radiusMeters)
	{
		int cx;
		int cy;
		int rx;
		int ry;
		vector edge = center;
		edge[0] = edge[0] + radiusMeters;
		if (!WorldToNativeMapScreen(center, cx, cy))
			return 0;
		if (!WorldToNativeMapScreen(edge, rx, ry))
			return 0;

		return AbsInt(rx - cx);
	}

	protected Widget CreateOverlayRect(WorkspaceWidget workspace, int left, int top, int width, int height, int color, float opacity)
	{
		Widget widget = CreateRectWidgetAtZ(workspace, m_wOverlayRoot, left, top, width, height, color, opacity, 0, SETUP_OVERLAY_Z_ORDER + 10, m_aOverlayDrawCommandSets);
		if (widget)
			m_aOverlayWidgets.Insert(widget);

		return widget;
	}

	protected Widget CreateOverlayCircle(WorkspaceWidget workspace, int left, int top, int diameter, int color, float opacity)
	{
		Widget widget = workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS, null, SETUP_OVERLAY_Z_ORDER + 10, m_wOverlayRoot);
		if (!widget)
			return null;

		widget.SetZOrder(SETUP_OVERLAY_Z_ORDER + 10);
		widget.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, diameter, diameter);
		SetupPolygonWidget(widget, BuildCircleVertices(diameter), color, m_aOverlayDrawCommandSets);
		widget.SetOpacity(opacity);
		m_aOverlayWidgets.Insert(widget);
		return widget;
	}

	protected TextWidget CreateOverlayLabel(WorkspaceWidget workspace, string text, int left, int top, int color)
	{
		TextWidget label = CreateWrappedTextWidgetAtZ(workspace, m_wOverlayRoot, text, left, top, ScalePx(160), ScalePx(32), ScaleFont(11), color, 0, false, SETUP_OVERLAY_Z_ORDER + 11);
		if (label)
			m_aOverlayWidgets.Insert(label);

		return label;
	}

	protected Widget CreateModalRect(WorkspaceWidget workspace, int left, int top, int width, int height, int color, float opacity, int userId, int zOrder)
	{
		Widget widget = CreateRectWidgetAtZ(workspace, m_wSetupRoot, left, top, width, height, color, opacity, userId, zOrder, m_aModalDrawCommandSets);
		if (widget)
			m_aModalWidgets.Insert(widget);

		return widget;
	}

	protected TextWidget CreateModalText(WorkspaceWidget workspace, string text, int left, int top, int width, int height, int fontSize, int color, int userId, bool bold, int zOrder)
	{
		TextWidget widget = CreateWrappedTextWidgetAtZ(workspace, m_wSetupRoot, text, left, top, width, height, fontSize, color, userId, bold, zOrder);
		if (widget)
			m_aModalWidgets.Insert(widget);

		return widget;
	}

	protected Widget CreateRectWidgetAtZ(WorkspaceWidget workspace, Widget parent, int left, int top, int width, int height, int color, float opacity, int userId, int zOrder, array<ref HST_SetupMapDrawCommandSet> commandSets)
	{
		Widget widget = workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE, null, zOrder, parent);
		if (!widget)
			return null;

		widget.SetZOrder(zOrder);
		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		SetupPolygonWidget(widget, BuildRectVertices(width, height), color, commandSets);
		widget.SetOpacity(opacity);
		if (userId > 0)
		{
			widget.SetUserID(userId);
			widget.AddHandler(m_WidgetHandler);
		}
		else
		{
			widget.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		}

		return widget;
	}

	protected TextWidget CreateWrappedTextWidgetAtZ(WorkspaceWidget workspace, Widget parent, string text, int left, int top, int width, int height, int fontSize, int color, int userId, bool bold, int zOrder)
	{
		Widget widget = workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NO_LOCALIZATION, null, zOrder, parent);
		if (!widget)
			return null;

		widget.SetZOrder(zOrder);
		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		TextWidget textWidget = TextWidget.Cast(widget);
		if (textWidget)
		{
			textWidget.SetText(text);
			textWidget.SetTextWrapping(true);
			ApplyTextStyle(textWidget, fontSize, bold);
		}

		widget.SetColorInt(color);
		if (userId > 0)
		{
			widget.SetUserID(userId);
			widget.AddHandler(m_WidgetHandler);
		}
		else
		{
			widget.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		}

		return textWidget;
	}

	protected bool SetupPolygonWidget(Widget widget, array<float> vertices, int color, array<ref HST_SetupMapDrawCommandSet> commandSets)
	{
		CanvasWidget canvas = CanvasWidget.Cast(widget);
		if (!canvas)
			return false;

		HST_SetupMapDrawCommandSet commandSet = new HST_SetupMapDrawCommandSet();
		PolygonDrawCommand command = new PolygonDrawCommand();
		command.m_iColor = color;
		command.m_Vertices = vertices;
		commandSet.m_aCommands.Insert(command);
		canvas.SetDrawCommands(commandSet.m_aCommands);
		if (commandSets)
			commandSets.Insert(commandSet);
		return true;
	}

	protected ref array<float> BuildRectVertices(int width, int height)
	{
		ref array<float> vertices = {};
		vertices.Insert(0.0);
		vertices.Insert(0.0);
		vertices.Insert(width);
		vertices.Insert(0.0);
		vertices.Insert(width);
		vertices.Insert(height);
		vertices.Insert(0.0);
		vertices.Insert(height);
		return vertices;
	}

	protected ref array<float> BuildCircleVertices(int diameter)
	{
		ref array<float> vertices = {};
		float radius = diameter / 2.0;
		for (int i = 0; i < 24; i++)
		{
			float angle = (6.283185 * i) / 24.0;
			vertices.Insert(radius + Math.Cos(angle) * radius);
			vertices.Insert(radius + Math.Sin(angle) * radius);
		}

		return vertices;
	}

	protected void ApplyTextStyle(TextWidget textWidget, int fontSize, bool bold)
	{
		if (!textWidget)
			return;

		if (MENU_FONT != "")
			textWidget.SetFont(MENU_FONT);

		textWidget.SetExactFontSize(fontSize);
		textWidget.SetLineSpacing(1.1);
		textWidget.SetBold(bold);
		textWidget.SetOutline(1, 0xDD000000);
		textWidget.SetShadow(2, 0xEE000000, 1, 1, 1);
	}

	protected int ZoneFillColor(string tone)
	{
		if (tone == "resistance")
			return 0x553FAE63;
		if (tone == "town")
			return 0x557FB3D5;
		if (tone == "major")
			return 0x66C9553D;

		return 0x55D39B45;
	}

	protected int ZonePointColor(string tone)
	{
		if (tone == "resistance")
			return 0xFF80D68F;
		if (tone == "town")
			return 0xFF9FD3FF;
		if (tone == "major")
			return 0xFFFF8E72;

		return 0xFFFFD166;
	}

	protected int ScalePx(float value)
	{
		return Math.Round(value * m_fScale);
	}

	protected int ScaleFont(float value)
	{
		int scaled = Math.Round(value * m_fScale);
		return ClampInt(scaled, 9, Math.Round(value * 1.15));
	}

	protected int ClampInt(int value, int minimum, int maximum)
	{
		if (value < minimum)
			return minimum;
		if (value > maximum)
			return maximum;

		return value;
	}

	protected int AbsInt(int value)
	{
		if (value < 0)
			return -value;

		return value;
	}

	protected float AbsFloat(float value)
	{
		if (value < 0.0)
			return -value;

		return value;
	}

	protected bool IsScreenRectVisible(int left, int top, int width, int height)
	{
		if (left + width < 0 || top + height < 0)
			return false;
		if (left > m_iScreenW || top > m_iScreenH)
			return false;

		return true;
	}

	protected bool IsLatestValidationResult(vector resolvedPosition)
	{
		float dx = AbsFloat(resolvedPosition[0] - m_vRequestedValidationPosition[0]);
		float dz = AbsFloat(resolvedPosition[2] - m_vRequestedValidationPosition[2]);
		return dx <= SETUP_VALIDATION_RESULT_TOLERANCE_METERS && dz <= SETUP_VALIDATION_RESULT_TOLERANCE_METERS;
	}

	protected void TickServerRequestTimeout(float timeSlice)
	{
		if (!m_bAwaitingServer)
		{
			m_fAwaitingServerAccumulator = 0;
			return;
		}

		m_fAwaitingServerAccumulator = m_fAwaitingServerAccumulator + timeSlice;
		if (m_fAwaitingServerAccumulator < SETUP_SERVER_REQUEST_TIMEOUT_SECONDS)
			return;

		m_fAwaitingServerAccumulator = 0;
		m_bAwaitingServer = false;
		if (!m_bConfirmOpen)
		{
			if (m_bIsCommander)
				m_sStatusText = "Select a location on the map to place the HQ";
			else
				m_sStatusText = "Please wait, the commander is selecting the HQ location...";
		}

		m_bOverlayDirty = true;
		DebugLog("setup server request timed out; allowing retry");
	}

	protected bool IsCoordinatorSetupFailure(string message)
	{
		return message.Contains("coordinator not ready") || message.Contains("request bridge not ready") || message.Contains("server coordinator not ready");
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

		return DecodePayloadField(line.Substring(start, end - start));
	}

	protected string DecodePayloadField(string value)
	{
		value.Replace("%7C", "|");
		value.Replace("%25", "%");
		return value;
	}

	protected bool ParseBool(string value)
	{
		return value == "1" || value == "true";
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

	protected void ClearZonePayload()
	{
		m_aZoneIds.Clear();
		m_aZoneLabels.Clear();
		m_aZoneTypes.Clear();
		m_aZoneOwners.Clear();
		m_aZoneXs.Clear();
		m_aZoneZs.Clear();
		m_aZoneRadii.Clear();
		m_aZoneTones.Clear();
	}

	protected void ClearWidgets()
	{
		foreach (Widget widget : m_aWidgets)
		{
			if (widget)
				widget.RemoveFromHierarchy();
		}

		m_aWidgets.Clear();
		m_aChromeDrawCommandSets.Clear();
	}

	protected void ClearOverlayWidgets()
	{
		foreach (Widget widget : m_aOverlayWidgets)
		{
			if (widget)
				widget.RemoveFromHierarchy();
		}

		m_aOverlayWidgets.Clear();
		m_aOverlayDrawCommandSets.Clear();
	}

	protected void ClearModalWidgets()
	{
		foreach (Widget widget : m_aModalWidgets)
		{
			if (widget)
				widget.RemoveFromHierarchy();
		}

		m_aModalWidgets.Clear();
		m_aModalDrawCommandSets.Clear();
	}

	protected void CloseCommandMenuIfOpen()
	{
		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (menu)
			menu.CloseMenuFromExternal();
	}

	protected void ActivateSetupInput()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.ActivateContext(SETUP_INPUT_CONTEXT);
		inputManager.ActivateContext(SETUP_CURSOR_CONTEXT);
	}

	protected void RefreshResolvedLocalPlayerId()
	{
		if (m_bResolvedLocalPlayerId)
			return;

		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (!request || request.ResolveLocalPlayerId() <= 0)
			return;

		m_bResolvedLocalPlayerId = true;
		RequestSetupStateNow();
	}

	protected void ClearBlockedSetupKeys()
	{
		Debug.ClearKey(KeyCode.KC_I);
		Debug.ClearKey(KeyCode.KC_W);
		Debug.ClearKey(KeyCode.KC_A);
		Debug.ClearKey(KeyCode.KC_S);
		Debug.ClearKey(KeyCode.KC_D);
		Debug.ClearKey(KeyCode.KC_SPACE);
		Debug.ClearKey(KeyCode.KC_TAB);
		Debug.ClearKey(KeyCode.KC_ESCAPE);
		Debug.ClearKey(KeyCode.KC_RETURN);
	}

	protected void RefreshLocalOwner(IEntity owner)
	{
		if (m_bIsLocalOwner)
			return;
		if (!IsLocalOwner(owner))
			return;

		m_bIsLocalOwner = true;
		s_LocalInstance = this;
		LogLocalReady();
		ActivatePendingSetupOverlay("Waiting for setup state...");
		RequestSetupStateNow();
	}

	protected void ActivatePendingSetupOverlay(string status)
	{
		if (!m_bSetupActive)
		{
			m_bSetupActive = true;
			m_bIsCommander = false;
			m_bConfirmOpen = false;
			m_bCandidateValid = false;
			m_bAwaitingServer = false;
			m_fAwaitingServerAccumulator = 0;
			m_bHasAuthoritativeSetupState = false;
			m_bResolvedLocalPlayerId = false;
		}

		if (!status.IsEmpty())
			m_sStatusText = status;

		m_bOverlayDirty = true;
	}

	protected void LogLocalReady()
	{
		if (m_bLoggedLocalReady)
			return;

		m_bLoggedLocalReady = true;
		Print("h-istasi setup ui | local player setup map component ready");
	}

	protected bool IsDebugLoggingEnabled()
	{
		return m_bDebugLoggingEnabled;
	}

	protected void DebugLog(string message)
	{
		if (!IsDebugLoggingEnabled())
			return;

		Print("h-istasi setup ui debug | " + message);
	}

	protected void DebugHeartbeat(float timeSlice)
	{
		if (!IsDebugLoggingEnabled())
			return;

		m_fDebugHeartbeatAccumulator += timeSlice;
		if (m_fDebugHeartbeatAccumulator < 5.0)
			return;

		m_fDebugHeartbeatAccumulator = 0;
		DebugLog(string.Format("heartbeat active=%1 phase=%2 commander=%3 requests=%4 payloads=%5 results=%6 overlay=%7 status=%8", m_bSetupActive, m_sPhase, m_bIsCommander, m_iSetupStateRequestCount, m_iSetupPayloadCount, m_iSetupResultCount, m_aOverlayWidgets.Count(), ShortenText(m_sStatusText, 96)));
	}

	protected bool IsLocalOwner(IEntity owner)
	{
		if (!owner)
			return false;

		BaseRplComponent rpl = BaseRplComponent.Cast(owner.FindComponent(BaseRplComponent));
		return !rpl || rpl.IsOwner();
	}
}
