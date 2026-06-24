class HST_MissionClientDrawCommandSet
{
	ref array<ref CanvasWidgetCommand> m_aCommands = {};
}

[ComponentEditorProps(category: "h-istasi", description: "Client mission notifications, intel cache, and detail panel")]
class HST_MissionClientComponentClass : ScriptComponentClass
{
}

class HST_MissionClientComponent : ScriptComponent
{
	static const int DETAIL_CLOSE_WIDGET_ID = 9801;
	static const int DETAIL_ROOT_Z = 2700;

	protected static HST_MissionClientComponent s_LocalInstance;

	protected IEntity m_OwnerEntity;
	protected bool m_bIsLocalOwner;
	protected string m_sLastIntelPayload;
	protected string m_sLastEventPayload;
	protected string m_sLastSummary;
	protected string m_sSelectedMissionId;
	protected bool m_bDetailOpen;
	protected float m_fNotificationRemaining;
	protected ref array<string> m_aRecentNotificationIds = {};
	protected ref array<float> m_aRecentNotificationRemaining = {};
	protected ref array<string> m_aQueuedNotificationIds = {};
	protected ref array<string> m_aQueuedNotificationCategories = {};
	protected ref array<string> m_aQueuedNotificationSeverities = {};
	protected ref array<string> m_aQueuedNotificationTitles = {};
	protected ref array<string> m_aQueuedNotificationMessages = {};
	protected ref array<float> m_aQueuedNotificationDurations = {};
	protected ref array<Widget> m_aWidgets = {};
	protected ref array<ref HST_MissionClientDrawCommandSet> m_aCanvasCommandSets = {};
	protected ref HST_MissionClientWidgetHandler m_WidgetHandler;
	protected ref array<string> m_aMissionIds = {};
	protected ref array<string> m_aMissionTitles = {};
	protected ref array<string> m_aMissionStatuses = {};
	protected ref array<string> m_aMissionCategories = {};
	protected ref array<string> m_aMissionZones = {};
	protected ref array<string> m_aMissionSites = {};
	protected ref array<string> m_aMissionPositions = {};
	protected ref array<string> m_aMissionRemaining = {};
	protected ref array<string> m_aMissionRequirements = {};
	protected ref array<string> m_aMissionProgress = {};
	protected ref array<string> m_aMissionRewards = {};
	protected ref array<string> m_aMissionFailures = {};
	protected ref array<string> m_aMissionMarkerIds = {};
	protected ref array<string> m_aObjectiveMissionIds = {};
	protected ref array<string> m_aObjectiveLabels = {};
	protected ref array<string> m_aObjectiveProgress = {};

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		m_bIsLocalOwner = IsLocalOwner(owner);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);
		if (m_bIsLocalOwner)
			BecomeLocalOwner();
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
		if (!m_bIsLocalOwner && IsLocalOwner(owner))
		{
			m_bIsLocalOwner = true;
			BecomeLocalOwner();
		}
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bIsLocalOwner)
		{
			if (IsLocalOwner(owner))
			{
				m_bIsLocalOwner = true;
				BecomeLocalOwner();
			}

			return;
		}

		if (m_fNotificationRemaining > 0)
		{
			m_fNotificationRemaining = Math.Max(0, m_fNotificationRemaining - timeSlice);
			if (m_fNotificationRemaining == 0 && !m_bDetailOpen)
			{
				ClearWidgets();
				ShowNextQueuedNotification();
			}
		}
		else if (!m_bDetailOpen && m_aQueuedNotificationIds.Count() > 0)
		{
			ShowNextQueuedNotification();
		}

		for (int i = m_aRecentNotificationIds.Count() - 1; i >= 0; i--)
		{
			if (i >= m_aRecentNotificationRemaining.Count())
			{
				m_aRecentNotificationIds.Remove(i);
				continue;
			}

			m_aRecentNotificationRemaining[i] = Math.Max(0, m_aRecentNotificationRemaining[i] - timeSlice);
			if (m_aRecentNotificationRemaining[i] <= 0)
			{
				m_aRecentNotificationIds.Remove(i);
				m_aRecentNotificationRemaining.Remove(i);
			}
		}
	}

	static HST_MissionClientComponent GetLocalInstance()
	{
		return s_LocalInstance;
	}

	void OnServerMissionEvent(string payload, string summary)
	{
		m_sLastEventPayload = payload;
		m_sLastSummary = summary;
		string eventType = ExtractPipeField(payload, 1);
		string missionId = ExtractPipeField(payload, 2);
		string title = ExtractPipeField(payload, 3);
		string message = summary;
		if (title.IsEmpty())
			title = "Mission";
		ShowTopMissionNotification("mission_" + eventType + "_" + missionId, "mission", MissionSeverity(eventType), MissionEventTitle(eventType), message, 5.0);
		RequestMissionIntel();
	}

	void OnServerNotification(string payload, string summary)
	{
		if (payload.IsEmpty() || !payload.Contains("HST_NOTIFICATION|"))
		{
			ShowTopMissionNotification(summary, "h-istasi", "info", "h-istasi", summary, 5.0);
			return;
		}

		string eventId = ExtractPipeField(payload, 1);
		string category = ExtractPipeField(payload, 2);
		string severity = ExtractPipeField(payload, 3);
		string title = ExtractPipeField(payload, 4);
		string message = ExtractPipeField(payload, 5);
		float duration = ExtractPipeField(payload, 9).ToFloat();
		if (duration <= 0)
			duration = 5.0;
		if (eventId.IsEmpty())
			eventId = category + "_" + title + "_" + message;
		if (title.IsEmpty())
			title = "h-istasi";
		if (message.IsEmpty())
			message = summary;

		ShowTopMissionNotification(eventId, category, severity, title, message, duration);
	}

	void OnServerMissionIntel(string payload)
	{
		m_sLastIntelPayload = payload;
		ParseMissionIntel(payload);
		if (m_bDetailOpen)
			RenderMissionDetailPanel();
	}

	bool OpenMissionDetailsForMarker(string markerId)
	{
		int index = m_aMissionMarkerIds.Find(markerId);
		if (index < 0)
			index = m_aMissionIds.Find(markerId);
		if (index < 0)
			return false;

		m_sSelectedMissionId = m_aMissionIds[index];
		m_bDetailOpen = true;
		RenderMissionDetailPanel();
		return true;
	}

	bool OpenMissionDetails(string missionId)
	{
		if (m_aMissionIds.Find(missionId) < 0)
			return false;

		m_sSelectedMissionId = missionId;
		m_bDetailOpen = true;
		RenderMissionDetailPanel();
		return true;
	}

	// Fallback entry for custom map-overlay hit tests when native marker click callbacks are not available.
	bool HandleDirectMissionMarkerClick(string markerId)
	{
		return OpenMissionDetailsForMarker(markerId);
	}

	protected void BecomeLocalOwner()
	{
		s_LocalInstance = this;
		RequestMissionIntel();
	}

	protected void RequestMissionIntel()
	{
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (request)
			request.RequestMissionIntel();
	}

	protected void ShowTopMissionNotification(string eventId, string category, string severity, string title, string message, float durationSeconds)
	{
		if (!eventId.IsEmpty() && FindRecentNotification(eventId) >= 0)
			return;

		durationSeconds = Math.Max(1.0, durationSeconds);
		TrackRecentNotification(eventId, Math.Max(durationSeconds, 5.0));
		if (m_fNotificationRemaining > 0 || m_bDetailOpen)
		{
			QueueTopMissionNotification(eventId, category, severity, title, message, durationSeconds);
			return;
		}

		RenderTopMissionNotification(eventId, category, severity, title, message, durationSeconds);
	}

	protected void QueueTopMissionNotification(string eventId, string category, string severity, string title, string message, float durationSeconds)
	{
		if (!eventId.IsEmpty() && m_aQueuedNotificationIds.Find(eventId) >= 0)
			return;

		m_aQueuedNotificationIds.Insert(eventId);
		m_aQueuedNotificationCategories.Insert(category);
		m_aQueuedNotificationSeverities.Insert(severity);
		m_aQueuedNotificationTitles.Insert(title);
		m_aQueuedNotificationMessages.Insert(message);
		m_aQueuedNotificationDurations.Insert(durationSeconds);

		while (m_aQueuedNotificationIds.Count() > 16)
		{
			m_aQueuedNotificationIds.Remove(0);
			m_aQueuedNotificationCategories.Remove(0);
			m_aQueuedNotificationSeverities.Remove(0);
			m_aQueuedNotificationTitles.Remove(0);
			m_aQueuedNotificationMessages.Remove(0);
			m_aQueuedNotificationDurations.Remove(0);
		}
	}

	protected void ShowNextQueuedNotification()
	{
		if (m_aQueuedNotificationIds.Count() == 0 || m_bDetailOpen)
			return;

		string eventId = m_aQueuedNotificationIds[0];
		string category = m_aQueuedNotificationCategories[0];
		string severity = m_aQueuedNotificationSeverities[0];
		string title = m_aQueuedNotificationTitles[0];
		string message = m_aQueuedNotificationMessages[0];
		float duration = m_aQueuedNotificationDurations[0];

		m_aQueuedNotificationIds.Remove(0);
		m_aQueuedNotificationCategories.Remove(0);
		m_aQueuedNotificationSeverities.Remove(0);
		m_aQueuedNotificationTitles.Remove(0);
		m_aQueuedNotificationMessages.Remove(0);
		m_aQueuedNotificationDurations.Remove(0);

		RenderTopMissionNotification(eventId, category, severity, title, message, duration);
	}

	protected void RenderTopMissionNotification(string eventId, string category, string severity, string title, string message, float durationSeconds)
	{
		durationSeconds = Math.Max(1.0, durationSeconds);

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		ClearWidgets();
		Widget root = workspace.CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID, 510, 24, 900, 92, WidgetFlags.VISIBLE, null, DETAIL_ROOT_Z);
		if (!root)
			return;

		m_aWidgets.Insert(root);
		int accent = NotificationAccentColor(severity, category);
		CreateRectWidget(workspace, root, 0, 0, 900, 92, 0xF21A232B, 1.0, 0);
		CreateRectWidget(workspace, root, 0, 88, 900, 4, accent, 1.0, 0);
		CreateTextWidget(workspace, root, ShortenText(title, 44), 24, 10, 380, 24, 18, accent, 0, true);
		CreateTextWidget(workspace, root, ShortenText(message, 140), 24, 38, 852, 42, 16, 0xFFF2F4F0, 0, false);
		m_fNotificationRemaining = durationSeconds;
	}

	protected string MissionEventTitle(string eventType)
	{
		if (eventType == "created")
			return "Mission Available";
		if (eventType == "completed")
			return "Mission Complete";
		if (eventType == "failed")
			return "Mission Failed";
		if (eventType == "expired")
			return "Mission Expired";
		if (eventType == "loaded")
			return "Cargo Loaded";
		if (eventType == "unloaded")
			return "Cargo Unloaded";
		if (eventType == "delivered")
			return "Delivery Complete";
		if (eventType == "captured")
			return "Asset Captured";
		if (eventType == "destroyed")
			return "Target Destroyed";

		return "Mission Update";
	}

	protected string MissionSeverity(string eventType)
	{
		if (eventType == "failed" || eventType == "expired")
			return "danger";
		if (eventType == "completed" || eventType == "delivered" || eventType == "captured")
			return "success";
		if (eventType == "created")
			return "info";

		return "warning";
	}

	protected int NotificationAccentColor(string severity, string category)
	{
		if (severity == "danger" || severity == "error")
			return 0xFFFF6B5C;
		if (severity == "success" || severity == "good")
			return 0xFF73D17C;
		if (severity == "warning" || severity == "warn")
			return 0xFFFFC45D;
		if (category == "enemy")
			return 0xFFFF8A5C;
		if (category == "vehicle" || category == "garage")
			return 0xFF8CC8FF;

		return 0xFFFFD98B;
	}

	protected int FindRecentNotification(string eventId)
	{
		if (eventId.IsEmpty())
			return -1;

		for (int i = 0; i < m_aRecentNotificationIds.Count(); i++)
		{
			if (m_aRecentNotificationIds[i] == eventId)
				return i;
		}

		return -1;
	}

	protected void TrackRecentNotification(string eventId, float durationSeconds)
	{
		if (eventId.IsEmpty())
			return;

		int existing = FindRecentNotification(eventId);
		if (existing >= 0)
		{
			if (existing < m_aRecentNotificationRemaining.Count())
				m_aRecentNotificationRemaining[existing] = durationSeconds;
			return;
		}

		m_aRecentNotificationIds.Insert(eventId);
		m_aRecentNotificationRemaining.Insert(durationSeconds);
	}

	protected void RenderMissionDetailPanel()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		ClearWidgets();
		int index = m_aMissionIds.Find(m_sSelectedMissionId);
		if (index < 0)
			return;

		Widget root = workspace.CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID, 300, 132, 920, 560, WidgetFlags.VISIBLE, null, DETAIL_ROOT_Z);
		if (!root)
			return;

		m_aWidgets.Insert(root);
		if (!m_WidgetHandler)
		{
			m_WidgetHandler = new HST_MissionClientWidgetHandler();
			m_WidgetHandler.Bind(this);
		}

		CreateRectWidget(workspace, root, 0, 0, 920, 560, 0xF4080D12, 1.0, 0);
		CreateRectWidget(workspace, root, 0, 0, 920, 5, 0xFFFFC45D, 1.0, 0);
		CreateTextWidget(workspace, root, ShortenText(m_aMissionTitles[index], 48), 28, 22, 520, 34, 26, 0xFFF7E6BE, 0, true);
		CreateTextWidget(workspace, root, string.Format("%1 | %2 | %3s left", m_aMissionCategories[index], m_aMissionStatuses[index], m_aMissionRemaining[index]), 28, 64, 620, 26, 17, 0xFFC9D4DC, 0, false);
		CreateTextWidget(workspace, root, "Close", 806, 28, 86, 24, 17, 0xFFF7E6BE, DETAIL_CLOSE_WIDGET_ID, true);
		CreateRectWidget(workspace, root, 790, 20, 110, 42, 0xFF604A24, 0.95, DETAIL_CLOSE_WIDGET_ID);

		CreatePanelRow(workspace, root, "Location", m_aMissionZones[index] + " / " + m_aMissionSites[index], 30, 112);
		CreatePanelRow(workspace, root, "Map position", m_aMissionPositions[index], 30, 154);
		CreatePanelRow(workspace, root, "Requirement", m_aMissionRequirements[index], 30, 196);
		CreatePanelRow(workspace, root, "Progress", m_aMissionProgress[index], 30, 258);
		CreatePanelRow(workspace, root, "Reward", m_aMissionRewards[index], 30, 320);
		CreatePanelRow(workspace, root, "Failure", m_aMissionFailures[index], 30, 382);

		CreateTextWidget(workspace, root, "Objectives", 560, 112, 220, 28, 22, 0xFFF7E6BE, 0, true);
		int rendered;
		for (int i = 0; i < m_aObjectiveMissionIds.Count(); i++)
		{
			if (m_aObjectiveMissionIds[i] != m_sSelectedMissionId)
				continue;

			CreateTextWidget(workspace, root, ShortenText(m_aObjectiveLabels[i], 28), 560, 154 + rendered * 44, 240, 22, 17, 0xFFF2F4F0, 0, true);
			CreateTextWidget(workspace, root, ShortenText(m_aObjectiveProgress[i], 24), 560, 178 + rendered * 44, 240, 20, 15, 0xFFFFD98B, 0, false);
			rendered++;
			if (rendered >= 8)
				break;
		}

		if (rendered == 0)
			CreateTextWidget(workspace, root, "No objective records.", 560, 154, 260, 26, 16, 0xFF9AA5AD, 0, false);
	}

	bool OnWidgetClicked(int widgetId)
	{
		if (widgetId != DETAIL_CLOSE_WIDGET_ID)
			return false;

		m_bDetailOpen = false;
		ClearWidgets();
		return true;
	}

	protected void CreatePanelRow(WorkspaceWidget workspace, Widget root, string label, string value, int left, int top)
	{
		CreateTextWidget(workspace, root, label, left, top, 150, 24, 16, 0xFFFFD98B, 0, true);
		CreateTextWidget(workspace, root, ShortenText(value, 76), left + 150, top, 360, 46, 16, 0xFFE7EDF1, 0, false);
	}

	protected Widget CreateRectWidget(WorkspaceWidget workspace, Widget parent, int left, int top, int width, int height, int color, float opacity, int userId)
	{
		Widget widget = workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE, null, DETAIL_ROOT_Z + 5, parent);
		if (!widget)
			return null;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		SetupCanvasRect(widget, width, height, color);
		widget.SetOpacity(opacity);
		if (userId > 0)
		{
			widget.SetUserID(userId);
			widget.AddHandler(m_WidgetHandler);
		}
		return widget;
	}

	protected TextWidget CreateTextWidget(WorkspaceWidget workspace, Widget parent, string text, int left, int top, int width, int height, int fontSize, int color, int userId, bool bold)
	{
		Widget widget = workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NO_LOCALIZATION, null, DETAIL_ROOT_Z + 10, parent);
		if (!widget)
			return null;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		TextWidget textWidget = TextWidget.Cast(widget);
		if (textWidget)
		{
			textWidget.SetText(text);
			textWidget.SetExactFontSize(fontSize);
			textWidget.SetBold(bold);
			textWidget.SetTextWrapping(true);
			textWidget.SetOutline(1, 0xDD000000);
			textWidget.SetShadow(2, 0xEE000000, 1, 1, 1);
		}

		widget.SetColorInt(color);
		if (userId > 0)
		{
			widget.SetUserID(userId);
			widget.AddHandler(m_WidgetHandler);
		}

		return textWidget;
	}

	protected bool SetupCanvasRect(Widget widget, int width, int height, int color)
	{
		CanvasWidget canvas = CanvasWidget.Cast(widget);
		if (!canvas)
			return false;

		HST_MissionClientDrawCommandSet commandSet = new HST_MissionClientDrawCommandSet();
		PolygonDrawCommand rectCommand = new PolygonDrawCommand();
		rectCommand.m_iColor = color;
		rectCommand.m_Vertices = BuildRectVertices(width, height);
		commandSet.m_aCommands.Insert(rectCommand);
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

	protected void ParseMissionIntel(string payload)
	{
		ClearMissionIntel();
		if (payload.IsEmpty() || !payload.Contains("MISSION|"))
			return;

		int cursor;
		while (true)
		{
			int lineStart = payload.IndexOfFrom(cursor, "MISSION|");
			if (lineStart < 0)
				break;
			int lineEnd = payload.IndexOfFrom(lineStart, "\n");
			if (lineEnd < 0)
				lineEnd = payload.Length();

			string line = payload.Substring(lineStart, lineEnd - lineStart);
			m_aMissionIds.Insert(ExtractPipeField(line, 1));
			m_aMissionTitles.Insert(ExtractPipeField(line, 2));
			m_aMissionStatuses.Insert(ExtractPipeField(line, 3));
			m_aMissionCategories.Insert(ExtractPipeField(line, 4));
			m_aMissionZones.Insert(ExtractPipeField(line, 5));
			m_aMissionSites.Insert(ExtractPipeField(line, 6));
			m_aMissionPositions.Insert(ExtractPipeField(line, 7));
			m_aMissionRemaining.Insert(ExtractPipeField(line, 8));
			m_aMissionRequirements.Insert(ExtractPipeField(line, 9));
			m_aMissionProgress.Insert(ExtractPipeField(line, 10));
			m_aMissionRewards.Insert(ExtractPipeField(line, 11));
			m_aMissionFailures.Insert(ExtractPipeField(line, 12));
			m_aMissionMarkerIds.Insert(ExtractPipeField(line, 13));
			cursor = lineEnd + 1;
		}

		cursor = 0;
		while (true)
		{
			int lineStart = payload.IndexOfFrom(cursor, "OBJECTIVE|");
			if (lineStart < 0)
				break;
			int lineEnd = payload.IndexOfFrom(lineStart, "\n");
			if (lineEnd < 0)
				lineEnd = payload.Length();

			string line = payload.Substring(lineStart, lineEnd - lineStart);
			m_aObjectiveMissionIds.Insert(ExtractPipeField(line, 1));
			m_aObjectiveLabels.Insert(ExtractPipeField(line, 3));
			m_aObjectiveProgress.Insert(string.Format("%1/%2 | complete %3 | failed %4", ExtractPipeField(line, 4), ExtractPipeField(line, 5), ExtractPipeField(line, 6), ExtractPipeField(line, 7)));
			cursor = lineEnd + 1;
		}
	}

	protected void ClearMissionIntel()
	{
		m_aMissionIds.Clear();
		m_aMissionTitles.Clear();
		m_aMissionStatuses.Clear();
		m_aMissionCategories.Clear();
		m_aMissionZones.Clear();
		m_aMissionSites.Clear();
		m_aMissionPositions.Clear();
		m_aMissionRemaining.Clear();
		m_aMissionRequirements.Clear();
		m_aMissionProgress.Clear();
		m_aMissionRewards.Clear();
		m_aMissionFailures.Clear();
		m_aMissionMarkerIds.Clear();
		m_aObjectiveMissionIds.Clear();
		m_aObjectiveLabels.Clear();
		m_aObjectiveProgress.Clear();
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

class HST_MissionClientWidgetHandler : ScriptedWidgetEventHandler
{
	protected HST_MissionClientComponent m_Client;

	void Bind(HST_MissionClientComponent client)
	{
		m_Client = client;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Client)
			return false;

		return m_Client.OnWidgetClicked(w.GetUserID());
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (!m_Client)
			return false;

		return m_Client.OnWidgetClicked(w.GetUserID());
	}
}
