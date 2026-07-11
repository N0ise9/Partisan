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
- Schema-50 campaign authority foundation with persisted monotonic IDs, typed
  command receipts, resource transactions, exact force quotes/manifests,
  durable per-projection SpawnQueue state, exact paid-infantry-QRF runtime
  lifecycle, bounded accepted-settlement replay tombstones, and the first
  canonical operation aggregate with persistent strategic projection
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
  and every other operation family remain open. Four projection assertions join
  the existing eight operation-record assertions, but none is packaged runtime
  proof until executed there.
- Typed support-recall completion across service, coordinator, visible-command
  dispatch, durable receipt, and diagnostics. Accepted terminal wording cannot
  be reclassified by presentation text; exact paired full refunds prevalidate
  both ledger legs before either changes. Other visible commands still retain
  the compatibility text classifier.
- Exact visible garrison recruitment quotes and exact player-paid infantry-QRF
  quotes. Garrison confirmation mutates purchase-time strategic state; paid QRF
  submits the same accepted one-group/member manifest unchanged to the engine
  adapter. Vehicle/asset/multi-root realization and remaining paid supports are
  not yet migrated.
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
  not migrated by this slice.
- Enemy commander service with patrol/QRF/search, counterattack, rebuild,
  support-call, and Petros attack orders that track physicalized or abstract
  runtime state and outcome application
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

- The prior stamped schema-49 source passed its foundation, Workbench Game
  creation, script-validation, and bounded project-open gates. Those results are
  historical source evidence, not certification of current schema-50 edits.
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
- The current schema-50 exact-QRF strategic movement/materialization/virtual-
  combat path has deterministic fixtures and persistence coverage in source.
  The packaged schema-49 check did not accept an exact paid QRF, so it provides
  no operation, movement, fold, virtual-combat, or restart evidence for this
  slice. Repository foundation validation and current Workbench script
  validation pass; the Game module loads 5,747 files/11,508 classes with CRC
  `16665f19`, and the correctly launched WorldEditor remains responsive through
  the bounded 20-second gate. Packaged behavior remains open.
- The non-cascade convoy artifact populated all three crew groups 2/2 but
  confirmed zero seated drivers through the full grace window. Current source
  registers each usable vehicle before seating, tries authority-local forced
  entry before the owner-RPC fallback, and probes retained registration
  directly. Current schema-50 compile/startup validation passes as described
  above, while 3/3 driver and movement proof remains open.
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
- The latest inspected Full Campaign Debug artifact predates schemas 43-49,
  contains a destructive save contamination and a large defense-probe cascade,
  and is not current certification evidence.
- The in-process runner now fails closed outside `HST_Dev`, clones campaign
  state, diverts checkpoints, and restores the original reference. Six known
  false-negative observations are repaired, but neither isolation nor those
  repaired rows have run in a fresh isolated artifact.

## Current Delivery Priorities

- Publish and prove the schema-50 marker/dialog/radio corrections while
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
- Runtime-prove the schema-43 through schema-50 authority chain: exact training,
  garrison, paid-QRF, queue/handoff, strategic travel, materialization/fold
  hysteresis, exact casualty/survivor transfer, bounded virtual combat,
  operation migration, settlement archive replay, typed recall receipt status,
  rejected paired-settlement conflicts, capacity, and save/restart idempotency.
- Runtime-prove 3/3 convoy drivers after the pre-seat registration and local-
  authority entry repair. Then use scoped disposable profiles to prove actual
  support movement, two-sample arrival within 75m, physical recall exit, and
  transactional waypoint reissue within the three-attempt bound. Include the
  crewless mixed-QRF case and require one neutral salvage detach, zero capture/
  marker pressure, no duplicate record, and restart-stable terminal state.
- Prove static marker widget readiness and implement authoritative host/client/
  late-join snapshot, revision, delete, acknowledgement, and resync behavior.
- Runtime-prove the schema-50 exact-QRF `OperationRecord` projection slice, then
  connect live physical contact/disengagement and deepen encounter simulation
  before extending exact runtime authority to vehicles, assets, garrisons,
  missions, enemy orders, or other supports.

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
