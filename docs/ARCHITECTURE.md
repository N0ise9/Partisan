# h-istasi Architecture

## Runtime Ownership

`HST_CampaignCoordinatorComponent` is the server-side entry point. It owns a
single `HST_CampaignState` and delegates to small services. The cross-cutting
schema-42 through schema-50 authority services are:

- `HST_StableIdService`: allocates persisted, monotonic IDs for generated
  commands and events, and builds deterministic operation/transaction links.
- `HST_CampaignCommandService`: validates typed command envelopes and keeps a
  bounded persisted receipt history so replaying the same request ID cannot
  apply the same mutation twice. `CompleteExplicit` records an authoritative
  applied/rejected status supplied by a migrated command result; `Complete`
  remains a presentation-text compatibility classifier for unmigrated visible
  commands.
- `HST_ResourceLedgerService`: records reserve, commit, cancel, and refund
  transitions for resource mutations. Troop training, exact garrison
  confirmation, and exact player-QRF confirmation/settlement consume it.
- `HST_CampaignEventLogService`: appends bounded persisted campaign events for
  authority decisions and resource transitions.
- `HST_ForceCatalogService`: owns the versioned exact group templates and
  validates their ordered execution-prefab slots.
- `HST_ForcePlanningService` and `HST_ForcePlanningIntegrityService`: issue and
  reconcile immutable garrison/player-QRF manifests and quotes, own
  deterministic hashing/catalog validation shared by queue admission, and keep
  accepted confirmation replay idempotent across later settlement.
- `HST_ForceSettlementArchiveService`: compacts only terminal, backlink-free
  accepted planning aggregates into bounded persisted replay tombstones. It owns
  full-row retention, archive capacity, replay reconstruction, and fail-closed
  admission when protected history cannot make room. For canonical exact-QRF
  operations, compaction additionally requires a coherently settled record,
  copies its typed terminal/revision/settlement evidence, and removes the full
  record with the other accepted aggregate rows.
- `HST_OperationService`: owns the version-1 canonical operation contract for
  confirmed exact paid player infantry QRFs. It registers one record only after
  successful confirmation; preserves immutable origin/assignment separately
  from the mutable tactical target; validates duty, materialization, engagement,
  recall, and settlement transitions; and rejects identity or terminal replay
  conflicts. Schema 50 adds strategic-route, projection-decision, and virtual-
  combat authority without widening the consumer set. Virtual combat drives the
  legal engagement transitions for this one abstract encounter; live physical
  combat contact is still not connected.
- `HST_StrategicMovementService`: owns the schema-50 direct-route cursor and
  conservative 2.5 m/s campaign movement for the exact paid infantry-QRF
  consumer. It derives ETA from route distance, advances only while strategic
  position authority is active, and caps catch-up at 30 campaign seconds per
  invocation.
- `HST_MaterializationService`: evaluates the exact QRF against a player-bubble
  materialize-in distance and a larger materialize-out distance. The hysteresis
  band prevents churn; an engaged physical projection cannot fold.
- `HST_VirtualCombatService`: resolves only an on-station exact infantry QRF
  against hostile abstract infantry in its target garrison. It advances in
  deterministic 30-second power steps, processes at most four steps per tick,
  retires exact manifest member slots, and persists fractional damage carries.
- `HST_OperationProjectionProofService`: adds focused movement, hysteresis,
  roster-transfer, and combat/restore assertions beside the existing eight
  operation-record assertions. These are source-level debug fixtures until a
  packaged runtime executes them.
- `HST_ForceSpawnQueueService`: owns schema-44 durable per-projection spawn
  batches, bounded priority/FIFO work acquisition, verified callbacks,
  retry/deadline/cancellation cleanup, pin-aware terminal retention, reporting,
  dependency-ordered cleanup, a durable nonterminal `READY_FOR_HANDOFF` state,
  explicit post-handoff completion, once-per-actual-restore reconciliation, and
  schema-50 strategic holds. A held batch performs no physical queue work and
  uses its frozen member slots as exact living/dead roster authority.
