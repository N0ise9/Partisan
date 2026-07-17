// Plain classification carrier shared by the engine serializer and focused
// diagnostics. Unlike PersistentState and ScriptedStateSerializer, this object
// is safe to construct directly in script.
class HST_CampaignPersistentStateLoadResult
{
	bool m_bRecordPresent;
	bool m_bRecordValid;
	bool m_bRecordUnsupportedFuture;
	string m_sValidationFailure;
	string m_sSnapshotFingerprint;
	ref HST_CampaignSaveData m_Snapshot;

	void SetInvalid(string failure)
	{
		m_bRecordPresent = true;
		m_bRecordValid = false;
		m_bRecordUnsupportedFuture = false;
		m_sValidationFailure = failure;
		m_sSnapshotFingerprint = "";
		m_Snapshot = null;
	}

	void SetUnsupportedFuture(string failure)
	{
		m_bRecordPresent = true;
		m_bRecordValid = false;
		m_bRecordUnsupportedFuture = true;
		m_sValidationFailure = failure;
		m_sSnapshotFingerprint = "";
		m_Snapshot = null;
	}

	void SetValid(
		HST_CampaignSaveData snapshot,
		string snapshotFingerprint)
	{
		m_bRecordPresent = true;
		m_bRecordValid = snapshot && !snapshotFingerprint.IsEmpty();
		m_bRecordUnsupportedFuture = false;
		m_sValidationFailure = "";
		m_sSnapshotFingerprint = snapshotFingerprint;
		m_Snapshot = snapshot;
		if (!m_bRecordValid)
			SetInvalid("native campaign payload classification rejected");
	}
}

// Engine-created transport proxy for the manually constructed campaign save
// snapshot. PersistentState instances are owned by the persistence system and
// must never be constructed directly by gameplay code.
class HST_CampaignPersistentState : PersistentState
{
	static const string LEGACY_ENVELOPE_MAGIC
		= "partisan_campaign_persistent_state_v1";
	static const int LEGACY_ENVELOPE_VERSION = 1;
	static const string ENVELOPE_MAGIC
		= "partisan_campaign_persistent_state_v2";
	static const int ENVELOPE_VERSION = 2;

	protected bool m_bLoadedRecordPresent;
	protected bool m_bLoadedRecordValid;
	protected bool m_bLoadedRecordUnsupportedFuture;
	protected string m_sValidationFailure;
	protected string m_sSnapshotFingerprint;
	protected ref HST_CampaignSaveData m_Snapshot;

	void SetSnapshotForSave(HST_CampaignSaveData snapshot)
	{
		m_Snapshot = snapshot;
		m_sSnapshotFingerprint = BuildSnapshotFingerprint(snapshot);
	}

	void ApplyLoadedClassification(
		HST_CampaignPersistentStateLoadResult result)
	{
		if (!result)
		{
			SetLoadedInvalid(
				"native campaign payload classification was unavailable");
			return;
		}

		m_bLoadedRecordPresent = result.m_bRecordPresent;
		m_bLoadedRecordValid = result.m_bRecordValid;
		m_bLoadedRecordUnsupportedFuture
			= result.m_bRecordUnsupportedFuture;
		m_sValidationFailure = result.m_sValidationFailure;
		m_sSnapshotFingerprint = result.m_sSnapshotFingerprint;
		m_Snapshot = result.m_Snapshot;
	}

	void SetLoadedInvalid(string failure)
	{
		m_bLoadedRecordPresent = true;
		m_bLoadedRecordValid = false;
		m_bLoadedRecordUnsupportedFuture = false;
		m_sValidationFailure = failure;
		m_sSnapshotFingerprint = "";
		m_Snapshot = null;
	}

	void SetLoadedUnsupportedFuture(string failure)
	{
		m_bLoadedRecordPresent = true;
		m_bLoadedRecordValid = false;
		m_bLoadedRecordUnsupportedFuture = true;
		m_sValidationFailure = failure;
		m_sSnapshotFingerprint = "";
		m_Snapshot = null;
	}

	bool HasLoadedRecord()
	{
		return m_bLoadedRecordPresent;
	}

	bool IsLoadedRecordValid()
	{
		return m_bLoadedRecordPresent && m_bLoadedRecordValid && m_Snapshot;
	}

