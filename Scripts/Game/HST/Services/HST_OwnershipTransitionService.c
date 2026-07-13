class HST_OwnershipTransitionRequest
{
	string m_sRequestId;
	string m_sZoneId;
	string m_sCause;
	string m_sSourceType;
	string m_sSourceId;
	string m_sActorIdentityId;
	string m_sExpectedOwnerFactionKey;
	int m_iExpectedRevision;
	string m_sNewOwnerFactionKey;
	string m_sReason;
	int m_iSupportReward;
	bool m_bApplyEnemyConsequences = true;
	bool m_bReconcileSecurity = true;
	bool m_bCreateSecurity = true;
	bool m_bNotify = true;
}

class HST_OwnershipTransitionResult
{
	bool m_bAccepted;
	bool m_bAlreadyApplied;
	bool m_bStateChanged;
	bool m_bCompleted;
	bool m_bNeedsRetry;
	string m_sFailureReason;
	ref HST_OwnershipTransitionState m_Transition;
	ref HST_ZoneState m_Zone;

	string BuildReport()
	{
		if (!m_Transition)
			return "Partisan ownership transition | rejected | " + m_sFailureReason;

		string report = string.Format(
			"Partisan ownership transition | request %1 | zone %2 | owner %3 -> %4 | revision %5 -> %6 | status %7",
			m_Transition.m_sRequestId,
			m_Transition.m_sZoneId,
			m_Transition.m_sPreviousOwnerFactionKey,
			m_Transition.m_sNewOwnerFactionKey,
			m_Transition.m_iExpectedRevision,
			m_Transition.m_iAppliedRevision,
			m_Transition.m_sStatus);
		return report + string.Format(
			" | accepted %1 | replay %2 | changed %3 | %4",
			m_bAccepted,
			m_bAlreadyApplied,
			m_bStateChanged,
			m_sFailureReason);
	}
}

// Schema-62 is the single authority for every post-bootstrap location owner
// change. Domain effects complete before any marker/menu projection is published.
// Persistence completion means the debounced durable checkpoint was scheduled;
// actual disk durability remains a persistence/runtime verification gate.
class HST_OwnershipTransitionService
{
	static const int EXACT_CONTRACT_VERSION = 1;
	static const int QUARANTINED_CONTRACT_VERSION = -62;
	static const int SCHEMA_VERSION = 62;
	static const int MAX_TRANSITION_ROWS = 512;
	static const int MIN_REPLAY_RETENTION_SECONDS = 86400;
	static const int MAX_REQUEST_ID_CHARACTERS = 180;
	static const int MAX_REASON_CHARACTERS = 320;
	static const int MAX_SOURCE_CHARACTERS = 180;
	static const int PENDING_RETRY_INTERVAL_SECONDS = 5;

	protected HST_CampaignPreset m_Preset;
	protected HST_BalanceConfig m_Balance;
	protected HST_EconomyService m_Economy;
	protected HST_StrategicService m_Strategic;
	protected HST_GarrisonService m_Garrisons;
	protected HST_CivilianService m_Civilians;
	protected HST_TownInfluenceService m_TownInfluence;
	protected HST_CampaignEventLogService m_CampaignEvents;
	protected HST_EnemyCommanderService m_EnemyCommander;
	protected HST_EnemyDirectorService m_EnemyDirector;
	protected HST_SupportRequestService m_SupportRequests;
	protected HST_PhysicalWarService m_PhysicalWar;
	protected HST_LocalSecurityOperationService m_LocalSecurityPatrols;
	protected HST_GarrisonPatrolOperationService m_GarrisonPatrols;
	protected HST_ZoneCaptureService m_ZoneCapture;
	protected HST_MapMarkerService m_MapMarkers;
	protected HST_ClientProjectionService m_ClientProjection;
	protected HST_PersistenceService m_Persistence;
	protected bool m_bStateChangedSinceConsume;
	protected ref array<string> m_aApplicationRequestStack = {};

	void ConfigureDomainServices(
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		HST_EconomyService economy,
		HST_StrategicService strategic,
		HST_GarrisonService garrisons,
		HST_CivilianService civilians,
		HST_CampaignEventLogService campaignEvents)
	{
		m_Preset = preset;
		m_Balance = balance;
		m_Economy = economy;
		m_Strategic = strategic;
		m_Garrisons = garrisons;
		m_Civilians = civilians;
		m_CampaignEvents = campaignEvents;
	}

	void ConfigureRuntimeServices(
		HST_EnemyCommanderService enemyCommander,
		HST_EnemyDirectorService enemyDirector,
		HST_SupportRequestService supportRequests,
		HST_PhysicalWarService physicalWar,
		HST_LocalSecurityOperationService localSecurityPatrols,
		HST_GarrisonPatrolOperationService garrisonPatrols,
		HST_ZoneCaptureService zoneCapture)
	{
		m_EnemyCommander = enemyCommander;
		m_EnemyDirector = enemyDirector;
		m_SupportRequests = supportRequests;
		m_PhysicalWar = physicalWar;
		m_LocalSecurityPatrols = localSecurityPatrols;
		m_GarrisonPatrols = garrisonPatrols;
		m_ZoneCapture = zoneCapture;
	}

	void SetTownInfluenceService(HST_TownInfluenceService townInfluence)
	{
		m_TownInfluence = townInfluence;
	}

	void ConfigureProjectionServices(
		HST_MapMarkerService mapMarkers,
		HST_ClientProjectionService clientProjection,
		HST_PersistenceService persistence)
	{
		m_MapMarkers = mapMarkers;
		m_ClientProjection = clientProjection;
		m_Persistence = persistence;
	}

	HST_OwnershipTransitionRequest BuildRequest(
		HST_CampaignState state,
		string zoneId,
		string newOwnerFactionKey,
		string cause,
		string sourceType,
		string sourceId,
		string reason,
		int supportReward = 0,
		string requestId = "",
		string actorIdentityId = "")
	{
		HST_OwnershipTransitionRequest request = new HST_OwnershipTransitionRequest();
		request.m_sZoneId = zoneId;
		request.m_sNewOwnerFactionKey = newOwnerFactionKey;
		request.m_sCause = cause;
		request.m_sSourceType = sourceType;
		request.m_sSourceId = sourceId;
		request.m_sReason = reason.Trim();
		if (request.m_sReason.IsEmpty())
			request.m_sReason = string.Format("ownership changed to %1", newOwnerFactionKey);
		request.m_iSupportReward = supportReward;
		request.m_sActorIdentityId = actorIdentityId;

		HST_ZoneState zone;
		if (state)
			zone = state.FindZone(zoneId);
		if (zone)
		{
			// FindZone accepts supported retired aliases. Freeze the canonical zone
			// identity into the request so an identical alias replay matches the
			// canonical durable receipt instead of looking like request-ID reuse.
			request.m_sZoneId = zone.m_sZoneId;
			request.m_sExpectedOwnerFactionKey = zone.m_sOwnerFactionKey;
			request.m_iExpectedRevision = Math.Max(1, zone.m_iOwnershipRevision);
		}

		request.m_sRequestId = requestId;
		if (request.m_sRequestId.IsEmpty() && zone)
		{
			string identity = sourceId;
			if (identity.IsEmpty())
				identity = string.Format("revision_%1", request.m_iExpectedRevision);
			request.m_sRequestId = string.Format(
				"ownership_%1_%2_%3_%4_%5",
				zone.m_sZoneId,
				cause,
				request.m_iExpectedRevision,
				newOwnerFactionKey,
				identity.Hash());
		}

		HST_OwnershipTransitionState existing;
		if (state && !request.m_sRequestId.IsEmpty())
			existing = state.FindOwnershipTransition(request.m_sRequestId);
		if (BuildRequestIdentityMatches(existing, request))
		{
			// A caller rebuilding an already accepted command sees the zone's new
			// live owner/revision. Restore the frozen precondition from the durable
			// receipt only after every caller-supplied semantic field matches; flag
			// changes made after BuildRequest remain guarded by RequestMatches.
			request.m_sExpectedOwnerFactionKey = existing.m_sExpectedOwnerFactionKey;
			request.m_iExpectedRevision = existing.m_iExpectedRevision;
		}
		return request;
	}

	protected bool BuildRequestIdentityMatches(
		HST_OwnershipTransitionState transition,
		HST_OwnershipTransitionRequest request)
	{
		if (!transition || !request || transition.m_sRequestId != request.m_sRequestId)
			return false;
		if (transition.m_sZoneId != request.m_sZoneId
			|| transition.m_sCause != request.m_sCause
			|| transition.m_sSourceType != request.m_sSourceType)
			return false;
		if (transition.m_sSourceId != request.m_sSourceId
			|| transition.m_sActorIdentityId != request.m_sActorIdentityId)
			return false;
		if (transition.m_sNewOwnerFactionKey != request.m_sNewOwnerFactionKey
			|| transition.m_sReason != request.m_sReason
			|| transition.m_iSupportReward != request.m_iSupportReward)
			return false;
		return true;
	}

