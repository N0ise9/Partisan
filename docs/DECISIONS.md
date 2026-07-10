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
