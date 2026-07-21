// Schema-65 persistence boundary for locality-level civilian consequences.
// Ambient actor topology remains session-only; only combat-danger episodes and
// exact durable consequence evidence survive save/restart.
class HST_CivilianConsequenceSaveValidationService
{
	static const int SCHEMA_VERSION = 65;
	static const int CONTRACT_VERSION = 1;
	static const int QUARANTINE_CONTRACT_VERSION = -65;
	static const int MAX_MUTABLE_REVISION = int.MAX - 16;
	static const int MAX_ID_CHARACTERS = 160;
	static const int MAX_PANIC_FUTURE_SECONDS = 600;
	static const int MIGRATED_ACTIVE_PANIC_SECONDS = 45;

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		if (!saveData)
			return;
		if (!saveData.m_aZones)
			saveData.m_aZones = new array<ref HST_ZoneState>();
		if (!saveData.m_aTownInfluenceEvents)
			saveData.m_aTownInfluenceEvents
				= new array<ref HST_TownInfluenceEventState>();
		map<string, ref HST_TownInfluenceEventState> influenceByEventId
			= new map<string, ref HST_TownInfluenceEventState>();
		map<string, int> influenceEventIdCounts = new map<string, int>();
		BuildInfluenceEventIndex(
			saveData,
			influenceByEventId,
			influenceEventIdCounts);
		foreach (HST_ZoneState zone : saveData.m_aZones)
		{
			if (!zone)
				continue;
			if (restoredSchemaVersion < SCHEMA_VERSION)
				MigrateSchema64Envelope(saveData, zone);
			else
				ValidateOrQuarantine(
					saveData,
					zone,
					influenceByEventId,
					influenceEventIdCounts);
		}
	}

	protected void MigrateSchema64Envelope(
		HST_CampaignSaveData saveData,
		HST_ZoneState zone)
	{
		if (!saveData || !zone)
			return;
		int nowSecond = Math.Max(0, saveData.m_iElapsedSeconds);
		bool activeDanger = zone.m_iCombatPresenceCurrentOperationCount > 0
			|| zone.m_iCombatPresenceRecentFireCount > 0;
		zone.m_iCivilianConsequenceContractVersion = CONTRACT_VERSION;
		zone.m_iCivilianConsequenceRevision = 1;
		zone.m_bCivilianCombatDangerActive = activeDanger;
		zone.m_iCivilianCombatEpisodeCount = 0;
		if (activeDanger)
			zone.m_iCivilianCombatEpisodeCount = 1;
		zone.m_iCivilianAdoptedCombatEpisodeCount
			= zone.m_iCivilianCombatEpisodeCount;
		zone.m_iCivilianLastAppliedCombatEpisodeCount
			= zone.m_iCivilianCombatEpisodeCount;
		zone.m_iCivilianLastCombatPresenceRevision
			= Math.Max(0, zone.m_iCombatPresenceRevision);
		zone.m_iCivilianDangerChangedSecond = 0;
		if (activeDanger)
			zone.m_iCivilianDangerChangedSecond = nowSecond;
		zone.m_iCivilianPanicUntilSecond = 0;
		if (activeDanger)
		{
			zone.m_iCivilianPanicUntilSecond = SafeFutureSecond(
				nowSecond,
				MIGRATED_ACTIVE_PANIC_SECONDS);
		}
		zone.m_sCivilianLastConsequenceEventId = "";
		zone.m_sCivilianConsequenceAuthorityFailure = "";
	}

	protected void ValidateOrQuarantine(
		HST_CampaignSaveData saveData,
		HST_ZoneState zone,
		map<string, ref HST_TownInfluenceEventState> influenceByEventId,
		map<string, int> influenceEventIdCounts)
	{
		if (!saveData || !zone)
			return;
		if (!zone.m_bCivilianCombatDangerActive
			&& zone.m_iCivilianPanicUntilSecond > 0
			&& zone.m_iCivilianPanicUntilSecond
				<= Math.Max(0, saveData.m_iElapsedSeconds))
		{
			zone.m_iCivilianPanicUntilSecond = 0;
		}
		if (zone.m_iCivilianConsequenceContractVersion
			== QUARANTINE_CONTRACT_VERSION)
		{
			zone.m_bCivilianCombatDangerActive = false;
			zone.m_iCivilianPanicUntilSecond = 0;
			if (zone.m_sCivilianConsequenceAuthorityFailure.IsEmpty())
			{
				zone.m_sCivilianConsequenceAuthorityFailure
					= "schema65 quarantined civilian consequence authority";
			}
			return;
		}
		string failure = ValidateEnvelope(
			saveData,
			zone,
			influenceByEventId,
			influenceEventIdCounts);
		if (failure.IsEmpty())
			return;
		zone.m_iCivilianConsequenceContractVersion
			= QUARANTINE_CONTRACT_VERSION;
		zone.m_bCivilianCombatDangerActive = false;
		zone.m_iCivilianPanicUntilSecond = 0;
		zone.m_sCivilianConsequenceAuthorityFailure = failure;
	}

	protected string ValidateEnvelope(
		HST_CampaignSaveData saveData,
		HST_ZoneState zone,
		map<string, ref HST_TownInfluenceEventState> influenceByEventId,
		map<string, int> influenceEventIdCounts)
	{
		if (!saveData || !zone)
			return "schema65 civilian consequence envelope is unavailable";
		int nowSecond = Math.Max(0, saveData.m_iElapsedSeconds);
		if (zone.m_iCivilianConsequenceContractVersion != CONTRACT_VERSION)
			return "schema65 unsupported civilian consequence contract";
		if (zone.m_iCivilianConsequenceRevision <= 0
			|| zone.m_iCivilianConsequenceRevision >= MAX_MUTABLE_REVISION)
			return "schema65 invalid civilian consequence revision";
		if (zone.m_iCivilianCombatEpisodeCount < 0
			|| zone.m_iCivilianCombatEpisodeCount
				> HST_CivilianConsequenceService.MAX_EPISODE_COUNT)
			return "schema65 invalid civilian combat episode count";
		if (zone.m_iCivilianAdoptedCombatEpisodeCount < 0
			|| zone.m_iCivilianAdoptedCombatEpisodeCount > 1
			|| zone.m_iCivilianAdoptedCombatEpisodeCount
				> zone.m_iCivilianLastAppliedCombatEpisodeCount
			|| zone.m_iCivilianLastAppliedCombatEpisodeCount < 0
			|| zone.m_iCivilianLastAppliedCombatEpisodeCount
				> zone.m_iCivilianCombatEpisodeCount
			|| zone.m_iCivilianCombatEpisodeCount
				- zone.m_iCivilianLastAppliedCombatEpisodeCount > 1)
			return "schema65 invalid civilian combat consequence receipt";
		if (zone.m_iCivilianLastCombatPresenceRevision < 0
			|| zone.m_iCivilianLastCombatPresenceRevision
				> Math.Max(0, zone.m_iCombatPresenceRevision))
			return "schema65 civilian combat receipt is ahead of combat authority";
		if (zone.m_iCivilianDangerChangedSecond < 0
			|| zone.m_iCivilianDangerChangedSecond > nowSecond)
			return "schema65 invalid civilian danger transition time";
		if (zone.m_bCivilianCombatDangerActive)
		{
			if (zone.m_iCivilianCombatEpisodeCount <= 0
				|| zone.m_iCivilianLastCombatPresenceRevision <= 0
				|| zone.m_iCivilianPanicUntilSecond
					< zone.m_iCivilianDangerChangedSecond
				|| zone.m_iCivilianPanicUntilSecond
					> SafeFutureSecond(nowSecond, MAX_PANIC_FUTURE_SECONDS))
				return "schema65 invalid active civilian danger envelope";
		}
		if (!zone.m_bCivilianCombatDangerActive
			&& zone.m_iCivilianPanicUntilSecond != 0
			&& (zone.m_iCivilianPanicUntilSecond
					< zone.m_iCivilianDangerChangedSecond
				|| zone.m_iCivilianPanicUntilSecond
					> SafeFutureSecond(nowSecond, MAX_PANIC_FUTURE_SECONDS)))
			return "schema65 invalid civilian panic recovery deadline";
		bool exactDangerFacts = zone.m_iCombatPresenceCurrentOperationCount > 0
			|| zone.m_iCombatPresenceRecentFireCount > 0;
		if (zone.m_iCivilianLastCombatPresenceRevision
				== zone.m_iCombatPresenceRevision
			&& zone.m_bCivilianCombatDangerActive != exactDangerFacts)
			return "schema65 civilian danger disagrees with current combat facts";
		if (zone.m_sCivilianLastConsequenceEventId
				!= zone.m_sCivilianLastConsequenceEventId.Trim()
			|| zone.m_sCivilianLastConsequenceEventId.Length()
				> MAX_ID_CHARACTERS)
			return "schema65 invalid civilian consequence event id";
		if (!zone.m_sCivilianLastConsequenceEventId.IsEmpty()
			&& zone.m_eType == HST_EZoneType.HST_ZONE_TOWN
			&& !HasUniqueConsequenceEvent(
				zone,
				influenceByEventId,
				influenceEventIdCounts))
			return "schema65 civilian consequence event receipt is invalid";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_TOWN
			&& !HasExactAppliedCombatReceipts(
				saveData,
				zone,
				influenceByEventId,
				influenceEventIdCounts))
			return "schema65 civilian combat episode receipt is invalid";
		if (!zone.m_sCivilianConsequenceAuthorityFailure.IsEmpty())
			return "schema65 civilian consequence authority carries a failure";
		return "";
	}

	protected bool HasUniqueConsequenceEvent(
		HST_ZoneState zone,
		map<string, ref HST_TownInfluenceEventState> influenceByEventId,
		map<string, int> influenceEventIdCounts)
	{
		if (!zone || !influenceByEventId || !influenceEventIdCounts
			|| zone.m_sCivilianLastConsequenceEventId.IsEmpty())
			return false;
		HST_TownInfluenceEventState match;
		int matchCount;
		influenceByEventId.Find(
			zone.m_sCivilianLastConsequenceEventId,
			match);
		influenceEventIdCounts.Find(
			zone.m_sCivilianLastConsequenceEventId,
			matchCount);
		if (matchCount != 1 || !match || !match.m_bApplied
			|| match.m_iContractVersion
				!= HST_TownInfluenceService.EXACT_CONTRACT_VERSION
			|| match.m_sZoneId != zone.m_sZoneId)
			return false;
		return match.m_sKind == HST_CivilianConsequenceService.KIND_CASUALTY
			|| match.m_sKind == HST_CivilianConsequenceService.KIND_VEHICLE_THEFT
			|| match.m_sKind == HST_CivilianConsequenceService.KIND_NEARBY_COMBAT;
	}

	protected bool HasExactAppliedCombatReceipts(
		HST_CampaignSaveData saveData,
		HST_ZoneState zone,
		map<string, ref HST_TownInfluenceEventState> influenceByEventId,
		map<string, int> influenceEventIdCounts)
	{
		if (!saveData || !zone || !influenceByEventId
			|| !influenceEventIdCounts)
			return false;
		if (zone.m_iCivilianAdoptedCombatEpisodeCount > 0)
		{
			string adoptedEventId
				= HST_CivilianConsequenceService.BuildCombatEpisodeEventId(
					zone.m_sZoneId,
					zone.m_iCivilianAdoptedCombatEpisodeCount);
			int adoptedEventCount;
			influenceEventIdCounts.Find(adoptedEventId, adoptedEventCount);
			if (adoptedEventId.IsEmpty() || adoptedEventCount != 0)
				return false;
		}
		int firstLiveEpisode
			= zone.m_iCivilianAdoptedCombatEpisodeCount + 1;
		int lastLiveEpisode
			= zone.m_iCivilianLastAppliedCombatEpisodeCount;
		if (firstLiveEpisode > lastLiveEpisode)
			return true;
		if (lastLiveEpisode - firstLiveEpisode + 1
			> saveData.m_aTownInfluenceEvents.Count())
			return false;
		for (int episode = firstLiveEpisode; episode <= lastLiveEpisode; episode++)
		{
			string eventId = HST_CivilianConsequenceService.BuildCombatEpisodeEventId(
				zone.m_sZoneId,
				episode);
			HST_TownInfluenceEventState eventState;
			int eventCount;
			influenceByEventId.Find(eventId, eventState);
			influenceEventIdCounts.Find(eventId, eventCount);
			if (eventId.IsEmpty() || eventCount != 1 || !eventState
				|| !eventState.m_bApplied
				|| eventState.m_iContractVersion
					!= HST_TownInfluenceService.EXACT_CONTRACT_VERSION
				|| eventState.m_sZoneId != zone.m_sZoneId
				|| eventState.m_sKind
					!= HST_CivilianConsequenceService.KIND_NEARBY_COMBAT
				|| !HasCanonicalCombatEventFingerprint(eventState))
				return false;
		}
		return true;
	}

	protected bool HasCanonicalCombatEventFingerprint(
		HST_TownInfluenceEventState eventState)
	{
		if (!eventState)
			return false;
		bool commandExact = eventState.m_sSourceId == eventState.m_sEventId
			&& eventState.m_sReason
				== "civilian danger from nearby combat episode"
			&& eventState.m_iExpiresAtSecond == 0
			&& eventState.m_iFIASupportDelta == 0
			&& eventState.m_iOccupierSupportDelta == 0
			&& eventState.m_iReputationDelta == 0
			&& eventState.m_iHeatDelta == 4
			&& eventState.m_iPopulationDelta == 0
			&& eventState.m_iPoliceDelta == 0
			&& eventState.m_iRoadblockDelta == 0;
		bool aggressionExact = eventState.m_sAggressionFactionKey.IsEmpty()
			&& eventState.m_iAggressionDelta == 0
			&& eventState.m_iAggressionBefore == 0
			&& eventState.m_iAggressionAfter == 0;
		bool supportExact = eventState.m_iRequestedFIABasisPointDelta == 0
			&& eventState.m_iRequestedOccupierBasisPointDelta == 0
			&& eventState.m_iRequestedInvaderBasisPointDelta == 0
			&& eventState.m_iEffectiveFIABasisPointDelta == 0
			&& eventState.m_iEffectiveOccupierBasisPointDelta == 0
			&& eventState.m_iEffectiveInvaderBasisPointDelta == 0
			&& eventState.m_iFIABasisPointsBefore
				== eventState.m_iFIABasisPointsAfter
			&& eventState.m_iOccupierBasisPointsBefore
				== eventState.m_iOccupierBasisPointsAfter
			&& eventState.m_iInvaderBasisPointsBefore
				== eventState.m_iInvaderBasisPointsAfter;
		bool populationExact = eventState.m_iInitialPopulationBefore
				== eventState.m_iInitialPopulationAfter
			&& eventState.m_iRemainingPopulationBefore
				== eventState.m_iRemainingPopulationAfter
			&& eventState.m_iDestroyedPopulationBefore
				== eventState.m_iDestroyedPopulationAfter
			&& !eventState.m_bPopulationScaled
#ifdef ENABLE_DIAG
			&& !eventState.m_bAbsoluteDebugSeed;
#else
			;
#endif
		return commandExact && aggressionExact
			&& supportExact && populationExact;
	}

	protected void BuildInfluenceEventIndex(
		HST_CampaignSaveData saveData,
		map<string, ref HST_TownInfluenceEventState> influenceByEventId,
		map<string, int> influenceEventIdCounts)
	{
		if (!saveData || !influenceByEventId || !influenceEventIdCounts)
			return;
		foreach (HST_TownInfluenceEventState eventState : saveData.m_aTownInfluenceEvents)
		{
			if (!eventState || eventState.m_sEventId.IsEmpty())
				continue;
			int count;
			if (influenceEventIdCounts.Find(eventState.m_sEventId, count))
				influenceEventIdCounts.Set(eventState.m_sEventId, 2);
			else
				influenceEventIdCounts.Set(eventState.m_sEventId, 1);
			HST_TownInfluenceEventState existing;
			if (!influenceByEventId.Find(eventState.m_sEventId, existing))
				influenceByEventId.Set(eventState.m_sEventId, eventState);
		}
	}

	protected int SafeFutureSecond(int nowSecond, int durationSeconds)
	{
		int boundedNow = Math.Max(0, nowSecond);
		int boundedDuration = Math.Max(0, durationSeconds);
		if (boundedNow > int.MAX - boundedDuration)
			return int.MAX;
		return boundedNow + boundedDuration;
	}
}
