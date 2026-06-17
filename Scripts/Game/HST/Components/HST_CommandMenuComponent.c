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

class HST_CommandMenuLayoutMetrics
{
	int m_iScreenW;
	int m_iScreenH;

	float m_fScale;
	bool m_bCompact;
	bool m_bVeryCompact;

	int m_iRootLeft;
	int m_iRootTop;
	int m_iRootWidth;
	int m_iRootHeight;

	int m_iMargin;
	int m_iGap;
	int m_iHeaderHeight;

	int m_iNavLeft;
	int m_iNavTop;
	int m_iNavWidth;
	int m_iNavHeight;

	int m_iStatsLeft;
	int m_iStatsTop;
	int m_iStatsWidth;
	int m_iStatsHeight;
	int m_iStatCardWidth;
	int m_iStatCardGap;

	int m_iMainLeft;
	int m_iMainTop;
	int m_iMainWidth;
	int m_iMainHeight;
	int m_iMainContentLeft;
	int m_iMainContentTop;
	int m_iMainContentWidth;
	int m_iMainContentHeight;

	int m_iRightLeft;
	int m_iRightTop;
	int m_iRightWidth;

	int m_iActivityTop;
	int m_iActivityHeight;
	int m_iActivityTextLeft;
	int m_iActivityTextWidth;

	int m_iActionsTop;
	int m_iActionsHeight;
	int m_iActionsTextLeft;
	int m_iActionsTextWidth;

	int m_iTabRowHeight;
	int m_iContentRowHeight;
	int m_iSectionRowHeight;
	int m_iActionRowHeight;
	int m_iFeedRowHeight;

	int m_iFontTiny;
	int m_iFontSmall;
	int m_iFontNormal;
	int m_iFontTitle;
	int m_iFontHeader;
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
	static const int TAB_WIDGET_ID_BASE = 10000;
	static const int ACTION_WIDGET_ID_BASE = 30000;
	static const int CLOSE_WIDGET_ID = 90000;
	static const int CONTENT_PREV_WIDGET_ID = 9010;
	static const int CONTENT_NEXT_WIDGET_ID = 9011;
	static const int ACTION_PREV_WIDGET_ID = 9020;
	static const int ACTION_NEXT_WIDGET_ID = 9021;
	static const int CONTENT_PAGE_SIZE = 19;
	static const int ACTION_PAGE_SIZE = 8;
	static const int ACTION_ROW_HEIGHT = 44;

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
	protected ref HST_CommandMenuLayoutMetrics m_Layout;
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
			actionReady = binding.CreateUserBinding(COMMAND_MENU_CUSTOM_ACTION, EInputDeviceType.KEYBOARD, INPUT_CONFIG);

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
		if (binding.GetBindings(COMMAND_MENU_CUSTOM_ACTION, bindings, EInputDeviceType.KEYBOARD, INPUT_CONFIG, false))
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

		binding.AddBinding(COMMAND_MENU_CUSTOM_ACTION, INPUT_CONFIG, COMMAND_MENU_KEYBOARD_BINDING, "down");
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
		int advance = pageSize;
		if (direction > 0)
			advance = ResolveVisibleContentPageAdvance(m_iContentPageStart);

