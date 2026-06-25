class HST_MapZoneOverlayRecord
{
	string m_sId;
	string m_sLabel;
	vector m_vWorldPosition;
	float m_fRadiusMeters;
	string m_sTone;
	bool m_bVisible = true;
}

class HST_MapZoneOverlayDrawCommandSet
{
	ref array<ref CanvasWidgetCommand> m_aCommands = {};
}

[BaseContainerProps()]
class HST_MapZoneOverlayUIComponent : SCR_MapUIBaseComponent
{
	static const float VIEWPORT_PAN_EPSILON = 0.5;
	static const float VIEWPORT_ZOOM_EPSILON = 0.001;

	protected static ref array<ref HST_MapZoneOverlayRecord> s_aZones = {};
	protected static bool s_bCandidateVisible;
	protected static vector s_vCandidatePosition = "0 0 0";
	protected static string s_sCandidateLabel;
	protected static int s_iCandidateColor;
	protected static bool s_bEnabled;
	protected static int s_iRevision;

	protected ref array<Widget> m_aWidgets = {};
	protected ref array<ref HST_MapZoneOverlayDrawCommandSet> m_aWidgetDrawCommandSets = {};
	protected Widget m_wMapFrame;
	protected bool m_bDirty = true;
	protected int m_iAppliedRevision = -1;
	protected bool m_bViewportStateValid;
	protected float m_fLastPanX;
	protected float m_fLastPanY;
	protected float m_fLastZoom;
	protected int m_iRedrawCount;
	protected string m_sLastDirtyReason;

	static void SetSetupZones(array<string> ids, array<string> labels, array<float> xs, array<float> zs, array<float> radii, array<string> tones)
	{
		s_aZones.Clear();
		if (!ids || !labels || !xs || !zs || !radii || !tones)
		{
			s_bEnabled = HasVisibleContent();
			s_iRevision++;
			return;
		}

		int count = labels.Count();
		for (int i = 0; i < count; i++)
		{
			if (!xs.IsIndexValid(i) || !zs.IsIndexValid(i) || !radii.IsIndexValid(i) || !tones.IsIndexValid(i))
				continue;

			HST_MapZoneOverlayRecord record = new HST_MapZoneOverlayRecord();
			if (ids.IsIndexValid(i))
				record.m_sId = ids[i];
			record.m_sLabel = labels[i];
			record.m_vWorldPosition = "0 0 0";
			record.m_vWorldPosition[0] = xs[i];
			record.m_vWorldPosition[2] = zs[i];
			record.m_fRadiusMeters = Math.Max(50.0, radii[i]);
			record.m_sTone = tones[i];
			record.m_bVisible = true;
			s_aZones.Insert(record);
		}

		s_bEnabled = HasVisibleContent();
		s_iRevision++;
	}

	static void ClearSetupZones()
	{
		s_aZones.Clear();
		s_bEnabled = HasVisibleContent();
		s_iRevision++;
	}

	static void SetSetupCandidate(vector position, string label, int color)
	{
		s_vCandidatePosition = position;
		s_sCandidateLabel = label;
		s_iCandidateColor = color;
		s_bCandidateVisible = true;
		s_bEnabled = HasVisibleContent();
		s_iRevision++;
	}

	static void ClearSetupCandidate()
	{
		s_bCandidateVisible = false;
		s_vCandidatePosition = "0 0 0";
		s_sCandidateLabel = "";
		s_iCandidateColor = 0;
		s_bEnabled = HasVisibleContent();
		s_iRevision++;
	}

	override void OnMapOpen(MapConfiguration config)
	{
		super.OnMapOpen(config);
		m_wMapFrame = config.RootWidgetRef.FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		m_MapEntity.GetOnMapPan().Insert(OnMapPan);
		m_MapEntity.GetOnMapZoom().Insert(OnMapZoom);
		m_MapEntity.GetOnMapPanEnd().Insert(OnMapPanEnd);
		m_bDirty = true;
		m_bViewportStateValid = false;
		m_iAppliedRevision = -1;
		m_iRedrawCount = 0;
	}

