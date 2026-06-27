class HST_UIRootService
{
	static const bool UI_ROOT_DEBUG_ENABLED = true;

	protected static ref HST_UIRootService s_Instance;

	protected ref HST_UIScreenBase m_CurrentScreen;
	protected ref HST_UIScreenBase m_ModalScreen;
	protected int m_iRevision;
	protected int m_iNotificationDepth;

	static HST_UIRootService Get()
	{
		if (!s_Instance)
			s_Instance = new HST_UIRootService();

		return s_Instance;
	}

	bool RequestOpen(HST_EUIScreenMode mode, string owner, Widget root = null, bool blocksGameplay = false, bool blocksMap = false, bool modal = false)
	{
		if (mode == HST_EUIScreenMode.NONE)
			return false;

		bool wantsModal = modal || mode == HST_EUIScreenMode.ACTION_DIALOG || mode == HST_EUIScreenMode.MISSION_DIALOG;
		if (!CanOpen(mode, owner, wantsModal))
		{
			DebugState("refused open", mode, owner, root, blocksGameplay, blocksMap, wantsModal, true);
			return false;
		}

		if (wantsModal)
		{
			if (m_ModalScreen && m_ModalScreen.MatchesExact(mode, owner, root, blocksGameplay, blocksMap, true))
				return true;

			if (!m_ModalScreen)
				m_ModalScreen = new HST_UIScreenBase();

			m_ModalScreen.Configure(mode, owner, root, blocksGameplay, blocksMap, true);
			m_iRevision++;
			DebugState("opened modal", mode, owner, root, blocksGameplay, blocksMap, true, false);
			return true;
		}

		if (!m_CurrentScreen)
			m_CurrentScreen = new HST_UIScreenBase();

		if (m_CurrentScreen.MatchesExact(mode, owner, root, blocksGameplay, blocksMap, false))
			return true;

		m_CurrentScreen.Configure(mode, owner, root, blocksGameplay, blocksMap, false);
		m_iRevision++;
		DebugState("opened screen", mode, owner, root, blocksGameplay, blocksMap, false, false);
		return true;
	}

	void NotifyClosed(HST_EUIScreenMode mode, string owner = "")
	{
		if (m_ModalScreen && m_ModalScreen.Matches(mode, owner))
		{
			Widget root = m_ModalScreen.m_wRoot;
			m_ModalScreen = null;
			m_iRevision++;
			DebugState("closed modal", mode, owner, root, false, false, true, false);
			return;
		}

		if (!m_CurrentScreen || !m_CurrentScreen.Matches(mode, owner))
		{
			DebugState("ignored close", mode, owner, null, false, false, false, false);
			return;
		}

		Widget root = m_CurrentScreen.m_wRoot;
		m_CurrentScreen = null;
		m_iRevision++;
		DebugState("closed screen", mode, owner, root, false, false, false, false);
	}

	void NotifyNotificationShown()
	{
		m_iNotificationDepth++;
		DebugState("notification shown", HST_EUIScreenMode.NONE, "notification", null, false, false, false, false);
	}

	void NotifyNotificationHidden()
	{
		if (m_iNotificationDepth > 0)
			m_iNotificationDepth--;

		DebugState("notification hidden", HST_EUIScreenMode.NONE, "notification", null, false, false, false, false);
	}

	bool CanOpen(HST_EUIScreenMode mode, string owner = "", bool modal = false)
	{
		bool wantsModal = modal || mode == HST_EUIScreenMode.ACTION_DIALOG || mode == HST_EUIScreenMode.MISSION_DIALOG;
		if (m_ModalScreen)
		{
			if (wantsModal)
				return m_ModalScreen.Matches(mode, owner);

			if (m_CurrentScreen && m_CurrentScreen.Matches(mode, owner))
				return true;

			return false;
		}

		HST_EUIScreenMode current = GetCurrentMode();
		if (wantsModal)
		{
			if (mode == HST_EUIScreenMode.SETUP_MAP)
				return current == HST_EUIScreenMode.SETUP_MAP;

			return current != HST_EUIScreenMode.SETUP_MAP;
		}

		if (current == HST_EUIScreenMode.NONE)
			return true;

		if (current == mode)
			return owner.IsEmpty() || (m_CurrentScreen && m_CurrentScreen.Matches(mode, owner));

		if (current == HST_EUIScreenMode.SETUP_MAP)
			return false;

		if (mode == HST_EUIScreenMode.COMMAND_MENU && current == HST_EUIScreenMode.LOADOUT_EDITOR)
			return false;

		if (mode == HST_EUIScreenMode.LOADOUT_EDITOR && current == HST_EUIScreenMode.COMMAND_MENU)
			return true;

		return true;
	}

	HST_EUIScreenMode GetCurrentMode()
	{
		if (!m_CurrentScreen)
			return HST_EUIScreenMode.NONE;

		return m_CurrentScreen.m_eMode;
	}

	HST_EUIScreenMode GetModalMode()
	{
		if (!m_ModalScreen)
			return HST_EUIScreenMode.NONE;

		return m_ModalScreen.m_eMode;
	}

	HST_EUIScreenMode GetTopmostMode()
	{
		if (m_ModalScreen)
			return m_ModalScreen.m_eMode;

		return GetCurrentMode();
	}

	string GetTopmostOwner()
	{
		if (m_ModalScreen)
			return m_ModalScreen.m_sOwner;

		if (m_CurrentScreen)
			return m_CurrentScreen.m_sOwner;

		return "";
	}

	bool IsTopmost(HST_EUIScreenMode mode, string owner = "")
	{
		if (m_ModalScreen)
			return m_ModalScreen.Matches(mode, owner);

		if (!m_CurrentScreen)
			return mode == HST_EUIScreenMode.NONE;

		return m_CurrentScreen.Matches(mode, owner);
	}

	bool CanHandleScreenInput(HST_EUIScreenMode mode, string owner = "")
	{
		if (m_ModalScreen)
			return false;

		if (!m_CurrentScreen)
			return mode == HST_EUIScreenMode.NONE;

		return m_CurrentScreen.Matches(mode, owner);
	}

	bool CanHandleModalInput(HST_EUIScreenMode mode, string owner = "")
	{
		if (!m_ModalScreen)
			return false;

		return m_ModalScreen.Matches(mode, owner);
	}

	bool IsGameplayBlocked()
	{
		if (m_CurrentScreen && m_CurrentScreen.m_bBlocksGameplay)
			return true;

		return m_ModalScreen && m_ModalScreen.m_bBlocksGameplay;
	}

	bool IsMapBlocked()
	{
		if (m_CurrentScreen && m_CurrentScreen.m_bBlocksMap)
			return true;

		return m_ModalScreen && m_ModalScreen.m_bBlocksMap;
	}

	bool IsModalOpen()
	{
		return m_ModalScreen != null;
	}

	bool IsNotificationVisible()
	{
		return m_iNotificationDepth > 0;
	}

	int GetRevision()
	{
		return m_iRevision;
	}

	protected void DebugState(string eventName, HST_EUIScreenMode mode, string owner, Widget root, bool blocksGameplay, bool blocksMap, bool modal, bool warning)
	{
		if (!UI_ROOT_DEBUG_ENABLED)
			return;

		string request = string.Format("event=%1 request=%2 owner=%3 modal=%4 blocksGameplay=%5 blocksMap=%6", eventName, HST_UIConstants.ModeName(mode), owner, modal, blocksGameplay, blocksMap);
		string state = string.Format("current=%1 modalScreen=%2 topmost=%3 topOwner=%4 notifications=%5 revision=%6", DescribeScreen(m_CurrentScreen), DescribeScreen(m_ModalScreen), HST_UIConstants.ModeName(GetTopmostMode()), GetTopmostOwner(), m_iNotificationDepth, m_iRevision);
		string rootSummary = "root=" + HST_UIDebug.WidgetSummary(root);
		string message = string.Format("h-istasi ui root | %1 | %2 | %3", request, state, rootSummary);
		if (warning)
			Print(message, LogLevel.WARNING);
		else
			Print(message);
	}

	protected string DescribeScreen(HST_UIScreenBase screen)
	{
		if (!screen)
			return "none";

		return string.Format("%1:%2 gameplay=%3 map=%4 modal=%5 root=%6", HST_UIConstants.ModeName(screen.m_eMode), screen.m_sOwner, screen.m_bBlocksGameplay, screen.m_bBlocksMap, screen.m_bModal, HST_UIDebug.WidgetSummary(screen.m_wRoot));
	}
}
