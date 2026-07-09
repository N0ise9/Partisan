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
- Certification evidence must identify its build, scenario, isolation method,
  save/migration boundary, and result.
- If isolation or restoration cannot be proven, the runtime certification gate
  remains open rather than risking persistent state.
