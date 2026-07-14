// Schema-59 restore boundary for durable radio-site lifecycle authority.
// Pre-schema-59 saves gain one ONLINE logical site per radio zone, but no
// physical ownership, binding, destruction, rebuild, mission, or receipt is
// inferred. Current-schema ambiguity is quarantined instead of guessed.
class HST_RadioSiteSaveValidationService
{
	static const int SCHEMA_VERSION = 59;
	static const string DESTROY_MISSION_ID = "destroy_radio_tower";
	static const string REBUILD_MISSION_ID = "dynamic_stop_tower_rebuild";
	static const string TARGET_KIND = "target";
	static const string TARGET_ROLE = "destroy_target";
	static const float REQUIRED_DEMOLITION_DAMAGE = 300.0;
	static const float MAX_TARGET_ZONE_DISTANCE_METERS = 220.0;
	static const float FROZEN_BINDING_TOLERANCE_METERS = 0.75;
	static const float PHYSICAL_PROJECTION_TOLERANCE_METERS = 12.0;
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
	static const string MIGRATION_EVENT_ID = "migration_schema59_radio_site_authority";
	static const string CONFLICT_EVENT_ID = "normalization_schema59_radio_site_authority_conflict";
	static const string TERMINAL_OBJECTIVE_COMPATIBILITY_EVENT_ID = "normalization_schema59_terminal_radio_objective_compatibility";
	static const string QUARANTINE_PHASE = "radio_site_authority_quarantined";

	protected HST_CampaignSaveData m_SaveData;

	void Normalize(HST_CampaignSaveData saveData, int restoredSchemaVersion)
	{
		m_SaveData = saveData;
		if (!m_SaveData)
			return;

		int conflictCount = RemoveNullSiteRows();
		if (restoredSchemaVersion < SCHEMA_VERSION)
			conflictCount += MigratePreSchema59Authority();
		else
			conflictCount += EnsureCurrentSchemaRadioRows();
		int normalizedTerminalObjectiveCount = NormalizeTerminalObjectiveCompatibility();

		foreach (HST_RadioSiteState site : m_SaveData.m_aRadioSites)
		{
			if (!site)
				continue;
			if (site.m_iContractVersion == HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION)
			{
				NormalizeQuarantinedSite(site);
				continue;
			}

			string failure = ValidateSite(site);
			if (failure.IsEmpty())
				continue;
			QuarantineSiteAggregate(site, failure);
			conflictCount++;
		}

		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!IsSchema59RadioMissionClaimant(m_SaveData, mission))
				continue;
			if (mission.m_iRadioSiteContractVersion == HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION)
			{
				NormalizeQuarantinedMission(mission);
				continue;
			}

