#ifdef ENABLE_DIAG
// Diagnostic evidence from exercising the production two-slot campaign JSON
// journal against the process-local profile directory.
class HST_CampaignProfileJournalAuthorityProofReport
{
	bool m_bGenerationAdvanceExact;
	bool m_bCanonicalGenerationOnePreservedExact;
	bool m_bTruncatedNewestFallbackExact;
	bool m_bBadFingerprintFallbackExact;
	bool m_bBothInvalidRejectedExact;
	bool m_bBothInvalidSourceFatalExact;
	bool m_bFutureEnvelopeRejectedExact;
	bool m_bUnknownMagicRejectedExact;
	bool m_bFutureSchemaRejectedExact;
	bool m_bFutureRawRejectedExact;
	bool m_bFutureArtifactWriteNonMutatingExact;
	bool m_bLegacyRawUpgradeExact;
	bool m_bSplitBrainRejectedExact;
	bool m_bBrokenChainRejectedExact;
	bool m_bGenerationOneParentGenerationRejectedExact;
	bool m_bAdjacentWrongParentRejectedExact;
	bool m_bNonAdjacentParentFingerprintRejectedExact;
	bool m_bDuplicateMetadataRejectedExact;
	bool m_bFutureWriteNonMutatingExact;
	bool m_bSelectedReadOnlyExact;
	bool m_bDegradedNativeRecoveryExact;
	bool m_bFallbackOnlyCheckpointExact;
	bool m_bFailedNativeCallbackNonMutatingExact;
	bool m_bValidNativeInvalidJournalExact;
	bool m_bValidNativeFutureJournalExact;
	bool m_bFutureNativeAuthorityRejectedExact;
	bool m_bLegacyNativeFingerprintAcceptedExact;
	bool m_bNativeV1LoadClassificationExact;
	bool m_bNativeV2LoadClassificationExact;
	bool m_bNativeInvalidFingerprintClassificationExact;
	bool m_bNativeFutureEnvelopeClassificationExact;
	bool m_bNewerJournalAuthorityExact;
	bool m_bNewerNativeAuthorityExact;
	bool m_bEqualOrderConflictRejectedExact;
	bool m_bLastSaveSecondNewerJournalExact;
	bool m_bLastSaveSecondNewerNativeExact;
	bool m_bEqualOrderSameFingerprintNativeExact;
	bool m_bLegacyRawEqualOrderNativeExact;
	bool m_bCheckpointSequenceOrderingExact;
	bool m_bAuthorityJournalMetadataExact;
	bool m_bCleanupExact;

	string m_sGenerationEvidence;
	string m_sGenerationPreservationEvidence;
	string m_sFaultFallbackEvidence;
	string m_sInvalidHistoryEvidence;
	string m_sFutureFormatEvidence;
	string m_sFutureRawEvidence;
	string m_sLegacyEvidence;
	string m_sSplitBrainEvidence;
	string m_sLineageEvidence;
	string m_sLineagePredicateEvidence;
	string m_sFutureWriteEvidence;
	string m_sReadOnlyEvidence;
	string m_sDegradedRecoveryEvidence;
	string m_sFallbackOnlyEvidence;
	string m_sFailedNativeCallbackEvidence;
	string m_sNativeArtifactToleranceEvidence;
	string m_sFutureNativeAuthorityEvidence;
	string m_sAuthorityOrderEvidence;
	string m_sAuthorityTieBreakEvidence;
	string m_sLegacyRawAuthorityEvidence;
	string m_sCleanupEvidence;

