# h-istasi

h-istasi is an original Arma Reforger resistance-campaign implementation. The
first preset targets Everon with a three-way war:

- FIA: player resistance
- US: occupying force
- USSR: invading force

## Current Status

The repository contains a broad-alpha campaign foundation:

- Reforger base-game project dependency only
- APL-ND licensing and third-party attribution
- Original Everon and compact development scenario shells
- Data contracts for presets, factions, maps, zones, balance, and missions
- A versioned campaign-state model with arsenal, vehicle cargo, garage,
  saved-loadout, issued-item, mission-runtime, active-group, support, exact
  infantry/convoy/rescue operations, radio-site lifecycle, build-mode,
  campaign-end, and persistence metadata
- Server-authoritative campaign, economy, mission, persistence, arsenal, loot,
  loadout-editor, garage/build, and native-checkpoint services
- A 39-entry configured mission-registry baseline
- Custom FIA HQ player spawn path for direct Workbench Play mode
- FIA spawnpoint/loadout authoring metadata for authored hideout candidates
- Setup-phase HQ selection, HQ movement, and Petros state hooks for the first
  playable resistance-campaign slice
- Abstract campaign systems for player lifecycle, town income/support, zone
  capture, garrisons, recruitment/training, and mission rewards/penalties
- Native respawn-request bootstrap with pending spawn tracking to avoid
  Workbench duplicate-spawn loops
- Everon alpha anchors, generated-route response movement, routed infantry
  move/sweep waypoints, mixed response vehicle spawning, and route-aware zone
  activation scaffolding for the physical AI war
- Dedicated Petros character prefab at
  `Prefabs/Characters/HST/Character_HST_Petros.et` for HQ contextual actions,
  with GUID metadata and a base-FIA spawn fallback only if the custom resource
  fails to spawn
- HST-owned HQ arsenal action surface at
  `Prefabs/Objects/HST/HST_HQArsenal.et`; stock FIA arsenal/MSAR behavior is
  not used for item authority, issuing, or loadout accounting, and inherited
  stock arsenal actions are filtered away from the h-istasi action surface
- Custom h-istasi loadout editor with a client fullscreen widget, server-built
  payloads, live equipment/storage nodes, compatible candidate replacement,
  five fixed personal loadout slots under `$profile:h-istasi/loadouts/v2`,
  finite/INF accounting, atomic apply/rollback, and an issued-item ledger owned
  by campaign state
- Procedural h-istasi `I` key HQ menu mounted on both development
  worlds with Setup, Overview, HQ/Petros, Missions, Map/War, Forces,
  Arsenal/Loot, Garage/Build, Members, and Admin tabs
- First-load server settings generation at `$profile:h-istasi/HST_Settings.json`
  with config-backed defaults for campaign, factions, economy, membership,
  world activation, arsenal/loot, persistence, logging, and feature toggles,
  including configurable infinite stamina, live resistance support group map
  tracking, and population-based campaign outcomes
- Server-side area and vehicle loot actions that deposit eligible nearby gear
  into the campaign arsenal or vehicle cargo, remove transferred source items
  when configured, and reject raw visual/support assets
- Virtual garage and build-mode scaffolding for nearby vehicle capture,
  safe-root validation, cargo preservation, dry-ground redeploy placement,
  field-vehicle snapshot/restore, and HQ runtime-asset rebuilds
- Broad-alpha campaign scaffolding for generated Everon sites/routes, mission
  objectives, campaign tasks, physical mission-runtime primitives, support
  requests, enemy commander orders, civilian town state, and player undercover
  state
- HQ knowledge/threat state and Defend Petros linkage across mission, enemy
  order, support request, attacker-group, marker, and campaign-end state
- Enemy commander orders and support requests that can physicalize near players
  or resolve abstractly off-screen while preserving runtime status and outcome
  diagnostics
- Physical enemy response fold-back for support/QRF groups that leave the event
  bubble, preserving survivor state while removing live runtime handles
- Mixed personnel/vehicle active groups now use living personnel—not an intact
  empty vehicle—as combat-effectiveness authority. Once previously observed
  personnel reach zero outside population grace, the group becomes terminal,
  contributes no capture pressure, retires its linked QRF marker, and preserves
  an intact attached vehicle as neutral salvage. Unadopted vehicles are session-
  only; an existing durable field record and cargo remain durable. Terminal zero-survivor
  truth is retained across save normalization; the bounded lifecycle proof is
  implemented, while real combat/salvage/restart observation remains open.
- Physical support route truth that keeps spawned support state on the live
  member centroid, treats ETA as the earliest arrival observation rather than
  completion, requires two live samples from distinct elapsed seconds within
  75m for arrival or
  recall exit, and rebuilds direct current-to-target/exit waypoint chains with
  at most three consecutive reissues without an 8m new-best distance
  improvement. Pre-repair restored arrival/exit rows receive one current live-
  distance revalidation. Fresh
  packaged movement, arrival, and recall proof remains open.
- Town influence event ledger with support, reputation, heat, population, and
  security-pressure deltas feeding political ownership flips and debug reports
- Runtime and garage vehicle heat/report state for undercover vehicle cover,
  with passenger compromise counts, report expiry, capture/redeploy handoff,
  save-data preservation, and one-button debug proof
- Request-driven force composition for support, mission, garrison, and debug probes, with
  serializable intent, tier, cost, manpower, vehicle-plan, skipped-prefab, and
  failure metadata retained on support, enemy-order, and active-group records
- A schema-52 force/operation authority boundary with durable per-projection results,
  explicit force/projection identity on active groups, exact required-slot
  admission, bounded priority/FIFO scheduling and retention, retry/deadline/
  cancellation handling, dependency-ordered cleanup, Game Master registration
  evidence, once-per-restore reconciliation, and canonical operation records
  with persistent strategic projection for confirmed exact paid infantry QRFs,
  newly planned exact enemy defensive QRFs, and newly started mission convoys
- Bounded accepted-settlement archives that replace eligible full quote,
  manifest, and linked ledger rows only after terminal aggregate proof and all
  physical/queue backlinks are gone. Compact rows preserve issue, confirmation,
  and committed-ledger replay for at least 86,400 campaign seconds; new planning
  fails closed when the shared history bound cannot safely make room. Capacity
  eviction applies full receipt reciprocity to every positive typed player-
  support contract, including exact QRF and Search-and-Destroy, and removes its
  tombstone/terminal-request pair only when replay validity, unique aggregate
  identity, and absence of live backlinks also pass. Historical contract-0 QRF
  keeps its minimal compatibility match; corrupt or quarantined typed pairs
  remain as evidence.
- An engine-facing force-spawn adapter driven by the production coordinator once
  per active-campaign second. Its first exact slice creates one infantry group
  root plus every frozen member slot, records a durable nonterminal
  `READY_FOR_HANDOFF` boundary, finalizes the projection in physical war before
  recording queue success, and prevents older group-population paths from
  duplicating queue-owned projections
- The first exact player-paid support slice: selecting a QRF map target issues
  an immutable two-step server quote, confirmation charges a flat $250 plus one
  HR for every authored catalog member through linked ledger transactions, and
  the accepted one-group manifest is submitted unchanged to SpawnQueue. Failure
  and pre-success cancellation refund both transactions once; recall settles
  eligible HR through the same ledger.
