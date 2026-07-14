# Partisan Feature Checklist

The active development tree uses Campaign Schema 70 while runtime settings
remain on Schema 24. Newly admitted enemy garrison rebuilds use exact contract
`1`: one capacity-bounded frozen infantry manifest, one prepaid support-pool
debit, and one reciprocal order/operation/manifest/spawn/group authority graph.
Admission freezes and preflights the manifest plus selected-zone ownership
capability before debit; after debit it constructs the reciprocal runtime graph
or performs the exact full rollback.
The roster travels in strategic space, preserves confirmed casualties through
physical projection and fold, and transfers its delivered survivors into held
garrison authority under an `OPEN`/`ON_STATION` operation and zero-delta receipt,
without double-counting aggregate population. Ownership
invalidation, admission failure, and prearrival termination settle through
explicit rollback or proportional survivor refund paths. Historical contract-
zero rebuilds remain isolated. Invalid or ambiguous current graphs quarantine
at `-70` without guessed deletion, refund, settlement, or outcome; linked
claimants and retention pins remain durable for diagnosis and replay safety.
Selected source/target ownership capability fencing rejects an initial ABA
change before pressure. A pressure-marked retry rechecks the same capability and
rejects before order creation or debit.

## Current Working-Tree Status

Campaign Schema 70 and runtime-settings Schema 24 remain the persisted
contracts. The current implementation rejects inactive demolition witnesses;
it introduces no save-schema or runtime-settings migration. Current source is
stamped `0e54f6cbc7f7084e5534fc603b491cba0d91b653`, UTC
`2026-07-14T18:31:39Z`, label
`schema70-settings24-active-demolition-witness`. It includes the preceding
`0b380f00fde65c4f2e22858faf8ddc6eab794131` SpawnQueue-resume checkpoint, the
`3ded248a4ded084dfb0e3aa8e54ae0a47d36cd5f` cleanup-ownership checkpoint, the
`2508a735863c153f95bae94adb13f3037b4cdeef` checkpoint-evidence matcher fix, and
the `89b7754bcd9ac7e8c41f2a8d7604784b5c1c1c83` support-roundtrip correction.

R20 `seed1985_t0_p1_u1784047342` remains the historical diagnosis of the
checkpoint-evidence false negative and four untracked Phase 22 orders. R21
`seed1985_t0_p1_u1784049066` remains the cleaner cleanup/persistence comparison:
zero open orders and zero exact runtime claimants, an exact persistence report,
and an exact-zero isolated-state diff.

Current R23 `seed1985_t0_p1_u1784054690` ran build `0e54f6c` and completed 688
cases at 564 PASS/51 WARN/66 FAIL/7 BLOCKED, with 5,440/5,650 required assertions
proven, 192 failed, and 18 blocked; it is not certified. All seven destroy-family
definitions pass mission-start, mission-runtime, and primitive-runtime cases.
The six generic destroy-target variants also pass
`primitive.destroy.no_ambient_witness_score` with zero damage, zero hits, no
source, zero evidence keys, and an undestroyed target before explicit damage.
The radio-tower definition uses its separate exact lifecycle primitive. All
seven cleanup rows remain WARN because the runner used administrative cleanup;
they are cleanup evidence rather than physical completion evidence.

R23 retains the complete `early_mechanics.spawn_queue` pass: retry generations
advance 1 -> 2, only the failed slot is scheduled, stale callbacks are rejected,
completion/replay and four registrations are exact, same-wave retry/sibling/
terminal progression is exact, and interrupted restore advances generation
1 -> 2 at sequence 9 with one first change, repeated no-op, and one completion.
Foundation, authority foundation, and all eight `local_security.*` assertions
pass.

R23 preserves exact seeded persistence at 11/11 missions, 22/22 mission assets,
21/21 runtime entities, 9/9 groups, 10/10 runtime vehicles, and 1/1 field
vehicles. Only `persistence.real_restart` is blocked in that family, and final
tracked-state restore is exact zero. One tracked exact-QRF settlement failure
leaves one tracked row open and two exact runtime claimants during the isolated
run, causing typed cleanup FAIL; the leak snapshot later records total open
orders 0 -> 2. Phase-persistence smoke/report WARN and 12 Phase-24 cases fail
later while the remnant is still present; the artifact does not assign every
downstream failure to that single cause. Clone restoration still returns tracked
state exactly to baseline.
R21 remains the cleaner cleanup/persistence comparison. World-scope restore,
external restart, package, network, and soak gates remain open.

The current source boundary includes:

- Bounded synthetic-time probes capture the shared campaign second and an
  enemy-strategic-authority fingerprint, normalize debug-owned future
  timestamps, restore the original second, and assert that both clock and enemy
  authority are unchanged before returning.
- While Campaign Debug owns an isolated state clone, the ordinary coordinator
  enemy-commander cadence is held. Phase 18, Phase 22, and Phase 24 still invoke
  the production commander explicitly, so their intended order paths remain
  covered without incidental orders or Defend Petros state leaking between
  cases.
- The isolated clone also holds ambient local-security progression whenever its
  force-spawn worker is held. This prevents a virtual patrol from entering
  MATERIALIZING with no process owner during checkpoint capture; R22 passes all
  eight local-security assertions.
- SpawnQueue bounded selection now recognizes due `DEFERRED` and
  `FAILED_RETRYABLE` slots when their owning batch can start another attempt.
  Manifest exactness and dependency registration still gate selection, and
  attempt normalization still happens after acquisition. R22 passes retry
  backoff, failed-slot-only scheduling, stale-generation rejection, duplicate-
  safe completion/replay, same-wave progression, and interrupted restore.
- Destroy-target nearby-witness scoring now rejects parented entities and
  inventory items attached to a parent slot before text classification. It
  requires an active physical projectile: either a triggered blast or a moving
  projectile. Entity-backed damage callbacks and nearby scans share one
  canonical entity source key. Each target retains at most 64 source receipts
  for its lifetime, fails closed at capacity, and writes receipt/local
  bookkeeping only after authoritative asset state actually mutates. Campaign
  Debug adds `primitive.destroy.no_ambient_witness_score` immediately before
  explicit demolition damage. R23 passes that assertion in all six generic
  destroy-target variants and passes all seven destroy-family start/runtime/
  primitive cases.
- Every Campaign Debug enemy order is tracked by its admitted stable identity.
  Cancellation, completion, and background-war cleanup dispatch open exact and
  contract-zero orders to their typed administrative settlement owners before
  any debug-prefix deletion. Cleanup then proves that no tracked open order or
  exact runtime claimant remains. Current source additionally tracks every
  order appended by the direct Phase 22 commander tick and compares the open-
  order baseline with the final count.
- Campaign marker cleanup recognizes exact QRF, counterattack,
  garrison-rebuild, and patrol markers only through the marker publisher's
  authoritative visibility predicates and canonical marker IDs. A shared
  category or linked ID alone is not accepted as backing.
- The render-bubble proof keeps synthetic clock state in
  `HST_CampaignDebugClockIsolationContext` and focused helpers. This reduced the
  already-large coordinator method's local-variable pressure after the same
  source shape reproduced a native Workbench `0xc0000374` failure before Game
  script diagnostics.
- The mission sweep now stages one disposable, debug-prefixed radio zone/site
  and supported transmitter inside its isolated state clone. Exact radio target
  selection cannot fall back to an authored tower. The ordered destroy and
  stop-rebuild cases use the normal physical callbacks, preserve production
  admission, assert receipts/epoch/rewards/one-attempt authority, and explicitly
  remove the temporary projection, transmitter, and radio-site row before live
  state restoration.
- The radio lifecycle queries the generic scripted base, the exact stock
  `SCR_DestructionMultiPhaseComponent`, and the stock destruction base, then
  returns their shared health/state authority. This applies consistently to
  fixture admission, physical damage, damage polling, authored restoration, and
  destroyed-state suppression.
- Schema-64 legacy support projection is limited to canonical curated towns.
  Nonpolitical/non-town civilian rows retain Schema-22 persisted FIA/occupier
  values, including zero; only pre-22 no-town restores receive legacy backfill.
- Campaign Debug checkpoint evidence accepts both production and debug-isolated
  checkpoint prefixes. R21 confirms that matcher and direct-tick cleanup
  ownership; R22 retains the Foundation/authority foundation passes while
  exercising the current queue stamp.

The crash-fix checkpoint passes Foundation with 793 script-symbol references,
compiles and completes Workbench create/destroy at 5,826 Game files/11,807
classes with CRC `287d01ec`, and remained alive at the 8-, 16-, and 24-second
cold-open checks before deliberate shutdown. The original disposable-radio-
fixture source passed headless Workbench script validation at the same file/class
counts, 46,634K static storage, CRC `d8a34f4b`, with a clean
`Script validation successful` result and no surviving Workbench/game process.
R16 checkpoint `a81d494` also passes fresh headless Workbench creation at
5,826/11,807, 46,639K static storage, CRC `c4113d38`, exit `0`, with no missing-
component, script, entity, or crash diagnostic and zero surviving processes.
The preceding SpawnQueue-resume tree passes headless Workbench create/destroy in
`logs_2026-07-14_13-40-55` at 5,826 Game files/11,807 classes, 46,641K static
storage, and CRC `be31cb18`, with no HST script or fatal diagnostic and zero
surviving engine processes. R22 confirms the SpawnQueue resume-selection delta
at runtime. The current active-demolition-witness tree passes fresh headless
Workbench Game/PC validation in `logs_2026-07-14_14-41-29` at 5,826 Game files/
11,807 classes, 46,643K static storage, and CRC `c3ab042e`. It reports
`Script validation successful`, no HST compile or fatal diagnostic, and zero
surviving engine processes after the test closes. R23 supplies the matching
targeted runtime evidence above.
R12 through R15 isolated the radio fixture's engine boundaries one at a time:
exact component lookup, enabled inherited resource state, engine `Kill()` for a
normal zero-health destruction callback, and the paired inherited
`RplComponent` required by enabled multiphase rebuild equipment. R15 proved the
destroy callback, receipt, objective `1/1`, exact `$450` reward, preserved
borrowed provenance, and immediate stop-rebuild admission before generated
equipment projection failed closed on the replication dependency. Every run
retained exact fixture cleanup and a zero final tracked-state diff. Checkpoint
`a81d494` enables that existing inherited RPL component without changing the
persisted schema; the audit records the latest end-to-end runtime result.
R10 remains the last proof for all five Phase 18 cases and the targeted Phase
20/22/24 boundaries. The wider runtime backlog remains open. The
[Campaign Debug verification audit](HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md)
owns run counts, log identities, and the remaining failure buckets; this
checklist records contracts and open gates rather than duplicating a rolling
run history.

## Historical Checkpoints

The entries below preserve sealed implementation and test checkpoints. They are
historical evidence, not a statement that the recorded run is the latest run
against the current working tree.

The scoped Schema-70 engine-proof checkpoint is sealed at implementation
`2f71236bfc02329a3c8000b104f1b7b1043dc99c`, UTC
`2026-07-13T22:20:52Z`, label
`schema70-settings24-exact-enemy-garrison-rebuild-engine-proof`, with stamp
commit `ef95555`. That checkpoint's post-clock-fix Workbench log
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

The recorded CLI run `seed1985_t0_p1_u1784003276` executed all 711 recorded
cases: 273 PASS, 40 WARN, 388 FAIL, and 10 BLOCKED. Certification proved
4,356/5,003 required assertions, with 598 failed and 49 blocked. Bootstrap
passed and the restored state diff was zero. That proves final restoration, not
intra-suite isolation; the run is not certification evidence.

Schema-70 `early_mechanics.force_authority` did run. Its delivery, restore, and
ownership fixture branches were trapped by live-player proximity and remained
`MATERIALIZING` instead of taking deterministic virtual projection. That
separately corrected source-only proof harness now isolates those projections
without changing
production behavior; a fresh CLI rerun is still required. Full Debug enemy
authority was healthy at campaign second 26. Captive-follow debug sampling began
at second 109 and advanced the shared campaign clock six times by five seconds
while ticking only `MissionRuntime`. The next normal resource tick correctly
quarantined both exact enemy pools for `enemy resource cadence checkpoint
diverged`. This test-harness clock leak caused the later strategic-mission
terminal/containment cascade and Phase 18 `AddResources` rejection; it was not a
separate production resource-authority defect. Source now removes those shared-
clock increments and asserts exact clock plus enemy-authority fingerprint
isolation. Production mission/resource fail-closed gates remain unchanged and
correct. A rerun is required to prove cascade removal. Earlier Phase 10-11
native exact crew-seat materialization/rollback containment failures and other
runtime defects remain separate and open. Packaged/
native execution, dedicated-server, serialization/restart, network/JIP/
reconnect, and soak gates also remain open.