	HST_OwnershipTransitionResult Apply(HST_CampaignState state, HST_OwnershipTransitionRequest request)
	{
		HST_OwnershipTransitionResult result = new HST_OwnershipTransitionResult();
		if (!state || !request)
			return Reject(result, "state or ownership request is unavailable");

		HST_OwnershipTransitionState existing = state.FindOwnershipTransition(request.m_sRequestId);
		if (existing)
		{
			result.m_Transition = existing;
			result.m_Zone = state.FindZone(existing.m_sZoneId);
			if (!RequestMatches(existing, request))
				return Reject(result, "ownership request id was reused with a different fingerprint");
			if (existing.m_bQuarantined || existing.m_iContractVersion == QUARANTINED_CONTRACT_VERSION)
				return Reject(result, "ownership transition authority is quarantined: " + existing.m_sFailureReason);
			if (existing.m_bCompleted)
			{
				result.m_bAccepted = true;
				result.m_bAlreadyApplied = true;
				result.m_bCompleted = true;
				return result;
			}
			return ContinueAcceptedTransitionScoped(state, existing, result);
		}

		string failureReason = ValidateNewRequest(state, request);
		if (!failureReason.IsEmpty())
			return Reject(result, failureReason);
		if (!EnsureAdmissionCapacity(state))
			return Reject(result, "ownership transition history is full of pinned or replay-retained authority");

		HST_ZoneState zone = state.FindZone(request.m_sZoneId);
		HST_OwnershipTransitionState transition = CreateTransition(state, zone, request);
		if (!transition || transition.m_sStrategicEventId.IsEmpty())
			return Reject(result, "ownership strategic event could not be admitted");
		state.m_aOwnershipTransitions.Insert(transition);
		zone.m_sActiveOwnershipTransitionRequestId = transition.m_sRequestId;
		result.m_Transition = transition;
		result.m_Zone = zone;
		result.m_bStateChanged = true;
		return ContinueAcceptedTransitionScoped(state, transition, result);
	}

	// Read-only admission fence used when another authority must commit first.
	// It mirrors every fallible step before a new durable receipt is inserted;
	// accepted partial/completed receipts are exact replay, not new admission.
	bool CanApply(
		HST_CampaignState state,
		HST_OwnershipTransitionRequest request,
		out string failure)
	{
		failure = "";
		if (!state || !request)
		{
			failure = "state or ownership request is unavailable";
			return false;
		}
		HST_OwnershipTransitionState existing
			= state.FindOwnershipTransition(request.m_sRequestId);
		if (existing)
		{
			if (!RequestMatches(existing, request))
			{
				failure = "ownership request id was reused with a different fingerprint";
				return false;
			}
			if (existing.m_bQuarantined
				|| existing.m_iContractVersion == QUARANTINED_CONTRACT_VERSION)
			{
				failure = "ownership transition authority is quarantined: "
					+ existing.m_sFailureReason;
				return false;
			}
			if (existing.m_bCompleted)
				return true;
			HST_ZoneState existingZone = state.FindZone(existing.m_sZoneId);
			if (!existingZone || !ResumeStateMatches(existingZone, existing))
			{
				failure = "zone owner/revision diverged from the transition receipt";
				return false;
			}
			return true;
		}

		failure = ValidateNewRequest(state, request);
		if (!failure.IsEmpty())
			return false;
		if (!CanEnsureAdmissionCapacity(state))
		{
			failure = "ownership transition history is full of pinned or replay-retained authority";
			return false;
		}
		string eventKind = "ownership_transition";
		if (request.m_sCause == "military_capture"
			|| request.m_sCause == "mission_capture")
			eventKind = "zone_captured";
		if (!m_Strategic.CanBuildStrategicEventId(state, eventKind, failure))
			return false;
		return true;
	}

	bool ReconcileAfterRestore(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		array<string> pendingRequestIds = {};
		foreach (HST_OwnershipTransitionState ownerAppliedCandidate : state.m_aOwnershipTransitions)
		{
			if (!ownerAppliedCandidate || ownerAppliedCandidate.m_bCompleted
				|| ownerAppliedCandidate.m_bQuarantined
				|| !ownerAppliedCandidate.m_bOwnerApplied
				|| !ownerAppliedCandidate.m_sProjectionParentRequestId.IsEmpty())
				continue;
			pendingRequestIds.Insert(ownerAppliedCandidate.m_sRequestId);
		}
		foreach (HST_OwnershipTransitionState candidate : state.m_aOwnershipTransitions)
		{
			if (!candidate || candidate.m_bCompleted || candidate.m_bQuarantined)
				continue;
			if (!pendingRequestIds.Contains(candidate.m_sRequestId))
				pendingRequestIds.Insert(candidate.m_sRequestId);
		}
		foreach (string requestId : pendingRequestIds)
		{
			HST_OwnershipTransitionState transition = state.FindOwnershipTransition(requestId);
			if (!transition || transition.m_bCompleted || transition.m_bQuarantined)
				continue;
			HST_OwnershipTransitionRequest request = RequestFromTransition(transition);
			HST_OwnershipTransitionResult result = Apply(state, request);
			if (result && result.m_bStateChanged)
				changed = true;
			if (!result || (!result.m_bAccepted && !result.m_bNeedsRetry
				&& !transition.m_bQuarantined))
			{
				QuarantineTransition(state, transition, "restore could not safely resume ownership transition");
				changed = true;
			}
		}
		return changed;
	}

	// Accepted partial transitions remain live authority during the current
	// process as well as across restore. Rate limiting keeps a persistent
	// external blocker from becoming a per-frame retry loop.
	bool TickPending(HST_CampaignState state, bool bypassCampaignClockRateLimit = false)
	{
		if (!state)
			return false;

		bool changed;
		array<string> pendingRequestIds = {};
		foreach (HST_OwnershipTransitionState ownerAppliedCandidate : state.m_aOwnershipTransitions)
		{
			if (!ownerAppliedCandidate || ownerAppliedCandidate.m_bCompleted
				|| ownerAppliedCandidate.m_bQuarantined
				|| !ownerAppliedCandidate.m_bOwnerApplied
				|| !ownerAppliedCandidate.m_sProjectionParentRequestId.IsEmpty())
				continue;
			if (!bypassCampaignClockRateLimit && ownerAppliedCandidate.m_iLastAttemptAtSecond > 0
				&& state.m_iElapsedSeconds < ownerAppliedCandidate.m_iLastAttemptAtSecond
					+ PENDING_RETRY_INTERVAL_SECONDS)
				continue;
			pendingRequestIds.Insert(ownerAppliedCandidate.m_sRequestId);
		}
		foreach (HST_OwnershipTransitionState candidate : state.m_aOwnershipTransitions)
		{
			if (!candidate || candidate.m_bCompleted || candidate.m_bQuarantined)
				continue;
			if (!bypassCampaignClockRateLimit && candidate.m_iLastAttemptAtSecond > 0
				&& state.m_iElapsedSeconds < candidate.m_iLastAttemptAtSecond + PENDING_RETRY_INTERVAL_SECONDS)
				continue;
			if (!pendingRequestIds.Contains(candidate.m_sRequestId))
				pendingRequestIds.Insert(candidate.m_sRequestId);
		}

		foreach (string requestId : pendingRequestIds)
		{
			HST_OwnershipTransitionState transition = state.FindOwnershipTransition(requestId);
			if (!transition || transition.m_bCompleted || transition.m_bQuarantined)
				continue;
			HST_OwnershipTransitionResult result = Apply(state, RequestFromTransition(transition));
			if (result && result.m_bStateChanged)
				changed = true;
			if (!result || (!result.m_bAccepted && !result.m_bNeedsRetry
				&& !transition.m_bQuarantined))
			{
				QuarantineTransition(state, transition, "runtime retry could not safely resume ownership transition");
				SchedulePartialPersistence();
				changed = true;
			}
		}
		return changed;
	}

	bool ConsumeStateChanged()
	{
		bool changed = m_bStateChangedSinceConsume;
		m_bStateChangedSinceConsume = false;
		return changed;
	}

	protected HST_OwnershipTransitionResult ContinueAcceptedTransitionScoped(
		HST_CampaignState state,
		HST_OwnershipTransitionState transition,
		HST_OwnershipTransitionResult result)
	{
		if (!transition)
			return Reject(result, "ownership transition scope is unavailable");
		m_aApplicationRequestStack.Insert(transition.m_sRequestId);
		HST_OwnershipTransitionResult scopedResult = ContinueAcceptedTransition(state, transition, result);
		int lastIndex = m_aApplicationRequestStack.Count() - 1;
		if (lastIndex >= 0)
			m_aApplicationRequestStack.Remove(lastIndex);
		return scopedResult;
	}

