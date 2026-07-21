#ifdef ENABLE_DIAG
// Fail-closed portable authority for the disposable three-process
// administrative campaign-reset persistence proof. The coordinator owns all
// engine save/load and production reset work; this service owns only bounded
// artifacts, synthetic sentinels, and exact cross-process evidence.
class HST_AdminCampaignResetPersistenceProofService
{
	static const string OWNER_MAGIC
		= "partisan_admin_campaign_reset_persistence_owner_v1";
	static const string GUARD_MAGIC
		= "partisan_admin_campaign_reset_persistence_guard_v1";
	static const string CARRIER_MAGIC
		= "partisan_admin_campaign_reset_persistence_carrier_v1";
	static const string RESULT_MAGIC
		= "partisan_admin_campaign_reset_persistence_result_v1";
	static const string OWNER_PURPOSE
		= "admin_campaign_reset_persistence";
	static const int AUTHORITY_VERSION = 1;
	static const int EXPECTED_STAGE_COUNT = 3;

	static const string STAGE_PREPARE_OLD_CHECKPOINT
		= "prepare_old_checkpoint";
	static const string STAGE_RESET_COMMIT = "reset_commit";
	static const string STAGE_STALE_NATIVE_NO_SAVE_VERIFY
		= "stale_native_no_save_verify";

	static const string SOURCE_NEW_CAMPAIGN = "new_campaign";
	static const string SOURCE_NATIVE = "native";
	static const string SOURCE_PROFILE_FALLBACK = "profile_fallback";
	static const string SAVE_TYPE_MANUAL = "manual";
	static const string SAVE_TYPE_NONE = "none";
	static const string MANUAL_SAVE_NAME
		= "Partisan manual checkpoint";

	static const string CANONICAL_WORLD
		= "Worlds/HST_Everon/HST_Everon.ent";
	static const string CANONICAL_WORLD_LOWER
		= "worlds/hst_everon/hst_everon.ent";
	static const string OLD_SENTINEL_TASK_ID
		= "hst_admin_campaign_reset_old_sentinel";
	static const string OLD_SENTINEL_TITLE
		= "Administrative Reset Old Campaign Sentinel";
	static const string OLD_SENTINEL_CATEGORY
		= "admin_campaign_reset_persistence_proof";
	static const string PRESERVED_PLAYER_ID_PREFIX
		= "admin_reset_proof_";
	static const string PRESERVED_PLAYER_DISPLAY_NAME
		= "Campaign Reset Proof";
	static const int MAX_RUN_ID_CHARACTERS = 64;
	static const int NONCE_CHARACTERS = 32;

	protected static string SanitizeRunId(string runId)
	{
		if (runId.IsEmpty() || runId.Length() > MAX_RUN_ID_CHARACTERS)
			return "";
		for (int index = 0; index < runId.Length(); index++)
		{
			string character = runId.Substring(index, 1);
			if (!"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"
				.Contains(character))
				return "";
		}
		return runId;
	}

	static bool ValidateRunId(string runId)
	{
		return !runId.IsEmpty() && SanitizeRunId(runId) == runId;
	}

	static bool ValidateNonce(string nonce)
	{
		if (nonce.Length() != NONCE_CHARACTERS)
			return false;
		for (int index = 0; index < nonce.Length(); index++)
		{
			string character = nonce.Substring(index, 1);
			if (!"0123456789abcdef".Contains(character))
				return false;
		}
		return true;
	}

	static bool ValidateStage(string stage)
	{
		return ResolveStageOrdinal(stage) >= 0;
	}

	static int ResolveStageOrdinal(string stage)
	{
		if (stage == STAGE_PREPARE_OLD_CHECKPOINT)
			return 0;
		if (stage == STAGE_RESET_COMMIT)
			return 1;
		if (stage == STAGE_STALE_NATIVE_NO_SAVE_VERIFY)
			return 2;
		return -1;
	}

	static string ResolveExpectedSource(string stage)
	{
		if (stage == STAGE_PREPARE_OLD_CHECKPOINT)
			return SOURCE_NEW_CAMPAIGN;
		if (stage == STAGE_RESET_COMMIT)
			return SOURCE_NATIVE;
		if (stage == STAGE_STALE_NATIVE_NO_SAVE_VERIFY)
			return SOURCE_PROFILE_FALLBACK;
		return "";
	}

	static int ResolveExpectedSourceJournalGeneration(string stage)
	{
		if (stage == STAGE_PREPARE_OLD_CHECKPOINT)
			return -1;
		if (stage == STAGE_RESET_COMMIT)
			return 1;
		if (stage == STAGE_STALE_NATIVE_NO_SAVE_VERIFY)
			return 3;
		return -2;
	}

	static bool StageCreatesSavePoints(string stage)
	{
		return stage == STAGE_PREPARE_OLD_CHECKPOINT
			|| stage == STAGE_RESET_COMMIT;
	}

	static bool StageIsNoSave(string stage)
	{
		return stage == STAGE_STALE_NATIVE_NO_SAVE_VERIFY;
	}

	static string ResolveExpectedSaveType(string stage)
	{
		if (StageCreatesSavePoints(stage))
			return SAVE_TYPE_MANUAL;
		if (StageIsNoSave(stage))
			return SAVE_TYPE_NONE;
		return "";
	}

	static string ResolveExpectedSaveName(string stage)
	{
		if (StageCreatesSavePoints(stage))
			return MANUAL_SAVE_NAME;
		return "";
	}

	static string NormalizeWorldIdentity(string world)
	{
		if (world.IsEmpty())
			return "";
		string normalized = world.Trim();
		normalized.ToLower();
		int pathStart = normalized.IndexOf(CANONICAL_WORLD_LOWER);
		if (pathStart < 0
			|| pathStart + CANONICAL_WORLD_LOWER.Length()
				!= normalized.Length())
			return "";
		if (pathStart > 0)
		{
			string boundary = normalized.Substring(pathStart - 1, 1);
			if (boundary != ":" && boundary != "/" && boundary != "}")
				return "";
		}
		return CANONICAL_WORLD;
	}

	protected static bool ValidateWorldIdentity(
		string artifactWorld,
		string expectedWorld)
	{
		return artifactWorld == CANONICAL_WORLD
			&& NormalizeWorldIdentity(expectedWorld) == CANONICAL_WORLD;
	}

