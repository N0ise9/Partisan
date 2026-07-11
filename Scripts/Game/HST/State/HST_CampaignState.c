[BaseContainerProps()]
class HST_FactionPoolState
{
	string m_sFactionKey;
	int m_iAttackResources;
	int m_iSupportResources;
	int m_iMoney;
	int m_iHR;
	int m_iAggression;
}

[BaseContainerProps()]
class HST_PlayerState
{
	string m_sIdentityId;
	string m_sDisplayName;
	string m_sFactionKey = "FIA";
	bool m_bMember;
	bool m_bAdmin;
	bool m_bGuest = true;
	int m_iMoney;
	int m_iRank;
	int m_iLastSeenPlayerId = -1;
	bool m_bHasSpawnRecord;
	int m_iSpawnCount;
	string m_sLastSpawnPrefab;
	vector m_vLastSpawnPosition;
}

[BaseContainerProps()]
class HST_ZoneState
{
	string m_sZoneId;
	string m_sDisplayName;
	string m_sSourceLayoutId;
	string m_sSourceLayerName;
	string m_sMarkerCallsign;
	string m_sMarkerTextColor;
	string m_sMarkerStyle;
	string m_sOwnerFactionKey;
	HST_EZoneType m_eType;
	vector m_vPosition;
	string m_sResourceKind;
	int m_iSupport;
	int m_iResistanceCaptureProgress;
	int m_iIncomeValue;
	int m_iCaptureRadiusMeters;
	int m_iPriority;
	int m_iGarrisonSlots;
	int m_iActivationRadiusMeters;
	string m_sCompositionId;
	string m_sSpawnProfileId;
	bool m_bActive;
	int m_iActiveInfantryCount;
	int m_iActiveVehicleCount;
	string m_sPatrolRouteId;
	string m_sQRFRouteId;
	string m_sMissionSiteId;
	int m_iQrfCooldownUntilSecond;
	ref array<string> m_aLinkedZoneIds = {};
}

[BaseContainerProps()]
class HST_GarrisonState
{
	string m_sGarrisonId;
	string m_sZoneId;
	string m_sFactionKey;
	int m_iInfantryCount;
	int m_iVehicleCount;
	ref array<string> m_aAcceptedManifestIds = {};
}

[BaseContainerProps()]
class HST_ActiveGroupState
{
	string m_sGroupId;
	string m_sOperationId;
	string m_sManifestId;
	string m_sSpawnResultId;
	string m_sForceId;
	string m_sProjectionId;
	string m_sZoneId;
	string m_sFactionKey;
	string m_sMissionInstanceId;
	string m_sSupportRequestId;
	string m_sEnemyOrderId;
	string m_sConvoyElementId;
	string m_sMissionAssetId;
	string m_sGarrisonZoneId;
	string m_sQRFInstanceId;
	string m_sPrefab;
	string m_sVehiclePrefab;
	string m_sCompositionRequestId;
	string m_sCompositionIntentId;
	string m_sCompositionTier;
	string m_sCompositionSummary;
	string m_sSpawnFallbackMode;
	string m_sSpawnFailureReason;
	vector m_vPosition;
	string m_sRouteId;
	vector m_vSourcePosition;
	vector m_vTargetPosition;
	string m_sRuntimeEntityId;
	string m_sRuntimeStatus = "queued";
	int m_iInfantryCount;
	int m_iVehicleCount;
	int m_iOriginalInfantryCount;
	int m_iOriginalVehicleCount;
	int m_iCompositionCost;
	int m_iCompositionManpower;
	int m_iCompositionVehicleCount;
	int m_iCompositionArmedVehicleCount;
	int m_iSpawnedAtSecond;
	int m_iLastSeenAliveCount;
	int m_iSurvivorInfantryCount;
	int m_iSurvivorVehicleCount;
	int m_iSpawnedAgentCount;
	int m_iAssignedWaypointCount;
	int m_iMaxObservedCrewAlive;
	int m_iDurableLivingInfantryCount;
	int m_iLastCasualtySecond;
	int m_iEliminatedAtSecond;
	int m_iLifecycleRevision;
	bool m_bEverHadLivingCrew;
	bool m_bEverPopulated;
	bool m_bSpawnCompleted;
	bool m_bCrewPopulationTerminallyFailed;
	string m_sCrewPopulationFailureReason;
	string m_sConvoyRuntimeStage;
	bool m_bQRF;
	bool m_bSpawnAttempted;
	bool m_bSpawnedEntity;
}

[BaseContainerProps()]
class HST_QRFState
{
	string m_sInstanceId;
	string m_sFactionKey;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sGroupId;
	int m_iStartedAtSecond;
	int m_iETASeconds;
	bool m_bResolved;
	bool m_bSucceeded;
}

[BaseContainerProps()]
class HST_OperationRecordState
{
	string m_sOperationId;
	HST_EOperationType m_eType = HST_EOperationType.HST_OPERATION_TYPE_UNKNOWN;
	int m_iContractVersion;
	string m_sOwnerFactionKey;
	string m_sActorIdentityId;
	string m_sIssueRequestId;
	string m_sConfirmationRequestId;
	string m_sSupportRequestId;
	string m_sEnemyOrderId;
	string m_sMissionInstanceId;
	string m_sQuoteId;
	string m_sManifestId;
	string m_sSpawnResultId;
	string m_sForceId;
	string m_sProjectionId;
	string m_sGroupId;
	string m_sOriginZoneId;
	vector m_vOriginPosition;
	string m_sAssignmentKind;
	string m_sAssignmentZoneId;
	vector m_vAssignmentPosition;
	string m_sTacticalTargetZoneId;
	vector m_vTacticalTargetPosition;
	vector m_vStrategicPosition;
	string m_sCurrentRouteId;
	string m_sRouteContractHash;
	int m_iProjectionContractVersion;
	int m_iRouteVersion;
	int m_iRouteWaypointIndex = -1;
	int m_iRouteLapCount;
	int m_iRouteLegSequence;
	int m_iRouteLoopStartedAtSecond;
	int m_iRouteLoopCompletedAtSecond;
	vector m_vRouteStartPosition;
	vector m_vRouteEndPosition;
	float m_fRouteTotalDistanceMeters;
	float m_fRouteProgressMeters;
	float m_fStrategicSpeedMetersPerSecond;
	int m_iStrategicLastUpdateSecond;
	int m_iLastProjectionDecisionSecond;
	int m_iLastNormalizedRestoreSequence = -1;
	int m_iVirtualCombatLastStepSecond;
	int m_iVirtualCombatStepIndex;
	int m_iVirtualCombatFriendlyDamageCarry;
	int m_iVirtualCombatHostileDamageCarry;
	int m_iLastVirtualFriendlyCount;
	int m_iLastVirtualHostileCount;
	int m_iArrivalConfirmationCount;
	int m_iLastArrivalConfirmationSecond;
	string m_sLastProjectionReason;
	string m_sLastVirtualCombatReason;
	string m_sRecallPolicyId;
	string m_sSettlementPolicyId;
	HST_EOperationDutyState m_eDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN;
	HST_EOperationDutyState m_eResumeDutyState = HST_EOperationDutyState.HST_OPERATION_DUTY_UNKNOWN;
	HST_EOperationEngagementMode m_eEngagementMode = HST_EOperationEngagementMode.HST_OPERATION_ENGAGEMENT_UNKNOWN;
	HST_EOperationMaterializationState m_eMaterializationState = HST_EOperationMaterializationState.HST_OPERATION_MATERIALIZATION_UNKNOWN;
	HST_EOperationPositionAuthority m_ePositionAuthority = HST_EOperationPositionAuthority.HST_OPERATION_POSITION_UNKNOWN;
	HST_EOperationSettlementState m_eSettlementState = HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_UNKNOWN;
	HST_EOperationTerminalResult m_eTerminalResult = HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_UNKNOWN;
	string m_sSettlementId;
	string m_sTerminalReason;
	int m_iDeterministicSeed;
	int m_iCreatedAtSecond;
	int m_iDutyStateEnteredAtSecond;
	int m_iEngagementStateEnteredAtSecond;
	int m_iMaterializationStateEnteredAtSecond;
	int m_iLastContactAtSecond;
	int m_iLastProgressAtSecond;
	int m_iSettledAtSecond;
	int m_iRevision = 1;
}

