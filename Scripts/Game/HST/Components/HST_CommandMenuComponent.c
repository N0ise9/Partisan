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
	protected ref array<Widget> m_aWidgets = {};
	protected ref array<ref HST_CommandMenuDrawCommandSet> m_aCanvasCommandSets = {};
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
		m_sLastPayload = payload;
		m_sLastResult = lastResult;
		ApplyHeaderFromPayload(payload);
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
			CloseMenu();
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
		Widget root = workspace.CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID, 28, 32, 1180, 820, WidgetFlags.VISIBLE, null, 2500);
		if (!root)
			return null;

		root.SetZOrder(2500);
		m_aWidgets.Insert(root);
		CreateRectWidget(workspace, root, 0, 0, 1180, 820, 0xF2080D12, 1.0, 0);
		return root;
	}

	protected void ShowMenuHint(string text, string title, float durationSeconds)
	{
		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (hintManager)
			hintManager.ShowCustomHint(text, title, durationSeconds);
	}

	protected void RenderHeader(WorkspaceWidget workspace, Widget root)
	{
		CreateRectWidget(workspace, root, 0, 0, 1180, 84, 0xFF111920, 0.98, 0);
		CreateTextWidget(workspace, root, "h-istasi HQ", 24, 14, 280, 48, 34, 0xFFF2D18B, 0, true);
		CreateTextWidget(workspace, root, "FIA Resistance Command", 300, 28, 300, 28, 16, 0xFFB7C7D7, 0, false);
		CreateTextWidget(workspace, root, BuildSelectedTabTitle(), 610, 18, 290, 42, 26, 0xFFECE6D2, 0, true);
		CreateRectWidget(workspace, root, 1040, 16, 96, 46, 0xFF5F6C76, 0.96, CLOSE_WIDGET_ID);
		CreateTextWidget(workspace, root, "Close", 1054, 28, 68, 24, 17, 0xFFF4EBD6, CLOSE_WIDGET_ID, true);
	}

	protected void RenderStats(WorkspaceWidget workspace, Widget root)
	{
		int left = 220;
		int top = 96;
		int width = 134;
		for (int i = 0; i < m_aStatLabels.Count(); i++)
		{
			if (i >= 4)
				break;

			int x = left + i * 144;
			CreateRectWidget(workspace, root, x, top, width, 58, ToneBackgroundColor(m_aStatTones[i]), 0.96, 0);
			CreateTextWidget(workspace, root, ShortenText(m_aStatLabels[i], 14), x + 10, top + 7, width - 20, 20, 14, 0xFFCAD2D7, 0, false);
			CreateTextWidget(workspace, root, ShortenText(m_aStatValues[i], 12), x + 10, top + 31, width - 20, 24, 19, ToneTextColor(m_aStatTones[i]), 0, true);
		}
	}

	protected void RenderNavigation(WorkspaceWidget workspace, Widget root)
	{
		CreateRectWidget(workspace, root, 20, 96, 178, 704, 0xFF0F151B, 0.98, 0);
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
		if (sectionCount == 0)
		{
			CreateRectWidget(workspace, root, 220, 172, 610, 626, 0xF31A232B, 0.98, 0);
			CreateTextWidget(workspace, root, ShortenText(m_sStatusText, 170), 244, 202, 562, 540, 17, 0xFFE0E0E0, 0, false);
			return;
		}

		int maxSections = Math.Min(sectionCount, 4);
		for (int i = 0; i < maxSections; i++)
		{
			int top = 172 + i * 154;
			RenderSectionCard(workspace, root, i, 220, top, 610, 144);
		}
	}

	protected void RenderSectionCard(WorkspaceWidget workspace, Widget root, int sectionIndex, int left, int top, int width, int height)
	{
		if (sectionIndex < 0 || sectionIndex >= m_aSectionIds.Count())
			return;

		string sectionId = m_aSectionIds[sectionIndex];
		CreateRectWidget(workspace, root, left, top, width, height, 0xF31A232B, 0.98, 0);
		CreateRectWidget(workspace, root, left, top, width, 4, 0xFFC4953B, 1.0, 0);
		CreateTextWidget(workspace, root, ShortenText(m_aSectionTitles[sectionIndex], 34), left + 18, top + 12, width - 36, 28, 21, 0xFFEFE2C4, 0, true);

		int rendered;
		for (int i = 0; i < m_aRowLabels.Count(); i++)
		{
			if (m_aRowSectionIds[i] != sectionId)
				continue;

			if (rendered >= 4)
				break;

			int rowTop = top + 50 + rendered * 24;
			CreateTextWidget(workspace, root, ShortenText(m_aRowLabels[i], 18), left + 18, rowTop, 154, 23, 15, 0xFFC4CDD3, 0, false);
			CreateTextWidget(workspace, root, ShortenText(m_aRowValues[i], 42), left + 184, rowTop, width - 204, 23, 15, ToneTextColor(m_aRowTones[i]), 0, false);
			rendered++;
		}

		if (rendered == 0)
			CreateTextWidget(workspace, root, "No entries", left + 18, top + 52, width - 36, 24, 16, 0xFF8D98A0, 0, false);
	}

	protected void RenderActivityPanel(WorkspaceWidget workspace, Widget root)
	{
		CreateRectWidget(workspace, root, 852, 96, 310, 302, 0xF31A232B, 0.98, 0);
		CreateRectWidget(workspace, root, 852, 96, 310, 4, 0xFF50704A, 1.0, 0);
		CreateTextWidget(workspace, root, "Activity", 872, 114, 180, 30, 20, 0xFFEFE2C4, 0, true);
		CreateTextWidget(workspace, root, ShortenText(BuildResultText(), 56), 872, 154, 270, 66, 16, 0xFFD2E7B8, 0, false);
		CreateTextWidget(workspace, root, "Campaign Notes", 872, 232, 214, 28, 20, 0xFFEFE2C4, 0, true);

		if (m_aFeedLines.Count() == 0)
		{
			CreateTextWidget(workspace, root, "Waiting for HQ traffic.", 872, 270, 270, 28, 16, 0xFFAFBAC1, 0, false);
			return;
		}

		for (int i = 0; i < m_aFeedLines.Count(); i++)
		{
			if (i >= 4)
				break;

			CreateTextWidget(workspace, root, ShortenText(m_aFeedLines[i], 34), 872, 270 + i * 30, 270, 28, 15, ToneTextColor(m_aFeedTones[i]), 0, false);
		}
	}

	protected void RenderActions(WorkspaceWidget workspace, Widget root)
	{
		CreateRectWidget(workspace, root, 852, 420, 310, 378, 0xF31A232B, 0.98, 0);
		CreateRectWidget(workspace, root, 852, 420, 310, 4, 0xFF8C4E43, 1.0, 0);
		CreateTextWidget(workspace, root, "Actions", 872, 438, 170, 30, 20, 0xFFEFE2C4, 0, true);
		for (int i = 0; i < m_aActionLabels.Count(); i++)
		{
			if (i >= 10)
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
			CreateRectWidget(workspace, root, 864, 478 + i * 33, 278, 29, rowColor, rowOpacity, ACTION_WIDGET_ID_BASE + i);

			string suffix = "";
			int color = 0xFFE5E5E5;
			if (!m_aActionEnabled[i])
			{
				suffix = " / " + m_aActionDisabledReasons[i];
				color = 0xFF8B9298;
			}

			CreateTextWidget(workspace, root, ShortenText(prefix + m_aActionLabels[i] + suffix, 34), 872, 482 + i * 33, 270, 28, 15, color, ACTION_WIDGET_ID_BASE + i, m_iSelectedControl == m_aTabIds.Count() + i);
		}

		if (m_aActionLabels.Count() == 0)
			CreateTextWidget(workspace, root, "No commands available.", 872, 482, 270, 30, 16, 0xFF9AA5AD, 0, false);
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
