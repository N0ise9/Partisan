# Partisan Current Status

> Generated from `docs/data/release_status.json` and `docs/data/antistasi_ce311_parity.json`. Do not hand-edit this file; run `tools/update-release-docs.ps1`.

## Release decision

**NO-GO - development alpha.** No release-candidate package is certified.

The retained candidate identity below binds its exact source HEAD, manifest, canonical four-file package index, addon identity, and validation tools. The generator verifies that the candidate source is between the audited gameplay revision and the live checkout HEAD.

## Identity

| Field | Current value |
| --- | --- |
| Status data as of | `2026-07-19T04:04:14.5724744Z` |
| Audited gameplay Git HEAD | `25dedb3e82ad516c28826830bc1e06a2d3940f53` |
| Embedded implementation identity | `32727238d74b29905c68e5a80bb5897dfdc783c0` |
| Embedded build UTC / label | `2026-07-18T16:34:38Z` / `schema71-settings24-focused-force-authority` |
| Campaign / runtime-settings schema | `71` / `24` |
| Workbench CRC | `e4cde465` |
| Release candidate / source HEAD | `partisan-rc-e11e7ea88a44-20260719T040154Z` / `e11e7ea88a44ea07d7a81c0b4009f029f0b297e1` |
| Runtime use disposition | `active-runtime-candidate` |
| Candidate manifest | `docs/evidence/release-candidates/partisan-rc-e11e7ea88a44-20260719T040154Z/candidate.json` |
| Manifest / ready-seal SHA-256 | `daed6876ce839a7fc6551257e4a4dd9bb0c92772c7e2d07be595acddde19e714` / `0ca7a5e2fbe6bf298baa542250cc7b47bf2b135a5382e032fc5febdddf579acc` |
| Aggregate package SHA-256 | `75b61eb19513de00e56a43ad3778885f89a7497c0eebe4d870bf3b11e62a0dad` (sha256-manifest-v1 over the canonical four-file package index) |
| Addon GUID / revision / version | `698532771130111D` / `unpublished-local-pack` / `0.1.0-rc.20260719T040154Z.e11e7ea8` |
| Workbench/tool identity | version `1.7.0.54` / SHA-256 `59ee98c352932c1aa8fb29970a377c1a9ea2f839e31d9ab072239212909d54c0` / validation CRC `e4cde465` |
| Server / client versions | `1.7.0.54` / `1.7.0.54` |

## Proof ladder

A pass never inherits upward. `partial` means some scoped evidence exists but the rung is not passed for the enabled campaign.

| Rung | Status | Honest scope |
| --- | --- | --- |
| Static/source/resource contracts | `passed` | Retained Foundation evidence is green inside the immutable candidate build for its exact source HEAD. |
| Enforce compile and configuration | `passed` | All five explicit Workbench targets are retained and green for the candidate source and canonical four-file package build. |
| Deterministic service contracts | `not-run` | The five packaged focused service suites have not run against the active replacement package; prior-package results remain historical and do not transfer. |
| Native engine-world behavior | `not-run` | Neither the corrected force-authority canary nor Full Campaign Debug has run against the active replacement package; prior-package results remain historical and do not transfer. |
| Packaged dedicated server | `not-run` | The active replacement candidate is sealed but has not been launched through the standard dedicated-server runtime gate. |
| Multiple clients, reconnect, and JIP | `not-run` | Host, two-client, reconnect, late-join, and packet-disruption convergence are not certified. |
| Fresh-process restart and fault injection | `partial` | Selected journal, shutdown, field-vehicle, exact-QRF, counterattack, and rebuild cuts pass; the arbitrary full campaign graph and fault matrix remain open. |
| Performance and long soak | `not-run` | The reported one-second hitch and long-campaign capacity limits have no current immutable-package soak evidence. |
| Canary release | `blocked` | Blocked by every lower unproven release rung. |
| Stable certification | `blocked` | No current matrix row is certified. |

## Retained evidence

