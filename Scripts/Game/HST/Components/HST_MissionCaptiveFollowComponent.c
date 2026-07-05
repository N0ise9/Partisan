[ComponentEditorProps(category: "h-istasi", description: "Keeps freed mission captives following their assigned player through AI movement")]
class HST_MissionCaptiveFollowComponentClass : ScriptComponentClass
{
}

class HST_MissionCaptiveFollowComponent : ScriptComponent
{
	static const string CAPTIVE_AI_GROUP_PREFAB = "{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et";
	static const string CAPTIVE_FOLLOW_WAYPOINT_PREFAB = "{A0509D3C4DD4475E}Prefabs/AI/Waypoints/AIWaypoint_Follow.et";
	static const string CAPTIVE_MOVE_WAYPOINT_PREFAB = "{FBA8DC8FDA0E770D}Prefabs/AI/Waypoints/AIWaypoint_Patrol_Hierarchy.et";
	static const float STOP_DISTANCE_METERS = 2.25;
	static const float WAYPOINT_REFRESH_DISTANCE_METERS = 14.0;
	static const float WAYPOINT_REACHED_REFRESH_DISTANCE_METERS = 4.0;
	static const float TARGET_MOVING_STEP_METERS = 0.35;
	static const float TARGET_LEAD_MULTIPLIER = 4.0;
	static const float MAX_TARGET_LEAD_METERS = 14.0;
	static const float SPRINT_CATCHUP_DISTANCE_METERS = 8.0;
	static const float STUCK_RECOVERY_DISTANCE_METERS = 6.0;
	static const float STUCK_RECOVERY_MIN_MOVE_METERS = 0.75;
	static const float DIRECT_CATCHUP_MIN_DISTANCE_METERS = 12.0;
	static const float DIRECT_CATCHUP_STEP_METERS = 8.0;
	static const int STUCK_RECOVERY_TICKS = 6;
	static const int FOLLOW_UPDATE_MS = 500;

	protected IEntity m_FollowTarget;
	protected IEntity m_GroupEntity;
	protected IEntity m_WaypointEntity;
	protected vector m_WaypointPosition;
	protected vector m_vLastTargetPosition;
	protected vector m_vLastOwnerPosition;
	protected bool m_bFollowing;
	protected bool m_bAddedToGroup;
	protected bool m_bHasLastTargetPosition;
	protected bool m_bHasLastOwnerPosition;
	protected bool m_bLoggedGroupCreated;
	protected bool m_bLoggedGroupFailed;
	protected bool m_bLoggedMissingMovement;
	protected bool m_bLoggedRequestFailed;
	protected bool m_bLoggedWaypointFallback;
	protected bool m_bLoggedWaypointFailed;
	protected bool m_bLoggedStuckRecovery;
	protected bool m_bLoggedDirectCatchup;
	protected bool m_bDirectFollowUnavailable;
	protected int m_iNoProgressTicks;

	bool IsFollowing()
	{
		return m_bFollowing && m_FollowTarget != null;
	}

	void StartFollowing(IEntity target)
	{
		if (!target)
			return;

		IEntity owner = GetOwner();
		if (!owner)
			return;

		if (m_bFollowing && m_FollowTarget == target)
		{
			FollowTick();
			return;
		}

		m_FollowTarget = target;
		m_bFollowing = true;
		m_bHasLastTargetPosition = false;
		m_bHasLastOwnerPosition = false;
		m_vLastTargetPosition = "0 0 0";
		m_vLastOwnerPosition = "0 0 0";
		m_iNoProgressTicks = 0;
		m_bLoggedGroupCreated = false;
		m_bLoggedGroupFailed = false;
		m_bLoggedMissingMovement = false;
		m_bLoggedRequestFailed = false;
		m_bLoggedWaypointFallback = false;
		m_bLoggedWaypointFailed = false;
		m_bLoggedStuckRecovery = false;
		m_bLoggedDirectCatchup = false;
		m_bDirectFollowUnavailable = false;

		FollowTick();
		ScriptCallQueue queue = GetGame().GetCallqueue();
		if (queue)
		{
			queue.Remove(FollowTick);
			queue.CallLater(FollowTick, FOLLOW_UPDATE_MS, true);
		}
	}

