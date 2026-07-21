#ifdef ENABLE_DIAG
// Holds the bounded physical-response foldback drive outside the campaign
// coordinator source file so Enforce can compile the proof without exceeding
// the coordinator translation unit's practical complexity ceiling.
class HST_CampaignDebugPhysicalResponseFoldbackDriveResult
{
	static const int PLAYER_RESTORE_OWNER_ACK_TIMEOUT_MS = 5000;
	static const float PLAYER_RESTORE_TRANSFORM_TOLERANCE_METERS = 0.001;

	ref HST_ActiveGroupState m_GroupAfterDrive;
	ref HST_CampaignDebugCaseResult m_Case;
	IEntity m_PlayerEntityBeforeFoldback;
	RplId m_PlayerReplicationIdBeforeFoldback = RplId.Invalid();
	vector m_aPlayerTransformBeforeFoldback[4];
	vector m_vPlayerPositionBeforeFoldback;
	int m_iPlayerId = -1;
	bool m_bPlayerPositionCaptured;
	bool m_bPlayerSessionCaptured;
	bool m_bPlayerRestoreOwned;
	bool m_bPlayerRestoreApplied;
	bool m_bPlayerRestoreOwnerAckPending;
	bool m_bPlayerRestoreOwnerAckExact;
	bool m_bPlayerRestoreDisconnectObserved;
	bool m_bPlayerRestoreSessionLost;
	bool m_bNearTeleport;
	bool m_bInsideBubbleBeforeLeave;
	bool m_bFarTeleport;
	bool m_bOutsideBubbleAfterLeave;
	bool m_bDeactivationTickChanged;
	bool m_bZoneDeactivated;
	bool m_bPhysicalFoldChanged;
	bool m_bRuntimeGroupAfterFold;
	bool m_bRuntimeVehicleAfterFold;
	bool m_bFoldTickChanged;
	bool m_bOrderSyncChanged;
	bool m_bPlayerRestored;
	int m_iPlayerRestoreAppliedObservationToken = -1;
	int m_iPlayerRestoreStableSampleObservationToken = -1;
	int m_iPlayerRestoreLastAttemptObservationToken = -1;
	int m_iPlayerRestoreApplySequence = -1;
	int m_iPlayerRestoreOwnerAckSequence = -1;
	int m_iPlayerRestoreOwnerAckObservedToken = -1;
	int m_iPlayerRestoreOwnerDispatchTick = -1;
	float m_fPlayerRestoreMaximumTransformDelta = -1.0;
	float m_fPlayerRestoreOwnerMaximumTransformDelta = -1.0;
	string m_sGroupStatusBeforeFold;
	bool m_bVehicleRuntimeBeforeFold;
	string m_sVehicleRuntimeEvidenceBeforeFold = "missing";
	string m_sPlayerRestoreRequestId;
	string m_sPlayerRestoreOwnerEvidence = "not received";
	string m_sPlayerRestoreEvidence = "not attempted";
	string m_sEvidence = "not attempted";

	void CaptureBeforeDrive(
		HST_ActiveGroupState group,
		HST_PhysicalWarService physicalWar)
	{
		if (!group)
			return;

		m_sGroupStatusBeforeFold = group.m_sRuntimeStatus;
		if (!physicalWar)
			return;

		m_bVehicleRuntimeBeforeFold
			= physicalWar.CampaignDebugHasRuntimeVehicleEntity(group.m_sGroupId);
		m_sVehicleRuntimeEvidenceBeforeFold
			= physicalWar.CampaignDebugBuildActiveGroupRuntimeVisualEvidence(
				group.m_sGroupId);
	}

	bool IsBubbleTransitionExact()
	{
		return m_bNearTeleport
			&& m_bInsideBubbleBeforeLeave
			&& m_bFarTeleport
			&& m_bOutsideBubbleAfterLeave;
	}

	bool IsPhysicalFoldExact()
	{
		return m_bDeactivationTickChanged
			&& m_bZoneDeactivated
			&& m_bPhysicalFoldChanged
			&& !m_bRuntimeGroupAfterFold
			&& !m_bRuntimeVehicleAfterFold;
	}

