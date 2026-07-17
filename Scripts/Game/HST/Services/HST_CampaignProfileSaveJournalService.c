// Crash-tolerant two-generation JSON mirror for campaign authority. FileIO has
// no atomic rename or exclusive lock, so the writer advances only the inactive
// slot and verifies that write before it can supersede the prior valid slot.
class HST_CampaignProfileSaveJournalService
{
	static const string ENVELOPE_MAGIC
		= "partisan_campaign_profile_journal_v1";
	static const int ENVELOPE_VERSION = 1;
	static const string SLOT_CANONICAL = "canonical";
	static const string SLOT_RECOVERY = "recovery";

	bool CanAdvanceVerifiedSnapshot(out string evidence)
	{
		evidence = "profile journal can advance";
		HST_CampaignProfileSaveResolution resolution = ResolveJournal();
		if (!resolution)
		{
			evidence = "profile journal resolution is unavailable";
			return false;
		}
		if (resolution.m_bUnsupportedFuture || resolution.m_bAmbiguous)
		{
			evidence
				= "profile journal has unsupported or ambiguous write-fenced authority";
			if (!resolution.m_sEvidence.IsEmpty())
				evidence += " | " + resolution.m_sEvidence;
			return false;
		}
		if (resolution.m_bHasSelection && resolution.m_Selected
			&& resolution.m_Selected.m_iGeneration >= int.MAX)
		{
			evidence = "profile journal generation space is exhausted";
			return false;
		}
		return true;
	}

	HST_CampaignProfileSaveResolution ResolveJournal()
	{
		HST_CampaignProfileSaveResolution result
			= new HST_CampaignProfileSaveResolution();
		result.m_Canonical = ReadCandidate(
			HST_ProfilePathService.CAMPAIGN_SAVE_FILE,
			SLOT_CANONICAL,
			true);
		result.m_Recovery = ReadCandidate(
			HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE,
			SLOT_RECOVERY,
			false);
		result.m_bArtifactsPresent
			= result.m_Canonical.m_bArtifactPresent
				|| result.m_Recovery.m_bArtifactPresent;
		result.m_bUnsupportedFuture
			= result.m_Canonical.m_bUnsupportedFuture
				|| result.m_Recovery.m_bUnsupportedFuture;
		if (result.m_Canonical.m_bValid)
			result.m_iValidCandidateCount++;
		if (result.m_Recovery.m_bValid)
			result.m_iValidCandidateCount++;

		// A recognizable future envelope or snapshot must never fall back to an
		// older generation and then overwrite the newer format.
		if (result.m_bUnsupportedFuture)
		{
			result.m_sEvidence = "profile journal rejected an unsupported future format"
				+ " | canonical " + result.m_Canonical.m_sEvidence
				+ " | recovery " + result.m_Recovery.m_sEvidence;
			return result;
		}

		if (result.m_iValidCandidateCount <= 0)
		{
			if (!result.m_bArtifactsPresent)
				result.m_sEvidence = "profile journal has no artifacts";
			else
			{
				result.m_sEvidence = "profile journal artifacts contain no valid generation"
					+ " | canonical " + result.m_Canonical.m_sEvidence
					+ " | recovery " + result.m_Recovery.m_sEvidence;
			}
			return result;
		}

		if (result.m_iValidCandidateCount == 1)
		{
			if (result.m_Canonical.m_bValid)
				result.m_Selected = result.m_Canonical;
			else
				result.m_Selected = result.m_Recovery;

			result.m_bHasSelection = true;
			result.m_bChainExact = IsStandaloneCandidateChainExact(
				result.m_Selected);
			result.m_sEvidence = string.Format(
				"profile journal selected sole valid slot %1 generation %2 | chain exact %3",
				result.m_Selected.m_sSlotLabel,
				result.m_Selected.m_iGeneration,
				result.m_bChainExact);
			return result;
		}

		HST_CampaignProfileSaveCandidate canonical = result.m_Canonical;
		HST_CampaignProfileSaveCandidate recovery = result.m_Recovery;
		if (canonical.m_iGeneration == recovery.m_iGeneration)
		{
			if (!AreExactDuplicateCandidates(canonical, recovery))
			{
				result.m_bAmbiguous = true;
				result.m_sEvidence = string.Format(
					"profile journal split brain or metadata conflict at generation %1",
					canonical.m_iGeneration);
				return result;
			}

			// Exact duplicate generations are safe. Prefer canonical
			// deterministically so the next write replaces recovery. Two equal
			// generations do not prove a parent/child chain, even when generation
			// one is independently valid.
			result.m_Selected = canonical;
			result.m_bHasSelection = true;
			result.m_bChainExact = false;
			result.m_sEvidence = string.Format(
				"profile journal selected exact duplicate canonical generation %1 | chain exact %2",
				canonical.m_iGeneration,
				result.m_bChainExact);
			return result;
		}

		HST_CampaignProfileSaveCandidate newer;
		HST_CampaignProfileSaveCandidate older;
		if (canonical.m_iGeneration > recovery.m_iGeneration)
		{
			newer = canonical;
			older = recovery;
		}
		else
		{
			newer = recovery;
			older = canonical;
		}

		bool adjacent = newer.m_iGeneration == older.m_iGeneration + 1;
		bool parentExact = newer.m_iPreviousGeneration == older.m_iGeneration
			&& newer.m_sPreviousSnapshotFingerprint
				== older.m_sSnapshotFingerprint;
		if (!adjacent || !parentExact)
		{
			result.m_bAmbiguous = true;
			result.m_sEvidence = string.Format(
				"profile journal valid generations have a broken chain | older/newer %1/%2 | adjacent/parent %3/%4",
				older.m_iGeneration,
				newer.m_iGeneration,
				adjacent,
				parentExact);
			return result;
		}

		result.m_Selected = newer;
		result.m_bHasSelection = true;
		result.m_bChainExact = true;
		result.m_sEvidence = string.Format(
			"profile journal selected %1 generation %2 over %3 generation %4 | chain exact",
			newer.m_sSlotLabel,
			newer.m_iGeneration,
			older.m_sSlotLabel,
			older.m_iGeneration);
		return result;
	}