- Support recall now returns a typed acceptance, mutation, terminal,
  disposition, request, and operation result. It is the first production
  visible command whose durable receipt status comes from that result instead
  of presentation wording. Exact full-refund settlement prevalidates both
  linked money and HR transaction identity, state, and deterministic settlement
  replay before refunding either transaction.
- Schema 50 advances each newly confirmed exact paid infantry QRF's versioned
  `OperationRecord`: immutable origin and assignment, mutable tactical target,
  typed duty/engagement/materialization/position/settlement state, stable
  execution links, revision, terminal settlement identity, direct-route cursor,
  strategic position, projection-decision evidence, and deterministic virtual-
  combat clocks. The first consumer can travel while virtual, materialize near a
  living player, fold clear of contact outside a larger radius, preserve exact
  survivor slots, and resolve bounded infantry-only combat against its target's
  abstract garrison. Physical combat contact, vehicles/assets, other operation
  families, and packaged runtime/restart proof remain open.
- Schema 51 makes newly planned infantry-only enemy defensive QRF the first
  exact enemy-order consumer. One distinct same-faction operational source, a
  same-faction defended target under resistance pressure, prepaid frozen roster,
  order, operation, held batch, and active group stay linked through
  strategic outbound travel, proximity materialization/fold, exact casualties,
  once-only defensive pressure, return to origin, and survivor-proportional
  attack/support settlement. Six `enemy_qrf.*` fixtures cover the source
  contract. Existing orders and every other order family remain legacy, and
  packaged movement/return/restart proof is still open.
- Schema 52 makes each newly started convoy mission a version-1 exact mission
  operation. Admission freezes one generated road route, exactly three vehicle
  slots, three linked crew groups, their durable crew-member slots, and at most
  one mission cargo/captive asset assigned to vehicle slot zero. Money and
  supplies require exactly one compatible cargo row, prisoners require exactly
  one compatible captive row, and ammo, armored, and reinforcement convoys
  forbid a separate cargo row. Admission rejects an unsupported role/kind,
  duplicate row, or prefab that is not a loadable mission-asset entity source.
  Captives must be boardable character prefabs with compartment access;
  ordinary payloads must be non-character entities. The convoy
  advances along its persisted route while virtual, materializes as an
  interceptable three-vehicle column near players, folds only when clear of
  contact and player/mission ownership, and resumes from its last authoritative
  route cursor with confirmed crew casualties still retired. A destroyed or
  captured vehicle with living crew becomes a stationary crew-only root: those
  exact survivors fold and rematerialize from their own durable position, while
  the terminal vehicle is never recreated. Member deaths are retained by exact
  frozen slot/entity identity rather than inferred from an aggregate count, so a
  dead early seat cannot respawn in place of a later survivor. Bubble decisions consider every
  separated living/recoverable root rather than only the aggregate convoy
  marker. Arrival and
  settlement now come from route/crew/outcome authority rather than the legacy
  travel timer. Crew elimination does not prematurely close a money, prisoner,
  ammo, armored, or other recovery-dependent mission: unresolved cargo/captive/
  vehicle state remains in an on-station recovery hold that can virtualize and
  rematerialize until its mission-specific outcome is resolved. Unresolved
  intact crewless vehicles can also fold, reappear, and be captured while other
  convoy crews are still fighting; the global recovery hold is not a prerequisite.
  Cargo on a destroyed or captured frozen carrier remains a ground recovery at
  that carrier's durable position; it is never reassigned. Restored
  historical convoy missions remain contract version
  `0` on the prior path. The source boundary is validated; packaged movement,
  intercept, fold/rematerialization, outcome, and restart proof remains open.
  Nine deterministic `mission_convoy.*` assertions cover admission and rollback,
  projection/fold gating, casualty restore, settlement, open/settled/recovery
  restore, aggregate-marker cleanup, and the materialization watchdog in source.
  Their admission/corruption fixtures additionally reject invalid cargo,
  duplicate or foreign authority, invalid seat topology, forged arrival
  receipts, illegal lifecycle pairs, and casualty authority on non-member roots,
  while preserving missionless exact-looking claimants as durable evidence.
  They are not packaged runtime evidence.
- Schema 53 makes each newly queued enemy `PATROL` order a version-1 exact
  enemy-patrol operation. Admission freezes one infantry root and its ordered
  member slots, debits proactive attack resources once, and links one order,
  generated route, operation, held batch, and active group. A persisted route
  cursor owns outbound travel, one closed on-station lap, and the distinct
  return-to-origin leg. The same exact roster moves between virtual and physical
  projection; mapped casualties remain retired across fold, reprojection, and
  restore. Physical contact holds the route clock until clear, preventing
  off-bubble catch-up from skipping the contact window. Return settlement refunds
  only the surviving fraction of the proactive attack debit and is replay-safe.
  Type plus contract-version dispatch keeps exact patrols separate from exact
  defensive QRFs and legacy orders. Historical patrol rows remain contract
  version `0`; malformed current-schema patrol authority is retained under
  quarantine version `-53` and never falls back to legacy or another exact owner.
  Physical movement, fold, save, and campaign-stop settlement now require a
  unique root/member adapter-to-PhysicalWar binding graph. A deleted binding with
  no observed death remains unresolved and cannot become a guessed casualty or
  survivor refund. Quarantine capture waits until both runtime registries are
  empty, then retains the diagnostic graph without refund. Ten
  deterministic `enemy_patrol.*` assertions cover admission, replay/refund,
  route loop/return, queue/roster transfer, contact transition, settlement,
  physical-shaped restore, corruption, dispatch/priority isolation, and marker
  lifecycle.
  These are source fixtures, not packaged gameplay proof.
- Schema 54 makes only newly issued policy-v2 purchased resistance garrisons
  executable exact patrol operations. Each accepted quote freezes one
  non-self-populating infantry group root plus the arbitrary ordered member
  slots actually purchased, then links the manifest to one
  `HST_OPERATION_TYPE_GARRISON_PATROL`, held SpawnQueue batch, local generated
  route, and active-group projection. The roster remains virtual and walks its
  persisted local loop indefinitely until a player enters the materialization
  bubble; physical fold and re-entry retain exact casualties and release only
  survivors. PhysicalWar excludes these groups from every legacy garrison
  activation, composition, patrol, fold, survivor, and cleanup owner. The
  purchase is never refunded: owner change, all members dead, campaign stop,
  setup, or typed spawn/route failure settles one replay-safe
  `exact_garrison_patrol_terminal` receipt and retires the projection. Historical
  policy-v1 purchases, initial-map and enemy
  aggregate garrisons, vehicles, and multi-root forces remain on the legacy
  aggregate path. Schema-54 restore invents no exact authority for those rows;
  malformed current exact-authority graphs retain evidence under quarantine
  version `-54`.
  One operation marker reports location, current owner, patrol role, and durable
  survivors, and the Forces UI distinguishes exact-patrol and legacy infantry.
  Nine deterministic `garrison_patrol.*` assertions cover admission, replay/
  rollback, roster projection, route loop, projection/casualty hold, settlement,
  restore, corruption quarantine, and marker lifecycle. These are source
  fixtures. The stamped Schema-54 tree identifies implementation
  `09a1470a4c27dbef866e8cbdba182a7df65fa027` and passes clean headless Workbench
  Game-module compile/create at 5,760 files/11,560 classes with CRC `c62de929`;
  its normal WorldEditor open remained alive for all ten samples over 20 seconds.
  Those gates provide source/Workbench proof only. Packaged native movement,
  casualty, fold/rematerialization, save/restart, marker rendering, networking,
  and JIP evidence remains open.
