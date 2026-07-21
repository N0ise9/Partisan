[BaseContainerProps()]
class HST_CombatPresenceContribution
{
	string m_sContributorId;
	string m_sGroupId;
	string m_sOperationId;
	string m_sFactionKey;
	string m_sKind;
	string m_sFact;
	vector m_vPosition;
	int m_iInfantryCount;
	int m_iMannedVehicleCount;
	int m_iStaticOperatorCount;
	int m_iCurrentOperationCount;
	int m_iRecentFireCount;
	bool m_bAuthoritativePhysicalSample;

	int GetLivingContributorCount()
	{
		return Math.Max(0, m_iInfantryCount)
			+ Math.Max(0, m_iMannedVehicleCount)
			+ Math.Max(0, m_iStaticOperatorCount);
	}
}

[BaseContainerProps()]
class HST_CombatPresenceAuthorityGap
{
	string m_sGroupId;
	string m_sFactionKey;
	vector m_vPosition;
	string m_sReason;
}

[BaseContainerProps()]
class HST_CombatPresenceResult
{
	bool m_bQueryValid;
	bool m_bHasLiveContributors;
	bool m_bBlocksProgress;
	bool m_bPersistedCoolingApplied;
	string m_sObserverFactionKey;
	string m_sExactFactionKey;
	string m_sZoneId;
	vector m_vCenter;
	float m_fRadiusMeters;
	HST_ECombatPresenceState m_eState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COLD;
	int m_iInfantryCount;
	int m_iMannedVehicleCount;
	int m_iStaticOperatorCount;
	int m_iCurrentOperationCount;
	int m_iRecentFireCount;
	int m_iContributorCount;
	int m_iOmittedContributorCount;
	int m_iUnresolvedAuthorityCount;
	int m_iCoolingRemainingSeconds;
	string m_sContributorHash;
	string m_sReason = "cold";
	ref array<ref HST_CombatPresenceContribution> m_aContributors = {};

	int GetLivingContributorCount()
	{
		return Math.Max(0, m_iInfantryCount)
			+ Math.Max(0, m_iMannedVehicleCount)
			+ Math.Max(0, m_iStaticOperatorCount);
	}
}

// Canonical, state-only combat-presence authority. Native runtime inspection is
// intentionally performed elsewhere and published onto HST_ActiveGroupState as
// a short-lived authoritative sample. This service never scans the world, so
// civilian traffic, props, markers, and unrelated faction-tagged entities cannot
// become combat contributors.
class HST_CombatPresenceService
{
	static const int MAX_PERSISTED_CONTRIBUTORS = 24;
	static const int MAX_SAMPLE_AGE_SECONDS = 2;
	static const int DEFAULT_COOLING_SECONDS = 30;
	static const int DEFAULT_ZONE_RADIUS_METERS = 180;
	static const int MAX_DIAGNOSTIC_FACT_CHARACTERS = 192;
#ifdef ENABLE_DIAG
	static const string NON_GAMEPLAY_SMOKE_GROUP_TOKEN = "hst_smoke";
#endif

	protected int m_iCoolingSeconds = DEFAULT_COOLING_SECONDS;
	protected HST_CampaignState m_CachedState;
	protected int m_iContributionCacheSecond = -1;
	protected ref array<ref HST_CombatPresenceContribution> m_aContributionCache = {};
	protected ref array<ref HST_CombatPresenceAuthorityGap> m_aAuthorityGapCache = {};
	protected ref array<ref HST_CombatPresenceContribution> m_aHostileContributionScratch = {};
	protected ref map<string, ref HST_CombatPresenceResult> m_mZoneHostileQueryCache
		= new map<string, ref HST_CombatPresenceResult>();
	protected ref map<string, ref HST_CombatPresenceResult> m_mZoneExactQueryCache
		= new map<string, ref HST_CombatPresenceResult>();

	void Configure(HST_BalanceConfig balance)
	{
		int resolvedCoolingSeconds = ResolveCoolingSeconds(balance);
		if (m_iCoolingSeconds == resolvedCoolingSeconds)
			return;
		m_iCoolingSeconds = resolvedCoolingSeconds;
		InvalidateCache();
	}

	void InvalidateCache()
	{
		m_CachedState = null;
		m_iContributionCacheSecond = -1;
		m_aContributionCache.Clear();
		m_aAuthorityGapCache.Clear();
		m_aHostileContributionScratch.Clear();
		ClearZoneQueryCaches();
	}

	static bool IsGroupCombatPresent(HST_CampaignState state, HST_ActiveGroupState group)
	{
		HST_CombatPresenceService service = new HST_CombatPresenceService();
		int nowSecond;
		if (state)
			nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_CombatPresenceContribution contribution = service.BuildGroupContribution(
			state,
			group,
			nowSecond);
		return contribution && contribution.GetLivingContributorCount() > 0;
	}

	// Returns false only when a spawned operational group's physical authority is
	// unresolved. A true return with combatEffective=false is authoritative zero
	// or terminal/ineligible evidence and may be used for terminal decisions.
	bool TryResolveGroupCombatPresence(
		HST_CampaignState state,
		HST_ActiveGroupState group,
		out bool combatEffective,
		out string reason)
	{
		combatEffective = false;
		reason = "combat-presence context missing";
		if (!state || !group)
			return false;

		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_CombatPresenceContribution contribution = BuildGroupContribution(
			state,
			group,
			nowSecond);
		if (contribution)
		{
			combatEffective = contribution.GetLivingContributorCount() > 0;
			reason = contribution.m_sFact;
			return true;
		}

		HST_CombatPresenceAuthorityGap authorityGap = BuildPhysicalAuthorityGap(
			state,
			group,
			nowSecond);
		if (authorityGap)
		{
			reason = authorityGap.m_sReason;
			return false;
		}

		reason = "authoritative zero or terminal combat presence";
		return true;
	}

	HST_CombatPresenceResult QueryHostilePresenceNear(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string observerFactionKey,
		vector center,
		float radiusMeters)
	{
		return QueryPresenceNear(
			state,
			preset,
			observerFactionKey,
			"",
			center,
			radiusMeters,
			true);
	}

	HST_CombatPresenceResult QueryExactFactionPresenceNear(
		HST_CampaignState state,
		string factionKey,
		vector center,
		float radiusMeters)
	{
		return QueryPresenceNear(
			state,
			null,
			"",
			factionKey,
			center,
			radiusMeters,
			false);
	}