	bool ReadSelectedSnapshot(
		out HST_CampaignSaveData saveData,
		out HST_CampaignProfileSaveResolution resolution,
		out string evidence)
	{
		saveData = null;
		resolution = ResolveJournal();
		evidence = "profile journal has no selected snapshot";
		if (!resolution || !resolution.m_bHasSelection
			|| !resolution.m_Selected
			|| !resolution.m_Selected.m_bValid
			|| !resolution.m_Selected.m_SaveData)
		{
			if (resolution)
				evidence = resolution.m_sEvidence;
			return false;
		}

		saveData = resolution.m_Selected.m_SaveData;
		evidence = resolution.m_sEvidence;
		return true;
	}

	bool WriteVerifiedSnapshot(
		HST_CampaignSaveData saveData,
		out HST_CampaignProfileSaveWriteReceipt receipt,
		out string evidence)
	{
		receipt = new HST_CampaignProfileSaveWriteReceipt();
		evidence = "profile journal write rejected";
		receipt.m_sEvidence = evidence;
		if (!saveData)
		{
			receipt.m_sEvidence = "profile journal write has no snapshot";
			evidence = receipt.m_sEvidence;
			return false;
		}
		if (saveData.m_iSchemaVersion != HST_CampaignState.SCHEMA_VERSION)
		{
			receipt.m_sEvidence = string.Format(
				"profile journal write rejected non-current schema %1",
				saveData.m_iSchemaVersion);
			evidence = receipt.m_sEvidence;
			return false;
		}

		string snapshotPayload;
		string snapshotFingerprint;
		if (!HST_CampaignPersistentState.TrySerializeSnapshot(
			saveData,
			snapshotPayload,
			snapshotFingerprint))
		{
			receipt.m_sEvidence
				= "profile journal snapshot serialization failed";
			evidence = receipt.m_sEvidence;
			return false;
		}

		HST_CampaignProfileSaveResolution before = ResolveJournal();
		receipt.m_Before = before;
		if (!before || before.m_bUnsupportedFuture || before.m_bAmbiguous)
		{
			receipt.m_sEvidence
				= "profile journal refused to overwrite an unsupported or ambiguous history";
			if (before && !before.m_sEvidence.IsEmpty())
				receipt.m_sEvidence += " | " + before.m_sEvidence;
			evidence = receipt.m_sEvidence;
			return false;
		}

		string targetPath;
		string targetSlot;
		int nextGeneration = 1;
		int previousGeneration;
		string previousFingerprint;
		if (before.m_bHasSelection && before.m_Selected)
		{
			if (before.m_Selected.m_iGeneration >= int.MAX)
			{
				receipt.m_sEvidence
					= "profile journal generation space is exhausted";
				evidence = receipt.m_sEvidence;
				return false;
			}

			nextGeneration = before.m_Selected.m_iGeneration + 1;
			previousGeneration = before.m_Selected.m_iGeneration;
			previousFingerprint
				= before.m_Selected.m_sSnapshotFingerprint;
			if (before.m_Selected.m_sSlotLabel == SLOT_CANONICAL)
			{
				targetPath = HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE;
				targetSlot = SLOT_RECOVERY;
			}
			else
			{
				targetPath = HST_ProfilePathService.CAMPAIGN_SAVE_FILE;
				targetSlot = SLOT_CANONICAL;
			}
		}
		else
		{
			// Preserve a lone or primary corrupt artifact while re-establishing one
			// verified generation from current live authority.
			if (before.m_Canonical.m_bArtifactPresent)
			{
				targetPath = HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE;
				targetSlot = SLOT_RECOVERY;
			}
			else
			{
				targetPath = HST_ProfilePathService.CAMPAIGN_SAVE_FILE;
				targetSlot = SLOT_CANONICAL;
			}
		}

		HST_CampaignProfileSaveEnvelope envelope
			= new HST_CampaignProfileSaveEnvelope();
		envelope.m_sMagic = ENVELOPE_MAGIC;
		envelope.m_iEnvelopeVersion = ENVELOPE_VERSION;
		envelope.m_iGeneration = nextGeneration;
		envelope.m_iSnapshotSchemaVersion = saveData.m_iSchemaVersion;
		envelope.m_sSnapshotFingerprint = snapshotFingerprint;
		envelope.m_iPreviousGeneration = previousGeneration;
		envelope.m_sPreviousSnapshotFingerprint = previousFingerprint;
		envelope.m_sSnapshotPayload = snapshotPayload;

		HST_ProfilePathService.EnsureProfileDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", envelope) || !context.SaveToFile(targetPath))
		{
			receipt.m_sEvidence = string.Format(
				"profile journal generation %1 write failed at slot %2",
				nextGeneration,
				targetSlot);
			evidence = receipt.m_sEvidence;
			return false;
		}

		bool targetAllowsLegacy = targetSlot == SLOT_CANONICAL;
		HST_CampaignProfileSaveCandidate readBack = ReadCandidate(
			targetPath,
			targetSlot,
			targetAllowsLegacy);
		bool readBackExact = readBack && readBack.m_bValid
			&& readBack.m_bEnvelopeRecognized
			&& !readBack.m_bLegacyRaw
			&& readBack.m_iGeneration == nextGeneration
			&& readBack.m_iSnapshotSchemaVersion == saveData.m_iSchemaVersion
			&& readBack.m_sSnapshotFingerprint == snapshotFingerprint
			&& readBack.m_iPreviousGeneration == previousGeneration
			&& readBack.m_sPreviousSnapshotFingerprint == previousFingerprint
			&& readBack.m_sSnapshotPayload == snapshotPayload;
		if (!readBackExact)
		{
			receipt.m_sEvidence = string.Format(
				"profile journal generation %1 readback failed at slot %2 | %3",
				nextGeneration,
				targetSlot,
				readBack.m_sEvidence);
			evidence = receipt.m_sEvidence;
			return false;
		}

		HST_CampaignProfileSaveResolution after = ResolveJournal();
		receipt.m_After = after;
		bool promotedExact = after && after.m_bHasSelection
			&& !after.m_bUnsupportedFuture && !after.m_bAmbiguous
			&& after.m_Selected
			&& after.m_Selected.m_sSlotLabel == targetSlot
			&& after.m_Selected.m_iGeneration == nextGeneration
			&& after.m_Selected.m_sSnapshotFingerprint == snapshotFingerprint;
		if (!promotedExact)
		{
			receipt.m_sEvidence = string.Format(
				"profile journal generation %1 did not become the selected slot %2",
				nextGeneration,
				targetSlot);
			if (after && !after.m_sEvidence.IsEmpty())
				receipt.m_sEvidence += " | " + after.m_sEvidence;
			evidence = receipt.m_sEvidence;
			return false;
		}

		receipt.m_bSuccess = true;
		receipt.m_sWrittenSlotLabel = targetSlot;
		receipt.m_iGeneration = nextGeneration;
		receipt.m_iSnapshotSchemaVersion = saveData.m_iSchemaVersion;
		receipt.m_sSnapshotFingerprint = snapshotFingerprint;
		receipt.m_iPreviousGeneration = previousGeneration;
		receipt.m_sPreviousSnapshotFingerprint = previousFingerprint;
		receipt.m_sEvidence = string.Format(
			"profile journal wrote and selected slot %1 generation %2 | parent %3 | fingerprint %4",
			targetSlot,
			nextGeneration,
			previousGeneration,
			snapshotFingerprint);
		evidence = receipt.m_sEvidence;
		return true;
	}

