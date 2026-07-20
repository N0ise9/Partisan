# Partisan Architecture

> Current release status and the strict proof ladder are generated in
> [`CURRENT_STATUS.md`](CURRENT_STATUS.md). Behavioral ownership and adaptation
> requirements are generated in
> [`ANTISTASI_CE311_PARITY_MATRIX.md`](ANTISTASI_CE311_PARITY_MATRIX.md).

Current sealed implementation/source identity:
`32727238d74b29905c68e5a80bb5897dfdc783c0`, UTC
`2026-07-18T16:34:38Z`, label
`schema71-settings24-focused-force-authority`. Campaign Schema 71 and
runtime-settings Schema 24 are current.
That identity adds the focused force-authority checkpoint and retains the
passing mixed-native proof described below.

## Release-Ledger Schema 3

Release-ledger Schema 3 is checked release metadata; it does not change
Campaign Schema 71 or runtime-settings Schema 24. Its
`historicalCandidateEvidence` property is a true JSON array with one or more
entries in oldest-to-newest retirement order. Every entry has exactly
`retirementDisposition`, `candidate`, and `evidence`. The supported retirement
topologies are deliberately closed:

- `rejected-after-full-profile` requires the package-bound focused result, an
  accepted corrected canary, and the rejected full-profile result.
- `rejected-after-corrected-canary` requires the package-bound focused result
  and rejected corrected canary, and forbids a `fullCampaignDebug` property.
  Absence is evidence truth: a stopped full run may not be represented by
  `null`, a placeholder, borrowed evidence, or a fabricated summary.

Every historical candidate ID, source HEAD, manifest path and digest, ready-seal
digest, and package digest must be distinct from every other historical entry
and the current artifact. Each referenced summary, envelope, run identity, and
hash must still bind to that same candidate. Candidate creation, focused,
canary, and permitted full-profile times must follow their gate order; the
corresponding source and harness commits must form the required Git ancestry
chain through the current checkout. Array position is therefore asserted
history, not presentation sorting.

The checked ledger now has two ordered entries. `history[0]` is
`partisan-rc-0e632ec4f63e-20260719T004133Z`, retired as
`rejected-after-full-profile`; `history[1]` is
`partisan-rc-e11e7ea88a44-20260719T040154Z`, retired as
`rejected-after-corrected-canary`. The e11 entry retains its own focused and
rejected corrected-canary evidence while `fullCampaignDebug` remains absent.
The same checked activation replaced every current-candidate field with the new
ee0 identity, so no current/history identity is duplicated or mixed. Release
remains `NO-GO`.

## Release-Candidate Build Boundary

`tools/new-guarded-release-candidate.ps1` is the single Gate-1 build-once
boundary. It accepts explicit diagnostic Workbench, packed dependency, standard
server, standard client, monitoring-root, and external output inputs. It rejects
a dirty checkout, runs Foundation, validates the project separately for PC,
XBOX_ONE, XBOX_SERIES, PS4, and PS5, and retains each target's raw guarded logs,
transcript, result, and cleanup record before packing.

Packing uses a nonce-owned checkout-local scratch directory because native
Workbench packing requires that shape. The scratch directory is fresh,
reparse-free, sentinel-owned by the exact wrapper process, and removable only
after an engine-process census and exact ownership revalidation. Candidate
content is assembled under a unique external partial directory. A successful
pack must contain exactly `Partisan/addon.gproj`, `Partisan/data.pak`,
`Partisan/resourceDatabase.rdb`, and `Partisan/thumbnail.png`; the packed
project ID, GUID, title, and dependency set must equal the source project
identity, and the packed thumbnail must be byte-identical to the tracked source
thumbnail. Per-file SHA-256 values are sorted into the portable
`sha256-manifest-v1` index, whose digest is the package identity.

The release manifest binds checkout HEAD and dirty state, embedded build stamp
and Git relationship, both persisted schema versions, source and packed addon
identity, diagnostic Workbench identity and every target result, standard
client/server identities, package files, and every retained evidence file. The
manifest is checked before and after the partial directory is renamed to its
final candidate identity. A matching ready seal is the last atomic publication
operation. A failed run keeps its uniquely owned partial or quarantined
evidence for diagnosis; it never publishes an unsealed directory as a
candidate.

`tools/run-guarded-workbench-validation.ps1` therefore has two explicit
release-evidence inputs: one validation target and one fresh retained-evidence
directory outside its disposable guard. It copies raw evidence only after
process quiescence, preserves relative paths, rejects destination escape, and
then removes only its exact owned guard.

### Schema-2 Raw-Evidence Publication Boundary

Future package-bound focused evidence begins with five serial, individually
named raw run envelopes retained outside the checkout, eight files per run.
`tools/New-PartisanFocusedAutotestAggregate.ps1` admits exactly that five-run,
40-file tree, rehashes every blob, and publishes the tracked Schema-2 focused
index. `tools/update-release-docs.ps1` is an independent consumer: through
`-EvidenceBundleRoot` or `PARTISAN_RELEASE_EVIDENCE_ROOT`, it reopens the
external raw tree, repeats the path, identity, and hash checks, and rederives
the result instead of trusting the tracked index alone. The aggregate's 35 `aggregate-policy`
assertions describe index-admission policy only; they do not become Campaign
Debug assertions and do not prove gameplay or packaged-runtime behavior.

Corrected-canary evidence follows the parallel ten-file path. The guarded
runner retains the raw Campaign Debug bundle outside the checkout,
`tools/New-PartisanCampaignDebugReleaseIndex.ps1` derives a Schema-2 release index,
and the tracked copy must remain byte-identical to the externally retained
index. The release-doc consumer independently reopens the same raw root and
rederives the ordered case, assertion, certification, diagnostic, cleanup, and
state-diff contracts. The current acceptance contract is exact: 11 cases at 9
PASS, 1 WARN, 0 FAIL, 1 BLOCKED, and 0 SKIPPED; 91 ordered assertion rows;
87/87 certifying rows; `cleanup.player_marker.live` in
`cleanup.player_marker_completion` is the sole non-certifying warning; and
`isolation.world_scope` in `cleanup.state_isolation_restore` is the sole
explicitly later-external non-certifying blocker. Any unexpected or
certification-counting blocker is red, and the exact 18-label state-diff set
must remain zero.

Both publishers bind candidate, package, run identity, and the committed Git
blobs for the runner, candidate module, publisher, and release-doc consumer.
They rederive the package digest from the exact four canonical
`path`/`indexPath` tuples instead of trusting the declared package hash. The
focused console contract also censes every suite-start and profile-success
marker globally, so an additive foreign marker cannot hide beside the expected
pair. Focused rejection receipts bind the candidate ID plus the package,
manifest, and ready-seal hashes; a same-ID receipt with different seals has no
precedence authority.
Accepted evidence bytes are immutable. A later rejected attempt records a
candidate-bound red replacement receipt or rejected index and cannot overwrite
the accepted artifact. Historical Schema-1 readers and evidence remain intact
for their original candidates; routing is based on the retained summary schema,
not a status shared by both schemas. Historical Schema-2 validation reads the
recorded immutable Git blobs and remains valid after legitimate later tool
maintenance, while active evidence still requires current worktree bytes to
match those blobs. No Schema-1 result is converted or transferred.
This publication architecture changes neither Campaign Schema 71 nor runtime-
settings Schema 24. Its static contracts and self-tests do not advance a gate,
execute runtime, or certify runtime behavior; only new evidence captured in the
ordered package-bound gate sequence can do so.

This boundary produced the first retained candidate,
`partisan-rc-c2b16c4a2d85-20260718T201442Z`, from clean source HEAD
`c2b16c4a2d85e71503cd46265feafb54bce69e83`. Its canonical four-file package
digest is `8f60260331c6c7473465dc4517b1063a179a8f4efeffdcfe3d5eccac9af476db`.
The exact manifest and ready seal are tracked under `docs/evidence` so generated
status can reject identity drift. The candidate remains retained-uncertified:
its artifact-identity evidence remains valid, but the focused-suite defect below
makes it ineligible for current runtime proof. No later runtime evidence may be
attached to it or combined with its replacement.

The first replacement was
`partisan-rc-b8deddc4b631-20260718T213322Z`, built from clean source HEAD
`b8deddc4b6314936b7ea04f36a35784622a46da6`. Its canonical package digest is
`82e1fd0bf7c3404b7fe842fa84efd10f225bf82fc76c11502b9a684b63f4f329`, and
all five Workbench targets pass at 5,846 files, 11,899 classes, and common CRC
`f27e637b`. Its accepted focused set and rejected canary/full captures remain
immutable evidence for that package only. The later map-locator correction
changed packaged source, so this candidate is now superseded for current
runtime proof.

The retained `rejected-after-runtime` candidate is
`partisan-rc-ee0e8add2a29-20260719T063815Z`, version
`0.1.0-rc.20260719T063815Z.ee0e8add`, built from clean source HEAD
`ee0e8add2a298e83fd304b7660c4fc480dc6383f`. Its exact four-file package
digest is `981258439b9d08866c4883471cacfe33aa373a36a667a39e8c939f285db74daf`;
the manifest SHA-256 is
`1b877e3aa21773a268704bcb3fe889768fca3aa2d78541aa7285b061398ce907`,
and the ready-seal SHA-256 is
`01741b85d0edba69f54b07388cdd7c452b8f6f1ad7ef4f6faf253918a4bbf280`.
Foundation passed all 874 checks. Every PC, XBOX_ONE, XBOX_SERIES, PS4, and PS5
Workbench target passed at 5,848 files, 11,901 classes, common CRC `f64e0868`.
The seal binds four package files and 50 evidence files. At approximately
`2026-07-19T07:02Z`, clean harness HEAD
`273ed14ba8526259c8b0d248177fa53b59ade683` passed all five canonical packaged
focused cases against the exact candidate and packed mount: JUnit 5/0/0/0, all
40 envelope files retained, 12 classifier checks passed per run, 11 diagnostics
= ten approved stock + one approved intentional + zero unapproved, and zero
cleanup/spill residue. Deterministic-service advances only to
`passed-noncertifying`. At approximately `2026-07-19T07:14Z`, clean harness
HEAD `4f8d7e2d7a39896737fd6754060523bf852c5fa8` then accepted the corrected
`force_authority` canary against the exact unchanged candidate and package. Run
`seed1985_t0_p1_u1784445266` recorded 11 cases at 9 PASS/1 WARN/0 FAIL/1
BLOCKED/0 SKIPPED, all 35/35 focused assertions and 87/87 certification-
counting assertions proven, 18/0 state restoration, ten rehashed envelope
files, two approved stock diagnostics plus zero unapproved, final orphan
cleanup, and zero cleanup/spill residue. Native-engine/world therefore advances
only to `passed-noncertifying`. At approximately `2026-07-19T07:27Z`, clean
harness HEAD `a5ccf36aee17a4f88d7f1c2f232ce9fc14652018` ran Full Campaign Debug
against the exact unchanged package. Run `seed1985_t0_p1_u1784446076`, leaf
`20260719T072739Z-97fc069d58cd427c848c83f99f39e5f9`, completed a mechanically
valid ten-file capture with stable candidate/package identity, envelope SHA-256
`fce4928444f15531f254ad4d7e119cf8bfe1d06e6fcb564518d2e052544d4278`,
18/0 state restoration, Phase 17 at 11/11, Phase 24 at 2/2, staged cleanup at
6/6, zero final orphans, and zero cleanup/spill residue. The independent result
is rejected red: 598 PASS/47 WARN/26 FAIL/13 BLOCKED/1 SKIPPED across 685
cases, with 5,630/5,695 required assertions proven, 50 failed, and 15 blocked.
Its 26 hard diagnostics classify as two approved stock, zero approved
intentional, and 24 unapproved: 22 Partisan and two runtime. An obsolete
fourteenth intentional-convoy
expectation demoted 13 valid intentional-negative diagnostics; the remaining
clusters are nine debug respawn-race errors and two HQ arsenal teardown errors.
Certification and diagnostic acceptance both fail independently.

Post-rejection commit `12f87e9` changes only future source behavior. The guarded
classifier now owns an exact 13-line intentional-convoy boundary split into 9
admission, 3 corruption, and 1 watchdog line, with 36 self-tests. Support-station
teardown no longer discovers identity after components begin disappearing: the
exact HQ arsenal prefab is cached during post-init, and only a cached match plus
a null catalog manager can bypass stock teardown. A new immutable package must
prove both changes; the rejected ee0 evidence is unchanged.

The next remediation layer preserves the authority split. Commit `64d1f70`
treats compacted confirmation as a read of sealed tombstone authority and checks
that path before dependencies needed only by live admission. Commit `ebaaeca`
keeps contact reseating forbidden except for a bounded recovery predicate over
exact convoy state, approved degraded/rebind modes, real runtime roots, living
unseated crew, cadence, and deadline. Neither change creates a second operation
owner or a persistent recovery state.

The historical e11 candidate's package-bound focused set passed under clean harness
`b1940f2` from `2026-07-19T04:44:01.2295133Z` through
`2026-07-19T04:45:58.8756237Z`. All five exact staged packed-candidate cases
passed with JUnit 5/0/0/0; all 40 envelope files rehashed, cleanup/spill residue
was zero, and the exact diagnostic census was 11 = ten approved stock + one
approved intentional + zero unapproved. The deterministic-service rung is
accepted as `passed-noncertifying`; summary SHA-256 is
`9ddade1cb86a209acf4aae02ded6f1a7713fe1e25ba577ae00ef1980e3de149a`.
The corrected `force_authority` canary then failed the package's
native-engine/world rung under clean harness HEAD
`937c86c5d2259a9da270ea76371001ac1d4c6eed`. Run
`seed1985_t0_p1_u1784437399`, leaf
`20260719T050302Z-0bbd740b4f0149baa0f34944dbd70fc9`, completed from
`2026-07-19T05:03:02.0611638Z` through
`2026-07-19T05:03:41.5393020Z` with exact candidate identity, packed mount,
stable package bytes, and stable artifacts. Its 11 cases ended 8 PASS, 1 WARN,
1 FAIL, 1 BLOCKED, and 0 SKIPPED; the focused proof passed only 33/35 assertions
and 85/87 counted conditions because `ownership_transition.aggregate` and
`ownership_transition.causes` failed. State restoration was 18/0, final orphan
cleanup passed, all ten envelope files rehashed, cleanup/spill residue was zero,
and the valid diagnostic census was two approved stock plus zero unapproved.
Envelope SHA-256 is
`8deca62633394025bfa976f6d883f9b500d56519fd13e875f241679f4799cd21`;
portable summary SHA-256 is
`af0aca25a84d8f757dbba8010950a658ce09937aa4048c35b2e372f1183eec69`.
The full profile was correctly stopped. This historical package is verification-
only and runtime-ineligible; execution control must reject it while preserving
its immutable evidence. No evidence from another package can satisfy any rung.

The historical canary exposed stale source fixtures after production intentionally began
requiring `FindActiveMission` to resolve a mission source before assigning
`mission_capture`. The fixtures supplied mission IDs without matching active
missions, so their expected cause/provenance and aggregate proof no longer
matched production. The source correction sealed in the then-active ee0 candidate
seeds the exact active
missions in both affected fixtures, pins mission cause, source type, and source
ID, preserves an unresolved-source negative case as `military_capture`, and
reports political and mission provenance separately. This source change cannot
repair the immutable e11 package. The ee0 focused rung now passes on its own
bytes; historical state-only 35/35 and 87/87 proof remains nonpackage and did
not advance it. The ee0 corrected canary has now independently reproduced that
35/35 and 87/87 boundary against its exact package and clean harness as
`passed-noncertifying`; the subsequent Full Campaign Debug result is the
rejected red boundary above. The candidate and package remain immutable,
release remains `NO-GO`, and any source corrections require a new candidate.

The older historical candidate,
`partisan-rc-0e632ec4f63e-20260719T004133Z`, remains immutable historical
evidence. It was built from clean source HEAD
`0e632ec4f63eab43e8c301d0755f10193d85131f`; its exact four-file package digest
is `e5d29458c33aeef9cd2b37476359acc6021fe78cf0fc74513d9a2f69ef0614dc`,
manifest SHA-256 is
`ea06318a8f5161f000685fe37ecab4f5c8a77d6b0e8205f502a6418e3365e76b`,
and ready-seal SHA-256 is
`cd91e569b8a4a453dad6b0f884f22afbb36b9b5f0de629fd70b2188875e47c53`.
Foundation and all five Workbench targets passed at 5,847 files, 11,900 classes,
common CRC `3a399db1`, zero hard errors, and exact-zero cleanup. Its five focused
cases passed under clean harness HEAD
`d4d8f29cda9896ce2c6a5b073dac2cbd03757700` from
`2026-07-19T01:08:50.9577409Z` through
`2026-07-19T01:09:44.4465092Z`: aggregate JUnit 5/0/0/0, 40 rehashed envelope
files, zero cleanup/spill residue, all 12 classifier checks per run, and 11
diagnostics = ten approved stock + one approved intentional + zero unapproved.
Its deterministic-service rung was accepted as `passed-noncertifying`; portable
summary SHA-256 is
`961ef6b0a84c26446468b31dd7ac5120448b21a442e9a823de4ff5dc804da7f9`.

The same superseded package then passed the corrected `force_authority` canary under
clean harness HEAD `20375141f840f74316ca46e7df047fcba3e6e344`. Run
`seed1985_t0_p1_u1784424219`, leaf
`20260719T012319Z-47423d741d0e4690b3c7dbbbab68cebd`, completed its 40-second
canonical runtime from `2026-07-19T01:23:19.5837772Z` through
`2026-07-19T01:24:02.2143579Z`. The report recorded 9 PASS, 1 WARN, 0 FAIL,
1 BLOCKED, and 0 SKIPPED across 11 cases; the focused proof passed 35/35
assertions and 87/87 certification-counting conditions. State restoration was
18/0, final orphan cleanup passed, all ten envelope files rehashed, and every
cleanup/spill counter was zero. The exact diagnostic census was two = two
approved stock + zero approved intentional + zero unapproved, with all 33
classifier self-tests passing. The envelope SHA-256 is
`e3705a849590b9fd3086fdb0caf5659df6e0c1029784612965c848a0f8f0a851`.
The portable summary is
`docs/evidence/campaign-debug/partisan-rc-0e632ec4f63e-20260719T004133Z-corrected-canary-20260719T012319Z.json`,
SHA-256 `f47fa5f0539c0c8c6024e096f3e034699bc6bfaf656734a0a2b32c9fee7b4aa8`.
The prior map-locator exception is absent for this scoped path. This historical
result advanced that package's scoped native-engine/world canary rung only to
`passed-noncertifying`; its later full result remains the immutable rejected red
boundary described below.

Runtime consumers now bind the active tracked manifest and ready seal to the
external sealed bundle before accepting a launch. They independently require
the exact top-level layout, all four package files, the complete retained
evidence inventory, canonical package digest, active release-status identity,
base-game packed roots, and the standard executable recorded for the selected
client/server role. An active runtime candidate also seals the exact adjacent
diagnostic client and server identities because those binaries, not the standard
runtime files, execute the diagnostic gates.

The engine never mounts the sealed bundle directly. Each runner copies only the
four verified package files into its nonce-owned guard, recomputes the package
digest, and launches the staged packed project with one two-root add-on search
argument, the exact add-on GUID, and a guard-owned add-on temp directory. It
rehashes the staged copy and the external candidate after process quiescence.
Raw runtime artifacts are copied to a separate fresh sidecar evidence run before
the guard is removed; a portable envelope binds their hashes to the candidate,
harness commit and launch-time file hashes, diagnostic and standard runtime
identities, the guarded settings bytes actually consumed,
launch scope, outcome, and exact cleanup census. The candidate bundle itself
remains immutable.

Both Campaign Debug and individually named focused-runner preflights pass the
argument-round-trip, staging, candidate-revalidation, and exact-zero cleanup
contract. Real runs additionally require the engine-owned `gproj` log record to
name the exact guarded project path and add-on GUID with packed mode. A
seventeen-case consumer self-test admits all three dispositions for verification,
admits only `active-runtime-candidate` for runtime use, and rejects altered
package bytes, manifests, seals, unexpected bundle/package files, staged drift,
and ambiguous add-on roots. Review of the stock autotest
transition identified a release-critical package defect: three service-only
suites lacked the empty-world override already used by two peer suites, so a
base-only scenario transition could unload their test types before JUnit output.
Those three overrides are source-enforced in the first replacement. Each older
candidate remains sealed, superseded evidence; every runtime rung for the
current candidate must restart from its own package identity rather than mixing
evidence chains.

The first replacement-package execution emitted one passing JUnit result for
each of the five named cases, but it also exposed a harness acceptance gap:
hard diagnostics were retained without participating in the pass predicate.
Those sidecars remain preliminary evidence. The focused runner now accepts only
two exactly ordered diagnostic classes: the two stock post-result filter
constructor messages, and one profile-journal native-failure injection when its
non-mutating proof tokens are present. Any other script or engine error, wrong
count, wrong order, wrong case, or missing proof token fails the run. This
harness-only correction did not change the sealed package identity.

All five cases then passed against that unchanged earlier package under clean
classifier-aware harness HEAD
`b3fc1e6f56d9cf8805bac1702a54e0b5284e0043`. Aggregate JUnit was 5 tests, 0
failures, 0 errors, and 0 skips. Each
engine log attested the exact staged packed project and candidate boundary; all
40 retained envelope files rehashed, and cleanup plus monitored spill residue
was zero. The exact diagnostic census was 11: ten approved stock post-result
filter messages, one approved in-suite journal fault injection, and zero
unapproved errors. `HardDiagnosticFree:false` is therefore intentional while
classification validity is true. The portable accepted-set summary is
`docs/evidence/focused-autotest/partisan-rc-b8deddc4b631-20260718T213322Z.json`
with SHA-256
`8bb36919f0649e0f48fad50305878ec883cf98a0021323ba1442017f1aa113b8`.
This closes only the packaged deterministic-service rung as
`passed-noncertifying`; Full Campaign Debug and every higher runtime rung remain
independent.

## Retained Prior-Package Full Campaign Debug Boundary

The same unchanged candidate produced a mechanically complete and stable raw
capture under clean harness HEAD `1bff1890830db08159826f63b550227aa7bb0da3`.
The `force_authority` canary report recorded its focused case at 35/35 assertions
and 87/87 certification-counting conditions, with exact candidate and packed-
mount attestations, ten rehashed files, an 18/0 state diff, final orphan cleanup,
and zero cleanup or spill residue. An independent timestamp-aware audit of its
canonical script log found three raw `SCRIPT (E)` lines: two exact approved stock
diagnostics and one unapproved diagnostic. The canary capture is therefore
preliminary and unaccepted; its report-level PASS observations do not constitute
a passed canary evidence gate.

The full package-bound run `seed1985_t0_p1_u1784414040` likewise retained exact
candidate and packed-mount identity, ten stable rehashed files, zero state drift,
final orphan cleanup, and zero cleanup or spill residue. Its canonical script log
contains 25 raw `SCRIPT (E)` lines, of which 19 are Partisan diagnostics. Exact
classification accounts for two approved stock diagnostics and 13 proof-bound
intentional diagnostics, leaving ten unapproved diagnostics. The report itself
remains red at 584 PASS, 49 WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED, with
5,562/5,688 required assertions proven, 112 failed, and 14 blocked. The portable
summary is
`docs/evidence/campaign-debug/partisan-rc-b8deddc4b631-20260718T213322Z.json`.

The wrapper-reported success records the behavior of the original wrapper; it is
not evidence acceptance. Both raw captures remain immutable and useful for
diagnosis. Clean committed classifier harness `38a094f` then reran the
`force_authority` canary against the unchanged package. Its focused proof again
recorded 35/35 assertions and 87/87 certification-counting conditions, but the
corrected raw census was three = two exact approved stock diagnostics + one
unapproved map-locator VM exception. Cleanup and spill residue were zero. The
runner failed closed, so the full profile was correctly not rerun.

The rejected canary exposed a map-locator lifecycle defect rather than a
classifier defect. Setup opens the `PLAIN` map before bootstrap has a controlled
character. The immediate no-player locator update removes its hint root but
leaves the ten-second callback scheduled; after bootstrap supplies the player,
that callback can dereference the stale widget state. The narrow
`SCR_MapLocator.CalculateClosestLocation` adaptation now validates the hint
layout, both hint-text widgets, and world directions before entering stock
behavior. An invalid state removes any remaining layout, cancels the stale
callback, clears the widget references, and returns; a valid state still uses
the stock calculation unchanged. Foundation and PC Workbench compile validation
pass at 5,847 files/11,900 classes and CRC `3a399db1`, with zero errors or
residue. That is source/compile proof only. Because this correction changed the
package, the earlier candidate and its immutable captures remain preliminary-
unaccepted. The correction was sealed in the now-superseded
`partisan-rc-0e632ec4f63e-20260719T004133Z`. Its five packaged focused cases did
not exercise the map-locator lifecycle, but its accepted corrected canary
exercised the formerly failing scoped path without the exception. That is
immutable non-certifying scoped runtime proof for that package only. Its
subsequent full profile is a rejected red capture: wrapper integrity passed, but
certification failed at 584/49/46/7/1 with 5,561/5,687 required assertions
proven, and the classifier found ten unapproved hard diagnostics. That
candidate's native-engine/world rung is `failed`. Historical candidate
`partisan-rc-e11e7ea88a44-20260719T040154Z` likewise retains its accepted
package-bound focused set but failed its corrected canary at 33/35 and 85/87
on the two ownership-transition fixture assertions above. Its full profile was
stopped, and it is ledger `history[1]` with disposition
`rejected-after-corrected-canary`. The fixture correction is sealed in active
candidate `partisan-rc-ee0e8add2a29-20260719T063815Z`. Its packaged focused set
and corrected `force_authority` canary both pass as package-bound non-certifying
evidence. Its Full Campaign Debug run is independently rejected at
598/47/26/13/1 and 5,630/5,695, with 24 unapproved diagnostics despite exact
capture integrity and zero residue.
No historical or post-capture result can be attached to it.

## Historical Focused Force-Authority Engine Checkpoint

At the retained non-package checkpoint, the `force_authority` profile built and
finalized a self-contained typed case containing exactly the combat-presence,
ownership-transition, and town-
influence assertion groups. It no longer validates a selected subset against a
larger parent case containing unrelated assertions. The engine-executed,
deterministic fixture passed all 35 targeted assertions, the focused case was
`PASS`, and all 87 counted required conditions for that focused scope were
proven. The isolated capture/restore boundary finished with 18 state rows and
zero nonzero diffs; script errors, Partisan errors, crashes, owned cleanup, and
artifact instability were all zero.

Political influence now submits its exact transition receipt directly to the
canonical ownership service even when an earlier top-level receipt is still
pending. The ownership service remains the sole ordering authority and persists
the later receipt as queued; the outer fallback scan owns the single wait fence.
Fixture dependencies are retained by strong references, migration scoring uses
immutable target values, and malformed shared-event proofs corrupt only copied
save rows after capture rather than live result objects.

That historical checkpoint's stamped Workbench validation loaded 5,846 files
and 11,899 classes at CRC `cad640f3`, with zero HST, script, or hard errors and
exact-zero owned cleanup. This was isolated, state-only engine fixture evidence.
It is not full Campaign
Debug certification or live/native-world, packaged-server, client/network/JIP,
multiplayer, performance, or soak proof.

## Current Controlled-Shutdown Native-Authority Fence

Controlled shutdown is an ordered fail-closed transaction rather than a loose
collection of cleanup callbacks. Before any irreversible scope latch, the
persistence owner runs a complete read-only preflight across prepared loot,
rescue, durable field-vehicle, and active-force state. It then runs campaign
capture preparation and repeats that complete preflight against the prepared
state. Only a fully accepted second pass may apply the nearby-vehicle latch,
repeat preparation, and then latch active-group, field/civilian-vehicle, and
rescue scopes in that order. Those latches are one-way for that shutdown
attempt, and every retry maintains and revalidates their quiesced state instead
of reopening gameplay authority.

The coordinator freezes commander, member, and administrative commands while
the transaction drains or quiesces. Mission, casualty, and lifecycle ingress is
also rejected at that boundary, so a native save callback cannot race a new
campaign mutation. A rejection before the first latch leaves the live campaign
unlatched; a rejection afterward keeps the already-latched scopes fenced and
fails the controlled end instead of resuming a partially drained campaign.

Rescue authority pins both its detached DTO graph and the exact native captive,
carrier, and follower topology. Read-only readiness checks cover carrier root,
manager, seat, occupancy, player/proxy transition, damage, and foreign
occupancy before latching; an unexpected occupant fails closed. A valid
`BOARDING` captive may still be outside a seat, while an observed or `BOARDED`
topology must match the pinned seat and carrier identity. Shutdown then pins the
carrier's full transform and recursively stops controller, engine, braking,
autohover, movement, and dynamic-physics state. Captive-follower callbacks,
waypoints, AI movement, and physics are likewise quiesced. Retries validate the
durable DTO pins, restore the pinned transforms, maintain every one-way scope,
and finally require the strict native topology again before commit.

The prior controlled-shutdown stamped checkpoint passes Foundation at 874 script-symbol references and
Workbench at 5,846 files/11,899 classes, CRC `9a79a33a`, with zero HST, script,
or hard errors and zero owned cleanup residue. The ordinary five-process chain
passes automatic, manual, controlled-shutdown, native-restart, and profile-
journal-restart stages, advances generations 1 -> 2 -> 3, preserves the exact
controlled-end bridge and field-vehicle state, and keeps both restore stages
read-only with zero cleanup.

The sealed extension runs the same five stages with one dedicated
mixed-native authority graph. Three exact captive states cover following,
seatless boarding, and boarded seat ownership while the real player exercises
occupancy, release, and foreign-occupant rejection. The carrier retains one
durable HST identity across restart; the newly spawned root's process-local
`RplId` proves replication availability but never replaces that durable key.
The authored seat token survives reseating, the active guard rematerializes
from strategic authority, native and profile-fallback fingerprints remain
exact, and all cleanup counters are zero. This closes that fixed graph, not
arbitrary rescue, mission, force, or active-world shutdown combinations.

## Current Campaign Recovery Journal Boundary

Campaign Schema 71 gives native persistence and the profile recovery journal
one monotonic source-selection order. The campaign DTO now persists
`m_iPersistenceCheckpointSequence`; every successful capture advances it before
serialization and rejects sequence exhaustion. Source comparison orders the
checkpoint sequence first, the existing restore sequence second, and the save
second last. Equal order is legal only when the normalized snapshot
fingerprints also match. Administrative new-campaign reset retains the prior
checkpoint and restore sequences, then advances the next capture, so a stale
pre-reset save cannot regain authority.

The profile recovery path is a verified two-slot journal. The canonical and
recovery files each carry a versioned envelope whose payload is the exact nested
serialized `HST_CampaignSaveData` string. Generation, parent generation, current
fingerprint, and parent fingerprint form the chain. The fingerprint contract is
`uuidv8-sha256-v1:<serialized-length>:<UUID>`, where UUID v8 is derived from the
native SHA-256-based generator. This is an accidental-corruption integrity check,
not authentication against a malicious writer.

Each checkpoint writes only the inactive slot, reads that candidate back, and
reruns journal selection before the new generation is accepted. The previous
valid slot is not overwritten until a newer verified generation has superseded
it. Exact same-generation duplicates select the canonical slot deterministically;
same-generation payload or metadata disagreement, nonadjacent generations,
broken parent identity, unknown or future envelope/payload versions, and other
ambiguous history fail closed and fence later journal writes. The engine file API
does not provide atomic rename or an exclusive lock, so this is crash-tolerant
single-writer recovery rather than an atomic filesystem transaction. It is also
not an off-device backup.

