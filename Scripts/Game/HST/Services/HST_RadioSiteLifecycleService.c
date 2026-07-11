class HST_RadioSiteTransitionResult
{
	bool m_bAccepted;
	bool m_bAlreadyApplied;
	bool m_bStateChanged;
	string m_sReason;
	HST_RadioSiteState m_Site;
	HST_ActiveMissionState m_Mission;
	HST_MissionAssetState m_Asset;
}

// Schema-59 is the single authority for radio transmitter identity, lifecycle,
// projection ownership, and the two exact radio mission transitions. A live
// entity is evidence/projection only; it never owns the durable outcome.
class HST_RadioSiteLifecycleService
{
	static const int EXACT_CONTRACT_VERSION = 1;
	static const int QUARANTINED_CONTRACT_VERSION = -59;
	static const int SCHEMA_VERSION = 59;
	static const string DESTROY_MISSION_ID = "destroy_radio_tower";
	static const string REBUILD_MISSION_ID = "dynamic_stop_tower_rebuild";
	static const string GENERATED_TOWER_PREFAB = "{6985327711303710}Prefabs/Objects/HST/HST_MissionProp_DestroyTarget.et";
	static const string REBUILD_EQUIPMENT_PREFAB = "{6985327711303940}Prefabs/Objects/HST/HST_RadioRebuildEquipment.et";
	static const string TARGET_KIND = "target";
	static const string TARGET_ROLE = "destroy_target";
	static const string TRANSITION_DESTROY_ADMISSION = "destroy_admission";
	static const string TRANSITION_DESTROY_SUCCESS = "destroy_success";
	static const string TRANSITION_DESTROY_FAILURE = "destroy_failure";
	static const string TRANSITION_DESTROY_EXPIRY = "destroy_expiry";
	static const string TRANSITION_REBUILD_ADMISSION = "rebuild_admission";
	static const string TRANSITION_REBUILD_SUCCESS = "rebuild_success";
	static const string TRANSITION_REBUILD_FAILURE = "rebuild_failure";
	static const string TRANSITION_REBUILD_EXPIRY = "rebuild_expiry";
	static const string TRANSITION_CAMPAIGN_STOP_DESTROY = "campaign_stop_destroy";
	static const string TRANSITION_CAMPAIGN_STOP_REBUILD = "campaign_stop_rebuild";
	static const float BINDING_SEARCH_RADIUS_METERS = 220.0;
	static const float FROZEN_BINDING_SEARCH_RADIUS_METERS = 30.0;
	static const float AUTHORED_SUPPRESSION_RADIUS_METERS = 24.0;
	static const float BINDING_POSITION_TOLERANCE_METERS = 0.75;
	// Safe-ground placement may use the fourth 2m clearance ring, including a
	// diagonal candidate. Twelve metres contains that bounded projection offset;
	// the authored-world binding remains independently frozen to 0.75m.
	static const float PHYSICAL_EVIDENCE_POSITION_TOLERANCE_METERS = 12.0;
	static const float DEFAULT_DEMOLITION_REQUIRED_DAMAGE = 300.0;
	static const int MAX_DEMOLITION_EVIDENCE_KEYS = 64;

	protected ref array<string> m_aProjectionSiteIds = {};
	protected ref array<string> m_aProjectionMissionInstanceIds = {};
	protected ref array<IEntity> m_aProjectionEntities = {};
	protected ref array<bool> m_aProjectionBorrowed = {};
	protected ref array<IEntity> m_aTransmitterCandidates = {};
	protected string m_sCandidateExpectedSiteId;
	// Authored transmitter discovery is intentionally amortized. A campaign can
	// contain many unresolved logical sites, and each discovery attempt performs
	// a bounded world query around the zone.
	protected int m_iNextUnresolvedProjectionIndex;

	static string BuildSiteId(string zoneId)
	{
		if (zoneId.IsEmpty())
			return "";
		return "radio_site_" + zoneId;
	}

	static string BuildTargetId(string zoneId)
	{
		if (zoneId.IsEmpty())
			return "";
		return "radio_target_" + zoneId;
	}

	static string BuildAdmissionRequestId(string siteId, string missionInstanceId, string missionId)
	{
		if (siteId.IsEmpty() || missionInstanceId.IsEmpty() || missionId.IsEmpty())
			return "";
		return "radio_admission_" + siteId + "_" + missionId + "_" + missionInstanceId;
	}

	static string BuildOutcomeRequestId(string siteId, string missionInstanceId, string missionId, string outcomeKind)
	{
		if (siteId.IsEmpty() || missionInstanceId.IsEmpty() || missionId.IsEmpty() || outcomeKind.IsEmpty())
			return "";
		return "radio_outcome_" + siteId + "_" + missionId + "_" + missionInstanceId + "_" + outcomeKind;
	}

	static string BuildDestructionReceiptId(string siteId, string missionInstanceId)
	{
		if (siteId.IsEmpty() || missionInstanceId.IsEmpty())
			return "";
		return "radio_destruction_" + siteId + "_" + missionInstanceId;
	}

	static string BuildRebuildReceiptId(string siteId, string missionInstanceId)
	{
		if (siteId.IsEmpty() || missionInstanceId.IsEmpty())
			return "";
		return "radio_rebuild_" + siteId + "_" + missionInstanceId;
	}

	static bool IsSupportedMissionId(string missionId)
	{
		return missionId == DESTROY_MISSION_ID || missionId == REBUILD_MISSION_ID;
	}

	static bool IsExactMission(HST_ActiveMissionState mission)
	{
		return mission && IsSupportedMissionId(mission.m_sMissionId)
			&& mission.m_iRadioSiteContractVersion == EXACT_CONTRACT_VERSION;
	}

	static bool IsQuarantinedMission(HST_ActiveMissionState mission)
	{
		return mission && IsSupportedMissionId(mission.m_sMissionId)
			&& mission.m_iRadioSiteContractVersion == QUARANTINED_CONTRACT_VERSION;
	}

	static bool IsManagedOrQuarantinedMission(HST_ActiveMissionState mission)
	{
		return IsExactMission(mission) || IsQuarantinedMission(mission);
	}

	static bool IsManagedOrQuarantinedAsset(HST_MissionAssetState asset)
	{
		return asset && (asset.m_iRadioSiteContractVersion == EXACT_CONTRACT_VERSION
			|| asset.m_iRadioSiteContractVersion == QUARANTINED_CONTRACT_VERSION);
	}

	static bool IsExactSiteZone(HST_CampaignState state, string zoneId)
	{
		if (!state || zoneId.IsEmpty())
			return false;
		HST_RadioSiteState site = state.FindRadioSiteForZone(zoneId);
		return site && (site.m_iContractVersion == EXACT_CONTRACT_VERSION
			|| site.m_iContractVersion == QUARANTINED_CONTRACT_VERSION);
	}

	static bool IsBroadcastOperational(HST_CampaignState state, string zoneId)
	{
		if (!state || zoneId.IsEmpty())
			return false;
		HST_RadioSiteState site = state.FindRadioSiteForZone(zoneId);
		return site && site.m_iContractVersion == EXACT_CONTRACT_VERSION
			&& site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
			&& site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED
			&& !site.m_sTargetPrefab.IsEmpty() && !IsZeroVectorStatic(site.m_vTargetPosition);
	}

