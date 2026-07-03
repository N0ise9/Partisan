# HST Campaign Debug Verification Audit

Date: 2026-07-03


Verdict: not fully implemented.

The current `Run Campaign Debug` implementation is a useful certification scaffold. It is no longer just a string/report smoke runner: it has typed results, artifacts, status/cancel/cleanup commands, stronger bootstrap/preflight/HQ/economy assertions, convoy readiness/progress probing, a POW captive free/follow probe, and mission cleanup checks.

It does not yet satisfy the full pasted contract for a complete one-button in-game verification suite. Large areas remain legacy string-wrapped smoke checks or are not represented at all.

## Implemented Evidence

- Admin command wiring exists for start/status/cancel/cleanup in `Scripts/Game/HST/Services/HST_CommandUIService.c`.
- The coordinator exposes `RequestAdminRunCampaignDebug`, `RequestAdminCampaignDebugStatus`, `RequestAdminCancelCampaignDebug`, and `RequestAdminCleanupCampaignDebug`.
- The runner remains sequenced through bootstrap, baseline, HQ, economy/support, early mechanics, mission sweep, phase smoke, final report, and completion.
- Typed result classes exist in `Scripts/Game/HST/Data/HST_CampaignDebugResult.c`.
- Artifacts write to `$profile:h-istasi/debug` as JSON, summary text, and state-diff text.
- Bootstrap records typed assertions for server authority, debug actor access, active campaign repair, HQ state, Petros state, teleport, and player presence.
- Preflight records typed assertions for key services, mission registry count/uniqueness/runtime/duration, compatible debug target zones, default faction/civilian prefab resource resolution, zone graph counts, and physical-war setting.
- HQ runtime records typed assertions for runtime flag, Petros state/position, arsenal state/position, HQ marker, and player position.
- Economy records typed exact-delta assertions for resource awards and training.
- Stage 3 support requests now clear prior player support, call the real support command, and assert the created support request record type, faction, target zone/position, ETA, money cost, status, and marker publication/pending state.
- Civilian aid now records typed money/support/heat assertions, and support cancellation now seeds and cancels a real player support request by ID.
- Phase 20/21 smoke now records typed town support, wanted heat, eligibility, clear-heat, undercover apply, weapon/vehicle compromise, roadblock/police scan, and clear-heat assertions.
- Convoy physical probing asserts vehicle asset counts, spawned vehicle entities, crew groups, alive crew, seated drivers, mobile vehicles, route assignment, waypoint assignment, readiness, progress sample presence, and hard-stuck count.
- POW/captive probing uses real `mission_captive_extract` and `mission_captive_follow` interactions and asserts freed/following carrier state.
- Mission cleanup checks active mission status, unresolved assets, mission-owned groups, and linked markers.
- The state-diff artifact snapshots start/end objectives, runtime vehicles, mission assets, active groups, support requests, enemy orders, markers, garage vehicles, arsenal items, civilian zones, and undercover records.
- Final completion now records a typed cleanup leak snapshot for active missions, player support, enemy orders, active groups, markers, and current/early debug mission IDs.
- The summary artifact now includes feature, mission, physical AI, and cleanup matrices built from typed case results, plus failure inspection command hints for non-pass cases.

## Not Fully Implemented

- Deterministic debug marker/entity prefixes and old debug-only spawned entity cleanup are not implemented.
- Preflight still does not verify mission prop prefabs or waypoint prefabs because those resources are selected by runtime primitive services rather than explicit mission definition fields.
- HQ checks do not prove physical Petros/arsenal entity IDs, stale entity removal/reuse, duplicate prevention, spawn point existence, or command-menu availability while the run is active.
- Stage 3 support/civilian/undercover coverage is still partial: it now asserts request records, support cancellation, town aid deltas, and undercover phase transitions, but it does not prove support physicalization, ETA progression over ticks, civilian physical population/faction/vehicle behavior, or town flip behavior.
- Early mechanics are mostly still report/action wrappers. Generated content, zone activation spawn/cleanup, civilian aid bounds, support cancellation cleanup, garage/vehicle/loadout action tests, and UI coverage are not upgraded to hard typed assertions.
- The all-mission sweep does not run primitive-specific physical probes for `kill_hvt`, `hold_area`, `clear_area`, `destroy_target`, `recover_cargo`, `deliver_supplies`, or most `rescue_extract` cases. Admin completion is still the common mission end path.
- Convoy probing uses existing readiness/progress status, but it does not actively sample positions every few seconds over a movement window or prove phase history from staging to moving to contact to arrival/elimination.
- The POW/captive probe verifies free/follow state once; it does not prove distance decreases over time, boarding/transport behavior, extraction completion, alive captive count, or reward/support/HR deltas.
- Phase 14-24 smoke steps remain mostly legacy string classifications outside the newly typed Phase 20/21 civilian/undercover probes. Arsenal, garage/source vehicles, garrisons/training, capture/counterattack, enemy commander orders, support physicalization, HQ threat/Defend Petros, UI/markers, campaign pacing, victory, and loss still need typed assertions.
- Background war and commander target testing is not implemented as a dedicated typed stage.
- War level escalation/aggression pressure tests are not implemented.
- Counterattack physical spawn/advance/wave tests are not implemented.
- Player render-bubble tests for far/near/leave/mission-assets/convoy-expired behavior are not implemented.
- Persistence roundtrip/temp-restore tests for active missions, field vehicles, garage/cargo, undercover, and civilian state are not implemented. Real restart/multiclient soak remains a warning gap.
- Final artifacts include the requested summary matrix sections, but those matrices still expose partial coverage because many underlying cases remain legacy string wrappers.
- Cleanup leak checks are not run after every major case; current coverage is selected support cleanup, mission cleanup checks, and a final run-level leak snapshot.
- Stall detection and timeout evidence dumps are not implemented for physical tests.
- Run profiles (`smoke`, `physical`, `full`) are not implemented; the runner has a fixed `full` profile label.

## Validation Run

- `git diff --check` passed, with only CRLF warnings reported by Git.
- Basic brace counts matched for the touched script files.
- `tools/validate-foundation.ps1` passed.
