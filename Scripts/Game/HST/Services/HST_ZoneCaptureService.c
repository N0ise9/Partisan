class HST_ZoneCaptureStatus
{
	string m_sZoneId;
	string m_sZoneName;
	string m_sOwnerFactionKey;
	bool m_bContested;
	bool m_bCapturable;
	int m_iCaptureRadiusMeters;
	int m_iFIACountNearby;
	int m_iPlayerCountNearby;
	int m_iFriendlyInfantryCountNearby;
	int m_iFriendlyInfantryStrengthNearby;
	int m_iFriendlyVehicleCountNearby;
	int m_iEnemyCountNearby;
	int m_iEnemyVehicleCountNearby;
	HST_ECombatPresenceState m_eCombatPresenceState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COLD;
	int m_iCombatPresenceCoolingSecondsRemaining;
	string m_sCombatPresenceReason;
	int m_iTrainingQualityBonusPercent;
	int m_iRequiredProgress;
	int m_iRawProgress;
	int m_iProgressPercent;
	bool m_bConquestGated;
	string m_sBlockedReason;
}

class HST_ZoneCaptureNotification
{
	string m_sEventId;
	string m_sTitle;
	string m_sMessage;
	string m_sZoneId;
	string m_sSeverity;
	vector m_vPosition;
	bool m_bOwnershipTransition;
}

class HST_ZoneCaptureService
{
	static const int CAPTURE_PROGRESS_REQUIRED = 100;
	static const int CAPTURE_PROGRESS_PER_SECOND = 2;
	static const int CAPTURE_DECAY_PER_SECOND = 1;
	static const int CONQUEST_OBJECTIVE_PROGRESS_CAP = 90;

	protected ref array<ref HST_ZoneCaptureNotification> m_aPendingNotifications = {};
	protected ref array<string> m_aNotificationKeys = {};
	protected ref array<int> m_aPlayerIdScratch = {};
	protected ref array<IEntity> m_aLivingPlayerEntityScratch = {};
	protected ref HST_CombatPresenceService m_CombatPresence = new HST_CombatPresenceService();
	protected HST_OwnershipTransitionService m_OwnershipTransitions;

	void SetOwnershipTransitionService(HST_OwnershipTransitionService ownershipTransitions)
	{
		m_OwnershipTransitions = ownershipTransitions;
	}

	void SetCombatPresenceService(HST_CombatPresenceService combatPresence)
	{
		if (combatPresence)
			m_CombatPresence = combatPresence;
	}

	bool CaptureZone(HST_CampaignState state, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, string factionKey, string resistanceFactionKey = "FIA")
	{
		if (!state || !m_OwnershipTransitions)
			return false;
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return false;
		string sourceId = string.Format("direct_%1_%2", zone.m_sZoneId, Math.Max(1, zone.m_iOwnershipRevision));
		HST_OwnershipTransitionRequest request = m_OwnershipTransitions.BuildRequest(
			state,
			zoneId,
			factionKey,
			"military_capture",
			"zone_capture",
			sourceId,
			"direct zone capture",
			0,
			"ownership_capture_" + sourceId);
		request.m_bApplyEnemyConsequences = factionKey == resistanceFactionKey;
		HST_OwnershipTransitionResult result = m_OwnershipTransitions.Apply(state, request);
		return result && result.m_bAccepted && result.m_bCompleted;
	}

	bool CaptureForResistance(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, int supportReward, HST_GarrisonService garrisons = null, HST_EnemyCommanderService enemyCommander = null, HST_EnemyDirectorService enemyDirector = null, HST_SupportRequestService support = null, string eventSourceId = "")
	{
		HST_OwnershipTransitionResult result = CaptureForResistanceDetailed(
			state,
			preset,
			strategic,
			economy,
			balance,
			zoneId,
			supportReward,
			garrisons,
			enemyCommander,
			enemyDirector,
			support,
			eventSourceId);
		return result && result.m_bAccepted && result.m_bCompleted;
	}

