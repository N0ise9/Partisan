// Profile-artifact authority for the disposable exact enemy-garrison-rebuild
// fresh-process proof. Every artifact is bound to one build, schema, world,
// session owner, cut, and (where applicable) one-use stage nonce.
class HST_EnemyGarrisonRebuildExternalRestartProofService
{
	static const string OWNER_MAGIC
		= "partisan_exact_garrison_rebuild_restart_owner_v1";
	static const string GUARD_MAGIC
		= "partisan_exact_garrison_rebuild_restart_guard_v1";
	static const string CARRIER_MAGIC
		= "partisan_exact_garrison_rebuild_restart_carrier_v1";
	static const string RESULT_MAGIC
		= "partisan_exact_garrison_rebuild_restart_result_v1";
	static const string OWNER_PURPOSE
		= "exact_garrison_rebuild_external_restart";
	static const int AUTHORITY_VERSION = 1;
	static const string CUT_DELIVERY_PENDING = "delivery_pending";
	static const string CUT_PHYSICAL_LIVE_FOLD = "physical_live_fold";
	static const string STAGE_PREPARE = "prepare";
	static const string STAGE_RECOVER = "recover";
	static const string STAGE_REPLAY = "replay";
	static const string CANONICAL_WORLD = "Worlds/HST_Dev/HST_Dev.ent";
	static const string CANONICAL_WORLD_LOWER = "worlds/hst_dev/hst_dev.ent";
	static const int MAX_RUN_ID_CHARACTERS = 64;
	static const int NONCE_CHARACTERS = 32;
	static const int MIN_PHYSICAL_LIVE_SAMPLE_COUNT = 2;
	static const int MAX_PHYSICAL_LIVE_SAMPLE_COUNT = 64;
	static const float MIN_PHYSICAL_LIVE_MOVEMENT_METERS = 1.0;
	static const float MIN_PHYSICAL_DISTANCE_CLOSURE_METERS = 0.5;
	static const float PHYSICAL_EVIDENCE_EPSILON_METERS = 0.1;
	static const int EXPECTED_ACCEPTED_MEMBER_COUNT = 9;
	static const int EXPECTED_LIVING_MEMBER_COUNT = 8;
	static const float EXPECTED_PRE_MATERIALIZATION_ROUTE_PROGRESS_METERS = 225.0;
	static const float EXPECTED_PRE_MATERIALIZATION_ROUTE_TOTAL_METERS = 300.0;

	static string SanitizeRunId(string runId)
	{
		if (runId.IsEmpty() || runId.Length() > MAX_RUN_ID_CHARACTERS)
			return "";
		for (int index = 0; index < runId.Length(); index++)
		{
			string character = runId.Substring(index, 1);
			if (!"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"
				.Contains(character))
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
		if (cutName == CUT_DELIVERY_PENDING)
			return 0;
		if (cutName == CUT_PHYSICAL_LIVE_FOLD)
			return 1;
		return -1;
	}

	static string CutName(int cut)
	{
		if (cut == 0)
			return CUT_DELIVERY_PENDING;
		if (cut == 1)
			return CUT_PHYSICAL_LIVE_FOLD;
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

	static bool StageCreatesSavePoint(string stage)
	{
		return stage == STAGE_PREPARE || stage == STAGE_RECOVER;
	}

	static string BuildOwnerPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_ExactGarrisonRebuildRestart_" + safeRunId + ".owner.json";
	}

	static string BuildGuardPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_ExactGarrisonRebuildRestart_" + safeRunId + ".guard.json";
	}

