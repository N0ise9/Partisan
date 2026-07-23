# Partisan Current Status

> Generated from `docs/data/release_status.json` and `docs/data/antistasi_ce311_parity.json`. Do not hand-edit this file; run `tools/update-release-docs.ps1`.

## Release decision

**NO-GO - development alpha.** No Workshop release is certified.

Gate 1 is `in-progress` on frozen source checkpoint `b0b8a6b677092b45f1c7392edcc2d82246d27bbc`. No rung advances without a tracked, hash-bound source-evidence summary.

The retained local-validation snapshot below is historical QA evidence, not source, a publishing input, or a distributable. The generator still verifies its exact source HEAD, manifest, package index, addon identity, and validation tools. Workbench publishing and Workshop/in-game delivery are the supported release path.

## Identity

| Field | Current value |
| --- | --- |
| Status data as of | `2026-07-23T01:16:00Z` |
| Audited gameplay Git HEAD | `b0b8a6b677092b45f1c7392edcc2d82246d27bbc` |
| Gate 1 source state | `in-progress` |
| Frozen publish-source HEAD | `b0b8a6b677092b45f1c7392edcc2d82246d27bbc` |
| Publish-input tree | `11999e99c9ab417068e9b371c4ae684217299a35ecad6f2f311e9fe4b4c846f1` / 436 rows / `git-ls-tree-sha256-v1` |
| Embedded implementation identity | `7fdf3988797edeb747f5d6a6951ad0382bd93db3` |
| Embedded build UTC / label | `2026-07-21T19:36:22Z` / `schema71-settings24-gate1-release-surface` |
| Campaign / runtime-settings schema | `71` / `24` |
| Current Gate 1 Workbench CRC | `82044bce` |
| Current Gate 1 Workbench/tool identity | version `1.7.0.54` / SHA-256 `59ee98c352932c1aa8fb29970a377c1a9ea2f839e31d9ab072239212909d54c0` |
| Current Gate 1 source resource database | `f99d0974e92c20e456e774299d627714cbc9bb6ca8eda9e066828b0d7627fc77` / 84996 bytes (generated cache, not source) |
| Historical snapshot Workbench CRC | `aeddce9b` |
| Retained validation snapshot / source HEAD | `partisan-rc-5b1f2e98f931-20260721T193941Z` / `5b1f2e98f93137230e686312c6e99cea7630dae4` |
| Snapshot embedded implementation identity | `7fdf3988797edeb747f5d6a6951ad0382bd93db3` |
| Snapshot embedded build UTC / label | `2026-07-21T19:36:22Z` / `schema71-settings24-gate1-release-surface` |
| Historical validation disposition | `historical-local-qa` |
| Retained snapshot manifest | `docs/evidence/release-candidates/partisan-rc-5b1f2e98f931-20260721T193941Z/candidate.json` |
| Historical manifest / ready-seal SHA-256 | `bef040090557ca1403d6505e2bd1150452ba467225a29adc4295641f5aa3c80c` / `173434122dce60dde8ff1dc939e2d5a916094bdb31096a641850772aa9853ad3` |
| Historical snapshot package SHA-256 | `af22d6322a215dbef466e49041fc07395cbb5ed7a5951fd3e0cee5f4a101f530` (sha256-manifest-v1 over the canonical four-file package index) |
| Historical snapshot addon GUID / revision / version | `698532771130111D` / `unpublished-local-pack` / `0.1.0-rc.20260721T193941Z.5b1f2e98` |
| Historical snapshot Workbench/tool identity | version `1.7.0.54` / SHA-256 `59ee98c352932c1aa8fb29970a377c1a9ea2f839e31d9ab072239212909d54c0` / validation CRC `aeddce9b` |
| Historical snapshot server / client versions | `1.7.0.54` / `1.7.0.54` |

## Proof ladder

A pass never inherits upward. `partial` means some scoped evidence exists but the rung is not passed for the enabled campaign.