	bool IsLoadedRecordUnsupportedFuture()
	{
		return m_bLoadedRecordPresent && m_bLoadedRecordUnsupportedFuture;
	}

	HST_CampaignSaveData GetSnapshot()
	{
		return m_Snapshot;
	}

	string GetSnapshotFingerprint()
	{
		return m_sSnapshotFingerprint;
	}

	string GetValidationFailure()
	{
		return m_sValidationFailure;
	}

	static string BuildSnapshotFingerprint(HST_CampaignSaveData snapshot)
	{
		string payload;
		string fingerprint;
		if (!TrySerializeSnapshot(snapshot, payload, fingerprint))
			return "";
		return fingerprint;
	}

	static bool TrySerializeSnapshot(
		HST_CampaignSaveData snapshot,
		out string payload,
		out string fingerprint)
	{
		payload = "";
		fingerprint = "";
		if (!snapshot)
			return false;

		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", snapshot))
			return false;
		payload = context.SaveToString();
		fingerprint = BuildPayloadFingerprint(payload);
		return !payload.IsEmpty() && !fingerprint.IsEmpty();
	}

	static string BuildPayloadFingerprint(string payload)
	{
		if (payload.IsEmpty())
			return "";
		// UUID v8 is generated natively from the first 128 SHA-256 bits of the
		// namespace and payload. Persist the algorithm tag and serialized-length contract
		// so engine string.Hash() changes cannot invalidate intact campaign data.
		UUID digest = UUID.GenV8(UUID.NAMESPACE_OID, payload);
		if (digest.IsNull())
			return "";
		return string.Format(
			"uuidv8-sha256-v1:%1:%2",
			payload.Length(),
			digest);
	}

	static string BuildLegacySnapshotFingerprint(HST_CampaignSaveData snapshot)
	{
		string payload;
		if (!TrySerializeLegacySnapshot(snapshot, payload))
			return "";
		return string.Format("%1:%2", payload.Length(), payload.Hash());
	}

	// Native envelope v1 was written by Campaign Schema 70 before
	// m_iPersistenceCheckpointSequence existed in HST_CampaignSaveData. Merely
	// reserializing that row through the current DTO inserts the new zero-valued
	// field and changes the historical length/hash identity. Reconstruct the one
	// exact pre-71 top-level segment instead; v1 never carried a Schema-71 row.
	static bool TrySerializeLegacySnapshot(
		HST_CampaignSaveData snapshot,
		out string payload)
	{
		payload = "";
		if (!snapshot || snapshot.m_iSchemaVersion != 70
			|| snapshot.m_iPersistenceCheckpointSequence != 0)
			return false;

		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", snapshot))
			return false;
		string currentPayload = context.SaveToString();
		if (currentPayload.IsEmpty())
			return false;

		string currentSegment = string.Format(
			"\"m_iPersistenceRestoreSequence\":%1,\"m_iPersistenceCheckpointSequence\":0,\"m_iForceSpawnQueueReconciledRestoreSequence\":%2",
			snapshot.m_iPersistenceRestoreSequence,
			snapshot.m_iForceSpawnQueueReconciledRestoreSequence);
		string legacySegment = string.Format(
			"\"m_iPersistenceRestoreSequence\":%1,\"m_iForceSpawnQueueReconciledRestoreSequence\":%2",
			snapshot.m_iPersistenceRestoreSequence,
			snapshot.m_iForceSpawnQueueReconciledRestoreSequence);
		int segmentStart = currentPayload.IndexOf(currentSegment);
		if (segmentStart < 0)
			return false;
		int suffixStart = segmentStart + currentSegment.Length();
		payload = currentPayload.Substring(0, segmentStart)
			+ legacySegment
			+ currentPayload.Substring(
				suffixStart,
				currentPayload.Length() - suffixStart);
		return !payload.IsEmpty()
			&& payload.IndexOf(
				"\"m_iPersistenceCheckpointSequence\":") < 0;
	}

	static bool TryValidateAndNormalizeSnapshotFingerprint(
		HST_CampaignSaveData snapshot,
		int envelopeVersion,
		string storedFingerprint,
		out string normalizedFingerprint)
	{
		normalizedFingerprint = "";
		if (!snapshot || storedFingerprint.IsEmpty()
			|| envelopeVersion != LEGACY_ENVELOPE_VERSION)
			return false;
		string expectedFingerprint
			= BuildLegacySnapshotFingerprint(snapshot);
		if (expectedFingerprint.IsEmpty()
			|| storedFingerprint != expectedFingerprint)
			return false;
		normalizedFingerprint = BuildSnapshotFingerprint(snapshot);
		return !normalizedFingerprint.IsEmpty();
	}

