[ComponentEditorProps(category: "h-istasi", description: "Keeps freed mission captives following their assigned player through AI movement")]
class HST_MissionCaptiveFollowComponentClass : ScriptComponentClass
{
}

class HST_MissionCaptiveFollowComponent : ScriptComponent
{
	static const string CAPTIVE_AI_GROUP_PREFAB = "{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et";
	static const string CAPTIVE_FOLLOW_WAYPOINT_PREFAB = "{FBA8DC8FDA0E770D}Prefabs/AI/Waypoints/AIWaypoint_Patrol_Hierarchy.et";
	static const float STOP_DISTANCE_METERS = 2.25;
	static const float WAYPOINT_REFRESH_DISTANCE_METERS = 14.0;
	static const float WAYPOINT_REACHED_REFRESH_DISTANCE_METERS = 4.0;
	static const float TARGET_MOVING_STEP_METERS = 0.35;
	static const float TARGET_LEAD_MULTIPLIER = 4.0;
	static const float MAX_TARGET_LEAD_METERS = 14.0;
	static const float SPRINT_CATCHUP_DISTANCE_METERS = 8.0;
	static const int FOLLOW_UPDATE_MS = 500;

	protected IEntity m_FollowTarget;
	protected IEntity m_GroupEntity;
	protected IEntity m_WaypointEntity;
	protected vector m_WaypointPosition;
	protected vector m_vLastTargetPosition;
	protected bool m_bFollowing;
	protected bool m_bAddedToGroup;
	protected bool m_bHasLastTargetPosition;
	protected bool m_bLoggedGroupCreated;
	protected bool m_bLoggedGroupFailed;
	protected bool m_bLoggedMissingMovement;
	protected bool m_bLoggedRequestFailed;
	protected bool m_bLoggedWaypointFallback;
	protected bool m_bLoggedWaypointFailed;
	protected bool m_bDirectFollowUnavailable;

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
			return;

		m_FollowTarget = target;
		m_bFollowing = true;
		m_bHasLastTargetPosition = false;
		m_vLastTargetPosition = "0 0 0";
		m_bLoggedGroupCreated = false;
		m_bLoggedGroupFailed = false;
		m_bLoggedMissingMovement = false;
		m_bLoggedRequestFailed = false;
		m_bLoggedWaypointFallback = false;
		m_bLoggedWaypointFailed = false;
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
		if (!m_bDirectFollowUnavailable && movement.RequestFollowPathOfEntity(m_FollowTarget))
			return;

		if (!m_bDirectFollowUnavailable)
		{
			if (!m_bLoggedRequestFailed)
			{
				Print(string.Format("h-istasi mission captive follow | RequestFollowPathOfEntity failed for %1 | group %2 | owner %3 | target %4", owner.GetName(), HasParentAIGroup(owner), owner.GetOrigin(), m_FollowTarget.GetOrigin()), LogLevel.WARNING);
				m_bLoggedRequestFailed = true;
			}
			m_bDirectFollowUnavailable = true;
		}

		IssueFollowWaypoint(group, ownerPosition, followPosition, movementType);
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
		return AIBaseMovementComponent.Cast(agent.GetMovementComponent());
	}

	protected bool IssueFollowWaypoint(AIGroup group, vector ownerPosition, vector targetPosition, EMovementType movementType)
	{
		if (!group)
			return false;
		if (IsZeroVector(targetPosition))
			return false;

		vector waypointPosition = HST_WorldPositionService.ResolveSafeGroundPosition(targetPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		if (m_WaypointEntity)
		{
			bool waypointReached = DistanceSq2D(ownerPosition, m_WaypointPosition) <= WAYPOINT_REACHED_REFRESH_DISTANCE_METERS * WAYPOINT_REACHED_REFRESH_DISTANCE_METERS;
			bool waypointStale = DistanceSq2D(m_WaypointPosition, waypointPosition) > WAYPOINT_REFRESH_DISTANCE_METERS * WAYPOINT_REFRESH_DISTANCE_METERS;
			if (!waypointReached && !waypointStale)
			{
				ApplyGroupFollowSpeed(group, movementType);
				return true;
			}
		}

		ClearFollowWaypoint(group);
		GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(CAPTIVE_FOLLOW_WAYPOINT_PREFAB, waypointPosition, "0 0 0");
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
