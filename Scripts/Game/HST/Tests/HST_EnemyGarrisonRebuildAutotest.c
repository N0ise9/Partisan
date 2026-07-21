#ifdef ENABLE_DIAG
// Diagnostic-only engine-process coverage for the deterministic Schema-70
// exact enemy garrison-rebuild report. Native entities, Full Campaign Debug,
// profile persistence, package/restart, networking, and soak remain separate.
class HST_EnemyGarrisonRebuildAutotestSuite : SCR_AutotestSuiteBase
{
	// This deterministic report is service-only. The stock autotest scenario
	// transition carries only the base add-on list, so opening its default world
	// would unload this packaged test type before JUnit can be written.
	override ResourceName GetWorldFile()
	{
		return "";
	}
}

// Retained as an unregistered compatibility implementation. Gate 1 executes
// the individually registered invariant cases below without double counting.
class HST_TEST_EnemyGarrisonRebuildAuthority : SCR_AutotestCaseBase
{
	[Step(EStage.Main)]
	bool Execute()
	{
		HST_EnemyGarrisonRebuildOperationProofService proof
			= new HST_EnemyGarrisonRebuildOperationProofService();
		HST_EnemyGarrisonRebuildOperationProofReport report = proof.Run();
		if (!report)
		{
			SetResultFailure(
				"Exact enemy garrison-rebuild proof did not return a report");
			return true;
		}

		Print("Partisan enemy garrison-rebuild autotest | "
			+ HST_BuildInfo.BuildRuntimeSummary());
		Print(report.BuildReport());
		Print(report.m_sAdmissionEvidence);
		Print(report.m_sDeliveryEvidence);
		Print(report.m_sCasualtyEvidence);
		Print(report.m_sRestoreEvidence);
		Print(report.m_sOwnershipEvidence);
		Print(report.m_sAdmissionRollbackEvidence);
		Print(report.m_sPrearrivalRefundEvidence);
		Print(report.m_sSettlementCrashEvidence);
		Print(report.m_sHistoricalEvidence);
		Print(report.m_sQuarantineEvidence);
		Print(report.m_sOrphanRuntimeEvidence);
		Print(report.m_sRetentionEvidence);
		Print(report.m_sSelectedOwnershipABAEvidence);

		AssertTrue(
			report.m_bAdmissionCapacityExact,
			"Garrison-rebuild admission/capacity proof failed: "
				+ report.m_sAdmissionEvidence);
		AssertTrue(
			report.m_bDeliveryHeldExact,
			"Garrison-rebuild delivery/held proof failed: "
				+ report.m_sDeliveryEvidence);
		AssertTrue(
			report.m_bCasualtyContinuityExact,
			"Garrison-rebuild casualty-continuity proof failed: "
				+ report.m_sCasualtyEvidence);
		AssertTrue(
			report.m_bRestoreExact,
			"Garrison-rebuild restore proof failed: "
				+ report.m_sRestoreEvidence);
		AssertTrue(
			report.m_bOwnershipTerminalExact,
			"Garrison-rebuild ownership-terminal proof failed: "
				+ report.m_sOwnershipEvidence);
		AssertTrue(
			report.m_bAdmissionRollbackExact,
			"Garrison-rebuild admission rollback proof failed: "
				+ report.m_sAdmissionRollbackEvidence);
		AssertTrue(
			report.m_bPrearrivalRefundExact,
			"Garrison-rebuild prearrival refund proof failed: "
				+ report.m_sPrearrivalRefundEvidence);
		AssertTrue(
			report.m_bSettlementCrashResumeExact,
			"Garrison-rebuild settlement crash-resume proof failed: "
				+ report.m_sSettlementCrashEvidence);
		AssertTrue(
			report.m_bHistoricalIsolationExact,
			"Garrison-rebuild historical isolation proof failed: "
				+ report.m_sHistoricalEvidence);
		AssertTrue(
			report.m_bSchema70QuarantineExact,
			"Garrison-rebuild Schema-70 quarantine proof failed: "
				+ report.m_sQuarantineEvidence);
		AssertTrue(
			report.m_bOrphanRuntimeQuarantineExact,
			"Garrison-rebuild orphan runtime quarantine proof failed: "
				+ report.m_sOrphanRuntimeEvidence);
		AssertTrue(
			report.m_bQuarantineRetentionExact,
			"Garrison-rebuild quarantine-retention proof failed: "
				+ report.m_sRetentionEvidence);
		AssertTrue(
			report.m_bSelectedOwnershipABAExact,
			"Garrison-rebuild selected ownership ABA proof failed: "
				+ report.m_sSelectedOwnershipABAEvidence);
		AssertTrue(
			report.AllExact(),
			"Full exact enemy garrison-rebuild proof failed: "
				+ report.BuildReport());
		SetResultSuccess();
		return true;
	}
}

