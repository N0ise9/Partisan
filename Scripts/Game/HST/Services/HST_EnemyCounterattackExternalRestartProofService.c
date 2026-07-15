// Synchronous profile-artifact I/O for the disposable exact-counterattack
// external process-restart proof. Nonce, build, schema, world, cut, and stage
// gates prevent stale artifacts from being accepted as current-run evidence.
class HST_EnemyCounterattackExternalRestartProofService
{
	static const string OWNER_MAGIC = "partisan_exact_counterattack_restart_owner_v1";
	static const string GUARD_MAGIC = "partisan_exact_counterattack_restart_guard_v1";
	static const string CARRIER_MAGIC = "partisan_exact_counterattack_restart_carrier_v1";
	static const string RESULT_MAGIC = "partisan_exact_counterattack_restart_result_v1";
	static const string OWNER_PURPOSE = "exact_counterattack_external_restart";
	static const int AUTHORITY_VERSION = 1;
	static const string CUT_OUTBOUND_VIRTUAL = "outbound_virtual";
	static const string STAGE_PREPARE = "prepare";
	static const string STAGE_RECOVER = "recover";
	static const string STAGE_REPLAY = "replay";
	static const string CANONICAL_WORLD = "Worlds/HST_Dev/HST_Dev.ent";
	static const string CANONICAL_WORLD_LOWER = "worlds/hst_dev/hst_dev.ent";
	static const int MAX_RUN_ID_CHARACTERS = 64;
	static const int NONCE_CHARACTERS = 32;
	static const float PROGRESS_EPSILON_METERS = 0.1;

	static string SanitizeRunId(string runId)
	{
		if (runId.IsEmpty() || runId.Length() > MAX_RUN_ID_CHARACTERS)
			return "";
		for (int index = 0; index < runId.Length(); index++)
		{
			string character = runId.Substring(index, 1);
			if (!"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-".Contains(character))
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

	static string NormalizeWorldIdentity(string world)
	{
		if (world.IsEmpty())
			return "";
		string normalized = world.Trim();
		normalized.ToLower();
		int pathStart = normalized.IndexOf(CANONICAL_WORLD_LOWER);
		if (pathStart < 0
			|| pathStart + CANONICAL_WORLD_LOWER.Length() != normalized.Length())
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

	static int ResolveCut(string cutName)
	{
		if (cutName == CUT_OUTBOUND_VIRTUAL)
			return 0;
		return -1;
	}

	static string CutName(int cut)
	{
		if (cut == 0)
			return CUT_OUTBOUND_VIRTUAL;
		return "";
	}

	static bool ValidateStage(string stage)
	{
		return stage == STAGE_PREPARE
			|| stage == STAGE_RECOVER
			|| stage == STAGE_REPLAY;
	}

	static int ResolveStageOrdinal(string stage)
	{
		if (stage == STAGE_PREPARE)
			return 0;
		if (stage == STAGE_RECOVER)
			return 1;
		if (stage == STAGE_REPLAY)
			return 2;
		return -1;
	}

	static string BuildOwnerPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_ExactCounterattackRestart_" + safeRunId + ".owner.json";
	}

	static string BuildGuardPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_ExactCounterattackRestart_" + safeRunId + ".guard.json";
	}

	static string BuildCarrierPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_ExactCounterattackRestart_" + safeRunId + ".carrier.json";
	}

