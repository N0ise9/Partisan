#ifdef ENABLE_DIAG
// Diagnostic-only engine-process coverage for the deterministic exact enemy
// defensive-QRF report. Native entities, Full Campaign Debug, profile
// persistence, package/restart, networking, and soak remain separate gates.
class HST_EnemyQRFAutotestSuite : SCR_AutotestSuiteBase
{
	// This deterministic report is service-only. Keep the packaged project
	// loaded so the stock base-only scenario transition cannot drop the test.
	override ResourceName GetWorldFile()
	{
		return "";
	}
}

// Retained as an unregistered compatibility implementation. Gate 1 executes
// the individually registered invariant cases below without double counting.
class HST_TEST_EnemyQRFAuthority : SCR_AutotestCaseBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		HST_EnemyQRFOperationProofService proof
			= new HST_EnemyQRFOperationProofService();
		HST_EnemyQRFOperationProofReport report = proof.Run();
		if (!report)
		{
			SetResultFailure("Exact enemy defensive-QRF proof did not return a report");
			return true;
		}

		bool allExact = AllExact(report);
		Print("Partisan enemy defensive-QRF autotest | "
			+ HST_BuildInfo.BuildRuntimeSummary());
		Print(report.m_sAdmissionEvidence);
		Print(report.m_sLegacyIsolationEvidence);
		Print(report.m_sProjectionEvidence);
		Print(report.m_sSettlementEvidence);
		Print(report.m_sRestoreEvidence);
		Print(report.m_sRejectionEvidence);
		Print(string.Format(
			"Exact enemy defensive-QRF aggregate | AllExact %1",
			allExact));

		AssertTrue(
			report.m_bAdmissionExact,
			"Enemy defensive-QRF admission proof failed: "
				+ report.m_sAdmissionEvidence);
		AssertTrue(
			report.m_bLegacyIsolationExact,
			"Enemy defensive-QRF legacy-isolation proof failed: "
				+ report.m_sLegacyIsolationEvidence);
		AssertTrue(
			report.m_bProjectionExact,
			"Enemy defensive-QRF projection proof failed: "
				+ report.m_sProjectionEvidence);
		AssertTrue(
			report.m_bSettlementExact,
			"Enemy defensive-QRF settlement proof failed: "
				+ report.m_sSettlementEvidence);
		AssertTrue(
			report.m_bRestoreExact,
			"Enemy defensive-QRF restore proof failed: "
				+ report.m_sRestoreEvidence);
		AssertTrue(
			report.m_bRejectionExact,
			"Enemy defensive-QRF rejection proof failed: "
				+ report.m_sRejectionEvidence);
		AssertTrue(
			allExact,
			"Full exact enemy defensive-QRF proof failed");
		SetResultSuccess();
		return true;
	}

	protected bool AllExact(HST_EnemyQRFOperationProofReport report)
	{
		return report
			&& report.m_bAdmissionExact
			&& report.m_bLegacyIsolationExact
			&& report.m_bProjectionExact
			&& report.m_bSettlementExact
			&& report.m_bRestoreExact
			&& report.m_bRejectionExact;
	}
}

// Individually registered cases keep each report invariant selectable. Every
// case executes the same proof service once, then owns one JUnit result; the
// compatibility implementation above is intentionally absent from the suite.
class HST_EnemyQRFIndividualAutotestBase : SCR_AutotestCaseBase
{
	static const int ADMISSION = 0;
	static const int LEGACY_ISOLATION = 1;
	static const int PROJECTION = 2;
	static const int SETTLEMENT = 3;
	static const int RESTORE = 4;
	static const int REJECTION = 5;

	protected bool ExecuteScenario(int scenario)
	{
		HST_EnemyQRFOperationProofService proof
			= new HST_EnemyQRFOperationProofService();
		HST_EnemyQRFOperationProofReport report = proof.Run();
		if (!report)
		{
			SetResultFailure(
				"Exact enemy defensive-QRF proof did not return a report");
			return true;
		}

		bool exact;
		string invariant;
		string evidence;
		switch (scenario)
		{
			case ADMISSION:
				exact = report.m_bAdmissionExact;
				invariant = "admission";
				evidence = report.m_sAdmissionEvidence;
				break;
			case LEGACY_ISOLATION:
				exact = report.m_bLegacyIsolationExact;
				invariant = "legacy isolation";
				evidence = report.m_sLegacyIsolationEvidence;
				break;
			case PROJECTION:
				exact = report.m_bProjectionExact;
				invariant = "projection";
				evidence = report.m_sProjectionEvidence;
				break;
			case SETTLEMENT:
				exact = report.m_bSettlementExact;
				invariant = "settlement";
				evidence = report.m_sSettlementEvidence;
				break;
			case RESTORE:
				exact = report.m_bRestoreExact;
				invariant = "restore";
				evidence = report.m_sRestoreEvidence;
				break;
			case REJECTION:
				exact = report.m_bRejectionExact;
				invariant = "rejection";
				evidence = report.m_sRejectionEvidence;
				break;
			default:
				SetResultFailure("Unknown enemy defensive-QRF scenario");
				return true;
		}

		Print("Partisan enemy defensive-QRF scenario | " + invariant
			+ " | " + HST_BuildInfo.BuildRuntimeSummary());
		Print(evidence);
		AssertTrue(
			exact,
			"Enemy defensive-QRF " + invariant + " proof failed: "
				+ evidence);
		SetResultSuccess();
		return true;
	}
}

[Test(suite: HST_EnemyQRFAutotestSuite)]
class HST_TEST_EnemyQRF_Admission
	: HST_EnemyQRFIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(ADMISSION);
	}
}

[Test(suite: HST_EnemyQRFAutotestSuite)]
class HST_TEST_EnemyQRF_LegacyIsolation
	: HST_EnemyQRFIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(LEGACY_ISOLATION);
	}
}

[Test(suite: HST_EnemyQRFAutotestSuite)]
class HST_TEST_EnemyQRF_Projection
	: HST_EnemyQRFIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(PROJECTION);
	}
}

[Test(suite: HST_EnemyQRFAutotestSuite)]
class HST_TEST_EnemyQRF_Settlement
	: HST_EnemyQRFIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(SETTLEMENT);
	}
}

[Test(suite: HST_EnemyQRFAutotestSuite)]
class HST_TEST_EnemyQRF_Restore
	: HST_EnemyQRFIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(RESTORE);
	}
}

[Test(suite: HST_EnemyQRFAutotestSuite)]
class HST_TEST_EnemyQRF_Rejection
	: HST_EnemyQRFIndividualAutotestBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		return ExecuteScenario(REJECTION);
	}
}
#endif
