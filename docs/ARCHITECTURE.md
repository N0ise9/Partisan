# h-istasi Architecture

## Runtime Ownership

`HST_CampaignCoordinatorComponent` is the server-side entry point. It owns a
single `HST_CampaignState` and delegates to small services. The schema-42
campaign-authority foundation adds four cross-cutting services before existing
feature services are migrated onto the contract:

- `HST_StableIdService`: allocates persisted, monotonic IDs for generated
  commands and events, and builds deterministic operation/transaction links.
- `HST_CampaignCommandService`: validates typed command envelopes and keeps a
  bounded persisted receipt history so replaying the same request ID cannot
  apply the same mutation twice.
- `HST_ResourceLedgerService`: records reserve, commit, cancel, and refund
  transitions for resource mutations. Troop training is the first production
  command routed through this ledger.
- `HST_CampaignEventLogService`: appends bounded persisted campaign events for
  authority decisions and resource transitions.

The remaining domain services are:

- `HST_EconomyService`: HR, faction money, support, aggression, war level,
  strategic score/control percentage, pacing recommendations, victory readiness,
  and balance diagnostics.
- `HST_MissionService`: mission eligibility, activation, deadlines, and
  completion rewards.
- `HST_PersistenceService`: campaign save-data migration/tracking, native
  Reforger checkpoint requests, autosave debouncing, and profile JSON fallback
  saves.
- `HST_PersistenceSmokeTestService`: deterministic persistence fixture seeding
  and restore verification reports.
- `HST_RuntimeSettingsService`: `$profile:h-istasi/HST_Settings.json`
  load/create/migration and settings application to preset/balance data.
- `HST_AuthorizationService`: persistent members, guests, admins, and the
  first commander-vacancy policy.
- `HST_StrategicService`: ownership changes, town support, Petros penalties,
  activation flags, population-first victory/loss evaluation, and durable
  campaign-end state/reporting.
- `HST_HQService`: initial HQ selection, HQ movement, Petros/cache/arsenal/tent/spawn-point
  runtime objects, rebuilds, Petros-loss state, HQ knowledge, HQ threat scans,
  and Defend Petros diagnostics.
- `HST_ArsenalService`: item counts, unlock thresholds, finite/INF withdrawals,
  garage records, and vehicle redeploy.
- `HST_LootService`: area loot, vehicle cargo, garage vehicle capture, field
  vehicle snapshot/restore, and safe vehicle-root scanning.
- `HST_LoadoutEditorService`: saved loadouts, live loadout/storage nodes,
  candidate replacement, finite/INF cost ledgers, atomic apply/rollback, and
  issued-item accounting.
- `HST_BuildModeService`: dry-ground placement resolution for garage redeploys
  and HQ runtime-asset rebuilds.
- `HST_ConvoyOutcomeService`: mission-specific convoy arrival, capture,
  delivery, ammo-point, garage-handoff, and outcome de-dupe effects.
- `HST_CommandUIService`: procedural command-menu payloads, visible action
  routing, reports, tab/action gating, marker/UI audit coverage, command/report
  coverage checks, and failed-action text.
- `HST_EnemyDirectorService`: scaled enemy attack/support resource income by
  owned zone value and war level, plus validated spending against separate
  attack and support pools.
- `HST_PlayerLifecycleService`: connected-player registration, deterministic
  Workbench identity fallback, personal money, and rank.
- `HST_TownService`: resistance income, HR income, and town support changes.
- `HST_GarrisonService`: abstract garrison creation and survivor fold-back.
- `HST_RecruitmentService`: troop training and abstract garrison recruitment.
- `HST_ZoneCaptureService`: capture helpers around strategic ownership
  changes.
- `HST_PlayerSpawnService`: custom FIA HQ spawn, Workbench identity fallback,
  native respawn requests, pending spawn tracking, and spawned-player records.
- `HST_PhysicalWarService`: player-proximity zone activation over the
  abstract garrison model.
- `HST_GeneratedContentService`: generated Everon mission sites, roadblock,
  support, stash, crashsite, and route records derived from stable zone
  anchors.
- `HST_MissionObjectiveService`: rough mission objectives, campaign task state,
  and no-admin objective progress hooks for broad-alpha mission families.
- `HST_MissionRuntimeService`: physical MVP mission primitives, world-condition
  objective polling, runtime inspection, and cleanup state.
- `HST_SupportRequestService`: stateful FIA/enemy support calls,
  ETA/status/cooldown reports, native-safe ground support activation, and
  physical or abstract resolution records.
- `HST_CivilianService`: town reputation/support, wanted heat,
  police/roadblock presence and scans, aid incidents, undercover eligibility,
  request/application, enforcement, compromise, and clear-state records.
- `HST_EnemyCommanderService`: enemy resource spending into patrol, roadblock,
  QRF, counterattack, rebuild, support-call, and Petros attack orders with
  physical/abstract runtime state.
