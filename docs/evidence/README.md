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

Current status selects `partisan-rc-b8deddc4b631-20260718T213322Z` as the active
runtime candidate. `partisan-rc-c2b16c4a2d85-20260718T201442Z` remains sealed
as superseded artifact evidence; its package-bound results cannot be attached
to the active replacement.

Runtime results are not appended to these sealed candidate directories. Each
candidate-aware runner writes a fresh external sidecar run whose portable
envelope binds raw-file hashes to candidate, package, manifest, ready seal,
harness, runtime-tool, result, and cleanup identities. Only a deliberately
selected portable summary may later be tracked here.

The accepted classifier-aware five-case summary for the active candidate is
`focused-autotest/partisan-rc-b8deddc4b631-20260718T213322Z.json`. It binds the
five external envelope hashes, exact candidate and harness identities, per-case
JUnit and diagnostic counts, aggregate totals, rehash status, and cleanup/spill
status without copying machine-local paths or mutable raw logs into the
repository. The raw sidecars remain external and immutable.
Repository attributes also keep these tracked summary JSON files on canonical
LF endings so the status-recorded digest remains stable across checkouts.

The current package-bound Campaign Debug evidence is summarized in
`campaign-debug/partisan-rc-b8deddc4b631-20260718T213322Z.json`. It binds one
guarded focused canary and one full run to the exact candidate, clean harness,
runner/module, guarded settings, envelope hashes, result totals, category
counts, rehash/error/cleanup predicates, and historical aggregate comparison.
The full wrapper succeeded and its artifacts are trustworthy, while
`fullCertificationPassed:false` remains the release result. Canonical LF is
required for this summary too, and the release generator rehashes and
cross-checks it against current status.

If a package defect requires a rebuild, the older record remains byte-identical
and is described as retained superseded evidence. It remains available for
archive validation and consumer preflight, but not current runtime proof. No
result from that package may be combined with the replacement candidate's
evidence chain.
