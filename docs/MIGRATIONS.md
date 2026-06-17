# Campaign Save Migrations

## Current Schema

`HST_CampaignState.SCHEMA_VERSION` is currently `18`. There is no schema 19 in
the codebase.

- Phase 14 arsenal, loot, loadout-editor, vehicle-cargo, garage/build, and
  field-vehicle persistence work uses the existing schema-18 campaign save
  container.
- The current save container captures campaign metadata, elapsed/save/restore
  counters, war resources, HQ/Petros/cache/arsenal/tent fields, faction pools,
  players, zones, garrisons, active groups, QRFs, map markers, arsenal items,
  garage vehicles, vehicle cargo, runtime vehicles, saved loadouts, issued
  loadout items, captured emplacements, ammo points, active missions, generated
  sites/routes, mission objectives/runtime entities/assets, support requests,
  enemy orders, civilian state, undercover state, and campaign tasks.
- `HST_LoadoutEditorSessionState` records are runtime/editor state and are not
  copied into `HST_CampaignSaveData`; durable saved loadouts and issued-item
  ledgers are copied, and personal templates are also written under
  `$profile:h-istasi/loadouts/v2` with loadout file schema `2`.
- Runtime settings remain schema `8` and are migrated separately by
  `HST_RuntimeSettingsService`.
- Campaign save data is normally tracked through `PersistenceSystem`; when
  scripted persistence cannot flush, the current same-container data can be
  written to and restored from `$profile:h-istasi/HST_CampaignSaveData.json`.
- Raw `IEntity`, `AIGroup`, waypoint, inventory-operation callback, and other
  runtime handles are not persisted as campaign truth.

## Schema 18

Phase 11 mission-specific convoy outcomes.

- `HST_CampaignState.SCHEMA_VERSION` is `18`.
- Active missions now persist convoy outcome-applied flags and a convoy outcome
  summary so mission-specific rewards and penalties do not replay after reload.
- Mission assets now persist an outcome-applied flag and outcome kind so captured
  convoy vehicles, payloads, and captives cannot duplicate asset-level outcomes.
- Existing saves default the new booleans to false and strings to empty during
  normal save-data migration/copy.
- No raw physical entities or runtime handles are persisted.

## Schema 17

Phase 7 convoy waypoint-chain movement.

- `HST_CampaignState.SCHEMA_VERSION` is `17`.
- Active convoy groups now persist `m_iAssignedWaypointCount` as diagnostic
  route-assignment state.
- Schema-16 convoy groups that already recorded `convoy_waypoints` are
  backfilled from their matching generated route waypoint count during
  save-data migration.
- No raw waypoint entities or AI waypoint references are persisted.

## Schema 16

Phase 3 convoy route state.

- `HST_CampaignState.SCHEMA_VERSION` is `16`.
- Generated routes now persist ordered `HST_RouteWaypointState` records.
- Schema-15 routes are backfilled from their legacy start, midpoint, and end
  positions during save-data migration/copy.
- Existing route start/mid/end fields remain for compatibility and are kept in
  sync with waypoint records.
- Route vehicle-safety validation is diagnostic state and can be recomputed from
  generated route waypoints.

## Schema 15

Baseline for the Phase 0/1 stabilization work.

- `HST_CampaignState.SCHEMA_VERSION` remains `15`.
- Persistence smoke-test baselines are stored in the existing persisted `HST_CampaignTaskState` array using `hst_smoke_persistence_expected`.
- Phase 1 zone composition spawn slots are generated at runtime and do not add persisted fields.
- Phase 2 capture diagnostics and notification de-dupe state are runtime-only; capture tuning moved through runtime settings schema `8` without changing campaign save fields.
