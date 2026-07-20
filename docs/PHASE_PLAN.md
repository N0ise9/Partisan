# Partisan Phase Plan

> Release work is now gate-driven. [`CURRENT_STATUS.md`](CURRENT_STATUS.md)
> carries the generated current decision and proof ladder;
> [`ANTISTASI_CE311_PARITY_MATRIX.md`](ANTISTASI_CE311_PARITY_MATRIX.md) carries
> the generated CE 3.11.1 behavior contract. Numbered historical phases below
> remain implementation context rather than certification state.

Campaign Schema 71 and runtime-settings Schema 24 are current. The current
sealed implementation/source identity is
`32727238d74b29905c68e5a80bb5897dfdc783c0`, UTC `2026-07-18T16:34:38Z`, label
`schema71-settings24-focused-force-authority`.
This stamp adds the focused force-authority checkpoint and retains the passing
mixed-native work described below.

## Release-Closure Gate Order

The active plan is no longer feature-phase expansion. Work proceeds in this
dependency order, with at most two runtime families in flight:

1. Gate 0: generated current truth, pinned CE 3.11.1 behavior contracts, schema
   freeze, and explicit unsupported surfaces.
2. Gate 1: build once, retain one immutable package, bind every evidence rung
   to its Git/package/toolchain identity, and rerun the current suite.
3. Gates 2-6: immediate UI regressions, one exact force runtime, one client
   view, one strategic/town truth, and one autonomous enemy campaign.
4. Gates 7-10: mission-by-mission behavior, resistance progression, full-graph
   persistence/fault tolerance, and complete-campaign balance.
5. Gate 11: cumulative canary and stable certification on the unchanged
   artifact.

`CURRENT_STATUS.md` declares the active release decision; this file retains the
detailed implementation order and historical rationale.

## Release-Ledger Migration and Activation Plan

Release-ledger Schema 3 does not alter Campaign Schema 71 or runtime-settings
Schema 24. `historicalCandidateEvidence` is now a one-or-more JSON array whose
declared order is oldest to newest. Each entry has exactly
`retirementDisposition`, `candidate`, and `evidence`. The checked ledger now
retains `partisan-rc-0e632ec4f63e-20260719T004133Z` as ordered `history[0]` with
`rejected-after-full-profile` and
`partisan-rc-e11e7ea88a44-20260719T040154Z` as ordered `history[1]` with
`rejected-after-corrected-canary`.

The ledger admits two retirement paths. `rejected-after-full-profile` requires
focused, accepted corrected-canary, and rejected full-profile evidence.
`rejected-after-corrected-canary` requires focused plus rejected corrected-
canary evidence and forbids `fullCampaignDebug`; a stopped full run must stay
absent rather than becoming null, placeholder, fabricated, or transferred
evidence. Each path must exact-rehash and correlate its own candidate,
manifest, ready seal, package, summaries, envelopes, and run identities.
Candidate identity keys and identity-defining hashes must be unique across the
ordered history and current artifact, while timestamps and Git ancestry must
prove the same oldest-to-newest gate sequence.

The activation completed as one fail-closed ledger operation: it validated the
new immutable current candidate, appended e11 exactly once, kept its full
property absent, replaced all current artifact/evidence/rung fields, and
regenerated the checked status without an observable mixed state. The new ee0
candidate must now earn its own package-bound chain. Release stays `NO-GO`.

## Gate 1 Build-Once Plan

The guarded release-candidate entry point now defines the Gate-1 artifact
boundary. It fails closed unless the checkout is clean, records Foundation,
runs and retains raw evidence for explicit PC, XBOX_ONE, XBOX_SERIES, PS4, and
PS5 Workbench validation, rechecks that HEAD and the worktree did not change,
and only then performs the native Workbench pack.

The pack uses a fresh nonce-owned checkout-local scratch directory but keeps the
candidate outside the checkout. The external candidate begins under a unique
partial identity. Exactly four native packed files are admitted, including the
tracked thumbnail; source and packed project identity must match, the thumbnail
bytes must match tracked source, and a sorted `sha256-manifest-v1` index
produces one aggregate package digest. The manifest then binds source, embedded stamp,
schemas, addon, toolchain, every target result, package, and retained evidence.
Only a generated-and-checked manifest permits the partial directory to become a
final candidate. The manifest is checked again after that move, and a matching
ready seal is written atomically last; without it, the directory is not a
published candidate.

### Pending Schema-2 Runtime-Evidence Sequence

Before the next candidate begins runtime promotion, the focused aggregate
producer, corrected-canary release-index producer, guarded runners, candidate
module, and release-doc consumer must form one stationary committed tool set.
Their producer self-tests, consumer self-tests, and Foundation checks must pass
against that same checkout. This is a pending static/tooling checkpoint only:
it does not alter Campaign Schema 71 or runtime-settings Schema 24, execute a
packaged runtime, prove runtime behavior, or advance Gate 1 or any later gate.

After that checkpoint, Gate 1 proceeds in this exact order:

1. Build and retain one new immutable candidate from the stationary checkout.
2. Run the five individually named packaged focused profiles serially against
   that exact candidate. Retain eight raw envelope files per profile outside
   the checkout, publish one Schema-2 aggregate over the exact five-run,
   40-file set, and have the release-doc consumer independently reopen and
   rehash that raw set. Its 35/35 `aggregate-policy` assertions prove only the
   aggregate admission contract; they are not Campaign Debug assertions or
   gameplay/runtime proof. Producer and consumer must also rederive the exact
   four-file package digest, reject additive suite/pass markers, and require all
   four candidate seal fields on any replacement receipt.
3. Only after the focused aggregate is accepted, run the corrected canary
   against the unchanged candidate and retain its ten-file raw bundle. Its
   current acceptance contract is exactly 9 PASS, 1 WARN, 0 FAIL, 1 BLOCKED,
   and 0 SKIPPED across 11 cases; 91 ordered assertion rows; 87/87 certifying
   rows; only the non-certifying `cleanup.player_marker.live` warning; only the
   explicitly later-external non-certifying `isolation.world_scope` blocker
   under `cleanup.state_isolation_restore`; and the exact 18-label zero-delta
   state set. Any unexpected or certification-counting blocker is red. Publish
   a Schema-2 release index and require the consumer to reopen and rederive the
   result from the raw bundle.
4. Run Full Campaign Debug only if that corrected-canary index is accepted.

All tracked Schema-1 summaries remain immutable historical evidence and remain
readable for their original packages. They are never rewritten, upgraded, or
transferred to the new candidate. A rejected replacement attempt must retain
candidate-bound red evidence without overwriting accepted bytes.
Historical routing follows the retained summary schema. Later tool maintenance
may advance the checkout without invalidating accepted Schema-2 history, whose
recorded immutable blobs and ancestry remain authoritative; active evidence
still requires stationary current bytes.

The boundary first retained candidate
`partisan-rc-c2b16c4a2d85-20260718T201442Z` from clean source HEAD
`c2b16c4a2d85e71503cd46265feafb54bce69e83`, with aggregate package SHA-256
`8f60260331c6c7473465dc4517b1063a179a8f4efeffdcfe3d5eccac9af476db`.
That artifact remains sealed but is superseded for runtime use. The first
replacement was `partisan-rc-b8deddc4b631-20260718T213322Z`, built from
clean source HEAD `b8deddc4b6314936b7ea04f36a35784622a46da6`, with aggregate
package SHA-256
`82e1fd0bf7c3404b7fe842fa84efd10f225bf82fc76c11502b9a684b63f4f329`
and all five Workbench targets at common CRC `f27e637b`. Its focused and
preliminary-unaccepted canary/full results remain historical evidence for that
package only.

The retained `rejected-after-runtime` candidate is
`partisan-rc-ee0e8add2a29-20260719T063815Z`, version
`0.1.0-rc.20260719T063815Z.ee0e8add`, built from clean source HEAD
`ee0e8add2a298e83fd304b7660c4fc480dc6383f`, with exact four-file package
SHA-256 `981258439b9d08866c4883471cacfe33aa373a36a667a39e8c939f285db74daf`,
manifest SHA-256
`1b877e3aa21773a268704bcb3fe889768fca3aa2d78541aa7285b061398ce907`,
and ready-seal SHA-256
`01741b85d0edba69f54b07388cdd7c452b8f6f1ad7ef4f6faf253918a4bbf280`.
Foundation passed all 874 checks. All five Workbench targets passed at 5,848
files/11,901 classes with common CRC `f64e0868`. The seal binds four package
files and 50 evidence files. At approximately `2026-07-19T07:02Z`, clean
harness HEAD `273ed14ba8526259c8b0d248177fa53b59ade683` passed the five
canonical packaged focused cases against the exact candidate and mount: JUnit
5/0/0/0, 40 retained envelope files, 12 classifier checks per run, 11 approved
diagnostics, and zero cleanup/spill residue. Deterministic-service is
`passed-noncertifying`. At approximately `2026-07-19T07:14Z`, clean harness
HEAD `4f8d7e2d7a39896737fd6754060523bf852c5fa8` accepted corrected canary
`seed1985_t0_p1_u1784445266` against the same unchanged candidate/package. The
11 cases ended 9 PASS/1 WARN/0 FAIL/1 BLOCKED/0 SKIPPED; all 35/35 focused and
87/87 certification-counting assertions were proven; state restoration was
18/0; ten files rehashed; the census was two approved stock plus zero unapproved
diagnostics; and final orphan cleanup plus every cleanup/spill boundary passed
at zero. Scoped native-engine/world is `passed-noncertifying`. Clean harness
HEAD `a5ccf36aee17a4f88d7f1c2f232ce9fc14652018` then ran Full Campaign Debug
against the exact unchanged package. Run `seed1985_t0_p1_u1784446076`, leaf
`20260719T072739Z-97fc069d58cd427c848c83f99f39e5f9`, retained an exact
ten-file capture with envelope SHA-256
`fce4928444f15531f254ad4d7e119cf8bfe1d06e6fcb564518d2e052544d4278`, 18/0
restoration, Phase 17 at 11/11, Phase 24 at 2/2, staged cleanup at 6/6, zero
final orphans, and zero residue. The full profile is
independently rejected red at 598 PASS/47 WARN/26 FAIL/13 BLOCKED/1 SKIPPED
across 685 cases and 5,630/5,695 required assertions. Its 26 hard diagnostics
are two approved stock + zero approved intentional + 24 unapproved (22 Partisan
and two runtime): an
obsolete fourteenth intentional-convoy expectation demoted 13 valid intentional
negatives, alongside nine debug respawn-race errors and two HQ arsenal teardown
errors. Certification and diagnostic acceptance both fail. Keep the candidate
immutable, require a new candidate for source fixes, and keep release `NO-GO`.

Post-rejection source commit `12f87e9` removes that obsolete classifier row for
future candidates and binds the remaining set as nine admission, three
corruption, and one watchdog diagnostic. It also caches the exact HQ arsenal
prefab identity before teardown so the null-catalog early return cannot depend
on component discovery during `OnDelete`. The expanded 36-check classifier and
teardown shield are source-fixed but not package-proven; ee0 keeps its original
33-check rejected evidence unchanged.

Commits `64d1f70` and `ebaaeca` then repair two deterministic/runtime product
clusters for the next candidate. A compacted paid-support confirmation now
replays from its sealed tombstone before live planning-service dependencies are
required. Convoy-contact reseating now has one bounded exception for degraded or
restored groups with real roots and living unseated crew, on the existing
five-second cadence and within the 45-second seating grace. Both remain pending
Workbench and package-bound runtime proof.

The historical e11 candidate's exact package-bound focused set passed under clean harness
`b1940f2` from `2026-07-19T04:44:01.2295133Z` through
`2026-07-19T04:45:58.8756237Z`: all five cases passed with JUnit 5/0/0/0, all 40
envelope files rehashed, cleanup/spill residue was zero, and the diagnostic
census was 11 = ten approved stock + one approved intentional + zero unapproved.
The deterministic-service rung is accepted as `passed-noncertifying`; the
corrected `force_authority` canary then failed the native-engine/world rung.
Clean harness HEAD `937c86c5d2259a9da270ea76371001ac1d4c6eed` retained run
`seed1985_t0_p1_u1784437399`, leaf
`20260719T050302Z-0bbd740b4f0149baa0f34944dbd70fc9`, from
`2026-07-19T05:03:02.0611638Z` through
`2026-07-19T05:03:41.5393020Z`. Package identity, packed mount, bytes, and
artifacts were exact and stable, but the 11 cases ended 8 PASS/1 WARN/1 FAIL/1
BLOCKED/0 SKIPPED. Focused proof was only 33/35 and 85/87 because
`ownership_transition.aggregate` and `ownership_transition.causes` failed.
State restoration was 18/0, final orphan cleanup passed, all ten envelope files
rehashed, cleanup/spill residue was zero, and the valid census was two approved
stock diagnostics plus zero unapproved. The rejected summary SHA-256 is
`af0aca25a84d8f757dbba8010950a658ce09937aa4048c35b2e372f1183eec69`.
The full profile was correctly stopped. Never borrow another package's result.

The historical failure root is a stale proof fixture after the intentional production
`FindActiveMission` guard: the fixture supplied a mission source ID without
seeding the corresponding active mission. The source correction seeds exact
missions in both affected fixtures, pins mission cause/type/ID, retains an
unresolved source as a military-capture negative case, and splits political and
mission diagnostics. That source change is sealed in the retained ee0 candidate,
whose fresh focused rung now passes; it cannot repair the historical e11 package.
The historical 35/35 and 87/87 state-only result is nonpackage and did not
advance ee0. Release remains `NO-GO`.

The older `partisan-rc-0e632ec4f63e-20260719T004133Z` remains
immutable historical evidence. It was built from clean source HEAD
`0e632ec4f63eab43e8c301d0755f10193d85131f` with exact package SHA-256
`e5d29458c33aeef9cd2b37476359acc6021fe78cf0fc74513d9a2f69ef0614dc`.
Its all-five focused set passed with JUnit 5/0/0/0, 40 rehashed envelope files,
zero cleanup/spill residue, and 11 diagnostics = ten approved stock + one
approved intentional + zero unapproved. Its corrected `force_authority` canary
then passed at 9/1/0/1/0, 35/35 and 87/87 focused proof, 18/0 restoration, two
approved stock diagnostics, zero unapproved diagnostics, ten rehashed files,
and zero cleanup/spill residue. The subsequent full profile is immutable
rejected red evidence at 584/49/46/7/1 and 5,561/5,687, with 112 failed and 14
blocked required assertions plus ten unapproved hard diagnostics. None of these
historical results transfers to retained ee0 or advances its fresh evidence chain.
Candidate-aware Campaign Debug and focused-runner preflights now validate the
tracked and external manifest/ready seal, stage and rehash the exact four-file
package under a disposable guard, construct the packed launch vector, and
return every cleanup boundary to zero. A seventeen-check consumer suite admits
all three dispositions for verification, admits only `active-runtime-candidate`
for runtime use, and rejects the exercised tamper and layout cases. Real
runs also recheck the clean harness, bind the guarded settings copy where
applicable, and require the engine-owned exact-path packed-project mount record.
The focused-runner audit found that three service-only suites in the first
package could be dropped by the stock base-only world transition before JUnit
output; the source now enforces the same empty-world
override as the two working suites.

The prior candidate's first all-five execution produced passing JUnit and exact
package-mount evidence but remains preliminary because hard diagnostics did not
participate in
the runner's pass predicate. The harness now accepts only the exact stock
post-result pair plus the profile-journal case's proven non-mutating fault
injection. The repeated classifier-aware set is now accepted: all five exact
packed runs passed with JUnit 5/0/0/0, 40/40 envelope files rehashed, zero
cleanup/spill residue, and 11 classified hard diagnostics consisting of ten
approved stock messages, one approved intentional journal injection, and zero
unapproved errors. The first five sidecars remain preliminary.

The prior candidate's original guarded `force_authority` canary and Full Campaign
Debug produced mechanically complete, stable raw captures against that exact,
then-active replacement,
but both remain preliminary-unaccepted. The original canary census is three raw
diagnostics = two exact approved stock + one unapproved. The full run has 25 raw
diagnostics, including a 19-line Partisan subset; two are exact approved stock,
13 are proof-bound intentional, and ten are unapproved. Its report remains
failed at 584 PASS, 49 WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED, with
5,562/5,688 required assertions proven, 112 failed, and 14 blocked.

Clean committed classifier harness `38a094f` then reran the canary against the
unchanged package. Its focused proof passed 35/35 and 87/87, but the exact census
again contained three = two approved stock + one unapproved map-locator VM
exception. The runner failed closed with zero cleanup/spill residue, and the
full profile was correctly not rerun. The reproduced root is a `PLAIN` map
opened before bootstrap supplies a controlled character: the immediate
no-player locator update removes the hint root but leaves a ten-second callback
that later reaches stale widget state.

The narrow lifecycle correction validates the map-locator layout, both text
widgets, and world directions before entering stock calculation; invalid state
removes the remaining layout, cancels the callback, clears references, and
returns. Valid stock behavior remains unchanged. Foundation and PC Workbench
compile validation passed at 5,847 files/11,900 classes and CRC `3a399db1`, with
zero errors or residue. The correction was sealed in the now-superseded
`partisan-rc-0e632ec4f63e-20260719T004133Z`. Its five packaged focused cases and
corrected scoped canary passed as non-certifying evidence; the formerly observed
exception was absent from that canary path. Its full profile is immutable
rejected red history: 584/49/46/7/1, 5,561/5,687 required assertions, and ten
unapproved hard diagnostics despite mechanically valid wrapper capture.
Post-capture corrections were sealed in the historical e11 replacement. Its exact
packed five-case focused rung is accepted as non-certifying, but its corrected
canary failed on the stale ownership-transition fixtures at 33/35 and 85/87;
the full profile was stopped. The fixture correction is sealed in retained ee0;
its five canonical packaged focused cases and corrected canary pass as
noncertifying evidence. Its independently executed full profile is rejected red
at 598/47/26/13/1 and 5,630/5,695 with 24 unapproved diagnostics.
Dedicated,
multiplayer/JIP, restart breadth, performance, and soak remain later independent
gates, and release remains `NO-GO`.

## Historical Focused Force-Authority Plan Checkpoint

The historical non-package deterministic state-only gate for combat presence,
ownership transition, and town influence passed: the dedicated focused engine
case proved 35/35 targeted assertions, 87/87 counted conditions, and an 18/0
state diff with zero errors, crashes, or artifact drift and exact-zero owned
cleanup. Its Foundation gate passed at 874; stamped Workbench passed 5,846
files/11,899 classes at CRC `cad640f3` with zero hard errors.

The retained prior-package Full Campaign Debug run provides a preliminary
diagnostic capture, not accepted integrated evidence. Its red 46/7 case boundary
shows that the focused observations did not certify the integrated owners, while
the ten unapproved diagnostics independently block acceptance. The corrected
canary already failed closed on the separate map-locator lifecycle exception.
The later, now-superseded package passed all five focused cases and its corrected
scoped canary, then produced its own rejected full result rather than reusing the
retained prior-package result. The historical e11 replacement cannot reuse either
package's evidence; its own focused set passed, but its corrected canary was
rejected on stale source fixtures and the full profile was stopped. The fixture
correction is sealed in retained ee0. Its five canonical packaged focused cases
and corrected canary pass as noncertifying evidence; the unchanged package's
full run is a mechanically exact rejected boundary at 598/47/26/13/1 and
5,630/5,695 with 24 unapproved diagnostics. Next work triages those clusters in
source and builds a new candidate before any rerun.
Later work broadens into native-world effects, real
persistence/restart, rendered client projection, packaged server/client,
multiplayer/network/JIP/reconnect, performance, and soak.

## Current Controlled-Shutdown Native Fence Plan

The controlled-end bridge now uses one ordered native-authority transaction.
Complete read-only readiness checks precede every one-way retention latch; state
preparation is followed by a second complete loot/rescue/field/active preflight
before the first subsystem latch. While draining, the coordinator rejects new
command, mission, casualty, member, and lifecycle ingress. Retry preserves the
latched scope, reapplies quiescence, and rejects native identity, carrier-seat/
player, damage, token, or foreign-occupancy drift before shutdown persistence.

This fence is process-local and changes neither Campaign Schema 71 nor runtime-
settings Schema 24. The ordinary five-process proof passes autosave, manual,
shutdown, native verification, and profile-fallback verification with
generations 1 -> 2 -> 3, exact controlled-end bridge and field-vehicle state,
read-only restore stages, and zero cleanup. Treat it as a scoped bridge/field
regression, not proof of every native shutdown topology.

The sealed checkpoint extends that same five-stage chain with the
dedicated mixed-native graph. It proves exact following, seatless-boarding, and
boarded captive states; player release and foreign-occupant rejection; durable
carrier binding independent of the restored process's `RplId`; stable authored
seat identity; and exact active-group/guard rematerialization through both
native and profile-fallback restart. Every stage and cleanup counter passes.
Next broaden from this fixed graph to arbitrary rescue, mission, carrier, and
force combinations while retaining Workshop/live multiplayer, JIP/reconnect,
abrupt-termination, and soak as certification gates.

## Current Exact Enemy-Garrison Rebuild Fresh-Process Restart Plan

The exact rebuild JSON restart subgate is closed by three fresh packaged
dedicated-server processes across `prepare -> recover -> replay`. Prepare cuts
`delivery_pending` at `225/300m` with nine accepted manifest slots, eight living
slots, one confirmed casualty, and prepared digest `6fdb9d3e45a08b92`.
Recover restores that exact semantic fingerprint, advances and delivers exactly
once, holds the eight surviving slots
without aggregate double count, and records the zero-refund delivered
receipt/resource mutation exactly once, producing delivered digest
`16b17e6617027292`. Replay restores that exact delivered semantic fingerprint as
a semantic no-op.

The journal slots and proof carrier are byte-read-only during replay, the
persisted chain is canonical-slot generation 1 to recovery-slot generation 2,
all stage exits are `0`, and cleanup is zero. An earlier focused CLI attempt lost
the requested test type during stock reload and emitted no JUnit result. The
superseded `partisan-rc-0e632ec4f63e-20260719T004133Z` candidate's later
`HST_TEST_EnemyGarrisonRebuildAuthority` case emitted one passing JUnit testcase
inside its accepted five-case set, with valid diagnostic classification and zero
cleanup/spill residue. This is immutable historical focused service proof only
and does not widen the fresh-process restart boundary or transfer to the active
package.

The companion `physical_live_fold` cut begins with the same nine accepted slots,
eight living slots, one prior casualty, and `225/300m` route state. Production
authority crosses `VIRTUAL -> MATERIALIZING -> PHYSICAL/LIVE` with one native
root, nine adapter handles, and eight living runtime members. The root moves
2.759m and closes 0.539m before the production
`PHYSICAL -> DEMATERIALIZING -> VIRTUAL` fold returns the exact eight survivors
and one casualty to virtual authority with no runtime residue. Generation-1
persistence, generation-2 recovery, and read-only/no-op replay all pass with
cleanup zero.

The exact fixture's delivery-pending and physical-live-fold restart cuts are
therefore closed, including scoped native handoff, measured movement, production
fold, and survivor continuity. Next prove natural full-route travel/combat,
other force families, multiplayer/JIP/reconnect, and soak. The separate
mixed-native controlled-shutdown fixture is now closed at its scoped five-stage
boundary, but neither fixture establishes broad runtime parity.

## Current Campaign Recovery Journal Plan

The two-generation profile recovery boundary is implemented. Canonical and
recovery slots hold the exact serialized campaign payload inside envelopes with
schema, generation, parent generation/fingerprint, and
`uuidv8-sha256-v1:<serialized-length>:<UUID>` fingerprint. The writer alternates
into the inactive slot, reads it back, validates it, and re-runs selection before
advancing. It preserves the previously selected valid generation until that
replacement verifies. A raw Schema-70-or-earlier canonical save is generation
0; the first current checkpoint writes recovery generation 1 and retains the
raw bytes.

Campaign Schema 71 persists monotonic checkpoint sequence, restore sequence,
and last-save second for native/journal ordering. Equal order requires matching
comparable normalized payload fingerprints. The newer valid source wins. No
native source uses the journal; unusable native data can enter explicit degraded
journal recovery and then keeps the session profile-only. With no valid native
source, present invalid journal artifacts are fatal; only absence of both
sources creates a new campaign. Unsupported-future native state is fatal before
journal routing. Split brain, broken links between otherwise valid generations,
future/ambiguous envelopes or payloads, duplicate-metadata conflict, and equal-
order fingerprint disagreement fail closed as journal authority. Valid native
startup is read-only when journal
artifacts are invalid or future. A later verified native checkpoint may repair
an ordinary invalid/truncated inactive slot; unsupported-future or ambiguous/
conflicting history remains write-fenced.

Native envelope version 2 stores the exact serialized payload string and
validates its fingerprint before DTO parsing. Structured Schema-70 version-1
native rows reconstruct and validate their legacy fingerprint before normalization.
Admin reset retains and advances the old checkpoint/restore sequence floors,
prepares one detached prospective DTO, and commits it synchronously to the
verified journal before retiring old runtime roots or staging native state.
That write-ahead generation is the irreversible reset boundary; later native
failure becomes degraded replica repair and cannot resurrect the old campaign.
Ordinary native-active JSON still advances after the matching successful
callback, fallback-only writes remain synchronous, and ordinary native failure
writes no new JSON.

The journal is crash-tolerant, not an atomic filesystem transaction. It assumes
one writer and provides no lock, atomic rename, authenticated storage, or off-
device backup. It recovers only the last completed native or journal generation.

The retained journal/shutdown stamped evidence passes Foundation at 874 references and Workbench at
5,846 files/11,899 classes and CRC `9a79a33a`, with zero hard errors and cleanup
residue. The focused journal testcase passes 1/1 and 41/41 exact booleans, with
native-load v1/v2/bad/future at 1/1/1/1 and an empty failed list. The
ordinary five-process chain passes all five stages: generations advance
1 -> 2 -> 3, canonical generation 3 is selected, both slots are valid with an
exact chain, native and profile-fallback restores remain read-only, exact field-
vehicle continuity is retained, and cleanup is zero. The native-versus-stale-
journal counterattack chain passes 3/3 with native selected, both journal files
unchanged, an exact chain, and zero cleanup.

The same sealed mixed-native checkpoint also passes all five
ordinary stages. Its captive/carrier/player/seat graph, durable carrier rebind,
stable seat token, guard rematerialization, restart/fallback fingerprints, and
cleanup remain exact.

The admin-reset chain passes 3/3. It advances journal generations 1 -> 2 -> 3,
selects the newer generation-3 reset journal over deliberately stale native
authority in a read-only final process, preserves the exact two-slot chain and
proof carrier, rejects an overlapping reset without mutation, and leaves
cleanup at zero.

The next persistence work is broader arbitrary-save migration and active-world
coverage. Multi-writer exclusion, off-device backup, Workshop/live clients,
network/JIP/reconnect, marker behavior, performance, soak, abrupt termination
beyond the last completed checkpoint, and unrelated Campaign Debug failures
remain open.

## Preceding Durable Field-Vehicle Restart Plan

The current persistence phase now includes one complete durable field-vehicle
vertical slice. HST campaign rows are the sole authority; Track and capture
detach native entity persistence with `StopTracking(true)`. Capture validates
stable ID, nonempty normalized full prefab and exact binding match, full 3D
position plus normalized upright yaw, active/tombstone binding, and abstract
cargo. Duplicate prefab values across rows remain valid. Destroyed unoccupied
roots become tombstones and their wrecks are removed, while living player
occupancy blocks destructive cleanup.

Startup restores this graph before normal capture or gameplay publication and
does not complete bootstrap until the detailed receipt is `AllExact`. Legacy
native-tombstone retirement is restricted to one unoccupied native-tracked
exact-full-prefab candidate within 3 meters and 3 degrees. Ambiguous or parent-
tracked candidates fail closed. Blocking shutdown maintains saved transforms,
stops controller/engine state, applies supported persistent brakes, and
deactivates hierarchical dynamic physics until commit.

The acceptance chain is complete for the scoped fixture. Prepare writes two
S1203 rows with abstract cargo counts 3/7. Fresh manual recovery spawns both,
moves A, destroys B through engine damage, and captures one live row plus one
tombstone. Shutdown and both no-save verification processes restore only A
while retaining B's tombstone, the exact serialized position/yaw/cargo graph,
and tolerance-verified physical placement. Every post-prepare
restore reports `adopted=0`, `retired-native=0`, exact spawned counts, and zero
native-tracked durable roots; shutdown proves its one live root remains
controller/physics-quiesced through commit.

All five processes exit `0`: periodic `AUTO` at tick 1,802 and
60.018852233886719 seconds (repeat dirty mark 30.016357421875 seconds), typed
`MANUAL`, blocking `SHUTDOWN`, native no-save verify, and profile-fallback no-
save verify. Foundation passes 839. The stamped Workbench compile passes 5,837
files/11,850 classes at CRC `37604e5a` with zero errors/residue.

The current recovery-journal plan above closes the former single-overwrite gap
with alternating verified generations. It is deliberately described as crash-
tolerant rather than atomic.

After that boundary, expand durable vehicle parity deliberately: fuel, partial
damage, attachments, physical trunk contents, arbitrary supported vehicles,
and a world-wide duplicate census. Workshop/live-client, multiplayer/JIP/
reconnect, performance, and soak gates remain required. The current proof's
duplicate census is limited to expected fixture positions and must not be
promoted to those broader claims.

## Preceding Periodic Autosave And Controlled Persistence Plan

The campaign DTO now travels through an engine-owned `PersistentState` proxy
with a versioned envelope and exact snapshot fingerprint. Both mission headers
install the server persistence system and enable the configured save types. An
unarmed proxy fails serialization, so startup cannot produce a valid-looking
save point with the campaign row omitted.

Startup waits for the persistence system behind a bounded 120-second fail-closed
gate. A valid loaded native record is authoritative before the profile fallback;
an invalid native record is fatal. A loaded save missing the campaign row may use
a valid older profile fallback for migration, but missing both records is fatal
instead of resetting the campaign. Only a genuinely fresh session with neither
source creates a new campaign.

Production checkpoint requests now carry explicit `AUTO`, `MANUAL`, or
`SHUTDOWN` intent. Native durability requires the matching completion callback;
only then is the exact pending snapshot mirrored to the profile fallback. A
failed native commit leaves the prior native/fallback pair intact and re-arms
checkpoint intent. Native-unavailable operation still uses the fallback
synchronously.

