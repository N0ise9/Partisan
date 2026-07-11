# h-istasi Capability Map

## Implemented Foundation

- Typed versioned campaign state
- Fixed FIA versus US versus USSR preset
- Campaign resource pools, HR, money, support, aggression, and war-level
  service surface
- Persistent member, guest, admin, commander-vacancy, player lifecycle,
  town-support, income, arsenal, vehicle-cargo, saved-loadout, issued-item,
  garage-record, runtime-vehicle, abstract-garrison, recruitment, and
  enemy-pool service surfaces
- Common mission lifecycle and configured mission-registry baseline
- Native Reforger manual and periodic checkpoint requests with
  `PersistenceSystem` tracking for the scripted campaign save container
- Original Everon world shell and stable strategic-zone IDs
- Custom FIA HQ player spawn path that bypasses stock Deployment Setup and
  uses game-mode player callbacks, a short spawn sweep, native respawn
  requests, pending spawn tracking, and spawn-success callbacks
- FIA Scenario Framework spawnpoints and role-selection loadouts retained as
  authoring metadata and fallback scaffolding
- HQ lifecycle service for setup-driven initial hideout selection, HQ
  movement, Petros state, Petros/cache/tent runtime object positions,
  Petros-loss penalties, HQ knowledge/threat, and Defend Petros state
- Versioned campaign save container for current state fields and nested arrays,
  with schema migration and restored-state application helpers
- Schema-55 campaign authority foundation with persisted monotonic IDs, typed
  command receipts, resource transactions, exact force quotes/manifests,
  durable per-projection SpawnQueue state, exact paid-infantry-QRF runtime
  lifecycle, bounded accepted-settlement replay tombstones, and the first
  two canonical infantry-QRF operation aggregates plus the first exact mission-
  convoy operation with persistent strategic projection
- One versioned `OperationRecord` for each newly confirmed exact paid player
  infantry QRF, plus conservative backfill for uniquely coherent accepted active
  schema-48 rows. It separates immutable origin/assignment from tactical target
  and persists typed duty, engagement, materialization, position, settlement,
  policy, terminal, execution-link, timing, revision, direct-route cursor,
  projection-decision, and virtual-combat authority. Schema 50 makes that one
  consumer travel while virtual, materialize/fold with hysteresis, preserve its
  exact living/dead slots, and fight only hostile abstract infantry at its target
  in bounded deterministic steps. Live physical combat-contact wiring,
  vehicles/assets, broader encounter simulation, generalized virtualization,
  and operation families beyond the enemy defensive slice below remain open.
  Four projection assertions join the existing eight operation-record
  assertions, but none is packaged runtime proof until executed there.
- Newly planned infantry-only enemy defensive QRFs are the second explicit
  version-1 operation type and the first exact enemy-order consumer. One
  distinct same-faction operational source, same-faction defended target under resistance pressure, prepaid frozen
  roster, order, operation, held batch, and active group remain linked through
  materialization/fold, once-only defensive pressure, return to origin, exact
  casualties, and survivor-proportional attack/support settlement. Six focused
  `enemy_qrf.*` assertions cover admission, legacy isolation, projection,
  settlement, restore, and rejection in source. Existing enemy orders and every
  other enemy order type remain contract version `0`.
- Newly started convoy missions are the third explicit version-1 operation type
  and the first narrow exact vehicle/multi-group consumer. One persisted
  generated road route, exactly three vehicle slots, three crew groups and
  ordered crew slots, plus optional cargo/captive assigned to vehicle zero stay
  linked through virtual travel, proximity materialization, physical
  interception, clear-contact fold, casualty-preserving rematerialization,
  route-authoritative arrival, and existing once-only convoy outcomes.
  Frozen slot/entity identity retains the exact member that died rather than
  choosing casualties from a count.
  Destroyed/captured vehicle survivors continue as exact stationary crew-only
  roots without vehicle resurrection, and bubble ownership considers separated
  living/recoverable roots. Frozen-carrier cargo remains at that terminal
  carrier's durable ground position rather than changing vehicle slots.
  Crew elimination leaves a recovery-dependent mission open in an on-station
  hold so unresolved cargo, captives, or vehicles can virtualize and
  rematerialize until their mission-specific outcome is resolved, including a
  cargo-only terminal-carrier root when no intact vehicle remains.
  Historical restored convoys remain contract version `0`; off-screen convoy
  combat and generalized vehicle/asset realization remain open.
