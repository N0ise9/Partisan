class HST_MapMarkerService
{
	static const string NATIVE_MARKER_MANAGER_COMPONENT = "SCR_MapMarkerManagerComponent";
	static const string NATIVE_MARKER_CONFIG = "{3583D42139D9A10B}Configs/Map/CampaignMapMarkerConfig.conf";
	static const float MARKER_DECONFLICT_RADIUS_METERS = 14.0;
	static const float MARKER_DECONFLICT_STEP_METERS = 22.0;
	static const int MARKER_DECONFLICT_ATTEMPTS = 12;
	static const int MAX_NATIVE_MARKERS = 96;
	static const int MAX_NATIVE_TACTICAL_MARKERS = 16;
	static const int NATIVE_OWNERSHIP_SYNC_INTERVAL_SECONDS = 30;
	static const string PERSISTENCE_SMOKE_PREFIX = "hst_smoke";

	protected ref array<IEntity> m_aNativeMarkerCandidates = {};
	protected ref array<ref SCR_MapMarkerBase> m_aRuntimeNativeMarkers = {};
	protected string m_sNativeMarkerEntityName;
	protected bool m_bNativePublishPending;
	protected float m_fNativePublishRetrySeconds;
	protected string m_sLastNativeMarkerSignature;
	protected int m_iLastNativeEligibleCount;
	protected int m_iLastNativePublishedCount;
	protected int m_iLastNativeSkippedCount;
	protected int m_iLastReportedMarkerRecordCount = -1;
	protected int m_iLastNativeOwnershipSyncSecond = -999999;

	bool RebuildAllMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state || !preset)
			return false;

		state.m_aMapMarkers.Clear();
		AddHQMarker(state, preset);
		AddZoneMarkers(state, preset);
		AddMissionMarkers(state, preset);
		AddQRFMarkers(state, preset);
		AddSupportRequestMarkers(state, preset);
		int previousReportedMarkerCount = m_iLastReportedMarkerRecordCount;
		int previousPublishedCount = m_iLastNativePublishedCount;
		int previousEligibleCount = m_iLastNativeEligibleCount;
		int previousSkippedCount = m_iLastNativeSkippedCount;
		bool ownershipSynced = SyncVisibleNativeMarkerOwnershipIfDue(state);
		bool published = PublishRuntimeNativeMarkers(state, preset);
		if (ShouldReportMarkerRebuild(state.m_aMapMarkers.Count(), published, ownershipSynced, previousReportedMarkerCount, previousPublishedCount, previousEligibleCount, previousSkippedCount))
		{
			Print(string.Format("h-istasi | rebuilt %1 campaign map marker record(s), native %2/%3 published, %4 skipped", state.m_aMapMarkers.Count(), m_iLastNativePublishedCount, m_iLastNativeEligibleCount, m_iLastNativeSkippedCount));
			m_iLastReportedMarkerRecordCount = state.m_aMapMarkers.Count();
		}
		return published || ownershipSynced;
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
		m_sLastNativeMarkerSignature = "";
		m_iLastNativeEligibleCount = 0;
		m_iLastNativePublishedCount = 0;
		m_iLastNativeSkippedCount = 0;
		m_iLastReportedMarkerRecordCount = -1;
		m_iLastNativeOwnershipSyncSecond = -999999;
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
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (!marker || !marker.m_bVisible)
				continue;

			if (marker.m_sCategory == "hq")
				hqCount++;
			else if (marker.m_sCategory == "town")
				townCount++;
			else if (marker.m_sCategory == "mission" || marker.m_sCategory == "mission_objective" || marker.m_sCategory == "mission_asset")
				missionCount++;
			else if (marker.m_sCategory == "qrf" || marker.m_sCategory == "support")
				qrfCount++;
			else
				strategicCount++;
		}

		string summary = string.Format("h-istasi map markers | total %1 | HQ %2 | towns %3", state.m_aMapMarkers.Count(), hqCount, townCount);
		string pending = "ready";
		if (m_bNativePublishPending)
			pending = "pending";
		string tactical = string.Format(" | strategic %1 | missions %2 | QRFs/support %3 | native manager %4 | native %5/%6 visible | skipped %7 | %8", strategicCount, missionCount, qrfCount, NATIVE_MARKER_MANAGER_COMPONENT, m_iLastNativePublishedCount, m_iLastNativeEligibleCount, m_iLastNativeSkippedCount, pending);
		return summary + tactical;
	}

	protected void AddHQMarker(HST_CampaignState state, HST_CampaignPreset preset)
	{
		if (!state.m_bHQDeployed)
			return;

		AddMarker(state, "hst_hq", state.m_sHQHideoutId, "FIA HQ", "", "hq", preset.m_sResistanceFactionKey, "PICK_UP2", FactionToMarkerColor(preset.m_sResistanceFactionKey, preset), state.m_vHQPosition, true, "green", "support");
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
			AddMarker(state, "hst_zone_" + zone.m_sZoneId, zone.m_sZoneId, label, "", category, zone.m_sOwnerFactionKey, icon, color, zone.m_vPosition, true, textColor, style);
		}
	}

	protected void AddMissionMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (IsPersistenceSmokeMission(mission))
				continue;
			if (AreMissionObjectivesComplete(state, mission))
				continue;

			bool exactMissionPosition;
			vector markerPosition = ResolveMissionMarkerPosition(state, mission, exactMissionPosition);
			string markerId = mission.m_sMarkerId;
			if (markerId.IsEmpty())
				markerId = "hst_mission_" + mission.m_sInstanceId;

			bool hasSpecificMarker = HasVisibleMissionAssetMarker(state, mission) || HasVisibleMissionObjectiveMarker(state, mission);
			if (!hasSpecificMarker)
				AddMarker(state, markerId, mission.m_sInstanceId, BuildMissionMarkerLabel(state, mission), "", "mission", preset.m_sResistanceFactionKey, MissionToMarkerIcon(mission), MissionToMarkerColor(mission), markerPosition, true, MissionToMarkerTextColor(mission), MissionToMarkerStyle(mission), !exactMissionPosition);
			AddMissionRouteMarkers(state, preset, mission);
			AddMissionObjectiveMarkers(state, preset, mission);
			AddMissionAssetMarkers(state, preset, mission);
		}
	}

	protected bool IsPersistenceSmokeMission(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		return mission.m_sInstanceId.Contains(PERSISTENCE_SMOKE_PREFIX) || mission.m_sMissionId.Contains(PERSISTENCE_SMOKE_PREFIX);
	}

	protected void AddMissionRouteMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		return;
	}

	protected void AddMissionObjectiveMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		bool hasPhysicalAssetMarker = HasVisibleMissionAssetMarker(state, mission);
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
				continue;
			if (mission.m_sRuntimePrimitive == "convoy_intercept" && objective.m_sTargetId == "convoy")
				continue;
			if (hasPhysicalAssetMarker && objective.m_sTargetId != "area" && objective.m_sRuntimePrimitive != "hold_area" && objective.m_sRuntimePrimitive != "clear_area")
				continue;

			HST_MissionAssetState linkedAsset = state.FindMissionAsset(objective.m_sLinkedRuntimeEntityId);
			if (linkedAsset && linkedAsset.m_sKind != "area")
				continue;

			string markerId = string.Format("hst_mission_obj_%1_%2", mission.m_sInstanceId, objective.m_sObjectiveId);
			vector objectivePosition = ResolveMissionObjectiveMarkerPosition(objective, linkedAsset);
			bool deconflictObjective = IsBroadZoneFallbackPosition(state, objective.m_sTargetZoneId, objectivePosition);
			AddMarker(state, markerId, mission.m_sInstanceId, BuildMissionObjectiveMarkerLabel(mission, objective), "", "mission_objective", preset.m_sResistanceFactionKey, MissionObjectiveToMarkerIcon(mission, objective), MissionToMarkerColor(mission), objectivePosition, true, MissionToMarkerTextColor(mission), MissionToMarkerStyle(mission) + "_objective", deconflictObjective);
		}
	}

	protected void AddMissionAssetMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (AreMissionObjectivesComplete(state, mission))
			return;

		if (mission.m_sRuntimePrimitive == "convoy_intercept")
		{
			AddMissionConvoyMarkers(state, preset, mission);
			return;
		}

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bDestroyed || asset.m_bDelivered)
				continue;
			if (asset.m_sKind == "area")
				continue;
			if (asset.m_sRole == "convoy_vehicle" && HasMissionConvoyPayloadSatisfied(state, mission))
				continue;

			vector position = asset.m_vCurrentPosition;
			string icon = MissionAssetToMarkerIcon(mission, asset);
			string style = MissionAssetToMarkerStyle(mission, asset);
			string label = BuildMissionAssetMarkerLabel(mission, asset);
			string color = MissionAssetToMarkerColor(mission, asset);

			if (!asset.m_bPickedUp && (asset.m_sRole == "city_supplies" || asset.m_sRole == "logistics_cargo" || asset.m_sRole == "convoy_payload"))
			{
				position = asset.m_vSourcePosition;
			}
			else if (asset.m_bPickedUp && !asset.m_bDelivered)
			{
				if (!IsZeroVector(asset.m_vTargetPosition))
					position = asset.m_vTargetPosition;
				else
					position = ResolveMissionAssetMarkerPosition(asset);
			}

			AddMarker(state, "hst_mission_asset_" + asset.m_sAssetId, mission.m_sInstanceId, label, "", "mission_asset", preset.m_sResistanceFactionKey, icon, color, position, true, MissionToMarkerTextColor(mission), style, true);
		}
	}

	protected void AddMissionConvoyMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission)
	{
		if (!state || !preset || !mission)
			return;

		HST_MissionAssetState outcomeAsset;
		if (ShouldShowConvoyOutcomeMarkers(mission))
			outcomeAsset = SelectPendingConvoyOutcomeAsset(state, mission);
		if (outcomeAsset && (outcomeAsset.m_sRole == "convoy_payload" || outcomeAsset.m_sRole == "convoy_captive"))
		{
			vector outcomePosition = ResolveConvoyOutcomeMarkerPosition(state, mission, outcomeAsset);
			AddMarker(state, "hst_mission_convoy_outcome_" + outcomeAsset.m_sAssetId, mission.m_sInstanceId, BuildConvoyOutcomeMarkerLabel(mission, outcomeAsset), "", "mission_asset", preset.m_sResistanceFactionKey, MissionAssetToMarkerIcon(mission, outcomeAsset), MissionToMarkerColor(mission), outcomePosition, true, MissionToMarkerTextColor(mission), MissionAssetToMarkerStyle(mission, outcomeAsset), true);
			return;
		}
		if (outcomeAsset && outcomeAsset.m_sRole == "convoy_vehicle" && mission.m_sRuntimePhase == "convoy_eliminated")
		{
			AddMissionConvoyVehicleMarkers(state, preset, mission, MissionMarkerTitle(mission));
			return;
		}

		vector convoyPosition = ResolveMissionConvoyAggregatePosition(state, mission);
		vector destinationPosition = ResolveMissionConvoyDestinationPosition(state, mission);
		string title = MissionMarkerTitle(mission);
		string destinationName = ResolveZoneDisplayNameById(state, mission.m_sTargetZoneId);
		bool hasConvoyVehicleAssets = HasUnresolvedMissionConvoyVehicleAsset(state, mission);
		if (hasConvoyVehicleAssets)
		{
			if (ShouldShowIndividualConvoyVehicleMarkers(mission))
				AddMissionConvoyVehicleMarkers(state, preset, mission, title);
			else
				AddMarker(state, "hst_mission_convoy_current_" + mission.m_sInstanceId, mission.m_sInstanceId, string.Format("Convoy - %1: neutralize crew", title), "", "mission_asset", preset.m_sResistanceFactionKey, "POINT_SPECIAL", MissionToMarkerColor(mission), convoyPosition, true, MissionToMarkerTextColor(mission), "mission_convoy_vehicle", true);
		}
		AddMarker(state, "hst_mission_convoy_dest_" + mission.m_sInstanceId, mission.m_sInstanceId, string.Format("Convoy destination - %1: %2", title, destinationName), "", "mission_objective", preset.m_sResistanceFactionKey, "OBJECTIVE_MARKER", MissionToMarkerColor(mission), destinationPosition, true, MissionToMarkerTextColor(mission), "mission_convoy_destination", true);
	}

	protected bool ShouldShowIndividualConvoyVehicleMarkers(HST_ActiveMissionState mission)
	{
		if (!mission)
			return false;

		return mission.m_sRuntimePhase == "convoy_moving" || mission.m_sRuntimePhase == "convoy_contact" || mission.m_sRuntimePhase == "convoy_eliminated";
	}

	protected bool HasUnresolvedMissionConvoyVehicleAsset(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == "convoy_vehicle" && !asset.m_bDestroyed && !asset.m_bDelivered)
				return true;
		}

		return false;
	}

	protected void AddMissionConvoyVehicleMarkers(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, string title)
	{
		int vehicleIndex;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != "convoy_vehicle" || asset.m_bDestroyed || asset.m_bDelivered)
				continue;

			vector position = asset.m_vCurrentPosition;
			if (IsZeroVector(position))
				position = asset.m_vSourcePosition;

			string markerId = "hst_mission_convoy_vehicle_" + asset.m_sAssetId;
			string label;
			if (mission.m_sRuntimePhase == "convoy_eliminated")
				label = string.Format("Convoy vehicle %1 - %2: capture vehicle", vehicleIndex + 1, title);
			else
				label = string.Format("Convoy vehicle %1 - %2: neutralize crew", vehicleIndex + 1, title);
			AddMarker(state, markerId, mission.m_sInstanceId, label, "", "mission_asset", preset.m_sResistanceFactionKey, "POINT_SPECIAL", MissionToMarkerColor(mission), position, true, MissionToMarkerTextColor(mission), "mission_convoy_vehicle", true);
			vehicleIndex++;
		}
	}

	protected vector ResolveMissionConvoyAggregatePosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		vector aggregate;
		int count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != "convoy_vehicle" || asset.m_bDestroyed || asset.m_bDelivered)
				continue;

			vector position = asset.m_vCurrentPosition;
			if (IsZeroVector(position))
				position = asset.m_vSourcePosition;
			aggregate[0] = aggregate[0] + position[0];
			aggregate[1] = aggregate[1] + position[1];
			aggregate[2] = aggregate[2] + position[2];
			count++;
		}

		if (count > 0)
		{
			aggregate[0] = aggregate[0] / count;
			aggregate[1] = aggregate[1] / count;
			aggregate[2] = aggregate[2] / count;
			return aggregate;
		}

		bool exactMissionPosition;
		return ResolveMissionMarkerPosition(state, mission, exactMissionPosition);
	}

	protected vector ResolveMissionConvoyDestinationPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId && asset.m_sRole == "convoy_vehicle" && !IsZeroVector(asset.m_vTargetPosition))
				return asset.m_vTargetPosition;
		}

		if (!IsZeroVector(mission.m_vTargetPosition))
			return mission.m_vTargetPosition;

		return ResolveMissionConvoyAggregatePosition(state, mission);
	}

	protected void AddQRFMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (!ShouldShowQRFMarker(state, qrf))
				continue;

			HST_ZoneState targetZone = state.FindZone(qrf.m_sTargetZoneId);
			if (!targetZone)
				continue;

			vector position = ResolveQRFMarkerPosition(state, qrf, targetZone);
			string label = BuildQRFMarkerLabel(state, qrf, targetZone);
			bool runtimeNative = ShouldPublishNativeTacticalMarkerInPlayerBubble(state, targetZone.m_sZoneId, position);
			AddMarker(state, "hst_qrf_" + qrf.m_sInstanceId, qrf.m_sInstanceId, label, "", "qrf", qrf.m_sFactionKey, "OBJECTIVE_MARKER", FactionToMarkerColor(qrf.m_sFactionKey, preset), position, true, FactionToMarkerTextColor(qrf.m_sFactionKey, preset), "enemy_response", true, runtimeNative);
		}
	}

	protected bool ShouldShowQRFMarker(HST_CampaignState state, HST_QRFState qrf)
	{
		if (!state || !qrf)
			return false;

		if (!qrf.m_bResolved)
			return true;

		if (qrf.m_sGroupId.IsEmpty())
			return false;

		HST_ActiveGroupState group = state.FindActiveGroup(qrf.m_sGroupId);
		if (!group)
			return false;

		return group.m_sRuntimeStatus != "eliminated" && group.m_sRuntimeStatus != "folded" && group.m_sRuntimeStatus != "spawn_failed";
	}

	protected void AddSupportRequestMarkers(HST_CampaignState state, HST_CampaignPreset preset)
	{
		foreach (HST_SupportRequestState request : state.m_aSupportRequests)
		{
			if (!request || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED || request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
				continue;

			vector position = ResolveSupportMarkerPosition(state, request);
			string color = FactionToMarkerColor(request.m_sFactionKey, preset);
			string label = BuildSupportMarkerLabel(state, request);
			bool runtimeNative = ShouldPublishNativeSupportMarker(request) && ShouldPublishNativeTacticalMarkerInPlayerBubble(state, request.m_sTargetZoneId, position);
			AddMarker(state, "hst_support_" + request.m_sRequestId, request.m_sRequestId, label, "", "support", request.m_sFactionKey, "POINT_OF_INTEREST", color, position, true, FactionToMarkerTextColor(request.m_sFactionKey, preset), "support_incoming", true, runtimeNative);
		}
	}

	protected void AddMarker(HST_CampaignState state, string markerId, string linkedId, string label, string callsign, string category, string ownerFactionKey, string iconHint, string colorHint, vector position, bool visible, string textColorHint, string styleHint, bool deconflict = false, bool runtimeNative = true)
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
		marker.m_vPosition = ResolveMarkerPresentationPosition(state, markerId, position, deconflict);
		marker.m_bVisible = visible;
		marker.m_bRuntimeNative = runtimeNative;
		state.m_aMapMarkers.Insert(marker);
	}

	protected bool ShouldPublishNativeSupportMarker(HST_SupportRequestState request)
	{
		if (!request)
			return false;

		return request.m_eType == HST_ESupportRequestType.HST_SUPPORT_QRF || request.m_eType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY;
	}

	protected bool ShouldPublishNativeTacticalMarkerInPlayerBubble(HST_CampaignState state, string zoneId, vector position)
	{
		if (!state)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (zone && zone.m_bActive)
			return true;

		if (IsZeroVector(position) && zone)
			position = zone.m_vPosition;

		return HST_WorldPositionService.IsPositionInsidePlayerEventBubble(position);
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

		string nativeSignature = BuildNativeMarkerSignature(state);
		if (nativeSignature == m_sLastNativeMarkerSignature && HasRuntimeNativeMarkers(markerManager))
		{
			m_bNativePublishPending = false;
			m_fNativePublishRetrySeconds = 0;
			return false;
		}

		ClearRuntimeNativeMarkers();
		m_iLastNativeEligibleCount = 0;
		m_iLastNativePublishedCount = 0;
		m_iLastNativeSkippedCount = 0;

		int tacticalCount;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (!IsNativeMarkerCandidate(marker))
				continue;

			m_iLastNativeEligibleCount++;
			if (!CanPublishNativeMarker(marker, tacticalCount, m_iLastNativePublishedCount))
			{
				m_iLastNativeSkippedCount++;
				continue;
			}

			if (CreateRuntimeNativeMarker(markerManager, marker, preset))
			{
				m_iLastNativePublishedCount++;
				if (IsTacticalNativeMarker(marker))
					tacticalCount++;
			}
			else
			{
				m_iLastNativeSkippedCount++;
			}
		}

		m_sLastNativeMarkerSignature = nativeSignature;
		m_bNativePublishPending = false;
		m_fNativePublishRetrySeconds = 0;
		return true;
	}

	protected bool ShouldReportMarkerRebuild(int markerCount, bool published, bool ownershipSynced, int previousMarkerCount, int previousPublishedCount, int previousEligibleCount, int previousSkippedCount)
	{
		if (!published && !ownershipSynced)
			return false;
		if (markerCount != previousMarkerCount)
			return true;
		if (m_iLastNativePublishedCount != previousPublishedCount)
			return true;
		if (m_iLastNativeEligibleCount != previousEligibleCount)
			return true;
		if (m_iLastNativeSkippedCount != previousSkippedCount)
			return true;

		return false;
	}

	protected bool CreateRuntimeNativeMarker(SCR_MapMarkerManagerComponent markerManager, HST_MapMarkerState marker, HST_CampaignPreset preset)
	{
		if (!markerManager || !marker || !marker.m_bVisible || !marker.m_bRuntimeNative)
			return false;

		SCR_MapMarkerBase nativeMarker = new SCR_MapMarkerBase();
		nativeMarker.SetType(SCR_EMapMarkerType.PLACED_CUSTOM);
		nativeMarker.SetIconEntry(ResolveNativeIcon(marker.m_sIconHint, marker.m_sCategory, marker.m_sStyleHint));
		nativeMarker.SetColorEntry(ResolveNativeColor(marker, preset));
		nativeMarker.SetRotation(0);
		nativeMarker.SetWorldPos(marker.m_vPosition[0], marker.m_vPosition[2]);
		nativeMarker.SetCustomText(ResolveNativeMarkerText(marker));
		nativeMarker.SetCanBeRemovedByOwner(false);
		nativeMarker.SetTimestampVisibility(false);
		markerManager.InsertStaticMarker(nativeMarker, false, true);
		m_aRuntimeNativeMarkers.Insert(nativeMarker);
		return true;
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
				else
					markerManager.RemoveStaticMarker(nativeMarker);
			}
		}

		m_aRuntimeNativeMarkers.Clear();
	}

	protected string BuildNativeMarkerSignature(HST_CampaignState state)
	{
		if (!state)
			return "";

		string signature = "";
		int tacticalCount;
		int publishedCount;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (!CanPublishNativeMarker(marker, tacticalCount, publishedCount))
				continue;

			signature = signature + "|" + BuildNativeMarkerSignatureEntry(marker);
			publishedCount++;
			if (IsTacticalNativeMarker(marker))
				tacticalCount++;
		}

		return signature;
	}

	protected string BuildNativeMarkerSignatureEntry(HST_MapMarkerState marker)
	{
		if (!marker)
			return "";

		int x = Math.Round(marker.m_vPosition[0]);
		int z = Math.Round(marker.m_vPosition[2]);
		if (IsTacticalNativeMarker(marker))
		{
			x = Math.Round(marker.m_vPosition[0] / 25.0) * 25;
			z = Math.Round(marker.m_vPosition[2] / 25.0) * 25;
		}
		return string.Format("%1:%2:%3:%4:%5:%6:%7:%8", marker.m_sMarkerId, ResolveNativeMarkerText(marker), marker.m_sCategory, marker.m_sOwnerFactionKey, marker.m_sIconHint, marker.m_sColorHint, x, z);
	}

	protected string ResolveNativeMarkerText(HST_MapMarkerState marker)
	{
		if (!marker)
			return "";

		if (marker.m_sCategory != "mission")
			return ShortMarkerText(marker.m_sLabel, 42);

		int separator = marker.m_sLabel.IndexOf(" |");
		if (separator <= 0)
			return ShortMarkerText(marker.m_sLabel, 42);

		return ShortMarkerText(marker.m_sLabel.Substring(0, separator), 42);
	}

	protected bool HasRuntimeNativeMarkers(SCR_MapMarkerManagerComponent markerManager)
	{
		if (!markerManager || m_aRuntimeNativeMarkers.Count() == 0)
			return false;

		foreach (SCR_MapMarkerBase nativeMarker : m_aRuntimeNativeMarkers)
		{
			if (!nativeMarker)
				return false;

			if (!markerManager.GetStaticMarkerByID(nativeMarker.GetMarkerID()))
				return false;
		}

		return true;
	}

	protected bool IsNativeMarkerCandidate(HST_MapMarkerState marker)
	{
		return marker && marker.m_bVisible && marker.m_bRuntimeNative;
	}

	protected bool CanPublishNativeMarker(HST_MapMarkerState marker, int tacticalCount, int publishedCount)
	{
		if (!IsNativeMarkerCandidate(marker))
			return false;

		if (marker.m_sCategory == "mission_route")
			return false;

		if (publishedCount >= MAX_NATIVE_MARKERS)
			return false;

		if (IsTacticalNativeMarker(marker) && tacticalCount >= MAX_NATIVE_TACTICAL_MARKERS)
			return false;

		return true;
	}

	protected bool IsTacticalNativeMarker(HST_MapMarkerState marker)
	{
		if (!marker)
			return false;

		return marker.m_sCategory == "mission" || marker.m_sCategory == "mission_objective" || marker.m_sCategory == "mission_asset" || marker.m_sCategory == "qrf" || marker.m_sCategory == "support";
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

	protected bool SyncVisibleNativeMarkerOwnershipIfDue(HST_CampaignState state)
	{
		if (!state)
			return false;

		if (state.m_iElapsedSeconds - m_iLastNativeOwnershipSyncSecond < NATIVE_OWNERSHIP_SYNC_INTERVAL_SECONDS)
			return false;

		m_iLastNativeOwnershipSyncSecond = state.m_iElapsedSeconds;
		SyncVisibleNativeMarkerOwnership(state);
		return true;
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

	protected vector ResolveMissionMarkerPosition(HST_CampaignState state, HST_ActiveMissionState mission, out bool exactMissionPosition)
	{
		exactMissionPosition = false;
		if (!state || !mission)
			return "0 0 0";

		vector representativeAssetPosition = ResolveRepresentativeMissionAssetPosition(state, mission);
		if (!IsZeroVector(representativeAssetPosition))
		{
			exactMissionPosition = true;
			return representativeAssetPosition;
		}

		vector activeObjectivePosition = ResolveRepresentativeObjectivePosition(state, mission);
		if (!IsZeroVector(activeObjectivePosition))
		{
			exactMissionPosition = !IsBroadZoneFallbackPosition(state, mission.m_sTargetZoneId, activeObjectivePosition);
			return activeObjectivePosition;
		}

		if (!IsZeroVector(mission.m_vTargetPosition))
		{
			exactMissionPosition = !IsBroadZoneFallbackPosition(state, mission.m_sTargetZoneId, mission.m_vTargetPosition);
			return mission.m_vTargetPosition;
		}

		HST_GeneratedSiteState site = state.FindGeneratedSite(mission.m_sSiteId);
		if (site)
		{
			exactMissionPosition = true;
			return site.m_vPosition;
		}

		HST_ZoneState zone = state.FindZone(mission.m_sTargetZoneId);
		if (zone)
			return zone.m_vPosition;

		return state.m_vHQPosition;
	}

	protected vector ResolveRepresentativeMissionAssetPosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bDestroyed || asset.m_bDelivered)
				continue;
			if (asset.m_sKind == "area")
				continue;

			return ResolveMissionAssetMarkerPosition(asset);
		}

		return "0 0 0";
	}

	protected vector ResolveRepresentativeObjectivePosition(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return "0 0 0";

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
				continue;

			HST_MissionAssetState linkedAsset = state.FindMissionAsset(objective.m_sLinkedRuntimeEntityId);
			return ResolveMissionObjectiveMarkerPosition(objective, linkedAsset);
		}

		return "0 0 0";
	}

	protected vector ResolveMissionObjectiveMarkerPosition(HST_MissionObjectiveState objective, HST_MissionAssetState linkedAsset)
	{
		if (linkedAsset)
			return ResolveMissionAssetMarkerPosition(linkedAsset);

		if (objective && !IsZeroVector(objective.m_vPosition))
			return objective.m_vPosition;

		return "0 0 0";
	}

	protected vector ResolveMissionAssetMarkerPosition(HST_MissionAssetState asset)
	{
		if (!asset)
			return "0 0 0";

		if (!IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;
		if (!IsZeroVector(asset.m_vSourcePosition))
			return asset.m_vSourcePosition;
		if (!IsZeroVector(asset.m_vTargetPosition))
			return asset.m_vTargetPosition;

		return "0 0 0";
	}

	protected vector ResolveMarkerPresentationPosition(HST_CampaignState state, string markerId, vector position, bool deconflict)
	{
		vector resolved = HST_WorldPositionService.ResolveGroundPosition(position, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
		if (!deconflict || !state || !HasMarkerCollision(state, resolved))
			return resolved;

		for (int attempt = 0; attempt < MARKER_DECONFLICT_ATTEMPTS; attempt++)
		{
			vector candidate = BuildMarkerOffsetPosition(resolved, markerId, attempt);
			candidate = HST_WorldPositionService.ResolveGroundPosition(candidate, HST_WorldPositionService.PROP_GROUND_OFFSET, false);
			if (!HasMarkerCollision(state, candidate))
				return candidate;
		}

		return resolved;
	}

	protected bool HasMarkerCollision(HST_CampaignState state, vector position)
	{
		if (!state)
			return false;

		float radiusSq = MARKER_DECONFLICT_RADIUS_METERS * MARKER_DECONFLICT_RADIUS_METERS;
		foreach (HST_MapMarkerState marker : state.m_aMapMarkers)
		{
			if (!marker || !marker.m_bVisible)
				continue;

			if (DistanceSq2D(marker.m_vPosition, position) <= radiusSq)
				return true;
		}

		return false;
	}

	protected vector BuildMarkerOffsetPosition(vector position, string markerId, int attempt)
	{
		vector candidate = position;
		int baseSlot = markerId.Length() + attempt;
		int ring = baseSlot / 8;
		int slot = baseSlot - ring * 8;
		float distance = MARKER_DECONFLICT_STEP_METERS + ring * 10.0;
		float x = 1.0;
		float z = 0.0;

		if (slot == 1)
		{
			x = 0.707;
			z = 0.707;
		}
		else if (slot == 2)
		{
			x = 0.0;
			z = 1.0;
		}
		else if (slot == 3)
		{
			x = -0.707;
			z = 0.707;
		}
		else if (slot == 4)
		{
			x = -1.0;
			z = 0.0;
		}
		else if (slot == 5)
		{
			x = -0.707;
			z = -0.707;
		}
		else if (slot == 6)
		{
			x = 0.0;
			z = -1.0;
		}
		else if (slot == 7)
		{
			x = 0.707;
			z = -0.707;
		}

		candidate[0] = candidate[0] + x * distance;
		candidate[2] = candidate[2] + z * distance;
		return candidate;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected bool IsBroadZoneFallbackPosition(HST_CampaignState state, string zoneId, vector position)
	{
		if (!state || zoneId.IsEmpty() || IsZeroVector(position))
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return false;

		return DistanceSq2D(zone.m_vPosition, position) <= 9.0;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}

	protected string MissionFamilyKey(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "mission";

		string primitive = mission.m_sRuntimePrimitive;
		string missionId = mission.m_sMissionId;

		if (primitive == "convoy_intercept" || missionId.Contains("convoy"))
			return "convoy";
		if (primitive == "rescue_extract" || missionId.Contains("rescue") || missionId.Contains("pow"))
			return "rescue";
		if (primitive == "kill_hvt" || missionId.Contains("assassinate") || missionId.Contains("officer"))
			return "hvt";
		if (primitive == "destroy_target" || missionId.Contains("destroy") || missionId.Contains("radio"))
			return "destroy";
		if (primitive == "deliver_supplies" || missionId.Contains("support_city") || missionId.Contains("supplies"))
			return "support";
		if (primitive == "recover_cargo" || missionId.Contains("logistics") || missionId.Contains("resource") || missionId.Contains("cache"))
			return "logistics";
		if (primitive == "hold_area" || primitive == "clear_area" || missionId.Contains("conquest"))
			return "conquest";

		return "mission";
	}

	protected string MissionFamilyLabel(HST_ActiveMissionState mission)
	{
		string family = MissionFamilyKey(mission);
		if (family == "convoy")
			return "Convoy";
		if (family == "rescue")
			return "Rescue";
		if (family == "hvt")
			return "HVT";
		if (family == "destroy")
			return "Destroy";
		if (family == "support")
			return "Support";
		if (family == "logistics")
			return "Logistics";
		if (family == "conquest")
			return "Assault";

		return "Mission";
	}

	protected string MissionToMarkerIcon(HST_ActiveMissionState mission)
	{
		string family = MissionFamilyKey(mission);
		if (family == "convoy")
			return "POINT_SPECIAL";
		if (family == "rescue")
			return "PICK_UP2";
		if (family == "hvt")
			return "POINT_OF_INTEREST";
		if (family == "destroy")
			return "MINE_SINGLE";
		if (family == "support")
			return "PICK_UP2";
		if (family == "logistics")
			return "PICK_UP2";
		if (family == "conquest")
			return "OBJECTIVE_MARKER";

		return "OBJECTIVE_MARKER";
	}

	protected string MissionToMarkerColor(HST_ActiveMissionState mission)
	{
		return "REFORGER_ORANGE";
	}

	protected string MissionToMarkerTextColor(HST_ActiveMissionState mission)
	{
		return "gold";
	}

	protected string MissionToMarkerStyle(HST_ActiveMissionState mission)
	{
		return "mission_clickable_" + MissionFamilyKey(mission);
	}

	protected string MissionObjectiveToMarkerIcon(HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!objective)
			return MissionToMarkerIcon(mission);

		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_KILL_TARGET)
			return "POINT_OF_INTEREST";
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET)
			return "MINE_SINGLE";
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RECOVER_LOOT || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DELIVER_SUPPLIES || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_RESCUE_CAPTIVES)
			return "PICK_UP2";
		if (objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_HOLD_AREA || objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_CLEAR_AREA)
			return "OBJECTIVE_MARKER";

		return MissionToMarkerIcon(mission);
	}

	protected string MissionAssetToMarkerIcon(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!asset)
			return MissionToMarkerIcon(mission);

		if (asset.m_bPickedUp && !asset.m_bDelivered)
			return "OBJECTIVE_MARKER";
		if (asset.m_sRole == "convoy_vehicle")
			return "POINT_SPECIAL";
		if (asset.m_sRole == "hvt")
			return "POINT_OF_INTEREST";
		if (asset.m_sRole == "destroy_target")
			return "MINE_SINGLE";
		if (asset.m_sRole == "city_supplies" || asset.m_sRole == "logistics_cargo" || asset.m_sRole == "convoy_payload" || asset.m_sRole == "captive" || asset.m_sRole == "convoy_captive")
			return "PICK_UP2";
		if (asset.m_sKind == "target")
			return "MINE_SINGLE";
		if (asset.m_sKind == "vehicle")
			return "POINT_SPECIAL";

		return MissionToMarkerIcon(mission);
	}

	protected string MissionAssetToMarkerColor(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		return "REFORGER_ORANGE";
	}

	protected string MissionAssetToMarkerStyle(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!asset)
			return MissionToMarkerStyle(mission) + "_asset";

		if (asset.m_bPickedUp && !asset.m_bDelivered)
			return "mission_delivery";
		if (asset.m_sRole == "convoy_vehicle")
			return "mission_convoy_vehicle";
		if (asset.m_sRole == "convoy_payload")
			return "mission_convoy_payload";
		if (asset.m_sRole == "convoy_captive")
			return "mission_convoy_captive";
		if (asset.m_sRole == "hvt")
			return "mission_hvt_asset";
		if (asset.m_sRole == "destroy_target")
			return "mission_destroy_asset";
		if (asset.m_sRole == "city_supplies")
			return "mission_support_pickup";
		if (asset.m_sRole == "logistics_cargo")
			return "mission_logistics_pickup";
		if (asset.m_sRole == "captive")
			return "mission_rescue_captive";

		return MissionToMarkerStyle(mission) + "_asset";
	}

	protected string BuildMissionMarkerLabel(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission)
			return "Mission";

		string family = MissionFamilyLabel(mission);
		string title = MissionMarkerTitle(mission);
		string targetName = ResolveZoneDisplayNameById(state, mission.m_sTargetZoneId);

		int complete;
		int total;
		if (state)
		{
			foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
			{
				if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
					continue;

				total++;
				if (objective.m_bComplete)
					complete++;
			}
		}

		return string.Format("%1 - %2: %3 | %4s | %5/%6", family, title, targetName, mission.m_iRemainingSeconds, complete, total);
	}

	protected string BuildMissionObjectiveMarkerLabel(HST_ActiveMissionState mission, HST_MissionObjectiveState objective)
	{
		if (!objective)
			return "Objective";

		string label = objective.m_sLabel;
		if (label.IsEmpty())
			label = objective.m_sTargetId;
		if (label.IsEmpty())
			label = "Objective";

		return string.Format("%1 - %2: %3", MissionFamilyLabel(mission), MissionMarkerTitle(mission), label);
	}

	protected string BuildMissionAssetMarkerLabel(HST_ActiveMissionState mission, HST_MissionAssetState asset)
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

		if (asset.m_sRole == "convoy_vehicle")
			verb = "Stop";
		else if (asset.m_sRole == "convoy_payload")
			verb = "Recover";
		else if (asset.m_sRole == "convoy_captive")
			verb = "Free";
		else if (asset.m_sRole == "hvt")
			verb = "Kill";

		string role = MissionAssetReadableRole(asset);
		return string.Format("%1 - %2: %3 %4", MissionFamilyLabel(mission), MissionMarkerTitle(mission), verb, role);
	}

	protected bool AreMissionObjectivesComplete(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		if (HasPendingRequiredConvoyOutcome(state, mission))
			return false;

		bool found;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;

			found = true;
			if (!objective.m_bComplete || objective.m_bFailed)
				return false;
		}

		return found;
	}

	protected bool HasPendingRequiredConvoyOutcome(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sRuntimePrimitive != "convoy_intercept")
			return false;

		if (mission.m_sMissionId == "convoy_money")
			return HasPendingConvoyAssetOutcome(state, mission, "convoy_payload");
		if (mission.m_sMissionId == "convoy_supplies")
			return !mission.m_bConvoyCrewEliminatedOutcomeApplied && HasPendingConvoyAssetOutcome(state, mission, "convoy_payload");
		if (mission.m_sMissionId == "convoy_prisoners")
			return HasPendingConvoyAssetOutcome(state, mission, "convoy_captive");
		if (mission.m_sMissionId == "convoy_ammo" || mission.m_sMissionId == "convoy_armored")
			return HasPendingConvoyVehicleCaptureOutcome(state, mission);

		return false;
	}

	protected bool HasPendingConvoyAssetOutcome(HST_CampaignState state, HST_ActiveMissionState mission, string role)
	{
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role)
				continue;

			if (!asset.m_bDelivered || !asset.m_bOutcomeApplied)
				return true;
		}

		return false;
	}

	protected bool HasPendingConvoyVehicleCaptureOutcome(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (mission.m_bConvoyVehicleCapturedOutcomeApplied)
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != "convoy_vehicle")
				continue;
			if (!asset.m_bDestroyed && !asset.m_bDelivered)
				return true;
		}

		return false;
	}

	protected bool ShouldShowConvoyOutcomeMarkers(HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_sRuntimePrimitive != "convoy_intercept")
			return false;

		if (mission.m_bConvoyCrewEliminatedOutcomeApplied)
			return true;
		if (mission.m_sRuntimePhase == "convoy_eliminated")
			return true;
		if (mission.m_sLastRuntimeEventKey == "convoy_complete" || mission.m_sLastRuntimeEventKey == "convoy_secured_sent")
			return true;

		return false;
	}

	protected HST_MissionAssetState SelectPendingConvoyOutcomeAsset(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != "convoy_intercept")
			return null;

		if (mission.m_sMissionId == "convoy_money" || mission.m_sMissionId == "convoy_supplies")
			return SelectPendingConvoyAssetByRole(state, mission, "convoy_payload");
		if (mission.m_sMissionId == "convoy_prisoners")
			return SelectPendingConvoyAssetByRole(state, mission, "convoy_captive");
		if ((mission.m_sMissionId == "convoy_ammo" || mission.m_sMissionId == "convoy_armored") && !mission.m_bConvoyVehicleCapturedOutcomeApplied)
			return SelectPendingConvoyAssetByRole(state, mission, "convoy_vehicle");

		return null;
	}

	protected HST_MissionAssetState SelectPendingConvoyAssetByRole(HST_CampaignState state, HST_ActiveMissionState mission, string role)
	{
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_sRole != role || asset.m_bDestroyed)
				continue;
			if (role == "convoy_vehicle" && asset.m_bDelivered)
				continue;
			if ((role == "convoy_payload" || role == "convoy_captive") && asset.m_bDelivered && asset.m_bOutcomeApplied)
				continue;

			return asset;
		}

		return null;
	}

	protected vector ResolveConvoyOutcomeMarkerPosition(HST_CampaignState state, HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		if (!asset)
			return ResolveMissionConvoyAggregatePosition(state, mission);

		if (asset.m_bPickedUp && !asset.m_bDelivered && !IsZeroVector(asset.m_vTargetPosition))
			return asset.m_vTargetPosition;

		if (!asset.m_bPickedUp && (asset.m_sRole == "convoy_payload" || asset.m_sRole == "convoy_captive"))
			return ResolveMissionConvoyAggregatePosition(state, mission);

		if (!IsZeroVector(asset.m_vCurrentPosition))
			return asset.m_vCurrentPosition;
		if (!IsZeroVector(asset.m_vSourcePosition))
			return asset.m_vSourcePosition;

		return ResolveMissionConvoyAggregatePosition(state, mission);
	}

	protected string BuildConvoyOutcomeMarkerLabel(HST_ActiveMissionState mission, HST_MissionAssetState asset)
	{
		string title = MissionMarkerTitle(mission);
		string role = MissionAssetReadableRole(asset);
		if (asset && asset.m_bPickedUp && !asset.m_bDelivered)
		{
			if (asset.m_sRole == "convoy_captive")
				return string.Format("Extraction - %1: extract %2", title, role);
			return string.Format("Delivery - %1: deliver %2", title, role);
		}
		if (asset && asset.m_sRole == "convoy_captive")
			return string.Format("Secured convoy - %1: free %2", title, role);

		return string.Format("Secured convoy - %1: recover %2", title, role);
	}

	protected bool HasMissionConvoyPayloadSatisfied(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || mission.m_sRuntimePrimitive != "convoy_intercept")
			return false;

		bool hasPayload;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			if (asset.m_sRole != "convoy_payload" && asset.m_sRole != "convoy_captive")
				continue;

			hasPayload = true;
			if (asset.m_sRole == "convoy_captive")
			{
				if (!asset.m_bDelivered)
					return false;
			}
			else if (!asset.m_bPickedUp && !asset.m_bDelivered)
			{
				return false;
			}
		}

		return hasPayload;
	}

	protected bool HasVisibleMissionAssetMarker(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId || asset.m_bDestroyed || asset.m_bDelivered)
				continue;
			if (asset.m_sKind == "area")
				continue;
			if (asset.m_sRole == "convoy_vehicle" && HasMissionConvoyPayloadSatisfied(state, mission))
				continue;

			return true;
		}

		return false;
	}

	protected bool HasVisibleMissionObjectiveMarker(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission)
			return false;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId || objective.m_bComplete || objective.m_bFailed)
				continue;

			HST_MissionAssetState linkedAsset = state.FindMissionAsset(objective.m_sLinkedRuntimeEntityId);
			if (linkedAsset && linkedAsset.m_sKind != "area")
				continue;

			return true;
		}

		return false;
	}

	protected string MissionMarkerTitle(HST_ActiveMissionState mission)
	{
		if (!mission)
			return "Mission";

		if (mission.m_sMissionId == "convoy_ammo")
			return "Ammo Convoy";
		if (mission.m_sMissionId == "convoy_supplies")
			return "Supply Convoy";
		if (mission.m_sMissionId == "convoy_prisoners")
			return "Prisoner Convoy";
		if (mission.m_sMissionId == "rescue_pows")
			return "POW Rescue";
		if (mission.m_sMissionId == "assassinate_officer")
			return "Kill Officer";
		if (mission.m_sMissionId == "destroy_radio_tower")
			return "Radio Tower";
		if (mission.m_sMissionId == "support_city_supplies")
			return "City Supplies";
		if (mission.m_sMissionId == "logistics_resource_cache")
			return "Resource Cache";

		string title = mission.m_sDisplayName;
		if (title.IsEmpty())
			title = mission.m_sMissionId;
		title = HST_DisplayNameService.ResolveReadableDisplayName(title);
		return ShortMarkerText(title, 22);
	}

	protected string MissionAssetReadableRole(HST_MissionAssetState asset)
	{
		if (!asset)
			return "asset";

		if (asset.m_sRole == "hvt")
			return "officer";
		if (asset.m_sRole == "destroy_target")
			return "target";
		if (asset.m_sRole == "logistics_cargo")
			return "cargo";
		if (asset.m_sRole == "city_supplies")
			return "supplies";
		if (asset.m_sRole == "captive")
			return "captive";
		if (asset.m_sRole == "convoy_vehicle")
			return "vehicle";
		if (asset.m_sRole == "convoy_payload")
			return "payload";
		if (asset.m_sRole == "convoy_captive")
			return "prisoner";

		string role = asset.m_sRole;
		if (role.IsEmpty())
			role = asset.m_sKind;
		if (role.IsEmpty())
			return "asset";
		role.Replace("_", " ");
		return role;
	}

	protected string ShortMarkerText(string text, int maxCharacters)
	{
		if (text.IsEmpty() || maxCharacters <= 0)
			return "";
		if (text.Length() <= maxCharacters)
			return text;
		if (maxCharacters <= 3)
			return text.Substring(0, maxCharacters);

		return text.Substring(0, maxCharacters - 3) + "...";
	}

	protected vector ResolveQRFMarkerPosition(HST_CampaignState state, HST_QRFState qrf, HST_ZoneState targetZone)
	{
		if (!state || !qrf)
			return "0 0 0";

		if (targetZone)
			return targetZone.m_vPosition;

		return state.m_vHQPosition;
	}

	protected string BuildQRFMarkerLabel(HST_CampaignState state, HST_QRFState qrf, HST_ZoneState targetZone)
	{
		if (!qrf)
			return "Enemy QRF";

		string targetName = ResolveZoneDisplayName(targetZone);
		string status = "mobilizing";
		if (!qrf.m_sGroupId.IsEmpty() && state)
		{
			HST_ActiveGroupState group = state.FindActiveGroup(qrf.m_sGroupId);
			if (qrf.m_bResolved && group)
				status = "active";
			else if (group && (group.m_sRuntimeStatus == "arrived" || group.m_sRuntimeStatus == "support_arrived"))
				status = "active";
			else
				status = "nearby";
		}

		int remaining = ResolveQRFRemainingSeconds(state, qrf);
		if (remaining > 0)
			return string.Format("%1 QRF near %2 | %3 | ETA %4s", qrf.m_sFactionKey, targetName, status, remaining);

		return string.Format("%1 QRF near %2 | %3", qrf.m_sFactionKey, targetName, status);
	}

	protected int ResolveQRFRemainingSeconds(HST_CampaignState state, HST_QRFState qrf)
	{
		if (!state || !qrf)
			return 0;

		return Math.Max(0, qrf.m_iStartedAtSecond + qrf.m_iETASeconds - state.m_iElapsedSeconds);
	}

	protected vector ResolveSupportMarkerPosition(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!state || !request)
			return "0 0 0";

		if (!IsZeroVector(request.m_vTargetPosition))
			return request.m_vTargetPosition;

		HST_ZoneState zone = state.FindZone(request.m_sTargetZoneId);
		if (zone)
			return zone.m_vPosition;

		return state.m_vHQPosition;
	}

	protected string BuildSupportMarkerLabel(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!request)
			return "Support";

		string faction = request.m_sFactionKey;
		if (faction.IsEmpty())
			faction = "Enemy";

		string typeLabel = SupportRequestTypeLabel(request.m_eType);
		string targetName = ResolveZoneDisplayNameById(state, request.m_sTargetZoneId);
		string status = SupportStatusLabel(request.m_eStatus);
		int remaining = ResolveSupportRemainingSeconds(state, request);
		if (request.m_eStatus == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
		{
			if (request.m_sGroupId.IsEmpty())
				return string.Format("%1 %2 to %3 | en route | ETA %4s", faction, typeLabel, targetName, remaining);

			HST_ActiveGroupState group;
			if (state)
				group = state.FindActiveGroup(request.m_sGroupId);
			if (group && group.m_sRuntimeStatus == "support_active")
				return string.Format("%1 %2 near %3 | ETA %4s", faction, typeLabel, targetName, remaining);

			return string.Format("%1 %2 at %3 | deployed", faction, typeLabel, targetName);
		}

		return string.Format("%1 %2 to %3 | %4 | ETA %5s", faction, typeLabel, targetName, status, remaining);
	}

	protected int ResolveSupportRemainingSeconds(HST_CampaignState state, HST_SupportRequestState request)
	{
		if (!request)
			return 0;

		if (!state)
			return request.m_iETASeconds;

		return Math.Max(0, request.m_iRequestedAtSecond + request.m_iETASeconds - state.m_iElapsedSeconds);
	}

	protected string SupportStatusLabel(HST_ESupportRequestStatus status)
	{
		if (status == HST_ESupportRequestStatus.HST_SUPPORT_ACTIVE)
			return "active";
		if (status == HST_ESupportRequestStatus.HST_SUPPORT_CANCELLED)
			return "cancelled";
		if (status == HST_ESupportRequestStatus.HST_SUPPORT_RESOLVED)
			return "resolved";

		return "queued";
	}

	protected string SupportRequestTypeLabel(HST_ESupportRequestType supportType)
	{
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_QRF)
			return "QRF";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SEARCH_AND_DESTROY)
			return "search team";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPRESSIVE_FIRE)
			return "fire support";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_TROOP_LANDING)
			return "troop landing";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_GBU || supportType == HST_ESupportRequestType.HST_SUPPORT_AIRSTRIKE_UMPK)
			return "airstrike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_CRUISE_MISSILE_KH55)
			return "missile strike";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_PATROL_SWEEP)
			return "patrol sweep";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_TRANSPORT)
			return "transport";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_SUPPLY_DROP)
			return "supply drop";
		if (supportType == HST_ESupportRequestType.HST_SUPPORT_EVACUATION)
			return "evacuation";

		return "support";
	}

	protected string ResolveZoneDisplayNameById(HST_CampaignState state, string zoneId)
	{
		if (!state || zoneId.IsEmpty())
			return "unknown location";

		HST_ZoneState zone = state.FindZone(zoneId);
		if (zone)
			return ResolveZoneDisplayName(zone);

		string text = HST_DefaultCatalog.GetZoneDisplayName(zoneId);
		if (!text.IsEmpty() && text != zoneId)
			return text;

		text = zoneId;
		text.Replace("_", " ");
		return text;
	}
}
