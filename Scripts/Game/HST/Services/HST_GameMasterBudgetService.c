class HST_GameMasterBudgetService
{
	static const int BUDGET_HEADROOM_MULTIPLIER = 500;
	static const int BUDGET_REPAIR_FALLBACK_ORIGINAL = 100;

	protected static bool s_bGameMasterBudgetsEnabled;
	protected static bool s_bLoggedGameMasterBudgetState;
	protected static ref set<EEditableEntityBudget> s_aManagedBudgetTypes;
	protected static int s_iDisabledBudgetPreSubtractRepairs;
	protected static bool s_bLoggedDisabledBudgetPreSubtractRepair;

	static void EnsureManagedBudgetTypes()
	{
		if (s_aManagedBudgetTypes)
			return;

		s_aManagedBudgetTypes = new set<EEditableEntityBudget>();
		s_aManagedBudgetTypes.Insert(EEditableEntityBudget.PROPS);
		s_aManagedBudgetTypes.Insert(EEditableEntityBudget.AI);
		s_aManagedBudgetTypes.Insert(EEditableEntityBudget.VEHICLES);
		s_aManagedBudgetTypes.Insert(EEditableEntityBudget.WAYPOINTS);
		s_aManagedBudgetTypes.Insert(EEditableEntityBudget.SYSTEMS);
	}

	static bool AreGameMasterBudgetsEnabled()
	{
		return s_bGameMasterBudgetsEnabled;
	}

	static bool IsManagedBudgetType(EEditableEntityBudget budgetType)
	{
		EnsureManagedBudgetTypes();
		return s_aManagedBudgetTypes.Contains(budgetType);
	}

	static int ResolveDisabledBudgetHeadroom(int originalBudget)
	{
		return Math.Max(originalBudget, 1) * BUDGET_HEADROOM_MULTIPLIER;
	}

	static int ResolveDisabledBudgetRepairHeadroom(int pendingBudgetCost = 0)
	{
		int fallbackHeadroom = ResolveDisabledBudgetHeadroom(BUDGET_REPAIR_FALLBACK_ORIGINAL);
		int pendingHeadroom = Math.Max(pendingBudgetCost, 0) + BUDGET_HEADROOM_MULTIPLIER;
		return Math.Max(fallbackHeadroom, pendingHeadroom);
	}

	static void NoteDisabledBudgetPreSubtractRepair(EEditableEntityBudget budgetType, int currentBudget, int targetBudget, int pendingBudgetCost)
	{
		s_iDisabledBudgetPreSubtractRepairs++;
		if (s_bLoggedDisabledBudgetPreSubtractRepair)
			return;

		s_bLoggedDisabledBudgetPreSubtractRepair = true;
		Print(string.Format("Partisan game master budgets | restored disabled-budget headroom before native subtract type=%1 current=%2 target=%3 pending=%4", typename.EnumToString(EEditableEntityBudget, budgetType), currentBudget, targetBudget, pendingBudgetCost));
	}

	static int CountDisabledBudgetPreSubtractRepairs()
	{
		return s_iDisabledBudgetPreSubtractRepairs;
	}

	static bool ReadConfiguredBudgetState()
	{
		HST_RuntimeSettingsService settingsService = new HST_RuntimeSettingsService();
		HST_RuntimeSettings settings = settingsService.LoadOrCreate();
		if (!settings || !settings.m_Features)
			return false;

		return settings.m_Features.m_bGameMasterBudgetsEnabled;
	}

	static void SetGameMasterBudgetsEnabled(bool enabled, string source = "runtime")
	{
		EnsureManagedBudgetTypes();

		bool changed = s_bGameMasterBudgetsEnabled != enabled;
		s_bGameMasterBudgetsEnabled = enabled;

		SCR_BudgetEditorComponent budgetEditor = SCR_BudgetEditorComponent.Cast(SCR_BudgetEditorComponent.GetInstance(SCR_BudgetEditorComponent, false, true));
		if (budgetEditor)
			budgetEditor.HistasiRefreshGameMasterBudgetCaps();

		if (changed || !s_bLoggedGameMasterBudgetState)
		{
			s_bLoggedGameMasterBudgetState = true;
			Print(string.Format("Partisan game master budgets | enabled=%1 source=%2", enabled, source));
		}
	}
}

