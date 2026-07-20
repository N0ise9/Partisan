# Partisan Decision Log

This log records architectural decisions that constrain implementation order.
Entries are append-only; superseded decisions should point to the replacing
entry instead of being silently rewritten.

## CRI-001 - Establish Campaign Authority Before Feature Expansion

- Status: Accepted
- Date: 2026-07-09

Context: The broad-alpha runtime contains many useful domain services, but their
presence does not prove a uniform authority, replay, transaction, or event
contract. Extending those paths first would multiply direct mutations and make
save/load failures harder to reconcile.

Decision: Establish the schema-42 campaign-authority foundation first. Persist a
monotonic ID sequence, bounded command receipts, resource transactions, and a
bounded campaign event log. Route consumers onto typed, idempotent commands and
the ledger incrementally, beginning with troop training.

Consequences:

- New feature breadth does not outrank closing authority dependencies.
- A migrated command must produce stable IDs, one durable receipt, explicit
  transaction outcomes, and bounded diagnostic events.
- Existing domain paths remain partial until explicitly migrated and certified;
  schema 42 alone is not full runtime certification.

## CRI-002 - Quote An Exact Manifest Before Paid Force Creation

- Status: Accepted
- Date: 2026-07-09

Context: Charging from requested aggregate counts while later selection or
spawning silently substitutes, omits, or partially creates units breaks resource
integrity and makes refunds ambiguous.

Decision: A paid force operation must first resolve an immutable quote and exact
manifest from authoritative catalogs. The manifest carries stable force and unit
IDs and the complete resource cost. Only after the full quote is valid and
affordable may the ledger reserve that exact cost. Creation is all-or-nothing:
every manifest entry is accepted into authoritative force state, or the
reservation is cancelled and no force is created.

Consequences:

- Support, recruitment, and garrison cost paths must not be declared migrated
  until the quote/manifest contract exists for their force type.
- Physical realization may be deferred or virtualized, but it may not change
  the purchased manifest or silently alter its price.
- Spawn failure affects projection state, not ownership of an already committed
  manifest, unless the operation contract explicitly cancels before commit.

## CRI-003 - Isolate Certification Runs From Campaign Saves

- Status: Accepted
- Date: 2026-07-09

Context: Full campaign diagnostics deliberately exercise destructive branches,
including terminal outcomes and resource depletion. Running them against a live
campaign can contaminate persisted state and invalidate later evidence.

Decision: Destructive runtime and certification profiles must execute in an
isolated disposable campaign or behind a proven snapshot-and-restore boundary.
Static validation and Workbench compilation may run against the development
workspace, but they do not substitute for isolated runtime certification.

Consequences:

- Do not run a destructive certification suite against a campaign save that a
  player or later test intends to keep.
- In-process profiles fail closed outside the development world. The live state
  is persisted and retained untouched while a deep clone owns the run; complete
  and cancel both restore the original reference and persistence tracking.
- External, restart, and soak workflows require a separately managed disposable
  profile and launcher instead of entering the shared in-process bootstrap.
- Certification evidence must identify its build, scenario, isolation method,
  save/migration boundary, and result.
- If isolation or restoration cannot be proven, the runtime certification gate
  remains open rather than risking persistent state.
- Campaign-state swapping does not restore world/player/service runtime state,
  so the development session must still be restarted after a run.

## CRI-004 - Make Player Garrison Recruitment All-Or-Nothing

- Status: Accepted
- Date: 2026-07-09
- Settlement-retention consequence superseded by CRI-005 on 2026-07-10.

Context: The quantity chooser previously charged one flat HR after silently
clamping the requested quantity to remaining capacity. That made the displayed
quantity, accepted roster, and charged basis disagree.

Decision: Player-visible garrison recruitment uses an expiring server quote and
an all-or-nothing capacity policy. Each accepted member costs $50 and one HR.
The first action only creates the immutable quote/manifest; a later action
confirms only the quote ID. No partial acceptance is allowed. Existing protected
internal/debug aggregate helpers retain caller-supplied total costs for
compatibility but are not the player purchase authority.

Consequences:

- A request for more members than remaining capacity fails without a manifest,
  debit, or garrison change.
- The exact UI confirmation row comes from the persisted quote, not the original
  quantity button.
- Confirmation creates two linked ledger rows, registers exactly the manifest
  member-count increment, verifies one acceptance-provenance link, and then
  commits. This is purchase-time strategic authority, not yet living-slot or
  physicalization authority.
- Equipment pricing/consumption is intentionally zero in this first policy and
  remains an explicit later extension rather than an implied hidden charge.
- Concurrent open quotes are bounded. Accepted settlement history remains
  durable and un-compacted until a replay-safe archive/tombstone policy exists.

## CRI-005 - Bound Accepted Force-Settlement History Without Losing Replay

- Status: Accepted
- Date: 2026-07-10

Context: Exact garrison and paid-QRF quotes, manifests, and ledger rows provide
replay safety, but retaining every settled aggregate in full eventually makes
planning history unbounded. Deleting those rows without a replacement would
reopen duplicate issue, confirmation, debit, refund, and conflict handling after
restart.

Decision: Retain accepted rows in full for at least 600 campaign seconds and
while any active-group, enemy-order, or force-spawn backlink remains. A paid QRF
also requires one unique terminal support settlement. Only a provably terminal,
unambiguous aggregate may compact atomically into a persisted replay tombstone.
Tombstones retain issue/confirmation/actor/
operation/manifest/hash/cost/transaction/settlement identity for at least
86,400 campaign seconds, cap at 256 rows, and share a hard 320-row planning
history bound with full quotes. Admission fails closed when protected history
cannot safely make room.

Consequences:

- Issue, confirmation, transaction commit, and conflict replay consult the
  compact archive before any mutation or debit.
- Live or ambiguous backlinks preserve full history; compaction never infers a
  terminal result from missing runtime handles.
- Production queue compaction must receive explicit pins so terminal queue rows
  do not permanently block planning or disappear while still referenced.
- Pre-schema-48 migration retains accepted history in full and creates no
  invented settlement tombstones.
- Runtime archive/restart/capacity evidence remains required before this policy
  is certified beyond static and Workbench validation.

## CRI-006 - Exactify New Player Search-and-Destroy Without Rewriting History

- Status: Accepted
- Date: 2026-07-11

Context: Player Search-and-Destroy is a moving paid infantry support family that
must continue its mission while outside the player render bubble. Letting the
legacy request infer casualties, respawn a full group, or complete from a timer
would violate the exact force and operation boundaries already established for
paid QRFs.

Decision: Schema 60 opts in only newly quoted and confirmed Search-and-Destroy
requests. One infantry-only catalog manifest, $350 plus HR-per-member ledger,
support request, separately typed operation, held SpawnQueue batch, and active
group remain one reciprocal aggregate. Physical projection is disposable;
frozen slots and the operation cursor are durable. A displaced fold returns to
the immutable assignment virtually, hostile clearance leaves the force on
station, and commander recall owns exit plus eligible living-HR settlement.

Consequences:

- Pre-60 Search-and-Destroy stays contract `0`; migration does not auto-upgrade
  or infer an exact roster, transaction, casualty, or settlement.
- Malformed current exact claimants quarantine at `-60` and cannot fall through
  to legacy execution or receive a guessed balance correction. Quarantined
  groups are globally non-operational and do not satisfy combat-presence checks.
- Vehicles, assets, empty rosters, and multi-root substitution are outside this
  cutover and fail closed.
- Fold, physical recall exit, and campaign-stop retirement first exhaustively
  reconcile the affected projection. A nonzero roster must prove one root plus
  exactly one unique live adapter and PhysicalWar binding per durable survivor;
  failure preserves physical authority instead of guessing a casualty or
  retiring the root.
- Persistence runs the exhaustive exact-infantry reconciliation before capture,
  validates each exact-support reciprocal graph and live binding cardinality,
  and refreshes its physical position. Held-batch cancellation snapshots the
  strategic living roster before cleanup so immediate recall settlement cannot
  use stale pre-casualty strength.
- Expired exact-support archive capacity applies full terminal-receipt
  reciprocity to every positive typed player-support contract, including exact
  QRF, before removing its paired tombstone/request. Replay validity, aggregate
  identity uniqueness, and absence of live backlinks must also pass. Historical
  contract-0 QRF retains minimal compatibility; malformed or quarantined typed
  pairs remain durable evidence.
- Schema 60 is the sealed checkpoint for this historical decision under label
  `schema60-exact-search-destroy`, implementation
  `fdf78493dd15915afe8d53f61a8ad1efd65b5635`, and UTC
  `2026-07-11T23:24:55Z`. Full Foundation passes with 644 symbol references and
  final Workbench Game validation records CRC `7aa80fc9` at 5,777 files/11,615
  classes; Schema 59 is the preceding checkpoint. The stamp does not certify
  packaged server/client, actual restart, rendered UI, stutter/horn, or live
  behavior.
- The compiled/wired Campaign Debug proof also covers valid paired capacity
  prune/restore and corrupt quarantine retention, but none of its Schema-60
  assertions has run. The casualty/fold/immediate-recall case mutates synthetic
  queue slots; it is not live adapter retirement or physical recall-exit
  evidence. Those remain packaged-runtime gates alongside movement/combat, real
  restart, rendered UI, reconnect, and JIP.

## CRI-007 - Retire Duplicate Strategic Locations Without Rewriting Frozen IDs

- Status: Accepted
- Date: 2026-07-11

Context: Maiden's Bay had two strategic rows a few meters apart: a town and the
Logistics Warehouse. Keeping both creates duplicate markers, civilians,
garrisons, income, and target candidates; blindly renaming historical rows would
instead corrupt frozen operation evidence.

Decision: The Logistics Warehouse is the sole enumerable strategic location.
Fresh defaults and world projection remove the town. Restore cleanup preserves
the warehouse's existing owner/economy, retires duplicate town-only state, and
uses a detached compatibility lookup for frozen old-ID references rather than
reinserting the retired town into the zone registry.

Consequences:

- No anchor means no mutation. Duplicate canonical authorities, or multiple
  legacy authorities without one canonical row, fail closed before rewrites and
  record only an idempotent conflict audit.
- No new resources, owner change, aggregate force transfer, mission outcome, or
  receipt is invented during cleanup. Duplicate ambient/garrison/police
  projections retire without fold-back or manpower credit.
- A save containing only the legacy town converts that row in place; a save
  containing both keeps the warehouse authoritative.
- Mutable generic references canonicalize. Nonzero typed authority remains
  frozen even when settled, quarantined, malformed, or recognizable only from
  an exact group mode/status; generic cleanup must not reinterpret it.
- Exact historical references may resolve a detached old-ID/old-position view
  until settlement, while ordinary mutable lookup resolves the canonical row.
  Runtime admission and duplicate checks treat those IDs as equivalent, but the
  detached view cannot create income, civilians, markers, or new orders.
- Mutable generated sites/routes rekey to the canonical namespace. Frozen
  typed site/route rows stay unchanged and receive deep canonical clones so new
  generation does not depend on rewriting historical identity.
- The deterministic migration fixture is compiled and wired into Campaign
  Debug, but has not been executed there. A source fixture cannot replace a
  packaged pre-update save/restart test.
- A packaged pre-update-save test must prove the duplicate does not return.

## CRI-008 - Project Campaign Markers From One Revisioned Client View

- Status: Accepted
- Date: 2026-07-11

Context: Server marker-state rebuilds and native marker publication did not give
each client a durable logical registry, global ordering, deletion history, or a
late-join/reconnect convergence contract. Publication counters therefore could
not prove that the host and every client held the same marker view, and sending
both server-native and client-local campaign markers would create duplicate
presentation owners.

Decision: Schema 61 introduces a marker-only authoritative projection stream.
Every logical marker keeps a stable ID, per-record revision, last global stream
sequence, and tombstone. The server publishes bounded, hashed snapshot chunks
or contiguous deltas from one monotonic epoch/sequence, accepts only
ownership-derived readiness and acknowledgements, and falls back to a fresh
snapshot whenever retained history cannot prove continuity. Each client keeps a
widget-independent registry, commits a snapshot only after all chunks pass
count/hash validation, applies deltas only in order, and reconciles native
campaign markers locally from that registry.

Consequences:

- Server-native campaign marker publication is retired. Dynamic player markers
  remain on their existing replicated-entity path and are explicitly outside
  this marker-only protocol.
- Duplicate or already-applied deltas are idempotent; a forward gap, invalid
  revision, epoch mismatch, malformed packet, or hash mismatch cannot partially
  mutate the client registry and instead requires resync.
- Each owner has at most one unacknowledged delta batch. Only its final
  hash-bearing packet is acknowledged and allowed to trigger native
  reconciliation; post-ACK mutations are dispatched immediately. A bounded
  readiness heartbeat confirms lost ACKs or restarts a genuinely incomplete
  in-flight stream without replacing a fresh delta.
- Encoder limits are authoritative too: an oversize row, snapshot, or delta is
  rejected before the session enters an impossible ACK wait.
- First join, late join, reconnect, an unavailable journal range, and explicit
  client recovery all converge through a complete snapshot. Connected-client
  ACK watermarks constrain bounded journal pruning.
- Authored static marker binding uses exact cached entity-name lookup with retry
  only for unresolved bindings and actual-faction verification. A periodic
  radius search is not a legitimate identity or performance boundary.
- The client hides an authored zone descriptor only after its replacement
  custom zone marker is confirmed live, caches that binding, and restores the
  descriptor's prior visibility on replacement failure/removal. The authored
  entity and its mechanics remain intact while duplicate stock/custom icons do
  not.
- Migration may discard and rebuild derived marker rows, but it may not infer a
  zone, owner, mission, force, or outcome. Malformed current projection state
  advances the epoch before rebuild so an old client stream cannot be reused.
- Deterministic stream fixtures and Workbench compilation are source evidence,
  not multiplayer certification. The gate remains open until a packaged host,
  two clients, reconnect, late join, map open/close, native widget rendering,
  and real save/restart all converge on the same epoch, watermark, and registry
  hash.
- The completed source/Workbench checkpoint is implementation
  `27672e67ce4285810f313130293df1ac917c9bdf`, UTC `2026-07-12T01:02:39Z`, label
  `schema61-authoritative-marker-projection`, Foundation 655, and Workbench CRC
  `df41a779` at 5,782 files/11,631 classes; normal hidden WorldEditor remained
  responsive 10/10 over 20 seconds. This does not change the packaged-runtime
  gate.
- This decision does not canonicalize zone ownership mutation. The next source
  slice remains one idempotent ownership-transition service whose complete side
  effects then feed the marker projection as derived output.

## CRI-009 - Separate Combat Presence From Render State And Empty Assets

- Status: Accepted
- Date: 2026-07-12

Context: Capture, mission contact, HQ threat, civilian safety, and enemy strategy
used related but non-identical active-group tests. A durable survivor-vehicle
count could make an uncrewed vehicle look hostile, while render activation could
stand in for strategic pressure. Repeating full group scans in every consumer
and for every zone would also recreate a visible one-second hot-path multiplier.

Decision: Schema 63 introduces one state-only combat-presence service. Native
physical inspection publishes a short-lived registered-group sample; all domain
consumers query the shared service. A conscious dismounted character contributes
as infantry. An operational occupied armed mobile platform or occupied static
weapon contributes once per platform. Cargo, empty vehicles, destroyed/burning
platforms, immobile mobile platforms, stale samples, and non-operational rows do
not contribute. Eligible virtual projections contribute durable living infantry
but never an abstract vehicle count. Zone heat persists separately as revisioned
`HOT`, `COOLING`, and `COLD` state with a configurable 30-second default cooling
deadline. Render projection independently enters at the activation radius and
exits at the larger deactivation radius.

Consequences:

- Capture, missions, HQ, civilians, enemy commander scoring/defense, and the
  legacy combat-present wrapper must not reintroduce local survivor/vehicle or
  zone-active heuristics. Legacy QRF dispatch requires resistance capture
  progress or a valid live canonical hostile result; invalid authority cannot
  become verified damage, target score, or high-impact support pressure.
- One injected service instance caches eligible contributions per campaign state
  and elapsed second. Same-tick mission, support, order, sampling, and physical-
  projection mutation boundaries explicitly invalidate the cache before later
  consumers rely on it.
- The native sampler keeps an indexed runtime registry and reusable scratch
  arrays; the state-only service caches contribution, authority-gap, and
  zone/radius results. An unresolved spawned-group authority gap invalidates the
  affected result. Legacy QRF arrival resolution treats that authority as
  tri-state: unresolved evidence defers resolution, while only authoritative
  zero pressure or missing/terminal durable group authority may fail the
  response. Capture also requires a living conscious character and does not
  count spectator or Game Master proxies as friendly presence.
- Stable heat and freshness-only sample refreshes do not mark persistence dirty.
  The first clear tick begins cooling once; repeated clear ticks do not extend
  its deadline. MissionRuntime retains a persisted `HOT` guard for the one
  boundary tick before the heat service observes the clear result and begins
  cooling.
- Restore always invalidates native physical samples. Pre-63 and malformed
  current heat become canonical `COLD` without inferring pressure from vehicles,
  markers, render state, or generic group rows; valid current cooling may keep
  its original deadline only when its duration is `1..300`, last-hot is not from
  the future, and the deadline is still strictly in the future.
- Runtime-settings Schema 23 owns
  `capture.combatPresenceCoolingSeconds`, default 30 and normalized to `1..300`.
- Authored zone activation overrides retain the global exit-minus-entry margin,
  with at least 100 metres of physicalization hysteresis. Cold zones with no
  potential hostile contributor skip full result allocation; capture defers
  hostile queries until resistance presence exists; and an unchanged runtime
  topology signature skips the redundant second native sampler/index
  rebuild while still detecting equal-count handle replacement and native group
  membership-count changes.
- Deterministic state fixtures are implementation evidence only. Native seat,
  damage, movement, cache ordering, real serialization/restart, packaged
  runtime, and multiplayer behavior remain open until directly tested.
  The sealed Schema-63 checkpoint identifies implementation
  `85a75c65e9c148a890d8d78b0288ae6483a5ccd9`, UTC
  `2026-07-12T08:22:05Z`, and label `schema63-canonical-combat-presence`.
  Foundation passes at 681 script-symbol references, and a normal Workbench
  Script Editor open compiled/created 5,788 files/11,670 classes with CRC
  `a40056c5`, no HST script errors, and no crash. Explicit validation also passes
  for WORKBENCH, PC, XBOX, PS4, and PS5 with exit code `0`.

## CRI-010 - Canonicalize Town Influence Before Expanding Political Gameplay

- Status: Accepted
- Date: 2026-07-12

Context: Town political truth was split between signed zone support and a
civilian FIA/occupier pair, while population and modifier aggregates lived on
the civilian row. Several production callers could mutate those fields through
different wrappers, and the per-second civilian tick folded every town across
all influence events. That made faction attribution, migration, flip ownership,
and the reported one-second stutter difficult to reason about. The Map/War tab
also displayed an arbitrary zone subset rather than contacted political truth.

Decision: Schema 64 introduces exactly one revisioned
`HST_TownInfluenceRecord` for each unique curated town. It is the sole owner of
separate FIA/occupier/invader basis-point support, initial/remaining/destroyed
population, contact/activity, event aggregates, and pending political intent.
Legacy zone/civilian support and population fields become projections only.
Typed influence events use the population formula pinned to commit
`6e4226d3863ca8673535386c2fff8b6e08a806c4`, and political ownership uses strict
hysteresis: resistance above `8000` basis points, enemy below `4000`, with
equality neutral. Every flip is submitted to `HST_OwnershipTransitionService`.

Consequences:

- Golden scaling is contractual: raw `+1` at initial populations 100, 25, and
  400 yields `+100`, `+200`, and `+50` basis points. Occupier and invader remain
  distinct even when a legacy signed projection uses the stronger enemy.
- Exact event identity is idempotent and preserves requested/effective deltas,
  population basis, before/after support, before/after population triples, and
  record revisions. Authorized absolute debug seeds set all three support
  targets and the population triple through the same event/replay path without
  rounding or owner-based enemy redirection. Current restore requires a
  continuous population chain and exact final-record match even when unrelated
  revisions fall between events.
  Political callers, mission outcomes, convoy/order effects, radio drift, and
  security pressure cannot write legacy support as truth or publish an owner
  directly.
- Contact requires explicit player, mission, incident, or resistance-activity
  evidence. Global radio/security drift alone does not reveal a town. Zone
  Pressure contains contacted valid towns only, current first and then stable
  FIA-support/name/ID order. Resistance Territory contains the complete
  published resistance-owned non-mission set in deterministic type/name/ID
  order and shares the marker projection's completed-parent publication fence.
- Pre-64 migration gives the legacy signed margin deterministic precedence on
  disagreement, adjusts the civilian pair minimally, records one bounded
  conflict fact, preserves legitimate destroyed population, and never replays
  legacy events. Malformed current authority quarantines at `-64` rather than
  being guessed. Simon's Wood remains ambient-only; the Maiden's Bay Logistics
  Warehouse remains nonpolitical.
- Influence aggregates update at mutation time. Event history is scanned only
  when a canonical record's next expiry is due. Verbose changed active-group
  survivor/count logs are throttled to 30 seconds. Both are source mitigations,
  not evidence that the one-second stutter is fixed.
- The preceding Schema 64/settings-23 checkpoint is sealed at implementation
  `6f3c913eaed66926cce38b2ecafcff94084898a3`, UTC
  `2026-07-12T11:28:41Z`, and label `schema64-canonical-town-influence`.
  Foundation passes at 696 references, and normal/all-five-
  configuration Workbench validation passes at 5,793 files/11,695 classes with
  CRC `36d5b017`, zero HST script errors, and zero surviving Workbench
  processes. Campaign Debug, save/restart, packaged runtime, rendered UI,
  performance, and multiplayer gates remain open.

## CRI-011 - Lease And Admit Ambient Population Under One Global Budget

- Status: Accepted
- Date: 2026-07-12

Context: Per-town civilian targets could multiply physical actors without a
global ceiling, and spawn-request success was treated too closely to usable
behavior. A requested traffic driver might not occupy the pilot seat, start the
engine, receive a live route, or move; repeated reconciliation could then add
work or leak transient save rows. Distance from spawn also could not distinguish
a player claim from AI movement. These ambiguities made ten-town load, recovery,
persistence, and the reported periodic stutter impossible to certify.

Decision: Keep logical town population authoritative and make every ambient
pedestrian, driver, and vehicle projection disposable. One pure allocator owns a
global actor budget and a nested traffic budget, counts each driver in both,
and distributes constrained demand through pedestrian floors, traffic floors,
and a rotating fair remainder. Allocation priority is leased for at least 120
seconds, while production rotates first reconciliation and may begin at most
four ambient root transactions per global health update. An in-flight root
reserves budget but is admitted as ready only after native behavior
acknowledgement: exact living group membership and a current waypoint for a
pedestrian; exact pilot occupancy, engine-on state, and a current route waypoint
for traffic. Movement health has startup grace, a no-progress deadline, bounded
recovery/backoff, and terminal recycle. Each actor owns an immutable projection
slot within its zone/kind reservation set; recovery keeps that identity and
derives a slot- and attempt-specific route.

Player occupancy is the only ambient-to-durable ownership signal. An occupied
ambient vehicle becomes a `field_vehicle`; movement distance alone never claims
it. Unclaimed ambient rows and linked cargo are excluded from save capture and
restore, and a legacy live detached ambient claim normalizes to the same field-
vehicle form. Campaign-state Schema remains 64 because the ambient lifecycle is
session-only; runtime-settings advances from Schema 23 to 24 for the new global
budgets and health controls.

Observe claims player-first before persistence on every server frame, then
repeat the observation synchronously at every production persistence boundary.
Player-first discovery avoids a full ambient-root occupancy scan, rejects dead
controlled occupants and destroyed roots, and fails the
capture closed when a promoted root lacks exact durable authority. Use one
session-only tracker for ambient promotions, field-vehicle restore/adoption, and
garage redeploy so transform, destruction, and cargo position are current at
capture. Preserve each saved durable ID across process restart; a current
process-local replication ID is not durable identity and cannot rekey a row or
its cargo. Run restore/registration before first-frame claim observation. Exact
registered entity/runtime-ID bindings win; initial recovery may
use only a unique same-prefab root within eight meters and fails closed on
ambiguity. At new-campaign reset, retain only occupied live tracked
`loot_vehicle`, `field_vehicle`, or `garage_redeploy` roots, normalize retained
rows to `field_vehicle`, copy their vehicle/cargo rows before state replacement,
and delete each other bound old-campaign root once. The scoped Campaign Debug
Phase 20 population path must select its allocation from the production global
plan and share the four-root transaction-start cap.

Consequences:

- Five traffic vehicles is the default daytime/low-heat demand target for a true
  town, not a fixed ceiling or a guarantee of five simultaneous cars. The setting
  can raise or lower it; remaining population, war
  level, connected-player budgets, and competing locality demand can reduce the
  allocation.
- Per-locality pedestrian plus driver demand is capped to the unique valid
  concrete appearance pool. Traffic reserves its demand first, pedestrians use
  the remainder, duplicate config entries do not inflate capacity, and selector
  exhaustion returns failure rather than a repeated visible actor.
- Pending and recovering roots consume their reserved capacity without being
  reported as behavior-ready. Failed acknowledgement cannot skip lifecycle
  states; illegal transitions are read-only. A dead driver, destroyed vehicle,
  lost authority, stuck root, or exhausted recovery budget recycles the whole
  transient aggregate rather than changing logical population.
- Immutable per-zone/kind slots prevent a replacement from colliding with an
  existing reservation, and give each recovery attempt a distinct deterministic
  path while preserving actor identity.
- Static military ambience treats owner or policy-key changes as replacement
  boundaries. It recycles unclaimed old roots, promotes player claims, resets
  its bounded initialization slots, and repopulates under the shared four-root
  transaction cap.
- Routine ambient movement, readiness, retry, and cleanup do not dirty campaign
  persistence. Only promotion of an observed player claim reports a durable
  mutation. This closes a source-level periodic dirty-state path but does not
  prove that the visible stutter is gone.
- Promoted vehicle snapshots do not themselves schedule a new save. They ensure
  that a pending autosave or major checkpoint captures the latest live transform,
  destruction, and cargo position; an inexact pre-capture reconciliation defers
  that checkpoint.
- Garage redeploy is one transaction: allocate a fresh campaign-stable vehicle
  ID, register the live root, restore cargo, remove the stored row, and spend.
  Failure rolls back the root, runtime/cargo rows, binding, and stored row rather
  than returning failure after a partial commit.
- Frame-frequency horn clearing walks the bounded ambient actor records directly;
  helper-by-root nested lookup is forbidden on this hot path.
- Pure allocator, lifecycle, settings-migration, and save-boundary proofs are
  compiled and wired into Campaign Debug. The sealed Schema-64 source/Workbench
  checkpoint for this decision identifies implementation
  `6afadc7c13681b78171939a740862e52328beffd`, UTC
  `2026-07-12T15:57:55Z`, and label
  `schema64-settings24-ambient-runtime-authority`. It passes
  Foundation at 711 references and normal/all-five Workbench validation at
  5,799 files/11,718 classes with CRC `bb083672`, zero HST script errors, and
  zero surviving Workbench processes.
- Campaign Debug execution, a native server, ten simultaneously eligible towns
  for ten minutes, native brief enter/exit, autosave/restart, promoted-root
  destruction, new-campaign reset, Campaign Debug Phase 20 production-path execution,
  rendered movement, profiling, multiplayer, and automatic casualty/theft/nearby-
  combat influence and panic/recovery behavior
  remain open gates. Commander aid and ownership/
  security-pressure paths exist in source but need runtime proof; deeper local-
  security behavior remains implementation work.

## CRI-012 - Route Civilian Consequences Through Exact Town Authority

- Status: Accepted; source/Workbench sealed
- Date: 2026-07-12

Context: The sealed ambient allocator could create, move, recover, recycle, and
promote disposable civilian projections, but actor death, vehicle theft, nearby
combat, and panic still had no automatic authority boundary. Applying these
effects directly from native callbacks would risk duplicate population loss,
aggression, and support changes. Treating every `HOT` zone as a civilian battle
would also turn an ordinary garrison into permanent fear and political pressure.
Minor localities need visible reactions without being promoted into political
towns.

