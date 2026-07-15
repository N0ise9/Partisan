// Disposable, profile-local exact-QRF restart proof artifacts. These DTOs
// deliberately contain no machine paths or live service references.
[BaseContainerProps()]
class HST_EnemyQRFPreparedRecoveryExpectation
{
	string m_sOrderId;
	string m_sOperationId;
	string m_sManifestId;
	string m_sManifestHash;
	string m_sBatchId;
	string m_sGroupId;
	string m_sSettlementKind;
	string m_sSettlementId;
	string m_sRefundMutationId;
	string m_sReason;
	int m_iAttackCost;
	int m_iSupportCost;
	int m_iAccepted;
	int m_iSurvivors;
	int m_iAttackRefund;
	int m_iSupportRefund;
	int m_iExpectedAttackPool;
	int m_iExpectedSupportPool;
	int m_iExpectedPoolRevision;
	int m_iExpectedPoolOperationalMutationCount;
	string m_sExpectedLastStrategicMutationId;
	string m_sExpectedLedgerDecisionReason;
	int m_iExpectedLedgerRecentDamageScore;
	int m_iExpectedLedgerLastDamageSecond;
	int m_iExpectedLedgerAttackSpent;
	int m_iExpectedLedgerSupportSpent;
	int m_iExpectedLedgerLastSpendSecond;
	int m_iExpectedLedgerCooldownUntilSecond;
	int m_iExpectedLedgerAttackRefunded;
	int m_iExpectedLedgerSupportRefunded;
	int m_iPreparedAtSecond;
	int m_iExpectedTerminalRevision;
}

[BaseContainerProps()]
class HST_EnemyQRFExternalRestartGuard
{
	string m_sMagic;
	string m_sRunId;
	string m_sRequestedCut;
	bool m_bAllowCanonicalCampaignOverwrite;
}

[BaseContainerProps()]
class HST_EnemyQRFExternalRestartCarrier
{
	string m_sMagic;
	string m_sRunId;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	string m_sWorld;
	string m_sCutName;
	int m_iCut;

	int m_iAccepted;
	int m_iCasualties;
	int m_iSurvivors;
	int m_iAttackRefund;
	int m_iSupportRefund;
	int m_iAttackBeforeRefund;
	int m_iSupportBeforeRefund;
	int m_iLedgerAttackSpentBeforeRefund;
	int m_iLedgerSupportSpentBeforeRefund;
	int m_iLedgerAttackRefundedBefore;
	int m_iLedgerSupportRefundedBefore;
	int m_iPreparedAtSecond;
	int m_iPrefixRevision;
	int m_iExpectedPrefixMutationCount;
	int m_iExpectedPrefixAttackDelta;
	int m_iExpectedPrefixSupportDelta;
	bool m_bExpectedPrefixReceiptApplied;
	string m_sSettlementKind;
	string m_sSettlementId;
	string m_sRefundMutationId;
	string m_sReason;

	ref HST_EnemyQRFPreparedRecoveryExpectation m_Expectation;
	string m_sPreparedFingerprint;
}

[BaseContainerProps()]
class HST_EnemyQRFExternalRestartResult
{
	string m_sMagic;
	string m_sRunId;
	string m_sStage;
	bool m_bSuccess;
	string m_sBuildSha;
	string m_sBuildUtc;
	string m_sBuildLabel;
	int m_iCampaignSchemaVersion;
	string m_sCutName;
	int m_iCut;
	bool m_bRestored;
	bool m_bStartupReconcileChanged;
	bool m_bSourceExact;
	bool m_bSameStateNoOp;
	bool m_bPersistedReadBackExact;
	string m_sFingerprint;
	string m_sEvidence;
}