The immediately preceding Schema-69/settings-24 checkpoint moved newly admitted
exact enemy counterattacks to contract `1` and one reciprocal order/operation/manifest/
batch/group graph: a frozen infantry roster travels directly in strategic
space, transfers between virtual and physical authority without restoring
casualties, resolves off-screen combat deterministically, requests any capture
through the canonical ownership service, returns to origin, and refunds living
survivors proportionally to exactly one originally charged attack or support
pool. Counterattacks restored from Schema 68 or earlier remain historical
contract `0`. Invalid or ambiguous current exact graphs quarantine at `-69`
without fabrication, deletion, refund, settlement, or outcome publication.
The appended `PREPARED` settlement state makes terminal intent durable before
the exact order/refund tuple is staged; the authority then refunds, records the
resource receipt, and finalizes, resuming safely on restore or a same-session
tick. Deterministically derived claimant IDs prevent a foreign or duplicate
batch, projection, force, or execution row from being cleaned as if it were the
expected aggregate.

The scoped Schema-69 engine-proof checkpoint is sealed at implementation
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

This seal covers source, Foundation, all-target Workbench, and focused engine
proof only. Full Campaign Debug in `HST_Dev`, serialization/restart,
package/native/live-server behavior, migration runtime, marker runtime,
network/JIP/reconnect, and soak remain open.

The preceding sealed Schema-68 planning checkpoint is implementation
`4c9a94a1cb4811b6e75a7dca5dba70efffcb523d`, UTC
`2026-07-13T15:43:01Z`, label
`schema68-settings24-enemy-planning-engine-proof`. The latest packaged
server run loaded the prior `f97b12e` build, created the canonical
`$profile:Partisan` root, and exposed a fresh-start enemy-authority regression:
configured strategic/planning
rows quarantined at `-67`/`-68`, disabling planning and emitting 598 repeated
warnings at roughly one-second cadence. No retired profile tree existed, so that
run did not exercise migration.

The sealed tree retains one production bootstrap factory for normal startup,
admin reset, and deterministic proof. It installs the exact three-role pools and
both idle enemy planners before restored-role validation. Unrelated malformed restore
state remains fail-closed. Recovery recognizes only the complete known poisoned
Schema-68 signature: the exact nonempty preset identity, three non-null role
pools, two poisoned planner rows, and empty mutation and order arrays; resource,
topology, preset, null-row, legacy-order, versioned-order, and other near misses
remain untouched. Unchanged planner-
unavailable logging now uses a 300-second reminder with immediate transition and
recovery reporting. Live Campaign Debug checks the actual state through the
read-only production exact-pool and exact-planner resolvers.

The complete retired profile tree is migrated before consumers run. Arbitrary
nested files use verified staging, destination recheck, byte verification, and
canonical/conflict-archive promotion before source removal; directory conflicts
are mirrored in the archive, and the retired root is removed only after verified
file and empty-directory cleanup. Same-process migration is guarded, while the
lack of atomic cross-process promotion/locking makes startup explicitly single-
writer. Campaign Debug's destructive marker probe now performs final production
repair and player-marker cleanup before returning. Foundation and all-target
Workbench validation pass. Foundation reports 753 script-symbol references.
Final stamped-tree all-target Workbench log `logs_2026-07-13_11-43-49` compiles
5,816 Game files/11,770 classes at CRC `5a998c21`; WORKBENCH, PC, XBOX, PS4,
and PS5 report `Script validation successful`, the process exited, and zero
Workbench processes survived cleanup. Campaign Debug, package execution,
packaged restart, actual retired-tree migration, dedicated/live-server,
multiplayer, and soak proof remain open.

The same sealed Schema-68 checkpoint makes periodic enemy target selection
commitment-aware before weighted ranking. Queued/active same-faction orders and
support plus open operations are reduced to canonical commitment roots, so linked
order/support/operation rows count once. Incompatible roots remove the target;
compatible roots remain with a deterministic `-12` score per root, capped at
`-24`. A root containing both compatible and blocking rows is counted once with
blocking precedence. Queued commitments and equivalent legacy/canonical zone IDs
participate, while settled/terminal and rival-faction rows do not. Multiple target
rejections retain a stable first diagnostic across input permutations.

An exact patrol root may coexist with a non-patrol defensive response. If order
selection instead proposes another patrol, the planner excludes that target and
deterministically reranks in the same due decision rather than wasting the
180-second cadence. Preparation is freeze-only. Admission always rechecks the
commitment fingerprint and active-order compatibility before debit, including a
pressure-marked retry; an unpressured admission also rechecks candidate/source
identity before pressure. If every target is committed, planning records an
explicit zero-cost skip without pressure, debit, order creation, or rival
mutation. Three deterministic source proofs and matching Campaign Debug
assertions cover the expanded selection/filter/diagnostic branches, the all-
committed skip, and both unpressured and pressure-marked post-freeze commitment
races. The official engine autotest
`HST_TEST_EnemyPlanningCommitmentAuthority` executed the focused authority report
in log `logs_2026-07-13_11-44-28`. Its JUnit result at
`2026-07-13T15:44:34.667Z` records one testcase, no failure, an empty failed
list, and `AllExact=true` across all 17 deterministic Schema-68 planning
fixtures, including retry quarantine repeated-pass idempotency. This focused
result does not execute Full Campaign Debug in `HST_Dev`, the coordinator
isolation/artifact path, or live campaign authority. Package, restart,
dedicated/live-server, network, and soak proof remain open.

The immediately preceding sealed source/Workbench checkpoint is the Schema-68/
settings-24 bootstrap/profile/marker correction at implementation
`fdf262637e74a70c12454f6c1d3789c2cd0a0f05`, UTC
`2026-07-13T13:19:22Z`, and label
`schema68-settings24-bootstrap-profile-marker-hardening`. The earlier Schema-68
planning-authority checkpoint is implementation
`356b0d47f96111c3b09eb7ede3cb34f0661c2b6e`, UTC
`2026-07-13T01:04:41Z`, label
`schema68-settings24-enemy-planning-authority`. The Schema-67 resource seal
identifies implementation
`2798cb20b824ed74419ab6dc9bdce03f18ef71df`, UTC
`2026-07-12T23:46:02Z`, and label
`schema67-settings24-enemy-strategic-resource-authority`. Schema 67 gives every
configured enemy role one versioned attack/support/aggression authority with
independent persisted resource-income/aggression-decay accumulators, cadence
bucket checkpoints, and a bounded replay-safe strategic mutation receipt
history. One server API owns income, spend, refund, aggression, and live admin/
debug adjustment; exact replay is read-only, fingerprint conflict and arithmetic
underflow/overflow fail atomically, and occupier/invader histories remain
independent. Zero-effect operational mutations still retain a receipt. Existing
exact QRF/patrol debit and refund policy links reciprocally into the receipts
without changing those operation contracts.

The operational bound is deliberately fail-closed rather than compacting
history. Each enemy role may retain at most 4,096 contiguous operational
receipts; the capped role rejects later operational mutations, while the other
enemy role and the compact per-role income/decay cadence continue independently.
Invalid/orphan/rejected-role receipt rows are attributed and quarantined, then
purged before capacity checks so they cannot consume valid receipt budget.

Pre-67 migration adopts current valid balances/aggression plus valid legacy
resource/aggression cadence accumulators as a baseline, initializes matching
last-bucket checkpoints, and
invents no receipt history, spend, refund, settlement, order, or planning
decision. Malformed current graphs quarantine at `-67`.
Mission outcomes admit their complete strategic plan before terminal state,
reward, capture, or cleanup publication; ownership aggression admission precedes
security/support replacement and owner publication. Disposable Campaign Debug
state clones and focused one-group materialization keep these proofs from
mutating unrelated live authority. The Schema-68 planner is not part of the
Schema-67 seal. Foundation passes at 736 script-symbol references.
Final stamped normal Workbench log `logs_2026-07-12_19-52-14` and all-five log
`logs_2026-07-12_19-52-36` both compile 5,809 Game files/11,751 classes at CRC
`a353fa0d`; all-five reports `Script validation successful` for WORKBENCH, PC,
XBOX, PS4, and PS5. Both runs have zero HST script errors, and zero Workbench
processes survived cleanup. The wired/static fixtures have not executed in
Campaign Debug, and this checkpoint does not certify any open Blueprint Phase 8
runtime gate.

The immediately preceding sealed source/Workbench save contract is Campaign Schema 66 with runtime settings
still at Schema 24. The sealed Schema-66 stamp identifies implementation
`a7031797e67d99a99a066038cd8fa39efc03cff1`, UTC
`2026-07-12T20:28:33Z`, and label
`schema66-settings24-local-security-marker-integrity`. It adds one deterministic exact local-security patrol epoch
for each eligible canonical enemy-held town. An authored 2–5 member frozen
manifest and held SpawnQueue slots own the roster; physical deaths survive fold,
re-entry, and restore without refill. Destruction applies one police `-1` event
and forbids same-epoch resurrection. A later positive police event or newer owner
revision is required to rearm. Ownership/pressure clear, setup/stop, and spawn
failure settle without that loss. Resistance-held towns now target zero
automatic police and roadblocks. Pre-66 migration preserves logical security,
ownership, support, and garrison facts while removing only backlink-free legacy
police projection rows and inventing no casualties, roster, fold credit, or
refund. Current malformed graphs quarantine at `-66`.

That sealed pass also repairs the client-local campaign-marker ownership
regression introduced at commit `27672e6`. Campaign markers are system-owned,
non-removable, and rebuilt from the authoritative client registry if native
state is deleted or mutated. Player-created/dynamic markers remain separately
editable. Foundation passes at 729 script-symbol references. Final stamped
normal/all-five Workbench checks pass at 5,806 Game files/11,740 classes with
CRC `ec860be7`, `Script validation successful`, zero HST script errors, and zero
surviving Workbench processes. The deterministic proof remains unexecuted in
Campaign Debug, and all native, save/restart, marker-input, package, multiplayer,
and soak gates remain open.

The previous sealed development save contract is Campaign Schema 65 with
runtime settings still at Schema 24. It identifies implementation
`609add9eeadf73816764c497178e2d35081307d1`, UTC
`2026-07-12T18:30:29Z`, and label
`schema65-settings24-civilian-consequence-authority`. This Blueprint Phase 8 civilian-consequence
slice adds bounded 256-row casualty and 64-row theft queues, a shared four-
transaction-per-frame drain, indefinite retry with bounded backoff, capture
deferral while consequence authority is pending, a civilian-theft receipt only
after exact-pilot durable vehicle promotion, and a persisted locality combat-
danger episode envelope. Passenger occupancy protects the transient root but is
not a claim or theft signal. Nearby-combat consequences require the current combat-presence
revision plus a positive current-operation or recent-fire fact; `HOT` alone is
inert. Canonical-town events preserve logical population, faction support,
reputation/wanted heat, and exact enemy-aggression before/after evidence while
physical pedestrians follow `Wandering -> Panicked -> Recovering`. Minor
localities remain panic-only where allowed and use bounded session-only receipt
fingerprints rather than invented political history.

Deterministic state-only proofs are complete and wired for casualty attribution,
replay/conflict, theft, combat episodes and `HOT`-only rejection, minor-locality
behavior, malformed authority, aggression admission, persistence validation, and
panic/recovery transitions. They are not native runtime certification and do not
perform real profile I/O. Final stamped normal Workbench compile/create and all-
five validation are clean at 5,802 Game files/11,728 classes with CRC
`c0a672b9`; all-five reports `Script validation successful`, both runs exited
`0`, zero HST script errors were observed, and zero Workbench processes survived
cleanup. The preceding unstamped CRC `be076102` remains preliminary evidence
only. Foundation passes at 717 script-symbol references. Campaign Debug execution,
native callback/waypoint behavior, real save/restart, rendered UI, packaged
server/client execution, multiplayer, and the ten-town/ten-minute stutter/churn
soak remain open.

The earlier sealed checkpoint is Campaign Schema 64/runtime-settings Schema 24
under implementation
`6afadc7c13681b78171939a740862e52328beffd`, UTC
`2026-07-12T15:57:55Z`, and label
`schema64-settings24-ambient-runtime-authority`. Final sealed-tree evidence is
Foundation at 711 script-symbol references plus normal Workbench compilation and
all-five-configuration validation at 5,799 files/11,718 classes with CRC
`bb083672`, explicit validation success, zero HST script errors, and zero
surviving Workbench processes.