- `HST_ForceSpawnAdapterService`: consumes eligible released queue work from the production
  one-second active-campaign coordinator tick. The first engine-facing slice
  creates exactly one infantry `SCR_AIGroup` root plus all frozen member slots,
  returns exact prefab/liveness/faction/native-group/Game Master/projection
  evidence, finalizes ready projections in physical war, and only then asks the
  queue to record success. For successfully handed-off exact infantry, it also
  maps authoritative life state back to the exact member slot, detaches confirmed
  dead members without deleting their corpses, and drives last-death cleanup.

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
- `HST_PhysicalWarService`: player-proximity zone activation over the abstract
  garrison model, plus exact adapter registration/handoff and guards that keep
  legacy population, repair, survivor, route, patrol, and cleanup paths from
  duplicating queue-owned projections. For spawned support groups it owns live-
  member-centroid route state, two distinct-time-sample arrival/recall
  confirmation within 75m, direct current-to-target/exit chains, bounded route reissue, and
  transactional replacement of service-owned waypoint entities. Exact QRF
  handoff normalizes into `support_active` so this same route authority owns it.
  For non-queue-managed mixed personnel/vehicle groups it owns a separate
  personnel terminal predicate: vehicle health cannot substitute for living
  infantry after prior population evidence and population grace. Terminal
  cleanup zeros strategic strength, fails an unresolved linked QRF, unregisters
  combat ownership, and releases an intact vehicle as neutral salvage. An
  existing durable field/loot/garage record remains durable; otherwise the
  salvage record is session-only. HQ safety uses separate scopes: 900m remains
  the hostile operation-staging exclusion, but whole-location activation is
  blocked only inside the location capture footprint (at least 150m for legacy
  records). Zone composition uses its own 150m immediate-clearance guard. This
  split is implemented in source and awaits packaged behavior proof.
- `HST_GeneratedContentService`: generated Everon mission sites, roadblock,
  support, stash, crashsite, and route records derived from stable zone
  anchors.
- `HST_MissionObjectiveService`: rough mission objectives, campaign task state,
  and no-admin objective progress hooks for broad-alpha mission families.
- `HST_MissionRuntimeService`: physical MVP mission primitives, world-condition
  objective polling, runtime inspection, and cleanup state.
- `HST_SupportRequestService`: stateful FIA/enemy support calls,
  ETA/status/cooldown reports, native-safe ground support activation, physical
  or abstract resolution records, the schema-46 exact paid-QRF bridge from
  accepted manifest to queue-owned projection, and schema-47 survivor restore,
  elimination, and linked ledger settlement policies. ETA is only the earliest
  physical-arrival observation; spawned requests do not become
  `physical_arrived` or complete recall until physical war confirms current
  living-member distance. Recall mutation returns `HST_SupportRecallResult`
  with explicit accepted/already-applied/state-changed/terminal flags,
  disposition, failure, request, and operation identity. Exact paired full
  refunds validate both linked transactions and their settlement eligibility
  before either refund begins. New confirmed exact paid infantry QRFs also opt
  into the canonical operation contract. In schema 50 they begin as held virtual
  projections, follow the strategic route, run the narrow on-station virtual-
  combat policy, materialize exact survivors near a player, and fold exact live
  survivors back to the held roster after players leave the larger radius while
  the operation is clear of contact. Queue admission, projection transfers,
  restore, recall, and settlement update the same operation/request/group/
  manifest/batch identities. Route initialization occurs while duty is still
  `STAGING`; `LinkOutboundVirtual` commits delivered strategic service by moving
  duty to `OUTBOUND`. A terminal failure before that commit fully refunds money
  and HR. A later no-handoff materialization failure retains money and refunds
  only the operation's last virtual-friendly count, including from
  `ON_STATION`, with replay-idempotent settlement. Restore also clears the linked
  request's physicalized flag and publishes a virtual runtime status so request,
  operation, and held batch cannot disagree about projection ownership.
- `HST_CivilianService`: town reputation/support, wanted heat,
  police/roadblock presence and scans, aid incidents, undercover eligibility,
  request/application, enforcement, compromise, clear-state records, and
  disposable pedestrian/traffic projections. Ambient actors receive dedicated
  CIV group roots that inherit the stock behavior/pathfinding/utility/
  replication stack; initial members attach through the engine's AI-composition
  path, and the service owns their group, driver, and waypoint cleanup. Current
  source classifies stock town-center locations separately from minor localities,
  permits civilian projection even when HQ safety suppresses hostile military
  activation, selects deterministic non-repeating concrete appearances, defaults
  true towns to five driven vehicles, limits the known woodland locality to two
  pedestrians, and clears horn input only on HST-owned ambient drivers. These
  behavior corrections await a republished runtime test.