- Newly issued policy-v2 purchased resistance garrisons are the fifth explicit
  version-1 operation type after the schema-53 exact enemy patrol. One
  `NotSpawned` empty execution root, arbitrary ordered purchased member roster,
  accepted quote/manifest/garrison backlinks, held batch, local generated route,
  and active group remain one authority graph. The patrol loops indefinitely
  while virtual, materializes only living slots near players, and folds exact
  survivors/cursor state. Owner change, all-dead, campaign stop, setup, or typed
  spawn/route failure settles once with zero refund. Historical policy-v1,
  initial-map, enemy aggregate,
  vehicle, and multi-root garrisons remain legacy; malformed current rows use
  version `-54` quarantine without legacy conversion.
- Newly started `assassinate_officer` guard infantry is the sixth explicit
  version-1 operation type. One zero-cost empty execution root and its exact
  catalog members own a route-less guard assignment offset from—but never
  linked as owner of—the HVT. Guard casualties survive fold/re-entry; all guards
  dead settles only the guard as `DESTROYED` while the HVT objective remains
  active. Mission, owner-change, and coherent spawn/assignment outcomes map to
  typed zero-refund settlement. Historical officer missions, other assassination
  variants, and every remaining mission family remain contract `0`; pre-55
  migration invents nothing. Malformed current rows use version `-55` quarantine
  without fallback or HVT failure. The existing HVT marker/UI row reports guard
  strength. The stamped Schema-55 tree passes foundation, clean Workbench Game
  validation at 5,763 files/11,570 classes with CRC `0ec8950e`, and a ten-sample/
  20-second normal WorldEditor open. Native projection/casualties, save/restart,
  rendered UI, owner-change, campaign-setup, packaged networking, reconnect, and
  JIP proof remain open.
- Newly started `assassinate_traitor` guard infantry is the seventh explicit
  operation-family consumer and reuses the mission-guard operation type at
  contract `2`. Its policy is `exact_assassinate_traitor_guard_v1`; malformed
  current authority uses `-56`. The Schema-55 officer path stays contract `1`/
  `-55`. The traitor path has the same route-less empty-root/member roster,
  separate HVT, survivor-only materialization/fold, HVT-independent all-dead
  result, typed zero-refund settlement, compact restore, and existing-HVT status
  projection. Historical/pre-56 traitor missions, `assassinate_specops`, and all
  other unsupported families remain contract `0`. Migration and conflict events
  are `migration_schema56_exact_traitor_guard` and
  `normalization_schema56_exact_traitor_guard_conflict`. Six source-proof
  categories cover the boundary; native entities/adapter casualties, actual
  save/restart, rendered UI, owner-change, setup, packaged networking, reconnect,
  and JIP are unclaimed. The stamped Schema-56 tree identifies implementation
  `bab5748d817ba434dae701cfbb3b92805d463678` / label
  `schema56-exact-traitor-guard` / stamp
  `03a65cd33bee69c6320389803cdd5a2ec8576fb0`, passes foundation, and passes Workbench Game
  validation at 5,764 files/11,573 classes with CRC `a18c67a5` and
  `Script validation successful`. Its bounded hidden normal WorldEditor open
  stayed alive for ten samples over 20 seconds; the latest log had no script-
  error/crash signature. This is not packaged behavior proof.
- Newly started `assassinate_specops` guard infantry is the eighth explicit
  operation-family consumer and reuses the mission-guard type at contract `3`,
  policy `exact_assassinate_specops_guard_v1`, intent
  `assassinate_specops_guard`, and quarantine `-57`. Officer `1`/`-55` and
  traitor `2`/`-56` remain exact. The spec-ops path reuses the route-less empty-
  root/member roster, separate HVT authority, survivor-only materialize/fold/
  re-entry, no virtual guard combat, typed zero-refund settlement, compact
  restore, and existing-HVT status. Historical/pre-57 spec-ops, ordinary
  `mission_group_*` rows, rescue missions, and unsupported families remain
  contract `0`. Migration/conflict events are
  `migration_schema57_exact_specops_guard` and
  `normalization_schema57_exact_specops_guard_conflict`. Six source-proof
  categories cover the boundary. Schema 57 is stamped at implementation
  `514ebdcbeb1ddfb2a383b19590382517113e2ff6` with build label
  `schema57-exact-specops-guard`. The full foundation gate passes, including
  Schema-55/56/57. Stamped Workbench Game validation loaded 5,765 files/11,576
  classes with CRC `e0b8578e` and `Script validation successful`; the bounded
  hidden normal WorldEditor open stayed alive for 10/10 samples over 20 seconds,
  and its log had no script-error/crash signature. All packaged/native/restart/
  UI/network gates remain open.