The preceding sealed checkpoint is Campaign Schema 64 on runtime-settings
Schema 23. It makes `HST_TownInfluenceRecord` the sole political support and
population authority for curated towns, retains separate FIA/occupier/invader
basis-point totals, uses strict `>8000` resistance and `<4000` enemy ownership
hysteresis, and delegates every resulting flip to
`HST_OwnershipTransitionService`. It also replaces generic Map/War pressure
with contacted-town Zone Pressure and a complete deterministic Resistance
Territory projection. Legacy support/population fields are migration/read-only
projections. That checkpoint identifies implementation
`6f3c913eaed66926cce38b2ecafcff94084898a3`, UTC
`2026-07-12T11:28:41Z`, and label `schema64-canonical-town-influence`.
Foundation passes for it at 696 script-symbol references.

Schema 63 is an earlier sealed source/Workbench checkpoint. It identifies implementation
`85a75c65e9c148a890d8d78b0288ae6483a5ccd9`, UTC
`2026-07-12T08:22:05Z`, and label
`schema63-canonical-combat-presence`.

Schema 62 is the preceding sealed canonical-ownership checkpoint under implementation
`7c93e0a485bcabe5a364c0b0cfeca235accb50f7`, UTC
`2026-07-12T06:11:19Z`, and label
`schema62-canonical-ownership-transition`. Foundation passes with 670 script-
symbol references. Headless Workbench Game validation loaded 5,785 files/11,652
classes with CRC `22c13a32` and zero script errors; the normal Script Editor open
remained responsive without a crash, and zero Workbench processes survived the
test. Schema 61 is the preceding sealed marker-projection foundation. Campaign
Debug and packaged-runtime evidence remain open.

This document tracks the feature-complete campaign target for Partisan. It is
implementation-focused: every row should eventually map to state fields,
service ownership, server actions, reports, persistence, and a Full Campaign
Debug proof. Implemented, verified, and certified are deliberately separate:
code presence is not runtime evidence, and runtime evidence is not certification
when the proof mutates the campaign it is meant to inspect.

## Final Target

Partisan should become a Reforger-native, server-authoritative, persistent,
whole-map resistance campaign where players start weak, build local support,
loot and steal resources, recruit and train forces, capture strategic zones,
survive escalating enemy pressure, and win through popular support and decisive
military control.

The architecture target remains:

```text
server-authoritative
persistent-state-first
abstract off-screen
physical near players and active objectives
deterministically generated
explainable through reports and debug UI
safe across save, restart, reconnect, and long sessions
honest about unavailable base-game assets
```

## Status Legend

| Status | Meaning |
| --- | --- |
| Implemented Foundation | The state/service/API exists and is integrated enough to build on. |
| Broad Alpha | Playable or testable shape exists, but behavior is simplified or needs content, tuning, or soak. |
| Partial | Some state/API exists, but the gameplay loop is incomplete. |
| Missing | Required for the final campaign target and not meaningfully implemented yet. |
| Deferred | Intentionally disabled until Reforger/base-game asset support exists. |
| Needs Soak | Implemented enough to test, but needs repeated save/load/MP/long-run validation. |
| Needs Runtime Proof | The source path exists, but required compile/startup and/or isolated runtime evidence is still missing; the row names the open gate. |
| Unsafe On Live State | A debug or migration path can persist destructive test mutations and must be isolated before use on valuable state. |

## Current Delivery Gate — Campaign Runtime Integrity

`Designed` means the contract and dependency order are recorded. `Implemented`
means the named production slice exists. `Verified` means an appropriate proof
has actually run against that slice. `Certified` additionally requires a safe,
isolated runtime run with no unresolved hard failures or required external gaps.

R23 is the latest targeted in-process diagnostic at 564 PASS/51 WARN/66 FAIL/7
BLOCKED, with 5,440/5,650 required assertions proven, 192 failed, 18 blocked,
and an exact-zero final tracked-state diff. Fresh Workbench validation and the
targeted active-demolition-witness runtime gate pass: all six quiet generic
destroy-target assertions and all seven destroy-family start/runtime/primitive
cases are green. The complete SpawnQueue family, Foundation/authority
foundation, all eight local-security assertions, and exact seeded persistence
counts also pass; only `persistence.real_restart` is blocked in that persistence
family. One tracked exact-QRF settlement failure leaves one tracked row open and
two runtime claimants, directly breaks typed cleanup, and precedes a leak
snapshot of two total open orders. It remains present during later phase-
persistence warnings and 12 Phase-24 failures without proving one cause for
every downstream case. R21 remains the
cleaner cleanup/persistence comparison, R10 the last positive baseline for the
boundaries, and R16 the isolated radio proof. The wider R23 run is diagnostic,
not certification evidence. Repair the exact-QRF settlement defect next, then
address proof-fixture dependency/radio ordering and prove world-scope/external
restart, package, native movement and combat, dedicated-server/client,
multiplayer, reconnect/JIP, and soak behavior.

### Historical Contract Context

The preceding Schema 68/settings 24 enemy-planning engine-proof checkpoint is
recorded under the identity above. It retains the bootstrap/profile/marker
correction from the immediately preceding seal. The previous packaged baseline
created canonical data but quarantined both fresh enemy authorities and emitted
598 one-second warnings. The new shared bootstrap, exact recovery/rejection boundary, read-only
live authority check, warning throttle, staged whole-tree profile move, and
marker-integrity cleanup plus commitment-aware planning pass Foundation 753 and
final stamped-tree all-target Workbench validation in
`logs_2026-07-13_11-43-49` at 5,816 Game files/11,770 classes and CRC
`5a998c21`; all five configurations succeed, the process exits, and zero
Workbench processes survive cleanup. Campaign planning also has focused official
engine-autotest evidence: one testcase with no failure, an empty failed list,
and `AllExact=true` for all 17 deterministic Schema-68 planning fixtures,
including retry quarantine repeated-pass idempotency, in log
`logs_2026-07-13_11-44-28` at JUnit UTC `2026-07-13T15:44:34.667Z`. Full Campaign
Debug in `HST_Dev`, coordinator isolation/artifact generation, live authority,
package execution, packaged restart/migration, live-server, multiplayer, soak,
and marker gates remain open before this checkpoint can be runtime-certified.

The sealed Schema 66/settings 24 checkpoint retains Schema
65's civilian-consequence authority and Schema 64's one canonical town record
for FIA/occupier/invader support, population,
contact, event aggregates, and political-flip state, then adds a revisioned
locality consequence envelope and exact aggression target/delta/before/after
evidence on town events. Logical population/support, reputation/wanted heat, and
enemy aggression remain durable independently of ambient actor count. The pinned
town formula remains commit `6e4226d3863ca8673535386c2fff8b6e08a806c4`:
population-scaled raw `+1` yields `+100`, `+200`, and `+50` basis points at
initial populations 100, 25, and 400, and equality at `8000` or `4000` does not
cross a flip boundary. Schema-64 saves initialize the new envelope from exact
current-operation/recent-fire facts, mark any active baseline as the adopted
applied floor, and do not create or replay political/aggression events. The
floor is `0..1`, `episode - lastApplied <= 1`, and any pending combat receipt
drains before a new danger edge. Simon's Wood remains a panic-only
minor locality rather than a political town, while Maiden's Bay remains the
Logistics Warehouse and receives no town-influence record. Current malformed
Schema-65 envelope authority quarantines at `-65`; invalid town influence still
fails closed through the existing `-64` boundary. Schema 66 adds exact enemy-
town local-security epochs above the police-pressure scalar and quarantines
malformed graphs at `-66`. Physical patrol roots/routes remain disposable, while
the frozen manifest and SpawnQueue slots own casualties through fold and restore.
Resistance automatic police/roadblock targets are zero. The same sealed tree
protects client-local campaign markers from the local-owner regression at
`27672e6`; player-authored markers retain their separate editable path.

Schema 63 remains the sealed combat-presence dependency beneath this pass. It adds one
crew-aware combat-presence authority, revisioned per-zone `HOT`, `COOLING`, and
`COLD` diagnostics, conservative restore/migration, and runtime-settings Schema
23's 30-second default cooling window. Physical samples accept only fresh
registered conscious infantry and operators of operational armed mobile/static
platforms; cargo, empty vehicles, destroyed/burning platforms, and immobile
mobile platforms do not contribute. Unresolved spawned-group physical authority
invalidates the affected query and makes consumers fail closed. Capture counts
only living conscious character players, excluding spectator/Game Master
proxies. Contribution, authority-gap, and zone/radius result caches share an
indexed runtime registry and reusable sampling buffers on the one-second path.
Foundation, normal Workbench compile/create, and explicit five-configuration
Script validation pass for both the sealed Schema-63 checkpoint and the
sealed Schema-64 tree. Schema-64 Campaign Debug and runtime verification
remain open.

Schema 64 composes the sealed Schema-62 ownership contract with Schema-63 combat
presence. Every location has
ownership contract version `1`, a monotonic ownership revision, and active/latest durable
receipt backlinks. Military capture, mission capture, political support, admin,
debug seed, and migration repair use one canonical transition request. The
receipt orders old exact/aggregate security and hostile-runtime retirement, new
security, frozen support consequences, owner revision, town and generated-site/
facility/logistics derivation, retaliation, economy/outcome, events, parent-aware
projection, notification, and persistence scheduling. Retry-capable steps occur
before owner publication. Identical replay is a no-op; changed fingerprints,
stale preconditions, and unsupported security fail closed. Current malformed
authority quarantines at `-62`. Cause flags remain deliberate: admin changes
reconcile security and notify without retaliation; debug seed also suppresses
notification; migration repair preserves security and suppresses retaliation/
notification; military and mission capture retain normal enemy consequences.
Admission and save normalization enforce those cause flags, and admission stops
before a revision that cannot safely advance to valid serialized authority.
Town support callers now submit typed influence events to the canonical town
service, and threshold intent enters this same transition queue; neither legacy
support fields nor political callers may publish an owner directly.

Schema 61 remains the sealed marker-only projection base. Schema-62 marker
protocol `2` adds a source ownership revision to the existing marker-local
revision, global stream sequence, epoch, tombstone, bounded snapshot/delta,
ownership-derived session, and atomic client-registry model. Nested political
flips caused by a capture delegate marker/menu/GM/notification publication to
their parent. If an earlier parent's exact linked-town support event encounters a
later pristine receipt already owning that town backlink, the influence fact
still applies exactly once and only threshold reconciliation waits for FIFO drain
and the periodic civilian pass. Later valid top-level requests are admitted as
pristine accepted/needs-retry receipts and execute in array order before any
domain mutation, so
exact mission, political, admin, and migration intent survives the fence and
restart. Rebuilt explicit-ID commands reuse frozen preconditions only after
semantic identity matches. Supported zone aliases canonicalize before request
fingerprint/replay.
Every later unresolved top-level follower must remain fully pristine; pre-owner
status alone is insufficient as restore authority.
Ordinary refreshes ignore queued pre-owner receipts but retain the prior
owner/revision for active owner-applied or completed unreleased receipts;
command-menu ownership is resolver-first and a retained marker only corroborates
the exact resolved owner/source revision. Unsafe marker rows quarantine/purge;
unavailable authority never falls back to a prior/raw owner. Parent publication
captures the full logical marker array plus epoch/sequence, stages one exact
parent/child receipt-zone-marker graph, validates and releases every child, then
commits. Failure restores the complete prior snapshot. Setup publication freezes
immutable `m_bSetupProjectionWithoutMarkers` receipt history that survives later
activation.
Restore accepts serialized queues only while every later
unresolved top-level follower remains pristine, and quarantines multiple owner-
applied incomplete publishers or an owner-applied publisher behind
earlier unresolved top-level authority, then iterates projection-parent
quarantine to a fixed point. Runtime/restore retry retains a concrete quarantine
cause instead of replacing it with generic resume text. Current-schema
normalization also rejects unrelated forged projection children and duplicate
completed claims for one zone/applied revision. It recomputes the exact sorted
support-target set of linked towns plus every town within 1,500 m, requires
applied targets to be its ordered prefix and each to
match the same single deterministic influence row/deltas, and validates reason,
garrison, counterattack/order, strategic/campaign event, marker, and setup-mode
correlations. Ownership strategic events record exact owner/capture/aggression
facts without unrelated queued/retry global deltas. Deterministic source fixtures
use production's logical marker-snapshot builder and cover all cause routes, queued
intent, replay/conflict/stale handling, recapture, interrupted restore,
source-revision correlation, co-located identity, two-child atomic publication,
staged full-snapshot rollback, resolver fail-close, setup history across
activation, support target/prefix and derived-correlation corruption,
persistence-deadline re-arm, malformed current-schema queue order, two restart
boundaries with exactly-once political completion, non-patrol/
orphan/late exact-security rejection, migration/quarantine, and retention. They
are not executed Campaign Debug or packaged multiplayer proof.