The production tick now advances independent periodic `AUTO` and first-edge
major-change debounce clocks. A repeat dirty mark coalesces without extending
the major interval. Accepted full-state checkpoints cover both lanes, rejected
major requests preserve periodic progress, rejected periodic requests back off
by the configured debounce, and in-flight work suppresses competitors while
both clocks continue. Scheduler receipts, attempt sequence, and clocks are
process-local; Campaign Schema 70 and settings Schema 24 do not change.

The controlled game-end bridge disables controls, drains only an already-
pending checkpoint without generating a new automatic or scripted request,
freezes mutation, requires a stable campaign fingerprint, and issues an exact
`SHUTDOWN` checkpoint with `BLOCKING=1`. Its retry window is 270 seconds. Only
successful completion continues the end transition. The retention handler owns
the committed campaign save and suppresses stock purge of that save while
`keepSessionSave=false` and the retention CLI override is absent.

The final guarded stamped run passes five fresh processes: production-tick
periodic `AUTO`, typed `MANUAL`, real `EndGameMode` `SHUTDOWN`, native restart,
and profile-fallback restart. Periodic AUTO reports `periodic_autosave` at tick
1800/60.020751953125 seconds; a repeat dirty mark at
30.020465850830082 seconds does not extend the configured 120-second first-edge
debounce. The run proves exact checkpoint UUID continuity, flags `0/0/1`, both
restore sources without verification saves, all five stage exits `0`, and zero
owned external residue. The deterministic source harness covers rejection
retry/fairness and in-flight suppression, but there is no separate live
SCRIPTED-at-debounce/rejection stage. Abrupt operating-system or service
termination can retain only the last completed checkpoint. Broader active-world
records, Workshop/live clients, networking/JIP/reconnect, migration, markers,
performance, and soak remain open. Durable endpoint ABA snapshots remain a
separate future contract-2 schema decision whose schema number is not yet
assigned.

## Preceding Counterattack Endpoint Owner/Claimant Restart Plan

This remains a schema-neutral and settings-neutral Blueprint Phase-9 proof
checkpoint. The guarded harness now owns seven counterattack cuts, each split
across fresh `prepare`, `recover`, and `replay` processes. Outbound `VIRTUAL`
remains the successful durable baseline. Raw `DEMATERIALIZING`/`LIVE` keeps its
exact `N-1` casualty tombstone, while raw `MATERIALIZING`/`STRATEGIC` keeps its
pre-handoff authority. Both transitional cuts must defer production capture and
recover from the unchanged canonical `VIRTUAL` fallback; neither raw graph is a
successful durable checkpoint.

The fourth cut exercises production materialization into genuine native
counterattack `PHYSICAL`/`LIVE` authority. It requires the real native root and
living members, exact SpawnAdapter/PhysicalWar bindings, and an independent
native live-position sample before durable corruption and after production
persistence. The live graph, normalized held-`VIRTUAL` readback, carrier, and
result must agree with that oracle, preserve the living roster, and clean every
runtime claimant before process exit.

The remaining three cuts stage N-1 one-pool survivor settlement before refund,
after refund/before receipt, and after receipt/before terminal finalization.
Recovery consumes each prefix exactly once, retains one debit and one
proportional refund, removes terminal batch/group claimants, and proves explicit
reconcile plus second-start no-ops.

Both carrier families freeze source/target owner faction and ownership revision.
Every cut requires unique endpoint rows and zero ownership-transition claimants
correlated by either canonical counterattack request ID or operation source ID.
The complete validators reject a source-revision mutation and both claimant-
identity forgeries; the physical cut additionally proves the positive endpoint
path on genuine native movement authority.

Final stamped Foundation passes 819. Workbench loads 5,832/11,835 at CRC
`3131538f`, exits `0`, reports `ScriptValidation true` with zero errors, and
cleans exactly. All seven process chains/21 stages run build `008cd481d5e5`, exit
`0`, and preserve exact fingerprint continuity. The three new chains are
`948803021dbbf846 -> 326a49119bc4b0a7 -> same`,
`e67861d2f8bdf456 -> 126ef467ff7c1abe -> same`, and
`c461df2b3cdfe0a2 -> 940b360563a6ead1 -> same`. Carrier validation requires
exactly one movement-or-settlement expectation family and includes a mixed-
family rejection self-test. The independent census finds zero engine processes
and guard roots with both proof mutexes free. This adds proof depth rather than
completing a new Blueprint phase.

The subsequent owner-applied checkpoint implemented the lifecycle-aware pre-
reconcile correlation and quarantine policy that was open here. This preceding checkpoint proved only
claimant absence and tamper rejection. Durable endpoint ABA
snapshots remain a separate future contract-2 schema decision whose schema
number is not yet assigned. Exact defensive QRF
and exact garrison rebuild share only the static
production persistence preflight and restore-normalization path; this runtime
cut does not prove those families. Native persistence-source selection, world
scope, package/live dedicated server-client behavior, migration, markers,
multiplayer/JIP/reconnect, performance, soak, and the broader certification
gates remain separate work.

## Preceding Phase 24 Runtime-Owner Snapshot Plan

Campaign Schema 70 and runtime-settings Schema 24 remained unchanged. That was
a schema-neutral and settings-neutral proof correction. Phase 24 preserves the
pre-runtime owner snapshot, then classifies bounded legitimate follow-up
admissions without allowing missing rows, duplicate identities, or owner
mutations to pass. Assertions are based on the sampled owner count.

The exact-counterattack authority assertion may validly `SKIP` only when all
exact-order, open, terminal, invalid, projection, support-leak, and
`VIRTUAL`/`MATERIALIZING`/`PHYSICAL`/`DEMATERIALIZING` metrics are zero. Phase
17 owns the deterministic exact-counterattack lifecycle proof. Full guarded run
`seed1985_t0_p1_u1784134163` on that stamped source passed both Phase-24
assertions with 14/14 sampled runtime owners, zero snapshot invariant failures,
and three open exact counterattacks under three `VIRTUAL` projections with zero
invalid authority or support leaks. Phase 17 passed 11/11, all six staged
spawn-adapter cleanup cases passed, all 18 state deltas were zero, and the error
census plus every process/profile/external cleanup counter was zero. Workbench
validation passed 5,830 files and 11,822 classes at CRC `e836e3b4`, with script
validation successful and zero hard errors. The next plan gate is the smallest
concrete failure slice from the still-uncertified 687-case suite: 583 PASS, 50
WARN, 46 FAIL, 7 BLOCKED, and 1 SKIPPED, with 5,537 of 5,685 certification
requirements proven.

## Persisted Schema 70 Capability Background

At that checkpoint, the development tree used Campaign Schema 70 while runtime
settings remained on Schema 24. This Phase-9 slice cuts newly admitted enemy garrison
rebuilds over to exact contract `1`: one capacity-bounded frozen infantry
manifest, one prepaid support-pool debit, and one reciprocal order/operation/
manifest/spawn/group graph. The manifest and selected-zone ownership capability
are frozen and preflighted before debit; after debit the reciprocal runtime
graph is built or the exact
full rollback runs. Confirmed casualties persist across virtual travel,
physical projection, fold, and restore; delivered survivors become held
garrison population under an `OPEN`/`ON_STATION` operation and zero-delta
receipt, without aggregate double counting. Ownership-terminal,
admission-rollback, and proportional prearrival-refund paths settle failure.
Historical contract-zero rebuilds remain isolated. Invalid or ambiguous current
graphs quarantine at `-70`, retaining linked claimants and pins without guessed
cleanup, deletion, refund, settlement, or outcome. Selected source/target
ownership capability fencing rejects an initial ABA change before pressure. A
pressure-marked retry rechecks the capability and rejects before order creation
or debit.

## Preceding Staged Faction-Audit Checkpoint

The preceding checkpoint retained Campaign Schema 70 and runtime-settings Schema
24. Its staged exact spawn-adapter proof was a runtime owner only while active,
prerequisite-ready, and backed by the exact reciprocal fixture graph. Intermediate
root-only frames still run the real faction and vehicle-ownership audit; only
the synthetic zero-member increment receives exact pre-handoff grace. `Finish()`
disables the owner, and final cleanup remains strict, so residue stays visible.

That checkpoint kept the real-frame Phase-17 exact-counterattack projection and one real
engine death with adapter-owned N-1 continuity. Exact life requires both
controller-aware and stock `SCR_AIDamageHandling` alive state; death preflight
requires enabled damage handling and settles for at most four real frames after
one `Kill`, without re-kill or live reconciliation. Initial, casualty, and
survivor physical confirmation independently re-sample the production owner for
at most four frames, OR-latch worker mutation telemetry, and require actual
`PHYSICAL`/`LIVE` authority, a successful non-held batch, one unique spawned
root, exact roster/runtime/handle/group counts and bindings, and zero legacy
support. Async settle stages continue to yield. Once a native handoff succeeds,
the debug runner now confirms `PHYSICAL` and performs the immediately controlled
fold or kill in the same invocation, matching production ordering and avoiding
an artificial one-second debug-only scheduling window. Binding diagnostics also
distinguish an incomplete handoff from a complete binding whose entity is
nonliving. Already-successful batches are never requeued or respawned.

R28 first failed all five casualty assertions, while unchanged R28b passed all
eleven Phase-17 assertions and proved N=9 to N-1=8. R29 narrowed one intermittent
defect to treating next-frame `tickChanged` as authority even though production
could already be physical. R30 then passed the first six Phase-17 assertions but
failed all five casualty assertions before `Kill` because one otherwise bound
and registered member became nonliving during the artificial scheduling window.
Phase-24 runtime-owner classification passed; exact-counterattack authority was
skipped after Phase-17 cleanup removed the focal order. The preceding R31 source
corrected that proof-only ordering seam. The earlier proof-backing source passed
Foundation at 808. Stamp commit `40c2d48` passes PC Game Module validation for
project ID `histasi`, GUID `698532771130111D`, at 5,830 Game files/11,822 classes
and CRC `dc565606`. The run reaches `Game created`, reports `Script validation
successful`, exits `0`, records zero hard errors, and leaves cleanup-guard,
process, default-log, and external-spill residue at zero. Campaign Debug/runtime
proof was pending at that checkpoint.

R31 `seed1985_t0_p1_u1784117364` ran the preceding exact stamped source
`393733cc165b96ec494c72f96741cf993d400ebd` across 687 cases
at 577 PASS/50 WARN/52 FAIL/7 BLOCKED/1 SKIPPED. Certification remained false:
5,528/5,685 required assertions were proven, with 139 failed, 18 blocked, and
zero certification warnings. All 11 Phase-17 targets passed exactly once.
Expected living count was 9, spawn ticks were `8/18` with deferred `0`, initial/
casualty/survivor physical settles were `1/1/1` of four, and casualty settle was
`1/4`. Both Phase-24 owner assertions passed; classification was `16/16`, with
open exact counterattacks/projection groups `2/2`. All 18 reported deltas across
the 29-line state diff were zero, the run recorded zero script, HST, or crash
errors, and exact cleanup left zero session residue, owned processes, new engine
processes, or default-log entries. The targeted proof-ordering gate is closed;
unrelated failures and external certification blockers remain open.

The immediately preceding exact defensive-QRF checkpoint closes only the guarded
canonical-fallback external-process restart subgate. Source
`25b2dc361bc935aea904e08a665755840389c6e0` is stamped by `ce2542b`, UTC
`2026-07-15T02:08:19Z`, label
`schema70-settings24-exact-qrf-external-restart`. It changes neither persisted
contract. All three exact settlement cuts pass fresh-process
`prepare -> recover -> replay`; this is not proof of native persistence-source
selection, package, dedicated/multiplayer behavior, physical world state,
migration, reconnect/JIP, or soak.

The scoped Schema-70 engine-proof checkpoint is sealed at implementation
`2f71236bfc02329a3c8000b104f1b7b1043dc99c`, UTC
`2026-07-13T22:20:52Z`, label
`schema70-settings24-exact-enemy-garrison-rebuild-engine-proof`, with stamp
commit `ef95555`. The sealed checkpoint's post-clock-fix Workbench log
`logs_2026-07-14_01-06-19` exits `0`, completes create/destroy, compiles 5,826
Game files/11,806 classes at CRC `b819d967`, contains no HST script error or new
native crash event, and leaves zero Workbench processes. The focused engine log
`logs_2026-07-14_00-52-56` at CRC `6fa838ee` records one passing
`HST_TEST_EnemyGarrisonRebuildAuthority` JUnit testcase, no JUnit failure, an
empty failed list, exit `0`, and zero surviving processes, but predates the
coordinator clock correction:
admission/capacity,
delivery/held authority,
casualty continuity, restore, ownership terminal handling, admission rollback,
prearrival refund, settlement crash resume, historical isolation, Schema-70
quarantine, orphan-runtime quarantine, quarantine retention, and selected-
ownership ABA rejection. Foundation passes at 790 script-symbol references. The
focused environment still records the known recoverable base-game VM diagnostic
plus two filter-constructor diagnostics. The focused run succeeds but is not
exception-free.

Latest Full Campaign Debug totals and exact run identity are maintained in the
Campaign Debug verification audit. The wider suite remains incomplete and is
not certification evidence even when a focused vertical slice passes.

R12 through R15 isolated the disposable radio fixture's exact component lookup,
enabled inherited resource, engine destruction action, and paired replication
dependency. R15 proved the normal destroy callback, deterministic receipt,
objective/reward outcome, borrowed provenance, and immediate stop-rebuild
admission before generated rebuild equipment failed closed because its enabled
multiphase component lacked the paired enabled inherited `RplComponent`.
Checkpoint `a81d494` enables that existing component ID; every diagnostic run
retained exact fixture cleanup and a zero final tracked-state diff.

The preceding R10 remains the last positive proof that all five Phase 18 cases
passed. Its Phase 20 proof established shared-clock restoration and
enemy-strategic-authority fingerprint isolation, with one town
behavior/authority case still failed. Phase 22 completed at four PASS, three
WARN, and zero FAIL: the stable-order identity, strategic-isolation baseline,
and native RUN-response defects all passed, while the remaining warnings are
movement observations. Phase 24 completed at 11 PASS, one WARN, and zero FAIL.
Typed enemy-order cleanup left zero settlement failures, zero tracked open
orders, and zero runtime claimants. R16 retains the isolated radio proof. R17
proved the generic-mission correction at 11/11, and R18 narrowed the sole exact
summary mismatch to `civilian_occupier_support`, 2,514 live versus 2,614
restored. The correction applies legacy no-town FIA/occupier backfill only to
pre-22 saves; Schema-22-and-newer stored values, including zero, are
authoritative.

R20 `seed1985_t0_p1_u1784047342`, R21
`seed1985_t0_p1_u1784049066`, and R23
`seed1985_t0_p1_u1784054690` remain dated diagnostic history. R23 first proved
the active-demolition-witness boundary but retained one exact-QRF settlement
failure, an open tracked row, two runtime claimants, and a later two-order leak.
It ran build `0e54f6c` at 564 PASS/51 WARN/66 FAIL/7 BLOCKED and proved
5,440/5,650 required assertions, with 192 failed and 18 blocked.

R24 `seed1985_t0_p1_u1784059798` ran bridge build `6303e58` across 688 cases at
578 PASS/50 WARN/53 FAIL/7 BLOCKED and proved 5,522/5,682 required assertions,
with 142 failed and 18 blocked. It closed the R23 contamination: typed enemy-
order cleanup passed with zero settlement failures, zero open orders, and zero
runtime claimants; the open-order leak remained 0 -> 0; core Phase-24 seed,
report, and outcome cases passed; and final tracked-state restoration was exact
zero. Its focused `enemy_qrf.settlement` and `enemy_qrf.persistence` assertions
still failed, isolating a one-pool validator mismatch and stale proof
classification.

R25b `seed1985_t0_p1_u1784063032` remains dated refund-authority evidence. It
ran implementation `434b73a` across 688 cases at 577 PASS/51 WARN/53 FAIL/7
BLOCKED and proved 5,523/5,681 required assertions, with 140 failed and 18
blocked.

Historical R26 `seed1985_t0_p1_u1784074264` ran the durable exact-QRF recovery
checkpoint across 688 cases at 577 PASS/51 WARN/54 FAIL/6 BLOCKED. It proved
5,504/5,667 required assertions, with 145 failed and 18 blocked. Both
`enemy_qrf.settlement` and `enemy_qrf.persistence` pass. Typed cleanup reports
settled 0, failures 0, open 0, and runtime 0; the open-order leak is 0 -> 0.
Seeded in-memory capture/restore remains exact at 11/11 missions, 22/22 mission
assets, 21/21 runtime entities, 9/9 groups, 10/10 runtime vehicles, and 1/1
field vehicle, and final tracked-state restoration is exact zero. Escalation
physicalization remains WARN. R26 remains the in-memory diagnostic and is not
rewritten by the separate canonical-fallback external-process proof. It is not
certified because unrelated failures and the overall
`persistence.real_restart`, world-scope, and manual external gates remain open.
Package, dedicated-server, network/JIP/reconnect, and soak gates also remain
separate.

Historical R27 `seed1985_t0_p1_u1784093667` ran the preceding stamped native-counterattack-
projection checkpoint under `full_certification` with CLI autostart. Across 688
cases it records 577 PASS/49 WARN/55 FAIL/6 BLOCKED/1 SKIPPED and proves
5,500/5,663 required assertions, with 145 failed and 18 blocked. The Phase-17
baseline, materializing, physical, fold, continuity, and clock-isolation
assertions each pass exactly once. Both Phase-24 runtime-owner-classification and
exact-counterattack-authority assertions also pass exactly once. All 29 tracked
state-diff lines are zero. The isolated run seeded only Schema-24 settings and no
campaign save, then removed its profile, process, log, and temporary session
artifacts exactly. R27 remains a diagnostic rather than whole-suite
certification; unrelated failures and the broader external gates remain open.
It predates the casualty-continuity checkpoint. R28 first failed the five
casualty assertions; unchanged R28b passed all five plus the preceding six.
R29 then reproduced the intermittent physical-confirmation race before `Kill`.
R30 `seed1985_t0_p1_u1784110353` ran 687 cases at 576 PASS/50 WARN/53 FAIL/7
BLOCKED/1 SKIPPED and proved 5,510/5,672 required assertions, with 144 failed and
18 blocked. Its first six Phase-17 assertions passed, but all five casualty
assertions failed before `Kill` because one otherwise bound and registered
member became nonliving in the artificial one-second debug scheduling window.
Phase-24 runtime-owner classification passed; exact-counterattack authority was
skipped because Phase-17 cleanup removed the focal order. All 18 reported deltas
across the 29-line state diff were zero, the run recorded zero HST script errors
or crash markers, and isolated cleanup was exact. R31 proves the preceding
proof-ordering correction with all targeted assertions passing and exact-zero
drift.

Workbench native heap corruption was reproduced when a large campaign-debug
method accumulated too many locals. Moving the debug-only state into a context
object and narrow helpers restored the native compiler boundary. The crash-fix
checkpoint, including the final marker and typed-cleanup refinements, passes
Foundation with 793 script-symbol references, compiles and completes Workbench
create/destroy at 5,826 Game files/11,807 classes with CRC `287d01ec`, and
remained alive at the 8-, 16-, and 24-second cold-open checks before deliberate
shutdown. Large coordinator cases must remain decomposed.

The current mission-sweep source retains one disposable exact radio-lifecycle
fixture inside Campaign Debug's isolated state clone. It supplies the legal
ONLINE prerequisite for `destroy_radio_tower`, then carries the same site into
the DESTROYED prerequisite for `dynamic_stop_tower_rebuild`, without weakening
production admission or selecting an authored transmitter. Both cases use the
normal physical server callbacks, and cleanup explicitly removes the temporary
projection, transmitter, zone, and radio-site authority before live state
restoration. The original fixture source passed headless Workbench script
validation at 5,826 Game files/11,807 classes, 46,634K static storage, CRC
`d8a34f4b`, with a clean
`Script validation successful` result. No Workbench or game process survived.
The R12-R15 ladder proved the exact component/resource/action boundaries and
retained exact cleanup. The current source queries the exact concrete type,
uses the engine destruction path, and enables the paired inherited replication
dependency required by generated rebuild equipment. Exact end-to-end runtime
evidence belongs in the Campaign Debug audit.

The historical exact-QRF external-restart checkpoint is stamped at source
`25b2dc361bc935aea904e08a665755840389c6e0`, UTC
`2026-07-15T02:08:19Z`, label
`schema70-settings24-exact-qrf-external-restart`, with stamp commit `ce2542b`.
It changes no persisted campaign or runtime-settings schema. Terminal intent is
persisted as `PREPARED`; the complete refund tuple is staged with
`applied=false`; the original debit, claimant graph, and durable survivor
authority are validated; the canonical refund is applied or replayed; the
applied receipt is published last; and the operation and order tails finalize.
Arbitrary old partial rows remain fail-closed rather than migration candidates.

R26's focused in-memory proof covers six committed cuts: dual-pool and support-
only shapes at pre-refund, post-refund/pre-receipt, and receipt/pre-terminal
interruption. It also covers three uncommitted full-refund cuts. Same-state
replay and a second capture/restore are no-ops. Bad current-provenance `SETTLED`
chains enter stable quarantine after Schema-67 normalization, while mutationless
historical settlement remains compatible. R26 remains in-memory evidence.

The separate guarded canonical-fallback proof ran `before_refund`,
`after_refund`, and `after_receipt`. Each cut used a fresh process for
`prepare`, `recover`, and `replay`; all nine stages succeeded and exited `0`.
The prepared source and terminal readback were exact, startup reconciliation
changed state only during recovery, and both explicit repeat reconciliation and
the fresh replay process were no-ops. This closes the exact defensive-QRF
canonical-fallback external-restart subgate only.

That sealed tree passes Foundation at 806 references. Stamped PC Workbench compiles and creates
the game at 5,830 Game files/11,820 classes, 46,915K static storage, and CRC
`ff59593b`; script validation succeeds with zero script errors. The focused
`HST_TEST_EnemyQRFAuthority` run records one testcase, zero failures, an empty
failed list, and `AllExact 1`; a known recoverable stock VM diagnostic remains.
R26 supplies the integrated in-memory settlement, persistence, cleanup, leak,
and exact-restore evidence above.

The preceding proof-ordering source passes Foundation at 808 script-symbol
references. Its final stamped Workbench Game-module gate loads 5,830 Game files
and 11,822 classes with 47,077K static storage at CRC `b789ee05`, reports
`Script validation successful`, exits `0`, and records zero script, HST, or
hard-failure signals. Guarded cleanup leaves zero session root, owned processes,
new default-log entries, or external spill. R31 supplies the preceding positive
all-eleven Phase-17 proof plus both Phase-24 owner assertions; R28b remains the
unchanged positive comparison, R29 supplies the earlier physical-publication
race, and R30 supplies the bound-and-registered nonliving-member scheduling
diagnosis.
The preceding owner-snapshot source separately passes Foundation at 808 references
and stamped PC Game validation at 5,830 files/11,822 classes with CRC
`e836e3b4`. Guarded run `seed1985_t0_p1_u1784134163` passes all 11 Phase-17
assertions, both Phase-24 assertions at 14/14 sampled owners with zero invariant
failures and three valid open exact `VIRTUAL` projections, all six staged
cleanup cases, all 18 state deltas, the zero error census, and exact external
cleanup.
Native persistence-source selection and all package, broader physical-world,
dedicated/multiplayer, migration, reconnect/JIP, and soak gates remain open.

The preceding active-demolition-witness checkpoint at
`0e54f6cbc7f7084e5534fc603b491cba0d91b653` changes no persisted campaign or
runtime-settings schema. Destroy-target scans reject parented and
inventory-slotted entities before requiring a moving physical projectile or a
triggered blast. Entity-backed callbacks and scans share one canonical source
key; each target keeps at most 64 lifetime receipts and records bookkeeping only
after authoritative mutation. Campaign Debug adds
`primitive.destroy.no_ambient_witness_score` before explicit demolition damage.

The earlier `0b380f00` checkpoint supplies the SpawnQueue resume delta;
`3ded248a` tracks every direct Phase-22 order through typed cleanup, and
`2508a735` accepts the debug-isolated checkpoint prefix. R21 confirms those
boundaries; R22 confirms the queue delta. The preceding demolition checkpoint
passes fresh headless Workbench Game/PC validation in
`logs_2026-07-14_14-41-29` at 5,826
Game files/11,807 classes, 46,643K static storage, and CRC `c3ab042e`.
Validation reports `Script validation successful`, no HST compile or fatal
diagnostic, and zero surviving engine processes after close. R23
`seed1985_t0_p1_u1784054690` remains the dated runtime proof for all six quiet
generic destroy-target assertions and all seven destroy-family start/runtime/
primitive cases. Historical external-restart Workbench, focused engine, R26 in-
memory, and guarded canonical-fallback process evidence is recorded above. The
exact QRF subgate is closed, while broader world-scope restart and the wider
certification gates remain open.

The immediately preceding Schema-69/settings-24 checkpoint moved newly admitted
enemy counterattacks to exact contract `1`: one frozen infantry manifest,
one directly routed operation graph, casualty-preserving virtual/physical
transfer, deterministic virtual combat, canonical ownership transition, return
to origin, and a survivor-proportional refund to exactly one originally charged
attack or support pool. Schema-68-and-earlier counterattacks remain historical
contract `0`, and invalid or ambiguous current graphs quarantine at `-69`
without fabricated authority, deletion, settlement, refund, or outcome. The
appended `PREPARED` settlement state preserves terminal intent across prepare,
tuple staging, refund, receipt recording, and finalization; restore and same-
session ticks resume it idempotently. Explicit plus deterministically derived
claimant IDs reject foreign or duplicate cleanup authority.

The scoped engine-proof checkpoint is sealed at implementation
`5bdcda938840ab769b41ff3e1856d908572a8c45`, UTC
`2026-07-13T19:40:35Z`, label
`schema69-settings24-exact-enemy-counterattack-engine-proof`, with stamp commit
`73a64ef`. Foundation passes at 771 script-symbol references. Final all-five
Workbench log `logs_2026-07-13_15-41-50` exits `0`, compiles 5,821 Game files/
11,786 classes at CRC `3a8bd64f`, explicitly validates WORKBENCH, PC, XBOX, PS4,
and PS5, contains no script or HST errors, and leaves zero Workbench processes.
Focused engine log `logs_2026-07-13_15-42-52` exits `0`, records one passing
JUnit testcase, an empty failed list, and `AllExact=1`, including valid PREPARED
recovery, same-session ABORTED recovery, foreign derived-ID collision hold, and
fail-closed SETTLED-without-resource-receipt handling. The autotest environment
also writes a recoverable base-game
`SCR_EditableEntityCore/GetPlayerIdentityId` VM exception to `crash.log` before
the HST case completes successfully, so the run is not exception-free.

This seals source, Foundation, all-target Workbench, and focused engine proof
only. Full Campaign Debug in `HST_Dev`, serialization/restart,
package/native/live-server behavior, migration runtime, marker runtime,
network/JIP/reconnect, and soak remain open.

The preceding Schema-68 planning checkpoint is implementation
`4c9a94a1cb4811b6e75a7dca5dba70efffcb523d`, UTC
`2026-07-13T15:43:01Z`, label
`schema68-settings24-enemy-planning-engine-proof`. The latest packaged
test used the prior `f97b12e` baseline. It created `$profile:Partisan`, had no
retired tree to migrate, and exposed a fresh-start failure: both enemy authorities
quarantined at `-67`/`-68`
and emitted 598 repeated one-second warnings.

The sealed tree retains the runtime-integrity correction's one production factory
for startup, admin reset, and fresh-state proof with the exact three-role pools and two idle enemy
planners. Restored malformed state remains fail-closed; recovery requires exact
row cardinality, no nulls, the exact nonempty campaign preset identity, empty
mutation/order arrays, and the complete known poisoned Schema-68 signature.
Resource, topology, preset, null-row, legacy-order, versioned-order, and other
near misses remain untouched. Unchanged unavailable
warnings use 300-second reminders, and live Campaign Debug observes authority
through the production exact resolvers without mutation.

The complete retired profile tree moves before consumers run through verified
staging, destination recheck, canonical/conflict archival, and final byte
comparison. Directory conflicts are archived, source directories delete deepest
first, and same-process re-entry is guarded; because Enforce has no atomic cross-
process promotion or exclusive lock, profile migration is explicitly single-
writer. Campaign Debug's destructive protected-marker probe retries final repair
and player-marker cleanup. Foundation passes at 753 script-symbol references.
Final stamped-tree all-target Workbench log `logs_2026-07-13_11-43-49` compiles
5,816 Game files/11,770 classes at CRC `5a998c21`; WORKBENCH, PC, XBOX, PS4,
and PS5 report `Script validation successful`, the process exited, and zero
Workbench processes survived cleanup. Campaign Debug, package execution,
packaged restart/migration, dedicated/live-server, multiplayer, and soak evidence
remain open.

A focused command-line Game-process autotest now sits between Workbench validation
and Campaign Debug. `HST_TEST_EnemyPlanningCommitmentAuthority` ran in
`logs_2026-07-13_11-44-28`; JUnit timestamp
`2026-07-13T15:44:34.667Z` records one testcase, zero failures, and an empty
failed list. `AllExact()` passed all 17 deterministic Schema-68 planning
fixtures, including retry-quarantine repeated-pass idempotency. This is isolated engine-executed service proof, not HST_Dev,
coordinator, world, persistence, restart, package, or network proof.

The sealed checkpoint stays within Schema 68 and makes periodic
enemy planning commitment-aware. Target scoring collapses linked same-faction
order/support/open-operation rows to one root, rejects incompatible roots before
ranking, and penalizes compatible roots by `-12` each up to `-24`. An exact
patrol can share a target with a non-patrol defensive response. A proposed
duplicate patrol excludes that target and deterministically reranks within the
same due decision, avoiding a wasted 180-second cycle. If compatible and blocking
rows share one root, blocking wins and the root counts once. Queued and equivalent
legacy/canonical targets participate; settled/terminal and rival rows are ignored,
and multi-reject diagnostics remain stable across permutations.

Preparation only freezes the decision. Commitment and active-order identity are
revalidated before every debit, including a pressure-marked retry; candidate and
source identity are additionally rechecked before unpressured admission applies
pressure. When all targets are committed, planning closes as a zero-cost skip
with no pressure, debit, order creation, or rival mutation. The focused engine
case executes the complete 17-fixture planning report, including the three
commitment-specific fields. Save validation now delegates planning quarantine
to the production authority so bounded failure reason, revision advancement,
and repeat idempotency use one path; the retry-tamper fixture advances the
campaign clock to its recorded retry time before testing fingerprint quarantine.
The matching Campaign Debug assertions for expanded selection, all-committed
skip, and both post-freeze commitment races have not run in Campaign Debug, a
packaged restart, or a live server.

