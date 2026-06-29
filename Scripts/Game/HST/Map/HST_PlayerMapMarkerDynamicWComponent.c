class HST_PlayerMapMarkerDynamicWComponent : SCR_MapMarkerDynamicWComponent
{
	static const int FACING_UPDATE_INTERVAL_MS = 100;
	static const float FACING_ROTATION_EPSILON = 0.1;

	protected int m_iPlayerId;
	protected bool m_bFacingUpdateActive;
	protected float m_fLastFacingRotation = -9999.0;

	void TrackPlayerFacing(notnull SCR_MapMarkerEntity marker)
	{
		m_iPlayerId = marker.GetMarkerConfigID();
		UpdateFacingRotation();
		StartFacingUpdates();
	}

	protected void StartFacingUpdates()
	{
		if (m_bFacingUpdateActive)
			return;

		m_bFacingUpdateActive = true;
		GetGame().GetCallqueue().CallLater(UpdateFacingRotation, FACING_UPDATE_INTERVAL_MS, true);
	}

	protected void StopFacingUpdates()
	{
		if (!m_bFacingUpdateActive)
			return;

		GetGame().GetCallqueue().Remove(UpdateFacingRotation);
		m_bFacingUpdateActive = false;
	}

	protected void UpdateFacingRotation()
	{
		if (!m_wMarkerIcon)
			return;

		IEntity playerEntity = ResolvePlayerEntity();
		if (!playerEntity)
			return;

		vector angles = playerEntity.GetAngles();
		float rotation = angles[1];
		if (Math.AbsFloat(rotation - m_fLastFacingRotation) < FACING_ROTATION_EPSILON)
			return;

		m_fLastFacingRotation = rotation;
		m_wMarkerIcon.SetRotation(rotation);
	}

	protected IEntity ResolvePlayerEntity()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager && m_iPlayerId > 0)
		{
			IEntity playerEntity = playerManager.GetPlayerControlledEntity(m_iPlayerId);
			if (playerEntity)
				return playerEntity;
		}

		if (m_MarkerEnt)
			return m_MarkerEnt.GetTarget();

		return null;
	}

	override void HandlerDeattached(Widget w)
	{
		StopFacingUpdates();
		super.HandlerDeattached(w);
	}
}