	bool AllExact()
	{
		bool generationExact = m_bGenerationAdvanceExact
			&& m_bCanonicalGenerationOnePreservedExact
			&& m_bTruncatedNewestFallbackExact
			&& m_bBadFingerprintFallbackExact;
		bool rejectionExact = m_bBothInvalidRejectedExact
			&& m_bBothInvalidSourceFatalExact
			&& m_bFutureNativeAuthorityRejectedExact
			&& m_bFutureEnvelopeRejectedExact
			&& m_bUnknownMagicRejectedExact
			&& m_bFutureSchemaRejectedExact
			&& m_bFutureRawRejectedExact
			&& m_bFutureArtifactWriteNonMutatingExact;
		bool compatibilityExact = m_bLegacyRawUpgradeExact
			&& m_bSplitBrainRejectedExact
			&& m_bBrokenChainRejectedExact
			&& m_bGenerationOneParentGenerationRejectedExact
			&& m_bAdjacentWrongParentRejectedExact
			&& m_bNonAdjacentParentFingerprintRejectedExact
			&& m_bDuplicateMetadataRejectedExact
			&& m_bFutureWriteNonMutatingExact;
		bool runtimeRecoveryExact = m_bSelectedReadOnlyExact
			&& m_bDegradedNativeRecoveryExact
			&& m_bFallbackOnlyCheckpointExact
			&& m_bFailedNativeCallbackNonMutatingExact
			&& m_bValidNativeInvalidJournalExact
			&& m_bValidNativeFutureJournalExact
			&& m_bLegacyNativeFingerprintAcceptedExact
			&& m_bNativeV1LoadClassificationExact
			&& m_bNativeV2LoadClassificationExact
			&& m_bNativeInvalidFingerprintClassificationExact
			&& m_bNativeFutureEnvelopeClassificationExact;
		bool authorityOrderingExact = m_bNewerJournalAuthorityExact
			&& m_bNewerNativeAuthorityExact
			&& m_bEqualOrderConflictRejectedExact
			&& m_bLastSaveSecondNewerJournalExact
			&& m_bLastSaveSecondNewerNativeExact
			&& m_bEqualOrderSameFingerprintNativeExact
			&& m_bLegacyRawEqualOrderNativeExact
			&& m_bCheckpointSequenceOrderingExact
			&& m_bAuthorityJournalMetadataExact
			&& m_bCleanupExact;
		return generationExact && rejectionExact
			&& compatibilityExact && runtimeRecoveryExact
			&& authorityOrderingExact;
	}

	string BuildReport()
	{
		string first = string.Format(
			"Partisan campaign profile journal authority proof | generation %1 | truncated/bad fingerprint %2/%3 | both invalid %4 | future envelope/schema %5/%6",
			m_bGenerationAdvanceExact,
			m_bTruncatedNewestFallbackExact,
			m_bBadFingerprintFallbackExact,
			m_bBothInvalidRejectedExact,
			m_bFutureEnvelopeRejectedExact,
			m_bFutureSchemaRejectedExact);
		string second = string.Format(
			" | invalid-source/future-write %1/%2 | legacy/split/lineage %3/%4/%5 | future input immutable %6",
			m_bBothInvalidSourceFatalExact,
			m_bFutureArtifactWriteNonMutatingExact,
			m_bLegacyRawUpgradeExact,
			m_bSplitBrainRejectedExact,
			m_bBrokenChainRejectedExact && m_bDuplicateMetadataRejectedExact,
			m_bFutureWriteNonMutatingExact);
		string third = string.Format(
			" | selected read-only/degraded/fallback-only %1/%2/%3 | newer journal/native %4/%5 | equal-order conflict %6 | cleanup %7",
			m_bSelectedReadOnlyExact,
			m_bDegradedNativeRecoveryExact,
			m_bFallbackOnlyCheckpointExact,
			m_bNewerJournalAuthorityExact,
			m_bNewerNativeAuthorityExact,
			m_bEqualOrderConflictRejectedExact,
			m_bCleanupExact);
		string fourth = string.Format(
			" | gen1 preserved/future raw %1/%2 | isolated lineage adjacent/non-adjacent %3/%4 | valid-native invalid/future %5/%6",
			m_bCanonicalGenerationOnePreservedExact,
			m_bFutureRawRejectedExact,
			m_bAdjacentWrongParentRejectedExact,
			m_bNonAdjacentParentFingerprintRejectedExact,
			m_bValidNativeInvalidJournalExact,
			m_bValidNativeFutureJournalExact);
		string fifth = string.Format(
			" | last-save journal/native %1/%2 | equal fingerprint native %3 | checkpoint order %4 | authority metadata %5 | future native fatal %6",
			m_bLastSaveSecondNewerJournalExact,
			m_bLastSaveSecondNewerNativeExact,
			m_bEqualOrderSameFingerprintNativeExact,
			m_bCheckpointSequenceOrderingExact,
			m_bAuthorityJournalMetadataExact,
			m_bFutureNativeAuthorityRejectedExact);
		string sixth = string.Format(
			" | failed native callback non-mutating %1 | legacy raw equal-order native %2",
			m_bFailedNativeCallbackNonMutatingExact,
			m_bLegacyRawEqualOrderNativeExact);
		string seventh = string.Format(
			" | unknown magic/gen1 parent/legacy native fingerprint %1/%2/%3",
			m_bUnknownMagicRejectedExact,
			m_bGenerationOneParentGenerationRejectedExact,
			m_bLegacyNativeFingerprintAcceptedExact);
		string eighth = string.Format(
			" | native load v1/v2/bad fingerprint/future %1/%2/%3/%4",
			m_bNativeV1LoadClassificationExact,
			m_bNativeV2LoadClassificationExact,
			m_bNativeInvalidFingerprintClassificationExact,
			m_bNativeFutureEnvelopeClassificationExact);
		return first + second + third + fourth + fifth + sixth + seventh + eighth
			+ string.Format(" | exact %1", AllExact());
	}
}
#endif