A valid raw canonical campaign JSON from Schema 70 or earlier is treated as
generation zero. The first Schema-71 journal checkpoint writes recovery
generation one and preserves the raw canonical bytes. Profile-tree migration
applies the same conservative rule to both campaign slots: differing retired and
canonical authority remains in place and startup fails rather than choosing or
deleting one side.

New native rows use native envelope version 2, store the exact serialized
payload string, and validate its fingerprint before DTO parsing. This keeps an
intact v2 row valid when a later DTO adds fields. Schema-70 version-1 native rows
remain readable: their legacy length/hash identity is reconstructed and
validated first, then normalized before source comparison. An unknown or future native envelope or
campaign schema is preserved and is fatal before journal fallback is considered.

Startup reconciles valid native and journal snapshots by the Schema-71 durable
order instead of always preferring native. The newer source wins; equal-order
fingerprint disagreement is fatal. A missing native row may recover from a valid
journal. An explicitly unusable native system may enter degraded journal
recovery and then remains profile-only for that session. A valid native row may
continue when journal artifacts are invalid or future. Source selection
preserves all such artifacts; a later verified native checkpoint may repair an
ordinary invalid inactive slot, while unsupported-future or
ambiguous/conflicting history remains write-fenced. Present invalid journal
artifacts without usable native authority are fatal; only the complete absence
of both authorities starts a new campaign.

Ordinary native-active checkpoints mirror the exact staged snapshot into the
rotating journal only after the post-commit save completion callback succeeds.
Every accepted ordinary checkpoint that completes natively therefore converges
that native success into a readback-verified JSON generation. A failed native
callback advances neither slot and re-arms retry; profile-only ordinary writes
complete synchronously. Capture, Campaign Debug isolation, tracked-state
restoration, and administrative reset reject while a native checkpoint is in
flight so the staged DTO cannot mutate under the callback.

Administrative reset is a separate write-ahead transaction. It retains durable
ordering floors, builds all fallible radio/civilian/field-vehicle plans, and
captures one complete prospective campaign into an exact detached DTO. After
reversible radio restoration, the persistence service commits that DTO to the
verified journal before native staging. Failure before this boundary rolls
preparation back and leaves the old campaign live.

The successful write-ahead callback is irreversible. In the same script call it
finalizes old radio projections, consumes no-reject civilian and field-vehicle
cleanup plans, removes old ambient roots, swaps live state, and rebuilds
projections. Native staging then uses the detached DTO. Native absence, staging
failure, callback failure, or timeout after the journal commit is degraded
replica repair; it may not resurrect the old campaign.

The prior journal/shutdown checkpoint passes Foundation at 874 references and stamped Workbench
validation at 5,846 files/11,899 classes, CRC `9a79a33a`, with zero hard errors
and zero owned cleanup residue. The focused journal authority autotest passes
1/1 with an empty failed list, 41/41 exact conditions, and native-v1/native-v2/
invalid-fingerprint/future-envelope classification at 1/1/1/1. Its cases include
generation-zero promotion, damaged-latest recovery, replacement after damage,
split-brain and broken-chain fencing, future-format rejection, and production
source-routing decisions.

The strict five-process campaign proof passes 5/5 across automatic, manual,
controlled shutdown, native restart, and profile-journal restart. The journal
advances generations 1 -> 2 -> 3 and ends at canonical generation 3 with two
valid slots, an exact chain, read-only restore stages, exact controlled-end
bridge and field-vehicle state, and zero cleanup. This is scoped bridge/field
regression evidence, not runtime certification of every native branch. The
guarded native counterattack prepare/recover/replay proof
passes 3/3, selects the newer valid native checkpoint over a deliberately stale
complete journal, preserves both journal files with their exact chain, and also
leaves cleanup at zero.

The same sealed checkpoint adds the dedicated mixed-native graph to
the five-stage chain. Durable carrier binding rather than a fresh process-local
`RplId` owns restart identity; stable seat recovery, player/foreign-occupancy
gates, guard rematerialization, native/profile-fallback readback, and cleanup
all remain exact.

The dedicated three-process administrative-reset proof passes 3/3 across
`prepare_old_checkpoint`, `reset_commit`, and
`stale_native_no_save_verify`. Journal generations advance 1 -> 2 -> 3. The old
checkpoint is `cp2/r0`, the deliberately stale native blocker is `cp4/r1`, the
committed reset is `cp5/r1`, and final recovered live authority is `cp6/r2`.
The last process deliberately loads stale valid native authority, selects the
newer profile fallback, enters degraded newer-journal recovery, performs no
save, and preserves canonical generation 3 plus recovery generation 2 as an
exact chain. Journal and proof-carrier bytes remain unchanged, the old sentinel
does not return, player and commander identity remain exact, an overlapping
reset is rejected without mutation, and cleanup is zero.

## Current Exact Enemy Garrison-Rebuild Fresh-Process Boundary

The guarded exact enemy-garrison-rebuild runner now closes two current-shape
restart cuts through independent dedicated-server processes and the production
two-slot JSON journal.

The `delivery_pending` cut creates one contract-`1` reciprocal order/operation/
manifest/batch/group graph, confirms one durable casualty, and saves it at
225/300 route meters with 9 accepted and 8 living members. Recovery selects
canonical generation 1, restores the same casualty and living-slot
fingerprints, advances the final 75 meters, and delivers exactly once. The
destination links the exact manifest without increasing aggregate infantry,
and the original debit plus one zero-refund delivery receipt remain exact.
Recovery writes generation 2; replay is read-only and a semantic no-op.

The separate `physical_live_fold` cut materializes that same 9-slot authority
as one physical root with 9 adapter handles and 8 living runtime members. The
guarded live sample records 2.759 meters of movement and 0.539 meters of route
closure before the production render-bubble path folds the projection back to
virtual authority. The fold retains exactly 8 living members and the one prior
casualty, removes all runtime members and handles, persists generation 1,
recovers the same virtual graph into generation 2, and leaves replay read-only
and semantically unchanged. Both cuts complete with zero owned engine
processes, disposable-profile artifacts, guards, temporary or spill files,
mutexes, adapter handles, PhysicalWar members, and cleanup failures.

These proofs close the exact guarded virtual-delivery and physical-live-fold
restart cuts for this fixture. They do not yet certify natural route traversal
or contact combat, other force families, arbitrary ownership invalidation,
multiplayer/JIP/reconnect, performance, or soak. The separate dedicated mixed-
native shutdown fixture now passes its scoped five-stage graph but does not
widen this garrison result. The focused garrison-rebuild autotest retains
a stock reload/JUnit harness gap, so this checkpoint does not claim a separate
focused-test PASS.

## Preceding Durable Field-Vehicle Restart Boundary

At this checkpoint, Campaign Schema 70 and runtime-settings Schema 24 remained
unchanged. Durable
`loot_vehicle`, `field_vehicle`, and `garage_redeploy` rows are campaign-ledger
authority. A physical root is only the current process projection of that row;
it is never a second durable owner. Tracking and every capture boundary call
`PersistenceSystem.StopTracking(root, true)` when native entity tracking has
appeared. A root owned by another tracked parent, a failed detach, or any native
tracked durable root after the pass is a fail-closed persistence error.

The ledger graph requires unique stable vehicle IDs, a nonempty normalized full
prefab identity that matches each binding, exact active/inactive status, one
live binding for each active row, no binding for a deleted or detached row, and
unique abstract-cargo keys whose last vehicle position follows the captured
root. Different rows may use the same prefab. Capture refreshes the full 3D
position and normalized upright yaw; pitch and roll intentionally normalize to
zero. A destroyed unoccupied root becomes
an inactive tombstone and its wreck is removed before save; destructive cleanup
is refused while a living player occupies the vehicle.

Startup restores durable field vehicles before normal campaign capture or
gameplay publication. It may adopt one unambiguous exact root or spawn the saved
prefab at the saved transform, but the complete result must satisfy `AllExact`:
eligible rows equal restored and tracked roots, inactive rows stay unbound, the
logical/binding graph is exact, and failed or ambiguous counts are zero. The
coordinator keeps persistence bootstrap closed until that result passes or its
bounded timeout fails the session.

Inactive rows do not generally delete nearby world entities. The only legacy
native-tombstone cleanup is a unique native-tracked, unoccupied root with the
same normalized full prefab within 3 meters and 3 degrees. That candidate is
detached with `StopTracking(true)` and deleted. Any ambiguity, different
tracking owner, occupant, or tolerance miss fails instead of guessing.

Blocking controlled shutdown adds a narrow physical stability fence. Each
active durable root is returned to its captured transform, its controller and
engine are stopped, persistent wheel/hand brakes are applied where supported,
and dynamic physics across the hierarchy is cleared and made inactive. The
bridge maintains and revalidates this quiescence while the native commit is in
flight. The root stays present through commit, but the HST ledger remains its
only persistence authority.

The strict five-process chain starts with two S1203 durable rows carrying
abstract cargo counts 3 and 7. The fresh manual process restores both by spawn,
moves A, destroys B through engine damage, captures B as a tombstone, and
removes its wreck. Shutdown and both no-save verification processes restore
only A while retaining B's tombstone, the exact serialized position/yaw/cargo
graph, and tolerance-verified physical placement.
Every post-prepare restore reports `adopted=0`, `retired-native=0`, exact spawned
counts, and zero native-tracked durable roots. Blocking shutdown proves its one
live root remains controller/physics-quiesced through commit. Native and profile
fallback recover the same result.

All five processes exit `0`. Periodic `AUTO` fires at tick 1,802 and
60.018852233886719 seconds, while the repeat dirty mark remains at
30.016357421875 seconds; `MANUAL`, blocking `SHUTDOWN`, native no-save, and
profile-fallback no-save stages also pass. Foundation passes 839. The latest
stamped Workbench compile passes 5,837 files/11,850 classes at CRC `37604e5a`
with zero script errors and zero residue.

This preceding boundary did not claim full fuel, partial-damage, attachment, or physical-
trunk parity. The proof's duplicate census is limited to expected fixture
positions, and arbitrary vehicle breadth, Workshop/live clients, multiplayer,
and soak remain open. Its directly overwritten JSON mirror limitation is closed
by the current Schema-71 recovery-journal boundary above.

That implementation identity is
`34fcb8e77726beb61dfb10cf650183b5ef99542c`, UTC
`2026-07-17T04:33:16Z`, label
`schema70-settings24-field-vehicle-restart`.

## Preceding Periodic Autosave And Controlled Persistence Boundary

Campaign Schema 70 and runtime-settings Schema 24 remain unchanged. Production
checkpoint intent is explicitly typed as `AUTO`, `MANUAL`, or `SHUTDOWN`.
When native persistence is active, the checkpoint first stages the exact
campaign snapshot, requests the native save point, and waits for the request's
`SaveGameManager` completion callback after commit. Only that successful native
commit may mirror the same snapshot into the profile fallback. This ordering
prevents a newer fallback from outranking the last completed native checkpoint
after a failed native commit. When native persistence is unavailable, the same
typed checkpoint may instead complete synchronously through the profile
fallback.

The production coordinator now advances two independent scheduler lanes through
`HST_PersistenceService.Tick`: periodic `AUTO` elapsed time and first-edge
major-change debounce. The first dirty edge starts the major-change interval;
later marks coalesce without extending it. An accepted full-state checkpoint
acknowledges both lanes. A rejected major-change request does not rewind the
periodic clock, while a rejected periodic `AUTO` request backs off by the
configured debounce interval. While any checkpoint is in flight, both clocks
continue advancing but competing scheduler requests remain suppressed. The
scheduler receipts, attempt sequence, clocks, and threshold evidence are
process-local control state rather than persisted campaign authority.

The production `SCR_BaseGameMode.EndGameMode` bridge owns controlled shutdown.
Its first request disables controls and campaign commands, drains only an
already-pending checkpoint rather than admitting a new automatic or scripted
request, requests a `BLOCKING` `SHUTDOWN` checkpoint, and then quiesces campaign
mutation. The controlled-end retry window is 270 seconds. The bridge waits on
the bounded post-commit `SaveGameManager` completion callback, recomputes the
campaign stability fingerprint, and only then continues the stock game-mode
transition. Timeout, callback failure, or a changed fingerprint fails closed
rather than claiming a durable shutdown.

Native and fallback retention remain mutually explicit. After a verified
native-authority shutdown commit, the game-mode save-data hook skips the stock
native purge even when server persistence retention is disabled and the CLI
retention flag is absent. A fallback-only shutdown instead clears/purges stale
native state so the fallback cannot be hidden by an older native row on the
next start. External process termination cannot be delayed by this bridge; it
can recover only the last checkpoint that completed before the process died.

The final stamped proof passes five fresh processes: production-tick periodic
`AUTO`, typed `MANUAL`, and real bridged `SHUTDOWN` checkpoints followed by
native and fallback restart verification. The periodic request reports origin
`periodic_autosave` at tick 1800 and 60.020751953125 seconds. A repeat dirty
mark at 30.020465850830082 seconds does not extend the configured 120-second
first-edge major-change debounce. Server configuration disabled session
retention and the CLI retention flag was absent. All five processes exit `0`;
`AUTO` and `MANUAL` use flags `0`, `SHUTDOWN` uses exact `BLOCKING` flag `1`,
both verification stages perform no save, and all cleanup counters return to
zero. The deterministic source harness covers rejection retry/fairness and
in-flight suppression, but this packaged run does not add a separate live
`SCRIPTED`-at-debounce or rejection stage. Guarded `OnAfterSave`/
`OnSaveCreated` correlation and transition polling remain proof evidence rather
than production dependencies.

Preceding implementation identity is
`952a2d33245074867df6afad1ffe25ce49fc9a11`, UTC
`2026-07-17T01:12:37Z`, label
`schema70-settings24-periodic-autosave-scheduler`.

## Preceding Native Persistence Source-Selection Boundary

Campaign Schema 70 and runtime-settings Schema 24 remain unchanged. The
serializable `HST_CampaignSaveData` remains a manually constructed campaign DTO;
it does not inherit engine persistence state. The persistence system instead
owns one `HST_CampaignPersistentState : PersistentState` proxy. Its scripted
serializer carries a versioned envelope, full DTO snapshot, and deterministic
fingerprint, while tracking and save calls always target the engine-created
proxy.

Startup source resolution is native-first and fail-closed. A valid native row
wins before the JSON profile fallback. An invalid native row, unreadable present
fallback, terminal persistence-system state, or untrackable proxy is fatal. A
loaded engine session missing its HST native row is fatal unless a valid profile
fallback is explicitly selected for migration; it is never mistaken for a new
campaign. Legacy fallback files still migrate into the canonical profile file.
At this preceding checkpoint, successful native saves and the profile recovery
copy had not yet gained the post-commit mirroring order defined by the current
boundary above.

The coordinator no longer assumes persistence is active during post-init. It
defers campaign adoption and every restore reconciler until the system reaches
`ACTIVE`, retries from the frame loop for at most 120 seconds, and closes the
session on timeout or fatal resolution. Only a fully validated source is
captured, attached to the native proxy, and admitted to gameplay callbacks.

Both campaign mission headers select the HST persistence `SystemsConfig` and
declare `m_eSaveTypes 15`. The guarded native proof requests blocking manual
savepoints, records the exact `OnSaveCreated` UUID and completion callback, and
loads that UUID in each fresh recovery process. Recover and replay must select
the native fingerprint ahead of a deliberately conflicting profile fallback;
the fallback remains byte-for-byte unchanged.

The proof launcher packs the current project through a guarded Workbench
ResourceManager process into a nonce-owned disposable add-on root, then starts a
dedicated server from the packed base project and an exact config mod entry.
Profile, temporary, pack, server, and process ownership are contained by the
guard. The one required workspace scratch directory has its own owner sentinel,
must remain unchanged while packing, and is removed only after exact ownership
is revalidated. Final cleanup requires zero scratch, guard, engine, and spill
residue.

This boundary is stamped by implementation
`a6e9069f29f8b844f8545b77b8894170ecd6d3b8`, stamp
`35fc01a399f4f688f28f4ef7afee6351fb6289b7`, UTC
`2026-07-16T20:53:27Z`, and label
`schema70-settings24-native-persistence-source-selection`. Final Foundation
passes 828. Workbench validation passes 5,834 files and 11,839 classes at CRC
`5fdd016f` with zero hard/script errors and zero cleanup residue. The final
packed prepare/recover/replay chain runs build `a6e9069f29f8`, proves
`new_campaign -> native -> native`, exits `0` at every stage, and returns every
guard, process, pack-scratch, watched-root, and spill-root counter to zero.

## Preceding Counterattack Owner-Applied Restart Boundary

Campaign Schema 70 and runtime-settings Schema 24 remain unchanged. Restore now
runs generic ownership normalization, then counterattack-specific correlation,
before runtime ownership reconciliation can publish a pending transition.
Counterattack ownership authority is correlated by canonical request ID or exact
operation source ID, counted once per row, and checked against the complete
canonical receipt fingerprint.

The legal lifecycle is explicit. Uncommitted and pre-recapture counterattacks
allow no claimant. A stable, open, clear-engagement, on-station operation with a
legal strategic/virtual or live/physical handoff pair may retain at most one
exact pending or completed receipt. Returning, durable-outcome, or recapture-
requiring state requires exactly one completed receipt. A completed historical
receipt remains evidence after a later ordinary zone recapture; incomplete
authority never does.

Foreign, duplicate, orphaned, engaged, premature, or fingerprint-invalid rows
are fenced before runtime reconciliation. The ownership row, reciprocal zone,
and campaign marker are quarantined through the canonical ownership validator;
the exact counterattack order is quarantined at `-69`; and its operation,
manifest, batch, group, and retained evidence are held without inventing a
capture, refund, settlement, cancellation, or outcome. Pre-69 restoration
applies the same fence to incomplete declared counterattack ownership before
historical preservation.

The focused proof covers legal pending authority in stable, materializing, and
dematerializing handoffs; legal completed returning authority; premature,
engaged, foreign, distinct-duplicate, and source-only orphan claimants; and the
actual generic-then-counterattack normalization order. Campaign Debug exposes
this independently as `enemy_counterattack.ownership_correlation`. The command-
line engine runner requires the exact non-skipped JUnit case, its empty failed-
list artifact, correlation evidence, contained process tree, recursive external
census, and zero cleanup residue.

The eighth guarded `owner_applied_pending` cut persists a raw legal owner-applied
incomplete receipt. Prepare derives and reads back its normalized pending form
without applying ownership. Recover validates that normalized source before
startup ownership reconciliation, requires exactly one ownership completion,
and then permits exactly one production counterattack tick to create raw
`RETURNING`; persistence derives normalized `RETURNING`. The raw and restored
returning structures differ while sharing the same canonical semantic
fingerprint.

Replay starts from normalized `RETURNING`, reports no ownership startup mutation,
executes no continuation tick, denies canonical fallback overwrite, and
preserves the save's SHA-256, byte length, and last-write UTC identity. The
ownership-startup flag is therefore `false -> true -> false` across prepare,
recover, and replay. Final stamped Foundation passes 819; Workbench passes
5,832/11,835 at CRC `417e9910` with zero hard errors; and all eight guarded
chains pass 24/24 stages with exact fingerprints, zero exits, and zero cleanup
residue. Durable endpoint ABA snapshots remain a future contract-2 schema
decision; their schema number is not yet assigned.

## Preceding Counterattack Endpoint Owner/Claimant Restart Boundary

Campaign Schema 70 and runtime-settings Schema 24 remain the persisted
contracts. This checkpoint is schema-neutral and settings-neutral: it adds no
persisted field, enum ordinal, operation contract, or migration rule.

Both external carrier families freeze the expected owner faction and ownership
revision for the source and target zones. Every one of the seven cuts requires
exactly one row for each endpoint, exact owner/revision equality, and zero
ownership-transition claimants correlated either by canonical request ID
`ownership_counterattack_<operationId>` or by operation source ID. The endpoint
rows and claimant count participate in fingerprints and no-op comparisons.
Tamper proof increments the source revision and injects each claimant identity
in turn; all three corruptions must be rejected through the complete movement
or settlement validator. The physical cut inherits the movement-family tamper
test and also exercises the positive endpoint check on its genuine native graph.

The guarded exact-counterattack restart harness now has seven scoped cuts. The
outbound-`VIRTUAL` cut persists canonical strategic authority and requires one
75 m recovery continuation followed by semantic-no-op replay. The
`dematerializing_before_hold` and `materializing_checkpoint_deferred` cuts each
validate their distinct raw transition graph, require production capture to
return no save, and prove that the retained canonical `VIRTUAL` fallback is
unchanged. The dematerializing fallback preserves the exact N-1 living roster
and confirmed casualty tombstone. Neither transitional raw graph is represented
as a successful durable checkpoint.

The `physical_live_position` cut drives the production counterattack owner and
scoped projection worker across real frames until one successful non-held batch
owns genuine `PHYSICAL`/`LIVE` authority. Its native topology must contain one
registered root, one member per durable living slot, root-plus-member adapter
handles, and exactly the living-member count in PhysicalWar. The proof samples
the native group centroid, deliberately makes both durable group and operation
positions stale, and then enters normal production persistence.

Physical exact-enemy-response persistence is an all-graphs transaction. It
first resolves family-owned reciprocal authority without mutation, validates
the successful batch, living roster, adapter/PhysicalWar bindings, and native
live position, and builds position plans for every physical exact defensive
QRF, counterattack, and garrison rebuild. Only after all other persistence
preflights pass does it apply the sampled group position and invoke the owning
family's operation refresh. Any application failure restores every already-
applied group position, operation position, revision, and progress timestamp.
Open exact-enemy-response `DEMATERIALIZING` authority defers capture before this
mutation boundary.

For the physical counterattack cut, capture refreshes both deliberately stale
durable positions from the live centroid. Persisted readback is normalized to
held `VIRTUAL`/`STRATEGIC` authority with a pending batch, cleared process-local
bindings, and exactly one reprojection. A fresh recovery advances exactly 75 m;
replay is a semantic no-op. Cleanup is focal and requires zero projection and
result handles, PhysicalWar members, or registered root before durable proof
rows are released. At that preceding checkpoint, defensive QRF and garrison
rebuild shared only the static persistence preflight and normalization contract,
while the counterattack held the genuine scoped native runtime proof. The
current `physical_live_fold` boundary above now closes that same scoped native
handoff/fold bar for the exact garrison fixture, not for defensive QRF.

The three settlement cuts persist the same nondegenerate, one-pool,
survivor-proportional `route_failed_survivors` intent at distinct crash windows:
`prepared_before_refund` has no refund mutation or receipt;
`prepared_after_refund` has exactly one refund mutation but no durable receipt;
and `prepared_after_receipt` has both while its operation remains `PREPARED`.
On the first restored start, production reconciliation consumes each prefix
exactly once, preserves exactly one original debit and one refund, publishes the
terminal `SETTLED` operation and aborted order, and removes the reciprocal batch
and group. An explicit second reconciliation and a second fresh start are both
semantic no-ops. The prefix-to-terminal operation revision is `+2`, `+2`, and
`+1`, respectively.

The final stamped source passes Foundation at 819 symbols. Workbench loads 5,832
Game files and 11,835 classes at CRC `3131538f`, exits `0`, reports
`ScriptValidation true` with zero errors, and leaves every cleanup counter at
zero. All seven guarded prepare/recover/replay chains, 21 stages total, run build
`008cd481d5e5`, exit `0`, preserve exact fingerprint continuity, and clean
exactly. Their digest chains are:

- `outbound_virtual`: `d9b7bed7d685d793 -> 05745f6799ed4196 -> same`.
- `dematerializing_before_hold`: `34fd3ec48963459e -> 3d105daaf7159131 -> same`.
- `materializing_checkpoint_deferred`: `0259acbbd2ea35c9 -> 809d6ccf51a7e393 -> same`.
- `physical_live_position`: `711a8081db90d496 -> 9012a0975998263f -> same`.
- `prepared_before_refund`: `948803021dbbf846 -> 326a49119bc4b0a7 -> same`.
- `prepared_after_refund`: `e67861d2f8bdf456 -> 126ef467ff7c1abe -> same`.
- `prepared_after_receipt`: `c461df2b3cdfe0a2 -> 940b360563a6ead1 -> same`.

Durable carrier validation is an exact exclusive choice: movement carriers own
only a movement expectation, while settlement carriers own only a settlement
expectation. The launcher negative self-test forges both families on one
settlement carrier and requires rejection. The independent post-run census finds
zero engine processes, zero Workbench guard roots, zero restart guard roots, and
both proof mutexes free.

At that checkpoint, the next counterattack slice was the lifecycle-aware pre-
reconcile decision for an orphan or pending counterattack-owned ownership
transition. The current boundary above now implements that correlation and
quarantine policy; the preceding checkpoint proved only zero-claimant authority
and tamper rejection. Durable endpoint ABA snapshots remain a future contract-2
schema decision whose schema number is not yet assigned. Native persistence-source
selection, world scope, package/live server-client, network proof, migration,
marker/runtime UI, multiplayer/JIP/reconnect, performance, soak, and wider
Campaign Debug failures remain open. This remains proof depth inside Blueprint
Phase 9 of 13 rather than phase completion.

## Preceding Phase 24 Runtime-Owner Snapshot Boundary

Campaign Schema 70 and runtime-settings Schema 24 remained the persisted
contracts. That checkpoint was schema-neutral and settings-neutral and added no
persisted field, enum ordinal, operation contract, or migration rule.

Phase 24 now preserves the pre-runtime production-owner snapshot instead of
discarding it after physicalization. A bounded follow-up pass admits legitimate
orders created after that snapshot, while missing rows, duplicate identities,
and owner mutations fail closed. Assertions use the sampled owner count rather
than the final created-order count, so every classified owner has one stable
sampling record. The exact-counterattack authority assertion may validly
`SKIP` only when every exact-order, open, terminal, invalid, projection,
support-leak, and projection-state metric is zero. Phase 17 remains the
deterministic proof owner for the complete exact-counterattack virtual/native
lifecycle.

Full guarded run `seed1985_t0_p1_u1784134163` on that stamped source
classified all 14 sampled Phase-24 runtime owners with zero snapshot invariant
failures. Three exact counterattacks remained open under three `VIRTUAL`
projections, with zero invalid-authority rows and zero support leaks; both
Phase-24 assertions passed. All 11 Phase-17 assertions and all six staged
spawn-adapter cleanup cases passed, all 18 state-diff rows remained zero, and
the script/Partisan/crash census plus every process/profile/external cleanup
counter remained zero. Workbench validation passed 5,830 files and 11,822
classes at CRC `e836e3b4`, with script validation successful and zero hard
errors. The wider 687-case suite remains uncertified at 583 PASS, 50 WARN, 46
FAIL, 7 BLOCKED, and 1 SKIPPED; 5,537 of 5,685 certification requirements were
proven, with 134 failed and 14 blocked.

### Preceding Staged Exact Spawn-Adapter Proof Ownership

The staged exact spawn-adapter proof is a runtime owner only while its proof is
active and all prerequisite authority is ready. Its backing predicate accepts
only the exact cancel, success, and failure fixture identities. It requires one
unique reciprocal group, manifest, and batch; recomputes and matches the frozen
manifest hash; and verifies the exact request, operation, manifest, result,
force, projection, faction, prefab, and policy fields. This narrow owner cannot
legitimize unrelated or ambiguous staged rows.

All generic post-case probes still run after each staged action. Exact live
fixture roots may suppress only the synthetic zero-controlled-member increment
while they remain in the fail-closed pre-handoff state. The audit still checks
their real root/member faction and vehicle ownership. `Finish()` disables that
proof owner after cleanup, and final cleanup receives no grace, so residue is
still classified as a failure rather than being hidden by proof state.

That preceding staged-faction source passed Foundation at 808 script-symbol
references. Its reciprocal-backing predecessor passed PC Game validation and a
guarded full run; that run proved zero orphan groups and exact final cleanup but
exposed the synthetic faction-audit increment in four root-only intermediate
frames.

### Preceding Exact Counterattack Native-Projection and Casualty-Continuity Boundary

Phase 17 now drives one already-admitted exact counterattack through the same
runtime ownership path used by production. The coordinator resolves the exact
counterattack owner, asks the commander to execute only that focal order, and
lets the operation owner move a held `VIRTUAL` roster into `MATERIALIZING` when
the controlled player enters the render boundary. A scoped adapter tick then
runs the production queue selector, spawn executor, and deleted-staged-handle
reconciliation only for the focal projection. A successful handoff publishes
`PHYSICAL` synchronously in production, so the proof now confirms that state and
performs the immediately controlled fold or kill in the same coordinator
invocation. It still yields across asynchronous death observation, corpse
cleanup, and each later re-entry. The probe does not mutate the shared campaign
clock to force retries due.

The preceding R31 source extends that production-owned cycle with one real engine
death. The projection-scoped adapter observes the dead native member, retires
exactly its durable slot, detaches and removes the proof corpse, and proves a
second reconciliation is a no-op. Production dematerialization then folds the
N-1 living-slot roster to held virtual authority, production reprojection
materializes only those N-1 survivors, and a final production fold returns the
same N-1 fingerprint to virtual authority without resurrecting the retired
slot. Generic PhysicalWar survivor sampling excludes active exact-counterattack
groups, leaving the projection adapter as the sole casualty-retirement owner for
that aggregate.

Exact runtime-member liveness now requires both the controller-aware living
predicate and the stock damage-state predicate. The proof refuses to issue its
single `Kill()` unless damage handling is enabled, then samples the resulting
death for at most four observations. It does not issue another kill and does not
reconcile the member while either liveness predicate still reports alive.

Physical confirmation is state-first and never reissues completed production
work. Initial, casualty, and survivor handoffs require the exact
`PHYSICAL`/live operation state, native root, durable roster, adapter handles
and bindings, PhysicalWar registrations, and zero legacy-support ownership.
Their handoff-to-confirmation transition stays in the same invocation; the
bounded sampler remains available only for genuinely asynchronous settlement.
A changed production tick is diagnostic evidence, not a success condition. The
binding validator now distinguishes an incomplete handed-off adapter binding
from a complete binding whose entity is nonliving.

Every state preserves the order/operation/manifest/batch/group identities, the
frozen manifest, living-slot fingerprint, original one-pool debit, enemy
resource totals, and absence of legacy support requests. `VIRTUAL` and idle
batch authority require zero native topology. `MATERIALIZING`, `PHYSICAL`, and
`DEMATERIALIZING` validate the exact living roster plus root/member/handle
cardinality. Transitional validation also checks projection and result keys,
unique entity IDs, slot kind, native group identity, and each adapter entity's
matching PhysicalWar registration; a runtime member cannot survive without its
native root.

Production typed settlement remains the first cleanup path. If a failed probe
still owns focal runtime, emergency cleanup requires both projection and result
keys, retires both bindings, removes their adapter handles, releases PhysicalWar
ownership, and succeeds only when both registries report no residue. If the
durable group row has already vanished, the group-keyed fallback first retires
its process-local PhysicalWar authority and then explicitly deletes each
unregistered orphan handle entity; any foreign registration or surviving entity
fails cleanup closed. If the tracked exact order disappears, the runner treats
that as fatal owner loss: it records the failed invariant, stops the run,
restores the isolated state snapshot, saves the aborted artifacts, and does not
execute later Phase-24 telemetry.

Phase 24 resolves one production runtime owner for every sampled escalation
order: legacy, exact QRF, exact counterattack, exact patrol, exact garrison
rebuild, quarantined, or unsupported. Legacy support physicalization remains a
separate warning path. Exact counterattacks must instead prove either one open
projection with exact `VIRTUAL`/`MATERIALIZING`/`PHYSICAL`/`DEMATERIALIZING`
authority or one terminal ledger with no runtime claimant, and must never own a
legacy support row.

