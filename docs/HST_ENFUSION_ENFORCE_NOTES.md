# h-istasi Enfusion / Enforce Notes

Purpose: capture reusable facts learned while building h-istasi so we do not rediscover the same Enfusion and Enforce Script edge cases repeatedly.

This file is for practical engine/script behavior, not project planning. Keep entries concrete: what failed, why it failed, what works instead, and where the example lives.

## UI Layouts

- Top-level UI layouts should be created with the workspace as the parent when they are meant to exist above game/map UI:
  - Use `workspace.CreateWidgets(LAYOUT, workspace)`.
  - This matches native UI patterns better than creating a top-level root with no parent.
  - Creation-time geometry can still report `0x0`; schedule a post-layout check before trusting bounds.

- Creation-time widget geometry is often not final.
  - `GetScreenSize` and `GetScreenPos` can return `0x0` immediately after `CreateWidgets`.
  - Use `GetGame().GetCallqueue().CallLater(..., 0, false)` and often a second pass around `50 ms` later for diagnostics or final visibility/z-order reassertion.
  - Current examples: `setup_prompt_ready`, `setup_confirm_modal_ready`, `notification_toast_ready`, `command_action_dialog_ready`, `mission_report_dialog_ready`, `command_menu_ready`, `loadout_editor_ready`.

- Ready diagnostics need to check more than layout creation.
  - A layout can be created and still be unusable because expected children are missing, hidden in hierarchy, or still reporting `0x0` bounds after anchor resolution.
  - Use `HST_UIDebug.LogReadyWidgetsCsv` in delayed ready passes beside geometry logs so playtest logs identify the exact failed widget state.
  - Negative sizes are different from delayed `0x0` layout resolution. Ready logs now separate `zero=`, `negative=`, and `offscreen=` so bad anchor signs are obvious.

- Row/list containers need post-layout child samples when debugging generated UI.
  - Creation-time row logs confirm data population, but list rows can still resolve into the wrong parent, wrong size, or wrong z-order after layout.
  - Use `HST_UIDebug.LogNamedChildSummaryCsv` during delayed ready passes for dynamic list hosts such as command tabs/main/actions and loadout slot/candidate/storage lists.
  - This logs the parent bounds plus the first few child widget bounds after Enfusion has settled anchors.

- Generated row and tile dimensions should live in the row layout resource.
  - Script-side `SizeLayoutWidget` overrides on every generated row reintroduce viewport-dependent geometry logic and can fight wrap/scroll hosts.
  - Prefer fixed `SizeLayoutWidget` dimensions in the row layout and let the parent `VerticalLayoutWidget` or `WrapLayoutWidget` place children.
  - Script should create the row, bind user ids, set colors/text/images, and leave row size to the layout.
  - Current example: `HST_LoadoutCandidateTile.layout` owns storage candidate tile width, height, and text bounds.

- Layout diagnostics must be runtime-gated.
  - Compile-time `static const` feature flags are useful as a hard kill switch, but every high-volume helper should also check the debug setting loaded from runtime settings.
  - Cache the loaded runtime setting in the debug helper so row/widget probes do not parse settings on every row.

- EDDS metafiles must have matching EDDS resources.
  - Runtime symptom: `RESOURCES (E): metafile without corresponding resource`.
  - If an icon is regenerated under a new path, delete any stale `.edds.meta` left behind at the old path; Workbench still scans it and reports the missing resource even when scripts reference the new GUID.
  - Current example: loadout editor search uses `Assets/512/search_icon.edds`; the old `Assets/1254/Search Icon.edds.meta` had no matching `.edds`.

- `root.FindAnyWidget(root.GetName())` may not find the root itself.
  - If debug code checks expected widgets and includes the root name, explicitly compare `root.GetName()` before reporting the root missing.

- A `ButtonWidget` does not accept arbitrary child widgets.
  - Runtime symptom: `GUI (E): Cannot add a child, the ButtonWidget CloseButton does not accept more children`.
  - Avoid putting custom `FrameWidget`/`PanelWidget`/`TextWidget` children inside a `ButtonWidget` unless the engine-provided slot pattern is known to work for that exact widget.
  - Stable workaround: make the button a real sibling hit target and place a sibling `TextWidget` over it with `Ignore Cursor = true`.
  - If a label renders blank while the button hit target still appears, first check whether the label is nested under the button. Move filter/sort/action labels out as siblings before chasing script-side text population.
  - Current examples: `HST_CommandMenu.layout` close button, `HST_SetupConfirmModal.layout` Yes/No labels, `HST_ActionDialog.layout` Cancel/Confirm labels, `HST_ReportDialog.layout` Close label, `HST_LoadoutEditor.layout` remove/filter/sort labels.

- If a layout-owned button does use a supported `ButtonWidgetSlot` child body, that body must fill the button slot.
  - Runtime symptom: the button rectangle is visible and clickable, but child labels report `0x0` or negative height and render blank.
  - Put `HorizontalAlign 3` and `VerticalAlign 3` inside the `Slot ButtonWidgetSlot {}` block for the body frame.
  - Current example: `HST_LoadoutEditor.layout` template/settings button bodies.

- Scripted layout bindings should only name widgets that actually exist in the layout resource.
  - Helpers such as `ConfigureStorageBrowserButton` can fail quietly for optional visual parts because `FindAnyWidget` returns null without a script error.
  - For layout-owned controls, keep hit targets, labels, accents, and other visual siblings named in the layout and validate those names against script calls.
  - If the button itself owns the background color, pass the button name for the background binding instead of inventing an unmatched `*Background` name.
  - Current example: `HST_LoadoutEditorComponent.ConfigureStorageBrowserButton` binds storage filter/sort/search buttons to sibling label/accent widgets in `HST_LoadoutEditor.layout`.

- Icon-only controls need text fallbacks wired from script.
  - A row layout can include a hidden fallback text widget, but it stays blank unless script populates and shows it when image loading fails.
  - Runtime symptom: storage/search tabs or controls appear as visible clickable buttons with no readable label if the icon resource is missing or not ready.
  - Current example: `HST_LoadoutEditorComponent.RenderStorageCategoryTabs()` sets `Fallback` through `GetStorageBrowserCategoryFallback()` when `SetLoadoutImageTexture()` fails.

- Fixed-width generated tab rows must be recalculated when the tab count changes.
  - Runtime symptom: a newly added storage/search tab is populated in logs but does not appear because the row prefab still has the wrong tab count width and padding.
  - For six storage browser tabs in the current right pane, `HST_LoadoutStorageCategoryTab.layout` uses compact tab cells and the host row uses symmetric insets so the tab strip stays centered after removing a category.

