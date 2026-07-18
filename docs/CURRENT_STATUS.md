# Partisan Current Status

> Generated from `docs/data/release_status.json` and `docs/data/antistasi_ce311_parity.json`. Do not hand-edit this file; run `tools/update-release-docs.ps1`.

## Release decision

**NO-GO - development alpha.** No release-candidate package is certified.

The retained candidate identity below binds its exact source HEAD, manifest, canonical four-file package index, addon identity, and validation tools. The generator verifies that the candidate source is between the audited gameplay revision and the live checkout HEAD.

## Identity

| Field | Current value |
| --- | --- |
| Status data as of | `2026-07-18T21:36:44Z` |
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
| Deterministic service contracts | `partial` | Selected focused fixtures pass and the active replacement contains all five service-only suite registrations, but individually registered packaged JUnit results have not run yet. |
| Native engine-world behavior | `partial` | Scoped native and fresh-process cuts exist; natural movement, full active-world behavior, UI rendering, and broad entity classification remain open. |
| Packaged dedicated server | `not-run` | The active replacement candidate is sealed, but it has not yet been launched through the standard dedicated-server runtime gate. |
| Multiple clients, reconnect, and JIP | `not-run` | Host, two-client, reconnect, late-join, and packet-disruption convergence are not certified. |
| Fresh-process restart and fault injection | `partial` | Selected journal, shutdown, field-vehicle, exact-QRF, counterattack, and rebuild cuts pass; the arbitrary full campaign graph and fault matrix remain open. |
| Performance and long soak | `not-run` | The reported one-second hitch and long-campaign capacity limits have no current immutable-package soak evidence. |
| Canary release | `blocked` | Blocked by every lower unproven release rung. |
| Stable certification | `blocked` | No current matrix row is certified. |

## Retained evidence

- Foundation: **passed** at 874 references for `b8deddc4b6314936b7ea04f36a35784622a46da6`.
- Workbench: **passed** at 5846 files / 11899 classes / CRC `f27e637b` for `b8deddc4b6314936b7ea04f36a35784622a46da6`.
- Focused force-authority profile: **35/35** cases and **87/87** counted conditions for `32727238d74b29905c68e5a80bb5897dfdc783c0`, with `CertificationPassed:false`. This is historical state-only, non-package, non-certifying evidence.
- Full Campaign Debug: **historical and failed** on `7c8b9c27b4ee553664fa2b44aea4a8d53c7123a5`: 583 PASS, 50 WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED; 5537/5685 required assertions proven. It predates the audited revision and must be rerun before its individual failures are treated as current.

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
| `STATUS-001` | `AUTH` | The replacement candidate is sealed and runtime-eligible with exact package and tool identities; current focused, Campaign Debug, dedicated, multiplayer/JIP, restart, performance, and soak rungs remain pending. |
| `STATUS-002` | `PERSIST` | The newest completed Full Campaign Debug result is historical and red; current failures and blockers are not yet reclassified on the audited source. |
| `STATUS-003` | `UI` | Known command-menu and modal-map defects remain open until source correction plus rendered packaged-client proof. |
| `STATUS-004` | `MOVE` | Natural sustained infantry and convoy travel, identical-waypoint suppression, and measured no-stutter behavior are not proven. |
| `STATUS-005` | `PROJ` | Campaign read-model convergence is not proven with host, two clients, reconnect, JIP, restart, and marker-cap boundaries. |
| `STATUS-006` | `PERF` | The one-second hitch and bounded-history admission cliffs remain unclosed. |
| `STATUS-007` | `MISSION` | Configured mission breadth still exceeds mission-specific CE 3.11.1 behavioral parity and proof. |
| `STATUS-008` | `SEC` | Development proof and destructive diagnostic surfaces have not yet been separated from the release package. |
| `STATUS-009` | `TEST` | The active replacement includes the empty-world override for all five service-only focused suites; five individually retained packaged JUnit results are still required. |

## Next release-closure step

Gate 1 retained candidate `partisan-rc-b8deddc4b631-20260718T213322Z`. Every later proof must consume the package identified by manifest `docs/evidence/release-candidates/partisan-rc-b8deddc4b631-20260718T213322Z/candidate.json` and aggregate SHA-256 `82e1fd0bf7c3404b7fe842fa84efd10f225bf82fc76c11502b9a684b63f4f329`; rebuilding creates a different candidate rather than extending this evidence chain.