The immediately preceding sealed source/Workbench boundary is the commitment-
aware Schema-68/settings-24 checkpoint at implementation
`695caf46ce6b4146e5407711b76d5e0c578d7392`, UTC
`2026-07-13T14:44:37Z`, label
`schema68-settings24-commitment-aware-enemy-planning`, Foundation 751, and
Workbench CRC `e483e71c`. The earlier bootstrap/profile/marker correction is
implementation `fdf262637e74a70c12454f6c1d3789c2cd0a0f05`, UTC
`2026-07-13T13:19:22Z`, label
`schema68-settings24-bootstrap-profile-marker-hardening`, Foundation 751, and
Workbench CRC `0544aa1d`. The earlier Schema-68 planning-authority checkpoint is
implementation `356b0d47f96111c3b09eb7ede3cb34f0661c2b6e`, UTC
`2026-07-13T01:04:41Z`, label
`schema68-settings24-enemy-planning-authority`, Foundation 744, and Workbench CRC
`971d30d0`. The Schema-67 resource seal is implementation
`2798cb20b824ed74419ab6dc9bdce03f18ef71df`, UTC
`2026-07-12T23:46:02Z`, and label
`schema67-settings24-enemy-strategic-resource-authority`. Foundation passes at
736 script-symbol references. Final stamped normal log
`logs_2026-07-12_19-52-14` and all-five log
`logs_2026-07-12_19-52-36` both load 5,809 Game files/11,751 classes with CRC
`a353fa0d`. All-five reports `Script validation successful` for WORKBENCH, PC,
XBOX, PS4, and PS5; zero HST script errors were observed and zero Workbench
processes survived cleanup. This first Blueprint
Phase 9 slice makes versioned `HST_FactionPoolState` the canonical per-enemy
attack/support/aggression owner with persisted independent resource-income and
aggression-decay accumulators plus last-bucket checkpoints. It records compact
periodic evidence and an un-compacted contiguous operational sequence behind one
income/spend/refund/aggression/live-adjustment API. Zero-effect operational
commands retain receipts. Exact replay is read-only; conflicting fingerprints and
underflow/overflow fail atomically; enemy roles stay independent; and existing
exact QRF/patrol debit-refund policy links into the authority without changing
those operation contracts. Order, defense-ledger, town-event, and ownership-
transition backlinks are validated during current-schema restore. Rejected or
orphan receipt rows are attributed/quarantined before purge and cannot consume
valid operational capacity. Mission terminal effects and ownership aggression
are admitted before their outer status/security/owner publication boundaries;
Campaign Debug registrations use disposable nested state and focused one-group
runtime calls, but these source fixtures remain wired/static and have not
executed in Campaign Debug.

Schema 67 is sealed at the source/Workbench boundary. Pre-67 migration adopts
current valid balances/aggression plus valid legacy resource/
aggression cadence accumulators only and invents no history,
spend, refund, settlement, order, or planning decision; malformed current graphs
quarantine at `-67`. Operational receipts never compact: 4,096 accepted rows per
enemy role is a hard lifetime limit, and later operational admission fails only
for the capped role while the rival and periodic cadence remain independent.
The sealed Schema-68 planner is not part of the Schema-67 seal.
Blueprint Phase 8 remains
runtime-uncertified.

The immediately preceding sealed source/Workbench checkpoint is Campaign Schema 66 while runtime settings
remains Schema 24. The sealed Schema-66 stamp identifies implementation
`a7031797e67d99a99a066038cd8fa39efc03cff1`, UTC
`2026-07-12T20:28:33Z`, and label
`schema66-settings24-local-security-marker-integrity`. The current Blueprint Phase 8 slice makes enemy-town local
security an exact roster authority: one deterministic epoch per eligible
canonical enemy town, one authored 2–5 member frozen manifest, exact survivor
slots through physical fold and restore, one police `-1` destruction event, no
same-epoch resurrection, and rearm only after a newer ownership revision or
later positive police event. Resistance-held towns target zero automatic police
and roadblocks. Pre-66 migration preserves logical security/owner/support/
garrison facts and removes only unlinked disposable legacy police projections;
current malformed graphs quarantine at `-66`.

That sealed tree also repairs the `27672e6` client-local campaign-marker owner
regression. Protected campaign markers are system-owned/non-removable and self-
heal from the authoritative registry after native deletion or mutation. Player-
created/dynamic markers remain editable. Foundation passes at 729 script-symbol
references. Final normal/all-five Workbench checks pass at 5,806 Game files/
11,740 classes with CRC `ec860be7`, `Script validation successful`, zero HST
script errors, and zero surviving processes. Campaign Debug, native local-
security, manual marker-input, real restart, package, multiplayer, and soak gates
remain open.

The previous sealed source/Workbench checkpoint is Campaign Schema 65/runtime-
settings Schema 24. It identifies implementation
`609add9eeadf73816764c497178e2d35081307d1`, UTC
`2026-07-12T18:30:29Z`, and label
`schema65-settings24-civilian-consequence-authority`. It adds exact canonical-town casualty,
resistance vehicle-theft, and real-combat episode events; aggression before/
after evidence with one matching strategic receipt; persisted danger/last-
applied episode authority; and native pedestrian panic/recovery. Final stamped
normal Workbench compile/create and all-five validation are clean at 5,802 Game
files/11,728 classes with CRC `c0a672b9`; all-five reports `Script validation
successful`, both runs exited `0`, zero HST script errors were observed, and
zero Workbench processes survived cleanup. The preceding unstamped CRC
`be076102` remains preliminary evidence only. Foundation passes at 717 script-
symbol references, and every runtime gate remains open.

The earlier sealed campaign source/Workbench checkpoint is Campaign Schema 64
on runtime-settings Schema 24. It identifies implementation
`6afadc7c13681b78171939a740862e52328beffd`, UTC
`2026-07-12T15:57:55Z`, and label
`schema64-settings24-ambient-runtime-authority`. It deliberately keeps campaign
persistence at Schema 64 while advancing runtime settings from Schema 23 to 24
for the Blueprint Phase 8 ambient-runtime slice. It implements global actor/
traffic caps, 120-second-or-
longer leased fair rotation, a four-root scheduler, asynchronous behavior-ready
admission, immutable per-zone/kind projection slots, bounded slot-specific
recovery, per-frame player-first claim observation, and a fail-closed transient-
versus-claimed vehicle persistence boundary. Foundation passes at 711 script-
symbol references. Stamped-tree normal Workbench compilation and all-five-
configuration validation pass at 5,799 files/11,718 classes with CRC `bb083672`,
successful validation, zero HST script
errors, and zero surviving Workbench processes. Pure-kernel fixtures do not
prove native runtime behavior. Campaign Debug, packaged runtime, actual
brief enter/exit, autosave/restart, promoted-root destruction, new-campaign
reset, Campaign Debug Phase 20 production-path execution, rendered UI, native
movement/recovery/recycling, the ten-town/ten-minute stutter/churn/performance soak, and
multiplayer proof remain pending.

Campaign Schema 64/runtime-settings Schema 23 is the preceding sealed canonical-
town-influence checkpoint. It identifies implementation
`6f3c913eaed66926cce38b2ecafcff94084898a3`, UTC
`2026-07-12T11:28:41Z`, and label `schema64-canonical-town-influence`.

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
test. Schema 61 is the preceding sealed marker-projection foundation. This seals
the ownership-transition checkpoint, not the broader Blueprint Phase-7 claim;
Campaign Debug and packaged evidence remain open.

This roadmap is the working implementation plan for Partisan. It is meant to be
updated as phases complete, split, or get refined through HST_Dev smoke tests.

Status legend:

- Complete: implemented, validated statically, and smoke-tested enough to move on.
- In progress: current active slice.
- Planned: accepted direction, not implemented yet.

Current-state authority in this document is intentionally narrow: the CRI table,
Blueprint Milestone Snapshot, Current Implementation Baseline, and Next
Engineering Milestones are current. The numbered phase sections preserve
acceptance detail and implementation history; they do not override
`FEATURE_CHECKLIST.md` or the runtime-evidence audit.

## Definition Of Done For Every Phase

Every phase should leave the project in a state that can be tested by a human
in HST_Dev and inspected through a clear server-side report path. Unless a phase
explicitly says otherwise, completion means:

- `tools/validate-foundation.ps1` passes or is intentionally extended and then
  passes.
- Deterministic state/service logic has a focused official engine autotest when
  it can run without a world. Its result must name the exact asserted boundary
  and must not be promoted to HST_Dev, Campaign Debug, persistence, or package
  proof.
- HST_Dev can launch far enough to exercise the changed system.
- The changed feature has a command-menu report, debug action, or log path that
  exposes its important runtime state.
- Campaign mutations remain server-authoritative and flow through services.
- Persistent campaign state does not store raw `IEntity` references.
- Missing Reforger APIs, invalid resources, unavailable services, or fragile
  runtime calls fail with visible reasons and preserve campaign state.
- No hidden addon dependency, non-base-game asset assumption, or save migration
  is introduced unless the phase explicitly calls for it.
- Large Workbench-facing methods keep debug state in small context objects and
  helpers. A successful script parse is not enough when local-variable pressure
  can cross the native compiler boundary; compile/create/destroy and a cold-open
  check are required after structural growth in coordinator proof methods.
- Phase status in this document is updated when the phase is completed.

## Current Baseline

Partisan is already past the blank-project stage. The repository has a
versioned mission-registry baseline, server-side campaign/economy/mission/
persistence/checkpoint services, player/HQ/Petros setup, custom arsenal, loot,
vehicle cargo, virtual garage/build scaffolding, loadout editor code, generated
Everon sites/routes, mission objectives, physical mission primitives, support
requests, enemy commander orders, civilian/undercover state, and an
Partisan command menu.

The current server runtime is centered on
`HST_CampaignCoordinatorComponent`. It instantiates services, restores or
creates campaign state, tracks persistence, refreshes markers, restores every
eligible durable field-vehicle row before gameplay publication, and then ticks
mission timers, objectives, mission runtime,
convoy runtime/outcomes, income, enemy resources, aggression decay, civilians,
support requests, enemy orders, HQ runtime objects, physical zone activation,
zone capture, and civilian population.

The state model already has the right campaign save-game spine:

- versioned faction pools, with sealed Schema-67 bounded enemy strategic
  mutation receipts for attack/support/aggression authority
- players
- zones
- garrisons
- active groups
- QRFs
- map markers with Schema-62 projection/source revisions, stream sequences,
  tombstones, projection epoch, and global watermark
- revisioned ownership-transition receipts with active/latest zone backlinks
- arsenal items
- garage vehicles
- vehicle cargo
- runtime vehicles
- saved loadouts
- issued loadout items
- captured emplacements
- ammo points
- active missions
- generated sites
- generated routes
- mission objectives
- mission runtime entities
- mission assets
- support requests
- canonical exact operation records for both infantry QRFs, player Search-and-
  Destroy, mission convoy, enemy and purchased-garrison patrols, all three
  assassination guards, newly started POW rescue, and enemy-town local security
  across schemas 50-66
- enemy orders
- civilian zone state
- sealed Schema-64 canonical town-influence records and exact/legacy
  influence events
- sealed Schema-66 local-security patrol envelopes with deterministic owner-
  revision/epoch identity, exact roster/backlinks, terminal loss, and rearm state
- undercover records
- campaign tasks
- monotonic authority sequence
- bounded command receipts
- resource transactions
- bounded campaign events

The plan should harden each existing service into a playable, testable vertical
slice. It should not rebuild the foundation.

## Campaign Runtime Integrity Program

Campaign Runtime Integrity (CRI) is the active dependency-ordered delivery gate.
Campaign Schema 71/runtime-settings Schema 24 is the current contract. The
sealed implementation/source is `32727238d74b29905c68e5a80bb5897dfdc783c0`, UTC
`2026-07-18T16:34:38Z`, label
`schema71-settings24-focused-force-authority`. It adds one dedicated engine case
for combat presence, ownership transitions, and town influence while retaining
the two-generation campaign journal, monotonic native/journal recovery order,
journal-authoritative administrative reset, controlled-shutdown native fence,
both exact rebuild restart cuts, and mixed-native shutdown/restart proof.
The focused case passes 35/35 targeted assertions, 87/87 counted conditions,
and 18/0 state diff with zero errors, crashes, or cleanup. This is isolated
state-only evidence and not full certification.
Foundation, Workbench, focused journal authority,
ordinary five-process, stale-journal/native, and admin-reset stale-native proofs
pass with exact selection and zero cleanup. The five-process proof also
retains periodic AUTO, manual move/destruction, blocking shutdown, native and
profile-fallback no-save restart, the exact serialized one-live/one-tombstone
position/yaw/cargo graph, tolerance-verified physical placement, zero native
overlap, and zero residue. The sealed extension closes the dedicated mixed-
native captive/carrier/player/seat graph across controlled shutdown, native and
profile-fallback restart, stable seat recovery, durable binding versus fresh
process-local `RplId`, guard rematerialization, and zero cleanup. This remains
scoped evidence, not runtime certification of every adversarial native shutdown
branch. The two garrison cuts additionally close scoped delivery plus native handoff, measured
movement, exact production fold, casualty continuity, and restart/replay. The
preceding periodic
scheduler checkpoint retains exact UUID/flag, debounce, retention, and
controlled-end authority.
Schema 68
remains the historical sealed planning checkpoint at implementation
`4c9a94a1cb4811b6e75a7dca5dba70efffcb523d`, Foundation 753, and final stamped-
tree Workbench CRC `5a998c21`. It retains the bootstrap/profile/marker correction
from the immediately preceding seal. The
latest packaged baseline proved canonical-root creation but failed fresh enemy
authority and did not contain a retired tree. The immediately preceding sealed
source/Workbench checkpoint is the commitment-aware planner at implementation
`695caf46ce6b4146e5407711b76d5e0c578d7392`, Foundation 751, and CRC
`e483e71c`. The earlier bootstrap/profile/marker correction is implementation
`fdf262637e74a70c12454f6c1d3789c2cd0a0f05`, Foundation 751, and CRC
`0544aa1d`. The earlier Schema-68 enemy-planning authority is
implementation `356b0d47f96111c3b09eb7ede3cb34f0661c2b6e`, Foundation 744,
and CRC `971d30d0`. Schema 67 is the earlier resource authority at implementation
`2798cb20b824ed74419ab6dc9bdce03f18ef71df`, Foundation 736, and CRC `a353fa0d`.
The focused command-line engine case closes the deterministic Schema-68 planning
rung with all 17 report fields exact. It does not change the sealed campaign or
settings schema and does not close any world or external-runtime gate.
Every Phase 8 runtime exit remains open; sealed Schema 67 and Schema 68 are later
source work, not runtime certification. Campaign Debug and packaged restart must
still prove fresh bootstrap, exact saved-state recovery, single-
writer whole-tree profile removal/conflict archival, and marker integrity before
runtime certification.
The matching Campaign Debug cases must still execute the commitment-aware
selection branches, same-decision patrol rerank, stable diagnostics, all-
committed skip, and unpressured/pressure-marked admission races, followed by
packaged restart and live-server proof.
Campaign Schema
65/runtime-settings Schema 24 remains
the previous sealed source/Workbench boundary. It identifies implementation
`609add9eeadf73816764c497178e2d35081307d1`, UTC
`2026-07-12T18:30:29Z`, and label
`schema65-settings24-civilian-consequence-authority`. Campaign Schema 64/runtime-
settings Schema 24 is the earlier sealed pair; the preceding
Schema-64/settings-23 checkpoint is the canonical-town-influence slice. The
Blueprint Phase 8 ambient-runtime checkpoint advances runtime settings only.
Schema 64 gives each curated town one canonical political
support/population/contact/flip record. FIA,
occupier, and invader support remain separate basis-point values. The scaling
formula pinned to commit `6e4226d3863ca8673535386c2fff8b6e08a806c4`
produces 100/200/50 basis points for raw `+1` at populations 100/25/400.
Resistance requires strictly more than 8000 basis points; enemy ownership
requires strictly less than 4000; equality is neutral. Every flip uses the
Schema-62 ownership service. Legacy fields are projections only, pre-64 facts
migrate once, and malformed current authority quarantines at `-64`. Exact events
preserve population before/after, absolute debug seeds share their idempotent
path, and restore validates the event chain through the final record. Zone
Pressure is contacted-only/current-first; Resistance Territory is complete,
deterministically ordered, and parent-publication fenced. Foundation and
Workbench compile/validation pass; Campaign Debug and runtime proof remain open.

The sealed Settings-24 slice adds global civilian actor/traffic caps,
120-second-or-longer leased fair rotation, a separate reconciliation cursor,
four root-spawn transactions per health update including initial static roots,
asynchronous behavior-ready admission, immutable per-zone/kind projection
slots, bounded slot-specific recovery/recycling, static military owner/policy
refresh, per-frame player-first claim observation that avoids a full ambient-root
occupancy scan, and a fail-closed transient-versus-claimed vehicle save boundary.
A shared live-root tracker registers promotions, restored/adopted field vehicles,
and garage redeploys for current transform/destruction/cargo state. Every
`HST_PersistenceService` capture/checkpoint path repeats reconciliation, as does
new-campaign reset. Saved durable IDs remain stable across process restart;
restore/registration precedes first-frame claims, and exact forward/reverse
bindings own targeting and deletion checks. Garage redeploy tracks a fresh
campaign-stable ID before payment/stored-row removal and rolls back its root,
runtime/cargo rows, binding, and staged stored row on failure. Reset can retain occupied live tracked `loot_vehicle`,
`field_vehicle`, and `garage_redeploy` roots, normalizes the retained records to
`field_vehicle`, and copies their vehicle/cargo state before replacing campaign
state. Campaign Debug Phase 20 consumes the production global plan/four-root
transaction-start cap. Five traffic vehicles is the configurable
daytime/low-heat true-town default when population and global budgets permit,
not a fixed ceiling or guarantee. Combined pedestrian/driver demand is capped to
the unique valid concrete appearance pool; invalid/duplicate config entries do
not inflate capacity and exhaustion never falls back to a visible duplicate.
Foundation passes at 711 references; stamped-tree normal and all-
five Workbench validation passes at 5,799 files/11,718 classes with CRC
`bb083672`, zero HST script errors, and zero surviving processes. Its sealed
identity is implementation `6afadc7c13681b78171939a740862e52328beffd`, UTC
`2026-07-12T15:57:55Z`, and label
`schema64-settings24-ambient-runtime-authority`. Campaign Debug, packaged native
behavior, actual save/restart, and a ten-town/ten-minute churn/stutter/performance
soak remain open for that seal.

The sealed Schema-65 source/Workbench extension routes tracked ambient deaths through a 256-row
casualty queue and deferred theft through a separate 64-row queue. They share a
four-attempt frame cap, retry indefinitely with bounded backoff, and defer
capture while work/fault authority remains. IDs come from persisted monotonic
campaign authority. Resistance civilian-vehicle theft requires an exact player
pilot and occurs only after player-first durable promotion; passengers only
protect the transient root. Canonical town consequence events can carry one exact enemy
aggression target/delta/before/after chain plus a unique matching strategic
receipt. Civilian combat pressure is episode-based and requires current-
operation or recent-fire evidence; `HOT` alone is inert. An already admitted
pending receipt drains before another edge. The zone persists the last observed
combat revision, danger episode, adopted Schema-64 baseline floor, last-applied
episode receipt, panic deadline, and last consequence event; it requires
`episode - lastApplied <= 1`, and a canonical-town edge can append at most one
political event. Pedestrians flee on a move waypoint at RUN and recover to
deterministic wandering at WALK, using a separate bounded panic-route counter
without hot-path AI reactivation. Minor localities are panic-only; their exact
receipt fingerprint map is bounded but session-only. Indexed restore validates
structural event/pool/receipt/backlink facts before a live-preset pass quarantines
non-enemy aggression targets; stable-ID exhaustion fails admission. Final
stamped normal and all-five Workbench checks are clean at 5,802 Game files/
11,728 classes with CRC `c0a672b9`, `Script validation successful`, zero HST
errors, and zero surviving processes. Foundation passes at 717 script-symbol
references. Campaign Debug, packaged runtime, real profile save/
restart, multiplayer, and soak evidence remain open.

The sealed Schema-66 extension creates the first exact automatic local-security
consumer. One eligible canonical enemy town with positive police pressure owns
a deterministic ownership-revision/epoch envelope, an authored 2–5 member
frozen manifest, one local-security operation, one held batch, and exact living
slots. Active towns materialize the same survivors and use process-local cyclic
waypoints; deactivation and persistence reconcile casualties before runtime
retirement and strategic hold. Restore re-enters held virtual authority and does
not refill casualties. Destruction applies police `-1` once and blocks the same
epoch from reopening. A newer ownership revision or later positive police event
is required for rearm; owner/pressure clear, setup/stop, and spawn failure settle
without loss. Resistance automatic police/roadblock targets are zero. Pre-66
migration preserves logical security/garrison facts and drops only unlinked
disposable legacy projections; current corruption quarantines at `-66`.

The same extension repairs campaign-marker ownership after the Schema-61
client-local cutover. Protected campaign markers bypass local-player ownership,
remain non-removable, and are rebuilt from the registry after native integrity
drift. Player markers remain editable. Source proofs are wired for both domains,
but final Workbench/Foundation, Campaign Debug, native, restart, package,
multiplayer, and soak gates remain open.

Schema 63 remains an earlier sealed dependency and gives existing forces one
canonical crew-aware combat-presence and zone-heat authority. Fresh
registered physical samples count conscious dismounted infantry, operational
occupied armed mobile platforms, and operational occupied static weapons;
cargo, empty vehicles, destroyed/burning platforms, immobile mobile platforms,
terminal/quarantined rows, and stale samples do not contribute. Eligible virtual
forces retain durable living-infantry authority but never infer pressure from an
abstract vehicle count. Capture, missions, HQ threat, civilian safety, and enemy
strategy share the result. Each zone persists bounded revisioned
`HOT -> COOLING -> COLD` diagnostics; the default cooling deadline is 30 seconds
and is not extended on an unchanged tick. Restore invalidates physical samples,
migrates pre-63 saves to `COLD`, and resets malformed current authority to
`COLD`. Zone rendering enters at the activation radius and exits at the larger
deactivation radius. Unresolved spawned-group authority invalidates affected
queries, and capture requires a living conscious character player rather than a
spectator or Game Master proxy. Indexed runtime registrations, reusable sampling
buffers, and contribution/authority-gap/radius caches keep the one-second path
allocation-light. Its sealed tree passes Foundation, normal Workbench
compile/create, and explicit five-configuration validation; runtime execution
remains open.

Schema 64 composes Schema-63 combat presence with the sealed Schema-62 ownership
boundary. One canonical
ownership service owns military capture, mission capture, political support, admin, debug
seed, and migration repair. Each location has contract version `1`, a monotonic
owner revision, one active transition backlink, and one latest-completed
backlink. An immutable expected-owner/revision receipt orders exact patrol and
aggregate security retirement, hostile-runtime cleanup, new security, frozen
support, owner revision, town/generated-site/facility/logistics derivation,
retaliation, economy/outcome, events, parent-aware projection, notification, and
persistence scheduling. Retry-capable steps occur before owner publication;
identical replay is a no-op and stale/conflicting authority fails closed.
Supported zone aliases canonicalize before fingerprint/replay, and admission
stops before a revision that cannot safely advance into valid serialized
authority.
Military/mission capture keeps enemy consequences; admin reconciles security and
notifies without retaliation; debug seed also suppresses notification; migration
repair preserves security and suppresses retaliation/notification.
Admission and current-save normalization enforce those cause flags. Political
support callers submit typed town-influence events; strict threshold intent is
pending until this ownership service accepts/completes it. No legacy support
projection may publish an owner.

Schema 61 remains the sealed marker-only client-projection foundation. Protocol
`2` now includes the source ownership revision independently from each marker's
own revision and global sequence. Nested political flips caused by a parent
capture complete their domain work but defer marker/menu/GM/notification
publication to that parent. Later valid top-level requests enter as pristine
accepted/needs-retry receipts and execute in array order before domain mutation,
preserving exact mission, political, admin, and migration intent. Ordinary
rebuilds ignore queued pre-owner receipts; active owner-applied and completed
unreleased receipts retain their previous owner/revision. Command UI resolves
exact zone/receipt authority first and uses a retained marker only as correlation;
unsafe rows quarantine/purge. Parent publication snapshots the full logical
marker array plus epoch/sequence, stages and validates each exact child receipt-
zone-marker chain, releases all children, then commits. Failure rolls back the
complete snapshot. Setup freezes immutable
`m_bSetupProjectionWithoutMarkers` history across activation.
Production and proof share one logical snapshot builder; source proof covers
staged rollback, resolver fail-close/purge, setup history, two-child atomic
release, and two restart boundaries with exactly-once political completion.
Restore accepts queued followers only while every later unresolved top-level
receipt remains pristine, quarantines multiple owner-applied
publishers or an owner-applied publisher behind earlier unresolved top-level
authority, rejects projection lifecycle/child-chain and duplicate completed
zone/revision authority, and propagates quarantine iteratively. Restore recomputes
the exact sorted support set of linked towns plus every town within 1,500 m,
requires an ordered applied prefix with same-row
event/delta evidence, and validates reason/garrison/counterattack/event/marker/
setup correlations.
An exact linked-town support event commits its influence fact once and submits
threshold intent immediately. Canonical ownership correlates existing intent or
queues its exact receipt behind earlier work; only the fallback scan waits for
FIFO drain.
Retry preserves concrete quarantine causes. Without exact resolver authority,
publication is unavailable. Explicit-ID reconstruction reuses frozen
preconditions only after semantic identity matches.
Ownership retry and then town policy run before setup/terminal returns with a
frozen-clock bypass; political repair there suppresses fresh retaliation and
notification. Ownership strategic events record exact owner/capture/aggression
facts and exclude unrelated queued/retry global deltas. The existing bounded snapshot/
delta, ACK, gap/resync, atomic registry, and client-local native reconciliation
wire model is unchanged. Schema 66 changes only native protected-marker
ownership/integrity: campaign markers are system-owned/non-removable and the
readiness keepalive self-heals missing or mutated native state. Player markers
remain on their editable separate path.
Major-change marks are edge-triggered/coalesced behind a bounded checkpoint
deadline. Repeated gameplay/retry heartbeats cannot reset it; the first change
after a successful checkpoint starts a new interval. Transition completion
re-arms process-local pending state after final backlink mutation without
extending an existing deadline.
Packaged host/two-client equality, ownership atomicity, actual reconnect/late
join, native rendering/security, map-close continuity, and real save/restart
remain open; Schema-62 fixtures have not run as Campaign Debug or packaged
runtime evidence.

Schema 61 is the preceding sealed marker-projection foundation:
implementation `27672e67ce4285810f313130293df1ac917c9bdf`, UTC
`2026-07-12T01:02:39Z`, and label
`schema61-authoritative-marker-projection`. Full Foundation passes with 655
symbol references; final Workbench Game validation loaded 5,782 files/11,631
classes with CRC `df41a779` and created the game; hidden normal WorldEditor
stayed responsive 10/10 over 20 seconds without a first-party error/crash
signature. In the preceding Schema-60 slice,
only newly quoted and confirmed player Search-
and-Destroy support receives contract `1`: one infantry-only catalog roster,
$350 plus HR-per-frozen-member ledger, request, separately typed operation, held
batch, and active group remain reciprocal. Its direct route, exact casualties,
virtual/physical projection, bounded abstract-garrison combat, displaced-fold
return to assignment, commander recall, and settlement remain durable. Hostile
clear leaves the force on station. Fold, physical recall exit, and campaign-stop
retirement first exhaustively reconcile the affected projection and require
exact root/member adapter and PhysicalWar cardinality while survivors remain.
Persistence runs the corresponding global exact-infantry pass, validates each
physical exact-support authority graph, and refreshes its live position before
capture. Held-batch cancellation snapshots strategic living strength before
cleanup. Pre-60 Search-and-Destroy remains contract
`0`; malformed current claimants quarantine at `-60` without legacy fallback or
guessed balances. Their groups are globally non-operational and not combat-
present. Expired exact-support capacity removes a tombstone and paired terminal
request only after replay validity, unique identity, no-live-backlink, and full
receipt-reciprocity checks; corrupt/quarantined pairs remain evidence. The
compiled/wired proof covers valid pair prune/restore and corrupt retention but
its Campaign Debug assertions have not run.

Schema 60 also retires the overlapping Maiden's Bay town in favor of the one
Logistics Warehouse. No-anchor saves remain untouched; ambiguous location
authority fails closed; mutable generic references canonicalize; and nonzero
typed authority remains frozen even when settled, quarantined, malformed, or
recognizable only from an exact group mode/status. Mutable generated content
rekeys, while frozen sites/routes receive deep canonical clones. Ordinary lookup
resolves the warehouse and exact historical lookup/runtime ID equivalence keeps
old assignments valid without re-enumerating the town. The isolated migration
proof is compiled and wired into Campaign Debug but has not run, and packaged
save/restart remains open. Focused runtime-source repairs remove known one-second
reconciliation/query/authority/diagnostic multipliers and disable AI horn timing/
output at the wheeled-vehicle base. The published server must still prove the
observed stutter, horns, and duplicate location are fixed.

Schema 59 is the preceding stamped source/Workbench boundary. Under that
contract every configured radio zone owns one contract-1 durable radio site,
while contradictory current authority uses quarantine `-59`. A resolved ONLINE
site can admit one exact destroy mission;
its physical destruction receipt moves the site to DESTROYED and removes radio
influence. Only that state can admit stop-rebuild, which targets construction
equipment and moves the site to REBUILDING, once per tower-destruction epoch.
Destroying the equipment records the stop attempt and keeps the site DESTROYED
without advancing that epoch; failure, expiry, or campaign stop completes one
generated replacement and returns ONLINE. Stable site target identity is
separate from each mission's unique physical runtime-entity identity. Immutable
authored prefab/position provenance remains frozen while the current projection
can hand off from borrowed to generated ownership; every mission snapshots that
ownership/provenance at admission. Authored transmitters are borrowed retained
multiphase-damage objects, never deleted, and
no initial fallback is invented for an absent or ambiguous target. Direct
  borrowed destruction requires reciprocal lock/revision and authoritative
  tracked damage state; generated explosive scoring also requires a live matching
  mission component, bounded position, and a unique key in a persisted bounded
  evidence set. Physical destroy/heal/rollback writes are verified. Authored
  identity uses a tight 0.75-meter tolerance while a 12-meter physical tolerance
  covers bounded safe-ground placement. Permanent generated
  ONLINE projections disable verbose witness logging and keep nearby-entity
  scans dormant until exact mission identity exists. New-campaign reset restores