	HST_OwnershipTransitionResult CaptureForResistanceDetailed(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, int supportReward, HST_GarrisonService garrisons = null, HST_EnemyCommanderService enemyCommander = null, HST_EnemyDirectorService enemyDirector = null, HST_SupportRequestService support = null, string eventSourceId = "")
	{
		string failure;
		HST_OwnershipTransitionRequest request = BuildResistanceCaptureRequest(
			state,
			preset,
			zoneId,
			supportReward,
			eventSourceId,
			failure);
		if (!request)
			return BuildRejectedOwnershipResult(failure);
		return m_OwnershipTransitions.Apply(state, request);
	}

	bool CanCaptureForResistanceDetailed(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string zoneId,
		int supportReward,
		string eventSourceId,
		out string failure)
	{
		HST_OwnershipTransitionRequest request = BuildResistanceCaptureRequest(
			state,
			preset,
			zoneId,
			supportReward,
			eventSourceId,
			failure);
		if (!request)
			return false;
		return m_OwnershipTransitions.CanApply(state, request, failure);
	}

	protected HST_OwnershipTransitionRequest BuildResistanceCaptureRequest(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string zoneId,
		int supportReward,
		string eventSourceId,
		out string failure)
	{
		failure = "";
		if (!state || !preset || !m_OwnershipTransitions)
		{
			failure = "capture state, preset, or ownership authority is unavailable";
			return null;
		}
		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
		{
			failure = "capture zone was not found";
			return null;
		}
		if (zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey)
		{
			failure = "capture zone is already resistance-owned";
			return null;
		}

		string cause = "military_capture";
		string sourceType = "zone_capture";
		string sourceId = eventSourceId;
		HST_ActiveMissionState sourceMission;
		if (!sourceId.IsEmpty())
			sourceMission = state.FindActiveMission(sourceId);
		if (sourceMission)
		{
			cause = "mission_capture";
			sourceType = "mission";
		}
		else if (sourceId.IsEmpty())
			sourceId = string.Format(
				"presence_%1_%2",
				zone.m_sZoneId,
				Math.Max(1, zone.m_iOwnershipRevision));
		return m_OwnershipTransitions.BuildRequest(
			state,
			zoneId,
			preset.m_sResistanceFactionKey,
			cause,
			sourceType,
			sourceId,
			"zone captured by resistance",
			supportReward,
			"ownership_capture_" + zone.m_sZoneId + "_" + sourceId);
	}

	bool AddResistanceCaptureProgress(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, int amount, int supportReward = 10, HST_GarrisonService garrisons = null, HST_EnemyCommanderService enemyCommander = null, HST_EnemyDirectorService enemyDirector = null, HST_SupportRequestService support = null, string eventSourceId = "")
	{
		if (!state || !preset || amount <= 0)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone || zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey || !IsDirectlyCapturableZone(zone))
			return false;

		int required = ResolveCaptureProgressRequired(balance);
		zone.m_iResistanceCaptureProgress = Math.Min(required, Math.Max(0, zone.m_iResistanceCaptureProgress + amount));
		Print(string.Format("Partisan capture | zone %1 resistance progress %2/%3", zoneId, zone.m_iResistanceCaptureProgress, required));
		if (HasIncompleteConquestMission(state, zone) && zone.m_iResistanceCaptureProgress >= Math.Min(CONQUEST_OBJECTIVE_PROGRESS_CAP, required - 1))
		{
			zone.m_iResistanceCaptureProgress = Math.Min(CONQUEST_OBJECTIVE_PROGRESS_CAP, required - 1);
			return true;
		}

		if (zone.m_iResistanceCaptureProgress < required)
			return true;

