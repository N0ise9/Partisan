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
			return "Partisan strategic event | missing event";

		string report = string.Format(
			"Partisan strategic event | id %1 | kind %2 | mission %3 | zone %4 | faction %5",
			m_Event.m_sEventId,
			m_Event.m_sKind,
			m_Event.m_sMissionId,
			m_Event.m_sTargetZoneId,
			m_Event.m_sTargetFactionKey
		);
		report = report + string.Format(
			" | applied %1 | changed %2 | money %3 | HR %4 | support %5 | capture %6",
			m_bApplied,
			m_bChanged,
			m_Event.m_iFactionMoneyDelta,
			m_Event.m_iHRDelta,
			m_Event.m_iTownSupportDelta,
			m_Event.m_iCaptureProgressDelta
		);
		report = report + string.Format(
			" | aggression %1 | attack %2 | supportRes %3 | HQ knowledge %4 | vehicle heat %5 | %6",
			m_Event.m_iAggressionDelta,
			m_Event.m_iAttackResourceDelta,
			m_Event.m_iSupportResourceDelta,
			m_Event.m_iHQKnowledgeDelta,
			m_Event.m_iVehicleHeatDelta,
			m_Event.m_sSummary
		);
		return report;
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
	bool m_bPopulationAuthorityValid;
}

class HST_StrategicService
{
	protected HST_TownInfluenceService m_TownInfluence;

	void SetTownInfluenceService(HST_TownInfluenceService townInfluence)
	{
		m_TownInfluence = townInfluence;
	}

	HST_StrategicEventApplyResult BeginZoneCaptureEvent(HST_CampaignState state, string zoneId, string previousOwner, string newOwner, string sourceId = "")
	{
		return BeginOwnershipTransitionEvent(
			state,
			zoneId,
			previousOwner,
			newOwner,
			"military_capture",
			"zone_capture",
			sourceId,
			string.Format("zone captured by %1", EmptyReportField(newOwner)));
	}

	HST_StrategicEventApplyResult BeginOwnershipTransitionEvent(
		HST_CampaignState state,
		string zoneId,
		string previousOwner,
		string newOwner,
		string cause,
		string sourceType,
		string sourceId,
		string reason)
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
		eventState.m_sKind = "ownership_transition";
		if (cause == "military_capture" || cause == "mission_capture")
			eventState.m_sKind = "zone_captured";
		eventState.m_sEventId = BuildStrategicEventId(state, eventState.m_sKind);
		if (eventState.m_sEventId.IsEmpty())
		{
			result.m_sReason = "strategic event id unavailable";
			return result;
		}
		eventState.m_sSourceType = sourceType;
		if (eventState.m_sSourceType.IsEmpty())
			eventState.m_sSourceType = "ownership_transition";
		if (sourceId.IsEmpty())
			eventState.m_sSourceId = zoneId;
		else
			eventState.m_sSourceId = sourceId;
		eventState.m_sTargetZoneId = zoneId;
		if (previousOwner.IsEmpty())
			eventState.m_sTargetFactionKey = zone.m_sOwnerFactionKey;
		else
			eventState.m_sTargetFactionKey = previousOwner;
		eventState.m_sReason = reason;
		if (eventState.m_sReason.IsEmpty())
			eventState.m_sReason = string.Format("ownership changed to %1", EmptyReportField(newOwner));
		eventState.m_iCreatedAtSecond = state.m_iElapsedSeconds;

		result.m_Event = eventState;
		result.m_sEventId = eventState.m_sEventId;
		result.m_bRecorded = true;
		state.m_aStrategicEvents.Insert(eventState);
		CaptureOwnershipTransitionEventStart(state, eventState);
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
		string outcomeKind = "mission_failure";
		if (succeeded)
			outcomeKind = "mission_success";
		HST_StrategicEventState existingEvent = FindMissionStrategicEvent(
			state,
			activeMission.m_sInstanceId,
			outcomeKind);
		if (existingEvent)
		{
			result.m_Event = existingEvent;
			result.m_sEventId = existingEvent.m_sEventId;
			result.m_bRecorded = true;
			result.m_bApplied = existingEvent.m_bApplied;
			result.m_sReason = "mission outcome already recorded";
			result.m_sSummary = existingEvent.m_sSummary;
			return result;
		}

		HST_StrategicEventState eventState = CreateMissionOutcomeEvent(state, preset, definition, activeMission, succeeded);
		if (!eventState)
		{
			result.m_sReason = "strategic event id unavailable";
			return result;
		}
		result.m_Event = eventState;
		result.m_sEventId = eventState.m_sEventId;
		CaptureStrategicEventBefore(state, eventState);
		array<ref HST_EnemyStrategicMutationCommand> mutationPlan = {};
		array<ref HST_EnemyStrategicMutationCommand> deferredMutationPreflightPlan = {};
		string mutationFailure;
		if (!BuildMissionOutcomeMutationPlan(
			state,
			preset,
			balance,
			definition,
			activeMission,
			succeeded,
			towns,
			zoneCapture,
			mutationPlan,
			deferredMutationPreflightPlan,
			mutationFailure)
			|| !ApplyMissionMutationPlan(
				state,
				preset,
				mutationPlan,
				deferredMutationPreflightPlan,
				mutationFailure))
		{
			result.m_sReason = mutationFailure;
			if (result.m_sReason.IsEmpty())
				result.m_sReason = "mission strategic mutation plan was rejected";
			return result;
		}
		state.m_aStrategicEvents.Insert(eventState);
		result.m_bRecorded = true;

		bool changed;
		if (succeeded)
			changed = ApplyMissionSuccessEvent(state, preset, economy, balance, towns, zoneCapture, garrisons, enemyCommander, enemyDirector, supportRequests, hq, definition, activeMission, applyDefinitionRewards);
		else
			changed = ApplyMissionFailureEvent(state, preset, economy, towns, hq, enemyDirector, definition, activeMission);

