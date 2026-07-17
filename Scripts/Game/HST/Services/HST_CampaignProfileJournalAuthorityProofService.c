#ifdef ENABLE_DIAG
[BaseContainerProps()]
class HST_CampaignNativeEnvelopeClassificationProofData
{
	string magic;
	int version;
	bool snapshotPresent;
	string snapshotFingerprint;
	string snapshotPayload;
	ref HST_CampaignSaveData snapshot;
}

// Destructive diagnostic coverage for the process-local campaign profile
// journal. Autotest runners provide a disposable profile; every scenario also
// removes both journal slots before use and the proof removes them on exit.
class HST_CampaignProfileJournalAuthorityProofService
{
	static const string NATIVE_FAILURE = "journal_autotest_native_failure";
	static const string DISPOSABLE_PROFILE_SENTINEL
		= "$profile:.partisan-focused-owner";

	HST_CampaignProfileJournalAuthorityProofReport Run()
	{
		HST_CampaignProfileJournalAuthorityProofReport report
			= new HST_CampaignProfileJournalAuthorityProofReport();
		if (!IsDisposableProfileAuthorized())
		{
			report.m_sCleanupEvidence
				= "disposable profile ownership sentinel missing";
			return report;
		}
		HST_ProfilePathService.EnsureProfileDirectory();
		CleanupJournalArtifacts();

		ProveGenerationAdvanceAndFaultFallback(report);
		ProveCanonicalGenerationOnePreservation(report);
		ProveBothInvalidRejection(report);
		ProveFutureFormatRejection(report);
		ProveFutureRawRejection(report);
		ProveLegacyRawUpgrade(report);
		ProveSplitBrainRejection(report);
		ProveLineageResolution(report);
		ProveIsolatedLineagePredicates(report);
		ProveFutureWriteIsNonMutating(report);
		ProveFallbackOnlyCheckpoint(report);
		ProveFailedNativeCheckpointCallback(report);
		ProveValidNativeInvalidJournalTolerance(report);
		ProveValidNativeFutureJournalTolerance(report);
		ProveNativeFingerprintVersioning(report);
		ProveFutureNativeAuthorityRejection(report);
		ProveAuthorityOrdering(report);
		ProveAuthorityTieBreakOrdering(report);
		ProveLegacyRawEqualOrderAuthority(report);

		report.m_bCleanupExact = CleanupJournalArtifacts();
		report.m_sCleanupEvidence = string.Format(
			"final canonical/recovery present %1/%2",
			FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_SAVE_FILE),
			FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE));
		return report;
	}

	protected bool IsDisposableProfileAuthorized()
	{
		if (!FileIO.FileExists(DISPOSABLE_PROFILE_SENTINEL))
			return false;
		return SCR_FileIOHelper.GetFileStringContent(
			DISPOSABLE_PROFILE_SENTINEL,
			false) == "owned";
	}

	protected void ProveGenerationAdvanceAndFaultFallback(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData saveOne;
		HST_CampaignSaveData saveTwo;
		HST_CampaignSaveData saveThree;
		HST_CampaignProfileSaveWriteReceipt receiptOne;
		HST_CampaignProfileSaveWriteReceipt receiptTwo;
		HST_CampaignProfileSaveWriteReceipt receiptThree;
		string setupEvidence;
		bool setupExact = EstablishThreeGenerations(
			journal,
			1100,
			saveOne,
			saveTwo,
			saveThree,
			receiptOne,
			receiptTwo,
			receiptThree,
			setupEvidence);

		string canonicalGenerationThree = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		string recoveryGenerationTwo = ReadFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_CampaignProfileSaveResolution resolved = journal.ResolveJournal();
		bool selectedGenerationThree = setupExact && resolved
			&& resolved.m_bHasSelection && !resolved.m_bAmbiguous
			&& !resolved.m_bUnsupportedFuture && resolved.m_bChainExact
			&& resolved.m_iValidCandidateCount == 2
			&& resolved.m_Selected
			&& resolved.m_Selected.m_sSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			&& resolved.m_Selected.m_iGeneration == 3
			&& resolved.m_Selected.m_sSnapshotFingerprint
				== receiptThree.m_sSnapshotFingerprint;
		bool generationIdentityExact = setupExact
			&& receiptOne.m_bSuccess && receiptOne.m_iGeneration == 1
			&& receiptOne.m_sWrittenSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			&& receiptOne.m_iPreviousGeneration == 0
			&& receiptTwo.m_bSuccess && receiptTwo.m_iGeneration == 2
			&& receiptTwo.m_sWrittenSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_RECOVERY
			&& receiptTwo.m_iPreviousGeneration == 1
			&& receiptTwo.m_sPreviousSnapshotFingerprint
				== receiptOne.m_sSnapshotFingerprint
			&& receiptThree.m_bSuccess && receiptThree.m_iGeneration == 3
			&& receiptThree.m_sWrittenSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			&& receiptThree.m_iPreviousGeneration == 2
			&& receiptThree.m_sPreviousSnapshotFingerprint
				== receiptTwo.m_sSnapshotFingerprint;
		bool inactiveSlotPreserved = setupExact
			&& !canonicalGenerationThree.IsEmpty()
			&& !recoveryGenerationTwo.IsEmpty()
			&& receiptTwo.m_After && receiptTwo.m_After.m_Selected
			&& receiptThree.m_Before && receiptThree.m_Before.m_Selected
			&& receiptThree.m_Before.m_Selected.m_sSnapshotFingerprint
				== receiptTwo.m_sSnapshotFingerprint
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== recoveryGenerationTwo;
		report.m_bGenerationAdvanceExact = generationIdentityExact
			&& inactiveSlotPreserved && selectedGenerationThree;
		report.m_sGenerationEvidence = setupEvidence + " | "
			+ DescribeResolution(resolved)
			+ string.Format(
				" | identity/preserved/selected %1/%2/%3",
				generationIdentityExact,
				inactiveSlotPreserved,
				selectedGenerationThree);

		string canonicalBeforeRead = canonicalGenerationThree;
		string recoveryBeforeRead = recoveryGenerationTwo;
		HST_CampaignSaveData selectedSave;
		HST_CampaignProfileSaveResolution readResolution;
		string readEvidence;
		bool selectedRead = journal.ReadSelectedSnapshot(
			selectedSave,
			readResolution,
			readEvidence);
		report.m_bSelectedReadOnlyExact = selectedRead
			&& SnapshotIdentityExact(selectedSave, saveThree)
			&& readResolution && readResolution.m_Selected
			&& readResolution.m_Selected.m_iGeneration == 3
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBeforeRead
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== recoveryBeforeRead;
		report.m_sReadOnlyEvidence = readEvidence
			+ string.Format(
				" | selected/read bytes exact %1/%2",
				selectedRead,
				report.m_bSelectedReadOnlyExact);

		HST_CampaignState fallbackState = new HST_CampaignState();
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_PersistenceSourceResolution source
			= persistence.ResolveProfileFallbackAfterNativeFailureForProof(
				fallbackState,
				NATIVE_FAILURE,
				true);
		bool degradedMetadataExact = source
			&& source.m_eSource
				== HST_ECampaignPersistenceSource
					.HST_PERSISTENCE_SOURCE_PROFILE_FALLBACK
			&& source.m_bDegradedNativeRecovery
			&& source.m_sDegradedNativeRecoveryReason == NATIVE_FAILURE
			&& source.m_bProfileFallbackPresent
			&& source.m_bProfileFallbackRead
			&& source.m_iProfileJournalGeneration == 3
			&& source.m_sProfileJournalSlot
				== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			&& source.m_iProfileJournalValidSlotCount == 2
			&& !source.m_bProfileJournalLegacyRaw
			&& source.m_bProfileJournalChainExact
			&& source.m_sProfileFallbackSnapshotFingerprint
				== receiptThree.m_sSnapshotFingerprint
			&& source.m_sSelectedSnapshotFingerprint
				== receiptThree.m_sSnapshotFingerprint;
		bool degradedStateExact = source && source.m_State == fallbackState
			&& fallbackState.m_iCampaignSeed == saveThree.m_iCampaignSeed
			&& fallbackState.m_sPresetId == saveThree.m_sPresetId
			&& fallbackState.m_iWarLevel == saveThree.m_iWarLevel
			&& fallbackState.m_iFactionMoney == saveThree.m_iFactionMoney
			&& fallbackState.m_bRestoredFromPersistence
			&& fallbackState.m_iPersistenceRestoreSequence
				== saveThree.m_iPersistenceRestoreSequence + 1;
		bool degradedBytesExact
			= ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBeforeRead
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== recoveryBeforeRead;
		report.m_bDegradedNativeRecoveryExact = degradedMetadataExact
			&& degradedStateExact && degradedBytesExact
			&& persistence.IsProfileFallbackOnlyForSession();
		report.m_sDegradedRecoveryEvidence = string.Format(
			"metadata/state/bytes %1/%2/%3",
			degradedMetadataExact,
			degradedStateExact,
			degradedBytesExact);
		if (source)
			report.m_sDegradedRecoveryEvidence += " | " + source.m_sEvidence;

		bool truncatedWritten = WriteTruncatedFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		HST_CampaignProfileSaveResolution truncated = journal.ResolveJournal();
		bool truncatedSelectionExact = truncatedWritten
			&& truncated && truncated.m_bArtifactsPresent
			&& truncated.m_bHasSelection && !truncated.m_bAmbiguous
			&& !truncated.m_bUnsupportedFuture
			&& !truncated.m_bChainExact
			&& truncated.m_iValidCandidateCount == 1
			&& truncated.m_Selected
			&& truncated.m_Selected.m_sSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_RECOVERY
			&& truncated.m_Selected.m_iGeneration == 2
			&& truncated.m_Selected.m_sSnapshotFingerprint
				== receiptTwo.m_sSnapshotFingerprint
			&& SnapshotIdentityExact(truncated.m_Selected.m_SaveData, saveTwo);
		string truncatedSelectedBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_CampaignProfileSaveWriteReceipt truncatedRepairReceipt;
		string truncatedRepairEvidence;
		bool truncatedRepaired = truncatedSelectionExact
			&& journal.WriteVerifiedSnapshot(
				BuildSnapshot(1199, "truncated-replacement"),
				truncatedRepairReceipt,
				truncatedRepairEvidence);
		HST_CampaignProfileSaveResolution truncatedAfter
			= journal.ResolveJournal();
		bool truncatedRepairExact = truncatedRepaired
			&& truncatedRepairReceipt
			&& truncatedRepairReceipt.m_iGeneration == 3
			&& truncatedRepairReceipt.m_sWrittenSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== truncatedSelectedBytes
			&& truncatedAfter && truncatedAfter.m_bHasSelection
			&& truncatedAfter.m_bChainExact
			&& truncatedAfter.m_iValidCandidateCount == 2
			&& truncatedAfter.m_Selected
			&& truncatedAfter.m_Selected.m_iGeneration == 3;
		report.m_bTruncatedNewestFallbackExact
			= truncatedSelectionExact && truncatedRepairExact;

		CleanupJournalArtifacts();
		setupExact = EstablishThreeGenerations(
			journal,
			2100,
			saveOne,
			saveTwo,
			saveThree,
			receiptOne,
			receiptTwo,
			receiptThree,
			setupEvidence);
		bool forgedBadFingerprint = false;
		if (setupExact)
		{
			forgedBadFingerprint = WriteEnvelope(
				HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
				saveThree,
				HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
				receiptThree.m_iGeneration,
				HST_CampaignState.SCHEMA_VERSION,
				receiptThree.m_iPreviousGeneration,
				receiptThree.m_sPreviousSnapshotFingerprint,
				"journal_autotest_bad_fingerprint");
		}
		HST_CampaignProfileSaveResolution badFingerprint
			= journal.ResolveJournal();
		bool badFingerprintSelectionExact = forgedBadFingerprint
			&& badFingerprint && badFingerprint.m_bArtifactsPresent
			&& badFingerprint.m_bHasSelection
			&& !badFingerprint.m_bAmbiguous
			&& !badFingerprint.m_bUnsupportedFuture
			&& !badFingerprint.m_bChainExact
			&& badFingerprint.m_iValidCandidateCount == 1
			&& badFingerprint.m_Canonical
			&& !badFingerprint.m_Canonical.m_bValid
			&& badFingerprint.m_Canonical.m_sEvidence
				== "profile journal exact payload fingerprint rejected"
			&& badFingerprint.m_Selected
			&& badFingerprint.m_Selected.m_sSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_RECOVERY
			&& badFingerprint.m_Selected.m_iGeneration == 2
			&& badFingerprint.m_Selected.m_sSnapshotFingerprint
				== receiptTwo.m_sSnapshotFingerprint
			&& SnapshotIdentityExact(
				badFingerprint.m_Selected.m_SaveData,
				saveTwo);
		string badFingerprintSelectedBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_CampaignProfileSaveWriteReceipt badFingerprintRepairReceipt;
		string badFingerprintRepairEvidence;
		bool badFingerprintRepaired = badFingerprintSelectionExact
			&& journal.WriteVerifiedSnapshot(
				BuildSnapshot(2199, "bad-fingerprint-replacement"),
				badFingerprintRepairReceipt,
				badFingerprintRepairEvidence);
		HST_CampaignProfileSaveResolution badFingerprintAfter
			= journal.ResolveJournal();
		bool badFingerprintRepairExact = badFingerprintRepaired
			&& badFingerprintRepairReceipt
			&& badFingerprintRepairReceipt.m_iGeneration == 3
			&& badFingerprintRepairReceipt.m_sWrittenSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== badFingerprintSelectedBytes
			&& badFingerprintAfter && badFingerprintAfter.m_bHasSelection
			&& badFingerprintAfter.m_bChainExact
			&& badFingerprintAfter.m_iValidCandidateCount == 2
			&& badFingerprintAfter.m_Selected
			&& badFingerprintAfter.m_Selected.m_iGeneration == 3;
		report.m_bBadFingerprintFallbackExact
			= badFingerprintSelectionExact && badFingerprintRepairExact;
		report.m_sFaultFallbackEvidence = "truncated "
			+ DescribeResolution(truncated) + " | repair "
			+ truncatedRepairEvidence + " | after "
			+ DescribeResolution(truncatedAfter) + " | bad fingerprint "
			+ DescribeResolution(badFingerprint) + " | repair "
			+ badFingerprintRepairEvidence + " | after "
			+ DescribeResolution(badFingerprintAfter);
	}

	protected void ProveCanonicalGenerationOnePreservation(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData saveOne
			= BuildSnapshot(2401, "canonical-preservation-one");
		HST_CampaignSaveData saveTwo
			= BuildSnapshot(2402, "canonical-preservation-two");
		HST_CampaignProfileSaveWriteReceipt receiptOne;
		HST_CampaignProfileSaveWriteReceipt receiptTwo;
		string evidenceOne;
		string evidenceTwo;
		bool wroteOne = journal.WriteVerifiedSnapshot(
			saveOne,
			receiptOne,
			evidenceOne);
		string canonicalGenerationOne = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		bool wroteTwo = wroteOne && !canonicalGenerationOne.IsEmpty()
			&& journal.WriteVerifiedSnapshot(
				saveTwo,
				receiptTwo,
				evidenceTwo);
		HST_CampaignProfileSaveResolution after = journal.ResolveJournal();
		bool canonicalBytesExact = wroteTwo
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalGenerationOne;
		bool lineageExact = receiptOne && receiptTwo
			&& receiptOne.m_bSuccess && receiptOne.m_iGeneration == 1
			&& receiptOne.m_sWrittenSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			&& receiptTwo.m_bSuccess && receiptTwo.m_iGeneration == 2
			&& receiptTwo.m_sWrittenSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_RECOVERY
			&& receiptTwo.m_iPreviousGeneration == 1
			&& receiptTwo.m_sPreviousSnapshotFingerprint
				== receiptOne.m_sSnapshotFingerprint;
		bool selectionExact = after && after.m_bHasSelection
			&& !after.m_bAmbiguous && !after.m_bUnsupportedFuture
			&& after.m_bChainExact && after.m_iValidCandidateCount == 2
			&& after.m_Selected && after.m_Selected.m_iGeneration == 2
			&& after.m_Selected.m_sSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_RECOVERY
			&& SnapshotIdentityExact(after.m_Selected.m_SaveData, saveTwo);
		report.m_bCanonicalGenerationOnePreservedExact
			= canonicalBytesExact && lineageExact && selectionExact;
		report.m_sGenerationPreservationEvidence = string.Format(
			"canonical bytes/lineage/selection %1/%2/%3 | write one %4 | write two %5 | %6",
			canonicalBytesExact,
			lineageExact,
			selectionExact,
			evidenceOne,
			evidenceTwo,
			DescribeResolution(after));
	}

	protected void ProveBothInvalidRejection(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		bool canonicalWritten = WriteTruncatedFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		bool recoveryWritten = WriteTruncatedFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignProfileSaveResolution resolution = journal.ResolveJournal();
		report.m_bBothInvalidRejectedExact = canonicalWritten
			&& recoveryWritten && resolution
			&& resolution.m_bArtifactsPresent
			&& !resolution.m_bHasSelection
			&& !resolution.m_bUnsupportedFuture
			&& !resolution.m_bAmbiguous
			&& resolution.m_iValidCandidateCount == 0;
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CampaignState fallbackState = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveProfileFallbackAfterNativeFailureForProof(
				fallbackState,
				NATIVE_FAILURE,
				true);
		report.m_bBothInvalidSourceFatalExact = source
			&& source.m_eSource
				== HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL
			&& source.m_bProfileFallbackPresent
			&& !source.m_bProfileFallbackRead
			&& source.m_iProfileJournalValidSlotCount == 0
			&& !source.HasSelectedState();
		report.m_sInvalidHistoryEvidence = DescribeResolution(resolution);
		if (source)
			report.m_sInvalidHistoryEvidence += " | source " + source.m_sEvidence;
	}

	protected void ProveFutureFormatRejection(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData saveOne = BuildSnapshot(3101, "future-envelope-base");
		HST_CampaignProfileSaveWriteReceipt receiptOne;
		string writeEvidence;
		bool baseWritten = journal.WriteVerifiedSnapshot(
			saveOne,
			receiptOne,
			writeEvidence);
		string canonicalBefore = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		HST_CampaignSaveData saveTwo = BuildSnapshot(3102, "future-envelope-newer");
		bool futureEnvelopeWritten = baseWritten && WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			saveTwo,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION + 1,
			2,
			HST_CampaignState.SCHEMA_VERSION,
			1,
			receiptOne.m_sSnapshotFingerprint,
			"");
		HST_CampaignProfileSaveResolution futureEnvelope
			= journal.ResolveJournal();
		report.m_bFutureEnvelopeRejectedExact = futureEnvelopeWritten
			&& futureEnvelope && futureEnvelope.m_bArtifactsPresent
			&& futureEnvelope.m_bUnsupportedFuture
			&& !futureEnvelope.m_bHasSelection
			&& futureEnvelope.m_iValidCandidateCount == 1
			&& futureEnvelope.m_Canonical
			&& futureEnvelope.m_Canonical.m_bValid
			&& futureEnvelope.m_Recovery
			&& futureEnvelope.m_Recovery.m_bUnsupportedFuture
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBefore;
		string futureEnvelopeBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_CampaignProfileSaveWriteReceipt futureArtifactReceipt;
		string futureArtifactEvidence;
		bool futureArtifactWriteAccepted = journal.WriteVerifiedSnapshot(
			BuildSnapshot(3103, "future-artifact-write-rejected"),
			futureArtifactReceipt,
			futureArtifactEvidence);
		report.m_bFutureArtifactWriteNonMutatingExact
			= report.m_bFutureEnvelopeRejectedExact
			&& !futureArtifactWriteAccepted && futureArtifactReceipt
			&& !futureArtifactReceipt.m_bSuccess
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBefore
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== futureEnvelopeBytes;

		CleanupJournalArtifacts();
		saveOne = BuildSnapshot(3151, "unknown-magic-base");
		baseWritten = journal.WriteVerifiedSnapshot(
			saveOne,
			receiptOne,
			writeEvidence);
		canonicalBefore = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		saveTwo = BuildSnapshot(3152, "unknown-magic-newer");
		bool unknownMagicWritten = baseWritten && WriteEnvelopeWithMagic(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			saveTwo,
			"partisan_campaign_profile_journal_future_identity",
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			2,
			HST_CampaignState.SCHEMA_VERSION,
			1,
			receiptOne.m_sSnapshotFingerprint,
			"");
		string unknownMagicBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_CampaignProfileSaveResolution unknownMagic
			= journal.ResolveJournal();
		HST_CampaignProfileSaveWriteReceipt unknownMagicReceipt;
		string unknownMagicWriteEvidence;
		bool unknownMagicWriteAccepted = journal.WriteVerifiedSnapshot(
			BuildSnapshot(3153, "unknown-magic-write-rejected"),
			unknownMagicReceipt,
			unknownMagicWriteEvidence);
		report.m_bUnknownMagicRejectedExact = unknownMagicWritten
			&& unknownMagic && unknownMagic.m_bArtifactsPresent
			&& unknownMagic.m_bUnsupportedFuture
			&& !unknownMagic.m_bHasSelection
			&& unknownMagic.m_iValidCandidateCount == 1
			&& unknownMagic.m_Recovery
			&& unknownMagic.m_Recovery.m_bUnsupportedFuture
			&& !unknownMagicWriteAccepted && unknownMagicReceipt
			&& !unknownMagicReceipt.m_bSuccess
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBefore
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== unknownMagicBytes;

		CleanupJournalArtifacts();
		saveOne = BuildSnapshot(3201, "future-schema-base");
		baseWritten = journal.WriteVerifiedSnapshot(
			saveOne,
			receiptOne,
			writeEvidence);
		canonicalBefore = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		saveTwo = BuildSnapshot(3202, "future-schema-newer");
		bool futureSchemaWritten = baseWritten && WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			saveTwo,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			2,
			HST_CampaignState.SCHEMA_VERSION + 1,
			1,
			receiptOne.m_sSnapshotFingerprint,
			"");
		HST_CampaignProfileSaveResolution futureSchema
			= journal.ResolveJournal();
		bool futureHeaderSchemaExact = futureSchemaWritten
			&& futureSchema && futureSchema.m_bArtifactsPresent
			&& futureSchema.m_bUnsupportedFuture
			&& !futureSchema.m_bHasSelection
			&& futureSchema.m_iValidCandidateCount == 1
			&& futureSchema.m_Canonical
			&& futureSchema.m_Canonical.m_bValid
			&& futureSchema.m_Recovery
			&& futureSchema.m_Recovery.m_bUnsupportedFuture
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBefore;

		CleanupJournalArtifacts();
		saveOne = BuildSnapshot(3301, "future-payload-base");
		baseWritten = journal.WriteVerifiedSnapshot(
			saveOne,
			receiptOne,
			writeEvidence);
		canonicalBefore = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		saveTwo = BuildSnapshot(3302, "future-payload-newer");
		saveTwo.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION + 1;
		saveTwo.m_iLastLoadedSchemaVersion = saveTwo.m_iSchemaVersion;
		bool futurePayloadWritten = baseWritten && WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			saveTwo,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			2,
			HST_CampaignState.SCHEMA_VERSION,
			1,
			receiptOne.m_sSnapshotFingerprint,
			"");
		HST_CampaignProfileSaveResolution futurePayload
			= journal.ResolveJournal();
		bool futurePayloadSchemaExact = futurePayloadWritten
			&& futurePayload && futurePayload.m_bArtifactsPresent
			&& futurePayload.m_bUnsupportedFuture
			&& !futurePayload.m_bHasSelection
			&& futurePayload.m_iValidCandidateCount == 1
			&& futurePayload.m_Recovery
			&& futurePayload.m_Recovery.m_bUnsupportedFuture
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBefore;
		report.m_bFutureSchemaRejectedExact
			= futureHeaderSchemaExact && futurePayloadSchemaExact;
		report.m_sFutureFormatEvidence = "envelope "
			+ DescribeResolution(futureEnvelope) + " | writer "
			+ futureArtifactEvidence + " | unknown magic "
			+ DescribeResolution(unknownMagic) + " | unknown writer "
			+ unknownMagicWriteEvidence + " | header schema "
			+ DescribeResolution(futureSchema) + " | payload schema "
			+ DescribeResolution(futurePayload);
	}

	protected void ProveFutureRawRejection(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignSaveData futureRaw
			= BuildSnapshot(3401, "future-raw-canonical");
		futureRaw.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION + 1;
		futureRaw.m_iLastLoadedSchemaVersion = futureRaw.m_iSchemaVersion;
		bool rawWritten = WriteRawSnapshot(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			futureRaw);
		string rawBytes = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignProfileSaveResolution resolution = journal.ResolveJournal();
		bool resolutionExact = rawWritten && !rawBytes.IsEmpty()
			&& resolution && resolution.m_bArtifactsPresent
			&& resolution.m_bUnsupportedFuture
			&& !resolution.m_bHasSelection && !resolution.m_bAmbiguous
			&& resolution.m_iValidCandidateCount == 0
			&& resolution.m_Canonical
			&& resolution.m_Canonical.m_bUnsupportedFuture;

		HST_CampaignProfileSaveWriteReceipt receipt;
		string writeEvidence;
		bool writeAccepted = journal.WriteVerifiedSnapshot(
			BuildSnapshot(3402, "future-raw-write-rejected"),
			receipt,
			writeEvidence);
		bool writerExact = !writeAccepted && receipt
			&& !receipt.m_bSuccess
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE) == rawBytes
			&& !FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);

		CleanupJournalArtifacts();
		HST_CampaignSaveData currentRaw
			= BuildSnapshot(3403, "current-raw-canonical-rejected");
		bool currentRawWritten = WriteRawSnapshot(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			currentRaw);
		HST_CampaignProfileSaveResolution currentRawResolution
			= journal.ResolveJournal();
		bool currentRawRejected = currentRawWritten
			&& currentRawResolution
			&& currentRawResolution.m_bArtifactsPresent
			&& !currentRawResolution.m_bHasSelection
			&& !currentRawResolution.m_bUnsupportedFuture
			&& !currentRawResolution.m_bAmbiguous
			&& currentRawResolution.m_iValidCandidateCount == 0;
		report.m_bFutureRawRejectedExact = resolutionExact && writerExact
			&& currentRawRejected;
		report.m_sFutureRawEvidence = string.Format(
			"future resolution/writer/current raw rejected %1/%2/%3 | %4 | %5",
			resolutionExact,
			writerExact,
			currentRawRejected,
			DescribeResolution(resolution),
			writeEvidence + " | "
				+ DescribeResolution(currentRawResolution));
	}

	protected void ProveLegacyRawUpgrade(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignSaveData rawSave = BuildSnapshot(4100, "legacy-raw-schema-70");
		rawSave.m_iSchemaVersion = 70;
		rawSave.m_iLastLoadedSchemaVersion = 70;
		rawSave.m_iPersistenceCheckpointSequence = 0;
		bool rawWritten = WriteRawSnapshot(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			rawSave);
		string rawBytes = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		string rawFingerprint
			= HST_CampaignPersistentState.BuildPayloadFingerprint(rawBytes);
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignProfileSaveResolution legacy = journal.ResolveJournal();
		bool legacyGenerationZero = rawWritten && !rawBytes.IsEmpty()
			&& legacy && legacy.m_bHasSelection
			&& legacy.m_iValidCandidateCount == 1
			&& legacy.m_Selected && legacy.m_Selected.m_bLegacyRaw
			&& legacy.m_Selected.m_sSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL
			&& legacy.m_Selected.m_iGeneration == 0
			&& legacy.m_Selected.m_iSnapshotSchemaVersion == 70
			&& legacy.m_Selected.m_sSnapshotPayload == rawBytes
			&& legacy.m_Selected.m_sSnapshotFingerprint == rawFingerprint;

		HST_CampaignSaveData nextSave = BuildSnapshot(4101, "legacy-first-envelope");
		HST_CampaignProfileSaveWriteReceipt receipt;
		string writeEvidence;
		bool advanced = legacyGenerationZero
			&& journal.WriteVerifiedSnapshot(nextSave, receipt, writeEvidence);
		HST_CampaignProfileSaveResolution after = journal.ResolveJournal();
		bool rawPreserved = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE) == rawBytes;
		report.m_bLegacyRawUpgradeExact = advanced && receipt
			&& receipt.m_bSuccess
			&& receipt.m_sWrittenSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_RECOVERY
			&& receipt.m_iGeneration == 1
			&& receipt.m_iPreviousGeneration == 0
			&& receipt.m_sPreviousSnapshotFingerprint == rawFingerprint
			&& rawPreserved && after && after.m_bHasSelection
			&& after.m_bChainExact && after.m_iValidCandidateCount == 2
			&& after.m_Selected
			&& after.m_Selected.m_sSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_RECOVERY
			&& after.m_Selected.m_iGeneration == 1
			&& SnapshotIdentityExact(after.m_Selected.m_SaveData, nextSave);
		report.m_sLegacyEvidence = "before " + DescribeResolution(legacy)
			+ " | write " + writeEvidence + " | after "
			+ DescribeResolution(after)
			+ string.Format(" | raw bytes preserved %1", rawPreserved);
	}

	protected void ProveSplitBrainRejection(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignSaveData left = BuildSnapshot(5101, "split-left");
		HST_CampaignSaveData right = BuildSnapshot(5102, "split-right");
		bool leftWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			left,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			1,
			HST_CampaignState.SCHEMA_VERSION,
			0,
			"",
			"");
		bool rightWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			right,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			1,
			HST_CampaignState.SCHEMA_VERSION,
			0,
			"",
			"");
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignProfileSaveResolution resolution = journal.ResolveJournal();
		string splitIntegrationEvidence;
		bool splitIntegrationExact
			= AmbiguousHistoryFailsClosedAndPreservesBytes(
				journal,
				"split-brain",
				splitIntegrationEvidence);
		report.m_bSplitBrainRejectedExact = leftWritten && rightWritten
			&& resolution && resolution.m_bArtifactsPresent
			&& resolution.m_iValidCandidateCount == 2
			&& resolution.m_bAmbiguous
			&& !resolution.m_bHasSelection
			&& !resolution.m_bUnsupportedFuture
			&& resolution.m_Canonical && resolution.m_Recovery
			&& resolution.m_Canonical.m_sSnapshotFingerprint
				!= resolution.m_Recovery.m_sSnapshotFingerprint
			&& splitIntegrationExact;
		report.m_sSplitBrainEvidence = DescribeResolution(resolution)
			+ " | " + splitIntegrationEvidence;
	}

	protected void ProveLineageResolution(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignSaveData first = BuildSnapshot(5201, "broken-chain-one");
		HST_CampaignSaveData third = BuildSnapshot(5203, "broken-chain-three");
		bool firstWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			first,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			1,
			HST_CampaignState.SCHEMA_VERSION,
			0,
			"",
			"");
		bool thirdWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			third,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			3,
			HST_CampaignState.SCHEMA_VERSION,
			2,
			"missing-generation-two",
			"");
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignProfileSaveResolution broken = journal.ResolveJournal();
		string brokenIntegrationEvidence;
		bool brokenIntegrationExact
			= AmbiguousHistoryFailsClosedAndPreservesBytes(
				journal,
				"broken-lineage",
				brokenIntegrationEvidence);
		report.m_bBrokenChainRejectedExact = firstWritten && thirdWritten
			&& broken && broken.m_iValidCandidateCount == 2
			&& broken.m_bAmbiguous && !broken.m_bHasSelection
			&& !broken.m_bChainExact && brokenIntegrationExact;

		CleanupJournalArtifacts();
		HST_CampaignSaveData duplicate = BuildSnapshot(5302, "duplicate-lineage");
		bool duplicateCanonicalWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			duplicate,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			2,
			HST_CampaignState.SCHEMA_VERSION,
			1,
			"parent-a",
			"");
		bool duplicateRecoveryWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			duplicate,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			2,
			HST_CampaignState.SCHEMA_VERSION,
			1,
			"parent-a",
			"");
		HST_CampaignProfileSaveResolution exactDuplicate
			= journal.ResolveJournal();
		bool exactDuplicateSelected = duplicateCanonicalWritten
			&& duplicateRecoveryWritten && exactDuplicate
			&& exactDuplicate.m_bHasSelection
			&& !exactDuplicate.m_bAmbiguous
			&& !exactDuplicate.m_bChainExact
			&& exactDuplicate.m_Selected
			&& exactDuplicate.m_Selected.m_sSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL;

		bool conflictRecoveryWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			duplicate,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			2,
			HST_CampaignState.SCHEMA_VERSION,
			1,
			"parent-b",
			"");
		HST_CampaignProfileSaveResolution metadataConflict
			= journal.ResolveJournal();
		string metadataIntegrationEvidence;
		bool metadataIntegrationExact
			= AmbiguousHistoryFailsClosedAndPreservesBytes(
				journal,
				"duplicate-metadata",
				metadataIntegrationEvidence);

		CleanupJournalArtifacts();
		HST_CampaignSaveData duplicateGenerationOne
			= BuildSnapshot(5301, "duplicate-generation-one");
		bool duplicateGenerationOneCanonicalWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			duplicateGenerationOne,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			1,
			HST_CampaignState.SCHEMA_VERSION,
			0,
			"",
			"");
		bool duplicateGenerationOneRecoveryWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			duplicateGenerationOne,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			1,
			HST_CampaignState.SCHEMA_VERSION,
			0,
			"",
			"");
		HST_CampaignProfileSaveResolution generationOneDuplicate
			= journal.ResolveJournal();
		bool generationOneDuplicateExact
			= duplicateGenerationOneCanonicalWritten
				&& duplicateGenerationOneRecoveryWritten
				&& generationOneDuplicate
				&& generationOneDuplicate.m_bHasSelection
				&& !generationOneDuplicate.m_bAmbiguous
				&& !generationOneDuplicate.m_bChainExact
				&& generationOneDuplicate.m_Selected
				&& generationOneDuplicate.m_Selected.m_sSlotLabel
					== HST_CampaignProfileSaveJournalService.SLOT_CANONICAL;
		report.m_bDuplicateMetadataRejectedExact = exactDuplicateSelected
			&& conflictRecoveryWritten && metadataConflict
			&& metadataConflict.m_iValidCandidateCount == 2
			&& metadataConflict.m_bAmbiguous
			&& !metadataConflict.m_bHasSelection
			&& metadataIntegrationExact && generationOneDuplicateExact;
		report.m_sLineageEvidence = "broken "
			+ DescribeResolution(broken) + " | "
			+ brokenIntegrationEvidence + " | duplicate "
			+ DescribeResolution(exactDuplicate) + " | conflict "
			+ DescribeResolution(metadataConflict) + " | "
			+ metadataIntegrationEvidence + " | generation-one duplicate "
			+ DescribeResolution(generationOneDuplicate);
	}

	protected void ProveIsolatedLineagePredicates(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData generationOne
			= BuildSnapshot(5351, "generation-one-parent-metadata");
		bool generationOneWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			generationOne,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			1,
			HST_CampaignState.SCHEMA_VERSION,
			1,
			"unexpected-generation-zero-parent",
			"");
		HST_CampaignProfileSaveResolution generationOneParent
			= journal.ResolveJournal();
		report.m_bGenerationOneParentGenerationRejectedExact = generationOneWritten
			&& generationOneParent
			&& generationOneParent.m_bArtifactsPresent
			&& generationOneParent.m_iValidCandidateCount == 0
			&& !generationOneParent.m_bHasSelection
			&& !generationOneParent.m_bUnsupportedFuture
			&& !generationOneParent.m_bAmbiguous
			&& generationOneParent.m_Canonical
			&& generationOneParent.m_Canonical.m_sEvidence
				== "profile journal generation 1 has a nonzero parent generation";

		CleanupJournalArtifacts();
		HST_CampaignSaveData adjacentFirst
			= BuildSnapshot(5401, "adjacent-wrong-parent-one");
		HST_CampaignSaveData adjacentSecond
			= BuildSnapshot(5402, "adjacent-wrong-parent-two");
		bool adjacentFirstWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			adjacentFirst,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			1,
			HST_CampaignState.SCHEMA_VERSION,
			0,
			"",
			"");
		bool adjacentSecondWritten = WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			adjacentSecond,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
			2,
			HST_CampaignState.SCHEMA_VERSION,
			1,
			"wrong-parent-fingerprint",
			"");
		HST_CampaignProfileSaveResolution adjacent = journal.ResolveJournal();
		report.m_bAdjacentWrongParentRejectedExact = adjacentFirstWritten
			&& adjacentSecondWritten && adjacent
			&& adjacent.m_iValidCandidateCount == 2
			&& adjacent.m_bAmbiguous && !adjacent.m_bHasSelection
			&& !adjacent.m_bUnsupportedFuture && !adjacent.m_bChainExact
			&& adjacent.m_Canonical && adjacent.m_Recovery
			&& adjacent.m_Recovery.m_iGeneration
				== adjacent.m_Canonical.m_iGeneration + 1
			&& adjacent.m_Recovery.m_iPreviousGeneration
				== adjacent.m_Canonical.m_iGeneration
			&& adjacent.m_Recovery.m_sPreviousSnapshotFingerprint
				!= adjacent.m_Canonical.m_sSnapshotFingerprint;

		CleanupJournalArtifacts();
		HST_CampaignSaveData nonAdjacentFirst
			= BuildSnapshot(5501, "non-adjacent-exact-parent-one");
		HST_CampaignSaveData nonAdjacentThird
			= BuildSnapshot(5503, "non-adjacent-exact-parent-three");
		string firstFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(nonAdjacentFirst);
		bool nonAdjacentFirstWritten = !firstFingerprint.IsEmpty()
			&& WriteEnvelope(
				HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
				nonAdjacentFirst,
				HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
				1,
				HST_CampaignState.SCHEMA_VERSION,
				0,
				"",
				"");
		bool nonAdjacentThirdWritten = nonAdjacentFirstWritten
			&& WriteEnvelope(
				HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
				nonAdjacentThird,
				HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION,
				3,
				HST_CampaignState.SCHEMA_VERSION,
				2,
				firstFingerprint,
				"");
		HST_CampaignProfileSaveResolution nonAdjacent = journal.ResolveJournal();
		report.m_bNonAdjacentParentFingerprintRejectedExact
			= nonAdjacentFirstWritten && nonAdjacentThirdWritten
			&& nonAdjacent && nonAdjacent.m_iValidCandidateCount == 2
			&& nonAdjacent.m_bAmbiguous && !nonAdjacent.m_bHasSelection
			&& !nonAdjacent.m_bUnsupportedFuture && !nonAdjacent.m_bChainExact
			&& nonAdjacent.m_Canonical && nonAdjacent.m_Recovery
			&& nonAdjacent.m_Recovery.m_iGeneration
				!= nonAdjacent.m_Canonical.m_iGeneration + 1
			&& nonAdjacent.m_Recovery.m_iPreviousGeneration
				== nonAdjacent.m_Recovery.m_iGeneration - 1
			&& nonAdjacent.m_Recovery.m_iPreviousGeneration
				!= nonAdjacent.m_Canonical.m_iGeneration
			&& nonAdjacent.m_Recovery.m_sPreviousSnapshotFingerprint
				== nonAdjacent.m_Canonical.m_sSnapshotFingerprint;
		report.m_sLineagePredicateEvidence = "generation-one parent "
			+ DescribeResolution(generationOneParent)
			+ " | adjacent wrong parent "
			+ DescribeResolution(adjacent)
			+ " | non-adjacent exact parent fingerprint "
			+ DescribeResolution(nonAdjacent);
	}

	protected void ProveFutureWriteIsNonMutating(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData saveOne = BuildSnapshot(6101, "future-write-one");
		HST_CampaignSaveData saveTwo = BuildSnapshot(6102, "future-write-two");
		HST_CampaignProfileSaveWriteReceipt receiptOne;
		HST_CampaignProfileSaveWriteReceipt receiptTwo;
		string evidenceOne;
		string evidenceTwo;
		bool setupExact = journal.WriteVerifiedSnapshot(
			saveOne,
			receiptOne,
			evidenceOne)
			&& journal.WriteVerifiedSnapshot(
				saveTwo,
				receiptTwo,
				evidenceTwo);
		string canonicalBefore = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		string recoveryBefore = ReadFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_CampaignSaveData future = BuildSnapshot(6103, "future-write-rejected");
		future.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION + 1;
		future.m_iLastLoadedSchemaVersion = future.m_iSchemaVersion;
		HST_CampaignProfileSaveWriteReceipt rejectedReceipt;
		string rejectedEvidence;
		bool writeAccepted = journal.WriteVerifiedSnapshot(
			future,
			rejectedReceipt,
			rejectedEvidence);
		HST_CampaignProfileSaveResolution after = journal.ResolveJournal();
		report.m_bFutureWriteNonMutatingExact = setupExact
			&& !writeAccepted && rejectedReceipt
			&& !rejectedReceipt.m_bSuccess
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBefore
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== recoveryBefore
			&& after && after.m_bHasSelection
			&& after.m_iValidCandidateCount == 2
			&& after.m_Selected && after.m_Selected.m_iGeneration == 2
			&& after.m_Selected.m_sSnapshotFingerprint
				== receiptTwo.m_sSnapshotFingerprint;
		report.m_sFutureWriteEvidence = rejectedEvidence
			+ " | after " + DescribeResolution(after);
	}

	protected void ProveFallbackOnlyCheckpoint(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData baseSave = BuildSnapshot(6201, "fallback-only-base");
		HST_CampaignProfileSaveWriteReceipt baseReceipt;
		string baseEvidence;
		bool baseWritten = journal.WriteVerifiedSnapshot(
			baseSave,
			baseReceipt,
			baseEvidence);
		string canonicalBefore = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);

		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CivilianService civilians = new HST_CivilianService();
		HST_CivilianConsequenceService consequences
			= new HST_CivilianConsequenceService();
		HST_PersistentFieldVehicleRuntimeService fieldVehicles
			= new HST_PersistentFieldVehicleRuntimeService();
		civilians.SetCivilianConsequenceService(consequences);
		civilians.SetPersistentFieldVehicleRuntimeService(fieldVehicles);
		persistence.SetCivilianService(civilians);
		HST_CampaignState fallbackState = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveProfileFallbackAfterNativeFailureForProof(
				fallbackState,
				NATIVE_FAILURE,
				true);
		bool sourceExact = baseWritten && source && source.HasSelectedState()
			&& source.m_eSource
				== HST_ECampaignPersistenceSource
					.HST_PERSISTENCE_SOURCE_PROFILE_FALLBACK
			&& persistence.IsProfileFallbackOnlyForSession();
		if (sourceExact)
		{
			fallbackState.m_iElapsedSeconds += 10;
			fallbackState.m_iFactionMoney += 17;
		}
		HST_PersistenceCheckpointRequest checkpoint;
		if (sourceExact)
			checkpoint = persistence.RequestManualCheckpointDetailed(fallbackState);
		HST_CampaignProfileSaveResolution after
			= persistence.GetLastProfileJournalResolution();
		bool checkpointExact = checkpoint && checkpoint.WasAccepted()
			&& checkpoint.m_bCampaignCaptured
			&& !checkpoint.m_bTransientStateStaged
			&& checkpoint.m_bProfileFallbackSaved
			&& !checkpoint.m_bSavePointRequested;
		bool journalExact = after && after.m_bHasSelection
			&& after.m_bChainExact && after.m_iValidCandidateCount == 2
			&& after.m_Selected && after.m_Selected.m_iGeneration == 2
			&& after.m_Selected.m_sSlotLabel
				== HST_CampaignProfileSaveJournalService.SLOT_RECOVERY
			&& after.m_Selected.m_SaveData
			&& after.m_Selected.m_SaveData.m_iFactionMoney
				== baseSave.m_iFactionMoney + 17
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBefore;
		report.m_bFallbackOnlyCheckpointExact = sourceExact
			&& checkpointExact && journalExact;
		report.m_sFallbackOnlyEvidence = string.Format(
			"source/checkpoint/journal %1/%2/%3 | %4",
			sourceExact,
			checkpointExact,
			journalExact,
			DescribeResolution(after));
		if (checkpoint)
			report.m_sFallbackOnlyEvidence += " | " + checkpoint.m_sEvidence;
		report.m_sFallbackOnlyEvidence += " | state "
			+ fallbackState.m_sLastPersistenceStatus;
	}

	protected void ProveFailedNativeCheckpointCallback(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignProfileSaveWriteReceipt receiptOne;
		HST_CampaignProfileSaveWriteReceipt receiptTwo;
		string evidenceOne;
		string evidenceTwo;
		bool setupExact = journal.WriteVerifiedSnapshot(
			BuildSnapshot(6751, "failed-callback-journal-one"),
			receiptOne,
			evidenceOne)
			&& journal.WriteVerifiedSnapshot(
				BuildSnapshot(6752, "failed-callback-journal-two"),
				receiptTwo,
				evidenceTwo);
		string canonicalBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		string recoveryBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_CampaignSaveData pendingSave
			= BuildSnapshot(6753, "failed-native-callback-pending");
		HST_CampaignState state = pendingSave.Restore();
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_PersistenceCheckpointRequest request;
		bool seamExact = persistence.SimulateFailedNativeCheckpointCallbackForProof(
			state,
			pendingSave,
			request);
		bool requestExact = request
			&& request.m_bCompletionReceived
			&& !request.m_bNativeCommitSucceeded
			&& !request.m_bProfileFallbackSaved
			&& request.m_bCampaignCaptured
			&& request.m_bTransientStateStaged
			&& request.m_bSavePointRequested
			&& !persistence.IsCheckpointSavePointInFlight();
		bool bytesExact = setupExact && !canonicalBytes.IsEmpty()
			&& !recoveryBytes.IsEmpty()
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBytes
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== recoveryBytes;
		HST_CampaignProfileSaveResolution after = journal.ResolveJournal();
		bool journalExact = after && after.m_bHasSelection
			&& after.m_bChainExact && after.m_iValidCandidateCount == 2
			&& after.m_Selected && after.m_Selected.m_iGeneration == 2
			&& after.m_Selected.m_sSnapshotFingerprint
				== receiptTwo.m_sSnapshotFingerprint;
		report.m_bFailedNativeCallbackNonMutatingExact = seamExact
			&& requestExact && bytesExact && journalExact;
		report.m_sFailedNativeCallbackEvidence = string.Format(
			"setup/seam/request/bytes/journal %1/%2/%3/%4/%5 | %6",
			setupExact,
			seamExact,
			requestExact,
			bytesExact,
			journalExact,
			DescribeResolution(after));
	}

	protected void ProveValidNativeInvalidJournalTolerance(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		bool canonicalWritten = WriteTruncatedFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		bool recoveryWritten = WriteTruncatedFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		string canonicalBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		string recoveryBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_CampaignSaveData nativeSave
			= BuildSnapshot(6801, "valid-native-invalid-journal");
		string nativeFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(nativeSave);
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CampaignState target = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveValidNativeAuthorityForProof(
				target,
				nativeSave,
				nativeFingerprint);
		HST_CampaignProfileSaveResolution resolution
			= persistence.GetLastProfileJournalResolution();
		bool resolutionExact = canonicalWritten && recoveryWritten
			&& resolution && resolution.m_bArtifactsPresent
			&& !resolution.m_bHasSelection
			&& !resolution.m_bUnsupportedFuture
			&& !resolution.m_bAmbiguous
			&& resolution.m_iValidCandidateCount == 0;
		bool metadataExact = SourceJournalMetadataExact(
			source,
			true,
			false,
			-1,
			"",
			0,
			false,
			false);
		bool bytesExact = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBytes
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== recoveryBytes;
		report.m_bValidNativeInvalidJournalExact = resolutionExact
			&& metadataExact && bytesExact
			&& NativeSelectionExact(source, target, nativeSave, nativeFingerprint)
			&& !persistence.IsProfileFallbackOnlyForSession();
		report.m_sNativeArtifactToleranceEvidence = "invalid journal "
			+ DescribeResolution(resolution) + " | source "
			+ AuthorityEvidence(source)
			+ string.Format(
				" | metadata/bytes %1/%2",
				metadataExact,
				bytesExact);
	}

	protected void ProveValidNativeFutureJournalTolerance(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData baseSave
			= BuildSnapshot(6901, "valid-native-future-base");
		HST_CampaignProfileSaveWriteReceipt baseReceipt;
		string writeEvidence;
		bool baseWritten = journal.WriteVerifiedSnapshot(
			baseSave,
			baseReceipt,
			writeEvidence);
		HST_CampaignSaveData futureSave
			= BuildSnapshot(6902, "valid-native-future-envelope");
		bool futureWritten = baseWritten && WriteEnvelope(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			futureSave,
			HST_CampaignProfileSaveJournalService.ENVELOPE_VERSION + 1,
			2,
			HST_CampaignState.SCHEMA_VERSION,
			1,
			baseReceipt.m_sSnapshotFingerprint,
			"");
		string canonicalBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		string recoveryBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_CampaignSaveData nativeSave
			= BuildSnapshot(6903, "valid-native-future-journal");
		string nativeFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(nativeSave);
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CampaignState target = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveValidNativeAuthorityForProof(
				target,
				nativeSave,
				nativeFingerprint);
		HST_CampaignProfileSaveResolution resolution
			= persistence.GetLastProfileJournalResolution();
		bool resolutionExact = futureWritten && resolution
			&& resolution.m_bArtifactsPresent
			&& resolution.m_bUnsupportedFuture
			&& !resolution.m_bHasSelection && !resolution.m_bAmbiguous
			&& resolution.m_iValidCandidateCount == 1
			&& resolution.m_Canonical && resolution.m_Canonical.m_bValid
			&& resolution.m_Recovery
			&& resolution.m_Recovery.m_bUnsupportedFuture;
		bool metadataExact = SourceJournalMetadataExact(
			source,
			true,
			false,
			-1,
			"",
			1,
			false,
			false);
		bool bytesExact = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBytes
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== recoveryBytes;
		report.m_bValidNativeFutureJournalExact = resolutionExact
			&& metadataExact && bytesExact
			&& NativeSelectionExact(source, target, nativeSave, nativeFingerprint)
			&& !persistence.IsProfileFallbackOnlyForSession();
		report.m_sNativeArtifactToleranceEvidence += " | future journal "
			+ DescribeResolution(resolution) + " | source "
			+ AuthorityEvidence(source)
			+ string.Format(
				" | metadata/bytes %1/%2",
				metadataExact,
				bytesExact);
	}

	protected void ProveFutureNativeAuthorityRejection(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignProfileSaveWriteReceipt receiptOne;
		HST_CampaignProfileSaveWriteReceipt receiptTwo;
		string evidenceOne;
		string evidenceTwo;
		bool setupExact = journal.WriteVerifiedSnapshot(
			BuildSnapshot(6951, "future-native-journal-one"),
			receiptOne,
			evidenceOne)
			&& journal.WriteVerifiedSnapshot(
				BuildSnapshot(6952, "future-native-journal-two"),
				receiptTwo,
				evidenceTwo);
		string canonicalBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		string recoveryBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CampaignState target = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveInspectedFutureNativeAuthorityForProof(
				target,
				"journal-autotest-future-native");
		bool sourceExact = source
			&& source.m_eSource
				== HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL
			&& source.m_bPersistenceSystemAvailable
			&& source.m_bPersistenceSystemLoadedData
			&& source.m_bNativeRecordPresent
			&& !source.m_bNativeRecordValid
			&& source.m_bNativeRecordUnsupportedFuture
			&& !source.m_bProfileFallbackPresent
			&& !source.m_bProfileFallbackRead
			&& !source.m_bDegradedNativeRecovery
			&& !source.HasSelectedState()
			&& source.m_sSelectedSnapshotFingerprint.IsEmpty()
			&& source.m_sEvidence.Contains(
				"unsupported future native campaign authority");
		bool bytesExact = setupExact && !canonicalBytes.IsEmpty()
			&& !recoveryBytes.IsEmpty()
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBytes
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== recoveryBytes;
		report.m_bFutureNativeAuthorityRejectedExact = sourceExact
			&& bytesExact && !persistence.IsProfileFallbackOnlyForSession()
			&& !persistence.GetLastProfileJournalResolution();
		report.m_sFutureNativeAuthorityEvidence += string.Format(
			" | future routing setup/source/bytes %1/%2/%3 | %4",
			setupExact,
			sourceExact,
			bytesExact,
			AuthorityEvidence(source));
	}

	protected void ProveNativeFingerprintVersioning(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		HST_CampaignSaveData snapshot
			= BuildSnapshot(6941, "legacy-native-fingerprint");
		snapshot.m_iSchemaVersion = 70;
		snapshot.m_iLastLoadedSchemaVersion = 70;
		snapshot.m_iPersistenceCheckpointSequence = 0;
		string legacyPayload;
		bool legacyPayloadExact = HST_CampaignPersistentState
			.TrySerializeLegacySnapshot(snapshot, legacyPayload)
			&& !legacyPayload.IsEmpty()
			&& legacyPayload.IndexOf(
				"\"m_iPersistenceCheckpointSequence\":") < 0;
		string storedLegacyFingerprint;
		if (legacyPayloadExact)
		{
			storedLegacyFingerprint = string.Format(
				"%1:%2",
				legacyPayload.Length(),
				legacyPayload.Hash());
		}
		string legacyFingerprint
			= HST_CampaignPersistentState.BuildLegacySnapshotFingerprint(snapshot);
		string currentFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(snapshot);
		JsonSaveContext currentContext = new JsonSaveContext();
		string currentPayload;
		string currentLayoutLegacyFingerprint;
		if (currentContext.WriteValue("", snapshot))
			currentPayload = currentContext.SaveToString();
		if (!currentPayload.IsEmpty())
		{
			currentLayoutLegacyFingerprint = string.Format(
				"%1:%2",
				currentPayload.Length(),
				currentPayload.Hash());
		}
		string legacyNormalized;
		string rejectedNormalized;
		bool legacyAccepted = legacyPayloadExact
			&& legacyFingerprint == storedLegacyFingerprint
			&& legacyFingerprint != currentLayoutLegacyFingerprint
			&& HST_CampaignPersistentState
				.TryValidateAndNormalizeSnapshotFingerprint(
					snapshot,
					HST_CampaignPersistentState.LEGACY_ENVELOPE_VERSION,
					storedLegacyFingerprint,
					legacyNormalized);
		bool crossVersionRejected = !HST_CampaignPersistentState
			.TryValidateAndNormalizeSnapshotFingerprint(
				snapshot,
				HST_CampaignPersistentState.ENVELOPE_VERSION,
				storedLegacyFingerprint,
				rejectedNormalized);
		snapshot.m_iPersistenceCheckpointSequence = 1;
		string nonzeroLegacyRejected
			= HST_CampaignPersistentState.BuildLegacySnapshotFingerprint(snapshot);
		snapshot.m_iPersistenceCheckpointSequence = 0;
		snapshot.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		string schema71LegacyRejected
			= HST_CampaignPersistentState.BuildLegacySnapshotFingerprint(snapshot);
		snapshot.m_iSchemaVersion = 70;
		snapshot.m_iLastLoadedSchemaVersion = 70;

		HST_CampaignNativeEnvelopeClassificationProofData legacyEnvelope
			= new HST_CampaignNativeEnvelopeClassificationProofData();
		legacyEnvelope.magic
			= HST_CampaignPersistentState.LEGACY_ENVELOPE_MAGIC;
		legacyEnvelope.version
			= HST_CampaignPersistentState.LEGACY_ENVELOPE_VERSION;
		legacyEnvelope.snapshotPresent = true;
		legacyEnvelope.snapshotFingerprint = storedLegacyFingerprint;
		legacyEnvelope.snapshot = snapshot;
		HST_CampaignPersistentStateLoadResult legacyLoad
			= ClassifyNativeEnvelopeForProof(legacyEnvelope);

		HST_CampaignSaveData currentSnapshot
			= BuildSnapshot(6942, "current-native-exact-payload");
		string currentSnapshotCanonicalPayload;
		string currentSnapshotCanonicalFingerprint;
		bool currentSnapshotSerialized = HST_CampaignPersistentState
			.TrySerializeSnapshot(
				currentSnapshot,
				currentSnapshotCanonicalPayload,
				currentSnapshotCanonicalFingerprint);
		// Leading legal JSON whitespace gives the exact stored payload a different
		// identity from current-layout reserialization. Acceptance therefore proves
		// the v2 path validates stored bytes before parsing instead of rebuilding
		// its fingerprint from the DTO.
		string currentSnapshotPayload = " " + currentSnapshotCanonicalPayload;
		string currentSnapshotFingerprint
			= HST_CampaignPersistentState.BuildPayloadFingerprint(
				currentSnapshotPayload);
		bool exactPayloadIdentityDistinct = !currentSnapshotFingerprint.IsEmpty()
			&& currentSnapshotFingerprint
				!= currentSnapshotCanonicalFingerprint;
		HST_CampaignNativeEnvelopeClassificationProofData currentEnvelope
			= new HST_CampaignNativeEnvelopeClassificationProofData();
		currentEnvelope.magic = HST_CampaignPersistentState.ENVELOPE_MAGIC;
		currentEnvelope.version = HST_CampaignPersistentState.ENVELOPE_VERSION;
		currentEnvelope.snapshotPresent = true;
		currentEnvelope.snapshotFingerprint = currentSnapshotFingerprint;
		currentEnvelope.snapshotPayload = currentSnapshotPayload;
		HST_CampaignPersistentStateLoadResult currentLoad
			= ClassifyNativeEnvelopeForProof(currentEnvelope);

		HST_CampaignNativeEnvelopeClassificationProofData invalidEnvelope
			= new HST_CampaignNativeEnvelopeClassificationProofData();
		invalidEnvelope.magic = HST_CampaignPersistentState.ENVELOPE_MAGIC;
		invalidEnvelope.version = HST_CampaignPersistentState.ENVELOPE_VERSION;
		invalidEnvelope.snapshotPresent = true;
		invalidEnvelope.snapshotFingerprint
			= "uuidv8-sha256-v1:1:00000000-0000-8000-8000-000000000000";
		invalidEnvelope.snapshotPayload = currentSnapshotPayload;
		HST_CampaignPersistentStateLoadResult invalidLoad
			= ClassifyNativeEnvelopeForProof(invalidEnvelope);

		HST_CampaignNativeEnvelopeClassificationProofData futureEnvelope
			= new HST_CampaignNativeEnvelopeClassificationProofData();
		futureEnvelope.magic = HST_CampaignPersistentState.ENVELOPE_MAGIC;
		futureEnvelope.version
			= HST_CampaignPersistentState.ENVELOPE_VERSION + 1;
		futureEnvelope.snapshotPresent = true;
		futureEnvelope.snapshotFingerprint = currentSnapshotFingerprint;
		futureEnvelope.snapshotPayload = currentSnapshotPayload;
		HST_CampaignPersistentStateLoadResult futureLoad
			= ClassifyNativeEnvelopeForProof(futureEnvelope);

		report.m_bNativeV1LoadClassificationExact = legacyLoad
			&& legacyLoad.m_bRecordPresent && legacyLoad.m_bRecordValid
			&& !legacyLoad.m_bRecordUnsupportedFuture
			&& legacyLoad.m_Snapshot
			&& legacyLoad.m_Snapshot.m_iSchemaVersion == 70
			&& legacyLoad.m_sSnapshotFingerprint == currentFingerprint
			&& legacyLoad.m_sValidationFailure.IsEmpty();
		report.m_bNativeV2LoadClassificationExact
			= currentSnapshotSerialized && exactPayloadIdentityDistinct
			&& currentLoad
			&& currentLoad.m_bRecordPresent && currentLoad.m_bRecordValid
			&& !currentLoad.m_bRecordUnsupportedFuture
			&& currentLoad.m_Snapshot
			&& currentLoad.m_Snapshot.m_iSchemaVersion
				== HST_CampaignState.SCHEMA_VERSION
			&& currentLoad.m_sSnapshotFingerprint
				== currentSnapshotFingerprint
			&& currentLoad.m_sValidationFailure.IsEmpty();
		report.m_bNativeInvalidFingerprintClassificationExact
			= invalidLoad && invalidLoad.m_bRecordPresent
			&& !invalidLoad.m_bRecordValid
			&& !invalidLoad.m_bRecordUnsupportedFuture
			&& !invalidLoad.m_Snapshot
			&& invalidLoad.m_sSnapshotFingerprint.IsEmpty()
			&& invalidLoad.m_sValidationFailure
				== "native campaign exact payload fingerprint rejected";
		report.m_bNativeFutureEnvelopeClassificationExact
			= futureLoad && futureLoad.m_bRecordPresent
			&& !futureLoad.m_bRecordValid
			&& futureLoad.m_bRecordUnsupportedFuture
			&& !futureLoad.m_Snapshot
			&& futureLoad.m_sSnapshotFingerprint.IsEmpty()
			&& futureLoad.m_sValidationFailure.Contains(
				"native campaign envelope is newer or unknown");
		report.m_bLegacyNativeFingerprintAcceptedExact = legacyAccepted
			&& crossVersionRejected
			&& legacyFingerprint != currentFingerprint
			&& legacyNormalized == currentFingerprint
			&& rejectedNormalized.IsEmpty()
			&& nonzeroLegacyRejected.IsEmpty()
			&& schema71LegacyRejected.IsEmpty();
		report.m_sFutureNativeAuthorityEvidence = string.Format(
			"native fingerprint pre71/cross/normalized/guards %1/%2/%3/%4 | load v1/v2/bad/future %5/%6/%7/%8",
			legacyAccepted,
			crossVersionRejected,
			legacyNormalized == currentFingerprint,
			nonzeroLegacyRejected.IsEmpty()
				&& schema71LegacyRejected.IsEmpty(),
			report.m_bNativeV1LoadClassificationExact,
			report.m_bNativeV2LoadClassificationExact,
			report.m_bNativeInvalidFingerprintClassificationExact,
			report.m_bNativeFutureEnvelopeClassificationExact);
	}

	protected void ProveAuthorityOrdering(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData newerProfile
			= BuildSnapshot(7101, "newer-profile-authority");
		newerProfile.m_iPersistenceCheckpointSequence = 10;
		newerProfile.m_iPersistenceRestoreSequence = 4;
		newerProfile.m_iLastSaveSecond = 400;
		HST_CampaignProfileSaveWriteReceipt profileReceipt;
		string writeEvidence;
		bool profileWritten = journal.WriteVerifiedSnapshot(
			newerProfile,
			profileReceipt,
			writeEvidence);
		string profileBytes = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		HST_CampaignSaveData olderNative
			= BuildSnapshot(7102, "older-native-authority");
		olderNative.m_iPersistenceCheckpointSequence = 10;
		olderNative.m_iPersistenceRestoreSequence = 3;
		olderNative.m_iLastSaveSecond = 900;
		string olderNativeFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(olderNative);
		HST_PersistenceService profileAuthority
			= new HST_PersistenceService();
		HST_CampaignState profileTarget = new HST_CampaignState();
		HST_PersistenceSourceResolution profileSource
			= profileAuthority.ResolveValidNativeAuthorityForProof(
				profileTarget,
				olderNative,
				olderNativeFingerprint);
		bool profileMetadataExact = profileSource
			&& profileSource.m_eSource
				== HST_ECampaignPersistenceSource
					.HST_PERSISTENCE_SOURCE_PROFILE_FALLBACK
			&& profileSource.m_bNativeRecordValid
			&& profileSource.m_bProfileFallbackRead
			&& profileSource.m_bDegradedNativeRecovery
			&& profileSource.m_sSelectedSnapshotFingerprint
				== profileReceipt.m_sSnapshotFingerprint
			&& !profileAuthority.IsProfileFallbackOnlyForSession();
		bool profileStateExact = profileSource
			&& profileSource.m_State == profileTarget
			&& profileTarget.m_sPresetId == newerProfile.m_sPresetId
			&& profileTarget.m_iFactionMoney == newerProfile.m_iFactionMoney
			&& profileTarget.m_iPersistenceRestoreSequence
				== newerProfile.m_iPersistenceRestoreSequence + 1;
		report.m_bNewerJournalAuthorityExact = profileWritten
			&& profileMetadataExact && profileStateExact
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== profileBytes;

		CleanupJournalArtifacts();
		HST_CampaignSaveData olderProfile
			= BuildSnapshot(7201, "older-profile-authority");
		olderProfile.m_iPersistenceCheckpointSequence = 20;
		olderProfile.m_iPersistenceRestoreSequence = 2;
		olderProfile.m_iLastSaveSecond = 900;
		profileWritten = journal.WriteVerifiedSnapshot(
			olderProfile,
			profileReceipt,
			writeEvidence);
		profileBytes = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		HST_CampaignSaveData newerNative
			= BuildSnapshot(7202, "newer-native-authority");
		newerNative.m_iPersistenceCheckpointSequence = 20;
		newerNative.m_iPersistenceRestoreSequence = 3;
		newerNative.m_iLastSaveSecond = 1;
		string newerNativeFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(newerNative);
		HST_PersistenceService nativeAuthority = new HST_PersistenceService();
		HST_CampaignState nativeTarget = new HST_CampaignState();
		HST_PersistenceSourceResolution nativeSource
			= nativeAuthority.ResolveValidNativeAuthorityForProof(
				nativeTarget,
				newerNative,
				newerNativeFingerprint);
		bool nativeMetadataExact = nativeSource
			&& nativeSource.m_eSource
				== HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_NATIVE
			&& nativeSource.m_bProfileFallbackRead
			&& !nativeSource.m_bDegradedNativeRecovery
			&& nativeSource.m_sSelectedSnapshotFingerprint
				== newerNativeFingerprint;
		bool nativeStateExact = nativeSource && nativeSource.m_State == nativeTarget
			&& nativeTarget.m_sPresetId == newerNative.m_sPresetId
			&& nativeTarget.m_iFactionMoney == newerNative.m_iFactionMoney
			&& nativeTarget.m_iPersistenceRestoreSequence
				== newerNative.m_iPersistenceRestoreSequence + 1;
		report.m_bNewerNativeAuthorityExact = profileWritten
			&& nativeMetadataExact && nativeStateExact
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== profileBytes;

		CleanupJournalArtifacts();
		HST_CampaignSaveData conflictProfile
			= BuildSnapshot(7301, "equal-order-profile");
		conflictProfile.m_iPersistenceCheckpointSequence = 30;
		conflictProfile.m_iPersistenceRestoreSequence = 5;
		conflictProfile.m_iLastSaveSecond = 500;
		profileWritten = journal.WriteVerifiedSnapshot(
			conflictProfile,
			profileReceipt,
			writeEvidence);
		profileBytes = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		HST_CampaignSaveData conflictNative
			= BuildSnapshot(7302, "equal-order-native");
		conflictNative.m_iPersistenceCheckpointSequence = 30;
		conflictNative.m_iPersistenceRestoreSequence = 5;
		conflictNative.m_iLastSaveSecond = 500;
		string conflictNativeFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(conflictNative);
		HST_PersistenceService conflictAuthority = new HST_PersistenceService();
		HST_CampaignState conflictTarget = new HST_CampaignState();
		HST_PersistenceSourceResolution conflictSource
			= conflictAuthority.ResolveValidNativeAuthorityForProof(
				conflictTarget,
				conflictNative,
				conflictNativeFingerprint);
		report.m_bEqualOrderConflictRejectedExact = profileWritten
			&& conflictSource
			&& conflictSource.m_eSource
				== HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL
			&& !conflictSource.HasSelectedState()
			&& conflictSource.m_sEvidence.Contains(
				"equal durable order but different fingerprints")
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== profileBytes;
		report.m_bAuthorityJournalMetadataExact = SourceJournalMetadataExact(
				profileSource,
				true,
				true,
				1,
				HST_CampaignProfileSaveJournalService.SLOT_CANONICAL,
				1,
				false,
				true)
			&& SourceJournalMetadataExact(
				nativeSource,
				true,
				true,
				1,
				HST_CampaignProfileSaveJournalService.SLOT_CANONICAL,
				1,
				false,
				true)
			&& SourceJournalMetadataExact(
				conflictSource,
				true,
				true,
				1,
				HST_CampaignProfileSaveJournalService.SLOT_CANONICAL,
				1,
				false,
				true);
		report.m_sAuthorityOrderEvidence = "profile "
			+ AuthorityEvidence(profileSource) + " | native "
			+ AuthorityEvidence(nativeSource) + " | conflict "
			+ AuthorityEvidence(conflictSource)
			+ string.Format(
				" | journal metadata exact %1",
				report.m_bAuthorityJournalMetadataExact);
	}

	protected void ProveAuthorityTieBreakOrdering(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		string lastSaveJournalEvidence;
		string lastSaveNativeEvidence;
		string equalFingerprintEvidence;
		string checkpointJournalEvidence;
		string checkpointNativeEvidence;
		report.m_bLastSaveSecondNewerJournalExact
			= ProveLastSaveSecondJournalWins(lastSaveJournalEvidence);
		report.m_bLastSaveSecondNewerNativeExact
			= ProveLastSaveSecondNativeWins(lastSaveNativeEvidence);
		report.m_bEqualOrderSameFingerprintNativeExact
			= ProveEqualOrderSameFingerprintNative(equalFingerprintEvidence);
		bool checkpointJournalExact
			= ProveCheckpointSequenceJournalWins(checkpointJournalEvidence);
		bool checkpointNativeExact
			= ProveCheckpointSequenceNativeWins(checkpointNativeEvidence);
		report.m_bCheckpointSequenceOrderingExact
			= checkpointJournalExact && checkpointNativeExact;
		report.m_sAuthorityTieBreakEvidence = "last-save journal "
			+ lastSaveJournalEvidence + " | last-save native "
			+ lastSaveNativeEvidence + " | equal fingerprint "
			+ equalFingerprintEvidence + " | checkpoint journal "
			+ checkpointJournalEvidence + " | checkpoint native "
			+ checkpointNativeEvidence;
	}

	protected bool ProveLastSaveSecondJournalWins(out string evidence)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData profileSave
			= BuildSnapshot(7401, "last-save-newer-profile");
		profileSave.m_iPersistenceCheckpointSequence = 40;
		profileSave.m_iPersistenceRestoreSequence = 6;
		profileSave.m_iLastSaveSecond = 700;
		HST_CampaignProfileSaveWriteReceipt receipt;
		string writeEvidence;
		bool profileWritten = journal.WriteVerifiedSnapshot(
			profileSave,
			receipt,
			writeEvidence);
		string profileBytes = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		HST_CampaignSaveData nativeSave
			= BuildSnapshot(7402, "last-save-older-native");
		nativeSave.m_iPersistenceCheckpointSequence = 40;
		nativeSave.m_iPersistenceRestoreSequence = 6;
		nativeSave.m_iLastSaveSecond = 600;
		string nativeFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(nativeSave);
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CampaignState target = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveValidNativeAuthorityForProof(
				target,
				nativeSave,
				nativeFingerprint);
		bool metadataExact = SourceJournalMetadataExact(
			source,
			true,
			true,
			1,
			HST_CampaignProfileSaveJournalService.SLOT_CANONICAL,
			1,
			false,
			true);
		bool exact = profileWritten && receipt && receipt.m_bSuccess
			&& ProfileSelectionExact(
				source,
				target,
				profileSave,
				receipt.m_sSnapshotFingerprint)
			&& metadataExact && !persistence.IsProfileFallbackOnlyForSession()
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== profileBytes;
		evidence = writeEvidence + " | " + AuthorityEvidence(source)
			+ string.Format(" | metadata %1", metadataExact);
		return exact;
	}

	protected bool ProveLastSaveSecondNativeWins(out string evidence)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData profileSave
			= BuildSnapshot(7501, "last-save-older-profile");
		profileSave.m_iPersistenceCheckpointSequence = 41;
		profileSave.m_iPersistenceRestoreSequence = 7;
		profileSave.m_iLastSaveSecond = 600;
		HST_CampaignProfileSaveWriteReceipt receipt;
		string writeEvidence;
		bool profileWritten = journal.WriteVerifiedSnapshot(
			profileSave,
			receipt,
			writeEvidence);
		string profileBytes = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		HST_CampaignSaveData nativeSave
			= BuildSnapshot(7502, "last-save-newer-native");
		nativeSave.m_iPersistenceCheckpointSequence = 41;
		nativeSave.m_iPersistenceRestoreSequence = 7;
		nativeSave.m_iLastSaveSecond = 700;
		string nativeFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(nativeSave);
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CampaignState target = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveValidNativeAuthorityForProof(
				target,
				nativeSave,
				nativeFingerprint);
		bool metadataExact = SourceJournalMetadataExact(
			source,
			true,
			true,
			1,
			HST_CampaignProfileSaveJournalService.SLOT_CANONICAL,
			1,
			false,
			true);
		bool exact = profileWritten && receipt && receipt.m_bSuccess
			&& NativeSelectionExact(source, target, nativeSave, nativeFingerprint)
			&& metadataExact && source.m_bProfileFallbackRead
			&& !persistence.IsProfileFallbackOnlyForSession()
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== profileBytes;
		evidence = writeEvidence + " | " + AuthorityEvidence(source)
			+ string.Format(" | metadata %1", metadataExact);
		return exact;
	}

	protected bool ProveEqualOrderSameFingerprintNative(out string evidence)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData snapshot
			= BuildSnapshot(7601, "equal-order-same-fingerprint");
		snapshot.m_iPersistenceCheckpointSequence = 42;
		snapshot.m_iPersistenceRestoreSequence = 8;
		snapshot.m_iLastSaveSecond = 800;
		HST_CampaignProfileSaveWriteReceipt receipt;
		string writeEvidence;
		bool profileWritten = journal.WriteVerifiedSnapshot(
			snapshot,
			receipt,
			writeEvidence);
		string profileBytes = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		string nativeFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(snapshot);
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CampaignState target = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveValidNativeAuthorityForProof(
				target,
				snapshot,
				nativeFingerprint);
		bool metadataExact = SourceJournalMetadataExact(
			source,
			true,
			true,
			1,
			HST_CampaignProfileSaveJournalService.SLOT_CANONICAL,
			1,
			false,
			true);
		bool fingerprintExact = receipt
			&& receipt.m_sSnapshotFingerprint == nativeFingerprint;
		bool exact = profileWritten && fingerprintExact
			&& NativeSelectionExact(source, target, snapshot, nativeFingerprint)
			&& metadataExact && source.m_bProfileFallbackRead
			&& !persistence.IsProfileFallbackOnlyForSession()
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== profileBytes;
		evidence = writeEvidence + " | " + AuthorityEvidence(source)
			+ string.Format(
				" | fingerprint/metadata %1/%2",
				fingerprintExact,
				metadataExact);
		return exact;
	}

	protected bool ProveCheckpointSequenceJournalWins(out string evidence)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData profileSave
			= BuildSnapshot(7701, "checkpoint-newer-profile");
		profileSave.m_iPersistenceCheckpointSequence = 51;
		profileSave.m_iPersistenceRestoreSequence = 1;
		profileSave.m_iLastSaveSecond = 1;
		HST_CampaignProfileSaveWriteReceipt receipt;
		string writeEvidence;
		bool profileWritten = journal.WriteVerifiedSnapshot(
			profileSave,
			receipt,
			writeEvidence);
		HST_CampaignSaveData nativeSave
			= BuildSnapshot(7702, "checkpoint-older-native");
		nativeSave.m_iPersistenceCheckpointSequence = 50;
		nativeSave.m_iPersistenceRestoreSequence = 100;
		nativeSave.m_iLastSaveSecond = 1000;
		string nativeFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(nativeSave);
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CampaignState target = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveValidNativeAuthorityForProof(
				target,
				nativeSave,
				nativeFingerprint);
		bool metadataExact = SourceJournalMetadataExact(
			source,
			true,
			true,
			1,
			HST_CampaignProfileSaveJournalService.SLOT_CANONICAL,
			1,
			false,
			true);
		bool exact = profileWritten && receipt
			&& ProfileSelectionExact(
				source,
				target,
				profileSave,
				receipt.m_sSnapshotFingerprint)
			&& metadataExact && !persistence.IsProfileFallbackOnlyForSession();
		evidence = writeEvidence + " | " + AuthorityEvidence(source)
			+ string.Format(" | metadata %1", metadataExact);
		return exact;
	}

	protected bool ProveCheckpointSequenceNativeWins(out string evidence)
	{
		CleanupJournalArtifacts();
		HST_CampaignProfileSaveJournalService journal
			= new HST_CampaignProfileSaveJournalService();
		HST_CampaignSaveData profileSave
			= BuildSnapshot(7801, "checkpoint-older-profile");
		profileSave.m_iPersistenceCheckpointSequence = 60;
		profileSave.m_iPersistenceRestoreSequence = 100;
		profileSave.m_iLastSaveSecond = 1000;
		HST_CampaignProfileSaveWriteReceipt receipt;
		string writeEvidence;
		bool profileWritten = journal.WriteVerifiedSnapshot(
			profileSave,
			receipt,
			writeEvidence);
		HST_CampaignSaveData nativeSave
			= BuildSnapshot(7802, "checkpoint-newer-native");
		nativeSave.m_iPersistenceCheckpointSequence = 61;
		nativeSave.m_iPersistenceRestoreSequence = 1;
		nativeSave.m_iLastSaveSecond = 1;
		string nativeFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(nativeSave);
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CampaignState target = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveValidNativeAuthorityForProof(
				target,
				nativeSave,
				nativeFingerprint);
		bool metadataExact = SourceJournalMetadataExact(
			source,
			true,
			true,
			1,
			HST_CampaignProfileSaveJournalService.SLOT_CANONICAL,
			1,
			false,
			true);
		bool exact = profileWritten
			&& NativeSelectionExact(source, target, nativeSave, nativeFingerprint)
			&& metadataExact && source.m_bProfileFallbackRead
			&& !persistence.IsProfileFallbackOnlyForSession();
		evidence = writeEvidence + " | " + AuthorityEvidence(source)
			+ string.Format(" | metadata %1", metadataExact);
		return exact;
	}

	protected void ProveLegacyRawEqualOrderAuthority(
		HST_CampaignProfileJournalAuthorityProofReport report)
	{
		CleanupJournalArtifacts();
		HST_CampaignSaveData snapshot
			= BuildSnapshot(7901, "legacy-raw-equal-order-native");
		snapshot.m_iSchemaVersion = 70;
		snapshot.m_iLastLoadedSchemaVersion = 70;
		snapshot.m_iPersistenceCheckpointSequence = 0;
		snapshot.m_iPersistenceRestoreSequence = 9;
		snapshot.m_iLastSaveSecond = 900;
		bool rawWritten = WriteFormattedRawSnapshot(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			snapshot);
		string rawBytes = ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		string rawFingerprint
			= HST_CampaignPersistentState.BuildPayloadFingerprint(rawBytes);
		string nativeFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(snapshot);
		bool fingerprintsDistinct = !rawFingerprint.IsEmpty()
			&& !nativeFingerprint.IsEmpty()
			&& rawFingerprint != nativeFingerprint;
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CampaignState target = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveValidNativeAuthorityForProof(
				target,
				snapshot,
				nativeFingerprint);
		bool metadataExact = SourceJournalMetadataExact(
			source,
			true,
			true,
			0,
			HST_CampaignProfileSaveJournalService.SLOT_CANONICAL,
			1,
			true,
			true);
		bool lineageFingerprintExact = source
			&& source.m_sProfileFallbackSnapshotFingerprint == rawFingerprint
			&& source.m_sSelectedSnapshotFingerprint == nativeFingerprint;
		bool bytesExact = rawWritten && !rawBytes.IsEmpty()
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE) == rawBytes
			&& !FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		report.m_bLegacyRawEqualOrderNativeExact = fingerprintsDistinct
			&& metadataExact && lineageFingerprintExact && bytesExact
			&& NativeSelectionExact(source, target, snapshot, nativeFingerprint)
			&& source.m_bProfileFallbackRead
			&& !persistence.IsProfileFallbackOnlyForSession();
		report.m_sLegacyRawAuthorityEvidence = string.Format(
			"raw/distinct/metadata/lineage/bytes %1/%2/%3/%4/%5 | %6",
			rawWritten,
			fingerprintsDistinct,
			metadataExact,
			lineageFingerprintExact,
			bytesExact,
			AuthorityEvidence(source));
	}

	protected bool EstablishThreeGenerations(
		HST_CampaignProfileSaveJournalService journal,
		int seedBase,
		out HST_CampaignSaveData saveOne,
		out HST_CampaignSaveData saveTwo,
		out HST_CampaignSaveData saveThree,
		out HST_CampaignProfileSaveWriteReceipt receiptOne,
		out HST_CampaignProfileSaveWriteReceipt receiptTwo,
		out HST_CampaignProfileSaveWriteReceipt receiptThree,
		out string evidence)
	{
		evidence = "three-generation setup rejected";
		saveOne = BuildSnapshot(seedBase + 1, "journal-generation-one");
		saveTwo = BuildSnapshot(seedBase + 2, "journal-generation-two");
		saveThree = BuildSnapshot(seedBase + 3, "journal-generation-three");
		string evidenceOne;
		string evidenceTwo;
		string evidenceThree;
		bool wroteOne = journal.WriteVerifiedSnapshot(
			saveOne,
			receiptOne,
			evidenceOne);
		bool wroteTwo = wroteOne && journal.WriteVerifiedSnapshot(
			saveTwo,
			receiptTwo,
			evidenceTwo);
		bool wroteThree = wroteTwo && journal.WriteVerifiedSnapshot(
			saveThree,
			receiptThree,
			evidenceThree);
		evidence = "generation one " + evidenceOne
			+ " | generation two " + evidenceTwo
			+ " | generation three " + evidenceThree;
		return wroteOne && wroteTwo && wroteThree
			&& receiptOne && receiptTwo && receiptThree;
	}

	protected bool SourceJournalMetadataExact(
		HST_PersistenceSourceResolution source,
		bool expectedPresent,
		bool expectedRead,
		int expectedGeneration,
		string expectedSlot,
		int expectedValidCount,
		bool expectedLegacyRaw,
		bool expectedChainExact)
	{
		return source
			&& source.m_bProfileFallbackPresent == expectedPresent
			&& source.m_bProfileFallbackRead == expectedRead
			&& source.m_iProfileJournalGeneration == expectedGeneration
			&& source.m_sProfileJournalSlot == expectedSlot
			&& source.m_iProfileJournalValidSlotCount == expectedValidCount
			&& source.m_bProfileJournalLegacyRaw == expectedLegacyRaw
			&& source.m_bProfileJournalChainExact == expectedChainExact;
	}

	protected bool RestoredStateExact(
		HST_CampaignState actual,
		HST_CampaignSaveData expected)
	{
		if (!actual || !expected)
			return false;
		bool identityExact = actual.m_sPresetId == expected.m_sPresetId
			&& actual.m_iCampaignSeed == expected.m_iCampaignSeed
			&& actual.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION;
		bool durableOrderExact
			= actual.m_iPersistenceCheckpointSequence
				== expected.m_iPersistenceCheckpointSequence
			&& actual.m_iPersistenceRestoreSequence
				== expected.m_iPersistenceRestoreSequence + 1
			&& actual.m_iLastSaveSecond == expected.m_iLastSaveSecond;
		bool progressExact = actual.m_iElapsedSeconds == expected.m_iElapsedSeconds
			&& actual.m_iWarLevel == expected.m_iWarLevel
			&& actual.m_iFactionMoney == expected.m_iFactionMoney
			&& actual.m_iHR == expected.m_iHR
			&& actual.m_iTrainingLevel == expected.m_iTrainingLevel
			&& actual.m_iNextAuthoritySequence
				== expected.m_iNextAuthoritySequence;
		return identityExact && durableOrderExact && progressExact
			&& actual.m_bPetrosAlive == expected.m_bPetrosAlive
			&& actual.m_bRestoredFromPersistence;
	}

	protected bool NativeSelectionExact(
		HST_PersistenceSourceResolution source,
		HST_CampaignState target,
		HST_CampaignSaveData nativeSave,
		string nativeFingerprint)
	{
		return source && source.m_State == target
			&& source.m_eSource
				== HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_NATIVE
			&& source.m_bPersistenceSystemAvailable
			&& source.m_bPersistenceSystemLoadedData
			&& source.m_bNativeRecordPresent && source.m_bNativeRecordValid
			&& !source.m_bNativeRecordUnsupportedFuture
			&& !source.m_bDegradedNativeRecovery
			&& source.m_sNativeSnapshotFingerprint == nativeFingerprint
			&& source.m_sSelectedSnapshotFingerprint == nativeFingerprint
			&& RestoredStateExact(target, nativeSave);
	}

	protected bool ProfileSelectionExact(
		HST_PersistenceSourceResolution source,
		HST_CampaignState target,
		HST_CampaignSaveData profileSave,
		string profileFingerprint)
	{
		return source && source.m_State == target
			&& source.m_eSource
				== HST_ECampaignPersistenceSource
					.HST_PERSISTENCE_SOURCE_PROFILE_FALLBACK
			&& source.m_bPersistenceSystemAvailable
			&& source.m_bPersistenceSystemLoadedData
			&& source.m_bNativeRecordPresent && source.m_bNativeRecordValid
			&& !source.m_bNativeRecordUnsupportedFuture
			&& source.m_bProfileFallbackRead
			&& source.m_bDegradedNativeRecovery
			&& source.m_sDegradedNativeRecoveryReason
				== "profile journal ordering is newer than the valid native record"
			&& source.m_sProfileFallbackSnapshotFingerprint
				== profileFingerprint
			&& source.m_sSelectedSnapshotFingerprint == profileFingerprint
			&& RestoredStateExact(target, profileSave);
	}

	protected HST_CampaignSaveData BuildSnapshot(int seed, string label)
	{
		HST_CampaignSaveData save = new HST_CampaignSaveData();
		save.m_iSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		save.m_iLastLoadedSchemaVersion = HST_CampaignState.SCHEMA_VERSION;
		save.m_sPresetId = "journal-autotest-" + label;
		save.m_iCampaignSeed = seed;
		save.m_iElapsedSeconds = seed + 17;
		save.m_iLastSaveSecond = seed + 11;
		save.m_iPersistenceCheckpointSequence = (seed % 11) + 1;
		save.m_iPersistenceRestoreSequence = seed % 7;
		save.m_iWarLevel = (seed % 10) + 1;
		save.m_iFactionMoney = seed * 3;
		save.m_iHR = (seed % 50) + 5;
		save.m_iTrainingLevel = seed % 6;
		save.m_iNextAuthoritySequence = seed + 100;
		save.m_bPetrosAlive = true;
		save.m_sLastPersistenceStatus = label;

		HST_FactionPoolState pool = new HST_FactionPoolState();
		pool.m_sFactionKey = "journal-faction-" + seed.ToString();
		pool.m_iAttackResources = seed + 3;
		pool.m_iSupportResources = seed + 5;
		pool.m_iStrategicRevision = (seed % 13) + 1;
		save.m_aFactionPools.Insert(pool);

		HST_PlayerState player = new HST_PlayerState();
		player.m_sIdentityId = "journal-player-" + seed.ToString();
		player.m_sDisplayName = "Journal Fixture";
		player.m_iMoney = seed + 7;
		player.m_bHasSpawnRecord = true;
		player.m_vLastSpawnPosition = Vector(seed, 12.5, seed + 1);
		save.m_aPlayers.Insert(player);

		HST_ZoneState zone = new HST_ZoneState();
		zone.m_sZoneId = "journal-zone-" + seed.ToString();
		zone.m_sDisplayName = "Journal Zone";
		zone.m_sOwnerFactionKey = pool.m_sFactionKey;
		zone.m_vPosition = Vector(seed + 2, 4.5, seed + 3);
		zone.m_iSupport = seed % 101;
		zone.m_aLinkedZoneIds.Insert("journal-linked-a");
		zone.m_aLinkedZoneIds.Insert("journal-linked-b");
		zone.m_aCombatPresenceContributorIds.Insert("journal-contributor");
		zone.m_aCombatPresenceContributorFacts.Insert("journal-fact");
		save.m_aZones.Insert(zone);

		HST_GarageVehicleState vehicle = new HST_GarageVehicleState();
		vehicle.m_sVehicleId = "journal-vehicle-" + seed.ToString();
		vehicle.m_sPrefab = "{0000000000000000}Journal/Vehicle.et";
		vehicle.m_sDisplayName = "Journal Stored Vehicle";
		vehicle.m_vPosition = Vector(seed + 4, 6.5, seed + 5);
		vehicle.m_vAngles = Vector(75, 0, 0);
		vehicle.m_iStoredAtSecond = seed + 9;
		vehicle.m_fFuel = 0.75;
		vehicle.m_bHadPhysicalCargo = true;

		HST_StoredVehicleCargoState cargo
			= new HST_StoredVehicleCargoState();
		cargo.m_sItemPrefab = "{0000000000000001}Journal/Cargo.et";
		cargo.m_sCategory = "journal-cargo";
		cargo.m_iCount = (seed % 5) + 1;
		vehicle.m_aStoredCargoItems.Insert(cargo);
		save.m_aGarageVehicles.Insert(vehicle);
		return save;
	}

	protected bool SnapshotIdentityExact(
		HST_CampaignSaveData actual,
		HST_CampaignSaveData expected)
	{
		if (!actual || !expected)
			return false;
		string actualFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(actual);
		string expectedFingerprint
			= HST_CampaignPersistentState.BuildSnapshotFingerprint(expected);
		bool richFixtureExact = expected.m_aFactionPools.Count() == 1
			&& expected.m_aPlayers.Count() == 1
			&& expected.m_aZones.Count() == 1
			&& expected.m_aZones[0]
			&& expected.m_aZones[0].m_aLinkedZoneIds.Count() == 2
			&& expected.m_aZones[0]
				.m_aCombatPresenceContributorFacts.Count() == 1
			&& expected.m_aGarageVehicles.Count() == 1
			&& expected.m_aGarageVehicles[0]
			&& expected.m_aGarageVehicles[0].m_aStoredCargoItems.Count() == 1;
		return richFixtureExact && !actualFingerprint.IsEmpty()
			&& actualFingerprint == expectedFingerprint;
	}

	protected bool AmbiguousHistoryFailsClosedAndPreservesBytes(
		HST_CampaignProfileSaveJournalService journal,
		string label,
		out string evidence)
	{
		evidence = "ambiguous history integration rejected";
		if (!journal || label.IsEmpty())
			return false;
		string canonicalBefore = ReadFile(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		string recoveryBefore = ReadFile(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		HST_PersistenceService persistence = new HST_PersistenceService();
		HST_CampaignState fallbackState = new HST_CampaignState();
		HST_PersistenceSourceResolution source
			= persistence.ResolveProfileFallbackAfterNativeFailureForProof(
				fallbackState,
				NATIVE_FAILURE,
				true);
		HST_CampaignProfileSaveWriteReceipt rejectedReceipt;
		string writeEvidence;
		bool writeAccepted = journal.WriteVerifiedSnapshot(
			BuildSnapshot(5999, label + "-write-rejected"),
			rejectedReceipt,
			writeEvidence);
		HST_CampaignProfileSaveResolution after = journal.ResolveJournal();
		bool sourceExact = source
			&& source.m_eSource
				== HST_ECampaignPersistenceSource.HST_PERSISTENCE_SOURCE_FATAL
			&& source.m_bProfileFallbackPresent
			&& !source.m_bProfileFallbackRead
			&& source.m_iProfileJournalValidSlotCount == 2
			&& !source.HasSelectedState();
		bool writerExact = !writeAccepted && rejectedReceipt
			&& !rejectedReceipt.m_bSuccess;
		bool bytesExact = !canonicalBefore.IsEmpty()
			&& !recoveryBefore.IsEmpty()
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				== canonicalBefore
			&& ReadFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE)
				== recoveryBefore;
		bool resolutionExact = after && after.m_bArtifactsPresent
			&& after.m_iValidCandidateCount == 2
			&& after.m_bAmbiguous && !after.m_bHasSelection
			&& !after.m_bUnsupportedFuture;
		evidence = string.Format(
			"%1 source/writer/bytes/resolution %2/%3/%4/%5 | %6 | %7",
			label,
			sourceExact,
			writerExact,
			bytesExact,
			resolutionExact,
			AuthorityEvidence(source),
			writeEvidence);
		return sourceExact && writerExact && bytesExact && resolutionExact;
	}

	protected HST_CampaignPersistentStateLoadResult ClassifyNativeEnvelopeForProof(
		HST_CampaignNativeEnvelopeClassificationProofData envelope)
	{
		if (!envelope)
			return null;
		JsonSaveContext saveContext = new JsonSaveContext();
		if (!saveContext.WriteValue("", envelope))
			return null;
		string payload = saveContext.SaveToString();
		if (payload.IsEmpty())
			return null;
		JsonLoadContext loadContext = new JsonLoadContext();
		if (!loadContext.LoadFromString(payload))
			return null;
		return HST_CampaignPersistentStateLoadClassifier.Classify(loadContext);
	}

	protected bool WriteRawSnapshot(string path, HST_CampaignSaveData save)
	{
		if (!save)
			return false;
		HST_ProfilePathService.EnsureProfileDirectory();
		JsonSaveContext context = new JsonSaveContext();
		return context.WriteValue("", save) && context.SaveToFile(path)
			&& FileIO.FileExists(path);
	}

	protected bool WriteFormattedRawSnapshot(
		string path,
		HST_CampaignSaveData save)
	{
		if (!save)
			return false;
		string payload;
		if (!HST_CampaignPersistentState.TrySerializeLegacySnapshot(
			save,
			payload))
			return false;
		HST_ProfilePathService.EnsureProfileDirectory();
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
			return false;
		file.WriteLine(" " + payload);
		file.Close();
		return FileIO.FileExists(path);
	}

	protected bool WriteEnvelope(
		string path,
		HST_CampaignSaveData save,
		int envelopeVersion,
		int generation,
		int snapshotSchemaVersion,
		int previousGeneration,
		string previousFingerprint,
		string fingerprintOverride)
	{
		return WriteEnvelopeWithMagic(
			path,
			save,
			HST_CampaignProfileSaveJournalService.ENVELOPE_MAGIC,
			envelopeVersion,
			generation,
			snapshotSchemaVersion,
			previousGeneration,
			previousFingerprint,
			fingerprintOverride);
	}

	protected bool WriteEnvelopeWithMagic(
		string path,
		HST_CampaignSaveData save,
		string envelopeMagic,
		int envelopeVersion,
		int generation,
		int snapshotSchemaVersion,
		int previousGeneration,
		string previousFingerprint,
		string fingerprintOverride)
	{
		if (!save)
			return false;
		string payload;
		JsonSaveContext payloadContext = new JsonSaveContext();
		if (!payloadContext.WriteValue("", save))
			return false;
		payload = payloadContext.SaveToString();
		string fingerprint
			= HST_CampaignPersistentState.BuildPayloadFingerprint(payload);
		if (payload.IsEmpty() || fingerprint.IsEmpty())
			return false;
		if (!fingerprintOverride.IsEmpty())
			fingerprint = fingerprintOverride;

		HST_CampaignProfileSaveEnvelope envelope
			= new HST_CampaignProfileSaveEnvelope();
		envelope.m_sMagic = envelopeMagic;
		envelope.m_iEnvelopeVersion = envelopeVersion;
		envelope.m_iGeneration = generation;
		envelope.m_iSnapshotSchemaVersion = snapshotSchemaVersion;
		envelope.m_sSnapshotFingerprint = fingerprint;
		envelope.m_iPreviousGeneration = previousGeneration;
		envelope.m_sPreviousSnapshotFingerprint = previousFingerprint;
		envelope.m_sSnapshotPayload = payload;
		HST_ProfilePathService.EnsureProfileDirectory();
		JsonSaveContext context = new JsonSaveContext();
		return context.WriteValue("", envelope) && context.SaveToFile(path)
			&& FileIO.FileExists(path);
	}

	protected bool WriteTruncatedFile(string path)
	{
		HST_ProfilePathService.EnsureProfileDirectory();
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
			return false;
		file.WriteLine("{");
		file.Close();
		return FileIO.FileExists(path);
	}

	protected string ReadFile(string path)
	{
		if (!FileIO.FileExists(path))
			return "";
		return SCR_FileIOHelper.GetFileStringContent(path, false);
	}

	protected bool CleanupJournalArtifacts()
	{
		if (FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_SAVE_FILE))
			FileIO.DeleteFile(HST_ProfilePathService.CAMPAIGN_SAVE_FILE);
		if (FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE))
			FileIO.DeleteFile(HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
		return !FileIO.FileExists(HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
			&& !FileIO.FileExists(
				HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
	}

	protected string DescribeResolution(
		HST_CampaignProfileSaveResolution resolution)
	{
		if (!resolution)
			return "resolution missing";
		string selected = "none";
		if (resolution.m_Selected)
		{
			selected = string.Format(
				"%1:%2",
				resolution.m_Selected.m_sSlotLabel,
				resolution.m_Selected.m_iGeneration);
		}
		return string.Format(
			"artifacts/valid/selected/future/ambiguous/chain %1/%2/%3/%4/%5/%6 | %7",
			resolution.m_bArtifactsPresent,
			resolution.m_iValidCandidateCount,
			selected,
			resolution.m_bUnsupportedFuture,
			resolution.m_bAmbiguous,
			resolution.m_bChainExact,
			resolution.m_sEvidence);
	}

	protected string AuthorityEvidence(HST_PersistenceSourceResolution source)
	{
		if (!source)
			return "missing";
		return source.BuildSourceLabel() + " | " + source.m_sEvidence;
	}
}
#endif
