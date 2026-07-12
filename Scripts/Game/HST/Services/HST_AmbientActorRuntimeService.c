// Session-only authority for one disposable ambient actor projection. Strategic
// civilian population remains authoritative elsewhere; these records only
// reserve a physical actor budget while their runtime transaction is alive.
class HST_AmbientActorRuntimeRecord
{
	string m_sRuntimeId;
	string m_sZoneId;
	string m_sKindId;
	string m_sStateId;
	string m_sReasonId;
	IEntity m_RootEntity;
	IEntity m_DriverEntity;
	AIGroup m_Group;
	int m_iCreatedAtSecond;
	int m_iStateChangedAtSecond;
	int m_iAdmittedAtSecond = -1;
	int m_iLastSampleAtSecond = -1;
	int m_iLastProgressAtSecond = -1;
	int m_iRetryAtSecond = -1;
	int m_iDespawnQueuedAtSecond = -1;
	vector m_vLastSamplePosition;
	// Immutable session identity for deterministic replacement and recovery.
	// Slot is unique within one zone/kind reservation set; seed is the actor's
	// original appearance/route seed rather than a mutable population count.
	int m_iProjectionSlot = -1;
	int m_iProjectionSeed;
	int m_iRecoveryCount;
	bool m_bAdmitted;
	bool m_bMovementObserved;
	bool m_bHasMovementSample;
}

// Pure lifecycle kernel. It deliberately does not discover, spawn, delete, or
// persist entities. World-facing callers own those side effects and advance a
// record only after the corresponding runtime acknowledgement succeeds.
class HST_AmbientActorRuntimeService
{
	static const string KIND_PEDESTRIAN = "pedestrian";
	static const string KIND_TRAFFIC = "traffic";

	static const string STATE_SPAWN_QUEUED = "SpawnQueued";
	static const string STATE_BEHAVIOR_INITIALIZING = "BehaviorInitializing";
	static const string STATE_WANDERING = "Wandering";
	static const string STATE_VEHICLE_SPAWNED = "VehicleSpawned";
	static const string STATE_DRIVER_SPAWNED = "DriverSpawned";
	static const string STATE_SEATING = "Seating";
	static const string STATE_DRIVER_CONFIRMED = "DriverConfirmed";
	static const string STATE_ENGINE_STARTING = "EngineStarting";
	static const string STATE_ENGINE_STARTED = "EngineStarted";
	static const string STATE_ROUTE_FOLLOWING = "RouteFollowing";
	static const string STATE_RECOVERING = "Recovering";
	static const string STATE_DESPAWN_QUEUED = "DespawnQueued";

	static const string REASON_CREATED = "created";
	static const string REASON_TRANSITION = "transition";
	static const string REASON_RECOVERY_REQUESTED = "recovery_requested";
	static const string REASON_RECOVERY_EXHAUSTED = "recovery_exhausted";
	static const string REASON_DESPAWN_REQUESTED = "despawn_requested";

	HST_AmbientActorRuntimeRecord CreateRecord(
		string runtimeId,
		string zoneId,
		string kindId,
		int nowSecond,
		int projectionSlot,
		int projectionSeed)
	{
		runtimeId = runtimeId.Trim();
		zoneId = zoneId.Trim();
		kindId = kindId.Trim();
		if (runtimeId.IsEmpty() || zoneId.IsEmpty() || !IsKnownKind(kindId)
			|| projectionSlot < 0)
			return null;

		HST_AmbientActorRuntimeRecord record
			= new HST_AmbientActorRuntimeRecord();
		record.m_sRuntimeId = runtimeId;
		record.m_sZoneId = zoneId;
		record.m_sKindId = kindId;
		record.m_sStateId = STATE_SPAWN_QUEUED;
		record.m_sReasonId = REASON_CREATED;
		record.m_iCreatedAtSecond = Math.Max(0, nowSecond);
		record.m_iStateChangedAtSecond = record.m_iCreatedAtSecond;
		record.m_iProjectionSlot = projectionSlot;
		record.m_iProjectionSeed = projectionSeed;
		return record;
	}

	// Legal edges are intentionally exact. A failed acknowledgement cannot skip
	// ahead to a budget-admitted state, and a rejected edge is read-only.
	bool TryTransition(
		HST_AmbientActorRuntimeRecord record,
		string nextStateId,
		string reasonId,
		int nowSecond)
	{
		if (!record || !IsKnownKind(record.m_sKindId))
			return false;

		nextStateId = nextStateId.Trim();
		if (!IsKnownState(nextStateId)
			|| !IsLegalTransition(record.m_sKindId, record.m_sStateId, nextStateId)
			|| nowSecond < record.m_iStateChangedAtSecond)
			return false;

		ApplyTransition(record, nextStateId, NormalizeReason(reasonId), nowSecond);
		return true;
	}

