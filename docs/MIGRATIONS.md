# Campaign Save Migrations

## Schema 16

Phase 3 convoy route state.

- `HST_CampaignState.SCHEMA_VERSION` is `16`.
- Generated routes now persist ordered `HST_RouteWaypointState` records.
- Schema-15 routes are backfilled from their legacy start, midpoint, and end
  positions during save-data migration/copy.
- Existing route start/mid/end fields remain for compatibility and are kept in
  sync with waypoint records.
- Route vehicle-safety validation is diagnostic state and can be recomputed from
  generated route waypoints.

## Schema 15

Baseline for the Phase 0/1 stabilization work.

- `HST_CampaignState.SCHEMA_VERSION` remains `15`.
- Persistence smoke-test baselines are stored in the existing persisted `HST_CampaignTaskState` array using `hst_smoke_persistence_expected`.
- Phase 1 zone composition spawn slots are generated at runtime and do not add persisted fields.
- Phase 2 capture diagnostics and notification de-dupe state are runtime-only; capture tuning moved through runtime settings schema `8` without changing campaign save fields.