- Schema 55 makes only newly started `assassinate_officer` guard infantry an
  exact, route-less mission operation. One empty execution root and its ordered
  catalog members remain a held survivor roster across materialization, mapped
  casualties, fold, and re-entry. It has no generated route or virtual-combat
  clock: while abstract it remains on station at a deterministic offset from the
  objective, and re-entry realizes only durable survivors. The HVT stays
  separate mission-objective/runtime-asset authority and is never a manifest
  member, operation asset, or group backlink. Eliminating every guard settles
  only the guard operation as `DESTROYED`, so the HVT mission can remain active.
  HVT success settles surviving guards `COMPLETED`; mission failure/expiry,
  campaign stop, or setup settles them `CANCELLED`; target-owner change is
  `INVALIDATED`; and coherent spawn/assignment failure is `SPAWN_FAILED`. Every
  path records the fixed terminal receipt once with zero refund or legacy-force
  transfer. Historical officer missions, the other assassination variants, and
  all remaining mission families stay contract `0`. Schema-55 restore invents
  nothing for those rows, accepts compact settled exact graphs, and quarantines
  malformed current authority at `-55` without a legacy fallback, guessed
  casualty, HVT backlink, or refund. Quarantine leaves the HVT mission playable
  and projects a diagnostic instead of substituting legacy guards.
  The existing HVT marker/UI row reports authoritative guard strength; no
  second marker is added. Deterministic fixtures are source evidence only;
  the stamped Schema-55 tree identifies implementation
  `552c2c4ff5ac7608fa248c614480a254769b61a4`, passes the full foundation gate,
  and passes clean Workbench Game validation at 5,763 files/11,570 classes with
  CRC `0ec8950e`; a normal WorldEditor open also remained alive for ten samples
  over 20 seconds. Native entity/adapter/casualty behavior, actual save/restart,
  rendered marker/UI, owner-change, campaign-setup, packaged networking,
  reconnect, and JIP proof remain open for this slice.
- Schema 56 extends that same mission-guard operation type only to guard infantry
  belonging to a newly started `assassinate_traitor` mission. Traitor guards use
  contract version `2`, manifest policy `exact_assassinate_traitor_guard_v1`, and
  quarantine version `-56`; the Schema-55 officer contract remains version `1`
  with `-55` quarantine. The traitor roster is likewise one route-less empty root
  plus exact ordered members, keeps its HVT separate, materializes/folds only
  durable survivors, settles every typed outcome with zero refund, accepts the
  same compact settled shape, and appends strength to the existing HVT marker/UI
  rather than adding a marker. Pre-56 and historical traitor missions,
  `assassinate_specops`, and every other mission family remain contract `0`.
  Migration records `migration_schema56_exact_traitor_guard` without inventing
  authority; malformed current rows record
  `normalization_schema56_exact_traitor_guard_conflict` and remain diagnostic
  under `-56` without fallback or HVT failure. Six focused source-proof
  categories cover admission/isolation, projection lifecycle, settlement,
  restore/migration, quarantine, and marker status. The stamped Schema-56 tree
  identifies implementation `bab5748d817ba434dae701cfbb3b92805d463678`
  and stamp `03a65cd33bee69c6320389803cdd5a2ec8576fb0`
  (`schema56-exact-traitor-guard`), passes the full foundation gate, and passes
  Workbench Game validation at 5,764 files/11,573 classes with CRC `a18c67a5`
  and `Script validation successful`. Its bounded hidden normal WorldEditor open
  stayed alive for all ten samples over 20 seconds, and the latest log had no
  script-error or crash signature. These are source/Workbench gates, not packaged
  behavior proof. Native entities/adapter casualties, real save/restart, rendered
  UI, owner-change, campaign setup, packaged networking, reconnect, and JIP remain
  unclaimed.
- Schema 57 completes the assassination-guard family by opting in only guard
  infantry created for a newly started `assassinate_specops` mission. Spec-ops
  guards use contract `3`, manifest policy
  `exact_assassinate_specops_guard_v1`, intent `assassinate_specops_guard`, and
  quarantine version `-57`. Officer contract `1`/`-55` and traitor contract
  `2`/`-56` remain exact and unchanged. Historical or pre-57 spec-ops missions,
  ordinary `mission_group_*` rows, and every unsupported family remain contract
  `0`. A generic spec-ops composition may propose multiple groups; contract `3`
  deterministically selects the strongest executable group, keeps the stable
  first group on ties, and freezes only that selected catalog roster. Discarded
  groups gain no authority. The new family reuses the same route-less empty-root/member roster,
  separate HVT authority, survivor-only materialize/fold/re-entry lifecycle,
  no-virtual-combat rule, typed zero-refund settlement, compact restore, and
  existing-HVT status. Migration records
  `migration_schema57_exact_specops_guard`; malformed current rows record
  `normalization_schema57_exact_specops_guard_conflict` and remain diagnostic at
  `-57`. Six `specops_guard.*` source-proof categories cover the boundary.
  Schema 57 is stamped at implementation
  `514ebdcbeb1ddfb2a383b19590382517113e2ff6` with build label
  `schema57-exact-specops-guard`. The full foundation gate passes, including
  the Schema-55/56/57 checks. Stamped Workbench Game validation loaded 5,765
  files/11,576 classes with CRC `e0b8578e` and `Script validation successful`;
  the bounded hidden normal WorldEditor open stayed alive for 10/10 samples
  over 20 seconds, and its log had no script-error/crash signature. These are
  source/Workbench gates only. Native entities, adapter casualties, actual
  save/restart, rendered UI, owner change, campaign setup, packaged networking,
  reconnect, and JIP remain open.