- Foundation: **passed** at 874 references for `e11e7ea88a44ea07d7a81c0b4009f029f0b297e1`.
- Workbench: **passed** at 5848 files / 11901 classes / CRC `e4cde465` for `e11e7ea88a44ea07d7a81c0b4009f029f0b297e1`.
- Active packaged focused autotests: **not run** for replacement candidate `partisan-rc-e11e7ea88a44-20260719T040154Z`; no prior-package result transfers to this package.
- Active Campaign Debug: **not run** for replacement candidate `partisan-rc-e11e7ea88a44-20260719T040154Z`; the corrected canary follows only after the packaged focused set is accepted.
- Historical packaged focused autotests: **5/5** cases and JUnit **5/0/0/0** tests/failures/errors/skips against prior exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. Hard diagnostics are explicitly not free: 11 total = 10 approved stock + 1 approved intentional + 0 unapproved, with 40 envelope files rehashed and zero cleanup/spill residue. Summary: `docs/evidence/focused-autotest/partisan-rc-0e632ec4f63e-20260719T004133Z.json` / SHA-256 `961ef6b0a84c26446468b31dd7ac5120448b21a442e9a823de4ff5dc804da7f9`; harness `d4d8f29cda9896ce2c6a5b073dac2cbd03757700`. This immutable non-certifying result does not attach to the active replacement.
- Focused force-authority profile: **35/35** cases and **87/87** counted conditions for `32727238d74b29905c68e5a80bb5897dfdc783c0`, with `CertificationPassed:false`. This is historical state-only, non-package, non-certifying evidence.
- Historical corrected force-authority canary: **accepted non-certifying** on prior exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. The focused proof passed 35/35 assertions and 87/87 counted certification conditions. The 33-check classifier found 2 hard diagnostics = 2 approved stock + 0 approved intentional + 0 unapproved. All 10 envelope files rehashed with zero cleanup/spill residue. Summary: `docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-corrected-canary-20260719T012319Z.json` / SHA-256 `f47fa5f0539c0c8c6024e096f3e034699bc6bfaf656734a0a2b32c9fee7b4aa8`; harness `20375141f840f74316ca46e7df047fcba3e6e344`. This immutable scoped acceptance does not transfer to the active replacement.
- Historical Full Campaign Debug: **rejected, red full profile** on prior exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. The wrapper capture completed mechanically with stable artifacts, 10 rehashed envelope files, and zero cleanup/spill residue, while runtime acceptance remained false. Certification stayed red at 584 PASS, 49 WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED with 5561/5687 required assertions proven, 112 failed, and 14 blocked. The fail-closed classifier found 25 hard diagnostics = 2 approved stock + 13 approved intentional + 10 unapproved. Summary: `docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-full-20260719T014151Z.json` / SHA-256 `ed225ba2acb6932437af55219ff0b6ba69f4a2111880acd11c2555c875819ca7`; clean harness `27052811bb192835fc09ab3cb052b36cabad5df4`. Mechanical capture success is not certification or diagnostic acceptance, and this immutable rejection does not attach to the active replacement.

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
| `STATUS-001` | `AUTH` | The active replacement package is sealed with green Foundation and all-target Workbench evidence, but its packaged focused set, corrected canary, and Full Campaign Debug have not run. The deterministic-service and native engine-world rungs are therefore not-run; dedicated, multiplayer/JIP, restart breadth, performance, and soak remain pending. |
| `STATUS-002` | `TEST` | Run the exact active package through the five focused suites in canonical order, accept the corrected force-authority canary, then run Full Campaign Debug. The prior package's red full-profile result and all prior scoped passes are historical only. |
| `STATUS-003` | `UI` | Known command-menu and modal-map defects remain open until source correction plus rendered packaged-client proof. |
| `STATUS-004` | `MOVE` | Natural sustained infantry and convoy travel, identical-waypoint suppression, and measured no-stutter behavior are not proven. |
| `STATUS-005` | `PROJ` | Campaign read-model convergence is not proven with host, two clients, reconnect, JIP, restart, and marker-cap boundaries. |
| `STATUS-006` | `PERF` | The one-second hitch and bounded-history admission cliffs remain unclosed. |
| `STATUS-007` | `MISSION` | Configured mission breadth still exceeds mission-specific CE 3.11.1 behavioral parity and proof. |
| `STATUS-008` | `SEC` | Development proof and destructive diagnostic surfaces have not yet been separated from the release package. |

## Next release-closure step

Run the individually named packaged focused service suites next against active replacement `partisan-rc-e11e7ea88a44-20260719T040154Z`, manifest `docs/evidence/release-candidates/partisan-rc-e11e7ea88a44-20260719T040154Z/candidate.json`, and aggregate package SHA-256 `75b61eb19513de00e56a43ad3778885f89a7497c0eebe4d870bf3b11e62a0dad`. If that exact-package set is accepted, run the corrected force-authority canary next; do not transfer the historical candidate's pass or rejection into either gate.