- Slot alignment keys must live inside the `Slot ... {}` block for the widget slot that owns them.
  - Runtime symptom: `GUI (E): Unknown keyword/data 'HorizontalAlign'` or `VerticalAlign` while loading the layout resource.
  - Do not put `HorizontalAlign` / `VerticalAlign` directly on a layout widget body such as `HorizontalLayoutWidgetClass`; move them into the slot block or remove them if the slot already stretches.
  - Current example: `HST_LoadoutEditor.layout` `TopTabItems`.

- Full-screen anchored roots should not be manually resized.
  - Avoid `FrameSlot.SetPos(root, 0, 0)` and `FrameSlot.SetSize(root, ...)` on roots using stretched anchors.
  - Runtime warning pattern: position/size only works when min/max anchors are the same.
  - Let layout anchors own full-screen geometry; script may set z-order, visibility, opacity, and text/data.

- Fixed-size centered modal panels should use the same `SizeX` / `SizeY` plus alignment pattern as working native dialogs.
  - Use `Anchor 0.5 0.5 0.5 0.5`, `PositionX 0`, `OffsetLeft 0`, `PositionY 0`, `OffsetTop 0`, `SizeX width`, `OffsetRight -width`, `SizeY height`, `OffsetBottom -height`, and `Alignment 0.5 0.5`.
  - A setup confirmation modal using direct center-relative offsets such as negative left/top and positive right/bottom created the layout and populated text, but the dialog resolved to `0x0` and buttons had unusable or negative bounds.
  - Current examples: `HST_SetupConfirmModal.layout`, `HST_ActionDialog.layout`, `HST_ReportDialog.layout`.

- `OffsetRight` and `OffsetBottom` signs depend on whether a slot has explicit `SizeX` / `SizeY`, whether that axis stretches, and which side the fixed anchor uses.
  - Slots with explicit `PositionX` / `PositionY` plus `SizeX` / `SizeY` often serialize the far edge as negative, such as a centered modal using `SizeX 620` with `OffsetRight -620`.
  - Top/left same-anchor fixed boxes without explicit size need negative far edges, such as `Anchor 0 0 0 0`, `OffsetLeft 116`, and `OffsetRight -560`. Positive far edges produced negative runtime sizes for top tabs, left buttons, left rail, and footer chrome.
  - Top/left same-anchor fixed-height children also need explicit `SizeY` with a negative far edge, such as `OffsetTop 72`, `SizeY 78`, and `OffsetBottom -150`. Positive far edges can collapse tab strips and text bands even when the parent panel is visible.
  - Stretched inset panels need positive far-edge margins, such as `Anchor 0 0 1 1`, `OffsetLeft 240`, and `OffsetRight 524`. Negative far-edge margins made command-menu center and right panels grow underneath sibling panels.
  - Right or bottom anchored fixed boxes can use negative left/top offsets to define size while keeping positive right/bottom offsets inside the parent.
  - Runtime symptom when this is wrong: delayed ready logs show negative widget sizes or panels wider/taller than their parent, such as left rails, navigation panels, top tabs, command-menu center panels, or footer hints resolving to impossible bounds.
  - Current examples: `HST_SetupConfirmModal.layout`, `HST_CommandMenu.layout`, `HST_LoadoutEditor.layout`, loadout row layouts.

- Mode-owned header/action/settings controls need fixed-height slots just like visible chrome.
  - Workbench can show top-anchored header text, action buttons, or settings rows that use positive `OffsetBottom` values, but runtime can hide or collapse them when the mode panel is toggled and repopulated.
  - Use explicit `SizeY` and a negative far edge for candidate headers, remove buttons, and settings rows; keep stretched anchors only for containers that truly fill their parent.
  - Current example: `HST_LoadoutEditor.layout` candidate header/remove controls and settings rows.

- Layout fragments intended to fill a parent slot should use stretched anchors at their root.
  - A root `FrameWidget` with `Anchor 0 0 0 0` can resolve to `0x0` when created inside a dynamically populated placeholder.
  - Use `Anchor 0 0 1 1` for reusable item/preview fragments and let the parent list, tile, or placeholder own the actual dimensions.
  - Current example: `HST_LoadoutItemPreviewCell.layout`.

- Keep visual children passive unless they are real controls.
  - Use `WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS` or layout `Ignore Cursor = true` for passive panels, labels, overlays, and notification visuals.
  - Do not place invisible full-screen widgets unless they are intentional visible modal roots with real content.

- Widget event callbacks can pass a null `Widget`.
  - `ScriptedWidgetEventHandler` methods must guard `!w` before calling `w.GetUserID()` or passing the widget into component logic.
  - Runtime symptom when this is wrong: VM exception in `OnMouseButtonUp` with a null variable named `w`.

- Full-screen render targets need a repeatable sibling layer order.
  - Keep the render target container low in the root z-order and reassert visible UI panels above it after the layout has resolved.
  - Schedule a delayed ready pass for final geometry logs because render-target and UI-layer widgets can report `0x0` during creation.
  - Current example: `HST_LoadoutEditor.layout` with `ApplyLoadoutLayerOrder` and `loadout_editor_ready` diagnostics.

- Complex full-screen shells also benefit from one repeatable layer-order pass.
  - Keep geometry in the layout file, but reassert sibling z-order for dimmers, shell surfaces, panels, headers, and overlaid button labels after creation and delayed layout resolution.
  - Current example: `HST_CommandMenu.layout` with `ApplyCommandMenuLayerOrder` and `command_menu_ready` diagnostics.

- Long-lived screen roots should be reused while open.
  - Recreating a workspace-parented layout root during data refresh can leave input state, scroll state, and delayed layout diagnostics racing against a removed shell.
  - Keep the top-level root cached for screens such as the command menu, then clear only dynamic row/list hosts with `while (container.GetChildren()) container.GetChildren().RemoveFromHierarchy()`.
  - Current example: `HST_CommandMenuComponent.EnsureMenuRoot`, `ClearDynamicMenuRows`, and `ClearMenuContainerChildren`.

- Layout defaults should match the script's first meaningful state.
  - For mode-driven screens, mark always-on chrome explicitly visible and mark inactive mode panels hidden in the layout.
  - Let script show the active mode panel during population instead of relying on a cleanup pass to hide every inactive panel after creation.
  - Current example: `HST_LoadoutEditor.layout` keeps core chrome visible and starts candidate/storage/save panels hidden.

## Coordinates And DPI

- Workspace raw size and layout size are different under UI scaling.
  - Example seen in testing: raw `2560x1440`, layout `1920x1080`.
  - Use `HST_UIWorkspaceMetrics.GetRawWorkspaceSize` only for raw pixel needs.
  - Use `HST_UIWorkspaceMetrics.GetLayoutSize` for layout coordinates.
  - Use `HST_UIWorkspaceMetrics.LayoutToRawPx` / `RawToLayoutPx` for conversions.

- Keep direct `workspace.DPIUnscale` / `workspace.DPIScale` calls inside the metrics helper.
  - Map projection overlays still receive native raw screen coordinates from `SCR_MapEntity.WorldToScreen`, but should route those values through `HST_UIWorkspaceMetrics.RawToLayoutPx`.