That preceding stamped tree passes Foundation at 808 script-symbol references. Its
Workbench Game Module run loads 5,830 Game files and 11,822 classes with 47,077K
static storage at CRC `b789ee05`, reports `Script validation successful`, exits
`0`, records zero script, HST, or hard-failure signals, and leaves exact-zero
session, owned-process, default-log, and spill cleanup.

R28 `seed1985_t0_p1_u1784098447` first exposed an intermittent observation race:
687 cases finished at 576 PASS/50 WARN/54 FAIL/6 BLOCKED/1 SKIPPED, with
5,517/5,684 certification assertions proven, 149 failed, 18 blocked, and zero
warning assertions. The five casualty assertions failed, while all 29 state-
diff lines, including 18 delta-bearing rows, remained zero and no mission state
leaked. An unchanged rerun, R28b `seed1985_t0_p1_u1784100187`, finished at 577
PASS/50 WARN/53 FAIL/6 BLOCKED/1 SKIPPED and 5,523/5,684 proven, with 143 failed
and 18 blocked. All 11 Phase-17 target assertions passed exactly once, the
Phase-24 runtime-owner assertion passed, exact authority remained skipped, and
the real casualty stayed N=9 -> N-1=8 through fold and re-entry with zero state
diff.

R29 `seed1985_t0_p1_u1784101849` ran the intermediate death-settle checkpoint.
It finished 687 cases at 576 PASS/50 WARN/54 FAIL/6 BLOCKED/1 SKIPPED and
5,521/5,687 certification assertions proven, with 148 failed, 18 blocked, and
zero warning assertions. Its first six Phase-17 assertions passed, but the five
casualty assertions failed before the kill because survivor re-entry was still
`MATERIALIZING`/spawned/not-yet-`PHYSICAL` (`1/1/0`); casualty death settling
therefore remained `0/4`. Both Phase-24 assertions passed with 16/16 runtime
owners classified, orders/open/terminal/invalid `1/1/0/0`, one projection with
V/M/P/D `1/0/0/0`, and zero support leaks. All 29 state-diff lines, including 18
delta-bearing rows, were zero and no mission state leaked. This isolated the
physical-observation seam addressed by the bounded sampler.

R30 `seed1985_t0_p1_u1784110353` completed 687 cases at 576 PASS/50 WARN/53
FAIL/7 BLOCKED/1 SKIPPED and proved 5,510/5,672 required assertions, with 144
failed and 18 blocked. The first six Phase-17 assertions passed, while the five
casualty assertions failed before the kill: an artificial one-second yield after
handoff let one still-bound and registered member become nonliving before the
proof's next observation. Phase-24 runtime-owner classification passed; exact
authority was skipped after focal cleanup removed its claimant. All 18 tracked
state deltas were zero, the run recorded no HST script error or crash marker,
and guarded external cleanup was exact. The preceding proof-ordering checkpoint
removes that artificial post-handoff yield while preserving the asynchronous
death, cleanup, and re-entry yields.

R31 `seed1985_t0_p1_u1784117364` completes that preceding guarded runtime proof on build
`393733cc165b96ec494c72f96741cf993d400ebd`, label
`schema70-settings24-native-counterattack-proof-ordering`. It finished 687
cases at 577 PASS/50 WARN/52 FAIL/7 BLOCKED/1 SKIPPED and proved 5,528/5,685
required certification assertions, with 139 failed, 18 blocked, and zero
certification warnings; the overall result remains false. All 11 Phase-17
projection/casualty assertions and both Phase-24 owner assertions passed. The
native casualty stayed N=9 -> N-1=8, with initial/casualty/survivor physical
settle observations of `1/1/1` within the four-observation limit and casualty
death observed at `1/4`. Phase 24 classified 16/16 sampled owners and retained
two open exact projections. The 29-line state diff contained 18 delta-bearing
rows, all zero; the run recorded zero script, HST, or crash errors and exact
external cleanup. This runtime-proves the same-invocation handoff -> `PHYSICAL`
-> controlled-action ordering without certifying the whole suite.

### Historical Exact Defensive-QRF Restart Boundary

Exact defensive-QRF settlement is now a durable replayable state machine. The
operation first persists `PREPARED` terminal intent. The owner then stages the
complete deterministic order/refund tuple with
`m_bResourceSettlementApplied = false`, validates the original debit, every
claimant backlink, and the durable survivor authority, applies or idempotently
replays the canonical refund, publishes the applied receipt last, and only then
finalizes the operation and order tail. The family-specific resource validator
continues to support support-only and dual-pool defensive funding without
weakening exact counterattacks' one-pool contract. It includes the preceding
prepared-recovery checkpoint at
`78db295a02936aa66899203cb33e50462b5fd557`, the refund-authority checkpoint at
`434b73a16ae92911896fdec095af6bce88168916`, the active-demolition-witness
checkpoint at
`0e54f6cbc7f7084e5534fc603b491cba0d91b653`, the SpawnQueue resume checkpoint at
`0b380f00fde65c4f2e22858faf8ddc6eab794131`,
the debug cleanup-ownership checkpoint at
`3ded248a4ded084dfb0e3aa8e54ae0a47d36cd5f`, the checkpoint-evidence correction
at `2508a735863c153f95bae94adb13f3037b4cdeef`, and the current-support restore
correction at `89b7754bcd9ac7e8c41f2a8d7604784b5c1c1c83`.

The sealed exact-QRF tree passes Foundation at 806 script-symbol references.
Stamped PC Workbench validation compiles 5,830 Game files/11,820 classes with 46,915K
static storage and CRC `ff59593b`, reports `Script validation successful`, and
records zero script errors. Focused `HST_TEST_EnemyQRFAuthority` execution
records one testcase, zero failures, an empty failed list, and `AllExact 1`.
The focused environment still emits the known recoverable stock player-audit VM
diagnostic, so it is successful but not exception-free.

The external restart matrix runs `prepare`, `recover`, and `replay` in separate
fresh engine processes for each of the before-refund, after-refund, and after-
receipt cuts. All nine stages report success and exit `0`. Recovery observes the
canonical prepared source and one startup settlement change, then proves an
explicit same-state reconciliation no-op and exact terminal fallback readback.
Replay observes that terminal source, changes nothing during startup, proves a
second explicit no-op, and preserves the same fingerprint. This evidence is
strictly for the canonical JSON fallback and this defensive-QRF settlement
aggregate; it does not certify native persistence-source selection, physical or
world state, migration, package, dedicated multiplayer, reconnect/JIP, or soak.

R23 `seed1985_t0_p1_u1784054690` remains dated active-demolition-witness proof
and the historical exact-QRF cleanup-contamination diagnosis. R24
`seed1985_t0_p1_u1784059798`, build `6303e58`, then completed 688 cases at 578
PASS/50 WARN/53 FAIL/7 BLOCKED and proved 5,522/5,682 required assertions, with
142 failed and 18 blocked. It restored typed cleanup to zero failures/open
orders/runtime claimants, kept the open-order leak at 0 -> 0, passed the core
Phase-24 seed/report/outcome chain, and restored an exact-zero tracked state,
while `enemy_qrf.settlement` and `enemy_qrf.persistence` still failed.

R25b `seed1985_t0_p1_u1784063032` remains the dated proof of the preceding
refund-authority checkpoint. Historical run R26
`seed1985_t0_p1_u1784074264`, executes
688 cases at 577 PASS/51 WARN/54 FAIL/6 BLOCKED and proves 5,504/5,667 required
assertions, with 145 failed and 18 blocked. Both `enemy_qrf.settlement` and
`enemy_qrf.persistence` pass, including all six committed dual-pool/support-only
crash cuts, all three uncommitted full-refund cuts, replay and second-restore
no-ops, fail-closed prepared corruption/tamper checks, stable current-provenance
`SETTLED` pool-tail quarantine, and mutationless historical compatibility. Typed cleanup remains
settled 0, failures 0, open 0, runtime 0; the open-order leak remains 0 -> 0.
Seeded in-memory restore matches 11/11 missions, 22/22 assets, 21/21 runtime
entities, 9/9 groups, 10/10 runtime vehicles, and 1/1 field vehicle, and the
final tracked-state diff is zero. Escalation physicalization remains WARN.
R26 remains historical in-process crash-cut evidence rather than external-
restart evidence. The separate guarded matrix now closes only the canonical-
fallback defensive-QRF settlement cut; world-scope, package, network, and soak
gates remain open.

### Synthetic-Time Isolation

Debug probes that need bounded future ticks use
`HST_CampaignDebugClockIsolationContext`. The context captures the shared
campaign second and a fingerprint of enemy strategic pools, receipts,
revisions, and cadence authority. The probe may advance synthetic time only
inside its bounded operation, then normalizes debug-owned future timestamps,
restores the original campaign second, and emits assertions for exact clock and
enemy-authority equality. A case must not advance the shared clock while ticking
only one dependent service.

### SpawnQueue Resume Selection

Bounded queue selection happens before `StartAttemptIfReady` normalizes durable
slot state for a new attempt. The selector therefore recognizes an ordinary
`QUEUED` slot, and also recognizes `DEFERRED` or `FAILED_RETRYABLE` slots only
when their owning batch is itself eligible to start a pending, deferred, or
retryable attempt. Frozen-manifest validation, exact slot identity, dependency
registration, strategic holds, deadlines, and bounded priority selection remain
unchanged. This makes persisted and same-session retry work selectable without
moving attempt mutation ahead of bounded acquisition or widening supported
manifest shapes.

R22 proves the production queue gate across retry backoff, failed-slot-only
selection, stale callback rejection, duplicate-safe completion/replay, same-wave
sibling progression, and interrupted-restore reconciliation. No campaign or
settings migration is required; previously stuck nonterminal rows become
resumable under their existing durable state.

### Active Demolition Witness Admission

Destroy-target nearby-witness scoring now admits only detached physical
projectiles or triggered blast entities. Candidate collection rejects any
entity with a parent, rejects inventory items attached to a parent slot, and
requires `BaseProjectileComponent` plus either a triggered
`BaseTriggerComponent` or a moving `ProjectileMoveComponent`. Projectile or
ammo metadata embedded in carried equipment can therefore no longer become
demolition evidence merely because it appears inside the scan radius.

Entity-backed damage callbacks and nearby-witness scans build the same
canonical source key from physical entity identity. Each target component keeps
at most 64 accepted source keys for its full lifetime; duplicate sources and
capacity overflow fail closed, and old receipts are neither timed out nor
evicted. The component requests the authoritative asset mutation first, then
records the lifetime receipt, duplicate window, and local diagnostic score only
after authoritative damage, hit count, or destroyed state actually changes.

Campaign Debug now asserts
`primitive.destroy.no_ambient_witness_score` immediately before its explicit
destroy-target damage action. The assertion requires zero demolition damage,
zero hits, no last source, no evidence keys, and an undestroyed authoritative
asset. R23 proves that assertion in all six generic destroy-target variants and
passes the mission-start, mission-runtime, and primitive-runtime cases for all
seven destroy-family definitions. This is targeted regression evidence; entity-
callback overlap, restore, packaged multiplayer, and soak gates remain open.

### Exact Defensive-QRF Prepared Settlement Recovery

Defensive-QRF settlement persists `PREPARED` terminal intent before resource
mutation. It stages settlement identity, kind, refund mutation identity,
accepted/survivor counts, and refund amounts as one complete deterministic tuple
with `m_bResourceSettlementApplied = false`. Before changing a pool, the owner
validates the original debit, reciprocal order/operation/manifest claimants, and
the durable survivor tuple from the manifest plus any retained batch/group
authority. The enemy resource authority then applies or idempotently replays
`enemy_resource_refund_<settlement-id>`. Only after that succeeds does the owner
publish the applied receipt, then finalize the operation and order tail. Applied
replay validates the canonical mutation backlink instead of trusting stored
amounts alone.

The QRF-specific settled-refund validator requires one canonical debit and one
canonical refund claimant with exact mutation identity, faction, order,
operation, manifest, zone, deltas, contribution shape, and chronology. It
accepts support-only or dual-pool defense-resource funding; exact counterattacks
continue to require exactly one charged pool. Partial tuples and conflicting
claimants remain fail-closed.

The focused proof captures and restores each accepted persisted prefix: before
refund, after refund but before receipt publication, and after receipt
publication but before the terminal tail. It covers dual-pool, support-only, and
uncommitted full-refund shapes that retain the operation while removing runtime-
commitment backlinks; a first recovery converges exactly, same-state replay is a
no-op, and a second capture/restore is also a no-op. Truly operationless
admission rollback retains its refund-first contract and is outside this durable
`PREPARED` recovery slice.
Corrupted `PREPARED` debit/pool tails, tampered survivor tuples, and malformed
current-provenance `SETTLED` receipt chains fail closed without another resource
mutation. Historical mutationless settlements remain compatible. Focused
`AllExact 1` and both R26 integrated QRF assertions pass. R26 remains the
historical deterministic in-memory capture/restore proof; arbitrary old partial
rows are not auto-healed.

A separate guarded development-world protocol now exercises this settlement
boundary through the canonical JSON fallback in fresh engine processes. The
`prepare` process writes and reads back one exact `PREPARED` cut. The `recover`
process verifies that source before startup reconciliation, requires exactly one
startup transition to the terminal graph, proves a second reconciliation is a
no-op, and persists and reads back the exact terminal fingerprint. The `replay`
process verifies that terminal source and the preceding recovery fingerprint,
then requires both startup and explicit reconciliation to remain no-ops. Each
stage writes its result last and closes the process. An explicit guard, run ID,
stage, cut, development-world gate, and Campaign Debug exclusion keep this
destructive proof outside ordinary campaign startup.

All three stages pass with exit `0` for the before-refund, after-refund, and
after-receipt cuts. This closes only the exact defensive-QRF canonical-fallback
settlement restart gate. The proof-specific source adoption does not establish
which source general production startup selects, and it says nothing about
native persistence, other operation families, physical/world state, migration,
package, dedicated multiplayer, reconnect/JIP, or soak.

Restore snapshots row-level strategic-receipt provenance before Schema-67
normalization can remove a malformed mutation. It revalidates current-provenance
`SETTLED` exact-QRF authority after that normalization. A bad chain disarms the
operation settlement to unknown authority, aborts the order, cancels and
strategically holds the reciprocal batch, retains the group without reviving
it, and preserves the stable
`exact_restore_resource_authority_quarantined` status across repeated restore.
Mutationless historical settlements do not acquire current receipt requirements.

### Enemy-Commander Isolation And Typed Cleanup

The ordinary coordinator enemy-commander cadence is held only while Campaign
Debug is running on its isolated state clone. Explicit Phase 18, Phase 22, and
Phase 24 calls still execute the production commander, preserving the intended
proof path while preventing incidental orders and Defend Petros missions from
appearing between case steps.

Every debug-admitted enemy order is tracked by its stable ID. Administrative
cleanup routes exact contracts to their operation-specific settlement owner and
contract-zero fixtures to the legacy commander/PhysicalWar settlement owner.
Those owners retire runtime claimants and apply the matching terminal resource
policy before the order becomes terminal. Cancellation and completion assert
typed settlement, zero tracked open orders, and zero exact runtime claimants
before narrow debug-prefix record cleanup begins. Prefix deletion is never a
substitute for resource or operation settlement.

The Phase-17 native counterattack probe follows the same ownership rule. Its
commander hook accepts only one already-resolved exact-counterattack order and
delegates to that operation's production runtime tick. Its adapter hook accepts
only one canonical projection and reuses the production queue, spawn, handoff,
and reconciliation passes. Bounded retries wait for real campaign frames; no
debug-only support request or synthetic time path may become the runtime owner.

The probe captures adapter counters/cursors and restores them after focal
runtime is retired. Normal typed cleanup is attempted first. A failed projection
can then invoke fail-closed dual-key projection/result retirement. When the
durable group row is missing, that path retires exact process-local PhysicalWar
ownership by group ID before explicitly deleting every now-unregistered orphan
entity; success still requires zero matching handles, runtime rows, and live
orphan entities. Losing the exact order itself is not recoverable probe
telemetry: it aborts and restores the isolated run before Phase 24.

### Marker Backing Authority

Campaign marker audit delegates exact operation backing to
`HST_MapMarkerService`. A valid exact QRF, counterattack, garrison-rebuild, or
patrol marker must have its canonical marker ID, reciprocal linked operation,
and pass the same operation-specific visibility predicate used by the publisher.
This prevents valid operation markers from being reported as orphans while also
preventing a shared category or arbitrary linked ID from concealing a real
orphan.

### Radio Physical Damage Authority

`HST_RadioSiteLifecycleService` resolves physical health/state through one
adapter that queries generic scripted `SCR_DamageManagerComponent`, the exact
stock `SCR_DestructionMultiPhaseComponent`, and the stock
`SCR_DestructionDamageManagerComponent` base before returning their shared
authority. The same adapter owns candidate admission, active damage polling,
direct fixture damage, mission configuration, new-campaign restoration, and
destroyed-state suppression. This prevents one radio lifecycle branch from
accepting a target that another branch cannot damage or restore.

Runtime discovery also requires the inherited concrete component resource to be
enabled. Generated demolition prefabs therefore override the existing inherited
damage component rather than adding a duplicate. When the enabled multiphase
component requires replication, its existing inherited `RplComponent` is
enabled as the paired dependency. Zero-health destruction follows the engine's
`Kill()` path and commits only after observing `DESTROYED`; nonzero restoration
writes the default hit zone and verifies the recovered health/state.

R12 through R15 isolated this boundary in four steps: base-class lookup is not
polymorphic discovery; an exact inherited component must also be enabled; zero
health must use the engine destruction action rather than rely on a scalar
setter; and enabling multiphase destruction on generated rebuild equipment
requires its paired inherited replication component. R15 proved the normal
destroy callback, receipt, mission objective/reward, and next-mission admission
before the missing rebuild replication dependency failed closed. Checkpoint
`a81d494` supplies that paired dependency without changing durable authority or
persisted schema. Exact runtime outcomes remain in the Campaign Debug audit.

### Workbench Compiler-Shape Boundary

The render-bubble diagnostic had accumulated enough local state to reproduce a
native `0xc0000374` Workbench heap failure before `Module: Game` and before any
script diagnostic. Moving its clock/fingerprint state into
`HST_CampaignDebugClockIsolationContext` and focused helpers reduced the main
method's local-variable pressure and restored Game compilation and bounded cold
open on the corrected source shape. Large Campaign Debug methods must continue
to use compact context/result objects and focused helpers; a clean text/static
gate alone does not prove that the native compiler can load the method.

The crash-fix tree passes Foundation with 793 script-symbol references,
compiles and completes Workbench create/destroy at 5,826 Game files/11,807
classes with CRC `287d01ec`, and remained alive at the 8-, 16-, and 24-second
cold-open checks before deliberate shutdown. R16 checkpoint `a81d494` also
passes fresh headless Workbench creation at the same file/class counts, 46,639K
static storage, CRC `c4113d38`, exit `0`, and zero surviving engine processes.
The preceding SpawnQueue-resume tree passes headless Workbench create/destroy in
`logs_2026-07-14_13-40-55` at 5,826 Game files/11,807 classes, 46,641K static
storage, and CRC `be31cb18`, with no HST script or fatal diagnostic and zero
surviving engine processes. R22 supplies runtime confirmation for the
SpawnQueue resume-selection delta. The dated active-demolition-witness tree
passes fresh headless Workbench Game/PC validation in
`logs_2026-07-14_14-41-29` at 5,826 Game files/11,807 classes, 46,643K static
storage, and CRC `c3ab042e`; validation reports `Script validation successful`,
no HST compile or fatal diagnostic, and zero surviving engine processes after
the test closes. R23 supplies the matching targeted runtime evidence above.
The wider Full Campaign Debug suite is still diagnostic rather than
certification. Packaged restart, dedicated-server/client, multiplayer,
reconnect/JIP, and soak evidence remain separate gates. Exact runtime identities
and results belong in the Campaign Debug verification audit.

## Historical Schema 70 / Settings 24 Exact Enemy Garrison-Rebuild Engine-Proof Checkpoint

At that historical checkpoint, the development tree used Campaign Schema 70
while runtime settings remained on Schema 24. Newly admitted enemy garrison
rebuilds use a separate
contract-`1` operation aggregate backed by one frozen infantry manifest, one
held SpawnQueue batch, and one active-group projection. Admission caps the
accepted roster by authoritative source infantry and the target garrison's
remaining capacity, then freezes and preflights the roster plus selected-zone
ownership capability. It charges exactly 10 support resources only after that
preflight, then builds the reciprocal runtime graph or performs the exact full
rollback. Historical rebuild
rows remain contract `0`; Schema 70 never infers their missing roster, debit,
operation, or delivery authority.

The frozen planning capability includes both selected zones' owners and
ownership revisions, and the order persists the target revision. Target or
source ownership ABA on initial admission is rejected before pressure even if
the owner later returns to the original string. A pressure-marked retry rechecks
the capability and rejects before order creation or debit. During execution,
virtual and physical modes project the same durable living slots. Confirmed casualties
survive materialization, fold, restore, return, and settlement; no render-bubble
transition may refill the roster.

Delivery records one durable zero-delta `delivered_garrison_transfer` receipt
and links the exact manifest to the destination garrison while the operation
remains `OPEN` and `ON_STATION`. The garrison does not
also increment aggregate infantry, preventing the same force from being counted
twice. The delivered roster remains held exact authority for later physical or
virtual projection. A delivered terminal event unlinks and retires that roster
without a refund. Before delivery, invalidated survivors return to origin and
eligible terminal paths refund the original support debit exactly or
proportionally from durable casualty truth.

Settlement reuses the Schema-69 `PREPARED` intent boundary. Resource receipt,
operation settlement, and final order lifecycle are resumable, independently
idempotent steps. A valid restored PREPARED or SETTLED prefix converges on the
same terminal order state without changing receipt identity or amount. Receipt,
terminal-policy, or settlement-identity conflicts fail closed at quarantine
contract `-70`.

Schema-70 restore classification runs before generic projection normalization;
final validation runs after generic, canonical ownership, strategic-resource,
and Schema-69 counterattack prerequisites. Valid process-bound projections fold
to strategic hold. Malformed, duplicate, partial, orphaned, or foreign claimants
retain evidence, but every claimed nonterminal batch is made non-executable and
process-only group state is cleared. Retention pins the quarantined reciprocal
identities so generic pruning cannot erase the evidence needed for diagnosis or
future explicit repair.

The scoped checkpoint is sealed at implementation
`2f71236bfc02329a3c8000b104f1b7b1043dc99c`, UTC
`2026-07-13T22:20:52Z`, label
`schema70-settings24-exact-enemy-garrison-rebuild-engine-proof`, and stamp
`ef95555`. That checkpoint's post-clock-fix Workbench log `logs_2026-07-14_01-06-19`
exits `0`, completes create/destroy, compiles 5,826 Game files/11,806 classes at
CRC `b819d967`, contains no HST script error or new native crash event, and
leaves zero Workbench processes. The focused autotest log
`logs_2026-07-14_00-52-56` at CRC `6fa838ee` records one passing
`HST_TEST_EnemyGarrisonRebuildAuthority` JUnit testcase, no JUnit failure, an
empty failed list, exit `0`, and zero surviving processes, but predates the
coordinator clock correction,
covering
capacity/admission, held delivery, casualty continuity, restore, ownership
terminal settlement, rollback/refund, PREPARED and SETTLED crash recovery,
historical isolation, quarantine and retention, and selected target/source ABA
rejection. Foundation passes at 790 script-symbol references. The focused
environment still records the known recoverable base-game VM diagnostic plus two
filter-constructor diagnostics. The focused run succeeds but is not exception-
free.

Post-seal campaign-debug isolation tracks every created enemy order by its
stable admitted ID. It never retags a debit-backed or exact-operation identity.
Before Phase 18 background-war setup or any Phase 24 ownership, resource-pool,
HQ, population, or aggression mutation, the coordinator dispatches open exact
QRF, counterattack, patrol, and rebuild orders to their typed administrative
settlement owners. Those owners report success only after their terminal ledger
is valid and every batch, group, adapter handle, physical root, and runtime
member claimant is gone. Contract-zero debug orders instead settle through the
enemy commander and PhysicalWar owner, with the original debit, deterministic
refund ID, one exact refund claimant, mutation shape, and chronology proven
before terminal status. Any failure blocks the state rewrite. Victory/loss
inactivity snapshots are captured only after a terminal-maintenance frame has
quiesced legitimate settlement.

The recorded CLI run `seed1985_t0_p1_u1784003276` executed 711 cases: 273
PASS, 40 WARN, 388 FAIL, and 10 BLOCKED. It proved 4,356/5,003 required
certification assertions, with 598 failed and 49 blocked. Bootstrap passed and
the restored state diff was zero, but this is not full runtime certification.

Schema-70 `early_mechanics.force_authority` ran. Live-player proximity trapped
its delivery, restore, and ownership branches in `MATERIALIZING`, so the
separately corrected source-only harness now forces deterministic virtual
fixture projection without changing production behavior. That repair needs a
fresh CLI rerun. Full Debug
enemy authority was healthy at campaign second 26. Captive-follow debug sampling
began at second 109 and advanced the shared campaign clock six times by five
seconds while ticking only `MissionRuntime`. The next normal resource tick
correctly quarantined both exact enemy pools for `enemy resource cadence
checkpoint diverged`. This test-harness clock leak caused the later strategic-
mission terminal/containment cascade and Phase 18 `AddResources` rejection; it
was not a separate production resource-authority defect. Source now removes
those shared-clock increments and asserts exact clock plus enemy-authority
fingerprint isolation. Production mission/resource fail-closed gates remain
unchanged and correct. A rerun is required to prove cascade removal. Earlier
Phase 10-11 native exact crew-seat materialization/rollback containment failures
and other runtime defects remain separate and open. Packaged/native, dedicated-server,
serialization/restart, network/JIP/reconnect, and soak proof remain open.

## Historical Schema 69 / Settings 24 Exact Enemy Counterattack Checkpoint

The immediately preceding campaign checkpoint used Campaign Schema 69 while
runtime settings remained on Schema 24. A newly admitted enemy counterattack
belonged to one versioned contract-`1` aggregate spanning the enemy order,
operation, frozen infantry manifest, held SpawnQueue batch, active-group
projection, direct strategic route, combat state, ownership request, and
resource settlement. The manifest is the sole roster authority: confirmed
casualties remain absent across virtual travel, materialization, fold, restore,
return, and settlement.

The operation may spend exactly one canonical enemy pool. Proactive attacks use
the attack pool; reactive capture responses may use the support pool. Virtual
combat is deterministic and updates the same roster used by physical
projection. A successful attack cannot publish ownership directly: it submits
to the canonical ownership-transition service and remains open across retry.
After the ownership result, living survivors return to origin and refund the
same originally charged pool proportionally; travel or projection changes never
refund resources.

Settlement uses the appended `PREPARED` operation-settlement state as durable
terminal intent without changing earlier persisted enum ordinals. The authority
prepares the operation, stages the exact order/refund tuple, applies or replays
the canonical refund, records the resource receipt, and only then finalizes the
operation and reciprocal runtime rows. Restore and same-session ticks resume
that sequence idempotently. Claimant scans include explicit backlinks and every
deterministically derived batch, projection, force, and execution identity, so a
duplicate or foreign residue holds/quarantines instead of being mistaken for a
safe cleanup candidate.

Counterattacks restored from Schema 68 or earlier remain historical contract
`0` and do not acquire exact authority. Current missing, duplicate, ambiguous,
or malformed exact graphs quarantine at `-69`. Quarantine holds the graph and is
idempotent; it must not fabricate or delete an order, operation, manifest,
roster, route, debit, refund, settlement, or ownership outcome.

This scoped engine-proof checkpoint is sealed at implementation
`5bdcda938840ab769b41ff3e1856d908572a8c45`, UTC
`2026-07-13T19:40:35Z`, label
`schema69-settings24-exact-enemy-counterattack-engine-proof`, with stamp commit
`73a64ef`. Foundation passes at 771 script-symbol references. Final all-five
Workbench log `logs_2026-07-13_15-41-50` exits `0`, compiles 5,821 Game files/
11,786 classes at CRC `3a8bd64f`, explicitly validates WORKBENCH, PC, XBOX, PS4,
and PS5, contains no script or HST errors, and leaves zero Workbench processes.
Focused engine log `logs_2026-07-13_15-42-52` exits `0`, records one passing
JUnit testcase, an empty failed list, and `AllExact=1`. It covers valid PREPARED
recovery, same-session ABORTED recovery, foreign derived-ID collision hold, and
fail-closed rejection of SETTLED state without a resource receipt. The autotest
environment also writes a recoverable base-game
`SCR_EditableEntityCore/GetPlayerIdentityId` VM exception to `crash.log` before
the HST test completes successfully, so this run is not exception-free.

The seal covers source, Foundation, all-target Workbench, and the focused engine
case only. Full Campaign Debug in `HST_Dev`, serialization/restart, packaged and
native runtime, live-server behavior, migration runtime, marker runtime,
network/JIP/reconnect, and soak proof remain open.

## Sealed Schema 68 / Settings 24 Enemy-Planning Engine-Proof Checkpoint

The preceding sealed source/Workbench checkpoint uses Campaign Schema 68 and
runtime-settings Schema 24. It is sealed at implementation
`4c9a94a1cb4811b6e75a7dca5dba70efffcb523d`, UTC
`2026-07-13T15:43:01Z`, label
`schema68-settings24-enemy-planning-engine-proof`. Foundation passes at
753 script-symbol references. Final stamped-tree all-target Workbench log
`logs_2026-07-13_11-43-49` compiles 5,816 Game files/11,770 classes at CRC
`5a998c21`; WORKBENCH, PC, XBOX, PS4, and PS5 report `Script validation
successful`, the process exited, and zero Workbench processes survived cleanup.
A focused command-line Game-process autotest now forms an intermediate proof rung
between Workbench validation and Campaign Debug. Case
`HST_TEST_EnemyPlanningCommitmentAuthority` ran in
`logs_2026-07-13_11-44-28`; its JUnit report at
`2026-07-13T15:44:34.667Z` contains one testcase, no failure, and an empty
failed list. `HST_EnemyPlanningProofReport.AllExact()` was true across all 17
deterministic Schema-68 planning fixtures, including retry quarantine and its
repeated-pass idempotency check. This proves the isolated planning services
executed in an engine process. Full Campaign Debug in `HST_Dev`, coordinator
isolation and artifact generation, live-authority observation, world
integration, package execution, actual migration/save/restart, dedicated and
live-server behavior, multiplayer/networking, and soak evidence remain open.

The immediately preceding schema-neutral bootstrap/profile/marker checkpoint is
sealed at implementation `fdf262637e74a70c12454f6c1d3789c2cd0a0f05`, UTC
`2026-07-13T13:19:22Z`, label
`schema68-settings24-bootstrap-profile-marker-hardening`, and final stamped-tree
all-target Workbench CRC `0544aa1d`.

The latest packaged server test loaded implementation `f97b12e`. It created and
used the canonical `$profile:Partisan` root, but no retired profile tree existed,
so migration was not exercised. The run exposed a fresh-start ordering defect:
both configured enemy strategic pools and planning rows were quarantined at
`-67`/`-68`, and the unchanged unavailable condition produced 598 warnings at
roughly one-second cadence.

That preceding source pass addresses the exact packaged failure without
weakening restore safety:

- One production bootstrap factory creates the current-schema fallback for
  startup, admin reset, and deterministic fresh-state proof. It installs the
  configured three-role pool topology and both idle enemy planning rows before
  restored-role validation. Persisted state still replaces those fallback arrays
  before validation, so unrelated malformed saves remain fail-closed.
