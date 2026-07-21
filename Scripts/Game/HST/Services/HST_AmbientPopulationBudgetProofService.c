#ifdef ENABLE_DIAG
class HST_AmbientPopulationBudgetProofReport
{
	bool m_bTenTownBoundedExact;
	bool m_bTrafficDriversInTotalExact;
	bool m_bRotationFairnessExact;
	bool m_bZeroBudgetExact;
	bool m_bDuplicateInputDeterminismExact;
	bool m_bDemandCapsExact;
	bool m_bGlobalBudgetBoundsExact;
	bool m_bLeaseStabilityExact;
	string m_sTenTownBoundedEvidence;
	string m_sTrafficDriversInTotalEvidence;
	string m_sRotationFairnessEvidence;
	string m_sZeroBudgetEvidence;
	string m_sDuplicateInputDeterminismEvidence;
	string m_sDemandCapsEvidence;
	string m_sGlobalBudgetBoundsEvidence;
	string m_sLeaseStabilityEvidence;

	bool AllExact()
	{
		return m_bTenTownBoundedExact
			&& m_bTrafficDriversInTotalExact
			&& m_bRotationFairnessExact
			&& m_bZeroBudgetExact
			&& m_bDuplicateInputDeterminismExact
			&& m_bDemandCapsExact
			&& m_bGlobalBudgetBoundsExact
			&& m_bLeaseStabilityExact;
	}

	string BuildReport()
	{
		string report = string.Format(
			"ambient population budget proof | all exact %1 | ten-town bounded %2 | drivers total %3 | rotation %4 | zero %5 | duplicate deterministic %6 | demand caps %7",
			AllExact(),
			m_bTenTownBoundedExact,
			m_bTrafficDriversInTotalExact,
			m_bRotationFairnessExact,
			m_bZeroBudgetExact,
			m_bDuplicateInputDeterminismExact,
			m_bDemandCapsExact);
		return report + string.Format(
			" | global bounds %1 | lease stability %2",
			m_bGlobalBudgetBoundsExact,
			m_bLeaseStabilityExact);
	}
}

class HST_AmbientPopulationBudgetProofService
{
	protected ref HST_AmbientPopulationBudgetService m_Service
		= new HST_AmbientPopulationBudgetService();

	HST_AmbientPopulationBudgetProofReport Run()
	{
		HST_AmbientPopulationBudgetProofReport report
			= new HST_AmbientPopulationBudgetProofReport();
		ProveTenTownBoundedness(report);
		ProveTrafficDriversConsumeTotalBudget(report);
		ProveRotationFairness(report);
		ProveZeroBudget(report);
		ProveDuplicateInputDeterminism(report);
		ProveDemandCaps(report);
		ProveGlobalBudgetBounds(report);
		ProveLeaseStability(report);
		return report;
	}

	protected void ProveTenTownBoundedness(
		HST_AmbientPopulationBudgetProofReport report)
	{
		array<ref HST_AmbientPopulationTownDemand> demands = {};
		for (int index; index < 10; index++)
		{
			demands.Insert(BuildDemand(
				string.Format("town_%1", index),
				6,
				2));
		}

		HST_AmbientPopulationBudgetPlan plan
			= m_Service.BuildPlan(demands, 27, 7, 3);
		bool townCapsExact = TownCapsExact(plan);
		report.m_bTenTownBoundedExact = plan
			&& plan.m_aTownAllocations.Count() == 10
			&& plan.CountAllocatedActors() == 27
			&& plan.m_iAllocatedTraffic == 7
			&& plan.IsWithinBudgets()
			&& townCapsExact
			&& CountTownsWithPedestrians(plan) == 10;
		report.m_sTenTownBoundedEvidence = string.Format(
			"towns %1 | actors %2/27 | traffic %3/7 | pedestrian towns %4 | caps %5",
			ResolveTownCount(plan),
			ResolveActorCount(plan),
			ResolveTrafficCount(plan),
			CountTownsWithPedestrians(plan),
			townCapsExact);
	}

