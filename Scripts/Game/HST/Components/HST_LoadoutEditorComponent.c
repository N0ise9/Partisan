class HST_LoadoutEditorWidgetHandler : ScriptedWidgetEventHandler
{
	protected HST_LoadoutEditorComponent m_Editor;

	void Bind(HST_LoadoutEditorComponent editor)
	{
		m_Editor = editor;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Editor)
			return false;

		return m_Editor.OnWidgetClickedWithButton(w.GetUserID(), button);
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (!m_Editor)
			return false;

		return m_Editor.OnWidgetClickedWithButton(w.GetUserID(), button);
	}
}

class HST_LoadoutEditorDrawCommandSet
{
	ref array<ref CanvasWidgetCommand> m_aCommands = {};
}

[ComponentEditorProps(category: "h-istasi", description: "Client-side fullscreen h-istasi loadout editor")]
class HST_LoadoutEditorComponentClass : ScriptComponentClass
{
}

class HST_LoadoutEditorComponent : ScriptComponent
{
	static const ResourceName EDITOR_LAYOUT = "{5AF2D86E07D44A51}UI/layouts/HST_LoadoutEditor.layout";
	static const ResourceName DEFAULT_PREVIEW_PREFAB = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	static const ResourceName PREVIEW_WORLD_PREFAB = "{71D2E9B5588949D8}Prefabs/HST/HST_LoadoutPreviewWorld.et";
	static const ResourceName PREVIEW_LIGHTS_PREFAB = "{604FFDF1DE53BD1D}Prefabs/HST/HST_LoadoutPreviewLights.et";
	static const string PREVIEW_MODE_CHARACTER = "character";
	static const string PREVIEW_MODE_ENTITY = "entity";
	static const string EDITOR_INPUT_CONTEXT = "InGameMenuContext";
	static const string EDITOR_CURSOR_CONTEXT = "InventoryContext";
	static const string NODE_LOADOUT_PREFIX = "live_loadout_";
	static const string NODE_WEAPON_PREFIX = "live_weapon_";
	static const string NODE_ATTACHMENT_PREFIX = "live_attach_";
	static const string NODE_STORAGE_PREFIX = "live_storage_";
	static const string NODE_STORAGE_ITEM_PREFIX = "live_storage_item_";
	static const int CLOSE_WIDGET_ID = 9200;
	static const int CANCEL_WIDGET_ID = 9201;
	static const int APPLY_WIDGET_ID = 9202;
	static const int SAVE_WIDGET_ID = 9203;
	static const int DELETE_TEMPLATE_WIDGET_ID = 9204;
	static const int ITEM_PAGE_PREV_WIDGET_ID = 9205;
	static const int ITEM_PAGE_NEXT_WIDGET_ID = 9206;
	static const int SLOT_PAGE_PREV_WIDGET_ID = 9207;
	static const int SLOT_PAGE_NEXT_WIDGET_ID = 9208;
	static const int TEMPLATE_PAGE_PREV_WIDGET_ID = 9209;
	static const int TEMPLATE_PAGE_NEXT_WIDGET_ID = 9210;
	static const int CLEAR_DRAFT_WIDGET_ID = 9211;
	static const int CAMERA_WIDGET_ID = 9212;
	static const int BACK_WIDGET_ID = 9214;
	static const int SWAP_WIDGET_ID = 9215;
	static const int RESET_PREVIEW_WIDGET_ID = 9216;
	static const int ROTATE_PREVIEW_WIDGET_ID = 9217;
	static const int MODE_WIDGET_ID_BASE = 9250;
	static const int CATEGORY_WIDGET_ID_BASE = 9300;
	static const int STORAGE_CATEGORY_WIDGET_ID_BASE = 9350;
	static const int ITEM_WIDGET_ID_BASE = 9400;
	static const int SLOT_REMOVE_WIDGET_ID_BASE = 9500;
	static const int SLOT_MINUS_WIDGET_ID_BASE = 9600;
	static const int SLOT_PLUS_WIDGET_ID_BASE = 9700;
	static const int TEMPLATE_WIDGET_ID_BASE = 9800;
	static const int SLOT_SELECT_WIDGET_ID_BASE = 9900;
	static const int NODE_WIDGET_ID_BASE = 11000;
	static const int CANDIDATE_WIDGET_ID_BASE = 12000;
	static const int REMOVE_SELECTED_NODE_WIDGET_ID = 13000;
	static const int ITEMS_PER_PAGE = 11;
	static const int STORAGE_ITEMS_PER_PAGE = 24;
	static const int SLOTS_PER_PAGE = 9;
	static const int TEMPLATES_PER_PAGE = 6;
	static const int PREVIEW_CAMERA = 0;
	static const int PREVIEW_LIGHT_DELAY_MS = 500;
	static const int PREVIEW_DRESS_DELAY_MS = 500;
	static const int PREVIEW_BOUNDS_RETRY_DELAY_MS = 120;
	static const int PREVIEW_BOUNDS_RETRY_COUNT = 4;
	static const int POST_ACTION_REFRESH_DELAY_MS = 350;
	static const ResourceName ICON_CLOTHING = "{A9AFA05DD269660A}Assets/512/clothing_icon.edds";
	static const ResourceName ICON_WEAPONS = "{4F051820B3912C59}Assets/512/weapons_icon.edds";
	static const ResourceName ICON_ATTACHMENTS = "{E77BB529AFB78928}Assets/512/attachments_icon.edds";
	static const ResourceName ICON_STORAGE = "{ED043AF0A83EC500}Assets/512/inventory_icon.edds";
	static const ResourceName ICON_SAVE = "{FDD6657E1E611D2D}Assets/512/save_icon.edds";
	static const ResourceName ICON_SETTINGS = "{B09A78C4D3DA8929}Assets/512/settings_icon.edds";
	static const ResourceName ICON_AMMO = "{705E39A54B927E94}Assets/512/ammunition_icon.edds";
	static const ResourceName ICON_EQUIPMENT = "{82311870FB87265B}Assets/512/equipment_icon.edds";
	static const ResourceName ICON_THROWABLES = "{15364AA4BD9F047E}Assets/512/throwables_icon.edds";
	static const ResourceName ICON_MEDICAL = "{5E7C2CD59EAB96ED}Assets/512/medical_icon.edds";

	protected static HST_LoadoutEditorComponent s_LocalInstance;

	protected IEntity m_OwnerEntity;
	protected bool m_bIsLocalOwner;
	protected bool m_bEditorOpen;
	protected string m_sEditorMode = "clothing";
	protected string m_sSelectedCategory = "clothing";
	protected string m_sSelectedSlotId;
	protected string m_sSelectedNodeId;
	protected string m_sSelectedTemplateId;
	protected string m_sPreviewPrefab = DEFAULT_PREVIEW_PREFAB;
	protected string m_sStatusText = "Choose a saved template or build a draft from recovered arsenal gear.";
	protected string m_sPreviewStatus;
	protected string m_sPreviewPosition;
	protected string m_sLastResult;
	protected bool m_bPreviewSpawned;
	protected bool m_bCandidateMode;
	protected bool m_bPostActionRefreshQueued;
	protected int m_iPreviewItemCount;
	protected int m_iDraftSlotCount;
	protected int m_iDraftItemCount;
	protected int m_iDraftFiniteCount;
	protected int m_iDraftInfiniteCount;
	protected int m_iItemPage;
	protected int m_iSlotPage;
	protected int m_iTemplatePage;
	protected int m_iCameraMode;
	protected float m_fPreviewYawDegrees;
	protected int m_iEditorWidth = 1280;
	protected int m_iEditorHeight = 850;
	protected ref array<string> m_aCategoryIds = {};
	protected ref array<string> m_aCategoryLabels = {};
	protected ref array<int> m_aCategoryCounts = {};
	protected ref array<string> m_aItemCategories = {};
	protected ref array<string> m_aItemPrefabs = {};
	protected ref array<string> m_aItemDisplays = {};
	protected ref array<string> m_aItemShortDisplays = {};
	protected ref array<string> m_aItemSlotLabels = {};
	protected ref array<string> m_aItemCounts = {};
	protected ref array<bool> m_aItemInfinite = {};
	protected ref array<bool> m_aItemPreviewEligible = {};
	protected ref array<int> m_aVisibleItemIndexes = {};
	protected ref array<string> m_aSlotIds = {};
	protected ref array<string> m_aSlotCategories = {};
	protected ref array<string> m_aSlotPrefabs = {};
	protected ref array<string> m_aSlotDisplays = {};
	protected ref array<string> m_aSlotShortDisplays = {};
	protected ref array<string> m_aSlotLabels = {};
	protected ref array<int> m_aSlotQuantities = {};
	protected ref array<bool> m_aSlotPreviewEligible = {};
	protected ref array<string> m_aNodeIds = {};
	protected ref array<string> m_aNodeParents = {};
	protected ref array<string> m_aNodeKinds = {};
	protected ref array<string> m_aNodeSlotKeys = {};
	protected ref array<string> m_aNodeLabels = {};
	protected ref array<string> m_aNodePrefabs = {};
	protected ref array<string> m_aNodeDisplays = {};
	protected ref array<string> m_aNodeCounts = {};
	protected ref array<bool> m_aNodeInfinite = {};
	protected ref array<bool> m_aNodeCanOpen = {};
	protected ref array<bool> m_aNodeCanRemove = {};
	protected ref array<bool> m_aNodeCanDeposit = {};
	protected ref array<string> m_aNodeFocus = {};
	protected ref array<int> m_aVisibleNodeIndexes = {};
	protected ref array<string> m_aCandidateNodeIds = {};
	protected ref array<string> m_aCandidatePrefabs = {};
	protected ref array<string> m_aCandidateDisplays = {};
	protected ref array<string> m_aCandidateShortDisplays = {};
	protected ref array<string> m_aCandidateCounts = {};
	protected ref array<bool> m_aCandidateInfinite = {};
	protected ref array<string> m_aCandidateKinds = {};
	protected ref array<bool> m_aCandidateCompatible = {};
	protected ref array<bool> m_aCandidateAmmoMatch = {};
	protected ref array<string> m_aCandidateIconHints = {};
	protected ref array<int> m_aVisibleCandidateIndexes = {};
	protected ref array<string> m_aLoadedCandidateNodeIds = {};
	protected ref array<string> m_aCandidateEmptyNodeIds = {};
	protected ref array<string> m_aCandidateEmptyReasons = {};
	protected ref array<string> m_aRequestedCandidateNodeIds = {};
	protected ref array<int> m_aRequestedCandidateFrames = {};
	protected ref array<int> m_aRequestedCandidateAttempts = {};
	protected ref array<string> m_aTemplateIds = {};
	protected ref array<string> m_aTemplateDisplays = {};
	protected ref array<int> m_aTemplateSlotCounts = {};
	protected ref array<Widget> m_aWidgets = {};
	protected ref array<ref HST_LoadoutEditorDrawCommandSet> m_aCanvasCommandSets = {};
	protected ref HST_LoadoutEditorWidgetHandler m_WidgetHandler;
	protected Widget m_RootWidget;
	protected Widget m_UILayerWidget;
	protected bool m_bRootFromLayout;
	protected RenderTargetWidget m_PreviewWidget;
	protected ref SharedItemRef m_PreviewWorldRef;
	protected BaseWorld m_PreviewWorld;
	protected IEntity m_PreviewStageEntity;
	protected IEntity m_PreviewLightEntity;
	protected IEntity m_PreviewSourceCharacter;
	protected IEntity m_PreviewEditedCharacter;
	protected IEntity m_PreviewSourceEntity;
	protected IEntity m_PreviewEntity;
	protected string m_sPreviewSourceMode = PREVIEW_MODE_CHARACTER;
	protected string m_sPreviewRenderKey;
	protected bool m_bPreviewLightSpawnQueued;
	protected bool m_bPreviewLightAnglesInitialized;
	protected vector m_vPreviewLightBaseAngles;
	protected vector m_vCameraTargetPosition = "5 1 5";
	protected vector m_vCameraTargetLook = "0 0.5 0";
	protected vector m_vCameraCurrentPosition = "5 1 5";
	protected vector m_vCameraCurrentLook = "0 0.5 0";
	protected vector m_vPreviewedEntityCenterWorld = vector.Zero;
	protected float m_fPreviewedEntitySize;
	protected float m_fPreviewedEntityDefaultDistance = 1.0;
	protected vector m_vCurrentCameraLookPosition = "0 0.5 0";
	protected vector m_aCurrentCameraMatrix[4];
	protected float m_fCameraDistanceToTarget;
	protected float m_fCameraLookDistanceToTarget;
	protected vector m_vCameraTargetDirection = "1 0 1";
	protected float m_fCameraTargetDistance = 1.0;
	protected bool m_bCameraInitialized;
	protected int m_iFrameSerial;
	protected int m_iLastActivatedWidgetId;
	protected int m_iLastActivatedButton;
	protected int m_iLastActivatedFrame = -1;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		m_bIsLocalOwner = IsLocalOwner(owner);
		SetEventMask(owner, EntityEvent.INIT | EntityEvent.FRAME);