| Rung | Status | Honest scope |
| --- | --- | --- |
| Static/source/resource contracts | `passed` | Current Foundation passed at 985 references for replacement source checkpoint b0b8a6b677092b45f1c7392edcc2d82246d27bbc, with exact 436-input source/worktree identity, zero checkout .pak files, and a tracked hash-bound summary. |
| Enforce compile and configuration | `passed` | Current all-target Workbench validation passed for replacement source checkpoint b0b8a6b677092b45f1c7392edcc2d82246d27bbc on PC, XBOX_ONE, XBOX_SERIES, PS4, and PS5: 5,849 files, 12,022 classes, common CRC 82044bce, zero hard errors, and exact cleanup. This is source compilation and validation for the normal Workshop publishing path; no generated archive is a source input. |
| Deterministic service contracts | `not-run` | The five source-native focused suites are pending for replacement checkpoint b0b8a6b677092b45f1c7392edcc2d82246d27bbc after Foundation and Workbench. Prior-checkpoint deterministic-service evidence does not transfer. |
| Native engine-world behavior | `not-run` | The source-native force-authority canary and Full Campaign Debug are pending for replacement checkpoint b0b8a6b677092b45f1c7392edcc2d82246d27bbc after the earlier Gate 1 rungs. The rejected 5717d05 checkpoint remains immutable history and does not transfer. |
| Workshop-installed dedicated server | `not-run` | A final Workbench-published, Workshop-downloaded revision has not entered the standard dedicated-server certification gate. |
| Multiple clients, reconnect, and JIP | `not-run` | Host, two-client, reconnect, late-join, and packet-disruption convergence are not certified. |
| Fresh-process restart and fault injection | `partial` | Selected journal, shutdown, field-vehicle, exact-QRF, counterattack, and rebuild cuts pass; the arbitrary full campaign graph and fault matrix remain open. |
| Performance and long soak | `not-run` | The reported one-second hitch and long-campaign capacity limits have no current Workshop-installed soak evidence. |
| Canary release | `blocked` | Blocked until the frozen source checkpoint passes Gate 1 and a final revision is published through Workbench to Workshop. |
| Stable certification | `blocked` | No current matrix row is certified. Source-native Full Campaign Debug and later Workshop-installed external gates remain open. |

## Retained evidence

