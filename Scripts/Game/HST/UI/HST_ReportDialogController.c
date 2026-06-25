class HST_ReportDialogData
{
	string m_sOwner;
	string m_sDebugOwner = "report_dialog";
	int m_iCloseWidgetId;
	string m_sReportId;
	string m_sTitle;
	string m_sSubtitle;
	string m_sLocation;
	string m_sMapPosition;
	string m_sRequirement;
	string m_sProgress;
	string m_sReward;
	string m_sFailure;
	ref array<string> m_aObjectiveLabels = {};
	ref array<string> m_aObjectiveValues = {};
}

class HST_ReportDialogController
{
	static const ResourceName REPORT_DIALOG_LAYOUT = "{D66CFA01E5AA4100}UI/layouts/HST_ReportDialog.layout";
	static const ResourceName REPORT_OBJECTIVE_ROW_LAYOUT = "{D66CFA01E5AA4300}UI/layouts/HST/Rows/HST_ReportObjectiveRow.layout";

	static Widget Render(WorkspaceWidget workspace, HST_ReportDialogData data, ScriptedWidgetEventHandler handler)
	{
		if (!workspace || !data)
			return null;

		int screenW;
		int screenH;
		HST_UIWorkspaceMetrics.GetLayoutSize(workspace, screenW, screenH);
		HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, data.m_sDebugOwner);
		float scale = HST_UIWorkspaceMetrics.GetScale(screenW, screenH, 0.70, 1.12);

		Widget root = workspace.CreateWidgets(REPORT_DIALOG_LAYOUT);
		HST_UIDebug.LogLayoutCreate(data.m_sDebugOwner, REPORT_DIALOG_LAYOUT, root);
		if (!root)
			return null;

		root.SetVisible(true);
		root.SetOpacity(1.0);
		root.SetZOrder(HST_UIConstants.Z_MISSION_DIALOG);
		if (!HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.MISSION_DIALOG, data.m_sOwner, root, false, false, true))
		{
			HST_UIDebug.LogLayoutRejected(data.m_sDebugOwner, REPORT_DIALOG_LAYOUT, root, "UI root refused report dialog");
			root.RemoveFromHierarchy();
			return null;
		}

		HST_UIDebug.LogExpectedWidgetsCsv(data.m_sDebugOwner, root, "HST_ReportDialogRoot|Dialog|Background|CloseButton|CloseLabel|Title|Subtitle|LocationLabel|LocationValue|MapPositionLabel|MapPositionValue|RequirementLabel|RequirementValue|ProgressLabel|ProgressValue|RewardLabel|RewardValue|FailureLabel|FailureValue|ObjectiveItems");
		HST_UIDebug.LogPopulation(data.m_sDebugOwner, string.Format("reportId=%1 title=%2 subtitle=%3 location=%4 position=%5 objectives=%6", ShortenText(data.m_sReportId, 48), ShortenText(data.m_sTitle, 64), ShortenText(data.m_sSubtitle, 80), ShortenText(data.m_sLocation, 64), ShortenText(data.m_sMapPosition, 64), data.m_aObjectiveLabels.Count()));

		BindClick(data.m_sDebugOwner, root, "CloseButton", data.m_iCloseWidgetId, handler);
		SetText(root, "Title", data.m_sTitle, 0xFFF7E6BE, HST_UIWorkspaceMetrics.ScaleFont(26, scale), true, false);
		SetText(root, "Subtitle", data.m_sSubtitle, 0xFFC9D4DC, HST_UIWorkspaceMetrics.ScaleFont(17, scale), false, false);
		SetText(root, "CloseLabel", "Close", 0xFFF7E6BE, HST_UIWorkspaceMetrics.ScaleFont(17, scale), true, false);
		SetText(root, "LocationLabel", "Location", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetText(root, "LocationValue", data.m_sLocation, 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetText(root, "MapPositionLabel", "Map position", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetText(root, "MapPositionValue", data.m_sMapPosition, 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetText(root, "RequirementLabel", "Requirement", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetText(root, "RequirementValue", data.m_sRequirement, 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetText(root, "ProgressLabel", "Progress", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetText(root, "ProgressValue", data.m_sProgress, 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetText(root, "RewardLabel", "Reward", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetText(root, "RewardValue", data.m_sReward, 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetText(root, "FailureLabel", "Failure", 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetText(root, "FailureValue", data.m_sFailure, 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetText(root, "ObjectivesTitle", "Objectives", 0xFFF7E6BE, HST_UIWorkspaceMetrics.ScaleFont(22, scale), true, false);

		Widget items = root.FindAnyWidget("ObjectiveItems");
		int rendered = 0;
		for (int i = 0; i < data.m_aObjectiveLabels.Count(); i++)
		{
			string value = "";
			if (data.m_aObjectiveValues.IsIndexValid(i))
				value = data.m_aObjectiveValues[i];

			AddObjectiveRow(workspace, items, ShortenText(data.m_aObjectiveLabels[i], 42), ShortenText(value, 42), scale, rendered);
			rendered++;
		}

		if (rendered == 0)
			AddObjectiveRow(workspace, items, "No objective records.", "", scale, rendered);

		HST_UIDebug.LogRowSummary("mission_report_objectives", REPORT_OBJECTIVE_ROW_LAYOUT, Math.Max(1, rendered), string.Format("reportId=%1 realObjectives=%2", ShortenText(data.m_sReportId, 48), rendered));
		return root;
	}

	static void Close(string owner)
	{
		HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.MISSION_DIALOG, owner);
	}

	protected static void BindClick(string debugOwner, Widget root, string widgetName, int userId, ScriptedWidgetEventHandler handler)
	{
		if (!root || userId <= 0)
			return;

		Widget widget = root.FindAnyWidget(widgetName);
		if (!widget)
			return;

		widget.SetUserID(userId);
		if (handler)
			widget.AddHandler(handler);
		HST_UIDebug.LogWidgetBound(debugOwner, root, widgetName, userId);
	}

	protected static void SetText(Widget root, string widgetName, string text, int color, int fontSize, bool bold, bool wrap)
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

	protected static void AddObjectiveRow(WorkspaceWidget workspace, Widget items, string label, string value, float scale, int rowIndex)
	{
		if (!workspace || !items)
			return;

		Widget row = workspace.CreateWidgets(REPORT_OBJECTIVE_ROW_LAYOUT, items);
		HST_UIDebug.LogRowSample("mission_report_objectives", REPORT_OBJECTIVE_ROW_LAYOUT, rowIndex, string.Format("label=%1 value=%2 row=%3", ShortenText(label, 64), ShortenText(value, 64), HST_UIDebug.WidgetSummary(row)));
		if (!row)
			return;

		SetText(row, "Label", label, 0xFFF2F4F0, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, true);
		SetText(row, "Value", value, 0xFFFFD98B, HST_UIWorkspaceMetrics.ScaleFont(14, scale), false, true);
	}

	protected static string ShortenText(string text, int maxCharacters)
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
