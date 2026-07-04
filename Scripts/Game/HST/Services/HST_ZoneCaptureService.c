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
	int m_iFriendlyVehicleCountNearby;
	int m_iEnemyCountNearby;
	int m_iEnemyVehicleCountNearby;
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
}

class HST_ZoneCaptureService
{
	static const int CAPTURE_PROGRESS_REQUIRED = 100;
	static const int CAPTURE_PROGRESS_PER_SECOND = 2;
	static const int CAPTURE_DECAY_PER_SECOND = 1;
	static const int CONQUEST_OBJECTIVE_PROGRESS_CAP = 90;

	protected ref array<ref HST_ZoneCaptureNotification> m_aPendingNotifications = {};
	protected ref array<string> m_aNotificationKeys = {};

	bool CaptureZone(HST_CampaignState state, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, string factionKey, string resistanceFactionKey = "FIA")
	{
		if (!state || !strategic || !economy || !balance)
			return false;

		return strategic.SetZoneOwner(state, economy, balance, zoneId, factionKey, resistanceFactionKey);
	}

	bool CaptureForResistance(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, int supportReward, HST_GarrisonService garrisons = null, HST_EnemyCommanderService enemyCommander = null, HST_EnemyDirectorService enemyDirector = null, HST_SupportRequestService support = null)
	{
		if (!state || !preset || !strategic || !economy || !balance)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone)
			return false;

		string oldOwner = zone.m_sOwnerFactionKey;
		if (!CaptureZone(state, strategic, economy, balance, zoneId, preset.m_sResistanceFactionKey, preset.m_sResistanceFactionKey))
			return false;

		EnsureCapturedZoneResistanceGarrison(state, preset, garrisons, zone);

		if (!oldOwner.IsEmpty() && oldOwner != preset.m_sResistanceFactionKey)
		{
			int aggression = ResolveCaptureAggression(balance, zone);
			economy.AddAggression(state, oldOwner, aggression);
			TryQueueCounterattack(state, preset, balance, enemyCommander, enemyDirector, support, oldOwner, zone);
		}

