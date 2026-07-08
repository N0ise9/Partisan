class HST_CommandMenuWidgetHandler : ScriptedWidgetEventHandler
{
	protected HST_CommandMenuComponent m_Menu;

	void Bind(HST_CommandMenuComponent menu)
	{
		m_Menu = menu;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Menu || !w)
			return false;

		return m_Menu.OnWidgetClicked(w.GetUserID(), button);
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (!m_Menu || !w)
			return false;

		return m_Menu.OnWidgetClicked(w.GetUserID(), button);
	}
}

class HST_CommandMenuLayoutMetrics
{
	int m_iScreenW;

	float m_fScale;
	bool m_bVeryCompact;

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
	static const string COMMAND_MENU_NATIVE_I_ACTION = "PlayerMenuInvite";
	static const string COMMAND_MENU_UP_ACTION = "MenuUp";
	static const string COMMAND_MENU_DOWN_ACTION = "MenuDown";
	static const string COMMAND_MENU_SELECT_ACTION = "MenuSelect";
	static const string COMMAND_MENU_BACK_ACTION = "MenuBack";
	static const string COMMAND_MENU_INPUT_CONTEXT = "HST_CommandMenuContext";
	static const string COMMAND_MENU_NATIVE_I_CONTEXT = "PlayerMenuContext";
	static const string COMMAND_MENU_BUILD = "2026-07-08-menu-input-r19-map-open-gate-proof";
	static const string MENU_INPUT_CONTEXT = "InGameMenuContext";
	static const string MENU_CURSOR_CONTEXT = "InventoryContext";
	static const string COMMAND_MENU_KEYBOARD_BINDING = "keyboard:KC_I";
	static const ResourceName INPUT_CONFIG = "Configs/HST/Input/HST_Input.conf";
	static const ResourceName COMMAND_MENU_LAYOUT = "{A7B8C9D001234550}UI/layouts/HST_CommandMenu.layout";
	static const string ACTION_DIALOG_OWNER = "HST_CommandMenuActionDialog";
	static const string MAP_TARGET_OVERLAY_OWNER = "HST_CommandMapTargetOverlay";
	static const ResourceName UI_SOLID_WHITE = "{56137CA0F2D3ACE6}Assets/Images/solid_white_square.edds";
	static const ResourceName MAP_TARGET_PROMPT_LAYOUT = "{A34F448C7E810600}UI/layouts/HST_SetupPromptBanner.layout";
	static const ResourceName COMMAND_SECTION_ROW_LAYOUT = "{A7B8C9D001234580}UI/layouts/HST/Rows/HST_CommandSectionRow.layout";
	static const ResourceName COMMAND_DATA_ROW_LAYOUT = "{A7B8C9D001234590}UI/layouts/HST/Rows/HST_CommandDataRow.layout";
	static const ResourceName COMMAND_DATA_ROW_COMPACT_LAYOUT = "{A7B8C9D0012345A0}UI/layouts/HST/Rows/HST_CommandDataRowCompact.layout";
	static const ResourceName COMMAND_ACTION_ROW_LAYOUT = "{A7B8C9D0012345B0}UI/layouts/HST/Rows/HST_CommandActionRow.layout";
	static const ResourceName COMMAND_PANEL_ACTION_ROW_LAYOUT = "{A7B8C9D0012365B0}UI/layouts/HST/Rows/HST_CommandPanelActionRow.layout";
	static const ResourceName COMMAND_FEED_ROW_LAYOUT = "{A7B8C9D0012345C0}UI/layouts/HST/Rows/HST_CommandFeedRow.layout";
	static const float POST_SETUP_INPUT_RECOVERY_SECONDS = 3.0;
	static const float POST_SETUP_INPUT_REBIND_INTERVAL_SECONDS = 0.25;
	static const float INPUT_DIAGNOSTIC_INTERVAL_SECONDS = 5.0;
	static const int INPUT_DIAGNOSTIC_MAX_LINES = 12;
	static const int COMMAND_ACTION_ROW_STRIDE = 70;
	static const int TAB_WIDGET_ID_BASE = 10000;
	static const int ACTION_WIDGET_ID_BASE = 30000;
	static const int CLOSE_WIDGET_ID = 90000;
	static const int ACTION_MODAL_CANCEL_WIDGET_ID = 90010;
	static const int ACTION_MODAL_CONFIRM_WIDGET_ID = 90011;
	static const int ACTION_CHOICE_WIDGET_ID_BASE = 90100;
	static const int ACTION_CHOICE_WIDGET_ID_LIMIT = 90228;
	static const int MAP_TARGET_PROMPT_Z = 2300;
	static const int MAP_TARGET_CURSOR_LINE_LONG = 30;
	static const int MAP_TARGET_CURSOR_LINE_SHORT = 3;
	static const int MAP_TARGET_CURSOR_CENTER_SIZE = 7;
	static const int COMMAND_DIMMER_Z = 0;
	static const int COMMAND_SURFACE_Z = 10;
	static const int COMMAND_PANEL_Z = 20;
	static const int COMMAND_HEADER_Z = 30;
	static const int COMMAND_CLOSE_Z = 40;

	protected static HST_CommandMenuComponent s_LocalInstance;

	protected bool m_bMenuOpen;
	protected string m_sSelectedTab = "overview";
	protected string m_sLastPayload;
	protected string m_sLastResult;
	protected string m_sLastActionName;
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
	protected Widget m_wMenuRoot;
	protected ref HST_CommandMenuLayoutMetrics m_Layout;
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
	protected bool m_bCommandMenuInputConsumedThisFrame;
	protected int m_iFrameSerial;
	protected int m_iLastActivatedWidgetId;
	protected int m_iLastActivatedButton;
	protected int m_iLastActivatedFrame = -1;
	protected bool m_bWasSetupBlocking;
	protected bool m_bLoggedCustomActionInput;
	protected bool m_bLoggedRawIKeyInput;
	protected bool m_bLoggedDuplicateToggle;
	protected bool m_bLoggedInputManagerMissing;
	protected bool m_bLoggedLocalReadyPrinted;
	protected bool m_bDebugLoggingEnabled;
	protected bool m_bPostLayoutRefreshQueued;
	protected int m_iLoggedLayoutW;
	protected int m_iLoggedLayoutH;
	protected int m_iRowCreateLogCount;
	protected bool m_bRowCreateLogSuppressed;
	protected float m_fPostSetupInputRecoveryRemaining;
	protected float m_fPostSetupInputRecoveryAccumulator;
	protected float m_fInputDiagnosticAccumulator;
	protected int m_iInputDiagnosticLineCount;
	protected ScrollLayoutWidget m_ContentScroll;
	protected ScrollLayoutWidget m_ActionScroll;
	protected ScrollLayoutWidget m_FeedScroll;
	protected float m_fContentScrollY;
	protected float m_fActionScrollY;
	protected float m_fFeedScrollY;
	protected Widget m_wActionDialogRoot;
	protected Widget m_wPostLayoutRoot;
	protected bool m_bActionDialogOpen;
	protected string m_sPendingActionLabel;
	protected string m_sPendingActionCommand;
	protected string m_sPendingActionArgument;
	protected ref array<string> m_aPendingChoiceLabels = {};
	protected ref array<string> m_aPendingChoiceArguments = {};
	protected bool m_bMapTargetActive;
	protected bool m_bMapTargetNativeInvokersBound;
	protected bool m_bMapTargetLocationSelectionEnabled;
	protected bool m_bMapTargetConfirmOpen;
	protected bool m_bMapTargetDialogCursorActive;
	protected bool m_bMapTargetSelectionSuppressedForModal;
	protected bool m_bMapTargetCompleting;
	protected bool m_bMapTargetPromptRefreshQueued;
	protected bool m_bCampaignDebugMapOpenGateMapOpenBeforeAttempt;
	protected bool m_bCampaignDebugMapOpenGateToggleAttempted;
	protected bool m_bCampaignDebugMapOpenGateToggleAccepted;
	protected bool m_bCampaignDebugMapOpenGateDirectAttempted;
	protected bool m_bCampaignDebugMapOpenGateOpenedMap;
	protected string m_sMapTargetLabel;
	protected string m_sMapTargetCommand;
	protected string m_sMapTargetArgument;
	protected string m_sMapTargetTab;
	protected string m_sMapTargetStatusText;
	protected int m_iMapTargetGarrisonCount;
	protected vector m_vMapTargetPosition = "0 0 0";
	protected SCR_MapEntity m_MapTargetEntity;
	protected Widget m_wMapTargetPromptRoot;
	protected Widget m_wMapTargetPromptPanel;
	protected Widget m_wMapTargetPromptRule;
	protected TextWidget m_wMapTargetPromptText;
	protected Widget m_wMapTargetCursorRoot;
	protected Widget m_wMapTargetCursorHorizontalShadow;
	protected Widget m_wMapTargetCursorVerticalShadow;
	protected Widget m_wMapTargetCursorHorizontal;
	protected Widget m_wMapTargetCursorVertical;
	protected Widget m_wMapTargetCursorCenter;

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
			CloseCommandMapTargetSelection("component delete", true);
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
		m_iFrameSerial++;

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

		if (HST_SetupMapComponent.IsSetupBlocking())
		{
			m_bWasSetupBlocking = true;
			if (m_bMenuOpen)
				CloseMenu("setup blocking");
			m_bCommandMenuKeyDownLastFrame = false;
			m_bRawIKeyDownLastFrame = false;
			return;
		}

		if (m_bWasSetupBlocking)
		{
			m_bWasSetupBlocking = false;
			ResetInputLatchAfterSetupMap();
		}

		if (m_bMapTargetActive)
		{
			TickCommandMapTarget(timeSlice);
			return;
		}

		TickPostSetupInputRecovery(timeSlice);