- `HST_EnemyCommanderService`: enemy resource spending into patrol, roadblock,
  QRF, counterattack, rebuild, support-call, and Petros attack orders with
  physical/abstract runtime state.
- `HST_MapMarkerService`: native marker rebuild/publish behavior plus marker
  status/detail/audit reports for strategic zones, missions, objectives,
  Defend Petros, support, QRFs, HQ, and convoy state. Linked terminal-group
  state is checked before unresolved-QRF visibility, preventing a dead response
  from retaining a tactical marker. Schema-50 source labels static zones with
  location and owner and reports virtual exact-QRF travel/on-station state.
- `HST_ZoneCompositionService`: runtime alpha composition slots for zone and
  mission physicalization diagnostics. Radio-site composition now retains a
  nearby intact authored transmitter instead of spawning a parallel tower; the mission
  runtime can borrow that damageable entity for destroy-target tracking without
  deleting it during generic cleanup. Generated towers remain a fallback for a
  missing or destroyed authored transmitter, including rebuild outcomes.

Static marker rendering has a separate client lifecycle boundary. A small
manager patch retries root creation before the stock static-marker update and
disables a marker that remains rootless. The rendered-map proof waits for the
delayed client pass and inspects actual active roots and widget components;
server publication/reconciler counts are reported only as native handles.
Config-backed modded classes on this path must also repeat the base class's
container metadata. A packaged schema-49 test verified that restoring the
original `BaseContainerProps` and custom-title attributes brought normal Game
Master and stock HUD initialization back. That run then exposed eighteen invalid
radio icon entries as giant boxes. Current source preserves the canonical placed-
marker table, appends or repairs a validated normal/glow radio icon by resource
identity instead of a hard-coded index, and rejects invalid entries. The map-
target prompt, target indicator, and dialog now share a map-local z-order below
the native workspace pointer. Those follow-up fixes still need packaged proof.

Static helper services keep repeated low-level behavior out of the coordinator:
`HST_WorldPositionService` resolves dry/safe positions and prefab spawning,
`HST_DisplayNameService` normalizes item and vehicle labels,
`HST_VehicleRootPolicy` centralizes safe vehicle-root eligibility,
`HST_VehicleCapabilityPolicy` classifies captured/source vehicles as ammo,
repair, fuel, armed, or transport sources, and
`HST_ConvoyVehicleControlAdapter` wraps native vehicle movement/seating calls,
registers valid pilotable vehicles with the crew utility before seating,
prefers forced authority-local entry for server-owned AI, and exposes a direct
retained-registration query for runtime proof.

The coordinator currently exposes server-only mutation methods that check
campaign phase, known IDs, and mission eligibility before changing state.
Client UI requests flow through player-owned request/RPC components and
ownership-resolved coordinator methods, so the server resolves the trusted
identity and enforces member, commander, and admin permissions instead of
trusting client-provided player IDs.

## Campaign Authority Boundary

Schema 43 extends the first campaign-authority foundation with exact force
planning. Schema 44 adds durable bounded SpawnQueue authority for executable
manifest projections. Schema 45 adds explicit force/projection identity on
active groups, durable Game Master registration evidence, dependency-ordered
cleanup, and the first engine-facing exact infantry adapter. Schema 46 makes
player-paid resistance QRF the first support consumer of the complete quote →
ledger → executable manifest → SpawnQueue → settlement path. None of these
schemas is a claim that every existing broad-alpha mutation uses the boundary.
Schema 47 adds durable exact-member casualty state, ever-populated/spawn-
completed terminal semantics, corpse-preserving root cleanup, and survivor-only
paid-QRF reprojection. Schema 48 bounds accepted quote/manifest/ledger history,
preserves compact issue/confirmation/transaction replay, and adds the missing
production caller for pin-aware terminal SpawnQueue maintenance.
Schema 49 adds a canonical, versioned operation aggregate for newly confirmed
exact paid player infantry QRFs and uniquely coherent active schema-48 rows.
That narrow slice is not a claim that legacy QRFs, other support, missions,
garrisons, enemy orders, or every force projection use the aggregate.
Schema 50 adds persistent strategic projection to that same narrow consumer:
direct-route progress, held exact roster authority, proximity hysteresis,
physical-to-virtual survivor transfer, bounded virtual infantry combat, and
restore normalization to one virtual projection. It does not migrate vehicles,
assets, convoys, garrisons, broad legacy supports, missions, or enemy orders.

