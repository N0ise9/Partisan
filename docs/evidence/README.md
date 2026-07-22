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
five-suite/40-blob tree remains in the immutable external evidence root. Each
`sourceRuns[].runEnvelopePath` is relative to that supplied root and must use
`<candidate-id>/focused-autotest/<profile>/<run-leaf>/run.json`. Publication and
release-doc consumption independently reopen the five envelopes, exact eight-
blob inventories, candidate manifests/seals, JUnit results, logs, and hashes.
The exact suite testcase counts are 14, 13, 17, 6, and 41. The active contract
requires all 91 individually named testcases at JUnit 91/0/0/0, while preserving
individual-case and whole-suite selection. The journal suite binds its 51
aggregate diagnostics to exact testcase intervals: 10 stock plus 41 intentional
native-save diagnostics.
The index binds the focused-run harness Git blobs plus the committed aggregate
producer and release-doc consumer. A mismatch produces red
`replacement-required` evidence; it never rewrites an accepted aggregate.
Package identity is independently recomputed from the exact canonical four-file
manifest tuple set. Console admission censes all suite-start and profile-success
shapes, and a replacement receipt has authority only when its candidate ID plus
package, manifest, and ready-seal hashes match the retained candidate.
Historical five-testcase JUnit evidence remains valid only for candidate
ancestry at or before `075558ac7b6c14d1bb3e5829a2b87f3dbb608351`; it remains
history and cannot satisfy the active 91-testcase contract.

New corrected `force_authority` canaries and full Campaign Debug runs share the
portable schema-2 Campaign Debug release-index envelope. Their tracked
`release-index.json` remains byte-identical to the copy in the external raw
bundle, whose canonical ten files are reopened and rehashed by the consumer.
The corrected-canary policy additionally derives the exact case/assertion
census, rejects hidden assertion failures or skips, and requires the current
green census of 9 PASS/1 WARN/0 FAIL/1 BLOCKED/0 SKIPPED.
`cleanup.player_marker.live` under `cleanup.player_marker_completion` is the
sole non-certifying warning; `isolation.world_scope` under
`cleanup.state_isolation_restore` is the sole explicitly later-external
non-certifying blocker. Any unexpected or certification-counting blocker is
red. The evidence closes only as scoped `passed-noncertifying` or red
`failed-corrected-canary`. Existing schema-1 summaries remain immutable
historical evidence and are not rewritten into the new format.

The consumer chooses Schema 1 or Schema 2 from the retained summary schema and
evidence kind, not from the shared `passed-noncertifying` status. Active Schema-2
evidence requires stationary current tool bytes. Historical Schema-2 evidence
instead reopens the recorded immutable Git blobs and ancestry so later legitimate
tool changes do not invalidate already accepted history.

Tool hashes in schema-2 evidence are admission boundaries. Run publication and
consumer self-tests only from a stationary committed tool set; dirty producer,
runner, candidate-module, or consumer bytes correctly fail closed.

## Gate 1 Release-Surface And Retention Evidence

An active candidate may attach one paired Gate 1 evidence set under
`release-surface-audit/` and `gate1-runtime-retention/`. Each tracked file is a
portable `release-index.json`; the complete raw run remains in the immutable
external evidence root. The release-doc consumer requires an explicit evidence
root, reopens the raw `run.json` and terminal `run.ready.json`, rehashes their
complete file census, verifies the clean harness Git ancestry and tool blobs,
and exact-matches the candidate ID, source HEAD, manifest and ready hashes,
package digest, and candidate binding. It then invokes each exact Git-bound
publisher in read-only verification mode so the full raw semantics, canonical
index bytes, strict scalar types, and terminal seal are revalidated by the same
contract that published them. One member without the other fails
closed, and an accepted Full Campaign Debug result cannot advance without both.