- Newly started `rescue_pows` is the ninth exact family consumer and introduces
  `HST_OPERATION_TYPE_MISSION_RESCUE`. Schema-58 contract `1`, policy
  `exact_rescue_pows_v1`, intent `rescue_pows_guard`, and quarantine `-58` own
  one guard roster plus exactly three typed captive slots. HELD/FREED captives
  fold outside the bubble; FOLLOWING/BOARDING/BOARDED retain stable escort/
  vehicle/seat evidence and stay projected. There is no virtual casualty or
  guard-combat inference. Guard elimination does not settle rescue; any captive
  casualty receipt fails, while three HQ extraction receipts succeed through
  normal mission outcomes. A 300-second grace is custody-only and forbids new
  claims. Historical POWs and `rescue_refugees` remain contract `0`. Migration/
  conflict events are `migration_schema58_exact_rescue_pows` and
  `normalization_schema58_exact_rescue_pows_conflict`. Source proof does not
  claim packaged native entities, vehicle seats, restart, rendered UI, owner
  change, setup, networking, reconnect, or JIP.
- Typed support-recall completion across service, coordinator, visible-command
  dispatch, durable receipt, and diagnostics. Accepted terminal wording cannot
  be reclassified by presentation text; exact paired full refunds prevalidate
  both ledger legs before either changes. Other visible commands still retain
  the compatibility text classifier.
- Exact visible garrison recruitment quotes and exact player-paid infantry-QRF
  quotes. A new policy-v2 garrison confirmation submits its accepted empty-root/
  member manifest unchanged to held exact patrol authority; historical policy-v1
  purchase provenance remains nondeployable. Paid QRF retains its existing
  adapter path. Vehicle/asset/multi-root realization and remaining paid supports
  are not yet migrated.
- Everon alpha anchors for strategic zones, towns, hideouts, routes, and
  mission sites
- Physical-war activation scaffold that marks nearby zones active, moves
  abstract garrison counts into route-aware active groups, and folds survivor
  counts back on deactivation
- Source-only HQ policy split: 900m protects hostile operation staging, whole-
  location activation uses the location capture footprint with a 150m fallback,
  and static composition clearance uses 150m. Nearby towns/bases are no longer
  erased wholesale by the staging radius; packaged proof remains open.
- Personnel-authoritative lifecycle for non-queue-managed mixed groups: an
  intact attached vehicle cannot keep a previously populated zero-infantry QRF
  combat-effective. The terminal transition clears capture/marker pressure,
  fails an incomplete linked QRF, preserves the intact vehicle as neutral
  salvage, retains any prior durable field/cargo record, treats unadopted
  salvage as session-only, and remains zero-survivor across schema-48 roundtrip.
- Coordinator dev actions for zone capture, income ticks, mission
  success/failure, training, recruitment, and garrison fold-back
- Dedicated Petros character prefab that inherits from FIA rifleman but can be
  edited independently from player spawn characters; runtime spawning uses the
  GUID-indexed custom prefab first and falls back to the base FIA prefab only
  if needed
- HQ arsenal supply-cache prefab with contextual actions for opening the
  Arsenal/Loot tab, opening the custom loadout editor, and depositing nearby
  loot into campaign arsenal state; inherited stock arsenal actions are
  filtered from the h-istasi HQ arsenal surface
- Procedural h-istasi HQ menu with resource stats, overview, HQ/Petros,
  mission board, map/war, forces, arsenal/loot, garage/build, member, admin,
  action, and activity/result panels
- Broad-alpha persistent state for generated sites/routes, mission objectives,
  campaign tasks, support requests, enemy orders, civilian town state, player
  undercover state, HQ threat/Defend Petros, and strategic campaign-end fields
- Schema 7 zone metadata for display names, resource kinds, capture radii,
  priority, composition IDs, spawn profiles, and linked-zone hints
- Everon 4x-style alpha campaign graph expansion with additional outposts,
  factories, resource depots, seaports, radio towers, banks, and police nodes,
  authored as h-istasi-owned config/anchors/marker stubs
- Generated Everon content service that creates alpha mission sites,
  roadblock/support/stash/crashsite points, and simple route records from the
  existing strategic zone anchors
- Mission objective/task service that attaches rough objectives to started
  missions and lets the no-admin commander flow progress them into normal
  mission completion rewards and strategic outcomes
- Mission runtime service that maps all 39 configured registry IDs into physical MVP
  primitives: kill HVT, hold/clear area, destroy target, recover cargo,
  rescue/extract, deliver supplies, and convoy intercept. This establishes
  configured breadth, not behavioral parity or runtime certification for every
  mission family.
