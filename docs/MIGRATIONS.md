# Campaign Save Migrations

## Current Schema

`HST_CampaignState.SCHEMA_VERSION` is currently `37`.

- Schema 37 adds the durable strategic-event ledger so mission success/failure
  and zone-capture consequence rows preserve source identity, target
  zone/faction, applied status, before/after owner fields, and money/HR/support/
  capture/aggression/resource/HQ-knowledge deltas across save-data roundtrips.
- Schema 36 adds the selected active-group vehicle prefab so mixed
  infantry/vehicle response groups can respawn the same vehicle choice after a
  save/load roundtrip.
- Schema 35 adds persisted support deployment proof for physicalized support:
  deployment route id, placement type/summary, target/road/HQ distances,
  road resolution, vehicle-safe result, and whether vehicle-safe placement was
  required.
- Schema 34 adds persisted population-outcome metadata for campaign-end state:
  outcome mode, initial/remaining/killed population, FIA-supporting population,
  support percent, and controlled/total airfields.
- Schema 33 adds durable active-group source-link fields and original force
  count fields so support, mission, QRF, and garrison active groups can be
  traced through fold-back, cleanup, refunds, and save-data roundtrips.
- Schema 32 extends durable vehicle heat/report fields to garage vehicles so
  reported vehicle cover state survives capture, virtual storage, redeploy, and
  save-data roundtrips.
- Schema 31 adds durable runtime vehicle heat/report fields so reported
  civilian vehicles, passenger compromises, report expiry, and vehicle-cover
  eligibility remain inspectable across saves.
- Schema 30 adds durable town influence events plus town population and
  influence aggregate fields so aid, civilian casualties, security pressure,
  support-majority flips, and active/expired modifiers remain inspectable
  across saves.
- Schema 29 adds durable enemy support-spend ledgers and enemy-order refund
  stamps so QRF/support cooldowns, recent damage pressure, max-spend denials,
  and survivor fold-back refunds remain inspectable across saves.
- Schema 28 adds durable force-composition metadata to active groups, support
  requests, and enemy orders. Existing saves load with empty composition
  summaries and zero composition counts until the next force-planning event.
- Schema 27 adds a durable display-only player name field so member/commander
  roster UI can show readable player names while backend identity remains the
  authority for permissions and ownership.
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
  enemy support ledgers, civilian state, town influence events, strategic
  events, undercover state, and campaign tasks.
- `HST_LoadoutEditorSessionState` records are runtime/editor state and are not
  copied into `HST_CampaignSaveData`; durable saved loadouts and issued-item
  ledgers are copied, and personal templates are also written under
  `$profile:h-istasi/loadouts/v2` with loadout file schema `2`.
- Runtime settings are schema `18` and are migrated separately by
  `HST_RuntimeSettingsService`.
- Campaign save data is normally tracked through `PersistenceSystem`; when
  scripted persistence cannot flush, the current same-container data can be
  written to and restored from `$profile:h-istasi/HST_CampaignSaveData.json`.
- Raw `IEntity`, `AIGroup`, waypoint, inventory-operation callback, and other
  runtime handles are not persisted as campaign truth.

## Runtime Settings Schema 18

Generated settings comments.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `18`.
- Generated settings now include JSON-safe `_comment` and `_comment_*` string
  fields that explain nearby settings. The runtime loader ignores these fields
  and still reads only the scalar gameplay keys.
- Existing settings migrate by rewriting the generated profile with comments
  while preserving known gameplay values.

## Runtime Settings Schema 17

Generated settings hideout key removal.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `17`.
- `campaign.defaultHideoutId` is no longer generated or read. Initial HQ
  placement is selected through the setup map flow, so the profile config should
  not imply a separate default hideout source of truth.
- Existing settings migrate by rewriting the generated profile without the
  obsolete hideout key while preserving the remaining scalar settings.

## Runtime Settings Schema 16

Resistance support group marker tracking.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `16`.
- `features.trackResistanceSupportGroupsOnMap` defaults to `true`.
- Existing settings migrate the feature on so spawned player-requested
  resistance support groups keep live map markers until they are terminal or
  despawned.

## Schema 37

Strategic event ledger.

- `HST_CampaignState.SCHEMA_VERSION` is `37`.
- `HST_StrategicEventState` records durable mission outcome and zone-capture
  consequences with event kind, source/mission ids, target zone/faction,
  applied status, summary, before/after owner fields, and deltas for money, HR,
  town support, capture progress, aggression, attack/support resources, and HQ
  knowledge.
- Existing schema-36 and older saves load with an empty strategic-event ledger;
  new mission success/failure outcomes and resistance zone captures append rows
  when they are applied.

## Schema 36

Active-group vehicle prefab.

- `HST_CampaignState.SCHEMA_VERSION` is `36`.
- `HST_ActiveGroupState` now persists the selected vehicle prefab for mixed
  infantry/vehicle groups.
- Existing schema-35 and older active groups load with an empty vehicle prefab;
  the runtime can select a valid faction vehicle when it next materializes a
  mixed active group that lacks a stored vehicle prefab.

## Schema 35

Support deployment proof.

- `HST_CampaignState.SCHEMA_VERSION` is `35`.
- `HST_SupportRequestState` now persists the deployment route id, placement
  type, placement summary, target/road/HQ distance meters, road resolution,
  vehicle-safe result, and whether vehicle-safe staging was required.
