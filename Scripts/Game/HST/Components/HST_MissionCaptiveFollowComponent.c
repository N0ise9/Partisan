[ComponentEditorProps(category: "h-istasi", description: "Keeps freed mission captives following their assigned player through AI movement")]
class HST_MissionCaptiveFollowComponentClass : ScriptComponentClass
{
}

class HST_MissionCaptiveFollowComponent : ScriptComponent
{
	static const string CAPTIVE_AI_GROUP_PREFAB = "{6985327711303910}Prefabs/Groups/HST/HST_RuntimeEmptyGroup.et";
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
	static const float VEHICLE_EXIT_OFFSET_METERS = 2.0;
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
	protected bool m_bLoggedVehicleBoardingFailed;
	protected bool m_bStationaryWhenStopped;
	protected bool m_bDirectFollowUnavailable;
	protected int m_iNoProgressTicks;
	protected string m_sGroupFactionKey = "CIV";
	protected string m_sFollowLogLabel = "mission captive follow";

	bool IsFollowing()
	{
		return m_bFollowing && m_FollowTarget != null;
	}

	void SetGroupFactionKey(string factionKey)
	{
		if (factionKey.IsEmpty())
			return;

		m_sGroupFactionKey = factionKey;
	}

	void SetFollowLogLabel(string label)
	{
		if (label.IsEmpty())
			return;

		m_sFollowLogLabel = label;
	}

	void SetStationaryWhenStopped(bool enabled)
	{
		m_bStationaryWhenStopped = enabled;
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
		m_bLoggedVehicleBoardingFailed = false;
		m_bDirectFollowUnavailable = false;
		SetOwnerMovementLocked(owner, false);

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
		if (m_bStationaryWhenStopped)
			SetOwnerMovementLocked(GetOwner(), true);

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
		if (SyncVehicleStateWithTarget(owner, targetPosition))
			return;

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
				Print(string.Format("h-istasi %1 | no AI movement component for %2", m_sFollowLogLabel, owner.GetName()), LogLevel.WARNING);
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
			Print(string.Format("h-istasi %1 | direct follow stalled, refreshing waypoint | owner %2 | target %3", m_sFollowLogLabel, owner.GetOrigin(), m_FollowTarget.GetOrigin()), LogLevel.WARNING);
			m_bLoggedStuckRecovery = true;
		}

		if (!m_bDirectFollowUnavailable)
		{
			if (!m_bLoggedRequestFailed)
			{
				Print(string.Format("h-istasi %1 | RequestFollowPathOfEntity failed for %2 | group %3 | owner %4 | target %5", m_sFollowLogLabel, owner.GetName(), HasParentAIGroup(owner), owner.GetOrigin(), m_FollowTarget.GetOrigin()), LogLevel.WARNING);
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
			Print(string.Format("h-istasi %1 | direct catch-up after stalled follow | owner %2 -> %3 | target %4 | distance %5m", m_sFollowLogLabel, ownerPosition, catchupPosition, followPosition, Math.Round(distance)), LogLevel.WARNING);
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
			Print(string.Format("h-istasi %1 | using entity follow waypoint fallback | target %2", m_sFollowLogLabel, targetEntity.GetName()));
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
				Print(string.Format("h-istasi %1 | failed to create follow waypoint", m_sFollowLogLabel), LogLevel.WARNING);
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
			Print(string.Format("h-istasi %1 | using refreshed waypoint fallback | target %2", m_sFollowLogLabel, waypointPosition));
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
					Print(string.Format("h-istasi %1 | failed to create follower AI group", m_sFollowLogLabel), LogLevel.WARNING);
					m_bLoggedGroupFailed = true;
				}
				return null;
			}

