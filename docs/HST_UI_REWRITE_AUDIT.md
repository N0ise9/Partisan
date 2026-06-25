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
- Main UI entry points now call the debug helper for setup map, command menu, mission notification/detail UI, and loadout editor.

## File Audit

| File | Current uses | Classification | Action |
| --- | --- | --- | --- |
| `Scripts/Game/HST/Services/HST_UIWorkspaceMetrics.c` | `workspace.GetWidth`, `workspace.GetHeight`, `DPIUnscale`, `DPIScale` | Allowed | This is the central conversion boundary. Keep direct DPI calls here. |
| `Scripts/Game/HST/Components/HST_SetupMapComponent.c` | Native map layout creation, `ScreenToWorld`, modal/prompt z-order, passive prompt flags | Allowed with guardrails | `ScreenToWorld` is allowed only after viewport readiness checks. Keep modal buttons widget-driven. Continue avoiding setup zone publish during setup. |
| `Scripts/Game/HST/Map/HST_MapZoneOverlayUIComponent.c` | `WorldToScreen`, DPI-unscale projection, canvas circle/line/text widgets with `IGNORE_CURSOR`/`NOFOCUS` | Allowed for map overlay | This is the explicit map projection exception. Keep redraw throttled and passive. |
| `Scripts/Game/HST/Components/HST_CommandMenuComponent.c` | Layout root, named region binding, row layout population, legacy canvas/text factories and scroll helper | Mixed | Main shell is layout-driven. Remaining generic `CreateRectWidget`, `CreateTextWidget`, and `CreateScrollContainer` should shrink as more rows/panels move to layout files. |
| `Scripts/Game/HST/Components/HST_CommandMenuRequestComponent.c` | No UI geometry matches in audit grep | Allowed | Request bridge can remain behavior-only. |
| `Scripts/Game/HST/Components/HST_LoadoutEditorComponent.c` | Layout root, named region binding, layout-owned left Back/ESC controls, layout-owned mode tabs, layout-owned left slot/storage rail shell and scroll hosts, layout-owned storage add-items panel shell/grid, row layout population, preview world, dynamic preview cells, legacy factories | Mixed | Major shell regions, left controls, mode tabs, left rail chrome, and the right add-items panel are layout-owned. Dynamic item previews and volume fill are allowed data-driven geometry. Remaining save/settings/candidate panels should be replaced by dedicated layouts/controllers. |
| `Scripts/Game/HST/Components/HST_MissionClientComponent.c` | Notification layout, report dialog layout, action dialog layout scaffold, objective row layout population | Mixed | Notifications and mission report details are layout-driven. Continue wiring future mission action/admin flows through `HST_ActionDialog.layout` instead of adding scripted panel geometry. |
| `Scripts/Game/HST/Services/HST_MapMarkerService.c` | No UI geometry matches in audit grep | Allowed | Native marker service should remain marker-record orchestration only. |
| `Scripts/Game/HST/Map/HST_CampaignMapMarkerDirector.c` | No UI geometry matches in audit grep | Allowed | Desired marker record builder is correctly separated from UI projection. |
| `Scripts/Game/HST/Map/HST_NativeMapMarkerReconciler.c` | No UI geometry matches in audit grep | Allowed | Native map marker reconciler owns native marker lifecycle, not UI widgets. |
| `UI/layouts/HST_SetupHQMap.layout` | Native setup map shell | Allowed | Setup-only layout. Keep separated from gameplay map config. |
| `UI/layouts/HST_SetupConfirmModal.layout` | Centered modal with real `NoButton` and `YesButton` | Allowed | Button behavior must remain widget-handler based. |
| `UI/layouts/HST_LoadoutEditor.layout` | Full-screen preview and named UI regions | Allowed, still expanding | Add dedicated tab/button/panel row layouts as script factories are retired. |
| `Configs/Map/HST_SetupHQMap.conf` | Setup-only native map config with setup cursor module, marker UI, and optional zone overlay component | Allowed for setup only | Used only by `HST_SetupMapComponent`; keep setup selection behavior out of gameplay. |
| `Configs/Map/HST_GameplayMap.conf` | Gameplay map config inheriting vanilla `MapFullscreen.conf` | Allowed | Keeps normal map tools and marker UI through the vanilla config instead of copying setup UI components. |

## Allowed Uses

- `SetZOrder` on created root/layout widgets where it establishes stacking order.
- `WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS` for passive notifications, setup prompt, map overlays, and noninteractive visual children.
- `FrameSlot.SetPos` / `FrameSlot.SetSize` for widgets created by script into non-stretched slots, such as dynamic map overlay primitives, preview cells, and data-dependent fill bars.
- `ScreenToWorld` in setup map click handling after viewport readiness.
- `WorldToScreen` in the map overlay component, followed by raw-to-layout conversion.

## Needs Conversion Helper

- Any future raw-to-layout measurement outside `HST_UIWorkspaceMetrics` should move to `RawToLayoutPx` or a new helper on that service.
- Repeated scroll-container placement helpers in command/loadout should become layout widgets with named `Scroll` and `Items` children.
- Data-dependent row fill widths should eventually use layout children where possible, with script setting only value/visibility.

## Should Move To Layout File

- Mission action/admin confirmation flows should use `HST_ActionDialog.layout` rather than ad hoc panels.
- Command menu fallback text/rect factories that are no longer part of the primary layout path.
- Loadout save/settings/candidate panels that still create substantial panel chrome in script.

## Should Be Deleted

- `UI/layouts/HST_ScriptedPanelRoot.layout` after mission detail/report dialogs move to named layouts.
- Any invisible coordinate hit-test logic for visible buttons.
- Any new direct `workspace.DPIUnscale` / `workspace.DPIScale` calls outside `HST_UIWorkspaceMetrics.c` and map overlay projection code.
- Any full-screen blockers except explicit modal blocker layouts.

## Remaining Acceptance Gaps

- Runtime-QA root service input blocking and topmost close behavior across setup, command menu, loadout, mission dialogs, and notifications.
- Wire any remaining mission action/admin confirmation flows into `HST_ActionDialog.layout`.
- Finish replacing remaining loadout save/settings/candidate panels with dedicated layout widgets.
- Run in-game/Workbench QA at 1920x1080, 2560x1440 with 1920x1080 layout size, and ultrawide.