- A schema-neutral recovery accepts only the complete known Schema-68 bootstrap-
  quarantine signature: the exact nonempty preset identity, exactly three pool
  rows with one neutral resistance row
  and the two exact poisoned enemy rows, exactly two poisoned planner rows, no
  null rows, and completely empty strategic-mutation and enemy-order arrays. It
  restores configured starting balances and baselines cadence at the current
  campaign second; topology, resource, preset, null-row, legacy-order, versioned-order,
  or other near misses remain unchanged and quarantined.
- Repeated unchanged planner-unavailable warnings are transition-based with a
  300-second reminder. A changed failure and recovery are reported immediately.
- Live Campaign Debug observes the real campaign through the same exact pool and
  planner admission resolvers used by production. It requires exactly two
  configured enemy pools/planners plus one neutral resistance pool and performs
  no mutation while gathering evidence.
- Before any profile consumer runs, arbitrary nested retired-profile files are
  copied through a verified staging area, the destination is rechecked before
  promotion, and source/destination bytes are compared again before source
  removal. Canonical file conflicts are retained under `legacy-profile-archive`;
  structural conflicts are mirrored under its `directory-conflicts` subtree;
  the retired root is removed only after all verified moves and deepest-first
  empty-directory cleanup succeed. A static guard prevents same-process re-entry.
  Enforce supplies no atomic cross-process no-overwrite promotion or exclusive
  file lock, so migration has an explicit single-writer startup contract.
- Campaign Debug now has an owner-client marker-integrity probe that mutates and
  deletes a protected campaign marker, invokes production self-heal, proves one
  canonical system-owned/non-removable projection with stable registry state,
  and separately proves ordinary player-marker edit/remove isolation. Final
  production repair and player-marker cleanup are retried before the destructive
  probe returns, including on a failed assertion path.

Those bootstrap/profile/marker changes are sealed only as source and all-target
Workbench evidence. They have not passed Campaign Debug, package execution, a
packaged restart, actual retired-tree migration, multiplayer, or soak proof yet.

That historical Schema-68 checkpoint extends the earlier work with a
schema-neutral, commitment-aware enemy-planning correction. Target admission
examines queued or active same-faction enemy orders and support requests plus open same-faction
operations at the equivalent zone identity. Linked order, support, and operation rows
collapse to one root commitment. An incompatible root removes the target before
scoring or ranking; an exact active patrol root remains eligible for a defensive
response but still blocks another patrol. If that compatible candidate later
resolves to a patrol order, preparation excludes it from the current selection
pass and deterministically reranks without changing the full candidate
fingerprint, so the same due cadence can choose a valid fallback. A root that
contains both compatible and blocking rows is classified once with conservative
blocking precedence. Each compatible root applies a deterministic `-12` score
penalty, capped at `-24`. The target-candidate fingerprint is now `ept2` and
includes the compatible-root count and penalty.

Decision preparation is freeze-only and has no target-pressure, resource, or
order side effect. Admission recomputes commitment identity before debit or
order creation even for a pressure-marked retry; an unpressured decision also
recomputes the target-candidate fingerprint before pressure. If every otherwise
eligible target is incompatibly committed, the decision completes as an
explicit zero-cost, zero-pressure `skipped` result. Existing unpressured
prepared rows carrying the older target-candidate identity can fail closed at
admission; no campaign- or settings-schema bump is required. This correction is
sealed as source, Foundation, and all-target Workbench evidence at the identity
and CRC above. Current save validation delegates planning quarantine to the
production `HST_EnemyPlanningAuthorityService.Quarantine()` boundary, retaining
bounded failure text while applying the same failure reason, revision increment,
and idempotent already-quarantined behavior as live authority. The retry-tamper
fixture advances the campaign clock to its recorded retry time before retrying,
so it proves the valid retry gate before fingerprint quarantine rather than
bypassing cadence. The focused command-line autotest executes all 17 deterministic
planning fixtures successfully. Matching Campaign Debug assertions remain wired
but unexecuted, and package execution, save/restart, dedicated and live-server
behavior, multiplayer, and soak proof remain open.

The engine-executed deterministic fixtures cover baseline adoption, independent
cadence, replay/conflict, commitment permutation and selection, freeze/retry and
crash windows, prepared-order adoption, retry-tamper quarantine, zero-target and
all-committed skips, roundtrip/current quarantine, fresh bootstrap, warning
throttling, queued order/support blockers, settled/terminal/rival isolation,
equivalent zone identities, mixed-root blocking precedence, stable diagnostics,
patrol fallback, and both commitment races. This focused in-memory service proof
does not instantiate HST_Dev, the coordinator, a campaign world, persistence, or
network transport; those separate boundaries cannot inherit its result.

The base Schema-68 planning-authority checkpoint remains historical while
runtime settings remains Schema 24. Its exact identity is implementation
`356b0d47f96111c3b09eb7ede3cb34f0661c2b6e`, UTC
`2026-07-13T01:04:41Z`, label
`schema68-settings24-enemy-planning-authority`, Foundation 744, and Workbench CRC
`971d30d0`. The earlier Schema-67 resource checkpoint is implementation
`2798cb20b824ed74419ab6dc9bdce03f18ef71df`, UTC
`2026-07-12T23:46:02Z`, and label
`schema67-settings24-enemy-strategic-resource-authority`; Foundation passes at
736 script-symbol references. Final stamped normal log
`logs_2026-07-12_19-52-14` and all-five log
`logs_2026-07-12_19-52-36` both compile 5,809 Game files/11,751 classes with CRC
`a353fa0d`. All-five reports `Script validation successful` for WORKBENCH, PC,
XBOX, PS4, and PS5; zero HST script errors were observed and zero Workbench
processes survived cleanup. This begins Blueprint Phase 9 source work; it does
not certify the still-open Blueprint Phase 8 native, restart, package,
multiplayer, marker-input, or soak gates.

The public product identity is **Partisan: Everon** and the repository identity
is **Partisan**. `HST_*` and the non-public `histasi` Workbench project ID remain
the internal code/resource namespace. `$profile:Partisan` is the canonical root
for generated settings, campaign fallback data, loadout-editor settings,
personal loadouts, debug artifacts, and verified conflict archives. Migration is
whole-tree and canonical-wins: nonconflicting files keep their relative paths,
conflicting retired files are archived without overwriting canonical data, and
no source file is deleted until its destination is byte-identical. New
destinations use a verified stage and final destination recheck; same-process
re-entry is guarded. The supported startup model is single-writer because
Enforce has no atomic cross-process no-overwrite promotion or exclusive lock.

Schema 67 version-controls `HST_FactionPoolState` as the sole per-enemy owner of
attack resources, support resources, aggression, and independent resource/
aggression cadence accumulators plus last-processed bucket checkpoints. Bounded
`HST_EnemyStrategicMutationState` receipts retain immutable mutation identity,
faction identity, kind, per-faction operational sequence, before/delta/after
values, source, time, and linked order, operation, and accounting evidence. One
server-side mutation boundary owns income, spend, refund, aggression, and live
admin/debug adjustment; exact replay is read-only, conflicting identity fails
closed, and underflow/overflow rejects atomically. Accepted zero-effect
operational commands retain receipts instead of disappearing as no-ops.

Operational history is an explicit lifetime admission bound, not an archive:
Schema 67 never evicts or compacts operational receipts. Each enemy role owns a
contiguous sequence of at most 4,096 operational receipts; reaching the bound
blocks only later operational mutations for that role. The rival role and
periodic income/decay continue independently. Periodic evidence is different:
one receipt per enemy/kind is compacted as newer buckets complete, while the
persisted accumulator and last-bucket checkpoint must close to the campaign
clock. Rejected/orphan/prior-quarantine rows are attributed and quarantined before
being purged from the canonical array, so they cannot consume valid per-role
capacity.

Existing exact defensive-QRF and enemy-patrol operation shapes remain unchanged
but their debit/refund evidence links reciprocally through the order's resource
mutation IDs. Restore additionally validates matching faction/operation/
manifest/zone facts, a unique support ledger for defense mutations, and typed
town-influence and ownership-transition sources. Unsupported enemy order
families remain legacy/deferred. Pre-67 migration adopts valid current
balances/aggression and legacy resource/aggression cadence accumulators as the
baseline, initializes the cadence checkpoints, and creates no history, spend,
refund, settlement, order, or planning decision; malformed current graphs
quarantine at `-67`. Mission terminal results admit a deep-copy-preflighted
strategic plan before status/reward/capture publication. Ownership transitions
admit the stable aggression mutation before security/support reconciliation and
owner publication. Nested Campaign Debug state cases use disposable clones, and
the focused physical-response case materializes/cleans only its supplied group
while restoring shared marker and AIWorld state. Persisted per-enemy planning
cadence and a frozen target/source/order/cost decision fingerprint are not part
of sealed Schema 67. They are the sealed Schema-68 slice above.

Schema 68 keeps planning truth separate from Schema-67 resource truth. Each
configured enemy role owns one `HST_EnemyPlanningState` with an independent
180-second checkpoint, revision, decision sequence, and latest disposition.
Stable sorted commitment, target-candidate, and source-candidate hashes freeze
the complete observed set rather than array order. The input fingerprint also
freezes war level, aggression, pool revision/balances, operational mutation
count, and commitment/candidate counts. The decision fingerprint freezes target,
source, order type, support type, capability/manifest facts, spend mode, attack/
support costs, target-pressure projection/application, and deterministic
decision, order, operation, and Schema-67 debit identities.

The current target-candidate pass is commitment-aware rather than treating the
commitment hash as observation-only evidence. It collapses linked queued/active
same-faction order, support, and open-operation rows to one root identity,
rejects incompatible target roots before ranking, and applies a deterministic
`-12` per-compatible-root penalty capped at `-24`. Exact active patrol is the
intentional compatibility exception: a non-patrol defensive response may still
target that zone, while the final order-type gate prevents a second patrol. If
one root has both compatible and blocking evidence, blocking wins and the root
is counted once. Known equivalent canonical/legacy zone IDs share commitment
identity, and rejection count/first-reason diagnostics remain deterministic
across array permutations. `ept2` fingerprints both the compatible-root count
and its score contribution.

Order type is selected only after target ranking. If the highest eligible exact-
patrol-compatible target deterministically resolves to `PATROL`, preparation
temporarily excludes that zone and reruns the same salted ranking against the
unchanged full `ept2` candidate fingerprint. It freezes the next compatible
target, or a clean no-target result, within the same due decision instead of
wasting the 180-second cadence on a duplicate-patrol skip.

The planner first persists a due choice as `prepared`. A retry keeps the same
frozen choice and advances only a bounded 30-second retry checkpoint.
Preparation itself is freeze-only. Before a prepared decision without an
existing durable order can debit resources or create an order, admission always
recomputes its commitment fingerprint, including for pressure-marked retries.
An unpressured row also recomputes the `ept2` target-candidate fingerprint and
revalidates the frozen target, source, and order-type gates before applying
pressure. A changed commitment therefore rejects a pressure-marked retry before
debit, while a changed candidate set rejects an unpressured row before any side
effect. If commitment filtering leaves no target, the frozen decision carries
zero cost and zero pressure and completes as `skipped`. Completion remains
explicit: `committed` requires one exact order and applied Schema-67 debit
receipt, `skipped` cannot claim applied target pressure, and `rejected` may
retain pressure already applied by an older or later-failing path. A prepared
row restored with its exact order and debit crash-window graph may reconcile to
committed. Periodic checkpoints remain independent, so a blocked faction cannot
advance or suppress its rival.
At the sealed Schema-68 checkpoint, immediate counterattacks and existing
debug/direct order entry points remain planning contract `0`. Provisional
Schema 69 changes only newly admitted counterattack operation contracts; it does
not rewrite Schema-68 planning history.

Pre-68 restore invents no decision. It clears claimed planner state and old-order
planner backlinks, then creates one configured-role baseline only after exact
Schema-67 pool roles are known: `last = elapsed`, `next = elapsed + 180`,
sequence `0`, and `idle`. Current missing, duplicate, foreign, malformed,
cadence-tampered, fingerprint-divergent, or broken order/debit graphs quarantine
at `-68`. Quarantine changes planning and order-planning metadata only; it never
changes or reconstructs a Schema-67 pool or strategic mutation receipt.

The immediately preceding sealed source/Workbench checkpoint advances the campaign save contract to Schema 66
while runtime settings remains Schema 24. The sealed Schema-66 stamp identifies
implementation `a7031797e67d99a99a066038cd8fa39efc03cff1`, UTC
`2026-07-12T20:28:33Z`, and label
`schema66-settings24-local-security-marker-integrity`.
`HST_LocalSecurityOperationService`
owns one exact patrol epoch for each eligible canonical enemy-held town with
positive police pressure. The durable graph is one deterministic
`HST_LocalSecurityPatrolState`, one typed operation, one frozen authored 2–5
member manifest, one SpawnQueue batch, and one active-group projection. Native
group roots and cyclic town waypoints are adapters only. Physical casualty
reconciliation precedes fold, persistence, ownership transition, and terminal
settlement; virtual hold and restore keep the same living member slots and never
refill a damaged roster.

An eliminated roster applies one idempotent police `-1` town-influence event and
then compacts to terminal operation/manifest history. It cannot respawn in the
same epoch simply because police pressure remains above zero. Rearm requires a
newer ownership revision or an applied positive police-pressure event after the
terminal receipt. Owner change, cleared security pressure, setup/campaign stop,
and spawn failure settle without a destruction loss. Resistance-held towns
target zero automatic police and roadblocks; resistance aggregate garrison
creation remains a separate ownership policy. Pre-66 migration removes only
unlinked disposable legacy police projection rows while preserving logical
political/security/garrison facts and inventing no casualties, roster, fold
credit, operation, or refund. Malformed current authority quarantines at `-66`.

That sealed source also repairs the Schema-61 client-local campaign-marker
ownership regression introduced at commit `27672e6`. Protected projection
records bypass the native local-owner assignment, enter the static marker array
as system-owned/non-removable markers, and retain those flags after widget
creation. The projection keepalive compares live position, label, icon, flags,
owner, and removability with the authoritative registry and rebuilds deleted or
mutated campaign markers. Dynamic/player-created markers remain separate and
editable. Foundation passes at 729 script-symbol references. Final stamped
normal/all-five Workbench checks pass at 5,806 Game files/11,740 classes with
CRC `ec860be7`, `Script validation successful`, zero HST script errors, and zero
surviving Workbench processes. Every native/package/restart/multiplayer/marker-
tamper gate remains open.

The previous sealed source/Workbench checkpoint is Campaign Schema 65/runtime-
settings Schema 24. It identifies
implementation `609add9eeadf73816764c497178e2d35081307d1`, UTC
`2026-07-12T18:30:29Z`, and label
`schema65-settings24-civilian-consequence-authority`. It extends the sealed ambient-runtime
foundation with exact casualty, resistance vehicle-theft, and nearby-combat
consequences; persisted locality danger episodes; and a native pedestrian
`Wandering -> Panicked -> Recovering -> Wandering` adapter. Final stamped normal
compile/create and all-five validation are clean at 5,802 Game files/11,728
classes with CRC `c0a672b9`; all-five reports `Script validation successful`,
both runs exited `0`, zero HST script errors were observed, and zero Workbench
processes survived cleanup. The preceding unstamped CRC `be076102` remains
preliminary evidence only. Foundation passes at 717 script-symbol references;
Campaign Debug, package, restart, and soak evidence remain open.

The previous sealed source/Workbench checkpoint keeps campaign-state Schema 64
and advances runtime settings from Schema 23 to Schema 24 for a Blueprint Phase
8 ambient-runtime slice. It identifies implementation
`6afadc7c13681b78171939a740862e52328beffd`, UTC
`2026-07-12T15:57:55Z`, and label
`schema64-settings24-ambient-runtime-authority`. One transient authority now owns global actor and traffic budgeting,
leased fair locality allocation, asynchronous behavior admission, movement
health, bounded recovery, and recycle. Ambient projection remains separate from
logical population and persistence: only an observed player-occupied vehicle is
promoted to durable `field_vehicle` authority.

That sealed checkpoint's source/Workbench evidence is Foundation at 711 script-symbol references;
normal compilation at 5,799 files/11,718 classes with CRC `bb083672`; successful
validation for all five configurations; zero HST script errors; and zero
surviving Workbench processes. Deterministic allocator, lifecycle, settings-
migration, and save-boundary proofs are compiled and wired but have not run
through Campaign Debug. Native server execution, ten-town/ten-minute soak,
brief enter/exit observation, autosave/restart, promoted-root destruction,
new-campaign reset, Campaign Debug Phase 20 production-path execution, automatic
casualty/theft/nearby-combat influence and panic/recovery behavior, rendered
behavior, stutter measurement, and multiplayer remain open. Commander aid and
ownership/security-pressure paths exist in source but still need runtime proof.
The later sealed Schema-66 slice implements exact enemy-town local security, but
that work is not covered by this older sealed evidence.

The preceding sealed checkpoint is campaign Schema 64 on runtime-settings Schema 23.
It adds one canonical town-influence state owner, strict political hysteresis
through the existing ownership transaction, a conservative migration/quarantine
boundary, and pure contacted-town/territory Map/War projections. Legacy
support/population fields are compatibility projections only. Schema 64
identifies implementation
`6f3c913eaed66926cce38b2ecafcff94084898a3`, UTC
`2026-07-12T11:28:41Z`, and label `schema64-canonical-town-influence`. It has no
Campaign Debug/package result. Its dedicated Foundation gate passes within the complete
696-reference run. Normal Workbench compilation and all-five-configuration
validation pass at 5,793 files/11,695 classes with CRC `36d5b017`, successful
validation, and zero HST script errors. Every Workbench instance was closed;
the verified post-run process count was zero. Real save/restart, rendered UI,
stutter measurement, and multiplayer execution remain unproven for that sealed
checkpoint.

Schema 63 is an earlier sealed source/Workbench checkpoint. It identifies implementation
`85a75c65e9c148a890d8d78b0288ae6483a5ccd9`, UTC
`2026-07-12T08:22:05Z`, and label
`schema63-canonical-combat-presence`.

Schema 62 is an earlier sealed canonical-ownership checkpoint under implementation
`7c93e0a485bcabe5a364c0b0cfeca235accb50f7`, UTC
`2026-07-12T06:11:19Z`, and label
`schema62-canonical-ownership-transition`. Foundation passes with 670 script-
symbol references. Headless Workbench Game validation loaded 5,785 files/11,652
classes with CRC `22c13a32` and zero script errors; the normal Script Editor open
remained responsive without a crash, and zero Workbench processes survived the
test. Schema 61 is the preceding sealed marker-projection foundation. Campaign
Debug and packaged-runtime evidence remain open.

## Runtime Ownership

`HST_CampaignCoordinatorComponent` is the server-side entry point. It owns a
single `HST_CampaignState` and delegates to small services. The cross-cutting
schema-42 through sealed schema-67 source authority services, plus the sealed
Blueprint Phase 8 transient runtime services, are:

- The Schema-67 enemy strategic resource boundary makes each versioned
  `HST_FactionPoolState` authoritative for one enemy role's attack resources,
  support resources, aggression, and persisted resource-income/aggression-decay
  accumulators/last-bucket checkpoints, plus its operational receipt count. It
  must be addressed by stable faction ID,
  never by an ambiguous first-enemy lookup.
- `HST_EnemyStrategicMutationState` is bounded durable replay evidence, not an
  event-sourcing requirement. Each receipt freezes mutation ID, contract/applied state,
  faction, mutation kind, per-faction operational sequence, signed deltas,
  before/after values, source, campaign second, and any exact order/operation/
  transaction backlinks. Accepted zero-effect operational commands also retain
  one row. Exact replay
  returns the retained result without mutating; fingerprint conflict and
  arithmetic underflow/overflow leave every value unchanged and fail closed.
- `HST_EnemyStrategicResourceService` owns income, spend, refund, aggression,
  and live admin/debug adjustment. Operational history is never compacted and
  hard-stops at 4,096 rows per faction; periodic evidence compacts separately.
  Exact defensive-QRF and enemy-patrol code retains its
  operation and settlement policy but receives canonical debit/refund receipt
  links. Legacy/deferred order families remain on their explicit compatibility
  paths until separate cutovers.
- This resource boundary does not persist commander planning cadence or freeze
  target/source/order decisions. Sealed Schema 68 owns those
  decision inputs and fingerprints as a separate contract on top of Schema 67.
- `HST_EnemyStrategicResourceProofService` is the deterministic state-only proof
  owner for legacy baseline adoption, replay/conflict/atomicity, income catch-up
  and contribution fingerprinting, attack-versus-support separation, faction
  isolation, aggression-versus-war-level independence, roundtrip, and `-67`
  quarantine. It is not Campaign Debug, process restart, or packaged evidence.
- `HST_EnemyPlanningSaveValidationService` delegates Schema-68 planning
  quarantine to `HST_EnemyPlanningAuthorityService`. Save validation still
  bounds imported failure text, while the authority owns failure reason,
  revision advancement, and idempotent repeat behavior. This prevents restore
  validation and live planning from maintaining different quarantine semantics.
- `HST_EnemyPlanningProofService` owns the 17 deterministic Schema-68 planning
  fixtures shared by Campaign Debug and focused engine tests.
  `HST_TEST_EnemyPlanningCommitmentAuthority` runs that report through the
  official command-line Game-process autotest framework and requires the three
  commitment-specific fields plus `AllExact()`. Its passing JUnit result is
  engine-executed service evidence, not coordinator, world, persistence,
  restart, package, or network evidence.

Schema 60 also separates recurring enforcement reads from identity refreshes:
the one-second maintenance path reuses registered player authority, while
connect/setup and explicit visible actions retain the full backend identity and
membership refresh boundary. This removes repeated authority resolution from
the hot tick without making cached presentation state an authorization source.

- `HST_CombatPresenceService`: is the sealed Schema-63 state-only
  authority for hostile and exact-faction presence near a point or zone. It
  never scans the world. Fresh registered physical samples count conscious
  dismounted infantry, each operational occupied armed mobile platform once,
  and each operational occupied static weapon once. Cargo occupants, empty
  vehicles, unarmed pilot-only vehicles without composition proof, destroyed or
  burning platforms, and immobile mobile platforms contribute nothing. Virtual
  exact convoys use surviving crew, never an abstract vehicle count; other
  eligible virtual groups use their durable infantry authority only while a
  current operation/materialization status proves them active. Settled,
  terminal, quarantined, or stale rows do not contribute. A spawned row whose
  physical authority is unresolved makes the query invalid, so safety/capture
  consumers block or choose their documented conservative result instead of
  guessing. Legacy QRF arrival resolution uses the group-level tri-state query:
  unresolved or population-pending authority waits for a later sample, while
  only authoritative zero pressure or missing/terminal durable group authority
  can fail the response. The
  coordinator injects one service instance into every consumer. That instance
  builds the eligible-contribution set once per campaign state/elapsed second,
  then reuses it for all radius/zone queries instead of rescanning every active
  group for every consumer and every zone.
- The physical sampler keeps a persistent group-to-runtime registry index and
  reuses member, platform, and agent scratch arrays. The query service separately
  caches contribution snapshots, authority-gap results, and zone/radius results
  until an explicit same-tick mutation invalidates them. These allocation-light
  boundaries avoid rebuilding maps/arrays and rescanning all groups for every
  consumer on the one-second coordinator path.
- The same service now answers zone capture, mission contact/area checks, HQ
  threat, civilian hostile-proximity checks, enemy target pressure/reactive
  defense/roadblocks, and the legacy group combat-present wrapper. A crewless
  vehicle therefore cannot become capture, mission, HQ, civilian, or strategic
  pressure merely because its durable survivor-vehicle count is nonzero.
- Zone capture counts a player only when the controlled entity is a living,
  conscious character. Spectator cameras, Game Master proxies, and other
  non-character controlled entities cannot satisfy friendly presence. Invalid
  hostile or exact-faction authority blocks capture rather than appearing clear.
- Each zone persists a revisioned `HOT`, `COOLING`, or `COLD` snapshot with
  separate infantry/manned-vehicle/static-operator counts, current-operation and
  recent-fire diagnostics, a deterministic contributor hash, and at most 24
  sorted contributor facts. Live contributors make a zone `HOT`; the first
  clear sample starts `COOLING`; the unchanged deadline is not extended by each
  tick; and expiry makes the zone `COLD`. Runtime-settings Schema 23 exposes
  `capture.combatPresenceCoolingSeconds`, default 30 seconds and normalized to
  the inclusive `1..300` range. Stable `HOT`/`COOLING` scans and freshness-only sample
  refreshes do not dirty persistence every second.
- `HST_CombatPresenceSaveValidationService`: invalidates every process-local
  physical sample on restore. Pre-63 saves receive a revision-1 `COLD` baseline
  without inferring combat from vehicle, marker, or generic group rows. Valid
  current `HOT`/`COOLING` diagnostics may restore, while malformed enum, count,
  deadline, hash, ordering, bound, or reason authority is reset conservatively
  to `COLD`.
- `HST_CombatPresenceProofService`: exercises empty-vehicle exclusion,
  authoritative infantry/mobile/static samples, virtual casualty continuity,
  exact `HOT -> COOLING -> HOT -> COOLING -> COLD` timing, pre-63 migration,
  current cooling restore, and malformed-current fail-closed normalization. It
  now passes all nine assertions in the dedicated focused engine case. That is
  deterministic state-only fixture evidence, not Full Campaign Debug or native-
  entity proof. Foundation passes at 681 references, normal Workbench compile/create
  succeeds at 5,788 files/11,670 classes with CRC `a40056c5`, and explicit
  five-configuration Script validation succeeds.
- `HST_PhysicalWarService.UpdateZoneActivation()` now uses the activation radius
  to enter the render bubble and the larger deactivation radius to leave it.
  This render hysteresis is deliberately separate from combat-presence heat:
  active entities may fold without erasing durable combat truth, and an empty
  rendered vehicle does not keep a zone hot.
- Coordinator tick order treats the shared cache as a snapshot boundary. It
  invalidates after mission, support, enemy-order, and physical-projection
  mutations; refreshes native registered-member samples before mission-runtime
  contact queries; invalidates/rebuilds after later projection changes; then
  derives zone heat before capture. This prevents a one-second cached result from
  surviving a same-tick mutation while avoiding the former zones-by-groups-by-
  consumers hot path. Runtime profiling and behavior validation remain pending.
- `HST_TownInfluenceService`: is the sealed Schema-64 political support
  and town-population owner. Exactly one valid contract-`1` record per curated
  town stores separate FIA/occupier/invader basis points, population, contact,
  influence aggregates, and pending ownership intent. Typed command/event
  identity provides idempotency and before/after revision evidence. Schema-64
  projection into legacy zone and civilian support/population fields applies
  only to those canonical curated towns. Nonpolitical and non-town civilian
  rows have no canonical town-influence authority and retain their Schema-22
  persisted FIA/occupier support values, including zero.
- Population-scaled support uses the pinned reference commit
  `6e4226d3863ca8673535386c2fff8b6e08a806c4` and
  `round(1000 * raw delta / sqrt(initial population))`. Raw `+1` at populations
  100, 25, and 400 must yield `+100`, `+200`, and `+50` basis points. FIA
  support strictly above `8000` requests resistance ownership; strictly below
  `4000` requests enemy ownership; equality is neutral. The service owns only
  intent and delegates owner publication to `HST_OwnershipTransitionService`.
- `HST_TownInfluenceSaveValidationService`: migrates pre-64 signed zone support,
  civilian faction support, population, contact, and legacy event evidence into
  the canonical record once. For a row with no canonical town record, only a
  save restored from before Schema 22 receives the legacy FIA/occupier support
  backfill; Schema-22-and-newer stored values are authoritative even when zero.
  Current duplicate, missing, orphaned, malformed, event-inconsistent, or
  pending-receipt-inconsistent authority quarantines at `-64`; restore never
  replays event effects. Simon's Wood is not a curated town, and the Maiden's
  Bay Logistics Warehouse has no political record.
- Town influence updates aggregate counts and earliest expiry at mutation time.
  The one-second path scans event history only when a unique valid record's
  persisted next expiry is due, and then only rebuilds the due-town subset.
  Verbose changed active-group survivor/count diagnostics independently use a
  30-second keyed throttle. Both are source stutter mitigations awaiting profiling.
- `HST_MapWarProjectionService`: is a pure state read model. Zone Pressure emits
  only valid explicitly contacted towns, marks the current town from player
  position/radius, then sorts current first and the rest by FIA basis points,
  display name, and ID. Resistance Territory emits every published resistance-
  owned non-mission-bookkeeping strategic zone in type/name/ID order while
  respecting incomplete ownership publication authority.
- `HST_TownInfluenceProofService` passes all 12 registered assertions in the
  focused engine case. `HST_MapWarProjectionProofService` remains compiled and
  wired but unexecuted in this checkpoint. Neither result proves rendered
  Map/War output, native runtime, persistence/restart, JIP, or multiplayer.
- `HST_AmbientPopulationBudgetService`: is the pure, session-only allocation
  authority for physical civilian projection. It canonicalizes and max-merges
  duplicate locality demands, rejects invalid demand, and allocates pedestrian
  floors, traffic floors, then at most one actor per town per fair-remainder
  round. Traffic allocations represent drivers and therefore consume both the
  total actor budget and the nested traffic budget. The plan is stable within a
  lease of at least 120 seconds; the lease expands when the configured startup,
  stuck, and retry windows require longer. The lease epoch rotates constrained
  floors, while a separate reconciliation cursor rotates which town receives
  the first available root transaction during each health update.
- Runtime-settings Schema 24 supplies global actor budgets of `48 + 12` per
  connected player and traffic budgets of `10 + 2` per player by default. Each
  war level above one removes four percent from both budgets, capped by the
  allocator's bounded penalty. These are physical-projection limits, not logical
  population. A true town defaults to a target of five traffic drivers in
  daytime/low-heat conditions only when its remaining population permits; the
  setting can raise or lower that target, and global caps plus competing demand
  may allocate fewer, so five simultaneous cars per town is
  not a guarantee. Combined pedestrian/driver demand cannot exceed the unique
  GUID-qualified concrete appearance pool. Traffic demand is preserved first,
  pedestrians use the remaining slots, and selector exhaustion fails closed
  instead of repeating a prefab already projected in that locality.
- `HST_AmbientActorRuntimeService`: is the pure lifecycle kernel for one
  disposable pedestrian or traffic projection. An in-flight record reserves
  budget but does not become behavior-ready until its exact acknowledgement
  path completes. Pedestrians require a living CIV group member and current
  wander waypoint. Traffic progresses through vehicle, driver, seating, driver
  confirmation, engine start, and route following; production admission checks
  the exact pilot occupant, engine-on state, and current waypoint rather than
  treating a spawn or move-in request as success. Illegal transitions are read-
  only. Every record also owns an immutable projection slot within its zone/kind
  reservation set and its original projection seed. Recovery preserves that
  identity while deriving a slot- and attempt-specific wander path or traffic
  route, so actor replacement does not depend on mutable population counts or
  current position.
- `HST_CivilianService`: owns world-facing ambient transactions and transient
  lifecycle rows. The global population pass runs on the configured health
  cadence and starts no more than four ambient root transactions per update.
  Startup grace and movement samples prevent premature recovery; lack of
  progress enters a bounded replan/backoff path, and exhaustion recycles the
  root and helpers. Pending and recovering rows continue to reserve budget, but
  only acknowledged wandering/route-following rows count as ready. The logical
  town population continues independently when a projection leaves the bubble
  or recycles. Static military ambience detects owner/policy-key changes,
  recycles unclaimed old roots, promotes player claims, resets its bounded
  initialization slots, and repopulates through the same transaction cap.
