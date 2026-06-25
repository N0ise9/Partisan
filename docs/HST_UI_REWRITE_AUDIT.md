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
- Main UI entry points now call the debug helper for setup map, command menu, shared notification toast, mission detail UI, and loadout editor.
- `HST_UIRootService` owns current-screen, modal-screen, notification, and modal-aware open/refresh arbitration so blocking screens cannot open underneath unrelated modals.

## File Audit

| File | Current uses | Classification | Action |
| --- | --- | --- | --- |
| `Scripts/Game/HST/Services/HST_UIWorkspaceMetrics.c` | `workspace.GetWidth`, `workspace.GetHeight`, `DPIUnscale`, `DPIScale` | Allowed | This is the central conversion boundary. Keep direct DPI calls here. |
| `Scripts/Game/HST/Components/HST_SetupMapComponent.c` | Native map layout creation, `ScreenToWorld`, centralized modal/prompt z-order, passive prompt flags, setup confirmation modal registration | Allowed with guardrails | `ScreenToWorld` is allowed only after viewport readiness checks. Keep modal buttons widget-driven. Setup confirmation is registered as a setup-owned modal in the shared UI root and should continue avoiding setup zone publish during setup. |
| `Scripts/Game/HST/Map/HST_MapZoneOverlayUIComponent.c` | `WorldToScreen`, DPI-unscale projection, canvas circle/line/text widgets with `IGNORE_CURSOR`/`NOFOCUS` | Allowed for map overlay | This is the explicit map projection exception. Keep redraw throttled and passive. |
| `Scripts/Game/HST/Components/HST_CommandMenuComponent.c` | Layout root, named region binding, row layout population, layout-owned scroll hosts and row resources, shared action-confirmation modal registered with the UI root service | Allowed with guardrails | Main shell and rows are layout-driven and command-menu blocking state is registered only after the layout root exists. Script no longer builds absolute panel placement metrics. Destructive/admin actions use `HST_ActionDialog.layout` with real Cancel/Confirm buttons and modal lifecycle tracking; keep new command-menu sections in layout files and named row resources rather than reintroducing generic canvas/text factories. |
| `Scripts/Game/HST/Components/HST_CommandMenuRequestComponent.c` | No UI geometry matches in audit grep | Allowed | Request bridge can remain behavior-only. |
| `Scripts/Game/HST/Components/HST_LoadoutEditorComponent.c` | Layout root, named region binding, layout-owned left Back/ESC controls, layout-owned mode tabs, layout-owned preview drag surface, layout-owned preview status/toast text, shared passive toast action hints, layout-owned storage category tabs, layout-owned row preview chrome and fallback text, layout-owned row preview cell anchors, layout-owned storage volume progress bar, layout-owned left slot/storage rail shell and scroll hosts, layout-owned candidate/template/settings panel shell and controls, layout-owned footer hint slots, layout-owned storage add-items panel shell/grid, row layout population, preview world, dynamic preview contents | Mixed | Major shell regions, left controls, mode tabs, preview drag capture, preview status/toast text, action feedback, storage category tabs, row preview chrome/fallback text, row preview cell anchors, storage volume track/fill, left rail chrome, candidate/template/settings panels, footer hints, and the right add-items panel are layout-owned or shared-controller owned. Blocking state is registered only after the editor layout root exists. Script no longer builds a safe-rect, absolute panel placement model, or screen-percentage panel metrics; it keeps only font scale and layout-file fallback region sizes before measuring named regions. |
| `Scripts/Game/HST/Components/HST_MissionClientComponent.c` | Modal report dialog layout, modal action dialog layout scaffold, objective row layout population, notification enqueue calls, root-service registration after report root creation | Mixed | Mission report details are layout-driven and only register modal state after the report layout root exists. Notifications now go through the shared passive toast controller. Report/action dialog roots are cursor-active modal layouts; continue wiring future mission action/admin flows through `HST_ActionDialog.layout` instead of adding scripted panel geometry. |
| `Scripts/Game/HST/UI/HST_NotificationToastController.c` | Passive top-centered toast layout creation, queueing, duration dismissal, UI-root notification state | Allowed | Owns notification widget lifetime for mission and command callers. Keeps notifications non-modal and cursor-ignored while preventing stale dismiss timers from clearing newer toasts. |
| `Scripts/Game/HST/Services/HST_MapMarkerService.c` | No UI geometry matches in audit grep | Allowed | Native marker service should remain marker-record orchestration only. |
| `Scripts/Game/HST/Map/HST_CampaignMapMarkerDirector.c` | No UI geometry matches in audit grep | Allowed | Desired marker record builder is correctly separated from UI projection. |
| `Scripts/Game/HST/Map/HST_NativeMapMarkerReconciler.c` | No UI geometry matches in audit grep | Allowed | Native map marker reconciler owns native marker lifecycle, not UI widgets. |
| `UI/layouts/HST_SetupHQMap.layout` | Native setup map shell | Allowed | Setup-only layout. Keep separated from gameplay map config. |
| `UI/layouts/HST_SetupConfirmModal.layout` | Full-screen transparent modal root with centered `Dialog` child and real `NoButton` / `YesButton` | Allowed | Button behavior must remain widget-handler based. The modal is parented to the setup map root and owns outside-click swallowing itself; no separate blocker layout should be reintroduced. |
| `UI/layouts/HST_LoadoutEditor.layout` | Full-screen preview and named UI regions | Allowed, still expanding | Add dedicated tab/button/panel row layouts as script factories are retired. |
| `Configs/Map/HST_SetupHQMap.conf` | Setup-only native map config with setup cursor module, marker UI, and optional zone overlay component | Allowed for setup only | Used only by `HST_SetupMapComponent`; keep setup selection behavior out of gameplay. |
| `Configs/Map/HST_GameplayMap.conf` | Gameplay map config inheriting vanilla `MapFullscreen.conf` | Allowed | Keeps normal map tools and marker UI through the vanilla config instead of copying setup UI components. |

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

- Mission and command-menu action/admin confirmation flows should use `HST_ActionDialog.layout` rather than ad hoc panels.
- No current loadout row geometry should move; storage volume now uses a native progress bar.

## Should Be Deleted

- Any reintroduced generic scripted panel root after mission detail/report dialogs moved to named layouts.
- Any invisible coordinate hit-test logic for visible buttons.
- Any new direct `workspace.DPIUnscale` / `workspace.DPIScale` calls outside `HST_UIWorkspaceMetrics.c` and map overlay projection code.
- Any full-screen blockers except explicit modal roots with visible, usable dialog content.

## Remaining Acceptance Gaps

- Runtime-QA root service input blocking and topmost close behavior across setup, command menu, loadout, mission dialogs, and notifications.
- Wire any remaining mission-specific action/admin confirmation flows into `HST_ActionDialog.layout`.
- Continue reducing remaining mission mixed areas without reintroducing command/loadout row geometry.
- Run in-game/Workbench QA at 1920x1080, 2560x1440 with 1920x1080 layout size, and ultrawide.
