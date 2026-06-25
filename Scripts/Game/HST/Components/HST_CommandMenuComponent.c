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
	static const ResourceName COMMAND_MENU_LAYOUT = "{A7B8C9D001234550}UI/layouts/HST_CommandMenu.layout";
	static const ResourceName SCRIPTED_PANEL_ROOT_LAYOUT = "{B55C6FB34BF95000}UI/layouts/HST_ScriptedPanelRoot.layout";
	static const ResourceName VERTICAL_SCROLL_LIST_LAYOUT = "{A7B8C9D001234560}UI/layouts/HST_VerticalScrollList.layout";
	static const ResourceName WRAP_SCROLL_GRID_LAYOUT = "{A7B8C9D001234570}UI/layouts/HST_WrapScrollGrid.layout";
	static const ResourceName UI_SOLID_WHITE = "{56137CA0F2D3ACE6}Assets/Images/solid_white_square.edds";
	static const ResourceName COMMAND_SECTION_ROW_LAYOUT = "{A7B8C9D001234580}UI/layouts/HST/Rows/HST_CommandSectionRow.layout";
	static const ResourceName COMMAND_DATA_ROW_LAYOUT = "{A7B8C9D001234590}UI/layouts/HST/Rows/HST_CommandDataRow.layout";
	static const ResourceName COMMAND_DATA_ROW_COMPACT_LAYOUT = "{A7B8C9D0012345A0}UI/layouts/HST/Rows/HST_CommandDataRowCompact.layout";
	static const ResourceName COMMAND_ACTION_ROW_LAYOUT = "{A7B8C9D0012345B0}UI/layouts/HST/Rows/HST_CommandActionRow.layout";
	static const ResourceName COMMAND_FEED_ROW_LAYOUT = "{A7B8C9D0012345C0}UI/layouts/HST/Rows/HST_CommandFeedRow.layout";
	static const float POST_SETUP_INPUT_RECOVERY_SECONDS = 3.0;
	static const float POST_SETUP_INPUT_REBIND_INTERVAL_SECONDS = 0.25;
	static const int COMMAND_ACTION_ROW_STRIDE = 70;
	static const int TAB_WIDGET_ID_BASE = 10000;
	static const int ACTION_WIDGET_ID_BASE = 30000;
	static const int CLOSE_WIDGET_ID = 90000;

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
	protected bool m_bWasSetupBlocking;
	protected bool m_bLoggedCustomActionInput;
	protected bool m_bLoggedRawIKeyInput;
	protected bool m_bLoggedDuplicateToggle;
	protected bool m_bDebugLoggingEnabled;
	protected int m_iLoggedLayoutW;
	protected int m_iLoggedLayoutH;
	protected int m_iRowCreateLogCount;
	protected bool m_bRowCreateLogSuppressed;
	protected float m_fExternalNotificationRemaining;
	protected float m_fPostSetupInputRecoveryRemaining;
	protected float m_fPostSetupInputRecoveryAccumulator;
	protected ScrollLayoutWidget m_ContentScroll;
	protected ScrollLayoutWidget m_ActionScroll;
	protected ScrollLayoutWidget m_FeedScroll;
	protected float m_fContentScrollY;
	protected float m_fActionScrollY;
	protected float m_fFeedScrollY;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		m_bIsLocalOwner = IsLocalOwner(owner);
		m_bDebugLoggingEnabled = HST_RuntimeSettingsService.LoadDebugLoggingEnabledQuiet();
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
			CloseMenu("component delete");
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

		if (HST_SetupMapComponent.IsSetupBlocking())
		{
			m_bWasSetupBlocking = true;
			if (m_bMenuOpen)
				CloseMenu("setup blocking");
			Debug.ClearKey(KeyCode.KC_I);
			m_bCommandMenuKeyDownLastFrame = false;
			m_bRawIKeyDownLastFrame = false;
			return;
		}

		if (m_bWasSetupBlocking)
		{
			m_bWasSetupBlocking = false;
			ResetInputLatchAfterSetupMap();
		}

		TickPostSetupInputRecovery(timeSlice);

		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.ActivateContext(MENU_INPUT_CONTEXT);
			inputManager.ActivateAction(COMMAND_MENU_CUSTOM_ACTION);
			PollRawCommandMenuKey();
			PollCommandMenuInput(inputManager);

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
		OpenMenuToTab("petros", "contextual action");
	}

	void OpenArsenalMenu()
	{
		OpenMenuToTab("arsenal", "contextual action");
	}

	void OpenMenuToTab(string tabId, string source = "external")
	{
		if (HST_SetupMapComponent.IsSetupBlocking())
			return;

		tabId = NormalizeTabId(tabId);
		if (!tabId.IsEmpty())
			m_sSelectedTab = tabId;

		if (!m_bMenuOpen)
		{
			OpenMenu(source);
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
		CloseMenu("external");
	}

	void ResetInputLatchAfterSetupMap()
	{
		StartPostSetupInputRecovery();
		ResetCommandMenuInputLatchNow();
		RebindCommandMenuInputAfterSetup();
		GetGame().GetCallqueue().CallLater(RebindCommandMenuInputAfterSetup, 0, false);
		GetGame().GetCallqueue().CallLater(RebindCommandMenuInputAfterSetup, 250, false);
		GetGame().GetCallqueue().CallLater(RebindCommandMenuInputAfterSetup, 750, false);
	}

	protected void ResetCommandMenuInputLatchNow()
	{
		SCR_RespawnSystemComponent.CloseRespawnMenu();
		m_fCommandMenuDebounceRemaining = 0;
		m_bCommandMenuKeyDownLastFrame = false;
		m_bRawIKeyDownLastFrame = false;
		Debug.ClearKey(KeyCode.KC_I);

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.ActivateContext(MENU_INPUT_CONTEXT);
		inputManager.ActivateAction(COMMAND_MENU_CUSTOM_ACTION);
	}

	protected void StartPostSetupInputRecovery()
	{
		m_fPostSetupInputRecoveryRemaining = POST_SETUP_INPUT_RECOVERY_SECONDS;
		m_fPostSetupInputRecoveryAccumulator = POST_SETUP_INPUT_REBIND_INTERVAL_SECONDS;
	}

	protected void TickPostSetupInputRecovery(float timeSlice)
	{
		if (m_fPostSetupInputRecoveryRemaining <= 0.0)
			return;

		m_fPostSetupInputRecoveryRemaining = Math.Max(0.0, m_fPostSetupInputRecoveryRemaining - timeSlice);
		m_fPostSetupInputRecoveryAccumulator += timeSlice;
		if (m_fPostSetupInputRecoveryAccumulator < POST_SETUP_INPUT_REBIND_INTERVAL_SECONDS)
			return;

		m_fPostSetupInputRecoveryAccumulator = 0;
		RebindCommandMenuInputAfterSetup();
	}

	protected void RebindCommandMenuInputAfterSetup()
	{
		if (!m_bIsLocalOwner)
			return;

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		if (!m_bInputRegistered)
			RegisterInputListeners();
		else
		{
			EnsureInputConfig(inputManager);
			EnsureIKeyBinding(inputManager);
		}

		inputManager.ActivateContext(MENU_INPUT_CONTEXT);
		inputManager.ActivateAction(COMMAND_MENU_CUSTOM_ACTION);
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
			CloseMenu("close button");
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
			TryToggleCommandMenu("raw KC_I");
			Debug.ClearKey(KeyCode.KC_I);
		}

		m_bRawIKeyDownLastFrame = keyDown;
	}

	protected bool TryToggleCommandMenu(string source)
	{
		if (!m_bIsLocalOwner)
		{
			DebugCommandMenuToggleRefused(source, "not local owner");
			return false;
		}

		if (HST_SetupMapComponent.IsSetupBlocking())
		{
			DebugCommandMenuToggleRefused(source, "setup blocking");
			if (m_bMenuOpen)
				CloseMenu("setup blocking");
			Debug.ClearKey(KeyCode.KC_I);
			return false;
		}

		if (m_fCommandMenuDebounceRemaining > 0)
		{
			DebugCommandMenuToggleRefused(source, "debounce");
			return false;
		}

		if (!HST_CommandMenuRequestComponent.GetLocalOwner() && !Replication.IsServer())
			DebugCommandMenuToggleRefused(source, "request bridge missing");

		SCR_RespawnSystemComponent.CloseRespawnMenu();
		m_fCommandMenuDebounceRemaining = 0.15;
		DebugLog(string.Format("input toggle source=%1 setupBlocking=%2 menuOpen=%3 localOwner=%4", source, HST_SetupMapComponent.IsSetupBlocking(), m_bMenuOpen, m_bIsLocalOwner));
		ToggleMenu(source);
		return true;
	}

	protected void DebugCommandMenuToggleRefused(string source, string reason)
	{
		DebugLog(string.Format("input refused source=%1 reason=%2 setupBlocking=%3 menuOpen=%4 localOwner=%5 inputRegistered=%6 customBinding=%7 debounce=%8", source, reason, HST_SetupMapComponent.IsSetupBlocking(), m_bMenuOpen, m_bIsLocalOwner, m_bInputRegistered, m_bCustomBindingReady, m_fCommandMenuDebounceRemaining));
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
			CloseMenu("menu back action");
	}

	protected void BecomeLocalOwner()
	{
		if (s_LocalInstance == this)
			return;

		s_LocalInstance = this;
		m_fInputRetryAccumulator = 0;
		DebugLog("local player menu component ready");
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
				DebugLog("waiting for I-key custom input action");

			m_bCustomBindingAttempted = true;
			return false;
		}

		m_bCustomBindingAttempted = true;
		DebugLog("HST_CommandMenu input action ready");
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
		DebugLog("bound I key to command menu");
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
			DebugLog("registered command menu input config");
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
		m_fContentScrollY = 0.0;
		m_fActionScrollY = 0.0;
		m_fFeedScrollY = 0.0;
	}

	protected void ToggleMenu(string source = "unknown")
	{
		if (m_bMenuOpen)
		{
			CloseMenu(source);
			return;
		}

		OpenMenu(source);
	}

	protected void OpenMenu(string source = "unknown")
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
		DebugLog("opened via " + source);
		ShowMenuHint("Command menu opened", "h-istasi", 2.0);
	}

	protected void CloseMenu(string source = "unknown")
	{
		if (!m_bMenuOpen)
			return;

		m_bMenuOpen = false;
		ClearWidgets();
		DebugLog("closed via " + source);
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

		SaveCommandMenuScrollOffsets();
		ClearWidgets();
		m_ContentScroll = null;
		m_ActionScroll = null;
		m_FeedScroll = null;
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

		Widget root = workspace.CreateWidgets(COMMAND_MENU_LAYOUT);
		if (!root)
			return null;

		FrameSlot.SetPos(root, m_Layout.m_iRootLeft, m_Layout.m_iRootTop);
		FrameSlot.SetSize(root, m_Layout.m_iRootWidth, m_Layout.m_iRootHeight);
		root.SetVisible(true);
		root.SetOpacity(1.0);
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

		int screenW;
		int screenH;
		HST_UIWorkspaceMetrics.GetLayoutSize(workspace, screenW, screenH);

		m_Layout.m_iScreenW = screenW;
		m_Layout.m_iScreenH = screenH;

		float scale = HST_UIWorkspaceMetrics.GetScale(screenW, screenH, 0.70, 1.12);

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

		int availableW = Math.Max(1, screenW - m_Layout.m_iMargin * 2);
		int availableH = Math.Max(1, screenH - m_Layout.m_iMargin * 2);
		int maxRootW = ScalePx(1680);
		int maxRootH = ScalePx(900);
		m_Layout.m_iRootWidth = Math.Min(availableW, maxRootW);
		m_Layout.m_iRootHeight = Math.Min(availableH, maxRootH);
		m_Layout.m_iRootLeft = Math.Max(0, (screenW - m_Layout.m_iRootWidth) / 2);
		m_Layout.m_iRootTop = Math.Max(0, (screenH - m_Layout.m_iRootHeight) / 2);

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

		int rightCandidateWidth = ClampLayoutInt(Math.Round(m_Layout.m_iRootWidth * 0.30), ScalePx(400), ScalePx(520));
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

		m_Layout.m_iFontTiny = ScaleFont(10);
		m_Layout.m_iFontSmall = ScaleFont(12);
		m_Layout.m_iFontNormal = ScaleFont(14);
		m_Layout.m_iFontTitle = ScaleFont(18);
		m_Layout.m_iFontHeader = ScaleFont(30);

		if (m_iLoggedLayoutW != screenW || m_iLoggedLayoutH != screenH)
		{
			m_iLoggedLayoutW = screenW;
			m_iLoggedLayoutH = screenH;
			DebugLog(string.Format("layout workspace=%1x%2 scale=%3 root=%4,%5 %6x%7", screenW, screenH, scale, m_Layout.m_iRootLeft, m_Layout.m_iRootTop, m_Layout.m_iRootWidth, m_Layout.m_iRootHeight));
		}
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

		int screenW;
		int screenH;
		HST_UIWorkspaceMetrics.GetLayoutSize(workspace, screenW, screenH);
		float scale = HST_UIWorkspaceMetrics.GetScale(screenW, screenH, 0.70, 1.12);
		int margin = HST_UIWorkspaceMetrics.ScalePx(24, scale);
		int width = Math.Min(HST_UIWorkspaceMetrics.ScalePx(900, scale), Math.Max(1, screenW - margin * 2));
		if (width < HST_UIWorkspaceMetrics.ScalePx(320, scale))
			width = Math.Max(1, screenW - margin * 2);

		int height = HST_UIWorkspaceMetrics.ScalePx(92, scale);
		int horizontalMargin = Math.Max(8, margin / 2);
		int verticalMargin = Math.Max(4, margin / 2);
		int left = HST_UIWorkspaceMetrics.ClampInt(Math.Max(0, (screenW - width) / 2), horizontalMargin, Math.Max(horizontalMargin, screenW - width - horizontalMargin));
		int top = HST_UIWorkspaceMetrics.ClampInt(margin, verticalMargin, Math.Max(verticalMargin, screenH - height - verticalMargin));

		Widget root = workspace.CreateWidgets(SCRIPTED_PANEL_ROOT_LAYOUT);
		if (!root)
			return;

		FrameSlot.SetPos(root, left, top);
		FrameSlot.SetSize(root, width, height);
		root.SetVisible(true);
		root.SetOpacity(1.0);
		root.SetZOrder(2850);
		root.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		m_aExternalNotificationWidgets.Insert(root);
		CreateExternalRectWidget(workspace, root, 0, 0, width, height, 0xF21A232B, 1.0);
		CreateExternalRectWidget(workspace, root, 0, height - Math.Max(2, HST_UIWorkspaceMetrics.ScalePx(4, scale)), width, Math.Max(2, HST_UIWorkspaceMetrics.ScalePx(4, scale)), 0xFFC4953B, 1.0);
		CreateExternalTextWidget(workspace, root, ShortenText(title, 44), HST_UIWorkspaceMetrics.ScalePx(24, scale), HST_UIWorkspaceMetrics.ScalePx(10, scale), width - HST_UIWorkspaceMetrics.ScalePx(48, scale), HST_UIWorkspaceMetrics.ScalePx(24, scale), HST_UIWorkspaceMetrics.ScaleFont(18, scale), 0xFFF2D18B, true);
		CreateExternalTextWidget(workspace, root, ShortenText(message, 140), HST_UIWorkspaceMetrics.ScalePx(24, scale), HST_UIWorkspaceMetrics.ScalePx(38, scale), width - HST_UIWorkspaceMetrics.ScalePx(48, scale), HST_UIWorkspaceMetrics.ScalePx(42, scale), HST_UIWorkspaceMetrics.ScaleFont(16, scale), 0xFFF2F4F0, false);
		m_fExternalNotificationRemaining = Math.Max(1.0, durationSeconds);
	}

	protected Widget CreateExternalRectWidget(WorkspaceWidget workspace, Widget parent, int left, int top, int width, int height, int color, float opacity)
	{
		Widget widget = workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS, null, 2851, parent);
		if (!widget)
			return null;

		widget.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
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
		Widget widget = workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NO_LOCALIZATION | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS, null, 2852, parent);
		if (!widget)
			return null;

		widget.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
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

		CreateRectWidget(workspace, root, m_Layout.m_iMainLeft, m_Layout.m_iMainTop, m_Layout.m_iMainWidth, m_Layout.m_iMainHeight, 0xF31A232B, 0.98, 0);
		CreateRectWidget(workspace, root, m_Layout.m_iMainLeft, m_Layout.m_iMainTop, m_Layout.m_iMainWidth, ScalePx(4), 0xFFC4953B, 1.0, 0);

		ScrollLayoutWidget scroll;
		Widget items;
		CreateScrollContainer(workspace, root, VERTICAL_SCROLL_LIST_LAYOUT, m_Layout.m_iMainContentLeft, m_Layout.m_iMainContentTop, m_Layout.m_iMainContentWidth, m_Layout.m_iMainContentHeight, scroll, items, false);
		m_ContentScroll = scroll;

		if (!items)
		{
			CreateWrappedTextWidget(workspace, root, "Main content unavailable: scroll layout missing Items.", m_Layout.m_iMainContentLeft, m_Layout.m_iMainContentTop, m_Layout.m_iMainContentWidth, ScalePx(40), m_Layout.m_iFontSmall, 0xFFFFD166, 0, false);
			return;
		}

		if (m_aSectionIds.Count() == 0)
		{
			Widget row = workspace.CreateWidgets(COMMAND_DATA_ROW_COMPACT_LAYOUT, items);
			DebugRowCreated("COMMAND_DATA_ROW_COMPACT_LAYOUT", row);
			if (row)
			{
				PrepareRowRoot(row);
				SetRowText(row, "Label", "", 0xFFC4CDD3, m_Layout.m_iFontSmall, true, true);
				SetRowText(row, "Value", BuildLabeledDisplayText("Status", m_sStatusText), 0xFFE0E0E0, m_Layout.m_iFontNormal, false, true);
				SetRowImageColor(row, "Background", 0x00111111, 0.01);
			}

			RestoreScrollPixels(m_ContentScroll, m_fContentScrollY);
			return;
		}

		for (int i = 0; i < m_aContentItemKinds.Count(); i++)
			AddCommandContentRow(workspace, items, i);

		RestoreScrollPixels(m_ContentScroll, m_fContentScrollY);
	}

	protected void AddCommandContentRow(WorkspaceWidget workspace, Widget items, int contentIndex)
	{
		if (!workspace || !items)
			return;

		if (contentIndex < 0 || contentIndex >= m_aContentItemKinds.Count())
			return;

		string kind = m_aContentItemKinds[contentIndex];
		if (kind == "section")
		{
			int sectionIndex = m_aContentItemSectionIndexes[contentIndex];
			Widget row = workspace.CreateWidgets(COMMAND_SECTION_ROW_LAYOUT, items);
			DebugRowCreated("COMMAND_SECTION_ROW_LAYOUT", row);
			if (!row)
				return;

			PrepareRowRoot(row);

			string title = "";
			if (sectionIndex >= 0 && sectionIndex < m_aSectionTitles.Count())
				title = m_aSectionTitles[sectionIndex];

			SetRowText(row, "Title", title, 0xFFEFE2C4, m_Layout.m_iFontTitle, true, true);
			SetRowImageColor(row, "Background", 0xFF263341, 0.72);
			return;
		}

		if (kind == "empty")
		{
			Widget row = workspace.CreateWidgets(COMMAND_DATA_ROW_LAYOUT, items);
			DebugRowCreated("COMMAND_DATA_ROW_LAYOUT", row);
			if (!row)
				return;

			PrepareRowRoot(row);

			SetRowText(row, "Label", "", 0xFF8D98A0, m_Layout.m_iFontSmall, false, true);
			SetRowText(row, "Value", "No entries", 0xFF8D98A0, m_Layout.m_iFontNormal, false, true);
			SetRowImageColor(row, "Background", 0x00111111, 0.01);
			return;
		}

		int rowIndex = m_aContentItemRowIndexes[contentIndex];
		if (rowIndex < 0 || rowIndex >= m_aRowLabels.Count())
			return;

		ResourceName layout = COMMAND_DATA_ROW_LAYOUT;
		string layoutLabel = "COMMAND_DATA_ROW_LAYOUT";
		if (m_Layout.m_bVeryCompact)
		{
			layout = COMMAND_DATA_ROW_COMPACT_LAYOUT;
			layoutLabel = "COMMAND_DATA_ROW_COMPACT_LAYOUT";
		}

		Widget row = workspace.CreateWidgets(layout, items);
		DebugRowCreated(layoutLabel, row);
		if (!row)
			return;

		PrepareRowRoot(row);

		SetRowText(row, "Label", "", 0xFFC4CDD3, m_Layout.m_iFontSmall, false, true);
		SetRowText(row, "Value", BuildLabeledDisplayText(m_aRowLabels[rowIndex], m_aRowValues[rowIndex]), ToneTextColor(m_aRowTones[rowIndex]), m_Layout.m_iFontNormal, false, true);
		SetRowImageColor(row, "Background", 0x00111111, 0.01);
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

		CreateWrappedTextWidget(workspace, root, "Campaign Notes", m_Layout.m_iActivityTextLeft, feedTop, m_Layout.m_iActivityTextWidth, ScalePx(28), m_Layout.m_iFontTitle, 0xFFEFE2C4, 0, true);

		int listTop = feedTop + ScalePx(36);
		int listBottom = m_Layout.m_iActivityTop + m_Layout.m_iActivityHeight - ScalePx(10);
		int listHeight = Math.Max(1, listBottom - listTop);

		ScrollLayoutWidget scroll;
		Widget items;
		CreateScrollContainer(workspace, root, VERTICAL_SCROLL_LIST_LAYOUT, m_Layout.m_iActivityTextLeft, listTop, m_Layout.m_iActivityTextWidth, listHeight, scroll, items, false);
		m_FeedScroll = scroll;

		if (!items)
		{
			CreateTextWidget(workspace, root, "Activity feed unavailable: scroll layout missing Items.", m_Layout.m_iActivityTextLeft, listTop, m_Layout.m_iActivityTextWidth, ScalePx(28), m_Layout.m_iFontSmall, 0xFFFFD166, 0, false);
			return;
		}

		if (m_aFeedLines.Count() == 0)
		{
			Widget row = workspace.CreateWidgets(COMMAND_FEED_ROW_LAYOUT, items);
			DebugRowCreated("COMMAND_FEED_ROW_LAYOUT", row);
			if (row)
			{
				PrepareRowRoot(row);
				SetRowText(row, "Text", "Waiting for HQ traffic.", 0xFFAFBAC1, m_Layout.m_iFontNormal, false, true);
			}
			RestoreScrollPixels(m_FeedScroll, m_fFeedScrollY);
			return;
		}

		for (int i = 0; i < m_aFeedLines.Count(); i++)
		{
			Widget row = workspace.CreateWidgets(COMMAND_FEED_ROW_LAYOUT, items);
			DebugRowCreated("COMMAND_FEED_ROW_LAYOUT", row);
			if (row)
			{
				PrepareRowRoot(row);
				SetRowText(row, "Text", m_aFeedLines[i], ToneTextColor(m_aFeedTones[i]), m_Layout.m_iFontSmall, false, true);
			}
		}

		RestoreScrollPixels(m_FeedScroll, m_fFeedScrollY);
	}

	protected void RenderActions(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		CreateRectWidget(workspace, root, m_Layout.m_iRightLeft, m_Layout.m_iActionsTop, m_Layout.m_iRightWidth, m_Layout.m_iActionsHeight, 0xF31A232B, 0.98, 0);
		CreateRectWidget(workspace, root, m_Layout.m_iRightLeft, m_Layout.m_iActionsTop, m_Layout.m_iRightWidth, ScalePx(4), 0xFF8C4E43, 1.0, 0);
		CreateTextWidget(workspace, root, "Actions", m_Layout.m_iActionsTextLeft, m_Layout.m_iActionsTop + ScalePx(18), ScalePx(170), ScalePx(30), m_Layout.m_iFontTitle, 0xFFEFE2C4, 0, true);
		int listTop = m_Layout.m_iActionsTop + ScalePx(58);
		int listHeight = Math.Max(1, m_Layout.m_iActionsTop + m_Layout.m_iActionsHeight - listTop - ScalePx(16));

		ScrollLayoutWidget scroll;
		Widget items;
		CreateScrollContainer(workspace, root, VERTICAL_SCROLL_LIST_LAYOUT, m_Layout.m_iActionsTextLeft, listTop, m_Layout.m_iActionsTextWidth, listHeight, scroll, items, false);
		m_ActionScroll = scroll;

		if (!items)
		{
			CreateTextWidget(workspace, root, "Actions unavailable: scroll layout missing Items.", m_Layout.m_iActionsTextLeft, listTop, m_Layout.m_iActionsTextWidth, ScalePx(30), m_Layout.m_iFontSmall, 0xFFFFD166, 0, false);
			return;
		}

		if (m_aActionLabels.Count() == 0)
		{
			Widget row = workspace.CreateWidgets(COMMAND_ACTION_ROW_LAYOUT, items);
			DebugRowCreated("COMMAND_ACTION_ROW_LAYOUT", row);
			if (row)
			{
				PrepareRowRoot(row);
				SetRowText(row, "Label", "No commands available.", 0xFF9AA5AD, m_Layout.m_iFontNormal, false, true);
				SetRowImageColor(row, "Background", 0x00111111, 0.01);
			}

			RestoreScrollPixels(m_ActionScroll, m_fActionScrollY);
			return;
		}

		for (int i = 0; i < m_aActionLabels.Count(); i++)
			AddCommandActionRow(workspace, items, i);

		RestoreScrollPixels(m_ActionScroll, m_fActionScrollY);
		EnsureSelectedActionVisible();
	}

	protected void AddCommandActionRow(WorkspaceWidget workspace, Widget items, int actionIndex)
	{
		if (!workspace || !items)
			return;

		if (actionIndex < 0 || actionIndex >= m_aActionLabels.Count())
			return;

		Widget row = workspace.CreateWidgets(COMMAND_ACTION_ROW_LAYOUT, items);
		DebugRowCreated("COMMAND_ACTION_ROW_LAYOUT", row);
		if (!row)
			return;

		PrepareRowRoot(row);
		BindRowClick(row, ACTION_WIDGET_ID_BASE + actionIndex);

		bool focused = m_iSelectedControl == m_aTabIds.Count() + actionIndex;
		bool enabled = true;
		if (actionIndex < m_aActionEnabled.Count())
			enabled = m_aActionEnabled[actionIndex];

		int bgColor = 0x00222222;
		float bgOpacity = 0.01;
		if (focused)
		{
			bgColor = 0xFF604A24;
			bgOpacity = 0.88;
		}

		int textColor = 0xFFE5E5E5;
		string text = m_aActionLabels[actionIndex];
		if (focused)
			text = "> " + text;

		if (!enabled)
		{
			textColor = 0xFF8B9298;
			string disabledReason = "";
			if (actionIndex < m_aActionDisabledReasons.Count())
				disabledReason = m_aActionDisabledReasons[actionIndex];

			if (!disabledReason.IsEmpty())
				text = text + "\n" + disabledReason;
		}

		SetRowImageColor(row, "Background", bgColor, bgOpacity);
		SetRowText(row, "Label", text, textColor, m_Layout.m_iFontNormal, focused, true);
	}

	protected Widget CreateScrollContainer(WorkspaceWidget workspace, Widget parent, ResourceName layout, int left, int top, int width, int height, out ScrollLayoutWidget scroll, out Widget items, bool trackForCleanup)
	{
		scroll = null;
		items = null;

		if (!workspace || !parent || layout.IsEmpty() || width <= 0 || height <= 0)
		{
			Print("h-istasi ui scroll | failed: invalid workspace/parent/layout/size", LogLevel.WARNING);
			return null;
		}

		Widget root = workspace.CreateWidgets(layout, parent);
		if (!root)
		{
			Print("h-istasi ui scroll | failed: could not create scroll layout " + layout, LogLevel.WARNING);
			return null;
		}

		FrameSlot.SetPos(root, left, top);
		FrameSlot.SetSize(root, width, height);
		root.SetZOrder(2580);

		scroll = ScrollLayoutWidget.Cast(root.FindAnyWidget("Scroll"));
		items = root.FindAnyWidget("Items");

		if (!scroll || !items)
		{
			Print("h-istasi ui scroll | failed: layout must contain widgets named Scroll and Items", LogLevel.WARNING);
			root.RemoveFromHierarchy();
			scroll = null;
			items = null;
			return null;
		}

		root.SetOpacity(1.0);
		root.SetVisible(true);
		scroll.SetVisible(true);
		items.SetVisible(true);

		if (trackForCleanup)
			m_aWidgets.Insert(root);

		return root;
	}

	protected TextWidget FindRowText(Widget row, string name)
	{
		if (!row)
			return null;

		return TextWidget.Cast(row.FindAnyWidget(name));
	}

	protected ImageWidget FindRowImage(Widget row, string name)
	{
		if (!row)
			return null;

		return ImageWidget.Cast(row.FindAnyWidget(name));
	}

	protected Widget FindRowWidget(Widget row, string name)
	{
		if (!row)
			return null;

		return row.FindAnyWidget(name);
	}

	protected void PrepareRowRoot(Widget row)
	{
		if (!row)
			return;

		row.SetVisible(true);
		row.SetOpacity(1.0);
	}

	protected void DebugRowCreated(string label, Widget row)
	{
		if (!IsDebugLoggingEnabled())
			return;

		if (!row)
		{
			Print("h-istasi menu debug | ui row failed to create " + label, LogLevel.WARNING);
			return;
		}

		if (m_iRowCreateLogCount < 12)
		{
			DebugLog("ui row created " + label);
			m_iRowCreateLogCount++;
			return;
		}

		if (!m_bRowCreateLogSuppressed)
		{
			m_bRowCreateLogSuppressed = true;
			DebugLog("ui row additional row creation logs suppressed");
		}
	}

	protected void SetRowText(Widget row, string widgetName, string text, int color, int fontSize, bool bold, bool wrap = true)
	{
		TextWidget textWidget = FindRowText(row, widgetName);
		if (!textWidget)
			return;

		textWidget.SetVisible(true);
		textWidget.SetOpacity(1.0);
		textWidget.SetText(text);
		textWidget.SetTextWrapping(wrap);
		textWidget.SetExactFontSize(fontSize);
		textWidget.SetLineSpacing(0.95);
		textWidget.SetBold(bold);
		textWidget.SetOutline(1, 0xDD000000);
		textWidget.SetShadow(2, 0xEE000000, 1, 1, 1);
		textWidget.SetColorInt(color);
		textWidget.SetZOrder(30);
	}

	protected void SetRowImageColor(Widget row, string widgetName, int color, float opacity = 1.0)
	{
		Widget widget = FindRowWidget(row, widgetName);
		if (!widget)
			return;

		ImageWidget image = ImageWidget.Cast(widget);
		if (image)
		{
			image.LoadImageTexture(0, UI_SOLID_WHITE);
			image.SetImage(0);
		}

		widget.SetColorInt(color);
		widget.SetOpacity(opacity);
		widget.SetVisible(true);
		widget.SetZOrder(0);
	}

	protected void BindRowClick(Widget row, int userId)
	{
		if (!row || userId <= 0)
			return;

		row.SetUserID(userId);
		row.AddHandler(m_WidgetHandler);

		BindRowChildClick(row, "ClickSurface", userId);
		BindRowChildClick(row, "Background", userId);
		BindRowChildClick(row, "Label", userId);
		BindRowChildClick(row, "Value", userId);
		BindRowChildClick(row, "Text", userId);
		BindRowChildClick(row, "Title", userId);
		BindRowChildClick(row, "Primary", userId);
		BindRowChildClick(row, "Secondary", userId);
		BindRowChildClick(row, "Name", userId);
		BindRowChildClick(row, "Count", userId);
		BindRowChildClick(row, "Meta", userId);
		BindRowChildClick(row, "OpenMarker", userId);
		BindRowChildClick(row, "PreviewAnchor", userId);
	}

	protected void BindRowChildClick(Widget row, string childName, int userId)
	{
		Widget child = row.FindAnyWidget(childName);
		if (child)
		{
			child.SetUserID(userId);
			child.AddHandler(m_WidgetHandler);
		}
	}

	protected void SaveCommandMenuScrollOffsets()
	{
		float x;
		float y;

		if (m_ContentScroll)
		{
			m_ContentScroll.GetSliderPosPixels(x, y);
			m_fContentScrollY = y;
		}

		if (m_ActionScroll)
		{
			m_ActionScroll.GetSliderPosPixels(x, y);
			m_fActionScrollY = y;
		}

		if (m_FeedScroll)
		{
			m_FeedScroll.GetSliderPosPixels(x, y);
			m_fFeedScrollY = y;
		}
	}

	protected void RestoreScrollPixels(ScrollLayoutWidget scroll, float y)
	{
		if (!scroll)
			return;

		scroll.SetSliderPosPixels(0, Math.Max(0.0, y));
	}

	protected void EnsureSelectedActionVisible()
	{
		if (!m_ActionScroll || !m_Layout)
			return;

		int actionIndex = m_iSelectedControl - m_aTabIds.Count();
		if (actionIndex < 0 || actionIndex >= m_aActionLabels.Count())
			return;

		int rowHeight = ScalePx(COMMAND_ACTION_ROW_STRIDE);
		int y = actionIndex * rowHeight;
		m_ActionScroll.ScrollToView(0, y, m_Layout.m_iActionsTextWidth, rowHeight);
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

	protected string BuildLabeledDisplayText(string label, string value)
	{
		label = label.Trim();
		value = value.Trim();

		if (label.IsEmpty())
			return value;
		if (value.IsEmpty())
			return label;

		return label + ": " + value;
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

		string debugLine = ExtractPayloadLine(payload, "DEBUG|");
		if (!debugLine.IsEmpty())
			m_bDebugLoggingEnabled = ParseBool(ExtractPipeField(debugLine, 1));
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

	protected string ExtractPayloadLine(string payload, string prefix)
	{
		if (payload.IsEmpty() || prefix.IsEmpty())
			return "";

		int start = payload.IndexOf(prefix);
		if (start < 0)
			return "";

		int end = payload.IndexOfFrom(start, "\n");
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
		return value == "true" || value == "1";
	}

	bool IsDebugLoggingEnabled()
	{
		return m_bDebugLoggingEnabled;
	}

	protected void DebugLog(string message)
	{
		if (!IsDebugLoggingEnabled())
			return;

		Print("h-istasi menu debug | " + message);
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

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId <= 0)
			return false;

		PlayerController controller = PlayerController.Cast(owner);
		return controller && controller.GetPlayerId() == localPlayerId;
	}
}
