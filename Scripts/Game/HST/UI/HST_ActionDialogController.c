class HST_ActionDialogData
{
	string m_sOwner;
	string m_sDebugOwner = "action_dialog";
	int m_iCancelWidgetId;
	int m_iConfirmWidgetId;
	string m_sTitle = "Confirm Action";
	string m_sMessage;
	string m_sCancelLabel = "Cancel";
	string m_sConfirmLabel = "Confirm";
}

class HST_ActionChoiceDialogData
{
	string m_sOwner;
	string m_sDebugOwner = "action_choice_dialog";
	int m_iCancelWidgetId;
	int m_iChoiceWidgetIdBase;
	string m_sTitle = "Select";
	string m_sMessage;
	string m_sCancelLabel = "Cancel";
	ref array<string> m_aChoiceLabels = {};
}

class HST_ActionDialogController
{
	static const ResourceName ACTION_DIALOG_LAYOUT = "{D66CFA01E5AA4200}UI/layouts/HST_ActionDialog.layout";

	static Widget Render(WorkspaceWidget workspace, HST_ActionDialogData data, ScriptedWidgetEventHandler handler)
	{
		if (!workspace || !data)
			return null;

		int screenW;
		int screenH;
		HST_UIWorkspaceMetrics.GetLayoutSize(workspace, screenW, screenH);
		HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, data.m_sDebugOwner);
		float scale = HST_UIWorkspaceMetrics.GetScale(screenW, screenH, 0.70, 1.12);

		Widget root = workspace.CreateWidgets(ACTION_DIALOG_LAYOUT, workspace);
		HST_UIDebug.LogLayoutCreate(data.m_sDebugOwner, ACTION_DIALOG_LAYOUT, root, workspace);
		if (!root)
			return null;