	void StopFollowing()
	{
		ClearFollowWaypoint(ResolveParentAIGroup(GetOwner()));
		m_bFollowing = false;
		m_FollowTarget = null;

		ScriptCallQueue queue = GetGame().GetCallqueue();
		if (queue)
			queue.Remove(FollowTick);
	}

	override void OnDelete(IEntity owner)
	{
		StopFollowing();
		super.OnDelete(owner);
	}

	protected void FollowTick()
	{
		IEntity owner = GetOwner();
		if (!owner || !m_bFollowing || !m_FollowTarget)
		{
			StopFollowing();
			return;
		}

		if (!Replication.IsServer())
			return;

		vector ownerPosition = owner.GetOrigin();
		vector targetPosition = m_FollowTarget.GetOrigin();
		float targetStepDistance = ResolveTargetStepDistance(targetPosition);
		vector followPosition = BuildResponsiveFollowPosition(targetPosition, targetStepDistance);
		bool forceWaypointRefresh = UpdateFollowProgressState(ownerPosition, targetPosition);
		RememberTargetPosition(targetPosition);

		if (DistanceSq2D(ownerPosition, targetPosition) <= STOP_DISTANCE_METERS * STOP_DISTANCE_METERS && targetStepDistance < TARGET_MOVING_STEP_METERS)
			return;

		AIGroup group;
		AIBaseMovementComponent movement = ResolveAIMovement(owner, group);
		if (!movement)
		{
			if (!m_bLoggedMissingMovement)
			{
				Print(string.Format("h-istasi mission captive follow | no AI movement component for %1", owner.GetName()), LogLevel.WARNING);
				m_bLoggedMissingMovement = true;
			}
			return;
		}

		EMovementType movementType = ResolveFollowMovementType(DistanceSq2D(ownerPosition, targetPosition));
		TryForceFollowSpeed(owner, movementType);
		if (!forceWaypointRefresh && !m_bDirectFollowUnavailable && movement.RequestFollowPathOfEntity(m_FollowTarget))
			return;
		if (forceWaypointRefresh && !m_bLoggedStuckRecovery)
		{
			Print(string.Format("h-istasi mission captive follow | direct follow stalled, refreshing waypoint | owner %1 | target %2", owner.GetOrigin(), m_FollowTarget.GetOrigin()), LogLevel.WARNING);
			m_bLoggedStuckRecovery = true;
		}

		if (!m_bDirectFollowUnavailable)
		{
			if (!m_bLoggedRequestFailed)
			{
				Print(string.Format("h-istasi mission captive follow | RequestFollowPathOfEntity failed for %1 | group %2 | owner %3 | target %4", owner.GetName(), HasParentAIGroup(owner), owner.GetOrigin(), m_FollowTarget.GetOrigin()), LogLevel.WARNING);
				m_bLoggedRequestFailed = true;
			}
			m_bDirectFollowUnavailable = true;
		}

		IssueFollowWaypoint(group, ownerPosition, followPosition, movementType, m_FollowTarget, forceWaypointRefresh);
		if (forceWaypointRefresh)
			TryApplyDirectCatchup(owner, ownerPosition, followPosition);
	}