	static string BuildCarrierPath(string runId)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty())
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_ExactGarrisonRebuildRestart_" + safeRunId + ".carrier.json";
	}

	static string BuildResultPath(string runId, string stage)
	{
		string safeRunId = SanitizeRunId(runId);
		if (safeRunId.IsEmpty() || !ValidateStage(stage))
			return "";
		return HST_ProfilePathService.DEBUG_DIRECTORY
			+ "/HST_ExactGarrisonRebuildRestart_" + safeRunId
			+ "." + stage + ".json";
	}

	static bool ValidateOwner(
		HST_EnemyGarrisonRebuildExternalRestartOwner owner,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedCut,
		string expectedWorld,
		out string evidence)
	{
		evidence = "external exact rebuild profile owner rejected";
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
			|| owner.m_iSettingsSchemaVersion != HST_RuntimeSettings.SCHEMA_VERSION
			|| !ValidateWorldIdentity(owner.m_sWorld, expectedWorld))
			return false;
		evidence = "external exact rebuild profile owner exact";
		return true;
	}

	static bool LoadOwner(
		string runId,
		out HST_EnemyGarrisonRebuildExternalRestartOwner owner,
		out string evidence)
	{
		owner = null;
		evidence = "external exact rebuild profile owner unavailable";
		string path = BuildOwnerPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		owner = new HST_EnemyGarrisonRebuildExternalRestartOwner();
		if (!context.ReadValue("", owner))
		{
			owner = null;
			return false;
		}
		evidence = "external exact rebuild profile owner loaded";
		return true;
	}

	static bool LoadAndValidateOwner(
		string sessionNonce,
		string runId,
		string cutName,
		string expectedWorld,
		out HST_EnemyGarrisonRebuildExternalRestartOwner owner,
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
		HST_EnemyGarrisonRebuildExternalRestartGuard guard,
		string expectedSessionNonce,
		string expectedStageNonce,
		string expectedRunId,
		string expectedCut,
		string expectedStage,
		string expectedWorld,
		out string evidence)
	{
		evidence = "external exact rebuild one-use stage lease rejected";
		int expectedOrdinal = ResolveStageOrdinal(expectedStage);
		if (!guard || !ValidateNonce(expectedSessionNonce)
			|| !ValidateNonce(expectedStageNonce)
			|| !ValidateRunId(expectedRunId) || ResolveCut(expectedCut) < 0
			|| expectedOrdinal < 0 || expectedWorld.IsEmpty())
			return false;
		if (guard.m_sMagic != GUARD_MAGIC
			|| guard.m_iVersion != AUTHORITY_VERSION
			|| guard.m_sSessionNonce != expectedSessionNonce
			|| guard.m_sStageNonce != expectedStageNonce
			|| guard.m_sRunId != expectedRunId
			|| SanitizeRunId(guard.m_sRunId) != guard.m_sRunId
			|| guard.m_sRequestedCut != expectedCut
			|| guard.m_sRequestedStage != expectedStage
			|| guard.m_iStageOrdinal != expectedOrdinal)
			return false;
		if (guard.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| guard.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| guard.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL
			|| guard.m_iCampaignSchemaVersion != HST_CampaignState.SCHEMA_VERSION
			|| guard.m_iSettingsSchemaVersion != HST_RuntimeSettings.SCHEMA_VERSION
			|| !ValidateWorldIdentity(guard.m_sWorld, expectedWorld)
			|| guard.m_bAllowCanonicalCampaignOverwrite
				!= StageCreatesSavePoint(expectedStage))
			return false;
		evidence = "external exact rebuild one-use stage lease exact";
		return true;
	}

	static bool SaveGuard(
		HST_EnemyGarrisonRebuildExternalRestartGuard guard,
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
			evidence = "external exact rebuild stage lease write failed";
			return false;
		}
		evidence = "external exact rebuild stage lease saved";
		return true;
	}

	static bool LoadGuard(
		string runId,
		out HST_EnemyGarrisonRebuildExternalRestartGuard guard,
		out string evidence)
	{
		guard = null;
		evidence = "external exact rebuild stage lease unavailable";
		string path = BuildGuardPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		guard = new HST_EnemyGarrisonRebuildExternalRestartGuard();
		if (!context.ReadValue("", guard))
		{
			guard = null;
			return false;
		}
		evidence = "external exact rebuild stage lease loaded";
		return true;
	}

	static bool LoadAndValidateGuard(
		string sessionNonce,
		string stageNonce,
		string runId,
		string cutName,
		string stage,
		string expectedWorld,
		out HST_EnemyGarrisonRebuildExternalRestartGuard guard,
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
		out HST_EnemyGarrisonRebuildExternalRestartGuard guard,
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
			evidence = "external exact rebuild stage lease disappeared";
			return false;
		}
		FileIO.DeleteFile(path);
		if (FileIO.FileExists(path))
		{
			evidence = "external exact rebuild stage lease could not be consumed";
			return false;
		}
		evidence = "external exact rebuild one-use stage lease consumed";
		return true;
	}

	protected static bool ValidateDeliveryPendingCarrierPhysicalShape(
		HST_EnemyGarrisonRebuildExternalRestartCarrier carrier,
		out string evidence)
	{
		evidence = "external exact rebuild delivery-pending physical shape rejected";
		if (!carrier)
			return false;
		bool countsExact = carrier.m_iExpectedPhysicalRootCount == 0
			&& carrier.m_iExpectedPhysicalAdapterHandleCount == 0
			&& carrier.m_iExpectedPhysicalRuntimeMemberCount == 0
			&& carrier.m_iLivePositionSampleCount == 0;
		bool preMaterializationRouteExact
			= Math.AbsFloat(carrier.m_fPreMaterializationRouteProgressMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& Math.AbsFloat(carrier.m_fPreMaterializationRouteTotalDistanceMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS;
		bool positionsExact = IsZeroVector(carrier.m_vInitialLivePosition)
			&& IsZeroVector(carrier.m_vFinalLivePosition)
			&& IsZeroVector(carrier.m_vFoldPosition);
		bool metricsExact
			= Math.AbsFloat(carrier.m_fInitialDistanceToTargetMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& Math.AbsFloat(carrier.m_fFinalDistanceToTargetMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& Math.AbsFloat(carrier.m_fLiveMovementMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& Math.AbsFloat(carrier.m_fDistanceClosedMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& Math.AbsFloat(carrier.m_fFoldRouteProgressMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS;
		bool flagsExact = !carrier.m_bPhysicalMovementExact
			&& !carrier.m_bPhysicalFoldExact;
		if (!countsExact || !preMaterializationRouteExact || !positionsExact
			|| !metricsExact || !flagsExact)
			return false;
		evidence = "external exact rebuild delivery-pending physical shape exact";
		return true;
	}

	protected static bool ValidatePhysicalLiveFoldCarrierShape(
		HST_EnemyGarrisonRebuildExternalRestartCarrier carrier,
		out string evidence)
	{
		evidence = "external exact rebuild physical-live-fold carrier rejected";
		if (!carrier || !carrier.m_Expectation)
			return false;
		int living = carrier.m_Expectation.m_iLivingMemberCount;
		bool countsExact = carrier.m_iExpectedPhysicalRootCount == 1
			&& carrier.m_iExpectedPhysicalRuntimeMemberCount == living
			&& carrier.m_iExpectedPhysicalAdapterHandleCount == living + 1;
		bool preMaterializationRouteExact = FloatsMatch(
				carrier.m_fPreMaterializationRouteProgressMeters,
				EXPECTED_PRE_MATERIALIZATION_ROUTE_PROGRESS_METERS)
			&& FloatsMatch(
				carrier.m_fPreMaterializationRouteTotalDistanceMeters,
				EXPECTED_PRE_MATERIALIZATION_ROUTE_TOTAL_METERS);
		bool sampleCountExact
			= carrier.m_iLivePositionSampleCount
				>= MIN_PHYSICAL_LIVE_SAMPLE_COUNT
			&& carrier.m_iLivePositionSampleCount
				<= MAX_PHYSICAL_LIVE_SAMPLE_COUNT;
		float measuredMovement = Distance2D(
			carrier.m_vInitialLivePosition,
			carrier.m_vFinalLivePosition);
		bool livePositionsExact
			= !IsZeroVector(carrier.m_vInitialLivePosition)
			&& !IsZeroVector(carrier.m_vFinalLivePosition)
			&& PositionsDiffer(
				carrier.m_vInitialLivePosition,
				carrier.m_vFinalLivePosition)
			&& carrier.m_fLiveMovementMeters
				>= MIN_PHYSICAL_LIVE_MOVEMENT_METERS
			&& FloatsMatch(carrier.m_fLiveMovementMeters, measuredMovement);
		float measuredClosure = carrier.m_fInitialDistanceToTargetMeters
			- carrier.m_fFinalDistanceToTargetMeters;
		bool closureExact = carrier.m_fInitialDistanceToTargetMeters
				> PHYSICAL_EVIDENCE_EPSILON_METERS
			&& carrier.m_fFinalDistanceToTargetMeters >= 0
			&& measuredClosure >= MIN_PHYSICAL_DISTANCE_CLOSURE_METERS
			&& carrier.m_fDistanceClosedMeters
				>= MIN_PHYSICAL_DISTANCE_CLOSURE_METERS
			&& FloatsMatch(carrier.m_fDistanceClosedMeters, measuredClosure);
		// The finalizer rewrites prepared authority from the normalized folded
		// snapshot. Equality binds the observed fold to the durable restart cut.
		bool foldExact = !IsZeroVector(carrier.m_vFoldPosition)
			&& PositionsMatch(
				carrier.m_vFoldPosition,
				carrier.m_vFinalLivePosition)
			&& PositionsMatch(
				carrier.m_vFoldPosition,
				carrier.m_vPreparedStrategicPosition)
			&& FloatsMatch(
				carrier.m_fFoldRouteProgressMeters,
				carrier.m_fPreparedRouteProgressMeters)
			&& Math.AbsFloat(carrier.m_fFoldRouteProgressMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& carrier.m_fPreparedRouteTotalDistanceMeters
				> PHYSICAL_EVIDENCE_EPSILON_METERS
			&& FloatsMatch(
				carrier.m_fPreparedRouteTotalDistanceMeters,
				carrier.m_fFinalDistanceToTargetMeters)
			&& carrier.m_fFinalDistanceToTargetMeters
				< carrier.m_fInitialDistanceToTargetMeters
			&& carrier.m_bPhysicalMovementExact
			&& carrier.m_bPhysicalFoldExact;
		if (!countsExact || !preMaterializationRouteExact || !sampleCountExact
			|| !livePositionsExact
			|| !closureExact || !foldExact)
		{
			evidence = string.Format(
				"external exact rebuild physical-live-fold counts/samples/positions/closure/fold rejected %1/%2/%3/%4/%5",
				countsExact,
				sampleCountExact,
				livePositionsExact,
				closureExact,
				foldExact);
			return false;
		}
		evidence = "external exact rebuild physical-live-fold carrier exact";
		return true;
	}

	static bool ValidateCarrier(
		HST_EnemyGarrisonRebuildExternalRestartCarrier carrier,
		string expectedSessionNonce,
		string expectedRunId,
		string expectedCut,
		string expectedWorld,
		out string evidence)
	{
		evidence = "external exact rebuild carrier rejected";
		int expectedCutValue = ResolveCut(expectedCut);
		if (!carrier || !ValidateNonce(expectedSessionNonce)
			|| !ValidateRunId(expectedRunId) || expectedCutValue < 0
			|| expectedWorld.IsEmpty() || !carrier.m_Expectation)
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
			|| carrier.m_iSettingsSchemaVersion
				!= HST_RuntimeSettings.SCHEMA_VERSION
			|| !ValidateWorldIdentity(carrier.m_sWorld, expectedWorld))
			return false;
		bool cutExact = carrier.m_sCutName == expectedCut
			&& carrier.m_iCut == expectedCutValue
			&& CutName(expectedCutValue) == expectedCut;
		bool routeShapeExact;
		if (expectedCut == CUT_DELIVERY_PENDING)
		{
			routeShapeExact = carrier.m_fPreparedRouteProgressMeters > 0
				&& carrier.m_fPreparedRouteProgressMeters
					< carrier.m_fPreparedRouteTotalDistanceMeters;
		}
		else if (expectedCut == CUT_PHYSICAL_LIVE_FOLD)
		{
			// Production dematerialization rebases the remaining route at the
			// sampled live position. The durable restart cut therefore owns zero
			// progress on a shorter, still-incomplete route.
			routeShapeExact
				= Math.AbsFloat(carrier.m_fPreparedRouteProgressMeters)
					<= PHYSICAL_EVIDENCE_EPSILON_METERS;
		}
		bool routeExact = carrier.m_iPreparedElapsedSecond > 0
			&& carrier.m_fPreparedRouteTotalDistanceMeters > 0
			&& routeShapeExact;
		bool positionExact = !IsZeroVector(carrier.m_vPreparedStrategicPosition);
		bool fingerprintExact
			= !carrier.m_sPreparedSemanticFingerprint.IsEmpty();
		if (!cutExact || !routeExact || !positionExact || !fingerprintExact)
			return false;

		HST_EnemyGarrisonRebuildExternalRestartExpectation expected
			= carrier.m_Expectation;
		bool operationIdentityExact = !expected.m_sOrderId.IsEmpty()
			&& !expected.m_sOperationId.IsEmpty()
			&& !expected.m_sManifestId.IsEmpty()
			&& !expected.m_sManifestHash.IsEmpty()
			&& !expected.m_sBatchId.IsEmpty();
		bool projectionIdentityExact = !expected.m_sGroupId.IsEmpty()
			&& !expected.m_sProjectionId.IsEmpty()
			&& !expected.m_sForceId.IsEmpty()
			&& !expected.m_sFactionKey.IsEmpty();
		bool endpointIdentityExact = !expected.m_sSourceZoneId.IsEmpty()
			&& !expected.m_sTargetZoneId.IsEmpty()
			&& expected.m_sSourceZoneId != expected.m_sTargetZoneId;
		bool identityExact = operationIdentityExact && projectionIdentityExact
			&& endpointIdentityExact;
		bool ownershipExact
			= !expected.m_sExpectedSourceOwnerFactionKey.IsEmpty()
			&& expected.m_sExpectedSourceOwnerFactionKey
				== expected.m_sFactionKey
			&& expected.m_iExpectedSourceOwnershipRevision > 0
			&& !expected.m_sExpectedTargetOwnerFactionKey.IsEmpty()
			&& expected.m_sExpectedTargetOwnerFactionKey
				== expected.m_sFactionKey
			&& expected.m_iExpectedTargetOwnershipRevision > 0;
		bool resourceBalanceExact = expected.m_iAttackCost == 0
			&& expected.m_iSupportCost
				== HST_OperationService.EXACT_ENEMY_GARRISON_REBUILD_SUPPORT_COST
			&& expected.m_iExpectedAttackPool >= 0
			&& expected.m_iExpectedSupportPool >= 0;
		bool resourceRevisionExact
			= expected.m_iExpectedPendingPoolRevision > 0
			&& expected.m_iExpectedPendingPoolOperationalMutationCount == 1
			&& !expected.m_sDebitMutationId.IsEmpty();
		bool resourceSettlementExact = expected.m_sDeliverySettlementKind
				== HST_OperationService
					.EXACT_ENEMY_GARRISON_REBUILD_DELIVERY_SETTLEMENT_KIND
			&& expected.m_sDeliverySettlementId
				== HST_OperationService.BuildSettlementId(
					expected.m_sOperationId,
					expected.m_sDeliverySettlementKind)
			&& expected.m_sRefundMutationId
				== "enemy_resource_refund_" + expected.m_sDeliverySettlementId;
		bool resourceExact = resourceBalanceExact && resourceRevisionExact
			&& resourceSettlementExact;
		bool rosterExact = expected.m_iAcceptedMemberCount
				== EXPECTED_ACCEPTED_MEMBER_COUNT
			&& expected.m_iLivingMemberCount == EXPECTED_LIVING_MEMBER_COUNT
			&& expected.m_iLivingMemberCount
				== expected.m_iAcceptedMemberCount - 1
			&& !expected.m_sLivingSlotFingerprint.IsEmpty()
			&& !expected.m_sConfirmedCasualtySlotId.IsEmpty()
			&& expected.m_sCasualtyTombstoneFingerprint.StartsWith(
				expected.m_sConfirmedCasualtySlotId + "|");
		bool garrisonExact = expected.m_iExpectedAggregateInfantry >= 0
			&& expected.m_iExpectedAuthoritativePendingInfantry
				== expected.m_iExpectedAggregateInfantry
					+ expected.m_iLivingMemberCount
			&& expected.m_iExpectedAuthoritativeDeliveredInfantry
				== expected.m_iExpectedAggregateInfantry
					+ expected.m_iLivingMemberCount;
		if (!identityExact || !ownershipExact || !resourceExact
			|| !rosterExact || !garrisonExact)
			return false;

		if (expectedCut == CUT_DELIVERY_PENDING)
			return ValidateDeliveryPendingCarrierPhysicalShape(
				carrier,
				evidence);
		if (expectedCut == CUT_PHYSICAL_LIVE_FOLD)
			return ValidatePhysicalLiveFoldCarrierShape(
				carrier,
				evidence);
		evidence = "external exact rebuild carrier cut family rejected";
		return false;
	}

	static bool SaveCarrier(
		HST_EnemyGarrisonRebuildExternalRestartCarrier carrier,
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
			evidence = "external exact rebuild carrier write failed";
			return false;
		}
		evidence = "external exact rebuild carrier saved";
		return true;
	}

	static bool LoadCarrier(
		string sessionNonce,
		string runId,
		string cutName,
		string expectedWorld,
		out HST_EnemyGarrisonRebuildExternalRestartCarrier carrier,
		out string evidence)
	{
		carrier = null;
		evidence = "external exact rebuild carrier unavailable";
		string path = BuildCarrierPath(runId);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		carrier = new HST_EnemyGarrisonRebuildExternalRestartCarrier();
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

	protected static bool ValidateDeliveryPendingResultPhysicalShape(
		HST_EnemyGarrisonRebuildExternalRestartResult result,
		out string evidence)
	{
		evidence = "external exact rebuild delivery-pending result shape rejected";
		if (!result)
			return false;
		bool countsExact = result.m_iPhysicalRootCount == 0
			&& result.m_iPhysicalAdapterHandleCount == 0
			&& result.m_iPhysicalRuntimeMemberCount == 0
			&& result.m_iLivePositionSampleCount == 0;
		bool positionsExact = IsZeroVector(result.m_vInitialLivePosition)
			&& IsZeroVector(result.m_vFinalLivePosition)
			&& IsZeroVector(result.m_vFoldPosition);
		bool metricsExact
			= Math.AbsFloat(result.m_fInitialDistanceToTargetMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& Math.AbsFloat(result.m_fFinalDistanceToTargetMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& Math.AbsFloat(result.m_fLiveMovementMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& Math.AbsFloat(result.m_fDistanceClosedMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& Math.AbsFloat(result.m_fFoldRouteProgressMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS;
		bool flagsExact = !result.m_bPhysicalBindingsExact
			&& !result.m_bPhysicalMovementExact
			&& !result.m_bPhysicalFoldExact;
		if (!countsExact || !positionsExact || !metricsExact || !flagsExact)
			return false;
		evidence = "external exact rebuild delivery-pending result shape exact";
		return true;
	}

	protected static bool ValidatePhysicalLiveFoldResultShape(
		HST_EnemyGarrisonRebuildExternalRestartResult result,
		string expectedStage,
		out string evidence)
	{
		evidence = "external exact rebuild physical-live-fold result shape rejected";
		if (!result || !ValidateStage(expectedStage))
			return false;
		bool countsExact = result.m_iPhysicalRootCount == 1
			&& result.m_iPhysicalRuntimeMemberCount > 0
			&& result.m_iPhysicalAdapterHandleCount
				== result.m_iPhysicalRuntimeMemberCount + 1;
		bool sampleCountExact
			= result.m_iLivePositionSampleCount
				>= MIN_PHYSICAL_LIVE_SAMPLE_COUNT
			&& result.m_iLivePositionSampleCount
				<= MAX_PHYSICAL_LIVE_SAMPLE_COUNT;
		float measuredMovement = Distance2D(
			result.m_vInitialLivePosition,
			result.m_vFinalLivePosition);
		bool livePositionsExact
			= !IsZeroVector(result.m_vInitialLivePosition)
			&& !IsZeroVector(result.m_vFinalLivePosition)
			&& PositionsDiffer(
				result.m_vInitialLivePosition,
				result.m_vFinalLivePosition)
			&& result.m_fLiveMovementMeters
				>= MIN_PHYSICAL_LIVE_MOVEMENT_METERS
			&& FloatsMatch(result.m_fLiveMovementMeters, measuredMovement);
		float measuredClosure = result.m_fInitialDistanceToTargetMeters
			- result.m_fFinalDistanceToTargetMeters;
		bool closureExact = result.m_fInitialDistanceToTargetMeters
				> PHYSICAL_EVIDENCE_EPSILON_METERS
			&& result.m_fFinalDistanceToTargetMeters >= 0
			&& measuredClosure >= MIN_PHYSICAL_DISTANCE_CLOSURE_METERS
			&& result.m_fDistanceClosedMeters
				>= MIN_PHYSICAL_DISTANCE_CLOSURE_METERS
			&& FloatsMatch(result.m_fDistanceClosedMeters, measuredClosure);
		bool foldExact = !IsZeroVector(result.m_vFoldPosition)
			&& PositionsMatch(
				result.m_vFoldPosition,
				result.m_vFinalLivePosition)
			&& Math.AbsFloat(result.m_fFoldRouteProgressMeters)
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& result.m_bPhysicalBindingsExact
			&& result.m_bPhysicalMovementExact
			&& result.m_bPhysicalFoldExact;
		bool stageProgressExact = false;
		if (expectedStage == STAGE_PREPARE)
		{
			stageProgressExact = FloatsMatch(
				result.m_fProgressBeforeMeters,
				result.m_fFoldRouteProgressMeters)
				&& FloatsMatch(
					result.m_fProgressAfterMeters,
					result.m_fFoldRouteProgressMeters);
		}
		else if (expectedStage == STAGE_RECOVER)
		{
			stageProgressExact = FloatsMatch(
				result.m_fProgressBeforeMeters,
				result.m_fFoldRouteProgressMeters)
				&& result.m_fProgressAfterMeters
					> result.m_fProgressBeforeMeters;
		}
		else
		{
			stageProgressExact = result.m_fProgressBeforeMeters
					+ PHYSICAL_EVIDENCE_EPSILON_METERS
					>= result.m_fFoldRouteProgressMeters
				&& FloatsMatch(
					result.m_fProgressAfterMeters,
					result.m_fProgressBeforeMeters);
		}
		if (!countsExact || !sampleCountExact || !livePositionsExact
			|| !closureExact || !foldExact || !stageProgressExact)
		{
			evidence = string.Format(
				"external exact rebuild physical-live-fold result counts/samples/positions/closure/fold/progress rejected %1/%2/%3/%4/%5/%6",
				countsExact,
				sampleCountExact,
				livePositionsExact,
				closureExact,
				foldExact,
				stageProgressExact);
			return false;
		}
		evidence = "external exact rebuild physical-live-fold result shape exact";
		return true;
	}

	protected static bool ValidateSuccessfulStageSemantics(
		HST_EnemyGarrisonRebuildExternalRestartResult result,
		string expectedStage,
		out string evidence)
	{
		evidence = "external exact rebuild result stage semantics rejected";
		if (!result || !ValidateStage(expectedStage))
			return false;
		if (expectedStage == STAGE_PREPARE)
		{
			if (result.m_bRestored || result.m_bStartupReconcileChanged
				|| result.m_bContinuationExact
				|| result.m_bSameStateSemanticNoOp
				|| result.m_bDeliveryReceiptExact
				|| result.m_bHeldGarrisonExact
				|| result.m_bAggregateNotDoubleCounted
				|| result.m_bResourceExactlyOnce
				|| result.m_sSourceSemanticFingerprint
					!= result.m_sFinalSemanticFingerprint)
				return false;
		}
		else if (expectedStage == STAGE_RECOVER)
		{
			if (!result.m_bRestored || !result.m_bContinuationExact
				|| result.m_bSameStateSemanticNoOp
				|| !result.m_bDeliveryReceiptExact
				|| !result.m_bHeldGarrisonExact
				|| !result.m_bAggregateNotDoubleCounted
				|| !result.m_bResourceExactlyOnce
				|| result.m_sSourceSemanticFingerprint
					== result.m_sFinalSemanticFingerprint
				|| result.m_fProgressAfterMeters
					<= result.m_fProgressBeforeMeters)
				return false;
		}
		else
		{
			if (!result.m_bRestored || result.m_bStartupReconcileChanged
				|| result.m_bContinuationExact
				|| !result.m_bSameStateSemanticNoOp
				|| !result.m_bDeliveryReceiptExact
				|| !result.m_bHeldGarrisonExact
				|| !result.m_bAggregateNotDoubleCounted
				|| !result.m_bResourceExactlyOnce
				|| result.m_sSourceSemanticFingerprint
					!= result.m_sFinalSemanticFingerprint
				|| !FloatsMatch(
					result.m_fProgressAfterMeters,
					result.m_fProgressBeforeMeters))
				return false;
		}
		evidence = "external exact rebuild result stage semantics exact";
		return true;
	}

	static bool ValidateResult(
		HST_EnemyGarrisonRebuildExternalRestartResult result,
		string expectedSessionNonce,
		string expectedStageNonce,
		string expectedRunId,
		string expectedCut,
		string expectedStage,
		string expectedWorld,
		out string evidence)
	{
		evidence = "external exact rebuild result rejected";
		int expectedCutValue = ResolveCut(expectedCut);
		if (!result || !ValidateNonce(expectedSessionNonce)
			|| !ValidateNonce(expectedStageNonce)
			|| !ValidateRunId(expectedRunId) || expectedCutValue < 0
			|| !ValidateStage(expectedStage) || expectedWorld.IsEmpty())
			return false;
		if (result.m_sMagic != RESULT_MAGIC
			|| result.m_sSessionNonce != expectedSessionNonce
			|| result.m_sStageNonce != expectedStageNonce
			|| result.m_sRunId != expectedRunId
			|| SanitizeRunId(result.m_sRunId) != result.m_sRunId
			|| result.m_sStage != expectedStage)
			return false;
		if (result.m_sBuildSha != HST_BuildInfo.BUILD_SHA
			|| result.m_sBuildUtc != HST_BuildInfo.BUILD_UTC
			|| result.m_sBuildLabel != HST_BuildInfo.BUILD_LABEL
			|| result.m_iCampaignSchemaVersion != HST_CampaignState.SCHEMA_VERSION
			|| result.m_iSettingsSchemaVersion
				!= HST_RuntimeSettings.SCHEMA_VERSION
			|| !ValidateWorldIdentity(result.m_sWorld, expectedWorld)
			|| result.m_sCutName != expectedCut
			|| result.m_iCut != expectedCutValue
			|| CutName(expectedCutValue) != expectedCut
			|| result.m_sEvidence.IsEmpty())
			return false;
		if (!result.m_bSuccess)
		{
			evidence = "external exact rebuild failure result exact";
			return true;
		}
		bool persistenceExact = result.m_bSourceExact
			&& result.m_bRuntimeClaimantsZero
			&& result.m_bPersistedReadBackExact
			&& result.m_bPreparedCutExact
			&& result.m_bCasualtyContinuityExact;
		bool fingerprintsExact
			= !result.m_sSourceSemanticFingerprint.IsEmpty()
			&& !result.m_sFinalSemanticFingerprint.IsEmpty();
		if (!persistenceExact || !fingerprintsExact)
			return false;

		string shapeEvidence;
		bool shapeExact = false;
		if (expectedCut == CUT_DELIVERY_PENDING)
		{
			shapeExact = ValidateDeliveryPendingResultPhysicalShape(
				result,
				shapeEvidence);
		}
		else if (expectedCut == CUT_PHYSICAL_LIVE_FOLD)
		{
			shapeExact = ValidatePhysicalLiveFoldResultShape(
				result,
				expectedStage,
				shapeEvidence);
		}
		if (!shapeExact)
		{
			evidence = shapeEvidence;
			return false;
		}
		return ValidateSuccessfulStageSemantics(
			result,
			expectedStage,
			evidence);
	}

	protected static bool ValidateSuccessfulResultAgainstCarrier(
		HST_EnemyGarrisonRebuildExternalRestartResult result,
		HST_EnemyGarrisonRebuildExternalRestartCarrier carrier,
		out string evidence)
	{
		evidence = "external exact rebuild result/carrier chain rejected";
		if (!result || !result.m_bSuccess || !carrier
			|| result.m_sSessionNonce != carrier.m_sSessionNonce
			|| result.m_sRunId != carrier.m_sRunId
			|| result.m_sCutName != carrier.m_sCutName
			|| result.m_iCut != carrier.m_iCut)
			return false;
		if (result.m_sCutName == CUT_DELIVERY_PENDING)
		{
			evidence = "external exact rebuild delivery-pending result/carrier chain exact";
			return true;
		}
		if (result.m_sCutName != CUT_PHYSICAL_LIVE_FOLD)
			return false;

		bool countsExact
			= result.m_iPhysicalRootCount
				== carrier.m_iExpectedPhysicalRootCount
			&& result.m_iPhysicalAdapterHandleCount
				== carrier.m_iExpectedPhysicalAdapterHandleCount
			&& result.m_iPhysicalRuntimeMemberCount
				== carrier.m_iExpectedPhysicalRuntimeMemberCount
			&& result.m_iLivePositionSampleCount
				== carrier.m_iLivePositionSampleCount;
		bool positionsExact = PositionsMatch(
				result.m_vInitialLivePosition,
				carrier.m_vInitialLivePosition)
			&& PositionsMatch(
				result.m_vFinalLivePosition,
				carrier.m_vFinalLivePosition)
			&& PositionsMatch(
				result.m_vFoldPosition,
				carrier.m_vFoldPosition);
		bool metricsExact = FloatsMatch(
				result.m_fInitialDistanceToTargetMeters,
				carrier.m_fInitialDistanceToTargetMeters)
			&& FloatsMatch(
				result.m_fFinalDistanceToTargetMeters,
				carrier.m_fFinalDistanceToTargetMeters)
			&& FloatsMatch(
				result.m_fLiveMovementMeters,
				carrier.m_fLiveMovementMeters)
			&& FloatsMatch(
				result.m_fDistanceClosedMeters,
				carrier.m_fDistanceClosedMeters)
			&& FloatsMatch(
				result.m_fFoldRouteProgressMeters,
				carrier.m_fFoldRouteProgressMeters);
		bool flagsExact = result.m_bPhysicalBindingsExact
			&& result.m_bPhysicalMovementExact
				== carrier.m_bPhysicalMovementExact
			&& result.m_bPhysicalFoldExact
				== carrier.m_bPhysicalFoldExact;
		bool stageChainExact = false;
		if (result.m_sStage == STAGE_PREPARE)
		{
			stageChainExact = result.m_sSourceSemanticFingerprint
					== carrier.m_sPreparedSemanticFingerprint
				&& result.m_sFinalSemanticFingerprint
					== carrier.m_sPreparedSemanticFingerprint
				&& FloatsMatch(
					result.m_fProgressBeforeMeters,
					carrier.m_fFoldRouteProgressMeters)
				&& FloatsMatch(
					result.m_fProgressAfterMeters,
					carrier.m_fFoldRouteProgressMeters);
		}
		else if (result.m_sStage == STAGE_RECOVER)
		{
			stageChainExact = result.m_sSourceSemanticFingerprint
					== carrier.m_sPreparedSemanticFingerprint
				&& FloatsMatch(
					result.m_fProgressBeforeMeters,
					carrier.m_fFoldRouteProgressMeters)
				&& FloatsMatch(
					result.m_fProgressAfterMeters,
					carrier.m_fPreparedRouteTotalDistanceMeters);
		}
		else if (result.m_sStage == STAGE_REPLAY)
		{
			stageChainExact = FloatsMatch(
					result.m_fProgressBeforeMeters,
					carrier.m_fPreparedRouteTotalDistanceMeters)
				&& FloatsMatch(
					result.m_fProgressAfterMeters,
					result.m_fProgressBeforeMeters);
		}
		if (!countsExact || !positionsExact || !metricsExact
			|| !flagsExact || !stageChainExact)
		{
			evidence = string.Format(
				"external exact rebuild result/carrier counts/positions/metrics/flags/stage rejected %1/%2/%3/%4/%5",
				countsExact,
				positionsExact,
				metricsExact,
				flagsExact,
				stageChainExact);
			return false;
		}
		evidence = "external exact rebuild physical-live-fold result/carrier chain exact";
		return true;
	}

	protected static bool ValidateResultCarrierAuthority(
		HST_EnemyGarrisonRebuildExternalRestartResult result,
		out string evidence)
	{
		evidence = "external exact rebuild result carrier authority rejected";
		if (!result)
			return false;
		if (!result.m_bSuccess || result.m_sCutName == CUT_DELIVERY_PENDING)
		{
			evidence = "external exact rebuild result carrier authority exact";
			return true;
		}
		HST_EnemyGarrisonRebuildExternalRestartCarrier carrier;
		string carrierEvidence;
		if (!LoadCarrier(
			result.m_sSessionNonce,
			result.m_sRunId,
			result.m_sCutName,
			result.m_sWorld,
			carrier,
			carrierEvidence))
		{
			evidence = "external exact rebuild result carrier unavailable: "
				+ carrierEvidence;
			return false;
		}
		return ValidateSuccessfulResultAgainstCarrier(
			result,
			carrier,
			evidence);
	}

	static bool SaveResult(
		HST_EnemyGarrisonRebuildExternalRestartResult result,
		out string evidence)
	{
		if (!result || !ValidateResult(
			result,
			result.m_sSessionNonce,
			result.m_sStageNonce,
			result.m_sRunId,
			result.m_sCutName,
			result.m_sStage,
			result.m_sWorld,
			evidence))
			return false;
		if (!ValidateResultCarrierAuthority(result, evidence))
			return false;
		string path = BuildResultPath(result.m_sRunId, result.m_sStage);
		if (path.IsEmpty())
			return false;
		HST_ProfilePathService.EnsureDebugDirectory();
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", result) || !context.SaveToFile(path))
		{
			evidence = "external exact rebuild result write failed";
			return false;
		}
		evidence = "external exact rebuild result saved";
		return true;
	}

	protected static bool ReadResultArtifact(
		string runId,
		string stage,
		out HST_EnemyGarrisonRebuildExternalRestartResult result)
	{
		result = null;
		string path = BuildResultPath(runId, stage);
		if (path.IsEmpty() || !FileIO.FileExists(path))
			return false;
		JsonLoadContext context = new JsonLoadContext();
		if (!context.LoadFromFile(path))
			return false;
		result = new HST_EnemyGarrisonRebuildExternalRestartResult();
		if (!context.ReadValue("", result))
		{
			result = null;
			return false;
		}
		return true;
	}

	static bool LoadResult(
		string sessionNonce,
		string stageNonce,
		string runId,
		string cutName,
		string stage,
		string expectedWorld,
		out HST_EnemyGarrisonRebuildExternalRestartResult result,
		out string evidence)
	{
		result = null;
		evidence = "external exact rebuild result unavailable";
		if (!ReadResultArtifact(runId, stage, result))
			return false;
		if (!ValidateResult(
			result,
			sessionNonce,
			stageNonce,
			runId,
			cutName,
			stage,
			expectedWorld,
			evidence))
			return false;
		return ValidateResultCarrierAuthority(result, evidence);
	}

	protected static bool LoadStoredSuccessfulResult(
		string sessionNonce,
		string runId,
		string cutName,
		string stage,
		string expectedWorld,
		HST_EnemyGarrisonRebuildExternalRestartCarrier carrier,
		out HST_EnemyGarrisonRebuildExternalRestartResult result,
		out string evidence)
	{
		result = null;
		evidence = "external exact rebuild prior result unavailable";
		if (!ReadResultArtifact(runId, stage, result)
			|| !result || !result.m_bSuccess
			|| !ValidateResult(
				result,
				sessionNonce,
				result.m_sStageNonce,
				runId,
				cutName,
				stage,
				expectedWorld,
				evidence))
			return false;
		return ValidateSuccessfulResultAgainstCarrier(
			result,
			carrier,
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
		evidence = "external exact rebuild disposable profile rejected";
		if (!ValidateNonce(sessionNonce) || !ValidateRunId(runId)
			|| ResolveCut(cutName) < 0 || !ValidateStage(stage)
			|| expectedWorld.IsEmpty())
			return false;
		HST_EnemyGarrisonRebuildExternalRestartOwner owner;
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
		string preparePath = BuildResultPath(runId, STAGE_PREPARE);
		string recoverPath = BuildResultPath(runId, STAGE_RECOVER);
		string replayPath = BuildResultPath(runId, STAGE_REPLAY);
		if (guardPath.IsEmpty() || carrierPath.IsEmpty()
			|| preparePath.IsEmpty() || recoverPath.IsEmpty()
			|| replayPath.IsEmpty() || !FileIO.FileExists(guardPath))
			return false;

		bool unrelatedData = FileIO.FileExists(
			HST_ProfilePathService.LEGACY_PROFILE_DIRECTORY)
			|| FileIO.FileExists(HST_ProfilePathService.LEGACY_ARCHIVE_DIRECTORY)
			|| FileIO.FileExists(
				HST_ProfilePathService.MIGRATION_STAGING_DIRECTORY)
			|| FileIO.FileExists(HST_ProfilePathService.VISUAL_SETTINGS_FILE)
			|| FileIO.FileExists(HST_ProfilePathService.LOADOUT_DIRECTORY);
		if (unrelatedData)
		{
			evidence = "external exact rebuild profile contains unrelated data";
			return false;
		}

		if (stage == STAGE_PREPARE)
		{
			bool persistentDataPresent = FileIO.FileExists(
				HST_ProfilePathService.CAMPAIGN_SAVE_FILE)
				|| FileIO.FileExists(
					HST_ProfilePathService.CAMPAIGN_RECOVERY_FILE);
			bool proofDataPresent = FileIO.FileExists(carrierPath)
				|| FileIO.FileExists(preparePath)
				|| FileIO.FileExists(recoverPath)
				|| FileIO.FileExists(replayPath);
			if (persistentDataPresent || proofDataPresent)
			{
				evidence = "external exact rebuild prepare requires a fresh disposable profile";
				return false;
			}
			evidence = "external exact rebuild prepare profile exact";
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
			evidence = "external exact rebuild restore stage has no valid campaign journal generation";
			return false;
		}

		HST_EnemyGarrisonRebuildExternalRestartCarrier carrier;
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
		HST_EnemyGarrisonRebuildExternalRestartResult prepareResult;
		string prepareEvidence;
		if (!LoadStoredSuccessfulResult(
			sessionNonce,
			runId,
			cutName,
			STAGE_PREPARE,
			expectedWorld,
			carrier,
			prepareResult,
			prepareEvidence))
		{
			evidence = evidence + " | prior prepare result rejected: "
				+ prepareEvidence;
			return false;
		}
		if (stage == STAGE_RECOVER)
		{
			if (FileIO.FileExists(recoverPath) || FileIO.FileExists(replayPath))
			{
				evidence = "external exact rebuild recover result storage is not fresh";
				return false;
			}
			evidence = "external exact rebuild recover profile exact";
			return true;
		}

		HST_EnemyGarrisonRebuildExternalRestartResult recoverResult;
		string recoverEvidence;
		if (!LoadStoredSuccessfulResult(
			sessionNonce,
			runId,
			cutName,
			STAGE_RECOVER,
			expectedWorld,
			carrier,
			recoverResult,
			recoverEvidence))
		{
			evidence = evidence + " | prior recover result rejected: "
				+ recoverEvidence;
			return false;
		}
		if (FileIO.FileExists(replayPath))
		{
			evidence = "external exact rebuild replay result storage is not fresh";
			return false;
		}
		evidence = "external exact rebuild replay profile exact";
		return true;
	}

	protected static bool FloatsMatch(float left, float right)
	{
		return Math.AbsFloat(left - right)
			<= PHYSICAL_EVIDENCE_EPSILON_METERS;
	}

	protected static bool IsZeroVector(vector value)
	{
		return Math.AbsFloat(value[0]) < 0.01
			&& Math.AbsFloat(value[1]) < 0.01
			&& Math.AbsFloat(value[2]) < 0.01;
	}

	protected static bool PositionsMatch(vector left, vector right)
	{
		return Math.AbsFloat(left[0] - right[0])
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& Math.AbsFloat(left[1] - right[1])
				<= PHYSICAL_EVIDENCE_EPSILON_METERS
			&& Math.AbsFloat(left[2] - right[2])
				<= PHYSICAL_EVIDENCE_EPSILON_METERS;
	}

	protected static bool PositionsDiffer(vector left, vector right)
	{
		return !PositionsMatch(left, right);
	}

	protected static float Distance2D(vector left, vector right)
	{
		float x = left[0] - right[0];
		float z = left[2] - right[2];
		return Math.Sqrt(x * x + z * z);
	}
}
