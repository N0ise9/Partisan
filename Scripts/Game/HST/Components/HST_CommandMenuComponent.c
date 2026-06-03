class HST_CommandMenuWidgetHandler : ScriptedWidgetEventHandler
{
	protected HST_CommandMenuComponent m_Menu;

	void Bind(HST_CommandMenuComponent menu)
	{
		m_Menu = menu;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Menu)
			return false;

		return m_Menu.OnWidgetClicked(w.GetUserID());
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (!m_Menu)
			return false;

		return m_Menu.OnWidgetClicked(w.GetUserID());
	}
}

[ComponentEditorProps(category: "h-istasi", description: "Client-side h-istasi command menu key listener and widget controller")]
class HST_CommandMenuComponentClass : SCR_BaseGameModeComponentClass
{
}

class HST_CommandMenuComponent : SCR_BaseGameModeComponent
{
	static const string COMMAND_MENU_ACTION = "Inventory";
	static const string COMMAND_MENU_CUSTOM_ACTION = "HST_CommandMenu";
	static const string COMMAND_MENU_UP_ACTION = "MenuUp";
	static const string COMMAND_MENU_DOWN_ACTION = "MenuDown";
	static const string COMMAND_MENU_SELECT_ACTION = "MenuSelect";
	static const string COMMAND_MENU_BACK_ACTION = "MenuBack";
	static const string MENU_INPUT_CONTEXT = "InGameMenuContext";
	static const string MENU_LAYOUT = "UI/layouts/HST_CommandMenu.layout";
	static const int TAB_WIDGET_ID_BASE = 1000;
	static const int ACTION_WIDGET_ID_BASE = 2000;

	protected static HST_CommandMenuComponent s_LocalInstance;

