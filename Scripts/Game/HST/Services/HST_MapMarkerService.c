class HST_MapMarkerService
{
	static const string NATIVE_MARKER_MANAGER_COMPONENT = "SCR_MapMarkerManagerComponent";
	static const string NATIVE_MARKER_CONFIG = "{3583D42139D9A10B}Configs/Map/CampaignMapMarkerConfig.conf";

	protected ref array<IEntity> m_aNativeMarkerCandidates = {};
	protected ref array<ref SCR_MapMarkerBase> m_aRuntimeNativeMarkers = {};
	protected string m_sNativeMarkerEntityName;
	protected bool m_bNativePublishPending;
	protected float m_fNativePublishRetrySeconds;

	bool RebuildAllMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return false;

		ClearRuntimeNativeMarkers();
		state.m_aMapMarkers.Clear();
		AddHQMarker(state, preset);
		AddHideoutMarkers(state, preset);
		AddZoneMarkers(state, preset);
		AddMissionMarkers(state, preset);
		AddQRFMarkers(state, preset);
		SyncVisibleNativeMarkerOwnership(state);
		bool published = PublishRuntimeNativeMarkers(state, preset);
		Print(string.Format("h-istasi | rebuilt %1 Tonka-style map marker record(s), published %2 native marker(s)", state.m_aMapMarkers.Count(), m_aRuntimeNativeMarkers.Count()));
		return published;
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
		ClearRuntimeNativeMarkers();
		m_bNativePublishPending = false;
		if (!state)
			return;

		state.m_aMapMarkers.Clear();
	}

	bool TickNativePublish(HST_CampaignState state, HST_CampaignPreset preset, float timeSlice)
	{
		if (!m_bNativePublishPending || !state || !preset)
			return false;

		m_fNativePublishRetrySeconds -= timeSlice;
		if (m_fNativePublishRetrySeconds > 0)
			return false;

		m_fNativePublishRetrySeconds = 1.0;
		ClearRuntimeNativeMarkers();
		return PublishRuntimeNativeMarkers(state, preset);
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
		int callsignCount;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (!marker || !marker.m_bVisible)
				continue;

			if (marker.m_sCategory == "hq" || marker.m_sCategory == "hideout")
				hqCount++;
			else if (marker.m_sCategory == "town")
				townCount++;
			else if (marker.m_sCategory == "mission" || marker.m_sCategory == "mission_objective" || marker.m_sCategory == "mission_asset")
				missionCount++;
			else if (marker.m_sCategory == "qrf")
				qrfCount++;
			else if (marker.m_sCategory == "callsign")
				callsignCount++;
			else
				strategicCount++;
		}

		string summary = string.Format("h-istasi map markers | total %1 | HQ/hideouts %2 | towns %3", state.m_aMapMarkers.Count(), hqCount, townCount);
		string tactical = string.Format(" | strategic %1 | callsigns %2 | missions %3 | QRFs %4 | native manager %5 | native visible %6", strategicCount, callsignCount, missionCount, qrfCount, NATIVE_MARKER_MANAGER_COMPONENT, m_aRuntimeNativeMarkers.Count());
		return summary + tactical;
	}

	protected void AddHQMarker(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state.m_bHQDeployed)
			return;

		AddMarker(state, "hst_hq", state.m_sHQHideoutId, "FIA HQ", "", "hq", preset.m_sResistanceFactionKey, "PICK_UP2", FactionToMarkerColor(preset.m_sResistanceFactionKey, preset), state.m_vHQPosition, true, "green", "support");
	}

	protected void AddHideoutMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_HideoutDefinition hideout : HST_DefaultCatalog.CreateHideouts())
		{
			if (!hideout || hideout.m_sHideoutId == state.m_sHQHideoutId)
				continue;

			AddMarker(state, "hst_hideout_" + hideout.m_sHideoutId, hideout.m_sHideoutId, hideout.m_sDisplayName + " Hideout", "", "hideout", preset.m_sResistanceFactionKey, "PICK_UP2", FactionToMarkerColor(preset.m_sResistanceFactionKey, preset), hideout.m_vPosition, true, "green", "support");
		}
	}

	protected void AddZoneMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;

			string category = ZoneToMarkerCategory(zone);
			string label = BuildZoneMarkerLabel(zone);
			string color = FactionToMarkerColor(zone.m_sOwnerFactionKey, preset);
			string icon = ZoneToMarkerIcon(zone);
			string textColor = ZoneToMarkerTextColor(zone);
			string style = ZoneToMarkerStyle(zone);
			AddMarker(state, "hst_zone_" + zone.m_sZoneId, zone.m_sZoneId, label, zone.m_sMarkerCallsign, category, zone.m_sOwnerFactionKey, icon, color, zone.m_vPosition, true, textColor, style);

			if (!zone.m_sMarkerCallsign.IsEmpty())
				AddMarker(state, "hst_zone_callsign_" + zone.m_sZoneId, zone.m_sZoneId, zone.m_sMarkerCallsign, zone.m_sMarkerCallsign, "callsign", zone.m_sOwnerFactionKey, "POINT_SPECIAL", "MAGENTA", BuildCallsignMarkerPosition(zone), true, "magenta", "callsign");
		}
	}

	protected void AddMissionMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;

			vector markerPosition = ResolveMissionMarkerPosition(state, mission);
			string markerId = mission.m_sMarkerId;
			if (markerId.IsEmpty())
				markerId = "hst_mission_" + mission.m_sInstanceId;

			AddMarker(state, markerId, mission.m_sInstanceId, BuildMissionMarkerLabel(state, mission), "", "mission", preset.m_sResistanceFactionKey, "OBJECTIVE_MARKER", "REFORGER_ORANGE", markerPosition, true, "white", "mission_clickable");
			AddMissionObjectiveMarkers(state, preset, mission);
			AddMissionAssetMarkers(state, preset, mission);
		}
	}

	protected void AddMissionObjectiveMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
				continue;

			HST_MissionAssetState linkedAsset = state.FindMissionAsset(objective.m_sLinkedRuntimeEntityId);
			if (linkedAsset && linkedAsset.m_sKind != "area")
				continue;

			string markerId = string.Format("hst_mission_obj_%1_%2", mission.m_sInstanceId, objective.m_sObjectiveId);
			AddMarker(state, markerId, mission.m_sInstanceId, ShortObjectiveLabel(objective), "", "mission_objective", preset.m_sResistanceFactionKey, "POINT_OF_INTEREST", "REFORGER_ORANGE", objective.m_vPosition, true, "white", "mission_objective");
		}
	}

	protected void AddMissionAssetMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bDestroyed || asset.m_bDelivered)
				continue;
			if (asset.m_sKind == "area")
				continue;

			vector position = asset.m_vCurrentPosition;
			string icon = "POINT_OF_INTEREST";
			string style = "mission_asset";
			string label = BuildMissionAssetMarkerLabel(asset);
			string color = "REFORGER_ORANGE";

			if (!asset.m_bPickedUp && (asset.m_sRole == "city_supplies" || asset.m_sRole == "logistics_cargo" || asset.m_sRole == "convoy_payload"))
			{
				position = asset.m_vSourcePosition;
				icon = "PICK_UP2";
				style = "mission_pickup";
			}
			else if (asset.m_bPickedUp && !asset.m_bDelivered)
			{
				position = asset.m_vTargetPosition;
				icon = "OBJECTIVE_MARKER";
				style = "mission_delivery";
			}
			else if (asset.m_sKind == "target")
			{
				icon = "MINE_SINGLE";
				color = "RED";
			}

			AddMarker(state, "hst_mission_asset_" + asset.m_sAssetId, mission.m_sInstanceId, label, "", "mission_asset", preset.m_sResistanceFactionKey, icon, color, position, true, "white", style);
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

			AddMarker(state, "hst_qrf_" + qrf.m_sInstanceId, qrf.m_sInstanceId, "Enemy QRF " + ResolveZoneDisplayName(targetZone), "", "qrf", qrf.m_sFactionKey, "OBJECTIVE_MARKER", FactionToMarkerColor(qrf.m_sFactionKey, preset), targetZone.m_vPosition, true, FactionToMarkerTextColor(qrf.m_sFactionKey, preset), "enemy_response");
		}
	}

	protected void AddMarker(HST_CampaignState state, string markerId, string linkedId, string label, string callsign, string category, string ownerFactionKey, string iconHint, string colorHint, vector position, bool visible, string textColorHint, string styleHint)
	{
		HST_MapMarkerState marker = new HST_MapMarkerState();
		marker.m_sMarkerId = markerId;
		marker.m_sLinkedId = linkedId;
		marker.m_sLabel = label;
		marker.m_sCallsign = callsign;
		marker.m_sCategory = category;
		marker.m_sOwnerFactionKey = ownerFactionKey;
		marker.m_sIconHint = iconHint;
		marker.m_sColorHint = colorHint;
		marker.m_sTextColorHint = textColorHint;
		marker.m_sStyleHint = styleHint;
		marker.m_vPosition = HST_WorldPositionService.ResolveGroundPosition(position, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		marker.m_bVisible = visible;
		marker.m_bRuntimeNative = true;
		state.m_aMapMarkers.Insert(marker);
	}

	protected bool PublishRuntimeNativeMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state)
			return false;

		SCR_MapMarkerManagerComponent markerManager = ResolveNativeMarkerManager();
		if (!markerManager)
		{
			m_bNativePublishPending = true;
			m_fNativePublishRetrySeconds = 0.25;
			Print("h-istasi | native map marker manager not ready; marker publish pending");
			return false;
		}

		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
			CreateRuntimeNativeMarker(markerManager, marker, preset);

		m_bNativePublishPending = false;
		m_fNativePublishRetrySeconds = 0;
		return true;
	}

	protected void CreateRuntimeNativeMarker(SCR_MapMarkerManagerComponent markerManager, HST_MapMarkerState marker, HST_CampaignPreset preset)
	{
		if (!markerManager || !marker || !marker.m_bVisible || !marker.m_bRuntimeNative)
			return;

		SCR_MapMarkerBase nativeMarker = new SCR_MapMarkerBase();
		nativeMarker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		nativeMarker.SetIconEntry(ResolveNativeIcon(marker.m_sIconHint, marker.m_sCategory, marker.m_sStyleHint));
		nativeMarker.SetColorEntry(ResolveNativeColor(marker, preset));
		nativeMarker.SetRotation(0);
		nativeMarker.SetWorldPos(marker.m_vPosition[0], marker.m_vPosition[2]);
		nativeMarker.SetCustomText(marker.m_sLabel);
		nativeMarker.SetCanBeRemovedByOwner(false);
		nativeMarker.SetTimestampVisibility(false);
		markerManager.InsertStaticMarker(nativeMarker, false, true);
		m_aRuntimeNativeMarkers.Insert(nativeMarker);
	}

	protected void ClearRuntimeNativeMarkers()
	{
		SCR_MapMarkerManagerComponent markerManager = ResolveNativeMarkerManager();
		if (markerManager)
		{
			foreach (SCR_MapMarkerBase nativeMarker : m_aRuntimeNativeMarkers)
			{
				if (!nativeMarker)
					continue;

				SCR_MapMarkerBase activeMarker = markerManager.GetStaticMarkerByID(nativeMarker.GetMarkerID());
				if (activeMarker)
					markerManager.RemoveStaticMarker(activeMarker);
			}
		}

		m_aRuntimeNativeMarkers.Clear();
	}

	protected SCR_MapMarkerManagerComponent ResolveNativeMarkerManager()
	{
		SCR_MapMarkerManagerComponent markerManager = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerManager)
			return markerManager;

		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;

		return SCR_MapMarkerManagerComponent.Cast(gameMode.FindComponent(SCR_MapMarkerManagerComponent));
	}

	protected SCR_EScenarioFrameworkMarkerCustom ResolveNativeIcon(string iconHint, string category, string styleHint)
	{
		if (iconHint == "PICK_UP2")
			return SCR_EScenarioFrameworkMarkerCustom.PICK_UP2;

		if (iconHint == "MINE_SINGLE")
			return SCR_EScenarioFrameworkMarkerCustom.MINE_SINGLE;

		if (iconHint == "POINT_SPECIAL")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_SPECIAL;

		if (iconHint == "POINT_OF_INTEREST")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_OF_INTEREST;

		if (iconHint == "OBSERVATION_POST")
			return SCR_EScenarioFrameworkMarkerCustom.OBSERVATION_POST;

		if (iconHint == "OBJECTIVE_MARKER")
			return SCR_EScenarioFrameworkMarkerCustom.OBJECTIVE_MARKER;

		if (category == "callsign")
			return SCR_EScenarioFrameworkMarkerCustom.POINT_SPECIAL;

		if (styleHint == "resource" || styleHint == "depot")
			return SCR_EScenarioFrameworkMarkerCustom.MINE_SINGLE;

		if (styleHint == "support" || category == "hq" || category == "hideout")
			return SCR_EScenarioFrameworkMarkerCustom.PICK_UP2;

		return SCR_EScenarioFrameworkMarkerCustom.OBJECTIVE_MARKER;
	}

	protected SCR_EScenarioFrameworkMarkerCustomColor ResolveNativeColor(HST_MapMarkerState marker, HST_CampaignPreset preset)
	{
		if (!marker)
			return SCR_EScenarioFrameworkMarkerCustomColor.WHITE;

		if (marker.m_sCategory == "callsign")
			return SCR_EScenarioFrameworkMarkerCustomColor.MAGENTA;

		string colorHint = marker.m_sColorHint;
		if (colorHint == "GREEN")
			return SCR_EScenarioFrameworkMarkerCustomColor.GREEN;

		if (colorHint == "BLUFOR")
			return SCR_EScenarioFrameworkMarkerCustomColor.BLUFOR;

		if (colorHint == "BLUE")
			return SCR_EScenarioFrameworkMarkerCustomColor.BLUE;

		if (colorHint == "RED")
			return SCR_EScenarioFrameworkMarkerCustomColor.RED;

		if (colorHint == "OPFOR")
			return SCR_EScenarioFrameworkMarkerCustomColor.OPFOR;

		if (colorHint == "MAGENTA" || marker.m_sTextColorHint == "magenta")
			return SCR_EScenarioFrameworkMarkerCustomColor.MAGENTA;

		if (colorHint == "CIVILIAN")
			return SCR_EScenarioFrameworkMarkerCustomColor.CIVILIAN;

		if (colorHint == "REFORGER_ORANGE" || marker.m_sTextColorHint == "gold")
			return SCR_EScenarioFrameworkMarkerCustomColor.REFORGER_ORANGE;

		if (preset && marker.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			return SCR_EScenarioFrameworkMarkerCustomColor.GREEN;

		if (preset && marker.m_sOwnerFactionKey == preset.m_sOccupierFactionKey)
			return SCR_EScenarioFrameworkMarkerCustomColor.BLUFOR;

		if (preset && marker.m_sOwnerFactionKey == preset.m_sInvaderFactionKey)
			return SCR_EScenarioFrameworkMarkerCustomColor.RED;

		return SCR_EScenarioFrameworkMarkerCustomColor.WHITE;
	}

	protected void SyncVisibleNativeMarkerOwnership(HST_CampaignState state)
	{
		if (!state)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey.IsEmpty())
				continue;

			m_sNativeMarkerEntityName = "HST_ConflictMapMarker_" + zone.m_sZoneId;
			m_aNativeMarkerCandidates.Clear();
			world.QueryEntitiesBySphere(zone.m_vPosition, 4, AddNativeMarkerCandidate, null, EQueryEntitiesFlags.ALL);
			foreach (IEntity markerEntity : m_aNativeMarkerCandidates)
			{
				FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(markerEntity.FindComponent(FactionAffiliationComponent));
				if (factionComponent)
					factionComponent.SetAffiliatedFactionByKey(zone.m_sOwnerFactionKey);
			}
		}

		m_sNativeMarkerEntityName = "";
		m_aNativeMarkerCandidates.Clear();
	}

	protected bool AddNativeMarkerCandidate(IEntity entity)
	{
		if (!entity || m_sNativeMarkerEntityName.IsEmpty())
			return true;

		if (entity.GetName() == m_sNativeMarkerEntityName)
			m_aNativeMarkerCandidates.Insert(entity);

		return true;
	}

	protected vector BuildCallsignMarkerPosition(HST_ZoneState zone)
	{
		vector position = zone.m_vPosition;
		position[2] = position[2] + 18;
		return position;
	}

	protected string FactionToMarkerColor(string factionKey, HST_CampaignPreset preset)
	{
		if (preset && factionKey == preset.m_sResistanceFactionKey)
			return "GREEN";

		if (preset && factionKey == preset.m_sOccupierFactionKey)
			return "BLUFOR";

		if (preset && factionKey == preset.m_sInvaderFactionKey)
			return "RED";

		return "REFORGER_ORANGE";
	}

	protected string FactionToMarkerTextColor(string factionKey, HST_CampaignPreset preset)
	{
		if (preset && factionKey == preset.m_sResistanceFactionKey)
			return "green";

		if (preset && factionKey == preset.m_sOccupierFactionKey)
			return "blue";

		if (preset && factionKey == preset.m_sInvaderFactionKey)
			return "red";

		return "white";
	}

	protected string ZoneToMarkerCategory(HST_ZoneState zone)
	{
		if (!zone)
			return "strategic";

		if (!zone.m_sMarkerStyle.IsEmpty())
			return zone.m_sMarkerStyle;

		HST_EZoneType zoneType = zone.m_eType;
		if (zoneType == HST_EZoneType.HST_ZONE_TOWN)
			return "town";

		if (zoneType == HST_EZoneType.HST_ZONE_HIDEOUT)
			return "hideout";

		if (zoneType == HST_EZoneType.HST_ZONE_MISSION_SITE)
			return "mission";

		if (zoneType == HST_EZoneType.HST_ZONE_RESOURCE)
			return "resource";

		if (zoneType == HST_EZoneType.HST_ZONE_FACTORY)
			return "factory";

		if (zoneType == HST_EZoneType.HST_ZONE_AIRFIELD || zoneType == HST_EZoneType.HST_ZONE_SEAPORT || zoneType == HST_EZoneType.HST_ZONE_OUTPOST)
			return "enemy_base";

		return "strategic";
	}

	protected string ZoneToMarkerIcon(HST_ZoneState zone)
	{
		if (!zone)
			return "OBJECTIVE_MARKER";

		if (zone && !zone.m_sMarkerStyle.IsEmpty())
		{
			if (zone.m_sMarkerStyle == "resource" || zone.m_sMarkerStyle == "depot")
				return "MINE_SINGLE";
			if (zone.m_sMarkerStyle == "town")
				return "MINE_SINGLE";
			if (zone.m_sMarkerStyle == "radio")
				return "OBSERVATION_POST";
			if (zone.m_sMarkerStyle == "enemy_base" || zone.m_sMarkerStyle == "stronghold")
				return "OBJECTIVE_MARKER";
			if (zone.m_sMarkerStyle == "support")
				return "PICK_UP2";
		}

		HST_EZoneType zoneType = zone.m_eType;
		if (zoneType == HST_EZoneType.HST_ZONE_RESOURCE || zoneType == HST_EZoneType.HST_ZONE_FACTORY)
			return "MINE_SINGLE";

		if (zoneType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return "OBSERVATION_POST";

		if (zoneType == HST_EZoneType.HST_ZONE_HIDEOUT)
			return "PICK_UP2";

		return "OBJECTIVE_MARKER";
	}

	protected string BuildZoneMarkerLabel(HST_ZoneState zone)
	{
		if (!zone)
			return "unknown";

		return ResolveZoneDisplayName(zone);
	}

	protected string ZoneToMarkerTextColor(HST_ZoneState zone)
	{
		if (!zone || zone.m_sMarkerTextColor.IsEmpty())
			return "black";

		return zone.m_sMarkerTextColor;
	}

	protected string ZoneToMarkerStyle(HST_ZoneState zone)
	{
		if (!zone || zone.m_sMarkerStyle.IsEmpty())
			return ZoneToMarkerCategory(zone);

		return zone.m_sMarkerStyle;
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

	protected vector ResolveMissionMarkerPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		if (mission.m_vTargetPosition[0] != 0 || mission.m_vTargetPosition[1] != 0 || mission.m_vTargetPosition[2] != 0)
			return mission.m_vTargetPosition;

		HST_GeneratedSiteState site = state.FindGeneratedSite(mission.m_sSiteId);
		if (site)
			return site.m_vPosition;

		HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
		if (zone)
			return zone.m_vPosition;

		return state.m_vHQPosition;
	}

	protected string BuildMissionMarkerLabel(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission)
			return "Mission";

		string title = mission.m_sDisplayName;
		if (title.IsEmpty())
			title = mission.m_sMissionId;

		int complete;
		int total;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			total++;
			if (objective.m_bComplete)
				complete++;
		}

		return string.Format("%1 | %2s | %3/%4", title, mission.m_iRemainingSeconds, complete, total);
	}

	protected string ShortObjectiveLabel(HST_MissionObjectiveState objective)
	{
		if (!objective)
			return "Objective";

		string label = objective.m_sLabel;
		if (label.IsEmpty())
			label = objective.m_sTargetId;
		if (label.IsEmpty())
			label = "Objective";

		return label;
	}

	protected string BuildMissionAssetMarkerLabel(HST_MissionAssetState asset)
	{
		if (!asset)
			return "Mission asset";

		string verb = "Locate";
		if (!asset.m_bPickedUp && (asset.m_sKind == "cargo" || asset.m_sKind == "captive"))
			verb = "Pickup";
		else if (asset.m_bPickedUp && !asset.m_bDelivered)
			verb = "Deliver";
		else if (asset.m_sKind == "target")
			verb = "Destroy";
		else if (asset.m_sKind == "character")
			verb = "Target";
		else if (asset.m_sKind == "vehicle")
			verb = "Intercept";

		string role = asset.m_sRole;
		if (role.IsEmpty())
			role = asset.m_sKind;
		role.Replace("_", " ");
		return verb + " " + role;
	}
}