		root.SetVisible(true);
		root.SetOpacity(1.0);
		root.SetZOrder(HST_UIConstants.Z_ACTION_DIALOG);
		if (!HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.ACTION_DIALOG, data.m_sOwner, root, false, false, true))
		{
			HST_UIDebug.LogLayoutRejected(data.m_sDebugOwner, ACTION_DIALOG_LAYOUT, root, "UI root refused action modal");
			root.RemoveFromHierarchy();
			return null;
		}

		HST_UIDebug.LogExpectedWidgetsCsv(data.m_sDebugOwner, root, "HST_ActionDialogRoot|Dialog|Title|Message|CancelButton|CancelLabel|ConfirmButton|ConfirmLabel");
		HST_UIDebug.LogPopulation(data.m_sDebugOwner, string.Format("owner=%1 title=%2 message=%3 cancelId=%4 confirmId=%5", data.m_sOwner, ShortenText(data.m_sTitle, 64), ShortenText(data.m_sMessage, 140), data.m_iCancelWidgetId, data.m_iConfirmWidgetId));

		SetText(root, "Title", data.m_sTitle, 0xFFF7E6BE, HST_UIWorkspaceMetrics.ScaleFont(24, scale), true, false);
		SetText(root, "Message", data.m_sMessage, 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false, true);
		SetText(root, "CancelLabel", data.m_sCancelLabel, 0xFFF2F4F0, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		SetText(root, "ConfirmLabel", data.m_sConfirmLabel, 0xFF17110A, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		BindClick(data.m_sDebugOwner, root, "CancelButton", data.m_iCancelWidgetId, handler);
		BindClick(data.m_sDebugOwner, root, "ConfirmButton", data.m_iConfirmWidgetId, handler);
		QueueReadyGeometry(root, data.m_sDebugOwner);
		return root;
	}

	static void Close(string owner)
	{
		HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.ACTION_DIALOG, owner);
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

	protected static void QueueReadyGeometry(Widget root, string debugOwner)
	{
		if (!root)
			return;

		GetGame().GetCallqueue().CallLater(LogReadyGeometry, 0, false, root, debugOwner);
		GetGame().GetCallqueue().CallLater(LogReadyGeometry, 50, false, root, debugOwner);
	}

	protected static void LogReadyGeometry(Widget root, string debugOwner)
	{
		if (!root || !root.IsVisibleInHierarchy())
			return;

		HST_UIDebug.LogWidgetGeometryCsv(debugOwner + "_ready", root, "HST_ActionDialogRoot|Dialog|Title|Message|CancelButton|CancelLabel|ConfirmButton|ConfirmLabel");
		HST_UIDebug.LogReadyWidgetsCsv(debugOwner + "_ready", root, "HST_ActionDialogRoot|Dialog|Title|Message|CancelButton|CancelLabel|ConfirmButton|ConfirmLabel");
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

class HST_ActionChoiceDialogController
{
	static const ResourceName ACTION_CHOICE_DIALOG_LAYOUT = "{E27B4D993F1A6C00}UI/layouts/HST_CommandChoiceDialog.layout";
	static const ResourceName ACTION_CHOICE_ROW_LAYOUT = "{A7B8C9D001237100}UI/layouts/HST/Rows/HST_CommandChoiceRow.layout";
	static const int MAX_CHOICES = 128;

	static Widget Render(WorkspaceWidget workspace, HST_ActionChoiceDialogData data, ScriptedWidgetEventHandler handler)
	{
		if (!workspace || !data)
			return null;

		int screenW;
		int screenH;
		HST_UIWorkspaceMetrics.GetLayoutSize(workspace, screenW, screenH);
		HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, data.m_sDebugOwner);
		float scale = HST_UIWorkspaceMetrics.GetScale(screenW, screenH, 0.70, 1.12);

		Widget root = workspace.CreateWidgets(ACTION_CHOICE_DIALOG_LAYOUT, workspace);
		HST_UIDebug.LogLayoutCreate(data.m_sDebugOwner, ACTION_CHOICE_DIALOG_LAYOUT, root, workspace);
		if (!root)
			return null;

		root.SetVisible(true);
		root.SetOpacity(1.0);
		root.SetZOrder(HST_UIConstants.Z_ACTION_DIALOG);
		if (!HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.ACTION_DIALOG, data.m_sOwner, root, false, false, true))
		{
			HST_UIDebug.LogLayoutRejected(data.m_sDebugOwner, ACTION_CHOICE_DIALOG_LAYOUT, root, "UI root refused action choice modal");
			root.RemoveFromHierarchy();
			return null;
		}

		HST_UIDebug.LogExpectedWidgetsCsv(data.m_sDebugOwner, root, "HST_CommandChoiceDialogRoot|Dialog|Title|Message|ChoiceScrollHost|ChoiceItems|CancelButton|CancelLabel|ChoiceButton0|ChoiceLabel0|ChoiceButton1|ChoiceLabel1|ChoiceButton2|ChoiceLabel2|ChoiceButton3|ChoiceLabel3|ChoiceButton4|ChoiceLabel4|ChoiceButton5|ChoiceLabel5");
		HST_UIDebug.LogPopulation(data.m_sDebugOwner, string.Format("owner=%1 title=%2 choices=%3 cancelId=%4 choiceBase=%5", data.m_sOwner, ShortenText(data.m_sTitle, 64), data.m_aChoiceLabels.Count(), data.m_iCancelWidgetId, data.m_iChoiceWidgetIdBase));

		SetText(root, "Title", data.m_sTitle, 0xFFF7E6BE, HST_UIWorkspaceMetrics.ScaleFont(24, scale), true, false);
		SetText(root, "Message", data.m_sMessage, 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(15, scale), false, true);
		SetText(root, "CancelLabel", data.m_sCancelLabel, 0xFFF2F4F0, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, false);
		BindClick(data.m_sDebugOwner, root, "CancelButton", data.m_iCancelWidgetId, handler);

		Widget choiceItems = root.FindAnyWidget("ChoiceItems");
		if (choiceItems)
		{
			for (int fixedIndex = 0; fixedIndex < 6; fixedIndex++)
				SetChoiceVisible(root, fixedIndex, false);

			int rowCount = Math.Min(MAX_CHOICES, data.m_aChoiceLabels.Count());
			for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
			{
				Widget row = workspace.CreateWidgets(ACTION_CHOICE_ROW_LAYOUT, choiceItems);
				if (!row)
					continue;

				SetText(row, "Label", data.m_aChoiceLabels[rowIndex], 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(14, scale), true, true);
				BindChoiceRowClick(data.m_sDebugOwner, row, data.m_iChoiceWidgetIdBase + rowIndex, handler);
				HST_UIDebug.LogWidgetBound(data.m_sDebugOwner, row, "HST_CommandChoiceRow", data.m_iChoiceWidgetIdBase + rowIndex);
			}

			QueueReadyGeometry(root, data.m_sDebugOwner);
			return root;
		}

		for (int i = 0; i < MAX_CHOICES; i++)
		{
			bool visible = i < data.m_aChoiceLabels.Count();
			SetChoiceVisible(root, i, visible);
			if (!visible)
				continue;

			SetText(root, "ChoiceLabel" + i.ToString(), data.m_aChoiceLabels[i], 0xFFE7EDF1, HST_UIWorkspaceMetrics.ScaleFont(16, scale), true, true);
			BindClick(data.m_sDebugOwner, root, "ChoiceButton" + i.ToString(), data.m_iChoiceWidgetIdBase + i, handler);
		}

		QueueReadyGeometry(root, data.m_sDebugOwner);
		return root;
	}

	protected static void SetChoiceVisible(Widget root, int index, bool visible)
	{
		if (!root)
			return;

		Widget button = root.FindAnyWidget("ChoiceButton" + index.ToString());
		if (button)
			button.SetVisible(visible);

		Widget label = root.FindAnyWidget("ChoiceLabel" + index.ToString());
		if (label)
			label.SetVisible(visible);
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

	protected static void BindChoiceRowClick(string debugOwner, Widget row, int userId, ScriptedWidgetEventHandler handler)
	{
		if (!row || userId <= 0)
			return;

		row.SetUserID(userId);
		if (handler)
			row.AddHandler(handler);
	}

	protected static void QueueReadyGeometry(Widget root, string debugOwner)
	{
		if (!root)
			return;

		GetGame().GetCallqueue().CallLater(LogReadyGeometry, 0, false, root, debugOwner);
		GetGame().GetCallqueue().CallLater(LogReadyGeometry, 50, false, root, debugOwner);
	}

	protected static void LogReadyGeometry(Widget root, string debugOwner)
	{
		if (!root || !root.IsVisibleInHierarchy())
			return;

		HST_UIDebug.LogWidgetGeometryCsv(debugOwner + "_ready", root, "HST_CommandChoiceDialogRoot|Dialog|Title|Message|ChoiceScrollHost|ChoiceItems|CancelButton|CancelLabel|ChoiceButton0|ChoiceLabel0|ChoiceButton1|ChoiceLabel1|ChoiceButton2|ChoiceLabel2|ChoiceButton3|ChoiceLabel3|ChoiceButton4|ChoiceLabel4|ChoiceButton5|ChoiceLabel5");
		HST_UIDebug.LogReadyWidgetsCsv(debugOwner + "_ready", root, "HST_CommandChoiceDialogRoot|Dialog|Title|Message|ChoiceScrollHost|ChoiceItems|CancelButton|CancelLabel|ChoiceButton0|ChoiceLabel0|ChoiceButton1|ChoiceLabel1|ChoiceButton2|ChoiceLabel2|ChoiceButton3|ChoiceLabel3|ChoiceButton4|ChoiceLabel4|ChoiceButton5|ChoiceLabel5");
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
