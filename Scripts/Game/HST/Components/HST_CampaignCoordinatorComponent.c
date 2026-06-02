[ComponentEditorProps(category: "h-istasi", description: "Server-authoritative h-istasi campaign coordinator")]
class HST_CampaignCoordinatorComponentClass : ScriptComponentClass
{
}

class HST_CampaignCoordinatorComponent : ScriptComponent
{
	protected ref HST_CampaignState m_State;
	protected ref HST_CampaignPreset m_Preset;
	protected ref HST_BalanceConfig m_Balance;
	protected ref HST_EconomyService m_Economy;
	protected ref HST_MissionService m_Missions;
	protected ref HST_PersistenceService m_Persistence;
	protected ref HST_AuthorizationService m_Authorization;
	protected ref HST_StrategicService m_Strategic;
	protected ref HST_ArsenalService m_Arsenal;
	protected ref HST_EnemyDirectorService m_EnemyDirector;
	protected ref HST_HQService m_HQ;
	protected float m_fSecondAccumulator;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		m_State = new HST_CampaignState();
		m_Preset = HST_DefaultCatalog.CreateRhsEveronPreset();
		m_Balance = HST_DefaultCatalog.CreateBalance();
		m_Economy = new HST_EconomyService();
		m_Missions = new HST_MissionService();
		m_Persistence = new HST_PersistenceService();
		m_Authorization = new HST_AuthorizationService();
		m_Strategic = new HST_StrategicService();
		m_Arsenal = new HST_ArsenalService();
		m_EnemyDirector = new HST_EnemyDirectorService();
		m_HQ = new HST_HQService();

		m_State.m_iFactionMoney = m_Balance.m_iStartingFactionMoney;
		m_State.m_iHR = m_Balance.m_iStartingHR;
		HST_DefaultCatalog.AddDefaultFactionPools(m_State, m_Balance, m_Preset);
		HST_DefaultCatalog.AddDefaultZones(m_State, m_Preset);
		m_HQ.SelectInitialHideout(m_State, HST_DefaultCatalog.GetDefaultHideoutId());

		SetEventMask(owner, EntityEvent.FRAME);
		Print("h-istasi | campaign coordinator initialized");
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer() || !m_State)
			return;

		m_Persistence.Tick(timeSlice, m_Balance.m_iAutosaveIntervalSeconds, m_Balance.m_iMajorChangeDebounceSeconds);
		m_fSecondAccumulator += timeSlice;
		if (m_fSecondAccumulator < 1)
			return;

		int elapsedSeconds = m_fSecondAccumulator;
		m_fSecondAccumulator -= elapsedSeconds;
		m_State.m_iElapsedSeconds += elapsedSeconds;
		m_Missions.Tick(m_State, m_Preset, elapsedSeconds);
	}

	HST_CampaignState GetState()
	{
		return m_State;
	}

	HST_PlayerState RegisterPlayer(string identityId, bool isAdmin = false)
	{
		if (!Replication.IsServer())
			return null;

		return m_Authorization.RegisterPlayer(m_State, identityId, isAdmin);
	}

	bool SetMembership(string actorIdentityId, string targetIdentityId, bool isMember)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Authorization.SetMembership(m_State, actorIdentityId, targetIdentityId, isMember);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	bool SetZoneOwner(string zoneId, string factionKey)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_Strategic.SetZoneOwner(m_State, m_Economy, m_Balance, zoneId, factionKey);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	HST_ArsenalItemState DepositArsenalItem(string prefab, int amount)
	{
		if (!Replication.IsServer())
			return null;

		HST_ArsenalItemState item = m_Arsenal.DepositItem(m_State, m_Balance, prefab, amount);
		if (item)
			m_Persistence.MarkMajorChange();
		return item;
	}

	bool RequestManualCheckpoint()
	{
		if (!Replication.IsServer())
			return false;

		return m_Persistence.RequestCheckpoint("h-istasi manual checkpoint");
	}

	bool SelectInitialHideout(string hideoutId)
	{
		if (!Replication.IsServer())
			return false;

		return SelectInitialHideout_S(hideoutId);
	}

	bool StartMission(string missionId, string targetZoneId = "")
	{
		if (!Replication.IsServer())
			return false;

		return StartMission_S(missionId, targetZoneId);
	}

	bool MoveHQ(string hideoutId)
	{
		if (!Replication.IsServer())
			return false;

		bool changed = m_HQ.MoveHQ(m_State, hideoutId);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	void OnPetrosKilled()
	{
		if (!Replication.IsServer())
			return;

		m_HQ.OnPetrosKilled(m_State, m_Economy, 250, 5);
		m_Persistence.MarkMajorChange();
	}

	protected bool SelectInitialHideout_S(string hideoutId)
	{
		bool changed = m_HQ.SelectInitialHideout(m_State, hideoutId);
		if (changed)
			m_Persistence.MarkMajorChange();
		return changed;
	}

	protected bool StartMission_S(string missionId, string targetZoneId)
	{
		if (!m_State || !m_Missions.Start(m_State, m_Preset, missionId, targetZoneId))
			return false;

		m_Persistence.MarkMajorChange();
		return true;
	}
}
