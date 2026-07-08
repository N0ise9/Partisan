class HST_StrategicEventApplyResult
{
	bool m_bRecorded;
	bool m_bApplied;
	bool m_bChanged;
	string m_sEventId;
	string m_sReason;
	string m_sSummary;
	ref HST_StrategicEventState m_Event;

	string BuildReport()
	{
		if (!m_Event)
			return "h-istasi strategic event | missing event";

		return string.Format(
			"h-istasi strategic event | id %1 | kind %2 | mission %3 | zone %4 | faction %5 | applied %6 | changed %7 | money %8 | HR %9 | support %10 | capture %11 | aggression %12 | attack %13 | supportRes %14 | HQ knowledge %15 | %16",
			m_Event.m_sEventId,
			m_Event.m_sKind,
			m_Event.m_sMissionId,
			m_Event.m_sTargetZoneId,
			m_Event.m_sTargetFactionKey,
			m_bApplied,
			m_bChanged,
			m_Event.m_iFactionMoneyDelta,
			m_Event.m_iHRDelta,
			m_Event.m_iTownSupportDelta,
			m_Event.m_iCaptureProgressDelta,
			m_Event.m_iAggressionDelta,
			m_Event.m_iAttackResourceDelta,
			m_Event.m_iSupportResourceDelta,
			m_Event.m_iHQKnowledgeDelta,
			m_Event.m_sSummary
		);
	}
}

class HST_CampaignOutcomeResult
{
	bool m_bEnded;
	HST_ECampaignPhase m_ePhase;
	string m_sReason;
	string m_sSummary;
	int m_iControlPercent;
	int m_iWarLevel;
	int m_iFIAZones;
	int m_iEnemyZones;
	string m_sOutcomeMode;
	int m_iInitialPopulation;
	int m_iRemainingPopulation;
	int m_iKilledPopulation;
	int m_iFIASupportPopulation;
	int m_iSupportPercent;
	int m_iAirfieldsControlled;
	int m_iAirfieldsTotal;
}

class HST_StrategicService
{
	HST_StrategicEventApplyResult BeginZoneCaptureEvent(HST_CampaignState state, string zoneId, string previousOwner, string newOwner, string sourceId = "")
	{
		HST_StrategicEventApplyResult result = new HST_StrategicEventApplyResult();
		if (!state || zoneId.IsEmpty())
		{
			result.m_sReason = "state or zone id not ready";
			return result;
		}

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
		{
			result.m_sReason = "zone not found";
			return result;
		}

		HST_StrategicEventState eventState = new HST_StrategicEventState();
		eventState.m_sKind = "zone_captured";
		eventState.m_sEventId = BuildStrategicEventId(state, eventState.m_sKind);
		eventState.m_sSourceType = "zone_capture";
		if (sourceId.IsEmpty())
			eventState.m_sSourceId = zoneId;
		else
			eventState.m_sSourceId = sourceId;
		eventState.m_sTargetZoneId = zoneId;
		if (previousOwner.IsEmpty())
			eventState.m_sTargetFactionKey = zone.m_sOwnerFactionKey;
		else
			eventState.m_sTargetFactionKey = previousOwner;
		eventState.m_sReason = string.Format("zone captured by %1", EmptyReportField(newOwner));
		eventState.m_iCreatedAtSecond = state.m_iElapsedSeconds;

		result.m_Event = eventState;
		result.m_sEventId = eventState.m_sEventId;
		result.m_bRecorded = true;
		state.m_aStrategicEvents.Insert(eventState);
		CaptureStrategicEventBefore(state, eventState);
		return result;
	}

	void CompleteStrategicEvent(HST_CampaignState state, HST_StrategicEventApplyResult result, bool applied, bool changed)
	{
		if (!state || !result || !result.m_Event)
			return;

		HST_StrategicEventState eventState = result.m_Event;
		RefreshStrategicEventAfter(state, eventState);
		eventState.m_bApplied = applied;
		eventState.m_sSummary = BuildStrategicEventSummary(eventState);
		result.m_bApplied = applied;
		result.m_bChanged = changed || HasStrategicEventDelta(eventState);
		result.m_sSummary = eventState.m_sSummary;
	}

	void DiscardStrategicEvent(HST_CampaignState state, HST_StrategicEventApplyResult result)
	{
		if (!state || !result || !result.m_Event)
			return;

		for (int i = state.m_aStrategicEvents.Count() - 1; i >= 0; i--)
		{
			HST_StrategicEventState eventState = state.m_aStrategicEvents[i];
			if (eventState && eventState.m_sEventId == result.m_Event.m_sEventId)
			{
				state.m_aStrategicEvents.Remove(i);
				break;
			}
		}

		result.m_bRecorded = false;
		result.m_bApplied = false;
		result.m_bChanged = false;
		result.m_sReason = "discarded";
	}