Schema 61 is the preceding sealed marker-projection foundation under
implementation `27672e67ce4285810f313130293df1ac917c9bdf`, UTC
`2026-07-12T01:02:39Z`, and label
`schema61-authoritative-marker-projection`; it is not a packaged-runtime
checkpoint. Full Foundation passes with 655 symbol references; final Workbench
Game validation loaded 5,782 files/11,631 classes with CRC `df41a779` and
created the game; hidden normal WorldEditor stayed responsive 10/10 over 20
seconds without a first-party error/crash signature. The preceding Schema-60 slice only opts newly quoted
and confirmed player Search-and-Destroy requests opt into the new contract-1
infantry operation. Its immutable quote/manifest, $350 plus HR-per-frozen-slot
ledger, held batch, active group, direct-route cursor, virtual combat, exact
casualty roster, return-to-assignment behavior, commander recall, and settlement
stay one authority graph. Physical fold, recall exit, and campaign-stop
retirement exhaustively reconcile the affected projection and require exact
root/member binding cardinality while survivors remain. Persistence applies the
same exhaustive casualty pass globally, validates reciprocal support ownership,
and refreshes the physical group position before capture. Held-batch cancellation
first snapshots the strategic living roster for immediate-recall settlement.
Pre-60 Search-and-Destroy stays contract `0`; malformed
current exact claimants quarantine at `-60` without legacy fallback or guessed
balances. Their groups are globally non-operational and not combat-present.
Expired archive capacity removes a valid exact-support tombstone and paired
terminal request only after replay, unique-identity, no-live-backlink, and full
receipt-reciprocity checks; corrupt/quarantined pairs remain evidence. The
compiled/wired proof includes valid pair prune/restore and corrupt retention,
but its assertions have not run. Schema 60 also retires the overlapping Maiden's Bay town in favor of
the Logistics Warehouse. No-anchor state is untouched, ambiguous authorities
fail closed, mutable generic references canonicalize, and all nonzero typed
authority—including settled/quarantined/malformed and graphless exact rows—
keeps its frozen ID. Frozen generated content receives a deep canonical clone
instead of being rewritten. Schema 60 also contains focused source repairs for
the observed one-second stutter and continuous AI horns.

Schema 59 is the preceding stamped source/Workbench checkpoint. It gives every
radio zone one durable lifecycle row and opts only newly started
`destroy_radio_tower` and `dynamic_stop_tower_rebuild` into radio-site contract
`1` and quarantine `-59`. Its stamped checkpoint identifies implementation
`37fb5f0ffbc80c4bba3151ba1e5d8be6ffcf8a21` with build label
`schema59-radio-site-lifecycle`; Workbench loaded 5,773 files/11,608 classes with
CRC `96914c26`, and the normal open stayed alive/responding for 10/10 samples.
Schema 60 independently passes the full Foundation gate with 644 symbol
references. Its final stamped Workbench Game validation loaded 5,777 files/
11,615 classes with CRC `7aa80fc9` and created the game; the correctly targeted
hidden normal WorldEditor stayed alive/responding for 10/10 samples over 20
seconds with no first-party error/crash signature. Diff check was clean apart
from line-ending warnings. Both Schema-60 proof services and the typed-QRF
mismatch assertion remain compiled/wired but unexecuted. Packaged server/client,
actual save/restart, physical movement/combat, rendered UI, reconnect, JIP,
stutter, and horn verification remain open. Schema 58 remains an earlier stamped checkpoint at implementation
`f0ba07ff2bc295d12542a3ea34b4c913e99b1869` and label
`schema58-exact-rescue-pows`. Schema 57 remains earlier history at implementation
`514ebdcbeb1ddfb2a383b19590382517113e2ff6` and label
`schema57-exact-specops-guard`; Schema 56 remains earlier history at
implementation `bab5748d817ba434dae701cfbb3b92805d463678`, stamp
`03a65cd33bee69c6320389803cdd5a2ec8576fb0`, and label
`schema56-exact-traitor-guard`.

### Active Gate Matrix

Source work has entered Blueprint Phase 9 of 13 through Schema 70, whose scoped
engine-proof checkpoint is sealed. This is sequence position, not nine
completed or certified phases. Blueprint Phase 8 remains runtime-uncertified,
and every earlier phase still has native/dedicated/restart or multiplayer exit
gates. Native tests were deliberately deferred during source implementation and
must be backfilled; an active later source contract does not waive those gates.

| Gate | Designed | Implemented | Verified | Certified | Current evidence / blocker |
| --- | --- | --- | --- | --- | --- |
| CRI-0 Truth and baseline | Campaign Schema 70/runtime-settings Schema 24 is the current persisted contract; the scoped Schema-70 rebuild checkpoint remains sealed historical evidence | Current source `0e54f6cbc7f7084e5534fc603b491cba0d91b653`, label `schema70-settings24-active-demolition-witness`, adds no schema/settings migration. It retains the R22-proven queue boundary and rejects parented/inventory demolition witnesses, canonicalizes entity-backed source identity, keeps 64 lifetime receipts, performs mutation-first bookkeeping, and adds `primitive.destroy.no_ambient_witness_score` | Fresh Workbench validation passes. R23 completes 688 cases at 564 PASS/51 WARN/66 FAIL/7 BLOCKED with 5,440/5,650 required assertions proven. All six quiet generic destroy-target assertions and all seven destroy-family start/runtime/primitive cases pass; SpawnQueue, exact seeded persistence, exact-zero final restore, and eight local-security assertions remain green. The unresolved exact-QRF settlement crash window leaves one tracked row open and two exact runtime claimants, directly breaks typed cleanup, and precedes a leak snapshot of two total open orders. Phase-persistence warnings and 12 Phase-24 failures occur later while the remnant remains, without proving one cause for every case; R21 remains the cleaner cleanup/persistence comparison | No | Repair the exact-QRF settlement crash-window defect while preserving R23 demolition-witness and R22 queue evidence plus R21 cleanup/persistence evidence. Proof-fixture dependency/radio ordering and the remaining world-scope/runtime gates remain before external restart, package, native world, dedicated-server/client, multiplayer, reconnect/JIP, and soak proof. |
| CRI-1 Authority foundation | Complete | Prior vertical slices plus one exact durable radio-site owner and one concrete stock damage-authority resolver | R16 proves the fixture-only destroy/rebuild chain through normal engine callbacks, deterministic receipts, unchanged epoch, exact rewards, second-attempt rejection, fixture cleanup, and zero final diff | No | One site row per radio zone owns stable target binding, ONLINE/DESTROYED/REBUILDING state, ownership, mission lock, typed transition, revision, and receipts; each mission owns a distinct physical runtime identity. Stop-rebuild is once per tower-destruction epoch, and stopping its equipment does not advance that epoch. Packaged authored-content binding, restart/streaming, multiplayer, and soak proof remain. |
| CRI-2 Force manifests | Complete for the sealed foundation and scoped Schema-70 engine proof | Durable SpawnQueue and exact infantry adapters retain the sealed consumers; the garrison-rebuild slice adds one capacity-bounded frozen infantry manifest without widening vehicle, asset, or multi-root admission | Foundation 790 plus focused deterministic admission/capacity, delivered-held, casualty-continuity, and restore assertions pass; native/package/restart behavior remains unproved | No | Package-prove that the roster remains frozen through live casualties, virtual/physical transfer, delivery, re-entry, and restart without refill or aggregate double count, while historical contract-zero rebuilds remain isolated. |
| CRI-3 Force runtime | Complete for scoped source/engine proof; runtime certification open | Existing casualty/reprojection paths remain. Schema 70 adds exact garrison-rebuild strategic/physical transfer, casualty fold, delivered held-roster authority, and terminal survivor settlement over one durable roster | R10 passes all five Phase 18 cases plus bounded shared-clock and enemy-strategic fingerprint isolation. Production render-bubble behavior is unchanged | No | Package-prove live rebuild movement, casualties, fold/re-entry, held delivery, ownership invalidation, prearrival settlement, and restart alongside every earlier force family. |
| CRI-4 Operation records | Scoped Schema-70 engine proof sealed for exact garrison-rebuild authority | Schema 70 adds a separately typed exact rebuild graph with reciprocal order/operation/manifest/spawn/group identity. Durable PREPARED resource-receipt and SETTLED-before-order-tail crash windows reconcile through production restore without duplicate refund or mutation | Foundation/focused proof and all five R10 Phase 18 cases pass; typed order cleanup leaves zero settlement failures, open orders, or runtime claimants | No | Package-prove the reciprocal graph, crash-window reconciliation, terminal cleanup, historical isolation, current `-70` quarantine, serialization, and restart. |
| CRI-5 Physical movement | Complete | Exact route/movement owners exist for selected support, QRF, patrol, convoy, and mission consumers; the current ambient slice adds behavior-ready traffic admission plus bounded movement recovery | Constituent source and Workbench gates pass, but the Blueprint Phase 5 native exit gate has not run | No | Prove configured supports move, identical waypoints are not reissued every update, groups do not systematically stop at every path node, convoys maintain useful spacing and complete routes, physical/strategic arrival agree, and stuck recovery resets after real progress. |
| CRI-6 Client projection | Complete for marker records in source | Revision/tombstone records, bounded snapshot/delta delivery, ownership-derived sessions, atomic client reconciliation, and protected system-owned markers remain. Exact QRF, counterattack, rebuild, and patrol audit backing now delegates to the publisher's canonical ID and visibility predicates | Source/static validation covers the authoritative backing route; rendered marker integrity and the destructive owner-client probe remain runtime gates | No | Execute the probe, then use a packaged host plus two clients to prove ordered create/update/delete, gap/resync, map reopen/restart, one self-healed protected marker, exact operation-marker continuity, and player-marker isolation. |
| CRI-7a Canonical ownership | Complete in Schema-62 source for location owner changes | One revisioned request/receipt routes military, mission, political, admin, debug, and migration causes; valid later requests become pristine queued receipts and array-order execution owns security, support, owner, derived policy, retaliation/economy/events, transactional parent-aware projection, notification, and persistence scheduling | Foundation and Workbench compile/validation pass for the sealed Schema-64 integration; its Campaign Debug and runtime behavior remain unverified | No | Schema 64 routes strict town-support threshold intent through this service and removes direct political owner publication. Package-prove exact-patrol settlement, setup/terminal retry, invalid-owner migration, admin accepted-pending reporting, post-liberation security, marker protocol-2 source revision, political retry/resume, counterattack/economy/event effects, quarantine, retention, and all callers. |
| CRI-7b Canonical combat presence | Complete in sealed Schema-63 source; Foundation and Workbench validation pass | One shared crew-aware query and zone-heat service consumes fresh registered physical samples or eligible durable virtual infantry. Capture, missions, HQ, civilians, and enemy strategy use the same result; empty/cargo-only vehicles never block. Zone diagnostics follow `HOT -> COOLING -> COLD`, defaulting to 30 seconds, and activation uses a larger exit radius than entry. | Foundation passes at 681 references; normal Workbench open compiled/created 5,788 files/11,670 classes at CRC `a40056c5` with no HST script error or crash, and explicit validation passes for all five configurations. Campaign Debug has not run. | No | Prove conscious/unconscious, dismounted/cargo/pilot/turret, armed/unarmed, mobile/static, destroyed/burning/immobile, sample freshness, virtual casualty continuity, fail-closed authority gaps/player filtering, cooling deadline, conservative migration/restore, no per-second allocation/save churn, all shared consumers, and native activation/deactivation behavior. |
| CRI-7c Canonical town influence and political map | Designed and sealed in Schema-64 source/Workbench | One `HST_TownInfluenceRecord` per curated town owns separate FIA/occupier/invader basis points, population, contact, event aggregates, and strict flip intent. Typed callers use the pinned population formula; exact events preserve population before/after and current restore validates the chain. Legacy fields project only. Zone Pressure filters contacted towns and sorts current first, then FIA support/name/ID; Resistance Territory is complete, deterministic, and parent-publication fenced | Foundation passes at 696 references; normal and all-configuration Workbench checks pass at 5,793 files/11,695 classes with CRC `36d5b017` and zero HST script errors. Campaign Debug, save/restart, and packaged results remain open | No | Prove `+1` at populations 100/25/400 yields 100/200/50 bp; `8000`/`4000` equality does not flip; every flip enters ownership receipts; pre-64 migration/current `-64` quarantine is conservative; occupier and invader remain distinct; Simon's Wood and Maiden's Bay stay nonpolitical; contact and both Map/War lists remain exact across restart/JIP. |
| CRI-8 Civilian runtime and political consequences | Ambient and first civilian-consequence authority are designed | Sealed Settings-24 ambience and Schema-65 consequences remain unchanged beneath sealed Schema 66 and sealed-source Schema 67 | Schema-65/66 Foundation/Workbench gates pass; native callback, movement, local-security, and real save/restart execution remain unverified | No | Native-prove the existing casualty/theft/combat/panic contract, aid, and security-pressure sources alongside the exact local-security boundary. Schema 67 does not certify this exit. |
| CRI-8b Exact enemy-town local security | Complete in sealed Schema-66 source/Workbench | One deterministic enemy-town epoch owns an authored 2–5 member manifest, held exact slots, physical/virtual transfer, exact casualties, compact terminal history, once-only police loss, no-resurrection, and bounded rearm. Resistance automatic police/roadblock targets are zero. Campaign Debug now holds ambient progression whenever its matching force-spawn worker is held | R22 retains the eliminated checkpoint materialization deferral and passes all eight `local_security.*` assertions | No | Package-prove native spawn/waypoints, casualty fold/re-entry, save/restart, ownership ordering, terminal/setup cleanup, no refill/no same-epoch resurrection, positive-pressure/new-owner rearm, pre-66 migration, multiplayer, and balance. |
| CRI-9 Canonical enemy strategic resources | Source-complete sealed Schema-67 source/Workbench contract | Versioned per-enemy attack/support/aggression pools, cadence bucket checkpoints, and immutable receipts with a contiguous per-faction operational sequence. Zero-effect operations retain evidence; exact QRF/patrol orders, defense ledgers, town events, and ownership transitions are reciprocal restore backlinks. Operational history never compacts: 4,096 accepted rows per faction is a hard lifetime limit and later operational admission for only that faction fails closed | Foundation passes at 736 references; final normal/all-five Workbench checks pass at 5,809/11,751 with CRC `a353fa0d`, successful five-configuration validation, zero HST script errors, and zero surviving processes. Core deterministic assertions are wired/static but unexecuted; real restart and packaged proof remain pending | No | Execute the source fixtures and real-restart replay/conflict/arithmetic/cadence/backlink/cap/quarantine cases; unsupported orders remain legacy/deferred. |
| CRI-9b Persisted enemy planning | Sealed Schema-68 commitment-aware source/Workbench contract with focused engine proof | One production factory supplies configured pools/planners; planning freezes inputs, filters commitment roots, revalidates admission before debit, and preserves fail-closed restore behavior. Campaign Debug holds only the ambient coordinator commander cadence while explicit production-path fixtures run | R10 passes held ambient cadence, all five Phase 18 cases, exact clock restoration, and unchanged enemy-strategic authority | No | Package-prove planning, exact recovery, near-miss quarantine, restart, dedicated-server networking, and soak without cadence-warning or incidental-order recurrence. |
| CRI-9c Exact enemy counterattack | Scoped Schema-69 source/Foundation/all-target Workbench/focused-engine checkpoint sealed | New contract-`1` rows freeze an infantry manifest, charge exactly one attack/support pool, travel directly while virtual, preserve casualties across projection changes, resolve deterministic virtual combat, enter canonical ownership transition, return, and refund only surviving roster cost. Appended `PREPARED` terminal intent makes settlement resumable, and explicit plus derived-ID claimant scans reject ambiguous cleanup. Historical counterattacks remain contract `0`; malformed current graphs quarantine at `-69` without fabricated authority, deletion, settlement, refund, or outcome | Foundation 771 and all-target Workbench pass. Focused log `logs_2026-07-13_15-42-52` has one passing JUnit testcase, an empty failed list, and `AllExact=1`, including valid PREPARED recovery, same-session ABORTED recovery, foreign derived-ID collision hold, and fail-closed SETTLED-without-receipt handling; the environment also records a recoverable base-game VM exception before successful HST completion | No | Execute Phase 17, then package-prove physical/virtual combat, ownership retry, return, proportional one-pool settlement, migration, quarantine idempotency, save/restart, marker continuity, multiplayer/JIP/reconnect, and soak. |
| CRI-9d Exact enemy garrison rebuild | Scoped Schema-70 Foundation/Workbench/focused-engine checkpoint sealed | New contract-`1` rows bind one capacity-bounded frozen infantry manifest, one prepaid support debit, and reciprocal order/operation/manifest/spawn/group authority. Casualties persist across strategic/physical transfer; delivery hands survivors to held garrison authority without aggregate double count. Historical contract-zero rebuilds stay isolated; malformed current graphs quarantine at `-70` | All five R10 Phase 18 cases pass. Campaign Debug cleanup routes every tracked exact or legacy order through its typed administrative settlement owner and leaves zero failures, open orders, or runtime claimants | No | Package-prove native movement/projection/fold/delivery, held-roster continuity, crash-window restore, ownership ABA rejection, quarantine idempotency, dedicated-server, networking/JIP/reconnect, and soak. |