	override void OnMapClose(MapConfiguration config)
	{
		m_MapEntity.GetOnMapPan().Remove(OnMapPan);
		m_MapEntity.GetOnMapZoom().Remove(OnMapZoom);
		m_MapEntity.GetOnMapPanEnd().Remove(OnMapPanEnd);
		ClearOverlayWidgets();
		m_wMapFrame = null;
		m_bViewportStateValid = false;
		super.OnMapClose(config);
	}

	override void Init()
	{
		m_bDirty = true;
	}

	override void Update(float timeSlice)
	{
		if (!s_bEnabled)
		{
			if (m_aWidgets.Count() > 0)
				ClearOverlayWidgets();
			m_iAppliedRevision = s_iRevision;
			return;
		}

		if (!m_bDirty && m_iAppliedRevision == s_iRevision)
			return;

		if (!CanProject())
		{
			if (m_aWidgets.Count() > 0)
				ClearOverlayWidgets();
			return;
		}

		Redraw();
	}

	protected void OnMapPan(float panX, float panY, bool adjusted)
	{
		MarkViewportDirty(panX, panY, ResolveCurrentZoom(), "pan");
	}

	protected void OnMapPanEnd(float panX, float panY)
	{
		MarkViewportDirty(panX, panY, ResolveCurrentZoom(), "pan_end");
	}

	protected void OnMapZoom(float zoom)
	{
		MarkViewportDirty(m_fLastPanX, m_fLastPanY, zoom, "zoom");
	}

	protected void Redraw()
	{
		int widgetsBefore = m_aWidgets.Count();
		ClearOverlayWidgets();
		m_bDirty = false;
		m_iAppliedRevision = s_iRevision;

		if (!m_MapEntity || !m_MapEntity.IsOpen() || !m_RootWidget)
			return;
		if (!CanProject())
		{
			m_bDirty = true;
			return;
		}

		if (!m_wMapFrame)
			m_wMapFrame = m_RootWidget.FindAnyWidget(SCR_MapConstants.MAP_FRAME_NAME);
		if (!m_wMapFrame)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		foreach (HST_MapZoneOverlayRecord zone : s_aZones)
		{
			if (zone && zone.m_bVisible)
				DrawZone(workspace, m_wMapFrame, zone);
		}

		if (s_bCandidateVisible)
			DrawCandidate(workspace, m_wMapFrame);

		m_iRedrawCount++;
		if (m_iRedrawCount <= 3 || (m_iRedrawCount % 20) == 0)
		{
			string summary = string.Format("redraw=%1 revision=%2 widgetsBefore=%3 widgetsAfter=%4 zones=%5 candidate=%6 reason=%7", m_iRedrawCount, s_iRevision, widgetsBefore, m_aWidgets.Count(), s_aZones.Count(), s_bCandidateVisible, m_sLastDirtyReason);
			summary = summary + " " + string.Format("pan=%1,%2 zoom=%3 dirty=%4 applied=%5", Math.Round(m_fLastPanX), Math.Round(m_fLastPanY), m_fLastZoom, m_bDirty, m_iAppliedRevision);
			HST_UIDebug.LogPopulation("map_zone_overlay", summary);
		}
	}

	protected void MarkViewportDirty(float panX, float panY, float zoom, string reason)
	{
		if (zoom <= 0.0 && m_MapEntity)
			zoom = m_MapEntity.GetCurrentZoom();

		if (m_bViewportStateValid)
		{
			bool panChanged = AbsFloat(panX - m_fLastPanX) > VIEWPORT_PAN_EPSILON || AbsFloat(panY - m_fLastPanY) > VIEWPORT_PAN_EPSILON;
			bool zoomChanged = AbsFloat(zoom - m_fLastZoom) > VIEWPORT_ZOOM_EPSILON;
			if (!panChanged && !zoomChanged)
				return;
		}

		m_bViewportStateValid = true;
		m_fLastPanX = panX;
		m_fLastPanY = panY;
		m_fLastZoom = zoom;
		m_sLastDirtyReason = reason;
		m_bDirty = true;
	}