	protected HST_OwnershipTransitionResult ContinueAcceptedTransition(
		HST_CampaignState state,
		HST_OwnershipTransitionState transition,
		HST_OwnershipTransitionResult result)
	{
		HST_ZoneState zone = state.FindZone(transition.m_sZoneId);
		result.m_Zone = zone;
		result.m_Transition = transition;
		if (!zone)
			return QuarantineResult(state, transition, result, "transition zone disappeared");
		if (!ResumeStateMatches(zone, transition))
			return QuarantineResult(state, transition, result, "zone owner/revision diverged from the transition receipt");

		transition.m_iAttemptCount++;
		transition.m_iLastAttemptAtSecond = state.m_iElapsedSeconds;
		transition.m_sFailureReason = "";
		transition.m_sStatus = "applying";
		result.m_bAccepted = true;
		if (HasEarlierUnresolvedTopLevelTransition(state, transition))
			return NeedsRetry(
				transition,
				result,
				"ownership transition is durably queued behind earlier publication authority");

		// Schema-67 admission is the first fallible domain handoff. A bounded
		// resource rejection must leave old/new security, hostile runtime, and
		// support policy untouched so retry resumes from one exact pre-owner state.
		HST_OwnershipTransitionResult stopped
			= ContinuePreOwnerStrategicAdmissionSteps(
			state,
			zone,
			transition,
			result);
		if (stopped)
			return stopped;

		stopped = ContinueSecurityAndSupportSteps(
			state,
			zone,
			transition,
			result);
		if (stopped)
			return stopped;

		stopped = ContinueOwnerAndDerivedSteps(state, zone, transition, result);
		if (stopped)
			return stopped;

		stopped = ContinueOutcomeAndEventSteps(state, zone, transition, result);
		if (stopped)
			return stopped;

		stopped = ContinueProjectionAndDurabilitySteps(state, zone, transition, result);
		if (stopped)
			return stopped;

		return CompleteAcceptedTransition(state, zone, transition, result);
	}

	protected HST_OwnershipTransitionResult ContinueSecurityAndSupportSteps(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition,
		HST_OwnershipTransitionResult result)
	{
		bool recheckPreOwnerSecurity = transition.m_bReconcileSecurity && !transition.m_bOwnerApplied;
		if (!transition.m_bOldSecurityRetired || recheckPreOwnerSecurity)
		{
			if (!transition.m_bReconcileSecurity)
			{
				if (!transition.m_bOldSecurityRetired)
				{
					transition.m_bOldSecurityRetired = true;
					result.m_bStateChanged = true;
				}
			}
			else
			{
				string authorityFailure = ValidateOldSecurityAuthority(
					state,
					zone,
					transition.m_sNewOwnerFactionKey);
				if (!authorityFailure.IsEmpty())
					return NeedsRetry(transition, result, authorityFailure);
				bool localSecurityChanged;
				string localSecurityFailure;
				if (!m_LocalSecurityPatrols.ReconcileZoneOwnershipChange(
					state,
					zone.m_sZoneId,
					transition.m_sNewOwnerFactionKey,
					localSecurityChanged,
					localSecurityFailure))
					return NeedsRetry(transition, result, localSecurityFailure);
				result.m_bStateChanged = localSecurityChanged || result.m_bStateChanged;
				bool patrolChanged;
				string patrolFailure;
				if (!m_GarrisonPatrols.ReconcileZoneOwnershipChange(
					state,
					zone.m_sZoneId,
					transition.m_sNewOwnerFactionKey,
					patrolChanged,
					patrolFailure))
					return NeedsRetry(transition, result, patrolFailure);
				result.m_bStateChanged = patrolChanged || result.m_bStateChanged;
				RetireOldAbstractSecurity(state, zone, transition);
				if (!transition.m_bOldSecurityRetired)
					result.m_bStateChanged = true;
				transition.m_bOldSecurityRetired = true;
			}
		}

		if (!transition.m_bHostileRuntimeRetired || recheckPreOwnerSecurity)
		{
			bool runtimeChanged;
			if (transition.m_bReconcileSecurity)
				runtimeChanged = m_PhysicalWar.CleanupZoneHostileRuntime(
					state,
					zone.m_sZoneId,
					transition.m_sNewOwnerFactionKey);
			if (!transition.m_bHostileRuntimeRetired)
				result.m_bStateChanged = true;
			transition.m_bHostileRuntimeRetired = true;
			result.m_bStateChanged = runtimeChanged || result.m_bStateChanged;
		}

		if (!transition.m_bNewSecurityApplied)
		{
			if (transition.m_bReconcileSecurity && !ApplyNewSecurityPolicy(state, zone, transition))
				return NeedsRetry(transition, result, "new-owner security policy could not be established");
			transition.m_bNewSecurityApplied = true;
			result.m_bStateChanged = true;
		}

		if (!transition.m_bSupportApplied)
		{
			string supportFailure = ApplySupportConsequences(state, transition);
			if (!supportFailure.IsEmpty())
				return NeedsRetry(transition, result, supportFailure);
			transition.m_bSupportApplied = true;
			result.m_bStateChanged = true;
		}
		return null;
	}

	// Schema-67 aggression admission is fallible and durable. It must settle
	// before the visible zone owner/revision changes so a bounded-capacity,
	// role, arithmetic, or backlink rejection can retry without publishing a
	// half-applied owner. The stable mutation ID makes this step replay-safe if
	// an older partial receipt resumes after its owner was already applied.
	protected HST_OwnershipTransitionResult ContinuePreOwnerStrategicAdmissionSteps(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition,
		HST_OwnershipTransitionResult result)
	{
		if (!transition || transition.m_bEnemyConsequencesApplied)
			return null;

		if (!transition.m_bOwnerApplied)
		{
			string ownerEffectFailure = ValidateStrategicOwnerEffectAdmission(
				state,
				transition);
			if (!ownerEffectFailure.IsEmpty())
				return QuarantineResult(
					state,
					transition,
					result,
					ownerEffectFailure);
		}

		int aggressionBefore = transition.m_iAggressionApplied;
		string aggressionFailure = AdmitEnemyAggression(
			state,
			zone,
			transition);
		if (!aggressionFailure.IsEmpty())
			return NeedsRetry(transition, result, aggressionFailure);
		if (transition.m_iAggressionApplied != aggressionBefore)
			result.m_bStateChanged = true;
		return null;
	}

	protected HST_OwnershipTransitionResult ContinueOwnerAndDerivedSteps(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition,
		HST_OwnershipTransitionResult result)
	{
		// Every retry-capable authority handoff is settled before the visible
		// owner/revision changes. This keeps a recoverable blocker from exposing
		// a half-applied new owner to unrelated once-per-second systems.
		if (!transition.m_bOwnerApplied)
		{
			if (!m_Strategic.RecordOwnershipTransitionOwnerEffect(
					state,
					transition.m_sStrategicEventId,
					transition.m_sPreviousOwnerFactionKey,
					transition.m_sNewOwnerFactionKey,
					zone.m_iResistanceCaptureProgress))
				return QuarantineResult(state, transition, result, "ownership strategic event could not record the owner effect");
			zone.m_sOwnerFactionKey = transition.m_sNewOwnerFactionKey;
			zone.m_iOwnershipRevision = transition.m_iExpectedRevision + 1;
			zone.m_sActiveOwnershipTransitionRequestId = transition.m_sRequestId;
			zone.m_iResistanceCaptureProgress = 0;
			zone.m_bActive = false;
			zone.m_iActiveInfantryCount = 0;
			zone.m_iActiveVehicleCount = 0;
			transition.m_iAppliedRevision = zone.m_iOwnershipRevision;
			transition.m_bOwnerApplied = true;
			result.m_bStateChanged = true;
		}

		if (!transition.m_bTownPolicyApplied)
		{
			if (transition.m_bReconcileSecurity)
				ApplyTownOwnershipPolicy(state, zone, transition);
			else
				transition.m_sSecurityDecision = "migration policy preserved existing town influence and security facts";
			transition.m_bTownPolicyApplied = true;
			result.m_bStateChanged = true;
		}

		if (!transition.m_bFacilitiesApplied || !transition.m_bLogisticsApplied)
		{
			SynchronizeDerivedLocationOwnership(state, zone, transition);
			transition.m_bFacilitiesApplied = true;
			transition.m_bLogisticsApplied = true;
			result.m_bStateChanged = true;
		}

		if (!transition.m_bEnemyConsequencesApplied)
		{
			if (!ApplyEnemyConsequences(state, zone, transition))
				return NeedsRetry(
					transition,
					result,
					transition.m_sEnemyConsequenceDecision);
			transition.m_bEnemyConsequencesApplied = true;
			result.m_bStateChanged = true;
		}
		return null;
	}

