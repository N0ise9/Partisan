# Campaign Save Migrations

## Current Schema

`HST_CampaignState.SCHEMA_VERSION` is currently `26`.

- Schema 26 adds durable HQ spawn-point position/prefab fields and backfills
  older deployed HQ saves so the HQ runtime object rebuild can spawn and
  verify a physical respawn marker.
- Phase 24 balance, pacing, and campaign outcomes add durable campaign-end report fields and backfill legacy ended saves.
- Phase 23 UI and marker polish does not require a campaign schema bump because marker audits, command coverage, failed-action text, and menu summaries are derived from existing persisted state.
- Phase 22 HQ threat and Defend Petros add durable HQ threat diagnostics and active defense mission/order/support/group links.
- Phase 21 undercover enforcement and police/roadblocks add durable applied/enforcement state, detection score/source, compromise reason, and police/roadblock scan audit fields.
- Phase 20 civilians, town support, and undercover reports add durable town support values and detailed undercover eligibility reasons.
- Phase 19 support request lifecycle hardening adds durable support-request
  runtime fields while preserving runtime handles as non-persisted data.
- Phase 18 enemy commander physical responses add durable enemy-order
  runtime fields while preserving runtime handles as non-persisted data.
- The current save container captures campaign metadata, elapsed/save/restore
  counters, war resources, campaign-end state, HQ/Petros/cache/arsenal/tent/spawn-point
  fields, HQ threat/Defend Petros state, faction pools, players, zones,
  garrisons, active groups, QRFs, map markers, arsenal items, garage vehicles,
  vehicle cargo, runtime vehicles, saved loadouts, issued loadout items,
  captured emplacements, ammo points, active missions, generated sites/routes,
  mission objectives/runtime entities/assets, support requests, enemy orders,
  civilian state, undercover state, and campaign tasks.
- `HST_LoadoutEditorSessionState` records are runtime/editor state and are not
  copied into `HST_CampaignSaveData`; durable saved loadouts and issued-item
  ledgers are copied, and personal templates are also written under
  `$profile:h-istasi/loadouts/v2` with loadout file schema `2`.
- Runtime settings remain schema `12` and are migrated separately by
  `HST_RuntimeSettingsService`.
- Campaign save data is normally tracked through `PersistenceSystem`; when
  scripted persistence cannot flush, the current same-container data can be
  written to and restored from `$profile:h-istasi/HST_CampaignSaveData.json`.
- Raw `IEntity`, `AIGroup`, waypoint, inventory-operation callback, and other
  runtime handles are not persisted as campaign truth.

## Schema 26

HQ spawn-point persistence and rebuild verification.

- `HST_CampaignState.SCHEMA_VERSION` is `26`.
- Campaign state now persists the HQ spawn-point position and prefab alongside
  Petros/cache/arsenal/tent HQ runtime metadata.
- Existing schema-25 and older deployed HQ saves backfill the spawn-point
  position near the HQ and clear the HQ runtime spawned flag so runtime objects
  are rebuilt with the new spawn point.
- Raw spawn-point entity handles remain runtime-only and are recreated by
  `HST_HQService`.

## Schema 25

Phase 24 balance, pacing, and campaign outcomes.

- `HST_CampaignState.SCHEMA_VERSION` is `25`.
- Campaign state now persists campaign-end reason, summary, elapsed second, control percent, war level, FIA/enemy zone counts, and whether an end report was generated.
- Existing schema-24 and older saves that were already WON/LOST backfill campaign-end fields from elapsed time, war level, and zone ownership counts.
- Runtime settings schema `9` adds balance knobs for starting training, war-level thresholds, victory/loss conditions, enemy income scaling, and aggression decay tuning.
- Balance and campaign-end reports are generated from existing zone/resource/mission state; raw world/runtime handles remain non-persistent.

## No Schema Bump: Phase 23 UI And Marker Polish

Phase 23 adds marker audits, command coverage reports, failed-action text, and
menu summary polish on top of existing persisted campaign state. These reports
are rebuilt from campaign fields such as markers, missions, support requests,
enemy orders, HQ/Petros state, civilian/undercover state, and runtime counters,
so no campaign save schema bump was required.

## Schema 24

Phase 22 HQ threat and Defend Petros.

- `HST_CampaignState.SCHEMA_VERSION` is `24`.
- Campaign state now persists HQ threat/knowledge diagnostics and active Defend Petros linkage.
- Defend Petros records persist linked mission/order/support/group ids, status, timing, attacker counts, failure reason, and outcome-applied flag.
- Existing schema-23 saves backfill HQ threat from HQ knowledge and mark existing Defend Petros mission ids as active.
- Raw Petros entities, attacker entities, AI groups, and runtime support entities remain runtime-only and are not persisted.

## Schema 23

Phase 21 undercover enforcement and police/roadblocks.

- `HST_CampaignState.SCHEMA_VERSION` is `23`.
- Undercover player records now persist applied/enforcement state, last compromise reason, detection source, enforcement zone, enforcement timestamps, detection score, and police/roadblock scan counters.
- Civilian town records now persist last police/roadblock scan times and the last security reason.
- Existing schema-22 records are backfilled from last eligibility/status/reason fields.
- Raw player entities, inventory handles, vehicle handles, and runtime civilian/police entities remain runtime-only and are not persisted.

## Schema 22

Phase 20 civilians, town support, and undercover reports.

- `HST_CampaignState.SCHEMA_VERSION` is `22`.
- Civilian town records now persist FIA support, occupier support, last incident reason, and last support-change time.
- Undercover player records now persist request state and the last detailed eligibility report: clothing, weapon/equipment, vehicle, off-road, enemy proximity, and wanted-heat reasons.
- Existing schema-21 civilian and undercover records are backfilled from reputation, support, police/roadblock presence, wanted heat, and last reason.
- Raw player entities, inventory handles, vehicle handles, and runtime civilian entities remain runtime-only and are not persisted.

## Schema 21

Phase 19 support request lifecycle hardening.

- `HST_CampaignState.SCHEMA_VERSION` is `21`.
- Support request records now persist runtime status, resolution kind, physicalization mode, activated time, physicalized time, resolved time, physicalized flag, and outcome-applied flag.
- Existing schema-20 support requests are backfilled from status, group linkage, strike runtime ID, and ETA.
- Raw `IEntity`, `AIGroup`, support component handles, spawned entities, and callback references remain runtime-only and are not persisted.

## Schema 20

Phase 18 enemy commander physical responses.

- `HST_CampaignState.SCHEMA_VERSION` is `20`.
- Enemy order records now persist support/group linkage, runtime status, source/target positions, physicalization time, resolution time, resolution kind, failure reason, and outcome flags.
- Existing schema-19 enemy orders are backfilled from status and support-request linkage.
- Raw `IEntity`, `AIGroup`, support component handles, and spawned entity references remain runtime-only and are not persisted.

## Schema 19

Phase 15 garage and vehicle persistence hardening.

- `HST_CampaignState.SCHEMA_VERSION` is `19`.
- Garage vehicle records and runtime vehicle records now persist source-vehicle
  capability fields: ammo source, repair source, fuel source, and source vehicle kind.
- Existing garage/runtime vehicle records are backfilled from prefab-based capability rules.
- Schema 19+ restores preserve persisted source capability fields; prefab backfill is only
  for pre-schema-19 records or newly captured world vehicles.
- Garage redeploy runtime vehicles are eligible for persistent field-vehicle restore.
- No raw `IEntity`, `AIGroup`, inventory handles, or runtime pointers are persisted.

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