	protected float ResolveCurrentZoom()
	{
		if (!m_MapEntity)
			return m_fLastZoom;

		return m_MapEntity.GetCurrentZoom();
	}

	protected void DrawZone(WorkspaceWidget workspace, Widget parent, HST_MapZoneOverlayRecord zone)
	{
		int sx;
		int sy;
		m_MapEntity.WorldToScreen(zone.m_vWorldPosition[0], zone.m_vWorldPosition[2], sx, sy, true);

		int radiusPx = Math.Max(4, ResolveRadiusPixels(zone.m_vWorldPosition, zone.m_fRadiusMeters));
		int left = Math.Round(workspace.DPIUnscale(sx - radiusPx));
		int top = Math.Round(workspace.DPIUnscale(sy - radiusPx));
		int diameter = Math.Round(workspace.DPIUnscale(radiusPx * 2));
		if (!IsOverlayRectWorthDrawing(left, top, diameter, parent, workspace))
			return;

		CreateCircle(workspace, parent, left, top, diameter, ZoneFillColor(zone.m_sTone), 0.48);

		int centerX = Math.Round(workspace.DPIUnscale(sx));
		int centerY = Math.Round(workspace.DPIUnscale(sy));
		int pointSize = 6;
		int pointLeft = centerX - pointSize / 2;
		int pointTop = centerY - pointSize / 2;
		CreateRect(workspace, parent, pointLeft, pointTop, pointSize, pointSize, ZonePointColor(zone.m_sTone), 1.0);
		if (diameter >= 18)
			CreateLabel(workspace, parent, ShortenText(zone.m_sLabel, 24), centerX + 9, centerY - 5, 0xFFE5ECEE);
	}

	protected void DrawCandidate(WorkspaceWidget workspace, Widget parent)
	{
		int sx;
		int sy;
		m_MapEntity.WorldToScreen(s_vCandidatePosition[0], s_vCandidatePosition[2], sx, sy, true);

		int x = Math.Round(workspace.DPIUnscale(sx));
		int y = Math.Round(workspace.DPIUnscale(sy));

		int outerSize = 34;
		if (!IsOverlayRectWorthDrawing(x - outerSize / 2, y - outerSize / 2, outerSize, parent, workspace))
			return;

		int innerSize = 14;
		CreateCircle(workspace, parent, x - outerSize / 2, y - outerSize / 2, outerSize, s_iCandidateColor, 0.34);
		CreateCircle(workspace, parent, x - innerSize / 2, y - innerSize / 2, innerSize, s_iCandidateColor, 0.95);
		CreateRect(workspace, parent, x - 2, y + innerSize / 2 - 1, 4, 12, s_iCandidateColor, 0.95);
		if (!s_sCandidateLabel.IsEmpty())
			CreateLabel(workspace, parent, s_sCandidateLabel, x + 15, y - 6, s_iCandidateColor);
	}

	protected int ResolveRadiusPixels(vector center, float radiusMeters)
	{
		int cx;
		int cy;
		int rx;
		int ry;
		vector edge = center;
		edge[0] = edge[0] + radiusMeters;
		m_MapEntity.WorldToScreen(center[0], center[2], cx, cy, true);
		m_MapEntity.WorldToScreen(edge[0], edge[2], rx, ry, true);
		return AbsInt(rx - cx);
	}

	protected bool CanProject()
	{
		if (!m_MapEntity || !m_MapEntity.IsOpen())
			return false;

		CanvasWidget mapWidget = m_MapEntity.GetMapWidget();
		if (!mapWidget)
			return false;

		float width;
		float height;
		mapWidget.GetScreenSize(width, height);
		return width > 0.0 && height > 0.0 && m_MapEntity.GetCurrentZoom() > 0.0 && mapWidget.PixelPerUnit() > 0.0;
	}

