#ifdef ENABLE_DIAG
// This suite is intended for an autotest runner with a disposable profile.
// The proof exercises the production journal paths and removes both slots.
class HST_CampaignProfileJournalAuthorityAutotestSuite
	: SCR_AutotestSuiteBase
{
	override ResourceName GetWorldFile()
	{
		return "";
	}
}

// Retained as an unregistered compatibility implementation. Gate 1 executes
// the individually registered invariant cases below without double counting.
class HST_TEST_CampaignProfileJournalAuthority : SCR_AutotestCaseBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		HST_CampaignProfileJournalAuthorityProofService proof
			= new HST_CampaignProfileJournalAuthorityProofService();
		HST_CampaignProfileJournalAuthorityProofReport report = proof.Run();
		if (!report)
		{
			SetResultFailure(
				"Campaign profile journal proof did not return a report");
			return true;
		}

		Print("Partisan campaign profile journal autotest | "
			+ HST_BuildInfo.BuildRuntimeSummary());
		Print(report.BuildReport());
		Print(report.m_sGenerationEvidence);
		Print(report.m_sGenerationPreservationEvidence);
		Print(report.m_sFaultFallbackEvidence);
		Print(report.m_sInvalidHistoryEvidence);
		Print(report.m_sFutureFormatEvidence);
		Print(report.m_sFutureRawEvidence);
		Print(report.m_sLegacyEvidence);
		Print(report.m_sSplitBrainEvidence);
		Print(report.m_sLineageEvidence);
		Print(report.m_sLineagePredicateEvidence);
		Print(report.m_sFutureWriteEvidence);
		Print(report.m_sReadOnlyEvidence);
		Print(report.m_sDegradedRecoveryEvidence);
		Print(report.m_sFallbackOnlyEvidence);
		Print(report.m_sFailedNativeCallbackEvidence);
		Print(report.m_sNativeArtifactToleranceEvidence);
		Print(report.m_sFutureNativeAuthorityEvidence);
		Print(report.m_sAuthorityOrderEvidence);
		Print(report.m_sAuthorityTieBreakEvidence);
		Print(report.m_sLegacyRawAuthorityEvidence);
		Print(report.m_sCleanupEvidence);

		AssertTrue(
			report.m_bGenerationAdvanceExact,
			"Journal generation advance failed: "
				+ report.m_sGenerationEvidence);
		AssertTrue(
			report.m_bCanonicalGenerationOnePreservedExact,
			"Canonical generation one changed during generation two write: "
				+ report.m_sGenerationPreservationEvidence);
		AssertTrue(
			report.m_bTruncatedNewestFallbackExact,
			"Truncated-newest fallback failed: "
				+ report.m_sFaultFallbackEvidence);
		AssertTrue(
			report.m_bBadFingerprintFallbackExact,
			"Bad-fingerprint fallback failed: "
				+ report.m_sFaultFallbackEvidence);
		AssertTrue(
			report.m_bBothInvalidRejectedExact,
			"Both-invalid rejection failed: "
				+ report.m_sInvalidHistoryEvidence);
		AssertTrue(
			report.m_bBothInvalidSourceFatalExact,
			"Both-invalid source did not fail closed: "
				+ report.m_sInvalidHistoryEvidence);
		AssertTrue(
			report.m_bFutureEnvelopeRejectedExact,
			"Future-envelope downgrade rejection failed: "
				+ report.m_sFutureFormatEvidence);
		AssertTrue(
			report.m_bUnknownMagicRejectedExact,
			"Unknown-envelope-magic downgrade rejection failed: "
				+ report.m_sFutureFormatEvidence);
		AssertTrue(
			report.m_bFutureSchemaRejectedExact,
			"Future-schema downgrade rejection failed: "
				+ report.m_sFutureFormatEvidence);
		AssertTrue(
			report.m_bFutureRawRejectedExact,
			"Future raw-canonical downgrade rejection failed: "
				+ report.m_sFutureRawEvidence);
		AssertTrue(
			report.m_bFutureArtifactWriteNonMutatingExact,
			"Future artifact was mutated by a writer: "
				+ report.m_sFutureFormatEvidence);
		AssertTrue(
			report.m_bLegacyRawUpgradeExact,
			"Legacy raw generation-zero upgrade failed: "
				+ report.m_sLegacyEvidence);
		AssertTrue(
			report.m_bSplitBrainRejectedExact,
			"Same-generation split-brain rejection failed: "
				+ report.m_sSplitBrainEvidence);
		AssertTrue(
			report.m_bBrokenChainRejectedExact,
			"Broken generation chain rejection failed: "
				+ report.m_sLineageEvidence);
		AssertTrue(
			report.m_bGenerationOneParentGenerationRejectedExact,
			"Generation-one nonzero parent generation rejection failed: "
				+ report.m_sLineagePredicateEvidence);
		AssertTrue(
			report.m_bAdjacentWrongParentRejectedExact,
			"Adjacent wrong-parent fingerprint rejection failed: "
				+ report.m_sLineagePredicateEvidence);
		AssertTrue(
			report.m_bNonAdjacentParentFingerprintRejectedExact,
			"Non-adjacent exact-parent-fingerprint rejection failed: "
				+ report.m_sLineagePredicateEvidence);
		AssertTrue(
			report.m_bDuplicateMetadataRejectedExact,
			"Duplicate metadata conflict rejection failed: "
				+ report.m_sLineageEvidence);
		AssertTrue(
			report.m_bFutureWriteNonMutatingExact,
			"Future-schema non-mutating write rejection failed: "
				+ report.m_sFutureWriteEvidence);
		AssertTrue(
			report.m_bSelectedReadOnlyExact,
			"Selected snapshot read changed journal bytes: "
				+ report.m_sReadOnlyEvidence);
		AssertTrue(
			report.m_bDegradedNativeRecoveryExact,
			"Degraded native recovery selection failed: "
				+ report.m_sDegradedRecoveryEvidence);
		AssertTrue(
			report.m_bFallbackOnlyCheckpointExact,
			"Fallback-only checkpoint failed: "
				+ report.m_sFallbackOnlyEvidence);
		AssertTrue(
			report.m_bFailedNativeCallbackNonMutatingExact,
			"Failed native callback changed the journal or omitted retry state: "
				+ report.m_sFailedNativeCallbackEvidence);
		AssertTrue(
			report.m_bValidNativeInvalidJournalExact,
			"Valid native authority did not tolerate invalid journal artifacts: "
				+ report.m_sNativeArtifactToleranceEvidence);
		AssertTrue(
			report.m_bValidNativeFutureJournalExact,
			"Valid native authority did not tolerate future journal artifacts: "
				+ report.m_sNativeArtifactToleranceEvidence);
		AssertTrue(
			report.m_bFutureNativeAuthorityRejectedExact,
			"Future native authority did not fail closed ahead of journal read: "
				+ report.m_sFutureNativeAuthorityEvidence);
		AssertTrue(
			report.m_bLegacyNativeFingerprintAcceptedExact,
			"Legacy native fingerprint was not validated and normalized: "
				+ report.m_sFutureNativeAuthorityEvidence);
		AssertTrue(
			report.m_bNativeV1LoadClassificationExact,
			"Native v1 load classification failed: "
				+ report.m_sFutureNativeAuthorityEvidence);
		AssertTrue(
			report.m_bNativeV2LoadClassificationExact,
			"Native v2 exact-payload load classification failed: "
				+ report.m_sFutureNativeAuthorityEvidence);
		AssertTrue(
			report.m_bNativeInvalidFingerprintClassificationExact,
			"Native invalid-fingerprint load classification failed: "
				+ report.m_sFutureNativeAuthorityEvidence);
		AssertTrue(
			report.m_bNativeFutureEnvelopeClassificationExact,
			"Native future-envelope load classification failed: "
				+ report.m_sFutureNativeAuthorityEvidence);
		AssertTrue(
			report.m_bNewerJournalAuthorityExact,
			"Newer journal authority selection failed: "
				+ report.m_sAuthorityOrderEvidence);
		AssertTrue(
			report.m_bNewerNativeAuthorityExact,
			"Newer native authority selection failed: "
				+ report.m_sAuthorityOrderEvidence);
		AssertTrue(
			report.m_bEqualOrderConflictRejectedExact,
			"Equal-order authority conflict did not fail closed: "
				+ report.m_sAuthorityOrderEvidence);
		AssertTrue(
			report.m_bLastSaveSecondNewerJournalExact,
			"Last-save-second did not select the newer journal: "
				+ report.m_sAuthorityTieBreakEvidence);
		AssertTrue(
			report.m_bLastSaveSecondNewerNativeExact,
			"Last-save-second did not select the newer native snapshot: "
				+ report.m_sAuthorityTieBreakEvidence);
		AssertTrue(
			report.m_bEqualOrderSameFingerprintNativeExact,
			"Equal-order same-fingerprint authority did not select native: "
				+ report.m_sAuthorityTieBreakEvidence);
		AssertTrue(
			report.m_bLegacyRawEqualOrderNativeExact,
			"Legacy raw equal-order authority manufactured a conflict: "
				+ report.m_sLegacyRawAuthorityEvidence);
		AssertTrue(
			report.m_bCheckpointSequenceOrderingExact,
			"Checkpoint sequence did not dominate legacy order fields: "
				+ report.m_sAuthorityTieBreakEvidence);
		AssertTrue(
			report.m_bAuthorityJournalMetadataExact,
			"Authority resolution omitted exact journal metadata: "
				+ report.m_sAuthorityOrderEvidence);
		AssertTrue(
			report.m_bCleanupExact,
			"Journal proof cleanup failed: " + report.m_sCleanupEvidence);
		AssertTrue(
			report.AllExact(),
			"Campaign profile journal authority proof failed: "
				+ report.BuildReport());
		SetResultSuccess();
		return true;
	}
}
// Individually registered cases expose every journal report invariant. The
// compatibility implementation above is intentionally absent from the suite.
class HST_CampaignProfileJournalIndividualAutotestBase
	: SCR_AutotestCaseBase
{
	static const int GENERATION_ADVANCE = 0;
	static const int CANONICAL_GENERATION_ONE_PRESERVED = 1;
	static const int TRUNCATED_NEWEST_FALLBACK = 2;
	static const int BAD_FINGERPRINT_FALLBACK = 3;
	static const int BOTH_INVALID_REJECTED = 4;
	static const int BOTH_INVALID_SOURCE_FATAL = 5;
	static const int FUTURE_ENVELOPE_REJECTED = 6;
	static const int UNKNOWN_MAGIC_REJECTED = 7;
	static const int FUTURE_SCHEMA_REJECTED = 8;
	static const int FUTURE_RAW_REJECTED = 9;
	static const int FUTURE_ARTIFACT_WRITE_NON_MUTATING = 10;
	static const int LEGACY_RAW_UPGRADE = 11;
	static const int SPLIT_BRAIN_REJECTED = 12;
	static const int BROKEN_CHAIN_REJECTED = 13;
	static const int GENERATION_ONE_PARENT_GENERATION_REJECTED = 14;
	static const int ADJACENT_WRONG_PARENT_REJECTED = 15;
	static const int NON_ADJACENT_PARENT_FINGERPRINT_REJECTED = 16;
	static const int DUPLICATE_METADATA_REJECTED = 17;
	static const int FUTURE_WRITE_NON_MUTATING = 18;
	static const int SELECTED_READ_ONLY = 19;
	static const int DEGRADED_NATIVE_RECOVERY = 20;
	static const int FALLBACK_ONLY_CHECKPOINT = 21;
	static const int FAILED_NATIVE_CALLBACK_NON_MUTATING = 22;
	static const int VALID_NATIVE_INVALID_JOURNAL = 23;
	static const int VALID_NATIVE_FUTURE_JOURNAL = 24;
	static const int FUTURE_NATIVE_AUTHORITY_REJECTED = 25;
	static const int LEGACY_NATIVE_FINGERPRINT_ACCEPTED = 26;
	static const int NATIVE_V1_LOAD_CLASSIFICATION = 27;
	static const int NATIVE_V2_LOAD_CLASSIFICATION = 28;
	static const int NATIVE_INVALID_FINGERPRINT_CLASSIFICATION = 29;
	static const int NATIVE_FUTURE_ENVELOPE_CLASSIFICATION = 30;
	static const int NEWER_JOURNAL_AUTHORITY = 31;
	static const int NEWER_NATIVE_AUTHORITY = 32;
	static const int EQUAL_ORDER_CONFLICT_REJECTED = 33;
	static const int LAST_SAVE_SECOND_NEWER_JOURNAL = 34;
	static const int LAST_SAVE_SECOND_NEWER_NATIVE = 35;
	static const int EQUAL_ORDER_SAME_FINGERPRINT_NATIVE = 36;
	static const int LEGACY_RAW_EQUAL_ORDER_NATIVE = 37;
	static const int CHECKPOINT_SEQUENCE_ORDERING = 38;
	static const int AUTHORITY_JOURNAL_METADATA = 39;
	static const int CLEANUP = 40;

	protected bool ExecuteScenario(int scenario)
	{
		HST_CampaignProfileJournalAuthorityProofService proof
			= new HST_CampaignProfileJournalAuthorityProofService();
		HST_CampaignProfileJournalAuthorityProofReport report = proof.Run();
		if (!report)
		{
			SetResultFailure(
				"Campaign profile journal proof did not return a report");
			return true;
		}

		bool exact;
		string invariant;
		string evidence;
		switch (scenario)
		{
			case GENERATION_ADVANCE:
				exact = report.m_bGenerationAdvanceExact;
				invariant = "generation advance";
				evidence = report.m_sGenerationEvidence;
				break;
			case CANONICAL_GENERATION_ONE_PRESERVED:
				exact = report.m_bCanonicalGenerationOnePreservedExact;
				invariant = "canonical generation-one preservation";
				evidence = report.m_sGenerationPreservationEvidence;
				break;
			case TRUNCATED_NEWEST_FALLBACK:
				exact = report.m_bTruncatedNewestFallbackExact;
				invariant = "truncated-newest fallback";
				evidence = report.m_sFaultFallbackEvidence;
				break;
			case BAD_FINGERPRINT_FALLBACK:
				exact = report.m_bBadFingerprintFallbackExact;
				invariant = "bad-fingerprint fallback";
				evidence = report.m_sFaultFallbackEvidence;
				break;
			case BOTH_INVALID_REJECTED:
				exact = report.m_bBothInvalidRejectedExact;
				invariant = "both-invalid rejection";
				evidence = report.m_sInvalidHistoryEvidence;
				break;
			case BOTH_INVALID_SOURCE_FATAL:
				exact = report.m_bBothInvalidSourceFatalExact;
				invariant = "both-invalid source fatality";
				evidence = report.m_sInvalidHistoryEvidence;
				break;
			case FUTURE_ENVELOPE_REJECTED:
				exact = report.m_bFutureEnvelopeRejectedExact;
				invariant = "future-envelope rejection";
				evidence = report.m_sFutureFormatEvidence;
				break;
			case UNKNOWN_MAGIC_REJECTED:
				exact = report.m_bUnknownMagicRejectedExact;
				invariant = "unknown-magic rejection";
				evidence = report.m_sFutureFormatEvidence;
				break;
			case FUTURE_SCHEMA_REJECTED:
				exact = report.m_bFutureSchemaRejectedExact;
				invariant = "future-schema rejection";
				evidence = report.m_sFutureFormatEvidence;
				break;
			case FUTURE_RAW_REJECTED:
				exact = report.m_bFutureRawRejectedExact;
				invariant = "future raw-canonical rejection";
				evidence = report.m_sFutureRawEvidence;
				break;
			case FUTURE_ARTIFACT_WRITE_NON_MUTATING:
				exact = report.m_bFutureArtifactWriteNonMutatingExact;
				invariant = "future-artifact non-mutating write";
				evidence = report.m_sFutureFormatEvidence;
				break;
			case LEGACY_RAW_UPGRADE:
				exact = report.m_bLegacyRawUpgradeExact;
				invariant = "legacy raw upgrade";
				evidence = report.m_sLegacyEvidence;
				break;
			case SPLIT_BRAIN_REJECTED:
				exact = report.m_bSplitBrainRejectedExact;
				invariant = "split-brain rejection";
				evidence = report.m_sSplitBrainEvidence;
				break;
			case BROKEN_CHAIN_REJECTED:
				exact = report.m_bBrokenChainRejectedExact;
				invariant = "broken-chain rejection";
				evidence = report.m_sLineageEvidence;
				break;
			case GENERATION_ONE_PARENT_GENERATION_REJECTED:
				exact = report.m_bGenerationOneParentGenerationRejectedExact;
				invariant = "generation-one parent-generation rejection";
				evidence = report.m_sLineagePredicateEvidence;
				break;
			case ADJACENT_WRONG_PARENT_REJECTED:
				exact = report.m_bAdjacentWrongParentRejectedExact;
				invariant = "adjacent wrong-parent rejection";
				evidence = report.m_sLineagePredicateEvidence;
				break;
			case NON_ADJACENT_PARENT_FINGERPRINT_REJECTED:
				exact = report.m_bNonAdjacentParentFingerprintRejectedExact;
				invariant = "non-adjacent parent-fingerprint rejection";
				evidence = report.m_sLineagePredicateEvidence;
				break;
			case DUPLICATE_METADATA_REJECTED:
				exact = report.m_bDuplicateMetadataRejectedExact;
				invariant = "duplicate-metadata rejection";
				evidence = report.m_sLineageEvidence;
				break;
			case FUTURE_WRITE_NON_MUTATING:
				exact = report.m_bFutureWriteNonMutatingExact;
				invariant = "future-schema non-mutating write";
				evidence = report.m_sFutureWriteEvidence;
				break;
			case SELECTED_READ_ONLY:
				exact = report.m_bSelectedReadOnlyExact;
				invariant = "selected read-only access";
				evidence = report.m_sReadOnlyEvidence;
				break;
			case DEGRADED_NATIVE_RECOVERY:
				exact = report.m_bDegradedNativeRecoveryExact;
				invariant = "degraded native recovery";
				evidence = report.m_sDegradedRecoveryEvidence;
				break;
			case FALLBACK_ONLY_CHECKPOINT:
				exact = report.m_bFallbackOnlyCheckpointExact;
				invariant = "fallback-only checkpoint";
				evidence = report.m_sFallbackOnlyEvidence;
				break;
			case FAILED_NATIVE_CALLBACK_NON_MUTATING:
				exact = report.m_bFailedNativeCallbackNonMutatingExact;
				invariant = "failed-native-callback non-mutation";
				evidence = report.m_sFailedNativeCallbackEvidence;
				break;
			case VALID_NATIVE_INVALID_JOURNAL:
				exact = report.m_bValidNativeInvalidJournalExact;
				invariant = "valid-native invalid-journal tolerance";
				evidence = report.m_sNativeArtifactToleranceEvidence;
				break;
			case VALID_NATIVE_FUTURE_JOURNAL:
				exact = report.m_bValidNativeFutureJournalExact;
				invariant = "valid-native future-journal tolerance";
				evidence = report.m_sNativeArtifactToleranceEvidence;
				break;
			case FUTURE_NATIVE_AUTHORITY_REJECTED:
				exact = report.m_bFutureNativeAuthorityRejectedExact;
				invariant = "future-native-authority rejection";
				evidence = report.m_sFutureNativeAuthorityEvidence;
				break;
			case LEGACY_NATIVE_FINGERPRINT_ACCEPTED:
				exact = report.m_bLegacyNativeFingerprintAcceptedExact;
				invariant = "legacy-native-fingerprint acceptance";
				evidence = report.m_sFutureNativeAuthorityEvidence;
				break;
			case NATIVE_V1_LOAD_CLASSIFICATION:
				exact = report.m_bNativeV1LoadClassificationExact;
				invariant = "native-v1 load classification";
				evidence = report.m_sFutureNativeAuthorityEvidence;
				break;
			case NATIVE_V2_LOAD_CLASSIFICATION:
				exact = report.m_bNativeV2LoadClassificationExact;
				invariant = "native-v2 load classification";
				evidence = report.m_sFutureNativeAuthorityEvidence;
				break;
			case NATIVE_INVALID_FINGERPRINT_CLASSIFICATION:
				exact = report.m_bNativeInvalidFingerprintClassificationExact;
				invariant = "native invalid-fingerprint classification";
				evidence = report.m_sFutureNativeAuthorityEvidence;
				break;
			case NATIVE_FUTURE_ENVELOPE_CLASSIFICATION:
				exact = report.m_bNativeFutureEnvelopeClassificationExact;
				invariant = "native future-envelope classification";
				evidence = report.m_sFutureNativeAuthorityEvidence;
				break;
			case NEWER_JOURNAL_AUTHORITY:
				exact = report.m_bNewerJournalAuthorityExact;
				invariant = "newer-journal authority";
				evidence = report.m_sAuthorityOrderEvidence;
				break;
			case NEWER_NATIVE_AUTHORITY:
				exact = report.m_bNewerNativeAuthorityExact;
				invariant = "newer-native authority";
				evidence = report.m_sAuthorityOrderEvidence;
				break;
			case EQUAL_ORDER_CONFLICT_REJECTED:
				exact = report.m_bEqualOrderConflictRejectedExact;
				invariant = "equal-order conflict rejection";
				evidence = report.m_sAuthorityOrderEvidence;
				break;
			case LAST_SAVE_SECOND_NEWER_JOURNAL:
				exact = report.m_bLastSaveSecondNewerJournalExact;
				invariant = "last-save-second newer-journal selection";
				evidence = report.m_sAuthorityTieBreakEvidence;
				break;
			case LAST_SAVE_SECOND_NEWER_NATIVE:
				exact = report.m_bLastSaveSecondNewerNativeExact;
				invariant = "last-save-second newer-native selection";
				evidence = report.m_sAuthorityTieBreakEvidence;
				break;
			case EQUAL_ORDER_SAME_FINGERPRINT_NATIVE:
				exact = report.m_bEqualOrderSameFingerprintNativeExact;
				invariant = "equal-order same-fingerprint native selection";
				evidence = report.m_sAuthorityTieBreakEvidence;
				break;
			case LEGACY_RAW_EQUAL_ORDER_NATIVE:
				exact = report.m_bLegacyRawEqualOrderNativeExact;
				invariant = "legacy-raw equal-order native selection";
				evidence = report.m_sLegacyRawAuthorityEvidence;
				break;
			case CHECKPOINT_SEQUENCE_ORDERING:
				exact = report.m_bCheckpointSequenceOrderingExact;
				invariant = "checkpoint-sequence ordering";
				evidence = report.m_sAuthorityTieBreakEvidence;
				break;
			case AUTHORITY_JOURNAL_METADATA:
				exact = report.m_bAuthorityJournalMetadataExact;
				invariant = "authority journal metadata";
				evidence = report.m_sAuthorityOrderEvidence;
				break;
			case CLEANUP:
				exact = report.m_bCleanupExact;
				invariant = "cleanup";
				evidence = report.m_sCleanupEvidence;
				break;
			default:
				SetResultFailure("Unknown campaign profile journal scenario");
				return true;
		}

		Print("Partisan campaign profile journal scenario | " + invariant
			+ " | " + HST_BuildInfo.BuildRuntimeSummary());
		Print(report.BuildReport());
		Print(evidence);
		AssertTrue(
			exact,
			"Campaign profile journal " + invariant + " proof failed: "
				+ evidence);
		SetResultSuccess();
		return true;
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_GenerationAdvance
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(GENERATION_ADVANCE);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_CanonicalGenerationOnePreserved
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(CANONICAL_GENERATION_ONE_PRESERVED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_TruncatedNewestFallback
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(TRUNCATED_NEWEST_FALLBACK);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_BadFingerprintFallback
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(BAD_FINGERPRINT_FALLBACK);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_BothInvalidRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(BOTH_INVALID_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_BothInvalidSourceFatal
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(BOTH_INVALID_SOURCE_FATAL);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_FutureEnvelopeRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(FUTURE_ENVELOPE_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_UnknownMagicRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(UNKNOWN_MAGIC_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_FutureSchemaRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(FUTURE_SCHEMA_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_FutureRawRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(FUTURE_RAW_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_FutureArtifactWriteNonMutating
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(FUTURE_ARTIFACT_WRITE_NON_MUTATING);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_LegacyRawUpgrade
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(LEGACY_RAW_UPGRADE);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_SplitBrainRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(SPLIT_BRAIN_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_BrokenChainRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(BROKEN_CHAIN_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_GenerationOneParentGenerationRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(GENERATION_ONE_PARENT_GENERATION_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_AdjacentWrongParentRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(ADJACENT_WRONG_PARENT_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_NonAdjacentParentFingerprintRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(NON_ADJACENT_PARENT_FINGERPRINT_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_DuplicateMetadataRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(DUPLICATE_METADATA_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_FutureWriteNonMutating
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(FUTURE_WRITE_NON_MUTATING);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_SelectedReadOnly
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(SELECTED_READ_ONLY);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_DegradedNativeRecovery
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(DEGRADED_NATIVE_RECOVERY);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_FallbackOnlyCheckpoint
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(FALLBACK_ONLY_CHECKPOINT);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_FailedNativeCallbackNonMutating
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(FAILED_NATIVE_CALLBACK_NON_MUTATING);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_ValidNativeInvalidJournal
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(VALID_NATIVE_INVALID_JOURNAL);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_ValidNativeFutureJournal
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(VALID_NATIVE_FUTURE_JOURNAL);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_FutureNativeAuthorityRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(FUTURE_NATIVE_AUTHORITY_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_LegacyNativeFingerprintAccepted
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(LEGACY_NATIVE_FINGERPRINT_ACCEPTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_NativeV1LoadClassification
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(NATIVE_V1_LOAD_CLASSIFICATION);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_NativeV2LoadClassification
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(NATIVE_V2_LOAD_CLASSIFICATION);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_NativeInvalidFingerprintClassification
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(NATIVE_INVALID_FINGERPRINT_CLASSIFICATION);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_NativeFutureEnvelopeClassification
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(NATIVE_FUTURE_ENVELOPE_CLASSIFICATION);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_NewerJournalAuthority
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(NEWER_JOURNAL_AUTHORITY);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_NewerNativeAuthority
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(NEWER_NATIVE_AUTHORITY);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_EqualOrderConflictRejected
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(EQUAL_ORDER_CONFLICT_REJECTED);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_LastSaveSecondNewerJournal
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(LAST_SAVE_SECOND_NEWER_JOURNAL);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_LastSaveSecondNewerNative
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(LAST_SAVE_SECOND_NEWER_NATIVE);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_EqualOrderSameFingerprintNative
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(EQUAL_ORDER_SAME_FINGERPRINT_NATIVE);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_LegacyRawEqualOrderNative
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(LEGACY_RAW_EQUAL_ORDER_NATIVE);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_CheckpointSequenceOrdering
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(CHECKPOINT_SEQUENCE_ORDERING);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_AuthorityJournalMetadata
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(AUTHORITY_JOURNAL_METADATA);
	}
}

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
class HST_TEST_CampaignProfileJournalAuthority_Cleanup
	: HST_CampaignProfileJournalIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(CLEANUP);
	}
}
#endif
