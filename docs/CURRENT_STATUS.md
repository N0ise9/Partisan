# Partisan Current Status

> Generated from `docs/data/release_status.json` and `docs/data/antistasi_ce311_parity.json`. Do not hand-edit this file; run `tools/update-release-docs.ps1`.

## Release decision

**NO-GO - development alpha.** No release-candidate package is certified.

The retained candidate identity below binds its exact source HEAD, manifest, canonical four-file package index, addon identity, and validation tools. The generator verifies that the candidate source is between the audited gameplay revision and the live checkout HEAD.

## Identity

| Field | Current value |
| --- | --- |
| Status data as of | `2026-07-18T22:47:19Z` |
| Audited gameplay Git HEAD | `25dedb3e82ad516c28826830bc1e06a2d3940f53` |
| Embedded implementation identity | `32727238d74b29905c68e5a80bb5897dfdc783c0` |
| Embedded build UTC / label | `2026-07-18T16:34:38Z` / `schema71-settings24-focused-force-authority` |
| Campaign / runtime-settings schema | `71` / `24` |
| Workbench CRC | `f27e637b` |
| Release candidate / source HEAD | `partisan-rc-b8deddc4b631-20260718T213322Z` / `b8deddc4b6314936b7ea04f36a35784622a46da6` |
| Runtime use disposition | `active-runtime-candidate` |
| Candidate manifest | `docs/evidence/release-candidates/partisan-rc-b8deddc4b631-20260718T213322Z/candidate.json` |
| Manifest / ready-seal SHA-256 | `c88d363423008bcba2366afa8d458613bf539c549969bcae73cd82e5cd9402a5` / `6518c536e7104f21af81c94c4959a66587c4efe67bf2339001489dcddad00d87` |
| Aggregate package SHA-256 | `82e1fd0bf7c3404b7fe842fa84efd10f225bf82fc76c11502b9a684b63f4f329` (sha256-manifest-v1 over the canonical four-file package index) |
| Addon GUID / revision / version | `698532771130111D` / `unpublished-local-pack` / `0.1.0-rc.20260718T213322Z.b8deddc4` |
| Workbench/tool identity | version `1.7.0.54` / SHA-256 `59ee98c352932c1aa8fb29970a377c1a9ea2f839e31d9ab072239212909d54c0` / validation CRC `f27e637b` |
| Server / client versions | `1.7.0.54` / `1.7.0.54` |

## Proof ladder

A pass never inherits upward. `partial` means some scoped evidence exists but the rung is not passed for the enabled campaign.

| Rung | Status | Honest scope |
| --- | --- | --- |
| Static/source/resource contracts | `passed` | Retained Foundation evidence is green inside the immutable candidate build for its exact source HEAD. |
| Enforce compile and configuration | `passed` | All five explicit Workbench targets are retained and green for the candidate source and canonical four-file package build. |
| Deterministic service contracts | `passed-noncertifying` | All five release-critical service suites passed individually against the exact staged packed candidate with 5/0/0/0 JUnit totals, exact classified diagnostics, and zero residue. This scoped rung does not inherit into native-world or later gates. |
| Native engine-world behavior | `failed` | The current package-bound full Campaign Debug run is trustworthy but red, including native mission-runtime and physicalization failures. Scoped passing cuts remain useful, but this rung cannot be partial-green while a current subset fails. |
| Packaged dedicated server | `not-run` | The active replacement candidate is sealed, but it has not yet been launched through the standard dedicated-server runtime gate. |
| Multiple clients, reconnect, and JIP | `not-run` | Host, two-client, reconnect, late-join, and packet-disruption convergence are not certified. |
| Fresh-process restart and fault injection | `partial` | Selected journal, shutdown, field-vehicle, exact-QRF, counterattack, and rebuild cuts pass; the arbitrary full campaign graph and fault matrix remain open. |
| Performance and long soak | `not-run` | The reported one-second hitch and long-campaign capacity limits have no current immutable-package soak evidence. |
| Canary release | `blocked` | Blocked by every lower unproven release rung. |
| Stable certification | `blocked` | No current matrix row is certified. |

