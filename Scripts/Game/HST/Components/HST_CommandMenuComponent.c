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

class HST_CommandMenuDrawCommandSet
{
	ref array<ref CanvasWidgetCommand> m_aCommands = {};
}

[ComponentEditorProps(category: "h-istasi", description: "Client-side h-istasi command menu key listener and widget controller")]
class HST_CommandMenuComponentClass : ScriptComponentClass
{
}

class HST_CommandMenuComponent : ScriptComponent
{
	static const string COMMAND_MENU_CUSTOM_ACTION = "HST_CommandMenu";
	static const string COMMAND_MENU_UP_ACTION = "MenuUp";
	static const string COMMAND_MENU_DOWN_ACTION = "MenuDown";
	static const string COMMAND_MENU_SELECT_ACTION = "MenuSelect";
	static const string COMMAND_MENU_BACK_ACTION = "MenuBack";
	static const string MENU_INPUT_CONTEXT = "InGameMenuContext";
	static const string MENU_CURSOR_CONTEXT = "InventoryContext";
	static const string COMMAND_MENU_KEYBOARD_BINDING = "keyboard:KC_I";
	static const ResourceName INPUT_CONFIG = "Configs/HST/Input/HST_Input.conf";
	static const ResourceName MENU_FONT = "";
	static const string MENU_LAYOUT = "UI/layouts/HST_CommandMenu.layout";
	static const int TAB_WIDGET_ID_BASE = 1000;
	static const int ACTION_WIDGET_ID_BASE = 2000;
	static const int CLOSE_WIDGET_ID = 9000;
	static const int CONTENT_PREV_WIDGET_ID = 9010;
	static const int CONTENT_NEXT_WIDGET_ID = 9011;
	static const int ACTION_PREV_WIDGET_ID = 9020;
	static const int ACTION_NEXT_WIDGET_ID = 9021;
	static const int CONTENT_PAGE_SIZE = 19;
	static const int ACTION_PAGE_SIZE = 8;
	static const int MAIN_PANEL_LEFT = 220;
	static const int MAIN_PANEL_TOP = 172;
	static const int MAIN_PANEL_WIDTH = 960;
	static const int MAIN_PANEL_HEIGHT = 764;
	static const int MAIN_CONTENT_LEFT = 244;
	static const int MAIN_CONTENT_WIDTH = 912;
	static const int RIGHT_PANEL_LEFT = 1260;
	static const int RIGHT_PANEL_WIDTH = 488;
	static const int RIGHT_TEXT_LEFT = 1280;
	static const int RIGHT_TEXT_WIDTH = 448;
	static const int ACTION_LIST_TOP = 492;
	static const int ACTION_ROW_HEIGHT = 44;
	static const int ACTION_ROW_STEP = 48;
	static const int ACTION_PAGER_TOP = 890;

	protected static HST_CommandMenuComponent s_LocalInstance;