- Stateful support request service with queued deployment pacing:
  FIA/enemy support has ETA/status/cooldown reporting, physical ground-group
  activation when players are nearby, and abstract resolution when off-screen or
  unsupported by base-game assets. Spawned support now retains its live member
  centroid, treats ETA as an earliest observation only, requires two live
  samples from distinct elapsed seconds within 75m for arrival/recall exit, and
  uses transactional direct target/exit waypoint chains with at most three
  consecutive reissues until an 8m new-best distance improvement resets the
  stall budget. Pre-repair restored arrival/exit rows receive one current live-
  distance revalidation. This
  source boundary still needs fresh packaged runtime proof.
- Exact paid player infantry QRF is the first and only support consumer on the
  schema-50 strategic projection path. Its frozen manifest slots remain the
  roster while virtual; a direct 2.5 m/s campaign cursor advances without a
  nearby player, proximity releases exact survivors for physical projection,
  leaving the larger radius while clear of contact folds the live roster back,
  and on-station virtual combat updates exact casualties plus the hostile
  abstract infantry garrison. Supply, search, roadblock, fire, air support,
  legacy/enemy QRF, vehicles, convoys, garrisons, missions, and enemy orders are
  not migrated by the schema-50 player slice.
- Enemy commander service with patrol/QRF/search, counterattack, rebuild,
  support-call, and Petros attack orders that track physicalized or abstract
  runtime state and outcome application. Schema 51 routes only newly planned
  infantry-only defensive QRFs through exact operation authority, suppressing a
  parallel legacy response at the same target and bypassing the fixed legacy
  resolution timer. Schema 53 routes only newly queued patrols through a separate
  type-plus-version exact owner with one proactive debit, one frozen infantry
  root, generated-route loop/return, contact hold, fold/reprojection, and exact
  survivor settlement. Historical patrols remain legacy.
- Newly purchased policy-v2 resistance garrisons use a separate exact
  `GARRISON_PATROL` owner. They walk a persisted infinite local route while held,
  use survivor-only materialization/fold, publish one exact marker/UI count, and
  never enter the legacy aggregate PhysicalWar activation/fold path. This does
  not migrate initial/enemy aggregate or vehicle garrisons.
- Civilian/undercover service with town support/reputation, wanted heat, police
  and roadblock presence/scans, aid effects, undercover eligibility,
  request/application, enforcement, compromise, and clear-state records.
  Current source separates true town centers from minor localities, decouples
  nearby civilian projection from HQ-suppressed hostile activation, selects
  distinct concrete appearances, defaults true towns to five driven vehicles,
  limits the known woodland locality to two pedestrians, and clears HST-owned
  ambient driver horn input. Republished behavior proof remains open.
- Command menu actions for setup hideout selection, dynamic mission targets,
  mission runtime and persistence inspection, HQ threat/Defend Petros reports,
  FIA support requests/cancel, support/enemy-order reports, civilian aid,
  undercover eligibility/request/check/clear, arsenal withdrawal, vehicle-cargo
  collection/unload, nearby vehicle garage capture, build-mode garage redeploy,
  HQ runtime-asset rebuild, marker audits, balance/pacing reports,
  campaign-end reports, command coverage checks, simple roster admin, and
  campaign reset
- Economy and enemy resource income now account for resource kind, priority,
  factories, ports, airfields, depots, radio towers, and police nodes
- Area and vehicle loot services with eligible-item scanning, base-game
  resource validation, blocked/finite-only/unlock policy checks, source item
  removal, HQ-object protection, and vehicle cargo reports
- Virtual garage scaffolding for safe root-vehicle capture, physical and
  virtual cargo preservation, verified despawn before record storage, dry-ground
  redeploy placement, runtime vehicle registration, and nearby field-vehicle
  snapshot/restore during checkpoint/restore flow
- Custom loadout editor path with server-authoritative HQ radius/member checks,
  live equipment and storage nodes, compatible candidate lists, five fixed
  personal save slots, `$profile:h-istasi/loadouts/v2` persistence, finite/INF
  cost ledgers, atomic apply/rollback, issued-item tracking, death-loss
  accounting, and removed external item purging
- Mission-specific convoy outcome state for delivered cargo/captives, captured
  vehicles, armored convoy garage handoff, ammo convoy ammo points, and outcome
  de-dupe across reloads
- Marker/UI polish with marker status/detail/audit reports, native marker
  publish diagnostics, command/report coverage audits, menu summaries, and
  clear failed-action text. Current source validates and dynamically resolves
  the radio icon instead of assuming an array index, includes location and owner
  in zone labels, keeps the map pointer above target-confirmation dialogs, and
  reuses authored radio transmitters for compositions and destroy targets.
  These repairs await packaged proof.