modded class SCR_BaseGameMode
{
	static const int CAMPAIGN_END_CHECKPOINT_RETRY_DELAY_MS = 1000;
	static const int CAMPAIGN_END_TRANSITION_POLL_DELAY_MS = 100;
	static const int CAMPAIGN_END_CHECKPOINT_LOG_INTERVAL = 30;
	static const string CAMPAIGN_END_PLAYER_RELEASE_EVIDENCE
		= "active-group controlled-shutdown player release required";
	// One ordinary checkpoint may already be in flight when controlled end starts.
	// Allow that request's 120-second timeout, a full shutdown checkpoint timeout,
	// and bounded queue/transition margin before failing closed under quiescence.
	static const int CAMPAIGN_END_CHECKPOINT_RETRY_WINDOW_MS = 270000;

	[RplProp(onRplName: "OnHistasiGameMasterBudgetsReplicated")]
	protected bool m_bHSTGameMasterBudgetsEnabled;
	protected bool m_bHSTCampaignEndRequested;
	protected bool m_bHSTCampaignEndCheckpointPending;
	protected bool m_bHSTCampaignEndCheckpointCommitted;
	protected bool m_bHSTCampaignEndCheckpointRetryQueued;
	protected bool m_bHSTCampaignEndTransitionPollQueued;
	protected bool m_bHSTCampaignEndCheckpointBlocked;
	protected bool m_bHSTCampaignEndWaitingForPlayerRelease;
	protected bool m_bHSTPreserveCampaignSessionDataOnEnd;
	protected bool m_bHSTUseProfileFallbackOnEnd;
	protected bool m_bHSTCampaignEndPendingNativeAuthority;
	protected int m_iHSTCampaignEndCheckpointAttempts;
	protected int m_iHSTCampaignEndCheckpointStartedTick;
	protected ref SCR_GameModeEndData m_HSTPendingCampaignEndData;
	protected ref SaveGameOperationCallback m_HSTCampaignEndCheckpointCallback;
	protected HST_PersistenceCheckpointRequest
		m_HSTCampaignEndCheckpointRequest;

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (Replication.IsServer())
		{
			m_bHSTGameMasterBudgetsEnabled = HST_GameMasterBudgetService.ReadConfiguredBudgetState();
			Replication.BumpMe();
		}

		ApplyHistasiGameMasterBudgetState("game-mode init");
	}

	bool AreHistasiGameMasterBudgetsEnabled()
	{
		return m_bHSTGameMasterBudgetsEnabled;
	}

	void SetHistasiGameMasterBudgetsEnabled(bool enabled, string source = "runtime")
	{
		m_bHSTGameMasterBudgetsEnabled = enabled;

		if (Replication.IsServer())
			Replication.BumpMe();

		ApplyHistasiGameMasterBudgetState(source);
	}

	protected void OnHistasiGameMasterBudgetsReplicated()
	{
		ApplyHistasiGameMasterBudgetState("replication");
	}

	protected void ApplyHistasiGameMasterBudgetState(string source)
	{
		HST_GameMasterBudgetService.SetGameMasterBudgetsEnabled(m_bHSTGameMasterBudgetsEnabled, source);
	}

	// Read-only controlled-end telemetry for the ordinary persistence proof.
	// These accessors do not trigger, retry, reset, or otherwise mutate the
	// production shutdown state machine.
	int HSTGetControlledCampaignEndCheckpointAttempts()
	{
		return m_iHSTCampaignEndCheckpointAttempts;
	}

	bool HSTIsControlledCampaignEndRequested()
	{
		return m_bHSTCampaignEndRequested;
	}

	bool HSTIsControlledCampaignEndCheckpointPending()
	{
		return m_bHSTCampaignEndCheckpointPending;
	}

	bool HSTIsControlledCampaignEndCheckpointCommitted()
	{
		return m_bHSTCampaignEndCheckpointCommitted;
	}

	bool HSTIsControlledCampaignEndWaitingForPlayerRelease()
	{
		return m_bHSTCampaignEndWaitingForPlayerRelease;
	}

	bool HSTAreControlledCampaignEndControlsAllowed()
	{
		return m_bAllowControls;
	}

	bool HSTIsControlledCampaignEndCheckpointBlocked()
	{
		return m_bHSTCampaignEndCheckpointBlocked;
	}

	// Partisan-owned game-mode completion is a controllable shutdown boundary.
	// Keep the game in its running state until the typed callback-gated checkpoint has
	// committed, then re-enter the stock transition. External process termination
	// has no delayable script callback and therefore remains protected only by the
	// latest already-completed checkpoint.
	override void EndGameMode(SCR_GameModeEndData endData)
	{
		if (m_bHSTCampaignEndCheckpointCommitted)
		{
			ClearHSTCampaignEndCheckpointState();
			super.EndGameMode(endData);
			return;
		}

		if (!IsMaster() || !Replication.IsServer())
		{
			super.EndGameMode(endData);
			return;
		}

		HST_CampaignCoordinatorComponent coordinator
			= HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			// This modded engine class can exist in non-Partisan scenarios. Do not
			// change their stock end-game behavior when no campaign coordinator owns
			// the world.
			super.EndGameMode(endData);
			return;
		}
#ifdef ENABLE_DIAG
		if (coordinator.IsCampaignDebugStateIsolationActive())
		{
			super.EndGameMode(endData);
			return;
		}