- `SCR_MapEntity.WorldToScreen` gives native raw screen coordinates.
  - Convert projected x/y with `HST_UIWorkspaceMetrics.RawToLayoutPx` before `FrameSlot.SetPos`.
  - For world-radius circles, project the center and a world-space edge, derive the raw pixel radius, then convert the final raw size with `HST_UIWorkspaceMetrics.RawToLayoutPx`.

- `SCR_MapEntity.ScreenToWorld` must wait for map readiness.
  - Do not use it before the map is open, `OnMapOpenComplete` has fired, the map has advanced a few frames, and the map widget has non-zero size/valid zoom.
  - This avoids division-by-zero and invalid setup selections.

## Map UI

- `SCR_MapEntity.SetupMapConfig` caches the active map config by `EMapEntityMode`, not by config resource path.
  - If a custom setup map uses `EMapEntityMode.FULLSCREEN`, the next normal fullscreen map can reuse the setup config and keep the setup-only UI component stack.
  - Use a distinct map mode for custom setup maps, such as `PLAIN`, and keep the normal player map on `FULLSCREEN`.
  - Current example: `HST_SetupHQMap.conf` / `HST_SetupMapComponent` use `PLAIN`; `HST_GameplayMap.conf` inherits vanilla fullscreen behavior for the normal map.

- Setup map config and gameplay map config must stay separate.
  - Setup can use a minimal map config with setup selection behavior and optional setup overlays.
  - Gameplay should inherit/preserve vanilla `MapFullscreen.conf` tools and marker UI.
  - Setup-only components must not pollute normal map behavior after HQ placement.

- Campaign markers should use native map marker systems.
  - Let `SCR_MapMarkerManagerComponent` handle projection and pan/zoom.
  - Keep zone radius circles as optional `SCR_MapUIBaseComponent` overlays, not campaign markers.

