#ifdef ENABLE_DIAG
// Synchronous profile-artifact I/O for the disposable exact-QRF external
// process-restart proof. The nonce, build, schema, cut, and stage gates prevent
// stale artifacts from being mistaken for evidence from the current run.
class HST_EnemyQRFExternalRestartProofService
{
	static const string GUARD_MAGIC = "partisan_exact_qrf_restart_guard_v1";
	static const string CARRIER_MAGIC = "partisan_exact_qrf_restart_carrier_v1";
	static const string RESULT_MAGIC = "partisan_exact_qrf_restart_result_v1";
	static const string CUT_BEFORE_REFUND = "before_refund";
	static const string CUT_AFTER_REFUND = "after_refund";
	static const string CUT_AFTER_RECEIPT = "after_receipt";
	static const string STAGE_PREPARE = "prepare";
	static const string STAGE_RECOVER = "recover";
	static const string STAGE_REPLAY = "replay";
	static const int MAX_RUN_ID_CHARACTERS = 64;

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

	static int ResolveCut(string cutName)
	{
		if (cutName == CUT_BEFORE_REFUND)
			return 0;
		if (cutName == CUT_AFTER_REFUND)
			return 1;
		if (cutName == CUT_AFTER_RECEIPT)
			return 2;
		return -1;
	}

	static string CutName(int cut)
	{
		if (cut == 0)
			return CUT_BEFORE_REFUND;
		if (cut == 1)
			return CUT_AFTER_REFUND;
		if (cut == 2)
			return CUT_AFTER_RECEIPT;
		return "";
	}

	static bool ValidateStage(string stage)
	{
		return stage == STAGE_PREPARE
			|| stage == STAGE_RECOVER
			|| stage == STAGE_REPLAY;
	}