#endif

		if (!IsRunning())
		{
			super.EndGameMode(endData);
			return;
		}

		if (!m_bHSTCampaignEndRequested)
		{
			m_bHSTCampaignEndRequested = true;
			SetHSTCampaignEndWaitingForPlayerRelease(false);
			m_bHSTPreserveCampaignSessionDataOnEnd = false;
			m_bHSTUseProfileFallbackOnEnd = false;
			m_HSTPendingCampaignEndData = endData;
			m_iHSTCampaignEndCheckpointStartedTick = System.GetTickCount();
			if (m_bAllowControls)
			{
				m_bAllowControls = false;
				Replication.BumpMe();
			}
			coordinator.BeginControlledCampaignEndCheckpointAttempt();
			Print(
				"Partisan controlled campaign end | draining one bounded interval before checkpoint capture");
			QueueHSTCampaignEndCheckpointRetry(
				"initial campaign-end drain interval");
			return;
		}
		if (m_bHSTCampaignEndCheckpointPending
			|| m_bHSTCampaignEndCheckpointRetryQueued
			|| m_bHSTCampaignEndTransitionPollQueued
			|| m_bHSTCampaignEndCheckpointBlocked)
			return;

		TryHSTCampaignEndCheckpoint(coordinator);
	}

	protected void TryHSTCampaignEndCheckpoint(
		HST_CampaignCoordinatorComponent coordinator)
	{
		if (!m_bHSTCampaignEndRequested || !coordinator)
			return;

		// A rejected player-release preflight is the only controlled-end state
		// that temporarily returns gameplay controls. Re-block controls before
		// every retry so no player can enter a sampled authority while the next
		// snapshot is being admitted.
		SetHSTCampaignEndWaitingForPlayerRelease(false);
		m_iHSTCampaignEndCheckpointAttempts++;
		coordinator.BeginControlledCampaignEndCheckpointAttempt();
		m_bHSTCampaignEndPendingNativeAuthority = false;
		m_bHSTCampaignEndCheckpointPending = true;
		m_HSTCampaignEndCheckpointRequest = null;
		m_HSTCampaignEndCheckpointCallback = new SaveGameOperationCallback(
			OnHSTCampaignEndCheckpointCompleted);
		HST_PersistenceCheckpointRequest request
			= coordinator.RequestGracefulShutdownCheckpoint(
				m_HSTCampaignEndCheckpointCallback);
		m_HSTCampaignEndCheckpointRequest = request;
		coordinator.ObserveControlledCampaignEndCheckpointRequest(request);
		bool durableRequestAccepted = false;
		if (request)
		{
			durableRequestAccepted = request.m_bSavePointRequested
				|| request.m_bProfileFallbackSaved;
		}
		if (durableRequestAccepted
			&& !coordinator.ArmControlledCampaignEndCheckpointQuiescence())
		{
			Print(
				"Partisan controlled campaign end | checkpoint queued but stability fingerprint was not armed",
				LogLevel.WARNING);
		}

		if (request && request.m_bSavePointRequested)
		{
			if (m_iHSTCampaignEndCheckpointAttempts == 1)
			{
				Print(
					"Partisan controlled campaign end | callback-gated shutdown checkpoint requested");
			}
			return;
		}

		m_bHSTCampaignEndCheckpointPending = false;
		m_HSTCampaignEndCheckpointCallback = null;
		if (request && request.m_bProfileFallbackSaved)
		{
			// Native persistence is unavailable, so the verified synchronous
			// profile mirror is the complete durability boundary for this
			// environment.
			if (!coordinator.IsControlledCampaignEndCheckpointStable())
			{
				QueueHSTCampaignEndCheckpointRetry(
					coordinator.GetControlledCampaignEndStabilityEvidence());
				return;
			}
			ContinueHSTCampaignEndAfterCheckpoint(false);
			return;
		}

		string failureEvidence = "checkpoint service unavailable";
		if (request)
			failureEvidence = request.m_sEvidence;
		if (IsHSTCampaignEndPlayerReleaseRequired(request))
		{
			SetHSTCampaignEndWaitingForPlayerRelease(true);
			QueueHSTCampaignEndCheckpointRetry(failureEvidence);
			return;
		}
		QueueHSTCampaignEndCheckpointRetry(failureEvidence);
	}

	protected bool IsHSTCampaignEndPlayerReleaseRequired(
		HST_PersistenceCheckpointRequest request)
	{
		return request
			&& !request.m_bControlledShutdownRuntimeQuiescenceApplied
			&& (request.m_bControlledShutdownPlayerReleaseRequired
				|| (!request.m_sEvidence.IsEmpty()
					&& request.m_sEvidence.Contains(
						CAMPAIGN_END_PLAYER_RELEASE_EVIDENCE)));
	}

	protected void SetHSTCampaignEndWaitingForPlayerRelease(bool waiting)
	{
		bool allowControls = waiting && super.GetAllowControlsTarget();
		bool changed = m_bHSTCampaignEndWaitingForPlayerRelease != waiting
			|| m_bAllowControls != allowControls;
		m_bHSTCampaignEndWaitingForPlayerRelease = waiting;
		m_bAllowControls = allowControls;
		if (changed && Replication.IsServer())
			Replication.BumpMe();
	}

	protected void OnHSTCampaignEndCheckpointCompleted(bool success)
	{
		if (!m_bHSTCampaignEndRequested
			|| !m_bHSTCampaignEndCheckpointPending)
			return;
		// A singleplayer blocking shutdown request may invoke its completion observer before
		// RequestGracefulShutdownCheckpoint returns its request DTO. Defer that
		// synchronous edge so the exact native/journal evidence is available.
		if (!m_HSTCampaignEndCheckpointRequest)
		{
			ScriptCallQueue callQueue = GetGame().GetCallqueue();
			if (callQueue)
			{
				callQueue.CallLater(
					OnHSTCampaignEndCheckpointCompleted,
					0,
					false,
					success);
				return;
			}
		}

		m_bHSTCampaignEndCheckpointPending = false;
		m_HSTCampaignEndCheckpointCallback = null;
		HST_CampaignCoordinatorComponent coordinator
			= HST_CampaignCoordinatorComponent.GetInstance();
		if (!success)
		{
			string completionEvidence
				= "native checkpoint or profile mirror completion failed";
			if (m_HSTCampaignEndCheckpointRequest
				&& !m_HSTCampaignEndCheckpointRequest.m_sEvidence.IsEmpty())
			{
				completionEvidence += " | "
					+ m_HSTCampaignEndCheckpointRequest.m_sEvidence;
			}
			if (coordinator)
			{
#ifdef ENABLE_DIAG
				coordinator.RejectControlledCampaignEndBridgeProof(
					completionEvidence);
#endif
			}
			QueueHSTCampaignEndCheckpointRetry(
				completionEvidence);
			return;
		}
		if (!coordinator
			|| !coordinator.IsControlledCampaignEndCheckpointStable())
		{
			string stabilityEvidence
				= "campaign coordinator unavailable after checkpoint commit";
			if (coordinator)
			{
				stabilityEvidence
					= coordinator.GetControlledCampaignEndStabilityEvidence();
#ifdef ENABLE_DIAG
				coordinator.RejectControlledCampaignEndBridgeProof(
					stabilityEvidence);
#endif
			}
			QueueHSTCampaignEndCheckpointRetry(
				stabilityEvidence);
			return;
		}

		Print(
			"Partisan controlled campaign end | shutdown checkpoint committed; continuing engine transition");
		ContinueHSTCampaignEndAfterCheckpoint(true);
	}

	protected void QueueHSTCampaignEndCheckpointRetry(string evidence)
	{
		if (!m_bHSTCampaignEndRequested
			|| m_bHSTCampaignEndCheckpointRetryQueued)
			return;
		if (System.GetTickCount(m_iHSTCampaignEndCheckpointStartedTick)
			>= CAMPAIGN_END_CHECKPOINT_RETRY_WINDOW_MS)
		{
			BlockHSTCampaignEndCheckpoint(evidence);
			return;
		}

		if (m_iHSTCampaignEndCheckpointAttempts == 1
			|| m_iHSTCampaignEndCheckpointAttempts
				% CAMPAIGN_END_CHECKPOINT_LOG_INTERVAL == 0)
		{
			Print(string.Format(
				"Partisan controlled campaign end | checkpoint deferred; retry %1 | %2",
				m_iHSTCampaignEndCheckpointAttempts,
				evidence),
				LogLevel.WARNING);
		}

		m_bHSTCampaignEndCheckpointRetryQueued = true;
		GetGame().GetCallqueue().CallLater(
			RetryHSTCampaignEndCheckpoint,
			CAMPAIGN_END_CHECKPOINT_RETRY_DELAY_MS,
			false);
	}

	protected void RetryHSTCampaignEndCheckpoint()
	{
		m_bHSTCampaignEndCheckpointRetryQueued = false;
		if (!m_bHSTCampaignEndRequested || !IsRunning())
			return;
		if (System.GetTickCount(m_iHSTCampaignEndCheckpointStartedTick)
			>= CAMPAIGN_END_CHECKPOINT_RETRY_WINDOW_MS)
		{
			BlockHSTCampaignEndCheckpoint(
				"checkpoint retry window expired");
			return;
		}

		HST_CampaignCoordinatorComponent coordinator
			= HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			QueueHSTCampaignEndCheckpointRetry(
				"campaign coordinator temporarily unavailable");
			return;
		}
		TryHSTCampaignEndCheckpoint(coordinator);
	}

	protected void BlockHSTCampaignEndCheckpoint(string evidence)
	{
		SetHSTCampaignEndWaitingForPlayerRelease(false);
		m_bHSTCampaignEndCheckpointRetryQueued = false;
		m_bHSTCampaignEndTransitionPollQueued = false;
		m_bHSTCampaignEndCheckpointBlocked = true;
		HST_CampaignCoordinatorComponent coordinator
			= HST_CampaignCoordinatorComponent.GetInstance();
		if (coordinator)
			coordinator.ForceControlledCampaignEndQuiescence(evidence);
		Print(string.Format(
			"Partisan controlled campaign end | blocked after %1 ms without a stable durable checkpoint; the quiesced server requires persistence investigation or a process restart | %2",
			System.GetTickCount(m_iHSTCampaignEndCheckpointStartedTick),
			evidence),
			LogLevel.ERROR);
	}

	protected void ContinueHSTCampaignEndAfterCheckpoint(
		bool nativeAuthorityCommitted)
	{
		if (!m_bHSTCampaignEndRequested)
			return;
		SetHSTCampaignEndWaitingForPlayerRelease(false);
		m_bHSTCampaignEndPendingNativeAuthority = nativeAuthorityCommitted;
		HST_CampaignCoordinatorComponent coordinator
			= HST_CampaignCoordinatorComponent.GetInstance();
		bool transitionPending;
		string transitionEvidence;
		if (!coordinator
			|| !coordinator.PrepareControlledCampaignEndTransition(
				transitionPending,
				transitionEvidence))
		{
			if (!coordinator)
			{
				transitionEvidence
					= "campaign coordinator unavailable before engine transition";
			}
			if (transitionPending)
				QueueHSTCampaignEndTransitionPoll(transitionEvidence);
			else
				BlockHSTCampaignEndCheckpoint(transitionEvidence);
			return;
		}

		m_bHSTPreserveCampaignSessionDataOnEnd
			= m_bHSTCampaignEndPendingNativeAuthority;
		m_bHSTUseProfileFallbackOnEnd
			= !m_bHSTCampaignEndPendingNativeAuthority;
		m_bHSTCampaignEndCheckpointCommitted = true;
		ref SCR_GameModeEndData endData = m_HSTPendingCampaignEndData;
		EndGameMode(endData);
	}

	protected void QueueHSTCampaignEndTransitionPoll(string evidence)
	{
		if (!m_bHSTCampaignEndRequested
			|| m_bHSTCampaignEndTransitionPollQueued
			|| m_bHSTCampaignEndCheckpointBlocked)
			return;
		if (System.GetTickCount(m_iHSTCampaignEndCheckpointStartedTick)
			>= CAMPAIGN_END_CHECKPOINT_RETRY_WINDOW_MS)
		{
			BlockHSTCampaignEndCheckpoint(evidence);
			return;
		}
		m_bHSTCampaignEndTransitionPollQueued = true;
		GetGame().GetCallqueue().CallLater(
			RetryHSTCampaignEndTransition,
			CAMPAIGN_END_TRANSITION_POLL_DELAY_MS,
			false);
	}

	protected void RetryHSTCampaignEndTransition()
	{
		m_bHSTCampaignEndTransitionPollQueued = false;
		if (!m_bHSTCampaignEndRequested
			|| m_bHSTCampaignEndCheckpointBlocked
			|| !IsRunning())
			return;
		ContinueHSTCampaignEndAfterCheckpoint(
			m_bHSTCampaignEndPendingNativeAuthority);
	}

	protected void ClearHSTCampaignEndCheckpointState()
	{
		SetHSTCampaignEndWaitingForPlayerRelease(false);
		m_bHSTCampaignEndRequested = false;
		m_bHSTCampaignEndCheckpointPending = false;
		m_bHSTCampaignEndCheckpointCommitted = false;
		m_bHSTCampaignEndCheckpointRetryQueued = false;
		m_bHSTCampaignEndTransitionPollQueued = false;
		m_bHSTCampaignEndCheckpointBlocked = false;
		m_bHSTCampaignEndPendingNativeAuthority = false;
		m_iHSTCampaignEndCheckpointAttempts = 0;
		m_iHSTCampaignEndCheckpointStartedTick = 0;
		m_HSTPendingCampaignEndData = null;
		m_HSTCampaignEndCheckpointCallback = null;
	}

	override protected void HandleOnGameModeEndSaveData()
	{
		if (m_bHSTUseProfileFallbackOnEnd)
		{
			SaveGameManager fallbackSaveManager = SaveGameManager.Get();
			if (fallbackSaveManager)
				fallbackSaveManager.SetSavingAllowed(false);
			PersistenceSystem fallbackPersistence
				= PersistenceSystem.GetInstance();
			if (fallbackPersistence)
				fallbackPersistence.ClearStorage(PersistenceSessionStorage);
			SaveGame staleActiveSave;
			if (fallbackSaveManager)
				staleActiveSave = fallbackSaveManager.GetActiveSave();
			if (fallbackSaveManager && staleActiveSave)
			{
				fallbackSaveManager.Purge(
					staleActiveSave.GetMissionResource(),
					staleActiveSave.GetPlaythroughNumber());
			}
			Print(
				"Partisan controlled campaign end | retained profile fallback authority and purged stale native session data");
			return;
		}
		if (!m_bHSTPreserveCampaignSessionDataOnEnd)
		{
			super.HandleOnGameModeEndSaveData();
			return;
		}

		SaveGameManager saveManager = SaveGameManager.Get();
		if (saveManager)
			saveManager.SetSavingAllowed(false);
		HST_CampaignCoordinatorComponent coordinator
			= HST_CampaignCoordinatorComponent.GetInstance();
		string receiptEvidence;
		if (!coordinator
			|| !coordinator.PublishControlledCampaignEndRetentionReceipt(
				receiptEvidence))
		{
			if (!coordinator)
				receiptEvidence = "campaign coordinator unavailable at retention";
			Print(
				"Partisan controlled campaign end | retention proof receipt failed | "
					+ receiptEvidence,
				LogLevel.ERROR);
		}
		Print(
			"Partisan controlled campaign end | retained the committed campaign session save");
	}

	override protected bool GetAllowControlsTarget()
	{
		if (m_bHSTCampaignEndRequested)
		{
			if (m_bHSTCampaignEndWaitingForPlayerRelease)
				return super.GetAllowControlsTarget();
			return false;
		}
		if (m_bHSTPreserveCampaignSessionDataOnEnd
			|| m_bHSTUseProfileFallbackOnEnd)
			return false;
		return super.GetAllowControlsTarget();
	}
}

