#ifdef ENABLE_DIAG
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
	static const string CUT_DEMATERIALIZING_BEFORE_HOLD
		= "dematerializing_before_hold";
	static const string CUT_MATERIALIZING_CHECKPOINT_DEFERRED
		= "materializing_checkpoint_deferred";
	static const string CUT_PHYSICAL_LIVE_POSITION = "physical_live_position";
	static const string CUT_PREPARED_BEFORE_REFUND = "prepared_before_refund";
	static const string CUT_PREPARED_AFTER_REFUND = "prepared_after_refund";
	static const string CUT_PREPARED_AFTER_RECEIPT = "prepared_after_receipt";
	static const string CUT_OWNER_APPLIED_PENDING = "owner_applied_pending";
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
		if (cutName == CUT_DEMATERIALIZING_BEFORE_HOLD)
			return 1;
		if (cutName == CUT_MATERIALIZING_CHECKPOINT_DEFERRED)
			return 2;
		if (cutName == CUT_PHYSICAL_LIVE_POSITION)
			return 3;
		if (cutName == CUT_PREPARED_BEFORE_REFUND)
			return 4;
		if (cutName == CUT_PREPARED_AFTER_REFUND)
			return 5;
		if (cutName == CUT_PREPARED_AFTER_RECEIPT)
			return 6;
		if (cutName == CUT_OWNER_APPLIED_PENDING)
			return 7;
		return -1;
	}

	static string CutName(int cut)
	{
		if (cut == 0)
			return CUT_OUTBOUND_VIRTUAL;
		if (cut == 1)
			return CUT_DEMATERIALIZING_BEFORE_HOLD;
		if (cut == 2)
			return CUT_MATERIALIZING_CHECKPOINT_DEFERRED;
		if (cut == 3)
			return CUT_PHYSICAL_LIVE_POSITION;
		if (cut == 4)
			return CUT_PREPARED_BEFORE_REFUND;
		if (cut == 5)
			return CUT_PREPARED_AFTER_REFUND;
		if (cut == 6)
			return CUT_PREPARED_AFTER_RECEIPT;
		if (cut == 7)
			return CUT_OWNER_APPLIED_PENDING;
		return "";
	}

	static bool IsPreparedSettlementCut(string cutName)
	{
		return cutName == CUT_PREPARED_BEFORE_REFUND
			|| cutName == CUT_PREPARED_AFTER_REFUND
			|| cutName == CUT_PREPARED_AFTER_RECEIPT;
	}

	static bool IsOwnerAppliedPendingCut(string cutName)
	{
		return cutName == CUT_OWNER_APPLIED_PENDING;
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
		bool allowCanonicalCampaignOverwrite
			= !IsOwnerAppliedPendingCut(expectedCut)
			|| expectedStage != STAGE_REPLAY;
		if (guard.m_bNativeSourceSelectionProof)
			allowCanonicalCampaignOverwrite = expectedStage == STAGE_PREPARE;
		if (guard.m_bAllowCanonicalCampaignOverwrite
			!= allowCanonicalCampaignOverwrite)
		{
			evidence = "external counterattack restart stage lease campaign journal overwrite authority rejected";
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

	protected static bool ValidatePreparedSettlementCarrier(
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		string expectedCut,
		out string evidence)
	{
		evidence = "external counterattack prepared-settlement carrier rejected";
		if (!carrier || !IsPreparedSettlementCut(expectedCut)
			|| carrier.m_Expectation || !carrier.m_SettlementExpectation)
		{
			evidence = "external counterattack prepared-settlement carrier family shape rejected";
			return false;
		}

		HST_EnemyCounterattackPreparedSettlementExpectation expectation
			= carrier.m_SettlementExpectation;
		if (expectation.m_sOrderId.IsEmpty()
			|| expectation.m_sOperationId.IsEmpty()
			|| expectation.m_sManifestId.IsEmpty()
			|| expectation.m_sManifestHash.IsEmpty()
			|| expectation.m_sBatchId.IsEmpty()
			|| expectation.m_sGroupId.IsEmpty()
			|| expectation.m_sProjectionId.IsEmpty()
			|| expectation.m_sForceId.IsEmpty()
			|| expectation.m_sFactionKey.IsEmpty()
			|| expectation.m_sSourceZoneId.IsEmpty()
			|| expectation.m_sTargetZoneId.IsEmpty()
			|| expectation.m_sDebitMutationId.IsEmpty())
		{
			evidence = "external counterattack prepared-settlement aggregate identity rejected";
			return false;
		}
		if (expectation.m_sSourceZoneId == expectation.m_sTargetZoneId)
		{
			evidence = "external counterattack prepared-settlement source and target conflict";
			return false;
		}
		if (expectation.m_sExpectedSourceOwnerFactionKey.IsEmpty()
			|| expectation.m_iExpectedSourceOwnershipRevision <= 0
			|| expectation.m_sExpectedTargetOwnerFactionKey.IsEmpty()
			|| expectation.m_iExpectedTargetOwnershipRevision <= 0)
		{
			evidence = "external counterattack prepared-settlement endpoint ownership rejected";
			return false;
		}

		bool attackFunded = expectation.m_iAttackCost > 0
			&& expectation.m_iSupportCost == 0;
		bool supportFunded = expectation.m_iSupportCost > 0
			&& expectation.m_iAttackCost == 0;
		if (!attackFunded && !supportFunded)
		{
			evidence = "external counterattack prepared-settlement one-pool funding rejected";
			return false;
		}
		if (carrier.m_iAccepted <= 1 || carrier.m_iCasualties <= 0
			|| carrier.m_iSurvivors <= 0
			|| carrier.m_iSurvivors
				!= carrier.m_iAccepted - carrier.m_iCasualties
			|| expectation.m_iAccepted != carrier.m_iAccepted
			|| expectation.m_iSurvivors != carrier.m_iSurvivors
			|| expectation.m_iAttackRefund != carrier.m_iAttackRefund
			|| expectation.m_iSupportRefund != carrier.m_iSupportRefund)
		{
			evidence = "external counterattack prepared-settlement survivor tuple rejected";
			return false;
		}
		int expectedAttackRefund = expectation.m_iAttackCost
			* carrier.m_iSurvivors / carrier.m_iAccepted;
		int expectedSupportRefund = expectation.m_iSupportCost
			* carrier.m_iSurvivors / carrier.m_iAccepted;
		if (carrier.m_iAttackRefund != expectedAttackRefund
			|| carrier.m_iSupportRefund != expectedSupportRefund
			|| carrier.m_iAttackRefund + carrier.m_iSupportRefund <= 0)
		{
			evidence = "external counterattack prepared-settlement proportional refund rejected";
			return false;
		}

		if (carrier.m_sSettlementKind != "route_failed_survivors"
			|| expectation.m_sSettlementKind != carrier.m_sSettlementKind
			|| expectation.m_sSettlementId != carrier.m_sSettlementId
			|| expectation.m_sRefundMutationId != carrier.m_sRefundMutationId
			|| expectation.m_sReason != carrier.m_sReason
			|| carrier.m_sReason.IsEmpty()
			|| carrier.m_sSettlementId
				!= HST_OperationService.BuildSettlementId(
					expectation.m_sOperationId,
					carrier.m_sSettlementKind)
			|| carrier.m_sRefundMutationId
				!= "enemy_resource_refund_" + carrier.m_sSettlementId)
		{
			evidence = "external counterattack prepared-settlement deterministic identity rejected";
			return false;
		}

		int expectedPrefixMutationCount;
		int expectedPrefixAttackDelta;
		int expectedPrefixSupportDelta;
		bool expectedPrefixReceiptApplied;
		if (expectedCut == CUT_PREPARED_AFTER_REFUND
			|| expectedCut == CUT_PREPARED_AFTER_RECEIPT)
		{
			expectedPrefixMutationCount = 1;
			expectedPrefixAttackDelta = carrier.m_iAttackRefund;
			expectedPrefixSupportDelta = carrier.m_iSupportRefund;
		}
		if (expectedCut == CUT_PREPARED_AFTER_RECEIPT)
			expectedPrefixReceiptApplied = true;
		if (carrier.m_iExpectedPrefixMutationCount
				!= expectedPrefixMutationCount
			|| carrier.m_iExpectedPrefixAttackDelta
				!= expectedPrefixAttackDelta
			|| carrier.m_iExpectedPrefixSupportDelta
				!= expectedPrefixSupportDelta
			|| carrier.m_bExpectedPrefixReceiptApplied
				!= expectedPrefixReceiptApplied)
		{
			evidence = "external counterattack prepared-settlement prefix policy rejected";
			return false;
		}

		int expectedTerminalRevision = carrier.m_iPrefixRevision + 2;
		if (expectedPrefixReceiptApplied)
			expectedTerminalRevision = carrier.m_iPrefixRevision + 1;
		if (carrier.m_iPreparedAtSecond <= 0 || carrier.m_iPrefixRevision <= 0
			|| expectation.m_iPreparedAtSecond != carrier.m_iPreparedAtSecond
			|| expectation.m_iExpectedTerminalRevision
				!= expectedTerminalRevision
			|| carrier.m_iAttackBeforeRefund < 0
			|| carrier.m_iSupportBeforeRefund < 0
			|| expectation.m_iExpectedAttackPool
				!= carrier.m_iAttackBeforeRefund + carrier.m_iAttackRefund
			|| expectation.m_iExpectedSupportPool
				!= carrier.m_iSupportBeforeRefund + carrier.m_iSupportRefund
			|| expectation.m_iExpectedPoolRevision <= 0
			|| expectation.m_iExpectedPoolOperationalMutationCount != 2
			|| expectation.m_sExpectedLastStrategicMutationId
				!= carrier.m_sRefundMutationId)
		{
			evidence = "external counterattack prepared-settlement terminal expectation rejected";
			return false;
		}

		if (carrier.m_sPreparedSettlementFingerprint.IsEmpty()
			|| carrier.m_sPreparedSettlementFingerprint
				!= carrier.m_sPreparedSemanticFingerprint
			|| carrier.m_sRawPreparedCutSemanticFingerprint
				!= carrier.m_sPreparedSemanticFingerprint
			|| carrier.m_iExpectedPhysicalAdapterHandleCount != 0
			|| carrier.m_iExpectedPhysicalRuntimeMemberCount != 0)
		{
			evidence = "external counterattack prepared-settlement fingerprint or physical count rejected";
			return false;
		}

		evidence = "external counterattack prepared-settlement carrier exact";
		return true;
	}

	static bool ValidateOwnerAppliedPendingCarrier(
		HST_EnemyCounterattackExternalRestartCarrier carrier,
		string expectedCut,
		out string evidence)
	{
		evidence = "external counterattack owner-applied pending carrier rejected";
		if (!carrier || !IsOwnerAppliedPendingCut(expectedCut)
			|| !carrier.m_Expectation || carrier.m_SettlementExpectation)
		{
			evidence = "external counterattack owner-applied pending carrier family shape rejected";
			return false;
		}

		HST_EnemyCounterattackOutboundVirtualExpectation expectation
			= carrier.m_Expectation;
		if (expectation.m_sOrderId.IsEmpty()
			|| expectation.m_sOperationId.IsEmpty()
			|| expectation.m_sManifestId.IsEmpty()
			|| expectation.m_sManifestHash.IsEmpty()
			|| expectation.m_sBatchId.IsEmpty()
			|| expectation.m_sGroupId.IsEmpty()
			|| expectation.m_sProjectionId.IsEmpty()
			|| expectation.m_sForceId.IsEmpty()
			|| expectation.m_sFactionKey.IsEmpty()
			|| expectation.m_sSourceZoneId.IsEmpty()
			|| expectation.m_sTargetZoneId.IsEmpty()
			|| expectation.m_sDebitMutationId.IsEmpty()
			|| expectation.m_sLivingSlotFingerprint.IsEmpty())
		{
			evidence = "external counterattack owner-applied pending aggregate identity rejected";
			return false;
		}
		if (expectation.m_sSourceZoneId == expectation.m_sTargetZoneId
			|| expectation.m_sExpectedSourceOwnerFactionKey.IsEmpty()
			|| expectation.m_iExpectedSourceOwnershipRevision <= 0
			|| expectation.m_sExpectedTargetOwnerFactionKey.IsEmpty()
			|| expectation.m_sExpectedTargetOwnerFactionKey != expectation.m_sFactionKey
			|| expectation.m_iExpectedTargetOwnershipRevision <= 0)
		{
			evidence = "external counterattack owner-applied pending endpoint authority rejected";
			return false;
		}

		bool attackFunded = expectation.m_iAttackCost > 0
			&& expectation.m_iSupportCost == 0;
		bool supportFunded = expectation.m_iSupportCost > 0
			&& expectation.m_iAttackCost == 0;
		if ((!attackFunded && !supportFunded)
			|| expectation.m_iExpectedAttackPool < 0
			|| expectation.m_iExpectedSupportPool < 0
			|| expectation.m_iExpectedPoolOperationalMutationCount != 1)
		{
			evidence = "external counterattack owner-applied pending resource authority rejected";
			return false;
		}

		bool rosterExact = expectation.m_iAcceptedMemberCount > 0
			&& expectation.m_iLivingMemberCount
				== expectation.m_iAcceptedMemberCount
			&& !expectation.m_bExpectedLivingSlotsEverAlive
			&& expectation.m_iExpectedNormalizedSlotAttemptCount == 0
			&& expectation.m_sConfirmedCasualtySlotId.IsEmpty()
			&& expectation.m_sCasualtyTombstoneFingerprint.IsEmpty()
			&& expectation.m_iExpectedNormalizedReprojectionCount == 0;
		if (!rosterExact)
		{
			evidence = "external counterattack owner-applied pending roster authority rejected";
			return false;
		}

		bool routeExact = carrier.m_iPreparedElapsedSecond > 0
			&& carrier.m_fPreparedRouteTotalDistanceMeters > 0
			&& carrier.m_fPreparedRouteProgressMeters
				>= carrier.m_fPreparedRouteTotalDistanceMeters
					- PROGRESS_EPSILON_METERS
			&& carrier.m_fPreparedRouteProgressMeters
				<= carrier.m_fPreparedRouteTotalDistanceMeters
					+ PROGRESS_EPSILON_METERS
			&& !IsZeroVector(carrier.m_vPreparedStrategicPosition);
		bool fingerprintExact
			= carrier.m_sRawPreparedCutSemanticFingerprint
				!= carrier.m_sPreparedSemanticFingerprint;
		bool physicalCountsExact
			= carrier.m_iExpectedPhysicalAdapterHandleCount == 0
			&& carrier.m_iExpectedPhysicalRuntimeMemberCount == 0;
		if (!routeExact || !fingerprintExact || !physicalCountsExact)
		{
			evidence = string.Format(
				"external counterattack owner-applied pending route/fingerprint/physical authority rejected %1/%2/%3",
				routeExact,
				fingerprintExact,
				physicalCountsExact);
			return false;
		}

		evidence = "external counterattack owner-applied pending carrier exact";
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
		{
			evidence = "external counterattack restart carrier prerequisite rejected";
			return false;
		}
		if (carrier.m_sMagic != CARRIER_MAGIC
			|| carrier.m_sSessionNonce != expectedSessionNonce
			|| carrier.m_sRunId != expectedRunId
			|| SanitizeRunId(carrier.m_sRunId) != carrier.m_sRunId)
		{
			evidence = "external counterattack restart carrier identity rejected";
			return false;
		}
		if (carrier.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| carrier.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| carrier.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL
			|| carrier.m_iCampaignSchemaVersion != HST_CampaignState.SCHEMA_VERSION
			|| !ValidateWorldIdentity(carrier.m_sWorld, expectedWorld))
		{
			evidence = "external counterattack restart carrier build, schema, or world rejected";
			return false;
		}
		if (carrier.m_sCutName != expectedCut || carrier.m_iCut != expectedCutValue
			|| carrier.m_sPreparedSemanticFingerprint.IsEmpty()
			|| carrier.m_sRawPreparedCutSemanticFingerprint.IsEmpty())
		{
			evidence = "external counterattack restart carrier cut or fingerprint rejected";
			return false;
		}
		if (carrier.m_bNativeSourceSelectionProof
			&& (!IsOwnerAppliedPendingCut(expectedCut)
				|| !UUID.IsUUID(carrier.m_sNativeSavePointId)
				|| carrier.m_sFallbackConflictSemanticFingerprint.IsEmpty()
				|| carrier.m_sFallbackConflictSemanticFingerprint
					== carrier.m_sPreparedSemanticFingerprint))
		{
			evidence
				= "external counterattack native-source carrier evidence rejected";
			return false;
		}
		if (!carrier.m_bNativeSourceSelectionProof
			&& (!carrier.m_sNativeSavePointId.IsEmpty()
				|| !carrier.m_sFallbackConflictSemanticFingerprint.IsEmpty()))
		{
			evidence
				= "external counterattack legacy carrier has native-source evidence";
			return false;
		}
		if (IsPreparedSettlementCut(expectedCut))
			return ValidatePreparedSettlementCarrier(carrier, expectedCut, evidence);
		if (IsOwnerAppliedPendingCut(expectedCut))
			return ValidateOwnerAppliedPendingCarrier(carrier, expectedCut, evidence);
		if (!carrier.m_Expectation || carrier.m_SettlementExpectation)
		{
			evidence = "external counterattack restart carrier movement family shape rejected";
			return false;
		}

		HST_EnemyCounterattackOutboundVirtualExpectation expectation = carrier.m_Expectation;
		if (expectation.m_sOrderId.IsEmpty() || expectation.m_sOperationId.IsEmpty()
			|| expectation.m_sManifestId.IsEmpty() || expectation.m_sManifestHash.IsEmpty()
			|| expectation.m_sBatchId.IsEmpty() || expectation.m_sGroupId.IsEmpty()
			|| expectation.m_sProjectionId.IsEmpty() || expectation.m_sForceId.IsEmpty()
			|| expectation.m_sFactionKey.IsEmpty() || expectation.m_sSourceZoneId.IsEmpty()
			|| expectation.m_sTargetZoneId.IsEmpty()
			|| expectation.m_sDebitMutationId.IsEmpty()
			|| expectation.m_sLivingSlotFingerprint.IsEmpty())
		{
			evidence = "external counterattack restart carrier aggregate identity rejected";
			return false;
		}
		if (expectation.m_sSourceZoneId == expectation.m_sTargetZoneId)
		{
			evidence = "external counterattack restart carrier source and target conflict";
			return false;
		}
		if (expectation.m_sExpectedSourceOwnerFactionKey.IsEmpty()
			|| expectation.m_iExpectedSourceOwnershipRevision <= 0
			|| expectation.m_sExpectedTargetOwnerFactionKey.IsEmpty()
			|| expectation.m_iExpectedTargetOwnershipRevision <= 0)
		{
			evidence = "external counterattack restart carrier endpoint ownership rejected";
			return false;
		}
		bool attackFunded = expectation.m_iAttackCost > 0
			&& expectation.m_iSupportCost == 0;
		bool supportFunded = expectation.m_iSupportCost > 0
			&& expectation.m_iAttackCost == 0;
		if (!attackFunded && !supportFunded)
		{
			evidence = "external counterattack restart carrier funding rejected";
			return false;
		}
		if (expectation.m_iExpectedAttackPool < 0
			|| expectation.m_iExpectedSupportPool < 0
			|| expectation.m_iExpectedPoolOperationalMutationCount != 1)
		{
			evidence = "external counterattack restart carrier resource expectation rejected";
			return false;
		}
		if (expectation.m_iAcceptedMemberCount <= 0
			|| expectation.m_iLivingMemberCount <= 0
			|| expectation.m_iLivingMemberCount > expectation.m_iAcceptedMemberCount)
		{
			evidence = "external counterattack restart carrier roster counts rejected";
			return false;
		}
		bool checkpointDeferredCut
			= expectedCut == CUT_DEMATERIALIZING_BEFORE_HOLD
				|| expectedCut == CUT_MATERIALIZING_CHECKPOINT_DEFERRED
				|| expectedCut == CUT_PHYSICAL_LIVE_POSITION;
		if (checkpointDeferredCut
			&& carrier.m_sRawPreparedCutSemanticFingerprint
				== carrier.m_sPreparedSemanticFingerprint)
		{
			evidence = "external counterattack restart carrier raw and normalized fingerprints collapsed";
			return false;
		}
		if (expectedCut == CUT_OUTBOUND_VIRTUAL)
		{
			if (expectation.m_bExpectedLivingSlotsEverAlive
				|| expectation.m_iExpectedNormalizedSlotAttemptCount != 0
				|| !expectation.m_sConfirmedCasualtySlotId.IsEmpty()
				|| !expectation.m_sCasualtyTombstoneFingerprint.IsEmpty()
				|| expectation.m_iExpectedNormalizedReprojectionCount != 0
				|| expectation.m_iLivingMemberCount
					!= expectation.m_iAcceptedMemberCount
				|| carrier.m_sRawPreparedCutSemanticFingerprint
					!= carrier.m_sPreparedSemanticFingerprint)
				return false;
		}
		else if (expectedCut == CUT_DEMATERIALIZING_BEFORE_HOLD)
		{
			if (expectation.m_sConfirmedCasualtySlotId.IsEmpty()
				|| expectation.m_sCasualtyTombstoneFingerprint.IsEmpty()
				|| !expectation.m_sCasualtyTombstoneFingerprint.StartsWith(
					expectation.m_sConfirmedCasualtySlotId + "|")
				|| expectation.m_iExpectedNormalizedReprojectionCount != 1
				|| expectation.m_iLivingMemberCount
					!= expectation.m_iAcceptedMemberCount - 1)
				return false;
		}
		else if (expectedCut == CUT_MATERIALIZING_CHECKPOINT_DEFERRED)
		{
			if (expectation.m_bExpectedLivingSlotsEverAlive
				|| expectation.m_iExpectedNormalizedSlotAttemptCount != 0
				|| !expectation.m_sConfirmedCasualtySlotId.IsEmpty()
				|| !expectation.m_sCasualtyTombstoneFingerprint.IsEmpty()
				|| expectation.m_iExpectedNormalizedReprojectionCount != 0
				|| expectation.m_iLivingMemberCount
					!= expectation.m_iAcceptedMemberCount)
				return false;
		}
		else if (expectedCut == CUT_PHYSICAL_LIVE_POSITION)
		{
			bool normalizedExpectationExact
				= expectation.m_bExpectedLivingSlotsEverAlive
				&& expectation.m_iExpectedNormalizedSlotAttemptCount == 1
				&& expectation.m_iExpectedNormalizedReprojectionCount == 1
				&& expectation.m_iLivingMemberCount
					== expectation.m_iAcceptedMemberCount;
			bool casualtyExpectationExact
				= expectation.m_sConfirmedCasualtySlotId.IsEmpty()
				&& expectation.m_sCasualtyTombstoneFingerprint.IsEmpty();
			bool physicalCountsExact
				= carrier.m_iExpectedPhysicalAdapterHandleCount
					== expectation.m_iLivingMemberCount + 1
				&& carrier.m_iExpectedPhysicalRuntimeMemberCount
					== expectation.m_iLivingMemberCount;
			bool physicalPositionsExact
				= !IsZeroVector(carrier.m_vInjectedStalePosition)
				&& !IsZeroVector(carrier.m_vPreparedLivePosition)
				&& PositionsDiffer(
					carrier.m_vInjectedStalePosition,
					carrier.m_vPreparedLivePosition);
			if (!normalizedExpectationExact || !casualtyExpectationExact
				|| !physicalCountsExact || !physicalPositionsExact)
			{
				evidence = string.Format(
					"external counterattack physical carrier expectation/casualty/counts/positions rejected %1/%2/%3/%4",
					normalizedExpectationExact,
					casualtyExpectationExact,
					physicalCountsExact,
					physicalPositionsExact);
				return false;
			}
		}
		bool preparedProgressExact = carrier.m_fPreparedRouteProgressMeters > 0;
		if (expectedCut == CUT_DEMATERIALIZING_BEFORE_HOLD
			|| expectedCut == CUT_PHYSICAL_LIVE_POSITION)
			preparedProgressExact = carrier.m_fPreparedRouteProgressMeters >= 0;
		if (carrier.m_iPreparedElapsedSecond <= 0
			|| carrier.m_fPreparedRouteTotalDistanceMeters <= 0
			|| !preparedProgressExact
			|| carrier.m_fPreparedRouteProgressMeters
				>= carrier.m_fPreparedRouteTotalDistanceMeters
			|| IsZeroVector(carrier.m_vPreparedStrategicPosition))
		{
			evidence = string.Format(
				"external counterattack restart carrier clock/route/position rejected %1/%2/%3/%4",
				carrier.m_iPreparedElapsedSecond,
				carrier.m_fPreparedRouteProgressMeters,
				carrier.m_fPreparedRouteTotalDistanceMeters,
				carrier.m_vPreparedStrategicPosition);
			return false;
		}

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

	protected static bool ValidatePreparedSettlementResult(
		HST_EnemyCounterattackExternalRestartResult result,
		string expectedStage,
		out string evidence)
	{
		evidence = "external counterattack prepared-settlement result rejected";
		if (!result || !result.m_bPreparedCutExact
			|| !result.m_bCasualtyContinuityExact
			|| result.m_iPhysicalAdapterHandleCount != 0
			|| result.m_iPhysicalRuntimeMemberCount != 0
			|| result.m_bPhysicalBindingsExact
			|| result.m_bLivePositionRefreshExact
			|| result.m_bPhysicalCaptureNormalizedExact)
			return false;

		if (expectedStage == STAGE_PREPARE)
		{
			if (result.m_bRestored || result.m_bStartupReconcileChanged
				|| result.m_bContinuationExact
				|| result.m_bSameStateSemanticNoOp
				|| result.m_sSourceSemanticFingerprint
					!= result.m_sFinalSemanticFingerprint
				|| result.m_sRawPreparedCutSemanticFingerprint
					!= result.m_sSourceSemanticFingerprint)
				return false;
		}
		else if (expectedStage == STAGE_RECOVER)
		{
			if (!result.m_bRestored || !result.m_bStartupReconcileChanged
				|| !result.m_bContinuationExact
				|| !result.m_bSameStateSemanticNoOp
				|| result.m_sSourceSemanticFingerprint
					== result.m_sFinalSemanticFingerprint
				|| result.m_sRawPreparedCutSemanticFingerprint
					!= result.m_sSourceSemanticFingerprint)
				return false;
		}
		else if (expectedStage == STAGE_REPLAY)
		{
			if (!result.m_bRestored || result.m_bStartupReconcileChanged
				|| result.m_bContinuationExact
				|| !result.m_bSameStateSemanticNoOp
				|| result.m_sSourceSemanticFingerprint
					!= result.m_sFinalSemanticFingerprint
				|| result.m_sRawPreparedCutSemanticFingerprint
					== result.m_sSourceSemanticFingerprint)
				return false;
		}
		else
			return false;

		evidence = "external counterattack prepared-settlement result exact";
		return true;
	}

	static bool ValidateOwnerAppliedPendingResult(
		HST_EnemyCounterattackExternalRestartResult result,
		string expectedStage,
		out string evidence)
	{
		evidence = "external counterattack owner-applied pending result rejected";
		if (!result || !result.m_bSourceExact
			|| !result.m_bRuntimeClaimantsZero
			|| !result.m_bPersistedReadBackExact
			|| !result.m_bPreparedCutExact
			|| !result.m_bCasualtyContinuityExact
			|| result.m_iPhysicalAdapterHandleCount != 0
			|| result.m_iPhysicalRuntimeMemberCount != 0
			|| result.m_bPhysicalBindingsExact
			|| result.m_bLivePositionRefreshExact
			|| result.m_bPhysicalCaptureNormalizedExact
			|| result.m_sSourceSemanticFingerprint.IsEmpty()
			|| result.m_sFinalSemanticFingerprint.IsEmpty()
			|| result.m_sRawPreparedCutSemanticFingerprint.IsEmpty())
			return false;

		if (expectedStage == STAGE_PREPARE)
		{
			if (result.m_bRestored
				|| result.m_bStartupReconcileChanged
				|| result.m_bOwnershipStartupReconcileChanged
				|| result.m_bContinuationExact
				|| result.m_bSameStateSemanticNoOp
				|| result.m_sSourceSemanticFingerprint
					!= result.m_sFinalSemanticFingerprint
				|| result.m_sRawPreparedCutSemanticFingerprint
					== result.m_sSourceSemanticFingerprint)
				return false;
		}
		else if (expectedStage == STAGE_RECOVER)
		{
			if (!result.m_bRestored
				|| !result.m_bOwnershipStartupReconcileChanged
				|| !result.m_bContinuationExact
				|| !result.m_bSameStateSemanticNoOp
				|| result.m_sSourceSemanticFingerprint
					== result.m_sFinalSemanticFingerprint
				|| result.m_sRawPreparedCutSemanticFingerprint
					== result.m_sSourceSemanticFingerprint)
				return false;
		}
		else if (expectedStage == STAGE_REPLAY)
		{
			if (!result.m_bRestored
				|| result.m_bOwnershipStartupReconcileChanged
				|| result.m_bContinuationExact
				|| !result.m_bSameStateSemanticNoOp
				|| result.m_sSourceSemanticFingerprint
					!= result.m_sFinalSemanticFingerprint
				|| result.m_sRawPreparedCutSemanticFingerprint
					== result.m_sSourceSemanticFingerprint)
				return false;
		}
		else
			return false;

		evidence = "external counterattack owner-applied pending result exact";
		return true;
	}

	protected static bool ValidateNativeSourceSelectionResult(
		HST_EnemyCounterattackExternalRestartResult result,
		string expectedCut,
		string expectedStage,
		out string evidence)
	{
		evidence = "external counterattack native-source result rejected";
		if (!result)
			return false;
		if (!result.m_bNativeSourceSelectionProof)
		{
			bool legacyExact = !result.m_bNativePersistenceLoaded
				&& !result.m_bFallbackConflictRejected
				&& !result.m_bNativeSavePointCommitted
				&& result.m_sNativeSavePointId.IsEmpty()
				&& result.m_sRestoreSource.IsEmpty()
				&& result.m_sFallbackConflictSemanticFingerprint.IsEmpty();
			if (legacyExact)
				evidence = "external counterattack legacy source result exact";
			return legacyExact;
		}

		if (!IsOwnerAppliedPendingCut(expectedCut)
			|| !UUID.IsUUID(result.m_sNativeSavePointId)
			|| result.m_sFallbackConflictSemanticFingerprint.IsEmpty())
			return false;
		if (expectedStage == STAGE_PREPARE)
		{
			if (result.m_bNativePersistenceLoaded
				|| result.m_bFallbackConflictRejected
				|| !result.m_bNativeSavePointCommitted
				|| result.m_sRestoreSource != "new_campaign")
				return false;
		}
		else if (expectedStage == STAGE_RECOVER)
		{
			if (!result.m_bNativePersistenceLoaded
				|| !result.m_bFallbackConflictRejected
				|| !result.m_bNativeSavePointCommitted
				|| result.m_sRestoreSource != "native")
				return false;
		}
		else if (expectedStage == STAGE_REPLAY)
		{
			if (!result.m_bNativePersistenceLoaded
				|| !result.m_bFallbackConflictRejected
				|| result.m_bNativeSavePointCommitted
				|| result.m_sRestoreSource != "native")
				return false;
		}
		else
			return false;

		evidence = "external counterattack native-source result exact";
		return true;
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
		string sourceSelectionEvidence;
		if (!ValidateNativeSourceSelectionResult(
			result,
			expectedCut,
			expectedStage,
			sourceSelectionEvidence))
		{
			evidence = sourceSelectionEvidence;
			return false;
		}
		if (IsOwnerAppliedPendingCut(expectedCut))
		{
			return ValidateOwnerAppliedPendingResult(
				result,
				expectedStage,
				evidence);
		}
		if (result.m_bOwnershipStartupReconcileChanged)
			return false;
		bool physicalCut = expectedCut == CUT_PHYSICAL_LIVE_POSITION;
		if (!result.m_bSourceExact || !result.m_bRuntimeClaimantsZero
			|| !result.m_bPersistedReadBackExact
			|| result.m_sSourceSemanticFingerprint.IsEmpty()
			|| result.m_sFinalSemanticFingerprint.IsEmpty()
			|| result.m_sRawPreparedCutSemanticFingerprint.IsEmpty())
			return false;
		if (IsPreparedSettlementCut(expectedCut))
		{
			return ValidatePreparedSettlementResult(
				result,
				expectedStage,
				evidence);
		}
		if ((expectedCut == CUT_DEMATERIALIZING_BEFORE_HOLD
			|| expectedCut == CUT_MATERIALIZING_CHECKPOINT_DEFERRED
			|| physicalCut)
			&& (!result.m_bPreparedCutExact
				|| !result.m_bCasualtyContinuityExact))
			return false;
		if ((expectedCut == CUT_DEMATERIALIZING_BEFORE_HOLD
			|| expectedCut == CUT_MATERIALIZING_CHECKPOINT_DEFERRED
			|| physicalCut)
			&& (result.m_sRawPreparedCutSemanticFingerprint
					== result.m_sSourceSemanticFingerprint
				|| result.m_sRawPreparedCutSemanticFingerprint
					== result.m_sFinalSemanticFingerprint))
			return false;
		if (physicalCut)
		{
			if (result.m_iPhysicalRuntimeMemberCount <= 0
				|| result.m_iPhysicalAdapterHandleCount
					!= result.m_iPhysicalRuntimeMemberCount + 1
				|| IsZeroVector(result.m_vInjectedStalePosition)
				|| IsZeroVector(result.m_vPreparedLivePosition)
				|| !PositionsDiffer(
					result.m_vInjectedStalePosition,
					result.m_vPreparedLivePosition))
				return false;
			if (expectedStage == STAGE_PREPARE
				&& (!result.m_bPhysicalBindingsExact
					|| !result.m_bLivePositionRefreshExact
					|| !result.m_bPhysicalCaptureNormalizedExact))
				return false;
		}

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
				|| FileIO.FileExists(
					HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
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

		HST_CampaignProfileSaveJournalService profileJournal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignProfileSaveResolution profileResolution
			= profileJournal.ResolveJournal();
		if (!profileResolution || !profileResolution.m_bArtifactsPresent
			|| !profileResolution.m_bHasSelection
			|| !profileResolution.m_Selected
			|| profileResolution.m_bUnsupportedFuture
			|| profileResolution.m_bAmbiguous)
		{
			evidence = "external counterattack restart restore stage has no valid campaign journal generation";
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

	protected static bool PositionsDiffer(vector left, vector right)
	{
		return Math.AbsFloat(left[0] - right[0]) > 0.1
			|| Math.AbsFloat(left[1] - right[1]) > 0.1
			|| Math.AbsFloat(left[2] - right[2]) > 0.1;
	}
}
#endif