	void Drive(
		HST_CampaignCoordinatorComponent coordinator,
		int playerId,
		IEntity foldbackPlayer,
		HST_CampaignState state,
		HST_BalanceConfig balance,
		HST_CampaignPreset preset,
		HST_PhysicalWarService physicalWar,
		HST_ZoneState targetZone,
		HST_GeneratedRouteState targetRoute,
		HST_SupportRequestState request,
		HST_ActiveGroupState group)
	{
		m_GroupAfterDrive = group;
		m_iPlayerId = playerId;
		if (IsLivingEntity(foldbackPlayer) && playerId > 0)
		{
			BaseRplComponent replication = BaseRplComponent.Cast(
				foldbackPlayer.FindComponent(BaseRplComponent));
			if (replication && replication.Id() != RplId.Invalid())
			{
				m_PlayerEntityBeforeFoldback = foldbackPlayer;
				m_PlayerReplicationIdBeforeFoldback = replication.Id();
				m_vPlayerPositionBeforeFoldback = foldbackPlayer.GetOrigin();
				foldbackPlayer.GetTransform(m_aPlayerTransformBeforeFoldback);
				m_bPlayerPositionCaptured = true;
				m_bPlayerSessionCaptured
					= coordinator
						.CampaignDebugValidatePhysicalResponseFoldbackPlayer(
							m_iPlayerId,
							m_PlayerEntityBeforeFoldback,
							m_PlayerReplicationIdBeforeFoldback,
							m_sPlayerRestoreEvidence);
			}
		}
		if (!m_bPlayerSessionCaptured)
		{
			m_sEvidence = "exact on-foot player session/full transform capture rejected | "
				+ m_sPlayerRestoreEvidence;
			return;
		}

		string foldGroupId = group.m_sGroupId;
		vector nearPosition = group.m_vPosition + "12 0 12";
		if (IsZeroPosition(group.m_vPosition))
			nearPosition = targetZone.m_vPosition + "12 0 12";
		// Own restoration before the first teleport request. The owner RPC can be
		// queued even when immediate server-side confirmation returns false.
		m_bPlayerRestoreOwned = true;
		m_bNearTeleport
			= coordinator.CampaignDebugTeleportPhysicalResponseFoldbackPlayer(
				nearPosition,
				"enemy physical response foldback near");
		m_bInsideBubbleBeforeLeave
			= HST_WorldPositionService.IsPositionInsidePlayerEventBubble(
				group.m_vPosition);

		vector farPosition = ResolveFarPosition(state, group.m_vPosition);
		m_bFarTeleport
			= coordinator.CampaignDebugTeleportPhysicalResponseFoldbackPlayer(
				farPosition,
				"enemy physical response foldback leave");
		m_bOutsideBubbleAfterLeave
			= !HST_WorldPositionService.IsPositionInsidePlayerEventBubble(
				group.m_vPosition);
		HST_CampaignState deactivationState = new HST_CampaignState();
		deactivationState.m_ePhase = state.m_ePhase;
		deactivationState.m_iElapsedSeconds = state.m_iElapsedSeconds;
		deactivationState.m_vHQPosition = state.m_vHQPosition;
		deactivationState.m_aZones.Insert(targetZone);
		deactivationState.m_aGeneratedRoutes.Insert(targetRoute);
		deactivationState.m_aSupportRequests.Insert(request);
		deactivationState.m_aActiveGroups.Insert(group);
		HST_EnemyDirectorService noDeactivationEnemyDirector = null;
		HST_ZoneCompositionService noDeactivationCompositions = null;
		m_bDeactivationTickChanged = physicalWar.UpdateZoneActivation(
			deactivationState,
			balance,
			preset,
			noDeactivationEnemyDirector,
			noDeactivationCompositions);
		m_bZoneDeactivated = !targetZone.m_bActive;
		m_GroupAfterDrive = state.FindActiveGroup(foldGroupId);
		if (m_GroupAfterDrive && m_bOutsideBubbleAfterLeave
			&& m_bZoneDeactivated)
		{
			m_bPhysicalFoldChanged = physicalWar.FoldActiveSupportGroup(
				state,
				foldGroupId);
		}
		m_GroupAfterDrive = state.FindActiveGroup(foldGroupId);
		m_bRuntimeGroupAfterFold
			= physicalWar.CampaignDebugHasRuntimeGroupEntity(foldGroupId);
		m_bRuntimeVehicleAfterFold
			= physicalWar.CampaignDebugHasRuntimeVehicleEntity(foldGroupId);
		m_sEvidence = string.Format(
			"near teleport %1 inside %2 | far teleport %3 outside %4 | deactivation tick %5 zone inactive %6 | physical fold %7 | runtime group/vehicle %8/%9",
			m_bNearTeleport,
			m_bInsideBubbleBeforeLeave,
			m_bFarTeleport,
			m_bOutsideBubbleAfterLeave,
			m_bDeactivationTickChanged,
			m_bZoneDeactivated,
			m_bPhysicalFoldChanged,
			m_bRuntimeGroupAfterFold,
			m_bRuntimeVehicleAfterFold);
	}

