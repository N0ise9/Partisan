[BaseContainerProps()]
class HST_SpawnPlacementRequest
{
	string m_sRequestId;
	string m_sPlacementType;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	vector m_vPreferredSourcePosition;
	vector m_vTargetPosition;
	float m_fMinStandoffMeters;
	float m_fMaxStandoffMeters;
	float m_fRoadSearchRadiusMeters = 350.0;
	bool m_bUseHQAsTarget;
	bool m_bRequireDryGround = true;
	bool m_bRequireVehicleSafe;
	bool m_bPreferRoadSource;
	bool m_bRequireRoadSource;
	bool m_bPreferRoadTarget;
	bool m_bRequireRoadTarget;
	bool m_bAvoidHQSafeRadius;
	bool m_bAvoidLivingPlayers;
	bool m_bAvoidActiveGroups;
	float m_fMinPlayerDistanceMeters;
	float m_fMinActiveGroupDistanceMeters;
	bool m_bExplain;
	string m_sReason;
}

[BaseContainerProps()]
class HST_SpawnPlacementResult
{
	bool m_bSuccess;
	string m_sFailureReason;
	string m_sRequestId;
	string m_sPlacementType;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	vector m_vTargetPosition;
	vector m_vSpawnPosition;
	vector m_vRoadForward;
	float m_fTargetDistanceMeters;
	float m_fHQDistanceMeters;
	float m_fRoadDistanceMeters;
	float m_fNearestPlayerDistanceMeters = -1.0;
	float m_fNearestActiveGroupDistanceMeters = -1.0;
	bool m_bDryGround;
	bool m_bVehicleSafe;
	bool m_bRoadResolved;
	bool m_bHQStandoffSatisfied;
	bool m_bPlayerClearanceSatisfied = true;
	bool m_bActiveGroupClearanceSatisfied = true;
	string m_sDebugSummary;
}