		m_iContentPageStart = ClampIntToRange(m_iContentPageStart + direction * advance, 0, maxStart);
		RenderMenu();
	}

	protected void ScrollActionPage(int direction)
	{
		if (!m_bMenuOpen)
			return;

		int pageSize = GetActionPageSize();
		int maxStart = ResolveMaxPageStart(m_aActionLabels.Count(), pageSize);
		int advance = pageSize;
		if (direction > 0)
			advance = ResolveVisibleActionPageAdvance(m_iActionPageStart);

		m_iActionPageStart = ClampIntToRange(m_iActionPageStart + direction * advance, 0, maxStart);
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

		BuildResponsiveLayout(workspace);
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
		if (!m_Layout)
			BuildResponsiveLayout(workspace);

		Widget root = workspace.CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID, m_Layout.m_iRootLeft, m_Layout.m_iRootTop, m_Layout.m_iRootWidth, m_Layout.m_iRootHeight, WidgetFlags.VISIBLE, null, 2500);
		if (!root)
			return null;

		root.SetZOrder(2500);
		m_aWidgets.Insert(root);
		CreateRectWidget(workspace, root, 0, 0, m_Layout.m_iRootWidth, m_Layout.m_iRootHeight, 0xF2080D12, 1.0, 0);
		return root;
	}

	protected int ScalePx(float value)
	{
		if (!m_Layout)
			return Math.Round(value);

		return Math.Round(value * m_Layout.m_fScale);
	}

	protected int ScaleFont(float value)
	{
		if (!m_Layout)
			return Math.Round(value);

		int scaled = Math.Round(value * m_Layout.m_fScale);
		return ClampIntToRange(scaled, 9, Math.Round(value * 1.15));
	}

	protected int ClampLayoutInt(int value, int minValue, int maxValue)
	{
		if (maxValue < minValue)
			return Math.Max(1, maxValue);
		if (value < minValue)
			return minValue;
		if (value > maxValue)
			return maxValue;

		return value;
	}

	protected void BuildResponsiveLayout(WorkspaceWidget workspace)
	{
		if (!workspace)
			return;

		if (!m_Layout)
			m_Layout = new HST_CommandMenuLayoutMetrics();

		int screenW = Math.Max(1, Math.Round(workspace.GetWidth()));
		int screenH = Math.Max(1, Math.Round(workspace.GetHeight()));

		m_Layout.m_iScreenW = screenW;
		m_Layout.m_iScreenH = screenH;

		float sx = screenW / 1920.0;
		float sy = screenH / 1080.0;
		float scale = Math.Min(sx, sy);
		scale = Math.Clamp(scale, 0.70, 1.12);

		m_Layout.m_fScale = scale;
		m_Layout.m_bCompact = screenW < 1500 || screenH < 850;
		m_Layout.m_bVeryCompact = screenW < 1250 || screenH < 720;

		m_Layout.m_iMargin = ScalePx(24);
		if (m_Layout.m_bCompact)
			m_Layout.m_iMargin = ScalePx(18);
		if (m_Layout.m_bVeryCompact)
			m_Layout.m_iMargin = ScalePx(12);

		m_Layout.m_iGap = ScalePx(20);
		if (m_Layout.m_bCompact)
			m_Layout.m_iGap = ScalePx(14);

		m_Layout.m_iRootLeft = m_Layout.m_iMargin;
		m_Layout.m_iRootTop = m_Layout.m_iMargin;
		m_Layout.m_iRootWidth = Math.Max(1, screenW - m_Layout.m_iMargin * 2);
		m_Layout.m_iRootHeight = Math.Max(1, screenH - m_Layout.m_iMargin * 2);

		m_Layout.m_iHeaderHeight = ScalePx(78);
		if (m_Layout.m_bVeryCompact)
			m_Layout.m_iHeaderHeight = ScalePx(68);

		m_Layout.m_iNavLeft = ScalePx(20);
		m_Layout.m_iNavTop = m_Layout.m_iHeaderHeight + ScalePx(14);
		m_Layout.m_iNavWidth = ClampLayoutInt(Math.Round(m_Layout.m_iRootWidth * 0.115), ScalePx(160), ScalePx(210));
		m_Layout.m_iNavHeight = Math.Max(1, m_Layout.m_iRootHeight - m_Layout.m_iNavTop - ScalePx(20));

		m_Layout.m_iStatsLeft = m_Layout.m_iNavLeft + m_Layout.m_iNavWidth + m_Layout.m_iGap;
		m_Layout.m_iStatsTop = m_Layout.m_iHeaderHeight + ScalePx(12);
		m_Layout.m_iStatsHeight = ScalePx(62);

		int rightCandidateWidth = ClampLayoutInt(Math.Round(m_Layout.m_iRootWidth * 0.285), ScalePx(380), ScalePx(540));
		int mainLeft = m_Layout.m_iStatsLeft;
		int rightLeft = m_Layout.m_iRootWidth - rightCandidateWidth - ScalePx(20);
		int mainWidth = rightLeft - mainLeft - m_Layout.m_iGap;

		if (m_Layout.m_bVeryCompact || mainWidth < ScalePx(560))
		{
			m_Layout.m_bCompact = true;
			m_Layout.m_iRightLeft = mainLeft;
			m_Layout.m_iRightWidth = Math.Max(1, m_Layout.m_iRootWidth - mainLeft - ScalePx(20));

			m_Layout.m_iMainLeft = mainLeft;
			m_Layout.m_iMainTop = m_Layout.m_iStatsTop + m_Layout.m_iStatsHeight + ScalePx(14);
			m_Layout.m_iMainWidth = m_Layout.m_iRightWidth;
			int compactAvailableHeight = Math.Max(1, m_Layout.m_iRootHeight - m_Layout.m_iMainTop - ScalePx(20));
			int compactGap = ScalePx(14);
			m_Layout.m_iMainHeight = Math.Round(compactAvailableHeight * 0.54);
			m_Layout.m_iMainHeight = ClampLayoutInt(m_Layout.m_iMainHeight, Math.Min(ScalePx(140), compactAvailableHeight), Math.Max(1, compactAvailableHeight - compactGap * 2 - ScalePx(60)));

			m_Layout.m_iActivityTop = m_Layout.m_iMainTop + m_Layout.m_iMainHeight + ScalePx(14);
			int remainingAfterMain = Math.Max(1, m_Layout.m_iRootHeight - m_Layout.m_iActivityTop - ScalePx(20));
			m_Layout.m_iActivityHeight = ClampLayoutInt(ScalePx(118), Math.Min(ScalePx(52), remainingAfterMain), Math.Max(1, remainingAfterMain - compactGap - ScalePx(44)));

			m_Layout.m_iActionsTop = m_Layout.m_iActivityTop + m_Layout.m_iActivityHeight + ScalePx(14);
			m_Layout.m_iActionsHeight = Math.Max(1, m_Layout.m_iRootHeight - m_Layout.m_iActionsTop - ScalePx(20));
		}
		else
		{
			m_Layout.m_iRightWidth = rightCandidateWidth;
			m_Layout.m_iRightLeft = rightLeft;

			m_Layout.m_iMainLeft = mainLeft;
			m_Layout.m_iMainTop = m_Layout.m_iStatsTop + m_Layout.m_iStatsHeight + ScalePx(14);
			m_Layout.m_iMainWidth = mainWidth;
			m_Layout.m_iMainHeight = Math.Max(1, m_Layout.m_iRootHeight - m_Layout.m_iMainTop - ScalePx(20));

			m_Layout.m_iActivityTop = m_Layout.m_iStatsTop;
			int rightAvailableHeight = Math.Max(1, m_Layout.m_iRootHeight - m_Layout.m_iActivityTop - ScalePx(20));
			int activityMaxHeight = Math.Max(1, rightAvailableHeight - ScalePx(120));
			activityMaxHeight = Math.Min(activityMaxHeight, ScalePx(340));
			m_Layout.m_iActivityHeight = ClampLayoutInt(Math.Round(m_Layout.m_iRootHeight * 0.34), Math.Min(ScalePx(250), activityMaxHeight), activityMaxHeight);

			m_Layout.m_iActionsTop = m_Layout.m_iActivityTop + m_Layout.m_iActivityHeight + ScalePx(20);
			m_Layout.m_iActionsHeight = Math.Max(1, m_Layout.m_iRootHeight - m_Layout.m_iActionsTop - ScalePx(20));
		}

		m_Layout.m_iStatsWidth = m_Layout.m_iMainWidth;
		m_Layout.m_iStatCardGap = ScalePx(12);
		m_Layout.m_iStatCardWidth = Math.Max(1, (m_Layout.m_iStatsWidth - m_Layout.m_iStatCardGap * 3) / 4);

		m_Layout.m_iMainContentLeft = m_Layout.m_iMainLeft + ScalePx(24);
		m_Layout.m_iMainContentTop = m_Layout.m_iMainTop + ScalePx(30);
		m_Layout.m_iMainContentWidth = Math.Max(1, m_Layout.m_iMainWidth - ScalePx(48));
		m_Layout.m_iMainContentHeight = Math.Max(1, m_Layout.m_iMainHeight - ScalePx(72));

		m_Layout.m_iActivityTextLeft = m_Layout.m_iRightLeft + ScalePx(20);
		m_Layout.m_iActivityTextWidth = Math.Max(1, m_Layout.m_iRightWidth - ScalePx(40));

		m_Layout.m_iActionsTextLeft = m_Layout.m_iRightLeft + ScalePx(20);
		m_Layout.m_iActionsTextWidth = Math.Max(1, m_Layout.m_iRightWidth - ScalePx(40));

		m_Layout.m_iTabRowHeight = ScalePx(44);
		m_Layout.m_iContentRowHeight = ScalePx(44);
		m_Layout.m_iSectionRowHeight = ScalePx(38);
		m_Layout.m_iActionRowHeight = ScalePx(58);
		m_Layout.m_iFeedRowHeight = ScalePx(42);

		m_Layout.m_iFontTiny = ScaleFont(10);
		m_Layout.m_iFontSmall = ScaleFont(12);
		m_Layout.m_iFontNormal = ScaleFont(14);
		m_Layout.m_iFontTitle = ScaleFont(18);
		m_Layout.m_iFontHeader = ScaleFont(30);
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

		int screenW = Math.Max(1, Math.Round(workspace.GetWidth()));
		int left = Math.Max(12, (screenW / 2) - 450);
		int width = Math.Min(900, screenW - left * 2);
		if (width < 360)
			width = Math.Max(240, screenW - 24);

		Widget root = workspace.CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID, left, 24, width, 92, WidgetFlags.VISIBLE, null, 2850);
		if (!root)
			return;

		root.SetZOrder(2850);
		m_aExternalNotificationWidgets.Insert(root);
		CreateExternalRectWidget(workspace, root, 0, 0, width, 92, 0xF21A232B, 1.0);
		CreateExternalRectWidget(workspace, root, 0, 88, width, 4, 0xFFC4953B, 1.0);
		CreateExternalTextWidget(workspace, root, ShortenText(title, 44), 24, 10, width - 48, 24, 18, 0xFFF2D18B, true);
		CreateExternalTextWidget(workspace, root, ShortenText(message, 140), 24, 38, width - 48, 42, 16, 0xFFF2F4F0, false);
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
		if (!m_Layout)
			return;

		int w = m_Layout.m_iRootWidth;
		int h = m_Layout.m_iHeaderHeight;
		CreateRectWidget(workspace, root, 0, 0, w, h, 0xFF111920, 0.98, 0);
		CreateTextWidget(workspace, root, "h-istasi HQ", ScalePx(24), ScalePx(12), ScalePx(260), h - ScalePx(16), m_Layout.m_iFontHeader, 0xFFF2D18B, 0, true);

		if (!m_Layout.m_bVeryCompact)
			CreateTextWidget(workspace, root, "FIA Resistance Command", ScalePx(300), ScalePx(28), ScalePx(300), ScalePx(28), m_Layout.m_iFontNormal, 0xFFB7C7D7, 0, false);

		int closeW = ScalePx(96);
		int closeH = ScalePx(44);
		int closeLeft = w - closeW - ScalePx(24);
		int titleLeft = ScalePx(640);
		if (m_Layout.m_bCompact)
			titleLeft = ScalePx(420);
		if (m_Layout.m_bVeryCompact)
			titleLeft = ScalePx(260);

		CreateWrappedTextWidget(workspace, root, BuildSelectedTabTitle(), titleLeft, ScalePx(14), Math.Max(ScalePx(160), closeLeft - titleLeft - ScalePx(20)), ScalePx(46), m_Layout.m_iFontTitle, 0xFFECE6D2, 0, true);
		CreateRectWidget(workspace, root, closeLeft, ScalePx(16), closeW, closeH, 0xFF5F6C76, 0.96, CLOSE_WIDGET_ID);
		CreateTextWidget(workspace, root, "Close", closeLeft + ScalePx(18), ScalePx(28), closeW - ScalePx(28), ScalePx(24), m_Layout.m_iFontNormal, 0xFFF4EBD6, CLOSE_WIDGET_ID, true);
	}

	protected void RenderStats(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		int left = m_Layout.m_iStatsLeft;
		int top = m_Layout.m_iStatsTop;
		int width = m_Layout.m_iStatCardWidth;
		int gap = m_Layout.m_iStatCardGap;
		for (int i = 0; i < m_aStatLabels.Count(); i++)
		{
			if (i >= 4)
				break;

			int x = left + i * (width + gap);
			CreateRectWidget(workspace, root, x, top, width, m_Layout.m_iStatsHeight, ToneBackgroundColor(m_aStatTones[i]), 0.96, 0);
			CreateWrappedTextWidget(workspace, root, m_aStatLabels[i], x + ScalePx(10), top + ScalePx(7), width - ScalePx(20), ScalePx(20), m_Layout.m_iFontSmall, 0xFFCAD2D7, 0, false);
			CreateWrappedTextWidget(workspace, root, m_aStatValues[i], x + ScalePx(10), top + ScalePx(29), width - ScalePx(20), ScalePx(28), m_Layout.m_iFontNormal, ToneTextColor(m_aStatTones[i]), 0, true);
		}
	}

	protected void RenderNavigation(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		int left = m_Layout.m_iNavLeft;
		int top = m_Layout.m_iNavTop;
		int width = m_Layout.m_iNavWidth;
		int height = m_Layout.m_iNavHeight;

		CreateRectWidget(workspace, root, left, top, width, height, 0xFF0F151B, 0.98, 0);
		CreateTextWidget(workspace, root, "Navigation", left + ScalePx(18), top + ScalePx(16), width - ScalePx(36), ScalePx(28), m_Layout.m_iFontTitle, 0xFFE6DECB, 0, true);
		int rowTop = top + ScalePx(58);
		for (int i = 0; i < m_aTabLabels.Count(); i++)
		{
			int rowY = rowTop + i * (m_Layout.m_iTabRowHeight + ScalePx(6));
			if (rowY + m_Layout.m_iTabRowHeight > top + height - ScalePx(12))
				break;

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
			CreateRectWidget(workspace, root, left + ScalePx(12), rowY, width - ScalePx(24), m_Layout.m_iTabRowHeight, background, opacity, TAB_WIDGET_ID_BASE + i);

			string label = m_aTabLabels[i];
			if (focused)
				label = "> " + label;

			int color = 0xFFCBD2D6;
			if (selected)
				color = 0xFFFFD98B;

			if (!m_aTabEnabled[i])
				color = 0xFF687179;

			CreateWrappedTextWidget(workspace, root, label, left + ScalePx(22), rowY + ScalePx(8), width - ScalePx(44), m_Layout.m_iTabRowHeight - ScalePx(8), m_Layout.m_iFontNormal, color, TAB_WIDGET_ID_BASE + i, selected);
		}
	}

	protected void RenderMainSections(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		int sectionCount = m_aSectionIds.Count();
		CreateRectWidget(workspace, root, m_Layout.m_iMainLeft, m_Layout.m_iMainTop, m_Layout.m_iMainWidth, m_Layout.m_iMainHeight, 0xF31A232B, 0.98, 0);
		CreateRectWidget(workspace, root, m_Layout.m_iMainLeft, m_Layout.m_iMainTop, m_Layout.m_iMainWidth, ScalePx(4), 0xFFC4953B, 1.0, 0);
		if (sectionCount == 0)
		{
			CreateWrappedTextWidget(workspace, root, m_sStatusText, m_Layout.m_iMainContentLeft, m_Layout.m_iMainContentTop, m_Layout.m_iMainContentWidth, m_Layout.m_iMainContentHeight, m_Layout.m_iFontNormal, 0xFFE0E0E0, 0, false);
			return;
		}

		ClampContentPage();
		int y = m_Layout.m_iMainContentTop;
		int contentBottom = m_Layout.m_iMainContentTop + m_Layout.m_iMainContentHeight;
		int rendered;
		for (int i = m_iContentPageStart; i < m_aContentItemKinds.Count(); i++)
		{
			int rowHeight = MeasureContentItemHeight(i, m_Layout.m_iMainContentWidth);
			if (rendered > 0 && y + rowHeight > contentBottom)
				break;

			RenderContentItem(workspace, root, i, m_Layout.m_iMainContentLeft, y, m_Layout.m_iMainContentWidth, rowHeight);
			y += rowHeight + ScalePx(6);
			rendered++;
		}

		if (m_iContentPageStart > 0 || m_iContentPageStart + rendered < m_aContentItemKinds.Count())
			RenderContentPager(workspace, root, m_Layout.m_iMainContentLeft, m_Layout.m_iMainTop + m_Layout.m_iMainHeight - ScalePx(38), m_Layout.m_iMainContentWidth, rendered);
	}

	protected int MeasureContentItemHeight(int contentIndex, int width)
	{
		if (!m_Layout)
			return 32;

		if (contentIndex < 0 || contentIndex >= m_aContentItemKinds.Count())
			return m_Layout.m_iContentRowHeight;

		string kind = m_aContentItemKinds[contentIndex];
		if (kind == "section")
			return m_Layout.m_iSectionRowHeight;
		if (kind == "empty")
			return ScalePx(34);

		int rowIndex = m_aContentItemRowIndexes[contentIndex];
		if (rowIndex < 0 || rowIndex >= m_aRowValues.Count())
			return m_Layout.m_iContentRowHeight;

		string value = m_aRowValues[rowIndex];
		int approxCharsPerLine = Math.Max(24, (width - ScalePx(260)) / Math.Max(1, ScalePx(7)));
		int lines = 1;
		if (value.Length() > approxCharsPerLine)
			lines = 2;
		if (value.Length() > approxCharsPerLine * 2)
			lines = 3;

		return Math.Max(m_Layout.m_iContentRowHeight, ScalePx(24 + lines * 18));
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
			CreateRectWidget(workspace, root, left - ScalePx(8), top - ScalePx(3), width + ScalePx(16), height + ScalePx(2), 0xFF263341, 0.72, 0);
			CreateWrappedTextWidget(workspace, root, m_aSectionTitles[sectionIndex], left, top + ScalePx(6), width, height - ScalePx(6), m_Layout.m_iFontTitle, 0xFFEFE2C4, 0, true);
			return;
		}

		if (kind == "empty")
		{
			CreateTextWidget(workspace, root, "No entries", left + ScalePx(18), top + ScalePx(4), width - ScalePx(36), ScalePx(24), m_Layout.m_iFontNormal, 0xFF8D98A0, 0, false);
			return;
		}

		if (rowIndex < 0 || rowIndex >= m_aRowLabels.Count())
			return;

		int labelWidth = ClampLayoutInt(Math.Round(width * 0.34), ScalePx(180), ScalePx(300));
		int valueLeft = left + labelWidth + ScalePx(18);
		int valueWidth = width - labelWidth - ScalePx(18);

		CreateWrappedTextWidget(workspace, root, m_aRowLabels[rowIndex], left, top + ScalePx(4), labelWidth, height - ScalePx(8), m_Layout.m_iFontNormal, 0xFFC4CDD3, 0, false);
		CreateWrappedTextWidget(workspace, root, m_aRowValues[rowIndex], valueLeft, top + ScalePx(4), valueWidth, height - ScalePx(8), m_Layout.m_iFontNormal, ToneTextColor(m_aRowTones[rowIndex]), 0, false);
	}

	protected void RenderContentPager(WorkspaceWidget workspace, Widget root, int left, int top, int width, int rendered)
	{
		int pageSize = GetContentPageSize();
		int start = Math.Min(m_iContentPageStart + 1, m_aContentItemKinds.Count());
		int end = Math.Min(m_iContentPageStart + Math.Max(1, rendered), m_aContentItemKinds.Count());
		string label = string.Format("Entries %1-%2 / %3", start, end, m_aContentItemKinds.Count());
		int labelLeft = left + ScalePx(132);
		int labelWidth = Math.Max(ScalePx(80), width - ScalePx(264));
		CreateTextWidget(workspace, root, label, labelLeft, top + ScalePx(4), labelWidth, ScalePx(24), m_Layout.m_iFontSmall, 0xFFAFBAC1, 0, false);

		bool canPrev = m_iContentPageStart > 0;
		bool canNext = m_iContentPageStart + Math.Max(1, rendered) < m_aContentItemKinds.Count();
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

		CreateRectWidget(workspace, root, left, top, ScalePx(120), ScalePx(30), prevColor, 0.9, CONTENT_PREV_WIDGET_ID);
		CreateTextWidget(workspace, root, "Prev", left + ScalePx(38), top + ScalePx(5), ScalePx(70), ScalePx(20), m_Layout.m_iFontSmall, prevTextColor, CONTENT_PREV_WIDGET_ID, true);
		CreateRectWidget(workspace, root, left + width - ScalePx(120), top, ScalePx(120), ScalePx(30), nextColor, 0.9, CONTENT_NEXT_WIDGET_ID);
		CreateTextWidget(workspace, root, "Next", left + width - ScalePx(82), top + ScalePx(5), ScalePx(70), ScalePx(20), m_Layout.m_iFontSmall, nextTextColor, CONTENT_NEXT_WIDGET_ID, true);
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
		if (!m_Layout)
			return;

		CreateRectWidget(workspace, root, m_Layout.m_iRightLeft, m_Layout.m_iActivityTop, m_Layout.m_iRightWidth, m_Layout.m_iActivityHeight, 0xF31A232B, 0.98, 0);
		CreateRectWidget(workspace, root, m_Layout.m_iRightLeft, m_Layout.m_iActivityTop, m_Layout.m_iRightWidth, ScalePx(4), 0xFF50704A, 1.0, 0);
		CreateTextWidget(workspace, root, "Activity", m_Layout.m_iActivityTextLeft, m_Layout.m_iActivityTop + ScalePx(18), ScalePx(180), ScalePx(30), m_Layout.m_iFontTitle, 0xFFEFE2C4, 0, true);
		int resultTop = m_Layout.m_iActivityTop + ScalePx(54);
		int resultHeight = ScalePx(66);
		if (m_Layout.m_bCompact)
			resultHeight = ScalePx(46);
		CreateWrappedTextWidget(workspace, root, BuildResultText(), m_Layout.m_iActivityTextLeft, resultTop, m_Layout.m_iActivityTextWidth, resultHeight, m_Layout.m_iFontNormal, 0xFFD2E7B8, 0, false);

		int feedTop = resultTop + resultHeight + ScalePx(10);
		if (feedTop + ScalePx(52) > m_Layout.m_iActivityTop + m_Layout.m_iActivityHeight)
			return;

		CreateTextWidget(workspace, root, "Campaign Notes", m_Layout.m_iActivityTextLeft, feedTop, ScalePx(214), ScalePx(28), m_Layout.m_iFontTitle, 0xFFEFE2C4, 0, true);

		if (m_aFeedLines.Count() == 0)
		{
			CreateTextWidget(workspace, root, "Waiting for HQ traffic.", m_Layout.m_iActivityTextLeft, feedTop + ScalePx(36), m_Layout.m_iActivityTextWidth, ScalePx(28), m_Layout.m_iFontNormal, 0xFFAFBAC1, 0, false);
			return;
		}

		int listTop = feedTop + ScalePx(36);
		int listBottom = m_Layout.m_iActivityTop + m_Layout.m_iActivityHeight - ScalePx(10);
		int visibleFeedRows = Math.Max(1, (listBottom - listTop) / Math.Max(1, m_Layout.m_iFeedRowHeight + ScalePx(6)));
		for (int i = 0; i < m_aFeedLines.Count(); i++)
		{
			if (i >= visibleFeedRows)
				break;

			CreateWrappedTextWidget(workspace, root, m_aFeedLines[i], m_Layout.m_iActivityTextLeft, listTop + i * (m_Layout.m_iFeedRowHeight + ScalePx(6)), m_Layout.m_iActivityTextWidth, m_Layout.m_iFeedRowHeight, m_Layout.m_iFontSmall, ToneTextColor(m_aFeedTones[i]), 0, false);
		}
	}

	protected void RenderActions(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		CreateRectWidget(workspace, root, m_Layout.m_iRightLeft, m_Layout.m_iActionsTop, m_Layout.m_iRightWidth, m_Layout.m_iActionsHeight, 0xF31A232B, 0.98, 0);
		CreateRectWidget(workspace, root, m_Layout.m_iRightLeft, m_Layout.m_iActionsTop, m_Layout.m_iRightWidth, ScalePx(4), 0xFF8C4E43, 1.0, 0);
		CreateTextWidget(workspace, root, "Actions", m_Layout.m_iActionsTextLeft, m_Layout.m_iActionsTop + ScalePx(18), ScalePx(170), ScalePx(30), m_Layout.m_iFontTitle, 0xFFEFE2C4, 0, true);
		EnsureActionPageContainsSelection();
		int listTop = m_Layout.m_iActionsTop + ScalePx(58);
		int listBottom = m_Layout.m_iActionsTop + m_Layout.m_iActionsHeight - ScalePx(42);
		int y = listTop;
		int rendered;
		for (int i = m_iActionPageStart; i < m_aActionLabels.Count(); i++)
		{
			int rowHeight = MeasureActionRowHeight(i, m_Layout.m_iActionsTextWidth);
			if (rendered > 0 && y + rowHeight > listBottom)
				break;

			RenderActionRow(workspace, root, i, m_Layout.m_iActionsTextLeft, y, m_Layout.m_iActionsTextWidth, rowHeight);
			y += rowHeight + ScalePx(8);
			rendered++;
		}

		if (m_aActionLabels.Count() == 0)
			CreateTextWidget(workspace, root, "No commands available.", m_Layout.m_iActionsTextLeft, listTop + ScalePx(8), m_Layout.m_iActionsTextWidth, ScalePx(30), m_Layout.m_iFontNormal, 0xFF9AA5AD, 0, false);
		else if (m_iActionPageStart > 0 || m_iActionPageStart + rendered < m_aActionLabels.Count())
			RenderActionPager(workspace, root, rendered);
	}

	protected int MeasureActionRowHeight(int actionIndex, int width)
	{
		if (!m_Layout)
			return ACTION_ROW_HEIGHT;

		if (actionIndex < 0 || actionIndex >= m_aActionLabels.Count())
			return m_Layout.m_iActionRowHeight;

		string label = m_aActionLabels[actionIndex];
		string disabledReason = "";
		if (actionIndex < m_aActionDisabledReasons.Count())
			disabledReason = m_aActionDisabledReasons[actionIndex];

		int charsPerLine = Math.Max(24, width / Math.Max(1, ScalePx(8)));
		int lines = 1;
		if (label.Length() > charsPerLine)
			lines++;

		if (actionIndex < m_aActionEnabled.Count() && !m_aActionEnabled[actionIndex] && !disabledReason.IsEmpty())
		{
			lines++;
			if (disabledReason.Length() > charsPerLine)
				lines++;
		}

		return Math.Max(m_Layout.m_iActionRowHeight, ScalePx(22 + lines * 18));
	}

	protected void RenderActionRow(WorkspaceWidget workspace, Widget root, int actionIndex, int left, int top, int width, int height)
	{
		bool focused = m_iSelectedControl == m_aTabIds.Count() + actionIndex;
		int rowColor = 0x00222222;
		float rowOpacity = 0.01;
		if (focused)
		{
			rowColor = 0xFF604A24;
			rowOpacity = 0.88;
		}

		CreateRectWidget(workspace, root, left - ScalePx(8), top, width + ScalePx(16), height, rowColor, rowOpacity, ACTION_WIDGET_ID_BASE + actionIndex);

		int color = 0xFFE5E5E5;
		string rowText = m_aActionLabels[actionIndex];
		if (focused)
			rowText = "> " + rowText;

		if (actionIndex < m_aActionEnabled.Count() && !m_aActionEnabled[actionIndex])
		{
			color = 0xFF8B9298;
			string disabledReason = "";
			if (actionIndex < m_aActionDisabledReasons.Count())
				disabledReason = m_aActionDisabledReasons[actionIndex];

			if (!disabledReason.IsEmpty())
				rowText = rowText + "\n" + disabledReason;
		}

		CreateWrappedTextWidget(workspace, root, rowText, left, top + ScalePx(7), width, height - ScalePx(10), m_Layout.m_iFontNormal, color, ACTION_WIDGET_ID_BASE + actionIndex, focused);
	}

	protected void RenderActionPager(WorkspaceWidget workspace, Widget root, int rendered)
	{
		int pageSize = GetActionPageSize();
		int start = Math.Min(m_iActionPageStart + 1, m_aActionLabels.Count());
		int end = Math.Min(m_iActionPageStart + Math.Max(1, rendered), m_aActionLabels.Count());
		string label = string.Format("%1-%2 / %3", start, end, m_aActionLabels.Count());
		int pagerTop = m_Layout.m_iActionsTop + m_Layout.m_iActionsHeight - ScalePx(34);
		int labelLeft = m_Layout.m_iActionsTextLeft + ScalePx(96);
		int labelWidth = Math.Max(ScalePx(70), m_Layout.m_iActionsTextWidth - ScalePx(192));
		CreateTextWidget(workspace, root, label, labelLeft, pagerTop + ScalePx(5), labelWidth, ScalePx(24), m_Layout.m_iFontSmall, 0xFFAFBAC1, 0, false);

		bool canPrev = m_iActionPageStart > 0;
		bool canNext = m_iActionPageStart + Math.Max(1, rendered) < m_aActionLabels.Count();
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

		CreateRectWidget(workspace, root, m_Layout.m_iActionsTextLeft, pagerTop, ScalePx(86), ScalePx(30), prevColor, 0.9, ACTION_PREV_WIDGET_ID);
		CreateTextWidget(workspace, root, "Prev", m_Layout.m_iActionsTextLeft + ScalePx(26), pagerTop + ScalePx(5), ScalePx(52), ScalePx(20), m_Layout.m_iFontSmall, prevTextColor, ACTION_PREV_WIDGET_ID, true);
		CreateRectWidget(workspace, root, m_Layout.m_iActionsTextLeft + m_Layout.m_iActionsTextWidth - ScalePx(86), pagerTop, ScalePx(86), ScalePx(30), nextColor, 0.9, ACTION_NEXT_WIDGET_ID);
		CreateTextWidget(workspace, root, "Next", m_Layout.m_iActionsTextLeft + m_Layout.m_iActionsTextWidth - ScalePx(60), pagerTop + ScalePx(5), ScalePx(52), ScalePx(20), m_Layout.m_iFontSmall, nextTextColor, ACTION_NEXT_WIDGET_ID, true);
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
		if (m_Layout)
			return Math.Max(1, m_Layout.m_iMainContentHeight / Math.Max(1, m_Layout.m_iContentRowHeight + ScalePx(6)));

		return CONTENT_PAGE_SIZE;
	}

	protected int GetActionPageSize()
	{
		if (m_Layout)
			return Math.Max(1, (m_Layout.m_iActionsHeight - ScalePx(96)) / Math.Max(1, m_Layout.m_iActionRowHeight + ScalePx(8)));

		return ACTION_PAGE_SIZE;
	}

	protected int ResolveVisibleContentPageAdvance(int startIndex)
	{
		if (!m_Layout)
			return GetContentPageSize();

		int y;
		int bottom = m_Layout.m_iMainContentHeight;
		int rendered;
		for (int i = startIndex; i < m_aContentItemKinds.Count(); i++)
		{
			int rowHeight = MeasureContentItemHeight(i, m_Layout.m_iMainContentWidth);
			if (rendered > 0 && y + rowHeight > bottom)
				break;

			y += rowHeight + ScalePx(6);
			rendered++;
		}

		return Math.Max(1, rendered);
	}

	protected int ResolveVisibleActionPageAdvance(int startIndex)
	{
		if (!m_Layout)
			return GetActionPageSize();

		int y;
		int bottom = Math.Max(ScalePx(60), m_Layout.m_iActionsHeight - ScalePx(100));
		int rendered;
		for (int i = startIndex; i < m_aActionLabels.Count(); i++)
		{
			int rowHeight = MeasureActionRowHeight(i, m_Layout.m_iActionsTextWidth);
			if (rendered > 0 && y + rowHeight > bottom)
				break;

			y += rowHeight + ScalePx(8);
			rendered++;
		}

		return Math.Max(1, rendered);
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

		int pageSize = ResolveVisibleActionPageAdvance(m_iActionPageStart);
		if (actionIndex < m_iActionPageStart || actionIndex >= m_iActionPageStart + pageSize)
			m_iActionPageStart = actionIndex;

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
