// Fail-closed artifact and campaign-sentinel authority for the disposable
// five-process ordinary persistence proof. The coordinator performs engine
// save/load work; this service owns only exact portable evidence contracts.
class HST_OrdinaryCampaignPersistenceProofService
{
	static const string OWNER_MAGIC
		= "partisan_ordinary_campaign_persistence_owner_v1";
	static const string GUARD_MAGIC
		= "partisan_ordinary_campaign_persistence_guard_v1";
	static const string CARRIER_MAGIC
		= "partisan_ordinary_campaign_persistence_carrier_v1";
	static const string RESULT_MAGIC
		= "partisan_ordinary_campaign_persistence_result_v1";
	static const string END_BRIDGE_RECEIPT_MAGIC
		= "partisan_ordinary_campaign_end_bridge_receipt_v1";
	static const string OWNER_PURPOSE
		= "ordinary_campaign_persistence";
	static const int AUTHORITY_VERSION = 1;
	static const int EXPECTED_STAGE_COUNT = 5;
	static const int AUTOSAVE_SCHEDULER_INTERVAL_SECONDS = 60;
	static const int AUTOSAVE_SCHEDULER_DEBOUNCE_SECONDS = 120;
	static const int AUTOSAVE_SCHEDULER_REMARK_SECONDS = 30;

	static const string STAGE_AUTOSAVE_CHECKPOINT = "autosave_checkpoint";
	static const string STAGE_MANUAL_CHECKPOINT = "manual_checkpoint";
	static const string STAGE_SHUTDOWN_CHECKPOINT = "shutdown_checkpoint";
	static const string STAGE_NATIVE_SHUTDOWN_VERIFY
		= "native_shutdown_verify";
	static const string STAGE_PROFILE_FALLBACK_VERIFY
		= "profile_fallback_verify";

	static const string SOURCE_NEW_CAMPAIGN = "new_campaign";
	static const string SOURCE_NATIVE = "native";
	static const string SOURCE_PROFILE_FALLBACK = "profile_fallback";
	static const string SAVE_TYPE_AUTO = "auto";
	static const string SAVE_TYPE_MANUAL = "manual";
	static const string SAVE_TYPE_SHUTDOWN = "shutdown";
	static const string SAVE_TYPE_NONE = "none";
	static const string AUTOSAVE_NAME
		= "Partisan autosave";
	static const string MANUAL_SAVE_NAME
		= "Partisan manual checkpoint";
	static const string SHUTDOWN_SAVE_NAME
		= "Partisan controlled shutdown";