| Concern | Current implementation | Target architecture |
| --- | --- | --- |
| Stable identity | A persisted monotonic allocator creates authority IDs; garrisons, quotes, manifests, transactions, and selected support/order/group records carry explicit stable links. Contract-version-1 exact QRFs bind that identity to one canonical operation row and schema-50 projection state. | Every durable operation, force, projection, command, transaction, and event has a stable ID and explicit links. |
| Command idempotency | Visible command requests carry request IDs; bounded receipts cover migrated training and quote/confirm commands. Support recall is the first production visible command to map a typed domain result into explicit receipt status and operation identity; its presentation sentence cannot turn an accepted terminal outcome into a rejection. Other visible commands still use the compatibility classifier. Full accepted state and schema-48 settlement tombstones prevent later duplicate issue/confirmation/ledger replay from charging again, including after QRF settlement. | Every player-visible and scheduled campaign mutation enters through a typed command envelope and produces one durable receipt. |
| Resource integrity | Troop training, visible garrison confirmation, and player-QRF confirmation/terminal settlement use the ledger. Exact QRF full-refund paths preflight the paired money/HR identities, coherent state, settleability, and deterministic settlement IDs before either mutation; this is a bounded paired preflight, not a general transactional rollback layer. Other support consumers still mutate resources through legacy services. | All resource changes use reserve/commit/cancel/refund transactions, with no direct debit paths outside the ledger. |
| Force exactness | Visible garrison recruitment freezes an exact priced purchase-provenance manifest. Player QRF freezes one executable authored group root plus every ordered member slot, charges flat $250 plus one HR per member, and never recomposes after issue. Garrison activation still does not consume its purchase manifest. | A quoted immutable force manifest is the only input to paid creation, and creation is all-or-nothing before any physical or virtual projection is published. |
| Force realization | SpawnQueue accepts only frozen, hash-valid, all-required executable manifests with a group root. An exact paid infantry QRF begins as one held virtual batch whose member slots are the roster. Entering the materialize-in radius releases exactly the living slots to the adapter; all verified slots reach `READY_FOR_HANDOFF`, physical war finalizes them, and then the queue records `SUCCEEDED`. Leaving the materialize-out radius while clear of contact samples the live roster and position, retires disposable entities, and returns the exact survivors to strategic hold. Confirmed casualties remain retired across every transfer and restore. Vehicle, asset, and multi-root manifests remain unsupported; garrison manifests remain nondeployable. | One adapter realizes every supported manifest, registers each slot exactly once, restores successful projections safely, and feeds durable living-force/casualty/retirement authority without bypass paths. |
| Operation lifecycle | Schema 50 keeps one canonical record after exact paid-QRF confirmation, separates immutable assignment from tactical recall target, and advances typed duty/materialization/position/settlement state through strategic travel, physical/virtual transfer, on-station virtual infantry combat, restore, recall, terminal settlement, and archive. Virtual combat uses the engagement API; live physical contact does not yet drive it. Legacy/enemy QRFs, broad legacy supports, garrisons, missions, and enemy orders remain outside this contract. | Every force/order uses one versioned operation aggregate with event-driven engagement, strategic movement progress, physical/virtual transfer, settlement, and client/JIP projection. |
| Event history | New command and ledger decisions append to a bounded persisted campaign event log. | All authoritative state transitions emit typed events consumed by projections, UI, diagnostics, and restore reconciliation. |
| Certification | Repository foundation validation covers the current schema-50 source contracts. Workbench script validation loads 5,747 files/11,508 classes with CRC `16665f19`, and the correctly launched WorldEditor remains responsive through the bounded 20-second gate. A packaged schema-49 check verified restored stock HUD and Game Master startup, then exposed marker and civilian defects repaired only in schema-50 source. The strategic projection fixtures, marker/cursor/radio corrections, civilian classification/diversity/traffic/horn corrections, late-admin guard, and save/restart projection behavior still need packaged execution. | Isolated physical runtime, save/load/reprojection, dedicated-server, reconnect, and JIP evidence certifies the full boundary. |