## Implementation Contract

Every feature slice should follow the same vertical path:

```text
durable state fields
request/result object where useful
server-authoritative command or service method
validation and clear failure reasons
physical behavior near players/events
abstract behavior off-screen
report/debug command
save migration if durable state changes
safe isolated Full Campaign Debug or scoped HST_Dev proof
```

Avoid adding durable truth to runtime entity handles. Physical entities are
projections of campaign state and must be restorable, foldable, or disposable.

## Feature Matrix

### Runtime Ownership And Service Spine

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Server-owned coordinator | One server-side component owns lifecycle and delegates domain logic. | Implemented Foundation | Keep moving feature logic from coordinator into services when it becomes reusable. | Keep |
| Domain services | Economy, missions, persistence, HQ, arsenal, loot, support, enemies, civilians, markers, garrisons, and physical war have broad service owners. | Implemented Foundation | Support recall now exposes an explicit typed result through service, coordinator, UI dispatch, receipt, and diagnostic boundaries. Ownership is not yet uniformly enforced for other commands; continue extracting coordinator-heavy mutations behind explicit service boundaries. | High |
| Server-authoritative actions | Clients request actions; the server resolves identity, permissions, phase, costs, targets, and mutation. | Broad Alpha | Visible training carries a request ID and durable receipt; visible support recall additionally derives receipt status from its typed accepted result and schedules receipt persistence even for unchanged/rejected outcomes. Extend that explicit completion contract to other mutating actions after their exact inputs exist. | Highest |
| Stable authority IDs and receipts | Server mutations use persisted monotonic IDs, bounded receipts, and replay-safe results. | Implemented Foundation / Needs Runtime Proof | Schema 65 makes stable-ID exhaustion return an empty ID rather than overflow or wrap; strategic/town admission then fails before mutation. Schema 58's reciprocal rescue identities remain unchanged. Historical POWs and unsupported families remain unversioned; native replay/restart evidence is still required. | Highest |
| Durable operation state | Every force/support/order has an immutable assignment, duty state, engagement mode, materialization state, route progress where applicable, recall/return policy, and terminal result. | Sealed multi-family foundation plus scoped Schema-70 engine proof | The exact garrison-rebuild type adds capacity-bounded admission, casualty continuity, held delivery, and PREPARED/SETTLED reconciliation. R10 proves all five Phase 18 cases and typed cleanup before prefixed deletion. Package-prove runtime/restart, generalized encounters, vehicle/multi-root policy, JIP, historical missions/garrisons, and other families. | Highest |
| Runtime diagnostics | Every major system has report/debug visibility. | Implemented Foundation | Schema 63 adds bounded sorted combat-presence contributor IDs/facts, separate infantry/manned-mobile/static counts, operation/recent-fire context, revision, state, reason, cooling remainder, and deterministic hash. Foundation and explicit Workbench validation pass, but the source proof has not run; add rendered/admin reporting after runtime execution. | High |

### Persistence And Restart Safety

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Versioned save state | Durable campaign facts survive restarts and schema migration. | Implemented Foundation / Scoped Schema-70 Engine-Proof Restore Contract Sealed | Schema 70 preserves historical rebuilds at contract `0`, validates current reciprocal claimant graphs before generic normalization, quarantines malformed/orphan current authority at `-70`, and reconciles valid PREPARED/SETTLED crash prefixes idempotently. Focused restore/replay proof passes; real serialization/restart remains open. | Highest |
| Runtime settings migration | Generated profile settings migrate forward without keeping obsolete setup knobs. | Provisional Settings Schema 24 Source / Current Foundation And Compile Proven / Needs Migration Runtime Proof | The `23 -> 24` semantic migration remains unchanged. Before settings load, the entire retired tree moves into `$profile:Partisan`; each destination must match byte-for-byte before source deletion, and canonical conflicts archive without overwrite. The latest package had no retired tree, so actual migration/removal remains unproved. | Keep |
| Profile fallback saves | Scripted saves work when native persistence is unavailable. | Implemented Foundation / Needs Restart And Migration Proof | Fallback saves use `$profile:Partisan/HST_CampaignSaveData.json`. Whole-tree migration moves an older fallback before persistence consumes it; existing canonical content wins while different retired content is archived. Prove copy verification, source/root removal, schema normalization, and repeated real restart before promising long-campaign safety. | High |
| Active runtime restore | Active missions, support, enemy orders, groups, vehicles, garage records, and undercover state restore without duplication. | Broad Alpha / Needs Soak | Build one repeatable restart route that touches all active record types. | Highest |
| Terminal campaign restore | Won/lost campaign state stays ended and does not resume normal services after load. | Broad Alpha | Continue proving terminal-frame HQ/runtime object behavior. | High |

### Strategic Map, Zones, Sites, And Routes

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Stable zone IDs | All strategic state anchors to durable IDs. | Implemented Foundation | Curated Everon location-plan IDs are upserted on top of the existing extras; continue validating route/site coverage during runtime playtests. | High |
| Zone type model | Towns, outposts, resources, factories, radio towers, airfields, seaports, banks, police, hideouts, and mission sites matter. | Broad Alpha | Figari and Morton remain stock town-center locations. Simon's Wood is normalized in place as a food/resource farm with a small guard footprint and two ambient civilians. Maiden's Bay is now represented once as the Logistics Warehouse resource; the overlapping legacy town, town civilians, influence rows, duplicate aggregate garrison manpower, marker, and anchor retire without fold-back credit. Mutable old-ID references canonicalize, while exact typed evidence stays frozen and resolves through detached historical lookup plus runtime old/canonical equivalence. The deterministic preflight assertion is wired but has not run; continue reviewing the remaining minor named localities. | High |
| Generated mission sites | Mission targets are generated from stable anchors and category/site rules. | Broad Alpha | Replace fallback sites with authored or validated site sets. | High |
| Generated routes | Convoys, QRFs, patrols, roadblocks, and mission movement use route-aware paths. | Broad Alpha / Needs Runtime Proof | All three exact assassination-guard contracts deliberately create no route: officer, traitor, and spec-ops guards remain on station at deterministic offsets from separate HVTs. Pre-opt-in/historical assassination missions retain prior paths. | Highest |
| Physical activation bubble | Near-player zones physicalize and off-screen forces stay abstract unless an active objective requires runtime entities. | Broad Alpha / Sealed Schema-63 hysteresis | An inactive zone enters at its activation radius; an already-active zone stays rendered until every living player crosses the larger deactivation radius. This projection hysteresis is separate from persisted combat heat. Ten explicit family consumers retain their existing fold/reprojection policies; native boundary movement and stutter measurements remain open. | Highest |
| Canonical combat presence | All systems agree on whether a force can exert combat pressure, independent of render state and empty assets. | Implemented in sealed Schema-63 source / Foundation and Workbench validation pass / Needs runtime proof | Fresh registered physical samples count conscious dismounted infantry, operational occupied armed mobile vehicles once per platform, and occupied operational static weapons once per platform. Cargo, empty vehicles, unproven unarmed pilot-only vehicles, destroyed/burning platforms, immobile mobile platforms, terminal/quarantined rows, and stale samples are excluded. An unresolved spawned-group authority gap invalidates the query; capture accepts only living conscious character players and excludes spectator/Game Master proxies. Eligible virtual forces contribute durable living infantry, including exact convoy surviving crew, never abstract vehicles. Shared contribution/authority-gap/radius caches and an indexed sampler registry avoid repeated allocation/scans. Capture, missions, HQ threat, civilian safety, and enemy strategy share the service; prove every classification and consumer in native runtime. | Highest |
| Canonical ownership transition | One server transition updates owner, garrison/security policy, support, facilities/logistics, markers/GM/menu, economy, enemy consequences, events, and persistence. | Implemented in Schema-62 source / Schema-64 political integration sealed / Needs Runtime Proof | All causes converge on one immutable receipt; valid followers remain pristine and execute FIFO. Schema 64 no longer allows town-support callers or legacy projections to publish ownership: strict hysteresis creates pending intent and delegates it to this receipt boundary. Parent publication remains transactional. Prove political retry/resume, exact support consequence callbacks, nested publication, migration, and every caller. | Highest |

