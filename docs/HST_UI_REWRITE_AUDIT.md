# h-istasi UI Rewrite Audit

Date: 2026-06-25

This audit tracks the current UI rewrite state against the layout-driven Enfusion UI goal. Classifications use the requested buckets: Allowed, Needs conversion helper, Should move to layout file, and Should be deleted.

## Coordinate Policy

Required policy:

- Raw workspace pixels are for `CreateWidgetInWorkspace` only.
- Layout coordinates come from `HST_UIWorkspaceMetrics.GetLayoutSize`.
- Raw/layout conversion goes through `HST_UIWorkspaceMetrics.LayoutToRawPx` and `HST_UIWorkspaceMetrics.RawToLayoutPx`.
- Map projection coordinates may use `SCR_MapEntity.WorldToScreen` plus `workspace.DPIUnscale` inside map overlay code only.

Current state:

- `Scripts/Game/HST/Services/HST_UIWorkspaceMetrics.c` owns raw/layout size helpers and raw/layout conversion helpers.
- `DebugWorkspaceMetrics` logs raw size, layout size, and effective DPI scale once per UI source.
- Main UI entry points now call the debug helper for setup map, command menu, shared notification toast, mission report dialogs, and loadout editor.
- `HST_UIDebug` logs created layout roots, expected widget presence, expected widget screen bounds, z-order, opacity, flags, row samples, and data population summaries so runtime test logs can identify hidden, zero-sized, off-screen, or mis-stacked layout widgets.
- `HST_UIRootService` owns current-screen, modal-screen, notification, topmost ownership, and modal-aware open/refresh arbitration so blocking screens cannot open underneath unrelated modals and keyboard input can ignore screens hidden under a modal. It logs every open, close, refused open, ignored close, and notification depth change with current/modal/topmost state and root widget geometry.

## File Audit