- Schema 58 adds a separate exact mission-rescue operation only for newly
  started `rescue_pows`. Contract `1`, policy `exact_rescue_pows_v1`, intent
  `rescue_pows_guard`, and quarantine `-58` own one frozen composite manifest:
  one catalog-backed hostile guard root/member roster plus exactly three
  externally projected captive slots. Captives use typed HELD/FREED/FOLLOWING/
  BOARDING/BOARDED/EXTRACTED/KILLED state with stable escort, carrier, seat,
  casualty, extraction, and projection evidence plus a bounded command
  ledger keyed by request/actor/command/revision. Replays are resolved before
  live proximity/HQ checks, cross-captive or changed-fingerprint reuse fails
  closed, accepted and the bounded recent rejected results remain stable across
  later disposition changes, rejection spam cannot block legal progression,
  and one slot is reserved for terminal extraction. Unbound HELD/FREED
  captives and the guard roster can fold outside the player bubble; custody
  states remain projected, and missing runtime entities never imply death.
  Outside grace a disconnected escort releases custody back to FREED without
  inventing a casualty; during grace disconnect invalidates the frozen custody
  set. Captive context actions use a replicated, fail-closed legality DTO so a
  client or late joiner does not expose simultaneous illegal actions while its
  local coordinator state is absent.
  Guard elimination does not complete the rescue, any observed captive death
  fails it, and exactly three HQ extraction receipts complete it through the
  normal mission reward path. The HQ extraction position is frozen at admission
  and blocks HQ relocation while open. A five-minute expiry grace opens only
  when all three living captives were already in custody, uses the frozen base
  deadline plus 300 seconds, and forbids new claims. Pre-58
  POW missions and `rescue_refugees` remain contract `0`; malformed current
  authority quarantines without invented captives, deaths, extractions, rewards,
  or fallback. Schema 58 is an earlier stamped source/Workbench baseline at
  implementation `f0ba07ff2bc295d12542a3ea34b4c913e99b1869` with build label
  `schema58-exact-rescue-pows`. The full foundation gate passes. Final stamped-
  tree Workbench Game validation loaded 5,770 files/11,594 classes with CRC
  `aa73883a` and `Script validation successful`; the bounded hidden normal
  WorldEditor open stayed responsive for 10/10 samples over 20 seconds with zero
  crash/error matches. These remain source/Workbench gates only. Packaged/native/
  restart/rendered-UI/owner-change/setup/network/reconnect/JIP proof remains
  open.
- Schema 59 replaces the earlier best-effort radio-tower overlap guards with one
  durable radio-site authority per radio zone. One deterministic site/target
  row owns ONLINE, DESTROYED, REBUILDING, or QUARANTINED state; an immutable
  authored prefab/position descriptor distinct from the current projection;
  borrowed-world versus campaign-generated ownership; reciprocal
  mission lock; typed request fingerprint; revision; timestamps; and
  destruction/rebuild receipts. Zone composition and generic mission runtime
  are fenced out for exact/quarantined rows. An authored transmitter is borrowed
  without deletion, ambiguous candidates fail closed before mutation, and a
  missing process handle never means destroyed. The stable site target identity
  is distinct from each mission's unique physical runtime-entity identity.
  Borrowed authored-target destruction requires the reciprocal active
  mission/site lock and revision, the tracked damage manager in DESTROYED state,
  and a projection near the frozen site. Generated-target explosive scoring also
  requires a live matching mission-asset component, a bounded physical
  position, and a unique key in the persisted bounded evidence set. Each exact
  mission asset also freezes the ownership and authored descriptor it admitted
  against, so a later ownership handoff cannot relabel demolition evidence.
  Health/destroy/reset writes are checked after mutation; a failed destroy,
  heal, or rollback quarantines or rejects the transition instead of claiming
  an outcome that the world did not accept. Authored identity uses a tight
  0.75-meter match, while a 12-meter physical-projection tolerance accounts for
  bounded safe-ground placement without weakening the frozen binding. Stop-
  rebuild uses destructible construction equipment, not a duplicate intact
  transmitter, and may be admitted only once
  per tower-destruction epoch; stopping that equipment records the attempt but
  does not manufacture a new tower-destruction epoch. The supported authored
  transmitter has retained multiphase damage behavior; permanent generated
  ONLINE projections disable verbose witness logging and keep nearby-entity
  scans dormant until an exact mission identity is configured. Only resolved
  ONLINE sites emit town radio influence, and zone/mission markers surface the
  exact lifecycle next to the existing location and owner label. A new-campaign
  reset restores the borrowed authored transmitter's health before replacing
  state or fails closed. A streamed-out borrowed target keeps its exact mission
  aggregate dormant in `radio_site_projection_pending` (or preserves
  `radio_site_target_destroyed` after evidence) until the same target returns;
  missing/cross-linked runtime rows quarantine. Generic runtime, composition,
  objective ticking, commander progress, and generic failure settlement cannot
  mutate exact or quarantined radio aggregates. Quarantine fails and cleans a
  corrupt current aggregate while preserving already-terminal historical
  outcome semantics. Newly started `destroy_radio_tower` and
  `dynamic_stop_tower_rebuild` use contract `1`; historical terminal rows stay
  contract `0`, active legacy rows fail closed, and malformed current authority
  quarantines at `-59`. Generated ONLINE restore is accepted only with prior
  destruction and completed-rebuild provenance. The focused proof invokes the
  production admission, durable evidence, and outcome transitions through
  projection-only seams, rejects a direct second rebuild admission, and checks
  linked quarantine cleanup. Source and Workbench gates do not substitute for
  native authored binding, explosives, streaming, real
  restart, rendered UI, packaged networking, reconnect, or JIP evidence.
  The stamped Schema-59 source/Workbench checkpoint identifies implementation
  `37fb5f0ffbc80c4bba3151ba1e5d8be6ffcf8a21` with build label
  `schema59-radio-site-lifecycle`. The full Foundation gate passes. Final
  stamped-tree Workbench Game validation loaded 5,773 files/11,608 classes with
  CRC `96914c26` and `Script validation successful`; the bounded hidden normal
  WorldEditor open stayed alive and responsive for 10/10 samples over 20 seconds
  with no script-compile or crash signature. Its one Steamworks stats-request
  error was nonfatal. Packaged gameplay proof remains independently open.
- Schema 60 is the latest stamped source/Workbench checkpoint. `HST_BuildInfo`
  records implementation `fdf78493dd15915afe8d53f61a8ad1efd65b5635`, UTC
  `2026-07-11T23:24:55Z`, and label `schema60-exact-search-destroy`; Schema 59
  is the preceding checkpoint. The full Foundation gate passes, including 644
  symbol references. Final stamped Workbench Game validation loaded 5,777 files/
  11,615 classes with CRC `7aa80fc9` and `Game successfully created`. The
  correctly targeted hidden normal WorldEditor stayed alive/responding for
  10/10 samples over 20 seconds with no first-party error or crash signature,
  and `git diff --check` was clean apart from line-ending warnings. Only newly
  quoted and confirmed player
  Search-and-Destroy support enters its separate contract-1 operation type. One
  immutable infantry-only manifest, $350 charge, HR-per-frozen-member ledger,
  held SpawnQueue batch, support request, operation, and active group remain one
  replay-safe aggregate. The direct strategic route continues off-screen;
  near-player projection realizes only durable survivors; fold returns those
  survivors to virtual authority; and deterministic 30-second infantry combat
  updates both frozen friendly slots and the hostile abstract garrison. A group
  folded away from its immutable assignment first travels back virtually. It
  stays on station after the hostile garrison clears until commander recall,
  whose typed exit/settlement path refunds only eligible living HR. Before fold,
  physical recall exit, or campaign-stop retirement, exact player
  support exhaustively reconciles that projection's mapped casualties; any
  surviving projection must then prove one root and exactly one unique live
  adapter/PhysicalWar binding per durable living slot before its runtime root is
  retired. Persistence performs the corresponding exhaustive pass across all
  physical exact infantry, validates each exact-support reciprocal graph and
  live binding cardinality, and refreshes a still-living support group's
  physical position before capture. A failed proof defers the authority transfer
  or checkpoint. Held-batch cancellation snapshots the strategic living roster
  before cleanup so an immediate recall cannot refund a stale full roster.
  Historical Search-and-Destroy requests remain contract `0`; malformed
  current exact claimants quarantine at `-60` without legacy fallback or
  guessed balances. A quarantined group is globally non-operational and absent
  from combat-presence checks. Expired archive capacity can remove a valid
  tombstone/terminal-request pair only after replay, unique-identity, no-live-
  backlink, and full receipt-reciprocity checks; malformed/quarantined pairs are
  retained. The compiled/wired Campaign Debug proof covers valid pair prune plus
  restore and corrupt quarantine retention, but those assertions have not run.
  The same checkpoint removes the overlapping Maiden's Bay town while preserving
  the Logistics Warehouse and conservative frozen-ID save compatibility. A
  no-anchor save is untouched; ambiguous duplicate authorities fail closed;
  mutable generic references canonicalize; and settled, quarantined, malformed,
  or graphless exact typed authority remains frozen. Frozen generated sites and
  routes receive deep canonical clones while keeping their historical rows,
  ordinary lookup resolves the warehouse, and exact validators may use a
  detached old-ID/old-position view with runtime old/canonical equivalence. The
  deterministic in-memory migration proof is compiled and wired into Campaign
  Debug but has not run there, and packaged save/restart proof remains open. It
  also removes known one-second tick multipliers (pure-vehicle count
  oscillation, redundant/no-convoy scans, repeated authority resolution,
  unbounded unresolved-radio queries, and eager expensive debug evidence) and
  disables AI horn behavior at the native wheeled-vehicle base while retaining
  the scoped ambient-driver input clear. These results certify only the stamped
  source/Workbench boundary. No Schema-60 packaged server/client, actual save/
  restart, rendered UI, stutter/horn, or live gameplay behavior proof exists.