Decision: Campaign Schema 65 adds one bounded civilian-consequence envelope to
each locality while keeping runtime-settings at Schema 24. Native observation is
an adapter only. A tracked civilian death reserves an exact event ID from the
persisted monotonic campaign allocator and enters a session queue capped at 256
rows; deferred theft uses a separate 64-row queue. The two queues share a maximum
of four attempted consequence transactions per server frame. Failed rows remain
queued indefinitely with bounded 5/10/15-second backoff, and capacity exhaustion
retains the observation rather than dropping it. Any retained receipt, queue
row, or authority fault defers persistence capture. A civilian vehicle is
promoted by the player-first claim path before a resistance theft event is
derived from its durable runtime ID. Only an exact player pilot is a claimant;
passenger-only roots remain non-recyclable during budget/health cleanup until
exit or pilot claim, but passengers do not promote the root or create theft.
Nearby combat begins a new consequence episode only when combat
presence supplies a current-operation or recent-fire fact; `HOT` without those
facts is inert, and an older pending episode receipt drains before a new edge.

Town events may carry one enemy aggression target, a positive bounded delta, and
exact before/after values. Admission requires unique target-pool authority,
arithmetic headroom, economy and strategic services, and an unclaimed strategic
source. One applied `town_influence` strategic receipt must match the event's
source, target faction, zone, timestamp, and aggression delta. Exact replay may
return that evidence but may not mutate town or aggression state again.

Physical pedestrians react through a native adapter. A danger episode or native
`EAIThreatState` transitions an admitted pedestrian from `Wandering` to
`Panicked`, replaces wander helpers with one move waypoint away from the threat,
and requests `EMovementType.RUN`. Once calm, the actor enters `Recovering`,
restores deterministic wander helpers at `EMovementType.WALK`, and returns to
`Wandering` after native waypoint acknowledgement. Panic remains behavior-ready
and does not spend the ordinary stuck-recovery budget. Lost/stalled panic routes
use a separate bounded recovery counter; the panic/recovery hot path never calls
AI activation again.

Consequences:

- Casualty callback re-entry cannot partially mutate durable authority. The
  actor's observed flag and exact receipt fence the callback and dead-character
  fallback. Queue capacities, per-frame work, and retry delay remain bounded,
  but rejected receipts are never discarded merely because the backoff cap was
  reached.
- Resistance casualty and theft policy may add enemy aggression only through an
  exact town event and its matching strategic receipt. Enemy or unknown casualty
  attribution follows its own bounded support/heat policy and cannot be silently
  redirected.
- Combat episode count, the adopted Schema-64 baseline floor, last-applied
  episode, last combat-presence revision, danger state, panic deadline, last
  event ID, and envelope revision survive restart. The adopted floor is `0..1`,
  `episode - lastApplied` cannot exceed one, and a canonical-town edge can append
  at most one political event. Ambient actors, panic waypoints, threat positions,
  and callback queue rows remain disposable session topology.
- Current restore requires each post-adoption live combat receipt to retain
  exact `+4` heat, zero other political/aggression/population effects, canonical
  source/reason, and unchanged support/population before/after values.
- Restore builds bounded influence/strategic indexes and validates structural
  town/aggression evidence before the civilian envelope, including the exact
  live-town last-event backlink. Pre-65 events gain empty/zero aggression fields
  without invented history. Save shape can prove unique pools, arithmetic, and
  receipts but not unserialized faction roles, so the coordinator immediately
  checks every restored aggression target against the live preset and
  quarantines a non-enemy chain. Malformed town/strategic evidence quarantines
  at `-64`, then malformed consequence-envelope authority quarantines at `-65`
  with danger/panic cleared. Restore never replays an applied consequence.
- Persisted stable-ID exhaustion never wraps. If strategic admission cannot
  obtain a new exact ID, the event fails before state mutation.
- A minor locality is panic-only: it receives no political population, support,
  aggression, or strategic event. Its bounded exact fingerprint map is session-
  only, so cross-process minor-locality replay/conflict identity remains an
  explicit limitation rather than a claimed restart guarantee.
- The current Schema-65/settings-24 source/Workbench checkpoint identifies
  implementation `609add9eeadf73816764c497178e2d35081307d1`, UTC
  `2026-07-12T18:30:29Z`, and label
  `schema65-settings24-civilian-consequence-authority`. Final stamped normal and
  all-five Workbench checks are clean at 5,802 Game files/11,728 classes with CRC
  `c0a672b9`, `Script validation successful`, zero HST script errors, and zero
  surviving Workbench processes. Foundation passes at 717 script-symbol
  references. Campaign Debug, native package, real profile save/restart,
  multiplayer, and soak gates remain open. Schema 64/settings 24 is the previous
  sealed checkpoint.

## CRI-013 - Make Enemy-Town Police An Exact Roster Authority

- Status: Accepted; sealed in Schema 66 source/Workbench, runtime proof pending
- Date: 2026-07-12

Context: Police presence was durable political/security pressure, but the
rendered `town_security_police` group was a disposable generic projection. A
physical group could lose members, leave the render bubble, and later return at
full strength because no aggregate owned the exact casualties. Treating police
presence as a living roster would create the opposite error: an abstract scalar
could invent members, fold casualties, or resurrect a destroyed patrol. A
resistance-held town also should not generate civilian police or roadblocks just
because old pressure had not yet drifted away.

Decision: Campaign Schema 66 adds one `HST_LocalSecurityPatrolState` per eligible
canonical enemy-held town while runtime settings remains Schema 24. The envelope
freezes a deterministic epoch identity from town, faction, ownership revision,
and epoch; one authored 2–5 member manifest; one local-security operation; one
SpawnQueue projection; and exact living slots. Police pressure selects roster
size and eligibility but never becomes casualty authority. The group may use
process-local cyclic town waypoints while physical. Before it folds, saves, or
settles, the exact adapter reconciles physical deaths; the same surviving slots
then enter strategic hold. Restore clears process-local handles and resumes from
that held roster. No route is persisted and no fold or restore can refill a dead
slot.

Complete destruction admits one exact `local_security_patrol_destroyed` town-
influence event with police `-1` and every other delta zero, then settles the
epoch as destroyed. Positive police pressure alone cannot reopen that epoch. A
new epoch requires either a newer canonical ownership revision or a later
applied positive police-pressure event. Owner change, pressure clear, setup,
campaign stop, and spawn failure settle without a destruction loss. Ownership
transition preflights and retires the old exact graph before owner publication;
persistence folds every open physical/dematerializing patrol or defers capture.
Resistance-owned towns target zero automatic police and roadblocks. The separate
conservative resistance garrison policy remains unchanged.

Consequences:

- Generic PhysicalWar fold, survivor, garrison, and repair paths must not claim
  local-security groups. Only the exact service owns their casualties and
  runtime retirement.
- Pre-66 migration removes only backlink-free disposable legacy police groups
  and clears stale patrol backlinks. It preserves police/roadblock pressure,
  ownership, support, and garrisons and invents no roster, casualty, operation,
  fold credit, or refund. The migration records one audit event.
- Current graphs require unique reciprocal patrol/zone/operation/manifest/batch/
  group identity. Malformed or conflicting authority quarantines at `-66` and
  cannot fall back to legacy behavior or exert combat pressure.
- Terminal history retains the compact operation and frozen manifest while the
  runtime batch and active-group row are removed. The envelope is reused only
  when a valid rearm source opens the next deterministic epoch.
- Deterministic proofs cover catalog bounds, admission replay, eligibility,
  casualty-preserving fold/restore, no refill, destruction replay, no-loss
  settlement, owner/positive-pressure rearm, and conflict quarantine. Native
  group/waypoint/casualty/fold, real save/restart, multiplayer, and soak remain
  independent gates.

## CRI-014 - Keep Projected Campaign Markers System-Owned

- Status: Accepted; sealed in Schema 66 source/Workbench, rendered-input proof pending
- Date: 2026-07-12

Context: The Schema-61 client-local marker projection introduced at commit
`27672e6` correctly removed the duplicate server-native campaign set, but its
static creation path called the stock local-only insertion method. That method
assigns the local player as owner before widget creation, overriding the intended
system ownership. As a result, a player could move, edit, or delete markers that
were derived campaign presentation rather than player annotations.

Decision: A protected client-local campaign marker bypasses the stock local-
owner assignment. It enters the native static array with owner `-1`, removal by
owner disabled, and those values reapplied after creation. The client readiness
keepalive checks native identity, position, text, icon, faction flags, owner,
removal policy, rotation, and timestamp visibility against the committed marker
registry. A missing or mutated native marker is rebuilt from that registry even
when the server stream has no new revision. Player-created and dynamic player
markers remain on their separate path and remain editable.

Consequences:

- The logical registry remains projection authority; a native marker edit or
  deletion cannot mutate campaign state.
- System-owned campaign markers and player-authored markers deliberately use
  different insertion and reconciliation paths.
- Packaged host/client testing must attempt delete, drag, and edit operations,
  observe non-removability or bounded self-heal, then repeat across map reopen,
  reconnect, and late join. Source inspection and Workbench compilation do not
  close that rendered-input gate.
- The source/Workbench-sealed, runtime-unexecuted Campaign Debug owner-client
  probe deliberately mutates and
  deletes one tracked campaign marker, calls the production reconciler after each
  fault, and requires canonical system ownership, non-removability, registry
  stability, static-count stability, and exactly one repaired instance. It also
  creates, edits, and removes an ordinary player marker to prove path isolation.
  This destructive fixture is wired in source but remains unexecuted.

## CRI-015 - Canonicalize Enemy Strategic Resources Before Planning Expansion

- Status: Accepted; sealed Schema 67 source/Workbench checkpoint, runtime proof pending
- Date: 2026-07-12

Context: Enemy attack resources, support resources, and aggression already drive
income, QRFs, patrols, pressure, and target selection, but broad-alpha mutation
paths do not provide one durable replay boundary. Exact defensive-QRF and patrol
operations can own their force lifecycles while still depending on direct or
weakly correlated pool debits/refunds. Persisting target decisions before these
inputs are canonical would freeze planning on top of ambiguous accounting and
make restart duplication or free assets difficult to reject.

Decision: Campaign Schema 67 keeps runtime settings at Schema 24 under sealed
label `schema67-settings24-enemy-strategic-resource-authority` and makes one
versioned `HST_FactionPoolState` the canonical attack-resource,
support-resource, aggression, and resource-income/aggression-decay cadence owner
for each configured enemy role, including persisted last-bucket checkpoints and
a per-faction operational count. All enemy income, spend, refund, aggression,
and live admin/debug pool adjustments enter through one server API. Each
accepted mutation appends a bounded `HST_EnemyStrategicMutationState` receipt
that freezes stable identity, contract/applied state, faction identity, mutation kind,
per-faction operational sequence, signed deltas, before/after values, source,
campaign second, and any exact order,
operation, and accounting backlinks. Exact replay returns the retained result
without mutation. Reusing the identity with different facts, arithmetic
underflow or overflow, an invalid enemy role, or a broken required backlink
fails closed and leaves every pool value unchanged.

Consequences:

- Occupier and invader resource/aggression histories are independent; no
  first-enemy or shared-pool shortcut may cross roles.
- Exact defensive-QRF and enemy-patrol order/operation shapes remain unchanged.
  Their admission debits and terminal refunds link to canonical Schema-67
  receipts through reciprocal order mutation IDs so replay/restart cannot
  duplicate an order charge or refund. Current restore also validates the unique
  defense ledger and typed town-influence/ownership-transition sources.
- Unsupported enemy order families remain explicitly legacy/deferred. Schema 67
  does not silently exactify their order lifecycle, reserve their forces, or
  grant them a durable planning decision.
- Pre-67 migration adopts each valid pool's current attack, support, aggression,
  and valid legacy resource/aggression cadence accumulators as its baseline. It
  initializes matching last-bucket checkpoints and a zero operational count, and
  creates no historical receipt, spend,
  refund, settlement, order, target decision, or planning event. Malformed
  current authority quarantines at `-67` rather than falling back.
- Operational receipts are lifetime replay evidence: do not compact or evict
  them in Schema 67. Each faction retains a contiguous sequence up to 4,096
  accepted operational rows, including zero-effect commands. Later operational
  admission for a capped faction fails closed; the rival faction and compact
  per-faction periodic evidence continue independently.
- Invalid, orphaned, rejected-role, and already-quarantined receipt rows are
  attributed and quarantined before they are removed from the canonical receipt
  array. They are not replay evidence and cannot consume the valid operational
  capacity of either enemy role. The affected authority still fails closed; row
  removal is capacity isolation, not repair or acceptance.
- A mission terminal outcome preflights its complete direct and deferred
  strategic mutation plan against a deep-copy authority before any terminal
  status, reward, support, capture, or cleanup publication. Ownership-change
  aggression admission likewise precedes security/support replacement and owner
  publication. Rejection leaves the outer transaction retryable instead of
  publishing a partially settled mission or zone.
- Mission terminal strategic-event IDs derive deterministically from outcome kind
  and mission instance, so rejection/retry cannot consume the shared authority
  sequence. Threshold-crossing mission capture uses one shared ownership-request
  builder for read-only admission and live apply, while the matching aggression
  command is preflighted on the clone. A non-admittable ownership request leaves
  direct receipts, rewards, capture progress, events, and terminal mission state
  unchanged; an admitted pending receipt remains durable retry authority.
- Schema 67 is sealed at implementation
  `2798cb20b824ed74419ab6dc9bdce03f18ef71df`, UTC
  `2026-07-12T23:46:02Z`, label
  `schema67-settings24-enemy-strategic-resource-authority`, and Foundation 736.
  Final stamped normal log `logs_2026-07-12_19-52-14` and all-five log
  `logs_2026-07-12_19-52-36` both compile 5,809 Game files/11,751 classes with
  CRC `a353fa0d`. All-five reports `Script validation successful` for WORKBENCH,
  PC, XBOX, PS4, and PS5; zero HST script errors were observed and zero
  Workbench processes survived cleanup. All Blueprint Phase 8 runtime gates
  remain open; this is not Campaign Debug or packaged runtime execution.
- Schema 66 is the immediately preceding sealed source/Workbench checkpoint at
  implementation `a7031797e67d99a99a066038cd8fa39efc03cff1`, UTC
  `2026-07-12T20:28:33Z`, label
  `schema66-settings24-local-security-marker-integrity`, Foundation 729, and
  Workbench 5,806 files/11,740 classes with CRC `ec860be7`.
- Persisted per-enemy planning cadence, deterministic candidate ordering, and a
  frozen target/source/order/cost decision fingerprint are implemented by the
  sealed Schema-68 contract in CRI-016 on top of this resource
  authority.

## CRI-016 - Persist Enemy Planning Without Taking Over Resource Truth

- Status: Accepted; sealed Schema 68 authority with a schema-neutral,
  commitment-aware source/Workbench correction and focused engine proof
- Date: 2026-07-12

Context: Sealed Schema 67 makes enemy resources, aggression, cadence, and
mutation receipts canonical, but commander planning remains process-local.
Restart can therefore lose which deterministic target/source decision was due,
whether its target pressure was already applied, and whether order/debit
creation crossed a crash boundary. Reconstructing that choice from current
zones, historical orders, or Schema-67 receipts would silently re-plan with
different inputs and could duplicate pressure or accounting.

Decision: Campaign Schema 68 keeps runtime settings at Schema
24 and adds one separate `HST_EnemyPlanningState` per configured enemy role.
Each row owns an independent 180-second checkpoint and freezes the latest
decision. Stable sorted commitment, target-candidate, and source-candidate
hashes make complete-set identity independent of traversal order. The frozen
input includes war level, aggression, Schema-67 pool revision/balances,
operational mutation count, commitments, and candidate counts. The frozen
decision includes target, source, order type, support type, capability/manifest,
spend mode, attack/support cost, target-pressure facts, and deterministic
decision, order, operation, and debit accounting IDs. Restore recomputes input
and decision fingerprints instead of trusting serialized hashes.

Target-candidate admission is part of that authority, not a separate heuristic.
Queued or active same-faction orders and support requests and open same-faction
operations at an equivalent target collapse to root commitments. Incompatible
roots remove a target before ranking. Compatible roots retain the target with a
deterministic `-12` score penalty per root, capped at `-24`. An exact active
patrol is compatible with a non-patrol defensive response, while the final
order-type gate still rejects a duplicate patrol. If compatible and blocking
rows resolve to the same root, conservative blocking precedence classifies that
root once. Known equivalent canonical/legacy zone IDs are the same target, and
permutation-stable rejection count and first-reason diagnostics describe why a
candidate was removed.

Consequences:

- Occupier and invader planning checkpoints are independent. Failure, retry,
  exhaustion, or quarantine for one faction cannot advance or suppress the
  other faction's checkpoint.
- Target ranking freezes the full `ept2` candidate identity before order-type
  selection. If the highest exact-patrol-compatible candidate later resolves to
  `PATROL`, preparation excludes that zone from the current selection pass and
  reruns the same deterministic ranking without changing the full candidate
  fingerprint. It can freeze a valid fallback, or a clean no-target result, in
  the same due decision instead of spending the cadence on a duplicate-patrol
  skip.
- A due decision first becomes `prepared`. Preparation is freeze-only: it does
  not apply target pressure, debit resources, or create an order. Before a
  prepared decision without a durable order can debit or create, admission
  always recomputes the commitment fingerprint, including on a pressure-marked
  retry. An unpressured decision also recomputes the `ept2` target-candidate
  fingerprint and frozen target/source/order gates before pressure. A
  post-freeze race therefore rejects an unpressured row before any side effect
  and a pressure-marked retry before debit. A retry retains the frozen decision
  and uses a 30-second retry checkpoint.
- Completion is explicitly `committed`, `skipped`, or `rejected`. Committed
  authority requires one exact order and one applied Schema-67 debit receipt; an
  exact prepared-plus-order crash window may reconcile to committed. If every
  target is incompatibly committed, preparation freezes zero target candidates,
  zero cost, and zero pressure and completes as `skipped` without changing the
  rival planner. Skipped authority cannot claim applied target pressure, while
  rejected authority may retain pressure already applied by an older or
  later-failing path.
- Transient failure before begin persists a 30-second preparation gate on the
  non-prepared role row, so missing capability cannot re-run every commander
  tick. A prepared row may validly persist before its positive pressure delta is
  applied. Pressure-mark authority and revision headroom are preflighted before
  the decayed ledger is normalized and mutated once.
- Exact QRF/patrol admission failure completes the planning decision only after
  the operation service leaves a durable aborted result. A null or non-terminal
  failure quarantines the planning/order link.
- Pre-68 restore creates no planning history. It clears claimed planning rows
  and planning backlinks on old orders, then creates one configured-role
  baseline only after exact Schema-67 pools validate: `last = elapsed`, `next =
  elapsed + 180`, sequence `0`, disposition `idle`, and no invented decision,
  target, source, order, manifest, cost, pressure, debit, candidate,
  commitment, or fingerprint. Old orders retain planning contract `0`.
- Current missing, duplicate, foreign-role, malformed, cadence-tampered,
  fingerprint-divergent, or broken order/debit authority quarantines at `-68`.
  Quarantine changes planning and order-planning metadata only. It never changes,
  repairs, debits, refunds, reconstructs, or replays Schema-67 pools or strategic
  mutation receipts.
- Immediate counterattacks and existing debug/direct order entry points do not
  claim periodic planner authority and remain planning contract `0`.
- The commitment-aware correction does not bump campaign Schema 68 or settings
  Schema 24. The target-candidate fingerprint prefix advances from `ept1` to
  `ept2` because compatible commitment count and penalty are now decision
  inputs. An older unpressured prepared row can fail closed at admission when
  its prior candidate identity no longer recomputes; no migration invents a new
  target or rewrites its decision.
- Planning quarantine has one production owner. Save validation bounds imported
  failure text and delegates to `HST_EnemyPlanningAuthorityService.Quarantine()`
  rather than duplicating field assignments. The authority therefore owns the
  same failure reason, one revision increment, and idempotent repeat behavior for
  restore validation and live planning. The retry-tamper fixture advances the
  campaign clock to the recorded retry time before testing fingerprint
  quarantine, so the proof first passes the real cadence gate.
- The commitment-aware correction is sealed at implementation
  `695caf46ce6b4146e5407711b76d5e0c578d7392`, UTC
  `2026-07-13T14:44:37Z`, label
  `schema68-settings24-commitment-aware-enemy-planning`, and Foundation 751.
  Final stamped-tree all-target Workbench log
  `logs_2026-07-13_10-45-27` compiles 5,815 Game files/11,768 classes with CRC
  `e483e71c`, validates WORKBENCH, PC, XBOX, PS4, and PS5 successfully, exits,
  and leaves zero Workbench processes. The expanded source proof covers queued
  order/support blockers, settled or terminal operation and rival-faction
  ignores, canonical/legacy zone equivalence, mixed-root blocking precedence,
  permutation-stable rejection diagnostics, deterministic patrol fallback,
  all-committed skip, and unpressured plus pressure-marked commitment races.
  Focused command-line Game-process case
  `HST_TEST_EnemyPlanningCommitmentAuthority` ran in
  `logs_2026-07-13_11-20-05`. Its JUnit report at
  `2026-07-13T15:20:12.403Z` records one testcase, zero failures, and an empty
  failed list; `AllExact()` passed for all 17 deterministic Schema-68 planning
  fixtures. This is isolated engine-executed service proof. Matching Campaign
  Debug assertions, HST_Dev/coordinator and world integration, persistence,
  package execution, save/restart, dedicated and live-server behavior,
  multiplayer, and soak proof remain open.
- The base Schema 68/settings 24 planning-authority checkpoint is sealed at
  implementation
  `356b0d47f96111c3b09eb7ede3cb34f0661c2b6e`, UTC
  `2026-07-13T01:04:41Z`, label
  `schema68-settings24-enemy-planning-authority`, Foundation 744, and Workbench
  CRC `971d30d0` at 5,812 files/11,761 classes. Final normal/all-five logs are
  `logs_2026-07-12_21-05-15` and `logs_2026-07-12_21-05-34`; all five target
  configurations validate successfully with zero HST script errors and zero
  surviving Workbench processes. The current shared state-only report now passes
  all 17 fields in the focused command-line engine case, but has not executed through
  Campaign Debug; native restart, package, dedicated-server, multiplayer, and
  soak evidence remains open.
- Schema 67/settings 24 is the resource-authority checkpoint immediately
  preceding that base planning seal at implementation
  `2798cb20b824ed74419ab6dc9bdce03f18ef71df`, UTC
  `2026-07-12T23:46:02Z`, label
  `schema67-settings24-enemy-strategic-resource-authority`, Foundation 736, and
  Workbench CRC `a353fa0d` at 5,809 files/11,751 classes.

## CRI-017 - Move Generated Profile Data Without Stranding Existing Campaigns

- Status: Accepted; sealed source/Workbench, runtime proof pending
- Date: 2026-07-13

Context: Public branding now uses Partisan, but generated settings, campaign
fallback data, loadout-editor preferences, personal loadouts, and debug artifacts
could still exist under the retired generated-data root. A file-by-file lazy read
would strand arbitrary nested data, retain duplicate sources indefinitely, and
make later consumers responsible for an incomplete cutover.

Decision: `$profile:Partisan` is the only generated-data root. Before server or
local-client consumers load settings, saves, preferences, loadouts, or debug
data, one shared service recursively enumerates the complete retired tree. A
nonconflicting file keeps its relative path. If the canonical destination already
exists with different bytes, canonical data wins and the retired file moves to
`$profile:Partisan/legacy-profile-archive` with deterministic collision suffixes.
Identical duplicates are deduplicated. A new destination is first copied to a
unique staging file and byte-verified; the canonical destination is rechecked
before promotion, and the source is compared with the promoted/archive file again
before deletion. Directory-path conflicts are mirrored under the archive's
`directory-conflicts` subtree. Directories are deleted deepest first and the
retired root is removed only when all file moves and empty-directory deletions
are re-verified. Any unverified operation leaves the source available and makes
migration retryable on a later startup.

Consequences:

- The Workbench folder name, non-public `histasi` project ID, `HST_*` technical
  convention, campaign schema 68, and runtime-settings schema 24 remain stable.
- Migration is byte-preserving and schema-neutral. After the tree move, normal
  settings/save/loadout readers still perform their existing semantic schema
  migration and normalization in the canonical root.
- A canonical conflict is never overwritten and is never used as permission to
  discard different retired data. The archive is data preservation, not a
  fallback source that consumers silently prefer over canonical state.
- A static in-process guard prevents recursive or duplicate migration calls.
  Enforce does not expose an atomic no-overwrite file promotion or an exclusive
  cross-process file lock, so this contract requires one process to own profile
  startup/migration at a time. Concurrent game, server, or Workbench processes
  must not share the profile while migration is active.
- The compatibility resolver remains a recovery net for a partial failed move,
  not the primary migration contract.
- The latest packaged test proved canonical-root creation only. No retired tree
  existed, so recursive copy, conflict archival, byte verification, source
  removal, empty-directory removal, and restart behavior remain open runtime
  gates. The schema-neutral correction is sealed at implementation
  `fdf262637e74a70c12454f6c1d3789c2cd0a0f05`, UTC
  `2026-07-13T13:19:22Z`, label
  `schema68-settings24-bootstrap-profile-marker-hardening`. Foundation passes
  at 751 script-symbol references; final stamped-tree all-target Workbench log
  `logs_2026-07-13_09-20-51` compiles 5,815 Game files/11,768 classes at CRC
  `0544aa1d`, validates WORKBENCH, PC, XBOX, PS4, and PS5 successfully, exits,
  and leaves zero Workbench processes. Those source gates do not prove real
  profile I/O, restart, package behavior, multiplayer, soak, or cross-process
  behavior.

## CRI-018 - Separate Fresh Enemy Bootstrap From Restored-State Validation

- Status: Accepted; sealed source/Workbench, runtime proof pending
- Date: 2026-07-13

Context: The latest packaged server test created a new Schema-68 state whose
configured enemy pools and planner rows were still empty when restored-role
validators ran. The validators correctly treated missing current-schema
authority as corruption and installed `-67`/`-68` quarantine rows. Foundation
initialization then saw rows present and did not replace them. Enemy planning was
disabled, and both configured roles emitted the unchanged unavailable warning on
each one-second commander tick: 598 warnings during the observed run.

Decision: One production bootstrap factory must construct the fallback used by
normal startup, admin reset, and deterministic fresh-state proof. That state must
contain the exact configured three-role pools and both idle enemy planning
baselines before it can enter the same validation pipeline as restored data.
Persistence may replace those arrays with serialized state;
therefore the restored validators remain before broad foundation repair and keep
missing, duplicate, malformed, or foreign authority fail-closed. For campaigns
already written by the known defect, a schema-neutral recovery may replace only
the complete generated quarantine signature: Schema 68 restored envelope,
exact nonempty preset identity, exactly three non-null pool rows containing one
neutral resistance row and two untouched `-67` enemy rows, exactly two untouched
`-68` planner rows, and empty
strategic-mutation and enemy-order arrays. Any field, identity, receipt, order,
null row, role, or cardinality mismatch rejects recovery without mutation.

Consequences:

- Fresh configured pools start at contract/revision `1/1` with configured
  balances, zero aggression/cadence accumulators, and no failure. Fresh planning
  rows start at `1/1`, `idle`, last bucket at the baseline second, and next bucket
  180 seconds later.
- Exact recovery restores the same baseline at the current elapsed second. It
  invents no catch-up income, aggression change, decision, order, receipt, or
  historical checkpoint.
- Unchanged planner-unavailable failures report immediately and then at most once
  per 300 seconds. Failure changes and successful recovery report immediately and
  reset the reminder state.
- Campaign Debug must prove fresh bootstrap, first validation, exact known-state
  recovery, resource/topology/preset/null-row/legacy-order/versioned-order
  rejection,
  unrelated-state preservation, one-shot idempotence, save roundtrip/validator
  acceptance, and warning transition/reminder/recovery timing. Its live-state
  check must call the read-only production exact-pool and exact-planner resolvers
  and require the exact three-role/two-planner topology without mutation.
- The shared correction seal is implementation
  `fdf262637e74a70c12454f6c1d3789c2cd0a0f05`, UTC
  `2026-07-13T13:19:22Z`, label
  `schema68-settings24-bootstrap-profile-marker-hardening`, Foundation 751, and
  final stamped-tree all-target Workbench CRC `0544aa1d` at 5,815 Game files/
  11,768 classes. It is not runtime-certified until Campaign Debug, a fresh
  packaged campaign, the affected-save restart, migration, multiplayer, and
  soak gates pass.

## CRI-019 - Prove Planning In-Engine And Centralize Idempotent Quarantine

