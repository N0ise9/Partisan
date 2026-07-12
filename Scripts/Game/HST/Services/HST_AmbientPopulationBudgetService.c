class HST_AmbientPopulationTownDemand
{
	string m_sZoneId;
	int m_iDesiredPedestrians;
	int m_iDesiredTraffic;
}

class HST_AmbientPopulationTownAllocation
{
	string m_sZoneId;
	int m_iDesiredPedestrians;
	int m_iDesiredTraffic;
	int m_iAllocatedPedestrians;
	int m_iAllocatedTraffic;

	int CountAllocatedActors()
	{
		return m_iAllocatedPedestrians + m_iAllocatedTraffic;
	}

	int CountUnmetPedestrians()
	{
		return Math.Max(0, m_iDesiredPedestrians - m_iAllocatedPedestrians);
	}

	int CountUnmetTraffic()
	{
		return Math.Max(0, m_iDesiredTraffic - m_iAllocatedTraffic);
	}
}

class HST_AmbientPopulationBudgetPlan
{
	int m_iTotalActorBudget;
	int m_iTrafficActorBudget;
	int m_iRotationEpoch;
	int m_iInputDemandCount;
	int m_iRejectedDemandCount;
	int m_iMergedDuplicateCount;
	int m_iAllocatedPedestrians;
	int m_iAllocatedTraffic;
	ref array<ref HST_AmbientPopulationTownAllocation> m_aTownAllocations = {};

	HST_AmbientPopulationTownAllocation Find(string zoneId)
	{
		if (zoneId.IsEmpty())
			return null;

		foreach (HST_AmbientPopulationTownAllocation allocation : m_aTownAllocations)
		{
			if (allocation && allocation.m_sZoneId == zoneId)
				return allocation;
		}
		return null;
	}

	int CountAllocatedActors()
	{
		// One traffic allocation means one driver. Vehicles are not actors, but
		// their drivers consume the same global actor budget as pedestrians.
		return m_iAllocatedPedestrians + m_iAllocatedTraffic;
	}

	bool IsWithinBudgets()
	{
		return m_iAllocatedPedestrians >= 0
			&& m_iAllocatedTraffic >= 0
			&& CountAllocatedActors() <= m_iTotalActorBudget
			&& m_iAllocatedTraffic <= m_iTrafficActorBudget;
	}

	string BuildReport()
	{
		string report = string.Format(
			"ambient population budget | towns %1 | actors %2/%3 | pedestrians %4 | traffic drivers %5/%6 | epoch %7",
			m_aTownAllocations.Count(),
			CountAllocatedActors(),
			m_iTotalActorBudget,
			m_iAllocatedPedestrians,
			m_iAllocatedTraffic,
			m_iTrafficActorBudget,
			m_iRotationEpoch);
		return report + string.Format(
			" | input/rejected/merged %1/%2/%3",
			m_iInputDemandCount,
			m_iRejectedDemandCount,
			m_iMergedDuplicateCount);
	}
}

// Pure transient allocator. It has no world or persistence dependencies: callers
// provide a complete demand snapshot and receive a deterministic immutable-by-
// convention plan for that snapshot.
class HST_AmbientPopulationBudgetService
{
	static const int MAX_CANONICAL_TOWNS = 256;
	static const int MAX_DESIRED_PER_KIND = 256;
	static const int MAX_CONNECTED_PLAYER_BUDGET_FACTOR = 32;

	int ResolveGlobalBudget(
		int baseBudget,
		int perPlayerBudget,
		int connectedPlayers,
		int warLevel,
		int warPenaltyPercent,
		int maximumBudget)
	{
		int boundedMaximum = Math.Max(0, maximumBudget);
		if (boundedMaximum == 0)
			return 0;

		int boundedBase = Math.Min(
			boundedMaximum,
			Math.Max(0, baseBudget));
		int boundedPerPlayer = Math.Min(
			boundedMaximum,
			Math.Max(0, perPlayerBudget));
		int boundedPlayers = Math.Min(
			MAX_CONNECTED_PLAYER_BUDGET_FACTOR,
			Math.Max(0, connectedPlayers));
		int rawBudget = Math.Min(
			boundedMaximum,
			boundedBase + boundedPerPlayer * boundedPlayers);

		int boundedWarLevel = Math.Min(32, Math.Max(1, warLevel));
		int boundedPenalty = Math.Min(100, Math.Max(0, warPenaltyPercent));
		int penaltyPercent = Math.Min(
			60,
			(boundedWarLevel - 1) * boundedPenalty);
		return rawBudget * (100 - penaltyPercent) / 100;
	}

