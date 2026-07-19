# Release-Candidate Identity Evidence

This directory retains portable, byte-identical copies of each published
candidate's `candidate.json` manifest and `candidate.ready.json` seal. The
generated current-status check verifies their hashes and cross-checks their
source, package, addon, toolchain, Workbench, and ready-seal identities.

These small tracked records are not substitutes for the immutable package and
raw evidence bundle. Proof attached to an active runtime candidate must consume
the exact candidate ID and aggregate package SHA-256 named by its manifest.
Rebuilding starts a new candidate directory and evidence chain; existing
records are not rewritten.
Repository attributes keep both JSON records on canonical LF endings so their
byte hashes remain stable across checkouts.

Current status selects `partisan-rc-0e632ec4f63e-20260719T004133Z` as the active
runtime candidate. It was built from clean source HEAD
`0e632ec4f63eab43e8c301d0755f10193d85131f`; its exact four-file package
SHA-256 is `e5d29458c33aeef9cd2b37476359acc6021fe78cf0fc74513d9a2f69ef0614dc`,
manifest SHA-256 is
`ea06318a8f5161f000685fe37ecab4f5c8a77d6b0e8205f502a6418e3365e76b`,
and ready-seal SHA-256 is
`cd91e569b8a4a453dad6b0f884f22afbb36b9b5f0de629fd70b2188875e47c53`.
Foundation and all five Workbench targets passed at 5,847 files/11,900 classes,
common CRC `3a399db1`, zero hard errors, and exact-zero cleanup. The manifest
binds four package files and 50 evidence files.

`partisan-rc-c2b16c4a2d85-20260718T201442Z` and
`partisan-rc-b8deddc4b631-20260718T213322Z` remain sealed as superseded artifact
evidence. Their package-bound results cannot be attached to the active
replacement.

Runtime results are not appended to these sealed candidate directories. Each
candidate-aware runner writes a fresh external sidecar run whose portable
envelope binds raw-file hashes to candidate, package, manifest, ready seal,
harness, runtime-tool, result, and cleanup identities. Only a deliberately
selected portable summary may later be tracked here.

The accepted classifier-aware five-case summary for the prior candidate is
`focused-autotest/partisan-rc-b8deddc4b631-20260718T213322Z.json`. It binds the
five external envelope hashes, exact candidate and harness identities, per-case
JUnit and diagnostic counts, aggregate totals, rehash status, and cleanup/spill
status without copying machine-local paths or mutable raw logs into the
repository. The raw sidecars remain external and immutable.
Repository attributes also keep these tracked summary JSON files on canonical
LF endings so the status-recorded digest remains stable across checkouts.

The prior candidate's original package-bound Campaign Debug evidence is
summarized in
`campaign-debug/partisan-rc-b8deddc4b631-20260718T213322Z.json`. It binds the
retained original focused canary and full run to the exact candidate, clean
harness identity, runner/module, guarded settings, envelope hashes, result
totals, category counts, rehash/cleanup predicates, corrected diagnostic audit,
and historical aggregate comparison. The original immutable raw files are
mechanically complete and stable, but their wrapper-reported success and zero
census are not acceptance. The timestamp-aware audit records three canary
diagnostics = two exact approved stock + one unapproved. The full capture has 25
diagnostics with a 19-line Partisan subset; across all 25, two are exact approved
stock, 13 are proof-bound intentional, and ten are unapproved.

The separate corrected-canary summary is
`campaign-debug/partisan-rc-b8deddc4b631-20260718T213322Z-corrected-canary-20260719T001339Z.json`.
Clean classifier harness `38a094f` reran only the `force_authority` canary
against the unchanged package. Its proof recorded 35/35 and 87/87, while the
corrected census was three = two approved stock + one unapproved map-locator VM
exception. Cleanup/spill residue was zero, but the runner failed closed. The
full profile was correctly not rerun after that rejection. All candidate-bound
captures remain preliminary-unaccepted, and the retained full report remains
failed.