- Status: Accepted; sealed source/Workbench and focused engine proof, campaign-
  world runtime proof pending
- Date: 2026-07-13

Context: The commitment-aware planning decision in CRI-016 was sealed and its
deterministic report was compiled, but compilation alone did not execute the
production service. Restore validation also duplicated planning-quarantine field
mutation, allowing repeated validation to risk revision or metadata churn instead
of sharing live planning's behavior.

Decision: Planning quarantine has one production owner. Save validation bounds
the imported failure reason and delegates to
`HST_EnemyPlanningAuthorityService.Quarantine()`. The first matching failure
normalizes the planning row and advances its revision once; linked order-
planning cleanup remains a separate fail-closed validator responsibility. An
already quarantined row is rejected without changing its revision or failure
strings on a repeated validation pass. Deterministic
planning service assertions run through the official focused command-line Game-
process autotest before the broader Campaign Debug rung.

Consequences:

- The retry-tamper fixture advances the campaign clock to its recorded retry time
  before triggering fingerprint quarantine, so it first passes the production
  cadence gate and then proves repeated-pass idempotency.
- The checkpoint is sealed at implementation
  `4c9a94a1cb4811b6e75a7dca5dba70efffcb523d`, UTC
  `2026-07-13T15:43:01Z`, label
  `schema68-settings24-enemy-planning-engine-proof`, and Foundation 753. Final
  stamped-tree all-target Workbench log `logs_2026-07-13_11-43-49` compiles
  5,816 Game files/11,770 classes with CRC `5a998c21`; WORKBENCH, PC, XBOX, PS4,
  and PS5 validate successfully, the process exits, and zero Workbench processes
  survive cleanup.
- Focused engine log `logs_2026-07-13_11-44-28` produces a JUnit result timestamped
  `2026-07-13T15:44:34.667Z` with one testcase, zero failures, an empty failed
  list, and `AllExact()` true across all 17 deterministic planning fixtures,
  including retry-quarantine repeated-pass idempotency.
- This rung does not execute Full Campaign Debug, HST_Dev coordinator isolation
  or artifacts, live campaign authority, persistence, package execution,
  save/restart, dedicated/live-server behavior, networking, multiplayer, or soak.
  Those gates remain open.

## CRI-020 - Version Enemy Counterattacks As Exact Operations

- Status: Accepted; scoped Schema-69 source/Foundation/all-target
  Workbench/focused-engine checkpoint sealed
- Date: 2026-07-13

Context: Counterattacks already existed as durable enemy orders, but their
legacy consumer did not make one frozen force roster authoritative across
off-screen travel, physical combat, bubble exit, capture, return, settlement,
and restart. Re-estimating a force after it leaves the render bubble can restore
dead members, lose an active mission, or let a timer replace the actual battle.
Cutting over old rows in place would also invent authority that was never saved.

Decision: Campaign Schema 69 appends a distinct enemy-counterattack operation
type and assigns contract `1` only to newly admitted exact counterattacks. One
reciprocal enemy-order, operation, frozen infantry-manifest, held-batch, and
active-group graph owns the lifecycle. The group travels on a direct virtual
route, uses the same living member slots when materialized, folds confirmed
casualties back into virtual authority, and resolves off-screen combat
deterministically against authoritative defenders. A successful attack submits
one stable request to the canonical ownership-transition service; it cannot
publish zone ownership directly. After the ownership result, survivors return
to origin and settle a proportional refund against exactly one originally
charged pool. Proactive counterattacks use attack resources; reactive capture
responses may use support resources. Projection transfer and combat do not
refund resources.

Consequences:

- Counterattacks restored from Schema 68 or earlier remain historical contract
  `0`. Migration never invents a source, operation, manifest, member roster,
  route, debit, refund, settlement, combat result, or ownership result.
- For newly admitted counterattacks only, this supersedes CRI-016's statement
  that immediate counterattacks remain contract `0`. It does not change the
  periodic planning contract or rewrite existing debug/direct history.
- Exact admission requires an infantry-only frozen manifest and exactly one
  positive attack/support cost. The other pool cost must be zero, and settlement
  must refund that same charged pool only.
- The appended `PREPARED` settlement state records terminal intent without
  renumbering earlier persisted values. Settlement prepares the operation,
  stages the complete order/refund tuple, applies or replays the canonical
  refund, records the resource receipt, and finalizes the operation only after
  the receipt is durable. Restore and same-session ticks resume any accepted
  prefix of this sequence.
- Physical and virtual modes are projections of the same durable aggregate.
  Confirmed deaths remain dead through fold, re-entry, restore, return, and
  settlement; no bubble transition may refill the roster.
- Current missing, duplicate, malformed, foreign, or ambiguous exact claimants
  quarantine at contract `-69`. Quarantine is idempotent and holds the graph; it
  must not delete authority, fall back to the legacy consumer, fabricate or
  apply a refund/settlement, or publish an ownership outcome. Claimant detection
  includes every deterministic batch, projection, force, and execution identity
  as well as explicit backlinks; optional uncommitted-full residue is removable
  only when it resolves uniquely.
- The schema adds no serialized fields. Its persistence boundary validates the
  existing reciprocal rows before generic projection normalization and may
  restore a valid physical projection as virtual authority at its last durable
  position while preserving its survivor ledger.
- The scoped checkpoint is sealed at implementation
  `5bdcda938840ab769b41ff3e1856d908572a8c45`, UTC
  `2026-07-13T19:40:35Z`, label
  `schema69-settings24-exact-enemy-counterattack-engine-proof`, with stamp commit
  `73a64ef`. Foundation passes at 771 script-symbol references. Final all-five
  Workbench log `logs_2026-07-13_15-41-50` exits `0`, compiles 5,821 Game files/
  11,786 classes at CRC `3a8bd64f`, explicitly validates WORKBENCH, PC, XBOX,
  PS4, and PS5, contains no script or HST errors, and leaves zero Workbench
  processes.
- Focused engine log `logs_2026-07-13_15-42-52` exits `0`, records one passing
  JUnit testcase, an empty failed list, and `AllExact=1`. It proves valid
  PREPARED recovery, same-session ABORTED recovery, foreign derived-ID collision
  hold, and fail-closed SETTLED-without-resource-receipt handling. The autotest
  environment also writes a recoverable base-game
  `SCR_EditableEntityCore/GetPlayerIdentityId` VM exception to `crash.log` before
  the HST case completes successfully, so the run is not exception-free.
- This seal does not certify Full Campaign Debug in `HST_Dev`, serialization or
  restart, package/native/live-server behavior, migration runtime, marker
  runtime, network/JIP/reconnect, or soak. Those gates remain open.

## CRI-021 - Version Enemy Garrison Rebuilds As Exact Held Forces

- Status: Accepted; scoped Schema-70 Workbench/focused-engine checkpoint sealed
- Date: 2026-07-13

Context: Enemy garrison rebuild orders previously lacked one authoritative force
identity across planning, travel, render-bubble transitions, delivery, later
combat, terminal settlement, and restart. Folding a delivered squad into an
aggregate infantry count would erase its individual casualty slots and could
double-count it while a physical or held projection still existed. Owner-string
checks alone also could not detect an ownership ABA between planning and debit.

Decision: Campaign Schema 70 appends a distinct enemy-garrison-rebuild operation
type and assigns exact contract `1` only to newly admitted rebuilds. Admission
freezes one infantry-only manifest bounded by authoritative source infantry and
target garrison capacity. The manifest and planning capability are preflighted
before the exact 10-support debit; after debit, the
enemy order, typed operation, manifest, held SpawnQueue batch, and active-group
projection form one reciprocal aggregate or exact rollback restores the debit.
The planning capability includes both
selected zones' owners and ownership revisions, and the order persists the
target revision.

Delivery records one zero-delta `delivered_garrison_transfer` receipt and links
the exact manifest to the destination garrison while the operation remains
`OPEN` and `ON_STATION`. It does not increment aggregate infantry. The same
durable living slots remain authoritative while the force is virtual, physical,
restored, or later removed. Before delivery, invalidated
survivors return to origin and eligible terminal paths refund the original
support debit exactly or proportionally. Delivered terminal invalidation,
destruction, or campaign stop unlinks and retires the held manifest without a
refund.

Consequences:

- Rebuild rows restored from Schema 69 or earlier remain historical contract
  `0`. Migration does not invent a manifest, operation, capacity decision,
  target revision, debit, casualty, delivery, or refund. A pre-70 row claiming a
  nonzero exact contract quarantines instead of being upgraded.
- A target or source ownership ABA changes the `epc70` capability hash even when
  the owner string returns to its original value. Initial admission rejects the
  changed capability before pressure; a pressure-marked retry rechecks and
  rejects before order creation or debit.
- Physical and virtual modes are projections of the same exact roster. Confirmed
  deaths remain dead through travel, fold, re-entry, restore, delivery, return,
  and settlement. Delivery never collapses the manifest into aggregate
  infantry.
- Once a nonzero exact enemy-order contract is admitted, its order ID and every
  deterministically derived reciprocal identity are immutable. Debug code may
  prefix contract-`0` compatibility rows, but exact aggregates are observed and
  cleaned by their original stable IDs; retagging one would sever operation,
  manifest, batch, group, debit, and receipt authority.
- Settlement reuses the appended `PREPARED` state. Resource receipt, operation
  settlement, and final order lifecycle are separately idempotent. Restore may
  repair a valid stale order tail from exact durable authority, but a conflicting
  receipt, terminal policy, or settlement identity quarantines without guessed
  cleanup or refund.
- Schema-70 restore classifies rebuild authority before generic active-group
  normalization and performs final validation after generic, ownership,
  strategic-resource, and Schema-69 counterattack prerequisites. Valid saved
  process-bound projections become process-free strategic hold while preserving
  casualties and durable position.
- Missing, duplicate, partial, orphaned, foreign, or otherwise ambiguous exact
  claimants quarantine at `-70`. Every claimed nonterminal batch becomes
  non-executable strategic hold, process-only group state is cleared, and
  retention pins all reciprocal evidence. Quarantine does not fabricate or
  erase a roster, settlement, delivery, casualty, debit, or refund.
- The checkpoint is sealed at implementation
  `2f71236bfc02329a3c8000b104f1b7b1043dc99c`, UTC
  `2026-07-13T22:20:52Z`, label
  `schema70-settings24-exact-enemy-garrison-rebuild-engine-proof`, and stamp
  `ef95555`. The sealed checkpoint's post-clock-fix Workbench log
  `logs_2026-07-14_01-06-19` exits `0`, completes create/destroy, compiles 5,826
  Game files/11,806 classes at CRC `b819d967`, contains no HST script error or
  new native crash event, and leaves zero Workbench processes. The focused log
  `logs_2026-07-14_00-52-56` at CRC `6fa838ee` records one passing
  `HST_TEST_EnemyGarrisonRebuildAuthority` JUnit testcase, no JUnit failure, an
  empty failed list, exit `0`, and zero surviving processes, but predates the
  coordinator clock correction. Foundation passes
  at 790 script-symbol references. The focused environment retains the known
  recoverable base-game VM diagnostic plus two filter-constructor diagnostics,
  so it succeeds but is not exception-free.
- The focused proof covers capacity/admission, held delivery, physical/virtual
  casualty continuity, restore, ownership terminal settlement, admission
  rollback, prearrival survivor refund, PREPARED/SETTLED crash resume,
  historical isolation, malformed and orphan quarantine, quarantine retention,
  and selected target/source ownership ABA rejection.
- Latest completed CLI run `seed1985_t0_p1_u1784017233` executed 679 cases:
  331 PASS, 255 WARN, 87 FAIL, and 6 BLOCKED. Certification proved
  5,092/5,328 assertions, with 210 failed and 26 blocked. Bootstrap and the
  exact zero final tracked-state diff passed; the run is not certified.
- Schema-70 clock/future normalization and Phase 18 exact counterattack,
  rebuild, and roadblock admission passed. The remaining Phase 18/22 cascade was
  an ambient coordinator commander tick that admitted an untracked Petros
  defense aggregate between cases. CRI-023 records the source correction and
  pending rerun. Other runtime failures, packaged/native, dedicated-server,
  serialization/restart, network/JIP/reconnect, and soak proof remain open.

## CRI-022 - Isolate Debug State Rewrites Through Typed Enemy-Order Settlement

- Status: Accepted; latest CLI execution proves the primary clock/cleanup
  boundary, follow-up cadence isolation pending rerun
- Date: 2026-07-13

Context: Full Campaign Debug creates both historical contract-zero enemy orders
and exact versioned operations. Retagging an admitted ID severs debit and graph
backlinks. Rewriting ownership or absolute strategic pools while an earlier
order remains open can invalidate its settlement basis, and merely marking an
order terminal can hide surviving physical/runtime claimants. A terminal
snapshot taken in the same frame as forced victory or loss can also record the
state before legitimate terminal maintenance settles those operations.

Decision: The debug coordinator keeps a separate stable-ID registry for every
created enemy order and never retags debit-backed or operation-backed
identities. Before Phase 18 background-war setup and every Phase 24 ownership,
pool, HQ, population, or aggression mutation, it must settle all tracked open
orders through their owning type. Exact QRF, counterattack, patrol, and garrison
rebuild services own versioned settlement. The enemy commander and PhysicalWar
service own contract-zero settlement. Any settlement or runtime-release failure
blocks the state rewrite, later escalation profiles, and aggression decay.
Terminal inactivity sampling occurs after one maintenance frame.

Consequences:

- Exact administrative settlement is successful only when the typed terminal
  ledger is valid and no batch, active group, adapter handle, physical root, or
  runtime member remains.
- Contract-zero administrative settlement proves the original one-pool debit,
  uses one deterministic refund mutation ID, requires exactly one correctly
  shaped and chronologically valid refund claimant, and retires linked support
  runtime before terminal status.
- Open-order counts use stable debug ownership rather than an ID prefix, so
  immutable versioned orders cannot disappear from cleanup assertions.
- Final disposable-state cleanup invokes the same typed settlement dispatcher
  before removing prefixed fixtures. Prefix deletion cannot stand in for an
  exact or contract-zero settlement, and any settlement/runtime-release failure
  is reported as a hard cleanup failure.
- Phase 24 profile and multi-cycle results carry explicit isolation failure
  evidence; a failure is a hard stop rather than a warning or partial probe.
- Latest completed Full Campaign Debug run `seed1985_t0_p1_u1784017233`
  emitted bootstrap PASS and an exact zero final tracked-state diff. Clock
  capture/restore, synthetic future-time normalization, enemy-authority
  fingerprint isolation, containment, mission cleanup, persistence-smoke
  cleanup, and Phase 18 rebuild admission passed. This proves the primary clock
  correction but does not certify the suite.
- The remaining Phase 18/22 post-case cascade was narrowed to an ambient
  coordinator commander tick outside the explicit fixture tick. CRI-023 records
  the follow-up cadence decision. Earlier native exact crew-seat
  materialization/rollback containment failures remain separate open runtime
  defects.

## CRI-023 - Isolate Ambient Debug Cadence And Delegate Marker Truth

- Status: Accepted in source; fresh Workbench and Full Campaign Debug proof
  pending
- Date: 2026-07-14

Context: Campaign Debug must exercise production enemy-commander ticks, but the
coordinator also runs its ordinary ambient commander tick between case frames.
After Phase 17 completed, that ambient cadence admitted an untracked Petros
defense order before Phase 18 seeded its own fixture. The first post-case probe
therefore observed valid production state that did not belong to the case and
the same aggregate polluted later Phase 22 assertions. Separately, the orphan-
marker checker recognized only legacy QRF/group backing and reported valid exact
QRF and patrol operation markers as orphans.

Decision: While Campaign Debug is running with its state-isolation clone active,
the coordinator holds only its ordinary ambient enemy-commander tick. Explicit
fixture calls continue to invoke the unchanged production tick at deterministic
points. The Phase 18 entry guard fails closed if an incidental untracked Petros
order or non-baseline defend-Petros mission already exists. Exact enemy-operation
marker backing is delegated to the marker publisher's public authoritative
predicate. That predicate requires the expected category, nonempty linked ID,
reciprocal operation ID, exact canonical marker ID, and the operation family's
own backing policy for QRF, counterattack, garrison rebuild, or patrol.

Consequences:

- Debug determinism comes from controlling coordinator cadence, not from
  bypassing the production commander or creating an aggregate and adopting it
  afterward.
- Ordinary gameplay is unchanged because the hold requires both an active
  Campaign Debug run and an active state-isolation clone.
- Explicit production ticks remain observable proof inputs, and the held-tick
  counter plus incidental-Petros entry assertions make a cadence leak visible.
- Marker prefixes are routing hints only. They cannot establish authority, and
  a canonical-looking marker without reciprocal publisher backing remains an
  orphan.
- The latest completed suite predates this source follow-up. Its 23 repeated
  post-case failures and repeated valid-operation marker warnings are causal
  evidence, not proof that the correction works. A fresh run is required.

## CRI-024 - Keep Workbench Proof Methods Below Native Compiler Pressure

- Status: Accepted; the 2026-07-14 compiler-shape checkpoint passed Foundation,
  Workbench compile/create/destroy, and bounded cold-open gates
- Date: 2026-07-14

Context: Workbench reproduced native `0xc0000374` heap corruption before the Game
module finished loading after additional local variables were added to an already
large campaign-debug render-bubble case. Script-level structure and Foundation
checks alone did not expose the failure. Removing unrelated functionality was
not necessary; the crash followed the method's local-state pressure.

Decision: Large coordinator proof cases store related mutable fixture state in a
small typed context object and delegate setup, sampling, restoration, and report
construction to narrow helpers. New proof coverage must not keep expanding one
monolithic method's local-variable footprint. Any structural growth in these
methods requires Workbench compile/create/destroy plus a timed cold-open check,
in addition to Foundation and source validation.

Consequences:

- That compiler-shape checkpoint passes Foundation with 793 script-symbol
  references, compiles and completes Workbench create/destroy at 5,826 Game files/11,807
  classes with CRC `287d01ec`, and remained alive at the 8-, 16-, and 24-second
  cold-open checks before deliberate shutdown. All launched processes were
  closed after verification.
- This is a native compiler/toolchain boundary, not a campaign schema or gameplay
  rule. No save migration or production authority change follows from it.
- The authoritative marker-backing and final typed-cleanup refinements are
  included in that exact-tree proof.

## CRI-025 - Separate Stable Ledger Identity From Debug Probe Telemetry

- Status: Accepted; exact-tree R10 runtime proof complete
- Date: 2026-07-14

Context: R9 proved that Phase 22 created, physicalized, and resolved the real
Petros attack, but its proof still produced false failures. It required a debug
prefix on an order whose admitted ID already anchored an operation and debit;
it compared strategic authority from before legitimate physicalization receipts
to the post-route state; and it treated loose formation as a failure even though
the group and all eight living agents requested RUN.

Decision: Campaign Debug never renames an admitted ledger identity. It registers
the stable order for typed cleanup and proves the order, operation, contract,
debit, and tracked ownership together. A synthetic route probe captures its
strategic comparison baseline only after legitimate setup and immediately before
route-time advancement. Response RUN succeeds from native group RUN intent or
all living agents requesting RUN; formation displacement remains telemetry and
does not gate the result.

Consequences:

- Exact-tree R10 finished Phase 22 with four PASS, three movement-only WARN, and
  zero FAIL. Stable identity, strategic isolation, and response RUN all passed.
- The movement warnings remain actionable runtime evidence; removing false proof
  gates does not convert a physically stalled group into a pass.
- Phase 18 remained five of five PASS, Phase 24 remained 11 PASS/one WARN/zero
  FAIL, typed enemy-order cleanup ended at zero failures/open/runtime claimants,
  and the final state diff was exact zero.
- This changes debug evidence and cleanup routing only. Campaign Schema 70 and
  runtime-settings Schema 24 remain unchanged.

## CRI-026 - Keep Generic Persistence Fixtures Out Of Exact Mission Contracts

- Status: Accepted; Foundation and Workbench passed; R17 runtime proof complete
- Date: 2026-07-14

Context: R16's seeded persistence roundtrip reported 11 active missions before
capture and 10 after restore even though every mission row, asset, runtime
entity, group, vehicle, and smoke sentinel remained present. The generic
destroy-target fixture declared `destroy_radio_tower` without the reciprocal
Schema-59 radio site, transition, revision, lock, and receipt authority. Restore
correctly classified and quarantined that malformed exact-radio claimant.

Decision: Mission definition IDs are authority contracts, including inside
debug fixtures. The generic destroy persistence sentinel uses
`destroy_outpost_cache`; `destroy_radio_tower` remains reserved for fixtures and
production missions that satisfy the exact Schema-59 lifecycle graph.
Foundation requires the generic mapping and rejects the former radio claimant.
Production radio classification and quarantine are not weakened or special-
cased for debug prefixes.

Consequences:

- The apparent R16 loss is classified as stale fixture setup, not proven
  production serializer data loss.
- The source correction changes no serialized field, schema version, mission
  runtime primitive, stable smoke instance ID, or migration rule.
- Stamped Workbench log `logs_2026-07-14_10-57-15` passes 5,826 Game files/
  11,807 classes, 46,639K static storage, CRC `734ab251`, create/destroy, and
  zero surviving processes.
- R17 `seed1985_t0_p1_u1784041822` proves 11/11 active smoke missions after
  restore, closing the generic-fixture count mismatch without weakening the
  exact radio lifecycle contract. R19 independently retains 11/11 while proving
  the later civilian-support correction.
- Real serialization/restart and the wider uncertified campaign-debug suite
  remain separate gates.

## CRI-027 - Preserve Schema-22 Civilian Support Zeros

- Status: Accepted; Foundation, Workbench, and R19 in-process proof complete;
  real restart pending
- Date: 2026-07-14

Context: After R17 corrected the generic persistence fixture, R18 restored all
11 missions and matched the summary hash but changed
`civilian_occupier_support` from `2514` live to `2614` restored. A legitimate
current-schema non-town civilian row carried zero FIA and occupier support. The
generic compatibility migration treated the occupier zero as missing legacy
data and inferred `100`, even though Schema 22 had already made both support
fields persisted authority.

Decision: For a civilian compatibility row without canonical town-influence
authority, inferred FIA and occupier support backfill runs only when
`restoredSchemaVersion < 22`. Schema-22-and-later zeros are valid saved values
and roundtrip unchanged. Curated towns continue to receive their compatibility
projection from validated Schema-64 town-influence authority; this decision does
not create a second political writer.

Consequences:

- This changes no campaign-schema or runtime-settings version, serialized field,
  or contract version. It corrects the legacy migration implementation to the
  pre-22 scope already documented for that backfill.
- Build `89b7754bcd9ac7e8c41f2a8d7604784b5c1c1c83`, UTC
  `2026-07-14T16:01:36Z`, label
  `schema70-settings24-current-support-roundtrip`, passes stamped Workbench log
  `logs_2026-07-14_12-02-05` at 5,826 Game files/11,807 classes, CRC
  `9d1cd471`, clean create/destroy, and zero surviving processes.
- R19 `seed1985_t0_p1_u1784044976` proves exact live/restored summary, report,
  and smoke counts: missions 11/11, assets 22/22, runtime entities 21/21,
  groups 9/9, runtime vehicles 10/10, field vehicles 1/1, and civilian occupier
  support 2514/2514. Only the intentional `persistence.real_restart` assertion
  remains BLOCKED for that case.
- Full R19 finished at 571 PASS/57 WARN/53 FAIL/7 BLOCKED and 5,492/5,665
  required assertions with an exact-zero final state diff. It is not a full
  certification, and real serialization/restart remains open.

## CRI-028 - Keep Campaign Debug Isolation Authority-Complete

- Status: Accepted; R21 runtime proof complete
- Date: 2026-07-14

Context: Campaign Debug isolation deliberately held the force-spawn worker while
the ordinary local-security producer still ran. That producer released Morton's
held patrol into `MATERIALIZING`, where the unavailable worker could not complete
the transfer; persistence correctly deferred rather than capture ambiguous
casualty authority. R20 then proved the transfer deferral was gone but exposed
two debug-proof defects: an isolated successful checkpoint was judged against a
production-only status prefix, and direct Phase 22 commander ticks appended four
orders that were not registered with the run's cleanup owner.

Decision: A debug-isolation guard that suspends a process worker also suspends
ambient producers whose durable transitions require that worker. The local-
security hold applies only while Campaign Debug owns its isolated clone;
production cadence is unchanged and the detached local-security proof may still
run through its own state and service. Checkpoint assertions distinguish exact
`isolated manual checkpoint` evidence from the production `checkpoint
requested:` prefix. Every order appended by direct debug commander ticks is
registered through the existing identity-safe cleanup path, and run leak
comparison uses open-order counts at both boundaries.

Consequences:

- Current build `3ded248a4ded084dfb0e3aa8e54ae0a47d36cd5f`, label
  `schema70-settings24-debug-cleanup-ownership`, passes stamped Workbench log
  `logs_2026-07-14_13-01-21` at 5,826 Game files/11,807 classes, CRC
  `c4a3e0a1`, clean create/destroy, and zero surviving processes.
- R20 `seed1985_t0_p1_u1784047342` contains no local-security materialization
  deferral and passes all eight local-security assertions. Its exact persistence
  roundtrip remains 11/11 missions, 22/22 assets, 21/21 runtime entities, 9/9
  groups, 10/10 runtime vehicles, 1/1 field vehicles, and civilian occupier
  support 2514/2514; the final tracked-state diff is exact zero.
- Source checkpoint `2508a735863c153f95bae94adb13f3037b4cdeef`, label
  `schema70-settings24-debug-checkpoint-evidence`, corrects isolated checkpoint
  classification. Current source checkpoint
  `3ded248a4ded084dfb0e3aa8e54ae0a47d36cd5f`, UTC
  `2026-07-14T17:00:29Z`, label
  `schema70-settings24-debug-cleanup-ownership`, adds incidental-order ownership
  and an open-row baseline. R21 confirms foundation checkpoint PASS with exact
  isolated evidence, typed cleanup PASS with zero open orders, and leak snapshot
  PASS at 0 -> 0.
- R21 `seed1985_t0_p1_u1784049066` remains uncertified at 564 PASS/63 WARN/54
  FAIL/7 BLOCKED and 5,494/5,659 required assertions, with 147 failed and 18
  blocked. It preserves all eight local-security assertions, exact persistence,
  and an exact-zero final state restore; only `persistence.real_restart` remains
  BLOCKED within persistence. Three `destroy_factory_asset` cases WARNed; the
  later R22 evidence recorded in CRI-029 supersedes their initial marker-timing
  classification with a production destroy-witness classifier defect. The
  separate world-scope restart block remains intentional.
- These corrections change no campaign schema, settings schema, serialized
  field, migration rule, or production gameplay cadence.

## CRI-029 - Make Retryable Spawn Work Visible Before Attempt Normalization

- Status: Accepted; R22 runtime proof complete
- Date: 2026-07-14

Context: The production spawn-queue acquisition path bounded and selected work
before `StartAttemptIfReady()` normalized retry/deferred state. Its
`HasSpawnWork()` gate recognized only `QUEUED` slots, so a batch containing
`FAILED_RETRYABLE` or `DEFERRED` work could never be selected to reach the
normalization that made the slot runnable. Retry generation stayed stale,
same-wave siblings could not progress, and restored nonterminal work could not
resume. The stale-generation and duplicate-suppression proof failures were
downstream consequences of that unreachable retry path, not independent
authority defects.

Decision: Bounded selection may see a `DEFERRED` or `FAILED_RETRYABLE` slot only
when its batch is itself `PENDING`, `DEFERRED`, or `FAILED_RETRYABLE`. Ordinary
`QUEUED` selection remains unchanged. Manifest exactness, dependency readiness,
and already registered siblings remain mandatory. Selection performs no state
mutation; generation advance and slot normalization remain in
`StartAttemptIfReady()` after the bounded winner is chosen.

Consequences:

- Current implementation `0b380f00fde65c4f2e22858faf8ddc6eab794131`, UTC
  `2026-07-14T17:40:21Z`, label
  `schema70-settings24-spawn-queue-resume`, passes stamped Workbench log
  `logs_2026-07-14_13-40-55` at 5,826 Game files/11,807 classes, 46,641K static
  storage, CRC `be31cb18`, clean create/destroy, zero HST script errors or
  fatal diagnostics, and zero surviving processes.
- R22 `seed1985_t0_p1_u1784051215` passes all 18 `spawn_queue` assertions. The
  retry advances generation 1 -> 2 at second 103 and records scheduled work,
  only-failed selection, stale rejection, completion, and replay exactly once,
  with four registered slots. Same-wave retry, sibling preservation, and
  terminal progression all succeed. Restore advances generation 1 -> 2 at
  reconciled sequence 9, changes once, is a no-op on repeat, and completes once.