[BaseContainerProps()]
class HST_MapMarkerState
{
	string m_sMarkerId;
	string m_sLinkedId;
	string m_sLabel;
	string m_sCallsign;
	string m_sCategory;
	string m_sOwnerFactionKey;
	string m_sIconHint;
	string m_sColorHint;
	string m_sTextColorHint;
	string m_sStyleHint;
	vector m_vPosition;
	bool m_bVisible = true;
	bool m_bRuntimeNative;
}

[BaseContainerProps()]
class HST_ArsenalItemState
{
	string m_sPrefab;
	string m_sDisplayName;
	string m_sCategory;
	int m_iCount;
	bool m_bUnlocked;
}

[BaseContainerProps()]
class HST_StoredVehicleCargoState
{
	string m_sItemPrefab;
	string m_sDisplayName;
	string m_sCategory;
	string m_sSource;
	int m_iCount;
}

[BaseContainerProps()]
class HST_GarageVehicleState
{
	string m_sVehicleId;
	string m_sPrefab;
	string m_sDisplayName;
	string m_sSourceZoneId;
	string m_sSourceFactionKey;
	string m_sSourceVehicleKind = "transport";
	int m_iStoredAtSecond;
	int m_iRedeployCost;
	vector m_vPosition;
	vector m_vAngles;
	float m_fFuel;
	string m_sDamageState;
	bool m_bArmed;
	bool m_bAmmoSource;
	bool m_bRepairSource;
	bool m_bFuelSource;
	bool m_bReported;
	bool m_bCanProvideUndercover;
	int m_iVehicleHeat;
	int m_iLastReportedSecond;
	int m_iReportedUntilSecond;
	int m_iLastVehicleHeatChangedSecond;
	int m_iPassengerCompromiseCount;
	string m_sLastReportedReason;
	string m_sLastReporterZoneId;
	bool m_bUnlocked;
	bool m_bHadPhysicalCargo;
	ref array<ref HST_StoredVehicleCargoState> m_aStoredCargoItems = {};
}

[BaseContainerProps()]
class HST_VehicleCargoItemState
{
	string m_sVehicleRuntimeId;
	string m_sVehiclePrefab;
	string m_sVehicleDisplayName;
	string m_sItemPrefab;
	string m_sDisplayName;
	string m_sCategory;
	int m_iCount;
	int m_iLastStoredAtSecond;
	vector m_vLastVehiclePosition;
}

[BaseContainerProps()]
class HST_RuntimeVehicleState
{
	string m_sVehicleRuntimeId;
	string m_sPrefab;
	string m_sDisplayName;
	string m_sFactionKey;
	string m_sZoneId;
	string m_sRuntimeKind;
	string m_sSourceVehicleKind = "transport";
	vector m_vPosition;
	vector m_vAngles;
	int m_iSpawnedAtSecond;
	bool m_bDetached;
	bool m_bDeleted;
	bool m_bAmmoSource;
	bool m_bRepairSource;
	bool m_bFuelSource;
	bool m_bReported;
	bool m_bCanProvideUndercover;
	int m_iVehicleHeat;
	int m_iLastReportedSecond;
	int m_iReportedUntilSecond;
	int m_iLastVehicleHeatChangedSecond;
	int m_iPassengerCompromiseCount;
	string m_sLastReportedReason;
	string m_sLastReporterZoneId;
}

[BaseContainerProps()]
class HST_LoadoutSlotState
{
	string m_sSlotId;
	string m_sItemPrefab;
	string m_sDisplayName;
	string m_sCategory;
	int m_iQuantity = 1;
	string m_sWeaponSlotId;
	string m_sAttachmentSlotId;
	string m_sParentSlotId;
	string m_sStorageId;
	string m_sSlotKind;
}

[BaseContainerProps()]
class HST_LoadoutNodeState
{
	string m_sNodeId;
	string m_sParentNodeId;
	string m_sKind;
	string m_sSlotKey;
	string m_sLabel;
	string m_sItemPrefab;
	string m_sDisplayName;
	string m_sCategory;
	string m_sFocus;
	string m_sCompatibilityKey;
	string m_sCountLabel;
	int m_iQuantity = 1;
	int m_iUsedCapacity;
	int m_iTotalCapacity;
	float m_fUsedVolume;
	float m_fTotalVolume;
	float m_fFreeVolume;
	int m_iStorageIndex = -1;
	int m_iSlotIndex = -1;
	int m_iParentSlotIndex = -1;
	bool m_bInfinite;
	bool m_bCanOpen;
	bool m_bCanRemove;
	bool m_bCanDeposit;
	bool m_bAmmoMatch;
}

[BaseContainerProps()]
class HST_SavedLoadoutState
{
	string m_sOwnerIdentityId;
	string m_sLoadoutId;
	string m_sDisplayName;
	string m_sCharacterPrefab;
	string m_sSerializedLoadout;
	string m_sClothingSummary;
	string m_sWeaponSummary;
	string m_sRequiredItemsSummary;
	int m_iUpdatedAtSecond;
	int m_iSlotIndex = -1;
	ref array<ref HST_LoadoutSlotState> m_aSlots = {};
}

[BaseContainerProps()]
class HST_IssuedLoadoutItemState
{
	string m_sOwnerIdentityId;
	string m_sItemPrefab;
	string m_sDisplayName;
	string m_sCategory;
	int m_iCount;
	bool m_bInfinite;
}

[BaseContainerProps()]
class HST_LoadoutEditorSessionState
{
	string m_sOwnerIdentityId;
	string m_sStatus = "closed";
	string m_sLastFailure;
	string m_sPreviewPrefab;
	string m_sPreviewStatus;
	vector m_vPreviewPosition;
	bool m_bPreviewSpawned;
	int m_iPreviewItemCount;
	string m_sCurrentLoadoutId;
	int m_iOpenedAtSecond;
	int m_iSavedLoadoutCount;
	int m_iIssuedFiniteCount;
	int m_iIssuedInfiniteCount;
	int m_iPlayerId;
	bool m_bLiveCharacterAvailable;
	ref array<ref HST_LoadoutSlotState> m_aDraftSlots = {};
	ref array<ref HST_LoadoutNodeState> m_aDraftNodes = {};
}

[BaseContainerProps()]
class HST_EmplacementState
{
	string m_sEmplacementId;
	string m_sPrefab;
	vector m_vPosition;
	vector m_vAngles;
}

[BaseContainerProps()]
class HST_AmmoPointState
{
	string m_sAmmoPointId;
	vector m_vPosition;
}

[BaseContainerProps()]
class HST_GunShopItemState
{
	string m_sItemId;
	string m_sPrefab;
	string m_sDisplayName;
	string m_sCategory;
	string m_sFactionKey;
	int m_iAvailableCount;
	int m_iPurchasedCount;
	int m_iBuyCost;
	int m_iSellCost;
	bool m_bCanSell;
}

