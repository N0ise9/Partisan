class HST_PlayerMapMarkerDynamicWComponent : SCR_MapMarkerDynamicWComponent
{
	static const int FACING_UPDATE_INTERVAL_MS = 100;
	static const float FACING_ROTATION_EPSILON = 0.1;
	static const float WHISPER_ICON_FORWARD_OFFSET_DEGREES = -50.0;

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

		vector yawPitchRoll = playerEntity.GetYawPitchRoll();
		float rotation = NormalizeMarkerRotation(yawPitchRoll[0] + WHISPER_ICON_FORWARD_OFFSET_DEGREES);
		if (Math.AbsFloat(rotation - m_fLastFacingRotation) < FACING_ROTATION_EPSILON)
			return;

		m_fLastFacingRotation = rotation;
		m_wMarkerIcon.SetRotation(rotation);
	}

	void SetLabelColor(int color)
	{
		if (m_wMarkerText)
			m_wMarkerText.SetColorInt(color);
	}

	void ForceVisibleStyle()
	{
		if (m_wRoot)
		{
			m_wRoot.SetVisible(true);
			m_wRoot.SetOpacity(1.0);
			m_wRoot.SetZOrder(80);
		}

		if (m_wMarkerIcon)
		{
			m_wMarkerIcon.SetVisible(true);
			m_wMarkerIcon.SetOpacity(1.0);
		}

		if (m_wMarkerText)
		{
			m_wMarkerText.SetVisible(true);
			m_wMarkerText.SetOpacity(1.0);
		}
	}

	protected float NormalizeMarkerRotation(float rotation)
	{
		while (rotation < 0.0)
			rotation = rotation + 360.0;
		while (rotation >= 360.0)
			rotation = rotation - 360.0;

		return rotation;
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
