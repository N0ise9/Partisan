#ifdef ENABLE_DIAG
// Diagnostic-only engine-process coverage for the deterministic planning report.
// Full Campaign Debug, world integration, persistence, and networking remain
// separate validation gates.
class HST_EnemyPlanningCommitmentAutotestSuite : SCR_AutotestSuiteBase
{
	// This deterministic report is service-only. Keep the already loaded
	// packaged project active instead of entering the base-only world transition.
	override ResourceName GetWorldFile()
	{
		return "";
	}
}

// Retained as an unregistered compatibility implementation. Gate 1 executes
// the individually registered invariant cases below without double counting.
class HST_TEST_EnemyPlanningCommitmentAuthority : SCR_AutotestCaseBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		HST_EnemyPlanningProofService proof
			= new HST_EnemyPlanningProofService();
		HST_EnemyPlanningProofReport report = proof.BuildAuthorityReport();
		if (!report)
		{
			SetResultFailure("Enemy planning proof did not return a report");
			return true;
		}

		Print("Partisan planning autotest | "
			+ HST_BuildInfo.BuildRuntimeSummary());
		Print(report.BuildReport());
		Print(report.m_sBaselineCadenceEvidence);
		Print(report.m_sDecisionEvidence);
		Print(report.m_sCommitmentSelectionEvidence);
		Print(report.m_sFreezeRetryEvidence);
		Print(report.m_sRecoveryEvidence);
		Print(report.m_sPersistenceQuarantineEvidence);
		Print(report.m_sBootstrapThrottleEvidence);

		AssertTrue(
			report.m_bCommitmentAwareSelectionExact,
			"Commitment-aware selection failed: "
				+ report.m_sCommitmentSelectionEvidence);
		AssertTrue(
			report.m_bAllCommittedSkipExact,
			"All-committed skip failed: " + report.m_sDecisionEvidence);
		AssertTrue(
			report.m_bCommitmentRaceRejectionExact,
			"Commitment race rejection failed: "
				+ report.m_sDecisionEvidence);
		AssertTrue(
			report.AllExact(),
			"Full enemy planning authority proof failed: "
				+ report.BuildReport());
		SetResultSuccess();
		return true;
	}
}

// Individually registered cases expose every authority-report invariant. The
// compatibility implementation above is intentionally absent from the suite.
class HST_EnemyPlanningIndividualAutotestBase : SCR_AutotestCaseBase
{
	static const int PRE68_BASELINE = 0;
	static const int INDEPENDENT_CADENCE = 1;
	static const int BEGIN_REPLAY_CONFLICT = 2;
	static const int COMMITMENT_PERMUTATION = 3;
	static const int COMMITMENT_AWARE_SELECTION = 4;
	static const int ALL_COMMITTED_SKIP = 5;
	static const int COMMITMENT_RACE_REJECTION = 6;
	static const int FROZEN_DECISION = 7;
	static const int RETRY_ENVELOPE = 8;
	static const int PREPARED_PRESSURE_CRASH_WINDOW = 9;
	static const int PREPARED_ORDER_ADOPTION = 10;
	static const int RETRY_TAMPER_QUARANTINE = 11;
	static const int ZERO_TARGET_SKIP = 12;
	static const int COMMITTED_ROUNDTRIP = 13;
	static const int CURRENT_QUARANTINE = 14;
	static const int FRESH_BOOTSTRAP = 15;
	static const int UNAVAILABLE_LOG_THROTTLE = 16;