The paired standard/diagnostic release-surface audit derives its member plan
from the candidate commit and probes the loaded package for all 67 forbidden
and 91 production-observability member surfaces. Inert
`ScriptModule.CompileScript` snippets cover methods, constants, and the guarded
`forceDebug` signature; typename metadata covers fields. Both modes require
supported compiler and metadata controls, so unavailable inspection fails
closed and cannot be reported as absence. Standard mode must report all
forbidden members absent and all production members present; diagnostic mode
must report all 158 present. The 9 forbidden literals remain candidate-bound
source-guard evidence; the audit makes no package-byte string-absence claim. A
standard launch uses `ArmaReforgerServer.exe` without any script definition;
the diagnostic launch uses `ArmaReforgerServerDiag.exe` plus exactly
`-scrDefine ENABLE_DIAG`. Executable provenance and preprocessing mode are
independently bound, and the publisher rejects any other topology. A
passing audit also resolves the type surfaces, deliberately generates the
production command menu, and invokes the read-only availability query for every
production command ID without executing a command action. It reports
`passed-noncertifying-release-surface-audit` with
`certificationPromotion: none`. Member-presence probes are inert; the audit
deliberately invokes production menu generation and read-only per-command
availability inspection, but executes no command action and does not mutate
campaign gameplay state. It does not certify gameplay, multiplayer,
persistence, restart, soak, or performance. The guarded child inherits no
standard streams, so its three required engine logs and any engine-emitted
`crash.log` are authoritative; an absent crash log is never synthesized. A
successful mode requires `crash.log` to be absent or whitespace-only and every
file below the log root to be one of the exact bound leaves. The machine-bound
`script-engine-and-process-fatal-v1` predicate covers `SCRIPT`/`ENGINE` error
severity, process-fatal signals, and audit `ERROR` markers; other retained
engine-channel severities remain outside this narrow predicate. Each mode's
Schema-2 census accepts either no policy-matched hard diagnostic (`0` raw
lines and `0` events) or the exact stock shutdown cluster observed on the active package:
two support-station catalog-manager events, each mirrored once across
`console.log`, `script.log`, and `error.log` after the passing result and
replication completion but before game destruction (`6` raw lines and `2`
events). The timestamp-free passing-result payload must appear exactly once in
each of `console.log` and `script.log`, and both timestamps must parse exactly.
The later result timestamp must precede replication finishing; replication
finishing, replication finished, and destruction then increase strictly.
Partial, extra, duplicated, malformed or reversed lifecycle, event-mirror
timestamp drift, message variants, non-empty bodies, misplaced, crash-channel,
and unapproved policy-matched events fail closed. The
runtime-retention bundle preserves diagnostic save-lineage evidence with the
same exact symbol pair and then observes the exact bytes through standard
server/client load-start-log runs that contain no script definition. It
reports `passed-noncertifying-retention`, keeps `certificationClaim: none`, and
keeps `standardSaveRestorationCertified: false`. Neither record promotes
release certification or substitutes for later multiplayer, restart, soak,
performance, or canary proof.

The source contract covers 55 wholly guarded carriers, 39 mixed files, 321
forbidden types, 71 forbidden commands, 67 forbidden members, and 9 forbidden
literals. Its production controls are four types, three commands, and 91
members. The source guard passes 15/15, the paired runner self-test passes 48
  checks, the surface and retention publisher suites pass 65 and 71/71 checks,
and the consumer passes 3 valid/optional plus 49 adversarial cases. These source
and publisher checkpoints do not constitute a runtime audit. The fourth and
fifth fresh audits each produced internally passing retail and diagnostic mode
records, exact 41-file censuses, zero crash artifacts, and exact cleanup. The
fourth stopped before publication when Git discovery returned multiple
application records. The fifth reached the corrected publisher but exact blob
validation rejected one CRLF worktree tool against its LF commit blob. Neither
directory has a release index or ready seal; both are unsealed diagnostic
residue, not tracked or accepted evidence. Shared bound tools are now LF-pinned,
and both runners check exact worktree/commit blob identity before engine launch.
The sixth audit passed that preflight and started retail. The passing-result
payload appeared exactly once in each result log, but the two engine timestamps
differed by one millisecond; the exact `6 raw / 2 event` shutdown cluster and
remaining lifecycle were otherwise valid. Full timestamped-line equality
rejected the mode, left `completedModeCount` at zero, and prevented the
diagnostic launch. Its external directory has a failure seal but no `run.json`,
release index, or ready seal; cleanup was exact. The semantic-payload/later-
timestamp correction is tooling-only, and this failure-sealed directory is also
diagnostic residue rather than tracked or accepted evidence. A fresh audit was
required against the unchanged package and is recorded next.

Seventh surface audit
`20260722T025639Z-ee290ff3af0f46908593dbf3002050bb` supplied that fresh run
under clean harness HEAD `11a3df0`. Retail and diagnostic each passed with exact
`0 raw / 0 event` diagnostics, no crash artifacts, a complete 41-file evidence
census, and exact cleanup with zero residue. Its terminal release-index SHA-256
is `2f38ea041a7a76281b093240a7c36635f2e6bed38646f4b76254153dca4adc49`,
  and the independent zero-write verifier passed. This is immutable passing
  release-surface evidence under its recorded tool bytes.