		HST_OwnershipTransitionResult captureResult = CaptureForResistanceDetailed(
			state,
			preset,
			strategic,
			economy,
			balance,
			zoneId,
			supportReward,
			garrisons,
			enemyCommander,
			enemyDirector,
			support,
			eventSourceId);
		return captureResult && captureResult.m_bAccepted;
	}

	protected HST_OwnershipTransitionResult BuildRejectedOwnershipResult(string failureReason)
	{
		HST_OwnershipTransitionResult result = new HST_OwnershipTransitionResult();
		result.m_sFailureReason = failureReason;
		return result;
	}

	bool TickContestedCapture(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, HST_GarrisonService garrisons, HST_EnemyCommanderService enemyCommander, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService support, int elapsedSeconds)
	{
		if (!state || !preset || !strategic || !economy || !balance || elapsedSeconds <= 0)
			return false;

		bool changed;
		string resistanceFactionKey = preset.m_sResistanceFactionKey;
		PrepareLivingPlayerScratch();
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_sOwnerFactionKey == resistanceFactionKey || zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
				continue;

			if (!IsDirectlyCapturableZone(zone))
			{
				if (zone.m_iResistanceCaptureProgress > 0)
					changed = DecayCaptureProgress(zone, balance, elapsedSeconds) || changed;
				continue;
			}

			HST_ZoneCaptureStatus status = BuildCaptureStatus(
				state,
				preset,
				balance,
				zone,
				"",
				false,
				true,
				true);
			if (status.m_iFIACountNearby <= 0)
			{
				changed = DecayCaptureProgress(zone, balance, elapsedSeconds) || changed;
				continue;
			}

			status.m_bContested = true;
			QueueNotification("contested", zone, "warning", CaptureTitle(zone, "contested"), string.Format("%1 contested by FIA.", ZoneLabel(zone)));

			if (status.m_sBlockedReason == "combat-presence authority unavailable")
			{
				QueueNotification("blocked_presence", zone, "warning", "Capture blocked: combat authority unavailable", string.Format("%1 cannot be secured until combat-presence authority is available.", ZoneLabel(zone)));
				changed = DecayCaptureProgress(zone, balance, elapsedSeconds) || changed;
				continue;
			}
			if (status.m_sBlockedReason == "enemies remain" || status.m_sBlockedReason == "hostile vehicles remain"
				|| status.m_sBlockedReason == "combat area cooling")
			{
				QueueNotification("blocked_enemies", zone, "warning", "Capture blocked: enemies remain", string.Format("%1 cannot be secured while hostile forces remain.", ZoneLabel(zone)));
				changed = DecayCaptureProgress(zone, balance, elapsedSeconds) || changed;
				continue;
			}
			if (status.m_sBlockedReason == "no FIA in radius" || status.m_sBlockedReason == "already FIA" || status.m_sBlockedReason == "not a military capture target")
			{
				changed = DecayCaptureProgress(zone, balance, elapsedSeconds) || changed;
				continue;
			}

			int progressAmount = ResolveProgressAmount(balance, state, zone, elapsedSeconds);
			int required = ResolveCaptureProgressRequired(balance);
			if (HasIncompleteConquestMission(state, zone) && zone.m_iResistanceCaptureProgress + progressAmount >= CONQUEST_OBJECTIVE_PROGRESS_CAP)
			{
				int cappedProgress = Math.Min(CONQUEST_OBJECTIVE_PROGRESS_CAP, required - 1);
				if (zone.m_iResistanceCaptureProgress < cappedProgress)
				{
					zone.m_iResistanceCaptureProgress = cappedProgress;
					changed = true;
				}
				continue;
			}

			changed = AddResistanceCaptureProgress(state, preset, strategic, economy, balance, zone.m_sZoneId, progressAmount, 5, garrisons, enemyCommander, enemyDirector, support) || changed;
		}

		return changed;
	}

	HST_ZoneCaptureStatus BuildCaptureStatus(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		string ownerFactionKeyOverride = "",
		bool useOwnerFactionKeyOverride = false,
		bool skipHostileQueryWithoutResistancePresence = false,
		bool reusePreparedPlayerScratch = false)
	{
		HST_ZoneCaptureStatus status = new HST_ZoneCaptureStatus();
		if (!zone)
			return status;

		string effectiveOwnerFactionKey = zone.m_sOwnerFactionKey;
		if (useOwnerFactionKeyOverride)
			effectiveOwnerFactionKey = ownerFactionKeyOverride;
		status.m_sZoneId = zone.m_sZoneId;
		status.m_sZoneName = ZoneLabel(zone);
		status.m_sOwnerFactionKey = effectiveOwnerFactionKey;
		status.m_bCapturable = IsDirectlyCapturableZone(zone);
		status.m_iCaptureRadiusMeters = ResolveCaptureRadius(zone, balance);
		status.m_iRequiredProgress = ResolveCaptureProgressRequired(balance);
		status.m_iRawProgress = Math.Max(0, zone.m_iResistanceCaptureProgress);
		status.m_iProgressPercent = ResolveProgressPercent(zone, balance);
		if (!status.m_bCapturable)
		{
			status.m_sBlockedReason = "not a military capture target";
			return status;
		}
		if (useOwnerFactionKeyOverride && effectiveOwnerFactionKey.IsEmpty())
		{
			status.m_sBlockedReason = "ownership publication unavailable";
			return status;
		}

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		status.m_iPlayerCountNearby = CountLivingPlayersInCaptureRadius(
			zone,
			balance,
			reusePreparedPlayerScratch);
		int friendlyInfantry;
		int friendlyInfantryStrength;
		int friendlyVehicles;
		CountFriendlyForcesInCaptureRadius(state, zone, balance, resistanceFactionKey, friendlyInfantry, friendlyInfantryStrength, friendlyVehicles);
		status.m_iFriendlyInfantryCountNearby = friendlyInfantry;
		status.m_iFriendlyInfantryStrengthNearby = friendlyInfantryStrength;
		status.m_iFriendlyVehicleCountNearby = friendlyVehicles;
		if (state)
			status.m_iTrainingQualityBonusPercent = HST_RecruitmentService.ResolveTrainingQualityBonusPercentForLevel(state.m_iTrainingLevel);
		status.m_iFIACountNearby = status.m_iPlayerCountNearby + status.m_iFriendlyInfantryStrengthNearby;
		status.m_bContested = status.m_iFIACountNearby > 0 && effectiveOwnerFactionKey != resistanceFactionKey;
		status.m_bConquestGated = HasIncompleteConquestMission(state, zone) && zone.m_iResistanceCaptureProgress >= Math.Min(CONQUEST_OBJECTIVE_PROGRESS_CAP, ResolveCaptureProgressRequired(balance) - 1);
		if (skipHostileQueryWithoutResistancePresence)
		{
			if (effectiveOwnerFactionKey == resistanceFactionKey)
			{
				status.m_sBlockedReason = "already FIA";
				return status;
			}
			if (status.m_iFIACountNearby <= 0)
			{
				status.m_sBlockedReason = "no FIA in radius";
				return status;
			}
		}
		HST_CombatPresenceResult hostilePresence = m_CombatPresence.QueryZoneHostilePresence(
			state,
			preset,
			resistanceFactionKey,
			zone,
			true);
		if (hostilePresence && hostilePresence.m_bQueryValid)
		{
			status.m_iEnemyCountNearby = hostilePresence.m_iInfantryCount
				+ hostilePresence.m_iStaticOperatorCount;
			status.m_iEnemyVehicleCountNearby = hostilePresence.m_iMannedVehicleCount;
			status.m_eCombatPresenceState = hostilePresence.m_eState;
			status.m_iCombatPresenceCoolingSecondsRemaining = hostilePresence.m_iCoolingRemainingSeconds;
			status.m_sCombatPresenceReason = hostilePresence.m_sReason;
		}
		else
			status.m_sCombatPresenceReason = "combat-presence authority unavailable";
		if (effectiveOwnerFactionKey == resistanceFactionKey)
			status.m_sBlockedReason = "already FIA";
		else if (status.m_iFIACountNearby <= 0)
			status.m_sBlockedReason = "no FIA in radius";
		else if (!hostilePresence || !hostilePresence.m_bQueryValid)
			status.m_sBlockedReason = "combat-presence authority unavailable";
		else if (status.m_iEnemyVehicleCountNearby > 0)
			status.m_sBlockedReason = "hostile vehicles remain";
		else if (status.m_iEnemyCountNearby > 0)
			status.m_sBlockedReason = "enemies remain";
		else if (status.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING)
			status.m_sBlockedReason = "combat area cooling";
		else if (status.m_bConquestGated)
			status.m_sBlockedReason = "conquest objective incomplete";
		else
			status.m_sBlockedReason = "";

		return status;
	}

	bool HasIncompleteConquestMissionForZone(HST_CampaignState state, HST_ZoneState zone)
	{
		return HasIncompleteConquestMission(state, zone);
	}

	string BuildCaptureReport(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		HST_MapMarkerService markers = null,
		int maxRows = 20)
	{
		if (!state)
			return "Partisan capture | campaign state not ready";

		string report = "Partisan capture | strategic capture report";
		int emitted;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || emitted >= maxRows)
				continue;

			string publishedOwnerFactionKey = ResolvePublishedZoneOwnerFactionKey(state, zone, markers);
			HST_ZoneCaptureStatus status = BuildCaptureStatus(
				state,
				preset,
				balance,
				zone,
				publishedOwnerFactionKey,
				markers != null);
			if (!status.m_bCapturable && zone.m_iResistanceCaptureProgress <= 0)
				continue;

			string blocked = status.m_sBlockedReason;
			if (blocked.IsEmpty())
				blocked = "capturing";

			string publishedOwnerLabel = status.m_sOwnerFactionKey;
			if (publishedOwnerLabel.IsEmpty())
				publishedOwnerLabel = "publication unavailable";
			string row = string.Format(
				"\n%1 | owner %2 | capturable %3 | radius %4m",
				status.m_sZoneName,
				publishedOwnerLabel,
				status.m_bCapturable,
				status.m_iCaptureRadiusMeters
			);
			row = row + string.Format(
				" | players %1 | FIA AI %2 strength %3 | FIA veh %4 | enemy %5 | enemy veh %6",
				status.m_iPlayerCountNearby,
				status.m_iFriendlyInfantryCountNearby,
				status.m_iFriendlyInfantryStrengthNearby,
				status.m_iFriendlyVehicleCountNearby,
				status.m_iEnemyCountNearby,
				status.m_iEnemyVehicleCountNearby
			);
			if (status.m_iTrainingQualityBonusPercent > 0)
				row = row + string.Format(" | training quality +%1 pct", status.m_iTrainingQualityBonusPercent);
			row = row + string.Format(
				" | progress %1/%2 (%3 percent) | %4",
				status.m_iRawProgress,
				status.m_iRequiredProgress,
				status.m_iProgressPercent,
				blocked
			);
			row = row + string.Format(
				" | combat %1 cooling %2s",
				HST_CombatPresenceService.StateName(status.m_eCombatPresenceState),
				status.m_iCombatPresenceCoolingSecondsRemaining);
			report = report + row;

			emitted++;
		}

		if (emitted == 0)
			report = report + "\nno capturable or progressing zones";

		return report;
	}

	protected string ResolvePublishedZoneOwnerFactionKey(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_MapMarkerService markers)
	{
		if (!zone)
			return "";
		if (!markers)
			return zone.m_sOwnerFactionKey;

		string publishedOwnerFactionKey;
		int publishedOwnershipRevision;
		if (!markers.ResolvePublishedZoneOwnership(
				state,
				zone,
				publishedOwnerFactionKey,
				publishedOwnershipRevision))
			return "";

		if (state)
		{
			HST_MapMarkerState retainedMarker = state.FindMapMarker("hst_zone_" + zone.m_sZoneId);
			if (retainedMarker && (!retainedMarker.m_bVisible || retainedMarker.m_bTombstone
				|| retainedMarker.m_sOwnerFactionKey != publishedOwnerFactionKey
				|| retainedMarker.m_iSourceRevision != publishedOwnershipRevision))
				return "";
		}
		return publishedOwnerFactionKey;
	}

	void DrainPendingNotifications(notnull array<ref HST_ZoneCaptureNotification> notifications)
	{
		notifications.Clear();
		foreach (HST_ZoneCaptureNotification notification : m_aPendingNotifications)
		{
			if (notification)
				notifications.Insert(notification);
		}

		m_aPendingNotifications.Clear();
	}

	protected bool DecayCaptureProgress(HST_ZoneState zone, HST_BalanceConfig balance, int elapsedSeconds)
	{
		if (!zone || zone.m_iResistanceCaptureProgress <= 0)
			return false;

		int nextProgress = Math.Max(0, zone.m_iResistanceCaptureProgress - elapsedSeconds * ResolveCaptureDecayPerSecond(balance));
		if (nextProgress == zone.m_iResistanceCaptureProgress)
			return false;

		zone.m_iResistanceCaptureProgress = nextProgress;
		return true;
	}

	protected bool IsDirectlyCapturableZone(HST_ZoneState zone)
	{
		if (!zone)
			return false;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return true;
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return true;

		return false;
	}

	protected int ResolveProgressAmount(HST_BalanceConfig balance, HST_CampaignState state, HST_ZoneState zone, int elapsedSeconds)
	{
		int amount = elapsedSeconds * ResolveCaptureProgressPerSecond(balance);
		int divisor = 100 + Math.Max(0, state.m_iWarLevel - 1) * 5 + Math.Max(0, zone.m_iPriority) * 3;
		amount = Math.Max(1, amount * 100 / divisor);
		return amount;
	}

	protected int ResolveCaptureProgressRequired(HST_BalanceConfig balance)
	{
		if (!balance || balance.m_iCaptureProgressRequired <= 0)
			return CAPTURE_PROGRESS_REQUIRED;

		return balance.m_iCaptureProgressRequired;
	}

	protected int ResolveCaptureProgressPerSecond(HST_BalanceConfig balance)
	{
		if (!balance || balance.m_iCaptureProgressPerSecond <= 0)
			return CAPTURE_PROGRESS_PER_SECOND;

		return balance.m_iCaptureProgressPerSecond;
	}

	protected int ResolveCaptureDecayPerSecond(HST_BalanceConfig balance)
	{
		if (!balance || balance.m_iCaptureDecayPerSecond <= 0)
			return CAPTURE_DECAY_PER_SECOND;

		return balance.m_iCaptureDecayPerSecond;
	}

	protected int ResolveCaptureAggression(HST_BalanceConfig balance, HST_ZoneState zone)
	{
		int baseAggression = 10;
		if (balance && balance.m_iCaptureAggressionBase > 0)
			baseAggression = balance.m_iCaptureAggressionBase;

		if (!zone)
			return baseAggression;

		return baseAggression + Math.Max(0, zone.m_iPriority / 5);
	}

	protected int ResolveCounterattackChance(HST_BalanceConfig balance, HST_CampaignState state, HST_ZoneState zone)
	{
		int chance = 45;
		if (balance && balance.m_iCaptureCounterattackChancePercent >= 0)
			chance = balance.m_iCaptureCounterattackChancePercent;

		if (state)
			chance += Math.Max(0, state.m_iWarLevel - 1) * 3;
		if (zone)
			chance += Math.Max(0, zone.m_iPriority / 2);

		return Math.Max(0, Math.Min(95, chance));
	}

	protected int ResolveProgressPercent(HST_ZoneState zone, HST_BalanceConfig balance)
	{
		if (!zone)
			return 0;

		return Math.Min(100, Math.Max(0, zone.m_iResistanceCaptureProgress * 100 / ResolveCaptureProgressRequired(balance)));
	}

	protected int ResolveCaptureRadius(HST_ZoneState zone, HST_BalanceConfig balance)
	{
		if (zone && zone.m_iCaptureRadiusMeters > 0)
			return zone.m_iCaptureRadiusMeters;

		if (balance && balance.m_iActivationRadiusMeters > 0)
			return balance.m_iActivationRadiusMeters;

		return 1200;
	}

	protected void PrepareLivingPlayerScratch()
	{
		m_aPlayerIdScratch.Clear();
		m_aLivingPlayerEntityScratch.Clear();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;
		playerManager.GetPlayers(m_aPlayerIdScratch);
		foreach (int playerId : m_aPlayerIdScratch)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (IsLivingEntity(playerEntity))
				m_aLivingPlayerEntityScratch.Insert(playerEntity);
		}
	}

	protected int CountLivingPlayersInCaptureRadius(
		HST_ZoneState zone,
		HST_BalanceConfig balance,
		bool reusePreparedPlayerScratch = false)
	{
		if (!zone)
			return 0;
		if (!reusePreparedPlayerScratch)
			PrepareLivingPlayerScratch();

		float radiusSq = ResolveCaptureRadius(zone, balance) * ResolveCaptureRadius(zone, balance);
		int count;
		foreach (IEntity playerEntity : m_aLivingPlayerEntityScratch)
		{
			if (!playerEntity || playerEntity.IsDeleted())
				continue;
			if (DistanceSq2D(playerEntity.GetOrigin(), zone.m_vPosition) <= radiusSq)
				count++;
		}

		return count;
	}

	protected void CountFriendlyForcesInCaptureRadius(HST_CampaignState state, HST_ZoneState zone, HST_BalanceConfig balance, string resistanceFactionKey, out int friendlyInfantry, out int friendlyInfantryStrength, out int friendlyVehicles)
	{
		friendlyInfantry = 0;
		friendlyInfantryStrength = 0;
		friendlyVehicles = 0;
		if (!state || !zone || resistanceFactionKey.IsEmpty())
			return;

		float radius = ResolveCaptureRadius(zone, balance);
		if (!m_CombatPresence.HasExactFactionContributionNear(
			state,
			resistanceFactionKey,
			zone.m_vPosition,
			radius))
			return;
		HST_CombatPresenceResult presence = m_CombatPresence.QueryExactFactionPresenceNear(
			state,
			resistanceFactionKey,
			zone.m_vPosition,
			radius);
		if (!presence || !presence.m_bQueryValid)
			return;
		friendlyInfantry = presence.m_iInfantryCount + presence.m_iStaticOperatorCount;
		friendlyInfantryStrength = HST_RecruitmentService.ResolveTrainingEffectiveInfantryStrengthForLevel(
			friendlyInfantry,
			state.m_iTrainingLevel);
		friendlyVehicles = presence.m_iMannedVehicleCount;
	}

	void QueueOwnershipTransitionNotification(
		HST_OwnershipTransitionState transition,
		HST_ZoneState zone)
	{
		if (!transition || !zone)
			return;

		string title = ZoneLabel(zone) + " ownership changed";
		string message = string.Format(
			"%1 changed control from %2 to %3 (revision %4).",
			ZoneLabel(zone),
			transition.m_sPreviousOwnerFactionKey,
			transition.m_sNewOwnerFactionKey,
			transition.m_iAppliedRevision);
		string severity = "warning";
		if (transition.m_sNewOwnerFactionKey == "FIA")
			severity = "success";
		HST_ZoneCaptureNotification notification = QueueNotification(
			"ownership_" + transition.m_sRequestId,
			zone,
			severity,
			title,
			message);
		if (notification)
			notification.m_bOwnershipTransition = true;
	}

	protected bool HasIncompleteConquestMission(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return false;

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
				|| !HST_MaidensBayLocationSaveValidationService.AreEquivalentZoneIds(
					mission.m_sTargetZoneId, zone.m_sZoneId))
				continue;
			if (!IsConquestMissionId(mission.m_sMissionId))
				continue;
			if (!AreMissionObjectivesComplete(state, mission.m_sInstanceId))
				return true;
		}

		return false;
	}

	protected bool IsConquestMissionId(string missionId)
	{
		if (missionId.IsEmpty() || missionId.Length() < 9)
			return false;

		return missionId.Substring(0, 9) == "conquest_";
	}

	protected bool AreMissionObjectivesComplete(HST_CampaignState state, string instanceId)
	{
		if (!state || instanceId.IsEmpty())
			return false;

		bool found;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != instanceId)
				continue;

			found = true;
			if (!objective.m_bComplete || objective.m_bFailed)
				return false;
		}

		return found;
	}

	protected void TryQueueCounterattack(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_EnemyCommanderService enemyCommander, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService support, string factionKey, HST_ZoneState capturedZone)
	{
		if (!state || !preset || !enemyCommander || !enemyDirector || factionKey.IsEmpty() || !capturedZone)
			return;

		int chance = ResolveCounterattackChance(balance, state, capturedZone);
		bool queued = enemyCommander.TryQueueImmediateCounterattack(state, preset, enemyDirector, support, factionKey, capturedZone, chance);
		if (queued)
			QueueNotification("counterattack", capturedZone, "warning", "Enemy counterattack expected", string.Format("%1 is preparing a counterattack against %2.", factionKey, ZoneLabel(capturedZone)));
	}

	protected void EnsureCapturedZoneResistanceGarrison(HST_CampaignState state, HST_CampaignPreset preset, HST_GarrisonService garrisons, HST_ZoneState zone)
	{
		if (!state || !preset || !garrisons || !zone)
			return;

		HST_GarrisonState garrison = garrisons.FindOrCreate(state, zone.m_sZoneId, preset.m_sResistanceFactionKey);
		if (!garrison)
			return;

		if (garrison.m_iInfantryCount > 0 || garrison.m_iVehicleCount > 0)
			return;

		int starterInfantry;
		if (zone.m_iGarrisonSlots > 0)
			starterInfantry = Math.Min(2, zone.m_iGarrisonSlots);

		if (starterInfantry <= 0)
			return;

		garrison.m_iInfantryCount = starterInfantry;
	}

	protected HST_ZoneCaptureNotification QueueNotification(string keySuffix, HST_ZoneState zone, string severity, string title, string message)
	{
		if (!zone || keySuffix.IsEmpty())
			return null;

		string key = zone.m_sZoneId + "_" + keySuffix;
		if (m_aNotificationKeys.Contains(key))
			return null;

		m_aNotificationKeys.Insert(key);
		HST_ZoneCaptureNotification notification = new HST_ZoneCaptureNotification();
		notification.m_sEventId = "capture_" + key;
		notification.m_sTitle = title;
		notification.m_sMessage = message;
		notification.m_sZoneId = zone.m_sZoneId;
		notification.m_sSeverity = severity;
		notification.m_vPosition = zone.m_vPosition;
		m_aPendingNotifications.Insert(notification);
		return notification;
	}

	protected string CaptureTitle(HST_ZoneState zone, string suffix)
	{
		return ZoneTypeTitle(zone) + " " + suffix;
	}

	protected string ZoneTypeTitle(HST_ZoneState zone)
	{
		if (!zone)
			return "Zone";

		if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST)
			return "Outpost";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RESOURCE)
			return "Resource";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
			return "Factory";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD)
			return "Airfield";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return "Seaport";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_RADIO_TOWER)
			return "Radio tower";

		return "Zone";
	}

	protected string ZoneLabel(HST_ZoneState zone)
	{
		if (!zone)
			return "Unknown zone";
		if (!zone.m_sDisplayName.IsEmpty())
			return zone.m_sDisplayName;

		return zone.m_sZoneId;
	}

	protected IEntity GetBestPlayerEntity(PlayerManager playerManager, int playerId)
	{
		if (!playerManager || playerId <= 0)
			return null;

		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (ChimeraCharacter.Cast(controlledEntity))
			return controlledEntity;

		IEntity mainEntity = SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
		if (ChimeraCharacter.Cast(mainEntity))
			return mainEntity;
		return null;
	}

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity)
			return false;
		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (character)
		{
			CharacterControllerComponent controller = character.GetCharacterController();
			return controller && controller.GetLifeState() == ECharacterLifeState.ALIVE
				&& !controller.IsUnconscious();
		}

		return false;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