	protected HST_CampaignProfileSaveCandidate ReadCandidate(
		string path,
		string slotLabel,
		bool allowLegacyRaw)
	{
		HST_CampaignProfileSaveCandidate result
			= new HST_CampaignProfileSaveCandidate();
		result.m_sPath = path;
		result.m_sSlotLabel = slotLabel;
		result.m_bArtifactPresent = FileIO.FileExists(path);
		if (!result.m_bArtifactPresent)
		{
			result.m_sEvidence = "missing";
			return result;
		}

		string filePayload = SCR_FileIOHelper.GetFileStringContent(path, false);
		if (filePayload.IsEmpty())
		{
			result.m_sEvidence = "empty or unreadable";
			return result;
		}

		HST_CampaignProfileSaveEnvelope envelope;
		JsonLoadContext envelopeContext = new JsonLoadContext();
		if (envelopeContext.LoadFromString(filePayload))
		{
			envelope = new HST_CampaignProfileSaveEnvelope();
			if (!envelopeContext.ReadValue("", envelope))
				envelope = null;
		}
		if (IsRecognizableEnvelope(envelope))
		{
			result.m_bEnvelopeRecognized = true;
			ValidateEnvelopeCandidate(result, envelope);
			return result;
		}

		if (!allowLegacyRaw)
		{
			result.m_sEvidence
				= "non-envelope data is not allowed in the recovery slot";
			return result;
		}

		JsonLoadContext rawContext = new JsonLoadContext();
		if (!rawContext.LoadFromString(filePayload))
		{
			result.m_sEvidence = "legacy raw JSON load failed";
			return result;
		}
		HST_CampaignSaveData rawSave = new HST_CampaignSaveData();
		if (!rawContext.ReadValue("", rawSave))
		{
			result.m_sEvidence = "legacy raw DTO read failed";
			return result;
		}
		if (rawSave.m_iSchemaVersion > HST_CampaignState.SCHEMA_VERSION)
		{
			result.m_bUnsupportedFuture = true;
			result.m_sEvidence = string.Format(
				"legacy raw schema %1 is newer than current schema %2",
				rawSave.m_iSchemaVersion,
				HST_CampaignState.SCHEMA_VERSION);
			return result;
		}
		if (rawSave.m_iSchemaVersion == HST_CampaignState.SCHEMA_VERSION)
		{
			result.m_sEvidence
				= "current-schema raw campaign data requires a journal envelope";
			return result;
		}
		if (!HST_CampaignPersistentState.IsSnapshotSchemaSupported(rawSave))
		{
			result.m_sEvidence = string.Format(
				"legacy raw schema %1 is invalid",
				rawSave.m_iSchemaVersion);
			return result;
		}

		string rawFingerprint
			= HST_CampaignPersistentState.BuildPayloadFingerprint(filePayload);
		if (rawFingerprint.IsEmpty())
		{
			result.m_sEvidence = "legacy raw fingerprint failed";
			return result;
		}

		result.m_bLegacyRaw = true;
		result.m_bValid = true;
		result.m_iGeneration = 0;
		result.m_iSnapshotSchemaVersion = rawSave.m_iSchemaVersion;
		result.m_sSnapshotFingerprint = rawFingerprint;
		result.m_sSnapshotPayload = filePayload;
		result.m_SaveData = rawSave;
		result.m_sEvidence = string.Format(
			"valid legacy raw generation 0 schema %1",
			rawSave.m_iSchemaVersion);
		return result;
	}