- Balance/pacing diagnostics for strategic score, control percentage, war-level
  thresholds, enemy pressure, victory readiness, and recommended next pressure
- Strategic victory/loss evaluation with persistent campaign-end reason,
  summary, elapsed time, control percent, war level, FIA/enemy zone counts, and
  campaign-end report generation state

## Current Verification Boundary

- The stamped Schema-54 tree passed foundation, Workbench Game creation/script
  validation, and bounded project-open gates. Those results are historical
  source evidence, not verification of current Schema-55 edits.
- A published schema-49 server/client check verified that normal stock HUD, Game
  Master access, map publication, and civilian traffic initialize again. This
  closes the earlier missing-config-metadata regression. The late-admin recursive
  role-change guard still needs a clean packaged check.
- The same run exposed eighteen invalid radio markers as giant colored boxes,
  location markers without names, and a map pointer rendered beneath the support
  confirmation dialog. It also showed a generated tower next to an existing
  transmitter. The marker-table, label, z-order, composition, and mission-target
  corrections exist only in current source and must be republished before they
  count as fixed.
- Civilian vehicles moved in that run, but town actors repeated one appearance,
  drivers held the horn, Figari and Morton projected no civilians because HQ
  safety cleared their shared military-active bit, and the known woodland
  locality projected full town ambience. Current source separates civilian
  eligibility, uses concrete variants, defaults true towns to five traffic
  vehicles, limits the minor locality to two pedestrians, and clears ambient
  horn input. Those outcomes are not yet runtime-proven.
- The schema-50 player exact-QRF strategic movement/materialization/virtual-
  combat path has deterministic fixtures and persistence coverage in source.
  The packaged schema-49 check did not accept an exact paid QRF, so it provides
  no operation, movement, fold, virtual-combat, or restart evidence for this
  slice. Schema 51 adds deterministic fixtures for exact enemy defensive-QRF
  admission, legacy isolation, projection, return/settlement, restore, and
  rejection, but none has executed in a packaged runtime. The stamped Schema-56
  tree identifies implementation `bab5748d817ba434dae701cfbb3b92805d463678`
  / label `schema56-exact-traitor-guard` / stamp
  `03a65cd33bee69c6320389803cdd5a2ec8576fb0`; foundation passes, Workbench Game
  validation loaded 5,764 files/11,573 classes with CRC `a18c67a5` and
  `Script validation successful`, and its bounded hidden normal WorldEditor open
  stayed alive for ten samples over 20 seconds without a latest-log script-error/
  crash signature. This is source/Workbench evidence, not packaged behavior proof.
- Schema 52 adds the exact mission-convoy aggregate in source. No packaged run has yet proved its
  three physical vehicles/crews, virtual route movement, materialization/fold,
  exact casualty persistence, arrival/outcome settlement, marker cleanup, or
  real process restart. The latest packaged convoy artifact predates this
  contract and therefore cannot certify it.
  Nine deterministic `mission_convoy.*` assertions cover admission and
  rollback, projection/fold gating, casualty-stable restore, settlement,
  open/settled/recovery restore, aggregate-marker cleanup, and the
  materialization watchdog. Admission/corruption subfixtures reject invalid
  cargo, foreign authority, invalid seat topology, forged arrival receipts,
  illegal lifecycle pairs, and non-member casualty roots while preserving
  missionless exact-looking durable claimants; they are not
  engine-backed proof.
- Schema 53 adds the exact enemy-patrol aggregate in source. No packaged run has
  proved proactive debit/refund, virtual outbound travel, physical
  materialization/fold, mapped casualty persistence, contact hold, one closed
  route lap, return, marker cleanup, or real process restart. Ten deterministic
  `enemy_patrol.*` assertions cover admission, collision-safe replay/refund,
  route loop, queue/roster bookkeeping, contact transition, settlement,
  physical-shaped restore, corruption, dispatch/priority isolation, and marker
  lifecycle. Production source additionally requires an exact root/member/
  PhysicalWar binding graph before movement, fold, save, or settlement; deleted
  bindings without observed death remain unresolved. Historical patrol rows remain contract
  version `0`; malformed current rows retain quarantine version `-53` without
  cross-owner fallback, guessed refund, or save while runtime ownership remains.
  These are source fixtures rather than engine-backed proof.