[BaseContainerProps()]
class HST_ActiveMissionState
{
	string m_sInstanceId;
	string m_sMissionId;
	string m_sDisplayName;
	string m_sOperationId;
	string m_sManifestId;
	string m_sSpawnResultId;
	string m_sSettlementId;
	int m_iOperationContractVersion;
	HST_EMissionStatus m_eStatus;
	HST_EMissionRuntimeMode m_eRuntimeMode = HST_EMissionRuntimeMode.HST_MISSION_RUNTIME_ABSTRACT;
	int m_iRemainingSeconds;
	string m_sTargetZoneId;
	string m_sSiteId;
	vector m_vTargetPosition;
	string m_sMarkerId;
	string m_sRuntimePrimitive;
	string m_sRuntimeType;
	string m_sRuntimePhase;
	string m_sRuntimeFailureReason;
	string m_sRuntimeEntityId;
	string m_sLastRuntimeEventKey;
	int m_iStartedAtSecond;
	int m_iActiveUntilSecond;
	int m_iRuntimeStartedAtSecond;
	int m_iRuntimeHoldSeconds;
	int m_iRuntimeETASeconds;
	int m_iRuntimeCounterA;
	int m_iRuntimeCounterB;
	int m_iRuntimeCounterC;
	int m_iRequiredCargoCount = 1;
	int m_iRecoveredCargoCount;
	int m_iRequiredCaptiveCount = 1;
	int m_iExtractedCaptiveCount;
	int m_iRequiredVehicleCount = 1;
	int m_iCapturedVehicleCount;
	int m_iRuntimePickupCount;
	int m_iRuntimeDeliveryCount;
	int m_iRuntimeDestroyedCount;
	bool m_bDynamic;
	bool m_bRequested;
	bool m_bStatic;
	bool m_bRuntimeSpawned;
	bool m_bRuntimeFallback;
	bool m_bRuntimeCleanupComplete;
	bool m_bCreatedNotificationSent;
	bool m_bCompletedNotificationSent;
	bool m_bFailedNotificationSent;
	bool m_bExpiredNotificationSent;
	bool m_bConvoyArrivalOutcomeApplied;
	bool m_bConvoyCrewEliminatedOutcomeApplied;
	bool m_bConvoyVehicleCapturedOutcomeApplied;
	bool m_bConvoyCargoDeliveredOutcomeApplied;
	bool m_bConvoyExpiredOutcomeApplied;
	string m_sConvoyOutcomeSummary;
	ref array<ref HST_GunShopItemState> m_aGunShopItems = {};
	string m_sGunShopSellerAssetId;
	string m_sGunShopDeliveryDriverAssetId;
	string m_sGunShopDeliveryVehicleAssetId;
	string m_sGunShopDeliveryMarkerId;
	vector m_vGunShopSellerPosition;
	vector m_vGunShopDeliveryPosition;
	bool m_bGunShopStockGenerated;
	bool m_bGunShopPurchaseMade;
	bool m_bGunShopPurchaseNoticeSent;
	bool m_bGunShopExpiryNoticeSent;
	bool m_bGunShopDeliverySpawned;
	bool m_bGunShopDeliveryNoticeSent;
	bool m_bGunShopDeliveryArrived;
	int m_iGunShopPurchasedTotal;
	int m_iGunShopDeliveryStartedAtSecond;
}

[BaseContainerProps()]
class HST_GeneratedSiteState
{
	string m_sSiteId;
	string m_sZoneId;
	string m_sRouteId;
	string m_sSourceLayerName;
	string m_sSourceCategory;
	string m_sSourceLayoutId;
	HST_EGeneratedSiteType m_eType;
	vector m_vPosition;
	vector m_vSecondaryPosition;
	int m_iRadiusMeters;
	int m_iWeight = 1;
	bool m_bValid = true;
	bool m_bOccupied;
	string m_sOwnerFactionKey;
}

[BaseContainerProps()]
class HST_RouteWaypointState
{
	string m_sRouteId;
	int m_iIndex;
	vector m_vPosition;
	int m_iRadiusMeters;
	string m_sHint;
}

[BaseContainerProps()]
class HST_GeneratedRouteState
{
	string m_sRouteId;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sSourceLayerName;
	string m_sSourceCategory;
	string m_sSourceLayoutId;
	vector m_vStartPosition;
	vector m_vMidPosition;
	vector m_vEndPosition;
	int m_iDistanceMeters;
	int m_iWaypointCount;
	bool m_bRoadRoute = true;
	bool m_bValidatedForVehicles;
	string m_sValidationFailureReason;
	ref array<ref HST_RouteWaypointState> m_aWaypoints = {};
}

[BaseContainerProps()]
class HST_MissionObjectiveState
{
	string m_sObjectiveId;
	string m_sMissionInstanceId;
	HST_EMissionObjectiveType m_eType;
	string m_sLabel;
	string m_sRequirementText;
	string m_sTargetId;
	string m_sTargetZoneId;
	string m_sPhysicalEntityId;
	string m_sLinkedRuntimeEntityId;
	string m_sRuntimePrimitive;
	vector m_vPosition;
	int m_iRequiredProgress = 1;
	int m_iCurrentProgress;
	int m_iHoldSeconds;
	int m_iRequiredHoldSeconds;
	int m_iCurrentCount;
	int m_iRequiredCount = 1;
	bool m_bExtractionStarted;
	bool m_bDeliveryStarted;
	bool m_bComplete;
	bool m_bFailed;
	bool m_bCleanupComplete;
	bool m_bWorldDetected;
	bool m_bAbstractFallback;
}

[BaseContainerProps()]
class HST_MissionRuntimeEntityState
{
	string m_sRuntimeEntityId;
	string m_sMissionInstanceId;
	string m_sKind;
	string m_sPrefab;
	vector m_vPosition;
	vector m_vAngles;
	bool m_bSpawned;
	bool m_bDestroyed;
	bool m_bRecovered;
}

[BaseContainerProps()]
class HST_MissionAssetState
{
	string m_sAssetId;
	string m_sMissionInstanceId;
	string m_sOperationId;
	string m_sManifestId;
	string m_sManifestSlotId;
	string m_sAssignedVehicleSlotId;
	string m_sConvoyElementId;
	string m_sKind;
	string m_sRole;
	string m_sPrefab;
	string m_sEntityId;
	string m_sCarriedByVehicleId;
	string m_sLastInteraction;
	bool m_bSpawned;
	bool m_bPickedUp;
	bool m_bDelivered;
	bool m_bDestroyed;
	bool m_bAlive = true;
	bool m_bAttachedToCarrier;
	bool m_bOutcomeApplied;
	string m_sOutcomeKind;
	float m_fDemolitionDamage;
	float m_fDemolitionRequiredDamage = 300.0;
	int m_iDemolitionHits;
	string m_sLastDemolitionSource;
	int m_iLastDemolitionSecond;
	vector m_vSourcePosition;
	vector m_vTargetPosition;
	vector m_vCurrentPosition;
	vector m_vLastKnownPosition;
	int m_iDeadlineSecond;
	int m_iCargoCapacityCost = 1;
	int m_iInteractionRadiusMeters;
}

[BaseContainerProps()]
class HST_ConvoyElementState
{
	string m_sElementId;
	string m_sOperationId;
	string m_sMissionInstanceId;
	string m_sManifestId;
	string m_sVehicleSlotId;
	string m_sCrewGroupElementId;
	string m_sVehicleAssetId;
	string m_sGroupId;
	string m_sCargoAssetId;
	string m_sVehiclePrefab;
	string m_sCrewGroupPrefab;
	string m_sTerminalReason;
	vector m_vFormationOffset;
	vector m_vCurrentPosition;
	int m_iOrdinal;
	int m_iOriginalCrewCount;
	int m_iSurvivingCrewCount;
	int m_iLastUpdatedSecond;
	int m_iRevision = 1;
	float m_fVehicleDamageFraction;
	float m_fFuelFraction = 1.0;
	float m_fAmmoFraction = 1.0;
	HST_EConvoyElementDisposition m_eDisposition = HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ACTIVE;
	bool m_bPhysicalized;
	bool m_bMobile = true;
}