	protected static string BuildOwnerPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_AdminCampaignResetPersistenceProof_" + safeRunId
			+ ".owner.json";
	}

	protected static string BuildGuardPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_AdminCampaignResetPersistenceProof_" + safeRunId
			+ ".guard.json";
	}

	protected static string BuildCarrierPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_AdminCampaignResetPersistenceProof_" + safeRunId
			+ ".carrier.json";
	}

	protected static string BuildResultPath(string runId, string stage)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty() || !ValidateStage(stage))
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_AdminCampaignResetPersistenceProof_" + safeRunId
			+ "." + stage + ".json";
	}

	protected static bool ValidateOwner(
		HST_AdminCampaignResetPersistenceProofOwner owner,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedWorld,
		out string evidence)
	{
		evidence = "admin reset persistence profile owner rejected";
		if (!owner || !ValidateNonce(expectedSessionNonce)
			|| !ValidateRunId(expectedRunId) || expectedWorld.IsEmpty())
			return false;
		if (owner.m_sMagic != OWNER_MAGIC
			|| owner.m_iVersion != AUTHORITY_VERSION
			|| owner.m_sPurpose != OWNER_PURPOSE
			|| owner.m_sSessionNonce != expectedSessionNonce
			|| owner.m_sRunId != expectedRunId
			|| SanitizeRunId(owner.m_sRunId) != owner.m_sRunId
			|| !ValidateNonce(owner.m_sPayloadNonce)
			|| owner.m_iExpectedStageCount != EXPECTED_STAGE_COUNT
			|| !owner.m_bDisposableProfile)
			return false;
		if (owner.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| owner.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| owner.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL
			|| owner.m_iCampaignSchemaVersion
				!= HST_CampaignState.SCHEMA_VERSION
			|| owner.m_iSettingsSchemaVersion
				!= HST_RuntimeSettings.SCHEMA_VERSION
			|| !ValidateWorldIdentity(owner.m_sWorld, expectedWorld))
			return false;
		evidence = "admin reset persistence profile owner exact";
		return true;
	}

	static bool LoadAndValidateOwner(
		string sessionNonce,
		string runId,
		string expectedWorld,
		out HST_AdminCampaignResetPersistenceProofOwner owner,
		out string evidence)
	{
		owner = null;
		evidence = "admin reset persistence profile owner unavailable";
		string path = BuildOwnerPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		owner = new HST_AdminCampaignResetPersistenceProofOwner();
		if (!context.ReadValue("", owner))
		{
			owner = null;
			return false;
		}
		return ValidateOwner(
			owner,
			sessionNonce,
			runId,
			expectedWorld,
			evidence);
	}

	protected static bool ValidateGuard(
		HST_AdminCampaignResetPersistenceProofGuard guard,
		string expectedSessionNonce,
		string expectedStageNonce,
		string expectedRunId,
		string expectedPayloadNonce,
		string expectedStage,
		string expectedWorld,
		out string evidence)
	{
		evidence = "admin reset persistence one-use stage lease rejected";
		int expectedOrdinal = ResolveStageOrdinal(expectedStage);
		if (!guard || !ValidateNonce(expectedSessionNonce)
			|| !ValidateNonce(expectedStageNonce)
			|| !ValidateRunId(expectedRunId)
			|| !ValidateNonce(expectedPayloadNonce)
			|| expectedOrdinal < 0 || expectedWorld.IsEmpty())
			return false;
		if (guard.m_sMagic != GUARD_MAGIC
			|| guard.m_iVersion != AUTHORITY_VERSION
			|| guard.m_sSessionNonce != expectedSessionNonce
			|| guard.m_sStageNonce != expectedStageNonce
			|| guard.m_sRunId != expectedRunId
			|| SanitizeRunId(guard.m_sRunId) != guard.m_sRunId
			|| guard.m_sPayloadNonce != expectedPayloadNonce
			|| guard.m_sRequestedStage != expectedStage
			|| guard.m_iStageOrdinal != expectedOrdinal)
			return false;
		if (guard.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| guard.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| guard.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL
			|| guard.m_iCampaignSchemaVersion
				!= HST_CampaignState.SCHEMA_VERSION
			|| guard.m_iSettingsSchemaVersion
				!= HST_RuntimeSettings.SCHEMA_VERSION
			|| !ValidateWorldIdentity(guard.m_sWorld, expectedWorld))
			return false;
		if (guard.m_sExpectedSource != ResolveExpectedSource(expectedStage)
			|| guard.m_iExpectedJournalGeneration
				!= ResolveExpectedSourceJournalGeneration(expectedStage)
			|| guard.m_bAllowCanonicalCampaignOverwrite
				!= StageCreatesSavePoints(expectedStage)
			|| guard.m_bNoSaveStage != StageIsNoSave(expectedStage))
			return false;
		if (expectedStage == STAGE_PREPARE_OLD_CHECKPOINT)
		{
			if (!guard.m_sExpectedLoadSavePointId.IsEmpty())
				return false;
		}
		else if (!UUID.IsUUID(guard.m_sExpectedLoadSavePointId))
			return false;
		evidence = "admin reset persistence one-use stage lease exact";
		return true;
	}

	static bool LoadValidateAndConsumeGuard(
		string sessionNonce,
		string stageNonce,
		string runId,
		string payloadNonce,
		string stage,
		string expectedWorld,
		out HST_AdminCampaignResetPersistenceProofGuard guard,
		out string evidence)
	{
		guard = null;
		evidence = "admin reset persistence one-use stage lease unavailable";
		string path = BuildGuardPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		guard = new HST_AdminCampaignResetPersistenceProofGuard();
		if (!context.ReadValue("", guard))
		{
			guard = null;
			return false;
		}
		if (!ValidateGuard(
			guard,
			sessionNonce,
			stageNonce,
			runId,
			payloadNonce,
			stage,
			expectedWorld,
			evidence))
			return false;
		if (!FileIO.FileExists(path))
		{
			evidence
				= "admin reset persistence stage lease disappeared before consumption";
			return false;
		}
		FileIO.DeleteFile(path);
		if (FileIO.FileExists(path))
		{
			evidence
				= "admin reset persistence stage lease could not be consumed";
			return false;
		}
		evidence = "admin reset persistence one-use stage lease consumed";
		return true;
	}

	static string BuildPreservedPlayerIdentityId(string payloadNonce)
	{
		if (!ValidateNonce(payloadNonce))
			return "";
		return PRESERVED_PLAYER_ID_PREFIX + payloadNonce;
	}

	protected static string BuildOldSentinelDescription(string payloadNonce)
	{
		return "payload " + payloadNonce + " | must not survive reset";
	}

	static string BuildOldSentinelFingerprint(string payloadNonce)
	{
		if (!ValidateNonce(payloadNonce))
			return "";
		string canonical = string.Format(
			"%1|%2|%3|%4|%5|0|0|0|false|false|false",
			OLD_SENTINEL_TASK_ID,
			payloadNonce,
			OLD_SENTINEL_TITLE,
			BuildOldSentinelDescription(payloadNonce),
			OLD_SENTINEL_CATEGORY);
		return HST_CampaignPersistentState.BuildPayloadFingerprint(canonical);
	}

	static string BuildPlayerFingerprint(HST_PlayerState player)
	{
		if (!player)
			return "";
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", player))
			return "";
		string payload = context.SaveToString();
		return HST_CampaignPersistentState.BuildPayloadFingerprint(payload);
	}

	protected static HST_CampaignTaskState FindUniqueOldSentinel(
		HST_CampaignState state,
		out int matchCount)
	{
		matchCount = 0;
		HST_CampaignTaskState result;
		if (!state || !state.m_aCampaignTasks)
			return null;
		foreach (HST_CampaignTaskState task : state.m_aCampaignTasks)
		{
			if (!task || task.m_sTaskId != OLD_SENTINEL_TASK_ID)
				continue;
			matchCount++;
			result = task;
		}
		if (matchCount != 1)
			return null;
		return result;
	}

	static bool ValidateOldSentinel(
		HST_CampaignState state,
		string payloadNonce,
		out int matchCount,
		out string fingerprint,
		out string evidence)
	{
		matchCount = 0;
		fingerprint = "";
		evidence = "admin reset old campaign sentinel rejected";
		if (!state || !ValidateNonce(payloadNonce))
			return false;
		HST_CampaignTaskState task = FindUniqueOldSentinel(
			state,
			matchCount);
		if (!task)
		{
			evidence = string.Format(
				"admin reset old sentinel cardinality rejected | count %1",
				matchCount);
			return false;
		}
		bool zeroPosition = Math.AbsFloat(task.m_vPosition[0]) < 0.001
			&& Math.AbsFloat(task.m_vPosition[1]) < 0.001
			&& Math.AbsFloat(task.m_vPosition[2]) < 0.001;
		if (task.m_sLinkedId != payloadNonce
			|| task.m_sTitle != OLD_SENTINEL_TITLE
			|| task.m_sDescription
				!= BuildOldSentinelDescription(payloadNonce)
			|| task.m_sCategory != OLD_SENTINEL_CATEGORY
			|| !zeroPosition || task.m_bActive
			|| task.m_bSucceeded || task.m_bFailed)
			return false;
		fingerprint = BuildOldSentinelFingerprint(payloadNonce);
		if (fingerprint.IsEmpty())
			return false;
		evidence = "admin reset old campaign sentinel exact | "
			+ fingerprint;
		return true;
	}

	static bool ValidateOldSentinelAbsent(
		HST_CampaignState state,
		out int matchCount,
		out string evidence)
	{
		matchCount = 0;
		evidence = "admin reset old campaign sentinel absence rejected";
		if (!state || !state.m_aCampaignTasks)
			return false;
		FindUniqueOldSentinel(state, matchCount);
		if (matchCount != 0)
			return false;
		evidence = "admin reset old campaign sentinel absent";
		return true;
	}

	static bool InstallOldSentinel(
		HST_CampaignState state,
		string payloadNonce,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "";
		evidence = "admin reset old campaign sentinel installation rejected";
		if (!state || !state.m_aCampaignTasks
			|| !ValidateNonce(payloadNonce))
			return false;
		int existingCount;
		FindUniqueOldSentinel(state, existingCount);
		if (existingCount != 0)
			return false;
		HST_CampaignTaskState task = new HST_CampaignTaskState();
		task.m_sTaskId = OLD_SENTINEL_TASK_ID;
		task.m_sLinkedId = payloadNonce;
		task.m_sTitle = OLD_SENTINEL_TITLE;
		task.m_sDescription = BuildOldSentinelDescription(payloadNonce);
		task.m_sCategory = OLD_SENTINEL_CATEGORY;
		task.m_vPosition = vector.Zero;
		task.m_bActive = false;
		task.m_bSucceeded = false;
		task.m_bFailed = false;
		state.m_aCampaignTasks.Insert(task);
		int matchCount;
		if (!ValidateOldSentinel(
			state,
			payloadNonce,
			matchCount,
			fingerprint,
			evidence))
			return false;
		evidence = "admin reset old campaign sentinel installed | "
			+ fingerprint;
		return true;
	}

	protected static HST_PlayerState FindUniquePreservedPlayer(
		HST_CampaignState state,
		string identityId,
		out int matchCount)
	{
		matchCount = 0;
		HST_PlayerState result;
		if (!state || !state.m_aPlayers || identityId.IsEmpty())
			return null;
		foreach (HST_PlayerState player : state.m_aPlayers)
		{
			if (!player || player.m_sIdentityId != identityId)
				continue;
			matchCount++;
			result = player;
		}
		if (matchCount != 1)
			return null;
		return result;
	}

	static bool ValidatePreservedIdentity(
		HST_CampaignState state,
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out string playerFingerprint,
		out string evidence)
	{
		playerFingerprint = "";
		evidence = "admin reset preserved identity rejected";
		if (!state || !carrier || !state.m_aPlayers
			|| state.m_aPlayers.Count() != 1
			|| carrier.m_sPreservedPlayerIdentityId.IsEmpty()
			|| carrier.m_sPreservedCommanderIdentityId.IsEmpty()
			|| carrier.m_sPreservedPlayerIdentityId
				!= BuildPreservedPlayerIdentityId(carrier.m_sPayloadNonce))
			return false;
		int matchCount;
		HST_PlayerState player = FindUniquePreservedPlayer(
			state,
			carrier.m_sPreservedPlayerIdentityId,
			matchCount);
		if (!player || matchCount != 1)
			return false;
		playerFingerprint = BuildPlayerFingerprint(player);
		if (playerFingerprint.IsEmpty()
			|| playerFingerprint != carrier.m_sPreservedPlayerFingerprint
			|| player.m_sDisplayName != PRESERVED_PLAYER_DISPLAY_NAME
			|| player.m_sFactionKey != "FIA"
			|| !player.m_bMember || !player.m_bAdmin || player.m_bGuest
			|| player.m_iMoney != 321 || player.m_iRank != 4
			|| player.m_iLastSeenPlayerId != -1
			|| player.m_bHasSpawnRecord || player.m_iSpawnCount != 0
			|| !player.m_sLastSpawnPrefab.IsEmpty()
			|| state.m_sCommanderIdentityId
				!= carrier.m_sPreservedCommanderIdentityId
			|| state.m_sCommanderIdentityId
				!= carrier.m_sPreservedPlayerIdentityId)
			return false;
		evidence = "admin reset preserved player/admin/commander identity exact | "
			+ playerFingerprint;
		return true;
	}

	static bool InstallPreservedIdentity(
		HST_CampaignState state,
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out string evidence)
	{
		evidence = "admin reset synthetic preserved identity installation rejected";
		if (!state || !carrier || !state.m_aPlayers
			|| state.m_aPlayers.Count() != 0
			|| !state.m_sCommanderIdentityId.IsEmpty())
			return false;
		string identityId = BuildPreservedPlayerIdentityId(
			carrier.m_sPayloadNonce);
		if (identityId.IsEmpty())
			return false;
		HST_PlayerState player = new HST_PlayerState();
		player.m_sIdentityId = identityId;
		player.m_sDisplayName = PRESERVED_PLAYER_DISPLAY_NAME;
		player.m_sFactionKey = "FIA";
		player.m_bMember = true;
		player.m_bAdmin = true;
		player.m_bGuest = false;
		player.m_iMoney = 321;
		player.m_iRank = 4;
		player.m_iLastSeenPlayerId = -1;
		player.m_bHasSpawnRecord = false;
		player.m_iSpawnCount = 0;
		player.m_sLastSpawnPrefab = "";
		player.m_vLastSpawnPosition = vector.Zero;
		state.m_aPlayers.Insert(player);
		state.m_sCommanderIdentityId = identityId;
		carrier.m_sPreservedPlayerIdentityId = identityId;
		carrier.m_sPreservedCommanderIdentityId = identityId;
		carrier.m_sPreservedPlayerFingerprint
			= BuildPlayerFingerprint(player);
		string observedFingerprint;
		if (!ValidatePreservedIdentity(
			state,
			carrier,
			observedFingerprint,
			evidence))
			return false;
		evidence = "admin reset synthetic preserved identity installed | "
			+ observedFingerprint;
		return true;
	}

	static bool CreateInitialCarrier(
		HST_AdminCampaignResetPersistenceProofOwner owner,
		string expectedWorld,
		out HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out string evidence)
	{
		carrier = null;
		evidence = "admin reset persistence initial carrier rejected";
		string ownerEvidence;
		if (!owner || !ValidateOwner(
			owner,
			owner.m_sSessionNonce,
			owner.m_sRunId,
			expectedWorld,
			ownerEvidence))
		{
			evidence += " | " + ownerEvidence;
			return false;
		}
		carrier = new HST_AdminCampaignResetPersistenceProofCarrier();
		carrier.m_sMagic = CARRIER_MAGIC;
		carrier.m_iVersion = AUTHORITY_VERSION;
		carrier.m_sSessionNonce = owner.m_sSessionNonce;
		carrier.m_sRunId = owner.m_sRunId;
		carrier.m_sPayloadNonce = owner.m_sPayloadNonce;
		carrier.m_sBuildSha = HST_BuildInfo.BUILD_SHA;
		carrier.m_sBuildUtc = HST_BuildInfo.BUILD_UTC;
		carrier.m_sBuildLabel = HST_BuildInfo.BUILD_LABEL;
		carrier.m_iCampaignSchemaVersion
			= HST_CampaignState.SCHEMA_VERSION;
		carrier.m_iSettingsSchemaVersion
			= HST_RuntimeSettings.SCHEMA_VERSION;
		carrier.m_sWorld = CANONICAL_WORLD;
		carrier.m_iCompletedStageOrdinal = -1;
		if (!ValidateInitialCarrier(
			carrier,
			owner.m_sSessionNonce,
			owner.m_sRunId,
			owner.m_sPayloadNonce,
			expectedWorld,
			evidence))
		{
			carrier = null;
			return false;
		}
		evidence = "admin reset persistence initial carrier created";
		return true;
	}

	protected static bool ValidateCarrierIdentity(
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedPayloadNonce,
		string expectedWorld,
		out string evidence)
	{
		evidence = "admin reset persistence carrier identity rejected";
		if (!carrier || !ValidateNonce(expectedSessionNonce)
			|| !ValidateRunId(expectedRunId)
			|| !ValidateNonce(expectedPayloadNonce)
			|| expectedWorld.IsEmpty())
			return false;
		if (carrier.m_sMagic != CARRIER_MAGIC
			|| carrier.m_iVersion != AUTHORITY_VERSION
			|| carrier.m_sSessionNonce != expectedSessionNonce
			|| carrier.m_sRunId != expectedRunId
			|| SanitizeRunId(carrier.m_sRunId) != carrier.m_sRunId
			|| carrier.m_sPayloadNonce != expectedPayloadNonce)
			return false;
		if (carrier.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| carrier.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| carrier.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL
			|| carrier.m_iCampaignSchemaVersion
				!= HST_CampaignState.SCHEMA_VERSION
			|| carrier.m_iSettingsSchemaVersion
				!= HST_RuntimeSettings.SCHEMA_VERSION
			|| !ValidateWorldIdentity(carrier.m_sWorld, expectedWorld))
			return false;
		evidence = "admin reset persistence carrier identity exact";
		return true;
	}

	protected static bool ValidateInitialCarrier(
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedPayloadNonce,
		string expectedWorld,
		out string evidence)
	{
		if (!ValidateCarrierIdentity(
			carrier,
			expectedSessionNonce,
			expectedRunId,
			expectedPayloadNonce,
			expectedWorld,
			evidence)
			|| carrier.m_iCompletedStageOrdinal != -1)
			return false;
		if (!carrier.m_sOldSentinelTaskId.IsEmpty()
			|| !carrier.m_sOldSentinelFingerprint.IsEmpty()
			|| !carrier.m_sPreservedPlayerIdentityId.IsEmpty()
			|| !carrier.m_sPreservedPlayerFingerprint.IsEmpty()
			|| !carrier.m_sPreservedCommanderIdentityId.IsEmpty()
			|| carrier.m_iOldMarkerProjectionEpoch != 0
			|| carrier.m_iResetMarkerProjectionEpoch != 0)
			return false;
		bool oldCheckpointEmpty = CheckpointTupleIsEmpty(
			carrier.m_sOldSavePointId,
			carrier.m_sOldSnapshotFingerprint,
			carrier.m_iOldCheckpointSequence,
			carrier.m_iOldRestoreSequence,
			carrier.m_iOldJournalGeneration,
			carrier.m_sOldJournalSlot,
			carrier.m_iOldJournalValidSlotCount,
			carrier.m_bOldJournalChainExact);
		bool blockerCheckpointEmpty = CheckpointTupleIsEmpty(
			carrier.m_sBlockerSavePointId,
			carrier.m_sBlockerSnapshotFingerprint,
			carrier.m_iBlockerCheckpointSequence,
			carrier.m_iBlockerRestoreSequence,
			carrier.m_iBlockerJournalGeneration,
			carrier.m_sBlockerJournalSlot,
			carrier.m_iBlockerJournalValidSlotCount,
			carrier.m_bBlockerJournalChainExact);
		bool resetCheckpointEmpty = CheckpointTupleIsEmpty(
			carrier.m_sResetSavePointId,
			carrier.m_sResetSnapshotFingerprint,
			carrier.m_iResetCheckpointSequence,
			carrier.m_iResetRestoreSequence,
			carrier.m_iResetJournalGeneration,
			carrier.m_sResetJournalSlot,
			carrier.m_iResetJournalValidSlotCount,
			carrier.m_bResetJournalChainExact);
		if (!oldCheckpointEmpty || !blockerCheckpointEmpty
			|| !resetCheckpointEmpty)
			return false;
		if (!ResetProofFlagsAreClear(carrier))
			return false;
		evidence = "admin reset persistence initial carrier exact";
		return true;
	}

	protected static bool CheckpointTupleIsEmpty(
		string savePointId,
		string snapshotFingerprint,
		int checkpointSequence,
		int restoreSequence,
		int journalGeneration,
		string journalSlot,
		int validSlotCount,
		bool chainExact)
	{
		return savePointId.IsEmpty() && snapshotFingerprint.IsEmpty()
			&& checkpointSequence == 0 && restoreSequence == 0
			&& journalGeneration == 0 && journalSlot.IsEmpty()
			&& validSlotCount == 0 && !chainExact;
	}

	protected static bool ResetProofFlagsAreClear(
		HST_AdminCampaignResetPersistenceProofCarrier carrier)
	{
		if (!carrier)
			return false;
		bool rejectionClear = !carrier.m_bInFlightCheckpointObserved
			&& !carrier.m_bInFlightResetRejected
			&& !carrier.m_bRejectedResetReturnedNoCheckpoint
			&& carrier.m_sRejectedResetBeforeFingerprint.IsEmpty()
			&& carrier.m_sRejectedResetAfterFingerprint.IsEmpty();
		bool rejectionEvidenceClear = !carrier.m_bRejectedResetStateExact
			&& !carrier.m_bRejectedResetSentinelExact
			&& !carrier.m_bRejectedResetIdentityExact
			&& !carrier.m_bRejectedResetEpochExact;
		bool blockerClear = !carrier.m_bBlockerCompletionReceived
			&& !carrier.m_bBlockerNativeCommitSucceeded
			&& !carrier.m_bBlockerProfileMirrorSaved
			&& !carrier.m_bBlockerOnAfterSaveSucceeded
			&& !carrier.m_bBlockerOnSaveCreatedObserved;
		bool resetClear = !carrier.m_bResetRemovedSentinel
			&& !carrier.m_bResetPreservedIdentity
			&& !carrier.m_bResetAdvancedEpoch
			&& !carrier.m_bResetRetainedSequenceFloors;
		return rejectionClear && rejectionEvidenceClear
			&& blockerClear && resetClear;
	}

	protected static bool ValidatePreparedCarrier(
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedPayloadNonce,
		string expectedWorld,
		out string evidence)
	{
		evidence = "admin reset prepared carrier rejected";
		string identityEvidence;
		if (!ValidateCarrierIdentity(
			carrier,
			expectedSessionNonce,
			expectedRunId,
			expectedPayloadNonce,
			expectedWorld,
			identityEvidence)
			|| carrier.m_iCompletedStageOrdinal != 0)
			return false;
		if (carrier.m_sOldSentinelTaskId != OLD_SENTINEL_TASK_ID
			|| carrier.m_sOldSentinelFingerprint
				!= BuildOldSentinelFingerprint(expectedPayloadNonce)
			|| carrier.m_sPreservedPlayerIdentityId
				!= BuildPreservedPlayerIdentityId(expectedPayloadNonce)
			|| carrier.m_sPreservedCommanderIdentityId
				!= carrier.m_sPreservedPlayerIdentityId
			|| carrier.m_sPreservedPlayerFingerprint.IsEmpty()
			|| carrier.m_iOldMarkerProjectionEpoch < 1
			|| carrier.m_iResetMarkerProjectionEpoch != 0)
			return false;
		if (!UUID.IsUUID(carrier.m_sOldSavePointId)
			|| carrier.m_sOldSnapshotFingerprint.IsEmpty()
			|| carrier.m_iOldCheckpointSequence != 2
			|| carrier.m_iOldRestoreSequence != 0
			|| carrier.m_iOldJournalGeneration != 1
			|| carrier.m_sOldJournalSlot
				!= HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			|| carrier.m_iOldJournalValidSlotCount != 1
			|| !carrier.m_bOldJournalChainExact)
			return false;
		bool blockerCheckpointEmpty = CheckpointTupleIsEmpty(
			carrier.m_sBlockerSavePointId,
			carrier.m_sBlockerSnapshotFingerprint,
			carrier.m_iBlockerCheckpointSequence,
			carrier.m_iBlockerRestoreSequence,
			carrier.m_iBlockerJournalGeneration,
			carrier.m_sBlockerJournalSlot,
			carrier.m_iBlockerJournalValidSlotCount,
			carrier.m_bBlockerJournalChainExact);
		bool resetCheckpointEmpty = CheckpointTupleIsEmpty(
			carrier.m_sResetSavePointId,
			carrier.m_sResetSnapshotFingerprint,
			carrier.m_iResetCheckpointSequence,
			carrier.m_iResetRestoreSequence,
			carrier.m_iResetJournalGeneration,
			carrier.m_sResetJournalSlot,
			carrier.m_iResetJournalValidSlotCount,
			carrier.m_bResetJournalChainExact);
		if (!blockerCheckpointEmpty || !resetCheckpointEmpty
			|| !ResetProofFlagsAreClear(carrier))
			return false;
		evidence = "admin reset prepared carrier exact";
		return true;
	}

	protected static bool PreparedSnapshotMatches(
		HST_AdminCampaignResetPersistenceProofCarrier first,
		HST_AdminCampaignResetPersistenceProofCarrier second)
	{
		if (!first || !second)
			return false;
		bool sentinelExact
			= first.m_sOldSentinelTaskId == second.m_sOldSentinelTaskId
			&& first.m_sOldSentinelFingerprint
				== second.m_sOldSentinelFingerprint;
		bool identityExact
			= first.m_sPreservedPlayerIdentityId
				== second.m_sPreservedPlayerIdentityId
			&& first.m_sPreservedPlayerFingerprint
				== second.m_sPreservedPlayerFingerprint
			&& first.m_sPreservedCommanderIdentityId
				== second.m_sPreservedCommanderIdentityId;
		bool checkpointIdentityExact
			= first.m_iOldMarkerProjectionEpoch
				== second.m_iOldMarkerProjectionEpoch
			&& first.m_sOldSavePointId == second.m_sOldSavePointId
			&& first.m_sOldSnapshotFingerprint
				== second.m_sOldSnapshotFingerprint;
		bool checkpointOrderExact
			= first.m_iOldCheckpointSequence
				== second.m_iOldCheckpointSequence
			&& first.m_iOldRestoreSequence
				== second.m_iOldRestoreSequence
			&& first.m_iOldJournalGeneration
				== second.m_iOldJournalGeneration;
		bool journalExact
			= first.m_sOldJournalSlot == second.m_sOldJournalSlot
			&& first.m_iOldJournalValidSlotCount
				== second.m_iOldJournalValidSlotCount
			&& first.m_bOldJournalChainExact
				== second.m_bOldJournalChainExact;
		return sentinelExact && identityExact && checkpointIdentityExact
			&& checkpointOrderExact && journalExact;
	}

	protected static bool ValidateResetCarrier(
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedPayloadNonce,
		string expectedWorld,
		out string evidence)
	{
		evidence = "admin reset committed carrier rejected";
		if (!carrier || carrier.m_iCompletedStageOrdinal != 1)
			return false;
		string identityEvidence;
		if (!ValidateCarrierIdentity(
			carrier,
			expectedSessionNonce,
			expectedRunId,
			expectedPayloadNonce,
			expectedWorld,
			identityEvidence))
			return false;
		if (carrier.m_sOldSentinelTaskId != OLD_SENTINEL_TASK_ID
			|| carrier.m_sOldSentinelFingerprint
				!= BuildOldSentinelFingerprint(expectedPayloadNonce)
			|| carrier.m_sPreservedPlayerIdentityId
				!= BuildPreservedPlayerIdentityId(expectedPayloadNonce)
			|| carrier.m_sPreservedCommanderIdentityId
				!= carrier.m_sPreservedPlayerIdentityId
			|| carrier.m_sPreservedPlayerFingerprint.IsEmpty()
			|| carrier.m_iOldMarkerProjectionEpoch < 1
			|| carrier.m_iOldMarkerProjectionEpoch >= int.MAX
			|| carrier.m_iResetMarkerProjectionEpoch
				!= carrier.m_iOldMarkerProjectionEpoch + 1)
			return false;
		if (!UUID.IsUUID(carrier.m_sOldSavePointId)
			|| carrier.m_sOldSnapshotFingerprint.IsEmpty()
			|| carrier.m_iOldCheckpointSequence != 2
			|| carrier.m_iOldRestoreSequence != 0
			|| carrier.m_iOldJournalGeneration != 1
			|| carrier.m_sOldJournalSlot
				!= HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			|| carrier.m_iOldJournalValidSlotCount != 1
			|| !carrier.m_bOldJournalChainExact)
			return false;
		if (!UUID.IsUUID(carrier.m_sBlockerSavePointId)
			|| carrier.m_sBlockerSavePointId == carrier.m_sOldSavePointId
			|| carrier.m_sBlockerSnapshotFingerprint.IsEmpty()
			|| carrier.m_sBlockerSnapshotFingerprint
				== carrier.m_sOldSnapshotFingerprint
			|| carrier.m_iBlockerCheckpointSequence
				!= carrier.m_iOldCheckpointSequence + 2
			|| carrier.m_iBlockerRestoreSequence
				!= carrier.m_iOldRestoreSequence + 1
			|| carrier.m_iBlockerJournalGeneration != 2
			|| carrier.m_sBlockerJournalSlot
				!= HST_CampaignProfileSaveJournalService.SLOT_RECOVERY
			|| carrier.m_iBlockerJournalValidSlotCount != 2
			|| !carrier.m_bBlockerJournalChainExact)
			return false;
		bool resetIdentityExact = UUID.IsUUID(carrier.m_sResetSavePointId)
			&& carrier.m_sResetSavePointId != carrier.m_sOldSavePointId
			&& carrier.m_sResetSavePointId != carrier.m_sBlockerSavePointId
			&& !carrier.m_sResetSnapshotFingerprint.IsEmpty();
		bool resetFingerprintExact
			= carrier.m_sResetSnapshotFingerprint
				!= carrier.m_sOldSnapshotFingerprint
			&& carrier.m_sResetSnapshotFingerprint
				!= carrier.m_sBlockerSnapshotFingerprint;
		bool resetOrderExact = carrier.m_iResetCheckpointSequence
				== carrier.m_iBlockerCheckpointSequence + 1
			&& carrier.m_iResetRestoreSequence
				== carrier.m_iBlockerRestoreSequence;
		bool resetJournalExact = carrier.m_iResetJournalGeneration == 3
			&& carrier.m_sResetJournalSlot
				== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			&& carrier.m_iResetJournalValidSlotCount == 2
			&& carrier.m_bResetJournalChainExact;
		if (!resetIdentityExact || !resetFingerprintExact
			|| !resetOrderExact || !resetJournalExact)
			return false;
		bool rejectionExact = carrier.m_bInFlightCheckpointObserved
			&& carrier.m_bInFlightResetRejected
			&& carrier.m_bRejectedResetReturnedNoCheckpoint
			&& !carrier.m_sRejectedResetBeforeFingerprint.IsEmpty()
			&& carrier.m_sRejectedResetBeforeFingerprint
				== carrier.m_sRejectedResetAfterFingerprint;
		bool rejectedStateExact = carrier.m_bRejectedResetStateExact
			&& carrier.m_bRejectedResetSentinelExact
			&& carrier.m_bRejectedResetIdentityExact
			&& carrier.m_bRejectedResetEpochExact;
		bool blockerCommitExact = carrier.m_bBlockerCompletionReceived
			&& carrier.m_bBlockerNativeCommitSucceeded
			&& carrier.m_bBlockerProfileMirrorSaved
			&& carrier.m_bBlockerOnAfterSaveSucceeded
			&& carrier.m_bBlockerOnSaveCreatedObserved;
		bool resetStateExact = carrier.m_bResetRemovedSentinel
			&& carrier.m_bResetPreservedIdentity
			&& carrier.m_bResetAdvancedEpoch
			&& carrier.m_bResetRetainedSequenceFloors;
		if (!rejectionExact || !rejectedStateExact
			|| !blockerCommitExact || !resetStateExact)
			return false;
		evidence = "admin reset committed carrier exact";
		return true;
	}

	protected static bool LoadCarrierFile(
		string runId,
		out HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out string evidence)
	{
		carrier = null;
		evidence = "admin reset persistence carrier unavailable";
		string path = BuildCarrierPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		carrier = new HST_AdminCampaignResetPersistenceProofCarrier();
		if (!context.ReadValue("", carrier))
		{
			carrier = null;
			return false;
		}
		return true;
	}

	static bool LoadAndValidateCarrier(
		string sessionNonce,
		string runId,
		string payloadNonce,
		string stage,
		string expectedWorld,
		out HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out string evidence)
	{
		carrier = null;
		if (!LoadCarrierFile(runId, carrier, evidence))
			return false;
		if (stage == STAGE_RESET_COMMIT)
		{
			return ValidatePreparedCarrier(
				carrier,
				sessionNonce,
				runId,
				payloadNonce,
				expectedWorld,
				evidence);
		}
		if (stage == STAGE_STALE_NATIVE_NO_SAVE_VERIFY)
		{
			return ValidateResetCarrier(
				carrier,
				sessionNonce,
				runId,
				payloadNonce,
				expectedWorld,
				evidence);
		}
		evidence = "admin reset carrier load is not valid for this stage";
		carrier = null;
		return false;
	}

	static bool ValidateGuardCarrierRelationship(
		HST_AdminCampaignResetPersistenceProofGuard guard,
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out string evidence)
	{
		evidence = "admin reset stage lease/carrier relationship rejected";
		if (!guard || !carrier
			|| guard.m_sSessionNonce != carrier.m_sSessionNonce
			|| guard.m_sRunId != carrier.m_sRunId
			|| guard.m_sPayloadNonce != carrier.m_sPayloadNonce)
			return false;
		if (guard.m_sRequestedStage == STAGE_RESET_COMMIT)
		{
			if (carrier.m_iCompletedStageOrdinal != 0
				|| guard.m_sExpectedLoadSavePointId
					!= carrier.m_sOldSavePointId)
				return false;
		}
		else if (guard.m_sRequestedStage
			== STAGE_STALE_NATIVE_NO_SAVE_VERIFY)
		{
			if (carrier.m_iCompletedStageOrdinal != 1
				|| guard.m_sExpectedLoadSavePointId
					!= carrier.m_sBlockerSavePointId)
				return false;
		}
		else if (guard.m_sRequestedStage
			!= STAGE_PREPARE_OLD_CHECKPOINT
			|| carrier.m_iCompletedStageOrdinal != -1
			|| !guard.m_sExpectedLoadSavePointId.IsEmpty())
			return false;
		evidence = "admin reset stage lease/carrier relationship exact";
		return true;
	}

	static bool SaveCarrier(
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out string evidence)
	{
		evidence = "admin reset persistence carrier write rejected";
		if (!carrier)
			return false;
		string validationEvidence;
		bool carrierExact = false;
		if (carrier.m_iCompletedStageOrdinal == 0)
		{
			carrierExact = ValidatePreparedCarrier(
				carrier,
				carrier.m_sSessionNonce,
				carrier.m_sRunId,
				carrier.m_sPayloadNonce,
				carrier.m_sWorld,
				validationEvidence);
		}
		else if (carrier.m_iCompletedStageOrdinal == 1)
		{
			carrierExact = ValidateResetCarrier(
				carrier,
				carrier.m_sSessionNonce,
				carrier.m_sRunId,
				carrier.m_sPayloadNonce,
				carrier.m_sWorld,
				validationEvidence);
		}
		if (!carrierExact)
		{
			evidence += " | " + validationEvidence;
			return false;
		}
		HST_AdminCampaignResetPersistenceProofOwner owner;
		string ownerEvidence;
		if (!LoadAndValidateOwner(
			carrier.m_sSessionNonce,
			carrier.m_sRunId,
			carrier.m_sWorld,
			owner,
			ownerEvidence)
			|| owner.m_sPayloadNonce != carrier.m_sPayloadNonce)
		{
			evidence = "admin reset persistence carrier owner rejected | "
				+ ownerEvidence;
			return false;
		}

		string path = BuildCarrierPath(carrier.m_sRunId);
		if (path.IsEmpty())
			return false;
		if (carrier.m_iCompletedStageOrdinal == 0)
		{
			if (FileIO.FileExists(path))
			{
				evidence = "admin reset prepared carrier storage is not fresh";
				return false;
			}
		}
		else
		{
			HST_AdminCampaignResetPersistenceProofCarrier prior;
			string priorEvidence;
			if (!LoadCarrierFile(carrier.m_sRunId, prior, priorEvidence)
				|| !ValidatePreparedCarrier(
					prior,
					carrier.m_sSessionNonce,
					carrier.m_sRunId,
					carrier.m_sPayloadNonce,
					carrier.m_sWorld,
					priorEvidence)
				|| !PreparedSnapshotMatches(prior, carrier))
			{
				evidence = "admin reset prior carrier transition rejected | "
					+ priorEvidence;
				return false;
			}
		}

		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", carrier) || !context.SaveToFile(path))
		{
			evidence = "admin reset persistence carrier write failed";
			return false;
		}
		evidence = "admin reset persistence carrier saved";
		return true;
	}

	static bool ValidateJournalSelection(
		HST_CampaignProfileSaveResolution resolution,
		int expectedGeneration,
		string expectedFingerprint,
		out string evidence)
	{
		evidence = "admin reset profile journal selection rejected";
		if (!resolution || expectedGeneration < 1
			|| expectedFingerprint.IsEmpty()
			|| !resolution.m_bArtifactsPresent
			|| !resolution.m_bHasSelection || !resolution.m_Selected
			|| resolution.m_bUnsupportedFuture || resolution.m_bAmbiguous
			|| !resolution.m_bChainExact)
			return false;
		string expectedSlot
			= HST_CampaignProfileSaveJournalService.SLOT_CANONICAL;
		if (expectedGeneration % 2 == 0)
			expectedSlot
				= HST_CampaignProfileSaveJournalService.SLOT_RECOVERY;
		int expectedValidCount = 2;
		if (expectedGeneration == 1)
			expectedValidCount = 1;
		HST_CampaignProfileSaveCandidate selected = resolution.m_Selected;
		if (!selected.m_bArtifactPresent || !selected.m_bEnvelopeRecognized
			|| selected.m_bLegacyRaw || !selected.m_bValid
			|| selected.m_bUnsupportedFuture || !selected.m_SaveData
			|| selected.m_sSlotLabel != expectedSlot
			|| selected.m_iGeneration != expectedGeneration
			|| selected.m_iSnapshotSchemaVersion
				!= HST_CampaignState.SCHEMA_VERSION
			|| selected.m_sSnapshotFingerprint != expectedFingerprint
			|| selected.m_sSnapshotPayload.IsEmpty()
			|| resolution.m_iValidCandidateCount != expectedValidCount)
			return false;
		evidence = string.Format(
			"admin reset profile journal selection exact | slot/generation/valid %1/%2/%3 | fingerprint %4",
			selected.m_sSlotLabel,
			selected.m_iGeneration,
			resolution.m_iValidCandidateCount,
			selected.m_sSnapshotFingerprint);
		return true;
	}

	static bool ValidateResetJournalBoundary(
		HST_CampaignProfileSaveResolution resolution,
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out HST_CampaignState selectedResetState,
		out HST_CampaignState staleBlockerState,
		out string evidence)
	{
		selectedResetState = null;
		staleBlockerState = null;
		evidence = "admin reset profile journal boundary rejected";
		if (!resolution || !carrier)
			return false;
		string selectionEvidence;
		if (!ValidateJournalSelection(
			resolution,
			3,
			carrier.m_sResetSnapshotFingerprint,
			selectionEvidence))
		{
			evidence += " | " + selectionEvidence;
			return false;
		}
		HST_CampaignProfileSaveCandidate canonical = resolution.m_Canonical;
		HST_CampaignProfileSaveCandidate recovery = resolution.m_Recovery;
		if (!canonical || !recovery
			|| resolution.m_Selected.m_sSlotLabel
				!= HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			|| canonical.m_sSlotLabel
				!= HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			|| recovery.m_sSlotLabel
				!= HST_CampaignProfileSaveJournalService.SLOT_RECOVERY)
			return false;
		if (!canonical.m_bArtifactPresent
			|| !canonical.m_bEnvelopeRecognized || canonical.m_bLegacyRaw
			|| !canonical.m_bValid || canonical.m_bUnsupportedFuture
			|| !canonical.m_SaveData || canonical.m_iGeneration != 3
			|| canonical.m_iSnapshotSchemaVersion
				!= HST_CampaignState.SCHEMA_VERSION
			|| canonical.m_sSnapshotFingerprint
				!= carrier.m_sResetSnapshotFingerprint
			|| canonical.m_iPreviousGeneration != 2
			|| canonical.m_sPreviousSnapshotFingerprint
				!= carrier.m_sBlockerSnapshotFingerprint)
			return false;
		if (!recovery.m_bArtifactPresent
			|| !recovery.m_bEnvelopeRecognized || recovery.m_bLegacyRaw
			|| !recovery.m_bValid || recovery.m_bUnsupportedFuture
			|| !recovery.m_SaveData || recovery.m_iGeneration != 2
			|| recovery.m_iSnapshotSchemaVersion
				!= HST_CampaignState.SCHEMA_VERSION
			|| recovery.m_sSnapshotFingerprint
				!= carrier.m_sBlockerSnapshotFingerprint
			|| recovery.m_iPreviousGeneration != 1
			|| recovery.m_sPreviousSnapshotFingerprint
				!= carrier.m_sOldSnapshotFingerprint)
			return false;
		selectedResetState = canonical.m_SaveData.Restore();
		staleBlockerState = recovery.m_SaveData.Restore();
		if (!selectedResetState || !staleBlockerState)
			return false;
		int selectedSentinelCount;
		string selectedSentinelEvidence;
		bool selectedSentinelAbsent = ValidateOldSentinelAbsent(
			selectedResetState,
			selectedSentinelCount,
			selectedSentinelEvidence);
		int staleSentinelCount;
		string staleSentinelFingerprint;
		string staleSentinelEvidence;
		bool staleSentinelExact = ValidateOldSentinel(
			staleBlockerState,
			carrier.m_sPayloadNonce,
			staleSentinelCount,
			staleSentinelFingerprint,
			staleSentinelEvidence)
			&& staleSentinelFingerprint
				== carrier.m_sOldSentinelFingerprint;
		string selectedPlayerFingerprint;
		string selectedIdentityEvidence;
		bool selectedIdentityExact = ValidatePreservedIdentity(
			selectedResetState,
			carrier,
			selectedPlayerFingerprint,
			selectedIdentityEvidence);
		string stalePlayerFingerprint;
		string staleIdentityEvidence;
		bool staleIdentityExact = ValidatePreservedIdentity(
			staleBlockerState,
			carrier,
			stalePlayerFingerprint,
			staleIdentityEvidence);
		bool selectedOrderExact
			= selectedResetState.m_iPersistenceCheckpointSequence
				== carrier.m_iResetCheckpointSequence
			&& selectedResetState.m_iPersistenceRestoreSequence
				== carrier.m_iResetRestoreSequence
			&& selectedResetState.m_iMarkerProjectionEpoch
				== carrier.m_iResetMarkerProjectionEpoch;
		bool staleOrderExact
			= staleBlockerState.m_iPersistenceCheckpointSequence
				== carrier.m_iBlockerCheckpointSequence
			&& staleBlockerState.m_iPersistenceRestoreSequence
				== carrier.m_iBlockerRestoreSequence
			&& staleBlockerState.m_iMarkerProjectionEpoch
				== carrier.m_iOldMarkerProjectionEpoch;
		if (!selectedSentinelAbsent || !staleSentinelExact
			|| !selectedIdentityExact || !staleIdentityExact
			|| !selectedOrderExact || !staleOrderExact)
		{
			evidence += " | " + selectionEvidence
				+ " | " + selectedSentinelEvidence
				+ " | " + staleSentinelEvidence
				+ " | " + selectedIdentityEvidence
				+ " | " + staleIdentityEvidence;
			return false;
		}
		evidence = selectionEvidence
			+ " | selected reset excludes old sentinel"
			+ " | stale recovery generation retains old sentinel"
			+ " | selected/stale durable order exact";
		return true;
	}

	static bool CreateStageResult(
		HST_AdminCampaignResetPersistenceProofOwner owner,
		HST_AdminCampaignResetPersistenceProofGuard guard,
		out HST_AdminCampaignResetPersistenceProofResult result,
		out string evidence)
	{
		result = null;
		evidence = "admin reset stage result creation rejected";
		if (!owner || !guard)
			return false;
		string ownerEvidence;
		if (!ValidateOwner(
			owner,
			owner.m_sSessionNonce,
			owner.m_sRunId,
			owner.m_sWorld,
			ownerEvidence))
			return false;
		string guardEvidence;
		if (!ValidateGuard(
			guard,
			owner.m_sSessionNonce,
			guard.m_sStageNonce,
			owner.m_sRunId,
			owner.m_sPayloadNonce,
			guard.m_sRequestedStage,
			owner.m_sWorld,
			guardEvidence))
			return false;
		result = new HST_AdminCampaignResetPersistenceProofResult();
		result.m_sMagic = RESULT_MAGIC;
		result.m_iVersion = AUTHORITY_VERSION;
		result.m_sSessionNonce = owner.m_sSessionNonce;
		result.m_sStageNonce = guard.m_sStageNonce;
		result.m_sRunId = owner.m_sRunId;
		result.m_sPayloadNonce = owner.m_sPayloadNonce;
		result.m_sStage = guard.m_sRequestedStage;
		result.m_iStageOrdinal = guard.m_iStageOrdinal;
		result.m_sBuildSha = HST_BuildInfo.BUILD_SHA;
		result.m_sBuildUtc = HST_BuildInfo.BUILD_UTC;
		result.m_sBuildLabel = HST_BuildInfo.BUILD_LABEL;
		result.m_iCampaignSchemaVersion
			= HST_CampaignState.SCHEMA_VERSION;
		result.m_iSettingsSchemaVersion
			= HST_RuntimeSettings.SCHEMA_VERSION;
		result.m_sWorld = CANONICAL_WORLD;
		result.m_sExpectedSource = ResolveExpectedSource(result.m_sStage);
		result.m_sExpectedSaveType
			= ResolveExpectedSaveType(result.m_sStage);
		result.m_sExpectedSaveName
			= ResolveExpectedSaveName(result.m_sStage);
		result.m_iSourceJournalGeneration = -1;
		result.m_iCommittedJournalGeneration = -1;
		result.m_bNoSaveStage = StageIsNoSave(result.m_sStage);
		evidence = "admin reset stage result created";
		return true;
	}

	static bool PopulateSourceObservation(
		HST_AdminCampaignResetPersistenceProofResult result,
		HST_PersistenceSourceResolution source,
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out string evidence)
	{
		evidence = "admin reset startup source observation rejected";
		if (!result || !source || !carrier
			|| !source.HasSelectedState()
			|| !ValidateStage(result.m_sStage))
			return false;
		result.m_sExpectedSource = ResolveExpectedSource(result.m_sStage);
		result.m_sSource = source.BuildSourceLabel();
		result.m_bPersistenceSystemAvailable
			= source.m_bPersistenceSystemAvailable;
		result.m_bPersistenceSystemLoadedData
			= source.m_bPersistenceSystemLoadedData;
		result.m_bNativeRecordPresent = source.m_bNativeRecordPresent;
		result.m_bNativeRecordValid = source.m_bNativeRecordValid;
		result.m_bProfileFallbackPresent
			= source.m_bProfileFallbackPresent;
		result.m_bProfileFallbackRead = source.m_bProfileFallbackRead;
		result.m_bDegradedNativeRecovery
			= source.m_bDegradedNativeRecovery;
		result.m_sDegradedNativeRecoveryReason
			= source.m_sDegradedNativeRecoveryReason;
		result.m_sNativeSnapshotFingerprint
			= source.m_sNativeSnapshotFingerprint;
		result.m_sProfileFallbackSnapshotFingerprint
			= source.m_sProfileFallbackSnapshotFingerprint;
		result.m_sSelectedSnapshotFingerprint
			= source.m_sSelectedSnapshotFingerprint;
		result.m_iSourceJournalGeneration
			= source.m_iProfileJournalGeneration;
		result.m_sSourceJournalSlot = source.m_sProfileJournalSlot;
		result.m_iSourceJournalValidSlotCount
			= source.m_iProfileJournalValidSlotCount;
		result.m_bSourceJournalLegacyRaw
			= source.m_bProfileJournalLegacyRaw;
		result.m_bSourceJournalChainExact
			= source.m_bProfileJournalChainExact;

		bool exact;
		if (result.m_sStage == STAGE_PREPARE_OLD_CHECKPOINT)
		{
			bool sourceExact = result.m_sSource == SOURCE_NEW_CAMPAIGN
				&& result.m_bPersistenceSystemAvailable
				&& !result.m_bPersistenceSystemLoadedData;
			bool recordsAbsent = !result.m_bNativeRecordPresent
				&& !result.m_bNativeRecordValid
				&& !result.m_bProfileFallbackPresent
				&& !result.m_bProfileFallbackRead
				&& !result.m_bDegradedNativeRecovery;
			bool recoveryEmpty
				= result.m_sDegradedNativeRecoveryReason.IsEmpty()
				&& result.m_sNativeSnapshotFingerprint.IsEmpty()
				&& result.m_sProfileFallbackSnapshotFingerprint.IsEmpty()
				&& result.m_sSelectedSnapshotFingerprint.IsEmpty();
			bool journalEmpty = result.m_iSourceJournalGeneration == -1
				&& result.m_sSourceJournalSlot.IsEmpty()
				&& result.m_iSourceJournalValidSlotCount == 0
				&& !result.m_bSourceJournalLegacyRaw
				&& !result.m_bSourceJournalChainExact;
			exact = sourceExact && recordsAbsent
				&& recoveryEmpty && journalEmpty;
		}
		else if (result.m_sStage == STAGE_RESET_COMMIT)
		{
			bool sourceExact = result.m_sSource == SOURCE_NATIVE
				&& result.m_bPersistenceSystemAvailable
				&& result.m_bPersistenceSystemLoadedData
				&& result.m_bNativeRecordPresent
				&& result.m_bNativeRecordValid;
			bool recoveryExact = result.m_bProfileFallbackPresent
				&& result.m_bProfileFallbackRead
				&& !result.m_bDegradedNativeRecovery
				&& result.m_sDegradedNativeRecoveryReason.IsEmpty();
			bool fingerprintsExact = result.m_sNativeSnapshotFingerprint
					== carrier.m_sOldSnapshotFingerprint
				&& result.m_sProfileFallbackSnapshotFingerprint
					== carrier.m_sOldSnapshotFingerprint
				&& result.m_sSelectedSnapshotFingerprint
					== carrier.m_sOldSnapshotFingerprint;
			bool journalExact = result.m_iSourceJournalGeneration == 1
				&& result.m_sSourceJournalSlot
					== HST_CampaignProfileSaveJournalService
						.SLOT_CANONICAL
				&& result.m_iSourceJournalValidSlotCount == 1
				&& !result.m_bSourceJournalLegacyRaw
				&& result.m_bSourceJournalChainExact;
			exact = sourceExact && recoveryExact
				&& fingerprintsExact && journalExact;
		}
		else
		{
			bool sourceExact = result.m_sSource == SOURCE_PROFILE_FALLBACK
				&& result.m_bPersistenceSystemAvailable
				&& result.m_bPersistenceSystemLoadedData
				&& result.m_bNativeRecordPresent
				&& result.m_bNativeRecordValid;
			bool recoveryExact = result.m_bProfileFallbackPresent
				&& result.m_bProfileFallbackRead
				&& result.m_bDegradedNativeRecovery
				&& result.m_sDegradedNativeRecoveryReason.Contains(
					"profile journal ordering is newer than the valid native record");
			bool fingerprintsExact = result.m_sNativeSnapshotFingerprint
					== carrier.m_sBlockerSnapshotFingerprint
				&& result.m_sProfileFallbackSnapshotFingerprint
					== carrier.m_sResetSnapshotFingerprint
				&& result.m_sSelectedSnapshotFingerprint
					== carrier.m_sResetSnapshotFingerprint;
			bool journalExact = result.m_iSourceJournalGeneration == 3
				&& result.m_sSourceJournalSlot
					== HST_CampaignProfileSaveJournalService
						.SLOT_CANONICAL
				&& result.m_iSourceJournalValidSlotCount == 2
				&& !result.m_bSourceJournalLegacyRaw
				&& result.m_bSourceJournalChainExact;
			exact = sourceExact && recoveryExact
				&& fingerprintsExact && journalExact;
		}
		result.m_bSourceExact = exact;
		evidence = string.Format(
			"admin reset startup source expected/actual/exact %1/%2/%3 | native/profile/selected %4/%5/%6",
			result.m_sExpectedSource,
			result.m_sSource,
			exact,
			result.m_sNativeSnapshotFingerprint,
			result.m_sProfileFallbackSnapshotFingerprint,
			result.m_sSelectedSnapshotFingerprint);
		return exact;
	}

	static bool PopulateCommittedJournalObservation(
		HST_AdminCampaignResetPersistenceProofResult result,
		HST_CampaignProfileSaveResolution resolution,
		string expectedFingerprint,
		out string evidence)
	{
		evidence = "admin reset committed journal observation rejected";
		if (!result || !resolution || expectedFingerprint.IsEmpty()
			|| !StageCreatesSavePoints(result.m_sStage))
			return false;
		int expectedGeneration = 1;
		if (result.m_sStage == STAGE_RESET_COMMIT)
			expectedGeneration = 3;
		if (!ValidateJournalSelection(
			resolution,
			expectedGeneration,
			expectedFingerprint,
			evidence))
			return false;
		result.m_iCommittedJournalGeneration
			= resolution.m_Selected.m_iGeneration;
		result.m_sCommittedJournalSlot
			= resolution.m_Selected.m_sSlotLabel;
		result.m_iCommittedJournalValidSlotCount
			= resolution.m_iValidCandidateCount;
		result.m_bCommittedJournalLegacyRaw
			= resolution.m_Selected.m_bLegacyRaw;
		result.m_bCommittedJournalChainExact = resolution.m_bChainExact;
		result.m_sCommittedJournalFingerprint
			= resolution.m_Selected.m_sSnapshotFingerprint;
		return true;
	}

	static bool PopulateLiveStateObservation(
		HST_AdminCampaignResetPersistenceProofResult result,
		HST_CampaignState state,
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out string evidence)
	{
		evidence = "admin reset live state observation rejected";
		if (!result || !state || !carrier || !ValidateStage(result.m_sStage))
			return false;
		result.m_sExpectedPlayerIdentityId
			= carrier.m_sPreservedPlayerIdentityId;
		result.m_sExpectedCommanderIdentityId
			= carrier.m_sPreservedCommanderIdentityId;
		result.m_sObservedCommanderIdentityId
			= state.m_sCommanderIdentityId;
		string playerEvidence;
		result.m_bPlayerIdentityExact = ValidatePreservedIdentity(
			state,
			carrier,
			result.m_sObservedPlayerFingerprint,
			playerEvidence);
		result.m_bCommanderIdentityExact
			= result.m_sObservedCommanderIdentityId
				== result.m_sExpectedCommanderIdentityId;

		bool sentinelExact;
		string sentinelEvidence;
		if (result.m_sStage == STAGE_PREPARE_OLD_CHECKPOINT)
		{
			sentinelExact = ValidateOldSentinel(
				state,
				carrier.m_sPayloadNonce,
				result.m_iLiveOldSentinelCount,
				result.m_sLiveOldSentinelFingerprint,
				sentinelEvidence)
				&& result.m_sLiveOldSentinelFingerprint
					== carrier.m_sOldSentinelFingerprint;
			result.m_bOldSentinelRejected = false;
			result.m_iExpectedMarkerProjectionEpoch
				= carrier.m_iOldMarkerProjectionEpoch;
			result.m_iExpectedCheckpointSequence
				= carrier.m_iOldCheckpointSequence;
			result.m_iExpectedRestoreSequence
				= carrier.m_iOldRestoreSequence;
		}
		else
		{
			sentinelExact = ValidateOldSentinelAbsent(
				state,
				result.m_iLiveOldSentinelCount,
				sentinelEvidence);
			result.m_sLiveOldSentinelFingerprint = "";
			result.m_bOldSentinelRejected = sentinelExact;
			result.m_iExpectedMarkerProjectionEpoch
				= carrier.m_iResetMarkerProjectionEpoch;
			result.m_iExpectedCheckpointSequence
				= carrier.m_iResetCheckpointSequence;
			result.m_iExpectedRestoreSequence
				= carrier.m_iResetRestoreSequence;
			if (result.m_sStage
				== STAGE_STALE_NATIVE_NO_SAVE_VERIFY)
			{
				result.m_iExpectedCheckpointSequence++;
				result.m_iExpectedRestoreSequence++;
			}
		}
		result.m_iObservedMarkerProjectionEpoch
			= state.m_iMarkerProjectionEpoch;
		result.m_bMarkerProjectionEpochExact
			= result.m_iObservedMarkerProjectionEpoch
				== result.m_iExpectedMarkerProjectionEpoch;
		result.m_iObservedCheckpointSequence
			= state.m_iPersistenceCheckpointSequence;
		result.m_iObservedRestoreSequence
			= state.m_iPersistenceRestoreSequence;
		result.m_bDurableOrderExact
			= result.m_iObservedCheckpointSequence
				== result.m_iExpectedCheckpointSequence
			&& result.m_iObservedRestoreSequence
				== result.m_iExpectedRestoreSequence;
		bool exact = sentinelExact && result.m_bPlayerIdentityExact
			&& result.m_bCommanderIdentityExact
			&& result.m_bMarkerProjectionEpochExact
			&& result.m_bDurableOrderExact;
		evidence = sentinelEvidence + " | " + playerEvidence
			+ string.Format(
				" | identity/commander/epoch/order %1/%2/%3/%4",
				result.m_bPlayerIdentityExact,
				result.m_bCommanderIdentityExact,
				result.m_bMarkerProjectionEpochExact,
				result.m_bDurableOrderExact);
		return exact;
	}

	static bool PopulateResetJournalBoundaryObservation(
		HST_AdminCampaignResetPersistenceProofResult result,
		HST_CampaignProfileSaveResolution resolution,
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out string evidence)
	{
		evidence = "admin reset stale journal sentinel observation rejected";
		if (!result || !carrier
			|| (result.m_sStage != STAGE_RESET_COMMIT
				&& result.m_sStage
					!= STAGE_STALE_NATIVE_NO_SAVE_VERIFY))
			return false;
		HST_CampaignState selectedResetState;
		HST_CampaignState staleBlockerState;
		if (!ValidateResetJournalBoundary(
			resolution,
			carrier,
			selectedResetState,
			staleBlockerState,
			evidence))
			return false;
		result.m_iStaleJournalOldSentinelCount = 0;
		result.m_sStaleJournalOldSentinelFingerprint = "";
		string sentinelEvidence;
		if (!ValidateOldSentinel(
			staleBlockerState,
			carrier.m_sPayloadNonce,
			result.m_iStaleJournalOldSentinelCount,
			result.m_sStaleJournalOldSentinelFingerprint,
			sentinelEvidence)
			|| result.m_sStaleJournalOldSentinelFingerprint
				!= carrier.m_sOldSentinelFingerprint)
		{
			evidence += " | " + sentinelEvidence;
			return false;
		}
		result.m_bOldSentinelRejected
			= result.m_iLiveOldSentinelCount == 0;
		evidence += " | " + sentinelEvidence;
		return result.m_bOldSentinelRejected;
	}

	protected static bool ValidateResultSource(
		HST_AdminCampaignResetPersistenceProofResult result,
		HST_AdminCampaignResetPersistenceProofCarrier carrier)
	{
		if (!result || !carrier || !result.m_bSourceExact
			|| result.m_sExpectedSource
				!= ResolveExpectedSource(result.m_sStage)
			|| result.m_sSource != result.m_sExpectedSource
			|| !result.m_bPersistenceSystemAvailable)
			return false;
		if (result.m_sStage == STAGE_PREPARE_OLD_CHECKPOINT)
		{
			bool recordsAbsent = !result.m_bPersistenceSystemLoadedData
				&& !result.m_bNativeRecordPresent
				&& !result.m_bNativeRecordValid
				&& !result.m_bProfileFallbackPresent
				&& !result.m_bProfileFallbackRead;
			bool recoveryEmpty = !result.m_bDegradedNativeRecovery
				&& result.m_sDegradedNativeRecoveryReason.IsEmpty()
				&& result.m_sNativeSnapshotFingerprint.IsEmpty()
				&& result.m_sProfileFallbackSnapshotFingerprint.IsEmpty()
				&& result.m_sSelectedSnapshotFingerprint.IsEmpty();
			bool journalEmpty = result.m_iSourceJournalGeneration == -1
				&& result.m_sSourceJournalSlot.IsEmpty()
				&& result.m_iSourceJournalValidSlotCount == 0
				&& !result.m_bSourceJournalLegacyRaw
				&& !result.m_bSourceJournalChainExact;
			return recordsAbsent && recoveryEmpty && journalEmpty;
		}
		if (result.m_sStage == STAGE_RESET_COMMIT)
		{
			bool authorityExact = result.m_bPersistenceSystemLoadedData
				&& result.m_bNativeRecordPresent
				&& result.m_bNativeRecordValid
				&& result.m_bProfileFallbackPresent
				&& result.m_bProfileFallbackRead;
			bool recoveryExact = !result.m_bDegradedNativeRecovery
				&& result.m_sDegradedNativeRecoveryReason.IsEmpty();
			bool fingerprintsExact = result.m_sNativeSnapshotFingerprint
					== carrier.m_sOldSnapshotFingerprint
				&& result.m_sProfileFallbackSnapshotFingerprint
					== carrier.m_sOldSnapshotFingerprint
				&& result.m_sSelectedSnapshotFingerprint
					== carrier.m_sOldSnapshotFingerprint;
			bool journalExact = result.m_iSourceJournalGeneration == 1
				&& result.m_sSourceJournalSlot
					== HST_CampaignProfileSaveJournalService
						.SLOT_CANONICAL
				&& result.m_iSourceJournalValidSlotCount == 1
				&& !result.m_bSourceJournalLegacyRaw
				&& result.m_bSourceJournalChainExact;
			return authorityExact && recoveryExact
				&& fingerprintsExact && journalExact;
		}
		bool authorityExact = result.m_bPersistenceSystemLoadedData
			&& result.m_bNativeRecordPresent
			&& result.m_bNativeRecordValid
			&& result.m_bProfileFallbackPresent
			&& result.m_bProfileFallbackRead;
		bool recoveryExact = result.m_bDegradedNativeRecovery
			&& result.m_sDegradedNativeRecoveryReason.Contains(
				"profile journal ordering is newer than the valid native record");
		bool fingerprintsExact = result.m_sNativeSnapshotFingerprint
				== carrier.m_sBlockerSnapshotFingerprint
			&& result.m_sProfileFallbackSnapshotFingerprint
				== carrier.m_sResetSnapshotFingerprint
			&& result.m_sSelectedSnapshotFingerprint
				== carrier.m_sResetSnapshotFingerprint;
		bool journalExact = result.m_iSourceJournalGeneration == 3
			&& result.m_sSourceJournalSlot
				== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			&& result.m_iSourceJournalValidSlotCount == 2
			&& !result.m_bSourceJournalLegacyRaw
			&& result.m_bSourceJournalChainExact;
		return authorityExact && recoveryExact
			&& fingerprintsExact && journalExact;
	}

	protected static bool ValidateResultState(
		HST_AdminCampaignResetPersistenceProofResult result,
		HST_AdminCampaignResetPersistenceProofCarrier carrier)
	{
		if (!result || !carrier)
			return false;
		bool playerExact = result.m_sExpectedPlayerIdentityId
				== carrier.m_sPreservedPlayerIdentityId
			&& result.m_sObservedPlayerFingerprint
				== carrier.m_sPreservedPlayerFingerprint
			&& result.m_bPlayerIdentityExact;
		bool commanderExact = result.m_sExpectedCommanderIdentityId
				== carrier.m_sPreservedCommanderIdentityId
			&& result.m_sObservedCommanderIdentityId
				== carrier.m_sPreservedCommanderIdentityId
			&& result.m_bCommanderIdentityExact;
		if (!playerExact || !commanderExact
			|| !result.m_bMarkerProjectionEpochExact
			|| !result.m_bDurableOrderExact)
			return false;
		if (result.m_sStage == STAGE_PREPARE_OLD_CHECKPOINT)
		{
			bool sentinelExact = result.m_iLiveOldSentinelCount == 1
				&& result.m_sLiveOldSentinelFingerprint
					== carrier.m_sOldSentinelFingerprint
				&& result.m_iStaleJournalOldSentinelCount == 0
				&& result.m_sStaleJournalOldSentinelFingerprint.IsEmpty()
				&& !result.m_bOldSentinelRejected;
			bool epochExact = result.m_iExpectedMarkerProjectionEpoch
					== carrier.m_iOldMarkerProjectionEpoch
				&& result.m_iObservedMarkerProjectionEpoch
					== carrier.m_iOldMarkerProjectionEpoch;
			bool checkpointExact = result.m_iExpectedCheckpointSequence
					== carrier.m_iOldCheckpointSequence
				&& result.m_iObservedCheckpointSequence
					== carrier.m_iOldCheckpointSequence;
			bool restoreExact = result.m_iExpectedRestoreSequence
					== carrier.m_iOldRestoreSequence
				&& result.m_iObservedRestoreSequence
					== carrier.m_iOldRestoreSequence;
			return sentinelExact && epochExact
				&& checkpointExact && restoreExact;
		}
		int expectedCheckpoint = carrier.m_iResetCheckpointSequence;
		int expectedRestore = carrier.m_iResetRestoreSequence;
		if (result.m_sStage == STAGE_STALE_NATIVE_NO_SAVE_VERIFY)
		{
			expectedCheckpoint++;
			expectedRestore++;
		}
		bool sentinelExact = result.m_iLiveOldSentinelCount == 0
			&& result.m_sLiveOldSentinelFingerprint.IsEmpty()
			&& result.m_iStaleJournalOldSentinelCount == 1
			&& result.m_sStaleJournalOldSentinelFingerprint
				== carrier.m_sOldSentinelFingerprint
			&& result.m_bOldSentinelRejected;
		bool epochExact = result.m_iExpectedMarkerProjectionEpoch
				== carrier.m_iResetMarkerProjectionEpoch
			&& result.m_iObservedMarkerProjectionEpoch
				== carrier.m_iResetMarkerProjectionEpoch;
		bool checkpointExact
			= result.m_iExpectedCheckpointSequence == expectedCheckpoint
			&& result.m_iObservedCheckpointSequence == expectedCheckpoint;
		bool restoreExact = result.m_iExpectedRestoreSequence == expectedRestore
			&& result.m_iObservedRestoreSequence == expectedRestore;
		return sentinelExact && epochExact
			&& checkpointExact && restoreExact;
	}

	protected static bool ValidateResultCheckpoint(
		HST_AdminCampaignResetPersistenceProofResult result,
		HST_AdminCampaignResetPersistenceProofCarrier carrier)
	{
		if (!result || !carrier
			|| result.m_sExpectedSaveType
				!= ResolveExpectedSaveType(result.m_sStage)
			|| result.m_sExpectedSaveName
				!= ResolveExpectedSaveName(result.m_sStage))
			return false;
		if (result.m_sStage == STAGE_STALE_NATIVE_NO_SAVE_VERIFY)
		{
			bool journalEmpty = result.m_iCommittedJournalGeneration == -1
				&& result.m_sCommittedJournalSlot.IsEmpty()
				&& result.m_iCommittedJournalValidSlotCount == 0
				&& !result.m_bCommittedJournalLegacyRaw
				&& !result.m_bCommittedJournalChainExact
				&& result.m_sCommittedJournalFingerprint.IsEmpty();
			bool activeIdentityExact = result.m_sExpectedPriorSavePointId
					== carrier.m_sBlockerSavePointId
				&& result.m_sObservedPriorSavePointId
					== carrier.m_sBlockerSavePointId
				&& result.m_sCreatedSavePointId.IsEmpty()
				&& result.m_sActiveSavePointId
					== carrier.m_sBlockerSavePointId;
			bool saveTypeExact = result.m_sCreatedSaveType == SAVE_TYPE_NONE
				&& result.m_sCreatedSaveName.IsEmpty();
			bool noRequestExact = !result.m_bCampaignCaptured
				&& !result.m_bTransientStateStaged
				&& !result.m_bSavePointRequested
				&& !result.m_bCompletionReceived;
			bool noCommitExact = !result.m_bNativeCommitSucceeded
				&& !result.m_bProfileMirrorSaved
				&& !result.m_bCompletionObserverSucceeded
				&& !result.m_bOnAfterSaveObserved
				&& !result.m_bOnAfterSaveSucceeded
				&& !result.m_bOnSaveCreatedObserved;
			return journalEmpty && activeIdentityExact && saveTypeExact
				&& noRequestExact && noCommitExact
				&& result.m_bActiveSaveExact;
		}

		int expectedGeneration = 1;
		string expectedSlot
			= HST_CampaignProfileSaveJournalService.SLOT_CANONICAL;
		int expectedValidCount = 1;
		string expectedFingerprint = carrier.m_sOldSnapshotFingerprint;
		string expectedPriorId = "";
		string expectedCreatedId = carrier.m_sOldSavePointId;
		if (result.m_sStage == STAGE_RESET_COMMIT)
		{
			expectedGeneration = 3;
			expectedValidCount = 2;
			expectedFingerprint = carrier.m_sResetSnapshotFingerprint;
			expectedPriorId = carrier.m_sBlockerSavePointId;
			expectedCreatedId = carrier.m_sResetSavePointId;
		}
		bool journalExact
			= result.m_iCommittedJournalGeneration == expectedGeneration
			&& result.m_sCommittedJournalSlot == expectedSlot
			&& result.m_iCommittedJournalValidSlotCount == expectedValidCount
			&& !result.m_bCommittedJournalLegacyRaw
			&& result.m_bCommittedJournalChainExact
			&& result.m_sCommittedJournalFingerprint == expectedFingerprint;
		bool savePointIdentityExact
			= result.m_sExpectedPriorSavePointId == expectedPriorId
			&& result.m_sObservedPriorSavePointId == expectedPriorId
			&& result.m_sCreatedSavePointId == expectedCreatedId
			&& result.m_sActiveSavePointId == expectedCreatedId
			&& UUID.IsUUID(result.m_sCreatedSavePointId);
		bool saveTypeExact = result.m_sCreatedSaveType == SAVE_TYPE_MANUAL
			&& result.m_sCreatedSaveName == MANUAL_SAVE_NAME;
		bool requestExact = result.m_bCampaignCaptured
			&& result.m_bTransientStateStaged
			&& result.m_bSavePointRequested
			&& result.m_bCompletionReceived
			&& result.m_bNativeCommitSucceeded;
		bool mirrorExact = result.m_bProfileMirrorSaved
			&& result.m_bCompletionObserverSucceeded
			&& result.m_bOnAfterSaveObserved
			&& result.m_bOnAfterSaveSucceeded
			&& result.m_bOnSaveCreatedObserved
			&& result.m_bActiveSaveExact;
		return journalExact && savePointIdentityExact && saveTypeExact
			&& requestExact && mirrorExact;
	}

	protected static bool ValidateResultResetRejection(
		HST_AdminCampaignResetPersistenceProofResult result,
		HST_AdminCampaignResetPersistenceProofCarrier carrier)
	{
		if (!result || !carrier)
			return false;
		if (result.m_sStage != STAGE_RESET_COMMIT)
		{
			bool resetClear = !result.m_bInFlightCheckpointObserved
				&& !result.m_bInFlightResetRejected
				&& !result.m_bRejectedResetReturnedNoCheckpoint
				&& result.m_sRejectedResetBeforeFingerprint.IsEmpty()
				&& result.m_sRejectedResetAfterFingerprint.IsEmpty();
			bool evidenceClear = !result.m_bRejectedResetStateExact
				&& !result.m_bRejectedResetSentinelExact
				&& !result.m_bRejectedResetIdentityExact
				&& !result.m_bRejectedResetEpochExact;
			return resetClear && evidenceClear;
		}
		bool rejectionExact = result.m_bInFlightCheckpointObserved
			&& result.m_bInFlightResetRejected
			&& result.m_bRejectedResetReturnedNoCheckpoint
			&& result.m_sRejectedResetBeforeFingerprint
				== carrier.m_sRejectedResetBeforeFingerprint
			&& result.m_sRejectedResetAfterFingerprint
				== carrier.m_sRejectedResetAfterFingerprint;
		bool fingerprintExact = result.m_sRejectedResetBeforeFingerprint
				== result.m_sRejectedResetAfterFingerprint
			&& !result.m_sRejectedResetBeforeFingerprint.IsEmpty();
		bool stateExact = result.m_bRejectedResetStateExact
			&& result.m_bRejectedResetSentinelExact
			&& result.m_bRejectedResetIdentityExact
			&& result.m_bRejectedResetEpochExact;
		return rejectionExact && fingerprintExact && stateExact;
	}

	protected static bool ValidateResultNoSave(
		HST_AdminCampaignResetPersistenceProofResult result)
	{
		if (!result)
			return false;
		if (result.m_sStage == STAGE_STALE_NATIVE_NO_SAVE_VERIFY)
		{
			return result.m_bNoSaveStage
				&& result.m_bSavingDisabledBeforeClose
				&& result.m_bNoCheckpointRequested
				&& result.m_bNoSaveEventsObserved
				&& result.m_bActiveSaveUnchanged;
		}
		return !result.m_bNoSaveStage
			&& !result.m_bSavingDisabledBeforeClose
			&& !result.m_bNoCheckpointRequested
			&& !result.m_bNoSaveEventsObserved
			&& !result.m_bActiveSaveUnchanged;
	}

	protected static bool ValidateResultCarrierRelationship(
		HST_AdminCampaignResetPersistenceProofResult result,
		HST_AdminCampaignResetPersistenceProofCarrier carrier,
		out string evidence)
	{
		evidence = "admin reset result/carrier relationship rejected";
		if (!result || !carrier
			|| result.m_sSessionNonce != carrier.m_sSessionNonce
			|| result.m_sRunId != carrier.m_sRunId
			|| result.m_sPayloadNonce != carrier.m_sPayloadNonce)
			return false;
		if (!ValidateResultSource(result, carrier)
			|| !ValidateResultState(result, carrier)
			|| !ValidateResultCheckpoint(result, carrier)
			|| !ValidateResultResetRejection(result, carrier)
			|| !ValidateResultNoSave(result))
			return false;
		if (result.m_sStage == STAGE_RESET_COMMIT)
		{
			bool blockerExact = carrier.m_bBlockerCompletionReceived
				&& carrier.m_bBlockerNativeCommitSucceeded
				&& carrier.m_bBlockerProfileMirrorSaved
				&& carrier.m_bBlockerOnAfterSaveSucceeded
				&& carrier.m_bBlockerOnSaveCreatedObserved;
			bool resetExact = carrier.m_bResetRemovedSentinel
				&& carrier.m_bResetPreservedIdentity
				&& carrier.m_bResetAdvancedEpoch
				&& carrier.m_bResetRetainedSequenceFloors;
			if (!blockerExact || !resetExact)
				return false;
		}
		evidence = "admin reset result/carrier relationship exact";
		return true;
	}

	protected static bool ValidateResult(
		HST_AdminCampaignResetPersistenceProofResult result,
		out string evidence)
	{
		evidence = "admin reset persistence stage result rejected";
		if (!result || !ValidateNonce(result.m_sSessionNonce)
			|| !ValidateNonce(result.m_sStageNonce)
			|| !ValidateRunId(result.m_sRunId)
			|| !ValidateNonce(result.m_sPayloadNonce)
			|| !ValidateStage(result.m_sStage)
			|| result.m_iStageOrdinal
				!= ResolveStageOrdinal(result.m_sStage))
			return false;
		bool identityExact = result.m_sMagic == RESULT_MAGIC
			&& result.m_iVersion == AUTHORITY_VERSION
			&& result.m_sBuildSha == HST_BuildInfo.BUILD_SHA
			&& result.m_sBuildUtc == HST_BuildInfo.BUILD_UTC
			&& result.m_sBuildLabel == HST_BuildInfo.BUILD_LABEL;
		bool schemaExact = result.m_iCampaignSchemaVersion
				== HST_CampaignState.SCHEMA_VERSION
			&& result.m_iSettingsSchemaVersion
				== HST_RuntimeSettings.SCHEMA_VERSION
			&& ValidateWorldIdentity(result.m_sWorld, result.m_sWorld);
		bool expectationExact = result.m_sExpectedSource
				== ResolveExpectedSource(result.m_sStage)
			&& result.m_sExpectedSaveType
				== ResolveExpectedSaveType(result.m_sStage)
			&& result.m_sExpectedSaveName
				== ResolveExpectedSaveName(result.m_sStage)
			&& !result.m_sEvidence.IsEmpty();
		if (!identityExact || !schemaExact || !expectationExact)
			return false;
		if (!result.m_bSuccess)
		{
			evidence = "admin reset persistence failed-stage result exact";
			return true;
		}
		evidence = "admin reset persistence successful-stage result structurally exact";
		return true;
	}

	static bool SaveResult(
		HST_AdminCampaignResetPersistenceProofResult result,
		out string evidence)
	{
		if (!result || !ValidateResult(result, evidence))
			return false;
		HST_AdminCampaignResetPersistenceProofOwner owner;
		string ownerEvidence;
		if (!LoadAndValidateOwner(
			result.m_sSessionNonce,
			result.m_sRunId,
			result.m_sWorld,
			owner,
			ownerEvidence)
			|| owner.m_sPayloadNonce != result.m_sPayloadNonce)
		{
			evidence = "admin reset result owner rejected | "
				+ ownerEvidence;
			return false;
		}
		if (result.m_bSuccess)
		{
			HST_AdminCampaignResetPersistenceProofCarrier carrier;
			string carrierEvidence;
			if (!LoadCarrierFile(result.m_sRunId, carrier, carrierEvidence))
			{
				evidence += " | " + carrierEvidence;
				return false;
			}
			bool carrierValid;
			if (result.m_sStage == STAGE_PREPARE_OLD_CHECKPOINT)
			{
				carrierValid = ValidatePreparedCarrier(
					carrier,
					result.m_sSessionNonce,
					result.m_sRunId,
					result.m_sPayloadNonce,
					result.m_sWorld,
					carrierEvidence);
			}
			else
			{
				carrierValid = ValidateResetCarrier(
					carrier,
					result.m_sSessionNonce,
					result.m_sRunId,
					result.m_sPayloadNonce,
					result.m_sWorld,
					carrierEvidence);
			}
			string relationshipEvidence;
			if (!carrierValid || !ValidateResultCarrierRelationship(
				result,
				carrier,
				relationshipEvidence))
			{
				evidence += " | " + carrierEvidence
					+ " | " + relationshipEvidence;
				return false;
			}
		}

		string path = BuildResultPath(result.m_sRunId, result.m_sStage);
		if (path.IsEmpty())
			return false;
		if (FileIO.FileExists(path))
		{
			evidence = "admin reset persistence result storage is not fresh";
			return false;
		}
		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", result) || !context.SaveToFile(path))
		{
			evidence = "admin reset persistence result write failed";
			return false;
		}
		evidence = "admin reset persistence stage result saved";
		return true;
	}

	static bool LoadResult(
		string sessionNonce,
		string stageNonce,
		string runId,
		string payloadNonce,
		string stage,
		string expectedWorld,
		out HST_AdminCampaignResetPersistenceProofResult result,
		out string evidence)
	{
		result = null;
		evidence = "admin reset persistence stage result unavailable";
		string path = BuildResultPath(runId, stage);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		result = new HST_AdminCampaignResetPersistenceProofResult();
		if (!context.ReadValue("", result))
		{
			result = null;
			return false;
		}
		if (result.m_sSessionNonce != sessionNonce
			|| result.m_sStageNonce != stageNonce
			|| result.m_sRunId != runId
			|| result.m_sPayloadNonce != payloadNonce
			|| result.m_sStage != stage
			|| !ValidateWorldIdentity(result.m_sWorld, expectedWorld))
			return false;
		return ValidateResult(result, evidence);
	}
}
#endif