- `HST_CivilianConsequenceService`: is the sealed Schema-65 source/Workbench state authority
  for automatic pedestrian casualties, resistance theft of civilian vehicles,
  and civilian reaction to nearby combat. Canonical towns submit exact typed
  commands through `HST_TownInfluenceService`; minor localities are deliberately
  panic-only and never gain a political town record. Political population stays
  independent of the number of disposable ambient actors.
- A native `OnControllableDestroyed` adapter recognizes only tracked ambient
  pedestrians or traffic drivers, allocates the casualty event ID from the
  persisted monotonic campaign sequence, and appends one row to a 256-entry
  session queue. A separate theft queue is capped at 64 rows. The queues share a
  four-transaction-per-frame drain before player-first vehicle observation and
  persistence, preventing callback re-entry from partially applying town,
  population, aggression, or event state. A rejected receipt remains queued
  indefinitely while its retry delay is bounded to 5/10/15 seconds; capacity
  exhaustion retains casualty observation before ID admission. Retained/queued/
  faulted consequence authority defers persistence capture. The health tick has
  a dead-character fallback, and the actor record's casualty flag/receipt keeps
  the callback and fallback from double-admitting the same projection.
- Player-first claim observation promotes the exact live civilian vehicle to
  durable `field_vehicle` authority before applying theft. Only an exact
  resistance player in a pilot seat produces the theft consequence, and the
  deterministic event ID is derived from the promoted durable vehicle runtime
  ID. A passenger makes the root non-recyclable during budget/health cleanup
  until exit or an exact pilot claim, but is neither a durable claim nor theft
  evidence. Military ambience and non-resistance claims do not become civilian
  theft events.
- Civilian combat reaction consumes the persisted combat-presence revision,
  current-operation count, recent-fire count, and contributor hash. `HOT` alone
  is not danger: at least one current operation or recent-fire fact is required.
  Any previously admitted but unapplied episode receipt drains before a new
  danger edge is admitted. Each false-to-true danger transition increments one
  persisted episode and applies at most one exact canonical-town event; the zone
  keeps the adopted Schema-64 baseline floor, last observed combat revision,
  last-applied episode, panic deadline, last consequence event, and a revisioned
  contract-`1` envelope. Runtime and restore require the adopted floor in `0..1`
  and `episode - lastApplied <= 1`. A clear observation ends danger without
  inventing another consequence.
- Current restore validates every live canonical-town combat receipt's complete
  fingerprint, not only ID/kind: heat is exactly `+4`; support, reputation,
  population, police, roadblock, and aggression deltas are zero; source equals
  event ID; the reason is canonical; and support/population before/after values
  are unchanged.
- `HST_TownInfluenceEventState` now carries an optional exact aggression target,
  requested delta, and aggression before/after values. Admission requires one
  unique enemy faction pool, bounded arithmetic headroom, injected economy and
  strategic services, and no existing strategic source claim. An aggression-
  carrying event is accepted only with its unique applied `town_influence`
  strategic receipt targeting the same faction, zone, source event, timestamp,
  and delta. Exact replay returns the existing event/receipt and cannot apply
  aggression twice.
- Pedestrian panic remains behavior-ready rather than consuming the ordinary
  stuck-recovery budget. The native adapter reads `EAIThreatState` or the exact
  locality danger envelope, clears the wander helpers, creates one stock move
  waypoint away from the threat, and applies `EMovementType.RUN`. When the panic
  deadline expires it enters bounded recovery, restores deterministic wander
  helpers at `EMovementType.WALK`, and returns to `Wandering` only after native
  waypoint acknowledgement. Panic waypoint loss or movement stall consumes a
  separate bounded panic-route recovery counter; renewed danger during recovery
  re-enters the same panic episode safely. AI activation remains a construction-
  time operation and is not repeated by the panic/recovery hot path. Traffic
  panic is outside this first slice.
- Minor-locality exact fingerprints are bounded but session-only. They reject a
  conflicting event-ID reuse during one process and permit panic/episode state
  without political mutation, but the fingerprint map itself is not persisted.
  Therefore exact replay/conflict identity for a minor locality across process
  restart remains an explicit limitation and an open certification gate.
- The scoped Campaign Debug Phase 20 population helper builds the same complete global plan as
  production and selects the requested town's resulting allocation. It uses the
  same four-root transaction cap; Campaign Debug cannot silently give one town
  an isolated full-demand budget that exceeds the configured global limits.
- Ambient reconciliation is persistence-clean unless it observes a durable
  player claim. Ordinary movement, readiness, retry, and cleanup no longer make
  `UpdatePhysicalTownPopulation()` report a campaign-state mutation, preventing
  that five-second transient pass from repeatedly scheduling persistence. This
  is a source-level stutter repair, not profiling evidence.
- The coordinator performs player-first ambient vehicle observation before the
  persistence tick on every server frame, avoiding a full ambient-root occupancy
  scan. It promotes only a
  live tracked ambient root with a live controlled occupant, and rejects dead
  occupants or destroyed vehicles. Every `HST_PersistenceService` capture or
  checkpoint path repeats the observation synchronously, and new-campaign reset
  runs the same reconciliation before state replacement; missing civilian
  authority or an inexact
  promoted binding defers capture instead of writing stale or transient state.
- `HST_PersistentFieldVehicleRuntimeService`: owns the session-only exact live-
  root-to-durable-ID bindings shared by ambient promotion, field-vehicle
  restore/adoption, and garage redeploy. Saved durable IDs are never replaced by
  process-local replication IDs during restore or later interaction. Restore and
  registration precede first-frame player claim observation. Registered bindings
  resolve exactly by entity or runtime ID; initial recovery may use only a unique same-prefab root
  within eight meters and fails closed on ambiguity. Every pre-capture pass
  refreshes current transform, destruction, and linked cargo position; a
  vanished or destroyed tracked root is marked deleted. New-campaign reset
  reconciles first, then can carry forward occupied live tracked `loot_vehicle`,
  `field_vehicle`, or `garage_redeploy` roots, normalizes retained rows to
  `field_vehicle`, and copies their vehicle/cargo rows before replacing state.
  As the sole live durable-root binding owner, the tracker deletes every other
  bound old-campaign root once.
- Garage redeploy creates a fresh campaign-stable runtime ID, inserts and tracks
  the root before committing stored-row removal or payment, and rolls back the
  live root, runtime/cargo rows, tracker binding, and stored row on any later
  failure. Loot, garage, deletion verification, and undercover vehicle policy
  resolve the exact tracker binding before any spatial fallback.
- Per-frame horn suppression iterates the bounded traffic actor records and their
  direct driver handles. It does not perform a helper-by-root linear search for
  every helper at the global population caps.
- `HST_AmbientVehicleSaveValidationService`: proves the intended persistence
  seam in memory. Unclaimed `CIV_TRAFFIC_VEHICLE`, `CIV_VEHICLE`, and
  `MILITARY_VEHICLE` rows and linked cargo are excluded from capture/restore.
  Observed player occupancy promotes a vehicle to `field_vehicle`; movement
  distance alone never claims it. A legacy live detached ambient claim migrates
  to `field_vehicle`, while deleted or unclaimed legacy ambience is pruned.
  The source-level per-frame observation and fail-closed capture barrier are not
  native evidence. Brief enter/exit timing, autosave/process restart, promoted-
  root destruction, new-campaign reset, and loss/duplication behavior remain
  unproven.
- `HST_AmbientPopulationBudgetProofService`,
  `HST_AmbientActorRuntimeProofService`, the Schema-24 settings migration proof,
  and the ambient save-boundary proof cover deterministic allocation, ten-town
  bounded demand, driver accounting, lease rotation, strict lifecycle edges,
  bounded recovery, settings defaults/bounds, and transient-versus-claimed save
  semantics. They are compiled and wired into Campaign Debug but have not been
  executed there. A native ten-town/ten-minute soak, real save/restart, and the
  Campaign Debug Phase 20 production-path execution, brief enter/exit,
  autosave/restart, destruction/reset, and Blueprint Phase 8 automatic
  casualty/theft/nearby-combat influence and panic/recovery behavior remain
  separate open gates. Commander aid and ownership/security-pressure paths need
  runtime proof. The later Schema-66 exact local-security slice is not certified
  by this older ambient evidence.
- `HST_LocalSecurityCatalogService`: resolves only the configured authored town-
  police group for the exact enemy faction, validates its executable root and
  ordered member prefabs, and freezes a police-strength-plus-one roster clamped
  to 2–5 members. It does not add a generic ForceCatalog entry or provide a
  resistance fallback.
- `HST_LocalSecurityOperationService`: owns one deterministic enemy-town patrol
  epoch. Positive police pressure admits at most one held exact graph per town;
  active-zone projection uses the common exact infantry adapter and process-
  local cyclic waypoints. Leaving the active bubble exhaustively reconciles
  casualties, retires runtime ownership, and returns survivors to strategic
  hold. Restore always clears process handles and re-enters that held state.
  Complete destruction writes one exact police `-1` event and blocks same-epoch
  resurrection; only a newer ownership revision or a later positive police
  event can open the next epoch. Ownership/pressure clear, spawn failure, setup,
  and campaign terminal paths settle without political loss. Its save validator
  isolates every strong claimant before generic normalization and quarantines a
  malformed current graph at `-66`.
- `HST_StableIdService`: allocates persisted, monotonic IDs for generated
  commands and events, and builds deterministic operation/transaction links.
- `HST_CampaignCommandService`: validates typed command envelopes and keeps a
  bounded persisted receipt history so replaying the same request ID cannot
  apply the same mutation twice. `CompleteExplicit` records an authoritative
  applied/rejected status supplied by a migrated command result; `Complete`
  remains a presentation-text compatibility classifier for unmigrated visible
  commands.
- `HST_ResourceLedgerService`: records reserve, commit, cancel, and refund
  transitions for resource mutations. Troop training, exact garrison
  confirmation, and exact player-QRF confirmation/settlement consume it.
- `HST_CampaignEventLogService`: appends bounded persisted campaign events for
  authority decisions and resource transitions.
- `HST_ForceCatalogService`: owns the versioned exact group templates and
  validates their ordered execution-prefab slots.
- `HST_ForcePlanningService` and `HST_ForcePlanningIntegrityService`: issue and
  reconcile immutable garrison/player-QRF/player-Search-and-Destroy quotes and
  manifests plus exact enemy defensive-QRF manifests, own
  deterministic hashing/catalog validation shared by queue admission, and keep
  accepted confirmation replay idempotent across later settlement.
- `HST_ForceSettlementArchiveService`: compacts only terminal, backlink-free
  accepted planning aggregates into bounded persisted replay tombstones. It owns
  full-row retention, archive capacity, replay reconstruction, and fail-closed
  admission when protected history cannot make room. For canonical exact-QRF
  operations, compaction additionally requires a coherently settled record,
  copies its typed terminal/revision/settlement evidence, and removes the full
  record with the other accepted aggregate rows. Expired exact-support capacity
  eviction treats the tombstone and retained terminal request as one pair. Every
  positive typed player-support contract, including exact QRF and Search-and-
  Destroy, must pass valid replay shape, unique aggregate identity, no live
  backlink, and complete terminal-receipt reciprocity before both rows leave.
  Historical contract-0 QRF retains its minimal compatibility match. Malformed
  or quarantined typed pairs remain durable evidence while another eligible row
  may be pruned.
- `HST_OperationService`: owns the version-1 canonical operation transition
  contracts for confirmed exact paid player QRFs, confirmed exact player Search-
  and-Destroy, and newly admitted exact enemy defensive-QRF orders, and supplies
  shared deterministic settlement identity
  to the schema-52 mission-convoy owner. For the QRF consumers it preserves immutable
  origin/assignment separately from the mutable tactical target; validates
  duty, materialization, engagement, return, and settlement transitions; and
  rejects identity or terminal replay conflicts. Schema 50 added strategic-
  route, projection-decision, and virtual-combat authority for the player QRF.
  Schema 60 adds a separately typed Search-and-Destroy assignment/recall/
  settlement policy while reusing that exact infantry projection boundary.
  Schema 51 adds the reciprocal enemy-order contract, two-sample physical
  arrival authority, return-to-origin duty, and typed `COMPLETED` settlement for
  the enemy defensive QRF without opting legacy rows or other order families in.
- `HST_StrategicMovementService`: owns the schema-50 direct-route cursor and
  conservative 2.5 m/s campaign movement shared by the two exact infantry-QRF
  consumers and Schema-60 player Search-and-Destroy. It derives ETA from route
  distance, advances only
  while strategic position authority is active, and caps catch-up at 30
  campaign seconds per invocation.
- `HST_MaterializationService`: evaluates either supported exact infantry QRF
  or exact player Search-and-Destroy
  against a player-bubble materialize-in distance and a larger materialize-out
  distance. The hysteresis band prevents churn; an engaged physical projection
  cannot fold.
- `HST_VirtualCombatService`: resolves only an on-station exact paid player
  infantry QRF or Search-and-Destroy operation against hostile abstract infantry
  in its target garrison. It advances in
  deterministic 30-second power steps, processes at most four steps per tick,
  retires exact manifest member slots, and persists fractional damage carries.
- `HST_OperationProjectionProofService`: adds focused movement, hysteresis,
  roster-transfer, and combat/restore assertions beside the existing eight
  operation-record assertions. These are source-level debug fixtures until a
  packaged runtime executes them.
- `HST_EnemyQRFOperationService`: owns schema 51 admission and runtime authority
  for newly planned infantry-only enemy defensive QRFs. It selects a distinct
  same-faction operational source for a same-faction defended target under resistance pressure, links one prepaid
  frozen manifest to one order, operation, held SpawnQueue batch, and active
  group; advances virtual/physical outbound and return legs; applies defensive
  capture-pressure reduction once on arrival; and settles survivor-proportional
  enemy resources once after return. Terminal settlement persists `PREPARED`,
  stages the full unapplied tuple, validates debit/claimant/survivor authority,
  applies or replays the refund, publishes the applied receipt, and finalizes
  the operation/order tail in that order. Missing or conflicting authority
  fails closed instead of falling back to the legacy timer/support path.
- `HST_EnemyQRFSaveValidationService`: supplies the static, read-only schema-
  neutral predicates for complete aggregate and resource chronology validation.
  `HST_CampaignSaveData` captures current row provenance, revalidates prepared
  and settled authority after Schema-67 normalization, disarms malformed rows,
  cancels/strategically holds the reciprocal batch, retains the group while
  skipping generic projection revival, and preserves the stable status through
  Schema-51 normalization.
  `HST_EnemyQRFOperationService.ReconcileAfterRestore()` publishes and
  stabilizes `exact_restore_resource_authority_quarantined` while leaving those
  retained claimant rows untouched. Mutationless historical settlement remains
  outside current receipt provenance.
- `HST_EnemyQRFOperationProofService`: contributes six focused
  `enemy_qrf.*` debug assertions for atomic admission, legacy isolation,
  projection transfer, survivor settlement, current-schema restore, and
  rejection/refund behavior. Its persistence assertion includes the six
  committed and three uncommitted prepared crash-cut fixtures, repeated
  recovery no-ops, corruption/tamper guards, settled pool-tail quarantine, and
  mutationless-history compatibility. They are deterministic in-process
  capture/restore fixtures, not packaged-runtime or real process-restart
  evidence.
- `HST_MissionConvoyOperationService`: owns schema 52 admission and runtime
  authority for newly started convoy missions. It freezes one persisted
  generated road route, exactly three vehicle slots, exactly three linked crew
  groups and their member slots, plus mission-kind-compatible cargo/captive
  authority assigned to vehicle zero. It advances the column virtually at 9 m/s,
  materializes
  within 1,800 m of a living player, folds beyond 2,200 m only when clear of
  contact, player occupancy, and unresolved interaction state, preserves exact
  crew casualties by frozen slot/entity identity and per-element vehicle state,
  confirms physical arrival
  from two distinct route-end samples, and settles only after the existing
  mission-convoy outcome owner has applied its once-only result. Historical
  restored convoy rows remain contract version `0` on the legacy timer/runtime
  path. Crew elimination can place unresolved money/prisoner/ammo/armored or
  other recoverable mission assets into an on-station recovery hold, keeping
  the exact operation open and reprojectable until the mission-specific outcome
  is actually resolved.
- `HST_MissionConvoyP1Policy`: supplies the small shared fail-closed policy
  surface for exact-convoy admission and restore. It enforces mission-specific
  cargo cardinality, role/kind, and loadable mission-asset entity requirements.
  Captives must be boardable characters with compartment access, while ordinary
  payloads must be non-character entities. The policy also owns
  the legal duty/resume and settlement/materialization/position pairs.
- `HST_MissionConvoySaveValidationService`: owns conservative schema-52
  migration, current-schema claimant validation, quarantine, derived-survivor
  normalization, and process-local restore cleanup. `HST_CampaignSaveData`
  delegates this boundary instead of carrying one compiler-heavy validator.
- `HST_MissionConvoyOperationProofService`: contributes nine focused
  `mission_convoy.*` assertions for atomic admission and rollback,
  virtual/projection and fold gating, casualty-stable restore, idempotent
  arrival/settlement, open/settled/recovery restore, aggregate-marker cleanup,
  and the materialization watchdog. Admission/corruption subfixtures reject
  invalid cargo, duplicate/hash/foreign authority, invalid seat topology,
  forged arrival receipts, illegal lifecycle pairs, and casualty authority on
  non-member roots, and preserve missionless exact-looking durable claimants.
  They are deterministic in-process fixtures, not packaged movement, rendering,
  or restart evidence.
- `HST_EnemyPatrolOperationService` and
  `HST_EnemyPatrolSaveValidationService`: own schema-53 exact authority for only
  newly queued enemy patrols, including proactive debit/settlement, generated-
  route outbound/lap/return state, exact projection transfer, legacy dispatch
  isolation, and `-53` quarantine.
- `HST_GarrisonPatrolOperationService`: owns schema-54 authority only for newly
  issued policy-v2 purchased resistance garrisons. It admits one exact arbitrary
  member roster under a non-self-populating group root, holds it as a virtual
  infinite local patrol, transfers only living slots through materialization/
  fold, and settles owner-change/all-dead/campaign-stop/setup or typed spawn/
  route-failure outcomes once without a refund. It never claims historical
  policy-v1, initial-map, enemy aggregate,
  vehicle, or multi-root garrisons.
- `HST_GarrisonPatrolSaveValidationService`: preserves every pre-schema-54
  garrison on its legacy representation, normalizes coherent current exact
  patrols to held survivor authority, and retains malformed current graphs under
  quarantine version `-54` without legacy conversion, refund, or guessed death.
- `HST_GarrisonPatrolOperationProofService`: contributes nine focused
  `garrison_patrol.*` assertions for admission, replay/rollback, roster
  projection, infinite route loop, projection/casualty hold, no-refund terminal
  settlement, restore, corruption quarantine, and marker lifecycle. They are
  deterministic source fixtures rather than packaged native behavior proof.
- `HST_MissionGuardOperationService`: owns schema-55 officer-guard, schema-56
  traitor-guard, and schema-57 spec-ops-guard authority. A newly started `assassinate_officer` mission uses
  contract `1`/policy `exact_assassinate_officer_guard_v1`; a newly started
  `assassinate_traitor` mission uses contract `2`/policy
  `exact_assassinate_traitor_guard_v1`; and a newly started
  `assassinate_specops` mission uses contract `3`/policy
  `exact_assassinate_specops_guard_v1` and intent `assassinate_specops_guard`.
  Generic spec-ops composition may propose multiple groups; contract `3`
  deterministically selects the strongest executable group, keeps the stable
  first group on ties, and freezes only that selected catalog roster. Discarded
  groups never become mission, operation, manifest, batch, or active-group
  authority.
  All three freeze one catalog-backed empty
  infantry root and ordered members, keep the HVT outside the manifest and
  operation asset graph, hold a route-less roster on station, transfer only
  durable survivors through materialization/fold, and map guard elimination or
  mission/owner/runtime outcomes to one zero-refund terminal receipt. Historical
  and pre-opt-in assassination missions and all unsupported families remain
  contract `0`.
- `HST_AssassinationGuardSaveValidationService`: preserves pre-opt-in mission and
  HVT history at contract `0`, validates either supported exact mission/
  operation/manifest/batch/group graph, accepts compact settled authority,
  normalizes coherent physical state to held survivors, and retains malformed
  officer/traitor/spec-ops rows under `-55`/`-56`/`-57` quarantine without legacy fallback,
  guessed casualty, HVT ownership, or refund. Schema-56 migration creates no
  traitor authority and records `migration_schema56_exact_traitor_guard`;
  current traitor conflicts record
  `normalization_schema56_exact_traitor_guard_conflict`. Quarantine remains
  diagnostic and does not terminate the otherwise playable HVT mission.
  Schema-57 migration likewise creates no spec-ops authority and records
  `migration_schema57_exact_specops_guard`; current spec-ops conflicts record
  `normalization_schema57_exact_specops_guard_conflict`. Ordinary
  `mission_group_*` rows are not exact claimants.
- `HST_MissionGuardOperationProofService`: contributes focused
  `mission_guard.*` source assertions for admission/legacy isolation, survivor
  projection, HVT-independent settlement, migration/restore, corruption
  quarantine, and existing-HVT marker/UI status. They are not native entity,
  adapter, save/restart, rendered UI, owner-change, campaign-setup, or packaged
  multiplayer evidence.
- `HST_TraitorGuardOperationProofService`: contributes the same six focused
  source-proof categories for contract-2 traitor admission/isolation, survivor
  projection and HVT separation, typed settlement, restore/migration,
  corruption quarantine, and existing-HVT marker/UI status. It also proves the
  contract-1 officer path coexists and pre-56 traitor/spec-ops rows remain
  contract `0`. Native entities, real adapter casualties, actual save/restart,
  rendered UI, owner-change, setup, packaged networking, reconnect, and JIP are
  explicitly unclaimed.
- `HST_SpecOpsGuardOperationProofService`: contributes the same six focused
  source-proof categories for contract-3 spec-ops admission/family isolation,
  survivor projection and HVT separation, typed zero-refund settlement,
  restore/pre-57 migration, `-57` corruption quarantine, and existing-HVT
  marker/UI status. It preserves officer contract `1`, traitor contract `2`,
  and historical spec-ops contract `0`. Schema 57 is stamped at implementation
  `514ebdcbeb1ddfb2a383b19590382517113e2ff6`; native/package/restart/render/
  network gates remain explicitly open.
- `HST_RescuePOWOperationService`: owns Schema-58 contract-1 authority only for
  newly started `rescue_pows`. One mission-rescue operation binds one frozen
  composite manifest, one hostile catalog guard roster, one external-asset
  SpawnQueue batch, and exactly three typed captive rows. It owns captive
  transition/idempotency through a bounded typed command ledger, stable escort/
  carrier/seat evidence, disconnect release before grace, unbound fold/re-entry,
  damage-only casualty receipts, one frozen HQ extraction anchor, base-deadline
  plus 300-second custody-only grace, and zero-resource typed settlement. It
  releases surviving custody links before non-success settlement so historical
  rows do not depend on mutable carrier vehicles. Guard elimination and captive
  outcome authority stay independent.
- `HST_RescuePOWExternalAssetPolicy`: is the sole permission boundary for
  external manifest asset rows. Queue admission/replay/batch validation requires
  the frozen hash-valid rescue policy/intent/kind, one deterministic guard root
  and ordered members, no vehicles, and exactly three deterministic captive
  slots. Adapter execution additionally requires a unique reciprocal active
  contract-1 mission-rescue operation, batch, group, and all three typed captive
  rows before it excludes those asset descriptors from generic work.
- `HST_RescuePOWExternalAssetPolicy`: is the shared fail-closed exemption gate
  for Schema-58 captive manifest descriptors. Queue admission, replay, and
  durable-batch validation require the exact frozen/hash-valid shape; adapter
  execution additionally requires a unique reciprocal active mission-rescue
  contract-1 graph and exactly three matching durable captive rows before the
  generic adapter may omit those descriptors. External rescue root registration
  preserves the rescue mode discriminator used by runtime and restore checks.
- `HST_RescuePOWSaveValidationService`: preserves historical POWs and refugees
  at contract `0`, validates reciprocal site/mission/operation/manifest/batch/
  group/captive authority, clears process bindings without erasing stable
  escort/carrier/seat receipts, accepts compact terminal graphs, and quarantines
  malformed current or strong pre-58 claimants at `-58`. It records
  `migration_schema58_exact_rescue_pows` and
  `normalization_schema58_exact_rescue_pows_conflict` without inventing a
  captive, casualty, extraction, reward, fallback, or force transfer.
- `HST_RescuePOWOperationProofService`: contributes six deterministic source
  categories for admission isolation, composite/external authority, captive
  transitions (including old accepted/rejected replay, request collisions,
  terminal reserved capacity, and deep-copy preservation), guard independence,
  hitch-stable outcome/grace and disconnect policy, and restore/quarantine. It
  does not claim native entities, natural combat, vehicle seats, real process
  restart, rendered UI, networking, reconnect, or JIP.
- `HST_RadioSiteLifecycleService`: owns the Schema-59 contract-1 radio-site
  aggregate for every configured radio zone. One stable site/zone/target
  identity owns transmitter binding, ONLINE/DESTROYED/REBUILDING lifecycle,
  the active exact destroy-or-stop-rebuild mission lock, typed transition
  request, revision, and destruction/rebuild receipts. A uniquely discovered
  supported authored transmitter is borrowed and never deleted. Its immutable
  authored prefab/position provenance remains separate from the mutable current
  projection descriptor across generated-ownership handoff; each exact mission
  asset snapshots that ownership and authored provenance at admission. Its
  retained multiphase damage object is the physical evidence source, and no initial
  generated fallback is invented when binding is absent or ambiguous. After a
  completed rebuild, the campaign owns one generated replacement while any
  authored resurrection is suppressed. Permanent generated ONLINE projections
  disable verbose witness logging and keep nearby-entity scans dormant until an
  exact mission identity is configured. The service alone creates or retires
  radio projections, so generic mission runtime and zone composition cannot
  place a second tower beside the canonical target. A new-campaign reset must
  reacquire and restore the authored transmitter before the state swap or reject
  the reset without abandoning the old campaign. Physical destroy/heal/rollback
  writes are verified. Authored identity uses a 0.75-meter tolerance, while
  physical projection evidence allows the bounded 12-meter safe-ground offset.
  A streamed-out borrowed target moves reciprocal mission/asset/runtime flags to
  an explicit pending phase without manufacturing destruction. Schema 60
  amortizes initial authored-world discovery to one unresolved site per campaign
  tick; the post-mission pass still reconciles every already-bound or campaign-
  generated projection immediately.
- `HST_RadioSiteSaveValidationService`: conservatively creates only logical
  ONLINE/unresolved sites for pre-59 saves, leaves historical terminal radio
  missions at contract `0`, fails active legacy radio claims closed, validates
  exact reciprocal mission/asset/site authority, distinct stable-site versus
  unique per-mission physical runtime identities, frozen physical bindings,
  persisted bounded demolition-evidence keys, mission-time ownership snapshots,
  and receipt/status/lifecycle evidence. Generated ONLINE state additionally
  requires destruction followed by completed-rebuild provenance. It quarantines
  contradictory current strong claimants at `-59` without inventing target
  binding, physical destruction, rebuild completion, receipts, or rewards.
  Current corrupt aggregates fail and clean up together; coherent already-
  terminal historical outcomes keep their terminal meaning.
- `HST_RadioSiteLifecycleProofService`: contributes six `radio_site.*`
  source assertions for binding/admission isolation, all lifecycle outcomes,
  replay/stale revision behavior, restore/migration/quarantine, influence
  suppression, and borrowed/generated ownership with construction-equipment
  targeting. The harness calls the production admission, durable evidence, and
  outcome transitions, rejects direct repeat rebuild admission, checks linked
  quarantine cleanup, and replaces only physical projection seams. Native
  authored discovery, natural explosive damage, damage-state reapplication,
  generated replacement,
  streaming re-entry, real save/restart, rendered UI, networking, reconnect,
  and JIP remain packaged-runtime gates.
- `HST_PlayerSearchDestroySaveValidationService`: owns Schema-60 restore
  isolation for player Search-and-Destroy. Pre-60 requests remain contract `0`
  with no inferred quote, manifest, ledger, operation, batch, group, casualty,
  or refund. A current contract-1 claimant must restore as one reciprocal exact
  graph; malformed strong claimants retain diagnostic evidence under `-60`,
  terminalize their runtime projection, and cannot fall through to legacy
  execution or alter balances by inference. Global active-group classification
  treats their quarantine status/mode as non-operational and not combat-present.
- `HST_PlayerSearchDestroyOperationProofService`: exercises the production quote
  and confirmation replay, infantry-only roster/cost, direct route, exact
  virtual-combat casualties, materialization/fold/re-entry, displaced-fold
  return to immutable assignment, commander recall/ledger settlement, legacy
  isolation, malformed-current quarantine, valid archive-pair capacity prune and
  restore, and corrupt quarantine retention. The service is compiled and wired
  into Campaign Debug, but its Schema-60 assertions have not been executed there.
  Their in-memory seams are not native movement/combat, packaged server, or real
  restart evidence.
- `HST_OperationRecordProofService` now also retains a typed exact-QRF archive
  pair whose receipt fields mismatch under forced capacity. That assertion is
  compiled and wired into Campaign Debug but has not been executed; contract-0
  QRF remains on its historical minimal-compatibility path.
- `HST_MaidensBayLocationSaveValidationService`: keeps
  `resource_logistics_warehouse` as the only enumerable strategic location,
  returns without mutation when neither location row exists, and fails closed
  before rewrites when canonical authority is ambiguous. With one authority it
  retires duplicate town civilian/influence/marker and ordinary aggregate-
  garrison state without fold-back credit, preserves the authoritative owner/
  economy, canonicalizes mutable generic references, and leaves all nonzero
  typed claimants frozen, including settled/quarantined/malformed and graphless
  exact rows. Mutable generated content rekeys; frozen site/route identity stays
  unchanged and receives a deep canonical clone. General lookup resolves the
  warehouse, while exact validators may use a detached old-ID/old-position view
  and runtime boundaries compare old/canonical IDs as equivalent. The retired
  town never becomes an income, marker, civilian, or new targeting source.
- `HST_MaidensBayLocationMigrationProofService`: builds isolated both-location
  and legacy-only saves, runs the production normalizer twice, and checks
  retirement, generic-reference rewrites, typed-graph isolation, ledger and
  generated-content results, idempotency, and compatibility lookup. It is
  compiled and wired to Campaign Debug assertion
  `location_taxonomy.maidens_bay_schema60`, but that assertion has not been
  runtime-executed and is not packaged save/restart evidence.
- `HST_OwnershipTransitionService`: owns every post-bootstrap location-owner
  mutation in Schema 62. It resolves supported zone aliases to the canonical zone
  identity before building the immutable request fingerprint or checking replay.
  The durable receipt validates expected owner/revision and reciprocal open
  operation authority for every accepted exact patrol manifest, freezes
  linked-town support and retaliation decisions, and advances the zone revision
  exactly once. Its checklist settles old exact patrol/aggregate security,
  hostile runtime, new-owner security, support, owner, town policy, generated
  sites and facility/logistics derivation, enemy consequences, economy/outcome,
  ownership-specific strategic/campaign events, marker/client projection,
  notification, and
  persistence scheduling. Retry-capable work precedes owner publication and
  exact authority is rechecked on pre-owner resume; an
  incomplete receipt remains the zone's sole active authority and resumes on a
  bounded tick or after restore. Military, mission, political, admin, debug, and
  migration callers all use this boundary.
