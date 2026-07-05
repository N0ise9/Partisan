class HST_GameMasterBudgetService
{
	static const int BUDGET_HEADROOM_MULTIPLIER = 500;

	protected static bool s_bGameMasterBudgetsEnabled;
	protected static bool s_bLoggedGameMasterBudgetState;
	protected static ref set<EEditableEntityBudget> s_aManagedBudgetTypes;

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
			Print(string.Format("h-istasi game master budgets | enabled=%1 source=%2", enabled, source));
		}
	}
}

modded class SCR_BaseGameMode
{
	[RplProp(onRplName: "OnHistasiGameMasterBudgetsReplicated")]
	protected bool m_bHSTGameMasterBudgetsEnabled;

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

			int disabledBudget = HST_GameMasterBudgetService.ResolveDisabledBudgetHeadroom(originalBudget);
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
		budgetSettings.SetCurrentBudget(currentBudget + deficit);
		m_iHSTBudgetDeficitCorrectionCount++;
		if (!m_bHSTBudgetDeficitCorrectionLogged)
		{
			m_bHSTBudgetDeficitCorrectionLogged = true;
			Print(string.Format("h-istasi game master budgets | corrected first disabled-budget deficit type=%1 adjusted=%2 change=%3 deficit=%4", typename.EnumToString(EEditableEntityBudget, entityBudget), originalBudgetValue, budgetChange, deficit));
		}
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
