# Partisan Current Status

> Generated from `docs/data/release_status.json` and `docs/data/antistasi_ce311_parity.json`. Do not hand-edit this file; run `tools/update-release-docs.ps1`.

## Release decision

**NO-GO - development alpha.** No release-candidate package is certified.

The retained candidate identity below binds its exact source HEAD, manifest, canonical four-file package index, addon identity, and validation tools. The generator verifies that the candidate source is between the audited gameplay revision and the live checkout HEAD.

## Identity

| Field | Current value |
| --- | --- |
| Status data as of | `2026-07-22T08:25:23Z` |
| Audited gameplay Git HEAD | `25dedb3e82ad516c28826830bc1e06a2d3940f53` |
| Embedded implementation identity | `7fdf3988797edeb747f5d6a6951ad0382bd93db3` |
| Embedded build UTC / label | `2026-07-21T19:36:22Z` / `schema71-settings24-gate1-release-surface` |
| Campaign / runtime-settings schema | `71` / `24` |
| Workbench CRC | `aeddce9b` |
| Release candidate / source HEAD | `partisan-rc-5b1f2e98f931-20260721T193941Z` / `5b1f2e98f93137230e686312c6e99cea7630dae4` |
| Candidate embedded implementation identity | `7fdf3988797edeb747f5d6a6951ad0382bd93db3` |
| Candidate embedded build UTC / label | `2026-07-21T19:36:22Z` / `schema71-settings24-gate1-release-surface` |
| Runtime use disposition | `active-runtime-candidate` |
| Candidate manifest | `docs/evidence/release-candidates/partisan-rc-5b1f2e98f931-20260721T193941Z/candidate.json` |
| Manifest / ready-seal SHA-256 | `bef040090557ca1403d6505e2bd1150452ba467225a29adc4295641f5aa3c80c` / `173434122dce60dde8ff1dc939e2d5a916094bdb31096a641850772aa9853ad3` |
| Aggregate package SHA-256 | `af22d6322a215dbef466e49041fc07395cbb5ed7a5951fd3e0cee5f4a101f530` (sha256-manifest-v1 over the canonical four-file package index) |
| Addon GUID / revision / version | `698532771130111D` / `unpublished-local-pack` / `0.1.0-rc.20260721T193941Z.5b1f2e98` |
| Workbench/tool identity | version `1.7.0.54` / SHA-256 `59ee98c352932c1aa8fb29970a377c1a9ea2f839e31d9ab072239212909d54c0` / validation CRC `aeddce9b` |
| Server / client versions | `1.7.0.54` / `1.7.0.54` |

## Proof ladder

A pass never inherits upward. `partial` means some scoped evidence exists but the rung is not passed for the enabled campaign.

| Rung | Status | Honest scope |
| --- | --- | --- |
| Static/source/resource contracts | `passed` | Retained Foundation evidence is green inside the immutable candidate build for its exact source HEAD. |
| Enforce compile and configuration | `passed` | All five explicit Workbench targets are retained and green for the candidate source and canonical four-file package build. |
| Deterministic service contracts | `not-run` | The 91 individually named packaged focused cases have not run against the active package; historical focused results do not transfer. |
| Native engine-world behavior | `not-run` | The same-package release-surface/runtime-retention pair passed and was independently consumed as noncertifying evidence. The 91-case focused aggregate, corrected force-authority canary, and Full Campaign Debug remain pending; historical runtime results do not transfer. |
| Packaged dedicated server | `not-run` | The active candidate has not entered the standard dedicated-server certification gate. |
| Multiple clients, reconnect, and JIP | `not-run` | Host, two-client, reconnect, late-join, and packet-disruption convergence are not certified. |
| Fresh-process restart and fault injection | `partial` | Selected journal, shutdown, field-vehicle, exact-QRF, counterattack, and rebuild cuts pass; the arbitrary full campaign graph and fault matrix remain open. |
| Performance and long soak | `not-run` | The reported one-second hitch and long-campaign capacity limits have no current immutable-package soak evidence. |
| Canary release | `blocked` | Blocked until the active package passes every lower Gate 1 proof boundary. |
| Stable certification | `blocked` | No current matrix row is certified. The active package passed its first noncertifying runtime boundary, while focused, canary, full-profile, and later external gates remain open. |