- R22 is targeted proof, not a full certification: 555 PASS/60 WARN/66 FAIL/7
  BLOCKED and 5,422/5,632 required assertions, with 192 failed and 18 blocked.
  Foundation, authority foundation, and all eight local-security assertions
  pass. Seeded in-process persistence remains exact, and final tracked-state
  restoration is exact zero.
- The same run exposed a separate production classifier defect: worn grenade-
  vest, M433, and M72 equipment can be scored as a destroy witness and permit
  premature completion away from the intended physical target. CRI-030 records
  the structural active-projectile correction and R23 runtime proof. R21 remains
  the cleaner cleanup comparison because R22 and R23 each reproduce the separate
  exact-QRF settlement defect, leaving one tracked row and two total open orders
  while later Phase 24 failures occur in the same contaminated run.
- This correction adds no campaign-schema or settings-schema version, serialized
  field, contract version, or migration rewrite. Valid current-schema deferred
  and retryable rows become resumable automatically; the pre-Schema-44 migration
  rule remains unchanged.

## CRI-030 - Admit Only Active Physical Demolition Witnesses

- Status: Accepted; Foundation, Workbench, and R23 runtime proof complete
- Date: 2026-07-14

Context: The destroy-target proximity fallback classified nearby entities from
resource and component hints without first proving that they were active
physical projectiles or blasts. Worn grenade equipment, carried ammunition, and
an equipped launcher could therefore contribute demolition score while no
explosive had reached the target. A short cooldown reduced repeat frequency but
did not establish one-source authority or prevent the same physical source from
scoring again later.

Decision: Proximity witnesses must be unparented entities with a projectile
component. An inventory item occupying a parent slot is rejected. A trigger-
based projectile is eligible only after it has triggered; any other projectile
requires a moving projectile component with nontrivial velocity. Entity-backed
callback and proximity observations use one canonical prefab-plus-entity key.
Each destroy-target component retains at most 64 accepted keys for its lifetime,
rejects replay and fails closed at capacity. A key is retained only after the
authoritative mission asset confirms a real damage, hit, or destroyed-state
mutation. Campaign Debug samples the asset before explicit damage and requires
`primitive.destroy.no_ambient_witness_score` to prove zero ambient progress.

Consequences:

- Current implementation `0e54f6cbc7f7084e5534fc603b491cba0d91b653`, UTC
  `2026-07-14T18:31:39Z`, label
  `schema70-settings24-active-demolition-witness`, changes no campaign schema,
  settings schema, serialized field, contract version, or migration rule.
- Foundation passes at 793 script-symbol references. Headless Workbench PC Game
  validation log `logs_2026-07-14_14-41-29` compiles 5,826 Game files/11,807
  classes with 46,643K static storage at CRC `c3ab042e`, reports `Script
  validation successful`, contains no HST compile error or fatal diagnostic,
  and leaves zero processes after closure.
- R23 `seed1985_t0_p1_u1784054690` executes 688 cases at 564 PASS/51 WARN/66
  FAIL/7 BLOCKED and proves 5,440/5,650 required assertions, with 192 failed and
  18 blocked. All six generic destroy-target runtime cases pass
  `primitive.destroy.no_ambient_witness_score` at damage 0, hits 0, source none,
  evidence 0, and destroyed 0 before explicit damage. The exact radio-tower
  primitive and those six generic destroy primitives all pass their start,
  runtime, and primitive cases; their cleanup cases are WARN-only because each
  mission had already completed through its runtime path.
- R23 also preserves all 18 spawn-queue assertions, the exact seeded in-process
  persistence roundtrip at 11/11 missions, 22/22 assets, 21/21 runtime entities,
  9/9 groups, 10/10 runtime vehicles, 1/1 field vehicles, and civilian occupier
  support 2,514/2,514. Its final tracked-state diff is exact zero. Real process
  restart remains intentionally BLOCKED and the wider suite remains
  uncertified.
- The 45-metre proximity fallback remains evidence by proximity, not collision
  proof. Native callback source identity should replace or further constrain the
  fallback when that identity is wired reliably.
- R23 preserves the separate pre-existing exact-QRF settlement crash window
  exposed by R22:
  refund mutation is written before the complete settlement tuple is validated.
  That ordering can leave an open exact operation after a rejected settlement
  and two exact runtime claimants; run-completion cleanup records settled 0,
  failures 1, one tracked open row, and total open orders 0 -> 2. That ordering
  was the next authority defect at this decision boundary; CRI-031 records its
  later correction and proof.

## CRI-031 - Give Exact Defensive QRFs Their Own Refund Authority

- Status: Superseded by CRI-032; its Foundation, PC-only Workbench, focused
  engine, and R25b evidence remains dated history
- Date: 2026-07-14

Context: R23 exposed a refund-applied/order-receipt-clean crash window that could
leave an exact defensive-QRF operation open after settlement rejection. The
first publication-order repair kept the order tuple clean while the canonical
refund applied or replayed and then published the tuple with the applied flag
last. R24 `seed1985_t0_p1_u1784059798`, build `6303e58`, proved that typed cleanup
and the open-order leak were clean again, but its focused QRF settlement and
persistence assertions still failed. The settlement owner was validating a
defensive QRF through the exact-counterattack settled-refund helper, whose
exactly-one-charged-pool rule rejects a legal defensive QRF funded from support
alone or from both defense-resource values. The persistence proof also matched
an obsolete diagnostic phrase and rendered several values as booleans, hiding
the already-correct missing-backlink invalidation evidence.

Decision: Exact defensive QRFs own a family-specific settled-resource validator.
It requires one canonical defense debit and one canonical defense refund with
exact contract version, applied state, mutation identity, faction, source,
order, operation, manifest, zone, deltas, empty contribution shape, and valid
chronology. It accepts support-only or dual-pool defensive funding. Exact
counterattacks retain their separate exactly-one-pool rule. Settlement preflights
the full tuple, applies or replays the resource mutation while every order-side
receipt field remains clean, then publishes settlement identity, kind, refund
mutation, accepted/survivor counts, refund amounts, and the applied flag last.
Applied replay and restore invalidation both use the QRF-specific validator.
Proof diagnostics classify the stable resource-authority failure family and
print real status, runtime reason, pool, backlink, and settlement values.

Consequences:

- Implementation `434b73a16ae92911896fdec095af6bce88168916`, stamp `7fac7ac`,
  UTC `2026-07-14T20:54:24Z`, label
  `schema70-settings24-exact-qrf-refund-authority`, changes no campaign schema,
  settings schema, serialized field, operation contract version, or migration
  rule. Campaign Schema 70 and settings Schema 24 remain current.
- Foundation passes at 795 script-symbol references. PC-only Workbench log
  `logs_2026-07-14_16-56-29` validates 5,827 Game files/11,809 classes, 46,667K
  static storage, and CRC `12b7df72`; script validation, game create/destroy,
  diagnostics, and process cleanup are clean. This is not an all-five-target
  Workbench result.
- Focused log `logs_2026-07-14_16-57-04` records one passing
  `HST_TEST_EnemyQRFAuthority`, no JUnit failure, an empty failed list, and
  `AllExact 1`. It covers dual-pool returned-survivor settlement/replay,
  support-only full settlement/replay, terminal restore, partial-receipt
  quarantine, and missing-group-backlink invalidation with retained rows and
  unchanged resources. The harness still emits the known recoverable player-
  audit VM exception and two filter-constructor diagnostics, so it is successful
  but not exception-free.
- R24 remains the dated bridge: 688 cases at 578 PASS/50 WARN/53 FAIL/7 BLOCKED,
  5,522/5,682 required assertions proven, 142 failed, and 18 blocked. Its two QRF
  assertions failed, but typed cleanup reported settled 0, failures 0, open 0,
  runtime 0; the open-order leak was 0 -> 0; core Phase-24 and exact-zero final
  restoration passed.
- R25b `seed1985_t0_p1_u1784063032` completes 688 cases at 577 PASS/51 WARN/53
  FAIL/7 BLOCKED and proves 5,523/5,681 required assertions, with 140 failed and
  18 blocked. Both `enemy_qrf.settlement` and `enemy_qrf.persistence` pass.
  Typed cleanup remains settled 0/failures 0/open 0/runtime 0, the open-order
  leak remains 0 -> 0, seeded
  persistence matches 11/11 missions, 22/22 assets, 21/21 runtime entities, 9/9
  groups, 10/10 runtime vehicles, and 1/1 field vehicle, and the final tracked
  diff is zero. Core Phase-24 seed/report/outcome cases pass; escalation
  physicalization remains WARN.
- R25b is diagnostic, not certification. Unrelated failures plus real-restart,
  world-scope, manual external, package, native, dedicated-server/client,
  multiplayer, reconnect/JIP, and soak gates remain.
- The current authority is same-session replay-safe but has no durable
  `PREPARED` process-restart protocol between refund application and order
  receipt publication. That crash window remains a future schema/authority
  gate, and arbitrary old partial rows remain fail-closed rather than being
  inferred or auto-healed.

## CRI-032 - Persist Exact Defensive-QRF Terminal Intent Before Refund

- Status: Accepted; source, Foundation, PC-only Workbench compile/create,
  focused engine, and R26 in-process proof complete; CRI-033 records the later
  canonical-fallback external-restart proof
- Date: 2026-07-14

Context: CRI-031 gave defensive QRFs the correct support-only/dual-pool resource
validator and repaired the same-session refund/replay path, but it deliberately
left the order receipt clean until after the resource mutation. A process stop
between refund mutation and receipt publication could therefore retain a
canonical pool change without durable terminal intent. Recovery also had to
distinguish historical mutationless settlements from current rows whose
strategic receipt chain was malformed or removed during Schema-67
normalization.

Decision: Exact defensive-QRF settlement is a durable replayable state machine.
The operation first persists `PREPARED` terminal intent. The owner stages the
complete settlement/refund tuple with `m_bResourceSettlementApplied = false`,
then validates the original debit, every reciprocal claimant, and the durable
survivor authority. It applies or idempotently replays the canonical refund,
publishes the applied receipt last, and only then finalizes the operation and
order tail. A valid restore resumes any accepted prefix without changing the
settlement identity, refund amount, or mutation count.

Current strategic-receipt provenance is captured before Schema-67 normalization
can remove a malformed mutation. After normalization, a current-provenance
`SETTLED` row must still pass the complete debit/refund/chronology validator. A
bad chain is disarmed to unknown settlement authority, aborts the order, cancels
or holds reciprocal runtime claimants, and retains the stable
`exact_restore_resource_authority_quarantined` result across repeated restore.
Historical mutationless settlements remain compatible and do not acquire
current receipt requirements. Arbitrary old partial rows remain fail-closed;
recovery is limited to a complete durable `PREPARED` graph.

Consequences:

- Implementation `78db295a02936aa66899203cb33e50462b5fd557`, stamp `b1f105a`,
  UTC `2026-07-15T00:08:27Z`, label
  `schema70-settings24-exact-qrf-prepared-recovery`, changes no campaign schema,
  settings schema, persisted enum ordinal, serialized field, or operation
  contract version. Campaign Schema 70 and settings Schema 24 remain current.
- Foundation passes at 802 script-symbol references. Stamped PC-only Workbench
  compile/create evidence covers 5,828 Game files/11,816 classes with 46,859K
  static storage and CRC `62dac921`, contains no HST/script compile error, and
  leaves zero engine processes after closure. It is not an all-target
  validation result.
- Focused engine execution records one passing `HST_TEST_EnemyQRFAuthority`
  testcase, no JUnit failure, an empty failed list,
  `AllExact 1`, and a no-op second restore. Six committed dual-pool/support-only
  fixtures cover the pre-refund, post-refund/pre-receipt, and receipt/pre-tail
  cuts. Three uncommitted full-refund fixtures cover the same cuts. Corrupted
  prepared debit/pool authority, tampered survivors, bad current settled pool
  tails, and mutationless history are also covered. Known recoverable harness
  diagnostics remain, so the run is successful but not exception-free.
- R26 `seed1985_t0_p1_u1784074264` executes 688 cases at 577 PASS/51 WARN/54
  FAIL/6 BLOCKED and proves 5,504/5,667 required assertions, with 145 failed and
  18 blocked. `enemy_qrf.settlement` and `enemy_qrf.persistence` pass. Typed
  cleanup remains settled 0/failures 0/open 0/runtime 0, the open-order leak is
  0 -> 0, seeded restore counts match, and the final tracked-state diff is zero.
  Escalation physicalization remains WARN.
- The crash-cut fixtures use in-process `HST_CampaignSaveData` capture/restore.
  They prove the in-memory save-data copy/capture/restore shape and deterministic
  reconciliation but do not stop and restart the executable. R26 remains this
  historical in-process evidence. CRI-033 later closes only the exact defensive-
  QRF canonical-fallback restart cuts; package, native persistence-source
  selection, live-server behavior, migration runtime, multiplayer,
  reconnect/JIP, and soak remain open certification gates.

## CRI-033 - Prove Exact Defensive-QRF Recovery Through Canonical Fallback

- Status: Accepted; Foundation, stamped PC Workbench, focused engine, and
  guarded external 3x3 restart matrix complete
- Date: 2026-07-15

Context: CRI-032 made a complete defensive-QRF `PREPARED` graph replayable and
proved all accepted settlement prefixes through deterministic in-process
capture/restore. That evidence did not establish that a stopped executable
could persist a prepared prefix, start a fresh engine process, run the startup
reconciler exactly once, persist the terminal graph, and then start another
fresh process without applying the refund or terminal transition again.

Decision: Exercise this boundary with a destructive, guard-gated development-
world protocol that uses only the canonical JSON profile fallback. Each run
requires an explicit guard, validated run ID, stage, and cut; it is rejected
outside the development world or while Campaign Debug is active. `prepare`
writes and reads back the exact `PREPARED` source. A fresh `recover` process
validates that source before startup reconciliation, adopts it only for this
proof, requires the startup reconciler to change the graph once, requires an
explicit second reconciliation to be a no-op, then writes and reads back the
exact terminal graph. A fresh `replay` process validates both the terminal
source and prior recovery fingerprint, requires startup and explicit
reconciliation to remain no-ops, and preserves the terminal fingerprint. Each
stage writes its result last and requests process closure.

Consequences:

- Implementation `25b2dc361bc935aea904e08a665755840389c6e0`, stamp `ce2542b`,
  UTC `2026-07-15T02:08:19Z`, label
  `schema70-settings24-exact-qrf-external-restart`, changes no campaign schema,
  settings schema, persisted field, enum ordinal, operation contract version,
  or migration rule. Campaign Schema 70 and settings Schema 24 remain current.
- Foundation passes at 806 script-symbol references. Stamped PC Workbench
  validation compiles 5,830 Game files/11,820 classes with 46,915K static
  storage and CRC `ff59593b`, reports `Script validation successful`, and
  records zero script errors.
- Focused `HST_TEST_EnemyQRFAuthority` execution records one testcase, zero
  failures, an empty failed list, and `AllExact 1`. The environment still emits
  the known recoverable stock player-audit VM diagnostic, so this result is
  successful but not exception-free.
- The external matrix passes `prepare`, `recover`, and `replay` in fresh engine
  processes for all three cuts: before refund, after refund but before receipt,
  and after receipt but before the terminal tail. All nine stages report
  success and exit `0`. Each recovery observes exact prepared source, one
  startup change, an explicit same-state no-op, and exact terminal readback;
  each replay observes exact terminal source and remains a no-op at both
  reconciliation points.
- R26 remains the historical in-process proof and is not reclassified as an
  external run. This decision closes only canonical JSON fallback recovery for
  these three exact defensive-QRF settlement prefixes. It does not certify
  native persistence-source selection or precedence, package/runtime delivery,
  dedicated multiplayer, physical or world state, migration, reconnect/JIP,
  soak, or any other operation family.

## CRI-034 - Keep Native Exact-Counterattack Projection Under Its Production Owner

- Status: Accepted; sealed source, Foundation, Workbench compile/create, and
  R27 scoped runtime assertions complete; whole suite not certified
- Date: 2026-07-15

Context: The earlier Phase-17 exact-counterattack case proved the persisted
operation graph and deterministic state-level lifecycle, but it did not run a
native root and frozen survivor roster through the real render-bubble handoff.
Phase-24 escalation telemetry also treated support-request physicalization as a
generic signal even though exact counterattacks have their own runtime owner and
must never leak into the legacy support path. A debug-only model of either
boundary could pass while production ownership or native bindings were broken.

Decision: Drive one already-admitted exact counterattack through its production
runtime owner on real campaign frames. The coordinator asks the commander to
tick only the focal order; the operation owner moves its held `VIRTUAL` roster
to `MATERIALIZING`; a projection-scoped adapter call runs the production queue,
native-spawn, handoff, and deleted-staged-handle passes; the owner confirms
`PHYSICAL`; and render-bubble exit retires native runtime before returning the
same durable survivors to `VIRTUAL`. The probe may wait for the real monotonic
campaign clock, but it may not advance synthetic time or create a legacy
support request to force progress.

Phase-24 escalation telemetry resolves every sampled order to exactly one
supported production owner before evaluating physicalization. An open exact
counterattack must own one reciprocal projection in exactly one of `VIRTUAL`,
`MATERIALIZING`, `PHYSICAL`, or `DEMATERIALIZING`; a terminal exact
counterattack must own a valid retained ledger and no runtime claimant.
Transitional authority validates exact roster and root/member/handle
cardinality, projection/result keys, entity uniqueness, native group identity,
and matching adapter-to-PhysicalWar registrations. Exact counterattacks must
have zero legacy support ownership.

Cleanup first delegates to the exact operation's typed production settlement.
If failed proof setup still owns focal runtime, a bounded emergency path retires
both result and projection bindings from the adapter and PhysicalWar registries.
Both keys are mandatory and cleanup succeeds only when no matching ownership
remains. If the durable group row has disappeared, the group-keyed fallback
first retires exact process-local PhysicalWar authority, then explicitly deletes
each validated orphan entity only after proving it is no longer registered;
foreign registration, a live entity after deletion, or any residual handle
fails cleanup closed. If the tracked exact order disappears, the owner boundary
itself is lost: record a failed invariant, stop the isolated run, restore its
snapshot, save aborted artifacts, and do not publish later Phase-24 telemetry
from that run.

Consequences:

- The reciprocal IDs, frozen manifest, living-slot fingerprint, original
  one-pool debit, enemy resources, and player placement must survive the
  `VIRTUAL` -> `MATERIALIZING` -> `PHYSICAL` -> `VIRTUAL` cycle unchanged.
- This source slice adds no campaign-schema or runtime-settings-schema version,
  serialized field, enum ordinal, operation contract version, or migration
  rule. Campaign Schema 70 and settings Schema 24 remain current.
- The sealed source is `4757bc86ffbc7a5fa08e64a9abf7ef74ddc1c003`,
  UTC `2026-07-15T05:21:40Z`, label
  `schema70-settings24-native-counterattack-projection`, with stamp commit
  `a22e6af`. Foundation passes at 808 script-symbol references.
- Stamped guarded Workbench Game compile/create loads 5,830 Game files/11,822
  classes, uses 47,019K static storage, and records CRC `2526746c`. It creates
  the Game with zero SCRIPT, HST, ENGINE, INIT, crash, or unhandled-error
  signals, removes the guarded session, and leaves zero launched Workbench
  processes. The log has no literal `Script validation successful` line, so
  this is clean compile/create evidence rather than explicit Script validation.
- R27 `seed1985_t0_p1_u1784093667` runs `full_certification` through
  `cli_autostart` in an isolated settings-only profile. All six Phase-17 native-
  projection assertions and both Phase-24 owner-authority assertions pass
  exactly once, and all 29 state-diff lines remain zero. The guarded runtime
  session is removed exactly.
- The complete R27 suite remains uncertified: 688 cases finish at 577 PASS/49
  WARN/55 FAIL/6 BLOCKED/1 SKIPPED, while certification proves 5,500/5,663
  required assertions with 145 failed, 18 blocked, and zero warning assertions.
  R26 remains the historical in-process exact-QRF baseline.
- Arrival, combat, capture, return, save/restart, package/live-server,
  multiplayer/network, reconnect/JIP, and soak behavior remain separate gates.

## CRI-035 - Give Native Exact-Counterattack Casualties One Runtime Owner

- Status: Accepted; R28b proves the scoped casualty-continuity contract, while
  later bounded observation hardening is tracked by CRI-036
- Date: 2026-07-15

Context: CRI-034 proved a complete unchanged-roster native projection cycle,
but did not prove that an actual engine death becomes one durable casualty and
stays dead through fold and re-entry. Exact counterattack members were also
still visible to generic PhysicalWar survivor sampling while the projection-
scoped force-spawn adapter owned exact member bindings. Letting both paths infer
survivors could double-retire a slot, race the roster count, or resurrect a
casualty on rematerialization.

Decision: Exact counterattack groups are excluded from generic PhysicalWar
survivor sampling. Their force-spawn adapter is the sole physical casualty
owner. The Phase-17 probe resolves one living member through projection-scoped
authority, applies a real engine damage-manager kill, and asks that adapter to
reconcile the exact projection. Reconciliation must retire exactly one known
slot, detach its entity, reduce all durable and runtime counts from N to N-1,
and become a no-op on replay.

The retired slot remains a durable tombstone. Before fold it keeps the observed
entity ID as physical death evidence. Fold clears that process-local identity
but preserves `RETIRED`, ever-alive, and casualty-confirmed state. Re-entry must
materialize only the N-1 living slots with no entity or adapter handle for the
casualty, and a second reconcile replay must remain a no-op. Once authority is
detached, the proof deletes the dead entity explicitly; failure cleanup applies
the same rule so debug evidence cannot leak a corpse into the world.

Consequences:

- Stable order, operation, manifest, spawn-result, force, and projection IDs;
  survivor fingerprint; original debit and pool balances; ownership; and zero
  legacy-support authority must survive death, fold, re-entry, and final fold.
- This changes no campaign schema, settings schema, serialized field, enum
  ordinal, operation contract version, or migration rule. Campaign Schema 70
  and runtime settings Schema 24 remain current.
- The sealed source is `22c425f416d607bd8a94027e1d486dc3f06c4c47`,
  UTC `2026-07-15T06:31:32Z`, label
  `schema70-settings24-native-counterattack-casualty-continuity`, with stamp
  commit `e002e82`. Foundation remains at 808 script-symbol references.
- The first compile exposed four `Formula too complex` proof predicates. They
  were split into bounded checks without weakening the combined invariant. The
  corrected stamped Workbench Game compile/create passes at 5,830 files/11,822
  classes/47,065K/CRC `cf64f51d`, exit `0`, with zero script/crash signals and
  exact process, temporary-session, and log cleanup.
- R28 `seed1985_t0_p1_u1784098447` first finished 687 cases at 576 PASS/50
  WARN/54 FAIL/6 BLOCKED/1 SKIPPED and 5,517/5,684 certification assertions
  proven, with 149 failed and 18 blocked. The five casualty assertions failed,
  while all 29 state-diff lines, including 18 delta-bearing rows, were zero and
  no mission state leaked.
- The unchanged R28b rerun `seed1985_t0_p1_u1784100187` finished at 577 PASS/50
  WARN/53 FAIL/6 BLOCKED/1 SKIPPED and 5,523/5,684 proven, with 143 failed and
  18 blocked. All 11 Phase-17 targets passed exactly once; Phase-24 runtime
  ownership passed, exact authority was skipped, the casualty stayed N=9 ->
  N-1=8 through fold/re-entry, and the tracked state diff remained zero. This
  proves the scoped casualty-owner contract but also classifies the first R28
  failure as intermittent observation, not deterministic roster corruption.
- Natural route travel and combat beyond the scoped death, capture, return,
  settlement, save/restart, package/live-server, multiplayer/network,
  reconnect/JIP, and soak behavior remain separate gates.

## CRI-036 - Observe Native Death and Physical Authority Without Reissuing Work

- Status: Accepted; R30 completed diagnostically and exposed the post-handoff
  proof-ordering seam addressed by CRI-037
- Date: 2026-07-15

Context: The first R28 run failed all five casualty-continuity assertions, but
an unchanged R28b rerun passed the complete N=9 -> N-1=8 cycle. Death state can
be visible through the controller-aware predicate and the stock damage manager
on different observations. After death observation was bounded, R29 exposed an
earlier form of the same timing problem: survivor reprojection had a native root
and spawned roster but production had not yet published `PHYSICAL` on the next
real frame. Treating one frame or a changed service tick as proof could fail a
healthy batch or tempt debug code to duplicate production work.

Decision: Exact runtime-member liveness requires both controller-aware and stock
damage-state predicates to report alive. The single proof `Kill()` is allowed
only when damage handling is enabled. Death confirmation samples for at most
four observations, never issues a second kill, and never asks the adapter to
reconcile while the member remains alive.

Initial, casualty, and survivor `PHYSICAL` confirmation likewise sample the
production owner for at most four real frames. Success requires the exact
`PHYSICAL` and live operation state plus the expected native root, durable
roster, slot handles, adapter bindings, PhysicalWar registrations, and zero
legacy-support ownership. A changed production tick is latched only for
diagnostics. The proof never requeues or respawns a batch that production
already completed successfully.

Consequences:

- R29 `seed1985_t0_p1_u1784101849`, on the intermediate death-settle stamp,
  finished 687 cases at 576 PASS/50 WARN/54 FAIL/6 BLOCKED/1 SKIPPED and
  5,521/5,687 certification assertions proven, with 148 failed and 18 blocked.
  Its first six Phase-17 assertions passed, but the five casualty assertions
  failed before the kill at survivor re-entry `MATERIALIZING`/spawned/
  `PHYSICAL` = `1/1/0`; casualty settle remained `0/4`.
- Both R29 Phase-24 assertions passed with 16/16 runtime owners classified,
  orders/open/terminal/invalid `1/1/0/0`, one projection with V/M/P/D
  `1/0/0/0`, and zero support
  leaks.
  All 29 state-diff lines, including 18 delta-bearing rows, remained zero and no
  mission state leaked. This makes R29 diagnostic evidence, not a casualty-
  continuity pass.
- The physical-settle source is
  `ad54861f639b020627492f122f39f7a6cbc5a929`, UTC
  `2026-07-15T08:26:21Z`, label
  `schema70-settings24-native-counterattack-physical-settle`, with stamp commit
  `c709839`. Campaign Schema 70 and settings Schema 24 remain unchanged.
- Foundation passes at 808 script-symbol references. Final stamped Workbench
  Game Module validation loads 5,830 Game files/11,822 classes, uses 47,077K
  static storage, records CRC `c79d806b`, reports `Script validation
  successful`, and exits `0` with zero script, HST, or hard-failure signals.
  Exact cleanup records session/owned-process/default-log/spill counts of
  `0/0/0/0`.
- R30 `seed1985_t0_p1_u1784110353` completed 687 cases at 576 PASS/50 WARN/53
  FAIL/7 BLOCKED/1 SKIPPED and proved 5,510/5,672 required assertions, with 144
  failed and 18 blocked. The first six Phase-17 assertions passed; the five
  casualty assertions failed before the kill because an artificial one-second
  post-handoff yield let one still-bound and registered member become nonliving
  before the proof observed it. Phase-24 runtime-owner classification passed,
  exact authority was skipped after focal cleanup, all 18 tracked state deltas
  were zero, no HST script or crash marker appeared, and guarded external
  cleanup was exact.

## CRI-037 - Preserve Production Ordering After Native Handoff

- Status: Accepted; source, stamped Workbench, and guarded R31 runtime proof pass;
  whole suite not certified
- Date: 2026-07-15

Context: R30 proved that the adapter handoff, durable registrations, and
production `PHYSICAL` publication were complete. The debug runner nevertheless
returned for an artificial one-second yield before its controlled fold or kill.
One still-bound and registered member became nonliving during that gap, so the
next observation correctly rejected the roster before the proof could issue its
intended casualty. Increasing the observation limit would not restore the
production call ordering and would misclassify a stable nonliving binding as a
transient incomplete handoff.

Decision: After a successful scoped native handoff, keep handoff -> `PHYSICAL`
confirmation -> immediately controlled fold or kill in the same coordinator
invocation. Bound the synchronous transition loop so an unexpected stage cannot
spin indefinitely. Preserve real-frame yields for asynchronous death
observation, corpse cleanup, and later re-entry; do not requeue, respawn, or
re-kill completed production work.