- The first exact force-runtime lifecycle slice: handed-off member slots retain
  durable ever-alive/casualty evidence, confirmed dead members detach from the
  native and Game Master group without deleting their corpses, the last death
  retires the exact root/marker, and restart reprojects only durable survivors.
- Request-driven spawn placement for physical support and debug probes, with
  road/dry-ground/vehicle-safe validation, player/active-AI clearance checks,
  and visible placement failure reasons
- Enemy support-spend ledgers for QRF/support damage pressure, same-zone
  cooldowns, max defense spend caps, survivor refunds, reportable denial
  reasons, and separated proactive attack versus reactive support spending.
  The exact enemy defensive QRF adds a deterministic full admission-failure or
  survivor-proportional post-commit settlement identity without changing other
  order refund policies.
- Civilian town support plus undercover eligibility, request/application,
  detection, police/roadblock scan, compromise, and clear-state enforcement
- Commander-facing no-admin actions for initial HQ selection, random mission
  start, objective/runtime inspection, FIA supply requests, support
  cancellation, civilian aid, support reports, garage/build reports,
  vehicle-cargo reports, loadout-editor status/application, generated content
  reports, persistence status, HQ threat reports, marker audits,
  balance/pacing reports, campaign-end reports, and undercover status/checks
- Phase 0/1 foundation and mission diagnostics are complete: the alpha command
  menu exposes foundation status, explicit checkpoint reporting, active mission
  inspection, selected mission inspection, and null-safe runtime mission fields
- Phase 2 convoy runtime diagnostics are complete: the Missions tab exposes a
  convoy runtime report with vehicle assets, active convoy groups, crew counts,
  travel distance, fallback mode, and explicit movement/spawn failure reasons
- Broad-alpha UI/report polish exposes marker status/detail/audit reports,
  command coverage smoke paths, clear failed-action text, HQ threat/Defend
  Petros reports, enemy order and physical-response reports, support
  ETA/status reports, balance/pacing diagnostics, and campaign-end summaries
- A packaged schema-49 check verified that the restored config metadata brings
  back normal stock HUD and Game Master access. That run also exposed separate
  marker and ambience defects: invalid radio icon entries rendered as giant
  boxes, zone markers lacked location names, the map pointer fell below a
  confirmation dialog, nearby civilian projection was coupled to hostile-zone
  activation, town actors repeated one appearance, traffic defaulted too low,
  drivers held the horn, and radio sites could receive a duplicate tower.
  Later source contains focused repairs, but they still require a
  republished runtime check.
- HQ proximity policy now keeps the 900m radius for hostile operation staging,
  while whole-location activation is excluded only inside that location's
  capture footprint (with a 150m legacy fallback) and composition clearance is
  limited to 150m. This source-only split prevents HQ safety from erasing nearby
  towns and bases while still keeping hostile spawns away from HQ.
- Admin-only campaign debug controls expose one-button smoke, physical, and
  full runtime verification profiles with status, cancel, cleanup, structured
  result artifacts, and bootstrap identity/admin evidence
- Persistent campaign win/loss state with schema-42 end reason, summary,
  elapsed time, strategic control, war level, FIA/enemy zone counts,
  population/support, airfield, outcome-mode, support deployment proof, and
  report generation metadata
- Versioned campaign save-data container that is migrated, tracked through
  `PersistenceSystem`, and flushed before native `SaveGameManager`
  checkpoint requests when saving is possible, with a profile JSON fallback at
  `$profile:h-istasi/HST_CampaignSaveData.json`. Before every real capture,
  persistence synchronously reconciles mapped physical exact-convoy, exact-
  enemy-patrol, exact-garrison-patrol, exact-mission-guard, and exact player-
  support members so a newly dead soldier cannot be serialized as alive between
  physical-war ticks. Physical exact player support additionally requires one
  reciprocal request/operation/batch/group graph, one root plus exactly one
  unique live binding and PhysicalWar member per durable survivor, and a fresh
  physical group position before capture. An open publication transaction,
  materializing exact-infantry handoff, missing/conflicting or aliased mapping,
  invalid live cardinality, unverifiable live position, or nonphysical operation
  retaining member mappings defers the checkpoint before any older tracked state
  is flushed or an engine savepoint is requested; the pending intent is retained
  and retried on the bounded debounce.

