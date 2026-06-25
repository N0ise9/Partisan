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
  - Current examples: `setup_prompt_ready`, `setup_confirm_modal_ready`, `command_menu_ready`, `loadout_editor_ready`.

- `root.FindAnyWidget(root.GetName())` may not find the root itself.
  - If debug code checks expected widgets and includes the root name, explicitly compare `root.GetName()` before reporting the root missing.

- A `ButtonWidget` does not accept arbitrary child widgets.
  - Runtime symptom: `GUI (E): Cannot add a child, the ButtonWidget CloseButton does not accept more children`.
  - Avoid putting custom `FrameWidget`/`PanelWidget`/`TextWidget` children inside a `ButtonWidget` unless the engine-provided slot pattern is known to work for that exact widget.
  - Stable workaround: make the button a real sibling hit target and place a sibling `TextWidget` over it with `Ignore Cursor = true`.
  - Current examples: `HST_CommandMenu.layout` close button, `HST_SetupConfirmModal.layout` Yes/No labels.

- Full-screen anchored roots should not be manually resized.
  - Avoid `FrameSlot.SetPos(root, 0, 0)` and `FrameSlot.SetSize(root, ...)` on roots using stretched anchors.
  - Runtime warning pattern: position/size only works when min/max anchors are the same.
  - Let layout anchors own full-screen geometry; script may set z-order, visibility, opacity, and text/data.

- Keep visual children passive unless they are real controls.
  - Use `WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS` or layout `Ignore Cursor = true` for passive panels, labels, overlays, and notification visuals.
  - Do not place invisible full-screen widgets unless they are intentional visible modal roots with real content.

## Coordinates And DPI

- Workspace raw size and layout size are different under UI scaling.
  - Example seen in testing: raw `2560x1440`, layout `1920x1080`.
  - Use `HST_UIWorkspaceMetrics.GetRawWorkspaceSize` only for raw pixel needs.
  - Use `HST_UIWorkspaceMetrics.GetLayoutSize` for layout coordinates.
  - Use `HST_UIWorkspaceMetrics.LayoutToRawPx` / `RawToLayoutPx` for conversions.

- Keep direct `workspace.DPIUnscale` / `workspace.DPIScale` calls inside the metrics helper, except map projection overlay code.
  - The map overlay is the exception because `SCR_MapEntity.WorldToScreen` returns native screen coordinates that must be unscaled before placing layout widgets.

- `SCR_MapEntity.WorldToScreen` gives native screen coordinates.
  - Convert projected x/y with `workspace.DPIUnscale` before `FrameSlot.SetPos`.
  - For world-radius circles, project the center and a world-space edge, derive the raw pixel radius, then DPI-unscale the final size.

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

## Input And Widget Events

- Real controls should be widget-driven.
  - Give interactive widgets stable `UserID`s and attach a `ScriptedWidgetEventHandler`.
  - Avoid raw coordinate hit testing for visible buttons.

- Some interactions may arrive through both `OnClick` and `OnMouseButtonUp`.
  - Use duplicate-activation guards where a widget action can be triggered by both callbacks in the same frame.

- Modal state belongs in one coordinator.
  - `HST_UIRootService` tracks current screen, modal screen, blocking behavior, and topmost owner.
  - Repeated identical `RequestOpen` calls should be idempotent; otherwise refresh loops create noisy revision churn.

- Notifications should not participate in blocking input.
  - Keep toast roots and children cursor-ignored and no-focus.
  - Track notification depth separately from current/modal screens.

## Enforce Script Practicalities

- Prefer explicit boolean checks for object references when returning a bool.
  - `if (widget) return true;` is clearer and safer than returning widget-reference expressions from `bool` methods.

- `CallLater` supports delayed UI repair/diagnostic passes and can accept arguments.
  - Existing examples with arguments include notification dismissal generation checks and preview entity finalization.

- Keep code comments sparse and practical.
  - Comments should capture non-obvious engine constraints, not restate simple assignments.

## Native Reference Sources

- Native map config reference: `Configs/Map/MapFullscreen.conf`.
- Native map layout reference: `UI/layouts/Map/MapMenu.layout`.
