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

	override void EOnEditorInit()
	{
		super.EOnEditorInit();
		CaptureHistasiOriginalBudgetCaps();
		HistasiRefreshGameMasterBudgetCaps();
	}

	void HistasiRefreshGameMasterBudgetCaps()
	{
		CaptureHistasiOriginalBudgetCaps();

		foreach (SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			EEditableEntityBudget budgetType = maxBudget.GetBudgetType();
			int originalBudget = m_mHSTOriginalBudgetCaps.Get(budgetType);
			if (HST_GameMasterBudgetService.AreGameMasterBudgetsEnabled())
			{
				maxBudget.SetBudgetValue(originalBudget);
				continue;
			}

			maxBudget.SetBudgetValue(originalBudget * HST_GameMasterBudgetService.BUDGET_HEADROOM_MULTIPLIER);
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

		foreach (SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
			m_mHSTOriginalBudgetCaps.Set(maxBudget.GetBudgetType(), maxBudget.GetBudgetValue());

		m_bHSTOriginalBudgetCapsCaptured = true;
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