the authored transmitter before state replacement or fails closed. A missing
borrowed projection enters an explicit dormant pending phase while missing or
cross-linked runtime authority quarantines. Generated ONLINE restore requires
destruction plus completed-rebuild provenance. Generic mission runtime, zone
composition, objective ticks, commander progress, and generic failure settlement
cannot own or mutate the exact projection/aggregate. Quarantine cleans corrupt
current linked rows while preserving coherent terminal history.
Focused proof calls production transitions and durable evidence, rejects a
direct second rebuild admission, proves linked quarantine cleanup, and replaces
only projection seams.
The current Campaign Debug sweep now creates one disposable transmitter and
debug-prefixed exact radio site solely inside its isolated state clone. Exact
radio target selection is fixture-only and fails closed rather than falling back
to an authored tower. The destroy case requires native DESTROYED state before
the normal server asset callback; the following stop-rebuild case uses normal
explosive evidence against generated equipment. Both require exact rewards,
receipts, epoch continuity, and one-attempt enforcement. Explicit service and
prefix cleanup must leave no fixture entity, projection, zone, or radio-site row
before the live state is restored. R16 now proves the entire isolated chain:
fixture admission, engine destruction and normal callback, destruction receipt,
generated rebuild equipment, explosive evidence, unchanged destruction epoch,
one rebuild receipt, exact `$450`/`$350` rewards, second-attempt rejection,
289-to-zero prefixed cleanup, six-to-zero tagged entities, and an exact-zero
final tracked-state diff. Packaged authored-content, restart/streaming,
multiplayer, and soak proof remain open.
Schema 60's full Foundation gate passes with 644 symbol references. Final
stamped Workbench Game validation loaded 5,777 files/11,615 classes with CRC
`7aa80fc9` and created the game. The correctly targeted hidden normal WorldEditor
stayed alive/responding for 10/10 samples over 20 seconds with no first-party
error/crash signature; diff check was clean apart from line-ending warnings.
Both Schema-60 Campaign Debug proof services and the typed-QRF mismatch assertion
are compiled/wired but unexecuted. Schema 58 remains an earlier stamped checkpoint at
implementation `f0ba07ff2bc295d12542a3ea34b4c913e99b1869` with build label
`schema58-exact-rescue-pows`. No Schema-60 packaged server/client, native/live
behavior, actual save/restart, rendered UI, stutter/horn, campaign setup,
networking, reconnect, or JIP proof exists.
Schema 62 is an earlier sealed source/Workbench checkpoint at implementation
`7c93e0a485bcabe5a364c0b0cfeca235accb50f7`, UTC
`2026-07-12T06:11:19Z`, and label
`schema62-canonical-ownership-transition`. Its Foundation gate passes with 670
script-symbol references. Headless Workbench Game validation loaded
5,785 files/11,652 classes with CRC `22c13a32` and zero script errors. The normal
Script Editor open remained responsive without a crash, and zero Workbench
processes survived. No packaged
host/client proof exists.
It supersedes feature-order implications in the legacy numbered roadmap below;
that roadmap remains useful for feature history and acceptance detail.

Work remains in Blueprint Phase 9 of 13 through Schema 70. The additional
counterattack restart subgate deepens proof inside that phase; it does not mark
a new phase complete. Blueprint Phase 8 and every earlier Blueprint phase still
retain native, dedicated-server,
restart, or multiplayer exit gates. Native tests deferred while source slices
advanced must be backfilled; reaching a later phase, sealing source, or
publishing a build does not waive them.

| Stage | Status | Exit condition |
| --- | --- | --- |
| CRI-0: Repository truth and baseline | Campaign Schema 71/settings 24 is current. `32727238d74b29905c68e5a80bb5897dfdc783c0`, label `schema71-settings24-focused-force-authority`, UTC `2026-07-18T16:34:38Z`, is the sealed implementation/source identity. | The historical non-package checkpoint passed Foundation at 874 and Workbench at 5,846/11,899 with CRC `cad640f3`, zero hard errors, and exact-zero owned cleanup. Its focused force-authority case passed 35/35 targeted assertions, 87/87 counted conditions, and 18/0 state diff with zero errors/crashes and exact-zero owned cleanup. Earlier journal, mixed-native, stale-journal/native, admin-reset, and exact-rebuild results remain sealed. Active-world/native breadth, persistence/restart, Workshop/server/client, migration, markers, network/JIP/reconnect, performance, soak, and unrelated Campaign Debug failures remain open. |
| CRI-1: Campaign authority foundation | Implemented foundation; isolated exact radio lifecycle runtime proof passes | Schema 59 adds one durable site/mission/target transition graph per radio zone, distinct stable-site and per-mission physical IDs, one stop-rebuild attempt per destruction epoch, contract `1`, and `-59` quarantine. R16 proves physical callbacks, deterministic receipts, unchanged epoch, second-attempt rejection, exact rewards, fixture cleanup, and zero final diff. Packaged authored binding, restart/streaming, multiplayer, and soak proof remain. |
| CRI-2: Exact force manifests | Sealed foundation plus exact rebuild delivery and physical-live-fold fresh-process proofs | The two fixture cuts retain nine accepted slots, one confirmed casualty, and the same eight survivors through exactly-once delivery or native projection, production fold, restart, and replay without refill or aggregate double count. Prove natural route/combat loss, other force families, multiplayer/JIP, soak, and contract-zero isolation without widening vehicle/asset/multi-root admission; generic realization remains open. |
| CRI-3 through CRI-5: Force runtime, operations, virtualization, and movement | In progress; exact rebuild has delivery/replay plus physical-live-fold proof, the counterattack harness retains its eight cuts, and the dedicated mixed-native graph passes the five-stage controlled-shutdown/restart chain | The rebuild fixture closes `delivery_pending` exactly-once delivery and `physical_live_fold` native handoff, measured movement, production fold, casualty continuity, restart, and read-only replay. The mixed-native fixture separately closes exact captive/carrier/player/seat continuity, durable rebind, guard rematerialization, and zero cleanup. These do not prove natural full-route travel/combat, other exact force families, arbitrary mission/force graphs, multiplayer/JIP, or soak. |
| CRI-6: Client projection | Schema-61 marker-only snapshot/delta/JIP projection is sealed; Schema 66 protects native campaign-marker ownership. The Campaign Debug probe mutates/deletes a tracked campaign marker through the owner client, invokes production repair, retries final repair, and separately edits/removes/cleans a player marker | Execute the compiled probe, then prove host/two-client/late-join equality, source/projection revisions, forced gap/resync, map-close continuity, manual protection/self-heal, registry/static-count stability, one canonical instance, editable player-marker isolation, and restart. |
| CRI-7: Ownership, combat presence, and town influence | Schema-62 ownership/protocol-2, Schema-63 combat presence, and Schema-64 canonical town influence/contact/Map-War/migration/hot-path source boundaries are sealed. Their dedicated deterministic state-only engine case passes 14/14 ownership, 9/9 combat-presence, and 12/12 town-influence assertions. | Broaden into Full Campaign Debug and native/world proof: active security/economy effects, native classifications/cooling, real migration/save/restart, rendered Zone Pressure/territory, packaged clients, network/JIP/reconnect, performance, and soak. |
| CRI-8: Civilian runtime and political consequences | The sealed Settings-24 checkpoint supplies the ambient budget/lifecycle/claim foundation. Sealed Schema 65 source/Workbench adds 256-casualty/64-theft queues, a combined four-attempt frame cap, bounded-backoff indefinite retry/capture deferral, exact-pilot theft after durable promotion with passenger-only non-recycling protection, exact town aggression/strategic receipts, pending-receipt-first non-HOT combat episodes with adopted-floor/last-applied invariants and full canonical live-combat fingerprints, indexed structural plus live-preset role restore validation, and native pedestrian panic/recovery with separate bounded route recovery. Minor-locality fingerprints remain session-only. Foundation passes at 717 script-symbol references and final stamped normal/all-five Workbench checks are clean. | Package-prove native death attribution/fallback deduplication, queue capacity/capture deferral, pilot-only post-promotion theft with passenger protection, exact replay/aggression/stable-ID failure, at-most-one town event per combat edge, clear/rebound, RUN-to-WALK panic recovery without hot-path AI activation, migration/quarantine order, all persistence/checkpoint paths, restart, multiplayer, and the ten-town soak. |
| CRI-8b: Exact local security | Sealed Schema 66 source/Workbench gives each eligible enemy town one authored 2–5 member exact epoch with survivor fold/restore, once-only police loss, no-resurrection, bounded rearm, zero resistance police/roadblocks, conservative migration, and `-66` quarantine | R22 retains the eliminated materialization deferral and passes all eight isolated local-security assertions. Package-prove native waypoints/casualties/fold/re-entry/restart, ownership/terminal ordering, migration, multiplayer, and balance. |
| CRI-9: Enemy commander strategic authority | Sealed Schema 67 owns per-enemy resources, Schema 68 planning, Schema 69 exact counterattacks, and Schema 70 exact rebuilds. Counterattack ownership restore and Schema-71 monotonic native/journal source selection remain implemented beneath the controlled-shutdown fence | The ordinary proof validates scoped checkpoint/end/restart authority, the stale-journal counterattack chain proves newer native selection without journal mutation, and exact rebuild closes both `delivery_pending` and `physical_live_fold` fresh-process cuts. Exact QRF and other strategic-family restarts, natural rebuild route/combat behavior, other force families, multiplayer/JIP, soak, and broader package/network behavior remain open. Durable endpoint ABA snapshots remain a separate future contract-2 schema decision whose schema number is not yet assigned. |
| CRI-10 through CRI-11: Missions and progression | Broad-alpha foundations implemented; convoy, all assassination guards, first rescue slice, and one exact purchased-garrison policy source-complete | Existing exact families retain narrow boundaries. Schema 58 adds only newly started `rescue_pows`; historical POWs, refugees, other rescue/mission families, aggregate forces, broader vehicle policy, runtime proof, mission depth, and tuning remain open. |
| CRI-12: Certification | Planned | Isolated dedicated-server, reconnect/JIP, save/load, long-soak, and migration evidence closes the program. |

### Blueprint Milestone Snapshot

| Player-visible milestone | Current state | Next proof boundary |
| --- | --- | --- |
| Exact Forces | Implemented sealed foundation plus exact enemy-garrison-rebuild delivery and physical-live-fold fresh-process proofs; both retain nine accepted slots, one casualty, eight survivors, and exact replay identity | Preserve queue retry/restore semantics plus the ambient commander and typed-cleanup boundaries, then prove natural route/combat behavior, other force families, multiplayer/JIP, and soak while retaining the existing exact-family backlog. |
| Clean Forces | First exact infantry-QRF lifecycle plus legacy mixed-group personnel terminal repair implemented; sealed Schema 63 excludes empty assets from pressure | Prove native/GM/strategic living counts, conscious/cargo/crew/static classification, corpse detachment, last-death cleanup, survivor reprojection, and one crewless mixed-QRF salvage transition across restart. |
| Living War | Broad-alpha paths plus the sealed exact projections and both exact rebuild fresh-process cuts share the combat-presence/heat boundary | Delivery and physical-live-fold subgates are closed for the rebuild fixture; now prove natural route/combat casualty continuity, every earlier exact roster, all Schema-63 consumers and `HOT -> COOLING -> COLD`, multiplayer/JIP, and soak. |
| Reliable Orders | Exact player QRF/Search-and-Destroy, defensive QRF/patrol, Schema-69 counterattack, and Schema-70 rebuild retain separate policies. Counterattack restore fences lifecycle-illegal ownership rows before runtime reconciliation; rebuild independently has exact `delivery_pending` and `physical_live_fold` restart chains | Preserve the counterattack eight-cut matrix and both rebuild results, then broaden natural route/combat, other force-family/world-scope, multiplayer/JIP, performance, and soak proof; do not infer other exact-family runtime coverage from this fixture. |
| One Campaign View | Schema 61 implements marker-only snapshot/delta/JIP projection; Schema 62 adds ownership source revision/fencing; Schema 66 protects campaign markers. Exact QRF/counterattack/rebuild/patrol backing now delegates to the publisher's reciprocal canonical predicate | Execute the owner-client probe and rerun orphan checks, then prove host/two-client/late-join equality, revisions, nested ownership atomicity, gap/resync, map-close continuity, one canonical repaired campaign marker, player-marker editability/removal, duplicate-free rendering, and restart. |
| Political Map | Schema 62 canonicalizes all ownership causes; Schema 63 supplies combat pressure/heat; Schema 64 supplies sole town support/population truth and contacted/territory projections. The historical focused deterministic engine case passed all three state-only groups; historical e11 canary rejected stale mission-source fixture assumptions. The correction is sealed in retained ee0, whose packaged focused gate and corrected canary pass noncertifying at 35/35 and 87/87. | The unchanged package's full profile is independently rejected at 598/47/26/13/1 and 5,630/5,695. Triage the clustered full failures and seal source corrections in a new candidate, then prove broader native ownership consequences and classifications, real migration/restart, rendered contact/current-first/complete-territory projection, JIP/reconnect, and no bypass. Broader encounter/facility consequences, performance, and soak remain. |
| Living Towns | Settings 24 adds budgeted ambience; Schema 65 adds town consequences/pedestrian panic; sealed Schema 66 source/Workbench adds exact enemy-town local-security rosters and zero resistance automatic police/roadblocks without making actor count political truth | Package-prove town taxonomy/ambience, casualty/theft/combat/panic, exact police roster casualties/fold/rearm, ownership policy, save/restart, cleanup/recycle, and ten towns for ten minutes without churn or one-second stutter. |
| Enemy Commander | Sealed resource, planning, and Schema-69 counterattack authority remains intact. Schema 70 makes newly admitted garrison rebuilds exact while leaving historical rebuilds on contract `0`; delivery-pending and physical-live-fold cuts now pass fresh-process recovery/replay | Preserve the validated ambient cadence isolation and explicit production-tick ownership; then prove natural route/combat behavior, other force families, multiplayer/JIP, and soak. |
| Mission Parity | All 39 configured IDs map to MVP primitives; convoy, all assassination guards, newly started `rescue_pows`, and exact radio lifecycle use narrow contracts. R16 proves the disposable isolated radio pair end to end. Current source adds structural active demolition-witness admission and a quiet pre-action assertion | Fresh Workbench validation passes. R23 proves all six generic `primitive.destroy.no_ambient_witness_score` assertions and all seven destroy-family start/runtime/primitive cases. Next package-prove authored radio binding/restart/streaming and runtime-prove callback-plus-scan deduplication, convoy, all three assassination guards, and Schema-58 rescue. Keep refugees, historical POWs, and other mission families legacy until their own explicit cutovers. |
| Resistance Progression | Arsenal, garage, training, undercover, HQ/Petros, and end-state foundations exist | Complete exact logistics/loadouts/static defenses and tune the full progression loop. |
| Campaign Certification | Retained `rejected-after-runtime` candidate `partisan-rc-ee0e8add2a29-20260719T063815Z`, version `0.1.0-rc.20260719T063815Z.ee0e8add`, is sealed from clean source HEAD `ee0e8add2a298e83fd304b7660c4fc480dc6383f`; package SHA-256 is `981258439b9d08866c4883471cacfe33aa373a36a667a39e8c939f285db74daf`. Foundation passed all 874 checks; all five Workbench targets passed at 5,848/11,901 and CRC `f64e0868`; the seal binds four package files and 50 evidence files. | Clean harness `273ed14ba8526259c8b0d248177fa53b59ade683` passed all five packaged focused cases at JUnit 5/0/0/0, 40 files, 12 classifier checks per run, 11 approved diagnostics, and zero residue. Clean harness `4f8d7e2d7a39896737fd6754060523bf852c5fa8` accepted the unchanged package's corrected canary at 9/1/0/1/0, 35/35, 87/87, 18/0 restoration, two approved stock plus zero unapproved diagnostics, ten files, and zero residue. Both scoped rungs are `passed-noncertifying`. Clean harness `a5ccf36aee17a4f88d7f1c2f232ce9fc14652018` then retained the unchanged package's rejected full-profile boundary at 598/47/26/13/1, 5,630/5,695, 18/0 restoration, 24 unapproved diagnostics, ten files, and zero residue. Historical 0e/e11 evidence remains isolated. No further runtime evidence may attach to ee0; build a new candidate for source fixes. Release remains `NO-GO`. Fuel/damage/attachments/trunk parity, abrupt-termination recovery beyond the last completed checkpoint, broader active-world records, Workshop/live clients, network/JIP/reconnect/soak, arbitrary migration, multi-writer/off-device recovery, markers, and performance remain open. |

CRI-1 and the first CRI-2 vertical slices remain intentionally narrow. Troop
training is the first production ledger consumer; exact visible garrison
confirmation is the second, and exact player QRF is the first paid executable
support consumer. Support recall is the first visible command to map a typed
domain result into explicit durable receipt status and aggregate identity;
other visible commands still use the compatibility classifier. Other command and cost paths remain on their legacy service
contracts until their dependency stage supplies the required exact quote,
manifest, or operation model. Schema 47 retains the bounded runtime scheduler
and native adapter for one exact infantry group root plus exact members, feeds it
the accepted QRF manifest, and restores only durable survivors. Existing broad-alpha physicalization paths do not
become exact merely because this first slice exists; only projections carrying
the explicit queue-owned identity use it, and the legacy guards hold those
specific projections rather than silently migrating unrelated groups.
Schema 49 supplies that operation model only for newly confirmed exact paid
player infantry QRFs and uniquely coherent accepted nonterminal schema-48 rows.
Schema 50 adds strategic route, materialization, exact virtual roster, and
bounded virtual-combat authority to the same infantry-only player consumer.
Schema 51 applies the shared exact route/roster/projection boundary only to
newly planned infantry-only enemy defensive QRF orders, with a separate
arrival-pressure, return-origin, and survivor-resource settlement policy.
Schema 52 applies a separate exact route/roster/projection boundary only to
newly started convoy missions: exactly three vehicles, three crew groups,
optional vehicle-zero cargo/captive, generated-route virtual travel, physical
intercept/fold, casualty persistence, and once-only mission-outcome settlement.
Schema 53 applies the shared one-root infantry projection boundary only to newly
queued enemy patrol orders, with generated-route outbound travel, one closed
lap, physical contact hold, fold/reprojection, return-origin duty, and proactive
  survivor settlement. Schema 54 then applies the same adapter boundary only to
  newly issued policy-v2 purchased resistance garrisons, using an empty
  executable root, arbitrary exact member roster, infinite local loop,
  survivor-only reprojection, and no-refund terminal policy. Schema 55 applies a
  route-less empty-root/member guard contract only to newly started
  `assassinate_officer` missions at version `1`. Schema 56 adds the same exact
  survivor/HVT-separated lifecycle only to newly started `assassinate_traitor`
  guards at contract `2`/policy `exact_assassinate_traitor_guard_v1`, with `-56`
  quarantine and conservative pre-56 migration. Schema 57 adds it only to newly
  started `assassinate_specops` guards at contract `3`, policy
  `exact_assassinate_specops_guard_v1`, intent `assassinate_specops_guard`, and
  `-57` conflict quarantine. Its restore path records
  `migration_schema57_exact_specops_guard`; conflicts record
  `normalization_schema57_exact_specops_guard_conflict`. Ordinary
  `mission_group_*` rows are not claimants. Historical/pre-opt-in assassination
  missions, policy-v1/aggregate garrisons, rescue and other unsupported mission/
  order families, and vehicle/multi-root forces remain legacy. All three guard
  paths are HVT-separated, route-less, survivor/casualty preserving across
  materialize-fold-re-entry, zero-refund, and excluded from virtual combat.

## Current Implementation Baseline

The codebase now includes the core implementation pieces that older phase rows
treated as future work:

- `HST_PersistenceService` owns typed `AUTO`, `MANUAL`, and `SHUTDOWN`
  checkpoints plus Schema-71 native/journal source ordering. Native success is
  callback-confirmed before writing the exact pending snapshot as the next
  journal generation. Fallback-only writes are synchronous. Checkpoint sequence,
  restore sequence, time, and normalized fingerprint select the newer valid
  source; future or ambiguous authority fails closed. Independent periodic and
  first-edge major-change clocks implement accepted-checkpoint coverage,
  rejection fairness, non-extending remarks, and in-flight suppression. The
  game-mode bridge rejects new ingress while draining, completes read-only
  readiness and post-prepare revalidation before one-way subsystem latches,
  maintains native quiescence/identity across retry, uses a 270-second retry
  window, commits blocking shutdown, and then owns retention/purge ordering. The
  final stamped focused and fresh-process proofs pass; the ordinary chain is a
  scoped coordinator/end and field-vehicle regression rather than exhaustive
  native-topology certification. Abrupt-termination
  recovery beyond the last completed checkpoint and broader
  active-world/client/network/arbitrary-migration/marker/performance/soak gates
  remain open.
- `HST_ForceCompositionService` owns request/result force planning for support,
  mission guards, garrison activation, QRFs, counterattacks, convoy guards, HQ
  attacks, and debug probes.
- `HST_ForceCatalogService`, `HST_ForcePlanningService`, and
  `HST_ForcePlanningIntegrityService` own versioned exact execution-prefab
  catalogs, immutable manifest/quote planning, and deterministic integrity.
- `HST_OperationService` owns shared version-1 transition authority for
  confirmed exact paid player infantry QRFs, newly confirmed player Search-and-
  Destroy, and newly planned exact enemy defensive QRFs, and supplies
  deterministic settlement identity to the
  schema-52 mission-convoy owner. The QRF operations preserve immutable origin/assignment separately from
  tactical target and validate queue/handoff, projection, restore, settlement,
  and replay transitions. Schema 50 adds the player direct-route and virtual-
  combat policy; Schema 60 gives Search-and-Destroy its separate operation type,
  immutable on-station assignment, displaced-fold return, and commander-recall
  settlement; schema 51 adds reciprocal enemy-order identity, outbound/
  return duty, two-sample physical arrival, defensive pressure, and typed
  survivor-resource settlement. `HST_MissionConvoyOperationService` separately
  owns the schema-52 convoy route, projection, arrival, and mission-outcome
  settlement policy. Live physical combat does not yet drive every player
  engagement, and no other support operation family is opted in.
- `HST_EnemyQRFOperationService` owns the enemy defensive-QRF admission and
  tick policy. It freezes one infantry roster from a distinct same-faction
  operational source for a same-faction defended target under resistance pressure, admits one held aggregate after
  the prepaid debit, suppresses parallel legacy authority, applies arrival
  pressure once, returns survivors to origin, and settles once or fails closed.
- Schema 69 adds a separate exact enemy-counterattack owner. It
  freezes one infantry roster, admits exactly one attack- or support-pool debit,
  advances a direct virtual route, preserves confirmed casualties across
  physical/virtual transfer, resolves deterministic virtual combat, delegates
  capture to canonical ownership authority, returns, and refunds surviving cost
  to the same originally charged pool. Historical counterattacks remain on
  contract `0`; malformed current graphs quarantine at `-69` without invented
  authority or effects. The appended `PREPARED` settlement intent orders
  prepare, stage, refund, record, and finalize across restore and same-session
  retry. Derived-ID claimant scans reject ambiguous residue. The scoped source,
  Foundation, all-target Workbench, and focused-engine proof pass. R27 proves
  the preceding production-owned materialize/physical/fold projection slice and
  owner-aware telemetry in Full Campaign Debug. R28 first failed the extended
  one-real-engine-death chain, unchanged R28b passed it, and R29 reproduced the
  earlier physical-confirmation race before `Kill`. R30 then exposed a bound and
  registered member becoming nonliving during an artificial one-second debug
  scheduling window. Current source retains bounded state-first physical
  settlement, keeps async settle stages yielding, and preserves same-invocation
  handoff -> `PHYSICAL` -> controlled fold/kill ordering before the sole-adapter
  casualty retirement and `N-1` fold/re-entry/final-fold continuity assertions;
  R31 passes all 11 assertions exactly once with bounded one-sample settles and
  exact-zero final drift. The current guarded fresh-process checkpoint separately
  retains outbound `VIRTUAL`; makes both raw `DEMATERIALIZING`/`LIVE` and
  `MATERIALIZING`/`STRATEGIC` production capture-deferral boundaries over the
  unchanged canonical `VIRTUAL` fallback; and adds genuine native counterattack
  `PHYSICAL`/`LIVE` capture. The physical cut requires the real root and living
  members, exact adapter/PhysicalWar bindings, an independent native live-position
  oracle before and after production persistence, normalized survivor continuity,
  continuation/replay, and zero residue. The three PREPARED cuts add the before-
  refund, after-refund/before-receipt, and after-receipt/before-finalize prefixes,
  one debit/one proportional refund, exactly-once terminal cleanup, and second-
  start no-op. The eighth cut adds raw-to-normalized owner-pending restore,
  exactly-once ownership completion, one production tick into raw/restored
  returning, and replay no-op with canonical non-overwrite identity. Final
  stamped Foundation passes 828 and Workbench passes at CRC `5fdd016f`. The
  final packed native owner-applied chain selects `new_campaign -> native ->
  native`, advances once, replays as a no-op, and cleans exactly. Native route
  travel, broader combat, every other dematerialization state, return travel,
  terminal outcome settlement, package/live server-client, performance, and soak
  remain open. Exact
  QRF/rebuild share only the static persistence preflight and normalization path,
  not this runtime proof.
- Schema 70 adds a separate exact enemy-garrison-rebuild owner. It freezes one
  capacity-bounded infantry roster, admits exactly one prepaid support-pool
  debit, and binds reciprocal order/operation/manifest/spawn/group authority.
  Confirmed casualties survive strategic/physical transfer and restore;
  delivered survivors become held garrison authority without aggregate double
  count. Ownership invalidation, admission failure, and prearrival termination
  use explicit rollback or proportional survivor refund. Historical rebuilds
  remain contract `0`. Malformed and orphan current graphs quarantine at `-70`,
  retaining linked claimants and pins without invented cleanup. Production
  restore reconciles PREPARED-with-receipt and SETTLED-before-order-tail crash
  windows idempotently. Selected ownership capability fencing rejects an initial
  ABA before pressure and a pressure-marked retry before order/debit. Foundation
  passes at 790 references, and current Workbench/focused checks pass. R10
  passes all five Phase 18 cases, exact clock/fingerprint isolation, the
  targeted Phase 22 identity/strategic/RUN assertions, and typed settlement
  cleanup with zero remaining open or runtime claimants. External runtime gates
  remain open.
- `HST_MissionConvoyOperationService` owns newly started exact convoy missions.
  It freezes one generated route, exactly three vehicle/crew elements and
  ordered crew slots, and mission-kind-compatible cargo/captive on vehicle zero.
  It advances them
  virtually, requests/observes the PhysicalWar projection near players, folds
  only when clear, preserves exact casualties, retains terminal-vehicle
  survivors as crew-only roots without vehicle resurrection, confirms arrival
  from route authority, and records one terminal receipt after the normal
  convoy outcome. Bubble ownership includes separated living/recovery roots.
- `HST_MissionConvoyP1Policy` owns exact-convoy cargo cardinality/role/kind/
  entity-prefab admission and legal lifecycle pairs.
  `HST_MissionConvoySaveValidationService` owns conservative schema-52
  migration, claimant validation, quarantine, and restore normalization.
- Exact-convoy persistence reconciles mapped physical members before every real
  capture. An open publication transaction or ambiguous mapping defers capture,
  flush, and savepoint request while retaining intent for bounded retry.
- `HST_StrategicMovementService` owns the two exact infantry-QRF consumers' and
  player Search-and-Destroy's direct campaign cursor at 2.5 m/s, derives ETA
  from distance, and limits
  catch-up to 30 campaign seconds per invocation.
- `HST_MaterializationService` owns those exact infantry operations' separate
  player-bubble in/out
  thresholds; the larger exit radius prevents projection thrash and contact
  blocks folding where that consumer owns engagement state.
- `HST_VirtualCombatService` owns only on-station exact player infantry-QRF and
  Search-and-Destroy combat against hostile abstract infantry at the target. It
  processes deterministic
  30-second steps, at most four per tick, and retires exact friendly slots while
  persisting both sides' fractional damage carry.
- `HST_OperationProjectionProofService` adds movement, hysteresis, exact roster-
  transfer, and combat/restore assertions beside the existing operation-record
  fixture. Source fixtures are not packaged runtime proof.
- `HST_PlayerSearchDestroyOperationProofService` adds Schema-60 quote/confirm
  replay, infantry-only roster/cost, operation/route, virtual combat, exact
  survivor projection, displaced-fold return-to-assignment, recall/settlement,
  legacy isolation, and quarantine assertions. Its save validator preserves
  pre-60 contract `0`, globally excludes quarantined groups from operational/
  combat presence, and retains malformed archive pairs. The proof also covers a
  valid paired capacity prune/restore and corrupt quarantine retention.
  Its casualty/fold/immediate-recall assertion operates on synthetic queue slots
  and proves bookkeeping only. It is not live adapter retirement, physical
  recall exit, packaged movement/combat, or real restart proof.
- `HST_MaidensBayLocationSaveValidationService` owns the anchor-gated location
  retirement. It canonicalizes mutable generic state only after selecting one
  authority, freezes all nonzero typed graphs, deep-clones frozen generated
  sites/routes into the canonical namespace, exposes detached historical lookup,
  and supplies old/canonical runtime ID equivalence. Its isolated both-row/old-
  only/idempotency proof is compiled and wired as
  `location_taxonomy.maidens_bay_schema60`, but the Campaign Debug assertion and
  packaged save/restart test remain unexecuted.
- `HST_PersistenceService` treats nonzero exact player-support contracts as
  fail-closed persistence claimants. It defers during materialization; otherwise
  it exhaustively reconciles exact-infantry casualties, requires unique
  reciprocal request/batch/group ownership and exact living binding cardinality,
  then refreshes each still-living physical support group's position before
  capture. A failure leaves checkpoint intent pending for bounded retry.
- `HST_EnemyQRFOperationProofService` adds six deterministic enemy admission,
  legacy-isolation, projection, settlement, restore, and rejection fixtures.
  They exercise in-process authority only; packaged AI movement, rendering,
  accounting, and real process restart remain open.
- `HST_MissionConvoyOperationProofService` adds nine aggregate assertions. Its
  admission/corruption subfixtures cover invalid cargo, foreign authority, seat
  topology, forged arrival receipts, legal lifecycle pairs, non-member casualty
  rejection, and missionless durable-claimant preservation. They remain source
  fixtures, not packaged engine proof.