	protected bool IsRecognizableEnvelope(
		HST_CampaignProfileSaveEnvelope envelope)
	{
		return envelope
			&& (!envelope.m_sMagic.IsEmpty()
				|| envelope.m_iEnvelopeVersion != 0
				|| envelope.m_iGeneration != 0
				|| envelope.m_iSnapshotSchemaVersion != 0
				|| !envelope.m_sSnapshotFingerprint.IsEmpty()
				|| envelope.m_iPreviousGeneration != 0
				|| !envelope.m_sPreviousSnapshotFingerprint.IsEmpty()
				|| !envelope.m_sSnapshotPayload.IsEmpty());
	}

	protected void ValidateEnvelopeCandidate(
		HST_CampaignProfileSaveCandidate result,
		HST_CampaignProfileSaveEnvelope envelope)
	{
		if (!result || !envelope)
			return;

		result.m_iGeneration = envelope.m_iGeneration;
		result.m_iSnapshotSchemaVersion
			= envelope.m_iSnapshotSchemaVersion;
		result.m_sSnapshotFingerprint = envelope.m_sSnapshotFingerprint;
		result.m_iPreviousGeneration = envelope.m_iPreviousGeneration;
		result.m_sPreviousSnapshotFingerprint
			= envelope.m_sPreviousSnapshotFingerprint;
		result.m_sSnapshotPayload = envelope.m_sSnapshotPayload;

		if (envelope.m_sMagic != ENVELOPE_MAGIC)
		{
			// Any parsed envelope with an explicit unknown identity may belong to
			// a newer format, even if that format no longer uses our reserved
			// prefix. Fence it from selection and all subsequent journal writes.
			if (!envelope.m_sMagic.IsEmpty())
				result.m_bUnsupportedFuture = true;
			result.m_sEvidence = "profile journal envelope magic rejected";
			return;
		}
		if (envelope.m_iEnvelopeVersion > ENVELOPE_VERSION)
		{
			result.m_bUnsupportedFuture = true;
			result.m_sEvidence = string.Format(
				"profile journal envelope version %1 is newer than supported version %2",
				envelope.m_iEnvelopeVersion,
				ENVELOPE_VERSION);
			return;
		}
		if (envelope.m_iEnvelopeVersion != ENVELOPE_VERSION)
		{
			result.m_sEvidence = "profile journal envelope version rejected";
			return;
		}
		if (envelope.m_iSnapshotSchemaVersion
			> HST_CampaignState.SCHEMA_VERSION)
		{
			result.m_bUnsupportedFuture = true;
			result.m_sEvidence = string.Format(
				"profile journal snapshot schema %1 is newer than current schema %2",
				envelope.m_iSnapshotSchemaVersion,
				HST_CampaignState.SCHEMA_VERSION);
			return;
		}
		if (envelope.m_iGeneration < 1
			|| envelope.m_iSnapshotSchemaVersion < 1
			|| envelope.m_sSnapshotFingerprint.IsEmpty()
			|| envelope.m_sSnapshotPayload.IsEmpty())
		{
			result.m_sEvidence = "profile journal envelope required fields rejected";
			return;
		}
		if (envelope.m_iGeneration == 1)
		{
			if (envelope.m_iPreviousGeneration != 0)
			{
				result.m_sEvidence
					= "profile journal generation 1 has a nonzero parent generation";
				return;
			}
		}
		else if (envelope.m_iPreviousGeneration
				!= envelope.m_iGeneration - 1
			|| envelope.m_sPreviousSnapshotFingerprint.IsEmpty())
		{
			result.m_sEvidence = "profile journal envelope parent identity rejected";
			return;
		}

		string payloadFingerprint
			= HST_CampaignPersistentState.BuildPayloadFingerprint(
				envelope.m_sSnapshotPayload);
		if (payloadFingerprint != envelope.m_sSnapshotFingerprint)
		{
			result.m_sEvidence
				= "profile journal exact payload fingerprint rejected";
			return;
		}

		JsonLoadContext payloadContext = new JsonLoadContext();
		if (!payloadContext.LoadFromString(envelope.m_sSnapshotPayload))
		{
			result.m_sEvidence = "profile journal snapshot payload JSON load failed";
			return;
		}
		HST_CampaignSaveData saveData = new HST_CampaignSaveData();
		if (!payloadContext.ReadValue("", saveData))
		{
			result.m_sEvidence = "profile journal snapshot payload DTO read failed";
			return;
		}
		if (saveData.m_iSchemaVersion > HST_CampaignState.SCHEMA_VERSION)
		{
			result.m_bUnsupportedFuture = true;
			result.m_sEvidence = string.Format(
				"profile journal payload schema %1 is newer than current schema %2",
				saveData.m_iSchemaVersion,
				HST_CampaignState.SCHEMA_VERSION);
			return;
		}
		if (saveData.m_iSchemaVersion != envelope.m_iSnapshotSchemaVersion
			|| !HST_CampaignPersistentState.IsSnapshotSchemaSupported(saveData))
		{
			result.m_sEvidence = string.Format(
				"profile journal payload/envelope schema mismatch %1/%2",
				saveData.m_iSchemaVersion,
				envelope.m_iSnapshotSchemaVersion);
			return;
		}

		result.m_bValid = true;
		result.m_SaveData = saveData;
		result.m_sEvidence = string.Format(
			"valid envelope generation %1 schema %2",
			envelope.m_iGeneration,
			envelope.m_iSnapshotSchemaVersion);
	}