	protected void ProveTrafficDriversConsumeTotalBudget(
		HST_AmbientPopulationBudgetProofReport report)
	{
		array<ref HST_AmbientPopulationTownDemand> demands = {
			BuildDemand("driver_town", 1, 10)
		};
		HST_AmbientPopulationBudgetPlan plan
			= m_Service.BuildPlan(demands, 3, 10, 0);
		HST_AmbientPopulationTownAllocation allocation;
		if (plan)
			allocation = plan.Find("driver_town");
		report.m_bTrafficDriversInTotalExact = plan && allocation
			&& plan.m_iTotalActorBudget == 3
			&& plan.m_iTrafficActorBudget == 3
			&& allocation.m_iAllocatedPedestrians == 1
			&& allocation.m_iAllocatedTraffic == 2
			&& plan.CountAllocatedActors() == 3
			&& plan.IsWithinBudgets();
		report.m_sTrafficDriversInTotalEvidence = string.Format(
			"pedestrians/traffic %1/%2 | actors %3/3 | normalized traffic budget %4",
			allocation && allocation.m_iAllocatedPedestrians,
			allocation && allocation.m_iAllocatedTraffic,
			ResolveActorCount(plan),
			plan && plan.m_iTrafficActorBudget);
	}

	protected void ProveRotationFairness(
		HST_AmbientPopulationBudgetProofReport report)
	{
		array<ref HST_AmbientPopulationTownDemand> demands = {
			BuildDemand("town_delta", 1, 0),
			BuildDemand("town_bravo", 1, 0),
			BuildDemand("town_alpha", 1, 0),
			BuildDemand("town_charlie", 1, 0)
		};
		array<string> winners = {};
		for (int epoch; epoch < 4; epoch++)
		{
			HST_AmbientPopulationBudgetPlan plan
				= m_Service.BuildPlan(demands, 1, 0, epoch);
			winners.Insert(FindPedestrianWinner(plan));
		}

		report.m_bRotationFairnessExact = winners.Count() == 4
			&& winners[0] == "town_alpha"
			&& winners[1] == "town_bravo"
			&& winners[2] == "town_charlie"
			&& winners[3] == "town_delta";
		report.m_sRotationFairnessEvidence = string.Format(
			"epoch winners %1/%2/%3/%4",
			ResolveString(winners, 0),
			ResolveString(winners, 1),
			ResolveString(winners, 2),
			ResolveString(winners, 3));
	}

	protected void ProveZeroBudget(
		HST_AmbientPopulationBudgetProofReport report)
	{
		array<ref HST_AmbientPopulationTownDemand> demands = {
			BuildDemand("zero_alpha", 5, 5),
			BuildDemand("zero_bravo", 5, 5)
		};
		HST_AmbientPopulationBudgetPlan zero
			= m_Service.BuildPlan(demands, 0, 8, 9);
		HST_AmbientPopulationBudgetPlan negative
			= m_Service.BuildPlan(demands, -5, -8, -9);
		report.m_bZeroBudgetExact = zero && negative
			&& zero.m_iTotalActorBudget == 0
			&& zero.m_iTrafficActorBudget == 0
			&& zero.CountAllocatedActors() == 0
			&& negative.m_iTotalActorBudget == 0
			&& negative.m_iTrafficActorBudget == 0
			&& negative.CountAllocatedActors() == 0;
		report.m_sZeroBudgetEvidence = string.Format(
			"zero actors/traffic %1/%2 | negative actors/traffic %3/%4",
			ResolveActorCount(zero),
			ResolveTrafficCount(zero),
			ResolveActorCount(negative),
			ResolveTrafficCount(negative));
	}

	protected void ProveDuplicateInputDeterminism(
		HST_AmbientPopulationBudgetProofReport report)
	{
		HST_AmbientPopulationTownDemand empty = BuildDemand("   ", 9, 9);
		HST_AmbientPopulationTownDemand negative
			= BuildDemand("invalid_negative", -3, -7);
		array<ref HST_AmbientPopulationTownDemand> first = {
			BuildDemand("duplicate", 4, 1),
			null,
			BuildDemand("stable", 2, 2),
			BuildDemand("duplicate", 2, 3),
			empty,
			negative
		};
		array<ref HST_AmbientPopulationTownDemand> second = {
			negative,
			BuildDemand("duplicate", 2, 3),
			BuildDemand("stable", 2, 2),
			empty,
			BuildDemand("duplicate", 4, 1),
			null
		};

		HST_AmbientPopulationBudgetPlan firstPlan
			= m_Service.BuildPlan(first, 7, 3, 2);
		HST_AmbientPopulationBudgetPlan secondPlan
			= m_Service.BuildPlan(second, 7, 3, 2);
		HST_AmbientPopulationTownAllocation duplicate;
		if (firstPlan)
			duplicate = firstPlan.Find("duplicate");
		report.m_bDuplicateInputDeterminismExact
			= PlansEqual(firstPlan, secondPlan)
			&& firstPlan.m_aTownAllocations.Count() == 2
			&& firstPlan.m_iMergedDuplicateCount == 1
			&& firstPlan.m_iRejectedDemandCount == 3
			&& duplicate
			&& duplicate.m_iDesiredPedestrians == 4
			&& duplicate.m_iDesiredTraffic == 3;
		report.m_sDuplicateInputDeterminismEvidence = string.Format(
			"plans equal %1 | towns %2 | rejected/merged %3/%4 | merged demand %5/%6",
			PlansEqual(firstPlan, secondPlan),
			ResolveTownCount(firstPlan),
			firstPlan && firstPlan.m_iRejectedDemandCount,
			firstPlan && firstPlan.m_iMergedDuplicateCount,
			duplicate && duplicate.m_iDesiredPedestrians,
			duplicate && duplicate.m_iDesiredTraffic);
	}