- Schema 54 adds the exact purchased-garrison patrol aggregate in source. Only a
  newly issued policy-v2 resistance purchase opts in; policy-v1 history and
  initial/enemy aggregate garrisons remain legacy. Source fixtures cover atomic
  admission, arbitrary members beneath an empty root, virtual infinite-loop
  cursor, survivor-only reprojection, PhysicalWar isolation, zero-refund owner-
  change/all-dead settlement, missing-live-runtime quarantine, marker/UI
  projection, and conservative `-54` restore quarantine through nine thematic
  `garrison_patrol.*` assertions. Production source also owns campaign-stop,
  setup, and typed route/spawn-failure terminal branches, but those branches do
  not yet have equivalent deterministic fixtures. No packaged run has proved native
  waypoint movement, observed casualties, fold/rematerialization, save/restart,
  client marker rendering, reconnect, or JIP for this slice.
- Schema 55 adds exact guard authority only for a newly started
  `assassinate_officer` mission. Source fixtures cover admission/legacy
  isolation, held-survivor projection, HVT-independent all-dead settlement,
  typed zero-refund outcomes, restore/migration, `-55` quarantine, and existing-
  HVT marker/UI status. Historical officer missions and every other mission
  family remain contract `0`; the HVT is not a manifest member or operation
  asset, and quarantine does not terminate it. No packaged run has proved native
  entity creation, real adapter bindings/casualties, physical fold/re-entry,
  actual save/restart, rendered status, owner-change, campaign setup, networking,
  reconnect, or JIP for this slice.
- Schema 56 adds exact guard authority only for a newly started
  `assassinate_traitor` mission at contract `2` and policy
  `exact_assassinate_traitor_guard_v1`. The six `traitor_guard.*` source-proof
  categories cover admission/family isolation, survivor projection/HVT
  separation, typed settlement, restore/pre-56 migration, `-56` conflict
  quarantine, and existing-HVT status. Officer contract `1` remains exact;
  historical/pre-56 traitor, spec-ops, and all other unsupported missions remain
  contract `0`. No packaged run has proved native entities, adapter casualties,
  actual save/restart, rendered UI, owner-change, setup, networking, reconnect,
  or JIP. The stamped implementation/Workbench evidence is recorded above and
  does not close those behavior gates.
- Schema 57 adds exact guard authority only for a newly started
  `assassinate_specops` mission at contract `3`, policy
  `exact_assassinate_specops_guard_v1`, and intent
  `assassinate_specops_guard`. Officer contract `1`/`-55` and traitor contract
  `2`/`-56` remain exact; historical/pre-57 spec-ops guards, unsupported
  families, and ordinary `mission_group_*` rows remain contract `0` and are not
  claimants. Generic spec-ops composition may propose multiple groups; contract
  `3` deterministically selects the strongest executable group, keeps the stable
  first group on ties, freezes only that selected catalog roster, and grants
  discarded groups no authority. The exact roster stays route-less and HVT-separated, preserves
  survivors/casualties through materialize-fold-re-entry, performs no virtual
  combat, and settles typed outcomes with zero refund. Restore records
  `migration_schema57_exact_specops_guard`; conflicting exact claimants
  quarantine at `-57` with
  `normalization_schema57_exact_specops_guard_conflict`. This source slice is
  stamped at implementation `514ebdcbeb1ddfb2a383b19590382517113e2ff6` under
  build label `schema57-exact-specops-guard`. Packaged/native/save-restart/
  rendered-UI/owner-change/setup/network/reconnect/JIP proof remains open.
- Schema 58 adds exact rescue authority only for a newly started `rescue_pows`.
  Six source categories cover admission/family isolation, composite guard-plus-
  captive authority, replay-safe captive transitions, guard independence,
  casualty/success/grace outcomes, and restore/quarantine. The queue executes
  only group/member slots; the rescue service owns three captive rows. Missing
  entities never imply death, HQ is the only extraction point, and configured
  target-town support uses the normal once-only reward path. Native/packaged/
  restart/rendered-UI/owner-change/setup/network/reconnect/JIP proof remains
  open.
- The non-cascade convoy artifact populated all three crew groups 2/2 but
  confirmed zero seated drivers through the full grace window. Current source
  registers each usable vehicle before seating, tries authority-local forced
  entry before the owner-RPC fallback, and probes retained registration
  directly. Current schema-52 source validation passes as described
  above, while 3/3 driver and movement proof remains open.
- Every real persistence capture now reconciles mapped physical exact-convoy,
  exact-patrol, and exact-mission-guard members first. An open outbound publication transaction,
  ambiguous mapping, or unverifiable live patrol position defers capture without
  flushing stale state or requesting a savepoint, retains intent, and retries on
  the bounded debounce. Real death-between-ticks, deferred-save retry, restore,
  and rematerialization still need packaged proof.
