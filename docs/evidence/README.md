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

## Portable Schema-2 Runtime Evidence

New packaged focused evidence is published as one tracked schema-2 aggregate
under `focused-autotest/`. The tracked JSON is only the portable index: the raw
five-run/40-blob tree remains in the immutable external evidence root. Each
`sourceRuns[].runEnvelopePath` is relative to that supplied root and must use
`<candidate-id>/focused-autotest/<profile>/<run-leaf>/run.json`. Publication and
release-doc consumption independently reopen the five envelopes, exact eight-
blob inventories, candidate manifests/seals, JUnit results, logs, and hashes.
The index binds the focused-run harness Git blobs plus the committed aggregate
producer and release-doc consumer. A mismatch produces red
`replacement-required` evidence; it never rewrites an accepted aggregate.
Package identity is independently recomputed from the exact canonical four-file
manifest tuple set. Console admission censes all suite-start and profile-success
shapes, and a replacement receipt has authority only when its candidate ID plus
package, manifest, and ready-seal hashes match the retained candidate.

New corrected `force_authority` canaries and full Campaign Debug runs share the
portable schema-2 Campaign Debug release-index envelope. Their tracked
`release-index.json` remains byte-identical to the copy in the external raw
bundle, whose canonical ten files are reopened and rehashed by the consumer.
The corrected-canary policy additionally derives the exact case/assertion
census, rejects hidden assertion failures or skips, and requires the current
green census of 9 PASS/2 WARN/0 FAIL/0 BLOCKED/0 SKIPPED. Its only warnings are
`cleanup.player_marker.live` and `isolation.world_scope` in their canonical
parent cases; any blocker is red. The evidence closes only as scoped
`passed-noncertifying` or red `failed-corrected-canary`. Existing schema-1
summaries remain immutable historical evidence and are not rewritten into the
new format.

The consumer chooses Schema 1 or Schema 2 from the retained summary schema and
evidence kind, not from the shared `passed-noncertifying` status. Active Schema-2
evidence requires stationary current tool bytes. Historical Schema-2 evidence
instead reopens the recorded immutable Git blobs and ancestry so later legitimate
tool changes do not invalidate already accepted history.

Tool hashes in schema-2 evidence are admission boundaries. Run publication and
consumer self-tests only from a stationary committed tool set; dirty producer,
runner, candidate-module, or consumer bytes correctly fail closed.

## Release-Ledger Schema 3 Historical Evidence

Release-ledger Schema 3 is distinct from Campaign Schema 71. Its
`historicalCandidateEvidence` value is a true JSON array with one or more
entries ordered oldest to newest. Each entry contains exactly
`retirementDisposition`, `candidate`, and `evidence`.

- `rejected-after-full-profile` requires that candidate's focused summary,
  accepted corrected-canary summary, and rejected full-profile summary.
- `rejected-after-corrected-canary` requires that candidate's focused summary
  and rejected corrected-canary summary. `fullCampaignDebug` is forbidden: a
  canary-stopped full run has no file, hash, placeholder, null, or borrowed
  substitute.

Candidate ID/source, manifest path/hash, ready-seal hash, package hash, and
candidate-bound summary/envelope/run identities must exact-match their retained
files and remain non-conflicting across the historical array and current
artifact. Evidence times and Git ancestry must agree with both the per-entry
gate topology and the declared oldest-to-newest array order.

The checked activation now records two ordered historical entries. `history[0]`
is `partisan-rc-0e632ec4f63e-20260719T004133Z` with
`rejected-after-full-profile`. `history[1]` is
`partisan-rc-e11e7ea88a44-20260719T040154Z` with
`rejected-after-corrected-canary`; its own focused and rejected corrected-canary
evidence are required and `fullCampaignDebug` remains absent. The activation
replaced the complete current-candidate surface without duplicating either
historical identity. Existing historical records and the `NO-GO` decision remain
unchanged.