	bool RestorePlayer(
		HST_CampaignCoordinatorComponent coordinator,
		int observationToken)
	{
		if (m_bPlayerRestored || m_bPlayerRestoreSessionLost)
			return true;
		string sessionLossEvidence
			= "authoritative player-disconnected event observed";
		bool sessionIrrecoverable = m_bPlayerRestoreDisconnectObserved;
		if (!sessionIrrecoverable)
		{
			sessionIrrecoverable = coordinator
				.CampaignDebugIsPhysicalResponseFoldbackPlayerSessionIrrecoverable(
					m_iPlayerId,
					m_PlayerEntityBeforeFoldback,
					m_PlayerReplicationIdBeforeFoldback,
					sessionLossEvidence);
		}
		if (m_bPlayerSessionCaptured && m_bPlayerRestoreOwned
			&& sessionIrrecoverable)
		{
			m_bPlayerRestoreSessionLost = true;
			m_bPlayerRestoreOwned = false;
			m_bPlayerRestoreApplied = false;
			m_bPlayerRestoreOwnerAckPending = false;
			m_iPlayerRestoreOwnerDispatchTick = -1;
			m_sPlayerRestoreEvidence
				= "terminal exact player session loss; nonexistent/replaced session ownership relinquished | "
					+ sessionLossEvidence;
			return true;
		}
		if (observationToken < 0)
		{
			m_sPlayerRestoreEvidence
				= "player restore observation token is unavailable";
			return false;
		}
		if (m_iPlayerRestoreLastAttemptObservationToken == observationToken)
			return false;
		m_iPlayerRestoreLastAttemptObservationToken = observationToken;
		if (!m_bPlayerSessionCaptured || !m_bPlayerPositionCaptured
			|| !m_bPlayerRestoreOwned)
		{
			m_sPlayerRestoreEvidence
				= "exact player session/full transform restore ownership was not captured";
			return false;
		}

		if (!m_bPlayerRestoreApplied)
		{
			m_iPlayerRestoreApplySequence
				= coordinator
					.CampaignDebugClaimPhysicalResponseFoldbackPlayerRestoreRequest(
						m_sPlayerRestoreRequestId);
			if (m_iPlayerRestoreApplySequence <= 0
				|| m_sPlayerRestoreRequestId.IsEmpty())
			{
				m_sPlayerRestoreEvidence
					= "player restore request identity is unavailable";
				return false;
			}
			m_iPlayerRestoreOwnerAckSequence = -1;
			m_iPlayerRestoreOwnerAckObservedToken = -1;
			m_bPlayerRestoreOwnerAckPending = true;
			m_bPlayerRestoreOwnerAckExact = false;
			m_iPlayerRestoreOwnerDispatchTick = System.GetTickCount();
			m_fPlayerRestoreOwnerMaximumTransformDelta = -1.0;
			m_sPlayerRestoreOwnerEvidence = "awaiting owner acknowledgement";
			m_bPlayerRestoreApplied
				= coordinator.CampaignDebugApplyPhysicalResponseFoldbackPlayerRestore(
					m_iPlayerId,
					m_PlayerEntityBeforeFoldback,
					m_PlayerReplicationIdBeforeFoldback,
					m_sPlayerRestoreRequestId,
					m_iPlayerRestoreApplySequence,
					m_aPlayerTransformBeforeFoldback,
					m_sPlayerRestoreEvidence);
			if (!m_bPlayerRestoreApplied)
			{
				string applyFailureEvidence = m_sPlayerRestoreEvidence;
				ArmPlayerRestoreReapply(
					"server restore apply/owner dispatch failed; reapply armed | "
						+ applyFailureEvidence);
				return false;
			}
			m_iPlayerRestoreAppliedObservationToken = observationToken;
		}

		if (m_bPlayerRestoreOwnerAckPending)
		{
			int ownerAcknowledgementWaitMs
				= System.GetTickCount(m_iPlayerRestoreOwnerDispatchTick);
			if (ownerAcknowledgementWaitMs
				>= PLAYER_RESTORE_OWNER_ACK_TIMEOUT_MS)
			{
				string timedOutRequestId = m_sPlayerRestoreRequestId;
				int timedOutApplySequence = m_iPlayerRestoreApplySequence;
				ArmPlayerRestoreReapply(string.Format(
					"owner acknowledgement timed out after %1ms for request %2 sequence %3; fresh request/sequence reapply armed",
					ownerAcknowledgementWaitMs,
					timedOutRequestId,
					timedOutApplySequence));
				return false;
			}
			m_sPlayerRestoreEvidence = string.Format(
				"request %1 sequence %2 dispatched at token %3 | awaiting exact owner acknowledgement for %4/%5ms",
				m_sPlayerRestoreRequestId,
				m_iPlayerRestoreApplySequence,
				m_iPlayerRestoreAppliedObservationToken,
				ownerAcknowledgementWaitMs,
				PLAYER_RESTORE_OWNER_ACK_TIMEOUT_MS);
			return false;
		}
		if (m_iPlayerRestoreOwnerAckSequence
			!= m_iPlayerRestoreApplySequence)
		{
			ArmPlayerRestoreReapply(
				"owner acknowledgement sequence mismatch; fresh request/sequence reapply armed");
			return false;
		}
		if (!m_bPlayerRestoreOwnerAckExact)
		{
			string ownerFailureEvidence = m_sPlayerRestoreOwnerEvidence;
			ArmPlayerRestoreReapply(
				"owner acknowledgement rejected transform; reapply armed | "
					+ ownerFailureEvidence);
			return false;
		}
		if (m_iPlayerRestoreOwnerAckObservedToken < 0)
		{
			m_iPlayerRestoreOwnerAckObservedToken = observationToken;
			m_sPlayerRestoreEvidence = string.Format(
				"request %1 sequence %2 owner acknowledgement exact at token %3 | owner maximum delta %4m | awaiting a distinct later server sample | %5",
				m_sPlayerRestoreRequestId,
				m_iPlayerRestoreApplySequence,
				m_iPlayerRestoreOwnerAckObservedToken,
				Math.Round(m_fPlayerRestoreOwnerMaximumTransformDelta),
				m_sPlayerRestoreOwnerEvidence);
			return false;
		}

		if (observationToken <= m_iPlayerRestoreOwnerAckObservedToken
			|| observationToken
				== m_iPlayerRestoreStableSampleObservationToken)
			return false;
		m_iPlayerRestoreStableSampleObservationToken = observationToken;
		m_bPlayerRestored
			= coordinator.CampaignDebugSamplePhysicalResponseFoldbackPlayerRestore(
				m_iPlayerId,
				m_PlayerEntityBeforeFoldback,
				m_PlayerReplicationIdBeforeFoldback,
				m_aPlayerTransformBeforeFoldback,
				m_fPlayerRestoreMaximumTransformDelta,
				m_sPlayerRestoreEvidence);
		if (m_bPlayerRestored)
			return true;

		string sampleFailureEvidence = m_sPlayerRestoreEvidence;
		ArmPlayerRestoreReapply(
			"later player sample failed; corrective full-transform reapply armed | "
				+ sampleFailureEvidence);
		return false;
	}

