[ComponentEditorProps(category: "h-istasi", description: "Player-owned h-istasi command menu request/RPC bridge")]
class HST_CommandMenuRequestComponentClass : ScriptComponentClass
{
}

class HST_CommandMenuRequestComponent : ScriptComponent
{
	protected static HST_CommandMenuRequestComponent s_LocalOwner;
	protected static HST_CommandMenuRequestComponent s_ServerBroadcaster;

	protected IEntity m_OwnerEntity;
	protected string m_sLastSnapshot;
	protected string m_sLastResult;
	protected bool m_bIsLocalOwner;
	protected float m_fOwnerRetryAccumulator;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		m_bIsLocalOwner = IsLocalOwner(owner);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);

		if (m_bIsLocalOwner)
			s_LocalOwner = this;
		if (Replication.IsServer() && !s_ServerBroadcaster)
			s_ServerBroadcaster = this;
	}

	override void OnDelete(IEntity owner)
	{
		if (s_LocalOwner == this)
			s_LocalOwner = null;
		if (s_ServerBroadcaster == this)
			s_ServerBroadcaster = null;

		super.OnDelete(owner);
	}

	override void EOnInit(IEntity owner)
	{
		RefreshLocalOwner(owner);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (m_bIsLocalOwner)
			return;

		m_fOwnerRetryAccumulator += timeSlice;
		if (m_fOwnerRetryAccumulator < 0.25)
			return;

		m_fOwnerRetryAccumulator = 0;
		RefreshLocalOwner(owner);
	}

	static HST_CommandMenuRequestComponent GetLocalOwner()
	{
		return s_LocalOwner;
	}

	static void BroadcastMissionEvent(string payload, string summary)
	{
		if (!s_ServerBroadcaster)
		{
			Print("h-istasi mission event | no server broadcaster ready", LogLevel.WARNING);
			return;
		}

		s_ServerBroadcaster.BroadcastMissionEvent_I(payload, summary);
	}

	static void BroadcastNotification(string payload, string summary = "")
	{
		if (!s_ServerBroadcaster)
		{
			Print("h-istasi notification | no server broadcaster ready", LogLevel.WARNING);
			return;
		}

		s_ServerBroadcaster.BroadcastNotification_I(payload, summary);
	}

	static void BroadcastMissionIntel(string payload)
	{
		if (!s_ServerBroadcaster)
			return;

		s_ServerBroadcaster.BroadcastMissionIntel_I(payload);
	}

	string GetLastSnapshot()
	{
		return m_sLastSnapshot;
	}

	string GetLastResult()
	{
		return m_sLastResult;
	}

	int ResolveLocalPlayerId()
	{
		if (!m_OwnerEntity)
			return 0;

		PlayerController controller = PlayerController.Cast(m_OwnerEntity);
		if (controller && controller.GetPlayerId() > 0)
			return controller.GetPlayerId();

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 0;

		int controlledPlayerId = playerManager.GetPlayerIdFromControlledEntity(m_OwnerEntity);
		if (controlledPlayerId > 0)
			return controlledPlayerId;

		BaseRplComponent rpl = BaseRplComponent.Cast(m_OwnerEntity.FindComponent(BaseRplComponent));
		if (rpl)
			return playerManager.GetPlayerIdFromEntityRplId(rpl.Id());

		return 0;
	}

	void RequestSnapshot(string selectedTabId, string lastResult = "")
	{
		if (Replication.IsServer())
		{
			SendSnapshotToOwner(selectedTabId, lastResult);
			return;
		}

		Rpc(RpcAsk_RequestSnapshot, selectedTabId, lastResult);
	}

	void RequestAction(string selectedTabId, string commandId, string argument = "")
	{
		if (Replication.IsServer())
		{
			SendActionResultToOwner(selectedTabId, commandId, argument);
			return;
		}

		Rpc(RpcAsk_RequestAction, selectedTabId, commandId, argument);
	}

	void RequestLoadoutEditorAction(string commandId, string argument = "")
	{
		if (Replication.IsServer())
		{
			SendLoadoutEditorActionResultToOwner(commandId, argument);
			return;
		}

		Rpc(RpcAsk_RequestLoadoutEditorAction, commandId, argument);
	}

	void RequestMissionIntel()
	{
		if (Replication.IsServer())
		{
			SendMissionIntelToOwner();
			return;
		}

		Rpc(RpcAsk_RequestMissionIntel);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestSnapshot(string selectedTabId, string lastResult)
	{
		SendSnapshotToOwner(selectedTabId, lastResult);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestAction(string selectedTabId, string commandId, string argument)
	{
		SendActionResultToOwner(selectedTabId, commandId, argument);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestLoadoutEditorAction(string commandId, string argument)
	{
		SendLoadoutEditorActionResultToOwner(commandId, argument);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_RequestMissionIntel()
	{
		SendMissionIntelToOwner();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveSnapshot(string payload, string lastResult)
	{
		m_sLastSnapshot = payload;
		m_sLastResult = lastResult;

		HST_CommandMenuComponent menu = HST_CommandMenuComponent.GetLocalInstance();
		if (menu)
			menu.OnServerSnapshot(payload, lastResult);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveLoadoutEditorPayload(string payload, string lastResult)
	{
		HST_LoadoutEditorComponent loadoutEditor = HST_LoadoutEditorComponent.GetLocalInstance();
		if (loadoutEditor)
			loadoutEditor.OnServerActionResult(payload, lastResult);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ReceiveMissionEvent(string payload, string summary)
	{
		HST_MissionClientComponent missionClient = HST_MissionClientComponent.GetLocalInstance();
		if (missionClient)
			missionClient.OnServerMissionEvent(payload, summary);
		else
			Print("h-istasi mission event | " + summary);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ReceiveNotification(string payload, string summary)
	{
		HST_MissionClientComponent missionClient = HST_MissionClientComponent.GetLocalInstance();
		if (missionClient)
			missionClient.OnServerNotification(payload, summary);
		else
			Print("h-istasi notification | " + summary);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RpcDo_ReceiveMissionIntelOwner(string payload)
	{
		HST_MissionClientComponent missionClient = HST_MissionClientComponent.GetLocalInstance();
		if (missionClient)
			missionClient.OnServerMissionIntel(payload);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ReceiveMissionIntel(string payload)
	{
		HST_MissionClientComponent missionClient = HST_MissionClientComponent.GetLocalInstance();
		if (missionClient)
			missionClient.OnServerMissionIntel(payload);
	}

	protected void SendSnapshotToOwner(string selectedTabId, string lastResult)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSnapshot("HST_MENU|offline|0\nSTATUS|h-istasi menu | coordinator not ready\nEND", lastResult);
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity);
		string payload = coordinator.BuildVisibleMenuPayload(playerId, selectedTabId, lastResult);
		DeliverSnapshot(payload, lastResult);
	}

	protected void BroadcastMissionEvent_I(string payload, string summary)
	{
		Rpc(RpcDo_ReceiveMissionEvent, payload, summary);
	}

	protected void BroadcastNotification_I(string payload, string summary)
	{
		Rpc(RpcDo_ReceiveNotification, payload, summary);
	}

	protected void BroadcastMissionIntel_I(string payload)
	{
		Rpc(RpcDo_ReceiveMissionIntel, payload);
	}

	protected void SendActionResultToOwner(string selectedTabId, string commandId, string argument)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverSnapshot("HST_MENU|offline|0\nSTATUS|h-istasi command | coordinator not ready\nEND", "coordinator not ready");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity);
		string result = coordinator.RequestVisibleMenuCommand(playerId, selectedTabId, commandId, argument);
		string payload = coordinator.BuildVisibleMenuPayload(playerId, selectedTabId, result);
		DeliverSnapshot(payload, result);
	}

	protected void SendLoadoutEditorActionResultToOwner(string commandId, string argument)
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverLoadoutEditorPayload("HST_LOADOUT_EDITOR|offline||false|0|0|0|0\nPREVIEW|false|0 0 0|0|coordinator not ready\nEND", "coordinator not ready");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity);
		string result = coordinator.RequestLoadoutEditorCommand(playerId, commandId, argument);
		string payload;
		if (commandId == "loadout_editor_candidates")
			payload = coordinator.BuildLoadoutEditorCandidatePayload(playerId, argument);
		else
			payload = coordinator.BuildLoadoutEditorPayload(playerId);
		DeliverLoadoutEditorPayload(payload, result);
	}

	protected void SendMissionIntelToOwner()
	{
		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (!coordinator)
		{
			DeliverMissionIntel("HST_MISSION_INTEL|offline|0\nEND");
			return;
		}

		int playerId = coordinator.ResolveAuthoritativePlayerId(m_OwnerEntity);
		DeliverMissionIntel(coordinator.BuildMissionIntelPayload(playerId));
	}

	protected void DeliverSnapshot(string payload, string lastResult)
	{
		BaseRplComponent rpl = BaseRplComponent.Cast(m_OwnerEntity.FindComponent(BaseRplComponent));
		if (Replication.IsServer() && (!rpl || rpl.IsOwner()))
		{
			RpcDo_ReceiveSnapshot(payload, lastResult);
			return;
		}

		Rpc(RpcDo_ReceiveSnapshot, payload, lastResult);
	}

	protected void DeliverLoadoutEditorPayload(string payload, string lastResult)
	{
		BaseRplComponent rpl = BaseRplComponent.Cast(m_OwnerEntity.FindComponent(BaseRplComponent));
		if (Replication.IsServer() && (!rpl || rpl.IsOwner()))
		{
			RpcDo_ReceiveLoadoutEditorPayload(payload, lastResult);
			return;
		}

		Rpc(RpcDo_ReceiveLoadoutEditorPayload, payload, lastResult);
	}

	protected void DeliverMissionIntel(string payload)
	{
		BaseRplComponent rpl = BaseRplComponent.Cast(m_OwnerEntity.FindComponent(BaseRplComponent));
		if (Replication.IsServer() && (!rpl || rpl.IsOwner()))
		{
			RpcDo_ReceiveMissionIntelOwner(payload);
			return;
		}

		Rpc(RpcDo_ReceiveMissionIntelOwner, payload);
	}

	protected void RefreshLocalOwner(IEntity owner)
	{
		if (m_bIsLocalOwner)
			return;

		if (!IsLocalOwner(owner))
			return;

		m_bIsLocalOwner = true;
		s_LocalOwner = this;
	}

	protected bool IsLocalOwner(IEntity owner)
	{
		if (!owner)
			return false;

		BaseRplComponent rpl = BaseRplComponent.Cast(owner.FindComponent(BaseRplComponent));
		return !rpl || rpl.IsOwner();
	}
}