		InputManager inputManager = GetGame().GetInputManager();
		if (inputManager)
		{
			inputManager.ActivateContext(COMMAND_MENU_INPUT_CONTEXT);
			inputManager.ActivateContext(COMMAND_MENU_NATIVE_I_CONTEXT);
			inputManager.ActivateContext(MENU_INPUT_CONTEXT);
			inputManager.ActivateAction(COMMAND_MENU_CUSTOM_ACTION);
			inputManager.ActivateAction(COMMAND_MENU_NATIVE_I_ACTION);
			m_bCommandMenuInputConsumedThisFrame = false;
			PollRawCommandMenuKey();
			PollCommandMenuInput(inputManager);
			TickInputDiagnostics(timeSlice, inputManager);

			if (m_bMenuOpen)
				inputManager.ActivateContext(MENU_CURSOR_CONTEXT);
		}
	}

	static HST_CommandMenuComponent GetLocalInstance()
	{
		if (!s_LocalInstance)
			RecoverLocalInstanceFromController("static lookup");

		return s_LocalInstance;
	}

	void ShowExternalNotification(string title, string message, float durationSeconds)
	{
		if (title.IsEmpty())
			title = "h-istasi";
		if (message.IsEmpty())
			return;

		HST_NotificationToastController.Get().Show(title + "_" + message, "h-istasi", "info", title, message, durationSeconds);
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

	void RunCampaignDebugRenderedProof(string requestId, string selectedTabId)
	{
		if (requestId.IsEmpty())
			return;

		selectedTabId = NormalizeTabId(selectedTabId);
		if (selectedTabId.IsEmpty())
			selectedTabId = "admin";

		OpenMenuToTab(selectedTabId, "campaign debug rendered proof");
		GetGame().GetCallqueue().CallLater(ReportCampaignDebugRenderedProof, 120, false, requestId, selectedTabId, 0);
		GetGame().GetCallqueue().CallLater(ReportCampaignDebugRenderedProof, 550, false, requestId, selectedTabId, 1);
	}

	void RunCampaignDebugMapOpenGateProof(string requestId)
	{
		if (requestId.IsEmpty())
			return;

		m_bCampaignDebugMapOpenGateMapOpenBeforeAttempt = false;
		m_bCampaignDebugMapOpenGateToggleAttempted = false;
		m_bCampaignDebugMapOpenGateToggleAccepted = false;
		m_bCampaignDebugMapOpenGateDirectAttempted = false;
		m_bCampaignDebugMapOpenGateOpenedMap = false;

		if (m_bMenuOpen)
			CloseMenu("campaign debug map-open gate preflight");

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity || !mapEntity.IsOpen())
		{
			MenuManager menuManager = GetGame().GetMenuManager();
			if (menuManager)
			{
				menuManager.OpenMenu(ChimeraMenuPreset.MapMenu);
				m_bCampaignDebugMapOpenGateOpenedMap = true;
			}
		}

		GetGame().GetCallqueue().CallLater(AttemptCampaignDebugMapOpenGateProof, 220, false, requestId);
		GetGame().GetCallqueue().CallLater(ReportCampaignDebugMapOpenGateProof, 620, false, requestId);
	}

	protected void AttemptCampaignDebugMapOpenGateProof(string requestId)
	{
		m_bCampaignDebugMapOpenGateMapOpenBeforeAttempt = IsNativeMapOpen();
		m_bCampaignDebugMapOpenGateToggleAttempted = true;
		m_bCampaignDebugMapOpenGateToggleAccepted = TryToggleCommandMenu("campaign debug map-open gate input");
		m_bCampaignDebugMapOpenGateDirectAttempted = true;
		OpenMenuToTab("admin", "campaign debug map-open gate direct");
	}

	protected void ReportCampaignDebugMapOpenGateProof(string requestId)
	{
		string report = BuildCampaignDebugMapOpenGateProofReport(requestId);
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (request)
			request.ReportCampaignDebugCommandMenuMapOpenGateProof(requestId, report);
		else
			Print("h-istasi menu | campaign debug map-open gate proof request bridge missing | " + report, LogLevel.WARNING);

		Print("h-istasi menu | campaign debug map-open gate proof | " + report);
		if (m_bMenuOpen)
			CloseMenu("campaign debug map-open gate cleanup");

		if (m_bCampaignDebugMapOpenGateOpenedMap)
			CloseCampaignDebugMapOpenGateNativeMap();
	}

	protected string BuildCampaignDebugMapOpenGateProofReport(string requestId)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		bool mapOpenAfterAttempt = mapEntity && mapEntity.IsOpen();
		Widget root = m_wMenuRoot;
		bool rootVisible = root && root.IsVisibleInHierarchy();
		string rootSummary = "none";
		if (root)
			rootSummary = HST_UIDebug.WidgetSummary(root);

		string report = string.Format("request %1 | player %2 | localOwner %3 | mapOpenBeforeAttempt %4 | toggleAttempted %5 | toggleAccepted %6 | directAttempted %7 | mapOpenAfterAttempt %8 | menuOpenAfterAttempt %9", requestId, ResolveLocalPlayerId(), m_bIsLocalOwner, m_bCampaignDebugMapOpenGateMapOpenBeforeAttempt, m_bCampaignDebugMapOpenGateToggleAttempted, m_bCampaignDebugMapOpenGateToggleAccepted, m_bCampaignDebugMapOpenGateDirectAttempted, mapOpenAfterAttempt, m_bMenuOpen);
		report = report + string.Format(" | rootAfterAttempt %1", rootVisible);
		report = report + string.Format(" | openedMapForProof %1 | top %2 | build %3", m_bCampaignDebugMapOpenGateOpenedMap, HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode()), COMMAND_MENU_BUILD);
		report = report + " | rootSummary " + ShortenText(rootSummary, 140);
		return report;
	}

	protected void CloseCampaignDebugMapOpenGateNativeMap()
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (menuManager)
			menuManager.CloseMenuByPreset(ChimeraMenuPreset.MapMenu);

		m_bCampaignDebugMapOpenGateOpenedMap = false;
	}

	protected void ReportCampaignDebugRenderedProof(string requestId, string selectedTabId, int passIndex)
	{
		string report = BuildCampaignDebugRenderedProofReport(requestId, selectedTabId, passIndex);
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (request)
			request.ReportCampaignDebugCommandMenuProof(requestId, report);
		else
			Print("h-istasi menu | campaign debug rendered proof request bridge missing | " + report, LogLevel.WARNING);

		Print("h-istasi menu | campaign debug rendered proof | " + report);
		if (passIndex > 0 && m_bMenuOpen)
			CloseMenu("campaign debug rendered proof");
	}

	protected string BuildCampaignDebugRenderedProofReport(string requestId, string selectedTabId, int passIndex)
	{
		array<string> expectedWidgets = {};
		BuildCampaignDebugRenderedProofWidgetList(expectedWidgets);
		Widget root = m_wMenuRoot;
		bool rootVisible = root && root.IsVisibleInHierarchy();
		int expected;
		int ready;
		string missing = "";
		string hidden = "";
		string zero = "";
		string negative = "";
		string offscreen = "";
		foreach (string widgetName : expectedWidgets)
		{
			if (widgetName.IsEmpty())
				continue;

			expected++;
			Widget widget = null;
			if (root)
			{
				widget = root.FindAnyWidget(widgetName);
				if (!widget && root.GetName() == widgetName)
					widget = root;
			}

			if (!widget)
			{
				missing = AppendCampaignDebugCsvValue(missing, widgetName);
				continue;
			}

			bool visible = widget.IsVisibleInHierarchy();
			bool hasNegativeBounds = CampaignDebugWidgetHasNegativeBounds(widget);
			bool hasZeroBounds = CampaignDebugWidgetHasZeroBounds(widget);
			bool hasBounds = CampaignDebugWidgetHasUsableBounds(widget);
			bool outsideRoot = hasBounds && CampaignDebugWidgetIsOutsideRoot(widget, root);
			if (visible && hasBounds && !outsideRoot)
			{
				ready++;
				continue;
			}

			if (!visible)
				hidden = AppendCampaignDebugCsvValue(hidden, CampaignDebugWidgetProblem(widgetName, widget));
			if (hasNegativeBounds)
				negative = AppendCampaignDebugCsvValue(negative, CampaignDebugWidgetProblem(widgetName, widget));
			else if (hasZeroBounds)
				zero = AppendCampaignDebugCsvValue(zero, CampaignDebugWidgetProblem(widgetName, widget));
			else if (outsideRoot)
				offscreen = AppendCampaignDebugCsvValue(offscreen, CampaignDebugWidgetProblem(widgetName, widget));
		}

		bool readyOk = rootVisible && expected > 0 && ready == expected && missing.IsEmpty() && hidden.IsEmpty() && zero.IsEmpty() && negative.IsEmpty() && offscreen.IsEmpty();
		string rootSummary = "none";
		if (root)
			rootSummary = HST_UIDebug.WidgetSummary(root);

		string report = string.Format("request %1 | player %2 | pass %3 | menuOpen %4 | tab %5 | requestedTab %6 | root %7 | readyOk %8 | ready %9", requestId, ResolveLocalPlayerId(), passIndex, m_bMenuOpen, m_sSelectedTab, selectedTabId, rootVisible, readyOk, ready);
		report = report + string.Format("/%1 | actions %2 | tabs %3 | top %4", expected, m_aActionLabels.Count(), m_aTabLabels.Count(), HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode()));
		report = report + string.Format(" | missing %1 | hidden %2 | zero %3 | negative %4 | offscreen %5", CampaignDebugEmptyList(missing), CampaignDebugEmptyList(hidden), CampaignDebugEmptyList(zero), CampaignDebugEmptyList(negative), CampaignDebugEmptyList(offscreen));
		report = report + " | rootSummary " + ShortenText(rootSummary, 140);
		return report;
	}

	protected void BuildCampaignDebugRenderedProofWidgetList(array<string> widgetNames)
	{
		if (!widgetNames)
			return;

		widgetNames.Insert("HST_CommandMenuRoot");
		widgetNames.Insert("CommandSurface");
		widgetNames.Insert("Header");
		widgetNames.Insert("HeaderTitle");
		widgetNames.Insert("HeaderSubtitle");
		widgetNames.Insert("CloseButton");
		widgetNames.Insert("CloseLabel");
		widgetNames.Insert("NavigationPanel");
		widgetNames.Insert("TabScroll");
		widgetNames.Insert("TabItems");
		widgetNames.Insert("StatsPanel");
		widgetNames.Insert("StatLayout");
		widgetNames.Insert("MainPanel");
		widgetNames.Insert("MainScroll");
		widgetNames.Insert("MainItems");
		widgetNames.Insert("ActivityPanel");
		widgetNames.Insert("ActionsPanel");
		widgetNames.Insert("ActionsScroll");
		widgetNames.Insert("ActionsItems");
	}

	protected bool CampaignDebugWidgetHasUsableBounds(Widget widget)
	{
		if (!widget)
			return false;

		float w;
		float h;
		widget.GetScreenSize(w, h);
		return w >= 1.0 && h >= 1.0;
	}

	protected bool CampaignDebugWidgetHasNegativeBounds(Widget widget)
	{
		if (!widget)
			return false;

		float w;
		float h;
		widget.GetScreenSize(w, h);
		return w < 0.0 || h < 0.0;
	}

	protected bool CampaignDebugWidgetHasZeroBounds(Widget widget)
	{
		if (!widget)
			return false;

		float w;
		float h;
		widget.GetScreenSize(w, h);
		return w >= 0.0 && h >= 0.0 && (w < 1.0 || h < 1.0);
	}

	protected bool CampaignDebugWidgetIsOutsideRoot(Widget widget, Widget root)
	{
		if (!widget || !root)
			return false;
		if (widget == root)
			return false;

		float rootX;
		float rootY;
		float rootW;
		float rootH;
		root.GetScreenPos(rootX, rootY);
		root.GetScreenSize(rootW, rootH);
		if (rootW < 1.0 || rootH < 1.0)
			return false;

		float x;
		float y;
		float w;
		float h;
		widget.GetScreenPos(x, y);
		widget.GetScreenSize(w, h);
		if (w < 1.0 || h < 1.0)
			return false;

		return (x + w) < rootX || (y + h) < rootY || x > (rootX + rootW) || y > (rootY + rootH);
	}

	protected string CampaignDebugWidgetProblem(string widgetName, Widget widget)
	{
		if (!widget)
			return widgetName + "[null]";

		float x;
		float y;
		float w;
		float h;
		widget.GetScreenPos(x, y);
		widget.GetScreenSize(w, h);
		return string.Format("%1[vis=%2;hier=%3;z=%4;pos=%5:%6;size=%7x%8]", widgetName, widget.IsVisible(), widget.IsVisibleInHierarchy(), widget.GetZOrder(), Math.Round(x), Math.Round(y), Math.Round(w), Math.Round(h));
	}

	protected string AppendCampaignDebugCsvValue(string values, string value)
	{
		if (value.IsEmpty())
			return values;
		if (values.IsEmpty())
			return value;

		return values + "," + value;
	}

	protected string CampaignDebugEmptyList(string values)
	{
		if (values.IsEmpty())
			return "none";

		return values;
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

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.ActivateContext(MENU_INPUT_CONTEXT);
		inputManager.ActivateContext(COMMAND_MENU_INPUT_CONTEXT);
		inputManager.ActivateContext(COMMAND_MENU_NATIVE_I_CONTEXT);
		inputManager.ActivateAction(COMMAND_MENU_CUSTOM_ACTION);
		inputManager.ActivateAction(COMMAND_MENU_NATIVE_I_ACTION);
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
		inputManager.ActivateContext(COMMAND_MENU_INPUT_CONTEXT);
		inputManager.ActivateContext(COMMAND_MENU_NATIVE_I_CONTEXT);
		inputManager.ActivateAction(COMMAND_MENU_CUSTOM_ACTION);
		inputManager.ActivateAction(COMMAND_MENU_NATIVE_I_ACTION);
	}

	void RunCommandFromContext(string tabId, string commandId, string argument = "")
	{
		tabId = NormalizeTabId(tabId);
		if (!tabId.IsEmpty())
			m_sSelectedTab = tabId;

		if (commandId.IsEmpty())
			return;

		string label = BuildContextActionLabel(commandId, argument);
		if (commandId == "member_promote_commander_choose")
		{
			ShowCommanderTransferChoiceDialog(label, argument);
			return;
		}
		if (commandId == "support_recall_choose")
		{
			ShowSupportRecallChoiceDialog(label, argument);
			return;
		}

		if (ShouldSelectMapTarget(commandId))
		{
			BeginCommandMapTargetSelection(label, commandId, argument);
			return;
		}

		if (ShouldConfirmAction(commandId))
		{
			ShowActionConfirmDialog(label, commandId, argument);
			return;
		}

		RequestConfirmedAction(label, commandId, argument);
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

	bool OnWidgetClicked(int widgetId, int button = 0)
	{
		if (IsDuplicateWidgetActivation(widgetId, button))
			return true;

		if (widgetId == ACTION_MODAL_CANCEL_WIDGET_ID)
		{
			if (!CanHandleActionDialogInput())
				return false;

			CancelPendingActionDialog();
			return true;
		}

		if (widgetId == ACTION_MODAL_CONFIRM_WIDGET_ID)
		{
			if (!CanHandleActionDialogInput())
				return false;

			ConfirmPendingActionDialog();
			return true;
		}

		if (widgetId >= ACTION_CHOICE_WIDGET_ID_BASE && widgetId < ACTION_CHOICE_WIDGET_ID_LIMIT)
		{
			if (!CanHandleActionDialogInput())
				return false;

			SelectPendingActionChoice(widgetId - ACTION_CHOICE_WIDGET_ID_BASE);
			return true;
		}

		if (!m_bMenuOpen)
			return m_bActionDialogOpen && CanHandleActionDialogInput();

		if (m_bActionDialogOpen)
			return true;

		if (!CanHandleCommandMenuInput())
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

	protected bool IsDuplicateWidgetActivation(int widgetId, int button)
	{
		if (widgetId == 0)
			return false;

		if (m_iLastActivatedFrame == m_iFrameSerial && m_iLastActivatedWidgetId == widgetId && m_iLastActivatedButton == button)
			return true;

		m_iLastActivatedWidgetId = widgetId;
		m_iLastActivatedButton = button;
		m_iLastActivatedFrame = m_iFrameSerial;
		return false;
	}

	protected bool RegisterInputListeners()
	{
		if (m_bInputRegistered)
			return true;

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
		{
			if (!m_bLoggedInputManagerMissing)
			{
				m_bLoggedInputManagerMissing = true;
				Print("h-istasi menu | input manager unavailable while registering command menu input", LogLevel.WARNING);
			}

			return false;
		}

		EnsureInputConfig(inputManager);
		EnsureIKeyBinding(inputManager);

		inputManager.AddActionListener(COMMAND_MENU_CUSTOM_ACTION, EActionTrigger.DOWN, OnCustomCommandMenuInput);
		inputManager.AddActionListener(COMMAND_MENU_NATIVE_I_ACTION, EActionTrigger.DOWN, OnNativeCommandMenuInput);
		inputManager.AddActionListener(COMMAND_MENU_UP_ACTION, EActionTrigger.DOWN, OnSelectPreviousInput);
		inputManager.AddActionListener(COMMAND_MENU_DOWN_ACTION, EActionTrigger.DOWN, OnSelectNextInput);
		inputManager.AddActionListener(COMMAND_MENU_SELECT_ACTION, EActionTrigger.DOWN, OnExecuteSelectedInput);
		inputManager.AddActionListener(COMMAND_MENU_BACK_ACTION, EActionTrigger.DOWN, OnCloseMenuInput);
		m_bInputRegistered = true;
		PrintCommandMenuReadyOnce("input registered");
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
		inputManager.RemoveActionListener(COMMAND_MENU_NATIVE_I_ACTION, EActionTrigger.DOWN, OnNativeCommandMenuInput);
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

		Print(string.Format("h-istasi menu | action input detected source=%1 value=%2 menuOpen=%3 top=%4", COMMAND_MENU_CUSTOM_ACTION, value, m_bMenuOpen, HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode())));
		m_bCommandMenuInputConsumedThisFrame = true;
		TryToggleCommandMenu("custom action");
	}

	protected void OnNativeCommandMenuInput(float value, EActionTrigger reason)
	{
		if (reason != EActionTrigger.DOWN)
			return;

		Print(string.Format("h-istasi menu | native I action input detected source=%1 value=%2 menuOpen=%3 top=%4", COMMAND_MENU_NATIVE_I_ACTION, value, m_bMenuOpen, HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode())));
		m_bCommandMenuInputConsumedThisFrame = true;
		TryToggleCommandMenu("native " + COMMAND_MENU_NATIVE_I_ACTION);
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
		else if (inputManager.GetActionTriggered(COMMAND_MENU_NATIVE_I_ACTION))
		{
			keyDown = true;
			source = COMMAND_MENU_NATIVE_I_ACTION;
		}

		if (m_bCommandMenuInputConsumedThisFrame)
		{
			m_bCommandMenuKeyDownLastFrame = keyDown;
			return;
		}

		if (keyDown && !m_bCommandMenuKeyDownLastFrame)
		{
			Print(string.Format("h-istasi menu | polled action input detected source=%1 menuOpen=%2 top=%3", source, m_bMenuOpen, HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode())));
			m_bCommandMenuInputConsumedThisFrame = true;
			TryToggleCommandMenu("poll " + source);
		}

		m_bCommandMenuKeyDownLastFrame = keyDown;
	}

	protected void PollRawCommandMenuKey()
	{
		int keyState = Debug.KeyState(KeyCode.KC_I);
		bool keyDown = keyState != 0;
		if (keyDown && !m_bRawIKeyDownLastFrame)
		{
			Print(string.Format("h-istasi menu | raw I input detected menuOpen=%1 top=%2", m_bMenuOpen, HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode())));
			m_bCommandMenuInputConsumedThisFrame = true;
			TryToggleCommandMenu("raw KC_I");
			Debug.ClearKey(KeyCode.KC_I);
		}

		m_bRawIKeyDownLastFrame = keyDown;
	}

	protected void TickInputDiagnostics(float timeSlice, InputManager inputManager)
	{
		if (!IsDebugLoggingEnabled() || m_iInputDiagnosticLineCount >= INPUT_DIAGNOSTIC_MAX_LINES)
			return;

		m_fInputDiagnosticAccumulator += timeSlice;
		if (m_fInputDiagnosticAccumulator < INPUT_DIAGNOSTIC_INTERVAL_SECONDS)
			return;

		m_fInputDiagnosticAccumulator = 0;
		m_iInputDiagnosticLineCount++;

		int rawState = Debug.KeyState(KeyCode.KC_I);
		bool actionTriggered = inputManager && inputManager.GetActionTriggered(COMMAND_MENU_CUSTOM_ACTION);
		bool nativeActionTriggered = inputManager && inputManager.GetActionTriggered(COMMAND_MENU_NATIVE_I_ACTION);
		string heartbeat = string.Format(
			"h-istasi menu debug | input heartbeat localPlayer=%1 resolvedPlayer=%2 ownerPlayer=%3 localOwner=%4 inputRegistered=%5 configRegistered=%6 customBinding=%7 actionTriggered=%8 rawIState=%9",
			SCR_PlayerController.GetLocalPlayerId(),
			ResolveLocalPlayerId(),
			ResolveOwnerPlayerId(m_OwnerEntity),
			m_bIsLocalOwner,
			m_bInputRegistered,
			m_bInputConfigRegistered,
			m_bCustomBindingReady,
			actionTriggered,
			rawState
		);
		heartbeat = heartbeat + string.Format(" nativeActionTriggered=%1", nativeActionTriggered);
		heartbeat = heartbeat + string.Format(" setupBlocking=%1 menuOpen=%2 top=%3", HST_SetupMapComponent.IsSetupBlocking(), m_bMenuOpen, HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode()));
		Print(heartbeat);
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
			return false;
		}

		if (!m_bMenuOpen && IsNativeMapOpen())
		{
			DebugCommandMenuToggleRefused(source, "map open");
			return false;
		}

		if (m_fCommandMenuDebounceRemaining > 0)
		{
			DebugCommandMenuToggleRefused(source, "debounce");
			return false;
		}

		if (m_bMenuOpen && !m_bActionDialogOpen && !IsCommandMenuTopmost())
		{
			DebugCommandMenuToggleRefused(source, "not topmost");
			return false;
		}

		if (!HST_CommandMenuRequestComponent.GetLocalOwner() && !Replication.IsServer())
			Print("h-istasi menu | input toggle continuing without local request bridge; menu will show cached/local data if available", LogLevel.WARNING);

		SCR_RespawnSystemComponent.CloseRespawnMenu();
		m_fCommandMenuDebounceRemaining = 0.15;
		if (m_bMenuOpen && m_bActionDialogOpen)
		{
			DebugLog("input toggle cancelled action dialog");
			CancelPendingActionDialog();
			return true;
		}

		DebugLog(string.Format("input toggle source=%1 setupBlocking=%2 menuOpen=%3 localOwner=%4", source, HST_SetupMapComponent.IsSetupBlocking(), m_bMenuOpen, m_bIsLocalOwner));
		Print(string.Format("h-istasi menu | input toggle source=%1 menuOpen=%2 localOwner=%3 inputRegistered=%4 customBinding=%5", source, m_bMenuOpen, m_bIsLocalOwner, m_bInputRegistered, m_bCustomBindingReady));
		ToggleMenu(source);
		return true;
	}

	protected bool IsNativeMapOpen()
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		return mapEntity && mapEntity.IsOpen();
	}

	protected void DebugCommandMenuToggleRefused(string source, string reason)
	{
		string message = string.Format("h-istasi menu | input refused source=%1 reason=%2 setupBlocking=%3 menuOpen=%4 localOwner=%5 inputRegistered=%6 customBinding=%7 debounce=%8", source, reason, HST_SetupMapComponent.IsSetupBlocking(), m_bMenuOpen, m_bIsLocalOwner, m_bInputRegistered, m_bCustomBindingReady, m_fCommandMenuDebounceRemaining);
		if (reason == "debounce")
			Print(message);
		else
			Print(message, LogLevel.WARNING);
		DebugLog(string.Format("input refused source=%1 reason=%2 setupBlocking=%3 menuOpen=%4 localOwner=%5 inputRegistered=%6 customBinding=%7 debounce=%8", source, reason, HST_SetupMapComponent.IsSetupBlocking(), m_bMenuOpen, m_bIsLocalOwner, m_bInputRegistered, m_bCustomBindingReady, m_fCommandMenuDebounceRemaining));
	}

	protected void OnSelectPreviousInput(float value, EActionTrigger reason)
	{
		if (m_bActionDialogOpen || !CanHandleCommandMenuInput())
			return;

		if (reason == EActionTrigger.DOWN)
			SelectPreviousAction();
	}

	protected void OnSelectNextInput(float value, EActionTrigger reason)
	{
		if (m_bActionDialogOpen || !CanHandleCommandMenuInput())
			return;

		if (reason == EActionTrigger.DOWN)
			SelectNextAction();
	}

	protected void OnExecuteSelectedInput(float value, EActionTrigger reason)
	{
		if (m_bActionDialogOpen || !CanHandleCommandMenuInput())
			return;

		if (reason == EActionTrigger.DOWN)
			ExecuteSelectedAction();
	}

	protected void OnCloseMenuInput(float value, EActionTrigger reason)
	{
		if (reason != EActionTrigger.DOWN)
			return;

		if (m_bActionDialogOpen)
		{
			if (!CanHandleActionDialogInput())
				return;

			CancelPendingActionDialog();
			return;
		}

		if (!CanHandleCommandMenuInput())
			return;

		CloseMenu("menu back action");
	}

	protected bool IsCommandMenuTopmost()
	{
		return HST_UIRootService.Get().IsTopmost(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent");
	}

	protected bool CanHandleCommandMenuInput()
	{
		return HST_UIRootService.Get().CanHandleScreenInput(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent");
	}

	protected bool CanHandleActionDialogInput()
	{
		return HST_UIRootService.Get().CanHandleModalInput(HST_EUIScreenMode.ACTION_DIALOG, ACTION_DIALOG_OWNER);
	}

	protected void BecomeLocalOwner()
	{
		if (s_LocalInstance == this)
			return;

		s_LocalInstance = this;
		m_fInputRetryAccumulator = 0;
		DebugLog("local player menu component ready");
		RegisterInputListeners();
		PrintCommandMenuReadyOnce("local owner");
	}

	protected static void RecoverLocalInstanceFromController(string reason)
	{
		PlayerController localController = GetGame().GetPlayerController();
		if (!localController)
			return;

		HST_CommandMenuComponent localComponent = HST_CommandMenuComponent.Cast(localController.FindComponent(HST_CommandMenuComponent));
		if (!localComponent)
			return;

		localComponent.ClaimRecoveredLocalOwner(reason);
	}

	protected void ClaimRecoveredLocalOwner(string reason)
	{
		if (!m_bIsLocalOwner)
			m_bIsLocalOwner = true;

		DebugLog("recovering local command menu component via " + reason);
		BecomeLocalOwner();
	}

	protected void PrintCommandMenuReadyOnce(string reason)
	{
		if (m_bLoggedLocalReadyPrinted)
			return;

		m_bLoggedLocalReadyPrinted = true;
		Print(string.Format("h-istasi menu | local command menu component ready via %1 | localPlayer=%2 ownerPlayer=%3 inputRegistered=%4 customBinding=%5 | build=%6", reason, SCR_PlayerController.GetLocalPlayerId(), ResolveOwnerPlayerId(m_OwnerEntity), m_bInputRegistered, m_bCustomBindingReady, COMMAND_MENU_BUILD));
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
		if (m_bMenuOpen)
		{
			DebugLog("open ignored; already open via " + source);
			return;
		}

		if (!HST_UIRootService.Get().CanOpen(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent"))
		{
			RecoverStaleSetupRootStateForCommandOpen(source);
			if (!HST_UIRootService.Get().CanOpen(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent"))
			{
				DebugLog("open refused by UI root via " + source);
				Print(string.Format("h-istasi menu | open refused by UI root source=%1 current=%2 top=%3 owner=%4", source, HST_UIConstants.ModeName(HST_UIRootService.Get().GetCurrentMode()), HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode()), HST_UIRootService.Get().GetTopmostOwner()), LogLevel.WARNING);
				m_fCommandMenuDebounceRemaining = 0;
				return;
			}
		}

		if (HST_SetupMapComponent.IsSetupBlocking())
		{
			DebugLog("open refused while setup is still blocking via " + source);
			Print(string.Format("h-istasi menu | open refused by setup blocking source=%1 current=%2 top=%3 owner=%4", source, HST_UIConstants.ModeName(HST_UIRootService.Get().GetCurrentMode()), HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode()), HST_UIRootService.Get().GetTopmostOwner()), LogLevel.WARNING);
			m_fCommandMenuDebounceRemaining = 0;
			return;
		}

		if (IsNativeMapOpen())
		{
			DebugLog("open refused while native map is open via " + source);
			Print(string.Format("h-istasi menu | open refused by native map source=%1 current=%2 top=%3 owner=%4", source, HST_UIConstants.ModeName(HST_UIRootService.Get().GetCurrentMode()), HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode()), HST_UIRootService.Get().GetTopmostOwner()), LogLevel.WARNING);
			m_fCommandMenuDebounceRemaining = 0;
			return;
		}

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
		if (!RenderMenu())
		{
			m_bMenuOpen = false;
			HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent");
			ClearActionDialog();
			ClearWidgets();
			DebugLog("open aborted: command menu layout root unavailable");
			Print(string.Format("h-istasi menu | open aborted because command menu root unavailable source=%1", source), LogLevel.WARNING);
			return;
		}

		DebugLog("opened via " + source);
		ShowMenuHint("Command menu opened", "h-istasi", 2.0);
	}

	protected void RecoverStaleSetupRootStateForCommandOpen(string source)
	{
		if (HST_SetupMapComponent.IsSetupBlocking())
			return;

		if (HST_UIRootService.Get().GetCurrentMode() != HST_EUIScreenMode.SETUP_MAP)
			return;

		HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.SETUP_MAP, "HST_SetupMapComponent");
		DebugLog("cleared stale setup UI root before command menu open via " + source);
	}

	protected void CloseMenu(string source = "unknown")
	{
		if (!m_bMenuOpen)
			return;

		m_bMenuOpen = false;
		HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent");
		ClearActionDialog();
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

		if (m_bActionDialogOpen)
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
			m_sLastActionName = m_aActionLabels[actionIndex];
			m_sLastResult = "h-istasi command | " + m_aActionLabels[actionIndex] + " | " + m_aActionDisabledReasons[actionIndex];
			ShowMenuHint(m_sLastResult, "h-istasi", 2.0);
			RenderMenu();
			return;
		}

		if (m_aActionCommands[actionIndex] == "member_promote_commander_choose")
		{
			ShowCommanderTransferChoiceDialog(m_aActionLabels[actionIndex], m_aActionArguments[actionIndex]);
			return;
		}
		if (m_aActionCommands[actionIndex] == "support_recall_choose")
		{
			ShowSupportRecallChoiceDialog(m_aActionLabels[actionIndex], m_aActionArguments[actionIndex]);
			return;
		}

		if (ShouldSelectMapTarget(m_aActionCommands[actionIndex]))
		{
			BeginCommandMapTargetSelection(m_aActionLabels[actionIndex], m_aActionCommands[actionIndex], m_aActionArguments[actionIndex]);
			return;
		}

		if (ShouldConfirmAction(m_aActionCommands[actionIndex]))
		{
			ShowActionConfirmDialog(m_aActionLabels[actionIndex], m_aActionCommands[actionIndex], m_aActionArguments[actionIndex]);
			return;
		}

		RequestConfirmedAction(m_aActionLabels[actionIndex], m_aActionCommands[actionIndex], m_aActionArguments[actionIndex]);
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

	protected void RequestConfirmedAction(string label, string commandId, string argument)
	{
		if (label.IsEmpty())
			label = commandId;

		m_sLastActionName = label;
		m_sLastResult = "h-istasi command | requested " + label;
		ShowMenuHint(m_sLastResult, "h-istasi", 2.0);
		RequestAction(commandId, argument);
		if (m_bMenuOpen)
			RenderMenu();
	}

	protected bool ShouldConfirmAction(string commandId)
	{
		if (commandId == "new_campaign")
			return true;
		if (commandId == "admin_purge_hst_native_markers")
			return true;
		if (commandId == "move_hq_here" || commandId == "move_hq" || commandId == "rebuild_hq_assets")
			return true;
		if (commandId == "mission_random" || commandId == "mission_zone" || commandId == "mission_category" || commandId == "complete_mission")
			return true;
		if (commandId == "mission_asset_deliver" || commandId == "mission_asset_sabotage" || commandId == "mission_vehicle_capture" || commandId == "mission_captive_extract")
			return true;
		if (commandId == "remove_garrison" || commandId == "cancel_support" || commandId == "support_recall" || commandId == "civilian_aid")
			return true;
		if (commandId == "member_promote_commander" || commandId == "admin_force_self_commander")
			return true;
		if (commandId == "call_supply" || commandId == "support_qrf" || commandId == "support_fire" || commandId == "support_search")
			return true;
		if (commandId == "support_gbu" || commandId == "support_umpk" || commandId == "support_kh55")
			return true;
		if (commandId == "activate_zone" || commandId == "deactivate_zone" || commandId == "capture_zone" || commandId == "progress_zone")
			return true;
		if (commandId == "debug_mission" || commandId == "debug_mission_id" || commandId == "award_small" || commandId == "income_now" || commandId == "progress_mission")
			return true;
		if (commandId == "admin_persistence_smoke_test" || commandId == "admin_phase23_failed_action_sample")
			return true;
		if (commandId.Contains("admin_") && (commandId.Contains("_seed") || commandId.Contains("_force") || commandId.Contains("_simulate") || commandId.Contains("_clear") || commandId.Contains("_queue") || commandId.Contains("_start") || commandId.Contains("_kill") || commandId.Contains("_succeed") || commandId.Contains("_apply") || commandId.Contains("_resolve")))
			return true;
		if (commandId.Contains("force_victory") || commandId.Contains("force_loss"))
			return true;

		return false;
	}

	protected bool ShouldSelectMapTarget(string commandId)
	{
		if (commandId == "recruit_zone" || commandId == "remove_garrison")
			return true;
		if (IsSupportMapTargetCommand(commandId))
			return true;

		return false;
	}

	protected bool IsSupportMapTargetCommand(string commandId)
	{
		if (commandId == "call_supply" || commandId == "support_qrf" || commandId == "support_fire" || commandId == "support_search")
			return true;
		if (commandId == "support_gbu" || commandId == "support_umpk" || commandId == "support_kh55")
			return true;

		return false;
	}

	protected bool ShouldReturnToCommandMenuAfterMapTarget(string commandId)
	{
		if (commandId == "recruit_zone" || commandId == "remove_garrison")
			return true;

		return IsSupportMapTargetCommand(commandId);
	}

	protected void BeginCommandMapTargetSelection(string label, string commandId, string argument)
	{
		if (label.IsEmpty())
			label = commandId;

		CloseMenu("map target selection");
		ClearActionDialog();
		CloseCommandMapTargetSelection("restart map target selection", true);

		m_bMapTargetActive = true;
		m_bMapTargetCompleting = false;
		m_bMapTargetConfirmOpen = false;
		m_sMapTargetLabel = label;
		m_sMapTargetCommand = commandId;
		m_sMapTargetArgument = argument;
		m_sMapTargetTab = m_sSelectedTab;
		m_sMapTargetStatusText = "Select a target location on the map.";
		m_iMapTargetGarrisonCount = 0;
		m_vMapTargetPosition = "0 0 0";
		BindCommandMapTargetInvokers();
		OpenCommandMapTarget();
		ShowMenuHint("Select a target location on the map", "h-istasi", 3.0);
	}

	protected void TickCommandMapTarget(float timeSlice)
	{
		if (!m_bMapTargetActive)
			return;

		if (!m_MapTargetEntity)
			m_MapTargetEntity = SCR_MapEntity.GetMapInstance();

		if (m_MapTargetEntity && m_MapTargetEntity.IsOpen())
		{
			RefreshCommandMapTargetPrompt();
			ApplyCommandMapDialogState();
			RefreshCommandMapTargetCursor();
			if (!m_bMapTargetConfirmOpen)
				SetCommandMapLocationSelectionEnabled(true);
			return;
		}

		OpenCommandMapTarget();
	}

	protected void OpenCommandMapTarget()
	{
		if (!m_bMapTargetActive)
			return;

		if (!m_MapTargetEntity)
			m_MapTargetEntity = SCR_MapEntity.GetMapInstance();

		if (m_MapTargetEntity && m_MapTargetEntity.IsOpen())
		{
			RefreshCommandMapTargetPrompt();
			SetCommandMapLocationSelectionEnabled(!m_bMapTargetConfirmOpen);
			return;
		}

		MenuManager menuManager = GetGame().GetMenuManager();
		if (!menuManager)
		{
			CancelCommandMapTargetSelection("map menu unavailable");
			return;
		}

		menuManager.OpenMenu(ChimeraMenuPreset.MapMenu);
		GetGame().GetCallqueue().CallLater(RefreshCommandMapTargetAfterOpen, 50, false);
		GetGame().GetCallqueue().CallLater(RefreshCommandMapTargetAfterOpen, 250, false);
	}

	protected void RefreshCommandMapTargetAfterOpen()
	{
		if (!m_bMapTargetActive)
			return;

		if (!m_MapTargetEntity)
			m_MapTargetEntity = SCR_MapEntity.GetMapInstance();

		if (!m_MapTargetEntity || !m_MapTargetEntity.IsOpen())
			return;

		RefreshCommandMapTargetPrompt();
		SetCommandMapLocationSelectionEnabled(!m_bMapTargetConfirmOpen);
		ApplyCommandMapDialogState();
		RefreshCommandMapTargetCursor();
	}

	protected void BindCommandMapTargetInvokers()
	{
		if (m_bMapTargetNativeInvokersBound)
			return;

		SCR_MapEntity.GetOnSelection().Insert(OnCommandMapTargetSelection);
		SCR_MapEntity.GetOnMapOpenComplete().Insert(OnCommandMapTargetOpenComplete);
		SCR_MapEntity.GetOnMapClose().Insert(OnCommandMapTargetClose);
		m_bMapTargetNativeInvokersBound = true;
	}

	protected void UnbindCommandMapTargetInvokers()
	{
		if (!m_bMapTargetNativeInvokersBound)
			return;

		SCR_MapEntity.GetOnSelection().Remove(OnCommandMapTargetSelection);
		SCR_MapEntity.GetOnMapOpenComplete().Remove(OnCommandMapTargetOpenComplete);
		SCR_MapEntity.GetOnMapClose().Remove(OnCommandMapTargetClose);
		m_bMapTargetNativeInvokersBound = false;
	}

	protected void OnCommandMapTargetOpenComplete(MapConfiguration config)
	{
		if (!m_bMapTargetActive)
			return;

		if (config && config.MapEntityMode != EMapEntityMode.FULLSCREEN)
			return;

		if (!m_MapTargetEntity)
			m_MapTargetEntity = SCR_MapEntity.GetMapInstance();

		RefreshCommandMapTargetPrompt();
		SetCommandMapLocationSelectionEnabled(!m_bMapTargetConfirmOpen);
		ApplyCommandMapDialogState();
		RefreshCommandMapTargetCursor();
	}

	protected void OnCommandMapTargetClose(MapConfiguration config)
	{
		if (!m_bMapTargetActive)
			return;

		if (m_bMapTargetCompleting)
			return;

		CancelCommandMapTargetSelection("map closed");
	}

	protected void OnCommandMapTargetSelection(vector screenPos)
	{
		if (!m_bMapTargetActive || m_bMapTargetConfirmOpen)
			return;

		if (!m_MapTargetEntity)
			m_MapTargetEntity = SCR_MapEntity.GetMapInstance();
		if (!m_MapTargetEntity || !m_MapTargetEntity.IsOpen())
			return;

		float worldX;
		float worldZ;
		m_MapTargetEntity.ScreenToWorld(Math.Round(screenPos[0]), Math.Round(screenPos[2]), worldX, worldZ);

		m_vMapTargetPosition = "0 0 0";
		m_vMapTargetPosition[0] = worldX;
		m_vMapTargetPosition[2] = worldZ;
		m_sMapTargetStatusText = string.Format("Target selected at X %1 Z %2. Confirm or choose again.", Math.Round(worldX), Math.Round(worldZ));
		SetCommandMapLocationSelectionEnabled(false);
		RefreshCommandMapTargetPrompt();
		if (m_sMapTargetCommand == "recruit_zone")
		{
			ShowCommandMapTargetGarrisonCountDialog();
			return;
		}

		ShowCommandMapTargetConfirmDialog();
	}

	protected void ShowCommandMapTargetGarrisonCountDialog()
	{
		if (m_iMapTargetGarrisonCount <= 0)
			m_iMapTargetGarrisonCount = 2;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			ShowCommandMapTargetConfirmDialog();
			return;
		}

		ClearActionDialog();
		if (!m_WidgetHandler)
		{
			m_WidgetHandler = new HST_CommandMenuWidgetHandler();
			m_WidgetHandler.Bind(this);
		}

		m_aPendingChoiceLabels.Clear();
		m_aPendingChoiceArguments.Clear();
		AppendMapTargetGarrisonCountChoice(1);
		AppendMapTargetGarrisonCountChoice(2);
		AppendMapTargetGarrisonCountChoice(4);
		AppendMapTargetGarrisonCountChoice(6);
		AppendMapTargetGarrisonCountChoice(8);
		AppendMapTargetGarrisonCountChoice(12);

		HST_ActionChoiceDialogData data = new HST_ActionChoiceDialogData();
		data.m_sOwner = ACTION_DIALOG_OWNER;
		data.m_sDebugOwner = "command_map_target_garrison_count";
		data.m_iCancelWidgetId = ACTION_MODAL_CANCEL_WIDGET_ID;
		data.m_iChoiceWidgetIdBase = ACTION_CHOICE_WIDGET_ID_BASE;
		data.m_sTitle = "Garrison FIA Count";
		string locationLabel = ResolveMapTargetLocationLabelForConfirm();
		if (locationLabel.IsEmpty())
			locationLabel = string.Format("X %1 Z %2", Math.Round(m_vMapTargetPosition[0]), Math.Round(m_vMapTargetPosition[2]));
		data.m_sMessage = "Select how many FIA to garrison at " + locationLabel + ".";
		data.m_sCancelLabel = "Choose Again";
		foreach (string choiceLabel : m_aPendingChoiceLabels)
			data.m_aChoiceLabels.Insert(choiceLabel);

		Widget root = HST_ActionChoiceDialogController.Render(workspace, data, m_WidgetHandler);
		if (!root)
		{
			m_sMapTargetStatusText = "Garrison count dialog unavailable. Select another target or close the map.";
			SetCommandMapLocationSelectionEnabled(true);
			RefreshCommandMapTargetPrompt();
			return;
		}

		m_wActionDialogRoot = root;
		m_bActionDialogOpen = true;
		m_bMapTargetConfirmOpen = true;
		m_sPendingActionLabel = "Select garrison count";
		m_sPendingActionCommand = "map_target_garrison_count";
		m_sPendingActionArgument = "";
		ApplyCommandMapDialogState();
		RefreshCommandMapTargetCursor();
		HST_UIDebug.LogPopulation("command_map_target_garrison_count", string.Format("target=%1 choices=%2", m_vMapTargetPosition, m_aPendingChoiceLabels.Count()));
	}

	protected void AppendMapTargetGarrisonCountChoice(int count)
	{
		if (count <= 0)
			return;
		if (m_aPendingChoiceArguments.Count() >= HST_ActionChoiceDialogController.MAX_CHOICES)
			return;

		m_aPendingChoiceLabels.Insert(string.Format("%1 FIA", count));
		m_aPendingChoiceArguments.Insert(count.ToString());
	}

	protected void ShowCommandMapTargetConfirmDialog()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			string fallbackLabel = m_sMapTargetLabel;
			string fallbackCommand = m_sMapTargetCommand;
			string fallbackArgument = BuildCommandMapTargetArgument(m_vMapTargetPosition);
			CloseCommandMapTargetSelection("confirm fallback", true);
			RequestConfirmedAction(fallbackLabel, fallbackCommand, fallbackArgument);
			return;
		}

		ClearActionDialog();
		if (!m_WidgetHandler)
		{
			m_WidgetHandler = new HST_CommandMenuWidgetHandler();
			m_WidgetHandler.Bind(this);
		}

		HST_ActionDialogData data = new HST_ActionDialogData();
		data.m_sOwner = ACTION_DIALOG_OWNER;
		data.m_sDebugOwner = "command_map_target_confirm";
		data.m_iCancelWidgetId = ACTION_MODAL_CANCEL_WIDGET_ID;
		data.m_iConfirmWidgetId = ACTION_MODAL_CONFIRM_WIDGET_ID;
		data.m_sTitle = "Confirm Map Target";
		data.m_sMessage = BuildCommandMapTargetConfirmMessage();
		data.m_sCancelLabel = "Choose Again";
		data.m_sConfirmLabel = "Confirm";

		Widget root = HST_ActionDialogController.Render(workspace, data, m_WidgetHandler);
		if (!root)
		{
			m_sMapTargetStatusText = "Confirmation dialog unavailable. Select another target or close the map.";
			SetCommandMapLocationSelectionEnabled(true);
			RefreshCommandMapTargetPrompt();
			return;
		}

		m_wActionDialogRoot = root;
		m_bActionDialogOpen = true;
		m_bMapTargetConfirmOpen = true;
		m_sPendingActionLabel = m_sMapTargetLabel;
		m_sPendingActionCommand = m_sMapTargetCommand;
		m_sPendingActionArgument = BuildCommandMapTargetArgument(m_vMapTargetPosition);
		ApplyCommandMapDialogState();
		RefreshCommandMapTargetCursor();
		HST_UIDebug.LogPopulation("command_map_target_confirm", string.Format("label=%1 command=%2 target=%3 argument=%4", ShortenText(m_sMapTargetLabel, 64), m_sMapTargetCommand, m_vMapTargetPosition, m_sPendingActionArgument));
	}

	protected string BuildCommandMapTargetArgument(vector targetPosition)
	{
		if (m_sMapTargetCommand == "recruit_zone")
		{
			int count = m_iMapTargetGarrisonCount;
			if (count <= 0)
				count = 2;
			return string.Format("map_target:%1:%2:count=%3", targetPosition[0], targetPosition[2], count);
		}

		return string.Format("map_target:%1:%2", targetPosition[0], targetPosition[2]);
	}

	protected string BuildCommandMapTargetConfirmMessage()
	{
		string actionText = BuildActionConfirmMessage(m_sMapTargetLabel, m_sMapTargetCommand, m_sMapTargetArgument);
		string message = string.Format("%1\n\nTarget: X %2 Z %3", actionText, Math.Round(m_vMapTargetPosition[0]), Math.Round(m_vMapTargetPosition[2]));
		string locationLabel = ResolveMapTargetLocationLabelForConfirm();
		if (!locationLabel.IsEmpty())
			message = message + "\nLocation: " + locationLabel;
		if (m_sMapTargetCommand == "recruit_zone")
		{
			int count = m_iMapTargetGarrisonCount;
			if (count <= 0)
				count = 2;
			message = message + string.Format("\nGarrison: %1 FIA", count);
		}

		return message;
	}

	protected string ResolveMapTargetLocationLabelForConfirm()
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
			return "";

		return coordinator.ResolveCommandMapTargetZoneLabel(m_sMapTargetCommand, m_vMapTargetPosition);
	}

	protected void SetCommandMapLocationSelectionEnabled(bool enabled)
	{
		if (!m_MapTargetEntity || !m_MapTargetEntity.IsOpen())
		{
			m_bMapTargetLocationSelectionEnabled = false;
			return;
		}

		SCR_MapCursorModule cursorModule = SCR_MapCursorModule.Cast(m_MapTargetEntity.GetMapModule(SCR_MapCursorModule));
		if (!cursorModule)
			return;

		if (m_bMapTargetLocationSelectionEnabled == enabled)
			return;

		cursorModule.ToggleLocationSelection(enabled);
		m_bMapTargetLocationSelectionEnabled = enabled;
	}

	protected void ApplyCommandMapDialogState()
	{
		bool shouldBlockMap = m_bMapTargetActive && m_bMapTargetConfirmOpen;
		if (!m_MapTargetEntity || !m_MapTargetEntity.IsOpen())
		{
			m_bMapTargetDialogCursorActive = false;
			m_bMapTargetSelectionSuppressedForModal = false;
			return;
		}

		SCR_MapCursorModule cursorModule = SCR_MapCursorModule.Cast(m_MapTargetEntity.GetMapModule(SCR_MapCursorModule));
		if (!cursorModule)
			return;

		if (shouldBlockMap)
		{
			cursorModule.ToggleLocationSelection(false);
			if (!m_bMapTargetDialogCursorActive)
			{
				cursorModule.HandleDialog(true);
				m_bMapTargetDialogCursorActive = true;
			}

			m_bMapTargetLocationSelectionEnabled = false;
			m_bMapTargetSelectionSuppressedForModal = true;
			return;
		}

		if (m_bMapTargetDialogCursorActive)
		{
			cursorModule.HandleDialog(false);
			m_bMapTargetDialogCursorActive = false;
		}

		if (!m_bMapTargetSelectionSuppressedForModal)
			return;

		m_bMapTargetSelectionSuppressedForModal = false;
	}

	protected void ReleaseCommandMapDialogState()
	{
		if (m_bMapTargetDialogCursorActive && m_MapTargetEntity && m_MapTargetEntity.IsOpen())
		{
			SCR_MapCursorModule cursorModule = SCR_MapCursorModule.Cast(m_MapTargetEntity.GetMapModule(SCR_MapCursorModule));
			if (cursorModule)
				cursorModule.HandleDialog(false);
		}

		m_bMapTargetDialogCursorActive = false;
		m_bMapTargetSelectionSuppressedForModal = false;
	}

	protected void RefreshCommandMapTargetPrompt()
	{
		if (!m_bMapTargetActive)
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		if (!m_wMapTargetPromptRoot)
		{
			m_wMapTargetPromptRoot = workspace.CreateWidgets(MAP_TARGET_PROMPT_LAYOUT, workspace);
			HST_UIDebug.LogLayoutCreate("command_map_target_prompt", MAP_TARGET_PROMPT_LAYOUT, m_wMapTargetPromptRoot, workspace);
			if (!m_wMapTargetPromptRoot)
				return;

			m_wMapTargetPromptPanel = m_wMapTargetPromptRoot.FindAnyWidget("HST_SetupPromptPanel");
			m_wMapTargetPromptRule = m_wMapTargetPromptRoot.FindAnyWidget("HST_SetupPromptRule");
			m_wMapTargetPromptText = TextWidget.Cast(m_wMapTargetPromptRoot.FindAnyWidget("HST_SetupPromptText"));
			m_wMapTargetPromptRoot.SetZOrder(MAP_TARGET_PROMPT_Z);
			if (!HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.GAMEPLAY_MAP_OVERLAY, MAP_TARGET_OVERLAY_OWNER, m_wMapTargetPromptRoot, false, false, false))
			{
				HST_UIDebug.LogLayoutRejected("command_map_target_prompt", MAP_TARGET_PROMPT_LAYOUT, m_wMapTargetPromptRoot, "UI root refused map-target overlay");
				m_wMapTargetPromptRoot.RemoveFromHierarchy();
				m_wMapTargetPromptRoot = null;
				m_wMapTargetPromptPanel = null;
				m_wMapTargetPromptRule = null;
				m_wMapTargetPromptText = null;
				return;
			}
		}

		ApplyCommandMapTargetPromptLayer();
		ApplyCommandMapTargetPromptText();
		QueueCommandMapTargetPromptRefresh();
	}

	protected void QueueCommandMapTargetPromptRefresh()
	{
		if (!m_bMapTargetActive || !m_wMapTargetPromptRoot || m_bMapTargetPromptRefreshQueued)
			return;

		m_bMapTargetPromptRefreshQueued = true;
		GetGame().GetCallqueue().CallLater(RefreshCommandMapTargetPromptAfterLayout, 50, false);
	}

	protected void RefreshCommandMapTargetPromptAfterLayout()
	{
		m_bMapTargetPromptRefreshQueued = false;
		if (!m_bMapTargetActive || !m_wMapTargetPromptRoot)
			return;

		ApplyCommandMapTargetPromptLayer();
		ApplyCommandMapTargetPromptText();
		HST_UIDebug.LogWidgetGeometryCsv("command_map_target_prompt_ready", m_wMapTargetPromptRoot, "HST_SetupPromptBannerRoot|HST_SetupPromptPanel|HST_SetupPromptRule|HST_SetupPromptText");
		HST_UIDebug.LogReadyWidgetsCsv("command_map_target_prompt_ready", m_wMapTargetPromptRoot, "HST_SetupPromptBannerRoot|HST_SetupPromptPanel|HST_SetupPromptRule|HST_SetupPromptText");
	}

	protected void ApplyCommandMapTargetPromptLayer()
	{
		if (m_wMapTargetPromptRoot)
		{
			m_wMapTargetPromptRoot.SetVisible(true);
			m_wMapTargetPromptRoot.SetZOrder(MAP_TARGET_PROMPT_Z);
		}

		if (m_wMapTargetPromptPanel)
		{
			m_wMapTargetPromptPanel.SetVisible(true);
			m_wMapTargetPromptPanel.SetZOrder(MAP_TARGET_PROMPT_Z + 1);
		}

		if (m_wMapTargetPromptRule)
		{
			m_wMapTargetPromptRule.SetVisible(true);
			m_wMapTargetPromptRule.SetZOrder(MAP_TARGET_PROMPT_Z + 2);
		}

		if (m_wMapTargetPromptText)
		{
			m_wMapTargetPromptText.SetVisible(true);
			m_wMapTargetPromptText.SetZOrder(MAP_TARGET_PROMPT_Z + 3);
		}
	}

	protected void ApplyCommandMapTargetPromptText()
	{
		if (!m_wMapTargetPromptText)
			return;

		string prompt = m_sMapTargetStatusText;
		if (prompt.IsEmpty())
			prompt = "Select a target location on the map.";
		if (!m_sMapTargetLabel.IsEmpty())
			prompt = m_sMapTargetLabel + " | " + prompt;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int screenW;
		int screenH;
		HST_UIWorkspaceMetrics.GetLayoutSize(workspace, screenW, screenH);
		float scale = HST_UIWorkspaceMetrics.GetScale(screenW, screenH, 0.70, 1.12);
		m_wMapTargetPromptText.SetText(prompt);
		m_wMapTargetPromptText.SetTextWrapping(true);
		m_wMapTargetPromptText.SetExactFontSize(HST_UIWorkspaceMetrics.ScaleFont(18, scale));
		m_wMapTargetPromptText.SetBold(true);
		m_wMapTargetPromptText.SetOutline(1, 0xDD000000);
		m_wMapTargetPromptText.SetShadow(2, 0xEE000000, 1, 1, 1);
		m_wMapTargetPromptText.SetColorInt(0xFFF2E6CA);
	}

	protected void ClearCommandMapTargetPrompt()
	{
		if (m_wMapTargetPromptRoot)
			m_wMapTargetPromptRoot.RemoveFromHierarchy();

		HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.GAMEPLAY_MAP_OVERLAY, MAP_TARGET_OVERLAY_OWNER);
		m_wMapTargetPromptRoot = null;
		m_wMapTargetPromptPanel = null;
		m_wMapTargetPromptRule = null;
		m_wMapTargetPromptText = null;
		m_bMapTargetPromptRefreshQueued = false;
	}

	protected void RefreshCommandMapTargetCursor()
	{
		if (!m_bMapTargetActive || !m_bMapTargetConfirmOpen || IsZeroVector(m_vMapTargetPosition))
		{
			ClearCommandMapTargetCursor();
			return;
		}

		if (!m_MapTargetEntity || !m_MapTargetEntity.IsOpen())
		{
			ClearCommandMapTargetCursor();
			return;
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		EnsureCommandMapTargetCursor(workspace);
		if (!m_wMapTargetCursorRoot)
			return;

		int screenX;
		int screenY;
		m_MapTargetEntity.WorldToScreen(m_vMapTargetPosition[0], m_vMapTargetPosition[2], screenX, screenY, true);
		int layoutX = HST_UIWorkspaceMetrics.RawToLayoutPx(workspace, screenX);
		int layoutY = HST_UIWorkspaceMetrics.RawToLayoutPx(workspace, screenY);
		ApplyCommandMapTargetCursorLayer(workspace, layoutX, layoutY);
	}

	protected bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01 && Math.AbsFloat(value[1]) < 0.01 && Math.AbsFloat(value[2]) < 0.01;
	}

	protected void EnsureCommandMapTargetCursor(WorkspaceWidget workspace)
	{
		if (m_wMapTargetCursorRoot)
			return;

		m_wMapTargetCursorRoot = workspace.CreateWidget(WidgetType.FrameWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS, Color.FromInt(0x00FFFFFF), HST_UIConstants.Z_MAP_TARGET_CURSOR);
		if (!m_wMapTargetCursorRoot)
			return;

		m_wMapTargetCursorRoot.SetVisible(true);
		m_wMapTargetCursorRoot.SetOpacity(1.0);
		m_wMapTargetCursorRoot.SetZOrder(HST_UIConstants.Z_MAP_TARGET_CURSOR);
		m_wMapTargetCursorRoot.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);

		m_wMapTargetCursorHorizontalShadow = CreateCommandMapTargetCursorPart(workspace, m_wMapTargetCursorRoot, 0xDD000000, HST_UIConstants.Z_MAP_TARGET_CURSOR + 1);
		m_wMapTargetCursorVerticalShadow = CreateCommandMapTargetCursorPart(workspace, m_wMapTargetCursorRoot, 0xDD000000, HST_UIConstants.Z_MAP_TARGET_CURSOR + 1);
		m_wMapTargetCursorHorizontal = CreateCommandMapTargetCursorPart(workspace, m_wMapTargetCursorRoot, 0xFFFFD365, HST_UIConstants.Z_MAP_TARGET_CURSOR + 2);
		m_wMapTargetCursorVertical = CreateCommandMapTargetCursorPart(workspace, m_wMapTargetCursorRoot, 0xFFFFD365, HST_UIConstants.Z_MAP_TARGET_CURSOR + 2);
		m_wMapTargetCursorCenter = CreateCommandMapTargetCursorPart(workspace, m_wMapTargetCursorRoot, 0xFFF2F4F0, HST_UIConstants.Z_MAP_TARGET_CURSOR + 3);
	}

	protected Widget CreateCommandMapTargetCursorPart(WorkspaceWidget workspace, Widget parent, int color, int zOrder)
	{
		if (!workspace || !parent)
			return null;

		Widget widget = workspace.CreateWidget(WidgetType.ImageWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS | WidgetFlags.STRETCH, Color.FromInt(color), zOrder, parent);
		if (!widget)
			return null;

		widget.SetVisible(true);
		widget.SetOpacity(1.0);
		widget.SetColorInt(color);
		widget.SetZOrder(zOrder);
		widget.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS | WidgetFlags.STRETCH);
		return widget;
	}

	protected void ApplyCommandMapTargetCursorLayer(WorkspaceWidget workspace, int centerX, int centerY)
	{
		if (!m_wMapTargetCursorRoot)
			return;

		int screenW;
		int screenH;
		HST_UIWorkspaceMetrics.GetLayoutSize(workspace, screenW, screenH);
		FrameSlot.SetPos(m_wMapTargetCursorRoot, 0, 0);
		FrameSlot.SetSize(m_wMapTargetCursorRoot, screenW, screenH);
		m_wMapTargetCursorRoot.SetVisible(true);
		m_wMapTargetCursorRoot.SetOpacity(1.0);
		m_wMapTargetCursorRoot.SetZOrder(HST_UIConstants.Z_MAP_TARGET_CURSOR);

		int longHalf = MAP_TARGET_CURSOR_LINE_LONG / 2;
		int shortHalf = MAP_TARGET_CURSOR_LINE_SHORT / 2;
		int centerHalf = MAP_TARGET_CURSOR_CENTER_SIZE / 2;

		ApplyCommandMapTargetCursorPart(m_wMapTargetCursorHorizontalShadow, centerX - longHalf - 1, centerY - shortHalf - 1, MAP_TARGET_CURSOR_LINE_LONG + 2, MAP_TARGET_CURSOR_LINE_SHORT + 2, HST_UIConstants.Z_MAP_TARGET_CURSOR + 1);
		ApplyCommandMapTargetCursorPart(m_wMapTargetCursorVerticalShadow, centerX - shortHalf - 1, centerY - longHalf - 1, MAP_TARGET_CURSOR_LINE_SHORT + 2, MAP_TARGET_CURSOR_LINE_LONG + 2, HST_UIConstants.Z_MAP_TARGET_CURSOR + 1);
		ApplyCommandMapTargetCursorPart(m_wMapTargetCursorHorizontal, centerX - longHalf, centerY - shortHalf, MAP_TARGET_CURSOR_LINE_LONG, MAP_TARGET_CURSOR_LINE_SHORT, HST_UIConstants.Z_MAP_TARGET_CURSOR + 2);
		ApplyCommandMapTargetCursorPart(m_wMapTargetCursorVertical, centerX - shortHalf, centerY - longHalf, MAP_TARGET_CURSOR_LINE_SHORT, MAP_TARGET_CURSOR_LINE_LONG, HST_UIConstants.Z_MAP_TARGET_CURSOR + 2);
		ApplyCommandMapTargetCursorPart(m_wMapTargetCursorCenter, centerX - centerHalf, centerY - centerHalf, MAP_TARGET_CURSOR_CENTER_SIZE, MAP_TARGET_CURSOR_CENTER_SIZE, HST_UIConstants.Z_MAP_TARGET_CURSOR + 3);
	}

	protected void ApplyCommandMapTargetCursorPart(Widget widget, int left, int top, int width, int height, int zOrder)
	{
		if (!widget)
			return;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		widget.SetVisible(true);
		widget.SetOpacity(1.0);
		widget.SetZOrder(zOrder);
	}

	protected void ClearCommandMapTargetCursor()
	{
		if (m_wMapTargetCursorRoot)
			m_wMapTargetCursorRoot.RemoveFromHierarchy();

		m_wMapTargetCursorRoot = null;
		m_wMapTargetCursorHorizontalShadow = null;
		m_wMapTargetCursorVerticalShadow = null;
		m_wMapTargetCursorHorizontal = null;
		m_wMapTargetCursorVertical = null;
		m_wMapTargetCursorCenter = null;
	}

	protected void CancelCommandMapTargetSelection(string reason)
	{
		ClearActionDialog();
		CloseCommandMapTargetSelection(reason, false);
		m_sLastActionName = "Cancelled";
		m_sLastResult = "h-istasi command | map target cancelled";
		if (!reason.IsEmpty())
			m_sLastResult = m_sLastResult + " | " + reason;
		ShowMenuHint(m_sLastResult, "h-istasi", 2.0);
		RequestSnapshot();
	}

	protected void CloseCommandMapTargetSelection(string reason, bool closeMap)
	{
		if (!m_bMapTargetActive && !m_bMapTargetNativeInvokersBound && !m_wMapTargetPromptRoot)
			return;

		m_bMapTargetCompleting = true;
		ReleaseCommandMapDialogState();
		SetCommandMapLocationSelectionEnabled(false);
		UnbindCommandMapTargetInvokers();
		ClearCommandMapTargetPrompt();
		ClearCommandMapTargetCursor();

		if (closeMap)
		{
			MenuManager menuManager = GetGame().GetMenuManager();
			if (menuManager)
				menuManager.CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
		}

		m_bMapTargetActive = false;
		m_bMapTargetConfirmOpen = false;
		m_bMapTargetLocationSelectionEnabled = false;
		m_bMapTargetDialogCursorActive = false;
		m_bMapTargetSelectionSuppressedForModal = false;
		m_bMapTargetPromptRefreshQueued = false;
		m_sMapTargetLabel = "";
		m_sMapTargetCommand = "";
		m_sMapTargetArgument = "";
		m_sMapTargetTab = "";
		m_sMapTargetStatusText = "";
		m_iMapTargetGarrisonCount = 0;
		m_vMapTargetPosition = "0 0 0";
		m_MapTargetEntity = null;
		m_bMapTargetCompleting = false;
		DebugLog("closed command map target selection via " + reason);
	}

	protected void ShowActionConfirmDialog(string label, string commandId, string argument)
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			RequestConfirmedAction(label, commandId, argument);
			return;
		}

		ClearActionDialog();
		if (!m_WidgetHandler)
		{
			m_WidgetHandler = new HST_CommandMenuWidgetHandler();
			m_WidgetHandler.Bind(this);
		}

		HST_ActionDialogData data = new HST_ActionDialogData();
		data.m_sOwner = ACTION_DIALOG_OWNER;
		data.m_sDebugOwner = "command_action_dialog";
		data.m_iCancelWidgetId = ACTION_MODAL_CANCEL_WIDGET_ID;
		data.m_iConfirmWidgetId = ACTION_MODAL_CONFIRM_WIDGET_ID;
		data.m_sTitle = "Confirm Action";
		data.m_sMessage = BuildActionConfirmMessage(label, commandId, argument);
		data.m_sCancelLabel = "Cancel";
		data.m_sConfirmLabel = "Confirm";

		Widget root = HST_ActionDialogController.Render(workspace, data, m_WidgetHandler);
		if (!root)
			return;

		m_wActionDialogRoot = root;
		m_bActionDialogOpen = true;
		m_sPendingActionLabel = label;
		m_sPendingActionCommand = commandId;
		m_sPendingActionArgument = argument;
		HST_UIDebug.LogPopulation("command_action_dialog", string.Format("label=%1 command=%2 argument=%3", ShortenText(label, 64), commandId, ShortenText(argument, 96)));
	}

	protected void ShowCommanderTransferChoiceDialog(string label, string argument)
	{
		ParseCommanderTransferChoiceArgument(argument);
		if (m_aPendingChoiceArguments.Count() == 0)
		{
			m_sLastActionName = label;
			m_sLastResult = "h-istasi command | transfer commander | no other member players";
			ShowMenuHint(m_sLastResult, "h-istasi", 2.0);
			if (m_bMenuOpen)
				RenderMenu();
			return;
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			m_sLastActionName = label;
			m_sLastResult = "h-istasi command | transfer commander | selection UI unavailable";
			RequestSnapshot();
			return;
		}

		ClearActionDialog();
		ParseCommanderTransferChoiceArgument(argument);
		if (!m_WidgetHandler)
		{
			m_WidgetHandler = new HST_CommandMenuWidgetHandler();
			m_WidgetHandler.Bind(this);
		}

		HST_ActionChoiceDialogData data = new HST_ActionChoiceDialogData();
		data.m_sOwner = ACTION_DIALOG_OWNER;
		data.m_sDebugOwner = "command_transfer_commander_dialog";
		data.m_iCancelWidgetId = ACTION_MODAL_CANCEL_WIDGET_ID;
		data.m_iChoiceWidgetIdBase = ACTION_CHOICE_WIDGET_ID_BASE;
		data.m_sTitle = "Transfer Commander";
		data.m_sMessage = "Select the member who should receive commander authority.";
		data.m_sCancelLabel = "Cancel";
		foreach (string choiceLabel : m_aPendingChoiceLabels)
			data.m_aChoiceLabels.Insert(choiceLabel);

		Widget root = HST_ActionChoiceDialogController.Render(workspace, data, m_WidgetHandler);
		if (!root)
		{
			m_aPendingChoiceLabels.Clear();
			m_aPendingChoiceArguments.Clear();
			return;
		}

		m_wActionDialogRoot = root;
		m_bActionDialogOpen = true;
		m_sPendingActionLabel = label;
		m_sPendingActionCommand = "member_promote_commander";
		m_sPendingActionArgument = "";
		HST_UIDebug.LogPopulation("command_transfer_commander_dialog", string.Format("label=%1 choices=%2 argument=%3", ShortenText(label, 64), m_aPendingChoiceLabels.Count(), ShortenText(argument, 160)));
	}

	protected void ShowSupportRecallChoiceDialog(string label, string argument)
	{
		ParseSupportRecallChoiceArgument(argument);
		if (m_aPendingChoiceArguments.Count() == 0)
		{
			m_sLastActionName = label;
			m_sLastResult = "h-istasi command | recall support | no deployed support teams";
			ShowMenuHint(m_sLastResult, "h-istasi", 2.0);
			if (m_bMenuOpen)
				RenderMenu();
			return;
		}

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			m_sLastActionName = label;
			m_sLastResult = "h-istasi command | recall support | selection UI unavailable";
			RequestSnapshot();
			return;
		}

		ClearActionDialog();
		ParseSupportRecallChoiceArgument(argument);
		if (!m_WidgetHandler)
		{
			m_WidgetHandler = new HST_CommandMenuWidgetHandler();
			m_WidgetHandler.Bind(this);
		}

		HST_ActionChoiceDialogData data = new HST_ActionChoiceDialogData();
		data.m_sOwner = ACTION_DIALOG_OWNER;
		data.m_sDebugOwner = "command_support_recall_dialog";
		data.m_iCancelWidgetId = ACTION_MODAL_CANCEL_WIDGET_ID;
		data.m_iChoiceWidgetIdBase = ACTION_CHOICE_WIDGET_ID_BASE;
		data.m_sTitle = "Recall Support Team";
		data.m_sMessage = "Select the deployed team to recall.";
		data.m_sCancelLabel = "Cancel";
		foreach (string choiceLabel : m_aPendingChoiceLabels)
			data.m_aChoiceLabels.Insert(choiceLabel);

		Widget root = HST_ActionChoiceDialogController.Render(workspace, data, m_WidgetHandler);
		if (!root)
		{
			m_aPendingChoiceLabels.Clear();
			m_aPendingChoiceArguments.Clear();
			return;
		}

		m_wActionDialogRoot = root;
		m_bActionDialogOpen = true;
		m_sPendingActionLabel = label;
		m_sPendingActionCommand = "support_recall";
		m_sPendingActionArgument = "";
		HST_UIDebug.LogPopulation("command_support_recall_dialog", string.Format("label=%1 choices=%2 argument=%3", ShortenText(label, 64), m_aPendingChoiceLabels.Count(), ShortenText(argument, 220)));
	}

	protected void ParseCommanderTransferChoiceArgument(string argument)
	{
		m_aPendingChoiceLabels.Clear();
		m_aPendingChoiceArguments.Clear();

		if (argument.IsEmpty())
			return;

		array<string> choices = {};
		argument.Split(";", choices, true);
		foreach (string choice : choices)
		{
			int separator = choice.IndexOf("~");
			if (separator <= 0)
				continue;

			string targetIdentityId = choice.Substring(0, separator);
			string targetLabel = choice.Substring(separator + 1, choice.Length() - separator - 1);
			if (targetIdentityId.IsEmpty() || targetLabel.IsEmpty())
				continue;

			m_aPendingChoiceArguments.Insert(targetIdentityId);
			m_aPendingChoiceLabels.Insert(ShortenText(targetLabel, 42));
			if (m_aPendingChoiceArguments.Count() >= HST_ActionChoiceDialogController.MAX_CHOICES)
				break;
		}
	}

	protected void ParseSupportRecallChoiceArgument(string argument)
	{
		m_aPendingChoiceLabels.Clear();
		m_aPendingChoiceArguments.Clear();

		if (argument.IsEmpty())
			return;

		array<string> choices = {};
		argument.Split(";", choices, true);
		foreach (string choice : choices)
		{
			int separator = choice.IndexOf("~");
			if (separator <= 0)
				continue;

			string requestId = choice.Substring(0, separator);
			string choiceLabel = choice.Substring(separator + 1, choice.Length() - separator - 1);
			if (requestId.IsEmpty() || choiceLabel.IsEmpty())
				continue;

			m_aPendingChoiceArguments.Insert(requestId);
			m_aPendingChoiceLabels.Insert(ShortenText(choiceLabel, 96));
			if (m_aPendingChoiceArguments.Count() >= HST_ActionChoiceDialogController.MAX_CHOICES)
				break;
		}
	}

	protected void SelectPendingActionChoice(int choiceIndex)
	{
		if (choiceIndex < 0 || choiceIndex >= m_aPendingChoiceArguments.Count())
			return;

		string commandId = m_sPendingActionCommand;
		if (commandId == "map_target_garrison_count")
		{
			string countArgument = m_aPendingChoiceArguments[choiceIndex];
			int count = countArgument.ToInt();
			if (count <= 0)
				count = 2;
			m_iMapTargetGarrisonCount = count;
			ClearActionDialog();
			m_bMapTargetConfirmOpen = false;
			m_sMapTargetStatusText = string.Format("Garrison count %1 selected. Confirm or choose again.", count);
			RefreshCommandMapTargetPrompt();
			ShowCommandMapTargetConfirmDialog();
			return;
		}

		if (commandId == "support_recall")
		{
			string recallLabel = "Recall support";
			if (choiceIndex < m_aPendingChoiceLabels.Count())
				recallLabel = recallLabel + ": " + m_aPendingChoiceLabels[choiceIndex];

			string recallArgument = m_aPendingChoiceArguments[choiceIndex];
			ClearActionDialog();
			ShowActionConfirmDialog(recallLabel, commandId, recallArgument);
			return;
		}

		string label = "Transfer commander";
		if (choiceIndex < m_aPendingChoiceLabels.Count())
			label = label + ": " + m_aPendingChoiceLabels[choiceIndex];

		string argument = m_aPendingChoiceArguments[choiceIndex];
		if (commandId.IsEmpty())
			commandId = "member_promote_commander";

		ClearActionDialog();
		RequestConfirmedAction(label, commandId, argument);
	}

	protected string BuildActionConfirmMessage(string label, string commandId, string argument)
	{
		if (commandId == "new_campaign")
			return "This will reset the h-istasi campaign state and return the campaign to initial setup. Confirm only if you intend to start over.";
		if (commandId == "admin_purge_hst_native_markers")
			return "This will remove h-istasi native map markers and rebuild marker state from campaign data.";
		if (commandId == "move_hq_here" || commandId == "move_hq")
			return "This will move the h-istasi HQ. Confirm only if the new location is intentional.";
		if (commandId == "rebuild_hq_assets")
			return "This will rebuild HQ assets around the current HQ location.";
		if (commandId == "mission_random" || commandId == "mission_zone" || commandId == "mission_category" || commandId == "debug_mission" || commandId == "debug_mission_id")
			return "This will start a mission and update campaign mission state.";
		if (commandId == "complete_mission")
			return "This will force-complete the selected mission objective.";
		if (commandId == "mission_asset_deliver" || commandId == "mission_asset_sabotage" || commandId == "mission_vehicle_capture" || commandId == "mission_captive_extract")
			return "This mission action will update the selected objective state.";
		if (commandId == "remove_garrison")
			return "This will remove FIA troops from the selected garrison.";
		if (commandId == "call_supply" || commandId == "support_qrf" || commandId == "support_fire" || commandId == "support_search" || commandId == "support_gbu" || commandId == "support_umpk" || commandId == "support_kh55")
			return "This will request campaign support at the selected target.";
		if (commandId == "cancel_support")
			return "This will cancel the active player support request.";
		if (commandId == "support_recall")
			return "This will order the selected support team to break contact and leave the area. Surviving FIA who exit are refunded as HR.";
		if (commandId == "civilian_aid")
			return "This will deliver civilian aid to the nearest eligible town.";
		if (commandId == "member_promote_commander")
			return "This will transfer commander authority to the selected member.";
		if (commandId == "admin_force_self_commander")
			return "This will make you the commander and leave the previous commander as a member.";
		if (commandId == "activate_zone" || commandId == "deactivate_zone" || commandId == "capture_zone" || commandId == "progress_zone")
			return "This debug command will directly mutate zone state.";
		if (commandId == "award_small" || commandId == "income_now" || commandId == "progress_mission" || commandId == "admin_persistence_smoke_test" || commandId.Contains("admin_phase"))
			return "This admin/debug command mutates campaign state.";
		if (commandId.Contains("force_victory"))
			return "This debug command forces the campaign victory state.";
		if (commandId.Contains("force_loss"))
			return "This debug command forces the campaign loss state.";

		return "Confirm command: " + label;
	}

	protected string BuildContextActionLabel(string commandId, string argument)
	{
		if (commandId == "move_hq_here")
			return "Move base to my position";
		if (commandId == "petros_relocate_hq")
			return "Relocate HQ";
		if (commandId == "petros_deploy_hq_here")
			return "Deploy HQ Here";
		if (commandId == "vehicle_collect_loot")
			return "Load loot to vehicle";
		if (commandId == "vehicle_unload_loot")
			return "Unload vehicle loot to arsenal";

		return commandId;
	}

	protected void CancelPendingActionDialog()
	{
		if (m_bMapTargetActive && m_bMapTargetConfirmOpen)
		{
			ClearActionDialog();
			m_bMapTargetConfirmOpen = false;
			ClearCommandMapTargetCursor();
			m_sMapTargetStatusText = "Select a target location on the map.";
			SetCommandMapLocationSelectionEnabled(true);
			ApplyCommandMapDialogState();
			RefreshCommandMapTargetPrompt();
			ShowMenuHint("Select another target location", "h-istasi", 2.0);
			return;
		}

		ClearActionDialog();
		m_sLastActionName = "Cancelled";
		m_sLastResult = "h-istasi command | cancelled";
		ShowMenuHint(m_sLastResult, "h-istasi", 2.0);
		if (m_bMenuOpen)
			RenderMenu();
	}

	protected void ConfirmPendingActionDialog()
	{
		string label = m_sPendingActionLabel;
		string commandId = m_sPendingActionCommand;
		string argument = m_sPendingActionArgument;
		bool returnToCommandMenu = m_bMapTargetActive && m_bMapTargetConfirmOpen && ShouldReturnToCommandMenuAfterMapTarget(commandId);
		string returnTab = m_sMapTargetTab;
		ClearActionDialog();
		if (m_bMapTargetActive && m_bMapTargetConfirmOpen)
		{
			m_bMapTargetConfirmOpen = false;
			CloseCommandMapTargetSelection("confirmed target", true);
			RequestConfirmedAction(label, commandId, argument);
			if (returnToCommandMenu)
			{
				if (returnTab.IsEmpty())
					returnTab = "forces";
				GetGame().GetCallqueue().CallLater(OpenMenuToTab, 250, false, returnTab, "map target confirmed");
			}
			return;
		}

		RequestConfirmedAction(label, commandId, argument);
	}

	protected void ClearActionDialog()
	{
		if (m_bActionDialogOpen)
			HST_ActionDialogController.Close(ACTION_DIALOG_OWNER);

		if (m_wActionDialogRoot)
			m_wActionDialogRoot.RemoveFromHierarchy();

		m_wActionDialogRoot = null;
		m_bActionDialogOpen = false;
		m_sPendingActionLabel = "";
		m_sPendingActionCommand = "";
		m_sPendingActionArgument = "";
		m_aPendingChoiceLabels.Clear();
		m_aPendingChoiceArguments.Clear();
	}

	protected bool RenderMenu()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
		{
			Print(BuildConsoleMenuText());
			return false;
		}

		SaveCommandMenuScrollOffsets();
		m_ContentScroll = null;
		m_ActionScroll = null;
		m_FeedScroll = null;
		if (!m_WidgetHandler)
		{
			m_WidgetHandler = new HST_CommandMenuWidgetHandler();
			m_WidgetHandler.Bind(this);
		}

		HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, "HST_CommandMenu");
		BuildResponsiveLayout(workspace);
		Widget root = EnsureMenuRoot(workspace);
		if (!root)
			return false;

		ClearDynamicMenuRows(root);
		RenderHeader(workspace, root);
		RenderNavigation(workspace, root);
		RenderStats(workspace, root);
		RenderMainSections(workspace, root);
		RenderActivityPanel(workspace, root);
		RenderActions(workspace, root);
		ApplyCommandMenuLayerOrder(root);
		HST_UIDebug.LogPopulation("command_menu", string.Format("selectedTab=%1 tabs=%2 stats=%3 sections=%4 rows=%5 contentItems=%6 actions=%7 feed=%8 status=%9", m_sSelectedTab, m_aTabIds.Count(), m_aStatLabels.Count(), m_aSectionIds.Count(), m_aRowLabels.Count(), m_aContentItemKinds.Count(), m_aActionLabels.Count(), m_aFeedLines.Count(), ShortenText(m_sStatusText, 120)));
		QueueCommandMenuPostLayoutRefresh(root);
		return true;
	}

	protected Widget EnsureMenuRoot(WorkspaceWidget workspace)
	{
		if (m_wMenuRoot && m_wMenuRoot.IsVisibleInHierarchy())
		{
			m_wMenuRoot.SetVisible(true);
			m_wMenuRoot.SetOpacity(1.0);
			m_wMenuRoot.SetZOrder(HST_UIConstants.Z_COMMAND_MENU);
			ApplyCommandMenuLayerOrder(m_wMenuRoot);
			if (!HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent", m_wMenuRoot, true, false, false))
			{
				HST_UIDebug.LogLayoutRejected("command_menu", COMMAND_MENU_LAYOUT, m_wMenuRoot, "UI root refused reused command menu");
				Print(string.Format("h-istasi menu | UI root refused reused command menu | current=%1 top=%2 owner=%3", HST_UIConstants.ModeName(HST_UIRootService.Get().GetCurrentMode()), HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode()), HST_UIRootService.Get().GetTopmostOwner()), LogLevel.WARNING);
				return null;
			}

			return m_wMenuRoot;
		}

		ClearWidgets();
		return CreateMenuRoot(workspace);
	}

	protected Widget CreateMenuRoot(WorkspaceWidget workspace)
	{
		if (!m_Layout)
			BuildResponsiveLayout(workspace);

		Widget root = workspace.CreateWidgets(COMMAND_MENU_LAYOUT, workspace);
		HST_UIDebug.LogLayoutCreate("command_menu", COMMAND_MENU_LAYOUT, root, workspace);
		if (!root)
			return null;

		HST_UIDebug.LogExpectedWidgetsCsv("command_menu", root, "HST_CommandMenuRoot|CommandSurface|Header|HeaderTitle|HeaderSubtitle|HeaderTabTitle|CloseButton|CloseLabel|NavigationPanel|TabScroll|TabItems|StatsPanel|StatLayout|MainPanel|MainScroll|MainItems|ActivityPanel|ActivityScroll|ActivityItems|ActionsPanel|ActionsScroll|ActionsItems");

		root.SetVisible(true);
		root.SetOpacity(1.0);
		root.SetZOrder(HST_UIConstants.Z_COMMAND_MENU);
		ApplyCommandMenuLayerOrder(root);
		if (!HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.COMMAND_MENU, "HST_CommandMenuComponent", root, true, false, false))
		{
			HST_UIDebug.LogLayoutRejected("command_menu", COMMAND_MENU_LAYOUT, root, "UI root refused command menu");
			Print(string.Format("h-istasi menu | UI root refused command menu | current=%1 top=%2 owner=%3", HST_UIConstants.ModeName(HST_UIRootService.Get().GetCurrentMode()), HST_UIConstants.ModeName(HST_UIRootService.Get().GetTopmostMode()), HST_UIRootService.Get().GetTopmostOwner()), LogLevel.WARNING);
			root.RemoveFromHierarchy();
			m_wMenuRoot = null;
			return null;
		}

		m_wMenuRoot = root;
		m_aWidgets.Insert(root);
		BindMenuClick(root, "CloseButton", CLOSE_WIDGET_ID);
		return root;
	}

	protected Widget FindMenuWidget(Widget root, string name)
	{
		if (!root || name.IsEmpty())
			return null;

		return root.FindAnyWidget(name);
	}

	protected TextWidget FindMenuText(Widget root, string name)
	{
		return TextWidget.Cast(FindMenuWidget(root, name));
	}

	protected void SetMenuText(Widget root, string name, string text, int color, int fontSize, bool bold, bool wrap = true)
	{
		TextWidget textWidget = FindMenuText(root, name);
		if (!textWidget)
			return;

		textWidget.SetVisible(true);
		textWidget.SetOpacity(1.0);
		textWidget.SetText(text);
		textWidget.SetTextWrapping(wrap);
		ApplyTextStyle(textWidget, fontSize, bold);
		textWidget.SetColorInt(color);
	}

	protected void SetMenuWidgetColor(Widget root, string name, int color, float opacity = 1.0)
	{
		Widget widget = FindMenuWidget(root, name);
		if (!widget)
			return;

		widget.SetColorInt(color);
		widget.SetOpacity(opacity);
		widget.SetVisible(true);
	}

	protected void BindMenuClick(Widget root, string name, int userId)
	{
		Widget widget = FindMenuWidget(root, name);
		if (!widget || userId <= 0)
			return;

		widget.SetUserID(userId);
		widget.AddHandler(m_WidgetHandler);
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

		float scale = HST_UIWorkspaceMetrics.GetScale(screenW, screenH, 0.70, 1.12);

		m_Layout.m_fScale = scale;
		m_Layout.m_bVeryCompact = screenW < 1250 || screenH < 720;

		m_Layout.m_iFontTiny = ScaleFont(10);
		m_Layout.m_iFontSmall = ScaleFont(12);
		m_Layout.m_iFontNormal = ScaleFont(14);
		m_Layout.m_iFontTitle = ScaleFont(18);
		m_Layout.m_iFontHeader = ScaleFont(30);

		if (m_iLoggedLayoutW != screenW || m_iLoggedLayoutH != screenH)
		{
			m_iLoggedLayoutW = screenW;
			m_iLoggedLayoutH = screenH;
			DebugLog(string.Format("layout workspace=%1x%2 scale=%3", screenW, screenH, scale));
		}
	}

	protected void ShowMenuHint(string text, string title, float durationSeconds)
	{
	}

	protected void RenderHeader(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		SetMenuText(root, "HeaderTitle", "h-istasi HQ", 0xFFF2D18B, m_Layout.m_iFontHeader, true, false);
		SetMenuText(root, "HeaderSubtitle", "FIA Resistance Command", 0xFFB7C7D7, m_Layout.m_iFontNormal, false, false);
		SetMenuText(root, "HeaderTabTitle", "", 0xFFECE6D2, m_Layout.m_iFontTitle, true, true);
		SetMenuText(root, "CloseLabel", "Close", 0xFFF4EBD6, m_Layout.m_iFontNormal, true, false);
		SetMenuWidgetColor(root, "CloseButton", 0xFF5F6C76, 0.96);
	}

	protected void ApplyCommandMenuLayerOrder(Widget root)
	{
		if (!root)
			return;

		root.SetVisible(true);
		root.SetOpacity(1.0);
		root.SetZOrder(HST_UIConstants.Z_COMMAND_MENU);
		SetMenuLayer(root, "ScreenDimmer", COMMAND_DIMMER_Z, true);
		SetMenuLayer(root, "CommandSurface", COMMAND_SURFACE_Z, true);
		SetMenuLayer(root, "CommandSurfaceBackground", COMMAND_SURFACE_Z, true);
		SetMenuLayer(root, "NavigationPanel", COMMAND_PANEL_Z, true);
		SetMenuLayer(root, "StatsPanel", COMMAND_PANEL_Z, true);
		SetMenuLayer(root, "MainPanel", COMMAND_PANEL_Z, true);
		SetMenuLayer(root, "ActivityPanel", COMMAND_PANEL_Z, true);
		SetMenuLayer(root, "ActivityFeedTitle", COMMAND_PANEL_Z + 1, false);
		SetMenuLayer(root, "ActivityScroll", COMMAND_PANEL_Z + 1, false);
		SetMenuLayer(root, "ActionsPanel", COMMAND_PANEL_Z, true);
		SetMenuLayer(root, "Header", COMMAND_HEADER_Z, true);
		SetMenuLayer(root, "HeaderBackground", COMMAND_HEADER_Z, true);
		SetMenuLayer(root, "HeaderTitle", COMMAND_HEADER_Z + 1, true);
		SetMenuLayer(root, "HeaderSubtitle", COMMAND_HEADER_Z + 1, true);
		SetMenuLayer(root, "HeaderTabTitle", COMMAND_HEADER_Z + 1, false);
		SetMenuLayer(root, "CloseButton", COMMAND_CLOSE_Z, true);
		SetMenuLayer(root, "CloseLabel", COMMAND_CLOSE_Z + 1, true);
	}

	protected void SetMenuLayer(Widget root, string name, int zOrder, bool visible)
	{
		Widget widget = FindMenuWidget(root, name);
		if (!widget)
			return;

		widget.SetVisible(visible);
		widget.SetZOrder(zOrder);
	}

	protected void QueueCommandMenuPostLayoutRefresh(Widget root)
	{
		if (!root)
			return;

		m_wPostLayoutRoot = root;
		if (m_bPostLayoutRefreshQueued)
			return;

		m_bPostLayoutRefreshQueued = true;
		GetGame().GetCallqueue().CallLater(RefreshCommandMenuPostLayout, 0, false);
		GetGame().GetCallqueue().CallLater(RefreshCommandMenuPostLayout, 50, false);
	}

	protected void RefreshCommandMenuPostLayout()
	{
		m_bPostLayoutRefreshQueued = false;
		Widget root = m_wPostLayoutRoot;
		if (!m_bMenuOpen || !root || !root.IsVisibleInHierarchy())
			return;

		root.SetVisible(true);
		root.SetOpacity(1.0);
		root.SetZOrder(HST_UIConstants.Z_COMMAND_MENU);
		ApplyCommandMenuLayerOrder(root);
		HST_UIDebug.LogWidgetGeometryCsv("command_menu_ready", root, "HST_CommandMenuRoot|ScreenDimmer|CommandSurface|Header|NavigationPanel|TabScroll|TabItems|StatsPanel|MainPanel|MainScroll|MainItems|ActivityPanel|ActionsPanel|ActionsScroll|ActionsItems|CloseButton|CloseLabel");
		HST_UIDebug.LogReadyWidgetsCsv("command_menu_ready", root, "HST_CommandMenuRoot|ScreenDimmer|CommandSurface|Header|NavigationPanel|TabScroll|TabItems|StatsPanel|MainPanel|MainScroll|MainItems|ActivityPanel|ActionsPanel|ActionsScroll|ActionsItems|CloseButton|CloseLabel");
		HST_UIDebug.LogNamedChildSummaryCsv("command_menu_ready", root, "TabItems|MainItems|ActionsItems", 5);
	}

	protected void RenderStats(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		for (int i = 0; i < 4; i++)
		{
			SetMenuWidgetColor(root, string.Format("Stat%1Background", i), ToneBackgroundColor(""), 0.96);
			SetMenuText(root, string.Format("Stat%1Label", i), "", 0xFFCAD2D7, m_Layout.m_iFontSmall, false, true);
			SetMenuText(root, string.Format("Stat%1Value", i), "", ToneTextColor(""), m_Layout.m_iFontNormal, true, true);
		}

		for (int i = 0; i < m_aStatLabels.Count(); i++)
		{
			if (i >= 4)
				break;

			string label = "";
			string value = "";
			string tone = "";
			label = m_aStatLabels[i];
			if (m_aStatValues.IsIndexValid(i))
				value = m_aStatValues[i];
			if (m_aStatTones.IsIndexValid(i))
				tone = m_aStatTones[i];

			SetMenuWidgetColor(root, string.Format("Stat%1Background", i), ToneBackgroundColor(tone), 0.96);
			SetMenuText(root, string.Format("Stat%1Label", i), label, 0xFFCAD2D7, m_Layout.m_iFontSmall, false, true);
			SetMenuText(root, string.Format("Stat%1Value", i), value, ToneTextColor(tone), m_Layout.m_iFontNormal, true, true);
		}
	}

	protected void RenderNavigation(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		SetMenuText(root, "NavigationTitle", "Navigation", 0xFFE6DECB, m_Layout.m_iFontTitle, true, false);

		Widget items = FindMenuWidget(root, "TabItems");
		if (!items)
			return;

		for (int i = 0; i < m_aTabLabels.Count(); i++)
		{
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

			string label = m_aTabLabels[i];
			if (focused)
				label = "> " + label;

			int color = 0xFFCBD2D6;
			if (selected)
				color = 0xFFFFD98B;

			if (!m_aTabEnabled[i])
				color = 0xFF687179;

			Widget row = workspace.CreateWidgets(COMMAND_ACTION_ROW_LAYOUT, items);
			DebugRowCreated("COMMAND_ACTION_ROW_LAYOUT tab", row);
			HST_UIDebug.LogRowSample("command_menu_tabs", COMMAND_ACTION_ROW_LAYOUT, i, string.Format("tab=%1 label=%2 selected=%3 focused=%4 enabled=%5 userId=%6 row=%7", m_aTabIds[i], ShortenText(m_aTabLabels[i], 48), selected, focused, m_aTabEnabled[i], TAB_WIDGET_ID_BASE + i, HST_UIDebug.WidgetSummary(row)));
			if (!row)
				continue;

			PrepareRowRoot(row);
			BindRowClick(row, TAB_WIDGET_ID_BASE + i);
			SetRowImageColor(row, "Background", background, opacity);
			SetRowText(row, "Label", label, color, m_Layout.m_iFontNormal, selected, true);
		}

		HST_UIDebug.LogRowSummary("command_menu_tabs", COMMAND_ACTION_ROW_LAYOUT, m_aTabLabels.Count(), string.Format("selectedTab=%1 focusedControl=%2", m_sSelectedTab, m_iSelectedControl));
	}

	protected void RenderMainSections(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		m_ContentScroll = ScrollLayoutWidget.Cast(FindMenuWidget(root, "MainScroll"));
		Widget items = FindMenuWidget(root, "MainItems");

		if (!items)
		{
			SetMenuText(root, "HeaderTabTitle", "Main content unavailable: layout missing MainItems.", 0xFFFFD166, m_Layout.m_iFontSmall, false, true);
			return;
		}

		if (m_aSectionIds.Count() == 0)
		{
			Widget row = workspace.CreateWidgets(COMMAND_DATA_ROW_COMPACT_LAYOUT, items);
			DebugRowCreated("COMMAND_DATA_ROW_COMPACT_LAYOUT", row);
			HST_UIDebug.LogRowSample("command_menu_main", COMMAND_DATA_ROW_COMPACT_LAYOUT, 0, string.Format("emptyStatus=%1 row=%2", ShortenText(m_sStatusText, 120), HST_UIDebug.WidgetSummary(row)));
			if (row)
			{
				PrepareRowRoot(row);
				SetRowText(row, "Label", "", 0xFFC4CDD3, m_Layout.m_iFontSmall, true, true);
				SetRowText(row, "Value", BuildLabeledDisplayText("Status", m_sStatusText), 0xFFE0E0E0, m_Layout.m_iFontNormal, false, true);
				SetRowImageColor(row, "Background", 0x00111111, 0.01);
			}

			HST_UIDebug.LogRowSummary("command_menu_main", COMMAND_DATA_ROW_COMPACT_LAYOUT, 1, "fallback status row");
			RestoreScrollPixels(m_ContentScroll, m_fContentScrollY);
			return;
		}

		for (int i = 0; i < m_aContentItemKinds.Count(); i++)
			AddCommandContentRow(workspace, items, i);

		HST_UIDebug.LogRowSummary("command_menu_main", COMMAND_DATA_ROW_LAYOUT, m_aContentItemKinds.Count(), string.Format("sections=%1 dataRows=%2 selectedTab=%3", m_aSectionIds.Count(), m_aRowLabels.Count(), m_sSelectedTab));
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
			string title = "";
			if (sectionIndex >= 0 && sectionIndex < m_aSectionTitles.Count())
				title = m_aSectionTitles[sectionIndex];
			HST_UIDebug.LogRowSample("command_menu_main", COMMAND_SECTION_ROW_LAYOUT, contentIndex, string.Format("kind=section sectionIndex=%1 title=%2 row=%3", sectionIndex, ShortenText(title, 64), HST_UIDebug.WidgetSummary(row)));
			if (!row)
				return;

			PrepareRowRoot(row);

			SetRowText(row, "Title", title, 0xFFEFE2C4, m_Layout.m_iFontTitle, true, true);
			SetRowImageColor(row, "Background", 0xFF263341, 0.72);
			return;
		}

		if (kind == "empty")
		{
			Widget row = workspace.CreateWidgets(COMMAND_DATA_ROW_LAYOUT, items);
			DebugRowCreated("COMMAND_DATA_ROW_LAYOUT", row);
			HST_UIDebug.LogRowSample("command_menu_main", COMMAND_DATA_ROW_LAYOUT, contentIndex, string.Format("kind=empty row=%1", HST_UIDebug.WidgetSummary(row)));
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
		HST_UIDebug.LogRowSample("command_menu_main", layout, contentIndex, string.Format("kind=data rowIndex=%1 label=%2 value=%3 tone=%4 row=%5", rowIndex, ShortenText(m_aRowLabels[rowIndex], 48), ShortenText(m_aRowValues[rowIndex], 96), m_aRowTones[rowIndex], HST_UIDebug.WidgetSummary(row)));
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

		string lastActivity = BuildResultText();
		SetMenuText(root, "ActivityTitle", "Last Activity", 0xFFEFE2C4, m_Layout.m_iFontTitle, true, false);
		SetMenuText(root, "ActivityResult", lastActivity, 0xFFD2E7B8, m_Layout.m_iFontNormal, false, true);
		SetMenuLayer(root, "ActivityFeedTitle", COMMAND_PANEL_Z + 1, false);
		SetMenuLayer(root, "ActivityScroll", COMMAND_PANEL_Z + 1, false);
		m_FeedScroll = null;
		HST_UIDebug.LogRowSample("command_menu_activity", COMMAND_FEED_ROW_LAYOUT, 0, string.Format("lastActivityOnly=true text=%1", ShortenText(lastActivity, 96)));
		HST_UIDebug.LogRowSummary("command_menu_activity", COMMAND_FEED_ROW_LAYOUT, 0, "last activity only");
	}

	protected void RenderActions(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		SetMenuText(root, "ActionsTitle", "Actions", 0xFFEFE2C4, m_Layout.m_iFontTitle, true, false);

		m_ActionScroll = ScrollLayoutWidget.Cast(FindMenuWidget(root, "ActionsScroll"));
		Widget items = FindMenuWidget(root, "ActionsItems");

		if (!items)
		{
			SetMenuText(root, "ActionsTitle", "Actions unavailable: layout missing ActionsItems.", 0xFFFFD166, m_Layout.m_iFontSmall, false, true);
			return;
		}

		if (m_aActionLabels.Count() == 0)
		{
			Widget row = workspace.CreateWidgets(COMMAND_PANEL_ACTION_ROW_LAYOUT, items);
			DebugRowCreated("COMMAND_PANEL_ACTION_ROW_LAYOUT", row);
			HST_UIDebug.LogRowSample("command_menu_actions", COMMAND_PANEL_ACTION_ROW_LAYOUT, 0, string.Format("emptyActions=true row=%1", HST_UIDebug.WidgetSummary(row)));
			if (row)
			{
				PrepareRowRoot(row);
				SetRowText(row, "Label", "No commands available.", 0xFF9AA5AD, m_Layout.m_iFontNormal, false, true);
				SetRowImageColor(row, "Background", 0x00111111, 0.01);
			}

			HST_UIDebug.LogRowSummary("command_menu_actions", COMMAND_PANEL_ACTION_ROW_LAYOUT, 1, "fallback action row");
			RestoreScrollPixels(m_ActionScroll, m_fActionScrollY);
			return;
		}

		for (int i = 0; i < m_aActionLabels.Count(); i++)
			AddCommandActionRow(workspace, items, i);

		HST_UIDebug.LogRowSummary("command_menu_actions", COMMAND_PANEL_ACTION_ROW_LAYOUT, m_aActionLabels.Count(), string.Format("selectedTab=%1 focusedControl=%2", m_sSelectedTab, m_iSelectedControl));
		RestoreScrollPixels(m_ActionScroll, m_fActionScrollY);
		EnsureSelectedActionVisible();
	}

	protected void AddCommandActionRow(WorkspaceWidget workspace, Widget items, int actionIndex)
	{
		if (!workspace || !items)
			return;

		if (actionIndex < 0 || actionIndex >= m_aActionLabels.Count())
			return;

		Widget row = workspace.CreateWidgets(COMMAND_PANEL_ACTION_ROW_LAYOUT, items);
		DebugRowCreated("COMMAND_PANEL_ACTION_ROW_LAYOUT", row);
		string debugCommandId = "";
		if (actionIndex < m_aActionCommands.Count())
			debugCommandId = m_aActionCommands[actionIndex];
		string debugArgument = "";
		if (actionIndex < m_aActionArguments.Count())
			debugArgument = m_aActionArguments[actionIndex];
		HST_UIDebug.LogRowSample("command_menu_actions", COMMAND_PANEL_ACTION_ROW_LAYOUT, actionIndex, string.Format("label=%1 command=%2 argument=%3 enabled=%4 row=%5", ShortenText(m_aActionLabels[actionIndex], 64), debugCommandId, ShortenText(debugArgument, 96), (actionIndex >= m_aActionEnabled.Count() || m_aActionEnabled[actionIndex]), HST_UIDebug.WidgetSummary(row)));
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
		int actionViewWidth = Math.Max(ScalePx(320), Math.Round(m_Layout.m_iScreenW * 0.25));
		m_ActionScroll.ScrollToView(0, y, actionViewWidth, rowHeight);
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
		m_wMenuRoot = null;
		m_wPostLayoutRoot = null;
		m_bPostLayoutRefreshQueued = false;
	}

	protected void ClearDynamicMenuRows(Widget root)
	{
		ClearMenuContainerChildren(root, "TabItems");
		ClearMenuContainerChildren(root, "MainItems");
		ClearMenuContainerChildren(root, "ActivityItems");
		ClearMenuContainerChildren(root, "ActionsItems");
	}

	protected void ClearMenuContainerChildren(Widget root, string name)
	{
		Widget container = FindMenuWidget(root, name);
		if (!container)
			return;

		while (container.GetChildren())
			container.GetChildren().RemoveFromHierarchy();
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
		if (!m_bCustomBindingReady)
			return "I key binding pending";

		string lastAction = m_sLastActionName;
		if (lastAction.IsEmpty())
			lastAction = "-";

		return lastAction;
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

		int localPlayerId = ResolveNativeLocalPlayerId();
		if (localPlayerId > 0)
			return localPlayerId;

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

		IEntity localControlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (localControlledEntity && owner == localControlledEntity)
			return true;

		PlayerController localController = GetGame().GetPlayerController();
		PlayerController ownerController = PlayerController.Cast(owner);
		if (localController)
		{
			if (ownerController && ownerController == localController)
				return true;

			HST_CommandMenuComponent localComponent = HST_CommandMenuComponent.Cast(localController.FindComponent(HST_CommandMenuComponent));
			if (localComponent == this)
				return true;
		}

		int localPlayerId = ResolveNativeLocalPlayerId();
		if (localPlayerId <= 0)
			return false;

		if (ownerController && ownerController.GetPlayerId() == localPlayerId)
			return true;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return false;

		int ownerPlayerId = playerManager.GetPlayerIdFromControlledEntity(owner);
		if (ownerPlayerId == localPlayerId)
			return true;

		BaseRplComponent rpl = BaseRplComponent.Cast(owner.FindComponent(BaseRplComponent));
		return rpl && playerManager.GetPlayerIdFromEntityRplId(rpl.Id()) == localPlayerId;
	}

	protected int ResolveNativeLocalPlayerId()
	{
		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId > 0)
			return localPlayerId;

		PlayerController localController = GetGame().GetPlayerController();
		if (localController && localController.GetPlayerId() > 0)
			return localController.GetPlayerId();

		IEntity controlledEntity = SCR_PlayerController.GetLocalControlledEntity();
		if (!controlledEntity)
			return 0;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		return playerManager.GetPlayerIdFromControlledEntity(controlledEntity);
	}

	protected int ResolveOwnerPlayerId(IEntity owner)
	{
		if (!owner)
			return 0;

		PlayerController controller = PlayerController.Cast(owner);
		if (controller)
			return controller.GetPlayerId();

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		int ownerPlayerId = playerManager.GetPlayerIdFromControlledEntity(owner);
		if (ownerPlayerId > 0)
			return ownerPlayerId;

		BaseRplComponent rpl = BaseRplComponent.Cast(owner.FindComponent(BaseRplComponent));
		if (rpl)
			return playerManager.GetPlayerIdFromEntityRplId(rpl.Id());

		return 0;
	}
}
