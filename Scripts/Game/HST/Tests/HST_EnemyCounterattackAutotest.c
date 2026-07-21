#ifdef ENABLE_DIAG
// Diagnostic-only engine-process coverage for the deterministic exact enemy
// counterattack report. Full Campaign Debug, native projection, networking,
// profile persistence, and package/restart validation remain separate gates.
class HST_EnemyCounterattackAutotestSuite : SCR_AutotestSuiteBase
{
	// This report is service-only and does not consume a world. Returning an
	// empty resource keeps the command-line runner in the already loaded project
	// context; the base-game scenario transition carries only the base addon list
	// and would otherwise drop the HST test type before it can write JUnit output.
	override ResourceName GetWorldFile()
	{
		return "";
	}
}

// Retained as an unregistered compatibility implementation. Gate 1 executes
// the individually registered invariant cases below without double counting.
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
		Print(report.m_sOwnershipCorrelationEvidence);
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
			report.m_bOwnershipCorrelationQuarantineExact,
			"Counterattack ownership-correlation proof failed: "
				+ report.m_sOwnershipCorrelationEvidence);
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

// Individually registered cases expose each exact report invariant. The
// compatibility implementation above is intentionally absent from the suite.
class HST_EnemyCounterattackIndividualAutotestBase : SCR_AutotestCaseBase
{
	static const int FROZEN_PLANNING = 0;
	static const int ADMISSION = 1;
	static const int VIRTUAL_TRAVEL = 2;
	static const int VIRTUAL_COMBAT = 3;
	static const int PHYSICAL_HANDOFF = 4;
	static const int OWNERSHIP_RETRY = 5;
	static const int SETTLEMENT_REPLAY = 6;
	static const int SUPPORT_SETTLEMENT = 7;
	static const int RESTORE_LIFECYCLE = 8;
	static const int RESOURCE_AUTHORITY_QUARANTINE = 9;
	static const int AMBIGUITY_HOLD = 10;
	static const int OWNERSHIP_CORRELATION_QUARANTINE = 11;
	static const int SCHEMA69_QUARANTINE = 12;
	static const int QUARANTINE_RETENTION = 13;

	protected bool ExecuteScenario(int scenario)
	{
		HST_EnemyCounterattackOperationProofService proof
			= new HST_EnemyCounterattackOperationProofService();
		HST_EnemyCounterattackOperationProofReport report = proof.Run();
		if (!report)
		{
			SetResultFailure(
				"Exact enemy counterattack proof did not return a report");
			return true;
		}

		bool exact;
		string invariant;
		string evidence;
		switch (scenario)
		{
			case FROZEN_PLANNING:
				exact = report.m_bFrozenPlanningExact;
				invariant = "frozen planning";
				evidence = report.m_sPlanningEvidence;
				break;
			case ADMISSION:
				exact = report.m_bAdmissionExact;
				invariant = "admission";
				evidence = report.m_sAdmissionEvidence;
				break;
			case VIRTUAL_TRAVEL:
				exact = report.m_bVirtualTravelExact;
				invariant = "virtual travel";
				evidence = report.m_sTravelEvidence;
				break;
			case VIRTUAL_COMBAT:
				exact = report.m_bVirtualCombatExact;
				invariant = "virtual combat";
				evidence = report.m_sCombatEvidence;
				break;
			case PHYSICAL_HANDOFF:
				exact = report.m_bPhysicalHandoffExact;
				invariant = "physical handoff";
				evidence = report.m_sPhysicalHandoffEvidence;
				break;
			case OWNERSHIP_RETRY:
				exact = report.m_bOwnershipRetryExact;
				invariant = "ownership retry";
				evidence = report.m_sOwnershipEvidence;
				break;
			case SETTLEMENT_REPLAY:
				exact = report.m_bSettlementReplayExact;
				invariant = "settlement replay";
				evidence = report.m_sSettlementEvidence;
				break;
			case SUPPORT_SETTLEMENT:
				exact = report.m_bSupportSettlementExact;
				invariant = "support settlement";
				evidence = report.m_sSupportSettlementEvidence;
				break;
			case RESTORE_LIFECYCLE:
				exact = report.m_bRestoreLifecycleExact;
				invariant = "restore lifecycle";
				evidence = report.m_sRestoreEvidence;
				break;
			case RESOURCE_AUTHORITY_QUARANTINE:
				exact = report.m_bResourceAuthorityQuarantineExact;
				invariant = "resource-authority quarantine";
				evidence = report.m_sResourceAuthorityEvidence;
				break;
			case AMBIGUITY_HOLD:
				exact = report.m_bAmbiguityHoldExact;
				invariant = "ambiguity hold";
				evidence = report.m_sAmbiguityEvidence;
				break;
			case OWNERSHIP_CORRELATION_QUARANTINE:
				exact = report.m_bOwnershipCorrelationQuarantineExact;
				invariant = "ownership-correlation quarantine";
				evidence = report.m_sOwnershipCorrelationEvidence;
				break;
			case SCHEMA69_QUARANTINE:
				exact = report.m_bSchema69QuarantineExact;
				invariant = "Schema-69 quarantine";
				evidence = report.m_sQuarantineEvidence;
				break;
			case QUARANTINE_RETENTION:
				exact = report.m_bQuarantineRetentionExact;
				invariant = "quarantine retention";
				evidence = report.m_sRetentionEvidence;
				break;
			default:
				SetResultFailure("Unknown enemy counterattack scenario");
				return true;
		}

		Print("Partisan enemy counterattack scenario | " + invariant
			+ " | " + HST_BuildInfo.BuildRuntimeSummary());
		Print(report.BuildReport());
		Print(evidence);
		AssertTrue(
			exact,
			"Enemy counterattack " + invariant + " proof failed: " + evidence);
		SetResultSuccess();
		return true;
	}
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_FrozenPlanning
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(FROZEN_PLANNING); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_Admission
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(ADMISSION); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_VirtualTravel
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(VIRTUAL_TRAVEL); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_VirtualCombat
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(VIRTUAL_COMBAT); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_PhysicalHandoff
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(PHYSICAL_HANDOFF); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_OwnershipRetry
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(OWNERSHIP_RETRY); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_SettlementReplay
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(SETTLEMENT_REPLAY); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_SupportSettlement
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(SUPPORT_SETTLEMENT); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_RestoreLifecycle
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(RESTORE_LIFECYCLE); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_ResourceAuthorityQuarantine
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(RESOURCE_AUTHORITY_QUARANTINE); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_AmbiguityHold
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(AMBIGUITY_HOLD); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_OwnershipCorrelationQuarantine
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(OWNERSHIP_CORRELATION_QUARANTINE); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_Schema69Quarantine
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(SCHEMA69_QUARANTINE); }
}

[Test(suite: HST_EnemyCounterattackAutotestSuite)]
class HST_TEST_EnemyCounterattack_QuarantineRetention
	: HST_EnemyCounterattackIndividualAutotestBase
{
	[Step(EStage.Main)] bool Execute() { return ExecuteScenario(QUARANTINE_RETENTION); }
}
#endif