	// Allocation-free preflight for hot loops that only need to know whether a
	// full result could contain a living contributor. Authority gaps are still
	// resolved by the full query whenever gameplay needs a decision.
	bool HasHostileContributionNear(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string observerFactionKey,
		vector center,
		float radiusMeters)
	{
		if (!state || !preset || observerFactionKey.IsEmpty() || radiusMeters < 0.0)
			return false;
		EnsureContributionCache(state);
		float radiusSq = radiusMeters * radiusMeters;
		foreach (HST_CombatPresenceContribution contribution : m_aContributionCache)
		{
			if (!contribution)
				continue;
			string relation = HST_FactionRelationService.ResolveRelation(
				preset,
				observerFactionKey,
				contribution.m_sFactionKey);
			if (!HST_FactionRelationService.IsHostileRelation(relation))
				continue;
			if (DistanceSq2D(contribution.m_vPosition, center) <= radiusSq)
				return true;
		}
		return false;
	}

	bool HasExactFactionContributionNear(
		HST_CampaignState state,
		string factionKey,
		vector center,
		float radiusMeters)
	{
		if (!state || factionKey.IsEmpty() || radiusMeters < 0.0)
			return false;
		EnsureContributionCache(state);
		float radiusSq = radiusMeters * radiusMeters;
		foreach (HST_CombatPresenceContribution contribution : m_aContributionCache)
		{
			if (!contribution || contribution.m_sFactionKey != factionKey)
				continue;
			if (DistanceSq2D(contribution.m_vPosition, center) <= radiusSq)
				return true;
		}
		return false;
	}

	HST_CombatPresenceResult QueryZoneHostilePresence(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string observerFactionKey,
		HST_ZoneState zone,
		bool includePersistedCooling)
	{
		if (!zone)
			return BuildInvalidResult(observerFactionKey, "", "zone is missing");

		float radius = ResolveZoneRadius(zone);
		HST_CombatPresenceResult result;
		string hostileAuthorityKey = observerFactionKey + "@" + BuildPresetCacheIdentity(preset);
		string cacheKey = BuildZoneQueryCacheKey("hostile", hostileAuthorityKey, zone, radius);
		if (!cacheKey.IsEmpty())
		{
			EnsureContributionCache(state);
			result = m_mZoneHostileQueryCache.Get(cacheKey);
		}
		if (!result)
		{
			result = QueryHostilePresenceNear(
				state,
				preset,
				observerFactionKey,
				zone.m_vPosition,
				radius);
			result.m_sZoneId = zone.m_sZoneId;
			if (!cacheKey.IsEmpty())
				m_mZoneHostileQueryCache.Set(cacheKey, result);
		}
		if (!result.m_bQueryValid)
			return result;
		if (result.m_bHasLiveContributors || !includePersistedCooling || !state)
			return result;
		// MissionRuntime can query before this tick's heat transition. Preserve the
		// previous HOT authority for that single boundary so the disappearance of
		// the last contributor cannot create a clear-area fail-open before
		// TickAllZoneHeat starts the cooling deadline.
		if (zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT)
		{
			HST_CombatPresenceResult hotGuardResult = CloneResult(result);
			hotGuardResult.m_eState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT;
			hotGuardResult.m_bBlocksProgress = true;
			hotGuardResult.m_sContributorHash = zone.m_sCombatPresenceContributorHash;
			hotGuardResult.m_sReason = "combat area awaiting cooling transition";
			return hotGuardResult;
		}
		if (zone.m_eCombatPresenceState != HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING)
			return result;
		if (zone.m_iCombatPresenceCoolingUntilSecond <= state.m_iElapsedSeconds)
			return result;

		HST_CombatPresenceResult coolingResult = CloneResult(result);
		ApplyPersistedCooling(coolingResult, zone, state.m_iElapsedSeconds);
		return coolingResult;
	}

	HST_CombatPresenceResult QueryZoneExactFactionPresence(
		HST_CampaignState state,
		string factionKey,
		HST_ZoneState zone)
	{
		if (!zone)
			return BuildInvalidResult("", factionKey, "zone is missing");
		float radius = ResolveZoneRadius(zone);
		string cacheKey = BuildZoneQueryCacheKey("exact", factionKey, zone, radius);
		HST_CombatPresenceResult result;
		if (!cacheKey.IsEmpty())
		{
			EnsureContributionCache(state);
			result = m_mZoneExactQueryCache.Get(cacheKey);
		}
		if (!result)
		{
			result = QueryExactFactionPresenceNear(
				state,
				factionKey,
				zone.m_vPosition,
				radius);
			result.m_sZoneId = zone.m_sZoneId;
			if (!cacheKey.IsEmpty())
				m_mZoneExactQueryCache.Set(cacheKey, result);
		}
		return result;
	}