[BaseContainerProps()]
class HST_SupportRequestState
{
	string m_sRequestId;
	string m_sOperationId;
	string m_sQuoteId;
	string m_sManifestId;
	string m_sSpawnResultId;
	string m_sCommandRequestId;
	string m_sMoneyTransactionId;
	string m_sHRTransactionId;
	string m_sFactionKey;
	string m_sCapabilityId;
	string m_sAssetProfileId;
	string m_sCompositionRequestId;
	string m_sCompositionIntentId;
	string m_sCompositionTier;
	string m_sCompositionSummary;
	string m_sCompositionFailureReason;
	string m_sStrikeKind;
	string m_sStrikeConfigResource;
	HST_ESupportRequestType m_eType;
	HST_ESupportRequestStatus m_eStatus;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sGroupId;
	string m_sRuntimeEntityId;
	string m_sDeploymentRouteId;
	string m_sDeploymentPlacementType;
	string m_sDeploymentSummary;
	string m_sSelectedGarageVehicleId;
	string m_sSelectedGarageVehiclePrefab;
	string m_sSelectedGarageVehicleDisplayName;
	string m_sRuntimeStatus = "queued";
	string m_sResolutionKind;
	string m_sPhysicalizationMode;
	vector m_vSourcePosition;
	vector m_vTargetPosition;
	int m_iRequestedAtSecond;
	int m_iETASeconds;
	int m_iAttackCost;
	int m_iSupportCost;
	int m_iMoneyCost;
	int m_iHRCost;
	int m_iPlannedInfantryCount;
	int m_iOperationContractVersion;
	int m_iRefundedHR;
	int m_iCompositionCost;
	int m_iCompositionManpower;
	int m_iCompositionVehicleCount;
	int m_iCompositionArmedVehicleCount;
	int m_iDeploymentTargetDistanceMeters;
	int m_iDeploymentRoadDistanceMeters;
	int m_iDeploymentHQDistanceMeters;
	int m_iCooldownUntilSecond;
	int m_iActivatedAtSecond;
	int m_iPhysicalizedAtSecond;
	int m_iResolvedAtSecond;
	int m_iRecallRequestedAtSecond;
	vector m_vRecallExitPosition;
	bool m_bHelicopterStyle;
	bool m_bPlayerRequested;
	bool m_bRecallRequested;
	bool m_bPhysicalStrikeSpawned;
	bool m_bDeploymentRoadResolved;
	bool m_bDeploymentVehicleSafe;
	bool m_bDeploymentVehicleSafeRequired;
	bool m_bAbstractResolved;
	bool m_bPhysicalized;
	bool m_bOutcomeApplied;
	bool m_bGarageVehicleConsumed;
	string m_sFailureReason;
}

[BaseContainerProps()]
class HST_EnemyOrderState
{
	string m_sOrderId;
	string m_sOperationId;
	int m_iOperationContractVersion;
	string m_sManifestId;
	string m_sManifestHash;
	string m_sSpawnResultId;
	string m_sFactionKey;
	HST_EEnemyOrderType m_eType;
	HST_EEnemyOrderStatus m_eStatus;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	string m_sCompositionRequestId;
	string m_sCompositionIntentId;
	string m_sCompositionTier;
	string m_sCompositionSummary;
	string m_sCompositionFailureReason;
	string m_sSupportRequestId;
	string m_sGroupId;
	string m_sRuntimeStatus = "queued";
	string m_sResolutionKind;
	string m_sFailureReason;
	vector m_vSourcePosition;
	vector m_vTargetPosition;
	int m_iCreatedAtSecond;
	int m_iResolveAtSecond;
	int m_iPhysicalizedAtSecond;
	int m_iResolvedAtSecond;
	int m_iAttackCost;
	int m_iSupportCost;
	int m_iRefundedAttackResources;
	int m_iRefundedSupportResources;
	int m_iCompositionCost;
	int m_iCompositionManpower;
	int m_iCompositionVehicleCount;
	int m_iCompositionArmedVehicleCount;
	string m_sResourceSettlementId;
	string m_sResourceSettlementKind;
	int m_iSettlementAcceptedMemberCount;
	int m_iSettlementSurvivorMemberCount;
	bool m_bStrategicServiceCommitted;
	bool m_bResourceSettlementApplied;
	bool m_bPhysicalized;
	bool m_bAbstractResolved;
	bool m_bOutcomeApplied;
	bool m_bResourceRefundApplied;
}

[BaseContainerProps()]
class HST_EnemySupportLedgerState
{
	string m_sFactionKey;
	string m_sZoneId;
	string m_sLastDecisionReason;
	int m_iRecentDamageScore;
	int m_iLastDamageSecond;
	int m_iAttackSpent;
	int m_iSupportSpent;
	int m_iLastSpendSecond;
	int m_iCooldownUntilSecond;
	int m_iRefundedAttackResources;
	int m_iRefundedSupportResources;
}

[BaseContainerProps()]
class HST_CivilianZoneState
{
	string m_sZoneId;
	int m_iReputation;
	int m_iFIASupport;
	int m_iOccupierSupport;
	int m_iWantedHeat;
	int m_iCivilianPresence;
	int m_iPolicePresence;
	int m_iRoadblockPresence;
	int m_iLastIncidentSecond;
	string m_sLastIncidentReason;
	int m_iLastSupportChangeSecond;
	int m_iLastRoadblockScanSecond;
	int m_iLastPoliceScanSecond;
	string m_sLastSecurityReason;
	bool m_bUndercoverRestricted;
	int m_iPopulationRemaining;
	int m_iPopulationKilled;
	int m_iInfluenceEventCount;
	int m_iActiveInfluenceModifierCount;
	int m_iExpiredInfluenceModifierCount;
	int m_iLastInfluenceEventSecond;
	string m_sLastInfluenceEventId;
	string m_sLastInfluenceKind;
	string m_sLastInfluenceReason;
}

[BaseContainerProps()]
class HST_TownInfluenceEventState
{
	string m_sEventId;
	string m_sZoneId;
	string m_sKind;
	string m_sSourceId;
	string m_sReason;
	int m_iCreatedAtSecond;
	int m_iExpiresAtSecond;
	int m_iFIASupportDelta;
	int m_iOccupierSupportDelta;
	int m_iReputationDelta;
	int m_iHeatDelta;
	int m_iPopulationDelta;
	int m_iPoliceDelta;
	int m_iRoadblockDelta;
	bool m_bApplied;
}

[BaseContainerProps()]
class HST_StrategicEventState
{
	string m_sEventId;
	string m_sKind;
	string m_sSourceType;
	string m_sSourceId;
	string m_sMissionId;
	string m_sMissionInstanceId;
	string m_sTargetZoneId;
	string m_sTargetFactionKey;
	string m_sReason;
	string m_sSummary;
	int m_iCreatedAtSecond;
	int m_iFactionMoneyDelta;
	int m_iHRDelta;
	int m_iAggressionDelta;
	int m_iAttackResourceDelta;
	int m_iSupportResourceDelta;
	int m_iTownSupportDelta;
	int m_iCaptureProgressDelta;
	int m_iHQKnowledgeDelta;
	string m_sOwnerBefore;
	string m_sOwnerAfter;
	int m_iSupportBefore;
	int m_iSupportAfter;
	int m_iCaptureProgressBefore;
	int m_iCaptureProgressAfter;
	int m_iHQKnowledgeBefore;
	int m_iHQKnowledgeAfter;
	string m_sVehicleRuntimeId;
	int m_iVehicleHeatBefore;
	int m_iVehicleHeatAfter;
	int m_iVehicleHeatDelta;
	bool m_bVehicleReportedBefore;
	bool m_bVehicleReportedAfter;
	int m_iVehicleReportedUntilBefore;
	int m_iVehicleReportedUntilAfter;
	int m_iVehicleReportedUntilDelta;
	bool m_bApplied;
}

[BaseContainerProps()]
class HST_CommandReceiptState
{
	string m_sRequestId;
	string m_sActorIdentityId;
	string m_sCommandId;
	string m_sArgument;
	string m_sResult;
	string m_sAggregateId;
	HST_ECampaignCommandStatus m_eStatus = HST_ECampaignCommandStatus.HST_COMMAND_PENDING;
	int m_iReceivedAtSecond;
	int m_iCompletedAtSecond;
}

