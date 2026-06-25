[ComponentEditorProps(category: "h-istasi", description: "Client mission notifications, intel cache, and detail panel")]
class HST_MissionClientComponentClass : ScriptComponentClass
{
}

class HST_MissionClientComponent : ScriptComponent
{
	static const int DETAIL_CLOSE_WIDGET_ID = 9801;
	static const ResourceName REPORT_DIALOG_LAYOUT = "{D66CFA01E5AA4100}UI/layouts/HST_ReportDialog.layout";
	static const ResourceName ACTION_DIALOG_LAYOUT = "{D66CFA01E5AA4200}UI/layouts/HST_ActionDialog.layout";
	static const ResourceName REPORT_OBJECTIVE_ROW_LAYOUT = "{D66CFA01E5AA4300}UI/layouts/HST/Rows/HST_ReportObjectiveRow.layout";

	protected static HST_MissionClientComponent s_LocalInstance;

	protected IEntity m_OwnerEntity;
	protected bool m_bIsLocalOwner;
	protected string m_sLastIntelPayload;
	protected string m_sLastEventPayload;
	protected string m_sLastSummary;
	protected string m_sSelectedMissionId;
	protected bool m_bDetailOpen;
	protected ref array<string> m_aRecentNotificationIds = {};
	protected ref array<float> m_aRecentNotificationRemaining = {};
	protected ref array<Widget> m_aWidgets = {};
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

