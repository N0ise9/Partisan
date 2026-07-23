class HST_ConvoyCrewSeatingResult
{
	int m_iLivingCrew;
	int m_iDiscoveredCompartments;
	int m_iDriverSlots;
	int m_iTurretSlots;
	int m_iCargoSlots;
	int m_iSeatedCrew;
	int m_iDriverSeatedCount;
	int m_iTurretSeatedCount;
	int m_iCargoSeatedCount;
	int m_iIssuedOrders;
	int m_iRemovedOverflowCrew;
	bool m_bDriverAssigned;
	bool m_bSeatingPending;
	string m_sReason;

	string ToReportString()
	{
		string report = string.Format("driver assigned %1 | seated crew %2/%3", BoolText(m_bDriverAssigned), m_iSeatedCrew, m_iLivingCrew);
		report = report + string.Format(" | turret seated %1 | cargo seated %2 | overflow removed %3 | seating pending %4", m_iTurretSeatedCount, m_iCargoSeatedCount, m_iRemovedOverflowCrew, BoolText(m_bSeatingPending));
		report = report + string.Format(" | seating orders %1 | compartments %2 | driver slots %3", m_iIssuedOrders, m_iDiscoveredCompartments, m_iDriverSlots);
		report = report + string.Format(" | turret slots %1 | cargo slots %2 | last seating reason %3", m_iTurretSlots, m_iCargoSlots, ReportText(m_sReason));
		return report;
	}

	protected string BoolText(bool value)
	{
		if (value)
			return "yes";

		return "no";
	}

	protected string ReportText(string value)
	{
		if (value.IsEmpty())
			return "none";

		return value;
	}
}

class HST_ConvoyVehicleControlAdapter
{
	static const string CONVOY_WAYPOINT_PREFAB = "{FBA8DC8FDA0E770D}Prefabs/AI/Waypoints/AIWaypoint_Patrol_Hierarchy.et";
	protected ref array<IEntity> m_aInterruptedBoardingCrew = {};
	protected ref array<IEntity> m_aForcedBoardingRecoveryCrew = {};
#ifdef ENABLE_DIAG
	static const string CAMPAIGN_DEBUG_PREFIX_ROOT = "hst_debug_";
	static const string CAMPAIGN_DEBUG_ENTITY_TAG = "HST_CAMPAIGN_DEBUG";
#endif

	bool TryBindCrewToVehicle(HST_ActiveGroupState groupState, IEntity groupEntity, IEntity vehicleEntity, out string reason)
	{
		reason = "";
		if (!groupState)
		{
			reason = "Convoy adapter cannot bind crew: group state missing.";
			return false;
		}
		if (!groupEntity)
		{
			reason = "Convoy adapter cannot bind crew: crew group entity missing.";
			return false;
		}
		if (!vehicleEntity)
		{
			reason = "Convoy adapter cannot bind crew: vehicle entity missing.";
			return false;
		}

		string vehicleRegistrationReason;
		if (!TryRegisterVehicleWithGroup(groupEntity, vehicleEntity, vehicleRegistrationReason))
		{
			reason = "Convoy adapter cannot bind crew before seating: " + vehicleRegistrationReason + ".";
			return false;
		}

		HST_ConvoyCrewSeatingResult seating = BuildCrewSeatingResult(groupEntity, vehicleEntity, true);
		reason = seating.ToReportString() + " | pre-seat " + vehicleRegistrationReason;
		return seating.m_bDriverAssigned;
	}

