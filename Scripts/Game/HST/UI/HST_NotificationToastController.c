class HST_NotificationToastRequest
{
	string m_sEventId;
	string m_sCategory;
	string m_sSeverity;
	string m_sTitle;
	string m_sMessage;
	float m_fDurationSeconds;
}

class HST_NotificationToastController
{
	static const ResourceName NOTIFICATION_TOAST_LAYOUT = "{A34F448C7E830600}UI/layouts/HST_NotificationToast.layout";

	protected static ref HST_NotificationToastController s_Instance;

	protected ref array<ref HST_NotificationToastRequest> m_aQueue = {};
	protected Widget m_wRoot;
	protected bool m_bVisible;
	protected int m_iToastGeneration;

	static HST_NotificationToastController Get()
	{
		if (!s_Instance)
			s_Instance = new HST_NotificationToastController();

		return s_Instance;
	}

	void Show(string eventId, string category, string severity, string title, string message, float durationSeconds)
	{
		if (message.IsEmpty())
			return;

		if (title.IsEmpty())
			title = "h-istasi";

		HST_NotificationToastRequest request = new HST_NotificationToastRequest();
		request.m_sEventId = eventId;
		request.m_sCategory = category;
		request.m_sSeverity = severity;
		request.m_sTitle = title;
		request.m_sMessage = message;
		request.m_fDurationSeconds = Math.Max(1.0, durationSeconds);

		m_aQueue.Insert(request);
		while (m_aQueue.Count() > 16)
			m_aQueue.Remove(0);

		if (!m_bVisible)
			ShowNext();
	}

	void Clear()
	{
		m_aQueue.Clear();
		m_iToastGeneration++;
		ClearCurrent();
	}

	protected void ShowNext()
	{
		if (m_aQueue.Count() == 0)
			return;

		HST_NotificationToastRequest request = m_aQueue[0];
		m_aQueue.Remove(0);
		Render(request);
	}

	protected void Render(HST_NotificationToastRequest request)
	{
		if (!request)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		m_iToastGeneration++;
		int generation = m_iToastGeneration;
		ClearCurrent();

		int screenW;
		int screenH;
		HST_UIWorkspaceMetrics.GetLayoutSize(workspace, screenW, screenH);
		HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, "HST_NotificationToast");
		float scale = HST_UIWorkspaceMetrics.GetScale(screenW, screenH, 0.70, 1.12);

		Widget root = workspace.CreateWidgets(NOTIFICATION_TOAST_LAYOUT);
		HST_UIDebug.LogLayoutCreate("notification_toast", NOTIFICATION_TOAST_LAYOUT, root);
		if (!root)
			return;

		HST_UIDebug.LogExpectedWidgetsCsv("notification_toast", root, "HST_NotificationRoot|Toast|Background|AccentLine|Title|Message");
		HST_UIDebug.LogPopulation("notification_toast", string.Format("event=%1 category=%2 severity=%3 title=%4 message=%5 duration=%6 queueRemaining=%7", request.m_sEventId, request.m_sCategory, request.m_sSeverity, ShortenText(request.m_sTitle, 64), ShortenText(request.m_sMessage, 120), request.m_fDurationSeconds, m_aQueue.Count()));

		root.SetVisible(true);
		root.SetOpacity(1.0);
		root.SetZOrder(HST_UIConstants.Z_NOTIFICATION);
		root.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		m_wRoot = root;
		m_bVisible = true;
		HST_UIRootService.Get().NotifyNotificationShown();

		Widget accentLine = root.FindAnyWidget("AccentLine");
		if (accentLine)
			accentLine.SetColorInt(NotificationAccentColor(request.m_sSeverity, request.m_sCategory));

		TextWidget titleWidget = TextWidget.Cast(root.FindAnyWidget("Title"));
		if (titleWidget)
		{
			titleWidget.SetText(ShortenText(request.m_sTitle, 44));
			titleWidget.SetTextWrapping(false);
			ApplyTextStyle(titleWidget, HST_UIWorkspaceMetrics.ScaleFont(18, scale), true);
			titleWidget.SetColorInt(NotificationAccentColor(request.m_sSeverity, request.m_sCategory));
		}

		TextWidget messageWidget = TextWidget.Cast(root.FindAnyWidget("Message"));
		if (messageWidget)
		{
			messageWidget.SetText(ShortenText(request.m_sMessage, 140));
			messageWidget.SetTextWrapping(true);
			ApplyTextStyle(messageWidget, HST_UIWorkspaceMetrics.ScaleFont(16, scale), false);
			messageWidget.SetColorInt(0xFFF2F4F0);
		}

		GetGame().GetCallqueue().CallLater(DismissCurrent, Math.Round(request.m_fDurationSeconds * 1000.0), false, generation);
	}

	protected void DismissCurrent(int generation)
	{
		if (generation != m_iToastGeneration)
			return;

		ClearCurrent();
		ShowNext();
	}

	protected void ClearCurrent()
	{
		if (m_wRoot)
			m_wRoot.RemoveFromHierarchy();

		m_wRoot = null;
		if (m_bVisible)
		{
			HST_UIRootService.Get().NotifyNotificationHidden();
			m_bVisible = false;
		}
	}

	protected void ApplyTextStyle(TextWidget textWidget, int fontSize, bool bold)
	{
		if (!textWidget)
			return;

		textWidget.SetExactFontSize(fontSize);
		textWidget.SetBold(bold);
		textWidget.SetOutline(1, 0xDD000000);
		textWidget.SetShadow(2, 0xEE000000, 1, 1, 1);
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