	static string BuildResultPath(string runId, string stage)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty() || !ValidateStage(stage))
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_ExactCounterattackRestart_" + safeRunId + "." + stage + ".json";
	}

	static bool ValidateOwner(
		HST_EnemyCounterattackExternalRestartOwner owner,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedCut,
		string expectedWorld,
		out string evidence)
	{
		evidence = "external counterattack restart profile owner rejected";
		if (!owner || !ValidateNonce(expectedSessionNonce)
			|| !ValidateRunId(expectedRunId) || ResolveCut(expectedCut) < 0
			|| expectedWorld.IsEmpty())
			return false;
		if (owner.m_sMagic != OWNER_MAGIC
			|| owner.m_iVersion != AUTHORITY_VERSION
			|| owner.m_sPurpose != OWNER_PURPOSE
			|| owner.m_sSessionNonce != expectedSessionNonce
			|| owner.m_sRunId != expectedRunId
			|| SanitizeRunId(owner.m_sRunId) != owner.m_sRunId
			|| owner.m_sRequestedCut != expectedCut
			|| !owner.m_bDisposableProfile)
			return false;
		if (owner.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| owner.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| owner.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL
			|| owner.m_iCampaignSchemaVersion != HST_CampaignState.SCHEMA_VERSION
			|| !ValidateWorldIdentity(owner.m_sWorld, expectedWorld))
			return false;
		evidence = "external counterattack restart profile owner exact";
		return true;
	}

	static bool LoadOwner(
		string runId,
		out HST_EnemyCounterattackExternalRestartOwner owner,
		out string evidence)
	{
		owner = null;
		evidence = "external counterattack restart profile owner unavailable";
		string path = BuildOwnerPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		owner = new HST_EnemyCounterattackExternalRestartOwner();
		if (!context.ReadValue("", owner))
		{
			owner = null;
			return false;
		}
		evidence = "external counterattack restart profile owner loaded";
		return true;
	}

	static bool LoadAndValidateOwner(
		string sessionNonce,
		string runId,
		string cutName,
		string expectedWorld,
		out HST_EnemyCounterattackExternalRestartOwner owner,
		out string evidence)
	{
		if (!LoadOwner(runId, owner, evidence))
			return false;
		return ValidateOwner(
			owner,
			sessionNonce,
			runId,
			cutName,
			expectedWorld,
			evidence);
	}

	static bool ValidateGuard(
		HST_EnemyCounterattackExternalRestartGuard guard,
		string expectedSessionNonce,
		string expectedStageNonce,
		string expectedRunId,
		string expectedCut,
		string expectedStage,
		string expectedWorld,
		out string evidence)
	{
		evidence = "external counterattack restart one-use stage lease rejected";
		int expectedStageOrdinal = ResolveStageOrdinal(expectedStage);
		if (!guard || !ValidateNonce(expectedSessionNonce)
			|| !ValidateNonce(expectedStageNonce) || !ValidateRunId(expectedRunId)
			|| ResolveCut(expectedCut) < 0 || expectedStageOrdinal < 0
			|| expectedWorld.IsEmpty())
			return false;
		if (guard.m_sMagic != GUARD_MAGIC
			|| guard.m_iVersion != AUTHORITY_VERSION
			|| guard.m_sSessionNonce != expectedSessionNonce
			|| guard.m_sStageNonce != expectedStageNonce
			|| guard.m_sRunId != expectedRunId
			|| SanitizeRunId(guard.m_sRunId) != guard.m_sRunId
			|| guard.m_sRequestedCut != expectedCut
			|| guard.m_sRequestedStage != expectedStage
			|| guard.m_iStageOrdinal != expectedStageOrdinal)
			return false;
		if (guard.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| guard.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| guard.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL
			|| guard.m_iCampaignSchemaVersion != HST_CampaignState.SCHEMA_VERSION
			|| !ValidateWorldIdentity(guard.m_sWorld, expectedWorld))
			return false;
		if (!guard.m_bAllowCanonicalCampaignOverwrite)
		{
			evidence = "external counterattack restart stage lease does not authorize disposable canonical campaign overwrite";
			return false;
		}
		evidence = "external counterattack restart one-use stage lease exact";
		return true;
	}

	static bool SaveGuard(
		HST_EnemyCounterattackExternalRestartGuard guard,
		out string evidence)
	{
		if (!guard || !ValidateGuard(
			guard,
			guard.m_sSessionNonce,
			guard.m_sStageNonce,
			guard.m_sRunId,
			guard.m_sRequestedCut,
			guard.m_sRequestedStage,
			guard.m_sWorld,
			evidence))
			return false;
		string path = BuildGuardPath(guard.m_sRunId);
		if (path.IsEmpty())
			return false;
		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", guard) || !context.SaveToFile(path))
		{
			evidence = "external counterattack restart guard write failed";
			return false;
		}
		evidence = "external counterattack restart guard saved";
		return true;
	}

	static bool LoadGuard(
		string runId,
		out HST_EnemyCounterattackExternalRestartGuard guard,
		out string evidence)
	{
		guard = null;
		evidence = "external counterattack restart guard unavailable";
		string path = BuildGuardPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		guard = new HST_EnemyCounterattackExternalRestartGuard();
		if (!context.ReadValue("", guard))
		{
			guard = null;
			return false;
		}
		evidence = "external counterattack restart guard loaded";
		return true;
	}

	static bool LoadAndValidateGuard(
		string sessionNonce,
		string stageNonce,
		string runId,
		string cutName,
		string stage,
		string expectedWorld,
		out HST_EnemyCounterattackExternalRestartGuard guard,
		out string evidence)
	{
		if (!LoadGuard(runId, guard, evidence))
			return false;
		return ValidateGuard(
			guard,
			sessionNonce,
			stageNonce,
			runId,
			cutName,
			stage,
			expectedWorld,
			evidence);
	}

	static bool ConsumeStageLease(
		string sessionNonce,
		string stageNonce,
		string runId,
		string cutName,
		string stage,
		string expectedWorld,
		out HST_EnemyCounterattackExternalRestartGuard guard,
		out string evidence)
	{
		if (!LoadAndValidateGuard(
			sessionNonce,
			stageNonce,
			runId,
			cutName,
			stage,
			expectedWorld,
			guard,
			evidence))
			return false;
		string path = BuildGuardPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
		{
			evidence = "external counterattack restart stage lease disappeared before consumption";
			return false;
		}
		FileIO.DeleteFile(path);
		if (FileIO.FileExists(path))
		{
			evidence = "external counterattack restart stage lease could not be consumed";
			return false;
		}
		evidence = "external counterattack restart one-use stage lease consumed";
		return true;
	}

	static bool ValidateCarrier(
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedCut,
		string expectedWorld,
		out string evidence)
	{
		evidence = "external counterattack restart carrier rejected";
		int expectedCutValue = ResolveCut(expectedCut);
		if (!carrier || !ValidateNonce(expectedSessionNonce)
			|| !ValidateRunId(expectedRunId) || expectedCutValue < 0
			|| expectedWorld.IsEmpty())
			return false;
		if (carrier.m_sMagic != CARRIER_MAGIC
			|| carrier.m_sSessionNonce != expectedSessionNonce
			|| carrier.m_sRunId != expectedRunId
			|| SanitizeRunId(carrier.m_sRunId) != carrier.m_sRunId)
			return false;
		if (carrier.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| carrier.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| carrier.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL
			|| carrier.m_iCampaignSchemaVersion != HST_CampaignState.SCHEMA_VERSION
			|| !ValidateWorldIdentity(carrier.m_sWorld, expectedWorld))
			return false;
		if (carrier.m_sCutName != expectedCut || carrier.m_iCut != expectedCutValue
			|| !carrier.m_Expectation || carrier.m_sPreparedSemanticFingerprint.IsEmpty())
			return false;

		HST_EnemyCounterattackOutboundVirtualExpectation expectation = carrier.m_Expectation;
		if (expectation.m_sOrderId.IsEmpty() || expectation.m_sOperationId.IsEmpty()
			|| expectation.m_sManifestId.IsEmpty() || expectation.m_sManifestHash.IsEmpty()
			|| expectation.m_sBatchId.IsEmpty() || expectation.m_sGroupId.IsEmpty()
			|| expectation.m_sProjectionId.IsEmpty() || expectation.m_sForceId.IsEmpty()
			|| expectation.m_sFactionKey.IsEmpty() || expectation.m_sSourceZoneId.IsEmpty()
			|| expectation.m_sTargetZoneId.IsEmpty() || expectation.m_sDebitMutationId.IsEmpty()
			|| expectation.m_sLivingSlotFingerprint.IsEmpty())
			return false;
		if (expectation.m_sSourceZoneId == expectation.m_sTargetZoneId)
			return false;
		bool attackFunded = expectation.m_iAttackCost > 0
			&& expectation.m_iSupportCost == 0;
		bool supportFunded = expectation.m_iSupportCost > 0
			&& expectation.m_iAttackCost == 0;
		if (!attackFunded && !supportFunded)
			return false;
		if (expectation.m_iExpectedAttackPool < 0
			|| expectation.m_iExpectedSupportPool < 0
			|| expectation.m_iExpectedPoolOperationalMutationCount != 1)
			return false;
		if (expectation.m_iAcceptedMemberCount <= 0
			|| expectation.m_iLivingMemberCount <= 0
			|| expectation.m_iLivingMemberCount > expectation.m_iAcceptedMemberCount)
			return false;
		if (carrier.m_iPreparedElapsedSecond <= 0
			|| carrier.m_fPreparedRouteTotalDistanceMeters <= 0
			|| carrier.m_fPreparedRouteProgressMeters <= 0
			|| carrier.m_fPreparedRouteProgressMeters
				>= carrier.m_fPreparedRouteTotalDistanceMeters
			|| IsZeroVector(carrier.m_vPreparedStrategicPosition))
			return false;

		evidence = "external counterattack restart carrier exact";
		return true;
	}

	static bool SaveCarrier(
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		if (!carrier || !ValidateCarrier(
			carrier,
			carrier.m_sSessionNonce,
			carrier.m_sRunId,
			carrier.m_sCutName,
			carrier.m_sWorld,
			evidence))
			return false;
		string path = BuildCarrierPath(carrier.m_sRunId);
		if (path.IsEmpty())
			return false;
		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", carrier) || !context.SaveToFile(path))
		{
			evidence = "external counterattack restart carrier write failed";
			return false;
		}
		evidence = "external counterattack restart carrier saved";
		return true;
	}

	static bool LoadCarrier(
		string sessionNonce,
		string runId,
		string cutName,
		string expectedWorld,
		out HST_EnemyCounterattackExternalRestartCarrier carrier,
		out string evidence)
	{
		carrier = null;
		evidence = "external counterattack restart carrier unavailable";
		string path = BuildCarrierPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		carrier = new HST_EnemyCounterattackExternalRestartCarrier();
		if (!context.ReadValue("", carrier))
		{
			carrier = null;
			return false;
		}
		return ValidateCarrier(
			carrier,
			sessionNonce,
			runId,
			cutName,
			expectedWorld,
			evidence);
	}

	static bool ValidateResult(
		HST_EnemyCounterattackExternalRestartResult result,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedCut,
		string expectedStage,
		string expectedWorld,
		out string evidence)
	{
		evidence = "external counterattack restart result rejected";
		int expectedCutValue = ResolveCut(expectedCut);
		if (!result || !ValidateNonce(expectedSessionNonce)
			|| !ValidateRunId(expectedRunId) || expectedCutValue < 0
			|| !ValidateStage(expectedStage) || expectedWorld.IsEmpty())
			return false;
		if (result.m_sMagic != RESULT_MAGIC
			|| result.m_sSessionNonce != expectedSessionNonce
			|| result.m_sRunId != expectedRunId
			|| SanitizeRunId(result.m_sRunId) != result.m_sRunId)
			return false;
		if (result.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| result.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| result.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL
			|| result.m_iCampaignSchemaVersion != HST_CampaignState.SCHEMA_VERSION
			|| !ValidateWorldIdentity(result.m_sWorld, expectedWorld))
			return false;
		if (result.m_sCutName != expectedCut || result.m_iCut != expectedCutValue
			|| result.m_sStage != expectedStage || result.m_sEvidence.IsEmpty())
			return false;
		if (!result.m_bSuccess)
		{
			evidence = "external counterattack restart result exact";
			return true;
		}
		if (!result.m_bSourceExact || !result.m_bRuntimeClaimantsZero
			|| !result.m_bPersistedReadBackExact
			|| result.m_sSourceSemanticFingerprint.IsEmpty()
			|| result.m_sFinalSemanticFingerprint.IsEmpty())
			return false;

		if (expectedStage == STAGE_PREPARE)
		{
			if (result.m_bRestored || result.m_bStartupReconcileChanged
				|| result.m_bContinuationExact || result.m_bSameStateSemanticNoOp
				|| Math.AbsFloat(result.m_fProgressAfterMeters
					- result.m_fProgressBeforeMeters) > PROGRESS_EPSILON_METERS
				|| result.m_sSourceSemanticFingerprint
					!= result.m_sFinalSemanticFingerprint)
				return false;
		}
		else if (expectedStage == STAGE_RECOVER)
		{
			float expectedProgressDelta
				= HST_StrategicMovementService.EXACT_PLAYER_QRF_SPEED_METERS_PER_SECOND
					* HST_StrategicMovementService.MAX_CATCHUP_SECONDS_PER_TICK;
			if (!result.m_bRestored || !result.m_bContinuationExact
				|| result.m_bSameStateSemanticNoOp
				|| Math.AbsFloat(
					(result.m_fProgressAfterMeters - result.m_fProgressBeforeMeters)
						- expectedProgressDelta) > PROGRESS_EPSILON_METERS
				|| result.m_sSourceSemanticFingerprint
					== result.m_sFinalSemanticFingerprint)
				return false;
		}
		else if (expectedStage == STAGE_REPLAY)
		{
			if (!result.m_bRestored || result.m_bContinuationExact
				|| !result.m_bSameStateSemanticNoOp
				|| Math.AbsFloat(result.m_fProgressAfterMeters
					- result.m_fProgressBeforeMeters) > PROGRESS_EPSILON_METERS
				|| result.m_sSourceSemanticFingerprint
					!= result.m_sFinalSemanticFingerprint)
				return false;
		}

		// Startup reconciliation may legitimately update bookkeeping on either
		// restored process. Success is decided by semantic continuity instead.
		evidence = "external counterattack restart result exact";
		return true;
	}

	// Call only after every stage-specific semantic and read-back check has
	// completed. The result is the final durable write for that process stage.
	static bool SaveResult(
		HST_EnemyCounterattackExternalRestartResult result,
		out string evidence)
	{
		if (!result || !ValidateResult(
			result,
			result.m_sSessionNonce,
			result.m_sRunId,
			result.m_sCutName,
			result.m_sStage,
			result.m_sWorld,
			evidence))
			return false;
		string path = BuildResultPath(result.m_sRunId, result.m_sStage);
		if (path.IsEmpty())
			return false;
		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", result) || !context.SaveToFile(path))
		{
			evidence = "external counterattack restart result write failed";
			return false;
		}
		evidence = "external counterattack restart result saved";
		return true;
	}

	static bool LoadResult(
		string sessionNonce,
		string runId,
		string cutName,
		string stage,
		string expectedWorld,
		out HST_EnemyCounterattackExternalRestartResult result,
		out string evidence)
	{
		result = null;
		evidence = "external counterattack restart result unavailable";
		string path = BuildResultPath(runId, stage);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		result = new HST_EnemyCounterattackExternalRestartResult();
		if (!context.ReadValue("", result))
		{
			result = null;
			return false;
		}
		return ValidateResult(
			result,
			sessionNonce,
			runId,
			cutName,
			stage,
			expectedWorld,
			evidence);
	}

	static bool ValidateDisposableProfileShape(
		string sessionNonce,
		string runId,
		string cutName,
		string stage,
		string expectedWorld,
		out string evidence)
	{
		evidence = "external counterattack restart disposable profile shape rejected";
		if (!ValidateNonce(sessionNonce) || !ValidateRunId(runId)
			|| ResolveCut(cutName) < 0 || ResolveStageOrdinal(stage) < 0
			|| expectedWorld.IsEmpty())
			return false;

		HST_EnemyCounterattackExternalRestartOwner owner;
		string ownerEvidence;
		if (!LoadAndValidateOwner(
			sessionNonce,
			runId,
			cutName,
			expectedWorld,
			owner,
			ownerEvidence))
		{
			evidence = evidence + " | " + ownerEvidence;
			return false;
		}

		string guardPath = BuildGuardPath(runId);
		string carrierPath = BuildCarrierPath(runId);
		string prepareResultPath = BuildResultPath(runId, STAGE_PREPARE);
		string recoverResultPath = BuildResultPath(runId, STAGE_RECOVER);
		string replayResultPath = BuildResultPath(runId, STAGE_REPLAY);
		if (guardPath.IsEmpty() || carrierPath.IsEmpty()
			|| prepareResultPath.IsEmpty() || recoverResultPath.IsEmpty()
			|| replayResultPath.IsEmpty() || !FileIO.FileExists(guardPath))
			return false;

		bool legacyDataPresent = FileIO.FileExists(
			HST_ProfilePathService.LEGACY_PROFILE_DIRECTORY);
		bool migrationResiduePresent = FileIO.FileExists(
			HST_ProfilePathService.LEGACY_ARCHIVE_DIRECTORY);
		migrationResiduePresent = migrationResiduePresent
			|| FileIO.FileExists(HST_ProfilePathService.MIGRATION_STAGING_DIRECTORY);
		bool unrelatedUserDataPresent = FileIO.FileExists(
			HST_ProfilePathService.VISUAL_SETTINGS_FILE);
		unrelatedUserDataPresent = unrelatedUserDataPresent
			|| FileIO.FileExists(HST_ProfilePathService.LOADOUT_DIRECTORY);
		if (legacyDataPresent || migrationResiduePresent || unrelatedUserDataPresent)
		{
			evidence = "external counterattack restart profile contains non-proof or migration data";
			return false;
		}

		if (stage == STAGE_PREPARE)
		{
			bool persistentDataPresent = FileIO.FileExists(
				HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
			persistentDataPresent = persistentDataPresent
				|| FileIO.FileExists(HST_ProfilePathService.SETTINGS_FILE);
			bool proofDataPresent = FileIO.FileExists(carrierPath)
				|| FileIO.FileExists(prepareResultPath);
			proofDataPresent = proofDataPresent
				|| FileIO.FileExists(recoverResultPath)
				|| FileIO.FileExists(replayResultPath);
			if (persistentDataPresent || proofDataPresent)
			{
				evidence = "external counterattack restart prepare requires a fresh disposable profile";
				return false;
			}
			evidence = "external counterattack restart prepare profile shape exact";
			return true;
		}

		if (!FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_SAVE_FILE))
		{
			evidence = "external counterattack restart restore stage has no canonical campaign save";
			return false;
		}
		HST_EnemyCounterattackExternalRestartCarrier carrier;
		string carrierEvidence;
		if (!LoadCarrier(
			sessionNonce,
			runId,
			cutName,
			expectedWorld,
			carrier,
			carrierEvidence))
		{
			evidence = evidence + " | " + carrierEvidence;
			return false;
		}
		HST_EnemyCounterattackExternalRestartResult prepareResult;
		string prepareEvidence;
		if (!LoadResult(
			sessionNonce,
			runId,
			cutName,
			STAGE_PREPARE,
			expectedWorld,
			prepareResult,
			prepareEvidence)
			|| !prepareResult.m_bSuccess)
		{
			evidence = evidence + " | prior prepare result rejected: "
				+ prepareEvidence;
			return false;
		}

		if (stage == STAGE_RECOVER)
		{
			if (FileIO.FileExists(recoverResultPath)
				|| FileIO.FileExists(replayResultPath))
			{
				evidence = "external counterattack restart recover result storage is not fresh";
				return false;
			}
			evidence = "external counterattack restart recover profile shape exact";
			return true;
		}

		HST_EnemyCounterattackExternalRestartResult recoverResult;
		string recoverEvidence;
		if (!LoadResult(
			sessionNonce,
			runId,
			cutName,
			STAGE_RECOVER,
			expectedWorld,
			recoverResult,
			recoverEvidence)
			|| !recoverResult.m_bSuccess)
		{
			evidence = evidence + " | prior recover result rejected: "
				+ recoverEvidence;
			return false;
		}
		if (FileIO.FileExists(replayResultPath))
		{
			evidence = "external counterattack restart replay result storage is not fresh";
			return false;
		}
		evidence = "external counterattack restart replay profile shape exact";
		return true;
	}

	protected static bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01
			&& Math.AbsFloat(value[1]) < 0.01
			&& Math.AbsFloat(value[2]) < 0.01;
	}
}