	protected HST_OwnershipTransitionResult ContinueOutcomeAndEventSteps(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition,
		HST_OwnershipTransitionResult result)
	{
		if (!transition.m_bEconomyApplied)
		{
			m_Economy.RecalculateWarLevel(state, m_Balance, m_Preset.m_sResistanceFactionKey);
			HST_CampaignOutcomeResult outcome = m_Strategic.EvaluateCampaignOutcomeDetailed(
				state,
				m_Economy,
				m_Balance,
				m_Preset.m_sResistanceFactionKey);
			if (outcome && outcome.m_bEnded)
				m_Strategic.ApplyCampaignOutcome(state, outcome);
			transition.m_bEconomyApplied = true;
			result.m_bStateChanged = true;
		}

		if (!transition.m_bStrategicEventCompleted)
		{
			HST_StrategicEventState strategicEvent = state.FindStrategicEvent(transition.m_sStrategicEventId);
			if (!strategicEvent)
				return QuarantineResult(state, transition, result, "ownership strategic event disappeared");
			HST_StrategicEventApplyResult applyResult = new HST_StrategicEventApplyResult();
			applyResult.m_Event = strategicEvent;
			applyResult.m_sEventId = strategicEvent.m_sEventId;
			applyResult.m_bRecorded = true;
			m_Strategic.CompleteOwnershipTransitionEvent(
				state,
				applyResult,
				transition.m_sPreviousOwnerFactionKey,
				transition.m_sNewOwnerFactionKey,
				transition.m_iAggressionApplied);
			transition.m_bStrategicEventCompleted = true;
			result.m_bStateChanged = true;
		}

		if (!transition.m_bEventAppended)
		{
			HST_CampaignEventState campaignEvent = m_CampaignEvents.Append(
				state,
				"ownership",
				"zone",
				zone.m_sZoneId,
				transition.m_sRequestId,
				string.Format("owner_%1_to_%2_revision_%3", transition.m_sPreviousOwnerFactionKey, transition.m_sNewOwnerFactionKey, transition.m_iAppliedRevision),
				transition.m_sReason);
			if (!campaignEvent)
				return NeedsRetry(transition, result, "campaign ownership event could not be appended");
			transition.m_sCampaignEventId = campaignEvent.m_sEventId;
			transition.m_bEventAppended = true;
			result.m_bStateChanged = true;
		}
		return null;
	}

	protected HST_OwnershipTransitionResult ContinueProjectionAndDurabilitySteps(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition,
		HST_OwnershipTransitionResult result)
	{
		transition.m_sStatus = "projecting";
		if (!transition.m_bProjectionRequested)
		{
			string projectionFailure = ProjectTransition(state, zone, transition);
			if (!projectionFailure.IsEmpty())
				return NeedsRetry(transition, result, projectionFailure);
			transition.m_bProjectionRequested = true;
			result.m_bStateChanged = true;
		}

		if (!transition.m_bNotificationApplied)
		{
			if (transition.m_bNotify && transition.m_sProjectionParentRequestId.IsEmpty())
				m_ZoneCapture.QueueOwnershipTransitionNotification(transition, zone);
			transition.m_bNotificationApplied = true;
			result.m_bStateChanged = true;
		}

		if (!transition.m_bPersistenceRequested)
		{
			m_Persistence.MarkMajorChange();
			transition.m_bPersistenceRequested = true;
			result.m_bStateChanged = true;
		}
		return null;
	}

	protected HST_OwnershipTransitionResult CompleteAcceptedTransition(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition,
		HST_OwnershipTransitionResult result)
	{
		transition.m_sStatus = "completed";
		transition.m_bCompleted = true;
		transition.m_iCompletedAtSecond = state.m_iElapsedSeconds;
		zone.m_sActiveOwnershipTransitionRequestId = "";
		zone.m_sLastOwnershipTransitionRequestId = transition.m_sRequestId;
		zone.m_sOwnershipAuthorityFailure = "";
		result.m_bCompleted = true;
		result.m_bStateChanged = true;
		m_bStateChangedSinceConsume = true;
		PruneCompletedHistory(state);
		// The persisted checklist may already say that durability was requested
		// before a restart. Re-arm the process-local deadline after the final
		// completion/backlink mutation without extending an already pending one.
		m_Persistence.EnsureMajorChangePending();
		Print(result.BuildReport());
		return result;
	}

	protected string ValidateNewRequest(HST_CampaignState state, HST_OwnershipTransitionRequest request)
	{
		if (request.m_sRequestId.IsEmpty() || request.m_sRequestId.Length() > MAX_REQUEST_ID_CHARACTERS)
			return "ownership request id is empty or too long";
		if (request.m_sZoneId.IsEmpty() || request.m_sNewOwnerFactionKey.IsEmpty())
			return "ownership request requires zone and target owner";
		if (!IsKnownCause(request.m_sCause))
			return "ownership request cause is unsupported";
		string causePolicyFailure = ValidateCausePolicy(request);
		if (!causePolicyFailure.IsEmpty())
			return causePolicyFailure;
		if (request.m_sSourceType.IsEmpty() || request.m_sSourceType.Length() > 80
			|| request.m_sSourceId.IsEmpty() || request.m_sSourceId.Length() > MAX_SOURCE_CHARACTERS
			|| request.m_sActorIdentityId.Length() > MAX_SOURCE_CHARACTERS
			|| request.m_sReason.Trim().IsEmpty()
			|| request.m_sReason.Length() > MAX_REASON_CHARACTERS)
			return "ownership request source or reason is invalid";
		if (request.m_iExpectedRevision <= 0 || request.m_iExpectedRevision >= int.MAX - 1)
			return "ownership request expected revision is invalid";
		if (request.m_iSupportReward < -100 || request.m_iSupportReward > 100)
			return "ownership support reward is outside the bounded range";
		if (!DependenciesReady(request))
			return "ownership transition dependencies are unavailable";
		HST_ZoneState zone = state.FindZone(request.m_sZoneId);
		if (!zone)
			return "ownership zone was not found";
		if (zone.m_iOwnershipContractVersion == QUARANTINED_CONTRACT_VERSION || !zone.m_sOwnershipAuthorityFailure.IsEmpty())
			return "ownership zone authority is quarantined: " + zone.m_sOwnershipAuthorityFailure;
		if (zone.m_iOwnershipContractVersion != EXACT_CONTRACT_VERSION)
			return "ownership zone contract version is unsupported";
		if (zone.m_eType == HST_EZoneType.HST_ZONE_HIDEOUT || zone.m_eType == HST_EZoneType.HST_ZONE_MISSION_SITE)
			return "ownership transition cannot target hideout or mission-site aggregates";
		if (!state.FindFactionPool(request.m_sNewOwnerFactionKey))
			return "ownership target faction has no campaign pool";
		if (request.m_sCause != "migration_repair"
			&& !state.FindFactionPool(request.m_sExpectedOwnerFactionKey))
			return "ownership expected faction has no campaign pool";
		if (zone.m_sOwnerFactionKey != request.m_sExpectedOwnerFactionKey)
			return "ownership expected owner is stale";
		if (Math.Max(1, zone.m_iOwnershipRevision) != request.m_iExpectedRevision)
			return "ownership expected revision is stale";
		if (zone.m_sOwnerFactionKey == request.m_sNewOwnerFactionKey)
			return "ownership target already controls the zone";
		if (!zone.m_sActiveOwnershipTransitionRequestId.IsEmpty()
			&& zone.m_sActiveOwnershipTransitionRequestId != request.m_sRequestId)
			return "another ownership transition already owns this zone";
		HST_OwnershipTransitionState latestTransition;
		if (!zone.m_sLastOwnershipTransitionRequestId.IsEmpty())
			latestTransition = state.FindOwnershipTransition(zone.m_sLastOwnershipTransitionRequestId);
		if (latestTransition && !latestTransition.m_sProjectionParentRequestId.IsEmpty()
			&& !latestTransition.m_bDeferredPublicationReleased)
			return "zone ownership publication is still delegated to a live parent transition";

		if (request.m_bReconcileSecurity)
		{
			string securityFailure = ValidateOldSecurityAuthority(state, zone, request.m_sNewOwnerFactionKey);
			if (!securityFailure.IsEmpty())
				return securityFailure;
		}
		return "";
	}

	protected bool HasEarlierUnresolvedTopLevelTransition(
		HST_CampaignState state,
		HST_OwnershipTransitionState transition)
	{
		if (!state || !transition || !transition.m_sProjectionParentRequestId.IsEmpty())
			return false;
		foreach (HST_OwnershipTransitionState candidate : state.m_aOwnershipTransitions)
		{
			if (candidate == transition || (candidate
				&& candidate.m_sRequestId == transition.m_sRequestId))
				return false;
			if (!candidate || candidate.m_bCompleted
				|| !candidate.m_sProjectionParentRequestId.IsEmpty())
				continue;
			return true;
		}
		return false;
	}

	protected string ValidateOldSecurityAuthority(HST_CampaignState state, HST_ZoneState zone, string newOwnerFactionKey)
	{
		string localSecurityFailure;
		if (!m_LocalSecurityPatrols.CanReconcileZoneOwnershipChange(
			state,
			zone.m_sZoneId,
			newOwnerFactionKey,
			localSecurityFailure))
			return localSecurityFailure;

		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
		{
			if (!garrison || garrison.m_sZoneId != zone.m_sZoneId || garrison.m_sFactionKey == newOwnerFactionKey)
				continue;
			foreach (string manifestId : garrison.m_aAcceptedManifestIds)
			{
				HST_ForceManifestState manifest = state.FindForceManifest(manifestId);
				if (!manifest || manifest.m_sPolicyId != HST_GarrisonPatrolOperationService.EXACT_POLICY_ID)
					return "non-patrol exact garrison authority must be settled before ownership can change";
				string manifestFailure;
				if (!m_GarrisonPatrols.ValidateAcceptedManifestOwnershipAuthority(
						state,
						garrison,
						manifest,
						manifestFailure))
					return manifestFailure;
			}
		}

		string patrolFailure;
		if (!m_GarrisonPatrols.CanReconcileZoneOwnershipChange(state, zone.m_sZoneId, newOwnerFactionKey, patrolFailure))
			return patrolFailure;
		return "";
	}