	bool TryAssignVehicleRoute(HST_ActiveGroupState groupState, IEntity groupEntity, IEntity vehicleEntity, array<vector> waypoints, out int assignedWaypointCount, out string reason)
	{
		assignedWaypointCount = 0;
		reason = "";
		if (!groupState)
		{
			reason = "Convoy adapter cannot assign route: group state missing.";
			return false;
		}
		if (!groupEntity)
		{
			reason = "Convoy adapter cannot assign route: crew group entity missing.";
			return false;
		}
		if (!vehicleEntity)
		{
			reason = "Convoy adapter cannot assign route: vehicle entity missing.";
			return false;
		}
		if (!waypoints || waypoints.Count() == 0)
		{
			reason = "Convoy adapter cannot assign route: no route waypoints.";
			return false;
		}
		if (waypoints.Count() <= 1)
		{
			reason = "Convoy adapter cannot assign route: waypoint-chain route needs at least two waypoint positions.";
			return false;
		}
		if (CountLivingCrew(groupEntity) <= 0)
		{
			reason = "Convoy adapter cannot assign route: no living crew agents.";
			return false;
		}

		string mobileReason;
		if (!IsVehicleMobile(vehicleEntity, mobileReason))
		{
			reason = "Convoy adapter cannot assign route: " + mobileReason;
			return false;
		}
		if (!HasLivingDriver(groupEntity, vehicleEntity))
		{
			reason = "Convoy adapter cannot assign route: no seated living AI driver is assigned to the vehicle.";
			return false;
		}

		AIGroup group = AIGroup.Cast(groupEntity);
		if (!group)
		{
			reason = "Convoy adapter cannot assign route: crew entity is not an AIGroup.";
			return false;
		}

		string vehicleRegistrationReason;
		if (!TryRegisterVehicleWithGroup(groupEntity, vehicleEntity, vehicleRegistrationReason))
		{
			reason = "Convoy adapter cannot assign route: " + vehicleRegistrationReason;
			return false;
		}

		array<IEntity> spawnedWaypoints = {};
		foreach (vector waypointPosition : waypoints)
		{
			IEntity waypointEntity = SpawnRouteWaypoint(waypointPosition, groupState.m_sGroupId, spawnedWaypoints.Count() + 1);
			if (!waypointEntity)
			{
				DeleteSpawnedWaypoints(spawnedWaypoints);
				reason = string.Format("Convoy adapter route assignment failed while creating AI waypoint %1/%2.", spawnedWaypoints.Count() + 1, waypoints.Count());
				return false;
			}

			spawnedWaypoints.Insert(waypointEntity);
		}

		foreach (IEntity waypointEntityToAssign : spawnedWaypoints)
		{
			AIWaypoint waypoint = AIWaypoint.Cast(waypointEntityToAssign);
			if (!waypoint)
			{
				DeleteSpawnedWaypoints(spawnedWaypoints);
				reason = "Convoy adapter route assignment failed while resolving spawned AI waypoint.";
				return false;
			}

			group.AddWaypoint(waypoint);
			assignedWaypointCount++;
		}

		reason = string.Format("Convoy adapter assigned route waypoint chain %1/%2; %3.", assignedWaypointCount, waypoints.Count(), vehicleRegistrationReason);
		return true;
	}

	protected bool TryRegisterVehicleWithGroup(IEntity groupEntity, IEntity vehicleEntity, out string reason)
	{
		reason = "";
		if (!groupEntity)
		{
			reason = "crew group entity missing for AI vehicle registration";
			return false;
		}
		if (!vehicleEntity)
		{
			reason = "vehicle entity missing for AI vehicle registration";
			return false;
		}

		SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(groupEntity.FindComponent(SCR_AIGroupUtilityComponent));
		if (!utility)
		{
			reason = "crew group has no AI group utility component for vehicle registration";
			return false;
		}

		IEntity vehicleUsageOwner = vehicleEntity;
		SCR_AIVehicleUsageComponent vehicleUsage = SCR_AIVehicleUsageComponent.FindOnNearestParent(vehicleEntity, vehicleUsageOwner);
		if (!vehicleUsage)
		{
			reason = "vehicle has no AI vehicle usage component";
			return false;
		}
		if (!vehicleUsage.IsVehicleTypeValid())
		{
			reason = "vehicle AI usage component has no valid vehicle type";
			return false;
		}
		if (!vehicleUsage.CanBePiloted())
		{
			reason = "vehicle AI usage component is not pilotable";
			return false;
		}

		utility.AddUsableVehicle(vehicleUsage);
		if (!utility.IsUsableVehicle(vehicleUsage))
		{
			reason = "vehicle usage registration was not retained by the AI group utility";
			return false;
		}

		reason = "vehicle usage registered for group movement";
		return true;
	}

	bool IsVehicleRegisteredWithGroup(IEntity groupEntity, IEntity vehicleEntity, out string reason)
	{
		reason = "";
		if (!groupEntity || !vehicleEntity)
		{
			reason = "crew group or vehicle entity missing for AI vehicle registration proof";
			return false;
		}

		SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(groupEntity.FindComponent(SCR_AIGroupUtilityComponent));
		if (!utility)
		{
			reason = "crew group has no AI group utility component";
			return false;
		}

		IEntity vehicleUsageOwner = vehicleEntity;
		SCR_AIVehicleUsageComponent vehicleUsage = SCR_AIVehicleUsageComponent.FindOnNearestParent(vehicleEntity, vehicleUsageOwner);
		if (!vehicleUsage)
		{
			reason = "vehicle has no AI vehicle usage component";
			return false;
		}
		if (!vehicleUsage.IsVehicleTypeValid())
		{
			reason = "vehicle AI usage component has no valid vehicle type";
			return false;
		}
		if (!vehicleUsage.CanBePiloted())
		{
			reason = "vehicle AI usage component is not pilotable";
			return false;
		}
		if (!utility.IsUsableVehicle(vehicleUsage))
		{
			reason = "vehicle usage is not retained by the crew group";
			return false;
		}

		reason = "vehicle usage is retained by the crew group";
		return true;
	}