- `HST_OwnershipTransitionSaveValidationService`: gives pre-62 zones contract
  version `1` and revision-1 baselines without inventing historical receipts or
  replaying effects. Current Schema-62 restore validates request identity,
  fingerprint/cause-policy bounds, checklist order, safely representable exact-
  plus-one revisions, the exact sorted support-target set and ordered applied
  prefix, same-row deterministic ownership-support events/deltas, strategic/
  campaign event authority, counterattack/order and garrison correlations,
  bounded nonblank reasons, array-ordered queued top-level receipts, at most one
  owner-applied incomplete publisher, FIFO position of that
  publisher, parent-owned publication, and reciprocal zone active/latest
  backlinks. Serialized queues restore only while every later unresolved top-
  level follower is pristine; pre-owner status alone is insufficient. It also
  rejects forged projection children whose claimed parent is unrelated and
  duplicate completed claims for one zone/applied revision.
  Multiple owner-applied publishers or an owner-applied publisher behind an
  earlier unresolved top-level receipt are ambiguous. Contradictory authority is
  preserved under contract `-62` quarantine instead of being repaired from a
  marker or guessed rollback. Projection-parent validation repeats to a fixed
  point, so a parent quarantined later in the array propagates quarantine through
  every unreleased descendant independently of serialized row order.
- A frozen linked-town support event applies its exact influence fact once and
  immediately asks the canonical ownership service to reconcile the resulting
  threshold. An unresolved earlier top-level receipt is not a reason to suppress
  admission: the service either correlates the already represented intent or
  persists the exact later receipt as queued under FIFO ordering. The periodic
  fallback scan waits for queued work and admits only otherwise-unrepresented
  intent.
  Ownership retry and then town policy run before setup/terminal returns with a
  frozen-clock bypass; political repair there suppresses fresh retaliation and
  notification. Every other support-policy failure remains retry-visible to the
  parent.
- `HST_OwnershipTransitionProofService`: defines deterministic source fixtures
  for military capture, political flip in both directions, enemy recapture,
  identical replay, fingerprint conflict, stale precondition rejection,
  interrupted projection/restore resume, marker source revision, co-located
  location identity, nested linked-town publication, prior-snapshot fencing, all
  six cause routes, durable serialized intent, linked-support/queued-town
  collision recovery, malformed current-schema FIFO publisher rejection, staged
  full-marker rollback, resolver fail-close and unsafe-row purge, setup-without-
  markers history across activation, deterministic support set/prefix and
  derived receipt correlations, persistence-deadline re-arm, two-child atomic
  release, two save/restore boundaries with exactly-once political completion,
  non-patrol/orphan/late exact-security
  fail-closed behavior, baseline migration, corruption quarantine, and retention
  pins. Its projection harness calls the
  same logical marker-snapshot builder as production and omits only native marker
  publication. All 14 registered ownership assertions pass in the focused
  state-only engine case. Full Campaign Debug, native publication, packaged
  runtime, persistence/restart, client/network/JIP, and multiplayer remain open.
- `HST_MissionAssetComponent` publishes a minimal replicated rescue-action DTO
  (evaluated, exact contract, disposition, active, quarantine, revision).
  `HST_MissionCaptiveActionPolicy` fails closed while that classification is
  pending, then exposes only the legal action; unchanged values do not create a
  replication bump. The server command/transition owner remains authoritative.
- `HST_ForceSpawnQueueService`: owns schema-44 durable per-projection spawn
  batches, bounded priority/FIFO work acquisition, verified callbacks,
  retry/deadline/cancellation cleanup, pin-aware terminal retention, reporting,
  dependency-ordered cleanup, a durable nonterminal `READY_FOR_HANDOFF` state,
  explicit post-handoff completion, once-per-actual-restore reconciliation, and
  schema-50/51 strategic holds plus Schema-60 player Search-and-Destroy, the
  schema-52 convoy, schema-54 garrison-patrol, schema-55/56/57 mission-guard
  roster holds, and the executable guard subgraph of Schema-58 rescue. A rescue
  batch explicitly excludes its three
  externally managed captive asset slots from queue result rows only after the
  shared exact external-asset policy validates their complete frozen shape. A held
  batch performs no generic queue work and
  uses its frozen member slots as exact living/dead roster authority.
- `HST_ForceSpawnAdapterService`: consumes eligible released queue work from the production
  one-second active-campaign coordinator tick. The first engine-facing slice
  creates exactly one infantry `SCR_AIGroup` root plus all frozen member slots,
  returns exact prefab/liveness/faction/native-group/Game Master/projection
  evidence, finalizes ready projections in physical war, and only then asks the
  queue to record success. For successfully handed-off exact infantry, it also
  maps authoritative life state back to the exact member slot, detaches confirmed
  dead members without deleting their corpses, and drives last-death cleanup.
  Its current-infantry validation now uses durable living slots after first
  handoff, allowing a schema-54 partial garrison survivor roster to rematerialize
  without restoring or requiring the original purchased count.

The remaining domain services are:

- `HST_EconomyService`: HR, faction money, support, aggression, war level,
  strategic score/control percentage, pacing recommendations, victory readiness,
  and balance diagnostics.
- `HST_MissionService`: mission eligibility, activation, deadlines, and
  completion rewards.
- `HST_PersistenceService`: campaign save-data migration/tracking, native
  Reforger checkpoint requests, typed automatic/manual/shutdown intent,
  independent periodic-autosave and first-edge major-change debounce clocks,
  scheduler-origin receipts, and profile JSON fallback saves. Accepted full-
  state checkpoints cover both scheduler lanes. Rejected major-change requests
  leave the periodic clock intact, rejected periodic requests use debounce
  backoff, and in-flight requests suppress competitors without freezing either
  clock. Native-active requests mirror their staged fallback only after the
  native post-commit completion callback succeeds;
  native-unavailable requests save the fallback synchronously. Every real
  capture first reconciles mapped physical exact-convoy and
  currently cut-over exact-infantry members. Exact player support additionally
  requires reciprocal ownership, exact living root/member binding cardinality,
  and a refreshed live position. Ambiguous or incomplete authority defers the
  capture and retains its checkpoint intent for bounded retry without flushing
  stale state or requesting a savepoint.
- `HST_PersistenceSmokeTestService`: deterministic persistence fixture seeding
  and restore verification reports.
- `HST_ProfilePathService`: startup-time recursive migration of the complete
  retired profile tree into `$profile:Partisan`, byte-for-byte copy verification,
  canonical-conflict archival, verified source removal, and deepest-first empty-
  directory cleanup before settings/save/loadout consumers run.
- `HST_RuntimeSettingsService`: `$profile:Partisan/HST_Settings.json`
  load/create/schema migration and settings application to preset/balance data
  after profile-tree migration.
- `HST_AuthorizationService`: persistent members, guests, admins, and the
  first commander-vacancy policy.
- `HST_StrategicService`: strategic-event admission/completion, town support,
  Petros penalties, activation flags, population-first victory/loss evaluation,
  and durable campaign-end state/reporting. Schema-62 owner writes are delegated
  to `HST_OwnershipTransitionService`.
- `HST_HQService`: initial HQ selection, HQ movement, Petros/cache/arsenal/tent/spawn-point
  runtime objects, rebuilds, Petros-loss state, HQ knowledge, HQ threat scans,
  and Defend Petros diagnostics.
- `HST_ArsenalService`: item counts, unlock thresholds, finite/INF withdrawals,
  garage records, and vehicle redeploy.
- `HST_LootService`: area loot, vehicle cargo, garage vehicle capture, field
  vehicle snapshot/restore, and safe vehicle-root scanning.
- `HST_LoadoutEditorService`: saved loadouts, live loadout/storage nodes,
  candidate replacement, finite/INF cost ledgers, atomic apply/rollback, and
  issued-item accounting.
- `HST_BuildModeService`: dry-ground placement resolution for garage redeploys
  and HQ runtime-asset rebuilds.
- `HST_ConvoyOutcomeService`: mission-specific convoy arrival, capture,
  delivery, ammo-point, garage-handoff, and outcome de-dupe effects.
- `HST_CommandUIService`: procedural command-menu payloads, visible action
  routing, reports, tab/action gating, marker/UI audit coverage, command/report
  coverage checks, and failed-action text. Zone lists, control totals, tones,
  capture rows, and income resolve ownership through the marker service's
  retained published snapshot, so a retrying/unreleased transition cannot leak
  its raw domain owner into the menu.
- `HST_EnemyDirectorService`: scaled enemy attack/support resource income by
  owned zone value and war level, plus validated spending against separate
  attack and support pools.
- `HST_PlayerLifecycleService`: connected-player registration, deterministic
  Workbench identity fallback, personal money, and rank.
- `HST_TownService`: resistance income, HR income, and town support changes.
- `HST_GarrisonService`: legacy aggregate garrison creation/fold-back plus the
  schema-54 exact purchased-manifest backlink and living-slot capacity reader.
- `HST_RecruitmentService`: troop training and abstract garrison recruitment.
- `HST_ZoneCaptureService`: military and mission capture request construction,
  ownership-transition notifications, and result mapping. It no longer owns a
  separate owner/security/marker side-effect sequence.
- `HST_PlayerSpawnService`: custom FIA HQ spawn, Workbench identity fallback,
  native respawn requests, pending spawn tracking, and spawned-player records.
- `HST_PhysicalWarService`: player-proximity zone activation over the abstract
  garrison model, plus exact adapter registration/handoff and guards that keep
  legacy population, repair, survivor, route, patrol, and cleanup paths from
  duplicating queue-owned projections. For spawned support groups it owns live-
  member-centroid route state, two distinct-time-sample arrival/recall
  confirmation within 75m, direct current-to-target/exit chains, bounded route reissue, and
  transactional replacement of service-owned waypoint entities. Exact QRF
  handoff normalizes into `support_active` so this same route authority owns it.
  For non-queue-managed mixed personnel/vehicle groups it owns a separate
  personnel terminal predicate: vehicle health cannot substitute for living
  infantry after prior population evidence and population grace. Terminal
  cleanup zeros strategic strength, fails an unresolved linked QRF, unregisters
  combat ownership, and releases an intact vehicle as neutral salvage. An
  existing durable field/loot/garage record remains durable; otherwise the
  salvage record is session-only. HQ safety uses separate scopes: 900m remains
  the hostile operation-staging exclusion, but whole-location activation is
  blocked only inside the location capture footprint (at least 150m for legacy
  records). Zone composition uses its own 150m immediate-clearance guard. This
  split is implemented in source and awaits packaged behavior proof. For a
  schema-52 exact mission convoy, PhysicalWar is also the sole engine projection
  adapter: it realizes the durable mobile crewed elements plus any separated
  intact crewless asset roots during outbound travel and recovery hold, samples current
  position/survivors/damage/fuel back into those elements, and folds owned
  vehicle/group handles without inventing casualties or applying a mission
  outcome. Materializing roots remain unpublished until the complete projection
  is physical. Commit revalidates and publishes every vehicle, group, mapped
  member, and cargo participant atomically before closing the outbound
  transaction; terminal ground cargo can be the only required recovery root, and
  clean player-bound cargo no longer pins the column during delivery.
  For a schema-53 exact enemy patrol it also exposes the live-position, contact,
  exact-route restart, and bounded route-recovery evidence consumed by the patrol
  operation service. Exact patrol groups are excluded from legacy garrison-
  patrol, survivor-repair, route, fold, and cleanup ownership.
  For a schema-54 exact purchased-garrison patrol it supplies the corresponding
  live-position and persisted local-route restart boundary while excluding the
  group from legacy aggregate activation, composition, waypoint, population,
  survivor, fold, and cleanup owners. The Schema-60 performance repair keeps
  pure-vehicle groups on vehicle-only living-count authority, skips the convoy
  whole-group survivor pass when no active convoy exists, and does not repeat a
  full member-count scan merely because a runtime entity already exists. Normal
  success diagnostics build costly visual evidence only after their 30-second
  throttle admits the log. These changes target the observed once-per-second
  stall cadence and still require a packaged server comparison.
- `HST_GeneratedContentService`: generated Everon mission sites, roadblock,
  support, stash, crashsite, and route records derived from stable zone
  anchors.
- `HST_MissionObjectiveService`: rough mission objectives, campaign task state,
  and no-admin objective progress hooks for broad-alpha mission families.
- `HST_MissionRuntimeService`: physical MVP mission primitives, world-condition
  objective polling, runtime inspection, and cleanup state. For a schema-52
  exact convoy it plans the three vehicle assets and optional cargo/captive
  before admission, then defers travel, arrival, and cargo-position authority
  to the mission-convoy operation instead of regenerating missing assets or
  resolving arrival from its legacy timer.
- `HST_SupportRequestService`: stateful FIA/enemy support calls,
  ETA/status/cooldown reports, native-safe ground support activation, physical
  or abstract resolution records, the schema-46 exact paid-QRF bridge from
  accepted manifest to queue-owned projection, and schema-47 survivor restore,
  elimination, and linked ledger settlement policies. ETA is only the earliest
  physical-arrival observation; spawned requests do not become
  `physical_arrived` or complete recall until physical war confirms current
  living-member distance. Recall mutation returns `HST_SupportRecallResult`
  with explicit accepted/already-applied/state-changed/terminal flags,
  disposition, failure, request, and operation identity. Exact paired full
  refunds validate both linked transactions and their settlement eligibility
  before either refund begins. New confirmed exact paid infantry QRFs also opt
  into the canonical operation contract. In schema 50 they begin as held virtual
  projections, follow the strategic route, run the narrow on-station virtual-
  combat policy, materialize exact survivors near a player, and fold exact live
  survivors back to the held roster after players leave the larger radius while
  the operation is clear of contact. Queue admission, projection transfers,
  restore, recall, and settlement update the same operation/request/group/
  manifest/batch identities. Schema 60 applies the same exact projection owner
  only to newly confirmed player Search-and-Destroy, with a separate operation
  discriminator, assignment policy, and archive identity. It remains on station
  after hostile infantry clears until commander recall, and a physical fold
  away from its assignment first enters virtual return-to-assignment. Route
  initialization occurs while duty is still
  `STAGING`; `LinkOutboundVirtual` commits delivered strategic service by moving
  duty to `OUTBOUND`. A terminal failure before that commit fully refunds money
  and HR. A later no-handoff materialization failure retains money and refunds
  only the operation's last virtual-friendly count, including from
  `ON_STATION`, with replay-idempotent settlement. Restore also clears the linked
  request's physicalized flag and publishes a virtual runtime status so request,
  operation, and held batch cannot disagree about projection ownership.
- `HST_CivilianService`: town reputation/support, wanted heat,
  police/roadblock presence and scans, aid incidents, undercover eligibility,
  request/application, enforcement, compromise, clear-state records, and
  disposable pedestrian/traffic projections. Ambient actors receive dedicated
  CIV group roots that inherit the stock behavior/pathfinding/utility/
  replication stack; initial members attach through the engine's AI-composition
  path, and the service owns their group, driver, and waypoint cleanup. Current
  source classifies stock town-center locations separately from minor localities,
  permits civilian projection even when HQ safety suppresses hostile military
  activation, selects deterministic non-repeating concrete appearances, defaults
  true towns to five driven vehicles, limits the known woodland locality to two
  pedestrians, and clears horn input only on HST-owned ambient drivers. Schema
  62 also runs a five-second sweep over durable town support/heat thresholds.
  A political event that reaches its threshold can immediately admit a pristine
  pre-owner receipt behind an earlier publisher; the sweep waits for existing
  queued work and admits at most one otherwise-unrepresented threshold per pass.
  This keeps intent restart-visible without bypassing array-order execution.
  Schema
  60 additionally overrides the native wheeled-vehicle base AI horn timing and
  horn sound-power fields, covering inherited AI-driven wheeled vehicles beyond
  the ambient-service input path. These behavior corrections await a
  republished runtime test.
- `HST_EnemyCommanderService`: enemy resource spending into patrol, roadblock,
  QRF, counterattack, rebuild, support-call, and Petros attack orders with
  physical/abstract runtime state. Schema 51 routes only newly planned
  infantry-only defensive QRFs through `HST_EnemyQRFOperationService`; their
  exact runtime bypasses the fixed legacy resolve timer and legacy support-row
  physicalization. Schema 53 dispatches newly queued patrol orders by both type
  and contract version to `HST_EnemyPatrolOperationService`; their proactive
  attack debit, exact one-root roster, generated-route loop, physical/virtual
  transfer, return, and settlement bypass the legacy patrol timer. Historical
  patrols and every other order type keep contract-version-0 behavior.
- `HST_MapMarkerService`: native marker rebuild/publish behavior plus marker
  status/detail/audit reports for strategic zones, missions, objectives,
  Defend Petros, support, QRFs, HQ, and convoy state. Linked terminal-group
  state is checked before unresolved-QRF visibility, preventing a dead response
  from retaining a tactical marker. Schema-50 source labels static zones with
  location and owner and reports virtual exact player-QRF travel/on-station
  state. Schema 51 publishes one marker for every open exact enemy defensive-QRF
  operation at its strategic or live cursor with faction, living count, duty,
  immutable source/assignment, and ETA. Schema 52 publishes one aggregate
  marker for an open exact mission convoy at the operation's strategic/live
  position and suppresses the three misleading per-vehicle convoy markers.
  Recovery retains the same aggregate current and destination marker pair for
  cargo and vehicle outcomes; it never revives legacy per-vehicle/outcome IDs.
  Schema 53 publishes one roster-authoritative marker per open exact enemy patrol
  at its virtual cursor or live position, with duty and living-count context, and
  removes it after terminal cleanup.
  Schema 54 publishes one marker per open exact purchased-garrison patrol at its
  virtual/live position with assignment location, current owner, role, and
  durable survivor count. Forces UI reports exact patrol infantry separately
  from legacy aggregate infantry; neither presentation owns the roster. Schema
  61 makes this rebuild the source of a revisioned logical marker registry: an
  unchanged stable ID keeps its revision/sequence, create or content update
  advances both record revision and the global stream, and removal emits a
  bounded tombstone. Server-native campaign marker publication is retired;
  clients reconcile native campaign markers from the authoritative registry.
  Schema 62 adds a separate source revision to zone records. A marker's local
  revision still describes projection changes; `m_iSourceRevision` identifies
  the exact ownership revision from which its owner/label was built.
  Authored static markers bind through one cached exact
  `HST_ConflictMapMarker_<zoneId>` entity-name lookup; the prior periodic
  radius scan is not part of identity or refresh authority.
- `HST_MarkerProjectionCodec`: owns marker protocol version `2`, bounded string
  encoding/decoding, escaped fields, snapshot/delta headers, packet record and
  character limits, source ownership revision, and deterministic live-registry
  hashing. Protocol `2` keeps the Schema-61 delivery/ACK model but adds the
  source-revision field to every encoded marker row. It rejects malformed or
  oversized payloads before a client registry can mutate.
- `HST_ClientProjectionService`: owns the server's current marker registry,
  bounded ordered event journal, and one readiness/ACK session per connected
  player. It sends hashed chunked snapshots for first join, reconnect, late join,
  epoch/hash mismatch, unavailable history, or resync; otherwise it replays only
  a retained contiguous delta range. The slowest valid connected ACK constrains
  pruning.
- `HST_ClientMarkerProjectionService`: owns the widget-independent client
  registry and native-marker reconciliation. Snapshot chunks stage until the
  complete count and hash validate, then replace the registry atomically.
  Deltas must begin at the next global sequence and advance each marker's
  projection revision exactly once. Zone ownership publication additionally
  requires the record's source revision to match its zone authority. Old
  duplicates are acknowledged idempotently; a gap or
  invalid revision requests resync without partial mutation. Map open/reopen
  only reconciles widgets from this already-current registry. The Schema-66
  marker-integrity repair also runs from the readiness keepalive: it detects a
  missing or altered native campaign marker even when no stream revision changed
  and reconciles it from the committed registry.
- `HST_MarkerProjectionSaveValidationService`: rebuilds pre-61 derived marker
  rows without inventing campaign facts and advances the epoch before replacing
  malformed current projection state. `HST_MarkerProjectionProofService`
  defines fixtures for snapshot/JIP, ordered create/update/delete, duplicate replay,
  dropped-delta resync, reconnect, ACK pruning, malformed input, and migration
  idempotency; they remain unexecuted deterministic source fixtures.
- `HST_ZoneCompositionService`: runtime alpha composition slots for zone and
  mission physicalization diagnostics. Schema 59 removes radio transmitters
  from this generic owner's spawn and cleanup authority. The radio-site service
  binds exactly one authored target when uniquely discoverable, never deletes a
  borrowed authored entity, and creates a generated transmitter only after a
  rebuild actually completes; unresolved initial sites receive no fallback.

Static marker rendering has a separate client lifecycle boundary. Schema 61
keeps the authoritative marker registry alive whether or not the map widget is
open, then creates/updates/removes client-local native campaign markers from that
registry when the native map surface is available. A small manager patch retries
root creation before the stock static-marker update and disables a marker that
remains rootless. The rendered-map proof waits for the delayed client pass and
inspects actual active roots and widget components; protocol registry equality
and native handle/widget readiness are distinct proof gates. Protected campaign
markers are inserted without the stock local-only path that assigns the local
player as marker owner. Their native owner is `-1`, owner removal stays disabled
from the first visible frame, and integrity drift causes a registry-derived
rebuild. Dynamic/player-created markers remain on their existing replicated-
entity or player-marker path, remain editable, and are not records in the
Schema-61 stream.
Config-backed modded classes on this path must also repeat the base class's
container metadata. A packaged schema-49 test verified that restoring the
original `BaseContainerProps` and custom-title attributes brought normal Game
Master and stock HUD initialization back. That run then exposed eighteen invalid
radio icon entries as giant boxes. Current source preserves the canonical placed-
marker table, appends or repairs a validated normal/glow radio icon by resource
identity instead of a hard-coded index, and rejects invalid entries. The map-
target prompt, target indicator, and dialog now share a map-local z-order below
the native workspace pointer. Those follow-up fixes still need packaged proof.

Static helper services keep repeated low-level behavior out of the coordinator:
`HST_WorldPositionService` resolves dry/safe positions and prefab spawning,
`HST_DisplayNameService` normalizes item and vehicle labels,
`HST_VehicleRootPolicy` centralizes safe vehicle-root eligibility,
`HST_VehicleCapabilityPolicy` classifies captured/source vehicles as ammo,
repair, fuel, armed, or transport sources, and
`HST_ConvoyVehicleControlAdapter` wraps native vehicle movement/seating calls,
registers valid pilotable vehicles with the crew utility before seating,
prefers forced authority-local entry for server-owned AI, and exposes a direct
retained-registration query for runtime proof.

The coordinator currently exposes server-only mutation methods that check
campaign phase, known IDs, and mission eligibility before changing state.
Client UI requests flow through player-owned request/RPC components and
ownership-resolved coordinator methods, so the server resolves the trusted
identity and enforces member, commander, and admin permissions instead of
trusting client-provided player IDs.
The same player-owned bridge carries marker-projection readiness, contiguous
acknowledgements, resync requests, and reliable owner-targeted payloads. Server
session identity is derived from the owned controller/component; a client-
supplied player ID is diagnostic input only and cannot select another session.

## Campaign Authority Boundary

Schema 43 extends the first campaign-authority foundation with exact force
planning. Schema 44 adds durable bounded SpawnQueue authority for executable
manifest projections. Schema 45 adds explicit force/projection identity on
active groups, durable Game Master registration evidence, dependency-ordered
cleanup, and the first engine-facing exact infantry adapter. Schema 46 makes
player-paid resistance QRF the first support consumer of the complete quote →
ledger → executable manifest → SpawnQueue → settlement path. None of these
schemas is a claim that every existing broad-alpha mutation uses the boundary.
Schema 47 adds durable exact-member casualty state, ever-populated/spawn-
completed terminal semantics, corpse-preserving root cleanup, and survivor-only
paid-QRF reprojection. Schema 48 bounds accepted quote/manifest/ledger history,
preserves compact issue/confirmation/transaction replay, and adds the missing
production caller for pin-aware terminal SpawnQueue maintenance.
Schema 49 adds a canonical, versioned operation aggregate for newly confirmed
exact paid player infantry QRFs and uniquely coherent active schema-48 rows.
That narrow slice is not a claim that legacy QRFs, other support, missions,
garrisons, enemy orders, or every force projection use the aggregate.
Schema 50 adds persistent strategic projection to that same narrow consumer:
direct-route progress, held exact roster authority, proximity hysteresis,
physical-to-virtual survivor transfer, bounded virtual infantry combat, and
restore normalization to one virtual projection. It does not migrate vehicles,
assets, convoys, garrisons, broad legacy supports, missions, or enemy orders.
Schema 51 adds a second, separately typed consumer: newly planned infantry-only
enemy defensive QRF orders. Admission freezes one prepaid roster from a distinct
same-faction operational source for a same-faction defended target under resistance pressure, links one operation/
batch/group aggregate, and suppresses a
parallel legacy QRF or support row for the same target. The force travels and
folds through the shared exact projection boundary, applies defensive pressure
once at its immutable assignment, returns to its immutable origin, and refunds
attack/support resources in proportion to exact survivors once. Pre-schema-51
orders, counterattacks, patrols, roadblocks, support calls, Petros attacks,
vehicles, garrisons, missions, and other force families remain contract version
`0` on their prior paths.
Schema 52 adds a third typed operation consumer and the first narrow exact
vehicle/multi-group aggregate: only a newly started `convoy_intercept` mission
is admitted with contract version `1`. Its frozen manifest contains exactly
three vehicle slots, one crew group per vehicle, ordered durable crew slots, and
an optional cargo/captive asset assigned to vehicle zero. Three durable convoy
element rows own per-vehicle position, survivor count, damage/fuel/ammunition
snapshot, disposition, and reciprocal mission/operation/manifest/runtime links.
  The operation owns generated-route progress, virtual movement, proximity
  materialization/fold, arrival, restore normalization, and terminal receipt;
  existing mission-outcome code remains the once-only reward/penalty owner.
  When crew elimination leaves cargo/captive/vehicle recovery unresolved, the
  operation remains open in an on-station recovery hold and may virtualize and
  rematerialize those assets instead of settling early.
Pre-schema-52 convoy rows remain contract version `0`, receive no invented
manifest/operation/element identity, and continue on the legacy path.
Schema 53 adds a fourth typed operation consumer: only newly queued enemy
`PATROL` orders receive contract version `1`. Admission freezes one infantry
root and its member slots, spends proactive attack resources once, and links one
order/route/operation/manifest/held-batch/group aggregate. A generic generated-
route cursor owns outbound travel, exactly one closed on-station lap, and a
separate return-to-origin leg. Virtual and physical projection share the frozen
roster; mapped casualties survive fold and restore, while physical contact holds
the route clock until clear. Return settles one survivor-proportional proactive-
attack refund. Type-plus-version dispatch keeps this policy isolated from the
exact defensive-QRF and legacy owners. Corrupt current-schema rows retain
diagnostic authority under quarantine version `-53` and never fall back to a
legacy timer. Historical patrol rows remain contract version `0` and receive no
invented route, roster, operation, or refund. Packaged schema-50 through
schema-53 certification remains independently open.
Schema 54 adds a fifth typed operation consumer: only a newly issued policy-v2
purchased resistance garrison receives `HST_OPERATION_TYPE_GARRISON_PATROL`
contract version `1`. Its immutable manifest contains one executable
`NotSpawned` container root and the arbitrary ordered members selected by the
quote. The accepted graph remains in strategic hold and walks a persisted local
route loop indefinitely while virtual; proximity projects only living slots and
fold retains exact casualties/cursor state. PhysicalWar excludes the exact group
from legacy aggregate garrison owners. Owner change, all-dead, campaign stop,
setup, or typed spawn/route failure records one
`exact_garrison_patrol_terminal` receipt with zero resource
refund and zero transfer into aggregate infantry. Policy-v1 purchases and all
initial/enemy aggregate, vehicle, and multi-root garrisons remain legacy.
Pre-schema-54 migration invents none of this authority; malformed current rows
retain quarantine version `-54`. Packaged schema-50 through schema-54
certification remains independently open.
Schema 55 adds a sixth typed operation consumer: only guard infantry for a
newly started `assassinate_officer` mission receives
`HST_OPERATION_TYPE_MISSION_GUARD` contract version `1`. The HVT remains outside
the exact manifest. One empty execution root and ordered member roster own a
route-less, offset guard assignment through strategic hold, physical projection,
mapped casualties, fold, re-entry, and zero-refund typed settlement. Guard
elimination settles `DESTROYED` without completing the HVT objective; HVT
success maps to `COMPLETED`, mission failure/expiry/campaign stop/setup to
`CANCELLED`, owner change to `INVALIDATED`, and coherent spawn/assignment failure
to `SPAWN_FAILED`. The operation has no route or virtual-combat owner. Historical
officer missions, other assassination variants, and all remaining mission
families remain contract `0`; pre-55 migration invents nothing. Malformed current
rows quarantine at `-55` without a legacy fallback, HVT backlink, guessed death,
or refund, and the diagnostic quarantine leaves the HVT mission playable. The
existing HVT marker and mission UI project guard strength instead of adding
another marker. The stamped Schema-55 tree identifies implementation
`552c2c4ff5ac7608fa248c614480a254769b61a4`, passes the full foundation gate and
clean Workbench Game validation at 5,763 files/11,570 classes with CRC
`0ec8950e`, and survives a ten-sample/20-second normal WorldEditor open. Native
entity/adapter/casualty behavior, save/restart, rendered UI, owner-change,
campaign-setup, packaged networking, reconnect, and JIP proof remain open.
Schema 56 adds a seventh explicit family consumer without adding a seventh
operation enum: guard infantry for only a newly started `assassinate_traitor`
mission uses `HST_OPERATION_TYPE_MISSION_GUARD` contract version `2`, manifest
policy `exact_assassinate_traitor_guard_v1`, and quarantine version `-56`.
Schema-55 officer guards remain contract `1` with `-55` quarantine. The traitor
path deliberately reuses the same route-less empty-root/member roster, separate
HVT authority, survivor-only materialization/fold, no-virtual-combat policy,
zero-refund typed terminal mapping, compact settled restore, and existing-HVT
status projection. Pre-56 and historical traitor missions, `assassinate_specops`,
and all other mission families remain contract `0`. Pre-56 migration records
`migration_schema56_exact_traitor_guard` and invents no authority; malformed
current traitor graphs record
`normalization_schema56_exact_traitor_guard_conflict` and remain diagnostic
without fallback or HVT failure. Six focused source-proof categories cover the
new contract, but Schema-56 native entities/adapter casualties, real save/
restart, rendered UI, owner-change, campaign setup, packaged networking,
reconnect, and JIP are unclaimed. The stamped Schema-56 tree identifies
implementation `bab5748d817ba434dae701cfbb3b92805d463678`, build label
`schema56-exact-traitor-guard`, stamp
`03a65cd33bee69c6320389803cdd5a2ec8576fb0`, and passes the full foundation gate. Workbench
Game validation loaded 5,764 files/11,573 classes with CRC `a18c67a5` and
reported `Script validation successful`; its bounded hidden normal WorldEditor
open stayed alive for all ten samples over 20 seconds and the latest log had no
script-error/crash signature. These source/Workbench gates do not close any
packaged behavior obligation.
Schema 57 adds an eighth explicit family consumer without adding an operation
enum: only guards for a newly started `assassinate_specops` mission use the
mission-guard operation at contract `3`, policy
`exact_assassinate_specops_guard_v1`, intent `assassinate_specops_guard`, and
quarantine `-57`. Officer `1`/`-55` and traitor `2`/`-56` remain unchanged. The
spec-ops path reuses the route-less empty-root/member roster, HVT separation,
survivor-only materialize/fold/re-entry, no virtual combat, typed zero-refund
settlement, compact restore, and existing-HVT status. Historical/pre-57 spec-ops
missions and unsupported families stay contract `0`; ordinary
`mission_group_*` rows are not claimants. Migration/conflict events are
`migration_schema57_exact_specops_guard` and
`normalization_schema57_exact_specops_guard_conflict`. Schema 57 is stamped at
implementation `514ebdcbeb1ddfb2a383b19590382517113e2ff6` with build label
`schema57-exact-specops-guard`. The full foundation gate passes, including its
Schema-55/56/57 checks. Stamped Workbench Game validation loaded 5,765 files/
11,576 classes with CRC `e0b8578e` and `Script validation successful`; the
bounded hidden normal WorldEditor open stayed alive for 10/10 samples over 20
seconds, and its log had no script-error/crash signature. These source/Workbench
gates leave every packaged/native/restart/UI/network gate open. The
assassination-guard family is exhausted. Schema 58 is the later separate rescue
cutover for newly started `rescue_pows` only; historical POWs,
`rescue_refugees`, and further rescue families remain legacy until explicitly
versioned.