	int ResolveLeaseEpoch(int elapsedSeconds, int leaseSeconds)
	{
		return Math.Max(0, elapsedSeconds) / Math.Max(1, leaseSeconds);
	}

	HST_AmbientPopulationBudgetPlan BuildPlan(
		array<ref HST_AmbientPopulationTownDemand> demands,
		int totalActorBudget,
		int trafficActorBudget,
		int rotationEpoch)
	{
		HST_AmbientPopulationBudgetPlan plan = new HST_AmbientPopulationBudgetPlan();
		plan.m_iTotalActorBudget = Math.Max(0, totalActorBudget);
		plan.m_iTrafficActorBudget = Math.Min(
			Math.Max(0, trafficActorBudget),
			plan.m_iTotalActorBudget);
		plan.m_iRotationEpoch = rotationEpoch;

		CanonicalizeDemands(demands, plan);
		if (plan.m_aTownAllocations.Count() == 0 || plan.m_iTotalActorBudget == 0)
			return plan;

		int startIndex = NormalizeRotation(
			rotationEpoch,
			plan.m_aTownAllocations.Count());

		// Floors deliberately admit pedestrians first. This gives every active
		// locality visible life before disposable traffic consumes scarce actors.
		AllocatePedestrianFloor(plan, startIndex);
		AllocateTrafficFloor(plan, startIndex);
		AllocateFairRemainder(plan, startIndex);
		return plan;
	}

	protected void CanonicalizeDemands(
		array<ref HST_AmbientPopulationTownDemand> demands,
		HST_AmbientPopulationBudgetPlan plan)
	{
		if (!plan || !demands)
			return;

		map<string, ref HST_AmbientPopulationTownDemand> mergedByZone
			= new map<string, ref HST_AmbientPopulationTownDemand>();
		array<string> zoneIds = {};
		foreach (HST_AmbientPopulationTownDemand demand : demands)
		{
			plan.m_iInputDemandCount++;
			if (!demand)
			{
				plan.m_iRejectedDemandCount++;
				continue;
			}

			string zoneId = demand.m_sZoneId.Trim();
			int desiredPedestrians = NormalizeDesired(demand.m_iDesiredPedestrians);
			int desiredTraffic = NormalizeDesired(demand.m_iDesiredTraffic);
			if (zoneId.IsEmpty() || (desiredPedestrians == 0 && desiredTraffic == 0))
			{
				plan.m_iRejectedDemandCount++;
				continue;
			}

			HST_AmbientPopulationTownDemand merged;
			if (mergedByZone.Find(zoneId, merged))
			{
				// Max-merge makes accidental duplicate rows idempotent instead of
				// allowing them to inflate population demand.
				merged.m_iDesiredPedestrians = Math.Max(
					merged.m_iDesiredPedestrians,
					desiredPedestrians);
				merged.m_iDesiredTraffic = Math.Max(
					merged.m_iDesiredTraffic,
					desiredTraffic);
				plan.m_iMergedDuplicateCount++;
				continue;
			}

			merged = new HST_AmbientPopulationTownDemand();
			merged.m_sZoneId = zoneId;
			merged.m_iDesiredPedestrians = desiredPedestrians;
			merged.m_iDesiredTraffic = desiredTraffic;
			mergedByZone.Set(zoneId, merged);
			zoneIds.Insert(zoneId);
		}

		zoneIds.Sort();
		foreach (string canonicalZoneId : zoneIds)
		{
			if (plan.m_aTownAllocations.Count() >= MAX_CANONICAL_TOWNS)
			{
				// The lexicographically first bounded set wins regardless of
				// caller iteration order.
				plan.m_iRejectedDemandCount++;
				continue;
			}
			HST_AmbientPopulationTownDemand canonicalDemand
				= mergedByZone.Get(canonicalZoneId);
			if (!canonicalDemand)
				continue;

			HST_AmbientPopulationTownAllocation allocation
				= new HST_AmbientPopulationTownAllocation();
			allocation.m_sZoneId = canonicalDemand.m_sZoneId;
			allocation.m_iDesiredPedestrians
				= canonicalDemand.m_iDesiredPedestrians;
			allocation.m_iDesiredTraffic = canonicalDemand.m_iDesiredTraffic;
			plan.m_aTownAllocations.Insert(allocation);
		}
	}