	// Returns true only when this sample proves threshold movement. The initial
	// sample and sub-threshold samples still advance sampling evidence without
	// resetting the no-progress clock.
	bool RecordMovementProgress(
		HST_AmbientActorRuntimeRecord record,
		vector samplePosition,
		int nowSecond,
		float minimumMovementMeters)
	{
		if (!IsBehaviorReady(record)
			|| nowSecond < record.m_iStateChangedAtSecond
			|| (record.m_iLastSampleAtSecond >= 0
				&& nowSecond < record.m_iLastSampleAtSecond))
			return false;

		bool moved;
		if (record.m_bHasMovementSample)
		{
			float threshold = Math.Max(0.01, minimumMovementMeters);
			moved = DistanceSq2D(record.m_vLastSamplePosition, samplePosition)
				>= threshold * threshold;
		}

		record.m_vLastSamplePosition = samplePosition;
		record.m_iLastSampleAtSecond = nowSecond;
		record.m_bHasMovementSample = true;
		if (moved)
		{
			record.m_bMovementObserved = true;
			record.m_iLastProgressAtSecond = nowSecond;
		}
		return moved;
	}

	bool IsBehaviorReady(HST_AmbientActorRuntimeRecord record)
	{
		if (!record || !record.m_bAdmitted)
			return false;
		if (record.m_sKindId == KIND_PEDESTRIAN)
			return record.m_sStateId == STATE_WANDERING;
		if (record.m_sKindId == KIND_TRAFFIC)
			return record.m_sStateId == STATE_ROUTE_FOLLOWING;
		return false;
	}

	bool IsBudgetReservation(HST_AmbientActorRuntimeRecord record)
	{
		return record
			&& !record.m_sRuntimeId.IsEmpty()
			&& !record.m_sZoneId.IsEmpty()
			&& IsKnownKind(record.m_sKindId)
			&& IsKnownState(record.m_sStateId)
			&& record.m_sStateId != STATE_DESPAWN_QUEUED;
	}

	bool ShouldRecover(
		HST_AmbientActorRuntimeRecord record,
		int nowSecond,
		int startupGraceSeconds,
		int stuckSeconds)
	{
		if (!IsBehaviorReady(record) || nowSecond < record.m_iStateChangedAtSecond)
			return false;

		int normalizedGrace = Math.Max(0, startupGraceSeconds);
		if (nowSecond - record.m_iStateChangedAtSecond < normalizedGrace)
			return false;

		int progressAtSecond = Math.Max(
			record.m_iStateChangedAtSecond,
			record.m_iLastProgressAtSecond);
		return nowSecond - progressAtSecond >= Math.Max(1, stuckSeconds);
	}

	// A recovery slot is consumed only after the ready->recovering edge succeeds.
	// Once all slots have been consumed, the same request deterministically queues
	// disposal instead of permitting an unbounded replan loop.
	bool BeginRecovery(
		HST_AmbientActorRuntimeRecord record,
		int nowSecond,
		int maxRecoveryAttempts,
		int retryBackoffSeconds,
		string reasonId)
	{
		if (!IsBehaviorReady(record) || nowSecond < record.m_iStateChangedAtSecond)
			return false;

		int normalizedMaximum = Math.Max(0, maxRecoveryAttempts);
		if (record.m_iRecoveryCount >= normalizedMaximum)
			return QueueDespawn(record, REASON_RECOVERY_EXHAUSTED, nowSecond);

		if (reasonId.Trim().IsEmpty())
			reasonId = REASON_RECOVERY_REQUESTED;
		if (!TryTransition(record, STATE_RECOVERING, reasonId, nowSecond))
			return false;

		record.m_iRecoveryCount++;
		record.m_iRetryAtSecond = nowSecond + Math.Max(0, retryBackoffSeconds);
		return true;
	}

	bool IsRetryDue(HST_AmbientActorRuntimeRecord record, int nowSecond)
	{
		return record
			&& record.m_sStateId == STATE_RECOVERING
			&& record.m_iRetryAtSecond >= 0
			&& nowSecond >= record.m_iRetryAtSecond;
	}

	// Explicit cancellation may occur before behavior admission (for example when
	// a town leaves the render bubble). TryTransition remains strict; this method
	// is the sole lifecycle escape hatch from an in-flight transaction.
	bool QueueDespawn(
		HST_AmbientActorRuntimeRecord record,
		string reasonId,
		int nowSecond)
	{
		if (!record || nowSecond < record.m_iStateChangedAtSecond)
			return false;
		if (record.m_sStateId == STATE_DESPAWN_QUEUED)
			return true;

		if (reasonId.Trim().IsEmpty())
			reasonId = REASON_DESPAWN_REQUESTED;
		if (record.m_sStateId == STATE_RECOVERING)
			return TryTransition(record, STATE_DESPAWN_QUEUED, reasonId, nowSecond);

		ApplyTransition(record, STATE_DESPAWN_QUEUED, reasonId, nowSecond);
		return true;
	}