	protected bool DependenciesReady(HST_OwnershipTransitionRequest request)
	{
		if (!m_Preset || !m_Balance || !m_Economy || !m_Strategic || !m_Garrisons
			|| !m_Civilians || !m_TownInfluence || !m_CampaignEvents || !m_PhysicalWar
			|| !m_LocalSecurityPatrols || !m_GarrisonPatrols
			|| !m_ZoneCapture || !m_MapMarkers || !m_Persistence)
			return false;
		if (request.m_bApplyEnemyConsequences && (!m_EnemyCommander || !m_EnemyDirector))
			return false;
		return true;
	}

	protected HST_OwnershipTransitionState CreateTransition(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionRequest request)
	{
		HST_OwnershipTransitionState transition = new HST_OwnershipTransitionState();
		transition.m_iContractVersion = EXACT_CONTRACT_VERSION;
		transition.m_sStatus = "validated";
		transition.m_sRequestId = request.m_sRequestId;
		transition.m_sZoneId = zone.m_sZoneId;
		transition.m_sCause = request.m_sCause;
		transition.m_sSourceType = request.m_sSourceType;
		transition.m_sSourceId = request.m_sSourceId;
		transition.m_sActorIdentityId = request.m_sActorIdentityId;
		transition.m_sReason = request.m_sReason;
		transition.m_sExpectedOwnerFactionKey = request.m_sExpectedOwnerFactionKey;
		transition.m_iExpectedRevision = request.m_iExpectedRevision;
		transition.m_sPreviousOwnerFactionKey = zone.m_sOwnerFactionKey;
		transition.m_sNewOwnerFactionKey = request.m_sNewOwnerFactionKey;
		transition.m_iSupportReward = request.m_iSupportReward;
		transition.m_bApplyEnemyConsequences = request.m_bApplyEnemyConsequences;
		transition.m_bReconcileSecurity = request.m_bReconcileSecurity;
		transition.m_bCreateSecurity = request.m_bCreateSecurity;
		transition.m_bNotify = request.m_bNotify;
		transition.m_iCreatedAtSecond = state.m_iElapsedSeconds;
		transition.m_bValidated = true;
		if (!m_aApplicationRequestStack.IsEmpty())
			transition.m_sProjectionParentRequestId = m_aApplicationRequestStack[m_aApplicationRequestStack.Count() - 1];
		PrepareSupportZoneIds(state, zone, transition);
		FreezeEnemyConsequenceDecision(state, zone, transition);

		HST_StrategicEventApplyResult strategicEvent = m_Strategic.BeginOwnershipTransitionEvent(
			state,
			zone.m_sZoneId,
			zone.m_sOwnerFactionKey,
			request.m_sNewOwnerFactionKey,
			request.m_sCause,
			request.m_sSourceType,
			request.m_sSourceId,
			request.m_sReason);
		if (strategicEvent && strategicEvent.m_Event)
			transition.m_sStrategicEventId = strategicEvent.m_sEventId;
		return transition;
	}