- Dynamic player markers need a marker config entry before `InsertDynamicMarker` can work.
  - `SCR_MapMarkerManagerComponent.InsertDynamicMarker(type, entity, configId)` resolves `type` through the active `SCR_MapMarkerConfig`; stock `CampaignMapMarkerConfig.conf` does not expose every dynamic marker type.
  - The custom config must be the marker manager config on every gameplay/dev world layer that should show the marker. Having `Configs/Map/HST_PlayerMapMarkerConfig.conf` in the addon is not enough if the active layer still points at a config without `SCR_EMapMarkerType.HST_PLAYER`.
  - If a world layer still points at the vanilla marker config, patch the active `SCR_MapMarkerManagerComponent` before reconciliation with `EnsureHSTMarkerConfig`, loading `Configs/Map/HST_PlayerMapMarkerConfig.conf` and initializing its dynamic entries. The service should retry and log config readiness instead of silently inserting an unconfigured marker type.
  - Server-side config repair is not enough for dynamic marker rendering. `SCR_MapMarkerEntity.OnCreateMarker()` resolves its widget entry on the client from the local marker manager config, so custom player marker entities must also ensure `HST_PlayerMapMarkerConfig.conf` before vanilla widget creation asks for `SCR_EMapMarkerType.HST_PLAYER`.
  - Inherit the custom config from the known `CampaignMapMarkerConfig.conf` parent and copy required HQC squad entries into the HST config. Do not depend on an unverified parent GUID for `CampaignHQCMapMarkerConfig.conf`; a bad parent resource ID makes the whole marker config fail before the HST entry can be used.
  - Do not use `PLACED_CUSTOM` for tracked entities; it is a static marker type. For h-istasi player tracking, the world layers point the marker manager at `Configs/Map/HST_PlayerMapMarkerConfig.conf`, which inherits the campaign marker config and adds `HST_PlayerMapMarkerEntry`.
  - Do not reuse `SCR_EMapMarkerType.DYNAMIC_EXAMPLE` for custom dynamic markers. `SCR_MapMarkerConfig.GetMarkerEntryConfigByType` returns the first matching entry, and the parent map marker config already defines the stock example entry. Add a dedicated modded enum value such as `SCR_EMapMarkerType.HST_PLAYER`.
  - The stock `SCR_MapMarkerEntryDynamicExample` registers its own spawn/death handlers. If h-istasi owns marker lifecycle through a service/reconciler, use a custom `SCR_MapMarkerEntryDynamic` subclass, call `super.InitServerLogic()` to bind the manager, and do not register stock events.
  - Custom marker entry classes used from `.conf` need the same `[BaseContainerProps(), SCR_MapMarkerTitle()]` attributes as stock marker entries; without them the config entry can fail to instantiate even though the script class exists.
  - Runtime symptom: `Unknown class 'HST_PlayerMapMarkerEntry'` while loading `HST_PlayerMapMarkerConfig.conf` means the config can load but the marker entry script class was not registered/compiled for that run, so `InsertDynamicMarker` will not have a usable `HST_PLAYER` entry.
  - Validate both the imageset resource and the quad name. `{2EFEA2AF1F38E7F0}UI/Textures/Icons/icons_wrapperUI-64.imageset` has `circle` but not `dot`; using a missing quad can leave an otherwise created marker visually blank.
  - Current h-istasi player marker art uses the `whisper` quad from `{3262679C50EF4F01}UI/Textures/Icons/icons_wrapperUI.imageset`; keep the dynamic marker layout passive with `Ignore Cursor` so it cannot steal map interaction.
  - `SCR_MapMarkerEntity.SetText()` is not replicated. If server-created dynamic markers need per-player labels, pass a stable value through replicated marker data such as `configId` and resolve the client-side label in `InitClientSettingsDynamic`. Replicated marker fields can arrive around widget creation, so reapply labels briefly from the callqueue if the first pass only has fallback text.
  - Reconciler dynamic cleanup must remove tracked domain ids even if the stored `SCR_MapMarkerEntity` pointer is already null. A stale dynamic handle can poison tracked counts and prevent player marker cleanup/recreate from converging.
  - Matching tracked dynamic handle counts are not enough to skip player marker reconciliation. Check that tracked `SCR_MapMarkerEntity` references are still present in `SCR_MapMarkerManagerComponent.GetDynamicMarkers()`, and recreate handles that have unregistered from the native manager.
  - Player markers use a separate reconciler from campaign/HQ/zone markers. Include `HST_PlayerMapMarkerService.BuildRuntimeReport()` in native marker admin reports so tests can see desired player records, tracked handles, live native handles, native dynamic marker count, marker-config entry readiness, and last reconcile failures in one place.
  - Admin native-marker purge/recovery commands must clear both the campaign marker reconciler and the separate player marker reconciler. If player markers are enabled, queue a player marker refresh after purge so live player handles are recreated from current controlled entities.
  - `SCR_PlayerNamesFilterCache` can lag marker widget creation and may return shortened display text. Prefer `PlayerManager.GetPlayerName(playerId)` when it is available, then fall back to the filter cache, and retry for a few seconds before accepting a generic `Player X` label.
  - If `HST_PlayerMapMarkerService` logs `native reconcile failed`, first verify the active world layer `SCR_MapMarkerManagerComponent.m_sMarkerCfgPath`, the custom marker entry config, and the dedicated `SCR_EMapMarkerType.HST_PLAYER` enum.
  - For custom dynamic visuals with client-resolved image/text, do not rely on `SCR_MapMarkerEntryDynamic.InitClientSettingsDynamic()` because base marker text/imageset are not replicated for server-created dynamic markers. Set image/color/text directly in the custom entry and force a visible alpha on faction colors.
  - Dynamic marker config entries must name a layout with `SCR_MapMarkerDynamicWComponent` or a subclass. The default `SCR_MapMarkerEntryConfig` layout is `MapMarkerBase.layout`, and `MapMarkerSquadMember.layout` uses `SCR_MapMarkerSquadMemberComponent`; both lack the dynamic handler that `SCR_MapMarkerEntity.OnCreateMarker()` dereferences on map open.
  - Native dynamic markers follow movement through their target entity. `SCR_MapMarkerEntity.SetTarget()` enables frame updates, and authority-side `EOnFrame()` copies `m_Target.GetOrigin()` into the replicated marker position, so player markers should not be recreated just to follow walking/driving movement.
  - The current `whisper` player-marker icon needs a forward-facing rotation offset of `-50` degrees before applying player yaw to line the icon up with the in-game facing direction.
  - Server-created dynamic marker prefabs need exactly one valid replication component. For h-istasi player markers, prefer a standalone `SCR_MapMarkerEntity` prefab with one local non-spatial/non-streamable `RplComponent`, matching the native squad marker shape. Do not inherit `MapMarkerEntityBase.et` and then add another `RplComponent`; the duplicate-component warning (`SCR_MapMarkerEntity component RplComponent cannot be combined with component RplComponent`) can leave `FindComponent(BaseRplComponent)` null during marker init and make player markers reconcile but fail to render.
  - Vanilla `SCR_MapMarkerEntity.EOnInit()` assumes `FindComponent(BaseRplComponent)` succeeds. If a dynamic marker prefab/resource state can produce a marker entity without a resolved RPL component, guard the modded init path before calling `IsOwner()` or `InsertDynamicMarker` can throw `NULL pointer to instance. Variable 'rplComp'`. Keep the marker entity active when RPL is temporarily unavailable; `SetTarget()` only sets the frame event mask, while active frame updates are what copy the target position into the replicated marker position.
  - Player marker facing rotation lives in the marker widget component, not the marker service. `HST_PlayerMapMarkerDynamicWComponent` rotates only `MarkerIcon` from the player entity's map yaw, leaving `MarkerText` upright and avoiding extra server marker churn. The `whisper` icon art points roughly 50 degrees off its widget zero after in-game alignment, so apply the icon forward offset when converting yaw to widget rotation.
  - Current working player-marker layout: `{6985327711306214}UI/layouts/HST/Map/HST_PlayerMapMarkerDynamic.layout`.
  - Custom player marker entries that set a name must also call `SetTextVisible(true)`.
  - Player markers should be gated to the active campaign phase. Setup bootstrap player entities are not gameplay map markers and can pollute the setup map or create stale dynamic handles before HQ setup is finalized.
  - For faction-colored player markers, resolve `SCR_FactionManager.SGetPlayerFaction(playerId)` client-side and fall back to the FIA faction color before using a hardcoded color.
  - Player marker reconcile signatures must include the resolved player/entity faction key, not just player id, target entity, and name. Faction assignment can arrive after the marker is created, and skipping reconciliation on an unchanged entity can leave the native marker with stale stream rules.
  - Do not hard-code the native marker stream faction to FIA. Use `SCR_FactionManager.SGetPlayerFaction(playerId)` first, then the controlled entity's `FactionAffiliationComponent`, and leave the stream key empty if neither is available so a bad fallback does not hide the player's own marker.
  - HST player markers should call `SCR_MapMarkerEntity.SetFaction()` once a real player/entity faction is known so native map faction visibility can include them. Keep the widget icon/text color client-resolved from the same player faction, and avoid hardcoded fallback stream factions that can hide hosted-server markers.
  - `SCR_MapMarkersUI.CreateDynamicMarkers()` can call `SCR_MapMarkerEntity.OnCreateMarker()` before the map frame is actually usable. A log before `super.OnCreateMarker()` only proves creation was requested; the reliable success signal is a non-null marker root/widget component or the custom marker entry's `InitClientSettingsDynamic()` log.
  - Gameplay-map open repair should retry until HST dynamic player marker entities exist and their widgets are created. Treat `0` dynamic HST player markers as not-ready for a few retries, because the replicated marker entity can arrive after `OnMapOpenComplete`.
  - If a cached dynamic marker widget root is detached from the current map hierarchy, clear the root and widget component before rebuilding. Native dynamic marker deletion/removal paths can leave a stale reference even though the visible map root has been destroyed.
  - Current examples: `HST_PlayerMapMarkerService`, `HST_PlayerMapMarkerEntry`, `HST_PlayerMapMarkerConfig.conf`.

- Map overlay widgets must be passive.
  - Create overlay primitives with `IGNORE_CURSOR` / `NOFOCUS`.
  - Redraw only when content or viewport state changes enough to matter.
  - Do not redraw on every frame while idle.

- Keep map overlay redraw ownership in the map UI component.
  - Setup/map-flow components should publish content changes, such as setup zones or the temporary HQ candidate marker.
  - The `SCR_MapUIBaseComponent` overlay should own projection, revision checks, pan/zoom thresholds, and widget reuse.
  - Avoid a second dirty/redraw loop in the flow component; duplicate redraw ownership makes readiness and input bugs harder to isolate.

## Input And Widget Events

- Real controls should be widget-driven.
  - Give interactive widgets stable `UserID`s and attach a `ScriptedWidgetEventHandler`.
  - Avoid raw coordinate hit testing for visible buttons.
  - When a render path programmatically syncs an `EditBoxWidget` with `SetText`, guard the paired `OnChange` handler so the sync cannot re-enter the render path. Attach the handler after the sync where practical, but still use an explicit guard because the reused widget may already have a handler.

- Some interactions may arrive through both `OnClick` and `OnMouseButtonUp`.
  - Prefer a single activation owner for normal buttons, usually `OnClick`.
  - Use duplicate-activation guards only where a widget action can unavoidably be triggered by both callbacks in the same frame.
  - Pass the mouse button through both event paths and key the guard by widget id, button, and frame serial so right-click/left-click actions remain distinct.
  - Current examples for guards include `HST_SetupMapComponent`, `HST_CommandMenuComponent`, and `HST_MissionClientComponent`; `HST_LoadoutEditorComponent` keeps normal button commands on `OnClick` and uses mouse-up only for preview drag release.