| File | Current uses | Classification | Action |
| --- | --- | --- | --- |
| `Scripts/Game/HST/Services/HST_UIWorkspaceMetrics.c` | `workspace.GetWidth`, `workspace.GetHeight`, `DPIUnscale`, `DPIScale` | Allowed | This is the central conversion boundary. Keep direct DPI calls here. |
| `Scripts/Game/HST/Components/HST_SetupMapComponent.c` | Workspace-parented native map layout creation, setup-map ready diagnostics, `ScreenToWorld`, centralized modal/prompt z-order, passive prompt flags, setup confirmation modal registration, signature-gated setup zone/candidate overlay publication | Allowed with guardrails | `ScreenToWorld` is allowed only after viewport readiness checks. Keep modal buttons widget-driven. Setup confirmation is registered as a setup-owned modal in the shared UI root. Setup zones and the temporary candidate marker publish through the passive map overlay component and must remain signature-gated. The setup component must not keep its own overlay dirty/redraw loop. |
| `Scripts/Game/HST/Map/HST_MapZoneOverlayUIComponent.c` | `WorldToScreen`, DPI-unscale projection, viewport-change-gated redraws, canvas circle/line/text widgets with `IGNORE_CURSOR`/`NOFOCUS` | Allowed for map overlay | This is the explicit map projection exception. Redraws are coalesced behind revision and pan/zoom threshold checks, and projection-not-ready frames no longer recreate widgets. Keep overlays passive and separate from marker lifecycle. |
| `Scripts/Game/HST/Components/HST_CommandMenuComponent.c` | Layout root, named region binding, row layout population, layout-owned scroll hosts and row resources, action-confirmation data handoff, topmost-aware keyboard input | Allowed with guardrails | Main shell and rows are layout-driven and command-menu blocking state is registered only after the layout root exists. Script no longer builds absolute panel placement metrics. Destructive/admin actions pass data into `HST_ActionDialogController`; keep new command-menu sections in layout files and named row resources rather than reintroducing generic canvas/text factories. |
| `Scripts/Game/HST/Components/HST_CommandMenuRequestComponent.c` | No UI geometry matches in audit grep | Allowed | Request bridge can remain behavior-only. |
| `Scripts/Game/HST/Components/HST_LoadoutEditorComponent.c` | Layout root, named region binding, centralized preview/UI layer ordering, layout-owned left Back/ESC controls, layout-owned mode tabs, layout-owned preview drag surface, layout-owned preview status/toast text, shared passive toast action hints, layout-owned storage category tabs, layout-owned row preview chrome and fallback text, layout-owned row preview cell anchors, layout-owned storage volume progress bar, layout-owned left slot/storage rail shell and scroll hosts, layout-owned candidate/template/settings panel shell and controls, layout-owned footer hint slots, layout-owned storage add-items panel shell/grid, row layout population, preview world, dynamic preview contents, topmost-aware keyboard input | Mixed | Major shell regions, left controls, mode tabs, preview drag capture, preview status/toast text, action feedback, storage category tabs, row preview chrome/fallback text, row preview cell anchors, storage volume track/fill, left rail chrome, candidate/template/settings panels, footer hints, and the right add-items panel are layout-owned or shared-controller owned. Blocking state is registered only after the editor layout root exists, and `ApplyLoadoutLayerOrder` reasserts the render-target/UI sibling order after render and delayed layout refresh. Script no longer builds a safe-rect, absolute panel placement model, screen-percentage panel metrics, or viewport-scaled fallback panel dimensions; it keeps font scale and fixed layout-coordinate fallbacks only until named regions can be measured. |
| `Scripts/Game/HST/Components/HST_MissionClientComponent.c` | Mission detail data selection, notification enqueue calls, report-dialog open/close delegation | Allowed with guardrails | Mission report details now hand `HST_ReportDialogData` to the shared report dialog controller instead of creating named widgets locally. Notifications go through the shared passive toast controller. Future mission action/admin flows should pass data into `HST_ActionDialogController`. |
| `Scripts/Game/HST/UI/HST_ActionDialogController.c` | Action dialog layout creation, modal root registration, named text population, cancel/confirm binding, layout diagnostics | Allowed | Owns the shared confirmation modal widget lifecycle for command and future mission/admin flows. Components should provide only owner/debug IDs, text, and button callback IDs. |
| `Scripts/Game/HST/UI/HST_ReportDialogController.c` | Report dialog layout creation, modal root registration, named text population, objective row layout population, close-button binding, layout diagnostics | Allowed | Owns the mission report dialog widget lifecycle and debug output. Keep mission report UI changes here or in layout resources; mission clients should only provide data and callbacks. |
| `Scripts/Game/HST/UI/HST_NotificationToastController.c` | Passive top-centered toast layout creation, queueing, duration dismissal, UI-root notification state | Allowed | Owns notification widget lifetime for mission and command callers. Keeps notifications non-modal and cursor-ignored while preventing stale dismiss timers from clearing newer toasts. |
| `Scripts/Game/HST/Services/HST_MapMarkerService.c` | No UI geometry matches in audit grep | Allowed | Native marker service should remain marker-record orchestration only. |
| `Scripts/Game/HST/Map/HST_CampaignMapMarkerDirector.c` | No UI geometry matches in audit grep | Allowed | Desired marker record builder is correctly separated from UI projection. |
| `Scripts/Game/HST/Map/HST_NativeMapMarkerReconciler.c` | No UI geometry matches in audit grep | Allowed | Native map marker reconciler owns native marker lifecycle, not UI widgets. |
| `UI/layouts/HST_SetupHQMap.layout` | Native setup map shell | Allowed | Setup-only layout. Keep separated from gameplay map config. |
| `UI/layouts/HST_SetupConfirmModal.layout` | Full-screen dim modal root with centered `Dialog` child and real `NoButton` / `YesButton` | Allowed | Button behavior must remain widget-handler based. The modal is created as top-level setup chrome above the native map root and owns outside-click swallowing itself; no separate blocker layout should be reintroduced. |
| `UI/layouts/HST_LoadoutEditor.layout` | Full-screen preview and named UI regions | Allowed, still expanding | Add dedicated tab/button/panel row layouts as script factories are retired. |
| `Configs/Map/HST_SetupHQMap.conf` | Setup-only native map config with setup cursor module, marker UI, zone overlay component, and `PLAIN` map mode | Allowed for setup only | Used only by `HST_SetupMapComponent`; keep setup selection behavior and setup-only zone overlays out of gameplay. It must not use `FULLSCREEN`, because `SCR_MapEntity.SetupMapConfig` caches by map mode and can otherwise reuse setup UI components for the normal map. |
| `Configs/Map/HST_GameplayMap.conf` | Gameplay map config inheriting vanilla `MapFullscreen.conf` | Allowed | Keeps normal map tools and marker UI through the vanilla config instead of copying setup UI components. The game mode map config component points its gadget map path here. |

