class HST_UIScreenBase
{
	HST_EUIScreenMode m_eMode = HST_EUIScreenMode.NONE;
	string m_sOwner;
	Widget m_wRoot;
	bool m_bBlocksGameplay;
	bool m_bBlocksMap;
	bool m_bModal;

	void Configure(HST_EUIScreenMode mode, string owner, Widget root, bool blocksGameplay, bool blocksMap, bool modal)
	{
		m_eMode = mode;
		m_sOwner = owner;
		m_wRoot = root;
		m_bBlocksGameplay = blocksGameplay;
		m_bBlocksMap = blocksMap;
		m_bModal = modal;
	}

	bool Matches(HST_EUIScreenMode mode, string owner = "")
	{
		if (m_eMode != mode)
			return false;
		if (!owner.IsEmpty() && m_sOwner != owner)
			return false;

		return true;
	}

	bool MatchesExact(HST_EUIScreenMode mode, string owner, Widget root, bool blocksGameplay, bool blocksMap, bool modal)
	{
		if (!Matches(mode, owner))
			return false;

		return m_wRoot == root && m_bBlocksGameplay == blocksGameplay && m_bBlocksMap == blocksMap && m_bModal == modal;
	}

	bool IsBlocking()
	{
		return m_bBlocksGameplay || m_bBlocksMap || m_bModal;
	}
}