- Avoid synchronously deleting and rebuilding a newly added control subtree from that same control's callback.
  - Existing long-lived menus may tolerate direct `RenderEditor()` calls from established row buttons, but new filter/sort/search controls are safer when they mutate state and queue a zero-delay callqueue render.
  - This keeps the handler return path from walking widgets that were just destroyed by the render.
  - Current example: `HST_LoadoutEditorComponent.QueueStorageBrowserRender()` for storage filter/sort/search controls.
  - The storage browser tab list is the authority for server-side candidate categories and client search. When a tab is removed, narrow both `HST_LoadoutEditorService.IsStorageBrowserCandidateCategory()` and client search/category helpers so hidden wearable/container categories cannot be returned through search.
  - `RenderTargetWidget.SetClearColor(true, color)` is needed for loadout preview background presets because the render target can remain opaque over the UI backdrop; changing only the backdrop or a sky-sphere material may not affect the visible clear/background color.
  - Keep storage search scroll state separate from the normal storage candidate grid scroll. Search query/results intentionally persist while the editor stays open, but search rerenders should restore `m_fStorageSearchScrollY` rather than inheriting the category grid's `m_fStorageCandidateScrollY`.

- Modal state belongs in one coordinator.
  - `HST_UIRootService` tracks current screen, modal screen, blocking behavior, and topmost owner.
  - Repeated identical `RequestOpen` calls should be idempotent; otherwise refresh loops create noisy revision churn.
  - Use distinct screen modes for distinct modal families. Action confirmations register as `ACTION_DIALOG`; mission report/details dialogs register as `MISSION_DIALOG`.

- Screen input and modal input need separate ownership gates.
  - A full-screen UI should only process keyboard, preview drag, and widget clicks when `HST_UIRootService.CanHandleScreenInput(mode, owner)` matches that screen and no modal is above it.
  - A modal should process its buttons through `CanHandleModalInput(mode, owner)` instead of checking the underlying screen's open flag.
  - This matters when a contextual command opens only a confirmation modal while the command menu shell is closed; the modal still owns Yes/No input, but the closed screen should not receive normal menu navigation.
  - Current examples: command action dialogs, mission report close, and loadout editor preview drag/tab/back input.

- Shared confirmation rules need to cover both row clicks and contextual commands.
  - Command-menu row activation and contextual user actions can reach the same campaign mutation through different methods.
  - Put the confirmation decision in the destination UI component, not in a specific row factory, so actions such as HQ relocation and mission-objective completion cannot bypass the shared modal when triggered from world-space user actions.
  - If a contextual action opens only the confirmation modal while the command menu is closed, confirmed/cancelled callbacks must not call the full command-menu render path unless the menu is actually open.
  - Current example: `HST_CommandMenuComponent.ShouldConfirmAction`, `RunCommandFromContext`, and `RequestConfirmedAction`.

- A modal over a native map needs cursor handling split between the UI root and the map selection mode.
  - Do not activate `DialogContext` / `InteractableDialogContext` over the map just to make buttons clickable; those contexts can create an extra OS-style pointer over the map cursor.
  - Disable the current map selection mode directly, for example `ToggleLocationSelection(false)`, while the modal is waiting for an answer.
  - Use `SCR_MapCursorModule.HandleDialog(true)` / `HandleDialog(false)` for map-owned dialog state so the native map cursor is allowed to travel above the modal and selection/drag cursor modes are suppressed.
  - The native custom map cursor lives in its own workspace cursor layout at z-order `10`. Keep setup-map modal chrome above the map root but below that cursor band, for example modal root/dimmer/dialog/buttons/text in `1..9`, when the map cursor must remain visible over confirmation buttons.
  - Do not force `WidgetManager.SetCursor(0)` over an active map cursor; the native map cursor already hides the real cursor and forcing it back creates a second pointer.
  - Avoid modal-owned cursor proxy widgets over native map dialogs. They are easy to layer above the dialog, but they create a third visible cursor and drift from the engine cursor lifecycle.
  - Current example: `HST_SetupMapComponent`.

- Notifications should not participate in blocking input.
  - Keep toast roots and children cursor-ignored and no-focus.
  - Track notification depth separately from current/modal screens.

## Enforce Script Practicalities

- `string.Format` placeholders are limited to `%1` through `%9`.
  - Do not use `%10` or higher in diagnostics or reports; split the report into multiple `string.Format` calls or append the remaining values with string concatenation.

- Prefer explicit boolean checks for object references when returning a bool.
  - `if (widget) return true;` is clearer and safer than returning widget-reference expressions from `bool` methods.

- `CallLater` supports delayed UI repair/diagnostic passes and can accept arguments.
  - Existing examples with arguments include notification dismissal generation checks and preview entity finalization.

- Reused layout roots need container cleanup, not just tracked-widget cleanup.
  - If generated rows are created into layout-owned containers and the root is reused, remove tracked dynamic widgets and then remove all children from the dynamic containers before repopulating.
  - Hidden or parent-hidden generated widgets can survive a mode switch if cleanup only clears visible children. Remove tracked dynamic widgets regardless of current visibility before the container sweep.
  - Reused static widgets should not call `AddHandler(m_WidgetHandler)` on every render. Route editor-owned bindings through an idempotent helper that checks `FindHandler(EditorWidgetHandlerType)` before adding the handler, or repeated renders can stack callbacks on filter/sort/close buttons.
  - `FrameWidgetSlot` does not accept `HorizontalAlign` / `VerticalAlign`. Keep those keys inside `AlignableSlot` or `OverlayWidgetSlot`; if the parser reports unknown alignment keywords at layout load time, inspect the nearest generated `FrameWidgetSlot` first.
  - When script populates text into layout-owned controls, set a known font in the common text setter. Newly added `TextWidget` labels without `FontProperties` can otherwise appear as blank clickable buttons even though the widgets exist.
  - Do not route normal button activation through both `OnClick` and `OnMouseButtonUp`. Use mouse-up only for drag state that explicitly consumes the event; otherwise return false and let `OnClick` perform the command once.
  - For layout-owned buttons whose label is a sibling `TextWidget`, validate and log both the button binding and the label widget summary when debugging blank labels. A visible/clickable button with a missing, hidden, zero-sized, or behind-parent label otherwise looks like a script click problem in the runtime log.