		if (m_bIsLocalOwner)
			s_LocalInstance = this;
	}

	override void OnDelete(IEntity owner)
	{
		if (s_LocalInstance == this)
			s_LocalInstance = null;

		CloseEditor(false);
		super.OnDelete(owner);
	}

	override void EOnInit(IEntity owner)
	{
		RefreshLocalOwner(owner);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_bEditorOpen)
			return;

		m_iFrameSerial++;
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.ActivateContext(EDITOR_INPUT_CONTEXT);
		inputManager.ActivateContext(EDITOR_CURSOR_CONTEXT);
		AnimatePreviewCamera(timeSlice);
	}

	static HST_LoadoutEditorComponent GetLocalInstance()
	{
		return s_LocalInstance;
	}

	void OpenFromArsenal(IEntity userEntity)
	{
		if (!m_bIsLocalOwner)
			return;

		HST_CommandMenuComponent commandMenu = HST_CommandMenuComponent.GetLocalInstance();
		if (commandMenu)
			commandMenu.CloseMenuFromExternal();

		m_PreviewEditedCharacter = userEntity;
		m_PreviewSourceEntity = userEntity;
		m_sPreviewSourceMode = PREVIEW_MODE_CHARACTER;
		m_sPreviewRenderKey = "";
		m_bCameraInitialized = false;
		m_bPostActionRefreshQueued = false;
		m_iCameraMode = 0;
		m_fPreviewYawDegrees = 0;
		TryUnequipHeldItem(userEntity);

		m_bEditorOpen = true;
		m_sEditorMode = "clothing";
		m_sSelectedCategory = "clothing";
		m_sSelectedSlotId = "";
		m_sSelectedNodeId = "";
		m_bCandidateMode = false;
		m_sStatusText = "Opening h-istasi arsenal editor. Counts, INF unlocks, and apply validation stay server-authoritative.";
		m_sLastResult = "requested editor session";
		RequestServerAction("loadout_editor_open_hq_arsenal", "");
		RenderEditor();
		ShowHint("Loadout editor opened");
	}

	bool OnWidgetClicked(int widgetId)
	{
		return OnWidgetClickedWithButton(widgetId, 0);
	}

	bool OnWidgetClickedWithButton(int widgetId, int button)
	{
		if (!m_bEditorOpen)
			return false;

		if (IsDuplicateWidgetActivation(widgetId, button))
			return true;

		if (widgetId == CLOSE_WIDGET_ID || widgetId == CANCEL_WIDGET_ID)
		{
			CloseEditor(true);
			return true;
		}

		if (widgetId == BACK_WIDGET_ID)
		{
			if (m_sEditorMode == "storage")
			{
				m_sEditorMode = "clothing";
				m_sSelectedCategory = ResolveDefaultCategoryForMode(m_sEditorMode);
				m_sSelectedSlotId = "";
				m_sSelectedNodeId = "";
				m_bCandidateMode = false;
				m_iItemPage = 0;
			}
			else if (m_bCandidateMode)
			{
				if (!ReturnFromAttachmentCandidateToWeapon())
					m_bCandidateMode = false;
				m_iItemPage = 0;
			}
			else if (m_sEditorMode == "attachments" && !m_sSelectedNodeId.IsEmpty())
			{
				m_sSelectedNodeId = "";
				m_sSelectedSlotId = "";
				m_sSelectedCategory = "attachment";
			}
			else
			{
				m_sEditorMode = "clothing";
				m_sSelectedCategory = ResolveDefaultCategoryForMode(m_sEditorMode);
				m_sSelectedSlotId = "";
				m_sSelectedNodeId = "";
			}
			m_iItemPage = 0;
			UpdatePreviewCamera(true);
			RenderEditor();
			return true;
		}

		if (widgetId == CAMERA_WIDGET_ID)
		{
			m_iCameraMode++;
			if (m_iCameraMode > 2)
				m_iCameraMode = 0;

			UpdatePreviewCamera(true);
			RenderEditor();
			return true;
		}

		if (widgetId == RESET_PREVIEW_WIDGET_ID)
		{
			m_iCameraMode = 0;
			m_fPreviewYawDegrees = 0;
			m_sSelectedSlotId = "";
			m_sSelectedNodeId = "";
			m_bCandidateMode = false;
			m_sPreviewRenderKey = "";
			RefreshPreviewWorldLoadout();
			UpdatePreviewCamera(false);
			RenderEditor();
			return true;
		}

		if (widgetId == ROTATE_PREVIEW_WIDGET_ID)
		{
			m_fPreviewYawDegrees = m_fPreviewYawDegrees + 45.0;
			if (m_fPreviewYawDegrees >= 360.0)
				m_fPreviewYawDegrees = 0;

			UpdatePreviewCameraImmediate(true, "rotate");
			RenderEditor();
			return true;
		}

		if (widgetId == SWAP_WIDGET_ID)
		{
			ShowHint(BuildSwapHint());
			m_sLastResult = BuildSwapHint();
			RenderEditor();
			return true;
		}

		if (widgetId == SAVE_WIDGET_ID)
		{
			m_sLastResult = "requested save draft";
			RequestServerAction("loadout_save", "");
			RenderEditor();
			ShowHint("Saved current draft request sent");
			return true;
		}

		if (widgetId == APPLY_WIDGET_ID)
		{
			m_sLastResult = "requested apply saved loadout";
			RequestServerAction("loadout_apply", "");
			RenderEditor();
			ShowHint("Apply request sent");
			return true;
		}

		if (widgetId == DELETE_TEMPLATE_WIDGET_ID)
		{
			if (m_sSelectedTemplateId.IsEmpty())
				return true;

			m_sLastResult = "requested delete template";
			RequestServerAction("loadout_delete", m_sSelectedTemplateId);
			RenderEditor();
			ShowHint("Delete template request sent");
			return true;
		}

		if (widgetId == CLEAR_DRAFT_WIDGET_ID)
		{
			m_sLastResult = "requested clear draft";
			RequestServerAction("loadout_clear_draft", "");
			RenderEditor();
			return true;
		}

		if (widgetId == ITEM_PAGE_PREV_WIDGET_ID)
		{
			m_iItemPage = Math.Max(0, m_iItemPage - 1);
			RenderEditor();
			return true;
		}

		if (widgetId == ITEM_PAGE_NEXT_WIDGET_ID)
		{
			m_iItemPage++;
			RenderEditor();
			return true;
		}

		if (widgetId == SLOT_PAGE_PREV_WIDGET_ID)
		{
			m_iSlotPage = Math.Max(0, m_iSlotPage - 1);
			RenderEditor();
			return true;
		}

		if (widgetId == SLOT_PAGE_NEXT_WIDGET_ID)
		{
			m_iSlotPage++;
			RenderEditor();
			return true;
		}

		if (widgetId == TEMPLATE_PAGE_PREV_WIDGET_ID)
		{
			m_iTemplatePage = Math.Max(0, m_iTemplatePage - 1);
			RenderEditor();
			return true;
		}

		if (widgetId == TEMPLATE_PAGE_NEXT_WIDGET_ID)
		{
			m_iTemplatePage++;
			RenderEditor();
			return true;
		}

		int modeIndex = widgetId - MODE_WIDGET_ID_BASE;
		if (modeIndex >= 0 && modeIndex < GetEditorModeCount())
		{
			m_sEditorMode = GetEditorModeId(modeIndex);
			m_sSelectedCategory = ResolveDefaultCategoryForMode(m_sEditorMode);
			m_sSelectedSlotId = "";
			m_sSelectedNodeId = "";
			m_bCandidateMode = false;
			m_iItemPage = 0;
			m_iSlotPage = 0;
			m_iTemplatePage = 0;
			if (m_sEditorMode == "storage")
			{
				EnsureSelectedStorageNode();
				EnsureCandidatePayloadForSelectedNode();
			}
			UpdatePreviewCamera(true);
			RenderEditor();
			return true;
		}

		int categoryIndex = widgetId - CATEGORY_WIDGET_ID_BASE;
		if (categoryIndex >= 0 && categoryIndex < m_aCategoryIds.Count())
		{
			m_sSelectedCategory = m_aCategoryIds[categoryIndex];
			m_sEditorMode = ResolveModeForCategory(m_sSelectedCategory);
			int selectedSlotIndex = FindSelectedSlotIndex();
			if (selectedSlotIndex < 0 || m_aSlotCategories[selectedSlotIndex] != m_sSelectedCategory)
				m_sSelectedSlotId = "";
			m_iItemPage = 0;
			UpdatePreviewCamera(true);
			RenderEditor();
			return true;
		}

		int storageCategoryIndex = widgetId - STORAGE_CATEGORY_WIDGET_ID_BASE;
		if (storageCategoryIndex >= 0 && storageCategoryIndex < GetStorageBrowserCategoryCount())
		{
			m_sSelectedCategory = GetStorageBrowserCategoryId(storageCategoryIndex);
			m_sEditorMode = "storage";
			m_iItemPage = 0;
			EnsureSelectedStorageNode();
			EnsureCandidatePayloadForSelectedNode();
			RenderEditor();
			return true;
		}

		int itemVisibleIndex = widgetId - ITEM_WIDGET_ID_BASE;
		if (itemVisibleIndex >= 0 && itemVisibleIndex < m_aVisibleItemIndexes.Count())
		{
			int itemIndex = m_aVisibleItemIndexes[itemVisibleIndex];
			if (itemIndex >= 0 && itemIndex < m_aItemPrefabs.Count())
			{
				if (!m_sSelectedSlotId.IsEmpty())
				{
					m_sLastResult = "requested swap item";
					RequestServerAction("loadout_replace_slot", m_sSelectedSlotId + ":" + m_aItemPrefabs[itemIndex]);
				}
				else
				{
					m_sLastResult = "requested add item";
					RequestServerAction("loadout_add_item", m_aItemPrefabs[itemIndex]);
				}

				RenderEditor();
				return true;
			}
		}

		int candidateVisibleIndex = widgetId - CANDIDATE_WIDGET_ID_BASE;
		if (candidateVisibleIndex >= 0 && candidateVisibleIndex < m_aVisibleCandidateIndexes.Count())
		{
			int candidateIndex = m_aVisibleCandidateIndexes[candidateVisibleIndex];
			if (candidateIndex >= 0 && candidateIndex < m_aCandidatePrefabs.Count())
			{
				string nodeId = m_sSelectedNodeId;
				if (nodeId.IsEmpty() && candidateIndex < m_aCandidateNodeIds.Count())
					nodeId = m_aCandidateNodeIds[candidateIndex];

				if (nodeId.IsEmpty())
					return true;

				m_sLastResult = "requested " + BuildNodeActionLabel();
				RequestServerAction(ResolveNodeSetCommand(nodeId), nodeId + ":" + m_aCandidatePrefabs[candidateIndex]);
				RenderEditor();
				return true;
			}
		}

		if (widgetId == REMOVE_SELECTED_NODE_WIDGET_ID)
		{
			if (m_sSelectedNodeId.IsEmpty())
				return true;

			m_sLastResult = "requested remove item";
			RequestServerAction(ResolveNodeRemoveCommand(m_sSelectedNodeId), m_sSelectedNodeId);
			RenderEditor();
			return true;
		}

		int nodeVisibleIndex = widgetId - NODE_WIDGET_ID_BASE;
		if (nodeVisibleIndex >= 0 && nodeVisibleIndex < m_aVisibleNodeIndexes.Count())
		{
			int nodeIndex = m_aVisibleNodeIndexes[nodeVisibleIndex];
			if (nodeIndex >= 0 && nodeIndex < m_aNodeIds.Count())
			{
				if (m_sEditorMode == "attachments" && nodeIndex < m_aNodeKinds.Count() && m_aNodeKinds[nodeIndex] == "weapon_slot")
				{
					m_sSelectedNodeId = m_aNodeIds[nodeIndex];
					m_sSelectedCategory = "attachment";
					m_sSelectedSlotId = "";
					m_bCandidateMode = false;
					m_iItemPage = 0;
					UpdatePreviewCamera(true);
					RenderEditor();
					return true;
				}

				if (nodeIndex < m_aNodeKinds.Count() && m_aNodeKinds[nodeIndex] == "storage_item")
				{
					if (button == 1)
					{
						m_sLastResult = "requested remove stored item";
						RequestServerAction("remove_node_item", m_aNodeIds[nodeIndex]);
						RenderEditor();
						return true;
					}

					if (nodeIndex < m_aNodePrefabs.Count() && !m_aNodePrefabs[nodeIndex].IsEmpty())
					{
						m_sLastResult = "requested add stored item";
						RequestServerAction("add_storage_item", m_aNodeIds[nodeIndex] + ":" + m_aNodePrefabs[nodeIndex]);
						RenderEditor();
						return true;
					}
				}

				m_sSelectedNodeId = m_aNodeIds[nodeIndex];
				m_sSelectedCategory = ResolveNodeCategory(nodeIndex);
				m_sSelectedSlotId = ResolveSlotIdFromNodeId(m_sSelectedNodeId);
				m_bCandidateMode = true;
				m_iItemPage = 0;
				EnsureCandidatePayloadForSelectedNode();
				UpdatePreviewCamera(true);
				RenderEditor();
				return true;
			}
		}

		int slotSelectIndex = widgetId - SLOT_SELECT_WIDGET_ID_BASE;
		if (slotSelectIndex >= 0 && slotSelectIndex < m_aSlotIds.Count())
		{
			m_sSelectedSlotId = m_aSlotIds[slotSelectIndex];
			if (FindStringIndex(m_aNodeIds, m_sSelectedSlotId) >= 0)
				m_sSelectedNodeId = m_sSelectedSlotId;
			else
				m_sSelectedNodeId = "node_" + m_sSelectedSlotId;
			m_bCandidateMode = true;
			m_sSelectedCategory = m_aSlotCategories[slotSelectIndex];
			m_sEditorMode = ResolveModeForCategory(m_sSelectedCategory);
			m_iItemPage = 0;
			EnsureCandidatePayloadForSelectedNode();
			UpdatePreviewCamera(true);
			RenderEditor();
			return true;
		}

		int slotRemoveIndex = widgetId - SLOT_REMOVE_WIDGET_ID_BASE;
		if (slotRemoveIndex >= 0 && slotRemoveIndex < m_aSlotIds.Count())
		{
			m_sLastResult = "requested remove slot";
			RequestServerAction("loadout_remove_slot", m_aSlotIds[slotRemoveIndex]);
			RenderEditor();
			return true;
		}

		int slotMinusIndex = widgetId - SLOT_MINUS_WIDGET_ID_BASE;
		if (slotMinusIndex >= 0 && slotMinusIndex < m_aSlotIds.Count())
		{
			int quantity = Math.Max(1, m_aSlotQuantities[slotMinusIndex] - 1);
			m_sLastResult = "requested quantity change";
			RequestServerAction("loadout_set_quantity", m_aSlotIds[slotMinusIndex] + ":" + quantity);
			RenderEditor();
			return true;
		}

		int slotPlusIndex = widgetId - SLOT_PLUS_WIDGET_ID_BASE;
		if (slotPlusIndex >= 0 && slotPlusIndex < m_aSlotIds.Count())
		{
			int quantity = Math.Max(1, m_aSlotQuantities[slotPlusIndex] + 1);
			m_sLastResult = "requested quantity change";
			RequestServerAction("loadout_set_quantity", m_aSlotIds[slotPlusIndex] + ":" + quantity);
			RenderEditor();
			return true;
		}

		int templateIndex = widgetId - TEMPLATE_WIDGET_ID_BASE;
		if (templateIndex >= 0 && templateIndex < m_aTemplateIds.Count())
		{
			m_sSelectedTemplateId = m_aTemplateIds[templateIndex];
			m_sLastResult = "requested select template";
			RequestServerAction("loadout_select", m_sSelectedTemplateId);
			RenderEditor();
			return true;
		}

		return false;
	}

	void OnServerActionResult(string payload, string lastResult)
	{
		if (!m_bEditorOpen)
			return;

		if (payload.Contains("HST_LOADOUT_CANDIDATES"))
		{
			if (!lastResult.IsEmpty())
				m_sLastResult = lastResult;

			ParseCandidatePayload(payload);
			RenderEditor();
			return;
		}

		if (!payload.Contains("HST_LOADOUT_EDITOR"))
			return;

		if (!lastResult.IsEmpty())
			m_sLastResult = lastResult;

		ParseEditorPayload(payload);
		if (m_bCandidateMode || m_sEditorMode == "storage")
			EnsureCandidatePayloadForSelectedNode();
		if (ShouldQueuePostActionRefresh(lastResult))
			QueuePostActionRefresh();
		RenderEditor();
	}

	protected bool ShouldQueuePostActionRefresh(string lastResult)
	{
		if (lastResult.IsEmpty())
			return false;

		return lastResult.Contains("| set ") || lastResult.Contains("| added ") || lastResult.Contains("| removed ") || lastResult.Contains("| cleared ") || lastResult.Contains("| applied ");
	}

	protected void QueuePostActionRefresh()
	{
		if (m_bPostActionRefreshQueued)
			return;

		m_bPostActionRefreshQueued = true;
		GetGame().GetCallqueue().CallLater(RequestPostActionRefresh, POST_ACTION_REFRESH_DELAY_MS, false);
	}

	protected void RequestPostActionRefresh()
	{
		m_bPostActionRefreshQueued = false;
		if (!m_bEditorOpen)
			return;

		RequestServerAction("loadout_editor_refresh", "");
	}

	void CloseEditor(bool requestServer)
	{
		if (!m_bEditorOpen)
			return;

		if (requestServer)
			RequestServerAction("loadout_editor_close", "");

		m_bEditorOpen = false;
		m_bPostActionRefreshQueued = false;
		ClearWidgets();
		DeleteEditorRoot();
		DeletePreviewWorld();
		ShowHint("Loadout editor closed");
	}

	protected void RequestServerAction(string commandId, string argument)
	{
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (request)
		{
			request.RequestLoadoutEditorAction(commandId, argument);
			return;
		}

		HST_CampaignCoordinatorComponent coordinator = HST_CampaignCoordinatorComponent.GetInstance();
		if (coordinator && Replication.IsServer())
		{
			int playerId = ResolveLocalPlayerId();
			m_sLastResult = coordinator.RequestLoadoutEditorCommand(playerId, commandId, argument);
			if (commandId == "loadout_editor_candidates")
				OnServerActionResult(coordinator.BuildLoadoutEditorCandidatePayload(playerId, argument), m_sLastResult);
			else
				OnServerActionResult(coordinator.BuildLoadoutEditorPayload(playerId), m_sLastResult);
			return;
		}

		m_sLastResult = "player request bridge not ready";
	}

	protected void RenderEditor()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		ClearWidgets();
		if (!m_WidgetHandler)
		{
			m_WidgetHandler = new HST_LoadoutEditorWidgetHandler();
			m_WidgetHandler.Bind(this);
		}

		m_iEditorWidth = Math.Max(1280, Math.Round(workspace.GetWidth()));
		m_iEditorHeight = Math.Max(850, Math.Round(workspace.GetHeight()));

		Widget root = EnsureEditorRoot(workspace);

		if (!root)
			return;

		if (!m_bRootFromLayout)
		{
			FrameSlot.SetPos(root, 0, 0);
			FrameSlot.SetSize(root, m_iEditorWidth, m_iEditorHeight);
		}

		root.SetZOrder(3600);

		m_PreviewWidget = RenderTargetWidget.Cast(root.FindAnyWidget("HST_LoadoutPreview"));
		ConfigurePreviewWidget();
		EnsurePreviewWorld();
		RefreshPreviewWorldLoadout();

		Widget uiRoot = ResolveUILayer(root);
		CreateButton(workspace, uiRoot, "ESC", 32, Math.Max(320, (m_iEditorHeight / 2) + 30), 58, 32, CLOSE_WIDGET_ID);
		CreateTextWidget(workspace, uiRoot, "<", 44, Math.Max(286, (m_iEditorHeight / 2) - 22), 34, 44, 36, 0xFFC4953B, BACK_WIDGET_ID, true);

		RenderModeTabs(workspace, uiRoot);
		if (m_sEditorMode == "storage")
		{
			RenderStorageRail(workspace, uiRoot);
			RenderStorageCandidatePanel(workspace, uiRoot);
		}
		else
		{
			RenderSlotRail(workspace, uiRoot);
			RenderCandidatePanel(workspace, uiRoot);
		}
		RenderPreviewStage(workspace, uiRoot);
		RenderFooter(workspace, uiRoot);
	}

	protected Widget EnsureEditorRoot(WorkspaceWidget workspace)
	{
		if (m_RootWidget)
			return m_RootWidget;

		m_RootWidget = workspace.CreateWidgets(EDITOR_LAYOUT);
		m_bRootFromLayout = m_RootWidget != null;
		if (!m_RootWidget)
			m_RootWidget = workspace.CreateWidgetInWorkspace(WidgetType.FrameWidgetTypeID, 0, 0, m_iEditorWidth, m_iEditorHeight, WidgetFlags.VISIBLE, null, 3600);

		m_UILayerWidget = null;
		return m_RootWidget;
	}

	protected Widget ResolveUILayer(Widget root)
	{
		if (!root)
			return null;

		if (!m_UILayerWidget)
			m_UILayerWidget = root.FindAnyWidget("HST_LoadoutUILayer");

		if (m_UILayerWidget)
		{
			m_UILayerWidget.SetZOrder(10);
			return m_UILayerWidget;
		}

		return root;
	}

	protected void DeleteEditorRoot()
	{
		if (m_RootWidget)
			m_RootWidget.RemoveFromHierarchy();

		m_RootWidget = null;
		m_UILayerWidget = null;
		m_bRootFromLayout = false;
		m_PreviewWidget = null;
	}

	protected bool IsDuplicateWidgetActivation(int widgetId, int button)
	{
		if (widgetId == 0)
			return false;

		if (m_iLastActivatedFrame == m_iFrameSerial && m_iLastActivatedWidgetId == widgetId && m_iLastActivatedButton == button)
			return true;

		m_iLastActivatedWidgetId = widgetId;
		m_iLastActivatedButton = button;
		m_iLastActivatedFrame = m_iFrameSerial;
		return false;
	}

	protected void RenderModeTabs(WorkspaceWidget workspace, Widget root)
	{
		int tabStart = 92;
		int tabTop = 80;
		int tabLeft = tabStart;
		CreateButton(workspace, root, "Q", 66, 100, 22, 22, MODE_WIDGET_ID_BASE + Math.Max(0, GetEditorModeIndex(m_sEditorMode) - 1));
		CreateRectWidget(workspace, root, tabStart, tabTop, 430, 62, 0xF20D1114, 1.0, 0);
		for (int i = 0; i < GetEditorModeCount(); i++)
		{
			string modeId = GetEditorModeId(i);
			int color = 0xFF12171B;
			int tabWidth = 54;
			if (modeId == m_sEditorMode)
			{
				color = 0xFFC4953B;
				tabWidth = 112;
			}

			CreateRectWidget(workspace, root, tabLeft, tabTop, tabWidth, 58, color, 0.98, MODE_WIDGET_ID_BASE + i);
			if (!CreateIconWidget(workspace, root, ResolveIconTexture(modeId), tabLeft + 14, tabTop + 17, 24, 24, MODE_WIDGET_ID_BASE + i, 0xFFFFFFFF))
				CreateTextWidget(workspace, root, GetEditorModeIcon(modeId), tabLeft + 14, tabTop + 17, 24, 22, 18, 0xFFFFFFFF, MODE_WIDGET_ID_BASE + i, true);
			if (modeId == m_sEditorMode)
				CreateTextWidget(workspace, root, GetEditorModeLabel(modeId), tabLeft + 46, tabTop + 18, 62, 20, 12, 0xFFFFFFFF, MODE_WIDGET_ID_BASE + i, true);

			tabLeft += tabWidth;
		}
		CreateButton(workspace, root, "E", tabLeft + 6, 100, 22, 22, MODE_WIDGET_ID_BASE + Math.Min(GetEditorModeCount() - 1, GetEditorModeIndex(m_sEditorMode) + 1));
		CreateRectWidget(workspace, root, tabStart, 140, tabLeft - tabStart, 3, 0xFFC4953B, 1.0, 0);
	}

	protected void RenderSlotRail(WorkspaceWidget workspace, Widget root)
	{
		if (m_bCandidateMode || m_sEditorMode == "settings" || m_sEditorMode == "save")
			return;

		CreateRectWidget(workspace, root, 92, 144, 390, Math.Max(420, m_iEditorHeight - 238), 0xEA0D1114, 1.0, 0);
		m_aVisibleNodeIndexes.Clear();
		int visibleCount = CountVisibleNodesForCurrentMode();
		int maxPage = GetMaxPage(visibleCount, SLOTS_PER_PAGE);
		m_iSlotPage = Math.Min(Math.Max(0, m_iSlotPage), maxPage);
		int startIndex = m_iSlotPage * SLOTS_PER_PAGE;
		int nodeOrdinal;
		int row;
		for (int nodeIndex = 0; nodeIndex < m_aNodeIds.Count(); nodeIndex++)
		{
			if (!IsNodeVisibleInCurrentMode(nodeIndex))
				continue;

			if (nodeOrdinal < startIndex)
			{
				nodeOrdinal++;
				continue;
			}

			if (row >= SLOTS_PER_PAGE)
				break;

			m_aVisibleNodeIndexes.Insert(nodeIndex);
			RenderNodeRow(workspace, root, nodeIndex, row, NODE_WIDGET_ID_BASE + row);
			nodeOrdinal++;
			row++;
		}

		if (row == 0)
			CreateTextWidget(workspace, root, "Empty Slot", 146, 176, 220, 22, 13, 0xFFC8D0D4, 0, false);

		int pagerTop = Math.Min(m_iEditorHeight - 116, 166 + SLOTS_PER_PAGE * 60);
		CreateButton(workspace, root, "<", 118, pagerTop, 32, 24, SLOT_PAGE_PREV_WIDGET_ID);
		CreateTextWidget(workspace, root, string.Format("%1 / %2", m_iSlotPage + 1, maxPage + 1), 160, pagerTop + 4, 56, 16, 11, 0xFFE2E6E8, 0, true);
		CreateButton(workspace, root, ">", 226, pagerTop, 32, 24, SLOT_PAGE_NEXT_WIDGET_ID);
	}

	protected void RenderStorageRail(WorkspaceWidget workspace, Widget root)
	{
		EnsureSelectedStorageNode();
		CreateRectWidget(workspace, root, 92, 144, 390, Math.Max(420, m_iEditorHeight - 238), 0xEA0D1114, 1.0, 0);
		CreateTextWidget(workspace, root, "Inventory Storage", 118, 162, 190, 22, 16, 0xFFEFE2C4, 0, true);
		m_aVisibleNodeIndexes.Clear();

		int visibleIndex;
		int rowTop = 198;
		for (int nodeIndex = 0; nodeIndex < m_aNodeKinds.Count(); nodeIndex++)
		{
			if (m_aNodeKinds[nodeIndex] != "storage")
				continue;

			if (visibleIndex >= 4)
				break;

			m_aVisibleNodeIndexes.Insert(nodeIndex);
			RenderStorageNodeRow(workspace, root, nodeIndex, 112, rowTop + visibleIndex * 54, 350, NODE_WIDGET_ID_BASE + visibleIndex, false);
			visibleIndex++;
		}

		int contentsTop = rowTop + Math.Max(1, visibleIndex) * 54 + 22;
		CreateRectWidget(workspace, root, 112, contentsTop - 10, 350, 2, 0xFFC4953B, 1.0, 0);
		CreateTextWidget(workspace, root, "Contents", 118, contentsTop, 160, 20, 13, 0xFFFFD166, 0, true);
		int contentRows;
		for (int nodeIndex = 0; nodeIndex < m_aNodeKinds.Count(); nodeIndex++)
		{
			if (m_aNodeKinds[nodeIndex] != "storage_item")
				continue;
			if (nodeIndex >= m_aNodeParents.Count() || m_aNodeParents[nodeIndex] != m_sSelectedNodeId)
				continue;

			if (contentRows >= 7)
				break;

			m_aVisibleNodeIndexes.Insert(nodeIndex);
			RenderStorageNodeRow(workspace, root, nodeIndex, 112, contentsTop + 28 + contentRows * 50, 350, NODE_WIDGET_ID_BASE + m_aVisibleNodeIndexes.Count() - 1, true);
			contentRows++;
		}

		if (visibleIndex == 0)
			CreateTextWidget(workspace, root, "No equipped storage.", 126, rowTop, 240, 22, 13, 0xFFB7C7D7, 0, false);
		else if (contentRows == 0)
			CreateTextWidget(workspace, root, "Empty", 126, contentsTop + 34, 180, 20, 13, 0xFFB7C7D7, 0, false);
	}

	protected void RenderStorageCandidatePanel(WorkspaceWidget workspace, Widget root)
	{
		EnsureSelectedStorageNode();
		int panelLeft = 520;
		int panelTop = 112;
		int panelWidth = Math.Min(900, Math.Max(640, m_iEditorWidth - panelLeft - 260));
		int panelHeight = Math.Min(560, Math.Max(468, m_iEditorHeight - 280));
		CreateRectWidget(workspace, root, panelLeft, panelTop, panelWidth, panelHeight, 0x940D1114, 1.0, 0);
		CreateRectWidget(workspace, root, panelLeft, panelTop, panelWidth, 3, 0xFFC4953B, 1.0, 0);
		CreateTextWidget(workspace, root, "Add Items", panelLeft + 20, panelTop + 18, 130, 22, 17, 0xFFEFE2C4, 0, true);
		CreateTextWidget(workspace, root, ShortenText(BuildStorageTargetLabel(), 54), panelLeft + 150, panelTop + 20, 430, 18, 12, 0xFFB7C7D7, 0, false);
		RenderStorageCategoryTabs(workspace, root, panelLeft + 20, panelTop + 54, panelWidth - 40);

		m_aVisibleCandidateIndexes.Clear();
		if (m_sSelectedNodeId.IsEmpty())
		{
			CreateTextWidget(workspace, root, "Select equipped storage on the left.", panelLeft + 24, panelTop + 112, 320, 24, 14, 0xFFB7C7D7, 0, false);
			return;
		}

		int itemCount = CountStorageCandidatesForSelectedNode();
		int maxPage = GetMaxPage(itemCount, STORAGE_ITEMS_PER_PAGE);
		m_iItemPage = Math.Min(Math.Max(0, m_iItemPage), maxPage);
		int startIndex = m_iItemPage * STORAGE_ITEMS_PER_PAGE;
		int candidateOrdinal;
		int visibleIndex;
		int cols = panelWidth / 280;
		if (cols < 2)
			cols = 2;
		if (cols > 4)
			cols = 4;
		int tileWidth = Math.Max(220, (panelWidth - 48 - (cols - 1) * 14) / cols);
		int firstTop = panelTop + 102;
		for (int candidateIndex = 0; candidateIndex < m_aCandidatePrefabs.Count(); candidateIndex++)
		{
			if (!IsCandidateVisibleForStorageBrowser(candidateIndex))
				continue;

			if (candidateOrdinal < startIndex)
			{
				candidateOrdinal++;
				continue;
			}

			if (visibleIndex >= STORAGE_ITEMS_PER_PAGE)
				break;

			int col = visibleIndex % cols;
			int row = visibleIndex / cols;
			int left = panelLeft + 22 + col * (tileWidth + 14);
			int top = firstTop + row * 54;
			m_aVisibleCandidateIndexes.Insert(candidateIndex);
			RenderStorageCandidateTile(workspace, root, candidateIndex, left, top, tileWidth, CANDIDATE_WIDGET_ID_BASE + visibleIndex);
			visibleIndex++;
			candidateOrdinal++;
		}

		if (visibleIndex == 0)
		{
			string emptyText = ResolveCandidateEmptyText(m_sSelectedNodeId, "No compatible items in this category.");
			if (FindStringIndex(m_aRequestedCandidateNodeIds, m_sSelectedNodeId) >= 0)
				emptyText = "Loading compatible items...";
			CreateTextWidget(workspace, root, emptyText, panelLeft + 24, firstTop + 8, 340, 24, 14, 0xFFB7C7D7, 0, false);
		}

		int pagerTop = Math.Min(m_iEditorHeight - 116, panelTop + panelHeight - 42);
		CreateButton(workspace, root, "<", panelLeft + 22, pagerTop, 32, 24, ITEM_PAGE_PREV_WIDGET_ID);
		CreateTextWidget(workspace, root, string.Format("%1 / %2", m_iItemPage + 1, maxPage + 1), panelLeft + 64, pagerTop + 4, 56, 16, 11, 0xFFE2E6E8, 0, true);
		CreateButton(workspace, root, ">", panelLeft + 130, pagerTop, 32, 24, ITEM_PAGE_NEXT_WIDGET_ID);
	}

	protected void RenderPreviewStage(WorkspaceWidget workspace, Widget root)
	{
		string toast = BuildStageToast();
		if (!toast.IsEmpty())
			CreateTextWidget(workspace, root, ShortenText(toast, 58), Math.Max(560, (m_iEditorWidth / 2) - 190), 54, 460, 24, 15, 0xFFEFC16D, 0, true);

		if (!m_PreviewWidget || !m_PreviewWorld)
			CreateTextWidget(workspace, root, "Preview target unavailable", Math.Max(560, (m_iEditorWidth / 2) - 120), 348, 280, 24, 16, 0xFFFFD166, 0, true);
	}

	protected string BuildStageToast()
	{
		if (!m_sLastResult.IsEmpty())
		{
			if (m_sLastResult.Contains("opened custom arsenal editor"))
				return "";

			string result = m_sLastResult;
			result.Replace("h-istasi loadout editor | ", "");
			result.Replace("h-istasi arsenal | ", "");
			if (result.Contains("requested "))
				return "";
			return result;
		}

		if (!m_sPreviewStatus.IsEmpty() && m_sPreviewStatus.Contains("skipped"))
			return m_sPreviewStatus;

		if (!m_bPreviewSpawned && !m_sPreviewStatus.IsEmpty())
			return m_sPreviewStatus;

		return "";
	}

	protected void RenderCandidatePanel(WorkspaceWidget workspace, Widget root)
	{
		if (m_sEditorMode == "save")
		{
			RenderTemplatePanel(workspace, root);
			return;
		}

		if (m_sEditorMode == "settings")
		{
			RenderSettingsPanel(workspace, root);
			return;
		}

		if (!m_bCandidateMode)
			return;

		int panelTop = 144;
		int panelHeight = Math.Max(420, m_iEditorHeight - 238);
		CreateRectWidget(workspace, root, 92, panelTop, 390, panelHeight, 0xEA0D1114, 1.0, 0);
		CreateRectWidget(workspace, root, 92, panelTop + 64, 390, 3, 0xFFC4953B, 1.0, 0);
		RenderSelectedNodeHeader(workspace, root);
		int selectedNodeIndex = FindSelectedNodeIndex();
		if (selectedNodeIndex >= 0 && selectedNodeIndex < m_aNodeCanRemove.Count() && m_aNodeCanRemove[selectedNodeIndex])
		{
			CreateRectWidget(workspace, root, 112, panelTop + 86, 350, 56, 0xFF15191C, 0.98, REMOVE_SELECTED_NODE_WIDGET_ID);
			CreateTextWidget(workspace, root, "X", 132, panelTop + 93, 44, 38, 34, 0xFFFFFFFF, REMOVE_SELECTED_NODE_WIDGET_ID, true);
			CreateTextWidget(workspace, root, "Remove", 194, panelTop + 103, 160, 22, 16, 0xFFE2E6E8, REMOVE_SELECTED_NODE_WIDGET_ID, true);
		}

		m_aVisibleCandidateIndexes.Clear();
		int itemCount = CountCandidatesForSelectedNode();
		int maxPage = GetMaxPage(itemCount, ITEMS_PER_PAGE);
		m_iItemPage = Math.Min(Math.Max(0, m_iItemPage), maxPage);
		int startIndex = m_iItemPage * ITEMS_PER_PAGE;
		int visibleIndex = 0;
		int candidateOrdinal = 0;
		int firstRowTop = panelTop + 154;
		for (int candidateIndex = 0; candidateIndex < m_aCandidatePrefabs.Count(); candidateIndex++)
		{
			if (!IsCandidateVisibleForSelectedNode(candidateIndex))
				continue;

			if (candidateOrdinal < startIndex)
			{
				candidateOrdinal++;
				continue;
			}

			if (visibleIndex >= ITEMS_PER_PAGE)
				break;

			int top = firstRowTop + visibleIndex * 54;
			m_aVisibleCandidateIndexes.Insert(candidateIndex);
			RenderCandidateRow(workspace, root, candidateIndex, top, CANDIDATE_WIDGET_ID_BASE + visibleIndex);
			visibleIndex++;
			candidateOrdinal++;
		}

		if (visibleIndex == 0)
		{
			string emptyText = ResolveCandidateEmptyText(m_sSelectedNodeId, "No compatible recovered items.");
			if (FindStringIndex(m_aRequestedCandidateNodeIds, m_sSelectedNodeId) >= 0)
				emptyText = "Loading compatible items...";
			CreateTextWidget(workspace, root, emptyText, 126, firstRowTop + 8, 260, 22, 13, 0xFFB7C7D7, 0, false);
		}

		int pagerTop = Math.Min(m_iEditorHeight - 116, firstRowTop + ITEMS_PER_PAGE * 54 + 8);
		CreateButton(workspace, root, "<", 118, pagerTop, 32, 24, ITEM_PAGE_PREV_WIDGET_ID);
		CreateTextWidget(workspace, root, string.Format("%1 / %2", m_iItemPage + 1, maxPage + 1), 160, pagerTop + 4, 56, 16, 11, 0xFFE2E6E8, 0, true);
		CreateButton(workspace, root, ">", 226, pagerTop, 32, 24, ITEM_PAGE_NEXT_WIDGET_ID);
	}

	protected void RenderTemplatePanel(WorkspaceWidget workspace, Widget root)
	{
		CreateRectWidget(workspace, root, 92, 144, 390, Math.Max(420, m_iEditorHeight - 238), 0xEA0D1114, 1.0, 0);
		CreateRectWidget(workspace, root, 92, 144, 390, 3, 0xFFC4953B, 1.0, 0);
		CreateTextWidget(workspace, root, "Save Loadout", 126, 166, 190, 24, 18, 0xFFEFE2C4, 0, true);
		int maxTemplatePage = GetMaxPage(m_aTemplateIds.Count(), TEMPLATES_PER_PAGE);
		m_iTemplatePage = Math.Min(Math.Max(0, m_iTemplatePage), maxTemplatePage);
		int firstTemplate = m_iTemplatePage * TEMPLATES_PER_PAGE;
		int templateLimit = Math.Min(m_aTemplateIds.Count(), firstTemplate + TEMPLATES_PER_PAGE);
		for (int templateIndex = firstTemplate; templateIndex < templateLimit; templateIndex++)
		{
			int visibleTemplate = templateIndex - firstTemplate;
			int topTemplate = 222 + visibleTemplate * 54;
			int color = 0xFF1B252E;
			if (m_aTemplateIds[templateIndex] == m_sSelectedTemplateId)
				color = 0xFF6F5124;

			CreateRectWidget(workspace, root, 112, topTemplate, 350, 48, color, 0.96, TEMPLATE_WIDGET_ID_BASE + templateIndex);
			CreateTextWidget(workspace, root, ShortenText(m_aTemplateDisplays[templateIndex], 28), 172, topTemplate + 8, 198, 17, 13, 0xFFE2E6E8, TEMPLATE_WIDGET_ID_BASE + templateIndex, m_aTemplateIds[templateIndex] == m_sSelectedTemplateId);
			CreateTextWidget(workspace, root, string.Format("%1 slots", m_aTemplateSlotCounts[templateIndex]), 172, topTemplate + 26, 100, 15, 10, 0xFFB7C7D7, TEMPLATE_WIDGET_ID_BASE + templateIndex, false);
			CreateTextWidget(workspace, root, "O", 132, topTemplate + 12, 30, 25, 18, 0xFFFFFFFF, TEMPLATE_WIDGET_ID_BASE + templateIndex, true);
		}

		if (m_aTemplateIds.Count() == 0)
			CreateTextWidget(workspace, root, "Empty Slot", 126, 222, 230, 22, 13, 0xFFB7C7D7, 0, false);

		CreateButton(workspace, root, "Save", 320, 166, 62, 26, SAVE_WIDGET_ID);
		CreateButton(workspace, root, "Clear", 392, 166, 58, 26, DELETE_TEMPLATE_WIDGET_ID);
		int pagerTop = Math.Min(m_iEditorHeight - 116, 222 + TEMPLATES_PER_PAGE * 54 + 8);
		CreateButton(workspace, root, "<", 118, pagerTop, 32, 24, TEMPLATE_PAGE_PREV_WIDGET_ID);
		CreateTextWidget(workspace, root, string.Format("%1 / %2", m_iTemplatePage + 1, maxTemplatePage + 1), 160, pagerTop + 4, 56, 16, 11, 0xFFE2E6E8, 0, true);
		CreateButton(workspace, root, ">", 226, pagerTop, 32, 24, TEMPLATE_PAGE_NEXT_WIDGET_ID);
	}

	protected void RenderSettingsPanel(WorkspaceWidget workspace, Widget root)
	{
		CreateRectWidget(workspace, root, 92, 144, 390, Math.Max(420, m_iEditorHeight - 238), 0xEA0D1114, 1.0, 0);
		CreateRectWidget(workspace, root, 92, 144, 390, 3, 0xFFC4953B, 1.0, 0);
		string panelTitle = "Settings";
		if (m_sEditorMode == "save")
			panelTitle = "Save Loadout";
		CreateTextWidget(workspace, root, panelTitle, 126, 166, 190, 24, 18, 0xFFEFE2C4, 0, true);
		if (m_sEditorMode == "save")
		{
			CreateTextWidget(workspace, root, "Save current draft or apply it from the footer.", 126, 214, 300, 34, 13, 0xFFE2E6E8, 0, false);
			CreateTextWidget(workspace, root, BuildDraftHeaderSummary(), 126, 268, 300, 20, 13, 0xFFFFD166, 0, true);
			CreateButton(workspace, root, "Save", 126, 340, 88, 32, SAVE_WIDGET_ID);
			CreateButton(workspace, root, "Apply", 230, 340, 92, 32, APPLY_WIDGET_ID);
			return;
		}
		CreateTextWidget(workspace, root, "Camera", 126, 232, 88, 20, 13, 0xFFFFD166, 0, true);
		CreateTextWidget(workspace, root, BuildCameraModeLabel(), 220, 232, 160, 20, 13, 0xFFE2E6E8, 0, false);
		CreateTextWidget(workspace, root, ShortenText(BuildStageToast(), 48), 126, 290, 280, 54, 13, 0xFFD7B66F, 0, false);
		CreateTextWidget(workspace, root, "Preview focus", 126, 424, 112, 20, 13, 0xFFFFD166, 0, true);
		CreateTextWidget(workspace, root, ShortenText(BuildSelectedPreviewTitle(), 32), 242, 424, 180, 20, 13, 0xFFE2E6E8, 0, false);
		CreateTextWidget(workspace, root, "Rotation", 126, 460, 112, 20, 13, 0xFFFFD166, 0, true);
		CreateTextWidget(workspace, root, string.Format("%1 deg", Math.Round(m_fPreviewYawDegrees)), 242, 460, 90, 20, 13, 0xFFE2E6E8, 0, false);
		CreateButton(workspace, root, "Rotate", 126, 500, 92, 32, ROTATE_PREVIEW_WIDGET_ID);
		CreateButton(workspace, root, "Reset", 234, 500, 82, 32, RESET_PREVIEW_WIDGET_ID);
	}

	protected void RenderFooter(WorkspaceWidget workspace, Widget root)
	{
		int footerTop = m_iEditorHeight - 76;
		int buttonLeft = Math.Max(820, m_iEditorWidth - 440);
		CreateButton(workspace, root, "<", buttonLeft, footerTop + 24, 44, 30, BACK_WIDGET_ID);
		string actionLabel = BuildNodeActionLabel();
		int actionWidgetId = SWAP_WIDGET_ID;
		if (m_sEditorMode == "save")
		{
			actionLabel = "Load";
			actionWidgetId = APPLY_WIDGET_ID;
		}
		CreateButton(workspace, root, actionLabel, buttonLeft + 58, footerTop + 24, 92, 30, actionWidgetId);
		CreateButton(workspace, root, BuildCameraModeLabel(), buttonLeft + 164, footerTop + 20, 88, 38, CAMERA_WIDGET_ID);
		if (m_sEditorMode == "save")
		{
			CreateButton(workspace, root, "Save", buttonLeft - 182, footerTop + 24, 72, 30, SAVE_WIDGET_ID);
			CreateButton(workspace, root, "Load", buttonLeft - 98, footerTop + 24, 74, 30, APPLY_WIDGET_ID);
		}
	}

	protected void ParseEditorPayload(string payload)
	{
		if (payload.IsEmpty())
			return;

		ClearEditorPayload();
		array<string> lines = {};
		payload.Split("\n", lines, false);
		foreach (string line : lines)
		{
			array<string> fields = {};
			line.Split("|", fields, false);
			if (fields.Count() == 0)
				continue;

			if (fields[0] == "HST_LOADOUT_EDITOR")
			{
				if (fields.Count() > 1)
					m_sStatusText = "Editor " + fields[1];
				if (fields.Count() > 2)
					m_sSelectedTemplateId = fields[2];
				if (fields.Count() > 3)
					m_bPreviewSpawned = fields[3] == "true" || fields[3] == "1";
				if (fields.Count() > 4)
					m_iDraftSlotCount = fields[4].ToInt();
				if (fields.Count() > 5)
					m_iDraftItemCount = fields[5].ToInt();
				if (fields.Count() > 6)
					m_iDraftFiniteCount = fields[6].ToInt();
				if (fields.Count() > 7)
					m_iDraftInfiniteCount = fields[7].ToInt();
				continue;
			}

			if (fields[0] == "PREVIEW" && fields.Count() >= 5)
			{
				m_bPreviewSpawned = fields[1] == "true" || fields[1] == "1";
				m_sPreviewPosition = fields[2];
				m_iPreviewItemCount = fields[3].ToInt();
				m_sPreviewStatus = fields[4];
				continue;
			}

			if (fields[0] == "PREVIEW_PREFAB" && fields.Count() >= 2)
			{
				m_sPreviewPrefab = fields[1];
				continue;
			}

			if (fields[0] == "CATEGORY" && fields.Count() >= 4)
			{
				m_aCategoryIds.Insert(fields[1]);
				m_aCategoryLabels.Insert(fields[2]);
				m_aCategoryCounts.Insert(fields[3].ToInt());
				continue;
			}

			if (fields[0] == "ITEM" && fields.Count() >= 6)
			{
				if (IsRemovedExternalPrefab(fields[2]))
					continue;

				m_aItemCategories.Insert(fields[1]);
				m_aItemPrefabs.Insert(fields[2]);
				string itemDisplay = ResolvePayloadDisplayText(fields[3]);
				string itemShortDisplay = itemDisplay;
				string itemSlotLabel = BuildSlotCategoryLabel(fields[1]);
				bool itemPreviewEligible = IsPreviewEligibleCategory(fields[1]);
				if (fields.Count() > 6)
					itemDisplay = ResolvePayloadDisplayText(fields[6]);
				if (fields.Count() > 7)
					itemShortDisplay = ResolvePayloadDisplayText(fields[7]);
				if (fields.Count() > 8)
					itemSlotLabel = fields[8];
				if (fields.Count() > 9)
					itemPreviewEligible = ParsePayloadBool(fields[9], itemPreviewEligible);

				itemDisplay = HST_DisplayNameService.ResolveItemDisplayName(null, fields[2], itemDisplay);
				itemShortDisplay = HST_DisplayNameService.ResolveShortItemDisplayName(itemShortDisplay, fields[2]);
				if (itemShortDisplay.IsEmpty())
					itemShortDisplay = itemDisplay;
				m_aItemDisplays.Insert(itemDisplay);
				m_aItemShortDisplays.Insert(itemShortDisplay);
				m_aItemSlotLabels.Insert(itemSlotLabel);
				m_aItemCounts.Insert(fields[4]);
				m_aItemInfinite.Insert(fields[5] == "INF");
				m_aItemPreviewEligible.Insert(itemPreviewEligible);
				continue;
			}

			if (fields[0] == "SLOT" && fields.Count() >= 6)
			{
				if (IsRemovedExternalPrefab(fields[3]))
					continue;

				m_aSlotIds.Insert(fields[1]);
				m_aSlotCategories.Insert(fields[2]);
				m_aSlotPrefabs.Insert(fields[3]);
				string slotDisplay = ResolvePayloadDisplayText(fields[4]);
				string slotShortDisplay = slotDisplay;
				string slotLabel = BuildSlotCategoryLabel(fields[2]);
				bool slotPreviewEligible = IsPreviewEligibleCategory(fields[2]);
				if (fields.Count() > 6)
					slotDisplay = ResolvePayloadDisplayText(fields[6]);
				if (fields.Count() > 7)
					slotShortDisplay = ResolvePayloadDisplayText(fields[7]);
				if (fields.Count() > 8)
					slotLabel = fields[8];
				if (fields.Count() > 9)
					slotPreviewEligible = ParsePayloadBool(fields[9], slotPreviewEligible);

				slotDisplay = HST_DisplayNameService.ResolveItemDisplayName(null, fields[3], slotDisplay);
				slotShortDisplay = HST_DisplayNameService.ResolveShortItemDisplayName(slotShortDisplay, fields[3]);
				if (slotShortDisplay.IsEmpty())
					slotShortDisplay = slotDisplay;
				m_aSlotDisplays.Insert(slotDisplay);
				m_aSlotShortDisplays.Insert(slotShortDisplay);
				m_aSlotLabels.Insert(slotLabel);
				m_aSlotQuantities.Insert(Math.Max(1, fields[5].ToInt()));
				m_aSlotPreviewEligible.Insert(slotPreviewEligible);
				continue;
			}

			if (fields[0] == "NODE" && fields.Count() >= 13)
			{
				if (IsRemovedExternalPrefab(fields[6]))
					continue;

				m_aNodeIds.Insert(fields[1]);
				m_aNodeParents.Insert(fields[2]);
				m_aNodeKinds.Insert(fields[3]);
				m_aNodeSlotKeys.Insert(fields[4]);
				m_aNodeLabels.Insert(fields[5]);
				m_aNodePrefabs.Insert(fields[6]);
				string nodeDisplay = ResolvePayloadDisplayText(fields[7]);
				if (!fields[6].IsEmpty())
					nodeDisplay = HST_DisplayNameService.ResolveItemDisplayName(null, fields[6], nodeDisplay);
				if (nodeDisplay.IsEmpty())
					nodeDisplay = "Empty Slot";
				m_aNodeDisplays.Insert(nodeDisplay);
				m_aNodeCounts.Insert(fields[8]);
				m_aNodeInfinite.Insert(ParsePayloadBool(fields[9], false));
				m_aNodeCanOpen.Insert(ParsePayloadBool(fields[10], false));
				m_aNodeCanRemove.Insert(ParsePayloadBool(fields[11], false));
				m_aNodeCanDeposit.Insert(ParsePayloadBool(fields[12], false));
				if (fields.Count() > 13)
					m_aNodeFocus.Insert(fields[13]);
				else
					m_aNodeFocus.Insert(fields[4]);
				continue;
			}

			if (fields[0] == "CANDIDATE" && fields.Count() >= 9)
			{
				if (IsRemovedExternalPrefab(fields[2]))
					continue;

				m_aCandidateNodeIds.Insert(fields[1]);
				m_aCandidatePrefabs.Insert(fields[2]);
				string candidateDisplay = ResolvePayloadDisplayText(fields[3]);
				candidateDisplay = HST_DisplayNameService.ResolveItemDisplayName(null, fields[2], candidateDisplay);
				m_aCandidateDisplays.Insert(candidateDisplay);
				string candidateShort = ResolvePayloadDisplayText(fields[4]);
				candidateShort = HST_DisplayNameService.ResolveShortItemDisplayName(candidateShort, fields[2]);
				if (candidateShort.IsEmpty())
					candidateShort = candidateDisplay;
				m_aCandidateShortDisplays.Insert(candidateShort);
				m_aCandidateCounts.Insert(fields[5]);
				m_aCandidateInfinite.Insert(fields[6] == "INF");
				m_aCandidateKinds.Insert(fields[7]);
				m_aCandidateCompatible.Insert(ParsePayloadBool(fields[8], true));
				if (fields.Count() > 9)
					m_aCandidateIconHints.Insert(fields[9]);
				else
					m_aCandidateIconHints.Insert(fields[7]);
				if (fields.Count() > 10)
					m_aCandidateAmmoMatch.Insert(ParsePayloadBool(fields[10], false));
				else
					m_aCandidateAmmoMatch.Insert(false);
				continue;
			}

			if (fields[0] == "TEMPLATE" && fields.Count() >= 4)
			{
				m_aTemplateIds.Insert(fields[1]);
				m_aTemplateDisplays.Insert(fields[2]);
				m_aTemplateSlotCounts.Insert(fields[3].ToInt());
			}
		}

		if (m_aCategoryIds.Count() == 0)
			BuildFallbackCategories();

		if (FindCategoryIndex(m_sSelectedCategory) < 0 && m_aCategoryIds.Count() > 0)
			m_sSelectedCategory = m_aCategoryIds[0];

		if (!m_sSelectedTemplateId.IsEmpty() && FindTemplateIndex(m_sSelectedTemplateId) < 0)
			m_sSelectedTemplateId = "";

		if (!m_sSelectedSlotId.IsEmpty() && FindSelectedSlotIndex() < 0)
			m_sSelectedSlotId = "";

		if (!m_sSelectedNodeId.IsEmpty() && FindSelectedNodeIndex() < 0)
		{
			m_sSelectedNodeId = "";
			m_bCandidateMode = false;
		}

		if (m_aNodeIds.Count() == 0)
			BuildFallbackNodesFromSlots();

		EnsureSelectedSlotForCategory();
		ClampPages();
	}

	protected void ParseCandidatePayload(string payload)
	{
		if (payload.IsEmpty())
			return;

		string candidateNodeId;
		string candidateStatus;
		string candidateEmptyReason;
		array<string> lines = {};
		payload.Split("\n", lines, false);
		foreach (string line : lines)
		{
			array<string> fields = {};
			line.Split("|", fields, false);
			if (fields.Count() == 0)
				continue;

			if (fields[0] == "HST_LOADOUT_CANDIDATES")
			{
				if (fields.Count() > 1)
					candidateNodeId = fields[1];
				if (fields.Count() > 2)
					candidateStatus = fields[2];
				if (fields.Count() > 4)
					candidateEmptyReason = ResolvePayloadDisplayText(fields[4]);
				continue;
			}

			if (fields[0] != "CANDIDATE" || fields.Count() < 9)
				continue;

			if (candidateNodeId.IsEmpty())
				candidateNodeId = fields[1];
		}

		if (candidateNodeId.IsEmpty())
			return;

		ClearCandidatePayloadForNode(candidateNodeId);
		if (!candidateEmptyReason.IsEmpty())
			SetCandidateEmptyReason(candidateNodeId, candidateEmptyReason);
		else if (!candidateStatus.IsEmpty() && candidateStatus != "ready")
			SetCandidateEmptyReason(candidateNodeId, candidateStatus);
		foreach (string line : lines)
		{
			array<string> fields = {};
			line.Split("|", fields, false);
			if (fields.Count() < 9 || fields[0] != "CANDIDATE")
				continue;
			if (fields[1] != candidateNodeId)
				continue;
			if (IsRemovedExternalPrefab(fields[2]))
				continue;

			m_aCandidateNodeIds.Insert(fields[1]);
			m_aCandidatePrefabs.Insert(fields[2]);
			string candidateDisplay = ResolvePayloadDisplayText(fields[3]);
			candidateDisplay = HST_DisplayNameService.ResolveItemDisplayName(null, fields[2], candidateDisplay);
			m_aCandidateDisplays.Insert(candidateDisplay);
			string candidateShort = ResolvePayloadDisplayText(fields[4]);
			candidateShort = HST_DisplayNameService.ResolveShortItemDisplayName(candidateShort, fields[2]);
			if (candidateShort.IsEmpty())
				candidateShort = candidateDisplay;
			m_aCandidateShortDisplays.Insert(candidateShort);
			m_aCandidateCounts.Insert(fields[5]);
			m_aCandidateInfinite.Insert(fields[6] == "INF");
			m_aCandidateKinds.Insert(fields[7]);
			m_aCandidateCompatible.Insert(ParsePayloadBool(fields[8], true));
			if (fields.Count() > 9)
				m_aCandidateIconHints.Insert(fields[9]);
			else
				m_aCandidateIconHints.Insert(fields[7]);
			if (fields.Count() > 10)
				m_aCandidateAmmoMatch.Insert(ParsePayloadBool(fields[10], false));
			else
				m_aCandidateAmmoMatch.Insert(false);
		}

		if (FindStringIndex(m_aLoadedCandidateNodeIds, candidateNodeId) < 0)
			m_aLoadedCandidateNodeIds.Insert(candidateNodeId);

		int requestedIndex = FindStringIndex(m_aRequestedCandidateNodeIds, candidateNodeId);
		if (requestedIndex >= 0)
		{
			m_aRequestedCandidateNodeIds.Remove(requestedIndex);
			if (requestedIndex < m_aRequestedCandidateFrames.Count())
				m_aRequestedCandidateFrames.Remove(requestedIndex);
			if (requestedIndex < m_aRequestedCandidateAttempts.Count())
				m_aRequestedCandidateAttempts.Remove(requestedIndex);
		}

		ClampPages();
	}

	protected void ClearCandidatePayloadForNode(string nodeId)
	{
		if (nodeId.IsEmpty())
			return;

		for (int i = m_aCandidateNodeIds.Count() - 1; i >= 0; i--)
		{
			if (m_aCandidateNodeIds[i] != nodeId)
				continue;

			m_aCandidateNodeIds.Remove(i);
			m_aCandidatePrefabs.Remove(i);
			m_aCandidateDisplays.Remove(i);
			m_aCandidateShortDisplays.Remove(i);
			m_aCandidateCounts.Remove(i);
			m_aCandidateInfinite.Remove(i);
			m_aCandidateKinds.Remove(i);
			m_aCandidateCompatible.Remove(i);
			m_aCandidateAmmoMatch.Remove(i);
			m_aCandidateIconHints.Remove(i);
		}

		int loadedIndex = FindStringIndex(m_aLoadedCandidateNodeIds, nodeId);
		if (loadedIndex >= 0)
			m_aLoadedCandidateNodeIds.Remove(loadedIndex);

		int emptyIndex = FindStringIndex(m_aCandidateEmptyNodeIds, nodeId);
		if (emptyIndex >= 0)
		{
			m_aCandidateEmptyNodeIds.Remove(emptyIndex);
			if (emptyIndex < m_aCandidateEmptyReasons.Count())
				m_aCandidateEmptyReasons.Remove(emptyIndex);
		}

		int requestedIndex = FindStringIndex(m_aRequestedCandidateNodeIds, nodeId);
		if (requestedIndex >= 0)
		{
			m_aRequestedCandidateNodeIds.Remove(requestedIndex);
			if (requestedIndex < m_aRequestedCandidateFrames.Count())
				m_aRequestedCandidateFrames.Remove(requestedIndex);
			if (requestedIndex < m_aRequestedCandidateAttempts.Count())
				m_aRequestedCandidateAttempts.Remove(requestedIndex);
		}
	}

	protected void EnsureCandidatePayloadForSelectedNode()
	{
		if (m_sSelectedNodeId.IsEmpty())
			return;

		if (FindStringIndex(m_aLoadedCandidateNodeIds, m_sSelectedNodeId) >= 0)
			return;

		int requestedIndex = FindStringIndex(m_aRequestedCandidateNodeIds, m_sSelectedNodeId);
		int requestAttempts = 0;
		if (requestedIndex >= 0)
		{
			int requestedFrame;
			if (requestedIndex < m_aRequestedCandidateFrames.Count())
				requestedFrame = m_aRequestedCandidateFrames[requestedIndex];
			if (requestedIndex < m_aRequestedCandidateAttempts.Count())
				requestAttempts = m_aRequestedCandidateAttempts[requestedIndex];

			if (m_iFrameSerial - requestedFrame < 120)
				return;

			m_aRequestedCandidateNodeIds.Remove(requestedIndex);
			if (requestedIndex < m_aRequestedCandidateFrames.Count())
				m_aRequestedCandidateFrames.Remove(requestedIndex);
			if (requestedIndex < m_aRequestedCandidateAttempts.Count())
				m_aRequestedCandidateAttempts.Remove(requestedIndex);

			requestAttempts++;
			if (requestAttempts >= 3)
			{
				if (FindStringIndex(m_aLoadedCandidateNodeIds, m_sSelectedNodeId) < 0)
					m_aLoadedCandidateNodeIds.Insert(m_sSelectedNodeId);
				m_sLastResult = "h-istasi loadout editor | compatible item request timed out";
				SetCandidateEmptyReason(m_sSelectedNodeId, "Compatible item request timed out");
				return;
			}
		}

		m_aRequestedCandidateNodeIds.Insert(m_sSelectedNodeId);
		m_aRequestedCandidateFrames.Insert(m_iFrameSerial);
		m_aRequestedCandidateAttempts.Insert(requestAttempts);
		RequestServerAction("loadout_editor_candidates", m_sSelectedNodeId);
	}

	protected string ResolveCandidateEmptyText(string nodeId, string fallback)
	{
		int index = FindStringIndex(m_aCandidateEmptyNodeIds, nodeId);
		if (index >= 0 && index < m_aCandidateEmptyReasons.Count() && !m_aCandidateEmptyReasons[index].IsEmpty())
			return m_aCandidateEmptyReasons[index];

		return fallback;
	}

	protected void SetCandidateEmptyReason(string nodeId, string reason)
	{
		if (nodeId.IsEmpty() || reason.IsEmpty())
			return;

		int index = FindStringIndex(m_aCandidateEmptyNodeIds, nodeId);
		if (index >= 0)
		{
			if (index < m_aCandidateEmptyReasons.Count())
				m_aCandidateEmptyReasons[index] = reason;
			return;
		}

		m_aCandidateEmptyNodeIds.Insert(nodeId);
		m_aCandidateEmptyReasons.Insert(reason);
	}

	protected int FindStringIndex(array<string> values, string value)
	{
		if (!values)
			return -1;

		for (int i = 0; i < values.Count(); i++)
		{
			if (values[i] == value)
				return i;
		}

		return -1;
	}

	protected void ClearEditorPayload()
	{
		m_aCategoryIds.Clear();
		m_aCategoryLabels.Clear();
		m_aCategoryCounts.Clear();
		m_aItemCategories.Clear();
		m_aItemPrefabs.Clear();
		m_aItemDisplays.Clear();
		m_aItemShortDisplays.Clear();
		m_aItemSlotLabels.Clear();
		m_aItemCounts.Clear();
		m_aItemInfinite.Clear();
		m_aItemPreviewEligible.Clear();
		m_aVisibleItemIndexes.Clear();
		m_aSlotIds.Clear();
		m_aSlotCategories.Clear();
		m_aSlotPrefabs.Clear();
		m_aSlotDisplays.Clear();
		m_aSlotShortDisplays.Clear();
		m_aSlotLabels.Clear();
		m_aSlotQuantities.Clear();
		m_aSlotPreviewEligible.Clear();
		m_aNodeIds.Clear();
		m_aNodeParents.Clear();
		m_aNodeKinds.Clear();
		m_aNodeSlotKeys.Clear();
		m_aNodeLabels.Clear();
		m_aNodePrefabs.Clear();
		m_aNodeDisplays.Clear();
		m_aNodeCounts.Clear();
		m_aNodeInfinite.Clear();
		m_aNodeCanOpen.Clear();
		m_aNodeCanRemove.Clear();
		m_aNodeCanDeposit.Clear();
		m_aNodeFocus.Clear();
		m_aVisibleNodeIndexes.Clear();
		m_aCandidateNodeIds.Clear();
		m_aCandidatePrefabs.Clear();
		m_aCandidateDisplays.Clear();
		m_aCandidateShortDisplays.Clear();
		m_aCandidateCounts.Clear();
		m_aCandidateInfinite.Clear();
		m_aCandidateKinds.Clear();
		m_aCandidateCompatible.Clear();
		m_aCandidateAmmoMatch.Clear();
		m_aCandidateIconHints.Clear();
		m_aVisibleCandidateIndexes.Clear();
		m_aLoadedCandidateNodeIds.Clear();
		m_aCandidateEmptyNodeIds.Clear();
		m_aCandidateEmptyReasons.Clear();
		m_aRequestedCandidateNodeIds.Clear();
		m_aRequestedCandidateFrames.Clear();
		m_aRequestedCandidateAttempts.Clear();
		m_aTemplateIds.Clear();
		m_aTemplateDisplays.Clear();
		m_aTemplateSlotCounts.Clear();
		m_bPreviewSpawned = false;
		m_iPreviewItemCount = 0;
		m_iDraftSlotCount = 0;
		m_iDraftItemCount = 0;
		m_iDraftFiniteCount = 0;
		m_iDraftInfiniteCount = 0;
		m_sPreviewStatus = "";
		m_sPreviewPosition = "";
		m_sPreviewPrefab = DEFAULT_PREVIEW_PREFAB;
	}

	protected void BuildFallbackCategories()
	{
		if (m_aCategoryIds.Count() > 0)
			return;

		InsertCategory("headgear", "Headgear", 0);
		InsertCategory("clothing", "Clothing", 0);
		InsertCategory("vest", "Vest", 0);
		InsertCategory("pants", "Pants", 0);
		InsertCategory("boots", "Boots", 0);
		InsertCategory("backpack", "Backpack", 0);
		InsertCategory("handwear", "Handwear", 0);
		InsertCategory("weapon", "Weapons", 0);
		InsertCategory("sidearm", "Sidearms", 0);
		InsertCategory("magazine", "Magazines", 0);
		InsertCategory("explosive", "Explosives", 0);
		InsertCategory("attachment", "Attachments", 0);
		InsertCategory("medical", "Medical", 0);
		InsertCategory("utility", "Utility", 0);
	}

	protected void InsertCategory(string categoryId, string label, int count)
	{
		m_aCategoryIds.Insert(categoryId);
		m_aCategoryLabels.Insert(label);
		m_aCategoryCounts.Insert(count);
	}

	protected void BuildFallbackNodesFromSlots()
	{
		for (int slotIndex = 0; slotIndex < m_aSlotIds.Count(); slotIndex++)
		{
			m_aNodeIds.Insert("node_" + m_aSlotIds[slotIndex]);
			m_aNodeParents.Insert("");
			m_aNodeKinds.Insert("slot");
			if (slotIndex < m_aSlotCategories.Count())
				m_aNodeSlotKeys.Insert(m_aSlotCategories[slotIndex]);
			else
				m_aNodeSlotKeys.Insert("utility");
			m_aNodeLabels.Insert(GetSlotLabel(slotIndex));
			if (slotIndex < m_aSlotPrefabs.Count())
				m_aNodePrefabs.Insert(m_aSlotPrefabs[slotIndex]);
			else
				m_aNodePrefabs.Insert("");
			m_aNodeDisplays.Insert(GetSlotShortDisplay(slotIndex));
			if (slotIndex < m_aSlotQuantities.Count())
				m_aNodeCounts.Insert(string.Format("%1", m_aSlotQuantities[slotIndex]));
			else
				m_aNodeCounts.Insert("0");
			m_aNodeInfinite.Insert(false);
			m_aNodeCanOpen.Insert(false);
			m_aNodeCanRemove.Insert(true);
			m_aNodeCanDeposit.Insert(false);
			if (slotIndex < m_aSlotCategories.Count())
				m_aNodeFocus.Insert(m_aSlotCategories[slotIndex]);
			else
				m_aNodeFocus.Insert("torso");
		}
	}

	protected void RenderNodeRow(WorkspaceWidget workspace, Widget root, int nodeIndex, int row, int userId)
	{
		int top = 166 + row * 60;
		int color = 0xFF15191C;
		if (nodeIndex >= 0 && nodeIndex < m_aNodeIds.Count() && m_aNodeIds[nodeIndex] == m_sSelectedNodeId)
			color = 0xFF6F5124;

		CreateRectWidget(workspace, root, 112, top, 370, 56, color, 0.98, userId);
		if (!CreateIconWidget(workspace, root, ResolveIconTexture(ResolveNodeIconKey(nodeIndex)), 126, top + 11, 32, 32, userId, 0xFFE6E6E6))
			CreateTextWidget(workspace, root, ResolveNodeIcon(nodeIndex), 126, top + 10, 48, 34, 26, 0xFFE6E6E6, userId, true);
		CreateTextWidget(workspace, root, ShortenText(GetNodeLabel(nodeIndex), 20), 184, top + 10, 180, 18, 14, 0xFFE2E6E8, userId, true);
		CreateTextWidget(workspace, root, ShortenText(GetNodeDisplay(nodeIndex), 28), 184, top + 28, 224, 18, 14, 0xFFD5D8D9, userId, false);
		if (nodeIndex >= 0 && nodeIndex < m_aNodeCanOpen.Count() && m_aNodeCanOpen[nodeIndex])
			CreateTextWidget(workspace, root, "w", 434, top + 12, 18, 18, 16, 0xFFFFFFFF, userId, true);
	}

	protected void RenderStorageNodeRow(WorkspaceWidget workspace, Widget root, int nodeIndex, int left, int top, int width, int userId, bool contentItem)
	{
		int color = 0xFF15191C;
		if (!contentItem && nodeIndex >= 0 && nodeIndex < m_aNodeIds.Count() && m_aNodeIds[nodeIndex] == m_sSelectedNodeId)
			color = 0xFF6F5124;
		if (contentItem)
			color = 0xCC10161A;

		int rowHeight = 46;
		CreateRectWidget(workspace, root, left, top, width, rowHeight, color, 0.98, userId);
		if (!CreateIconWidget(workspace, root, ResolveIconTexture(ResolveNodeIconKey(nodeIndex)), left + 10, top + 8, 28, 28, userId, 0xFFE6E6E6))
			CreateTextWidget(workspace, root, ResolveNodeIcon(nodeIndex), left + 10, top + 6, 34, 30, 22, 0xFFE6E6E6, userId, true);
		CreateTextWidget(workspace, root, ShortenText(GetNodeLabel(nodeIndex), 20), left + 54, top + 7, 170, 16, 12, 0xFFE2E6E8, userId, true);
		CreateTextWidget(workspace, root, ShortenText(GetNodeDisplay(nodeIndex), 28), left + 54, top + 24, 218, 16, 12, 0xFFD5D8D9, userId, false);
		if (contentItem && nodeIndex >= 0 && nodeIndex < m_aNodeCounts.Count() && !m_aNodeCounts[nodeIndex].IsEmpty())
			CreateTextWidget(workspace, root, "x" + m_aNodeCounts[nodeIndex], left + width - 48, top + 15, 34, 14, 10, 0xFFFFD166, userId, true);
	}

	protected void RenderSelectedNodeHeader(WorkspaceWidget workspace, Widget root)
	{
		int nodeIndex = FindSelectedNodeIndex();
		if (!CreateIconWidget(workspace, root, ResolveIconTexture(ResolveNodeIconKey(nodeIndex)), 122, 166, 34, 34, 0, 0xFFE6E6E6))
			CreateTextWidget(workspace, root, ResolveNodeIcon(nodeIndex), 122, 162, 48, 42, 28, 0xFFE6E6E6, 0, true);
		CreateTextWidget(workspace, root, GetNodeLabel(nodeIndex), 184, 166, 220, 20, 15, 0xFFE2E6E8, 0, true);
		CreateTextWidget(workspace, root, ShortenText(GetNodeDisplay(nodeIndex), 28), 184, 186, 250, 20, 15, 0xFFD5D8D9, 0, false);
		CreateTextWidget(workspace, root, "w", 438, 166, 18, 18, 16, 0xFFFFFFFF, 0, true);
	}

	protected void RenderCandidateRow(WorkspaceWidget workspace, Widget root, int candidateIndex, int top, int userId)
	{
		int color = 0xFF15191C;
		if (IsCandidateCurrentNodeItem(candidateIndex))
			color = 0xFF6F5124;
		else if (candidateIndex >= 0 && candidateIndex < m_aCandidateAmmoMatch.Count() && m_aCandidateAmmoMatch[candidateIndex])
			color = 0xFF3D3520;
		CreateRectWidget(workspace, root, 112, top, 370, 50, color, 0.98, userId);
		if (!CreateIconWidget(workspace, root, ResolveIconTexture(ResolveCandidateIconKey(candidateIndex)), 126, top + 9, 30, 30, userId, 0xFFE6E6E6))
			CreateTextWidget(workspace, root, ResolveCandidateIcon(candidateIndex), 126, top + 8, 46, 34, 24, 0xFFE6E6E6, userId, true);
		CreateTextWidget(workspace, root, ShortenText(GetCandidateShortDisplay(candidateIndex), 32), 184, top + 11, 232, 20, 14, 0xFFE2E6E8, userId, false);
		CreateTextWidget(workspace, root, BuildCandidateCountLabel(candidateIndex), 424, top + 14, 42, 16, 10, 0xFFFFD166, userId, true);
	}

	protected void RenderStorageCandidateTile(WorkspaceWidget workspace, Widget root, int candidateIndex, int left, int top, int width, int userId)
	{
		int color = 0xDD15191C;
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateAmmoMatch.Count() && m_aCandidateAmmoMatch[candidateIndex])
			color = 0xFF3D3520;
		CreateRectWidget(workspace, root, left, top, width, 48, color, 0.98, userId);
		if (!CreateIconWidget(workspace, root, ResolveIconTexture(ResolveCandidateIconKey(candidateIndex)), left + 10, top + 8, 30, 30, userId, 0xFFE6E6E6))
			CreateTextWidget(workspace, root, ResolveCandidateIcon(candidateIndex), left + 10, top + 8, 36, 30, 22, 0xFFE6E6E6, userId, true);
		CreateTextWidget(workspace, root, ShortenText(GetCandidateShortDisplay(candidateIndex), 28), left + 52, top + 9, width - 104, 18, 12, 0xFFE2E6E8, userId, false);
		CreateTextWidget(workspace, root, BuildCandidateCountLabel(candidateIndex), left + width - 44, top + 16, 34, 14, 10, 0xFFFFD166, userId, true);
	}

	protected int FindSelectedNodeIndex()
	{
		if (m_sSelectedNodeId.IsEmpty())
			return -1;

		for (int i = 0; i < m_aNodeIds.Count(); i++)
		{
			if (m_aNodeIds[i] == m_sSelectedNodeId)
				return i;
		}

		return -1;
	}

	protected int CountVisibleNodesForCurrentMode()
	{
		int count;
		for (int i = 0; i < m_aNodeIds.Count(); i++)
		{
			if (IsNodeVisibleInCurrentMode(i))
				count++;
		}

		return count;
	}

	protected bool IsNodeVisibleInCurrentMode(int nodeIndex)
	{
		if (nodeIndex < 0 || nodeIndex >= m_aNodeKinds.Count())
			return false;

		string kind = m_aNodeKinds[nodeIndex];
		string category = ResolveNodeCategory(nodeIndex);
		if (m_sEditorMode == "clothing")
			return kind == "slot" && (category == "headgear" || category == "clothing" || category == "vest" || category == "pants" || category == "boots" || category == "backpack" || category == "handwear");
		if (m_sEditorMode == "weapons")
			return kind == "weapon_slot" || category == "weapon" || category == "launcher" || category == "sidearm";
		if (m_sEditorMode == "attachments")
		{
			if (!m_sSelectedNodeId.IsEmpty())
				return m_aNodeIds[nodeIndex] == m_sSelectedNodeId || (kind == "attachment" && nodeIndex < m_aNodeParents.Count() && m_aNodeParents[nodeIndex] == m_sSelectedNodeId);

			return kind == "weapon_slot" && IsAttachableWeaponCategory(category) && nodeIndex < m_aNodePrefabs.Count() && !m_aNodePrefabs[nodeIndex].IsEmpty();
		}
		if (m_sEditorMode == "storage")
			return kind == "storage" || kind == "storage_item";

		return false;
	}

	protected bool IsAttachableWeaponCategory(string category)
	{
		return category == "weapon" || category == "launcher" || category == "sidearm";
	}

	protected string ResolveNodeCategory(int nodeIndex)
	{
		if (nodeIndex >= 0 && nodeIndex < m_aNodeSlotKeys.Count() && !m_aNodeSlotKeys[nodeIndex].IsEmpty())
			return m_aNodeSlotKeys[nodeIndex];

		return "utility";
	}

	protected string GetNodeLabel(int nodeIndex)
	{
		if (nodeIndex >= 0 && nodeIndex < m_aNodeLabels.Count() && !m_aNodeLabels[nodeIndex].IsEmpty())
			return m_aNodeLabels[nodeIndex];

		return "Empty Slot";
	}

	protected string GetNodeDisplay(int nodeIndex)
	{
		if (nodeIndex >= 0 && nodeIndex < m_aNodeDisplays.Count() && !m_aNodeDisplays[nodeIndex].IsEmpty())
			return m_aNodeDisplays[nodeIndex];

		return "Empty Slot";
	}

	protected string ResolveNodeIcon(int nodeIndex)
	{
		string category = ResolveNodeCategory(nodeIndex);
		if (category == "headgear")
			return "^";
		if (category == "clothing")
			return "J";
		if (category == "vest")
			return "V";
		if (category == "pants")
			return "P";
		if (category == "boots")
			return "b";
		if (category == "backpack")
			return "B";
		if (category == "handwear")
			return "H";
		if (category == "weapon" || category == "launcher" || category == "sidearm")
			return ">";
		if (category == "attachment")
			return "w";
		if (category == "magazine")
			return "|||";
		if (category == "explosive")
			return "*";
		if (category == "medical")
			return "+";

		return "O";
	}

	protected int CountCandidatesForSelectedNode()
	{
		int count;
		for (int i = 0; i < m_aCandidateNodeIds.Count(); i++)
		{
			if (IsCandidateVisibleForSelectedNode(i))
				count++;
		}

		return count;
	}

	protected bool IsCandidateVisibleForSelectedNode(int candidateIndex)
	{
		if (candidateIndex < 0 || candidateIndex >= m_aCandidateNodeIds.Count())
			return false;

		if (m_sSelectedNodeId.IsEmpty())
			return false;

		return m_aCandidateNodeIds[candidateIndex] == m_sSelectedNodeId;
	}

	protected bool IsCandidateVisibleForStorageBrowser(int candidateIndex)
	{
		if (candidateIndex < 0 || candidateIndex >= m_aCandidateNodeIds.Count())
			return false;
		if (m_sSelectedNodeId.IsEmpty() || m_aCandidateNodeIds[candidateIndex] != m_sSelectedNodeId)
			return false;
		if (candidateIndex >= m_aCandidateKinds.Count())
			return false;

		return IsCategoryInStorageBrowserTab(m_aCandidateKinds[candidateIndex], m_sSelectedCategory);
	}

	protected int CountStorageCandidatesForSelectedNode()
	{
		int count;
		for (int i = 0; i < m_aCandidateNodeIds.Count(); i++)
		{
			if (IsCandidateVisibleForStorageBrowser(i))
				count++;
		}

		return count;
	}

	protected string GetCandidateShortDisplay(int candidateIndex)
	{
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateShortDisplays.Count() && !m_aCandidateShortDisplays[candidateIndex].IsEmpty())
			return m_aCandidateShortDisplays[candidateIndex];
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateDisplays.Count())
			return m_aCandidateDisplays[candidateIndex];

		return "Item";
	}

	protected string BuildCandidateCountLabel(int candidateIndex)
	{
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateInfinite.Count() && m_aCandidateInfinite[candidateIndex])
			return "INF";
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateCounts.Count())
			return "x" + m_aCandidateCounts[candidateIndex];

		return "";
	}

	protected string ResolveCandidateIcon(int candidateIndex)
	{
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateIconHints.Count())
		{
			string hint = m_aCandidateIconHints[candidateIndex];
			if (hint == "weapon")
				return ">";
			if (hint == "ammo")
				return "|||";
			if (hint == "wrench")
				return "w";
			if (hint == "grenade")
				return "*";
			if (hint == "clothing")
				return "J";
		}

		return "O";
	}

	protected string ResolveCandidateIconKey(int candidateIndex)
	{
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateIconHints.Count() && !m_aCandidateIconHints[candidateIndex].IsEmpty())
			return m_aCandidateIconHints[candidateIndex];
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateKinds.Count())
			return m_aCandidateKinds[candidateIndex];

		return "utility";
	}

	protected string ResolveNodeIconKey(int nodeIndex)
	{
		if (nodeIndex < 0 || nodeIndex >= m_aNodeKinds.Count())
			return m_sEditorMode;
		if (m_aNodeKinds[nodeIndex] == "storage")
			return "storage";
		if (nodeIndex >= 0 && nodeIndex < m_aNodeSlotKeys.Count() && !m_aNodeSlotKeys[nodeIndex].IsEmpty())
			return m_aNodeSlotKeys[nodeIndex];

		return ResolveNodeCategory(nodeIndex);
	}

	protected bool IsCandidateCurrentNodeItem(int candidateIndex)
	{
		int nodeIndex = FindSelectedNodeIndex();
		if (nodeIndex < 0 || candidateIndex < 0 || nodeIndex >= m_aNodePrefabs.Count() || candidateIndex >= m_aCandidatePrefabs.Count())
			return false;

		return !m_aNodePrefabs[nodeIndex].IsEmpty() && m_aNodePrefabs[nodeIndex] == m_aCandidatePrefabs[candidateIndex];
	}

	protected int FindCategoryIndex(string categoryId)
	{
		for (int i = 0; i < m_aCategoryIds.Count(); i++)
		{
			if (m_aCategoryIds[i] == categoryId)
				return i;
		}

		return -1;
	}

	protected int FindTemplateIndex(string templateId)
	{
		for (int i = 0; i < m_aTemplateIds.Count(); i++)
		{
			if (m_aTemplateIds[i] == templateId)
				return i;
		}

		return -1;
	}

	protected string ResolveSelectedCategoryLabel()
	{
		int categoryIndex = FindCategoryIndex(m_sSelectedCategory);
		if (categoryIndex >= 0 && categoryIndex < m_aCategoryLabels.Count())
			return m_aCategoryLabels[categoryIndex];

		return "Recovered Items";
	}

	protected int CountItemsForSelectedCategory()
	{
		int count;
		foreach (string category : m_aItemCategories)
		{
			if (category == m_sSelectedCategory)
				count++;
		}

		return count;
	}

	protected string BuildCountLabel(int itemIndex)
	{
		if (itemIndex < 0 || itemIndex >= m_aItemCounts.Count())
			return "";

		if (itemIndex < m_aItemInfinite.Count() && m_aItemInfinite[itemIndex])
			return "INF";

		return "x" + m_aItemCounts[itemIndex];
	}

	protected string BuildPreviewStatusLabel()
	{
		if (!m_bPreviewSpawned)
			return "preview offline";

		if (!m_sPreviewStatus.IsEmpty())
			return ShortenText(m_sPreviewStatus, 28);

		return string.Format("%1 item(s)", m_iPreviewItemCount);
	}

	protected string BuildDraftHeaderSummary()
	{
		return string.Format("%1 slots / %2 items | finite %3 | INF %4", m_iDraftSlotCount, m_iDraftItemCount, m_iDraftFiniteCount, m_iDraftInfiniteCount);
	}

	protected string FindFirstSlotDisplayForCategory(string category)
	{
		for (int i = 0; i < m_aSlotCategories.Count(); i++)
		{
			if (m_aSlotCategories[i] == category)
				return GetSlotShortDisplay(i);
		}

		return "empty";
	}

	protected string FindFirstWeaponSlotDisplay()
	{
		for (int i = 0; i < m_aSlotCategories.Count(); i++)
		{
			if (m_aSlotCategories[i] == "weapon" || m_aSlotCategories[i] == "launcher")
				return GetSlotShortDisplay(i);
		}

		return "empty";
	}

	protected string BuildSupportSlotSummary()
	{
		int magazines = CountSlotsInCategory("magazine");
		int explosives = CountSlotsInCategory("explosive");
		int medical = CountSlotsInCategory("medical");
		int utility = CountSlotsInCategory("utility") + CountSlotsInCategory("attachment");
		if (magazines + explosives + medical + utility <= 0)
			return "empty";

		return string.Format("M%1 E%2 Med%3 U%4", magazines, explosives, medical, utility);
	}

	protected int CountSlotsInCategory(string category)
	{
		int count;
		for (int i = 0; i < m_aSlotCategories.Count(); i++)
		{
			if (m_aSlotCategories[i] == category)
				count += Math.Max(1, m_aSlotQuantities[i]);
		}

		return count;
	}

	protected string BuildSlotCategoryTag(string category)
	{
		if (category == "clothing")
			return "UNI";
		if (category == "headgear")
			return "HEAD";
		if (category == "vest")
			return "VEST";
		if (category == "pants")
			return "PANT";
		if (category == "boots")
			return "BOOT";
		if (category == "backpack")
			return "PACK";
		if (category == "handwear")
			return "HAND";
		if (category == "weapon")
			return "WPN";
		if (category == "sidearm")
			return "PST";
		if (category == "launcher")
			return "AT";
		if (category == "magazine")
			return "MAG";
		if (category == "explosive")
			return "EXP";
		if (category == "attachment")
			return "ATT";
		if (category == "medical")
			return "MED";

		return "KIT";
	}

	protected string BuildSlotCategoryLabel(string category)
	{
		if (category == "clothing")
			return "Uniform";
		if (category == "headgear")
			return "Head";
		if (category == "vest")
			return "Vest";
		if (category == "pants")
			return "Pants";
		if (category == "boots")
			return "Boots";
		if (category == "backpack")
			return "Back";
		if (category == "handwear")
			return "Handwear";
		if (category == "weapon")
			return "Weapon";
		if (category == "sidearm")
			return "Sidearm";
		if (category == "launcher")
			return "Launcher";
		if (category == "magazine")
			return "Ammo";
		if (category == "explosive")
			return "Explosive";
		if (category == "attachment")
			return "Attachment";
		if (category == "medical")
			return "Medical";

		return "Gear";
	}

	protected bool IsPreviewEligibleCategory(string category)
	{
		return category == "clothing" || category == "headgear" || category == "vest" || category == "pants" || category == "boots" || category == "backpack" || category == "handwear" || category == "weapon" || category == "sidearm" || category == "launcher" || category == "attachment";
	}

	protected bool ParsePayloadBool(string value, bool fallback)
	{
		if (value == "true" || value == "1" || value == "TRUE")
			return true;
		if (value == "false" || value == "0" || value == "FALSE")
			return false;

		return fallback;
	}

	protected string ResolvePayloadDisplayText(string value)
	{
		if (value.IsEmpty())
			return "";

		if (value.Length() > 0 && value.Substring(0, 1) == "#")
		{
			string translated = WidgetManager.Translate(value);
			if (!translated.IsEmpty() && translated != value && translated.Substring(0, 1) != "#")
				return translated;
		}

		return value;
	}

	protected int GetMaxPage(int itemCount, int pageSize)
	{
		if (pageSize <= 0 || itemCount <= pageSize)
			return 0;

		return (itemCount - 1) / pageSize;
	}

	protected void ClampPages()
	{
		m_iItemPage = Math.Min(Math.Max(0, m_iItemPage), GetMaxPage(CountItemsForSelectedCategory(), ITEMS_PER_PAGE));
		m_iSlotPage = Math.Min(Math.Max(0, m_iSlotPage), GetMaxPage(m_aSlotIds.Count(), SLOTS_PER_PAGE));
		m_iTemplatePage = Math.Min(Math.Max(0, m_iTemplatePage), GetMaxPage(m_aTemplateIds.Count(), TEMPLATES_PER_PAGE));
	}

	protected int GetEditorModeCount()
	{
		return 6;
	}

	protected string GetEditorModeId(int index)
	{
		if (index == 0)
			return "clothing";
		if (index == 1)
			return "weapons";
		if (index == 2)
			return "attachments";
		if (index == 3)
			return "storage";
		if (index == 4)
			return "save";

		return "settings";
	}

	protected string GetEditorModeLabel(string modeId)
	{
		if (modeId == "clothing")
			return "Clothing";
		if (modeId == "weapons")
			return "Weapons";
		if (modeId == "attachments")
			return "Attachments";
		if (modeId == "storage")
			return "Storage";
		if (modeId == "save")
			return "Save";

		return "Settings";
	}

	protected string GetEditorModeIcon(string modeId)
	{
		if (modeId == "clothing")
			return "I";
		if (modeId == "weapons")
			return ">";
		if (modeId == "attachments")
			return "w";
		if (modeId == "storage")
			return "::";
		if (modeId == "save")
			return "S";

		return "*";
	}

	protected int GetEditorModeIndex(string modeId)
	{
		for (int i = 0; i < GetEditorModeCount(); i++)
		{
			if (GetEditorModeId(i) == modeId)
				return i;
		}

		return 0;
	}

	protected string ResolveDefaultCategoryForMode(string modeId)
	{
		if (modeId == "clothing")
			return "clothing";
		if (modeId == "weapons")
			return "weapon";
		if (modeId == "attachments")
			return "attachment";
		if (modeId == "storage")
			return "magazine";

		return m_sSelectedCategory;
	}

	protected string ResolveModeForCategory(string category)
	{
		if (category == "clothing" || category == "headgear" || category == "vest" || category == "pants" || category == "boots" || category == "backpack" || category == "handwear")
			return "clothing";
		if (category == "weapon" || category == "launcher" || category == "sidearm")
			return "weapons";
		if (category == "attachment")
			return "attachments";
		if (category == "magazine" || category == "explosive" || category == "medical" || category == "utility")
			return "storage";

		return m_sEditorMode;
	}

	protected bool IsCategoryVisibleForMode(string category, string modeId)
	{
		if (modeId == "clothing")
			return category == "clothing" || category == "headgear" || category == "vest" || category == "pants" || category == "boots" || category == "backpack" || category == "handwear";
		if (modeId == "weapons")
			return category == "weapon" || category == "launcher" || category == "sidearm";
		if (modeId == "attachments")
			return category == "attachment";
		if (modeId == "storage")
			return category == "magazine" || category == "explosive" || category == "medical" || category == "utility" || category == "weapon" || category == "launcher" || category == "sidearm" || category == "attachment" || category == "clothing" || category == "headgear" || category == "vest" || category == "pants" || category == "boots" || category == "backpack" || category == "handwear";
		if (modeId == "settings" || modeId == "save")
			return true;

		return false;
	}

	protected int GetStorageBrowserCategoryCount()
	{
		return 7;
	}

	protected string GetStorageBrowserCategoryId(int index)
	{
		if (index == 0)
			return "magazine";
		if (index == 1)
			return "utility";
		if (index == 2)
			return "explosive";
		if (index == 3)
			return "medical";
		if (index == 4)
			return "weapon";
		if (index == 5)
			return "attachment";

		return "clothing";
	}

	protected string GetStorageBrowserCategoryLabel(string category)
	{
		if (category == "magazine")
			return "Ammunition";
		if (category == "utility")
			return "Equipment";
		if (category == "explosive")
			return "Throwables";
		if (category == "medical")
			return "Medical";
		if (category == "weapon")
			return "Weapons";
		if (category == "attachment")
			return "Attachments";

		return "Clothing";
	}

	protected bool IsCategoryInStorageBrowserTab(string itemCategory, string tabCategory)
	{
		if (tabCategory == "weapon")
			return itemCategory == "weapon" || itemCategory == "launcher" || itemCategory == "sidearm";
		if (tabCategory == "clothing")
			return itemCategory == "clothing" || itemCategory == "headgear" || itemCategory == "vest" || itemCategory == "pants" || itemCategory == "boots" || itemCategory == "backpack" || itemCategory == "handwear";

		return itemCategory == tabCategory;
	}

	protected bool IsSlotVisibleInCurrentMode(int slotIndex)
	{
		if (slotIndex < 0 || slotIndex >= m_aSlotCategories.Count())
			return false;

		if (m_sEditorMode == "save" || m_sEditorMode == "settings")
			return true;

		return IsCategoryVisibleForMode(m_aSlotCategories[slotIndex], m_sEditorMode);
	}

	protected int CountVisibleSlotsForCurrentMode()
	{
		int count;
		for (int i = 0; i < m_aSlotCategories.Count(); i++)
		{
			if (IsSlotVisibleInCurrentMode(i))
				count++;
		}

		return count;
	}

	protected string BuildModeTitle()
	{
		if (m_sEditorMode == "save")
			return "Saved Loadouts";
		if (m_sEditorMode == "settings")
			return "Editor Settings";

		return ResolveSelectedCategoryLabel();
	}

	protected void EnsureSelectedStorageNode()
	{
		if (FindSelectedNodeIndex() >= 0)
		{
			int selectedIndex = FindSelectedNodeIndex();
			if (selectedIndex < m_aNodeKinds.Count() && m_aNodeKinds[selectedIndex] == "storage")
				return;
		}

		for (int i = 0; i < m_aNodeKinds.Count(); i++)
		{
			if (m_aNodeKinds[i] != "storage")
				continue;

			m_sSelectedNodeId = m_aNodeIds[i];
			m_bCandidateMode = true;
			return;
		}

		m_sSelectedNodeId = "";
	}

	protected bool ReturnFromAttachmentCandidateToWeapon()
	{
		if (m_sEditorMode != "attachments")
			return false;

		int selectedNodeIndex = FindSelectedNodeIndex();
		if (selectedNodeIndex < 0 || selectedNodeIndex >= m_aNodeKinds.Count())
			return false;

		if (m_aNodeKinds[selectedNodeIndex] != "attachment")
			return false;

		if (selectedNodeIndex >= m_aNodeParents.Count() || m_aNodeParents[selectedNodeIndex].IsEmpty())
			return false;

		m_sSelectedNodeId = m_aNodeParents[selectedNodeIndex];
		m_sSelectedSlotId = "";
		m_sSelectedCategory = "attachment";
		m_bCandidateMode = false;
		return true;
	}

	protected string BuildStorageTargetLabel()
	{
		int nodeIndex = FindSelectedNodeIndex();
		if (nodeIndex < 0)
			return "Select Storage";

		return GetNodeDisplay(nodeIndex);
	}

	protected void RenderStorageCategoryTabs(WorkspaceWidget workspace, Widget root, int left, int top, int width)
	{
		int count = GetStorageBrowserCategoryCount();
		int tabWidth = Math.Max(92, (width - (count - 1) * 8) / count);
		for (int i = 0; i < count; i++)
		{
			string category = GetStorageBrowserCategoryId(i);
			int tabLeft = left + i * (tabWidth + 8);
			int color = 0xFF12171B;
			if (category == m_sSelectedCategory)
				color = 0xFF6F5124;

			CreateRectWidget(workspace, root, tabLeft, top, tabWidth, 32, color, 0.98, STORAGE_CATEGORY_WIDGET_ID_BASE + i);
			CreateIconWidget(workspace, root, ResolveIconTexture(category), tabLeft + 8, top + 6, 20, 20, STORAGE_CATEGORY_WIDGET_ID_BASE + i, 0xFFF5E8CE);
			CreateTextWidget(workspace, root, ShortenText(GetStorageBrowserCategoryLabel(category), 13), tabLeft + 34, top + 9, tabWidth - 38, 14, 10, 0xFFF5E8CE, STORAGE_CATEGORY_WIDGET_ID_BASE + i, category == m_sSelectedCategory);
		}
	}

	protected void RenderCategoryChips(WorkspaceWidget workspace, Widget root)
	{
		int visible;
		for (int i = 0; i < m_aCategoryIds.Count(); i++)
		{
			if (!IsCategoryVisibleForMode(m_aCategoryIds[i], m_sEditorMode))
				continue;

			if (visible >= 4)
				break;

			int left = 70 + visible * 68;
			int color = 0xFF12171B;
			if (m_aCategoryIds[i] == m_sSelectedCategory)
				color = 0xFF6F5124;

			CreateRectWidget(workspace, root, left, 210, 62, 24, color, 0.98, CATEGORY_WIDGET_ID_BASE + i);
			CreateTextWidget(workspace, root, ShortenText(BuildSlotCategoryTag(m_aCategoryIds[i]), 6), left + 7, 216, 48, 13, 10, 0xFFF5E8CE, CATEGORY_WIDGET_ID_BASE + i, m_aCategoryIds[i] == m_sSelectedCategory);
			visible++;
		}
	}

	protected string BuildSelectedPreviewTitle()
	{
		int slotIndex = FindSelectedSlotIndex();
		if (slotIndex >= 0)
			return GetSlotLabel(slotIndex) + " / " + GetSlotShortDisplay(slotIndex);

		return BuildModeTitle();
	}

	protected string BuildPreviewMetaLine()
	{
		return string.Format("%1 | %2", BuildCameraModeLabel(), ShortenText(m_sPreviewStatus, 44));
	}

	protected string BuildCameraModeLabel()
	{
		if (m_iCameraMode == 1)
			return "Detail";
		if (m_iCameraMode == 2)
			return "Weapon";

		return "Camera";
	}

	protected int FindSelectedSlotIndex()
	{
		if (m_sSelectedSlotId.IsEmpty())
			return -1;

		for (int i = 0; i < m_aSlotIds.Count(); i++)
		{
			if (m_aSlotIds[i] == m_sSelectedSlotId)
				return i;
		}

		return -1;
	}

	protected void EnsureSelectedSlotForCategory()
	{
		if (m_sEditorMode == "save" || m_sEditorMode == "settings")
			return;

		int selectedSlotIndex = FindSelectedSlotIndex();
		if (selectedSlotIndex >= 0 && selectedSlotIndex < m_aSlotCategories.Count() && m_aSlotCategories[selectedSlotIndex] == m_sSelectedCategory)
			return;

		for (int slotIndex = 0; slotIndex < m_aSlotIds.Count(); slotIndex++)
		{
			if (slotIndex >= m_aSlotCategories.Count())
				continue;

			if (m_aSlotCategories[slotIndex] != m_sSelectedCategory)
				continue;

			m_sSelectedSlotId = m_aSlotIds[slotIndex];
			return;
		}

		m_sSelectedSlotId = "";
	}

	protected string GetSlotLabel(int slotIndex)
	{
		if (slotIndex >= 0 && slotIndex < m_aSlotLabels.Count() && !m_aSlotLabels[slotIndex].IsEmpty())
			return m_aSlotLabels[slotIndex];
		if (slotIndex >= 0 && slotIndex < m_aSlotCategories.Count())
			return BuildSlotCategoryLabel(m_aSlotCategories[slotIndex]);

		return "Slot";
	}

	protected string GetSlotShortDisplay(int slotIndex)
	{
		if (slotIndex >= 0 && slotIndex < m_aSlotShortDisplays.Count() && !m_aSlotShortDisplays[slotIndex].IsEmpty())
			return m_aSlotShortDisplays[slotIndex];
		if (slotIndex >= 0 && slotIndex < m_aSlotDisplays.Count() && !m_aSlotDisplays[slotIndex].IsEmpty())
			return m_aSlotDisplays[slotIndex];

		return "empty";
	}

	protected string GetItemSlotLabel(int itemIndex)
	{
		if (itemIndex >= 0 && itemIndex < m_aItemSlotLabels.Count() && !m_aItemSlotLabels[itemIndex].IsEmpty())
			return m_aItemSlotLabels[itemIndex];
		if (itemIndex >= 0 && itemIndex < m_aItemCategories.Count())
			return BuildSlotCategoryLabel(m_aItemCategories[itemIndex]);

		return "Item";
	}

	protected string GetItemShortDisplay(int itemIndex)
	{
		if (itemIndex >= 0 && itemIndex < m_aItemShortDisplays.Count() && !m_aItemShortDisplays[itemIndex].IsEmpty())
			return m_aItemShortDisplays[itemIndex];
		if (itemIndex >= 0 && itemIndex < m_aItemDisplays.Count() && !m_aItemDisplays[itemIndex].IsEmpty())
			return m_aItemDisplays[itemIndex];

		return "item";
	}

	protected bool IsItemSelectedForSlot(int itemIndex)
	{
		int slotIndex = FindSelectedSlotIndex();
		if (slotIndex < 0 || itemIndex < 0 || slotIndex >= m_aSlotPrefabs.Count() || itemIndex >= m_aItemPrefabs.Count())
			return false;

		return m_aSlotPrefabs[slotIndex] == m_aItemPrefabs[itemIndex];
	}

	protected string BuildSwapActionLabel()
	{
		if (FindSelectedSlotIndex() >= 0)
			return "Swap";

		return "Add";
	}

	protected string BuildNodeActionLabel()
	{
		if (!m_bCandidateMode)
		{
			if (m_sEditorMode == "storage")
				return "Open";
			return "Swap";
		}

		int nodeIndex = FindSelectedNodeIndex();
		if (nodeIndex < 0)
			return "Add";

		if (nodeIndex < m_aNodePrefabs.Count() && m_aNodePrefabs[nodeIndex].IsEmpty())
			return "Add";

		return "Swap";
	}

	protected string ResolveNodeSetCommand(string nodeId)
	{
		if (nodeId.Contains("attach"))
			return "set_attachment";

		int nodeIndex = FindSelectedNodeIndex();
		if (nodeIndex >= 0 && nodeIndex < m_aNodeKinds.Count() && (m_aNodeKinds[nodeIndex] == "storage" || m_aNodeKinds[nodeIndex] == "storage_item"))
			return "add_storage_item";

		return "set_node_item";
	}

	protected string ResolveNodeRemoveCommand(string nodeId)
	{
		if (nodeId.Contains("attach"))
			return "remove_attachment";

		int nodeIndex = FindSelectedNodeIndex();
		if (nodeIndex >= 0 && nodeIndex < m_aNodeKinds.Count() && m_aNodeKinds[nodeIndex] == "storage_item")
			return "remove_node_item";

		return "remove_node_item";
	}

	protected string ResolveSlotIdFromNodeId(string nodeId)
	{
		if (nodeId.IsEmpty())
			return "";

		if (nodeId.IndexOf("node_") != 0)
			return "";

		return nodeId.Substring(5, nodeId.Length() - 5);
	}

	protected string BuildSwapHeaderText()
	{
		int slotIndex = FindSelectedSlotIndex();
		if (slotIndex >= 0)
			return "Swap " + GetSlotLabel(slotIndex);

		return "Add " + ResolveSelectedCategoryLabel();
	}

	protected string BuildSwapTargetText()
	{
		int slotIndex = FindSelectedSlotIndex();
		if (slotIndex >= 0)
			return GetSlotShortDisplay(slotIndex);

		return "Select recovered gear below";
	}

	protected string BuildSwapHint()
	{
		int slotIndex = FindSelectedSlotIndex();
		if (slotIndex >= 0)
			return "Select a recovered item to swap " + GetSlotLabel(slotIndex);

		return "Select a recovered item to add it to the draft";
	}

	protected void ConfigurePreviewWidget()
	{
		if (!m_PreviewWidget)
			return;

		m_PreviewWidget.SetVisible(true);
		m_PreviewWidget.SetOpacity(1.0);
		m_PreviewWidget.SetZOrder(0);
	}

	protected void EnsurePreviewWorld()
	{
		if (!m_PreviewWidget)
			return;

		if (!m_PreviewWorldRef)
		{
			m_PreviewWorldRef = BaseWorld.CreateWorld("HSTLoadoutPreview", "HSTLoadoutPreview");
			if (m_PreviewWorldRef)
				m_PreviewWorld = m_PreviewWorldRef.GetRef();

			if (m_PreviewWorld)
			{
				m_PreviewWorld.SetCameraType(PREVIEW_CAMERA, CameraType.PERSPECTIVE);
				m_PreviewWorld.SetCameraNearPlane(PREVIEW_CAMERA, 0.001);
				m_PreviewWorld.SetCameraFarPlane(PREVIEW_CAMERA, 150);
				m_PreviewWorld.SetCameraVerticalFOV(PREVIEW_CAMERA, 40);
				ResetPreviewCameraToReferenceDefault();
				SpawnPreviewStage();
			}
		}

		if (m_PreviewWorld)
		{
			m_PreviewWidget.SetWorld(m_PreviewWorld, PREVIEW_CAMERA);
			RefreshPreviewLightState();
			if (m_PreviewEntity)
				UpdatePreviewCamera(false);
		}
	}

	protected void ResetPreviewCameraToReferenceDefault()
	{
		if (!m_PreviewWorld)
			return;

		vector camera = "5 1 5";
		vector look = "0 0.5 0";
		m_vCameraTargetPosition = camera;
		m_vCameraTargetLook = look;
		m_vCameraCurrentPosition = camera;
		m_vCameraCurrentLook = look;
		m_vCurrentCameraLookPosition = look;
		m_vCameraTargetDirection = vector.Direction(look, camera).Normalized();
		m_fCameraTargetDistance = vector.Distance(look, camera);
		m_bCameraInitialized = true;
		vector mat[4];
		mat[3] = camera;
		SCR_Math3D.LookAt(camera, look, vector.Up, mat);
		for (int i = 0; i < 4; i++)
			m_aCurrentCameraMatrix[i] = mat[i];
		m_PreviewWorld.SetCameraEx(PREVIEW_CAMERA, mat);
		UpdatePreviewLightAngles();
	}

	protected void SpawnPreviewStage()
	{
		if (!m_PreviewWorld || m_PreviewStageEntity)
			return;

		Resource stageResource = Resource.Load(PREVIEW_WORLD_PREFAB);
		if (!stageResource || !stageResource.IsValid())
		{
			m_sPreviewStatus = "preview stage resource failed";
			return;
		}

		m_PreviewStageEntity = GetGame().SpawnEntityPrefab(stageResource, m_PreviewWorld);
		if (!m_PreviewStageEntity)
			m_sPreviewStatus = "preview stage spawn failed";
	}

	protected void QueuePreviewLightSpawn()
	{
		if (!m_PreviewWorld || m_PreviewLightEntity || m_bPreviewLightSpawnQueued)
			return;

		m_bPreviewLightSpawnQueued = true;
		GetGame().GetCallqueue().CallLater(SpawnPreviewLight, PREVIEW_LIGHT_DELAY_MS, false);
	}

	protected void SpawnPreviewLight()
	{
		m_bPreviewLightSpawnQueued = false;
		if (!m_PreviewWorld || m_PreviewLightEntity)
			return;

		Resource lightResource = Resource.Load(PREVIEW_LIGHTS_PREFAB);
		if (!lightResource || !lightResource.IsValid())
		{
			if (m_sPreviewStatus.IsEmpty())
				m_sPreviewStatus = "preview light resource failed";
			return;
		}

		m_PreviewLightEntity = GetGame().SpawnEntityPrefab(lightResource, m_PreviewWorld);
		if (!m_PreviewLightEntity)
		{
			if (m_sPreviewStatus.IsEmpty())
				m_sPreviewStatus = "preview light spawn failed";
			return;
		}

		m_vPreviewLightBaseAngles = m_PreviewLightEntity.GetAngles();
		m_bPreviewLightAnglesInitialized = true;
		UpdatePreviewLightAngles();
	}

	protected void RefreshPreviewLightState()
	{
		if (!m_PreviewLightEntity)
			QueuePreviewLightSpawn();
		else
			UpdatePreviewLightAngles();
	}

	protected void DeletePreviewLight()
	{
		if (m_PreviewLightEntity)
			SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewLightEntity);

		m_PreviewLightEntity = null;
		m_bPreviewLightSpawnQueued = false;
		m_bPreviewLightAnglesInitialized = false;
	}

	protected void RefreshPreviewWorldLoadout()
	{
		if (!m_PreviewWorld)
			return;

		string renderKey = BuildPreviewRenderKey();
		if (renderKey == m_sPreviewRenderKey && m_PreviewEntity)
			return;

		m_sPreviewRenderKey = renderKey;
		DeletePreviewEntities();

		IEntity previewSource;
		string previewMode;
		string previewLabel;
		if (ResolvePreviewSource(previewSource, previewMode, previewLabel) && TryCreateLivePreviewEntity(previewSource, previewMode, previewLabel))
			return;

		m_sPreviewStatus = "building fallback mannequin";
		RefreshFallbackPreviewMannequin();
	}

	protected void DeletePreviewEntities()
	{
		if (m_PreviewEntity)
			SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewEntity);

		if (m_PreviewSourceCharacter)
			SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewSourceCharacter);

		m_PreviewEntity = null;
		m_PreviewSourceCharacter = null;
		m_PreviewSourceEntity = null;
		m_bPreviewSpawned = false;
		m_iPreviewItemCount = 0;
	}

	protected bool TryCreateLivePreviewEntity(IEntity previewSource, string previewMode, string previewLabel)
	{
		if (!previewSource)
			return false;

		InventoryItemComponent previewItem = InventoryItemComponent.Cast(previewSource.FindComponent(InventoryItemComponent));
		if (!previewItem)
		{
			m_sPreviewStatus = "live preview source has no item component";
			m_bPreviewSpawned = false;
			return false;
		}

		ResetPreviewCameraToReferenceDefault();
		m_PreviewEntity = previewItem.CreatePreviewEntity(m_PreviewWorld, PREVIEW_CAMERA);
		if (!m_PreviewEntity)
		{
			m_sPreviewStatus = "live preview clone failed";
			m_bPreviewSpawned = false;
			return false;
		}

		m_PreviewSourceEntity = previewSource;
		m_sPreviewSourceMode = previewMode;
		if (previewMode == PREVIEW_MODE_ENTITY)
			m_PreviewEntity.SetOrigin(vector.Up);
		m_PreviewEntity.Update();
		SetPreviewEntityQualityRecursive(m_PreviewEntity);
		vector debugMins;
		vector debugMaxs;
		m_PreviewEntity.GetWorldBounds(debugMins, debugMaxs);
		Print(string.Format("h-istasi preview | mode=%1 source=%2 preview=%3 mins=%4 maxs=%5 size=%6 world=%7 light=%8", previewMode, previewSource, m_PreviewEntity, debugMins, debugMaxs, vector.Distance(debugMins, debugMaxs), m_PreviewWorld, m_PreviewLightEntity));
		if (!HasUsablePreviewBounds(m_PreviewEntity))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewEntity);
			m_PreviewEntity = null;
			m_sPreviewStatus = "live preview clone had no bounds";
			m_bPreviewSpawned = false;
			return false;
		}

		m_bPreviewSpawned = true;
		m_iPreviewItemCount = Math.Max(m_iDraftItemCount, m_aSlotPrefabs.Count());
		if (previewLabel.IsEmpty())
			previewLabel = ResolvePreviewEntityDisplayName(previewSource);
		if (previewMode == PREVIEW_MODE_ENTITY)
			m_sPreviewStatus = "preview " + ShortenText(previewLabel, 36);
		else
			m_sPreviewStatus = "preview live character";

		UpdatePreviewedEntityMetrics();
		UpdatePreviewCameraImmediate(true, "live");
		return true;
	}

	protected void RefreshFallbackPreviewMannequin()
	{
		if (!m_PreviewWorld)
			return;

		m_sPreviewSourceMode = PREVIEW_MODE_CHARACTER;
		ResourceName previewPrefab = m_sPreviewPrefab;
		if (previewPrefab.IsEmpty())
			previewPrefab = DEFAULT_PREVIEW_PREFAB;

		Resource loaded = Resource.Load(previewPrefab);
		if (!loaded || !loaded.IsValid())
		{
			if (previewPrefab != DEFAULT_PREVIEW_PREFAB)
			{
				previewPrefab = DEFAULT_PREVIEW_PREFAB;
				loaded = Resource.Load(previewPrefab);
			}
		}

		if (!loaded || !loaded.IsValid())
		{
			m_sPreviewStatus = "preview fallback resource failed";
			m_bPreviewSpawned = false;
			return;
		}

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = vector.Up;
		m_PreviewSourceCharacter = GetGame().SpawnEntityPrefabLocal(loaded, m_PreviewWorld, params);
		if (!m_PreviewSourceCharacter)
		{
			m_sPreviewStatus = "preview character spawn failed";
			m_bPreviewSpawned = false;
			return;
		}

		m_PreviewSourceCharacter.SetFixedLOD(0);
		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(m_PreviewSourceCharacter.FindComponent(SCR_InventoryStorageManagerComponent));
		int inserted;
		int removed = ClearLocalPreviewInventory(inventory);
		string firstFailure;
		for (int slotIndex = 0; slotIndex < m_aSlotPrefabs.Count(); slotIndex++)
		{
			if (slotIndex < m_aSlotPreviewEligible.Count() && !m_aSlotPreviewEligible[slotIndex])
			{
				if (firstFailure.IsEmpty())
					firstFailure = "gear only";
				continue;
			}

			string slotPrefab = m_aSlotPrefabs[slotIndex];
			if (IsRemovedExternalPrefab(slotPrefab))
			{
				if (firstFailure.IsEmpty())
					firstFailure = "removed external item";
				continue;
			}

			string failure;
			string slotId;
			if (slotIndex < m_aSlotIds.Count())
				slotId = m_aSlotIds[slotIndex];

			if (TryInsertLocalPreviewItem(inventory, slotPrefab, slotId, failure))
				inserted++;
			else if (firstFailure.IsEmpty())
				firstFailure = failure;
		}

		IEntity previewCharacter = m_PreviewSourceCharacter;
		GetGame().GetCallqueue().CallLater(FinishFallbackPreviewMannequin, PREVIEW_DRESS_DELAY_MS, false, previewCharacter, inserted, removed, firstFailure);
	}

	protected void FinishFallbackPreviewMannequin(IEntity previewCharacter, int inserted, int removed, string firstFailure)
	{
		if (!previewCharacter || previewCharacter != m_PreviewSourceCharacter)
			return;

		m_PreviewEntity = CreatePreviewCloneFromDressedSource(previewCharacter);
		if (m_PreviewEntity)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(previewCharacter);
			m_PreviewSourceCharacter = null;
		}
		else
		{
			m_PreviewEntity = previewCharacter;
		}

		m_PreviewSourceCharacter = null;
		FinalizeDressedPreviewEntity(inserted, firstFailure, 0);
	}

	protected void FinalizeDressedPreviewEntity(int inserted, string firstFailure, int retryCount)
	{
		if (!m_PreviewEntity)
			return;

		m_PreviewEntity.SetOrigin(vector.Up);
		m_PreviewEntity.Update();
		SetPreviewEntityQualityRecursive(m_PreviewEntity);
		if (!HasUsablePreviewBounds(m_PreviewEntity))
		{
			if (retryCount < PREVIEW_BOUNDS_RETRY_COUNT)
			{
				GetGame().GetCallqueue().CallLater(FinalizeDressedPreviewEntity, PREVIEW_BOUNDS_RETRY_DELAY_MS, false, inserted, firstFailure, retryCount + 1);
				return;
			}

			m_sPreviewStatus = "preview mannequin had no bounds";
			m_bPreviewSpawned = false;
			return;
		}

		m_bPreviewSpawned = true;
		m_iPreviewItemCount = inserted;
		if (firstFailure.IsEmpty())
			m_sPreviewStatus = string.Format("preview mannequin with %1 item(s)", inserted);
		else
			m_sPreviewStatus = string.Format("preview skipped %1", ShortenText(firstFailure, 34));

		UpdatePreviewedEntityMetrics();
		UpdatePreviewCameraImmediate(true, "fallback");
	}

	protected IEntity CreatePreviewCloneFromDressedSource(IEntity sourceCharacter)
	{
		if (!sourceCharacter)
			return null;

		InventoryItemComponent previewItem = InventoryItemComponent.Cast(sourceCharacter.FindComponent(InventoryItemComponent));
		if (!previewItem)
			return null;

		ResetPreviewCameraToReferenceDefault();
		return previewItem.CreatePreviewEntity(m_PreviewWorld, PREVIEW_CAMERA);
	}

	protected bool HasUsablePreviewBounds(IEntity entity)
	{
		if (!entity)
			return false;

		vector mins;
		vector maxs;
		entity.GetWorldBounds(mins, maxs);
		return vector.Distance(mins, maxs) > 0.1;
	}

	protected void UpdatePreviewedEntityMetrics()
	{
		m_vPreviewedEntityCenterWorld = vector.Zero;
		m_fPreviewedEntitySize = 0;
		m_fPreviewedEntityDefaultDistance = 1.0;
		if (!m_PreviewEntity)
			return;

		vector mins;
		vector maxs;
		m_PreviewEntity.GetWorldBounds(mins, maxs);
		m_vPreviewedEntityCenterWorld = vector.Lerp(mins, maxs, 0.5);
		m_fPreviewedEntitySize = vector.Distance(mins, maxs);
		m_fPreviewedEntityDefaultDistance = Math.Max(1.0, m_fPreviewedEntitySize * 1.5);
	}

	protected void SetPreviewEntityQualityRecursive(IEntity entity)
	{
		if (!entity)
			return;

		entity.SetVComponentFlags(VCFlags.NOFILTER & VCFlags.NOLIGHT);
		entity.SetFixedLOD(0);
		IEntity child = entity.GetChildren();
		while (child)
		{
			SetPreviewEntityQualityRecursive(child);
			child = child.GetSibling();
		}
	}

	protected bool PositionPreviewEntityAtStage(IEntity entity, vector targetCenter)
	{
		if (!entity)
			return false;

		vector mins;
		vector maxs;
		entity.GetWorldBounds(mins, maxs);
		float size = vector.Distance(mins, maxs);
		if (size <= 0.1)
			return false;

		vector center = vector.Lerp(mins, maxs, 0.5);
		vector origin = entity.GetOrigin();
		entity.SetOrigin(origin + (targetCenter - center));
		entity.Update();
		return true;
	}

	protected int ClearLocalPreviewInventory(SCR_InventoryStorageManagerComponent inventory)
	{
		if (!inventory)
			return 0;

		array<IEntity> items = {};
		inventory.GetItems(items, EStoragePurpose.PURPOSE_ANY);
		int removed;
		foreach (IEntity item : items)
		{
			if (item && inventory.TryDeleteItem(item))
				removed++;
		}

		return removed;
	}

	protected bool TryInsertLocalPreviewItem(SCR_InventoryStorageManagerComponent inventory, string prefab, string slotId, out string failure)
	{
		failure = "";
		if (!inventory || prefab.IsEmpty())
		{
			failure = "missing preview inventory";
			return false;
		}

		ResourceName resourceName = prefab;
		Resource loaded = Resource.Load(resourceName);
		if (!loaded || !loaded.IsValid())
		{
			failure = "resource failed";
			return false;
		}

		EntitySpawnParams params = new EntitySpawnParams;
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = vector.Zero;
		IEntity itemEntity = GetGame().SpawnEntityPrefabLocal(loaded, m_PreviewWorld, params);
		if (!itemEntity)
		{
			failure = "spawn failed";
			return false;
		}

		if (!InventoryItemComponent.Cast(itemEntity.FindComponent(InventoryItemComponent)))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(itemEntity);
			failure = "not inventory";
			return false;
		}

		BaseInventoryStorageComponent storage;
		int storageSlotId = -1;
		if (!ResolveExactPreviewInsertTarget(slotId, storage, storageSlotId))
			storage = inventory.FindStorageForInsert(itemEntity, null, EStoragePurpose.PURPOSE_ANY);

		if (!storage)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(itemEntity);
			failure = "no slot";
			return false;
		}

		HST_LoadoutEditorInsertCallback callback = new HST_LoadoutEditorInsertCallback();
		callback.m_TemporaryEntity = itemEntity;
		inventory.TryInsertItemInStorage(itemEntity, storage, storageSlotId, callback);
		return true;
	}

	protected bool ResolveExactPreviewInsertTarget(string slotId, out BaseInventoryStorageComponent storage, out int storageSlotId)
	{
		storage = null;
		storageSlotId = -1;
		if (!m_PreviewSourceCharacter || slotId.IsEmpty())
			return false;

		if (slotId.IndexOf(NODE_LOADOUT_PREFIX) == 0)
		{
			int loadoutSlotIndex = ParseSinglePreviewNodeIndex(slotId, NODE_LOADOUT_PREFIX);
			SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(m_PreviewSourceCharacter.FindComponent(SCR_InventoryStorageManagerComponent));
			if (!inventory)
				return false;

			SCR_CharacterInventoryStorageComponent characterStorage = inventory.GetCharacterStorage();
			if (!characterStorage || loadoutSlotIndex < 0 || loadoutSlotIndex >= characterStorage.GetSlotsCount())
				return false;

			InventoryStorageSlot loadoutSlot = characterStorage.GetSlot(loadoutSlotIndex);
			if (!loadoutSlot)
				return false;

			storage = characterStorage;
			storageSlotId = loadoutSlot.GetID();
			return true;
		}

		if (slotId.IndexOf(NODE_WEAPON_PREFIX) == 0)
		{
			BaseInventoryStorageComponent weaponStorage;
			InventoryStorageSlot weaponSlot;
			if (!ResolvePreviewIndexedStorageSlot(m_PreviewSourceCharacter, slotId, NODE_WEAPON_PREFIX, weaponStorage, weaponSlot))
				return false;

			storage = weaponStorage;
			storageSlotId = weaponSlot.GetID();
			return true;
		}

		if (slotId.IndexOf(NODE_ATTACHMENT_PREFIX) == 0)
		{
			int weaponStorageIndex;
			int weaponSlotIndex;
			int attachmentSlotIndex;
			if (!ParseThreePreviewNodeIndexes(slotId, NODE_ATTACHMENT_PREFIX, weaponStorageIndex, weaponSlotIndex, attachmentSlotIndex))
				return false;

			array<BaseInventoryStorageComponent> storages = {};
			FindPreviewInventoryStoragesWithSlots(m_PreviewSourceCharacter, storages);
			if (weaponStorageIndex < 0 || weaponStorageIndex >= storages.Count())
				return false;

			BaseInventoryStorageComponent weaponStorage = storages[weaponStorageIndex];
			if (!weaponStorage || weaponSlotIndex < 0 || weaponSlotIndex >= weaponStorage.GetSlotsCount())
				return false;

			IEntity weaponEntity = weaponStorage.GetSlot(weaponSlotIndex).GetAttachedEntity();
			if (!weaponEntity)
				return false;

			SCR_WeaponAttachmentsStorageComponent attachmentStorage = SCR_WeaponAttachmentsStorageComponent.Cast(weaponEntity.FindComponent(SCR_WeaponAttachmentsStorageComponent));
			if (!attachmentStorage || attachmentSlotIndex < 0 || attachmentSlotIndex >= attachmentStorage.GetSlotsCount())
				return false;

			InventoryStorageSlot attachmentSlot = attachmentStorage.GetSlot(attachmentSlotIndex);
			if (!attachmentSlot)
				return false;

			storage = attachmentStorage;
			storageSlotId = attachmentSlot.GetID();
			return true;
		}

		return false;
	}

	protected bool ResolvePreviewSource(out IEntity previewSource, out string previewMode, out string previewLabel)
	{
		previewSource = null;
		previewMode = PREVIEW_MODE_CHARACTER;
		previewLabel = "character";

		IEntity character = ResolvePreviewEditedCharacter();
		if (!character)
			return false;

		string nodeId = m_sSelectedNodeId;
		if ((nodeId.IsEmpty() || FindStringIndex(m_aNodeIds, nodeId) < 0) && !m_sSelectedSlotId.IsEmpty())
			nodeId = m_sSelectedSlotId;

		if (!nodeId.IsEmpty() && ResolvePreviewEntityForNode(character, nodeId, previewSource, previewMode))
		{
			previewLabel = ResolvePreviewLabel(nodeId, previewSource);
			return true;
		}

		previewSource = character;
		previewMode = PREVIEW_MODE_CHARACTER;
		previewLabel = "character";
		return true;
	}

	protected IEntity ResolvePreviewEditedCharacter()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (playerManager)
		{
			IEntity controlledEntity = playerManager.GetPlayerControlledEntity(ResolveLocalPlayerId());
			if (controlledEntity)
			{
				m_PreviewEditedCharacter = controlledEntity;
				return controlledEntity;
			}
		}

		return m_PreviewEditedCharacter;
	}

	protected bool ResolvePreviewEntityForNode(IEntity character, string nodeId, out IEntity previewSource, out string previewMode)
	{
		previewSource = null;
		previewMode = PREVIEW_MODE_CHARACTER;
		if (!character || nodeId.IsEmpty())
			return false;

		if (nodeId.IndexOf(NODE_LOADOUT_PREFIX) == 0)
		{
			previewSource = character;
			previewMode = PREVIEW_MODE_CHARACTER;
			return true;
		}

		if (nodeId.IndexOf(NODE_STORAGE_ITEM_PREFIX) == 0)
		{
			int containerSlotIndex;
			int itemIndex;
			if (!ParseTwoPreviewNodeIndexes(nodeId, NODE_STORAGE_ITEM_PREFIX, containerSlotIndex, itemIndex))
				return false;

			array<BaseInventoryStorageComponent> storages = {};
			if (ResolvePreviewStorageTargets(character, NODE_STORAGE_PREFIX + string.Format("%1", containerSlotIndex), storages) <= 0)
				return false;

			array<IEntity> contents = {};
			array<IEntity> visited = {};
			GatherPreviewStorageContentEntitiesFromStorages(storages, contents, visited);
			if (itemIndex < 0 || itemIndex >= contents.Count())
				return false;

			previewSource = contents[itemIndex];
			previewMode = PREVIEW_MODE_ENTITY;
			return previewSource != null;
		}

		if (nodeId.IndexOf(NODE_STORAGE_PREFIX) == 0)
		{
			int containerSlotIndex = ParseSinglePreviewNodeIndex(nodeId, NODE_STORAGE_PREFIX);
			previewSource = ResolvePreviewStorageContainer(character, containerSlotIndex);
			previewMode = PREVIEW_MODE_ENTITY;
			return previewSource != null;
		}

		if (nodeId.IndexOf(NODE_WEAPON_PREFIX) == 0)
		{
			BaseInventoryStorageComponent weaponStorage;
			InventoryStorageSlot weaponSlot;
			if (!ResolvePreviewIndexedStorageSlot(character, nodeId, NODE_WEAPON_PREFIX, weaponStorage, weaponSlot))
				return false;

			previewSource = weaponSlot.GetAttachedEntity();
			previewMode = PREVIEW_MODE_ENTITY;
			return previewSource != null;
		}

		if (nodeId.IndexOf(NODE_ATTACHMENT_PREFIX) == 0)
		{
			int weaponStorageIndex;
			int weaponSlotIndex;
			int attachmentSlotIndex;
			if (!ParseThreePreviewNodeIndexes(nodeId, NODE_ATTACHMENT_PREFIX, weaponStorageIndex, weaponSlotIndex, attachmentSlotIndex))
				return false;

			array<BaseInventoryStorageComponent> storages = {};
			FindPreviewInventoryStoragesWithSlots(character, storages);
			if (weaponStorageIndex < 0 || weaponStorageIndex >= storages.Count())
				return false;

			BaseInventoryStorageComponent weaponStorage = storages[weaponStorageIndex];
			if (!weaponStorage || weaponSlotIndex < 0 || weaponSlotIndex >= weaponStorage.GetSlotsCount())
				return false;

			IEntity weaponEntity = weaponStorage.GetSlot(weaponSlotIndex).GetAttachedEntity();
			if (!weaponEntity)
				return false;

			SCR_WeaponAttachmentsStorageComponent attachmentStorage = SCR_WeaponAttachmentsStorageComponent.Cast(weaponEntity.FindComponent(SCR_WeaponAttachmentsStorageComponent));
			if (!attachmentStorage || attachmentSlotIndex < 0 || attachmentSlotIndex >= attachmentStorage.GetSlotsCount())
				return false;

			previewSource = weaponEntity;
			previewMode = PREVIEW_MODE_ENTITY;
			return previewSource != null;
		}

		return false;
	}

	protected bool ResolvePreviewIndexedStorageSlot(IEntity character, string nodeId, string prefix, out BaseInventoryStorageComponent storage, out InventoryStorageSlot slot)
	{
		storage = null;
		slot = null;
		int storageIndex;
		int slotIndex;
		if (!ParseTwoPreviewNodeIndexes(nodeId, prefix, storageIndex, slotIndex))
			return false;

		array<BaseInventoryStorageComponent> storages = {};
		FindPreviewInventoryStoragesWithSlots(character, storages);
		if (storageIndex < 0 || storageIndex >= storages.Count())
			return false;

		storage = storages[storageIndex];
		if (!storage || slotIndex < 0 || slotIndex >= storage.GetSlotsCount())
			return false;

		slot = storage.GetSlot(slotIndex);
		return slot != null;
	}

	protected IEntity ResolvePreviewStorageContainer(IEntity character, int containerSlotIndex)
	{
		if (!character || containerSlotIndex < 0)
			return null;

		SCR_CharacterInventoryStorageComponent characterStorage = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (!characterStorage || containerSlotIndex >= characterStorage.GetSlotsCount())
			return null;

		InventoryStorageSlot slot = characterStorage.GetSlot(containerSlotIndex);
		if (!slot)
			return null;

		return slot.GetAttachedEntity();
	}

	protected BaseInventoryStorageComponent ResolvePreviewStorageTarget(IEntity character, int containerSlotIndex)
	{
		array<BaseInventoryStorageComponent> targets = {};
		if (ResolvePreviewStorageTargets(character, NODE_STORAGE_PREFIX + string.Format("%1", containerSlotIndex), targets) <= 0)
			return null;

		return targets[0];
	}

	protected int ResolvePreviewStorageTargets(IEntity character, string nodeId, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		if (!character || nodeId.IsEmpty())
			return 0;

		int containerSlotIndex = -1;
		if (nodeId.IndexOf(NODE_STORAGE_ITEM_PREFIX) == 0)
			containerSlotIndex = ParseSinglePreviewNodeIndex(nodeId, NODE_STORAGE_ITEM_PREFIX);
		else if (nodeId.IndexOf(NODE_STORAGE_PREFIX) == 0)
			containerSlotIndex = ParseSinglePreviewNodeIndex(nodeId, NODE_STORAGE_PREFIX);

		if (containerSlotIndex < 0)
			return 0;

		IEntity container = ResolvePreviewStorageContainer(character, containerSlotIndex);
		if (!container)
			return 0;

		FindPreviewCargoDepositStorages(container, outStorages);
		return outStorages.Count();
	}

	protected int FindUsablePreviewDepositStorages(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		if (!entity)
			return 0;

		array<Managed> components = {};
		entity.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed component : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!IsUsablePreviewDepositStorage(storage) || outStorages.Find(storage) >= 0)
				continue;

			outStorages.Insert(storage);
		}

		return outStorages.Count();
	}

	protected int FindPreviewInventoryStoragesWithSlots(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		if (!entity)
			return 0;

		array<Managed> components = {};
		entity.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed component : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!storage || storage.GetSlotsCount() <= 0)
				continue;

			outStorages.Insert(storage);
		}

		return outStorages.Count();
	}

	protected bool IsUsablePreviewDepositStorage(BaseInventoryStorageComponent storage)
	{
		if (!storage || storage.GetSlotsCount() <= 0)
			return false;

		if (SCR_SalineStorageComponent.Cast(storage) || SCR_TourniquetStorageComponent.Cast(storage))
			return false;

		if (SCR_Enum.HasFlag(storage.GetPurpose(), EStoragePurpose.PURPOSE_DEPOSIT))
			return true;

		if (SCR_UniversalInventoryStorageComponent.Cast(storage))
			return true;

		return false;
	}

	protected bool IsPreviewCargoDepositStorage(BaseInventoryStorageComponent storage)
	{
		if (!storage || storage.GetSlotsCount() <= 0)
			return false;

		if (SCR_SalineStorageComponent.Cast(storage) || SCR_TourniquetStorageComponent.Cast(storage))
			return false;

		if (SCR_Enum.HasFlag(storage.GetPurpose(), EStoragePurpose.PURPOSE_DEPOSIT))
			return true;

		if (SCR_UniversalInventoryStorageComponent.Cast(storage))
			return true;

		return false;
	}

	protected int FindPreviewCargoDepositStorages(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages)
	{
		array<IEntity> visited = {};
		FindPreviewCargoDepositStoragesRecursive(entity, outStorages, visited, 0);
		return outStorages.Count();
	}

	protected void FindPreviewCargoDepositStoragesRecursive(IEntity entity, notnull array<BaseInventoryStorageComponent> outStorages, notnull array<IEntity> visited, int depth)
	{
		if (!entity || depth > 8 || visited.Find(entity) >= 0)
			return;

		visited.Insert(entity);
		array<Managed> components = {};
		entity.FindComponents(BaseInventoryStorageComponent, components);
		foreach (Managed component : components)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(component);
			if (!storage || storage.GetSlotsCount() <= 0)
				continue;

			if (IsPreviewCargoDepositStorage(storage) && outStorages.Find(storage) < 0)
				outStorages.Insert(storage);

			if (IsPreviewStructuralAttachmentStorage(storage))
				FindPreviewCargoDepositStoragesInAttachedEntities(storage, outStorages, visited, depth + 1);
		}
	}

	protected bool IsPreviewStructuralAttachmentStorage(BaseInventoryStorageComponent storage)
	{
		if (!storage || storage.GetSlotsCount() <= 0)
			return false;

		return SCR_Enum.HasFlag(storage.GetPurpose(), EStoragePurpose.PURPOSE_EQUIPMENT_ATTACHMENT);
	}

	protected void FindPreviewCargoDepositStoragesInAttachedEntities(BaseInventoryStorageComponent storage, notnull array<BaseInventoryStorageComponent> outStorages, notnull array<IEntity> visited, int depth)
	{
		array<IEntity> attached = {};
		array<IEntity> attachedVisited = {};
		GatherPreviewStorageContentEntities(storage, attached, attachedVisited);
		foreach (IEntity child : attached)
			FindPreviewCargoDepositStoragesRecursive(child, outStorages, visited, depth);
	}

	protected void GatherPreviewStorageContentEntitiesFromStorages(notnull array<BaseInventoryStorageComponent> storages, notnull array<IEntity> outItems, notnull array<IEntity> visited)
	{
		foreach (BaseInventoryStorageComponent storage : storages)
			GatherPreviewStorageContentEntities(storage, outItems, visited);
	}

	protected void GatherPreviewStorageContentEntities(BaseInventoryStorageComponent storage, notnull array<IEntity> outItems, notnull array<IEntity> visited)
	{
		if (!storage)
			return;

		array<InventoryItemComponent> ownedItems = {};
		storage.GetOwnedItems(ownedItems);
		foreach (InventoryItemComponent itemComponent : ownedItems)
		{
			if (!itemComponent)
				continue;

			InventoryStorageSlot parentSlot = itemComponent.GetParentSlot();
			if (parentSlot)
				AddPreviewStorageContentEntity(parentSlot.GetAttachedEntity(), outItems, visited);
		}

		int slotCount = storage.GetSlotsCount();
		for (int slotIndex = 0; slotIndex < slotCount; slotIndex++)
		{
			InventoryStorageSlot slot = storage.GetSlot(slotIndex);
			if (slot)
				AddPreviewStorageContentEntity(slot.GetAttachedEntity(), outItems, visited);
		}
	}

	protected void AddPreviewStorageContentEntity(IEntity item, notnull array<IEntity> outItems, notnull array<IEntity> visited)
	{
		if (!item || visited.Find(item) >= 0)
			return;

		visited.Insert(item);
		outItems.Insert(item);
	}

	protected string ResolvePreviewLabel(string nodeId, IEntity previewSource)
	{
		int nodeIndex = FindStringIndex(m_aNodeIds, nodeId);
		if (nodeIndex >= 0 && nodeIndex < m_aNodeDisplays.Count() && !m_aNodeDisplays[nodeIndex].IsEmpty())
			return m_aNodeDisplays[nodeIndex];

		int slotIndex = FindStringIndex(m_aSlotIds, nodeId);
		if (slotIndex >= 0 && slotIndex < m_aSlotDisplays.Count() && !m_aSlotDisplays[slotIndex].IsEmpty())
			return m_aSlotDisplays[slotIndex];

		return ResolvePreviewEntityDisplayName(previewSource);
	}

	protected string ResolvePreviewEntityDisplayName(IEntity entity)
	{
		if (!entity)
			return "entity";

		string prefab = ResolvePreviewEntityPrefab(entity);
		string displayName;
		InventoryItemComponent itemComponent = InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
		if (itemComponent && itemComponent.GetUIInfo())
			displayName = itemComponent.GetUIInfo().GetName();

		displayName = HST_DisplayNameService.ResolveItemDisplayName(null, prefab, displayName);
		if (displayName.IsEmpty())
			return "entity";

		return displayName;
	}

	protected string ResolvePreviewEntityPrefab(IEntity entity)
	{
		if (!entity || !entity.GetPrefabData())
			return "";

		return entity.GetPrefabData().GetPrefabName();
	}

	protected string BuildPreviewEntityKey(IEntity entity)
	{
		if (!entity)
			return "none";

		BaseRplComponent rpl = BaseRplComponent.Cast(entity.FindComponent(BaseRplComponent));
		if (rpl)
			return string.Format("%1:%2", ResolvePreviewEntityPrefab(entity), rpl.Id());

		return string.Format("%1:%2", ResolvePreviewEntityPrefab(entity), entity);
	}

	protected bool ParseTwoPreviewNodeIndexes(string nodeId, string prefix, out int first, out int second)
	{
		first = -1;
		second = -1;
		if (nodeId.IndexOf(prefix) != 0)
			return false;

		string rest = nodeId.Substring(prefix.Length(), nodeId.Length() - prefix.Length());
		array<string> parts = {};
		rest.Split("_", parts, false);
		if (parts.Count() < 2)
			return false;

		first = parts[0].ToInt();
		second = parts[1].ToInt();
		return true;
	}

	protected bool ParseThreePreviewNodeIndexes(string nodeId, string prefix, out int first, out int second, out int third)
	{
		first = -1;
		second = -1;
		third = -1;
		if (nodeId.IndexOf(prefix) != 0)
			return false;

		string rest = nodeId.Substring(prefix.Length(), nodeId.Length() - prefix.Length());
		array<string> parts = {};
		rest.Split("_", parts, false);
		if (parts.Count() < 3)
			return false;

		first = parts[0].ToInt();
		second = parts[1].ToInt();
		third = parts[2].ToInt();
		return true;
	}

	protected int ParseSinglePreviewNodeIndex(string nodeId, string prefix)
	{
		if (nodeId.IndexOf(prefix) != 0)
			return -1;

		string rest = nodeId.Substring(prefix.Length(), nodeId.Length() - prefix.Length());
		array<string> parts = {};
		rest.Split("_", parts, false);
		if (parts.Count() == 0)
			return -1;

		return parts[0].ToInt();
	}

	protected string BuildPreviewRenderKey()
	{
		IEntity previewSource;
		string previewMode;
		string previewLabel;
		ResolvePreviewSource(previewSource, previewMode, previewLabel);

		string key = m_sPreviewPrefab;
		key = key + "|source:" + BuildPreviewEntityKey(previewSource);
		key = key + "|mode:" + previewMode;
		key = key + "|node:" + m_sSelectedNodeId;
		key = key + "|slot:" + m_sSelectedSlotId;
		for (int i = 0; i < m_aSlotPrefabs.Count(); i++)
			key = key + "|" + m_aSlotPrefabs[i] + ":" + m_aSlotQuantities[i];
		for (int nodeIndex = 0; nodeIndex < m_aNodePrefabs.Count(); nodeIndex++)
		{
			if (nodeIndex < m_aNodeIds.Count() && !m_aNodePrefabs[nodeIndex].IsEmpty())
				key = key + "|node:" + m_aNodeIds[nodeIndex] + ":" + m_aNodePrefabs[nodeIndex];
		}

		return key;
	}

	protected bool GetPreviewCharacterBounds(out vector mins, out vector maxs, out vector center, out float size)
	{
		mins = vector.Zero;
		maxs = vector.Zero;
		center = "0 1 0";
		size = 3.0;
		if (!m_PreviewEntity)
			return false;

		m_PreviewEntity.GetWorldBounds(mins, maxs);
		center = vector.Lerp(mins, maxs, 0.5);
		size = vector.Distance(mins, maxs);
		return size > 0.1;
	}

	protected void UpdatePreviewCamera(bool allowSelectedFocus)
	{
		if (!m_PreviewWorld)
			return;

		vector mins;
		vector maxs;
		vector boundsCenter;
		float boundsSize;
		bool hasBounds = GetPreviewCharacterBounds(mins, maxs, boundsCenter, boundsSize);
		if (!hasBounds)
		{
			ResetPreviewCameraToReferenceDefault();
			return;
		}

		float boundsHeight = maxs[1] - mins[1];
		if (boundsHeight < 1.6)
			boundsHeight = 1.6;

		UpdatePreviewedEntityMetrics();
		vector targetCameraLookPosition = m_vPreviewedEntityCenterWorld;
		vector targetCameraDirection = BuildPreviewCameraDirection(0);
		float cameraTargetDistance = Math.Clamp(m_fPreviewedEntitySize * 1.5, 3.2, 7.0);
		if (m_sPreviewSourceMode == PREVIEW_MODE_ENTITY)
		{
			cameraTargetDistance = Math.Clamp(m_fPreviewedEntitySize * 1.25, 0.85, 3.6);
			targetCameraLookPosition = m_vPreviewedEntityCenterWorld;
			targetCameraDirection = BuildPreviewCameraDirection(0);
			if (m_aCurrentCameraMatrix[3][0] < -0.1)
				targetCameraDirection = vector.Direction(targetCameraLookPosition, targetCameraLookPosition - vector.Right);
		}
		else if (m_iCameraMode == 1)
		{
			targetCameraLookPosition[1] = Math.Max(1.55, maxs[1] - (boundsHeight * 0.18));
			cameraTargetDistance = Math.Clamp(m_fPreviewedEntitySize * 1.05, 2.8, 4.6);
		}
		else if (m_iCameraMode == 2)
		{
			targetCameraLookPosition[1] = Math.Max(1.1, mins[1] + (boundsHeight * 0.55));
			cameraTargetDistance = Math.Clamp(m_fPreviewedEntitySize * 1.15, 3.2, 4.8);
		}

		m_vCameraTargetDirection = RotatePreviewCameraOffset(targetCameraDirection, m_fPreviewYawDegrees).Normalized();
		m_vCameraTargetLook = targetCameraLookPosition;
		m_fCameraTargetDistance = cameraTargetDistance;
		m_vCameraTargetPosition = m_vCameraTargetLook + (m_vCameraTargetDirection * m_fCameraTargetDistance);
	}

	protected vector BuildPreviewCameraDirection(float yawDegrees)
	{
		vector direction = vector.Direction(vector.Zero, vector.Forward + vector.Right);
		return RotatePreviewCameraOffset(direction, yawDegrees).Normalized();
	}

	protected vector RotatePreviewCameraOffset(vector offset, float yawDegrees)
	{
		float radians = yawDegrees * 0.01745329252;
		vector rotated = offset;
		rotated[0] = (offset[0] * Math.Cos(radians)) - (offset[2] * Math.Sin(radians));
		rotated[2] = (offset[0] * Math.Sin(radians)) + (offset[2] * Math.Cos(radians));
		return rotated;
	}

	protected void UpdatePreviewCameraImmediate(bool allowSelectedFocus, string reason)
	{
		UpdatePreviewCamera(allowSelectedFocus);
		Print(string.Format("h-istasi preview camera | %1 entity=%2 target=%3 look=%4 direction=%5 distance=%6 yaw=%7 sourceMode=%8", reason, m_PreviewEntity, m_vCameraTargetPosition, m_vCameraTargetLook, m_vCameraTargetDirection, m_fCameraTargetDistance, m_fPreviewYawDegrees, m_sPreviewSourceMode));
	}

	protected void AnimatePreviewCamera(float timeSlice)
	{
		if (!m_PreviewWorld || !m_bCameraInitialized)
			return;

		m_PreviewWorld.GetCamera(PREVIEW_CAMERA, m_aCurrentCameraMatrix);
		vector targetCameraPositionByDistance = m_vCameraTargetLook + (m_vCameraTargetDirection * m_fCameraTargetDistance);
		m_fCameraDistanceToTarget = vector.Distance(m_aCurrentCameraMatrix[3], targetCameraPositionByDistance);
		if (m_fCameraDistanceToTarget > 0.025)
			m_aCurrentCameraMatrix[3] = m_aCurrentCameraMatrix[3] + ((targetCameraPositionByDistance - m_aCurrentCameraMatrix[3]) * ((m_fCameraDistanceToTarget * 2.0) * timeSlice * 2.0));

		m_fCameraLookDistanceToTarget = vector.Distance(m_vCurrentCameraLookPosition, m_vCameraTargetLook);
		if (m_fCameraLookDistanceToTarget > 0.025)
			m_vCurrentCameraLookPosition = m_vCurrentCameraLookPosition + ((m_vCameraTargetLook - m_vCurrentCameraLookPosition) * (m_fCameraLookDistanceToTarget * timeSlice * 5.0));

		m_vCameraCurrentPosition = m_aCurrentCameraMatrix[3];
		m_vCameraCurrentLook = m_vCurrentCameraLookPosition;
		SCR_Math3D.LookAt(m_aCurrentCameraMatrix[3], m_vCurrentCameraLookPosition, vector.Up, m_aCurrentCameraMatrix);
		m_PreviewWorld.SetCameraEx(PREVIEW_CAMERA, m_aCurrentCameraMatrix);
		UpdatePreviewLightAngles();
	}

	protected void ApplyPreviewCameraImmediate(vector camera, vector look)
	{
		if (!m_PreviewWorld)
			return;

		m_vCameraCurrentPosition = camera;
		m_vCameraCurrentLook = look;
		m_vCurrentCameraLookPosition = look;
		m_bCameraInitialized = true;
		vector mat[4];
		SCR_Math3D.LookAt(camera, look, vector.Up, mat);
		for (int i = 0; i < 4; i++)
			m_aCurrentCameraMatrix[i] = mat[i];
		m_PreviewWorld.SetCameraEx(PREVIEW_CAMERA, mat);
		UpdatePreviewLightAngles();
	}

	protected void UpdatePreviewLightAngles()
	{
		if (!m_PreviewWorld || !m_PreviewLightEntity || !m_bPreviewLightAnglesInitialized || !m_bCameraInitialized)
			return;

		vector cameraMatrix[4];
		m_PreviewWorld.GetCamera(PREVIEW_CAMERA, cameraMatrix);
		vector cameraAngles = Math3D.MatrixToAngles(cameraMatrix);
		vector lightAngles = m_vPreviewLightBaseAngles;
		lightAngles[1] = m_vPreviewLightBaseAngles[1] + cameraAngles[0];
		m_PreviewLightEntity.SetAngles(lightAngles);
		m_PreviewLightEntity.Update();
	}

	protected ResourceName ResolveIconTexture(string key)
	{
		if (key == "clothing" || key == "headgear" || key == "vest" || key == "pants" || key == "boots" || key == "backpack" || key == "handwear")
			return ICON_CLOTHING;
		if (key == "weapons" || key == "weapon" || key == "launcher" || key == "sidearm")
			return ICON_WEAPONS;
		if (key == "attachments" || key == "attachment" || key == "wrench")
			return ICON_ATTACHMENTS;
		if (key == "storage" || key == "inventory")
			return ICON_STORAGE;
		if (key == "save")
			return ICON_SAVE;
		if (key == "settings")
			return ICON_SETTINGS;
		if (key == "magazine" || key == "ammo")
			return ICON_AMMO;
		if (key == "explosive" || key == "grenade" || key == "throwable")
			return ICON_THROWABLES;
		if (key == "medical")
			return ICON_MEDICAL;

		return ICON_EQUIPMENT;
	}

	protected bool CreateIconWidget(WorkspaceWidget workspace, Widget parent, ResourceName texture, int left, int top, int width, int height, int userId, int color)
	{
		if (!workspace || !parent || texture.IsEmpty())
			return false;

		Widget widget = workspace.CreateWidget(WidgetType.ImageWidgetTypeID, WidgetFlags.VISIBLE, null, 3710, parent);
		ImageWidget imageWidget = ImageWidget.Cast(widget);
		if (!imageWidget)
			return false;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		imageWidget.LoadImageTexture(0, texture);
		imageWidget.SetSize(width, height);
		widget.SetColorInt(color);
		if (userId != 0)
		{
			widget.SetUserID(userId);
			widget.AddHandler(m_WidgetHandler);
		}

		m_aWidgets.Insert(widget);
		return true;
	}

	protected void DeletePreviewWorld()
	{
		DeletePreviewEntities();
		DeletePreviewLight();

		if (m_PreviewStageEntity)
			SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewStageEntity);

		m_PreviewEditedCharacter = null;
		m_PreviewSourceEntity = null;
		m_PreviewStageEntity = null;
		m_PreviewWidget = null;
		m_PreviewWorld = null;
		m_PreviewWorldRef = null;
		m_bPreviewLightSpawnQueued = false;
		m_bPreviewLightAnglesInitialized = false;
		m_sPreviewSourceMode = PREVIEW_MODE_CHARACTER;
		m_sPreviewRenderKey = "";
		m_vCameraTargetPosition = "5 1 5";
		m_vCameraTargetLook = "0 0.5 0";
		m_vCameraCurrentPosition = "5 1 5";
		m_vCameraCurrentLook = "0 0.5 0";
		m_vPreviewedEntityCenterWorld = vector.Zero;
		m_fPreviewedEntitySize = 0;
		m_fPreviewedEntityDefaultDistance = 1.0;
		m_vCurrentCameraLookPosition = "0 0.5 0";
		m_vCameraTargetDirection = "1 0 1";
		m_fCameraTargetDistance = 1.0;
		m_fCameraDistanceToTarget = 0;
		m_fCameraLookDistanceToTarget = 0;
		m_bCameraInitialized = false;
	}

	protected void CreateButton(WorkspaceWidget workspace, Widget root, string label, int left, int top, int width, int height, int userId)
	{
		CreateRectWidget(workspace, root, left, top, width, height, 0xEE11171B, 0.98, userId);
		if (height >= 18 && width >= 18)
			CreateRectWidget(workspace, root, left, top, width, 3, 0xFFC4953B, 1.0, userId);

		int inset = 14;
		int fontSize = 14;
		int topInset = 7;
		if (width < 64)
			inset = 8;
		if (width < 32)
		{
			inset = 4;
			fontSize = 11;
			topInset = 4;
		}

		if (height <= 30)
		{
			fontSize = Math.Min(fontSize, 12);
			topInset = 7;
		}

		CreateTextWidget(workspace, root, label, left + inset, top + topInset, Math.Max(8, width - inset * 2), Math.Max(8, height - 8), fontSize, 0xFFF4EBD6, userId, true);
	}

	protected Widget CreateRectWidget(WorkspaceWidget workspace, Widget parent, int left, int top, int width, int height, int color, float opacity, int userId)
	{
		Widget widget = workspace.CreateWidget(WidgetType.CanvasWidgetTypeID, WidgetFlags.VISIBLE, null, 3650, parent);
		if (!widget)
			return null;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		SetupCanvasRect(widget, width, height, color);
		widget.SetOpacity(opacity);
		if (userId > 0)
		{
			widget.SetUserID(userId);
			widget.AddHandler(m_WidgetHandler);
		}

		m_aWidgets.Insert(widget);
		return widget;
	}

	protected bool SetupCanvasRect(Widget widget, int width, int height, int color)
	{
		CanvasWidget canvas = CanvasWidget.Cast(widget);
		if (!canvas)
			return false;

		HST_LoadoutEditorDrawCommandSet commandSet = new HST_LoadoutEditorDrawCommandSet();
		PolygonDrawCommand rectCommand = new PolygonDrawCommand();
		rectCommand.m_iColor = color;
		rectCommand.m_Vertices = BuildRectVertices(width, height);
		commandSet.m_aCommands.Insert(rectCommand);
		canvas.SetDrawCommands(commandSet.m_aCommands);
		m_aCanvasCommandSets.Insert(commandSet);
		return true;
	}

	protected ref array<float> BuildRectVertices(int width, int height)
	{
		ref array<float> vertices = {};
		float rectWidth = width;
		float rectHeight = height;
		vertices.Insert(0.0);
		vertices.Insert(0.0);
		vertices.Insert(rectWidth);
		vertices.Insert(0.0);
		vertices.Insert(rectWidth);
		vertices.Insert(rectHeight);
		vertices.Insert(0.0);
		vertices.Insert(rectHeight);
		return vertices;
	}

	protected TextWidget CreateTextWidget(WorkspaceWidget workspace, Widget parent, string text, int left, int top, int width, int height, int fontSize, int color, int userId, bool bold)
	{
		Widget widget = workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.NO_LOCALIZATION, null, 3700, parent);
		if (!widget)
			return null;

		FrameSlot.SetPos(widget, left, top);
		FrameSlot.SetSize(widget, width, height);
		TextWidget textWidget = TextWidget.Cast(widget);
		if (textWidget)
		{
			textWidget.SetText(text);
			textWidget.SetTextWrapping(false);
			textWidget.SetExactFontSize(fontSize);
			textWidget.SetLineSpacing(1.1);
			textWidget.SetBold(bold);
			textWidget.SetOutline(1, 0xDD000000);
			textWidget.SetShadow(2, 0xEE000000, 1, 1, 1);
		}

		widget.SetColorInt(color);
		if (userId > 0)
		{
			widget.SetUserID(userId);
			widget.AddHandler(m_WidgetHandler);
		}

		m_aWidgets.Insert(widget);
		return textWidget;
	}

	protected void ClearWidgets()
	{
		foreach (Widget widget : m_aWidgets)
		{
			if (widget)
				widget.RemoveFromHierarchy();
		}

		m_aWidgets.Clear();
		m_aCanvasCommandSets.Clear();
	}

	protected void ShowHint(string text)
	{
		SCR_HintManagerComponent hintManager = SCR_HintManagerComponent.GetInstance();
		if (hintManager)
			hintManager.ShowCustomHint(text, "h-istasi", 2.0);
	}

	protected string ShortenText(string text, int maxCharacters)
	{
		if (text.IsEmpty() || maxCharacters <= 0)
			return "";

		if (text.Length() <= maxCharacters)
			return text;

		if (maxCharacters <= 3)
			return text.Substring(0, maxCharacters);

		return text.Substring(0, maxCharacters - 3) + "...";
	}

	protected bool IsRemovedExternalPrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		if (prefab.Contains("595F2BF") || prefab.Contains("1337C0DE") || prefab.Contains("BADC0DED") || prefab.Contains("StatusQuo") || prefab.Contains("ContentPack"))
			return true;

		Resource loaded = Resource.Load(prefab);
		return !loaded || !loaded.IsValid();
	}

	protected void TryUnequipHeldItem(IEntity character)
	{
		if (!character)
			return;

		SCR_InventoryStorageManagerComponent storageManager = SCR_InventoryStorageManagerComponent.Cast(character.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!storageManager)
			return;

		SCR_CharacterInventoryStorageComponent storage = storageManager.GetCharacterStorage();
		if (storage)
			storage.UnequipCurrentItem();
	}

	protected int ResolveLocalPlayerId()
	{
		HST_CommandMenuRequestComponent request = HST_CommandMenuRequestComponent.GetLocalOwner();
		if (request && request.ResolveLocalPlayerId() > 0)
			return request.ResolveLocalPlayerId();

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return 1;

		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		if (playerIds.Count() == 0)
			return 1;

		return playerIds[0];
	}

	protected void RefreshLocalOwner(IEntity owner)
	{
		if (m_bIsLocalOwner)
			return;

		if (!IsLocalOwner(owner))
			return;

		m_bIsLocalOwner = true;
		s_LocalInstance = this;
	}

	protected bool IsLocalOwner(IEntity owner)
	{
		if (!owner)
			return false;

		BaseRplComponent rpl = BaseRplComponent.Cast(owner.FindComponent(BaseRplComponent));
		return !rpl || rpl.IsOwner();
	}
}
