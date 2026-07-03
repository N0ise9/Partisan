# HST Campaign Debug Verification Audit

Date: 2026-07-03


Verdict: not fully implemented.

The current `Run Campaign Debug` implementation is a useful certification scaffold. It is no longer just a string/report smoke runner: it has typed results, deterministic debug run prefixes, artifacts, status/cancel/cleanup commands, stronger bootstrap/preflight/HQ/economy assertions, convoy readiness/progress probing, a POW captive free/follow probe, Phase 24 pacing/end assertions, and mission cleanup checks.

It does not yet satisfy the full pasted contract for a complete one-button in-game verification suite. Large areas remain legacy string-wrapped smoke checks or are not represented at all.

## Implemented Evidence

- Admin command wiring exists for start/status/cancel/cleanup in `Scripts/Game/HST/Services/HST_CommandUIService.c`.
- The coordinator exposes `RequestAdminRunCampaignDebug`, `RequestAdminCampaignDebugStatus`, `RequestAdminCancelCampaignDebug`, and `RequestAdminCleanupCampaignDebug`.
- The runner remains sequenced through bootstrap, baseline, HQ, economy/support, early mechanics, mission sweep, phase smoke, final report, and completion.
- Run profiles are accepted as `smoke`, `physical`, and `full`: `smoke` skips the long early/mission/phase sweeps, `physical` keeps early mechanics plus the mission/physical sweep and skips late phase smoke, and `full` preserves the complete sequence.
- Typed result classes exist in `Scripts/Game/HST/Data/HST_CampaignDebugResult.c`.
- Artifacts write to `$profile:h-istasi/debug` as JSON, summary text, and state-diff text.
- Runs now record a deterministic debug marker prefix, mission prefix, and entity tag in JSON, summary, state-diff, and status output.
- Run start removes stale `hst_debug_`-prefixed persisted state before capturing start counts. Forced debug-started missions are retagged before objective/runtime initialization so derived objective/runtime/asset/group/marker records inherit the run prefix.
- Debug-created player support, Phase 19 support smoke records, and Phase 17/18/22 enemy-order seeds are retagged with the current run prefix before marker refresh/report output; support request cases assert the prefix directly.
- Bootstrap records typed assertions for server authority, debug actor access, active campaign repair, HQ state, Petros state, teleport, and player presence.
- Preflight records typed assertions for key services, mission registry count/uniqueness/runtime/duration, compatible debug target zones, default faction/civilian prefab resource resolution, runtime-selected mission prop/vehicle prefab resolution, runtime waypoint prefab resolution, zone graph counts, and physical-war setting.
- HQ runtime records typed assertions for runtime flag, tracked Petros/cache/arsenal/tent/spawn-point entity count, Petros/cache/arsenal/tent/spawn-point runtime entity keys and positions, arsenal usability, HQ marker, active admin command-menu campaign-debug controls, command coverage, and player position.
- Economy records typed exact-delta assertions for resource awards and training.
- Stage 3 support requests now clear prior player support, call the real support command, assert the created support request record type, faction, target zone/position, ETA, money cost, status, marker publication/pending state, controlled ETA progression, and QRF/search ground-support physicalization into linked active groups before cleaning the debug probe.
- Civilian aid now records typed money/support/heat assertions, and support cancellation now seeds and cancels a real player support request by ID.
- Phase 20/21 smoke now records typed town support, wanted heat, eligibility, clear-heat, undercover apply, weapon/vehicle compromise, roadblock/police scan, and clear-heat assertions.
- Phase 18/19 smoke now records typed enemy-order/support assertions for debug-prefixed records, order/support type, faction/player-requested policy, target validity, resource cost fields, open-order resolution, and forced-ETA inbound support evidence. Phase 18 also seeds a background-war state, runs the normal commander tick threshold, and asserts occupier/invader order creation, POI targets, duplicate guards, source/target positions, context-appropriate order types, and enemy pool spending.
- Phase 24 smoke now records typed campaign pacing/end assertions for early/mid/late seeded resource profiles, control percent, FIA/enemy zone counts, max enemy pool pressure, forced victory phase/reason/control metadata, and forced loss threshold metadata.
- Convoy physical probing asserts vehicle asset counts, spawned vehicle entities, crew groups, alive crew, seated drivers, mobile vehicles, route assignment, waypoint assignment, readiness, progress sample presence, and hard-stuck count.
- POW/captive probing uses real `mission_captive_extract` and `mission_captive_follow` interactions and asserts freed/following carrier state.
- Mission cleanup checks active mission status, unresolved assets, mission-owned groups, and linked markers.
- The state-diff artifact snapshots start/end objectives, runtime vehicles, mission assets, active groups, support requests, enemy orders, markers, garage vehicles, arsenal items, civilian zones, and undercover records.
- Admin cleanup and run completion now record typed prefixed-state cleanup cases for missions, objectives, runtime entities, mission assets, active groups, runtime vehicles, QRFs, support requests, enemy orders, map markers, and campaign tasks, then rebuild campaign markers when marker-backed state changed.
- Final completion now records a typed cleanup leak snapshot for active missions, player support, enemy orders, active groups, markers, current/early debug mission IDs, and remaining `hst_debug_`-prefixed persisted records.
- Non-legacy typed cases now emit a `post_case_cleanup.*` leak probe while the runner is active. The probe allows the mission intentionally under test, preserves pre-existing active mission IDs from the run-start snapshot, and asserts unexpected active missions, orphan mission assets, orphan active groups, orphan linked markers, and backing states missing markers.
- The summary artifact now includes feature, mission, physical AI, and cleanup matrices built from typed case results, plus failure inspection command hints for non-pass cases.