	protected bool ExecuteScenario(int scenario)
	{
		HST_EnemyPlanningProofService proof
			= new HST_EnemyPlanningProofService();
		HST_EnemyPlanningProofReport report = proof.BuildAuthorityReport();
		if (!report)
		{
			SetResultFailure("Enemy planning proof did not return a report");
			return true;
		}

		bool exact;
		string invariant;
		string evidence;
		switch (scenario)
		{
			case PRE68_BASELINE:
				exact = report.m_bPre68BaselineExact;
				invariant = "pre-68 baseline";
				evidence = report.m_sBaselineCadenceEvidence;
				break;
			case INDEPENDENT_CADENCE:
				exact = report.m_bIndependentCadenceExact;
				invariant = "independent cadence";
				evidence = report.m_sBaselineCadenceEvidence;
				break;
			case BEGIN_REPLAY_CONFLICT:
				exact = report.m_bBeginReplayConflictExact;
				invariant = "begin replay conflict";
				evidence = report.m_sDecisionEvidence;
				break;
			case COMMITMENT_PERMUTATION:
				exact = report.m_bCommitmentPermutationExact;
				invariant = "commitment permutation";
				evidence = report.m_sDecisionEvidence;
				break;
			case COMMITMENT_AWARE_SELECTION:
				exact = report.m_bCommitmentAwareSelectionExact;
				invariant = "commitment-aware selection";
				evidence = report.m_sCommitmentSelectionEvidence;
				break;
			case ALL_COMMITTED_SKIP:
				exact = report.m_bAllCommittedSkipExact;
				invariant = "all-committed skip";
				evidence = report.m_sDecisionEvidence;
				break;
			case COMMITMENT_RACE_REJECTION:
				exact = report.m_bCommitmentRaceRejectionExact;
				invariant = "commitment race rejection";
				evidence = report.m_sDecisionEvidence;
				break;
			case FROZEN_DECISION:
				exact = report.m_bFrozenDecisionExact;
				invariant = "frozen decision";
				evidence = report.m_sFreezeRetryEvidence;
				break;
			case RETRY_ENVELOPE:
				exact = report.m_bRetryEnvelopeExact;
				invariant = "retry envelope";
				evidence = report.m_sFreezeRetryEvidence;
				break;
			case PREPARED_PRESSURE_CRASH_WINDOW:
				exact = report.m_bPreparedPressureCrashWindowExact;
				invariant = "prepared pressure crash window";
				evidence = report.m_sRecoveryEvidence;
				break;
			case PREPARED_ORDER_ADOPTION:
				exact = report.m_bPreparedOrderAdoptionExact;
				invariant = "prepared order adoption";
				evidence = report.m_sRecoveryEvidence;
				break;
			case RETRY_TAMPER_QUARANTINE:
				exact = report.m_bRetryTamperQuarantineExact;
				invariant = "retry tamper quarantine";
				evidence = report.m_sRecoveryEvidence;
				break;
			case ZERO_TARGET_SKIP:
				exact = report.m_bZeroTargetSkipExact;
				invariant = "zero-target skip";
				evidence = report.m_sPersistenceQuarantineEvidence;
				break;
			case COMMITTED_ROUNDTRIP:
				exact = report.m_bCommittedRoundtripExact;
				invariant = "committed roundtrip";
				evidence = report.m_sPersistenceQuarantineEvidence;
				break;
			case CURRENT_QUARANTINE:
				exact = report.m_bCurrentQuarantineExact;
				invariant = "current quarantine";
				evidence = report.m_sPersistenceQuarantineEvidence;
				break;
			case FRESH_BOOTSTRAP:
				exact = report.m_bFreshBootstrapExact;
				invariant = "fresh bootstrap";
				evidence = report.m_sBootstrapThrottleEvidence;
				break;
			case UNAVAILABLE_LOG_THROTTLE:
				exact = report.m_bUnavailableLogThrottleExact;
				invariant = "unavailable-log throttle";
				evidence = report.m_sBootstrapThrottleEvidence;
				break;
			default:
				SetResultFailure("Unknown enemy planning scenario");
				return true;
		}

		Print("Partisan enemy planning scenario | " + invariant
			+ " | " + HST_BuildInfo.BuildRuntimeSummary());
		Print(report.BuildReport());
		Print(evidence);
		AssertTrue(
			exact,
			"Enemy planning " + invariant + " proof failed: " + evidence);
		SetResultSuccess();
		return true;
	}
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_Pre68Baseline
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(PRE68_BASELINE); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_IndependentCadence
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(INDEPENDENT_CADENCE); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_BeginReplayConflict
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(BEGIN_REPLAY_CONFLICT); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_CommitmentPermutation
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(COMMITMENT_PERMUTATION); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_CommitmentAwareSelection
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(COMMITMENT_AWARE_SELECTION); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_AllCommittedSkip
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(ALL_COMMITTED_SKIP); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_CommitmentRaceRejection
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(COMMITMENT_RACE_REJECTION); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_FrozenDecision
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(FROZEN_DECISION); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_RetryEnvelope
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(RETRY_ENVELOPE); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_PreparedPressureCrashWindow
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(PREPARED_PRESSURE_CRASH_WINDOW); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_PreparedOrderAdoption
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(PREPARED_ORDER_ADOPTION); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_RetryTamperQuarantine
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(RETRY_TAMPER_QUARANTINE); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_ZeroTargetSkip
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(ZERO_TARGET_SKIP); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_CommittedRoundtrip
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(COMMITTED_ROUNDTRIP); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_CurrentQuarantine
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(CURRENT_QUARANTINE); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_FreshBootstrap
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(FRESH_BOOTSTRAP); }
}

[Test(suite: HST_EnemyPlanningCommitmentAutotestSuite)]
class HST_TEST_EnemyPlanning_UnavailableLogThrottle
	: HST_EnemyPlanningIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(UNAVAILABLE_LOG_THROTTLE); }
}
#endif
