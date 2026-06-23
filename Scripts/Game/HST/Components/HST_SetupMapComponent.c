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
			return false;

		return m_Component.OnSetupWidgetActivated(w.GetUserID(), x, y);
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
	static const int ROOT_SHIELD_WIDGET_ID = 71000;
	static const int MAP_WIDGET_ID = 71001;
	static const int CONFIRM_YES_WIDGET_ID = 71002;
	static const int CONFIRM_NO_WIDGET_ID = 71003;
	static const int SETUP_Z_ORDER = 50000;
	static const float SETUP_STATE_REQUEST_INTERVAL_SECONDS = 2.5;
	static const ResourceName MENU_FONT = "";

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
	protected bool m_bRenderDirty = true;
	protected string m_sPhase = "setup";
	protected string m_sStatusText = "Waiting for setup state...";
	protected string m_sLastResult;
	protected vector m_vWorldMin = "0 0 0";
	protected vector m_vWorldMax = "12800 0 12800";
	protected vector m_vCandidatePosition = "0 0 0";
	protected ref array<string> m_aZoneIds = {};
	protected ref array<string> m_aZoneLabels = {};
	protected ref array<string> m_aZoneTypes = {};
	protected ref array<string> m_aZoneOwners = {};
	protected ref array<float> m_aZoneXs = {};
	protected ref array<float> m_aZoneZs = {};
	protected ref array<float> m_aZoneRadii = {};
	protected ref array<string> m_aZoneTones = {};
	protected ref array<Widget> m_aWidgets = {};
	protected ref array<ref HST_SetupMapDrawCommandSet> m_aCanvasCommandSets = {};
	protected ref HST_SetupMapWidgetHandler m_WidgetHandler;
	protected float m_fOwnerRetryAccumulator;
	protected float m_fRequestAccumulator;
	protected float m_fDebugHeartbeatAccumulator;
	protected int m_iScreenW;
	protected int m_iScreenH;
	protected int m_iMapLeft;
	protected int m_iMapTop;
	protected int m_iMapWidth;
	protected int m_iMapHeight;
	protected int m_iSetupStateRequestCount;
	protected int m_iSetupPayloadCount;
	protected int m_iSetupResultCount;
	protected int m_iRenderCount;
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

		ClearWidgets();
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

		if (m_bSetupActive)
		{
			ClearBlockedSetupKeys();
			SCR_RespawnSystemComponent.CloseRespawnMenu();
			CloseCommandMenuIfOpen();
			DebugHeartbeat(timeSlice);
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (workspace)
		{
			int screenW = Math.Max(1, Math.Round(workspace.GetWidth()));
			int screenH = Math.Max(1, Math.Round(workspace.GetHeight()));
			if (screenW != m_iScreenW || screenH != m_iScreenH)
				m_bRenderDirty = true;
		}

		if (m_bRenderDirty)
			RenderSetupMap();
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
			m_bRenderDirty = true;
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

		if (request)
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
			ClearWidgets();
			DebugLog("setup inactive; cleared setup UI");
			return;
		}

		m_bRenderDirty = true;
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
		m_sLastResult = message;
		m_sStatusText = message;
		m_bAwaitingServer = false;
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

		m_bRenderDirty = true;
	}

	bool OnSetupWidgetActivated(int widgetId, int x, int y)
	{
		if (!m_bSetupActive)
			return false;

		if (widgetId == CONFIRM_NO_WIDGET_ID)
		{
			m_bConfirmOpen = false;
			m_sStatusText = "Select a location on the map to place the HQ";
			m_bRenderDirty = true;
			return true;
		}

		if (widgetId == CONFIRM_YES_WIDGET_ID)
		{
			if (m_bCandidateValid)
				RequestConfirmPosition();
			return true;
		}

		if (widgetId == MAP_WIDGET_ID)
		{
			if (!m_bIsCommander || m_bConfirmOpen || m_bAwaitingServer)
				return true;

			vector worldPosition = ScreenToWorldPosition(x, y);
			m_vCandidatePosition = worldPosition;
			m_bCandidateValid = false;
			RequestValidatePosition(worldPosition);
			m_bRenderDirty = true;
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

	protected void RequestValidatePosition(vector worldPosition)
	{
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (!request)
		{
			m_sStatusText = "setup request bridge not ready";
			m_bRenderDirty = true;
			DebugLog("validate request aborted: request bridge missing");
			return;
		}

		m_bAwaitingServer = true;
		m_sStatusText = "Checking HQ location...";
		DebugLog(string.Format("validate request %1", worldPosition));
		request.RequestSetupValidatePosition(worldPosition[0], worldPosition[2]);
	}

	protected void RequestConfirmPosition()
	{
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (!request)
		{
			m_sStatusText = "setup request bridge not ready";
			m_bRenderDirty = true;
			DebugLog("confirm request aborted: request bridge missing");
			return;
		}

		m_bAwaitingServer = true;
		m_sStatusText = "Placing HQ...";
		m_bRenderDirty = true;
		DebugLog(string.Format("confirm request %1", m_vCandidatePosition));
		request.RequestSetupConfirmPosition(m_vCandidatePosition[0], m_vCandidatePosition[2]);
	}

	protected void RenderSetupMap()
	{
		m_bRenderDirty = false;
		if (!m_bSetupActive)
		{
			ClearWidgets();
			return;
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		m_iRenderCount++;
		if (m_iRenderCount <= 3 || (m_iRenderCount % 20) == 0)
			DebugLog(string.Format("render #%1 screen=%2x%3 commander=%4 confirm=%5 awaiting=%6 zones=%7", m_iRenderCount, m_iScreenW, m_iScreenH, m_bIsCommander, m_bConfirmOpen, m_bAwaitingServer, m_aZoneIds.Count()));

		ClearWidgets();
		m_iScreenW = Math.Max(1, Math.Round(workspace.GetWidth()));
		m_iScreenH = Math.Max(1, Math.Round(workspace.GetHeight()));
		float sx = m_iScreenW / 1920.0;
		float sy = m_iScreenH / 1080.0;
		m_fScale = Math.Clamp(Math.Min(sx, sy), 0.70, 1.15);

		Widget root = workspace.CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID, 0, 0, m_iScreenW, m_iScreenH, WidgetFlags.VISIBLE, null, SETUP_Z_ORDER);
		if (!root)
			return;

		root.SetZOrder(SETUP_Z_ORDER);
		m_aWidgets.Insert(root);
		CreateRectWidget(workspace, root, 0, 0, m_iScreenW, m_iScreenH, 0xF214252C, 1.0, 0);
		CreateRectWidget(workspace, root, 0, 0, m_iScreenW, m_iScreenH, 0x01000000, 0.01, ROOT_SHIELD_WIDGET_ID);

		int promptHeight = ScalePx(76);
		CreateRectWidget(workspace, root, 0, 0, m_iScreenW, promptHeight, 0xF20D1318, 1.0, 0);
		CreateRectWidget(workspace, root, 0, promptHeight - ScalePx(4), m_iScreenW, ScalePx(4), 0xFFC4953B, 1.0, 0);
		string prompt = m_sStatusText;
		if (m_bSetupActive && m_bHasAuthoritativeSetupState && !m_bIsCommander)
			prompt = "Please wait, the commander is selecting the HQ location...";
		else if (m_bSetupActive && m_sStatusText.IsEmpty())
			prompt = "Select a location on the map to place the HQ";
		CreateWrappedTextWidget(workspace, root, prompt, ScalePx(36), ScalePx(18), m_iScreenW - ScalePx(72), ScalePx(42), ScaleFont(22), 0xFFF2E6CA, 0, true);

		BuildMapLayout(promptHeight);
		RenderMapPanel(workspace, root);
		if (m_bConfirmOpen)
			RenderConfirmationModal(workspace, root);
	}

	protected void BuildMapLayout(int promptHeight)
	{
		int margin = ScalePx(34);
		m_iMapLeft = margin;
		m_iMapTop = promptHeight + margin;
		m_iMapWidth = Math.Max(1, m_iScreenW - margin * 2);
		m_iMapHeight = Math.Max(1, m_iScreenH - m_iMapTop - margin);
	}

	protected void RenderMapPanel(WorkspaceWidget workspace, Widget root)
	{
		CreateRectWidget(workspace, root, m_iMapLeft, m_iMapTop, m_iMapWidth, m_iMapHeight, 0xFF182B33, 1.0, 0);
		CreateGrid(workspace, root);
		for (int i = 0; i < m_aZoneLabels.Count(); i++)
			CreateZoneMarker(workspace, root, i);

		if (m_bCandidateValid || m_bAwaitingServer)
			CreateCandidateMarker(workspace, root);

		CreateRectWidget(workspace, root, m_iMapLeft, m_iMapTop, m_iMapWidth, m_iMapHeight, 0x01000000, 0.01, MAP_WIDGET_ID);
	}

	protected void CreateGrid(WorkspaceWidget workspace, Widget root)
	{
		int gridColor = 0x234C6970;
		int lineSize = Math.Max(1, ScalePx(1));
		for (int i = 1; i < 8; i++)
		{
			int x = m_iMapLeft + Math.Round((m_iMapWidth * i) / 8.0);
			int y = m_iMapTop + Math.Round((m_iMapHeight * i) / 8.0);
			CreateRectWidget(workspace, root, x, m_iMapTop, lineSize, m_iMapHeight, gridColor, 1.0, 0);
			CreateRectWidget(workspace, root, m_iMapLeft, y, m_iMapWidth, lineSize, gridColor, 1.0, 0);
		}
	}

	protected void CreateZoneMarker(WorkspaceWidget workspace, Widget root, int index)
	{
		float worldX = m_aZoneXs[index];
		float worldZ = m_aZoneZs[index];
		int centerX = WorldToScreenX(worldX);
		int centerY = WorldToScreenY(worldZ);
		float mapMetersWide = Math.Max(1.0, m_vWorldMax[0] - m_vWorldMin[0]);
		float radiusPxFloat = (m_aZoneRadii[index] / mapMetersWide) * m_iMapWidth;
		int radiusPx = Math.Max(4, Math.Round(radiusPxFloat));
		int color = ZoneFillColor(m_aZoneTones[index]);
		CreateCircleWidget(workspace, root, centerX - radiusPx, centerY - radiusPx, radiusPx * 2, color, 0.44);
		CreateRectWidget(workspace, root, centerX - ScalePx(3), centerY - ScalePx(3), ScalePx(6), ScalePx(6), ZonePointColor(m_aZoneTones[index]), 1.0, 0);

		if (radiusPx >= ScalePx(9))
			CreateWrappedTextWidget(workspace, root, ShortenText(m_aZoneLabels[index], 24), centerX + ScalePx(6), centerY - ScalePx(8), ScalePx(160), ScalePx(32), ScaleFont(11), 0xFFE5ECEE, 0, false);
	}

	protected void CreateCandidateMarker(WorkspaceWidget workspace, Widget root)
	{
		int centerX = WorldToScreenX(m_vCandidatePosition[0]);
		int centerY = WorldToScreenY(m_vCandidatePosition[2]);
		int size = ScalePx(22);
		int color = 0xFFFFD166;
		if (m_bAwaitingServer)
			color = 0xFF9FD3FF;

		CreateRectWidget(workspace, root, centerX - size / 2, centerY - ScalePx(2), size, ScalePx(4), color, 1.0, 0);
		CreateRectWidget(workspace, root, centerX - ScalePx(2), centerY - size / 2, ScalePx(4), size, color, 1.0, 0);
		CreateWrappedTextWidget(workspace, root, "HQ", centerX + ScalePx(10), centerY + ScalePx(8), ScalePx(48), ScalePx(20), ScaleFont(12), color, 0, true);
	}

	protected void RenderConfirmationModal(WorkspaceWidget workspace, Widget root)
	{
		int modalW = Math.Min(ScalePx(560), m_iScreenW - ScalePx(80));
		int modalH = ScalePx(210);
		int left = (m_iScreenW - modalW) / 2;
		int top = (m_iScreenH - modalH) / 2;
		CreateRectWidget(workspace, root, left - ScalePx(16), top - ScalePx(16), modalW + ScalePx(32), modalH + ScalePx(32), 0xC8000000, 1.0, 0);
		CreateRectWidget(workspace, root, left, top, modalW, modalH, 0xFF1D2932, 1.0, 0);
		CreateRectWidget(workspace, root, left, top, modalW, ScalePx(4), 0xFFC4953B, 1.0, 0);
		CreateWrappedTextWidget(workspace, root, "Are you sure you want to place the HQ here?", left + ScalePx(34), top + ScalePx(34), modalW - ScalePx(68), ScalePx(72), ScaleFont(22), 0xFFF2E6CA, 0, true);
		int buttonW = ScalePx(160);
		int buttonH = ScalePx(50);
		int buttonTop = top + modalH - ScalePx(74);
		int noLeft = left + modalW / 2 - buttonW - ScalePx(12);
		int yesLeft = left + modalW / 2 + ScalePx(12);
		CreateRectWidget(workspace, root, noLeft, buttonTop, buttonW, buttonH, 0xFF46535D, 1.0, CONFIRM_NO_WIDGET_ID);
		CreateTextWidget(workspace, root, "No", noLeft + ScalePx(20), buttonTop + ScalePx(13), buttonW - ScalePx(40), ScalePx(26), ScaleFont(17), 0xFFF4EBD6, CONFIRM_NO_WIDGET_ID, true);
		CreateRectWidget(workspace, root, yesLeft, buttonTop, buttonW, buttonH, 0xFFC4953B, 1.0, CONFIRM_YES_WIDGET_ID);
		CreateTextWidget(workspace, root, "Yes", yesLeft + ScalePx(20), buttonTop + ScalePx(13), buttonW - ScalePx(40), ScalePx(26), ScaleFont(17), 0xFF111820, CONFIRM_YES_WIDGET_ID, true);
	}

	protected Widget CreateRectWidget(WorkspaceWidget workspace, Widget parent, int left, int top, int width, int height, int color, float opacity, int userId)
	{
		Widget widget = workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE, null, SETUP_Z_ORDER + 1, parent);
		if (!widget)
			return null;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		SetupPolygonWidget(widget, BuildRectVertices(width, height), color);
		widget.SetOpacity(opacity);
		if (userId > 0)
		{
			widget.SetUserID(userId);
			widget.AddHandler(m_WidgetHandler);
		}

		return widget;
	}

	protected Widget CreateCircleWidget(WorkspaceWidget workspace, Widget parent, int left, int top, int diameter, int color, float opacity)
	{
		Widget widget = workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE, null, SETUP_Z_ORDER + 1, parent);
		if (!widget)
			return null;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, diameter, diameter);
		SetupPolygonWidget(widget, BuildCircleVertices(diameter), color);
		widget.SetOpacity(opacity);
		return widget;
	}

	protected bool SetupPolygonWidget(Widget widget, array<float> vertices, int color)
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
		m_aCanvasCommandSets.Insert(commandSet);
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

	protected TextWidget CreateTextWidget(WorkspaceWidget workspace, Widget parent, string text, int left, int top, int width, int height, int fontSize, int color, int userId, bool bold)
	{
		Widget widget = workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NO_LOCALIZATION, null, SETUP_Z_ORDER + 2, parent);
		if (!widget)
			return null;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		TextWidget textWidget = TextWidget.Cast(widget);
		if (textWidget)
		{
			textWidget.SetText(text);
			textWidget.SetTextWrapping(false);
			ApplyTextStyle(textWidget, fontSize, bold);
		}

		widget.SetColorInt(color);
		if (userId > 0)
		{
			widget.SetUserID(userId);
			widget.AddHandler(m_WidgetHandler);
		}

		return textWidget;
	}

	protected TextWidget CreateWrappedTextWidget(WorkspaceWidget workspace, Widget parent, string text, int left, int top, int width, int height, int fontSize, int color, int userId, bool bold)
	{
		Widget widget = workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NO_LOCALIZATION, null, SETUP_Z_ORDER + 2, parent);
		if (!widget)
			return null;

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

		return textWidget;
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

	protected int WorldToScreenX(float worldX)
	{
		float span = Math.Max(1.0, m_vWorldMax[0] - m_vWorldMin[0]);
		float normalized = Math.Clamp((worldX - m_vWorldMin[0]) / span, 0.0, 1.0);
		return m_iMapLeft + Math.Round(normalized * m_iMapWidth);
	}

	protected int WorldToScreenY(float worldZ)
	{
		float span = Math.Max(1.0, m_vWorldMax[2] - m_vWorldMin[2]);
		float normalized = Math.Clamp((worldZ - m_vWorldMin[2]) / span, 0.0, 1.0);
		return m_iMapTop + Math.Round((1.0 - normalized) * m_iMapHeight);
	}

	protected vector ScreenToWorldPosition(int x, int y)
	{
		vector position = "0 0 0";
		float nx = Math.Clamp(x / Math.Max(1.0, m_iMapWidth), 0.0, 1.0);
		float ny = Math.Clamp(y / Math.Max(1.0, m_iMapHeight), 0.0, 1.0);
		position[0] = m_vWorldMin[0] + nx * (m_vWorldMax[0] - m_vWorldMin[0]);
		position[2] = m_vWorldMin[2] + (1.0 - ny) * (m_vWorldMax[2] - m_vWorldMin[2]);
		return position;
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
		m_aCanvasCommandSets.Clear();
	}

	protected void CloseCommandMenuIfOpen()
	{
		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (menu)
			menu.CloseMenuFromExternal();
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
			m_bHasAuthoritativeSetupState = false;
		}

		if (!status.IsEmpty())
			m_sStatusText = status;

		m_bRenderDirty = true;
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
		DebugLog(string.Format("heartbeat active=%1 phase=%2 commander=%3 requests=%4 payloads=%5 results=%6 widgets=%7 status=%8", m_bSetupActive, m_sPhase, m_bIsCommander, m_iSetupStateRequestCount, m_iSetupPayloadCount, m_iSetupResultCount, m_aWidgets.Count(), ShortenText(m_sStatusText, 96)));
	}

	protected bool IsLocalOwner(IEntity owner)
	{
		if (!owner)
			return false;

		BaseRplComponent rpl = BaseRplComponent.Cast(owner.FindComponent(BaseRplComponent));
		return !rpl || rpl.IsOwner();
	}
}