[BaseContainerProps()]
class HST_ResourceTransactionState
{
	string m_sTransactionId;
	string m_sCommandRequestId;
	string m_sOperationId;
	string m_sQuoteId;
	string m_sManifestId;
	string m_sActorIdentityId;
	string m_sResourceType;
	string m_sReason;
	string m_sLastSettlementId;
	HST_EResourceTransactionStatus m_eStatus = HST_EResourceTransactionStatus.HST_TRANSACTION_RESERVED;
	int m_iAmount;
	int m_iRefundedAmount;
	int m_iCreatedAtSecond;
	int m_iSettledAtSecond;
}

[BaseContainerProps()]
class HST_CampaignEventState
{
	string m_sEventId;
	string m_sCategory;
	string m_sAggregateType;
	string m_sAggregateId;
	string m_sCommandRequestId;
	string m_sTransition;
	string m_sReason;
	int m_iCreatedAtSecond;
}

[BaseContainerProps()]
class HST_PlayerUndercoverState
{
	string m_sIdentityId;
	HST_EUndercoverStatus m_eStatus = HST_EUndercoverStatus.HST_UNDERCOVER_CLEAR;
	int m_iWantedHeat;
	int m_iCompromisedUntilSecond;
	int m_iLastCheckedSecond;
	string m_sLastReason;
	bool m_bUndercoverRequested;
	bool m_bUndercoverApplied;
	bool m_bEnforcementEnabled = true;
	string m_sAppliedMode;
	string m_sLastCompromiseReason;
	string m_sLastDetectionSource;
	string m_sLastEnforcementZoneId;
	int m_iLastEnforcementSecond;
	int m_iLastCompromisedSecond;
	int m_iDetectionScore;
	int m_iRoadblockScanCount;
	int m_iPoliceScanCount;
	bool m_bLastRoadblockScanFailed;
	bool m_bLastPoliceScanFailed;
	bool m_bLastEligibilityResult;
	string m_sLastZoneId;
	string m_sLastEligibilitySummary;
	string m_sClothingReason;
	string m_sWeaponReason;
	string m_sVehicleReason;
	string m_sOffroadReason;
	string m_sEnemyProximityReason;
	string m_sWantedHeatReason;
	int m_iLastEligibilityCheckSecond;
}

[BaseContainerProps()]
class HST_CampaignTaskState
{
	string m_sTaskId;
	string m_sLinkedId;
	string m_sTitle;
	string m_sDescription;
	string m_sCategory;
	vector m_vPosition;
	bool m_bActive = true;
	bool m_bSucceeded;
	bool m_bFailed;
}

[BaseContainerProps()]
class HST_CampaignState
{
	static const int SCHEMA_VERSION = 57;

	int m_iSchemaVersion = SCHEMA_VERSION;
	int m_iLastLoadedSchemaVersion = SCHEMA_VERSION;
	string m_sPresetId = "vanilla_everon";
	int m_iCampaignSeed = 1985;
	HST_ECampaignPhase m_ePhase = HST_ECampaignPhase.HST_CAMPAIGN_SETUP;
	int m_iElapsedSeconds;
	int m_iLastSaveSecond;
	int m_iLastRestoreSecond;
	int m_iPersistenceRestoreSequence;
	int m_iForceSpawnQueueReconciledRestoreSequence;
	int m_iWarLevel = 1;
	int m_iFactionMoney = 1000;
	int m_iHR = 20;
	int m_iTrainingLevel = 1;
	string m_sCampaignEndReason;
	string m_sCampaignEndSummary;
	int m_iCampaignEndedAtSecond;
	int m_iCampaignEndControlPercent;
	int m_iCampaignEndWarLevel;
	int m_iCampaignEndFIAZones;
	int m_iCampaignEndEnemyZones;
	string m_sCampaignEndOutcomeMode;
	int m_iCampaignEndInitialPopulation;
	int m_iCampaignEndRemainingPopulation;
	int m_iCampaignEndKilledPopulation;
	int m_iCampaignEndFIASupportPopulation;
	int m_iCampaignEndSupportPercent;
	int m_iCampaignEndAirfieldsControlled;
	int m_iCampaignEndAirfieldsTotal;
	bool m_bCampaignEndReportGenerated;
	int m_iIncomeAccumulatorSeconds;
	int m_iEnemyResourceAccumulatorSeconds;
	int m_iAggressionAccumulatorSeconds;
	int m_iNextAuthoritySequence = 1;
	string m_sCommanderIdentityId;
	string m_sHQHideoutId;
	vector m_vHQPosition;
	vector m_vPetrosPosition;
	vector m_vHQCachePosition;
	vector m_vArsenalPosition;
	vector m_vHQTentPosition;
	vector m_vHQSpawnPointPosition;
	bool m_bHQDeployed;
	bool m_bHQRuntimeObjectsSpawned;
	bool m_bPetrosAlive = true;
	bool m_bRestoredFromPersistence;
	int m_iPetrosDeaths;
	int m_iHQKnowledge;
	int m_iLastHQAttackSecond;
	int m_iHQThreatLevel;
	int m_iHQKnowledgeLastChangedSecond;
	int m_iLastHQActivitySecond;
	int m_iLastHQThreatScanSecond;
	string m_sLastHQKnowledgeReason;
	string m_sLastHQThreatReason;
	bool m_bDefendPetrosActive;
	string m_sDefendPetrosMissionId;
	string m_sDefendPetrosOrderId;
	string m_sDefendPetrosSupportRequestId;
	string m_sDefendPetrosAttackerGroupId;
	string m_sDefendPetrosStatus = "inactive";
	string m_sDefendPetrosFailureReason;
	int m_iDefendPetrosStartedSecond;
	int m_iDefendPetrosEndsSecond;
	int m_iDefendPetrosLastUpdateSecond;
	int m_iDefendPetrosAttackerCount;
	int m_iDefendPetrosAliveAttackerCount;
	int m_iDefendPetrosKilledCount;
	bool m_bDefendPetrosOutcomeApplied;
	string m_sPetrosPrefab;
	string m_sHQCachePrefab;
	string m_sArsenalPrefab;
	string m_sHQTentPrefab;
	string m_sHQSpawnPointPrefab;
	string m_sLastPersistenceStatus = "not tracked";
	int m_iRuntimeCivilianCharacterCount;
	int m_iRuntimeCivilianVehicleCount;
	int m_iRuntimeMilitaryVehicleCount;
	int m_iRuntimeSpawnFailureCount;
	string m_sLastRuntimeSpawnFailurePrefab;
	string m_sHQArsenalRuntimeStatus = "pending";
	string m_sLastHQArsenalFailure;
	int m_iLastVehicleTargetCandidates;
	string m_sLastVehicleTargetStatus = "not scanned";
	string m_sLastVehicleTargetPrefab;
	string m_sLastVehicleTargetReason;
	float m_fLastVehicleTargetDistanceMeters;
	int m_iLastVehicleTargetCargoEntries;
	string m_sBuildModeStatus = "not active";
	string m_sLastBuildModeFailure;
	string m_sLastBuildModePrefab;
	vector m_vLastBuildModePosition;
	float m_fLastBuildModeYaw;
	string m_sLoadoutEditorStatus = "closed";
	string m_sLastLoadoutEditorFailure;