- Historical snapshot Foundation: **passed** at 985 references for `5b1f2e98f93137230e686312c6e99cea7630dae4`. It does not advance the revised Gate 1.
- Historical snapshot Workbench: **passed** at 5849 files / 12022 classes / CRC `aeddce9b` for `5b1f2e98f93137230e686312c6e99cea7630dae4`. It does not advance the revised Gate 1.
- Current Gate 1 source evidence: Foundation **passed**; all-target Workbench **passed**; five-suite focused **pending**; force-authority canary **pending**; Full Campaign Debug **pending**.
- Historical local-package QA: snapshot `partisan-rc-5b1f2e98f931-20260721T193941Z`, its manifest/seal, release-surface/runtime-retention pair, and rejected focused batches remain immutable forensic evidence. They are not active Gate 1 or Workshop release authority and are not required to match current source-workflow tool bytes.
- Focused force-authority profile: **35/35** cases and **87/87** counted conditions for `32727238d74b29905c68e5a80bb5897dfdc783c0`, with `CertificationPassed:false`. This is historical state-only, non-package, non-certifying evidence.
- Historical packaged focused autotests: **5/5** cases and JUnit **5/0/0/0** tests/failures/errors/skips against prior exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. Hard diagnostics are explicitly not free: 11 total = 10 approved stock + 1 approved intentional + 0 unapproved, with 40 envelope files rehashed and zero cleanup/spill residue. Summary: `docs/evidence/focused-autotest/partisan-rc-0e632ec4f63e-20260719T004133Z.json` / SHA-256 `961ef6b0a84c26446468b31dd7ac5120448b21a442e9a823de4ff5dc804da7f9`; harness `d4d8f29cda9896ce2c6a5b073dac2cbd03757700`. This immutable non-certifying result does not attach to the retained historical validation snapshot.
- Historical corrected force-authority canary: **accepted non-certifying** on prior exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. The focused proof passed 35/35 assertions and 87/87 counted certification conditions. The 33-check classifier found 2 hard diagnostics = 2 approved stock + 0 approved intentional + 0 unapproved. All 10 envelope files rehashed with zero cleanup/spill residue. Summary: `docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-corrected-canary-20260719T012319Z.json` / SHA-256 `f47fa5f0539c0c8c6024e096f3e034699bc6bfaf656734a0a2b32c9fee7b4aa8`; harness `20375141f840f74316ca46e7df047fcba3e6e344`. This immutable scoped acceptance does not transfer to the retained historical validation snapshot.
- Historical Full Campaign Debug: **rejected, red full profile** on prior exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. The wrapper capture completed mechanically with stable artifacts, 10 rehashed envelope files, and zero cleanup/spill residue, while runtime acceptance remained false. Certification stayed red at 584 PASS, 49 WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED with 5561/5687 required assertions proven, 112 failed, and 14 blocked. The fail-closed classifier found 25 hard diagnostics = 2 approved stock + 13 approved intentional + 10 unapproved. Summary: `docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-full-20260719T014151Z.json` / SHA-256 `ed225ba2acb6932437af55219ff0b6ba69f4a2111880acd11c2555c875819ca7`; clean harness `27052811bb192835fc09ab3cb052b36cabad5df4`. Mechanical capture success is not certification or diagnostic acceptance, and this immutable rejection does not attach to the retained historical validation snapshot.
- Historical packaged focused autotests: **5/5** cases and JUnit **5/0/0/0** tests/failures/errors/skips against prior exact candidate `partisan-rc-e11e7ea88a44-20260719T040154Z`. Hard diagnostics are explicitly not free: 11 total = 10 approved stock + 1 approved intentional + 0 unapproved, with 40 envelope files rehashed and zero cleanup/spill residue. Summary: `docs/evidence/focused-autotest/partisan-rc-e11e7ea88a44-20260719T040154Z.json` / SHA-256 `9ddade1cb86a209acf4aae02ded6f1a7713fe1e25ba577ae00ef1980e3de149a`; harness `b1940f241e28f163202807385f7140f048921447`. This immutable non-certifying result does not attach to the retained historical validation snapshot.
- Historical corrected force-authority canary: **rejected, focused proof failed** on prior exact candidate `partisan-rc-e11e7ea88a44-20260719T040154Z`. The focused proof passed 33/35 assertions and 85/87 counted conditions. The classifier remained valid at 2 approved and 0 unapproved hard diagnostics. All 10 envelope files rehashed with zero cleanup/spill residue. Summary: `docs/evidence/campaign-debug/partisan-rc-e11e7ea88a44-20260719T040154Z-corrected-canary-20260719T050302Z.json` / SHA-256 `af0aca25a84d8f757dbba8010950a658ce09937aa4048c35b2e372f1183eec69`; harness `937c86c5d2259a9da270ea76371001ac1d4c6eed`. This immutable rejection does not transfer to the retained historical validation snapshot.
- Historical Full Campaign Debug: **stopped and not run** for prior exact candidate `partisan-rc-e11e7ea88a44-20260719T040154Z`; its `rejected-after-corrected-canary` contract ended the chain at the rejected corrected canary.
- Historical packaged focused autotests: **5/5** cases and JUnit **5/0/0/0** tests/failures/errors/skips against prior exact candidate `partisan-rc-ee0e8add2a29-20260719T063815Z`. Hard diagnostics are explicitly not free: 11 total = 10 approved stock + 1 approved intentional + 0 unapproved, with 40 envelope files rehashed and zero cleanup/spill residue. Summary: `docs/evidence/focused-autotest/partisan-rc-ee0e8add2a29-20260719T063815Z.json` / SHA-256 `0a8fcfc5ca739ff261be644cdfcb02311e4ef967c374f093e2963e4b1374800b`; harness `273ed14ba8526259c8b0d248177fa53b59ade683`. This immutable non-certifying result does not attach to the retained historical validation snapshot.
- Historical corrected force-authority canary: **accepted non-certifying** on prior exact candidate `partisan-rc-ee0e8add2a29-20260719T063815Z`. The focused proof passed 35/35 assertions and 87/87 counted certification conditions. The 33-check classifier found 2 hard diagnostics = 2 approved stock + 0 approved intentional + 0 unapproved. All 10 envelope files rehashed with zero cleanup/spill residue. Summary: `docs/evidence/campaign-debug/partisan-rc-ee0e8add2a29-20260719T063815Z-corrected-canary-20260719T071408Z.json` / SHA-256 `f3521fdee20811efd37a260d23498aad43d75435cc01331022ffb8565df34b42`; harness `4f8d7e2d7a39896737fd6754060523bf852c5fa8`. This immutable scoped acceptance does not transfer to the retained historical validation snapshot.
- Historical Full Campaign Debug: **rejected, red full profile** on prior exact candidate `partisan-rc-ee0e8add2a29-20260719T063815Z`. The wrapper capture completed mechanically with stable artifacts, 10 rehashed envelope files, and zero cleanup/spill residue, while runtime acceptance remained false. Certification stayed red at 598 PASS, 47 WARN, 26 FAIL, 13 BLOCKED, and 1 SKIPPED with 5630/5695 required assertions proven, 50 failed, and 15 blocked. The fail-closed classifier found 26 hard diagnostics = 2 approved stock + 0 approved intentional + 24 unapproved. Summary: `docs/evidence/campaign-debug/partisan-rc-ee0e8add2a29-20260719T063815Z-full-20260719T072739Z.json` / SHA-256 `e83bc1e752ac4c1abc5cb57ce097459642e17637f6747e4edc8e7d57569c1884`; clean harness `a5ccf36aee17a4f88d7f1c2f232ce9fc14652018`. Mechanical capture success is not certification or diagnostic acceptance, and this immutable rejection does not attach to the retained historical validation snapshot.

