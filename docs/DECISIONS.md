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