	ref array<ref HST_FactionPoolState> m_aFactionPools = {};
	ref array<ref HST_PlayerState> m_aPlayers = {};
	ref array<ref HST_ZoneState> m_aZones = {};
	ref array<ref HST_GarrisonState> m_aGarrisons = {};
	ref array<ref HST_ActiveGroupState> m_aActiveGroups = {};
	ref array<ref HST_QRFState> m_aQRFs = {};
	ref array<ref HST_OperationRecordState> m_aOperations = {};
	ref array<ref HST_MapMarkerState> m_aMapMarkers = {};
	ref array<ref HST_ArsenalItemState> m_aArsenalItems = {};
	ref array<ref HST_GarageVehicleState> m_aGarageVehicles = {};
	ref array<ref HST_VehicleCargoItemState> m_aVehicleCargoItems = {};
	ref array<ref HST_RuntimeVehicleState> m_aRuntimeVehicles = {};
	ref array<ref HST_SavedLoadoutState> m_aSavedLoadouts = {};
	ref array<ref HST_IssuedLoadoutItemState> m_aIssuedLoadoutItems = {};
	ref array<ref HST_LoadoutEditorSessionState> m_aLoadoutEditorSessions = {};
	ref array<ref HST_EmplacementState> m_aCapturedEmplacements = {};
	ref array<ref HST_AmmoPointState> m_aAmmoPoints = {};
	ref array<ref HST_ActiveMissionState> m_aActiveMissions = {};
	ref array<ref HST_GeneratedSiteState> m_aGeneratedSites = {};
	ref array<ref HST_GeneratedRouteState> m_aGeneratedRoutes = {};
	ref array<ref HST_MissionObjectiveState> m_aMissionObjectives = {};
	ref array<ref HST_MissionRuntimeEntityState> m_aMissionRuntimeEntities = {};
	ref array<ref HST_MissionAssetState> m_aMissionAssets = {};
	ref array<ref HST_ConvoyElementState> m_aConvoyElements = {};
	ref array<ref HST_SupportRequestState> m_aSupportRequests = {};
	ref array<ref HST_EnemyOrderState> m_aEnemyOrders = {};
	ref array<ref HST_EnemySupportLedgerState> m_aEnemySupportLedgers = {};
	ref array<ref HST_CivilianZoneState> m_aCivilianZones = {};
	ref array<ref HST_TownInfluenceEventState> m_aTownInfluenceEvents = {};
	ref array<ref HST_StrategicEventState> m_aStrategicEvents = {};
	ref array<ref HST_CommandReceiptState> m_aCommandReceipts = {};
	ref array<ref HST_ResourceTransactionState> m_aResourceTransactions = {};
	ref array<ref HST_CampaignEventState> m_aCampaignEvents = {};
	ref array<ref HST_ForceManifestState> m_aForceManifests = {};
	ref array<ref HST_ForceQuoteState> m_aForceQuotes = {};
	ref array<ref HST_ForceSettlementTombstoneState> m_aForceSettlementTombstones = {};
	ref array<ref HST_ForceSpawnResultState> m_aForceSpawnResults = {};
	ref array<ref HST_PlayerUndercoverState> m_aUndercoverPlayers = {};
	ref array<ref HST_CampaignTaskState> m_aCampaignTasks = {};

	HST_FactionPoolState FindFactionPool(string factionKey)
	{
		foreach (HST_FactionPoolState pool : m_aFactionPools)
		{
			if (pool.m_sFactionKey == factionKey)
				return pool;
		}

		return null;
	}

	HST_ZoneState FindZone(string zoneId)
	{
		foreach (HST_ZoneState zone : m_aZones)
		{
			if (zone.m_sZoneId == zoneId)
				return zone;
		}

		return null;
	}

	HST_PlayerState FindPlayer(string identityId)
	{
		foreach (HST_PlayerState player : m_aPlayers)
		{
			if (player.m_sIdentityId == identityId)
				return player;
		}

		return null;
	}

	HST_ArsenalItemState FindArsenalItem(string prefab)
	{
		foreach (HST_ArsenalItemState item : m_aArsenalItems)
		{
			if (item && item.m_sPrefab == prefab)
				return item;
		}

		if (!HST_ArsenalItemEquivalence.IsPaperMapPrefab(prefab))
			return null;

		foreach (HST_ArsenalItemState item : m_aArsenalItems)
		{
			if (item && HST_ArsenalItemEquivalence.AreEquivalentPrefabs(item.m_sPrefab, prefab))
				return item;
		}

		return null;
	}

	HST_GarageVehicleState FindGarageVehicle(string vehicleId)
	{
		foreach (HST_GarageVehicleState vehicle : m_aGarageVehicles)
		{
			if (vehicle.m_sVehicleId == vehicleId)
				return vehicle;
		}

		return null;
	}

	HST_RuntimeVehicleState FindRuntimeVehicle(string vehicleRuntimeId)
	{
		foreach (HST_RuntimeVehicleState vehicle : m_aRuntimeVehicles)
		{
			if (vehicle && vehicle.m_sVehicleRuntimeId == vehicleRuntimeId)
				return vehicle;
		}

		return null;
	}

	bool RemoveRuntimeVehicle(string vehicleRuntimeId)
	{
		if (vehicleRuntimeId.IsEmpty())
			return false;

		for (int i = m_aRuntimeVehicles.Count() - 1; i >= 0; i--)
		{
			HST_RuntimeVehicleState vehicle = m_aRuntimeVehicles[i];
			if (!vehicle || vehicle.m_sVehicleRuntimeId != vehicleRuntimeId)
				continue;

			m_aRuntimeVehicles.Remove(i);
			return true;
		}

		return false;
	}

	HST_VehicleCargoItemState FindVehicleCargoItem(string vehicleRuntimeId, string itemPrefab)
	{
		foreach (HST_VehicleCargoItemState cargoItem : m_aVehicleCargoItems)
		{
			if (cargoItem.m_sVehicleRuntimeId == vehicleRuntimeId && cargoItem.m_sItemPrefab == itemPrefab)
				return cargoItem;
		}

		return null;
	}

	HST_SavedLoadoutState FindSavedLoadout(string ownerIdentityId, string loadoutId)
	{
		foreach (HST_SavedLoadoutState loadout : m_aSavedLoadouts)
		{
			if (loadout && loadout.m_sOwnerIdentityId == ownerIdentityId && loadout.m_sLoadoutId == loadoutId)
				return loadout;
		}

		return null;
	}

	HST_SavedLoadoutState FindFirstSavedLoadout(string ownerIdentityId)
	{
		foreach (HST_SavedLoadoutState loadout : m_aSavedLoadouts)
		{
			if (loadout && loadout.m_sOwnerIdentityId == ownerIdentityId)
				return loadout;
		}

		return null;
	}

	HST_IssuedLoadoutItemState FindIssuedLoadoutItem(string ownerIdentityId, string itemPrefab)
	{
		foreach (HST_IssuedLoadoutItemState issuedItem : m_aIssuedLoadoutItems)
		{
			if (issuedItem && issuedItem.m_sOwnerIdentityId == ownerIdentityId && issuedItem.m_sItemPrefab == itemPrefab)
				return issuedItem;
		}

		if (!HST_ArsenalItemEquivalence.IsPaperMapPrefab(itemPrefab))
			return null;

		foreach (HST_IssuedLoadoutItemState issuedItem : m_aIssuedLoadoutItems)
		{
			if (issuedItem && issuedItem.m_sOwnerIdentityId == ownerIdentityId && HST_ArsenalItemEquivalence.AreEquivalentPrefabs(issuedItem.m_sItemPrefab, itemPrefab))
				return issuedItem;
		}

		return null;
	}

	HST_LoadoutEditorSessionState FindLoadoutEditorSession(string ownerIdentityId)
	{
		foreach (HST_LoadoutEditorSessionState session : m_aLoadoutEditorSessions)
		{
			if (session && session.m_sOwnerIdentityId == ownerIdentityId)
				return session;
		}

		return null;
	}

	HST_GarrisonState FindGarrison(string zoneId, string factionKey)
	{
		foreach (HST_GarrisonState garrison : m_aGarrisons)
		{
			if (!garrison)
				continue;
			if (garrison.m_sZoneId == zoneId && garrison.m_sFactionKey == factionKey)
			{
				if (garrison.m_sGarrisonId.IsEmpty())
					garrison.m_sGarrisonId = HST_StableIdService.BuildGarrisonId(zoneId, factionKey);
				return garrison;
			}
		}

		return null;
	}

	HST_ActiveGroupState FindActiveGroup(string groupId)
	{
		foreach (HST_ActiveGroupState group : m_aActiveGroups)
		{
			if (group.m_sGroupId == groupId)
				return group;
		}

		return null;
	}

