class HST_PhysicalWarService
{
	static const int MAX_ACTIVE_INFANTRY_PER_ZONE = 6;
	static const int MAX_ACTIVE_VEHICLES_PER_ZONE = 1;
	static const int QRF_ATTACK_RESOURCE_COST = 15;
	static const int QRF_SUPPORT_RESOURCE_COST = 5;
	static const int QRF_ETA_SECONDS = 180;
	static const int QRF_COOLDOWN_SECONDS = 900;
	static const int ROUTE_STATE_UPDATE_SECONDS = 30;
	static const float HQ_SAFE_RADIUS_METERS = 900;

	protected ref array<string> m_aRuntimeGroupIds = {};
	protected ref array<IEntity> m_aRuntimeGroupEntities = {};
	protected ref array<string> m_aRuntimeVehicleGroupIds = {};
	protected ref array<IEntity> m_aRuntimeVehicleEntities = {};

	bool UpdateZoneActivation(HST_CampaignState state, HST_BalanceConfig balance, HST_CampaignPreset preset = null, HST_EnemyDirectorService enemyDirector = null, HST_ZoneCompositionService compositions = null)
	{
		if (!state || !balance)
			return false;

		EnsureRuntimeGroupEntities(state);
		bool survivorChanged = UpdateRuntimeGroupSurvivors(state);

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return survivorChanged;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		bool changed;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			bool shouldBeActive = !IsZoneInsideHQSafeArea(state, zone) && IsAnyLivingPlayerNearZone(playerManager, playerIds, zone, balance);
			if (zone.m_bActive == shouldBeActive)
				continue;

			zone.m_bActive = shouldBeActive;
			if (shouldBeActive)
				changed = ActivateZone(state, zone, compositions) || changed;
			else
				changed = DeactivateZone(state, zone, compositions) || changed;

			changed = true;
			Print(string.Format("h-istasi | zone %1 physical activation = %2", zone.m_sZoneId, shouldBeActive));
		}

		if (UpdateQRF(state, preset, enemyDirector))
			changed = true;
		if (UpdateActiveGroupRoutes(state))
			changed = true;