	HST_StrategicEventApplyResult ApplyMissionOutcomeEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_BalanceConfig balance, HST_TownService towns, HST_ZoneCaptureService zoneCapture, HST_GarrisonService garrisons, HST_EnemyCommanderService enemyCommander, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService supportRequests, HST_HQService hq, HST_MissionDefinition definition, HST_ActiveMissionState activeMission, bool succeeded, bool applyDefinitionRewards = true)
	{
		HST_StrategicEventApplyResult result = new HST_StrategicEventApplyResult();
		if (!state || !preset || !economy || !definition || !activeMission)
		{
			result.m_sReason = "state/preset/economy/mission not ready";
			return result;
		}

		HST_StrategicEventState eventState = CreateMissionOutcomeEvent(state, preset, definition, activeMission, succeeded);
		result.m_Event = eventState;
		result.m_sEventId = eventState.m_sEventId;
		result.m_bRecorded = true;
		state.m_aStrategicEvents.Insert(eventState);
		CaptureStrategicEventBefore(state, eventState);

		bool changed;
		if (succeeded)
			changed = ApplyMissionSuccessEvent(state, preset, economy, balance, towns, zoneCapture, garrisons, enemyCommander, enemyDirector, supportRequests, hq, definition, activeMission, applyDefinitionRewards);
		else
			changed = ApplyMissionFailureEvent(state, preset, economy, towns, hq, definition, activeMission);

		RefreshStrategicEventAfter(state, eventState);
		eventState.m_bApplied = true;
		eventState.m_sSummary = BuildStrategicEventSummary(eventState);
		result.m_bApplied = true;
		result.m_bChanged = changed || HasStrategicEventDelta(eventState);
		result.m_sSummary = eventState.m_sSummary;
		return result;
	}

	HST_StrategicEventApplyResult ApplyMissionExpiryEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_MissionDefinition definition, HST_ActiveMissionState activeMission)
	{
		HST_StrategicEventApplyResult result = new HST_StrategicEventApplyResult();
		if (!state || !preset || !economy || !definition || !activeMission)
		{
			result.m_sReason = "state/preset/economy/mission not ready";
			return result;
		}

		if (definition.m_sMissionId == "dynamic_defend_petros")
		{
			result.m_sReason = "defense expiry resolves as success";
			return result;
		}

		HST_StrategicEventState existingEvent = FindMissionStrategicEvent(state, activeMission.m_sInstanceId, "mission_expired");
		if (existingEvent)
		{
			result.m_Event = existingEvent;
			result.m_sEventId = existingEvent.m_sEventId;
			result.m_bRecorded = true;
			result.m_sReason = "mission expiry already recorded";
			result.m_sSummary = existingEvent.m_sSummary;
			return result;
		}

		HST_StrategicEventState eventState = CreateMissionExpiryEvent(state, preset, definition, activeMission);
		result.m_Event = eventState;
		result.m_sEventId = eventState.m_sEventId;
		result.m_bRecorded = true;
		state.m_aStrategicEvents.Insert(eventState);
		CaptureStrategicEventBefore(state, eventState);

		bool changed = ApplyMissionExpiryConsequences(state, preset, economy, definition);

		RefreshStrategicEventAfter(state, eventState);
		eventState.m_bApplied = true;
		eventState.m_sSummary = BuildStrategicEventSummary(eventState);
		result.m_bApplied = true;
		result.m_bChanged = changed || HasStrategicEventDelta(eventState);
		result.m_sSummary = eventState.m_sSummary;
		return result;
	}

	HST_StrategicEventApplyResult BeginConvoyOutcomeEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState mission, string kind, string targetZoneId = "", string sourceId = "", string targetFactionKey = "")
	{
		HST_StrategicEventApplyResult result = new HST_StrategicEventApplyResult();
		if (!state || !mission)
		{
			result.m_sReason = "state or convoy mission not ready";
			return result;
		}

		HST_StrategicEventState eventState = new HST_StrategicEventState();
		if (kind.IsEmpty())
			eventState.m_sKind = "convoy_outcome";
		else
			eventState.m_sKind = kind;
		eventState.m_sEventId = BuildStrategicEventId(state, eventState.m_sKind);
		eventState.m_sSourceType = "convoy_outcome";
		if (sourceId.IsEmpty())
			eventState.m_sSourceId = mission.m_sInstanceId;
		else
			eventState.m_sSourceId = sourceId;
		eventState.m_sMissionId = mission.m_sMissionId;
		eventState.m_sMissionInstanceId = mission.m_sInstanceId;
		if (targetZoneId.IsEmpty())
			eventState.m_sTargetZoneId = mission.m_sTargetZoneId;
		else
			eventState.m_sTargetZoneId = targetZoneId;
		eventState.m_sTargetFactionKey = ResolveStrategicEventTargetFactionForZone(state, preset, eventState.m_sTargetZoneId, targetFactionKey);
		eventState.m_sReason = eventState.m_sKind + ": " + mission.m_sMissionId;
		eventState.m_iCreatedAtSecond = state.m_iElapsedSeconds;

		result.m_Event = eventState;
		result.m_sEventId = eventState.m_sEventId;
		result.m_bRecorded = true;
		state.m_aStrategicEvents.Insert(eventState);
		CaptureStrategicEventBefore(state, eventState);
		return result;
	}

	HST_StrategicEventApplyResult BeginSupportNearHQEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_SupportRequestState request)
	{
		HST_StrategicEventApplyResult result = new HST_StrategicEventApplyResult();
		if (!state || !request)
		{
			result.m_sReason = "state or support request not ready";
			return result;
		}

		HST_StrategicEventState eventState = new HST_StrategicEventState();
		eventState.m_sKind = "support_near_hq";
		eventState.m_sEventId = BuildStrategicEventId(state, eventState.m_sKind);
		eventState.m_sSourceType = "support_request";
		eventState.m_sSourceId = request.m_sRequestId;
		eventState.m_sTargetZoneId = request.m_sTargetZoneId;
		eventState.m_sTargetFactionKey = request.m_sFactionKey;
		eventState.m_sReason = "hostile support near HQ: " + request.m_sRequestId;
		eventState.m_iCreatedAtSecond = state.m_iElapsedSeconds;

		result.m_Event = eventState;
		result.m_sEventId = eventState.m_sEventId;
		result.m_bRecorded = true;
		state.m_aStrategicEvents.Insert(eventState);
		CaptureStrategicEventBefore(state, eventState);
		return result;
	}

	bool SetZoneOwner(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, string factionKey, string resistanceFactionKey = "FIA")
	{
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone || !state.FindFactionPool(factionKey) || zone.m_sOwnerFactionKey == factionKey)
			return false;

		string previousOwner = zone.m_sOwnerFactionKey;
		zone.m_sOwnerFactionKey = factionKey;
		zone.m_iResistanceCaptureProgress = 0;
		zone.m_iActiveInfantryCount = 0;
		zone.m_iActiveVehicleCount = 0;
		ClearZoneGarrison(state, zoneId, previousOwner);
		economy.RecalculateWarLevel(state, balance, resistanceFactionKey);
		EvaluateCampaignOutcome(state, economy, balance, resistanceFactionKey);
		return true;
	}

	bool AddTownSupport(HST_CampaignState state, string zoneId, int amount)
	{
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return false;

		zone.m_iSupport = Math.Max(-100, Math.Min(100, zone.m_iSupport + amount));
		return true;
	}

	bool SetZoneActive(HST_CampaignState state, string zoneId, bool active)
	{
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return false;

		zone.m_bActive = active;
		return true;
	}

	void OnPetrosKilled(HST_CampaignState state)
	{
		state.m_bPetrosAlive = false;
		state.m_iFactionMoney = Math.Max(0, state.m_iFactionMoney / 2);
		state.m_iHR = Math.Max(0, state.m_iHR / 2);
	}

	protected bool ApplyMissionSuccessEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_BalanceConfig balance, HST_TownService towns, HST_ZoneCaptureService zoneCapture, HST_GarrisonService garrisons, HST_EnemyCommanderService enemyCommander, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService supportRequests, HST_HQService hq, HST_MissionDefinition definition, HST_ActiveMissionState activeMission, bool applyDefinitionRewards)
	{
		bool changed;
		if (applyDefinitionRewards)
		{
			int moneyBefore = state.m_iFactionMoney;
			int hrBefore = state.m_iHR;
			economy.AddFactionMoney(state, definition.m_iRewardMoney);
			economy.AddHR(state, definition.m_iRewardHR);
			changed = changed || state.m_iFactionMoney != moneyBefore || state.m_iHR != hrBefore;
		}

		if (activeMission.m_sTargetZoneId.IsEmpty())
			return changed;

		if (definition.m_sMissionId == "dynamic_defend_petros")
			return changed;

		HST_ZoneState zone = state.FindZone(activeMission.m_sTargetZoneId);
		if (!zone)
			return changed;

		if (zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
		{
			int aggressionBefore = ResolveFactionAggression(state, zone.m_sOwnerFactionKey);
			economy.AddAggression(state, zone.m_sOwnerFactionKey, ResolveMissionSuccessAggression(definition));
			changed = changed || ResolveFactionAggression(state, zone.m_sOwnerFactionKey) != aggressionBefore;
		}

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
		{
			if (zoneCapture && zoneCapture.AddResistanceCaptureProgress(state, preset, this, economy, balance, zone.m_sZoneId, 60, 15, garrisons, enemyCommander, enemyDirector, supportRequests))
				changed = true;
			return changed;
		}

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT)
		{
			if (towns && towns.AddSupport(state, zone.m_sZoneId, 25))
				changed = true;
			return changed;
		}

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
		{
			if (enemyDirector)
				enemyDirector.AddResources(state, zone.m_sOwnerFactionKey, -12, -6);
			if ((definition.m_sMissionId == "destroy_radio_tower" || definition.m_sMissionId == "dynamic_stop_tower_rebuild") && hq)
				changed = hq.ReduceHQKnowledge(state, 20, "mission success: " + definition.m_sMissionId) || changed;
			if (zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey && zoneCapture)
				changed = zoneCapture.AddResistanceCaptureProgress(state, preset, this, economy, balance, zone.m_sZoneId, 35, 10, garrisons, enemyCommander, enemyDirector, supportRequests) || changed;
			return true;
		}

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY || definition.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
		{
			if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			{
				economy.AddFactionMoney(state, 150);
				economy.AddHR(state, 1);
			}
			if (enemyDirector)
				enemyDirector.AddResources(state, zone.m_sOwnerFactionKey, -8, -4);
			return true;
		}

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DYNAMIC)
		{
			if (definition.m_sMissionId == "dynamic_city_flip_battle")
			{
				if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
				{
					if (towns && towns.AddSupport(state, zone.m_sZoneId, 25))
						changed = true;
					return changed;
				}

				if (zoneCapture && zoneCapture.AddResistanceCaptureProgress(state, preset, this, economy, balance, zone.m_sZoneId, 50, 10, garrisons, enemyCommander, enemyDirector, supportRequests))
					changed = true;
				return changed;
			}

			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			{
				if (towns && towns.AddSupport(state, zone.m_sZoneId, 10))
					changed = true;
				return changed;
			}

			if (zoneCapture && zoneCapture.AddResistanceCaptureProgress(state, preset, this, economy, balance, zone.m_sZoneId, 20, 5, garrisons, enemyCommander, enemyDirector, supportRequests))
				changed = true;
			return changed;
		}

		return changed;
	}

	protected bool ApplyMissionFailureEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_TownService towns, HST_HQService hq, HST_MissionDefinition definition, HST_ActiveMissionState activeMission)
	{
		bool changed;
		int occupierAggressionBefore = ResolveFactionAggression(state, preset.m_sOccupierFactionKey);
		economy.AddAggression(state, preset.m_sOccupierFactionKey, definition.m_iFailureAggression);
		changed = changed || ResolveFactionAggression(state, preset.m_sOccupierFactionKey) != occupierAggressionBefore;

		if (definition.m_sMissionId == "assassinate_traitor" && hq)
			changed = hq.AddHQKnowledge(state, 35, "traitor escaped / failed assassination") || changed;

		if (definition.m_sMissionId == "dynamic_defend_petros")
		{
			if (!state.m_bDefendPetrosOutcomeApplied && hq)
				changed = hq.AddHQKnowledge(state, 25, "Defend Petros failed") || changed;
			if (state.m_iLastHQAttackSecond != state.m_iElapsedSeconds)
			{
				state.m_iLastHQAttackSecond = state.m_iElapsedSeconds;
				changed = true;
			}
			return changed;
		}

		if (definition.m_iFailureAggression >= 4 && hq)
			changed = hq.AddHQKnowledge(state, 8 + definition.m_iFailureAggression, "high aggression mission failure: " + definition.m_sMissionId) || changed;

		if (changed && definition.m_iFailureAggression >= 4)
			return true;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT && !activeMission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState zone = state.FindZone(activeMission.m_sTargetZoneId);
			if (zone && towns)
			{
				if (towns.AddSupport(state, zone.m_sZoneId, -10))
					changed = true;
				return changed;
			}
		}

		if (!activeMission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState targetZone = state.FindZone(activeMission.m_sTargetZoneId);
			if (targetZone && targetZone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey)
			{
				int targetAggressionBefore = ResolveFactionAggression(state, targetZone.m_sOwnerFactionKey);
				economy.AddAggression(state, targetZone.m_sOwnerFactionKey, Math.Max(1, definition.m_iFailureAggression / 2));
				return ResolveFactionAggression(state, targetZone.m_sOwnerFactionKey) != targetAggressionBefore || changed;
			}
		}

		return changed;
	}

	protected bool ApplyMissionExpiryConsequences(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_MissionDefinition definition)
	{
		if (definition.m_sMissionId == "dynamic_defend_petros")
			return false;

		int occupierAggressionBefore = ResolveFactionAggression(state, preset.m_sOccupierFactionKey);
		economy.AddAggression(state, preset.m_sOccupierFactionKey, definition.m_iFailureAggression);
		return ResolveFactionAggression(state, preset.m_sOccupierFactionKey) != occupierAggressionBefore;
	}

	protected HST_StrategicEventState CreateMissionOutcomeEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionDefinition definition, HST_ActiveMissionState activeMission, bool succeeded)
	{
		HST_StrategicEventState eventState = new HST_StrategicEventState();
		if (succeeded)
			eventState.m_sKind = "mission_success";
		else
			eventState.m_sKind = "mission_failure";
		eventState.m_sEventId = BuildStrategicEventId(state, eventState.m_sKind);
		eventState.m_sSourceType = "mission";
		eventState.m_sSourceId = activeMission.m_sInstanceId;
		eventState.m_sMissionId = definition.m_sMissionId;
		eventState.m_sMissionInstanceId = activeMission.m_sInstanceId;
		eventState.m_sTargetZoneId = activeMission.m_sTargetZoneId;
		eventState.m_sTargetFactionKey = ResolveStrategicEventTargetFaction(state, preset, activeMission, succeeded);
		eventState.m_sReason = eventState.m_sKind + ": " + definition.m_sMissionId;
		eventState.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		return eventState;
	}

	protected HST_StrategicEventState CreateMissionExpiryEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionDefinition definition, HST_ActiveMissionState activeMission)
	{
		HST_StrategicEventState eventState = new HST_StrategicEventState();
		eventState.m_sKind = "mission_expired";
		eventState.m_sEventId = BuildStrategicEventId(state, eventState.m_sKind);
		eventState.m_sSourceType = "mission";
		eventState.m_sSourceId = activeMission.m_sInstanceId;
		eventState.m_sMissionId = definition.m_sMissionId;
		eventState.m_sMissionInstanceId = activeMission.m_sInstanceId;
		eventState.m_sTargetZoneId = activeMission.m_sTargetZoneId;
		eventState.m_sTargetFactionKey = preset.m_sOccupierFactionKey;
		eventState.m_sReason = eventState.m_sKind + ": " + definition.m_sMissionId;
		eventState.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		return eventState;
	}

	protected string ResolveStrategicEventTargetFaction(HST_CampaignState state, HST_CampaignPreset preset, HST_ActiveMissionState activeMission, bool succeeded)
	{
		if (!state || !activeMission)
			return "";

		if (!succeeded && preset)
			return preset.m_sOccupierFactionKey;

		HST_ZoneState zone = state.FindZone(activeMission.m_sTargetZoneId);
		if (zone)
			return zone.m_sOwnerFactionKey;

		if (preset)
			return preset.m_sResistanceFactionKey;

		return "";
	}

	protected string ResolveStrategicEventTargetFactionForZone(HST_CampaignState state, HST_CampaignPreset preset, string zoneId, string targetFactionKey)
	{
		if (!targetFactionKey.IsEmpty())
			return targetFactionKey;

		if (state)
		{
			HST_ZoneState zone = state.FindZone(zoneId);
			if (zone && !zone.m_sOwnerFactionKey.IsEmpty())
				return zone.m_sOwnerFactionKey;
		}

		if (preset)
			return preset.m_sResistanceFactionKey;

		return "";
	}

	protected HST_StrategicEventState FindMissionStrategicEvent(HST_CampaignState state, string missionInstanceId, string kind)
	{
		if (!state || missionInstanceId.IsEmpty())
			return null;

		foreach (HST_StrategicEventState eventState : state.m_aStrategicEvents)
		{
			if (!eventState || eventState.m_sMissionInstanceId != missionInstanceId)
				continue;
			if (!kind.IsEmpty() && eventState.m_sKind != kind)
				continue;

			return eventState;
		}

		return null;
	}

	protected void CaptureStrategicEventBefore(HST_CampaignState state, HST_StrategicEventState eventState)
	{
		if (!state || !eventState)
			return;

		eventState.m_iFactionMoneyDelta = -state.m_iFactionMoney;
		eventState.m_iHRDelta = -state.m_iHR;
		eventState.m_iHQKnowledgeBefore = state.m_iHQKnowledge;
		eventState.m_iHQKnowledgeDelta = -state.m_iHQKnowledge;

		HST_ZoneState zone = state.FindZone(eventState.m_sTargetZoneId);
		if (zone)
		{
			eventState.m_sOwnerBefore = zone.m_sOwnerFactionKey;
			eventState.m_iSupportBefore = zone.m_iSupport;
			eventState.m_iSupportAfter = zone.m_iSupport;
			eventState.m_iTownSupportDelta = -zone.m_iSupport;
			eventState.m_iCaptureProgressBefore = zone.m_iResistanceCaptureProgress;
			eventState.m_iCaptureProgressAfter = zone.m_iResistanceCaptureProgress;
			eventState.m_iCaptureProgressDelta = -zone.m_iResistanceCaptureProgress;
		}

		HST_FactionPoolState pool = state.FindFactionPool(eventState.m_sTargetFactionKey);
		if (pool)
		{
			eventState.m_iAggressionDelta = -pool.m_iAggression;
			eventState.m_iAttackResourceDelta = -pool.m_iAttackResources;
			eventState.m_iSupportResourceDelta = -pool.m_iSupportResources;
		}
	}

	protected void RefreshStrategicEventAfter(HST_CampaignState state, HST_StrategicEventState eventState)
	{
		if (!state || !eventState)
			return;

		eventState.m_iFactionMoneyDelta += state.m_iFactionMoney;
		eventState.m_iHRDelta += state.m_iHR;
		eventState.m_iHQKnowledgeAfter = state.m_iHQKnowledge;
		eventState.m_iHQKnowledgeDelta += state.m_iHQKnowledge;

		HST_ZoneState zone = state.FindZone(eventState.m_sTargetZoneId);
		if (zone)
		{
			eventState.m_sOwnerAfter = zone.m_sOwnerFactionKey;
			eventState.m_iSupportAfter = zone.m_iSupport;
			eventState.m_iTownSupportDelta += zone.m_iSupport;
			eventState.m_iCaptureProgressAfter = zone.m_iResistanceCaptureProgress;
			eventState.m_iCaptureProgressDelta += zone.m_iResistanceCaptureProgress;
		}

		HST_FactionPoolState pool = state.FindFactionPool(eventState.m_sTargetFactionKey);
		if (pool)
		{
			eventState.m_iAggressionDelta += pool.m_iAggression;
			eventState.m_iAttackResourceDelta += pool.m_iAttackResources;
			eventState.m_iSupportResourceDelta += pool.m_iSupportResources;
		}
	}

	protected bool HasStrategicEventDelta(HST_StrategicEventState eventState)
	{
		if (!eventState)
			return false;

		return eventState.m_iFactionMoneyDelta != 0
			|| eventState.m_iHRDelta != 0
			|| eventState.m_iAggressionDelta != 0
			|| eventState.m_iAttackResourceDelta != 0
			|| eventState.m_iSupportResourceDelta != 0
			|| eventState.m_iTownSupportDelta != 0
			|| eventState.m_iCaptureProgressDelta != 0
			|| eventState.m_iHQKnowledgeDelta != 0
			|| eventState.m_sOwnerBefore != eventState.m_sOwnerAfter;
	}

	protected string BuildStrategicEventSummary(HST_StrategicEventState eventState)
	{
		if (!eventState)
			return "";

		return string.Format(
			"%1 | money %2 HR %3 | support %4 | capture %5 | aggression %6 | resources %7/%8 | HQ %9 | owner %10 -> %11",
			eventState.m_sReason,
			eventState.m_iFactionMoneyDelta,
			eventState.m_iHRDelta,
			eventState.m_iTownSupportDelta,
			eventState.m_iCaptureProgressDelta,
			eventState.m_iAggressionDelta,
			eventState.m_iAttackResourceDelta,
			eventState.m_iSupportResourceDelta,
			eventState.m_iHQKnowledgeDelta,
			EmptyReportField(eventState.m_sOwnerBefore),
			EmptyReportField(eventState.m_sOwnerAfter)
		);
	}

	string BuildStrategicEventReport(HST_CampaignState state, int maxRows = 20)
	{
		if (!state)
			return "h-istasi strategic events | state not ready";

		string report = string.Format("h-istasi strategic events | total %1 | showing %2", state.m_aStrategicEvents.Count(), maxRows);
		int emitted;
		for (int i = state.m_aStrategicEvents.Count() - 1; i >= 0; i--)
		{
			if (emitted >= maxRows)
				break;

			HST_StrategicEventState eventState = state.m_aStrategicEvents[i];
			if (!eventState)
				continue;

			report = report + string.Format(
				"\n%1 | %2 | mission %3/%4 | zone %5 | faction %6 | applied %7 | money %8 HR %9 support %10 capture %11 aggression %12 resources %13/%14 HQ %15 | %16",
				eventState.m_sEventId,
				eventState.m_sKind,
				EmptyReportField(eventState.m_sMissionId),
				EmptyReportField(eventState.m_sMissionInstanceId),
				EmptyReportField(eventState.m_sTargetZoneId),
				EmptyReportField(eventState.m_sTargetFactionKey),
				eventState.m_bApplied,
				eventState.m_iFactionMoneyDelta,
				eventState.m_iHRDelta,
				eventState.m_iTownSupportDelta,
				eventState.m_iCaptureProgressDelta,
				eventState.m_iAggressionDelta,
				eventState.m_iAttackResourceDelta,
				eventState.m_iSupportResourceDelta,
				eventState.m_iHQKnowledgeDelta,
				eventState.m_sSummary
			);
			emitted++;
		}

		return report;
	}

	protected string BuildStrategicEventId(HST_CampaignState state, string kind)
	{
		string safeKind = kind;
		if (safeKind.IsEmpty())
			safeKind = "event";

		return string.Format("strategic_%1_%2_%3", safeKind, state.m_iElapsedSeconds, state.m_aStrategicEvents.Count());
	}

	protected int ResolveFactionAggression(HST_CampaignState state, string factionKey)
	{
		if (!state || factionKey.IsEmpty())
			return 0;

		HST_FactionPoolState pool = state.FindFactionPool(factionKey);
		if (!pool)
			return 0;

		return pool.m_iAggression;
	}

	protected int ResolveMissionSuccessAggression(HST_MissionDefinition definition)
	{
		if (!definition)
			return 1;

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return 8;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return 6;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			return 5;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DYNAMIC)
			return 4;

		return 2;
	}

	HST_CampaignOutcomeResult EvaluateCampaignOutcomeDetailed(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, string resistanceFactionKey)
	{
		HST_CampaignOutcomeResult result = new HST_CampaignOutcomeResult();
		if (!state || !economy || !balance || state.m_ePhase != HST_ECampaignPhase.HST_CAMPAIGN_ACTIVE)
			return result;

		int fiaZones = CountZonesOwnedBy(state, resistanceFactionKey);
		int enemyZones = CountZonesNotOwnedBy(state, resistanceFactionKey);
		int score = economy.CalculateResistanceStrategicScore(state, resistanceFactionKey);
		int totalScore = CalculateTotalStrategicScoreForOutcome(state, economy);
		int controlPercent;
		if (totalScore > 0)
			controlPercent = Math.Round(score * 100.0 / totalScore);

		result.m_iControlPercent = controlPercent;
		result.m_iWarLevel = state.m_iWarLevel;
		result.m_iFIAZones = fiaZones;
		result.m_iEnemyZones = enemyZones;
		result.m_iInitialPopulation = GetTotalInitialPopulation(state);
		result.m_iRemainingPopulation = GetTotalRemainingPopulation(state);
		result.m_iKilledPopulation = GetTotalKilledPopulation(state);
		result.m_iFIASupportPopulation = GetTotalFIASupportPopulation(state);
		result.m_iSupportPercent = CalculatePopulationSupportPercent(result.m_iFIASupportPopulation, result.m_iRemainingPopulation);
		result.m_iAirfieldsTotal = CountZonesOfType(state, HST_EZoneType.HST_ZONE_AIRFIELD);
		result.m_iAirfieldsControlled = CountTypeOwnedBy(state, HST_EZoneType.HST_ZONE_AIRFIELD, resistanceFactionKey);
		result.m_sOutcomeMode = ResolveOutcomeMode(balance);

		if (!IsCampaignOutcomeCheckDue(state, balance))
			return result;

		if (balance.m_bPopulationOutcomeEnabled)
		{
			if (ShouldLosePopulationCampaign(result, balance))
			{
				result.m_bEnded = true;
				result.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_LOST;
				result.m_sReason = "loss: civilian catastrophe";
				result.m_sSummary = string.Format("Civilian killed population %1/%2 exceeded one third of initial population.", result.m_iKilledPopulation, result.m_iInitialPopulation);
				return result;
			}

			bool populationSupportReady = result.m_iRemainingPopulation > 0 && result.m_iFIASupportPopulation * 100 >= result.m_iRemainingPopulation * balance.m_iVictoryPopulationSupportPercent;
			bool populationAirfieldsReady = !balance.m_bVictoryRequiresAirfields || AreAllAirfieldsControlledBy(state, resistanceFactionKey);
			if (populationSupportReady && populationAirfieldsReady)
			{
				result.m_bEnded = true;
				result.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_WON;
				result.m_sReason = "victory: population support and airfield control";
				result.m_sSummary = string.Format("FIA support population %1/%2 (%3 pct) reached the %4 pct threshold with airfields %5/%6 controlled.", result.m_iFIASupportPopulation, result.m_iRemainingPopulation, result.m_iSupportPercent, balance.m_iVictoryPopulationSupportPercent, result.m_iAirfieldsControlled, result.m_iAirfieldsTotal);
				return result;
			}

			return result;
		}

		bool airfieldsReady = !balance.m_bVictoryRequiresAirfields || AreAllAirfieldsControlledBy(state, resistanceFactionKey);
		bool seaportsReady = !balance.m_bVictoryRequiresSeaports || AreAllTypeOwnedBy(state, HST_EZoneType.HST_ZONE_SEAPORT, resistanceFactionKey);
		bool controlReady = controlPercent >= balance.m_iVictoryControlPercent;
		if (balance.m_bLegacyControlVictoryEnabled && controlReady && airfieldsReady && seaportsReady)
		{
			result.m_bEnded = true;
			result.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_WON;
			result.m_sReason = "victory: strategic control achieved";
			result.m_sSummary = string.Format("FIA controls %1 percent of strategic score with required airfields %2 and seaports %3.", controlPercent, airfieldsReady, seaportsReady);
			return result;
		}

		if (ShouldLoseCampaign(state, balance, resistanceFactionKey))
		{
			result.m_bEnded = true;
			result.m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_LOST;
			result.m_sReason = "loss: FIA collapse";
			result.m_sSummary = string.Format("FIA collapsed with money %1 HR %2 Petros deaths %3.", state.m_iFactionMoney, state.m_iHR, state.m_iPetrosDeaths);
			return result;
		}

		return result;
	}

	void ApplyCampaignOutcome(HST_CampaignState state, HST_CampaignOutcomeResult outcome)
	{
		if (!state || !outcome || !outcome.m_bEnded)
			return;

		state.m_ePhase = outcome.m_ePhase;
		state.m_sCampaignEndReason = outcome.m_sReason;
		state.m_sCampaignEndSummary = outcome.m_sSummary;
		state.m_iCampaignEndedAtSecond = state.m_iElapsedSeconds;
		state.m_iCampaignEndControlPercent = outcome.m_iControlPercent;
		state.m_iCampaignEndWarLevel = outcome.m_iWarLevel;
		state.m_iCampaignEndFIAZones = outcome.m_iFIAZones;
		state.m_iCampaignEndEnemyZones = outcome.m_iEnemyZones;
		state.m_sCampaignEndOutcomeMode = outcome.m_sOutcomeMode;
		state.m_iCampaignEndInitialPopulation = outcome.m_iInitialPopulation;
		state.m_iCampaignEndRemainingPopulation = outcome.m_iRemainingPopulation;
		state.m_iCampaignEndKilledPopulation = outcome.m_iKilledPopulation;
		state.m_iCampaignEndFIASupportPopulation = outcome.m_iFIASupportPopulation;
		state.m_iCampaignEndSupportPercent = outcome.m_iSupportPercent;
		state.m_iCampaignEndAirfieldsControlled = outcome.m_iAirfieldsControlled;
		state.m_iCampaignEndAirfieldsTotal = outcome.m_iAirfieldsTotal;
		state.m_bCampaignEndReportGenerated = true;
	}

	string BuildCampaignEndReport(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, HST_CampaignPreset preset)
	{
		if (!state)
			return "h-istasi campaign end | state not ready";

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		string phase = string.Format("%1", state.m_ePhase);
		int score;
		int totalScore;
		int controlPercent;
		if (economy)
		{
			score = economy.CalculateResistanceStrategicScore(state, resistanceFactionKey);
			totalScore = CalculateTotalStrategicScoreForOutcome(state, economy);
			if (totalScore > 0)
				controlPercent = Math.Round(score * 100.0 / totalScore);
		}

		int initialPopulation = GetTotalInitialPopulation(state);
		int remainingPopulation = GetTotalRemainingPopulation(state);
		int killedPopulation = GetTotalKilledPopulation(state);
		int fiaSupportPopulation = GetTotalFIASupportPopulation(state);
		int supportPercent = CalculatePopulationSupportPercent(fiaSupportPopulation, remainingPopulation);
		int killedPercent;
		if (initialPopulation > 0)
			killedPercent = Math.Round(killedPopulation * 100.0 / initialPopulation);
		int airfieldsTotal = CountZonesOfType(state, HST_EZoneType.HST_ZONE_AIRFIELD);
		int airfieldsControlled = CountTypeOwnedBy(state, HST_EZoneType.HST_ZONE_AIRFIELD, resistanceFactionKey);
		int nextOutcomeCheckSecond = GetNextOutcomeCheckSecond(state, balance);
		string outcomeMode = "population";
		bool airfieldsRequired;
		if (balance)
		{
			outcomeMode = ResolveOutcomeMode(balance);
			airfieldsRequired = balance.m_bVictoryRequiresAirfields;
		}

		string report = string.Format(
			"h-istasi campaign end | phase %1 | ended %2 | reason %3 | summary %4 | control %5 pct | score %6/%7 | war %8 | FIA zones %9",
			phase,
			state.m_iCampaignEndedAtSecond,
			state.m_sCampaignEndReason,
			state.m_sCampaignEndSummary,
			controlPercent,
			score,
			totalScore,
			state.m_iWarLevel,
			CountZonesOwnedBy(state, resistanceFactionKey)
		);
		report = report + string.Format(" | enemy zones %1", CountZonesNotOwnedBy(state, resistanceFactionKey));
		report = report + string.Format("\noutcome | mode %1 | next check second %2", outcomeMode, nextOutcomeCheckSecond);
		report = report + string.Format("\npopulation | initial %1 | remaining %2 | killed %3 (%4 pct) | FIA support population %5 (%6 pct of remaining)", initialPopulation, remainingPopulation, killedPopulation, killedPercent, fiaSupportPopulation, supportPercent);
		report = report + string.Format("\nairfields | controlled %1/%2 | required %3", airfieldsControlled, airfieldsTotal, airfieldsRequired);

		if (state.m_bCampaignEndReportGenerated)
		{
			report = report + string.Format(
				"\nfinal persisted | control %1 pct | war %2 | FIA zones %3 | enemy zones %4 | mode %5",
				state.m_iCampaignEndControlPercent,
				state.m_iCampaignEndWarLevel,
				state.m_iCampaignEndFIAZones,
				state.m_iCampaignEndEnemyZones,
				EmptyReportField(state.m_sCampaignEndOutcomeMode)
			);
			report = report + string.Format("\nfinal population | initial %1 | remaining %2 | killed %3 | FIA support %4 (%5 pct) | airfields %6/%7", state.m_iCampaignEndInitialPopulation, state.m_iCampaignEndRemainingPopulation, state.m_iCampaignEndKilledPopulation, state.m_iCampaignEndFIASupportPopulation, state.m_iCampaignEndSupportPercent, state.m_iCampaignEndAirfieldsControlled, state.m_iCampaignEndAirfieldsTotal);
		}

		if (balance)
		{
			report = report + string.Format(
				"\nrequirements | population outcome %1 | support %2 pct | legacy control enabled %3 | legacy victory %4 pct | airfields %5 | seaports %6 | loss enabled %7 | Petros death limit %8 | grace %9s",
				balance.m_bPopulationOutcomeEnabled,
				balance.m_iVictoryPopulationSupportPercent,
				balance.m_bLegacyControlVictoryEnabled,
				balance.m_iVictoryControlPercent,
				balance.m_bVictoryRequiresAirfields,
				balance.m_bVictoryRequiresSeaports,
				balance.m_bLossConditionEnabled,
				balance.m_iLossPetrosDeathLimit,
				balance.m_iLossGraceSeconds
			);
		}

		return report;
	}

	int CountZonesOwnedBy(HST_CampaignState state, string factionKey)
	{
		int count;
		if (!state)
			return count;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (IsStrategicZoneCounted(zone) && zone.m_sOwnerFactionKey == factionKey)
				count++;
		}

		return count;
	}

	int CountZonesNotOwnedBy(HST_CampaignState state, string factionKey)
	{
		int count;
		if (!state)
			return count;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (IsStrategicZoneCounted(zone) && zone.m_sOwnerFactionKey != factionKey)
				count++;
		}

		return count;
	}

	int CountZonesOfType(HST_CampaignState state, HST_EZoneType zoneType)
	{
		int count;
		if (!state)
			return count;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_eType == zoneType)
				count++;
		}

		return count;
	}

	int CountTypeOwnedBy(HST_CampaignState state, HST_EZoneType zoneType, string factionKey)
	{
		int count;
		if (!state)
			return count;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_eType == zoneType && zone.m_sOwnerFactionKey == factionKey)
				count++;
		}

		return count;
	}

	int GetTotalInitialPopulation(HST_CampaignState state)
	{
		int total;
		if (!state)
			return total;

		foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
		{
			if (!town)
				continue;

			total += Math.Max(0, town.m_iPopulationRemaining) + Math.Max(0, town.m_iPopulationKilled);
		}

		return total;
	}

	int GetTotalRemainingPopulation(HST_CampaignState state)
	{
		int total;
		if (!state)
			return total;

		foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
		{
			if (town)
				total += Math.Max(0, town.m_iPopulationRemaining);
		}

		return total;
	}

	int GetTotalKilledPopulation(HST_CampaignState state)
	{
		int total;
		if (!state)
			return total;

		foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
		{
			if (town)
				total += Math.Max(0, town.m_iPopulationKilled);
		}

		return total;
	}

	int GetTotalFIASupportPopulation(HST_CampaignState state)
	{
		int total;
		if (!state)
			return total;

		foreach (HST_CivilianZoneState town : state.m_aCivilianZones)
		{
			if (!town)
				continue;

			int remaining = Math.Max(0, town.m_iPopulationRemaining);
			int support = Math.Max(0, Math.Min(100, town.m_iFIASupport));
			total += Math.Round(remaining * support / 100.0);
		}

		return total;
	}

	bool AreAllAirfieldsControlledBy(HST_CampaignState state, string factionKey)
	{
		int total = CountZonesOfType(state, HST_EZoneType.HST_ZONE_AIRFIELD);
		return total > 0 && CountTypeOwnedBy(state, HST_EZoneType.HST_ZONE_AIRFIELD, factionKey) == total;
	}

	protected bool IsStrategicZoneCounted(HST_ZoneState zone)
	{
		return zone && zone.m_eType != HST_EZoneType.HST_ZONE_HIDEOUT && zone.m_eType != HST_EZoneType.HST_ZONE_MISSION_SITE;
	}

	protected void ClearZoneGarrison(HST_CampaignState state, string zoneId, string factionKey)
	{
		if (!state || zoneId.IsEmpty() || factionKey.IsEmpty())
			return;

		HST_GarrisonState garrison = state.FindGarrison(zoneId, factionKey);
		if (!garrison)
			return;

		garrison.m_iInfantryCount = 0;
		garrison.m_iVehicleCount = 0;
	}

	protected void EvaluateCampaignOutcome(HST_CampaignState state, HST_EconomyService economy, HST_BalanceConfig balance, string resistanceFactionKey)
	{
		HST_CampaignOutcomeResult outcome = EvaluateCampaignOutcomeDetailed(state, economy, balance, resistanceFactionKey);
		if (!outcome || !outcome.m_bEnded)
			return;

		ApplyCampaignOutcome(state, outcome);
	}

	protected int CalculateTotalStrategicScoreForOutcome(HST_CampaignState state, HST_EconomyService economy)
	{
		if (!economy)
			return 0;

		return economy.CalculateTotalStrategicScore(state);
	}

	protected bool AreAllTypeOwnedBy(HST_CampaignState state, HST_EZoneType zoneType, string factionKey)
	{
		bool found;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != zoneType)
				continue;

			found = true;
			if (zone.m_sOwnerFactionKey != factionKey)
				return false;
		}

		return found;
	}

	protected bool IsCampaignOutcomeCheckDue(HST_CampaignState state, HST_BalanceConfig balance)
	{
		if (!state || !balance)
			return true;

		return state.m_iIncomeAccumulatorSeconds == 0;
	}

	protected int GetNextOutcomeCheckSecond(HST_CampaignState state, HST_BalanceConfig balance)
	{
		if (!state || !balance)
			return 0;

		int interval = Math.Max(1, balance.m_iZoneIncomeIntervalSeconds);
		int remaining = interval - Math.Max(0, state.m_iIncomeAccumulatorSeconds);
		if (remaining <= 0)
			remaining = interval;
		return state.m_iElapsedSeconds + remaining;
	}

	protected string ResolveOutcomeMode(HST_BalanceConfig balance)
	{
		if (!balance)
			return "unknown";
		if (balance.m_bPopulationOutcomeEnabled)
			return "population";
		if (balance.m_bLegacyControlVictoryEnabled)
			return "legacy_control";
		return "disabled";
	}

	protected int CalculatePopulationSupportPercent(int supportPopulation, int remainingPopulation)
	{
		if (remainingPopulation <= 0)
			return 0;

		return Math.Round(supportPopulation * 100.0 / remainingPopulation);
	}

	protected bool ShouldLosePopulationCampaign(HST_CampaignOutcomeResult result, HST_BalanceConfig balance)
	{
		if (!result || !balance || !balance.m_bLossConditionEnabled)
			return false;
		if (result.m_iInitialPopulation <= 0)
			return false;

		return result.m_iKilledPopulation * 3 > result.m_iInitialPopulation;
	}

	protected string EmptyReportField(string value)
	{
		if (value.IsEmpty())
			return "-";

		return value;
	}

	protected bool ShouldLoseCampaign(HST_CampaignState state, HST_BalanceConfig balance, string resistanceFactionKey)
	{
		if (!balance.m_bLossConditionEnabled)
			return false;
		if (state.m_iElapsedSeconds < balance.m_iLossGraceSeconds)
			return false;
		if (state.m_iPetrosDeaths >= balance.m_iLossPetrosDeathLimit)
			return true;
		if (state.m_iHR <= balance.m_iLossHRThreshold && state.m_iFactionMoney <= balance.m_iLossMoneyThreshold && !HasRecoverableFriendlyZone(state, resistanceFactionKey))
			return true;

		return false;
	}

	protected bool HasRecoverableFriendlyZone(HST_CampaignState state, string resistanceFactionKey)
	{
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (zone && zone.m_sOwnerFactionKey == resistanceFactionKey)
				return true;
		}

		return false;
	}
}
