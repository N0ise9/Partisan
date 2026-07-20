// Holds the bounded physical-response foldback drive outside the campaign
// coordinator source file so Enforce can compile the proof without exceeding
// the coordinator translation unit's practical complexity ceiling.
class HST_CampaignDebugPhysicalResponseFoldbackDriveResult
{
	ref HST_ActiveGroupState m_GroupAfterDrive;
	vector m_vPlayerPositionBeforeFoldback;
	bool m_bPlayerPositionCaptured;
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
	string m_sGroupStatusBeforeFold;
	bool m_bVehicleRuntimeBeforeFold;
	string m_sVehicleRuntimeEvidenceBeforeFold = "missing";
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
		if (IsLivingEntity(foldbackPlayer))
		{
			m_vPlayerPositionBeforeFoldback = foldbackPlayer.GetOrigin();
			m_bPlayerPositionCaptured
				= !IsZeroPosition(m_vPlayerPositionBeforeFoldback);
		}

		string foldGroupId = group.m_sGroupId;
		vector nearPosition = group.m_vPosition + "12 0 12";
		if (IsZeroPosition(group.m_vPosition))
			nearPosition = targetZone.m_vPosition + "12 0 12";
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

	void RestorePlayer(HST_CampaignCoordinatorComponent coordinator)
	{
		if (!m_bPlayerPositionCaptured)
			return;

		coordinator.CampaignDebugTeleportPhysicalResponseFoldbackPlayer(
			m_vPlayerPositionBeforeFoldback,
			"enemy physical response foldback restore");
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