	void ObservePlayerDisconnected(int playerId)
	{
		if (playerId == m_iPlayerId && m_bPlayerSessionCaptured
			&& m_bPlayerRestoreOwned && !m_bPlayerRestored)
			m_bPlayerRestoreDisconnectObserved = true;
	}

	bool AcceptPlayerRestoreOwnerAcknowledgement(
		int playerId,
		string restoreRequestId,
		int applySequence,
		RplId actualPlayerReplicationId,
		bool ownerTransformExact,
		float ownerMaximumTransformDelta,
		string evidence)
	{
		if (m_bPlayerRestored || m_bPlayerRestoreSessionLost
			|| !m_bPlayerRestoreOwned || !m_bPlayerRestoreOwnerAckPending
			|| playerId != m_iPlayerId
			|| restoreRequestId != m_sPlayerRestoreRequestId
			|| applySequence != m_iPlayerRestoreApplySequence)
			return false;
		if (m_iPlayerRestoreOwnerDispatchTick < 0
			|| System.GetTickCount(m_iPlayerRestoreOwnerDispatchTick)
				>= PLAYER_RESTORE_OWNER_ACK_TIMEOUT_MS)
			return false;

		bool replicationExact = actualPlayerReplicationId
			== m_PlayerReplicationIdBeforeFoldback;
		bool ownerTransformMetricExact = ownerMaximumTransformDelta >= 0.0
			&& ownerMaximumTransformDelta
				<= PLAYER_RESTORE_TRANSFORM_TOLERANCE_METERS;
		m_iPlayerRestoreOwnerAckSequence = applySequence;
		m_bPlayerRestoreOwnerAckPending = false;
		m_iPlayerRestoreOwnerDispatchTick = -1;
		m_bPlayerRestoreOwnerAckExact
			= ownerTransformExact && ownerTransformMetricExact
				&& replicationExact;
		m_fPlayerRestoreOwnerMaximumTransformDelta
			= ownerMaximumTransformDelta;
		m_sPlayerRestoreOwnerEvidence = string.Format(
			"reported replication %1/%2 exact %3 | owner transform exact/metric exact/delta %4/%5/%6 | %7",
			actualPlayerReplicationId,
			m_PlayerReplicationIdBeforeFoldback,
			replicationExact,
			ownerTransformExact,
			ownerTransformMetricExact,
			ownerMaximumTransformDelta,
			evidence);
		return true;
	}

