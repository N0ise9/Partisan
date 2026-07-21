class HST_CivilianConsequenceResult
{
	bool m_bAccepted;
	bool m_bAlreadyApplied;
	bool m_bChanged;
	bool m_bPanicActive;
	bool m_bPanicOnly;
	bool m_bDangerActive;
	string m_sEventId;
	string m_sEventKind;
	string m_sFailureReason;
	ref HST_TownInfluenceResult m_TownInfluenceResult;

	string BuildReport()
	{
		return string.Format(
			"Partisan civilian consequence | accepted %1 | replay %2 | changed %3 | panic %4 | panic-only %5 | danger %6 | kind %7 | event %8 | %9",
			m_bAccepted,
			m_bAlreadyApplied,
			m_bChanged,
			m_bPanicActive,
			m_bPanicOnly,
			m_bDangerActive,
			ReportText(m_sEventKind),
			ReportText(m_sEventId),
			ReportText(m_sFailureReason));
	}

	protected string ReportText(string value)
	{
		if (value.IsEmpty())
			return "none";
		return value;
	}
}

// Bounded, state-only authority for civilian political consequences. Native
// death, vehicle-claim, and combat observation remain adapter concerns: callers
// supply exact durable receipts and already-resolved state facts here.
class HST_CivilianConsequenceService
{
	static const int CONTRACT_VERSION = 1;
	static const int MAX_MUTABLE_REVISION = int.MAX - 16;
	static const int MAX_EPISODE_COUNT = 1000000;
	static const int MAX_OBSERVED_COUNT = 1000000;
	static const int MAX_ID_CHARACTERS = 160;
	static const int MAX_REASON_CHARACTERS = 192;
	static const int PANIC_DURATION_SECONDS = 45;
	static const int PANIC_REFRESH_THRESHOLD_SECONDS = 15;
	static const int MAX_MINOR_EXACT_RECEIPTS = 4096;
	static const string EVENT_CIVILIAN_CASUALTY = "civilian_casualty";
	static const string EVENT_CIVILIAN_VEHICLE_THEFT = "civilian_vehicle_theft";
	static const string EVENT_CIVILIAN_NEARBY_COMBAT = "civilian_nearby_combat";
	static const string KIND_CASUALTY = "civilian_casualty";
	static const string KIND_VEHICLE_THEFT = "civilian_vehicle_theft";
	static const string KIND_NEARBY_COMBAT = "civilian_nearby_combat";

	protected HST_TownInfluenceService m_TownInfluence;
	protected HST_CampaignPreset m_Preset;
	protected ref map<string, string> m_mMinorExactReceiptFingerprints
		= new map<string, string>();

	void SetTownInfluenceService(HST_TownInfluenceService townInfluence)
	{
		m_TownInfluence = townInfluence;
	}

	void SetCampaignPreset(HST_CampaignPreset preset)
	{
		m_Preset = preset;
	}

	// Minor-locality exact fingerprints are deliberately session-only. A new
	// campaign in the same process must not inherit receipts from the campaign
	// that was just reset.
	void ResetRuntimeSession()
	{
		m_mMinorExactReceiptFingerprints.Clear();
	}

	HST_CivilianConsequenceResult RegisterPedestrianCasualty(
		HST_CampaignState state,
		string zoneId,
		string exactEventId,
		string responsibleFactionKey = "",
		string sourceId = "")
	{
		HST_CivilianConsequenceResult result
			= NewResult(EVENT_CIVILIAN_CASUALTY, exactEventId);
		HST_ZoneState zone;
		string failure = ValidateCommonContext(
			state,
			zoneId,
			exactEventId,
			sourceId,
			zone);
		if (!failure.IsEmpty())
			return Reject(result, state, zone, failure, false);

		responsibleFactionKey = responsibleFactionKey.Trim();
		failure = ValidateResponsibleFaction(
			state,
			responsibleFactionKey,
			true);
		if (!failure.IsEmpty())
			return Reject(result, state, zone, failure, false);

		string attribution = ResolveAttribution(responsibleFactionKey);
		if (attribution.IsEmpty())
			return Reject(
				result,
				state,
				zone,
				"civilian casualty attribution is unavailable",
				false);

		HST_TownInfluenceCommand command = BuildCasualtyCommand(
			state,
			zone,
			exactEventId.Trim(),
			ResolveSourceId(sourceId, exactEventId),
			responsibleFactionKey,
			attribution);
		if (!command)
			return Reject(
				result,
				state,
				zone,
				"civilian casualty consequence could not be built",
				false);
		return ExecutePoliticalConsequence(
			state,
			zone,
			command,
			result,
			false,
			true,
			true,
			0);
	}