Concurrent open garrison quotes are capped and expired/terminal unreferenced
planning rows can be pruned. SpawnQueue terminal projection rows have explicit
backlink pins, a 600-second minimum retention window, and a 128-row admission
bound; the production coordinator now supplies pins and runs that maintenance.
Schema 48 retains accepted rows in full for at least 600 seconds and while any
projection backlink exists, then atomically moves replay identity and final
transaction evidence into at most 256 tombstones with an 86,400-second minimum
window. Full quotes plus tombstones share a hard 320-row admission bound. An old
evicted quote fails closed rather than recreating a debit or aggregate mutation.

Until those target columns are closed, domain services remain authoritative for
their existing behavior, but they must not be described as uniformly
idempotent, ledger-backed, or exact-manifest driven.

Campaign-debug state isolation is fail-closed. Every in-process profile is
restricted to the development world. The persistence service first durably
captures the live campaign, the coordinator runs the suite against a deep-cloned
`HST_CampaignState`, checkpoints only isolated in-memory save data, and swaps the
untouched live state back on completion or cancellation. External/soak profiles
cannot enter this runner. This protects authoritative campaign persistence, but
does not roll back world entities, player inventory/health, delayed callbacks,
or service caches, so a development-session restart remains part of the safety
boundary.

## Authoring Contracts

The addon keeps authoring data separate from runtime state:

- `HST_CampaignPreset`: fixed scenario roles and capability switches.
- `HST_FactionTemplate`: faction identity and capability declarations.
- `HST_MapDefinition`: stable IDs for zones and hideout candidates.
- `HST_BalanceConfig`: campaign balance and progression values.
- `HST_MissionRegistryConfig`: mission definitions and deferred capabilities.

The current coordinator uses `HST_DefaultCatalog` as runtime authority. The
checked-in faction `.conf` resources are not loaded by that path and are not
guaranteed mirrors; they must not be cited as gameplay truth until one loader
and startup validator replaces the duplicated declarations. Schema-43 exact
force planning uses `HST_ForceCatalogService`, whose explicit catalog version
and ordered slots are validated against effective prefab containers at runtime.
Schema-44 queue admission consumes the frozen execution-prefab slots directly;
manifest identity binds the input but never substitutes for a projection or
idempotency key. Schema 45 keeps the group root and member entities as runtime
projections while persisting their force/projection links and registration
evidence. Schema 46 selects one authored player-QRF group, freezes all ordered
slots, and submits that exact manifest without invoking the broad composition
service. Schema 47 persists each exact member's living/retired lifecycle and
reuses the immutable manifest while excluding confirmed casualties from restore
reprojection. The current adapter deliberately rejects vehicle, asset, and multi-root
manifests instead of shortening them or falling back to broad composition.

## Persistence

The persistence service tracks `HST_CampaignSaveData` through
`PersistenceSystem`, applies restored state through a schema migration path,
and flushes the tracked scripted state before requesting
`SaveGameManager.RequestSavePoint` when saving is possible and allowed. The
service also writes `$profile:h-istasi/HST_CampaignSaveData.json` as a profile
fallback when scripted persistence cannot be flushed, and will load that file
if no restored `PersistenceSystem` state is available. The state model is
versioned from day one. `HST_CampaignSaveData` is the deep-copy save container
for current campaign fields and nested runtime arrays. Schema 50 persists the
monotonic authority sequence, bounded command receipts, resource transactions,
campaign events, stable operation/garrison links, immutable force manifests,
expiring quotes, durable per-projection spawn batches/slot evidence,
Game Master registration evidence, explicit active-group force/projection IDs,
support quote/capability/schedule fields, linked exact-QRF aggregate identities,
successful-handoff/reprojection counters, member casualty tombstones, active-
group ever-populated/spawn-completed/living-force evidence, and the queue
restore/reconciliation epochs, plus bounded accepted-settlement replay tombstones
and their final resource-status/refund evidence. It also persists canonical
exact-QRF operation records with immutable assignment, mutable tactical target,
typed duty/engagement/materialization/position/settlement state, execution links,
policy IDs, timestamps, terminal result, revision, direct-route cursor and
distance, strategic speed/update clock, projection decisions, virtual-combat
clock/damage carries, exact virtual force counts, and strategic batch holds,
alongside campaign
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
to players. An actual persisted restore increments its epoch once, then the
coordinator reconciles the queue before garrison/player-QRF confirmation and
open-resource reservations. Accepted exact QRFs never reacquire saved entity
IDs. Schema 50 clears process-local root/member/native-group evidence and keeps
one nonterminal batch in strategic hold; its confirmed-dead slots remain retired
and only exact survivors can be released for later materialization. Interrupted
pending, `READY_FOR_HANDOFF`, and previously successful physical batches all
normalize to that same virtual authority instead of assuming an interrupted
handoff succeeded or respawning immediately. A post-handoff technical failure
still retains paid money and refunds durable survivors only. Terminal prefab,
verification, handoff, and casualty history remains evidence, while terminal
entity/native-group IDs are cleared rather than reacquired as living authority.