- `HST_ForceSpawnQueueService` owns durable request/result/projection identity,
  exact required-slot admission, two-batch/eight-action tick acquisition,
  retry/deadline/cancellation cleanup, verified callbacks, 64-batch/512-slot
  active bounds, 64 slots per request, 128 terminal rows with explicit pins and
  a 600-second minimum retention window, production reporting, and once-per-
  restore reconciliation. Cleanup acquisition is dependency ordered as assets,
  members, vehicles, then group roots across bounded waves. Schema-50 strategic
  holds suspend physical queue work and make frozen member slots the exact
  virtual roster. Terminal entity/native-group IDs are historical evidence
  cleared on restore, not a living roster.
- Bounded selection now admits an ordinary `QUEUED` slot, or a due `DEFERRED`/
  `FAILED_RETRYABLE` slot when its owning batch can start another attempt.
  Frozen-manifest exactness and dependency registration remain selection gates;
  `StartAttemptIfReady` still advances the generation and normalizes slots only
  after acquisition. R22 passes retry generation/backoff, failed-slot-only work,
  stale-generation rejection, duplicate-safe completion/replay, same-wave
  sibling progression, interrupted-restore reconciliation, and exact
  registration. No schema/settings migration is required.
- `HST_ForceSpawnAdapterService` consumes queue work from the coordinator's
  one-second active-phase tick. The implemented slice accepts exactly one
  infantry group root with its required member slots, verifies exact prefab,
  liveness, faction, native group, Game Master hierarchy, and projection links,
  then records durable nonterminal `READY_FOR_HANDOFF`. It runs only after a
  held virtual projection is released for materialization, finalizes the exact
  group in `HST_PhysicalWarService` before `CompleteProjectionHandoff` records
  `SUCCEEDED`. Unsupported vehicle, asset, or multi-root manifests fail closed
  rather than materializing a shorter force.
- `HST_PhysicalWarService` exposes the narrow exact registration/handoff bridge
  and holds legacy spawn, member-repair, survivor, route, patrol, and cleanup
  paths while a queue-owned projection is not ready. This prevents duplicate
  broad-alpha population without changing unrelated physical-war ownership.
- HQ safety no longer uses one 900m rule for every purpose. That radius remains
  the hostile operation-staging exclusion; whole-location activation uses the
  location capture footprint with a 150m fallback, and static composition
  clearance uses 150m. The split is source-only until the next packaged test.
- Setup and won/lost phases do not run normal spawn acquisition. Instead, they
  request cancellation for every nonterminal batch and drain dependency-ordered
  cleanup with a monotonic runtime-only clock that does not advance campaign
  elapsed time. Schema-50/51 restore clears process-local IDs and normalizes
  every opted-in nonterminal exact infantry QRF—pending, interrupted handoff,
  or prior physical success—to one held virtual batch with the same exact
  survivor and casualty slots. A later materialization releases only survivors;
  the enemy slice additionally clears process-local physical-arrival samples.
  Player-paid infantry QRF, newly confirmed player Search-and-Destroy, and newly
  planned enemy defensive QRF use this production path; other support/order
  families remain on legacy services. Search-and-Destroy pre-60 rows stay
  contract `0`, while malformed current claimants use `-60` quarantine.
  Schema-54 policy-v2 purchased resistance garrisons now use one executable
  `NotSpawned` container root plus their arbitrary exact member slots, begin in
  held virtual local patrol, and rematerialize only durable survivors. Historical
  policy-v1, initial/enemy aggregate and vehicle garrisons remain legacy.
  Event-driven physical casualty subscription, vehicle/asset/multi-root
  lifecycle, other force consumers, and packaged physical/restart proof remain
  open.
- `HST_SpawnPlacementService` owns request/result placement for QRF staging, HQ
  attack standoff, convoy endpoints, dry-ground checks, vehicle-safe placement,
  road preference, and HQ standoff.
- Physical support deployments persist their resolved route id, placement
  type/summary, target/road/HQ distances, road resolution, vehicle-safe result,
  and linked active-group force counts so one-button debug can inspect the
  support request -> active group -> fold-back chain through save/load.
- Eligible unspawned non-queue-managed legacy support/QRF groups can still
  advance through their existing abstract policy. Each opted-in exact infantry
  QRF instead begins as a held queue projection and advances from its persistent
  direct-route cursor without a nearby player. Both materialize exact survivors
  inside the player bubble and fold the live position/roster outside the larger
  exit radius. The player policy can resolve bounded virtual infantry combat on
  station; the enemy policy applies one defensive-pressure outcome and returns
  to origin for proportional survivor settlement.
- A newly confirmed exact Search-and-Destroy uses the same held/direct-route/
  survivor boundary with its own type and $350 plus exact-slot HR quote. It
  virtualizes observed survivors, simulates on-station abstract infantry combat,
  returns to assignment after a displaced fold, and remains on station after
  hostile clear until commander recall settles eligible living HR.
- Materialized support retains the living-member centroid instead of elapsed-
  time interpolation. ETA opens the arrival check; two live samples from
  distinct elapsed seconds within 75m confirm physical arrival or recall exit.
  Exact QRF handoff normalizes to `support_active` so the same physical route
  owner observes it until it folds again.
- Populated spawned support receives a direct route from the current live
  position through a safe midpoint to the current target or recall exit. Stalled
  chains have a maximum of three consecutive reissues until an 8m new-best
  distance improvement resets the stall budget. Replacement is transactional:
  prepare a complete service-owned chain first, retain the old chain on failure,
  and remove/delete the old waypoints before attaching the replacement.
- Vehicle-capable support/QRF active groups now persist their selected vehicle
  prefab and spawn a linked runtime vehicle entity when the infantry group
  becomes live. Runtime vehicle roots are recursively cleared of engine faction
  claims so selected faction catalogs do not make the spawned vehicles
  player-unusable.
- Mixed personnel/vehicle groups no longer use an intact empty vehicle as a
  substitute for living personnel. After population was observed and both
  population guards expire, zero living infantry terminally clears strategic
  infantry/vehicle strength, resolves an incomplete linked QRF as failed,
  removes capture/marker pressure, and releases an intact vehicle as unclaimed
  `detached_active_vehicle` session salvage unless it already owns a durable
  field/loot/garage record, in which case that record and cargo remain durable.
  Terminal save normalization keeps
  those counts at zero. Vehicle-only projections and mission convoys retain
  their separate policies.
- Enemy support ledgers track recent damage pressure, cooldowns, max defense
  spend, same-zone stacking, and survivor refunds. Proactive background-war and
  HQ-pressure orders spend attack resources only, while QRFs, capture-triggered
  counterattacks, rebuilds, roadblocks, and direct enemy support requests spend
  support resources through the ledger.
- Enemy commander target scoring is relation-aware: same-faction holdings,
  resistance-held zones, and rival-held zones have distinct owner-score reasons,
  while hideout and mission-site bookkeeping anchors are excluded. The sealed
  Schema-68 checkpoint also reduces linked same-faction order/support/operation
  rows to one commitment root, rejects incompatible roots before weighted
  ranking, and scores compatible roots at `-12` each up to `-24`. Exact patrol
  remains compatible with a non-patrol defensive response. When order selection
  proposes another patrol, the target is excluded and selection reruns
  deterministically inside the same due decision rather than waiting another 180
  seconds. Blocking classification wins when a root contains mixed compatible/
  blocking rows. Queued and equivalent legacy/canonical targets participate;
  settled/terminal and rival rows are ignored, and the first diagnostic across
  multiple rejected targets is permutation-stable. Preparation freezes without
  pressure. Admission always rechecks commitments and active-order compatibility
  before debit, including pressure-marked retry; unpressured admission additionally
  rechecks candidate/source identity before pressure. The focused command-line
  engine case passes all 17 deterministic planner fixtures. R10 proves the held
  ambient cadence and exact clock/fingerprint boundary, but the separate enemy-
  target-scoring relation case still fails. Repair that resolver without
  regressing the planner fixtures, then rerun the affected coordinator path.
- Schema 62 replaces distributed ownership side effects with one revisioned
  durable receipt for military, mission, political, admin, debug, and migration
  causes. Retry-capable security/support work precedes owner publication. Exact
  patrol manifests require one reciprocal open operation and are revalidated on
  pre-owner retry. Later valid top-level requests are accepted as pristine queued
  receipts, then execute in array order. Nested political children defer
  publication to their capture parent. The active parent stages the complete
  logical marker snapshot, validates/releases the exact child graph, then commits
  or rolls back all rows/epoch/sequence. Command UI is resolver-first; marker rows
  only correlate and unsafe ones purge. Setup no-marker history survives
  activation. Restore
  accepts only pristine queued followers and quarantines multiple owner-applied
  publishers or an owner-applied publisher behind earlier unresolved top-level
  authority. It rejects projection/support/garrison/counterattack/reason/event/
  marker correlation failures. Support target authority is an exact sorted set
  with an ordered applied prefix and same-row event/delta evidence. Ownership
  strategic events exclude unrelated global deltas. Ownership retry then town
  policy run before setup/terminal returns
  with frozen-clock bypass; frozen-phase political repair suppresses fresh
  retaliation/notification, and retry preserves concrete quarantine causes.
  Invalid-owner repair reconciles restored receipts first, scans sequentially
  every five seconds, defers accepted/transient work, and quarantines only
  structural/manual-repair cases. Admin reports distinguish accepted-pending from
  rejected. Completion re-arms process-local persistence pending state. Receipt
  restore, migration, quarantine, and retention are implemented in source, and
  marker protocol `2` correlates each zone marker to its owner source revision.
  Foundation passes with 670 script-symbol references; Workbench validation is
  clean at CRC `22c13a32`, and the normal Script Editor open remained responsive
  without a crash. All packaged/native/restart evidence remains open.
- Static location map markers are visually separated from QRF tactical response
  markers. Towns, base/outpost/seaport/airfield locations, and mission-site
  anchors publish non-QRF icon hints. Schema-50 source resolves the radio icon
  by validated resource identity rather than a hard-coded array index, labels
  each zone with location and owner, and keeps the native map pointer above the
  support confirmation dialog. It also retains one nearby authored transmitter
  for a radio composition and lets destroy-target missions borrow it safely.
  The packaged map/dialog/radio proof remains open.
- Civilian town influence events track support, reputation, heat, population,
  police, roadblock, active/expired modifier counts, and political ownership
  consequences.
- Civilian/vehicle heat and undercover enforcement already cover request/apply,
  wanted heat, vehicle reports, passenger compromise, police/roadblock scans,
  and explicit reports.
- Civilian runtime classification now distinguishes stock town centers from
  minor localities. Nearby ambience no longer depends on the hostile military-
  active bit. True towns default to five driven vehicles in daytime/low heat when
  population and global Settings-24 budgets permit; the known woodland locality
  receives two pedestrians and no town traffic. Concrete appearance selection
  avoids same-town duplicates, and HST-owned ambient driver horn input is
  cleared. The sealed Blueprint Phase 8 ambient source/Workbench checkpoint applies leased fair allocation,
  four-root transaction-start scheduling, asynchronous readiness, immutable slots with
  slot-specific recovery routes, owner-aware static military refresh, per-frame
  claim observation, fail-closed pre-capture reconciliation, and transient-
  vehicle save filtering. The Campaign Debug Phase 20 helper uses the same production global
  plan and root transaction-start cap. Schema 60's native horn repair and the single Maiden's Bay
  Logistics Warehouse remain intact. Packaged behavior proof is still open.
- Sealed Schema 65 source/Workbench adds `HST_CivilianConsequenceService` and restore
  validation. A tracked ambient death is retained for a 256-row casualty queue;
  deferred theft has a 64-row queue. Both share four attempts per frame, bounded-
  backoff indefinite retry, and fail-closed capture deferral. Actor-local casualty
  evidence prevents callback/fallback duplication. Player-first observation
  promotes a civilian vehicle only for an exact pilot before a resistance-theft
  event is derived from its durable runtime ID; passengers only protect the root.
  Nearby-combat influence drains a pending receipt before any new edge, uses a
  revisioned danger episode/adopted floor/last-applied receipt, requires
  `episode - lastApplied <= 1`, and accepts current-operation or recent-fire
  evidence, never `HOT` alone. Exact canonical-town
  events persist an optional enemy aggression target/delta/before/after chain
  and require one matching applied strategic receipt. Minor localities remain
  panic-only and keep their conflicting-replay fingerprint map in session memory.
  Physical pedestrians use a move waypoint and RUN while panicked, then restore
  deterministic wander helpers and WALK after calm acknowledgement; route loss/
  stall has a separate bounded counter and no hot-path AI activation. Indexed
  restore validates structural links before live-preset enemy-role quarantine.
  Final stamped normal/all-five Workbench checks are clean at 5,802 Game files/
  11,728 classes and CRC `c0a672b9`; Foundation passes at 717 script-symbol
  references, while every native/package/soak gate remains open.
- The stamped Schema-60 performance repair keeps pure-vehicle groups on vehicle-only
  counts, skips redundant/no-convoy global survivor work, reuses registered
  player authority in recurring enforcement, attempts one unresolved radio-site
  discovery per tick, and builds expensive visual evidence only after its log
  throttle admits the line. The observed once-per-second stutter remains open
  until a fresh server measurement.
- Category mission selection is active: commander mission starts select a
  category, then the server chooses a valid definition and nearby eligible
  target.
- Campaign end rules now default to population support plus all-airfield
  control for victory and killed population greater than one third for loss,
  with legacy control-percent victory remaining optional.

## Next Engineering Milestones

1. Prove broader active-world records, Workshop/live server-client behavior,
   and networking. Preserve the closed periodic scheduler/debounce boundary and
   the historical counterattack and native-source
   matrices. Keep durable endpoint ABA snapshots as a separate future contract-
   2 schema decision whose schema number is not yet assigned; migration,
   markers, multiplayer/JIP/reconnect,
   performance, soak, and wider Campaign Debug failures remain open.
2. Retain the closed 35-assertion focused state-only boundary, then runtime-
   prove Schema-62 active security/economy/event consequences and every native
   caller; Schema-63 character/crew/platform classifications, all consumers,
   cooling and activation/deactivation; and Schema-64 real migration,
   serialization/restart, expiry, and rendered Map/War projection. Close the
   same authority across Full Campaign Debug, packaged server/client,
   multiplayer/network/JIP/reconnect, performance, and soak without treating
   the focused result as certification.
3. In the published server check, preserve
   stock HUD/Game Master access, require valid-sized location-plus-owner radio
   markers, pointer-over-dialog order, one authored transmitter per site, and
   correct destroy/rebuild binding. At Maiden's Bay require only the Logistics
   Warehouse. Measure the prior once-per-second stutter and listen around both
   civilian and other AI-driven wheeled vehicles for continuous horns.
4. Runtime-prove Schema-60 player Search-and-Destroy: immutable infantry-only
   quote/manifest, $350 plus exact-slot HR ledger, direct-route virtual travel,
   proximity materialization/fold, exact casualties, abstract combat, displaced-
   fold return to assignment, on-station hold after hostile clear, commander
   recall/living-HR settlement, archive replay, contract-0 legacy isolation,
   safe paired capacity eviction, corrupt quarantine retention, `-60` quarantine,
   and real save/restart. Require a live adapter-observed
   casualty, cardinality-checked projection/root retirement during fold, and a
   separate physical recall exit; synthetic queue-slot proof is insufficient.
5. Run Campaign Debug and then a packaged ten-town/ten-minute ambient soak that
   includes Figari, Morton, and the minor woodland locality. Prove deterministic
   leased fairness, the bounded four-root transaction-start cap, exact CIV group/waypoint
   readiness, exact pilot/engine/route readiness, the configurable daytime/low-
   heat traffic target (default five) when population/global budgets permit, two minor-
   locality pedestrians, silent movement, static military owner/policy refresh,
   teardown/re-entry, bounded recovery/recycle, and no once-per-second stutter or
   allocation churn. Prove per-frame brief enter/exit observation, the fail-
   closed capture barrier, promoted-root transform/destruction/cargo tracking,
   and occupied-only new-campaign retention through native autosave/restart and
   reset. Require two nearby durable rows with the same prefab to restore to two
   distinct physical roots without collapse. Campaign Debug Phase 20 must run the
   production global plan and four-root transaction-start cap; pure allocator/lifecycle/save proofs
   are insufficient. Also prove one native tracked
   casualty callback plus dead-character fallback deduplication, persisted-
   sequence receipt identity, civilian theft only after exact durable promotion,
   town event/aggression/strategic receipt replay, conflict and overflow failure,
   combat episode/clear/rebound with no `HOT`-only inference, and pedestrian move-
   waypoint RUN followed by acknowledged calm WALK recovery. Execute pre-65
   migration and current `-64` town/strategic then `-65` locality-envelope
   quarantine cases in dependency order. Treat minor-locality cross-process
   replay/conflict identity as open while its exact fingerprint remains session-
   only. Add the Schema-66 local-security matrix: exact authored 2–5 member
   roster, cyclic waypoint readiness, live casualties through fold/re-entry,
   restart with no refill, once-only police loss, no same-epoch resurrection,
   newer-owner/later-positive-event rearm, non-loss owner/pressure/stop
   settlement, zero resistance police/roadblocks, pre-66 migration, and `-66`
   quarantine.
6. Runtime-prove campaign-debug isolation, completion/cancellation/interrupted
   recovery, the repaired observations, and a clean development-session restart
   before interpreting a new full artifact.
7. Prove the schema-43 through sealed schema-66 authority chain, then the sealed
   Schema-67 resource authority under its exact source identity,
   across exact training,
   garrison, both player support types, SpawnQueue, strategic travel,
   materialization/fold, exact casualties, virtual combat, archive/recall,
   enemy-QRF/patrol/convoy policies, radio lifecycle, and save/restart replay.
8. Prove the schema-52 convoy and physical support route boundaries: three exact
   vehicle/crew elements, cargo/captive recovery, 3/3 drivers, interception,
   casualty-preserving fold/re-entry, arrival/outcome/marker cleanup, actual
   movement, two-sample arrival/recall exit, bounded waypoint replacement, and
   the mixed-QRF neutral-salvage terminal case.
9. Prove the implemented Schema-61 marker stream and Schema-66 marker-integrity
   repair with one host, two clients,
   disconnect/reconnect, and late join. Require equal epoch/watermark/hash,
   ordered revision/tombstone create/update/delete, forced gap/resync, map-close
   continuity, duplicate-free native cleanup, and real save/restart. Attempt
   delete/move/edit on protected campaign markers and require system ownership/
   non-removability or bounded self-heal; player-created markers must remain
   editable.
10. Runtime-prove schema-53 enemy patrol and schema-54 purchased-garrison patrol
   route/casualty/settlement/marker/restart behavior while historical patrols and
   policy-v1/aggregate/vehicle garrisons remain legacy.
11. Runtime-prove Schema-55/56/57 assassination guards, Schema-58 POW rescue, and
    Schema-59 radio sites without conflating their contracts. Cover native
    entities, exact casualties/assets, every typed outcome, quarantine, actual
    save/restart, rendered UI, setup, networking, reconnect, and JIP.
12. After this sealed checkpoint receives Campaign Debug and immediate packaged
    evidence, re-evaluate the remaining roadmap and runtime gates before
    selecting the next Blueprint source slice.

## Game-Mode Target

The player fantasy is FIA resistance on Everon with almost nothing. The US
occupying force owns towns, outposts, resources, factories, ports, and airfields.
The USSR invading force owns or contests selected zones. Players loot, recruit,
train, complete missions, capture zones, build support, and gradually turn the
island against the occupier.

Campaign pacing target:

- Early game: scavenge weapons, use civilian cover, ambush patrols, loot bodies,
  and avoid direct fights.
- Mid game: complete missions, capture weak outposts, build garrisons, garage
  captured vehicles, and improve training.
- Late game: coordinate squads, defend against counterattacks, attack airfields
  and ports, respond to QRFs, and push toward full-map victory.

## Strategic Map Model

Zones are economic, military, mission-generation, and victory nodes. Every
strategic zone should eventually carry:

- `zoneId`
- `displayName`
- `type`
- `ownerFactionKey`
- `position`
- `captureRadius`
- `activationRadius`
- `incomeValue`
- `priority`
- `support`
- `resistanceCaptureProgress`
- `garrisonSlots`
- `compositionId`
- `spawnProfileId`
- `patrolRouteId`
- `qrfRouteId`
- `missionSiteId`
- `linkedZoneIds`

`HST_ZoneState` already contains most of this model. Future work should refine
the existing record instead of creating a parallel strategic map.

## Town Model

Towns should not flip just because soldiers die nearby. Town control should be
driven by support and civilian conditions.

Town ownership inputs:

- resistance support
- occupier support
- civilian population
- civilian casualties by faction
- wanted heat
- police presence
- roadblock presence
- undercover restrictions

Rules:

- Town flips to FIA when FIA support is greater than occupier support.
- Town flips back or becomes hostile when occupier support is greater than FIA
  support.
- Civilian deaths and failed support missions reduce FIA support or increase
  wanted heat.
- Supply/city-aid missions increase FIA support.
- Enemy supply convoys can increase occupier support if they arrive.

`HST_CivilianZoneState` already has the first usable fields: reputation, wanted
heat, civilian presence, police presence, roadblock presence, last incident, and
undercover restriction.

## Progression Model

Partisan should use these progression axes as campaign-facing state:

- HR: recruitment capacity.
- Faction money: commander spending, training, vehicles, and support.
- Personal money: player rewards, transfers, and personal buys.
- Training level: FIA AI quality and equipment quality.
- War level: enemy/friendly equipment tier and mission difficulty.
- Aggression: enemy response budget, QRF chance, Petros attacks, and roadblocks.
- Town support: town ownership, HR income, and mission availability.
- HQ knowledge: chance/frequency of defend-Petros attacks.
- Arsenal unlocks: resistance equipment modernization.
- Garage inventory: captured vehicle progression.

## Standing Rules

- `HST_CampaignState` is the persistent source of truth.
- Campaign mutations must be server-authoritative.
- Persistent campaign state must not store raw `IEntity` references.
- Off-screen forces stay abstract.
- Physical AI activates around players, active objectives, QRFs, convoys, and
  important mission runtime.
- Every new feature needs a debug/report path in the command menu or logs.
- Every feature must be testable in `HST_Dev` before Everon polish.
- Do not add hidden addon dependencies or non-base-game asset assumptions.
- If a Reforger API call is unavailable or fragile, fail with a clear reason
  and preserve campaign state.

## Architectural Principles

### Server-Authoritative Campaign State

All campaign changes go through server-side services. Client UI sends requests;
the server validates phase, permissions, player identity, target IDs, distances,
costs, mission state, and cooldowns.

Never let a client directly mutate:

- money
- HR
- arsenal counts
- garage records
- mission success/failure
- zone ownership
- garrisons
- support requests
- enemy orders
- undercover state

### Abstract First, Physical Second

For every system, define abstract campaign state before physical implementation.

Example:

- Abstract: garrison at an outpost has eight infantry and one vehicle.
- Physical: when players enter activation radius, spawn a group and vehicle.
- Fold-back: when players leave, count survivors and fold them back into the
  garrison.

`HST_PhysicalWarService` already follows this shape for zone activation and
survivor fold-back. New features should keep using this pattern.

### Physical Entities Are Disposable

Every spawned entity should have a state record that can survive the entity
being deleted or the server restarting:

- runtime entity ID
- mission instance ID, zone ID, or group ID
- prefab
- position
- angles
- spawned flag
- destroyed/recovered/deleted flags

If the server restarts, Partisan should recreate, abstract, or safely discard
physical projections based on persistent state. It must not rely on raw entity
handles as truth.

### Deterministic Generation

Generated sites, routes, mission targets, convoy starts, and support spawn
points should come from stable inputs:

- campaign seed
- zone ID
- mission ID
- elapsed-second bucket
- war level
- faction key

Avoid pure random calls that cannot be reproduced or diagnosed after restart.

### Small Vertical Slices

Every implementation task should be shippable and testable in `HST_Dev` before
Everon polish. A good phase slice defines:

- goal
- files to inspect
- files likely to modify
- state fields changed
- server methods added
- UI/debug command added
- acceptance tests
- do-not-change constraints

## Service Ownership Map

### Coordinator

File: `Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c`

Responsibilities:

- create services
- restore/create campaign state
- apply settings
- register players
- tick campaign services
- route UI/RPC commands
- broadcast mission/support/order/capture events
- refresh markers
- trigger persistence checkpoints

Rules:

- Keep coordinator thin.
- Do not put domain logic directly in coordinator.
- Add services for new domains.
- Add coordinator methods only as authenticated server entry points.
- Every coordinator command should return a human-readable result string and
  optionally emit a feed/event entry.

Baseline acceptance:

- Server starts `HST_Dev`.
- Coordinator initializes once.
- State schema version is current.
- Menu opens.
- Manual save works or reports a clear not-available reason.
- Mission start command works.
- No service throws null-state errors during a short idle smoke pass.

### Campaign State

File: `Scripts/Game/HST/State/HST_CampaignState.c`

Persistent state should remain the only durable truth. Future fields should fall
into one of three patterns.

Durable campaign facts:

- zone owner, ownership revision, and active/latest transition backlinks
- ownership transition receipts and ordered completion evidence
- town support
- arsenal item count
- garage vehicle record
- player membership
- HQ position
- Petros alive/dead

Active runtime records:

- active mission
- active group
- support request
- enemy order
- convoy runtime
- runtime vehicle
- mission asset

Transient diagnostics:

- last spawn failure
- last vehicle target reason
- last UI result
- last mission runtime event

Acceptance pattern:

- Start campaign.
- Start mission.
- Spawn runtime assets.
- Save.
- Restart.
- Verify mission, assets, markers, resources, HQ, and players restore.
- Complete mission.
- Save again.
- Restart.
- Verify completed mission is not physically respawned.

### Enemy Strategic Resources

Files: `Scripts/Game/HST/Services/HST_EnemyStrategicResourceService.c` and
`Scripts/Game/HST/Services/HST_EnemyStrategicResourceSaveValidationService.c`

Sealed Schema-67 source ownership:

- One versioned `HST_FactionPoolState` owns attack resources, support resources,
  aggression, cadence accumulators/checkpoints, and its operational receipt count
  for each configured enemy role.
- `HST_EnemyStrategicResourceService` owns enemy income, spend, refund,
  aggression, and live admin/debug adjustment. Callers provide stable source
  identity and immutable facts; they do not write balances directly.
- A bounded `HST_EnemyStrategicMutationState` receipt freezes mutation identity,
  faction, kind, per-faction operational sequence, before/delta/after values,
  source/time, and required exact order/operation/accounting links. Accepted
  zero-effect operational commands retain a receipt.
- Operational receipts are never compacted or evicted. Each faction's sequence
  stops accepting operational mutations after 4,096 rows; the rival faction and
  compact per-faction periodic evidence continue independently.
- Exact replay is read-only. Conflicting identity, invalid role, underflow,
  overflow, cap exhaustion, or a broken required backlink fails before any
  value or revision changes.
- Exact defensive-QRF and enemy-patrol debit/refund policies remain in their
  operation services but settle resources only through this shared authority.
  Restore verifies the order mutation IDs and faction/operation/manifest/zone,
  the unique defense ledger, and typed town/ownership sources. Unsupported enemy
  orders remain legacy/deferred.
- Restore adopts pre-67 valid balances/aggression as a baseline without
  synthesizing history, initializes cadence checkpoints from legacy accumulators,
  and quarantines malformed current authority at `-67`.

This Schema-67 service does not own target scoring or order selection. Sealed
Schema 68 owns the separate per-enemy cadence and frozen decision
fingerprint without taking over Schema-67 resources.

### Ownership Transition

File: `Scripts/Game/HST/Services/HST_OwnershipTransitionService.c`

Responsibilities:

- admit one immutable expected-owner/revision request per location, including a
  pristine accepted/needs-retry receipt behind an earlier top-level publisher
- canonicalize supported zone aliases before fingerprinting or replay
- reject revisions that cannot safely advance exactly once into valid save state
- enforce cause policy in admission and current-save normalization
- execute unresolved top-level receipts in array order before any domain mutation
- settle exact patrol and aggregate security before owner publication
- apply frozen support, owner revision, town/facility/logistics policy,
  retaliation, economy/outcome, events, projection, notification, and
  persistence through one durable checklist
- resume coherent incomplete receipts on the bounded runtime tick or restore,
  including setup/terminal phases with frozen campaign time
- require every later unresolved top-level follower to remain pristine
- delegate nested political publication to the parent capture
- ignore queued pre-owner receipts during ordinary marker rebuilds and keep
  command UI resolver-first, using retained markers only as correlation
- reconcile durable civilian ownership thresholds every five seconds without
  duplicating an already queued political request
- apply an exact linked-town support fact once and submit its threshold receipt
  immediately to canonical FIFO ordering; defer only the fallback scan
- freeze the exact sorted support-target set; require an ordered applied prefix
  and the same single deterministic event row/deltas per applied target
- run town policy after ownership retry and before setup/terminal returns with a
  frozen-clock bypass; suppress fresh retaliation/notification for frozen-phase
  political repair
- stage the full logical marker array/epoch/sequence, validate and release every
  exact child receipt-zone-marker chain, then commit or roll back the full snapshot
- preserve immutable setup-without-markers receipt history through activation
- share one logical marker-snapshot builder between production and proof
- reconcile restored receipts before sequential five-second invalid-owner repair
  and quarantine only structural/manual-repair cases
- propagate parent quarantine iteratively during restore, independent of row
  order
- reject unrelated forged projection children and duplicate completed authority
  for one zone/applied revision
- validate bounded reason, stable garrison rows, counterattack decision/order,
  strategic/campaign event, marker, and setup-mode correlations
- keep ownership strategic events receipt-scoped to owner/capture/aggression facts
  and exclude unrelated queued/retry global deltas
- preserve concrete quarantine reasons across runtime/restore retry
- purge unsafe zone-marker rows and fail projection closed without exact resolver
  authority
- re-arm process-local persistence pending state after final completion/backlink
  mutation without extending an existing deadline
- retain bounded replay authority and quarantine malformed current graphs

Rules:

- Every post-bootstrap owner change uses this service, including military,
  mission, political, admin, debug, and migration repair.
- Exact mission, political, admin, and migration intent remains durable while
  queued; explicit-ID reconstruction reuses frozen preconditions only after
  semantic identity matches, and admin reports distinguish accepted-pending from
  rejection.
- Queued followers may coexist only while every later unresolved top-level
  receipt is pristine; pre-owner status alone is insufficient. Multiple owner-
  applied
  incomplete top-level publishers, or an owner-applied publisher behind earlier
  unresolved top-level authority, are ambiguous restore authority.
- A retryable blocker cannot expose the new owner or marker.
- Identical replay changes nothing; fingerprint conflict or stale precondition
  rejects.