- `HST_MapMarkerService`: native marker rebuild/publish behavior plus marker
  status/detail/audit reports for strategic zones, missions, objectives,
  Defend Petros, support, QRFs, HQ, and convoy state.
- `HST_ZoneCompositionService`: runtime alpha composition slots for zone and
  mission physicalization diagnostics.

Static helper services keep repeated low-level behavior out of the coordinator:
`HST_WorldPositionService` resolves dry/safe positions and prefab spawning,
`HST_DisplayNameService` normalizes item and vehicle labels,
`HST_VehicleRootPolicy` centralizes safe vehicle-root eligibility,
`HST_VehicleCapabilityPolicy` classifies captured/source vehicles as ammo,
repair, fuel, armed, or transport sources, and
`HST_ConvoyVehicleControlAdapter` wraps native vehicle movement/seating calls.

The coordinator currently exposes server-only mutation methods that check
campaign phase, known IDs, and mission eligibility before changing state.
Client UI requests flow through player-owned request/RPC components and
ownership-resolved coordinator methods, so the server resolves the trusted
identity and enforces member, commander, and admin permissions instead of
trusting client-provided player IDs.

## Campaign Authority Boundary

Schema 42 is the first campaign-authority foundation, not a claim that every
existing broad-alpha mutation already uses the new boundary.

| Concern | Current implementation | Target architecture |
| --- | --- | --- |
| Stable identity | A persisted monotonic allocator creates authority IDs, and selected support, order, and active-group records carry operation links. | Every durable operation, force, projection, command, transaction, and event has a stable ID and explicit links. |
| Command idempotency | Visible command requests carry a request ID; bounded receipts make the migrated troop-training path replay-safe. | Every player-visible and scheduled campaign mutation enters through a typed command envelope and produces one durable receipt. |
| Resource integrity | Troop training reserves and commits faction money through the ledger; legacy consumers still mutate resources through their existing services. | All resource changes use reserve/commit/cancel/refund transactions, with no direct debit paths outside the ledger. |
| Force exactness | Existing force-planning and physical-war services remain broad-alpha consumers with partial operation links. | A quoted immutable force manifest is the only input to paid creation, and creation is all-or-nothing before any physical or virtual projection is published. |
| Event history | New command and ledger decisions append to a bounded persisted campaign event log. | All authoritative state transitions emit typed events consumed by projections, UI, diagnostics, and restore reconciliation. |
| Certification | Static validation and Workbench script compilation cover the schema-42 foundation. | Isolated runtime, save/load, dedicated-server, reconnect, and JIP evidence certifies the full boundary. |

Until those target columns are closed, domain services remain authoritative for
their existing behavior, but they must not be described as uniformly
idempotent, ledger-backed, or exact-manifest driven.

## Authoring Contracts

The addon keeps authoring data separate from runtime state:

- `HST_CampaignPreset`: fixed scenario roles and capability switches.
- `HST_FactionTemplate`: faction identity and capability declarations.
- `HST_MapDefinition`: stable IDs for zones and hideout candidates.
- `HST_BalanceConfig`: campaign balance and progression values.
- `HST_MissionRegistryConfig`: mission definitions and deferred capabilities.

The checked-in `.conf` resources are the intended data source and are validated
against the runtime fallback catalog. The current coordinator still uses the
matching fallback catalog while Workbench-safe resource loading and serializer
prefabs are connected.

## Persistence

The persistence service tracks `HST_CampaignSaveData` through
`PersistenceSystem`, applies restored state through a schema migration path,
and flushes the tracked scripted state before requesting
`SaveGameManager.RequestSavePoint` when saving is possible and allowed. The
service also writes `$profile:h-istasi/HST_CampaignSaveData.json` as a profile
fallback when scripted persistence cannot be flushed, and will load that file
if no restored `PersistenceSystem` state is available. The
state model is versioned from day one. `HST_CampaignSaveData` is the deep-copy
save container for current campaign fields and nested runtime arrays. Schema 42
persists the monotonic authority sequence, bounded command receipts, resource
transactions, campaign events, and stable operation links alongside campaign
metadata, resources, campaign-end
reason/summary/elapsed second/control/war/zone-count fields, outcome-mode,
population/support, airfield metadata, support deployment proof, active-group
vehicle prefab, active-group route waypoint counts, runtime infantry waypoint assignment and final-sweep state,
HQ/Petros/cache/arsenal/tent/spawn-point fields,
faction pools, players, zones, garrisons, active groups, QRFs, map markers,
generated content, objectives, mission runtime, mission assets, support, enemy
order, civilian, undercover, arsenal, garage, vehicle cargo, runtime vehicle,
saved loadout, issued-item, ammo point, and captured-emplacement records.
Loadout editor sessions remain runtime/editor state, while durable saved
loadouts and issued-item ledgers are copied into the save container. Save compatibility
still needs broader Workbench restart/load soak testing before it is promised
to players.