	HST_CivilianConsequenceResult RegisterCivilianVehicleTheft(
		HST_CampaignState state,
		string zoneId,
		string exactEventId,
		string responsibleFactionKey,
		string sourceId = "")
	{
		HST_CivilianConsequenceResult result
			= NewResult(EVENT_CIVILIAN_VEHICLE_THEFT, exactEventId);
		HST_ZoneState zone;
		string failure = ValidateCommonContext(
			state,
			zoneId,
			exactEventId,
			sourceId,
			zone);
		if (!failure.IsEmpty())
			return Reject(result, state, zone, failure, false);

		responsibleFactionKey = responsibleFactionKey.Trim();
		failure = ValidateResponsibleFaction(
			state,
			responsibleFactionKey,
			false);
		if (!failure.IsEmpty())
			return Reject(result, state, zone, failure, false);
		if (responsibleFactionKey != m_Preset.m_sResistanceFactionKey)
		{
			return Reject(
				result,
				state,
				zone,
				"civilian vehicle theft requires resistance attribution",
				false);
		}

		HST_TownInfluenceCommand command = BuildTheftCommand(
			state,
			zone,
			exactEventId.Trim(),
			ResolveSourceId(sourceId, exactEventId));
		if (!command)
			return Reject(
				result,
				state,
				zone,
				"civilian vehicle theft consequence could not be built",
				false);
		return ExecutePoliticalConsequence(
			state,
			zone,
			command,
			result,
			false,
			false,
			false,
			0);
	}

	// HOT is deliberately absent from this API. A real danger episode requires
	// at least one current operation or recent-fire fact from combat presence.
	HST_CivilianConsequenceResult ObserveNearbyCombat(
		HST_CampaignState state,
		string zoneId,
		int combatPresenceRevision,
		int currentOperationCount,
		int recentFireCount,
		string contributorHash = "")
	{
		HST_CivilianConsequenceResult result
			= NewResult(EVENT_CIVILIAN_NEARBY_COMBAT, "");
		HST_ZoneState zone;
		string failure = ValidateObservationContext(
			state,
			zoneId,
			combatPresenceRevision,
			currentOperationCount,
			recentFireCount,
			contributorHash,
			zone);
		if (!failure.IsEmpty())
			return Reject(result, state, zone, failure, false);

		bool danger = currentOperationCount > 0 || recentFireCount > 0;
		if (combatPresenceRevision == zone.m_iCivilianLastCombatPresenceRevision
			&& danger != zone.m_bCivilianCombatDangerActive)
		{
			return Reject(
				result,
				state,
				zone,
				"combat presence revision was reused with different danger facts",
				false);
		}
		int prospectiveEpisode = zone.m_iCivilianCombatEpisodeCount;
		bool priorReceiptChanged;
		if (zone.m_iCivilianLastAppliedCombatEpisodeCount
			< zone.m_iCivilianCombatEpisodeCount)
		{
			HST_CivilianConsequenceResult pending
				= ExecutePendingCombatEpisode(state, zone);
			if (!pending || !pending.m_bAccepted)
			{
				if (pending)
					return pending;
				return Reject(
					result,
					state,
					zone,
					"pending civilian combat episode could not be executed",
					false);
			}
			priorReceiptChanged = pending.m_bChanged;
		}
		if (danger && !zone.m_bCivilianCombatDangerActive)
		{
			if (prospectiveEpisode >= MAX_EPISODE_COUNT)
			{
				HST_CivilianConsequenceResult exhausted = Reject(
					result,
					state,
					zone,
					"civilian combat episode authority is exhausted",
					false);
				exhausted.m_bChanged = priorReceiptChanged;
				return exhausted;
			}
			prospectiveEpisode++;
		}
		if (danger
			&& zone.m_iCivilianLastAppliedCombatEpisodeCount
				< prospectiveEpisode
			&& BuildCombatEpisodeEventId(
				zone.m_sZoneId,
				prospectiveEpisode).IsEmpty())
		{
			HST_CivilianConsequenceResult invalidId = Reject(
				result,
				state,
				zone,
				"civilian combat episode id could not be built",
				false);
			invalidId.m_bChanged = priorReceiptChanged;
			return invalidId;
		}

		bool localChanged;
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (combatPresenceRevision
			> zone.m_iCivilianLastCombatPresenceRevision)
		{
			zone.m_iCivilianLastCombatPresenceRevision
				= combatPresenceRevision;
			localChanged = true;
		}

		if (!danger)
		{
			if (zone.m_bCivilianCombatDangerActive)
			{
				zone.m_bCivilianCombatDangerActive = false;
				zone.m_iCivilianDangerChangedSecond = nowSecond;
				localChanged = true;
			}
			if (zone.m_iCivilianPanicUntilSecond > 0
				&& zone.m_iCivilianPanicUntilSecond <= nowSecond)
			{
				zone.m_iCivilianPanicUntilSecond = 0;
				localChanged = true;
			}
			CommitZoneMutation(zone, localChanged);
			result.m_bAccepted = true;
			result.m_bChanged = localChanged || priorReceiptChanged;
			result.m_bDangerActive = false;
			result.m_bPanicActive
				= zone.m_iCivilianPanicUntilSecond > nowSecond;
			return result;
		}

		if (!zone.m_bCivilianCombatDangerActive)
		{
			zone.m_bCivilianCombatDangerActive = true;
			zone.m_iCivilianCombatEpisodeCount = prospectiveEpisode;
			zone.m_iCivilianDangerChangedSecond = nowSecond;
			localChanged = true;
		}
		localChanged = RefreshPanic(zone, nowSecond, false) || localChanged;
		if (zone.m_iCivilianLastAppliedCombatEpisodeCount
			== zone.m_iCivilianCombatEpisodeCount)
		{
			CommitZoneMutation(zone, localChanged);
			result.m_bAccepted = true;
			result.m_bAlreadyApplied = true;
			result.m_bChanged = localChanged || priorReceiptChanged;
			result.m_bDangerActive = true;
			result.m_bPanicActive
				= zone.m_iCivilianPanicUntilSecond > nowSecond;
			return result;
		}
		string eventId = BuildCombatEpisodeEventId(
			zone.m_sZoneId,
			zone.m_iCivilianCombatEpisodeCount);
		result.m_sEventId = eventId;
		result.m_bDangerActive = true;
		if (eventId.IsEmpty())
		{
			return Reject(
				result,
				state,
				zone,
				"civilian combat episode id could not be built",
				localChanged);
		}

		HST_TownInfluenceCommand command = BuildCombatCommand(
			zone,
			eventId);
		if (!command)
		{
			return Reject(
				result,
				state,
				zone,
				"civilian nearby-combat consequence could not be built",
				localChanged);
		}
		HST_CivilianConsequenceResult executed = ExecutePoliticalConsequence(
			state,
			zone,
			command,
			result,
			localChanged,
			true,
			false,
			zone.m_iCivilianCombatEpisodeCount);
		if (executed)
			executed.m_bChanged = executed.m_bChanged || priorReceiptChanged;
		return executed;
	}

