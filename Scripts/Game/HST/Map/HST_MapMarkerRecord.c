enum HST_EMapMarkerRenderMode
{
	STATIC_WORLD,
	DYNAMIC_ENTITY,
	SIMULATED_TRACKED,
	LOCAL_PREVIEW,
	AREA_OVERLAY
}

class HST_MapMarkerRecord
{
	string m_sId;
	HST_EMapMarkerRenderMode m_eRenderMode = HST_EMapMarkerRenderMode.STATIC_WORLD;

	vector m_vWorldPosition;
	string m_sTargetRuntimeId;
	IEntity m_TargetEntity;

	string m_sLabel;
	string m_sShortLabel;
	string m_sCategory;
	string m_sFactionKey;
	string m_sTone;
	int m_iPriority;

	SCR_EMapMarkerType m_eMarkerType = SCR_EMapMarkerType.PLACED_CUSTOM;
	int m_iConfigId = -1;
	int m_iIconEntry;
	int m_iColorEntry;
	int m_iFactionFlags;

	bool m_bVisible = true;
	bool m_bCanPlayerRemove;
	bool m_bLocalOnly;
	bool m_bServerMarker = true;

	int m_iRevision;
	int m_iLastChangedSecond;

	string BuildNativeSignature()
	{
		int x = Math.Round(m_vWorldPosition[0]);
		int z = Math.Round(m_vWorldPosition[2]);
		if (m_eRenderMode == HST_EMapMarkerRenderMode.SIMULATED_TRACKED)
		{
			x = Math.Round(m_vWorldPosition[0] / 25.0) * 25;
			z = Math.Round(m_vWorldPosition[2] / 25.0) * 25;
		}

		string signature = string.Format("%1|%2|%3|%4|%5|%6|%7|%8|%9",
			m_eRenderMode,
			x,
			z,
			m_sShortLabel,
			m_sCategory,
			m_sFactionKey,
			m_eMarkerType,
			m_iConfigId,
			m_iIconEntry);
		return signature + string.Format("|%1|%2|%3|%4",
			m_iColorEntry,
			m_iFactionFlags,
			m_bLocalOnly,
			m_bServerMarker);
	}
}

class HST_NativeStaticMarkerHandle
{
	string m_sDomainId;
	int m_iNativeMarkerId = -1;
	ref SCR_MapMarkerBase m_Marker;
	string m_sRevisionSignature;
}

class HST_MapMarkerReconcileResult
{
	int m_iDesired;
	int m_iPublishedStatic;
	int m_iPublishedDynamic;
	int m_iCreated;
	int m_iUpdated;
	int m_iRemoved;
	int m_iUnchanged;
	int m_iFailed;
	bool m_bChanged;

	void Reset()
	{
		m_iDesired = 0;
		m_iPublishedStatic = 0;
		m_iPublishedDynamic = 0;
		m_iCreated = 0;
		m_iUpdated = 0;
		m_iRemoved = 0;
		m_iUnchanged = 0;
		m_iFailed = 0;
		m_bChanged = false;
	}
}