- Physical support requests now copy their placement proof into save data so a
  support request, linked active group, and folded support resolution can be
  audited after save/load.
- Vehicle-capable support requests require vehicle-safe staging. If vehicle-safe
  staging is specifically unavailable, the support deployment downgrades to
  infantry-only staging instead of failing the whole response.

## Schema 34

Population outcome metadata.

- `HST_CampaignState.SCHEMA_VERSION` is `34`.
- Campaign end state now persists the outcome mode, initial population,
  remaining population, killed population, FIA-supporting population, support
  percent, controlled airfields, and total airfields.
- Population victory is the default outcome mode: enough remaining population
  must support the resistance and all airfields must be controlled.
- Population loss is the default loss mode: killed population greater than one
  third of initial population ends the campaign.
- Existing ended saves backfill the new metadata from civilian town state and
  airfield ownership when possible; older control-based ended saves keep
  `legacy_control` as the persisted outcome mode.
- Runtime settings schema `15` adds `populationOutcomeEnabled`,
  `victoryPopulationSupportPercent`, and `legacyControlVictoryEnabled`.

## Schema 33

Active group source links and baseline force counts.

- `HST_CampaignState.SCHEMA_VERSION` is `33`.
- `HST_ActiveGroupState` now persists source identifiers for mission, support
  request, garrison zone, and QRF ownership, plus original infantry and vehicle
  counts.
- Migration backfills missing original counts from current active-group counts
  and links existing groups from support requests, QRF records, active mission
  guard/convoy group ids, or their zone id as a garrison-source fallback.

## Schema 32

Garage vehicle heat and undercover vehicle-cover handoff state.

- `HST_CampaignState.SCHEMA_VERSION` is `32`.
- `HST_GarageVehicleState` now persists reported state, civilian vehicle-cover
  eligibility, heat value, report timestamps, last report reason/zone, and
  passenger compromise count.
- Runtime-to-garage capture and garage-to-runtime redeploy copy vehicle heat
  metadata through the handoff. Existing schema-31 and older garage vehicles
  backfill civilian-cover eligibility from source faction, source kind, and
  civilian vehicle prefab hints, and start with no reported heat unless a newer
  save already carried it.

## Schema 31

Runtime vehicle heat and undercover vehicle-cover state.

- `HST_CampaignState.SCHEMA_VERSION` is `31`.
- `HST_RuntimeVehicleState` now persists whether a runtime vehicle is reported,
  whether it can provide civilian undercover cover, its heat value, report
  timestamps, last report reason/zone, and passenger compromise count.
- Existing schema-30 and older saves backfill civilian-cover eligibility from
  runtime vehicle faction, runtime kind, and civilian vehicle prefab hints.
  Existing runtime vehicles start with no reported heat unless they were already
  marked hot by newer state.

## Schema 30

Town influence events and population aggregates.

- `HST_CampaignState.SCHEMA_VERSION` is `30`.
- `HST_TownInfluenceEventState` persists event id, zone id, kind, source,
  reason, created/expiry seconds, support/reputation/heat/population/security
  deltas, and whether the event was applied.
- `HST_CivilianZoneState` now persists population remaining/killed, influence
  event counts, active/expired modifier counts, and the latest influence event
  metadata.
- Existing schema-29 and older saves backfill town population from civilian
  presence and mark legacy influence summaries from the last incident/security
  fields. Existing saves start with no influence event rows until the next
  town-support event is registered.

## Schema 29

Enemy support-spend ledgers and refunds.

- `HST_CampaignState.SCHEMA_VERSION` is `29`.
- `HST_EnemySupportLedgerState` persists per-faction/per-zone recent damage
  score, last damage time, attack/support spend, last spend time, same-zone
  cooldown, refund totals, and the latest decision reason.
- `HST_EnemyOrderState` now persists attack/support refund amounts and whether
  the resource refund has already been applied, preventing survivor fold-back
  refunds from replaying after reload.
- Existing schema-28 and older saves default ledgers to empty and refund stamps
  to zero/false; no destructive backfill is required.

## Schema 28

Force composition metadata.

- `HST_CampaignState.SCHEMA_VERSION` is `28`.
- `HST_ActiveGroupState`, `HST_SupportRequestState`, and
  `HST_EnemyOrderState` now persist composition request id, intent, selected
  tier, summary text, planned cost, planned manpower, planned vehicle count,
  and planned armed-vehicle count. Support requests and enemy orders also keep
  a composition failure reason.
- The persisted fields are diagnostics and replay hints only. Runtime entity,
  group, vehicle, and waypoint handles remain non-persisted.
- Existing schema-27 and older saves default these fields to empty strings and
  zero counts; no backfill is required.

## Schema 27

Player roster display names.

- `HST_CampaignState.SCHEMA_VERSION` is `27`.
- Runtime settings schema `13` adds `features.infiniteStaminaEnabled`,
  defaulting to `true` for generated and migrated settings. This is not a
  campaign-save migration.
- `HST_PlayerState` now persists `m_sDisplayName`, refreshed from the connected
  player manager when a player registers or is seen during menu/permission
  checks.
- Member and commander UI uses the display name for labels and only shows a
  shortened backend identity as secondary evidence.
- Permission checks, admin grants, loadouts, undercover state, and ownership
  records continue to use backend identity/SteamID64 values, not display names.
- Existing schema-26 and older saves default display names to empty and refresh
  them when the player reconnects.

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