	static string BuildStatusText(HST_CampaignState state, string zoneId)
	{
		if (!state || zoneId.IsEmpty())
			return "UNRESOLVED";
		HST_RadioSiteState site = state.FindRadioSiteForZone(zoneId);
		if (!site)
			return "UNRESOLVED";
		if (site.m_iContractVersion == QUARANTINED_CONTRACT_VERSION
			|| site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_QUARANTINED)
			return "QUARANTINED";
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED)
			return "UNRESOLVED";
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED)
			return "DESTROYED";
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING)
			return "REBUILDING";
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE)
			return "ONLINE";
		return "UNRESOLVED";
	}

	static bool IsSupportedTransmitterPrefab(string prefab)
	{
		if (prefab == GENERATED_TOWER_PREFAB)
			return true;
		if (prefab.IsEmpty() || !prefab.Contains("TransmitterTower_01"))
			return false;
		if (prefab.Contains("TransmitterTower_01_light") || prefab.Contains("_base.et"))
			return false;
		return true;
	}

	bool EnsureSites(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_ZoneState zone : state.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_RADIO_TOWER || zone.m_sZoneId.IsEmpty())
				continue;

			int count = CountSitesForZone(state, zone.m_sZoneId);
			if (count == 0)
			{
				HST_RadioSiteState site = CreateLogicalSite(state, zone);
				state.m_aRadioSites.Insert(site);
				changed = true;
				continue;
			}
			if (count <= 1)
				continue;

			foreach (HST_RadioSiteState duplicate : state.m_aRadioSites)
			{
				if (duplicate && duplicate.m_sZoneId == zone.m_sZoneId)
					changed = QuarantineSite(state, duplicate, "duplicate durable radio-site rows claim one zone") || changed;
			}
		}
		return changed;
	}

	bool ReconcileAfterRestore(HST_CampaignState state)
	{
		if (!state)
			return false;
		ClearTrackedProjections(true);
		bool changed = EnsureSites(state);
		return ReconcileProjections(state) || changed;
	}

	bool PrepareForNewCampaignReset(
		HST_CampaignState oldState,
		out string failureReason)
	{
		failureReason = "";
		if (!oldState)
		{
			failureReason = "old campaign radio-site state is unavailable";
			return false;
		}
		// An explicit new-campaign reset owns restoration of borrowed authored
		// health and retirement of projections generated by the old campaign. Build
		// the complete restoration set before mutating any world entity.
		array<IEntity> authoredCandidates = {};
		array<SCR_DamageManagerComponent> authoredDamageManagers = {};
		array<float> authoredPriorHealth = {};
		array<EDamageState> authoredPriorStates = {};
		foreach (HST_RadioSiteState site : oldState.m_aRadioSites)
		{
			if (!site
				|| site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED)
				continue;
			if (site.m_sAuthoredTargetPrefab.IsEmpty()
				|| IsZeroVectorStatic(site.m_vAuthoredTargetPosition))
			{
				failureReason = "new campaign reset requires frozen authored transmitter provenance for every prior resolved radio site";
				return false;
			}

			IEntity authored;
			if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD)
				authored = FindProjection(site.m_sSiteId);
			if (!authored)
			{
				CollectTransmitterCandidates(
					site.m_vAuthoredTargetPosition,
					FROZEN_BINDING_SEARCH_RADIUS_METERS,
					site.m_sSiteId);
				if (HasCandidateClaimedByOtherSite(site.m_sSiteId)
					|| m_aTransmitterCandidates.Count() != 1)
				{
					failureReason = "new campaign reset requires one unambiguous authored transmitter per prior resolved radio site";
					return false;
				}
				authored = m_aTransmitterCandidates[0];
			}
			if (!FrozenAuthoredCandidateMatchesSite(site, authored))
			{
				failureReason = "new campaign reset could not recover a frozen authored transmitter binding";
				return false;
			}
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(
				authored.FindComponent(SCR_DamageManagerComponent));
			if (authoredCandidates.Contains(authored) || !damageManager)
			{
				failureReason = "new campaign reset authored transmitter set is duplicated or not damageable";
				return false;
			}
			authoredCandidates.Insert(authored);
			authoredDamageManagers.Insert(damageManager);
			authoredPriorHealth.Insert(damageManager.GetHealthScaled());
			authoredPriorStates.Insert(damageManager.GetState());
		}

		bool restorationExact = true;
		foreach (SCR_DamageManagerComponent damageManager : authoredDamageManagers)
		{
			if (!damageManager.SetHealthScaled(1.0)
				|| damageManager.GetState() == EDamageState.DESTROYED
				|| !float.AlmostEqual(damageManager.GetHealthScaled(), 1.0, 0.001))
			{
				restorationExact = false;
				break;
			}
		}
		if (!restorationExact)
		{
			bool rollbackExact = true;
			for (int restoreIndex = 0; restoreIndex < authoredDamageManagers.Count(); restoreIndex++)
			{
				SCR_DamageManagerComponent rollbackManager = authoredDamageManagers[restoreIndex];
				if (!rollbackManager.SetHealthScaled(authoredPriorHealth[restoreIndex])
					|| rollbackManager.GetState() != authoredPriorStates[restoreIndex]
					|| !float.AlmostEqual(
						rollbackManager.GetHealthScaled(),
						authoredPriorHealth[restoreIndex],
						0.001))
					rollbackExact = false;
			}
			failureReason = "new campaign reset could not restore every authored transmitter to an operational damage state";
			if (!rollbackExact)
				failureReason = failureReason + "; prior physical damage state rollback was not exact";
			return false;
		}
		ClearTrackedProjections(true);
		return true;
	}

	bool ReconcileProjections(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed;
		foreach (HST_RadioSiteState site : state.m_aRadioSites)
		{
			if (!site || site.m_iContractVersion != EXACT_CONTRACT_VERSION)
				continue;
			// Periodic authored-world discovery is owned by the round-robin budget in
			// TickBeforeMissionRuntime. This post-mission pass still reconciles every
			// already-bound or campaign-generated projection immediately.
			if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED)
				continue;
			changed = EnsureSiteProjection(state, site) || changed;
		}
		return changed;
	}

	protected HST_RadioSiteState CreateLogicalSite(HST_CampaignState state, HST_ZoneState zone)
	{
		HST_RadioSiteState site = new HST_RadioSiteState();
		site.m_iContractVersion = EXACT_CONTRACT_VERSION;
		site.m_sSiteId = BuildSiteId(zone.m_sZoneId);
		site.m_sZoneId = zone.m_sZoneId;
		site.m_sTargetId = BuildTargetId(zone.m_sZoneId);
		site.m_eLifecycleState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
		site.m_eTargetOwnership = HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED;
		site.m_sLastTransitionReason = "schema-59 logical radio site created without inventing a physical binding";
		site.m_iLastTransitionSecond = Math.Max(0, state.m_iElapsedSeconds);
		site.m_iRevision = 1;
		return site;
	}

	bool CanStartMission(HST_CampaignState state, string missionId, string targetZoneId, out string failureReason)
	{
		failureReason = "";
		if (!IsSupportedMissionId(missionId))
			return true;
		if (!state || targetZoneId.IsEmpty())
		{
			failureReason = "exact radio mission requires a radio-site target";
			return false;
		}

		HST_RadioSiteState site = state.FindRadioSiteForZone(targetZoneId);
		if (!site || site.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_QUARANTINED)
		{
			failureReason = "radio-site authority is missing, ambiguous, or quarantined";
			return false;
		}
		if (!site.m_sActiveMissionInstanceId.IsEmpty())
		{
			failureReason = "radio site already has an active lifecycle mission";
			return false;
		}
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED)
		{
			EnsureSiteProjection(state, site);
			if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED)
			{
				failureReason = "radio-site transmitter has not been bound to an authored world target";
				return false;
			}
		}

		if (missionId == DESTROY_MISSION_ID)
		{
			if (site.m_eLifecycleState != HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE)
			{
				failureReason = "destroy mission requires an ONLINE radio site";
				return false;
			}
			EnsureSiteProjection(state, site);
			if (!FindProjection(site.m_sSiteId))
			{
				failureReason = "ONLINE radio transmitter projection is not currently authoritative";
				return false;
			}
			return true;
		}

		if (site.m_eLifecycleState != HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED
			|| site.m_sLastDestructionReceiptId.IsEmpty())
		{
			failureReason = "stop-rebuild mission requires a durably DESTROYED radio site";
			return false;
		}
		if (!site.m_sLastRebuildReceiptId.IsEmpty())
		{
			failureReason = "this destruction epoch already consumed its one rebuild attempt";
			return false;
		}
		return true;
	}

	bool PrepareNewMissionContract(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !mission || !IsSupportedMissionId(mission.m_sMissionId))
			return false;

		HST_RadioSiteState site = state.FindRadioSiteForZone(mission.m_sTargetZoneId);
		if (!site || site.m_iContractVersion != EXACT_CONTRACT_VERSION)
		{
			mission.m_iRadioSiteContractVersion = QUARANTINED_CONTRACT_VERSION;
			mission.m_sRuntimePhase = "radio_site_authority_quarantined";
			mission.m_sRuntimeFailureReason = "exact radio mission could not claim one durable radio site";
			return false;
		}

		mission.m_iRadioSiteContractVersion = EXACT_CONTRACT_VERSION;
		mission.m_sRadioSiteId = site.m_sSiteId;
		mission.m_sRadioSiteTransitionRequestId = BuildAdmissionRequestId(
			site.m_sSiteId,
			mission.m_sInstanceId,
			mission.m_sMissionId);
		mission.m_iRadioSiteRevision = 0;
		return !mission.m_sRadioSiteTransitionRequestId.IsEmpty();
	}

	HST_RadioSiteTransitionResult AdmitNewMission(
		HST_CampaignState state,
		HST_ActiveMissionState mission)
	{
		HST_RadioSiteTransitionResult result = NewResult(mission);
		if (!state || !IsExactMission(mission))
			return Reject(result, "mission does not carry the exact radio-site contract");

		HST_RadioSiteState site = state.FindRadioSite(mission.m_sRadioSiteId);
		result.m_Site = site;
		if (!site || site.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| site.m_sZoneId != mission.m_sTargetZoneId)
			return QuarantineAdmission(state, result, "mission/site backlink is missing or ambiguous");

		string expectedRequestId = BuildAdmissionRequestId(site.m_sSiteId, mission.m_sInstanceId, mission.m_sMissionId);
		if (mission.m_sRadioSiteTransitionRequestId != expectedRequestId)
			return QuarantineAdmission(state, result, "mission admission request is not deterministic");

		string transitionKind = TRANSITION_DESTROY_ADMISSION;
		HST_ERadioSiteLifecycleState fromState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
		HST_ERadioSiteLifecycleState toState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
		if (mission.m_sMissionId == REBUILD_MISSION_ID)
		{
			transitionKind = TRANSITION_REBUILD_ADMISSION;
			fromState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED;
			toState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING;
		}

		if (site.m_sActiveTransitionRequestId == expectedRequestId)
		{
			if (AdmissionFingerprintMatches(site, mission, transitionKind, fromState, toState))
			{
				string replayFailure;
				if (!ValidateReplayAggregate(state, site, replayFailure))
					return QuarantineAdmission(state, result, "admission replay aggregate is invalid: " + replayFailure);
				HST_MissionAssetState replayAsset = FindExactMissionAsset(state, mission.m_sInstanceId);
				if (!ReconcileAdmissionProjection(state, site, mission, replayAsset))
					return QuarantineAdmission(state, result, "admission replay lost its exact target projection");
				result.m_bAccepted = true;
				result.m_bAlreadyApplied = true;
				result.m_Asset = replayAsset;
				return result;
			}
			return QuarantineAdmission(state, result, "admission request replay changed its typed transition fingerprint");
		}
		if (!site.m_sActiveMissionInstanceId.IsEmpty())
			return Reject(result, "radio site already has an active lifecycle lock");
		if (site.m_eLifecycleState != fromState)
			return Reject(result, "radio-site lifecycle no longer permits this mission admission");
		if (mission.m_sMissionId == REBUILD_MISSION_ID
			&& (site.m_sLastDestructionReceiptId.IsEmpty()
				|| !site.m_sLastRebuildReceiptId.IsEmpty()))
			return QuarantineAdmission(
				state,
				result,
				"rebuild admission bypassed the one-attempt destruction-epoch contract");
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED)
			return Reject(result, "radio-site physical target is unresolved");
		if (CountMissionAssets(state, mission.m_sInstanceId) > 0)
			return QuarantineAdmission(state, result, "exact radio mission already owns unexpected target assets");

		if (!PrepareProjectionForAdmission(state, site, mission))
			return Reject(result, "radio-site target projection is unavailable for exact admission");
		if (mission.m_sMissionId == REBUILD_MISSION_ID)
		{
			// Ownership moves now: the failed/expired rebuild path will create the
			// replacement transmitter, while the active mission projects equipment.
			ForgetProjection(site.m_sSiteId, false);
			site.m_eTargetOwnership = HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN;
			site.m_sTargetPrefab = GENERATED_TOWER_PREFAB;
			site.m_iRebuildStartedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
			site.m_sLastRebuildReceiptId = "";
			site.m_sLastRebuildMissionInstanceId = "";
			site.m_iRebuiltAtSecond = 0;
		}

		ApplyAcceptedTransition(
			state,
			site,
			mission,
			expectedRequestId,
			transitionKind,
			fromState,
			toState,
			"exact radio mission admission");
		HST_MissionAssetState asset = CreateMissionTargetAggregate(state, site, mission);
		result.m_Asset = asset;
		bool projectionReady = ReconcileAdmissionProjection(state, site, mission, asset);
		if (!projectionReady || site.m_iContractVersion != EXACT_CONTRACT_VERSION
			|| !IsExactMission(mission) || !asset
			|| asset.m_iRadioSiteContractVersion != EXACT_CONTRACT_VERSION)
			return QuarantineAdmission(state, result, "radio-site admission could not publish one exact target projection");
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		result.m_sReason = "exact radio mission admitted";
		return result;
	}

	bool TickBeforeMissionRuntime(HST_CampaignState state)
	{
		if (!state)
			return false;

		bool changed = EnsureSites(state);
		changed = ReconcileOneUnresolvedProjection(state) || changed;
		foreach (HST_RadioSiteState site : state.m_aRadioSites)
		{
			if (!site || site.m_iContractVersion != EXACT_CONTRACT_VERSION)
				continue;
			if (site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED)
				changed = EnsureSiteProjection(state, site) || changed;
			if (site.m_iContractVersion != EXACT_CONTRACT_VERSION)
				continue;
			if (site.m_sActiveMissionInstanceId.IsEmpty())
				continue;

			HST_ActiveMissionState mission = state.FindActiveMission(site.m_sActiveMissionInstanceId);
			HST_MissionAssetState asset = FindExactMissionAsset(state, site.m_sActiveMissionInstanceId);
			if (!mission || !asset || !IsExactMission(mission)
				|| mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			{
				changed = QuarantineSite(state, site, "active radio-site lock lost its exact mission or target asset") || changed;
				continue;
			}
			if (!MissionOwnsCurrentSiteLock(state, site, mission, asset))
			{
				changed = QuarantineMissionAggregate(
					state,
					mission,
					"active radio-site damage polling lost its reciprocal site lock") || changed;
				continue;
			}

			IEntity projection = FindProjection(site.m_sSiteId);
			if (!projection || asset.m_bDestroyed)
				continue;
			SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(
				projection.FindComponent(SCR_DamageManagerComponent));
			if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
			{
				if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD
					&& mission.m_sMissionId == DESTROY_MISSION_ID)
				{
					if (MarkPhysicalTargetDestroyed(state, site, mission, asset, projection.GetOrigin(), "authoritative borrowed-target damage state"))
						changed = true;
					else
						changed = QuarantineMissionAggregate(
							state,
							mission,
							"borrowed target destruction could not commit its reciprocal runtime position") || changed;
				}
				else
					changed = ResetOwnedProjectionAfterUnsupportedDamage(state, site, mission, asset) || changed;
			}
		}
		return changed;
	}

	protected bool ReconcileOneUnresolvedProjection(HST_CampaignState state)
	{
		if (!state || state.m_aRadioSites.IsEmpty())
			return false;

		int siteCount = state.m_aRadioSites.Count();
		int startIndex = m_iNextUnresolvedProjectionIndex;
		if (startIndex < 0 || startIndex >= siteCount)
			startIndex = 0;
		for (int offset = 0; offset < siteCount; offset++)
		{
			int siteIndex = (startIndex + offset) % siteCount;
			HST_RadioSiteState site = state.m_aRadioSites[siteIndex];
			if (!site || site.m_iContractVersion != EXACT_CONTRACT_VERSION
				|| site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED)
				continue;

			m_iNextUnresolvedProjectionIndex = (siteIndex + 1) % siteCount;
			return EnsureSiteProjection(state, site);
		}

		m_iNextUnresolvedProjectionIndex = 0;
		return false;
	}

	string FindCompletedActiveMissionId(HST_CampaignState state)
	{
		if (!state)
			return "";
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMission(mission) || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			if (CanCompleteMission(state, mission))
				return mission.m_sInstanceId;
		}
		return "";
	}

	bool CanCompleteMission(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!state || !IsExactMission(mission)
			|| mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
			return false;
		HST_RadioSiteState site = state.FindRadioSite(mission.m_sRadioSiteId);
		HST_MissionAssetState asset = FindExactMissionAsset(state, mission.m_sInstanceId);
		if (!MissionOwnsCurrentSiteLock(state, site, mission, asset))
			return false;
		return asset.m_bDestroyed && !asset.m_bAlive && IsDestroyObjectiveComplete(state, mission.m_sInstanceId);
	}

	protected bool MissionOwnsCurrentSiteLock(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset)
	{
		if (!state || !site || !mission || !asset)
			return false;
		bool contractsExact = site.m_iContractVersion == EXACT_CONTRACT_VERSION
			&& IsExactMission(mission)
			&& asset.m_iRadioSiteContractVersion == EXACT_CONTRACT_VERSION;
		bool aggregateBacklinks = asset.m_sRadioSiteId == site.m_sSiteId
			&& asset.m_sMissionInstanceId == mission.m_sInstanceId
			&& mission.m_sRadioSiteId == site.m_sSiteId
			&& mission.m_sTargetZoneId == site.m_sZoneId;
		bool lockExact = site.m_sActiveMissionInstanceId == mission.m_sInstanceId
			&& site.m_sActiveMissionId == mission.m_sMissionId
			&& site.m_sActiveTransitionRequestId == mission.m_sRadioSiteTransitionRequestId
			&& site.m_iRevision == mission.m_iRadioSiteRevision;
		bool ownershipSnapshotExact = asset.m_eRadioSiteTargetOwnership == site.m_eTargetOwnership
			&& asset.m_sRadioSiteAuthoredTargetPrefab == site.m_sAuthoredTargetPrefab
			&& PositionsMatch2D(
				asset.m_vRadioSiteAuthoredTargetPosition,
				site.m_vAuthoredTargetPosition,
				BINDING_POSITION_TOLERANCE_METERS);
		return contractsExact && aggregateBacklinks && lockExact && ownershipSnapshotExact
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE;
	}

	protected bool MissionAggregateReadyForSettlement(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		bool successOutcome)
	{
		if (!state || !site || !mission || !asset)
			return false;
		if (asset.m_iRadioSiteContractVersion != EXACT_CONTRACT_VERSION
			|| asset.m_sRadioSiteId != site.m_sSiteId
			|| asset.m_sMissionInstanceId != mission.m_sInstanceId
			|| mission.m_sTargetZoneId != site.m_sZoneId)
			return false;
		if (asset.m_sAssetId != "asset_" + mission.m_sInstanceId + "_destroy_target_0"
			|| asset.m_sEntityId != asset.m_sAssetId + "_entity"
			|| asset.m_sKind != TARGET_KIND || asset.m_sRole != TARGET_ROLE
			|| CountMissionAssets(state, mission.m_sInstanceId) != 1)
			return false;
		if (asset.m_eRadioSiteTargetOwnership != site.m_eTargetOwnership
			|| asset.m_sRadioSiteAuthoredTargetPrefab != site.m_sAuthoredTargetPrefab
			|| !PositionsMatch2D(
				asset.m_vRadioSiteAuthoredTargetPosition,
				site.m_vAuthoredTargetPosition,
				BINDING_POSITION_TOLERANCE_METERS))
			return false;
		if (!IsSupportedTransmitterPrefab(site.m_sAuthoredTargetPrefab)
			|| site.m_sAuthoredTargetPrefab == GENERATED_TOWER_PREFAB
			|| IsZeroVectorStatic(site.m_vAuthoredTargetPosition))
			return false;
		string expectedSitePrefab = asset.m_sRadioSiteAuthoredTargetPrefab;
		if (asset.m_eRadioSiteTargetOwnership
			== HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
			expectedSitePrefab = GENERATED_TOWER_PREFAB;
		else if (asset.m_eRadioSiteTargetOwnership
			!= HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD)
			return false;
		if (site.m_sTargetPrefab != expectedSitePrefab
			|| !PositionsMatch2D(
				site.m_vTargetPosition,
				site.m_vAuthoredTargetPosition,
				BINDING_POSITION_TOLERANCE_METERS))
			return false;
		string expectedPrefab = expectedSitePrefab;
		if (mission.m_sMissionId == REBUILD_MISSION_ID)
			expectedPrefab = REBUILD_EQUIPMENT_PREFAB;
		if (asset.m_sPrefab != expectedPrefab
			|| !PositionsMatch2D(asset.m_vSourcePosition, site.m_vTargetPosition, BINDING_POSITION_TOLERANCE_METERS))
			return false;

		HST_MissionRuntimeEntityState runtime = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtime || CountMissionRuntimeEntities(state, mission.m_sInstanceId) != 1
			|| runtime.m_sMissionInstanceId != mission.m_sInstanceId
			|| runtime.m_sKind != "radio_site_target"
			|| runtime.m_sPrefab != asset.m_sPrefab
			|| runtime.m_bSpawned != asset.m_bSpawned
			|| runtime.m_bDestroyed != asset.m_bDestroyed)
			return false;

		HST_MissionObjectiveState objective = FindExactMissionObjective(state, mission.m_sInstanceId);
		HST_CampaignTaskState task = state.FindCampaignTask("task_" + mission.m_sInstanceId);
		if (!objective || CountMissionObjectives(state, mission.m_sInstanceId) != 1
			|| objective.m_eType != HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET
			|| objective.m_sTargetId != site.m_sTargetId
			|| objective.m_sTargetZoneId != site.m_sZoneId
			|| objective.m_sPhysicalEntityId != asset.m_sEntityId
			|| objective.m_sLinkedRuntimeEntityId != asset.m_sEntityId
			|| !task || CountMissionTasks(state, mission.m_sInstanceId) != 1
			|| task.m_sLinkedId != mission.m_sInstanceId
			|| !task.m_bActive || task.m_bSucceeded || task.m_bFailed)
			return false;
		if (successOutcome)
			return CanCompleteMissionAfterStatusCommit(state, mission, asset);
		return !asset.m_bDestroyed && asset.m_bAlive && !asset.m_bOutcomeApplied
			&& asset.m_sOutcomeKind.IsEmpty() && !objective.m_bComplete
			&& mission.m_iRuntimeDestroyedCount == 0;
	}

	bool HandleAssetDestroyed(
		HST_CampaignState state,
		string assetId,
		vector position,
		out string resultText,
		out string missionInstanceId)
	{
		resultText = "h-istasi radio site | target not managed by exact radio authority";
		missionInstanceId = "";
		if (!state || assetId.IsEmpty())
			return false;

		HST_MissionAssetState asset = state.FindMissionAsset(assetId);
		if (!asset || !IsManagedOrQuarantinedAsset(asset))
			return false;
		missionInstanceId = asset.m_sMissionInstanceId;
		HST_ActiveMissionState mission = state.FindActiveMission(missionInstanceId);
		HST_RadioSiteState site = state.FindRadioSite(asset.m_sRadioSiteId);
		if (!IsExactMission(mission) || !site || site.m_iContractVersion != EXACT_CONTRACT_VERSION)
		{
			resultText = "h-istasi radio site | exact target authority is quarantined";
			return false;
		}
		if (asset.m_bDestroyed)
		{
			resultText = "h-istasi radio site | target destruction already recorded";
			return false;
		}
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
		{
			resultText = "h-istasi radio site | ignored stale destruction evidence after mission settlement";
			return false;
		}
		if (!MissionOwnsCurrentSiteLock(state, site, mission, asset))
		{
			resultText = "h-istasi radio site | stale or cross-linked destruction evidence quarantined";
			return QuarantineMissionAggregate(state, mission, "direct destruction evidence lost its reciprocal radio-site lock");
		}
		if (site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD
			|| mission.m_sMissionId != DESTROY_MISSION_ID)
		{
			resultText = "h-istasi radio site | generated targets require exact explosive-score evidence";
			return false;
		}

		IEntity projection = FindProjection(site.m_sSiteId);
		SCR_DamageManagerComponent damageManager;
		if (projection)
			damageManager = SCR_DamageManagerComponent.Cast(projection.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager || damageManager.GetState() != EDamageState.DESTROYED)
		{
			resultText = "h-istasi radio site | direct destruction rejected without authoritative physical damage state";
			return false;
		}

		vector projectionPosition = projection.GetOrigin();
		if ((!IsZeroVectorStatic(position)
				&& !PositionsMatch2D(position, projectionPosition, PHYSICAL_EVIDENCE_POSITION_TOLERANCE_METERS))
			|| !PositionsMatch2D(
				projectionPosition,
				site.m_vTargetPosition,
				PHYSICAL_EVIDENCE_POSITION_TOLERANCE_METERS))
		{
			resultText = "h-istasi radio site | direct destruction position does not match the verified target";
			return false;
		}
		bool changed = MarkPhysicalTargetDestroyed(
			state,
			site,
			mission,
			asset,
			projectionPosition,
			"authoritative physical destruction callback");
		if (changed)
			resultText = "h-istasi radio site | exact target physically destroyed";
		if (changed)
			return true;
		resultText = "h-istasi radio site | physical destruction could not commit its reciprocal runtime state";
		return QuarantineMissionAggregate(
			state,
			mission,
			"borrowed target destruction could not commit its reciprocal runtime position");
	}

	bool ApplyExplosiveDamage(
		HST_CampaignState state,
		string assetId,
		vector position,
		float damage,
		string sourceLabel,
		out string resultText,
		out string missionInstanceId)
	{
		resultText = "h-istasi radio site | target not managed by exact radio authority";
		missionInstanceId = "";
		if (!state || assetId.IsEmpty() || damage <= 0.0 || sourceLabel.Trim().IsEmpty())
			return false;
		HST_MissionAssetState asset = state.FindMissionAsset(assetId);
		if (!asset || !IsManagedOrQuarantinedAsset(asset))
			return false;
		missionInstanceId = asset.m_sMissionInstanceId;
		HST_ActiveMissionState mission = state.FindActiveMission(missionInstanceId);
		HST_RadioSiteState site = state.FindRadioSite(asset.m_sRadioSiteId);
		if (!IsExactMission(mission) || !site || site.m_iContractVersion != EXACT_CONTRACT_VERSION)
		{
			resultText = "h-istasi radio site | exact demolition authority is quarantined";
			return false;
		}
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
		{
			resultText = "h-istasi radio site | ignored stale explosive evidence after mission settlement";
			return false;
		}
		if (!MissionOwnsCurrentSiteLock(state, site, mission, asset))
		{
			resultText = "h-istasi radio site | stale or cross-linked explosive evidence quarantined";
			return QuarantineMissionAggregate(state, mission, "explosive evidence lost its reciprocal radio-site lock");
		}
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD)
		{
			resultText = "h-istasi radio site | authored transmitter requires observed physical destruction";
			return false;
		}
		if (asset.m_bDestroyed)
		{
			resultText = "h-istasi radio site | target destruction already recorded";
			return false;
		}

		IEntity projection = FindProjection(site.m_sSiteId);
		HST_MissionAssetComponent projectionAsset;
		if (projection)
			projectionAsset = HST_MissionAssetComponent.Cast(
				projection.FindComponent(HST_MissionAssetComponent));
		if (!projection || !projectionAsset
			|| projectionAsset.GetAssetId() != asset.m_sAssetId
			|| projectionAsset.GetMissionInstanceId() != mission.m_sInstanceId
			|| projectionAsset.GetRole() != asset.m_sRole)
		{
			resultText = "h-istasi radio site | explosive evidence rejected without the configured live target";
			return false;
		}
		vector projectionPosition = projection.GetOrigin();
		if (IsZeroVectorStatic(position))
			position = projectionPosition;
		if (!PositionsMatch2D(position, projectionPosition, PHYSICAL_EVIDENCE_POSITION_TOLERANCE_METERS)
			|| !PositionsMatch2D(projectionPosition, site.m_vTargetPosition, PHYSICAL_EVIDENCE_POSITION_TOLERANCE_METERS))
		{
			resultText = "h-istasi radio site | explosive evidence position does not match the live target";
			return false;
		}

		bool thresholdReached;
		bool evidenceAlreadyApplied;
		float previousRequiredDamage = asset.m_fDemolitionRequiredDamage;
		float previousDamage = asset.m_fDemolitionDamage;
		int previousEvidenceCount = asset.m_aDemolitionEvidenceKeys.Count();
		if (!RecordExactExplosiveEvidence(
				state,
				site,
				mission,
				asset,
				projectionPosition,
				damage,
				sourceLabel,
				thresholdReached,
				evidenceAlreadyApplied))
		{
			resultText = "h-istasi radio site | explosive evidence receipt is invalid or exhausted";
			bool evidenceStateChanged = asset.m_fDemolitionRequiredDamage != previousRequiredDamage
				|| asset.m_fDemolitionDamage != previousDamage
				|| asset.m_aDemolitionEvidenceKeys.Count() != previousEvidenceCount;
			bool quarantined = QuarantineMissionAggregate(
				state,
				mission,
				"exact demolition evidence could not record one bounded durable receipt");
			return evidenceStateChanged || quarantined;
		}
		if (evidenceAlreadyApplied)
		{
			resultText = "h-istasi radio site | explosive evidence receipt already applied";
			return false;
		}
		if (!thresholdReached)
		{
			resultText = string.Format(
				"h-istasi radio site | demolition progress %1/%2",
				Math.Round(asset.m_fDemolitionDamage),
				Math.Round(asset.m_fDemolitionRequiredDamage));
			return true;
		}

		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(
			projection.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager || (damageManager.GetState() != EDamageState.DESTROYED
			&& (!damageManager.SetHealthScaled(0.0)
				|| damageManager.GetState() != EDamageState.DESTROYED)))
		{
			resultText = "h-istasi radio site | exact demolition score could not destroy the live target projection";
			QuarantineMissionAggregate(
				state,
				mission,
				"generated target refused authoritative physical destruction after exact explosive score");
			// The bounded evidence receipt was appended before the physical write.
			// Report a durable change even when quarantine was already converged.
			return true;
		}
		if (!MarkPhysicalTargetDestroyed(
			state,
			site,
			mission,
			asset,
			projectionPosition,
			"exact explosive score threshold"))
		{
			resultText = "h-istasi radio site | exact demolition state could not commit after physical destruction";
			QuarantineMissionAggregate(
				state,
				mission,
				"generated target destruction could not commit its durable exact outcome");
			return true;
		}
		resultText = "h-istasi radio site | exact demolition target destroyed";
		return true;
	}

	protected bool RecordExactExplosiveEvidence(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		vector projectionPosition,
		float damage,
		string evidenceRequestId,
		out bool thresholdReached,
		out bool alreadyApplied)
	{
		thresholdReached = false;
		alreadyApplied = false;
		string normalizedRequestId = evidenceRequestId.Trim();
		if (!state || !site || !mission || !asset || damage <= 0.0
			|| normalizedRequestId.IsEmpty()
			|| !MissionOwnsCurrentSiteLock(state, site, mission, asset)
			|| site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN
			|| IsZeroVectorStatic(projectionPosition)
			|| !PositionsMatch2D(
				projectionPosition,
				site.m_vTargetPosition,
				PHYSICAL_EVIDENCE_POSITION_TOLERANCE_METERS))
			return false;
		HST_MissionRuntimeEntityState runtime = FindReciprocalMissionRuntime(
			state,
			mission,
			asset);
		if (!runtime)
			return false;
		float requiredDamage = asset.m_fDemolitionRequiredDamage;
		if (requiredDamage <= 0.0)
			requiredDamage = DEFAULT_DEMOLITION_REQUIRED_DAMAGE;
		asset.m_fDemolitionRequiredDamage = requiredDamage;
		if (asset.m_aDemolitionEvidenceKeys.Contains(normalizedRequestId))
		{
			alreadyApplied = true;
			thresholdReached = asset.m_fDemolitionDamage >= requiredDamage;
			return true;
		}
		if (asset.m_aDemolitionEvidenceKeys.Count() >= MAX_DEMOLITION_EVIDENCE_KEYS)
			return false;

		asset.m_aDemolitionEvidenceKeys.Insert(normalizedRequestId);
		asset.m_fDemolitionDamage += damage;
		asset.m_iDemolitionHits = asset.m_aDemolitionEvidenceKeys.Count();
		asset.m_sLastDemolitionSource = normalizedRequestId;
		asset.m_iLastDemolitionSecond = Math.Max(0, state.m_iElapsedSeconds);
		asset.m_vCurrentPosition = projectionPosition;
		asset.m_vLastKnownPosition = projectionPosition;
		runtime.m_vPosition = projectionPosition;
		thresholdReached = asset.m_fDemolitionDamage >= requiredDamage;
		return true;
	}

	HST_RadioSiteTransitionResult OnMissionSucceeded(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED)
			return Reject(NewResult(mission), "radio mission has not committed success");
		if (mission.m_sMissionId == DESTROY_MISSION_ID)
			return ApplyMissionOutcome(state, mission, TRANSITION_DESTROY_SUCCESS);
		return ApplyMissionOutcome(state, mission, TRANSITION_REBUILD_SUCCESS);
	}

	HST_RadioSiteTransitionResult OnMissionFailed(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_FAILED)
			return Reject(NewResult(mission), "radio mission has not committed failure");
		if (mission.m_sMissionId == DESTROY_MISSION_ID)
			return ApplyMissionOutcome(state, mission, TRANSITION_DESTROY_FAILURE);
		return ApplyMissionOutcome(state, mission, TRANSITION_REBUILD_FAILURE);
	}

	HST_RadioSiteTransitionResult OnMissionExpired(HST_CampaignState state, HST_ActiveMissionState mission)
	{
		if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_EXPIRED)
			return Reject(NewResult(mission), "radio mission has not committed expiry");
		if (mission.m_sMissionId == DESTROY_MISSION_ID)
			return ApplyMissionOutcome(state, mission, TRANSITION_DESTROY_EXPIRY);
		return ApplyMissionOutcome(state, mission, TRANSITION_REBUILD_EXPIRY);
	}

	bool SettleOpenSitesForCampaignStop(HST_CampaignState state, string reason)
	{
		if (!state)
			return false;
		bool changed;
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!IsExactMission(mission) || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE)
				continue;
			mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
			mission.m_sRuntimePhase = "failed";
			mission.m_sRuntimeFailureReason = reason;
			string transitionKind = TRANSITION_CAMPAIGN_STOP_DESTROY;
			if (mission.m_sMissionId == REBUILD_MISSION_ID)
				transitionKind = TRANSITION_CAMPAIGN_STOP_REBUILD;
			HST_RadioSiteTransitionResult settled = ApplyMissionOutcome(state, mission, transitionKind);
			if (!settled || !settled.m_bAccepted)
			{
				string quarantineReason = "campaign-stop radio settlement rejected";
				if (settled && !settled.m_sReason.IsEmpty())
					quarantineReason = quarantineReason + ": " + settled.m_sReason;
				changed = QuarantineMissionAggregate(state, mission, quarantineReason) || changed;
				continue;
			}
			changed = settled.m_bStateChanged || changed;
		}
		return changed;
	}

	bool QuarantineMissionAggregate(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		string reason)
	{
		if (!state || !mission)
			return false;
		HST_RadioSiteState site = state.FindRadioSite(mission.m_sRadioSiteId);
		if (site)
			return QuarantineSite(state, site, reason);
		return CleanupQuarantinedMissionAggregate(state, mission, reason);
	}

	protected HST_RadioSiteTransitionResult ApplyMissionOutcome(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		string transitionKind)
	{
		HST_RadioSiteTransitionResult result = NewResult(mission);
		if (!state || !IsExactMission(mission))
			return Reject(result, "mission does not carry exact radio outcome authority");
		HST_RadioSiteState site = state.FindRadioSite(mission.m_sRadioSiteId);
		result.m_Site = site;
		result.m_Asset = FindExactMissionAsset(state, mission.m_sInstanceId);
		if (!site || site.m_iContractVersion != EXACT_CONTRACT_VERSION)
			return Reject(result, "radio-site outcome backlink is missing or quarantined");

		HST_ERadioSiteLifecycleState fromState;
		HST_ERadioSiteLifecycleState toState;
		if (!ResolveOutcomeShape(mission, transitionKind, fromState, toState))
			return Reject(result, "radio-site mission status does not match the requested outcome transition");
		string requestId = BuildOutcomeRequestId(site.m_sSiteId, mission.m_sInstanceId, mission.m_sMissionId, transitionKind);
		if (site.m_sLastTransitionRequestId == requestId)
		{
			if (site.m_sLastTransitionMissionInstanceId == mission.m_sInstanceId
				&& site.m_sLastTransitionKind == transitionKind
				&& site.m_eLastTransitionFromState == fromState
				&& site.m_eLastTransitionToState == toState
				&& site.m_eLifecycleState == toState
				&& site.m_iLastTransitionRecordedRevision == site.m_iRevision
				&& mission.m_iRadioSiteRevision == site.m_iRevision)
			{
				string replayFailure;
				if (!ValidateReplayAggregate(state, site, replayFailure))
				{
					QuarantineMissionAggregate(state, mission, "outcome replay aggregate is invalid: " + replayFailure);
					return Reject(result, "radio-site outcome replay aggregate is invalid: " + replayFailure);
				}
				if (!ReconcileProjectionForOutcomeReplay(state, site, mission))
				{
					QuarantineMissionAggregate(state, mission, "outcome replay projection reconciliation failed");
					return Reject(result, "radio-site outcome replay projection reconciliation failed");
				}
				result.m_bAccepted = true;
				result.m_bAlreadyApplied = true;
				result.m_sReason = "radio-site outcome already applied";
				return result;
			}
			return Reject(result, "radio-site outcome request replay changed its typed fingerprint");
		}

		if (mission.m_iRadioSiteRevision != site.m_iRevision)
			return Reject(result, "stale radio-site outcome revision");
		if (site.m_sActiveMissionInstanceId != mission.m_sInstanceId
			|| site.m_sActiveMissionId != mission.m_sMissionId
			|| site.m_sActiveTransitionRequestId != mission.m_sRadioSiteTransitionRequestId)
			return Reject(result, "radio-site outcome no longer owns the active site lock");
		if (site.m_eLifecycleState != fromState)
			return Reject(result, "radio-site lifecycle changed before outcome settlement");
		bool successOutcome = transitionKind == TRANSITION_DESTROY_SUCCESS
			|| transitionKind == TRANSITION_REBUILD_SUCCESS;
		if (!MissionAggregateReadyForSettlement(state, site, mission, result.m_Asset, successOutcome))
			return Reject(result, "radio-site mission aggregate is not coherent for outcome settlement");
		if (transitionKind == TRANSITION_DESTROY_SUCCESS && !CanCompleteMissionAfterStatusCommit(state, mission, result.m_Asset))
			return Reject(result, "destroy success lacks exact physical destruction evidence");
		if (transitionKind == TRANSITION_REBUILD_SUCCESS && !CanCompleteMissionAfterStatusCommit(state, mission, result.m_Asset))
			return Reject(result, "stop-rebuild success lacks exact equipment destruction evidence");

		ApplyAcceptedTransition(state, site, mission, requestId, transitionKind, fromState, toState, "exact radio mission outcome");
		ClearActiveLock(site);
		if (transitionKind == TRANSITION_DESTROY_SUCCESS)
		{
			site.m_sLastDestructionReceiptId = BuildDestructionReceiptId(site.m_sSiteId, mission.m_sInstanceId);
			site.m_sLastDestructionMissionInstanceId = mission.m_sInstanceId;
			site.m_iDestroyedAtSecond = Math.Max(0, state.m_iElapsedSeconds);
			ClearRebuildEvidence(site);
		}
		else if (transitionKind == TRANSITION_REBUILD_SUCCESS)
		{
			// Destroying construction equipment stops this rebuild attempt; it does
			// not create a new transmitter-destruction epoch that can be farmed.
			site.m_sLastRebuildReceiptId = BuildRebuildReceiptId(site.m_sSiteId, mission.m_sInstanceId);
			site.m_sLastRebuildMissionInstanceId = mission.m_sInstanceId;
			site.m_iRebuiltAtSecond = 0;
		}
		else if (transitionKind == TRANSITION_REBUILD_FAILURE
			|| transitionKind == TRANSITION_REBUILD_EXPIRY
			|| transitionKind == TRANSITION_CAMPAIGN_STOP_REBUILD)
		{
			site.m_sLastRebuildReceiptId = BuildRebuildReceiptId(site.m_sSiteId, mission.m_sInstanceId);
			site.m_sLastRebuildMissionInstanceId = mission.m_sInstanceId;
			site.m_iRebuiltAtSecond = Math.Max(site.m_iRebuildStartedAtSecond, Math.Max(0, state.m_iElapsedSeconds));
			site.m_eTargetOwnership = HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN;
			site.m_sTargetPrefab = GENERATED_TOWER_PREFAB;
		}

		if (!ReconcileProjectionAfterOutcome(state, site, mission, transitionKind))
		{
			string projectionFailure = "radio-site outcome projection reconciliation quarantined the aggregate";
			QuarantineMissionAggregate(state, mission, projectionFailure);
			FinalizeMissionAggregate(state, mission, result.m_Asset);
			result.m_bStateChanged = true;
			return Reject(result, projectionFailure);
		}

		FinalizeMissionAggregate(state, mission, result.m_Asset);
		result.m_bAccepted = true;
		result.m_bStateChanged = true;
		result.m_sReason = "radio-site outcome applied";
		return result;
	}

	protected bool ResolveOutcomeShape(
		HST_ActiveMissionState mission,
		string transitionKind,
		out HST_ERadioSiteLifecycleState fromState,
		out HST_ERadioSiteLifecycleState toState)
	{
		fromState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_UNKNOWN;
		toState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_UNKNOWN;
		if (!mission)
			return false;
		if (transitionKind == TRANSITION_DESTROY_SUCCESS)
		{
			fromState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
			toState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED;
			return mission.m_sMissionId == DESTROY_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED;
		}
		if (transitionKind == TRANSITION_DESTROY_FAILURE || transitionKind == TRANSITION_CAMPAIGN_STOP_DESTROY)
		{
			fromState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
			toState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
			return mission.m_sMissionId == DESTROY_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED;
		}
		if (transitionKind == TRANSITION_DESTROY_EXPIRY)
		{
			fromState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
			toState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
			return mission.m_sMissionId == DESTROY_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED;
		}
		if (transitionKind == TRANSITION_REBUILD_SUCCESS)
		{
			fromState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING;
			toState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED;
			return mission.m_sMissionId == REBUILD_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED;
		}
		if (transitionKind == TRANSITION_REBUILD_FAILURE || transitionKind == TRANSITION_CAMPAIGN_STOP_REBUILD)
		{
			fromState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING;
			toState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
			return mission.m_sMissionId == REBUILD_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED;
		}
		if (transitionKind == TRANSITION_REBUILD_EXPIRY)
		{
			fromState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING;
			toState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
			return mission.m_sMissionId == REBUILD_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED;
		}
		return false;
	}

	protected void ApplyAcceptedTransition(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		string requestId,
		string transitionKind,
		HST_ERadioSiteLifecycleState fromState,
		HST_ERadioSiteLifecycleState toState,
		string reason)
	{
		site.m_iRevision = Math.Max(1, site.m_iRevision) + 1;
		mission.m_iRadioSiteRevision = site.m_iRevision;
		site.m_eLifecycleState = toState;
		site.m_sLastTransitionRequestId = requestId;
		site.m_sLastTransitionMissionInstanceId = mission.m_sInstanceId;
		site.m_sLastTransitionKind = transitionKind;
		site.m_eLastTransitionFromState = fromState;
		site.m_eLastTransitionToState = toState;
		site.m_iLastTransitionRecordedRevision = site.m_iRevision;
		site.m_sLastTransitionReason = reason + ": " + transitionKind;
		site.m_iLastTransitionSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (transitionKind == TRANSITION_DESTROY_ADMISSION || transitionKind == TRANSITION_REBUILD_ADMISSION)
		{
			site.m_sActiveMissionInstanceId = mission.m_sInstanceId;
			site.m_sActiveMissionId = mission.m_sMissionId;
			site.m_sActiveTransitionRequestId = requestId;
		}
	}

	protected bool AdmissionFingerprintMatches(
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		string transitionKind,
		HST_ERadioSiteLifecycleState fromState,
		HST_ERadioSiteLifecycleState toState)
	{
		return site && mission
			&& site.m_sActiveMissionInstanceId == mission.m_sInstanceId
			&& site.m_sActiveMissionId == mission.m_sMissionId
			&& site.m_sLastTransitionRequestId == mission.m_sRadioSiteTransitionRequestId
			&& site.m_sLastTransitionMissionInstanceId == mission.m_sInstanceId
			&& site.m_sLastTransitionKind == transitionKind
			&& site.m_eLastTransitionFromState == fromState
			&& site.m_eLastTransitionToState == toState
			&& site.m_eLifecycleState == toState
			&& site.m_iLastTransitionRecordedRevision == site.m_iRevision
			&& mission.m_iRadioSiteRevision == site.m_iRevision;
	}

	protected bool ValidateReplayAggregate(
		HST_CampaignState state,
		HST_RadioSiteState site,
		out string failureReason)
	{
		failureReason = "";
		if (!state || !site)
		{
			failureReason = "state or site is unavailable";
			return false;
		}
		HST_CampaignSaveData snapshot = new HST_CampaignSaveData();
		snapshot.Capture(state);
		HST_RadioSiteState snapshotSite;
		foreach (HST_RadioSiteState candidate : snapshot.m_aRadioSites)
		{
			if (!candidate || candidate.m_sSiteId != site.m_sSiteId)
				continue;
			if (snapshotSite)
			{
				failureReason = "duplicate site rows";
				return false;
			}
			snapshotSite = candidate;
		}
		if (!snapshotSite)
		{
			failureReason = "site row is missing from the durable snapshot";
			return false;
		}
		HST_RadioSiteSaveValidationService validator = new HST_RadioSiteSaveValidationService();
		failureReason = validator.ValidateCurrentAggregate(snapshot, snapshotSite);
		return failureReason.IsEmpty();
	}

	protected void ClearActiveLock(HST_RadioSiteState site)
	{
		if (!site)
			return;
		site.m_sActiveMissionInstanceId = "";
		site.m_sActiveMissionId = "";
		site.m_sActiveTransitionRequestId = "";
	}

	protected void ClearRebuildEvidence(HST_RadioSiteState site)
	{
		if (!site)
			return;
		site.m_sLastRebuildReceiptId = "";
		site.m_sLastRebuildMissionInstanceId = "";
		site.m_iRebuildStartedAtSecond = 0;
		site.m_iRebuiltAtSecond = 0;
	}

	protected bool PrepareProjectionForAdmission(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission)
	{
		if (!state || !site || !mission)
			return false;
		if (mission.m_sMissionId != DESTROY_MISSION_ID)
			return true;
		EnsureSiteProjection(state, site);
		return site.m_iContractVersion == EXACT_CONTRACT_VERSION
			&& FindProjection(site.m_sSiteId) != null;
	}

	protected bool ReconcileAdmissionProjection(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset)
	{
		if (!state || !site || !mission || !asset)
			return false;
		EnsureSiteProjection(state, site);
		if (site.m_iContractVersion != EXACT_CONTRACT_VERSION)
			return false;
		if (!ConfigureProjectionForMission(state, site, mission, asset))
			return false;
		return FindProjection(site.m_sSiteId) != null
			&& asset.m_bSpawned && mission.m_bRuntimeSpawned;
	}

	protected bool ReconcileProjectionAfterOutcome(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		string transitionKind)
	{
		if (!state || !site || !mission)
			return false;
		if (transitionKind == TRANSITION_DESTROY_SUCCESS)
			FinalizeDestroyProjection(site, true);
		else if (transitionKind == TRANSITION_REBUILD_SUCCESS)
			ForgetProjection(site.m_sSiteId, true);
		else if (transitionKind == TRANSITION_REBUILD_FAILURE
			|| transitionKind == TRANSITION_REBUILD_EXPIRY
			|| transitionKind == TRANSITION_CAMPAIGN_STOP_REBUILD)
		{
			ForgetProjection(site.m_sSiteId, true);
			EnsureSiteProjection(state, site);
		}
		else
			FinalizeDestroyProjection(site, false);
		if (site.m_iContractVersion != EXACT_CONTRACT_VERSION)
			return false;
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
			&& site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
			return GeneratedProjectionSupportsExactEvidence(
				FindProjection(site.m_sSiteId),
				site.m_sTargetPrefab,
				site.m_vTargetPosition);
		return true;
	}

	protected bool ReconcileProjectionForOutcomeReplay(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission)
	{
		if (!state || !site || !mission)
			return false;
		EnsureSiteProjection(state, site);
		if (site.m_iContractVersion != EXACT_CONTRACT_VERSION)
			return false;
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
			&& site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
			return GeneratedProjectionSupportsExactEvidence(
				FindProjection(site.m_sSiteId),
				site.m_sTargetPrefab,
				site.m_vTargetPosition);
		return true;
	}

	protected HST_MissionAssetState CreateMissionTargetAggregate(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission)
	{
		HST_MissionAssetState asset = new HST_MissionAssetState();
		asset.m_sAssetId = "asset_" + mission.m_sInstanceId + "_destroy_target_0";
		asset.m_sMissionInstanceId = mission.m_sInstanceId;
		asset.m_sKind = TARGET_KIND;
		asset.m_sRole = TARGET_ROLE;
		asset.m_sPrefab = site.m_sTargetPrefab;
		asset.m_sEntityId = asset.m_sAssetId + "_entity";
		if (mission.m_sMissionId == REBUILD_MISSION_ID)
		{
			asset.m_sPrefab = REBUILD_EQUIPMENT_PREFAB;
		}
		asset.m_iRadioSiteContractVersion = EXACT_CONTRACT_VERSION;
		asset.m_sRadioSiteId = site.m_sSiteId;
		asset.m_eRadioSiteTargetOwnership = site.m_eTargetOwnership;
		asset.m_sRadioSiteAuthoredTargetPrefab = site.m_sAuthoredTargetPrefab;
		asset.m_vRadioSiteAuthoredTargetPosition = site.m_vAuthoredTargetPosition;
		asset.m_vSourcePosition = site.m_vTargetPosition;
		asset.m_vTargetPosition = site.m_vTargetPosition;
		asset.m_vCurrentPosition = site.m_vTargetPosition;
		asset.m_vLastKnownPosition = site.m_vTargetPosition;
		asset.m_iDeadlineSecond = mission.m_iActiveUntilSecond;
		asset.m_fDemolitionRequiredDamage = DEFAULT_DEMOLITION_REQUIRED_DAMAGE;
		asset.m_bAlive = true;
		asset.m_bSpawned = FindProjection(site.m_sSiteId) != null;
		state.m_aMissionAssets.Insert(asset);

		HST_MissionRuntimeEntityState runtime = new HST_MissionRuntimeEntityState();
		runtime.m_sRuntimeEntityId = asset.m_sEntityId;
		runtime.m_sMissionInstanceId = mission.m_sInstanceId;
		runtime.m_sKind = "radio_site_target";
		runtime.m_sPrefab = asset.m_sPrefab;
		runtime.m_vPosition = site.m_vTargetPosition;
		runtime.m_bSpawned = asset.m_bSpawned;
		state.m_aMissionRuntimeEntities.Insert(runtime);

		mission.m_vTargetPosition = site.m_vTargetPosition;
		mission.m_sRuntimePrimitive = "radio_site_destroy";
		if (mission.m_sMissionId == REBUILD_MISSION_ID)
			mission.m_sRuntimePrimitive = "radio_site_rebuild";
		mission.m_sRuntimePhase = "radio_site_active";
		mission.m_bRuntimeSpawned = asset.m_bSpawned;

		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			objective.m_eType = HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET;
			objective.m_sLabel = "Destroy transmitter";
			objective.m_sRequirementText = "Physically destroy the bound radio transmitter.";
			if (mission.m_sMissionId == REBUILD_MISSION_ID)
			{
				objective.m_sLabel = "Stop transmitter rebuild";
				objective.m_sRequirementText = "Destroy the marked construction equipment before the replacement transmitter comes online.";
			}
			objective.m_sTargetId = site.m_sTargetId;
			objective.m_sTargetZoneId = site.m_sZoneId;
			objective.m_sPhysicalEntityId = asset.m_sEntityId;
			objective.m_sLinkedRuntimeEntityId = asset.m_sEntityId;
			objective.m_sRuntimePrimitive = mission.m_sRuntimePrimitive;
			objective.m_vPosition = site.m_vTargetPosition;
			objective.m_iRequiredProgress = 1;
			objective.m_iCurrentProgress = 0;
			objective.m_iRequiredCount = 1;
			objective.m_iCurrentCount = 0;
			objective.m_bComplete = false;
			objective.m_bFailed = false;
			break;
		}

		HST_CampaignTaskState task = state.FindCampaignTask("task_" + mission.m_sInstanceId);
		if (task)
			task.m_vPosition = site.m_vTargetPosition;
		return asset;
	}

	protected HST_MissionRuntimeEntityState FindReciprocalMissionRuntime(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset)
	{
		if (!state || !mission || !asset)
			return null;
		HST_MissionRuntimeEntityState runtime = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtime || CountMissionRuntimeEntities(state, mission.m_sInstanceId) != 1
			|| runtime.m_sMissionInstanceId != mission.m_sInstanceId
			|| runtime.m_sKind != "radio_site_target"
			|| runtime.m_sPrefab != asset.m_sPrefab
			|| runtime.m_bDestroyed != asset.m_bDestroyed)
			return null;
		return runtime;
	}

	protected bool MarkPhysicalTargetDestroyed(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset,
		vector position,
		string evidence)
	{
		if (!state || !site || !mission || !asset || asset.m_bDestroyed
			|| !MissionOwnsCurrentSiteLock(state, site, mission, asset)
			|| IsZeroVectorStatic(position)
			|| !PositionsMatch2D(
				position,
				site.m_vTargetPosition,
				PHYSICAL_EVIDENCE_POSITION_TOLERANCE_METERS))
			return false;
		HST_MissionRuntimeEntityState runtime = FindReciprocalMissionRuntime(
			state,
			mission,
			asset);
		if (!runtime)
			return false;
		asset.m_bDestroyed = true;
		asset.m_bAlive = false;
		asset.m_bOutcomeApplied = true;
		asset.m_sOutcomeKind = "physically_destroyed";
		asset.m_sLastInteraction = evidence;
		if (!IsZeroVectorStatic(position))
		{
			asset.m_vCurrentPosition = position;
			asset.m_vLastKnownPosition = position;
			runtime.m_vPosition = position;
		}
		mission.m_iRuntimeDestroyedCount = Math.Max(1, mission.m_iRuntimeDestroyedCount);
		mission.m_sRuntimePhase = "radio_site_target_destroyed";
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			objective.m_iCurrentProgress = Math.Max(1, objective.m_iRequiredProgress);
			objective.m_iCurrentCount = Math.Max(1, objective.m_iRequiredCount);
			objective.m_bWorldDetected = true;
			objective.m_bComplete = true;
			break;
		}
		runtime.m_bDestroyed = true;
		return true;
	}

	protected bool IsDestroyObjectiveComplete(HST_CampaignState state, string missionInstanceId)
	{
		if (!state || missionInstanceId.IsEmpty())
			return false;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != missionInstanceId)
				continue;
			return objective.m_eType == HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET
				&& objective.m_bComplete && !objective.m_bFailed;
		}
		return false;
	}

	protected bool CanCompleteMissionAfterStatusCommit(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset)
	{
		return state && mission && asset && asset.m_bDestroyed && !asset.m_bAlive
			&& IsDestroyObjectiveComplete(state, mission.m_sInstanceId);
	}

	protected void FinalizeMissionAggregate(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset)
	{
		if (!state || !mission)
			return;
		mission.m_bRuntimeCleanupComplete = true;
		mission.m_bRuntimeSpawned = false;
		HST_CampaignTaskState task = state.FindCampaignTask("task_" + mission.m_sInstanceId);
		if (task)
		{
			task.m_bActive = false;
			task.m_bSucceeded = mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED;
			task.m_bFailed = mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED
				|| mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED;
		}
		if (asset)
		{
			asset.m_bSpawned = false;
			if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED)
				asset.m_sLastInteraction = "radio site mission settled without target destruction";
			HST_MissionRuntimeEntityState runtime = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtime)
				runtime.m_bSpawned = false;
		}
	}

	protected bool ConfigureProjectionForMission(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset)
	{
		if (!state || !site || !mission || !asset
			|| !MissionOwnsCurrentSiteLock(state, site, mission, asset))
			return false;
		IEntity projection = FindProjection(site.m_sSiteId);
		if (!projection || projection.IsDeleted()
			|| !PositionsMatch2D(
				projection.GetOrigin(),
				site.m_vTargetPosition,
				PHYSICAL_EVIDENCE_POSITION_TOLERANCE_METERS)
			|| ResolveEntityPrefab(projection) != asset.m_sPrefab
			|| !SCR_DamageManagerComponent.Cast(
				projection.FindComponent(SCR_DamageManagerComponent)))
			return false;
		HST_MissionAssetComponent component = HST_MissionAssetComponent.Cast(
			projection.FindComponent(HST_MissionAssetComponent));
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN
			&& !component)
			return false;
		if (component)
		{
			component.ConfigureMissionAsset(asset.m_sAssetId, mission.m_sInstanceId, asset.m_sRole);
			if (component.GetAssetId() != asset.m_sAssetId
				|| component.GetMissionInstanceId() != mission.m_sInstanceId
				|| component.GetRole() != asset.m_sRole)
				return false;
		}
		HST_MissionRuntimeEntityState runtime = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtime || runtime.m_sMissionInstanceId != mission.m_sInstanceId
			|| runtime.m_sPrefab != asset.m_sPrefab)
			return false;
		asset.m_bSpawned = true;
		asset.m_vCurrentPosition = projection.GetOrigin();
		asset.m_vLastKnownPosition = projection.GetOrigin();
		mission.m_bRuntimeSpawned = true;
		mission.m_sRuntimePhase = "radio_site_active";
		runtime.m_bSpawned = true;
		runtime.m_vPosition = projection.GetOrigin();
		return true;
	}

	protected bool GeneratedProjectionSupportsExactEvidence(
		IEntity projection,
		string expectedPrefab,
		vector expectedPosition)
	{
		return projection && !projection.IsDeleted()
			&& ResolveEntityPrefab(projection) == expectedPrefab
			&& PositionsMatch2D(
				projection.GetOrigin(),
				expectedPosition,
				PHYSICAL_EVIDENCE_POSITION_TOLERANCE_METERS)
			&& SCR_DamageManagerComponent.Cast(
				projection.FindComponent(SCR_DamageManagerComponent))
			&& HST_MissionAssetComponent.Cast(
				projection.FindComponent(HST_MissionAssetComponent));
	}

	protected bool EnsureSiteProjection(HST_CampaignState state, HST_RadioSiteState site)
	{
		if (!state || !site || site.m_iContractVersion != EXACT_CONTRACT_VERSION)
			return false;
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE)
			return EnsureOnlineProjection(state, site);
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED)
			return EnsureDestroyedProjection(state, site);
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING)
			return EnsureRebuildProjection(state, site);
		return false;
	}

	protected bool EnsureOnlineProjection(HST_CampaignState state, HST_RadioSiteState site)
	{
		bool changed;
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED)
		{
			HST_ZoneState zone = state.FindZone(site.m_sZoneId);
			if (!zone)
				return QuarantineSite(state, site, "logical radio site lost its zone authority");
			CollectTransmitterCandidates(zone.m_vPosition, BINDING_SEARCH_RADIUS_METERS, site.m_sSiteId);
			if (HasCandidateClaimedByOtherSite(site.m_sSiteId))
				return QuarantineSite(state, site, "transmitter candidate is already claimed by another radio site");
			if (m_aTransmitterCandidates.Count() == 0)
				return false;
			if (m_aTransmitterCandidates.Count() != 1)
				return QuarantineSite(state, site, "authored transmitter binding is ambiguous within the radio zone");

			IEntity candidate = m_aTransmitterCandidates[0];
			string prefab = ResolveEntityPrefab(candidate);
			if (!IsSupportedTransmitterPrefab(prefab))
				return QuarantineSite(state, site, "authored transmitter candidate has unsupported prefab identity");
			if (!SCR_DamageManagerComponent.Cast(candidate.FindComponent(SCR_DamageManagerComponent)))
				return QuarantineSite(state, site, "authored transmitter candidate is not damageable");
			site.m_sTargetPrefab = prefab;
			site.m_vTargetPosition = candidate.GetOrigin();
			site.m_sAuthoredTargetPrefab = prefab;
			site.m_vAuthoredTargetPosition = candidate.GetOrigin();
			site.m_eTargetOwnership = HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD;
			site.m_iRevision = Math.Max(1, site.m_iRevision) + 1;
			site.m_sLastTransitionReason = "bound one authored world transmitter without taking deletion ownership";
			site.m_iLastTransitionSecond = Math.Max(0, state.m_iElapsedSeconds);
			RegisterProjection(site.m_sSiteId, "", candidate, true);
			changed = true;
		}

		IEntity projection = FindProjection(site.m_sSiteId);
		if (!projection && site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD)
		{
			CollectTransmitterCandidates(site.m_vTargetPosition, FROZEN_BINDING_SEARCH_RADIUS_METERS, site.m_sSiteId);
			if (HasCandidateClaimedByOtherSite(site.m_sSiteId))
				return QuarantineSite(state, site, "frozen transmitter binding overlaps another radio site");
			if (m_aTransmitterCandidates.Count() > 1)
				return QuarantineSite(state, site, "frozen authored transmitter binding became ambiguous");
			if (m_aTransmitterCandidates.Count() == 1)
			{
				IEntity rebound = m_aTransmitterCandidates[0];
				if (!FrozenCandidateMatchesSite(site, rebound))
					return QuarantineSite(state, site, "authored transmitter rebind conflicts with frozen prefab or position");
				RegisterProjection(site.m_sSiteId, "", rebound, true);
				projection = rebound;
				changed = true;
			}
		}
		else if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
		{
			changed = SuppressAuthoredTransmitterResurrection(state, site) || changed;
			if (site.m_iContractVersion != EXACT_CONTRACT_VERSION)
				return changed;
			if (!projection)
			{
				GenericEntity generated = SpawnProjectionPrefab(site.m_sTargetPrefab, site.m_vTargetPosition);
				if (!generated)
					return QuarantineSite(state, site, "generated ONLINE transmitter projection could not spawn") || changed;
				RegisterProjection(site.m_sSiteId, "", generated, false);
				projection = generated;
				changed = true;
			}
			if (!GeneratedProjectionSupportsExactEvidence(
				projection,
				site.m_sTargetPrefab,
				site.m_vTargetPosition))
				return QuarantineSite(state, site, "generated ONLINE transmitter lacks exact evidence components or frozen identity") || changed;
		}

		if (!projection)
		{
			if (site.m_sActiveMissionInstanceId.IsEmpty())
				return changed;
			HST_ActiveMissionState pendingMission = state.FindActiveMission(site.m_sActiveMissionInstanceId);
			HST_MissionAssetState pendingAsset = FindExactMissionAsset(
				state,
				site.m_sActiveMissionInstanceId);
			if (!pendingMission || !pendingAsset
				|| !MissionOwnsCurrentSiteLock(state, site, pendingMission, pendingAsset))
				return QuarantineSite(
					state,
					site,
					"missing borrowed projection lost its reciprocal active aggregate") || changed;
			HST_MissionRuntimeEntityState pendingRuntime = state.FindMissionRuntimeEntity(
				pendingAsset.m_sEntityId);
			if (!pendingRuntime
				|| CountMissionRuntimeEntities(state, pendingMission.m_sInstanceId) != 1
				|| pendingRuntime.m_sMissionInstanceId != pendingMission.m_sInstanceId
				|| pendingRuntime.m_sKind != "radio_site_target"
				|| pendingRuntime.m_sPrefab != pendingAsset.m_sPrefab
				|| pendingRuntime.m_bDestroyed != pendingAsset.m_bDestroyed)
				return QuarantineSite(
					state,
					site,
					"missing borrowed projection lost its reciprocal runtime entity") || changed;
			return MarkBorrowedProjectionPending(state, pendingMission, pendingAsset) || changed;
		}
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(
			projection.FindComponent(SCR_DamageManagerComponent));
		if (damageManager && damageManager.GetState() == EDamageState.DESTROYED)
		{
			HST_ActiveMissionState mission = state.FindActiveMission(site.m_sActiveMissionInstanceId);
			HST_MissionAssetState asset = FindExactMissionAsset(state, site.m_sActiveMissionInstanceId);
			if (mission && asset && IsExactMission(mission)
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
				&& mission.m_sMissionId == DESTROY_MISSION_ID)
			{
				if (!MissionOwnsCurrentSiteLock(state, site, mission, asset))
					return QuarantineMissionAggregate(
						state,
						mission,
						"ONLINE transmitter damage polling lost its reciprocal site lock") || changed;
				if (asset.m_bDestroyed)
					return changed;
				if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD)
				{
					if (MarkPhysicalTargetDestroyed(state, site, mission, asset, projection.GetOrigin(), "authoritative borrowed-target damage state"))
						return true;
					return QuarantineMissionAggregate(
						state,
						mission,
						"borrowed target destruction could not commit its reciprocal runtime position") || changed;
				}
				return ResetOwnedProjectionAfterUnsupportedDamage(state, site, mission, asset) || changed;
			}
			if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
				return ResetOwnedProjectionAfterUnsupportedDamage(state, site, mission, asset) || changed;
			return QuarantineSite(state, site, "ONLINE transmitter was physically destroyed outside its exact mission outcome") || changed;
		}

		if (!site.m_sActiveMissionInstanceId.IsEmpty())
		{
			HST_ActiveMissionState mission = state.FindActiveMission(site.m_sActiveMissionInstanceId);
			HST_MissionAssetState asset = FindExactMissionAsset(state, site.m_sActiveMissionInstanceId);
			if (!mission || !asset
				|| !ConfigureProjectionForMission(state, site, mission, asset))
				return QuarantineSite(
					state,
					site,
					"active ONLINE radio mission could not configure its exact projection") || changed;
		}
		else
		{
			HST_MissionAssetComponent staleComponent = HST_MissionAssetComponent.Cast(
				projection.FindComponent(HST_MissionAssetComponent));
			if (staleComponent)
				staleComponent.ConfigureMissionAsset("", "", "");
		}
		return changed;
	}

	protected bool MarkBorrowedProjectionPending(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset)
	{
		if (!state || !mission || !asset)
			return false;
		if (asset.m_eRadioSiteTargetOwnership
			!= HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD)
			return QuarantineMissionAggregate(
				state,
				mission,
				"missing borrowed projection carries non-borrowed mission-time ownership");
		HST_MissionRuntimeEntityState runtime = state.FindMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtime)
			return QuarantineMissionAggregate(
				state,
				mission,
				"missing borrowed projection lost its durable runtime-entity row");
		string pendingPhase = "radio_site_projection_pending";
		if (asset.m_bDestroyed)
			pendingPhase = "radio_site_target_destroyed";
		bool changed = mission.m_bRuntimeSpawned || asset.m_bSpawned || runtime.m_bSpawned
			|| mission.m_sRuntimePhase != pendingPhase;
		mission.m_bRuntimeSpawned = false;
		mission.m_sRuntimePhase = pendingPhase;
		asset.m_bSpawned = false;
		runtime.m_bSpawned = false;
		return changed;
	}

	protected bool EnsureDestroyedProjection(HST_CampaignState state, HST_RadioSiteState site)
	{
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED)
			return QuarantineSite(state, site, "DESTROYED radio site has unresolved target ownership");

		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
		{
			bool changed = ForgetProjection(site.m_sSiteId, true);
			return SuppressAuthoredTransmitterResurrection(state, site) || changed;
		}

		IEntity projection = FindProjection(site.m_sSiteId);
		bool changed;
		if (!projection)
		{
			CollectTransmitterCandidates(site.m_vTargetPosition, FROZEN_BINDING_SEARCH_RADIUS_METERS, site.m_sSiteId);
			if (HasCandidateClaimedByOtherSite(site.m_sSiteId))
				return QuarantineSite(state, site, "destroyed transmitter binding overlaps another radio site");
			if (m_aTransmitterCandidates.Count() > 1)
				return QuarantineSite(state, site, "destroyed authored transmitter rebind is ambiguous");
			if (m_aTransmitterCandidates.Count() == 1)
			{
				projection = m_aTransmitterCandidates[0];
				if (!FrozenCandidateMatchesSite(site, projection))
					return QuarantineSite(state, site, "destroyed authored transmitter rebind conflicts with frozen prefab or position");
				RegisterProjection(site.m_sSiteId, "", projection, true);
				changed = true;
			}
		}
		if (!projection)
			return changed;
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(
			projection.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager)
			return QuarantineSite(state, site, "destroyed authored transmitter lost its damage authority") || changed;
		if (damageManager.GetState() != EDamageState.DESTROYED)
		{
			if (!damageManager.SetHealthScaled(0.0)
				|| damageManager.GetState() != EDamageState.DESTROYED)
				return QuarantineSite(state, site, "destroyed authored transmitter refused physical damage reapplication") || changed;
			changed = true;
		}
		return changed;
	}

	protected bool EnsureRebuildProjection(HST_CampaignState state, HST_RadioSiteState site)
	{
		if (site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN
			|| site.m_sActiveMissionId != REBUILD_MISSION_ID
			|| site.m_sActiveMissionInstanceId.IsEmpty())
			return QuarantineSite(state, site, "REBUILDING radio site lacks generated ownership or exact mission lock");

		bool changed = SuppressAuthoredTransmitterResurrection(state, site);
		if (site.m_iContractVersion != EXACT_CONTRACT_VERSION)
			return changed;
		HST_ActiveMissionState mission = state.FindActiveMission(site.m_sActiveMissionInstanceId);
		HST_MissionAssetState asset = FindExactMissionAsset(state, site.m_sActiveMissionInstanceId);
		if (!mission || !asset || !IsExactMission(mission))
			return QuarantineSite(state, site, "REBUILDING radio site lost its mission target aggregate") || changed;
		if (asset.m_bDestroyed)
			return changed;

		IEntity projection = FindProjection(site.m_sSiteId);
		if (!projection)
		{
			GenericEntity equipment = SpawnProjectionPrefab(REBUILD_EQUIPMENT_PREFAB, site.m_vTargetPosition);
			if (!equipment)
				return QuarantineMissionAggregate(
					state,
					mission,
					"active rebuild equipment projection could not spawn") || changed;
			RegisterProjection(site.m_sSiteId, mission.m_sInstanceId, equipment, false);
			projection = equipment;
			changed = true;
		}
		if (!GeneratedProjectionSupportsExactEvidence(
				projection,
				REBUILD_EQUIPMENT_PREFAB,
				site.m_vTargetPosition)
			|| !ConfigureProjectionForMission(state, site, mission, asset))
			return QuarantineMissionAggregate(
				state,
				mission,
				"active rebuild equipment lacks exact evidence components or mission identity") || changed;
		return changed;
	}

	protected GenericEntity SpawnProjectionPrefab(string prefab, vector position)
	{
		if (prefab.IsEmpty() || IsZeroVectorStatic(position))
			return null;
		vector resolved = HST_WorldPositionService.ResolveSafeGroundPosition(
			position,
			HST_WorldPositionService.PROP_GROUND_OFFSET,
			false,
			2.0);
		GenericEntity entity = HST_WorldPositionService.SpawnPrefab(
			prefab,
			resolved,
			HST_WorldPositionService.BuildUprightAngles(0));
		if (entity)
			HST_WorldPositionService.ApplyUprightEntityTransform(
				entity,
				resolved,
				HST_WorldPositionService.BuildUprightAngles(0));
		return entity;
	}

	protected bool ResetOwnedProjectionAfterUnsupportedDamage(
		HST_CampaignState state,
		HST_RadioSiteState site,
		HST_ActiveMissionState mission,
		HST_MissionAssetState asset)
	{
		if (!state || !site
			|| site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
			return false;
		bool changed = ForgetProjection(site.m_sSiteId, true);
		if (asset)
			asset.m_bSpawned = false;
		if (mission)
			mission.m_bRuntimeSpawned = false;
		if (asset)
		{
			HST_MissionRuntimeEntityState runtime = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtime)
				runtime.m_bSpawned = false;
		}
		// Owned generated targets reject raw damage-state completion. Recreate the
		// projection while preserving exact explosive-score progress and receipts.
		return EnsureSiteProjection(state, site) || changed;
	}

	protected void CollectTransmitterCandidates(vector center, float radius, string expectedSiteId)
	{
		m_aTransmitterCandidates.Clear();
		m_sCandidateExpectedSiteId = expectedSiteId;
		if (!GetGame() || !GetGame().GetWorld())
			return;
		GetGame().GetWorld().QueryEntitiesBySphere(
			center,
			radius,
			AddTransmitterCandidate,
			null,
			EQueryEntitiesFlags.ALL);
	}

	protected bool AddTransmitterCandidate(IEntity entity)
	{
		if (!entity || entity.IsDeleted())
			return true;
		IEntity candidate = entity.GetRootParent();
		if (!candidate)
			candidate = entity;
		if (candidate.IsDeleted())
			return true;

		bool recognized = IsTransmitterEntity(entity) || IsTransmitterEntity(candidate);
		if (!recognized || m_aTransmitterCandidates.Contains(candidate))
			return true;
		int trackedIndex = m_aProjectionEntities.Find(candidate);
		if (trackedIndex >= 0 && m_aProjectionSiteIds[trackedIndex] == m_sCandidateExpectedSiteId)
			return true;
		if (trackedIndex < 0 && HST_MissionAssetComponent.Cast(candidate.FindComponent(HST_MissionAssetComponent)))
			return true;
		m_aTransmitterCandidates.Insert(candidate);
		return true;
	}

	protected bool IsTransmitterEntity(IEntity entity)
	{
		if (!entity)
			return false;
		string prefab = ResolveEntityPrefab(entity);
		if (!prefab.IsEmpty())
			return IsSupportedTransmitterPrefab(prefab);
		MapDescriptorComponent descriptor = MapDescriptorComponent.Cast(
			entity.FindComponent(MapDescriptorComponent));
		if (descriptor && descriptor.GetBaseType() == EMapDescriptorType.MDT_TRANSMITTER)
			return true;
		return false;
	}

	protected string ResolveEntityPrefab(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "";
		return entity.GetPrefabData().GetPrefabName();
	}

	protected bool SuppressAuthoredTransmitterResurrection(
		HST_CampaignState state,
		HST_RadioSiteState site)
	{
		if (!state || !site || site.m_sAuthoredTargetPrefab.IsEmpty()
			|| IsZeroVectorStatic(site.m_vAuthoredTargetPosition))
			return false;
		CollectTransmitterCandidates(
			site.m_vAuthoredTargetPosition,
			AUTHORED_SUPPRESSION_RADIUS_METERS,
			site.m_sSiteId);
		if (HasCandidateClaimedByOtherSite(site.m_sSiteId))
			return QuarantineSite(state, site, "authored transmitter suppression overlaps another radio site");
		if (m_aTransmitterCandidates.Count() > 1)
			return QuarantineSite(state, site, "authored transmitter suppression target is ambiguous");
		if (m_aTransmitterCandidates.Count() == 0)
			return false;
		IEntity authored = m_aTransmitterCandidates[0];
		if (!FrozenAuthoredCandidateMatchesSite(site, authored))
			return QuarantineSite(state, site, "authored transmitter suppression candidate conflicts with frozen provenance");
		SCR_DamageManagerComponent damageManager = SCR_DamageManagerComponent.Cast(
			authored.FindComponent(SCR_DamageManagerComponent));
		if (!damageManager)
			return QuarantineSite(state, site, "authored transmitter suppression target lost its damage authority");
		if (damageManager.GetState() == EDamageState.DESTROYED)
			return false;
		if (!damageManager.SetHealthScaled(0.0)
			|| damageManager.GetState() != EDamageState.DESTROYED)
			return QuarantineSite(state, site, "authored transmitter suppression could not reapply the frozen destroyed state");
		return true;
	}

	protected void RegisterProjection(string siteId, string missionInstanceId, IEntity entity, bool borrowed)
	{
		if (siteId.IsEmpty() || !entity)
			return;
		int index = m_aProjectionSiteIds.Find(siteId);
		if (index >= 0)
		{
			m_aProjectionMissionInstanceIds[index] = missionInstanceId;
			m_aProjectionEntities[index] = entity;
			m_aProjectionBorrowed[index] = borrowed;
			return;
		}
		m_aProjectionSiteIds.Insert(siteId);
		m_aProjectionMissionInstanceIds.Insert(missionInstanceId);
		m_aProjectionEntities.Insert(entity);
		m_aProjectionBorrowed.Insert(borrowed);
	}

	protected IEntity FindProjection(string siteId)
	{
		int index = m_aProjectionSiteIds.Find(siteId);
		if (index < 0 || index >= m_aProjectionEntities.Count())
			return null;
		IEntity entity = m_aProjectionEntities[index];
		if (entity && !entity.IsDeleted())
			return entity;
		m_aProjectionSiteIds.Remove(index);
		m_aProjectionMissionInstanceIds.Remove(index);
		m_aProjectionEntities.Remove(index);
		m_aProjectionBorrowed.Remove(index);
		return null;
	}

	protected bool IsTrackedProjectionEntity(IEntity entity)
	{
		return entity && !entity.IsDeleted() && m_aProjectionEntities.Contains(entity);
	}

	protected bool HasCandidateClaimedByOtherSite(string expectedSiteId)
	{
		foreach (IEntity candidate : m_aTransmitterCandidates)
		{
			int index = m_aProjectionEntities.Find(candidate);
			if (index >= 0 && m_aProjectionSiteIds[index] != expectedSiteId)
				return true;
		}
		return false;
	}

	protected bool ForgetProjection(string siteId, bool deleteOwned)
	{
		int index = m_aProjectionSiteIds.Find(siteId);
		if (index < 0)
			return false;
		IEntity entity = m_aProjectionEntities[index];
		bool borrowed = m_aProjectionBorrowed[index];
		if (deleteOwned && entity && !entity.IsDeleted() && !borrowed)
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
		m_aProjectionSiteIds.Remove(index);
		m_aProjectionMissionInstanceIds.Remove(index);
		m_aProjectionEntities.Remove(index);
		m_aProjectionBorrowed.Remove(index);
		return true;
	}

	protected void ClearTrackedProjections(bool deleteOwned)
	{
		for (int index = m_aProjectionSiteIds.Count() - 1; index >= 0; index--)
		{
			IEntity entity = m_aProjectionEntities[index];
			bool borrowed = m_aProjectionBorrowed[index];
			if (deleteOwned && entity && !entity.IsDeleted() && !borrowed)
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
		}
		m_aProjectionSiteIds.Clear();
		m_aProjectionMissionInstanceIds.Clear();
		m_aProjectionEntities.Clear();
		m_aProjectionBorrowed.Clear();
	}

	protected void FinalizeDestroyProjection(HST_RadioSiteState site, bool succeeded)
	{
		if (!site)
			return;
		int index = m_aProjectionSiteIds.Find(site.m_sSiteId);
		if (index < 0)
			return;
		IEntity projection = m_aProjectionEntities[index];
		bool borrowed = m_aProjectionBorrowed[index];
		HST_MissionAssetComponent component;
		if (projection && !projection.IsDeleted())
			component = HST_MissionAssetComponent.Cast(projection.FindComponent(HST_MissionAssetComponent));
		if (component)
			component.ConfigureMissionAsset("", "", "");
		if (succeeded && !borrowed)
			ForgetProjection(site.m_sSiteId, true);
	}

	protected HST_MissionAssetState FindExactMissionAsset(HST_CampaignState state, string missionInstanceId)
	{
		HST_MissionAssetState match;
		if (!state || missionInstanceId.IsEmpty())
			return null;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != missionInstanceId
				|| asset.m_iRadioSiteContractVersion != EXACT_CONTRACT_VERSION)
				continue;
			if (match)
				return null;
			match = asset;
		}
		return match;
	}

	protected int CountMissionAssets(HST_CampaignState state, string missionInstanceId)
	{
		int count;
		if (!state || missionInstanceId.IsEmpty())
			return count;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == missionInstanceId)
				count++;
		}
		return count;
	}

	protected HST_MissionObjectiveState FindExactMissionObjective(
		HST_CampaignState state,
		string missionInstanceId)
	{
		HST_MissionObjectiveState match;
		if (!state || missionInstanceId.IsEmpty())
			return null;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != missionInstanceId)
				continue;
			if (match)
				return null;
			match = objective;
		}
		return match;
	}

	protected int CountMissionObjectives(HST_CampaignState state, string missionInstanceId)
	{
		int count;
		if (!state || missionInstanceId.IsEmpty())
			return count;
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == missionInstanceId)
				count++;
		}
		return count;
	}

	protected int CountMissionRuntimeEntities(HST_CampaignState state, string missionInstanceId)
	{
		int count;
		if (!state || missionInstanceId.IsEmpty())
			return count;
		foreach (HST_MissionRuntimeEntityState runtime : state.m_aMissionRuntimeEntities)
		{
			if (runtime && runtime.m_sMissionInstanceId == missionInstanceId)
				count++;
		}
		return count;
	}

	protected int CountMissionTasks(HST_CampaignState state, string missionInstanceId)
	{
		int count;
		if (!state || missionInstanceId.IsEmpty())
			return count;
		foreach (HST_CampaignTaskState task : state.m_aCampaignTasks)
		{
			if (task && task.m_sLinkedId == missionInstanceId)
				count++;
		}
		return count;
	}

	protected int CountSitesForZone(HST_CampaignState state, string zoneId)
	{
		int count;
		if (!state || zoneId.IsEmpty())
			return count;
		foreach (HST_RadioSiteState site : state.m_aRadioSites)
		{
			if (site && site.m_sZoneId == zoneId)
				count++;
		}
		return count;
	}

	protected HST_RadioSiteTransitionResult NewResult(HST_ActiveMissionState mission)
	{
		HST_RadioSiteTransitionResult result = new HST_RadioSiteTransitionResult();
		result.m_Mission = mission;
		return result;
	}

	protected HST_RadioSiteTransitionResult Reject(HST_RadioSiteTransitionResult result, string reason)
	{
		if (!result)
		{
			HST_RadioSiteTransitionResult fallback = new HST_RadioSiteTransitionResult();
			fallback.m_bAccepted = false;
			fallback.m_sReason = reason;
			return fallback;
		}
		result.m_bAccepted = false;
		result.m_sReason = reason;
		return result;
	}

	protected HST_RadioSiteTransitionResult QuarantineAdmission(
		HST_CampaignState state,
		HST_RadioSiteTransitionResult result,
		string reason)
	{
		if (result && result.m_Site)
			QuarantineSite(state, result.m_Site, reason);
		if (result && result.m_Mission)
			CleanupQuarantinedMissionAggregate(state, result.m_Mission, reason);
		return Reject(result, reason);
	}

	protected bool QuarantineSite(HST_CampaignState state, HST_RadioSiteState site, string reason)
	{
		if (!site)
			return false;
		bool changed = site.m_iContractVersion != QUARANTINED_CONTRACT_VERSION
			|| site.m_eLifecycleState != HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_QUARANTINED;
		string activeMissionInstanceId = site.m_sActiveMissionInstanceId;
		string lastTransitionMissionInstanceId = site.m_sLastTransitionMissionInstanceId;
		string destructionMissionInstanceId = site.m_sLastDestructionMissionInstanceId;
		string rebuildMissionInstanceId = site.m_sLastRebuildMissionInstanceId;
		site.m_iContractVersion = QUARANTINED_CONTRACT_VERSION;
		site.m_eLifecycleState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_QUARANTINED;
		site.m_sLastTransitionReason = "schema-59 radio-site quarantine: " + reason;
		site.m_iLastTransitionSecond = Math.Max(0, state.m_iElapsedSeconds);
		if (changed)
			site.m_iRevision = Math.Max(1, site.m_iRevision) + 1;
		ClearActiveLock(site);
		// Quarantine may retire only a projection this service owns. A borrowed
		// authored transmitter is forgotten but never deleted.
		ForgetProjection(site.m_sSiteId, true);
		foreach (HST_ActiveMissionState mission : state.m_aActiveMissions)
		{
			if (!mission
				|| (mission.m_sRadioSiteId != site.m_sSiteId
					&& (activeMissionInstanceId.IsEmpty()
						|| mission.m_sInstanceId != activeMissionInstanceId)
					&& (lastTransitionMissionInstanceId.IsEmpty()
						|| mission.m_sInstanceId != lastTransitionMissionInstanceId)
					&& (destructionMissionInstanceId.IsEmpty()
						|| mission.m_sInstanceId != destructionMissionInstanceId)
					&& (rebuildMissionInstanceId.IsEmpty()
						|| mission.m_sInstanceId != rebuildMissionInstanceId)))
				continue;
			changed = CleanupQuarantinedMissionAggregate(state, mission, reason) || changed;
		}
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset
				|| (asset.m_sRadioSiteId != site.m_sSiteId
					&& (activeMissionInstanceId.IsEmpty()
						|| asset.m_sMissionInstanceId != activeMissionInstanceId)
					&& (lastTransitionMissionInstanceId.IsEmpty()
						|| asset.m_sMissionInstanceId != lastTransitionMissionInstanceId)
					&& (destructionMissionInstanceId.IsEmpty()
						|| asset.m_sMissionInstanceId != destructionMissionInstanceId)
					&& (rebuildMissionInstanceId.IsEmpty()
						|| asset.m_sMissionInstanceId != rebuildMissionInstanceId)))
				continue;
			asset.m_iRadioSiteContractVersion = QUARANTINED_CONTRACT_VERSION;
			asset.m_sLastInteraction = "schema59_radio_site_quarantined";
			asset.m_bSpawned = false;
			changed = true;
			HST_MissionRuntimeEntityState runtime = state.FindMissionRuntimeEntity(asset.m_sEntityId);
			if (runtime)
				runtime.m_bSpawned = false;
		}
		foreach (HST_MissionRuntimeEntityState orphanRuntime : state.m_aMissionRuntimeEntities)
		{
			if (orphanRuntime
				&& ((!activeMissionInstanceId.IsEmpty()
						&& orphanRuntime.m_sMissionInstanceId == activeMissionInstanceId)
					|| (!lastTransitionMissionInstanceId.IsEmpty()
						&& orphanRuntime.m_sMissionInstanceId == lastTransitionMissionInstanceId)
					|| (!destructionMissionInstanceId.IsEmpty()
						&& orphanRuntime.m_sMissionInstanceId == destructionMissionInstanceId)
					|| (!rebuildMissionInstanceId.IsEmpty()
						&& orphanRuntime.m_sMissionInstanceId == rebuildMissionInstanceId)))
				orphanRuntime.m_bSpawned = false;
		}
		return changed;
	}

	protected bool CleanupQuarantinedMissionAggregate(
		HST_CampaignState state,
		HST_ActiveMissionState mission,
		string reason)
	{
		if (!state || !mission)
			return false;
		bool failCurrentOutcome = mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			|| !mission.m_bRuntimeCleanupComplete;
		bool changed = mission.m_iRadioSiteContractVersion != QUARANTINED_CONTRACT_VERSION
			|| mission.m_bRuntimeSpawned || !mission.m_bRuntimeCleanupComplete
			|| (failCurrentOutcome
				&& mission.m_eStatus != HST_EMissionStatus.HST_MISSION_FAILED);
		mission.m_iRadioSiteContractVersion = QUARANTINED_CONTRACT_VERSION;
		if (failCurrentOutcome)
			mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
		mission.m_sRuntimePhase = "radio_site_authority_quarantined";
		mission.m_sRuntimeFailureReason = reason;
		mission.m_bRuntimeSpawned = false;
		mission.m_bRuntimeCleanupComplete = true;
		foreach (HST_MissionAssetState asset : state.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			asset.m_iRadioSiteContractVersion = QUARANTINED_CONTRACT_VERSION;
			asset.m_sLastInteraction = "schema59_radio_site_quarantined";
			asset.m_bSpawned = false;
		}
		foreach (HST_MissionRuntimeEntityState runtime : state.m_aMissionRuntimeEntities)
		{
			if (runtime && runtime.m_sMissionInstanceId == mission.m_sInstanceId)
				runtime.m_bSpawned = false;
		}
		foreach (HST_MissionObjectiveState objective : state.m_aMissionObjectives)
		{
			if (!failCurrentOutcome || !objective
				|| objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			objective.m_bFailed = true;
			objective.m_bCleanupComplete = true;
		}
		foreach (HST_CampaignTaskState task : state.m_aCampaignTasks)
		{
			if (!failCurrentOutcome || !task || task.m_sLinkedId != mission.m_sInstanceId)
				continue;
			task.m_bActive = false;
			task.m_bSucceeded = false;
			task.m_bFailed = true;
		}
		return changed;
	}

	protected bool PositionsMatch2D(vector first, vector second, float toleranceMeters)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz <= toleranceMeters * toleranceMeters;
	}

	protected bool FrozenCandidateMatchesSite(HST_RadioSiteState site, IEntity candidate)
	{
		if (!site || !candidate || candidate.IsDeleted())
			return false;
		return ResolveEntityPrefab(candidate) == site.m_sTargetPrefab
			&& PositionsMatch2D(
				candidate.GetOrigin(),
				site.m_vTargetPosition,
				BINDING_POSITION_TOLERANCE_METERS);
	}

	protected bool FrozenAuthoredCandidateMatchesSite(HST_RadioSiteState site, IEntity candidate)
	{
		if (!site || !candidate || candidate.IsDeleted()
			|| site.m_sAuthoredTargetPrefab.IsEmpty()
			|| IsZeroVectorStatic(site.m_vAuthoredTargetPosition))
			return false;
		return ResolveEntityPrefab(candidate) == site.m_sAuthoredTargetPrefab
			&& PositionsMatch2D(
				candidate.GetOrigin(),
				site.m_vAuthoredTargetPosition,
				BINDING_POSITION_TOLERANCE_METERS);
	}

	protected static bool IsZeroVectorStatic(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}
}