	protected void ProveDemandCaps(
		HST_AmbientPopulationBudgetProofReport report)
	{
		array<ref HST_AmbientPopulationTownDemand> demands = {
			BuildDemand("caps_alpha", 2, 1),
			BuildDemand("caps_bravo", 1, 0)
		};
		HST_AmbientPopulationBudgetPlan plan
			= m_Service.BuildPlan(demands, 100, 100, 0);
		HST_AmbientPopulationTownAllocation alpha;
		HST_AmbientPopulationTownAllocation bravo;
		if (plan)
		{
			alpha = plan.Find("caps_alpha");
			bravo = plan.Find("caps_bravo");
		}
		report.m_bDemandCapsExact = plan && alpha && bravo
			&& plan.CountAllocatedActors() == 4
			&& plan.m_iAllocatedPedestrians == 3
			&& plan.m_iAllocatedTraffic == 1
			&& alpha.m_iAllocatedPedestrians == 2
			&& alpha.m_iAllocatedTraffic == 1
			&& bravo.m_iAllocatedPedestrians == 1
			&& bravo.m_iAllocatedTraffic == 0
			&& TownCapsExact(plan);
		report.m_sDemandCapsEvidence = string.Format(
			"actors/pedestrians/traffic %1/%2/%3 | caps %4",
			ResolveActorCount(plan),
			plan && plan.m_iAllocatedPedestrians,
			ResolveTrafficCount(plan),
			TownCapsExact(plan));
	}

	protected void ProveGlobalBudgetBounds(
		HST_AmbientPopulationBudgetProofReport report)
	{
		int noPlayer = m_Service.ResolveGlobalBudget(48, 12, 0, 1, 4, 256);
		int onePlayer = m_Service.ResolveGlobalBudget(48, 12, 1, 1, 4, 256);
		int fourPlayerWarFive = m_Service.ResolveGlobalBudget(48, 12, 4, 5, 4, 256);
		int trafficWarTen = m_Service.ResolveGlobalBudget(10, 2, 1, 10, 4, 64);
		int hostileMaximum = m_Service.ResolveGlobalBudget(2147483647, 2147483647, 2147483647, -2147483647, 2147483647, 256);
		int negative = m_Service.ResolveGlobalBudget(-1, -1, -1, -1, -1, 256);
		report.m_bGlobalBudgetBoundsExact = noPlayer == 48
			&& onePlayer == 60
			&& fourPlayerWarFive == 80
			&& trafficWarTen == 7
			&& hostileMaximum == 256
			&& negative == 0;
		report.m_sGlobalBudgetBoundsEvidence = string.Format(
			"actor no-player/one/four-war5 %1/%2/%3 | traffic war10 %4 | hostile/negative %5/%6",
			noPlayer,
			onePlayer,
			fourPlayerWarFive,
			trafficWarTen,
			hostileMaximum,
			negative);
	}

	protected void ProveLeaseStability(
		HST_AmbientPopulationBudgetProofReport report)
	{
		int epoch0 = m_Service.ResolveLeaseEpoch(0, 120);
		int epoch119 = m_Service.ResolveLeaseEpoch(119, 120);
		int epoch120 = m_Service.ResolveLeaseEpoch(120, 120);
		int epoch239 = m_Service.ResolveLeaseEpoch(239, 120);
		int epoch240 = m_Service.ResolveLeaseEpoch(240, 120);
		int invalid = m_Service.ResolveLeaseEpoch(-100, 0);
		report.m_bLeaseStabilityExact = epoch0 == 0
			&& epoch119 == 0
			&& epoch120 == 1
			&& epoch239 == 1
			&& epoch240 == 2
			&& invalid == 0;
		report.m_sLeaseStabilityEvidence = string.Format(
			"epochs 0/119/120/239/240 %1/%2/%3/%4/%5 | invalid %6",
			epoch0,
			epoch119,
			epoch120,
			epoch239,
			epoch240,
			invalid);
	}