- Projection-local marker revision never substitutes for zone source revision.
- A retained marker may veto a mismatched resolver result but never authorize one.
- A published parent has no unreleased direct child; a released child has exact
  parent, receipt, zone, marker, event, reason, and setup-mode correlation.
- Latest, incomplete, quarantined, unreleased-child, and unresolved-order
  receipts are not pruning candidates.

Baseline acceptance:

- Military and political flips each advance one revision and produce one latest
  reciprocal receipt.
- Enemy recapture repeats the same contract without reusing a request ID.
- Interrupted publication resumes from save without replaying completed domain
  effects.
- Exact unsupported security blocks before owner mutation.
- Parent/child changes publish one coherent marker graph whose source revisions
  match their zones.
- A two-child staged publication releases neither child and restores the complete
  prior marker snapshot when either marker is missing, then releases both and
  commits together after both exact chains match.
- One queued political event remains exactly-once across pending and completed
  save/restore boundaries plus repeat reconciliation.
- Pre-62 owners remain unchanged as revision-1 baselines; malformed current
  authority stays quarantined.
- Missing retained-marker authority reports publication unavailable rather than
  leaking a prior/raw owner.
- Applied support checklist evidence must be exact; missing or mismatched event/
  delta facts quarantine rather than skipping the consequence.
- Setup-without-markers history remains unchanged after active marker rebuild.
- Completion re-arms a lost process-local checkpoint deadline after restore.

## Legacy Roadmap Phase Status

| Phase | Name | Status |
| --- | --- | --- |
| 0 | Stabilize project rules and validation | Complete |
| 1 | Mission runtime visibility and diagnostics | Complete |
| 2 | Convoy runtime report | Complete |
| 3 | Convoy route state | Complete |
| 4 | Convoy readiness gating | Complete |
| 5 | Convoy vehicle-control adapter | Complete |
| 6 | Real convoy crew seating | In progress - schema-52 exact projection is source-complete; 3/3 drivers need packaged proof |
| 7 | Convoy waypoint-chain movement | In progress - schema-52 route authority is source-complete; live waypoint proof pending |
| 8 | Convoy progress, fold, and destination arrival | In progress - schema-52 virtual/physical route cursor and fold are source-complete; packaged proof pending |
| 9 | Convoy contact behavior | Complete |
| 10 | Generic convoy completion | Complete for legacy path; exact schema-52 settlement source-complete |
| 11 | Mission-specific convoy outcomes | Complete; schema-52 operation defers to this once-only owner |
| 12 | Active mission persistence | Complete in source; schema-52 exact convoy restart proof pending |
| 13 | Non-convoy mission primitive hardening | Complete |
| 14 | Arsenal, loot, and finite/infinite unlock loop | In progress - loadout smoke pending |
| 15 | Garage and vehicle persistence | In progress - broad-alpha scaffold exists |
| 16 | Recruitment, training, and garrisons | In progress - recruitment/garrison foundation exists |
| 17 | Zone capture and ownership | In progress - Schema-62 canonical ownership, Schema-63 shared combat presence, and Schema-64 town influence are sealed. Their focused state-only engine groups pass 14/14, 9/9, and 12/12 respectively; native classifications/consequences, persistence/restart, rendered Map/War, packaged client/network behavior, performance, and broader encounter depth remain pending. |
| 18 | Enemy commander physical responses | In progress - exact defensive QRF/patrol operations remain narrow; sealed Schema 67 canonicalizes resources/aggression, Schema 68 persists commitment-aware planning, Schema 69 adds exact newly admitted counterattacks, and Schema 70 adds exact newly admitted garrison rebuilds. The `delivery_pending` and `physical_live_fold` cuts pass fresh-process recovery/replay with exactly-once delivery, scoped native handoff and measured movement, exact production fold, roster/casualty continuity, read-only replay, and zero cleanup. Natural route/combat, other force families, multiplayer/JIP, and soak remain open before widening the planner. |
| 19 | Support requests | In progress - exact player infantry-QRF strategic projection; broad legacy supports remain |
| 20 | Civilians, town support, and undercover reports | In progress - Settings 24 supplies ambient budgets; sealed Schema 65 adds exact consequences/panic; sealed Schema 66 source/Workbench adds exact enemy-town local-security roster authority and zero resistance automatic police/roadblocks. Campaign Debug, packaged native behavior, save/restart, marker input, and ten-town soak remain open. |
| 21 | Undercover enforcement and police/roadblocks | In progress - scan enforcement remains broad alpha; Schema 66 now owns exact enemy-town patrol casualties/fold/rearm, while native encounters and balance remain unproven |
| 22 | HQ threat and Defend Petros | In progress - HQ threat and defense broad alpha |
| 23 | UI and map marker polish | In progress - Schema-62 protocol `2` remains sealed atop Schema 61; sealed Schema 66 source/Workbench repairs system ownership/non-removability and self-heal for campaign markers while preserving editable player markers. Rendered manual edit/delete/move, host/two-client/reconnect/JIP, and nested publication await packaged proof |
| 24 | Balance, campaign pacing, and victory/loss | In progress - population outcome default |
| 25 | Full-campaign soak testing | Planned |

## Roadmap Sequencing Rationale

The roadmap is ordered so each phase increases confidence before increasing
complexity. Phases 0-2 establish the diagnostic foundation: campaign health,
mission runtime state, and convoy internals are visible before movement,
persistence, and player-facing polish are expanded.

Phases 3-12 focus on convoy, mission, and persistence hardening because convoy
runtime crosses the highest-risk boundaries first: generated sites, routes,
spawned assets, active groups, AI behavior, mission cleanup, and save/load
restore. Getting that slice stable gives the rest of the campaign a repeatable
pattern for runtime projections of persistent state.

Phases 13-19 broaden the campaign loop into additional mission families,
commander behavior, physical war activation, zone capture, garrisons, and
support/QRF systems. Phases 20-24 then deepen the player-facing resistance
systems: civilians, undercover rules, economy, HQ logistics, UI, and balance.
Phase 25 is deliberately a hardening and soak phase rather than a feature phase;
it exists to prove the whole system can survive long server sessions and
save/load cycles.

## System Roadmap

These system notes are cross-cutting guidance for the phase list. They describe
where each major campaign subsystem is headed and what a future phase should
protect while implementing it.

### Setup And Campaign Phase

Purpose: start in setup, let the commander choose HQ, then move to active
campaign play.

Required behavior:

- `CAMPAIGN_SETUP`: allow admin/commander to select initial hideout; block
  missions, income, enemy commander, and capture.
- `CAMPAIGN_ACTIVE`: allow missions, income, enemy orders, support, capture,
  HQ movement, and checkpoints.
- `CAMPAIGN_ENDED`: freeze campaign mutation and show victory/loss state.

Future hardening:

- Add a central `CanMutateCampaign(actionId)` helper.
- Reject mission/economy/enemy/capture actions during setup.
- Add command-menu status explaining blocked actions.
- Add dev report for phase and HQ state.

Acceptance:

- Starting a mission before HQ selection returns a clear failure.
- Selecting HQ deploys Petros/cache/arsenal/tent/spawn-point.
- Moving HQ updates all HQ runtime positions.
- Save/load preserves selected HQ.

### Persistence And Schema Migration

Purpose: make the war survive restarts.

Persist:

- campaign metadata
- phase
- elapsed seconds
- resources
- players
- HQ
- zones
- garrisons
- active groups
- missions
- mission assets
- runtime vehicles
- arsenal
- garage
- support requests
- enemy orders
- civilian state
- undercover state
- generated sites/routes
- tasks

Do not persist as required truth:

- raw `IEntity` pointers
- `AIGroup` pointers
- spawned entity object handles
- temporary UI selection
- unsynchronized client-only data

Future smoke tools:

- `persistence_smoke_prepare`: creates test arsenal item, garage vehicle, active
  mission, mission asset, and active group, then forces checkpoint.
- `persistence_smoke_verify`: verifies restored records and reports missing
  categories separately.

Acceptance:

- Run prepare, save, restart, verify.
- Verification reports each category separately.

### Map Zones, Sites, And Routes

Purpose: drive income, missions, enemy orders, garrisons, QRFs, convoy routes,
town support, and victory.

Required zone types:

- town
- outpost
- resource
- factory
- radio tower
- airfield
- seaport
- bank
- police station
- roadblock
- support/stash/crash site
- HQ hideout

Required generated content:

- primary objective position
- secondary objective position
- roadblock/convoy ambush point
- support/cache point
- civilian stash
- crash/salvage point
- patrol route
- QRF route
- convoy route

Route model target:

```c
class HST_RouteWaypointState
{
	string m_sRouteId;
	int m_iIndex;
	vector m_vPosition;
	int m_iRadiusMeters;
	string m_sHint; // road, junction, bridge, destination, fallback
}

class HST_GeneratedRouteState
{
	string m_sRouteId;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	vector m_vStartPosition;
	vector m_vEndPosition;
	int m_iDistanceMeters;
	bool m_bRoadRoute;
	bool m_bValidatedForVehicles;
	ref array<ref HST_RouteWaypointState> m_aWaypoints;
}
```

Acceptance:

- Generated content report shows sites and routes.
- Every strategic zone has a primary site.
- Every outpost/resource/town has at least one mission-compatible site.
- Every convoy-capable zone has a vehicle-safe start and destination.
- Generated route positions are dry ground.
- No generated route starts inside HQ safe radius.

### Factions And Templates

Purpose: define units, vehicles, loadouts, equipment tiers, group prefabs,
patrol types, QRF types, support capabilities, and war-level scaling.

Fixed Everon preset:

- FIA: player resistance.
- US: occupying force.
- USSR: invading force.

Target faction contract:

```c
class HST_FactionTemplate
{
	string factionKey;
	string displayName;
	string sideKind; // resistance, occupier, invader
	array<string> infantryGroupPrefabs;
	array<string> patrolGroupPrefabs;
	array<string> qrfGroupPrefabs;
	array<string> vehiclePrefabsByTier;
	array<string> convoyVehiclePrefabsByTier;
	array<string> staticWeaponPrefabs;
	array<string> civilianVehiclePrefabs;
	array<string> startingRebelItemPrefabs;
	array<string> lootBlacklistPrefabs;
	array<string> unlockBlockedPrefabs;
	bool supportsHelicopter;
	bool supportsArmor;
	bool supportsArtillery;
	bool supportsAirstrike;
}
```

Slices:

- Keep base US/USSR/FIA templates and validate every prefab on server start.
- Add war-level equipment tier selection.
- Have mission runtime and physical war ask faction templates for composition.
- Add faction capability flags.
- Disable unsupported support calls and mission families honestly.
- Add future mod preset extension points without hidden dependencies.

Acceptance:

- Faction report lists valid group prefabs and vehicle prefabs.
- Invalid prefab is logged once and skipped.
- War level changes selected vehicle tier.
- Convoy vehicle selection never returns empty when convoy mission is enabled.

### Economy, HR, War Level, And Aggression

Purpose: pace the campaign.

Income tick:

- FIA towns generate HR and money.
- FIA resources generate money.
- FIA factories modify resource income.
- FIA seaports modify HR/vehicle availability.
- Enemy zones generate enemy attack/support resources.

War level increases from:

- captured territory
- captured high-value zones
- optional elapsed time
- optional successful missions

War level affects:

- enemy equipment tier
- convoy composition
- QRF strength
- mission difficulty
- support response type

Aggression increases from:

- killing enemies
- completing hostile missions
- taking flags
- destroying assets
- shooting civilians or prisoners
- capturing zones

Aggression decreases from:

- time decay
- failed rebel actions
- possibly aid/support actions

Aggression affects:

- QRF chance
- roadblocks
- patrol density
- Petros attack chance
- support calls
- enemy resource spending priorities

Future report:

- `BuildEconomyBreakdown(state)` showing income sources by zone type, enemy
  resource income by faction, and aggression decay timer.

### HQ, Petros, Members, Guests, And Commander

Purpose: define the resistance hub and permissions.

Required HQ objects:

- Petros
- arsenal/cache
- garage/vehicle box
- tent/rest point
- map/whiteboard
- flag/recruitment point
- optional build box

Petros behavior:

- Petros alive: missions and HQ actions are available.
- Petros killed: apply HR/money penalty, fail defend-Petros mission, increase
  HQ pressure, and enter recovery/campaign-loss flow depending final design.

Authorization shape:

- Admin: reset, force save, debug, membership management.
- Commander: spend faction money, move HQ, train troops, recruit squads, manage
  garrisons, request support, start missions, manage roadblocks/watchposts.
- Member: request missions, recruit personal squad, use arsenal within limits,
  garage vehicles if allowed, loot/deposit.
- Guest: limited access and no limited arsenal items unless policy allows.

Future hardening:

- `CanUseAction(playerId, actionId)`.
- Action category: admin, commander, member, guest.
- UI disabled reason instead of hiding all blocked actions.
- Membership report.

### Arsenal, Looting, Loadouts, And Unlocks

Purpose: complete the loot-to-unlock resistance progression loop.

Current direction:

- `HST_ArsenalService` owns deposits, counts, unlock thresholds, finite items,
  infinite unlocked items, and issued-item accounting.
- `HST_LootService` scans nearby entities, collects eligible inventory/loose
  items, skips blocked/friendly/player-owned items, and captures vehicles into
  garage records.

Target behavior:

- Area loot deposits eligible items into campaign arsenal.
- Vehicle loot transfers eligible cargo into nearby vehicle cargo.
- Vehicle cargo can transfer to arsenal.
- Arsenal counts finite items.
- Items unlock when threshold is reached.
- Unlocked items become infinite.
- Blocked items never unlock or never deposit depending config.
- Loadout editor validates costs before applying.
- Failed loadout application is atomic and does not lose items.
- AI recruit loadouts can eventually draw from unlocked arsenal tiers.

Acceptance:

- Depositing an item increases arsenal count.
- Item unlocks at configured threshold.
- Withdrawing finite item decrements count.
- Withdrawing unlocked item does not decrement count.
- Blocked item does not unlock.
- Loadout apply is atomic.
- Save/load preserves arsenal counts and unlock state.

### Garage And Vehicle Persistence

Purpose: make captured vehicles part of campaign progression.

Target behavior:

- Capture nearby vehicle into garage.
- Validate vehicle root and reject parts/proxies/scenery.
- Validate vehicle is not occupied.
- Capture physical and virtual cargo.
- Delete physical vehicle only after garage record is prepared.
- Redeploy vehicle near HQ at a safe slot.
- Restore cargo on redeploy.
- Track ammo/repair/fuel source vehicles.

Acceptance:

- Captured vehicle appears in garage report.
- Captured vehicle despawns from world.
- Vehicle cargo is preserved.
- Redeployed vehicle spawns at safe HQ position.
- Garage record is removed or updated according to redeploy policy.
- Ammo truck is recognized as ammo source.
- Save/load preserves garage vehicles and cargo.

### Mission Runtime

Purpose: make mission starts, objectives, runtime entities, cleanup, and
save/load deterministic.

Mission lifecycle:

- select eligible mission
- select target zone/site
- create active mission record
- create objective records
- initialize runtime primitive
- create mission assets/runtime entity records
- activate physical projection only when appropriate
- update objective progress
- complete/fail/expire with clear reason
- clean up disposable physical entities
- preserve durable campaign effects

Mission primitive targets:

- `kill_hvt`
- `hold_area`
- `clear_area`
- `destroy_target`
- `recover_cargo`
- `rescue_extract`
- `deliver_supplies`
- `convoy_intercept`
- `defend_petros`

Acceptance:

- Every active mission has an inspectable runtime report.
- Objective links are stable and visible.
- Missing target/site/entity data reports `none` or `missing:<id>`.
- Runtime cleanup does not erase durable rewards/penalties.
- Active missions survive save/load.

### Convoys

Purpose: make convoy missions a long-lived physical/abstract hybrid that can be
ambushed, tracked, saved, and resolved.

Generic convoy model:

- active mission owns convoy objective and mission assets
- each vehicle asset has source/current/target positions
- each active convoy group has faction, group prefab, crew count, vehicle state,
  fallback mode, spawn failure reason, and runtime status
- route state provides ordered waypoints
- readiness gate decides whether movement can begin
- vehicle-control adapter owns seating, driver, gunner, mobility, and route
  assignment checks
- progress tracker owns stuck detection and destination arrival
- contact detector owns ambush/contact phase transitions
- mission-specific hooks apply unique rewards/penalties

Schema-52 exact convoy cutover:

- only a newly started convoy mission receives operation contract version `1`;
  restored historical convoy rows remain contract version `0`
- admission freezes one generated road route, exactly three vehicle slots,
  three linked crew groups and ordered members, and mission-kind-compatible
  cargo/captive on vehicle zero; invalid cardinality, role/kind, or entity
  prefab fails admission
- one durable convoy element owns each vehicle/crew pair across virtual,
  materializing, physical, folded, restored, and settled states
- off-screen travel advances the persisted route cursor; off-screen combat is
  not yet simulated
- near players, the PhysicalWar convoy adapter projects the exact surviving
  elements; a clear convoy folds outside the larger radius without applying a
  casualty or outcome; outbound publication is atomic across every vehicle,
  group, mapped member, and cargo participant
- route-authoritative arrival and durable zero-crew completion feed the existing
  once-only mission-specific outcome owner before operation settlement
- crew elimination with unresolved cargo/captive/vehicle recovery enters an
  on-station recovery hold that may fold/rematerialize until the required
  mission-specific outcome is resolved

Mission-specific convoy targets:

- `convoy_ammo`: captured ammo vehicle creates ammo-source effect or ammo reward.
- `convoy_armored`: captured armor can be garaged; composition scales with war
  level.
- `convoy_money`: full payout requires money vehicle/cargo delivered to HQ.
- `convoy_prisoners`: prisoners must be freed/extracted for full reward.
- `convoy_reinforcements`: arrival strengthens target garrison.
- `convoy_supplies`: arrival increases occupier support; captured/delivered
  supplies increase FIA support.

Convoy persistence:

- Save mission state, phase, route ID, route waypoints, vehicle assets, current
  positions, crew group states, runtime vehicle states, ETA, objective progress,
  and destroyed/captured/delivered flags.
- On load, do not assume physical entities still exist.
- If players are near, respawn physical vehicles/crew at saved positions and
  reassign route from nearest waypoint.
- If players are far, keep convoy abstract and advance/resolve by state.

### Support Requests

Purpose: represent FIA and enemy support as stateful, testable requests.

FIA support targets:

- supply support
- ground reinforcement
- mortar support only if an honest asset path exists
- vehicle delivery
- optional extraction later

Enemy support targets:

- QRF
- patrol/search
- ground reinforcement
- abstract helicopter-style support when no safe asset path exists
- artillery only if honest asset path exists

Request contract:

- validates cost
- validates cooldown
- records source and target
- has ETA and status
- physicalizes near players
- resolves abstractly when off-screen

Acceptance:

- FIA support request appears in support report.
- Enemy support request spends enemy resources.
- Request has ETA and status.
- Request resolves physically when players are nearby.
- Legacy requests may resolve abstractly when players are far; an exact paid
  infantry QRF instead remains virtual and continues its operation.
- Cooldown prevents spam.
- Save/load preserves active support requests.

### UI, Map Markers, And Player Feedback

Purpose: make the campaign understandable without reading logs.

Current command menu tabs:

- Setup
- Overview
- HQ/Petros
- Missions
- Map/War
- Forces
- Arsenal/Loot
- Members
- Admin

Target UI panels:

- Overview: resources, HR, war level, aggression, support, active missions.
- HQ/Petros: HQ status, Petros health, move HQ, train troops, save.
- Missions: available missions, active missions, objective progress, runtime
  report, admin abandon/fail debug.
- Map/War: zones by owner, garrison strength, town support, enemy orders,
  QRF/support status.
- Forces: FIA garrisons, active groups, recruitment, training.
- Arsenal/Loot: loot nearby, loot to vehicle, arsenal report, loadout editor,
  garage report, capture/redeploy vehicle.
- Members: commander, members, guests, permissions.
- Admin: reset, save, debug resources, force mission, spawn test convoy,
  persistence smoke tests.

Marker targets:

- active mission marker
- objective marker
- convoy start marker, if useful and not too revealing
- convoy last-known marker
- convoy destination marker, possibly approximate
- QRF/support inbound marker
- capture progress marker
- zone ownership marker
- HQ visibility rules

## Testing And Validation Strategy

### Static And Compile Gates

- PowerShell foundation validation script.
- Resource path validation.
- Missing prefab report.
- Enum/string constant validation.
- State schema migration validation.
- No hidden dependency checks.

### Focused Command-Line Engine Autotests

- Use the official `SCR_Autotest` Game-process runner for deterministic service
  cases that do not require HST_Dev, a coordinator, players, or world entities.
- Require JUnit and the failed-list artifact to agree with console evidence.
- A passing focused case closes only its named service assertions. Campaign
  Debug isolation, persistence/restart, package identity, and networking remain
  separate cumulative rungs.
- The current planning case is
  `HST_TEST_EnemyPlanningCommitmentAuthority`: one testcase, zero failures, an
  empty failed list, and `AllExact()` true across all 17 deterministic
  Schema-68 planning fixtures, including retry-quarantine repeated-pass
  idempotency, in `logs_2026-07-13_11-44-28` with JUnit timestamp
  `2026-07-13T15:44:34.667Z`.

### Service Smoke Tests

In Reforger scripting these are usually dev commands or smoke services:

- economy tick test
- arsenal deposit/unlock/withdraw test
- garage capture/redeploy test
- mission start/complete/fail test
- route generation test
- convoy spawn test
- persistence prepare/verify test

### HST_Dev Scenario Tests

- Start campaign.
- Select HQ.
- Start each mission family.
- Spawn each runtime primitive.
- Complete/fail by interaction.
- Save/load.
- Repeat as local host.

### Full Everon Soak

- Two-hour idle with no players moving.
- Two-hour mission loop.
- Multi-client join/leave test.
- Save/restart under active mission.
- Active convoy save/restart.
- Zone capture/counterattack loop.

### Convoy-Specific Test Matrix

Route generation:

- valid route
- no route
- route too short
- route near HQ
- wet/water route

Spawn:

- three or more vehicles spawn
- one vehicle fails
- crew group fails
- no driver assigned
- invalid vehicle prefab

Movement:

- staging to moving
- waypoint assignment success
- waypoint assignment failure to static ambush/contact
- moving marker update
- stuck detection
- destination arrival

Combat:

- player approaches and contact starts
- kill one crew group and progress becomes 1/N
- kill all crews and objective succeeds
- destroy cargo vehicle and branch mission-specific result
- capture vehicle with living crew is blocked
- capture after crew eliminated is allowed

Persistence:

- save in staging
- save while moving
- save after contact
- save after one vehicle destroyed
- save after success before cleanup

## Task Template

Use this template for future implementation requests.

```text
You are working in N0ise9/Partisan.

Goal:
  [one specific behavior]

Context:
  Partisan is server-authoritative. Campaign state is the source of truth.
  Physical entities are runtime projections and must be restorable or safely
  abstracted. Do not add hidden dependencies or non-base-game assets.

Files to inspect first:
  [file list]

Files likely to modify:
  [file list]

Implement:
  1. [specific code change]
  2. [specific code change]
  3. [specific code change]

State changes:
  [new fields/classes, migration considerations]

UI/debug:
  [report/action/command to add]

Acceptance:
  1. [testable result]
  2. [testable result]
  3. [testable result]

Constraints:
  - Keep server-authoritative.
  - Do not trust client-supplied player IDs.
  - Do not store raw IEntity references in persistent state.
  - Do not break existing HST_Dev flow.
  - If native API is unavailable, fail with a clear reason and preserve state.
```

## Phase 0 - Stabilize Project Rules And Validation

Status: Complete

Goal: create a safe baseline so every later phase can be tested without
breaking the foundation.

Implementation:

- Run or extend existing validation tooling.
- Confirm all `HST_` prefixed scripts compile.
- Confirm `HST_Dev` starts.
- Confirm the campaign coordinator initializes once.
- Confirm the command menu opens.
- Confirm the campaign state schema version is current.
- Confirm manual save/checkpoint path does not error.
- Add a foundation status report if one is not already present.

Acceptance criteria:

- `HST_Dev` launches.
- Server coordinator initializes.
- Alpha command menu opens.
- Foundation status/report command returns campaign phase, schema version, HQ
  state, active mission count, and active group count.
- Manual checkpoint command returns a clear success or clear not-available
  reason.
- No new dependencies are introduced.

## Phase 1 - Mission Runtime Visibility And Diagnostics

Status: Complete

Goal: before improving missions, make active mission state visible and
debuggable.

Implementation:

- Add or improve mission runtime inspection.
- Report instance ID, mission ID, display name, target zone, site ID, runtime
  primitive, runtime phase, remaining seconds, objective totals, mission asset
  count, runtime entity count, and failure reason for each active mission.
- Add command-menu action `Inspect Active Missions`.
- Add per-active-mission inspection actions where an active mission exists.

Acceptance criteria:

- Starting any mission creates a visible active mission report.
- Report includes runtime primitive and runtime phase.
- Report includes objective progress.
- Report includes mission assets and runtime entities.
- Expired, failed, and completed missions show clear final status.
- Mission reports do not crash with missing target/site/entity data.

## Phase 2 - Convoy Runtime Report

Status: Complete

Goal: make convoy internals visible before changing AI movement.

Implementation:

- Add `BuildConvoyRuntimeReport(state, mission)`.
- Report mission instance ID, mission ID, phase, ETA, source position, target
  position, route/site ID, vehicle asset count, and mission failure reason.
- Report each convoy vehicle asset: asset ID, prefab, source position, current
  position, target position, spawned, destroyed, delivered/captured, and last
  interaction.
- Report each active convoy group: group ID, faction, prefab, spawned entity,
  runtime status, crew count, alive crew count, source position, target
  position, fallback mode, and spawn failure reason.
- Add Missions tab action `Convoy Runtime Report`.
- Add route/travel distance visibility and explicit static fallback when
  vehicle control is unavailable.

Acceptance criteria:

- Starting `convoy_ammo` in `HST_Dev` creates a convoy report.
- Convoy report lists all convoy vehicle assets.
- Convoy report lists all active convoy groups.
- Report shows whether each vehicle/group spawned.
- Report shows alive crew count.
- Report shows why movement or spawn failed.
- Report remains valid after convoy fails or completes.

Notes carried into Phase 3:

- Verified ground vehicle variety now uses live faction campaign entity
  catalogs first, then GUID-qualified base-game fallback resources.
- Real AI vehicle embark/movement is not implemented yet; convoy reports now
  expose this as a clear static-ambush fallback reason.

## Phase 3 - Convoy Route State

Status: Complete

Implementation/static validation complete. HST_Dev smoke test confirmed route
waypoint reporting, generated-route diagnostics, faction-catalog vehicle
variety, and clean convoy staging without resource-error spam.

Goal: move from start/end-only convoy behavior toward explicit route data.

Implementation:

- Add `HST_RouteWaypointState`.
- Extend `HST_GeneratedRouteState` with an ordered route waypoint array.
- Generate at least start, midpoint, and destination waypoints.
- Add route validation fields: road route, vehicle-safe, distance meters, and
  waypoint count.
- Add route reporting to generated content reports and convoy reports.
- Read base-game faction campaign entity catalogs for convoy vehicle
  candidates, then validate wheeled/tracked ground vehicles and exclude
  helicopters/aircraft.

Suggested state:

```c
[BaseContainerProps()]
class HST_RouteWaypointState
{
	string m_sRouteId;
	int m_iIndex;
	vector m_vPosition;
	int m_iRadiusMeters;
	string m_sHint;
}
```

Acceptance criteria:

- Generated content creates route waypoints.
- Every generated route has at least three waypoints.
- Convoy mission references a route or produces a clear fallback reason.
- Route report shows waypoint count and distance.
- Invalid route does not crash mission start.
- Convoy vehicle selection uses verified faction-catalog ground vehicle prefabs
  first, with GUID-qualified fallback prefabs only when the catalog is
  unavailable or sparse.
- Invalid or guessed vehicle prefab candidates are removed or skipped without
  resource errors.
- Ground-vehicle candidate report shows usable convoy vehicle counts per
  faction.

## Phase 4 - Convoy Readiness Gating

Status: Complete

Implementation/static validation and HST_Dev smoke testing complete.

Goal: prevent convoys from entering `convoy_moving` unless they are actually
ready to move.

Implementation:

- Separate vehicle asset exists, physical vehicle spawned, crew group spawned,
  crew alive, driver available, and waypoint/route assigned.
- Add readiness check before staging changes to moving.
- Remain staging while within grace period.
- Fall back to convoy contact/static ambush or fail with explicit reason after
  grace period.
- Add readiness status to convoy report.

Acceptance criteria:

- Convoy does not enter moving phase if no vehicles spawned.
- Convoy does not enter moving phase if crew groups failed.
- Convoy does not enter moving phase if route/waypoint assignment failed.
- Failure or fallback reason is visible in convoy report.
- Existing static fallback still lets players complete/fail the mission.
- Readiness report separates vehicle assets, spawned vehicles, crew groups,
  alive crew, driver availability, route assignment, and waypoint assignment.
- Convoy moving notifications, markers, and status changes are emitted only
  after readiness succeeds.

## Phase 5 - Convoy Vehicle-Control Adapter

Status: Complete

Goal: isolate Reforger vehicle seating, driver, gunner, and route-control logic
behind one adapter.

Implementation:

- Create `HST_ConvoyVehicleControlAdapter`.
- Add methods to bind crew, assign a vehicle route, count living crew, check
  for a living driver, and check whether a vehicle is mobile.
- Return explicit success/failure reasons from every method.
- Make `HST_PhysicalWarService` call the adapter instead of assuming
  `AddWaypoint` means the vehicle can move.

Suggested interface:

```c
class HST_ConvoyVehicleControlAdapter
{
	bool TryBindCrewToVehicle(
		HST_ActiveGroupState groupState,
		IEntity groupEntity,
		IEntity vehicleEntity,
		out string reason
	);

	bool TryAssignVehicleRoute(
		HST_ActiveGroupState groupState,
		IEntity groupEntity,
		IEntity vehicleEntity,
		array<vector> waypoints,
		out int assignedWaypointCount,
		out string reason
	);

	int CountLivingCrew(IEntity groupEntity);

	bool HasLivingDriver(IEntity groupEntity, IEntity vehicleEntity);

	bool IsVehicleMobile(IEntity vehicleEntity, out string reason);
}
```

Acceptance criteria:

- Project compiles with the new adapter.
- Convoy logic calls the adapter.
- Convoy report shows adapter result.
- Existing convoy behavior is preserved or fails with clearer reasons.
- No raw `IEntity` references are added to persistent state.

