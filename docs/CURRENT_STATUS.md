# Partisan Current Status

> Generated from `docs/data/release_status.json` and `docs/data/antistasi_ce311_parity.json`. Do not hand-edit this file; run `tools/update-release-docs.ps1`.

## Release decision

**NO-GO - development alpha.** No release-candidate package is certified.

The retained candidate identity below binds its exact source HEAD, manifest, canonical four-file package index, addon identity, and validation tools. The generator verifies that the candidate source is between the audited gameplay revision and the live checkout HEAD.

## Identity

| Field | Current value |
| --- | --- |
| Status data as of | `2026-07-19T01:54:57.0935077Z` |
| Audited gameplay Git HEAD | `25dedb3e82ad516c28826830bc1e06a2d3940f53` |
| Embedded implementation identity | `32727238d74b29905c68e5a80bb5897dfdc783c0` |
| Embedded build UTC / label | `2026-07-18T16:34:38Z` / `schema71-settings24-focused-force-authority` |
| Campaign / runtime-settings schema | `71` / `24` |
| Workbench CRC | `3a399db1` |
| Release candidate / source HEAD | `partisan-rc-0e632ec4f63e-20260719T004133Z` / `0e632ec4f63eab43e8c301d0755f10193d85131f` |
| Runtime use disposition | `active-runtime-candidate` |
| Candidate manifest | `docs/evidence/release-candidates/partisan-rc-0e632ec4f63e-20260719T004133Z/candidate.json` |
| Manifest / ready-seal SHA-256 | `ea06318a8f5161f000685fe37ecab4f5c8a77d6b0e8205f502a6418e3365e76b` / `cd91e569b8a4a453dad6b0f884f22afbb36b9b5f0de629fd70b2188875e47c53` |
| Aggregate package SHA-256 | `e5d29458c33aeef9cd2b37476359acc6021fe78cf0fc74513d9a2f69ef0614dc` (sha256-manifest-v1 over the canonical four-file package index) |
| Addon GUID / revision / version | `698532771130111D` / `unpublished-local-pack` / `0.1.0-rc.20260719T004133Z.0e632ec4` |
| Workbench/tool identity | version `1.7.0.54` / SHA-256 `59ee98c352932c1aa8fb29970a377c1a9ea2f839e31d9ab072239212909d54c0` / validation CRC `3a399db1` |
| Server / client versions | `1.7.0.54` / `1.7.0.54` |

## Proof ladder

A pass never inherits upward. `partial` means some scoped evidence exists but the rung is not passed for the enabled campaign.

| Rung | Status | Honest scope |
| --- | --- | --- |
| Static/source/resource contracts | `passed` | Retained Foundation evidence is green inside the immutable candidate build for its exact source HEAD. |
| Enforce compile and configuration | `passed` | All five explicit Workbench targets are retained and green for the candidate source and canonical four-file package build. |
| Deterministic service contracts | `passed-noncertifying` | All five individually named packaged focused service suites passed against the exact active package and clean harness, with 5/0/0/0 JUnit totals, 11 fully classified hard diagnostics, zero unapproved diagnostics, 40 envelope files rehashed, and zero cleanup or spill residue. This does not certify a higher rung. |
| Native engine-world behavior | `failed` | The corrected force-authority canary remains a valid scoped pass, but the active full profile is red: 46 failed cases, 7 blocked cases, 112 failed and 14 blocked required assertions, plus 10 unapproved hard diagnostics. Mechanical capture integrity does not override certification or diagnostic failure. |
| Packaged dedicated server | `not-run` | The active replacement candidate is sealed but has not been launched through the standard dedicated-server runtime gate. |
| Multiple clients, reconnect, and JIP | `not-run` | Host, two-client, reconnect, late-join, and packet-disruption convergence are not certified. |
| Fresh-process restart and fault injection | `partial` | Selected journal, shutdown, field-vehicle, exact-QRF, counterattack, and rebuild cuts pass; the arbitrary full campaign graph and fault matrix remain open. |
| Performance and long soak | `not-run` | The reported one-second hitch and long-campaign capacity limits have no current immutable-package soak evidence. |
| Canary release | `blocked` | Blocked by every lower unproven release rung. |
| Stable certification | `blocked` | No current matrix row is certified. |

## Retained evidence