The persistence binding validator now emits separate failures for an incomplete
handed-off adapter binding and for a complete handed-off binding whose entity is
nonliving. This is diagnostic classification only: both remain invalid exact
living authority, and no persisted ownership rule is weakened.

Consequences:

- Campaign Schema 70 and runtime-settings Schema 24 remain current. This change
  adds no serialized field, enum ordinal, operation contract, normalization, or
  migration rule.
- Current source is `393733cc165b96ec494c72f96741cf993d400ebd`, UTC
  `2026-07-15T10:46:59Z`, label
  `schema70-settings24-native-counterattack-proof-ordering`, with stamp commit
  `a8210af`.
- The final stamped tree passes Foundation at 808 script-symbol references.
  Workbench Game Module validation loads 5,830 Game files and 11,822 classes
  with 47,077K static storage at CRC `b789ee05`, reports `Script validation
  successful`, exits `0`, records zero script, HST, or hard-failure signals,
  and leaves exact session/owned-process/default-log/spill cleanup.
- R31 `seed1985_t0_p1_u1784117364`, on build
  `393733cc165b96ec494c72f96741cf993d400ebd` and label
  `schema70-settings24-native-counterattack-proof-ordering`, finished 687 cases
  at 577 PASS/50 WARN/52 FAIL/7 BLOCKED/1 SKIPPED. It proved 5,528/5,685
  required certification assertions, with 139 failed, 18 blocked, zero
  certification warnings, and a false overall certification result.
- All 11 Phase-17 projection/casualty assertions and both Phase-24 owner
  assertions passed. The casualty boundary remained N=9 -> N-1=8; initial,
  casualty, and survivor physical settlement each completed at observation
  `1/4`, and casualty death completed at observation `1/4`. Phase 24 classified
  16/16 sampled owners with two open exact projections.
- R31's 29-line state diff contained 18 delta-bearing rows, all zero. It
  recorded zero script, HST, or crash errors and exact external cleanup. This
  runtime-proves the same-invocation handoff -> `PHYSICAL` -> controlled-action
  decision while preserving the asynchronous yields.
- The whole suite remains uncertified. Packaged, dedicated-server, multiplayer,
  reconnect/JIP, and soak results are not claimed here.

## CRI-038 - Keep Staged Debug Fixture Ownership Explicit

- Status: Accepted; source, Foundation, and stamped Workbench complete; full
  runtime pending
- Date: 2026-07-15

Context: The R30 and R31 post-case cleanup audit reported the exact spawn-
adapter partial-cancel start fixture as an orphan. That case intentionally
retains its active group, frozen manifest, and queued batch for the later proof
steps, while the final step owns cleanup and state isolation. The generic audit
predates this multi-step proof and recognized only production owners. Deleting
the rows after the start case would destroy the proof; suppressing the audit for
the staged cases would hide real leaks.

Decision: Keep every post-case cleanup probe enabled. Recognize a staged debug
owner only while its proof service is active and only when one fail-closed
predicate proves the unique expected fixture group, its cancel/success/failure
projection family, reciprocal group/manifest/batch spawn-result, request,
operation, force, and projection identities, and a frozen manifest whose
recomputed hash matches both the stored manifest and batch hash. When the proof
service finishes, this allowance ends and final state isolation must return the
staged rows to baseline.

Consequences:

- Arbitrary spawn-adapter rows, partial matches, and post-finish residue remain
  orphans; the decision does not weaken production leak detection.
- Current source is `6a7943a37bd9338f176724718ec132ff108e9c82`, UTC
  `2026-07-15T12:54:09Z`, label
  `schema70-settings24-spawn-adapter-proof-backing`, with stamp commit `40c2d48`.
  Foundation passes at 808 script-symbol references. Stamped PC Game-module
  Workbench validation resolves the exact project/GUID pair, loads 5,830 Game
  files and 11,822 classes at CRC `dc565606`, creates the game, reports `Script
  validation successful`, exits `0`, and records zero hard errors. Cleanup
  guard, surviving-process, default-residue, and external-spill counts are
  `0/0/0/0`. The full runtime rerun remains pending.
- Campaign Schema 70 and runtime-settings Schema 24 remain unchanged. No
  serialized field, enum ordinal, operation contract, or migration is added.
- R30 and R31 remain historical evidence on the preceding proof-ordering
  source; a fresh full run is required to validate the new staged-owner
  classification.

## CRI-039 - Prove Virtual Counterattack Restart Without Borrowing Live Proximity

- Status: Accepted; source, Foundation, stamped Workbench, and guarded
  three-process virtual-restart proof complete
- Date: 2026-07-15

Context: The exact-counterattack graph already retained its reciprocal order,
operation, manifest, spawn batch, slot, active-group, pool-debit, route, and
survivor authority while virtual. In-process capture/restore and native
projection cases did not prove that one stopped executable could persist that
graph, a fresh executable could resume it once, and another fresh executable
could replay the result without advancing it again. A living player's normal
proximity context can also ask production to materialize a healthy group. That
behavior is correct for gameplay but would turn a virtual-restart proof into an
unbounded physicalization test and make its source state environment-dependent.

Decision: Admit this destructive proof only through an exact one-use CLI lease.
The lease binds the guard, run, stage, build, world, and schema identities,
fails closed on any mismatch, and may be consumed once by that fresh process.
`prepare`, `recover`, and `replay` therefore execute in three separately owned
engine processes against an exact disposable canonical-fallback carrier.

Ordinary gameplay continues to instantiate
`HST_EnemyCounterattackOperationService`. Only the exactly authorized proof
instantiates `HST_EnemyCounterattackOperationProofHarness`, which subclasses
that production service and retains its counterattack tick, route, persistence,
and settlement logic. The harness replaces only the player-proximity and
materialization decision with a deterministic no-nearby-player result. It does
not become another gameplay owner or certify the physical branch.

Each stage validates the complete reciprocal graph through one semantic
fingerprint and exact persisted readback. `prepare` writes the admitted virtual
source. A fresh `recover` process requires that exact source, advances the
production virtual operation once, and reads back the expected continuation. A
fresh `replay` process requires the recovered fingerprint and must preserve it
as a semantic no-op. Both runtime adapters and PhysicalWar must have zero
claimants throughout this virtual proof.

The launcher owns every stage process and its sentinel-scoped profile, result,
log, and temporary artifacts. Process containment, exact result identity,
readback, exit status, fingerprint continuity, and cleanup are all mandatory
success gates. Any surviving launched process or owned artifact fails the run;
diagnostic evidence remains bounded and the disposable tree is removed.

Consequences:

- Source `339b72ec3ed63132e46f3df84540d74d3e938d16`, UTC
  `2026-07-15T22:32:02Z`, label
  `schema70-settings24-counterattack-virtual-restart-proof`, is stamped by
  `7736b42`. Campaign Schema 70 and runtime-settings Schema 24 remain current;
  this slice adds no serialized field, persisted enum ordinal, operation
  contract version, normalization, or migration rule.
- Foundation passes at 814 script-symbol references. Stamped Workbench
  validation loads 5,832 Game files and 11,828 classes, records CRC `92fcd4a4`,
  and completes without a script-validation error.
- The guarded external run passes `prepare`, `recover`, and `replay` with exact
  source/readback identity. Recovery advances the expected virtual continuation
  once; replay preserves the recovered semantic fingerprint. All three stages
  exit `0`, the fingerprint chain is exact, runtime claimant counts remain
  zero, and every process and cleanup counter finishes at zero.
- This decision certifies only the canonical-fallback outbound-virtual
  counterattack graph and its first deterministic continuation/no-op replay.
  It does not certify live player-proximity decisions, materialization,
  dematerialization, physical combat, arrival or capture settlement, prepared
  settlement crash cuts, native persistence-source selection or precedence,
  migration, package delivery, live dedicated server/client behavior,
  multiplayer, reconnect/JIP, performance, or soak.

## CRI-040 - Preserve Raw Dematerialization Authority Across Restart

- Status: Accepted; source, Foundation, stamped Workbench, and guarded
  three-process proof complete
- Date: 2026-07-15

Context: The outbound-virtual restart proof did not cross the boundary where a
counterattack had already handed off a physical roster, recorded a casualty,
published its live position, and begun dematerializing. Normalizing that state
inside `prepare` would prove only a prepared virtual clone, not that a fresh
process could consume the actual interrupted authority. Restoring only the
operation position while leaving the active group's source position stale could
also restart later strategic movement from the wrong point.

Decision: Add one guarded `dematerializing_before_hold` cut. Establish it through
the production operation and queue transitions, stop at
`DEMATERIALIZING`/`LIVE` before strategic hold, and persist that raw state. Bind
the carrier to separate raw and normalized semantic fingerprints. `prepare`
must validate normalized readback and then re-track the raw fingerprint; a fresh
`recover` process owns the actual normalization.

Current-schema restore converts the valid live aggregate to held
`VIRTUAL`/strategic authority exactly once. It clears process-local topology,
increments the successful batch's reprojection count once, preserves exactly
one retired casualty tombstone and its N-1 living-slot fingerprint, and aligns
both active-group current and source positions with the operation's strategic
position. Recovery must then advance exactly 75 meters; replay must preserve the
recovered semantic fingerprint without another business-state change.

Consequences:

- Source `87a4ae2491ec5b83d37dbc43e1658f3380bb8b1c`, UTC
  `2026-07-16T01:06:29Z`, label
  `schema70-settings24-counterattack-dematerializing-restart-proof`, is stamped
  by `3a15f77`. Campaign Schema 70 and runtime-settings Schema 24 remain current.
  This adds no serialized field, enum ordinal, operation contract version, or
  legacy migration; it tightens valid current-schema restore normalization.
- Foundation passes at 816 script-symbol references. Stamped Workbench
  validation loads 5,832 Game files and 11,830 classes at CRC `699fab13`, reports
  zero hard errors, and finishes with every cleanup counter at zero.
- Both outbound-virtual and dematerializing-before-hold cuts pass `prepare`,
  `recover`, and `replay` in fresh processes with exact readback, fingerprint
  chaining, zero projection-scoped runtime claimants, zero exits, and zero
  process/profile/log/guard/spill residue.
- The physical handoff in this cut is controlled proof state. This decision does
  not certify live player proximity, native spawning or combat, an interrupted
  `MATERIALIZING` or `PHYSICAL` cut, every dematerializing interruption,
  prepared settlement, native persistence selection, package/live server-client
  behavior, migration, multiplayer/JIP/reconnect, performance, or soak.

## CRI-041 - Keep Materializing Counterattacks Behind The Last Safe Checkpoint

- Status: Accepted; source, Foundation, stamped Workbench, and guarded
  three-process checkpoint-deferral proof complete
- Date: 2026-07-15

Context: Production persistence intentionally refuses to capture any open exact-
infantry operation while its authority is `MATERIALIZING`. That interval has
released strategic hold but has not established the complete native root,
member, adapter, and PhysicalWar topology needed to make a physical checkpoint
trustworthy. Persisting a handcrafted raw `MATERIALIZING` row would bypass this
fail-closed contract and prove a save shape production is designed not to
publish.

Decision: The guarded `materializing_checkpoint_deferred` cut must preserve the
last safe canonical `VIRTUAL` checkpoint. `prepare` writes that baseline, makes
a migration-free in-memory copy, and enters `MATERIALIZING`/strategic authority
on only the copy through the production materialization transition. A
production `CaptureAndTrackState` attempt must return no save, publish the exact
materialization-in-progress deferral, and leave the raw in-memory graph
unchanged. An independent canonical reread must still match the original
`VIRTUAL` fingerprint and have zero adapter or PhysicalWar claimants.

The carrier binds the raw interruption fingerprint separately from the durable
baseline fingerprint. A fresh `recover` process never consumes the raw graph;
it loads the retained `VIRTUAL` baseline and advances production strategic
movement exactly once. A fresh `replay` process must preserve that recovered
fingerprint as a semantic no-op.

Consequences:

- Source `d97c0e03a222d8681e6a47d5e01a593324564e05`, UTC
  `2026-07-16T02:24:44Z`, label
  `schema70-settings24-counterattack-materializing-deferred-restart-proof`, is
  stamped by `6f4b2893b7abda9064b1f6fcba561e3e06847e68`. Campaign Schema 70
  and runtime-settings Schema 24 remain current; no serialized field, enum
  ordinal, operation contract, normalization, or migration rule is added.
- Foundation passes at 816 script-symbol references. Stamped Workbench
  validation loads 5,832 Game files and 11,830 classes at CRC `f7307712`,
  reports successful validation, exits `0`, and finishes with every cleanup
  counter at zero.
- The canonical baseline remains `793c4b001ef2751d`. Recovery advances exactly
  75 meters to `39d0ff3942d3445c`, replay preserves that digest, and the final
  independent process, profile, log, temporary, guard, spill, and mutex census
  is zero.
- CRI-039 outbound-virtual and CRI-040 dematerializing-before-hold evidence
  remain valid. CRI-040 uses controlled synthetic physical authority and does
  not certify the native binding/live-position persistence preflight required
  for a genuine `PHYSICAL` checkpoint.
- The next counterattack restart slice is genuine `PHYSICAL` persistence with
  exact live bindings and authoritative live position. `PREPARED` settlement
  prefix recovery follows that slice. Package/live server-client behavior,
  migration, multiplayer/JIP/reconnect, performance, and soak remain open.

## CRI-042 - Defer Dematerialization And Capture Physical Enemy Responses Atomically

- Status: Accepted; implementation, Foundation, stamped Workbench, and stamped
  four-cut fresh-process proof complete
- Date: 2026-07-16

Context: CRI-040 established a useful interrupted `DEMATERIALIZING` graph with
one confirmed casualty, but persisting that raw transition let a controlled
proof shape bypass the complete native-binding and live-position preflight
required of production physical authority. Physical exact-enemy-response
capture also updated family state after shared reconciliation. Without one
read-only preflight and rollback plan for every physical graph, a later family
failure could leave earlier durable positions partially refreshed.

Decision: Open exact defensive-QRF, counterattack, or garrison-rebuild authority
in `DEMATERIALIZING` is not a capturable state. Production persistence must
return no save before mutation, identify the blocking operation, and leave the
last canonical `VIRTUAL` fallback unchanged. CRI-040 remains historical evidence
for constructing and validating the raw N-1 graph, but this decision supersedes
only its instruction to persist that raw dematerializing disposition. The
materializing boundary remains the same fail-closed retained-fallback contract
established by CRI-041.

For an open `PHYSICAL`/`LIVE` exact enemy response, persistence first resolves
the family-owned reciprocal order, operation, successful non-held batch, and
active group without mutation. It requires a positive durable living roster,
exact root/member adapter and PhysicalWar bindings, and an authoritative native
live position. It builds plans for every matching physical graph and completes
the remaining persistence preflights before applying any sampled position. The
commit applies the live position to the group, invokes the owning QRF,
counterattack, or rebuild service to refresh its operation, and rolls back every
already-applied group position, operation position, revision, and progress
timestamp if any family refresh fails. An isolated capture failure must return
no captured save rather than exposing a previously tracked snapshot.

Add `physical_live_position` as the fourth guarded counterattack restart cut.
Its prepare stage uses the production counterattack owner and scoped projection
worker across real frames. Acceptance requires `PHYSICAL`/`LIVE` authority, one
registered root plus one member per durable living slot, N+1 adapter handles,
and N PhysicalWar runtime members. The cut samples the native centroid,
deliberately makes both durable positions stale, and requires production
persistence to refresh both. Persisted readback must be held
`VIRTUAL`/`STRATEGIC` authority with a pending batch, cleared runtime bindings,
and exactly one reprojection. A fresh recovery advances 75 m, replay is a
semantic no-op, and bounded cleanup removes the focal projection and result
handles, runtime members, and root.

Consequences:

- Source `2d4c76f9b08c6a2d0acaeb6dcafc077841fe3fd8`, UTC
  `2026-07-16T04:46:10Z`, label
  `schema70-settings24-counterattack-physical-live-restart-proof`, is stamped by
  `b035c5b1f9fcab083f5434e713de8541d828a979`. Campaign Schema 70 and runtime-
  settings Schema 24 remain current; no serialized field, enum ordinal,
  operation contract, legacy migration, or compatibility inference is added.
- Foundation passes. Stamped Workbench validation loads 5,832 Game files and
  11,834 classes at CRC `f732e575`, records zero hard errors, and leaves all
  cleanup counters at zero.
- The stamped four-cut fresh-process matrix passes every prepare/recover/replay
  chain. Its digests match the pre-stamp reference values, every stage exits
  `0`, and every cleanup counter is zero.
- Defensive QRF and garrison rebuild share the static physical persistence
  preflight, live-position refresh, rollback, and normalization contract. Only
  the exact counterattack has the genuine scoped native runtime restart proof in
  this checkpoint.
- The next counterattack restart slice is `PREPARED` settlement recovery.
  Native persistence-source selection beyond the guarded profile seam, world
  scope, package/live server-client behavior, migration, markers/rendered UI,
  multiplayer/JIP/reconnect, performance, soak, and wider Campaign Debug
  failures remain open.

## CRI-043 - Prove Counterattack Prepared Settlement Prefixes In Fresh Processes

- Status: Accepted; implementation, Foundation, stamped Workbench, and final
  stamped seven-cut validation complete
- Date: 2026-07-16

Context: The in-memory counterattack proof already covered restoration before a
refund, after a refund, and after its durable receipt. The four-cut external
harness proved route and materialization persistence, but it could not show that
the production startup reconciler consumes those three durable `PREPARED`
transaction prefixes exactly once across real process boundaries. Reusing the
movement fingerprint would also hide the refund mutation and assume that the
batch/group still existed after terminal cleanup.

Decision: Extend the guarded counterattack harness with
`prepared_before_refund`, `prepared_after_refund`, and
`prepared_after_receipt`. Every cut uses a nondegenerate N-1 survivor roster,
one charged resource pool, `route_failed_survivors`, and a proportional refund.
The prefixes respectively require zero refund mutations with no receipt; one
refund mutation with no receipt; and one refund mutation with the receipt while
the operation remains `PREPARED`. The carrier binds the complete aggregate,
settlement IDs, expected pool/revision tail, prefix mutation/receipt policy, and
prepared fingerprint.

The first restored startup must validate the prefix before ordinary
reconciliation, terminalize it exactly once through production, retain exactly
one original debit and one refund, settle the operation, abort the terminal
order, and remove the reciprocal batch/group claimants. A second reconciliation
in that process and the next fresh startup must both be semantic no-ops. The
result artifact remains the final durable write, and all existing owner, nonce,
one-use stage lease, process containment, spill, profile, and cleanup gates stay
mandatory.

The final source also makes movement and settlement carrier shapes an exact
exclusive choice. A movement carrier must own only its movement expectation; a
settlement carrier must own only its settlement expectation. The launcher now
forges a mixed-family settlement carrier as a negative self-test and requires
the carrier assertion to reject it.

Consequences:

- Implementation/source `02f64410670a3ffced10c8e099c05eaf5a469cb0`, UTC
  `2026-07-16T12:17:23Z`, label
  `schema70-settings24-counterattack-prepared-settlement-restart-proof`, is
  represented by stamp commit `8d538064a4ec049a34172bd688f8bb992c9312dc`.
  Campaign Schema 70 and runtime-settings Schema 24 remain unchanged.
- Final stamped Foundation passes at 819 symbols. Workbench loads 5,832 Game
  files/11,835 classes at CRC `b02931ee`, exits `0`, reports
  `ScriptValidation true` with zero errors, and cleans exactly.
- All seven guarded cuts pass every prepare/recover/replay stage on build
  `02f64410670a`, 21 stages total, at exit `0` with exact fingerprint continuity
  and zero residue.
- The three new digest chains are `1af36d0feaa72444 -> c7226f77c1e25550 ->
  same`, `be4db517916fa4dc -> 70f25322893d791b -> same`, and
  `a82efe52307d55f6 -> 18e03304bf1022f1 -> same`. The four preceding chains are
  unchanged.
- An independent final census finds zero engine processes, zero Workbench guard
  roots, zero restart guard roots, and both proof mutexes free.
- The next implementation slice should bind live source/target owner revisions
  and operation-owned ownership-transition claimant absence before widening the
  counterattack fingerprint claim. Native persistence-source selection, world
  scope, package/live server-client, migration, markers, multiplayer/JIP/
  reconnect, performance, soak, and wider Campaign Debug failures remain open.

## CRI-044 - Bind Counterattack Restart Proof To Endpoint And Claimant Authority

- Status: Accepted; implementation, Foundation, stamped Workbench, and final
  stamped seven-cut validation complete
- Date: 2026-07-16

Context: CRI-043 proved fresh-process movement and settlement recovery, but its
carrier fingerprints did not freeze the live source/target ownership rows or
detect an ownership transition correlated to the counterattack operation. A
route or settlement could therefore remain fingerprint-stable while endpoint
owner/revision authority changed or a request-ID/source-ID claimant appeared.

Decision: Extend both movement and settlement carrier families with expected
source and target owner faction plus ownership revision. Every prepare, recover,
and replay validator must resolve exactly one row for each endpoint, require the
frozen owner/revision values, and require exactly zero ownership-transition rows
correlated either by request ID `ownership_counterattack_<operationId>` or by
operation source ID. Endpoint semantic rows and the claimant count belong in
the fingerprints and no-op comparisons.

The tamper proof must increment the source ownership revision and inject each
claimant identity separately. All three mutations must be rejected through the
complete movement or settlement validator, after which the restored graph must
again pass exactly. The genuine physical cut inherits the movement-family
negative proof and must also pass positive endpoint validation on its native
graph. This is proof-only hardening: it changes no persisted schema, operation
contract, production ownership lifecycle, or migration rule.

Consequences:

- Implementation/source `008cd481d5e55b43c7afc902cd5e906cbb297415`, UTC
  `2026-07-16T13:07:11Z`, label
  `schema70-settings24-counterattack-endpoint-owner-claimant-restart-proof`, is
  represented by stamp commit `776523b75c3c98ececb8405f411d6af6b64370a3`.
  Campaign Schema 70 and runtime-settings Schema 24 remain unchanged.
- Final stamped Foundation passes at 819 symbols. Workbench loads 5,832 Game
  files/11,835 classes at CRC `3131538f`, exits `0`, reports
  `ScriptValidation true` with zero errors, and cleans exactly.
- All seven guarded prepare/recover/replay chains run build `008cd481d5e5`, 21
  stages total, at exit `0` with exact fingerprints and cleanup. The digest
  chains are `d9b7bed7d685d793 -> 05745f6799ed4196 -> same`,
  `34fd3ec48963459e -> 3d105daaf7159131 -> same`,
  `0259acbbd2ea35c9 -> 809d6ccf51a7e393 -> same`,
  `711a8081db90d496 -> 9012a0975998263f -> same`,
  `948803021dbbf846 -> 326a49119bc4b0a7 -> same`,
  `e67861d2f8bdf456 -> 126ef467ff7c1abe -> same`, and
  `c461df2b3cdfe0a2 -> 940b360563a6ead1 -> same`.
- The independent census finds zero engine processes, zero Workbench and restart
  guard roots, and both proof mutexes free.
- The next concrete item is a lifecycle-aware pre-reconcile decision and
  implementation for canonical correlation, resume/terminal handling, and
  quarantine of an orphan or pending counterattack-owned ownership transition.
  This checkpoint proves absence and tamper rejection; it does not fix that
  production case. Durable endpoint ABA snapshots remain a future contract-2
  schema decision whose schema number is not yet assigned.

## CRI-045 - Fence Counterattack Ownership Before Runtime Reconciliation

- Status: Accepted; implementation and focused engine proof complete; owner-
  applied restart closure recorded by CRI-046
- Date: 2026-07-16

Context: CRI-044 proved that the seven restart carriers reject any correlated
ownership claimant, but production restore still normalized generic ownership
rows before counterattack aggregates and then ran runtime ownership
reconciliation before counterattack-specific reconciliation. A structurally
valid but orphaned, premature, duplicate, or lifecycle-illegal
`enemy_counterattack` transition could therefore publish ownership before its
aggregate was checked.

Decision: After generic ownership normalization and before runtime ownership
reconciliation, correlate every counterattack claimant by canonical request ID
`ownership_counterattack_<operationId>` OR exact operation source ID, counting
each row once. Reverse orphan declaration also recognizes the counterattack
source type, request prefix, or a source ID naming an exact counterattack
operation. Require the full canonical receipt fingerprint and one reciprocal
exact order/operation aggregate.

Uncommitted and pre-recapture state allows zero claimants. An active/open,
clear-engagement, on-station operation may retain zero or one exact pending or
completed receipt only for `STRATEGIC` with `VIRTUAL`/`MATERIALIZING`, or `LIVE`
with `PHYSICAL`/`DEMATERIALIZING`. Returning, applied recapture outcome, or a
settlement that requires recapture must own exactly one completed immutable
receipt. A completed receipt remains historical after a later ordinary zone
recapture; incomplete authority does not.

Premature, engaged, foreign-fingerprint, duplicate, missing-required, orphaned,
or malformed authority must fail closed before runtime reconciliation. Reuse
the canonical ownership validator to quarantine the transition, reciprocal
zone, and campaign marker while preserving the first durable failure and
converging backlinks. Quarantine the exact counterattack order at `-69` and hold
the operation, manifest, batch, group, and retained evidence. Do not choose an
ambiguous row, apply capture, refund, settle, cancel, delete, or invent an
outcome. Pre-69 incomplete declared counterattack rows receive the same fence
before historical preservation.

Consequences:

- Implementation/source `541a79f7e5f49394c6f78a630d9e05340c8e2959`, UTC
  `2026-07-16T15:31:05Z`, label
  `schema70-settings24-counterattack-ownership-pre-reconcile-fence`, is stamped
  by `f220b5aa183760f6bc6f20974d4cdb3a4a04dd3f`. Campaign Schema 70 and runtime-
  settings Schema 24 remain unchanged.
- Final stamped Foundation passes 819. Workbench passes 5,832/11,835 at CRC
  `61930e5a` with zero hard errors. The exact focused JUnit case passes 1/1 with
  zero failure, error, or skip, one empty failed-list artifact, stamped identity
  plus required lifecycle-correlation evidence, and every process/profile/temp/
  spill counter zero.
- Campaign Debug exposes `enemy_counterattack.ownership_correlation` separately
  from the aggregate. This checkpoint does not claim a new broader Full Campaign
  Debug execution.
- CRI-046 closes the guarded owner-applied-incomplete fresh-process gate. Native
  persistence-source selection, package/live server-client execution,
  networking/JIP, rendered markers, performance, and soak remain open. Durable
  endpoint ABA remains a future contract-2 schema decision whose schema number
  is not yet assigned.

## CRI-046 - Prove Owner-Applied Counterattack Recovery Across Fresh Processes

- Status: Accepted; implementation, final stamped Workbench, and eight-cut
  matrix complete
- Date: 2026-07-16

Context: CRI-045 fenced lifecycle-invalid counterattack ownership before runtime
reconciliation, but its focused proof did not cross a real process boundary with
a legal owner-applied incomplete receipt.

Decision: Add `owner_applied_pending` as the eighth guarded counterattack restart
cut. Prepare persists the raw legal receipt and proves that restore derives its
normalized pending form without applying ownership. Recover observes that
normalized source before ownership startup reconciliation, requires the receipt
to complete exactly once without a second owner revision or strategic effect,
then permits exactly one production counterattack tick to enter raw `RETURNING`;
persisted readback must be normalized `RETURNING`.

Replay must not reapply ownership or advance the operation. It must deny
canonical fallback overwrite and preserve the save's SHA-256, byte length, and
last-write UTC identity. Ownership startup mutation must be
`false -> true -> false` across prepare, recover, and replay. The preceding seven
cuts must remain regression-clean and must never report an ownership startup
mutation.

Consequences:

- Implementation/source `7eb0a98977c523f6713a9e2088eab7ba20a333fd`, UTC
  `2026-07-16T17:12:17Z`, label
  `schema70-settings24-counterattack-owner-applied-restart`, is stamped by
  `8947b2668655fcb58d8339c8b3f77541c39661bc`. Campaign Schema 70 and runtime-
  settings Schema 24 remain unchanged.
- Pre-stamp evidence passes the owner cut 3/3 with the required
  `false -> true -> false` ownership sequence, and the preceding seven cuts pass
  21/21.