		return changed || survivorChanged;
	}

	protected bool IsAnyLivingPlayerNearZone(PlayerManager playerManager, array<int> playerIds, HST_ZoneState zone, HST_BalanceConfig balance)
	{
		if (!zone)
			return false;

		float radius = zone.m_iActivationRadiusMeters;
		if (radius <= 0)
			radius = balance.m_iActivationRadiusMeters;

		float radiusSq = radius * radius;
		foreach (int playerId : playerIds)
		{
			IEntity playerEntity = GetBestPlayerEntity(playerManager, playerId);
			if (!IsLivingPlayerEntity(playerEntity))
				continue;

			if (DistanceSq2D(playerEntity.GetOrigin(), zone.m_vPosition) <= radiusSq)
				return true;
		}

		return false;
	}

	protected bool ActivateZone(HST_CampaignState state, HST_ZoneState zone, HST_ZoneCompositionService compositions = null)
	{
		bool changed;
		array<ref HST_ZoneSpawnSlotState> slots = {};
		if (compositions)
		{
			changed = compositions.EnsureZoneComposition(state, zone) || changed;
			slots = compositions.BuildZoneSpawnSlots(state, zone);
		}

		if (HasActiveGarrisonGroup(state, zone))
		{
			ApplyActiveZoneCounts(state, zone);
			return changed;
		}

		HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, zone.m_sOwnerFactionKey);
		if (!garrison)
		{
			zone.m_iActiveInfantryCount = 0;
			zone.m_iActiveVehicleCount = 0;
			return changed;
		}

		int garrisonInfantryBefore = garrison.m_iInfantryCount;
		int garrisonVehiclesBefore = garrison.m_iVehicleCount;
		int infantryCount = Math.Min(garrison.m_iInfantryCount, ResolveActiveInfantryCap(zone));
		int vehicleCount = Math.Min(garrison.m_iVehicleCount, ResolveActiveVehicleCap(zone));
		if (infantryCount <= 0 && vehicleCount <= 0)
		{
			zone.m_iActiveInfantryCount = 0;
			zone.m_iActiveVehicleCount = 0;
			return changed;
		}

		garrison.m_iInfantryCount = Math.Max(0, garrison.m_iInfantryCount - infantryCount);
		garrison.m_iVehicleCount = Math.Max(0, garrison.m_iVehicleCount - vehicleCount);

		int spawnedInfantryGroups = SpawnZoneInfantryGroups(state, zone, slots, infantryCount, compositions);
		int spawnedVehicleGroups = SpawnZoneVehicleGroups(state, zone, slots, vehicleCount);
		ApplyActiveZoneCounts(state, zone);
		string activationReport = string.Format("h-istasi | activated zone %1 | requested infantry %2/%3 vehicles %4/%5 | spawned infantry groups %6 vehicle groups %7", zone.m_sZoneId, infantryCount, garrisonInfantryBefore, vehicleCount, garrisonVehiclesBefore, spawnedInfantryGroups, spawnedVehicleGroups);
		activationReport = activationReport + string.Format(" | active now infantry %1 vehicles %2 | abstract garrison now infantry %3 vehicles %4", zone.m_iActiveInfantryCount, zone.m_iActiveVehicleCount, garrison.m_iInfantryCount, garrison.m_iVehicleCount);
		Print(string.Format("%1", activationReport));
		return true;
	}

	protected bool DeactivateZone(HST_CampaignState state, HST_ZoneState zone, HST_ZoneCompositionService compositions = null)
	{
		bool changed;
		int foldedGroups;
		int returnedInfantry;
		int returnedVehicles;
		for (int i = state.m_aActiveGroups.Count() - 1; i >= 0; i--)
		{
			HST_ActiveGroupState activeGroup = state.m_aActiveGroups[i];
			if (!activeGroup || activeGroup.m_sZoneId != zone.m_sZoneId || activeGroup.m_bQRF)
				continue;

			int beforeInfantry;
			int beforeVehicles;
			HST_GarrisonState beforeGarrison = state.FindGarrison(activeGroup.m_sZoneId, activeGroup.m_sFactionKey);
			if (beforeGarrison)
			{
				beforeInfantry = beforeGarrison.m_iInfantryCount;
				beforeVehicles = beforeGarrison.m_iVehicleCount;
			}

			FoldActiveGroup(state, activeGroup);
			HST_GarrisonState afterGarrison = state.FindGarrison(activeGroup.m_sZoneId, activeGroup.m_sFactionKey);
			if (afterGarrison)
			{
				returnedInfantry += Math.Max(0, afterGarrison.m_iInfantryCount - beforeInfantry);
				returnedVehicles += Math.Max(0, afterGarrison.m_iVehicleCount - beforeVehicles);
			}

			DeleteRuntimeGroupEntity(activeGroup.m_sGroupId);
			state.m_aActiveGroups.Remove(i);
			foldedGroups++;
			changed = true;
		}

		zone.m_iActiveInfantryCount = 0;
		zone.m_iActiveVehicleCount = 0;
		if (compositions)
			changed = compositions.CleanupZoneComposition(zone.m_sZoneId) || changed;
		if (changed)
		{
			HST_GarrisonState garrison = state.FindGarrison(zone.m_sZoneId, zone.m_sOwnerFactionKey);
			int garrisonInfantry;
			int garrisonVehicles;
			if (garrison)
			{
				garrisonInfantry = garrison.m_iInfantryCount;
				garrisonVehicles = garrison.m_iVehicleCount;
			}

			Print(string.Format("h-istasi | deactivated zone %1 | folded groups %2 | returned infantry %3 vehicles %4 | abstract garrison now infantry %5 vehicles %6", zone.m_sZoneId, foldedGroups, returnedInfantry, returnedVehicles, garrisonInfantry, garrisonVehicles));
		}
		return changed;
	}

	protected int SpawnZoneInfantryGroups(HST_CampaignState state, HST_ZoneState zone, array<ref HST_ZoneSpawnSlotState> slots, int infantryCount, HST_ZoneCompositionService compositions)
	{
		if (infantryCount <= 0)
			return 0;

		int groupCount = ResolveInfantryGroupCount(infantryCount);
		int remainingInfantry = infantryCount;
		int spawnedGroups;
		bool patrolAssigned;
		for (int groupIndex = 0; groupIndex < groupCount; groupIndex++)
		{
			int remainingGroups = groupCount - groupIndex;
			int groupInfantry = Math.Max(1, remainingInfantry / remainingGroups);
			remainingInfantry = Math.Max(0, remainingInfantry - groupInfantry);

			HST_ZoneSpawnSlotState slot;
			string runtimeStatus = "guard_center";
			if (compositions)
			{
				if (groupIndex == 0)
				{
					slot = compositions.SelectSlot(slots, HST_ZoneCompositionService.SLOT_INFANTRY, 0);
				}
				else
				{
					slot = compositions.SelectSlot(slots, HST_ZoneCompositionService.SLOT_PATROL, groupIndex - 1);
					runtimeStatus = "patrol_distributed";
					patrolAssigned = true;
				}

				if (!slot)
					slot = compositions.SelectSlot(slots, HST_ZoneCompositionService.SLOT_INFANTRY, groupIndex);
			}

			if (SpawnActiveZoneGroup(state, zone, zone.m_sOwnerFactionKey, groupInfantry, 0, false, slot, runtimeStatus))
				spawnedGroups++;
		}

		if (patrolAssigned && compositions)
			compositions.ReportPatrolWaypointUnavailable(zone.m_sZoneId);

		return spawnedGroups;
	}

	protected int SpawnZoneVehicleGroups(HST_CampaignState state, HST_ZoneState zone, array<ref HST_ZoneSpawnSlotState> slots, int vehicleCount)
	{
		if (vehicleCount <= 0)
			return 0;

		int spawnedVehicles;
		for (int vehicleIndex = 0; vehicleIndex < vehicleCount; vehicleIndex++)
		{
			HST_ZoneSpawnSlotState slot;
			if (slots)
			{
				foreach (HST_ZoneSpawnSlotState candidate : slots)
				{
					if (!candidate || candidate.m_sKind != HST_ZoneCompositionService.SLOT_VEHICLE || candidate.m_bOccupied)
						continue;

					candidate.m_bOccupied = true;
					slot = candidate;
					break;
				}
			}

			if (!slot)
			{
				FoldUnspawnedForces(state, zone.m_sZoneId, zone.m_sOwnerFactionKey, 0, 1);
				Print(string.Format("h-istasi | vehicle spawn skipped for %1: no safe vehicle slot", zone.m_sZoneId), LogLevel.WARNING);
				continue;
			}

			if (SpawnActiveZoneGroup(state, zone, zone.m_sOwnerFactionKey, 0, 1, false, slot, "vehicle_guard"))
				spawnedVehicles++;
		}

		return spawnedVehicles;
	}

	protected bool SpawnActiveZoneGroup(HST_CampaignState state, HST_ZoneState zone, string factionKey, int infantryCount, int vehicleCount, bool qrf, HST_ZoneSpawnSlotState slot, string runtimeStatus)
	{
		HST_ActiveGroupState activeGroup = CreateActiveGroup(state, zone, factionKey, infantryCount, vehicleCount, qrf);
		ApplySpawnSlot(activeGroup, slot, runtimeStatus);
		if (vehicleCount > 0 && infantryCount <= 0)
		{
			activeGroup.m_sPrefab = SelectVehiclePrefab(state, zone, factionKey, state.m_aActiveGroups.Count());
			activeGroup.m_sSpawnFallbackMode = "vehicle";
		}

		state.m_aActiveGroups.Insert(activeGroup);
		bool spawned;
		if (vehicleCount > 0 && infantryCount <= 0)
			spawned = TrySpawnActiveVehicle(activeGroup, state);
		else
			spawned = TrySpawnActiveGroup(activeGroup, state);

		if (!spawned)
		{
			FoldActiveGroup(state, activeGroup);
			state.m_aActiveGroups.Remove(state.m_aActiveGroups.Count() - 1);
			ApplyActiveZoneCounts(state, zone);
			NotifyRuntimeEvent("ai_spawn_failed_" + activeGroup.m_sGroupId, "Enemy Spawn Failed", string.Format("%1 could not spawn at %2. %3", zone.m_sDisplayName, zone.m_sZoneId, activeGroup.m_sSpawnFailureReason), zone.m_sZoneId, activeGroup.m_vPosition, 6.0);
			return false;
		}

		return true;
	}

	protected void ApplySpawnSlot(HST_ActiveGroupState activeGroup, HST_ZoneSpawnSlotState slot, string runtimeStatus)
	{
		if (!activeGroup)
			return;

		if (slot)
		{
			activeGroup.m_vPosition = slot.m_vPosition;
			activeGroup.m_vSourcePosition = slot.m_vPosition;
			activeGroup.m_vTargetPosition = slot.m_vPosition;
			activeGroup.m_sSpawnFallbackMode = slot.m_sSlotId;
		}

		if (!runtimeStatus.IsEmpty())
			activeGroup.m_sRuntimeStatus = runtimeStatus;
	}

	protected int ResolveInfantryGroupCount(int infantryCount)
	{
		if (infantryCount <= 4)
			return 1;
		if (infantryCount <= 8)
			return 2;
		if (infantryCount <= 12)
			return 3;

		return 4;
	}

	protected void FoldUnspawnedForces(HST_CampaignState state, string zoneId, string factionKey, int infantryCount, int vehicleCount)
	{
		if (!state || zoneId.IsEmpty() || factionKey.IsEmpty())
			return;

		HST_GarrisonState garrison = state.FindGarrison(zoneId, factionKey);
		if (!garrison)
		{
			garrison = new HST_GarrisonState();
			garrison.m_sZoneId = zoneId;
			garrison.m_sFactionKey = factionKey;
			state.m_aGarrisons.Insert(garrison);
		}

		garrison.m_iInfantryCount += Math.Max(0, infantryCount);
		garrison.m_iVehicleCount += Math.Max(0, vehicleCount);
	}

	protected bool UpdateQRF(HST_CampaignState state, HST_CampaignPreset preset, HST_EnemyDirectorService enemyDirector)
	{
		bool changed = ResolveArrivedQRFs(state);
		if (!enemyDirector)
			return changed;

		string resistanceFactionKey = "FIA";
		if (preset && !preset.m_sResistanceFactionKey.IsEmpty())
			resistanceFactionKey = preset.m_sResistanceFactionKey;

		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone.m_bActive || zone.m_sOwnerFactionKey == resistanceFactionKey || zone.m_sQRFRouteId.IsEmpty())
				continue;

			if (IsZoneInsideHQSafeArea(state, zone))
				continue;

			if (state.m_iElapsedSeconds < zone.m_iQrfCooldownUntilSecond)
				continue;

			if (state.FindActiveQRF(zone.m_sZoneId, zone.m_sOwnerFactionKey))
				continue;

			if (!enemyDirector.TrySpend(state, zone.m_sOwnerFactionKey, QRF_ATTACK_RESOURCE_COST, QRF_SUPPORT_RESOURCE_COST))
				continue;

			HST_ActiveGroupState activeGroup = CreateActiveGroup(state, zone, zone.m_sOwnerFactionKey, Math.Min(ResolveActiveInfantryCap(zone), Math.Max(2, zone.m_iGarrisonSlots / 3 + state.m_iWarLevel / 2)), ResolveActiveVehicleCap(zone), true);
			state.m_aActiveGroups.Insert(activeGroup);
			if (!TrySpawnActiveGroup(activeGroup, state))
			{
				state.m_aActiveGroups.Remove(state.m_aActiveGroups.Count() - 1);
				zone.m_iQrfCooldownUntilSecond = state.m_iElapsedSeconds + QRF_COOLDOWN_SECONDS;
				NotifyRuntimeEvent("qrf_spawn_failed", "QRF Spawn Failed", string.Format("%1 QRF could not spawn for %2. %3", zone.m_sOwnerFactionKey, zone.m_sDisplayName, activeGroup.m_sSpawnFailureReason), zone.m_sZoneId, activeGroup.m_vPosition, 6.0);
				changed = true;
				continue;
			}

			HST_QRFState qrf = new HST_QRFState();
			qrf.m_sInstanceId = string.Format("qrf_%1_%2_%3", zone.m_sZoneId, zone.m_sOwnerFactionKey, state.m_iElapsedSeconds);
			qrf.m_sFactionKey = zone.m_sOwnerFactionKey;
			qrf.m_sSourceZoneId = zone.m_sZoneId;
			qrf.m_sTargetZoneId = zone.m_sZoneId;
			qrf.m_sGroupId = activeGroup.m_sGroupId;
			qrf.m_iStartedAtSecond = state.m_iElapsedSeconds;
			qrf.m_iETASeconds = QRF_ETA_SECONDS;
			state.m_aQRFs.Insert(qrf);
			zone.m_iQrfCooldownUntilSecond = state.m_iElapsedSeconds + QRF_COOLDOWN_SECONDS;
			Print(string.Format("h-istasi | dispatched QRF %1 to active zone %2", qrf.m_sInstanceId, zone.m_sZoneId));
			NotifyRuntimeEvent("qrf_dispatched_" + qrf.m_sInstanceId, "QRF Dispatched", string.Format("%1 is sending a quick reaction force toward %2.", zone.m_sOwnerFactionKey, zone.m_sDisplayName), zone.m_sZoneId, activeGroup.m_vPosition, 6.0);
			changed = true;
		}

		return changed;
	}

	protected bool ResolveArrivedQRFs(HST_CampaignState state)
	{
		bool changed;
		foreach (HST_QRFState qrf : state.m_aQRFs)
		{
			if (!qrf || qrf.m_bResolved)
				continue;

			if (state.m_iElapsedSeconds < qrf.m_iStartedAtSecond + qrf.m_iETASeconds)
				continue;

			qrf.m_bResolved = true;
			qrf.m_bSucceeded = true;
			HST_ActiveGroupState activeGroup = state.FindActiveGroup(qrf.m_sGroupId);
			if (activeGroup)
				activeGroup.m_sRuntimeStatus = "arrived";
			changed = true;
			Print(string.Format("h-istasi | QRF %1 reached zone %2", qrf.m_sInstanceId, qrf.m_sTargetZoneId));
			vector qrfPosition;
			if (activeGroup)
				qrfPosition = activeGroup.m_vPosition;
			NotifyRuntimeEvent("qrf_arrived_" + qrf.m_sInstanceId, "QRF Arrived", string.Format("%1 QRF reached %2.", qrf.m_sFactionKey, qrf.m_sTargetZoneId), qrf.m_sTargetZoneId, qrfPosition, 6.0);
		}

		return changed;
	}

	protected bool UpdateActiveGroupRoutes(HST_CampaignState state)
	{
		if (!state || state.m_iElapsedSeconds % ROUTE_STATE_UPDATE_SECONDS != 0)
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || !activeGroup.m_bSpawnedEntity)
				continue;
			if (!activeGroup.m_bQRF && activeGroup.m_sRuntimeStatus != "support_active")
				continue;
			if (activeGroup.m_sRuntimeStatus != "routing" && activeGroup.m_sRuntimeStatus != "support_active")
				continue;

			int duration = ResolveRouteDurationSeconds(activeGroup);
			int elapsed = state.m_iElapsedSeconds - activeGroup.m_iSpawnedAtSecond;
			if (elapsed < 0)
				elapsed = 0;

			float progress = Math.Min(1.0, elapsed * 1.0 / duration);
			vector position = LerpPosition(activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition, progress);
			position = HST_WorldPositionService.ResolveSafeGroundPosition(position, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
			if (DistanceSq2D(activeGroup.m_vPosition, position) < 25)
			{
				if (progress >= 1.0 && activeGroup.m_sRuntimeStatus != "arrived")
				{
					activeGroup.m_sRuntimeStatus = "arrived";
					changed = true;
				}
				continue;
			}

			activeGroup.m_vPosition = position;
			SetRuntimeGroupEntitiesOrigin(activeGroup.m_sGroupId, position);

			if (progress >= 1.0)
				activeGroup.m_sRuntimeStatus = "arrived";

			changed = true;
		}

		return changed;
	}

	protected int ResolveRouteDurationSeconds(HST_ActiveGroupState activeGroup)
	{
		if (!activeGroup)
			return QRF_ETA_SECONDS;

		if (activeGroup.m_bQRF)
			return QRF_ETA_SECONDS;

		float distance = Math.Sqrt(DistanceSq2D(activeGroup.m_vSourcePosition, activeGroup.m_vTargetPosition));
		return Math.Max(120, Math.Round(distance / 6.0));
	}

	protected vector LerpPosition(vector sourcePosition, vector targetPosition, float progress)
	{
		vector result;
		result[0] = sourcePosition[0] + (targetPosition[0] - sourcePosition[0]) * progress;
		result[1] = sourcePosition[1] + (targetPosition[1] - sourcePosition[1]) * progress;
		result[2] = sourcePosition[2] + (targetPosition[2] - sourcePosition[2]) * progress;
		return result;
	}

	protected HST_ActiveGroupState CreateActiveGroup(HST_CampaignState state, HST_ZoneState zone, string factionKey, int infantryCount, int vehicleCount, bool qrf)
	{
		HST_ActiveGroupState activeGroup = new HST_ActiveGroupState();
		activeGroup.m_sGroupId = BuildGroupId(state, zone, factionKey, qrf);
		activeGroup.m_sZoneId = zone.m_sZoneId;
		activeGroup.m_sFactionKey = factionKey;
		activeGroup.m_sPrefab = SelectGroupPrefab(state, zone, factionKey, qrf);
		activeGroup.m_sRouteId = ResolveGroupRouteId(zone, qrf);
		activeGroup.m_vSourcePosition = ResolveGroupSourcePosition(state, zone, activeGroup.m_sRouteId, qrf);
		activeGroup.m_vTargetPosition = ResolveGroupTargetPosition(state, zone, activeGroup.m_sRouteId, qrf);
		activeGroup.m_vPosition = HST_WorldPositionService.ResolveSafeGroundPosition(activeGroup.m_vSourcePosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		activeGroup.m_sRuntimeStatus = "queued";
		activeGroup.m_iInfantryCount = infantryCount;
		activeGroup.m_iVehicleCount = vehicleCount;
		activeGroup.m_iLastSeenAliveCount = infantryCount + vehicleCount;
		activeGroup.m_iSurvivorInfantryCount = infantryCount;
		activeGroup.m_iSurvivorVehicleCount = vehicleCount;
		activeGroup.m_bQRF = qrf;
		return activeGroup;
	}

	protected string BuildGroupId(HST_CampaignState state, HST_ZoneState zone, string factionKey, bool qrf)
	{
		string prefix = "grp";
		if (qrf)
			prefix = "qrf";

		return string.Format("%1_%2_%3_%4_%5", prefix, zone.m_sZoneId, factionKey, state.m_iElapsedSeconds, state.m_aActiveGroups.Count());
	}

	protected string SelectGroupPrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, bool qrf)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (!faction)
			return "";

		int seed = BuildGroupSelectionSeed(state, zone, qrf);
		array<string> candidates = {};
		if (qrf)
		{
			AppendUniqueGroupPrefabs(candidates, faction.m_aQRFGroupPrefabs);
			AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
			return SelectValidGroupPrefabFromList(candidates, seed, factionKey, "qrf");
		}

		AppendUniqueGroupPrefabs(candidates, faction.m_aGroupPrefabs);
		AppendUniqueGroupPrefabs(candidates, faction.m_aPatrolGroupPrefabs);
		return SelectValidGroupPrefabFromList(candidates, seed, factionKey, "garrison");
	}

	protected string SelectVehiclePrefab(HST_CampaignState state, HST_ZoneState zone, string factionKey, int index)
	{
		HST_FactionTemplate faction = HST_DefaultCatalog.CreateFactionTemplate(factionKey);
		if (!faction || faction.m_aVehiclePrefabs.Count() == 0)
			return "";

		int seed = BuildGroupSelectionSeed(state, zone, false) + index * 47;
		int startIndex = HST_DefaultCatalog.PositiveMod(seed, faction.m_aVehiclePrefabs.Count());
		for (int offset = 0; offset < faction.m_aVehiclePrefabs.Count(); offset++)
		{
			int candidateIndex = HST_DefaultCatalog.PositiveMod(startIndex + offset, faction.m_aVehiclePrefabs.Count());
			string prefab = faction.m_aVehiclePrefabs[candidateIndex];
			if (IsValidVehiclePrefabResource(prefab, factionKey))
				return prefab;
		}

		Print(string.Format("h-istasi | no valid active vehicle prefab found for faction %1", factionKey), LogLevel.WARNING);
		return "";
	}

	protected void AppendUniqueGroupPrefabs(array<string> candidates, array<string> source)
	{
		if (!candidates || !source)
			return;

		foreach (string prefab : source)
		{
			if (prefab.IsEmpty() || candidates.Contains(prefab))
				continue;

			candidates.Insert(prefab);
		}
	}

	protected int BuildGroupSelectionSeed(HST_CampaignState state, HST_ZoneState zone, bool qrf)
	{
		int seed = 271;
		if (state)
			seed += state.m_iCampaignSeed * 19 + state.m_iElapsedSeconds * 3 + state.m_aActiveGroups.Count() * 97;
		if (zone)
			seed += zone.m_iPriority * 41 + zone.m_sZoneId.Length() * 113 + Math.Round(zone.m_vPosition[0]) + Math.Round(zone.m_vPosition[2]);
		if (qrf)
			seed += 7001;
		if (seed < 0)
			seed = -seed;

		return seed;
	}

	protected bool TrySpawnActiveGroup(HST_ActiveGroupState activeGroup, HST_CampaignState state = null)
	{
		if (!activeGroup || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		if (activeGroup.m_bSpawnAttempted && !activeGroup.m_bSpawnedEntity)
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
		{
			activeGroup.m_sSpawnFailureReason = "Respawn system is unavailable.";
			return false;
		}

		activeGroup.m_bSpawnAttempted = true;
		string requestedStatus = activeGroup.m_sRuntimeStatus;
		activeGroup.m_sRuntimeStatus = "spawning";
		activeGroup.m_sSpawnFallbackMode = "group";
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = 0;

		vector spawnPosition = HST_WorldPositionService.ResolveSafeGroundPosition(activeGroup.m_vPosition, HST_WorldPositionService.CHARACTER_GROUND_OFFSET, false, 2.0);
		activeGroup.m_vPosition = spawnPosition;

		GenericEntity entity;
		int agentCount = 0;
		string failureReason = "";
		if (activeGroup.m_sPrefab.IsEmpty())
		{
			failureReason = string.Format("No group prefab configured for faction %1.", activeGroup.m_sFactionKey);
		}
		else if (IsValidGroupPrefabResource(activeGroup.m_sPrefab, activeGroup.m_sFactionKey))
		{
			entity = respawnSystem.DoSpawn(activeGroup.m_sPrefab, spawnPosition, "0 0 0");
			if (!entity)
				failureReason = string.Format("Group prefab did not spawn: %1.", activeGroup.m_sPrefab);
			else
				agentCount = ResolveSpawnedAgentCount(entity, activeGroup);
		}
		else
		{
			failureReason = string.Format("Invalid group prefab: %1.", activeGroup.m_sPrefab);
		}

		if (!entity)
		{
			activeGroup.m_sSpawnFailureReason = failureReason;
			if (activeGroup.m_sSpawnFailureReason.IsEmpty())
				activeGroup.m_sSpawnFailureReason = string.Format("Group prefab spawn failed for faction %1.", activeGroup.m_sFactionKey);
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			Print(string.Format("h-istasi | active group prefab spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			return false;
		}

		activeGroup.m_bSpawnedEntity = true;
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = ResolveSpawnedRuntimeStatus(activeGroup, requestedStatus);
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = agentCount;
		activeGroup.m_iLastSeenAliveCount = Math.Max(agentCount, activeGroup.m_iInfantryCount + activeGroup.m_iVehicleCount);
		activeGroup.m_iSurvivorInfantryCount = activeGroup.m_iInfantryCount;
		if (state)
			activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		m_aRuntimeGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeGroupEntities.Insert(entity);
		Print(string.Format("h-istasi | spawned active group %1 using %2 (%3 agents)", activeGroup.m_sGroupId, activeGroup.m_sSpawnFallbackMode, agentCount));
		return true;
	}

	protected bool TrySpawnActiveVehicle(HST_ActiveGroupState activeGroup, HST_CampaignState state = null)
	{
		if (!activeGroup || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
			return false;

		if (activeGroup.m_bSpawnAttempted && !activeGroup.m_bSpawnedEntity)
			return false;

		SCR_RespawnSystemComponent respawnSystem = SCR_RespawnSystemComponent.GetInstance();
		if (!respawnSystem)
		{
			activeGroup.m_sSpawnFailureReason = "Respawn system is unavailable.";
			return false;
		}

		activeGroup.m_bSpawnAttempted = true;
		string requestedStatus = activeGroup.m_sRuntimeStatus;
		activeGroup.m_sRuntimeStatus = "spawning";
		activeGroup.m_sSpawnFallbackMode = "vehicle";
		activeGroup.m_sSpawnFailureReason = "";

		vector spawnPosition;
		if (!HST_WorldPositionService.TryResolveVehicleSpawnPosition(activeGroup.m_vPosition, spawnPosition, true))
		{
			activeGroup.m_sSpawnFailureReason = "No dry vehicle-safe ground at assigned slot.";
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			Print(string.Format("h-istasi | active vehicle spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			return false;
		}

		activeGroup.m_vPosition = spawnPosition;
		string failureReason = "";
		GenericEntity entity;
		if (activeGroup.m_sPrefab.IsEmpty())
		{
			failureReason = string.Format("No vehicle prefab configured for faction %1.", activeGroup.m_sFactionKey);
		}
		else if (IsValidVehiclePrefabResource(activeGroup.m_sPrefab, activeGroup.m_sFactionKey))
		{
			entity = respawnSystem.DoSpawn(activeGroup.m_sPrefab, spawnPosition, HST_WorldPositionService.BuildUprightAngles(0));
			if (!entity)
				failureReason = string.Format("Vehicle prefab did not spawn: %1.", activeGroup.m_sPrefab);
		}
		else
		{
			failureReason = string.Format("Invalid vehicle prefab: %1.", activeGroup.m_sPrefab);
		}

		if (!entity)
		{
			activeGroup.m_sSpawnFailureReason = failureReason;
			if (activeGroup.m_sSpawnFailureReason.IsEmpty())
				activeGroup.m_sSpawnFailureReason = string.Format("Vehicle prefab spawn failed for faction %1.", activeGroup.m_sFactionKey);
			activeGroup.m_sRuntimeStatus = "spawn_failed";
			Print(string.Format("h-istasi | active vehicle prefab spawn failed for %1 (%2): %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, activeGroup.m_sSpawnFailureReason), LogLevel.WARNING);
			return false;
		}

		activeGroup.m_bSpawnedEntity = true;
		activeGroup.m_sRuntimeEntityId = activeGroup.m_sGroupId;
		activeGroup.m_sRuntimeStatus = ResolveSpawnedRuntimeStatus(activeGroup, requestedStatus);
		activeGroup.m_sSpawnFailureReason = "";
		activeGroup.m_iSpawnedAgentCount = 1;
		activeGroup.m_iLastSeenAliveCount = 1;
		activeGroup.m_iSurvivorInfantryCount = 0;
		activeGroup.m_iSurvivorVehicleCount = Math.Max(1, activeGroup.m_iVehicleCount);
		if (state)
			activeGroup.m_iSpawnedAtSecond = state.m_iElapsedSeconds;
		m_aRuntimeVehicleGroupIds.Insert(activeGroup.m_sGroupId);
		m_aRuntimeVehicleEntities.Insert(entity);
		Print(string.Format("h-istasi | spawned active vehicle %1 using %2 at %3", activeGroup.m_sGroupId, activeGroup.m_sPrefab, spawnPosition));
		return true;
	}

	protected string ResolveSpawnedRuntimeStatus(HST_ActiveGroupState activeGroup, string requestedStatus)
	{
		if (!requestedStatus.IsEmpty() && requestedStatus != "queued" && requestedStatus != "spawning")
			return requestedStatus;

		if (activeGroup && activeGroup.m_bQRF)
			return "routing";

		return "guard_center";
	}

	protected string SelectValidGroupPrefabFromList(array<string> prefabs, int seed, string factionKey, string purpose)
	{
		if (!prefabs || prefabs.Count() == 0)
			return "";

		int startIndex = HST_DefaultCatalog.PositiveMod(seed, prefabs.Count());
		for (int offset = 0; offset < prefabs.Count(); offset++)
		{
			int index = HST_DefaultCatalog.PositiveMod(startIndex + offset, prefabs.Count());
			string prefab = prefabs[index];
			if (!IsValidGroupPrefabResource(prefab, factionKey))
				continue;

			return prefab;
		}

		Print(string.Format("h-istasi | no valid %1 group prefab found for faction %2", purpose, factionKey), LogLevel.WARNING);
		return "";
	}

	protected bool IsValidGroupPrefabResource(string prefab, string factionKey)
	{
		if (prefab.IsEmpty())
			return false;

		if (prefab.Contains("_NotSpawned") || prefab.Contains("NotSpawned"))
		{
			Print(string.Format("h-istasi | rejected non-spawning group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		if (prefab.Contains("PlayableGroup.et"))
		{
			Print(string.Format("h-istasi | rejected player placeholder group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			Print(string.Format("h-istasi | rejected missing group prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		return true;
	}

	protected bool IsValidVehiclePrefabResource(string prefab, string factionKey)
	{
		if (prefab.IsEmpty())
			return false;

		if (!HST_VehicleRootPolicy.IsEligibleVehicleRootPrefab(prefab))
		{
			Print(string.Format("h-istasi | rejected invalid vehicle root prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		Resource loaded = Resource.Load(prefab);
		if (!loaded || !loaded.IsValid())
		{
			Print(string.Format("h-istasi | rejected missing vehicle prefab %1 for faction %2", prefab, factionKey), LogLevel.WARNING);
			return false;
		}

		return true;
	}

	protected int ResolveSpawnedAgentCount(GenericEntity entity, HST_ActiveGroupState activeGroup)
	{
		if (!entity || !activeGroup)
			return 0;

		AIGroup group = AIGroup.Cast(entity);
		if (!group)
		{
			Print(string.Format("h-istasi | spawned prefab %1 for %2 did not create an AIGroup", activeGroup.m_sPrefab, activeGroup.m_sGroupId), LogLevel.WARNING);
			return 0;
		}

		int agentCount = group.GetAgentsCount();
		if (agentCount <= 0)
		{
			Print(string.Format("h-istasi | spawned AIGroup %1 for %2 has no agents yet; keeping spawned group entity", activeGroup.m_sPrefab, activeGroup.m_sGroupId), LogLevel.WARNING);
			return 0;
		}

		return agentCount;
	}

	protected void NotifyRuntimeEvent(string eventId, string title, string message, string zoneId, vector position, float durationSeconds)
	{
		string payload = string.Format("HST_NOTIFICATION|%1|enemy|warning|%2|%3|%4||%5|%6", eventId, PayloadText(title), PayloadText(message), zoneId, position, durationSeconds);
		HST_CommandMenuRequestComponent.BroadcastNotification(payload, title + ": " + message);
	}

	protected string PayloadText(string value)
	{
		if (value.IsEmpty())
			return "";

		string result = value;
		result.Replace("|", "/");
		result.Replace("\n", " ");
		return result;
	}

	protected int ResolveActiveInfantryCap(HST_ZoneState zone)
	{
		if (!zone)
			return MAX_ACTIVE_INFANTRY_PER_ZONE;

		int cap = MAX_ACTIVE_INFANTRY_PER_ZONE + Math.Max(0, zone.m_iPriority / 12);
		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			cap += 2;
		return Math.Min(12, cap);
	}

	protected int ResolveActiveVehicleCap(HST_ZoneState zone)
	{
		if (!zone)
			return MAX_ACTIVE_VEHICLES_PER_ZONE;

		if (zone.m_eType == HST_EZoneType.HST_ZONE_AIRFIELD || zone.m_eType == HST_EZoneType.HST_ZONE_SEAPORT)
			return 2;

		return MAX_ACTIVE_VEHICLES_PER_ZONE;
	}

	protected void EnsureRuntimeGroupEntities(HST_CampaignState state)
	{
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || HasRuntimeGroupEntity(activeGroup.m_sGroupId))
				continue;

			if (activeGroup.m_iVehicleCount > 0 && activeGroup.m_iInfantryCount <= 0)
				TrySpawnActiveVehicle(activeGroup, state);
			else
				TrySpawnActiveGroup(activeGroup, state);
		}
	}

	protected bool UpdateRuntimeGroupSurvivors(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || !activeGroup.m_bSpawnedEntity || activeGroup.m_sRuntimeStatus == "folded" || activeGroup.m_sRuntimeStatus == "spawn_failed")
				continue;

			int aliveCount = CountAliveRuntimeGroupAgents(activeGroup.m_sGroupId);
			if (aliveCount <= 0 && activeGroup.m_iSpawnedAgentCount <= 0)
				continue;
			if (aliveCount > 0 && activeGroup.m_iSpawnedAgentCount <= 0)
			{
				activeGroup.m_iSpawnedAgentCount = aliveCount;
				changed = true;
				Print(string.Format("h-istasi | active group populated %1 | zone %2 | live agents %3 | expected infantry %4 vehicles %5 | status %6", activeGroup.m_sGroupId, activeGroup.m_sZoneId, aliveCount, activeGroup.m_iInfantryCount, activeGroup.m_iVehicleCount, activeGroup.m_sRuntimeStatus));
			}
			if (aliveCount == activeGroup.m_iLastSeenAliveCount)
				continue;

			int previousAliveCount = activeGroup.m_iLastSeenAliveCount;
			activeGroup.m_iLastSeenAliveCount = aliveCount;
			if (activeGroup.m_iVehicleCount > 0 && activeGroup.m_iInfantryCount <= 0)
			{
				activeGroup.m_iSurvivorInfantryCount = 0;
				activeGroup.m_iSurvivorVehicleCount = Math.Min(activeGroup.m_iVehicleCount, aliveCount);
			}
			else
			{
				activeGroup.m_iSurvivorInfantryCount = Math.Min(activeGroup.m_iInfantryCount, aliveCount);
				activeGroup.m_iSurvivorVehicleCount = 0;
			}
			if (aliveCount <= 0)
				activeGroup.m_sRuntimeStatus = "eliminated";
			Print(string.Format("h-istasi | active group survivors %1 | zone %2 | alive %3 from %4 | survivors infantry %5/%6 vehicles %7/%8 | status %9", activeGroup.m_sGroupId, activeGroup.m_sZoneId, aliveCount, previousAliveCount, activeGroup.m_iSurvivorInfantryCount, activeGroup.m_iInfantryCount, activeGroup.m_iSurvivorVehicleCount, activeGroup.m_iVehicleCount, activeGroup.m_sRuntimeStatus));
			changed = true;
		}

		if (changed)
		{
			foreach (HST_ZoneState zone : state.m_aZones)
			{
				if (zone && zone.m_bActive)
					ApplyActiveZoneCounts(state, zone);
			}
		}

		return changed;
	}

	protected int CountAliveRuntimeGroupAgents(string groupId)
	{
		int aliveCount;
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId)
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (!entity)
				continue;

			AIGroup group = AIGroup.Cast(entity);
			if (group)
			{
				aliveCount += Math.Max(0, group.GetAgentsCount());
				continue;
			}

			if (IsLivingEntity(entity))
				aliveCount++;
		}

		for (int j = 0; j < m_aRuntimeVehicleGroupIds.Count(); j++)
		{
			if (m_aRuntimeVehicleGroupIds[j] != groupId)
				continue;

			IEntity vehicle = m_aRuntimeVehicleEntities[j];
			if (IsLivingEntity(vehicle))
				aliveCount++;
		}

		return aliveCount;
	}

	protected bool HasActiveGarrisonGroup(HST_CampaignState state, HST_ZoneState zone)
	{
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_bQRF || activeGroup.m_sZoneId != zone.m_sZoneId || activeGroup.m_sFactionKey != zone.m_sOwnerFactionKey)
				continue;
			if (activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == "folded")
				continue;

			if (activeGroup.m_iLastSeenAliveCount > 0 || activeGroup.m_iSurvivorInfantryCount > 0 || activeGroup.m_iSurvivorVehicleCount > 0)
				return true;
		}

		return false;
	}

	protected void ApplyActiveZoneCounts(HST_CampaignState state, HST_ZoneState zone)
	{
		int infantryCount;
		int vehicleCount;
		foreach (HST_ActiveGroupState activeGroup : state.m_aActiveGroups)
		{
			if (!activeGroup || activeGroup.m_bQRF || activeGroup.m_sZoneId != zone.m_sZoneId)
				continue;

			if (activeGroup.m_sRuntimeStatus == "eliminated" || activeGroup.m_sRuntimeStatus == "spawn_failed" || activeGroup.m_sRuntimeStatus == "folded")
				continue;

			infantryCount += Math.Max(0, activeGroup.m_iSurvivorInfantryCount);
			vehicleCount += Math.Max(0, activeGroup.m_iSurvivorVehicleCount);
		}

		zone.m_iActiveInfantryCount = infantryCount;
		zone.m_iActiveVehicleCount = vehicleCount;
	}

	protected void FoldActiveGroup(HST_CampaignState state, HST_ActiveGroupState activeGroup)
	{
		HST_GarrisonState garrison = state.FindGarrison(activeGroup.m_sZoneId, activeGroup.m_sFactionKey);
		if (!garrison)
		{
			garrison = new HST_GarrisonState();
			garrison.m_sZoneId = activeGroup.m_sZoneId;
			garrison.m_sFactionKey = activeGroup.m_sFactionKey;
			state.m_aGarrisons.Insert(garrison);
		}

		int survivorInfantry = activeGroup.m_iSurvivorInfantryCount;
		int survivorVehicles = activeGroup.m_iSurvivorVehicleCount;
		if (activeGroup.m_sRuntimeStatus != "eliminated" && !activeGroup.m_bSpawnedEntity && survivorInfantry <= 0 && activeGroup.m_iInfantryCount > 0)
			survivorInfantry = activeGroup.m_iInfantryCount;
		if (activeGroup.m_sRuntimeStatus != "eliminated" && !activeGroup.m_bSpawnedEntity && survivorVehicles <= 0 && activeGroup.m_iVehicleCount > 0)
			survivorVehicles = activeGroup.m_iVehicleCount;

		string previousStatus = activeGroup.m_sRuntimeStatus;
		activeGroup.m_sRuntimeStatus = "folded";
		garrison.m_iInfantryCount += Math.Max(0, survivorInfantry);
		garrison.m_iVehicleCount += Math.Max(0, survivorVehicles);
		Print(string.Format("h-istasi | folded active group %1 | zone %2 | status %3 | returned infantry %4/%5 vehicles %6/%7 | last alive %8 | spawned agents %9", activeGroup.m_sGroupId, activeGroup.m_sZoneId, previousStatus, survivorInfantry, activeGroup.m_iInfantryCount, survivorVehicles, activeGroup.m_iVehicleCount, activeGroup.m_iLastSeenAliveCount, activeGroup.m_iSpawnedAgentCount));
	}

	protected string ResolveGroupRouteId(HST_ZoneState zone, bool qrf)
	{
		if (!zone)
			return "";

		if (qrf && !zone.m_sQRFRouteId.IsEmpty())
			return zone.m_sQRFRouteId;

		return zone.m_sPatrolRouteId;
	}

	protected vector ResolveGroupSourcePosition(HST_CampaignState state, HST_ZoneState zone, string routeId, bool qrf)
	{
		HST_GeneratedRouteState route;
		if (state && !routeId.IsEmpty())
			route = state.FindGeneratedRoute(routeId);

		if (route)
		{
			if (qrf)
				return route.m_vStartPosition;

			return route.m_vMidPosition;
		}

		if (zone)
			return zone.m_vPosition;

		return "0 0 0";
	}

	protected vector ResolveGroupTargetPosition(HST_CampaignState state, HST_ZoneState zone, string routeId, bool qrf)
	{
		HST_GeneratedRouteState route;
		if (state && !routeId.IsEmpty())
			route = state.FindGeneratedRoute(routeId);

		if (route)
			return route.m_vEndPosition;

		if (zone)
			return zone.m_vPosition;

		return "0 0 0";
	}

	protected bool HasRuntimeGroupEntity(string groupId)
	{
		return m_aRuntimeGroupIds.Find(groupId) >= 0 || m_aRuntimeVehicleGroupIds.Find(groupId) >= 0;
	}

	protected IEntity GetRuntimeGroupEntity(string groupId)
	{
		int index = m_aRuntimeGroupIds.Find(groupId);
		if (index < 0 || index >= m_aRuntimeGroupEntities.Count())
		{
			int vehicleIndex = m_aRuntimeVehicleGroupIds.Find(groupId);
			if (vehicleIndex < 0 || vehicleIndex >= m_aRuntimeVehicleEntities.Count())
				return null;

			return m_aRuntimeVehicleEntities[vehicleIndex];
		}

		return m_aRuntimeGroupEntities[index];
	}

	protected void SetRuntimeGroupEntitiesOrigin(string groupId, vector position)
	{
		for (int i = 0; i < m_aRuntimeGroupIds.Count(); i++)
		{
			if (m_aRuntimeGroupIds[i] != groupId)
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (entity)
				entity.SetOrigin(position);
		}
	}

	protected void DeleteRuntimeGroupEntity(string groupId)
	{
		for (int i = m_aRuntimeGroupIds.Count() - 1; i >= 0; i--)
		{
			if (m_aRuntimeGroupIds[i] != groupId)
				continue;

			IEntity entity = m_aRuntimeGroupEntities[i];
			if (entity)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);

			m_aRuntimeGroupIds.Remove(i);
			m_aRuntimeGroupEntities.Remove(i);
		}

		for (int j = m_aRuntimeVehicleGroupIds.Count() - 1; j >= 0; j--)
		{
			if (m_aRuntimeVehicleGroupIds[j] != groupId)
				continue;

			IEntity vehicle = m_aRuntimeVehicleEntities[j];
			if (vehicle)
				SCR_EntityHelper.DeleteEntityAndChildren(vehicle);

			m_aRuntimeVehicleGroupIds.Remove(j);
			m_aRuntimeVehicleEntities.Remove(j);
		}
	}

	protected bool IsZoneInsideHQSafeArea(HST_CampaignState state, HST_ZoneState zone)
	{
		if (!state || !zone || !state.m_bHQDeployed)
			return false;

		return DistanceSq2D(state.m_vHQPosition, zone.m_vPosition) <= HQ_SAFE_RADIUS_METERS * HQ_SAFE_RADIUS_METERS;
	}

	protected IEntity GetBestPlayerEntity(PlayerManager playerManager, int playerId)
	{
		if (!playerManager || playerId <= 0)
			return null;

		IEntity controlledEntity = playerManager.GetPlayerControlledEntity(playerId);
		if (controlledEntity)
			return controlledEntity;

		return SCR_PossessingManagerComponent.GetPlayerMainEntity(playerId);
	}

	protected bool IsLivingPlayerEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected bool IsLivingEntity(IEntity entity)
	{
		if (!entity)
			return false;

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(entity.FindComponent(SCR_DamageManagerComponent));
		return !damageManager || damageManager.GetState() != EDamageState.DESTROYED;
	}

	protected float DistanceSq2D(vector a, vector b)
	{
		float x = a[0] - b[0];
		float z = a[2] - b[2];
		return x * x + z * z;
	}
}