	protected HST_CivilianConsequenceResult ExecutePendingCombatEpisode(
		HST_CampaignState state,
		HST_ZoneState zone)
	{
		HST_CivilianConsequenceResult result
			= NewResult(EVENT_CIVILIAN_NEARBY_COMBAT, "");
		if (!state || !zone
			|| zone.m_iCivilianLastAppliedCombatEpisodeCount
				>= zone.m_iCivilianCombatEpisodeCount)
		{
			return Reject(
				result,
				state,
				zone,
				"pending civilian combat episode is unavailable",
				false);
		}
		string eventId = BuildCombatEpisodeEventId(
			zone.m_sZoneId,
			zone.m_iCivilianLastAppliedCombatEpisodeCount + 1);
		result.m_sEventId = eventId;
		if (eventId.IsEmpty())
		{
			return Reject(
				result,
				state,
				zone,
				"pending civilian combat episode id could not be built",
				false);
		}
		HST_TownInfluenceCommand command = BuildCombatCommand(zone, eventId);
		if (!command)
		{
			return Reject(
				result,
				state,
				zone,
				"pending civilian nearby-combat consequence could not be built",
				false);
		}
		return ExecutePoliticalConsequence(
			state,
			zone,
			command,
			result,
			false,
			true,
			false,
			zone.m_iCivilianLastAppliedCombatEpisodeCount + 1);
	}

	static string BuildCombatEpisodeEventId(string zoneId, int episodeCount)
	{
		zoneId = zoneId.Trim();
		if (zoneId.IsEmpty() || episodeCount <= 0
			|| episodeCount > MAX_EPISODE_COUNT)
			return "";
		string eventId = string.Format(
			"civilian_combat_%1_%2",
			zoneId,
			episodeCount);
		if (eventId.Length() > MAX_ID_CHARACTERS)
			return "";
		return eventId;
	}