	protected bool IsOverlayRectWorthDrawing(int left, int top, int size, Widget mapFrame, WorkspaceWidget workspace)
	{
		if (!mapFrame || !workspace || size <= 0)
			return false;

		float frameW;
		float frameH;
		mapFrame.GetScreenSize(frameW, frameH);
		frameW = workspace.DPIUnscale(frameW);
		frameH = workspace.DPIUnscale(frameH);
		if (left + size < -128 || top + size < -128)
			return false;
		if (left > frameW + 128 || top > frameH + 128)
			return false;

		return true;
	}

	protected Widget CreateCircle(WorkspaceWidget workspace, Widget parent, int left, int top, int diameter, int color, float opacity)
	{
		Widget widget = workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS, null, 1, parent);
		if (!widget)
			return null;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, diameter, diameter);
		SetupPolygonWidget(widget, BuildCircleVertices(diameter * 0.5, 56), color);
		widget.SetOpacity(opacity);
		m_aWidgets.Insert(widget);
		return widget;
	}

	protected Widget CreateRect(WorkspaceWidget workspace, Widget parent, int left, int top, int width, int height, int color, float opacity)
	{
		Widget widget = workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS, null, 2, parent);
		if (!widget)
			return null;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		SetupPolygonWidget(widget, BuildRectVertices(width, height), color);
		widget.SetOpacity(opacity);
		m_aWidgets.Insert(widget);
		return widget;
	}

	protected TextWidget CreateLabel(WorkspaceWidget workspace, Widget parent, string text, int left, int top, int color)
	{
		Widget widget = workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NO_LOCALIZATION | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS, null, 3, parent);
		if (!widget)
			return null;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, 160, 32);
		TextWidget textWidget = TextWidget.Cast(widget);
		if (textWidget)
		{
			textWidget.SetText(text);
			textWidget.SetTextWrapping(true);
			textWidget.SetExactFontSize(11);
			textWidget.SetOutline(1, 0xDD000000);
			textWidget.SetShadow(2, 0xEE000000, 1, 1, 1);
		}

		widget.SetColorInt(color);
		m_aWidgets.Insert(widget);
		return textWidget;
	}

	protected bool SetupPolygonWidget(Widget widget, array<float> vertices, int color)
	{
		CanvasWidget canvas = CanvasWidget.Cast(widget);
		if (!canvas)
			return false;

		HST_MapZoneOverlayDrawCommandSet commandSet = new HST_MapZoneOverlayDrawCommandSet();
		PolygonDrawCommand command = new PolygonDrawCommand();
		command.m_iColor = color;
		command.m_Vertices = vertices;
		commandSet.m_aCommands.Insert(command);
		canvas.SetDrawCommands(commandSet.m_aCommands);
		m_aWidgetDrawCommandSets.Insert(commandSet);
		return true;
	}

	protected ref array<float> BuildCircleVertices(float radius, int segments)
	{
		ref array<float> vertices = {};
		if (radius <= 0.0 || segments < 8)
			return vertices;

		for (int i = 0; i < segments; i++)
		{
			float angle = (i * 360.0 / segments) * Math.DEG2RAD;
			vertices.Insert(radius + Math.Cos(angle) * radius);
			vertices.Insert(radius + Math.Sin(angle) * radius);
		}

		return vertices;
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

	protected void ClearOverlayWidgets()
	{
		foreach (Widget widget : m_aWidgets)
		{
			if (widget)
				widget.RemoveFromHierarchy();
		}

		m_aWidgets.Clear();
		m_aWidgetDrawCommandSets.Clear();
	}

	protected int ZoneFillColor(string tone)
	{
		if (tone == "resistance")
			return 0x8A226B3C;
		if (tone == "town")
			return 0x8A3F7397;
		if (tone == "major")
			return 0x9A8D3023;

		return 0x8A95601F;
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

	protected static bool HasVisibleContent()
	{
		return s_bCandidateVisible || s_aZones.Count() > 0;
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
}