		RefreshStrategicEventAfter(state, eventState);
		eventState.m_bApplied = true;
		eventState.m_sSummary = BuildStrategicEventSummary(eventState);
		result.m_bApplied = true;
		result.m_bChanged = changed || HasStrategicEventDelta(eventState);
		result.m_sSummary = eventState.m_sSummary;
		return result;
	}

	HST_StrategicEventApplyResult ApplyMissionExpiryEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_MissionDefinition definition, HST_ActiveMissionState activeMission, HST_EnemyDirectorService enemyDirector = null)
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
			if (existingEvent.m_sEventId.IsEmpty())
			{
				result.m_sReason = "existing strategic event id unavailable";
				return result;
			}
			result.m_Event = existingEvent;
			result.m_sEventId = existingEvent.m_sEventId;
			result.m_bRecorded = true;
			result.m_bApplied = existingEvent.m_bApplied;
			result.m_sReason = "mission expiry already recorded";
			result.m_sSummary = existingEvent.m_sSummary;
			return result;
		}

		HST_StrategicEventState eventState = CreateMissionExpiryEvent(state, preset, definition, activeMission);
		if (!eventState)
		{
			result.m_sReason = "strategic event id unavailable";
			return result;
		}
		result.m_Event = eventState;
		result.m_sEventId = eventState.m_sEventId;
		CaptureStrategicEventBefore(state, eventState);
		array<ref HST_EnemyStrategicMutationCommand> mutationPlan = {};
		array<ref HST_EnemyStrategicMutationCommand> deferredMutationPreflightPlan = {};
		string mutationFailure;
		if (!BuildMissionExpiryMutationPlan(
			state,
			preset,
			definition,
			activeMission,
			mutationPlan,
			mutationFailure)
			|| !ApplyMissionMutationPlan(
				state,
				preset,
				mutationPlan,
				deferredMutationPreflightPlan,
				mutationFailure))
		{
			result.m_sReason = mutationFailure;
			if (result.m_sReason.IsEmpty())
				result.m_sReason = "mission expiry strategic mutation plan was rejected";
			return result;
		}
		state.m_aStrategicEvents.Insert(eventState);
		result.m_bRecorded = true;

		bool changed = ApplyMissionExpiryConsequences(state, preset, economy, definition, activeMission, enemyDirector);

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
		if (eventState.m_sEventId.IsEmpty())
		{
			result.m_sReason = "strategic event id unavailable";
			return result;
		}
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
		if (eventState.m_sEventId.IsEmpty())
		{
			result.m_sReason = "strategic event id unavailable";
			return result;
		}
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

	HST_StrategicEventApplyResult BeginVehicleReportEvent(HST_CampaignState state, HST_RuntimeVehicleState vehicle, string zoneId, string reason)
	{
		HST_StrategicEventApplyResult result = new HST_StrategicEventApplyResult();
		if (!state || !vehicle || vehicle.m_sVehicleRuntimeId.IsEmpty())
		{
			result.m_sReason = "state or runtime vehicle not ready";
			return result;
		}

		HST_StrategicEventState eventState = new HST_StrategicEventState();
		eventState.m_sKind = "vehicle_reported";
		eventState.m_sEventId = BuildStrategicEventId(state, eventState.m_sKind);
		if (eventState.m_sEventId.IsEmpty())
		{
			result.m_sReason = "strategic event id unavailable";
			return result;
		}
		eventState.m_sSourceType = "runtime_vehicle";
		eventState.m_sSourceId = vehicle.m_sVehicleRuntimeId;
		eventState.m_sVehicleRuntimeId = vehicle.m_sVehicleRuntimeId;
		if (zoneId.IsEmpty())
			eventState.m_sTargetZoneId = vehicle.m_sZoneId;
		else
			eventState.m_sTargetZoneId = zoneId;
		eventState.m_sTargetFactionKey = vehicle.m_sFactionKey;
		eventState.m_sReason = "vehicle reported: " + reason;
		eventState.m_iCreatedAtSecond = state.m_iElapsedSeconds;

		result.m_Event = eventState;
		result.m_sEventId = eventState.m_sEventId;
		result.m_bRecorded = true;
		state.m_aStrategicEvents.Insert(eventState);
		CaptureStrategicEventBefore(state, eventState);
		return result;
	}

	HST_StrategicEventApplyResult BeginTownInfluenceEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_TownInfluenceEventState influenceEvent)
	{
		HST_StrategicEventApplyResult result = new HST_StrategicEventApplyResult();
		if (!state || !influenceEvent)
		{
			result.m_sReason = "state or town influence event not ready";
			return result;
		}
		if (influenceEvent.m_sEventId.IsEmpty() || influenceEvent.m_sZoneId.IsEmpty())
		{
			result.m_sReason = "town influence event id or zone not ready";
			return result;
		}

		HST_ZoneState zone = state.FindZone(influenceEvent.m_sZoneId);
		if (!zone)
		{
			result.m_sReason = "town influence zone not found";
			return result;
		}

		HST_StrategicEventState eventState = new HST_StrategicEventState();
		eventState.m_sKind = "town_influence";
		eventState.m_sEventId = BuildStrategicEventId(state, eventState.m_sKind);
		if (eventState.m_sEventId.IsEmpty())
		{
			result.m_sReason = "strategic event id unavailable";
			return result;
		}
		eventState.m_sSourceType = "town_influence";
		eventState.m_sSourceId = influenceEvent.m_sEventId;
		eventState.m_sTargetZoneId = influenceEvent.m_sZoneId;
		eventState.m_sTargetFactionKey = ResolveTownInfluenceTargetFaction(state, preset, influenceEvent);
		eventState.m_sReason = BuildTownInfluenceReason(influenceEvent);
		eventState.m_iCreatedAtSecond = state.m_iElapsedSeconds;

		result.m_Event = eventState;
		result.m_sEventId = eventState.m_sEventId;
		result.m_bRecorded = true;
		state.m_aStrategicEvents.Insert(eventState);
		CaptureStrategicEventBefore(state, eventState);
		return result;
	}

	bool RecordOwnershipTransitionOwnerEffect(
		HST_CampaignState state,
		string eventId,
		string previousOwner,
		string newOwner,
		int captureProgressBefore)
	{
		if (!state || eventId.IsEmpty() || previousOwner.IsEmpty() || newOwner.IsEmpty())
			return false;
		HST_StrategicEventState eventState = state.FindStrategicEvent(eventId);
		if (!eventState || eventState.m_bApplied
			|| (eventState.m_sKind != "ownership_transition" && eventState.m_sKind != "zone_captured"))
			return false;
		eventState.m_sOwnerBefore = previousOwner;
		eventState.m_iCaptureProgressBefore = Math.Max(0, captureProgressBefore);
		eventState.m_iCaptureProgressAfter = 0;
		eventState.m_iCaptureProgressDelta = -eventState.m_iCaptureProgressBefore;
		return true;
	}

	void CompleteOwnershipTransitionEvent(
		HST_CampaignState state,
		HST_StrategicEventApplyResult result,
		string previousOwner,
		string newOwner,
		int aggressionApplied)
	{
		if (!state || !result || !result.m_Event)
			return;

		HST_StrategicEventState eventState = result.m_Event;
		eventState.m_sOwnerBefore = previousOwner;
		eventState.m_sOwnerAfter = newOwner;
		// Ownership effects are recorded from their exact receipt/checklist facts.
		// Never subtract global admission state from completion state: a queued or
		// retrying receipt may span unrelated economy, resource, or HQ mutations.
		eventState.m_iFactionMoneyDelta = 0;
		eventState.m_iHRDelta = 0;
		eventState.m_iHQKnowledgeBefore = 0;
		eventState.m_iHQKnowledgeAfter = 0;
		eventState.m_iHQKnowledgeDelta = 0;
		eventState.m_iSupportBefore = 0;
		eventState.m_iSupportAfter = 0;
		eventState.m_iTownSupportDelta = 0;
		eventState.m_iAggressionDelta = Math.Max(0, aggressionApplied);
		eventState.m_iAttackResourceDelta = 0;
		eventState.m_iSupportResourceDelta = 0;
		eventState.m_bApplied = true;
		eventState.m_sSummary = BuildStrategicEventSummary(eventState);
		result.m_bApplied = true;
		result.m_bChanged = true;
		result.m_sSummary = eventState.m_sSummary;
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
			string targetFactionKey = zone.m_sOwnerFactionKey;
			int aggressionDelta = ResolveMissionSuccessAggression(definition);
			RecordEnemySupportThreat(state, enemyDirector, targetFactionKey, zone, Math.Max(3, aggressionDelta), "mission success pressure: " + definition.m_sMissionId);
			changed = aggressionDelta != 0 || changed;
		}

		// Strategic admission intentionally precedes terminal mission/lifecycle
		// publication. It applies rewards/consequences only and never mutates the
		// transmitter aggregate. Keep stop-rebuild reachable despite DYNAMIC category.
		if (HST_RadioSiteLifecycleService.IsExactMission(activeMission))
		{
			if (hq)
				changed = hq.ReduceHQKnowledge(state, 20, "mission success: " + definition.m_sMissionId) || changed;
			if (activeMission.m_sMissionId == HST_RadioSiteLifecycleService.DESTROY_MISSION_ID)
			{
				if (zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey && zoneCapture)
					changed = zoneCapture.AddResistanceCaptureProgress(state, preset, this, economy, balance, zone.m_sZoneId, 35, 10, garrisons, enemyCommander, enemyDirector, supportRequests, activeMission.m_sInstanceId) || changed;
				return true;
			}
			if (zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey && zoneCapture)
				changed = zoneCapture.AddResistanceCaptureProgress(state, preset, this, economy, balance, zone.m_sZoneId, 20, 5, garrisons, enemyCommander, enemyDirector, supportRequests, activeMission.m_sInstanceId) || changed;
			return changed;
		}

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
		{
			if (zoneCapture && zoneCapture.AddResistanceCaptureProgress(state, preset, this, economy, balance, zone.m_sZoneId, 60, 15, garrisons, enemyCommander, enemyDirector, supportRequests, activeMission.m_sInstanceId))
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
			if ((definition.m_sMissionId == "destroy_radio_tower" || definition.m_sMissionId == "dynamic_stop_tower_rebuild") && hq)
				changed = hq.ReduceHQKnowledge(state, 20, "mission success: " + definition.m_sMissionId) || changed;
			if (zone.m_sOwnerFactionKey != preset.m_sResistanceFactionKey && zoneCapture)
				changed = zoneCapture.AddResistanceCaptureProgress(state, preset, this, economy, balance, zone.m_sZoneId, 35, 10, garrisons, enemyCommander, enemyDirector, supportRequests, activeMission.m_sInstanceId) || changed;
			return true;
		}

		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY || definition.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
		{
			if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY)
			{
				economy.AddFactionMoney(state, 150);
				economy.AddHR(state, 1);
			}
			return true;
		}

		if (HST_RescuePOWOperationService.IsExactMission(activeMission))
		{
			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN && definition.m_iRewardSupport != 0
				&& towns && towns.AddSupport(state, zone.m_sZoneId, definition.m_iRewardSupport))
				changed = true;
			return changed;
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

				if (zoneCapture && zoneCapture.AddResistanceCaptureProgress(state, preset, this, economy, balance, zone.m_sZoneId, 50, 10, garrisons, enemyCommander, enemyDirector, supportRequests, activeMission.m_sInstanceId))
					changed = true;
				return changed;
			}

			if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			{
				if (towns && towns.AddSupport(state, zone.m_sZoneId, 10))
					changed = true;
				return changed;
			}

			if (zoneCapture && zoneCapture.AddResistanceCaptureProgress(state, preset, this, economy, balance, zone.m_sZoneId, 20, 5, garrisons, enemyCommander, enemyDirector, supportRequests, activeMission.m_sInstanceId))
				changed = true;
			return changed;
		}

		return changed;
	}

	protected bool ApplyMissionFailureEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_TownService towns, HST_HQService hq, HST_EnemyDirectorService enemyDirector, HST_MissionDefinition definition, HST_ActiveMissionState activeMission)
	{
		bool changed = definition.m_iFailureAggression != 0;
		HST_ZoneState failureZone = null;
		if (!activeMission.m_sTargetZoneId.IsEmpty())
			failureZone = state.FindZone(activeMission.m_sTargetZoneId);
		if (failureZone && definition.m_iFailureAggression > 0)
			RecordEnemySupportThreat(state, enemyDirector, preset.m_sOccupierFactionKey, failureZone, Math.Max(2, definition.m_iFailureAggression), "mission failure pressure: " + definition.m_sMissionId);

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
				int targetAggressionDelta = Math.Max(1, definition.m_iFailureAggression / 2);
				RecordEnemySupportThreat(state, enemyDirector, targetZone.m_sOwnerFactionKey, targetZone, Math.Max(2, targetAggressionDelta), "mission failure local pressure: " + definition.m_sMissionId);
				return targetAggressionDelta != 0 || changed;
			}
		}

		return changed;
	}

	protected bool ApplyMissionExpiryConsequences(HST_CampaignState state, HST_CampaignPreset preset, HST_EconomyService economy, HST_MissionDefinition definition, HST_ActiveMissionState activeMission, HST_EnemyDirectorService enemyDirector = null)
	{
		if (definition.m_sMissionId == "dynamic_defend_petros")
			return false;

		if (definition.m_iFailureAggression > 0 && activeMission && !activeMission.m_sTargetZoneId.IsEmpty())
		{
			HST_ZoneState targetZone = state.FindZone(activeMission.m_sTargetZoneId);
			if (targetZone)
				RecordEnemySupportThreat(state, enemyDirector, preset.m_sOccupierFactionKey, targetZone, Math.Max(2, definition.m_iFailureAggression), "mission expiry pressure: " + definition.m_sMissionId);
		}
		return definition.m_iFailureAggression != 0;
	}

	protected void RecordEnemySupportThreat(HST_CampaignState state, HST_EnemyDirectorService enemyDirector, string factionKey, HST_ZoneState targetZone, int threatScore, string reason)
	{
		if (!state || !enemyDirector || factionKey.IsEmpty() || !targetZone || threatScore <= 0)
			return;

		enemyDirector.RecordZoneDamageSignal(state, factionKey, targetZone, threatScore, reason);
	}

	protected HST_StrategicEventState CreateMissionOutcomeEvent(HST_CampaignState state, HST_CampaignPreset preset, HST_MissionDefinition definition, HST_ActiveMissionState activeMission, bool succeeded)
	{
		HST_StrategicEventState eventState = new HST_StrategicEventState();
		if (succeeded)
			eventState.m_sKind = "mission_success";
		else
			eventState.m_sKind = "mission_failure";
		eventState.m_sEventId = BuildMissionStrategicEventId(
			state,
			eventState.m_sKind,
			activeMission.m_sInstanceId);
		if (eventState.m_sEventId.IsEmpty())
			return null;
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
		eventState.m_sEventId = BuildMissionStrategicEventId(
			state,
			eventState.m_sKind,
			activeMission.m_sInstanceId);
		if (eventState.m_sEventId.IsEmpty())
			return null;
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

	protected string ResolveTownInfluenceTargetFaction(HST_CampaignState state, HST_CampaignPreset preset, HST_TownInfluenceEventState influenceEvent)
	{
		if (!influenceEvent)
			return "";

		string targetFactionKey = "";
		if (!influenceEvent.m_sAggressionFactionKey.IsEmpty())
		{
			return ResolveStrategicEventTargetFactionForZone(
				state,
				preset,
				influenceEvent.m_sZoneId,
				influenceEvent.m_sAggressionFactionKey);
		}
		if (preset)
		{
			if (influenceEvent.m_iContractVersion == HST_TownInfluenceService.EXACT_CONTRACT_VERSION)
			{
				int resistancePressure = Math.Max(0, influenceEvent.m_iEffectiveFIABasisPointDelta);
				resistancePressure += Math.Max(0, -influenceEvent.m_iEffectiveOccupierBasisPointDelta);
				resistancePressure += Math.Max(0, -influenceEvent.m_iEffectiveInvaderBasisPointDelta);
				resistancePressure += Math.Max(0, influenceEvent.m_iReputationDelta) * 100;
				int sharedEnemyPressure = Math.Max(0, -influenceEvent.m_iEffectiveFIABasisPointDelta);
				sharedEnemyPressure += Math.Max(0, influenceEvent.m_iHeatDelta) * 100;
				sharedEnemyPressure += Math.Max(0, influenceEvent.m_iPoliceDelta) * 100;
				sharedEnemyPressure += Math.Max(0, influenceEvent.m_iRoadblockDelta) * 100;
				int occupierPressure = Math.Max(0, influenceEvent.m_iEffectiveOccupierBasisPointDelta);
				int invaderPressure = Math.Max(0, influenceEvent.m_iEffectiveInvaderBasisPointDelta);
				HST_ZoneState zone = state.FindZone(influenceEvent.m_sZoneId);
				if (zone && zone.m_sOwnerFactionKey == preset.m_sInvaderFactionKey)
					invaderPressure += sharedEnemyPressure;
				else
					occupierPressure += sharedEnemyPressure;
				if (invaderPressure > resistancePressure && invaderPressure > occupierPressure)
					targetFactionKey = preset.m_sInvaderFactionKey;
				else if (occupierPressure > resistancePressure)
					targetFactionKey = preset.m_sOccupierFactionKey;
				else
					targetFactionKey = preset.m_sResistanceFactionKey;
			}
			else
			{
				int resistancePressure = influenceEvent.m_iFIASupportDelta + Math.Max(0, influenceEvent.m_iReputationDelta);
				int occupierPressure = influenceEvent.m_iOccupierSupportDelta;
				occupierPressure += Math.Max(0, influenceEvent.m_iHeatDelta);
				occupierPressure += Math.Max(0, influenceEvent.m_iPoliceDelta);
				occupierPressure += Math.Max(0, influenceEvent.m_iRoadblockDelta);
				if (resistancePressure >= occupierPressure)
					targetFactionKey = preset.m_sResistanceFactionKey;
				else
					targetFactionKey = preset.m_sOccupierFactionKey;
			}
		}

		return ResolveStrategicEventTargetFactionForZone(state, preset, influenceEvent.m_sZoneId, targetFactionKey);
	}

	protected string BuildTownInfluenceReason(HST_TownInfluenceEventState influenceEvent)
	{
		if (!influenceEvent)
			return "town influence";

		int invaderDelta = influenceEvent.m_iRequestedInvaderBasisPointDelta / 100;
		if (influenceEvent.m_bAbsoluteDebugSeed)
			invaderDelta = influenceEvent.m_iRequestedInvaderBasisPointDelta;
		string reason = string.Format(
			"town influence %1 | FIA %2 occupier %3 invader %4 rep %5 heat %6",
			EmptyReportField(influenceEvent.m_sKind),
			influenceEvent.m_iFIASupportDelta,
			influenceEvent.m_iOccupierSupportDelta,
			invaderDelta,
			influenceEvent.m_iReputationDelta,
			influenceEvent.m_iHeatDelta
		);
		if (influenceEvent.m_iContractVersion == HST_TownInfluenceService.EXACT_CONTRACT_VERSION)
		{
			reason = reason + string.Format(
				" | effective FIA/occupier/invader %1/%2/%3 bp",
				influenceEvent.m_iEffectiveFIABasisPointDelta,
				influenceEvent.m_iEffectiveOccupierBasisPointDelta,
				influenceEvent.m_iEffectiveInvaderBasisPointDelta);
		}
		reason = reason + string.Format(
			" pop %1 police %2 roadblocks %3",
			influenceEvent.m_iPopulationDelta,
			influenceEvent.m_iPoliceDelta,
			influenceEvent.m_iRoadblockDelta
		);
		if (!influenceEvent.m_sAggressionFactionKey.IsEmpty()
			|| influenceEvent.m_iAggressionDelta != 0)
		{
			reason = reason + string.Format(
				" aggression %1 %2",
				influenceEvent.m_sAggressionFactionKey,
				influenceEvent.m_iAggressionDelta);
		}
		if (!influenceEvent.m_sReason.IsEmpty())
			reason = reason + " | " + influenceEvent.m_sReason;
		return reason;
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

	protected int ResolveCanonicalZoneSupport(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return 0;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
		{
			if (!m_TownInfluence)
				return 0;
			return m_TownInfluence.ResolveSignedSupportPercent(state, zone.m_sZoneId);
		}
		return zone.m_iSupport;
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
			int signedSupport = ResolveCanonicalZoneSupport(state, zone);
			eventState.m_iSupportBefore = signedSupport;
			eventState.m_iSupportAfter = signedSupport;
			eventState.m_iTownSupportDelta = -signedSupport;
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

		CaptureStrategicEventVehicleBefore(state, eventState);
	}

	protected void CaptureOwnershipTransitionEventStart(
		HST_CampaignState state,
		HST_StrategicEventState eventState)
	{
		if (!state || !eventState)
			return;

		eventState.m_iFactionMoneyDelta = 0;
		eventState.m_iHRDelta = 0;
		eventState.m_iHQKnowledgeBefore = 0;
		eventState.m_iHQKnowledgeAfter = 0;
		eventState.m_iHQKnowledgeDelta = 0;
		HST_ZoneState zone = state.FindZone(eventState.m_sTargetZoneId);
		if (zone)
		{
			eventState.m_sOwnerBefore = zone.m_sOwnerFactionKey;
			eventState.m_iSupportBefore = 0;
			eventState.m_iSupportAfter = 0;
			eventState.m_iTownSupportDelta = 0;
			eventState.m_iCaptureProgressBefore = Math.Max(0, zone.m_iResistanceCaptureProgress);
			eventState.m_iCaptureProgressAfter = eventState.m_iCaptureProgressBefore;
			eventState.m_iCaptureProgressDelta = 0;
		}
		eventState.m_iAggressionDelta = 0;
		eventState.m_iAttackResourceDelta = 0;
		eventState.m_iSupportResourceDelta = 0;
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
			int signedSupport = ResolveCanonicalZoneSupport(state, zone);
			eventState.m_iSupportAfter = signedSupport;
			eventState.m_iTownSupportDelta += signedSupport;
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

		RefreshStrategicEventVehicleAfter(state, eventState);
	}

	protected void CaptureStrategicEventVehicleBefore(HST_CampaignState state, HST_StrategicEventState eventState)
	{
		if (!state || !eventState || eventState.m_sSourceType != "runtime_vehicle" || eventState.m_sSourceId.IsEmpty())
			return;

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(eventState.m_sSourceId);
		if (!vehicle)
			return;

		eventState.m_sVehicleRuntimeId = vehicle.m_sVehicleRuntimeId;
		eventState.m_iVehicleHeatBefore = vehicle.m_iVehicleHeat;
		eventState.m_iVehicleHeatAfter = vehicle.m_iVehicleHeat;
		eventState.m_iVehicleHeatDelta = -vehicle.m_iVehicleHeat;
		eventState.m_bVehicleReportedBefore = vehicle.m_bReported;
		eventState.m_bVehicleReportedAfter = vehicle.m_bReported;
		eventState.m_iVehicleReportedUntilBefore = vehicle.m_iReportedUntilSecond;
		eventState.m_iVehicleReportedUntilAfter = vehicle.m_iReportedUntilSecond;
		eventState.m_iVehicleReportedUntilDelta = -vehicle.m_iReportedUntilSecond;
	}

	protected void RefreshStrategicEventVehicleAfter(HST_CampaignState state, HST_StrategicEventState eventState)
	{
		if (!state || !eventState || eventState.m_sSourceType != "runtime_vehicle" || eventState.m_sSourceId.IsEmpty())
			return;

		HST_RuntimeVehicleState vehicle = state.FindRuntimeVehicle(eventState.m_sSourceId);
		if (!vehicle)
			return;

		eventState.m_sVehicleRuntimeId = vehicle.m_sVehicleRuntimeId;
		eventState.m_iVehicleHeatAfter = vehicle.m_iVehicleHeat;
		eventState.m_iVehicleHeatDelta += vehicle.m_iVehicleHeat;
		eventState.m_bVehicleReportedAfter = vehicle.m_bReported;
		eventState.m_iVehicleReportedUntilAfter = vehicle.m_iReportedUntilSecond;
		eventState.m_iVehicleReportedUntilDelta += vehicle.m_iReportedUntilSecond;
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
			|| eventState.m_iVehicleHeatDelta != 0
			|| eventState.m_iVehicleReportedUntilDelta != 0
			|| eventState.m_bVehicleReportedBefore != eventState.m_bVehicleReportedAfter
			|| eventState.m_sOwnerBefore != eventState.m_sOwnerAfter;
	}

	protected string BuildStrategicEventSummary(HST_StrategicEventState eventState)
	{
		if (!eventState)
			return "";

		string summary = string.Format(
			"%1 | money %2 HR %3 | support %4 | capture %5 | aggression %6",
			eventState.m_sReason,
			eventState.m_iFactionMoneyDelta,
			eventState.m_iHRDelta,
			eventState.m_iTownSupportDelta,
			eventState.m_iCaptureProgressDelta,
			eventState.m_iAggressionDelta
		);
		summary = summary + string.Format(
			" | resources %1/%2 | HQ %3",
			eventState.m_iAttackResourceDelta,
			eventState.m_iSupportResourceDelta,
			eventState.m_iHQKnowledgeDelta
		);
		summary = summary + string.Format(
			" | vehicle heat %1 report %2->%3 until %4",
			eventState.m_iVehicleHeatDelta,
			eventState.m_bVehicleReportedBefore,
			eventState.m_bVehicleReportedAfter,
			eventState.m_iVehicleReportedUntilDelta
		);
		summary = summary + string.Format(
			" | owner %1 -> %2",
			EmptyReportField(eventState.m_sOwnerBefore),
			EmptyReportField(eventState.m_sOwnerAfter)
		);
		return summary;
	}

	string BuildStrategicEventReport(HST_CampaignState state, int maxRows = 20)
	{
		if (!state)
			return "Partisan strategic events | state not ready";

		string report = string.Format("Partisan strategic events | total %1 | showing %2", state.m_aStrategicEvents.Count(), maxRows);
		int emitted;
		for (int i = state.m_aStrategicEvents.Count() - 1; i >= 0; i--)
		{
			if (emitted >= maxRows)
				break;

			HST_StrategicEventState eventState = state.m_aStrategicEvents[i];
			if (!eventState)
				continue;

			string row = string.Format(
				"\n%1 | %2 | mission %3/%4 | zone %5 | faction %6 | applied %7",
				eventState.m_sEventId,
				eventState.m_sKind,
				EmptyReportField(eventState.m_sMissionId),
				EmptyReportField(eventState.m_sMissionInstanceId),
				EmptyReportField(eventState.m_sTargetZoneId),
				EmptyReportField(eventState.m_sTargetFactionKey),
				eventState.m_bApplied
			);
			row = row + string.Format(
				" | money %1 HR %2 support %3 capture %4 aggression %5",
				eventState.m_iFactionMoneyDelta,
				eventState.m_iHRDelta,
				eventState.m_iTownSupportDelta,
				eventState.m_iCaptureProgressDelta,
				eventState.m_iAggressionDelta
			);
			row = row + string.Format(
				" resources %1/%2 HQ %3",
				eventState.m_iAttackResourceDelta,
				eventState.m_iSupportResourceDelta,
				eventState.m_iHQKnowledgeDelta
			);
			row = row + string.Format(
				" vehicle %1 heat %2 report %3->%4",
				EmptyReportField(eventState.m_sVehicleRuntimeId),
				eventState.m_iVehicleHeatDelta,
				eventState.m_bVehicleReportedBefore,
				eventState.m_bVehicleReportedAfter
			);
			report = report + row + " | " + eventState.m_sSummary;
			emitted++;
		}

		return report;
	}

	protected string BuildStrategicEventId(HST_CampaignState state, string kind)
	{
		if (!state)
			return "";
		string safeKind = kind;
		if (safeKind.IsEmpty())
			safeKind = "event";
		for (int attempt; attempt < 64; attempt++)
		{
			string candidate = HST_StableIdService.NextId(
				state,
				"strategic_" + safeKind);
			if (candidate.IsEmpty())
				return "";
			bool duplicate;
			foreach (HST_StrategicEventState existing : state.m_aStrategicEvents)
			{
				if (existing && existing.m_sEventId == candidate)
				{
					duplicate = true;
					break;
				}
			}
			if (!duplicate)
				return candidate;
		}
		return "";
	}

	// Mission terminal identities derive from the durable mission identity. A
	// rejected resource or ownership preflight therefore cannot consume the
	// campaign-wide authority sequence, and an exact retry addresses the same
	// event without relying on a process-local allocation attempt.
	protected string BuildMissionStrategicEventId(
		HST_CampaignState state,
		string kind,
		string missionInstanceId)
	{
		if (!state || kind.IsEmpty() || missionInstanceId.IsEmpty())
			return "";
		string candidate = "strategic_" + kind + "_" + missionInstanceId;
		foreach (HST_StrategicEventState existing : state.m_aStrategicEvents)
		{
			if (existing && existing.m_sEventId == candidate)
				return "";
		}
		return candidate;
	}

	// Read-only counterpart to BuildStrategicEventId for authorities that must
	// prove a later admission cannot fail after another domain commits.
	bool CanBuildStrategicEventId(
		HST_CampaignState state,
		string kind,
		out string failure)
	{
		failure = "";
		if (!state)
		{
			failure = "strategic event state is unavailable";
			return false;
		}
		string safeKind = kind;
		if (safeKind.IsEmpty())
			safeKind = "event";
		int sequence = state.m_iNextAuthoritySequence;
		if (sequence < 1)
			sequence = 1;
		for (int attempt; attempt < 64; attempt++)
		{
			if (sequence >= int.MAX)
			{
				failure = "strategic event authority sequence is exhausted";
				return false;
			}
			string candidate = string.Format(
				"%1_%2_%3",
				"strategic_" + safeKind,
				state.m_iCampaignSeed,
				sequence);
			bool duplicate;
			foreach (HST_StrategicEventState existing : state.m_aStrategicEvents)
			{
				if (existing && existing.m_sEventId == candidate)
				{
					duplicate = true;
					break;
				}
			}
			if (!duplicate)
				return true;
			sequence++;
		}
		failure = "strategic event identity collision budget is exhausted";
		return false;
	}

	protected bool BuildMissionOutcomeMutationPlan(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		HST_MissionDefinition definition,
		HST_ActiveMissionState activeMission,
		bool succeeded,
		HST_TownService towns,
		HST_ZoneCaptureService zoneCapture,
		notnull array<ref HST_EnemyStrategicMutationCommand> plan,
		notnull array<ref HST_EnemyStrategicMutationCommand> deferredPreflightPlan,
		out string failure)
	{
		failure = "";
		if (!state || !preset || !definition || !activeMission)
		{
			failure = "mission strategic mutation planning authority is unavailable";
			return false;
		}
		if (succeeded)
		{
			if (!BuildMissionSuccessMutationPlan(
				state,
				preset,
				definition,
				activeMission,
				plan,
				failure))
				return false;
			return BuildMissionCaptureMutationPreflight(
				state,
				preset,
				balance,
				definition,
				activeMission,
				zoneCapture,
				deferredPreflightPlan,
				failure);
		}
		return BuildMissionFailureMutationPlan(
			state,
			preset,
			definition,
			activeMission,
			towns,
			plan,
			failure);
	}

	protected bool BuildMissionSuccessMutationPlan(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_MissionDefinition definition,
		HST_ActiveMissionState activeMission,
		notnull array<ref HST_EnemyStrategicMutationCommand> plan,
		out string failure)
	{
		failure = "";
		if (activeMission.m_sTargetZoneId.IsEmpty()
			|| definition.m_sMissionId == "dynamic_defend_petros")
			return true;
		HST_ZoneState zone = state.FindZone(activeMission.m_sTargetZoneId);
		if (!zone || zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			return true;

		plan.Insert(BuildMissionMutationCommand(
			"enemy_aggression_mission_success_" + activeMission.m_sInstanceId,
			zone.m_sOwnerFactionKey,
			"mission_success",
			activeMission,
			0,
			0,
			ResolveMissionSuccessAggression(definition)));

		int requestedAttackDamage;
		int requestedSupportDamage;
		if (HST_RadioSiteLifecycleService.IsExactMission(activeMission)
			&& activeMission.m_sMissionId == HST_RadioSiteLifecycleService.DESTROY_MISSION_ID)
		{
			requestedAttackDamage = 12;
			requestedSupportDamage = 6;
		}
		else if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
		{
			requestedAttackDamage = 12;
			requestedSupportDamage = 6;
		}
		else if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONVOY
			|| definition.m_eCategory == HST_EMissionCategory.HST_MISSION_LOGISTICS)
		{
			requestedAttackDamage = 8;
			requestedSupportDamage = 4;
		}
		if (requestedAttackDamage <= 0 && requestedSupportDamage <= 0)
			return true;

		string resourceMutationId
			= "enemy_resource_mission_success_" + activeMission.m_sInstanceId;
		int attackDelta;
		int supportDelta;
		HST_EnemyStrategicMutationState existing
			= state.FindEnemyStrategicMutation(resourceMutationId);
		if (existing)
		{
			attackDelta = existing.m_iAttackDelta;
			supportDelta = existing.m_iSupportDelta;
		}
		else
		{
			HST_FactionPoolState pool = state.FindFactionPool(zone.m_sOwnerFactionKey);
			if (!pool)
			{
				failure = "mission success enemy resource pool is unavailable";
				return false;
			}
			attackDelta = -Math.Min(
				Math.Max(0, pool.m_iAttackResources),
				requestedAttackDamage);
			supportDelta = -Math.Min(
				Math.Max(0, pool.m_iSupportResources),
				requestedSupportDamage);
		}
		if (attackDelta == 0 && supportDelta == 0 && !existing)
			return true;
		plan.Insert(BuildMissionMutationCommand(
			resourceMutationId,
			zone.m_sOwnerFactionKey,
			"mission_success_resource_damage",
			activeMission,
			attackDelta,
			supportDelta,
			0));
		return true;
	}

	// Capture progress can synchronously enter the ownership authority. Its
	// Schema-67 aggression command is admitted against the same preflight clone,
	// but remains deferred to that authority so it cannot become an orphan if an
	// earlier ownership-domain admission rejects for an unrelated reason.
	protected bool BuildMissionCaptureMutationPreflight(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		HST_MissionDefinition definition,
		HST_ActiveMissionState activeMission,
		HST_ZoneCaptureService zoneCapture,
		notnull array<ref HST_EnemyStrategicMutationCommand> deferredPreflightPlan,
		out string failure)
	{
		failure = "";
		if (!zoneCapture || activeMission.m_sTargetZoneId.IsEmpty())
			return true;
		HST_ZoneState zone = state.FindZone(activeMission.m_sTargetZoneId);
		if (!zone || zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey
			|| !IsMissionCaptureTarget(zone))
			return true;

		int progressAmount = ResolveMissionSuccessCaptureProgressAmount(
			definition,
			activeMission,
			zone);
		if (progressAmount <= 0)
			return true;
		int required = HST_ZoneCaptureService.CAPTURE_PROGRESS_REQUIRED;
		if (balance && balance.m_iCaptureProgressRequired > 0)
			required = balance.m_iCaptureProgressRequired;
		int projectedProgress = Math.Min(
			required,
			Math.Max(0, zone.m_iResistanceCaptureProgress + progressAmount));
		if (zoneCapture.HasIncompleteConquestMissionForZone(state, zone)
			&& projectedProgress >= Math.Min(
				HST_ZoneCaptureService.CONQUEST_OBJECTIVE_PROGRESS_CAP,
				required - 1))
			return true;
		if (projectedProgress < required)
			return true;

		int supportReward = ResolveMissionSuccessCaptureSupportReward(
			definition,
			activeMission);
		string ownershipAdmissionFailure;
		if (!zoneCapture.CanCaptureForResistanceDetailed(
				state,
				preset,
				zone.m_sZoneId,
				supportReward,
				activeMission.m_sInstanceId,
				ownershipAdmissionFailure))
		{
			failure = "mission capture ownership admission rejected";
			if (!ownershipAdmissionFailure.IsEmpty())
				failure = ownershipAdmissionFailure;
			return false;
		}

		string ownershipRequestId = "ownership_capture_"
			+ zone.m_sZoneId + "_" + activeMission.m_sInstanceId;
		HST_EnemyStrategicMutationCommand command
			= new HST_EnemyStrategicMutationCommand();
		command.m_sMutationId = "enemy_aggression_ownership_"
			+ ownershipRequestId;
		command.m_sFactionKey = zone.m_sOwnerFactionKey;
		command.m_sKind = "ownership_transition";
		command.m_sSourceId = ownershipRequestId;
		command.m_sZoneId = zone.m_sZoneId;
		int captureAggression = 10;
		if (balance && balance.m_iCaptureAggressionBase > 0)
			captureAggression = balance.m_iCaptureAggressionBase;
		command.m_iAggressionDelta = captureAggression
			+ Math.Max(0, zone.m_iPriority / 5);
		deferredPreflightPlan.Insert(command);
		return true;
	}

	protected int ResolveMissionSuccessCaptureSupportReward(
		HST_MissionDefinition definition,
		HST_ActiveMissionState activeMission)
	{
		if (HST_RadioSiteLifecycleService.IsExactMission(activeMission))
		{
			if (activeMission.m_sMissionId
				== HST_RadioSiteLifecycleService.DESTROY_MISSION_ID)
				return 10;
			return 5;
		}
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return 15;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return 10;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DYNAMIC
			&& definition.m_sMissionId == "dynamic_city_flip_battle")
			return 10;
		return 5;
	}

	protected int ResolveMissionSuccessCaptureProgressAmount(
		HST_MissionDefinition definition,
		HST_ActiveMissionState activeMission,
		HST_ZoneState zone)
	{
		if (HST_RadioSiteLifecycleService.IsExactMission(activeMission))
		{
			if (activeMission.m_sMissionId
				== HST_RadioSiteLifecycleService.DESTROY_MISSION_ID)
				return 35;
			return 20;
		}
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_CONQUEST)
			return 60;
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_DESTROY)
			return 35;
		if (definition.m_eCategory != HST_EMissionCategory.HST_MISSION_DYNAMIC
			|| HST_RescuePOWOperationService.IsExactMission(activeMission))
			return 0;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN)
			return 0;
		if (definition.m_sMissionId == "dynamic_city_flip_battle")
			return 50;
		return 20;
	}

	protected bool IsMissionCaptureTarget(HST_ZoneState zone)
	{
		if (!zone)
			return false;
		return zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST
			|| zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE
			|| zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY
			|| zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT
			|| zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD
			|| zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER;
	}

	protected bool BuildMissionFailureMutationPlan(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_MissionDefinition definition,
		HST_ActiveMissionState activeMission,
		HST_TownService towns,
		notnull array<ref HST_EnemyStrategicMutationCommand> plan,
		out string failure)
	{
		failure = "";
		plan.Insert(BuildMissionMutationCommand(
			"enemy_aggression_mission_failure_primary_" + activeMission.m_sInstanceId,
			preset.m_sOccupierFactionKey,
			"mission_failure",
			activeMission,
			0,
			0,
			definition.m_iFailureAggression));
		if (definition.m_sMissionId == "dynamic_defend_petros"
			|| definition.m_iFailureAggression >= 4
			|| activeMission.m_sTargetZoneId.IsEmpty())
			return true;
		HST_ZoneState targetZone = state.FindZone(activeMission.m_sTargetZoneId);
		if (definition.m_eCategory == HST_EMissionCategory.HST_MISSION_SUPPORT
			&& targetZone && towns)
			return true;
		if (!targetZone
			|| targetZone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
			return true;
		plan.Insert(BuildMissionMutationCommand(
			"enemy_aggression_mission_failure_local_" + activeMission.m_sInstanceId,
			targetZone.m_sOwnerFactionKey,
			"mission_failure_local",
			activeMission,
			0,
			0,
			Math.Max(1, definition.m_iFailureAggression / 2)));
		return true;
	}

	protected bool BuildMissionExpiryMutationPlan(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_MissionDefinition definition,
		HST_ActiveMissionState activeMission,
		notnull array<ref HST_EnemyStrategicMutationCommand> plan,
		out string failure)
	{
		failure = "";
		if (!state || !preset || !definition || !activeMission)
		{
			failure = "mission expiry strategic mutation planning authority is unavailable";
			return false;
		}
		plan.Insert(BuildMissionMutationCommand(
			"enemy_aggression_mission_expiry_" + activeMission.m_sInstanceId,
			preset.m_sOccupierFactionKey,
			"mission_expiry",
			activeMission,
			0,
			0,
			definition.m_iFailureAggression));
		return true;
	}

	protected HST_EnemyStrategicMutationCommand BuildMissionMutationCommand(
		string mutationId,
		string factionKey,
		string kind,
		HST_ActiveMissionState activeMission,
		int attackDelta,
		int supportDelta,
		int aggressionDelta)
	{
		HST_EnemyStrategicMutationCommand command
			= new HST_EnemyStrategicMutationCommand();
		command.m_sMutationId = mutationId;
		command.m_sFactionKey = factionKey;
		command.m_sKind = kind;
		command.m_sSourceId = activeMission.m_sInstanceId;
		command.m_sOperationId = activeMission.m_sOperationId;
		command.m_sManifestId = activeMission.m_sManifestId;
		command.m_sZoneId = activeMission.m_sTargetZoneId;
		command.m_iAttackDelta = attackDelta;
		command.m_iSupportDelta = supportDelta;
		command.m_iAggressionDelta = aggressionDelta;
		return command;
	}

	// Dry-run the complete mutation set against an exact deep copy. The live pass
	// then executes the same deterministic commands without any intervening state
	// mutation, so no reward/support/capture write can precede a rejected receipt.
	protected bool ApplyMissionMutationPlan(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		notnull array<ref HST_EnemyStrategicMutationCommand> plan,
		notnull array<ref HST_EnemyStrategicMutationCommand> deferredPreflightPlan,
		out string failure)
	{
		failure = "";
		if (!state || !preset)
		{
			failure = "mission strategic mutation authority is unavailable";
			return false;
		}
		if (plan.IsEmpty() && deferredPreflightPlan.IsEmpty())
			return true;
		HST_CampaignSaveData snapshot = new HST_CampaignSaveData();
		snapshot.Capture(state);
		HST_CampaignState preflightState = new HST_CampaignState();
		snapshot.ApplyTo(preflightState, false);
		HST_EnemyStrategicResourceService preflightAuthority
			= new HST_EnemyStrategicResourceService();
		foreach (HST_EnemyStrategicMutationCommand command : plan)
		{
			HST_EnemyStrategicMutationResult preflight
				= preflightAuthority.ApplyMutation(
					preflightState,
					preset,
					command);
			if (preflight && preflight.m_bAccepted)
				continue;
			failure = "mission strategic mutation preflight rejected";
			if (preflight && !preflight.m_sFailureReason.IsEmpty())
				failure = preflight.m_sFailureReason;
			return false;
		}
		foreach (HST_EnemyStrategicMutationCommand deferredCommand : deferredPreflightPlan)
		{
			HST_EnemyStrategicMutationResult deferredPreflight
				= preflightAuthority.ApplyMutation(
					preflightState,
					preset,
					deferredCommand);
			if (deferredPreflight && deferredPreflight.m_bAccepted)
				continue;
			failure = "mission deferred strategic mutation preflight rejected";
			if (deferredPreflight
				&& !deferredPreflight.m_sFailureReason.IsEmpty())
				failure = deferredPreflight.m_sFailureReason;
			return false;
		}

		HST_EnemyStrategicResourceService liveAuthority
			= new HST_EnemyStrategicResourceService();
		foreach (HST_EnemyStrategicMutationCommand commandToApply : plan)
		{
			HST_EnemyStrategicMutationResult applied
				= liveAuthority.ApplyMutation(
					state,
					preset,
					commandToApply);
			if (applied && applied.m_bAccepted)
				continue;
			failure = "mission strategic mutation live commit diverged from preflight";
			if (applied && !applied.m_sFailureReason.IsEmpty())
				failure = applied.m_sFailureReason;
			return false;
		}
		return true;
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
		HST_TownPopulationAggregate population;
		if (m_TownInfluence)
			population = m_TownInfluence.BuildPopulationAggregate(state);
		if (population && population.m_bAuthorityValid)
		{
			result.m_bPopulationAuthorityValid = true;
			result.m_iInitialPopulation = population.m_iInitialPopulation;
			result.m_iRemainingPopulation = population.m_iRemainingPopulation;
			result.m_iKilledPopulation = population.m_iDestroyedPopulation;
			result.m_iFIASupportPopulation = population.m_iFIASupportPopulation;
		}
		result.m_iSupportPercent = CalculatePopulationSupportPercent(result.m_iFIASupportPopulation, result.m_iRemainingPopulation);
		result.m_iAirfieldsTotal = CountZonesOfType(state, HST_EZoneType.HST_ZONE_AIRFIELD);
		result.m_iAirfieldsControlled = CountTypeOwnedBy(state, HST_EZoneType.HST_ZONE_AIRFIELD, resistanceFactionKey);
		result.m_sOutcomeMode = ResolveOutcomeMode(balance);

		if (!IsCampaignOutcomeCheckDue(state, balance))
			return result;
		if (balance.m_bPopulationOutcomeEnabled
			&& !result.m_bPopulationAuthorityValid)
		{
			result.m_sReason = "blocked: canonical town population authority unavailable";
			return result;
		}

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

			bool populationSupportReady
				= HST_TownInfluenceService.MeetsPopulationSupportThreshold(
					result.m_iFIASupportPopulation,
					result.m_iRemainingPopulation,
					balance.m_iVictoryPopulationSupportPercent);
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
			return "Partisan campaign end | state not ready";

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
			"Partisan campaign end | phase %1 | ended %2 | reason %3 | summary %4 | control %5 pct | score %6/%7 | war %8 | FIA zones %9",
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
		if (!m_TownInfluence)
			return 0;
		HST_TownPopulationAggregate aggregate
			= m_TownInfluence.BuildPopulationAggregate(state);
		if (!aggregate || !aggregate.m_bAuthorityValid)
			return 0;
		return aggregate.m_iInitialPopulation;
	}

	int GetTotalRemainingPopulation(HST_CampaignState state)
	{
		if (!m_TownInfluence)
			return 0;
		HST_TownPopulationAggregate aggregate
			= m_TownInfluence.BuildPopulationAggregate(state);
		if (!aggregate || !aggregate.m_bAuthorityValid)
			return 0;
		return aggregate.m_iRemainingPopulation;
	}

	int GetTotalKilledPopulation(HST_CampaignState state)
	{
		if (!m_TownInfluence)
			return 0;
		HST_TownPopulationAggregate aggregate
			= m_TownInfluence.BuildPopulationAggregate(state);
		if (!aggregate || !aggregate.m_bAuthorityValid)
			return 0;
		return aggregate.m_iDestroyedPopulation;
	}

	int GetTotalFIASupportPopulation(HST_CampaignState state)
	{
		if (!m_TownInfluence)
			return 0;
		HST_TownPopulationAggregate aggregate
			= m_TownInfluence.BuildPopulationAggregate(state);
		if (!aggregate || !aggregate.m_bAuthorityValid)
			return 0;
		return aggregate.m_iFIASupportPopulation;
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