- Foundation: **passed** at 874 references for `0e632ec4f63eab43e8c301d0755f10193d85131f`.
- Workbench: **passed** at 5847 files / 11900 classes / CRC `3a399db1` for `0e632ec4f63eab43e8c301d0755f10193d85131f`.
- Active packaged focused autotests: **5/5** cases and JUnit **5/0/0/0** tests/failures/errors/skips against exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. Hard diagnostics are explicitly not free: 11 total = 10 approved stock + 1 approved intentional + 0 unapproved, with 40 envelope files rehashed and zero cleanup/spill residue. Summary: `docs/evidence/focused-autotest/partisan-rc-0e632ec4f63e-20260719T004133Z.json` / SHA-256 `961ef6b0a84c26446468b31dd7ac5120448b21a442e9a823de4ff5dc804da7f9`; clean harness `d4d8f29cda9896ce2c6a5b073dac2cbd03757700`. This exact-package deterministic-service result is non-certifying and does not color the native-engine-world rung.
- Active corrected force-authority canary: **accepted, passed-noncertifying** on exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. The focused proof passed 35/35 assertions and 87/87 counted conditions; the 33-check classifier accepted 2 hard diagnostics = 2 approved stock + 0 approved intentional + 0 unapproved. All 10 envelope files were rehashed with an exact 18-row zero-delta state diff, final orphan cleanup, and zero cleanup/spill residue. Summary: `docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-corrected-canary-20260719T012319Z.json` / SHA-256 `f47fa5f0539c0c8c6024e096f3e034699bc6bfaf656734a0a2b32c9fee7b4aa8`; clean harness `20375141f840f74316ca46e7df047fcba3e6e344`. This scoped canary does not certify Full Campaign Debug.
- Active Full Campaign Debug: **rejected, red full profile** on exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. The wrapper capture completed mechanically with stable artifacts, 10 rehashed envelope files, and zero cleanup/spill residue, while runtime acceptance remained false. Certification stayed red at 584 PASS, 49 WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED with 5561/5687 required assertions proven, 112 failed, and 14 blocked. The fail-closed classifier found 25 hard diagnostics = 2 approved stock + 13 approved intentional + 10 unapproved. Summary: `docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-full-20260719T014151Z.json` / SHA-256 `ed225ba2acb6932437af55219ff0b6ba69f4a2111880acd11c2555c875819ca7`; clean harness `27052811bb192835fc09ab3cb052b36cabad5df4`. Mechanical capture success is not certification or diagnostic acceptance.
- Historical packaged focused autotests: **5/5** cases and JUnit **5/0/0/0** tests/failures/errors/skips against prior exact candidate `partisan-rc-b8deddc4b631-20260718T213322Z`. Hard diagnostics are explicitly not free: 11 total = 10 approved stock + 1 approved intentional + 0 unapproved, with 40 envelope files rehashed and zero cleanup/spill residue. Summary: `docs/evidence/focused-autotest/partisan-rc-b8deddc4b631-20260718T213322Z.json` / SHA-256 `8bb36919f0649e0f48fad50305878ec883cf98a0021323ba1442017f1aa113b8`; harness `b3fc1e6f56d9cf8805bac1702a54e0b5284e0043`. This immutable non-certifying result does not attach to the active replacement.
- Focused force-authority profile: **35/35** cases and **87/87** counted conditions for `32727238d74b29905c68e5a80bb5897dfdc783c0`, with `CertificationPassed:false`. This is historical state-only, non-package, non-certifying evidence.
- Historical corrected force-authority canary: **rejected fail-closed** on prior exact candidate `partisan-rc-b8deddc4b631-20260718T213322Z`. The focused proof remained 35/35 assertions and 87/87 counted certification conditions, but the 33-check classifier found 3 hard diagnostics = 2 approved stock + 0 approved intentional + 1 unapproved `virtual-machine-exception`. All 10 envelope files rehashed with zero cleanup/spill residue. Summary: `docs/evidence/campaign-debug/partisan-rc-b8deddc4b631-20260718T213322Z-corrected-canary-20260719T001339Z.json` / SHA-256 `2f9656ecf6e8640f4203e231dfe531d0b5eda017619e4f0d83a3e823e66d6951`; harness `38a094fe223232145801bd60e707adf0c80c13d2`. This immutable rejection does not transfer to the active replacement.
- Historical Full Campaign Debug capture: **preliminary and unaccepted** on prior exact candidate `partisan-rc-b8deddc4b631-20260718T213322Z`: 584 PASS, 49 WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED; 5562/5688 required assertions proven, with 112 failed and 14 blocked. Candidate identity, packed mount, artifact stability, cleanup, and envelope rehash were mechanically verified, but the original wrapper missed timestamp-prefixed errors. Its corrected overlay found 3 canary diagnostics with 1 unapproved and 25 full-run diagnostics with 10 unapproved; wrapper-reported success is not acceptance. Summary: `docs/evidence/campaign-debug/partisan-rc-b8deddc4b631-20260718T213322Z.json` / SHA-256 `0198f244322fceae63bc0d345a0fa3330e52af3886c540530189054e425538f9`; capture harness `1bff1890830db08159826f63b550227aa7bb0da3`. This result does not attach to the active replacement.

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
| `STATUS-001` | `AUTH` | The active package's full-profile capture completed, but certification failed and the corrected classifier found 10 unapproved hard diagnostics. The native engine-world rung is failed; dedicated, multiplayer/JIP, restart breadth, performance, and soak remain pending. |
| `STATUS-002` | `TEST` | The active full profile is release-blocking at 584 PASS, 49 WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED with 5561/5687 required assertions proven and 10 unapproved diagnostics. Triage these exact-package failures before rerunning the full profile. |
| `STATUS-003` | `UI` | Known command-menu and modal-map defects remain open until source correction plus rendered packaged-client proof. |
| `STATUS-004` | `MOVE` | Natural sustained infantry and convoy travel, identical-waypoint suppression, and measured no-stutter behavior are not proven. |
| `STATUS-005` | `PROJ` | Campaign read-model convergence is not proven with host, two clients, reconnect, JIP, restart, and marker-cap boundaries. |
| `STATUS-006` | `PERF` | The one-second hitch and bounded-history admission cliffs remain unclosed. |
| `STATUS-007` | `MISSION` | Configured mission breadth still exceeds mission-specific CE 3.11.1 behavioral parity and proof. |
| `STATUS-008` | `SEC` | Development proof and destructive diagnostic surfaces have not yet been separated from the release package. |

## Next release-closure step

The active full-profile checkpoint is retained and release-blocking. Triage its 46 failed cases, 7 blocked cases, 112 failed required assertions, 14 blocked required assertions, and 10 unapproved diagnostics against exact package `e5d29458c33aeef9cd2b37476359acc6021fe78cf0fc74513d9a2f69ef0614dc`; rerun the full profile only after the current source fixes are sealed into a new immutable candidate. Do not attach post-capture source changes to this package's evidence chain.