- Final stamped Foundation passes 819. Workbench passes 5,832/11,835 at CRC
  `417e9910` with script validation true and zero hard errors. All eight guarded
  chains pass 24/24 fresh-process stages with exact fingerprints, zero exits,
  and every cleanup/watch/spill counter zero; the independent census also finds
  zero engine processes, guard roots, and held proof mutexes.
- This closes only the scoped guarded fallback-carrier restart cut. Native
  persistence-source selection, world/package/live-server behavior,
  networking/JIP/reconnect, migration, markers, performance, soak, and durable
  endpoint ABA remain open.

## CRI-047 - Select Native Campaign Persistence Before Profile Fallback

- Status: Accepted; implementation, stamped Workbench, and guarded native-over-
  fallback restart proof complete
- Date: 2026-07-16

Context: CRI-046 proved exact recovery through the canonical JSON fallback, but
that seam could not prove that an engine session save contained the campaign or
that native state won when native and fallback sources disagreed. Treating the
campaign DTO itself as engine-owned state would also violate the persistence
system's construction and serializer contract.

Decision: Keep `HST_CampaignSaveData` as the manually constructed serializable
DTO. Register one engine-created `HST_CampaignPersistentState : PersistentState`
proxy and a scripted, versioned envelope serializer; track, configure, and save
that proxy while its snapshot carries the DTO and deterministic fingerprint.

Resolve startup authority only after native persistence is `ACTIVE`, using a
native-first, fail-closed policy. Select a valid native row before consulting
the profile fallback. Invalid native data, an unreadable present fallback,
terminal system state, missing proxy, or failed tracking is fatal. A loaded
engine session with no HST native row and no valid migration fallback is also
fatal, never a fresh campaign. Valid legacy profile data may still migrate into
the canonical fallback. At this checkpoint, ordinary checkpoints mirrored
current data there for bounded backward recovery; CRI-048 supersedes that write
ordering for native-active checkpoints.

Defer the complete restore/reconciliation pipeline while the persistence system
is initializing, with a 120-second frame-driven limit. Do not publish gameplay
callbacks or capture the selected state until source validation and all startup
reconcilers succeed. Both mission headers must select the HST persistence
`SystemsConfig` and set `m_eSaveTypes 15`.

The guarded proof must request blocking manual savepoints, correlate both the
`OnSaveCreated` UUID and completion callback, and pass the exact UUID to each
fresh recovery process. Recover and replay must report source `native`, reject
the intentionally conflicting profile fallback, and preserve that fallback's
file signature. The launcher must pack the current project into a nonce-owned
disposable add-on root, run the dedicated server from the packed base project
and exact config mod entry, and leave no loose-project server argument. Profile,
temporary, pack, server, and process artifacts remain guard-owned. Any required
workspace scratch is separately sentinel-owned and may be removed only after
exact owner revalidation; zero scratch and process residue is a release gate.

Consequences:

- Implementation/source `a6e9069f29f8b844f8545b77b8894170ecd6d3b8`, UTC
  `2026-07-16T20:53:27Z`, label
  `schema70-settings24-native-persistence-source-selection`, is stamped by
  `35fc01a399f4f688f28f4ef7afee6351fb6289b7`. Campaign Schema 70 and runtime-
  settings Schema 24 remain unchanged.
- Final Foundation passes 828. Workbench validation passes 5,834 files and
  11,839 classes at CRC
  `5fdd016f`, with zero hard/script errors and zero cleanup residue.
- The final packed prepare/recover/replay chain runs build `a6e9069f29f8` and
  exits `0` at every stage. It proves `new_campaign -> native -> native`, one
  recovery continuation, replay no-op with no new save point, an unchanged
  conflicting fallback, exact fingerprint continuity, and zero guard, pack-
  scratch, process, watched-root, or spill-root residue.
- This decision certifies the scoped native source-selection and guarded
  fresh-process precedence boundary. It does not close general packaged
  server/client, networking/JIP/reconnect, migration breadth, markers,
  performance, soak, or durable endpoint ABA work.

## CRI-048 - Gate Controlled Campaign End On A Completed Typed Checkpoint

- Status: Accepted; implementation and final stamped five-process proof complete
- Date: 2026-07-16

Context: CRI-047 established native-first startup authority and exact save-point
correlation, but it did not make ordinary automatic/manual checkpoints or the
real game-mode shutdown path one durable production contract. Mirroring a newer
profile fallback before a native commit could also let a failed native request
leave two recovery sources at different logical checkpoints. Stock game-mode
end may purge native session state when retention is disabled, so a successful
save callback alone was not sufficient shutdown evidence.

Decision: Give every production checkpoint one explicit type: `AUTO`, `MANUAL`,
or `SHUTDOWN`. When native persistence is active, stage one exact snapshot,
request the native save point, and wait for its post-commit `SaveGameManager`
completion callback before writing the same snapshot to the profile fallback.
A failed native commit must leave the previous native state and matching
previous fallback intact. When native persistence is unavailable, complete the
typed request synchronously through the profile fallback.

Override the production `SCR_BaseGameMode.EndGameMode` boundary for controlled
shutdown. Disable controls and new campaign commands, allow one normal
coordinator interval to drain already-admitted work, request a `BLOCKING`
`SHUTDOWN` checkpoint, and then quiesce campaign mutation. Wait for the bounded
post-commit completion callback, recompute the stability fingerprint, and enter
the stock transition only if the checkpoint remains exact. Timeout, callback
failure, or a changed fingerprint fails closed. Production does not depend on
proof-only `OnAfterSave`/`OnSaveCreated` event correlation or transition
polling.

Make retention authority explicit in the stock save-data hook. After a verified
native-authority shutdown commit, skip the stock native purge even when server
configuration disables session retention and the CLI retention flag is absent.
After a fallback-only shutdown, clear and purge stale native state so an older
native row cannot hide the newer fallback at restart. This bridge cannot delay
an external process kill; that failure mode retains only the last checkpoint
that completed before termination.

Consequences:

- Implementation/source `dceefed3eb3c8f9c93210d4d9b5dcd9510d549c1`, UTC
  `2026-07-16T23:52:22Z`, label
  `schema70-settings24-controlled-campaign-persistence`, leaves Campaign Schema
  70 and runtime-settings Schema 24 unchanged.
- The final stamped proof passes five fresh processes: the typed
  `AUTO` request seam, typed `MANUAL`, real bridged `SHUTDOWN`, native restart,
  and fallback restart on build `dceefed3eb3c`. All stages exit `0`; `AUTO` and
  `MANUAL` use flags `0`, `SHUTDOWN` uses exact `BLOCKING` flag `1`, native and
  fallback verification perform no save, persistence retention is disabled,
  the CLI retention flag is absent, and every cleanup counter is zero.
- Guarded `OnAfterSave`/`OnSaveCreated` correlation and transition polling prove
  the real game-mode bridge without becoming production dependencies. The
  `AUTO` stage proves the typed request seam, not autosave scheduler/debounce
  cadence.
- This decision supersedes CRI-047 only for checkpoint/fallback write ordering
  and controlled game-mode-end retention. CRI-047 remains the native-first
  startup and source-selection contract.
- Abrupt process termination, multiplayer/client behavior, migration breadth,
  performance, and soak remain outside this scoped proof.

## CRI-049 - Make Periodic Autosave Fair Against Major-Change Debounce

- Status: Accepted; implementation and final stamped five-process proof complete
- Date: 2026-07-16

Context: CRI-048 established the typed checkpoint and controlled-end durability
contract, but its AUTO proof called the request seam directly. It did not prove
that the production frame tick would reach periodic autosave while major-change
debounce was pending. Shared clock resets could allow repeated dirty marks or a
rejected major request to starve periodic AUTO, while an in-flight request could
also admit a competing scheduler request if the two lanes were not coordinated.

Decision: Advance periodic AUTO elapsed time and first-edge major-change debounce
as independent scheduler lanes through `HST_PersistenceService.Tick`. The first
dirty edge starts the major interval and later marks coalesce without extending
it. An accepted full-state checkpoint acknowledges both lanes. If major-change
and periodic work become due together, major-change keeps priority, but a
rejected major request must not rewind periodic elapsed time so AUTO remains due
on the next eligible tick. A rejected periodic request backs off by the
configured debounce interval. While a checkpoint is in flight, both clocks
continue advancing but competing scheduler requests remain suppressed.

Return a process-local scheduler receipt for every attempted request, including
origin, sequence, tick, configured intervals, elapsed clocks, and threshold
evidence. These receipts and clocks are diagnostics/control state; they are not
serialized campaign authority. A later mutation or failed completion re-arms
the applicable work after an accepted request has covered the previous dirty
generation.

During controlled game end, drain only an already-pending checkpoint. Do not
generate a new AUTO or SCRIPTED request while waiting to issue the exact
blocking SHUTDOWN checkpoint. Allow a 270-second controlled-end retry window so
the configured 120-second request bound and shutdown bound both fit with margin.

Consequences:

- Implementation/source `952a2d33245074867df6afad1ffe25ce49fc9a11`, UTC
  `2026-07-17T01:12:37Z`, label
  `schema70-settings24-periodic-autosave-scheduler`, leaves Campaign Schema 70
  and runtime-settings Schema 24 unchanged.
- The final stamped five-process proof reaches production origin
  `periodic_autosave` at tick 1800 and 60.020751953125 seconds. A repeated
  dirty mark at 30.020465850830082 seconds does not extend the configured
  120-second first-edge major debounce. Manual, real controlled-end shutdown,
  native restart, and profile-fallback restart also pass; request flags are
  `0/0/1`, every process exits `0`, and every cleanup counter is zero.
- The deterministic source harness covers rejected-major fairness, periodic
  backoff, simultaneous priority, late mutation, completion re-arm, and
  in-flight clock/suppression behavior. The packaged chain proves real periodic
  AUTO plus first-edge hold; it does not add a separate live SCRIPTED-at-
  debounce or rejection stage.
- Periodic scheduler/debounce is no longer an open certification item. Abrupt
  termination beyond the last completed checkpoint, broader active-world
  records, Workshop/live clients, network/JIP/reconnect, migration, markers,
  performance, soak, and unrelated Campaign Debug failures remain open.

## CRI-050 - Give Durable Field Vehicles One Persistence Authority

- Status: Accepted; implementation and strict five-process fresh-start proof
  complete
- Date: 2026-07-17

Context: CRI-049 proved production scheduler cadence and controlled checkpoint
durability, but broad active-world records remained open. Durable field vehicles
could still appear in both the HST campaign ledger and native entity persistence,
allowing duplicate restoration or tombstone resurrection. Restore also needed a
bounded exact bootstrap gate, while a moving physical root could prevent the
blocking shutdown save from completing.

Decision: Make the HST runtime-vehicle ledger the sole persistence authority for
supported `loot_vehicle`, `field_vehicle`, and `garage_redeploy` roots. Treat
each physical entity as a process-local projection. At initial Track and every
capture boundary, detach native entity persistence with
`PersistenceSystem.StopTracking(root, true)`. Reject a root owned by a different
tracked parent, a failed detach, or any native-tracked durable root remaining
after the pass.

Require unique stable runtime IDs, nonempty normalized full prefab identity with
an exact binding match (while allowing repeated prefab values), one living
binding per active row, no binding for deleted/detached rows, full 3D position
plus normalized upright yaw, and unique abstract-cargo keys. Capture
a destroyed unoccupied root as a tombstone and delete its wreck before save.
Refuse destructive cleanup while a living player occupies the vehicle.

Run durable field-vehicle restore before normal campaign capture or gameplay
publication. Bootstrap remains pending until the detailed restore receipt is
`AllExact`, with eligible/restored/tracked counts equal, logical and binding
graphs exact, inactive rows unbound, and failure/ambiguity counts zero; the
bounded bootstrap timeout remains fail-closed. Adopt only one unambiguous exact
root, otherwise spawn the saved full prefab at the saved transform and preserve
the stable ID.

Inactive tombstones do not authorize general proximity deletion. The only
legacy native cleanup is one unoccupied native-tracked candidate with exact
normalized full prefab identity within 3 meters and 3 degrees. Detach that root
with `StopTracking(true)` and verify the detach before deletion. Any ambiguity,
occupancy, parent tracking, or tolerance mismatch is fatal rather than guessed.

For a blocking controlled shutdown, leave each active durable root present but
stabilize it at the captured transform: shut down controller and engine state,
apply supported persistent brakes, clear forces and velocities, and make every
dynamic physics body in the hierarchy inactive. Maintain and revalidate that
state through the native commit. This quiescence is process-local and does not
create a second saved vehicle representation.

Consequences:

- Implementation/source `34fcb8e77726beb61dfb10cf650183b5ef99542c`,
  UTC `2026-07-17T04:33:16Z`, label
  `schema70-settings24-field-vehicle-restart`, leaves Campaign Schema 70 and
  runtime-settings Schema 24 unchanged.
- Foundation passes 839 references. The stamped Workbench compile passes 5,837
  files and 11,850 classes at CRC `37604e5a`, with zero script
  errors and zero residue.
- The strict five-process chain passes periodic `AUTO` at tick 1,802 and
  60.018852233886719 seconds, with the repeat dirty mark held at
  30.016357421875 seconds, then typed `MANUAL`, blocking `SHUTDOWN`, native
  no-save verification, and profile-fallback no-save verification. Every stage
  exits `0` and all owned cleanup counters are zero.
- The fixture begins with two S1203 durable rows and abstract cargo counts 3/7.
  Manual recovery spawns both, moves A, destroys B through engine damage, and
  captures one live row plus B's tombstone. Every later restore reports
  `adopted=0`, `retired-native=0`, exact spawned counts, and zero native-tracked
  roots. Shutdown keeps exactly one live root controller/physics-quiesced
  through commit. Native and fallback reproduce the same transform, tombstone,
  and cargo graph.
- This decision does not certify fuel, partial damage, attachments, physical
  trunk contents, or arbitrary vehicle breadth. The proof duplicate census is
  limited to expected fixture positions. Workshop/live clients, multiplayer/
  JIP/reconnect, performance, and soak remain open.
- At this checkpoint the profile fallback remained one directly overwritten
  JSON file. CRI-051 closes that limitation with a verified two-slot recovery
  journal while explicitly retaining the engine file API's non-atomic limits.

## CRI-051 - Give Native and Profile Campaign Saves One Monotonic Recovery Order

- Status: Accepted; implementation, focused authority proof, and scoped
  fresh-process restart proof complete
- Date: 2026-07-17

Context: Native persistence and the directly overwritten JSON fallback carried
the same campaign DTO but did not share a durable ordering identity. Native-first
selection could hide a later valid recovery snapshot, while writing one JSON
file in place could destroy the only script-readable recovery copy during an
interrupted or damaged write. A game/runtime change that made native authority
unavailable also needed an explicit degraded path rather than an inferred fresh
campaign. The engine file API exposes neither atomic rename nor an exclusive
writer lock, so the solution must preserve a known-good generation without
claiming filesystem atomicity.

Decision: Advance the persisted campaign contract to Schema 71 while keeping
runtime-settings Schema 24. Add `m_iPersistenceCheckpointSequence` to the
campaign DTO and advance it before every accepted full-state capture. Compare
native and profile snapshots by checkpoint sequence, then restore sequence, then
save second. Equal order is valid only when normalized full-payload fingerprints
match; disagreement is conflicting authority and fails closed. Sequence
exhaustion rejects capture. Administrative new-campaign reset retains the prior
checkpoint and restore order so its next capture is newer than every pre-reset
campaign save.

Replace the one-file fallback writer with a two-slot profile recovery journal.
The canonical and recovery slots carry a versioned envelope containing the exact
nested `HST_CampaignSaveData` payload, generation, parent generation, current
fingerprint, and parent fingerprint. Fingerprints use
`uuidv8-sha256-v1:<serialized-length>:<UUID>`, derived through the native
SHA-256-based UUID v8 generator. Treat that value as an accidental-corruption
integrity check, not authentication or proof against a malicious writer.

Write only the inactive slot. Read it back and rerun full selection before it can
supersede the previous valid slot. Select an exact same-generation duplicate
deterministically from canonical, but reject differing same-generation payload
or metadata, nonadjacent generations, broken parent identity, unsupported future
formats, and any other ambiguous history. Preserve those artifacts and fence
later journal writes. This is a crash-tolerant single-writer journal, not an
atomic rename transaction, an exclusive-lock protocol, or an off-device backup.

When native persistence is active, advance the journal from the same staged
snapshot only after the post-commit `SaveGameManager` completion callback
succeeds. A failed callback writes no journal generation and rearms checkpoint
retry. When native persistence is unavailable, or the session has explicitly
entered degraded profile-only recovery after unusable native authority, complete
the journal checkpoint synchronously.

Write new native rows with native envelope version 2, storing the exact
serialized payload string and fingerprinting those bytes. Validate the exact
fingerprint before parsing the DTO so a later added field cannot invalidate an
intact row through reserialization. Continue to read Schema-70 native envelope
version 1 by reconstructing and validating its legacy length/hash identity, then
normalizing the snapshot before source comparison. Preserve and reject unknown or future native envelopes or campaign
schemas before journal fallback is considered.

Treat a valid raw Schema-70-or-earlier canonical campaign JSON as journal
generation zero. On its first Schema-71 checkpoint, write recovery generation
one linked to that legacy authority and leave the raw canonical bytes intact.
During profile-tree migration, compare both campaign slots exactly. Retain and
fail on any difference between retired and canonical campaign authority instead
of choosing one or deleting the old tree.

At startup, select the newer valid native or journal snapshot by the Schema-71
durable order. A missing native row may recover from a valid journal. Explicitly
unusable native authority may select a journal in degraded profile-only mode for
the rest of the session. A valid native row may continue despite damaged or
future journal artifacts. Source selection preserves them; a later verified
native checkpoint may repair ordinary invalid inactive data, while
unsupported-future or ambiguous/conflicting history remains write-fenced.
Present invalid journal artifacts without usable native authority are fatal;
only the absence of both sources admits a new campaign.

Consequences:

- At the CRI-051 checkpoint, Campaign Schema 71 and runtime-settings Schema 24
  were current. Implementation
  `85572fca9340074c3c198c758f857c4f57b600d9`, UTC
  `2026-07-17T09:37:00Z`, label
  `schema71-settings24-campaign-recovery-journal`, is the sealed source; CRI-050
  remains the preceding field-vehicle checkpoint.
- Foundation passes 851 references. Stamped Workbench validation loads 5,842
  files/11,862 classes at CRC `c4bc4b3d` with zero hard errors and zero owned
  cleanup residue.
- The focused journal authority autotest passes 1/1 with an empty failed list,
  41/41 exact conditions, and native-v1/native-v2/invalid-fingerprint/future-
  envelope classification at 1/1/1/1. The cases include generation-zero
  promotion, damaged-latest recovery, replacement after damage, split-brain and
  broken-chain fencing, future-format rejection, and production source routing.
- The strict five-process chain passes 5/5 across automatic, manual, controlled
  shutdown, native restart, and profile-journal restart. It advances generations
  1 -> 2 -> 3, ends at canonical generation 3 with two valid slots and an exact
  chain, keeps both restore stages read-only, preserves exact field-vehicle state,
  and leaves cleanup at zero. The guarded native counterattack chain passes 3/3,
  selects a newer native checkpoint over a deliberately stale complete journal,
  preserves both journal slots and their exact chain, and leaves cleanup at zero.
- CRI-052 supersedes this decision only for administrative-reset commit ordering.
  Its write-ahead source, Workbench gate, and focused stale-native runtime proof
  are complete.
- Two generations protect against a damaged latest slot or interrupted inactive-
  slot update, but not simultaneous writers, corruption of both slots, profile
  loss, storage-device failure, or malicious replacement. Long-lived alpha
  campaigns still need external backups.

## CRI-052 - Commit Administrative Reset Before Native Replication

- Status: Accepted; final implementation, guarded Workbench, and three-process
  stale-native proof complete
- Date: 2026-07-17

Context: CRI-051 keeps ordinary checkpoints native-first: native success mirrors
the same staged DTO into the rotating verified JSON journal. Reusing that order
for admin new-campaign reset leaves no safe place to retire old runtime roots.
Cleanup before any durable replacement risks loss, while waiting for native
completion can capture stale roots. Rolling back after a newer reset JSON has
already committed would also restore authority that startup must reject as old.

Decision: Keep the ordinary contract unchanged. Every accepted native-active
ordinary checkpoint mirrors its exact staged snapshot into the verified journal
only after native success; profile-only ordinary checkpoints commit
synchronously.

Give admin reset a write-ahead transaction. Retain ordering floors, prepare every
fallible radio/civilian/field-vehicle cleanup plan, exact-clone the complete
prospective campaign, and apply only reversible radio restoration. Commit that
detached DTO synchronously to the rotating verified journal before native
staging. Journal failure rolls preparation back and leaves the old campaign.

The successful journal callback synchronously retires old radio, civilian,
ambient-vehicle, and non-retained field-vehicle roots, swaps campaign state, and
rebuilds projections in the same script call. Native staging then uses the
already-detached DTO. Once the write-ahead generation commits, native absence,
staging failure, callback failure, or timeout is degraded replica repair and may
not resurrect the old campaign.

Consequences:

- Campaign Schema 71 and runtime-settings Schema 24 remained current. At this
  decision's acceptance, implementation `3714e9c6d9e1d5dc802db5f8ededf4505acf256b`, UTC
  `2026-07-17T15:18:07Z`, label
  `schema71-settings24-admin-reset-write-ahead`, was the sealed source.
- Its stamped Foundation proof passed 859 references. Guarded Workbench validation loaded 5,844
  files and 11,870 classes at CRC `2b350976`, with zero HST, script, or hard
  errors and zero owned cleanup residue.
- The dedicated three-process
  `prepare_old_checkpoint -> reset_commit -> stale_native_no_save_verify` proof
  passes 3/3. Journal generations advance 1 -> 2 -> 3. The old checkpoint is
  `cp2/r0`, the deliberately stale native blocker is `cp4/r1`, the committed
  reset is `cp5/r1`, and final recovered live authority is `cp6/r2`.
- The final process deliberately loads stale valid native authority, selects the
  newer profile fallback, enters degraded newer-journal recovery, performs no
  save, and preserves canonical generation 3 plus recovery generation 2 as an
  exact chain. Journal and proof-carrier bytes remain unchanged, the old
  sentinel stays absent, player/commander identity remains exact, an overlapping
  reset is rejected without mutation, and cleanup is zero.

## CRI-053 - Prove Exact Enemy Garrison Rebuild Across Fresh Processes

- Status: Accepted; implementation and both guarded three-process restart cuts
  complete
- Date: 2026-07-17

Context: The campaign already carried the authority needed to rebuild an enemy
garrison from an accepted force manifest, and the in-process deterministic proof
covered that transition. It did not prove that current-shape rebuild authority
survives a real process boundary, preserves a prior casualty and partial route
progress, applies delivery exactly once, crosses the virtual/physical boundary,
folds back to strategic authority, and stays idempotent on replay.

Decision: Add narrow `prepare -> recover -> replay` dedicated-server cuts over
the production two-slot JSON journal. Bind every stage to the exact build,
schema, world, campaign, operation, force, resource, cut, and one-use stage
lease. Prepare one contract-1 rebuild at 225 of 300 meters with nine accepted
members, eight living members, one casualty, and no inherited process-local
claimant.

The `delivery_pending` cut advances the remaining virtual route through the
production owner and applies delivery once. It holds the exact eight-survivor
manifest at the destination, avoids an accepted-garrison double count, adds only
living strength, retains the original debit, and appends exactly one zero-refund
receipt. The `physical_live_fold` cut additionally requires
`VIRTUAL -> MATERIALIZING -> PHYSICAL/LIVE`, exact adapter/native bindings,
observed movement and target closure, and production
`PHYSICAL -> DEMATERIALIZING -> VIRTUAL` folding without losing the casualty
ledger. Both cuts persist generation 2 linked to generation 1. Replay performs
no save, leaves both journal slots and the proof carrier unchanged, creates no
second effect, and leaves no runtime claimant.

Consequences:

- Campaign Schema 71 and runtime-settings Schema 24 remain current. The sealed
  garrison implementation is
  `4ac1c5610eccc1c4f750055dc169b1063be38143`, UTC
  `2026-07-17T20:52:03Z`, label
  `schema71-settings24-garrison-rebuild-physical-fold`.
- Its stamped source gate passed Foundation at 865 references and Workbench at
  5,846 files/11,876 classes, CRC `57609980`, with zero hard errors and zero
  owned cleanup residue.
- Both cuts also pass after the later controlled-shutdown checkpoint. The
  `delivery_pending` cut proves exactly-once delivery and byte-read-only replay.
  The `physical_live_fold` cut proves one native root, nine adapter handles,
  eight living native members, 2.759 meters of movement, 0.539 meters of target
  closure, exact production fold, exact restart continuation, and replay no-op.
  Every cleanup counter is zero.
- This closes the deterministic exact-garrison virtual/physical/fold restart
  fixture. Natural route and combat variation, other force families, packaged
  live gameplay, multiplayer/JIP/reconnect, performance, and soak remain open.
  The focused autotest remains blocked by its reload/JUnit harness gap, so no
  new focused-autotest pass is claimed.

## CRI-054 - Fence Native Runtime Authority Before Controlled Shutdown

- Status: Accepted; implementation, Foundation, stamped Workbench, and scoped
  restart regressions complete; mixed-native proof status superseded by
  CRI-055
- Date: 2026-07-17

Context: The controlled-end bridge and durable field-vehicle fence did not prove
that every live save owner remained stable together. Nearby vehicle adoption,
active groups, civilian and field vehicles, rescue DTOs, captive followers,
carriers, seats, players, and native physics could change between fallible
checks. Publishing one owner latch before another preflight failed would make a
retry observe a partially committed shutdown transaction.

Decision: Make blocking `SHUTDOWN` an ordered one-way transaction. Run read-only
preflights for nearby durable vehicles, rescue authority, field/civilian
vehicles, and active groups; run complete capture preparation; then repeat the
prepared-graph and player checks before the first latch. After that latch,
repeat preparation and latch or maintain active-group, vehicle, and rescue
scopes in order. Once any scope is latched, retries maintain its original pins
and never reopen it.

Freeze commander/member/admin and mission/casualty/lifecycle ingress while the
coordinator drains and quiesces. Pin exact DTO plus native entity/group/member/
vehicle/captive/carrier/seat/damage/player topology. Reject foreign or player
occupancy before latching and publish canonical player-release evidence.
Quiesce follower callbacks, waypoints, AI, movement, and physics, plus recursive
group/vehicle controller, engine, brake, autohover, velocity, and physics state.
On retry, reapply pinned transforms while rejecting identity or topology drift.

Consequences:

- Campaign Schema 71 and runtime-settings Schema 24 remain unchanged. Current
  implementation is `60596bf77d056b9e63ed1bbbf4d11c1941330fe6`, UTC
  `2026-07-18T14:12:51Z`, label
  `schema71-settings24-mixed-native-shutdown-restart`.
- Foundation passes 874 references. Stamped Workbench validation passes 5,846
  files/11,899 classes at CRC `9a79a33a`, with zero hard errors and zero owned
  cleanup residue.
- The current ordinary five-process regression passes autosave, manual,
  shutdown, native verification, and profile-fallback verification. Generations
  advance 1 -> 2 -> 3; controlled-end bridging and field-vehicle state are
  exact; both verification stages are read-only; cleanup is zero. Both exact
  garrison restart cuts also pass on this build.
- At CRI-054 acceptance, the ordinary fixture was scoped bridge/field-vehicle
  regression evidence and had not yet exercised the dedicated mixed-native
  graph.
  CRI-055 closes that fixed graph without broadening the result to arbitrary
  mission/force shapes or packaged live multiplayer.

## CRI-055 - Prove Mixed-Native Controlled Shutdown Across Five Processes

- Status: Accepted and sealed; scoped proof complete
- Date: 2026-07-18

Context: CRI-054 established the ordered one-way shutdown fence, but its sealed
evidence exercised the coordinator/end bridge and durable field vehicles rather
than one simultaneous native rescue/carrier/player/seat and active-force graph.
The remaining gap was whether that mixed graph could reject foreign occupancy,
survive the blocking commit, and restore exactly through both native and profile
fallback without confusing a new process-local replication identity for the
persisted carrier identity.

Decision: Extend the disposable ordinary persistence chain with one dedicated
mixed-native graph and require all five stages: automatic checkpoint, manual
checkpoint, controlled shutdown, native restart verification, and profile-
fallback restart verification. The graph carries exact `FOLLOWING`, seatless
`BOARDING`, and seated `BOARDED` captives; a real player occupancy/release path;
foreign-occupant rejection before every one-way latch; one durable carrier and
stable authored seat token; and one exact active-group guard projection.