	protected bool m_bMenuOpen;
	protected string m_sSelectedTab = "overview";
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
	protected ref array<string> m_aStatLabels = {};
	protected ref array<string> m_aStatValues = {};
	protected ref array<string> m_aStatTones = {};
	protected ref array<string> m_aSectionIds = {};
	protected ref array<string> m_aSectionTitles = {};
	protected ref array<string> m_aRowSectionIds = {};
	protected ref array<string> m_aRowLabels = {};
	protected ref array<string> m_aRowValues = {};
	protected ref array<string> m_aRowTones = {};
	protected ref array<string> m_aFeedLines = {};
	protected ref array<string> m_aFeedTones = {};
	protected ref array<string> m_aContentItemKinds = {};
	protected ref array<int> m_aContentItemSectionIndexes = {};
	protected ref array<int> m_aContentItemRowIndexes = {};
	protected ref array<Widget> m_aWidgets = {};
	protected ref array<ref HST_CommandMenuDrawCommandSet> m_aCanvasCommandSets = {};
	protected ref array<Widget> m_aExternalNotificationWidgets = {};
	protected ref array<ref HST_CommandMenuDrawCommandSet> m_aExternalNotificationCommandSets = {};
	protected ref array<string> m_aExternalNotificationTitleQueue = {};
	protected ref array<string> m_aExternalNotificationMessageQueue = {};
	protected ref array<float> m_aExternalNotificationDurationQueue = {};
	protected ref HST_CommandMenuWidgetHandler m_WidgetHandler;
	protected IEntity m_OwnerEntity;
	protected bool m_bIsLocalOwner;
	protected bool m_bInputRegistered;
	protected bool m_bInputConfigRegistered;
	protected bool m_bCustomBindingAttempted;
	protected bool m_bCustomBindingReady;
	protected float m_fInputRetryAccumulator;
	protected float m_fCommandMenuDebounceRemaining;
	protected bool m_bCommandMenuKeyDownLastFrame;
	protected bool m_bRawIKeyDownLastFrame;
	protected bool m_bLoggedCustomActionInput;
	protected bool m_bLoggedRawIKeyInput;
	protected bool m_bLoggedDuplicateToggle;
	protected float m_fExternalNotificationRemaining;
	protected int m_iContentPageStart;
	protected int m_iActionPageStart;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		m_bIsLocalOwner = IsLocalOwner(owner);
		BuildTabList();
		BuildActionList();
		ClearRichPayload();
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);

		if (m_bIsLocalOwner)
			BecomeLocalOwner();
	}

	override void OnDelete(IEntity owner)
	{
		if (m_bIsLocalOwner)
		{
			UnregisterInputListeners();
			ClearExternalNotificationWidgets();
			CloseMenu();
			if (s_LocalInstance == this)
				s_LocalInstance = null;
		}

		super.OnDelete(owner);
	}

	override void EOnInit(IEntity owner)
	{
		RefreshLocalOwnership(owner);

		if (m_bIsLocalOwner && !m_bInputRegistered)
			RegisterInputListeners();
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_fCommandMenuDebounceRemaining > 0)
			m_fCommandMenuDebounceRemaining = Math.Max(0, m_fCommandMenuDebounceRemaining - timeSlice);

		if (m_bIsLocalOwner)
			TickExternalNotification(timeSlice);

		if (!m_bIsLocalOwner)
		{
			m_fInputRetryAccumulator += timeSlice;
			if (m_fInputRetryAccumulator >= 0.25)
			{
				m_fInputRetryAccumulator = 0;
				RefreshLocalOwnership(owner);
			}

			return;
		}

		if (!m_bInputRegistered)
		{
			m_fInputRetryAccumulator += timeSlice;
			if (m_fInputRetryAccumulator >= 0.25)
			{
				m_fInputRetryAccumulator = 0;
				RegisterInputListeners();
			}
		}
		else if (!m_bCustomBindingReady)
		{
			m_fInputRetryAccumulator += timeSlice;
			if (m_fInputRetryAccumulator >= 0.25)
			{
				m_fInputRetryAccumulator = 0;
				InputManager retryInputManager = GetGame().GetInputManager();
				if (retryInputManager)
				{
					EnsureInputConfig(retryInputManager);
					EnsureIKeyBinding(retryInputManager);
				}
			}
		}

		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.ActivateContext(MENU_INPUT_CONTEXT);
			inputManager.ActivateAction(COMMAND_MENU_CUSTOM_ACTION);
			PollCommandMenuInput(inputManager);
			PollRawCommandMenuKey();

			if (m_bMenuOpen)
				inputManager.ActivateContext(MENU_CURSOR_CONTEXT);
		}
	}

	static HST_CommandMenuComponent GetLocalInstance()
	{
		return s_LocalInstance;
	}

	void ShowExternalNotification(string title, string message, float durationSeconds)
	{
		if (title.IsEmpty())
			title = "h-istasi";
		if (message.IsEmpty())
			return;

		if (m_fExternalNotificationRemaining > 0)
		{
			QueueExternalNotification(title, message, durationSeconds);
			return;
		}

		RenderExternalNotification(title, message, durationSeconds);
	}

	void OpenPetrosMenu()
	{
		OpenMenuToTab("petros");
	}

	void OpenArsenalMenu()
	{
		OpenMenuToTab("arsenal");
	}

	void OpenMenuToTab(string tabId)
	{
		tabId = NormalizeTabId(tabId);
		if (!tabId.IsEmpty())
			m_sSelectedTab = tabId;

		if (!m_bMenuOpen)
		{
			OpenMenu();
			return;
		}

		int tabIndex = m_aTabIds.Find(m_sSelectedTab);
		if (tabIndex >= 0)
		{
			SwitchToTab(tabIndex);
			return;
		}

		RequestSnapshot();
		RenderMenu();
	}

	void CloseMenuFromExternal()
	{
		CloseMenu();
	}

	void RunCommandFromContext(string tabId, string commandId, string argument = "")
	{
		tabId = NormalizeTabId(tabId);
		if (!tabId.IsEmpty())
			m_sSelectedTab = tabId;

		if (commandId.IsEmpty())
			return;

		m_sLastResult = "h-istasi command | requested " + commandId;
		ShowMenuHint(m_sLastResult, "h-istasi", 2.0);
		RequestAction(commandId, argument);
		if (m_bMenuOpen)
		{
			int tabIndex = m_aTabIds.Find(m_sSelectedTab);
			if (tabIndex >= 0)
				m_iSelectedControl = tabIndex;

			BuildActionList();
			RenderMenu();
		}
	}

	void OnServerSnapshot(string payload, string lastResult = "")
	{
		Print("h-istasi menu | snapshot received");
		bool showClosedResult = !m_bMenuOpen && !lastResult.IsEmpty() && lastResult != m_sLastResult;
		string previousTab = m_sSelectedTab;
		m_sLastPayload = payload;
		m_sLastResult = lastResult;
		ApplyHeaderFromPayload(payload);
		if (previousTab != m_sSelectedTab)
			ResetMenuScroll();
		m_sStatusText = ExtractSection(payload, "STATUS");
		if (m_sStatusText.IsEmpty())
			m_sStatusText = payload;

		string result = ExtractSection(payload, "RESULT");
		if (!result.IsEmpty())
			m_sLastResult = result;

		ParseTabsFromPayload(payload);
		ParseRichPayload(payload);
		ParseActionsFromPayload(payload);
		ClampContentPage();
		ClampActionPage();
		int selectedTabIndex = m_aTabIds.Find(m_sSelectedTab);
		if (selectedTabIndex >= 0 && m_iSelectedControl < m_aTabIds.Count())
			m_iSelectedControl = selectedTabIndex;

		if (m_iSelectedControl >= GetControlCount())
			m_iSelectedControl = Math.Max(0, GetControlCount() - 1);

		if (m_bMenuOpen)
			RenderMenu();
		else if (showClosedResult)
			ShowMenuHint(lastResult, "h-istasi", 3.0);
	}

	bool OnWidgetClicked(int widgetId)
	{
		if (!m_bMenuOpen)
			return false;

		if (widgetId == CLOSE_WIDGET_ID)
		{
			CloseMenu();
			return true;
		}

		if (widgetId == CONTENT_PREV_WIDGET_ID)
		{
			ScrollContentPage(-1);
			return true;
		}

		if (widgetId == CONTENT_NEXT_WIDGET_ID)
		{
			ScrollContentPage(1);
			return true;
		}

		if (widgetId == ACTION_PREV_WIDGET_ID)
		{
			ScrollActionPage(-1);
			return true;
		}

		if (widgetId == ACTION_NEXT_WIDGET_ID)
		{
			ScrollActionPage(1);
			return true;
		}

		if (widgetId >= TAB_WIDGET_ID_BASE && widgetId < ACTION_WIDGET_ID_BASE)
		{
			SwitchToTab(widgetId - TAB_WIDGET_ID_BASE);
			return true;
		}

		if (widgetId >= ACTION_WIDGET_ID_BASE)
		{
			int actionIndex = widgetId - ACTION_WIDGET_ID_BASE;
			if (actionIndex < 0 || actionIndex >= m_aActionCommands.Count())
				return false;

			m_iSelectedControl = m_aTabIds.Count() + actionIndex;
			ExecuteSelectedAction();
			return true;
		}

		return false;
	}

	protected bool RegisterInputListeners()
	{
		if (m_bInputRegistered)
			return true;

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return false;

		EnsureInputConfig(inputManager);
		EnsureIKeyBinding(inputManager);

		inputManager.AddActionListener(COMMAND_MENU_CUSTOM_ACTION, EActionTrigger.DOWN, OnCustomCommandMenuInput);
		inputManager.AddActionListener(COMMAND_MENU_UP_ACTION, EActionTrigger.DOWN, OnSelectPreviousInput);
		inputManager.AddActionListener(COMMAND_MENU_DOWN_ACTION, EActionTrigger.DOWN, OnSelectNextInput);
		inputManager.AddActionListener(COMMAND_MENU_SELECT_ACTION, EActionTrigger.DOWN, OnExecuteSelectedInput);
		inputManager.AddActionListener(COMMAND_MENU_BACK_ACTION, EActionTrigger.DOWN, OnCloseMenuInput);
		m_bInputRegistered = true;
		Print("h-istasi menu | input registered; custom ready " + m_bCustomBindingReady);
		return true;
	}

	protected void UnregisterInputListeners()
	{
		if (!m_bInputRegistered)
			return;

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.RemoveActionListener(COMMAND_MENU_CUSTOM_ACTION, EActionTrigger.DOWN, OnCustomCommandMenuInput);
		inputManager.RemoveActionListener(COMMAND_MENU_UP_ACTION, EActionTrigger.DOWN, OnSelectPreviousInput);
		inputManager.RemoveActionListener(COMMAND_MENU_DOWN_ACTION, EActionTrigger.DOWN, OnSelectNextInput);
		inputManager.RemoveActionListener(COMMAND_MENU_SELECT_ACTION, EActionTrigger.DOWN, OnExecuteSelectedInput);
		inputManager.RemoveActionListener(COMMAND_MENU_BACK_ACTION, EActionTrigger.DOWN, OnCloseMenuInput);
		m_bInputRegistered = false;
	}

	protected void OnCustomCommandMenuInput(float value, EActionTrigger reason)
	{
		if (reason != EActionTrigger.DOWN)
			return;

		if (!m_bLoggedCustomActionInput)
		{
			m_bLoggedCustomActionInput = true;
			Print("h-istasi menu | custom action listener fired");
		}

		TryToggleCommandMenu("custom action");
	}

	protected void PollCommandMenuInput(InputManager inputManager)
	{
		bool keyDown = false;
		string source = "";
		if (inputManager.GetActionTriggered(COMMAND_MENU_CUSTOM_ACTION))
		{
			keyDown = true;
			source = COMMAND_MENU_CUSTOM_ACTION;
		}

		if (keyDown && !m_bCommandMenuKeyDownLastFrame)
			TryToggleCommandMenu("poll " + source);

		m_bCommandMenuKeyDownLastFrame = keyDown;
	}

	protected void PollRawCommandMenuKey()
	{
		int keyState = Debug.KeyState(KeyCode.KC_I);
		bool keyDown = keyState != 0;
		if (keyDown && !m_bRawIKeyDownLastFrame)
		{
			if (!m_bLoggedRawIKeyInput)
			{
				m_bLoggedRawIKeyInput = true;
				Print("h-istasi menu | raw KC_I edge seen");
			}

			TryToggleCommandMenu("raw KC_I");
			Debug.ClearKey(KeyCode.KC_I);
		}

		m_bRawIKeyDownLastFrame = keyDown;
	}

	protected bool TryToggleCommandMenu(string source)
	{
		if (m_fCommandMenuDebounceRemaining > 0)
		{
			if (!m_bLoggedDuplicateToggle)
			{
				m_bLoggedDuplicateToggle = true;
				Print("h-istasi menu | ignored duplicate toggle from " + source);
			}

			return false;
		}

		bool wasOpen = m_bMenuOpen;
		m_fCommandMenuDebounceRemaining = 0.15;
		ToggleMenu();
		if (wasOpen)
			Print("h-istasi menu | closed by " + source);
		else
			Print("h-istasi menu | opened by " + source);

		return true;
	}

	protected void OnSelectPreviousInput(float value, EActionTrigger reason)
	{
		if (reason == EActionTrigger.DOWN)
			SelectPreviousAction();
	}

	protected void OnSelectNextInput(float value, EActionTrigger reason)
	{
		if (reason == EActionTrigger.DOWN)
			SelectNextAction();
	}

	protected void OnExecuteSelectedInput(float value, EActionTrigger reason)
	{
		if (reason == EActionTrigger.DOWN)
			ExecuteSelectedAction();
	}

	protected void OnCloseMenuInput(float value, EActionTrigger reason)
	{
		if (reason == EActionTrigger.DOWN)
			CloseMenu();
	}

	protected void BecomeLocalOwner()
	{
		if (s_LocalInstance == this)
			return;

		s_LocalInstance = this;
		m_fInputRetryAccumulator = 0;
		Print("h-istasi menu | local player menu component ready");
		RegisterInputListeners();
	}

	protected void RefreshLocalOwnership(IEntity owner)
	{
		if (m_bIsLocalOwner)
			return;

		if (!IsLocalOwner(owner))
			return;

		m_bIsLocalOwner = true;
		BecomeLocalOwner();
	}

	protected bool EnsureIKeyBinding(InputManager inputManager)
	{
		if (m_bCustomBindingReady)
			return m_bCustomBindingReady;

		InputBinding binding = inputManager.CreateUserBinding();
		if (!binding)
			return false;

		BaseContainer customAction = binding.FindAction(COMMAND_MENU_CUSTOM_ACTION);
		bool actionReady = customAction != null;
		if (!actionReady)
			actionReady = binding.CreateUserBinding(COMMAND_MENU_CUSTOM_ACTION, EInputDeviceType.KEYBOARD, "");

		if (!actionReady)
		{
			if (!m_bCustomBindingAttempted)
				Print("h-istasi menu | waiting for I-key custom input action", LogLevel.WARNING);

			m_bCustomBindingAttempted = true;
			return false;
		}

		m_bCustomBindingAttempted = true;
		Print("h-istasi menu | HST_CommandMenu input action ready");
		array<string> bindings = {};
		if (binding.GetBindings(COMMAND_MENU_CUSTOM_ACTION, bindings, EInputDeviceType.KEYBOARD, "", false))
		{
			foreach (string keyBinding : bindings)
			{
				if (keyBinding == COMMAND_MENU_KEYBOARD_BINDING)
				{
					m_bCustomBindingReady = true;
					return true;
				}
			}
		}

		binding.AddBinding(COMMAND_MENU_CUSTOM_ACTION, "", COMMAND_MENU_KEYBOARD_BINDING, "down");
		m_bCustomBindingReady = true;
		Print("h-istasi menu | bound I key to command menu");
		return true;
	}

	protected bool EnsureInputConfig(InputManager inputManager)
	{
		if (m_bInputConfigRegistered)
			return true;

		InputBinding binding = inputManager.CreateUserBinding();
		if (!binding)
			return false;

		array<ResourceName> customConfigs = {};
		binding.GetCustomConfigs(customConfigs);
		if (customConfigs.Find(INPUT_CONFIG) < 0)
		{
			customConfigs.Insert(INPUT_CONFIG);
			binding.SetCustomConfigs(customConfigs);
			Print("h-istasi menu | registered command menu input config");
		}

		m_bInputConfigRegistered = true;
		return true;
	}

	protected void BuildTabList()
	{
		m_aTabIds.Clear();
		m_aTabLabels.Clear();
		m_aTabEnabled.Clear();

		AddTab("setup", "Setup", true);
		AddTab("overview", "Overview", true);
		AddTab("petros", "HQ/Petros", true);
		AddTab("missions", "Missions", true);
		AddTab("map", "Map/War", true);
		AddTab("forces", "Forces", true);
		AddTab("arsenal", "Arsenal/Loot", true);
		AddTab("garage", "Garage", true);
		AddTab("members", "Members", true);
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

	protected void ResetMenuScroll()
	{
		m_iContentPageStart = 0;
		m_iActionPageStart = 0;
	}

	protected void ScrollContentPage(int direction)
	{
		if (!m_bMenuOpen)
			return;

		int pageSize = GetContentPageSize();
		int maxStart = ResolveMaxPageStart(m_aContentItemKinds.Count(), pageSize);
		m_iContentPageStart = ClampIntToRange(m_iContentPageStart + direction * pageSize, 0, maxStart);
		RenderMenu();
	}

	protected void ScrollActionPage(int direction)
	{
		if (!m_bMenuOpen)
			return;

		int pageSize = GetActionPageSize();
		int maxStart = ResolveMaxPageStart(m_aActionLabels.Count(), pageSize);
		m_iActionPageStart = ClampIntToRange(m_iActionPageStart + direction * pageSize, 0, maxStart);
		if (m_aActionLabels.Count() > 0)
			m_iSelectedControl = m_aTabIds.Count() + m_iActionPageStart;
		RenderMenu();
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
			m_sSelectedTab = "overview";

		m_sSelectedTab = NormalizeTabId(m_sSelectedTab);
		ResetMenuScroll();
		BuildTabList();
		BuildActionList();
		m_iSelectedControl = Math.Max(0, m_aTabIds.Find(m_sSelectedTab));
		m_sStatusText = "h-istasi menu | requesting server snapshot";
		RequestSnapshot();
		RenderMenu();
		Print("h-istasi menu | opened");
		ShowMenuHint("Command menu opened", "h-istasi", 2.0);
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
			ShowMenuHint(m_sLastResult, "h-istasi", 2.0);
			RenderMenu();
			return;
		}

		m_sLastResult = "h-istasi command | requested " + m_aActionLabels[actionIndex];
		ShowMenuHint(m_sLastResult, "h-istasi", 2.0);
		RequestAction(m_aActionCommands[actionIndex], m_aActionArguments[actionIndex]);
		RenderMenu();
	}

	protected void SwitchToTab(int tabIndex)
	{
		if (tabIndex < 0 || tabIndex >= m_aTabIds.Count())
			return;

		m_sSelectedTab = NormalizeTabId(m_aTabIds[tabIndex]);
		m_iSelectedControl = tabIndex;
		ResetMenuScroll();
		m_sStatusText = "h-istasi menu | requesting " + m_aTabLabels[tabIndex];
		if (!m_aTabEnabled[tabIndex])
			m_sStatusText = m_sStatusText + " (locked)";
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

		Widget root = CreateMenuRoot(workspace);
		if (!root)
			return;

		RenderHeader(workspace, root);
		RenderNavigation(workspace, root);
		RenderStats(workspace, root);
		RenderMainSections(workspace, root);
		RenderActivityPanel(workspace, root);
		RenderActions(workspace, root);
	}

	protected Widget CreateMenuRoot(WorkspaceWidget workspace)
	{
		Widget root = workspace.CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID, 24, 24, 1768, 960, WidgetFlags.VISIBLE, null, 2500);
		if (!root)
			return null;

		root.SetZOrder(2500);
		m_aWidgets.Insert(root);
		CreateRectWidget(workspace, root, 0, 0, 1768, 960, 0xF2080D12, 1.0, 0);
		return root;
	}

	protected void ShowMenuHint(string text, string title, float durationSeconds)
	{
	}

	protected void TickExternalNotification(float timeSlice)
	{
		if (m_fExternalNotificationRemaining <= 0)
			return;

		m_fExternalNotificationRemaining = Math.Max(0, m_fExternalNotificationRemaining - timeSlice);
		if (m_fExternalNotificationRemaining <= 0)
		{
			ClearExternalNotificationWidgets();
			ShowNextExternalNotification();
		}
	}

	protected void QueueExternalNotification(string title, string message, float durationSeconds)
	{
		m_aExternalNotificationTitleQueue.Insert(title);
		m_aExternalNotificationMessageQueue.Insert(message);
		m_aExternalNotificationDurationQueue.Insert(Math.Max(1.0, durationSeconds));

		while (m_aExternalNotificationTitleQueue.Count() > 16)
		{
			m_aExternalNotificationTitleQueue.Remove(0);
			m_aExternalNotificationMessageQueue.Remove(0);
			m_aExternalNotificationDurationQueue.Remove(0);
		}
	}

	protected void ShowNextExternalNotification()
	{
		if (m_aExternalNotificationTitleQueue.Count() == 0)
			return;

		string title = m_aExternalNotificationTitleQueue[0];
		string message = m_aExternalNotificationMessageQueue[0];
		float duration = m_aExternalNotificationDurationQueue[0];
		m_aExternalNotificationTitleQueue.Remove(0);
		m_aExternalNotificationMessageQueue.Remove(0);
		m_aExternalNotificationDurationQueue.Remove(0);
		RenderExternalNotification(title, message, duration);
	}

	protected void RenderExternalNotification(string title, string message, float durationSeconds)
	{
		ClearExternalNotificationWidgets();

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		Widget root = workspace.CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID, 510, 24, 900, 92, WidgetFlags.VISIBLE, null, 2850);
		if (!root)
			return;

		root.SetZOrder(2850);
		m_aExternalNotificationWidgets.Insert(root);
		CreateExternalRectWidget(workspace, root, 0, 0, 900, 92, 0xF21A232B, 1.0);
		CreateExternalRectWidget(workspace, root, 0, 88, 900, 4, 0xFFC4953B, 1.0);
		CreateExternalTextWidget(workspace, root, ShortenText(title, 44), 24, 10, 380, 24, 18, 0xFFF2D18B, true);
		CreateExternalTextWidget(workspace, root, ShortenText(message, 140), 24, 38, 852, 42, 16, 0xFFF2F4F0, false);
		m_fExternalNotificationRemaining = Math.Max(1.0, durationSeconds);
	}

	protected Widget CreateExternalRectWidget(WorkspaceWidget workspace, Widget parent, int left, int top, int width, int height, int color, float opacity)
	{
		Widget widget = workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE, null, 2851, parent);
		if (!widget)
			return null;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		SetupExternalCanvasRect(widget, width, height, color);
		widget.SetOpacity(opacity);
		return widget;
	}

	protected bool SetupExternalCanvasRect(Widget widget, int width, int height, int color)
	{
		CanvasWidget canvas = CanvasWidget.Cast(widget);
		if (!canvas)
			return false;

		HST_CommandMenuDrawCommandSet commandSet = new HST_CommandMenuDrawCommandSet();
		PolygonDrawCommand rectCommand = new PolygonDrawCommand();
		rectCommand.m_iColor = color;
		rectCommand.m_Vertices = BuildRectVertices(width, height);
		commandSet.m_aCommands.Insert(rectCommand);
		canvas.SetDrawCommands(commandSet.m_aCommands);
		m_aExternalNotificationCommandSets.Insert(commandSet);
		return true;
	}

	protected TextWidget CreateExternalTextWidget(WorkspaceWidget workspace, Widget parent, string text, int left, int top, int width, int height, int fontSize, int color, bool bold)
	{
		Widget widget = workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NO_LOCALIZATION, null, 2852, parent);
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
		return textWidget;
	}

	protected void RenderHeader(WorkspaceWidget workspace, Widget root)
	{
		CreateRectWidget(workspace, root, 0, 0, 1768, 84, 0xFF111920, 0.98, 0);
		CreateTextWidget(workspace, root, "h-istasi HQ", 24, 14, 280, 48, 34, 0xFFF2D18B, 0, true);
		CreateTextWidget(workspace, root, "FIA Resistance Command", 300, 28, 300, 28, 16, 0xFFB7C7D7, 0, false);
		CreateTextWidget(workspace, root, BuildSelectedTabTitle(), 720, 18, 560, 42, 26, 0xFFECE6D2, 0, true);
		CreateRectWidget(workspace, root, 1636, 16, 96, 46, 0xFF5F6C76, 0.96, CLOSE_WIDGET_ID);
		CreateTextWidget(workspace, root, "Close", 1650, 28, 68, 24, 17, 0xFFF4EBD6, CLOSE_WIDGET_ID, true);
	}

	protected void RenderStats(WorkspaceWidget workspace, Widget root)
	{
		int left = 220;
		int top = 96;
		int width = 170;
		for (int i = 0; i < m_aStatLabels.Count(); i++)
		{
			if (i >= 4)
				break;

			int x = left + i * 182;
			CreateRectWidget(workspace, root, x, top, width, 58, ToneBackgroundColor(m_aStatTones[i]), 0.96, 0);
			CreateTextWidget(workspace, root, ShortenText(m_aStatLabels[i], 18), x + 10, top + 7, width - 20, 20, 14, 0xFFCAD2D7, 0, false);
			CreateTextWidget(workspace, root, ShortenText(m_aStatValues[i], 18), x + 10, top + 31, width - 20, 24, 19, ToneTextColor(m_aStatTones[i]), 0, true);
		}
	}

	protected void RenderNavigation(WorkspaceWidget workspace, Widget root)
	{
		CreateRectWidget(workspace, root, 20, 96, 178, 840, 0xFF0F151B, 0.98, 0);
		CreateTextWidget(workspace, root, "Navigation", 40, 114, 138, 28, 19, 0xFFE6DECB, 0, true);
		for (int i = 0; i < m_aTabLabels.Count(); i++)
		{
			int top = 154 + i * 50;
			bool selected = m_aTabIds[i] == m_sSelectedTab;
			bool focused = m_iSelectedControl == i;
			int background = 0x00222222;
			if (selected)
				background = 0xFF604A24;
			else if (focused)
				background = 0xFF263341;

			float opacity = 0.01;
			if (selected || focused)
				opacity = 0.96;
			CreateRectWidget(workspace, root, 32, top - 8, 154, 42, background, opacity, TAB_WIDGET_ID_BASE + i);

			string label = m_aTabLabels[i];
			if (focused)
				label = "> " + label;

			int color = 0xFFCBD2D6;
			if (selected)
				color = 0xFFFFD98B;

			if (!m_aTabEnabled[i])
				color = 0xFF687179;

			CreateTextWidget(workspace, root, ShortenText(label, 16), 44, top, 130, 28, 16, color, TAB_WIDGET_ID_BASE + i, selected);
		}
	}

	protected void RenderMainSections(WorkspaceWidget workspace, Widget root)
	{
		int sectionCount = m_aSectionIds.Count();
		CreateRectWidget(workspace, root, MAIN_PANEL_LEFT, MAIN_PANEL_TOP, MAIN_PANEL_WIDTH, MAIN_PANEL_HEIGHT, 0xF31A232B, 0.98, 0);
		CreateRectWidget(workspace, root, MAIN_PANEL_LEFT, MAIN_PANEL_TOP, MAIN_PANEL_WIDTH, 4, 0xFFC4953B, 1.0, 0);
		if (sectionCount == 0)
		{
			CreateWrappedTextWidget(workspace, root, ShortenText(m_sStatusText, 900), MAIN_CONTENT_LEFT, 202, MAIN_CONTENT_WIDTH, 680, 16, 0xFFE0E0E0, 0, false);
			return;
		}

		ClampContentPage();
		int pageSize = GetContentPageSize();
		int rendered = 0;
		for (int i = m_iContentPageStart; i < m_aContentItemKinds.Count(); i++)
		{
			if (rendered >= pageSize)
				break;

			int rowTop = 198 + rendered * 36;
			RenderContentItem(workspace, root, i, MAIN_CONTENT_LEFT, rowTop, MAIN_CONTENT_WIDTH, 32);
			rendered++;
		}

		if (m_aContentItemKinds.Count() > pageSize)
			RenderContentPager(workspace, root, MAIN_CONTENT_LEFT, 900, MAIN_CONTENT_WIDTH);
	}

	protected void RenderContentItem(WorkspaceWidget workspace, Widget root, int contentIndex, int left, int top, int width, int height)
	{
		if (contentIndex < 0 || contentIndex >= m_aContentItemKinds.Count())
			return;

		string kind = m_aContentItemKinds[contentIndex];
		int sectionIndex = m_aContentItemSectionIndexes[contentIndex];
		int rowIndex = m_aContentItemRowIndexes[contentIndex];
		if (kind == "section")
		{
			CreateRectWidget(workspace, root, left - 8, top - 3, width + 16, height + 2, 0xFF263341, 0.72, 0);
			CreateTextWidget(workspace, root, ShortenText(m_aSectionTitles[sectionIndex], 68), left, top + 3, width, 24, 19, 0xFFEFE2C4, 0, true);
			return;
		}

		if (kind == "empty")
		{
			CreateTextWidget(workspace, root, "No entries", left + 18, top + 4, width - 36, 24, 15, 0xFF8D98A0, 0, false);
			return;
		}

		if (rowIndex < 0 || rowIndex >= m_aRowLabels.Count())
			return;

		CreateTextWidget(workspace, root, ShortenText(m_aRowLabels[rowIndex], 34), left, top + 2, 232, height, 14, 0xFFC4CDD3, 0, false);
		CreateWrappedTextWidget(workspace, root, ShortenText(m_aRowValues[rowIndex], 130), left + 250, top + 2, width - 250, height, 14, ToneTextColor(m_aRowTones[rowIndex]), 0, false);
	}

	protected void RenderContentPager(WorkspaceWidget workspace, Widget root, int left, int top, int width)
	{
		int pageSize = GetContentPageSize();
		int start = Math.Min(m_iContentPageStart + 1, m_aContentItemKinds.Count());
		int end = Math.Min(m_iContentPageStart + pageSize, m_aContentItemKinds.Count());
		string label = string.Format("Entries %1-%2 / %3", start, end, m_aContentItemKinds.Count());
		CreateTextWidget(workspace, root, label, left + 268, top + 4, 280, 24, 14, 0xFFAFBAC1, 0, false);

		bool canPrev = m_iContentPageStart > 0;
		bool canNext = m_iContentPageStart + pageSize < m_aContentItemKinds.Count();
		int prevColor = 0xFF2F3B45;
		int nextColor = 0xFF2F3B45;
		int prevTextColor = 0xFFE5E5E5;
		int nextTextColor = 0xFFE5E5E5;
		if (!canPrev)
		{
			prevColor = 0xFF1E252B;
			prevTextColor = 0xFF6D777F;
		}
		if (!canNext)
		{
			nextColor = 0xFF1E252B;
			nextTextColor = 0xFF6D777F;
		}

		CreateRectWidget(workspace, root, left, top, 120, 30, prevColor, 0.9, CONTENT_PREV_WIDGET_ID);
		CreateTextWidget(workspace, root, "Prev", left + 38, top + 5, 70, 20, 14, prevTextColor, CONTENT_PREV_WIDGET_ID, true);
		CreateRectWidget(workspace, root, left + width - 120, top, 120, 30, nextColor, 0.9, CONTENT_NEXT_WIDGET_ID);
		CreateTextWidget(workspace, root, "Next", left + width - 82, top + 5, 70, 20, 14, nextTextColor, CONTENT_NEXT_WIDGET_ID, true);
	}

	protected int CountRowsForSection(string sectionId)
	{
		int count = 0;
		foreach (string rowSectionId : m_aRowSectionIds)
		{
			if (rowSectionId == sectionId)
				count++;
		}

		return count;
	}

	protected void RenderActivityPanel(WorkspaceWidget workspace, Widget root)
	{
		CreateRectWidget(workspace, root, RIGHT_PANEL_LEFT, 96, RIGHT_PANEL_WIDTH, 318, 0xF31A232B, 0.98, 0);
		CreateRectWidget(workspace, root, RIGHT_PANEL_LEFT, 96, RIGHT_PANEL_WIDTH, 4, 0xFF50704A, 1.0, 0);
		CreateTextWidget(workspace, root, "Activity", RIGHT_TEXT_LEFT, 114, 180, 30, 20, 0xFFEFE2C4, 0, true);
		CreateWrappedTextWidget(workspace, root, ShortenText(BuildResultText(), 144), RIGHT_TEXT_LEFT, 154, RIGHT_TEXT_WIDTH, 76, 16, 0xFFD2E7B8, 0, false);
		CreateTextWidget(workspace, root, "Campaign Notes", RIGHT_TEXT_LEFT, 244, 214, 28, 20, 0xFFEFE2C4, 0, true);

		if (m_aFeedLines.Count() == 0)
		{
			CreateTextWidget(workspace, root, "Waiting for HQ traffic.", RIGHT_TEXT_LEFT, 282, RIGHT_TEXT_WIDTH, 28, 16, 0xFFAFBAC1, 0, false);
			return;
		}

		for (int i = 0; i < m_aFeedLines.Count(); i++)
		{
			if (i >= 4)
				break;

			CreateWrappedTextWidget(workspace, root, ShortenText(m_aFeedLines[i], 86), RIGHT_TEXT_LEFT, 282 + i * 30, RIGHT_TEXT_WIDTH, 28, 15, ToneTextColor(m_aFeedTones[i]), 0, false);
		}
	}

	protected void RenderActions(WorkspaceWidget workspace, Widget root)
	{
		CreateRectWidget(workspace, root, RIGHT_PANEL_LEFT, 434, RIGHT_PANEL_WIDTH, 502, 0xF31A232B, 0.98, 0);
		CreateRectWidget(workspace, root, RIGHT_PANEL_LEFT, 434, RIGHT_PANEL_WIDTH, 4, 0xFF8C4E43, 1.0, 0);
		CreateTextWidget(workspace, root, "Actions", RIGHT_TEXT_LEFT, 452, 170, 30, 20, 0xFFEFE2C4, 0, true);
		EnsureActionPageContainsSelection();
		int pageSize = GetActionPageSize();
		int rendered = 0;
		for (int i = m_iActionPageStart; i < m_aActionLabels.Count(); i++)
		{
			if (rendered >= pageSize)
				break;

			string prefix = "  ";
			int rowColor = 0x00222222;
			float rowOpacity = 0.01;
			if (m_iSelectedControl == m_aTabIds.Count() + i)
			{
				prefix = "> ";
				rowColor = 0xFF604A24;
				rowOpacity = 0.88;
			}
			int rowTop = ACTION_LIST_TOP + rendered * ACTION_ROW_STEP;
			CreateRectWidget(workspace, root, RIGHT_TEXT_LEFT - 8, rowTop, RIGHT_TEXT_WIDTH + 16, ACTION_ROW_HEIGHT, rowColor, rowOpacity, ACTION_WIDGET_ID_BASE + i);

			int color = 0xFFE5E5E5;
			string rowText = prefix + ShortenText(m_aActionLabels[i], 56);
			if (!m_aActionEnabled[i])
			{
				color = 0xFF8B9298;
				string disabledReason = "";
				if (i < m_aActionDisabledReasons.Count())
					disabledReason = m_aActionDisabledReasons[i];

				if (!disabledReason.IsEmpty())
					rowText = rowText + "\n  " + ShortenText(disabledReason, 56);
			}

			CreateWrappedTextWidget(workspace, root, rowText, RIGHT_TEXT_LEFT, rowTop + 5, RIGHT_TEXT_WIDTH, ACTION_ROW_HEIGHT - 8, 14, color, ACTION_WIDGET_ID_BASE + i, m_iSelectedControl == m_aTabIds.Count() + i);
			rendered++;
		}

		if (m_aActionLabels.Count() == 0)
			CreateTextWidget(workspace, root, "No commands available.", RIGHT_TEXT_LEFT, 496, RIGHT_TEXT_WIDTH, 30, 16, 0xFF9AA5AD, 0, false);
		else if (m_aActionLabels.Count() > pageSize)
			RenderActionPager(workspace, root);
	}

	protected void RenderActionPager(WorkspaceWidget workspace, Widget root)
	{
		int pageSize = GetActionPageSize();
		int start = Math.Min(m_iActionPageStart + 1, m_aActionLabels.Count());
		int end = Math.Min(m_iActionPageStart + pageSize, m_aActionLabels.Count());
		string label = string.Format("%1-%2 / %3", start, end, m_aActionLabels.Count());
		CreateTextWidget(workspace, root, label, RIGHT_TEXT_LEFT + 170, ACTION_PAGER_TOP + 5, 110, 24, 13, 0xFFAFBAC1, 0, false);

		bool canPrev = m_iActionPageStart > 0;
		bool canNext = m_iActionPageStart + pageSize < m_aActionLabels.Count();
		int prevColor = 0xFF2F3B45;
		int nextColor = 0xFF2F3B45;
		int prevTextColor = 0xFFE5E5E5;
		int nextTextColor = 0xFFE5E5E5;
		if (!canPrev)
		{
			prevColor = 0xFF1E252B;
			prevTextColor = 0xFF6D777F;
		}
		if (!canNext)
		{
			nextColor = 0xFF1E252B;
			nextTextColor = 0xFF6D777F;
		}

		CreateRectWidget(workspace, root, RIGHT_TEXT_LEFT, ACTION_PAGER_TOP, 86, 30, prevColor, 0.9, ACTION_PREV_WIDGET_ID);
		CreateTextWidget(workspace, root, "Prev", RIGHT_TEXT_LEFT + 26, ACTION_PAGER_TOP + 5, 52, 20, 13, prevTextColor, ACTION_PREV_WIDGET_ID, true);
		CreateRectWidget(workspace, root, RIGHT_TEXT_LEFT + RIGHT_TEXT_WIDTH - 86, ACTION_PAGER_TOP, 86, 30, nextColor, 0.9, ACTION_NEXT_WIDGET_ID);
		CreateTextWidget(workspace, root, "Next", RIGHT_TEXT_LEFT + RIGHT_TEXT_WIDTH - 60, ACTION_PAGER_TOP + 5, 52, 20, 13, nextTextColor, ACTION_NEXT_WIDGET_ID, true);
	}

	protected Widget CreateRectWidget(WorkspaceWidget workspace, Widget parent, int left, int top, int width, int height, int color, float opacity, int userId)
	{
		Widget widget = workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE, null, 2550, parent);
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

	protected bool SetupCanvasRect(Widget widget, int width, int height, int color)
	{
		CanvasWidget canvas = CanvasWidget.Cast(widget);
		if (!canvas)
			return false;

		HST_CommandMenuDrawCommandSet commandSet = new HST_CommandMenuDrawCommandSet();
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
		float rectWidth = width;
		float rectHeight = height;
		vertices.Insert(0.0);
		vertices.Insert(0.0);
		vertices.Insert(rectWidth);
		vertices.Insert(0.0);
		vertices.Insert(rectWidth);
		vertices.Insert(rectHeight);
		vertices.Insert(0.0);
		vertices.Insert(rectHeight);
		return vertices;
	}

	protected TextWidget CreateTextWidget(WorkspaceWidget workspace, Widget parent, string text, int left, int top, int width, int height, int fontSize, int color, int userId, bool bold)
	{
		Widget widget = workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NO_LOCALIZATION, null, 2600, parent);
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
		Widget widget = workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NO_LOCALIZATION, null, 2600, parent);
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

	protected void ClearExternalNotificationWidgets()
	{
		foreach (Widget widget : m_aExternalNotificationWidgets)
		{
			if (widget)
				widget.RemoveFromHierarchy();
		}

		m_aExternalNotificationWidgets.Clear();
		m_aExternalNotificationCommandSets.Clear();
		m_fExternalNotificationRemaining = 0;
	}

	protected void ClearRichPayload()
	{
		m_aStatLabels.Clear();
		m_aStatValues.Clear();
		m_aStatTones.Clear();
		m_aSectionIds.Clear();
		m_aSectionTitles.Clear();
		m_aRowSectionIds.Clear();
		m_aRowLabels.Clear();
		m_aRowValues.Clear();
		m_aRowTones.Clear();
		m_aFeedLines.Clear();
		m_aFeedTones.Clear();
		m_aContentItemKinds.Clear();
		m_aContentItemSectionIndexes.Clear();
		m_aContentItemRowIndexes.Clear();
	}

	protected string BuildResultText()
	{
		string inputStatus = "";
		if (!m_bCustomBindingReady)
			inputStatus = "Input\nI key binding pending\n";

		if (m_sLastResult.IsEmpty())
			return inputStatus + "Last action\n-";

		return inputStatus + "Last action\n" + m_sLastResult;
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

	protected int GetContentPageSize()
	{
		return CONTENT_PAGE_SIZE;
	}

	protected int GetActionPageSize()
	{
		return ACTION_PAGE_SIZE;
	}

	protected void ClampContentPage()
	{
		int maxStart = ResolveMaxPageStart(m_aContentItemKinds.Count(), GetContentPageSize());
		m_iContentPageStart = ClampIntToRange(m_iContentPageStart, 0, maxStart);
	}

	protected void ClampActionPage()
	{
		int maxStart = ResolveMaxPageStart(m_aActionLabels.Count(), GetActionPageSize());
		m_iActionPageStart = ClampIntToRange(m_iActionPageStart, 0, maxStart);
	}

	protected void EnsureActionPageContainsSelection()
	{
		int actionIndex = m_iSelectedControl - m_aTabIds.Count();
		if (actionIndex < 0)
		{
			ClampActionPage();
			return;
		}

		if (actionIndex >= m_aActionLabels.Count())
		{
			ClampActionPage();
			return;
		}

		int pageSize = GetActionPageSize();
		if (actionIndex < m_iActionPageStart || actionIndex >= m_iActionPageStart + pageSize)
			m_iActionPageStart = (actionIndex / pageSize) * pageSize;

		ClampActionPage();
	}

	protected int ResolveMaxPageStart(int itemCount, int pageSize)
	{
		if (itemCount <= pageSize || pageSize <= 0)
			return 0;

		return ((itemCount - 1) / pageSize) * pageSize;
	}

	protected int ClampIntToRange(int value, int minimum, int maximum)
	{
		if (value < minimum)
			return minimum;
		if (value > maximum)
			return maximum;

		return value;
	}

	protected string BuildSelectedTabTitle()
	{
		int tabIndex = m_aTabIds.Find(m_sSelectedTab);
		if (tabIndex >= 0)
			return m_aTabLabels[tabIndex];

		return "Overview";
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

	protected void ApplyHeaderFromPayload(string payload)
	{
		if (payload.IsEmpty() || !payload.Contains("HST_MENU|"))
			return;

		int lineEnd = payload.IndexOf("\n");
		if (lineEnd < 0)
			lineEnd = payload.Length();

		string line = payload.Substring(0, lineEnd);
		string tabId = NormalizeTabId(ExtractPipeField(line, 1));
		if (!tabId.IsEmpty() && tabId != "offline")
			m_sSelectedTab = tabId;
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
			AddTab(NormalizeTabId(ExtractPipeField(line, 1)), ExtractPipeField(line, 2), ParseBool(ExtractPipeField(line, 3)));
			cursor = lineEnd + 1;
		}
	}

	protected void ParseRichPayload(string payload)
	{
		ClearRichPayload();
		ParseStatsFromPayload(payload);
		ParseSectionsFromPayload(payload);
		ParseRowsFromPayload(payload);
		ParseFeedFromPayload(payload);
		BuildContentItemList();
	}

	protected void ParseStatsFromPayload(string payload)
	{
		int cursor;
		while (true)
		{
			int lineStart = payload.IndexOfFrom(cursor, "STAT|");
			if (lineStart < 0)
				break;

			int lineEnd = payload.IndexOfFrom(lineStart, "\n");
			if (lineEnd < 0)
				lineEnd = payload.Length();

			string line = payload.Substring(lineStart, lineEnd - lineStart);
			m_aStatLabels.Insert(ExtractPipeField(line, 1));
			m_aStatValues.Insert(ExtractPipeField(line, 2));
			m_aStatTones.Insert(ExtractPipeField(line, 3));
			cursor = lineEnd + 1;
		}
	}

	protected void ParseSectionsFromPayload(string payload)
	{
		int cursor;
		while (true)
		{
			int lineStart = payload.IndexOfFrom(cursor, "SECTION|");
			if (lineStart < 0)
				break;

			int lineEnd = payload.IndexOfFrom(lineStart, "\n");
			if (lineEnd < 0)
				lineEnd = payload.Length();

			string line = payload.Substring(lineStart, lineEnd - lineStart);
			m_aSectionIds.Insert(ExtractPipeField(line, 1));
			m_aSectionTitles.Insert(ExtractPipeField(line, 2));
			cursor = lineEnd + 1;
		}
	}

	protected void ParseRowsFromPayload(string payload)
	{
		int cursor;
		while (true)
		{
			int lineStart = payload.IndexOfFrom(cursor, "ROW|");
			if (lineStart < 0)
				break;

			int lineEnd = payload.IndexOfFrom(lineStart, "\n");
			if (lineEnd < 0)
				lineEnd = payload.Length();

			string line = payload.Substring(lineStart, lineEnd - lineStart);
			m_aRowSectionIds.Insert(ExtractPipeField(line, 1));
			m_aRowLabels.Insert(ExtractPipeField(line, 2));
			m_aRowValues.Insert(ExtractPipeField(line, 3));
			m_aRowTones.Insert(ExtractPipeField(line, 4));
			cursor = lineEnd + 1;
		}
	}

	protected void ParseFeedFromPayload(string payload)
	{
		int cursor;
		while (true)
		{
			int lineStart = payload.IndexOfFrom(cursor, "FEED|");
			if (lineStart < 0)
				break;

			int lineEnd = payload.IndexOfFrom(lineStart, "\n");
			if (lineEnd < 0)
				lineEnd = payload.Length();

			string line = payload.Substring(lineStart, lineEnd - lineStart);
			m_aFeedLines.Insert(ExtractPipeField(line, 1));
			m_aFeedTones.Insert(ExtractPipeField(line, 2));
			cursor = lineEnd + 1;
		}
	}

	protected void BuildContentItemList()
	{
		m_aContentItemKinds.Clear();
		m_aContentItemSectionIndexes.Clear();
		m_aContentItemRowIndexes.Clear();

		for (int sectionIndex = 0; sectionIndex < m_aSectionIds.Count(); sectionIndex++)
		{
			string sectionId = m_aSectionIds[sectionIndex];
			AddContentItem("section", sectionIndex, -1);
			bool hasRows = false;
			for (int rowIndex = 0; rowIndex < m_aRowSectionIds.Count(); rowIndex++)
			{
				if (m_aRowSectionIds[rowIndex] != sectionId)
					continue;

				AddContentItem("row", sectionIndex, rowIndex);
				hasRows = true;
			}

			if (!hasRows)
				AddContentItem("empty", sectionIndex, -1);
		}
	}

	protected void AddContentItem(string kind, int sectionIndex, int rowIndex)
	{
		m_aContentItemKinds.Insert(kind);
		m_aContentItemSectionIndexes.Insert(sectionIndex);
		m_aContentItemRowIndexes.Insert(rowIndex);
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
			string tabId = NormalizeTabId(ExtractPipeField(line, 1));
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
		best = MinPositiveIndex(best, payload.IndexOfFrom(start, "\nSTAT|"));
		best = MinPositiveIndex(best, payload.IndexOfFrom(start, "\nSECTION|"));
		best = MinPositiveIndex(best, payload.IndexOfFrom(start, "\nROW|"));
		best = MinPositiveIndex(best, payload.IndexOfFrom(start, "\nFEED|"));
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

	protected string NormalizeTabId(string tabId)
	{
		if (tabId == "general")
			return "overview";

		if (tabId == "commander")
			return "forces";

		if (tabId == "hq")
			return "petros";

		if (tabId == "vehicles")
			return "garage";

		return tabId;
	}

	protected int ToneTextColor(string tone)
	{
		if (tone == "good")
			return 0xFFB7E48F;

		if (tone == "warn")
			return 0xFFFFD166;

		if (tone == "bad")
			return 0xFFFF8E72;

		return 0xFFE2E6E8;
	}

	protected int ToneBackgroundColor(string tone)
	{
		if (tone == "good")
			return 0xFF263B2D;

		if (tone == "warn")
			return 0xFF423719;

		if (tone == "bad")
			return 0xFF43231F;

		return 0xFF222C34;
	}

	protected int ResolveLocalPlayerId()
	{
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (request && request.ResolveLocalPlayerId() > 0)
			return request.ResolveLocalPlayerId();

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 1;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		if (playerIds.Count() == 0)
			return 1;

		return playerIds[0];
	}

	protected bool IsLocalOwner(IEntity owner)
	{
		if (!owner)
			return false;

		BaseRplComponent rpl = BaseRplComponent.Cast(owner.FindComponent(BaseRplComponent));
		return !rpl || rpl.IsOwner();
	}
}