	static const string CANONICAL_WORLD
		= "Worlds/HST_Everon/HST_Everon.ent";
	static const string CANONICAL_WORLD_LOWER
		= "worlds/hst_everon/hst_everon.ent";
	static const string SENTINEL_TASK_ID
		= "hst_ordinary_campaign_persistence_proof_sentinel";
	static const string SENTINEL_TITLE
		= "Ordinary Campaign Persistence Sentinel";
	static const string SENTINEL_CATEGORY
		= "ordinary_campaign_persistence_proof";
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
		if (stage == STAGE_AUTOSAVE_CHECKPOINT)
			return 0;
		if (stage == STAGE_MANUAL_CHECKPOINT)
			return 1;
		if (stage == STAGE_SHUTDOWN_CHECKPOINT)
			return 2;
		if (stage == STAGE_NATIVE_SHUTDOWN_VERIFY)
			return 3;
		if (stage == STAGE_PROFILE_FALLBACK_VERIFY)
			return 4;
		return -1;
	}

	static string ResolveExpectedSource(string stage)
	{
		if (stage == STAGE_AUTOSAVE_CHECKPOINT)
			return SOURCE_NEW_CAMPAIGN;
		if (stage == STAGE_MANUAL_CHECKPOINT
			|| stage == STAGE_SHUTDOWN_CHECKPOINT
			|| stage == STAGE_NATIVE_SHUTDOWN_VERIFY)
			return SOURCE_NATIVE;
		if (stage == STAGE_PROFILE_FALLBACK_VERIFY)
			return SOURCE_PROFILE_FALLBACK;
		return "";
	}

	static int ResolveExpectedSentinelGeneration(string stage)
	{
		if (stage == STAGE_AUTOSAVE_CHECKPOINT)
			return 1;
		if (stage == STAGE_MANUAL_CHECKPOINT)
			return 2;
		if (stage == STAGE_SHUTDOWN_CHECKPOINT
			|| stage == STAGE_NATIVE_SHUTDOWN_VERIFY
			|| stage == STAGE_PROFILE_FALLBACK_VERIFY)
			return 3;
		return 0;
	}

	static bool StageCreatesSavePoint(string stage)
	{
		return stage == STAGE_AUTOSAVE_CHECKPOINT
			|| stage == STAGE_MANUAL_CHECKPOINT
			|| stage == STAGE_SHUTDOWN_CHECKPOINT;
	}

	static int ResolveExpectedRequestFlags(string stage)
	{
		if (stage == STAGE_SHUTDOWN_CHECKPOINT)
			return ESaveGameRequestFlags.BLOCKING;
		return 0;
	}

	static string ResolveExpectedSaveType(string stage)
	{
		if (stage == STAGE_AUTOSAVE_CHECKPOINT)
			return SAVE_TYPE_AUTO;
		if (stage == STAGE_MANUAL_CHECKPOINT)
			return SAVE_TYPE_MANUAL;
		if (stage == STAGE_SHUTDOWN_CHECKPOINT)
			return SAVE_TYPE_SHUTDOWN;
		if (stage == STAGE_NATIVE_SHUTDOWN_VERIFY
			|| stage == STAGE_PROFILE_FALLBACK_VERIFY)
			return SAVE_TYPE_NONE;
		return "";
	}

	static string ResolveSaveTypeLabel(ESaveGameType saveType)
	{
		if (saveType == ESaveGameType.AUTO)
			return SAVE_TYPE_AUTO;
		if (saveType == ESaveGameType.MANUAL)
			return SAVE_TYPE_MANUAL;
		if (saveType == ESaveGameType.SHUTDOWN)
			return SAVE_TYPE_SHUTDOWN;
		return "";
	}

	static bool TryResolveExpectedSaveType(
		string stage,
		out ESaveGameType saveType)
	{
		saveType = ESaveGameType.MANUAL;
		if (stage == STAGE_AUTOSAVE_CHECKPOINT)
		{
			saveType = ESaveGameType.AUTO;
			return true;
		}
		if (stage == STAGE_MANUAL_CHECKPOINT)
		{
			saveType = ESaveGameType.MANUAL;
			return true;
		}
		if (stage == STAGE_SHUTDOWN_CHECKPOINT)
		{
			saveType = ESaveGameType.SHUTDOWN;
			return true;
		}
		return false;
	}

	static string ResolveExpectedSaveName(string stage)
	{
		if (stage == STAGE_AUTOSAVE_CHECKPOINT)
			return AUTOSAVE_NAME;
		if (stage == STAGE_MANUAL_CHECKPOINT)
			return MANUAL_SAVE_NAME;
		if (stage == STAGE_SHUTDOWN_CHECKPOINT)
			return SHUTDOWN_SAVE_NAME;
		return "";
	}

	static string ResolveExpectedActiveSaveType(string stage)
	{
		if (stage == STAGE_MANUAL_CHECKPOINT)
			return SAVE_TYPE_AUTO;
		if (stage == STAGE_SHUTDOWN_CHECKPOINT)
			return SAVE_TYPE_MANUAL;
		if (stage == STAGE_NATIVE_SHUTDOWN_VERIFY)
			return SAVE_TYPE_SHUTDOWN;
		return "";
	}

	static string ResolveExpectedActiveSaveName(string stage)
	{
		if (stage == STAGE_MANUAL_CHECKPOINT)
			return AUTOSAVE_NAME;
		if (stage == STAGE_SHUTDOWN_CHECKPOINT)
			return MANUAL_SAVE_NAME;
		if (stage == STAGE_NATIVE_SHUTDOWN_VERIFY)
			return SHUTDOWN_SAVE_NAME;
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
			+ "/HST_OrdinaryCampaignPersistenceProof_" + safeRunId
			+ ".owner.json";
	}

	protected static string BuildGuardPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_OrdinaryCampaignPersistenceProof_" + safeRunId
			+ ".guard.json";
	}

	protected static string BuildCarrierPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_OrdinaryCampaignPersistenceProof_" + safeRunId
			+ ".carrier.json";
	}

	protected static string BuildResultPath(string runId, string stage)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty() || !ValidateStage(stage))
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_OrdinaryCampaignPersistenceProof_" + safeRunId
			+ "." + stage + ".json";
	}

	static string BuildEndBridgeReceiptPath(string runId, string stage)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty() || stage != STAGE_SHUTDOWN_CHECKPOINT)
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_OrdinaryCampaignPersistenceProof_" + safeRunId
			+ "." + stage + ".end_bridge.json";
	}

	protected static bool ValidateOwner(
		HST_OrdinaryCampaignPersistenceOwner owner,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedWorld,
		out string evidence)
	{
		evidence = "ordinary persistence profile owner rejected";
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
		evidence = "ordinary persistence profile owner exact";
		return true;
	}

	static bool LoadAndValidateOwner(
		string sessionNonce,
		string runId,
		string expectedWorld,
		out HST_OrdinaryCampaignPersistenceOwner owner,
		out string evidence)
	{
		owner = null;
		evidence = "ordinary persistence profile owner unavailable";
		string path = BuildOwnerPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		owner = new HST_OrdinaryCampaignPersistenceOwner();
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
		HST_OrdinaryCampaignPersistenceGuard guard,
		string expectedSessionNonce,
		string expectedStageNonce,
		string expectedRunId,
		string expectedPayloadNonce,
		string expectedStage,
		string expectedWorld,
		out string evidence)
	{
		evidence = "ordinary persistence one-use stage lease rejected";
		int expectedStageOrdinal = ResolveStageOrdinal(expectedStage);
		if (!guard || !ValidateNonce(expectedSessionNonce)
			|| !ValidateNonce(expectedStageNonce)
			|| !ValidateRunId(expectedRunId)
			|| !ValidateNonce(expectedPayloadNonce)
			|| expectedStageOrdinal < 0 || expectedWorld.IsEmpty())
			return false;
		if (guard.m_sMagic != GUARD_MAGIC
			|| guard.m_iVersion != AUTHORITY_VERSION
			|| guard.m_sSessionNonce != expectedSessionNonce
			|| guard.m_sStageNonce != expectedStageNonce
			|| guard.m_sRunId != expectedRunId
			|| SanitizeRunId(guard.m_sRunId) != guard.m_sRunId
			|| guard.m_sPayloadNonce != expectedPayloadNonce
			|| guard.m_sRequestedStage != expectedStage
			|| guard.m_iStageOrdinal != expectedStageOrdinal)
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
			|| guard.m_iExpectedSentinelGeneration
				!= ResolveExpectedSentinelGeneration(expectedStage)
			|| guard.m_sExpectedSaveType
				!= ResolveExpectedSaveType(expectedStage)
			|| guard.m_sExpectedSaveName
				!= ResolveExpectedSaveName(expectedStage)
			|| guard.m_bAllowCanonicalCampaignOverwrite
				!= StageCreatesSavePoint(expectedStage))
			return false;
		evidence = "ordinary persistence one-use stage lease exact";
		return true;
	}

	static bool LoadValidateAndConsumeGuard(
		string sessionNonce,
		string stageNonce,
		string runId,
		string payloadNonce,
		string stage,
		string expectedWorld,
		out HST_OrdinaryCampaignPersistenceGuard guard,
		out string evidence)
	{
		guard = null;
		evidence = "ordinary persistence one-use stage lease unavailable";
		string path = BuildGuardPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		guard = new HST_OrdinaryCampaignPersistenceGuard();
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
				= "ordinary persistence stage lease disappeared before consumption";
			return false;
		}
		FileIO.DeleteFile(path);
		if (FileIO.FileExists(path))
		{
			evidence
				= "ordinary persistence stage lease could not be consumed";
			return false;
		}
		evidence = "ordinary persistence one-use stage lease consumed";
		return true;
	}

	static bool CreateInitialCarrier(
		HST_OrdinaryCampaignPersistenceOwner owner,
		string expectedWorld,
		out HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		carrier = null;
		evidence = "ordinary persistence initial carrier rejected";
		string ownerEvidence;
		if (!owner || !ValidateOwner(
			owner,
			owner.m_sSessionNonce,
			owner.m_sRunId,
			expectedWorld,
			ownerEvidence))
		{
			evidence = evidence + " | " + ownerEvidence;
			return false;
		}

		carrier = new HST_OrdinaryCampaignPersistenceCarrier();
		carrier.m_sMagic = CARRIER_MAGIC;
		carrier.m_iVersion = AUTHORITY_VERSION;
		carrier.m_sSessionNonce = owner.m_sSessionNonce;
		carrier.m_sRunId = owner.m_sRunId;
		carrier.m_sPayloadNonce = owner.m_sPayloadNonce;
		carrier.m_sBuildSha = HST_BuildInfo.BUILD_SHA;
		carrier.m_sBuildUtc = HST_BuildInfo.BUILD_UTC;
		carrier.m_sBuildLabel = HST_BuildInfo.BUILD_LABEL;
		carrier.m_iCampaignSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		carrier.m_iSettingsSchemaVersion = HST_RuntimeSettings.SCHEMA_VERSION;
		carrier.m_sWorld = CANONICAL_WORLD;
		carrier.m_sSentinelTaskId = SENTINEL_TASK_ID;
		carrier.m_iCurrentSentinelGeneration = 0;
		evidence = "ordinary persistence initial carrier created";
		return true;
	}

	protected static string BuildSentinelDescription(
		string payloadNonce,
		int generation)
	{
		return string.Format(
			"payload %1 | generation %2",
			payloadNonce,
			generation);
	}

	static string BuildSentinelFingerprint(
		string payloadNonce,
		int generation)
	{
		if (!ValidateNonce(payloadNonce) || generation < 1 || generation > 3)
			return "";
		string canonical = string.Format(
			"%1|%2|%3|%4|%5|0|0|0|false|false|false",
			SENTINEL_TASK_ID,
			payloadNonce,
			SENTINEL_TITLE,
			BuildSentinelDescription(payloadNonce, generation),
			SENTINEL_CATEGORY);
		return string.Format(
			"ocp1_%1_%2",
			canonical.Hash(),
			(canonical + "|secondary").Hash());
	}

	protected static HST_CampaignTaskState FindUniqueSentinel(
		HST_CampaignState state,
		out int matchCount)
	{
		matchCount = 0;
		HST_CampaignTaskState result;
		if (!state || !state.m_aCampaignTasks)
			return null;
		foreach (HST_CampaignTaskState task : state.m_aCampaignTasks)
		{
			if (!task || task.m_sTaskId != SENTINEL_TASK_ID)
				continue;
			matchCount++;
			result = task;
		}
		if (matchCount != 1)
			return null;
		return result;
	}

	static bool ValidateExpectedSentinel(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		int expectedGeneration,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "";
		evidence = "ordinary persistence campaign sentinel rejected";
		if (!state || !carrier || expectedGeneration < 1
			|| expectedGeneration > 3
			|| !ValidateNonce(carrier.m_sPayloadNonce)
			|| carrier.m_sSentinelTaskId != SENTINEL_TASK_ID)
			return false;

		int matchCount;
		HST_CampaignTaskState task = FindUniqueSentinel(state, matchCount);
		if (!task)
		{
			evidence = string.Format(
				"ordinary persistence sentinel cardinality rejected | count %1",
				matchCount);
			return false;
		}

		bool zeroPosition = Math.AbsFloat(task.m_vPosition[0]) < 0.001
			&& Math.AbsFloat(task.m_vPosition[1]) < 0.001
			&& Math.AbsFloat(task.m_vPosition[2]) < 0.001;
		if (task.m_sLinkedId != carrier.m_sPayloadNonce
			|| task.m_sTitle != SENTINEL_TITLE
			|| task.m_sDescription
				!= BuildSentinelDescription(
					carrier.m_sPayloadNonce,
					expectedGeneration)
			|| task.m_sCategory != SENTINEL_CATEGORY || !zeroPosition
			|| task.m_bActive || task.m_bSucceeded || task.m_bFailed)
			return false;

		fingerprint = BuildSentinelFingerprint(
			carrier.m_sPayloadNonce,
			expectedGeneration);
		string carrierFingerprint
			= ResolveCarrierSentinelFingerprint(carrier, expectedGeneration);
		if (fingerprint.IsEmpty() || carrierFingerprint != fingerprint)
		{
			evidence
				= "ordinary persistence sentinel generation fingerprint rejected";
			return false;
		}

		evidence = string.Format(
			"ordinary persistence sentinel generation %1 exact | fingerprint %2",
			expectedGeneration,
			fingerprint);
		return true;
	}

	static bool InstallExpectedSentinel(
		HST_CampaignState state,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		int generation,
		out string fingerprint,
		out string evidence)
	{
		fingerprint = "";
		evidence = "ordinary persistence sentinel installation rejected";
		if (!state || !carrier || generation < 1 || generation > 3
			|| !state.m_aCampaignTasks
			|| carrier.m_iCurrentSentinelGeneration != generation - 1)
			return false;

		HST_CampaignTaskState task;
		if (generation == 1)
		{
			int existingCount;
			FindUniqueSentinel(state, existingCount);
			if (existingCount != 0)
			{
				evidence
					= "ordinary persistence generation 1 requires no prior sentinel";
				return false;
			}
			task = new HST_CampaignTaskState();
			task.m_sTaskId = SENTINEL_TASK_ID;
			state.m_aCampaignTasks.Insert(task);
		}
		else
		{
			string priorFingerprint;
			string priorEvidence;
			if (!ValidateExpectedSentinel(
				state,
				carrier,
				generation - 1,
				priorFingerprint,
				priorEvidence))
			{
				evidence = evidence + " | " + priorEvidence;
				return false;
			}
			int existingCount;
			task = FindUniqueSentinel(state, existingCount);
			if (!task)
				return false;
		}

		task.m_sLinkedId = carrier.m_sPayloadNonce;
		task.m_sTitle = SENTINEL_TITLE;
		task.m_sDescription = BuildSentinelDescription(
			carrier.m_sPayloadNonce,
			generation);
		task.m_sCategory = SENTINEL_CATEGORY;
		task.m_vPosition = vector.Zero;
		task.m_bActive = false;
		task.m_bSucceeded = false;
		task.m_bFailed = false;

		fingerprint = BuildSentinelFingerprint(
			carrier.m_sPayloadNonce,
			generation);
		if (generation == 1)
			carrier.m_sGeneration1SentinelFingerprint = fingerprint;
		else if (generation == 2)
			carrier.m_sGeneration2SentinelFingerprint = fingerprint;
		else
			carrier.m_sGeneration3SentinelFingerprint = fingerprint;
		carrier.m_iCurrentSentinelGeneration = generation;

		if (!ValidateExpectedSentinel(
			state,
			carrier,
			generation,
			fingerprint,
			evidence))
			return false;
		evidence = string.Format(
			"ordinary persistence sentinel generation %1 installed | fingerprint %2",
			generation,
			fingerprint);
		return true;
	}

	protected static string ResolveCarrierSentinelFingerprint(
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		int generation)
	{
		if (!carrier)
			return "";
		if (generation == 1)
			return carrier.m_sGeneration1SentinelFingerprint;
		if (generation == 2)
			return carrier.m_sGeneration2SentinelFingerprint;
		if (generation == 3)
			return carrier.m_sGeneration3SentinelFingerprint;
		return "";
	}

	protected static string ResolveCarrierProfileFallbackFingerprint(
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		int generation)
	{
		if (!carrier)
			return "";
		if (generation == 1)
			return carrier.m_sGeneration1ProfileFallbackFingerprint;
		if (generation == 2)
			return carrier.m_sGeneration2ProfileFallbackFingerprint;
		if (generation == 3)
			return carrier.m_sGeneration3ProfileFallbackFingerprint;
		return "";
	}

	protected static bool ValidateGenerationOneCarrier(
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		evidence = "ordinary persistence generation 1 carrier rejected";
		string expectedSentinel = BuildSentinelFingerprint(
			carrier.m_sPayloadNonce,
			1);
		if (carrier.m_sGeneration1SentinelFingerprint != expectedSentinel
			|| !carrier.m_sGeneration2SentinelFingerprint.IsEmpty()
			|| !carrier.m_sGeneration3SentinelFingerprint.IsEmpty()
			|| !UUID.IsUUID(carrier.m_sAutosaveSavePointId))
			return false;
		if (!carrier.m_sManualSavePointId.IsEmpty()
			|| !carrier.m_sShutdownSavePointId.IsEmpty()
			|| !carrier.m_sPriorSavePointId.IsEmpty()
			|| carrier.m_sCurrentSavePointId
				!= carrier.m_sAutosaveSavePointId)
			return false;
		if (carrier.m_sAutosaveSource != SOURCE_NEW_CAMPAIGN
			|| !carrier.m_sAutosaveSourceFingerprint.IsEmpty()
			|| carrier.m_bAutosaveWasDataLoaded
			|| carrier.m_bAutosaveNativeRecordPresent
			|| carrier.m_bAutosaveNativeRecordValid)
			return false;
		if (carrier.m_sAutosaveCreatedSaveType != SAVE_TYPE_AUTO
			|| carrier.m_sAutosaveCreatedSaveName != AUTOSAVE_NAME
			|| !carrier.m_sManualSource.IsEmpty()
			|| !carrier.m_sManualSourceFingerprint.IsEmpty())
			return false;
		if (carrier.m_bManualWasDataLoaded
			|| carrier.m_bManualNativeRecordPresent
			|| carrier.m_bManualNativeRecordValid
			|| !carrier.m_sShutdownSource.IsEmpty()
			|| !carrier.m_sShutdownSourceFingerprint.IsEmpty())
			return false;
		if (carrier.m_bShutdownWasDataLoaded
			|| carrier.m_bShutdownNativeRecordPresent
			|| carrier.m_bShutdownNativeRecordValid
			|| !carrier.m_sManualCreatedSaveType.IsEmpty()
			|| !carrier.m_sManualCreatedSaveName.IsEmpty())
			return false;
		if (!carrier.m_sShutdownCreatedSaveType.IsEmpty()
			|| !carrier.m_sShutdownCreatedSaveName.IsEmpty()
			|| carrier.m_sGeneration1ProfileFallbackFingerprint.IsEmpty()
			|| !carrier.m_bGeneration1ProfileFallbackExact)
			return false;
		if (!carrier.m_sGeneration2ProfileFallbackFingerprint.IsEmpty()
			|| !carrier.m_sGeneration3ProfileFallbackFingerprint.IsEmpty()
			|| carrier.m_bGeneration2ProfileFallbackExact
			|| carrier.m_bGeneration3ProfileFallbackExact
			|| carrier.m_sLatestProfileFallbackFingerprint
				!= carrier.m_sGeneration1ProfileFallbackFingerprint)
			return false;
		evidence = "ordinary persistence generation 1 carrier exact";
		return true;
	}

	protected static bool ValidateGenerationTwoCarrier(
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		evidence = "ordinary persistence generation 2 carrier rejected";
		string priorEvidence;
		if (!ValidateGenerationOneCarrierSnapshot(carrier, priorEvidence))
		{
			evidence = evidence + " | " + priorEvidence;
			return false;
		}
		string expectedSentinel = BuildSentinelFingerprint(
			carrier.m_sPayloadNonce,
			2);
		if (carrier.m_sGeneration2SentinelFingerprint != expectedSentinel
			|| !carrier.m_sGeneration3SentinelFingerprint.IsEmpty()
			|| !UUID.IsUUID(carrier.m_sManualSavePointId)
			|| !carrier.m_sShutdownSavePointId.IsEmpty()
			|| carrier.m_sManualSavePointId == carrier.m_sAutosaveSavePointId)
			return false;
		if (carrier.m_sPriorSavePointId
				!= carrier.m_sAutosaveSavePointId
			|| carrier.m_sCurrentSavePointId
				!= carrier.m_sManualSavePointId
			|| carrier.m_sManualSource != SOURCE_NATIVE
			|| carrier.m_sManualSourceFingerprint
				!= carrier.m_sGeneration1ProfileFallbackFingerprint)
			return false;
		if (!carrier.m_bManualWasDataLoaded
			|| !carrier.m_bManualNativeRecordPresent
			|| !carrier.m_bManualNativeRecordValid
			|| carrier.m_sManualCreatedSaveType != SAVE_TYPE_MANUAL
			|| carrier.m_sManualCreatedSaveName != MANUAL_SAVE_NAME)
			return false;
		if (!carrier.m_sShutdownSource.IsEmpty()
			|| !carrier.m_sShutdownSourceFingerprint.IsEmpty()
			|| carrier.m_bShutdownWasDataLoaded
			|| carrier.m_bShutdownNativeRecordPresent
			|| carrier.m_bShutdownNativeRecordValid)
			return false;
		if (!carrier.m_sShutdownCreatedSaveType.IsEmpty()
			|| !carrier.m_sShutdownCreatedSaveName.IsEmpty()
			|| carrier.m_sGeneration2ProfileFallbackFingerprint.IsEmpty()
			|| !carrier.m_bGeneration2ProfileFallbackExact)
			return false;
		if (!carrier.m_sGeneration3ProfileFallbackFingerprint.IsEmpty()
			|| carrier.m_bGeneration3ProfileFallbackExact
			|| carrier.m_sLatestProfileFallbackFingerprint
				!= carrier.m_sGeneration2ProfileFallbackFingerprint)
			return false;
		evidence = "ordinary persistence generation 2 carrier exact";
		return true;
	}

	protected static bool ValidateGenerationThreeCarrier(
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		evidence = "ordinary persistence generation 3 carrier rejected";
		string priorEvidence;
		if (!ValidateGenerationTwoCarrierSnapshot(carrier, priorEvidence))
		{
			evidence = evidence + " | " + priorEvidence;
			return false;
		}
		string expectedSentinel = BuildSentinelFingerprint(
			carrier.m_sPayloadNonce,
			3);
		if (carrier.m_sGeneration3SentinelFingerprint != expectedSentinel
			|| !UUID.IsUUID(carrier.m_sShutdownSavePointId)
			|| carrier.m_sShutdownSavePointId == carrier.m_sAutosaveSavePointId
			|| carrier.m_sShutdownSavePointId == carrier.m_sManualSavePointId)
			return false;
		if (carrier.m_sPriorSavePointId
				!= carrier.m_sManualSavePointId
			|| carrier.m_sCurrentSavePointId
				!= carrier.m_sShutdownSavePointId
			|| carrier.m_sShutdownSource != SOURCE_NATIVE
			|| carrier.m_sShutdownSourceFingerprint
				!= carrier.m_sGeneration2ProfileFallbackFingerprint)
			return false;
		if (!carrier.m_bShutdownWasDataLoaded
			|| !carrier.m_bShutdownNativeRecordPresent
			|| !carrier.m_bShutdownNativeRecordValid
			|| carrier.m_sShutdownCreatedSaveType != SAVE_TYPE_SHUTDOWN
			|| carrier.m_sShutdownCreatedSaveName != SHUTDOWN_SAVE_NAME)
			return false;
		if (carrier.m_sGeneration3ProfileFallbackFingerprint.IsEmpty()
			|| !carrier.m_bGeneration3ProfileFallbackExact
			|| carrier.m_sLatestProfileFallbackFingerprint
				!= carrier.m_sGeneration3ProfileFallbackFingerprint)
			return false;
		evidence = "ordinary persistence generation 3 carrier exact";
		return true;
	}

	// Validate generation 1 without its mutable current/prior/latest aliases so
	// later carrier generations independently retain the whole earlier chain.
	protected static bool ValidateGenerationOneCarrierSnapshot(
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		evidence = "ordinary persistence retained generation 1 rejected";
		if (carrier.m_sGeneration1SentinelFingerprint
				!= BuildSentinelFingerprint(carrier.m_sPayloadNonce, 1)
			|| !UUID.IsUUID(carrier.m_sAutosaveSavePointId)
			|| carrier.m_sAutosaveSource != SOURCE_NEW_CAMPAIGN
			|| !carrier.m_sAutosaveSourceFingerprint.IsEmpty())
			return false;
		if (carrier.m_bAutosaveWasDataLoaded
			|| carrier.m_bAutosaveNativeRecordPresent
			|| carrier.m_bAutosaveNativeRecordValid
			|| carrier.m_sAutosaveCreatedSaveType != SAVE_TYPE_AUTO
			|| carrier.m_sAutosaveCreatedSaveName != AUTOSAVE_NAME)
			return false;
		if (carrier.m_sGeneration1ProfileFallbackFingerprint.IsEmpty()
			|| !carrier.m_bGeneration1ProfileFallbackExact)
			return false;
		evidence = "ordinary persistence retained generation 1 exact";
		return true;
	}

	protected static bool ValidateGenerationTwoCarrierSnapshot(
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		evidence = "ordinary persistence retained generation 2 rejected";
		string firstEvidence;
		if (!ValidateGenerationOneCarrierSnapshot(carrier, firstEvidence))
			return false;
		if (carrier.m_sGeneration2SentinelFingerprint
				!= BuildSentinelFingerprint(carrier.m_sPayloadNonce, 2)
			|| !UUID.IsUUID(carrier.m_sManualSavePointId)
			|| carrier.m_sManualSavePointId == carrier.m_sAutosaveSavePointId
			|| carrier.m_sManualSource != SOURCE_NATIVE
			|| carrier.m_sManualSourceFingerprint
				!= carrier.m_sGeneration1ProfileFallbackFingerprint)
			return false;
		if (!carrier.m_bManualWasDataLoaded
			|| !carrier.m_bManualNativeRecordPresent
			|| !carrier.m_bManualNativeRecordValid
			|| carrier.m_sManualCreatedSaveType != SAVE_TYPE_MANUAL
			|| carrier.m_sManualCreatedSaveName != MANUAL_SAVE_NAME)
			return false;
		if (carrier.m_sGeneration2ProfileFallbackFingerprint.IsEmpty()
			|| !carrier.m_bGeneration2ProfileFallbackExact)
			return false;
		evidence = "ordinary persistence retained generation 2 exact";
		return true;
	}

	protected static bool ValidateCarrier(
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedPayloadNonce,
		string expectedWorld,
		int expectedGeneration,
		out string evidence)
	{
		evidence = "ordinary persistence carrier rejected";
		if (!carrier || !ValidateNonce(expectedSessionNonce)
			|| !ValidateRunId(expectedRunId)
			|| !ValidateNonce(expectedPayloadNonce)
			|| expectedWorld.IsEmpty() || expectedGeneration < 1
			|| expectedGeneration > 3)
			return false;
		if (carrier.m_sMagic != CARRIER_MAGIC
			|| carrier.m_iVersion != AUTHORITY_VERSION
			|| carrier.m_sSessionNonce != expectedSessionNonce
			|| carrier.m_sRunId != expectedRunId
			|| SanitizeRunId(carrier.m_sRunId) != carrier.m_sRunId
			|| carrier.m_sPayloadNonce != expectedPayloadNonce
			|| carrier.m_sSentinelTaskId != SENTINEL_TASK_ID
			|| carrier.m_iCurrentSentinelGeneration != expectedGeneration)
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

		bool valid;
		if (expectedGeneration == 1)
			valid = ValidateGenerationOneCarrier(carrier, evidence);
		else if (expectedGeneration == 2)
			valid = ValidateGenerationTwoCarrier(carrier, evidence);
		else
			valid = ValidateGenerationThreeCarrier(carrier, evidence);
		return valid;
	}

	protected static bool LoadCarrierFile(
		string runId,
		out HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		carrier = null;
		evidence = "ordinary persistence carrier unavailable";
		string path = BuildCarrierPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		carrier = new HST_OrdinaryCampaignPersistenceCarrier();
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
		out HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		carrier = null;
		evidence = "ordinary persistence prior-stage carrier rejected";
		int expectedGeneration;
		if (stage == STAGE_MANUAL_CHECKPOINT)
			expectedGeneration = 1;
		else if (stage == STAGE_SHUTDOWN_CHECKPOINT)
			expectedGeneration = 2;
		else if (stage == STAGE_NATIVE_SHUTDOWN_VERIFY
			|| stage == STAGE_PROFILE_FALLBACK_VERIFY)
			expectedGeneration = 3;
		else
		{
			evidence
				= "ordinary persistence carrier is required only after stage 1";
			return false;
		}

		if (!LoadCarrierFile(runId, carrier, evidence))
			return false;
		return ValidateCarrier(
			carrier,
			sessionNonce,
			runId,
			payloadNonce,
			expectedWorld,
			expectedGeneration,
			evidence);
	}

	protected static bool ValidateRetainedCarrierPrefix(
		HST_OrdinaryCampaignPersistenceCarrier previous,
		HST_OrdinaryCampaignPersistenceCarrier current,
		int completedGeneration,
		out string evidence)
	{
		evidence = "ordinary persistence retained carrier prefix rejected";
		if (!previous || !current || completedGeneration < 1
			|| completedGeneration > 2)
			return false;
		if (previous.m_sSessionNonce != current.m_sSessionNonce
			|| previous.m_sRunId != current.m_sRunId
			|| previous.m_sPayloadNonce != current.m_sPayloadNonce
			|| previous.m_sWorld != current.m_sWorld)
			return false;
		if (previous.m_sGeneration1SentinelFingerprint
				!= current.m_sGeneration1SentinelFingerprint
			|| previous.m_sAutosaveSavePointId
				!= current.m_sAutosaveSavePointId
			|| previous.m_sAutosaveSource != current.m_sAutosaveSource
			|| previous.m_sAutosaveSourceFingerprint
				!= current.m_sAutosaveSourceFingerprint)
			return false;
		if (previous.m_bAutosaveWasDataLoaded
				!= current.m_bAutosaveWasDataLoaded
			|| previous.m_bAutosaveNativeRecordPresent
				!= current.m_bAutosaveNativeRecordPresent
			|| previous.m_bAutosaveNativeRecordValid
				!= current.m_bAutosaveNativeRecordValid)
			return false;
		if (previous.m_sAutosaveCreatedSaveType
				!= current.m_sAutosaveCreatedSaveType
			|| previous.m_sAutosaveCreatedSaveName
				!= current.m_sAutosaveCreatedSaveName
			|| previous.m_sGeneration1ProfileFallbackFingerprint
				!= current.m_sGeneration1ProfileFallbackFingerprint
			|| previous.m_bGeneration1ProfileFallbackExact
				!= current.m_bGeneration1ProfileFallbackExact)
			return false;

		if (completedGeneration == 2)
		{
			if (previous.m_sGeneration2SentinelFingerprint
					!= current.m_sGeneration2SentinelFingerprint
				|| previous.m_sManualSavePointId
					!= current.m_sManualSavePointId
				|| previous.m_sManualSource != current.m_sManualSource
				|| previous.m_sManualSourceFingerprint
					!= current.m_sManualSourceFingerprint)
				return false;
			if (previous.m_bManualWasDataLoaded
					!= current.m_bManualWasDataLoaded
				|| previous.m_bManualNativeRecordPresent
					!= current.m_bManualNativeRecordPresent
				|| previous.m_bManualNativeRecordValid
					!= current.m_bManualNativeRecordValid)
				return false;
			if (previous.m_sManualCreatedSaveType
					!= current.m_sManualCreatedSaveType
				|| previous.m_sManualCreatedSaveName
					!= current.m_sManualCreatedSaveName
				|| previous.m_sGeneration2ProfileFallbackFingerprint
					!= current.m_sGeneration2ProfileFallbackFingerprint
				|| previous.m_bGeneration2ProfileFallbackExact
					!= current.m_bGeneration2ProfileFallbackExact)
				return false;
		}

		evidence = string.Format(
			"ordinary persistence retained carrier generation %1 exact",
			completedGeneration);
		return true;
	}

	static bool SaveCarrier(
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		if (!carrier || !ValidateCarrier(
			carrier,
			carrier.m_sSessionNonce,
			carrier.m_sRunId,
			carrier.m_sPayloadNonce,
			carrier.m_sWorld,
			carrier.m_iCurrentSentinelGeneration,
			evidence))
			return false;
		HST_OrdinaryCampaignPersistenceOwner owner;
		string ownerEvidence;
		if (!LoadAndValidateOwner(
			carrier.m_sSessionNonce,
			carrier.m_sRunId,
			carrier.m_sWorld,
			owner,
			ownerEvidence)
			|| owner.m_sPayloadNonce != carrier.m_sPayloadNonce)
		{
			evidence = "ordinary persistence carrier owner rejected | "
				+ ownerEvidence;
			return false;
		}
		string path = BuildCarrierPath(carrier.m_sRunId);
		if (path.IsEmpty())
			return false;
		if (carrier.m_iCurrentSentinelGeneration == 1
			&& FileIO.FileExists(path))
		{
			evidence
				= "ordinary persistence generation 1 carrier storage is not fresh";
			return false;
		}
		if (carrier.m_iCurrentSentinelGeneration > 1)
		{
			HST_OrdinaryCampaignPersistenceCarrier previous;
			string previousEvidence;
			int previousGeneration
				= carrier.m_iCurrentSentinelGeneration - 1;
			if (!LoadCarrierFile(
				carrier.m_sRunId,
				previous,
				previousEvidence)
				|| !ValidateCarrier(
					previous,
					carrier.m_sSessionNonce,
					carrier.m_sRunId,
					carrier.m_sPayloadNonce,
					carrier.m_sWorld,
					previousGeneration,
					previousEvidence)
				|| !ValidateRetainedCarrierPrefix(
					previous,
					carrier,
					previousGeneration,
					previousEvidence))
			{
				evidence = "ordinary persistence prior carrier rejected | "
					+ previousEvidence;
				return false;
			}
		}
		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", carrier) || !context.SaveToFile(path))
		{
			evidence = "ordinary persistence carrier write failed";
			return false;
		}
		evidence = "ordinary persistence carrier saved";
		return true;
	}

	protected static bool ValidateResultCarrierRelationship(
		HST_OrdinaryCampaignPersistenceResult result,
		HST_OrdinaryCampaignPersistenceCarrier carrier,
		out string evidence)
	{
		evidence = "ordinary persistence result/carrier relationship rejected";
		int generation = ResolveExpectedSentinelGeneration(result.m_sStage);
		if (!carrier || result.m_iSentinelGeneration != generation
			|| result.m_sSentinelFingerprint
				!= ResolveCarrierSentinelFingerprint(carrier, generation)
			|| result.m_sExpectedSentinelFingerprint
				!= result.m_sSentinelFingerprint
			|| result.m_sExpectedProfileFallbackFingerprint
				!= ResolveCarrierProfileFallbackFingerprint(carrier, generation)
			|| result.m_sProfileFallbackReadBackFingerprint
				!= result.m_sExpectedProfileFallbackFingerprint)
			return false;

		if (result.m_sStage == STAGE_AUTOSAVE_CHECKPOINT)
		{
			if (!result.m_sExpectedPriorSavePointId.IsEmpty()
				|| result.m_sExpectedCurrentSavePointId
					!= carrier.m_sAutosaveSavePointId
				|| result.m_sCreatedSavePointId
					!= carrier.m_sAutosaveSavePointId
				|| result.m_sSource != carrier.m_sAutosaveSource
				|| result.m_sSourceFingerprint
					!= carrier.m_sAutosaveSourceFingerprint)
				return false;
		}
		else if (result.m_sStage == STAGE_MANUAL_CHECKPOINT)
		{
			if (result.m_sExpectedPriorSavePointId
					!= carrier.m_sAutosaveSavePointId
				|| result.m_sExpectedCurrentSavePointId
					!= carrier.m_sManualSavePointId
				|| result.m_sCreatedSavePointId
					!= carrier.m_sManualSavePointId
				|| result.m_sSource != carrier.m_sManualSource
				|| result.m_sSourceFingerprint
					!= carrier.m_sManualSourceFingerprint)
				return false;
		}
		else if (result.m_sStage == STAGE_SHUTDOWN_CHECKPOINT)
		{
			if (result.m_sExpectedPriorSavePointId
					!= carrier.m_sManualSavePointId
				|| result.m_sExpectedCurrentSavePointId
					!= carrier.m_sShutdownSavePointId
				|| result.m_sCreatedSavePointId
					!= carrier.m_sShutdownSavePointId
				|| result.m_sSource != carrier.m_sShutdownSource
				|| result.m_sSourceFingerprint
					!= carrier.m_sShutdownSourceFingerprint)
				return false;
		}
		else if (result.m_sStage == STAGE_NATIVE_SHUTDOWN_VERIFY)
		{
			if (result.m_sExpectedPriorSavePointId
					!= carrier.m_sShutdownSavePointId
				|| result.m_sExpectedCurrentSavePointId
					!= carrier.m_sShutdownSavePointId
				|| result.m_sSourceFingerprint
					!= carrier.m_sGeneration3ProfileFallbackFingerprint)
				return false;
		}
		else if (result.m_sStage == STAGE_PROFILE_FALLBACK_VERIFY)
		{
			if (result.m_sExpectedPriorSavePointId
					!= carrier.m_sShutdownSavePointId
				|| !result.m_sExpectedCurrentSavePointId.IsEmpty()
				|| result.m_sSourceFingerprint
					!= carrier.m_sGeneration3ProfileFallbackFingerprint)
				return false;
		}

		evidence = "ordinary persistence result/carrier relationship exact";
		return true;
	}

	static bool ValidateResult(
		HST_OrdinaryCampaignPersistenceResult result,
		out string evidence)
	{
		evidence = "ordinary persistence stage result rejected";
		if (!result || !ValidateNonce(result.m_sSessionNonce)
			|| !ValidateNonce(result.m_sStageNonce)
			|| !ValidateNonce(result.m_sPayloadNonce)
			|| !ValidateRunId(result.m_sRunId)
			|| !ValidateStage(result.m_sStage)
			|| result.m_iStageOrdinal
				!= ResolveStageOrdinal(result.m_sStage))
			return false;
		if (result.m_sMagic != RESULT_MAGIC
			|| result.m_iVersion != AUTHORITY_VERSION
			|| result.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| result.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| result.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL
			|| result.m_iCampaignSchemaVersion
				!= HST_CampaignState.SCHEMA_VERSION
			|| result.m_iSettingsSchemaVersion
				!= HST_RuntimeSettings.SCHEMA_VERSION
			|| !ValidateWorldIdentity(result.m_sWorld, result.m_sWorld)
			|| result.m_sEvidence.IsEmpty())
			return false;
		if (!result.m_bSuccess)
		{
			evidence = "ordinary persistence failed-stage result exact";
			return true;
		}

		string expectedSource = ResolveExpectedSource(result.m_sStage);
		int expectedGeneration
			= ResolveExpectedSentinelGeneration(result.m_sStage);
		if (result.m_sExpectedSource != expectedSource
			|| result.m_sSource != expectedSource || !result.m_bSourceExact
			|| !result.m_bPersistenceSystemAvailable)
			return false;
		if (result.m_sActiveSaveType
				!= ResolveExpectedActiveSaveType(result.m_sStage)
			|| result.m_sActiveSaveName
				!= ResolveExpectedActiveSaveName(result.m_sStage))
			return false;
		if (result.m_iExpectedSentinelGeneration != expectedGeneration
			|| result.m_iSentinelGeneration != expectedGeneration
			|| !result.m_bSentinelExact
			|| result.m_sExpectedSentinelFingerprint.IsEmpty()
			|| result.m_sSentinelFingerprint
				!= result.m_sExpectedSentinelFingerprint)
			return false;
		if (result.m_sExpectedProfileFallbackFingerprint.IsEmpty()
			|| result.m_sProfileFallbackReadBackFingerprint
				!= result.m_sExpectedProfileFallbackFingerprint
			|| !result.m_bProfileFallbackReadBackExact)
			return false;

		if (result.m_sStage == STAGE_AUTOSAVE_CHECKPOINT)
		{
			if (!result.m_sSourceFingerprint.IsEmpty()
				|| result.m_bWasDataLoaded
				|| result.m_bNativeRecordPresent
				|| result.m_bNativeRecordValid)
				return false;
		}
		else if (result.m_sStage == STAGE_PROFILE_FALLBACK_VERIFY)
		{
			if (result.m_sSourceFingerprint.IsEmpty()
				|| result.m_bWasDataLoaded
				|| result.m_bNativeRecordPresent
				|| result.m_bNativeRecordValid)
				return false;
		}
		else if (result.m_sSourceFingerprint.IsEmpty()
			|| !result.m_bWasDataLoaded
			|| !result.m_bNativeRecordPresent
			|| !result.m_bNativeRecordValid)
			return false;

		if (result.m_sStage == STAGE_AUTOSAVE_CHECKPOINT)
		{
			if (!result.m_bSchedulerExercised
				|| !result.m_bSchedulerThresholdCrossed
				|| !result.m_bSchedulerMajorChangePendingAtAttempt
				|| !result.m_bSchedulerDebounceRemarked
				|| !result.m_bSchedulerDebounceHeld
				|| result.m_sSchedulerOrigin
					!= HST_PersistenceService
						.SCHEDULER_ORIGIN_PERIODIC_AUTOSAVE
				|| result.m_iSchedulerAttemptSequence != 1
				|| result.m_iSchedulerTickCountAtAttempt <= 1
				|| result.m_iSchedulerAutosaveIntervalSeconds
					!= AUTOSAVE_SCHEDULER_INTERVAL_SECONDS
				|| result.m_iSchedulerMajorChangeDebounceSeconds
					!= AUTOSAVE_SCHEDULER_DEBOUNCE_SECONDS)
				return false;
			if (result.m_fSchedulerCumulativeSecondsAtAttempt
					< AUTOSAVE_SCHEDULER_INTERVAL_SECONDS
				|| result.m_fSchedulerDebounceRemarkElapsedSeconds
					< AUTOSAVE_SCHEDULER_REMARK_SECONDS
				|| result.m_fSchedulerDebounceRemarkElapsedSeconds
					> AUTOSAVE_SCHEDULER_REMARK_SECONDS + 5.0
				|| result.m_fSchedulerAutosaveElapsedBeforeSeconds
					>= AUTOSAVE_SCHEDULER_INTERVAL_SECONDS
				|| result.m_fSchedulerAutosaveElapsedAtAttemptSeconds
					< AUTOSAVE_SCHEDULER_INTERVAL_SECONDS
				|| result.m_fSchedulerAutosaveElapsedAtAttemptSeconds
					> AUTOSAVE_SCHEDULER_INTERVAL_SECONDS + 10.0)
				return false;
			if (result.m_fSchedulerMajorChangeElapsedBeforeSeconds
					>= AUTOSAVE_SCHEDULER_DEBOUNCE_SECONDS
				|| result.m_fSchedulerMajorChangeElapsedAtAttemptSeconds
					< AUTOSAVE_SCHEDULER_INTERVAL_SECONDS
				|| result.m_fSchedulerMajorChangeElapsedAtAttemptSeconds
					>= AUTOSAVE_SCHEDULER_DEBOUNCE_SECONDS)
				return false;
		}
		else if (result.m_bSchedulerExercised
			|| result.m_bSchedulerThresholdCrossed
			|| result.m_bSchedulerMajorChangePendingAtAttempt
			|| result.m_bSchedulerDebounceRemarked
			|| result.m_bSchedulerDebounceHeld
			|| !result.m_sSchedulerOrigin.IsEmpty()
			|| result.m_iSchedulerAttemptSequence != 0
			|| result.m_iSchedulerTickCountAtAttempt != 0
			|| result.m_iSchedulerAutosaveIntervalSeconds != 0
			|| result.m_iSchedulerMajorChangeDebounceSeconds != 0
			|| result.m_fSchedulerCumulativeSecondsAtAttempt != 0
			|| result.m_fSchedulerDebounceRemarkElapsedSeconds != 0
			|| result.m_fSchedulerAutosaveElapsedBeforeSeconds != 0
			|| result.m_fSchedulerAutosaveElapsedAtAttemptSeconds != 0
			|| result.m_fSchedulerMajorChangeElapsedBeforeSeconds != 0
			|| result.m_fSchedulerMajorChangeElapsedAtAttemptSeconds != 0)
			return false;

		bool createsSavePoint = StageCreatesSavePoint(result.m_sStage);
		if (result.m_sExpectedSaveType
				!= ResolveExpectedSaveType(result.m_sStage)
			|| result.m_sExpectedSaveName
				!= ResolveExpectedSaveName(result.m_sStage))
			return false;
		if (createsSavePoint)
		{
			int expectedRequestFlags
				= ResolveExpectedRequestFlags(result.m_sStage);
			if (result.m_iExpectedRequestFlags != expectedRequestFlags
				|| result.m_iObservedRequestFlags != expectedRequestFlags
				|| !result.m_bRequestFlagsExact)
				return false;
			if (result.m_sCreatedSaveType != result.m_sExpectedSaveType
				|| result.m_sCreatedSaveName != result.m_sExpectedSaveName
				|| !result.m_bNativePayloadPrepared
				|| !result.m_bSavePointRequested)
				return false;
			if (!result.m_bCompletionCallbackSucceeded
				|| !result.m_bOnAfterSaveObserved
				|| !result.m_bOnAfterSaveSucceeded
				|| !result.m_bOnSaveCreatedObserved)
				return false;
			if (!result.m_bPriorSavePointExact
				|| result.m_sObservedPriorSavePointId
					!= result.m_sExpectedPriorSavePointId
				|| !result.m_bActiveSavePointExact)
				return false;
			if (!UUID.IsUUID(result.m_sCreatedSavePointId)
				|| result.m_sActiveSavePointId
					!= result.m_sCreatedSavePointId
				|| result.m_sExpectedCurrentSavePointId
					!= result.m_sCreatedSavePointId)
				return false;
		}
		else
		{
			if (result.m_iExpectedRequestFlags != 0
				|| result.m_iObservedRequestFlags != 0
				|| result.m_bRequestFlagsExact)
				return false;
			if (result.m_sCreatedSaveType != SAVE_TYPE_NONE
				|| !result.m_sCreatedSaveName.IsEmpty()
				|| !result.m_sCreatedSavePointId.IsEmpty()
				|| result.m_bNativePayloadPrepared)
				return false;
			if (result.m_bSavePointRequested
				|| result.m_bCompletionCallbackSucceeded
				|| result.m_bOnAfterSaveObserved
				|| result.m_bOnAfterSaveSucceeded)
				return false;
			if (result.m_bOnSaveCreatedObserved
				|| !result.m_bPriorSavePointExact
				|| !result.m_bActiveSavePointExact)
				return false;
			if (result.m_sStage == STAGE_NATIVE_SHUTDOWN_VERIFY
				&& (!UUID.IsUUID(result.m_sActiveSavePointId)
					|| result.m_sActiveSavePointId
						!= result.m_sExpectedCurrentSavePointId
					|| result.m_sObservedPriorSavePointId
						!= result.m_sActiveSavePointId))
				return false;
			if (result.m_sStage == STAGE_PROFILE_FALLBACK_VERIFY
				&& (!result.m_sObservedPriorSavePointId.IsEmpty()
					|| !result.m_sActiveSavePointId.IsEmpty()
					|| !result.m_sExpectedCurrentSavePointId.IsEmpty()))
				return false;
		}

		evidence = "ordinary persistence stage result exact";
		return true;
	}

	static bool SaveResult(
		HST_OrdinaryCampaignPersistenceResult result,
		out string evidence)
	{
		if (!result || !ValidateResult(result, evidence))
			return false;
		HST_OrdinaryCampaignPersistenceOwner owner;
		string ownerEvidence;
		if (!LoadAndValidateOwner(
			result.m_sSessionNonce,
			result.m_sRunId,
			result.m_sWorld,
			owner,
			ownerEvidence)
			|| owner.m_sPayloadNonce != result.m_sPayloadNonce)
		{
			evidence = "ordinary persistence result owner rejected | "
				+ ownerEvidence;
			return false;
		}
		if (result.m_bSuccess)
		{
			HST_OrdinaryCampaignPersistenceCarrier carrier;
			string carrierEvidence;
			if (!LoadCarrierFile(result.m_sRunId, carrier, carrierEvidence)
				|| !ValidateCarrier(
					carrier,
					result.m_sSessionNonce,
					result.m_sRunId,
					result.m_sPayloadNonce,
					result.m_sWorld,
					ResolveExpectedSentinelGeneration(result.m_sStage),
					carrierEvidence)
				|| !ValidateResultCarrierRelationship(
					result,
					carrier,
					carrierEvidence))
			{
				evidence = evidence + " | " + carrierEvidence;
				return false;
			}
		}

		string path = BuildResultPath(result.m_sRunId, result.m_sStage);
		if (path.IsEmpty())
			return false;
		if (FileIO.FileExists(path))
		{
			evidence
				= "ordinary persistence stage result storage is not fresh";
			return false;
		}
		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", result) || !context.SaveToFile(path))
		{
			evidence = "ordinary persistence stage result write failed";
			return false;
		}
		evidence = "ordinary persistence stage result saved";
		return true;
	}

	static bool LoadResult(
		string sessionNonce,
		string stageNonce,
		string runId,
		string payloadNonce,
		string stage,
		string expectedWorld,
		out HST_OrdinaryCampaignPersistenceResult result,
		out string evidence)
	{
		result = null;
		evidence = "ordinary persistence stage result unavailable";
		string path = BuildResultPath(runId, stage);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		result = new HST_OrdinaryCampaignPersistenceResult();
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

	static bool ValidateEndBridgeReceipt(
		HST_OrdinaryCampaignEndBridgeReceipt receipt,
		string expectedSessionNonce,
		string expectedStageNonce,
		string expectedRunId,
		string expectedPayloadNonce,
		string expectedStage,
		string expectedWorld,
		string expectedShutdownSavePointId,
		out string evidence)
	{
		evidence = "ordinary persistence end bridge receipt rejected";
		if (!receipt || !ValidateNonce(expectedSessionNonce)
			|| !ValidateNonce(expectedStageNonce)
			|| !ValidateRunId(expectedRunId)
			|| !ValidateNonce(expectedPayloadNonce))
			return false;
		if (expectedStage != STAGE_SHUTDOWN_CHECKPOINT
			|| expectedWorld.IsEmpty()
			|| !UUID.IsUUID(expectedShutdownSavePointId))
			return false;

		bool identityExact = receipt.m_sMagic == END_BRIDGE_RECEIPT_MAGIC
			&& receipt.m_iVersion == AUTHORITY_VERSION
			&& receipt.m_sSessionNonce == expectedSessionNonce
			&& receipt.m_sStageNonce == expectedStageNonce
			&& receipt.m_sRunId == expectedRunId
			&& SanitizeRunId(receipt.m_sRunId) == receipt.m_sRunId
			&& receipt.m_sPayloadNonce == expectedPayloadNonce;
		if (!identityExact)
			return false;
		bool stageExact = receipt.m_sStage == expectedStage
			&& receipt.m_iStageOrdinal
				== ResolveStageOrdinal(expectedStage);
		if (!stageExact)
			return false;

		bool buildExact = receipt.m_sBuildSha == HST_BuildInfo.BUILD_SHA
			&& receipt.m_sBuildUtc == HST_BuildInfo.BUILD_UTC
			&& receipt.m_sBuildLabel == HST_BuildInfo.BUILD_LABEL
			&& receipt.m_iCampaignSchemaVersion
				== HST_CampaignState.SCHEMA_VERSION
			&& receipt.m_iSettingsSchemaVersion
				== HST_RuntimeSettings.SCHEMA_VERSION;
		if (!buildExact
			|| !ValidateWorldIdentity(receipt.m_sWorld, expectedWorld)
			|| receipt.m_sEvidence.IsEmpty())
			return false;

		bool bridgeExact = receipt.m_bSuccess
			&& receipt.m_bEndGameModeIntercepted
			&& receipt.m_bStableCheckpointObserved
			&& receipt.m_bRetentionHandlerExecuted
			&& receipt.m_bKeepSessionSaveCLIAbsent
			&& receipt.m_bPersistenceKeepSessionDataDisabled;
		if (!bridgeExact || !receipt.m_bShutdownSavePointExact)
			return false;
		if (!UUID.IsUUID(receipt.m_sExpectedShutdownSavePointId)
			|| !UUID.IsUUID(receipt.m_sObservedShutdownSavePointId))
			return false;
		if (receipt.m_sExpectedShutdownSavePointId
				!= expectedShutdownSavePointId
			|| receipt.m_sObservedShutdownSavePointId
				!= expectedShutdownSavePointId)
			return false;

		evidence
			= "ordinary persistence end bridge receipt exact";
		return true;
	}

	static bool SaveEndBridgeReceipt(
		HST_OrdinaryCampaignEndBridgeReceipt receipt,
		out string evidence)
	{
		if (!receipt || !ValidateEndBridgeReceipt(
			receipt,
			receipt.m_sSessionNonce,
			receipt.m_sStageNonce,
			receipt.m_sRunId,
			receipt.m_sPayloadNonce,
			receipt.m_sStage,
			receipt.m_sWorld,
			receipt.m_sExpectedShutdownSavePointId,
			evidence))
			return false;

		HST_OrdinaryCampaignPersistenceOwner owner;
		string ownerEvidence;
		if (!LoadAndValidateOwner(
			receipt.m_sSessionNonce,
			receipt.m_sRunId,
			receipt.m_sWorld,
			owner,
			ownerEvidence)
			|| owner.m_sPayloadNonce != receipt.m_sPayloadNonce)
		{
			evidence = "ordinary persistence end bridge owner rejected | "
				+ ownerEvidence;
			return false;
		}

		string path = BuildEndBridgeReceiptPath(
			receipt.m_sRunId,
			receipt.m_sStage);
		if (path.IsEmpty())
			return false;
		if (FileIO.FileExists(path))
		{
			evidence
				= "ordinary persistence end bridge receipt storage is not fresh";
			return false;
		}

		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", receipt) || !context.SaveToFile(path))
		{
			evidence
				= "ordinary persistence end bridge receipt write failed";
			return false;
		}
		evidence = "ordinary persistence end bridge receipt saved";
		return true;
	}

	static bool LoadAndValidateEndBridgeReceipt(
		string sessionNonce,
		string stageNonce,
		string runId,
		string payloadNonce,
		string stage,
		string expectedWorld,
		string expectedShutdownSavePointId,
		out HST_OrdinaryCampaignEndBridgeReceipt receipt,
		out string evidence)
	{
		receipt = null;
		evidence = "ordinary persistence end bridge receipt unavailable";
		string path = BuildEndBridgeReceiptPath(runId, stage);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		receipt = new HST_OrdinaryCampaignEndBridgeReceipt();
		if (!context.ReadValue("", receipt))
		{
			receipt = null;
			return false;
		}
		return ValidateEndBridgeReceipt(
			receipt,
			sessionNonce,
			stageNonce,
			runId,
			payloadNonce,
			stage,
			expectedWorld,
			expectedShutdownSavePointId,
			evidence);
	}
}