The schema-49 migration from schema 48 creates a contract-version-1 operation
only for a uniquely coherent accepted nonterminal exact paid player QRF. It preserves request,
quote, manifest, queue, group, transaction, and balance state and leaves every
pre-exact, terminal, archived-only, incomplete, or ambiguous row on contract
version `0`. Schema-50 migration opts only coherent exact infantry-QRF
operations into strategic projection. On every restore, a nonterminal opted-in
projection clears process-local entity IDs, becomes one held virtual batch with
strategic position authority, and retains exact survivor/casualty slots, duty,
assignment, economy, and terminal history. Unsupported or ambiguous aggregates
remain operation-only. These rules have deterministic source coverage but no
packaged process-restart evidence.

Normal adapter acquisition runs only after the campaign enters the active phase.
During setup and won/lost phases, the coordinator requests cancellation for all
nonterminal batches and keeps draining dependency-ordered cleanup with a
monotonic runtime-only clock. That cleanup clock advances retry/defer eligibility
without advancing campaign elapsed time. Schema 47 implements successful
survivor reprojection and casualty/retirement authority for the exact paid
infantry-QRF consumer. It confirms physical death only from a present slot-
mapped entity's authoritative life state; deletion or a missing entity is not
casualty evidence. Schema 50 adds virtual roster transfer, direct strategic
movement, materialization hysteresis, and narrow virtual combat for that same
consumer. Event-driven physical life-state subscription, vehicle/asset rosters,
generalized folding, live physical engagement events, and generalization to
every force consumer remain open. Paid support is only partially migrated to
this path: player QRF is exact, while supply, search, roadblock, fire, and air
support remain on legacy services. Current garrison purchase manifests remain
nondeployable.

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
deactivation. A mixed group with previously observed personnel becomes terminal
when its living infantry reaches zero after delayed-population and live-count
grace, even if its attached vehicle remains intact. That vehicle becomes
unclaimed, detached salvage and no longer contributes to garrison,
capture, QRF, or marker strength; vehicle-only projections keep their existing
separate semantics. Existing durable field ownership/cargo is preserved; an
unadopted salvage record is session-only. Eliminated/spawn-failed save normalization preserves zero
counts and clears process-local spawn identity rather than reconstructing
original force totals; folded rows retain the survivor counts already returned
to the abstract garrison.
Broad-alpha services generate mission sites/routes, attach
objectives/tasks to started missions, poll physical MVP mission primitives from
world conditions, spend scaled enemy pools into orders/support calls, and let
orders/support either physicalize near players or resolve abstractly off-screen.
The schema-50 exception is the newly confirmed exact paid infantry QRF: its
single operation and frozen member roster move continuously through virtual,
materializing, physical, dematerializing, and virtual states rather than using a
one-shot abstract outcome. That path remains intentionally isolated from
vehicles, convoys, garrisons, enemy orders, missions, and broad legacy support.
HQ knowledge feeds HQ threat, Defend Petros, markers, and campaign-end pressure;
civilian town support and undercover enforcement feed wanted heat, roadblock and
police scans, and HQ exposure. Civilian render eligibility is separate from the
hostile military activation bit so HQ safety does not erase nearby town life.
Radio compositions prefer an existing world transmitter, and radio destroy
missions can borrow it as their tracked damage target. Loot, vehicle cargo,
virtual garage, build
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