- Normal-play support evidence marked three groups `physical_arrived` while its
  logged targets and deterministic recall-exit vectors imply nominal current
  positions approximately 434m, 455m, and 505m away. Current source removes ETA-only completion, uses the living-member
  centroid, normalizes exact QRF handoff to `support_active`, and bounds direct
  target/exit waypoint reissue. A Phase 22 group populated 9/9 without observed
  advance, but campaign-time-only samples are not physical-stall proof. Fresh
  packaged support movement, arrival, and recall proof remains open.
- The same normal-play artifact independently showed a mixed QRF repeatedly
  reconciling at zero living infantry because its intact empty vehicle supplied
  the aggregate living count, including after the target changed ownership.
  The deterministic lifecycle proof now covers terminal state, capture pressure,
  QRF marker ordering, replay, roundtrip, and vehicle-only controls. Real entity
  detachment, player salvage, replication, and restart still need a disposable
  packaged runtime proof.
- The latest inspected Full Campaign Debug artifact predates schemas 43-57,
  contains a destructive save contamination and a large defense-probe cascade,
  and is not current certification evidence.
- The in-process runner now fails closed outside `HST_Dev`, clones campaign
  state, diverts checkpoints, and restores the original reference. Six known
  false-negative observations are repaired, but neither isolation nor those
  repaired rows have run in a fresh isolated artifact.

## Current Delivery Priorities

- Publish and test the stamped Schema-57 build with officer-, traitor-, and spec-ops-
  guard diagnostics while preserving the stamped Schema-56 checkpoint and
  proving the schema-50
  marker/dialog/radio corrections and
  preserving the already restored stock HUD and Game Master behavior. Require
  valid-sized icons, location-plus-owner labels, pointer-over-dialog ordering,
  one transmitter at authored sites, and correct radio destroy-target binding.
- Re-run civilian town/locality activation and require zero unregistered member-
  state RPCs, distinct appearances, five moving traffic vehicles per true town,
  two pedestrians at the minor woodland locality, no stuck horns, and Figari/
  Morton ambience independent of HQ-suppressed hostile activation.
- Prove campaign-debug isolation through completion, cancellation, interrupted
  recovery, and development-session restart, then replace the historical full
  artifact with corrected evidence.
- Runtime-prove the schema-43 through schema-57 authority chain: exact training,
  garrison, paid-QRF, queue/handoff, strategic travel, materialization/fold
  hysteresis, exact casualty/survivor transfer, bounded virtual combat,
  operation migration, settlement archive replay, typed recall receipt status,
  rejected paired-settlement conflicts, enemy defensive-QRF admission/legacy
  isolation/return/resource settlement, capacity, and save/restart idempotency.
- Runtime-prove the schema-52 exact convoy's frozen route, three vehicle/crew
  elements, vehicle-zero cargo/captive assignment, virtual travel, 3/3 drivers,
  physical interception, contact-to-clear transition, casualty-preserving fold/
  rematerialization, two-sample
  arrival, once-only outcome settlement, aggregate marker cleanup, and restart.
  Then use scoped disposable profiles to prove actual
  support movement, two-sample arrival within 75m, physical recall exit, and
  transactional waypoint reissue within the three-attempt bound. Include the
  crewless mixed-QRF case and require one neutral salvage detach, zero capture/
  marker pressure, no duplicate record, and restart-stable terminal state.
- Runtime-prove the schema-53 exact patrol: one proactive debit, one frozen
  infantry root, outbound virtual travel, physical materialization/fold, mapped
  casualties, contact-held progress, one closed generated-route lap, return,
  survivor refund, marker cleanup, and save/restart. Historical patrols must
  remain contract version `0` and quarantined rows must never enter legacy code.
- Runtime-prove the schema-54 exact purchased-garrison patrol: only new policy-v2
  resistance purchases opt in; exact arbitrary members remain under one empty
  root; virtual local looping, survivor-only physical fold/re-entry, owner-change/
  all-dead/campaign-stop/setup and typed spawn/route-failure zero-refund
  settlement, marker/UI cleanup, save/restart, reconnect, and JIP all preserve one
  roster/cursor. Historical policy-v1,
  initial/enemy aggregate, vehicle, and multi-root garrisons remain legacy.
- Runtime-prove the schema-55 exact officer-mission guard: only newly started
  `assassinate_officer` guards receive one route-less empty-root/member roster.
  Prove native materialization/casualties/fold, all-guards-dead with an active
  HVT, every typed zero-refund outcome, `-55` quarantine without fallback or HVT
  failure, compact settlement, existing-HVT marker/UI status, save/restart,
  owner-change, campaign setup, networking, reconnect, and JIP. Historical
  officer missions remain contract `0`; the separately opted-in traitor path must
  not alter officer contract `1`.