	protected HST_TownInfluenceCommand BuildCasualtyCommand(
		HST_CampaignState state,
		HST_ZoneState zone,
		string eventId,
		string sourceId,
		string responsibleFactionKey,
		string attribution)
	{
		if (!state || !zone || eventId.IsEmpty() || sourceId.IsEmpty())
			return null;
		HST_TownInfluenceCommand command = NewCommand(
			zone,
			eventId,
			sourceId,
			EVENT_CIVILIAN_CASUALTY,
			"civilian pedestrian casualty attributed to " + attribution);
		command.m_iPopulationDelta = -1;
		if (attribution == "resistance")
		{
			string enemyFactionKey = ResolveCurrentEnemyFactionKey(state, zone);
			if (enemyFactionKey.IsEmpty()
				|| !SetEnemySupportDelta(command, enemyFactionKey, 3))
				return null;
			command.m_iRawFIASupportDelta = -5;
			command.m_iReputationDelta = -5;
			command.m_iHeatDelta = 3;
			command.m_sAggressionFactionKey = enemyFactionKey;
			command.m_iAggressionDelta = 2;
		}
		else if (attribution == "enemy")
		{
			if (!SetEnemySupportDelta(command, responsibleFactionKey, -5))
				return null;
			command.m_iRawFIASupportDelta = 3;
			command.m_iReputationDelta = 3;
			command.m_iHeatDelta = 3;
		}
		else
		{
			command.m_iHeatDelta = 2;
		}
		command.m_bPopulationScaled = true;
		return command;
	}

	protected HST_TownInfluenceCommand BuildTheftCommand(
		HST_CampaignState state,
		HST_ZoneState zone,
		string eventId,
		string sourceId)
	{
		if (!state || !zone || eventId.IsEmpty() || sourceId.IsEmpty())
			return null;
		string enemyFactionKey = ResolveCurrentEnemyFactionKey(state, zone);
		if (enemyFactionKey.IsEmpty())
			return null;
		HST_TownInfluenceCommand command = NewCommand(
			zone,
			eventId,
			sourceId,
			EVENT_CIVILIAN_VEHICLE_THEFT,
			"civilian vehicle theft attributed to resistance");
		command.m_iRawFIASupportDelta = -2;
		if (!SetEnemySupportDelta(command, enemyFactionKey, 1))
			return null;
		command.m_iReputationDelta = -2;
		command.m_iHeatDelta = 2;
		command.m_sAggressionFactionKey = enemyFactionKey;
		command.m_iAggressionDelta = 1;
		command.m_bPopulationScaled = true;
		return command;
	}

	protected HST_TownInfluenceCommand BuildCombatCommand(
		HST_ZoneState zone,
		string eventId)
	{
		if (!zone || eventId.IsEmpty())
			return null;
		HST_TownInfluenceCommand command = NewCommand(
			zone,
			eventId,
			eventId,
			EVENT_CIVILIAN_NEARBY_COMBAT,
			"civilian danger from nearby combat episode");
		command.m_iHeatDelta = 4;
		return command;
	}

	protected HST_TownInfluenceCommand NewCommand(
		HST_ZoneState zone,
		string eventId,
		string sourceId,
		string eventKind,
		string reason)
	{
		HST_TownInfluenceCommand command = new HST_TownInfluenceCommand();
		command.m_sCommandId = eventId;
		command.m_sEventId = eventId;
		command.m_sTownId = zone.m_sZoneId;
		command.m_sEventKind = eventKind;
		command.m_sSourceId = sourceId;
		command.m_sReason = LimitText(reason, MAX_REASON_CHARACTERS);
		command.m_bPopulationScaled = false;
		command.m_bMarkContacted = false;
		command.m_bMarkResistanceActivity = false;
		command.m_bReconcileOwnership = true;
		return command;
	}