Schema 58 adds the ninth explicit family consumer and seventh operation type:
only newly started `rescue_pows` use mission-rescue contract `1`, policy
`exact_rescue_pows_v1`, intent `rescue_pows_guard`, and quarantine `-58`. One
hash-valid composite manifest owns a separately executed exact guard roster and
three policy-validated captive descriptors. Typed captive rows, the frozen HQ
anchor, command receipts, custody/grace rules, damage-only death, exact
extraction receipts, and mission-owned terminal mapping remain authoritative;
historical POWs, refugees, and other mission families stay contract `0`.

Schema 59 adds a separate exact strategic-site owner rather than another force
family. Each radio zone receives one durable site row. A destroy mission may
start only from a resolved ONLINE site and succeeds only after physical target
destruction. Borrowed authored-target destruction must match the reciprocal
active site/mission lock and revision, an authoritative DESTROYED damage manager,
and the frozen projection. Generated-target explosive scoring additionally needs
a live matching mission component, a bounded physical position, and a unique key
in the persisted bounded evidence set. Mission-time ownership/provenance prevents
later ownership handoff from relabeling that evidence. Success moves the site
to DESTROYED and suppresses its town influence. A stop-rebuild mission may start
only once for a given tower-destruction epoch, moves the site to REBUILDING on
admission, and targets construction equipment rather than a second transmitter.
Destroying that equipment records the rebuild attempt and returns the site to
DESTROYED without advancing the tower-destruction epoch; mission failure,
expiry, or campaign stop completes the rebuild and returns one campaign-
generated transmitter to ONLINE. The stable site target ID never doubles as a
mission projection handle: every mission owns a unique physical runtime-entity
ID. Every accepted admission or outcome uses a deterministic request, typed
from/to states, receipt, and monotonic revision. Generic mission runtime, zone
composition, objective ticks, commander progress, and generic failure settlement
skip exact and quarantined radio aggregates. Missing borrowed projections enter
an explicit dormant pending phase; broken reciprocal runtime rows quarantine.
Generated ONLINE restore requires a coherent destruction receipt followed by a
completed-rebuild receipt. Packaged proof of authored discovery, natural
explosives, damage reapplication, generated
replacement, stream re-entry, real restart, rendered markers/UI, networking,
reconnect, and JIP remains open.

Schema 60 adds the tenth explicit family consumer and eighth operation type:
only newly quoted and confirmed player Search-and-Destroy support uses contract
`1`, policy `support_search_destroy_exact_infantry_1`, and quarantine `-60`.
One catalog-selected infantry-only roster is frozen at $350 plus one HR per
member slot and linked through quote, ledger, request, operation, held batch, and
active group. It follows the direct strategic route, materializes/folds exact
survivors, and applies bounded virtual infantry combat at its assignment. A
physical fold farther than 75 meters from that assignment becomes
`RETURNING_TO_ASSIGNMENT`; hostile clearance leaves the force on station until
commander recall. Before fold, physical recall exit, or campaign-stop
retirement, the projection receives an exhaustive mapped-casualty
reconciliation; a nonzero roster must then prove one root plus exactly one
unique adapter and PhysicalWar member binding per durable living slot. Held-
batch cancellation first snapshots the strategic living roster. Persistence
applies an exhaustive global exact-infantry reconcile, repeats reciprocal and
cardinality checks for each physical exact player-support aggregate, and
refreshes its live position before capture; failure defers the retirement or
checkpoint. Pre-60 requests remain contract `0`, and malformed current claimants
cannot cross into legacy authority. Schema 60 is the preceding stamped source/
Workbench checkpoint: implementation `fdf78493dd15915afe8d53f61a8ad1efd65b5635`,
UTC `2026-07-11T23:24:55Z`, label `schema60-exact-search-destroy`, Foundation
pass with 644 symbol references, and final Workbench CRC `7aa80fc9` at 5,777
files/11,615 classes. Schema 59 is the preceding checkpoint. This provenance is
not packaged or live-behavior certification.

Schema 61 adds the first bounded authoritative client projection without adding
an operation family. The durable marker registry uses stable IDs, record
revisions, tombstones, an epoch, and a monotonic global sequence. A server
snapshot/delta journal and ownership-derived ACK sessions feed widget-independent
client registries, which in turn own local native campaign markers. One in-flight
delta batch, final-packet ACK, post-ACK catch-up, bounded readiness heartbeat,
and per-dispatch restart age prevent overlap while recovering lost ACKs or
incomplete streams. Builders and decoders enforce the same payload limits. The
client caches authored descriptor bindings and hides one only after its custom
zone replacement is live, restoring prior visibility on failure. Shared
priority/stable-ID ordering makes marker caps deterministic. The compiled source
proof defines ordering, idempotency, gaps/resync, reconnect/JIP-shaped snapshots,
rapid mutations, lost ACK, epoch reset, pruning, malformed/oversize input, and migration. Packaged host/two-client equality,
actual reconnect/late join, native widget behavior, and process restart remain
open, and its fixtures have not been executed as runtime evidence. The sealed
Schema-61 marker checkpoint identifies implementation
`27672e67ce4285810f313130293df1ac917c9bdf`, UTC `2026-07-12T01:02:39Z`, and
label `schema61-authoritative-marker-projection`. Full Foundation passes with
655 symbol references; final Workbench Game validation loaded 5,782 files/
11,631 classes with CRC `df41a779` and created the game. The hidden normal
WorldEditor stayed alive/responding for 10/10 samples over 20 seconds without a
first-party error/crash signature.

Schema 62 adds canonical location ownership without widening any exact-force
family. A zone carries contract version `1`, monotonic owner revision, one active
receipt backlink, and one latest-completed backlink. Admission canonicalizes
supported zone aliases before fingerprinting or replay, then validates expected
owner/revision, target faction, cause, immutable request fingerprint, and all old
exact-garrison authority before creating a receipt. The receipt owns
an ordered, explicit checklist. Each accepted exact patrol manifest must have
exactly one reciprocal, open, non-quarantined patrol operation, and that graph is
rechecked on pre-owner resume before its lifecycle settles. Orphan, duplicate,
quarantined, settled-but-still-accepted, and unsupported non-patrol exact
security fail closed. Old aggregate security and hostile runtime are retired,
new security is
established, and linked-town support is applied before the owner/revision write,
so a recoverable failure never exposes a half-applied new owner. Later steps
derive town security, generated-site/facility/logistics ownership, frozen
counterattack/aggression, economy/outcome, events, projection, notification, and
persistence from that result.

Top-level transitions publish only after their domain checklist is complete.
When a capture's exact support reward politically flips a linked town, the child
transition finishes its own domain work and records its parent request, but its
marker/menu/GM/notification publication remains deferred until the parent rebuilds
all markers. If a later pristine queued top-level receipt already owns that
town's active backlink, the earlier parent still applies its exact support/
influence fact once and defers only political threshold reconciliation. The
periodic civilian pass resumes that threshold after FIFO ownership work drains;
the parent never waits cyclically on a nested child it cannot admit. Later valid
top-level requests are admitted as pristine accepted/
needs-retry receipts and execute in array order before any domain mutation.
Ordinary rebuilds ignore queued pre-owner receipts, while an owner-applied active
receipt or a completed unreleased child retains its prior published owner/
revision. Published ownership is resolver-first: exact zone plus active/latest
receipt authority selects owner/revision, then an existing retained marker is
only a visibility/tombstone/owner/source-revision correlation check. It cannot
authorize ownership by itself. Missing or unsafe authority reports publication
unavailable, and restore quarantine purges unsafe zone-marker rows. Only the
active owner-applied parent request can authorize the single rebuild containing
its new owner and every completed child. Restore permits
serialized queues only while every later unresolved top-level follower is
pristine; it quarantines multiple owner-applied
incomplete top-level publishers or an owner-applied publisher behind earlier
unresolved top-level authority. Ownership publication captures the complete
logical marker array plus projection epoch/sequence, stages the authorized
parent/child graph, then validates each exact child receipt, reciprocal zone
backlink, owner/revision, marker ID, and marker source revision. All children are
released only after that full validation, and client/native publication commits
after release. Any failure restores the exact marker array, epoch, and sequence;
ordinary rebuilds cannot interleave with an active stage. Setup publication has
no live zone-marker rows and instead freezes `m_bSetupProjectionWithoutMarkers`
plus its exact decision on parent and children. That immutable receipt history
survives save/restore and later active-phase marker rebuilds. Production and
deterministic proof call the same logical snapshot builder and exercise the
staged rollback and two-child all-or-nothing release. This prevents nested or
unrelated refreshes from publishing a mixed ownership graph. Identical request replay
returns the receipt without mutation; changed-fingerprint reuse, stale owner/
revision, same-zone authority, and unsupported security reject. Incomplete
accepted receipts persist and retry at a bounded cadence or restore, including
setup and terminal phases where the campaign clock is frozen. If applying a
retry quarantines the receipt, runtime and restore reconciliation preserve that
concrete failure reason on receipt and zone rather than overwriting it with a
generic retry failure. Malformed current graphs retain `-62` quarantine
evidence. History is capped at 512 rows with at
least 86,400 campaign seconds of replay retention, while latest, incomplete,
quarantined, unreleased-
child, and unresolved-order rows remain pinned.

Maintenance orders ownership retry before town policy. Both run before setup or
terminal early returns and bypass frozen campaign-time rate limiting; a political
repair admitted in those frozen phases suppresses fresh retaliation and
notification.

All causes share receipt/idempotency authority, not identical policy flags.
Military and mission captures retain normal enemy consequences. Generic admin
changes reconcile security and notify but suppress retaliation. Debug seeding
also suppresses notification. Migration repair suppresses retaliation and
notification and preserves existing security instead of creating or retiring a
garrison. Admission and current-schema normalization enforce these cause flags.
Admission also stops before a revision that cannot safely advance exactly once
or serialize as valid current authority. Exact mission, political, admin, and
migration requests retain their
original fingerprint while queued. Rebuilt explicit-ID commands reuse frozen
preconditions only after semantic request identity matches. Admin capture/
progress reports distinguish an
accepted-pending receipt from a true rejection rather than finalizing queued work
as failed.

Support authority is frozen as the canonical sorted set of linked towns plus
every town within 1,500 m. Restore recomputes that set, requires applied rows to
be its ordered retry prefix, and requires every applied entry to use one exact deterministic influence
event row whose ID, causal tuple, reason, and deltas all match. The same validator
correlates stable old/new garrison IDs and rows, counterattack chance/roll/
selection plus any unique counterattack order, bounded nonblank receipt reasons,
and unique campaign/strategic rows. Ownership strategic events deliberately do
not diff global money, HR, HQ knowledge, support, or enemy resources across a
queued/retry window; they record exact owner before/after, capture progress, and
receipt aggression instead.

`m_bPersistenceRequested` means the service scheduled the normal major-change
checkpoint after publication; it is not evidence that native persistence or the
profile fallback reached disk. Only a real save/restart result can close that
durability gate.
Major-change scheduling is edge-triggered and coalesced behind a bounded
deadline. Repeated gameplay or retry heartbeats do not move that deadline; after
a successful checkpoint, the next first change starts a fresh interval. Receipt
completion calls `EnsureMajorChangePending()` after its final status/backlink
mutation, so a restored durable `m_bPersistenceRequested` flag cannot leave the
process-local deadline disarmed; an already pending deadline is not extended.

Liberated security policy is deliberately conservative: enemy town police and
roadblocks are retired, and a resistance-held location receives at most two
aggregate infantry when it has garrison capacity, with no automatically created
vehicle. Enemy recapture scales infantry from capacity/priority and may add one
vehicle at an outpost, airfield, seaport, or factory and two at an airfield or
seaport. Existing exact new-owner garrison authority is retained instead of
being overwritten. This policy is implemented source behavior, not yet packaged
balance or native-spawn evidence.

| Concern | Current implementation | Target architecture |
| --- | --- | --- |
| Stable identity | A persisted monotonic allocator creates authority IDs; garrisons, quotes, manifests, transactions, and selected support/order/group records carry explicit stable links. Schema 58 additionally binds the rescue operation/guard graph to three deterministic captive slots, one frozen extraction position, and stable escort, carrier, seat, command, casualty, extraction, and projection evidence. | Every durable operation, force, projection, command, transaction, and event has a stable ID and explicit links. |
| Command idempotency | Visible command requests carry request IDs; bounded receipts cover migrated training and quote/confirm commands. Support recall maps a typed result into explicit receipt status. Schema 58 additionally keeps accepted and rejected captive request/actor/command/result/revision rows, rejects cross-captive or changed-fingerprint reuse before live checks, and reserves one non-evicting terminal slot. Other visible commands still use the compatibility classifier. | Every player-visible and scheduled campaign mutation enters through a typed command envelope and produces one durable receipt. |
| Resource integrity | Troop training, visible garrison confirmation, player-QRF, and Schema-60 Search-and-Destroy confirmation/terminal settlement use the resistance resource ledger. Sealed Schema 67 separately makes versioned enemy attack/support/aggression pools and bounded strategic mutation receipts canonical; exact enemy QRF/patrol debit/refund policy links into that boundary while unsupported orders remain legacy. Exact defensive QRF settlement validates its support-only-or-dual-pool debit/refund graph, persists `PREPARED`, stages a complete unapplied tuple, validates durable survivors and claimants, applies/replays the refund, publishes the applied receipt last, and then finalizes terminal tails. | All resource changes use reserve/commit/cancel/refund transactions or the typed per-enemy strategic mutation boundary, with no direct debit paths outside the owning authority. |
| Force exactness | Existing exact infantry/convoy/rescue shapes remain unchanged. Schema 60 adds one infantry-only Search-and-Destroy root/member manifest and rejects vehicles, assets, empty rosters, and multi-root substitution. Historical Search-and-Destroy, other support/mission families, policy-v1/initial/enemy aggregate garrisons, and unsupported vehicle/multi-root policy remain outside these cutovers. | A quoted immutable force manifest is the only input to paid creation, and creation is all-or-nothing before any physical or virtual projection is published. |
| Force realization | SpawnQueue accepts frozen, hash-valid, all-required one-root infantry manifests. Its bounded selector now admits due deferred/retryable slots when their batch can start another attempt, while preserving manifest/dependency validation and normalizing slots only after selection. Both QRFs, player Search-and-Destroy, exact enemy patrol, policy-v2 purchased-garrison patrol, all three exact assassination guards, exact Schema-69 enemy counterattacks, and exact Schema-70 enemy garrison rebuilds release only durable living member slots; each empty root contributes no authored members. Counterattack and rebuild physical/virtual transfer use the same casualty slots and never refill them. Delivered rebuild manifests remain exact held garrison authority instead of being copied into aggregate infantry. R22 passes retry, stale-generation, same-wave, and interrupted-restore queue proof. The schema-52 convoy keeps its separate three-element PhysicalWar adapter. Generic vehicle/asset/multi-root and historical aggregate consumers remain unsupported. | One adapter realizes every supported manifest, registers each slot exactly once, restores successful projections safely, and feeds durable living-force/casualty/retirement authority without bypass paths. |
| Operation lifecycle | Schemas 50-60 retain their exact paths. Schema 69 adds a separately typed enemy-counterattack operation with one direct route, deterministic virtual combat, casualty-preserving physical/virtual transfer, canonical ownership request/retry, return-to-origin duty, and survivor-proportional one-pool settlement. Schema 70 adds a separately typed enemy-garrison-rebuild operation with frozen capacity, exact held delivery under an `OPEN`/`ON_STATION` operation and zero-delta receipt, proportional prearrival survivor refund, later zero-refund terminal retirement, and PREPARED/SETTLED crash-resume finalization. Historical counterattacks and rebuilds remain outside their exact contracts. | Every force/order uses one versioned operation aggregate with event-driven engagement, strategic movement progress, physical/virtual transfer, settlement, and client/JIP projection. |
| Event history | New command and ledger decisions append to a bounded persisted campaign event log. | All authoritative state transitions emit typed events consumed by projections, UI, diagnostics, and restore reconciliation. |
| Canonical ownership | Schema 62 routes military capture, mission capture, political support, admin, debug seed, and migration repair through one revisioned, idempotent receipt. The receipt owns security, support consequences, owner, town/facility/logistics derivation, retaliation, economy/outcome, events, parent-aware projection, notification, and persistence scheduling. Schema 64 submits strict political threshold intent directly to this service and cannot publish town ownership itself; a valid later receipt queues behind an unresolved earlier receipt under the canonical service's ordering. Production assigns mission cause only when `FindActiveMission` resolves the supplied source. | Historical e11 canary evidence exposed a stale mission-source proof setup: `ownership_transition.aggregate` and `ownership_transition.causes` failed. Current source seeds exact active missions, verifies cause/type/ID provenance, and keeps an unresolved-source military negative. That correction is sealed in retained ee0, whose packaged focused set and corrected canary pass noncertifying at 35/35 and 87/87. The unchanged package's full profile is independently rejected red at 598/47/26/13/1 and 5,630/5,695. Native projection, active-world security/economy effects, save/restart, packaged clients, and all caller routes remain runtime gates. |
| Canonical combat presence | Sealed Schema 63 separates registered physical infantry/manned-mobile/static samples, eligible virtual infantry, bounded cached queries, and revisioned zone heat from render state and empty assets. Capture, missions, HQ, civilians, enemy strategy, and the legacy wrapper share one injected service. The focused deterministic engine fixture passes its combat-presence group as part of the 35/35 focused case. | The result is state-only fixture evidence. Runtime-prove native occupant/platform classification, every consumer, fail-closed authority/player filtering, allocation/cache invalidation/order, 30-second cooling, conservative restore, no save-dirty churn, and activation/deactivation hysteresis before deepening encounter and town truth. |
| Canonical town influence | Sealed Schema 64 gives each curated town one revisioned support/population/contact/flip record, separate FIA/occupier/invader basis points, exact typed events, strict 8000/4000 hysteresis, conservative pre-64 migration/current `-64` quarantine, and legacy projections limited to canonical curated towns. Current exact events persist population triples before/after mutation; absolute debug seeds use the same idempotent boundary, and restore verifies the complete population chain plus the final record. Mutation-time aggregates and due-expiry-only history scans replace the old unconditional one-second fold. Immutable target-pair scoring and copied-save corruption now exercise the intended migration/quarantine boundary. | The focused deterministic engine fixture passes its town-influence group. Production/native callers, save/restart, expiry, rendered Map/War rows, town taxonomy, JIP, and no-stutter behavior remain open. |
| Civilian consequences | Sealed Schema 65 source/Workbench adds exact town casualty/theft/combat events, optional aggression target/delta/before/after evidence plus one matching strategic receipt, persisted combat episodes/adopted floor/last-applied receipt, bounded 256-casualty and 64-theft queues with a combined four-transaction frame cap and indefinite bounded-backoff retry, exact-pilot post-promotion theft, and pedestrian panic/recovery with separate bounded route recovery. Minor localities remain panic-only and keep their exact fingerprint map in session memory. Foundation passes at 717 script-symbol references. Final stamped normal/all-five Workbench checks are clean at 5,802 Game files/11,728 classes with CRC `c0a672b9`, `Script validation successful`, zero HST script errors, and zero surviving processes. | Execute the deterministic fixtures, then package-prove native callback attribution, no duplicate casualty/theft/aggression, threat-driven RUN/calm WALK transitions, restore/quarantine, minor-locality restart behavior, and balance under multiplayer/soak load. |
| Enemy-town local security | Sealed Schema 66 source/Workbench gives each eligible canonical enemy town one deterministic exact patrol epoch backed by a frozen authored 2–5 member roster and held SpawnQueue slots. Casualties survive physical fold and restore; no generic police projection may refill or fold counts into it. Destruction applies exactly one police `-1` event, while owner/pressure clear, setup/stop, and spawn failure settle without loss. Same-epoch resurrection is forbidden; a newer owner revision or later positive police event is required for rearm. Resistance automatic police/roadblock targets are zero. Pre-66 migration preserves logical facts and drops only backlink-free disposable legacy projections. | R22 passes all eight isolated `local_security.*` assertions with no materialization deferral. While Campaign Debug holds the force-spawn worker, it also holds the ambient producer so no patrol can be released into a process-only MATERIALIZING state before checkpoint capture. Package-prove native roster, waypoints, casualties, fold/re-entry, no refill/no resurrection, exact rearm, ownership ordering, persistence/restart, campaign terminal cleanup, multiplayer, and balance. |
| Canonical enemy strategic resources | Sealed Schema 67 makes each versioned `HST_FactionPoolState` the unique attack/support/aggression/cadence owner for one enemy role. Compact periodic receipts and last-bucket checkpoints are separate from a contiguous un-compacted operational sequence, including zero-effect rows, capped at 4,096 per faction. One API owns live mutation; restore validates exact order/ledger/town/ownership backlinks. Pre-67 restore adopts baseline values/checkpoints without invented history; malformed current graphs quarantine at `-67`. | Real-restart cadence, cap, reciprocal backlinks, and no-bypass proof remain open. Schema-68 planning consumes this authority without mutating or repairing it. |
| Canonical enemy planning | Sealed Schema 68 adds one independent 180-second planning row per configured enemy, sorted commitment/target/source hashes, frozen input and decision fingerprints, bounded preparation/prepared retry, explicit committed/skipped/rejected completion, exact order/debit backlinks, conservative pre-68 baselines, and Schema-68 `-68` quarantine. The sealed bootstrap correction shares one production fresh-state factory, admits only the exact known quarantine signature, and throttles unchanged unavailable warnings. Commitment roots use blocking precedence, compatible penalties, and deterministic same-decision reranking; preparation remains freeze-only and admission is rechecked before debit. | R10 proves held ambient cadence, exact clock/fingerprint isolation, all five Phase 18 cases, and explicit Phase 22/24 production ticks. Package-prove restart, dedicated/live-server, multiplayer/networking, and soak behavior. |
| Exact enemy counterattack | Schema 69 gives new contract-`1` counterattacks one reciprocal order/operation/manifest/batch/group aggregate, one charged attack or support pool, direct virtual travel, deterministic combat, physical/virtual casualty continuity, canonical ownership transition, return, and proportional survivor refund. Schema-neutral restore correlates request-ID-or-operation-source ownership rows before ownership reconciliation. The eighth guarded cut proves raw-to-normalized pending restore, exactly-once owner completion, one production tick into raw/restored `RETURNING`, and replay with no ownership/tick/canonical-file mutation. Historical rows remain contract `0`; malformed Schema-69 graphs quarantine at `-69`. | The guarded native branch now proves prepare/recover/replay source precedence through exact savepoint UUIDs while preserving the conflicting fallback. Durable endpoint ABA snapshots remain a future contract-2 schema decision whose schema number is not yet assigned; broader world/package/client/network behavior, migration breadth, markers, performance, and soak remain open. |
| Exact enemy garrison rebuild | Schema 70 gives new contract-`1` rebuilds one support-funded reciprocal aggregate, capacity-capped frozen infantry, owner-and-revision-bound target/source capability, casualty-preserving virtual/physical travel, and an exact manifest link at the destination. Delivery records a zero-delta receipt, remains `OPEN`/`ON_STATION`, and never double-counts aggregate infantry. Prearrival survivors return and refund proportionally; delivered terminal events unlink and retire without refund. Malformed or orphaned authority quarantines at `-70`, becomes non-executable, and remains retention-pinned. The guarded `delivery_pending` proof preserves one casualty across 225/300 meters, delivers the 8 survivors once, and replays read-only. The separate `physical_live_fold` proof materializes one root, 9 handles, and 8 living runtime members, observes 2.759 meters of movement and 0.539 meters of closure, folds through the production path to the exact 8-living/1-casualty virtual graph, then restart/replays without mutation or residue. | The two scoped guarded restart cuts pass. The separate mixed-native controlled-shutdown graph also passes its five-stage restart/fallback proof. Natural route/contact combat, other force families, ownership ABA, arbitrary mission/force graphs, multiplayer/JIP/reconnect, markers, performance, and soak remain open. The focused autotest also retains a stock reload/JUnit harness gap. |
| Political Map/War projection | Sealed Schema 64 supplies contacted-only Zone Pressure with current-first/stable support ordering and complete deterministic Resistance Territory from published canonical ownership. Resistance Territory reuses the marker projection's completed-parent ownership resolver, preventing a nested child from appearing before its parent transition publishes. | Prove rendered rows, current-town detection, discovery, incomplete ownership fencing, no arbitrary truncation, save/restart, reconnect, and JIP. |
| Client marker projection | Schema 61 implements stable marker IDs with record revisions/tombstones, one epoch/global sequence, bounded hashed snapshot and ordered-delta packets, ownership-derived sessions, an atomic registry, deterministic priority capping, and client-local native reconciliation. Schema 62 adds ownership source revision, while the Schema-66 repair keeps protected campaign markers system-owned/non-removable and self-healing. Exact QRF, counterattack, garrison-rebuild, and patrol audit backing now calls the marker publisher's canonical-ID and operation-specific visibility predicates. | Execute the destructive owner-client probe, then package-prove edit/delete resistance, bounded self-heal, exact operation-marker continuity, snapshot/delta, map reopen, reconnect, and JIP. |
| Radio physical authority | Schema 59 keeps one exact lifecycle owner per site. The current adapter queries the generic base, exact stock `SCR_DestructionMultiPhaseComponent`, and destruction base, then returns shared health/state authority across admission, polling, writes, restore, and suppression. Generated demolition resources enable the existing inherited multiphase/RPL pair, and zero-health destruction uses the engine `Kill()` path. | R16 proves the isolated disposable destroy -> stop-rebuild chain, including normal callback, deterministic receipts, unchanged destruction epoch, exact `$450`/`$350` rewards, second-attempt rejection, exact cleanup, and zero final state diff. Packaged authored binding, restart/streaming reapplication, multiplayer, and soak proof remain open. |
| Destroy-target demolition witness | Nearby evidence must be an unparented physical projectile with active movement or a triggered blast; parented/inventory equipment is rejected before text classification. Witness scans and entity-backed callbacks share one canonical source key. A target retains at most 64 lifetime source receipts, fails closed at capacity, and writes local bookkeeping only after authoritative asset mutation. | Fresh Workbench validation passes. R23 passes all six generic `primitive.destroy.no_ambient_witness_score` assertions and all seven destroy-family start/runtime/primitive cases. Preserve one-source/one-score behavior through callback-plus-scan overlap, carried equipment, restore, multiplayer, and soak proof. |
| Controlled campaign persistence | Schema 71 gives typed automatic, manual, and shutdown requests one staged snapshot and one persisted checkpoint sequence. Native-active requests advance the verified two-slot profile journal only after the post-commit `SaveGameManager` callback; native-unavailable or explicitly profile-only sessions write it synchronously. Startup reconciles native and journal snapshots by checkpoint sequence, restore sequence, save second, and matching normalized fingerprint. Future or ambiguous authority is preserved and fenced. Controlled end drains pending work, uses a 270-second retry window, then requests blocking shutdown and preserves or purges native state according to the authority that committed. | The focused 41-case journal authority proof and strict automatic/manual/shutdown/native/journal five-process chain pass. The sealed extension proves one mixed-native captive/carrier/player/seat and guard graph across the same stages, with durable carrier rebind, stable seat recovery, native/profile fallback, and zero cleanup. This is crash-tolerant single-writer recovery, not atomic rename, authenticated storage, an off-device backup, or broad active-world/client/network certification. |
| Campaign Debug isolation | The runner deep-clones campaign state, suspends normal persistence, and restores the live state. Bounded probes additionally capture/restore the shared clock and enemy-strategic fingerprint; the coordinator holds ambient commander cadence while the clone is active. It also holds ambient local-security progression whenever its matching worker is held. The separate restart harness is not a Campaign Debug clone: a strict startup guard authorizes one disposable source-selection carrier and one-use prepare/recover/replay lease. | Retained prior-package run `seed1985_t0_p1_u1784414040` produced a mechanically complete, stable, preliminary-unaccepted capture with exact candidate/mount identity and zero drift or cleanup residue. Its report remains red at 584 PASS/49 WARN/46 FAIL/7 BLOCKED/1 SKIPPED and 5,562/5,688 required assertions, while the corrected census records 25 raw script errors, 19 in the Partisan subset, and ten unapproved diagnostics. Historical 0e canary `seed1985_t0_p1_u1784424219` is immutable scoped `passed-noncertifying` evidence: 9/1/0/1/0 cases, 35/35 and 87/87 focused proof, 18/0 state restoration, two approved stock diagnostics, zero unapproved diagnostics, and zero residue. The same package's full run `seed1985_t0_p1_u1784425330` is immutable rejected red history at 584/49/46/7/1, 5,561/5,687, and ten unapproved diagnostics. Historical e11 canary `seed1985_t0_p1_u1784437399` is immutable rejected evidence: 8/1/1/1/0 cases, 33/35 and 85/87 focused proof, 18/0 restoration, two approved stock diagnostics, zero unapproved diagnostics, ten rehashed files, and zero residue; full was stopped. Retained ee0 has an accepted five-case packaged focused set under clean harness `273ed14ba8526259c8b0d248177fa53b59ade683` and an accepted corrected canary `seed1985_t0_p1_u1784445266` under clean harness `4f8d7e2d7a39896737fd6754060523bf852c5fa8`: 9/1/0/1/0 cases, 35/35 and 87/87, 18/0 restoration, two approved stock plus zero unapproved diagnostics, ten rehashed files, and zero residue. Both scoped rungs are `passed-noncertifying`. Its unchanged-package full run `seed1985_t0_p1_u1784446076` under clean harness `a5ccf36aee17a4f88d7f1c2f232ce9fc14652018` is mechanically exact but rejected red at 598/47/26/13/1 and 5,630/5,695, with 24 unapproved diagnostics and zero residue. Its runtime disposition is `rejected-after-runtime`; no further runtime evidence may attach. The guarded native restart branch proves one separate source-precedence chain; no result generalizes to another package, arbitrary client/network, or certification behavior. |
| Workbench compiler shape | Large Campaign Debug methods use compact context/result objects and focused helpers. The render-bubble proof keeps clock state in `HST_CampaignDebugClockIsolationContext` rather than extending an already-large local frame. | Preserve this boundary and require a fresh Game compile plus bounded cold open for future large proof additions; repository text/static validation cannot exclude a native compiler heap failure. |
| Certification | Campaign Schema 71 and runtime-settings Schema 24 are current. `32727238d74b29905c68e5a80bb5897dfdc783c0`, UTC `2026-07-18T16:34:38Z`, label `schema71-settings24-focused-force-authority`, is the sealed embedded implementation identity. Retained `rejected-after-runtime` candidate `partisan-rc-ee0e8add2a29-20260719T063815Z`, version `0.1.0-rc.20260719T063815Z.ee0e8add`, was built from clean source HEAD `ee0e8add2a298e83fd304b7660c4fc480dc6383f`. Its package, manifest, and ready SHA-256 values are `981258439b9d08866c4883471cacfe33aa373a36a667a39e8c939f285db74daf`, `1b877e3aa21773a268704bcb3fe889768fca3aa2d78541aa7285b061398ce907`, and `01741b85d0edba69f54b07388cdd7c452b8f6f1ad7ef4f6faf253918a4bbf280`. Foundation passed all 874 checks, and all five Workbench targets passed at 5,848/11,901 and CRC `f64e0868`; its seal binds four package files and 50 evidence files. | Clean harness `273ed14ba8526259c8b0d248177fa53b59ade683` passed ee0's five packaged focused cases with JUnit 5/0/0/0, 40 retained envelope files, 12 classifier checks per run, 11 approved diagnostics, and zero residue. Clean harness `4f8d7e2d7a39896737fd6754060523bf852c5fa8` accepted the unchanged package's corrected canary at 9/1/0/1/0, 35/35, 87/87, 18/0 restoration, two approved stock plus zero unapproved diagnostics, ten files, and zero residue. Deterministic-service and the scoped native-engine/world canary are `passed-noncertifying`. Full Campaign Debug under clean harness `a5ccf36aee17a4f88d7f1c2f232ce9fc14652018` is rejected at 598/47/26/13/1 and 5,630/5,695, with 24 unapproved diagnostics despite exact capture integrity and zero residue. Ledger `history[0]` retains 0e and `history[1]` retains e11 without evidence transfer. The retained candidate/package remains immutable and cannot accept further runtime evidence; source fixes require a new candidate. Release remains `NO-GO`; Workshop/server/client/network/JIP/reconnect, markers/UI, arbitrary migration, performance, and soak remain open. |