	protected bool AreExactDuplicateCandidates(
		HST_CampaignProfileSaveCandidate first,
		HST_CampaignProfileSaveCandidate second)
	{
		if (!first || !second || !first.m_bValid || !second.m_bValid)
			return false;
		return first.m_iGeneration == second.m_iGeneration
			&& first.m_iSnapshotSchemaVersion
				== second.m_iSnapshotSchemaVersion
			&& first.m_sSnapshotFingerprint == second.m_sSnapshotFingerprint
			&& first.m_sSnapshotPayload == second.m_sSnapshotPayload
			&& first.m_iPreviousGeneration == second.m_iPreviousGeneration
			&& first.m_sPreviousSnapshotFingerprint
				== second.m_sPreviousSnapshotFingerprint
			&& first.m_bLegacyRaw == second.m_bLegacyRaw;
	}

	protected bool IsStandaloneCandidateChainExact(
		HST_CampaignProfileSaveCandidate candidate)
	{
		if (!candidate || !candidate.m_bValid)
			return false;
		if (candidate.m_iGeneration == 0)
			return candidate.m_bLegacyRaw;
		return candidate.m_iGeneration == 1
			&& candidate.m_iPreviousGeneration == 0
			&& candidate.m_sPreviousSnapshotFingerprint.IsEmpty();
	}
}