- Keep live inventory payloads single-owner.
  - If a server payload emits real storage contents as storage-node children, do not also re-emit those same entries as synthetic top-level inventory rows.
  - A simple guard is to treat slots with `m_sSlotKind == "storage_item"` or a non-empty parent slot id as nested storage rows only.
  - Clearing only an array of widgets misses rows that were not inserted into that array and causes tabs/lists to accumulate stale entries.
  - Hidden layout regions may not be reliably found by a global cleanup pass, depending on visibility and resolution timing.
  - Clear the active dynamic container again after the region has been resolved and shown, then populate it.
  - Treat the selected loadout edit target as a single resolved context, not as separate slot-only and node-only states. A layout row may leave only a selected slot id while candidate payloads and remove commands still need the live node id.
  - Invalidate compatible-item candidate payloads after loadout or storage mutations. Arsenal counts, storage capacity, and compatibility can change immediately after inserting/removing an item, so a cached empty candidate result can become stale.
  - Storage candidate troubleshooting needs separate counts for recovered arsenal items, slot/category matches, and live inventory compatibility matches. A category can have recovered items and still return an empty visible panel because the selected storage has no live insert target, no free capacity, or no compatible slots after the last insertion.
  - Storage candidate payloads should include matching recovered arsenal items even when the selected storage cannot currently accept them, with the `compatible` field carrying the fit result. Keep non-storage candidate lists server-filtered to compatible items only.
  - Magazine ammo usability can be computed independently from storage fit. This lets the UI show an "ammo usable" filter even when the item cannot fit in the selected container.
  - Storage capacity display should mirror `SCR_InventoryStorageBaseUI`: direct inventory storages can use `GetOccupiedSpace()` / `GetMaxVolumeCapacity()`, but `ClothNodeStorageComponent` capacity must be summed from owned `SCR_UniversalInventoryStorageComponent` instances. The cloth-node component itself can report misleading full/empty percentages.
  - Storage browser filter/sort state belongs in the local visual settings file, not the campaign state. Defaults should be fit-only and A-Z, with server authority still enforced by `add_storage_item`.
  - The storage ammo filter is tab-contextual. Keep its saved bit when leaving the ammunition tab, but only apply and show it active while `m_sSelectedCategory == "magazine"` so other tabs do not appear filtered by ammo.
  - Name sort direction and count sort direction are separate settings. When count sorting is selected, use count as the primary order and A-Z/Z-A as the tie-breaker; this lets the UI show `A-Z`/`Z-A` and `INF-1`/`1-INF` together.
  - Non-fitting storage candidates should keep their arsenal count badge. Use a light red row highlight for the fit failure and let the server-authoritative `add_storage_item` path reject clicks that still do not fit.
  - Structural pouch/holster/bandolier/carrier/scabbard/sheath/e-tool carrier or case-style storage attachments should not be emitted as addable storage-browser candidates. Check prefab, display, and short display names before spawning compatibility probes, because Reforger item paths and localized display names can expose different structural-container clues. These attachments can be child containers on looted webbing, but the player should add/remove the parent webbing or magazines inside those child pouches, not the pouch prefab itself.
  - Block structural inventory containers at deposit time and purge them from saved arsenal state. The current common bucket is non-loadout-clothing entities with structural attachment/cargo storage plus display/prefab tokens such as personal belongings, pouch, holster, bandolier/bandoleer, scabbard, sheath, compass case, suspenders, and e-tool/entrenching-tool carriers. Do not block real wearable backpacks/field packs or loadout cloth components just because they have cargo storage.
  - Structural wearable/storage prefab tokens are inconsistent. Treat `fieldpack`, `field_pack`, `field pack`, `ruck*`, and ALICE pack names as backpack/wearable storage aliases, while blocking `pouch`, e-tool `carrier`/`case`/`holder`/`cover` qualifiers, and similar structural child containers from arsenal storage candidates. Do not block the actual equippable entrenching tool just because its name contains `etool` or `entrenching tool`.
  - Run structural child-container token checks before source-category fallback in the loadout editor. If an arsenal entry arrives as `backpack`, `vest`, or generic equipment but its prefab/display says `buttpack`, `pouch`, `scabbard`, `compass case`, or e-tool carrier/case/cover, classify it as utility and let the structural filter block it instead of letting the source category resurrect it as wearable gear.
  - Broad `carrier` text is not enough by itself to block an item; real wearable armor carriers exist. Treat carrier as structural only with stronger context such as entrenching-tool, compass, radio, pouch, or case-style tokens.
  - Area-loot classification needs the same wearable/container resolver as the loadout editor. Worn BDU blouses, ALICE webbing, vests, pants, and backpacks can have cargo storage and still be real loot; classify them as loadout clothing before the structural cargo-storage probe. Explicit structural tokens such as pouches, suspenders, compass cases, scabbards, and e-tool carriers still win before wearable exemptions.
  - Storage content rows should gather from every insert-capable storage under the selected worn container, then filter out structural container shell entities before grouping rows. This lets magazines/flashlights inserted into webbing child pouches show in the bottom-left contents without showing the pouch/carrier as an add/remove item.
  - For worn webbing such as ALICE, insert-capable storage means the real cargo/deposit storages reached by recursing into child pouches, not the parent `ClothNodeStorageComponent` or structural attachment slots themselves. Including structural attachment storages in the insert/capacity list can inflate percentages, target pouch shells, and reject otherwise valid cargo inconsistently.
  - Treat paper map item prefabs as arsenal-equivalent across factions. `PaperMap_01_folded_US`, `PaperMap_01_folded_USSR`, `PaperMap_01_folded_FIA`, and the generic folded map should share arsenal lookup, issued-loadout lookup, saved-loadout cost grouping, and refund/withdraw accounting.
  - Keep client search filtering and server candidate/deposit filtering on the same shared token helper (`HST_ArsenalItemFilter.HasBlockedStructuralContainerToken`). A separate client-only list can miss cases such as compass cases or suspenders and show entries the server will reject.
  - Explicit blocked structural tokens must run before any broad wearable exemption. After pouch/carrier/case-style tokens are rejected, `BaseLoadoutClothComponent` is a valid wearable signal because some real clothing/backpack/webbing items arrive from loot classification as generic equipment or utility.
  - Bandages/field dressings use medicine-prefab paths and may not include the word `Bandage` in the prefab. Classify `Medicine`, `FieldDressing`, `FirstAid`, `Bandage`, `Morphine`, `Tourniquet`, and `Medkit` tokens as `medical` before falling back to generic equipment.
  - Storage-browser candidate preview keys must ignore `live_storage_item_` draft slots. Changing the contents of the selected container should refresh inventory lists and capacity bars, but it should not rebuild/refocus the rendered entity unless the displayed container/gear actually changes.
  - Wrap-layout storage grids need children whose root slot is `WrapLayoutWidgetSlot`, and the tile's inner `SizeLayout` must not horizontally fill the parent. Reusing a vertical-row layout or a fill-aligned size wrapper can collapse the right-hand storage browser to a single column even when a fixed tile width is set from script.
  - If a wrap-grid child is a clickable `ButtonWidget`, also keep a `SizeLayoutWidget` child with explicit width/height and enforce those overrides when creating rows. Otherwise the button root can stretch to the scroll width and visually behave like a single-column list even under a `WrapLayoutWidget`.
  - Do not use `LayoutSlot` manipulation helpers on a widget whose root slot is `WrapLayoutWidgetSlot`; the engine logs an invalid-slot error and click handlers can crash. Resize storage-browser tiles through the inner `SizeLayoutWidget` width/min/max overrides instead.
  - Two-column storage browser tiles need to derive width from the active right pane, not a single global constant. Reserve room for panel margins, tile padding, and the scrollbar, then clamp to a practical min/max so narrow panes keep two columns and wide panes use the available text area.
  - Worn clothing swapped through live inventory can expose useful cargo through `BaseInventoryStorageComponent.GetOwnedStorages()` even when the direct clothing storage has no insertable slots. Discover owned cargo/deposit storages before skipping zero-slot parent storage components, or BDU blouses, ALICE webbing, and similar swapped clothing can disappear from the storage tab while still showing capacity in the vanilla inventory UI.
  - Some worn clothing/webbing cargo storages live on child entities under the equipped item rather than on the equipped entity's own storage component or owned-storage list. Bounded child-entity recursion is needed when collecting cargo/deposit storages for the storage tab.
  - Some swapped clothing storage is volume-only from the loadout editor's perspective. Treat a storage with `GetMaxVolumeCapacity() > 0` as capacity-bearing even if `GetSlotsCount() == 0`, then recurse owned/deposit storages for actual insert targets and contents.
  - Empty live loadout slot compatibility sometimes needs a category fallback before exact `LoadoutAreaType` equality. Looted backpacks/field packs can be correctly classified as `backpack` in the arsenal while the native slot-area type comparison still rejects them, so validate the live node category first for loadout clothing categories.
  - Keep armored vests and load-bearing webbing as distinct editor categories. PASGT/armor vests should label as `Armored Vest`; ALICE/LBE/chest-rig/grenadier-style webbing should label as `Chest Rig`. Run backpack alias detection before webbing detection so ALICE packs/field packs remain backpack candidates.

