[BaseContainerProps(configRoot: true)]
modded class SCR_EditorManagerCore
{
	protected bool m_bHSTApplyDeferredPlayerRoleChange;

	override protected void OnPlayerRoleChange(int playerId, EPlayerRole roleFlags)
	{
		if (m_bHSTApplyDeferredPlayerRoleChange)
		{
			m_bHSTApplyDeferredPlayerRoleChange = false;
			super.OnPlayerRoleChange(playerId, roleFlags);
			return;
		}

		if (!GetGame() || !GetGame().GetCallqueue())
		{
			super.OnPlayerRoleChange(playerId, roleFlags);
			return;
		}

		// Role-provided editor modes can change GAME_MASTER while the game mode's
		// player-role ScriptInvoker is still active. Apply this listener on the
		// next frame so stock mode and role synchronization cannot re-enter it.
		GetGame().GetCallqueue().Call(HST_ApplyDeferredPlayerRoleChange, playerId, roleFlags);
	}

	void HST_ApplyDeferredPlayerRoleChange(int playerId, EPlayerRole roleFlags)
	{
		m_bHSTApplyDeferredPlayerRoleChange = true;
		OnPlayerRoleChange(playerId, roleFlags);
	}
}