### Factions And Relations

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Resistance faction | Players, HQ, arsenal, recruits, and support use the resistance faction consistently. | Implemented Foundation | Continue replacing hardcoded faction checks with preset-driven helpers. | Keep |
| Occupier and invader factions | Enemy factions have separate pools, zones, support, and target behavior. | Partial | Relation helpers now drive enemy targeting/order decisions; continue migrating remaining broad faction checks across civilians, support, and missions. | Highest |
| Relation matrix | Code asks whether two factions are hostile/neutral/allied instead of assuming non-resistance means enemy. | Broad Alpha | Keep relation checks centralized and expand runtime proof wherever a system distinguishes resistance enemies, rival enemies, and neutral actors. | Highest |
| Faction templates | Group pools, vehicles, support capabilities, equipment tiering, and fallback rules are data-driven. | Broad Alpha | Move more template data into configs and validate resources on startup. | High |

### Players, Membership, Squads, And Command

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Membership and commander authority | Durable member/admin identity and explicit commander transfer/election rules gate server actions. | Broad Alpha | SteamID64 membership/admin identity, commander vacancy/handoff, one transfer chooser, and force-myself admin recovery exist. Add election/points/rank policy, reconnect/JIP proof, and complete permission coverage. | High |
| Player squads and high-command orders | Persistent player-owned squads can recruit, receive duties, move, fight, return, and survive materialization/restart without becoming client-owned truth. | Missing | Define squad aggregate, ownership/commander permissions, exact recruitment/loadout cost, operation linkage, marker/JIP projection, and virtual/physical lifecycle. | Highest |

### Economy, War Level, And Strategic Score

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| HR and faction money | Recruitment, support, training, garrisons, HQ movement, and logistics spend durable resources through exact, auditable transactions. | Broad Alpha | Paid training, visible garrison confirmation, paid infantry QRF, and stamped Schema-60 Search-and-Destroy source use the shared ledger. QRF charges $250 plus roster HR; Search-and-Destroy charges $350 plus one HR per frozen member and settles living HR through commander recall. Supply, roadblock, fire, air support, HQ movement, logistics, and remaining legacy mutations still need exact quote/transaction cutovers. | Highest |
| Personal money | Player rewards and personal purchases are distinct from faction money. | Partial | Add clear transfer and reward routing. | Medium |
| Town/resource income | Income scales from town support/population and captured resources. | Broad Alpha | Economy inspection now includes next income plus source-category totals for towns, resources, factories, seaports, airfields, banks, and other owned zones. Town money income is now multiplied by surviving civilian population share, town HR income requires enough surviving population, and Full Campaign Debug proves both the math and report evidence; next make resource-specific effects more visible. | High |
| War level | Strategic control and escalation drive enemy quality, support, missions, detection, and training caps. | Implemented Foundation | Resistance training cap now resolves from war level plus two, maxing at 10, with runtime proof; next tie war level consistently into AI skill/equipment, composition, garage limits, and airfield gates. | Highest |
| Strategic score tuning | Zone values and war thresholds produce a stable campaign pace. | Partial | Choose final score weights and report next meaningful captures. | High |

### Enemy Strategy And Support Pressure

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Enemy resource pools | Enemy attack/support capacity and aggression grow from map control and pressure through one canonical per-enemy authority. | Sealed Schema-67 Source/Workbench Contract | Source implements one income/spend/refund/aggression/live-adjustment API, independent persisted cadence checkpoints, zero-effect operational receipts, and a 4,096-row per-faction hard stop with no operational eviction. Foundation passes at 736 references and final normal/all-five Workbench checks pass at 5,809/11,751 with CRC `a353fa0d`; execute replay/conflict/arithmetic, role isolation, cadence, cap, baseline-only migration, and `-67` quarantine before tuning. | Highest |
| Support spend ledger | Same-zone support stacking, recent damage pressure, spend caps, cooldowns, and refunds are tracked. | Sealed Schema-67 Source Authority / Exact QRF and patrol operation policy retained | Schema 67 links exact defensive-QRF attack/support debits and survivor refunds plus exact patrol proactive debit/admission refund/survivor refund to canonical receipts. Fold/materialization never refunds. Unsupported enemy order families retain legacy order lifecycle, but post-67 resource/aggression mutations still use the canonical receipt API. Source fixtures remain wired/static; packaged accounting and restart proof remain open. | High |
| Enemy commander orders | Counterattacks, rebuilds, roadblocks, support calls, and HQ pressure queue durable orders. | Scoped Schema-70 exact-garrison-rebuild engine proof / prior exact-counterattack and planning seals retained | Periodic planning retains its sealed behavior. R10 passes all five Phase 18 cases, the targeted Phase 22 identity/strategic/RUN paths, and typed cleanup with zero failures/open/runtime claimants. Packaged runtime and earlier planning/restart gates remain open. | Highest |
| Abstract resolution | Off-screen orders and support resolve without needing physical entities. | Scoped exact-garrison-rebuild engine proof plus sealed exact counterattack/QRF/patrol routes | The rebuild slice retains one roster through strategic travel and physical projection/fold, and all five R10 Phase 18 cases pass. Save/restart, package, movement, and soak behavior remain open. | High |
| Physical response | Near-player enemy responses spawn, move, fight, and fold back. | Schema-70 source/engine proof sealed / Needs runtime proof | Exact garrison-rebuild members materialize from the frozen roster, fold confirmed casualties back to strategic authority, and deliver only surviving held slots. Native spawn/movement/contact/fold/re-entry/casualty, delivery, marker, restart, and multiplayer proof remain open. | Highest |

### Civilians, Town Influence, And Population

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Town support ledger | Events change support, reputation, heat, security, population, and enemy aggression with explainable history. | Sealed Schema-64/65 Town Authority / Sealed Schema-67 Enemy-Aggression Consumer / Needs Runtime Proof | `HST_TownInfluenceRecord` remains the sole political support/population truth. Schema 65 preserves exact aggression before/after evidence; Schema 67 requires its matching aggression mutation to enter the canonical per-enemy resource API and receipt history. A canonical-town danger edge appends at most one political event; pending receipts drain before a new edge. Replay is read-only, conflict and stable-ID exhaustion fail before mutation, and restore separates structural validation from live-preset enemy-role validation. Execute source proofs, native callbacks, and real save/restart. | Highest |
| Political town flips | Town ownership changes by support majority with strict hysteresis, not only direct combat. | Sealed Canonical Schema-64 Source/Workbench / Schema-62 owner route | FIA support strictly `>8000` bp requests resistance ownership; strictly `<4000` requests the strongest enemy authority; equality stays neutral. Occupier and invader support remain separate. Pending flips retry only through `HST_OwnershipTransitionService`; exact event replay is idempotent. Runtime-prove both directions, equality, enemy selection, nested publication, and restart. | High |
| Population state | Population remaining/killed affects support, income, victory, and loss. | Broad Alpha / Schema-65 Automatic Casualty Source Complete / Needs Soak | Town income and HR scale/gate from remaining population. An exact tracked civilian death now queues one canonical casualty event that decrements logical population independently of physical actor count; server attribution, real profile restart, and terminal-outcome soak remain open. | Highest |
| Civilian runtime population | True towns have a configurable daytime/low-heat driven-traffic target, default five, when town population and global budgets permit; the target is not a per-town guarantee. Minor localities retain their smaller policy. Logical population remains canonical while disposable physical actors rotate fairly. | Sealed Settings-24 Ambient Foundation / Sealed Schema-65 Source/Workbench Pedestrian Panic / Needs Native Runtime Proof | Global actor/traffic caps use 120-second-or-longer leases, deterministic fair rotation, and at most four root-spawn starts per health update. Pedestrian/traffic admission still requires exact native behavior acknowledgement; immutable slots drive bounded recovery. Player-first observation promotes only an exact player pilot; passenger occupancy protects a transient root without claiming it. Schema 65 adds pedestrian `Wandering -> Panicked -> Recovering`: a danger/casualty threat assigns a running move waypoint away from danger, then restores walking/wander only after acknowledgement. Rebound panic does not spend stuck recovery, while route loss/stall consumes a separate bounded panic-route counter; the hot path does not reactivate AI. Traffic panic is outside this slice. Session topology stays nondurable. Run Campaign Debug, native brief enter/exit, autosave/restart, destruction/reset, two-nearby-same-prefab restoration, and the ten-town/ten-minute churn/stutter soak. | High |
| Civilian political consequences | Casualty, theft, and combat danger change durable logical state once even when no ambient actors are rendered. | Sealed Schema-65 Source/Workbench Complete / Needs Native And Restart Proof | The server callback retains exact tracked civilian deaths for a 256-row casualty queue; theft has a separate 64-row queue. Both share a four-attempt frame cap, retain failed receipts indefinitely with bounded backoff, and defer capture while authority is pending. Exact resistance-pilot use of a civilian vehicle applies theft only after durable promotion, keyed by the durable vehicle ID; passenger-only roots remain non-recyclable until exit or pilot claim but are not theft. Combat requires current-operation or recent-fire facts at the current combat-presence revision; `HOT` alone is inert. A pending receipt drains before the next edge, `episode - lastApplied <= 1`, and one edge applies at most one exact canonical-town event. Current restore requires the canonical live combat fingerprint: heat `+4`, zero support/reputation/population/police/roadblock/aggression deltas, exact source/reason, and unchanged support/population before/after. Minor localities are panic-only with session-only de-duplication. Foundation and final stamped normal/all-five Workbench checks are clean; native callback/waypoint, package, real save/restart, and multiplayer gates remain open. | Highest |
| Police and roadblocks | Security systems create scan pressure and town-state consequences. | Sealed Schema-66 Exact Enemy-Town Patrol / Needs Runtime Proof | Enemy police/roadblock pressure still derives from owner relation, heat, support margin, and war level. Positive police pressure at a canonical enemy town admits one exact 2–5 member patrol epoch; physical casualties persist through fold/restore and complete destruction applies police `-1` once. The same epoch cannot respawn without a later positive police event or newer owner revision. Resistance-owned towns target zero automatic police and roadblocks; the separate at-most-two aggregate resistance infantry policy has no automatic vehicle. Package-test roster behavior, balance, scans, and ownership transitions. | High |

### Undercover And Wanted Enforcement

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Undercover request/apply | Players can request and hold cover only when state and appearance allow it. | Broad Alpha | Live clothing slots, weapon manager state, and carried inventory now feed eligibility; next soak real multiplayer clothing swaps and expand any concealed-weapon nuance if needed. | Highest |
| Vehicle cover and heat | Civilian vehicles provide cover until reported, heated, or compromised. | Broad Alpha | Continue proving report/clear/garage/redeploy handoff in runtime runs. | High |
| Passenger compromise | Reported vehicles and compromised passengers affect player cover. | Broad Alpha | Expand to multi-passenger and roadblock/police scans. | High |
| Off-road and security scans | Driving behavior, visible gear, police, and roadblocks can compromise cover. | Broad Alpha | Police and roadblock scan chance now scales from security presence, town/player heat, war level, enemy aggression, and blocking eligibility evidence, with deterministic chance/roll reasons and Full Campaign Debug scaling proof; next soak real police/roadblock encounters and deepen off-road telemetry. | High |
| Undercover reports | Failure reasons are explicit and actionable. | Implemented Foundation | Keep every new detection rule explainable in one report. | Keep |