	int CountLivingCrew(IEntity groupEntity)
	{
		if (!groupEntity)
			return 0;

		array<IEntity> livingCrew = {};
		string reason;
		CollectLivingCrewEntities(groupEntity, livingCrew, reason);
		return livingCrew.Count();
	}

	int MoveUnseatedLivingCrewNearVehicle(IEntity groupEntity, IEntity vehicleEntity, vector position)
	{
		if (!groupEntity || !vehicleEntity)
			return 0;

		array<IEntity> livingCrew = {};
		string reason;
		CollectLivingCrewEntities(groupEntity, livingCrew, reason);
		int moved;
		foreach (IEntity crewEntity : livingCrew)
		{
			if (!crewEntity)
				continue;
			if (IsCrewSeatedInVehicle(crewEntity, vehicleEntity))
				continue;
			if (IsCrewGettingIntoVehicle(crewEntity, vehicleEntity))
				continue;

			vector crewPosition = BuildCrewStagingPosition(position, moved);
			HST_WorldPositionService.ApplyUprightEntityTransform(crewEntity, crewPosition, crewEntity.GetYawPitchRoll());
			moved++;
		}

		return moved;
	}

	protected vector BuildCrewStagingPosition(vector centerPosition, int index)
	{
		vector position = centerPosition;
		int slotIndex = index % 8;
		float offset = 2.75;
		if (slotIndex == 0)
		{
			position[0] = position[0] + offset;
			position[2] = position[2] + offset;
		}
		else if (slotIndex == 1)
		{
			position[0] = position[0] - offset;
			position[2] = position[2] + offset;
		}
		else if (slotIndex == 2)
		{
			position[0] = position[0] + offset;
			position[2] = position[2] - offset;
		}
		else if (slotIndex == 3)
		{
			position[0] = position[0] - offset;
			position[2] = position[2] - offset;
		}
		else if (slotIndex == 4)
		{
			position[0] = position[0] + offset * 1.6;
		}
		else if (slotIndex == 5)
		{
			position[0] = position[0] - offset * 1.6;
		}
		else if (slotIndex == 6)
		{
			position[2] = position[2] + offset * 1.6;
		}
		else
		{
			position[2] = position[2] - offset * 1.6;
		}

		return HST_WorldPositionService.ResolveSafeGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 4.0);
	}

	bool HasLivingDriver(IEntity groupEntity, IEntity vehicleEntity)
	{
		if (!groupEntity || !vehicleEntity)
			return false;

		HST_ConvoyCrewSeatingResult seating = BuildCrewSeatingResult(groupEntity, vehicleEntity, false);
		return seating.m_bDriverAssigned;
	}

	bool AreLivingCrewMounted(IEntity groupEntity, IEntity vehicleEntity, out string reason)
	{
		HST_ConvoyCrewSeatingResult seating = BuildCrewSeatingResult(groupEntity, vehicleEntity, false);
		reason = seating.ToReportString();
		return seating.m_iLivingCrew > 0 && seating.m_bDriverAssigned && seating.m_iSeatedCrew >= seating.m_iLivingCrew;
	}

	bool AreAllLivingCrewDismounted(IEntity groupEntity, IEntity vehicleEntity, out string reason)
	{
		array<IEntity> livingCrew = {};
		string crewReason;
		CollectLivingCrewEntities(groupEntity, livingCrew, crewReason);
		if (livingCrew.Count() <= 0)
		{
			reason = crewReason;
			return false;
		}

		if (!vehicleEntity)
		{
			reason = "vehicle entity missing while living crew remains";
			return true;
		}

		HST_ConvoyCrewSeatingResult seating = BuildCrewSeatingResult(groupEntity, vehicleEntity, false);
		reason = seating.ToReportString();
		return seating.m_iLivingCrew > 0 && seating.m_iSeatedCrew <= 0;
	}

	string BuildCrewSeatingReport(IEntity groupEntity, IEntity vehicleEntity)
	{
		HST_ConvoyCrewSeatingResult seating = BuildCrewSeatingResult(groupEntity, vehicleEntity, false);
		return seating.ToReportString();
	}

	bool IsVehicleMobile(IEntity vehicleEntity, out string reason)
	{
		reason = "";
		if (!vehicleEntity)
		{
			reason = "vehicle entity missing";
			return false;
		}
		if (!IsLivingEntity(vehicleEntity))
		{
			reason = "vehicle is destroyed";
			return false;
		}

		reason = "vehicle entity is alive";
		return true;
	}

	protected HST_ConvoyCrewSeatingResult BuildCrewSeatingResult(IEntity groupEntity, IEntity vehicleEntity, bool issueOrders)
	{
		HST_ConvoyCrewSeatingResult result = new HST_ConvoyCrewSeatingResult();
		if (!groupEntity)
		{
			result.m_sReason = "crew group entity missing";
			return result;
		}
		if (!vehicleEntity)
		{
			result.m_sReason = "vehicle entity missing";
			return result;
		}

		array<IEntity> livingCrew = {};
		string crewReason;
		CollectLivingCrewEntities(groupEntity, livingCrew, crewReason);
		result.m_iLivingCrew = livingCrew.Count();
		if (result.m_iLivingCrew <= 0)
		{
			result.m_sReason = crewReason;
			return result;
		}

		string mobileReason;
		if (!IsVehicleMobile(vehicleEntity, mobileReason))
		{
			result.m_sReason = mobileReason;
			return result;
		}

		BaseCompartmentManagerComponent compartmentManager = ResolveCompartmentManager(vehicleEntity);
		if (!compartmentManager)
		{
			result.m_sReason = "vehicle has no compartment manager";
			return result;
		}

		array<BaseCompartmentSlot> slots = {};
		compartmentManager.GetCompartments(slots);
		result.m_iDiscoveredCompartments = slots.Count();
		ScanCompartmentSlots(slots, livingCrew, result);
		result.m_bSeatingPending = HasCrewBoardingVehicle(livingCrew, vehicleEntity);

		array<IEntity> orderedCrew = {};
		array<BaseCompartmentSlot> orderedSlots = {};
		string orderReason;
		string driverOrderReason;
		if (issueOrders)
		{
			if (!result.m_bDriverAssigned && TryOrderNextCrewIntoSlot(livingCrew, orderedCrew, orderedSlots, vehicleEntity, slots, ECompartmentType.PILOT, driverOrderReason))
			{
				result.m_iIssuedOrders++;
				result.m_bSeatingPending = true;
				RefreshSeatedCrewState(slots, livingCrew, vehicleEntity, result);
			}

			// Never let a pending/failed pilot transition consume its bounded
			// recovery against a turret or cargo slot in the same ordering pass.
			// Auxiliary seating begins only after the slot scan confirms a living
			// pilot occupant.
			while (result.m_bDriverAssigned
				&& TryOrderNextCrewIntoSlot(livingCrew, orderedCrew, orderedSlots, vehicleEntity, slots, ECompartmentType.TURRET, orderReason))
			{
				result.m_iIssuedOrders++;
				result.m_bSeatingPending = true;
			}

			while (result.m_bDriverAssigned
				&& TryOrderNextCrewIntoSlot(livingCrew, orderedCrew, orderedSlots, vehicleEntity, slots, ECompartmentType.CARGO, orderReason))
			{
				result.m_iIssuedOrders++;
				result.m_bSeatingPending = true;
			}

			if (result.m_iIssuedOrders > 0)
			{
				RefreshSeatedCrewState(slots, livingCrew, vehicleEntity, result);
				if (!result.m_bDriverAssigned)
					result.m_bSeatingPending = true;
			}
		}

		if (issueOrders && result.m_bDriverAssigned && !result.m_bSeatingPending && result.m_iIssuedOrders <= 0 && result.m_iSeatedCrew < result.m_iLivingCrew)
		{
			result.m_iRemovedOverflowCrew = RemoveUnseatedOverflowCrew(livingCrew, vehicleEntity);
			if (result.m_iRemovedOverflowCrew > 0)
				result.m_iLivingCrew = Math.Max(result.m_iSeatedCrew, result.m_iLivingCrew - result.m_iRemovedOverflowCrew);
		}

		if (result.m_iDriverSlots <= 0)
			result.m_sReason = "vehicle has no accessible driver compartment";
		else if (!result.m_bDriverAssigned && result.m_iIssuedOrders > 0)
			result.m_sReason = "waiting for authoritative seat transition to confirm a driver";
		else if (!result.m_bDriverAssigned && !driverOrderReason.IsEmpty())
			result.m_sReason = driverOrderReason;
		else if (!result.m_bDriverAssigned && !orderReason.IsEmpty())
			result.m_sReason = orderReason;
		else if (!result.m_bDriverAssigned)
			result.m_sReason = "no seated living AI driver yet";
		else if (result.m_iRemovedOverflowCrew > 0)
			result.m_sReason = string.Format("driver seated; removed %1 surplus convoy crew without vehicle seats", result.m_iRemovedOverflowCrew);
		else if (result.m_iSeatedCrew < result.m_iLivingCrew && (result.m_iTurretSlots + result.m_iCargoSlots) > 0)
			result.m_sReason = "driver seated; remaining crew seating may still be in progress";
		else
			result.m_sReason = "driver seated";

		return result;
	}

	protected bool CollectLivingCrewEntities(IEntity groupEntity, array<IEntity> livingCrew, out string reason)
	{
		reason = "";
		if (!groupEntity)
		{
			reason = "crew group entity missing";
			return false;
		}

		AIGroup group = AIGroup.Cast(groupEntity);
		if (group)
		{
			array<AIAgent> agents = {};
			group.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;

				IEntity controlledEntity = agent.GetControlledEntity();
				if (!IsLivingEntity(controlledEntity))
					continue;

				if (livingCrew.Find(controlledEntity) < 0)
					livingCrew.Insert(controlledEntity);
			}

			if (livingCrew.Count() <= 0)
				reason = "crew group has not populated living controlled agents";
			else
				reason = "living controlled agents found";

			return livingCrew.Count() > 0;
		}

		if (IsLivingEntity(groupEntity))
			livingCrew.Insert(groupEntity);

		if (livingCrew.Count() <= 0)
			reason = "crew entity is not living";
		else
			reason = "single living crew entity found";

		return livingCrew.Count() > 0;
	}

	protected int RemoveUnseatedOverflowCrew(array<IEntity> livingCrew, IEntity vehicleEntity)
	{
		if (!livingCrew || !vehicleEntity)
			return 0;

		int removed;
		foreach (IEntity crewEntity : livingCrew)
		{
			if (!crewEntity)
				continue;
			if (IsCrewSeatedInVehicle(crewEntity, vehicleEntity))
				continue;
			if (IsCrewGettingIntoVehicle(crewEntity, vehicleEntity))
				continue;

			SCR_EntityHelper.DeleteEntityAndChildren(crewEntity);
			removed++;
		}

		return removed;
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

	protected void ScanCompartmentSlots(array<BaseCompartmentSlot> slots, array<IEntity> livingCrew, HST_ConvoyCrewSeatingResult result)
	{
		if (!slots || !livingCrew || !result)
			return;

		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;

			if (IsDriverSlot(slot))
				result.m_iDriverSlots++;
			else if (slot.GetType() == ECompartmentType.TURRET)
				result.m_iTurretSlots++;
			else if (slot.GetType() == ECompartmentType.CARGO)
				result.m_iCargoSlots++;

			IEntity occupant = slot.GetOccupant();
			if (!IsCrewEntity(livingCrew, occupant))
				continue;
			if (!IsLivingEntity(occupant))
				continue;

			result.m_iSeatedCrew++;
			if (IsDriverSlot(slot))
				result.m_iDriverSeatedCount++;
			else if (slot.GetType() == ECompartmentType.TURRET)
				result.m_iTurretSeatedCount++;
			else if (slot.GetType() == ECompartmentType.CARGO)
				result.m_iCargoSeatedCount++;
		}

		result.m_bDriverAssigned = result.m_iDriverSeatedCount > 0;
	}

	protected void RefreshSeatedCrewState(array<BaseCompartmentSlot> slots, array<IEntity> livingCrew, IEntity vehicleEntity, HST_ConvoyCrewSeatingResult result)
	{
		if (!slots || !livingCrew || !result)
			return;

		result.m_iSeatedCrew = 0;
		result.m_iDriverSeatedCount = 0;
		result.m_iTurretSeatedCount = 0;
		result.m_iCargoSeatedCount = 0;

		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;

			IEntity occupant = slot.GetOccupant();
			if (!IsCrewEntity(livingCrew, occupant))
				continue;
			if (!IsLivingEntity(occupant))
				continue;

			result.m_iSeatedCrew++;
			if (IsDriverSlot(slot))
				result.m_iDriverSeatedCount++;
			else if (slot.GetType() == ECompartmentType.TURRET)
				result.m_iTurretSeatedCount++;
			else if (slot.GetType() == ECompartmentType.CARGO)
				result.m_iCargoSeatedCount++;
		}

		result.m_bDriverAssigned = result.m_iDriverSeatedCount > 0;
		result.m_bSeatingPending = HasCrewBoardingVehicle(livingCrew, vehicleEntity);
	}

	protected bool TryOrderNextCrewIntoSlot(array<IEntity> livingCrew, array<IEntity> orderedCrew, array<BaseCompartmentSlot> orderedSlots, IEntity vehicleEntity, array<BaseCompartmentSlot> slots, ECompartmentType compartmentType, out string reason)
	{
		reason = "";
		if (!livingCrew || !orderedCrew || !orderedSlots || !vehicleEntity || !slots)
			return false;

		foreach (IEntity crewEntity : livingCrew)
		{
			if (!crewEntity || orderedCrew.Find(crewEntity) >= 0)
				continue;
			if (IsCrewSeatedInVehicle(crewEntity, vehicleEntity))
				continue;

			BaseCompartmentSlot slot = FindAvailableSlotForCrew(slots, orderedSlots, crewEntity, compartmentType);
			if (!slot)
			{
				reason = ResolveMissingSlotReason(compartmentType);
				return false;
			}

			if (TryMoveCrewIntoSlot(crewEntity, vehicleEntity, slot, compartmentType, reason))
			{
				orderedCrew.Insert(crewEntity);
				orderedSlots.Insert(slot);
				return true;
			}
		}

		if (reason.IsEmpty())
			reason = "no available living crew member for " + ResolveCompartmentLabel(compartmentType);

		return false;
	}

	protected BaseCompartmentSlot FindAvailableSlotForCrew(array<BaseCompartmentSlot> slots, array<BaseCompartmentSlot> orderedSlots, IEntity crewEntity, ECompartmentType compartmentType)
	{
		if (!slots || !orderedSlots || !crewEntity)
			return null;

		foreach (BaseCompartmentSlot slot : slots)
		{
			if (orderedSlots.Find(slot) >= 0)
				continue;
			if (!IsSlotForCompartmentType(slot, compartmentType))
				continue;
			if (!IsSlotAvailableForCrew(slot, crewEntity))
				continue;

			return slot;
		}

		return null;
	}

	protected bool TryMoveCrewIntoSlot(IEntity crewEntity, IEntity vehicleEntity, BaseCompartmentSlot slot, ECompartmentType compartmentType, out string reason)
	{
		reason = "";
		PruneBoardingRecoveryState();
		if (!crewEntity || !vehicleEntity || !slot)
		{
			reason = "crew, vehicle, or compartment missing";
			return false;
		}

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(crewEntity.FindComponent(SCR_CompartmentAccessComponent));
		if (!access)
		{
			reason = "crew member has no compartment access component";
			return false;
		}
		if (access.IsInCompartment() && access.GetVehicle() == vehicleEntity)
		{
			m_aInterruptedBoardingCrew.RemoveItem(crewEntity);
			m_aForcedBoardingRecoveryCrew.RemoveItem(crewEntity);
			reason = "crew member is already seated in the vehicle";
			return false;
		}

		RplComponent crewReplication = RplComponent.Cast(
			crewEntity.FindComponent(RplComponent));
		AIControlComponent crewControl = AIControlComponent.Cast(
			crewEntity.FindComponent(AIControlComponent));
		AIAgent crewAgent;
		if (crewControl)
			crewAgent = crewControl.GetAIAgent();
		bool aiControlled = crewAgent
			&& crewAgent.GetControlledEntity() == crewEntity;
		bool authorityLocalAI = aiControlled
			&& (!crewReplication || !crewReplication.IsProxy());
		bool nonReplicated = RplSession.Mode() == RplMode.None;
		bool canSeatLocally = nonReplicated || authorityLocalAI;
		string localityEvidence = string.Format(
			"mode %1 | ai controlled %2 | replication present %3 owner %4 proxy %5",
			RplSession.Mode(),
			aiControlled,
			crewReplication != null,
			crewReplication && crewReplication.IsOwner(),
			crewReplication && crewReplication.IsProxy());
		if (access.IsGettingIn())
		{
			if (!canSeatLocally)
			{
				reason = "non-local crew member is already getting into a vehicle without confirmed occupancy | "
					+ localityEvidence;
#ifdef ENABLE_DIAG
				Print("Partisan convoy seating debug | pending non-local transition | " + reason);
#endif
				return false;
			}
			if (!m_aInterruptedBoardingCrew.Contains(crewEntity))
			{
				access.InterruptVehicleActionQueue(true, true, true);
				m_aInterruptedBoardingCrew.Insert(crewEntity);
				reason = "interrupted one unconfirmed authority-local boarding transition; forced recovery waits for the next seating observation | "
					+ localityEvidence;
#ifdef ENABLE_DIAG
				Print("Partisan convoy seating debug | bounded interrupt | " + reason);
#endif
				return false;
			}
			if (m_aForcedBoardingRecoveryCrew.Contains(crewEntity))
			{
				reason = "bounded authority-local boarding recovery exhausted without confirmed occupancy | "
					+ localityEvidence;
#ifdef ENABLE_DIAG
				Print("Partisan convoy seating debug | recovery exhausted | " + reason);
#endif
				return false;
			}
		}
		else if (m_aInterruptedBoardingCrew.Contains(crewEntity)
			&& m_aForcedBoardingRecoveryCrew.Contains(crewEntity))
		{
			reason = "bounded authority-local boarding recovery ended without a pending transition or confirmed occupancy | "
				+ localityEvidence;
#ifdef ENABLE_DIAG
			Print("Partisan convoy seating debug | recovery ended unseated | " + reason);
#endif
			return false;
		}
		if (slot.IsGetInLockedFor(crewEntity))
		{
			reason = "target compartment is locked for crew member";
			return false;
		}

		IEntity slotOwner = slot.GetOwner();
		if (!slotOwner)
			slotOwner = vehicleEntity;

		// Convoy members are collected from an AIGroup. A replicated server-owned
		// AI character is authority-local when it is not a proxy even if IsOwner()
		// is false; IsOwner() alone therefore cannot gate the direct native call.
		// MoveInVehicle only proves owner-RPC acceptance, so a non-replicated world
		// must never fall through to that request when the local forced call fails.
		bool boundedRecoveryAttempt
			= m_aInterruptedBoardingCrew.Contains(crewEntity)
				&& !m_aForcedBoardingRecoveryCrew.Contains(crewEntity);
		if (boundedRecoveryAttempt)
			m_aForcedBoardingRecoveryCrew.Insert(crewEntity);
		bool directAccepted = false;
		if (canSeatLocally)
			directAccepted = access.GetInVehicle(
				slotOwner,
				slot,
				true,
				-1,
				ECloseDoorAfterActions.INVALID,
				true);
		if (directAccepted)
		{
			bool occupancyConfirmed
				= (access.IsInCompartment()
					&& access.GetVehicle() == vehicleEntity)
					|| slot.GetOccupant() == crewEntity;
			if (occupancyConfirmed)
			{
				m_aInterruptedBoardingCrew.RemoveItem(crewEntity);
				m_aForcedBoardingRecoveryCrew.RemoveItem(crewEntity);
				reason = "server-authoritative compartment move-in completed";
			}
			else
				reason = "server-authoritative compartment move-in accepted";
			reason = reason + string.Format(
				" | bounded recovery %1 | ",
				boundedRecoveryAttempt) + localityEvidence
				+ string.Format(
					" | after getting in %1 in compartment %2 target vehicle %3 slot occupant %4",
					access.IsGettingIn(),
					access.IsInCompartment(),
					access.GetVehicle() == vehicleEntity,
					slot.GetOccupant() == crewEntity);
#ifdef ENABLE_DIAG
			Print("Partisan convoy seating debug | direct accepted | " + reason);
#endif
			return true;
		}
		if (nonReplicated)
		{
			reason = "non-replicated authority-local forced compartment move-in rejected | "
				+ localityEvidence;
#ifdef ENABLE_DIAG
			Print("Partisan convoy seating debug | direct rejected | " + reason);
#endif
			return false;
		}

		bool ownerRequestAccepted
			= access.MoveInVehicle(vehicleEntity, compartmentType, true, slot);
		if (ownerRequestAccepted)
		{
			reason = "owner compartment move-in request accepted without occupancy confirmation | "
				+ localityEvidence;
#ifdef ENABLE_DIAG
			Print("Partisan convoy seating debug | owner request accepted | " + reason);
#endif
			return true;
		}

		reason = "compartment move-in request was rejected | "
			+ localityEvidence
			+ string.Format(
				" | slot accessible %1 occupied %2 reserved %3 locked %4",
				slot.IsCompartmentAccessible(),
				slot.IsOccupied(),
				slot.IsReserved(),
				slot.IsGetInLockedFor(crewEntity));
#ifdef ENABLE_DIAG
		Print("Partisan convoy seating debug | all requests rejected | " + reason);
#endif
		return false;
	}

	protected void PruneBoardingRecoveryState()
	{
		for (int interruptedIndex = m_aInterruptedBoardingCrew.Count() - 1; interruptedIndex >= 0; interruptedIndex--)
		{
			IEntity interruptedCrew = m_aInterruptedBoardingCrew[interruptedIndex];
			if (interruptedCrew && !interruptedCrew.IsDeleted())
				continue;
			m_aInterruptedBoardingCrew.Remove(interruptedIndex);
		}
		for (int forcedIndex = m_aForcedBoardingRecoveryCrew.Count() - 1; forcedIndex >= 0; forcedIndex--)
		{
			IEntity forcedCrew = m_aForcedBoardingRecoveryCrew[forcedIndex];
			if (forcedCrew && !forcedCrew.IsDeleted())
				continue;
			m_aForcedBoardingRecoveryCrew.Remove(forcedIndex);
		}
	}

	protected bool IsSlotForCompartmentType(BaseCompartmentSlot slot, ECompartmentType compartmentType)
	{
		if (!slot)
			return false;

		if (compartmentType == ECompartmentType.PILOT)
			return IsDriverSlot(slot);

		return slot.GetType() == compartmentType;
	}

	protected bool IsSlotAvailableForCrew(BaseCompartmentSlot slot, IEntity crewEntity)
	{
		if (!slot || !crewEntity)
			return false;
		if (!slot.IsCompartmentAccessible())
			return false;
		if (slot.IsOccupied())
			return false;
		if (slot.IsReserved() && !slot.IsReservedBy(crewEntity))
			return false;
		if (slot.IsGetInLockedFor(crewEntity))
			return false;

		return true;
	}

	protected bool IsDriverSlot(BaseCompartmentSlot slot)
	{
		if (!slot)
			return false;

		return slot.IsPiloting() || slot.GetType() == ECompartmentType.PILOT;
	}

	protected bool IsCrewSeatedInVehicle(IEntity crewEntity, IEntity vehicleEntity)
	{
		if (!crewEntity || !vehicleEntity)
			return false;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(crewEntity.FindComponent(SCR_CompartmentAccessComponent));
		return access && access.IsInCompartment() && access.GetVehicle() == vehicleEntity;
	}

	protected bool IsCrewGettingIntoVehicle(IEntity crewEntity, IEntity vehicleEntity)
	{
		if (!crewEntity || !vehicleEntity)
			return false;

		SCR_CompartmentAccessComponent access = SCR_CompartmentAccessComponent.Cast(crewEntity.FindComponent(SCR_CompartmentAccessComponent));
		return access && access.IsGettingIn();
	}

	protected bool HasCrewBoardingVehicle(array<IEntity> livingCrew, IEntity vehicleEntity)
	{
		if (!livingCrew || !vehicleEntity)
			return false;

		foreach (IEntity crewEntity : livingCrew)
		{
			if (IsCrewGettingIntoVehicle(crewEntity, vehicleEntity))
				return true;
		}

		return false;
	}

	protected bool IsCrewEntity(array<IEntity> livingCrew, IEntity entity)
	{
		if (!livingCrew || !entity)
			return false;

		return livingCrew.Find(entity) >= 0;
	}

	protected string ResolveMissingSlotReason(ECompartmentType compartmentType)
	{
		if (compartmentType == ECompartmentType.PILOT)
			return "no accessible driver compartment is available";
		if (compartmentType == ECompartmentType.TURRET)
			return "no accessible turret compartment is available";
		if (compartmentType == ECompartmentType.CARGO)
			return "no accessible cargo compartment is available";

		return "no accessible compartment is available";
	}

	protected string ResolveCompartmentLabel(ECompartmentType compartmentType)
	{
		if (compartmentType == ECompartmentType.PILOT)
			return "driver compartment";
		if (compartmentType == ECompartmentType.TURRET)
			return "turret compartment";
		if (compartmentType == ECompartmentType.CARGO)
			return "cargo compartment";

		return "vehicle compartment";
	}

	protected IEntity SpawnRouteWaypoint(vector position, string groupId, int waypointIndex)
	{
		ResourceName waypointResource = CONVOY_WAYPOINT_PREFAB;
		Resource loaded = Resource.Load(waypointResource);
		if (!loaded || !loaded.IsValid())
			return null;

		vector waypointPosition = HST_WorldPositionService.ResolveSafeGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, true, 8.0);
		GenericEntity waypointEntity = HST_WorldPositionService.SpawnPrefab(CONVOY_WAYPOINT_PREFAB, waypointPosition, "0 0 0");
