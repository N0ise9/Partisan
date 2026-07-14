#ifdef ENABLE_DIAG
// Diagnostic-only engine-process coverage for the deterministic exact enemy
// defensive-QRF report. Native entities, Full Campaign Debug, profile
// persistence, package/restart, networking, and soak remain separate gates.
class HST_EnemyQRFAutotestSuite : SCR_AutotestSuiteBase
{
}

[Test(suite: HST_EnemyQRFAutotestSuite)]
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
#endif