### Garrison, Recruitment, Training, And Forces

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Garrison state | Captured zones store abstract defender manpower and vehicles. | Implemented Foundation / Needs Runtime Proof | A new policy-v2 resistance purchase links its immutable manifest to the stable garrison while the held batch's living slots, not `m_iInfantryCount`, own its exact manpower. Historical policy-v1/initial/enemy defenders remain aggregate. Capacity sums legacy infantry, exact living/reserved manifests, and active legacy infantry without double counting. Typed-quarantine projections reconcile mapped deaths, retire unambiguous runtime ownership, terminalize outside normal queue capacity, and remain non-operational so diagnostic survivors cannot exert capture or placement pressure. Vehicle and static-assignment roster authority remains open. | Highest |
| Recruitment and HR costs | Commander spends an exact, quoted amount of HR/money to recruit a committed force manifest. | Implemented Foundation / Needs Runtime Proof | Selection still uses an expiring all-or-nothing quote and confirmation sends only its ID. Policy-v2 now freezes an empty executable root and the arbitrary priced member roster, atomically commits linked money/HR transactions and exact patrol authority, and replays without duplicate charge. Reports, map eligibility, and legacy direct recruitment reserve exact living patrol slots alongside legacy/active infantry; direct recruitment rejects partial capacity and verifies its full delta before charging. Terminal operation outcomes refund zero. Arsenal/equipment consumption and packaged execution remain open. | Highest |
| Training | Training improves resistance AI quality and caps by war level. | Broad Alpha / Needs Runtime Proof | Training now uses the shared request ID, durable command receipt, and reserve/commit resource transaction path. Existing quality/capture behavior remains implemented, but the schema-42 authority path has only static and Workbench validation until an isolated runtime case runs. | Highest |
| Static defenses | Players can assign static weapons or emplacements to garrisons. | Missing | Add durable state, placement, capture, and spawn/fold behavior. | Medium |
| Garrison physicalization | Active zones spawn defenders from garrison and composition services. | Broad Alpha / Exact policy-v2 infantry slice source-complete | Only new policy-v2 purchased resistance garrisons use an exact empty-root/member manifest, held virtual local loop, survivor-only materialization/fold, restore normalization, no-refund terminal settlement, and marker/UI projection. PhysicalWar legacy owners are excluded from those groups. Historical policy-v1, initial-map and enemy aggregate garrisons still use broad composition and count fold; vehicles, multi-root plans, static assignments, and packaged native casualty/fold/save proof remain open. | Highest |
| Exact force spawn queue | Each force projection realizes one immutable executable manifest through bounded, retryable, restore-safe work and verified registration. | Implemented Foundation / R22 In-Process Queue Proof / Needs Packaged Proof | The bounded selector admits ordinary queued work and due deferred/retryable slots only when the owning batch can start another attempt; manifest exactness and dependency registration remain mandatory, and attempt normalization remains post-selection. R22 passes retry generation/backoff, failed-slot-only work, stale-generation rejection, duplicate-safe completion/replay, same-wave sibling progression, interrupted-restore reconciliation, and exact registration. The generic adapter serves the exact counterattack, Schema-66 local security, two QRFs, player Search-and-Destroy, exact enemy/garrison patrols, and all three assassination-guard contracts. Counterattack handoff releases only living frozen slots and folds confirmed casualties back into the same manifest. Schema 52 retains its convoy-specific path. Generic vehicles/assets/multi-root, historical mission/aggregate forces, event-driven casualty subscription, and packaged proof remain open. | Highest |

### Missions And Objectives

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Category mission selection | Commander chooses mission category; mission and valid target are selected server-side. | Broad Alpha | Gun Shop is now a rarer dynamic candidate; continue tuning category candidate rules and player-facing disabled reasons. | High |
| Mission runtime primitives | HVT, destroy, hold/clear, rescue, cargo, convoy, support, and gun-shop primitives have physical action paths. | Broad Alpha / Exact convoy, guards, first rescue, and radio lifecycle implemented in source | Schema 59 gives radio destruction/rebuild one exact physical-evidence path and separate rebuild equipment while preserving the Schema-58 rescue contract. R16 proves both isolated radio primitives through engine physical evidence, normal callbacks, receipts, exact rewards, and one-attempt enforcement with exact cleanup. Packaged authored-content radio behavior and the other runtime families remain open. | Highest |
| Mission persistence | Active missions, objectives, assets, runtime entities, and markers survive restart. | Broad Alpha / Needs Soak | Schema 59 adds no-invention radio migration, stable-site versus unique per-mission runtime identities, immutable authored provenance, typed transition/revision/receipt validation, generated-ONLINE destruction-plus-rebuild provenance, one rebuild-stop attempt per destruction epoch, ownership handoff, explicit borrowed projection-pending state, and linked `-59` quarantine cleanup that preserves coherent historical terminal outcomes. New-campaign reset verifies authored-target healing or rolls back/fails closed. Real restart must cover borrowed ONLINE, DESTROYED damage reapplication, REBUILDING equipment, generated replacement, exact outcomes, marker/UI recovery, and the existing rescue matrix. | Highest |
| Strategic mission effects | Missions affect money, HR, town support, aggression, HQ knowledge, enemy pools, and unlocks. | Broad Alpha | Mission success/failure, mission expiry, convoy outcomes, resistance zone captures, hostile support resolved near HQ, vehicle reports, town influence, radio broadcasts, and security pressure now record durable compact strategic-event rows; next route more mission-family consequences through the same ledger. | High |
| Mission reports | Reports explain active objective, target, assets, runtime phase, and failure blockers. | Implemented Foundation | Missions-tab active rows are compact one-row summaries; keep detailed inspection in mission reports and expand per-family detail as missions get unique content. | Keep |

### Arsenal, Garage, Loadout, And Logistics

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Persistent arsenal | Loot deposits unlock faction equipment and support finite item accounting. | Broad Alpha | Finish AI loadout policy from arsenal unlocks. | High |
| Loadout editor | Client UI edits loadouts, server applies atomically, and finite items are rolled back on failure. | Broad Alpha | Finish rendered UI smoke and long-run inventory soak. | High |
| Vehicle capture and garage | Vehicles can be captured, stored, redeployed, and restored with cargo/heat metadata. | Broad Alpha / Needs Soak | Redeployed/restored and runtime-spawned vehicles now clear engine faction claims recursively; add capacity, class limits, fuel/damage polish, and repeated restart tests. | High |
| Vehicle cargo | Captured/redeployed vehicles retain cargo records and restored contents. | Broad Alpha | Soak with mission cargo, garage handoff, and loot operations. | High |
| Fuel/ammo/repair sources | Vehicle roles affect logistics and field support. | Partial | Add durable fuel and repair logistics rules. | Medium |

### Remaining Parity Systems

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Intelligence, reveal, interrogation, and informants | Intel rewards reveal bounded sites/enemies/communications; prisoners and informants have safe server-owned consequences. | Partial | HQ knowledge, staged markers, rescue captives, and mission reports provide pieces, but no unified intel economy or interrogation/informant lifecycle exists. Define durable intel records, visibility policy, enabled mission dependencies, and JIP/save behavior. | High |
| Respawn, incapacitation, and revive | Resistance spawn availability, injury/revive, penalties, and reconnect behavior are explicit and server-authoritative. | Partial | Custom HQ spawn and lifecycle callbacks exist. Define incapacitation/revive policy, equipment/death penalties, unavailable-HQ behavior, and dedicated-server/JIP tests. | High |
| Fast travel | Travel is available only under safe control/combat/operation restrictions and cannot duplicate or reset forces. | Missing | Specify whether the first public alpha includes it. If included, add server validation, materialization guards, cost/cooldown, destination policy, and save/JIP proof; otherwise record an intentional omission. | Medium |
| Construction, fortifications, and observation posts | Placement has exact cost, ownership, persistence, deletion/refund, marker, town-support, and AI/static-assignment rules. | Partial | Build/garage and captured-emplacement scaffolding exist, while static defenses and strategic construction policy remain incomplete. | High |
| Radio and intelligence network | Radio sites affect communications, intercepted support, town influence, enemy knowledge, and player intel. | Partial / Exact lifecycle implemented in source | Schema 59 makes town broadcast depend only on one resolved ONLINE site and gives destroy/rebuild durable authority. Native binding/restart proof remains open; capability gates, interception rewards, and broader intelligence effects remain later work. | Medium |
| Medical and logistical recovery | Wounded forces, vehicles, fuel, ammo, repair, rearm, salvage, and strategic resources settle without duplication. | Partial | Vehicle source classification, cargo, garage, ammo points, and support scaffolding exist. Add durable recovery/salvage transactions and exact force/vehicle lifecycle integration. | High |

### UI, Markers, Reports, And Debug Suite

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Command menu | One in-game menu exposes setup, missions, forces, map/war, arsenal, garage, members, and admin controls. | Broad Alpha / Needs Runtime Proof | Packaged schema 49 proved stock HUD/Game Master recovery but not later source. Schema 60 exposes Search-and-Destroy as an exact server quote/confirm flow with frozen roster, $350, exact HR, and ETA while preserving legacy pre-60 requests. Republish and prove HUD/Game Master preservation, quote/action visibility, duplicate replay, pointer layering, and supported resolutions. | Highest |
| Map markers | HQ, zones, missions, support, QRFs, orders, garrison patrols, and active deliveries publish linked markers with cleanup proof. | Broad Alpha / Protocol-2 Projection / Schema-66 Integrity Repair / Needs Runtime Proof | Logical campaign markers stream to a widget-independent client registry and reconcile as client-local native markers; server-native campaign publication is retired. The `27672e6` local-owner regression is repaired by protected system-owned/non-removable insertion plus keepalive integrity checks and registry-derived self-heal. Player-created/dynamic markers remain editable on their separate path. Republish and prove manual delete/move/edit behavior, self-heal, host/client atomicity, cleanup, map reopen, reconnect/JIP, and existing POW presentation. | Highest |
| Authoritative client projection and JIP | Host, clients, reconnects, and late joiners converge on the same snapshot watermark and ordered revisioned create/update/delete stream. | Implemented Foundation for marker records / Needs Runtime Proof | Schema 61 supplies delivery/ACK; Schema 62 adds source revision; Schema 66 protects native campaign-marker ownership without changing the wire protocol. Prove host/two-client/late-join equality, ownership revision correlation, immutable/self-healing campaign markers, editable player markers, native rendering, and restart. Menu snapshots, campaign tasks, general notifications, and dynamic player markers remain outside this protocol. | Highest |
| Modal map targeting | Target selection owns map/input/cursor/modal state through one idempotent state machine. | Broad Alpha / Needs Runtime Proof | Normal map targeting and confirmation flows exist with ESC handling and duplicate-click guards. Prove Closed -> Selecting -> Confirming -> Submitting/Closing behavior, Choose Again re-arm after pointer release, cursor/modal layering, and atomic ESC teardown at supported resolutions. | Highest |
| Map/War information model | Players see contacted town pressure and resistance territory without redundant or misleading rows. | Sealed in Schema-64 Source/Workbench / Needs Runtime Verification | Zone Pressure contains only explicitly contacted valid canonical towns; the player's current contacted town sorts first, then remaining towns by ascending FIA basis points and stable name/ID ties. Resistance Territory includes every published resistance-owned strategic zone except mission bookkeeping, ordered deterministically by type/name/ID with no arbitrary six-row cap. Current ownership receipt authority is respected. Prove rendered output, paging/scale, restart, reconnect, and JIP. | High |
| Full Campaign Debug | One button runs a true runtime certification suite and writes structured artifacts without changing the campaign under test. | Broad Alpha / R23 diagnostic / Not certified | R23 completed 688 cases at 564 PASS/51 WARN/66 FAIL/7 BLOCKED and proved 5,440/5,650 required assertions, with 192 failed and 18 blocked. The six quiet generic destroy-target assertions, all seven destroy-family start/runtime/primitive cases, SpawnQueue, Foundation/authority foundation, all eight local-security assertions, exact seeded persistence counts, and exact-zero final restore pass. One tracked exact-QRF settlement failure leaves one tracked row open and two exact runtime claimants, directly failing typed cleanup; the leak snapshot later records two total open orders. Phase-persistence warnings and 12 Phase-24 failures occur afterward while the remnant remains, without proving one cause for every downstream case. R21 remains the cleaner cleanup/persistence comparison. World-scope/external restart, package, network, and soak gates remain. | Highest |
| Scoped debug profiles | Smaller profiles isolate feature families for fast iteration. | Implemented Foundation | Keep profiles explicit and never treat external/restart/soak gaps as PASS. | Keep |
| Build provenance | Runtime logs and artifacts identify the exact code build from one authoritative source. | Implemented Foundation / Needs Packaged Proof | Runtime, menu, admin, and debug artifact summaries now consume `HST_BuildInfo`; prove the stamped identity in a packaged dedicated-server/client run. | High |

### Campaign End And Long-Run Soak

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Victory | Default victory depends on population support and decisive airfield control. | Broad Alpha / Needs Soak | Soak population/airfield outcomes across save/load and tune support thresholds. | Highest |
| Loss | Default loss depends on civilian catastrophe, with optional collapse settings. | Broad Alpha / Needs Soak | Soak killed-population outcomes across save/load and mission/civilian event paths. | Highest |
| Multiplayer soak | Campaign survives co-op, reconnect, restart, active missions, active support, and terminal saves. | Needs Soak | Build repeatable 2/4/8+ player test profiles. | Highest |
| Performance soak | Physical/abstract transitions do not leave stuck groups, duplicate vehicles, missing markers, or periodic frame stalls. | Needs Soak / Sealed Schema-64 Repair Plus Sealed Schema-65 Consequences And Schema-66 Local Security | The latest user run showed a visible once-per-second stutter. Schema 64 keeps influence aggregates incremental and throttles active-group logs; Schema 65 adds bounded queues/state iteration and consumes indexed combat facts rather than a hot-path world scan. None is runtime profiling evidence. Profile the exact sealed Schema-66 package identity for the Phase 8 exit and measure freezes per minute before calling the stutter fixed. The later sealed Schema-67 source identity does not replace that run. | Highest |

## Highest-Impact Next Tasks

