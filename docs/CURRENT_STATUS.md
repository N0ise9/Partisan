# Partisan Current Status

> Generated from `docs/data/release_status.json` and `docs/data/antistasi_ce311_parity.json`. Do not hand-edit this file; run `tools/update-release-docs.ps1`.

## Release decision

**NO-GO - development alpha.** No release-candidate package is certified.

The audited gameplay revision is fixed below. A tracked Markdown file cannot embed the hash of the commit that contains itself; the generator verifies that the audited revision is an ancestor of the checkout and prints the live checkout HEAD when it runs. Gate 1 evidence must record the exact post-checkout Git SHA and package hash together.

## Identity

| Field | Current value |
| --- | --- |
| Status data as of | `2026-07-18T16:34:38Z` |
| Audited gameplay Git HEAD | `25dedb3e82ad516c28826830bc1e06a2d3940f53` |
| Embedded implementation identity | `32727238d74b29905c68e5a80bb5897dfdc783c0` |
| Embedded build UTC / label | `2026-07-18T16:34:38Z` / `schema71-settings24-focused-force-authority` |
| Campaign / runtime-settings schema | `71` / `24` |
| Workbench CRC | `cad640f3` |
| Release package SHA-256 | not built |
| Server / client versions | not recorded |

## Proof ladder

A pass never inherits upward. `partial` means some scoped evidence exists but the rung is not passed for the enabled campaign.

| Rung | Status | Honest scope |
| --- | --- | --- |
| Static/source/resource contracts | `passed` | Retained Foundation evidence is green for the embedded implementation; the new release-spec drift check must remain green on later revisions. |
| Enforce compile and configuration | `passed` | Retained Workbench evidence is green for the embedded implementation identity, not for a release package. |
| Deterministic service contracts | `partial` | Selected focused fixtures pass, but aggregate autotest registration and the full current suite are incomplete. |
| Native engine-world behavior | `partial` | Scoped native and fresh-process cuts exist; natural movement, full active-world behavior, UI rendering, and broad entity classification remain open. |
| Packaged dedicated server | `not-run` | No immutable release-candidate package identity is retained for the audited revision. |
| Multiple clients, reconnect, and JIP | `not-run` | Host, two-client, reconnect, late-join, and packet-disruption convergence are not certified. |
| Fresh-process restart and fault injection | `partial` | Selected journal, shutdown, field-vehicle, exact-QRF, counterattack, and rebuild cuts pass; the arbitrary full campaign graph and fault matrix remain open. |
| Performance and long soak | `not-run` | The reported one-second hitch and long-campaign capacity limits have no current immutable-package soak evidence. |
| Canary release | `blocked` | Blocked by every lower unproven release rung. |
| Stable certification | `blocked` | No current matrix row is certified. |

## Retained evidence

- Foundation: **passed** at 874 references for `32727238d74b29905c68e5a80bb5897dfdc783c0`.
- Workbench: **passed** at 5846 files / 11899 classes / CRC `cad640f3` for `32727238d74b29905c68e5a80bb5897dfdc783c0`.
- Focused force-authority profile: **35/35** cases and **87/87** counted conditions, with `CertificationPassed:false`. This is state-only, non-certifying evidence.
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
| `STATUS-001` | `AUTH` | No single immutable package and evidence bundle identifies every proof rung. |
| `STATUS-002` | `PERSIST` | The newest completed Full Campaign Debug result is historical and red; current failures and blockers are not yet reclassified on the audited source. |
| `STATUS-003` | `UI` | Known command-menu and modal-map defects remain open until source correction plus rendered packaged-client proof. |
| `STATUS-004` | `MOVE` | Natural sustained infantry and convoy travel, identical-waypoint suppression, and measured no-stutter behavior are not proven. |
| `STATUS-005` | `PROJ` | Campaign read-model convergence is not proven with host, two clients, reconnect, JIP, restart, and marker-cap boundaries. |
| `STATUS-006` | `PERF` | The one-second hitch and bounded-history admission cliffs remain unclosed. |
| `STATUS-007` | `MISSION` | Configured mission breadth still exceeds mission-specific CE 3.11.1 behavioral parity and proof. |
| `STATUS-008` | `SEC` | Development proof and destructive diagnostic surfaces have not yet been separated from the release package. |

## Next release-closure step

Gate 0's generated truth surface is complete. Gate 1 is the current work boundary: commit the guarded build-once tooling, build one clean package, and record Git, Workbench, package, addon, server, and client identities in one retained evidence bundle before rerunning the current proof ladder.