## Specification coverage

- CE 3.11.1 product contract rows: **24** (development-only 1; legacy 3; missing 2; partial 18).
- Configured/runtime mission rows: **39/39**, all mapped to a product contract row.
- Routed command/action IDs: **177**, exact source/manifest set match and all mapped.
- Concrete contextual action classes: **17**, exact source/manifest set match and all mapped.
- Product specification: `docs/ANTISTASI_CE311_PARITY_MATRIX.md`.

Coverage means the surface is named and classified. It does not mean the behavior is exact or certified.

## Active blockers

| ID | Category | Blocker |
| --- | --- | --- |
| `STATUS-001` | `AUTH` | Gate 1 authority is now the clean addon source checkpoint plus its Workbench and source-native evidence. The external package snapshot and every package-bound result are retained historical/local QA only; they are not source or Workshop release authority. |
| `STATUS-002` | `TEST` | Foundation and all-target Workbench validation pass for replacement source checkpoint b0b8a6b677092b45f1c7392edcc2d82246d27bbc. Run the five source-native focused suites next, then the force-authority canary and Full Campaign Debug. No generated .pak is part of source or this gate. |
| `STATUS-003` | `UI` | Known command-menu and modal-map defects remain open until source correction plus rendered Workshop-installed client proof. |
| `STATUS-004` | `MOVE` | Natural sustained infantry and convoy travel, identical-waypoint suppression, and measured no-stutter behavior are not proven. |
| `STATUS-005` | `PROJ` | Campaign read-model convergence is not proven with host, two clients, reconnect, JIP, restart, and marker-cap boundaries. |
| `STATUS-006` | `PERF` | The one-second hitch and bounded-history admission cliffs remain unclosed. |
| `STATUS-007` | `MISSION` | Configured mission breadth still exceeds mission-specific CE 3.11.1 behavioral parity and proof. |

## Next release-closure step

The local package snapshot `partisan-rc-5b1f2e98f931-20260721T193941Z` is retained only as historical QA evidence and is not release authority. Gate 1 is frozen at source checkpoint `b0b8a6b677092b45f1c7392edcc2d82246d27bbc`; run the five source-native focused suites next, followed by the force-authority canary and Full Campaign Debug. No generated package belongs in source. Workbench publishes an accepted final revision to Workshop, and the game downloads it.