modded class SCR_BudgetEditorComponent
{
	protected ref map<EEditableEntityBudget, int> m_mHSTOriginalBudgetCaps = new map<EEditableEntityBudget, int>();
	protected bool m_bHSTOriginalBudgetCapsCaptured;
	protected bool m_bHSTBudgetDeficitHandlerRegistered;
	protected int m_iHSTBudgetDeficitCorrectionCount;
	protected bool m_bHSTBudgetDeficitCorrectionLogged;

	override void EOnEditorInit()
	{
		super.EOnEditorInit();
		CaptureHistasiOriginalBudgetCaps();
		HistasiRefreshGameMasterBudgetCaps();
	}

	override protected void EOnEditorInitServer()
	{
		super.EOnEditorInitServer();
		HistasiRegisterBudgetDeficitHandler();
	}

	override protected void EOnEditorDeleteServer()
	{
		HistasiUnregisterBudgetDeficitHandler();
		super.EOnEditorDeleteServer();
	}

	void HistasiRefreshGameMasterBudgetCaps()
	{
		CaptureHistasiOriginalBudgetCaps();

		foreach (SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			if (!maxBudget)
				continue;

			EEditableEntityBudget budgetType = maxBudget.GetBudgetType();
			int originalBudget = m_mHSTOriginalBudgetCaps.Get(budgetType);
			if (HST_GameMasterBudgetService.AreGameMasterBudgetsEnabled())
			{
				maxBudget.SetBudgetValue(originalBudget);
				continue;
			}

			int disabledBudget = HistasiResolveDisabledBudgetHeadroom(budgetType);
			maxBudget.SetBudgetValue(disabledBudget);

			if (!HST_GameMasterBudgetService.IsManagedBudgetType(budgetType))
				continue;

			SCR_EditableEntityCoreBudgetSetting budgetSettings;
			if (m_EntityCore && m_EntityCore.GetBudget(budgetType, budgetSettings) && budgetSettings && budgetSettings.GetCurrentBudget() < disabledBudget)
				budgetSettings.SetCurrentBudget(disabledBudget);
		}
	}

	override protected bool IsBudgetCapEnabled()
	{
		if (this.ClassName() == "SCR_CampaignBuildingBudgetEditorComponent")
			return super.IsBudgetCapEnabled();

		if (!HST_GameMasterBudgetService.AreGameMasterBudgetsEnabled())
			return false;

		return super.IsBudgetCapEnabled();
	}

	protected void CaptureHistasiOriginalBudgetCaps()
	{
		if (m_bHSTOriginalBudgetCapsCaptured)
			return;
		if (!m_MaxBudgets)
			return;

		foreach (SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			if (maxBudget)
				m_mHSTOriginalBudgetCaps.Set(maxBudget.GetBudgetType(), maxBudget.GetBudgetValue());
		}

		m_bHSTOriginalBudgetCapsCaptured = true;
	}

	int HistasiResolveDisabledBudgetHeadroom(EEditableEntityBudget budgetType)
	{
		CaptureHistasiOriginalBudgetCaps();

		int originalBudget;
		if (m_mHSTOriginalBudgetCaps.Find(budgetType, originalBudget))
			return HST_GameMasterBudgetService.ResolveDisabledBudgetHeadroom(originalBudget);

		return HST_GameMasterBudgetService.ResolveDisabledBudgetRepairHeadroom();
	}

	protected void HistasiRegisterBudgetDeficitHandler()
	{
		if (m_bHSTBudgetDeficitHandlerRegistered || !m_EntityCore)
			return;

		m_EntityCore.Event_OnEntityBudgetUpdatedPerEntity.Insert(HistasiOnEntityBudgetUpdatedPerEntity);
		m_bHSTBudgetDeficitHandlerRegistered = true;
	}

	protected void HistasiUnregisterBudgetDeficitHandler()
	{
		if (!m_bHSTBudgetDeficitHandlerRegistered || !m_EntityCore)
			return;

		m_EntityCore.Event_OnEntityBudgetUpdatedPerEntity.Remove(HistasiOnEntityBudgetUpdatedPerEntity);
		m_bHSTBudgetDeficitHandlerRegistered = false;
	}

	bool HistasiIsBudgetCapEnabledForDiagnostics()
	{
		return IsBudgetCapEnabled();
	}

	bool HistasiIsBudgetDeficitHandlerRegistered()
	{
		return m_bHSTBudgetDeficitHandlerRegistered;
	}

	int HistasiCountManagedBudgetTypes()
	{
		CaptureHistasiOriginalBudgetCaps();
		if (!m_MaxBudgets)
			return 0;

		int count;
		foreach (SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			if (maxBudget && HST_GameMasterBudgetService.IsManagedBudgetType(maxBudget.GetBudgetType()))
				count++;
		}

		return count;
	}

	int HistasiCountManagedBudgetsAtDisabledHeadroom()
	{
		CaptureHistasiOriginalBudgetCaps();
		if (!m_MaxBudgets)
			return 0;

		int count;
		foreach (SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			if (!maxBudget || !HST_GameMasterBudgetService.IsManagedBudgetType(maxBudget.GetBudgetType()))
				continue;

			int originalBudget = m_mHSTOriginalBudgetCaps.Get(maxBudget.GetBudgetType());
			int expectedBudget = HST_GameMasterBudgetService.ResolveDisabledBudgetHeadroom(originalBudget);
			if (maxBudget.GetBudgetValue() == expectedBudget)
				count++;
		}

		return count;
	}

	int HistasiCountManagedBudgetsAtOriginalCap()
	{
		CaptureHistasiOriginalBudgetCaps();
		if (!m_MaxBudgets)
			return 0;

		int count;
		foreach (SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			if (!maxBudget || !HST_GameMasterBudgetService.IsManagedBudgetType(maxBudget.GetBudgetType()))
				continue;

			int originalBudget = m_mHSTOriginalBudgetCaps.Get(maxBudget.GetBudgetType());
			if (maxBudget.GetBudgetValue() == originalBudget)
				count++;
		}

		return count;
	}

	int HistasiCountManagedCurrentBudgetsAtDisabledHeadroom()
	{
		CaptureHistasiOriginalBudgetCaps();
		if (!m_MaxBudgets)
			return 0;

		int count;
		foreach (SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			if (!maxBudget || !HST_GameMasterBudgetService.IsManagedBudgetType(maxBudget.GetBudgetType()))
				continue;

			EEditableEntityBudget budgetType = maxBudget.GetBudgetType();
			int originalBudget = m_mHSTOriginalBudgetCaps.Get(budgetType);
			int expectedBudget = HST_GameMasterBudgetService.ResolveDisabledBudgetHeadroom(originalBudget);
			if (HistasiResolveCurrentBudgetValue(budgetType) >= expectedBudget)
				count++;
		}

		return count;
	}

	int HistasiCountBudgetDeficitCorrections()
	{
		return m_iHSTBudgetDeficitCorrectionCount;
	}

	protected int HistasiResolveCurrentBudgetValue(EEditableEntityBudget budgetType)
	{
		SCR_EditableEntityCoreBudgetSetting budgetSettings;
		if (m_EntityCore && m_EntityCore.GetBudget(budgetType, budgetSettings) && budgetSettings)
			return budgetSettings.GetCurrentBudget();

		return -1;
	}

	string HistasiBuildGameMasterBudgetDiagnostics()
	{
		CaptureHistasiOriginalBudgetCaps();

		string entries;
		if (m_MaxBudgets)
		{
			foreach (SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
			{
				if (!maxBudget || !HST_GameMasterBudgetService.IsManagedBudgetType(maxBudget.GetBudgetType()))
					continue;

				EEditableEntityBudget budgetType = maxBudget.GetBudgetType();
				int originalBudget = m_mHSTOriginalBudgetCaps.Get(budgetType);
				string entry = string.Format("%1 cap %2 original %3 tracked %4", typename.EnumToString(EEditableEntityBudget, budgetType), maxBudget.GetBudgetValue(), originalBudget, HistasiResolveCurrentBudgetValue(budgetType));
				if (entries.IsEmpty())
					entries = entry;
				else
					entries = entries + "; " + entry;
			}
		}

		if (entries.IsEmpty())
			entries = "none";

		string report = string.Format("editor %1 | budgetsEnabled %2 | capEnabled %3 | handler %4 | managed %5 | headroom %6 | original %7 | multiplier %8 | deficitCorrections %9",
			this.ClassName(),
			HST_GameMasterBudgetService.AreGameMasterBudgetsEnabled(),
			HistasiIsBudgetCapEnabledForDiagnostics(),
			m_bHSTBudgetDeficitHandlerRegistered,
			HistasiCountManagedBudgetTypes(),
			HistasiCountManagedBudgetsAtDisabledHeadroom(),
			HistasiCountManagedBudgetsAtOriginalCap(),
			HST_GameMasterBudgetService.BUDGET_HEADROOM_MULTIPLIER,
			HistasiCountBudgetDeficitCorrections());
		report = report + string.Format(" | trackedHeadroom %1", HistasiCountManagedCurrentBudgetsAtDisabledHeadroom());
		report = report + string.Format(" | preSubtractRepairs %1", HST_GameMasterBudgetService.CountDisabledBudgetPreSubtractRepairs());
		return report + " | " + entries;
	}

	protected void HistasiOnEntityBudgetUpdatedPerEntity(EEditableEntityBudget entityBudget, int originalBudgetValue, int budgetChange, int updatedBudgetValue, SCR_EditableEntityComponent entity)
	{
		if (HST_GameMasterBudgetService.AreGameMasterBudgetsEnabled())
			return;
		if (!HST_GameMasterBudgetService.IsManagedBudgetType(entityBudget))
			return;
		if (budgetChange >= 0 || updatedBudgetValue >= 0)
			return;

		SCR_EditableEntityCoreBudgetSetting budgetSettings;
		if (!m_EntityCore || !m_EntityCore.GetBudget(entityBudget, budgetSettings) || !budgetSettings)
			return;

		int deficit = -updatedBudgetValue;
		int currentBudget = budgetSettings.GetCurrentBudget();
		int targetBudget = Math.Max(HistasiResolveDisabledBudgetHeadroom(entityBudget), HST_GameMasterBudgetService.ResolveDisabledBudgetRepairHeadroom(deficit));
		budgetSettings.SetCurrentBudget(Math.Max(currentBudget + deficit, targetBudget));
		m_iHSTBudgetDeficitCorrectionCount++;
		if (!m_bHSTBudgetDeficitCorrectionLogged)
		{
			m_bHSTBudgetDeficitCorrectionLogged = true;
			Print(string.Format("Partisan game master budgets | corrected first disabled-budget deficit type=%1 adjusted=%2 change=%3 deficit=%4", typename.EnumToString(EEditableEntityBudget, entityBudget), originalBudgetValue, budgetChange, deficit));
		}
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(EEditableEntityBudget, "m_BudgetType")]
modded class SCR_EditableEntityCoreBudgetSetting
{
	override int SubtractFromBudget(SCR_EntityBudgetValue budgetCost = null)
	{
		int budgetCostValue = GetMinBudgetCost();
		if (budgetCost)
			budgetCostValue = Math.Max(budgetCost.GetBudgetValue(), budgetCostValue);

		HistasiEnsureDisabledBudgetHeadroomBeforeSubtract(budgetCostValue);
		return super.SubtractFromBudget(budgetCost);
	}

	override int SubtractFromBudget(int budgetValue)
	{
		HistasiEnsureDisabledBudgetHeadroomBeforeSubtract(budgetValue);
		return super.SubtractFromBudget(budgetValue);
	}

	protected void HistasiEnsureDisabledBudgetHeadroomBeforeSubtract(int pendingBudgetCost)
	{
		if (HST_GameMasterBudgetService.AreGameMasterBudgetsEnabled())
			return;
		if (!HST_GameMasterBudgetService.IsManagedBudgetType(GetBudgetType()))
			return;

		int currentBudget = GetCurrentBudget();
		int targetBudget = HST_GameMasterBudgetService.ResolveDisabledBudgetRepairHeadroom(pendingBudgetCost);
		if (currentBudget >= targetBudget)
			return;

		SetCurrentBudget(targetBudget);
		HST_GameMasterBudgetService.NoteDisabledBudgetPreSubtractRepair(GetBudgetType(), currentBudget, targetBudget, pendingBudgetCost);
	}
}

modded class SCR_PlacingEditorComponent
{
	override bool IsThereEnoughBudgetToSpawn(IEntityComponentSource entitySource)
	{
		if (!HST_GameMasterBudgetService.AreGameMasterBudgetsEnabled())
			return true;

		return super.IsThereEnoughBudgetToSpawn(entitySource);
	}

	override void CheckBudgetOwner()
	{
		if (!HST_GameMasterBudgetService.AreGameMasterBudgetsEnabled())
			return;

		super.CheckBudgetOwner();
	}

	override void OnBudgetMaxReached(EEditableEntityBudget entityBudget, bool maxReached)
	{
		if (!HST_GameMasterBudgetService.AreGameMasterBudgetsEnabled() && HST_GameMasterBudgetService.IsManagedBudgetType(entityBudget))
			return;

		super.OnBudgetMaxReached(entityBudget, maxReached);
	}
}