## Phase 6 - Real Convoy Crew Seating

Status: In progress - current production repair is compile-proven; fresh scoped runtime proof pending.

The older implementation/static smoke did not survive the later dedicated
runtime artifact: all three populated crews remained driverless through the
grace window. Current source repairs the ordering and authority path, but this
phase is not complete again until the scoped convoy proof passes.

Goal: make convoy vehicles actually controllable by AI.

Implementation:

- In `HST_ConvoyVehicleControlAdapter`, discover living group-controlled AI
  agents and vehicle compartments.
- Register each valid pilotable vehicle with the crew-group utility before any
  seating request, and verify retained registration directly.
- Assign the first suitable server-owned AI to the driver seat through forced
  authority-local compartment entry, with the owner RPC as fallback after a
  direct rejection or for a non-local entity.
- Assign the next suitable AI to gunner/turret seats if available.
- Assign remaining AI to cargo/passenger seats.
- Verify the driver is a living group-controlled occupant in a pilot slot.
- Add seating result fields to convoy reports: driver assigned, seated crew,
  turret seated, cargo seated, seating pending, and last seating reason.
- Retry seating during convoy staging before readiness checks.
- Keep convoy waypoint assignment blocked until every active convoy vehicle has
  a seated living AI driver.
- Enforce convoy start-to-end straight-line horizontal distance between 2000m
  and 5000m, and report planned distance, band validity, and whether generated
  route staging is vehicle-safe.

Acceptance criteria:

- Spawned convoy vehicle has an AI driver.
- Convoy report says driver assigned.
- Convoy report says how many crew were seated.
- Convoy report says planned distance is inside the 2000-5000m band.
- Convoy report says planned source/target are route-staged and vehicle-safe.
- Missing driver seats and missing crew agents produce clear reasons.
- Vehicle is not considered ready unless driver assignment succeeds.
- No persistent raw `IEntity` references or save-schema migrations are added.

## Phase 7 - Convoy Waypoint-Chain Movement

Status: In progress - implemented; fresh live waypoint-chain proof pending after seating repair.

The waypoint-chain implementation and static contracts exist, but the latest
driverless convoy artifact prevented a valid live chain. Fresh proof resumes
only after Phase 6 confirms every vehicle has a living pilot occupant.

Goal: replace single destination waypoint with an ordered route waypoint chain.

Implementation:

- Build an ordered `array<vector>` from route waypoints.
- Spawn an AI waypoint for each route point.
- Add waypoints to the convoy AI group in order.
- Optionally stagger departure by vehicle index.
- Store assigned waypoint count in `HST_ActiveGroupState.m_iAssignedWaypointCount`
  and report.
- Keep old single-waypoint path only as fallback.

Acceptance criteria:

- Convoy report shows assigned waypoint count.
- Waypoint count is greater than one for generated routes.
- Convoy enters moving phase only after waypoint-chain assignment succeeds.
- If waypoint spawning fails, convoy falls back to static ambush/contact with a
  clear reason.
- Vehicles attempt to travel toward the destination.

## Legacy Roadmap Phase 8 - Convoy Progress, Stuck Detection, And Destination Arrival

Status: In progress - schema-52 route/projection/fold authority is implemented
and source-validated; natural physical progress/intercept/fold/arrival proof is
pending.

For newly started schema-52 convoys, the persisted generated-route cursor now
replaces the travel timer as strategic movement authority. The exact operation
advances virtually at 9 m/s, materializes inside 1,800 m, folds outside 2,200 m
only when clear after its minimum physical window, preserves per-element state
and crew casualties, and requires two distinct physical route-end samples
within 50 m. Historical convoy rows keep the legacy behavior described below.

Implementation/static validation target: legacy roadmap Phase 8 now tracks transient convoy
progress in `HST_PhysicalWarService` without adding campaign save fields. A
smoke-test follow-up makes destination arrival win over single-group dismount
recovery, keeps convoy starts compact and vehicle-safe without shortening the
2000-5000m convoy trip, narrows arrival to 50 meters, and uses
RoadNetworkManager-resolved road endpoints, waypoints, and recovery snaps.
HST_Dev smoke testing passed for road-based spawning, movement, stuck recovery,
arrival failure with living crew, crew-neutralized completion, and inactive
convoy runtime cleanup.

Goal: track whether convoy vehicles are actually moving and respond if they get
stuck.

Implementation:

- Sync convoy vehicle positions every five seconds.
- Track distance to destination per vehicle.
- Track last movement/progress timestamp.
- Detect no-progress/stuck state.
- Reissue route once after a stuck threshold.
- Resolve convoy starts, destinations, assigned waypoint positions, and recovery
  snap points through RoadNetworkManager road geometry instead of terrain
  material or generic safe-ground checks.
- If stuck too long, snap the vehicle only to a nearby road-resolved point when
  the nearest player is at least 300 meters away.
- Keep physical and report the stuck reason when players are too close, no
  road snap point can be resolved, the point is too close to destination, the
  point would advance too far toward destination, or crew
  reseating cannot confirm a driver.
- Move unseated living crew to the recovered vehicle, reseat through the convoy
  vehicle adapter, orient the vehicle along route travel direction, then reissue
  route only after driver recovery succeeds.
- Keep marker refresh at 30 seconds.
- Fail convoy if it reaches destination with living crew.
- Resolve destination arrival before moving convoy dismount/contact fallback.
- Treat a single dismounted or driverless moving convoy group as a bounded
  reseat/rebind recovery issue instead of whole-convoy contact fallback.
- Stage convoy vehicle starts as compact road-resolved columns; destinations
  and all assigned waypoint positions must also be road-resolved.
- Choose a seeded random convoy start distance between 2000m and 5000m each
  time convoy assets are initialized.
- Pre-probe the full convoy vehicle column before creating mission assets; if
  any vehicle slot fails the safe-spawn or distance-band checks, reject that
  candidate and probe another instead of creating a partial convoy.
- Reject convoy spawn slots that are not RoadNetworkManager road points wide
  enough for vehicle footprint samples; terrain height/footprint checks remain
  secondary stability checks only.
- Re-check road placement during physical vehicle spawn and route-snap recovery,
  and use a smaller spawn lift to avoid dropping trucks onto uneven terrain.
- Preserve the existing 2000-5000m convoy travel-distance contract for every
  planned convoy vehicle slot.
- Keep convoy vehicle start slots in a compact route-column formation instead
  of scattering them through wide spawn-clearance searches.
- Wait for a reseated driver before consuming a route reissue attempt.
- Show live computed distance-to-destination in the convoy report, with sampled
  progress distance retained as a diagnostic.

Acceptance criteria:

- Moving convoy updates mission asset current positions.
- Convoy marker updates periodically.
- Convoy report shows distance to destination.
- Convoy report shows stuck/no-progress status.
- Convoy report shows RoadNetworkManager availability and road-resolved source,
  current, target, waypoint, and recovery placement results.
- Route is not reissued every tick.
- Convoy fails if destination is reached with living crew.
- Convoy does not falsely fail while still staging.
- A single dismounted convoy group does not immediately switch the mission to
  `convoy_contact`.
- Convoy report shows moving recovery reasons for dismounted or driverless
  groups.
- Convoy arrival failure uses a 50 meter destination radius.
- Convoy planned distance remains inside the 2000-5000m band for the convoy
  column, not only the first vehicle.
- Convoy setup creates either all planned convoy vehicle assets or none; it
  should not create a mission with later convoy vehicles guaranteed to fail
  physical spawn.
- Convoy pre-probing and physical spawn reject non-road, narrow-road, rocky, or
  uneven large-vehicle footprints instead of allowing trucks to spawn off-road,
  tipped, or on rocks.
- Convoy report shows route snap attempt/result, distance-to-destination, player
  gate, and crew reseat result.
- Convoy vehicles spawn as a compact column on generated route staging rather
  than hundreds of meters apart.
- Convoy Runtime Report shows the assigned road-snapped runtime waypoint chain
  per convoy group instead of treating static route-template waypoints as the
  authoritative movement path.

HST_Dev smoke steps:

1. Launch `Missions/HST_Dev.conf` in Workbench Play mode.
2. Press `I`, open Setup, and choose `Start HQ: central hills`.
3. Open Admin and run `Force: Ammo convoy`.
4. Open Missions and run `Convoy Runtime Report`.
5. Wait for the convoy to leave staging; verify phase becomes `convoy_moving`,
   waypoint count is greater than one, band valid is `yes`, and
   distance-to-destination appears. Vehicle source/current distances should all
   be inside the 2000-5000m band, and the vehicles should be a compact route
   column on roads, not scattered hundreds of meters apart, upside down, or
   sitting on visible rocks/steep side-tilt terrain.
6. Watch the map for convoy marker movement; it should refresh periodically,
   about every 30 seconds.
7. For unstuck testing, block a convoy vehicle or let terrain stop it, then
   move the player at least 300 meters away. After the hard-stuck window, the
   report should show route reissue, then route snap to a nearby road-resolved
   point, distance-to-destination for the snap point, road forward/orientation,
   and crew reseat result.
8. If one crew group dismounts, run `Convoy Runtime Report`; the mission should
   remain `convoy_moving` unless every moving group has lost control, and the
   report should show moving recovery status.
9. For arrival testing, do not kill the crew. When the convoy reaches
   within 50 meters of the destination with living crew, the mission should
   fail with `Convoy reached its destination with living crew.`
10. Repeat once with crew killed before arrival; the convoy should not falsely
   fail due to destination arrival after crew neutralization.

## Phase 9 - Convoy Contact Behavior

Status: Complete

Implementation/static validation: Phase 9 adds a 120 meter convoy contact
radius and transitions active convoy missions into `convoy_contact` when the
ambush starts. Contact is stored through existing active mission phase state and
does not add campaign schema fields. Static validation passed, and HST_Dev
smoke testing confirmed proximity contact without contact-only completion.

Goal: make ambushes transition convoys into a combat/contact state.

Implementation:

- Detect contact when a player is within 120 meters, crew count decreases,
  convoy vehicle is destroyed, vehicle is captured, or later when
  shots/explosions are nearby.
- Set mission phase to `convoy_contact`.
- Apply `convoy_contact` status to active convoy groups.
- Keep the objective active.
- Do not instantly fail or complete just because contact started.
- Preserve `convoy_contact` when generic mission vehicle destroy/capture
  handlers update convoy vehicle assets.
- Show contact radius and contact reason in the convoy runtime report.

Acceptance criteria:

- Approaching convoy changes phase to `convoy_contact`.
- Killing convoy crew changes objective progress.
- Destroying a convoy vehicle updates asset/runtime state.
- Convoy can still complete after contact.
- Contact phase survives save/load.

HST_Dev smoke steps:

1. Launch `Missions/HST_Dev.conf` in Workbench Play mode.
2. Press `I`, open Setup, and choose `Start HQ: central hills`.
3. Open Admin and run `Force: Ammo convoy`.
4. Open Missions and run `Convoy Runtime Report`; wait until the convoy has
   spawned and preferably reaches `convoy_moving`.
5. Approach within 120 meters of the convoy without killing anyone. Run
   `Convoy Runtime Report` again and verify mission phase is `convoy_contact`,
   convoy groups show `convoy_contact`, mission status is still active, and
   objective progress has not completed or failed from contact alone.
6. Kill one convoy crew group. Run the report and verify crew neutralized
   progress updates, the mission remains active/contact, and it does not
   instantly complete unless all required convoy crew groups are neutralized.
7. Destroy one convoy vehicle. Run the report and verify the matching convoy
   vehicle asset shows destroyed/runtime destroyed state while the mission
   remains `convoy_contact`.
8. Try capturing a convoy vehicle while its crew is alive; it should be blocked.
   Neutralize that vehicle's crew, capture it again, and verify the asset shows
   captured/delivered without generic phase `captured` replacing
   `convoy_contact`.
9. Neutralize all convoy crew groups after contact. Verify the convoy can
   progress to `convoy_eliminated` and then complete through the normal mission
   outcome path.
10. While the mission is in `convoy_contact`, run `Manual checkpoint`, restart
   or reload HST_Dev, and verify the active mission still reports
   `convoy_contact`.

## Phase 10 - Generic Convoy Completion

Status: Complete for the legacy path; schema-52 exact completion/settlement is
implemented in source and awaits packaged proof.

Implementation/static validation: Phase 10 makes convoy completion
server-authoritative through crew elimination and destination-arrival checks.
The convoy runtime report now exposes completion eligibility, crew progress,
living crew, and vehicle resolution counts. HST_Dev smoke testing confirmed
crew-gated capture, destroyed/captured vehicle exclusion, generic success on
all crews eliminated, failure on live-crewed arrival, preserved intact vehicles
after crew-elimination success, and failed convoy cleanup. Static validation
passed.

Goal: make generic convoy success/failure reliable before adding
mission-specific rewards.

Implementation:

- For a contract-version-1 convoy, use durable zero living crew across all
  active elements to complete the crew objective; missing runtime handles do
  not count as elimination.
- Keep the exact operation open in an on-station recovery hold when crew
  elimination leaves required cargo, captive, or vehicle recovery unresolved;
  settle only after that outcome or another terminal mission result.
- Suppress legacy timer arrival. Virtual arrival comes from route progress and
  physical arrival requires two distinct route-end samples.

- Count total convoy groups, eliminated convoy groups, and living convoy crew.
- Complete objective when all required convoy crews are eliminated.
- Fail mission when convoy arrives with living crew.
- Allow vehicle capture only after associated crew is neutralized.
- Cleanly mark destroyed/captured convoy vehicles.
- Preserve intact convoy vehicles after crew-elimination success so players can
  optionally capture them; destroyed vehicles and failed/arrived convoys still
  clean up.
- Keep rewards and penalties centralized through the coordinator's normal
  mission success/failure path.
- Report `can complete`, `must fail`, reason, required/eliminated groups,
  living crew, and active/destroyed/captured vehicle counts in Convoy Runtime
  Report.

Acceptance criteria:

- Killing one convoy crew updates progress 1/N.
- Killing all convoy crews completes objective.
- Capturing a vehicle with living crew is blocked.
- Capturing a vehicle after crew is eliminated succeeds.
- Destroyed convoy vehicle is not respawned.
- Intact convoy vehicles remain in-world after crew-elimination success.
- Live-crewed arrival fails the mission and cleans up convoy assets.
- Mission success applies generic reward once.
- Mission failure applies failure penalty once.

## Phase 11 - Mission-Specific Convoy Outcomes

Status: Complete; schema-52 exact operations defer to this once-only outcome
owner before writing terminal settlement.

Goal: make convoy mission types meaningfully different.

Implementation:

- Add outcome hooks: `OnConvoyArrived`, `OnConvoyCrewEliminated`,
  `OnConvoyVehicleCaptured`, `OnConvoyCargoDelivered`, and
  `OnConvoyMissionExpired`.
- `convoy_ammo`: ammo truck capture creates an ammo source or ammo reward.
- `convoy_armored`: captured armor can be garaged; composition scales with war
  level.
- `convoy_money`: full payout requires money vehicle/cargo delivered to HQ.
- `convoy_prisoners`: prisoners/captive asset must be freed/extracted for full
  reward.
- `convoy_reinforcements`: arrival strengthens the target garrison.
- `convoy_supplies`: arrival increases occupier support; captured/delivered
  supplies increase FIA support.

Acceptance criteria:

- Reinforcements convoy arrival increases target garrison.
- Supplies convoy arrival increases occupier town support.
- Captured supplies delivered to town increases FIA support.
- Ammo truck capture creates an ammo-source effect or visible garage/source
  flag.
- Money convoy does not grant full payout unless delivered.
- Prisoner convoy can grant HR/support when prisoners are extracted.

Completion notes:

- Added mission-specific convoy outcome service, save guards, runtime reports,
  mission asset follow-up actions, I-menu guidance, and map marker transitions.
- HST_Dev smoke testing confirmed supply delivery and prisoner extraction
  outcomes; cleanup now preserves neutralized convoy vehicles after delivery so
  players can keep or capture them.
- `tools/validate-foundation.ps1` passes with schema 18.

## Phase 12 - Active Mission Persistence

Status: Complete for persisted mission data; schema-52 exact convoy process-
restart/rematerialization proof remains open.

Goal: make active missions, especially convoys, survive save/load.

Implementation:

- Save active mission fields, objectives, mission assets, runtime entity
  records, active convoy groups, and runtime vehicle records.
- On restore, do not assume physical entities still exist.
- Respawn or abstract active runtime based on player proximity and mission
  phase.
- Add a persistence smoke test for active convoy.

Acceptance criteria:

- Save during convoy staging, restart, convoy remains staging.
- Save during convoy moving, restart, convoy remains active.
- Save during convoy contact, restart, objective progress remains.
- Destroyed convoy vehicles stay destroyed after restart.
- Captured convoy vehicles stay captured/garaged after restart.
- Mission does not duplicate vehicles after restore.
- Active non-convoy mission records preserve objectives, mission assets, and
  final status through save/load.
- Restore path does not duplicate mission runtime entities or active groups.

Current notes:

- A schema-52 exact convoy persists its mission/operation/manifest/held-batch
  links and three convoy elements. Restore clears process-local handles and
  arrival samples, retains generated-route progress, element position/vehicle
  state, cargo assignment, and confirmed crew casualties, and resumes one
  coherent open aggregate virtually. Only member slots may carry casualty
  tombstones, legal lifecycle pairs are enumerated, and missionless exact-looking
  rows remain durable quarantine evidence. Historical convoys remain version `0`.
- Before each real capture, persistence reconciles mapped physical exact-convoy
  members. An open publication transaction or ambiguous mapping defers capture
  before flush/savepoint, retains checkpoint intent, and retries on the bounded
  retry cadence. Packaged death-between-ticks and deferred-save replay remain open.

- Active mission restore now repairs runtime fields, objective-to-asset links,
  mission asset runtime records, and restored spawned flags without adding a
  new save schema.
- Restored convoy runtime normalizes staging/moving/contact phases, rebuilds
  missing live handles from saved convoy assets/groups, preserves destroyed or
  captured vehicle state, and removes the short restore grace-window dependency.
- Re-review hardening keeps preserved convoy crew counts alive through the
  Workbench zero-agent population window so restored or freshly respawned
  convoy groups do not fail before agents report live.
- Persistence smoke coverage now seeds staging, moving, and contact convoy
  records and reports duplicate mission assets, runtime entities, active
  groups, and runtime vehicles.
- Workbench follow-up: smoke fixtures are diagnostic-only and are skipped by
  map markers, mission runtime ticking, convoy physicalization, mission intel,
  and normal command UI surfaces so seeded smoke state does not appear as live
  campaign content.
- `tools/validate-foundation.ps1` passes with schema 18.
- HST_Dev Workbench smoke passed for active convoy restore across moving/contact:
  destroyed convoy slots remain terminal after restart, survivor groups do not
  respawn in a loop, restored convoy completion applies the FIA reward, and the
  completed convoy no longer falls through into failure.

## Phase 13 - Non-Convoy Mission Primitive Hardening

Status: Complete

Goal: bring other mission types up to the same reliability level as convoys.

Mission primitives:

- `kill_hvt`
- `hold_area`
- `clear_area`
- `destroy_target`
- `recover_cargo`
- `rescue_extract`
- `deliver_supplies`

Implementation:

- Runtime assets spawn reliably.
- Objectives link to correct assets/entities.
- Player interaction validates distance and state.
- Objective completion is deterministic.
- Failure reason is clear.
- Runtime cleanup is safe.
- Save/load restore works.

Acceptance criteria:

- Assassination can be completed by killing/sabotaging HVT.
- Destroy mission completes when target is destroyed.
- Logistics recovery completes on pickup; supply delivery completes on delivery.
- Rescue mission can free and deliver captives.
- Hold/clear mission progresses only when conditions are met.
- All mission types show useful runtime reports.
- All mission types survive save/load.

Current notes:

- Non-convoy primitive polling is now explicit for HVT, hold, clear, destroy,
  cargo recovery, rescue/extract, and supply delivery missions.
- Mission asset initialization is idempotent by role/count, so partial restored
  primitive state can self-repair without duplicating assets.
- Asset-driven objectives continue to progress when matching mission assets
  exist, even if a fallback prop is present.
- Recover-cargo objectives now remain pickup-complete. Restored legacy
  `hq_delivery` objectives for active recover-cargo missions are completed by
  the restore repair pass instead of blocking completion.
- Runtime reports now include asset interaction radius, cargo capacity,
  attached/carrier state, outcome state, and delivery distance.
- Persistence smoke coverage now seeds one active mission for each non-convoy
  primitive.
- Workbench follow-up: primitive smoke fixtures are excluded from ordinary
  mission reports, objective ticking, mission start duplicate gates, and map
  marker publication while remaining visible to the persistence smoke report.
- HST_Dev primitive testing passed well enough to close the phase: HVT kill,
  hold/conquest, clear-area, destroy target, resource cache, POW rescue/follow,
  city supplies, mission action visibility, save/load carrier restore, and
  persistence smoke are all covered by implementation and validation.
- Expired missions now preserve and continue player-bound cargo/POW extraction
  scenes, and expired convoy contact preserves the live firefight while players
  remain inside the render bubble.
- Phase 13's primitive remains complete, but the latest packaged check exposed a
  real duplicate-radio-tower presentation/runtime-authority defect. Current
  schema-50 source retains an existing transmitter instead of spawning a second
  composition tower and lets destroy-target missions borrow that entity without
  generic cleanup deleting it. Publish and test destroy/rebuild behavior before
  treating the correction as closed.
- `tools/validate-foundation.ps1` passes with schema 18.

## Phase 14 - Arsenal, Loot, And Finite/Infinite Unlock Loop

Status: In progress - loadout smoke pending. Arsenal, loot, finite/INF policy,
vehicle cargo, and field-vehicle save/load restore have passed HST_Dev smoke.
The loadout editor implementation is present in code, but the phase remains
open until the end-to-end Workbench path is exercised.

Goal: complete the Partisan loot-to-unlock progression loop.

Implementation:

- Area loot deposits eligible items into the arsenal.
- Vehicle loot stores eligible items in nearby vehicle cargo.
- Vehicle cargo can transfer to the arsenal.
- Arsenal counts finite items.
- Items unlock when threshold is reached.
- Unlocked items become infinite.
- Blocked items never unlock or never deposit, depending on config.
- Loadout editor validates finite/infinite cost before applying.
- Failed loadout application does not lose items.

Acceptance criteria:

- Depositing an item increases arsenal count.
- Item unlocks at configured threshold.
- Withdrawing a finite item decrements count.
- Withdrawing an unlocked item does not decrement count.
- Blocked item does not unlock.
- Loadout apply is atomic.
- Save/load preserves arsenal counts and unlock state.

Checkpoint notes:

- Arsenal item rules are typed balance config, with blocked and finite-only
  behavior visible in reports.
- Raw visual/support assets are rejected from arsenal and vehicle cargo.
- Area loot, vehicle cargo collection, vehicle unload, and reports have been
  exercised in HST_Dev.
- Nearby Workbench-spawned field vehicles are snapshotted before manual
  checkpoint and restored after save/load as `loot_vehicle` runtime records.
- Loadout editor code now includes live equipment/storage nodes, compatible
  candidates, fixed personal save slots, profile loadout files, finite/INF cost
  validation, atomic apply/rollback, issued-item accounting, death-loss
  handling, and removed external state purging.
- Loadout editor HST_Dev smoke remains before Phase 14 can be marked complete.

## Phase 15 - Garage And Vehicle Persistence

Status: In progress - broad-alpha garage/source-vehicle foundation exists.
Capture, cargo preservation, dry-ground redeploy, field-vehicle snapshot/
restore, runtime/source metadata, and typed reports/probes exist. Exact
anti-duplication transactions, repair/rearm/refuel rules, progression limits,
and restart/multiplayer soak remain open.

Goal: make captured vehicles a reliable campaign progression system.

Implementation:

- Capture nearby vehicle into garage.
- Validate vehicle root.
- Validate vehicle is not occupied.
- Capture physical and virtual cargo.
- Delete physical vehicle only after garage record is prepared.
- Redeploy vehicle near HQ.
- Restore cargo on redeploy.
- Track ammo/repair/fuel source vehicles.

Acceptance criteria:

- Captured vehicle appears in garage report.
- Captured vehicle despawns from world.
- Vehicle cargo is preserved.
- Redeployed vehicle spawns at safe HQ position.
- Garage record is removed or updated according to redeploy policy.
- Ammo truck is recognized as ammo source.
- Save/load preserves garage vehicles and cargo.

## Phase 16 - Recruitment, Training, And Garrisons

Status: In progress - recruitment, training, abstract garrisons, exact visible
garrison quotes, and typed smoke coverage exist. The exact enemy rebuild fixture
now proves delivery plus physical-live projection/fold/restart, but generic
player-garrison manifests, arsenal-driven AI loadouts, static defenses, broader
typed command cutovers, and runtime/restart coverage remain open.

Goal: connect HR, money, training, arsenal unlocks, and abstract garrisons.

Implementation:

- Recruit FIA units using HR and money.
- Train troops using faction money.
- Training level affects AI quality.
- Arsenal unlocks affect recruit loadouts.
- Commander can add/remove garrison units.
- Garrison records remain abstract until activated.
- Active group survivors fold back into garrison state.

Acceptance criteria:

- Recruiting spends HR and money.
- Training spends faction money and increases training level.
- Recruit report changes when weapons unlock.
- Garrison can be added to a friendly zone.
- Entering a zone activates garrison physically.
- Killing part of a garrison and leaving folds survivors back.
- Save/load preserves garrisons.

## Phase 17 - Zone Capture And Ownership

Status: In progress - capture progress and Schema-62 canonical all-cause
ownership exist in source. Sealed Schema 63 replaces raw group/vehicle
pressure with one crew-aware shared authority and revisioned
`HOT -> COOLING -> COLD` zone state. One durable revisioned receipt now owns exact/legacy
security settlement, support, owner, town/facility/logistics derivation,
retaliation, economy/outcome, events, parent-aware projection, notification, and
persistence scheduling. Later capture/mission/admin/migration requests remain
durable as array-ordered pre-owner receipts behind an active publisher.
Sealed Schema 64 routes strict town-support hysteresis into the same receipt
and removes legacy/direct political owner mutation. Packaged native security/
restart/protocol-2 convergence, Schema-64 Campaign Debug/runtime execution,
broader encounter consequences, and soak remain open; the preceding Schema-63
Foundation and Workbench gates pass.

The current stamped Phase-17 source follows one newly admitted exact
counterattack across real runner frames. Player proximity drives the production
commander from held `VIRTUAL` authority into `MATERIALIZING`; a scoped spawn-
adapter worker establishes one native root and the exact living frozen roster
under `PHYSICAL`/live authority. Physical confirmation is state-first: each
initial or re-entry seam re-reads the production authority, batch, root, roster,
handles, adapter bindings, and support-row exclusion for at most four real runner
frames. The production tick's mutation result remains diagnostic because a
successful projection can settle between proof frames without another mutation.
Async settle stages still yield, but successful handoff, `PHYSICAL`
confirmation, and the immediately controlled fold or kill now stay in the same
runner invocation, matching production ordering. The proof never requeues,
retires, or respawns a succeeded projection. Binding diagnostics distinguish an
incomplete handoff from a complete binding whose entity is nonliving. Leaving
the materialize-out radius folds the same survivors back to held `VIRTUAL`
authority. It then rematerializes the full
roster, kills one real engine member, lets only the scoped adapter retire the
durable casualty slot, and folds `N-1`. Re-entry materializes only the survivors,
reconciliation replay is a no-op, and a final fold preserves the same `N-1`
identity without resurrection. Strict roster/root/member/handle topology, real-
clock isolation, stable identity/resource continuity, zero legacy support
ownership, and fatal isolated-run restore on missing order remain asserted.
The preceding proof-ordering implementation passes Foundation at 808 script-symbol references.
Its final stamped Workbench Game-module gate loads 5,830 Game files and 11,822
classes with 47,077K static storage at CRC `b789ee05`, reports `Script validation
successful`, exits `0`, records zero script/HST/hard-failure signals, and leaves
zero session root, owned processes, new default-log entries, or external spill.
R28 first
failed the five casualty-continuity assertions; unchanged R28b passed all eleven
targeted Phase-17 assertions; R29 reproduced a `MATERIALIZING`/succeeded-batch
proof seam before `Kill`; and R30 passed the first six but failed all five
casualty assertions when one bound and registered member became nonliving before
the controlled kill. R31 validates the same-invocation ordering correction: all
eleven Phase-17 assertions pass exactly once, each physical settle and the
casualty settle complete in one of four samples, and final tracked-state drift is
zero.
The preceding owner-snapshot implementation passes Foundation at 808, stamped PC
Game validation at CRC `e836e3b4`, and guarded runtime proof
`seed1985_t0_p1_u1784134163`: Phase 17 is 11/11, Phase 24 classifies 14/14
owners with zero invariant failures and three valid open exact `VIRTUAL`
projections, and every state-drift, error, and cleanup counter is zero.

Goal: make the map conquest loop work.

Implementation:

- Detect players/FIA inside capture radius.
- Query canonical hostile combat presence inside capture radius. Count fresh
  conscious registered infantry, each operational occupied armed mobile/static
  platform once, or eligible durable virtual infantry; exclude cargo, empty or
  disabled assets, stale samples, and terminal/quarantined groups.
- Progress capture only when FIA are present and hostiles are cleared or below
  threshold.
- Pause or decay progress while live presence is `HOT` or the persisted
  30-second-by-default `COOLING` deadline is active.
- Submit the expected owner/revision to the canonical transition when progress
  reaches threshold.
- Canonicalize supported zone aliases before request fingerprint/replay.
- Settle old exact patrols and aggregate security before publishing the owner.
- Apply the conservative liberated starter policy: retire enemy town police/
  roadblocks and create at most two FIA aggregate infantry with no automatic
  vehicle, while retaining exact new-owner authority.
- Update markers through protocol `2`, correlating source ownership revision
  independently from marker-local revision/stream sequence.
- Stage the full logical marker snapshot and commit only after every nested child
  receipt-zone-marker chain validates/releases; roll back all rows/epoch/sequence
  on failure.
- Freeze aggression/counterattack choice once per transition receipt.
- Advance the exact counterattack projection only through its production
  commander/runtime owner and scoped force-spawn adapter, preserving the frozen
  roster and one-pool debit from `VIRTUAL` through `MATERIALIZING` and `PHYSICAL`,
  then back to `VIRTUAL`.
- Confirm each proof-side `PHYSICAL` transition from production authority and
  exact topology within four real runner frames; record tick mutation only as
  diagnostics and never requeue or respawn an already succeeded projection.