The first retention invocation failed before run-directory creation or engine
launch because dot-sourcing the ordinary persistence library reset same-named
`ClientExecutable`, `WatchedRoots`, `SpillRoots`, `StageTimeoutSeconds`,
`PollMilliseconds`, and `ResultGraceSeconds` caller values to defaults. The
runner now forwards every overlap, and the 64th retention-publisher regression
executes the actual import boundary with distinct sentinels. This pre-run failure
  did not invalidate the surface result at that checkpoint.

Retention retry `20260722T031531Z-434ebf5a6831` completed four diagnostic stages
and emitted a successful fifth-stage result before a normal-exit liveness/CIM-
recapture race returned `PGR_WAIT_IDENTITY_UNKNOWN`. It published no run envelope,
index, ready seal, or standard-retention context. Preserve its unique permanent-
NO-GO guard and session as unsealed forensic evidence. The corrected process
policy moved the 40-child reproduction from 27 false unknowns to zero and keeps
live inspection failure, mismatch, and unreadable state fail-closed. Owned-run
failures now write read-only `cleanup.json` plus atomic create-only
`run.failure.json`; the finalizer deletes nothing and preserves session or guard
bytes still present when it begins. A late failure may retain partial `run.json`
or release-index files, which the cleanup audit records, but it adds no ready
seal or success output. The completed shutdown stage's three stock backend-
identity and one stock editor-teardown VM
exceptions remain classified in its exact optional crash log; they are not an
exception-free claim or the later guard failure.

The identity correction changes the shared guarded-runtime blob recorded by the
seventh surface evidence. That result cannot pair with current retention, so both
halves require a fresh current-tool run against the unchanged package. The active
candidate still has no accepted pair, `STATUS-008` remains open, and release
remains `NO-GO`.

The next fresh surface attempt,
`20260722T041412Z-12c9176117444c9cb734fbb80ed0e31f`, bound clean harness
`e22da19`. Retail finalized as a pass. Diagnostic wrote a passing 321/4 type,
71/3 command, and 67/91 member probe/result, then completed replication and game
destruction. Roughly 132 milliseconds after the final log, the global ownership
census queried the now-exited ledger-owned process image and failed before the
diagnostic receipt/mode record. The create-only failure seal and cleanup record
are exact; there is no run envelope, release index, ready seal, live engine,
listener, runtime-addon mount, or reparse residue. The V2 census now resolves the
ledger entry first and accepts dead only for that known observed process; live
inspection failure, mismatch, and all unclaimed PIDs still fail closed. This
attempt is permanent-NO-GO forensic evidence. The repair changes the shared tool
binding again, so another fresh surface run is mandatory before retention.

Corrected surface audit
`20260722T043428Z-6dfc9b8f53d249808d9f5f4f97516455` supplied that fresh result
under clean harness `fe018c1`. Retail passed at exact `0 raw / 0 event`;
diagnostic passed with the exact approved `6 raw / 2 event` stock shutdown
cluster. The exact 41-file census and cleanup left zero residue. Release-index
SHA-256 `52bb83ffd810760eba27e7ca6ee490710fdb61bcc2e87f99e29c32ec63823ad5`
passed independent zero-write verification, so this is the accepted surface half
for the unchanged candidate.

Retention run `20260722T043633Z-4caead8fcfba` completed all five diagnostic
stages but zero standard stages. Real native saves existed below
`profile/.save/game`, while the snapshotter read `.save/game` one level too high;
the retained lineage therefore had journal bytes but no native bytes. The first
standard server reported its requested UUID missing, started a new playthrough,
and selected `startup source profile_fallback`. Its live console was also still
growing, so the obsolete requirement for two identical whole-file hashes timed
out. The failure-sealed run has exact cleanup, no run envelope, index, ready
seal, success output, or live residue. It is noncertifying harness-failure
evidence, not a package pass or demonstrated package defect.

Retention snapshots now read the actual profile subtree but preserve the stable
portable `files/native/.save/game` namespace; restore prepends `profile/`.
Before standard launch, the runner verifies manifest hashes and census, exact
copied rows, unique requested UUIDs, `m_sMissionResource`, exact `2/1/8` save
types, and nonempty `System/` payloads. The publisher independently requires the
exact stage save sets and retained standard-console source/restore markers.
Readiness now requires two consecutive marker-positive reads while allowing log
growth, with process identity checks before and after reading; native stages
reject missing loads or new playthroughs and require `startup source native`,
while fallback requires zero load authority and exact `profile_fallback`.
Bounded failure state is path-free. The no-engine suite passes 71/71.