## Not Fully Implemented

- HQ checks now prove tracked runtime entity keys and positions for Petros/cache/arsenal/tent/spawn-point after rebuild and verify active admin command-menu campaign-debug controls, but they do not scan the wider world for duplicate HQ entities.
- Debug cleanup is deterministic for persisted state and marker records, but it still does not perform a generic world scan for arbitrary untracked physical entities by tag.
- Stage 3 support/civilian/undercover coverage is still partial: it now asserts request records, controlled ETA progression, QRF/search physicalization, support cancellation, town aid deltas, and undercover phase transitions, but it does not prove support group movement/arrival, civilian physical population/faction/vehicle behavior, or town flip behavior.
- Early mechanics are mostly still report/action wrappers. Generated content, zone activation spawn/cleanup, civilian aid bounds, support cancellation cleanup, garage/vehicle/loadout action tests, and UI coverage are not upgraded to hard typed assertions.
- The all-mission sweep does not run primitive-specific physical probes for `kill_hvt`, `hold_area`, `clear_area`, `destroy_target`, `recover_cargo`, `deliver_supplies`, or most `rescue_extract` cases. Admin completion is still the common mission end path.
- Convoy probing uses existing readiness/progress status, but it does not actively sample positions every few seconds over a movement window or prove phase history from staging to moving to contact to arrival/elimination.
- The POW/captive probe verifies free/follow state once; it does not prove distance decreases over time, boarding/transport behavior, extraction completion, alive captive count, or reward/support/HR deltas.
- Phase 14-24 smoke steps remain partly legacy string classifications outside the newly typed Phase 18/19 enemy/support, Phase 20/21 civilian/undercover, and Phase 24 pacing/end probes. Arsenal, garage/source vehicles, garrisons/training, capture/counterattack physical advance, HQ threat/Defend Petros, and UI/markers still need stronger typed assertions. Phase 24 does not yet prove long-window escalation pressure or post-end service inactivity.
- Background war and commander target testing is folded into the Phase 18 typed smoke step rather than implemented as a separate stage. It verifies command selection/resource spending for one controlled tick, but it does not prove physical group movement/arrival or long-running occupier-vs-invader campaign dynamics.
- Quantitative war level escalation/aggression pressure tests over controlled windows are not implemented; Phase 18 now proves one seeded commander tick and Phase 24 checks seeded profiles/max enemy pools only.
- Counterattack physical spawn/advance/wave tests are not implemented.
- Player render-bubble tests for far/near/leave/mission-assets/convoy-expired behavior are not implemented.
- Persistence roundtrip/temp-restore tests for active missions, field vehicles, garage/cargo, undercover, and civilian state are not implemented. Real restart/multiclient soak remains a warning gap.
- Final artifacts include the requested summary matrix sections, but those matrices still expose partial coverage because many underlying cases remain legacy string wrappers.
- Cleanup leak checks now run after non-legacy typed cases, but legacy string-wrapped smoke cases still need migration before every major case has equivalent cleanup evidence. Prefixed persisted state cleanup exists, but arbitrary untracked world-entity deletion by tag remains outside the current service boundary.
- Stall detection and timeout evidence dumps are not implemented for physical tests.

## Validation Run

- `git diff --check` passed, with only CRLF warnings reported by Git.
- Basic brace counts matched for the touched script files.
- `tools/validate-foundation.ps1` passed.