	static string BuildGuardPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_ExactQRFRestart_" + safeRunId + ".guard.json";
	}

	static string BuildCarrierPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_ExactQRFRestart_" + safeRunId + ".carrier.json";
	}

	static string BuildResultPath(string runId, string stage)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty() || !ValidateStage(stage))
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_ExactQRFRestart_" + safeRunId + "." + stage + ".json";
	}

	static bool ValidateGuard(
		HST_EnemyQRFExternalRestartGuard guard,
		string expectedRunId,
		string expectedCut,
		out string evidence)
	{
		evidence = "external restart guard rejected";
		if (!guard || !ValidateRunId(expectedRunId))
			return false;
		if (ResolveCut(expectedCut) < 0)
			return false;
		if (guard.m_sMagic != GUARD_MAGIC)
			return false;
		if (guard.m_sRunId != expectedRunId
			|| SanitizeRunId(guard.m_sRunId) != guard.m_sRunId)
			return false;
		if (guard.m_sRequestedCut != expectedCut)
			return false;
		if (!guard.m_bAllowCanonicalCampaignOverwrite)
		{
			evidence = "external restart guard does not authorize disposable canonical campaign overwrite";
			return false;
		}
		evidence = "external restart guard exact";
		return true;
	}

	static bool SaveGuard(
		HST_EnemyQRFExternalRestartGuard guard,
		out string evidence)
	{
		if (!guard || !ValidateGuard(
			guard,
			guard.m_sRunId,
			guard.m_sRequestedCut,
			evidence))
			return false;
		string path = BuildGuardPath(guard.m_sRunId);
		if (path.IsEmpty())
			return false;
		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", guard) || !context.SaveToFile(path))
		{
			evidence = "external restart guard write failed";
			return false;
		}
		evidence = "external restart guard saved";
		return true;
	}

	static bool LoadGuard(
		string runId,
		out HST_EnemyQRFExternalRestartGuard guard,
		out string evidence)
	{
		guard = null;
		evidence = "external restart guard unavailable";
		string path = BuildGuardPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		guard = new HST_EnemyQRFExternalRestartGuard();
		if (!context.ReadValue("", guard))
		{
			guard = null;
			return false;
		}
		evidence = "external restart guard loaded";
		return true;
	}

	static bool LoadAndValidateGuard(
		string runId,
		string cutName,
		out HST_EnemyQRFExternalRestartGuard guard,
		out string evidence)
	{
		if (!LoadGuard(runId, guard, evidence))
			return false;
		return ValidateGuard(guard, runId, cutName, evidence);
	}

	static bool ValidateCarrier(
		HST_EnemyQRFExternalRestartCarrier carrier,
		string expectedRunId,
		string expectedCut,
		out string evidence)
	{
		evidence = "external restart carrier rejected";
		int expectedCutValue = ResolveCut(expectedCut);
		if (!carrier || !ValidateRunId(expectedRunId) || expectedCutValue < 0)
			return false;
		if (carrier.m_sMagic != CARRIER_MAGIC
			|| carrier.m_sRunId != expectedRunId
			|| SanitizeRunId(carrier.m_sRunId) != carrier.m_sRunId)
			return false;
		if (carrier.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| carrier.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| carrier.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL)
			return false;
		if (carrier.m_iCampaignSchemaVersion != HST_CampaignState.SCHEMA_VERSION)
			return false;
		if (carrier.m_sCutName != expectedCut || carrier.m_iCut != expectedCutValue)
			return false;
		if (carrier.m_sWorld.IsEmpty() || !carrier.m_Expectation
			|| carrier.m_sPreparedFingerprint.IsEmpty()
			|| carrier.m_sPreparedFingerprint.StartsWith("prepared-recovery-prefix-"))
			return false;
		if (carrier.m_Expectation.m_sOrderId.IsEmpty()
			|| carrier.m_Expectation.m_sOperationId.IsEmpty()
			|| carrier.m_Expectation.m_sManifestId.IsEmpty()
			|| carrier.m_Expectation.m_sManifestHash.IsEmpty()
			|| carrier.m_Expectation.m_sBatchId.IsEmpty()
			|| carrier.m_Expectation.m_sGroupId.IsEmpty())
			return false;
		if (carrier.m_Expectation.m_iAttackCost != 12
			|| carrier.m_Expectation.m_iSupportCost != 8)
			return false;
		if (carrier.m_iAccepted <= 1 || carrier.m_iCasualties <= 0
			|| carrier.m_iSurvivors <= 0
			|| carrier.m_iSurvivors != carrier.m_iAccepted - carrier.m_iCasualties)
			return false;
		if (carrier.m_Expectation.m_iAccepted != carrier.m_iAccepted
			|| carrier.m_Expectation.m_iSurvivors != carrier.m_iSurvivors
			|| carrier.m_Expectation.m_iAttackRefund != carrier.m_iAttackRefund
			|| carrier.m_Expectation.m_iSupportRefund != carrier.m_iSupportRefund)
			return false;
		if (carrier.m_iAttackRefund
				!= 12 * carrier.m_iSurvivors / carrier.m_iAccepted
			|| carrier.m_iSupportRefund
				!= 8 * carrier.m_iSurvivors / carrier.m_iAccepted)
			return false;
		if (carrier.m_Expectation.m_iExpectedAttackPool
				!= carrier.m_iAttackBeforeRefund + carrier.m_iAttackRefund
			|| carrier.m_Expectation.m_iExpectedSupportPool
				!= carrier.m_iSupportBeforeRefund + carrier.m_iSupportRefund)
			return false;
		if (carrier.m_Expectation.m_sSettlementKind != carrier.m_sSettlementKind
			|| carrier.m_Expectation.m_sSettlementId != carrier.m_sSettlementId
			|| carrier.m_Expectation.m_sRefundMutationId != carrier.m_sRefundMutationId
			|| carrier.m_Expectation.m_sReason != carrier.m_sReason)
			return false;
		if (carrier.m_sSettlementKind != "invalidated_survivors"
			|| carrier.m_sSettlementId
				!= HST_OperationService.BuildSettlementId(
					carrier.m_Expectation.m_sOperationId,
					carrier.m_sSettlementKind)
			|| carrier.m_sRefundMutationId
				!= "enemy_resource_refund_" + carrier.m_sSettlementId
			|| carrier.m_sReason
				!= "exact enemy defensive QRF durable prepared recovery proof")
			return false;
		if (carrier.m_iPreparedAtSecond <= 0 || carrier.m_iPrefixRevision <= 0
			|| carrier.m_Expectation.m_iPreparedAtSecond != carrier.m_iPreparedAtSecond)
			return false;
		int expectedMutationCount;
		int expectedAttackDelta;
		int expectedSupportDelta;
		bool expectedReceipt;
		if (expectedCutValue >= 1)
		{
			expectedMutationCount = 1;
			expectedAttackDelta = carrier.m_iAttackRefund;
			expectedSupportDelta = carrier.m_iSupportRefund;
		}
		if (expectedCutValue >= 2)
			expectedReceipt = true;
		if (carrier.m_iExpectedPrefixMutationCount != expectedMutationCount
			|| carrier.m_iExpectedPrefixAttackDelta != expectedAttackDelta
			|| carrier.m_iExpectedPrefixSupportDelta != expectedSupportDelta
			|| carrier.m_bExpectedPrefixReceiptApplied != expectedReceipt)
			return false;
		int expectedTerminalRevision = carrier.m_iPrefixRevision + 2;
		if (expectedReceipt)
			expectedTerminalRevision = carrier.m_iPrefixRevision + 1;
		if (carrier.m_Expectation.m_iExpectedTerminalRevision
			!= expectedTerminalRevision)
			return false;
		evidence = "external restart carrier exact";
		return true;
	}

	static bool SaveCarrier(
		HST_EnemyQRFExternalRestartCarrier carrier,
		out string evidence)
	{
		if (!carrier || !ValidateCarrier(
			carrier,
			carrier.m_sRunId,
			carrier.m_sCutName,
			evidence))
			return false;
		string path = BuildCarrierPath(carrier.m_sRunId);
		if (path.IsEmpty())
			return false;
		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", carrier) || !context.SaveToFile(path))
		{
			evidence = "external restart carrier write failed";
			return false;
		}
		evidence = "external restart carrier saved";
		return true;
	}

	static bool LoadCarrier(
		string runId,
		string cutName,
		out HST_EnemyQRFExternalRestartCarrier carrier,
		out string evidence)
	{
		carrier = null;
		evidence = "external restart carrier unavailable";
		string path = BuildCarrierPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		carrier = new HST_EnemyQRFExternalRestartCarrier();
		if (!context.ReadValue("", carrier))
		{
			carrier = null;
			return false;
		}
		return ValidateCarrier(carrier, runId, cutName, evidence);
	}

	static bool ValidateResult(
		HST_EnemyQRFExternalRestartResult result,
		string expectedRunId,
		string expectedCut,
		string expectedStage,
		out string evidence)
	{
		evidence = "external restart result rejected";
		int expectedCutValue = ResolveCut(expectedCut);
		if (!result || !ValidateRunId(expectedRunId)
			|| expectedCutValue < 0 || !ValidateStage(expectedStage))
			return false;
		if (result.m_sMagic != RESULT_MAGIC
			|| result.m_sRunId != expectedRunId
			|| SanitizeRunId(result.m_sRunId) != result.m_sRunId)
			return false;
		if (result.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| result.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| result.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL)
			return false;
		if (result.m_iCampaignSchemaVersion != HST_CampaignState.SCHEMA_VERSION)
			return false;
		if (result.m_sCutName != expectedCut || result.m_iCut != expectedCutValue
			|| result.m_sStage != expectedStage)
			return false;
		if (result.m_sEvidence.IsEmpty())
			return false;
		if (result.m_bSuccess)
		{
			if (!result.m_bSourceExact || !result.m_bPersistedReadBackExact
				|| result.m_sFingerprint.IsEmpty())
				return false;
			if (expectedStage == STAGE_PREPARE
				&& (result.m_bRestored
					|| result.m_bStartupReconcileChanged
					|| result.m_bSameStateNoOp))
				return false;
			if (expectedStage == STAGE_RECOVER
				&& (!result.m_bRestored
					|| !result.m_bStartupReconcileChanged
					|| !result.m_bSameStateNoOp))
				return false;
			if (expectedStage == STAGE_REPLAY
				&& (!result.m_bRestored
					|| result.m_bStartupReconcileChanged
					|| !result.m_bSameStateNoOp))
				return false;
		}
		evidence = "external restart result exact";
		return true;
	}

	// Call only after every stage-specific read-back check has completed. The
	// result file is the final durable write for that process stage.
	static bool SaveResult(
		HST_EnemyQRFExternalRestartResult result,
		out string evidence)
	{
		if (!result || !ValidateResult(
			result,
			result.m_sRunId,
			result.m_sCutName,
			result.m_sStage,
			evidence))
			return false;
		string path = BuildResultPath(result.m_sRunId, result.m_sStage);
		if (path.IsEmpty())
			return false;
		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", result) || !context.SaveToFile(path))
		{
			evidence = "external restart result write failed";
			return false;
		}
		evidence = "external restart result saved";
		return true;
	}

	static bool LoadResult(
		string runId,
		string cutName,
		string stage,
		out HST_EnemyQRFExternalRestartResult result,
		out string evidence)
	{
		result = null;
		evidence = "external restart result unavailable";
		string path = BuildResultPath(runId, stage);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		result = new HST_EnemyQRFExternalRestartResult();
		if (!context.ReadValue("", result))
		{
			result = null;
			return false;
		}
		return ValidateResult(result, runId, cutName, stage, evidence);
	}
}
#endif