- Route exact-counterattack physical deaths through the scoped spawn adapter as
  the sole casualty authority; fold and reproject only the `N-1` living slots,
  reject resurrection, and require repeated reconciliation to be a no-op.
- Record ownership strategic events from exact owner/capture/aggression receipt
  facts rather than unrelated global deltas across retry.
- Let capture-triggered political child flips finish domain state but publish
  marker/menu/GM/notification changes with the parent.

Acceptance criteria:

- Empty enemy outpost can be captured by holding.
- Hostile presence pauses capture.
- Capture progress appears in report/UI.
- Zone owner changes to FIA on completion.
- Owner revision advances exactly once and receipt/backlinks are reciprocal.
- Marker changes owner/color and reports the same source revision.
- Enemy counterattack order can be queued.
- Historical R27 proves
  `phase17.counterattack.native_projection.baseline`, `.materializing`,
  `.physical`, `.fold`, `.continuity`, and `.clock_isolation` without leaving
  transient runtime residue.
- R28 first failed and unchanged R28b passed
  `phase17.counterattack.native_projection.native_casualty`, `.casualty_fold`,
  `.casualty_reentry`, `.casualty_replay`, and `.casualty_continuity` exactly
  once each without resurrection, replay mutation, or final-state drift.
- R29 reproduces the pre-`Kill` physical-confirmation race while both Phase-24
  owner assertions pass. R30 passes the first six Phase-17 assertions but fails
  the five casualty assertions before `Kill` when one bound and registered member
  becomes nonliving inside an artificial one-second debug scheduling window;
  Phase-24 runtime-owner classification passes and exact authority skips after
  cleanup removes the focal order. R31 proves all eleven Phase-17 assertions
  exactly once with initial/casualty/survivor physical settles `1/1/1` of four,
  casualty settle `1/4`, all 18 tracked deltas zero, and both Phase-24 owner
  assertions passing.
- Save/load preserves captured owner/receipt and resumes an interrupted
  transition without replaying completed effects.
- Identical replay is a no-op; fingerprint conflicts and stale expected
  owner/revision reject.
- Unsupported exact security blocks before owner mutation.

## Phase 18 - Enemy Commander Physical Responses

Status: In progress - resource-limited order selection, proactive/reactive pool
separation, physical/abstract response, target scoring, and typed smoke coverage
exist. Schema 51 makes newly planned infantry-only defensive QRF the first exact
enemy-order cutover: one frozen prepaid roster travels outbound, materializes or
folds with exact survivors, applies defensive pressure once, returns to origin,
and settles proportional survivor resources once. Existing rows and every other
enemy order remain on their legacy paths. The sealed Schema-68 checkpoint
filters and ranks targets against canonical commitment roots, keeps exact patrol
compatible with a defensive response, and reranks another target in the same due
decision when patrol would duplicate patrol. Mixed roots use blocking precedence,
queued/equivalent-ID rows participate, and settled/terminal/rival rows do not.
Target pressure stays out of preparation; commitments are rechecked before every
debit, including pressure-marked retry. Campaign Debug, packaged sustained
movement/contact, arrival/return, restart, live-server proof, and long-window
behavior remain open.

Schema 69 retains the exact newly admitted counterattack owner. Schema 70 adds
the next narrow cutover for newly admitted garrison rebuilds: one capacity-
bounded frozen infantry manifest, one prepaid support debit, reciprocal order/
operation/manifest/spawn/group authority, casualty-preserving strategic/
physical transfer, and delivered held-garrison authority without aggregate
double count. Historical rebuilds remain contract `0`; malformed/orphan current
graphs quarantine at `-70`. Its stamped Workbench compile/create and focused
engine proof passed all 13 headlines, and Foundation passed at 790 references.
The dated Full Campaign Debug evidence for that slice passes Schema-70 clock/
future isolation and Phase 18 counterattack/rebuild/roadblock admission. Its
remaining Phase 18/22
cascade came from an ambient coordinator commander tick that admitted an
untracked Petros defense between cases. Source now holds that cadence only while
the debug state-isolation clone is active; explicit production ticks remain the
fixture driver. Typed settlement precedes prefix cleanup, and exact operation-
marker backing delegates to the publisher's reciprocal canonical predicates.
The exact `delivery_pending` and `physical_live_fold` fresh-process cuts now
pass. The latter proves scoped native handoff, one root/nine adapter handles/
eight living runtime members, 2.759m movement/0.539m closure, production fold,
exact eight-survivor/one-casualty restart, read-only replay, and zero cleanup.
Natural full-route travel/combat, other force families, multiplayer/JIP, and
soak proof remain open.
Phase 17 remains the zone-capture and Schema-69 exact-counterattack slice.

Goal: turn enemy orders into visible war activity.

Enemy order types:

- Patrol
- QRF
- Counterattack
- Rebuild garrison
- Roadblock
- Support call
- Petros attack

Implementation:

- Keep enemy resource spending server-authoritative.
- Queue enemy orders from aggression, war level, capture events, and HQ
  knowledge.
- Physicalize QRF/counterattack only near players or active objectives.
- Resolve off-screen orders abstractly.
- Update garrisons, roadblocks, support requests, or mission state on
  resolution.
- Collapse linked order/support/operation commitments to one target root, reject
  incompatible roots before ranking, give blocking rows precedence in a mixed
  root, and apply the capped deterministic penalty to compatible roots. Treat
  queued and equivalent legacy/canonical target IDs as current; ignore settled,
  terminal, and rival authority.
- If an exact-patrol target resolves to another patrol order, exclude it and
  deterministically rerank within the same due decision instead of consuming the
  180-second cadence with a skip.
- Freeze periodic decisions without pressure, then revalidate commitments,
  candidate identity, and order compatibility before pressure/debit. Recheck the
  commitment fingerprint on pressure-marked retry before debit. Record an all-
  committed map as an explicit zero-cost skip.
- For a newly planned infantry-only defensive QRF, select a distinct
  same-faction source, freeze and debit one exact roster, suppress a parallel
  legacy QRF/support response, and keep strategic travel active off-screen.
- Require two live-position samples from distinct campaign seconds for physical
  target/origin arrival. Apply target pressure once, return exact survivors, and
  settle one deterministic resource refund. A fold alone never settles.

Acceptance criteria:

- Enemy order report shows queued/active/resolved orders.
- Capturing a zone can trigger counterattack.
- Counterattack spends enemy resources.
- If players stay nearby, counterattack can spawn physically.
- If players leave, order resolves abstractly.
- Rebuild order increases enemy garrison.
- Roadblock order increases roadblock presence.
- Exact enemy defensive QRF creates one order/operation/manifest/batch/group
  aggregate and no legacy support or QRF row.
- Exact enemy defensive QRF preserves casualties through materialize/fold and
  restore, then refunds only the surviving fraction after return.
- A linked commitment graph rejects its target once, an exact patrol can admit a
  non-patrol defense, and a duplicate patrol reranks to the next target without
  waiting another cycle.
- Mixed compatible/blocking rows sharing one root reject once with blocking
  precedence. Queued alias rows block their equivalent canonical target;
  settled/terminal and rival rows are ignored. Multiple rejected targets retain
  the same first diagnostic under input permutation.
- A commitment inserted after freeze rejects admission before pressure/debit;
  the same check still runs after pressure is marked and before debit. All targets
  committed produces no order, pressure, resource, or rival mutation.

## Phase 19 - Support Requests

Status: In progress - queued state, ETA, cooldown, reports, physical/abstract
resolution, cancellation/recall scaffolding, a typed recall result/explicit
visible-command receipt path, and typed smoke coverage exist.
Player-paid infantry QRF is the first exact support consumer; other support
types remain legacy. Schema 50 gives only that exact infantry QRF persistent
direct-route movement while virtual, proximity-based materialization/fold with
hysteresis, an exact frozen-slot roster, and bounded on-station virtual infantry
combat. When physical, support still treats ETA as an earliest check and
requires two live samples from distinct elapsed seconds within 75m for arrival
or recall exit. Reliable packaged virtual travel, projection transfer, combat,
physical travel/arrival/return, settlement, and restart proof remains open.

Goal: make FIA and enemy support requests stateful and testable.

Implementation:

- Player/FIA support requests: supply support and ground support; future
  mortar/air only if assets exist.
- Enemy support requests: QRF, patrol/search, ground support, and abstract
  helicopter-style support if no safe asset path exists.
- Each request validates cost/cooldown, records source/target, has ETA, and
  physicalizes or resolves abstractly.
- The exact paid player infantry QRF keeps one operation and one frozen-slot
  roster across virtual travel, materialization, physical fold, virtual combat,
  recall, settlement, and restore. This rule does not apply to other supports.
- Spawned requests route directly from the current living-member centroid to the
  current target or recall exit. Service-owned waypoint replacement is
  transactional and stalled chains have a maximum of three consecutive reissues
  until an observed 8m new-best distance improvement resets the stall budget.
- Recall exposes accepted/already-applied/state-changed/terminal disposition and
  operation identity. Presentation wording never decides receipt authority;
  exact full refunds prevalidate both linked ledger legs before either changes.

Acceptance criteria:

- FIA support request appears in support report.
- Enemy support request spends enemy resources.
- Request has ETA and status.
- ETA alone cannot complete a spawned request; live distance must confirm
  arrival, and recall settlement must confirm the spawned group reached its
  exit.
- Request resolves physically when players are nearby.
- Legacy requests may resolve abstractly when players are far; an exact paid
  infantry QRF instead remains virtual and continues its operation.
- Cooldown prevents spam.
- Save/load preserves active support requests.
- Save/load restores an open exact infantry QRF as one held virtual projection
  with the same confirmed survivor/casualty slots and no process-local entities.

## Campaign Debug Phase 20 - Civilians, Town Support, And Undercover Reports

Status: In progress - sealed Schema 64 makes one revisioned town-influence
record the sole support/population/contact/flip authority for curated towns and
routes strict political intent through canonical ownership. Aid, civilian
projection, undercover eligibility, and typed reports/smoke coverage exist. A
packaged schema-49 run proved civilian traffic can move but exposed repeated
town appearances, stuck horns, missing Figari/Morton
ambience from HQ-coupled military activation, and full-town ambience at a minor
woodland locality. Schema-50 source separates civilian eligibility, selects
concrete variants, limits that locality to two pedestrians, and clears scoped
ambient horn input. Schema 60 adds native wheeled-base horn suppression and
removes the overlapping Maiden's Bay town while keeping the Logistics Warehouse.
Schema 64 also keeps Simon's Wood ambient-only, preserves separate occupier/
invader support, adds contacted Zone Pressure and complete Resistance Territory,
processes influence history only when an expiry is due, and throttles verbose
active-group change logs to 30 seconds. The sealed Settings-24 source/Workbench
checkpoint for the Blueprint Phase 8 slice adds global budgeted fair allocation, four-root
transaction-start scheduling,
asynchronous readiness, immutable per-zone/kind projection slots, bounded slot-
specific recovery/recycle, owner-aware static refresh, per-frame player-first
claim observation, and fail-closed transient-versus-claimed vehicle persistence.
Promoted roots retain live transform/destruction/cargo bindings. New-campaign
reset can retain occupied live tracked `loot_vehicle`, `field_vehicle`, and
`garage_redeploy` roots, normalizes them to `field_vehicle`, and copies retained
vehicle/cargo state before replacing campaign state. Campaign Debug Phase 20 now
uses the complete production global plan and four-root transaction-start cap. Foundation
passes at 711 references; normal/all-five Workbench checks pass at 5,799 files/
11,718 classes and CRC `bb083672`, with zero HST errors and zero surviving
processes. Campaign Debug, packaged native behavior, actual save/restart, the
ten-town/ten-minute soak, rendered political UI, and stutter proof remain open.
Native brief enter/exit, autosave/restart, promoted-root destruction, reset, and
Campaign Debug Phase 20 execution remain proof gates. Aid and ownership/security-
pressure paths exist in source but need runtime proof. Sealed Schema 65 source/Workbench now
implements automatic casualty, resistance theft after durable promotion, exact
nearby-combat episodes, town aggression receipts, and pedestrian panic/recovery.
That work has no native evidence yet. Sealed Schema 66 source/Workbench now
implements the first exact enemy-town local-security roster authority; Foundation
and Workbench validation pass, but runtime evidence remains open. Final stamped
Schema-65 normal/all-five Workbench validation is clean
at 5,802 Game files/11,728 classes with CRC `c0a672b9`, `Script validation
successful`, zero HST script errors, and zero surviving processes; the post-
Foundation passes at 717 script-symbol references; runtime gates remain open.

Goal: make the civilian layer visible and start enforcing undercover rules
gradually.

Implementation:

- Town support report: canonical FIA/occupier/invader basis points, signed
  support, record revision/contact/hysteresis/population, reputation, wanted
  heat, police presence, and roadblock presence.
- Population-scaled political changes use the pinned formula goldens
  100/200/50 basis points for raw `+1` at populations 100/25/400. Strictly more
  than 8000 requests resistance ownership; strictly less than 4000 requests
  enemy ownership; equality stays neutral. All flips use
  `HST_OwnershipTransitionService`.
- Classify curated stock town centers separately from minor localities; military
  HQ-safety suppression must not erase civilian life near a player.
- Give true towns a configurable daytime/low-heat target of driven ambient
  vehicles (default five) when canonical population and global actor/traffic
  budgets permit. Five is not a fixed ceiling or unconditional guarantee. Minor
  localities receive their smaller configured
  pedestrian budget without town-scale traffic; appearances remain
  deterministic and non-repeating within a locality.
- Allocate scarce physical actors through deterministic 120-second-or-longer
  leases and a rotating reconciliation cursor. Begin at most four root-spawn
  transactions per
  health update, including first-pass static civilian and military roots. The
  scoped Campaign Debug Phase 20 helper must select its town from this same
  complete global plan and transaction-start cap.
- Treat native behavior assignment as asynchronous. Admit a pedestrian only
  after the exact living CIV member, one-member group, and active wander
  waypoint agree. Admit traffic only after the exact pilot, usable vehicle,
  running engine, and active route agree.
- Keep immutable per-zone/kind projection slots and original seeds across
  readiness/health checks. Derive recovery routes from the slot plus attempt,
  use bounded backoff/recycling, and refresh static military ambience when the
  location owner or policy key changes while preserving player claims.
- Keep routine session topology non-durable. Filter unclaimed ambient rows and
  cargo from save/restore, promote claimed vehicles to `field_vehicle`, and
  migrate legacy detached ambient claims through the same durable boundary.
- Observe occupancy player-first before persistence every server frame, avoiding
  a full ambient-root occupancy scan, and repeat reconciliation in every
  `HST_PersistenceService` capture/checkpoint path plus new-campaign reset.
  Reject destroyed roots and dead controlled occupants. Resolve the exact
  registered binding first; use a unique same-prefab root within 8 meters only
  for initial/recovery binding, and fail closed when that fallback is ambiguous.
  Retain current transform/destruction/cargo snapshots. Reset may retain occupied
  live tracked `loot_vehicle`, `field_vehicle`, and `garage_redeploy` roots,
  normalizes them to `field_vehicle`, and copies retained vehicle/cargo state
  before replacing campaign state.
- Reset horn input only for HST-owned ambient drivers, disable AI horn timing/
  output in the inherited native wheeled-vehicle base, and clean ambient
  vehicles, groups, and waypoints when the locality leaves the render bubble.
- Reserve tracked civilian-casualty IDs from the persisted monotonic allocator,
  queue callback observations under a fixed bound, and drain them before the
  persistence tick. Use the actor receipt to deduplicate the native callback and
  health-tick dead-character fallback.
- Apply civilian vehicle theft only after player-first promotion succeeds. Build
  the resistance theft event from the durable vehicle runtime ID; do not infer a
  theft from movement, a transient root, a military vehicle, or a non-resistance
  claim.
- Start civilian combat danger only from current-operation or recent-fire facts.
  `HOT` by itself is not a consequence. Persist the combat revision, episode,
  last-applied episode receipt, danger transition time, panic deadline, and last
  consequence event.
- Persist an optional enemy aggression target, positive bounded delta, and exact
  before/after values on each canonical town event. Require one unique target
  pool and one applied strategic receipt with the same source event, zone,
  target, timestamp, and delta; replay must not add aggression again.
- Drive admitted pedestrians through `Wandering -> Panicked -> Recovering ->
  Wandering`. Native danger replaces wander helpers with one move waypoint away
  from the threat and RUN speed; calm recovery restores deterministic wander
  helpers and WALK speed before acknowledgement. Minor localities are panic-only
  and do not gain political population/support; their bounded exact fingerprint
  map remains session-only.
- Admit one deterministic exact local-security patrol epoch only for a canonical
  enemy town with positive police pressure. Freeze an authored 2–5 member roster,
  keep casualties in durable SpawnQueue slots through fold/restore, apply police
  `-1` once on destruction, forbid same-epoch resurrection, and rearm only after
  newer ownership or a later positive police event. Resistance automatic police
  and roadblock targets are zero; generic police projection may not refill or
  fold this roster.
- Undercover eligibility report: clothing reason, weapon reason, vehicle
  reason, off-road reason, enemy proximity reason, and wanted heat reason.
- Add player commands to request undercover, clear compromised undercover, and
  show undercover status.

Acceptance criteria:

- Town report shows support/reputation/wanted state.
- Undercover report gives explicit eligibility reason.
- Civilian clothing can be eligible.
- Military equipment can be ineligible.
- Wanted heat can block or compromise undercover.
- Save/load preserves civilian and undercover state.
- Figari and Morton project town ambience near a player even when their hostile
  garrison is held inactive by HQ safety; the known woodland locality projects
  two pedestrians and no town traffic.
- Ten simultaneously eligible towns run for ten minutes with fair leased
  allocation, bounded admission/recovery/recycle, stable totals, no repeated
  full-town churn, and no once-per-second stutter. Daytime/low-heat traffic
  approaches the configured target (default five) only where population and
  global budgets allow.
- A native brief enter/exit promotes exactly one durable field vehicle;
  destroyed/dead-occupant cases do not claim, unclaimed rows/cargo stay absent
  across autosave/restart, the claimed vehicle restores at its current transform
  without duplicate ambience, and reset retains only occupied live tracked
  durable roots at the boundary, normalized to `field_vehicle`. Two nearby
  durable rows sharing one prefab restore to two distinct physical roots without
  collapsing onto one root.
- Static military ambience refreshes to the current owner/policy through the
  shared transaction cap without deleting a player claim.
- Maiden's Bay publishes only the Logistics Warehouse, and both ambient and
  other AI-driven wheeled vehicles remain silent without a one-second stutter.
- Simon's Wood remains a two-pedestrian ambient locality with no political town
  record. Contacted Zone Pressure lists the current town first, and Resistance
  Territory lists every published resistance-owned strategic zone in stable
  type/name/ID order.
- Commander aid and ownership/security-pressure paths receive packaged runtime
  proof. Automatic civilian casualty, theft, nearby-combat influence, aggression
  receipts, and panic/recovery receive deterministic and packaged proof for
  replay, conflict, restore, and balance. Pre-65 restore must clear uninvented
  aggression evidence; malformed current town/strategic evidence quarantines at
  `-64` before malformed civilian episode authority quarantines at `-65`.
  Minor-locality cross-process replay identity remains open. Schema-66 local
  security must additionally prove exact native members/waypoints, casualty fold/
  re-entry/restart without refill, once-only loss, no-resurrection/rearm, non-loss
  terminal causes, ownership ordering, migration/quarantine, and zero resistance
  automatic police/roadblocks.

## Phase 21 - Undercover Enforcement And Police/Roadblocks

Status: In progress - undercover apply/clear, wanted and vehicle heat,
weapon/vehicle compromise, police/roadblock scans, and typed smoke coverage
exist. Live equipment/clothing/off-road enforcement, behavior-ready security
actors, multiplayer feedback, and soak remain open.

Goal: move from reports to actual undercover gameplay.

Implementation:

- Apply undercover state to player.
- Detect suspicious actions: weapon drawn/fired, military gear, military
  vehicle, off-road driving near enemies, restricted zone entry, and
  roadblock/checkpoint scan.
- Compromise player when detection succeeds.
- Increase wanted heat after hostile/civilian incidents.
- Roadblocks and police increase detection chance.
- Report why undercover was lost.

Acceptance criteria:

- Player can enter undercover when eligible.
- Player loses undercover when firing/drawing weapon near enemies.
- Military vehicle blocks undercover.
- Roadblock/police presence can compromise player.
- Compromise reason is visible.
- Wanted heat decays or clears according to rules.

## Phase 22 - HQ Threat And Defend Petros

Status: In progress - HQ knowledge/threat, Petros attack orders, linked support/
group/mission/objective/task state, campaign consequences, and typed smoke
coverage exist. Spawned Petros-attack support now uses the same live-centroid,
direct-target, two-sample arrival boundary. A historical 9/9 attacker group did
not show advance, but campaign-time-only sampling is not physical-stall proof.
Natural movement/contact/arrival, multi-wave defense, recovery policy, restart,
and multiplayer proof remain open.

Goal: implement the signature enemy punishment loop around HQ knowledge and
Petros.

Implementation:

- Track HQ knowledge.
- Increase HQ knowledge from traitor failure, enemy patrol near HQ, player
  activity near HQ, and high-aggression events.
- Enemy commander can queue Petros attack.
- Spawn enemy attack groups at HQ standoff positions.
- Mission objective: defend Petros for duration or eliminate attackers.
- Failure: Petros killed, HR/money penalty, possible forced HQ move or
  recovery.

Acceptance criteria:

- HQ knowledge is visible in report.
- Enemy commander can queue Petros attack.
- Defend Petros mission starts.
- Attackers spawn outside HQ safe radius.
- Petros death fails mission.
- Successful defense clears/lowers threat or completes mission.
- Save/load preserves active defend-Petros mission.

## Phase 23 - UI And Map Marker Polish

Status: In progress - broad-alpha command-menu and marker paths exist, and the
packaged schema-49 run proved that map publication returned with the stock HUD.
It also exposed invalid radio icons as giant boxes, owner-only labels, pointer-
under-dialog ordering, and duplicate radio transmitters. Schema 59 retains the
dynamic icon validation, location-plus-owner label, and pointer-layer repairs,
then makes its radio-site lifecycle service the sole transmitter projection
owner. Markers append ONLINE/DESTROYED/REBUILDING/UNRESOLVED/QUARANTINED state;
authored transmitters are borrowed without deletion, and a generated replacement
appears only after completed rebuild settlement.
Schema 51 additionally publishes the open exact enemy defensive-QRF operation at
its strategic/live cursor with living count, duty, endpoints, and ETA. Schema
52 publishes one aggregate exact-convoy marker at its strategic/live operation
position and suppresses three per-vehicle markers. Schema 61 implements the
marker-only authoritative client boundary: revisioned/tombstoned logical records,
epoch/global sequence, bounded hashed snapshot/delta packets, ACK/gap/resync,
reconnect/JIP snapshots, an atomic widget-independent registry, and client-local
native reconciliation. Server-native campaign marker publication is retired;
dynamic player markers remain separate. Sealed Schema 66 source/Workbench repairs the local-owner
regression introduced at `27672e6`: protected campaign markers enter native
state with owner `-1` and removal disabled, while the readiness keepalive
rebuilds a missing or mutated marker from the committed registry. Player-created
markers remain editable. Focused packaged host/two-client,
rendered-widget, reconnect/late-join, and restart proof remains open under CRI-6.

Goal: make the campaign understandable without reading logs.

Implementation:

- Improve menu tabs: Overview, HQ/Petros, Missions, Map/War, Forces,
  Arsenal/Loot, Members, and Admin.
- Improve marker behavior for zone owners, active missions, objectives,
  convoy last-known position, support/QRF, capture progress, and HQ marker
  visibility rules.
- Preserve the canonical placed-marker table, append/repair radio normal/glow
  resources by identity, and reject invalid icon entries rather than assuming a
  stock array index.
- Keep map-target prompt/indicator/dialog in the map-local layer below the native
  pointer, and route both radio mission IDs through the Schema-59 lifecycle
  owner rather than generic composition/runtime projection.
- Publish one marker for every open schema-51 exact enemy defensive-QRF
  operation at its strategic or live position, including faction, living count,
  duty, immutable route endpoints, and ETA. Remove it when the operation closes.
- Publish one aggregate marker for every open schema-52 exact convoy operation
  and suppress its per-vehicle markers. Remove it after terminal settlement.
- Preserve a marker record's revision and stream sequence across an unchanged
  rebuild; emit one ordered revision for create/update and one tombstone revision
  for deletion.
- Keep the client registry current while the map is closed. Reconcile native
  campaign markers from it when the map opens, without a second server-native
  campaign marker set. Bind authored zone markers through their exact cached
  names rather than a periodic radius scan.
- Insert protected client-local campaign markers as system-owned/non-removable
  records rather than through the stock local-player owner assignment. Compare
  live position, text, icon, flags, owner, and removal policy on readiness
  keepalive and self-heal integrity drift from the authoritative registry. Do not
  apply this restriction to player-created markers.

Acceptance criteria:

- Player can understand active mission state from the menu.
- Convoy marker updates while moving.
- Zone markers update after capture.
- Zone marker text contains both the location name and current owner.
- Radio markers remain normal-sized, show location plus owner plus lifecycle
  status, and an authored radio site has one canonical tower.
- The native pointer remains visible over map-target confirmation dialogs.
- Support/QRF markers appear when relevant.
- Exact enemy defensive-QRF markers follow virtual/physical travel and disappear
  on terminal settlement without a duplicate legacy marker.
- Exact mission-convoy markers follow virtual/physical travel as one operation
  and disappear on terminal settlement without three oversized vehicle rows.
- Host, two clients, reconnect, and late join converge on the same Schema-61
  epoch, watermark, and live-registry hash after snapshot, ordered deltas, and a
  forced gap/resync; map close/reopen does not lose logical state.
- Players cannot persistently delete, move, or edit system campaign markers; the
  native action is rejected or the next bounded keepalive restores exact registry
  state. Player-created markers remain editable across the same test.
- Campaign Debug's owner-client integrity probe must mutate and delete a tracked
  campaign marker through production seams, then prove one canonical repaired
  instance, unchanged registry/static counts, system ownership/non-removability,
  and independent player-marker edit/removal behavior.
- Failed actions show clear result text.
- Admin/debug actions are separated from normal player actions.
- UI preserves command/report paths for every feature introduced by the phase
  plan.
- Service failure reasons are shown verbatim or safely summarized instead of
  being hidden behind generic UI text.

## Phase 24 - Balance, Campaign Pacing, And Victory/Loss

Status: In progress - population outcome default. Strategic pacing and enemy
pressure smoke exists; population-support victory/loss is implemented and still
needs real restart/multiplayer soak. The current source classifies every
controlled escalation order by its production runtime owner and records exact-
counterattack open/terminal/invalid authority plus `V/M/P/D` projection totals.
Legacy support physicalization remains separate. The preceding integrated guarded run
`seed1985_t0_p1_u1784134163` executes
`phase24.escalation.runtime_owner_classification` and
`phase24.escalation.exact_counterattack_authority` exactly once each. Both pass:
classification is 14/14 with zero snapshot invariant failures, while three open
exact counterattacks own three `VIRTUAL` projections with zero invalid authority
or support leaks. R27, R30, and R31 remain historical development evidence. The
wider suite remains uncertified because unrelated failures and external gates
remain.

Goal: turn working systems into a coherent campaign loop.

Implementation:

- Tune starting HR/money/training.
- Tune income intervals and values.
- Tune arsenal unlock thresholds.
- Tune mission rewards and penalties.
- Tune aggression gain/decay.
- Tune enemy resource income/spending.
- Require every controlled escalation order to resolve to exactly one supported
  production runtime owner. For exact counterattacks, require one reciprocal
  projection for each open operation, no runtime claimant for a retained
  terminal ledger, exact `V/M/P/D` topology, and zero legacy support-request
  leakage.
- Tune war-level thresholds.
- Default victory: remaining population support reaches the configured
  threshold and all airfields are controlled.
- Default loss: killed population exceeds one third of initial population.
- Keep legacy control-percent victory optional for debug or alternate presets.
- Add campaign end report with population, support, airfield, outcome-mode,
  next-check, and persisted terminal metadata.

Acceptance criteria:

- War level increases as FIA gains territory.
- Enemy equipment/resource pressure scales with war level.
- Early game remains loot-focused.
- Mid game enables outpost/resource capture.
- Late game enables airfield/seaport pressure.
- Victory triggers only when population support and airfield control are both
  satisfied in default mode.
- Loss triggers when civilian killed population exceeds the catastrophe
  threshold in default mode.
- Campaign end state persists with outcome mode, population counts, support
  population, and airfield metadata.
- Full Campaign Debug passes
  `phase24.escalation.runtime_owner_classification` and
  `phase24.escalation.exact_counterattack_authority`; the legacy support/group
  physicalization observations remain separately classified.

## Phase 25 - Full-Campaign Soak Testing

Status: Planned

Goal: find long-session bugs, restart bugs, and multiplayer edge cases.

HST_Dev smoke plan:

- Start campaign.
- Select HQ.
- Start each mission family.
- Complete/fail each primitive.
- Save/restart after each.

Convoy soak plan:

- Start convoy.
- Save during staging, moving, and contact.
- Destroy one vehicle.
- Capture one vehicle.
- Let convoy arrive.
- Kill all crews.
- Verify no duplicate vehicles after restart.

Campaign-loop plan:

- Loot items.
- Unlock item.
- Recruit FIA.
- Capture zone.
- Trigger counterattack.
- Garage vehicle.
- Redeploy vehicle.
- Request support.
- Go undercover.
- Trigger wanted heat.

Multiplayer plan:

- Join as host.
- Join as second client.
- Verify guest/member/commander permissions.
- Disconnect/reconnect.
- Save with multiple players.
- Restart and rejoin.

Acceptance criteria:

- No duplicate player spawns.
- No duplicate convoy vehicles after restart.
- No active mission becomes impossible without a clear failure path.
- Save/load does not lose arsenal, garage, garrisons, zones, HQ, missions, or
  support requests.
- Server can run a two-hour session without campaign tick errors.
- Full Everon can run with multiple active systems without runaway entity
  spawning.
- Static validation passes before and after soak fixes.
- Soak report records tested build or commit, scenario, duration, active systems,
  failures, and follow-up issues.

## Authoritative Feature Checklist

The duplicate master checklist formerly stored here is retired. Use
[`FEATURE_CHECKLIST.md`](FEATURE_CHECKLIST.md) as the single feature-status and delivery-gate
tracker. Keep this phase plan focused on sequencing, architectural constraints,
and phase-specific acceptance criteria.