	protected void ApplyTownOwnershipPolicy(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition)
	{
		if (zone.m_eType != HST_EZoneType.HST_ZONE_TOWN)
		{
			transition.m_sSecurityDecision = "town policy not applicable; strategic-site policy selected";
			return;
		}

		HST_CivilianZoneState town = state.FindCivilianZone(zone.m_sZoneId);
		if (!town)
		{
			transition.m_sSecurityDecision = "town influence row unavailable; owner changed without inventing political facts";
			return;
		}

		if (transition.m_sNewOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
		{
			town.m_iPolicePresence = 0;
			town.m_iRoadblockPresence = 0;
			town.m_sLastSecurityReason = "canonical ownership transition retired enemy town security";
		}
		else
		{
			town.m_iPolicePresence = Math.Max(1, Math.Max(town.m_iPolicePresence, zone.m_iGarrisonSlots / 6));
			town.m_sLastSecurityReason = "canonical ownership transition established enemy town security";
		}
		town.m_bUndercoverRestricted
			= m_TownInfluence.ResolveSignedSupportPercent(state, zone.m_sZoneId) < 25;
		transition.m_sSecurityDecision = town.m_sLastSecurityReason;
	}

	protected void RetireOldAbstractSecurity(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition)
	{
		foreach (HST_GarrisonState garrison : state.m_aGarrisons)
		{
			if (!garrison || garrison.m_sZoneId != zone.m_sZoneId || garrison.m_sFactionKey == transition.m_sNewOwnerFactionKey)
				continue;
			if (transition.m_sOldGarrisonId.IsEmpty()
				&& garrison.m_sFactionKey == transition.m_sPreviousOwnerFactionKey)
				transition.m_sOldGarrisonId = garrison.m_sGarrisonId;
			garrison.m_iInfantryCount = 0;
			garrison.m_iVehicleCount = 0;
		}
	}

	protected bool ApplyNewSecurityPolicy(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition)
	{
		if (!transition.m_bCreateSecurity || zone.m_iGarrisonSlots <= 0)
		{
			transition.m_sSecurityDecision = transition.m_sSecurityDecision + " | new garrison not applicable";
			return true;
		}

		HST_GarrisonState garrison = m_Garrisons.FindOrCreate(state, zone.m_sZoneId, transition.m_sNewOwnerFactionKey);
		if (!garrison)
			return false;
		transition.m_sNewGarrisonId = garrison.m_sGarrisonId;
		if (!garrison.m_aAcceptedManifestIds.IsEmpty())
		{
			transition.m_sSecurityDecision = transition.m_sSecurityDecision + " | retained existing exact new-owner garrison authority";
			return true;
		}

		int infantryCount = Math.Min(2, zone.m_iGarrisonSlots);
		int vehicleCount;
		if (transition.m_sNewOwnerFactionKey != m_Preset.m_sResistanceFactionKey)
		{
			infantryCount = Math.Min(zone.m_iGarrisonSlots, Math.Max(2, zone.m_iGarrisonSlots / 2 + zone.m_iPriority / 8));
			if (zone.m_eType == HST_EZoneType.HST_ZONE_OUTPOST
				|| zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD
				|| zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT
				|| zone.m_eType == HST_EZoneType.HST_ZONE_FACTORY)
				vehicleCount = 1;
			if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
				vehicleCount = 2;
		}
		garrison.m_iInfantryCount = Math.Max(garrison.m_iInfantryCount, infantryCount);
		garrison.m_iVehicleCount = Math.Max(garrison.m_iVehicleCount, vehicleCount);
		transition.m_sSecurityDecision = transition.m_sSecurityDecision
			+ string.Format(" | new garrison %1 infantry %2 vehicles %3", garrison.m_sGarrisonId, garrison.m_iInfantryCount, garrison.m_iVehicleCount);
		return true;
	}

	protected string ApplySupportConsequences(HST_CampaignState state, HST_OwnershipTransitionState transition)
	{
		if (transition.m_iSupportReward == 0 || transition.m_aSupportZoneIds.IsEmpty())
			return "";

		foreach (string zoneId : transition.m_aSupportZoneIds)
		{
			if (transition.m_aAppliedSupportZoneIds.Contains(zoneId))
				continue;
			bool reconcileOwnership = zoneId != transition.m_sZoneId;
			if (reconcileOwnership && HasLaterPristineQueuedTopLevelForZone(
					state,
					transition,
					zoneId))
				reconcileOwnership = false;
			if (!m_TownInfluence.RegisterOwnershipSupportReward(
					state,
					m_Preset,
					zoneId,
					transition.m_iSupportReward,
					transition.m_sRequestId,
					reconcileOwnership))
				return "ownership support consequence failed for town " + zoneId;
			transition.m_aAppliedSupportZoneIds.Insert(zoneId);
		}
		return "";
	}

	// A later accepted top-level receipt owns its zone backlink while it waits
	// behind this parent. If this parent's frozen support event crosses that
	// town, applying the exact influence fact is safe, but trying to admit a
	// nested political child would make the two receipts wait on each other.
	// Only a provably later, pristine receipt receives this deferral; any other
	// policy failure remains visible to the parent retry path.
	protected bool HasLaterPristineQueuedTopLevelForZone(
		HST_CampaignState state,
		HST_OwnershipTransitionState parent,
		string zoneId)
	{
		if (!state || !parent || zoneId.IsEmpty())
			return false;

		bool parentSeen;
		foreach (HST_OwnershipTransitionState candidate : state.m_aOwnershipTransitions)
		{
			if (!parentSeen)
			{
				if (candidate && candidate.m_sRequestId == parent.m_sRequestId)
					parentSeen = true;
				continue;
			}
			if (!IsPristineQueuedTopLevelTransition(candidate) || candidate.m_sZoneId != zoneId)
				continue;

			HST_ZoneState zone = state.FindZone(zoneId);
			return zone && zone.m_sActiveOwnershipTransitionRequestId == candidate.m_sRequestId;
		}
		return false;
	}

	protected bool IsPristineQueuedTopLevelTransition(HST_OwnershipTransitionState transition)
	{
		if (!transition || transition.m_bCompleted || transition.m_bQuarantined)
			return false;
		if (!transition.m_sProjectionParentRequestId.IsEmpty() || transition.m_bOwnerApplied)
			return false;
		if (transition.m_iAppliedRevision > 0 || transition.m_bOldSecurityRetired
			|| transition.m_bHostileRuntimeRetired || transition.m_bNewSecurityApplied)
			return false;
		if (transition.m_bSupportApplied || transition.m_bTownPolicyApplied
			|| transition.m_bFacilitiesApplied || transition.m_bLogisticsApplied)
			return false;
		if (transition.m_bEconomyApplied || transition.m_bEnemyConsequencesApplied
			|| transition.m_bStrategicEventCompleted || transition.m_bEventAppended)
			return false;
		if (transition.m_bProjectionRequested || transition.m_bNotificationApplied
			|| transition.m_bSetupProjectionWithoutMarkers || transition.m_bPersistenceRequested)
			return false;
		return true;
	}

	protected void SynchronizeDerivedLocationOwnership(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition)
	{
		int generatedSitesChanged;
		foreach (HST_GeneratedSiteState site : state.m_aGeneratedSites)
		{
			if (!site || site.m_sZoneId != zone.m_sZoneId || site.m_sOwnerFactionKey == zone.m_sOwnerFactionKey)
				continue;
			site.m_sOwnerFactionKey = zone.m_sOwnerFactionKey;
			generatedSitesChanged++;
		}
		transition.m_sFacilityLogisticsDecision = string.Format(
			"owner-derived income, facility, radio, respawn, supply, and logistics views invalidated; generated sites synchronized %1",
			generatedSitesChanged);
	}

	protected bool ApplyEnemyConsequences(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition)
	{
		if (!transition.m_bApplyEnemyConsequences
			|| transition.m_sNewOwnerFactionKey != m_Preset.m_sResistanceFactionKey
			|| transition.m_sPreviousOwnerFactionKey.IsEmpty()
			|| transition.m_sPreviousOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
		{
			transition.m_sEnemyConsequenceDecision = "enemy retaliation not applicable for this ownership policy";
			return true;
		}

		if (transition.m_iAggressionApplied <= 0)
		{
			transition.m_sEnemyConsequenceDecision
				= "enemy aggression resource mutation was not admitted before owner publication";
			return false;
		}
		m_EnemyDirector.RecordZoneDamageSignal(
			state,
			transition.m_sPreviousOwnerFactionKey,
			zone,
			Math.Max(8, transition.m_iAggressionApplied),
			"canonical ownership transition pressure");

		if (transition.m_bCounterattackSelected)
		{
			int orderCountBefore = state.m_aEnemyOrders.Count();
			transition.m_bCounterattackQueued = m_EnemyCommander.TryQueueImmediateCounterattack(
				state,
				m_Preset,
				m_EnemyDirector,
				m_SupportRequests,
				transition.m_sPreviousOwnerFactionKey,
				zone,
				100);
			if (transition.m_bCounterattackQueued && state.m_aEnemyOrders.Count() > orderCountBefore)
			{
				HST_EnemyOrderState order = state.m_aEnemyOrders[state.m_aEnemyOrders.Count() - 1];
				if (order)
					transition.m_sEnemyOrderId = order.m_sOrderId;
			}
		}
		transition.m_sEnemyConsequenceDecision = string.Format(
			"aggression %1 | counterattack chance %2 roll %3 selected %4 queued %5 order %6",
			transition.m_iAggressionApplied,
			transition.m_iCounterattackChance,
			transition.m_iCounterattackRoll,
			transition.m_bCounterattackSelected,
			transition.m_bCounterattackQueued,
			transition.m_sEnemyOrderId);
		return true;
	}

	protected string AdmitEnemyAggression(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition)
	{
		if (!transition.m_bApplyEnemyConsequences
			|| transition.m_sNewOwnerFactionKey != m_Preset.m_sResistanceFactionKey
			|| transition.m_sPreviousOwnerFactionKey.IsEmpty()
			|| transition.m_sPreviousOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
			return "";

		int aggression = ResolveCaptureAggression(zone);
		if (transition.m_iAggressionApplied > 0
			&& transition.m_iAggressionApplied != aggression)
			return "ownership aggression amount diverged before owner publication";
		if (!m_Economy.AddAggression(
			state,
			transition.m_sPreviousOwnerFactionKey,
			aggression,
			"enemy_aggression_ownership_" + transition.m_sRequestId,
			"ownership_transition",
			transition.m_sRequestId,
			"",
			"",
			"",
			transition.m_sZoneId))
			return "enemy aggression resource mutation could not be admitted before owner publication";
		transition.m_iAggressionApplied = aggression;
		return "";
	}

	protected string ValidateStrategicOwnerEffectAdmission(
		HST_CampaignState state,
		HST_OwnershipTransitionState transition)
	{
		if (!state || !transition || transition.m_sStrategicEventId.IsEmpty()
			|| transition.m_sPreviousOwnerFactionKey.IsEmpty()
			|| transition.m_sNewOwnerFactionKey.IsEmpty())
			return "ownership strategic event is unavailable before resource admission";
		HST_StrategicEventState eventState
			= state.FindStrategicEvent(transition.m_sStrategicEventId);
		if (!eventState || eventState.m_bApplied
			|| (eventState.m_sKind != "ownership_transition"
				&& eventState.m_sKind != "zone_captured"))
			return "ownership strategic event cannot accept owner evidence before resource admission";
		return "";
	}

	protected string ProjectTransition(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition)
	{
		if (!transition.m_sProjectionParentRequestId.IsEmpty())
		{
			HST_OwnershipTransitionState parent = state.FindOwnershipTransition(transition.m_sProjectionParentRequestId);
			if (!parent || parent.m_bQuarantined)
				return "nested ownership projection parent is unavailable";
			transition.m_sProjectionDecision = "marker, authored/GM, menu, and notification publication delegated to parent "
				+ transition.m_sProjectionParentRequestId;
			return "";
		}

		string childPreflightFailure = ValidateDeferredChildPublications(state, transition, false);
		if (!childPreflightFailure.IsEmpty())
			return childPreflightFailure;
		HST_OwnershipMarkerProjectionTransaction markerTransaction = m_MapMarkers.StageOwnershipTransitionMarkers(
			state,
			m_Preset,
			transition.m_sRequestId);
		if (!markerTransaction)
			return "ownership marker snapshot could not be staged";
		if (state.m_ePhase == HST_ECampaignPhase.HST_CAMPAIGN_SETUP)
		{
			string setupChildFailure = ReleaseDeferredChildPublications(state, transition, false);
			if (!setupChildFailure.IsEmpty())
			{
				m_MapMarkers.RollbackOwnershipTransitionMarkers(state, markerTransaction);
				return setupChildFailure;
			}
			transition.m_bSetupProjectionWithoutMarkers = true;
			transition.m_sProjectionDecision = "setup phase has no live zone-marker projection; owner-derived views will publish on activation";
			m_MapMarkers.CommitOwnershipTransitionMarkers(state, m_Preset, markerTransaction);
			return "";
		}

		HST_MapMarkerState marker = state.FindMapMarker("hst_zone_" + zone.m_sZoneId);
		if (!marker || marker.m_sOwnerFactionKey != zone.m_sOwnerFactionKey
			|| marker.m_iSourceRevision != zone.m_iOwnershipRevision)
		{
			m_MapMarkers.RollbackOwnershipTransitionMarkers(state, markerTransaction);
			return "zone marker did not correlate to the ownership revision";
		}
		string childFailure = ReleaseDeferredChildPublications(state, transition, true);
		if (!childFailure.IsEmpty())
		{
			m_MapMarkers.RollbackOwnershipTransitionMarkers(state, markerTransaction);
			return childFailure;
		}
		transition.m_sMarkerId = marker.m_sMarkerId;
		transition.m_iMarkerProjectionEpoch = state.m_iMarkerProjectionEpoch;
		transition.m_iMarkerRevision = marker.m_iRevision;
		transition.m_iMarkerStreamSequence = marker.m_iStreamSequence;
		transition.m_sProjectionDecision = string.Format(
			"zone marker %1 source revision %2 | authored/GM, menu, and notification views derive from the same zone",
			marker.m_sMarkerId,
			marker.m_iSourceRevision);
		m_MapMarkers.CommitOwnershipTransitionMarkers(state, m_Preset, markerTransaction);
		if (m_ClientProjection)
		{
			m_ClientProjection.Synchronize(state, "ownership transition " + transition.m_sRequestId);
			HST_CommandMenuRequestComponent.PublishPendingMarkerProjectionToConnectedOwners(
				"ownership transition " + transition.m_sRequestId);
		}
		return "";
	}

	protected string ReleaseDeferredChildPublications(
		HST_CampaignState state,
		HST_OwnershipTransitionState parent,
		bool liveProjection)
	{
		string validationFailure = ValidateDeferredChildPublications(state, parent, liveProjection);
		if (!validationFailure.IsEmpty())
			return validationFailure;

		foreach (HST_OwnershipTransitionState child : state.m_aOwnershipTransitions)
		{
			if (!child || child.m_sProjectionParentRequestId != parent.m_sRequestId
				|| child.m_bDeferredPublicationReleased)
				continue;
			HST_ZoneState childZone = state.FindZone(child.m_sZoneId);

			if (liveProjection)
			{
				HST_MapMarkerState childMarker = state.FindMapMarker("hst_zone_" + childZone.m_sZoneId);
				child.m_sMarkerId = childMarker.m_sMarkerId;
				child.m_iMarkerProjectionEpoch = state.m_iMarkerProjectionEpoch;
				child.m_iMarkerRevision = childMarker.m_iRevision;
				child.m_iMarkerStreamSequence = childMarker.m_iStreamSequence;
			}

			child.m_sProjectionDecision = child.m_sProjectionDecision
				+ " | publication released by parent " + parent.m_sRequestId;
			if (!liveProjection)
				child.m_bSetupProjectionWithoutMarkers = true;
			if (child.m_bNotify)
				m_ZoneCapture.QueueOwnershipTransitionNotification(child, childZone);
			child.m_bDeferredPublicationReleased = true;
		}
		return "";
	}

	protected string ValidateDeferredChildPublications(
		HST_CampaignState state,
		HST_OwnershipTransitionState parent,
		bool liveProjection)
	{
		if (!state || !parent)
			return "deferred ownership publication requires state and parent authority";
		foreach (HST_OwnershipTransitionState child : state.m_aOwnershipTransitions)
		{
			if (!child || child.m_sProjectionParentRequestId != parent.m_sRequestId
				|| child.m_bDeferredPublicationReleased)
				continue;
			if (!child.m_bCompleted || child.m_bQuarantined
				|| child.m_iContractVersion != EXACT_CONTRACT_VERSION
				|| !child.m_bOwnerApplied)
				return "nested ownership transition is not complete for parent publication";
			HST_ZoneState childZone = state.FindZone(child.m_sZoneId);
			if (!childZone || childZone.m_sLastOwnershipTransitionRequestId != child.m_sRequestId)
				return "nested ownership transition zone backlink is not publishable";
			if (child.m_sPreviousOwnerFactionKey != child.m_sExpectedOwnerFactionKey
				|| child.m_iAppliedRevision != child.m_iExpectedRevision + 1
				|| childZone.m_iOwnershipContractVersion != EXACT_CONTRACT_VERSION
				|| !childZone.m_sOwnershipAuthorityFailure.IsEmpty()
				|| !childZone.m_sActiveOwnershipTransitionRequestId.IsEmpty()
				|| childZone.m_sOwnerFactionKey != child.m_sNewOwnerFactionKey
				|| childZone.m_iOwnershipRevision != child.m_iAppliedRevision)
				return "nested ownership transition zone authority does not match its completed receipt";
			if (liveProjection)
			{
				HST_MapMarkerState childMarker = state.FindMapMarker("hst_zone_" + childZone.m_sZoneId);
				if (!childMarker || childMarker.m_sOwnerFactionKey != childZone.m_sOwnerFactionKey
					|| childMarker.m_iSourceRevision != childZone.m_iOwnershipRevision)
					return "nested ownership marker did not correlate to its source revision";
			}
		}
		return "";
	}

	protected void PrepareSupportZoneIds(
		HST_CampaignState state,
		HST_ZoneState capturedZone,
		HST_OwnershipTransitionState transition)
	{
		if (!state || !capturedZone || transition.m_iSupportReward == 0)
			return;
		foreach (string linkedZoneId : capturedZone.m_aLinkedZoneIds)
		{
			HST_ZoneState linkedZone = state.FindZone(linkedZoneId);
			if (linkedZone && linkedZone.m_eType == HST_EZoneType.HST_ZONE_TOWN
				&& !transition.m_aSupportZoneIds.Contains(linkedZone.m_sZoneId))
				transition.m_aSupportZoneIds.Insert(linkedZone.m_sZoneId);
		}

		float supportRadiusSq = 1500.0 * 1500.0;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_TOWN
				|| transition.m_aSupportZoneIds.Contains(zone.m_sZoneId))
				continue;
			if (DistanceSq2D(zone.m_vPosition, capturedZone.m_vPosition) <= supportRadiusSq)
				transition.m_aSupportZoneIds.Insert(zone.m_sZoneId);
		}
		transition.m_aSupportZoneIds.Sort();
	}

	protected void FreezeEnemyConsequenceDecision(
		HST_CampaignState state,
		HST_ZoneState zone,
		HST_OwnershipTransitionState transition)
	{
		if (!transition.m_bApplyEnemyConsequences
			|| transition.m_sNewOwnerFactionKey != m_Preset.m_sResistanceFactionKey
			|| transition.m_sPreviousOwnerFactionKey.IsEmpty()
			|| transition.m_sPreviousOwnerFactionKey == m_Preset.m_sResistanceFactionKey)
			return;

		int chance = 45;
		if (m_Balance && m_Balance.m_iCaptureCounterattackChancePercent >= 0)
			chance = m_Balance.m_iCaptureCounterattackChancePercent;
		chance += Math.Max(0, state.m_iWarLevel - 1) * 3;
		chance += Math.Max(0, zone.m_iPriority / 2);
		transition.m_iCounterattackChance = Math.Max(0, Math.Min(95, chance));
		transition.m_iCounterattackRoll = HST_DefaultCatalog.PositiveMod(
			string.Format("%1|%2|%3", state.m_iCampaignSeed, transition.m_sRequestId, zone.m_sZoneId).Hash(),
			100);
		transition.m_bCounterattackSelected = transition.m_iCounterattackRoll < transition.m_iCounterattackChance;
	}

	protected int ResolveCaptureAggression(HST_ZoneState zone)
	{
		int baseAggression = 10;
		if (m_Balance && m_Balance.m_iCaptureAggressionBase > 0)
			baseAggression = m_Balance.m_iCaptureAggressionBase;
		if (!zone)
			return baseAggression;
		return baseAggression + Math.Max(0, zone.m_iPriority / 5);
	}

	protected bool RequestMatches(HST_OwnershipTransitionState transition, HST_OwnershipTransitionRequest request)
	{
		if (!transition || !request)
			return false;
		if (transition.m_sRequestId != request.m_sRequestId
			|| transition.m_sZoneId != request.m_sZoneId
			|| transition.m_sCause != request.m_sCause
			|| transition.m_sSourceType != request.m_sSourceType)
			return false;
		if (transition.m_sSourceId != request.m_sSourceId
			|| transition.m_sActorIdentityId != request.m_sActorIdentityId
			|| transition.m_sExpectedOwnerFactionKey != request.m_sExpectedOwnerFactionKey
			|| transition.m_iExpectedRevision != request.m_iExpectedRevision)
			return false;
		if (transition.m_sNewOwnerFactionKey != request.m_sNewOwnerFactionKey
			|| transition.m_sReason != request.m_sReason
			|| transition.m_iSupportReward != request.m_iSupportReward)
			return false;
		if (transition.m_bApplyEnemyConsequences != request.m_bApplyEnemyConsequences
			|| transition.m_bReconcileSecurity != request.m_bReconcileSecurity
			|| transition.m_bCreateSecurity != request.m_bCreateSecurity
			|| transition.m_bNotify != request.m_bNotify)
			return false;
		return true;
	}

	protected bool ResumeStateMatches(HST_ZoneState zone, HST_OwnershipTransitionState transition)
	{
		if (!zone || !transition)
			return false;
		if (!transition.m_bOwnerApplied)
			return zone.m_sOwnerFactionKey == transition.m_sPreviousOwnerFactionKey
				&& Math.Max(1, zone.m_iOwnershipRevision) == transition.m_iExpectedRevision;
		return zone.m_sOwnerFactionKey == transition.m_sNewOwnerFactionKey
			&& zone.m_iOwnershipRevision == transition.m_iAppliedRevision
			&& transition.m_iAppliedRevision == transition.m_iExpectedRevision + 1;
	}

	protected HST_OwnershipTransitionRequest RequestFromTransition(HST_OwnershipTransitionState transition)
	{
		HST_OwnershipTransitionRequest request = new HST_OwnershipTransitionRequest();
		request.m_sRequestId = transition.m_sRequestId;
		request.m_sZoneId = transition.m_sZoneId;
		request.m_sCause = transition.m_sCause;
		request.m_sSourceType = transition.m_sSourceType;
		request.m_sSourceId = transition.m_sSourceId;
		request.m_sActorIdentityId = transition.m_sActorIdentityId;
		request.m_sExpectedOwnerFactionKey = transition.m_sExpectedOwnerFactionKey;
		request.m_iExpectedRevision = transition.m_iExpectedRevision;
		request.m_sNewOwnerFactionKey = transition.m_sNewOwnerFactionKey;
		request.m_sReason = transition.m_sReason;
		request.m_iSupportReward = transition.m_iSupportReward;
		request.m_bApplyEnemyConsequences = transition.m_bApplyEnemyConsequences;
		request.m_bReconcileSecurity = transition.m_bReconcileSecurity;
		request.m_bCreateSecurity = transition.m_bCreateSecurity;
		request.m_bNotify = transition.m_bNotify;
		return request;
	}

	protected bool EnsureAdmissionCapacity(HST_CampaignState state)
	{
		PruneCompletedHistory(state);
		return state.m_aOwnershipTransitions.Count() < MAX_TRANSITION_ROWS;
	}

	protected bool CanEnsureAdmissionCapacity(HST_CampaignState state)
	{
		if (!state)
			return false;
		if (state.m_aOwnershipTransitions.Count() < MAX_TRANSITION_ROWS)
			return true;
		foreach (HST_OwnershipTransitionState candidate : state.m_aOwnershipTransitions)
		{
			if (CanPruneTransition(state, candidate))
				return true;
		}
		return false;
	}

	protected void PruneCompletedHistory(HST_CampaignState state)
	{
		if (!state)
			return;
		while (state.m_aOwnershipTransitions.Count() >= MAX_TRANSITION_ROWS)
		{
			int pruneIndex = -1;
			int oldestSecond = int.MAX;
			for (int i; i < state.m_aOwnershipTransitions.Count(); i++)
			{
				HST_OwnershipTransitionState candidate = state.m_aOwnershipTransitions[i];
				if (!CanPruneTransition(state, candidate))
					continue;
				if (candidate.m_iCompletedAtSecond >= oldestSecond)
					continue;
				oldestSecond = candidate.m_iCompletedAtSecond;
				pruneIndex = i;
			}
			if (pruneIndex < 0)
				return;
			state.m_aOwnershipTransitions.Remove(pruneIndex);
		}
	}

	protected bool CanPruneTransition(HST_CampaignState state, HST_OwnershipTransitionState transition)
	{
		if (!state || !transition || !transition.m_bCompleted || transition.m_bQuarantined)
			return false;
		if (state.m_iElapsedSeconds < transition.m_iCompletedAtSecond + MIN_REPLAY_RETENTION_SECONDS)
			return false;
		HST_ZoneState zone = state.FindZone(transition.m_sZoneId);
		if (zone && zone.m_sLastOwnershipTransitionRequestId == transition.m_sRequestId)
			return false;
		// Exact counterattack settlement and replay use the immutable completed
		// ownership receipt even after a later capture replaces the zone's current
		// owner/backlink. Retain that receipt for as long as its durable order row
		// remains available for validation or quarantine diagnosis.
		if (transition.m_sSourceType == "enemy_counterattack")
		{
			foreach (HST_EnemyOrderState counterattack : state.m_aEnemyOrders)
			{
				if (!counterattack
					|| counterattack.m_eType
						!= HST_EEnemyOrderType.HST_ENEMY_ORDER_COUNTERATTACK)
					continue;
				bool exactOrQuarantined = counterattack.m_iOperationContractVersion
					== HST_EnemyCounterattackOperationService.EXACT_CONTRACT_VERSION;
				exactOrQuarantined = exactOrQuarantined
					|| counterattack.m_iOperationContractVersion
						== HST_EnemyCounterattackSaveValidationService.QUARANTINED_CONTRACT_VERSION;
				if (!exactOrQuarantined)
					continue;
				string requestId = "ownership_counterattack_" + counterattack.m_sOperationId;
				if (transition.m_sRequestId == requestId
					&& transition.m_sSourceId == counterattack.m_sOperationId
					&& transition.m_sZoneId == counterattack.m_sTargetZoneId
					&& transition.m_sNewOwnerFactionKey == counterattack.m_sFactionKey)
					return false;
			}
		}
		foreach (HST_OwnershipTransitionState child : state.m_aOwnershipTransitions)
		{
			if (child && child.m_sProjectionParentRequestId == transition.m_sRequestId
				&& !child.m_bDeferredPublicationReleased)
				return false;
		}
		if (!transition.m_sEnemyOrderId.IsEmpty())
		{
			HST_EnemyOrderState order = state.FindEnemyOrder(transition.m_sEnemyOrderId);
			if (order && order.m_eStatus != HST_EEnemyOrderStatus.HST_ENEMY_ORDER_RESOLVED)
				return false;
		}
		return true;
	}

	protected bool IsKnownCause(string cause)
	{
		return cause == "military_capture"
			|| cause == "mission_capture"
			|| cause == "political_support"
			|| cause == "admin"
			|| cause == "debug_seed"
			|| cause == "migration_repair";
	}

	protected string ValidateCausePolicy(HST_OwnershipTransitionRequest request)
	{
		if (!request)
			return "ownership request cause policy is unavailable";
		if (request.m_sCause == "admin")
		{
			if (request.m_bApplyEnemyConsequences)
				return "admin ownership policy cannot apply enemy consequences";
			if (request.m_iSupportReward != 0)
				return "admin ownership policy cannot apply capture support";
		}
		if (request.m_sCause == "debug_seed")
		{
			if (request.m_bApplyEnemyConsequences)
				return "debug ownership policy cannot apply enemy consequences";
			if (request.m_bNotify)
				return "debug ownership policy cannot publish gameplay notification";
			if (request.m_iSupportReward != 0)
				return "debug ownership policy cannot apply capture support";
		}
		if (request.m_sCause == "migration_repair")
		{
			if (request.m_bApplyEnemyConsequences || request.m_bNotify)
				return "migration ownership policy cannot apply retaliation or notification";
			if (request.m_bReconcileSecurity || request.m_bCreateSecurity)
				return "migration ownership policy cannot rewrite security authority";
			if (request.m_iSupportReward != 0)
				return "migration ownership policy cannot apply capture support";
		}
		if (request.m_sCause == "political_support" && request.m_iSupportReward != 0)
			return "political ownership policy cannot recursively apply capture support";
		return "";
	}

	protected HST_OwnershipTransitionResult NeedsRetry(
		HST_OwnershipTransitionState transition,
		HST_OwnershipTransitionResult result,
		string failureReason)
	{
		transition.m_sFailureReason = failureReason;
		transition.m_sStatus = "applying";
		result.m_bAccepted = true;
		result.m_bNeedsRetry = true;
		result.m_bStateChanged = true;
		result.m_sFailureReason = failureReason;
		m_bStateChangedSinceConsume = true;
		SchedulePartialPersistence();
		return result;
	}

	protected HST_OwnershipTransitionResult Reject(HST_OwnershipTransitionResult result, string failureReason)
	{
		if (!result)
		{
			HST_OwnershipTransitionResult fallback = new HST_OwnershipTransitionResult();
			fallback.m_bAccepted = false;
			fallback.m_sFailureReason = failureReason;
			return fallback;
		}
		result.m_bAccepted = false;
		result.m_sFailureReason = failureReason;
		return result;
	}

	protected HST_OwnershipTransitionResult QuarantineResult(
		HST_CampaignState state,
		HST_OwnershipTransitionState transition,
		HST_OwnershipTransitionResult result,
		string failureReason)
	{
		QuarantineTransition(state, transition, failureReason);
		result.m_bAccepted = false;
		result.m_bStateChanged = true;
		result.m_sFailureReason = failureReason;
		m_bStateChangedSinceConsume = true;
		SchedulePartialPersistence();
		return result;
	}

	protected void SchedulePartialPersistence()
	{
		// This deliberately does not set m_bPersistenceRequested: that checklist
		// bit means the completed transition checkpoint was scheduled.
		if (m_Persistence)
			m_Persistence.MarkMajorChange();
	}

	protected void QuarantineTransition(
		HST_CampaignState state,
		HST_OwnershipTransitionState transition,
		string failureReason)
	{
		if (!transition)
			return;
		transition.m_iContractVersion = QUARANTINED_CONTRACT_VERSION;
		transition.m_sStatus = "quarantined";
		transition.m_bQuarantined = true;
		transition.m_sFailureReason = failureReason;
		HST_ZoneState zone;
		if (state)
			zone = state.FindZone(transition.m_sZoneId);
		if (zone)
		{
			zone.m_iOwnershipContractVersion = QUARANTINED_CONTRACT_VERSION;
			zone.m_sOwnershipAuthorityFailure = failureReason;
			zone.m_sActiveOwnershipTransitionRequestId = transition.m_sRequestId;
		}
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return dx * dx + dz * dz;
	}
}