	bool IsOperationalActiveGroup(HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		if (IsQuarantinedActiveGroup(group))
			return false;
		HST_ActiveMissionState mission;
		if (!group.m_sMissionInstanceId.IsEmpty())
			mission = FindActiveMission(group.m_sMissionInstanceId);
		HST_OperationRecordState operation;
		if (!group.m_sOperationId.IsEmpty())
			operation = FindOperation(group.m_sOperationId);
		if (HST_MissionGuardOperationService.IsMissionGuardGroupClaimant(this, group))
		{
			int expectedGuardContract;
			if (mission)
				expectedGuardContract = HST_MissionGuardOperationService.ResolveExpectedContractVersion(
					mission.m_sMissionId);
			if (!HST_MissionGuardOperationService.IsExactMissionGuardGroup(this, group)
				|| !mission || !operation
				|| !HST_MissionGuardOperationService.IsExactMission(mission)
				|| expectedGuardContract <= 0
				|| mission.m_eStatus != HST_EMissionStatus.HST_MISSION_ACTIVE
				|| operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_GUARD
				|| operation.m_iContractVersion != expectedGuardContract
				|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
				|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
				return false;
			return mission.m_sOperationId == operation.m_sOperationId
				&& operation.m_sGroupId == group.m_sGroupId;
		}
		bool exactMissionClaim = mission && mission.m_sRuntimePrimitive == "convoy_intercept"
			&& mission.m_iOperationContractVersion != 0;
		bool exactOperationClaim = operation && operation.m_eType == HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY;
		bool exactElementClaim = !group.m_sConvoyElementId.IsEmpty();
		bool convoyIdClaim = group.m_sGroupId.StartsWith("mission_convoy_");
		if (!exactMissionClaim && !exactOperationClaim && !exactElementClaim)
		{
			if (!convoyIdClaim)
				return true;
			return mission && mission.m_sRuntimePrimitive == "convoy_intercept"
				&& mission.m_iOperationContractVersion == 0
				&& mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE;
		}

		int missionClaimants;
		foreach (HST_ActiveMissionState candidateMission : m_aActiveMissions)
		{
			if (candidateMission && candidateMission.m_sInstanceId == group.m_sMissionInstanceId)
				missionClaimants++;
		}
		int operationClaimants;
		foreach (HST_OperationRecordState candidateOperation : m_aOperations)
		{
			if (!candidateOperation || candidateOperation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY)
				continue;
			if (candidateOperation.m_sOperationId == group.m_sOperationId
				|| candidateOperation.m_sMissionInstanceId == group.m_sMissionInstanceId)
				operationClaimants++;
		}
		int groupClaimants;
		foreach (HST_ActiveGroupState candidateGroup : m_aActiveGroups)
		{
			if (!candidateGroup)
				continue;
			if (candidateGroup.m_sGroupId == group.m_sGroupId
				|| (!group.m_sConvoyElementId.IsEmpty() && candidateGroup.m_sConvoyElementId == group.m_sConvoyElementId))
				groupClaimants++;
		}
		HST_ConvoyElementState element;
		int elementClaimants;
		foreach (HST_ConvoyElementState candidateElement : m_aConvoyElements)
		{
			if (!candidateElement)
				continue;
			if (candidateElement.m_sElementId == group.m_sConvoyElementId
				|| candidateElement.m_sGroupId == group.m_sGroupId)
			{
				element = candidateElement;
				elementClaimants++;
			}
		}

		if (missionClaimants != 1 || operationClaimants != 1 || groupClaimants != 1 || elementClaimants != 1
			|| !element || element.m_sOperationId != group.m_sOperationId
			|| element.m_sMissionInstanceId != group.m_sMissionInstanceId || element.m_sGroupId != group.m_sGroupId
			|| !operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY
			|| operation.m_sMissionInstanceId != group.m_sMissionInstanceId
			|| operation.m_eSettlementState != HST_EOperationSettlementState.HST_OPERATION_SETTLEMENT_OPEN
			|| operation.m_eTerminalResult != HST_EOperationTerminalResult.HST_OPERATION_TERMINAL_NONE)
			return false;
		return mission && mission.m_eStatus == HST_EMissionStatus.HST_MISSION_ACTIVE
			&& mission.m_sRuntimePrimitive == "convoy_intercept"
			&& mission.m_sOperationId == operation.m_sOperationId
			&& mission.m_iOperationContractVersion == 1
			&& operation.m_iContractVersion == 1
			&& !group.m_sConvoyElementId.IsEmpty();
	}

	bool IsQuarantinedActiveGroup(HST_ActiveGroupState group)
	{
		if (!group)
			return false;
		if (group.m_sRuntimeStatus == HST_MissionGuardOperationService.QUARANTINE_STATUS
			|| group.m_sRuntimeStatus == "exact_garrison_patrol_quarantined"
			|| group.m_sRuntimeStatus == "exact_patrol_quarantined"
			|| group.m_sRuntimeStatus == "exact_patrol_orphan_quarantined"
			|| group.m_sRuntimeStatus == "exact_runtime_authority_quarantined")
			return true;
		return group.m_sSpawnFallbackMode == HST_MissionGuardOperationService.QUARANTINE_STATUS
			|| group.m_sSpawnFallbackMode == "exact_garrison_patrol_quarantined"
			|| group.m_sSpawnFallbackMode == "exact_enemy_patrol_quarantined";
	}

	int CountOperationalActiveGroups()
	{
		int count;
		foreach (HST_ActiveGroupState group : m_aActiveGroups)
		{
			if (IsOperationalActiveGroup(group))
				count++;
		}
		return count;
	}

	bool IsCombatPresentActiveGroup(HST_ActiveGroupState group)
	{
		if (!IsOperationalActiveGroup(group))
			return false;
		if (group.m_sConvoyElementId.IsEmpty())
			return true;
		HST_OperationRecordState operation = FindOperation(group.m_sOperationId);
		if (!operation || operation.m_eType != HST_EOperationType.HST_OPERATION_TYPE_MISSION_CONVOY)
			return true;
		HST_ConvoyElementState element = FindConvoyElement(group.m_sConvoyElementId);
		return element && element.m_iSurvivingCrewCount > 0
			&& element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_ABANDONED
			&& element.m_eDisposition != HST_EConvoyElementDisposition.HST_CONVOY_ELEMENT_DISPOSITION_RETIRED;
	}

	HST_QRFState FindActiveQRF(string targetZoneId, string factionKey)
	{
		foreach (HST_QRFState qrf : m_aQRFs)
		{
			if (!qrf.m_bResolved && qrf.m_sTargetZoneId == targetZoneId && qrf.m_sFactionKey == factionKey)
				return qrf;
		}

		return null;
	}

	HST_ActiveMissionState FindActiveMission(string instanceId)
	{
		foreach (HST_ActiveMissionState mission : m_aActiveMissions)
		{
			if (mission.m_sInstanceId == instanceId)
				return mission;
		}

		return null;
	}

	HST_MapMarkerState FindMapMarker(string markerId)
	{
		foreach (HST_MapMarkerState marker : m_aMapMarkers)
		{
			if (marker.m_sMarkerId == markerId)
				return marker;
		}

		return null;
	}

	HST_GeneratedSiteState FindGeneratedSite(string siteId)
	{
		foreach (HST_GeneratedSiteState site : m_aGeneratedSites)
		{
			if (site.m_sSiteId == siteId)
				return site;
		}

		return null;
	}

	HST_GeneratedRouteState FindGeneratedRoute(string routeId)
	{
		foreach (HST_GeneratedRouteState route : m_aGeneratedRoutes)
		{
			if (route.m_sRouteId == routeId)
				return route;
		}

		return null;
	}

	HST_SupportRequestState FindSupportRequest(string requestId)
	{
		foreach (HST_SupportRequestState request : m_aSupportRequests)
		{
			if (request.m_sRequestId == requestId)
				return request;
		}

		return null;
	}

	HST_EnemySupportLedgerState FindEnemySupportLedger(string factionKey, string zoneId)
	{
		foreach (HST_EnemySupportLedgerState ledger : m_aEnemySupportLedgers)
		{
			if (ledger && ledger.m_sFactionKey == factionKey && ledger.m_sZoneId == zoneId)
				return ledger;
		}

		return null;
	}

	HST_CivilianZoneState FindCivilianZone(string zoneId)
	{
		foreach (HST_CivilianZoneState civilianZone : m_aCivilianZones)
		{
			if (civilianZone.m_sZoneId == zoneId)
				return civilianZone;
		}

		return null;
	}

	HST_TownInfluenceEventState FindTownInfluenceEvent(string eventId)
	{
		foreach (HST_TownInfluenceEventState eventState : m_aTownInfluenceEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return eventState;
		}

		return null;
	}

	HST_StrategicEventState FindStrategicEvent(string eventId)
	{
		foreach (HST_StrategicEventState eventState : m_aStrategicEvents)
		{
			if (eventState && eventState.m_sEventId == eventId)
				return eventState;
		}

		return null;
	}

	HST_OperationRecordState FindOperation(string operationId)
	{
		if (operationId.IsEmpty())
			return null;

		foreach (HST_OperationRecordState operation : m_aOperations)
		{
			if (operation && operation.m_sOperationId == operationId)
				return operation;
		}

		return null;
	}

	HST_EnemyOrderState FindEnemyOrder(string orderId)
	{
		if (orderId.IsEmpty())
			return null;

		foreach (HST_EnemyOrderState order : m_aEnemyOrders)
		{
			if (order && order.m_sOrderId == orderId)
				return order;
		}

		return null;
	}

	HST_ForceManifestState FindForceManifest(string manifestId)
	{
		foreach (HST_ForceManifestState manifest : m_aForceManifests)
		{
			if (manifest && manifest.m_sManifestId == manifestId)
				return manifest;
		}

		return null;
	}

	HST_ForceQuoteState FindForceQuote(string quoteId)
	{
		foreach (HST_ForceQuoteState quote : m_aForceQuotes)
		{
			if (quote && quote.m_sQuoteId == quoteId)
				return quote;
		}

		return null;
	}

	HST_ForceSettlementTombstoneState FindForceSettlementTombstone(string quoteId)
	{
		foreach (HST_ForceSettlementTombstoneState tombstone : m_aForceSettlementTombstones)
		{
			if (tombstone && tombstone.m_sQuoteId == quoteId)
				return tombstone;
		}
		return null;
	}

	HST_ForceSettlementTombstoneState FindForceSettlementTombstoneByCommandRequest(string commandRequestId)
	{
		if (commandRequestId.IsEmpty())
			return null;
		foreach (HST_ForceSettlementTombstoneState tombstone : m_aForceSettlementTombstones)
		{
			if (tombstone && tombstone.m_sCommandRequestId == commandRequestId)
				return tombstone;
		}
		return null;
	}

	HST_ForceSettlementTombstoneState FindForceSettlementTombstoneByTransaction(string transactionId)
	{
		if (transactionId.IsEmpty())
			return null;
		foreach (HST_ForceSettlementTombstoneState tombstone : m_aForceSettlementTombstones)
		{
			if (tombstone && tombstone.FindTransaction(transactionId))
				return tombstone;
		}
		return null;
	}

	int CountForceSettlementTombstonesByTransaction(string transactionId)
	{
		int count;
		if (transactionId.IsEmpty())
			return count;
		foreach (HST_ForceSettlementTombstoneState tombstone : m_aForceSettlementTombstones)
		{
			if (tombstone && tombstone.FindTransaction(transactionId))
				count++;
		}
		return count;
	}

	HST_ForceSpawnResultState FindForceSpawnResult(string resultId)
	{
		foreach (HST_ForceSpawnResultState spawnResult : m_aForceSpawnResults)
		{
			if (spawnResult && spawnResult.m_sResultId == resultId)
				return spawnResult;
		}

		return null;
	}

	HST_ForceSpawnResultState FindForceSpawnResultByManifest(string manifestId)
	{
		if (manifestId.IsEmpty())
			return null;

		HST_ForceSpawnResultState match;
		foreach (HST_ForceSpawnResultState spawnResult : m_aForceSpawnResults)
		{
			if (!spawnResult || spawnResult.m_sManifestId != manifestId)
				continue;
			if (match)
				return null;

			match = spawnResult;
		}

		return match;
	}

	array<ref HST_ForceSpawnResultState> FindForceSpawnResultsByManifest(string manifestId)
	{
		array<ref HST_ForceSpawnResultState> matches = {};
		if (manifestId.IsEmpty())
			return matches;

		foreach (HST_ForceSpawnResultState spawnResult : m_aForceSpawnResults)
		{
			if (spawnResult && spawnResult.m_sManifestId == manifestId)
				matches.Insert(spawnResult);
		}

		return matches;
	}

	HST_ForceSpawnResultState FindForceSpawnResultByRequest(string requestId)
	{
		if (requestId.IsEmpty())
			return null;

		foreach (HST_ForceSpawnResultState spawnResult : m_aForceSpawnResults)
		{
			if (spawnResult && spawnResult.m_sRequestId == requestId)
				return spawnResult;
		}

		return null;
	}

	HST_ForceSpawnResultState FindForceSpawnResultByProjection(string projectionId)
	{
		if (projectionId.IsEmpty())
			return null;

		foreach (HST_ForceSpawnResultState spawnResult : m_aForceSpawnResults)
		{
			if (spawnResult && spawnResult.m_sProjectionId == projectionId)
				return spawnResult;
		}

		return null;
	}

	HST_CommandReceiptState FindCommandReceipt(string requestId)
	{
		foreach (HST_CommandReceiptState receipt : m_aCommandReceipts)
		{
			if (receipt && receipt.m_sRequestId == requestId)
				return receipt;
		}

		return null;
	}

	HST_ResourceTransactionState FindResourceTransaction(string transactionId)
	{
		foreach (HST_ResourceTransactionState transaction : m_aResourceTransactions)
		{
			if (transaction && transaction.m_sTransactionId == transactionId)
				return transaction;
		}

		return null;
	}

	HST_PlayerUndercoverState FindUndercoverPlayer(string identityId)
	{
		foreach (HST_PlayerUndercoverState undercover : m_aUndercoverPlayers)
		{
			if (undercover.m_sIdentityId == identityId)
				return undercover;
		}

		return null;
	}

	HST_CampaignTaskState FindCampaignTask(string taskId)
	{
		foreach (HST_CampaignTaskState task : m_aCampaignTasks)
		{
			if (task.m_sTaskId == taskId)
				return task;
		}

		return null;
	}

	HST_MissionRuntimeEntityState FindMissionRuntimeEntity(string runtimeEntityId)
	{
		foreach (HST_MissionRuntimeEntityState runtimeEntity : m_aMissionRuntimeEntities)
		{
			if (runtimeEntity && runtimeEntity.m_sRuntimeEntityId == runtimeEntityId)
				return runtimeEntity;
		}

		return null;
	}

	HST_MissionAssetState FindMissionAsset(string assetId)
	{
		foreach (HST_MissionAssetState asset : m_aMissionAssets)
		{
			if (asset && asset.m_sAssetId == assetId)
				return asset;
		}

		return null;
	}

	HST_ConvoyElementState FindConvoyElement(string elementId)
	{
		foreach (HST_ConvoyElementState element : m_aConvoyElements)
		{
			if (element && element.m_sElementId == elementId)
				return element;
		}

		return null;
	}

	int CountMissionAssets(string instanceId, string role = "")
	{
		int count;
		foreach (HST_MissionAssetState asset : m_aMissionAssets)
		{
			if (!asset || asset.m_sMissionInstanceId != instanceId)
				continue;
			if (!role.IsEmpty() && asset.m_sRole != role)
				continue;

			count++;
		}

		return count;
	}
}