## World Layout

`Worlds/HST_Everon/HST_Everon.ent` is an original subscene over vanilla
Everon. Named layer files reserve ownership boundaries for physical authoring.
Strategic IDs live in `Configs/HST/Maps/HST_Everon.conf` so persistence does
not depend on fragile entity names.

The checked-in world shells include a base-backed AI world with explicit Eden
soldier and vehicle navmesh configs, a perception manager, faction, loadout,
radio, chat, and respawn managers so Workbench can initialize without relying
on Conflict's strategic brain.

Direct `.ent` Play mode no longer depends on the stock Deployment Setup menu.
The coordinator is an `SCR_BaseGameModeComponent`, so it receives game-mode
state and player-connected callbacks directly. It also runs a short frame sweep
for connected players without controlled pawns, which covers Workbench timing
where player `1` exists before its respawn component is ready. The respawn
system remains as possession plumbing, but its spawn logic is
`HST_PlayerSpawnLogic`, which delegates to `HST_PlayerSpawnService`. The
service registers the connected player as FIA, submits an `SCR_FreeSpawnData`
request for the default FIA rifleman at the selected HQ hideout, tracks the
request as pending, and records player state only when the native spawn
callback reports success. This prevents the Workbench frame sweep from
creating duplicate bodies while Reforger is still finalizing ownership.

`StartingPoints.layer` still contains FIA-affiliated Scenario Framework
spawnpoint slots and FIA role-selection loadouts, but those are now authoring
metadata and a debug fallback, not the normal player-side path. US
remains the occupier in the strategic preset. Game Master-spawned characters
are still not expected to advance h-istasi's player lifecycle. Workbench
offline play may log blank identity ID errors from stock reconnect or
editable-entity systems; treat those as non-blocking if a character is spawned
and possessed.

`HST_HQService` owns the server-side HQ lifecycle: setup-driven initial hideout
selection, HQ movement between authored hideouts, Petros/cache/arsenal/tent/spawn-point
runtime positions, and Petros-loss penalties. Runtime Petros spawning tries the
custom h-istasi prefab first through its GUID-qualified metadata resource and
falls back to the base FIA character only if that resource cannot spawn. The HQ
arsenal uses a GUID-indexed HST supply-cache prefab whose contextual actions
open the same Arsenal/Loot menu path used by the I-key menu and the custom
loadout editor path, with inherited stock arsenal actions filtered out. A stock
FIA cache fallback is only used if the custom object cannot spawn.

The alpha HQ menu is procedural rather than layout-resource loaded. The server
keeps the existing `HST_MENU`, `TAB`, `STATUS`, `RESULT`, and `ACTION` payload
lines while adding optional `STAT`, `SECTION`, `ROW`, and `FEED` lines for the
h-istasi overview, HQ/Petros, missions, map/war, forces, arsenal/loot,
garage/build, members, and admin panels. The custom loadout editor has its own
`HST_LOADOUT_EDITOR` and `HST_LOADOUT_CANDIDATES` payloads but still routes
mutations through the same server-authoritative request bridge. Contextual
Petros, HQ arsenal, and vehicle cargo actions call the same bridge as menu
clicks so local hosts and MP clients follow one command path.

## Campaign Framework Spine

The first campaign loop is now connected as a physical/abstract hybrid. Zones
carry type, position, income, support, activation radius, route IDs, mission
site IDs, and garrison-slot data in `HST_CampaignState`; garrisons are stored
as infantry and vehicle counts. The physical-war service marks zones active
when players enter their activation radius, converts abstract garrison counts
into route-aware active groups, and folds survivor counts back before
deactivation. Broad-alpha services generate mission sites/routes, attach
objectives/tasks to started missions, poll physical MVP mission primitives from
world conditions, spend scaled enemy pools into orders/support calls, and let
orders/support either physicalize near players or resolve abstractly off-screen.
HQ knowledge feeds HQ threat, Defend Petros, markers, and campaign-end pressure;
civilian town support and undercover enforcement feed wanted heat, roadblock and
police scans, and HQ exposure. Loot, vehicle cargo, virtual garage, build
placement, vehicle capability classification, and loadout editor systems are
owned by campaign state instead of stock arsenal behavior. Mission success,
failure, timeout, convoy outcome, support resolution, enemy orders, vehicle/cargo
paths, and civilian aid mutate economy, support, capture progress, arsenal,
garage, aggression, HQ threat, and victory/loss readiness. Coordinator hooks
expose deterministic server-only actions for Workbench tests and no-admin player
actions for setup, random missions, support requests/cancel, civilian aid,
undercover status/checks, looting, vehicle cargo, garage capture/redeploy,
loadout application, marker/command audits, balance/campaign-end reports,
income, training, recruitment, and HQ moves.