	protected bool TryApplyDirectCatchup(IEntity owner, vector ownerPosition, vector followPosition)
	{
		if (!owner || IsZeroVector(followPosition))
			return false;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(owner.FindComponent(SCR_CompartmentAccessComponent));
		if (access && access.IsInCompartment())
			return false;

		float distanceSq = DistanceSq2D(ownerPosition, followPosition);
		if (distanceSq <= DIRECT_CATCHUP_MIN_DISTANCE_METERS * DIRECT_CATCHUP_MIN_DISTANCE_METERS)
			return false;

		float distance = Math.Sqrt(distanceSq);
		float stepDistance = distance - STOP_DISTANCE_METERS;
		if (stepDistance > DIRECT_CATCHUP_STEP_METERS)
			stepDistance = DIRECT_CATCHUP_STEP_METERS;
		if (stepDistance <= 0.1)
			return false;

		float dx = followPosition[0] - ownerPosition[0];
		float dz = followPosition[2] - ownerPosition[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length <= 0.01)
			return false;

		vector catchupPosition = ownerPosition;
		catchupPosition[0] = catchupPosition[0] + (dx / length) * stepDistance;
		catchupPosition[2] = catchupPosition[2] + (dz / length) * stepDistance;
		catchupPosition = HST_WorldPositionService.ResolveSafeGroundPosition(catchupPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 1.0);
		HST_WorldPositionService.ApplyUprightEntityTransform(owner, catchupPosition, owner.GetYawPitchRoll());

		Physics physics = owner.GetPhysics();
		if (physics)
		{
			physics.SetVelocity(vector.Zero);
			physics.SetAngularVelocity(vector.Zero);
		}

		m_vLastOwnerPosition = catchupPosition;
		m_iNoProgressTicks = 0;
		if (!m_bLoggedDirectCatchup)
		{
			Print(string.Format("h-istasi mission captive follow | direct catch-up after stalled follow | owner %1 -> %2 | target %3 | distance %4m", ownerPosition, catchupPosition, followPosition, Math.Round(distance)), LogLevel.WARNING);
			m_bLoggedDirectCatchup = true;
		}

		return true;
	}

	protected AIBaseMovementComponent ResolveAIMovement(IEntity owner, out AIGroup group)
	{
		group = null;
		AIControlComponent control = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!control)
			return null;

		control.ActivateAI();
		AIAgent agent = control.GetControlAIAgent();
		if (!agent)
			return null;

