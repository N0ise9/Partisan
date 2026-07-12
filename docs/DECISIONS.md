# h-istasi Decision Log

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
- Schema 60 is the latest stamped source/Workbench checkpoint under label
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