		ApplyLinkedTownSupport(state, zone, supportReward);
		QueueNotification("secured", zone, "success", CaptureTitle(zone, "secured"), string.Format("%1 secured by FIA.", ZoneLabel(zone)));
		Print(string.Format("h-istasi capture | secured %1 | previous owner %2 | support reward %3", zone.m_sZoneId, oldOwner, supportReward));
		return true;
	}

	bool AddResistanceCaptureProgress(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, string zoneId, int amount, int supportReward = 10, HST_GarrisonService garrisons = null, HST_EnemyCommanderService enemyCommander = null, HST_EnemyDirectorService enemyDirector = null, HST_SupportRequestService support = null)
	{
		if (!state || !preset || amount <= 0)
			return false;

		HST_ZoneState zone = state.FindZone(zoneId);
		if (!zone || zone.m_sOwnerFactionKey == preset.m_sResistanceFactionKey || !IsDirectlyCapturableZone(zone))
			return false;

		int required = ResolveCaptureProgressRequired(balance);
		zone.m_iResistanceCaptureProgress = Math.Min(required, Math.Max(0, zone.m_iResistanceCaptureProgress + amount));
		Print(string.Format("h-istasi capture | zone %1 resistance progress %2/%3", zoneId, zone.m_iResistanceCaptureProgress, required));
		if (HasIncompleteConquestMission(state, zone) && zone.m_iResistanceCaptureProgress >= Math.Min(CONQUEST_OBJECTIVE_PROGRESS_CAP, required - 1))
		{
			zone.m_iResistanceCaptureProgress = Math.Min(CONQUEST_OBJECTIVE_PROGRESS_CAP, required - 1);
			return true;
		}

		if (zone.m_iResistanceCaptureProgress < required)
			return true;

		return CaptureForResistance(state, preset, strategic, economy, balance, zoneId, supportReward, garrisons, enemyCommander, enemyDirector, support);
	}

	bool TickContestedCapture(HST_CampaignState state, HST_CampaignPreset preset, HST_StrategicService strategic, HST_EconomyService economy, HST_BalanceConfig balance, HST_GarrisonService garrisons, HST_EnemyCommanderService enemyCommander, HST_EnemyDirectorService enemyDirector, HST_SupportRequestService support, int elapsedSeconds)
	{
		if (!state || !preset || !strategic || !economy || !balance || elapsedSeconds <= 0)
			return false;

		bool changed;
		string resistanceFactionKey = preset.m_sResistanceFactionKey;
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

			HST_ZoneCaptureStatus status = BuildCaptureStatus(state, preset, balance, zone);
			if (status.m_iFIACountNearby <= 0)
			{
				changed = DecayCaptureProgress(zone, balance, elapsedSeconds) || changed;
				continue;
			}

			status.m_bContested = true;
			QueueNotification("contested", zone, "warning", CaptureTitle(zone, "contested"), string.Format("%1 contested by FIA.", ZoneLabel(zone)));

			if (status.m_sBlockedReason == "enemies remain" || status.m_sBlockedReason == "hostile vehicles remain")
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

	HST_ZoneCaptureStatus BuildCaptureStatus(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, HST_ZoneState zone)
	{
		HST_ZoneCaptureStatus status = new HST_ZoneCaptureStatus();
		if (!zone)
			return status;

		status.m_sZoneId = zone.m_sZoneId;
		status.m_sZoneName = ZoneLabel(zone);
		status.m_sOwnerFactionKey = zone.m_sOwnerFactionKey;
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

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		status.m_iPlayerCountNearby = CountLivingPlayersInCaptureRadius(zone, balance);
		int friendlyInfantry;
		int friendlyVehicles;
		CountFriendlyForcesInCaptureRadius(state, zone, balance, resistanceFactionKey, friendlyInfantry, friendlyVehicles);
		status.m_iFriendlyInfantryCountNearby = friendlyInfantry;
		status.m_iFriendlyVehicleCountNearby = friendlyVehicles;
		status.m_iFIACountNearby = status.m_iPlayerCountNearby + status.m_iFriendlyInfantryCountNearby;
		int enemyInfantry;
		int enemyVehicles;
		CountHostilesInCaptureRadius(state, zone, balance, resistanceFactionKey, enemyInfantry, enemyVehicles);
		status.m_iEnemyCountNearby = enemyInfantry;
		status.m_iEnemyVehicleCountNearby = enemyVehicles;
		status.m_bContested = status.m_iFIACountNearby > 0 && zone.m_sOwnerFactionKey != resistanceFactionKey;
		status.m_bConquestGated = HasIncompleteConquestMission(state, zone) && zone.m_iResistanceCaptureProgress >= Math.Min(CONQUEST_OBJECTIVE_PROGRESS_CAP, ResolveCaptureProgressRequired(balance) - 1);

		if (zone.m_sOwnerFactionKey == resistanceFactionKey)
			status.m_sBlockedReason = "already FIA";
		else if (status.m_iFIACountNearby <= 0)
			status.m_sBlockedReason = "no FIA in radius";
		else if (status.m_iEnemyVehicleCountNearby > 0)
			status.m_sBlockedReason = "hostile vehicles remain";
		else if (status.m_iEnemyCountNearby > 0)
			status.m_sBlockedReason = "enemies remain";
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

	string BuildCaptureReport(HST_CampaignState state, HST_CampaignPreset preset, HST_BalanceConfig balance, int maxRows = 20)
	{
		if (!state)
			return "h-istasi capture | campaign state not ready";

		string report = "h-istasi capture | strategic capture report";
		int emitted;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || emitted >= maxRows)
				continue;

			HST_ZoneCaptureStatus status = BuildCaptureStatus(state, preset, balance, zone);
			if (!status.m_bCapturable && zone.m_iResistanceCaptureProgress <= 0)
				continue;

			string blocked = status.m_sBlockedReason;
			if (blocked.IsEmpty())
				blocked = "capturing";

			string row = string.Format(
				"\n%1 | owner %2 | capturable %3 | radius %4m",
				status.m_sZoneName,
				status.m_sOwnerFactionKey,
				status.m_bCapturable,
				status.m_iCaptureRadiusMeters
			);
			row = row + string.Format(
				" | players %1 | FIA AI %2 | FIA veh %3 | enemy %4 | enemy veh %5",
				status.m_iPlayerCountNearby,
				status.m_iFriendlyInfantryCountNearby,
				status.m_iFriendlyVehicleCountNearby,
				status.m_iEnemyCountNearby,
				status.m_iEnemyVehicleCountNearby
			);
			row = row + string.Format(
				" | progress %1/%2 (%3 percent) | %4",
				status.m_iRawProgress,
				status.m_iRequiredProgress,
				status.m_iProgressPercent,
				blocked
			);
			report = report + row;

			emitted++;
		}

		if (emitted == 0)
			report = report + "\nno capturable or progressing zones";

		return report;
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

	protected int CountLivingPlayersInCaptureRadius(HST_ZoneState zone, HST_BalanceConfig balance)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager || !zone)
			return 0;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		float radiusSq = ResolveCaptureRadius(zone, balance) * ResolveCaptureRadius(zone, balance);
		int count;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingEntity(playerEntity))
				continue;
			if (DistanceSq2D(playerEntity.GetOrigin(), zone.m_vPosition) <= radiusSq)
				count++;
		}

		return count;
	}

	protected void CountFriendlyForcesInCaptureRadius(HST_CampaignState state, HST_ZoneState zone, HST_BalanceConfig balance, string resistanceFactionKey, out int friendlyInfantry, out int friendlyVehicles)
	{
		friendlyInfantry = 0;
		friendlyVehicles = 0;
		if (!state || !zone || resistanceFactionKey.IsEmpty())
			return;

		float radius = ResolveCaptureRadius(zone, balance);
		float radiusSq = radius * radius;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sFactionKey != resistanceFactionKey)
				continue;
			if (activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == "folded")
				continue;
			if (DistanceSq2D(activeGroup.m_vPosition, zone.m_vPosition) > radiusSq)
				continue;

			friendlyInfantry += Math.Max(0, activeGroup.m_iSurvivorInfantryCount);
			friendlyVehicles += Math.Max(0, activeGroup.m_iSurvivorVehicleCount);
		}
	}

	protected void CountHostilesInCaptureRadius(HST_CampaignState state, HST_ZoneState zone, HST_BalanceConfig balance, string resistanceFactionKey, out int hostileInfantry, out int hostileVehicles)
	{
		hostileInfantry = 0;
		hostileVehicles = 0;
		if (!state || !zone)
			return;

		float radiusSq = ResolveCaptureRadius(zone, balance) * ResolveCaptureRadius(zone, balance);
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_sFactionKey == resistanceFactionKey)
				continue;
			if (activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == "folded")
				continue;
			if (DistanceSq2D(activeGroup.m_vPosition, zone.m_vPosition) > radiusSq)
				continue;

			hostileInfantry += Math.Max(0, activeGroup.m_iSurvivorInfantryCount);
			hostileVehicles += Math.Max(0, activeGroup.m_iSurvivorVehicleCount);
		}
	}

	protected bool HasIncompleteConquestMission(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone)
			return false;

		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE || mission.m_sTargetZoneId != zone.m_sZoneId)
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

	protected void ApplyLinkedTownSupport(HST_CampaignState state, HST_ZoneState capturedZone, int supportReward)
	{
		if (!state || !capturedZone || supportReward == 0)
			return;

		array<string> appliedZoneIds = {};
		foreach (string linkedZoneId : capturedZone.m_aLinkedZoneIds)
			ApplyTownSupportById(state, linkedZoneId, supportReward, appliedZoneIds);

		ApplyNearbyTownSupport(state, capturedZone, supportReward, appliedZoneIds);
	}

	protected void ApplyTownSupportById(HST_CampaignState state, string zoneId, int supportReward, notnull array<string> appliedZoneIds)
	{
		if (!state || zoneId.IsEmpty() || appliedZoneIds.Contains(zoneId))
			return;

		HST_ZoneState linkedZone = state.FindZone(zoneId);
		if (!linkedZone || linkedZone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
			return;

		linkedZone.m_iSupport = Math.Max(-100, Math.Min(100, linkedZone.m_iSupport + supportReward));
		appliedZoneIds.Insert(zoneId);
	}

	protected void ApplyNearbyTownSupport(HST_CampaignState state, HST_ZoneState capturedZone, int supportReward, notnull array<string> appliedZoneIds)
	{
		if (!state || !capturedZone)
			return;

		float supportRadiusSq = 1500 * 1500;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN || appliedZoneIds.Contains(zone.m_sZoneId))
				continue;

			if (DistanceSq2D(zone.m_vPosition, capturedZone.m_vPosition) > supportRadiusSq)
				continue;

			zone.m_iSupport = Math.Max(-100, Math.Min(100, zone.m_iSupport + supportReward));
			appliedZoneIds.Insert(zone.m_sZoneId);
		}
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

	protected void QueueNotification(string keySuffix, HST_ZoneState zone, string severity, string title, string message)
	{
		if (!zone || keySuffix.IsEmpty())
			return;

		string key = zone.m_sZoneId + "_" + keySuffix;
		if (m_aNotificationKeys.Contains(key))
			return;

		m_aNotificationKeys.Insert(key);
		HST_ZoneCaptureNotification notification = new HST_ZoneCaptureNotification();
		notification.m_sEventId = "capture_" + key;
		notification.m_sTitle = title;
		notification.m_sMessage = message;
		notification.m_sZoneId = zone.m_sZoneId;
		notification.m_sSeverity = severity;
		notification.m_vPosition = zone.m_vPosition;
		m_aPendingNotifications.Insert(notification);
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
		if (controlledEntity)
			return controlledEntity;

		return SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
	}

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