The canonical ownership dependency and first shared crew-aware combat-presence/
heat dependency remain sealed through Schema 63. Sealed Schema 64 adds the
town-truth and political-map dependency and has passed Foundation plus normal
and all-configuration Workbench validation. The dedicated focused engine case
also passes the three state-only groups at 14/14, 9/9, and 12/12. Together they
advance Blueprint Phase 7 but do not finish it: native
runtime proof, generalized encounter outcomes, and broader post-liberation/
facility consequences remain subsequent work.

Concurrent open garrison quotes are capped and expired/terminal unreferenced
planning rows can be pruned. SpawnQueue terminal projection rows have explicit
backlink pins, a 600-second minimum retention window, and a 128-row admission
bound; the production coordinator now supplies pins and runs that maintenance.
Schema 48 retains accepted rows in full for at least 600 seconds and while any
projection backlink exists, then atomically moves replay identity and final
transaction evidence into at most 256 tombstones with an 86,400-second minimum
window. Full quotes plus tombstones share a hard 320-row admission bound. An old
evicted quote fails closed rather than recreating a debit or aggregate mutation.

Until those target columns are closed, domain services remain authoritative for
their existing behavior, but they must not be described as uniformly
idempotent, ledger-backed, or exact-manifest driven.

Campaign-debug state isolation is fail-closed. Every in-process profile is
restricted to the development world. The persistence service first durably
captures the live campaign, the coordinator runs the suite against a deep-cloned
`HST_CampaignState`, checkpoints only isolated in-memory save data, and swaps the
untouched live state back on completion or cancellation. External/soak profiles
cannot enter this runner. This protects authoritative campaign persistence, but
does not roll back world entities, player inventory/health, delayed callbacks,
or service caches, so a development-session restart remains part of the safety
boundary.

## Authoring Contracts

The addon keeps authoring data separate from runtime state:

- `HST_CampaignPreset`: fixed scenario roles and capability switches.
- `HST_FactionTemplate`: faction identity and capability declarations.
- `HST_MapDefinition`: stable IDs for zones and hideout candidates.
- `HST_BalanceConfig`: campaign balance and progression values.
- `HST_MissionRegistryConfig`: mission definitions and deferred capabilities.

The current coordinator uses `HST_DefaultCatalog` as runtime authority. The
checked-in faction `.conf` resources are not loaded by that path and are not
guaranteed mirrors; they must not be cited as gameplay truth until one loader
and startup validator replaces the duplicated declarations. Schema-43 exact
force planning uses `HST_ForceCatalogService`, whose explicit catalog version
and ordered slots are validated against effective prefab containers at runtime.
Schema-44 queue admission consumes the frozen execution-prefab slots directly;
manifest identity binds the input but never substitutes for a projection or
idempotency key. Schema 45 keeps the group root and member entities as runtime
projections while persisting their force/projection links and registration
evidence. Schema 46 selects one authored player-QRF group, freezes all ordered
slots, and submits that exact manifest without invoking the broad composition
service. Schema 47 persists each exact member's living/retired lifecycle and
reuses the immutable manifest while excluding confirmed casualties from restore
reprojection. The current adapter deliberately rejects vehicle, asset, and multi-root
manifests instead of shortening them or falling back to broad composition.

## Persistence

Before persistence or settings consumers read profile data, the profile path
service attempts the recursive verified tree move described above. A failed copy,
content comparison, source deletion, or empty-directory cleanup leaves the
remaining source available and reports a retryable warning rather than claiming
completion.

The persistence service tracks `HST_CampaignSaveData` through
`PersistenceSystem`, applies restored state through a schema migration path,
and accepts typed `AUTO`, `MANUAL`, and `SHUTDOWN` checkpoint requests. With
native authority active, it flushes the tracked scripted state, requests the
native save point, waits for its post-commit `SaveGameManager` completion
callback, and advances the exact staged snapshot into the profile journal only
after that callback succeeds. A failed callback leaves both journal generations
untouched and rearms the checkpoint.
If native persistence is unavailable, it may complete the typed checkpoint by
writing the profile journal synchronously. A session that recovers from an
explicitly unusable native system remains profile-only instead of attempting a
later native write against that degraded authority.

The profile journal alternates the canonical and recovery slots. Each version-1
journal envelope contains the exact nested save payload, generation and parent
metadata, and the payload fingerprint
`uuidv8-sha256-v1:<serialized-length>:<UUID>`. The inactive slot must write,
read back, validate, and become the selected chain head before it supersedes the
older slot. Raw canonical saves from Schema 70 or earlier are generation zero;
their first Schema-71 successor is written to the recovery slot without changing
the raw bytes. The file API supplies neither atomic rename nor an exclusive lock,
so this is a single-writer crash-tolerant journal rather than an atomic or
authenticated backup.

Native envelope version 2 stores and fingerprints the exact serialized payload
string, validating bytes before DTO parsing. Schema-70 version-1 native rows
validate their reconstructed legacy length/hash fingerprint and normalize to a
current comparable identity. Startup orders valid native and journal
snapshots by checkpoint sequence, restore sequence, and save second. The newer
source wins; equal order with different fingerprints is fatal. Missing or
explicitly unusable native authority may select a valid journal, while unknown
or future native authority fails before journal fallback. Source selection
preserves invalid journal artifacts. A later verified native checkpoint may
repair ordinary invalid inactive data, while future, split-brain, broken-chain,
or otherwise ambiguous history remains write-fenced. If a retired campaign slot
remains after tree migration because its canonical counterpart is missing or
different, startup fails until that unresolved authority is handled; journal
selection never reads directly from the retired tree.

Controlled game-mode end has a separate fail-closed bridge. It drains one
normal coordinator interval with controls and new campaign commands disabled,
requests one `BLOCKING` shutdown checkpoint, quiesces further mutation, waits
for bounded post-commit callback completion, and rechecks the campaign stability
fingerprint before entering the stock transition. The retention hook preserves
a verified native commit by bypassing the stock purge; a journal-only result
explicitly removes stale native authority. `OnAfterSave`/`OnSaveCreated`
correlation and transition polling belong to the guarded proof and are not
required by production. A forced process kill cannot execute this bridge and
therefore guarantees only the last previously completed checkpoint.

The state model is
versioned from day one. `HST_CampaignSaveData` is the deep-copy save container
for current campaign fields and nested runtime arrays. Sealed Schema-67
restore first establishes one versioned faction-pool authority per configured
enemy and validates its bounded strategic mutation receipts before any service
can mutate income, spend, refunds, aggression, or live pool targets. Pre-67 state adopts valid
pool balances, aggression, and legacy cadence as a baseline with initialized
bucket checkpoints and no synthesized receipt or
planning/order/accounting history. Current duplicate pools, invalid enemy roles,
noncanonical arithmetic/operational sequences, cadence checkpoint divergence,
broken exact order/ledger/town/ownership backlinks, conflicting receipt
identities, or out-of-bound histories quarantine at `-67`; restore never
replays a retained mutation. Sealed Schema 67 contains no planner cadence or
decision fingerprint, and the active Schema-68 migration cannot infer either
from pool or order history. It installs only conservative role baselines for
pre-68 saves; current Schema-68 rows must validate their frozen fingerprints,
cadence, roles, and order/debit backlinks or quarantine at `-68`.

Schema-66 restore then
isolates exact local-security claimants before generic group/manifest/batch
normalization, then applies the existing Schema-65 dependency order. Pre-66
migration clears stale patrol backlinks and removes only unlinked disposable
legacy police groups; it preserves police/roadblock pressure, ownership,
support, and garrisons and creates no exact roster, casualty, fold credit,
operation, or refund. Current active graphs require unique reciprocal patrol/
zone/operation/manifest/batch/group identity and exact slot counts; terminal
graphs retain only their compact operation/manifest evidence. Malformed
authority quarantines at `-66`. Schema-65 restore keeps a strict dependency
order and builds bounded event/receipt indexes before per-zone
validation: current/pre-65 town event shape and structural aggression evidence
are normalized first; combat-presence physical samples are invalidated;
ownership and marker authority are normalized; town pending-owner links and
their unique strategic receipts are validated; only then is each civilian-
consequence zone envelope migrated or quarantined. Save-shape validation proves
pool uniqueness, arithmetic, and receipt linkage, but cannot infer faction roles
that are not serialized. Immediately after restore, the live preset validates
every aggression target as a configured enemy and quarantines the town/event/
locality chain otherwise. Pre-65 town events receive empty/zero aggression
fields rather than invented economic history. A malformed
aggression target/delta/before/after chain or missing/duplicate/mismatched
strategic receipt quarantines the canonical town authority at `-64`; a malformed
danger episode, adopted baseline floor, last-applied receipt, combat revision,
panic deadline, or exact live-town last-event backlink quarantines the locality
consequence envelope at `-65` and clears active danger/panic. Restore enforces
`episode - lastApplied <= 1` and never replays either event's effects. Stable-ID
exhaustion returns no strategic ID and fails admission before mutation. Before autosave, major-
change, manual-checkpoint, or campaign-debug baseline capture, schemas 52 through
66 synchronously reconcile mapped physical exact-convoy members and every
currently cut-over physical or dematerializing exact-infantry family. Patrol,
mission-guard, and player-support projections must prove one unique handed-off
root, one unique live handle per durable survivor, matching result/projection
and slot keys, matching PhysicalWar membership, and an authoritative live
position where that family moves. Exact player support also requires one
unambiguous reciprocal request/operation/batch/group graph; its global exhaustive
casualty pass is followed by a per-aggregate root/member cardinality proof and a
live physical-position refresh before capture. An unexplained deleted binding
remains unresolved rather than becoming a casualty. An open outbound publication
transaction, materializing exact-infantry handoff, missing/conflicting mapping,
aliased or cross-key runtime ownership, invalid cardinality, unverifiable
physical position, or nonphysical operation retaining process-local member
mappings defers capture before an older tracked snapshot is flushed or an engine
savepoint is requested. Open exact local-security patrols are included in that
barrier: a physical or dematerializing patrol must fold to exact held survivor
authority, and failure defers capture. Ownership transition uses the same
preflight before old-security cleanup or owner publication. Quarantined patrols
also defer until both adapter and
PhysicalWar runtime ownership are empty, then retain the diagnostic graph under
strategic authority without refund. The pending checkpoint intent remains set
and retries on the bounded retry cadence. Schema 53 persists the
monotonic authority sequence, bounded command receipts, resource transactions,
campaign events, stable operation/garrison links, immutable force manifests,
expiring quotes, durable per-projection spawn batches/slot evidence,
Game Master registration evidence, explicit active-group force/projection IDs,
support quote/capability/schedule fields, linked exact-QRF aggregate identities,
successful-handoff/reprojection counters, member casualty tombstones, active-
group ever-populated/spawn-completed/living-force evidence, and the queue
restore/reconciliation epochs, plus bounded accepted-settlement replay tombstones
and their final resource-status/refund evidence. It also persists canonical
exact-QRF operation records with immutable assignment, mutable tactical target,
typed duty/engagement/materialization/position/settlement state, execution links,
policy IDs, timestamps, terminal result, revision, direct-route cursor and
distance, strategic speed/update clock, projection decisions, virtual-combat
clock/damage carries, exact virtual force counts, strategic batch holds, enemy-
order source/manifest/service-commit/resource-settlement authority, reciprocal
enemy-order/operation/group backlinks, and distinct-second physical-arrival
confirmation state, plus exact enemy-patrol generated-route waypoint/lap/leg
cursor and loop clocks, proactive resource settlement, contact-held projection,
and reciprocal route/order/operation/manifest/batch/group identity, plus exact
mission-convoy operation/manifest/spawn/
settlement links, three durable convoy elements, per-element vehicle/crew/cargo
identity and state, mission-asset vehicle-slot assignment, generated-route
cursor, and exact convoy arrival/settlement authority,
  plus schema-54 exact purchased-garrison quote/manifest/garrison/operation/
  generated-route/held-batch/group links, local loop cursor, exact casualties,
  and no-refund terminal receipt, plus schema-55/56/57 exact officer/traitor/spec-ops mission/
  operation/manifest/held-batch/group links, route-less guard anchors, exact
  casualties, typed terminal results, separate HVT authority, and mission-family
  contract/policy/quarantine identity, plus Schema-60 Search-and-Destroy quote/
  manifest/paired-ledger/request/operation/held-batch/group links, exact roster,
  route/combat/return-to-assignment state, recall/settlement identity, and
  contract-0/`-60` restore disposition, alongside campaign
metadata, resources, campaign-end
reason/summary/elapsed second/control/war/zone-count fields, outcome-mode,
population/support, airfield metadata, support deployment proof, active-group
vehicle prefab, active-group route waypoint counts, runtime infantry waypoint assignment and final-sweep state,
HQ/Petros/cache/arsenal/tent/spawn-point fields,
versioned faction pools and sealed Schema-67 bounded enemy strategic
mutation receipts, players, zones, garrisons, active groups, QRFs, map markers with
Schema-62 per-record projection/source revision, stream, and tombstone metadata
plus the marker-projection epoch and global sequence, durable ownership-
transition receipts and zone active/latest ownership backlinks,
generated content, objectives, mission runtime, mission assets, support, enemy
order, civilian, undercover, arsenal, garage, vehicle cargo, runtime vehicle,
saved loadout, issued-item, ammo point, and captured-emplacement records.
Loadout editor sessions remain runtime/editor state, while durable saved
loadouts and issued-item ledgers are copied into the save container. Save compatibility
still needs broader Workbench restart/load soak testing before it is promised
to players. An actual persisted restore increments its epoch once, then the
coordinator reconciles the queue before garrison/player-QRF confirmation,
  schema-51 enemy-QRF authority, schema-52 mission-convoy normalization, schema-53
  enemy-patrol normalization, schema-54 purchased-garrison patrol normalization,
  schema-55 officer-mission guard normalization, schema-56 traitor-mission guard
  normalization, schema-57 spec-ops-mission guard normalization, Schema-60
  player Search-and-Destroy normalization, Maiden's Bay location retirement,
  Schema-61 marker-projection normalization, Schema-62 ownership-authority
  normalization and incomplete-transition resume, and
open-resource reservations. Accepted exact
QRFs never reacquire saved entity IDs. Schema 50 clears process-local root/
member/native-group evidence and keeps one nonterminal batch in strategic hold;
schema 51 applies that same one-owner restore rule to its opted-in enemy QRF and
also clears its physical-arrival sample counters. Confirmed-dead slots remain
retired and only exact survivors can be released for later materialization. Interrupted
pending, `READY_FOR_HANDOFF`, and previously successful physical batches all
normalize to that same virtual authority instead of assuming an interrupted
handoff succeeded or respawning immediately. A post-handoff technical failure
still retains paid money and refunds durable survivors only. Terminal prefab,
verification, handoff, and casualty history remains evidence, while terminal
entity/native-group IDs are cleared rather than reacquired as living authority.

The schema-49 migration from schema 48 creates a contract-version-1 operation
only for a uniquely coherent accepted nonterminal exact paid player QRF. It preserves request,
quote, manifest, queue, group, transaction, and balance state and leaves every
pre-exact, terminal, archived-only, incomplete, or ambiguous row on contract
version `0`. Schema-50 migration opts only coherent exact infantry-QRF
operations into strategic projection. On every restore, a nonterminal opted-in
projection clears process-local entity IDs, becomes one held virtual batch with
strategic position authority, and retains exact survivor/casualty slots, duty,
assignment, economy, and terminal history. Unsupported or ambiguous aggregates
remain operation-only. These rules have deterministic source coverage but no
packaged process-restart evidence.

Schema-51 migration never invents exact authority for a historical enemy order.
Every pre-schema-51 enemy row remains contract version `0` with no backfilled
source claim, manifest, operation, service commitment, or refund. A current-
schema versioned enemy defensive QRF must restore with one coherent reciprocal
order/operation/manifest/batch/group aggregate or fail closed and preserve its
evidence for once-only resource settlement. A complete `PREPARED` terminal graph
may resume through the staged unapplied tuple, refund, applied receipt, and
terminal tail; an arbitrary partial graph may not. Current-provenance `SETTLED`
rows are revalidated after Schema-67 normalization, while mutationless
historical settlements keep their compatibility boundary. Physical runtime
handles are discarded, exact casualty roster and route cursor remain durable,
and an open projection resumes as one held virtual batch. Deterministic
in-memory capture/restore fixtures cover these shapes in source. A guarded
fresh-process matrix additionally proves the three accepted terminal-settlement
prefixes through the canonical JSON fallback; native persistence-source
selection, migration runtime, other restored operation states, physical/world
state, package, and multiplayer remain unproven.

Schema-52 migration never opts a historical active convoy into exact authority.
Every pre-schema-52 convoy remains operation contract version `0`; migration
does not invent a route, manifest, vehicle slot, crew roster, cargo carrier,
convoy element, or settlement receipt. A current-schema open exact convoy must
restore as one coherent mission/operation/manifest/held-batch/three-element
aggregate or fail closed. Restore discards process-local vehicle/group handles
and physical arrival samples while retaining route progress, element positions,
vehicle state, cargo assignment, and confirmed crew casualties, then resumes
the operation virtually. Only member slots may carry casualty tombstones, open
authority must use an enumerated legal duty/resume and materialization/position
pair, and missionless or partially unlinked exact-looking rows remain durable
quarantine evidence instead of being deleted by generic cleanup. This shape is
  source-validated; physical
  rematerialization and real process restart remain packaged-runtime-open.

Schema-54 migration never opts a historical garrison into exact patrol
authority. Policy-v1 accepted purchases, initial-map and enemy aggregates, and
vehicle counts retain their prior representation with no invented operation,
roster, route, batch, casualty, or settlement. A coherent current policy-v2
graph clears process-local handles and resumes one held virtual survivor roster
on the same local loop. A malformed current graph is retained at quarantine
version `-54` without refund, guessed death, or aggregate conversion. Physical
and dematerializing rows join pre-capture root/member/PhysicalWar binding and
live-position validation; capture defers during a partial materialization wave
until its all-required handoff or terminal result is authoritative. This shape
has deterministic source proof and passes the stamped Schema-54 Workbench
compile/open gates. Native movement, fold, save/restart, networking, and JIP
remain packaged-runtime-open.

Normal adapter acquisition runs only after the campaign enters the active phase.
During setup and won/lost phases, the coordinator requests cancellation for all
nonterminal batches and keeps draining dependency-ordered cleanup with a
monotonic runtime-only clock. That cleanup clock advances retry/defer eligibility
without advancing campaign elapsed time. Schema 47 implements successful
survivor reprojection and casualty/retirement authority for the exact paid
infantry-QRF consumer. It confirms physical death only from a present slot-
mapped entity's authoritative life state; deletion or a missing entity is not
casualty evidence. Schema 50 adds virtual roster transfer, direct strategic
movement, materialization hysteresis, and narrow virtual combat for that same
consumer. Schema 51 reuses the exact infantry projection and casualty boundary
  for newly planned enemy defensive QRFs, but not the player-QRF virtual-combat
  policy. Schema 52 adds a separate exact three-vehicle mission-convoy projection
and atomic fold path. Terminal destroyed/captured vehicle roots retain any
living crew as stationary crew-only projections without vehicle resurrection,
and proximity ownership takes the nearest separated living/recoverable root;
  it does not generalize the SpawnQueue adapter or simulate off-screen combat.
  Schema 53 reuses the one-root infantry queue/adapter for newly queued exact
  enemy patrols, but gives them a generated-route loop, contact-held route clock,
  return-origin duty, and proactive survivor-refund policy distinct from both
  QRF consumers. Schema 54 reuses the same adapter only for newly issued
  policy-v2 purchased resistance garrisons, using an empty executable root,
  arbitrary exact member roster, infinite local loop, survivor-only
  rematerialization, and no-refund settlement.
Schema-60 migration never promotes a historical Search-and-Destroy request.
  Pre-60 rows remain contract `0`; current exact-looking rows must validate one
  reciprocal quote/manifest/ledger/request/operation/batch/group graph or retain
  evidence under `-60` quarantine with no inferred balance, casualty, or refund.
The same migration keeps the Logistics Warehouse as the sole enumerable
Maiden's Bay site. No-anchor saves remain untouched and ambiguous authorities
fail closed. Generic references and mutable generated content canonicalize,
while nonzero typed graphs and their frozen generated rows remain unchanged;
deep canonical clones support new generation. Mutable lookup resolves the
warehouse, and detached old-ID lookup plus runtime ID equivalence protects
frozen historical links. The isolated migration proof is compiled and wired to
Campaign Debug but has not been executed; packaged restore/restart remains open.

Schema-61 migration treats map markers as derived projection state. Pre-61
records are cleared and rebuilt from zones, missions, forces, and other durable
owners with fresh revision/stream metadata; migration never infers one of those
owners from an old marker. A current-state projection with invalid epoch,
sequence, revision, tombstone, stable-ID, or stream relationships is also cleared
and rebuilt, but first advances the epoch so a connected client cannot continue
an incompatible stream. The migration and normalization events are idempotent.
The source proof exercises this boundary in memory; serialization and a real
process restart remain packaged-runtime gates.

Schema-62 migration preserves every preexisting owner and other domain fact,
clears any unavailable historical transition rows, and gives each existing zone
contract version `1` with revision `1` and empty active/latest backlinks. It
records `migration_schema62_ownership_transition` once and does not infer or
replay capture support, security, aggression, counterattacks, economy, outcomes,
events, or notifications. Live zone marker rows receive source revision `1`;
later marker rebuild remains derived from zone authority. Current-schema restore
validates the full receipt checklist and reciprocal backlinks, then resumes those
receipts before invalid-owner sanitation. Invalid owners are repaired
sequentially on startup and a five-second retry scan; accepted or transient work
remains deferred, while structural contradictions and an existing quarantined
top-level authority require explicit manual-repair quarantine. Runtime/restore
retries preserve any concrete quarantine cause produced during application;
generic resume text is only a fallback when no such cause exists. Current-
schema normalization rejects forged cause-policy flags, non-pristine later
top-level followers, forged or lifecycle-invalid projection children, duplicate
completed authority claims for the same zone/applied revision, noncanonical
support target sets or applied prefixes, support claims without one exact same-
row deterministic influence event/deltas, counterattack/garrison/reason/event
mismatches, invalid setup-without-markers history, and contradictory zone/receipt/
marker authority. Quarantining current zone authority purges its unsafe marker
rows. It records
`normalization_schema62_ownership_transition_conflict`, keeps evidence under
`-62` quarantine, and never guesses which owner or partial side effect should be
rolled back. Coherent queued pre-owner receipts retain array order; multiple
owner-applied incomplete publishers or an owner-applied publisher behind earlier
unresolved top-level authority are ambiguous. Real serialization,
process restart, native security
settlement, and host/client revision convergence remain packaged gates.

Event-driven physical life-state subscription, generalized
vehicle/asset rosters, generalized folding, live physical engagement events,
and generalization to every force consumer remain open. Paid support is only
partially migrated to this path: player QRF and newly confirmed player Search-
and-Destroy are exact, while supply, roadblock, fire, and air support remain on
legacy services. Historical Search-and-Destroy and policy-v1 garrison purchase
manifests remain nondeployable; initial/enemy aggregate garrisons and garrison
vehicles/multi-root forces remain on legacy PhysicalWar paths.

## World Layout

`Worlds/HST_Everon/HST_Everon.ent` is an original subscene over vanilla
Everon. Named layer files reserve ownership boundaries for physical authoring.
Strategic IDs live in `Configs/HST/Maps/HST_Everon.conf` so persistence does
not depend on fragile entity names.

The checked-in world shells include a base-backed AI world with explicit Eden
soldier and vehicle navmesh configs, a perception manager, faction, loadout,
radio, chat, and respawn managers so Workbench can initialize without relying
on Conflict's strategic brain.

Direct `.ent` Play mode no longer depends on the stock Deployment Setup menu.
The coordinator is an `SCR_BaseGameModeComponent`, so it receives game-mode
state and player-connected callbacks directly. It also runs a short frame sweep
for connected players without controlled pawns, which covers Workbench timing
where player `1` exists before its respawn component is ready. The respawn
system remains as possession plumbing, but its spawn logic is
`HST_PlayerSpawnLogic`, which delegates to `HST_PlayerSpawnService`. The
service registers the connected player as FIA, submits an `SCR_FreeSpawnData`
request for the default FIA rifleman at the selected HQ hideout, tracks the
request as pending, and records player state only when the native spawn
callback reports success. This prevents the Workbench frame sweep from
creating duplicate bodies while Reforger is still finalizing ownership.

`StartingPoints.layer` still contains FIA-affiliated Scenario Framework
spawnpoint slots and FIA role-selection loadouts, but those are now authoring
metadata and a debug fallback, not the normal player-side path. US
remains the occupier in the strategic preset. Game Master-spawned characters
are still not expected to advance Partisan's player lifecycle. Workbench
offline play may log blank identity ID errors from stock reconnect or
editable-entity systems; treat those as non-blocking if a character is spawned
and possessed.

`HST_HQService` owns the server-side HQ lifecycle: setup-driven initial hideout
selection, HQ movement between authored hideouts, Petros/cache/arsenal/tent/spawn-point
runtime positions, and Petros-loss penalties. Runtime Petros spawning tries the
custom Partisan prefab first through its GUID-qualified metadata resource and
falls back to the base FIA character only if that resource cannot spawn. The HQ
arsenal uses a GUID-indexed HST supply-cache prefab whose contextual actions
open the same Arsenal/Loot menu path used by the I-key menu and the custom
loadout editor path, with inherited stock arsenal actions filtered out. A stock
FIA cache fallback is only used if the custom object cannot spawn.

The alpha HQ menu is procedural rather than layout-resource loaded. The server
keeps the existing `HST_MENU`, `TAB`, `STATUS`, `RESULT`, and `ACTION` payload
lines while adding optional `STAT`, `SECTION`, `ROW`, and `FEED` lines for the
Partisan overview, HQ/Petros, missions, map/war, forces, arsenal/loot,
garage/build, members, and admin panels. The custom loadout editor has its own
`HST_LOADOUT_EDITOR` and `HST_LOADOUT_CANDIDATES` payloads but still routes
mutations through the same server-authoritative request bridge. Contextual
Petros, HQ arsenal, and vehicle cargo actions call the same bridge as menu
clicks so local hosts and MP clients follow one command path.

## Campaign Framework Spine

The first campaign loop is now connected as a physical/abstract hybrid. Zones
carry type, position, income, support, activation radius, route IDs, mission
site IDs, and garrison-slot data in `HST_CampaignState`; garrisons are stored
as infantry and vehicle counts. The physical-war service marks zones active
when players enter their activation radius, converts abstract garrison counts
into route-aware active groups, and folds survivor counts back before
deactivation. A mixed group with previously observed personnel becomes terminal
when its living infantry reaches zero after delayed-population and live-count
grace, even if its attached vehicle remains intact. That vehicle becomes
unclaimed, detached salvage and no longer contributes to garrison,
capture, QRF, or marker strength; vehicle-only projections keep their existing
separate semantics. Existing durable field ownership/cargo is preserved; an
unadopted salvage record is session-only. Eliminated/spawn-failed save normalization preserves zero
counts and clears process-local spawn identity rather than reconstructing
original force totals; folded rows retain the survivor counts already returned
to the abstract garrison.
Broad-alpha services generate mission sites/routes, attach
objectives/tasks to started missions, poll physical MVP mission primitives from
world conditions, spend scaled enemy pools into orders/support calls, and let
orders/support either physicalize near players or resolve abstractly off-screen.
The schema-50 exception is the newly confirmed exact paid infantry QRF: its
single operation and frozen member roster move continuously through virtual,
materializing, physical, dematerializing, and virtual states rather than using a
one-shot abstract outcome. Schema 51 applies the same projection ownership to
new infantry-only enemy defensive-QRF orders, with a distinct source/target,
arrival-pressure, return, and resource-settlement policy. Schema 52 gives only
new mission convoys their own generated-route virtual cursor and exact
three-element physical projection; it does not opt historical convoys or other
  missions in. Schema 53 opts only newly queued patrol orders into one generated-
  route outbound/lap/return policy; historical patrols remain legacy. Schema 54
  opts only newly issued policy-v2 purchased resistance garrisons into an exact
  indefinitely looping local patrol and leaves historical/initial/enemy aggregate
  garrisons legacy. Schema 60 separately opts only newly confirmed player Search-
  and-Destroy into direct strategic travel, exact survivor projection, bounded
  virtual combat, return-to-assignment, and commander-recall settlement. These
  narrow paths remain intentionally isolated from broad legacy support, non-
  convoy missions, and every other enemy order family.
HQ knowledge feeds HQ threat, Defend Petros, markers, and campaign-end pressure;
civilian town support and undercover enforcement feed wanted heat, roadblock and
police scans, and HQ exposure. Civilian render eligibility is separate from the
hostile military activation bit so HQ safety does not erase nearby town life.
Schema-59 radio sites borrow one uniquely resolved authored transmitter and
route destroy/rebuild missions through their durable lifecycle owner; generic
composition and mission runtime cannot create or delete that projection. Loot,
vehicle cargo,
virtual garage, build
placement, vehicle capability classification, and loadout editor systems are
owned by campaign state instead of stock arsenal behavior. Mission success,
failure, timeout, convoy outcome, support resolution, enemy orders, vehicle/cargo
paths, and civilian aid mutate economy, support, capture progress, arsenal,
garage, aggression, HQ threat, and victory/loss readiness. Coordinator hooks
expose deterministic server-only actions for Workbench tests and no-admin player
actions for setup, random missions, support requests/cancel, civilian aid,
undercover status/checks, looting, vehicle cargo, garage capture/redeploy,
loadout application, marker/command audits, balance/campaign-end reports,
income, training, recruitment, and HQ moves.