- Loadout editor saved loadouts:
  - Fixed save slots are server-owned campaign state and profile persistence. Passing `slot_N` to `loadout_save` selects the slot; it must not overwrite an already renamed display name unless the slot was empty or an explicit rename command was sent.
  - Use `JsonSaveContext.SaveToString()` and `JsonLoadContext.LoadFromString()` with `SCR_PlayerArsenalLoadout.ReadLoadoutString/ApplyLoadoutString`. Obsolete `SCR_Json*` helpers can compile with warnings or fail to round-trip current native loadout data.
  - Serialized native saved loadouts include attachment and child-slot metadata that may not be present as h-istasi recovered arsenal entries. For serialized slots, skip attachment metadata and other missing-arsenal rows in the cost ledger instead of failing before `SCR_PlayerArsenalLoadout.ApplyLoadoutString()` can apply the saved native string.
  - After a successful serialized native loadout apply, refresh the server-side `SCR_PlayerController` main entity with `SetInitialMainEntity(playerEntity)`. h-istasi applies in place, but the controller refresh still helps possession/UI systems observe the changed entity state.
  - Save/load/reset feedback should be a short-lived in-editor toast, not a global notification. Use a generation token when scheduling delayed clears so an older timer cannot erase a newer prompt. Disabled `Load` buttons should stay visible for empty fixed slots.
  - Loadout candidate icon hints also drive native row previews. Use preview-capable hints such as `medical`, `utility`, and `equipment`; a generic value that the client does not recognize as preview-capable will render as a flat fallback icon even when the prefab can be previewed.
  - Fixed saved loadout slots should be emitted in deterministic slot order from the service payload, including empty slots. A per-slot row can then show both `Save` and `Load`; keep `Load` visible but do not bind a user id when the slot is empty.
  - Per-slot save buttons should send the fixed loadout id such as `slot_0` through the existing `loadout_save` request. The service can treat that argument as a slot target while preserving the old global save behavior for empty arguments.
  - Saved loadout rename/load must stay server-routed through the coordinator. The client can edit a non-empty slot name inline, but the mutation should send `loadout_rename` with `slot_id:name`; load should fail visibly if the serialized native loadout cannot be parsed or applied instead of reporting success.
  - If a button icon is intentionally suppressed in script, also remove the layout fallback text and let the label span the full button. Hidden dynamic widgets can still be visible for a frame during layout debug/initial population, so stale symbols such as the candidate Edit `>` can flash or reserve dead spacing.
  - Loadout editor search should use the full recovered arsenal item arrays (`m_aItem*`), not the selected storage candidate arrays (`m_aCandidate*`). The search result click should send `add_storage_item` with the selected storage node id and prefab; if no storage is selected, show a status and do not issue the request.
  - Storage search category matching should include the raw item category, the slot category label, and the visible storage browser tab label. Grouped tabs such as Weapons and Clothing otherwise will not match items whose raw categories are `weapon`, `launcher`, `headgear`, `vest`, etc.
  - Numeric or short storage searches should match normalized display/category text, not prefab GUID/path noise. Only include prefab paths in search for explicit path-like queries; otherwise strings such as `556` can match unrelated items through resource identifiers instead of user-visible names.
  - Edit-box search inputs can be handled through `ScriptedWidgetEventHandler.OnChange(Widget w, bool finished)` after assigning a stable user id and adding the handler to the `EditBoxWidget`. Remember to clear layout-owned dynamic result containers such as `StorageSearchItems` when reusing the editor root.
  - If an edit-box render path rebuilds dynamic siblings on each query change, restore focus with `WorkspaceWidget.SetFocusedWidget(input, true)` after syncing text and attaching the handler, or continuous typing can stall after the first refresh.

- Dynamic horizontal tabs should be centered by sizing the layout-owned host to the children, not by moving each tab.
  - A tab button with a fixed width and layout padding contributes both to the desired strip width.
  - If the host is wider than the sum of the tab buttons plus their padding, Enfusion lays children from the left and leaves the extra space at the end.
  - Current example: the loadout editor mode tabs use symmetric host insets so the six layout-generated tab buttons sit centered over the left pane.
  - Fixed-size preview boxes inside the candidate header need the same negative far-edge rule as other top/left same-anchor boxes. Positive `OffsetRight` / `OffsetBottom` values produced negative runtime size for the header preview anchor, which made the current equipped item preview disappear.