			HST_RadioSiteState site = FindUniqueSite(mission.m_sRadioSiteId);
			if (!site && IsRadioLifecycleMissionId(mission.m_sMissionId))
				site = FindUniqueSiteForZone(mission.m_sTargetZoneId);
			HST_MissionAssetState asset = FindUniqueMissionAsset(mission.m_sInstanceId);
			string failure = ValidateMissionAggregate(mission, site, asset);
			if (failure.IsEmpty())
				continue;
			QuarantineMissionAggregate(mission, site, failure);
			conflictCount++;
		}

		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!IsSchema59RadioAssetClaimant(m_SaveData, asset))
				continue;
			if (asset.m_iRadioSiteContractVersion == HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION)
				continue;

			HST_ActiveMissionState mission = FindUniqueMission(asset.m_sMissionInstanceId);
			HST_RadioSiteState site = FindUniqueSite(asset.m_sRadioSiteId);
			string failure;
			if (!mission || mission.m_iRadioSiteContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION)
				failure = "exact radio target asset has no reciprocal exact mission";
			else if (!site || site.m_iContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION)
				failure = "exact radio target asset has no reciprocal exact site";
			else
				failure = ValidateMissionAsset(asset, mission, site);
			if (failure.IsEmpty())
				continue;
			QuarantineMissionAggregate(mission, site, failure);
			QuarantineAsset(asset, failure);
			conflictCount++;
		}

		RecordTerminalObjectiveCompatibility(normalizedTerminalObjectiveCount);
		RecordNormalizationConflict(conflictCount);
		m_SaveData = null;
	}

	string ValidateCurrentAggregate(HST_CampaignSaveData saveData, HST_RadioSiteState site)
	{
		m_SaveData = saveData;
		if (!m_SaveData || !site)
		{
			m_SaveData = null;
			return "radio-site authority is unavailable";
		}

		string failure = ValidateSite(site);
		if (failure.IsEmpty())
		{
			string missionInstanceId = site.m_sActiveMissionInstanceId;
			if (missionInstanceId.IsEmpty())
				missionInstanceId = site.m_sLastTransitionMissionInstanceId;
			if (!missionInstanceId.IsEmpty())
			{
				HST_ActiveMissionState mission = FindUniqueMission(missionInstanceId);
				HST_MissionAssetState asset = FindUniqueMissionAsset(missionInstanceId);
				failure = ValidateMissionAggregate(mission, site, asset);
			}
		}
		m_SaveData = null;
		return failure;
	}

	static bool IsSchema59RadioMissionClaimant(HST_CampaignSaveData saveData, HST_ActiveMissionState mission)
	{
		if (!saveData || !mission)
			return false;
		if (mission.m_iRadioSiteContractVersion == HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
			|| mission.m_iRadioSiteContractVersion == HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION)
			return true;
		if (mission.m_iRadioSiteContractVersion == 0
			&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& IsRadioLifecycleMissionId(mission.m_sMissionId))
			return true;

		foreach (HST_RadioSiteState site : saveData.m_aRadioSites)
		{
			if (!site || mission.m_sInstanceId.IsEmpty())
				continue;
			if (site.m_sActiveMissionInstanceId == mission.m_sInstanceId
				|| site.m_sLastTransitionMissionInstanceId == mission.m_sInstanceId
				|| site.m_sLastDestructionMissionInstanceId == mission.m_sInstanceId
				|| site.m_sLastRebuildMissionInstanceId == mission.m_sInstanceId)
				return true;
		}
		return false;
	}

	static bool IsSchema59RadioAssetClaimant(HST_CampaignSaveData saveData, HST_MissionAssetState asset)
	{
		if (!saveData || !asset)
			return false;
		if (asset.m_iRadioSiteContractVersion == HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
			|| asset.m_iRadioSiteContractVersion == HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION)
			return true;

		foreach (HST_ActiveMissionState mission : saveData.m_aActiveMissions)
		{
			if (!IsSchema59RadioMissionClaimant(saveData, mission)
				|| mission.m_sInstanceId.IsEmpty() || mission.m_sRadioSiteId.IsEmpty())
				continue;
			if (asset.m_sMissionInstanceId == mission.m_sInstanceId
				&& asset.m_sRadioSiteId == mission.m_sRadioSiteId)
				return true;
		}
		return false;
	}

	protected int RemoveNullSiteRows()
	{
		int removed;
		for (int index = m_SaveData.m_aRadioSites.Count() - 1; index >= 0; index--)
		{
			if (m_SaveData.m_aRadioSites[index])
				continue;
			m_SaveData.m_aRadioSites.Remove(index);
			removed++;
		}
		return removed;
	}

	protected int MigratePreSchema59Authority()
	{
		int discardedUnsupportedRows = m_SaveData.m_aRadioSites.Count();
		m_SaveData.m_aRadioSites.Clear();

		int failedLegacyActiveMissions;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!mission)
				continue;
			if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
				&& IsRadioLifecycleMissionId(mission.m_sMissionId))
			{
				mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
				mission.m_sRuntimePhase = "schema59_legacy_radio_authority_failed";
				mission.m_sRuntimeFailureReason = "pre-schema-59 active radio mission failed closed because it had no exclusive durable site owner";
				failedLegacyActiveMissions++;
			}
			mission.m_iRadioSiteContractVersion = 0;
			mission.m_sRadioSiteId = "";
			mission.m_sRadioSiteTransitionRequestId = "";
			mission.m_iRadioSiteRevision = 0;
		}
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset)
				continue;
			asset.m_iRadioSiteContractVersion = 0;
			asset.m_sRadioSiteId = "";
		}

		array<string> migratedZoneIds = {};
		int createdCount;
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_RADIO_TOWER
				|| zone.m_sZoneId.IsEmpty() || migratedZoneIds.Contains(zone.m_sZoneId))
				continue;

			HST_RadioSiteState site = CreateLogicalSite(zone, false);
			m_SaveData.m_aRadioSites.Insert(site);
			migratedZoneIds.Insert(zone.m_sZoneId);
			createdCount++;
		}

		if (HasEvent(MIGRATION_EVENT_ID))
			return discardedUnsupportedRows;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = MIGRATION_EVENT_ID;
		eventState.m_sCategory = "migration";
		eventState.m_sAggregateType = "radio_site";
		eventState.m_sAggregateId = "schema59";
		eventState.m_sTransition = "legacy_radio_zones_initialized_online_unresolved";
		eventState.m_sReason = string.Format(
			"created %1 ONLINE logical radio sites with unresolved physical ownership, discarded %2 unsupported pre-schema-59 site rows, failed closed %3 active legacy radio missions without applying outcomes or rewards, retained terminal historical radio missions and all assets at contract zero, and inferred no binding, destruction, rebuild, or receipt",
			createdCount,
			discardedUnsupportedRows,
			failedLegacyActiveMissions);
		eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(eventState);
		return discardedUnsupportedRows;
	}

	protected int EnsureCurrentSchemaRadioRows()
	{
		array<string> handledZoneIds = {};
		int missingCount;
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (!zone || zone.m_eType != HST_EZoneType.HST_ZONE_RADIO_TOWER
				|| zone.m_sZoneId.IsEmpty() || handledZoneIds.Contains(zone.m_sZoneId))
				continue;
			handledZoneIds.Insert(zone.m_sZoneId);
			if (CountSitesForZone(zone.m_sZoneId) > 0)
				continue;

			m_SaveData.m_aRadioSites.Insert(CreateLogicalSite(zone, true));
			missingCount++;
		}
		return missingCount;
	}

	protected HST_RadioSiteState CreateLogicalSite(HST_ZoneState zone, bool quarantine)
	{
		HST_RadioSiteState site = new HST_RadioSiteState();
		site.m_iContractVersion = HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION;
		site.m_sZoneId = zone.m_sZoneId;
		site.m_sSiteId = HST_RadioSiteLifecycleService.BuildSiteId(zone.m_sZoneId);
		site.m_sTargetId = HST_RadioSiteLifecycleService.BuildTargetId(zone.m_sZoneId);
		site.m_eLifecycleState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE;
		site.m_eTargetOwnership = HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED;
		site.m_sLastTransitionReason = "schema-59 migration preserved historical radio zone as online without inferring a physical binding";
		site.m_iLastTransitionSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		site.m_iRevision = 1;
		if (!quarantine)
			return site;

		site.m_iContractVersion = HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION;
		site.m_eLifecycleState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_QUARANTINED;
		site.m_sLastTransitionReason = "schema-59 current save omitted required durable radio-site authority";
		return site;
	}

	protected string ValidateSite(HST_RadioSiteState site)
	{
		if (!site || site.m_iContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION)
			return "radio-site contract version is unsupported";
		HST_ZoneState zone = FindUniqueRadioZone(site.m_sZoneId);
		if (!zone)
			return "radio-site zone authority is missing, ambiguous, or not a radio zone";
		if (site.m_sSiteId != HST_RadioSiteLifecycleService.BuildSiteId(site.m_sZoneId)
			|| site.m_sTargetId != HST_RadioSiteLifecycleService.BuildTargetId(site.m_sZoneId)
			|| CountSitesForSiteId(site.m_sSiteId) != 1
			|| CountSitesForZone(site.m_sZoneId) != 1
			|| CountSitesForTarget(site.m_sTargetId) != 1)
			return "radio-site stable identity is missing or ambiguous";
		if (site.m_iRevision < 1 || site.m_iDestroyedAtSecond < 0
			|| site.m_iRebuildStartedAtSecond < 0 || site.m_iRebuiltAtSecond < 0
			|| site.m_iLastTransitionSecond < 0)
			return "radio-site revision or transition time is invalid";
		if (site.m_iDestroyedAtSecond > m_SaveData.m_iElapsedSeconds
			|| site.m_iRebuildStartedAtSecond > m_SaveData.m_iElapsedSeconds
			|| site.m_iRebuiltAtSecond > m_SaveData.m_iElapsedSeconds
			|| site.m_iLastTransitionSecond > m_SaveData.m_iElapsedSeconds)
			return "radio-site transition time exceeds campaign authority";

		bool lifecycleValid = site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
			|| site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED
			|| site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING;
		if (!lifecycleValid)
			return "radio-site lifecycle state is unsupported";

		bool activeMission = !site.m_sActiveMissionInstanceId.IsEmpty();
		if (activeMission != !site.m_sActiveMissionId.IsEmpty()
			|| activeMission != !site.m_sActiveTransitionRequestId.IsEmpty())
			return "radio-site active mission lock is incomplete";
		bool lastTransition = !site.m_sLastTransitionRequestId.IsEmpty();
		if (lastTransition != !site.m_sLastTransitionMissionInstanceId.IsEmpty())
			return "radio-site last transition replay fingerprint is incomplete";
		if (!lastTransition
			&& (!site.m_sLastTransitionKind.IsEmpty()
				|| site.m_eLastTransitionFromState != HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_UNKNOWN
				|| site.m_eLastTransitionToState != HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_UNKNOWN
				|| site.m_iLastTransitionRecordedRevision != 0))
			return "radio-site empty replay fingerprint carries typed transition residue";
		if (lastTransition
			&& (site.m_sLastTransitionKind.IsEmpty()
				|| !IsOperationalLifecycle(site.m_eLastTransitionFromState)
				|| !IsOperationalLifecycle(site.m_eLastTransitionToState)
				|| site.m_eLastTransitionToState != site.m_eLifecycleState
				|| site.m_iLastTransitionRecordedRevision != site.m_iRevision))
			return "radio-site typed replay fingerprint conflicts with current lifecycle or revision";
		if ((!site.m_sLastDestructionReceiptId.IsEmpty()) != (!site.m_sLastDestructionMissionInstanceId.IsEmpty()))
			return "radio-site destruction receipt is incomplete";
		if ((!site.m_sLastRebuildReceiptId.IsEmpty()) != (!site.m_sLastRebuildMissionInstanceId.IsEmpty()))
			return "radio-site rebuild receipt is incomplete";
		if ((site.m_sLastDestructionReceiptId.IsEmpty() && site.m_iDestroyedAtSecond != 0)
			|| (site.m_sLastRebuildReceiptId.IsEmpty()
				&& site.m_eLifecycleState != HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING
				&& (site.m_iRebuildStartedAtSecond != 0 || site.m_iRebuiltAtSecond != 0)))
			return "radio-site transition timestamps lack reciprocal receipt evidence";
		if ((!site.m_sLastDestructionReceiptId.IsEmpty() && CountReceiptClaims(site.m_sLastDestructionReceiptId) != 1)
			|| (!site.m_sLastRebuildReceiptId.IsEmpty() && CountReceiptClaims(site.m_sLastRebuildReceiptId) != 1))
			return "radio-site transition receipt is reused across sites or transition kinds";
		if ((!site.m_sActiveTransitionRequestId.IsEmpty() && CountOtherSitesClaimingRequest(site, site.m_sActiveTransitionRequestId) > 0)
			|| (!site.m_sLastTransitionRequestId.IsEmpty() && CountOtherSitesClaimingRequest(site, site.m_sLastTransitionRequestId) > 0))
			return "radio-site transition request is reused across sites";
		if (activeMission
			&& (!lastTransition
				|| site.m_sLastTransitionMissionInstanceId != site.m_sActiveMissionInstanceId
				|| site.m_sLastTransitionRequestId != site.m_sActiveTransitionRequestId))
			return "radio-site admission fingerprint does not match its active mission lock";

		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED)
		{
			if (site.m_eLifecycleState != HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
				|| !site.m_sTargetPrefab.IsEmpty() || !IsZeroVector(site.m_vTargetPosition)
				|| !site.m_sAuthoredTargetPrefab.IsEmpty() || !IsZeroVector(site.m_vAuthoredTargetPosition)
				|| activeMission || lastTransition
				|| !site.m_sLastDestructionReceiptId.IsEmpty() || !site.m_sLastRebuildReceiptId.IsEmpty())
				return "unresolved radio-site binding carried invented lifecycle or mission authority";
			return "";
		}

		if (site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD
			&& site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
			return "radio-site target ownership is unsupported";
		if (!HST_RadioSiteLifecycleService.IsSupportedTransmitterPrefab(site.m_sAuthoredTargetPrefab)
			|| site.m_sAuthoredTargetPrefab == HST_RadioSiteLifecycleService.GENERATED_TOWER_PREFAB
			|| IsZeroVector(site.m_vAuthoredTargetPosition)
			|| DistanceSq2D(site.m_vAuthoredTargetPosition, zone.m_vPosition)
				> MAX_TARGET_ZONE_DISTANCE_METERS * MAX_TARGET_ZONE_DISTANCE_METERS)
			return "resolved radio-site frozen authored provenance is incomplete";
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD
			&& (site.m_sTargetPrefab != site.m_sAuthoredTargetPrefab
				|| !PositionsMatch(
					site.m_vTargetPosition,
					site.m_vAuthoredTargetPosition,
					FROZEN_BINDING_TOLERANCE_METERS)))
			return "borrowed radio-site target conflicts with frozen authored provenance";
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN
			&& (site.m_sTargetPrefab != HST_RadioSiteLifecycleService.GENERATED_TOWER_PREFAB
				|| !PositionsMatch(
					site.m_vTargetPosition,
					site.m_vAuthoredTargetPosition,
					FROZEN_BINDING_TOLERANCE_METERS)
				|| site.m_sLastDestructionReceiptId.IsEmpty()))
			return "generated radio-site target lacks exact rebuild/destruction provenance";
		if (!site.m_sLastRebuildReceiptId.IsEmpty()
			&& site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
			return "radio-site rebuild receipt does not own a generated campaign target";
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN
			&& site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
			&& (site.m_sLastRebuildReceiptId.IsEmpty() || site.m_iRebuiltAtSecond < site.m_iRebuildStartedAtSecond))
			return "generated ONLINE radio-site target lacks completed rebuild provenance";
		if (HasOtherResolvedSiteAtFrozenBinding(site))
			return "multiple radio sites claim the same frozen physical transmitter binding";
		if ((site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED
				|| site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING)
			&& site.m_sLastDestructionReceiptId.IsEmpty())
			return "offline radio-site lacks durable destruction evidence";
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING
			&& site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
			return "rebuilding radio-site does not own a generated campaign target";
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED)
		{
			if (site.m_sLastRebuildReceiptId.IsEmpty()
				&& (site.m_iRebuildStartedAtSecond != 0 || site.m_iRebuiltAtSecond != 0))
				return "destroyed radio-site retained rebuild timestamps without an attempt receipt";
			if (!site.m_sLastRebuildReceiptId.IsEmpty()
				&& (site.m_iRebuiltAtSecond != 0
					|| site.m_iRebuildStartedAtSecond < site.m_iDestroyedAtSecond))
				return "stopped rebuild receipt conflicts with the current destruction epoch";
		}
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING
			&& (!site.m_sLastRebuildReceiptId.IsEmpty()
				|| site.m_iRebuiltAtSecond != 0
				|| site.m_iRebuildStartedAtSecond < site.m_iDestroyedAtSecond
				|| !activeMission || site.m_sActiveMissionId != REBUILD_MISSION_ID))
			return "rebuilding radio-site carries completed or non-monotonic rebuild evidence";
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
			&& !site.m_sLastRebuildReceiptId.IsEmpty()
			&& (site.m_eTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN
				|| site.m_iRebuiltAtSecond < 0
				|| site.m_iRebuiltAtSecond < site.m_iRebuildStartedAtSecond))
			return "rebuilt online radio-site lacks completed generated-target authority";
		if (!site.m_sLastDestructionReceiptId.IsEmpty()
			&& (site.m_iDestroyedAtSecond > site.m_iLastTransitionSecond
				|| (!site.m_sLastRebuildReceiptId.IsEmpty() && site.m_iRebuildStartedAtSecond < site.m_iDestroyedAtSecond)
				|| (site.m_iRebuiltAtSecond > 0 && site.m_iRebuiltAtSecond < site.m_iRebuildStartedAtSecond)))
			return "radio-site transition timestamps are not monotonic";
		if (site.m_eLifecycleState == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING
			&& site.m_iRebuildStartedAtSecond > site.m_iLastTransitionSecond)
			return "radio-site rebuilding timestamp exceeds the latest transition";

		if (activeMission)
		{
			HST_ActiveMissionState mission = FindUniqueMission(site.m_sActiveMissionInstanceId);
			if (!mission || mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
				|| mission.m_iRadioSiteContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
				|| mission.m_sMissionId != site.m_sActiveMissionId
				|| mission.m_sRadioSiteId != site.m_sSiteId
				|| mission.m_sRadioSiteTransitionRequestId != site.m_sActiveTransitionRequestId
				|| mission.m_iRadioSiteRevision != site.m_iRevision)
				return "radio-site active mission lock is not reciprocal at the current revision";
			if ((site.m_sActiveMissionId == DESTROY_MISSION_ID
					&& site.m_eLifecycleState != HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE)
				|| (site.m_sActiveMissionId == REBUILD_MISSION_ID
					&& site.m_eLifecycleState != HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING)
				|| !IsRadioLifecycleMissionId(site.m_sActiveMissionId))
				return "radio-site active mission conflicts with lifecycle state";
		}

		if (lastTransition)
		{
			HST_ActiveMissionState transitionMission = FindUniqueMission(site.m_sLastTransitionMissionInstanceId);
			if (!transitionMission
				|| transitionMission.m_iRadioSiteContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
				|| transitionMission.m_sRadioSiteId != site.m_sSiteId
				|| transitionMission.m_iRadioSiteRevision != site.m_iRevision)
				return "radio-site last transition replay fingerprint is not reciprocal";
			string expectedTransitionRequestId = BuildExpectedTransitionRequestId(site, transitionMission);
			if (expectedTransitionRequestId.IsEmpty()
				|| site.m_sLastTransitionRequestId != expectedTransitionRequestId)
				return "radio-site latest transition request is not deterministic";
			string outcomeFailure = ValidateLatestMissionOutcome(site, transitionMission);
			if (!outcomeFailure.IsEmpty())
				return outcomeFailure;
		}
		string receiptFailure = ValidateReceiptBacklinks(site);
		if (!receiptFailure.IsEmpty())
			return receiptFailure;
		return "";
	}

	protected string ValidateMissionAggregate(
		HST_ActiveMissionState mission,
		HST_RadioSiteState site,
		HST_MissionAssetState asset)
	{
		if (!mission || mission.m_iRadioSiteContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
			|| !IsRadioLifecycleMissionId(mission.m_sMissionId))
			return "exact radio lifecycle mission contract or type is invalid";
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
			&& mission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED
			&& mission.m_eStatus != HST_EMissionStatus.HST_MISSION_FAILED
			&& mission.m_eStatus != HST_EMissionStatus.HST_MISSION_EXPIRED)
			return "exact radio lifecycle mission status is invalid";
		if (mission.m_sInstanceId.IsEmpty() || CountMissionsForInstance(mission.m_sInstanceId) != 1
				|| mission.m_sRadioSiteId.IsEmpty() || mission.m_sRadioSiteTransitionRequestId.IsEmpty()
				|| CountMissionsForTransitionRequest(mission.m_sRadioSiteTransitionRequestId) != 1
				|| mission.m_iRadioSiteRevision < 1)
			return "exact radio lifecycle mission identity or replay fingerprint is invalid";
		if (!site || site.m_iContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
			|| site.m_sSiteId != mission.m_sRadioSiteId || site.m_sZoneId != mission.m_sTargetZoneId)
			return "exact radio lifecycle mission has no reciprocal site";
		if (mission.m_sRadioSiteTransitionRequestId
			!= HST_RadioSiteLifecycleService.BuildAdmissionRequestId(
				site.m_sSiteId,
				mission.m_sInstanceId,
				mission.m_sMissionId))
			return "exact radio lifecycle mission admission request is not deterministic";
		if (site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED
			|| mission.m_iRadioSiteRevision > site.m_iRevision)
			return "exact radio lifecycle mission binding or revision is invalid";
		string expectedRuntimePrimitive = "radio_site_destroy";
		if (mission.m_sMissionId == REBUILD_MISSION_ID)
			expectedRuntimePrimitive = "radio_site_rebuild";
		if (mission.m_sRuntimePrimitive != expectedRuntimePrimitive || mission.m_bRuntimeFallback)
			return "exact radio lifecycle mission runtime primitive or fallback authority is invalid";

		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE)
		{
			if (mission.m_iRadioSiteRevision != site.m_iRevision
				|| site.m_sActiveMissionInstanceId != mission.m_sInstanceId
				|| site.m_sActiveMissionId != mission.m_sMissionId
				|| site.m_sActiveTransitionRequestId != mission.m_sRadioSiteTransitionRequestId)
				return "active exact radio lifecycle mission lock is not reciprocal";
		}
		else if (site.m_sActiveMissionInstanceId == mission.m_sInstanceId)
			return "terminal exact radio lifecycle mission retained an active site lock";

		if (site.m_sLastTransitionMissionInstanceId == mission.m_sInstanceId
			&& site.m_iRevision != mission.m_iRadioSiteRevision)
			return "exact radio lifecycle mission conflicts with the latest replay fingerprint";
		if (!asset || CountAssetsForMission(mission.m_sInstanceId) != 1)
			return "exact radio lifecycle mission does not own exactly one target asset";
		string assetFailure = ValidateMissionAsset(asset, mission, site);
		if (!assetFailure.IsEmpty())
			return assetFailure;
		return ValidateMissionOutcomeEvidence(mission, site, asset);
	}

	protected string ValidateMissionAsset(
		HST_MissionAssetState asset,
		HST_ActiveMissionState mission,
		HST_RadioSiteState site)
	{
		if (!asset || !mission || !site
			|| asset.m_iRadioSiteContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
			|| asset.m_sRadioSiteId != site.m_sSiteId
			|| asset.m_sMissionInstanceId != mission.m_sInstanceId)
			return "exact radio target asset reciprocal identity is invalid";
		string expectedAssetId = "asset_" + mission.m_sInstanceId + "_destroy_target_0";
		string expectedEntityId = expectedAssetId + "_entity";
		if (asset.m_sAssetId != expectedAssetId || CountAssetsForAssetId(asset.m_sAssetId) != 1
			|| asset.m_sEntityId != expectedEntityId
			|| asset.m_sKind != TARGET_KIND || asset.m_sRole != TARGET_ROLE)
			return "exact radio target asset kind, role, or identity is invalid";
		if (asset.m_sRadioSiteAuthoredTargetPrefab != site.m_sAuthoredTargetPrefab
			|| !PositionsMatch(
				asset.m_vRadioSiteAuthoredTargetPosition,
				site.m_vAuthoredTargetPosition,
				FROZEN_BINDING_TOLERANCE_METERS)
			|| (asset.m_eRadioSiteTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD
				&& asset.m_eRadioSiteTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN))
			return "exact radio target asset lost its mission-time ownership or authored provenance";
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& asset.m_eRadioSiteTargetOwnership != site.m_eTargetOwnership)
			return "active exact radio target ownership conflicts with the current site";
		string expectedAssetPrefab = asset.m_sRadioSiteAuthoredTargetPrefab;
		if (asset.m_eRadioSiteTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
			expectedAssetPrefab = HST_RadioSiteLifecycleService.GENERATED_TOWER_PREFAB;
		if (mission.m_sMissionId == REBUILD_MISSION_ID)
		{
			if (asset.m_eRadioSiteTargetOwnership != HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN)
				return "exact rebuild equipment does not carry generated mission-time ownership";
			expectedAssetPrefab = HST_RadioSiteLifecycleService.REBUILD_EQUIPMENT_PREFAB;
		}
		if (asset.m_sPrefab != expectedAssetPrefab
			|| !PositionsMatch(asset.m_vSourcePosition, site.m_vTargetPosition)
			|| !PositionsMatch(mission.m_vTargetPosition, site.m_vTargetPosition))
			return "exact radio target asset binding conflicts with the durable site";
		if (asset.m_fDemolitionRequiredDamage != REQUIRED_DEMOLITION_DAMAGE
			|| asset.m_fDemolitionDamage < 0.0 || asset.m_iDemolitionHits < 0
			|| asset.m_iLastDemolitionSecond < 0
			|| asset.m_iLastDemolitionSecond > m_SaveData.m_iElapsedSeconds)
			return "exact radio target demolition evidence is invalid";
		if (!asset.m_aDemolitionEvidenceKeys
			|| asset.m_aDemolitionEvidenceKeys.Count() > HST_RadioSiteLifecycleService.MAX_DEMOLITION_EVIDENCE_KEYS
			|| asset.m_iDemolitionHits != asset.m_aDemolitionEvidenceKeys.Count())
			return "exact radio target durable demolition receipt count is invalid";
		array<string> uniqueEvidenceKeys = {};
		foreach (string evidenceKey : asset.m_aDemolitionEvidenceKeys)
		{
			if (evidenceKey.Trim().IsEmpty() || uniqueEvidenceKeys.Contains(evidenceKey))
				return "exact radio target durable demolition receipt identity is empty or duplicated";
			uniqueEvidenceKeys.Insert(evidenceKey);
		}
		bool hasDemolitionEvidence = asset.m_fDemolitionDamage > 0.0
			|| asset.m_iDemolitionHits > 0 || !asset.m_sLastDemolitionSource.IsEmpty()
			|| asset.m_aDemolitionEvidenceKeys.Count() > 0;
		if (hasDemolitionEvidence
			&& (asset.m_fDemolitionDamage <= 0.0 || asset.m_iDemolitionHits <= 0
				|| asset.m_sLastDemolitionSource.IsEmpty()
				|| asset.m_sLastDemolitionSource
					!= asset.m_aDemolitionEvidenceKeys[asset.m_aDemolitionEvidenceKeys.Count() - 1]))
			return "exact radio target demolition evidence is incomplete";
		if (!hasDemolitionEvidence && asset.m_iLastDemolitionSecond != 0)
			return "exact radio target demolition timestamp invents absent score evidence";
		bool generatedTarget = asset.m_eRadioSiteTargetOwnership
			== HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_GENERATED_CAMPAIGN;
		if (!generatedTarget
			&& (hasDemolitionEvidence || asset.m_iLastDemolitionSecond != 0))
			return "borrowed authored radio target cannot claim generated explosive-score evidence";
		if (generatedTarget)
		{
			bool demolitionThresholdReached = asset.m_fDemolitionDamage >= REQUIRED_DEMOLITION_DAMAGE;
			if (demolitionThresholdReached != asset.m_bDestroyed)
				return "generated radio target destruction does not match the exact explosive-score threshold";
		}
		HST_MissionRuntimeEntityState runtime = FindUniqueMissionRuntimeEntity(asset.m_sEntityId);
		if (!runtime || CountRuntimeEntitiesForMission(mission.m_sInstanceId) != 1
			|| runtime.m_sMissionInstanceId != mission.m_sInstanceId
			|| runtime.m_sKind != "radio_site_target"
			|| runtime.m_sPrefab != asset.m_sPrefab
			|| !PositionsMatch(
				runtime.m_vPosition,
				site.m_vTargetPosition,
				PHYSICAL_PROJECTION_TOLERANCE_METERS)
			|| !PositionsMatch(runtime.m_vPosition, asset.m_vCurrentPosition)
			|| !PositionsMatch(runtime.m_vPosition, asset.m_vLastKnownPosition)
			|| runtime.m_bSpawned != asset.m_bSpawned
			|| runtime.m_bDestroyed != asset.m_bDestroyed
			|| runtime.m_bRecovered)
			return "exact radio target runtime-entity backlink is invalid";
		bool activeMission = mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE;
		bool projectionFlagsLive = mission.m_bRuntimeSpawned
			&& asset.m_bSpawned && runtime.m_bSpawned;
		bool projectionFlagsDormant = !mission.m_bRuntimeSpawned
			&& !asset.m_bSpawned && !runtime.m_bSpawned;
		bool borrowedProjectionPending = projectionFlagsDormant && !asset.m_bDestroyed
			&& asset.m_eRadioSiteTargetOwnership
				== HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_BORROWED_WORLD
			&& mission.m_sRuntimePhase == "radio_site_projection_pending";
		bool unprojectedCompletionPending = projectionFlagsDormant && asset.m_bDestroyed
			&& mission.m_sRuntimePhase == "radio_site_target_destroyed";
		if (activeMission && (mission.m_bRuntimeCleanupComplete
			|| (!projectionFlagsLive && !borrowedProjectionPending
				&& !unprojectedCompletionPending)))
			return "active exact radio lifecycle projection flags are not live and reciprocal";
		if (!activeMission
			&& (!projectionFlagsDormant || !mission.m_bRuntimeCleanupComplete))
			return "terminal exact radio lifecycle projection flags are not fully cleaned up";
		return "";
	}

	protected string ValidateMissionOutcomeEvidence(
		HST_ActiveMissionState mission,
		HST_RadioSiteState site,
		HST_MissionAssetState asset)
	{
		if (!mission || !site || !asset)
			return "exact radio lifecycle outcome evidence is unavailable";
		HST_MissionObjectiveState objective = FindUniqueDestroyObjective(mission.m_sInstanceId);
		if (!objective || CountObjectivesForMission(mission.m_sInstanceId) != 1)
			return "exact radio lifecycle mission does not own exactly one destroy objective";
		HST_CampaignTaskState task = FindUniqueMissionTask(mission.m_sInstanceId);
		if (!task || CountTasksForMission(mission.m_sInstanceId) != 1
			|| task.m_sTaskId != "task_" + mission.m_sInstanceId
			|| task.m_sLinkedId != mission.m_sInstanceId
			|| task.m_sCategory != "mission"
			|| !PositionsMatch(task.m_vPosition, site.m_vTargetPosition))
			return "exact radio lifecycle mission task backlink is invalid";
		if (objective.m_eType != HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET
			|| objective.m_sTargetId != site.m_sTargetId
			|| objective.m_sTargetZoneId != site.m_sZoneId
			|| objective.m_sPhysicalEntityId != asset.m_sEntityId
			|| objective.m_sLinkedRuntimeEntityId != asset.m_sEntityId
			|| objective.m_iRequiredProgress != 1 || objective.m_iRequiredCount != 1
			|| objective.m_bAbstractFallback)
			return "exact radio lifecycle destroy objective binding is invalid";
		bool activeMission = mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE;
		if (objective.m_bCleanupComplete == activeMission)
			return "exact radio lifecycle objective cleanup state contradicts mission status";

		bool physicalDestruction = asset.m_bDestroyed && !asset.m_bAlive
			&& asset.m_bOutcomeApplied && asset.m_sOutcomeKind == "physically_destroyed"
			&& objective.m_bComplete && !objective.m_bFailed
			&& objective.m_bWorldDetected
			&& objective.m_iCurrentProgress == 1
			&& objective.m_iCurrentCount == 1
			&& mission.m_iRuntimeDestroyedCount == 1;
		bool noPhysicalDestruction = !asset.m_bDestroyed && asset.m_bAlive
			&& !asset.m_bOutcomeApplied && asset.m_sOutcomeKind.IsEmpty()
			&& !objective.m_bComplete
			&& !objective.m_bWorldDetected
			&& objective.m_iCurrentProgress == 0
			&& objective.m_iCurrentCount == 0
			&& mission.m_iRuntimeDestroyedCount == 0;
		bool activeNoDestructionClaim = noPhysicalDestruction
			&& !objective.m_bFailed;
		bool terminalNoDestructionClaim = noPhysicalDestruction
			&& objective.m_bFailed;

		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED)
		{
			if (!physicalDestruction || task.m_bActive || !task.m_bSucceeded || task.m_bFailed)
				return "successful exact radio lifecycle mission lacks physical target and objective evidence";
			return "";
		}
		if (mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED
			|| mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED)
		{
			if (!terminalNoDestructionClaim || task.m_bActive || task.m_bSucceeded || !task.m_bFailed)
				return "failed or expired exact radio lifecycle mission contradicts target destruction evidence";
			return "";
		}
		if (physicalDestruction)
		{
			if (!task.m_bActive || task.m_bSucceeded || task.m_bFailed)
				return "active exact radio lifecycle completion evidence conflicts with task state";
			return "";
		}
		if (!activeNoDestructionClaim
			|| !task.m_bActive || task.m_bSucceeded || task.m_bFailed)
			return "active exact radio lifecycle mission has contradictory target or objective evidence";
		return "";
	}

	protected int NormalizeTerminalObjectiveCompatibility()
	{
		int normalizedCount;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!mission)
				continue;
			HST_MissionObjectiveState objective = FindUniqueDestroyObjective(mission.m_sInstanceId);
			if (!CanNormalizeTerminalObjectiveCompatibility(mission, objective))
				continue;
			objective.m_bFailed = true;
			objective.m_bCleanupComplete = true;
			normalizedCount++;
		}
		return normalizedCount;
	}

	protected bool CanNormalizeTerminalObjectiveCompatibility(
		HST_ActiveMissionState mission,
		HST_MissionObjectiveState objective)
	{
		if (!mission || !objective
			|| mission.m_iRadioSiteContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
			|| !IsRadioLifecycleMissionId(mission.m_sMissionId))
			return false;
		if (mission.m_eStatus != HST_EMissionStatus.HST_MISSION_FAILED
			&& mission.m_eStatus != HST_EMissionStatus.HST_MISSION_EXPIRED)
			return false;
		if (mission.m_bRuntimeSpawned || !mission.m_bRuntimeCleanupComplete
			|| mission.m_iRuntimeDestroyedCount != 0)
			return false;
		if (mission.m_sInstanceId.IsEmpty()
			|| CountMissionsForInstance(mission.m_sInstanceId) != 1)
			return false;
		if (mission.m_sRadioSiteId.IsEmpty()
			|| mission.m_sRadioSiteTransitionRequestId.IsEmpty()
			|| CountMissionsForTransitionRequest(mission.m_sRadioSiteTransitionRequestId) != 1
			|| mission.m_iRadioSiteRevision < 1)
			return false;

		HST_RadioSiteState site = FindUniqueSite(mission.m_sRadioSiteId);
		if (!site || site.m_iContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
			|| site.m_sZoneId != mission.m_sTargetZoneId
			|| site.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED
			|| mission.m_iRadioSiteRevision > site.m_iRevision
			|| site.m_sActiveMissionInstanceId == mission.m_sInstanceId)
			return false;
		if (mission.m_sRadioSiteTransitionRequestId
				!= HST_RadioSiteLifecycleService.BuildAdmissionRequestId(
					site.m_sSiteId,
					mission.m_sInstanceId,
					mission.m_sMissionId))
			return false;

		string expectedRuntimePrimitive = "radio_site_destroy";
		if (mission.m_sMissionId == REBUILD_MISSION_ID)
			expectedRuntimePrimitive = "radio_site_rebuild";
		if (mission.m_sRuntimePrimitive != expectedRuntimePrimitive || mission.m_bRuntimeFallback)
			return false;
		if (!ValidateSite(site).IsEmpty() || !ValidateReceiptBacklinks(site).IsEmpty())
			return false;

		HST_MissionAssetState asset = FindUniqueMissionAsset(mission.m_sInstanceId);
		if (!asset || CountAssetsForMission(mission.m_sInstanceId) != 1
			|| !ValidateMissionAsset(asset, mission, site).IsEmpty())
			return false;
		if (asset.m_bDestroyed || !asset.m_bAlive
			|| asset.m_bOutcomeApplied || !asset.m_sOutcomeKind.IsEmpty())
			return false;

		HST_CampaignTaskState task = FindUniqueMissionTask(mission.m_sInstanceId);
		if (!task || CountTasksForMission(mission.m_sInstanceId) != 1
			|| task.m_sTaskId != "task_" + mission.m_sInstanceId
			|| task.m_sLinkedId != mission.m_sInstanceId
			|| task.m_sCategory != "mission"
			|| !PositionsMatch(task.m_vPosition, site.m_vTargetPosition))
			return false;
		if (task.m_bActive || task.m_bSucceeded || !task.m_bFailed)
			return false;

		if (CountObjectivesForMission(mission.m_sInstanceId) != 1
			|| objective.m_eType != HST_EMissionObjectiveType.HST_OBJECTIVE_DESTROY_TARGET
			|| objective.m_sTargetId != site.m_sTargetId
			|| objective.m_sTargetZoneId != site.m_sZoneId)
			return false;
		if (objective.m_sPhysicalEntityId != asset.m_sEntityId
			|| objective.m_sLinkedRuntimeEntityId != asset.m_sEntityId
			|| objective.m_iRequiredProgress != 1
			|| objective.m_iRequiredCount != 1)
			return false;
		if (objective.m_iCurrentProgress != 0 || objective.m_iCurrentCount != 0
			|| objective.m_bComplete || objective.m_bWorldDetected
			|| objective.m_bAbstractFallback)
			return false;
		return !objective.m_bFailed && !objective.m_bCleanupComplete;
	}

	protected void QuarantineSiteAggregate(HST_RadioSiteState site, string failure)
	{
		if (!site)
			return;
		string activeMissionInstanceId = site.m_sActiveMissionInstanceId;
		string lastTransitionMissionInstanceId = site.m_sLastTransitionMissionInstanceId;
		string destructionMissionInstanceId = site.m_sLastDestructionMissionInstanceId;
		string rebuildMissionInstanceId = site.m_sLastRebuildMissionInstanceId;
		QuarantineSite(site, failure);
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!mission)
				continue;
			if ((!site.m_sSiteId.IsEmpty() && mission.m_sRadioSiteId == site.m_sSiteId)
				|| (!activeMissionInstanceId.IsEmpty() && mission.m_sInstanceId == activeMissionInstanceId)
				|| (!lastTransitionMissionInstanceId.IsEmpty() && mission.m_sInstanceId == lastTransitionMissionInstanceId)
				|| (!destructionMissionInstanceId.IsEmpty() && mission.m_sInstanceId == destructionMissionInstanceId)
				|| (!rebuildMissionInstanceId.IsEmpty() && mission.m_sInstanceId == rebuildMissionInstanceId))
				QuarantineMission(mission, failure);
		}
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset)
				continue;
			if ((!site.m_sSiteId.IsEmpty() && asset.m_sRadioSiteId == site.m_sSiteId)
				|| (!activeMissionInstanceId.IsEmpty()
					&& asset.m_sMissionInstanceId == activeMissionInstanceId)
				|| (!lastTransitionMissionInstanceId.IsEmpty()
					&& asset.m_sMissionInstanceId == lastTransitionMissionInstanceId)
				|| (!destructionMissionInstanceId.IsEmpty()
					&& asset.m_sMissionInstanceId == destructionMissionInstanceId)
				|| (!rebuildMissionInstanceId.IsEmpty()
					&& asset.m_sMissionInstanceId == rebuildMissionInstanceId))
				QuarantineAsset(asset, failure);
		}
		foreach (HST_MissionRuntimeEntityState runtime : m_SaveData.m_aMissionRuntimeEntities)
		{
			if (runtime
				&& ((!activeMissionInstanceId.IsEmpty()
						&& runtime.m_sMissionInstanceId == activeMissionInstanceId)
					|| (!lastTransitionMissionInstanceId.IsEmpty()
						&& runtime.m_sMissionInstanceId == lastTransitionMissionInstanceId)
					|| (!destructionMissionInstanceId.IsEmpty()
						&& runtime.m_sMissionInstanceId == destructionMissionInstanceId)
					|| (!rebuildMissionInstanceId.IsEmpty()
						&& runtime.m_sMissionInstanceId == rebuildMissionInstanceId)))
				runtime.m_bSpawned = false;
		}
		if (!activeMissionInstanceId.IsEmpty())
		{
			foreach (HST_MissionObjectiveState objective : m_SaveData.m_aMissionObjectives)
			{
				if (!objective || objective.m_sMissionInstanceId != activeMissionInstanceId)
					continue;
				objective.m_bFailed = true;
				objective.m_bCleanupComplete = true;
			}
			foreach (HST_CampaignTaskState task : m_SaveData.m_aCampaignTasks)
			{
				if (!task || task.m_sLinkedId != activeMissionInstanceId)
					continue;
				task.m_bActive = false;
				task.m_bSucceeded = false;
				task.m_bFailed = true;
			}
		}
		ClearQuarantinedSiteLock(site);
	}

	protected void QuarantineMissionAggregate(
		HST_ActiveMissionState mission,
		HST_RadioSiteState site,
		string failure)
	{
		if (site)
			QuarantineSiteAggregate(site, failure);
		else
			QuarantineMission(mission, failure);
		if (!mission)
			return;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == mission.m_sInstanceId
				&& (asset.m_iRadioSiteContractVersion != 0 || asset.m_sRadioSiteId == mission.m_sRadioSiteId))
				QuarantineAsset(asset, failure);
		}
	}

	protected void QuarantineSite(HST_RadioSiteState site, string failure)
	{
		if (!site)
			return;
		bool newlyQuarantined = site.m_iContractVersion != HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION;
		site.m_iContractVersion = HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION;
		site.m_eLifecycleState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_QUARANTINED;
		site.m_sLastTransitionReason = "schema-59 radio-site quarantine: " + failure;
		if (newlyQuarantined)
			site.m_iRevision = Math.Max(1, site.m_iRevision) + 1;
		else
			site.m_iRevision = Math.Max(1, site.m_iRevision);
	}

	protected void NormalizeQuarantinedSite(HST_RadioSiteState site)
	{
		if (!site)
			return;
		site.m_iContractVersion = HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION;
		site.m_eLifecycleState = HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_QUARANTINED;
		site.m_iRevision = Math.Max(1, site.m_iRevision);
		if (site.m_sLastTransitionReason.IsEmpty())
			site.m_sLastTransitionReason = "schema-59 radio-site authority remains quarantined";
		ClearQuarantinedSiteLock(site);
	}

	protected void QuarantineMission(HST_ActiveMissionState mission, string failure)
	{
		if (!mission)
			return;
		bool failCurrentOutcome = mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			|| !mission.m_bRuntimeCleanupComplete;
		mission.m_iRadioSiteContractVersion = HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION;
		if (failCurrentOutcome)
			mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
		mission.m_sRuntimePhase = QUARANTINE_PHASE;
		mission.m_sRuntimeFailureReason = failure;
		CleanupQuarantinedMissionAggregate(mission, failCurrentOutcome);
	}

	protected void NormalizeQuarantinedMission(HST_ActiveMissionState mission)
	{
		if (!mission)
			return;
		bool failCurrentOutcome = mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			|| !mission.m_bRuntimeCleanupComplete;
		mission.m_iRadioSiteContractVersion = HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION;
		if (failCurrentOutcome)
			mission.m_eStatus = HST_EMissionStatus.HST_MISSION_FAILED;
		mission.m_sRuntimePhase = QUARANTINE_PHASE;
		if (mission.m_sRuntimeFailureReason.IsEmpty())
			mission.m_sRuntimeFailureReason = "schema-59 radio-site authority remains quarantined";
		CleanupQuarantinedMissionAggregate(mission, failCurrentOutcome);
	}

	protected void QuarantineAsset(HST_MissionAssetState asset, string failure)
	{
		if (!asset)
			return;
		asset.m_iRadioSiteContractVersion = HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION;
		asset.m_sLastInteraction = "schema59_radio_site_quarantined";
		asset.m_bSpawned = false;
		foreach (HST_MissionRuntimeEntityState runtime : m_SaveData.m_aMissionRuntimeEntities)
		{
			if (runtime && runtime.m_sRuntimeEntityId == asset.m_sEntityId)
				runtime.m_bSpawned = false;
		}
	}

	protected void ClearQuarantinedSiteLock(HST_RadioSiteState site)
	{
		if (!site)
			return;
		site.m_sActiveMissionInstanceId = "";
		site.m_sActiveMissionId = "";
		site.m_sActiveTransitionRequestId = "";
	}

	protected void CleanupQuarantinedMissionAggregate(
		HST_ActiveMissionState mission,
		bool failCurrentOutcome)
	{
		if (!mission)
			return;
		mission.m_bRuntimeSpawned = false;
		mission.m_bRuntimeCleanupComplete = true;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			asset.m_iRadioSiteContractVersion = HST_RadioSiteLifecycleService.QUARANTINED_CONTRACT_VERSION;
			asset.m_sLastInteraction = "schema59_radio_site_quarantined";
			asset.m_bSpawned = false;
		}
		foreach (HST_MissionRuntimeEntityState runtime : m_SaveData.m_aMissionRuntimeEntities)
		{
			if (runtime && runtime.m_sMissionInstanceId == mission.m_sInstanceId)
				runtime.m_bSpawned = false;
		}
		foreach (HST_MissionObjectiveState objective : m_SaveData.m_aMissionObjectives)
		{
			if (!failCurrentOutcome || !objective
				|| objective.m_sMissionInstanceId != mission.m_sInstanceId)
				continue;
			objective.m_bFailed = true;
			objective.m_bCleanupComplete = true;
		}
		foreach (HST_CampaignTaskState task : m_SaveData.m_aCampaignTasks)
		{
			if (!failCurrentOutcome || !task || task.m_sLinkedId != mission.m_sInstanceId)
				continue;
			task.m_bActive = false;
			task.m_bSucceeded = false;
			task.m_bFailed = true;
		}
	}

	protected HST_ZoneState FindUniqueRadioZone(string zoneId)
	{
		HST_ZoneState match;
		if (zoneId.IsEmpty())
			return null;
		foreach (HST_ZoneState zone : m_SaveData.m_aZones)
		{
			if (!zone || zone.m_sZoneId != zoneId)
				continue;
			if (match || zone.m_eType != HST_EZoneType.HST_ZONE_RADIO_TOWER)
				return null;
			match = zone;
		}
		return match;
	}

	protected HST_RadioSiteState FindUniqueSite(string siteId)
	{
		HST_RadioSiteState match;
		if (siteId.IsEmpty())
			return null;
		foreach (HST_RadioSiteState site : m_SaveData.m_aRadioSites)
		{
			if (!site || site.m_sSiteId != siteId)
				continue;
			if (match)
				return null;
			match = site;
		}
		return match;
	}

	protected HST_RadioSiteState FindUniqueSiteForZone(string zoneId)
	{
		HST_RadioSiteState match;
		if (zoneId.IsEmpty())
			return null;
		foreach (HST_RadioSiteState site : m_SaveData.m_aRadioSites)
		{
			if (!site || site.m_sZoneId != zoneId)
				continue;
			if (match)
				return null;
			match = site;
		}
		return match;
	}

	protected HST_ActiveMissionState FindUniqueMission(string instanceId)
	{
		HST_ActiveMissionState match;
		if (instanceId.IsEmpty())
			return null;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (!mission || mission.m_sInstanceId != instanceId)
				continue;
			if (match)
				return null;
			match = mission;
		}
		return match;
	}

	protected HST_MissionAssetState FindUniqueMissionAsset(string instanceId)
	{
		HST_MissionAssetState match;
		if (instanceId.IsEmpty())
			return null;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != instanceId)
				continue;
			if (match)
				return null;
			match = asset;
		}
		return match;
	}

	protected HST_MissionObjectiveState FindUniqueDestroyObjective(string instanceId)
	{
		HST_MissionObjectiveState match;
		if (instanceId.IsEmpty())
			return null;
		foreach (HST_MissionObjectiveState objective : m_SaveData.m_aMissionObjectives)
		{
			if (!objective || objective.m_sMissionInstanceId != instanceId)
				continue;
			if (match)
				return null;
			match = objective;
		}
		return match;
	}

	protected HST_MissionRuntimeEntityState FindUniqueMissionRuntimeEntity(string runtimeEntityId)
	{
		HST_MissionRuntimeEntityState match;
		if (runtimeEntityId.IsEmpty())
			return null;
		foreach (HST_MissionRuntimeEntityState runtime : m_SaveData.m_aMissionRuntimeEntities)
		{
			if (!runtime || runtime.m_sRuntimeEntityId != runtimeEntityId)
				continue;
			if (match)
				return null;
			match = runtime;
		}
		return match;
	}

	protected HST_CampaignTaskState FindUniqueMissionTask(string instanceId)
	{
		HST_CampaignTaskState match;
		if (instanceId.IsEmpty())
			return null;
		foreach (HST_CampaignTaskState task : m_SaveData.m_aCampaignTasks)
		{
			if (!task || task.m_sLinkedId != instanceId)
				continue;
			if (match)
				return null;
			match = task;
		}
		return match;
	}

	protected int CountObjectivesForMission(string instanceId)
	{
		int count;
		foreach (HST_MissionObjectiveState objective : m_SaveData.m_aMissionObjectives)
		{
			if (objective && objective.m_sMissionInstanceId == instanceId)
				count++;
		}
		return count;
	}

	protected int CountRuntimeEntitiesForMission(string instanceId)
	{
		int count;
		foreach (HST_MissionRuntimeEntityState runtime : m_SaveData.m_aMissionRuntimeEntities)
		{
			if (runtime && runtime.m_sMissionInstanceId == instanceId)
				count++;
		}
		return count;
	}

	protected int CountTasksForMission(string instanceId)
	{
		int count;
		foreach (HST_CampaignTaskState task : m_SaveData.m_aCampaignTasks)
		{
			if (task && task.m_sLinkedId == instanceId)
				count++;
		}
		return count;
	}

	protected int CountSitesForSiteId(string siteId)
	{
		int count;
		foreach (HST_RadioSiteState site : m_SaveData.m_aRadioSites)
		{
			if (site && site.m_sSiteId == siteId)
				count++;
		}
		return count;
	}

	protected int CountSitesForZone(string zoneId)
	{
		int count;
		foreach (HST_RadioSiteState site : m_SaveData.m_aRadioSites)
		{
			if (site && site.m_sZoneId == zoneId)
				count++;
		}
		return count;
	}

	protected int CountSitesForTarget(string targetId)
	{
		int count;
		foreach (HST_RadioSiteState site : m_SaveData.m_aRadioSites)
		{
			if (site && site.m_sTargetId == targetId)
				count++;
		}
		return count;
	}

	protected bool HasOtherResolvedSiteAtFrozenBinding(HST_RadioSiteState expectedSite)
	{
		if (!expectedSite || IsZeroVector(expectedSite.m_vAuthoredTargetPosition))
			return false;
		foreach (HST_RadioSiteState other : m_SaveData.m_aRadioSites)
		{
			if (!other || other == expectedSite
				|| other.m_eTargetOwnership == HST_ERadioSiteTargetOwnership.HST_RADIO_SITE_TARGET_UNRESOLVED
				|| IsZeroVector(other.m_vAuthoredTargetPosition))
				continue;
			if (PositionsMatch(
				expectedSite.m_vAuthoredTargetPosition,
				other.m_vAuthoredTargetPosition,
				FROZEN_BINDING_TOLERANCE_METERS))
				return true;
		}
		return false;
	}

	protected int CountMissionsForInstance(string instanceId)
	{
		int count;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (mission && mission.m_sInstanceId == instanceId)
				count++;
		}
		return count;
	}

	protected int CountMissionsForTransitionRequest(string requestId)
	{
		int count;
		if (requestId.IsEmpty())
			return count;
		foreach (HST_ActiveMissionState mission : m_SaveData.m_aActiveMissions)
		{
			if (mission && mission.m_sRadioSiteTransitionRequestId == requestId
				&& mission.m_iRadioSiteContractVersion != 0)
				count++;
		}
		return count;
	}

	protected int CountReceiptClaims(string receiptId)
	{
		int count;
		if (receiptId.IsEmpty())
			return count;
		foreach (HST_RadioSiteState site : m_SaveData.m_aRadioSites)
		{
			if (!site)
				continue;
			if (site.m_sLastDestructionReceiptId == receiptId)
				count++;
			if (site.m_sLastRebuildReceiptId == receiptId)
				count++;
		}
		return count;
	}

	protected int CountOtherSitesClaimingRequest(HST_RadioSiteState expectedSite, string requestId)
	{
		int count;
		if (!expectedSite || requestId.IsEmpty())
			return count;
		foreach (HST_RadioSiteState site : m_SaveData.m_aRadioSites)
		{
			if (!site || site == expectedSite)
				continue;
			if (site.m_sActiveTransitionRequestId == requestId
				|| site.m_sLastTransitionRequestId == requestId)
				count++;
		}
		return count;
	}

	protected int CountAssetsForMission(string instanceId)
	{
		int count;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (asset && asset.m_sMissionInstanceId == instanceId)
				count++;
		}
		return count;
	}

	protected int CountAssetsForAssetId(string assetId)
	{
		int count;
		foreach (HST_MissionAssetState asset : m_SaveData.m_aMissionAssets)
		{
			if (asset && asset.m_sAssetId == assetId)
				count++;
		}
		return count;
	}

	protected string ValidateReceiptBacklinks(HST_RadioSiteState site)
	{
		if (!site)
			return "radio-site receipt authority is unavailable";
		if (!site.m_sLastDestructionReceiptId.IsEmpty())
		{
			HST_ActiveMissionState destructionMission = FindUniqueMission(site.m_sLastDestructionMissionInstanceId);
			if (!destructionMission
				|| site.m_sLastDestructionReceiptId
					!= HST_RadioSiteLifecycleService.BuildDestructionReceiptId(site.m_sSiteId, destructionMission.m_sInstanceId)
				|| destructionMission.m_iRadioSiteContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
				|| destructionMission.m_sRadioSiteId != site.m_sSiteId
				|| destructionMission.m_sMissionId != DESTROY_MISSION_ID
				|| destructionMission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED
				|| destructionMission.m_iRadioSiteRevision > site.m_iRevision)
				return "radio-site destruction receipt backlink is not reciprocal";
		}
		if (!site.m_sLastRebuildReceiptId.IsEmpty())
		{
			HST_ActiveMissionState rebuildMission = FindUniqueMission(site.m_sLastRebuildMissionInstanceId);
			if (!rebuildMission
				|| site.m_sLastRebuildReceiptId
					!= HST_RadioSiteLifecycleService.BuildRebuildReceiptId(site.m_sSiteId, rebuildMission.m_sInstanceId)
				|| rebuildMission.m_iRadioSiteContractVersion != HST_RadioSiteLifecycleService.EXACT_CONTRACT_VERSION
				|| rebuildMission.m_sRadioSiteId != site.m_sSiteId
				|| rebuildMission.m_sMissionId != REBUILD_MISSION_ID
				|| (rebuildMission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED
					&& rebuildMission.m_eStatus != HST_EMissionStatus.HST_MISSION_FAILED
					&& rebuildMission.m_eStatus != HST_EMissionStatus.HST_MISSION_EXPIRED)
				|| rebuildMission.m_iRadioSiteRevision > site.m_iRevision)
				return "radio-site rebuild receipt backlink is not reciprocal";
			if ((rebuildMission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED
					&& site.m_eLifecycleState != HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED)
				|| (rebuildMission.m_eStatus != HST_EMissionStatus.HST_MISSION_SUCCEEDED
					&& site.m_eLifecycleState != HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE))
				return "radio-site rebuild receipt outcome conflicts with the durable lifecycle";
		}
		return "";
	}

	protected string ValidateLatestMissionOutcome(
		HST_RadioSiteState site,
		HST_ActiveMissionState mission)
	{
		if (!site || !mission)
			return "radio-site latest mission outcome authority is unavailable";
		if (site.m_sLastTransitionKind == TRANSITION_DESTROY_ADMISSION)
		{
			if (mission.m_sMissionId == DESTROY_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
				&& IsTransitionShape(site,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE))
				return "";
			return "destroy-admission typed transition conflicts with active mission or lifecycle";
		}
		if (site.m_sLastTransitionKind == TRANSITION_REBUILD_ADMISSION)
		{
			if (mission.m_sMissionId == REBUILD_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
				&& IsTransitionShape(site,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING))
				return "";
			return "rebuild-admission typed transition conflicts with active mission or lifecycle";
		}

		if (site.m_sLastTransitionKind == TRANSITION_DESTROY_SUCCESS)
		{
			if (mission.m_sMissionId == DESTROY_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED
				&& IsTransitionShape(site,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED))
				return "";
			return "destroy-success typed transition conflicts with mission outcome or lifecycle";
		}
		if (site.m_sLastTransitionKind == TRANSITION_DESTROY_FAILURE
			|| site.m_sLastTransitionKind == TRANSITION_CAMPAIGN_STOP_DESTROY)
		{
			if (mission.m_sMissionId == DESTROY_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED
				&& IsTransitionShape(site,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE))
				return "";
			return "destroy-failure typed transition conflicts with mission outcome or lifecycle";
		}
		if (site.m_sLastTransitionKind == TRANSITION_DESTROY_EXPIRY)
		{
			if (mission.m_sMissionId == DESTROY_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED
				&& IsTransitionShape(site,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE))
				return "";
			return "destroy-expiry typed transition conflicts with mission outcome or lifecycle";
		}
		if (site.m_sLastTransitionKind == TRANSITION_REBUILD_SUCCESS)
		{
			if (mission.m_sMissionId == REBUILD_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_SUCCEEDED
				&& IsTransitionShape(site,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED))
				return "";
			return "rebuild-success typed transition conflicts with mission outcome or lifecycle";
		}
		if (site.m_sLastTransitionKind == TRANSITION_REBUILD_FAILURE
			|| site.m_sLastTransitionKind == TRANSITION_CAMPAIGN_STOP_REBUILD)
		{
			if (mission.m_sMissionId == REBUILD_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_FAILED
				&& IsTransitionShape(site,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE))
				return "";
			return "rebuild-failure typed transition conflicts with mission outcome or lifecycle";
		}
		if (site.m_sLastTransitionKind == TRANSITION_REBUILD_EXPIRY)
		{
			if (mission.m_sMissionId == REBUILD_MISSION_ID
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_EXPIRED
				&& IsTransitionShape(site,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING,
					HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE))
				return "";
			return "rebuild-expiry typed transition conflicts with mission outcome or lifecycle";
		}
		return "radio-site latest typed transition kind is unsupported";
	}

	protected string BuildExpectedTransitionRequestId(
		HST_RadioSiteState site,
		HST_ActiveMissionState mission)
	{
		if (!site || !mission)
			return "";
		if (site.m_sLastTransitionKind == TRANSITION_DESTROY_ADMISSION
			|| site.m_sLastTransitionKind == TRANSITION_REBUILD_ADMISSION)
		{
			return HST_RadioSiteLifecycleService.BuildAdmissionRequestId(
				site.m_sSiteId,
				mission.m_sInstanceId,
				mission.m_sMissionId);
		}
		return HST_RadioSiteLifecycleService.BuildOutcomeRequestId(
			site.m_sSiteId,
			mission.m_sInstanceId,
			mission.m_sMissionId,
			site.m_sLastTransitionKind);
	}

	protected bool IsTransitionShape(
		HST_RadioSiteState site,
		HST_ERadioSiteLifecycleState fromState,
		HST_ERadioSiteLifecycleState toState)
	{
		return site
			&& site.m_eLastTransitionFromState == fromState
			&& site.m_eLastTransitionToState == toState
			&& site.m_eLifecycleState == toState;
	}

	protected static bool IsRadioLifecycleMissionId(string missionId)
	{
		return missionId == DESTROY_MISSION_ID || missionId == REBUILD_MISSION_ID;
	}

	protected static bool IsOperationalLifecycle(HST_ERadioSiteLifecycleState lifecycle)
	{
		return lifecycle == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_ONLINE
			|| lifecycle == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_DESTROYED
			|| lifecycle == HST_ERadioSiteLifecycleState.HST_RADIO_SITE_LIFECYCLE_REBUILDING;
	}

	protected bool PositionsMatch(vector first, vector second, float toleranceMeters = 0.5)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz <= toleranceMeters * toleranceMeters;
	}

	protected float DistanceSq2D(vector first, vector second)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz;
	}

	protected bool IsZeroVector(vector value)
	{
		return value[0] == 0 && value[1] == 0 && value[2] == 0;
	}

	protected bool HasEvent(string eventId)
	{
		foreach (HST_CampaignEventState eventState : m_SaveData.m_aCampaignEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return true;
		}
		return false;
	}

	protected void RecordTerminalObjectiveCompatibility(int normalizedCount)
	{
		if (normalizedCount <= 0 || HasEvent(TERMINAL_OBJECTIVE_COMPATIBILITY_EVENT_ID))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = TERMINAL_OBJECTIVE_COMPATIBILITY_EVENT_ID;
		eventState.m_sCategory = "normalization";
		eventState.m_sAggregateType = "radio_site";
		eventState.m_sAggregateId = "schema59_terminal_objective";
		eventState.m_sTransition = "terminal_objective_cleanup_projection_restored";
		eventState.m_sReason = string.Format(
			"projected cleanup-complete and failed objective metadata for %1 already-terminal exact radio missions only after mission, task, asset, runtime, site, transition, and receipt evidence agreed; no physical destruction, lifecycle outcome, receipt, reward, or task result was inferred",
			normalizedCount);
		eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(eventState);
	}

	protected void RecordNormalizationConflict(int conflictCount)
	{
		if (conflictCount <= 0 || HasEvent(CONFLICT_EVENT_ID))
			return;
		HST_CampaignEventState eventState = new HST_CampaignEventState();
		eventState.m_sEventId = CONFLICT_EVENT_ID;
		eventState.m_sCategory = "normalization";
		eventState.m_sAggregateType = "radio_site";
		eventState.m_sAggregateId = "schema59";
		eventState.m_sTransition = "ambiguous_radio_site_authority_quarantined";
		eventState.m_sReason = string.Format(
			"quarantined %1 missing, duplicate, malformed, stale, or non-reciprocal radio-site authority claims without inventing a binding, destruction, rebuild, mission outcome, or receipt",
			conflictCount);
		eventState.m_iCreatedAtSecond = Math.Max(0, m_SaveData.m_iElapsedSeconds);
		m_SaveData.m_aCampaignEvents.Insert(eventState);
	}
}
