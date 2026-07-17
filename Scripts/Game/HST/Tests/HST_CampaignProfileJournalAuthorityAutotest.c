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

[Test(suite: HST_CampaignProfileJournalAuthorityAutotestSuite)]
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
#endif