1. Use R23 as the latest targeted runtime evidence and R21 as the cleaner
   cleanup/persistence comparison; R10 remains the last positive baseline for
   the earlier targeted boundaries. Preserve R22's passing retry generation/
   backoff, failed-slot-only scheduling, stale-generation rejection, duplicate-
   safe completion/replay, same-wave progression, interrupted restore, exact
   seeded persistence counts, exact-zero final restore, Foundation/authority
   foundation, and eight local-security assertions. Preserve R21's exact
   persistence report and zero-to-zero enemy-order cleanup. Preserve the fresh
   Workbench result and R23's six quiet generic destroy-target assertions plus
   seven passing destroy-family start/runtime/primitive cases. Then repair the
   exact-QRF settlement crash-window defect, followed by
   proof-fixture dependency/radio ordering. Close
   world-scope restart, Phase-20 town behavior/authority, and wider runtime
   failures. Preserve R16's passing isolated radio destroy/rebuild callbacks,
   receipts, epoch, rewards, one-attempt rule, and cleanup alongside capacity-bounded
   admission, one prepaid support debit, frozen reciprocal authority,
   virtual/physical casualty continuity, delivered held-roster transfer without
   aggregate double count, ownership terminal handling, admission rollback,
   proportional prearrival refund, PREPARED/SETTLED crash resume, contract-zero
   isolation, selected ownership ABA rejection, and idempotent `-70` malformed/
   orphan quarantine with retention. Then package-prove serialization/restart,
   dedicated-server, networking/JIP/reconnect, and soak behavior. Execute the
   still-open Schema-69 counterattack and Schema-68 planning runtime gates in the
   same controlled verification program.
2. Runtime-prove the Schema-62 ownership boundary: every cause route, one
   revision increment, identical replay no-op, conflict/stale rejection,
   array-ordered queued intent, setup/terminal pre-owner retry, exact-patrol
   settlement, post-liberation security, staged rollback/two-child publication,
   resolver-first UI and unsafe-marker purge, setup history through activation,
   exact support/derived correlations, receipt-scoped strategic events,
   persistence re-arm, marker source revision, two-restart exact-once political
   completion, invalid-owner migration, quarantine, and retention. In the same
   isolated boundary, prove Schema-63 conscious/crew-aware sampling, empty and
   cargo-only vehicle exclusion, shared capture/mission/HQ/civilian/enemy
   consumers, 30-second `HOT -> COOLING -> COLD`, conservative restore, and
   activation/deactivation hysteresis. Also prove Schema-64 formula goldens,
   strict thresholds, all-cause ownership delegation, pre-64 migration/current
   quarantine, distinct enemy support, contacted Zone Pressure, complete
   Resistance Territory, and expiry-only event rescans.
3. Runtime-prove civilian projection across at least ten simultaneously eligible
   towns for ten minutes, including Figari, Morton, and Simon's Wood. Verify the
   configurable daytime/low-heat traffic target, default five per true town, is
   budget-limited and fairly rotated, the minor locality remains at two
   pedestrians, appearances remain unique after replacement, traffic stays silent
   and moving, static
   military ambience refreshes after owner/policy changes, and teardown/recycle
   does not churn. Require Maiden's Bay to appear only as the Logistics Warehouse
   and compare freezes-per-minute against the observed once-per-second stutter.
   Prove the per-frame brief enter/exit observation and fail-closed barrier on
   every `HST_PersistenceService` capture/checkpoint path, plus new-campaign reset
   reconciliation, through native autosave/restart and promoted-root destruction.
   Include exact promotion, two-nearby-same-prefab no-collapse restoration, reset
   retention/normalization, and no-duplication evidence. Exercise the Schema-65
   server casualty callback and bounded queue, exact attribution/fallback,
   civilian theft only after durable promotion, and current-fact combat danger.
   Require `HOT` alone to remain inert, at most one canonical-town political
   mutation per danger episode,
   logical population/support/reputation/heat/aggression continuity without
   rendered actors, and complete pedestrian `Wandering -> Panicked -> Recovering`
   behavior. Save/restart must preserve the canonical episode envelope while
   acknowledging that minor-locality panic receipt fingerprints are session-
   only and cannot create political history. In the same run, prove one exact
   enemy-town local-security roster, native cyclic movement, casualty-preserving
   fold/re-entry, save/restart with no refill, once-only police loss on
   destruction, no same-epoch resurrection, rearm only after a newer owner
   revision or later positive police event, no loss on owner/pressure clear or
   campaign stop, and zero automatic resistance police/roadblocks.
4. Runtime-prove the Schema-60 exact player Search-and-Destroy operation: one
   immutable infantry-only quote/manifest, $350 and exact-slot HR accounting,
   direct virtual travel, proximity materialization/fold, mapped casualties,
   off-screen combat, return-to-assignment after a displaced fold, on-station
   hold after hostile clear, commander recall, living-HR settlement, archive
   replay, safe paired capacity eviction, corrupt quarantine retention, contract-
   0 legacy isolation, `-60` quarantine, and save/restart.
   Include a live adapter-observed casualty followed by cardinality-checked
   fold/root retirement and a separate physical recall exit; synthetic queue-
   slot proof does not close either runtime gate.
5. Runtime-prove the schema-50 exact paid infantry-QRF operation through ten
   off-bubble minutes, materialize/fold hysteresis, exact casualties, on-station
   virtual combat, recall, pre/post-commit failure settlement, and save/restart at
   every projection state. Do not promote source assertions to runtime proof.
6. Runtime-prove the schema-51 infantry-only enemy defensive QRF: one prepaid
   frozen roster from a distinct source, no parallel legacy response, strategic
   outbound/return travel, materialization/fold hysteresis, exact casualties,
   two-sample physical arrival, once-only defensive pressure, proportional
   survivor refund, marker movement/cleanup, and save/restart replay. Do not
   widen the claim to counterattacks, vehicles, patrols, or convoys.
7. Runtime-prove the schema-52 exact mission convoy: one frozen route, exactly
   three vehicle/crew elements, optional cargo/captive on vehicle zero, virtual
   travel without timer arrival, 3/3 physical drivers, interception, contact-
   to-clear transition, exact non-suffix casualty identity, partial crewless-
   vehicle and cargo-only ground recovery, player-bound cargo fold, duplicate-
   free capture-to-garage handoff, casualty-preserving fold/rematerialization,
   two-sample arrival, once-only outcome/settlement, aggregate current/
   destination marker cleanup, and save/restart. Historical
   restored convoys must remain contract version `0`.
8. Runtime-prove the schema-53 exact enemy patrol: one proactive debit, one
   frozen infantry root, outbound virtual movement, physical materialization,
   mapped casualty fold/reprojection, contact-held progress, one closed route
   lap, return, survivor refund, marker cleanup, and save/restart. Historical
   patrols must remain contract version `0`, and corrupt current rows must remain
   quarantined without legacy fallback.
9. Runtime-prove the schema-54 exact purchased-garrison patrol: only a new
   policy-v2 resistance purchase gets one exact empty root and arbitrary member
   roster, a held virtual infinite local loop, survivor-only materialization/
   fold, owner-change/all-dead/campaign-stop/setup and typed spawn/route-failure
   no-refund settlement, marker/UI cleanup, and save/restart/JIP continuity.
   Historical policy-v1, initial/enemy
    aggregate, vehicle, and multi-root garrisons must remain legacy.
10. Runtime-prove the schema-55 exact officer-mission guard: only a newly started
   `assassinate_officer` mission gets one route-less empty root and ordered guard
   roster. Prove survivor-only materialization/fold, all-guards-dead/HVT-active
   independence, all typed zero-refund outcomes, compact settlement, `-55`
   quarantine without fallback or HVT failure, existing-HVT marker/UI status,
   native adapter/casualties, save/restart, owner-change, campaign setup,
   networking, reconnect, and JIP. Historical officer missions stay contract `0`;
   the separate Schema-56 traitor policy must not alter officer contract `1`.
11. Runtime-prove the schema-56 exact traitor-mission guard: only a newly started
   `assassinate_traitor` mission gets contract `2` and policy
   `exact_assassinate_traitor_guard_v1`. Prove the same route-less roster/HVT
   separation, survivor lifecycle, typed zero-refund outcomes, compact restore,
   existing-HVT status, and `-56` quarantine while preserving officer contract
   `1`. Cover native entities/adapter casualties, real save/restart, rendered UI,
    owner-change, setup, networking, reconnect, and JIP. Pre-56 traitor, spec-ops,
   and every unsupported family must remain contract `0` within the Schema-56
   historical boundary; the separate Schema-57 path must not alter contract `2`.
12. Runtime-prove the schema-57 exact spec-ops-mission guard: only a newly
    started `assassinate_specops` mission gets contract `3`, policy
    `exact_assassinate_specops_guard_v1`, intent `assassinate_specops_guard`, and
    `-57` quarantine. Prove route-less survivor materialization/fold/re-entry,
    HVT separation, no virtual guard combat, typed zero-refund outcomes, compact
    restore, existing-HVT status, native entities/adapter casualties, actual
    save/restart, rendered UI, owner change, setup, networking, reconnect, and
    JIP. Officer `1` and traitor `2` must remain exact; historical/pre-57
    spec-ops and ordinary `mission_group_*` rows must remain contract `0`.
13. Execute the isolated campaign-debug suite in `HST_Dev`, prove completion,
    cancellation, interrupted-run recovery, and unchanged live persistence, then
    replace the historical contaminated artifact.
14. Runtime-prove Schema 61 plus the Schema-66 marker-integrity repair with one
    host, two clients, reconnect, and late join.
    Require equal epoch/watermark/registry hash after initial snapshot, ordered
    create/update/delete, a forced dropped-delta resync, map close/reopen, native
    marker cleanup, and real save/restart. Attempt to delete, move, and edit each
    protected campaign-marker family and require system ownership/non-removability
    or bounded registry-derived self-heal. Confirm player-created markers remain
    editable, there is no duplicate server-native campaign set, and dynamic
    player markers stay outside the campaign stream claim.
15. Schema 58 completes the first separately versioned rescue vertical for
    newly started `rescue_pows`. Runtime-prove its three POW projections,
    vehicle seats, fold/re-entry, death/success/grace outcomes, restart,
    rendered UI, reconnect, and JIP before opting in another rescue family.
16. R16 closes the disposable Full Campaign Debug radio chain: fixture-only
    selection, physical transmitter destruction, the normal destroyed callback,
    destruction receipt, stop-rebuild admission, generated-equipment explosive
    evidence, the rebuild-attempt receipt, unchanged epoch, second-attempt
    rejection, exact rewards, and zero fixture residue in both world and
    restored state all pass. Next runtime-prove Schema 59 radio-site authority
    against packaged authored content: one authored binding with no duplicate,
    physical destroy success, influence removal before income,
    destroyed-state restart/streaming reapplication, stop-rebuild equipment,
    exactly one stop attempt per tower-destruction epoch, no epoch advance when
    that equipment is destroyed, ownership-specific evidence and durable dedupe checks,
    0.75-meter authored identity versus 12-meter safe-ground projection tolerance,
    checked physical writes, borrowed projection-pending recovery, linked quarantine cleanup,
    unique per-mission runtime IDs, success/failure/expiry, generated replacement
    whose unbound witness scan stays dormant, new-campaign authored-target restoration,
    markers/UI, reconnect, and JIP. Keep broader radio intelligence outside this
    proof. The projection-seam source harness is not packaged evidence.
17. Run repeated packaged multiplayer restart/performance soak, then tune economy,
    war level, aggression, support pressure, mission pacing, and simulation speed.

## Definition Of Done For The Final Campaign Loop

The campaign loop is feature-complete when this works without admin/debug
intervention:

```text
1. Player joins and spawns at resistance HQ.
2. Commander chooses and maintains HQ and Petros.
3. Players loot equipment and deposit it into the persistent arsenal.
4. Arsenal unlocks improve resistance equipment.
5. Players use civilian cover and unreported vehicles to scout and ambush.
6. Police, roadblocks, wanted heat, and vehicle heat can compromise them.
7. Players complete aid, rescue, sabotage, convoy, and capture missions.
8. Missions mutate money, HR, town support, aggression, HQ knowledge, and enemy resources.
9. Towns flip politically through support.
10. Captured strategic zones generate income and war-level pressure.
11. Players recruit, train, and garrison captured zones.
12. Enemy resources tick, target choices score threats, and responses launch.
13. Enemy responses physicalize near players and resolve abstractly off-screen.
14. Survivors fold back, resources refund, garrisons update, and reports explain outcomes.
15. The campaign survives save/restart through every stage.
16. Victory occurs through population support and decisive military control.
17. Loss occurs through civilian catastrophe or optional collapse settings.
```
