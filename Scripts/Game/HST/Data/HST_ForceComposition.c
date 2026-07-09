[BaseContainerProps()]
class HST_ForceRequest
{
	string m_sRequestId;
	string m_sFactionKey;
	string m_sIntentId;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	vector m_vSourcePosition;
	vector m_vTargetPosition;
	int m_iWarLevel;
	int m_iTrainingLevel;
	int m_iBudget;
	int m_iMinManpower;
	int m_iMaxManpower;
	bool m_bAllowInfantry = true;
	bool m_bAllowVehicles;
	bool m_bAllowArmedVehicles;
	bool m_bAllowLightArmor;
	bool m_bAllowHeavyArmor;
	bool m_bAllowHelicopter;
	bool m_bAllowStatics;
	bool m_bRestrictToForcedPrefabs;
	float m_fLocalThreat;
	float m_fAggression;
	float m_fMechanizedShare;
	string m_sReason;
	bool m_bExplain;
	ref array<string> m_aForcedGroupPrefabs = {};
	ref array<string> m_aForcedVehiclePrefabs = {};
}

[BaseContainerProps()]
class HST_GroupSpawnPlan
{
	string m_sPrefab;
	string m_sRole;
	string m_sTier;
	string m_sFailureReason;
	int m_iManpower;
	int m_iCost;
	int m_iWeight = 1;
	bool m_bSkipped;
}

[BaseContainerProps()]
class HST_VehicleSpawnPlan
{
	string m_sPrefab;
	string m_sRole;
	string m_sTier;
	string m_sFailureReason;
	int m_iCrew;
	int m_iCost;
	bool m_bArmed;
	bool m_bLightArmor;
	bool m_bHeavyArmor;
	bool m_bSkipped;
}

[BaseContainerProps()]
class HST_StaticWeaponSpawnPlan
{
	string m_sPrefab;
	string m_sRole;
	string m_sTier;
	string m_sFailureReason;
	int m_iCrew;
	int m_iCost;
	bool m_bSkipped;
}

[BaseContainerProps()]
class HST_ForceCompositionResult
{
	bool m_bSuccess;
	string m_sFailureReason;
	string m_sRequestId;
	string m_sFactionKey;
	string m_sIntentId;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	int m_iWarLevel;
	int m_iTrainingLevel;
	int m_iTrainingQualityBonusPercent;
	int m_iBudget;
	int m_iTotalCost;
	int m_iManpower;
	int m_iEffectiveManpower;
	int m_iVehicleCount;
	int m_iArmedVehicleCount;
	int m_iSkippedPrefabCount;
	string m_sSelectedTier;
	string m_sDebugSummary;
	ref array<ref HST_GroupSpawnPlan> m_aGroups = {};
	ref array<ref HST_VehicleSpawnPlan> m_aVehicles = {};
	ref array<ref HST_StaticWeaponSpawnPlan> m_aStatics = {};

	HST_GroupSpawnPlan GetPrimaryGroup()
	{
		foreach (HST_GroupSpawnPlan plan : m_aGroups)
		{
			if (plan && !plan.m_bSkipped && !plan.m_sPrefab.IsEmpty())
				return plan;
		}

		return null;
	}

	HST_VehicleSpawnPlan GetPrimaryVehicle()
	{
		foreach (HST_VehicleSpawnPlan plan : m_aVehicles)
		{
			if (plan && !plan.m_bSkipped && !plan.m_sPrefab.IsEmpty())
				return plan;
		}

		return null;
	}
}