This is a broad-alpha campaign foundation, not a public alpha. Petros and the
HQ arsenal have live contextual-action prefabs for the alpha menu path, while
the arsenal, garage, and loadout editor remain custom h-istasi economy systems
instead of stock/MSAR arsenal behavior. The campaign loop is now connected far
enough to track HQ exposure, queue Defend Petros pressure, resolve support and
enemy orders physically or abstractly, enforce undercover state, publish marker
coverage, and persist won/lost campaign outcomes. The systems are still rough:
cache/tent polish, save/restart soak testing, final surveyed Everon
coordinates, richer AI waypoints, full loadout-editor HST_Dev smoke, garage
progression polish, balance tuning, and mission-specific interactable props
still need to be connected incrementally. The current exact adapter
supports exactly one infantry group root and its exact member slots; vehicle,
asset, and multi-root manifests fail closed as unsupported in the generic
SpawnQueue adapter. The schema-52 convoy consumer has a separate narrowly
owned three-vehicle/three-crew physical-war projection path. Player-paid QRF is
the first support type migrated to this path and, in schema 50, the first force
with persistent strategic movement, materialization hysteresis, exact virtual
roster authority, and bounded virtual combat. Campaign schema 51 adds the first
enemy consumer: newly planned infantry-only defensive QRF orders use one frozen
prepaid manifest, held strategic projection, live-position materialization,
return-to-origin duty, exact survivor-proportional settlement, and one operation
  marker. Existing enemy QRF rows remain contract version zero. Schema 53 adds a
  second exact enemy-order consumer: newly queued patrols use one frozen infantry
  root, a generated-route loop cursor, virtual/physical transfer, contact hold,
  return, and survivor-proportional proactive-attack settlement. Historical
  patrol rows remain contract version zero. Schema 54 adds a fifth exact
  operation consumer only for newly issued policy-v2 purchased resistance
  garrisons. Historical policy-v1 purchases, initial/enemy aggregate garrisons,
  counterattacks, roadblocks, support calls, and other support types remain on
  their prior state paths. Schema 55 adds the sixth exact operation type only
  for guard infantry created with a newly started `assassinate_officer` mission;
  its HVT remains separate. Schema 56 adds a second consumer of that same type
  only for guard infantry created with a newly started `assassinate_traitor`
  mission at contract `2`. Schema 57 adds a third consumer only for guard
  infantry created with a newly started `assassinate_specops` mission at
  contract `3`. Schema 58 adds the seventh explicit operation type for newly
  started `rescue_pows`; historical/pre-opt-in assassination and POW missions,
  `rescue_refugees`, and all other unsupported mission families remain contract
  `0`.
  Newly started convoy missions
  use the schema-52 exact contract; restored historical convoy rows remain
  version zero.
Policy-v2 garrison purchase manifests now carry one executable empty-root group
and arbitrary ordered member slots. They remain in strategic hold as an exact
local patrol and rematerialize only their durable survivors. Legacy garrison
activation still composes historical/initial/enemy aggregate counts rather than
consuming those old purchase records; vehicle and multi-root garrison execution
remain unsupported. Paid-QRF
restore clears process-local IDs and creates one held virtual batch with the
same confirmed casualty tombstones and durable survivor slots. Entering the
materialize-in radius then releases one new root plus only those survivors.
Exact enemy defensive-QRF restore uses the same held-survivor normalization,
clears process-local physical-arrival samples, and retains its immutable route
and prepaid resource-settlement authority. For the player QRF, initial deployment
failure still refunds money and HR; a technical reprojection failure retains the
already-delivered money cost and refunds surviving HR only.
Exact convoy restore discards process-local vehicle/group handles, retains the
route cursor, per-element vehicle state, cargo assignment, and confirmed crew
casualties, and resumes as one virtual operation until a player again enters the
materialization bubble. Current-schema restore accepts casualty tombstones only
on member slots, enumerates legal open duty and materialization/position pairs,
and retains missionless or partially unlinked exact-looking rows as quarantine
  evidence rather than deleting them through generic cleanup. General lifecycle
  authority beyond the two exact infantry-QRF consumers, the exact player
  Search-and-Destroy consumer, the exact enemy-patrol, the exact purchased-
  garrison patrol, the three exact assassination-mission guard families, the
  narrow exact mission-convoy consumer, and the exact `rescue_pows` guard-plus-
  captive consumer remains open. Normal spawn
acquisition runs once per active-campaign second. During setup or after a won/
lost outcome, the coordinator cancels every nonterminal batch and drains its
cleanup with a monotonic runtime-only clock without advancing campaign elapsed
time. A batch whose exact slots are all verified remains durably
`READY_FOR_HANDOFF` until physical-war finalization succeeds; only then does the
queue record `SUCCEEDED`. Restoring pending, ready, or previously physical state
clears process-local evidence and returns the operation to the single held
virtual projection instead of treating interrupted handoff as success or
respawning immediately. The focused HST_Dev proofs are implemented, but they are
not runtime evidence until a fresh isolated run executes them.

## Current Delivery Priorities

The implementation blueprint's Campaign Runtime Integrity sequence controls
current work. Feature breadth already exists; the immediate goal is to make its
authority, runtime projection, persistence, and client evidence trustworthy:

Schema 60 is the latest stamped source/Workbench development boundary, not a
runtime-proven checkpoint. It adds only the new exact player Search-and-Destroy
contract plus the conservative Maiden's Bay retirement and focused stutter/horn
repairs described above. The Maiden's Bay migration fixture is compiled and
wired into Campaign Debug, but the assertion has not run and no packaged save/
restart has exercised it. The Search-and-Destroy proof and typed-QRF archive-
mismatch assertion are likewise compiled/wired but unexecuted. Schema 59 is the
preceding stamped checkpoint and its exact radio-site lifecycle is unchanged.
Packaged schema-50
through schema-60 certification remains independently open; source or Workbench
validation does not certify native entities, actual restart, rendered UI,
networking, reconnect, or JIP.

1. Publish the exact stamped Schema-60 package for the next server check.
   Preserve its recorded implementation, UTC, label, and CRC so runtime evidence
   remains tied to this checkpoint; Schema 59 is the preceding boundary.
2. In that published check, cover the schema-50 marker,
   map-dialog, radio-site, and civilian repairs. Require valid-sized marker
   icons, `Location | Owner` labels, a
   visible pointer over the support confirmation dialog, one authored radio
   tower per existing site, and correct destroy-target binding. The already
   restored stock HUD and Game Master access must remain intact.
3. Prove civilian projection in a fresh packaged run: zero unregistered group-
   member RPCs, varied concrete appearances, five driven vehicles per true
   town, two pedestrians at the known minor woodland locality, no stuck horns,
   and actual distance-over-time movement. Figari and Morton must project
   civilians and eligible military garrisons near a player; Maiden's Bay must
   show only the Logistics Warehouse; and the separate 900 m hostile-operation
   staging clearance must not erase either location. Confirm that the observed
   once-per-second stutter and continuous AI horns are gone; source inspection
   alone does not close either runtime defect.
4. Execute the isolated `HST_Dev` completion/cancellation/restart boundary and
   replace the historical Full Campaign Debug artifact with corrected evidence.
5. Runtime-prove exact paid-QRF creation and the schema-50 operation path:
   strategic travel from the direct-route cursor, bounded catch-up, proximity
   materialization/fold hysteresis, exact casualty/survivor transfer, on-station
   virtual combat, recall/settlement, archive replay, and once-only accounting
   across save/restart.
6. Runtime-prove the schema-51 exact infantry-only enemy defensive QRF: one
   prepaid frozen roster, no duplicate legacy response, strategic outbound and
   return travel, materialization/fold hysteresis, exact casualties, two-sample
   physical arrival, once-only defensive pressure, proportional survivor
   settlement, marker movement/cleanup, and save/restart replay.
7. Runtime-prove the schema-52 exact mission convoy: one frozen generated road
   route, exactly three linked vehicles and three crew groups, cargo/captive
   assignment to vehicle zero, virtual travel without timer arrival,
   materialization with 3/3 seated living drivers, physical interception,
   contact-to-clear transition followed by casualty-preserving fold/
   rematerialization, partial crewless-vehicle reprojection/capture, non-suffix
   member casualties, cargo-only terminal-carrier recovery, detached player-bound
   cargo fold, duplicate-free garage handoff, two-sample physical arrival,
   once-only mission outcome/settlement, aggregate current/destination marker
   cleanup, and save/restart. Then prove the repaired
   support route-truth boundary through actual movement, two-sample arrival,
   recall exit, and bounded transactional waypoint reissue in scoped disposable
   profiles. In the same disposable boundary, reproduce the crewless mixed-QRF
   case and prove one terminal transition, zero capture/marker pressure, one
   neutral salvage detachment, no duplicate vehicle, and no survivor
   resurrection after restart.
