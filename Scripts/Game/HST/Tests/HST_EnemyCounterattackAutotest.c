#ifdef ENABLE_DIAG
// Diagnostic-only engine-process coverage for the deterministic exact enemy
// counterattack report. Full Campaign Debug, native projection, networking,
// profile persistence, and package/restart validation remain separate gates.
class HST_EnemyCounterattackAutotestSuite : SCR_AutotestSuiteBase
{
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattackAuthority : SCR_AutotestCaseBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		HST_EnemyCounterattackOperationProofService proof
			= new HST_EnemyCounterattackOperationProofService();
		HST_EnemyCounterattackOperationProofReport report = proof.Run();
		if (!report)
		{
			SetResultFailure("Exact enemy counterattack proof did not return a report");
			return true;
		}

		Print("Partisan counterattack autotest | "
			+ HST_BuildInfo.BuildRuntimeSummary());
		Print(report.BuildReport());
		Print(report.m_sPlanningEvidence);
		Print(report.m_sAdmissionEvidence);
		Print(report.m_sTravelEvidence);
		Print(report.m_sCombatEvidence);
		Print(report.m_sPhysicalHandoffEvidence);
		Print(report.m_sOwnershipEvidence);
		Print(report.m_sSettlementEvidence);
		Print(report.m_sSupportSettlementEvidence);
		Print(report.m_sRestoreEvidence);
		Print(report.m_sResourceAuthorityEvidence);
		Print(report.m_sAmbiguityEvidence);
		Print(report.m_sQuarantineEvidence);
		Print(report.m_sRetentionEvidence);

		AssertTrue(
			report.m_bFrozenPlanningExact,
			"Counterattack planning proof failed: " + report.m_sPlanningEvidence);
		AssertTrue(
			report.m_bAdmissionExact,
			"Counterattack admission proof failed: " + report.m_sAdmissionEvidence);
		AssertTrue(
			report.m_bVirtualTravelExact,
			"Counterattack travel proof failed: " + report.m_sTravelEvidence);
		AssertTrue(
			report.m_bVirtualCombatExact,
			"Counterattack combat proof failed: " + report.m_sCombatEvidence);
		AssertTrue(
			report.m_bPhysicalHandoffExact,
			"Counterattack physical handoff proof failed: "
				+ report.m_sPhysicalHandoffEvidence);
		AssertTrue(
			report.m_bOwnershipRetryExact,
			"Counterattack ownership proof failed: " + report.m_sOwnershipEvidence);
		AssertTrue(
			report.m_bSettlementReplayExact,
			"Counterattack settlement proof failed: " + report.m_sSettlementEvidence);
		AssertTrue(
			report.m_bSupportSettlementExact,
			"Counterattack support settlement proof failed: "
				+ report.m_sSupportSettlementEvidence);
		AssertTrue(
			report.m_bRestoreLifecycleExact,
			"Counterattack restore proof failed: " + report.m_sRestoreEvidence);
		AssertTrue(
			report.m_bResourceAuthorityQuarantineExact,
			"Counterattack resource-authority proof failed: "
				+ report.m_sResourceAuthorityEvidence);
		AssertTrue(
			report.m_bAmbiguityHoldExact,
			"Counterattack ambiguity-hold proof failed: " + report.m_sAmbiguityEvidence);
		AssertTrue(
			report.m_bSchema69QuarantineExact,
			"Counterattack quarantine proof failed: " + report.m_sQuarantineEvidence);
		AssertTrue(
			report.m_bQuarantineRetentionExact,
			"Counterattack quarantine-retention proof failed: "
				+ report.m_sRetentionEvidence);
		AssertTrue(
			report.AllExact(),
			"Full exact enemy counterattack proof failed: " + report.BuildReport());
		SetResultSuccess();
		return true;
	}
}
#endif