			SCR_AIGroup scrGroup = SCR_AIGroup.Cast(group);
			if (scrGroup)
				scrGroup.InitFactionKey(m_sGroupFactionKey);
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
			Print(string.Format("h-istasi %1 | attached follower to AI group | group agents %2", m_sFollowLogLabel, group.GetAgentsCount()));
			m_bLoggedGroupCreated = true;
		}
		return group;
	}

	protected bool SyncVehicleStateWithTarget(IEntity owner, vector targetPosition)
	{
		if (!owner || !m_FollowTarget)
			return false;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(owner.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
			return false;

		IEntity targetVehicle = CompartmentAccessComponent.GetVehicleIn(m_FollowTarget);
		if (targetVehicle)
		{
			string reason;
			if (TryMoveFollowerIntoVehicle(owner, targetVehicle, reason))
				return true;

			if (!m_bLoggedVehicleBoardingFailed)
			{
				Print(string.Format("h-istasi %1 | vehicle boarding failed: %2", m_sFollowLogLabel, reason), LogLevel.WARNING);
				m_bLoggedVehicleBoardingFailed = true;
			}
			return false;
		}

		if (!access.IsInCompartment())
			return false;

		if (access.IsGettingOut())
			return true;

		vector exitPosition = ResolveFollowerVehicleExitPosition(owner, targetPosition);
		vector exitTransform[4];
		Math3D.MatrixIdentity4(exitTransform);
		exitTransform[3] = exitPosition;
		if (access.GetOutVehicle_NoDoor(exitTransform, false, true, true))
			return true;

		if (access.GetOutVehicle(EGetOutType.TELEPORT, -1, ECloseDoorAfterActions.INVALID, true, true))
			return true;

		access.AskOwnerToGetOutFromVehicle(EGetOutType.TELEPORT, -1, ECloseDoorAfterActions.INVALID, true, true);
		return true;
	}

	protected bool TryMoveFollowerIntoVehicle(IEntity followerEntity, IEntity vehicleEntity, out string reason)
	{
		reason = "";
		if (!followerEntity || !vehicleEntity)
		{
			reason = "follower or vehicle missing";
			return false;
		}

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(followerEntity.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
		{
			reason = "follower has no compartment access component";
			return false;
		}
		if (access.IsInCompartment() && access.GetVehicle() == vehicleEntity)
		{
			reason = "follower already seated";
			return true;
		}
		if (access.IsGettingIn())
		{
			reason = "follower already boarding";
			return true;
		}

		BaseCompartmentManagerComponent compartmentManager = ResolveCompartmentManager(vehicleEntity);
		if (!compartmentManager)
		{
			reason = "vehicle has no compartment manager";
			return false;
		}

		array<BaseCompartmentSlot> slots = {};
		compartmentManager.GetCompartments(slots);
		BaseCompartmentSlot slot = FindAvailableFollowerSeat(slots, followerEntity);
		if (!slot)
		{
			reason = "vehicle has no accessible cargo seat";
			return false;
		}

		IEntity slotOwner = slot.GetOwner();
		if (!slotOwner)
			slotOwner = vehicleEntity;
		if (access.GetInVehicle(slotOwner, slot, true, -1, ECloseDoorAfterActions.INVALID, true))
		{
			reason = "server-authoritative cargo move-in completed";
			return true;
		}

		if (!access.MoveInVehicle(vehicleEntity, ECompartmentType.CARGO, true, slot))
		{
			reason = "vehicle boarding order rejected";
			return false;
		}

		reason = "animated vehicle boarding order issued";
		return true;
	}

	protected BaseCompartmentManagerComponent ResolveCompartmentManager(IEntity vehicleEntity)
	{
		if (!vehicleEntity)
			return null;

		SCR_BaseCompartmentManagerComponent scrManager = SCR_BaseCompartmentManagerComponent.Cast(vehicleEntity.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (scrManager)
			return scrManager;

		return BaseCompartmentManagerComponent.Cast(vehicleEntity.FindComponent(BaseCompartmentManagerComponent));
	}

	protected BaseCompartmentSlot FindAvailableFollowerSeat(array<BaseCompartmentSlot> slots, IEntity followerEntity)
	{
		if (!slots || !followerEntity)
			return null;

		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;
			if (slot.GetType() != ECompartmentType.CARGO)
				continue;
			if (!slot.IsCompartmentAccessible())
				continue;
			if (slot.IsOccupied())
				continue;
			if (slot.IsReserved() && !slot.IsReservedBy(followerEntity))
				continue;
			if (slot.IsGetInLockedFor(followerEntity))
				continue;

			return slot;
		}

		return null;
	}

	protected vector ResolveFollowerVehicleExitPosition(IEntity owner, vector targetPosition)
	{
		vector exitPosition = targetPosition;
		vector ownerPosition = owner.GetOrigin();
		float dx = ownerPosition[0] - targetPosition[0];
		float dz = ownerPosition[2] - targetPosition[2];
		float length = Math.Sqrt(dx * dx + dz * dz);
		if (length <= 0.01)
		{
			exitPosition[0] = exitPosition[0] + VEHICLE_EXIT_OFFSET_METERS;
		}
		else
		{
			exitPosition[0] = exitPosition[0] + (dx / length) * VEHICLE_EXIT_OFFSET_METERS;
			exitPosition[2] = exitPosition[2] + (dz / length) * VEHICLE_EXIT_OFFSET_METERS;
		}

		return HST_WorldPositionService.ResolveSafeGroundPosition(exitPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 1.0);
	}

	protected void SetOwnerMovementLocked(IEntity owner, bool locked)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(owner);
		if (!character)
			return;

		CharacterControllerComponent controller = character.GetCharacterController();
		if (!controller)
			return;

		if (locked)
			controller.SetMovement(0, vector.Zero);
		controller.SetDisableMovementControls(locked);
		controller.SetDisableWeaponControls(true);
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
