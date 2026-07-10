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
- Schema-48 campaign authority foundation with persisted monotonic IDs, typed
  command receipts, resource transactions, exact force quotes/manifests,
  durable per-projection SpawnQueue state, exact paid-infantry-QRF runtime
  lifecycle, and bounded accepted-settlement replay tombstones
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
  unsupported by base-game assets
- Enemy commander service with patrol/QRF/search, counterattack, rebuild,
  support-call, and Petros attack orders that track physicalized or abstract
  runtime state and outcome application
- Civilian/undercover service with town support/reputation, wanted heat, police
  and roadblock presence/scans, aid effects, undercover eligibility,
  request/application, enforcement, compromise, and clear-state records
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
  clear failed-action text
- Balance/pacing diagnostics for strategic score, control percentage, war-level
  thresholds, enemy pressure, victory readiness, and recommended next pressure
- Strategic victory/loss evaluation with persistent campaign-end reason,
  summary, elapsed time, control percent, war level, FIA/enemy zone counts, and
  campaign-end report generation state

## Current Verification Boundary

- Foundation validation and schema-48 Workbench Game compilation/creation pass
  at 5,739 files and 11,472 classes.
- A separate normal WorldEditor project open survived the bounded 20-second
  crash gate without a script-error or crash signature.
- The latest inspected Full Campaign Debug artifact predates schemas 43-48,
  contains a destructive save contamination and a large defense-probe cascade,
  and is not current certification evidence.
- The in-process runner now fails closed outside `HST_Dev`, clones campaign
  state, diverts checkpoints, and restores the original reference. Six known
  false-negative observations are repaired, but neither isolation nor those
  repaired rows have run in a fresh isolated artifact.

## Current Delivery Priorities

- Prove campaign-debug isolation through completion, cancellation, interrupted
  recovery, and development-session restart, then replace the historical full
  artifact with corrected evidence.
- Runtime-prove the schema-43 through schema-48 authority chain: exact training,
  garrison, paid-QRF, queue/handoff, exact casualties, survivor reprojection,
  settlement archive replay, capacity, and save/restart idempotency.
- Reproduce and repair genuine convoy seating/movement, support routing,
  physical response, arrival/contact, return, and recall failures with scoped
  disposable debug profiles.
- Prove static marker widget readiness and implement authoritative host/client/
  late-join snapshot, revision, delete, acknowledgement, and resync behavior.
- Add durable operation assignment/duty/engagement/materialization state before
  extending exact runtime authority to vehicles, assets, garrisons, and other
  support consumers.

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