Carrier identity across restart is the durable one-to-one HST runtime binding.
The restored root's valid `RplId` proves only current-session replication and
must not rewrite or be compared as the persisted identity from the prior
process. Restore recreates the carrier, rematerializes the exact guard graph,
recreates all captive projections, and reseats the boarded captive by its stable
seat identity. Native and profile-fallback verification must preserve the same
logical fingerprint and leave no owned process, profile, guard, temporary,
adapter, or runtime residue.

Consequences:

- Campaign Schema 71 and runtime-settings Schema 24 remain unchanged. The
  sealed implementation/source identity is
  `60596bf77d056b9e63ed1bbbf4d11c1941330fe6`, UTC
  `2026-07-18T14:12:51Z`, label
  `schema71-settings24-mixed-native-shutdown-restart`.
- All five stages pass with exact captive dispositions, carrier and seat
  topology, player/foreign-occupancy protocol, durable carrier binding, guard
  rematerialization, native/profile-fallback restart fingerprints, and zero
  cleanup.
- This closes only the dedicated graph. Workshop/live multiplayer,
  JIP/reconnect, long soak, abrupt termination, arbitrary rescue and mission
  families, other force graphs, and broader active-world persistence remain
  independent certification gates.

## CRI-056 - Give Focused Engine Profiles A Self-Contained Typed Case

- Status: Accepted and sealed; scoped proof complete
- Date: 2026-07-18

Context: The `force_authority` runner selected only combat-presence, ownership-
transition, and town-influence assertions, but recorded the broader 300-
assertion force case. All selected assertions could pass while unrelated,
unselected assertions still made the parent case fail. Additional fixture defects
also obscured the intended authority boundaries: temporary injected services
were not retained strongly, migration candidates were compared against mutable
outputs, and malformed-save tests changed live result references rather than the
captured save copy.

Decision: A focused profile must build and finalize one dedicated typed case
whose assertion set exactly matches its advertised scope. The
`force_authority` case owns only the 9 combat-presence, 14 ownership-transition,
and 12 town-influence assertions. The runner validates that exact set plus build
provenance, artifact stability, state isolation, errors, crashes, and owned
cleanup. Direct political influence admission submits its exact receipt to the
canonical ownership service, which persists later valid work as queued and owns
FIFO execution; the outer fallback scan retains the one-new-fallback-command-
per-pass fence.

Consequences:

- Campaign Schema 71 and runtime-settings Schema 24 remain unchanged. The
  sealed implementation/source identity is
  `32727238d74b29905c68e5a80bb5897dfdc783c0`, UTC
  `2026-07-18T16:34:38Z`, label
  `schema71-settings24-focused-force-authority`.
- Foundation passes 874 references. Stamped Workbench validation passes 5,846
  files/11,899 classes at CRC `cad640f3`, with zero HST, script, or hard errors
  and exact-zero owned cleanup.
- The focused case passes all 35 targeted assertions and all 87 counted
  conditions. All 18 tracked state-diff rows remain zero; script and Partisan
  errors, crashes, and artifact drift are zero, and owned cleanup is exact-zero.
- `CertificationPassed:false` is intentional. A focused profile may prove its
  typed case but may not claim Full Campaign Debug, native-world,
  persistence/restart, packaged server/client, multiplayer/network/JIP/
  reconnect, performance, or soak certification.

## CRI-057 - Freeze Schema 71 And Generate One Release Truth Surface

- Status: Accepted
- Date: 2026-07-18

Context: Current evidence was distributed across large chronological documents
whose scoped checkpoints used several overlapping meanings of "current",
"verified", and "complete". The newest focused 35/35 state fixture is green,
while the newest completed Full Campaign Debug result is older and red. The
repository also exposed 39 mission IDs, a large dynamic command surface, and
contextual world actions without one machine-backed CE 3.11.1 behavioral
crosswalk. Further schema and feature expansion would increase that
certification gap.

Decision: Freeze Campaign Schema 71 and runtime-settings Schema 24 while the
release-closure gates run. Permit a schema advance only for data loss, a hard
campaign-lifetime cutoff, or a release-blocking persistence defect with its
migration proof. Make `docs/data/release_status.json` and
`docs/data/antistasi_ce311_parity.json` the checked data sources for generated
`CURRENT_STATUS.md` and `ANTISTASI_CE311_PARITY_MATRIX.md`. Require deterministic
generation and Foundation drift checks. Exact-compare the 39 config/runtime
mission IDs, the explicit routed command-action manifest, and the explicit
concrete contextual-action manifest; every surface maps to a behavioral
contract even when its disposition is legacy, missing, or development-only.

Keep checkout Git HEAD and embedded implementation identity separate. A commit
cannot embed its own hash in tracked content, and stamp commits intentionally
identify their implementation parent. Gate 1 must therefore retain both
identities, their relation, dirty state, toolchain versions/hashes, Workbench
CRC, schemas, package hashes, and evidence hashes in one candidate manifest.

Consequences:

- The current release decision is `NO-GO` development alpha.
- The focused force-authority result remains valid only for its state fixture;
  it does not supersede the older red integrated run or certify a package.
- Source or config additions that create a new mission, routed command, or
  contextual action fail the release-doc check until explicitly classified.
- Chronological documents retain useful mechanics and evidence, but the two
  generated documents own current status and the pinned behavior contract.
- Gate 1 must build a fresh Schema-71 candidate. A cached or older-schema
  package cannot inherit current source evidence.

## CRI-058 - Publish A Release Candidate Only After Complete Identity Binding

- Status: Accepted
- Date: 2026-07-18
- First-candidate runtime-use consequence superseded by CRI-059 on 2026-07-18.

Context: Existing validation and packaging runners could compile or exercise
source and could create temporary packed content, but they did not retain one
immutable package joined to every source, toolchain, and evidence identity.
Workbench validation also selected its platform implicitly and deleted raw
guarded logs during cleanup. Reusing separate or older results would make it
impossible to prove that later gates exercised the package being considered for
release.

Decision: Use `tools/new-guarded-release-candidate.ps1` as the single Gate-1
build-once boundary. Require a clean checkout, Foundation, explicit
PC/XBOX_ONE/XBOX_SERIES/PS4/PS5 Workbench passes with retained raw evidence, an
unchanged source recheck, and one guarded native Workbench pack. Admit exactly
the four native release files (`addon.gproj`, `data.pak`,
`resourceDatabase.rdb`, and `thumbnail.png`), exact-match source versus packed
project identity, and require the packed thumbnail to match tracked source
bytes. Define package identity as the aggregate SHA-256 of the canonical sorted
`sha256-manifest-v1` file index. These five configurations are the compiled
package targets; standard dedicated-server execution of the PC package remains
a separate runtime rung rather than a sixth `HEADLESS` compile result.

Build under a unique external partial directory and use only a fresh,
nonce-owned checkout-local scratch directory for native pack mechanics. Generate
and check one manifest that binds checkout and embedded build identity and
relationship, dirty state, both schemas, addon identity, Workbench and all-five
results, standard server/client identities, package files, and all retained
evidence. The candidate version is also the unpublished local addon's explicit
version. Rename the partial directory to its final candidate identity only
after that check, check the manifest again at the final location, and create the
matching ready seal as the final atomic publish operation.

Consequences:

- A failed run retains its uniquely owned partial evidence but never publishes
  it as a release candidate.
- Workbench raw evidence is copied by contained relative path only after process
  quiescence and before the exact disposable guard is removed.
- A clean source or green temporary pack cannot be combined later with evidence
  from another Git HEAD, package digest, target, or tool binary.
- The first retained candidate is
  `partisan-rc-c2b16c4a2d85-20260718T201442Z`, built from clean source HEAD
  `c2b16c4a2d85e71503cd46265feafb54bce69e83` with aggregate package SHA-256
  `8f60260331c6c7473465dc4517b1063a179a8f4efeffdcfe3d5eccac9af476db`.
  This closes the artifact-identity portion of Gate 1, not later runtime rungs.
  At acceptance, required release gates were to consume that unchanged package;
  CRI-059 subsequently supersedes that runtime-use consequence after the
  focused-suite defect, while preserving the rule that a rebuild starts a new
  candidate and evidence chain.

## CRI-059 - Stage Sealed Candidates And Supersede On Harness-Package Defects

- Status: Accepted
- Date: 2026-07-18

Context: The first sealed candidate proved build, toolchain, package, and
Workbench identity, but the Campaign Debug and focused wrappers still selected
the mutable checkout project. Mounting the external sealed directory directly
would also expose release bytes to engine write authority. Separately, the
stock autotest framework performs a base-only scenario transition for suites
that return its default world. Three service-only suites lacked the empty-world
override already used by two peer suites, so their packaged test type could be
unloaded before JUnit output.

Decision: Make the active tracked release status, tracked manifest/ready seal,
and external sealed bundle one fail-closed consumer contract. Validate exact
package/evidence inventory and hashes plus the selected runtime role, copy only
the verified four-file package into a nonce-owned guard, and launch the staged
packed project with explicit add-on GUID and a guard-owned add-on temp root.
Revalidate staged and external bytes after process quiescence. Retain raw
runtime output only in a separate fresh sidecar envelope bound to candidate,
harness, tool, outcome, and cleanup identities.

Give every service-only focused suite an explicit empty `GetWorldFile()` result
and enforce it in Foundation. Because those changes alter the package, do not
pretend the first candidate can inherit their proof. Retain it unchanged,
publish one replacement candidate, and restart the current focused/full-suite
evidence chain from the replacement.

Consequences:

- A candidate package is never mounted from its sealed storage location and is
  never mutated to append runtime evidence.
- Runtime evidence identifies both the standard manifest-pinned executable and
  the distinct, exact manifest-pinned diagnostic executable actually used by
  diagnostic gates.
- Release status is an execution control: a superseded candidate is eligible
  only for archive validation and consumer preflight, while a real launch
  requires `active-runtime-candidate`.
- Runtime envelopes bind the guarded settings copy actually consumed and prove
  that the clean harness HEAD and runner/module hashes did not change during
  execution.
- Preflight or historical evidence on the first candidate cannot certify the
  replacement, while the replacement cannot claim the first candidate's package
  identity.
- A release-critical test-registration defect is a valid reason to supersede a
  candidate before later gates, but each later rebuild restarts the evidence
  chain rather than merging results.

## CRI-060 - Activate The Single Focused-Suite Replacement Candidate

- Status: Accepted
- Date: 2026-07-18

Context: CRI-059 required exactly one replacement because the three
service-only suite registration repairs changed `data.pak`. The first package
had to remain byte-identical and its artifact evidence had to remain available,
but it could not receive current runtime results. The replacement also needed to
seal the distinct diagnostic binaries actually used by the guarded proof gates.

Decision: Retain the first candidate as superseded evidence and activate
`partisan-rc-b8deddc4b631-20260718T213322Z`, built from clean source HEAD
`b8deddc4b6314936b7ea04f36a35784622a46da6`. Its canonical four-file package
SHA-256 is
`82e1fd0bf7c3404b7fe842fa84efd10f225bf82fc76c11502b9a684b63f4f329`,
its Workbench CRC is `f27e637b` on all five targets, its manifest SHA-256 is
`c88d363423008bcba2366afa8d458613bf539c549969bcae73cd82e5cd9402a5`, and
its ready-seal SHA-256 is
`6518c536e7104f21af81c94c4959a66587c4efe67bf2339001489dcddad00d87`.
Mark only this candidate `active-runtime-candidate`.

Consequences:

- Every later focused, Campaign Debug, dedicated, multiplayer/JIP, restart,
  performance, soak, canary, and certification proof must consume this unchanged
  package identity.
- Foundation and all-five Workbench evidence are current for the replacement;
  no engine-world, dedicated, multiplayer, restart, performance, or soak result
  is implied by the build.
- The next gate is five individually named packaged focused tests, followed by
  current Full Campaign Debug. Any later rebuild creates another evidence chain
  and cannot inherit these package-bound results.

## CRI-061 - Bind Focused Hard Diagnostics Into Result Acceptance

- Status: Accepted
- Date: 2026-07-18

Context: The first all-five execution against the active replacement emitted
one passing JUnit result per named case, exact packed mounts, and zero cleanup or
spill residue. It also proved that the runner treated script and engine errors
as retained diagnostics only. Four cases emitted the same two stock post-result
filter-constructor errors. The profile-journal case additionally emitted one
intentional native-save failure inside its non-mutating fault-injection proof.
No other script or engine errors appeared, but the pass predicate did not prove
that classification.

Decision: Require a hard-diagnostic census for focused success. Approve exactly
two stock filter diagnostics only after runner completion, JUnit write, and
failed-list write. Approve exactly one native-save failure only for the profile-
journal testcase, inside its suite and before success, with both exact non-
mutating proof tokens. Reject every unapproved message, wrong count, wrong
ordering, wrong testcase, or missing proof token. Record total, approved, and
unapproved counts and explicitly report that these runs are not hard-diagnostic-
free.

Consequences:

- The initial five sidecars remain immutable preliminary evidence. All five
  cases rerun under one clean committed classifier-aware harness before their
  package-bound results are accepted.
- This change affects the external PowerShell harness and its separately
  recorded commit and file hash. It does not change the sealed package,
  manifest, ready seal, or add-on bytes and therefore does not create another
  package candidate.

## CRI-062 - Accept The Classifier-Bound Packaged Focused Set

- Status: Accepted
- Date: 2026-07-18

Context: CRI-061 made hard-diagnostic classification part of focused-run
success without changing the sealed candidate. The five preliminary sidecars
could not be promoted because their earlier harness did not enforce that
predicate. The same five package-bound cases therefore had to rerun under one
clean, committed classifier-aware harness.

Decision: Accept the second five-case set under harness HEAD
`b3fc1e6f56d9cf8805bac1702a54e0b5284e0043` as the packaged deterministic-
service rung for candidate `partisan-rc-b8deddc4b631-20260718T213322Z` and
package SHA-256
`82e1fd0bf7c3404b7fe842fa84efd10f225bf82fc76c11502b9a684b63f4f329`.
All five exact staged packed mounts passed with JUnit 5/0/0/0. All 40 envelope
files rehashed, cleanup and spill residue were zero, and the exact diagnostic
census was 11 = 10 approved stock + 1 approved intentional journal injection +
0 unapproved. Record `HardDiagnosticFree:false` alongside valid classification
rather than describing the set as error-free. Retain the first five sidecars as
preliminary, superseded-for-acceptance evidence.

Consequences:

- The deterministic-service rung is `passed-noncertifying`; no native-world,
  Full Campaign Debug, dedicated, multiplayer/JIP, restart breadth,
  performance, soak, canary, or stable-certification result is implied.
- The tracked portable summary binds the exact five envelope hashes and the
  clean runner/module identities. Release-document generation rehashes and
  cross-checks that summary against the active candidate and status totals.
- Current Full Campaign Debug is the next evidence gate against the same
  unchanged package. A valid but red integrated report remains diagnostic
  evidence and must not be called a pass.

## CRI-063 - Superseded Full-Suite Evidence Acceptance

- Status: Superseded by CRI-064
- Date: 2026-07-18

Context: This decision recorded the then-believed conclusion that the guarded
canary and full run had clean error censuses and that wrapper success made the
red full report acceptable as current evidence. A later audit showed that the
wrapper's line-start expressions missed timestamp-prefixed `SCRIPT (E)` lines.
Its zero-error premise and resulting evidence-acceptance conclusion were false.

Decision: Retain CRI-063 only as the superseded record of that mistaken
classification. Its `NO-GO`, failed native-engine/world, and blocked stable-
certification outcomes remain conservative, but it no longer authorizes the
canary or full capture as accepted evidence.

Consequences:

- The raw files and wrapper-reported result remain immutable historical facts.
- CRI-064 owns the corrected census, evidence disposition, and rerun rule.

## CRI-064 - Retain Preliminary Captures and Correct the Diagnostic Gate

- Status: Accepted
- Date: 2026-07-18

Context: An independent timestamp-aware census of each canonical script log
found three raw `SCRIPT (E)` lines in the `force_authority` canary. Two are exact
approved stock diagnostics and one is unapproved. The full run contains 25 raw
`SCRIPT (E)` lines, including a 19-line Partisan subset. Exact classification
accounts for two approved stock diagnostics and 13 proof-bound intentional
diagnostics, leaving ten unapproved. The original wrapper reported zero because
its census required `SCRIPT (E)` at the start of a timestamped line.

The captures are otherwise mechanically complete and stable: candidate and
packed-mount identity, guarded settings, artifact hashes and rehashes, state
restoration, final orphan cleanup, and cleanup/spill hygiene remain recorded.
The canary report recorded 35/35 focused assertions and 87/87 counted
conditions. The full report remains failed at 584 PASS, 49 WARN, 46 FAIL, 7
BLOCKED, and 1 SKIPPED, with 112 failed and 14 blocked required assertions.

Decision: Preserve the immutable raw captures and their wrapper-reported fields,
but classify both evidence sets as `preliminary-unaccepted`. Wrapper success is
not acceptance. Record the independent raw, subset, approved, intentional, and
unapproved counts separately, and require the release generator to fail closed
unless the corrected classifier disposition is explicit and exact.

Repair the external runner to census one canonical script log and one canonical
console log with timestamp-aware matching. Count `SCRIPT (E)` from the script
log, merge only unique or excess hard diagnostics from the console log, and
retain console-only `ENGINE (E)` and crash signals. Then rerun the canary and
full profile against the unchanged package. A runner-only correction may extend
this candidate's evidence chain because it does not alter packaged bytes. Any
gameplay, in-package fixture, or package change still requires a newly sealed
candidate and a new evidence chain.

Consequences:

- The release decision remains `NO-GO`; native-engine/world remains failed and
  stable certification remains blocked.
- Report totals and case-exclusive triage remain useful diagnostic coordinates,
  but they are not accepted release evidence.
- Gameplay correction begins only after a corrected canary produces a trustworthy
  classification. A rejected canary stops the sequence before the full profile;
  CRI-065 records this outcome without changing the preliminary disposition.
- Dedicated, multiplayer/JIP, restart breadth, performance, soak, canary-release,
  and stable-certification gates remain independent.

Outcome update: Clean committed harness `38a094f` completed the corrected
`force_authority` canary first and rejected it on one unapproved map-locator VM
exception. The full profile was therefore not rerun. CRI-065 records the
resulting staged-gate interpretation and package correction; this update does
not rewrite CRI-064's original evidence disposition or repair decision.

## CRI-065 - Stop at the Rejected Canary and Guard Map-Locator Lifecycle

- Status: Accepted
- Date: 2026-07-18

Context: The corrected classifier under clean committed harness `38a094f`
reran `force_authority` against the unchanged active candidate. The focused
proof again recorded 35/35 assertions and 87/87 certification-counting
conditions. The exact raw census was three hard diagnostics = two approved
stock diagnostics + one unapproved map-locator VM exception. Cleanup and spill
residue were zero. The runner failed closed, so the full profile was correctly
not rerun.

The exception reproduced a concrete lifecycle defect. Campaign Debug setup
opens the `PLAIN` map before bootstrap has a controlled character. The stock
locator's immediate no-player update removes the hint root but leaves its
ten-second callback scheduled. When bootstrap supplies the player before that
callback fires, the callback can dereference stale widget state.

Decision: Preserve the rejected canary as immutable preliminary-unaccepted
evidence and treat canary then full as a staged sequence: a rejected canary
stops the sequence. Do not grandfather the VM exception or spend a full run to
reconfirm a known gate failure.

Add a narrow modded `SCR_MapLocator.CalculateClosestLocation` guard. Before
entering stock behavior, validate the hint layout, both hint-text widgets, and
world directions. On invalid state, remove any remaining layout, cancel the
stale callback, clear the references, and return. On valid state, preserve the
stock implementation unchanged. Foundation and PC Workbench compile validation
pass at 5,847 files/11,900 classes and CRC `3a399db1`, with zero errors and
residue.

Consequences:

- The validation result is source/compile proof, not runtime proof that the
  delayed callback is safe in the packaged map lifecycle.
- The correction changes packaged source, so it cannot extend the unchanged
  candidate's runtime evidence chain. Seal a new immutable candidate before the
  next canary.
- Rerun the corrected canary against that new candidate. Run Full Campaign
  Debug only if the canary is accepted; keep the old full capture as preliminary
  triage rather than mixing package identities.
- The release decision remains `NO-GO`; dedicated, multiplayer/JIP, restart
  breadth, performance, soak, canary-release, and stable certification remain
  independent.

Outcome update: The required package replacement is now sealed as
`partisan-rc-0e632ec4f63e-20260719T004133Z`. CRI-066 records its exact identity,
evidence disposition, and runtime-gate order. This closes CRI-065's build action
only; it does not prove the delayed callback safe at runtime.

## CRI-066 - Activate the Map-Locator Replacement Without Inheriting Runtime Evidence

- Status: Accepted
- Date: 2026-07-18

Context: CRI-065 required a new package because the map-locator lifecycle guard
changed packaged source. The preceding candidate's accepted focused set and its
preliminary-unaccepted canary/full captures remain useful package-bound history,
but none can prove the changed package. The replacement therefore needed a new
immutable seal and a fresh evidence chain.

Decision: Activate `partisan-rc-0e632ec4f63e-20260719T004133Z`, built from clean
source HEAD `0e632ec4f63eab43e8c301d0755f10193d85131f`. Its canonical four-file
package SHA-256 is
`e5d29458c33aeef9cd2b37476359acc6021fe78cf0fc74513d9a2f69ef0614dc`,
its manifest SHA-256 is
`ea06318a8f5161f000685fe37ecab4f5c8a77d6b0e8205f502a6418e3365e76b`,
and its ready-seal SHA-256 is
`cd91e569b8a4a453dad6b0f884f22afbb36b9b5f0de629fd70b2188875e47c53`.
Foundation and all five Workbench targets passed at 5,847 files/11,900 classes
and common CRC `3a399db1`, with zero hard errors and exact-zero cleanup. The
sealed inventories contain four package files and 50 evidence files.

Consequences:

- The seal is artifact/build evidence only. The candidate has no package-bound
  focused, corrected-canary, or Full Campaign Debug result yet and does not prove
  the map-locator correction at runtime.
- Run all five packaged focused cases on this exact package first. Run the
  corrected `force_authority` canary only after all five pass, and run Full
  Campaign Debug only if that canary is accepted.
- Keep the preceding rejected canary and full capture immutable as historical
  preliminary-unaccepted evidence. Never combine their candidate, package, or
  runtime result identities with this chain.
- The release decision remains `NO-GO`; dedicated, multiplayer/JIP, restart
  breadth, performance, soak, canary-release, and stable certification remain
  independent.

Outcome update: The active candidate's first classifier-aware five-case set is
now accepted as `passed-noncertifying`. CRI-067 records the exact harness,
window, counts, and next staged gate; this does not make the map-locator runtime
correction or any integrated Campaign Debug result pass.

## CRI-067 - Accept the Replacement Candidate's Packaged Focused Set

- Status: Accepted
- Date: 2026-07-18

Context: CRI-066 activated the map-locator replacement with a fresh evidence
chain and required all five package-bound focused cases before another corrected
canary. The five cases ran against exact candidate
`partisan-rc-0e632ec4f63e-20260719T004133Z` under clean harness HEAD
`d4d8f29cda9896ce2c6a5b073dac2cbd03757700` from
`2026-07-19T01:08:50.9577409Z` through
`2026-07-19T01:09:44.4465092Z`.

Decision: Accept that five-case set as the active package's non-certifying
deterministic-service rung. All 5/5 cases passed with aggregate JUnit 5/0/0/0.
All 40 envelope files rehashed, every cleanup and spill count was zero, and all
12 diagnostic-classifier checks passed in each run. The exact hard-diagnostic
census was 11 = ten approved stock diagnostics + one approved intentional
journal fault + zero unapproved. Record `HardDiagnosticFree:false` with valid
classification. Bind the result through portable summary SHA-256
`961ef6b0a84c26446468b31dd7ac5120448b21a442e9a823de4ff5dc804da7f9`.

Consequences:

- The deterministic-service rung is `passed-noncertifying`. It does not prove
  the delayed map-locator callback safe or satisfy corrected-canary, Full
  Campaign Debug, native-world, dedicated, multiplayer/JIP, restart breadth,
  performance, soak, canary-release, or stable-certification gates.
- Run the corrected `force_authority` canary next against the unchanged active
  candidate. Run Full Campaign Debug only if that canary is accepted.
- Preserve the preceding package's focused, rejected-canary, and failed-full
  results as immutable historical evidence; none transfers into this chain.
- The release decision remains `NO-GO`.

Outcome update: The active candidate's corrected scoped `force_authority`
canary is now accepted as `passed-noncertifying`. CRI-068 records its exact
identity, counts, map-locator finding, and progression to Full Campaign Debug.

## CRI-068 - Accept the Corrected Active-Candidate Canary as Scoped Evidence

- Status: Accepted
- Date: 2026-07-18

Context: CRI-067 advanced the active package's deterministic-service rung and
required a corrected canary before the full profile. Clean harness HEAD
`20375141f840f74316ca46e7df047fcba3e6e344` ran `force_authority` against the
unchanged candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. Run
`seed1985_t0_p1_u1784424219`, leaf
`20260719T012319Z-47423d741d0e4690b3c7dbbbab68cebd`, started at
`2026-07-19T01:23:19.5837772Z`, completed at
`2026-07-19T01:24:02.2143579Z`, and recorded 40 canonical runtime seconds.

The 11-case report recorded 9 PASS, 1 WARN, 0 FAIL, 1 BLOCKED, and 0 SKIPPED.
The focused proof passed all 35/35 assertions and all 87/87 certification-
counting conditions. State restoration was 18/0, final orphan cleanup passed
with zero active groups, all ten envelope files rehashed, and every owned
cleanup and monitored spill counter was zero. The exact hard-diagnostic census
was two = two approved stock + zero approved intentional + zero unapproved.
All 33 classifier self-tests passed. Envelope SHA-256 is
`e3705a849590b9fd3086fdb0caf5659df6e0c1029784612965c848a0f8f0a851`.
The portable summary is
`docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-corrected-canary-20260719T012319Z.json`,
SHA-256 `f47fa5f0539c0c8c6024e096f3e034699bc6bfaf656734a0a2b32c9fee7b4aa8`.

Decision: Accept this corrected canary as `passed-noncertifying` scoped native-
engine/world evidence. The unapproved map-locator VM exception reproduced by
the prior package is absent for this scoped path. This result validates the
specific canary path and permits the staged sequence to proceed; it does not
certify the full native engine-world surface.

Consequences:

- Run Full Campaign Debug next against the unchanged active package.
- Preserve the accepted canary summary and external raw sidecar as one exact
  candidate/harness evidence chain; do not combine them with prior-package
  captures.
- Keep broader map lifecycle, dedicated server/client, multiplayer/JIP,
  restart breadth, performance, soak, canary-release, and stable certification
  as independent gates.
- The release decision remains `NO-GO`.

Outcome update: The active candidate's Full Campaign Debug profile is now a
mechanically complete but rejected red capture. CRI-069 records the exact
identity, separates wrapper integrity from runtime acceptance, and keeps the
release ladder fail-closed.

## CRI-069 - Record the Active Full Profile as Rejected Red Evidence

- Status: Accepted
- Date: 2026-07-18

Context: CRI-068 permitted the unchanged active package to advance from its
accepted scoped canary into Full Campaign Debug. Clean harness HEAD
`27052811bb192835fc09ab3cb052b36cabad5df4` ran exact candidate
`partisan-rc-0e632ec4f63e-20260719T004133Z`. Run
`seed1985_t0_p1_u1784425330`, leaf
`20260719T014151Z-470870c9cc7e4493afb9a6ceb6ff2bce`, started at
`2026-07-19T01:41:51.6669572Z` and completed at
`2026-07-19T01:54:57.0935077Z`.

Decision: Preserve the wrapper capture as mechanically valid evidence while
rejecting the runtime result. The wrapper verified exact candidate and packed-
mount identity, stable artifacts, ten rehashed envelope files, 18/0 state
restoration, final orphan cleanup, and zero cleanup/spill residue. Runtime and
certification acceptance remained false. The 687-case result was 584 PASS, 49
WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED. Certification proved 5,561/5,687
required assertions, with 112 failed and 14 blocked. The fail-closed census was
25 hard diagnostics = two approved stock + 13 approved intentional + ten
unapproved. Bind the result through envelope SHA-256
`f61bd05fcc5c95c5d0ddbbeb46a9220771d116b86bad1ad4f26340f4853ec825`
and portable summary SHA-256
`ed225ba2acb6932437af55219ff0b6ba69f4a2111880acd11c2555c875819ca7`.