The retained `rejected-after-runtime` candidate is
`partisan-rc-ee0e8add2a29-20260719T063815Z`, version
`0.1.0-rc.20260719T063815Z.ee0e8add`, built from clean source HEAD
`ee0e8add2a298e83fd304b7660c4fc480dc6383f`. Its exact four-file package
SHA-256 is
`981258439b9d08866c4883471cacfe33aa373a36a667a39e8c939f285db74daf`,
manifest SHA-256 is
`1b877e3aa21773a268704bcb3fe889768fca3aa2d78541aa7285b061398ce907`,
and ready-seal SHA-256 is
`01741b85d0edba69f54b07388cdd7c452b8f6f1ad7ef4f6faf253918a4bbf280`.
Foundation passed all 874 checks. All five Workbench targets passed at 5,848
files/11,901 classes with common CRC `f64e0868`; the seal binds four package
files and 50 evidence files. At approximately `2026-07-19T07:02Z`, clean harness
HEAD `273ed14ba8526259c8b0d248177fa53b59ade683` passed all five canonical
packaged focused cases against the exact ee0 candidate and packed mount. The
aggregate was JUnit 5/0/0/0, all 40 envelope files were retained, every run
passed all 12 classifier checks, the diagnostic census was 11 = ten approved
stock + one approved intentional + zero unapproved, and cleanup/spill residue
was zero. This advances deterministic-service only to
`passed-noncertifying`. At approximately `2026-07-19T07:14Z`, clean harness
HEAD `4f8d7e2d7a39896737fd6754060523bf852c5fa8` then accepted corrected canary
`seed1985_t0_p1_u1784445266` against the same exact unchanged candidate and
package. Its 11 cases ended 9 PASS/1 WARN/0 FAIL/1 BLOCKED/0 SKIPPED. All 35/35
focused assertions and 87/87 certification-counting assertions were proven;
state restoration was 18/0; all ten envelope files rehashed; all 33 classifier
checks passed; the diagnostic census was two approved stock plus zero
unapproved; and final orphan cleanup plus every cleanup/spill boundary passed
at zero. Scoped native-engine/world advances only to `passed-noncertifying`.

Clean harness HEAD `a5ccf36aee17a4f88d7f1c2f232ce9fc14652018` then ran Full
Campaign Debug against the exact unchanged candidate/package. Run
`seed1985_t0_p1_u1784446076`, leaf
`20260719T072739Z-97fc069d58cd427c848c83f99f39e5f9`, completed from
`2026-07-19T07:27:39.1454367Z` through `2026-07-19T07:40:09.5714410Z`.
Candidate binding, packed mount, package bytes, and artifacts were stable. All
ten envelope files rehashed to SHA-256
`fce4928444f15531f254ad4d7e119cf8bfe1d06e6fcb564518d2e052544d4278`;
state restoration was 18/0; Phase 17 passed 11/11; Phase 24 passed 2/2; staged
cleanup passed 6/6; final orphan cleanup found zero active groups; and every
cleanup/spill count was zero.

The exact capture is rejected red. Its 685 cases ended 598 PASS/47 WARN/26
FAIL/13 BLOCKED/1 SKIPPED, and it proved 5,630/5,695 required assertions with
50 failed and 15 blocked. The 26 hard diagnostics classify as two approved
stock, zero approved intentional, and 24 unapproved: 22 Partisan and two
runtime. An obsolete fourteenth intentional-convoy classifier expectation
demoted 13 valid intentional-negative diagnostics; nine debug respawn-race
errors and two HQ arsenal teardown errors
complete the unapproved set. Certification and diagnostic acceptance therefore
fail independently. The portable summary is
`campaign-debug/partisan-rc-ee0e8add2a29-20260719T063815Z-full-20260719T072739Z.json`,
SHA-256 `e83bc1e752ac4c1abc5cb57ce097459642e17637f6747e4edc8e7d57569c1884`.
The candidate/package remains immutable; source fixes require a new candidate.
Release remains `NO-GO`.

Source commit `12f87e9` subsequently repairs the obsolete classifier
expectation and exact HQ teardown identity for a future candidate. It requires
the 13 retained intentional convoy lines as 9 admission, 3 corruption, and 1
watchdog diagnostic, expands the classifier to 36 self-tests, and caches the
exact HQ arsenal prefab identity before teardown. This is repair-history only:
it does not alter the portable ee0 summary or its captured 33-check census.

Source commits `64d1f70` and `ebaaeca` then repair compacted paid-support
confirmation replay and bounded convoy-contact seating recovery. The archive
proof now intentionally supplies no live planning services; the convoy retry is
limited to exact degraded/rebind evidence, runtime roots, living unseated crew,
five-second cadence, and the 45-second grace. No portable runtime result is
attached here: both changes await a new candidate.