Only retention-specific bound tools and Markdown changed after the accepted
surface run. The evidence consumer permits distinct descendant harness commits
and independently exact-checks each half's recorded blobs against current bytes,
so the surface result remains eligible for the pending retention pair.

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

The checked activation now records three ordered historical entries. `history[0]`
is `partisan-rc-0e632ec4f63e-20260719T004133Z` with
`rejected-after-full-profile`. `history[1]` is
`partisan-rc-e11e7ea88a44-20260719T040154Z` with
`rejected-after-corrected-canary`; its own focused and rejected corrected-canary
evidence are required and `fullCampaignDebug` remains absent. `history[2]` is
`partisan-rc-ee0e8add2a29-20260719T063815Z` with
`rejected-after-full-profile`; its own focused, accepted corrected-canary, and
rejected full-profile evidence remain attached only to that package. The
activation replaced the complete current-candidate surface with the 5b identity
without duplication or evidence transfer. The `NO-GO` decision remains unchanged.

The active Gate 1 candidate is
`partisan-rc-5b1f2e98f931-20260721T193941Z`, version
`0.1.0-rc.20260721T193941Z.5b1f2e98`, built from clean source HEAD
`5b1f2e98f93137230e686312c6e99cea7630dae4`. Its exact four-file package
SHA-256 is
`af22d6322a215dbef466e49041fc07395cbb5ed7a5951fd3e0cee5f4a101f530`,
manifest SHA-256 is
`bef040090557ca1403d6505e2bd1150452ba467225a29adc4295641f5aa3c80c`,
and ready-seal SHA-256 is
`173434122dce60dde8ff1dc939e2d5a916094bdb31096a641850772aa9853ad3`.
Foundation passed 985 references. All five Workbench targets passed at 5,849
files/12,022 classes with common CRC `aeddce9b`; the seal binds four package
files and 50 evidence files. The tracked manifest and ready seal are the portable
identity records; the package and complete raw evidence bundle remain external,
immutable, and untracked. Corrected release-surface run
`20260722T043428Z-6dfc9b8f53d249808d9f5f4f97516455` remains accepted under its
recorded tool bytes and independently verified 41-file bundle. Only retention-
specific bound tools and Markdown changed afterward. Complete fresh corrected
retention and consume it with that surface half, then run the 91-case focused,
corrected-canary, and Full Campaign Debug gates in order against the unchanged
candidate.

Historical ledger `history[2]`, retired as `rejected-after-full-profile`, is
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
expectation and exact HQ teardown identity. It requires
the 13 retained intentional convoy lines as 9 admission, 3 corruption, and 1
watchdog diagnostic, expands the classifier to 36 self-tests, and caches the
exact HQ arsenal prefab identity before teardown. This is repair-history only:
it does not alter the portable ee0 summary or its captured 33-check census.

Source commits `64d1f70` and `ebaaeca` then repair compacted paid-support
confirmation replay and bounded convoy-contact seating recovery. The archive
proof now intentionally supplies no live planning services; the convoy retry is
limited to exact degraded/rebind evidence, runtime roots, living unseated crew,
five-second cadence, and the 45-second grace. Those changes are sealed in the
active 5b candidate, but no portable runtime result is attached for them.

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
in the then-active ee0 candidate. Its focused rung and corrected canary pass as
package-bound noncertifying evidence. Its exact-package full profile is the
rejected red boundary above; no evidence transferred from the rejected e11
package.

`partisan-rc-c2b16c4a2d85-20260718T201442Z`,
`partisan-rc-b8deddc4b631-20260718T213322Z`,
`partisan-rc-0e632ec4f63e-20260719T004133Z`,
`partisan-rc-e11e7ea88a44-20260719T040154Z`, and
`partisan-rc-ee0e8add2a29-20260719T063815Z` remain sealed as superseded or
rejected artifact evidence. The 0e candidate retains its accepted focused and
corrected-canary results plus its rejected full result. The e11 candidate
retains its accepted focused and rejected corrected-canary results and no full
result. The ee0 candidate retains its own accepted focused and corrected-canary
results plus its rejected full result as `history[2]`. None of their package-
bound results can be attached to active candidate
`partisan-rc-5b1f2e98f931-20260721T193941Z`.

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