8. Runtime-prove the Schema-60 exact infantry-only player Search-and-Destroy
   operation: immutable quote/manifest/ledger identity, $350 plus exact roster
   HR, no vehicle/asset/multi-root substitution, direct-route travel, virtual-
   to-physical-to-virtual survivor continuity, mapped casualties, off-screen
   combat, virtual return to assignment after a displaced fold, commander
   recall, living-HR settlement, archive replay, contract-0 legacy isolation,
   `-60` quarantine, safe paired archive-capacity eviction, corrupt-pair
   retention, and real save/restart. Require live adapter-observed
   casualties, cardinality-checked root retirement during fold, and the physical
   recall-exit path; the deterministic queue casualty/fold fixture is source
   bookkeeping evidence only. Clearing hostile infantry must leave the group on
   station until recalled.
9. Prove static marker widget readiness plus authoritative host/client/late-join
   projection before treating marker publication as player-visible truth.
10. Runtime-prove the schema-53 exact newly queued enemy-patrol slice: one frozen
   infantry roster, once-only proactive attack debit, outbound virtual travel,
   materialization/fold with mapped casualties, physical contact hold, one
   closed patrol lap, return, survivor-proportional refund, marker cleanup, and
   save/restart. Historical patrols must remain contract version `0`.
11. Runtime-prove the schema-54 exact purchased-garrison patrol: a newly issued
   policy-v2 resistance purchase must hold one exact arbitrary member roster,
   loop locally while virtual, materialize its empty root plus living slots,
   retain mapped casualties across fold/re-entry, publish and clean up its
   marker, and settle without a refund on owner change, all-dead, or campaign
   stop. Save/restart and late join must preserve the same roster and cursor.
   Historical policy-v1, initial-map, enemy aggregate, vehicle, and multi-root
   garrisons must remain isolated on their legacy paths.
12. Runtime-prove the schema-55 exact officer-assassination guard: only a newly
    started `assassinate_officer` mission may admit one route-less empty-root/
    member roster. Prove native materialization, mapped casualties, fold/re-entry,
    all-guards-dead independence from the HVT objective, every typed zero-refund
    terminal outcome, `-55` quarantine without fallback, compact settlement,
    existing-HVT marker/UI status, save/restart, owner change, campaign setup,
    reconnect, and JIP. Historical officer missions, other assassination
    variants, and every other mission family must remain contract `0`.
13. Runtime-prove the schema-56 exact traitor-assassination guard: only a newly
    started `assassinate_traitor` mission may use contract `2` and
    `exact_assassinate_traitor_guard_v1`. Prove the same route-less exact roster,
    HVT separation, survivor projection/fold, typed zero-refund outcomes, compact
    restore, existing-HVT status, and `-56` quarantine boundary without changing
    the Schema-55 officer contract. Cover native entities/adapter casualties,
    real save/restart, rendered UI, owner-change, setup, packaged networking,
    reconnect, and JIP. Pre-56 traitor missions remain contract `0`, and the
    separately opted Schema-57 spec-ops path must not alter traitor contract `2`.
14. Runtime-prove the schema-57 exact spec-ops-assassination guard: only a newly
    started `assassinate_specops` mission may use contract `3`, policy
    `exact_assassinate_specops_guard_v1`, intent `assassinate_specops_guard`, and
    `-57` quarantine. Prove the same route-less exact roster, HVT separation,
    survivor materialize/fold/re-entry, typed zero-refund outcomes, compact
    restore, existing-HVT status, and no virtual combat while preserving officer
    contract `1` and traitor contract `2`. Historical/pre-57 spec-ops, ordinary
    `mission_group_*` rows, rescue missions, and unsupported families must remain
    contract `0`. Cover every packaged/native/restart/UI/network gate still open.
15. Runtime-prove the schema-58 exact POW rescue: only newly started
    `rescue_pows` may use mission-rescue contract `1`, policy
    `exact_rescue_pows_v1`, intent `rescue_pows_guard`, and `-58` quarantine.
    Prove one exact guard roster plus three captive identities, rendered action
    legality, natural boarding/seat evidence, fold/re-entry, casualty failure,
    three-receipt HQ success, custody-only grace, actual save/restart, owner
    change, campaign setup, packaged networking, reconnect, and JIP. Historical
    POWs, `rescue_refugees`, and unsupported rescue families must remain contract
    `0`.

The schema-50 player exact-QRF projection slice, schema-51 enemy defensive-QRF
slice, schema-52 exact mission-convoy slice, schema-53 exact enemy-patrol slice,
schema-54 exact purchased-garrison patrol slice, and schema-55 exact officer-
mission guard slice define six explicit operation types. Schema 56 adds the
traitor guard as a seventh explicit family consumer of the mission-guard type,
using its separate contract version and policy. Schema 57 adds the spec-ops
guard as an eighth explicit family consumer without adding an operation type.
Schema 58 adds mission rescue as a seventh explicit operation type and a ninth
exact family consumer, limited to newly started `rescue_pows`.
Schema 60 adds player Search-and-Destroy as an eighth explicit operation type
and a tenth exact family consumer. The infantry-QRF and Search-and-Destroy
virtual routes use a conservative direct
campaign cursor, materialization uses separate in/out distances, and the frozen
manifest slots remain the living/dead roster across fold and restore. Player
virtual combat is deliberately limited to on-station exact infantry QRF and
Search-and-Destroy operations against abstract target-garrison infantry. Search-
and-Destroy remains on station after hostile clearance until commander recall;
the enemy defensive policy instead applies
bounded capture-pressure relief once, returns to origin, and settles surviving
resources. The convoy instead advances a persisted generated road-route cursor
at convoy speed, owns three durable vehicle/crew elements, projects physically
only near players, and settles through existing convoy mission outcomes. The
patrol uses its generated route as a persisted outbound/closed-loop/return
cursor, holds route progress during physical contact, and settles the surviving
  proactive-attack fraction on return. The purchased-garrison patrol instead
  remains assigned to its zone, loops locally without a terminal lap count, and
  keeps its paid roster as the sole virtual/physical survivor authority with no
  refund path. All three exact assassination guards remain route-less and on station;
  each held exact roster virtualizes only observed survivor state, while its
  separate HVT keeps mission outcome authority. The convoy does not yet simulate off-screen
combat; virtual convoy casualties change only
from previously observed physical state. This does not complete the broader
operations milestone: generalized live-contact authority, terrain and
  ammunition effects, broad legacy supports, historical/initial/enemy aggregate
  garrisons, garrison vehicles and multi-root forces, other mission forces,
  other enemy orders, historical patrols, and client/JIP projection remain
  future slices. The assassination-guard family is exhausted after Schema 57;
  Schema 58 implements the first rescue vertical only. Schema 59 separately
  implements the exact radio-site lifecycle. Schema 60 adds only the new player
  Search-and-Destroy force consumer; further rescue and unsupported mission/
  support families still require their own explicit cutovers.