## Retained evidence

- Foundation: **passed** at 874 references for `b8deddc4b6314936b7ea04f36a35784622a46da6`.
- Workbench: **passed** at 5846 files / 11899 classes / CRC `f27e637b` for `b8deddc4b6314936b7ea04f36a35784622a46da6`.
- Packaged focused autotests: **5/5** cases and JUnit **5/0/0/0** tests/failures/errors/skips against exact candidate `partisan-rc-b8deddc4b631-20260718T213322Z`. Hard diagnostics are explicitly not free: 11 total = 10 approved stock + 1 approved intentional + 0 unapproved, with 40 envelope files rehashed and zero cleanup/spill residue. Summary: `docs/evidence/focused-autotest/partisan-rc-b8deddc4b631-20260718T213322Z.json` / SHA-256 `8bb36919f0649e0f48fad50305878ec883cf98a0021323ba1442017f1aa113b8`; harness `b3fc1e6f56d9cf8805bac1702a54e0b5284e0043`. This is passed non-certifying service evidence.
- Focused force-authority profile: **35/35** cases and **87/87** counted conditions for `32727238d74b29905c68e5a80bb5897dfdc783c0`, with `CertificationPassed:false`. This is historical state-only, non-package, non-certifying evidence.
- Full Campaign Debug: **current package-bound run failed certification** on exact candidate `partisan-rc-b8deddc4b631-20260718T213322Z`: 584 PASS, 49 WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED; 5562/5688 required assertions proven, with 112 failed and 14 blocked. The wrapper, candidate boundary, packed mount, artifact stability, error census, cleanup, and envelope rehash checks passed, so this is trustworthy red evidence rather than a release pass. Summary: `docs/evidence/campaign-debug/partisan-rc-b8deddc4b631-20260718T213322Z.json` / SHA-256 `bbc88ab51b8f053d8aff14d263c09489bd1bb0d7578290d0894a5ccd683e2a77`; harness `1bff1890830db08159826f63b550227aa7bb0da3`.

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
| `STATUS-001` | `AUTH` | The replacement candidate is sealed and runtime-eligible with exact package and tool identities, and its packaged focused service rung passed; current Full Campaign Debug is red, while dedicated, multiplayer/JIP, restart breadth, performance, and soak remain pending. |
| `STATUS-002` | `TEST` | Current package-bound Full Campaign Debug completed with valid artifacts but failed certification: 46 failed cases, 7 blocked cases, 112 failed required assertions, and 14 blocked required assertions now require evidence-backed triage. |
| `STATUS-003` | `UI` | Known command-menu and modal-map defects remain open until source correction plus rendered packaged-client proof. |
| `STATUS-004` | `MOVE` | Natural sustained infantry and convoy travel, identical-waypoint suppression, and measured no-stutter behavior are not proven. |
| `STATUS-005` | `PROJ` | Campaign read-model convergence is not proven with host, two clients, reconnect, JIP, restart, and marker-cap boundaries. |
| `STATUS-006` | `PERF` | The one-second hitch and bounded-history admission cliffs remain unclosed. |
| `STATUS-007` | `MISSION` | Configured mission breadth still exceeds mission-specific CE 3.11.1 behavioral parity and proof. |
| `STATUS-008` | `SEC` | Development proof and destructive diagnostic surfaces have not yet been separated from the release package. |

## Next release-closure step

Gate 1 retained exact candidate `partisan-rc-b8deddc4b631-20260718T213322Z` through a trustworthy but red Full Campaign Debug result. Classify and reproduce the smallest concrete root among its 46 failed and 7 blocked cases before changing source. Any correction must be validated and sealed as a new candidate; it cannot extend this immutable package's evidence chain.