	protected HST_CivilianConsequenceResult ExecutePoliticalConsequence(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_TownInfluenceCommand command,
		HST_CivilianConsequenceResult result,
		bool localChanged,
		bool allowPanicOnly,
		bool triggerPanicForNewEvent,
		int appliedCombatEpisode)
	{
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		result.m_bPanicActive
			= zone.m_iCivilianPanicUntilSecond > nowSecond;
		result.m_bDangerActive = zone.m_bCivilianCombatDangerActive;
		if (zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
		{
			if (!allowPanicOnly)
			{
				return Reject(
					result,
					state,
					zone,
					"civilian political consequence requires a canonical town",
					localChanged);
			}
			string fingerprint = BuildCommandFingerprint(command);
			string existingFingerprint;
			if (m_mMinorExactReceiptFingerprints.Find(
				command.m_sEventId,
				existingFingerprint))
			{
				if (existingFingerprint != fingerprint)
				{
					return Reject(
						result,
						state,
						zone,
						"minor-locality event id was reused with a different fingerprint",
						localChanged);
				}
				localChanged = ApplyCombatEpisodeReceipt(
					zone,
					appliedCombatEpisode) || localChanged;
				CommitZoneMutation(zone, localChanged);
				result.m_bAccepted = true;
				result.m_bAlreadyApplied = true;
				result.m_bChanged = localChanged;
				result.m_bPanicOnly = true;
				return result;
			}
			if (m_mMinorExactReceiptFingerprints.Count()
				>= MAX_MINOR_EXACT_RECEIPTS)
			{
				return Reject(
					result,
					state,
					zone,
					"minor-locality exact receipt authority is full",
					localChanged);
			}
			m_mMinorExactReceiptFingerprints.Set(
				command.m_sEventId,
				fingerprint);
			if (triggerPanicForNewEvent)
				localChanged = RefreshPanic(
					zone,
					state.m_iElapsedSeconds,
					true) || localChanged;
			localChanged = SetLastEvent(zone, command.m_sEventId)
				|| localChanged;
			localChanged = ApplyCombatEpisodeReceipt(
				zone,
				appliedCombatEpisode) || localChanged;
			CommitZoneMutation(zone, localChanged);
			result.m_bAccepted = true;
			result.m_bChanged = localChanged;
			result.m_bPanicOnly = true;
			result.m_bPanicActive
				= zone.m_iCivilianPanicUntilSecond > nowSecond;
			return result;
		}

		if (!m_TownInfluence)
		{
			return Reject(
				result,
				state,
				zone,
				"town influence consequence authority is unavailable",
				localChanged);
		}
		if (!m_TownInfluence.FindValidRecord(state, zone.m_sZoneId))
		{
			return Reject(
				result,
				state,
				zone,
				"canonical town influence record is unavailable",
				localChanged);
		}

		HST_TownInfluenceResult townResult = m_TownInfluence.Execute(
			state,
			command);
		result.m_TownInfluenceResult = townResult;
		if (!townResult || !townResult.m_bAccepted)
		{
			string townFailure = "town influence consequence was rejected";
			if (townResult && !townResult.m_sFailureReason.IsEmpty())
				townFailure = townResult.m_sFailureReason;
			return Reject(result, state, zone, townFailure, localChanged);
		}

		if (!townResult.m_bAlreadyApplied)
		{
			if (triggerPanicForNewEvent)
				localChanged = RefreshPanic(
					zone,
					state.m_iElapsedSeconds,
					true) || localChanged;
			localChanged = SetLastEvent(zone, command.m_sEventId)
				|| localChanged;
		}
		localChanged = ApplyCombatEpisodeReceipt(
			zone,
			appliedCombatEpisode) || localChanged;
		CommitZoneMutation(zone, localChanged);
		result.m_bAccepted = true;
		result.m_bAlreadyApplied = townResult.m_bAlreadyApplied;
		result.m_bChanged = townResult.m_bChanged || localChanged;
		result.m_sFailureReason = townResult.m_sFailureReason;
		result.m_bPanicActive
			= zone.m_iCivilianPanicUntilSecond > nowSecond;
		return result;
	}

	protected string ValidateCommonContext(
		HST_CampaignState state,
		string zoneId,
		string exactEventId,
		string sourceId,
		out HST_ZoneState zone)
	{
		zone = null;
		if (!state)
			return "campaign state is unavailable";
		if (!m_Preset)
			return "campaign preset is unavailable";
		string failure = ResolveUniqueZone(state, zoneId, zone);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateZoneAuthority(zone);
		if (!failure.IsEmpty())
			return failure;
		exactEventId = exactEventId.Trim();
		if (exactEventId.IsEmpty())
			return "civilian consequence requires an exact event id";
		if (exactEventId.Length() > MAX_ID_CHARACTERS)
			return "civilian consequence event id is too long";
		sourceId = sourceId.Trim();
		if (sourceId.Length() > MAX_ID_CHARACTERS)
			return "civilian consequence source id is too long";
		return ValidatePresetFactionKeys(state);
	}

	protected string ValidateObservationContext(
		HST_CampaignState state,
		string zoneId,
		int combatPresenceRevision,
		int currentOperationCount,
		int recentFireCount,
		string contributorHash,
		out HST_ZoneState zone)
	{
		zone = null;
		if (!state)
			return "campaign state is unavailable";
		if (!m_Preset)
			return "campaign preset is unavailable";
		string failure = ResolveUniqueZone(state, zoneId, zone);
		if (!failure.IsEmpty())
			return failure;
		failure = ValidateZoneAuthority(zone);
		if (!failure.IsEmpty())
			return failure;
		if (combatPresenceRevision <= 0
			|| combatPresenceRevision > MAX_MUTABLE_REVISION)
			return "combat presence revision is invalid";
		if (zone.m_iCombatPresenceRevision != combatPresenceRevision)
			return "combat presence observation is not the current zone revision";
		if (combatPresenceRevision
			< zone.m_iCivilianLastCombatPresenceRevision)
			return "combat presence observation is stale";
		if (currentOperationCount < 0
			|| currentOperationCount > MAX_OBSERVED_COUNT
			|| recentFireCount < 0
			|| recentFireCount > MAX_OBSERVED_COUNT)
			return "combat presence danger counts are invalid";
		if (zone.m_iCombatPresenceCurrentOperationCount
				!= currentOperationCount
			|| zone.m_iCombatPresenceRecentFireCount != recentFireCount)
			return "combat presence danger facts do not match zone authority";
		contributorHash = contributorHash.Trim();
		if (contributorHash.Length() > MAX_ID_CHARACTERS)
			return "combat presence contributor hash is too long";
		if (zone.m_sCombatPresenceContributorHash
				!= zone.m_sCombatPresenceContributorHash.Trim()
			|| zone.m_sCombatPresenceContributorHash.Length()
				> MAX_ID_CHARACTERS
			|| zone.m_sCombatPresenceContributorHash != contributorHash)
			return "combat presence contributor hash does not match zone authority";
		return ValidatePresetFactionKeys(state);
	}

	protected string ValidateZoneAuthority(HST_ZoneState zone)
	{
		if (!zone)
			return "civilian consequence zone is unavailable";
		if (zone.m_iCivilianConsequenceContractVersion != CONTRACT_VERSION)
			return "civilian consequence zone contract is invalid";
		if (zone.m_iCivilianConsequenceRevision <= 0
			|| zone.m_iCivilianConsequenceRevision >= MAX_MUTABLE_REVISION)
			return "civilian consequence zone revision is invalid";
		if (zone.m_iCivilianCombatEpisodeCount < 0
			|| zone.m_iCivilianCombatEpisodeCount > MAX_EPISODE_COUNT)
			return "civilian combat episode count is invalid";
		if (zone.m_iCivilianAdoptedCombatEpisodeCount < 0
			|| zone.m_iCivilianAdoptedCombatEpisodeCount > 1
			|| zone.m_iCivilianAdoptedCombatEpisodeCount
				> zone.m_iCivilianLastAppliedCombatEpisodeCount
			|| zone.m_iCivilianLastAppliedCombatEpisodeCount < 0
			|| zone.m_iCivilianLastAppliedCombatEpisodeCount
				> zone.m_iCivilianCombatEpisodeCount
			|| zone.m_iCivilianCombatEpisodeCount
				- zone.m_iCivilianLastAppliedCombatEpisodeCount > 1)
			return "civilian applied combat episode receipt is invalid";
		if (zone.m_iCivilianLastCombatPresenceRevision < 0
			|| zone.m_iCivilianLastCombatPresenceRevision > MAX_MUTABLE_REVISION
			|| zone.m_iCivilianDangerChangedSecond < 0
			|| zone.m_iCivilianPanicUntilSecond < 0)
			return "civilian consequence timing authority is invalid";
		if (zone.m_bCivilianCombatDangerActive
			&& zone.m_iCivilianCombatEpisodeCount <= 0)
			return "active civilian danger has no combat episode";
		if (!IsBoundedTrimmed(
				zone.m_sCivilianLastConsequenceEventId,
				MAX_ID_CHARACTERS)
			|| !IsBoundedTrimmed(
				zone.m_sCivilianConsequenceAuthorityFailure,
				MAX_REASON_CHARACTERS))
			return "civilian consequence diagnostic authority is invalid";
		if (!zone.m_sCivilianConsequenceAuthorityFailure.IsEmpty())
			return "civilian consequence zone authority is quarantined";
		return "";
	}

	protected string ValidatePresetFactionKeys(HST_CampaignState state)
	{
		if (!state || !m_Preset)
			return "civilian consequence faction context is unavailable";
		string resistance = m_Preset.m_sResistanceFactionKey.Trim();
		string occupier = m_Preset.m_sOccupierFactionKey.Trim();
		string invader = m_Preset.m_sInvaderFactionKey.Trim();
		if (resistance != m_Preset.m_sResistanceFactionKey
			|| occupier != m_Preset.m_sOccupierFactionKey
			|| invader != m_Preset.m_sInvaderFactionKey)
			return "campaign preset faction keys are not canonical";
		if (resistance.IsEmpty() || resistance.Length() > MAX_ID_CHARACTERS)
			return "resistance faction authority is invalid";
		if (resistance == occupier || resistance == invader)
			return "resistance and enemy faction authority overlap";
		HST_FactionPoolState pool;
		if (!FindUniqueFactionPool(state, resistance, pool) || !pool)
			return "resistance faction pool is unavailable or ambiguous";
		if (!occupier.IsEmpty())
		{
			if (occupier.Length() > MAX_ID_CHARACTERS
				|| !FindUniqueFactionPool(state, occupier, pool) || !pool)
				return "occupier faction pool is unavailable or ambiguous";
		}
		if (!invader.IsEmpty() && invader != occupier)
		{
			if (invader.Length() > MAX_ID_CHARACTERS
				|| !FindUniqueFactionPool(state, invader, pool) || !pool)
				return "invader faction pool is unavailable or ambiguous";
		}
		if (occupier.IsEmpty() && invader.IsEmpty())
			return "enemy faction authority is unavailable";
		return "";
	}

	protected string ValidateResponsibleFaction(
		HST_CampaignState state,
		string factionKey,
		bool allowUnknown)
	{
		if (factionKey.IsEmpty())
		{
			if (allowUnknown)
				return "";
			return "civilian consequence responsible faction is required";
		}
		if (factionKey.Length() > MAX_ID_CHARACTERS)
			return "civilian consequence responsible faction is too long";
		if (factionKey != m_Preset.m_sResistanceFactionKey
			&& factionKey != m_Preset.m_sOccupierFactionKey
			&& factionKey != m_Preset.m_sInvaderFactionKey)
			return "civilian consequence responsible faction is unknown";
		HST_FactionPoolState pool;
		if (!FindUniqueFactionPool(state, factionKey, pool) || !pool)
			return "civilian consequence responsible faction is ambiguous";
		return "";
	}

	protected string ResolveAttribution(string responsibleFactionKey)
	{
		if (responsibleFactionKey.IsEmpty())
			return "unknown";
		if (responsibleFactionKey == m_Preset.m_sResistanceFactionKey)
			return "resistance";
		if (responsibleFactionKey == m_Preset.m_sOccupierFactionKey
			|| responsibleFactionKey == m_Preset.m_sInvaderFactionKey)
			return "enemy";
		return "";
	}

	protected string ResolveCurrentEnemyFactionKey(
		HST_CampaignState state,
		HST_ZoneState zone)
	{
		if (!state || !zone || !m_Preset)
			return "";
		string enemy = zone.m_sOwnerFactionKey.Trim();
		if (enemy != m_Preset.m_sOccupierFactionKey
			&& enemy != m_Preset.m_sInvaderFactionKey)
			enemy = m_Preset.m_sOccupierFactionKey.Trim();
		if (enemy.IsEmpty())
			enemy = m_Preset.m_sInvaderFactionKey.Trim();
		HST_FactionPoolState pool;
		if (enemy.IsEmpty()
			|| !FindUniqueFactionPool(state, enemy, pool) || !pool)
			return "";
		return enemy;
	}

	protected bool SetEnemySupportDelta(
		HST_TownInfluenceCommand command,
		string enemyFactionKey,
		int delta)
	{
		if (!command || !m_Preset || enemyFactionKey.IsEmpty())
			return false;
		if (enemyFactionKey == m_Preset.m_sOccupierFactionKey)
		{
			command.m_iRawOccupierSupportDelta = delta;
			return true;
		}
		if (enemyFactionKey == m_Preset.m_sInvaderFactionKey)
		{
			command.m_iRawInvaderSupportDelta = delta;
			return true;
		}
		return false;
	}

	protected string BuildCommandFingerprint(HST_TownInfluenceCommand command)
	{
		if (!command)
			return "";
		string fingerprint = string.Format(
			"%1|%2|%3|%4|%5|%6|%7|%8|%9",
			command.m_sTownId,
			command.m_sEventKind,
			command.m_sSourceId,
			command.m_sReason,
			command.m_iRawFIASupportDelta,
			command.m_iRawOccupierSupportDelta,
			command.m_iRawInvaderSupportDelta,
			command.m_iReputationDelta,
			command.m_iHeatDelta);
		fingerprint = fingerprint + string.Format(
			"|%1|%2|%3|%4|%5|%6|%7|%8|%9",
			command.m_iPopulationDelta,
			command.m_iPoliceDelta,
			command.m_iRoadblockDelta,
			command.m_sAggressionFactionKey,
			command.m_iAggressionDelta,
			command.m_iDurationSeconds,
			command.m_bPopulationScaled,
			command.m_bMarkContacted,
			command.m_bMarkResistanceActivity);
		bool absoluteSeed;
#ifdef ENABLE_DIAG
		absoluteSeed = command.m_bAbsoluteDebugSeed;
#endif
		return fingerprint + string.Format(
			"|%1|%2|%3",
			command.m_bReconcileOwnership,
			absoluteSeed,
			command.m_sEventId);
	}

	protected string ResolveUniqueZone(
		HST_CampaignState state,
		string zoneId,
		out HST_ZoneState zone)
	{
		zone = null;
		zoneId = zoneId.Trim();
		if (zoneId.IsEmpty() || zoneId.Length() > MAX_ID_CHARACTERS)
			return "civilian consequence zone id is invalid";
		HST_ZoneState resolved = state.FindZone(zoneId);
		if (!resolved || resolved.m_sZoneId.IsEmpty())
			return "civilian consequence zone was not found";
		int matchCount;
		foreach (HST_ZoneState candidate : state.m_aZones)
		{
			if (candidate && candidate.m_sZoneId == resolved.m_sZoneId)
			{
				matchCount++;
				zone = candidate;
			}
		}
		if (matchCount != 1)
		{
			zone = null;
			return "civilian consequence zone authority is ambiguous";
		}
		return "";
	}

	protected bool FindUniqueFactionPool(
		HST_CampaignState state,
		string factionKey,
		out HST_FactionPoolState match)
	{
		match = null;
		if (!state || factionKey.IsEmpty())
			return false;
		foreach (HST_FactionPoolState pool : state.m_aFactionPools)
		{
			if (!pool || pool.m_sFactionKey != factionKey)
				continue;
			if (match)
				return false;
			match = pool;
		}
		return true;
	}

	protected bool RefreshPanic(
		HST_ZoneState zone,
		int elapsedSecond,
		bool forceRefresh)
	{
		if (!zone)
			return false;
		int nowSecond = Math.Max(0, elapsedSecond);
		int remaining = zone.m_iCivilianPanicUntilSecond - nowSecond;
		if (!forceRefresh && remaining > PANIC_REFRESH_THRESHOLD_SECONDS)
			return false;
		int requestedUntil = int.MAX;
		if (nowSecond <= int.MAX - PANIC_DURATION_SECONDS)
			requestedUntil = nowSecond + PANIC_DURATION_SECONDS;
		if (requestedUntil <= zone.m_iCivilianPanicUntilSecond)
			return false;
		zone.m_iCivilianPanicUntilSecond = requestedUntil;
		return true;
	}

	protected bool SetLastEvent(HST_ZoneState zone, string eventId)
	{
		if (!zone || zone.m_sCivilianLastConsequenceEventId == eventId)
			return false;
		zone.m_sCivilianLastConsequenceEventId = eventId;
		return true;
	}

	protected bool ApplyCombatEpisodeReceipt(
		HST_ZoneState zone,
		int appliedCombatEpisode)
	{
		if (!zone || appliedCombatEpisode <= 0
			|| appliedCombatEpisode
				<= zone.m_iCivilianLastAppliedCombatEpisodeCount)
			return false;
		if (appliedCombatEpisode > zone.m_iCivilianCombatEpisodeCount)
			return false;
		zone.m_iCivilianLastAppliedCombatEpisodeCount
			= appliedCombatEpisode;
		return true;
	}

	protected void CommitZoneMutation(HST_ZoneState zone, bool changed)
	{
		if (!zone || !changed)
			return;
		if (zone.m_iCivilianConsequenceRevision < MAX_MUTABLE_REVISION)
			zone.m_iCivilianConsequenceRevision++;
	}

	protected HST_CivilianConsequenceResult Reject(
		HST_CivilianConsequenceResult result,
		HST_CampaignState state,
		HST_ZoneState zone,
		string failure,
		bool localChanged)
	{
		if (!result)
			result = NewResult("unknown", "");
		failure = LimitText(failure.Trim(), MAX_REASON_CHARACTERS);
		if (failure.IsEmpty())
			failure = "civilian consequence rejected";
		CommitZoneMutation(zone, localChanged);
		result.m_bChanged = localChanged;
		result.m_sFailureReason = failure;
		if (zone)
		{
			int nowSecond;
			if (state)
				nowSecond = Math.Max(0, state.m_iElapsedSeconds);
			result.m_bDangerActive = zone.m_bCivilianCombatDangerActive;
			result.m_bPanicActive
				= zone.m_iCivilianPanicUntilSecond > nowSecond;
		}
		return result;
	}

	protected HST_CivilianConsequenceResult NewResult(
		string eventKind,
		string eventId)
	{
		HST_CivilianConsequenceResult result
			= new HST_CivilianConsequenceResult();
		result.m_sEventKind = eventKind;
		result.m_sEventId = eventId.Trim();
		return result;
	}

	protected string ResolveSourceId(string sourceId, string eventId)
	{
		sourceId = sourceId.Trim();
		if (sourceId.IsEmpty())
			sourceId = eventId.Trim();
		return sourceId;
	}

	protected bool IsBoundedTrimmed(string value, int maxCharacters)
	{
		return value.Length() <= maxCharacters
			&& value == value.Trim();
	}

	protected string LimitText(string value, int maxCharacters)
	{
		if (maxCharacters <= 0)
			return "";
		if (value.Length() <= maxCharacters)
			return value;
		return value.Substring(0, maxCharacters);
	}
}