#ifdef ENABLE_DIAG
		ApplyCampaignDebugEntityName(waypointEntity, string.Format("convoy_waypoint_%1", waypointIndex), groupId);
#endif
		AIWaypoint waypoint = AIWaypoint.Cast(waypointEntity);
		if (!waypoint)
		{
			if (waypointEntity)
				SCR_EntityHelper.DeleteEntityAndChildren(waypointEntity);
			return null;
		}

		return waypointEntity;
	}

#ifdef ENABLE_DIAG
	protected void ApplyCampaignDebugEntityName(IEntity entity, string label, string sourceId)
	{
		if (!entity || sourceId.IsEmpty() || !sourceId.Contains(CAMPAIGN_DEBUG_PREFIX_ROOT))
			return;

		entity.SetName(CAMPAIGN_DEBUG_ENTITY_TAG + "_" + SanitizeCampaignDebugEntityToken(label) + "_" + SanitizeCampaignDebugEntityToken(sourceId));
	}

	protected string SanitizeCampaignDebugEntityToken(string value)
	{
		string safe = value;
		safe.Replace("/", "_");
		safe.Replace(":", "_");
		safe.Replace(" ", "_");
		safe.Replace("@", "_");
		safe.Replace(".", "_");
		if (safe.Length() > 160)
			return safe.Substring(0, 160);

		return safe;
	}
#endif

	protected void DeleteSpawnedWaypoints(array<IEntity> waypoints)
	{
		if (!waypoints)
			return;

		foreach (IEntity waypoint : waypoints)
		{
			if (waypoint)
				SCR_EntityHelper.DeleteEntityAndChildren(waypoint);
		}
	}

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity)
			return false;

		ChimeraCharacter character = ChimeraCharacter.Cast(entity);
		if (character)
		{
			CharacterControllerComponent controller = character.GetCharacterController();
			if (controller)
				return controller.GetLifeState() != ECharacterLifeState.DEAD;
		}

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}
}