The last published schema-49 server/client check is runtime evidence that stock
HUD, Game Master, map markers, and civilian traffic initialize again. It also
records the marker, cursor, radio, classification, appearance, horn, and
military/civilian activation defects, not proof of later source repairs. A newer
user server check additionally observed a pronounced once-per-second stutter,
continuous AI horns, and overlapping Maiden's Bay town/warehouse markers. Those
observations are the runtime baseline for the Schema-60 performance, horn, and
location fixes. Schema 60 is the latest stamped source/Workbench checkpoint at
implementation `fdf78493dd15915afe8d53f61a8ad1efd65b5635`, UTC
`2026-07-11T23:24:55Z`, build label `schema60-exact-search-destroy`, and CRC
`7aa80fc9`; Schema 59 is the preceding checkpoint. A republished server/client
test is still required
before those repairs, either exact-QRF projection path, or any source-validated
schema-52 convoy/schema-53 enemy-patrol/schema-54 purchased-garrison-patrol/
  schema-55 officer-guard/schema-56 traitor-guard/schema-57 spec-ops-guard/
  schema-58 exact POW-rescue/Schema-59 radio-site/Schema-60 exact Search-and-
  Destroy paths
is called runtime-proven. The late-admin
role-change guard, campaign-debug
isolation, convoy seating, physical support movement/arrival/recall, and restart
boundaries remain open as separate gates.

For documentation authority, `docs/FEATURE_CHECKLIST.md` owns current feature
status and next tasks; `docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md` owns the
latest runtime evidence; `docs/MIGRATIONS.md` owns save-schema history; and the
top CRI sections of `docs/PHASE_PLAN.md` own delivery order. The numbered phase
details remain acceptance/history material and do not override those current
sections.

## Alpha Command Menu

Press `I` in `HST_Dev` or `HST_Everon` to open the h-istasi alpha command
menu. The menu is a client widget driven by server-built snapshots and renders
a campaign HQ interface with a resource bar, navigation, campaign
cards, action list, and activity/result feed. The Setup tab displays the
effective server config and lets the commander choose the first HQ hideout
before the campaign enters the active phase. `$profile:h-istasi/HST_Settings.json`
remains the source of truth for defaults that apply to newly created
campaigns, but initial HQ placement is selected through the setup map flow and
the generated runtime settings no longer expose a default hideout id. The file
uses JSON-safe `_comment` fields to explain settings because raw JSON comments
are not supported. The same settings file also controls h-istasi's Game Master budget
policy through `features.gameMasterBudgetsEnabled`, which defaults to `false`
so GM placement budgets are disabled automatically on game start.
`features.infiniteStaminaEnabled` defaults to `true`; when enabled, local
player stamina is refilled through the native stamina component and the sprint
exhaustion vignette/blur effect is suppressed.
`features.trackResistanceSupportGroupsOnMap` defaults to `true`; when enabled,
spawned player-requested resistance support groups keep a live map marker until
the group is killed, folded, or despawned.

During initial HQ placement the setup map owns keyboard input; complete HQ
placement first, then `I` opens the command menu and server snapshots add the
Admin tab for configured SteamID64 admins.

- `MenuUp` / `MenuDown`: change selection
- `MenuSelect`: run the selected command
- `MenuBack` or `I`: close the menu

The menu routes through server-authoritative coordinator requests and covers
campaign overview, markers and marker audits, economy, balance/pacing,
campaign-end state, zones, missions, mission runtime, manual checkpoint,
persistence status, income tick, training, first HQ selection, HQ movement, HQ
threat, Defend Petros, FIA recruitment, mission start, random mission
selection, objective progress, support requests/cancellation and ETA/status,
enemy order/physical-response reports, civilian aid, undercover
eligibility/request/check/clear flows, zone capture/activation, arsenal
reporting, vehicle cargo, garage capture/redeploy, nearby loot collection,
loadout editor status/application, generated content reports, HQ asset rebuilds,
command coverage and failed-action smoke reports, roster admin helpers,
campaign reset, one-button campaign debug verification, and small debug
resource awards. Commander support and garrison call-ins use the normal in-game
map for target selection and are disabled when the commander has no map gadget.
Garrison and player-QRF targeting issue exact server quotes; the Forces tab then
shows the frozen count/cost and confirms or cancels only the actor-owned quote
ID through a confirmation dialog.
Multiplayer clients use a player-owned request/RPC component;
the server resolves the caller from ownership instead of trusting a client
provided player ID. Petros opens this same menu path through contextual
interactions, and the visible HQ arsenal is backed by the same server commands
from the Arsenal/Loot tab.

## Admin and Runtime Debug

To grant h-istasi admin rights on a local or dedicated server, add the player's
raw 17-digit SteamID64 to `membership.adminIdentityIds` in
`$profile:h-istasi/HST_Settings.json`. Backend UUIDs, BattleEye GUIDs,
`workbench_player_N` aliases, and per-session player IDs are not durable
h-istasi admin tokens.

The Admin tab exposes `Run Full Campaign Debug` as a one-button diagnostic
harness. It is not runtime certification by itself. In-process debug profiles
are restricted to `HST_Dev`, run against a cloned campaign state, suspend real
campaign checkpoints, and restore the untouched live state on completion or
cancel. A development-session restart is still required because world entities,
player state, and service caches are outside campaign save data. External,
restart, and soak profiles require a separately managed disposable profile and
launcher instead of starting through the in-process runner. Results are written under
`$profile:h-istasi/debug` as structured JSON, summary text, and state-diff text.
A valid admin grant should be visible in logs as a
`settings SteamID64` runtime grant, and command-menu input diagnostics log the
local `I` key/action path when troubleshooting menu access.

For dedicated server tests, repack/publish the Workbench addon before launching
the dedicated server. Server boot, admin diagnostics, command-menu readiness,
and structured debug artifacts must report the same runtime identity from
`HST_BuildInfo`: full commit SHA, UTC build time, label, campaign schema 52, and
runtime-settings schema. Missing or mismatched identity means the packaged
server/client runtime is stale or mixed, even if the repository is newer.

## Design Rules

- Prefix addon-owned scripts and resources with `HST_`.
- Keep the strategic campaign server-authoritative.
- Use native Reforger persistence and session saves.
- Keep off-screen forces abstract and activate physical AI only around
  players and active objectives.
- Track feature-complete campaign work in `docs/FEATURE_CHECKLIST.md` and keep
  planning docs focused on h-istasi behavior, status, gaps, and acceptance
  tests.
- Do not copy source assets from neighboring addons.
- Do not add hidden dependencies.

## Development Scenarios

- `Missions/HST_Everon.conf`: full-island campaign shell
- `Missions/HST_Dev.conf`: compact systems-development shell

See `docs/ARCHITECTURE.md`, `docs/FEATURE_CHECKLIST.md`,
`docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md`, `docs/PARITY.md`, and
`docs/PHASE_PLAN.md` for the implementation map, feature checklist, runtime
verification audit, and current phase roadmap.