The then-active candidate's package-bound focused set is accepted as
`passed-noncertifying`. Clean harness `b1940f2` ran all five cases against the
exact staged packed candidate from `2026-07-19T04:44:01.2295133Z` through
`2026-07-19T04:45:58.8756237Z`: JUnit 5/0/0/0, all 40 envelope files rehashed,
11 hard diagnostics = ten approved stock + one approved intentional + zero
unapproved, and zero cleanup/spill residue. The portable summary is
`focused-autotest/partisan-rc-e11e7ea88a44-20260719T040154Z.json`, SHA-256
`9ddade1cb86a209acf4aae02ded6f1a7713fe1e25ba577ae00ef1980e3de149a`.
This closes only the deterministic-service rung. The native-engine/world rung
failed for this package at the corrected `force_authority` canary. Clean harness
HEAD `937c86c5d2259a9da270ea76371001ac1d4c6eed` retained run
`seed1985_t0_p1_u1784437399` under leaf
`20260719T050302Z-0bbd740b4f0149baa0f34944dbd70fc9` from
`2026-07-19T05:03:02.0611638Z` through
`2026-07-19T05:03:41.5393020Z`. Candidate identity, packed mount, package
bytes, and artifacts remained exact and stable. The 11 cases ended at 8 PASS,
1 WARN, 1 FAIL, 1 BLOCKED, and 0 SKIPPED; focused proof was 33/35 and 85/87,
with `ownership_transition.aggregate` and `ownership_transition.causes` failed.
State restoration was 18/0, final orphan cleanup passed, all ten envelope files
rehashed, and cleanup/spill residue was zero. The classifier was valid with two
approved stock diagnostics and zero unapproved diagnostics. Envelope SHA-256 is
`8deca62633394025bfa976f6d883f9b500d56519fd13e875f241679f4799cd21`.
The rejected portable summary is
`campaign-debug/partisan-rc-e11e7ea88a44-20260719T040154Z-corrected-canary-20260719T050302Z.json`,
SHA-256 `af0aca25a84d8f757dbba8010950a658ce09937aa4048c35b2e372f1183eec69`.
The full profile was correctly stopped and remains `not-run`; release remains
`NO-GO`.

The historical e11 canary isolated a stale proof-fixture contract rather than
package drift.
Production now requires `FindActiveMission` to resolve the supplied source
before classifying a capture as mission-caused, but the ownership proof fixtures
still supplied unbacked mission IDs. The source correction seeds the exact
active missions in both fixtures, pins mission cause/type/ID provenance, retains
an unresolved-source negative case as military capture, and separates political
from mission diagnostics. Because that correction changed source, it was sealed
in the retained ee0 candidate. Its focused rung and corrected canary pass as
package-bound noncertifying evidence. Its exact-package full profile is the
rejected red boundary above; no evidence transferred from the rejected e11
package.

`partisan-rc-c2b16c4a2d85-20260718T201442Z`,
`partisan-rc-b8deddc4b631-20260718T213322Z`,
`partisan-rc-0e632ec4f63e-20260719T004133Z`, and
`partisan-rc-e11e7ea88a44-20260719T040154Z` remain sealed as superseded or
rejected artifact evidence. The 0e candidate retains its accepted focused and
corrected-canary results plus its rejected full result. The e11 candidate
retains its accepted focused and rejected corrected-canary results and no full
result. None of their package-bound results can be attached to the retained ee0
candidate, whose runtime rejection also forbids any further attached evidence.

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

The accepted classifier-aware five-case summary for the superseded
`partisan-rc-0e632ec4f63e-20260719T004133Z` candidate is
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

The accepted corrected-canary summary for that superseded candidate is
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

The immutable rejected full-profile summary for that superseded candidate is
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
This is a rejected red historical full-profile boundary. That candidate's
native-engine/world rung is `failed`; its map-locator source fix is sealed only
in the retained rejected replacement and may not be attached to this immutable
package.

Canonical LF is required for the tracked summaries, and the release generator
rehashes and cross-checks them against current status without transferring the
prior package's evidence to the new package.

If a package defect requires a rebuild, the older record remains byte-identical
and is described as retained superseded evidence. It remains available for
archive validation and consumer preflight, but not current runtime proof. No
result from that package may be combined with the replacement candidate's
evidence chain.