- `ItemPreviewWidget` does not reliably render every inventory prefab.
  - Keep a category/fallback icon available when prefab or entity preview setup fails.
  - Do not hide the fallback icon just because the entity preview lookup returned null for a non-empty prefab.
  - Empty live equipment nodes still need a visible fallback tile plus `Empty` text; suppressing the fallback makes the header look like a missing render instead of an empty slot.
  - Attachment nodes should resolve their preview icon key as `attachment`, not as slot keys such as `optic` or `muzzle`. Slot keys are useful labels, but they are not in the native-preview eligibility list and will force an equipped attachment preview down the fallback-only path.
  - Attachment slot scans should only treat an attached entity as editable if it has `InventoryItemComponent`. Prefab child props without inventory components, such as structural handguards or fixture meshes, can exist under a weapon slot but should render as empty/non-removable in the loadout editor.
  - Worn clothing Edit means cosmetic/functional attachment-slot editing, not cargo storage editing. Route the equipped clothing item into the existing attachment child-node view; keep cargo insertion/removal in the storage tab.
  - Clothing Edit needs an internal state distinct from weapon attachment editing, while mapping the visible top tab back to Clothing. This prevents Back/click behavior from leaving the player in weapon attachment mode after editing clothing child slots.
  - Clothing attachment discovery should follow broad slot validation: an `AttachmentSlotComponent` with a real attachment slot type is inspectable even if `ShouldShowInInspection()` is false. Recurse child and owned attachment storages, but skip `SCR_UniversalInventoryStorageComponent` cargo storages so inventory pouches do not become cosmetic edit slots.
  - Worn clothing cosmetic slots can be `LoadoutSlotInfo` entries on `ClothNodeStorageComponent`, not only `AttachmentSlotComponent` entries. Treat those loadout child slots as edit nodes for clothing Edit, while continuing to filter structural pouch/scabbard/carrier slots out of add/remove candidate lists.
  - Clothing child-slot candidates should mirror native loadout slot matching: compare the child `LoadoutSlotInfo.GetAreaType().Type().ToString()` with the candidate prefab entity's `BaseLoadoutClothComponent.GetAreaType().Type().ToString()`, then still run the live `CanStoreItem`/`CanReplaceItem` check. Broad arsenal categories are too loose for these slots and can admit unrelated maps, full clothing, or utility items.
  - Spawned clothing/webbing prefabs can carry default stored items inside structural equipment slots. Clean default bayonet contents from those structural slots before insertion or preview, but do not delete the scabbard/carrier shell because it is part of the worn item appearance.
  - When editing clothing attachment children, keep the equipped parent row non-clickable. Clicking the parent while already in its child-node view should not exit into a replacement candidate list.

- Preview camera movement should be intentional.
- Auto-frame the loadout editor preview once when opening or rebuilding the preview world.
- The visible color behind the mannequin comes from the preview stage `MatSkyBox`/`SkyPreset`, not just the `RenderTargetWidget` tint or the decorative sky sphere. Render-background color settings should update the spawned `GenericWorldEntity.GetSkyMaterial()` colors or rebuild the stage with an alternate sky preset.
- Storage selection and storage item add/remove should not invalidate the preview render key or move the camera unless the visible equipped entity actually changes appearance.
- Do not include selected storage-item ids in the preview render key; doing so forces unnecessary clone rebuilds and camera motion while adding repeated inventory contents.
- Non-storage item/node selection should reset the preview camera to the default auto-frame path and use a closer entity framing distance, so choosing a weapon/attachment/clothing edit context does not leave the camera at the previous full-character view. Keep this separate from storage add/remove operations to avoid motion while filling inventories.

- Native input hints should use the widget-library input button components when possible.
  - `SCR_InputButtonComponent` can bind an action name and label to the current input device.
  - Pass configured input action names to `SetAction`, not literal keys. The component resolves the current keyboard, mouse, or controller glyph.
  - Keep generated hint widgets passive with `WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS` when they are only visual footer/context hints.
  - Hand-drawn key boxes are useful as fallbacks, but they do not update like native button hints.
  - If a persistent footer or row container is repopulated, clear its child widgets before adding native hint widgets; stale children remain visible even when the backing arrays were cleared.

- Avoid clearing action keys during UI teardown unless that UI will continue owning input for the next frame.
  - `Debug.ClearKey(KeyCode.KC_I)` is acceptable while setup is actively blocking command menu input, but clearing it after setup finalization can consume the first legitimate command-menu open press.
  - Prefer resetting the destination UI input latch and rebinding its input context after a modal/setup screen closes.
  - If a raw key bridge is needed for the destination UI, clear the raw key only after the destination UI actually handles a toggle. A post-setup recovery method can run on the same frame as the user's first real press and eat that press if it calls `Debug.ClearKey` speculatively.

- `ItemPreviewWidget` setup is not proof that a prefab will render useful pixels.
  - Put a category/fallback image behind the native preview widget and keep it visible at low opacity after calling `SetPreviewItemFromPrefab` or `SetPreviewItem`.
  - This preserves a visible row affordance for unsupported prefabs without adding script geometry or hiding native previews that do render.
  - Do not hard-exclude worn equipment categories from native previews. Clothing, storage containers, and weapons should attempt native preview first with the fallback underlay still present, because a visible character item may have a valid preview prefab even if some inventory entries still fall back.
  - For equipped items and live inventory contents, prefer entity previews before prefab previews. Clear the preview widget, then call `SetPreviewItem(previewWidget, entity)`.
  - For arsenal candidates that only have prefab resources, call `SetPreviewItemFromPrefab(previewWidget, prefabResource)` and leave the fallback underlay visible.

- Slot edit remove actions must be independent from compatible replacement candidates.
  - A slot can have a removable equipped item even when the arsenal has no valid replacement entries.
  - Gate the UI remove control on the selected item existing, then let the server-side inventory path reject only truly invalid removals.
  - Header text for an edit context should show the slot/category label and equipped item display name separately; otherwise an equipped item can look like an anonymous preview tile.

- Reused mode panels should hide stale child content on mode reset and re-show the parent before populating.
  - A child such as settings content can remain marked visible while its parent panel is hidden, which makes logs look populated but the screen appear blank.
  - Reset both the parent region and mutually exclusive child panels before rendering the active mode.

- Keep code comments sparse and practical.
  - Comments should capture non-obvious engine constraints, not restate simple assignments.

## AI And Spawning

- `AIGroup` prefab spawning is not necessarily populated on the same frame as `SpawnEntityPrefab`.
  - Native/editor logs can report that a group prefab was spawned while its agents are still invisible or pending creation.
  - h-istasi mission/physical-war spawning should keep the existing population grace/polling path before declaring a group failed. The July 2026 logs showed HST-spawned groups reporting zero agents first and then later folding/populating correctly.
  - Game Master/editor placement uses the editor path, not the HST physical-war service path. If editor-placed squads only appear after camera movement, inspect streaming/editor placement addons or native editor activation first; do not assume the HST active-group population guard is involved unless HST spawn logs appear around the placement.

## Native Reference Sources

- Native map config reference: `Configs/Map/MapFullscreen.conf`.
- Native map layout reference: `UI/layouts/Map/MapMenu.layout`.