	protected int NormalizeDesired(int desired)
	{
		return Math.Min(Math.Max(0, desired), MAX_DESIRED_PER_KIND);
	}

	protected int NormalizeRotation(int rotationEpoch, int count)
	{
		if (count <= 0)
			return 0;
		int normalized = rotationEpoch % count;
		if (normalized < 0)
			normalized += count;
		return normalized;
	}

	protected void AllocatePedestrianFloor(
		HST_AmbientPopulationBudgetPlan plan,
		int startIndex)
	{
		int townCount = plan.m_aTownAllocations.Count();
		for (int offset; offset < townCount; offset++)
		{
			if (!HasActorCapacity(plan))
				return;
			HST_AmbientPopulationTownAllocation allocation
				= plan.m_aTownAllocations[(startIndex + offset) % townCount];
			if (allocation && allocation.m_iDesiredPedestrians > 0)
				AllocatePedestrian(plan, allocation);
		}
	}

	protected void AllocateTrafficFloor(
		HST_AmbientPopulationBudgetPlan plan,
		int startIndex)
	{
		int townCount = plan.m_aTownAllocations.Count();
		for (int offset; offset < townCount; offset++)
		{
			if (!HasActorCapacity(plan) || !HasTrafficCapacity(plan))
				return;
			HST_AmbientPopulationTownAllocation allocation
				= plan.m_aTownAllocations[(startIndex + offset) % townCount];
			if (allocation && allocation.m_iDesiredTraffic > 0)
				AllocateTraffic(plan, allocation);
		}
	}

	protected void AllocateFairRemainder(
		HST_AmbientPopulationBudgetPlan plan,
		int startIndex)
	{
		int townCount = plan.m_aTownAllocations.Count();
		int round;
		while (HasActorCapacity(plan))
		{
			bool progressed;
			int roundStart = (startIndex + round) % townCount;
			for (int offset; offset < townCount; offset++)
			{
				if (!HasActorCapacity(plan))
					return;
				HST_AmbientPopulationTownAllocation allocation
					= plan.m_aTownAllocations[(roundStart + offset) % townCount];
				if (!allocation)
					continue;

				bool pedestrianUnmet = allocation.CountUnmetPedestrians() > 0;
				bool trafficUnmet = allocation.CountUnmetTraffic() > 0
					&& HasTrafficCapacity(plan);
				if (!pedestrianUnmet && !trafficUnmet)
					continue;

				// At most one actor per town per round. Alternating the preferred
				// kind prevents a large pedestrian or traffic request from
				// monopolizing the shared actor remainder.
				bool preferTraffic = ((round + offset + startIndex) % 2) == 0;
				if (trafficUnmet && (preferTraffic || !pedestrianUnmet))
					AllocateTraffic(plan, allocation);
				else
					AllocatePedestrian(plan, allocation);
				progressed = true;
			}

			if (!progressed)
				return;
			round++;
		}
	}

	protected bool HasActorCapacity(HST_AmbientPopulationBudgetPlan plan)
	{
		return plan && plan.CountAllocatedActors() < plan.m_iTotalActorBudget;
	}

	protected bool HasTrafficCapacity(HST_AmbientPopulationBudgetPlan plan)
	{
		return plan && plan.m_iAllocatedTraffic < plan.m_iTrafficActorBudget;
	}

	protected bool AllocatePedestrian(
		HST_AmbientPopulationBudgetPlan plan,
		HST_AmbientPopulationTownAllocation allocation)
	{
		if (!HasActorCapacity(plan) || !allocation
			|| allocation.m_iAllocatedPedestrians
				>= allocation.m_iDesiredPedestrians)
			return false;
		allocation.m_iAllocatedPedestrians++;
		plan.m_iAllocatedPedestrians++;
		return true;
	}

	protected bool AllocateTraffic(
		HST_AmbientPopulationBudgetPlan plan,
		HST_AmbientPopulationTownAllocation allocation)
	{
		if (!HasActorCapacity(plan) || !HasTrafficCapacity(plan) || !allocation
			|| allocation.m_iAllocatedTraffic >= allocation.m_iDesiredTraffic)
			return false;
		allocation.m_iAllocatedTraffic++;
		plan.m_iAllocatedTraffic++;
		return true;
	}
}