Consequences:

- Mark native-engine/world `failed`; keep canary-release and stable blocked.
- Keep release `NO-GO`. Mechanical capture success is not certification or
  diagnostic acceptance.
- Triage the exact 46/7 case, 112/14 required-assertion, and ten-unapproved-
  diagnostic boundary before another full-profile attempt.
- Any source correction made after this capture is outside the immutable
  package. Seal a replacement candidate and start a fresh evidence chain; do
  not attach those corrections to this result.
- Dedicated server/client, multiplayer/JIP, restart breadth, performance, and
  soak remain independent gates.

## CRI-070 - Activate the Source-Fixed Replacement Candidate

- Status: Accepted
- Date: 2026-07-19

Context: CRI-069 rejected the immediately prior package's full profile and
required every post-capture correction to enter a new immutable evidence
chain. Source HEAD `e11e7ea88a44ea07d7a81c0b4009f029f0b297e1` contains that
correction batch. Its clean candidate build passed the 874-reference
Foundation gate and all five explicit Workbench targets at 5,848 files / 11,901
classes / common CRC `e4cde465`, with zero first-party hard errors and zero
cleanup or monitored-boundary residue.

Decision: Activate
`partisan-rc-e11e7ea88a44-20260719T040154Z`, version
`0.1.0-rc.20260719T040154Z.e11e7ea8`, as the sole runtime candidate. Bind it to
aggregate package SHA-256
`75b61eb19513de00e56a43ad3778885f89a7497c0eebe4d870bf3b11e62a0dad`,
manifest SHA-256
`daed6876ce839a7fc6551257e4a4dd9bb0c92772c7e2d07be595acddde19e714`,
and ready-seal SHA-256
`0ca7a5e2fbe6bf298baa542250cc7b47bf2b135a5382e032fc5febdddf579acc`.
The manifest retains the canonical four-file package and 50 evidence files.
Treat the candidate as artifact-only until its own runtime sidecars exist.

Consequences:

- Reclassify the immediately prior `0e632ec4` candidate's five-case focused
  pass, accepted corrected canary, and rejected full profile as immutable
  historical evidence. None of those results transfers to this package.
- Reset the active deterministic-service and native engine/world rungs to
  `not-run`; static/source/resource and compile/configuration remain green for
  this exact source and package.
- Run all five packaged focused cases in canonical order. Run the corrected
  `force_authority` canary only if all five pass, and run Full Campaign Debug
  only if that canary is accepted.
- Keep release `NO-GO`. Dedicated server/client, multiplayer/JIP, restart
  breadth, performance, soak, canary-release, and stable certification remain
  independent gates.

Outcome update: The source-fixed replacement's five canonical packaged focused
cases are now accepted as a scoped `passed-noncertifying` deterministic-service
rung. CRI-071 records the exact package and harness binding, diagnostic census,
and progression to the corrected canary without advancing native-engine/world
or release certification.

## CRI-071 - Accept the Source-Fixed Replacement Packaged Focused Set

- Status: Accepted
- Date: 2026-07-19

Context: CRI-070 activated a source-fixed replacement and required a fresh
package-bound chain beginning with the five canonical focused cases. Exact
candidate `partisan-rc-e11e7ea88a44-20260719T040154Z`, built from source HEAD
`e11e7ea88a44ea07d7a81c0b4009f029f0b297e1` with package SHA-256
`75b61eb19513de00e56a43ad3778885f89a7497c0eebe4d870bf3b11e62a0dad`,
ran serially in canonical order under clean harness HEAD
`b1940f241e28f163202807385f7140f048921447`. The evidence window began at
`2026-07-19T04:44:01.2295133Z` and ended at
`2026-07-19T04:45:58.8756237Z`.

Decision: Accept the five-case set as the active package's scoped
`passed-noncertifying` deterministic-service rung. All 5/5 cases passed with
aggregate JUnit 5/0/0/0. All 40 retained envelope files rehashed, every cleanup
and monitored spill count was zero, and all 12 diagnostic-classifier checks
passed in each run. The exact hard-diagnostic census was 11 = ten approved
stock diagnostics + one approved intentional journal fault + zero unapproved.
Record `HardDiagnosticFree:false`; valid classification and exactly zero
unapproved diagnostics, rather than an empty raw error channel, satisfy this
focused gate. Bind the aggregate through portable summary SHA-256
`9ddade1cb86a209acf4aae02ded6f1a7713fe1e25ba577ae00ef1980e3de149a`.

Consequences:

- The focused result is package-bound to the exact active candidate and clean
  harness above. Do not combine it with any preceding candidate's canary or
  full-profile evidence.
- The deterministic-service rung is `passed-noncertifying`; native-engine/world
  remains `not-run`. The result does not certify Full Campaign Debug, dedicated
  server/client, multiplayer/JIP, restart breadth, performance, soak,
  canary-release, or stable certification.
- Run the corrected `force_authority` canary next against the unchanged active
  package. Run Full Campaign Debug only if that canary is accepted.
- The release decision remains `NO-GO`.

Outcome update: The source-fixed replacement's corrected canary is a stable,
fully retained, but rejected capture. CRI-072 records the two stale ownership-
proof assertions, stops the package before Full Campaign Debug, and requires
the fixture correction to begin a new immutable candidate chain.

## CRI-072 - Reject the Active Replacement Canary and Require a New Candidate

- Status: Accepted
- Date: 2026-07-19

Context: CRI-071 advanced the exact active candidate
`partisan-rc-e11e7ea88a44-20260719T040154Z` from its accepted packaged focused
set to the corrected `force_authority` canary. Clean harness HEAD
`937c86c5d2259a9da270ea76371001ac1d4c6eed` ran
`seed1985_t0_p1_u1784437399` from `2026-07-19T05:03:02.0611638Z` through
`2026-07-19T05:03:41.5393020Z` against the unchanged sealed package.

Decision: Preserve the complete envelope but reject the proof result. The 11
cases ended 8 PASS, 1 WARN, 1 FAIL, 1 BLOCKED, and 0 SKIPPED. Focused authority
proved 33/35 assertions and 85/87 counted conditions. Its two failed assertions
were `ownership_transition.aggregate` and `ownership_transition.causes`; the
cause evidence reported mission 0 and serialized queued/political exact-once
0, while restore and repeat/restart remained green. Candidate/package binding,
packed-mount verification, stable artifacts, the 33-check diagnostic
classifier, all ten rehashed envelope files, 18/0 state restoration, final
orphan cleanup, and zero cleanup/spill residue remained valid. The diagnostic
census was two approved stock + zero intentional + zero unapproved. Bind this
rejection through envelope SHA-256
`8deca62633394025bfa976f6d883f9b500d56519fd13e875f241679f4799cd21`
and portable summary SHA-256
`af0aca25a84d8f757dbba8010950a658ce09937aa4048c35b2e372f1183eec69`.

Root cause: Production correctly assigns `mission_capture` provenance only
when `FindActiveMission` resolves the exact supplied source ID. Both affected
proof paths supplied invented mission IDs without matching active missions, so
production preserved them as military zone-capture sources while the stale
fixture still expected mission provenance. The current source correction adds
one exact active-mission factory used by both cause routing and serialized
queued-intent proof, asserts mission cause/type/ID, and retains an unresolved-ID
negative case as `military_capture` / `zone_capture` with the original source
ID. It also separates political-policy exactness from restored mission
provenance so either defect remains visible, while keeping restore and
repeat/restart cardinality independently observable. Foundation pins these
positive, negative, serialized, and reporting contracts.

Consequences:

- Do not roll back production mission-source attribution and do not change
  Campaign Schema 71 or settings Schema 24. This is a stale proof-fixture
  correction, not a gameplay or persisted-data contract change.
- Stop Full Campaign Debug for the rejected package. Preserve its focused pass
  and rejected canary as immutable package-bound evidence; neither can certify
  a later package.
- Mark the package `rejected-after-runtime`. Verification may still rehash and
  inspect it, but every runtime consumer must reject it before process creation.
  The seventeen-check consumer suite pins active, superseded, rejected, invalid,
  tamper, and layout paths.
- Seal the fixture correction in a new immutable candidate, then restart the
  required focused -> corrected-canary -> full sequence from that identity.
  Full may run only after the new candidate's canary is accepted.
- Native-engine/world remains failed for this package. Dedicated server/client,
  multiplayer/JIP, restart breadth, performance, soak, canary-release, and
  stable certification remain independent gates.
- The release decision remains `NO-GO`.

## CRI-073 - Adopt an Ordered Historical-Candidate Release Ledger

- Status: Accepted
- Date: 2026-07-19

Context: The singular Schema-2 historical-candidate object can preserve one
retired package boundary but cannot accumulate later rejected candidates without
overwriting history or inventing a common gate topology. The current retained
artifact, `partisan-rc-e11e7ea88a44-20260719T040154Z`, was rejected at its
corrected canary and therefore has no full-profile evidence. This decision is
about release-ledger Schema 3 only. Campaign Schema 71 and runtime-settings
Schema 24 remain unchanged.

Decision: Make `historicalCandidateEvidence` a true one-or-more JSON array in
oldest-to-newest retirement order. Every entry has exactly
`retirementDisposition`, `candidate`, and `evidence`. Admit only these closed
topologies:

- `rejected-after-full-profile`: package-bound focused evidence, an accepted
  corrected canary, and rejected full-profile evidence are all required.
- `rejected-after-corrected-canary`: package-bound focused evidence and a
  rejected corrected canary are required; `fullCampaignDebug` is forbidden.
  A stopped full run must remain absent rather than `null`, `not-run`, borrowed,
  or fabricated.

The migration records only
`partisan-rc-0e632ec4f63e-20260719T004133Z` as
`rejected-after-full-profile`. Its package, manifest, and ready-seal SHA-256
values remain
`e5d29458c33aeef9cd2b37476359acc6021fe78cf0fc74513d9a2f69ef0614dc`,
`ea06318a8f5161f000685fe37ecab4f5c8a77d6b0e8205f502a6418e3365e76b`, and
`cd91e569b8a4a453dad6b0f884f22afbb36b9b5f0de629fd70b2188875e47c53`.
Its focused, corrected-canary, and full portable-summary SHA-256 values remain
`961ef6b0a84c26446468b31dd7ac5120448b21a442e9a823de4ff5dc804da7f9`,
`f47fa5f0539c0c8c6024e096f3e034699bc6bfaf656734a0a2b32c9fee7b4aa8`, and
`ed225ba2acb6932437af55219ff0b6ba69f4a2111880acd11c2555c875819ca7`.

Keep `partisan-rc-e11e7ea88a44-20260719T040154Z` solely as the current retained
`rejected-after-runtime` artifact until a replacement is actually activated.
Its package, manifest, and ready-seal SHA-256 values remain
`75b61eb19513de00e56a43ad3778885f89a7497c0eebe4d870bf3b11e62a0dad`,
`daed6876ce839a7fc6551257e4a4dd9bb0c92772c7e2d07be595acddde19e714`, and
`0ca7a5e2fbe6bf298baa542250cc7b47bf2b135a5382e032fc5febdddf579acc`.
Its accepted focused and rejected corrected-canary summary SHA-256 values remain
`9ddade1cb86a209acf4aae02ded6f1a7713fe1e25ba577ae00ef1980e3de149a` and
`af0aca25a84d8f757dbba8010950a658ce09937aa4048c35b2e372f1183eec69`;
there is no full-profile summary.

Consequences:

- Candidate IDs, source HEADs, manifest paths and hashes, ready-seal hashes,
  package hashes, and candidate-bound evidence identities must be exact and
  non-conflicting across every historical entry and the current artifact.
  Every referenced file is rehashed and cross-correlated to its own candidate.
- Candidate creation and gate timestamps must prove the declared order.
  Candidate source and focused/canary/permitted-full harness commits must prove
  the disposition-specific Git ancestry chain through the current checkout.
  Rendering preserves array order and keeps each candidate's evidence together.
- The next candidate activation is one fail-closed ledger transition. It must
  validate the new current identity, append `e11e7ea88a44` exactly once after
  `0e632ec4f63e` as `rejected-after-corrected-canary`, keep
  `fullCampaignDebug` absent, and replace every current artifact, evidence, and
  proof-rung field together. An intermediate duplicate or mixed identity is
  invalid and must not be published.
- No evidence count, hash, proof outcome, failed rung, or package eligibility is
  improved by this structural migration. Release remains `NO-GO`.

Outcome: CRI-074 completed the anticipated atomic transition. The ordered
history now contains 0e followed by e11, and ee0 is the active runtime candidate.

## CRI-074 - Activate the Fixture-Corrected ee0 Release Candidate

- Status: Accepted
- Date: 2026-07-19

Context: CRI-073 defined the fail-closed transition required when replacing the
e11 candidate rejected at corrected canary. The proof-fixture correction is now
sealed in a clean build-once candidate. Activation must preserve the complete
e11 failure boundary without transferring its evidence or fabricating a full
result.

Decision: Activate `partisan-rc-ee0e8add2a29-20260719T063815Z`, version
`0.1.0-rc.20260719T063815Z.ee0e8add`, from clean source HEAD
`ee0e8add2a298e83fd304b7660c4fc480dc6383f`. Its exact four-file package,
manifest, and ready-seal SHA-256 values are respectively
`981258439b9d08866c4883471cacfe33aa373a36a667a39e8c939f285db74daf`,
`1b877e3aa21773a268704bcb3fe889768fca3aa2d78541aa7285b061398ce907`, and
`01741b85d0edba69f54b07388cdd7c452b8f6f1ad7ef4f6faf253918a4bbf280`.
Foundation passes all 874 references. PC, PS4, PS5, XBOX_ONE, and XBOX_SERIES
each pass at 5,848 files/11,901 classes with common CRC `f64e0868`; the seal
binds four package files and 50 evidence files.

In the same checked transition, retain
`partisan-rc-0e632ec4f63e-20260719T004133Z` as ordered `history[0]` with
`rejected-after-full-profile`, and append
`partisan-rc-e11e7ea88a44-20260719T040154Z` exactly once as ordered `history[1]`
with `rejected-after-corrected-canary`. The e11 entry retains package SHA-256
`75b61eb19513de00e56a43ad3778885f89a7497c0eebe4d870bf3b11e62a0dad`,
manifest SHA-256
`daed6876ce839a7fc6551257e4a4dd9bb0c92772c7e2d07be595acddde19e714`,
ready-seal SHA-256
`0ca7a5e2fbe6bf298baa542250cc7b47bf2b135a5382e032fc5febdddf579acc`,
focused-summary SHA-256
`9ddade1cb86a209acf4aae02ded6f1a7713fe1e25ba577ae00ef1980e3de149a`,
and rejected corrected-canary summary SHA-256
`af0aca25a84d8f757dbba8010950a658ce09937aa4048c35b2e372f1183eec69`.
Its corrected-canary envelope SHA-256 remains
`8deca62633394025bfa976f6d883f9b500d56519fd13e875f241679f4799cd21`.
`fullCampaignDebug` is absent because the full profile did not run.

Consequences:

- At activation, the ee0 candidate was runtime-eligible but not runtime-proven;
  its packaged focused, corrected-canary, and full-profile rungs were `not-run`.
- Historical 35/35 and 87/87 state-only force-authority proof remains nonpackage
  and noncertifying; no historical result advances ee0.
- The activation's next gate was the five canonical packaged focused cases.
  Corrected canary and full remained later conditional gates.
- Release remains `NO-GO`; all independent dedicated, multiplayer/JIP, restart,
  migration, performance, soak, and certification gates remain open.

Outcome update: The ee0 candidate's five canonical packaged focused cases are
now accepted as `passed-noncertifying`. CRI-075 records the exact harness and
result boundary. Native-engine/world remains `not-run`.

## CRI-075 - Accept the ee0 Packaged Focused Gate

- Status: Accepted
- Date: 2026-07-19

Context: CRI-074 activated exact candidate
`partisan-rc-ee0e8add2a29-20260719T063815Z` with no transferred runtime
evidence. Its first required runtime gate was the five canonical packaged
focused cases against the exact staged package.

Decision: Accept the five-case set run at approximately
`2026-07-19T07:02Z` under clean harness HEAD
`273ed14ba8526259c8b0d248177fa53b59ade683` as the active package's scoped
`passed-noncertifying` deterministic-service rung. All 5/5 cases passed with
aggregate JUnit 5/0/0/0. All 40 envelope files were retained, every run passed
all 12 diagnostic-classifier checks, exact candidate and packed-mount binding
passed, and every cleanup and monitored spill count was zero. The exact hard-
diagnostic census was 11 = ten approved stock + one approved intentional + zero
unapproved.

Consequences:

- This acceptance belongs only to ee0 and clean harness
  `273ed14ba8526259c8b0d248177fa53b59ade683`. Historical 0e/e11 package or
  state-only evidence does not contribute to it.
- Deterministic-service is `passed-noncertifying`; native-engine/world remains
  `not-run`. This does not certify Full Campaign Debug, dedicated server/client,
  multiplayer/JIP, restart breadth, performance, soak, canary release, or stable
  release.
- Run the corrected `force_authority` canary next against the exact unchanged
  candidate. Run the full profile only if that canary is accepted.
- Release remains `NO-GO`.

Outcome update: The unchanged ee0 candidate's corrected `force_authority`
canary is now accepted as scoped `passed-noncertifying` native-engine/world
evidence. CRI-076 records the exact package, harness, and result boundary. Full
Campaign Debug is the next gate.

## CRI-076 - Accept the ee0 Corrected Force-Authority Canary

- Status: Accepted
- Date: 2026-07-19

Context: CRI-075 accepted the active ee0 package's five canonical focused
suites and required a corrected `force_authority` canary against the exact
unchanged candidate before Full Campaign Debug could run.

Decision: Accept run `seed1985_t0_p1_u1784445266`, captured from
`2026-07-19T07:14:08.9013899Z` through `2026-07-19T07:14:47.8353805Z` under
clean harness HEAD `4f8d7e2d7a39896737fd6754060523bf852c5fa8`, as scoped
`passed-noncertifying` corrected-canary evidence for candidate
`partisan-rc-ee0e8add2a29-20260719T063815Z` and exact package SHA-256
`981258439b9d08866c4883471cacfe33aa373a36a667a39e8c939f285db74daf`.
The 11 cases ended 9 PASS, 1 WARN, 0 FAIL, 1 BLOCKED, and 0 SKIPPED. All 35/35
focused assertions and 87/87 certification-counting assertions were proven;
state restoration was 18/0. All ten envelope files rehashed, final orphan
cleanup passed, all 33 classifier checks passed, and every cleanup and monitored
spill count was zero. The exact hard-diagnostic census was two approved stock
plus zero unapproved.

Consequences:

- This result belongs only to the exact ee0 package and clean harness
  `4f8d7e2d7a39896737fd6754060523bf852c5fa8`; no historical or state-only
  result contributes to it.
- The corrected native-engine/world canary advances only to
  `passed-noncertifying`. It does not certify Full Campaign Debug, dedicated
  server/client, multiplayer/JIP, restart breadth, migration, performance, soak,
  canary release, or stable release.
- Full Campaign Debug is now the next gate against the same unchanged package.
- Release remains `NO-GO`.

Outcome update: Full Campaign Debug has now run against the exact unchanged ee0
package and is rejected red. CRI-077 records the mechanically exact capture,
independent certification failure, diagnostic clusters, and immutable-package
boundary.

## CRI-077 - Record the ee0 Full Campaign Debug Result as Rejected Red

- Status: Accepted
- Date: 2026-07-19

Context: CRI-076 accepted the corrected canary only as scoped
`passed-noncertifying` native-engine/world evidence and authorized the full
profile against the same unchanged package. A valid capture is not itself a
passing certification result.

Decision: Record Full Campaign Debug run `seed1985_t0_p1_u1784446076`, leaf
`20260719T072739Z-97fc069d58cd427c848c83f99f39e5f9`, captured from
`2026-07-19T07:27:39.1454367Z` through `2026-07-19T07:40:09.5714410Z` under
clean harness HEAD `a5ccf36aee17a4f88d7f1c2f232ce9fc14652018`, as the rejected
red full-profile boundary for exact candidate
`partisan-rc-ee0e8add2a29-20260719T063815Z` and exact package SHA-256
`981258439b9d08866c4883471cacfe33aa373a36a667a39e8c939f285db74daf`.
The wrapper completed a mechanically valid ten-file capture with stable
candidate/package identity, envelope SHA-256
`fce4928444f15531f254ad4d7e119cf8bfe1d06e6fcb564518d2e052544d4278`,
18/0 state restoration, Phase 17 at 11/11, Phase 24 at 2/2, staged cleanup at
6/6, zero final orphans, and every cleanup/spill count at zero.

Certification nevertheless failed independently. The 685 cases ended 598 PASS,
47 WARN, 26 FAIL, 13 BLOCKED, and 1 SKIPPED. The report proved 5,630/5,695
required assertions, with 50 failed and 15 blocked. Diagnostic acceptance also
failed independently: 26 hard diagnostics classified as two approved stock,
zero approved intentional, and 24 unapproved: 22 Partisan and two runtime. An
obsolete fourteenth intentional-convoy classifier expectation demoted the 13
valid intentional-negative convoy diagnostics; nine debug respawn-race errors and two HQ arsenal
teardown errors form the remaining unapproved clusters. The portable summary is
`docs/evidence/campaign-debug/partisan-rc-ee0e8add2a29-20260719T063815Z-full-20260719T072739Z.json`,
SHA-256 `e83bc1e752ac4c1abc5cb57ce097459642e17637f6747e4edc8e7d57569c1884`.

Consequences:

- The full-profile result is rejected red even though candidate identity,
  capture integrity, state restoration, and cleanup are valid. Mechanical
  success does not override either failed acceptance surface.
- The earlier focused and corrected-canary rungs remain scoped
  `passed-noncertifying` evidence for ee0; they do not certify the full profile.
- Mark ee0 `rejected-after-runtime` and keep the candidate and package immutable.
  No further runtime evidence may attach. Any classifier, fixture, race, or
  teardown correction changes source and therefore requires a new candidate and
  a fresh package-bound evidence chain.
- Release remains `NO-GO`. Dedicated server/client, multiplayer/JIP, restart
  breadth, migration, performance, soak, canary-release, and stable-release
  gates remain independently open.

## CRI-078 - Repair Exact Diagnostic Boundaries Without Rewriting Rejected Evidence

- Status: Accepted
- Date: 2026-07-19

Context: CRI-077 retained ee0's mechanically exact but rejected full-profile
result. Its diagnostic census exposed an obsolete fourteenth intentional-convoy
expectation and a teardown-time identity lookup that could no longer reliably
find the exact HQ arsenal component.

Decision: In source commit `12f87e9`, remove the obsolete settlement diagnostic
from the approval set and retain exactly 13 intentional negatives: 9 admission,
3 corruption, and 1 watchdog line. Keep settlement proof independent, add a
negative self-test for every retained group, and raise the current classifier
self-test count from 33 to 36. Cache exact HQ arsenal prefab identity during
post-init and permit the support-station teardown early return only when both
that cache is true and the entity catalog manager is absent. Never perform
component discovery in the teardown callback.

Consequences:

- Historical summaries remain byte-for-byte authoritative at their captured
  33-check count; the release-doc validator maps that immutable runner revision
  separately from the current 36-check runner.
- The classifier cannot approve a convoy diagnostic merely because another
  proof group passed, and the removed settlement line cannot be reintroduced as
  a requirement for correct runtime behavior.
- The teardown shield stays limited to one exact prefab and cannot suppress
  errors for arbitrary support stations.
- These are source-fixed, not package-proven. A new candidate must restart every
  package-bound promotion rung, and release remains `NO-GO`.

## CRI-079 - Separate Sealed Replay From Live Admission and Bound Contact Reseating

- Status: Accepted
- Date: 2026-07-19

Context: The rejected full profile showed that compacted paid-support
confirmations lost replay after their full planning graph was archived, and a
contact-phase convoy could retain real vehicles and crew while never obtaining a
seated driver. Broadly relaxing service gates or contact reseating would violate
the exact authority and combat-stability contracts.

Decision: Commit `64d1f70` recognizes a matching sealed settlement tombstone
before checking dependencies required only for a new live confirmation. Archive
proofs must invoke that replay with null preset, economy, support-request, and
ledger services. Commit `ebaaeca` permits contact reseating only for an active,
operational, spawned, nonterminal exact convoy in one of three explicit degraded
modes or valid restore-rebind grace, with real crew/vehicle roots, living crew,
no seated driver, the existing five-second cadence, and a deadline of
`max(spawnedAt,lastRestore)+45s`.

Consequences:

- Archived replay remains read-only and cannot depend on reconstructing the
  removed planning graph or on live-service availability.
- Ordinary contact and any group outside the exact recovery predicate retain
  the no-reseat rule, preventing combat-time occupant churn.
- No new persisted schema or second operation owner is introduced.
- Static proof is necessary but insufficient; a new immutable package must
  prove archive replay and contact seating, and release remains `NO-GO`.

## CRI-080 - Publish Future Runtime Evidence From Raw Schema-2 Contracts

- Status: Proposed; pending a stationary committed tool set and passing
  producer, consumer, and Foundation self-tests
- Date: 2026-07-20

Context: Historical Schema-1 summaries remain authoritative for their captured
packages, but they are not the forward contract for independently reopening
every raw blob, ordered assertion row, and evidence-tool binding. Future
focused and corrected-canary promotion must fail closed on hidden file,
assertion, diagnostic, candidate, package, or tool drift without rewriting the
immutable historical record.

Decision: For the next immutable candidate and every later candidate, publish
focused evidence from exactly five serial, individually named packaged runs
with eight raw envelope files each. The Schema-2 focused aggregate must reopen
and rehash all 40 files, bind the candidate/package/run identities and
committed tool blobs, and prove its distinct 35/35 `aggregate-policy`
assertions. The release-doc consumer must independently reopen the external raw
tree through its explicit evidence-bundle root and rederive the aggregate. Both
producer and consumer must rederive the package SHA-256 from the exact canonical
four-file tuple set, and both must reject any additive suite-start or profile
success marker. A focused rejection receipt has precedence only when its
candidate ID and package, manifest, and ready-seal hashes all match the
authenticated candidate.

Only an accepted focused aggregate may authorize the corrected canary against
the unchanged package. The Schema-2 corrected-canary release index must derive
from the retained ten-file raw bundle and accept exactly 11 cases at 9 PASS,
1 WARN, 0 FAIL, 1 BLOCKED, and 0 SKIPPED; 91 ordered assertion rows; 87/87
certifying rows; `cleanup.player_marker.live` in
`cleanup.player_marker_completion` as the sole non-certifying warning;
`isolation.world_scope` in `cleanup.state_isolation_restore` as the sole
explicitly later-external non-certifying blocker; and the exact 18-label
zero-delta state set. Any unexpected or certification-counting blocker is red.
Its tracked index must remain byte-identical to the external index, and the
release-doc consumer must independently reopen the raw bundle and rederive
case, proof, diagnostic, cleanup, and state-diff truth.

Accepted Schema-2 evidence is immutable. A rejected later attempt records a
candidate-bound red replacement receipt or rejected index and never overwrites
accepted bytes. All Schema-1 summaries and readers remain immutable historical
evidence for their original packages; no historical result is upgraded,
rewritten, borrowed, or transferred.

Historical routing is summary-schema-aware because a pinned Schema-1 canary and
the forward Schema-2 canary may legitimately share `passed-noncertifying`.
Historical Schema-2 tool provenance is verified against the recorded Git blobs
and ancestry, without requiring a later worktree to retain those old bytes;
active evidence continues to require a stationary exact worktree.

Consequences:

- This decision remains proposed until the complete tool set is committed and
  its producer, consumer, and Foundation self-tests pass against one stationary
  checkout. No Schema-2 runtime result is claimed by this decision.
- These tooling and static contracts do not execute runtime, prove gameplay,
  certify a package, or advance Gate 1 or any later gate. The 35 aggregate-
  policy assertions are not Campaign Debug assertions.
- Campaign Schema 71 and runtime-settings Schema 24 remain unchanged. This is
  an evidence-publication contract, not a campaign/settings migration.
- A new immutable candidate must earn focused, corrected-canary, and permitted
  full-profile evidence in order. Historical evidence cannot satisfy any rung.
- Release remains `NO-GO`.
