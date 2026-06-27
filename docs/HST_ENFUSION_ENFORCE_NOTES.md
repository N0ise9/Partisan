# h-istasi Enfusion / Enforce Notes

Purpose: capture reusable facts learned while rebuilding h-istasi UI so we do not rediscover the same Enfusion and Enforce Script edge cases repeatedly.

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

- Layout diagnostics must be runtime-gated.
  - Compile-time `static const` feature flags are useful as a hard kill switch, but every high-volume helper should also check the debug setting loaded from runtime settings.
  - Cache the loaded runtime setting in the debug helper so row/widget probes do not parse settings on every row.

- `root.FindAnyWidget(root.GetName())` may not find the root itself.
  - If debug code checks expected widgets and includes the root name, explicitly compare `root.GetName()` before reporting the root missing.

- A `ButtonWidget` does not accept arbitrary child widgets.
  - Runtime symptom: `GUI (E): Cannot add a child, the ButtonWidget CloseButton does not accept more children`.
  - Avoid putting custom `FrameWidget`/`PanelWidget`/`TextWidget` children inside a `ButtonWidget` unless the engine-provided slot pattern is known to work for that exact widget.
  - Stable workaround: make the button a real sibling hit target and place a sibling `TextWidget` over it with `Ignore Cursor = true`.
  - Current examples: `HST_CommandMenu.layout` close button, `HST_SetupConfirmModal.layout` Yes/No labels, `HST_ActionDialog.layout` Cancel/Confirm labels, `HST_ReportDialog.layout` Close label.

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

- Some interactions may arrive through both `OnClick` and `OnMouseButtonUp`.
  - Use duplicate-activation guards where a widget action can be triggered by both callbacks in the same frame.
  - Pass the mouse button through both event paths and key the guard by widget id, button, and frame serial so right-click/left-click actions remain distinct.
  - Current examples: `HST_SetupMapComponent`, `HST_CommandMenuComponent`, `HST_LoadoutEditorComponent`, and `HST_MissionClientComponent`.

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

- Prefer explicit boolean checks for object references when returning a bool.
  - `if (widget) return true;` is clearer and safer than returning widget-reference expressions from `bool` methods.

- `CallLater` supports delayed UI repair/diagnostic passes and can accept arguments.
  - Existing examples with arguments include notification dismissal generation checks and preview entity finalization.

- Reused layout roots need container cleanup, not just tracked-widget cleanup.
  - If generated rows are created into layout-owned containers and the root is reused, remove tracked dynamic widgets and then remove all children from the dynamic containers before repopulating.
  - Hidden or parent-hidden generated widgets can survive a mode switch if cleanup only clears visible children. Remove tracked dynamic widgets regardless of current visibility before the container sweep.

- Keep live inventory payloads single-owner.
  - If a server payload emits real storage contents as storage-node children, do not also re-emit those same entries as synthetic top-level inventory rows.
  - A simple guard is to treat slots with `m_sSlotKind == "storage_item"` or a non-empty parent slot id as nested storage rows only.
  - Clearing only an array of widgets misses rows that were not inserted into that array and causes tabs/lists to accumulate stale entries.
  - Hidden layout regions may not be reliably found by a global cleanup pass, depending on visibility and resolution timing.
  - Clear the active dynamic container again after the region has been resolved and shown, then populate it.

- `ItemPreviewWidget` does not reliably render every inventory prefab.
  - Keep a category/fallback icon available when prefab or entity preview setup fails.
  - Do not hide the fallback icon just because the entity preview lookup returned null for a non-empty prefab.

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

- Keep code comments sparse and practical.
  - Comments should capture non-obvious engine constraints, not restate simple assignments.

## Native Reference Sources

- Native map config reference: `Configs/Map/MapFullscreen.conf`.
- Native map layout reference: `UI/layouts/Map/MapMenu.layout`.
