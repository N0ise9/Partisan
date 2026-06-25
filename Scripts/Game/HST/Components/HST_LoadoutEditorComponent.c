class HST_LoadoutEditorWidgetHandler : ScriptedWidgetEventHandler
{
	protected HST_LoadoutEditorComponent m_Editor;

	void Bind(HST_LoadoutEditorComponent editor)
	{
		m_Editor = editor;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Editor || !w)
			return false;

		return m_Editor.OnWidgetClickedWithButton(w.GetUserID(), button);
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (!m_Editor || !w)
			return false;

		return m_Editor.OnWidgetMouseButtonDown(w.GetUserID(), x, y, button);
	}

	override bool OnMouseButtonUp(Widget w, int x, int y, int button)
	{
		if (!m_Editor || !w)
			return false;

		if (m_Editor.OnWidgetMouseButtonUp(w.GetUserID(), x, y, button))
			return true;

		return m_Editor.OnWidgetClickedWithButton(w.GetUserID(), button);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (!m_Editor || !w)
			return false;

		return m_Editor.OnWidgetMouseEnter(w.GetUserID(), x, y);
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (!m_Editor || !w)
			return false;

		return m_Editor.OnWidgetMouseLeave(w.GetUserID(), x, y);
	}
}

class HST_LoadoutEditorLayoutMetrics
{
	float m_fScale;
	bool m_bCompact;
	bool m_bVeryCompact;
	int m_iRailWidth;
	int m_iRailHeight;
	int m_iMainWidth;
	int m_iMainHeight;
	int m_iFontTiny;
	int m_iFontSmall;
	int m_iFontNormal;
	int m_iFontTitle;
}

[ComponentEditorProps(category: "h-istasi", description: "Client-side fullscreen h-istasi loadout editor")]
class HST_LoadoutEditorComponentClass : ScriptComponentClass
{
}

class HST_LoadoutEditorComponent : ScriptComponent
{
	static const ResourceName EDITOR_LAYOUT = "{5AF2D86E07D44A51}UI/layouts/HST_LoadoutEditor.layout";
	static const ResourceName ITEM_PREVIEW_CELL_LAYOUT = "{6B43C4A98B4F47F2}UI/layouts/HST_LoadoutItemPreviewCell.layout";
	static const ResourceName VERTICAL_SCROLL_LIST_LAYOUT = "{A7B8C9D001234560}UI/layouts/HST_VerticalScrollList.layout";
	static const ResourceName WRAP_SCROLL_GRID_LAYOUT = "{A7B8C9D001234570}UI/layouts/HST_WrapScrollGrid.layout";
	static const ResourceName UI_SOLID_WHITE = "{56137CA0F2D3ACE6}Assets/Images/solid_white_square.edds";
	static const ResourceName LOADOUT_NODE_ROW_LAYOUT = "{A7B8C9D0012345D0}UI/layouts/HST/Rows/HST_LoadoutNodeRow.layout";
	static const ResourceName LOADOUT_STORAGE_ROW_LAYOUT = "{A7B8C9D0012345E0}UI/layouts/HST/Rows/HST_LoadoutStorageRow.layout";
	static const ResourceName LOADOUT_STORAGE_ITEM_ROW_LAYOUT = "{A7B8C9D0012345F0}UI/layouts/HST/Rows/HST_LoadoutStorageItemRow.layout";
	static const ResourceName LOADOUT_CANDIDATE_TILE_LAYOUT = "{A7B8C9D001234600}UI/layouts/HST/Rows/HST_LoadoutCandidateTile.layout";
	static const ResourceName LOADOUT_STORAGE_CATEGORY_TAB_LAYOUT = "{A7B8C9D001234610}UI/layouts/HST/Rows/HST_LoadoutStorageCategoryTab.layout";
	static const ResourceName LOADOUT_TAB_BUTTON_LAYOUT = "{D66CFA01E5AA4400}UI/layouts/HST_LoadoutEditor_TabButton.layout";
	static const ResourceName DEFAULT_PREVIEW_PREFAB = "{84B40583F4D1B7A3}Prefabs/Characters/Factions/INDFOR/FIA/Character_FIA_Rifleman.et";
	static const ResourceName PREVIEW_WORLD_PREFAB = "{71D2E9B5588949D8}Prefabs/HST/HST_LoadoutPreviewWorld.et";
	static const ResourceName PREVIEW_LIGHTS_PREFAB = "{604FFDF1DE53BD1D}Prefabs/HST/HST_LoadoutPreviewLights.et";
	static const ResourceName PREVIEW_WORLD_MATERIAL_NEUTRAL = "{E5D84E1C698E6462}Prefabs/HST/HST_LoadoutPreviewSkySphere.emat";
	static const ResourceName PREVIEW_WORLD_MATERIAL_FIELD = "{A19E4F4D03C04B21}Prefabs/HST/HST_LoadoutPreviewWorld_Field.emat";
	static const ResourceName PREVIEW_WORLD_MATERIAL_SLATE = "{A19E4F4D03C04B22}Prefabs/HST/HST_LoadoutPreviewWorld_Slate.emat";
	static const ResourceName PREVIEW_WORLD_MATERIAL_DAWN = "{A19E4F4D03C04B23}Prefabs/HST/HST_LoadoutPreviewWorld_Dawn.emat";
	static const ResourceName PREVIEW_WORLD_MATERIAL_NIGHT = "{A19E4F4D03C04B24}Prefabs/HST/HST_LoadoutPreviewWorld_Night.emat";
	static const string PREVIEW_MODE_CHARACTER = "character";
	static const string PREVIEW_MODE_ENTITY = "entity";
	static const string EDITOR_INPUT_CONTEXT = "MenuContext";
	static const string EDITOR_CURSOR_CONTEXT = "InventoryContext";
	static const string LOADOUT_ACTION_TAB_LEFT = "MenuTabLeft";
	static const string LOADOUT_ACTION_TAB_RIGHT = "MenuTabRight";
	static const string LOADOUT_ACTION_BACK = "MenuBack";
	static const string LOADOUT_ACTION_SELECT_MOUSE = "MenuSelectMouse";
	static const string LOADOUT_ACTION_MOUSE_RIGHT = "MouseRight";
	static const string NODE_LOADOUT_PREFIX = "live_loadout_";
	static const string NODE_WEAPON_PREFIX = "live_weapon_";
	static const string NODE_ATTACHMENT_PREFIX = "live_attach_";
	static const string NODE_STORAGE_PREFIX = "live_storage_";
	static const string NODE_STORAGE_ITEM_PREFIX = "live_storage_item_";
	static const int LOADOUT_PREVIEW_Z = 1;
	static const int LOADOUT_UI_LAYER_Z = 100;
	static const int LOADOUT_PREVIEW_INPUT_Z = 2;
	static const int LOADOUT_PANEL_Z = 20;
	static const int LOADOUT_FLOATING_Z = 40;
	static const int CLOSE_WIDGET_ID = 9200;
	static const int CANCEL_WIDGET_ID = 9201;
	static const int APPLY_WIDGET_ID = 9202;
	static const int SAVE_WIDGET_ID = 9203;
	static const int DELETE_TEMPLATE_WIDGET_ID = 9204;
	static const int ITEM_PAGE_PREV_WIDGET_ID = 9205;
	static const int ITEM_PAGE_NEXT_WIDGET_ID = 9206;
	static const int TEMPLATE_PAGE_PREV_WIDGET_ID = 9209;
	static const int TEMPLATE_PAGE_NEXT_WIDGET_ID = 9210;
	static const int CLEAR_DRAFT_WIDGET_ID = 9211;
	static const int CAMERA_WIDGET_ID = 9212;
	static const int BACK_WIDGET_ID = 9214;
	static const int SWAP_WIDGET_ID = 9215;
	static const int RESET_PREVIEW_WIDGET_ID = 9216;
	static const int ROTATE_PREVIEW_WIDGET_ID = 9217;
	static const int PREVIEW_DRAG_WIDGET_ID = 9218;
	static const int SETTINGS_LIGHT_MINUS_WIDGET_ID = 9220;
	static const int SETTINGS_LIGHT_PLUS_WIDGET_ID = 9221;
	static const int SETTINGS_PANEL_PRESET_WIDGET_ID = 9222;
	static const int SETTINGS_ACCENT_PRESET_WIDGET_ID = 9223;
	static const int SETTINGS_ROW_PRESET_WIDGET_ID = 9224;
	static const int SETTINGS_WORLD_PRESET_WIDGET_ID = 9225;
	static const int SETTINGS_RESET_WIDGET_ID = 9226;
	static const int MODE_WIDGET_ID_BASE = 9250;
	static const int STORAGE_CATEGORY_WIDGET_ID_BASE = 9350;
	static const int ITEM_WIDGET_ID_BASE = 9400;
	static const int SLOT_REMOVE_WIDGET_ID_BASE = 9500;
	static const int SLOT_MINUS_WIDGET_ID_BASE = 9600;
	static const int SLOT_PLUS_WIDGET_ID_BASE = 9700;
	static const int TEMPLATE_WIDGET_ID_BASE = 9800;
	static const int SLOT_SELECT_WIDGET_ID_BASE = 9900;
	static const int NODE_WIDGET_ID_BASE = 20000;
	static const int CANDIDATE_WIDGET_ID_BASE = 40000;
	static const int REMOVE_SELECTED_NODE_WIDGET_ID = 70000;
	static const int ITEMS_PER_PAGE = 11;
	static const int TEMPLATES_PER_PAGE = 6;
	static const int LOADOUT_LAYOUT_FALLBACK_RAIL_WIDTH = 444;
	static const int LOADOUT_LAYOUT_FALLBACK_RAIL_HEIGHT = 856;
	static const int LOADOUT_LAYOUT_FALLBACK_MAIN_WIDTH = 704;
	static const int LOADOUT_LAYOUT_FALLBACK_MAIN_HEIGHT = 856;
	static const int PREVIEW_CAMERA = 0;
	static const int PREVIEW_LIGHT_DELAY_MS = 500;
	static const int PREVIEW_DRESS_DELAY_MS = 500;
	static const int PREVIEW_BOUNDS_RETRY_DELAY_MS = 120;
	static const int PREVIEW_BOUNDS_RETRY_COUNT = 4;
	static const int POST_ACTION_REFRESH_DELAY_MS = 350;
	static const int PANEL_BACK_COLOR = 0xF20D1114;
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
	static const ResourceName FALLBACK_PREVIEW_ICON = "{82311870FB87265B}Assets/512/equipment_icon.edds";

	protected static HST_LoadoutEditorComponent s_LocalInstance;

	protected IEntity m_OwnerEntity;
	protected bool m_bIsLocalOwner;
	protected bool m_bEditorOpen;
	protected bool m_bInputRegistered;
	protected string m_sEditorMode = "clothing";
	protected string m_sSelectedCategory = "clothing";
	protected string m_sSelectedSlotId;
	protected string m_sSelectedNodeId;
	protected string m_sSelectedStorageContainerNodeId;
	protected string m_sSelectedStoredItemNodeId;
	protected string m_sSelectedTemplateId;
	protected string m_sPreviewPrefab = DEFAULT_PREVIEW_PREFAB;
	protected string m_sStatusText = "Choose a saved template or build a draft from recovered arsenal gear.";
	protected string m_sPreviewStatus;
	protected string m_sPreviewPosition;
	protected string m_sLastResult;
	protected string m_sServerFailureText;
	protected bool m_bPreviewSpawned;
	protected bool m_bCandidateMode;
	protected bool m_bPostActionRefreshQueued;
	protected bool m_bPostLayoutRefreshQueued;
	protected bool m_bDebugLoggingEnabled;
	protected int m_iPreviewItemCount;
	protected int m_iDraftSlotCount;
	protected int m_iDraftItemCount;
	protected int m_iDraftFiniteCount;
	protected int m_iDraftInfiniteCount;
	protected int m_iItemPage;
	protected int m_iTemplatePage;
	protected int m_iCameraMode;
	protected float m_fPreviewYawDegrees;
	protected int m_iRawWorkspaceWidth = 1280;
	protected int m_iRawWorkspaceHeight = 850;
	protected int m_iEditorLayoutWidth = 1280;
	protected int m_iEditorLayoutHeight = 850;
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
	protected ref array<int> m_aNodeUsedCapacities = {};
	protected ref array<int> m_aNodeTotalCapacities = {};
	protected ref array<float> m_aNodeUsedVolumes = {};
	protected ref array<float> m_aNodeTotalVolumes = {};
	protected ref array<float> m_aNodeFreeVolumes = {};
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
	protected int m_iDebugPreviewCellLogIndex;
	protected ref HST_LoadoutEditorLayoutMetrics m_Layout;
	protected ref HST_LoadoutEditorWidgetHandler m_WidgetHandler;
	protected ref HST_LoadoutEditorVisualSettings m_VisualSettings;
	protected ref HST_LoadoutEditorVisualSettingsService m_VisualSettingsService;
	protected ScrollLayoutWidget m_SlotScroll;
	protected ScrollLayoutWidget m_CandidateScroll;
	protected ScrollLayoutWidget m_TemplateScroll;
	protected ScrollLayoutWidget m_StorageCandidateScroll;
	protected ScrollLayoutWidget m_StorageContainerScroll;
	protected ScrollLayoutWidget m_StorageContentScroll;
	protected float m_fSlotScrollY;
	protected float m_fCandidateScrollY;
	protected float m_fTemplateScrollY;
	protected float m_fStorageCandidateScrollY;
	protected float m_fStorageContainerScrollY;
	protected float m_fStorageContentScrollY;
	protected Widget m_RootWidget;
	protected Widget m_UILayerWidget;
	protected bool m_bRootFromLayout;
	protected ItemPreviewManagerEntity m_ItemPreviewManager;
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
	protected bool m_bPreviewDragFocused;
	protected bool m_bPreviewDragActive;
	protected int m_iPreviewDragLastX;
	protected int m_iPreviewDragLastY;

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
		if (!m_bIsLocalOwner)
			RefreshLocalOwner(owner);

		if (!m_bEditorOpen)
			return;

		m_iFrameSerial++;
		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.ActivateContext(EDITOR_INPUT_CONTEXT);
		inputManager.ActivateContext(EDITOR_CURSOR_CONTEXT);
		HandlePreviewDragInput(inputManager);
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

		if (!HST_UIRootService.Get().CanOpen(HST_EUIScreenMode.LOADOUT_EDITOR, "HST_LoadoutEditorComponent"))
			return;

		m_PreviewEditedCharacter = userEntity;
		m_PreviewSourceEntity = userEntity;
		m_sPreviewSourceMode = PREVIEW_MODE_CHARACTER;
		m_sPreviewRenderKey = "";
		m_bCameraInitialized = false;
		m_bPostActionRefreshQueued = false;
		m_iCameraMode = 0;
		m_fPreviewYawDegrees = 0;
		EnsureVisualSettings();
		TryUnequipHeldItem(userEntity);

		m_bEditorOpen = true;
		RegisterInputListeners();
		m_sEditorMode = "clothing";
		m_sSelectedCategory = "clothing";
		m_sSelectedSlotId = "";
		m_sSelectedNodeId = "";
		ClearStorageSelection();
		m_bCandidateMode = false;
		ResetLoadoutScroll();
		m_sStatusText = "Opening h-istasi arsenal editor. Counts, INF unlocks, and apply validation stay server-authoritative.";
		m_sLastResult = "requested editor session";
		if (!RenderEditor())
		{
			CloseEditorInternal(false);
			return;
		}