	bool NeedsRetainedPlayerRestore()
	{
		return m_bPlayerSessionCaptured
			&& m_bPlayerRestoreOwned
			&& !m_bPlayerRestoreSessionLost
			&& !m_bPlayerRestored;
	}

	protected void ArmPlayerRestoreReapply(string evidence)
	{
		m_bPlayerRestoreApplied = false;
		m_bPlayerRestoreOwnerAckPending = false;
		m_bPlayerRestoreOwnerAckExact = false;
		m_iPlayerRestoreApplySequence = -1;
		m_iPlayerRestoreOwnerAckSequence = -1;
		m_iPlayerRestoreOwnerAckObservedToken = -1;
		m_iPlayerRestoreAppliedObservationToken = -1;
		m_iPlayerRestoreStableSampleObservationToken = -1;
		m_iPlayerRestoreOwnerDispatchTick = -1;
		m_fPlayerRestoreOwnerMaximumTransformDelta = -1.0;
		m_sPlayerRestoreRequestId = "";
		m_sPlayerRestoreOwnerEvidence = "not received";
		m_sPlayerRestoreEvidence = evidence;
	}

	protected vector ResolveFarPosition(
		HST_CampaignState state,
		vector groupPosition)
	{
		float minimumDistance
			= HST_WorldPositionService.GetPlayerEventBubbleRadiusMeters() + 500.0;
		float minimumDistanceSq = minimumDistance * minimumDistance;
		if (state && !IsZeroPosition(state.m_vHQPosition)
			&& DistanceSq2D(groupPosition, state.m_vHQPosition)
				>= minimumDistanceSq)
			return state.m_vHQPosition + "8 0 8";

		HST_ZoneState farthestZone;
		float farthestDistanceSq;
		if (state)
		{
			foreach (HST_ZoneState zone : state.m_aZones)
			{
				if (!zone || IsZeroPosition(zone.m_vPosition))
					continue;
				float distanceSq = DistanceSq2D(
					groupPosition,
					zone.m_vPosition);
				if (farthestZone && distanceSq <= farthestDistanceSq)
					continue;
				farthestZone = zone;
				farthestDistanceSq = distanceSq;
			}
		}
		if (farthestZone && farthestDistanceSq >= minimumDistanceSq)
			return farthestZone.m_vPosition + "8 0 8";

		return groupPosition + "5000 0 5000";
	}

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity || entity.IsDeleted())
			return false;
		CharacterControllerComponent controller
			= CharacterControllerComponent.Cast(
				entity.FindComponent(CharacterControllerComponent));
		return controller && !controller.IsDead();
	}

	protected bool IsZeroPosition(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return dx * dx + dz * dz;
	}
}
#endif