	static bool IsSnapshotSchemaSupported(HST_CampaignSaveData snapshot)
	{
		return snapshot
			&& snapshot.m_iSchemaVersion >= 1
			&& snapshot.m_iSchemaVersion <= HST_CampaignState.SCHEMA_VERSION;
	}
}

// Pure load classifier for native campaign rows. The current envelope hashes
// and stores one exact JSON payload string, so adding fields to the runtime DTO
// cannot invalidate an intact older v2 row merely by changing reserialization.
// Native v1 remains the historical Schema-70 structured-snapshot layout.
class HST_CampaignPersistentStateLoadClassifier
{
	static HST_CampaignPersistentStateLoadResult Classify(
		notnull LoadContext context)
	{
		HST_CampaignPersistentStateLoadResult result
			= new HST_CampaignPersistentStateLoadResult();

		string magic;
		int version;
		if (!context.ReadValue("magic", magic)
			|| !context.ReadValue("version", version))
		{
			result.SetInvalid(
				"native campaign envelope header could not be read");
			return result;
		}

		bool unknownMagic = !magic.IsEmpty()
			&& magic != HST_CampaignPersistentState.ENVELOPE_MAGIC
			&& magic != HST_CampaignPersistentState.LEGACY_ENVELOPE_MAGIC;
		if (unknownMagic
			|| version > HST_CampaignPersistentState.ENVELOPE_VERSION)
		{
			result.SetUnsupportedFuture(string.Format(
				"native campaign envelope is newer or unknown | magic %1 | version %2/%3",
				magic,
				version,
				HST_CampaignPersistentState.ENVELOPE_VERSION));
			return result;
		}

		bool currentIdentity
			= magic == HST_CampaignPersistentState.ENVELOPE_MAGIC
				&& version == HST_CampaignPersistentState.ENVELOPE_VERSION;
		bool legacyIdentity
			= magic == HST_CampaignPersistentState.LEGACY_ENVELOPE_MAGIC
				&& version
					== HST_CampaignPersistentState.LEGACY_ENVELOPE_VERSION;
		if (!currentIdentity && !legacyIdentity)
		{
			result.SetInvalid(
				"native campaign envelope identity rejected");
			return result;
		}

		bool snapshotPresent;
		string storedFingerprint;
		if (!context.ReadValue("snapshotPresent", snapshotPresent)
			|| !context.ReadValue("snapshotFingerprint", storedFingerprint)
			|| !snapshotPresent || storedFingerprint.IsEmpty())
		{
			result.SetInvalid(
				"native campaign envelope payload header rejected");
			return result;
		}

		if (legacyIdentity)
			return ClassifyLegacySnapshot(context, storedFingerprint, result);
		return ClassifyExactPayload(context, storedFingerprint, result);
	}

	protected static HST_CampaignPersistentStateLoadResult ClassifyLegacySnapshot(
		notnull LoadContext context,
		string storedFingerprint,
		HST_CampaignPersistentStateLoadResult result)
	{
		HST_CampaignSaveData snapshot = new HST_CampaignSaveData();
		if (!context.ReadValue("snapshot", snapshot))
		{
			result.SetInvalid(
				"legacy native campaign payload could not be read");
			return result;
		}
		if (snapshot.m_iSchemaVersion > HST_CampaignState.SCHEMA_VERSION)
		{
			result.SetUnsupportedFuture(string.Format(
				"native campaign payload schema %1 is newer than supported schema %2",
				snapshot.m_iSchemaVersion,
				HST_CampaignState.SCHEMA_VERSION));
			return result;
		}
		if (!HST_CampaignPersistentState.IsSnapshotSchemaSupported(snapshot))
		{
			result.SetInvalid("native campaign payload schema rejected");
			return result;
		}

		string normalizedFingerprint;
		if (!HST_CampaignPersistentState
			.TryValidateAndNormalizeSnapshotFingerprint(
				snapshot,
				HST_CampaignPersistentState.LEGACY_ENVELOPE_VERSION,
				storedFingerprint,
				normalizedFingerprint))
		{
			result.SetInvalid(
				"native campaign payload fingerprint rejected");
			return result;
		}

		result.SetValid(snapshot, normalizedFingerprint);
		return result;
	}