		RequestServerAction("loadout_editor_open_hq_arsenal", "");
	}

	bool OnWidgetClicked(int widgetId)
	{
		return OnWidgetClickedWithButton(widgetId, 0);
	}

	bool OnWidgetMouseEnter(int widgetId, int x, int y)
	{
		if (widgetId != PREVIEW_DRAG_WIDGET_ID)
			return false;

		m_bPreviewDragFocused = true;
		return false;
	}

	bool OnWidgetMouseLeave(int widgetId, int x, int y)
	{
		if (widgetId != PREVIEW_DRAG_WIDGET_ID)
			return false;

		m_bPreviewDragFocused = false;
		return false;
	}

	bool OnWidgetMouseButtonDown(int widgetId, int x, int y, int button)
	{
		if (widgetId != PREVIEW_DRAG_WIDGET_ID)
			return false;

		if (button == 0)
			return false;

		m_bPreviewDragActive = true;
		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		m_iPreviewDragLastX = mouseX;
		m_iPreviewDragLastY = mouseY;
		return true;
	}

	bool OnWidgetMouseButtonUp(int widgetId, int x, int y, int button)
	{
		if (widgetId != PREVIEW_DRAG_WIDGET_ID)
			return false;

		if (!m_bPreviewDragActive)
			return false;

		m_bPreviewDragActive = false;
		return true;
	}

	bool OnWidgetClickedWithButton(int widgetId, int button)
	{
		if (!m_bEditorOpen)
			return false;

		if (IsDuplicateWidgetActivation(widgetId, button))
			return true;

		DebugLog(string.Format("click widget=%1 button=%2 mode=%3 category=%4 candidateMode=%5 selectedNode=%6 selectedSlot=%7", widgetId, button, m_sEditorMode, m_sSelectedCategory, m_bCandidateMode, m_sSelectedNodeId, m_sSelectedSlotId));

		if (widgetId == CLOSE_WIDGET_ID || widgetId == CANCEL_WIDGET_ID)
		{
			CloseEditor(true);
			return true;
		}

		if (widgetId == BACK_WIDGET_ID)
		{
			HandleBackAction(false);
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
			ClearStorageSelection();
			m_bCandidateMode = false;
			m_sPreviewRenderKey = "";
			ResetLoadoutScroll();
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

		if (widgetId == SETTINGS_LIGHT_MINUS_WIDGET_ID)
		{
			AdjustPreviewLight(-1.0);
			return true;
		}

		if (widgetId == SETTINGS_LIGHT_PLUS_WIDGET_ID)
		{
			AdjustPreviewLight(1.0);
			return true;
		}

		if (widgetId == SETTINGS_PANEL_PRESET_WIDGET_ID)
		{
			CycleVisualPreset("panel");
			return true;
		}

		if (widgetId == SETTINGS_ACCENT_PRESET_WIDGET_ID)
		{
			CycleVisualPreset("accent");
			return true;
		}

		if (widgetId == SETTINGS_ROW_PRESET_WIDGET_ID)
		{
			CycleVisualPreset("row");
			return true;
		}

		if (widgetId == SETTINGS_WORLD_PRESET_WIDGET_ID)
		{
			CycleVisualPreset("world");
			return true;
		}

		if (widgetId == SETTINGS_RESET_WIDGET_ID)
		{
			m_VisualSettings = new HST_LoadoutEditorVisualSettings();
			SaveVisualSettings();
			ApplyPreviewLightSettings();
			RebuildPreviewStage();
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
			SwitchEditorModeByIndex(modeIndex);
			return true;
		}

		int storageCategoryIndex = widgetId - STORAGE_CATEGORY_WIDGET_ID_BASE;
		if (storageCategoryIndex >= 0 && storageCategoryIndex < GetStorageBrowserCategoryCount())
		{
			m_sSelectedCategory = GetStorageBrowserCategoryId(storageCategoryIndex);
			m_sEditorMode = "storage";
			m_iItemPage = 0;
			m_fCandidateScrollY = 0.0;
			m_fStorageCandidateScrollY = 0.0;
			EnsureSelectedStorageNode();
			EnsureCandidatePayloadForStorageContainer();
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
				string commandId = ResolveNodeSetCommand(nodeId);
				if (m_sEditorMode == "storage")
				{
					nodeId = ResolveSelectedStorageContainerNodeId();
					commandId = "add_storage_item";
				}
				else if (nodeId.IsEmpty() && candidateIndex < m_aCandidateNodeIds.Count())
				{
					nodeId = m_aCandidateNodeIds[candidateIndex];
					commandId = ResolveNodeSetCommand(nodeId);
				}

				if (nodeId.IsEmpty())
					return true;

				if (m_sEditorMode == "storage")
					m_sLastResult = "requested add storage item";
				else
					m_sLastResult = "requested " + BuildNodeActionLabel();
				RequestServerAction(commandId, nodeId + ":" + m_aCandidatePrefabs[candidateIndex]);
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
				if (m_sEditorMode == "storage" && nodeIndex < m_aNodeKinds.Count() && m_aNodeKinds[nodeIndex] == "storage")
				{
					SelectStorageContainerNode(nodeIndex);
					UpdatePreviewCamera(true);
					RenderEditor();
					return true;
				}

				if (m_sEditorMode == "storage" && nodeIndex < m_aNodeKinds.Count() && m_aNodeKinds[nodeIndex] == "storage_item")
				{
					if (button == 1)
					{
						m_sLastResult = "requested remove stored item";
						RequestServerAction("remove_node_item", m_aNodeIds[nodeIndex]);
						RenderEditor();
						return true;
					}

					string targetNodeId = ResolveSelectedStorageContainerNodeId();
					if (targetNodeId.IsEmpty() && nodeIndex < m_aNodeParents.Count())
						targetNodeId = m_aNodeParents[nodeIndex];
					if (targetNodeId.IsEmpty() || nodeIndex >= m_aNodePrefabs.Count() || m_aNodePrefabs[nodeIndex].IsEmpty())
						return true;

					m_sLastResult = "requested add another stored item";
					RequestServerAction("add_storage_item", targetNodeId + ":" + m_aNodePrefabs[nodeIndex]);
					RenderEditor();
					return true;
				}

				if (m_sEditorMode == "attachments" && nodeIndex < m_aNodeKinds.Count() && m_aNodeKinds[nodeIndex] == "weapon_slot")
				{
					m_sSelectedNodeId = m_aNodeIds[nodeIndex];
					m_sSelectedCategory = "attachment";
					m_sSelectedSlotId = "";
					m_bCandidateMode = false;
					m_iItemPage = 0;
					m_fCandidateScrollY = 0.0;
					m_fStorageCandidateScrollY = 0.0;
					UpdatePreviewCamera(true);
					RenderEditor();
					return true;
				}

				m_sSelectedNodeId = m_aNodeIds[nodeIndex];
				m_sSelectedCategory = ResolveNodeCategory(nodeIndex);
				m_sSelectedSlotId = ResolveSlotIdFromNodeId(m_sSelectedNodeId);
				m_bCandidateMode = true;
				m_iItemPage = 0;
				m_fCandidateScrollY = 0.0;
				m_fStorageCandidateScrollY = 0.0;
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
			m_fCandidateScrollY = 0.0;
			m_fStorageCandidateScrollY = 0.0;
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

	protected bool RegisterInputListeners()
	{
		if (m_bInputRegistered)
			return true;

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return false;

		inputManager.AddActionListener(LOADOUT_ACTION_TAB_LEFT, EActionTrigger.DOWN, OnLoadoutTabLeftInput);
		inputManager.AddActionListener(LOADOUT_ACTION_TAB_RIGHT, EActionTrigger.DOWN, OnLoadoutTabRightInput);
		inputManager.AddActionListener(LOADOUT_ACTION_BACK, EActionTrigger.DOWN, OnLoadoutBackInput);
		m_bInputRegistered = true;
		return true;
	}

	protected void UnregisterInputListeners()
	{
		if (!m_bInputRegistered)
			return;

		InputManager inputManager = GetGame().GetInputManager();
		if (!inputManager)
			return;

		inputManager.RemoveActionListener(LOADOUT_ACTION_TAB_LEFT, EActionTrigger.DOWN, OnLoadoutTabLeftInput);
		inputManager.RemoveActionListener(LOADOUT_ACTION_TAB_RIGHT, EActionTrigger.DOWN, OnLoadoutTabRightInput);
		inputManager.RemoveActionListener(LOADOUT_ACTION_BACK, EActionTrigger.DOWN, OnLoadoutBackInput);
		m_bInputRegistered = false;
	}

	protected void OnLoadoutTabLeftInput(float value, EActionTrigger reason)
	{
		if (reason != EActionTrigger.DOWN || !m_bEditorOpen)
			return;

		if (!IsLoadoutEditorTopmost())
			return;

		SwitchEditorModeByIndex(GetEditorModeIndex(m_sEditorMode) - 1);
	}

	protected void OnLoadoutTabRightInput(float value, EActionTrigger reason)
	{
		if (reason != EActionTrigger.DOWN || !m_bEditorOpen)
			return;

		if (!IsLoadoutEditorTopmost())
			return;

		SwitchEditorModeByIndex(GetEditorModeIndex(m_sEditorMode) + 1);
	}

	protected void OnLoadoutBackInput(float value, EActionTrigger reason)
	{
		if (reason != EActionTrigger.DOWN || !m_bEditorOpen)
			return;

		if (!IsLoadoutEditorTopmost())
			return;

		HandleBackAction(true);
	}

	protected bool IsLoadoutEditorTopmost()
	{
		return HST_UIRootService.Get().IsTopmost(HST_EUIScreenMode.LOADOUT_EDITOR, "HST_LoadoutEditorComponent");
	}

	protected void SwitchEditorModeByIndex(int modeIndex)
	{
		int modeCount = GetEditorModeCount();
		if (modeCount <= 0)
			return;

		if (modeIndex < 0)
			modeIndex = modeCount - 1;
		else if (modeIndex >= modeCount)
			modeIndex = 0;

		m_sEditorMode = GetEditorModeId(modeIndex);
		m_sSelectedCategory = ResolveDefaultCategoryForMode(m_sEditorMode);
		m_sSelectedSlotId = "";
		m_sSelectedNodeId = "";
		ClearStorageSelection();
		m_bCandidateMode = false;
		ResetLoadoutScroll();
		if (m_sEditorMode == "storage")
		{
			EnsureSelectedStorageNode();
			EnsureCandidatePayloadForStorageContainer();
		}
		UpdatePreviewCamera(true);
		RenderEditor();
	}

	protected bool CanNavigateBack()
	{
		if (m_sEditorMode == "storage" || m_sEditorMode == "save" || m_sEditorMode == "settings")
			return true;
		if (m_bCandidateMode)
			return true;
		if (m_sEditorMode == "attachments" && !m_sSelectedNodeId.IsEmpty())
			return true;
		if (m_sEditorMode != "clothing")
			return true;

		return false;
	}

	protected void HandleBackAction(bool closeAtRoot)
	{
		if (!CanNavigateBack())
		{
			if (closeAtRoot)
				CloseEditor(true);
			return;
		}

		if (m_sEditorMode == "storage")
		{
			m_sEditorMode = "clothing";
			m_sSelectedCategory = ResolveDefaultCategoryForMode(m_sEditorMode);
			m_sSelectedSlotId = "";
			m_sSelectedNodeId = "";
			ClearStorageSelection();
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
		ResetLoadoutScroll();
		UpdatePreviewCamera(true);
		RenderEditor();
	}

	void OnServerActionResult(string payload, string lastResult)
	{
		if (!m_bEditorOpen)
			return;

		if (IsDebugLoggingEnabled())
		{
			string payloadKind = "ignored";
			if (payload.Contains("HST_LOADOUT_CANDIDATES"))
				payloadKind = "candidates";
			else if (payload.Contains("HST_LOADOUT_EDITOR"))
				payloadKind = "editor";

			DebugLog(string.Format("server result kind=%1 result=%2", payloadKind, ShortenText(lastResult, 120)));
		}

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
		if (m_sEditorMode == "storage")
			EnsureCandidatePayloadForStorageContainer();
		else if (m_bCandidateMode)
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
		CloseEditorInternal(requestServer);
	}

	protected void CloseEditorInternal(bool requestServer)
	{
		if (!m_bEditorOpen)
			return;

		if (requestServer)
			RequestServerAction("loadout_editor_close", "");

		m_bEditorOpen = false;
		m_bPostActionRefreshQueued = false;
		m_bPostLayoutRefreshQueued = false;
		HST_UIRootService.Get().NotifyClosed(HST_EUIScreenMode.LOADOUT_EDITOR, "HST_LoadoutEditorComponent");
		UnregisterInputListeners();
		ClearWidgets();
		DeleteEditorRoot();
		DeletePreviewWorld();
	}

	protected void RequestServerAction(string commandId, string argument)
	{
		DebugLog(string.Format("request command=%1 arg=%2", commandId, ShortenText(argument, 120)));

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

	protected void EnsureVisualSettings()
	{
		if (m_VisualSettings)
		{
			m_VisualSettings.Normalize();
			return;
		}

		if (!m_VisualSettingsService)
			m_VisualSettingsService = new HST_LoadoutEditorVisualSettingsService();

		m_VisualSettings = m_VisualSettingsService.LoadOrCreate();
		if (!m_VisualSettings)
			m_VisualSettings = new HST_LoadoutEditorVisualSettings();

		m_VisualSettings.Normalize();
	}

	protected void SaveVisualSettings()
	{
		EnsureVisualSettings();
		if (!m_VisualSettingsService)
			m_VisualSettingsService = new HST_LoadoutEditorVisualSettingsService();

		m_VisualSettingsService.Save(m_VisualSettings);
	}

	protected void AdjustPreviewLight(float delta)
	{
		EnsureVisualSettings();
		m_VisualSettings.m_fPreviewLightLV = Math.Clamp(m_VisualSettings.m_fPreviewLightLV + delta, 0.0, 24.0);
		SaveVisualSettings();
		ApplyPreviewLightSettings();
		RenderEditor();
	}

	protected void CycleVisualPreset(string settingId)
	{
		EnsureVisualSettings();
		if (settingId == "panel")
			m_VisualSettings.m_iPanelPreset = (m_VisualSettings.m_iPanelPreset + 1) % 5;
		else if (settingId == "accent")
			m_VisualSettings.m_iAccentPreset = (m_VisualSettings.m_iAccentPreset + 1) % 6;
		else if (settingId == "row")
			m_VisualSettings.m_iRowPreset = (m_VisualSettings.m_iRowPreset + 1) % 5;
		else if (settingId == "world")
			m_VisualSettings.m_iWorldPreset = (m_VisualSettings.m_iWorldPreset + 1) % 5;

		SaveVisualSettings();
		if (settingId == "world")
			RebuildPreviewStage();
		RenderEditor();
	}

	protected int GetPanelBackColor()
	{
		EnsureVisualSettings();
		if (m_VisualSettings.m_iPanelPreset == 1)
			return 0xF2141B18;
		if (m_VisualSettings.m_iPanelPreset == 2)
			return 0xF211171D;
		if (m_VisualSettings.m_iPanelPreset == 3)
			return 0xF21D1713;
		if (m_VisualSettings.m_iPanelPreset == 4)
			return 0xF2181A23;

		return PANEL_BACK_COLOR;
	}

	protected int GetTabBackColor()
	{
		EnsureVisualSettings();
		if (m_VisualSettings.m_iPanelPreset == 1)
			return 0xF218211D;
		if (m_VisualSettings.m_iPanelPreset == 2)
			return 0xF2141C23;
		if (m_VisualSettings.m_iPanelPreset == 3)
			return 0xF2221C16;
		if (m_VisualSettings.m_iPanelPreset == 4)
			return 0xF21B1D28;

		return 0xF20D1114;
	}

	protected int GetAccentColor()
	{
		EnsureVisualSettings();
		if (m_VisualSettings.m_iAccentPreset == 1)
			return 0xFFD6B24A;
		if (m_VisualSettings.m_iAccentPreset == 2)
			return 0xFF8FB7A0;
		if (m_VisualSettings.m_iAccentPreset == 3)
			return 0xFFB88364;
		if (m_VisualSettings.m_iAccentPreset == 4)
			return 0xFF88AFC5;
		if (m_VisualSettings.m_iAccentPreset == 5)
			return 0xFFE6D78B;

		return 0xFFC4953B;
	}

	protected int GetBaseRowColor()
	{
		EnsureVisualSettings();
		if (m_VisualSettings.m_iRowPreset == 1)
			return 0xF21B2523;
		if (m_VisualSettings.m_iRowPreset == 2)
			return 0xF219232B;
		if (m_VisualSettings.m_iRowPreset == 3)
			return 0xF2231E19;
		if (m_VisualSettings.m_iRowPreset == 4)
			return 0xF21A1D25;

		return 0xF2151C20;
	}

	protected int GetSelectedRowColor()
	{
		EnsureVisualSettings();
		if (m_VisualSettings.m_iRowPreset == 1)
			return 0xFF5C6044;
		if (m_VisualSettings.m_iRowPreset == 2)
			return 0xFF59606A;
		if (m_VisualSettings.m_iRowPreset == 3)
			return 0xFF6B5B40;
		if (m_VisualSettings.m_iRowPreset == 4)
			return 0xFF4B5E6C;

		return 0xFF6F5124;
	}

	protected int GetMatchedRowColor()
	{
		EnsureVisualSettings();
		if (m_VisualSettings.m_iRowPreset == 1)
			return 0xFF314237;
		if (m_VisualSettings.m_iRowPreset == 2)
			return 0xFF303B47;
		if (m_VisualSettings.m_iRowPreset == 3)
			return 0xFF453A2D;
		if (m_VisualSettings.m_iRowPreset == 4)
			return 0xFF2D3B45;

		return 0xFF3D3520;
	}

	protected int GetVolumeTrackColor()
	{
		EnsureVisualSettings();
		if (m_VisualSettings.m_iRowPreset == 1)
			return 0xEE1D2B26;
		if (m_VisualSettings.m_iRowPreset == 2)
			return 0xEE1A2630;
		if (m_VisualSettings.m_iRowPreset == 3)
			return 0xEE2C251E;
		if (m_VisualSettings.m_iRowPreset == 4)
			return 0xEE1B232B;

		return 0xEE172126;
	}

	protected int GetPreviewWorldTintColor()
	{
		return 0xFFFFFFFF;
	}

	protected ResourceName GetPreviewWorldMaterial()
	{
		EnsureVisualSettings();
		if (m_VisualSettings.m_iWorldPreset == 1)
			return PREVIEW_WORLD_MATERIAL_FIELD;
		if (m_VisualSettings.m_iWorldPreset == 2)
			return PREVIEW_WORLD_MATERIAL_SLATE;
		if (m_VisualSettings.m_iWorldPreset == 3)
			return PREVIEW_WORLD_MATERIAL_DAWN;
		if (m_VisualSettings.m_iWorldPreset == 4)
			return PREVIEW_WORLD_MATERIAL_NIGHT;

		return PREVIEW_WORLD_MATERIAL_NEUTRAL;
	}

	protected string GetPresetLabel(string settingId, int preset)
	{
		if (settingId == "panel")
		{
			if (preset == 1)
				return "Field";
			if (preset == 2)
				return "Steel";
			if (preset == 3)
				return "Walnut";
			if (preset == 4)
				return "Navy";
			return "HST";
		}
		if (settingId == "accent")
		{
			if (preset == 1)
				return "Gold";
			if (preset == 2)
				return "Olive";
			if (preset == 3)
				return "Rust";
			if (preset == 4)
				return "Blue";
			if (preset == 5)
				return "Sand";
			return "Amber";
		}
		if (settingId == "row")
		{
			if (preset == 1)
				return "Green";
			if (preset == 2)
				return "Slate";
			if (preset == 3)
				return "Brown";
			if (preset == 4)
				return "Blue";
			return "Dark";
		}
		if (settingId == "world")
		{
			if (preset == 1)
				return "Field";
			if (preset == 2)
				return "Cool";
			if (preset == 3)
				return "Dawn";
			if (preset == 4)
				return "Night";
			return "Neutral";
		}

		return "";
	}

	protected bool RenderEditor()
	{
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return false;

		SaveLoadoutScrollOffsets();
		ClearWidgets();
		m_SlotScroll = null;
		m_CandidateScroll = null;
		m_TemplateScroll = null;
		m_StorageCandidateScroll = null;
		m_StorageContainerScroll = null;
		m_StorageContentScroll = null;
		if (!m_WidgetHandler)
		{
			m_WidgetHandler = new HST_LoadoutEditorWidgetHandler();
			m_WidgetHandler.Bind(this);
		}

		HST_UIWorkspaceMetrics.GetRawWorkspaceSize(workspace, m_iRawWorkspaceWidth, m_iRawWorkspaceHeight);
		HST_UIWorkspaceMetrics.GetLayoutSize(workspace, m_iEditorLayoutWidth, m_iEditorLayoutHeight);
		HST_UIWorkspaceMetrics.DebugWorkspaceMetrics(workspace, "HST_LoadoutEditor");
		m_iEditorWidth = m_iEditorLayoutWidth;
		m_iEditorHeight = m_iEditorLayoutHeight;
		EnsureVisualSettings();
		BuildResponsiveLayout();

		Widget root = EnsureEditorRoot(workspace);

		if (!root)
			return false;

		root.SetZOrder(HST_UIConstants.Z_LOADOUT_EDITOR);
		if (!HST_UIRootService.Get().RequestOpen(HST_EUIScreenMode.LOADOUT_EDITOR, "HST_LoadoutEditorComponent", root, true, true, false))
		{
			HST_UIDebug.LogLayoutRejected("loadout_editor", EDITOR_LAYOUT, root, "UI root refused loadout editor");
			DeleteEditorRoot();
			return false;
		}

		Widget previewContainer = root.FindAnyWidget("HST_LoadoutPreviewContainer");
		if (previewContainer)
		{
			previewContainer.SetVisible(true);
			previewContainer.SetOpacity(1.0);
			previewContainer.SetZOrder(LOADOUT_PREVIEW_Z);
		}

		m_PreviewWidget = RenderTargetWidget.Cast(root.FindAnyWidget("HST_LoadoutPreview"));
		ConfigurePreviewWidget();
		ConfigurePreviewDimmer(root);
		EnsurePreviewWorld();
		RefreshPreviewWorldLoadout();

		Widget uiRoot = ResolveUILayer(root);
		m_iDebugPreviewCellLogIndex = 0;
		ResetLoadoutModeRegions(uiRoot);
		ConfigurePreviewDragSurface(uiRoot);
		RenderLeftButtons(workspace, uiRoot);
		RenderModeTabs(workspace, uiRoot);
		// Editor visual states:
		// clothing/weapons/attachments: left slot rail + mannequin
		// candidate mode: selected slot editor + compatible candidates + mannequin
		// storage: left storage/contents + right add-items grid + mannequin
		// save/settings: fixed editor panels + mannequin/background
		if (m_sEditorMode == "storage")
		{
			RenderStorageRail(workspace, uiRoot);
			RenderStorageCandidatePanel(workspace, uiRoot);
		}
		else if (m_sEditorMode == "save")
		{
			RenderTemplatePanel(workspace, uiRoot);
		}
		else if (m_sEditorMode == "settings")
		{
			RenderSettingsPanel(workspace, uiRoot);
		}
		else if (m_bCandidateMode)
		{
			RenderCandidatePanel(workspace, uiRoot);
		}
		else
		{
			RenderSlotRail(workspace, uiRoot);
		}
		RenderPreviewStage(workspace, uiRoot);
		RenderContextHints(workspace, uiRoot);
		ApplyLoadoutLayerOrder(root);
		string debugSummary = string.Format("mode=%1 candidateMode=%2 selectedNode=%3 selectedSlot=%4 selectedStorage=%5 selectedStoredItem=%6 category=%7 nodes=%8 visibleNodes=%9", m_sEditorMode, m_bCandidateMode, ShortenText(m_sSelectedNodeId, 48), ShortenText(m_sSelectedSlotId, 48), ShortenText(m_sSelectedStorageContainerNodeId, 48), ShortenText(m_sSelectedStoredItemNodeId, 48), m_sSelectedCategory, m_aNodeIds.Count(), m_aVisibleNodeIndexes.Count());
		debugSummary = debugSummary + " " + string.Format("candidates=%1 visibleCandidates=%2 templates=%3 raw=%4x%5 layout=%6x%7", m_aCandidatePrefabs.Count(), m_aVisibleCandidateIndexes.Count(), m_aTemplateIds.Count(), m_iRawWorkspaceWidth, m_iRawWorkspaceHeight, m_iEditorLayoutWidth, m_iEditorLayoutHeight);
		debugSummary = debugSummary + " " + string.Format("rail=%1x%2 main=%3x%4 previewWidget=%5 previewWorld=%6", m_Layout.m_iRailWidth, m_Layout.m_iRailHeight, m_Layout.m_iMainWidth, m_Layout.m_iMainHeight, m_PreviewWidget != null, m_PreviewWorld != null);
		debugSummary = debugSummary + " " + string.Format("root=%1 ui=%2", HST_UIDebug.WidgetSummary(root), HST_UIDebug.WidgetSummary(uiRoot));
		HST_UIDebug.LogPopulation("loadout_editor", debugSummary);
		QueueLoadoutPostLayoutRefresh();
		return true;
	}

	protected Widget EnsureEditorRoot(WorkspaceWidget workspace)
	{
		if (m_RootWidget)
			return m_RootWidget;

		if (!m_RootWidget)
			m_RootWidget = workspace.CreateWidgets(EDITOR_LAYOUT, workspace);

		HST_UIDebug.LogLayoutCreate("loadout_editor", EDITOR_LAYOUT, m_RootWidget, workspace);
		if (!m_RootWidget)
			return null;

		m_RootWidget.SetVisible(true);
		m_RootWidget.SetOpacity(1.0);
		m_RootWidget.SetZOrder(HST_UIConstants.Z_LOADOUT_EDITOR);

		m_bRootFromLayout = true;
		m_UILayerWidget = null;
		HST_UIDebug.LogExpectedWidgetsCsv("loadout_editor", m_RootWidget, "HST_LoadoutEditorRoot|HST_LoadoutPreviewContainer|HST_LoadoutPreview|HST_LoadoutUILayer|PreviewDragSurface|LeftButtons|LoadoutBackButton|LoadoutCloseButton|TopTabs|TopTabItems|LeftRail|SlotRailScroll|SlotRailItems|StorageContainerItems|StorageContentItems|CandidateList|CandidateItems|StorageBrowser|StorageCandidateItems|SavePanel|TemplateItems|SettingsContent|Footer|Toast|ToastText|PreviewUnavailableText");
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
			m_UILayerWidget.SetVisible(true);
			m_UILayerWidget.SetOpacity(1.0);
			m_UILayerWidget.SetZOrder(LOADOUT_UI_LAYER_Z);
			return m_UILayerWidget;
		}

		return root;
	}

	protected Widget ResolveLoadoutRegion(Widget root, string name)
	{
		if (!root || name.IsEmpty())
			return root;

		Widget uiLayer = ResolveUILayer(root);
		if (!uiLayer)
			uiLayer = root;

		Widget region = uiLayer.FindAnyWidget(name);
		if (region)
		{
			region.SetVisible(true);
			region.SetOpacity(1.0);
			return region;
		}

		return uiLayer;
	}

	protected void ApplyLoadoutLayerOrder(Widget root)
	{
		if (!root)
			return;

		root.SetVisible(true);
		root.SetOpacity(1.0);
		root.SetZOrder(HST_UIConstants.Z_LOADOUT_EDITOR);

		ApplyLoadoutNamedWidgetLayer(root, "HST_LoadoutBackdrop", 0, false);
		ApplyLoadoutNamedWidgetLayer(root, "HST_LoadoutPreviewContainer", LOADOUT_PREVIEW_Z, true);
		if (m_PreviewWidget)
			ApplyLoadoutWidgetLayer(m_PreviewWidget, LOADOUT_PREVIEW_Z, true);
		ApplyLoadoutNamedWidgetLayer(root, "HST_LoadoutDimmer", LOADOUT_PREVIEW_Z + 1, false);

		Widget uiLayer = ResolveUILayer(root);
		if (!uiLayer)
			return;

		ApplyLoadoutWidgetLayer(uiLayer, LOADOUT_UI_LAYER_Z, true);
		ApplyLoadoutWidgetLayer(uiLayer.FindAnyWidget("PreviewDragSurface"), LOADOUT_PREVIEW_INPUT_Z, true);
		ApplyLoadoutWidgetLayer(uiLayer.FindAnyWidget("LeftButtons"), LOADOUT_PANEL_Z, false);
		ApplyLoadoutWidgetLayer(uiLayer.FindAnyWidget("TopTabs"), LOADOUT_PANEL_Z, false);
		ApplyLoadoutWidgetLayer(uiLayer.FindAnyWidget("LeftRail"), LOADOUT_PANEL_Z, false);
		ApplyLoadoutWidgetLayer(uiLayer.FindAnyWidget("CandidateList"), LOADOUT_PANEL_Z, false);
		ApplyLoadoutWidgetLayer(uiLayer.FindAnyWidget("StorageBrowser"), LOADOUT_PANEL_Z, false);
		ApplyLoadoutWidgetLayer(uiLayer.FindAnyWidget("SavePanel"), LOADOUT_PANEL_Z, false);
		ApplyLoadoutWidgetLayer(uiLayer.FindAnyWidget("Footer"), LOADOUT_FLOATING_Z, false);
		ApplyLoadoutWidgetLayer(uiLayer.FindAnyWidget("Toast"), LOADOUT_FLOATING_Z, false);
		ApplyLoadoutWidgetLayer(uiLayer.FindAnyWidget("PreviewUnavailableText"), LOADOUT_FLOATING_Z, false);
	}

	protected void ApplyLoadoutWidgetLayer(Widget widget, int zOrder, bool forceVisible)
	{
		if (!widget)
			return;

		if (forceVisible)
		{
			widget.SetVisible(true);
			widget.SetOpacity(1.0);
		}

		widget.SetZOrder(zOrder);
	}

	protected void ApplyLoadoutNamedWidgetLayer(Widget root, string widgetName, int zOrder, bool forceVisible)
	{
		if (!root || widgetName.IsEmpty())
			return;

		ApplyLoadoutWidgetLayer(root.FindAnyWidget(widgetName), zOrder, forceVisible);
	}

	protected void ResetLoadoutModeRegions(Widget root)
	{
		if (!root)
			return;

		SetLoadoutWidgetVisible(root, "LeftRail", false);
		SetLoadoutWidgetVisible(root, "EditedSlotHeader", false);
		SetLoadoutWidgetVisible(root, "SlotList", false);
		SetLoadoutWidgetVisible(root, "CandidateList", false);
		SetLoadoutWidgetVisible(root, "StorageBrowser", false);
		SetLoadoutWidgetVisible(root, "SavePanel", false);
	}

	protected void ShowSlotRailWidgets(Widget railRoot)
	{
		if (!railRoot)
			return;

		SetLoadoutWidgetVisible(railRoot, "SlotRailList", true);
		SetLoadoutWidgetVisible(railRoot, "SlotRailEmpty", false);
		SetLoadoutWidgetVisible(railRoot, "StorageRailTitle", false);
		SetLoadoutWidgetVisible(railRoot, "StorageContainerList", false);
		SetLoadoutWidgetVisible(railRoot, "StorageContainerEmpty", false);
		SetLoadoutWidgetVisible(railRoot, "StorageContentRule", false);
		SetLoadoutWidgetVisible(railRoot, "StorageContentTitle", false);
		SetLoadoutWidgetVisible(railRoot, "StorageContentList", false);
		SetLoadoutWidgetVisible(railRoot, "StorageContentEmpty", false);
	}

	protected void ShowStorageRailWidgets(Widget railRoot)
	{
		if (!railRoot)
			return;

		SetLoadoutWidgetVisible(railRoot, "SlotRailList", false);
		SetLoadoutWidgetVisible(railRoot, "SlotRailEmpty", false);
		SetLoadoutWidgetVisible(railRoot, "StorageRailTitle", true);
		SetLoadoutWidgetVisible(railRoot, "StorageContainerList", true);
		SetLoadoutWidgetVisible(railRoot, "StorageContainerEmpty", false);
		SetLoadoutWidgetVisible(railRoot, "StorageContentRule", true);
		SetLoadoutWidgetVisible(railRoot, "StorageContentTitle", true);
		SetLoadoutWidgetVisible(railRoot, "StorageContentList", true);
		SetLoadoutWidgetVisible(railRoot, "StorageContentEmpty", false);
	}

	protected void ShowCandidatePanelWidgets(Widget panelRoot)
	{
		if (!panelRoot)
			return;

		SetLoadoutWidgetVisible(panelRoot, "CandidateRemoveButton", false);
		SetLoadoutWidgetVisible(panelRoot, "CandidateListFrame", true);
		SetLoadoutWidgetVisible(panelRoot, "CandidateEmpty", false);
	}

	protected void ShowTemplatePanelWidgets(Widget panelRoot)
	{
		if (!panelRoot)
			return;

		SetLoadoutWidgetVisible(panelRoot, "SettingsContent", false);
		SetLoadoutWidgetVisible(panelRoot, "TemplateSaveButton", true);
		SetLoadoutWidgetVisible(panelRoot, "TemplateClearButton", true);
		SetLoadoutWidgetVisible(panelRoot, "TemplateList", true);
		SetLoadoutWidgetVisible(panelRoot, "TemplateEmpty", false);
	}

	protected void HideTemplatePanelWidgets(Widget panelRoot)
	{
		if (!panelRoot)
			return;

		SetLoadoutWidgetVisible(panelRoot, "TemplateSaveButton", false);
		SetLoadoutWidgetVisible(panelRoot, "TemplateClearButton", false);
		SetLoadoutWidgetVisible(panelRoot, "TemplateList", false);
		SetLoadoutWidgetVisible(panelRoot, "TemplateEmpty", false);
	}

	protected void ShowSettingsPanelWidgets(Widget panelRoot)
	{
		if (!panelRoot)
			return;

		SetLoadoutWidgetVisible(panelRoot, "SettingsContent", true);
	}

	protected void ConfigureLoadoutPanelShell(Widget panelRoot, string title)
	{
		if (!panelRoot)
			return;

		SetLoadoutWidgetColor(panelRoot, "SavePanelBackground", GetPanelBackColor(), 1.0);
		SetLoadoutWidgetColor(panelRoot, "SavePanelAccent", GetAccentColor(), 1.0);
		SetLoadoutText(panelRoot, "SavePanelTitle", title, 0xFFEFE2C4, m_Layout.m_iFontTitle, true, false);
	}

	protected void ConfigureLoadoutPanelButton(Widget root, string buttonName, string backgroundName, string accentName, string labelName, string label, int userId)
	{
		if (!root || userId <= 0)
			return;

		Widget button = root.FindAnyWidget(buttonName);
		if (!button)
			return;

		button.SetVisible(true);
		button.SetOpacity(1.0);
		button.SetColorInt(0xEE11171B);
		button.SetUserID(userId);
		button.AddHandler(m_WidgetHandler);

		if (!backgroundName.IsEmpty())
			SetLoadoutWidgetColor(root, backgroundName, 0xEE11171B, 0.98);
		if (!accentName.IsEmpty())
			SetLoadoutWidgetColor(root, accentName, GetAccentColor(), 1.0);
		if (!labelName.IsEmpty())
			SetLoadoutText(root, labelName, label, 0xFFF4EBD6, m_Layout.m_iFontSmall, true, false);
	}

	protected void RenderLeftButtons(WorkspaceWidget workspace, Widget root)
	{
		if (!workspace || !root || !m_Layout)
			return;

		Widget leftButtons = ResolveLoadoutRegion(root, "LeftButtons");
		if (!leftButtons)
			return;

		bool canBack = CanNavigateBack();
		BindLoadoutClick(leftButtons, "LoadoutBackButton", BACK_WIDGET_ID);
		BindLoadoutClick(leftButtons, "LoadoutCloseButton", CLOSE_WIDGET_ID);
		float backOpacity = 0.45;
		float backAccentOpacity = 0.35;
		int backAccentColor = 0x884B5960;
		int backTextColor = 0x889EA7AE;
		if (canBack)
		{
			backOpacity = 0.98;
			backAccentOpacity = 1.0;
			backAccentColor = GetAccentColor();
			backTextColor = 0xFFF4EBD6;
		}

		SetLoadoutWidgetColor(leftButtons, "BackBackground", 0xEE11171B, backOpacity);
		SetLoadoutWidgetColor(leftButtons, "BackAccent", backAccentColor, backAccentOpacity);
		SetLoadoutText(leftButtons, "BackLabel", "Back", backTextColor, m_Layout.m_iFontSmall, true, false);
		SetLoadoutWidgetColor(leftButtons, "CloseBackground", 0xEE11171B, 0.98);
		SetLoadoutWidgetColor(leftButtons, "CloseAccent", GetAccentColor(), 1.0);
		SetLoadoutText(leftButtons, "CloseLabel", "ESC", 0xFFF4EBD6, m_Layout.m_iFontSmall, true, false);
	}

	protected void BindLoadoutClick(Widget root, string widgetName, int userId)
	{
		if (!root || userId <= 0)
			return;

		Widget widget = root.FindAnyWidget(widgetName);
		if (!widget)
			return;

		widget.SetUserID(userId);
		widget.AddHandler(m_WidgetHandler);
	}

	protected void SetLoadoutWidgetColor(Widget root, string widgetName, int color, float opacity)
	{
		if (!root)
			return;

		Widget widget = root.FindAnyWidget(widgetName);
		if (!widget)
			return;

		widget.SetVisible(true);
		widget.SetOpacity(opacity);
		widget.SetColorInt(color);
	}

	protected void SetLoadoutWidgetVisible(Widget root, string widgetName, bool visible)
	{
		if (!root)
			return;

		Widget widget = root.FindAnyWidget(widgetName);
		if (widget)
			widget.SetVisible(visible);
	}

	protected bool SetLoadoutImageTexture(Widget root, string widgetName, ResourceName texture, int color)
	{
		if (!root || texture.IsEmpty())
			return false;

		ImageWidget imageWidget = ImageWidget.Cast(root.FindAnyWidget(widgetName));
		if (!imageWidget)
			return false;

		if (!imageWidget.LoadImageTexture(0, texture))
			return false;

		imageWidget.SetImage(0);
		imageWidget.SetVisible(true);
		imageWidget.SetOpacity(1.0);
		imageWidget.SetColorInt(color);
		return true;
	}

	protected void SetLoadoutText(Widget root, string widgetName, string text, int color, int fontSize, bool bold, bool wrap)
	{
		if (!root)
			return;

		TextWidget textWidget = TextWidget.Cast(root.FindAnyWidget(widgetName));
		if (!textWidget)
			return;

		textWidget.SetVisible(true);
		textWidget.SetOpacity(1.0);
		textWidget.SetText(text);
		textWidget.SetTextWrapping(wrap);
		textWidget.SetExactFontSize(fontSize);
		textWidget.SetBold(bold);
		textWidget.SetOutline(1, 0xDD000000);
		textWidget.SetShadow(2, 0xEE000000, 1, 1, 1);
		textWidget.SetColorInt(color);
	}

	protected void GetRegionLayoutSize(WorkspaceWidget workspace, Widget region, int fallbackWidth, int fallbackHeight, out int width, out int height)
	{
		width = fallbackWidth;
		height = fallbackHeight;
		if (!workspace || !region)
			return;

		float rawWidth;
		float rawHeight;
		region.GetScreenSize(rawWidth, rawHeight);
		int layoutWidth = HST_UIWorkspaceMetrics.RawToLayoutPx(workspace, rawWidth);
		int layoutHeight = HST_UIWorkspaceMetrics.RawToLayoutPx(workspace, rawHeight);
		if (layoutWidth > 0)
			width = layoutWidth;
		if (layoutHeight > 0)
			height = layoutHeight;
	}

	protected void DeleteEditorRoot()
	{
		if (m_RootWidget)
			m_RootWidget.RemoveFromHierarchy();

		m_RootWidget = null;
		m_UILayerWidget = null;
		m_bRootFromLayout = false;
		m_bPostLayoutRefreshQueued = false;
		m_PreviewWidget = null;
	}

	protected void QueueLoadoutPostLayoutRefresh()
	{
		if (!m_RootWidget || m_bPostLayoutRefreshQueued)
			return;

		m_bPostLayoutRefreshQueued = true;
		GetGame().GetCallqueue().CallLater(RefreshLoadoutPostLayout, 0, false);
		GetGame().GetCallqueue().CallLater(RefreshLoadoutPostLayout, 50, false);
	}

	protected void RefreshLoadoutPostLayout()
	{
		m_bPostLayoutRefreshQueued = false;
		if (!m_bEditorOpen || !m_RootWidget || !m_RootWidget.IsVisibleInHierarchy())
			return;

		m_RootWidget.SetVisible(true);
		m_RootWidget.SetOpacity(1.0);
		m_RootWidget.SetZOrder(HST_UIConstants.Z_LOADOUT_EDITOR);

		Widget previewContainer = m_RootWidget.FindAnyWidget("HST_LoadoutPreviewContainer");
		if (previewContainer)
		{
			previewContainer.SetVisible(true);
			previewContainer.SetOpacity(1.0);
			previewContainer.SetZOrder(LOADOUT_PREVIEW_Z);
		}

		if (m_PreviewWidget)
			m_PreviewWidget.SetZOrder(LOADOUT_PREVIEW_Z);

		Widget uiRoot = ResolveUILayer(m_RootWidget);
		if (uiRoot)
		{
			uiRoot.SetVisible(true);
			uiRoot.SetOpacity(1.0);
			uiRoot.SetZOrder(LOADOUT_UI_LAYER_Z);
		}

		ApplyLoadoutLayerOrder(m_RootWidget);
		HST_UIDebug.LogWidgetGeometryCsv("loadout_editor_ready", m_RootWidget, "HST_LoadoutEditorRoot|HST_LoadoutPreviewContainer|HST_LoadoutPreview|HST_LoadoutDimmer|HST_LoadoutUILayer|PreviewDragSurface|TopTabs|TopTabItems|LeftButtons|LeftRail|SlotRailScroll|SlotRailItems|CandidateList|CandidateItems|StorageBrowser|StorageCandidateItems|SavePanel|TemplateItems|SettingsContent|Footer|Toast|ToastText|PreviewUnavailableText");
		HST_UIDebug.LogReadyWidgetsCsv("loadout_editor_ready", m_RootWidget, "HST_LoadoutEditorRoot|HST_LoadoutPreviewContainer|HST_LoadoutPreview|HST_LoadoutUILayer|PreviewDragSurface|TopTabs|TopTabItems|LeftButtons|LoadoutBackButton|LoadoutCloseButton|LeftRail|SlotRailScroll|Footer");
		HST_UIDebug.LogPopulation("loadout_editor_ready", string.Format("root=%1 previewContainer=%2 preview=%3 ui=%4 tabs=%5 rail=%6 footer=%7 drag=%8", HST_UIDebug.WidgetSummary(m_RootWidget), HST_UIDebug.WidgetSummary(m_RootWidget.FindAnyWidget("HST_LoadoutPreviewContainer")), HST_UIDebug.WidgetSummary(m_PreviewWidget), HST_UIDebug.WidgetSummary(uiRoot), HST_UIDebug.WidgetSummary(m_RootWidget.FindAnyWidget("TopTabs")), HST_UIDebug.WidgetSummary(m_RootWidget.FindAnyWidget("LeftRail")), HST_UIDebug.WidgetSummary(m_RootWidget.FindAnyWidget("Footer")), HST_UIDebug.WidgetSummary(m_RootWidget.FindAnyWidget("PreviewDragSurface"))));
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

	protected int ScalePx(float value)
	{
		if (!m_Layout)
			return Math.Round(value);

		return Math.Round(value * m_Layout.m_fScale);
	}

	protected int ScaleFont(float value)
	{
		if (!m_Layout)
			return Math.Round(value);

		int scaled = Math.Round(value * m_Layout.m_fScale);
		return ClampInt(scaled, 8, Math.Round(value * 1.15));
	}

	protected int ClampInt(int value, int minValue, int maxValue)
	{
		if (maxValue < minValue)
			return Math.Max(1, maxValue);

		if (value < minValue)
			return minValue;

		if (value > maxValue)
			return maxValue;

		return value;
	}

	protected void BuildResponsiveLayout()
	{
		if (!m_Layout)
			m_Layout = new HST_LoadoutEditorLayoutMetrics();

		int w = Math.Max(1, m_iEditorWidth);
		int h = Math.Max(1, m_iEditorHeight);

		float sx = w / 1920.0;
		float sy = h / 1080.0;
		float scale = Math.Min(sx, sy);
		scale = Math.Clamp(scale, 0.68, 1.15);

		m_Layout.m_fScale = scale;
		m_Layout.m_bCompact = w < 1550 || h < 880;
		m_Layout.m_bVeryCompact = w < 1300 || h < 760;

		// Fallbacks are layout coordinates; live named regions replace these through GetRegionLayoutSize.
		m_Layout.m_iRailWidth = LOADOUT_LAYOUT_FALLBACK_RAIL_WIDTH;
		m_Layout.m_iRailHeight = LOADOUT_LAYOUT_FALLBACK_RAIL_HEIGHT;
		m_Layout.m_iMainWidth = LOADOUT_LAYOUT_FALLBACK_MAIN_WIDTH;
		m_Layout.m_iMainHeight = LOADOUT_LAYOUT_FALLBACK_MAIN_HEIGHT;

		m_Layout.m_iFontTiny = ScaleFont(10);
		m_Layout.m_iFontSmall = ScaleFont(12);
		m_Layout.m_iFontNormal = ScaleFont(14);
		m_Layout.m_iFontTitle = ScaleFont(17);
	}

	protected void RenderModeTabs(WorkspaceWidget workspace, Widget root)
	{
		if (!workspace || !root || !m_Layout)
			return;

		Widget target = ResolveLoadoutRegion(root, "TopTabs");
		if (!target)
			return;

		SetLoadoutWidgetColor(target, "TopTabsBackground", GetTabBackColor(), 1.0);

		Widget items = target.FindAnyWidget("TopTabItems");
		if (!items)
			return;

		int activeIndex = GetEditorModeIndex(m_sEditorMode);
		int previousIndex = activeIndex - 1;
		if (previousIndex < 0)
			previousIndex = GetEditorModeCount() - 1;

		AddLoadoutTabButton(workspace, items, "Q", "", MODE_WIDGET_ID_BASE + previousIndex, false);
		for (int i = 0; i < GetEditorModeCount(); i++)
		{
			string modeId = GetEditorModeId(i);
			bool active = modeId == m_sEditorMode;
			AddLoadoutTabButton(workspace, items, GetEditorModeIcon(modeId), ResolveIconTexture(modeId), MODE_WIDGET_ID_BASE + i, active);
		}

		int nextIndex = activeIndex + 1;
		if (nextIndex >= GetEditorModeCount())
			nextIndex = 0;

		AddLoadoutTabButton(workspace, items, "E", "", MODE_WIDGET_ID_BASE + nextIndex, false);
		HST_UIDebug.LogRowSummary("loadout_mode_tabs", LOADOUT_TAB_BUTTON_LAYOUT, GetEditorModeCount() + 2, string.Format("activeMode=%1 activeIndex=%2", m_sEditorMode, activeIndex));
	}

	protected void AddLoadoutTabButton(WorkspaceWidget workspace, Widget parent, string fallback, ResourceName texture, int userId, bool active)
	{
		if (!workspace || !parent)
			return;

		Widget tab = workspace.CreateWidgets(LOADOUT_TAB_BUTTON_LAYOUT, parent);
		if (!tab)
			return;

		m_aWidgets.Insert(tab);
		tab.SetUserID(userId);
		tab.AddHandler(m_WidgetHandler);

		int background = 0xFF12171B;
		int accent = 0x884B5960;
		int foreground = 0xFFF4EBD6;
		float accentOpacity = 0.35;
		if (active)
		{
			background = GetAccentColor();
			accent = GetAccentColor();
			foreground = 0xFFFFFFFF;
			accentOpacity = 1.0;
		}

		SetLoadoutWidgetColor(tab, "Background", background, 0.98);
		SetLoadoutWidgetColor(tab, "Accent", accent, accentOpacity);
		if (SetLoadoutImageTexture(tab, "Icon", texture, foreground))
		{
			SetLoadoutWidgetVisible(tab, "Fallback", false);
			HST_UIDebug.LogRowSample("loadout_mode_tabs", LOADOUT_TAB_BUTTON_LAYOUT, Math.Max(0, userId - MODE_WIDGET_ID_BASE), string.Format("userId=%1 active=%2 fallback=%3 texture=%4 image=true row=%5", userId, active, fallback, texture, HST_UIDebug.WidgetSummary(tab)));
			return;
		}

		SetLoadoutWidgetVisible(tab, "Icon", false);
		SetLoadoutText(tab, "Fallback", fallback, foreground, ScaleFont(22), true, false);
		HST_UIDebug.LogRowSample("loadout_mode_tabs", LOADOUT_TAB_BUTTON_LAYOUT, Math.Max(0, userId - MODE_WIDGET_ID_BASE), string.Format("userId=%1 active=%2 fallback=%3 texture=%4 image=false row=%5", userId, active, fallback, texture, HST_UIDebug.WidgetSummary(tab)));
	}

	protected void RenderSlotRail(WorkspaceWidget workspace, Widget root)
	{
		if (m_bCandidateMode || m_sEditorMode == "settings" || m_sEditorMode == "save")
			return;
		if (!m_Layout)
			return;

		Widget railRoot = ResolveLoadoutRegion(root, "LeftRail");
		int railWidth;
		int railHeight;
		GetRegionLayoutSize(workspace, railRoot, m_Layout.m_iRailWidth, m_Layout.m_iRailHeight, railWidth, railHeight);
		m_Layout.m_iRailWidth = railWidth;
		m_Layout.m_iRailHeight = railHeight;
		SetLoadoutWidgetColor(railRoot, "LeftRailBackground", GetPanelBackColor(), 1.0);
		SetLoadoutWidgetColor(railRoot, "LeftRailAccent", GetAccentColor(), 1.0);
		ShowSlotRailWidgets(railRoot);

		m_aVisibleNodeIndexes.Clear();
		m_SlotScroll = ScrollLayoutWidget.Cast(railRoot.FindAnyWidget("SlotRailScroll"));
		Widget items = railRoot.FindAnyWidget("SlotRailItems");

		if (!items)
		{
			SetLoadoutText(railRoot, "SlotRailEmpty", "Slot list unavailable: layout missing SlotRailItems.", 0xFFFFD166, m_Layout.m_iFontSmall, false, true);
			return;
		}

		int visibleIndex = 0;
		for (int nodeIndex = 0; nodeIndex < m_aNodeIds.Count(); nodeIndex++)
		{
			if (!IsNodeVisibleInCurrentMode(nodeIndex))
				continue;

			m_aVisibleNodeIndexes.Insert(nodeIndex);
			AddLoadoutNodeRow(workspace, items, nodeIndex, NODE_WIDGET_ID_BASE + visibleIndex);
			visibleIndex++;
		}

		if (visibleIndex == 0)
		{
			Widget row = workspace.CreateWidgets(LOADOUT_NODE_ROW_LAYOUT, items);
			DebugRowCreated("LOADOUT_NODE_ROW_LAYOUT", row);
			HST_UIDebug.LogRowSample("loadout_slot_rows", LOADOUT_NODE_ROW_LAYOUT, 0, string.Format("empty=true reason=no visible nodes mode=%1 category=%2 row=%3", m_sEditorMode, m_sSelectedCategory, HST_UIDebug.WidgetSummary(row)));
			if (row)
			{
				PrepareRowRoot(row);
				SetRowText(row, "Primary", "Empty Slot", 0xFFC8D0D4, m_Layout.m_iFontNormal, false, true);
				SetRowText(row, "Secondary", "", 0xFFC8D0D4, m_Layout.m_iFontSmall, false, true);
				SetRowText(row, "OpenMarker", "", 0xFFFFFFFF, m_Layout.m_iFontTitle, true, false);
				SetRowImageColor(row, "Background", 0x00111111, 0.01);
			}
		}

		HST_UIDebug.LogRowSummary("loadout_slot_rows", LOADOUT_NODE_ROW_LAYOUT, Math.Max(1, visibleIndex), string.Format("mode=%1 category=%2 selectedNode=%3 visible=%4 totalNodes=%5", m_sEditorMode, m_sSelectedCategory, ShortenText(m_sSelectedNodeId, 48), visibleIndex, m_aNodeIds.Count()));
		RestoreScrollPixels(m_SlotScroll, m_fSlotScrollY);
	}

	protected void RenderStorageRail(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		EnsureSelectedStorageNode();
		string selectedStorageNodeId = ResolveSelectedStorageContainerNodeId();

		Widget railRoot = ResolveLoadoutRegion(root, "LeftRail");
		int railWidth;
		int railHeight;
		GetRegionLayoutSize(workspace, railRoot, m_Layout.m_iRailWidth, m_Layout.m_iRailHeight, railWidth, railHeight);
		m_Layout.m_iRailWidth = railWidth;
		m_Layout.m_iRailHeight = railHeight;

		SetLoadoutWidgetColor(railRoot, "LeftRailBackground", GetPanelBackColor(), 1.0);
		SetLoadoutWidgetColor(railRoot, "LeftRailAccent", GetAccentColor(), 1.0);
		ShowStorageRailWidgets(railRoot);
		SetLoadoutText(railRoot, "StorageRailTitle", "Inventory Storage", 0xFFEFE2C4, m_Layout.m_iFontTitle, true, false);
		SetLoadoutWidgetColor(railRoot, "StorageContentRule", GetAccentColor(), 1.0);
		SetLoadoutText(railRoot, "StorageContentTitle", "Contents", 0xFFFFD166, m_Layout.m_iFontSmall, true, false);
		m_aVisibleNodeIndexes.Clear();

		m_StorageContainerScroll = ScrollLayoutWidget.Cast(railRoot.FindAnyWidget("StorageContainerScroll"));
		Widget containerItems = railRoot.FindAnyWidget("StorageContainerItems");

		int visibleIndex = 0;
		for (int nodeIndex = 0; nodeIndex < m_aNodeKinds.Count(); nodeIndex++)
		{
			if (m_aNodeKinds[nodeIndex] != "storage")
				continue;

			m_aVisibleNodeIndexes.Insert(nodeIndex);
			AddLoadoutStorageContainerRow(workspace, containerItems, nodeIndex, NODE_WIDGET_ID_BASE + visibleIndex);
			visibleIndex++;
		}

		if (!containerItems)
			SetLoadoutText(railRoot, "StorageContainerEmpty", "Storage list unavailable: layout missing StorageContainerItems.", 0xFFFFD166, m_Layout.m_iFontSmall, false, true);
		else if (visibleIndex == 0)
		{
			Widget row = workspace.CreateWidgets(LOADOUT_STORAGE_ROW_LAYOUT, containerItems);
			DebugRowCreated("LOADOUT_STORAGE_ROW_LAYOUT", row);
			HST_UIDebug.LogRowSample("loadout_storage_containers", LOADOUT_STORAGE_ROW_LAYOUT, 0, string.Format("empty=true reason=no equipped storage row=%1", HST_UIDebug.WidgetSummary(row)));
			if (row)
			{
				PrepareRowRoot(row);
				SetRowText(row, "Primary", "No equipped storage.", 0xFFB7C7D7, m_Layout.m_iFontNormal, false, true);
				SetRowText(row, "Secondary", "", 0xFFB7C7D7, m_Layout.m_iFontSmall, false, true);
				SetRowText(row, "Meta", "", 0xFFFFD166, m_Layout.m_iFontTiny, false, false);
				SetRowImageColor(row, "Background", 0x00111111, 0.01);
				SetRowImageColor(row, "VolumeBack", 0x00111111, 0.01);
				SetRowImageColor(row, "VolumeFill", 0x00111111, 0.01);
			}
		}

		m_StorageContentScroll = ScrollLayoutWidget.Cast(railRoot.FindAnyWidget("StorageContentScroll"));
		Widget contentItems = railRoot.FindAnyWidget("StorageContentItems");

		int contentRows = 0;
		for (int nodeIndex = 0; nodeIndex < m_aNodeKinds.Count(); nodeIndex++)
		{
			if (m_aNodeKinds[nodeIndex] != "storage_item")
				continue;
			if (nodeIndex >= m_aNodeParents.Count() || m_aNodeParents[nodeIndex] != selectedStorageNodeId)
				continue;

			m_aVisibleNodeIndexes.Insert(nodeIndex);
			AddLoadoutStorageItemRow(workspace, contentItems, nodeIndex, NODE_WIDGET_ID_BASE + m_aVisibleNodeIndexes.Count() - 1);
			contentRows++;
		}

		if (!contentItems)
			SetLoadoutText(railRoot, "StorageContentEmpty", "Contents unavailable: layout missing StorageContentItems.", 0xFFFFD166, m_Layout.m_iFontSmall, false, true);
		else if (contentRows == 0)
		{
			Widget row = workspace.CreateWidgets(LOADOUT_STORAGE_ITEM_ROW_LAYOUT, contentItems);
			DebugRowCreated("LOADOUT_STORAGE_ITEM_ROW_LAYOUT", row);
			HST_UIDebug.LogRowSample("loadout_storage_items", LOADOUT_STORAGE_ITEM_ROW_LAYOUT, 0, string.Format("empty=true selectedStorage=%1 row=%2", ShortenText(selectedStorageNodeId, 48), HST_UIDebug.WidgetSummary(row)));
			if (row)
			{
				PrepareRowRoot(row);
				SetRowText(row, "Name", "Empty", 0xFFB7C7D7, m_Layout.m_iFontNormal, false, true);
				SetRowText(row, "Count", "", 0xFFB7C7D7, m_Layout.m_iFontTiny, false, false);
				SetRowImageColor(row, "Background", 0x00111111, 0.01);
			}
		}

		HST_UIDebug.LogRowSummary("loadout_storage_containers", LOADOUT_STORAGE_ROW_LAYOUT, Math.Max(1, visibleIndex), string.Format("selectedStorage=%1 storageRows=%2 totalNodes=%3", ShortenText(selectedStorageNodeId, 48), visibleIndex, m_aNodeIds.Count()));
		HST_UIDebug.LogRowSummary("loadout_storage_items", LOADOUT_STORAGE_ITEM_ROW_LAYOUT, Math.Max(1, contentRows), string.Format("selectedStorage=%1 contentRows=%2 visibleNodes=%3", ShortenText(selectedStorageNodeId, 48), contentRows, m_aVisibleNodeIndexes.Count()));
		RestoreScrollPixels(m_StorageContainerScroll, m_fStorageContainerScrollY);
		RestoreScrollPixels(m_StorageContentScroll, m_fStorageContentScrollY);
	}

	protected void RenderStorageCandidatePanel(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		EnsureSelectedStorageNode();
		string selectedStorageNodeId = ResolveSelectedStorageContainerNodeId();
		Widget panelRoot = ResolveLoadoutRegion(root, "StorageBrowser");
		int panelWidth;
		int panelHeight;
		GetRegionLayoutSize(workspace, panelRoot, m_Layout.m_iMainWidth, m_Layout.m_iMainHeight, panelWidth, panelHeight);
		m_Layout.m_iMainWidth = panelWidth;
		m_Layout.m_iMainHeight = panelHeight;

		SetLoadoutWidgetColor(panelRoot, "StorageBrowserBackground", GetPanelBackColor(), 1.0);
		SetLoadoutWidgetColor(panelRoot, "StorageBrowserAccent", GetAccentColor(), 1.0);
		SetLoadoutText(panelRoot, "StorageBrowserTitle", "Add Items", 0xFFEFE2C4, m_Layout.m_iFontTitle, true, false);
		SetLoadoutText(panelRoot, "StorageBrowserSubtitle", ShortenText(BuildStorageTargetLabel(), 96), 0xFFE2E6E8, m_Layout.m_iFontNormal, false, false);
		SetLoadoutWidgetVisible(panelRoot, "StorageCandidateEmpty", false);

		Widget categoryRoot = panelRoot.FindAnyWidget("StorageCategoryTabs");
		RenderStorageCategoryTabs(workspace, categoryRoot);

		m_aVisibleCandidateIndexes.Clear();
		if (selectedStorageNodeId.IsEmpty())
		{
			SetLoadoutText(panelRoot, "StorageCandidateEmpty", "Select equipped storage on the left.", 0xFFB7C7D7, m_Layout.m_iFontNormal, false, true);
			return;
		}

		RenderStorageCandidateGrid(workspace, panelRoot);
	}

	protected void RenderStorageCandidateGrid(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout || !root)
			return;

		m_aVisibleCandidateIndexes.Clear();

		m_StorageCandidateScroll = ScrollLayoutWidget.Cast(root.FindAnyWidget("StorageCandidateScroll"));
		Widget items = root.FindAnyWidget("StorageCandidateItems");

		if (!items)
		{
			SetLoadoutText(root, "StorageCandidateEmpty", "Candidate grid unavailable: layout missing StorageCandidateItems.", 0xFFFFD166, m_Layout.m_iFontSmall, false, true);
			return;
		}

		int visibleIndex = 0;

		for (int candidateIndex = 0; candidateIndex < m_aCandidatePrefabs.Count(); candidateIndex++)
		{
			if (!IsCandidateVisibleForStorageBrowser(candidateIndex))
				continue;

			m_aVisibleCandidateIndexes.Insert(candidateIndex);
			AddLoadoutCandidateTile(workspace, items, candidateIndex, CANDIDATE_WIDGET_ID_BASE + visibleIndex);
			visibleIndex++;
		}

		if (visibleIndex == 0)
		{
			string targetNodeId = ResolveSelectedStorageContainerNodeId();
			string emptyText = ResolveCandidateEmptyText(targetNodeId, "No compatible items in this category.");
			if (FindStringIndex(m_aRequestedCandidateNodeIds, targetNodeId) >= 0)
				emptyText = "Loading compatible items...";

			Widget row = workspace.CreateWidgets(LOADOUT_CANDIDATE_TILE_LAYOUT, items);
			DebugRowCreated("LOADOUT_CANDIDATE_TILE_LAYOUT", row);
			HST_UIDebug.LogRowSample("loadout_storage_candidates", LOADOUT_CANDIDATE_TILE_LAYOUT, 0, string.Format("empty=true target=%1 category=%2 text=%3 row=%4", ShortenText(targetNodeId, 48), m_sSelectedCategory, ShortenText(emptyText, 96), HST_UIDebug.WidgetSummary(row)));
			if (row)
			{
				PrepareRowRoot(row);
				ConfigureStorageCandidateTile(row);
				SetRowImageColor(row, "Background", 0x00111111, 0.01);
				SetRowWidgetVisible(row, "Name", false);
				SetRowWidgetVisible(row, "Count", false);
				SetRowWidgetVisible(row, "PreviewBack", false);
				SetRowWidgetVisible(row, "PreviewLine", false);
				SetRowWidgetVisible(row, "PreviewAnchor", false);
				SetRowText(row, "EmptyText", emptyText, 0xFFB7C7D7, m_Layout.m_iFontNormal, false, true);
			}
		}

		HST_UIDebug.LogRowSummary("loadout_storage_candidates", LOADOUT_CANDIDATE_TILE_LAYOUT, Math.Max(1, visibleIndex), string.Format("target=%1 category=%2 visible=%3 totalCandidates=%4", ShortenText(ResolveSelectedStorageContainerNodeId(), 48), m_sSelectedCategory, visibleIndex, m_aCandidatePrefabs.Count()));
		RestoreScrollPixels(m_StorageCandidateScroll, m_fStorageCandidateScrollY);
	}

	protected void AddLoadoutNodeRow(WorkspaceWidget workspace, Widget items, int nodeIndex, int userId)
	{
		if (!workspace || !items)
			return;

		Widget row = workspace.CreateWidgets(LOADOUT_NODE_ROW_LAYOUT, items);
		DebugRowCreated("LOADOUT_NODE_ROW_LAYOUT", row);
		DebugLoadoutNodeRow("loadout_slot_rows", LOADOUT_NODE_ROW_LAYOUT, Math.Max(0, userId - NODE_WIDGET_ID_BASE), row, nodeIndex, userId, "slot");
		if (!row)
			return;

		PrepareRowRoot(row);
		BindRowClick(row, userId);

		int color = GetBaseRowColor();
		if (nodeIndex >= 0 && nodeIndex < m_aNodeIds.Count() && m_aNodeIds[nodeIndex] == m_sSelectedNodeId)
			color = GetSelectedRowColor();

		SetRowImageColor(row, "Background", color, 0.98);

		string primaryText = GetVisibleNodePrimaryText(nodeIndex);
		string secondaryText = GetNodeDisplay(nodeIndex);
		if (IsDuplicateDisplayText(primaryText, secondaryText))
			secondaryText = "";
		if (secondaryText.IsEmpty())
			secondaryText = "Empty Slot";

		bool canOpen = nodeIndex >= 0 && nodeIndex < m_aNodeCanOpen.Count() && m_aNodeCanOpen[nodeIndex];
		AddNodePreviewToRow(workspace, row, nodeIndex, userId);
		AddNodeOverlayText(workspace, row, primaryText, secondaryText, canOpen, userId);
	}

	protected void AddLoadoutStorageContainerRow(WorkspaceWidget workspace, Widget items, int nodeIndex, int userId)
	{
		if (!workspace || !items)
			return;

		Widget row = workspace.CreateWidgets(LOADOUT_STORAGE_ROW_LAYOUT, items);
		DebugRowCreated("LOADOUT_STORAGE_ROW_LAYOUT", row);
		DebugLoadoutNodeRow("loadout_storage_containers", LOADOUT_STORAGE_ROW_LAYOUT, Math.Max(0, userId - NODE_WIDGET_ID_BASE), row, nodeIndex, userId, "storage_container");
		if (!row)
			return;

		PrepareRowRoot(row);
		BindRowClick(row, userId);

		int color = GetBaseRowColor();
		if (nodeIndex >= 0 && nodeIndex < m_aNodeIds.Count() && m_aNodeIds[nodeIndex] == m_sSelectedStorageContainerNodeId)
			color = GetSelectedRowColor();

		SetRowImageColor(row, "Background", color, 0.98);

		string primaryText = GetNodeLabel(nodeIndex);
		string secondaryText = GetNodeDisplay(nodeIndex);
		if (IsDuplicateDisplayText(primaryText, secondaryText))
			secondaryText = "";

		string metaText = BuildStorageCapacityLabel(nodeIndex);

		SetRowImageColor(row, "VolumeBack", GetVolumeTrackColor(), 1.0);
		SetStorageVolumeFill(row, nodeIndex);

		AddNodePreviewToRow(workspace, row, nodeIndex, userId);
		AddStorageContainerOverlayText(workspace, row, primaryText, secondaryText, metaText, userId);
	}

	protected void AddLoadoutStorageItemRow(WorkspaceWidget workspace, Widget items, int nodeIndex, int userId)
	{
		if (!workspace || !items)
			return;

		Widget row = workspace.CreateWidgets(LOADOUT_STORAGE_ITEM_ROW_LAYOUT, items);
		DebugRowCreated("LOADOUT_STORAGE_ITEM_ROW_LAYOUT", row);
		DebugLoadoutNodeRow("loadout_storage_items", LOADOUT_STORAGE_ITEM_ROW_LAYOUT, Math.Max(0, userId - NODE_WIDGET_ID_BASE), row, nodeIndex, userId, "storage_item");
		if (!row)
			return;

		PrepareRowRoot(row);
		BindRowClick(row, userId);
		string countText = BuildNodeCountBadge(nodeIndex, true);
		int color = GetBaseRowColor();
		if (nodeIndex >= 0 && nodeIndex < m_aNodeIds.Count() && m_aNodeIds[nodeIndex] == m_sSelectedStoredItemNodeId)
			color = GetSelectedRowColor();
		SetRowImageColor(row, "Background", color, 0.98);
		AddNodePreviewToRow(workspace, row, nodeIndex, userId);
		AddStorageItemOverlayText(workspace, row, GetNodeDisplay(nodeIndex), countText, userId);
	}

	protected void AddStorageContainerOverlayText(WorkspaceWidget workspace, Widget row, string primary, string secondary, string meta, int userId)
	{
		if (!workspace || !row || !m_Layout)
			return;

		SetRowTextOrHide(row, "Primary", ShortenText(primary, 36), 0xFFEFE2C4, m_Layout.m_iFontNormal, true, false);
		SetRowTextOrHide(row, "Secondary", ShortenText(secondary, 42), 0xFFD5D8D9, m_Layout.m_iFontSmall, false, false);
		SetRowTextOrHide(row, "Meta", ShortenText(meta, 34), 0xFFFFD166, m_Layout.m_iFontSmall, false, false);
	}

	protected void AddStorageItemOverlayText(WorkspaceWidget workspace, Widget row, string name, string countText, int userId)
	{
		if (!workspace || !row || !m_Layout)
			return;

		SetRowTextOrHide(row, "Name", ShortenText(name, 38), 0xFFE2E6E8, m_Layout.m_iFontNormal, false, true);
		SetRowTextOrHide(row, "Count", countText, 0xFFFFD166, m_Layout.m_iFontSmall, true, false);
	}

	protected string GetVisibleNodePrimaryText(int nodeIndex)
	{
		if (IsAttachmentNodeIndex(nodeIndex))
			return ResolveAttachmentNodeLabel(nodeIndex);

		string slotLabel = ResolveNodeSlotLabel(nodeIndex);
		if (!slotLabel.IsEmpty())
			return slotLabel;

		return GetNodeLabel(nodeIndex);
	}

	protected void AddNodeOverlayText(WorkspaceWidget workspace, Widget row, string primary, string secondary, bool canOpen, int userId)
	{
		if (!workspace || !row || !m_Layout)
			return;

		SetRowTextOrHide(row, "Primary", ShortenText(primary, 30), 0xFFEFE2C4, m_Layout.m_iFontNormal, true, false);
		SetRowTextOrHide(row, "Secondary", ShortenText(secondary, 46), 0xFFD5D8D9, m_Layout.m_iFontNormal, false, false);
		if (canOpen)
			SetRowTextOrHide(row, "OpenMarker", "W", 0xFFFFFFFF, m_Layout.m_iFontTitle, true, false);
		else
			SetRowWidgetVisible(row, "OpenMarker", false);
	}

	protected void AddCandidateListRow(WorkspaceWidget workspace, Widget items, int candidateIndex, int userId)
	{
		if (!workspace || !items)
			return;

		Widget row = workspace.CreateWidgets(LOADOUT_NODE_ROW_LAYOUT, items);
		DebugRowCreated("LOADOUT_NODE_ROW_LAYOUT", row);
		DebugLoadoutCandidateRow("loadout_candidate_rows", LOADOUT_NODE_ROW_LAYOUT, Math.Max(0, userId - CANDIDATE_WIDGET_ID_BASE), row, candidateIndex, userId, "candidate_list");
		if (!row)
			return;

		PrepareRowRoot(row);
		BindRowClick(row, userId);

		int color = GetBaseRowColor();
		if (IsCandidateCurrentNodeItem(candidateIndex))
			color = GetSelectedRowColor();
		else if (candidateIndex >= 0 && candidateIndex < m_aCandidateAmmoMatch.Count() && m_aCandidateAmmoMatch[candidateIndex])
			color = GetMatchedRowColor();

		SetRowImageColor(row, "Background", color, 0.98);
		AddCandidatePreviewToListRow(workspace, row, candidateIndex, userId);
		AddCandidateListOverlayText(workspace, row, GetCandidateDisplayName(candidateIndex), BuildCandidateCountLabel(candidateIndex, true), userId);
	}

	protected void AddCandidatePreviewToListRow(WorkspaceWidget workspace, Widget row, int candidateIndex, int userId)
	{
		Widget anchor = FindRowWidget(row, "PreviewAnchor");
		if (!anchor)
			return;

		anchor.SetVisible(true);
		anchor.SetOpacity(1.0);
		anchor.SetZOrder(20);

		ShowRowPreviewChrome(row);
		CreateCandidatePreviewCell(workspace, anchor, candidateIndex, userId, 0xFFE6E6E6);
	}

	protected void AddCandidateListOverlayText(WorkspaceWidget workspace, Widget row, string name, string countText, int userId)
	{
		if (!workspace || !row || !m_Layout)
			return;

		SetRowTextOrHide(row, "Primary", ShortenText(name, 44), 0xFFE2E6E8, m_Layout.m_iFontNormal, false, true);
		SetRowTextOrHide(row, "Secondary", countText, 0xFFFFD166, m_Layout.m_iFontSmall, true, false);
		SetRowWidgetVisible(row, "OpenMarker", false);
	}

	protected void AddTemplateRow(WorkspaceWidget workspace, Widget items, int templateIndex, int userId)
	{
		if (!workspace || !items)
			return;
		if (templateIndex < 0 || templateIndex >= m_aTemplateIds.Count())
			return;

		Widget row = workspace.CreateWidgets(LOADOUT_NODE_ROW_LAYOUT, items);
		DebugRowCreated("LOADOUT_NODE_ROW_LAYOUT", row);
		DebugLoadoutTemplateRow("loadout_templates", LOADOUT_NODE_ROW_LAYOUT, templateIndex, row, templateIndex, userId);
		if (!row)
			return;

		PrepareRowRoot(row);
		BindRowClick(row, userId);

		int color = GetBaseRowColor();
		bool selected = m_aTemplateIds[templateIndex] == m_sSelectedTemplateId;
		if (selected)
			color = GetSelectedRowColor();

		SetRowImageColor(row, "Background", color, 0.98);
		SetRowText(row, "Primary", ShortenText(m_aTemplateDisplays[templateIndex], 36), 0xFFE2E6E8, m_Layout.m_iFontNormal, selected, true);
		SetRowText(row, "Secondary", string.Format("%1 slots", m_aTemplateSlotCounts[templateIndex]), 0xFFB7C7D7, m_Layout.m_iFontSmall, false, false);
		SetRowText(row, "OpenMarker", "", 0xFFFFFFFF, m_Layout.m_iFontTitle, false, false);
		ShowRowPreviewChrome(row);
		SetRowWidgetVisible(row, "PreviewAnchor", false);
		SetRowText(row, "PreviewFallback", "O", 0xFFF5E8CE, ScaleFont(20), true, false);
	}

	protected void AddLoadoutCandidateTile(WorkspaceWidget workspace, Widget items, int candidateIndex, int userId)
	{
		if (!workspace || !items)
			return;

		Widget tile = workspace.CreateWidgets(LOADOUT_CANDIDATE_TILE_LAYOUT, items);
		DebugRowCreated("LOADOUT_CANDIDATE_TILE_LAYOUT", tile);
		DebugLoadoutCandidateRow("loadout_storage_candidates", LOADOUT_CANDIDATE_TILE_LAYOUT, Math.Max(0, userId - CANDIDATE_WIDGET_ID_BASE), tile, candidateIndex, userId, "storage_candidate_tile");
		if (!tile)
			return;

		PrepareRowRoot(tile);
		BindRowClick(tile, userId);
		SetRowWidgetVisible(tile, "EmptyText", false);

		ConfigureStorageCandidateTile(tile);

		int color = GetBaseRowColor();
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateAmmoMatch.Count() && m_aCandidateAmmoMatch[candidateIndex])
			color = GetMatchedRowColor();

		string countText = BuildCandidateCountLabel(candidateIndex, true);
		string itemName = GetCandidateDisplayName(candidateIndex);

		SetRowImageColor(tile, "Background", color, 0.98);
		AddCandidatePreviewToRow(workspace, tile, candidateIndex, userId);

		AddCandidateTileOverlayText(workspace, tile, itemName, countText, userId);
	}

	protected int GetStorageCandidateTileWidth()
	{
		if (!m_Layout)
			return ScalePx(354);

		int listWidth = Math.Max(ScalePx(260), m_Layout.m_iMainWidth - ScalePx(40));
		int gap = GetStorageCandidateTileGap();
		int columns = GetStorageCandidateColumnCount(listWidth);
		if (m_Layout.m_bVeryCompact)
			columns = ClampInt(columns, 1, 2);

		int tileWidth = (listWidth - ((columns - 1) * gap)) / columns;
		return Math.Max(ScalePx(220), tileWidth);
	}

	protected int GetStorageCandidateTileGap()
	{
		if (!m_Layout)
			return ScalePx(10);

		if (m_Layout.m_bVeryCompact)
			return ScalePx(8);
		if (m_Layout.m_bCompact)
			return ScalePx(8);

		return ScalePx(10);
	}

	protected int GetStorageCandidateColumnCount(int listWidth)
	{
		if (!m_Layout)
			return 3;

		if (listWidth < ScalePx(540))
			return 1;
		if (listWidth < ScalePx(760) || m_Layout.m_bVeryCompact)
			return 2;
		return 3;
	}

	protected int GetStorageCandidateTileBodyWidth()
	{
		return Math.Max(ScalePx(180), GetStorageCandidateTileWidth() - GetStorageCandidateTileGap());
	}

	protected void ConfigureStorageCandidateTile(Widget tile)
	{
		if (!tile || !m_Layout)
			return;

		int tileWidth = GetStorageCandidateTileWidth();
		int tileHeight = ScalePx(140);
		SizeLayoutWidget sizeLayout = SizeLayoutWidget.Cast(tile.FindAnyWidget("SizeLayout"));
		if (sizeLayout)
		{
			sizeLayout.EnableWidthOverride(true);
			sizeLayout.SetWidthOverride(tileWidth);
			sizeLayout.EnableHeightOverride(true);
			sizeLayout.SetHeightOverride(tileHeight);
		}
	}

	protected void AddCandidateTileOverlayText(WorkspaceWidget workspace, Widget tile, string name, string countText, int userId)
	{
		if (!workspace || !tile || !m_Layout)
			return;

		SetRowTextOrHide(tile, "Name", ShortenText(name, 68), 0xFFE2E6E8, m_Layout.m_iFontNormal, false, true);
		SetRowTextOrHide(tile, "Count", countText, 0xFFFFD166, m_Layout.m_iFontSmall, true, false);
	}

	protected void AddNodePreviewToRow(WorkspaceWidget workspace, Widget row, int nodeIndex, int userId)
	{
		Widget anchor = FindRowWidget(row, "PreviewAnchor");
		if (!anchor)
			return;

		anchor.SetVisible(true);
		anchor.SetOpacity(1.0);
		anchor.SetZOrder(20);

		ShowRowPreviewChrome(row);
		CreateNodePreviewCell(workspace, anchor, nodeIndex, userId, 0xFFE6E6E6);
	}

	protected void AddCandidatePreviewToRow(WorkspaceWidget workspace, Widget row, int candidateIndex, int userId)
	{
		Widget anchor = FindRowWidget(row, "PreviewAnchor");
		if (!anchor)
			return;

		anchor.SetVisible(true);
		anchor.SetOpacity(1.0);
		anchor.SetZOrder(20);

		ShowRowPreviewChrome(row);
		CreateCandidatePreviewCell(workspace, anchor, candidateIndex, userId, 0xFFE6E6E6);
	}

	protected void ShowRowPreviewChrome(Widget row)
	{
		SetRowImageColor(row, "PreviewBack", 0xEE05080A, 1.0);
		SetRowImageColor(row, "PreviewLine", 0x664B5960, 1.0);
		SetRowWidgetVisible(row, "PreviewFallback", false);
		SetRowWidgetVisible(row, "EmptyText", false);
	}

	protected void SetStorageVolumeFill(Widget row, int nodeIndex)
	{
		ProgressBarWidget fill = ProgressBarWidget.Cast(FindRowWidget(row, "VolumeFill"));
		if (!fill)
			return;

		Widget back = FindRowWidget(row, "VolumeBack");
		if (back)
		{
			back.SetOpacity(1.0);
			back.SetColorInt(GetVolumeTrackColor());
			back.SetVisible(true);
			back.SetZOrder(4);
		}

		float ratio = Math.Clamp(GetNodeVolumeRatio(nodeIndex), 0.0, 1.0);
		fill.SetMin(0.0);
		fill.SetMax(1.0);
		fill.SetCurrent(ratio);
		fill.SetDrawBackground(false);
		if (ratio <= 0.0)
			fill.SetOpacity(0.0);
		else
			fill.SetOpacity(0.96);
		fill.SetColorInt(ResolveStorageVolumeColor(nodeIndex));
		fill.SetVisible(true);
		fill.SetZOrder(6);
	}

	protected void RenderPreviewStage(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout || !root)
			return;

		string toast = BuildStageToast();
		bool showToast = !toast.IsEmpty();
		SetLoadoutWidgetVisible(root, "Toast", showToast);
		if (showToast)
		{
			SetLoadoutWidgetColor(root, "ToastBackground", 0xEE11171B, 0.96);
			SetLoadoutWidgetColor(root, "ToastAccent", GetAccentColor(), 1.0);
			SetLoadoutText(root, "ToastText", ShortenText(toast, 58), 0xFFEFC16D, m_Layout.m_iFontTitle, true, false);
		}

		bool previewUnavailable = !m_PreviewWidget || !m_PreviewWorld;
		SetLoadoutWidgetVisible(root, "PreviewUnavailableText", previewUnavailable);
		if (previewUnavailable)
			SetLoadoutText(root, "PreviewUnavailableText", "Preview target unavailable", 0xFFFFD166, m_Layout.m_iFontTitle, true, false);
	}

	protected string BuildStageToast()
	{
		string result = BuildFailureToastText(m_sLastResult);
		if (!result.IsEmpty())
			return result;

		result = BuildFailureToastText(m_sServerFailureText);
		if (!result.IsEmpty())
			return result;

		return BuildFailureToastText(m_sPreviewStatus);
	}

	protected string BuildFailureToastText(string rawText)
	{
		if (rawText.IsEmpty())
			return "";

		string text = rawText;
		text.Replace("h-istasi loadout editor | ", "");
		text.Replace("h-istasi arsenal | ", "");
		text = text.Trim();

		if (!IsFailureToastText(text))
			return "";

		text.Replace("failed: ", "");
		text.Replace("error: ", "");
		text = text.Trim();

		return NormalizeFailureToastText(text);
	}

	protected bool IsFailureToastText(string text)
	{
		if (text.IsEmpty())
			return false;

		string lowered = text;
		lowered.ToLower();
		if (lowered.Contains("failed") || lowered.Contains("failure") || lowered.Contains("error"))
			return true;
		if (lowered.Contains("not ready") || lowered.Contains("unavailable") || lowered.Contains("not available"))
			return true;
		if (lowered.Contains("server required") || lowered.Contains("unknown command"))
			return true;
		if (lowered.Contains("missing ") || lowered.Contains("malformed") || lowered.Contains("not found"))
			return true;
		if (lowered.Contains("no live") || lowered.Contains("no storage") || lowered.Contains("cannot") || lowered.Contains("could not"))
			return true;
		if (lowered.Contains("timed out") || lowered.Contains("denied"))
			return true;
		if (lowered.Contains("full") || lowered.Contains("empty"))
			return true;

		return false;
	}

	protected string NormalizeFailureToastText(string text)
	{
		if (text.IsEmpty())
			return "";

		string lowered = text;
		lowered.ToLower();
		if (lowered.Contains("inventory full") || lowered.Contains("no storage can accept") || lowered.Contains("failed to find suitable storage") || lowered.Contains("suitable storage"))
			return "Inventory Full";
		if (lowered.Contains("player inventory manager not available"))
			return "Inventory Unavailable";
		if (lowered.Contains("item not available in arsenal"))
			return "Item Not Available";
		if (lowered.Contains("campaign/player state not ready"))
			return "Player State Not Ready";
		if (lowered.Contains("selected loadout slot is empty"))
			return "Selected Loadout Slot Is Empty";
		if (lowered.Contains("selected storage container is empty"))
			return "Selected Storage Is Empty";
		if (lowered.Contains("no live player entity"))
			return "No Live Player Entity";

		return text;
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
		if (!m_Layout)
			return;

		Widget panelRoot = ResolveLoadoutRegion(root, "CandidateList");
		int panelWidth;
		int panelHeight;
		GetRegionLayoutSize(workspace, panelRoot, m_Layout.m_iRailWidth, m_Layout.m_iRailHeight, panelWidth, panelHeight);
		m_Layout.m_iRailWidth = panelWidth;
		m_Layout.m_iRailHeight = panelHeight;
		SetLoadoutWidgetColor(panelRoot, "CandidatePanelBackground", GetPanelBackColor(), 1.0);
		SetLoadoutWidgetColor(panelRoot, "CandidateHeaderRule", GetAccentColor(), 1.0);
		ShowCandidatePanelWidgets(panelRoot);
		RenderSelectedNodeHeader(workspace, panelRoot);
		int selectedNodeIndex = FindSelectedNodeIndex();
		if (selectedNodeIndex >= 0 && selectedNodeIndex < m_aNodeCanRemove.Count() && m_aNodeCanRemove[selectedNodeIndex])
		{
			ConfigureLoadoutPanelButton(panelRoot, "CandidateRemoveButton", "CandidateRemoveBackground", "CandidateRemoveAccent", "CandidateRemoveLabel", "Remove", REMOVE_SELECTED_NODE_WIDGET_ID);
			SetLoadoutText(panelRoot, "CandidateRemoveIcon", "X", 0xFFFFFFFF, ScaleFont(34), true, false);
		}

		m_aVisibleCandidateIndexes.Clear();
		m_CandidateScroll = ScrollLayoutWidget.Cast(panelRoot.FindAnyWidget("CandidateScroll"));
		Widget items = panelRoot.FindAnyWidget("CandidateItems");
		if (!items)
		{
			SetLoadoutText(panelRoot, "CandidateEmpty", "Candidate list unavailable: layout missing CandidateItems.", 0xFFFFD166, m_Layout.m_iFontSmall, false, true);
			return;
		}

		int visibleIndex = 0;
		for (int candidateIndex = 0; candidateIndex < m_aCandidatePrefabs.Count(); candidateIndex++)
		{
			if (!IsCandidateVisibleForSelectedNode(candidateIndex))
				continue;

			m_aVisibleCandidateIndexes.Insert(candidateIndex);
			AddCandidateListRow(workspace, items, candidateIndex, CANDIDATE_WIDGET_ID_BASE + visibleIndex);
			visibleIndex++;
		}

		if (visibleIndex == 0)
		{
			string emptyText = ResolveCandidateEmptyText(m_sSelectedNodeId, "No compatible recovered items.");
			if (FindStringIndex(m_aRequestedCandidateNodeIds, m_sSelectedNodeId) >= 0)
				emptyText = "Loading compatible items...";

			Widget row = workspace.CreateWidgets(LOADOUT_NODE_ROW_LAYOUT, items);
			DebugRowCreated("LOADOUT_NODE_ROW_LAYOUT", row);
			HST_UIDebug.LogRowSample("loadout_candidate_rows", LOADOUT_NODE_ROW_LAYOUT, 0, string.Format("empty=true selectedNode=%1 text=%2 row=%3", ShortenText(m_sSelectedNodeId, 48), ShortenText(emptyText, 96), HST_UIDebug.WidgetSummary(row)));
			if (row)
			{
				PrepareRowRoot(row);
				SetRowText(row, "Primary", emptyText, 0xFFB7C7D7, m_Layout.m_iFontNormal, false, true);
				SetRowText(row, "Secondary", "", 0xFFB7C7D7, m_Layout.m_iFontSmall, false, true);
				SetRowText(row, "OpenMarker", "", 0xFFFFFFFF, m_Layout.m_iFontTitle, true, false);
				SetRowImageColor(row, "Background", 0x00111111, 0.01);
				SetRowWidgetVisible(row, "PreviewBack", false);
				SetRowWidgetVisible(row, "PreviewLine", false);
				SetRowWidgetVisible(row, "PreviewAnchor", false);
			}
		}

		HST_UIDebug.LogRowSummary("loadout_candidate_rows", LOADOUT_NODE_ROW_LAYOUT, Math.Max(1, visibleIndex), string.Format("selectedNode=%1 visible=%2 totalCandidates=%3 requested=%4", ShortenText(m_sSelectedNodeId, 48), visibleIndex, m_aCandidatePrefabs.Count(), FindStringIndex(m_aRequestedCandidateNodeIds, m_sSelectedNodeId) >= 0));
		RestoreScrollPixels(m_CandidateScroll, m_fCandidateScrollY);
	}

	protected void RenderTemplatePanel(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		Widget panelRoot = ResolveLoadoutRegion(root, "SavePanel");
		int panelWidth;
		int panelHeight;
		GetRegionLayoutSize(workspace, panelRoot, m_Layout.m_iRailWidth, m_Layout.m_iRailHeight, panelWidth, panelHeight);
		m_Layout.m_iRailWidth = panelWidth;
		m_Layout.m_iRailHeight = panelHeight;
		ConfigureLoadoutPanelShell(panelRoot, "Save Loadout");
		ShowTemplatePanelWidgets(panelRoot);
		ConfigureLoadoutPanelButton(panelRoot, "TemplateSaveButton", "TemplateSaveBackground", "TemplateSaveAccent", "TemplateSaveLabel", "Save", SAVE_WIDGET_ID);
		ConfigureLoadoutPanelButton(panelRoot, "TemplateClearButton", "TemplateClearBackground", "TemplateClearAccent", "TemplateClearLabel", "Clear", DELETE_TEMPLATE_WIDGET_ID);

		m_TemplateScroll = ScrollLayoutWidget.Cast(panelRoot.FindAnyWidget("TemplateScroll"));
		Widget items = panelRoot.FindAnyWidget("TemplateItems");
		if (!items)
		{
			SetLoadoutText(panelRoot, "TemplateEmpty", "Template list unavailable: layout missing TemplateItems.", 0xFFFFD166, m_Layout.m_iFontSmall, false, true);
			return;
		}

		for (int templateIndex = 0; templateIndex < m_aTemplateIds.Count(); templateIndex++)
			AddTemplateRow(workspace, items, templateIndex, TEMPLATE_WIDGET_ID_BASE + templateIndex);

		if (m_aTemplateIds.Count() == 0)
		{
			Widget row = workspace.CreateWidgets(LOADOUT_NODE_ROW_LAYOUT, items);
			DebugRowCreated("LOADOUT_NODE_ROW_LAYOUT", row);
			HST_UIDebug.LogRowSample("loadout_templates", LOADOUT_NODE_ROW_LAYOUT, 0, string.Format("empty=true row=%1", HST_UIDebug.WidgetSummary(row)));
			if (row)
			{
				PrepareRowRoot(row);
				SetRowText(row, "Primary", "Empty Slot", 0xFFB7C7D7, m_Layout.m_iFontNormal, false, true);
				SetRowText(row, "Secondary", "", 0xFFB7C7D7, m_Layout.m_iFontSmall, false, true);
				SetRowText(row, "OpenMarker", "", 0xFFFFFFFF, m_Layout.m_iFontTitle, true, false);
				SetRowImageColor(row, "Background", 0x00111111, 0.01);
				SetRowWidgetVisible(row, "PreviewBack", false);
				SetRowWidgetVisible(row, "PreviewLine", false);
				SetRowWidgetVisible(row, "PreviewAnchor", false);
			}
		}

		HST_UIDebug.LogRowSummary("loadout_templates", LOADOUT_NODE_ROW_LAYOUT, Math.Max(1, m_aTemplateIds.Count()), string.Format("selectedTemplate=%1 templates=%2", ShortenText(m_sSelectedTemplateId, 48), m_aTemplateIds.Count()));
		RestoreScrollPixels(m_TemplateScroll, m_fTemplateScrollY);
	}

	protected void RenderSettingsPanel(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		EnsureVisualSettings();

		Widget panelRoot = ResolveLoadoutRegion(root, "SavePanel");
		int panelWidth;
		int panelHeight;
		GetRegionLayoutSize(workspace, panelRoot, m_Layout.m_iRailWidth, m_Layout.m_iRailHeight, panelWidth, panelHeight);
		m_Layout.m_iRailWidth = panelWidth;
		m_Layout.m_iRailHeight = panelHeight;
		ConfigureLoadoutPanelShell(panelRoot, "Settings");
		HideTemplatePanelWidgets(panelRoot);
		ShowSettingsPanelWidgets(panelRoot);

		SetLoadoutText(panelRoot, "SettingsLightLabel", "Preview Light", 0xFFFFD166, m_Layout.m_iFontNormal, true, false);
		SetLoadoutText(panelRoot, "SettingsLightValue", string.Format("%1 LV", Math.Round(m_VisualSettings.m_fPreviewLightLV)), 0xFFE2E6E8, m_Layout.m_iFontNormal, false, false);
		ConfigureLoadoutPanelButton(panelRoot, "SettingsLightMinusButton", "", "", "SettingsLightMinusLabel", "-", SETTINGS_LIGHT_MINUS_WIDGET_ID);
		ConfigureLoadoutPanelButton(panelRoot, "SettingsLightPlusButton", "", "", "SettingsLightPlusLabel", "+", SETTINGS_LIGHT_PLUS_WIDGET_ID);

		SetLoadoutText(panelRoot, "SettingsPanelPresetLabel", "Menu Panel Color", 0xFFFFD166, m_Layout.m_iFontNormal, true, false);
		SetLoadoutText(panelRoot, "SettingsPanelPresetValue", GetPresetLabel("panel", m_VisualSettings.m_iPanelPreset), 0xFFE2E6E8, m_Layout.m_iFontNormal, false, false);
		ConfigureLoadoutPanelButton(panelRoot, "SettingsPanelPresetButton", "", "", "SettingsPanelPresetButtonLabel", "Next", SETTINGS_PANEL_PRESET_WIDGET_ID);

		SetLoadoutText(panelRoot, "SettingsAccentPresetLabel", "Highlight Color", 0xFFFFD166, m_Layout.m_iFontNormal, true, false);
		SetLoadoutText(panelRoot, "SettingsAccentPresetValue", GetPresetLabel("accent", m_VisualSettings.m_iAccentPreset), 0xFFE2E6E8, m_Layout.m_iFontNormal, false, false);
		ConfigureLoadoutPanelButton(panelRoot, "SettingsAccentPresetButton", "", "", "SettingsAccentPresetButtonLabel", "Next", SETTINGS_ACCENT_PRESET_WIDGET_ID);

		SetLoadoutText(panelRoot, "SettingsRowPresetLabel", "Row Color", 0xFFFFD166, m_Layout.m_iFontNormal, true, false);
		SetLoadoutText(panelRoot, "SettingsRowPresetValue", GetPresetLabel("row", m_VisualSettings.m_iRowPreset), 0xFFE2E6E8, m_Layout.m_iFontNormal, false, false);
		ConfigureLoadoutPanelButton(panelRoot, "SettingsRowPresetButton", "", "", "SettingsRowPresetButtonLabel", "Next", SETTINGS_ROW_PRESET_WIDGET_ID);

		SetLoadoutText(panelRoot, "SettingsWorldPresetLabel", "Render Background Color", 0xFFFFD166, m_Layout.m_iFontNormal, true, false);
		SetLoadoutText(panelRoot, "SettingsWorldPresetValue", GetPresetLabel("world", m_VisualSettings.m_iWorldPreset), 0xFFE2E6E8, m_Layout.m_iFontNormal, false, false);
		ConfigureLoadoutPanelButton(panelRoot, "SettingsWorldPresetButton", "", "", "SettingsWorldPresetButtonLabel", "Next", SETTINGS_WORLD_PRESET_WIDGET_ID);

		ConfigureLoadoutPanelButton(panelRoot, "SettingsDefaultsButton", "", "", "SettingsDefaultsLabel", "Defaults", SETTINGS_RESET_WIDGET_ID);
		ConfigureLoadoutPanelButton(panelRoot, "SettingsResetPreviewButton", "", "", "SettingsResetPreviewLabel", "Reset Preview", RESET_PREVIEW_WIDGET_ID);
	}

	protected void RenderFooter(WorkspaceWidget workspace, Widget root)
	{
	}

	protected void RenderContextHints(WorkspaceWidget workspace, Widget root)
	{
		if (!workspace || !root || !m_Layout)
			return;

		Widget footerRoot = ResolveLoadoutRegion(root, "Footer");
		HideLoadoutFooterHints(footerRoot);
		SetLoadoutFooterHint(footerRoot, "FooterPrevTabHint", "FooterPrevTabKeyBack", "FooterPrevTabKey", "FooterPrevTabLabel", "Q", "Prev Tab");
		SetLoadoutFooterHint(footerRoot, "FooterNextTabHint", "FooterNextTabKeyBack", "FooterNextTabKey", "FooterNextTabLabel", "E", "Next Tab");
		string backLabel = "Close";
		if (CanNavigateBack())
			backLabel = "Back";
		SetLoadoutFooterHint(footerRoot, "FooterBackHint", "FooterBackKeyBack", "FooterBackKey", "FooterBackLabel", "ESC", backLabel);
		if (m_sEditorMode == "storage")
		{
			SetLoadoutFooterHint(footerRoot, "FooterPrimaryHint", "FooterPrimaryKeyBack", "FooterPrimaryKey", "FooterPrimaryLabel", "LMB", "Add One");
			SetLoadoutFooterHint(footerRoot, "FooterSecondaryHint", "FooterSecondaryKeyBack", "FooterSecondaryKey", "FooterSecondaryLabel", "RMB", "Remove One");
		}
		else if (m_bCandidateMode)
		{
			SetLoadoutFooterHint(footerRoot, "FooterPrimaryHint", "FooterPrimaryKeyBack", "FooterPrimaryKey", "FooterPrimaryLabel", "LMB", "Select");
		}
	}

	protected void HideLoadoutFooterHints(Widget footerRoot)
	{
		if (!footerRoot)
			return;

		SetLoadoutWidgetVisible(footerRoot, "FooterPrevTabHint", false);
		SetLoadoutWidgetVisible(footerRoot, "FooterNextTabHint", false);
		SetLoadoutWidgetVisible(footerRoot, "FooterBackHint", false);
		SetLoadoutWidgetVisible(footerRoot, "FooterPrimaryHint", false);
		SetLoadoutWidgetVisible(footerRoot, "FooterSecondaryHint", false);
	}

	protected void SetLoadoutFooterHint(Widget footerRoot, string hintName, string keyBackName, string keyName, string labelName, string keyText, string labelText)
	{
		if (!footerRoot)
			return;

		SetLoadoutWidgetVisible(footerRoot, hintName, true);
		SetLoadoutWidgetColor(footerRoot, keyBackName, 0xDD11171B, 0.98);
		SetLoadoutText(footerRoot, keyName, keyText, 0xFFF4EBD6, m_Layout.m_iFontSmall, true, false);
		SetLoadoutText(footerRoot, labelName, labelText, 0xFFE2E6E8, m_Layout.m_iFontSmall, false, false);
	}

	protected void ConfigurePreviewDragSurface(Widget root)
	{
		if (!root)
			return;

		Widget surface = root.FindAnyWidget("PreviewDragSurface");
		if (!surface)
			return;

		surface.SetVisible(true);
		surface.SetOpacity(0.01);
		surface.SetColorInt(0x00111111);
		surface.SetUserID(PREVIEW_DRAG_WIDGET_ID);
		surface.AddHandler(m_WidgetHandler);
		surface.SetZOrder(LOADOUT_PREVIEW_INPUT_Z);
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

			if (fields[0] == "FAILURE" && fields.Count() >= 2)
			{
				m_sServerFailureText = ResolvePayloadDisplayText(fields[1]);
				continue;
			}

			if (fields[0] == "DEBUG" && fields.Count() >= 2)
			{
				m_bDebugLoggingEnabled = ParsePayloadBool(fields[1], false);
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

				itemDisplay = NormalizeKnownVariantDisplay(HST_DisplayNameService.ResolveItemDisplayName(null, fields[2], itemDisplay), fields[2]);
				itemShortDisplay = NormalizeKnownVariantDisplay(HST_DisplayNameService.ResolveShortItemDisplayName(itemShortDisplay, fields[2]), fields[2]);
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

				slotDisplay = NormalizeKnownVariantDisplay(HST_DisplayNameService.ResolveItemDisplayName(null, fields[3], slotDisplay), fields[3]);
				slotShortDisplay = NormalizeKnownVariantDisplay(HST_DisplayNameService.ResolveShortItemDisplayName(slotShortDisplay, fields[3]), fields[3]);
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
					nodeDisplay = NormalizeKnownVariantDisplay(HST_DisplayNameService.ResolveItemDisplayName(null, fields[6], nodeDisplay), fields[6]);
				if (nodeDisplay.IsEmpty())
					nodeDisplay = "Empty Slot";
				m_aNodeDisplays.Insert(nodeDisplay);
				m_aNodeCounts.Insert(fields[8]);
				m_aNodeUsedVolumes.Insert(0.0);
				m_aNodeTotalVolumes.Insert(0.0);
				m_aNodeFreeVolumes.Insert(0.0);
				m_aNodeInfinite.Insert(ParsePayloadBool(fields[9], false));
				m_aNodeCanOpen.Insert(ParsePayloadBool(fields[10], false));
				m_aNodeCanRemove.Insert(ParsePayloadBool(fields[11], false));
				m_aNodeCanDeposit.Insert(ParsePayloadBool(fields[12], false));
				if (fields.Count() > 13)
					m_aNodeFocus.Insert(fields[13]);
				else
					m_aNodeFocus.Insert(fields[4]);
				if (fields.Count() > 14)
					m_aNodeUsedCapacities.Insert(Math.Max(0, fields[14].ToInt()));
				else
					m_aNodeUsedCapacities.Insert(0);
				if (fields.Count() > 15)
					m_aNodeTotalCapacities.Insert(Math.Max(0, fields[15].ToInt()));
				else
					m_aNodeTotalCapacities.Insert(0);
				continue;
			}

			if (fields[0] == "STORAGE" && fields.Count() >= 6)
			{
				int nodeIndex = FindStringIndex(m_aNodeIds, fields[1]);
				if (nodeIndex >= 0)
				{
					EnsureNodeCapacityArrays(nodeIndex);
					EnsureNodeVolumeArrays(nodeIndex);
					if (nodeIndex < m_aNodeLabels.Count())
						m_aNodeLabels[nodeIndex] = ResolvePayloadDisplayText(fields[2]);
					if (nodeIndex < m_aNodeCounts.Count())
						m_aNodeCounts[nodeIndex] = fields[3];
					m_aNodeUsedCapacities[nodeIndex] = Math.Max(0, fields[3].ToInt());
					m_aNodeTotalCapacities[nodeIndex] = Math.Max(0, fields[4].ToInt());
					if (fields.Count() > 6)
						m_aNodeUsedVolumes[nodeIndex] = Math.Max(0.0, ParsePayloadFloat(fields[6], 0.0));
					if (fields.Count() > 7)
						m_aNodeTotalVolumes[nodeIndex] = Math.Max(0.0, ParsePayloadFloat(fields[7], 0.0));
					if (fields.Count() > 8)
						m_aNodeFreeVolumes[nodeIndex] = Math.Max(0.0, ParsePayloadFloat(fields[8], 0.0));
					if (fields.Count() > 5 && nodeIndex < m_aNodeCanDeposit.Count())
						m_aNodeCanDeposit[nodeIndex] = ParsePayloadBool(fields[5], m_aNodeCanDeposit[nodeIndex]);
				}

				continue;
			}

			if (fields[0] == "CANDIDATE" && fields.Count() >= 9)
			{
				if (IsRemovedExternalPrefab(fields[2]))
					continue;

				m_aCandidateNodeIds.Insert(fields[1]);
				m_aCandidatePrefabs.Insert(fields[2]);
				string candidateDisplay = ResolvePayloadDisplayText(fields[3]);
				candidateDisplay = NormalizeKnownVariantDisplay(HST_DisplayNameService.ResolveItemDisplayName(null, fields[2], candidateDisplay), fields[2]);
				m_aCandidateDisplays.Insert(candidateDisplay);
				string candidateShort = ResolvePayloadDisplayText(fields[4]);
				candidateShort = NormalizeKnownVariantDisplay(HST_DisplayNameService.ResolveShortItemDisplayName(candidateShort, fields[2]), fields[2]);
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

		if (m_sEditorMode == "storage")
		{
			if (FindStorageBrowserCategoryIndex(m_sSelectedCategory) < 0)
				m_sSelectedCategory = ResolveDefaultStorageCategory();
		}
		else if (FindCategoryIndex(m_sSelectedCategory) < 0 && m_aCategoryIds.Count() > 0)
		{
			m_sSelectedCategory = m_aCategoryIds[0];
		}

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

		ValidateStorageSelectionAfterPayload();
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
			candidateDisplay = NormalizeKnownVariantDisplay(HST_DisplayNameService.ResolveItemDisplayName(null, fields[2], candidateDisplay), fields[2]);
			m_aCandidateDisplays.Insert(candidateDisplay);
			string candidateShort = ResolvePayloadDisplayText(fields[4]);
			candidateShort = NormalizeKnownVariantDisplay(HST_DisplayNameService.ResolveShortItemDisplayName(candidateShort, fields[2]), fields[2]);
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

		if (IsDebugLoggingEnabled())
		{
			int candidateCount;
			for (int candidateIndex = 0; candidateIndex < m_aCandidateNodeIds.Count(); candidateIndex++)
			{
				if (m_aCandidateNodeIds[candidateIndex] == candidateNodeId)
					candidateCount++;
			}

			DebugLog(string.Format("candidate payload node=%1 status=%2 count=%3 reason=%4", candidateNodeId, candidateStatus, candidateCount, ShortenText(candidateEmptyReason, 120)));
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
		EnsureCandidatePayloadForNode(m_sSelectedNodeId);
	}

	protected void EnsureCandidatePayloadForStorageContainer()
	{
		EnsureSelectedStorageNode();
		EnsureCandidatePayloadForNode(ResolveSelectedStorageContainerNodeId());
	}

	protected void EnsureCandidatePayloadForNode(string nodeId)
	{
		if (nodeId.IsEmpty())
			return;

		if (FindStringIndex(m_aLoadedCandidateNodeIds, nodeId) >= 0)
			return;

		int requestedIndex = FindStringIndex(m_aRequestedCandidateNodeIds, nodeId);
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
				if (FindStringIndex(m_aLoadedCandidateNodeIds, nodeId) < 0)
					m_aLoadedCandidateNodeIds.Insert(nodeId);
				m_sLastResult = "h-istasi loadout editor | compatible item request timed out";
				SetCandidateEmptyReason(nodeId, "Compatible item request timed out");
				return;
			}
		}

		m_aRequestedCandidateNodeIds.Insert(nodeId);
		m_aRequestedCandidateFrames.Insert(m_iFrameSerial);
		m_aRequestedCandidateAttempts.Insert(requestAttempts);
		RequestServerAction("loadout_editor_candidates", nodeId);
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

	protected void EnsureNodeCapacityArrays(int requiredIndex)
	{
		while (m_aNodeUsedCapacities.Count() <= requiredIndex)
			m_aNodeUsedCapacities.Insert(0);

		while (m_aNodeTotalCapacities.Count() <= requiredIndex)
			m_aNodeTotalCapacities.Insert(0);
	}

	protected void EnsureNodeVolumeArrays(int requiredIndex)
	{
		while (m_aNodeUsedVolumes.Count() <= requiredIndex)
			m_aNodeUsedVolumes.Insert(0.0);

		while (m_aNodeTotalVolumes.Count() <= requiredIndex)
			m_aNodeTotalVolumes.Insert(0.0);

		while (m_aNodeFreeVolumes.Count() <= requiredIndex)
			m_aNodeFreeVolumes.Insert(0.0);
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
		m_aNodeUsedCapacities.Clear();
		m_aNodeTotalCapacities.Clear();
		m_aNodeUsedVolumes.Clear();
		m_aNodeTotalVolumes.Clear();
		m_aNodeFreeVolumes.Clear();
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
		m_sServerFailureText = "";
	}

	protected void BuildFallbackCategories()
	{
		if (m_aCategoryIds.Count() > 0)
			return;

		InsertCategory("headgear", "Headgear", 0);
		InsertCategory("clothing", "Clothing", 0);
		InsertCategory("vest", "Armored Vest", 0);
		InsertCategory("pants", "Pants", 0);
		InsertCategory("boots", "Boots", 0);
		InsertCategory("backpack", "Backpack", 0);
		InsertCategory("handwear", "Handwear", 0);
		InsertCategory("weapon", "Weapons", 0);
		InsertCategory("sidearm", "Sidearms", 0);
		InsertCategory("magazine", "Ammunition", 0);
		InsertCategory("explosive", "Throwables", 0);
		InsertCategory("attachment", "Attachments", 0);
		InsertCategory("medical", "Medical", 0);
		InsertCategory("utility", "Equipment", 0);
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
			m_aNodeUsedCapacities.Insert(0);
			m_aNodeTotalCapacities.Insert(0);
			m_aNodeUsedVolumes.Insert(0.0);
			m_aNodeTotalVolumes.Insert(0.0);
			m_aNodeFreeVolumes.Insert(0.0);
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

	protected int GetNodeUsedCapacity(int nodeIndex)
	{
		if (nodeIndex < 0 || nodeIndex >= m_aNodeUsedCapacities.Count())
			return 0;

		return Math.Max(0, m_aNodeUsedCapacities[nodeIndex]);
	}

	protected int GetNodeAvailableFitOptions(int nodeIndex)
	{
		if (nodeIndex < 0 || nodeIndex >= m_aNodeTotalCapacities.Count())
			return 0;

		return Math.Max(0, m_aNodeTotalCapacities[nodeIndex]);
	}

	protected string BuildNodeStorageStatusLabel(int nodeIndex)
	{
		int fitOptions = GetNodeAvailableFitOptions(nodeIndex);
		if (fitOptions <= 0)
			return "no matching room";

		if (fitOptions == 1)
			return "1 fit option";

		return string.Format("%1 fit options", fitOptions);
	}

	protected string BuildNodeStorageItemLabel(int nodeIndex)
	{
		int used = GetNodeUsedCapacity(nodeIndex);
		if (used == 1)
			return "1 item";

		return string.Format("%1 items", used);
	}

	protected float GetNodeUsedVolume(int nodeIndex)
	{
		if (nodeIndex < 0 || nodeIndex >= m_aNodeUsedVolumes.Count())
			return 0.0;

		return Math.Max(0.0, m_aNodeUsedVolumes[nodeIndex]);
	}

	protected float GetNodeTotalVolume(int nodeIndex)
	{
		if (nodeIndex < 0 || nodeIndex >= m_aNodeTotalVolumes.Count())
			return 0.0;

		return Math.Max(0.0, m_aNodeTotalVolumes[nodeIndex]);
	}

	protected float GetNodeFreeVolume(int nodeIndex)
	{
		if (nodeIndex < 0 || nodeIndex >= m_aNodeFreeVolumes.Count())
			return 0.0;

		return Math.Max(0.0, m_aNodeFreeVolumes[nodeIndex]);
	}

	protected float GetNodeVolumeRatio(int nodeIndex)
	{
		float total = GetNodeTotalVolume(nodeIndex);
		if (total <= 0.0)
			return 0.0;

		return Math.Clamp(GetNodeUsedVolume(nodeIndex) / total, 0.0, 1.0);
	}

	protected int ResolveStorageVolumeColor(int nodeIndex)
	{
		float ratio = GetNodeVolumeRatio(nodeIndex);
		if (ratio >= 0.95)
			return 0xFFB94A3C;
		if (ratio >= 0.75)
			return 0xFFE0A03A;

		return GetAccentColor();
	}

	protected string BuildNodeVolumeLabel(int nodeIndex)
	{
		float total = GetNodeTotalVolume(nodeIndex);
		if (total <= 0.0)
			return "0% / 100%";

		int usedPercent = Math.Round(GetNodeVolumeRatio(nodeIndex) * 100.0);
		usedPercent = ClampInt(usedPercent, 0, 100);
		return string.Format("%1", usedPercent) + "% / 100%";
	}

	protected string BuildStorageCapacityLabel(int nodeIndex)
	{
		return BuildNodeVolumeLabel(nodeIndex);
	}

	protected void RenderSelectedNodeHeader(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout)
			return;

		int nodeIndex = FindSelectedNodeIndex();
		SetLoadoutWidgetColor(root, "CandidateHeaderPreviewBack", 0xEE05080A, 1.0);
		SetLoadoutWidgetColor(root, "CandidateHeaderPreviewLine", 0x664B5960, 1.0);

		Widget anchor = root.FindAnyWidget("CandidateHeaderPreviewAnchor");
		if (anchor)
			CreateNodePreviewCell(workspace, anchor, nodeIndex, 0, 0xFFE6E6E6);

		SetLoadoutText(root, "CandidateHeaderSlot", GetNodeLabel(nodeIndex), 0xFFE2E6E8, m_Layout.m_iFontNormal, true, false);
		SetLoadoutText(root, "CandidateHeaderName", ShortenText(GetNodeDisplay(nodeIndex), 38), 0xFFD5D8D9, m_Layout.m_iFontNormal, false, false);
		SetLoadoutText(root, "CandidateHeaderMarker", "W", 0xFFFFFFFF, m_Layout.m_iFontTitle, true, false);
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
		string label;
		if (nodeIndex >= 0 && nodeIndex < m_aNodeLabels.Count())
			label = m_aNodeLabels[nodeIndex].Trim();

		string display;
		if (nodeIndex >= 0 && nodeIndex < m_aNodeDisplays.Count())
			display = m_aNodeDisplays[nodeIndex].Trim();

		if (IsAttachmentNodeIndex(nodeIndex) && IsGenericAttachmentLabel(label))
		{
			string attachmentLabel = ResolveAttachmentNodeLabel(nodeIndex);
			if (!attachmentLabel.IsEmpty())
				return attachmentLabel;
		}

		if (!label.IsEmpty() && !IsDuplicateDisplayText(label, display))
			return label;

		string resolvedLabel = ResolveNodeSlotLabel(nodeIndex);
		if (!resolvedLabel.IsEmpty())
			return resolvedLabel;

		if (!label.IsEmpty())
			return label;

		return "Empty Slot";
	}

	protected string GetNodeDisplay(int nodeIndex)
	{
		if (nodeIndex >= 0 && nodeIndex < m_aNodeDisplays.Count() && !m_aNodeDisplays[nodeIndex].IsEmpty())
			return m_aNodeDisplays[nodeIndex];

		return "Empty Slot";
	}

	protected string ResolveNodeSlotLabel(int nodeIndex)
	{
		if (nodeIndex < 0)
			return "";

		string kind;
		if (nodeIndex < m_aNodeKinds.Count())
			kind = m_aNodeKinds[nodeIndex];

		if (kind == "storage" || kind == "storage_item")
			return "";
		if (kind == "attachment")
			return ResolveAttachmentNodeLabel(nodeIndex);

		string category = ResolveNodeCategory(nodeIndex);
		if (category == "weapon")
			return "Primary Weapon";
		if (category == "launcher")
			return "Secondary Weapon";
		if (category == "sidearm")
			return "Sidearm";
		if (category == "headgear")
			return "Headgear";
		if (category == "clothing")
			return "Jacket";
		if (category == "vest")
			return "Armored Vest";
		if (category == "pants")
			return "Pants";
		if (category == "boots")
			return "Boots";
		if (category == "backpack")
			return "Backpack";
		if (category == "handwear")
			return "Handwear";
		if (category == "magazine")
			return "Ammunition";
		if (category == "explosive")
			return ResolveExplosiveNodeLabel(nodeIndex);
		if (category == "medical")
			return "Medical";
		if (category == "attachment")
			return ResolveAttachmentNodeLabel(nodeIndex);

		return BuildSlotCategoryLabel(category);
	}

	protected string ResolveAttachmentNodeLabel(int nodeIndex)
	{
		string label;
		if (nodeIndex >= 0 && nodeIndex < m_aNodeLabels.Count())
			label = m_aNodeLabels[nodeIndex].Trim();
		if (label == "Optics")
			return "Optic";
		if (!IsGenericAttachmentLabel(label))
			return label;

		string slotKey;
		if (nodeIndex >= 0 && nodeIndex < m_aNodeSlotKeys.Count())
			slotKey = m_aNodeSlotKeys[nodeIndex];
		string inferredLabel = InferAttachmentLabelFromNode(nodeIndex, slotKey);
		if (!inferredLabel.IsEmpty())
			return inferredLabel;

		return ResolveAttachmentSlotKeyLabel(slotKey);
	}

	protected bool IsAttachmentNodeIndex(int nodeIndex)
	{
		if (nodeIndex < 0)
			return false;

		if (nodeIndex < m_aNodeKinds.Count() && m_aNodeKinds[nodeIndex] == "attachment")
			return true;

		if (nodeIndex < m_aNodeIds.Count() && m_aNodeIds[nodeIndex].StartsWith(NODE_ATTACHMENT_PREFIX))
			return true;

		return false;
	}

	protected bool IsGenericAttachmentLabel(string label)
	{
		label = label.Trim();
		if (label.IsEmpty())
			return true;
		if (IsLikelyRawSlotIdentifier(label))
			return true;

		string lowered = label;
		lowered.ToLower();
		return lowered == "attachment" || lowered == "attachment slot" || lowered == "equipment" || lowered == "equipment slot";
	}

	protected string InferAttachmentLabelFromNode(int nodeIndex, string slotKey)
	{
		string combined = slotKey;
		if (nodeIndex >= 0 && nodeIndex < m_aNodeLabels.Count())
			combined = combined + " " + m_aNodeLabels[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodeDisplays.Count())
			combined = combined + " " + m_aNodeDisplays[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodePrefabs.Count())
			combined = combined + " " + m_aNodePrefabs[nodeIndex];
		combined.ToLower();

		if (combined.Contains("optic") || combined.Contains("scope") || combined.Contains("sight"))
			return "Optic";
		if (combined.Contains("muzzle") || combined.Contains("suppressor") || combined.Contains("flash_hider") || combined.Contains("flashhider") || combined.Contains("flash hider") || combined.Contains("compensator"))
			return "Muzzle";
		if (combined.Contains("underbarrel") || combined.Contains("bipod") || combined.Contains("grip"))
			return "Underbarrel";
		if (combined.Contains("bayonet"))
			return "Bayonet";
		if (combined.Contains("handguard"))
			return "Handguard";
		if (combined.Contains("stock") || combined.Contains("butt"))
			return "Stock";
		if (combined.Contains("magazine") || combined.Contains(" mag") || combined.Contains("_mag"))
			return "Magazine";
		if (combined.Contains("rail") || combined.Contains("accessory"))
			return "Accessory Rail";

		string candidateLabel = InferAttachmentLabelFromLoadedCandidates(nodeIndex);
		if (!candidateLabel.IsEmpty())
			return candidateLabel;

		return "";
	}

	protected string InferAttachmentLabelFromLoadedCandidates(int nodeIndex)
	{
		if (nodeIndex < 0 || nodeIndex >= m_aNodeIds.Count())
			return "";

		string nodeId = m_aNodeIds[nodeIndex];
		string inferred;
		for (int i = 0; i < m_aCandidateNodeIds.Count(); i++)
		{
			if (m_aCandidateNodeIds[i] != nodeId)
				continue;

			string candidateCombined;
			if (i < m_aCandidatePrefabs.Count())
				candidateCombined = candidateCombined + " " + m_aCandidatePrefabs[i];
			if (i < m_aCandidateDisplays.Count())
				candidateCombined = candidateCombined + " " + m_aCandidateDisplays[i];
			candidateCombined.ToLower();

			string label = InferAttachmentLabelFromText(candidateCombined);
			if (label.IsEmpty())
				continue;
			if (inferred.IsEmpty())
				inferred = label;
			else if (inferred != label)
				return "";
		}

		return inferred;
	}

	protected string InferAttachmentLabelFromText(string text)
	{
		text.ToLower();
		if (text.Contains("optic") || text.Contains("scope") || text.Contains("sight"))
			return "Optic";
		if (text.Contains("muzzle") || text.Contains("suppressor") || text.Contains("flash_hider") || text.Contains("flashhider") || text.Contains("flash hider") || text.Contains("compensator"))
			return "Muzzle";
		if (text.Contains("underbarrel") || text.Contains("bipod") || text.Contains("grip"))
			return "Underbarrel";
		if (text.Contains("bayonet"))
			return "Bayonet";
		if (text.Contains("handguard"))
			return "Handguard";
		if (text.Contains("stock") || text.Contains("butt"))
			return "Stock";
		if (text.Contains("magazine") || text.Contains(" mag") || text.Contains("_mag"))
			return "Magazine";
		if (text.Contains("rail") || text.Contains("accessory"))
			return "Accessory Rail";

		return "";
	}

	protected bool IsLikelyRawSlotIdentifier(string value)
	{
		value = value.Trim();
		if (value.Length() < 10)
			return false;

		int hexLike;
		for (int i = 0; i < value.Length(); i++)
		{
			string c = value.Substring(i, 1);
			if ("0123456789abcdefABCDEF".Contains(c))
				hexLike++;
		}

		return hexLike >= value.Length() - 1;
	}

	protected string ResolveAttachmentSlotKeyLabel(string slotKey)
	{
		slotKey.ToLower();
		if (slotKey == "optic")
			return "Optic";
		if (slotKey == "muzzle")
			return "Muzzle";
		if (slotKey == "underbarrel")
			return "Underbarrel";
		if (slotKey == "bayonet")
			return "Bayonet";
		if (slotKey == "handguard")
			return "Handguard";
		if (slotKey == "stock")
			return "Stock";
		if (slotKey == "magazine")
			return "Magazine";
		if (slotKey == "rail")
			return "Accessory Rail";
		if (slotKey == "equipment")
			return "Attachment Slot";

		return "Attachment Slot";
	}

	protected string ResolveExplosiveNodeLabel(int nodeIndex)
	{
		string label = "";
		if (nodeIndex >= 0 && nodeIndex < m_aNodeLabels.Count())
			label = m_aNodeLabels[nodeIndex];

		string loweredLabel = label;
		loweredLabel.ToLower();
		if (loweredLabel.Contains("grenade"))
			return "Grenade";
		if (loweredLabel.Contains("throwable") || loweredLabel.Contains("throw"))
			return "Throwable";

		string combined = label;
		if (nodeIndex >= 0 && nodeIndex < m_aNodeFocus.Count())
			combined = combined + " " + m_aNodeFocus[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodeIds.Count())
			combined = combined + " " + m_aNodeIds[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodeDisplays.Count())
			combined = combined + " " + m_aNodeDisplays[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodePrefabs.Count())
			combined = combined + " " + m_aNodePrefabs[nodeIndex];

		combined.ToLower();
		if (combined.Contains("throwable") || combined.Contains("throw") || combined.Contains("flare") || combined.Contains("smoke") || combined.Contains("chem") || combined.Contains("an-m8") || combined.Contains("m18"))
			return "Throwable";
		if (combined.Contains("grenade") || combined.Contains("rgd") || combined.Contains("m67") || combined.Contains("m433") || combined.Contains("hedp"))
			return "Grenade";

		return "Throwable";
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

		string targetNodeId = ResolveSelectedStorageContainerNodeId();
		if (targetNodeId.IsEmpty() || m_aCandidateNodeIds[candidateIndex] != targetNodeId)
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

	protected string GetCandidateDisplayName(int candidateIndex)
	{
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateDisplays.Count() && !m_aCandidateDisplays[candidateIndex].IsEmpty())
			return m_aCandidateDisplays[candidateIndex];

		return GetCandidateShortDisplay(candidateIndex);
	}

	protected string NormalizeKnownVariantDisplay(string displayName, string prefab)
	{
		if (prefab.IsEmpty())
			return displayName;

		string loweredPrefab = prefab;
		loweredPrefab.ToLower();
		bool isFlarePrefab = loweredPrefab.Contains("flare");
		if (loweredPrefab.Contains("ammo_flare_40mm_starparachute_green") || loweredPrefab.Contains("starparachute_green"))
			return "40mm M195 Green Star Parachute Flare";
		if (loweredPrefab.Contains("ammo_flare_40mm_starparachute_red") || loweredPrefab.Contains("starparachute_red"))
			return "40mm M126A1 Red Star Parachute Flare";
		if (loweredPrefab.Contains("ammo_flare_40mm_starparachute_white") || loweredPrefab.Contains("starparachute_white"))
			return "40mm M127A1 White Star Parachute Flare";
		if (loweredPrefab.Contains("ammo_flare_40mm_vg40op_white") || loweredPrefab.Contains("vg40op_white"))
			return "40mm VG-40OP White Flare";
		if (loweredPrefab.Contains("ammo_flare_30mm_rsp30_green"))
			return "30mm RSP-30 Green Flare";
		if (loweredPrefab.Contains("ammo_flare_30mm_rsp30_red"))
			return "30mm RSP-30 Red Flare";
		if (loweredPrefab.Contains("ammo_flare_30mm_rsp30_white"))
			return "30mm RSP-30 White Flare";
		if (loweredPrefab.Contains("m661") || (isFlarePrefab && (loweredPrefab.Contains("m195") || loweredPrefab.Contains("rsp30_green") || loweredPrefab.Contains("green_flare") || loweredPrefab.Contains("flare_green") || loweredPrefab.Contains("_green"))))
		{
			if (loweredPrefab.Contains("rsp30") || loweredPrefab.Contains("m195"))
				return "Flare Launcher - Green";
			return "40mm M661 Green Flare";
		}
		if (loweredPrefab.Contains("m662") || (isFlarePrefab && (loweredPrefab.Contains("m126a1") || loweredPrefab.Contains("rsp30_red") || loweredPrefab.Contains("red_flare") || loweredPrefab.Contains("flare_red") || loweredPrefab.Contains("_red"))))
		{
			if (loweredPrefab.Contains("rsp30") || loweredPrefab.Contains("m126a1"))
				return "Flare Launcher - Red";
			return "40mm M662 Red Flare";
		}
		if (loweredPrefab.Contains("m583") || (isFlarePrefab && (loweredPrefab.Contains("m127a1") || loweredPrefab.Contains("rsp30_white") || loweredPrefab.Contains("white_flare") || loweredPrefab.Contains("flare_white") || loweredPrefab.Contains("_white"))))
		{
			if (loweredPrefab.Contains("rsp30") || loweredPrefab.Contains("m127a1"))
				return "Flare Launcher - White";
			return "40mm M583A1 White Flare";
		}

		return displayName;
	}

	protected bool IsDuplicateDisplayText(string first, string second)
	{
		first = first.Trim();
		second = second.Trim();
		if (first.IsEmpty() || second.IsEmpty())
			return false;

		return first == second;
	}

	protected string BuildTwoLineDisplayText(string label, string display)
	{
		label = label.Trim();
		display = display.Trim();
		if (label.IsEmpty())
			return display;
		if (display.IsEmpty() || IsDuplicateDisplayText(label, display))
			return label;

		return label + "\n" + display;
	}

	protected string BuildCountBadgeLabel(string rawCount, bool infinite, bool showOne = true)
	{
		if (infinite)
			return "INF";

		if (rawCount.IsEmpty())
			return "";

		int count = Math.Max(0, rawCount.ToInt());
		if (count <= 0)
			return "";

		if (!showOne && count <= 1)
			return "";

		return "x" + count;
	}

	protected string BuildNodeCountBadge(int nodeIndex, bool showOne = false)
	{
		if (nodeIndex < 0 || nodeIndex >= m_aNodeCounts.Count())
			return "";

		bool infinite = false;
		if (nodeIndex >= 0 && nodeIndex < m_aNodeInfinite.Count())
			infinite = m_aNodeInfinite[nodeIndex];

		return BuildCountBadgeLabel(m_aNodeCounts[nodeIndex], infinite, showOne);
	}

	protected string BuildCandidateCountLabel(int candidateIndex, bool showOne = true)
	{
		if (candidateIndex < 0 || candidateIndex >= m_aCandidateCounts.Count())
			return "";

		bool infinite = false;
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateInfinite.Count())
			infinite = m_aCandidateInfinite[candidateIndex];

		return BuildCountBadgeLabel(m_aCandidateCounts[candidateIndex], infinite, showOne);
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

		bool infinite = false;
		if (itemIndex < m_aItemInfinite.Count())
			infinite = m_aItemInfinite[itemIndex];

		return BuildCountBadgeLabel(m_aItemCounts[itemIndex], infinite, true);
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
			return "Jacket";
		if (category == "headgear")
			return "Headgear";
		if (category == "vest")
			return "Armored Vest";
		if (category == "pants")
			return "Pants";
		if (category == "boots")
			return "Boots";
		if (category == "backpack")
			return "Backpack";
		if (category == "handwear")
			return "Handwear";
		if (category == "weapon")
			return "Primary Weapon";
		if (category == "sidearm")
			return "Sidearm";
		if (category == "launcher")
			return "Secondary Weapon";
		if (category == "magazine")
			return "Ammunition";
		if (category == "explosive")
			return "Throwables";
		if (category == "attachment")
			return "Attachment";
		if (category == "medical")
			return "Medical";

		return "Equipment";
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

	protected bool IsDebugLoggingEnabled()
	{
		return m_bDebugLoggingEnabled;
	}

	protected void DebugLog(string message)
	{
		if (!IsDebugLoggingEnabled())
			return;

		Print("h-istasi loadout editor debug | " + message);
	}

	protected float ParsePayloadFloat(string value, float fallback = 0.0)
	{
		if (value.IsEmpty())
			return fallback;

		return value.ToFloat();
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
			return ResolveDefaultStorageCategory();

		return m_sSelectedCategory;
	}

	protected string ResolveDefaultStorageCategory()
	{
		return "magazine";
	}

	protected string ResolveModeForCategory(string category)
	{
		if (category == "clothing" || category == "headgear" || category == "vest" || category == "pants" || category == "boots" || category == "backpack" || category == "handwear")
			return "clothing";
		if (category == "weapon" || category == "launcher" || category == "sidearm")
			return "weapons";
		if (category == "attachment")
			return "attachments";
		if (category == "magazine" || category == "explosive" || category == "medical" || category == "utility" || category == "weapon_group" || category == "clothing_group")
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
			return category == "magazine" || category == "explosive" || category == "medical" || category == "utility" || category == "weapon" || category == "launcher" || category == "sidearm" || category == "clothing" || category == "headgear" || category == "vest" || category == "pants" || category == "boots" || category == "backpack" || category == "handwear" || category == "weapon_group" || category == "clothing_group";
		if (modeId == "settings" || modeId == "save")
			return true;

		return false;
	}

	protected int GetStorageBrowserCategoryCount()
	{
		return 6;
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
			return "weapon_group";

		return "clothing_group";
	}

	protected int FindStorageBrowserCategoryIndex(string category)
	{
		for (int i = 0; i < GetStorageBrowserCategoryCount(); i++)
		{
			if (GetStorageBrowserCategoryId(i) == category)
				return i;
		}

		return -1;
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
		if (category == "weapon_group")
			return "Weapons";
		if (category == "clothing_group")
			return "Clothing";

		return "Items";
	}

	protected bool IsCategoryInStorageBrowserTab(string itemCategory, string tabCategory)
	{
		if (tabCategory == "weapon_group")
			return itemCategory == "weapon" || itemCategory == "launcher" || itemCategory == "sidearm";
		if (tabCategory == "clothing_group")
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

	protected void ValidateStorageSelectionAfterPayload()
	{
		if (!m_sSelectedStorageContainerNodeId.IsEmpty() && FindStorageContainerNodeIndex(m_sSelectedStorageContainerNodeId) < 0)
			m_sSelectedStorageContainerNodeId = "";

		if (!m_sSelectedStoredItemNodeId.IsEmpty())
		{
			int storedNodeIndex = FindStoredItemNodeIndex(m_sSelectedStoredItemNodeId);
			if (storedNodeIndex < 0)
			{
				m_sSelectedStoredItemNodeId = "";
			}
			else if (storedNodeIndex < m_aNodeParents.Count() && FindStorageContainerNodeIndex(m_aNodeParents[storedNodeIndex]) >= 0)
			{
				m_sSelectedStorageContainerNodeId = m_aNodeParents[storedNodeIndex];
			}
		}

		if (m_sEditorMode == "storage")
			EnsureSelectedStorageNode();
	}

	protected void ClearStorageSelection()
	{
		m_sSelectedStorageContainerNodeId = "";
		m_sSelectedStoredItemNodeId = "";
	}

	protected int FindStorageContainerNodeIndex(string nodeId)
	{
		int nodeIndex = FindStringIndex(m_aNodeIds, nodeId);
		if (nodeIndex < 0 || nodeIndex >= m_aNodeKinds.Count())
			return -1;
		if (m_aNodeKinds[nodeIndex] != "storage")
			return -1;

		return nodeIndex;
	}

	protected int FindStoredItemNodeIndex(string nodeId)
	{
		int nodeIndex = FindStringIndex(m_aNodeIds, nodeId);
		if (nodeIndex < 0 || nodeIndex >= m_aNodeKinds.Count())
			return -1;
		if (m_aNodeKinds[nodeIndex] != "storage_item")
			return -1;

		return nodeIndex;
	}

	protected string ResolveParentStorageNodeForSelectedItem()
	{
		int nodeIndex = FindStoredItemNodeIndex(m_sSelectedStoredItemNodeId);
		if (nodeIndex < 0)
			nodeIndex = FindStoredItemNodeIndex(m_sSelectedNodeId);
		if (nodeIndex < 0 || nodeIndex >= m_aNodeParents.Count())
			return "";

		string parentNodeId = m_aNodeParents[nodeIndex];
		if (FindStorageContainerNodeIndex(parentNodeId) < 0)
			return "";

		return parentNodeId;
	}

	protected string ResolveSelectedStorageContainerNodeId()
	{
		if (FindStorageContainerNodeIndex(m_sSelectedStorageContainerNodeId) >= 0)
			return m_sSelectedStorageContainerNodeId;

		string parentNodeId = ResolveParentStorageNodeForSelectedItem();
		if (!parentNodeId.IsEmpty())
			return parentNodeId;

		if (FindStorageContainerNodeIndex(m_sSelectedNodeId) >= 0)
			return m_sSelectedNodeId;

		return "";
	}

	protected void SelectStorageContainerNode(int nodeIndex)
	{
		if (nodeIndex < 0 || nodeIndex >= m_aNodeIds.Count())
			return;

		m_sSelectedStorageContainerNodeId = m_aNodeIds[nodeIndex];
		m_sSelectedStoredItemNodeId = "";
		m_sSelectedNodeId = m_sSelectedStorageContainerNodeId;
		m_sSelectedSlotId = "";
		m_sSelectedCategory = ResolveDefaultStorageCategory();
		m_bCandidateMode = true;
		m_iItemPage = 0;
		m_fCandidateScrollY = 0.0;
		m_fStorageCandidateScrollY = 0.0;
		m_sPreviewRenderKey = "";
		EnsureCandidatePayloadForStorageContainer();
	}

	protected void SelectStoredItemNode(int nodeIndex)
	{
		if (nodeIndex < 0 || nodeIndex >= m_aNodeIds.Count())
			return;

		m_sSelectedStoredItemNodeId = m_aNodeIds[nodeIndex];
		if (nodeIndex < m_aNodeParents.Count() && FindStorageContainerNodeIndex(m_aNodeParents[nodeIndex]) >= 0)
			m_sSelectedStorageContainerNodeId = m_aNodeParents[nodeIndex];
		m_sSelectedNodeId = m_sSelectedStoredItemNodeId;
		m_sSelectedSlotId = "";
		m_bCandidateMode = false;
		m_sPreviewRenderKey = "";
	}

	protected void EnsureSelectedStorageNode()
	{
		string selectedContainerNodeId = ResolveSelectedStorageContainerNodeId();
		if (!selectedContainerNodeId.IsEmpty())
		{
			m_sSelectedStorageContainerNodeId = selectedContainerNodeId;
			if (m_sSelectedNodeId.IsEmpty())
			{
				m_sSelectedNodeId = selectedContainerNodeId;
				m_bCandidateMode = true;
			}
			return;
		}

		for (int i = 0; i < m_aNodeKinds.Count(); i++)
		{
			if (m_aNodeKinds[i] != "storage")
				continue;

			m_sSelectedStorageContainerNodeId = m_aNodeIds[i];
			if (m_sSelectedNodeId.IsEmpty() || m_sSelectedStoredItemNodeId.IsEmpty())
			{
				m_sSelectedNodeId = m_aNodeIds[i];
				m_bCandidateMode = true;
			}
			return;
		}

		m_sSelectedStorageContainerNodeId = "";
		if (m_sEditorMode == "storage")
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
		int nodeIndex = FindStorageContainerNodeIndex(m_sSelectedStorageContainerNodeId);
		if (nodeIndex < 0)
			nodeIndex = FindStorageContainerNodeIndex(ResolveSelectedStorageContainerNodeId());
		if (nodeIndex < 0)
			nodeIndex = FindSelectedNodeIndex();
		if (nodeIndex < 0)
			return "Select Storage";

		return string.Format("%1 | %2", GetNodeDisplay(nodeIndex), BuildStorageCapacityLabel(nodeIndex));
	}

	protected void RenderStorageCategoryTabs(WorkspaceWidget workspace, Widget root)
	{
		if (!m_Layout || !workspace || !root)
			return;

		int count = GetStorageBrowserCategoryCount();
		if (count <= 0)
			return;

		for (int i = 0; i < count; i++)
		{
			Widget tab = workspace.CreateWidgets(LOADOUT_STORAGE_CATEGORY_TAB_LAYOUT, root);
			if (!tab)
				continue;

			m_aWidgets.Insert(tab);
			tab.SetUserID(STORAGE_CATEGORY_WIDGET_ID_BASE + i);
			tab.AddHandler(m_WidgetHandler);

			string category = GetStorageBrowserCategoryId(i);
			bool active = category == m_sSelectedCategory;
			HST_UIDebug.LogRowSample("loadout_storage_category_tabs", LOADOUT_STORAGE_CATEGORY_TAB_LAYOUT, i, string.Format("category=%1 label=%2 count=%3 active=%4 userId=%5 row=%6", category, ShortenText(GetStorageBrowserCategoryLabel(category), 48), CountStorageCandidatesForTab(category), active, STORAGE_CATEGORY_WIDGET_ID_BASE + i, HST_UIDebug.WidgetSummary(tab)));

			int background = GetTabBackColor();
			int accent = 0x884B5960;
			int foreground = 0xFFF5E8CE;
			float accentOpacity = 0.35;
			if (active)
			{
				background = GetSelectedRowColor();
				accent = GetAccentColor();
				foreground = 0xFFFFFFFF;
				accentOpacity = 1.0;
			}

			SetLoadoutWidgetColor(tab, "Background", background, 0.98);
			SetLoadoutWidgetColor(tab, "Accent", accent, accentOpacity);
			SetLoadoutWidgetVisible(tab, "Fallback", false);
			if (!SetLoadoutImageTexture(tab, "Icon", ResolveIconTexture(category), foreground))
				SetLoadoutWidgetVisible(tab, "Icon", false);
		}

		HST_UIDebug.LogRowSummary("loadout_storage_category_tabs", LOADOUT_STORAGE_CATEGORY_TAB_LAYOUT, count, string.Format("selectedCategory=%1 target=%2", m_sSelectedCategory, ShortenText(ResolveSelectedStorageContainerNodeId(), 48)));
	}

	protected int CountStorageCandidatesForTab(string tabCategory)
	{
		int count;
		string targetNodeId = ResolveSelectedStorageContainerNodeId();

		for (int i = 0; i < m_aCandidateKinds.Count(); i++)
		{
			if (i >= m_aCandidateNodeIds.Count())
				continue;

			if (targetNodeId.IsEmpty() || m_aCandidateNodeIds[i] != targetNodeId)
				continue;

			if (IsCategoryInStorageBrowserTab(m_aCandidateKinds[i], tabCategory))
				count++;
		}

		return count;
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

	protected void ConfigurePreviewDimmer(Widget root)
	{
		if (!root)
			return;

		Widget backdrop = root.FindAnyWidget("HST_LoadoutBackdrop");
		if (backdrop)
		{
			backdrop.SetVisible(true);
			backdrop.SetOpacity(1.0);
			backdrop.SetZOrder(0);
			backdrop.SetFlags(WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS);
		}

		Widget dimmer = root.FindAnyWidget("HST_LoadoutDimmer");
		if (!dimmer)
			return;

		dimmer.SetVisible(false);
		dimmer.SetOpacity(0.0);
		dimmer.SetZOrder(2);
	}

	protected void ConfigurePreviewWidget()
	{
		if (!m_PreviewWidget)
			return;

		m_PreviewWidget.SetVisible(true);
		m_PreviewWidget.SetOpacity(1.0);
		m_PreviewWidget.SetColorInt(GetPreviewWorldTintColor());
		m_PreviewWidget.SetZOrder(LOADOUT_PREVIEW_Z);
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
		else
			ApplyPreviewWorldSettings();
	}

	protected void RebuildPreviewStage()
	{
		if (m_PreviewStageEntity)
			SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewStageEntity);

		m_PreviewStageEntity = null;
		SpawnPreviewStage();
	}

	protected void ApplyPreviewWorldSettings()
	{
		if (!m_PreviewStageEntity)
			return;

		EnsureVisualSettings();
		if (m_VisualSettings.m_iWorldPreset <= 0)
			return;

		ApplyPreviewWorldMaterialToSkySphereRecursive(m_PreviewStageEntity, GetPreviewWorldMaterial());
	}

	protected bool ApplyPreviewWorldMaterialToSkySphereRecursive(IEntity entity, ResourceName material)
	{
		if (!entity)
			return false;

		bool applied = false;
		if (entity != m_PreviewStageEntity && IsPreviewSkySphereEntity(entity))
		{
			SCR_Global.SetMaterial(entity, material, true);
			return true;
		}

		IEntity child = entity.GetChildren();
		while (child)
		{
			if (ApplyPreviewWorldMaterialToSkySphereRecursive(child, material))
				applied = true;
			child = child.GetSibling();
		}

		return applied;
	}

	protected bool IsPreviewSkySphereEntity(IEntity entity)
	{
		if (!entity)
			return false;

		vector mins;
		vector maxs;
		entity.GetWorldBounds(mins, maxs);
		float height = maxs[1] - mins[1];
		float width = Math.Max(maxs[0] - mins[0], maxs[2] - mins[2]);
		return height > 20.0 && width > 20.0;
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
		ApplyPreviewLightSettings();
		UpdatePreviewLightAngles();
	}

	protected void RefreshPreviewLightState()
	{
		if (!m_PreviewLightEntity)
			QueuePreviewLightSpawn();
		else
		{
			ApplyPreviewLightSettings();
			UpdatePreviewLightAngles();
		}
	}

	protected void ApplyPreviewLightSettings()
	{
		EnsureVisualSettings();
		if (!m_PreviewLightEntity)
			return;

		ApplyPreviewLightSettingsRecursive(m_PreviewLightEntity);
	}

	protected void ApplyPreviewLightSettingsRecursive(IEntity entity)
	{
		if (!entity)
			return;

		LightEntity light = LightEntity.Cast(entity);
		if (light)
			light.SetColor(Color.FromInt(Color.WHITE), m_VisualSettings.m_fPreviewLightLV);

		IEntity child = entity.GetChildren();
		while (child)
		{
			ApplyPreviewLightSettingsRecursive(child);
			child = child.GetSibling();
		}
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
		if (previewMode == PREVIEW_MODE_ENTITY)
			PositionPreviewEntityAtStage(m_PreviewEntity, "0 1.15 0");
		else
			PositionPreviewEntityAtStage(m_PreviewEntity, "0 0.95 0");
		m_PreviewEntity.Update();
		SetPreviewEntityQualityRecursive(m_PreviewEntity);
		vector debugMins;
		vector debugMaxs;
		m_PreviewEntity.GetWorldBounds(debugMins, debugMaxs);
		DebugLog(string.Format("preview mode=%1 source=%2 preview=%3 mins=%4 maxs=%5 size=%6 world=%7 light=%8", previewMode, previewSource, m_PreviewEntity, debugMins, debugMaxs, vector.Distance(debugMins, debugMaxs), m_PreviewWorld, m_PreviewLightEntity));
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

		entity.SetVComponentFlags(VCFlags.NOFILTER);
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

	protected IEntity ResolveItemIconPreviewEntityForNode(string nodeId)
	{
		if (nodeId.IsEmpty())
			return null;

		IEntity character = ResolvePreviewEditedCharacter();
		if (!character)
			return null;

		if (nodeId.IndexOf(NODE_LOADOUT_PREFIX) == 0)
		{
			int slotIndex = ParseSinglePreviewNodeIndex(nodeId, NODE_LOADOUT_PREFIX);
			SCR_CharacterInventoryStorageComponent characterStorage = SCR_CharacterInventoryStorageComponent.Cast(character.FindComponent(SCR_CharacterInventoryStorageComponent));
			if (!characterStorage || slotIndex < 0 || slotIndex >= characterStorage.GetSlotsCount())
				return null;

			InventoryStorageSlot slot = characterStorage.GetSlot(slotIndex);
			if (!slot)
				return null;

			return slot.GetAttachedEntity();
		}

		if (nodeId.IndexOf(NODE_STORAGE_ITEM_PREFIX) == 0)
		{
			int containerSlotIndex;
			int itemIndex;
			if (!ParseTwoPreviewNodeIndexes(nodeId, NODE_STORAGE_ITEM_PREFIX, containerSlotIndex, itemIndex))
				return null;

			array<BaseInventoryStorageComponent> storages = {};
			if (ResolvePreviewStorageTargets(character, NODE_STORAGE_PREFIX + string.Format("%1", containerSlotIndex), storages) <= 0)
				return null;

			array<IEntity> contents = {};
			array<IEntity> visited = {};
			GatherPreviewStorageContentEntitiesFromStorages(storages, contents, visited);
			if (itemIndex < 0 || itemIndex >= contents.Count())
				return null;

			return contents[itemIndex];
		}

		if (nodeId.IndexOf(NODE_STORAGE_PREFIX) == 0)
		{
			int containerSlotIndex = ParseSinglePreviewNodeIndex(nodeId, NODE_STORAGE_PREFIX);
			return ResolvePreviewStorageContainer(character, containerSlotIndex);
		}

		if (nodeId.IndexOf(NODE_WEAPON_PREFIX) == 0)
		{
			BaseInventoryStorageComponent weaponStorage;
			InventoryStorageSlot weaponSlot;
			if (!ResolvePreviewIndexedStorageSlot(character, nodeId, NODE_WEAPON_PREFIX, weaponStorage, weaponSlot))
				return null;

			return weaponSlot.GetAttachedEntity();
		}

		if (nodeId.IndexOf(NODE_ATTACHMENT_PREFIX) == 0)
		{
			int weaponStorageIndex;
			int weaponSlotIndex;
			int attachmentSlotIndex;
			if (!ParseThreePreviewNodeIndexes(nodeId, NODE_ATTACHMENT_PREFIX, weaponStorageIndex, weaponSlotIndex, attachmentSlotIndex))
				return null;

			array<BaseInventoryStorageComponent> storages = {};
			FindPreviewInventoryStoragesWithSlots(character, storages);
			if (weaponStorageIndex < 0 || weaponStorageIndex >= storages.Count())
				return null;

			BaseInventoryStorageComponent weaponStorage = storages[weaponStorageIndex];
			if (!weaponStorage || weaponSlotIndex < 0 || weaponSlotIndex >= weaponStorage.GetSlotsCount())
				return null;

			InventoryStorageSlot weaponSlot = weaponStorage.GetSlot(weaponSlotIndex);
			if (!weaponSlot)
				return null;

			IEntity weaponEntity = weaponSlot.GetAttachedEntity();
			if (!weaponEntity)
				return null;

			SCR_WeaponAttachmentsStorageComponent attachmentStorage = SCR_WeaponAttachmentsStorageComponent.Cast(weaponEntity.FindComponent(SCR_WeaponAttachmentsStorageComponent));
			if (!attachmentStorage || attachmentSlotIndex < 0 || attachmentSlotIndex >= attachmentStorage.GetSlotsCount())
				return null;

			InventoryStorageSlot attachmentSlot = attachmentStorage.GetSlot(attachmentSlotIndex);
			if (!attachmentSlot)
				return null;

			return attachmentSlot.GetAttachedEntity();
		}

		return null;
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

		if (IsPreviewStructuralAttachmentStorage(storage))
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
			if (nodeIndex >= m_aNodeIds.Count())
				continue;
			if (m_aNodePrefabs[nodeIndex].IsEmpty())
				continue;
			if (nodeIndex < m_aNodeKinds.Count() && m_aNodeKinds[nodeIndex] == "storage_item")
				continue;

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
		DebugLog(string.Format("preview camera %1 entity=%2 target=%3 look=%4 direction=%5 distance=%6 yaw=%7 sourceMode=%8", reason, m_PreviewEntity, m_vCameraTargetPosition, m_vCameraTargetLook, m_vCameraTargetDirection, m_fCameraTargetDistance, m_fPreviewYawDegrees, m_sPreviewSourceMode));
	}

	protected void HandlePreviewDragInput(InputManager inputManager)
	{
		if (!inputManager || !m_bPreviewDragActive)
			return;

		if (!m_bPreviewDragFocused && inputManager.GetActionValue("Inventory_Drag") <= 0.05)
		{
			m_bPreviewDragActive = false;
			return;
		}

		int mouseX;
		int mouseY;
		WidgetManager.GetMousePos(mouseX, mouseY);
		int deltaX = mouseX - m_iPreviewDragLastX;
		int deltaY = mouseY - m_iPreviewDragLastY;
		m_iPreviewDragLastX = mouseX;
		m_iPreviewDragLastY = mouseY;

		if (deltaX == 0 && deltaY == 0)
			return;

		PanPreviewCamera(deltaX * 0.85, deltaY * 0.85);
	}

	protected bool IsPreviewCameraCandidateValid(vector direction, vector position)
	{
		float minDirectionY = -0.06;
		float maxDirectionY = 0.30;
		if (m_sPreviewSourceMode == PREVIEW_MODE_ENTITY)
		{
			minDirectionY = -0.05;
			maxDirectionY = 0.24;
		}

		if (direction[1] < minDirectionY || direction[1] > maxDirectionY)
			return false;

		float minCameraY = 0.55;
		if (m_sPreviewSourceMode == PREVIEW_MODE_ENTITY)
			minCameraY = 0.35;
		float size = Math.Max(0.5, m_fPreviewedEntitySize);
		float maxCameraY = m_vPreviewedEntityCenterWorld[1] + Math.Max(0.85, size * 0.72);
		if (m_sPreviewSourceMode == PREVIEW_MODE_ENTITY)
			maxCameraY = m_vPreviewedEntityCenterWorld[1] + Math.Max(1.05, size * 0.62);
		if (position[1] < minCameraY)
			return false;
		if (position[1] > maxCameraY)
			return false;

		return true;
	}

	protected void PanPreviewCamera(float inputX, float inputY)
	{
		if (!m_PreviewWorld || !m_bCameraInitialized)
			return;

		m_PreviewWorld.GetCamera(PREVIEW_CAMERA, m_aCurrentCameraMatrix);

		vector newMatrix[4];
		newMatrix = m_aCurrentCameraMatrix;
		newMatrix[3] = m_vCameraTargetLook + (m_vCameraTargetDirection * m_fCameraTargetDistance);

		SCR_Math3D.RotateAround(newMatrix, m_vCameraTargetLook, m_aCurrentCameraMatrix[1], inputX * 0.01, newMatrix);
		m_vCameraTargetDirection = vector.Direction(m_vCameraTargetLook, newMatrix[3]).Normalized();

		SCR_Math3D.RotateAround(newMatrix, m_vCameraTargetLook, m_aCurrentCameraMatrix[0], inputY * 0.01, newMatrix);
		vector candidateDirection = vector.Direction(m_vCameraTargetLook, newMatrix[3]).Normalized();
		vector candidatePosition = m_vCameraTargetLook + (candidateDirection * m_fCameraTargetDistance);
		if (!IsPreviewCameraCandidateValid(candidateDirection, candidatePosition))
			return;

		m_vCameraTargetDirection = candidateDirection;
		m_vCameraTargetPosition = candidatePosition;
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

	protected void ResetLoadoutScroll()
	{
		m_iItemPage = 0;
		m_iTemplatePage = 0;
		m_fSlotScrollY = 0.0;
		m_fCandidateScrollY = 0.0;
		m_fTemplateScrollY = 0.0;
		m_fStorageCandidateScrollY = 0.0;
		m_fStorageContainerScrollY = 0.0;
		m_fStorageContentScrollY = 0.0;
	}

	protected ItemPreviewManagerEntity GetItemPreviewManager()
	{
		if (m_ItemPreviewManager)
			return m_ItemPreviewManager;

		ChimeraWorld chimeraWorld = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!chimeraWorld)
			return null;

		m_ItemPreviewManager = chimeraWorld.GetItemPreviewManager();
		return m_ItemPreviewManager;
	}

	protected Widget CreateItemPreviewCell(WorkspaceWidget workspace, Widget parent, int userId)
	{
		if (!workspace || !parent)
			return null;

		Widget cell = workspace.CreateWidgets(ITEM_PREVIEW_CELL_LAYOUT, parent);
		int debugIndex = m_iDebugPreviewCellLogIndex;
		m_iDebugPreviewCellLogIndex++;
		HST_UIDebug.LogRowSample("loadout_preview_cells", ITEM_PREVIEW_CELL_LAYOUT, debugIndex, string.Format("userId=%1 parent=%2 cell=%3", userId, HST_UIDebug.WidgetSummary(parent), HST_UIDebug.WidgetSummary(cell)));
		if (!cell)
			return null;

		cell.SetVisible(true);
		cell.SetOpacity(1.0);
		cell.SetZOrder(20);

		Widget slotImage = cell.FindAnyWidget("SlotImage");
		if (slotImage)
		{
			slotImage.SetVisible(true);
			slotImage.SetOpacity(1.0);
			slotImage.SetZOrder(21);
		}

		Widget slotPreview = cell.FindAnyWidget("SlotPreview");
		if (slotPreview)
		{
			slotPreview.SetVisible(false);
			slotPreview.SetOpacity(0.0);
			slotPreview.SetZOrder(22);
		}

		if (userId != 0)
		{
			cell.SetUserID(userId);
			cell.AddHandler(m_WidgetHandler);
			BindItemPreviewCellChild(cell, "SlotPreview", userId);
			BindItemPreviewCellChild(cell, "SlotImage", userId);
		}

		m_aWidgets.Insert(cell);
		return cell;
	}

	protected void BindItemPreviewCellChild(Widget cell, string childName, int userId)
	{
		if (!cell || userId == 0)
			return;

		Widget child = cell.FindAnyWidget(childName);
		if (!child)
			return;

		child.SetUserID(userId);
		child.AddHandler(m_WidgetHandler);
	}

	protected void SetPreviewCellFallbackIcon(Widget cell, ResourceName iconTexture, int color)
	{
		if (!cell)
			return;

		ItemPreviewWidget previewWidget = ItemPreviewWidget.Cast(cell.FindAnyWidget("SlotPreview"));
		if (previewWidget)
		{
			previewWidget.SetVisible(false);
			previewWidget.SetOpacity(0.0);
		}

		ImageWidget imageWidget = ImageWidget.Cast(cell.FindAnyWidget("SlotImage"));
		if (!imageWidget)
			return;

		ResourceName texture = iconTexture;
		if (texture.IsEmpty())
			texture = FALLBACK_PREVIEW_ICON;

		imageWidget.LoadImageTexture(0, texture);
		imageWidget.SetImage(0);
		imageWidget.SetColorInt(color);
		imageWidget.SetVisible(true);
		imageWidget.SetOpacity(1.0);
		imageWidget.SetZOrder(21);
	}

	protected void SetPreviewCellNeutralFallback(Widget cell)
	{
		if (!cell)
			return;

		ItemPreviewWidget previewWidget = ItemPreviewWidget.Cast(cell.FindAnyWidget("SlotPreview"));
		if (previewWidget)
		{
			previewWidget.SetVisible(false);
			previewWidget.SetOpacity(0.0);
		}

		ImageWidget imageWidget = ImageWidget.Cast(cell.FindAnyWidget("SlotImage"));
		if (imageWidget)
		{
			imageWidget.SetVisible(false);
			imageWidget.SetOpacity(0.0);
		}
	}
	protected bool SetPreviewCellFromPrefab(Widget cell, string prefab, ResourceName fallbackIcon, int color, bool showFallbackIcon = true)
	{
		if (!cell)
			return false;

		if (showFallbackIcon)
			SetPreviewCellFallbackIcon(cell, fallbackIcon, color);
		else
			SetPreviewCellNeutralFallback(cell);

		if (prefab.IsEmpty())
			return false;

		ItemPreviewManagerEntity previewManager = GetItemPreviewManager();
		ItemPreviewWidget previewWidget = ItemPreviewWidget.Cast(cell.FindAnyWidget("SlotPreview"));
		if (!previewManager || !previewWidget)
			return false;

		ResourceName resourceName = prefab;
		Resource resource = Resource.Load(resourceName);
		if (!resource || !resource.IsValid())
			return false;

		ImageWidget imageWidget = ImageWidget.Cast(cell.FindAnyWidget("SlotImage"));
		if (imageWidget)
		{
			imageWidget.SetVisible(false);
			imageWidget.SetOpacity(0.0);
		}

		previewWidget.SetVisible(true);
		previewWidget.SetOpacity(1.0);
		previewWidget.SetZOrder(22);
		previewManager.SetPreviewItemFromPrefab(previewWidget, resourceName, null, true);
		return true;
	}

	protected bool SetPreviewCellFromEntity(Widget cell, IEntity entity, ResourceName fallbackIcon, int color, bool showFallbackIcon = true)
	{
		if (!cell)
			return false;

		if (showFallbackIcon)
			SetPreviewCellFallbackIcon(cell, fallbackIcon, color);
		else
			SetPreviewCellNeutralFallback(cell);

		if (!entity)
			return false;

		ItemPreviewManagerEntity previewManager = GetItemPreviewManager();
		ItemPreviewWidget previewWidget = ItemPreviewWidget.Cast(cell.FindAnyWidget("SlotPreview"));
		if (!previewManager || !previewWidget)
			return false;

		ImageWidget imageWidget = ImageWidget.Cast(cell.FindAnyWidget("SlotImage"));
		if (imageWidget)
		{
			imageWidget.SetVisible(false);
			imageWidget.SetOpacity(0.0);
		}

		previewWidget.SetVisible(true);
		previewWidget.SetOpacity(1.0);
		previewWidget.SetZOrder(22);
		previewManager.SetPreviewItem(previewWidget, entity, null, true);
		return true;
	}

	protected bool CreateNodePreviewCell(WorkspaceWidget workspace, Widget root, int nodeIndex, int userId, int color)
	{
		ResourceName fallbackIcon = ResolveIconTexture(ResolveNodeIconKey(nodeIndex));
		Widget cell = CreateItemPreviewCell(workspace, root, userId);
		if (!cell)
			return false;

		string prefab = "";
		if (nodeIndex >= 0 && nodeIndex < m_aNodePrefabs.Count())
			prefab = m_aNodePrefabs[nodeIndex];

		if (!prefab.IsEmpty() && SetPreviewCellFromPrefab(cell, prefab, fallbackIcon, color, true))
			return true;

		IEntity itemEntity = null;
		if (nodeIndex >= 0 && nodeIndex < m_aNodeIds.Count())
			itemEntity = ResolveItemIconPreviewEntityForNode(m_aNodeIds[nodeIndex]);

		return SetPreviewCellFromEntity(cell, itemEntity, fallbackIcon, color, itemEntity != null);
	}

	protected bool CreateCandidatePreviewCell(WorkspaceWidget workspace, Widget root, int candidateIndex, int userId, int color)
	{
		ResourceName fallbackIcon = ResolveIconTexture(ResolveCandidateIconKey(candidateIndex));
		Widget cell = CreateItemPreviewCell(workspace, root, userId);
		if (!cell)
			return false;

		string prefab = "";
		if (candidateIndex >= 0 && candidateIndex < m_aCandidatePrefabs.Count())
			prefab = m_aCandidatePrefabs[candidateIndex];

		return SetPreviewCellFromPrefab(cell, prefab, fallbackIcon, color, true);
	}

	protected ResourceName ResolveIconTexture(string key)
	{
		if (key == "clothing_group" || key == "clothing" || key == "headgear" || key == "vest" || key == "pants" || key == "boots" || key == "backpack" || key == "handwear")
			return ICON_CLOTHING;
		if (key == "weapon_group" || key == "weapons" || key == "weapon" || key == "launcher" || key == "sidearm")
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

	protected TextWidget FindRowText(Widget row, string name)
	{
		if (!row)
			return null;

		return TextWidget.Cast(row.FindAnyWidget(name));
	}

	protected ImageWidget FindRowImage(Widget row, string name)
	{
		if (!row)
			return null;

		return ImageWidget.Cast(row.FindAnyWidget(name));
	}

	protected Widget FindRowWidget(Widget row, string name)
	{
		if (!row)
			return null;

		return row.FindAnyWidget(name);
	}

	protected void SetRowChildLayer(Widget row, string widgetName, int zOrder)
	{
		Widget widget = FindRowWidget(row, widgetName);
		if (widget)
			widget.SetZOrder(zOrder);
	}

	protected void PrepareRowRoot(Widget row)
	{
		if (!row)
			return;

		row.SetVisible(true);
		row.SetOpacity(1.0);
		SetRowChildLayer(row, "Body", 0);
		SetRowChildLayer(row, "Background", 0);
		SetRowChildLayer(row, "PreviewBack", 4);
		SetRowChildLayer(row, "PreviewLine", 5);
		SetRowChildLayer(row, "VolumeBack", 4);
		SetRowChildLayer(row, "VolumeFill", 6);
		SetRowChildLayer(row, "PreviewAnchor", 20);
		SetRowChildLayer(row, "Primary", 40);
		SetRowChildLayer(row, "Secondary", 40);
		SetRowChildLayer(row, "Meta", 40);
		SetRowChildLayer(row, "Name", 40);
		SetRowChildLayer(row, "Count", 50);
		SetRowChildLayer(row, "OpenMarker", 50);
	}

	protected void DebugRowCreated(string label, Widget row)
	{
		if (row)
		{
			DebugLog("ui row created " + label);
			return;
		}

		DebugLog("ui row failed to create " + label);
	}

	protected void DebugLoadoutNodeRow(string owner, ResourceName layout, int sampleIndex, Widget row, int nodeIndex, int userId, string role)
	{
		string nodeId = "";
		string parentId = "";
		string kind = "";
		string slotKey = "";
		string label = "";
		string display = "";
		string count = "";
		string prefab = "";
		bool canOpen;
		bool canRemove;
		bool canDeposit;
		bool selected;

		if (nodeIndex >= 0 && nodeIndex < m_aNodeIds.Count())
		{
			nodeId = m_aNodeIds[nodeIndex];
			selected = nodeId == m_sSelectedNodeId || nodeId == m_sSelectedStorageContainerNodeId || nodeId == m_sSelectedStoredItemNodeId;
		}
		if (nodeIndex >= 0 && nodeIndex < m_aNodeParents.Count())
			parentId = m_aNodeParents[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodeKinds.Count())
			kind = m_aNodeKinds[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodeSlotKeys.Count())
			slotKey = m_aNodeSlotKeys[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodeLabels.Count())
			label = m_aNodeLabels[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodeDisplays.Count())
			display = m_aNodeDisplays[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodeCounts.Count())
			count = m_aNodeCounts[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodePrefabs.Count())
			prefab = m_aNodePrefabs[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodeCanOpen.Count())
			canOpen = m_aNodeCanOpen[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodeCanRemove.Count())
			canRemove = m_aNodeCanRemove[nodeIndex];
		if (nodeIndex >= 0 && nodeIndex < m_aNodeCanDeposit.Count())
			canDeposit = m_aNodeCanDeposit[nodeIndex];

		string nodeSummary = string.Format("role=%1 nodeIndex=%2 userId=%3 id=%4 parent=%5 kind=%6 slot=%7", role, nodeIndex, userId, ShortenText(nodeId, 48), ShortenText(parentId, 48), kind, slotKey);
		nodeSummary = nodeSummary + " " + string.Format("label=%1 display=%2 count=%3 prefabSet=%4 selected=%5 canOpen=%6 canRemove=%7 canDeposit=%8 row=%9", ShortenText(label, 48), ShortenText(display, 64), count, !prefab.IsEmpty(), selected, canOpen, canRemove, canDeposit, HST_UIDebug.WidgetSummary(row));
		HST_UIDebug.LogRowSample(owner, layout, sampleIndex, nodeSummary);
	}

	protected void DebugLoadoutCandidateRow(string owner, ResourceName layout, int sampleIndex, Widget row, int candidateIndex, int userId, string role)
	{
		string nodeId = "";
		string display = "";
		string count = "";
		string kind = "";
		string prefab = "";
		bool compatible;
		bool ammoMatch;
		bool currentItem;

		if (candidateIndex >= 0 && candidateIndex < m_aCandidateNodeIds.Count())
			nodeId = m_aCandidateNodeIds[candidateIndex];
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateDisplays.Count())
			display = m_aCandidateDisplays[candidateIndex];
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateCounts.Count())
			count = m_aCandidateCounts[candidateIndex];
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateKinds.Count())
			kind = m_aCandidateKinds[candidateIndex];
		if (candidateIndex >= 0 && candidateIndex < m_aCandidatePrefabs.Count())
			prefab = m_aCandidatePrefabs[candidateIndex];
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateCompatible.Count())
			compatible = m_aCandidateCompatible[candidateIndex];
		if (candidateIndex >= 0 && candidateIndex < m_aCandidateAmmoMatch.Count())
			ammoMatch = m_aCandidateAmmoMatch[candidateIndex];
		currentItem = IsCandidateCurrentNodeItem(candidateIndex);

		string candidateSummary = string.Format("role=%1 candidateIndex=%2 userId=%3 nodeId=%4 display=%5 count=%6 kind=%7 prefabSet=%8 compatible=%9", role, candidateIndex, userId, ShortenText(nodeId, 48), ShortenText(display, 64), count, kind, !prefab.IsEmpty(), compatible);
		candidateSummary = candidateSummary + " " + string.Format("ammoMatch=%1 current=%2 row=%3", ammoMatch, currentItem, HST_UIDebug.WidgetSummary(row));
		HST_UIDebug.LogRowSample(owner, layout, sampleIndex, candidateSummary);
	}

	protected void DebugLoadoutTemplateRow(string owner, ResourceName layout, int sampleIndex, Widget row, int templateIndex, int userId)
	{
		string templateId = "";
		string display = "";
		int slots;
		bool selected;

		if (templateIndex >= 0 && templateIndex < m_aTemplateIds.Count())
		{
			templateId = m_aTemplateIds[templateIndex];
			selected = templateId == m_sSelectedTemplateId;
		}
		if (templateIndex >= 0 && templateIndex < m_aTemplateDisplays.Count())
			display = m_aTemplateDisplays[templateIndex];
		if (templateIndex >= 0 && templateIndex < m_aTemplateSlotCounts.Count())
			slots = m_aTemplateSlotCounts[templateIndex];

		HST_UIDebug.LogRowSample(owner, layout, sampleIndex, string.Format("templateIndex=%1 userId=%2 id=%3 display=%4 slots=%5 selected=%6 row=%7", templateIndex, userId, ShortenText(templateId, 48), ShortenText(display, 64), slots, selected, HST_UIDebug.WidgetSummary(row)));
	}

	protected void SetRowText(Widget row, string widgetName, string text, int color, int fontSize, bool bold, bool wrap = true)
	{
		TextWidget textWidget = FindRowText(row, widgetName);
		if (!textWidget)
			return;

		textWidget.SetVisible(true);
		textWidget.SetOpacity(1.0);
		textWidget.SetText(text);
		textWidget.SetTextWrapping(wrap);
		textWidget.SetExactFontSize(fontSize);
		textWidget.SetLineSpacing(4.0);
		textWidget.SetBold(bold);
		textWidget.SetOutline(1, 0xDD000000);
		textWidget.SetShadow(2, 0xEE000000, 1, 1, 1);
		textWidget.SetColorInt(color);
		textWidget.SetZOrder(40);
	}

	protected void SetRowTextOrHide(Widget row, string widgetName, string text, int color, int fontSize, bool bold, bool wrap = true)
	{
		if (text.IsEmpty())
		{
			SetRowWidgetVisible(row, widgetName, false);
			return;
		}

		SetRowText(row, widgetName, text, color, fontSize, bold, wrap);
	}

	protected void SetRowWidgetVisible(Widget row, string widgetName, bool visible)
	{
		Widget widget = FindRowWidget(row, widgetName);
		if (widget)
			widget.SetVisible(visible);
	}

	protected void SetRowImageColor(Widget row, string widgetName, int color, float opacity = 1.0)
	{
		Widget widget = FindRowWidget(row, widgetName);
		if (!widget)
			return;

		ProgressBarWidget progress = ProgressBarWidget.Cast(widget);
		if (progress)
		{
			progress.SetMin(0.0);
			progress.SetMax(1.0);
			progress.SetCurrent(0.0);
			progress.SetDrawBackground(false);
		}

		ImageWidget image = ImageWidget.Cast(widget);
		if (image)
		{
			image.LoadImageTexture(0, UI_SOLID_WHITE);
			image.SetImage(0);
		}

		widget.SetColorInt(color);
		widget.SetOpacity(opacity);
		widget.SetVisible(true);
		if (widgetName == "VolumeFill")
			widget.SetZOrder(6);
		else if (widgetName == "PreviewLine")
			widget.SetZOrder(5);
		else if (widgetName == "VolumeBack" || widgetName == "PreviewBack")
			widget.SetZOrder(4);
		else
			widget.SetZOrder(0);
	}

	protected void BindRowClick(Widget row, int userId)
	{
		if (!row || userId <= 0)
			return;

		row.SetUserID(userId);
		row.AddHandler(m_WidgetHandler);

		BindRowChildClick(row, "ClickSurface", userId);
		BindRowChildClick(row, "Background", userId);
		BindRowChildClick(row, "Label", userId);
		BindRowChildClick(row, "Value", userId);
		BindRowChildClick(row, "Text", userId);
		BindRowChildClick(row, "Title", userId);
		BindRowChildClick(row, "Primary", userId);
		BindRowChildClick(row, "Secondary", userId);
		BindRowChildClick(row, "Name", userId);
		BindRowChildClick(row, "Count", userId);
		BindRowChildClick(row, "Meta", userId);
		BindRowChildClick(row, "OpenMarker", userId);
		BindRowChildClick(row, "PreviewAnchor", userId);
		BindRowChildClick(row, "VolumeBack", userId);
		BindRowChildClick(row, "VolumeFill", userId);
	}

	protected void BindRowChildClick(Widget row, string childName, int userId)
	{
		Widget child = row.FindAnyWidget(childName);
		if (child)
		{
			child.SetUserID(userId);
			child.AddHandler(m_WidgetHandler);
		}
	}

	protected void SaveLoadoutScrollOffsets()
	{
		float x;
		float y;

		if (m_SlotScroll)
		{
			m_SlotScroll.GetSliderPosPixels(x, y);
			m_fSlotScrollY = y;
		}

		if (m_CandidateScroll)
		{
			m_CandidateScroll.GetSliderPosPixels(x, y);
			m_fCandidateScrollY = y;
		}

		if (m_TemplateScroll)
		{
			m_TemplateScroll.GetSliderPosPixels(x, y);
			m_fTemplateScrollY = y;
		}

		if (m_StorageCandidateScroll)
		{
			m_StorageCandidateScroll.GetSliderPosPixels(x, y);
			m_fStorageCandidateScrollY = y;
		}

		if (m_StorageContainerScroll)
		{
			m_StorageContainerScroll.GetSliderPosPixels(x, y);
			m_fStorageContainerScrollY = y;
		}

		if (m_StorageContentScroll)
		{
			m_StorageContentScroll.GetSliderPosPixels(x, y);
			m_fStorageContentScrollY = y;
		}
	}

	protected void RestoreScrollPixels(ScrollLayoutWidget scroll, float y)
	{
		if (!scroll)
			return;

		scroll.SetSliderPosPixels(0, Math.Max(0.0, y));
	}

	protected void ClearWidgets()
	{
		for (int i = m_aWidgets.Count() - 1; i >= 0; i--)
		{
			Widget widget = m_aWidgets[i];
			if (widget)
				widget.RemoveFromHierarchy();
		}

		m_aWidgets.Clear();
	}

	protected void ShowHint(string text)
	{
		if (text.IsEmpty())
			return;

		HST_NotificationToastController.Get().Show("loadout_" + text, "loadout", "info", "h-istasi loadout", text, 2.0);
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

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId <= 0)
			return false;

		PlayerController controller = PlayerController.Cast(owner);
		return controller && controller.GetPlayerId() == localPlayerId;
	}
}