## Allowed Uses

- `SetZOrder` on created root/layout widgets where it establishes stacking order.
- `WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS` for passive notifications, setup prompt, map overlays, and noninteractive visual children.
- `FrameSlot.SetPos` / `FrameSlot.SetSize` for widgets created by script into non-stretched slots, such as dynamic map overlay primitives.
- `ScreenToWorld` in setup map click handling after viewport readiness.
- `WorldToScreen` in the map overlay component, followed by raw-to-layout conversion.

## Needs Conversion Helper

- Any future raw-to-layout measurement outside `HST_UIWorkspaceMetrics` should move to `RawToLayoutPx` or a new helper on that service.
- Any future repeated scroll-container placement helpers should become layout widgets with named `Scroll` and `Items` children.
- Any future data-dependent row fill widths should use native value widgets or layout children, with script setting only value/visibility.
- Any future notification source should enqueue through `HST_NotificationToastController`, not create its own toast root.

## Should Move To Layout File

- Mission and command-menu action/admin confirmation flows should use `HST_ActionDialogController` and `HST_ActionDialog.layout` rather than ad hoc panels.
- No current loadout row geometry should move; storage volume now uses a native progress bar.

## Should Be Deleted

- Any reintroduced generic scripted panel root after mission detail/report dialogs moved to named layouts.
- Any invisible coordinate hit-test logic for visible buttons.
- Any new direct `workspace.DPIUnscale` / `workspace.DPIScale` calls outside `HST_UIWorkspaceMetrics.c` and map overlay projection code.
- Any full-screen blockers except explicit modal roots with visible, usable dialog content.

## Runtime Checkpoint 2026-06-25

- `HST_UIRootService.RequestOpen` is now idempotent for identical current/modal roots so repeated setup-map refreshes do not spam root-state transitions or revision churn.
- Setup map, setup prompt, setup confirm modal, command menu, and loadout editor top-level layouts are created with the workspace as parent and now emit delayed `*_ready` geometry logs after layout has had a chance to resolve from creation-time `0x0` bounds.
- Setup confirmation modal text/button labels are script-populated after creation and after the delayed layout refresh. The label widgets are siblings over the real buttons instead of children inside button widgets.
- Command menu close-button hierarchy no longer puts a child frame inside `CloseButton`, removing the runtime GUI error seen in the latest log.
- Loadout editor uses `ApplyLoadoutLayerOrder` after render and delayed layout refresh to keep the render target low, the UI layer above it, the preview drag surface behind panels, and expanded `loadout_editor_ready` geometry logs for the next test pass.
- Setup candidate marker changed from a cross to a small temporary dot/ring marker overlay, keeping setup selection separate from persistent gameplay marker lifecycle.
- Setup map now uses a distinct non-fullscreen map mode and the world map config component explicitly points the normal gadget map at `HST_GameplayMap.conf`, preventing setup's minimal map UI component stack from being reused by the normal gameplay map.
- Shared notification, action-dialog, and report-dialog roots now use workspace-parented layout creation and emit delayed `*_ready` geometry logs, matching the setup/command/loadout chrome lifecycle.
- Setup no longer runs its own overlay dirty/redraw loop. It publishes setup zone/candidate content changes, and `HST_MapZoneOverlayUIComponent` owns projection/redraw timing from content revisions and map viewport changes.

## Remaining Acceptance Gaps

- Runtime-QA root service input blocking across setup, command menu, loadout, mission dialogs, and notifications.
- Wire any remaining mission-specific action/admin confirmation flows into `HST_ActionDialogController`.
- Continue reducing remaining mission action/admin areas without reintroducing command/loadout row geometry.
- Run in-game/Workbench QA at 1920x1080, 2560x1440 with 1920x1080 layout size, and ultrawide.