		CloseMissionDetails();
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
		if (m_bDetailOpen && !RenderMissionDetailPanel())
			CloseMissionDetails();
	}

	bool OpenMissionDetailsForMarker(string markerId)
	{
		int index = m_aMissionMarkerIds.Find(markerId);
		if (index < 0)
			index = m_aMissionIds.Find(markerId);
		if (index < 0)
			return false;

		return OpenMissionDetailsAtIndex(index);
	}

	bool OpenMissionDetails(string missionId)
	{
		int index = m_aMissionIds.Find(missionId);
		if (index < 0)
			return false;

		return OpenMissionDetailsAtIndex(index);
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
		HST_NotificationToastController.Get().Show(eventId, category, severity, title, message, durationSeconds);
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

	protected bool OpenMissionDetailsAtIndex(int index)
	{
		if (index < 0 || index >= m_aMissionIds.Count())
			return false;

		m_sSelectedMissionId = m_aMissionIds[index];
		m_bDetailOpen = true;
		if (RenderMissionDetailPanel())
			return true;

		CloseMissionDetails();
		return false;
	}

	protected bool RenderMissionDetailPanel()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		ClearWidgets();
		int index = m_aMissionIds.Find(m_sSelectedMissionId);
		if (index < 0)
			return false;

		int screenW;
		int screenH;
		HST_UIWorkspaceMetrics.GetLayoutSize(workspace, screenW, screenH);
		HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, "HST_MissionDetail");
		float scale = HST_UIWorkspaceMetrics.GetScale(screenW, screenH, 0.70, 1.12);

		Widget root = workspace.CreateWidgets(REPORT_DIALOG_LAYOUT);
		HST_UIDebug.LogLayoutCreate("mission_report_dialog", REPORT_DIALOG_LAYOUT, root);
		if (!root)
			return false;

		root.SetVisible(true);
		root.SetOpacity(1.0);
		root.SetZOrder(HST_UIConstants.Z_MISSION_DIALOG);
		if (!HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.MISSION_DIALOG, "HST_MissionClientComponent", root, false, false, true))
		{
			HST_UIDebug.LogLayoutRejected("mission_report_dialog", REPORT_DIALOG_LAYOUT, root, "UI root refused mission dialog");
			root.RemoveFromHierarchy();
			return false;
		}

		m_aWidgets.Insert(root);
		HST_UIDebug.LogExpectedWidgetsCsv("mission_report_dialog", root, "HST_ReportDialogRoot|Dialog|Background|CloseButton|CloseLabel|Title|Subtitle|LocationLabel|LocationValue|MapPositionLabel|MapPositionValue|RequirementLabel|RequirementValue|ProgressLabel|ProgressValue|RewardLabel|RewardValue|FailureLabel|FailureValue|ObjectiveItems");
		HST_UIDebug.LogPopulation("mission_report_dialog", string.Format("missionId=%1 title=%2 category=%3 status=%4 remaining=%5 zone=%6 site=%7 position=%8", ShortenText(m_sSelectedMissionId, 48), ShortenText(m_aMissionTitles[index], 64), m_aMissionCategories[index], m_aMissionStatuses[index], m_aMissionRemaining[index], ShortenText(m_aMissionZones[index], 48), ShortenText(m_aMissionSites[index], 48), ShortenText(m_aMissionPositions[index], 48)));
		if (!m_WidgetHandler)
		{
			m_WidgetHandler = new HST_MissionClientWidgetHandler();
			m_WidgetHandler.Bind(this);
		}

		BindDialogClick(root, "CloseButton", DETAIL_CLOSE_WIDGET_ID);
		SetDialogText(root, "Title", ShortenText(m_aMissionTitles[index], 64), 0xFFF7E6BE, HST_UIWorkspaceMetrics.ScaleFont(26, scale), true, false);
		SetDialogText(root, "Subtitle", string.Format("%1 | %2 | %3s left", m_aMissionCategories[index], m_aMissionStatuses[index], m_aMissionRemaining[index]), 0xFFC9D4DC, HST_UIWorkspaceMetrics.ScaleFont(17, scale), false, false);
		SetDialogText(root, "CloseLabel", "Close", 0xFFF7E6BE, HST_UIWorkspaceMetrics.ScaleFont(17, scale), true, false);
		SetDialogText(root, "LocationLabel", "Location", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetDialogText(root, "LocationValue", m_aMissionZones[index] + " / " + m_aMissionSites[index], 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetDialogText(root, "MapPositionLabel", "Map position", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetDialogText(root, "MapPositionValue", m_aMissionPositions[index], 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetDialogText(root, "RequirementLabel", "Requirement", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetDialogText(root, "RequirementValue", m_aMissionRequirements[index], 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetDialogText(root, "ProgressLabel", "Progress", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetDialogText(root, "ProgressValue", m_aMissionProgress[index], 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetDialogText(root, "RewardLabel", "Reward", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetDialogText(root, "RewardValue", m_aMissionRewards[index], 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetDialogText(root, "FailureLabel", "Failure", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetDialogText(root, "FailureValue", m_aMissionFailures[index], 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetDialogText(root, "ObjectivesTitle", "Objectives", 0xFFF7E6BE, HST_UIWorkspaceMetrics.ScaleFont(22, scale), true, false);

		Widget items = root.FindAnyWidget("ObjectiveItems");
		int rendered;
		for (int i = 0; i < m_aObjectiveMissionIds.Count(); i++)
		{
			if (m_aObjectiveMissionIds[i] != m_sSelectedMissionId)
				continue;

			AddObjectiveRow(workspace, items, ShortenText(m_aObjectiveLabels[i], 42), ShortenText(m_aObjectiveProgress[i], 42), scale, rendered);
			rendered++;
		}

		if (rendered == 0)
			AddObjectiveRow(workspace, items, "No objective records.", "", scale, rendered);

		HST_UIDebug.LogRowSummary("mission_report_objectives", REPORT_OBJECTIVE_ROW_LAYOUT, Math.Max(1, rendered), string.Format("missionId=%1 realObjectives=%2", ShortenText(m_sSelectedMissionId, 48), rendered));
		return true;
	}

	bool OnWidgetClicked(int widgetId)
	{
		if (widgetId != DETAIL_CLOSE_WIDGET_ID)
			return false;

		CloseMissionDetails();
		return true;
	}

	protected void CloseMissionDetails()
	{
		bool wasOpen = m_bDetailOpen;
		m_bDetailOpen = false;
		m_sSelectedMissionId = "";
		if (wasOpen)
			HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.MISSION_DIALOG, "HST_MissionClientComponent");

		ClearWidgets();
	}

	protected void BindDialogClick(Widget root, string widgetName, int userId)
	{
		if (!root || userId <= 0)
			return;

		Widget widget = root.FindAnyWidget(widgetName);
		if (!widget)
			return;

		widget.SetUserID(userId);
		widget.AddHandler(m_WidgetHandler);
		HST_UIDebug.LogWidgetBound("mission_report_dialog", root, widgetName, userId);
	}

	protected void SetDialogText(Widget root, string widgetName, string text, int color, int fontSize, bool bold, bool wrap)
	{
		if (!root)
			return;

		TextWidget textWidget = TextWidget.Cast(root.FindAnyWidget(widgetName));
		if (!textWidget)
			return;

		textWidget.SetVisible(true);
		textWidget.SetOpacity(1.0);
		textWidget.SetText(text);
		textWidget.SetTextWrapping(wrap);
		textWidget.SetExactFontSize(fontSize);
		textWidget.SetBold(bold);
		textWidget.SetOutline(1, 0xDD000000);
		textWidget.SetShadow(2, 0xEE000000, 1, 1, 1);
		textWidget.SetColorInt(color);
	}

	protected void AddObjectiveRow(WorkspaceWidget workspace, Widget items, string label, string value, float scale, int rowIndex)
	{
		if (!workspace || !items)
			return;

		Widget row = workspace.CreateWidgets(REPORT_OBJECTIVE_ROW_LAYOUT, items);
		HST_UIDebug.LogRowSample("mission_report_objectives", REPORT_OBJECTIVE_ROW_LAYOUT, rowIndex, string.Format("label=%1 value=%2 row=%3", ShortenText(label, 64), ShortenText(value, 64), HST_UIDebug.WidgetSummary(row)));
		if (!row)
			return;

		SetDialogText(row, "Label", label, 0xFFF2F4F0, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, true);
		SetDialogText(row, "Value", value, 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(14, scale), false, true);
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