The accepted classifier-aware five-case summary for the active candidate is
`focused-autotest/partisan-rc-0e632ec4f63e-20260719T004133Z.json`, SHA-256
`961ef6b0a84c26446468b31dd7ac5120448b21a442e9a823de4ff5dc804da7f9`.
Clean harness HEAD `d4d8f29cda9896ce2c6a5b073dac2cbd03757700` completed the set from
`2026-07-19T01:08:50.9577409Z` through
`2026-07-19T01:09:44.4465092Z`. All five cases passed with JUnit 5/0/0/0; all
40 envelope files rehashed, every run passed 12 classifier checks, cleanup and
spill residue were zero, and the exact census was 11 = ten approved stock + one
approved intentional + zero unapproved. This is a `passed-noncertifying`
deterministic-service rung, not corrected-canary, Full Campaign Debug, or
runtime proof of the map-locator correction.

The accepted corrected-canary summary for the active candidate is
`campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-corrected-canary-20260719T012319Z.json`,
SHA-256 `f47fa5f0539c0c8c6024e096f3e034699bc6bfaf656734a0a2b32c9fee7b4aa8`.
Clean harness HEAD `20375141f840f74316ca46e7df047fcba3e6e344` retained run
`seed1985_t0_p1_u1784424219` under leaf
`20260719T012319Z-47423d741d0e4690b3c7dbbbab68cebd`. The run started at
`2026-07-19T01:23:19.5837772Z`, completed at
`2026-07-19T01:24:02.2143579Z`, and recorded 40 canonical runtime seconds. Its
11 cases were 9 PASS/1 WARN/0 FAIL/1 BLOCKED/0 SKIPPED; focused proof passed
35/35 assertions and 87/87 certification-counting conditions. State
restoration was 18/0, final orphan cleanup passed, all ten envelope files
rehashed, and every cleanup/spill counter was zero. The exact diagnostic census
was two approved stock + zero approved intentional + zero unapproved, with all
33 classifier self-tests passing. Envelope SHA-256 is
`e3705a849590b9fd3086fdb0caf5659df6e0c1029784612965c848a0f8f0a851`.
The prior map-locator exception is absent for this scoped path. This is accepted
`passed-noncertifying` native-engine/world canary evidence, not full
certification.

The active full-profile summary is
`campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-full-20260719T014151Z.json`,
SHA-256 `ed225ba2acb6932437af55219ff0b6ba69f4a2111880acd11c2555c875819ca7`.
Clean harness HEAD `27052811bb192835fc09ab3cb052b36cabad5df4` retained run
`seed1985_t0_p1_u1784425330` under leaf
`20260719T014151Z-470870c9cc7e4493afb9a6ceb6ff2bce`. The wrapper completed a
mechanically valid capture with stable artifacts, ten rehashed envelope files,
18/0 state restoration, final orphan cleanup, and zero cleanup/spill residue.
Runtime acceptance and certification remained false: 584 PASS/49 WARN/46
FAIL/7 BLOCKED/1 SKIPPED and 5,561/5,687 required assertions proven, with 112
failed and 14 blocked. The classifier found 25 hard diagnostics = two approved
stock + 13 approved intentional + ten unapproved. Envelope SHA-256 is
`f61bd05fcc5c95c5d0ddbbeb46a9220771d116b86bad1ad4f26340f4853ec825`.
This is a rejected red full-profile boundary. The native-engine/world rung is
failed and release remains `NO-GO`; post-capture source fixes require a new
sealed candidate and may not be attached to this immutable package.

Canonical LF is required for the tracked summaries, and the release generator
rehashes and cross-checks them against current status without transferring the
prior package's evidence to the new package.

If a package defect requires a rebuild, the older record remains byte-identical
and is described as retained superseded evidence. It remains available for
archive validation and consumer preflight, but not current runtime proof. No
result from that package may be combined with the replacement candidate's
evidence chain.