		agent.ActivateAI();
		group = EnsureCaptiveAIGroup(owner, agent);
		return agent.GetMovementComponent();
	}

	protected bool IssueFollowWaypoint(AIGroup group, vector ownerPosition, vector targetPosition, EMovementType movementType, IEntity targetEntity, bool forceRefresh = false)
	{
		if (!group)
			return false;
		if (IsZeroVector(targetPosition))
			return false;

		vector waypointPosition = HST_WorldPositionService.ResolveSafeGroundPosition(targetPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		if (m_WaypointEntity)
		{
			SCR_EntityWaypoint entityWaypoint = SCR_EntityWaypoint.Cast(m_WaypointEntity);
			if (entityWaypoint)
			{
				bool waypointStale = DistanceSq2D(m_WaypointPosition, waypointPosition) > WAYPOINT_REFRESH_DISTANCE_METERS * WAYPOINT_REFRESH_DISTANCE_METERS;
				if (targetEntity && entityWaypoint.GetEntity() == targetEntity && !forceRefresh && !waypointStale)
				{
					ApplyGroupFollowSpeed(group, movementType);
					ApplyGroupFollowFormation(group);
					return true;
				}
			}
			else
			{
				bool waypointReached = DistanceSq2D(ownerPosition, m_WaypointPosition) <= WAYPOINT_REACHED_REFRESH_DISTANCE_METERS * WAYPOINT_REACHED_REFRESH_DISTANCE_METERS;
				bool waypointStale = DistanceSq2D(m_WaypointPosition, waypointPosition) > WAYPOINT_REFRESH_DISTANCE_METERS * WAYPOINT_REFRESH_DISTANCE_METERS;
				if (!forceRefresh && !waypointReached && !waypointStale)
				{
					ApplyGroupFollowSpeed(group, movementType);
					ApplyGroupFollowFormation(group);
					return true;
				}
			}
		}

		ClearFollowWaypoint(group);
		if (targetEntity && !forceRefresh && IssueEntityFollowWaypoint(group, targetEntity, waypointPosition, movementType))
			return true;

		return IssueStaticMoveWaypoint(group, waypointPosition, movementType);
	}

	protected bool IssueEntityFollowWaypoint(AIGroup group, IEntity targetEntity, vector waypointPosition, EMovementType movementType)
	{
		if (!group || !targetEntity)
			return false;

		GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(CAPTIVE_FOLLOW_WAYPOINT_PREFAB, waypointPosition, "0 0 0");
		SCR_EntityWaypoint waypoint = SCR_EntityWaypoint.Cast(waypointEntity);
		if (!waypoint)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
			return false;
		}

		waypoint.SetEntity(targetEntity);
		waypoint.SetCompletionRadius(STOP_DISTANCE_METERS);
		group.AddWaypoint(waypoint);
		ApplyGroupFollowSpeed(group, movementType);
		ApplyGroupFollowFormation(group);
		m_WaypointEntity = waypointEntity;
		m_WaypointPosition = waypointPosition;
		if (!m_bLoggedWaypointFallback)
		{
			Print(string.Format("h-istasi mission captive follow | using entity follow waypoint fallback | target %1", targetEntity.GetName()));
			m_bLoggedWaypointFallback = true;
		}
		return true;
	}

	protected bool IssueStaticMoveWaypoint(AIGroup group, vector waypointPosition, EMovementType movementType)
	{
		if (!group)
			return false;

		GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(CAPTIVE_MOVE_WAYPOINT_PREFAB, waypointPosition, "0 0 0");
		AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
		if (!waypoint)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
			if (!m_bLoggedWaypointFailed)
			{
				Print("h-istasi mission captive follow | failed to create follow waypoint", LogLevel.WARNING);
				m_bLoggedWaypointFailed = true;
			}
			return false;
		}

		waypoint.SetCompletionRadius(STOP_DISTANCE_METERS);
		group.AddWaypoint(waypoint);
		ApplyGroupFollowSpeed(group, movementType);
		ApplyGroupFollowFormation(group);
		m_WaypointEntity = waypointEntity;
		m_WaypointPosition = waypointPosition;
		if (!m_bLoggedWaypointFallback)
		{
			Print(string.Format("h-istasi mission captive follow | using refreshed waypoint fallback | target %1", waypointPosition));
			m_bLoggedWaypointFallback = true;
		}
		return true;
	}

	protected void ClearFollowWaypoint(AIGroup group)
	{
		if (!m_WaypointEntity)
			return;

		AIWaypoint waypoint = AIWaypoint.Cast(m_WaypointEntity);
		if (group && waypoint)
			group.RemoveWaypoint(waypoint);

		SCR_EntityHelper.DeleteEntityAndChildren(m_WaypointEntity);
		m_WaypointEntity = null;
		m_WaypointPosition = "0 0 0";
	}

	protected AIGroup EnsureCaptiveAIGroup(IEntity owner, AIAgent agent)
	{
		if (!owner || !agent)
			return null;

		AIGroup group = agent.GetParentGroup();
		if (group)
		{
			group.ActivateAllMembers();
			return group;
		}

		if (m_GroupEntity)
			group = AIGroup.Cast(m_GroupEntity);

		if (!group)
		{
			m_GroupEntity = HST_WorldPositionService.SpawnPrefab(CAPTIVE_AI_GROUP_PREFAB, owner.GetOrigin(), "0 0 0");
			group = AIGroup.Cast(m_GroupEntity);
			if (!group)
			{
				if (!m_bLoggedGroupFailed)
				{
					Print("h-istasi mission captive follow | failed to create captive AI group", LogLevel.WARNING);
					m_bLoggedGroupFailed = true;
				}
				return null;
			}

			SCR_AIGroup scrGroup = SCR_AIGroup.Cast(group);
			if (scrGroup)
				scrGroup.InitFactionKey("CIV");
		}

		SCR_AIGroup captiveGroup = SCR_AIGroup.Cast(group);
		if (!m_bAddedToGroup)
		{
			if (captiveGroup)
				captiveGroup.AddAgentFromControlledEntity(owner);
			else
				group.AddAgent(agent);
			m_bAddedToGroup = true;
		}

		group.ActivateAllMembers();
		agent.ActivateAI();
		if (!m_bLoggedGroupCreated)
		{
			Print(string.Format("h-istasi mission captive follow | attached captive to AI group | group agents %1", group.GetAgentsCount()));
			m_bLoggedGroupCreated = true;
		}
		return group;
	}

	protected void TryForceFollowSpeed(IEntity owner, EMovementType movementType)
	{
		AIControlComponent control = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!control)
			return;

		AIAgent agent = control.GetControlAIAgent();
		if (!agent)
			return;

		AIGroup group = EnsureCaptiveAIGroup(owner, agent);
		if (!group)
			return;

		ApplyGroupFollowSpeed(group, movementType);
	}

	protected EMovementType ResolveFollowMovementType(float targetDistanceSq)
	{
		if (targetDistanceSq > SPRINT_CATCHUP_DISTANCE_METERS * SPRINT_CATCHUP_DISTANCE_METERS)
			return EMovementType.SPRINT;

		return EMovementType.RUN;
	}

	protected void ApplyGroupFollowSpeed(AIGroup group, EMovementType movementType)
	{
		if (!group)
			return;

		AIGroupMovementComponent groupMovement = AIGroupMovementComponent.Cast(group.GetMovementComponent());
		if (groupMovement)
			groupMovement.SetGroupCharactersWantedMovementType(movementType);
	}

	protected void ApplyGroupFollowFormation(AIGroup group)
	{
		if (!group)
			return;

		AIGroupMovementComponent groupMovement = AIGroupMovementComponent.Cast(group.FindComponent(AIGroupMovementComponent));
		if (groupMovement)
			groupMovement.SetFormationDisplacement(1);
	}

	protected float ResolveTargetStepDistance(vector targetPosition)
	{
		if (!m_bHasLastTargetPosition)
			return 0.0;

		return Math.Sqrt(DistanceSq2D(targetPosition, m_vLastTargetPosition));
	}

	protected vector BuildResponsiveFollowPosition(vector targetPosition, float targetStepDistance)
	{
		vector followPosition = targetPosition;
		if (!m_bHasLastTargetPosition || targetStepDistance < TARGET_MOVING_STEP_METERS)
			return followPosition;

		float leadDistance = Math.Min(MAX_TARGET_LEAD_METERS, targetStepDistance * TARGET_LEAD_MULTIPLIER);
		float dx = targetPosition[0] - m_vLastTargetPosition[0];
		float dz = targetPosition[2] - m_vLastTargetPosition[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length <= 0.01)
			return followPosition;

		followPosition[0] = followPosition[0] + (dx / length) * leadDistance;
		followPosition[2] = followPosition[2] + (dz / length) * leadDistance;
		return followPosition;
	}

	protected void RememberTargetPosition(vector targetPosition)
	{
		m_vLastTargetPosition = targetPosition;
		m_bHasLastTargetPosition = true;
	}

	protected bool UpdateFollowProgressState(vector ownerPosition, vector targetPosition)
	{
		if (DistanceSq2D(ownerPosition, targetPosition) <= STUCK_RECOVERY_DISTANCE_METERS * STUCK_RECOVERY_DISTANCE_METERS)
		{
			m_iNoProgressTicks = 0;
			m_vLastOwnerPosition = ownerPosition;
			m_bHasLastOwnerPosition = true;
			return false;
		}

		if (!m_bHasLastOwnerPosition)
		{
			m_iNoProgressTicks = 0;
			m_vLastOwnerPosition = ownerPosition;
			m_bHasLastOwnerPosition = true;
			return false;
		}

		if (DistanceSq2D(ownerPosition, m_vLastOwnerPosition) >= STUCK_RECOVERY_MIN_MOVE_METERS * STUCK_RECOVERY_MIN_MOVE_METERS)
		{
			m_iNoProgressTicks = 0;
			m_vLastOwnerPosition = ownerPosition;
			return false;
		}

		m_iNoProgressTicks++;
		m_vLastOwnerPosition = ownerPosition;
		return m_iNoProgressTicks >= STUCK_RECOVERY_TICKS;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float dx = a[0] - b[0];
		float dz = a[2] - b[2];
		return dx * dx + dz * dz;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected AIGroup ResolveParentAIGroup(IEntity owner)
	{
		if (!owner)
			return null;

		AIControlComponent control = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!control)
			return null;

		AIAgent agent = control.GetControlAIAgent();
		if (!agent)
			return null;

		return agent.GetParentGroup();
	}

	protected bool HasParentAIGroup(IEntity owner)
	{
		if (!owner)
			return false;

		AIControlComponent control = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (!control)
			return false;

		AIAgent agent = control.GetControlAIAgent();
		return agent && agent.GetParentGroup() != null;
	}
}
