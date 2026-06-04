class HST_MapMarkerService
{
	static const string NATIVE_MARKER_MANAGER_COMPONENT = "SCR_MapMarkerManagerComponent";
	static const string NATIVE_MARKER_CONFIG = "{3583D42139D9A10B}Configs/Map/CampaignMapMarkerConfig.conf";
	static const string NATIVE_DOT_MARKER_ENTITY = "SCR_MapMarkerDotCircle";
	static const string NATIVE_DOT_MARKER_PREFAB = "{9B40AE75EA95FBEC}Prefabs/Markers/MapMarkerDotCircle.et";

	bool RebuildAllMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return false;

		state.m_aMapMarkers.Clear();
		AddHQMarker(state, preset);
		AddHideoutMarkers(state, preset);
		AddZoneMarkers(state, preset);
		AddMissionMarkers(state, preset);
		AddQRFMarkers(state, preset);
		Print(string.Format("h-istasi | rebuilt %1 native map marker record(s)", state.m_aMapMarkers.Count()));
		return true;
	}

	bool RefreshHQMarker(HST_CampaignState state, HST_CampaignPreset preset)
	{
		return RebuildAllMarkers(state, preset);
	}

	bool RefreshZoneMarker(HST_CampaignState state, HST_CampaignPreset preset, string zoneId)
	{
		return RebuildAllMarkers(state, preset);
	}

	bool RefreshMissionMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		return RebuildAllMarkers(state, preset);
	}

	void CleanupMarkers(HST_CampaignState state)
	{
		if (!state)
			return;

		state.m_aMapMarkers.Clear();
	}

	string BuildMarkerReport(HST_CampaignState state)
	{
		if (!state)
			return "h-istasi map markers | campaign state not ready";

		int hqCount;
		int townCount;
		int strategicCount;
		int missionCount;
		int qrfCount;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (!marker || !marker.m_bVisible)
				continue;

			if (marker.m_sCategory == "hq" || marker.m_sCategory == "hideout")
				hqCount++;
			else if (marker.m_sCategory == "town")
				townCount++;
			else if (marker.m_sCategory == "mission")
				missionCount++;
			else if (marker.m_sCategory == "qrf")
				qrfCount++;
			else
				strategicCount++;
		}

		string summary = string.Format("h-istasi map markers | total %1 | HQ/hideouts %2 | towns %3", state.m_aMapMarkers.Count(), hqCount, townCount);
		string tactical = string.Format(" | strategic %1 | missions %2 | QRFs %3 | native manager %4", strategicCount, missionCount, qrfCount, NATIVE_MARKER_MANAGER_COMPONENT);
		return summary + tactical;
	}

	protected void AddHQMarker(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state.m_bHQDeployed)
			return;

		AddMarker(state, "hst_hq", state.m_sHQHideoutId, "FIA HQ", "hq", preset.m_sResistanceFactionKey, "PICK_UP2", "BLUFOR", state.m_vHQPosition, true);
	}

	protected void AddHideoutMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_HideoutDefinition hideout : HST_DefaultCatalog.CreateHideouts())
		{
			if (!hideout || hideout.m_sHideoutId == state.m_sHQHideoutId)
				continue;

			AddMarker(state, "hst_hideout_" + hideout.m_sHideoutId, hideout.m_sHideoutId, hideout.m_sDisplayName + " Hideout", "hideout", preset.m_sResistanceFactionKey, "PICK_UP2", "BLUFOR", hideout.m_vPosition, true);
		}
	}

	protected void AddZoneMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			string category = ZoneTypeToMarkerCategory(zone.m_eType);
			string label = ResolveZoneDisplayName(zone) + " " + ZoneTypeToLabel(zone.m_eType);
			string color = FactionToMarkerColor(zone.m_sOwnerFactionKey, preset);
			string icon = ZoneTypeToMarkerIcon(zone.m_eType);
			AddMarker(state, "hst_zone_" + zone.m_sZoneId, zone.m_sZoneId, label, category, zone.m_sOwnerFactionKey, icon, color, zone.m_vPosition, true);
		}
	}

	protected void AddMissionMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			vector markerPosition = state.m_vHQPosition;
			if (!mission.m_sTargetZoneId.IsEmpty())
			{
				HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
				if (zone)
					markerPosition = zone.m_vPosition;
			}

			AddMarker(state, "hst_mission_" + mission.m_sInstanceId, mission.m_sInstanceId, "Mission " + mission.m_sMissionId, "mission", preset.m_sResistanceFactionKey, "OBJECTIVE_MARKER", "REFORGER_ORANGE", markerPosition, true);
		}
	}

	protected void AddQRFMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (!qrf || qrf.m_bResolved)
				continue;

			HST_ZoneState targetZone = state.FindZone(qrf.m_sTargetZoneId);
			if (!targetZone)
				continue;

			AddMarker(state, "hst_qrf_" + qrf.m_sInstanceId, qrf.m_sInstanceId, "Enemy QRF " + ResolveZoneDisplayName(targetZone), "qrf", qrf.m_sFactionKey, "OBJECTIVE_MARKER", FactionToMarkerColor(qrf.m_sFactionKey, preset), targetZone.m_vPosition, true);
		}
	}

	protected void AddMarker(HST_CampaignState state, string markerId, string linkedId, string label, string category, string ownerFactionKey, string iconHint, string colorHint, vector position, bool visible)
	{
		HST_MapMarkerState marker = new HST_MapMarkerState();
		marker.m_sMarkerId = markerId;
		marker.m_sLinkedId = linkedId;
		marker.m_sLabel = label;
		marker.m_sCategory = category;
		marker.m_sOwnerFactionKey = ownerFactionKey;
		marker.m_sIconHint = iconHint;
		marker.m_sColorHint = colorHint;
		marker.m_vPosition = HST_WorldPositionService.ResolveGroundPosition(position, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		marker.m_bVisible = visible;
		marker.m_bRuntimeNative = true;
		state.m_aMapMarkers.Insert(marker);
	}

	protected string FactionToMarkerColor(string factionKey, HST_CampaignPreset preset)
	{
		if (preset && factionKey == preset.m_sResistanceFactionKey)
			return "BLUFOR";

		if (preset && factionKey == preset.m_sInvaderFactionKey)
			return "OPFOR";

		if (preset && factionKey == preset.m_sOccupierFactionKey)
			return "RED";

		return "REFORGER_ORANGE";
	}

	protected string ZoneTypeToMarkerCategory(HST_EZoneType zoneType)
	{
		if (zoneType == HST_EZoneType.HST_ZONE_TOWN)
			return "town";

		if (zoneType == HST_EZoneType.HST_ZONE_HIDEOUT)
			return "hideout";

		if (zoneType == HST_EZoneType.HST_ZONE_MISSION_SITE)
			return "mission";

		return "strategic";
	}

	protected string ZoneTypeToMarkerIcon(HST_EZoneType zoneType)
	{
		if (zoneType == HST_EZoneType.HST_ZONE_RESOURCE || zoneType == HST_EZoneType.HST_ZONE_FACTORY)
			return "MINE_SINGLE";

		if (zoneType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return "MARK_QUESTION";

		if (zoneType == HST_EZoneType.HST_ZONE_HIDEOUT)
			return "PICK_UP2";

		return "OBJECTIVE_MARKER";
	}

	protected string ZoneTypeToLabel(HST_EZoneType zoneType)
	{
		if (zoneType == HST_EZoneType.HST_ZONE_TOWN)
			return "Town";

		if (zoneType == HST_EZoneType.HST_ZONE_OUTPOST)
			return "Outpost";

		if (zoneType == HST_EZoneType.HST_ZONE_RESOURCE)
			return "Resource";

		if (zoneType == HST_EZoneType.HST_ZONE_FACTORY)
			return "Factory";

		if (zoneType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return "Radio";

		if (zoneType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return "Airfield";

		if (zoneType == HST_EZoneType.HST_ZONE_SEAPORT)
			return "Seaport";

		return "Zone";
	}

	protected string ResolveZoneDisplayName(HST_ZoneState zone)
	{
		if (!zone)
			return "unknown";

		if (!zone.m_sDisplayName.IsEmpty())
			return zone.m_sDisplayName;

		return HST_DefaultCatalog.GetZoneDisplayName(zone.m_sZoneId);
	}
}