	protected bool m_bMenuOpen;
	protected string m_sSelectedTab = "general";
	protected string m_sLastPayload;
	protected string m_sLastResult;
	protected string m_sStatusText = "h-istasi menu | waiting for server snapshot";
	protected int m_iSelectedControl;
	protected ref array<string> m_aTabIds = {};
	protected ref array<string> m_aTabLabels = {};
	protected ref array<bool> m_aTabEnabled = {};
	protected ref array<string> m_aActionLabels = {};
	protected ref array<string> m_aActionCommands = {};
	protected ref array<string> m_aActionArguments = {};
	protected ref array<bool> m_aActionEnabled = {};
	protected ref array<string> m_aActionDisabledReasons = {};
	protected ref array<Widget> m_aWidgets = {};
	protected ref HST_CommandMenuWidgetHandler m_WidgetHandler;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_LocalInstance = this;
		RegisterInputListeners();
		BuildTabList();
		BuildActionList();
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void OnDelete(IEntity owner)
	{
		UnregisterInputListeners();
		CloseMenu();
		if (s_LocalInstance == this)
			s_LocalInstance = null;

		super.OnDelete(owner);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bMenuOpen)
			return;

		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
			inputManager.ActivateContext(MENU_INPUT_CONTEXT);
	}

	static HST_CommandMenuComponent GetLocalInstance()
	{
		return s_LocalInstance;
	}

	void OnServerSnapshot(string payload, string lastResult = "")
	{
		m_sLastPayload = payload;
		m_sLastResult = lastResult;
		m_sStatusText = ExtractSection(payload, "STATUS");
		if (m_sStatusText.IsEmpty())
			m_sStatusText = payload;

		string result = ExtractSection(payload, "RESULT");
		if (!result.IsEmpty())
			m_sLastResult = result;

		ParseTabsFromPayload(payload);
		ParseActionsFromPayload(payload);
		if (m_iSelectedControl >= GetControlCount())
			m_iSelectedControl = Math.Max(0, GetControlCount() - 1);

		if (m_bMenuOpen)
			RenderMenu();
	}

	bool OnWidgetClicked(int widgetId)
	{
		if (!m_bMenuOpen)
			return false;

		if (widgetId >= TAB_WIDGET_ID_BASE && widgetId < ACTION_WIDGET_ID_BASE)
		{
			SwitchToTab(widgetId - TAB_WIDGET_ID_BASE);
			return true;
		}

		if (widgetId >= ACTION_WIDGET_ID_BASE)
		{
			m_iSelectedControl = m_aTabIds.Count() + widgetId - ACTION_WIDGET_ID_BASE;
			ExecuteSelectedAction();
			return true;
		}

		return false;
	}

	protected void RegisterInputListeners()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.AddActionListener(COMMAND_MENU_ACTION, EActionTrigger.DOWN, ToggleMenu);
		inputManager.AddActionListener(COMMAND_MENU_CUSTOM_ACTION, EActionTrigger.DOWN, ToggleMenu);
		inputManager.AddActionListener(COMMAND_MENU_UP_ACTION, EActionTrigger.DOWN, SelectPreviousAction);
		inputManager.AddActionListener(COMMAND_MENU_DOWN_ACTION, EActionTrigger.DOWN, SelectNextAction);
		inputManager.AddActionListener(COMMAND_MENU_SELECT_ACTION, EActionTrigger.DOWN, ExecuteSelectedAction);
		inputManager.AddActionListener(COMMAND_MENU_BACK_ACTION, EActionTrigger.DOWN, CloseMenu);
	}

	protected void UnregisterInputListeners()
	{
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.RemoveActionListener(COMMAND_MENU_ACTION, EActionTrigger.DOWN, ToggleMenu);
		inputManager.RemoveActionListener(COMMAND_MENU_CUSTOM_ACTION, EActionTrigger.DOWN, ToggleMenu);
		inputManager.RemoveActionListener(COMMAND_MENU_UP_ACTION, EActionTrigger.DOWN, SelectPreviousAction);
		inputManager.RemoveActionListener(COMMAND_MENU_DOWN_ACTION, EActionTrigger.DOWN, SelectNextAction);
		inputManager.RemoveActionListener(COMMAND_MENU_SELECT_ACTION, EActionTrigger.DOWN, ExecuteSelectedAction);
		inputManager.RemoveActionListener(COMMAND_MENU_BACK_ACTION, EActionTrigger.DOWN, CloseMenu);
	}

	protected void BuildTabList()
	{
		m_aTabIds.Clear();
		m_aTabLabels.Clear();
		m_aTabEnabled.Clear();

		AddTab("setup", "Setup", true);
		AddTab("general", "General", true);
		AddTab("petros", "Petros/HQ", true);
		AddTab("commander", "Commander", true);
		AddTab("arsenal", "Arsenal/Loot", true);
		AddTab("admin", "Admin", true);
	}

	protected void AddTab(string tabId, string label, bool enabled)
	{
		m_aTabIds.Insert(tabId);
		m_aTabLabels.Insert(label);
		m_aTabEnabled.Insert(enabled);
	}

	protected void BuildActionList()
	{
		m_aActionLabels.Clear();
		m_aActionCommands.Clear();
		m_aActionArguments.Clear();
		m_aActionEnabled.Clear();
		m_aActionDisabledReasons.Clear();
	}

	protected void AddAction(string label, string commandId, string argument, bool enabled = true, string disabledReason = "")
	{
		m_aActionLabels.Insert(label);
		m_aActionCommands.Insert(commandId);
		m_aActionArguments.Insert(argument);
		m_aActionEnabled.Insert(enabled);
		m_aActionDisabledReasons.Insert(disabledReason);
	}

	protected void ToggleMenu()
	{
		if (m_bMenuOpen)
		{
			CloseMenu();
			return;
		}

		OpenMenu();
	}

	protected void OpenMenu()
	{
		m_bMenuOpen = true;
		if (m_sSelectedTab.IsEmpty())
			m_sSelectedTab = "general";

		BuildTabList();
		BuildActionList();
		m_iSelectedControl = Math.Max(0, m_aTabIds.Find(m_sSelectedTab));
		m_sStatusText = "h-istasi menu | requesting server snapshot";
		RequestSnapshot();
		RenderMenu();
		Print("h-istasi menu | opened");
	}

	protected void CloseMenu()
	{
		if (!m_bMenuOpen)
			return;

		m_bMenuOpen = false;
		ClearWidgets();
		Print("h-istasi menu | closed");
	}

	protected void SelectPreviousAction()
	{
		if (!m_bMenuOpen)
			return;

		int count = GetControlCount();
		if (count <= 0)
			return;

		m_iSelectedControl--;
		if (m_iSelectedControl < 0)
			m_iSelectedControl = count - 1;

		RenderMenu();
	}

	protected void SelectNextAction()
	{
		if (!m_bMenuOpen)
			return;

		int count = GetControlCount();
		if (count <= 0)
			return;

		m_iSelectedControl++;
		if (m_iSelectedControl >= count)
			m_iSelectedControl = 0;

		RenderMenu();
	}

	protected void ExecuteSelectedAction()
	{
		if (!m_bMenuOpen)
			return;

		if (m_iSelectedControl < m_aTabIds.Count())
		{
			SwitchToTab(m_iSelectedControl);
			return;
		}

		int actionIndex = m_iSelectedControl - m_aTabIds.Count();
		if (actionIndex < 0 || actionIndex >= m_aActionCommands.Count())
			return;

		if (!m_aActionEnabled[actionIndex])
		{
			m_sLastResult = "h-istasi command | " + m_aActionLabels[actionIndex] + " | " + m_aActionDisabledReasons[actionIndex];
			RenderMenu();
			return;
		}

		RequestAction(m_aActionCommands[actionIndex], m_aActionArguments[actionIndex]);
		m_sLastResult = "h-istasi command | requested " + m_aActionLabels[actionIndex];
		RenderMenu();
	}

	protected void SwitchToTab(int tabIndex)
	{
		if (tabIndex < 0 || tabIndex >= m_aTabIds.Count())
			return;

		if (!m_aTabEnabled[tabIndex])
			return;

		m_sSelectedTab = m_aTabIds[tabIndex];
		m_iSelectedControl = tabIndex;
		m_sStatusText = "h-istasi menu | requesting " + m_aTabLabels[tabIndex];
		BuildActionList();
		RequestSnapshot();
		RenderMenu();
	}

	protected void RequestSnapshot()
	{
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (request)
		{
			request.RequestSnapshot(m_sSelectedTab, m_sLastResult);
			return;
		}

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (coordinator && Replication.IsServer())
		{
			OnServerSnapshot(coordinator.BuildVisibleMenuPayload(ResolveLocalPlayerId(), m_sSelectedTab, m_sLastResult), m_sLastResult);
			return;
		}

		OnServerSnapshot("HST_MENU|offline|0\nSTATUS|h-istasi menu | player request bridge not ready\nEND", "request bridge not ready");
	}

	protected void RequestAction(string commandId, string argument)
	{
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (request)
		{
			request.RequestAction(m_sSelectedTab, commandId, argument);
			return;
		}

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (coordinator && Replication.IsServer())
		{
			string result = coordinator.RequestVisibleMenuCommand(ResolveLocalPlayerId(), m_sSelectedTab, commandId, argument);
			OnServerSnapshot(coordinator.BuildVisibleMenuPayload(ResolveLocalPlayerId(), m_sSelectedTab, result), result);
			return;
		}

		OnServerSnapshot("HST_MENU|offline|0\nSTATUS|h-istasi command | player request bridge not ready\nEND", "request bridge not ready");
	}

	protected void RenderMenu()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			Print(BuildConsoleMenuText());
			return;
		}

		ClearWidgets();
		if (!m_WidgetHandler)
		{
			m_WidgetHandler = new HST_CommandMenuWidgetHandler();
			m_WidgetHandler.Bind(this);
		}

		Widget root = workspace.CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID, 80, 64, 1180, 760, WidgetFlags.VISIBLE, null, 2500);
		root.SetColorInt(0xE615171A);
		root.SetOpacity(0.96);
		m_aWidgets.Insert(root);

		CreateTextWidget(workspace, "h-istasi", 116, 92, 260, 36, 28, 0xFFEDE6DA, 0, true);
		CreateTextWidget(workspace, "I close/open    MenuUp/MenuDown select    MenuSelect run    MenuBack close", 380, 99, 760, 24, 18, 0xFFC7C7C7, 0, false);
		RenderTabs(workspace);
		RenderActions(workspace);
		CreateTextWidget(workspace, m_sStatusText, 116, 190, 640, 560, 18, 0xFFE0E0E0, 0, false);
		CreateTextWidget(workspace, BuildResultText(), 790, 190, 420, 180, 18, 0xFFD2E7B8, 0, false);
		CreateTextWidget(workspace, "UI/layouts/HST_CommandMenu.layout", 790, 746, 420, 24, 14, 0xFF777777, 0, false);
	}

	protected void RenderTabs(WorkspaceWidget workspace)
	{
		int left = 116;
		for (int i = 0; i < m_aTabLabels.Count(); i++)
		{
			int color = 0xFFCFCFCF;
			string label = m_aTabLabels[i];
			if (m_aTabIds[i] == m_sSelectedTab)
			{
				color = 0xFFE9C46A;
				label = "[" + label + "]";
			}

			if (m_iSelectedControl == i)
				label = "> " + label;

			if (!m_aTabEnabled[i])
				color = 0xFF777777;

			CreateTextWidget(workspace, label, left, 142, 170, 30, 18, color, TAB_WIDGET_ID_BASE + i, false);
			left += 176;
		}
	}

	protected void RenderActions(WorkspaceWidget workspace)
	{
		int top = 398;
		CreateTextWidget(workspace, "Actions", 790, top - 36, 180, 28, 20, 0xFFEDE6DA, 0, true);
		for (int i = 0; i < m_aActionLabels.Count(); i++)
		{
			string prefix = "  ";
			if (m_iSelectedControl == m_aTabIds.Count() + i)
				prefix = "> ";

			string suffix = "";
			int color = 0xFFE5E5E5;
			if (!m_aActionEnabled[i])
			{
				suffix = " | " + m_aActionDisabledReasons[i];
				color = 0xFF888888;
			}

			CreateTextWidget(workspace, prefix + m_aActionLabels[i] + suffix, 790, top + i * 34, 420, 30, 18, color, ACTION_WIDGET_ID_BASE + i, false);
		}
	}

	protected TextWidget CreateTextWidget(WorkspaceWidget workspace, string text, int left, int top, int width, int height, int fontSize, int color, int userId, bool bold)
	{
		Widget widget = workspace.CreateWidgetInWorkspace(WidgetType.TextWidgetTypeID, left, top, width, height, WidgetFlags.VISIBLE | WidgetFlags.NO_LOCALIZATION | WidgetFlags.WRAP_TEXT, null, 2600);
		TextWidget textWidget = TextWidget.Cast(widget);
		if (textWidget)
		{
			textWidget.SetText(text);
			textWidget.SetExactFontSize(fontSize);
			textWidget.SetTextWrapping(true);
			textWidget.SetBold(bold);
			textWidget.SetShadow(1, 0xCC000000, 1, 1, 1);
		}

		widget.SetColorInt(color);
		if (userId > 0)
		{
			widget.SetUserID(userId);
			widget.AddHandler(m_WidgetHandler);
		}

		m_aWidgets.Insert(widget);
		return textWidget;
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

	protected string BuildResultText()
	{
		if (m_sLastResult.IsEmpty())
			return "Last action\n-";

		return "Last action\n" + m_sLastResult;
	}

	protected string BuildConsoleMenuText()
	{
		string text = "h-istasi menu | " + m_sSelectedTab + "\n" + m_sStatusText;
		for (int i = 0; i < m_aActionLabels.Count(); i++)
			text = text + "\n" + m_aActionLabels[i];

		return text;
	}

	protected int GetControlCount()
	{
		return m_aTabIds.Count() + m_aActionLabels.Count();
	}

	protected void ParseTabsFromPayload(string payload)
	{
		if (payload.IsEmpty() || !payload.Contains("TAB|"))
			return;

		m_aTabIds.Clear();
		m_aTabLabels.Clear();
		m_aTabEnabled.Clear();

		int cursor;
		while (true)
		{
			int lineStart = payload.IndexOfFrom(cursor, "TAB|");
			if (lineStart < 0)
				break;

			int lineEnd = payload.IndexOfFrom(lineStart, "\n");
			if (lineEnd < 0)
				lineEnd = payload.Length();

			string line = payload.Substring(lineStart, lineEnd - lineStart);
			AddTab(ExtractPipeField(line, 1), ExtractPipeField(line, 2), ParseBool(ExtractPipeField(line, 3)));
			cursor = lineEnd + 1;
		}
	}

	protected void ParseActionsFromPayload(string payload)
	{
		BuildActionList();
		if (payload.IsEmpty() || !payload.Contains("ACTION|"))
			return;

		int cursor;
		while (true)
		{
			int lineStart = payload.IndexOfFrom(cursor, "ACTION|");
			if (lineStart < 0)
				break;

			int lineEnd = payload.IndexOfFrom(lineStart, "\n");
			if (lineEnd < 0)
				lineEnd = payload.Length();

			string line = payload.Substring(lineStart, lineEnd - lineStart);
			string tabId = ExtractPipeField(line, 1);
			if (tabId == m_sSelectedTab)
				AddAction(ExtractPipeField(line, 2), ExtractPipeField(line, 3), ExtractPipeField(line, 4), ParseBool(ExtractPipeField(line, 5)), ExtractPipeField(line, 6));

			cursor = lineEnd + 1;
		}
	}

	protected string ExtractSection(string payload, string sectionId)
	{
		string marker = sectionId + "|";
		int start = payload.IndexOf(marker);
		if (start < 0)
			return "";

		start += marker.Length();
		int end = FindNextPayloadSection(payload, start);
		if (end < 0)
			end = payload.Length();

		return payload.Substring(start, end - start);
	}

	protected int FindNextPayloadSection(string payload, int start)
	{
		int best = -1;
		best = MinPositiveIndex(best, payload.IndexOfFrom(start, "\nTAB|"));
		best = MinPositiveIndex(best, payload.IndexOfFrom(start, "\nSTATUS|"));
		best = MinPositiveIndex(best, payload.IndexOfFrom(start, "\nRESULT|"));
		best = MinPositiveIndex(best, payload.IndexOfFrom(start, "\nACTION|"));
		best = MinPositiveIndex(best, payload.IndexOfFrom(start, "\nEND"));
		return best;
	}

	protected int MinPositiveIndex(int current, int candidate)
	{
		if (candidate < 0)
			return current;

		if (current < 0 || candidate < current)
			return candidate;

		return current;
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

	protected bool ParseBool(string value)
	{
		return value == "true" || value == "1";
	}

	protected int ResolveLocalPlayerId()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 1;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		if (playerIds.Count() == 0)
			return 1;

		return playerIds[0];
	}
}