	protected HST_AmbientPopulationTownDemand BuildDemand(
		string zoneId,
		int desiredPedestrians,
		int desiredTraffic)
	{
		HST_AmbientPopulationTownDemand demand
			= new HST_AmbientPopulationTownDemand();
		demand.m_sZoneId = zoneId;
		demand.m_iDesiredPedestrians = desiredPedestrians;
		demand.m_iDesiredTraffic = desiredTraffic;
		return demand;
	}

	protected bool TownCapsExact(HST_AmbientPopulationBudgetPlan plan)
	{
		if (!plan || !plan.IsWithinBudgets())
			return false;
		foreach (HST_AmbientPopulationTownAllocation allocation : plan.m_aTownAllocations)
		{
			if (!allocation
				|| allocation.m_iAllocatedPedestrians < 0
				|| allocation.m_iAllocatedTraffic < 0
				|| allocation.m_iAllocatedPedestrians
					> allocation.m_iDesiredPedestrians
				|| allocation.m_iAllocatedTraffic
					> allocation.m_iDesiredTraffic)
				return false;
		}
		return true;
	}

	protected int CountTownsWithPedestrians(
		HST_AmbientPopulationBudgetPlan plan)
	{
		if (!plan)
			return 0;
		int count;
		foreach (HST_AmbientPopulationTownAllocation allocation : plan.m_aTownAllocations)
		{
			if (allocation && allocation.m_iAllocatedPedestrians > 0)
				count++;
		}
		return count;
	}

	protected string FindPedestrianWinner(
		HST_AmbientPopulationBudgetPlan plan)
	{
		if (!plan)
			return "";
		foreach (HST_AmbientPopulationTownAllocation allocation : plan.m_aTownAllocations)
		{
			if (allocation && allocation.m_iAllocatedPedestrians == 1)
				return allocation.m_sZoneId;
		}
		return "";
	}

	protected bool PlansEqual(
		HST_AmbientPopulationBudgetPlan left,
		HST_AmbientPopulationBudgetPlan right)
	{
		if (!left || !right)
			return false;
		if (left.m_iTotalActorBudget != right.m_iTotalActorBudget
			|| left.m_iTrafficActorBudget != right.m_iTrafficActorBudget
			|| left.m_iRotationEpoch != right.m_iRotationEpoch)
			return false;
		if (left.m_iRejectedDemandCount != right.m_iRejectedDemandCount
			|| left.m_iMergedDuplicateCount != right.m_iMergedDuplicateCount)
			return false;
		if (left.m_iAllocatedPedestrians != right.m_iAllocatedPedestrians
			|| left.m_iAllocatedTraffic != right.m_iAllocatedTraffic)
			return false;
		if (left.m_aTownAllocations.Count() != right.m_aTownAllocations.Count())
			return false;

		for (int index; index < left.m_aTownAllocations.Count(); index++)
		{
			HST_AmbientPopulationTownAllocation leftAllocation
				= left.m_aTownAllocations[index];
			HST_AmbientPopulationTownAllocation rightAllocation
				= right.m_aTownAllocations[index];
			if (!leftAllocation || !rightAllocation)
				return false;
			if (leftAllocation.m_sZoneId != rightAllocation.m_sZoneId
				|| leftAllocation.m_iDesiredPedestrians != rightAllocation.m_iDesiredPedestrians
				|| leftAllocation.m_iDesiredTraffic != rightAllocation.m_iDesiredTraffic)
				return false;
			if (leftAllocation.m_iAllocatedPedestrians != rightAllocation.m_iAllocatedPedestrians
				|| leftAllocation.m_iAllocatedTraffic != rightAllocation.m_iAllocatedTraffic)
				return false;
		}
		return true;
	}

	protected int ResolveTownCount(HST_AmbientPopulationBudgetPlan plan)
	{
		if (!plan)
			return -1;
		return plan.m_aTownAllocations.Count();
	}

	protected int ResolveActorCount(HST_AmbientPopulationBudgetPlan plan)
	{
		if (!plan)
			return -1;
		return plan.CountAllocatedActors();
	}

	protected int ResolveTrafficCount(HST_AmbientPopulationBudgetPlan plan)
	{
		if (!plan)
			return -1;
		return plan.m_iAllocatedTraffic;
	}

	protected string ResolveString(array<string> values, int index)
	{
		if (!values || index < 0 || index >= values.Count())
			return "missing";
		return values[index];
	}
}
#endif