	protected bool IsKnownKind(string kindId)
	{
		return kindId == KIND_PEDESTRIAN || kindId == KIND_TRAFFIC;
	}

	protected bool IsKnownState(string stateId)
	{
		return stateId == STATE_SPAWN_QUEUED
			|| stateId == STATE_BEHAVIOR_INITIALIZING
			|| stateId == STATE_WANDERING
			|| stateId == STATE_VEHICLE_SPAWNED
			|| stateId == STATE_DRIVER_SPAWNED
			|| stateId == STATE_SEATING
			|| stateId == STATE_DRIVER_CONFIRMED
			|| stateId == STATE_ENGINE_STARTING
			|| stateId == STATE_ENGINE_STARTED
			|| stateId == STATE_ROUTE_FOLLOWING
			|| stateId == STATE_RECOVERING
			|| stateId == STATE_DESPAWN_QUEUED;
	}

	protected bool IsLegalTransition(
		string kindId,
		string currentStateId,
		string nextStateId)
	{
		if (kindId == KIND_PEDESTRIAN)
		{
			if (currentStateId == STATE_SPAWN_QUEUED)
				return nextStateId == STATE_BEHAVIOR_INITIALIZING;
			if (currentStateId == STATE_BEHAVIOR_INITIALIZING)
				return nextStateId == STATE_WANDERING;
			if (currentStateId == STATE_WANDERING)
				return nextStateId == STATE_RECOVERING;
			if (currentStateId == STATE_RECOVERING)
				return nextStateId == STATE_WANDERING
					|| nextStateId == STATE_DESPAWN_QUEUED;
			return false;
		}

		if (kindId == KIND_TRAFFIC)
		{
			if (currentStateId == STATE_SPAWN_QUEUED)
				return nextStateId == STATE_VEHICLE_SPAWNED;
			if (currentStateId == STATE_VEHICLE_SPAWNED)
				return nextStateId == STATE_DRIVER_SPAWNED;
			if (currentStateId == STATE_DRIVER_SPAWNED)
				return nextStateId == STATE_SEATING;
			if (currentStateId == STATE_SEATING)
				return nextStateId == STATE_DRIVER_CONFIRMED;
			if (currentStateId == STATE_DRIVER_CONFIRMED)
				return nextStateId == STATE_ENGINE_STARTING;
			if (currentStateId == STATE_ENGINE_STARTING)
				return nextStateId == STATE_ENGINE_STARTED;
			if (currentStateId == STATE_ENGINE_STARTED)
				return nextStateId == STATE_ROUTE_FOLLOWING;
			if (currentStateId == STATE_ROUTE_FOLLOWING)
				return nextStateId == STATE_RECOVERING;
			if (currentStateId == STATE_RECOVERING)
				return nextStateId == STATE_ROUTE_FOLLOWING
					|| nextStateId == STATE_DESPAWN_QUEUED;
		}
		return false;
	}

	protected void ApplyTransition(
		HST_AmbientActorRuntimeRecord record,
		string nextStateId,
		string reasonId,
		int nowSecond)
	{
		record.m_sStateId = nextStateId;
		record.m_sReasonId = reasonId;
		record.m_iStateChangedAtSecond = nowSecond;

		bool readyState = nextStateId == STATE_WANDERING
			|| nextStateId == STATE_ROUTE_FOLLOWING;
		if (readyState)
		{
			if (!record.m_bAdmitted)
				record.m_iAdmittedAtSecond = nowSecond;
			record.m_bAdmitted = true;
			record.m_iLastProgressAtSecond = nowSecond;
			record.m_iLastSampleAtSecond = -1;
			record.m_iRetryAtSecond = -1;
			record.m_vLastSamplePosition = "0 0 0";
			record.m_bHasMovementSample = false;
		}
		else if (nextStateId == STATE_RECOVERING)
		{
			record.m_iRetryAtSecond = -1;
		}
		else if (nextStateId == STATE_DESPAWN_QUEUED)
		{
			record.m_bAdmitted = false;
			record.m_iRetryAtSecond = -1;
			record.m_iDespawnQueuedAtSecond = nowSecond;
		}
	}

	protected string NormalizeReason(string reasonId)
	{
		reasonId = reasonId.Trim();
		if (reasonId.IsEmpty())
			return REASON_TRANSITION;
		return reasonId;
	}

	protected float DistanceSq2D(vector first, vector second)
	{
		float dx = first[0] - second[0];
		float dz = first[2] - second[2];
		return dx * dx + dz * dz;
	}
}