	protected static HST_CampaignPersistentStateLoadResult ClassifyExactPayload(
		notnull LoadContext context,
		string storedFingerprint,
		HST_CampaignPersistentStateLoadResult result)
	{
		string snapshotPayload;
		if (!context.ReadValue("snapshotPayload", snapshotPayload)
			|| snapshotPayload.IsEmpty())
		{
			result.SetInvalid(
				"native campaign exact payload could not be read");
			return result;
		}

		string exactFingerprint
			= HST_CampaignPersistentState.BuildPayloadFingerprint(
				snapshotPayload);
		if (exactFingerprint.IsEmpty()
			|| exactFingerprint != storedFingerprint)
		{
			result.SetInvalid(
				"native campaign exact payload fingerprint rejected");
			return result;
		}

		JsonLoadContext payloadContext = new JsonLoadContext();
		if (!payloadContext.LoadFromString(snapshotPayload))
		{
			result.SetInvalid(
				"native campaign exact payload JSON load failed");
			return result;
		}
		HST_CampaignSaveData snapshot = new HST_CampaignSaveData();
		if (!payloadContext.ReadValue("", snapshot))
		{
			result.SetInvalid(
				"native campaign exact payload DTO read failed");
			return result;
		}
		if (snapshot.m_iSchemaVersion > HST_CampaignState.SCHEMA_VERSION)
		{
			result.SetUnsupportedFuture(string.Format(
				"native campaign payload schema %1 is newer than supported schema %2",
				snapshot.m_iSchemaVersion,
				HST_CampaignState.SCHEMA_VERSION));
			return result;
		}
		if (!HST_CampaignPersistentState.IsSnapshotSchemaSupported(snapshot))
		{
			result.SetInvalid("native campaign payload schema rejected");
			return result;
		}

		// Preserve the validated exact payload identity. It is also the identity
		// written into the matching profile-journal generation after commit.
		result.SetValid(snapshot, storedFingerprint);
		return result;
	}
}

class HST_CampaignPersistentStateSerializer : ScriptedStateSerializer
{
	override static typename GetTargetType()
	{
		return HST_CampaignPersistentState;
	}

	override ESerializeResult Serialize(
		notnull Managed instance,
		notnull SaveContext context)
	{
		HST_CampaignPersistentState persistentState
			= HST_CampaignPersistentState.Cast(instance);
		if (!persistentState)
			return ESerializeResult.ERROR;

		HST_CampaignSaveData snapshot = persistentState.GetSnapshot();
		if (!HST_CampaignPersistentState.IsSnapshotSchemaSupported(snapshot))
		{
			// This state is configured as required campaign authority. A save while
			// bootstrap has not armed its snapshot must fail, never commit a valid
			// looking save point with the campaign row silently omitted.
			return ESerializeResult.ERROR;
		}
		string snapshotPayload;
		string fingerprint;
		if (!HST_CampaignPersistentState.TrySerializeSnapshot(
			snapshot,
			snapshotPayload,
			fingerprint))
			return ESerializeResult.ERROR;

		if (!context.WriteValue(
			"magic",
			HST_CampaignPersistentState.ENVELOPE_MAGIC)
			|| !context.WriteValue(
				"version",
				HST_CampaignPersistentState.ENVELOPE_VERSION)
			|| !context.WriteValue("snapshotPresent", true)
			|| !context.WriteValue("snapshotFingerprint", fingerprint)
			|| !context.WriteValue("snapshotPayload", snapshotPayload))
			return ESerializeResult.ERROR;

		return ESerializeResult.OK;
	}

	override bool Deserialize(
		notnull Managed instance,
		notnull LoadContext context)
	{
		HST_CampaignPersistentState persistentState
			= HST_CampaignPersistentState.Cast(instance);
		if (!persistentState)
			return false;

		HST_CampaignPersistentStateLoadResult loadResult
			= HST_CampaignPersistentStateLoadClassifier.Classify(context);
		persistentState.ApplyLoadedClassification(loadResult);
		// Classification belongs to the campaign source resolver. Returning false
		// would invoke serializer fault handling and could make typed invalid/future
		// evidence unavailable before the resolver can fail closed or recover.
		return true;
	}
}