- Runtime-prove the schema-56 exact traitor-mission guard: only newly started
  `assassinate_traitor` guards receive contract `2` and policy
  `exact_assassinate_traitor_guard_v1`. Prove the route-less exact roster, HVT
  separation, survivor materialization/fold, typed zero-refund outcomes, compact
  restore, `-56` quarantine, existing-HVT status, native entities/adapter
  casualties, actual save/restart, rendered UI, owner-change, setup, networking,
  reconnect, and JIP. Officer contract `1` must remain exact; historical/pre-56
  traitor and all other unsupported families must remain contract `0`. The
  Schema-57 spec-ops path must not alter traitor contract `2`.
- Runtime-prove the schema-57 exact spec-ops-mission guard: only newly started
  `assassinate_specops` guards receive contract `3`, policy
  `exact_assassinate_specops_guard_v1`, and intent
  `assassinate_specops_guard`. Prove the route-less exact roster, HVT
  separation, survivor materialization/fold/re-entry, casualty-stable restore,
  typed zero-refund outcomes, compact restore, `-57` conflict quarantine,
  existing-HVT status, and the no-virtual-combat boundary. Then prove native
  entities/adapter casualties, actual save/restart, rendered UI, owner-change,
  setup, networking, reconnect, and JIP. Officer contract `1` and traitor
  contract `2` must remain exact; historical/pre-57 spec-ops guards, ordinary
  `mission_group_*` rows, and unsupported families must remain contract `0`.
- Prove static marker widget readiness and implement authoritative host/client/
  late-join snapshot, revision, delete, acknowledgement, and resync behavior.
- Runtime-prove both exact-QRF `OperationRecord` policies: player virtual combat/
  recall/archive and enemy defensive arrival/return/proportional settlement,
  including physical/virtual transfer, marker cleanup, and restore for both.
  Then connect live physical contact/disengagement and deepen encounter
  simulation without treating source implementation as packaged proof.
- Continue the implementation blueprint with one explicitly versioned mission-
  force consumer at a time. Schema 57 exhausts assassination guards and Schema
  58 implements only the first rescue slice. Other rescue and unsupported
  mission families remain legacy. Packaged schema-50 through schema-58
  certification remains independently required.

## Next Playable Expansion

- Restart/migration testing for `HST_CampaignSaveData` under native Reforger
  save/load, including active support/order/Defend Petros and won/lost saves
- HST_Dev end-to-end smoke and UX polish for the custom loadout editor,
  including live inventory edge cases, death-loss accounting, and save/load
  confidence
- Garage/source-vehicle soak for capture, cargo preservation, redeploy,
  ammo/repair/fuel classification, runtime vehicle restore, and longer restart
  testing
- Mission-family polish that replaces remaining physical MVP shortcuts with
  mission-specific props, richer hold/clear checks, and better objective text
- Richer active-group waypoints/routes for QRFs, support, counterattacks,
  Defend Petros attackers, and survivor fold-back into abstract garrisons
- Tune Phase 24 starting training, war-level thresholds, enemy income scaling,
  aggression decay, victory/loss thresholds, and pacing recommendations through
  repeated runs
- Phase 25 repeated campaign loops and long-soak testing across HST_Dev and
  HST_Everon begin only after the Runtime Integrity evidence above is reliable

## Later Alpha Increments

- Exact full-Everon coordinate survey and replacement of remaining 4x-style
  alpha anchors once a Workbench/PAC extraction path is available
- Authored h-istasi HQ entities for cache/tent polish and customized Petros
  appearance/loadout
- Full player-facing member, guest, commander election, and admin UI polish
- Deeper town, factory, resource, radio tower, police, and city-flip behavior
  beyond the current broad-alpha support/enforcement layer
- Mission-specific world logic and unique content for every registry entry
- Explicit dispositions and implementations for intelligence/reveal,
  interrogation/informants, respawn/revive, fast travel, construction/
  fortifications, radio/intelligence networks, player squads/high command, and
  medical/logistical recovery; dependent actions and missions stay disabled
  when a system is intentionally deferred
- Larger multiplayer and long-duration campaign soak, including 16-player runs

## Deferred Capabilities

The base-game preset must disable mechanics that cannot be honestly
represented with stock Arma Reforger resources. Do not add hidden dependencies
or synthetic replacements.

- Fixed-wing aircraft support
- SEAD support when no suitable radar and SAM asset pair is available
- Any artillery support without a suitable physical base-game asset
