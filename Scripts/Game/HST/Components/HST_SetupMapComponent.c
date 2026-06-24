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

		return m_Component.OnSetupOverlayClicked(w, w.GetUserID(), x, y);
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (!m_Component)
			return true;

		return m_Component.OnSetupOverlayPressed(w, w.GetUserID(), x, y);
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
	// SCR_CursorCustom is a workspace-level widget at z-order 10; keep setup below it.
	static const int SETUP_Z_ORDER = 9;
	static const int SETUP_OVERLAY_Z_ORDER = 51000;
	static const int SETUP_MODAL_Z_ORDER = 52000;
	static const int SETUP_MAP_MARKER_Z_ORDER = SETUP_OVERLAY_Z_ORDER + 10;
	static const int SETUP_CHROME_Z_ORDER = SETUP_OVERLAY_Z_ORDER + 40;
	static const float SETUP_STATE_REQUEST_INTERVAL_SECONDS = 2.5;
	static const float SETUP_SERVER_REQUEST_TIMEOUT_SECONDS = 5.0;
	static const float SETUP_VALIDATION_RESULT_TOLERANCE_METERS = 8.0;
	static const float SETUP_MODAL_CLICK_SUPPRESSION_SECONDS = 0.35;
	static const int SETUP_MAP_READY_MAX_RETRIES = 8;
	static const ResourceName SETUP_NATIVE_MAP_LAYOUT = "{6985327711306200}UI/layouts/HST_SetupHQMap.layout";
	static const ResourceName SETUP_NATIVE_MAP_CONFIG = "{6985327711306210}Configs/Map/HST_SetupHQMap.conf";
	static const string SETUP_INPUT_CONTEXT = "InGameMenuContext";
	static const string SETUP_MAP_CONTEXT = "MapContext";

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
	protected bool m_bSetupMapOpenComplete;
	protected bool m_bSetupMapInitialViewApplied;
	protected bool m_bSetupMapReadyQueued;
	protected bool m_bNativeMapViewportReady;
	protected bool m_bOverlayRedrawQueued;
	protected bool m_bSetupFinalized;
	protected bool m_bOverlayDirty = true;
	protected bool m_bSetupLocationSelectionEnabled;
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
	protected ref array<Widget> m_aChromeWidgets = {};
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
	protected float m_fModalClickSuppressionSeconds;
	protected int m_iScreenW; // Layout width used for FrameSlot positions.
	protected int m_iScreenH; // Layout height used for FrameSlot positions.
	protected int m_iSetupStateRequestCount;
	protected int m_iSetupPayloadCount;
	protected int m_iSetupResultCount;
	protected int m_iOverlayRenderCount;
	protected int m_iSetupMapReadyRetries;
	protected int m_iMapReadyFrames;
	protected int m_iConfirmNoLeft;
	protected int m_iConfirmNoTop;
	protected int m_iConfirmYesLeft;
	protected int m_iConfirmYesTop;
	protected int m_iConfirmButtonW;
	protected int m_iConfirmButtonH;
	protected bool m_bLoggedLocalReady;
	protected bool m_bLoggedBridgeMissing;
	protected bool m_bLoggedBridgeRecovered;
	protected float m_fScale = 1.0;
	protected Widget m_wModalRoot;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		m_bIsLocalOwner = IsLocalOwner(owner);
		m_bDebugLoggingEnabled = HST_RuntimeSettingsService.LoadDebugLoggingEnabledQuiet();
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

		if (!m_bSetupFinalized)
		{
			m_fRequestAccumulator += timeSlice;
			if (m_fRequestAccumulator >= SETUP_STATE_REQUEST_INTERVAL_SECONDS)
			{
				m_fRequestAccumulator = 0;
				RequestSetupStateNow();
			}
		}

		if (!m_bSetupActive)
			return;

		RefreshResolvedLocalPlayerId();
		ActivateSetupInput();
		ClearBlockedSetupKeys();
		SCR_RespawnSystemComponent.CloseRespawnMenu();
		CloseCommandMenuIfOpen();
		TickServerRequestTimeout(timeSlice);
		TickModalClickSuppression(timeSlice);
		RefreshScreenMetrics();
		EnsureNativeSetupMapOpen();
		UpdateSetupPrompt();
		UpdateConfirmationVisibility();
		UpdateSetupLocationSelectionMode();
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
		if (!m_bIsLocalOwner || m_bSetupFinalized)
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
				DebugLog("request bridge not ready; setup state request pending");
			}

			return;
		}

		if (m_bLoggedBridgeMissing && !m_bLoggedBridgeRecovered)
		{
			m_bLoggedBridgeRecovered = true;
			DebugLog("request bridge recovered");
		}

		request.RequestSetupState(m_sLastResult);
	}

	void OnServerSetupState(string payload, string lastResult = "")
	{
		m_iSetupPayloadCount++;
		m_bHasAuthoritativeSetupState = true;
		m_sLastResult = lastResult;
		ParseSetupStatePayload(payload);
		PublishSetupZoneOverlay();
		DebugLog(string.Format("state payload #%1 phase=%2 active=%3 commander=%4 zones=%5 status=%6", m_iSetupPayloadCount, m_sPhase, m_bSetupActive, m_bIsCommander, m_aZoneIds.Count(), ShortenText(m_sStatusText, 96)));
		if (!m_bSetupActive)
		{
			ClearSetupCandidateOverlay();
			HST_MapZoneOverlayUIComponent.ClearSetupZones();
			if (IsAuthoritativeSetupComplete())
				FinalizeSetupMap();
			else
				CloseNativeSetupMap();

			return;
		}

		m_bOverlayDirty = true;
	}

	protected bool IsAuthoritativeSetupComplete()
	{
		return m_bHasAuthoritativeSetupState && !m_bSetupActive && m_sPhase == "active";
	}

	protected void FinalizeSetupMap()
	{
		if (m_bSetupFinalized)
			return;

		m_bSetupFinalized = true;
		m_bSetupActive = false;
		m_bConfirmOpen = false;
		m_bCandidateValid = false;
		m_bAwaitingServer = false;
		m_fRequestAccumulator = 0;
		m_fAwaitingServerAccumulator = 0;
		m_fModalClickSuppressionSeconds = 0;
		m_vCandidatePosition = "0 0 0";
		m_vRequestedValidationPosition = "0 0 0";
		ClearSetupCandidateOverlay();
		HST_MapZoneOverlayUIComponent.ClearSetupZones();
		CloseNativeSetupMap();
		CloseRespawnMenuAfterSetup();
		GetGame().GetCallqueue().CallLater(CloseRespawnMenuAfterSetup, 0, false);
		GetGame().GetCallqueue().CallLater(CloseRespawnMenuAfterSetup, 250, false);
		GetGame().GetCallqueue().CallLater(CloseRespawnMenuAfterSetup, 750, false);
		ResetCommandMenuInputLatch();
		Debug.ClearKey(KeyCode.KC_I);
		DebugLog("setup finalized; native setup map closed");
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
			UpdateSetupLocationSelectionMode();
		}
		else if (action == "validate")
		{
			m_bCandidateValid = false;
			m_bConfirmOpen = false;
		}
		else if (action == "confirm" && accepted)
		{
			m_bConfirmOpen = false;
			m_bCandidateValid = false;
			m_vCandidatePosition = "0 0 0";
			RequestSetupStateNow();
		}
		else if (action == "confirm")
		{
			m_bConfirmOpen = false;
			m_bCandidateValid = false;
			m_vCandidatePosition = "0 0 0";
		}

		m_bOverlayDirty = true;
		PublishSetupCandidateOverlay();
		UpdateConfirmationVisibility();
		UpdateSetupLocationSelectionMode();
	}

	bool OnSetupOverlayPressed(Widget widget, int widgetId, int x, int y)
	{
		if (m_bSetupActive && m_bConfirmOpen && IsConfirmModalInput(widget, widgetId, x, y))
			SuppressNativeMapSelection();

		return true;
	}

	bool OnSetupOverlayClicked(Widget widget, int widgetId, int x = -1, int y = -1)
	{
		if (!m_bSetupActive)
			return true;

		int resolvedWidgetId = ResolveConfirmWidgetId(widget, widgetId, x, y);
		if (IsConfirmModalInput(widget, resolvedWidgetId, x, y))
			SuppressNativeMapSelection();

		if (resolvedWidgetId == CONFIRM_NO_WIDGET_ID)
		{
			CancelConfirmSelection();
			return true;
		}

		if (resolvedWidgetId == CONFIRM_YES_WIDGET_ID)
		{
			if (m_bCandidateValid && !m_bAwaitingServer)
				RequestConfirmPosition();
			return true;
		}

		return true;
	}

	protected int ResolveConfirmWidgetId(Widget widget, int widgetId, int x, int y)
	{
		if (!m_bConfirmOpen)
			return widgetId;
		if (widgetId == CONFIRM_YES_WIDGET_ID || widgetId == CONFIRM_NO_WIDGET_ID)
			return widgetId;

		if (IsWidgetPointInsideRect(widget, x, y, m_iConfirmYesLeft, m_iConfirmYesTop, m_iConfirmButtonW, m_iConfirmButtonH))
			return CONFIRM_YES_WIDGET_ID;
		if (IsWidgetPointInsideRect(widget, x, y, m_iConfirmNoLeft, m_iConfirmNoTop, m_iConfirmButtonW, m_iConfirmButtonH))
			return CONFIRM_NO_WIDGET_ID;

		return widgetId;
	}

	protected bool IsConfirmModalInput(Widget widget, int widgetId, int x, int y)
	{
		if (widgetId == CONFIRM_YES_WIDGET_ID || widgetId == CONFIRM_NO_WIDGET_ID || widgetId == CONFIRM_BLOCKER_WIDGET_ID)
			return true;
		if (!m_bConfirmOpen)
			return false;

		return IsWidgetPointInsideRect(widget, x, y, m_iConfirmYesLeft, m_iConfirmYesTop, m_iConfirmButtonW, m_iConfirmButtonH)
			|| IsWidgetPointInsideRect(widget, x, y, m_iConfirmNoLeft, m_iConfirmNoTop, m_iConfirmButtonW, m_iConfirmButtonH);
	}

	protected bool HandleConfirmModalMapSelection(int layoutX, int layoutY, int alternateLayoutX, int alternateLayoutY)
	{
		int resolvedWidgetId = ResolveConfirmLayoutPoint(layoutX, layoutY);
		if (resolvedWidgetId == 0)
			resolvedWidgetId = ResolveConfirmLayoutPoint(alternateLayoutX, alternateLayoutY);

		SuppressNativeMapSelection();

		if (resolvedWidgetId == CONFIRM_NO_WIDGET_ID)
		{
			CancelConfirmSelection();
			return true;
		}

		if (resolvedWidgetId == CONFIRM_YES_WIDGET_ID)
		{
			if (m_bCandidateValid && !m_bAwaitingServer)
				RequestConfirmPosition();

			return true;
		}

		DebugLog("ignored native map selection behind setup confirmation modal");
		return true;
	}

	protected int ResolveConfirmLayoutPoint(int layoutX, int layoutY)
	{
		if (!m_bConfirmOpen)
			return 0;

		return ResolveConfirmLayoutPointUnchecked(layoutX, layoutY);
	}

	protected int ResolveConfirmLayoutPointUnchecked(int layoutX, int layoutY)
	{
		if (IsPointInsideRect(layoutX, layoutY, m_iConfirmYesLeft, m_iConfirmYesTop, m_iConfirmButtonW, m_iConfirmButtonH))
			return CONFIRM_YES_WIDGET_ID;
		if (IsPointInsideRect(layoutX, layoutY, m_iConfirmNoLeft, m_iConfirmNoTop, m_iConfirmButtonW, m_iConfirmButtonH))
			return CONFIRM_NO_WIDGET_ID;

		return 0;
	}

	protected void CancelConfirmSelection()
	{
		m_bConfirmOpen = false;
		m_sStatusText = "Select a location on the map to place the HQ";
		m_bOverlayDirty = true;
		ClearSetupCandidateOverlay();
		UpdateConfirmationVisibility();
		UpdateSetupLocationSelectionMode();
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

	protected void PublishSetupZoneOverlay()
	{
		if (!m_bSetupActive)
		{
			HST_MapZoneOverlayUIComponent.ClearSetupZones();
			return;
		}

		HST_MapZoneOverlayUIComponent.SetSetupZones(m_aZoneIds, m_aZoneLabels, m_aZoneXs, m_aZoneZs, m_aZoneRadii, m_aZoneTones);
	}

	protected void PublishSetupCandidateOverlay()
	{
		if (!m_bSetupActive || IsZeroVector(m_vCandidatePosition) || (!m_bCandidateValid && !m_bAwaitingServer))
		{
			HST_MapZoneOverlayUIComponent.ClearSetupCandidate();
			return;
		}

		int color = 0xFFFFD166;
		if (m_bAwaitingServer)
			color = 0xFF9FD3FF;

		HST_MapZoneOverlayUIComponent.SetSetupCandidate(m_vCandidatePosition, "HQ", color);
	}

	protected void ClearSetupCandidateOverlay()
	{
		HST_MapZoneOverlayUIComponent.ClearSetupCandidate();
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
			m_wModalRoot = m_wSetupRoot.FindAnyWidget("HST_SetupModalRoot");
			if (!m_wModalRoot)
				m_wModalRoot = m_wSetupRoot;

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
				if (!m_bSetupMapOpenComplete && IsNativeSetupMapZoomInitialized())
				{
					m_bSetupMapOpenComplete = true;
					QueueApplySetupMapReadyState();
				}
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

		ResetSetupMapReadyState();
		m_MapEntity.OpenMap(mapConfig);
		m_bNativeMapOpen = true;
		QueueApplySetupMapReadyState();
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
		SCR_MapEntity.GetOnMapClose().Insert(OnNativeSetupMapClose);
		m_bNativeInvokersBound = true;
	}

	protected void CloseNativeSetupMap()
	{
		SetSetupLocationSelectionEnabled(false);
		ClearSetupCandidateOverlay();
		HST_MapZoneOverlayUIComponent.ClearSetupZones();

		if (m_bNativeInvokersBound)
		{
			SCR_MapEntity.GetOnSelection().Remove(OnNativeMapSelection);
			SCR_MapEntity.GetOnMapPan().Remove(OnNativeMapPan);
			SCR_MapEntity.GetOnMapZoom().Remove(OnNativeMapZoom);
			SCR_MapEntity.GetOnMapOpenComplete().Remove(OnNativeMapOpenComplete);
			SCR_MapEntity.GetOnMapClose().Remove(OnNativeSetupMapClose);
			m_bNativeInvokersBound = false;
		}

		if (m_bNativeMapOpen && m_MapEntity && m_MapEntity.IsOpen() && m_wSetupRoot && m_MapEntity.GetMapMenuRoot() == m_wSetupRoot)
			m_MapEntity.CloseMap();

		m_bNativeMapOpen = false;
		ResetSetupMapReadyState();
		ClearModalWidgets();
		ClearOverlayWidgets();
		ClearChromeWidgets();
		ClearWidgets();
		m_wSetupRoot = null;
		m_wOverlayRoot = null;
		m_wModalRoot = null;
		m_wPromptText = null;
	}

	protected void OnNativeMapSelection(vector screenPos)
	{
		float nativeScreenX = screenPos[0];
		float nativeScreenY = screenPos[2];
		int layoutX;
		int layoutY;
		int alternateLayoutX;
		int alternateLayoutY;
		NativeScreenToLayoutCandidates(nativeScreenX, nativeScreenY, layoutX, layoutY, alternateLayoutX, alternateLayoutY);

		if (m_bSetupActive && m_bConfirmOpen)
		{
			HandleConfirmModalMapSelection(layoutX, layoutY, alternateLayoutX, alternateLayoutY);
			return;
		}

		if (m_fModalClickSuppressionSeconds > 0.0)
		{
			m_fModalClickSuppressionSeconds = 0;
			DebugLog("ignored native map selection from setup modal click");
			return;
		}

		if (!m_bSetupActive || !m_bIsCommander || m_bConfirmOpen || m_bAwaitingServer)
			return;
		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return;
		if (!IsSetupMapViewReady())
			return;

		if (IsSetupChromePoint(layoutX, layoutY, alternateLayoutX, alternateLayoutY))
			return;

		float worldX;
		float worldZ;
		m_MapEntity.ScreenToWorld(Math.Round(nativeScreenX), Math.Round(nativeScreenY), worldX, worldZ);

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
		QueueOverlayRedraw("pan");
	}

	protected void OnNativeMapZoom(float zoom)
	{
		QueueOverlayRedraw("zoom");
	}

	protected void OnNativeMapOpenComplete(MapConfiguration config)
	{
		if (m_bSetupActive && IsSetupMapRootActive())
		{
			m_bSetupMapOpenComplete = true;
			m_iMapReadyFrames = 0;
			QueueApplySetupMapReadyState();
		}

		QueueOverlayRedraw("map open complete");
	}

	protected void OnNativeSetupMapClose(MapConfiguration config)
	{
		if (!m_bSetupActive || m_bSetupFinalized)
			return;
		if (config && m_wSetupRoot && config.RootWidgetRef != m_wSetupRoot)
			return;

		m_bNativeMapOpen = false;
		SetSetupLocationSelectionEnabled(false);
		ResetSetupMapReadyState();
		GetGame().GetCallqueue().CallLater(ReopenSetupMapAfterCloseAttempt, 0, false);
	}

	protected void ReopenSetupMapAfterCloseAttempt()
	{
		if (!m_bSetupActive || m_bSetupFinalized)
			return;

		EnsureNativeSetupMapOpen();
	}

	protected bool IsSetupMapRootActive()
	{
		return m_MapEntity && m_MapEntity.IsOpen() && m_wSetupRoot && m_MapEntity.GetMapMenuRoot() == m_wSetupRoot;
	}

	protected void ResetSetupMapReadyState()
	{
		m_bSetupMapOpenComplete = false;
		m_bSetupMapInitialViewApplied = false;
		m_bSetupMapReadyQueued = false;
		m_bNativeMapViewportReady = false;
		m_bOverlayRedrawQueued = false;
		m_iSetupMapReadyRetries = 0;
		m_iMapReadyFrames = 0;
	}

	protected void QueueApplySetupMapReadyState()
	{
		if (m_bSetupMapReadyQueued)
			return;

		m_bSetupMapReadyQueued = true;
		GetGame().GetCallqueue().CallLater(ApplySetupMapReadyState, 0, false);
	}

	protected void ApplySetupMapReadyState()
	{
		m_bSetupMapReadyQueued = false;
		if (!m_bSetupActive || !m_bSetupMapOpenComplete || !IsSetupMapRootActive())
			return;

		if (!IsNativeSetupMapZoomInitialized())
		{
			if (m_iSetupMapReadyRetries < SETUP_MAP_READY_MAX_RETRIES)
			{
				m_iSetupMapReadyRetries++;
				QueueApplySetupMapReadyState();
				return;
			}

			DebugLog("setup map ready state skipped: native zoom was not initialized");
			return;
		}

		m_iMapReadyFrames++;
		if (m_iMapReadyFrames < 2)
		{
			QueueApplySetupMapReadyState();
			return;
		}

		m_bNativeMapViewportReady = true;
		ApplySetupMapInitialView();
		UpdateSetupLocationSelectionMode();
		QueueOverlayRedraw("native map viewport ready");
	}

	protected void UpdateSetupLocationSelectionMode()
	{
		bool shouldEnable = m_bSetupActive
			&& m_bIsCommander
			&& !m_bConfirmOpen
			&& !m_bAwaitingServer
			&& IsSetupMapViewReady();

		SetSetupLocationSelectionEnabled(shouldEnable);
	}

	protected void SetSetupLocationSelectionEnabled(bool enabled)
	{
		if (!m_MapEntity || !m_MapEntity.IsOpen())
		{
			m_bSetupLocationSelectionEnabled = false;
			return;
		}

		SCR_MapCursorModule cursorModule = SCR_MapCursorModule.Cast(m_MapEntity.GetMapModule(SCR_MapCursorModule));
		if (!cursorModule)
			return;

		if (m_bSetupLocationSelectionEnabled == enabled)
			return;

		cursorModule.ToggleLocationSelection(enabled);
		m_bSetupLocationSelectionEnabled = enabled;
	}

	protected bool IsSetupMapViewReady()
	{
		if (!m_bSetupMapOpenComplete || !m_bNativeMapViewportReady || !IsSetupMapRootActive())
			return false;
		if (!IsNativeSetupMapZoomInitialized())
			return false;

		return true;
	}

	protected bool IsNativeSetupMapZoomInitialized()
	{
		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return false;
		CanvasWidget mapWidget = m_MapEntity.GetMapWidget();
		if (!mapWidget)
			return false;
		float mapScreenW;
		float mapScreenH;
		mapWidget.GetScreenSize(mapScreenW, mapScreenH);
		if (mapScreenW <= 0.0 || mapScreenH <= 0.0)
			return false;
		if (m_MapEntity.GetCurrentZoom() <= 0.0)
			return false;
		if (m_MapEntity.GetMinZoom() <= 0.0)
			return false;

		return true;
	}

	protected void ApplySetupMapInitialView()
	{
		if (m_bSetupMapInitialViewApplied || !IsSetupMapViewReady())
			return;

		m_MapEntity.ZoomOut();
		m_bSetupMapInitialViewApplied = true;
		QueueOverlayRedraw("initial view");
	}

	protected void QueueOverlayRedraw(string reason)
	{
		if (m_bOverlayRedrawQueued)
			return;

		m_bOverlayRedrawQueued = true;
		GetGame().GetCallqueue().CallLater(FlushOverlayRedraw, 0, false);
	}

	protected void FlushOverlayRedraw()
	{
		m_bOverlayRedrawQueued = false;
		if (!m_bNativeMapViewportReady)
			return;

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
		PublishSetupCandidateOverlay();
		UpdateSetupLocationSelectionMode();
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
		PublishSetupCandidateOverlay();
		UpdateSetupLocationSelectionMode();
		request.RequestSetupConfirmPosition(m_vCandidatePosition[0], m_vCandidatePosition[2]);
	}

	protected void RefreshScreenMetrics()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		int screenW;
		int screenH;
		HST_UIWorkspaceMetrics.GetLayoutSize(workspace, screenW, screenH);
		if (screenW != m_iScreenW || screenH != m_iScreenH)
		{
			m_iScreenW = screenW;
			m_iScreenH = screenH;
			m_bOverlayDirty = true;
			m_bNativeMapViewportReady = false;
			m_iMapReadyFrames = 0;
			m_fScale = HST_UIWorkspaceMetrics.GetScale(m_iScreenW, m_iScreenH, 0.70, 1.15);
			if (m_bSetupMapOpenComplete)
				QueueApplySetupMapReadyState();

			if (m_wOverlayRoot)
			{
				ClearChromeWidgets();
				m_wPromptText = CreatePromptText(workspace, m_wOverlayRoot);
				UpdateSetupPrompt();
			}

			if (m_bConfirmOpen)
			{
				ClearModalWidgets();
				UpdateConfirmationVisibility();
			}
		}
	}

	protected TextWidget CreatePromptText(WorkspaceWidget workspace, Widget parent)
	{
		Widget panel = CreateRectWidgetAtZ(workspace, parent, 0, 0, Math.Max(1, m_iScreenW), ScalePx(76), 0xFF0D1318, 1.0, 0, SETUP_CHROME_Z_ORDER, m_aChromeDrawCommandSets);
		if (panel)
			m_aChromeWidgets.Insert(panel);

		Widget rule = CreateRectWidgetAtZ(workspace, parent, 0, ScalePx(72), Math.Max(1, m_iScreenW), ScalePx(4), 0xFFC4953B, 1.0, 0, SETUP_CHROME_Z_ORDER + 1, m_aChromeDrawCommandSets);
		if (rule)
			m_aChromeWidgets.Insert(rule);

		TextWidget prompt = CreateWrappedTextWidgetAtZ(workspace, parent, "", ScalePx(36), ScalePx(18), Math.Max(1, m_iScreenW - ScalePx(72)), ScalePx(42), ScaleFont(22), 0xFFF2E6CA, 0, true, SETUP_CHROME_Z_ORDER + 2);
		if (prompt)
			m_aChromeWidgets.Insert(prompt);

		return prompt;
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
		int modalH = ScalePx(280);
		int left = (m_iScreenW - modalW) / 2;
		int top = (m_iScreenH - modalH) / 2;
		CreateModalRect(workspace, 0, 0, m_iScreenW, m_iScreenH, 0xA8000000, 1.0, CONFIRM_BLOCKER_WIDGET_ID, SETUP_MODAL_Z_ORDER + 1);
		CreateModalRect(workspace, left - ScalePx(16), top - ScalePx(16), modalW + ScalePx(32), modalH + ScalePx(32), 0xE0000000, 1.0, CONFIRM_BLOCKER_WIDGET_ID, SETUP_MODAL_Z_ORDER + 2);
		CreateModalRect(workspace, left, top, modalW, modalH, 0xFF1D2932, 1.0, CONFIRM_BLOCKER_WIDGET_ID, SETUP_MODAL_Z_ORDER + 3);
		CreateModalRect(workspace, left, top, modalW, ScalePx(4), 0xFFC4953B, 1.0, CONFIRM_BLOCKER_WIDGET_ID, SETUP_MODAL_Z_ORDER + 4);
		CreateModalText(workspace, "Are you sure you want to place the HQ here?", left + ScalePx(34), top + ScalePx(34), modalW - ScalePx(68), ScalePx(120), ScaleFont(22), 0xFFF2E6CA, CONFIRM_BLOCKER_WIDGET_ID, true, SETUP_MODAL_Z_ORDER + 5);
		int buttonW = ScalePx(160);
		int buttonH = ScalePx(50);
		int buttonTop = top + modalH - ScalePx(74);
		int noLeft = left + modalW / 2 - buttonW - ScalePx(12);
		int yesLeft = left + modalW / 2 + ScalePx(12);
		m_iConfirmNoLeft = noLeft;
		m_iConfirmNoTop = buttonTop;
		m_iConfirmYesLeft = yesLeft;
		m_iConfirmYesTop = buttonTop;
		m_iConfirmButtonW = buttonW;
		m_iConfirmButtonH = buttonH;
		CreateModalRect(workspace, noLeft, buttonTop, buttonW, buttonH, 0xFF46535D, 1.0, CONFIRM_NO_WIDGET_ID, SETUP_MODAL_Z_ORDER + 5);
		CreateModalText(workspace, "No", noLeft + ScalePx(20), buttonTop + ScalePx(13), buttonW - ScalePx(40), ScalePx(26), ScaleFont(17), 0xFFF4EBD6, CONFIRM_NO_WIDGET_ID, true, SETUP_MODAL_Z_ORDER + 6);
		CreateModalRect(workspace, yesLeft, buttonTop, buttonW, buttonH, 0xFFC4953B, 1.0, CONFIRM_YES_WIDGET_ID, SETUP_MODAL_Z_ORDER + 5);
		CreateModalText(workspace, "Yes", yesLeft + ScalePx(20), buttonTop + ScalePx(13), buttonW - ScalePx(40), ScalePx(26), ScaleFont(17), 0xFF111820, CONFIRM_YES_WIDGET_ID, true, SETUP_MODAL_Z_ORDER + 6);
		CreateModalHitTarget(workspace, noLeft, buttonTop, buttonW, buttonH, CONFIRM_NO_WIDGET_ID, SETUP_MODAL_Z_ORDER + 12);
		CreateModalHitTarget(workspace, yesLeft, buttonTop, buttonW, buttonH, CONFIRM_YES_WIDGET_ID, SETUP_MODAL_Z_ORDER + 12);
	}

	protected void RedrawNativeMapOverlay()
	{
		ClearOverlayWidgets();
		if (!m_bSetupActive || !m_wOverlayRoot || !IsSetupMapViewReady())
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		m_iOverlayRenderCount++;
		if (m_iOverlayRenderCount <= 3 || (m_iOverlayRenderCount % 20) == 0)
			DebugLog(string.Format("overlay render #%1 commander=%2 confirm=%3 awaiting=%4 zones=%5", m_iOverlayRenderCount, m_bIsCommander, m_bConfirmOpen, m_bAwaitingServer, m_aZoneIds.Count()));

		PublishSetupZoneOverlay();
		PublishSetupCandidateOverlay();
	}

	protected void NativeScreenToLayoutCandidates(float nativeScreenX, float nativeScreenY, out int layoutX, out int layoutY, out int alternateLayoutX, out int alternateLayoutY)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		HST_UIWorkspaceMetrics.GetNativePointCandidates(workspace, nativeScreenX, nativeScreenY, layoutX, layoutY, alternateLayoutX, alternateLayoutY);
	}

	protected bool IsSetupChromePoint(int layoutX, int layoutY, int alternateLayoutX, int alternateLayoutY)
	{
		int chromeHeight = ScalePx(76);
		return layoutY <= chromeHeight || alternateLayoutY <= chromeHeight;
	}

	protected Widget CreateModalRect(WorkspaceWidget workspace, int left, int top, int width, int height, int color, float opacity, int userId, int zOrder)
	{
		Widget widget = CreateRectWidgetAtZ(workspace, ResolveModalRoot(), left, top, width, height, color, opacity, userId, zOrder, m_aModalDrawCommandSets);
		if (widget)
			m_aModalWidgets.Insert(widget);

		return widget;
	}

	protected TextWidget CreateModalText(WorkspaceWidget workspace, string text, int left, int top, int width, int height, int fontSize, int color, int userId, bool bold, int zOrder)
	{
		TextWidget widget = CreateWrappedTextWidgetAtZ(workspace, ResolveModalRoot(), text, left, top, width, height, fontSize, color, userId, bold, zOrder);
		if (widget)
			m_aModalWidgets.Insert(widget);

		return widget;
	}

	protected Widget CreateModalHitTarget(WorkspaceWidget workspace, int left, int top, int width, int height, int userId, int zOrder)
	{
		Widget widget = workspace.CreateWidget(WidgetType.ButtonWidgetTypeID, WidgetFlags.VISIBLE, null, zOrder, ResolveModalRoot());
		if (!widget)
			return null;

		widget.SetZOrder(zOrder);
		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		widget.SetOpacity(0.01);
		widget.SetUserID(userId);
		widget.AddHandler(m_WidgetHandler);
		m_aModalWidgets.Insert(widget);
		return widget;
	}

	protected Widget ResolveModalRoot()
	{
		if (m_wModalRoot)
			return m_wModalRoot;

		return m_wSetupRoot;
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

	protected void ApplyTextStyle(TextWidget textWidget, int fontSize, bool bold)
	{
		if (!textWidget)
			return;

		textWidget.SetExactFontSize(fontSize);
		textWidget.SetLineSpacing(1.1);
		textWidget.SetBold(bold);
		textWidget.SetOutline(1, 0xDD000000);
		textWidget.SetShadow(2, 0xEE000000, 1, 1, 1);
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

	protected float AbsFloat(float value)
	{
		if (value < 0.0)
			return -value;

		return value;
	}

	protected bool IsZeroVector(vector value)
	{
		return AbsFloat(value[0]) < 0.01 && AbsFloat(value[1]) < 0.01 && AbsFloat(value[2]) < 0.01;
	}

	protected bool IsPointInsideRect(int x, int y, int left, int top, int width, int height)
	{
		if (width <= 0 || height <= 0)
			return false;
		if (x < left || y < top)
			return false;
		if (x > left + width || y > top + height)
			return false;

		return true;
	}

	protected bool IsWidgetPointInsideRect(Widget widget, int x, int y, int left, int top, int width, int height)
	{
		if (IsPointInsideRect(x, y, left, top, width, height))
			return true;
		if (!widget)
			return false;

		float widgetX;
		float widgetY;
		widget.GetScreenPos(widgetX, widgetY);
		if (IsPointInsideRect(Math.Round(widgetX) + x, Math.Round(widgetY) + y, left, top, width, height))
			return true;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		int layoutX = Math.Round(workspace.DPIUnscale(widgetX)) + x;
		int layoutY = Math.Round(workspace.DPIUnscale(widgetY)) + y;
		return IsPointInsideRect(layoutX, layoutY, left, top, width, height);
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
		UpdateSetupLocationSelectionMode();
		DebugLog("setup server request timed out; allowing retry");
	}

	protected void SuppressNativeMapSelection()
	{
		m_fModalClickSuppressionSeconds = SETUP_MODAL_CLICK_SUPPRESSION_SECONDS;
	}

	protected void TickModalClickSuppression(float timeSlice)
	{
		if (m_fModalClickSuppressionSeconds <= 0.0)
			return;

		m_fModalClickSuppressionSeconds = m_fModalClickSuppressionSeconds - timeSlice;
		if (m_fModalClickSuppressionSeconds < 0.0)
			m_fModalClickSuppressionSeconds = 0;
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

	protected void ClearChromeWidgets()
	{
		foreach (Widget widget : m_aChromeWidgets)
		{
			if (widget)
				widget.RemoveFromHierarchy();
		}

		m_aChromeWidgets.Clear();
		m_aChromeDrawCommandSets.Clear();
		m_wPromptText = null;
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
		ClearConfirmButtonRects();
	}

	protected void ClearConfirmButtonRects()
	{
		m_iConfirmNoLeft = 0;
		m_iConfirmNoTop = 0;
		m_iConfirmYesLeft = 0;
		m_iConfirmYesTop = 0;
		m_iConfirmButtonW = 0;
		m_iConfirmButtonH = 0;
	}

	protected void CloseCommandMenuIfOpen()
	{
		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (menu)
			menu.CloseMenuFromExternal();
	}

	protected void CloseRespawnMenuAfterSetup()
	{
		SCR_RespawnSystemComponent.CloseRespawnMenu();
	}

	protected void ResetCommandMenuInputLatch()
	{
		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (menu)
			menu.ResetInputLatchAfterSetupMap();
	}

	protected void ActivateSetupInput()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.ActivateContext(SETUP_INPUT_CONTEXT);
		inputManager.ActivateContext(SETUP_MAP_CONTEXT);
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
		if (m_bSetupFinalized)
			return;

		if (!m_bSetupActive)
		{
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
		DebugLog("local player setup map component ready");
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

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId <= 0)
			return false;

		PlayerController controller = PlayerController.Cast(owner);
		return controller && controller.GetPlayerId() == localPlayerId;
	}
}