// Individually registered cases expose each exact report invariant. The
// compatibility implementation above is intentionally absent from the suite.
class HST_EnemyGarrisonRebuildIndividualAutotestBase
	: SCR_AutotestCaseBase
{
	static const int ADMISSION_CAPACITY = 0;
	static const int DELIVERY_HELD = 1;
	static const int CASUALTY_CONTINUITY = 2;
	static const int RESTORE = 3;
	static const int OWNERSHIP_TERMINAL = 4;
	static const int ADMISSION_ROLLBACK = 5;
	static const int PREARRIVAL_REFUND = 6;
	static const int SETTLEMENT_CRASH_RESUME = 7;
	static const int HISTORICAL_ISOLATION = 8;
	static const int SCHEMA70_QUARANTINE = 9;
	static const int ORPHAN_RUNTIME_QUARANTINE = 10;
	static const int QUARANTINE_RETENTION = 11;
	static const int SELECTED_OWNERSHIP_ABA = 12;

	protected bool ExecuteScenario(int scenario)
	{
		HST_EnemyGarrisonRebuildOperationProofService proof
			= new HST_EnemyGarrisonRebuildOperationProofService();
		HST_EnemyGarrisonRebuildOperationProofReport report = proof.Run();
		if (!report)
		{
			SetResultFailure(
				"Exact enemy garrison-rebuild proof did not return a report");
			return true;
		}

		bool exact;
		string invariant;
		string evidence;
		switch (scenario)
		{
			case ADMISSION_CAPACITY:
				exact = report.m_bAdmissionCapacityExact;
				invariant = "admission capacity";
				evidence = report.m_sAdmissionEvidence;
				break;
			case DELIVERY_HELD:
				exact = report.m_bDeliveryHeldExact;
				invariant = "delivery held";
				evidence = report.m_sDeliveryEvidence;
				break;
			case CASUALTY_CONTINUITY:
				exact = report.m_bCasualtyContinuityExact;
				invariant = "casualty continuity";
				evidence = report.m_sCasualtyEvidence;
				break;
			case RESTORE:
				exact = report.m_bRestoreExact;
				invariant = "restore";
				evidence = report.m_sRestoreEvidence;
				break;
			case OWNERSHIP_TERMINAL:
				exact = report.m_bOwnershipTerminalExact;
				invariant = "ownership terminal";
				evidence = report.m_sOwnershipEvidence;
				break;
			case ADMISSION_ROLLBACK:
				exact = report.m_bAdmissionRollbackExact;
				invariant = "admission rollback";
				evidence = report.m_sAdmissionRollbackEvidence;
				break;
			case PREARRIVAL_REFUND:
				exact = report.m_bPrearrivalRefundExact;
				invariant = "prearrival refund";
				evidence = report.m_sPrearrivalRefundEvidence;
				break;
			case SETTLEMENT_CRASH_RESUME:
				exact = report.m_bSettlementCrashResumeExact;
				invariant = "settlement crash resume";
				evidence = report.m_sSettlementCrashEvidence;
				break;
			case HISTORICAL_ISOLATION:
				exact = report.m_bHistoricalIsolationExact;
				invariant = "historical isolation";
				evidence = report.m_sHistoricalEvidence;
				break;
			case SCHEMA70_QUARANTINE:
				exact = report.m_bSchema70QuarantineExact;
				invariant = "Schema-70 quarantine";
				evidence = report.m_sQuarantineEvidence;
				break;
			case ORPHAN_RUNTIME_QUARANTINE:
				exact = report.m_bOrphanRuntimeQuarantineExact;
				invariant = "orphan runtime quarantine";
				evidence = report.m_sOrphanRuntimeEvidence;
				break;
			case QUARANTINE_RETENTION:
				exact = report.m_bQuarantineRetentionExact;
				invariant = "quarantine retention";
				evidence = report.m_sRetentionEvidence;
				break;
			case SELECTED_OWNERSHIP_ABA:
				exact = report.m_bSelectedOwnershipABAExact;
				invariant = "selected ownership ABA";
				evidence = report.m_sSelectedOwnershipABAEvidence;
				break;
			default:
				SetResultFailure("Unknown enemy garrison-rebuild scenario");
				return true;
		}

		Print("Partisan enemy garrison-rebuild scenario | " + invariant
			+ " | " + HST_BuildInfo.BuildRuntimeSummary());
		Print(report.BuildReport());
		Print(evidence);
		AssertTrue(
			exact,
			"Enemy garrison-rebuild " + invariant + " proof failed: "
				+ evidence);
		SetResultSuccess();
		return true;
	}
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_AdmissionCapacity
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(ADMISSION_CAPACITY); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_DeliveryHeld
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(DELIVERY_HELD); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_CasualtyContinuity
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(CASUALTY_CONTINUITY); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_Restore
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(RESTORE); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_OwnershipTerminal
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(OWNERSHIP_TERMINAL); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_AdmissionRollback
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(ADMISSION_ROLLBACK); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_PrearrivalRefund
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(PREARRIVAL_REFUND); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_SettlementCrashResume
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(SETTLEMENT_CRASH_RESUME); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_HistoricalIsolation
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(HISTORICAL_ISOLATION); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_Schema70Quarantine
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(SCHEMA70_QUARANTINE); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_OrphanRuntimeQuarantine
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(ORPHAN_RUNTIME_QUARANTINE); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_QuarantineRetention
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(QUARANTINE_RETENTION); }
}

[Test(suite: HST_EnemyGarrisonRebuildAutotestSuite)]
class HST_TEST_EnemyGarrisonRebuild_SelectedOwnershipABA
	: HST_EnemyGarrisonRebuildIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(SELECTED_OWNERSHIP_ABA); }
}
#endif