## Retained evidence

- Foundation: **passed** at 985 references for `5b1f2e98f93137230e686312c6e99cea7630dae4`.
- Workbench: **passed** at 5849 files / 12022 classes / CRC `aeddce9b` for `5b1f2e98f93137230e686312c6e99cea7630dae4`.
- Active release-surface audit: **passed-noncertifying-release-surface-audit** from `2026-07-22T08:13:45.3297133+00:00` through `2026-07-22T08:14:26.6827197+00:00` on run `release_surface_20260722T081345Z_45f8c65a9dbc463abfa1` / leaf `20260722T081345Z-45f8c65a9dbc463abfa1c72e9a2a6042`. Candidate binding `06b5fa23ccd10bfd1f8621f6b03c25d9c36719cc9c7d08ebcc73dabbba8c9f5f`; 41 files revalidated; policy `script-engine-and-process-fatal-v1` diagnostic census 6 raw lines / 2 events = 6 / 2 approved stock + 0 / 0 unapproved across 1 clean mode(s) and 1 exact stock-cluster mode(s); tracked index `docs/evidence/release-surface-audit/partisan-rc-5b1f2e98f931-20260721T193941Z-20260722T081345Z-45f8c65a9dbc463abfa1c72e9a2a6042.json` / SHA-256 `c54c96ecbe128723fd09d662964659be4388de99260d5677feac9338973122d1`; ready SHA-256 `d3387977b0f1df39486e27da20393c881f80faee37701f433a9806941b72a068`; clean harness `52c7e2b96e33ee990a2622b6e43ff7c8ab348af2`. Certification promotion is `none`: this proves the contracted loaded-package surface boundary only and makes no package-byte string-absence claim for the nine source-guarded literals.
- Active Gate 1 runtime retention: **passed-noncertifying-retention** from `2026-07-22T08:15:31.5385389+00:00` through `2026-07-22T08:21:22.8955325+00:00` on run `gate1_20260722T081531Z_ecf73c44732b49da8b91` / leaf `20260722T081531Z-ecf73c44732b`. Candidate binding `06b5fa23ccd10bfd1f8621f6b03c25d9c36719cc9c7d08ebcc73dabbba8c9f5f`; 251 files revalidated; tracked index `docs/evidence/gate1-runtime-retention/partisan-rc-5b1f2e98f931-20260721T193941Z-20260722T081531Z-ecf73c44732b.json` / SHA-256 `5b5ad57e61b254188b99e5e53ea8936a4b76e170a8b96529a4b965d44d6625fb`; ready SHA-256 `6d05989c1efc84da287baceab1af96c95e0855ee2fb02d2d81342ab4a64e435f`; clean harness `52c7e2b96e33ee990a2622b6e43ff7c8ab348af2`. Certification claim is `none` and standard-save restoration certified is `false`; this scoped retention evidence does not advance a later release rung.
- Active packaged focused autotests: **not run** for replacement candidate `partisan-rc-5b1f2e98f931-20260721T193941Z`; no prior-package result transfers to this package.
- Active Campaign Debug: **not run** for replacement candidate `partisan-rc-5b1f2e98f931-20260721T193941Z`; the corrected canary follows only after the packaged focused set is accepted.
- Focused force-authority profile: **35/35** cases and **87/87** counted conditions for `32727238d74b29905c68e5a80bb5897dfdc783c0`, with `CertificationPassed:false`. This is historical state-only, non-package, non-certifying evidence.
- Historical packaged focused autotests: **5/5** cases and JUnit **5/0/0/0** tests/failures/errors/skips against prior exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. Hard diagnostics are explicitly not free: 11 total = 10 approved stock + 1 approved intentional + 0 unapproved, with 40 envelope files rehashed and zero cleanup/spill residue. Summary: `docs/evidence/focused-autotest/partisan-rc-0e632ec4f63e-20260719T004133Z.json` / SHA-256 `961ef6b0a84c26446468b31dd7ac5120448b21a442e9a823de4ff5dc804da7f9`; harness `d4d8f29cda9896ce2c6a5b073dac2cbd03757700`. This immutable non-certifying result does not attach to the active replacement.
- Historical corrected force-authority canary: **accepted non-certifying** on prior exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. The focused proof passed 35/35 assertions and 87/87 counted certification conditions. The 33-check classifier found 2 hard diagnostics = 2 approved stock + 0 approved intentional + 0 unapproved. All 10 envelope files rehashed with zero cleanup/spill residue. Summary: `docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-corrected-canary-20260719T012319Z.json` / SHA-256 `f47fa5f0539c0c8c6024e096f3e034699bc6bfaf656734a0a2b32c9fee7b4aa8`; harness `20375141f840f74316ca46e7df047fcba3e6e344`. This immutable scoped acceptance does not transfer to the active replacement.
- Historical Full Campaign Debug: **rejected, red full profile** on prior exact candidate `partisan-rc-0e632ec4f63e-20260719T004133Z`. The wrapper capture completed mechanically with stable artifacts, 10 rehashed envelope files, and zero cleanup/spill residue, while runtime acceptance remained false. Certification stayed red at 584 PASS, 49 WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED with 5561/5687 required assertions proven, 112 failed, and 14 blocked. The fail-closed classifier found 25 hard diagnostics = 2 approved stock + 13 approved intentional + 10 unapproved. Summary: `docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-full-20260719T014151Z.json` / SHA-256 `ed225ba2acb6932437af55219ff0b6ba69f4a2111880acd11c2555c875819ca7`; clean harness `27052811bb192835fc09ab3cb052b36cabad5df4`. Mechanical capture success is not certification or diagnostic acceptance, and this immutable rejection does not attach to the active replacement.
- Historical packaged focused autotests: **5/5** cases and JUnit **5/0/0/0** tests/failures/errors/skips against prior exact candidate `partisan-rc-e11e7ea88a44-20260719T040154Z`. Hard diagnostics are explicitly not free: 11 total = 10 approved stock + 1 approved intentional + 0 unapproved, with 40 envelope files rehashed and zero cleanup/spill residue. Summary: `docs/evidence/focused-autotest/partisan-rc-e11e7ea88a44-20260719T040154Z.json` / SHA-256 `9ddade1cb86a209acf4aae02ded6f1a7713fe1e25ba577ae00ef1980e3de149a`; harness `b1940f241e28f163202807385f7140f048921447`. This immutable non-certifying result does not attach to the active replacement.
- Historical corrected force-authority canary: **rejected, focused proof failed** on prior exact candidate `partisan-rc-e11e7ea88a44-20260719T040154Z`. The focused proof passed 33/35 assertions and 85/87 counted conditions. The classifier remained valid at 2 approved and 0 unapproved hard diagnostics. All 10 envelope files rehashed with zero cleanup/spill residue. Summary: `docs/evidence/campaign-debug/partisan-rc-e11e7ea88a44-20260719T040154Z-corrected-canary-20260719T050302Z.json` / SHA-256 `af0aca25a84d8f757dbba8010950a658ce09937aa4048c35b2e372f1183eec69`; harness `937c86c5d2259a9da270ea76371001ac1d4c6eed`. This immutable rejection does not transfer to the active replacement.
- Historical Full Campaign Debug: **stopped and not run** for prior exact candidate `partisan-rc-e11e7ea88a44-20260719T040154Z`; its `rejected-after-corrected-canary` contract ended the chain at the rejected corrected canary.
- Historical packaged focused autotests: **5/5** cases and JUnit **5/0/0/0** tests/failures/errors/skips against prior exact candidate `partisan-rc-ee0e8add2a29-20260719T063815Z`. Hard diagnostics are explicitly not free: 11 total = 10 approved stock + 1 approved intentional + 0 unapproved, with 40 envelope files rehashed and zero cleanup/spill residue. Summary: `docs/evidence/focused-autotest/partisan-rc-ee0e8add2a29-20260719T063815Z.json` / SHA-256 `0a8fcfc5ca739ff261be644cdfcb02311e4ef967c374f093e2963e4b1374800b`; harness `273ed14ba8526259c8b0d248177fa53b59ade683`. This immutable non-certifying result does not attach to the active replacement.
- Historical corrected force-authority canary: **accepted non-certifying** on prior exact candidate `partisan-rc-ee0e8add2a29-20260719T063815Z`. The focused proof passed 35/35 assertions and 87/87 counted certification conditions. The 33-check classifier found 2 hard diagnostics = 2 approved stock + 0 approved intentional + 0 unapproved. All 10 envelope files rehashed with zero cleanup/spill residue. Summary: `docs/evidence/campaign-debug/partisan-rc-ee0e8add2a29-20260719T063815Z-corrected-canary-20260719T071408Z.json` / SHA-256 `f3521fdee20811efd37a260d23498aad43d75435cc01331022ffb8565df34b42`; harness `4f8d7e2d7a39896737fd6754060523bf852c5fa8`. This immutable scoped acceptance does not transfer to the active replacement.
- Historical Full Campaign Debug: **rejected, red full profile** on prior exact candidate `partisan-rc-ee0e8add2a29-20260719T063815Z`. The wrapper capture completed mechanically with stable artifacts, 10 rehashed envelope files, and zero cleanup/spill residue, while runtime acceptance remained false. Certification stayed red at 598 PASS, 47 WARN, 26 FAIL, 13 BLOCKED, and 1 SKIPPED with 5630/5695 required assertions proven, 50 failed, and 15 blocked. The fail-closed classifier found 26 hard diagnostics = 2 approved stock + 0 approved intentional + 24 unapproved. Summary: `docs/evidence/campaign-debug/partisan-rc-ee0e8add2a29-20260719T063815Z-full-20260719T072739Z.json` / SHA-256 `e83bc1e752ac4c1abc5cb57ce097459642e17637f6747e4edc8e7d57569c1884`; clean harness `a5ccf36aee17a4f88d7f1c2f232ce9fc14652018`. Mechanical capture success is not certification or diagnostic acceptance, and this immutable rejection does not attach to the active replacement.

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
| `STATUS-001` | `AUTH` | The active Gate 1 candidate has green Foundation, all-target Workbench, and same-package paired release-surface/runtime-retention evidence. The 91-case focused aggregate, corrected canary, and Full Campaign Debug are still required; no historical result transfers. |
| `STATUS-002` | `TEST` | Run and independently consume the 91-case packaged focused set against the unchanged candidate, then the corrected force-authority canary, and only after an accepted canary run Full Campaign Debug. |
| `STATUS-003` | `UI` | Known command-menu and modal-map defects remain open until source correction plus rendered packaged-client proof. |
| `STATUS-004` | `MOVE` | Natural sustained infantry and convoy travel, identical-waypoint suppression, and measured no-stutter behavior are not proven. |
| `STATUS-005` | `PROJ` | Campaign read-model convergence is not proven with host, two clients, reconnect, JIP, restart, and marker-cap boundaries. |
| `STATUS-006` | `PERF` | The one-second hitch and bounded-history admission cliffs remain unclosed. |
| `STATUS-007` | `MISSION` | Configured mission breadth still exceeds mission-specific CE 3.11.1 behavioral parity and proof. |

## Next release-closure step

Run the individually named packaged focused service suites next against active replacement `partisan-rc-5b1f2e98f931-20260721T193941Z`, manifest `docs/evidence/release-candidates/partisan-rc-5b1f2e98f931-20260721T193941Z/candidate.json`, and aggregate package SHA-256 `af22d6322a215dbef466e49041fc07395cbb5ed7a5951fd3e0cee5f4a101f530`. If that exact-package set is accepted, run the corrected force-authority canary next; do not transfer the historical candidate's pass or rejection into either gate.