	bool TickAllZoneHeat(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		string observerFactionKey)
	{
		if (!state || !preset || observerFactionKey.IsEmpty())
			return false;
		Configure(balance);
		EnsureContributionCache(state);
		m_aHostileContributionScratch.Clear();
		foreach (HST_CombatPresenceContribution contribution : m_aContributionCache)
		{
			if (!contribution)
				continue;
			string relation = HST_FactionRelationService.ResolveRelation(
				preset,
				observerFactionKey,
				contribution.m_sFactionKey);
			if (HST_FactionRelationService.IsHostileRelation(relation))
				m_aHostileContributionScratch.Insert(contribution);
		}
		bool changed;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone)
				continue;
			// A canonical COLD zone with no possible hostile contributor needs no
			// result DTO, diagnostic map, sort, or full contribution scan. HOT and
			// COOLING zones always advance so their deadline remains authoritative.
			if (zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COLD
				&& !HasCachedContributionNear(
					m_aHostileContributionScratch,
					zone.m_vPosition,
					ResolveZoneRadius(zone)))
				continue;
			if (TickZoneHeat(state, preset, balance, zone, observerFactionKey))
				changed = true;
		}
		return changed;
	}

	protected bool HasCachedContributionNear(
		array<ref HST_CombatPresenceContribution> contributions,
		vector center,
		float radiusMeters)
	{
		if (!contributions || contributions.Count() <= 0 || radiusMeters < 0.0)
			return false;
		float radiusSq = radiusMeters * radiusMeters;
		foreach (HST_CombatPresenceContribution contribution : contributions)
		{
			if (contribution && DistanceSq2D(contribution.m_vPosition, center) <= radiusSq)
				return true;
		}
		return false;
	}

	// Returns true only for a semantic state, contributor, or count change. A
	// stable HOT or COOLING zone therefore does not dirty persistence every scan.
	bool TickZoneHeat(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		HST_BalanceConfig balance,
		HST_ZoneState zone,
		string observerFactionKey)
	{
		if (!state || !preset || !zone || observerFactionKey.IsEmpty())
			return false;
		Configure(balance);
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		HST_CombatPresenceResult current = QueryZoneHostilePresence(
			state,
			preset,
			observerFactionKey,
			zone,
			false);
		if (!current.m_bQueryValid)
			return false;

		if (current.m_bHasLiveContributors)
			return ApplyHotSnapshot(zone, current, nowSecond);

		if (zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT)
			return BeginCooling(zone, nowSecond, m_iCoolingSeconds);

		if (zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING)
		{
			if (zone.m_iCombatPresenceCoolingUntilSecond > nowSecond)
				return false;
			return ApplyColdSnapshot(zone, "cold");
		}

		if (zone.m_eCombatPresenceState != HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COLD)
			return ApplyColdSnapshot(zone, "cold after invalid runtime state");
		return CanonicalizeStableCold(zone);
	}

	string BuildDiagnosticsReport(HST_CombatPresenceResult result)
	{
		if (!result)
			return "combat presence | result missing";
		string report = string.Format(
			"combat presence | valid %1 | state %2 | blocks %3 | live %4 | infantry/vehicle/static %5/%6/%7",
			result.m_bQueryValid,
			StateName(result.m_eState),
			result.m_bBlocksProgress,
			result.m_bHasLiveContributors,
			result.m_iInfantryCount,
			result.m_iMannedVehicleCount,
			result.m_iStaticOperatorCount);
		report = report + string.Format(
			" | operation/recent %1/%2 | contributors %3 omitted %4 | cooling %5s | hash %6 | reason %7",
			result.m_iCurrentOperationCount,
			result.m_iRecentFireCount,
			result.m_iContributorCount,
			result.m_iOmittedContributorCount,
			result.m_iCoolingRemainingSeconds,
			result.m_sContributorHash,
			result.m_sReason);
		foreach (HST_CombatPresenceContribution contribution : result.m_aContributors)
		{
			if (contribution)
				report = report + " | " + contribution.m_sContributorId + ": " + contribution.m_sFact;
		}
		return report;
	}

	string BuildZoneDiagnosticsReport(HST_ZoneState zone, int nowSecond)
	{
		if (!zone)
			return "combat presence zone | missing";
		int coolingRemaining;
		if (zone.m_eCombatPresenceState == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING)
			coolingRemaining = Math.Max(0, zone.m_iCombatPresenceCoolingUntilSecond - nowSecond);
		string report = string.Format(
			"combat presence zone %1 | state %2 | revision %3 | infantry/vehicle/static %4/%5/%6",
			zone.m_sZoneId,
			StateName(zone.m_eCombatPresenceState),
			zone.m_iCombatPresenceRevision,
			zone.m_iCombatPresenceInfantryCount,
			zone.m_iCombatPresenceMannedVehicleCount,
			zone.m_iCombatPresenceStaticOperatorCount);
		report = report + string.Format(
			" | operation/recent %1/%2 | cooling %3s | hash %4 | reason %5",
			zone.m_iCombatPresenceCurrentOperationCount,
			zone.m_iCombatPresenceRecentFireCount,
			coolingRemaining,
			zone.m_sCombatPresenceContributorHash,
			zone.m_sCombatPresenceReason);
		if (!zone.m_aCombatPresenceContributorIds || !zone.m_aCombatPresenceContributorFacts)
			return report + " | malformed contributor arrays";
		int pairCount = Math.Min(
			zone.m_aCombatPresenceContributorIds.Count(),
			zone.m_aCombatPresenceContributorFacts.Count());
		for (int pairIndex; pairIndex < pairCount; pairIndex++)
		{
			report = report + " | " + zone.m_aCombatPresenceContributorIds[pairIndex]
				+ ": " + zone.m_aCombatPresenceContributorFacts[pairIndex];
		}
		return report;
	}

	static string BuildContributorHash(
		array<string> contributorIds,
		array<string> contributorFacts,
		int infantryCount,
		int mannedVehicleCount,
		int staticOperatorCount,
		int currentOperationCount,
		int recentFireCount)
	{
		string canonical = string.Format(
			"cp1|%1|%2|%3|%4|%5",
			Math.Max(0, infantryCount),
			Math.Max(0, mannedVehicleCount),
			Math.Max(0, staticOperatorCount),
			Math.Max(0, currentOperationCount),
			Math.Max(0, recentFireCount));
		if (contributorIds && contributorFacts)
		{
			int pairCount = Math.Min(contributorIds.Count(), contributorFacts.Count());
			for (int pairIndex; pairIndex < pairCount; pairIndex++)
				canonical = canonical + "|" + contributorIds[pairIndex] + "=" + contributorFacts[pairIndex];
		}
		return string.Format("cp1_%1_%2", canonical.Hash(), (canonical + "|secondary").Hash());
	}

	static string StateName(HST_ECombatPresenceState state)
	{
		if (state == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT)
			return "hot";
		if (state == HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING)
			return "cooling";
		return "cold";
	}

	protected HST_CombatPresenceResult QueryPresenceNear(
		HST_CampaignState state,
		HST_CampaignPreset preset,
		string observerFactionKey,
		string exactFactionKey,
		vector center,
		float radiusMeters,
		bool hostileQuery)
	{
		HST_CombatPresenceResult result = new HST_CombatPresenceResult();
		result.m_sObserverFactionKey = observerFactionKey;
		result.m_sExactFactionKey = exactFactionKey;
		result.m_vCenter = center;
		result.m_fRadiusMeters = Math.Max(0.0, radiusMeters);

		if (!state || radiusMeters < 0.0)
		{
			result.m_sReason = "state missing or radius invalid";
			return result;
		}
		if (hostileQuery && (!preset || observerFactionKey.IsEmpty()))
		{
			result.m_sReason = "hostile query authority is missing";
			return result;
		}
		if (!hostileQuery && exactFactionKey.IsEmpty())
		{
			result.m_sReason = "exact faction query is missing its faction";
			return result;
		}

		result.m_bQueryValid = true;
		float radiusSq = radiusMeters * radiusMeters;
		array<string> selectedKeys = {};
		map<string, ref HST_CombatPresenceContribution> selectedByKey
			= new map<string, ref HST_CombatPresenceContribution>();
		EnsureContributionCache(state);
		foreach (HST_CombatPresenceContribution contribution : m_aContributionCache)
		{
			if (!contribution)
				continue;
			if (hostileQuery)
			{
				string relation = HST_FactionRelationService.ResolveRelation(
					preset,
					observerFactionKey,
					contribution.m_sFactionKey);
				if (!HST_FactionRelationService.IsHostileRelation(relation))
					continue;
			}
			else if (contribution.m_sFactionKey != exactFactionKey)
				continue;

			if (DistanceSq2D(contribution.m_vPosition, center) > radiusSq)
				continue;

			AccumulateResult(result, contribution);
			InsertBoundedContribution(selectedKeys, selectedByKey, contribution);
		}

		foreach (string selectedKey : selectedKeys)
		{
			HST_CombatPresenceContribution selected = selectedByKey.Get(selectedKey);
			if (selected)
				result.m_aContributors.Insert(selected);
		}
		FinalizeResult(result);
		ApplyMatchingAuthorityGaps(
			result,
			preset,
			observerFactionKey,
			exactFactionKey,
			center,
			radiusSq,
			hostileQuery);
		return result;
	}

	protected void EnsureContributionCache(HST_CampaignState state)
	{
		if (!state)
		{
			InvalidateCache();
			return;
		}
		int nowSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (m_CachedState == state && m_iContributionCacheSecond == nowSecond)
			return;

		m_CachedState = state;
		m_iContributionCacheSecond = nowSecond;
		m_aContributionCache.Clear();
		m_aAuthorityGapCache.Clear();
		ClearZoneQueryCaches();
		foreach (HST_ActiveGroupState group : state.m_aActiveGroups)
		{
			HST_CombatPresenceContribution contribution = BuildGroupContribution(
				state,
				group,
				nowSecond);
			if (contribution)
				m_aContributionCache.Insert(contribution);
			else
			{
				HST_CombatPresenceAuthorityGap authorityGap = BuildPhysicalAuthorityGap(
					state,
					group,
					nowSecond);
				if (authorityGap)
					m_aAuthorityGapCache.Insert(authorityGap);
			}
		}
	}

	protected void ClearZoneQueryCaches()
	{
		m_mZoneHostileQueryCache.Clear();
		m_mZoneExactQueryCache.Clear();
	}

	protected string BuildZoneQueryCacheKey(
		string queryKind,
		string factionKey,
		HST_ZoneState zone,
		float radiusMeters)
	{
		if (queryKind.IsEmpty() || factionKey.IsEmpty() || !zone || zone.m_sZoneId.IsEmpty())
			return "";
		return string.Format(
			"%1|%2|%3|%4|%5|%6",
			queryKind,
			factionKey,
			zone.m_sZoneId,
			zone.m_vPosition[0],
			zone.m_vPosition[2],
			radiusMeters);
	}

	protected string BuildPresetCacheIdentity(HST_CampaignPreset preset)
	{
		if (!preset)
			return "missing";
		return preset.m_sPresetId + "@" + preset.m_sResistanceFactionKey
			+ "@" + preset.m_sOccupierFactionKey + "@" + preset.m_sInvaderFactionKey;
	}

	protected HST_CombatPresenceResult CloneResult(HST_CombatPresenceResult source)
	{
		if (!source)
			return null;
		HST_CombatPresenceResult clone = new HST_CombatPresenceResult();
		clone.m_bQueryValid = source.m_bQueryValid;
		clone.m_bHasLiveContributors = source.m_bHasLiveContributors;
		clone.m_bBlocksProgress = source.m_bBlocksProgress;
		clone.m_bPersistedCoolingApplied = source.m_bPersistedCoolingApplied;
		clone.m_sObserverFactionKey = source.m_sObserverFactionKey;
		clone.m_sExactFactionKey = source.m_sExactFactionKey;
		clone.m_sZoneId = source.m_sZoneId;
		clone.m_vCenter = source.m_vCenter;
		clone.m_fRadiusMeters = source.m_fRadiusMeters;
		clone.m_eState = source.m_eState;
		clone.m_iInfantryCount = source.m_iInfantryCount;
		clone.m_iMannedVehicleCount = source.m_iMannedVehicleCount;
		clone.m_iStaticOperatorCount = source.m_iStaticOperatorCount;
		clone.m_iCurrentOperationCount = source.m_iCurrentOperationCount;
		clone.m_iRecentFireCount = source.m_iRecentFireCount;
		clone.m_iContributorCount = source.m_iContributorCount;
		clone.m_iOmittedContributorCount = source.m_iOmittedContributorCount;
		clone.m_iUnresolvedAuthorityCount = source.m_iUnresolvedAuthorityCount;
		clone.m_iCoolingRemainingSeconds = source.m_iCoolingRemainingSeconds;
		clone.m_sContributorHash = source.m_sContributorHash;
		clone.m_sReason = source.m_sReason;
		foreach (HST_CombatPresenceContribution contribution : source.m_aContributors)
			clone.m_aContributors.Insert(contribution);
		return clone;
	}

	protected HST_CombatPresenceAuthorityGap BuildPhysicalAuthorityGap(
		HST_CampaignState state,
		HST_ActiveGroupState group,
		int nowSecond)
	{
		if (!state || !group || !group.m_bSpawnedEntity)
			return null;
		if (group.m_sGroupId.IsEmpty() || group.m_sFactionKey.IsEmpty())
			return null;
#ifdef ENABLE_DIAG
		if (group.m_sGroupId.Contains(NON_GAMEPLAY_SMOKE_GROUP_TOKEN))
			return null;
#endif
		if (IsFreshAuthoritativeSample(group, nowSecond))
			return null;
		if (!state.IsOperationalActiveGroup(group) || state.IsQuarantinedActiveGroup(group))
			return null;
		if (IsTerminalGroupStatus(group.m_sRuntimeStatus))
			return null;

		HST_OperationRecordState operation;
		if (!group.m_sOperationId.IsEmpty())
			operation = state.FindOperation(group.m_sOperationId);
		HST_ConvoyElementState convoyElement;
		if (!group.m_sConvoyElementId.IsEmpty())
			convoyElement = state.FindConvoyElement(group.m_sConvoyElementId);
		if (convoyElement && !IsCombatEligibleConvoyElement(convoyElement))
			return null;

		HST_CombatPresenceAuthorityGap authorityGap = new HST_CombatPresenceAuthorityGap();
		authorityGap.m_sGroupId = group.m_sGroupId;
		authorityGap.m_sFactionKey = group.m_sFactionKey;
		authorityGap.m_vPosition = ResolveContributionPosition(group, operation, convoyElement);
		authorityGap.m_sReason = group.m_sCombatPresenceSampleReason;
		if (authorityGap.m_sReason.IsEmpty())
			authorityGap.m_sReason = "physical sample missing or stale";
		return authorityGap;
	}

	protected void ApplyMatchingAuthorityGaps(
		HST_CombatPresenceResult result,
		HST_CampaignPreset preset,
		string observerFactionKey,
		string exactFactionKey,
		vector center,
		float radiusSq,
		bool hostileQuery)
	{
		if (!result)
			return;
		string firstGapId;
		string firstGapReason;
		foreach (HST_CombatPresenceAuthorityGap authorityGap : m_aAuthorityGapCache)
		{
			if (!authorityGap)
				continue;
			if (hostileQuery)
			{
				string relation = HST_FactionRelationService.ResolveRelation(
					preset,
					observerFactionKey,
					authorityGap.m_sFactionKey);
				if (!HST_FactionRelationService.IsHostileRelation(relation))
					continue;
			}
			else if (authorityGap.m_sFactionKey != exactFactionKey)
				continue;
			if (DistanceSq2D(authorityGap.m_vPosition, center) > radiusSq)
				continue;

			result.m_iUnresolvedAuthorityCount++;
			if (firstGapId.IsEmpty())
			{
				firstGapId = authorityGap.m_sGroupId;
				firstGapReason = authorityGap.m_sReason;
			}
		}
		if (result.m_iUnresolvedAuthorityCount <= 0)
			return;

		result.m_bQueryValid = false;
		result.m_bBlocksProgress = true;
		result.m_sReason = string.Format(
			"combat-presence authority unresolved | candidates=%1 | first=%2 | reason=%3",
			result.m_iUnresolvedAuthorityCount,
			firstGapId,
			firstGapReason);
	}

	protected HST_CombatPresenceContribution BuildGroupContribution(
		HST_CampaignState state,
		HST_ActiveGroupState group,
		int nowSecond)
	{
		if (!state || !group || group.m_sGroupId.IsEmpty() || group.m_sFactionKey.IsEmpty())
			return null;
#ifdef ENABLE_DIAG
		if (group.m_sGroupId.Contains(NON_GAMEPLAY_SMOKE_GROUP_TOKEN))
			return null;
#endif
		if (!state.IsOperationalActiveGroup(group) || state.IsQuarantinedActiveGroup(group))
			return null;
		if (IsTerminalGroupStatus(group.m_sRuntimeStatus))
			return null;

		HST_OperationRecordState operation;
		if (!group.m_sOperationId.IsEmpty())
			operation = state.FindOperation(group.m_sOperationId);
		HST_ConvoyElementState convoyElement;
		if (!group.m_sConvoyElementId.IsEmpty())
			convoyElement = state.FindConvoyElement(group.m_sConvoyElementId);
		bool exactConvoy = convoyElement != null;
		if (exactConvoy && !IsCombatEligibleConvoyElement(convoyElement))
			return null;

		int infantryCount;
		int mannedVehicleCount;
		int staticOperatorCount;
		bool authoritativePhysicalSample;
		if (group.m_bSpawnedEntity)
		{
			if (!IsFreshAuthoritativeSample(group, nowSecond))
				return null;
			authoritativePhysicalSample = true;
			infantryCount = Math.Max(0, group.m_iCombatEffectiveInfantryCount);
			mannedVehicleCount = Math.Max(0, group.m_iOperationalMannedVehicleCount);
			staticOperatorCount = Math.Max(0, group.m_iCombatEffectiveStaticOperatorCount);
			if (exactConvoy)
				infantryCount = Math.Min(infantryCount, convoyElement.m_iSurvivingCrewCount);
		}
		else if (exactConvoy)
		{
			// Durable convoy crew is living infantry authority while virtual. An
			// abstract vehicle count is deliberately never treated as a contributor.
			infantryCount = Math.Max(0, convoyElement.m_iSurvivingCrewCount);
		}
		else
		{
			if (!IsEligibleVirtualRecord(group, operation))
				return null;
			infantryCount = Math.Max(
				Math.Max(0, group.m_iDurableLivingInfantryCount),
				Math.Max(0, group.m_iSurvivorInfantryCount));
		}

		if (infantryCount <= 0 && mannedVehicleCount <= 0 && staticOperatorCount <= 0)
			return null;

		HST_CombatPresenceContribution contribution = new HST_CombatPresenceContribution();
		contribution.m_sContributorId = "group:" + group.m_sGroupId;
		contribution.m_sGroupId = group.m_sGroupId;
		contribution.m_sOperationId = group.m_sOperationId;
		contribution.m_sFactionKey = group.m_sFactionKey;
		contribution.m_vPosition = ResolveContributionPosition(group, operation, convoyElement);
		contribution.m_iInfantryCount = infantryCount;
		contribution.m_iMannedVehicleCount = mannedVehicleCount;
		contribution.m_iStaticOperatorCount = staticOperatorCount;
		contribution.m_bAuthoritativePhysicalSample = authoritativePhysicalSample;
		contribution.m_sKind = ResolveContributionKind(
			infantryCount,
			mannedVehicleCount,
			staticOperatorCount);
		if (IsCurrentCombatOperation(operation))
			contribution.m_iCurrentOperationCount = 1;
		if (HasRecentFireEvidence(group, operation, nowSecond))
			contribution.m_iRecentFireCount = 1;
		contribution.m_sFact = BuildContributionFact(contribution, group);
		return contribution;
	}

	protected bool IsFreshAuthoritativeSample(HST_ActiveGroupState group, int nowSecond)
	{
		if (!group || !group.m_bCombatPresenceSampleAuthoritative)
			return false;
		if (group.m_iCombatPresenceSampleSecond < 0)
			return false;
		if (group.m_iCombatPresenceSampleSecond > nowSecond)
			return false;
		return nowSecond - group.m_iCombatPresenceSampleSecond <= MAX_SAMPLE_AGE_SECONDS;
	}

	protected bool IsEligibleVirtualRecord(
		HST_ActiveGroupState group,
		HST_OperationRecordState operation)
	{
		if (!group || group.m_bSpawnedEntity)
			return false;
		if (operation)
		{
			if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
				return false;
			if (operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
				return false;
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_VIRTUAL)
				return true;
			if (operation.m_eMaterializationState == HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_MATERIALIZING)
				return true;
		}
		return group.m_sRuntimeStatus == "virtual" || group.m_sRuntimeStatus.Contains("_virtual");
	}

	protected bool IsCombatEligibleConvoyElement(HST_ConvoyElementState element)
	{
		if (!element || element.m_iSurvivingCrewCount <= 0)
			return false;
		if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED)
			return false;
		if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_DESTROYED)
			return false;
		if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_RETIRED)
			return false;
		if (element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_CAPTURED)
			return false;
		return element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE
			|| element.m_eDisposition == HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ARRIVED;
	}

	protected bool IsTerminalGroupStatus(string status)
	{
		if (status.IsEmpty())
			return false;
		if (status == "eliminated" || status == "convoy_eliminated")
			return true;
		if (status == "folded" || status == "spawn_failed")
			return true;
		if (status == "despawned" || status == "deleted" || status == "retired")
			return true;
		if (status == "cancelled" || status == "canceled" || status == "support_recall_exited")
			return true;
		if (status.Contains("quarantined") || status.Contains("_terminal_"))
			return true;
		return false;
	}

	protected bool IsCurrentCombatOperation(HST_OperationRecordState operation)
	{
		if (!operation)
			return false;
		if (operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN)
			return false;
		if (operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
			return false;
		return operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_CONTACT
			|| operation.m_eEngagementMode == HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_ENGAGED;
	}

	protected bool HasRecentFireEvidence(
		HST_ActiveGroupState group,
		HST_OperationRecordState operation,
		int nowSecond)
	{
		int latestEvidenceSecond;
		if (group)
			latestEvidenceSecond = Math.Max(0, group.m_iLastCasualtySecond);
		if (operation)
			latestEvidenceSecond = Math.Max(latestEvidenceSecond, operation.m_iLastContactAtSecond);
		if (latestEvidenceSecond <= 0 || latestEvidenceSecond > nowSecond)
			return false;
		return nowSecond - latestEvidenceSecond <= m_iCoolingSeconds;
	}

	protected vector ResolveContributionPosition(
		HST_ActiveGroupState group,
		HST_OperationRecordState operation,
		HST_ConvoyElementState convoyElement)
	{
		if (convoyElement && !IsZeroVector(convoyElement.m_vCurrentPosition))
			return convoyElement.m_vCurrentPosition;
		if (operation
			&& operation.m_ePositionAuthority == HST_EOperationPositionAuthority.HST_OPERATION_POSITION_STRATEGIC
			&& !IsZeroVector(operation.m_vStrategicPosition))
			return operation.m_vStrategicPosition;
		if (group)
			return group.m_vPosition;
		return vector.Zero;
	}

	protected string ResolveContributionKind(
		int infantryCount,
		int mannedVehicleCount,
		int staticOperatorCount)
	{
		int populatedKinds;
		if (infantryCount > 0)
			populatedKinds++;
		if (mannedVehicleCount > 0)
			populatedKinds++;
		if (staticOperatorCount > 0)
			populatedKinds++;
		if (populatedKinds > 1)
			return "mixed";
		if (mannedVehicleCount > 0)
			return "manned_combat_vehicle";
		if (staticOperatorCount > 0)
			return "static_operator";
		return "infantry";
	}

	protected string BuildContributionFact(
		HST_CombatPresenceContribution contribution,
		HST_ActiveGroupState group)
	{
		string sampleKind = "durable_virtual";
		if (contribution.m_bAuthoritativePhysicalSample)
			sampleKind = "authoritative_physical";
		string fact = string.Format(
			"%1 faction=%2 infantry=%3 mannedVehicle=%4 static=%5 operation=%6 recentFire=%7 source=%8",
			contribution.m_sKind,
			contribution.m_sFactionKey,
			contribution.m_iInfantryCount,
			contribution.m_iMannedVehicleCount,
			contribution.m_iStaticOperatorCount,
			contribution.m_iCurrentOperationCount,
			contribution.m_iRecentFireCount,
			sampleKind);
		if (group && contribution.m_bAuthoritativePhysicalSample
			&& !group.m_sCombatPresenceSampleReason.IsEmpty())
			fact = fact + " sample=" + LimitText(group.m_sCombatPresenceSampleReason, 64);
		return LimitText(fact, MAX_DIAGNOSTIC_FACT_CHARACTERS);
	}

	protected void AccumulateResult(
		HST_CombatPresenceResult result,
		HST_CombatPresenceContribution contribution)
	{
		if (!result || !contribution)
			return;
		result.m_iInfantryCount = SafeAdd(
			result.m_iInfantryCount,
			Math.Max(0, contribution.m_iInfantryCount));
		result.m_iMannedVehicleCount = SafeAdd(
			result.m_iMannedVehicleCount,
			Math.Max(0, contribution.m_iMannedVehicleCount));
		result.m_iStaticOperatorCount = SafeAdd(
			result.m_iStaticOperatorCount,
			Math.Max(0, contribution.m_iStaticOperatorCount));
		result.m_iCurrentOperationCount = SafeAdd(
			result.m_iCurrentOperationCount,
			Math.Max(0, contribution.m_iCurrentOperationCount));
		result.m_iRecentFireCount = SafeAdd(
			result.m_iRecentFireCount,
			Math.Max(0, contribution.m_iRecentFireCount));
		result.m_iContributorCount = SafeAdd(result.m_iContributorCount, 1);
	}

	protected void InsertBoundedContribution(
		array<string> selectedKeys,
		map<string, ref HST_CombatPresenceContribution> selectedByKey,
		HST_CombatPresenceContribution contribution)
	{
		if (!selectedKeys || !selectedByKey || !contribution)
			return;
		string key = contribution.m_sContributorId;
		if (key.IsEmpty() || selectedByKey.Contains(key))
			return;
		selectedKeys.Insert(key);
		selectedByKey.Set(key, contribution);
		selectedKeys.Sort();
		if (selectedKeys.Count() <= MAX_PERSISTED_CONTRIBUTORS)
			return;
		int lastIndex = selectedKeys.Count() - 1;
		string removedKey = selectedKeys[lastIndex];
		selectedKeys.Remove(lastIndex);
		selectedByKey.Remove(removedKey);
	}

	protected void FinalizeResult(HST_CombatPresenceResult result)
	{
		if (!result)
			return;
		result.m_iOmittedContributorCount = Math.Max(
			0,
			result.m_iContributorCount - result.m_aContributors.Count());
		result.m_bHasLiveContributors = result.GetLivingContributorCount() > 0;
		result.m_bBlocksProgress = result.m_bHasLiveContributors;
		if (result.m_bHasLiveContributors)
			result.m_eState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT;
		array<string> ids = {};
		array<string> facts = {};
		foreach (HST_CombatPresenceContribution contribution : result.m_aContributors)
		{
			if (!contribution)
				continue;
			ids.Insert(contribution.m_sContributorId);
			facts.Insert(contribution.m_sFact);
		}
		if (result.m_bHasLiveContributors)
		{
			result.m_sContributorHash = BuildContributorHash(
				ids,
				facts,
				result.m_iInfantryCount,
				result.m_iMannedVehicleCount,
				result.m_iStaticOperatorCount,
				result.m_iCurrentOperationCount,
				result.m_iRecentFireCount);
			result.m_sReason = string.Format(
				"live contributors=%1 infantry=%2 mannedVehicles=%3 staticOperators=%4",
				result.m_iContributorCount,
				result.m_iInfantryCount,
				result.m_iMannedVehicleCount,
				result.m_iStaticOperatorCount);
		}
		else
		{
			result.m_sContributorHash = "";
			result.m_sReason = "no combat-effective contributors";
		}
	}

	protected void ApplyPersistedCooling(
		HST_CombatPresenceResult result,
		HST_ZoneState zone,
		int nowSecond)
	{
		if (!result || !zone)
			return;
		result.m_eState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING;
		result.m_bBlocksProgress = true;
		result.m_bPersistedCoolingApplied = true;
		result.m_iCoolingRemainingSeconds = Math.Max(
			0,
			zone.m_iCombatPresenceCoolingUntilSecond - nowSecond);
		result.m_sContributorHash = zone.m_sCombatPresenceContributorHash;
		result.m_sReason = "combat area cooling";
		if (!zone.m_aCombatPresenceContributorIds || !zone.m_aCombatPresenceContributorFacts)
			return;
		int pairCount = Math.Min(
			zone.m_aCombatPresenceContributorIds.Count(),
			zone.m_aCombatPresenceContributorFacts.Count());
		pairCount = Math.Min(pairCount, MAX_PERSISTED_CONTRIBUTORS);
		for (int pairIndex; pairIndex < pairCount; pairIndex++)
		{
			HST_CombatPresenceContribution archived = new HST_CombatPresenceContribution();
			archived.m_sContributorId = zone.m_aCombatPresenceContributorIds[pairIndex];
			archived.m_sKind = "cooling_evidence";
			archived.m_sFact = zone.m_aCombatPresenceContributorFacts[pairIndex];
			result.m_aContributors.Insert(archived);
		}
		result.m_iContributorCount = result.m_aContributors.Count();
	}

	protected bool ApplyHotSnapshot(
		HST_ZoneState zone,
		HST_CombatPresenceResult result,
		int nowSecond)
	{
		if (!zone || !result)
			return false;
		array<string> ids = {};
		array<string> facts = {};
		foreach (HST_CombatPresenceContribution contribution : result.m_aContributors)
		{
			if (!contribution)
				continue;
			ids.Insert(contribution.m_sContributorId);
			facts.Insert(contribution.m_sFact);
		}
		bool changed = zone.m_eCombatPresenceState != HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT;
		changed = changed || zone.m_iCombatPresenceInfantryCount != result.m_iInfantryCount;
		changed = changed || zone.m_iCombatPresenceMannedVehicleCount != result.m_iMannedVehicleCount;
		changed = changed || zone.m_iCombatPresenceStaticOperatorCount != result.m_iStaticOperatorCount;
		changed = changed || zone.m_iCombatPresenceCurrentOperationCount != result.m_iCurrentOperationCount;
		changed = changed || zone.m_iCombatPresenceRecentFireCount != result.m_iRecentFireCount;
		changed = changed || zone.m_sCombatPresenceContributorHash != result.m_sContributorHash;
		changed = changed || !StringArraysEqual(zone.m_aCombatPresenceContributorIds, ids);
		changed = changed || !StringArraysEqual(zone.m_aCombatPresenceContributorFacts, facts);
		if (!changed)
			return false;

		zone.m_eCombatPresenceState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_HOT;
		zone.m_iCombatPresenceLastHotSecond = nowSecond;
		zone.m_iCombatPresenceCoolingUntilSecond = 0;
		zone.m_iCombatPresenceInfantryCount = result.m_iInfantryCount;
		zone.m_iCombatPresenceMannedVehicleCount = result.m_iMannedVehicleCount;
		zone.m_iCombatPresenceStaticOperatorCount = result.m_iStaticOperatorCount;
		zone.m_iCombatPresenceCurrentOperationCount = result.m_iCurrentOperationCount;
		zone.m_iCombatPresenceRecentFireCount = result.m_iRecentFireCount;
		zone.m_sCombatPresenceContributorHash = result.m_sContributorHash;
		zone.m_sCombatPresenceReason = result.m_sReason;
		EnsureContributorArrays(zone);
		CopyStrings(ids, zone.m_aCombatPresenceContributorIds);
		CopyStrings(facts, zone.m_aCombatPresenceContributorFacts);
		IncrementRevision(zone);
		return true;
	}

	protected bool BeginCooling(HST_ZoneState zone, int nowSecond, int coolingSeconds)
	{
		if (!zone)
			return false;
		zone.m_eCombatPresenceState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COOLING;
		zone.m_iCombatPresenceLastHotSecond = nowSecond;
		zone.m_iCombatPresenceCoolingUntilSecond = nowSecond + Math.Max(1, coolingSeconds);
		zone.m_iCombatPresenceInfantryCount = 0;
		zone.m_iCombatPresenceMannedVehicleCount = 0;
		zone.m_iCombatPresenceStaticOperatorCount = 0;
		zone.m_iCombatPresenceCurrentOperationCount = 0;
		zone.m_iCombatPresenceRecentFireCount = 0;
		zone.m_sCombatPresenceReason = "combat area cooling";
		IncrementRevision(zone);
		return true;
	}

	protected bool ApplyColdSnapshot(HST_ZoneState zone, string reason)
	{
		if (!zone)
			return false;
		zone.m_eCombatPresenceState = HST_ECombatPresenceState.HST_COMBAT_PRESENCE_COLD;
		zone.m_iCombatPresenceLastHotSecond = 0;
		zone.m_iCombatPresenceCoolingUntilSecond = 0;
		zone.m_iCombatPresenceInfantryCount = 0;
		zone.m_iCombatPresenceMannedVehicleCount = 0;
		zone.m_iCombatPresenceStaticOperatorCount = 0;
		zone.m_iCombatPresenceCurrentOperationCount = 0;
		zone.m_iCombatPresenceRecentFireCount = 0;
		zone.m_sCombatPresenceContributorHash = "";
		zone.m_sCombatPresenceReason = reason;
		EnsureContributorArrays(zone);
		ClearStrings(zone.m_aCombatPresenceContributorIds);
		ClearStrings(zone.m_aCombatPresenceContributorFacts);
		IncrementRevision(zone);
		return true;
	}

	protected bool CanonicalizeStableCold(HST_ZoneState zone)
	{
		if (!zone)
			return false;
		bool changed = zone.m_iCombatPresenceLastHotSecond != 0;
		changed = changed || zone.m_iCombatPresenceCoolingUntilSecond != 0;
		changed = changed || zone.m_iCombatPresenceInfantryCount != 0;
		changed = changed || zone.m_iCombatPresenceMannedVehicleCount != 0;
		changed = changed || zone.m_iCombatPresenceStaticOperatorCount != 0;
		changed = changed || zone.m_iCombatPresenceCurrentOperationCount != 0;
		changed = changed || zone.m_iCombatPresenceRecentFireCount != 0;
		changed = changed || !zone.m_sCombatPresenceContributorHash.IsEmpty();
		changed = changed || zone.m_sCombatPresenceReason != "cold";
		changed = changed || (zone.m_aCombatPresenceContributorIds
			&& zone.m_aCombatPresenceContributorIds.Count() > 0);
		changed = changed || (zone.m_aCombatPresenceContributorFacts
			&& zone.m_aCombatPresenceContributorFacts.Count() > 0);
		if (!changed)
			return false;
		return ApplyColdSnapshot(zone, "cold");
	}

	protected void IncrementRevision(HST_ZoneState zone)
	{
		if (!zone)
			return;
		if (zone.m_iCombatPresenceRevision <= 0)
			zone.m_iCombatPresenceRevision = 1;
		else if (zone.m_iCombatPresenceRevision < int.MAX - 1)
			zone.m_iCombatPresenceRevision++;
	}

	protected HST_CombatPresenceResult BuildInvalidResult(
		string observerFactionKey,
		string exactFactionKey,
		string reason)
	{
		HST_CombatPresenceResult result = new HST_CombatPresenceResult();
		result.m_sObserverFactionKey = observerFactionKey;
		result.m_sExactFactionKey = exactFactionKey;
		result.m_sReason = reason;
		return result;
	}

	protected static int ResolveCoolingSeconds(HST_BalanceConfig balance)
	{
		if (!balance || balance.m_iCombatPresenceCoolingSeconds <= 0)
			return DEFAULT_COOLING_SECONDS;
		return Math.Min(300, balance.m_iCombatPresenceCoolingSeconds);
	}

	protected static float ResolveZoneRadius(HST_ZoneState zone)
	{
		if (zone && zone.m_iCaptureRadiusMeters > 0)
			return zone.m_iCaptureRadiusMeters;
		return DEFAULT_ZONE_RADIUS_METERS;
	}

	protected static int SafeAdd(int current, int increment)
	{
		if (increment <= 0)
			return Math.Max(0, current);
		if (current >= int.MAX - increment)
			return int.MAX;
		return Math.Max(0, current) + increment;
	}

	protected static bool StringArraysEqual(array<string> left, array<string> right)
	{
		if (!left && !right)
			return true;
		if (!left || !right)
			return false;
		if (left.Count() != right.Count())
			return false;
		for (int index; index < left.Count(); index++)
		{
			if (left[index] != right[index])
				return false;
		}
		return true;
	}

	protected static void EnsureContributorArrays(HST_ZoneState zone)
	{
		if (!zone)
			return;
		if (!zone.m_aCombatPresenceContributorIds)
			zone.m_aCombatPresenceContributorIds = new array<string>();
		if (!zone.m_aCombatPresenceContributorFacts)
			zone.m_aCombatPresenceContributorFacts = new array<string>();
	}

	protected static void CopyStrings(array<string> source, array<string> target)
	{
		if (!target)
			return;
		target.Clear();
		if (!source)
			return;
		foreach (string value : source)
			target.Insert(value);
	}

	protected static void ClearStrings(array<string> target)
	{
		if (!target)
			return;
		target.Clear();
	}

	protected static float DistanceSq2D(vector left, vector right)
	{
		float dx = left[0] - right[0];
		float dz = left[2] - right[2];
		return dx * dx + dz * dz;
	}

	protected static bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected static string LimitText(string value, int maxCharacters)
	{
		if (maxCharacters <= 0 || value.IsEmpty())
			return "";
		if (value.Length() <= maxCharacters)
			return value;
		return value.Substring(0, maxCharacters);
	}
}
