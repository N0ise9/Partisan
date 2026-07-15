# Campaign Save Migrations

Current build identity: source `4757bc86ffbc7a5fa08e64a9abf7ef74ddc1c003`,
UTC `2026-07-15T05:21:40Z`, label
`schema70-settings24-native-counterattack-projection`, stamped by `a22e6af`.

## Current Schema

`HST_CampaignState.SCHEMA_VERSION` is `70` and
`HST_RuntimeSettings.SCHEMA_VERSION` remains `24`. The current stamped checkpoint
adds no save-schema, settings-schema, serialized field, persisted enum
ordinal, operation contract version, or migration rule. Its real-frame Phase-17
exact-counterattack projection proof and owner-aware Phase-24 runtime telemetry
operate only on the existing Schema-69/70 authority graph.

Current source is implementation
`4757bc86ffbc7a5fa08e64a9abf7ef74ddc1c003`, UTC
`2026-07-15T05:21:40Z`, label
`schema70-settings24-native-counterattack-projection`, with stamp commit
`a22e6af`. The immediately preceding exact defensive-QRF checkpoint is
`25b2dc361bc935aea904e08a665755840389c6e0`, UTC
`2026-07-15T02:08:19Z`, label
`schema70-settings24-exact-qrf-external-restart`, with stamp commit `ce2542b`.
It also changes no persisted contract.

Fresh exact defensive-QRF settlement persists `PREPARED` terminal intent, stages
the complete deterministic order/refund tuple with
`m_bResourceSettlementApplied = false`, validates the original debit, reciprocal
claimants, and durable survivor authority, applies or replays the canonical
refund, publishes the applied receipt last, and then finalizes the operation and
order tail. Its QRF-specific validator continues to accept exact dual-pool and
support-only debit/refund graphs. Complete current `PREPARED` graphs can resume;
arbitrary partial rows remain fail-closed.

The sealed checkpoint adds a guarded external-process proof without changing
that persisted shape. For each committed dual-pool interruption cut--before the
refund, after the refund before its receipt, and after the receipt before the
terminal tail--a fresh engine process writes or reads the existing canonical
profile-fallback JSON. The first recovery reconcile changes the exact state once;
the next startup and an explicit same-state reconcile are no-ops. All nine
prepare/recover/replay stages passed with exact build/schema/cut identity,
canonical readback, terminal fingerprints, exit code zero, and guarded cleanup.
The harness adopts the validated canonical readback before the startup QRF
reconciler; it does not certify native persistence-source selection.

That sealed tree passes Foundation at 806 references. Stamped PC Workbench
validation compiles 5,830 Game files/11,820 classes with 46,915K static storage
and CRC `ff59593b`,
reports `Script validation successful`, contains no HST/script compile error,
and leaves zero processes. The focused `HST_TEST_EnemyQRFAuthority` run records
one testcase, zero failures, an empty failed list, and `AllExact=1`; the known
recoverable stock VM diagnostic precedes successful HST completion.

The current stamped tree passes Foundation at 808 references. Its guarded
Workbench Game compile/create loads 5,830 files and 11,822 classes with 47,019K
static storage at CRC `2526746c`, creates the Game, and records zero SCRIPT, HST,
ENGINE, INIT, crash, or unhandled-error signals. The log does not emit the
literal `Script validation successful`, so this is clean compile/create evidence
rather than explicit Script validation. The guarded session, temporary parent,
and Workbench processes were all removed.

R27 `seed1985_t0_p1_u1784093667` ran `full_certification` through CLI autostart
across 688 cases at 577 PASS/49 WARN/55 FAIL/6 BLOCKED/1 SKIPPED and proved
5,500/5,663 required assertions, with 145 failed and 18 blocked. Each of the six
Phase-17 native-projection assertions and both Phase-24 owner-authority
assertions passes exactly once, and all 29 tracked state-diff lines are zero.
The isolated run seeded only Schema-24 settings and no campaign save, then
removed its profile, process, log, and temporary session artifacts exactly. This
runtime result validates the scoped existing-contract behavior; it does not
introduce a migration rule or certify the unrelated remaining suite.

Historical R26 `seed1985_t0_p1_u1784074264` executes 688 cases at 577 PASS/51 WARN/54
FAIL/6 BLOCKED and proves 5,504/5,667 required assertions, with 145 failed and 18
blocked. Both integrated QRF assertions pass. Typed cleanup is settled
0/failures 0/open 0/runtime 0, the open-order leak is 0 -> 0, seeded in-memory
restore counts match, and the final tracked-state diff is zero. The crash-cut
fixtures remain in-process save-data capture/restore evidence. The separate
external matrix closes only the committed dual-pool exact defensive-QRF
canonical-fallback cuts. Full campaign/native-source, world, package,
multiplayer, migration, JIP/reconnect, and soak restart certification remains
open.

Schema 70 originally introduced the conservative persistence boundary for exact
enemy garrison rebuilds while retaining Schema 69 exact enemy counterattacks
unchanged. That dated scoped checkpoint remains sealed at implementation
`2f71236bfc02329a3c8000b104f1b7b1043dc99c`, UTC
`2026-07-13T22:20:52Z`, label
`schema70-settings24-exact-enemy-garrison-rebuild-engine-proof`, and stamp
`ef95555`. Its Workbench/focused evidence remains historical and does not replace
the current checkpoint above.

The preceding active-demolition-witness checkpoint is implementation
`0e54f6cbc7f7084e5534fc603b491cba0d91b653`, label
`schema70-settings24-active-demolition-witness`. It admits only structurally
active, unparented projectile/blast witnesses, rejects parent-slot inventory,
canonicalizes entity-backed source keys, retains at most 64 accepted keys for
the component lifetime, and records a key only after authoritative asset
mutation. It also adds the pre-action
`primitive.destroy.no_ambient_witness_score` assertion. Foundation passes at
793 script-symbol references. Headless Workbench PC Game validation log
`logs_2026-07-14_14-41-29` compiles 5,826 Game files/11,807 classes with 46,643K
static storage at CRC `c3ab042e`, reports `Script validation successful`,
contains no HST compile error or fatal diagnostic, and leaves zero processes
after closure. R23 `seed1985_t0_p1_u1784054690` proves the pre-action zero state
in all six generic destroy-target primitives and passes those six plus the exact
radio-tower destroy primitive through their runtime paths. The preceding queue
checkpoint
`0b380f00fde65c4f2e22858faf8ddc6eab794131`, label
`schema70-settings24-spawn-queue-resume`, makes deferred/retryable work visible
to bounded selection before post-selection attempt normalization and is runtime-
proven by R22. The earlier cleanup checkpoint
`3ded248a4ded084dfb0e3aa8e54ae0a47d36cd5f` registers incidental orders created
by direct Campaign Debug commander ticks with the existing identity-safe cleanup
owner and compares open enemy-order counts at both run boundaries. The earlier
source checkpoint
`2508a735863c153f95bae94adb13f3037b4cdeef`, UTC
`2026-07-14T16:57:02Z`, label
`schema70-settings24-debug-checkpoint-evidence`, distinguishes isolated capture
evidence from a production checkpoint request. None of these corrections changes
a save-schema version, settings version, serialized field, contract version, or
migration rule. R21 runtime-proves the two debug corrections, R22 runtime-proves
the queue correction, and R23 runtime-proves the demolition-witness correction.
The demolition receipt set is component-lifetime runtime state, not a new
serialized migration authority.

Valid current-schema `DEFERRED` and `FAILED_RETRYABLE` spawn-queue rows require
no data rewrite: the corrected selector can resume them automatically while
preserving manifest/dependency exactness and already registered siblings. The
pre-Schema-44 rule remains unchanged: old nonterminal queue rows without the
required current authority still fail closed rather than being promoted.

The current persistence-roundtrip correction is stamped at implementation
`89b7754bcd9ac7e8c41f2a8d7604784b5c1c1c83`, UTC
`2026-07-14T16:01:36Z`, label
`schema70-settings24-current-support-roundtrip`. It changes no save-schema or
runtime-settings version, serialized field, or contract version. It does correct
the legacy civilian-support migration gate to its documented pre-Schema-22
scope: when no canonical town-influence authority exists, compatibility
backfill for both FIA and occupier support now runs only when
`restoredSchemaVersion < 22`. A zero saved by Schema 22 or later is valid
persisted authority and is not evidence of a missing value.

R17 proved the preceding generic-fixture correction with 11/11 active smoke
missions after restore. R18 then isolated the remaining summary delta to
`civilian_occupier_support`, live `2514` versus restored `2614`: a current-schema
non-town zero had been mistaken for absent legacy data and backfilled to `100`.
R19 `seed1985_t0_p1_u1784044976` first proved the corrected in-process summary,
report, and smoke counts exactly. R23 `seed1985_t0_p1_u1784054690` and dated
R25b `seed1985_t0_p1_u1784063032` independently preserved that seeded
roundtrip, including civilian occupier support 2514/2514. Historical R26
`seed1985_t0_p1_u1784074264` reconfirms missions 11/11, assets 22/22, runtime
entities 21/21, groups 9/9, runtime vehicles 10/10, and field vehicles 1/1.
`persistence.real_restart` remains intentionally BLOCKED as the general
campaign assertion because these R26 results are in-process capture/restore and
the external exact-QRF matrix is deliberately narrower. The preceding queue
build's stamped Workbench log
`logs_2026-07-14_13-40-55` compiles 5,826 Game files/11,807 classes with 46,641K
static storage at CRC `be31cb18`, completes clean create/destroy with zero HST
script errors or fatal diagnostics, and leaves zero processes. Later phase
persistence smoke/report cases WARN after subsequent drift; the earlier seeded
capture/restore remains exact.

R12 through R15 isolated four runtime/prefab boundaries in sequence: concrete
component discovery, the inherited component's enabled state, the engine
destruction action required for a normal callback, and the paired inherited
`RplComponent` required by enabled rebuild-equipment destruction. Checkpoint
`a81d494` enables that existing replication component under its authored ID.
These are component lookup, prefab resource, and engine-action corrections only;
they add no serialized field, contract version, save-schema revision, or
migration rule.

A later Workbench native heap crash was traced to local-variable pressure in one
large campaign-debug method, not to Schema-70 deserialization. Moving its
debug-only state into a context object and narrow helpers restored the native
compiler boundary. The crash-fix tree, including the later marker and
cleanup refinements, passes Foundation at 793 references, compiles and completes
Workbench create/destroy at 5,826 Game files/11,807 classes with CRC `287d01ec`,
and remained alive at the 8-, 16-, and 24-second cold-open checks before
deliberate shutdown. No migration rule is inferred from that toolchain boundary.

No schema field is added by the post-seal campaign-debug isolation hardening.
Historical contract-zero debug orders remain contract zero and are never
upgraded to an exact graph. When an isolated fixture must stop one, the legacy
commander/PhysicalWar owner requires its original debit and writes one
deterministic, shape-validated refund before terminal state; exact versioned
orders remain stable-ID authorities and settle only through their typed
operation services. Any missing or conflicting authority blocks the fixture
instead of inventing migration data or rewriting ownership/resource pools.
Disposable cleanup is sequenced after that typed settlement and cannot use an
ID prefix as a substitute for authority release.

Clock capture/restore, future-time normalization, the state-isolation clone,
ambient commander cadence holding, local-security producer holding, exact
checkpoint-evidence classification, identity-safe debug-order tracking, and
exact operation-marker backing are all debug/runtime mechanics. They add no
serialized fields and require no save migration. The cloned clock is restored
to the captured campaign second, any synthetic future timestamps are normalized
inside the disposable state, and explicit fixture calls still exercise the
production enemy-commander tick. While the isolated force-spawn worker is held,
the ambient local-security producer is held with it so durable patrol authority
cannot stop in a worker-only `MATERIALIZING` state; production cadence and the
detached proof fixture are unchanged. Exact operation markers are considered
backed only when category, reciprocal operation link, canonical marker ID, and
operation-family policy agree through the marker publisher's authoritative
predicate; prefix membership carries no migration meaning.

R23 `seed1985_t0_p1_u1784054690` remains dated diagnostic history. It first
proved the demolition-witness correction: all six generic destroy-target
primitives retained zero damage, hits, source, evidence, and destroyed state
before explicit damage, and those six plus the exact radio-tower primitive
passed. It also exposed the exact-QRF refund-ordering defect and retained one
open tracked row, two runtime claimants, and a later two-order leak. It ran build
`0e54f6c` at 564 PASS/51 WARN/66 FAIL/7 BLOCKED and proved 5,440/5,650 required
assertions, with 192 failed and 18 blocked.

R24 `seed1985_t0_p1_u1784059798` ran bridge build `6303e58` across 688 cases at
578 PASS/50 WARN/53 FAIL/7 BLOCKED and proved 5,522/5,682 required assertions,
with 142 failed and 18 blocked. It cleared the R23 contamination: typed cleanup
reported zero settlement failures, zero open orders, and zero runtime claimants;
the order leak remained 0 -> 0; core Phase-24 seed/report/outcome cases passed;
and final tracked-state restoration was exact zero. The remaining focused QRF
failures isolated a one-pool validator mismatch and stale proof classification,
not a save migration.

R25b `seed1985_t0_p1_u1784063032` remains the dated refund-authority proof.
Historical R26 `seed1985_t0_p1_u1784074264` ran implementation `78db295a` across
688 cases at 577 PASS/51 WARN/54 FAIL/6 BLOCKED and proved 5,504/5,667 required
assertions, with 145 failed and 18 blocked. Both `enemy_qrf.settlement` and
`enemy_qrf.persistence` pass, including the complete committed and uncommitted
`PREPARED` crash-cut matrix. Typed cleanup reports settled 0, failures 0, open
0, and runtime 0; the order leak remains 0 -> 0. Current-schema seeded
persistence remains exact at 11/11 missions, 22/22 assets, 21/21 runtime
entities, 9/9 groups, 10/10 runtime vehicles, and 1/1 field vehicle, and the
final tracked-state diff is zero. Escalation physicalization remains WARN. The
wider run remains uncertified because this is in-process capture/restore and
unrelated failures plus real external restart, world-scope, manual,
packaged/native, dedicated-server, network/JIP/reconnect, and soak gaps remain.
No migration rule is inferred from any of these runtime results.
The preceding R10 remains the last positive proof that all five Phase 18 cases
passed; its Phase 20
clock/fingerprint isolation passed with one town behavior/authority case still
failed; Phase 22 completed at four PASS/three WARN/zero FAIL; and Phase 24
completed at 11 PASS/one WARN/zero FAIL. Typed order cleanup left zero
settlement failures, open tracked orders, or runtime claimants. R17 proved the
generic fixture correction at 11/11, and R19/R21/R22 prove the later Schema-22
zero-value correction without weakening exact radio or canonical town
authority.
Packaged/native, dedicated-server, serialization/restart,
network/JIP/reconnect, and soak proof remain open. Schema 69 is historical and
no longer the current save contract.

An earlier Schema-68 commitment-aware checkpoint is sealed at implementation
`695caf46ce6b4146e5407711b76d5e0c578d7392`, UTC
`2026-07-13T14:44:37Z`, label
`schema68-settings24-commitment-aware-enemy-planning`, Foundation 751, and final
stamped-tree all-target Workbench CRC `e483e71c`. The earlier schema-neutral
profile-tree movement and known Schema-68 bootstrap correction are sealed at implementation
`fdf262637e74a70c12454f6c1d3789c2cd0a0f05`, UTC
`2026-07-13T13:19:22Z`, label
`schema68-settings24-bootstrap-profile-marker-hardening`, Foundation 751, and
final stamped-tree all-target Workbench CRC `0544aa1d`.

The latest packaged `f97b12e` test created canonical `$profile:Partisan` settings
and campaign data but contained no retired profile tree, so recursive migration
was not exercised. It also wrote a fresh save whose configured strategic pools
and planner rows were quarantined at `-67`/`-68`; 598 unchanged warnings followed
at roughly one-second cadence. The sealed bootstrap correction routes startup,
admin reset, and fresh-state proof through one production bootstrap factory.
Signature-gated recovery requires the exact nonempty preset identity, exactly
three non-null role
pools, exactly two poisoned enemy planner rows, and empty strategic-mutation and
enemy-order arrays; resource, topology, preset, null-row, legacy-order,
versioned-order, and every other near miss stay fail-closed and unchanged.
Unchanged warnings use 300-second reminders, and live
Campaign Debug observes authority through the read-only production exact
resolvers. Its Campaign Debug, package execution, packaged restart, actual
migration, multiplayer, and soak proof remain open.

The historical Schema-68 sealed checkpoint added a schema-neutral,
commitment-aware planning correction.
Queued/active same-faction orders and support requests and open same-faction
operations now affect target admission and ranking. Linked rows collapse to one
root commitment; incompatible roots reject the target before ranking, while
compatible roots impose a deterministic `-12` penalty each, capped at `-24`.
Exact active patrol remains compatible with a non-patrol defensive response but
still blocks another patrol. If order-type resolution chooses `PATROL` for that
candidate, preparation excludes it and deterministically reranks without
changing the full candidate fingerprint. Mixed compatible/blocking rows sharing
one root resolve once with blocking precedence. Preparation is freeze-only;
admission rechecks commitments before debit even on a pressure-marked retry, and
rechecks `ept2` candidate identity before pressure for an unpressured row.
Planning quarantine now has one production owner. Save validation bounds the
imported failure reason and delegates to that authority. The first matching
failure normalizes the planning row and advances its revision once; linked
order-planning cleanup remains a separate fail-closed validator responsibility.
An already quarantined row is rejected without changing its revision or failure
strings on a repeated validation pass. The focused engine case
`HST_TEST_EnemyPlanningCommitmentAuthority` ran in
`logs_2026-07-13_11-44-28`; its JUnit report at
`2026-07-13T15:44:34.667Z` records one testcase, zero failures, an empty failed
list, and `AllExact()` true for all 17 fixtures, including retry-quarantine
repeated-pass idempotency. This correction is sealed only as source, Foundation,
all-target Workbench, and focused engine evidence at the identity and CRC above.
Campaign Debug, HST_Dev/coordinator isolation and artifacts, live authority,
package execution,
save/restart, dedicated and live-server behavior, multiplayer, and soak proof
remain open.

The base Schema 68/settings 24 enemy-planning authority is an earlier sealed
source/Workbench checkpoint at implementation
`356b0d47f96111c3b09eb7ede3cb34f0661c2b6e`, UTC
`2026-07-13T01:04:41Z`, label
`schema68-settings24-enemy-planning-authority`, Foundation 744, and Workbench CRC
`971d30d0`. Schema 67/settings 24 is the earlier resource checkpoint at
`2798cb20b824ed74419ab6dc9bdce03f18ef71df`, UTC
`2026-07-12T23:46:02Z`, and label
`schema67-settings24-enemy-strategic-resource-authority`. Foundation passes at
736 script-symbol references. Final stamped normal log
`logs_2026-07-12_19-52-14` and all-five log
`logs_2026-07-12_19-52-36` both load 5,809 Game files/11,751 classes with CRC
`a353fa0d`. All-five reports `Script validation successful` for WORKBENCH, PC,
XBOX, PS4, and PS5; zero HST script errors were observed and zero Workbench
processes survived cleanup. Schema 67
versions one `HST_FactionPoolState` per enemy role and persists bounded
`HST_EnemyStrategicMutationState` receipts for canonical income, spend, refund,
and aggression mutation. Operational receipts use a contiguous per-faction
sequence, include accepted zero-effect commands, never compact, and hard-stop at
4,096 per faction. Periodic receipts compact independently and are fenced by
persisted last-bucket checkpoints. Pre-67 migration adopts valid current balances,
aggression, and legacy cadence accumulators only; it invents no mutation history, spend, refund, settlement,
order, or planning decision. Malformed Schema-67 authority quarantines at `-67`.

The sealed Schema-68 source adds one separate planning row per
configured enemy. It freezes each role's independent 180-second checkpoint and
latest deterministic decision without moving resource truth out of Schema 67.
Its source and Workbench boundary is sealed; its source proof routes remain
unexecuted Campaign Debug evidence.

The Partisan: Everon branding change does not alter campaign or settings schema.
The non-public `histasi` Workbench project ID and `HST_*` source/resource
convention remain stable. `$profile:Partisan` is the canonical generated-profile
root. Before server or local-client consumers run, the shared path service
recursively enumerates the complete retired tree and handles every nested file:

- If the canonical relative destination is absent, copy the file there.
- If canonical content already has identical bytes, keep canonical and remove
  the duplicate source.
- If canonical content differs, preserve canonical and copy the retired file
  under `$profile:Partisan/legacy-profile-archive`, using a deterministic suffix
  when an archive name is already occupied by different bytes.
- For every new canonical or archive destination, first copy the source to a
  unique migration staging file and compare it byte-for-byte. Recheck that the
  final destination is still absent before promotion, then compare source and
  final destination again before deleting the source. A failed copy,
  comparison, archive allocation, or deletion leaves the source for a later
  retry.
- If a retired directory path conflicts with canonical file structure, mirror
  that relative directory under the archive's `directory-conflicts` subtree.
  If that archive directory cannot be created, retain the source directory and
  report the incomplete migration.
- Delete directories deepest first and remove the retired root only after the
  files are gone and empty-directory deletion is re-verified.

After that byte-preserving move, settings, campaign fallback, editor preferences,
and loadouts continue through their existing semantic schema migration and
normalization paths. The compatibility resolver remains only as a partial-failure
recovery net. This whole-tree move does not require a campaign or runtime-settings
schema bump. The latest packaged test did not contain a retired tree, so copy,
archive, removal, and restart evidence is still open.

A static guard prevents re-entry within one game process. Enforce file I/O does
not expose atomic no-overwrite promotion, create-if-absent, or an exclusive file
lock, so the supported migration contract is single-writer startup. Do not run
multiple server, client, or Workbench processes against the same profile while
the retired tree is being migrated. Under that contract, canonical content wins
and is never deliberately overwritten; verified staging narrows failure windows
but is not a cross-process transaction primitive.

The immediately preceding sealed source/Workbench checkpoint is Schema
66/settings 24. It is the Blueprint Phase 8 local-security checkpoint, not runtime
certification. Its source stamp identifies
implementation `a7031797e67d99a99a066038cd8fa39efc03cff1`, UTC
`2026-07-12T20:28:33Z`, and label
`schema66-settings24-local-security-marker-integrity`. Schema 66 persists one exact enemy-
town local-security envelope with deterministic ownership-revision/epoch
identity, an authored frozen 2–5 member roster, operation/manifest/batch/group
links, exact living counts, terminal/loss receipts, and rearm evidence. Physical
roots and cyclic waypoints remain process-local. That sealed source also repairs
the `27672e6` client-local campaign-marker ownership regression; marker state is
derived presentation and does not add save fields. Foundation passes at 729
script-symbol references. Final normal/all-five Workbench checks pass at 5,806
Game files/11,740 classes with CRC `ec860be7`, explicit validation success, zero
HST script errors, and zero surviving processes. Campaign Debug, native, save/
restart, package, multiplayer, marker-input, and soak evidence remain pending.

The previous sealed campaign checkpoint is Schema 65. It identifies implementation
`609add9eeadf73816764c497178e2d35081307d1`, UTC
`2026-07-12T18:30:29Z`, and label
`schema65-settings24-civilian-consequence-authority`. Schema 65 persists a
revisioned locality consequence envelope and exact aggression before/after
evidence on town-influence events while keeping ambient actor topology session-
only. The deterministic casualty, replay/conflict, attribution, theft, combat-
episode, minor-locality, malformed-authority, aggression-admission, persistence,
and pedestrian panic/recovery proof cases are complete and wired. They are
state-only source proofs, not native entities, real profile serialization,
package, server/client, or multiplayer evidence. Final stamped normal Workbench
compile/create and all-five validation are clean at 5,802 Game files/11,728
classes with CRC `c0a672b9`; all-five reports `Script validation successful`,
both runs exited `0`, zero HST script errors were observed, and zero Workbench
processes survived cleanup. The preceding unstamped CRC `be076102` remains
preliminary evidence only. Foundation passes at 717 script-symbol references;
native, package, real-profile, multiplayer, and soak gates remain open.

The earlier sealed checkpoint is Campaign
Schema 64/runtime-settings Schema 24. It identifies implementation
`6afadc7c13681b78171939a740862e52328beffd`, UTC
`2026-07-12T15:57:55Z`, and label
`schema64-settings24-ambient-runtime-authority`. Its final sealed-tree evidence is
Foundation at 711 script-symbol references and normal compilation at 5,799 files/
11,718 classes with CRC `bb083672`, explicit validation success for all five
configurations, zero HST script errors, and zero surviving Workbench processes.
Campaign Debug, real profile save/
restart, native callbacks and movement, rendered behavior, ten-town/ten-minute
stutter/churn measurement, and multiplayer remain open.

The earlier sealed checkpoint is Campaign Schema 64 on runtime-settings Schema
23.
It adds the canonical curated-town political support/population boundary on top
of the sealed Schema-62 ownership and Schema-63 combat-presence dependencies.
That checkpoint identifies implementation
`6f3c913eaed66926cce38b2ecafcff94084898a3`, UTC
`2026-07-12T11:28:41Z`, and label `schema64-canonical-town-influence`.
Foundation passes at 696 script-symbol references, including the
dedicated Schema-64 gate. Normal Workbench compilation and all-five-
configuration validation pass at 5,793 files/11,695 classes with CRC
`36d5b017`, successful validation, and zero HST script errors. Every Workbench
instance was closed and the verified process count was zero. Campaign Debug,
real serialization, save/restart, packaged runtime, rendered UI, stutter
measurement, and multiplayer proof remain pending for that sealed checkpoint.

Schema 63 is an earlier sealed source/Workbench checkpoint. It identifies implementation
`85a75c65e9c148a890d8d78b0288ae6483a5ccd9`, UTC
`2026-07-12T08:22:05Z`, and label
`schema63-canonical-combat-presence`.

Schema 62 is the preceding sealed source/Workbench checkpoint for canonical,
revisioned ownership transitions under implementation
`7c93e0a485bcabe5a364c0b0cfeca235accb50f7`, UTC
`2026-07-12T06:11:19Z`, and label
`schema62-canonical-ownership-transition`. Foundation passes with 670 script-
symbol references. Headless Workbench Game validation loaded 5,785 files/11,652
classes with CRC `22c13a32` and zero script errors; the normal Script Editor open
remained responsive without a crash, and zero Workbench processes survived the
test. Schema 61 is the preceding sealed marker-projection foundation. Packaged
evidence remains open.

## No Schema Bump: Exact Defensive-QRF PREPARED Recovery and External Proof

The current checkpoint adds no serialized field or enum insertion. Exact
defensive QRFs reuse the `PREPARED` settlement value appended under Schema 69
and the existing Schema-51 order/operation/resource fields. Restore may resume
only a complete current exact-QRF graph whose persisted authority proves all of
the following:

- one reciprocal order, operation, and manifest plus the allowed durable
  batch/group survivor authority;
- `PREPARED` settlement state, terminal intent/result, reason, settlement
  identity, and reachable order predecessor state;
- one complete staged order/refund tuple with
  `m_bResourceSettlementApplied = false` before refund publication;
- one canonical original debit with exact faction/order/operation/manifest/zone
  backlinks, deltas, contribution shape, and chronology; and
- either no refund mutation yet, or the one exact matching refund mutation
  represented by the staged tuple.

Recovery validates that graph, applies or replays the canonical refund,
publishes the applied receipt last, and finalizes the operation/order tail. The
first reconcile changes the accepted prefix; an identical replay and a second
capture/restore are no-ops. The uncommitted full-refund proof retains its
operation after removing runtime-commitment backlinks; at most one deterministic
inert residual batch and at most one deterministic inert residual group may
participate. Truly operationless admission rollback retains its existing
refund-first contract and is outside durable `PREPARED` recovery.

Container schema alone cannot distinguish historical mutationless settlement
from current strategic-receipt authority. Restore therefore snapshots row-level
receipt provenance before Schema-67 normalization. After that normalization it
revalidates every current-provenance `SETTLED` exact-QRF chain and every
`PREPARED` pool/debit/refund chain. A malformed current chain disarms settlement
authority, aborts the order, cancels and strategically holds the reciprocal
batch, retains the group, and persists the stable
`exact_restore_resource_authority_quarantined` result. A repeat restore must
preserve that status, reason, pool balance, and mutation count. Generic
projection restore must not revive the retained group.

A historical settlement with no row-level strategic receipt provenance remains
mutationless compatibility history. Restore does not fabricate debit/refund
mutations for it. Conversely, any current receipt identity or aggregate claimant
opts the row into strict validation even when the claimed mutation is malformed
or missing. Arbitrary old partial tuples never become recovery candidates and
remain fail-closed.

The focused engine proof and R26 still exercise the full deterministic matrix
through in-process `HST_CampaignSaveData` capture/restore. Separately, the
guarded external harness stops and starts a fresh executable for prepare,
recover, and replay at each committed dual-pool cut. It verifies the source
before reconciliation, requires the first recovery to change state exactly
once, requires replay to be a no-op with the same terminal fingerprint, and
reads the terminal state back from the canonical fallback. This adds no
migration: it reuses the current Schema-70 JSON shape. Support-only,
uncommitted-full-refund, malformed/quarantine, and historical-compatibility cuts
remain in-process only, and native save-source precedence plus broader campaign
restart certification remain open.

## Schema 70

Schema 70 appends `HST_OPERATION_TYPE_ENEMY_GARRISON_REBUILD` after every
Schema-69 operation ordinal and persists the selected target's ownership
revision on `HST_EnemyOrderState`. It does not reinterpret older rebuild rows.
Rebuild orders restored from Schema 69 or earlier remain historical contract-`0`
rows; a pre-70 rebuild carrying a nonzero exact contract is quarantined instead
of being upgraded from incomplete evidence.

- Newly admitted exact rebuilds use contract `1`, cost exactly 10 support
  resources, and freeze one infantry-only manifest sized from authoritative
  source capacity and the target's available garrison capacity. The manifest and
  selected-zone ownership capability are preflighted before debit; after debit,
  one reciprocal order/operation/manifest/batch/group graph owns the roster and
  lifecycle or exact rollback restores the debit.
- The planning capability hash includes the selected target and source zone IDs,
  owners, and ownership revisions. A target or source ownership ABA therefore
  changes the capability even when the final owner string matches again. Initial
  admission rejects before pressure; a pressure-marked retry rechecks and rejects
  before order creation or debit.
- The same living manifest slots travel virtually, materialize physically, fold
  confirmed casualties, and restore conservatively. Delivery links the exact
  manifest to the destination garrison and records one zero-delta
  `delivered_garrison_transfer` receipt while the operation remains `OPEN` and
  `ON_STATION`; it does not add the roster to aggregate infantry or create a
  replacement force.
- A delivered roster remains exact held authority and may continue to
  materialize or dematerialize. Terminal invalidation, destruction, or campaign
  stop unlinks that manifest and settles with no refund. Before delivery,
  invalidated survivors return to origin and eligible terminal paths refund the
  original support debit exactly or proportionally from the durable living
  roster.
- `PREPARED` remains the durable terminal-intent boundary introduced by Schema
  69. Exact receipt application, operation settlement, and final order lifecycle
  are independently resumable. Restore repairs a valid stale order tail from the
  receipt and settled operation without changing receipt identity or amount;
  conflicting receipt, terminal policy, or settlement identity quarantines
  instead of guessing.
- Valid saved physical or in-progress projections normalize to process-free
  strategic hold while preserving confirmed casualties and durable positions.
  Schema-70 classification runs before generic active-group normalization; final
  validation runs after generic, ownership, strategic-resource, and Schema-69
  counterattack prerequisites.
- Missing, duplicate, partial, orphaned, foreign, or otherwise ambiguous exact
  authority quarantines at `-70`. Quarantine freezes every claimed nonterminal
  batch into non-executable strategic hold, clears process-only group state, and
  retains the order, operation, manifest, batch, group, debit, and refund
  evidence through retention/pruning. It never fabricates cleanup, settlement,
  delivery, casualty, or refund authority.

The stamped checkpoint identity and remaining runtime gates are recorded under
Current Schema above. The focused engine proof covers admission/capacity,
held-roster delivery, physical/virtual casualty continuity, restore, ownership
terminal settlement, admission rollback, prearrival survivor refund,
PREPARED/SETTLED crash resume, historical isolation, malformed and orphan
quarantine, quarantine retention, and selected target/source ownership ABA
rejection. R10 passes exact clock/fingerprint isolation, all five Phase 18
cases, the targeted Phase 22 stable-identity/strategic/RUN assertions, and typed
order cleanup with zero settlement failures, open orders, or runtime claimants.
The ambient cadence hold, typed cleanup ordering, and exact marker-backing
validation do not alter Schema 70. R17 proved that R16's 11-live/10-restored
mission count was a debug-fixture classifier collision, not migration proof or
evidence of a general production persistence defect. R19 separately proves the
current-schema civilian zero-value gate and exact in-process persistence
roundtrip. Native/package,
restart, dedicated-server, network/JIP/reconnect, and soak gates remain open.

## Schema 69

Schema 69 adds no new serialized fields. It appends the persisted enemy-
counterattack operation enum value and the `PREPARED` operation-settlement value
after all existing values, then defines how the existing enemy-order, operation,
frozen-manifest, spawn-batch, active-group, route, virtual-combat, ownership, and
resource-settlement fields form one exact aggregate.

- Counterattacks restored from Schema 68 or earlier remain historical contract-
  zero rows. Migration does not invent a source, manifest, roster, route,
  operation, debit, refund, settlement, or ownership outcome.
- New exact rows use contract `1`. They must retain one frozen infantry-only
  manifest, reciprocal durable identities, a direct route, and exactly one
  prepaid resource pool: attack for proactive orders or support for reactive
  capture responses.
- Valid live physical rows restore conservatively as strategic virtual
  authority after adopting their last durable position and confirmed survivor
  ledger. Confirmed casualties remain confirmed.
- The same living manifest slots drive deterministic virtual combat and later
  materialization. Restore never estimates a replacement roster from faction,
  source, target, cost, or previous aggregate counts.
- Capture remains pending until the canonical ownership-transition authority
  accepts and completes the stable request. Return and survivor-proportional
  settlement refund exactly the attack or support pool recorded by the original
  debit; restore does not redirect, infer, or replay either effect.
- `PREPARED` stores terminal intent before settlement completes. The operation
  is prepared, the complete order/refund tuple is staged, the canonical refund
  is applied or replayed, the resource receipt is recorded, and only then is the
  operation finalized. Restore and same-session ticks resume this sequence
  idempotently. An uncommitted full-refund prefix may lack operation/manifest
  rows and retain at most one uniquely matching inert batch/group residue.
- Invalid, ambiguous, partial, or unsupported exact claimants quarantine at
  `-69`. Quarantine is idempotent and fail-closed: it holds physical projection
  but never downgrades to the legacy consumer, deletes authority, fabricates a
  refund, settles resources, or applies a capture outcome. Claimant scans cover
  explicit backlinks and deterministic batch, projection, force, and execution
  identities, preventing foreign or duplicate residue from being treated as a
  safe cleanup candidate.
- Generic projection normalization skips this operation family; the dedicated
  Schema-69 validator owns its restore boundary before later generic restore
  projection handling.

The scoped source/Foundation/all-target Workbench/focused-engine checkpoint is
sealed at the identity above. The focused report covers valid PREPARED recovery,
same-session ABORTED recovery, foreign derived-ID collision hold, and
fail-closed SETTLED state without a resource receipt. No native serialization or
process restart ran: Full Campaign Debug, actual old/current-save migration,
profile serialization/restart, package/native/live-server behavior, marker
runtime, networking/JIP/reconnect, and soak remain open until direct evidence is
recorded.

## Schema 68

- The Schema-68 enemy-planning authority was sealed on runtime-settings Schema
  24 at implementation `356b0d47f96111c3b09eb7ede3cb34f0661c2b6e`,
  UTC `2026-07-13T01:04:41Z`, label
  `schema68-settings24-enemy-planning-authority`, Foundation 744, and Workbench
  CRC `971d30d0` at 5,812 files/11,761 classes. Its state-only assertions are
  wired but Campaign Debug, save/restart, package, dedicated-server,
  multiplayer, and soak proof remains open. The later schema-neutral bootstrap/
  profile/marker correction is sealed at implementation
  `fdf262637e74a70c12454f6c1d3789c2cd0a0f05`, Foundation 751, and final
  stamped-tree all-target Workbench CRC `0544aa1d`; `f97b12e` remains its prior
  packaged failure baseline. The commitment-aware correction is the immediately
  preceding seal at implementation
  `695caf46ce6b4146e5407711b76d5e0c578d7392`, UTC
  `2026-07-13T14:44:37Z`, label
  `schema68-settings24-commitment-aware-enemy-planning`, Foundation 751, and CRC
  `e483e71c`. The current schema-neutral engine-proof correction is sealed at
  implementation `4c9a94a1cb4811b6e75a7dca5dba70efffcb523d`, UTC
  `2026-07-13T15:43:01Z`, label
  `schema68-settings24-enemy-planning-engine-proof`, Foundation 753, and final
  stamped-tree all-target Workbench log `logs_2026-07-13_11-43-49` at 5,816 Game
  files/11,770 classes with CRC `5a998c21`. Campaign Debug, package
  execution, save/restart, dedicated and live-server behavior, multiplayer, and
  soak proof remain open. Schema 67 remains the preceding resource authority
  recorded below.
- Each configured enemy role owns exactly one separate
  `HST_EnemyPlanningState`. The row persists its own 180-second cadence,
  revision, and decision sequence; occupier and invader checkpoints advance
  independently.
- The frozen input records war level, aggression, Schema-67 pool revision and
  balances, operational mutation count, commitments, and stable sorted
  commitment/target/source candidate hashes. The frozen decision records the
  selected target and source, enemy order type, support type, capability and
  manifest identity, spend mode, attack/support cost, target-pressure
  before/delta/after and application state, and stable decision/order/operation/
  debit accounting identities. Input and decision fingerprints must recompute
  exactly.
- Current target admission classifies queued or active same-faction enemy orders
  and support requests and open same-faction operations at equivalent zone IDs.
  Rows linked through an order or operation collapse to one root commitment.
  Incompatible roots remove the target before scoring or ranking; terminal and
  rival-faction rows do not block it. An exact active patrol root is compatible
  with a non-patrol defensive response, but the final order-type gate blocks a
  duplicate patrol. If a root contains both compatible and blocking rows,
  blocking takes precedence and that root is counted once. Known equivalent
  canonical/legacy zone IDs resolve to the same target. Every compatible root
  contributes a deterministic `-12` score penalty capped at `-24`.
- The target-candidate fingerprint is `ept2`. It includes each retained
  candidate's compatible-root count and commitment score so commitment-aware
  ranking is part of the frozen candidate-set identity rather than diagnostics
  alone. If the highest exact-patrol-compatible candidate later resolves to
  `PATROL`, preparation excludes it only from the current selection pass and
  reruns the same salted ranking. The full candidate count and `ept2` fingerprint
  remain unchanged while the decision selects the next valid target or a clean
  no-target result, avoiding a wasted planning cadence.
- Rejection diagnostics are deterministic: linked rows collapse before the
  incompatible-root count is reported, blocking evidence is sorted, and the
  canonical first rejection reason is stable across input permutations.
- Pre-68 migration clears every claimed planning row and every planning backlink
  on an old enemy order. After configured roles and exact Schema-67 pools are
  known, it creates one baseline per role with `last = elapsed`, `next = elapsed
  + 180`, decision sequence `0`, disposition `idle`, and no invented decision,
  target, order, debit, pressure, candidate, commitment, or fingerprint. Old
  orders retain planning contract `0`.
- Current Schema-68 missing, duplicate, foreign-role, malformed, cadence-tampered,
  fingerprint-divergent, or broken order/debit-backlink authority quarantines at
  `-68`. Quarantine changes only planning metadata and the planning metadata on
  affected orders; it never changes, reconstructs, debits, refunds, or repairs a
  Schema-67 pool or strategic mutation receipt. Save validation bounds imported
  failure text and delegates to the production planning authority. The first
  matching quarantine normalizes the row and advances its revision once; a
  repeated pass with the same reason changes nothing.
- New current-schema fallback construction is different from restore migration:
  the shared production factory used by startup, admin reset, and proof creates
  the exact configured three-role pools and two idle planning rows before the
  restored-role validators run. Persistence replaces those fallback arrays when
  serialized state exists, so missing/malformed restored authority still reaches
  the validators unchanged and remains fail-closed.
- A schema-neutral compatibility recovery is allowed only for the complete known
  generated poison signature: restored Schema 68, exact nonempty preset identity,
  exactly three non-null pools
  containing one neutral resistance row and one untouched `-67` row per enemy,
  exactly two untouched `-68` planner rows, and empty strategic-mutation and
  enemy-order arrays. It rebuilds configured balances with `1/1` pool authority
  and `1/1` idle planning baselines at the current elapsed second. Any resource,
  topology, preset, null-row, legacy-order, versioned-order, or other mismatch performs
  no mutation. This is not a general current-schema quarantine repair. The
  deterministic fixture also requires unrelated state preservation, a no-op
  second recovery attempt, validator acceptance, and an exact save roundtrip.
- A due decision first persists as `prepared`. Preparation is freeze-only and
  does not apply target pressure, debit resources, or create an order. For a
  prepared decision without an existing durable order, admission always
  recomputes the commitment fingerprint before debit or order creation,
  including when target pressure is already marked. An unpressured row also
  recomputes the `ept2` target-candidate fingerprint and revalidates the frozen
  target/source/order gates before pressure. A changed commitment therefore
  rejects a pressure-marked retry before debit, while a changed candidate set
  rejects an unpressured row before any side effect. A retry remains the same
  frozen decision and is bounded to a 30-second retry checkpoint.
- Completion is an explicit `committed`, `skipped`, or `rejected` disposition.
  `committed` requires one exact order and applied Schema-67 debit receipt; a
  prepared row with that exact crash-window graph may reconcile to committed.
  If every eligible target is incompatibly committed, the planner freezes zero
  candidates, zero cost, and zero pressure and completes as `skipped` without an
  order, debit, or rival-planner mutation. `skipped` may not claim applied target
  pressure; `rejected` may retain pressure already applied by an older or
  later-failing path.
- A transient failure before a due decision can freeze persists a separate
  non-prepared `nextRetry = now + 30` gate and bounded reason without changing
  the prior terminal decision. Current restore rejects retry timestamps beyond
  the saved elapsed second plus 30. Successful begin clears that gate.
- `prepared` with positive frozen pressure and `applied = false` is a valid
  pre-pressure crash window and is now the normal post-freeze state for a new
  actionable decision. A matching prepared order plus exact Schema-67 debit may
  be adopted as committed; adoption clears retry residue. Revision headroom is
  reserved before begin, pressure mark, retry, and completion so no step can
  serialize `int.MAX` or mutate pressure without completion room.
- This correction does not change campaign Schema 68 or settings Schema 24. An
  older unpressured prepared row carrying the `ept1` candidate identity remains
  readable, but current admission can reject it before pressure, debit, or order
  mutation when `ept2` does not match. Migration does not select a replacement
  target or rewrite the frozen decision.
- The expanded deterministic proof covers queued order and support
  blockers; settled or terminal operation and rival-faction ignores; equivalent
  canonical/legacy zone IDs; mixed-root blocking precedence; stable rejection
  diagnostics across permutations; patrol reranking; and commitment changes on
  both unpressured and pressure-marked prepared rows. The focused official engine
  case passes `AllExact()` across all 17 fixtures, including retry-quarantine
  repeated-pass idempotency, with one testcase, zero failures, and an empty failed
  list. These fixtures remain unexecuted Campaign Debug evidence until the
  coordinator assertion runs.
- Immediate counterattacks and existing debug/direct order paths do not claim
  periodic planner authority. Their orders remain planning contract `0` unless
  they are admitted through the Schema-68 prepared-decision boundary.

## Schema 67

- `HST_EnemyStrategicResourceSaveValidationService` owns pre-67 baseline adoption,
  current-shape validation, receipt/pool quarantine, and post-copy validation.
- Each configured enemy role owns exactly one versioned `HST_FactionPoolState`.
  Its canonical durable facts are faction identity with a validated live-preset
  enemy role, attack-resource balance,
  support-resource balance, aggression, resource/aggression cadence accumulators
  and last-bucket checkpoints, contract/revision, per-faction operational count,
  migration baseline, bounded receipt history, and fail-closed
  authority diagnostics. A duplicate or
  non-enemy pool is not silently merged or selected by array order.
- Every accepted operational spend, refund, aggression, or adjustment command,
  including a zero-effect command, retains one bounded
  immutable `HST_EnemyStrategicMutationState`. The receipt freezes stable
  mutation identity, contract/applied state, faction, mutation kind, operational
  sequence, and signed deltas,
  before/after values, source identity, campaign second, and any required enemy
  order, exact operation, or accounting backlinks.
- Restore validates retained receipts without replaying them. Each receipt's own
  before/delta/after arithmetic must be exact; when a predecessor revision is
  retained, its after values must match the next before values; the unique latest
  retained receipt must match the pool revision/projection. Identity/fingerprint
  and retained revisions remain unique. Exact replay is an API result, never a
  restore mutation.
- Exact defensive-QRF and enemy-patrol contracts remain unchanged. Linked exact
  rows resolve their reciprocal debit/refund mutation IDs;
  the receipt must name the same faction, order/operation, mutation purpose,
  amount, and terminal/admission source. Historical/legacy enemy orders receive
  no invented exact backlink and keep their compatibility lifecycle, while any
  post-67 resource/aggression mutation still enters the canonical API.
- Operational receipt retention never compacts or evicts in Schema 67. Each
  faction must retain exactly one row for every operational sequence from `1`
  through its persisted count, up to 4,096. Reaching the cap rejects later
  operational mutations for only that faction. The rival pool and periodic
  cadence remain independent. Periodic income/decay is the only compacted
  evidence: at most one current row per faction/kind, with the corresponding
  last-bucket checkpoint equal to elapsed time minus its accumulator.
- For every pre-67 save, preserve each unique valid enemy pool's current attack
  resources, support resources, and aggression as its Schema-67 baseline. Set
  the version/revision to the migration baseline, copy the valid legacy global
  resource-income and aggression-decay accumulators into each enemy pool, and
  initialize last-bucket checkpoints from elapsed time minus those accumulators,
  a zero operational count, and an empty mutation history. Do not derive receipts by diffing totals, replay old strategic events,
  or infer income, spend, refunds, settlements, orders, targets, or decisions.
- Pre-67 missing/duplicate/invalid pool data follows the established conservative
  pool normalization policy only when the canonical enemy role and values are
  unambiguous. Ambiguity fails closed; migration must not move values between
  occupier and invader or choose the first row. A missing configured role inserts
  an inert `-67` placeholder instead of silently inventing a zero-valued pool.
- A current Schema-67 duplicate pool, invalid role, malformed contract/revision,
  broken arithmetic or operational sequence chain, conflicting mutation identity,
  invalid cadence checkpoint/bound/order, missing required exact-consumer,
  support-ledger, town-event, or ownership-transition backlink
  quarantines at `-67`. Quarantine changes authority metadata but never repairs
  or replays resource projections; it cannot fall
  back to direct pool mutation or legacy exact-QRF/patrol accounting.
- Restore removes invalid, orphaned, rejected-role, and prior `-67` receipt
  tombstones from the canonical receipt array only after attribution and
  quarantine are recorded. Those rows are not accepted replay evidence and do
  not count against either faction's 4,096-row operational capacity. The
  attributed pool remains quarantined; purging the rejected row never converts
  malformed authority into a valid mutation.
- Schema-67 deep-copy/restore includes every versioned pool field and bounded
  receipt field. Real restart must preserve identical balances, aggression,
  revisions, operational counts/sequences, cadence checkpoints, fingerprints,
  and backlinks without applying any retained mutation again.
- Schema 67 does not persist enemy-planning cadence, candidate sets, selected
  target/source/order, cost quote, or decision fingerprint. Sealed Schema 68 adds
  those facts without reconstructing them from Schema-67 resource receipts or
  historical orders.

## Schema 66

- A current save may carry at most one `HST_LocalSecurityPatrolState` per
  canonical town. The envelope persists contract/revision, town/faction,
  canonical ownership revision, epoch, source, baseline flag, exact policy and
  police strength, deterministic operation/manifest/hash/batch/force/projection/
  group IDs, active/terminal state, original/living infantry, timestamps, exact
  loss-event application, terminal reason, and authority failure.
- Active authority must resolve one unique reciprocal town backlink, typed local-
  security operation, frozen authored manifest, held/pending SpawnQueue batch,
  and active group. Original roster size is `clamp(policeStrength + 1, 2, 5)`;
  envelope, operation, group, and durable member slots must agree on living
  strength. A process-local route, member entity ID, root handle, or waypoint
  cursor is not valid save authority.
- Current restore isolates local-security operations, manifests, batches, and
  groups before generic normalization. This prevents generic duplicate cleanup,
  fold, garrison repair, or survivor normalization from deleting or refilling an
  exact claimant. Active pending/physical authority is normalized to held
  virtual state with process handles cleared and living strength derived from
  durable slots. Terminal graphs retain the compact operation and manifest but
  no batch, active group, or runtime ownership.
- Complete destruction requires terminal `DESTROYED`, living zero, and exactly
  one applied `local_security_patrol_destroyed_<patrolId>` town-influence event
  with police `-1`, the local-security operation as source, and every other
  political/population/aggression/contact effect zero. A non-destruction terminal
  result may not claim this loss. Conflicts quarantine the aggregate at `-66`.
- A terminal epoch remains the zone backlink and blocks automatic resurrection.
  A later epoch may replace it only after canonical ownership revision advances
  or an applied exact town event after terminal/loss has a positive police delta.
  The new epoch increments deterministic identity. Merely retaining positive
  police pressure is insufficient.
- Ownership change, pressure clear, campaign setup/stop, and spawn failure settle
  without a destruction event. Persistence and ownership transitions reconcile
  physical exact casualties/bindings first and defer rather than save/publish an
  ambiguous graph.
- Resistance-held towns target zero automatic police and roadblocks. This policy
  changes future one-step pressure drift; it does not fabricate a migration event
  that rewrites historical pressure instantly and does not alter the separate
  aggregate resistance garrison policy.
- For every pre-66 save, clear zone local-security backlinks and remove only a
  legacy active group whose mode contains `town_security_police` and whose exact,
  mission, support, enemy-order, convoy, garrison, and QRF links are all empty.
  Preserve logical police, roadblock, owner, support, and garrison facts. Do not
  fold its displayed count, infer casualties, create an exact roster/operation,
  charge/refund resources, or apply police loss.
- Record `migration_schema66_local_security_patrol_authority` once with the
  number of disposable projection rows retired. A pre-66 save that somehow
  carries an exact local-security envelope is quarantined rather than adopted as
  trustworthy history.
- The client-local marker integrity repair changes no persisted marker protocol
  or campaign schema facts. On load/stream convergence, protected campaign
  markers are reconstructed as system-owned/non-removable native projections;
  player-created markers remain separate and editable.

## Schema 65

- Every locality persists a contract-`1`, revisioned civilian-consequence
  envelope: danger-active state, combat-episode count, adopted Schema-64 baseline
  floor, last applied episode, consumed combat-presence revision, danger-
  transition second, panic deadline, last exact consequence ID, and fail-closed
  authority failure. The adopted floor is `0..1`; runtime and restore require
  `episode - lastApplied <= 1`. Ambient actor, group, waypoint, threat-position,
  casualty/theft queue, and minor-locality receipt-fingerprint topology remains
  session-only.
- The server destruction callback observes only an exact tracked pedestrian or
  traffic driver and retains the observation for a 256-row casualty queue. A
  separate 64-row queue owns deferred theft. The queues share a four-attempt-per-
  frame drain before claim observation and persistence. Failed receipts remain
  queued indefinitely with bounded 5/10/15-second backoff; queue exhaustion
  retains the observation before ID admission, and pending/faulted consequence
  authority defers capture. A dead-character health check supplies a once-only
  fallback.
- An accepted canonical-town casualty is at most one exact town-influence event. It
  changes the logical remaining/destroyed population, faction support,
  reputation, wanted heat, and attributable enemy aggression without depending
  on whether any other physical civilians exist. Replay with the same
  fingerprint is read-only; conflicting reuse fails closed.
- A civilian traffic/static vehicle theft event can be admitted only after the
  live root successfully promotes to durable `field_vehicle` authority. The
  exact receipt is derived from the durable vehicle ID and applies only for a
  resistance-attributed exact player-pilot claim. Passenger occupancy keeps the
  transient live root out of budget/health recycling until exit or pilot claim,
  but is not promotion or theft evidence.
  Military vehicles and enemy-attributed claims do not receive this consequence.
- Nearby combat consumes the current combat-presence revision and the exact
  current-operation and recent-fire counts stored for the zone. At least one of
  those counts must be positive. `HOT`, `COOLING`, or a garrison presence alone
  cannot open an episode. An already admitted but unapplied receipt drains before
  a new false-to-true edge; that edge advances the persisted episode once and
  applies at most one exact canonical-town event. Repeated observations refresh
  panic without replaying political effects.
- Canonical-town consequence events keep logical population/support in
  `HST_TownInfluenceRecord`, reputation/wanted heat in the durable civilian town
  state, and aggression in the targeted faction pool. Each nonzero aggression
  mutation now records target, delta, and exact before/after values on the town
  event plus one matching applied strategic receipt. Restore validates all of
  those correlations before accepting the event.
- Physical pedestrians use the bounded `Wandering -> Panicked -> Recovering`
  lifecycle. Danger replaces wander behavior with a running move waypoint away
  from the stored threat; recovery restores walking and a deterministic wander
  path only after native acknowledgement. Rebound danger returns the actor to
  panic without spending the ordinary stuck-recovery budget. Lost/stalled panic
  routes use a separate bounded recovery counter, and the hot path does not call
  AI activation. This is physical projection behavior, not logical population
  authority, and traffic has no panic state in this slice.
- A minor locality has no canonical political record. Casualty and nearby-
  combat paths may therefore trigger panic only and use a bounded in-memory
  exact-receipt fingerprint; they cannot write population, support, heat, or
  aggression. That de-duplication receipt does not survive restart. Vehicle-
  theft political consequences require a canonical town and fail closed for a
  minor locality.
- Schema-64 migration clears the newly introduced aggression evidence on every
  older town event, then initializes each zone envelope from its already stored
  combat facts. Positive current-operation/recent-fire facts create episode `1`,
  mark that baseline as the adopted applied floor, and set a bounded 45-second panic deadline;
  otherwise the envelope starts inactive. Migration creates no casualty, theft,
  combat, town-influence, aggression, or strategic event and replays no older
  event effects.
- Current Schema-65 restore validates envelope bounds, revision ordering,
  present danger against the current exact combat facts, adopted-floor/applied
  bounds, panic time, and the canonical town's exact live last-consequence
  backlink. Bounded indexes resolve influence events and strategic receipts once
  for per-zone validation. Save shape proves unique pool/arithmetic/receipt
  structure but cannot know unserialized preset roles, so the coordinator runs a
  post-restore live-preset pass and quarantines any aggression target that is not
  a configured enemy. Invalid envelope authority is quarantined at `-65` with
  danger and panic disabled; inconsistent exact town-influence/aggression
  authority follows the existing Schema-64 fail-closed event/record quarantine
  rather than being repaired by replay.
- Each post-adoption live combat receipt must have exact `+4` heat; zero support,
  reputation, population, police, roadblock, and aggression effects; canonical
  source/reason; and unchanged support/population before/after. ID/kind alone is
  insufficient.
- Stable ID allocation no longer wraps at exhaustion. An empty strategic ID
  fails event admission before mutation.
- Deterministic state-only proofs are complete and wired for resistance/enemy/
  unknown casualty attribution, population scaling, replay/conflict, theft,
  `HOT`-only inertness, danger episode edges, minor-locality panic-only behavior,
  malformed input, aggression admission, save-copy/current-restore quarantine,
  and pedestrian panic/recovery transitions. Final stamped normal and all-five
  Workbench checks are clean at 5,802 Game files/11,728 classes with CRC
  `c0a672b9`; Foundation passes at 717 script-symbol references. Native callback
  ordering, actual death attribution, waypoint movement, profile
  I/O/restart, package execution, rendered behavior, multiplayer, and soak remain
  open gates.

## Runtime Settings Schema 24

- The `23 -> 24` settings migration changed the generated runtime-settings
  contract only and originally left Campaign Schema 64 unchanged. The current
  development save contract later advances independently through Campaign
  Schemas 65 and 66;
  runtime settings remain Schema 24. Schema-23 settings files receive the new
  class defaults when keys are absent, retain explicit valid operator values,
  normalize every value to its documented performance bound, then rewrite as
  Schema 24.
- `civilianGlobalActorBudgetBase` defaults to `48`, and
  `civilianGlobalActorBudgetPerPlayer` defaults to `12`. This is the global cap
  for physically projected pedestrian actors plus traffic drivers. It never
  changes logical town population.
- `civilianGlobalTrafficBudgetBase` defaults to `10`, and
  `civilianGlobalTrafficBudgetPerPlayer` defaults to `2`. The traffic cap is
  nested inside the actor cap because every allocated traffic vehicle requires
  one driver.
- `civilianWarLevelBudgetPenaltyPercent` defaults to `4` percent per war level
  above one. The allocator bounds both the setting and the total applied
  penalty.
- `civilianRuntimeHealthIntervalSeconds`,
  `civilianRuntimeStartupGraceSeconds`, `civilianRuntimeStuckSeconds`,
  `civilianRuntimeMaxRecoveryAttempts`, and
  `civilianRuntimeRetryBackoffSeconds` default to `5`, `15`, `30`, `2`, and
  `20`. Normalization keeps startup, stuck, and retry windows at least as long
  as the health interval and prevents unbounded recovery.
- The existing `civilianDrivingVehicleCountPerTown` default of `5` is the
  configurable daytime/low-heat true-town demand target, not a fixed ceiling.
  Operators may raise or lower the normalized `0..8` setting. Remaining
  population, war level, the global actor/traffic budgets, and competing
  eligible localities may produce a smaller allocation; migration does not
  promise five simultaneous cars in every town.
- Physical pedestrian plus driver demand for one locality is capped to the count
  of unique GUID-qualified character prefabs. Traffic keeps its requested share
  first and pedestrians fit the remainder. Duplicate/invalid config rows do not
  inflate capacity, and exhaustion no longer falls back to a cloned appearance.
- The settings migration proof covers exact Schema-23 defaults, preservation of
  explicit valid overrides, and performance-bound normalization. It is compiled
  and wired into Campaign Debug but has not executed there.

## Blueprint Phase 8 Ambient Runtime Boundary (Campaign Schema 64)

- Ambient pedestrian and traffic lifecycle records, world handles, groups,
  waypoints, and unclaimed vehicle roots remain process-local. They reserve
  physical budgets while queued or recovering but are neither logical
  population nor campaign-save authority.
- The allocator max-merges duplicate locality demand, applies pedestrian and
  traffic floors, and distributes the remainder fairly with at most one actor
  per town per round. A lease of at least 120 seconds stabilizes constrained
  priority before the epoch rotates it. The production reconciliation cursor
  also rotates first service, and no more than four ambient root transactions
  begin per global health update.
- A pedestrian is admitted only after exact living CIV group membership and a
  current wander waypoint are observed. Traffic is admitted only after exact
  living CIV driver membership, pilot-compartment occupancy, engine-on state,
  and a current route waypoint are acknowledged. A request to spawn, move into
  a vehicle, start an engine, or assign a route is not itself success evidence.
- Admitted movement is sampled after startup grace. Stuck roots enter bounded
  recovery and backoff; exhausted roots, dead drivers, destroyed vehicles, and
  lost authority recycle their complete transient aggregate. Logical population
  continues without physical duplication. Static military ambience detects an
  owner or policy-key change, recycles unclaimed old roots, preserves/promotes
  player claims, resets its bounded initialization slots, and repopulates under
  the same four-transaction cap.
- Every pedestrian/traffic lifecycle record has an immutable slot within its
  zone/kind reservation set and retains its original projection seed. Recovery
  builds a deterministic slot- and attempt-specific route without changing that
  identity. The scoped Campaign Debug Phase 20 population helper now selects its
  town from the complete production global plan and consumes the same four-root
  cap.
- Save capture and restore exclude unclaimed `CIV_TRAFFIC_VEHICLE`,
  `CIV_VEHICLE`, and `MILITARY_VEHICLE` rows plus linked cargo. Player occupancy
  promotes an ambient root to durable `field_vehicle` authority; distance moved
  does not. Current normalization converts a legacy live detached ambient claim
  to `field_vehicle` and removes deleted or unclaimed legacy ambience. This
  changes normalization policy without advancing campaign Schema 64 because no
  new durable fields are introduced.
- Player-first occupancy observation runs before persistence on every server
  frame and avoids a full ambient-root occupancy scan. It promotes only live
  tracked roots with live controlled occupants; destroyed roots and dead
  controlled occupants are rejected. Every `HST_PersistenceService` capture or
  checkpoint path repeats the observation behind a fail-closed reconciliation
  barrier, and new-campaign reset performs its own reconciliation. One session-
  only live-root tracker registers ambient promotions, restored/adopted field
  vehicles, and garage redeploys so current transform, destruction, and linked
  cargo position are refreshed before capture.
- A saved durable vehicle ID is campaign identity, even when it resembles an old
  `rpl_*` value. Restore keeps that ID and binds the new root to it; process-local
  replication IDs never rekey a durable row or linked cargo. Restore/registration
  runs before first-frame claim observation. Loot/garage targeting uses exact
  forward and reverse tracker bindings, with unique positional recovery only.
- Garage redeploy allocates a fresh campaign-stable ID and completes tracker
  admission before stored-row removal or payment. Failure removes the spawned
  root, runtime/cargo rows, and binding and restores the stored row when needed.
- A new-campaign reset reconciles registered durable roots before replacing the
  old state. It may retain only occupied live tracked roots whose authority is
  `loot_vehicle`, `field_vehicle`, or `garage_redeploy`; retained rows normalize
  to `field_vehicle`, and their vehicle and linked-cargo rows are copied into the
  new state before replacement. Every other bound root is deleted once. These
  are runtime/capture policies, not new serialized fields, so Campaign Schema
  remains 64.
- The in-memory save-boundary proof covers unclaimed exclusion, claimed field-
  vehicle roundtrip, legacy detached-claim conversion, durable non-ambient
  control rows, and cargo pruning. It is source proof only. Native brief
  enter/exit, autosave/process restart, promoted-root destruction,
  new-campaign-reset, two-nearby-same-prefab restoration without root collapse,
  and loss/duplication checks remain open.
- Routine ambience and movement no longer report a durable campaign-state
  change. Only player-claim promotion can dirty the campaign from this pass.
  This removes a periodic persistence scheduling path implicated in the
  reported stutter, but native profiling is still required.
- Pure budget and actor-lifecycle proofs cover ten-town bounded allocation,
  shared driver accounting, lease rotation, zero/capped demand, strict
  transition paths, read-only illegal edges, movement progress, and bounded
  recycle. The sealed Schema-65 source/Workbench continuation adds the separate civilian-
  consequence and panic proofs described above. These source suites are complete
  and wired, but the ten-town/ten-minute native soak and real save/restart remain
  open gates. Aid and ownership/security-pressure source paths still need
  runtime proof. Exact local-security behavior is introduced later by Schema 66
  and is not evidence for this older ambient checkpoint.

## Schema 64

- One contract-`1` `HST_TownInfluenceRecord` per unique curated
  `HST_ZONE_TOWN` is the sole political support and population authority. It
  persists revision, separate FIA/occupier/invader support in `0..10000` basis
  points, initial/remaining/destroyed population, explicit contact and
  resistance-activity evidence, hysteresis/pending ownership state, radio and
  propaganda aggregates, influence-event counts/next expiry, and last mutation
  evidence.
- The population-scaled formula is pinned to reference commit
  `6e4226d3863ca8673535386c2fff8b6e08a806c4`: effective basis points are
  `round(1000 * raw delta / sqrt(initial population))`. Golden fixtures require
  raw `+1` at populations 100, 25, and 400 to produce `+100`, `+200`, and `+50`
  basis points. Unscaled raw `-50` produces `-5000` basis points.
- Political hysteresis is strict. FIA support above `8000` basis points may
  request resistance ownership; FIA support below `4000` may request enemy
  ownership. Exactly `8000` and exactly `4000` remain neutral. Occupier and
  invader support stay distinct, and enemy selection uses canonical faction
  support/owner evidence rather than collapsing them into one stored total.
  Every flip request and retry goes through `HST_OwnershipTransitionService`;
  migration and compatibility projections never publish an owner.
- Exact influence events add contract version, requested/effective basis-point
  deltas per faction, initial population used, scaling flag, record revisions,
  before/after faction support, and before/after initial/remaining/destroyed
  population. Authorized absolute debug seeds record exact support basis-point
  differences and population changes rather than rounding targets or writing
  the record directly. Replaying an identical event is a no-op; conflicting
  identity fails closed. Permanent events use no expiry. Timed modifiers record
  an expiry, update aggregate counts at mutation time, and trigger an event-
  history rescan only when a canonical record's next expiry is due, avoiding
  the former all-towns-by-all-events one-second scan.
- Pre-64 migration creates records only for unique curated towns. The legacy
  zone signed-support margin has precedence when it disagrees with the civilian
  FIA/occupier pair; the pair is adjusted minimally and one bounded migration-
  conflict event records the disagreement. With no valid civilian pair,
  migration starts from neutral `50/50` and projects the signed margin. Existing
  destroyed population is preserved; migration does not resurrect a town whose
  remaining population is legitimately zero. Legacy influence events become
  contract `0` evidence and are never replayed as Schema-64 mutations.
- Current-schema restore removes null rows, requires exactly one valid canonical
  record per curated town, validates event/revision/aggregate/contact/pending-
  receipt reciprocity, validates every exact population transition and adjacent
  before/after chain, requires the final record population to equal the latest
  exact event, and projects compatibility fields only after authority passes.
  Duplicate, missing, orphaned, malformed, inconsistent, or event-linked
  authority quarantines at contract `-64` rather than being guessed or replayed.
- `HST_ZoneState.m_iSupport` and civilian support/population fields remain only
  signed/read-model compatibility projections. FIA signed support is canonical
  FIA basis points minus the greater of occupier or invader basis points. No
  production political mutation may write those legacy fields as truth.
- Schema-60 Maiden's Bay normalization now also removes a retired town-influence
  record. The Logistics Warehouse is the only canonical Maiden's Bay location
  and is nonpolitical. Simon's Wood remains a resource/minor-locality ambience
  source and receives no canonical town-influence record.
- Town contact is explicit evidence from player, mission, incident, or
  resistance activity; global radio/security drift alone does not reveal a
  town. Zone Pressure emits only valid contacted towns, with the current town
  first and remaining rows ordered by FIA support, display name, and ID.
  Resistance Territory emits every published resistance-owned strategic zone
  except mission bookkeeping in deterministic type/name/ID order and observes
  incomplete ownership-transition publication authority through the same
  completed-parent resolver used by marker projection.
- Schema 64 also throttles verbose changed active-group survivor/count logs to
  30 seconds. Together with due-expiry-only influence rescans, this is a source
  mitigation for the reported one-second stutter, not runtime proof that the
  stutter is gone.
- Persistence smoke now always creates/reuses its reserved nonpolitical mission-
  site sentinel. It cannot select a curated town and write legacy civilian
  support beside canonical town authority.
- Migration and quarantine code passes the full 696-reference Foundation gate
  and normal/all-configuration Workbench compile validation at 5,793 files/
  11,695 classes with CRC `36d5b017` and zero HST script errors. These checks do
  not serialize a real profile or prove a process restart; the deterministic
  migration fixtures also remain unexecuted until Campaign Debug runs.

## Schema 63

- Every zone gains a revisioned combat-presence state: `COLD`, `HOT`, or
  `COOLING`; last-hot and cooling-deadline seconds; separate infantry,
  operational manned-mobile, and static-operator counts; current-operation and
  recent-fire diagnostics; a deterministic contributor hash/reason; and paired
  sorted contributor ID/fact arrays bounded to 24 rows.
- Active groups gain a short-lived physical sample containing conscious
  infantry, operational occupied armed mobile platforms, operational occupied
  static weapons, sample second, authoritative flag, and reason. Persistence
  copies the fields for structural roundtrip, but restore always clears their
  counts and invalidates the authoritative sample because native entity handles
  and occupancy observations cannot cross a process boundary.
- A pre-63 save receives a canonical revision-1 `COLD` baseline for every zone.
  Migration does not infer heat from an active bit, generic survivor count,
  vehicle count, marker, garrison, or unrelated group row and does not replay a
  combat consequence.
- Current-schema `HOT` state restores only with positive living counts, zero
  cooling deadline, valid nonblank diagnostics, paired unique sorted contributor
  rows, and an exact recomputed hash. Oversized otherwise-valid hot diagnostics
  cap to 24 and rehash.
- Current-schema `COOLING` restores only with zero live/operation/fire counts, a
  last-hot time no later than restored campaign time, a still-future deadline
  exactly `1..300` seconds after last-hot, exact `combat area cooling` reason,
  bounded paired diagnostics, and canonical hash shape. This preserves a valid
  already-started deadline rather than restarting or extending it on load;
  expired or temporally malformed cooling becomes canonical `COLD`.
- `COLD` restores only in canonical zero/empty shape. An unknown state,
  negative count, malformed deadline, hash mismatch, unsorted/duplicate/unpaired
  diagnostics, invalid reason, or any other malformed current authority resets
  that zone conservatively to `COLD` while retaining a positive revision.
- Runtime-settings Schema 23 adds
  `capture.combatPresenceCoolingSeconds`. New and pre-23 settings default to 30
  seconds, normalization clamps it to the inclusive `1..300` range, and application copies
  it into the balance contract. Schema 22's true-town civilian-traffic migration
  remains unchanged.
- The new combat-presence service is shared by capture, mission contact/area
  checks, HQ threat, civilian safety, and enemy strategy. Empty or cargo-only
  vehicles no longer gain pressure from a durable vehicle count. Separately,
  physical zone activation enters at the activation radius and exits at the
  larger deactivation radius. These are sealed source contracts, not runtime
  migration evidence.

## Schema 62

- Every existing zone gains ownership contract version `1`, baseline ownership
  revision `1`, and active/latest transition backlinks. Each accepted owner
  change must advance that revision exactly once through a durable
  `HST_OwnershipTransitionState` receipt.
- New receipts persist the immutable expected-owner/revision fingerprint,
  previous/new owner, cause/source/reason/policy, frozen support targets and
  retaliation decision, attempt/timing state, strategic/campaign event links,
  security links, derived decisions, marker epoch/revision/source/sequence
  correlation, immutable setup-without-markers history, and an explicit ordered
  completion checklist. Coherent incomplete
  receipts resume after restore; retryable failure is not converted into a
  completed owner change. Admission stops before a revision that cannot safely
  advance exactly once into valid serialized authority.
- Request construction resolves supported retired zone aliases to the canonical
  durable zone ID before fingerprinting or replay lookup, so an alias spelling
  cannot fork receipt identity.
- Admission and current-schema normalization enforce cause policy: admin cannot
  retaliate; debug cannot retaliate or notify; migration repair cannot retaliate,
  notify, or rewrite security.
- A valid later top-level request is admitted immediately as a pristine durable
  receipt even when an earlier publisher is unresolved. It returns accepted plus
  needs-retry and remains pre-owner in serialized array order; no security,
  support, owner, or derived-domain step runs until every earlier unresolved
  top-level receipt completes. This preserves exact mission, political, admin,
  and migration intent across the fence and process restart.
- Schema 61 saves preserve owners, support, garrisons, operations, events,
  outcomes, and all other domain facts. Migration creates no historical
  transition claims and replays no rewards, security, aggression,
  counterattacks, or capture outcomes; existing owners simply become revision-1
  baselines and one idempotent migration event records that boundary.
- Current-schema malformed or non-reciprocal transition/backlink authority is
  quarantined at contract `-62` without guessing a rollback or reconstructing
  domain truth from markers. Validation includes unique request/zone identity,
  bounded nonblank fingerprint/reason fields, known factions/causes, exact-plus-
  one revision, checklist order, exact support set/applied prefix/same-row event,
  strategic/campaign events, counterattack/order and garrison correlations,
  parent/child projection lifecycle, marker evidence, setup mode, and reciprocal
  active/latest zone backlinks. One idempotent
  conflict event records normalization. Projection-parent checks repeat until no
  new quarantine appears, so malformed-parent quarantine reaches descendants
  regardless of their serialized row order. Forged projection children with an
  unrelated parent and duplicate completed claims for the same zone/applied
  revision also quarantine. Quarantining current zone authority purges every
  unsafe zone-marker row instead of retaining it as apparent ownership evidence.
- Any number of queued top-level followers is valid restore state only while each
  later unresolved follower remains fully pristine and keeps serialized order;
  pre-owner status alone is insufficient. More than one owner-applied incomplete
  top-level publisher, or an owner-applied publisher behind an earlier unresolved
  top-level receipt, is ambiguous and quarantines rather than choosing a
  publication owner. Ordinary rebuilds ignore queued pre-owner
  receipts, but retain the prior owner/revision for an owner-applied active
  receipt or completed unreleased child. Published ownership is resolved from
  exact zone/receipt authority first; a retained marker is only a correlation
  check. Malformed authority reports publication unavailable instead of exposing
  prior/raw ownership. Explicit-ID reconstruction reuses frozen preconditions
  only after semantic identity matches the receipt.
- Accepted exact patrol manifests are valid security authority only when each
  has exactly one reciprocal, open, non-quarantined patrol operation with the
  same owner and assignment. Admission and every pre-owner retry recheck this
  graph, so orphan or late conflicting authority cannot be erased as aggregate
  garrison counts.
- Ownership history is bounded to 512 rows with a minimum 86,400-campaign-second
  replay window. Latest, incomplete, quarantined, unreleased-child, and
  unresolved-enemy-order receipts remain pinned; admission fails closed if no
  eligible completed receipt can make room.
- Zone-marker transport protocol `2` carries the source ownership revision in
  addition to its projection-local revision and global stream sequence. This
  lets custom marker state be correlated with the exact owner revision while
  leaving the Schema-61 delivery/ACK model intact. Marker-local revision and
  source ownership revision are deliberately independent.
- A support consequence can create a nested political transition. Parent
  publication snapshots all logical marker rows plus projection epoch/sequence,
  stages the full graph, validates each exact child receipt-zone-marker chain,
  releases every child, and commits client/native publication only afterward. A
  failed validation restores the exact prior snapshot. Setup publication records
  no marker evidence and freezes immutable parent/child
  `m_bSetupProjectionWithoutMarkers` history that
  remains historical after activation builds current markers.
- When an earlier parent's exact linked-town support event targets a town whose
  active backlink belongs to a later pristine queued top-level receipt, the exact
  support/influence event still applies once. Only political threshold
  reconciliation is deferred, avoiding a parent/child backlink cycle; the normal
  periodic civilian pass evaluates that threshold after the FIFO queue drains.
  Ownership maintenance runs pending receipt retry first, then town policy before
  setup/terminal returns with a frozen-clock bypass. Any political repair in a
  frozen phase suppresses fresh retaliation and notification.
- Frozen support targets are the exact sorted deterministic set of linked towns
  plus every town within 1,500 m. Applied targets must be its ordered retry
  prefix, with each target backed by the same single exact deterministic influence row and deltas.
  Missing, extra, reordered, split-row, or mismatched evidence quarantines.
- Ownership strategic events record exact owner before/after, capture progress,
  and receipt aggression. They intentionally zero unrelated global money/HR/HQ/
  support/enemy-resource deltas that may change while a receipt waits.
- Durable civilian support/heat events remain authoritative if their ownership
  threshold is reached behind a top-level fence. The exact political request is
  admitted as queued pre-owner authority; after restore, a five-second fallback
  sweep waits for existing queued work and admits at most one otherwise-
  unrepresented durable threshold without duplicating the original event.
- Invalid-owner sanitation runs only after restored ownership receipts reconcile.
  It scans sequentially at startup and every five real-time seconds, defers
  accepted or transient repair work, and quarantines only structural conflicts
  or cases already blocked by quarantined top-level authority and requiring
  manual repair. Ownership receipt retry runs in setup and terminal phases with
  campaign-clock rate limiting bypassed, so a frozen campaign clock cannot strand
  migration or other accepted work.
- If runtime or restore retry discovers a structural contradiction and the
  application path quarantines the receipt, retain that concrete receipt/zone
  reason. Do not replace it with generic runtime- or restore-resume text; generic
  text is only a fallback for a failure that produced no quarantine reason.
- Major-change marks are edge-triggered and coalesced behind one bounded
  checkpoint deadline. Repeated gameplay/retry heartbeats do not reset that due
  time; the first change after a successful checkpoint starts a new interval.
  Transition completion re-arms process-local pending state after final status/
  backlink mutation even when restored durable state already says persistence
  was requested, without extending an existing deadline.
- Admin capture and progress reports preserve the result distinction: a valid
  queued request reports accepted-pending with its receipt identity, while an
  invalid request reports the actual rejection.
- Deterministic source proof covers pre-62 migration idempotency, malformed-
  current quarantine, interrupted receipt save/restore/resume, queued intent,
  malformed FIFO publisher ordering, replay retention, source-revision
  projection, staged full-snapshot rollback, resolver fail-close/purge, setup
  history across activation, support set/prefix and derived correlation
  corruption, persistence deadline/re-arm, two-child atomic release, two save/
  restore boundaries with exactly-once political completion, and orphan/late
  exact-security rejection. Production and proof use
  the same logical marker-snapshot builder; only native publication is omitted in
  the proof harness. Headless Workbench validation is clean;
  Campaign Debug, real profile serialization/restart, multiplayer, native-
  marker/security, and gameplay checks remain open while this schema is being
  completed.

## Schema 61

- Schema 61 makes `HST_CampaignState.m_aMapMarkers` a durable, revisioned
  marker-projection journal boundary. Every marker record now carries a
  per-record revision, the global stream sequence that last changed it, a
  tombstone flag, and tombstone time. Campaign state additionally persists a
  positive projection epoch and monotonic global projection sequence.
- Marker rebuild compares each desired record with the prior record by stable
  marker ID and all projected content. An unchanged rebuild preserves revision
  and stream sequence. Create or content update increments that record's
  revision and consumes exactly one global sequence. Removal emits a revisioned
  tombstone instead of silently deleting the projection event. Reusable marker
  identities retain their tombstones so a later recreation cannot reset their
  revision; other tombstones use bounded age/count retention.
- The server owns a marker-only current registry and bounded delta journal.
  Protocol version `1` encodes bounded string packets: snapshot chunks carry
  epoch, snapshot identity, watermark, chunk index/count, total-record count,
  and registry hash; delta packets carry an ordered from/to sequence range and
  final registry hash. Payload decoding rejects malformed, oversized,
  unsupported-version, noncontiguous, or invalid-record input without mutating
  client state. The builders enforce the same row, packet, record, and chunk
  limits before a session can wait for an ACK.
- A player-owned request bridge announces readiness and sends contiguous ACKs
  and resync requests over reliable owner/server RPCs. The server derives the
  player from component ownership, keeps one session per connected player,
  replays retained contiguous deltas when safe, and sends a fresh snapshot for
  first join, reconnect, late join, epoch/hash mismatch, journal gap, invalid
  ACK, or explicit resync. Journal pruning is constrained by the slowest valid
  connected acknowledgement. Each owner has one in-flight delta batch; only its
  final hash-bearing packet ACKs and triggers presentation. Immediate post-ACK
  catch-up plus a five-second readiness heartbeat recover rapid mutations,
  incomplete streams, and lost ACKs without overlapping a fresh delta.
- The client registry is independent of whether the map widget is open. It
  stages all snapshot chunks, verifies count and hash, and swaps them atomically.
  It then accepts only the next contiguous stream sequence and the next legal
  per-record revision. An already-applied delta is idempotently acknowledged;
  a future gap requests resync without partially applying the packet.
- Native campaign marker entities are now reconciled locally from that client
  registry. Server-native campaign marker publication is retired to prevent a
  duplicate server/client marker set. Dynamic player markers remain on their
  existing replicated-entity path and are not part of the Schema-61 stream.
  Shared priority/stable-ID ordering makes the native cap deterministic. The
  stock authored zone descriptor is cached and hidden only after its custom
  replacement is confirmed live; its prior visibility is restored if that
  replacement fails or disappears.
- Static authored marker binding no longer polls a radius every 30 seconds.
  `HST_MapMarkerService` performs exact cached lookup using the authored
  `HST_ConflictMapMarker_<zoneId>` entity name, retries only unresolved names,
  compares the entity's actual affiliated faction before skipping a write, and
  reports missing/bound identity explicitly.
- Pre-61 map-marker rows are derived presentation state, not campaign facts.
  Migration clears and deterministically rebuilds them from authoritative
  campaign state, preserves no guessed marker revision, and records
  `migration_schema61_marker_projection` once. A malformed Schema-61 projection
  clears and rebuilds the derived rows, advances the epoch so clients cannot
  reuse the old stream, and records
  `normalization_schema61_marker_projection_rebuild` once. Neither path invents
  zones, owners, missions, forces, or outcomes.
- The compiled deterministic `HST_MarkerProjectionProofService` defines initial/late-join
  snapshot equality, stable rebuild semantics, ordered create/update/delete,
  duplicate idempotency, snapshot-pending and rapid mutation catch-up,
  final-chunk-only multi-packet ACK, lost-ACK heartbeat recovery, dropped-delta
  resync with atomic snapshot staging, lower-watermark epoch reset, reconnect
  snapshot, acknowledgement-constrained pruning, malformed/oversize rejection,
  and migration idempotency fixtures. They have not been executed as Campaign
  Debug or packaged runtime evidence. A packaged host plus two-client run must still prove
  equal hashes/watermarks, disconnect/reconnect and late join, native marker
  rendering/cleanup, map-close continuity, and real save/restart.

## Schema 60

- Schema 60 adds a separately typed exact player Search-and-Destroy operation
  only for newly quoted and confirmed `HST_SUPPORT_SEARCH_AND_DESTROY`
  requests. Contract `1` links one immutable quote, infantry-only manifest,
  paired ledger transactions, support request, operation, held SpawnQueue batch,
  and active group. The quote costs $350 plus one HR for every frozen member
  slot; the selected catalog roster targets `3 + war level` members within the
  supported catalog bounds. Vehicles, assets, empty rosters, and multi-root
  manifests are rejected rather than shortened or substituted.
- The operation uses its own
  `HST_OPERATION_TYPE_PLAYER_SUPPORT_SEARCH_DESTROY` discriminator while
  reusing the exact infantry strategic-projection boundary. It persists direct-
  route progress, virtual/physical ownership, exact member casualties, virtual
  combat clocks, immutable assignment, mutable live position, and recall/
  settlement identity. Leaving the render bubble folds only observed survivors.
  A physical on-station group folded more than 75 meters from its assignment
  enters `RETURNING_TO_ASSIGNMENT`, travels back virtually, and resumes on-
  station behavior instead of fighting from the wrong strategic position.
- Off-screen combat uses the existing deterministic 30-second infantry power
  steps against the target's abstract hostile garrison. Eliminating the hostile
  infantry clears contact but does not auto-complete the purchased operation;
  it remains on station until commander recall. Recall follows the typed exit
  path and settles eligible living HR through the linked ledger without
  inventing survivors or replaying a refund.
- Physical fold, physical recall exit, and campaign-stop retirement first run
  an exhaustive projection-scoped casualty reconciliation. If survivors remain,
  retirement additionally requires one handed-off root and exactly one unique
  live adapter/PhysicalWar member binding per durable living slot. Persistence
  performs the global exhaustive exact-infantry pass, validates each physical
  exact-support request/operation/batch/group graph and binding cardinality, and
  refreshes the live group position before capture. Any missing or conflicting
  authority defers retirement or checkpoint rather than inventing a casualty.
  Held-batch cancellation snapshots the strategic living roster before queue
  cleanup so immediate recall refunds only the roster that actually survived.
- Pre-Schema-60 Search-and-Destroy requests remain contract `0` on their legacy
  path. Migration does not infer a quote, roster, transaction, operation, batch,
  group, casualty, or settlement. Malformed current exact claimants retain their
  evidence under quarantine contract `-60`, are prevented from falling through
  to legacy execution, and do not receive guessed balance changes. Their group
  quarantine mode/status is recognized by global campaign-state classification,
  so the retained evidence is neither operational nor combat-present.
- Expired exact-support archive capacity applies full receipt reciprocity to
  every positive typed player-support contract, including exact QRF and Search-
  and-Destroy. It prunes a tombstone and its paired terminal request atomically
  only when the tombstone is valid replay authority,
  the aggregate identity has exactly one claimant, no live quote/manifest/
  operation/transaction/batch/group backlink remains, and the terminal request
  fully reciprocates the archived identity, assignment, costs, schedule,
  settlement, and refund receipt. Historical contract-0 QRF retains its minimal
  compatibility match. A malformed or quarantined typed pair remains durable
  evidence and cannot be selected merely to make capacity.
- Schema 60 also removes the overlapping `town_maidens_bay` strategic row and
  keeps `resource_logistics_warehouse` as the one canonical Maiden's Bay
  location. Location normalization is anchor-gated: a save containing neither
  row is left byte-stable. More than one canonical row, or multiple legacy rows
  without one canonical authority, fails closed before any graph, projection,
  ledger, or generated-content rewrite; only one idempotent conflict audit is
  appended. A lone legacy row converts in place and retains its nonzero owner,
  economy, and aggregate-force values. When both rows exist, the canonical
  warehouse keeps its owner/economy and aggregate manpower; the duplicate town
  does not receive a fold-back or manpower credit.
- Once unique location authority exists, mutable contract-0 campaign references
  canonicalize to the warehouse and duplicate ambient/garrison/police
  projections retire directly. Any nonzero typed claimant remains frozen,
  including settled, malformed, quarantined, and graphless exact rows
  recognized from their group mode/status. Typed legacy garrison backlinks may
  retain one zero-manpower compatibility shell, but cannot add to the canonical
  garrison.
- Mutable generated sites/routes rekey to the canonical ID, with canonical
  collisions removing the legacy duplicate. A site or route referenced by
  frozen typed authority remains byte-stable and instead receives a deep
  canonical clone, including route endpoints, validation fields, and every
  waypoint rekey. General `FindZone(oldId)` calls resolve the canonical mutable
  row; exact historical validators may request a detached old-ID/old-position
  view. Runtime duplicate/admission checks treat the old and canonical IDs as
  equivalent without re-enumerating the retired town.
- Duplicate enemy-support ledgers merge only after their stale spend/damage
  windows expire. Spend and refund totals add, recent damage takes the bounded
  maximum, timestamps/cooldown take the maximum, and the newest applicable
  decision reason wins.
- The one-second stutter and horn repairs do not add durable save fields. Runtime
  reconciliation now avoids pure-vehicle count oscillation and redundant/no-
  convoy scans, amortizes unresolved radio discovery, reuses cached recurring
  player authority, and builds expensive visual diagnostics only when their
  throttled log will emit. The native wheeled-vehicle base override disables AI
  horn timing and perceived horn output; the existing ambient-driver input clear
  remains a scoped defensive layer.
- `HST_MaidensBayLocationMigrationProofService` is compiled and wired into
  Campaign Debug as `location_taxonomy.maidens_bay_schema60`. Its isolated
  in-memory fixtures cover both-row and old-only migration, generic-reference
  normalization, frozen-graph isolation, ledger/generated-content handling,
  idempotency, and lookup compatibility. That Campaign Debug assertion has not
  been runtime-executed, so it is not migration or restart evidence.
- Schema-60 full Foundation passes with 644 symbol references. Final stamped
  Workbench Game validation loaded 5,777 files/11,615 classes with CRC
  `7aa80fc9` and created the game. The correctly targeted hidden normal
  WorldEditor stayed alive/responding for 10/10 samples over 20 seconds with no
  first-party error/crash signature; diff check was clean apart from line-ending
  warnings. These are source/Workbench gates only. Packaged server/client,
  actual save/restart, rendered UI/marker, physical movement/combat, reconnect,
  JIP, stutter, and horn gates remain open.
  The Search-and-Destroy proof service is compiled and wired into Campaign Debug,
  but its Schema-60 assertions have not run. It now includes valid paired archive
  capacity prune plus save/restore and corrupt quarantine retention. Its
  deterministic fold/immediate-recall casualty case uses synthetic queue-slot
  state; live adapter casualty retirement and physical recall exit remain
  packaged-runtime gates.
- The compiled/wired `HST_OperationRecordProofService` archive assertion now
  also covers typed exact-QRF receipt mismatch retention under forced capacity;
  it has not been executed by Campaign Debug.

## Schema 59

- Schema 59 introduces one durable `HST_RadioSiteState` for every radio zone.
  The row owns a deterministic site ID and transmitter target ID, immutable
  authored prefab/position provenance plus the mutable current projection after
  resolution, typed ONLINE/DESTROYED/REBUILDING lifecycle,
  BORROWED_WORLD or GENERATED_CAMPAIGN ownership, one reciprocal active-mission
  lock, typed transition fingerprints, revision, timestamps, and destruction/
  rebuild receipts. A process-local entity handle is only a projection; losing
  it is never destruction evidence. The stable site target ID is separate from
  the unique deterministic physical runtime-entity ID assigned to each exact
  mission.

- The radio lifecycle service is the only transmitter projection owner. It
  adopts one unambiguous authored transmitter without taking deletion
  ownership, freezes that binding, and prevents zone composition or generic
  mission runtime from creating, repairing, completing, or deleting an exact
  radio target. Ambiguous authored candidates quarantine before world mutation.
  DESTROYED authored targets have their damage state reapplied after streaming
  or restart. A replacement transmitter is campaign-generated only after a
  rebuild actually finishes; the active stop-rebuild mission projects separate
  construction equipment rather than a second intact tower. The supported
  authored transmitter has retained multiphase damage behavior, so borrowing,
  destruction, rebind, and reset do not depend on deleting or replacing its
  world identity. Permanent generated ONLINE projections disable verbose
  witness logging and keep the nearby-entity witness query dormant until an
  exact asset/mission/role identity is configured.

- Only newly started `destroy_radio_tower` and
  `dynamic_stop_tower_rebuild` missions opt into radio-site contract `1`.
  Destroy admission requires a resolved ONLINE site and records ONLINE to
  ONLINE; physical destruction then commits ONLINE to DESTROYED. Stop-rebuild
  admission is the production DESTROYED to REBUILDING transition. Its success
  returns to DESTROYED and records one rebuild-attempt receipt without changing
  the tower destruction receipt or epoch. Only one stop-rebuild attempt is
  accepted per destruction epoch. Failure, expiry, or campaign stop completes
  the rebuild as ONLINE with GENERATED_CAMPAIGN ownership and a rebuild receipt.
  Normal and forced/debug starts share the same lifecycle gate, and generic
  objective progress or direct sabotage cannot manufacture success.

- Borrowed authored-target destroy reports are accepted only when the mission
  and site hold reciprocal active lock/revision state, the tracked damage manager
  is authoritatively DESTROYED, and its position matches the frozen binding.
  Generated-target explosive reports additionally require a matching live
  mission-asset component, a bounded projection position, and a durable unique
  evidence key in a persisted bounded dedupe set. Each mission asset snapshots
  the ownership and authored descriptor present at admission, so later ownership
  handoff cannot relabel historical demolition evidence. Raw damage-state
  completion is restricted to the borrowed authored destroy target; generated
  transmitter/equipment completion requires exact
  explosive-score evidence. New-campaign reset reacquires and restores the
  authored transmitter before state replacement or fails closed. Physical
  destroy/heal/rollback writes are verified after mutation. The immutable
  authored match stays within 0.75 meters; physical projection evidence allows
  the bounded 12-meter safe-ground offset.

- A borrowed projection that is temporarily absent keeps its reciprocal active
  aggregate but clears mission/asset/runtime spawned flags and records
  `radio_site_projection_pending`; already-destroyed evidence preserves
  `radio_site_target_destroyed`. Missing, duplicate, or cross-linked runtime
  rows quarantine instead of waiting out the mission. Generic runtime,
  composition, objective ticking, commander progress, and generic failure
  settlement skip exact and quarantined radio authority.

- Every accepted admission or outcome increments the site revision and copies
  it to the mission. An immediate retry of the same deterministic request and
  typed fingerprint is already-applied without mutation; changed fingerprints
  conflict, and an old request after a later lifecycle cycle is rejected by the
  stale revision. Only resolved exact ONLINE sites emit town
  `radio_broadcast` influence. DESTROYED, REBUILDING, unresolved, missing, and
  quarantined sites emit none, and income evaluates this state after lifecycle
  reconciliation.

- Restores from schema 58 or earlier record
  `migration_schema59_radio_site_authority`. They create one ONLINE/UNRESOLVED
  logical row per radio zone without inferring a physical binding, destruction,
  rebuild, mission outcome, receipt, or reward. Active legacy radio missions
  fail closed at contract `0`; terminal historical rows remain contract `0`.
  Current-schema duplicate identities, cross-site request/receipt reuse, remote
  or unsupported bindings, illegal lifecycle/outcome shapes, bad timestamps,
  and broken mission/site/asset revisions quarantine at `-59` and record
  `normalization_schema59_radio_site_authority_conflict`. Generated ONLINE
  restore requires a destruction receipt followed by a completed-rebuild
  receipt. Quarantine fails and cleans a corrupt current linked aggregate while
  preserving coherent already-terminal historical outcome semantics.

- A compatibility normalization also accepts the one legacy terminal-objective
  projection emitted by earlier valid Schema 59 through 70 builds: an exact
  FAILED/EXPIRED radio mission whose runtime, asset, task, site, typed transition,
  and receipt authority are already coherently settled, but whose objective still
  has both `m_bFailed` and `m_bCleanupComplete` false. Restore sets only those two
  objective metadata fields and records
  `normalization_schema59_terminal_radio_objective_compatibility`; it never
  infers destruction, a lifecycle outcome, receipt, reward, or task result. Any
  incomplete, active, physically contradictory, duplicate, or otherwise
  malformed aggregate remains on the normal fail-closed quarantine path.

- Source proof and Workbench compilation cover state transitions, replay/stale
  rejection, ownership handoff, migration/roundtrip/quarantine, generic-owner
  isolation, influence suppression, marker/UI status, and coordinator ordering.
  The proof invokes production admission/outcome transitions through projection-
  only seams. It also exercises the production durable-evidence helper, rejects
  a direct second rebuild admission in the same epoch, and proves linked
  quarantine cleanup. Native authored-entity discovery, actual explosive destruction and
  damage-state reapplication, generated replacement visuals, streaming, real
  process restart, rendered UI, packaged networking, reconnect, and JIP remain
  packaged-runtime gates until a republished run supplies evidence.
  The stamped Schema-59 source/Workbench checkpoint identifies implementation
  `37fb5f0ffbc80c4bba3151ba1e5d8be6ffcf8a21` with build label
  `schema59-radio-site-lifecycle`. The full Foundation gate passes. Final
  Workbench validation loaded 5,773 files/11,608 classes with CRC `96914c26` and
  `Script validation successful`; a bounded normal open stayed alive/responding
  for 10/10 samples over 20 seconds with no script-compile/crash signature. Its
  one Steamworks stats-request error was nonfatal.

## Schema 58

- Schema 58 opts in only a newly started `rescue_pows` mission. The mission and
  reciprocal `HST_OPERATION_TYPE_MISSION_RESCUE` operation use contract `1`,
  manifest policy `exact_rescue_pows_v1`, guard intent `rescue_pows_guard`,
  projection contract `1`, and malformed-current quarantine version `-58`. It is
  the ninth explicit family consumer across the seventh operation type.
  Historical/pre-58 `rescue_pows`, `rescue_refugees`, generic captive rows, and
  every other unsupported family remain contract `0`.

- One operation owns one frozen composite manifest. Its executable subgraph is
  one catalog-backed hostile infantry root plus the exact ordered member slots.
  Its externally managed asset subgraph is exactly three required `captive`
  slots. The SpawnQueue batch records external asset authority and contains only
  group/member result rows; the mission-rescue service owns the three typed
  `HST_MissionAssetState` captive rows. No prefab-name manpower estimate,
  generic mission guard, vehicle slot, route, resource debit, refund, or generic
  asset-spawn result becomes authority.

- Captives persist an ordinal and typed UNKNOWN/HELD/FREED/FOLLOWING/BOARDING/
  BOARDED/EXTRACTED/KILLED disposition plus stable escort identity, carrier
  vehicle identity, seat token, last command result, a bounded typed
  command ledger (request, actor, command, result, and recorded revision),
  casualty receipt, extraction receipt, projection identity, transition second,
  revision, and projection generation. The mission and all three captives also
  retain one frozen HQ extraction position and one common base deadline.
  Process-local entity handles and compatibility carrier strings are cleared on
  restore. A missing projection is never death evidence; only observed
  authoritative damage state can record KILLED.

- HELD and FREED captives can fold to durable state outside the player/render
  bubble and reproject at their last authoritative position. FOLLOWING,
  BOARDING, and BOARDED remain projected because they are bound to a stable
  escort/carrier. Carrier discovery requires a connected stable escort and a
  replication-backed/runtime-tracked vehicle identity; the position-derived
  local fallback is rejected. Native seat evidence advances BOARDING to BOARDED.
  Outside grace, a disconnected escort releases custody to FREED without death;
  during grace it invalidates the frozen custody set. Terminal non-success
  settlement likewise releases surviving custody rows to FREED, so historical
  rescue validity never depends on a later mutable carrier-vehicle row.

- Guard elimination does not settle the rescue. Any one of the three required
  casualty receipts fails the mission; exactly three unique extraction receipts
  at the frozen HQ extraction point complete it through the existing mission
  outcome/reward path. The target town receives the configured support reward
  once when applicable. Timer expiry grants exactly 300 seconds of extraction
  grace only when all three living captives were already in stable custody; the
  grace accepts no new release or escort claim and cannot produce both expiry
  and success. Its end is always the frozen base deadline plus 300 seconds, so
  delayed ticks do not change durable timing authority. HQ relocation is blocked
  while that frozen extraction authority is open; settled history remains valid
  after a later HQ move.

- Restores from schema 57 or earlier record
  `migration_schema58_exact_rescue_pows`. They preserve historical POW missions,
  legacy `mission_group_*` guards, legacy captive actions, and rewards without
  inventing an exact mission, operation, manifest, guard roster, captive
  identity, carrier, seat, casualty, extraction, settlement, or reward receipt.

- Current-schema validation requires unique reciprocal mission/operation/
  manifest/batch/group identity, a valid generated mission site on the target
  zone, exact target/site position, one hostile catalog roster, three captive
  slot/row bijections, legal typed transitions, coherent grace and settlement,
  and no foreign resource/order/quote links. Valid open rows normalize only
  process bindings; command IDs are unique across exact captive rows,
  actor/command fingerprints and recorded revisions must match, extraction rows
  must be inside the frozen radius, and terminal mission/result combinations are
  exact. Compact settled rows may omit their terminal batch and guard group.
  Conflicts become `-58`, record
  `normalization_schema58_exact_rescue_pows_conflict`, and remain diagnostic
  without legacy fallback, guessed casualty, invented extraction, reward, or
  force transfer.

- Source proof covers exact admission/family isolation, composite manifest and
  external asset execution, captive transition/idempotency and carrier/seat
  evidence, render fold/re-entry, casualty/success/grace settlement, save
  roundtrip/migration, corruption quarantine, and state-derived UI/marker text.
  Schema 58 is an earlier stamped source/Workbench baseline at implementation
  `f0ba07ff2bc295d12542a3ea34b4c913e99b1869` with build label
  `schema58-exact-rescue-pows`. The full foundation gate passes. Final stamped-
  tree Workbench Game validation loaded 5,770 files/11,594 classes with CRC
  `aa73883a` and `Script validation successful`; the bounded hidden normal
  WorldEditor open stayed responsive for 10/10 samples over 20 seconds with zero
  crash/error matches. Native entity behavior, actual process restart, rendered
  UI, owner change, campaign setup, packaged networking, reconnect, and JIP
  remain open until a republished runtime run supplies those artifacts.

## Schema 57

- Schema 57 opts in only guard infantry belonging to a newly started
  `assassinate_specops` mission. The mission and reciprocal
  `HST_OPERATION_TYPE_MISSION_GUARD` operation use contract `3`; the frozen
  manifest policy is `exact_assassinate_specops_guard_v1`, the mission-family
  intent is `assassinate_specops_guard`, and malformed current authority uses
  quarantine version `-57`. Newly started officer and traitor guards retain
  contracts `1` and `2`, policies `exact_assassinate_officer_guard_v1` and
  `exact_assassinate_traitor_guard_v1`, and quarantine versions `-55` and `-56`.
  Historical or pre-schema-57 spec-ops missions and every unsupported mission
  family remain contract `0` (contract zero).

- Generic spec-ops composition may propose multiple groups. Contract `3`
  deterministically selects the strongest executable group, keeps the stable
  first group on ties, and freezes only that selected catalog roster. Discarded
  candidate groups never become mission, operation, manifest, batch, active-
  group, or restore authority.

- The contract-3 spec-ops aggregate reuses the exact assassination-guard shape:
  one catalog-backed `NotSpawned` empty execution root, exact ordered infantry
  member slots, one held SpawnQueue batch, and one mission-owned active group.
  It owns no route, generated cursor, vehicle, projected asset, virtual-combat
  clock, or resource cost. The HVT stays separate mission-objective/runtime-
  asset authority and is never a manifest member, operation asset, or group
  backlink.

- Durable member slots remain the sole guard-strength authority. Materialization
  releases only living slots; mapped physical deaths retire those exact slots;
  fold and re-entry preserve the same survivor set and last confirmed position.
  There is no virtual guard combat or inferred casualty. Exact and quarantined
  officer/traitor/spec-ops guard claimants are excluded from generic mission-
  group, garrison, route, survivor-repair, captured-zone, and cleanup owners.
  Ordinary historical `mission_group_*` rows are not exact claimants and gain no
  authority merely from a mission ID or matching instance backlink.

- Guard and HVT outcomes remain independent. All spec-ops guards dead settles
  the guard operation `DESTROYED` while its HVT mission may remain active. HVT
  success settles surviving guards `COMPLETED`; mission failure/expiry and
  campaign stop/setup settle `CANCELLED`; target-owner change settles
  `INVALIDATED`; coherent spawn/assignment failure settles `SPAWN_FAILED`.
  Every terminal path terminalizes its typed queue authority, records
  `exact_mission_guard_terminal` once, transfers no survivors, and applies a
  zero refund.

- Restores from schema 56 or earlier record
  `migration_schema57_exact_specops_guard`. Migration preserves coherent
  Schema-55 officer and Schema-56 traitor exact authority, while historical/
  pre-57 spec-ops missions, HVTs, objectives, and ordinary `mission_group_*`
  rows remain contract zero. It invents no manifest, roster, casualty,
  operation, projection, settlement, or claimant identity from mission IDs or
  aggregate counts.

- Current-schema validation dispatches by mission family and contract. A valid
  spec-ops graph requires unique reciprocal mission/operation/manifest/batch/
  group identity, policy `exact_assassinate_specops_guard_v1`, intent
  `assassinate_specops_guard`, exact empty-root/member bijection, offset
  stationary assignment, zero route/vehicle/asset/resource authority, legal
  projection state, separate HVT ownership, and the fixed terminal receipt.
  Coherent physical rows normalize to held survivors; compact settled graphs may
  omit their batch, group, and terminal HVT runtime rows.

- Malformed current spec-ops authority becomes contract `-57` and records
  `normalization_schema57_exact_specops_guard_conflict`. Quarantine is
  diagnostic and non-operational: it creates no legacy fallback, guessed
  casualty, HVT backlink, mission failure, refund, or force transfer. Existing
  HVT marker/UI rows append roster-authoritative guard status instead of adding
  a duplicate marker.

- `HST_SpecOpsGuardOperationProofService` covers six source-proof categories:
  admission/family isolation, survivor projection and HVT separation, typed
  zero-refund settlement, restore/migration, corruption quarantine, and
  existing-HVT marker status. Schema 57 is stamped at implementation
  `514ebdcbeb1ddfb2a383b19590382517113e2ff6` with build label
  `schema57-exact-specops-guard`. The full foundation gate passes, including
  Schema-55/56/57. Stamped Workbench Game validation loaded 5,765 files/11,576
  classes with CRC `e0b8578e` and `Script validation successful`; the bounded
  hidden normal WorldEditor open stayed alive for 10/10 samples over 20 seconds,
  and its log had no script-error/crash signature. Native entities, adapter
  casualties, actual save/restart, rendered UI, owner-change, campaign setup,
  packaged networking, reconnect, and JIP remain open.

- Schema 57 exhausts the assassination-guard family and opts in no rescue
  authority. Schema 58, documented above, is the next separate successor
  cutover for newly started `rescue_pows` only.

## Schema 56

- Schema 56 opts in only guard infantry belonging to a newly started
  `assassinate_traitor` mission. The mission and reciprocal
  `HST_OPERATION_TYPE_MISSION_GUARD` operation use contract `2`; the
  frozen manifest policy is `exact_assassinate_traitor_guard_v1`, and malformed
  current authority uses quarantine version `-56`. Newly started Schema-55
  `assassinate_officer` guards remain exact at contract `1` with `-55`
  quarantine. Historical or pre-schema-56 traitor missions,
  `assassinate_specops`, and every other unsupported mission family remain
  contract `0` (contract zero).

- The contract-2 traitor aggregate deliberately reuses the proven mission-guard
  shape: one catalog-backed `NotSpawned` empty execution root, exact ordered
  infantry member slots, one held SpawnQueue batch, and one mission-owned active
  group. It owns no route, generated cursor, vehicle, projected asset, or
  resource cost. The HVT remains separate mission-objective/runtime-asset
  authority and is never a manifest member, operation asset, or group backlink.

- The held member slots remain the only guard-strength authority. Near-player
  materialization releases only durable living members; mapped physical deaths
  retire their exact slots. Fold retires process-local projection ownership and
  requeues the same survivors at their last confirmed position. Re-entry realizes
  only those survivors. There is no virtual guard combat or inferred casualty.
  Generic mission-group, garrison, route, survivor-repair, captured-zone, and
  cleanup paths exclude exact and quarantined officer/traitor guard claimants.

- Guard and HVT outcomes remain independent. All traitor guards dead settles the
  guard operation `DESTROYED` while its HVT mission may remain active. HVT success
  settles surviving guards `COMPLETED`; mission failure/expiry and campaign stop/
  setup settle `CANCELLED`; target owner change settles `INVALIDATED`; coherent
  spawn/assignment failure settles `SPAWN_FAILED`. Every terminal path reconciles
  typed runtime authority, terminalizes its queue batch, records
  `exact_mission_guard_terminal` once, and refunds/transfers zero resources or
  legacy force. This is a zero refund policy on every terminal path.

- Restores from schema 55 or earlier record
  `migration_schema56_exact_traitor_guard`. Migration preserves coherent
  Schema-55 officer contract-1 authority exactly, while historical/pre-56
  traitor missions, HVTs, objectives, and legacy `mission_group_*` guards remain
  contract `0`. It never invents a traitor manifest, roster, casualty, operation,
  projection, or settlement from mission IDs or aggregate counts.

- Current-schema validation dispatches by mission family and contract. A valid
  traitor graph requires unique deterministic mission/operation/manifest/batch/
  group identity, policy `exact_assassinate_traitor_guard_v1`, exact empty-root/
  member bijection, offset stationary assignment, zero route/vehicle/asset/
  resource authority, legal virtual/physical state, separate HVT ownership, and
  the fixed terminal receipt. Coherent physical rows normalize to held survivors.
  Compact settled graphs may omit batch, group, and terminal HVT runtime rows; a
  settled `DESTROYED` guard may coexist with an active HVT mission.

- Malformed current traitor authority becomes contract `-56` and records
  `normalization_schema56_exact_traitor_guard_conflict`. Quarantine is
  non-operational and creates no legacy fallback, guessed casualty, HVT backlink,
  mission failure, refund, or force transfer. Existing HVT map/UI rows append
  roster-authoritative `guards N`, `guards neutralized`, or `guard authority
  unavailable`; no duplicate guard marker is published. Officer authority keeps
  its Schema-55 contract/policy/quarantine semantics alongside the new family.

- `HST_TraitorGuardOperationProofService` covers six source-proof categories:
  admission/isolation, survivor projection and HVT separation, typed settlement,
  restore/migration, corruption quarantine, and existing-HVT marker status. It
  also checks officer contract-1 coexistence and legacy traitor/spec-ops
  isolation. Native entities, real adapter handles/casualties, actual save/
  restart, rendered UI, owner-change, campaign setup, packaged networking,
  reconnect, and JIP remain unclaimed. The stamped Schema-56 implementation is
  `bab5748d817ba434dae701cfbb3b92805d463678`, build label
  `schema56-exact-traitor-guard`. It passes the full foundation gate; Workbench
  Game validation loaded 5,764 files/11,573 classes with CRC `a18c67a5` and
  `Script validation successful`. Its bounded hidden normal WorldEditor open
  stayed alive for all ten samples over 20 seconds and the latest log had no
  script-error/crash signature. These are source/Workbench gates only.

- Schema 57 later opts in newly started spec-ops guards through contract `3`.
  This Schema-56 section remains the stamped historical traitor-only boundary
  and does not retroactively convert pre-57 spec-ops missions.

## Schema 55

- Schema 55 opts in only guard infantry belonging to a newly started
  `assassinate_officer` mission. The mission contract and reciprocal
  `HST_OPERATION_TYPE_MISSION_GUARD` operation use version `1`; the frozen
  manifest policy is `exact_assassinate_officer_guard_v1`. Historical officer
  missions, `assassinate_traitor`, `assassinate_specops`, and every other mission
  family remain contract version `0`. The HVT remains a mission objective and
  mission-runtime asset: it is never a manifest member, projected asset, or
  operation-owned entity.

- Admission freezes one catalog-backed `NotSpawned` execution root and every
  ordered infantry member slot, with no vehicles, projected assets, money, HR,
  equipment, attack-resource, or support-resource cost. The mission, operation,
  manifest, held SpawnQueue batch, and mission-owned active group form one
  reciprocal aggregate. A deterministic guard anchor is offset from the HVT so
  the root and its members do not overlap the target; the HVT position remains
  the immutable tactical target. No route or generated-route cursor is created.

- The held roster is the only guard-strength authority. Entering the
  materialization radius releases the root and durable living member slots;
  mapped physical deaths retire only their member slots. Contact and recent
  casualties hold the physical projection. Leaving the larger radius retires
  the runtime projection and requeues the same survivors at their last confirmed
  position. Re-entry realizes only those survivors. Generic mission-group,
  zone-garrison, survivor-repair, route, captured-zone, and cleanup paths exclude
  exact and quarantined guard claimants.

- Guard elimination and HVT outcome remain independent. If every guard member
  dies, the operation settles `DESTROYED` with zero survivors while the active
  HVT objective remains playable. HVT success settles surviving guards
  `COMPLETED`; mission failure or expiry and campaign stop/setup settle
  `CANCELLED`; coherent spawn/assignment failure settles `SPAWN_FAILED`; target
  owner change settles `INVALIDATED`. Every terminal path first reconciles and
  retires typed runtime authority, terminalizes the queue batch, records the
  fixed settlement kind `exact_mission_guard_terminal` once, and applies zero
  refund or legacy-force transfer.

- Restores from schema 54 or earlier record
  `migration_schema55_exact_mission_guard` and preserve historical officer
  missions, HVTs, objectives, and `mission_group_*` guards exactly as legacy
  contract-zero state. Migration never infers a manifest, member roster,
  casualty, operation, projection, or settlement from aggregate counts or an
  officer mission ID. Unsupported pre-schema-55 exact-shaped operations are
  retained as quarantined evidence instead of being adopted.

- Current-schema validation requires unique deterministic mission/operation/
  manifest/batch/group identity, an exact empty-root/member bijection, the
  offset stationary assignment, zero vehicle/asset/resource authority, legal
  virtual/physical state, and the fixed terminal receipt. Coherent physical
  state restores to one held survivor roster with process-local IDs cleared.
  Compact settled graphs may legitimately have already removed their batch,
  group, and terminal HVT runtime row. A settled `DESTROYED` guard may coexist
  with an active HVT mission; other terminal outcomes require a non-active
  mission.

- Malformed current authority becomes contract version `-55`. Quarantined rows
  are non-operational and never create a legacy fallback, guessed casualty,
  refund, or HVT ownership backlink. Strongly owned successful projections use
  typed casualty reconciliation and projection-key cleanup; foreign or
  ambiguous rows remain untouched. This boundary records
  `normalization_schema55_exact_mission_guard_conflict`. Persistence defers while materialization is
  incomplete or a live/quarantined binding cannot be proven. Existing HVT map
  and mission UI rows append roster-authoritative `guards N`, `guards
  neutralized`, or `guard authority unavailable`; no duplicate guard marker is
  published.

- Deterministic source proofs cover admission/replay/rollback and legacy
  isolation, survivor/HVT separation, typed terminal mapping, restore/migration,
  corruption quarantine, and marker-status projection. Native entity creation,
  real adapter bindings and casualties, physical return-to-anchor behavior,
  actual process restart, rendered UI, packaged multiplayer, reconnect, and JIP
  remain open until a scoped packaged run proves them. Owner-change and campaign-
  setup settlement are also open engine/runtime gates. The stamped Schema-55
  tree identifies implementation `552c2c4ff5ac7608fa248c614480a254769b61a4`,
  passes the full foundation gate and clean Workbench Game validation at 5,763
  files/11,570 classes with CRC `0ec8950e`, and survives a ten-sample/20-second
  normal WorldEditor open. Those results are not packaged behavior proof.

- Schema 56 later opts in newly started traitor guards through their own
  contract-2 policy. This Schema-55 section remains the historical officer-only
  boundary and does not retroactively convert pre-56 traitor missions.

## Schema 54

- Schema 54 opts in only a newly issued, newly accepted policy-v2 purchased
  resistance garrison. Its manifest policy is `garrison_exact_patrol_2`; its
  canonical operation type is `HST_OPERATION_TYPE_GARRISON_PATROL` at contract
  version `1`. Admission freezes one executable `NotSpawned` group root that
  contributes no self-populated soldiers plus the arbitrary ordered infantry
  member slots selected and priced by the quote. The accepted quote, manifest,
  resource transactions, garrison backlink, operation, held SpawnQueue batch,
  generated local route, and active-group projection form one reciprocal graph.
  The legacy aggregate infantry count is not incremented for this policy; exact
  living slots reserve garrison capacity through their accepted-manifest link.

- The exact purchased roster begins in strategic hold and remains assigned to
  its immutable resistance-owned zone. Its four-point local generated route uses
  the shared persisted waypoint/lap/leg cursor as an infinite on-station patrol
  loop. Virtual ticks continue that route outside the render bubble. Entering
  the materialize-in radius releases the empty root plus only durable living
  member slots; leaving the larger radius after the casualty/contact hold folds
  the authoritative live position and survivors back into the same held batch.
  The adapter roster check now compares current infantry to the durable living
  slot count after first handoff, so a partial survivor roster can rematerialize
  without restoring the original purchased count. Confirmed dead slots never
  return.

- `HST_PhysicalWarService` recognizes `exact_garrison_patrol` ownership and
  excludes those groups from legacy aggregate zone activation, broad composition,
  garrison patrol assignment, population repair, survivor fold, route mutation,
  and cleanup. Legacy PhysicalWar remains the sole owner for initial-map, enemy,
  historical aggregate, vehicle, and other non-policy-v2 garrisons. Schema 54 is
  an infantry-only single-root cutover, not generic vehicle or multi-root
  garrison realization.

- This purchase has a zero refund policy: no survivor or terminal refund. Owner
  change, exact all-dead authority, campaign stop, setup, or typed spawn/route
  failure retires runtime ownership and records the
  deterministic settlement kind `exact_garrison_patrol_terminal` exactly once.
  Fold, rematerialization, quarantine, and terminal cleanup never mutate the
  committed money or HR transactions. Settlement removes the live garrison
  backlink only after adapter and PhysicalWar ownership are safely retired.

- Every save restored from schema 53 or earlier preserves accepted policy-v1
  purchases and all initial/enemy aggregate garrisons on their existing legacy
  representation. Migration records `migration_schema54_exact_garrison_patrol`
  and deliberately creates no exact operation, roster, batch, group, route,
  casualty, or settlement identity for historical rows. A pre-schema-54 save
  that somehow contains an exact garrison-patrol operation is unsupported and is
  retained as quarantine evidence rather than executed or converted to an
  aggregate count.

- A current-schema policy-v2 graph must validate unique quote/manifest/garrison/
  operation/batch/group/route identity, exact policy and hash, member-only
  casualty tombstones, legal lifecycle state, and terminal receipt. Coherent
  open physical-shaped state clears process-local handles and resumes as one
  held virtual survivor roster with the same local route cursor. Malformed,
  conflicting, non-unique, or unsupported current authority becomes contract
  version `-54`; it cannot fall into legacy activation, invent a casualty, or
  refund the purchase. Capture remains deferred while adapter or PhysicalWar
  runtime ownership survives, after which process-local residue may be cleared
  while the diagnostic graph remains durable.

- Persistence defers capture while an exact patrol is still materializing so a
  partial, not-yet-handed-off spawn wave cannot become durable or resurrect an
  unrecorded death. Once fully physical or dematerializing, persistence
  reconciles member deaths, exact binding cardinality, and live position before
  capture. An unexplained missing or aliased binding defers the checkpoint
  before stale tracked state is flushed.
  One roster-authoritative operation marker follows the virtual/live patrol and
  reports location, owner, role, and survivor count; Forces UI reports exact
  patrol infantry separately from legacy aggregate infantry. Nine deterministic
  `garrison_patrol.*` assertions cover admission, replay/rollback, roster
  projection, route loop, projection hold, settlement, restore, corruption, and
  marker lifecycle. These fixtures are source evidence. The stamped Schema-54
  tree identifies implementation `09a1470a4c27dbef866e8cbdba182a7df65fa027`,
  clean Workbench Game compile/create at 5,760 files/11,560 classes with CRC
  `c62de929`, and a ten-sample/20-second normal WorldEditor open. Packaged native
  waypoint movement,
  casualty observation, fold/rematerialization, save/restart, marker rendering,
  network replication, reconnect, and JIP proof remain open.

## Schema 53

- Schema 53 adds one canonical operation consumer only: an enemy `PATROL` order
  newly queued under the current schema. Its order contract version is `1`; the
  reciprocal operation type is `HST_OPERATION_TYPE_ENEMY_PATROL`. Admission
  freezes one executable infantry group root and every ordered member slot,
  links one generated route/order/operation/manifest/held-batch/active-group
  aggregate, and records the exact proactive attack-resource debit.

- The operation uses the generic generated-route cursor fields on
  `HST_OperationRecordState`: ordered waypoint index, lap count, leg sequence,
  loop start/completion seconds, route contract hash, strategic position, and
  bounded update clock. Outbound travel reaches route index zero, one closed lap
  advances through the ordered positions and returns to index zero, and the
  return-to-origin leg is represented separately with waypoint index `-1`.
  Virtual ticks advance that same cursor; physical fold rebases the current leg
  from the authoritative live position without changing aggregate identity.

- Exact patrols reuse the one-root infantry SpawnQueue/adapter boundary. The
  held member slots remain the living/casualty authority while virtual,
  materializing, physical, folded, and restored. A mapped physical casualty
  retires only that slot. Leaving the render bubble folds the surviving roster
  and restarts the current generated-route leg; re-entry realizes only survivors.
  Physical contact records a hold and consumes the route clock until a clear
  transition restarts the frozen current leg, so delayed virtual catch-up cannot
  skip time spent in contact.
  Every physical tick, fold, checkpoint, and campaign-stop settlement first
  proves a unique root, a unique live handle for each durable survivor, exact
  result/projection/slot ownership, non-aliased entity identity, and matching
  PhysicalWar membership. An unexplained deleted binding remains unresolved and
  cannot be converted into a casualty, refund, fold, or saved survivor.

- Resource ownership is distinct from the defensive-QRF ledger. Patrol admission
  spends proactive attack resources once. A post-debit admission rejection
  refunds the full debit once; after commitment, return or terminal settlement
  refunds only the integer survivor fraction once. Replays reuse the persisted
  settlement identity and cannot debit or refund twice. Fold/materialization
  never owns a refund.

- Enemy-order runtime dispatch now uses both order type and contract version.
  Exact patrol version `1`, exact defensive-QRF version `1`, legacy version `0`,
  quarantine, and unsupported nonzero rows have distinct owners. A current-
  schema patrol whose route, backlinks, roster, lifecycle, or settlement receipt
  is incomplete/conflicting is retained as diagnostic evidence under contract
  version `-53`; it never falls back to the legacy patrol timer or another exact
  owner. Admission scans secondary claimant identities and rollback removes only
  rows inserted by that attempt, so a foreign collision is retained unchanged.

- Every patrol restored from schema 52 or earlier remains contract version `0`.
  Migration records `migration_schema53_enemy_patrol_authority` with transition
  `legacy_enemy_patrols_preserved_contract_zero`, but does not invent a route,
  roster, operation, debit, refund, batch, or group backlink for a historical
  order. Current-schema exact rows must validate as one unique reciprocal graph.
  A coherent open row clears process-local entity/native-group mappings and
  resumes as one held strategic projection with the same cursor and casualty
  roster; a coherent settled row retains its receipt while runtime handles are
  retired.

- Before any real save-data capture, persistence reconciles mapped physical
  and dematerializing exact-patrol casualties, proves the complete adapter/
  PhysicalWar binding graph, and requires an authoritative live patrol position.
  A conflicting/missing member mapping, runtime claimant, or unverifiable live
  position defers capture before a stale tracked snapshot is flushed or a
  savepoint is requested. Pending save intent is retained for bounded retry.
  Quarantined open or settled graphs remain unsaveable while any adapter or
  PhysicalWar runtime owner exists; after both registries are empty, only process-
  local residue is normalized and all diagnostic rows remain without refund.

- One exact-patrol marker follows the persisted virtual cursor or live physical
  position and reports the durable living roster. It is removed after terminal
  cleanup. Ten deterministic `enemy_patrol.*` assertions cover admission,
  collision-safe replay/refund, route loop/return, queue/roster bookkeeping,
  contact transition, settlement, physical-shaped restore, backlink corruption,
  type/version/priority isolation, and marker lifecycle. These fixtures and the
  final stamped implementation `8ab694fdf61e56c6a5e343782f2225660d3aeeb7`, clean headless Workbench
  compile/create at 5,757 files/11,550 classes with CRC `3232b15a`, plus a normal
  WorldEditor open that remained alive
  for ten samples over 20 seconds without a crash signature, are source/Workbench
  evidence only; packaged movement, contact, fold/reprojection, accounting,
  marker rendering, and process-restart proof remain open.

## Schema 52

- Schema 52 adds one new canonical operation consumer only: a convoy mission
  newly started under the current schema. Its `HST_ActiveMissionState` stores
  operation contract version `1` plus reciprocal operation, manifest, held
  spawn-result, and settlement IDs. The linked operation has type
  `HST_OPERATION_TYPE_MISSION_CONVOY` and keeps immutable mission/source/target
  identity, a persisted generated-road-route cursor, strategic/live position
  authority, materialization state, arrival samples, and terminal settlement.

- Admission is all-or-nothing. It freezes exactly three vehicle slots, three
  linked crew-group slots, every ordered crew-member slot, and at most one
  mission cargo/captive asset assigned to vehicle slot zero. A held roster
  batch preserves stable living/casualty slots while the convoy is virtual; it
  is not generic SpawnQueue permission to realize arbitrary vehicle or
  multi-root manifests.
  Failed admission resets its uncommitted contract and matching asset links;
  only a complete reciprocal version-1 authority graph is retained. Generic
  queue restore, cancellation, duplicate handling, and compaction exclude this
  held exact roster.
  Money and supplies require exactly one cargo row, prisoners require exactly
  one captive row, and ammo, armored, and reinforcement convoys forbid a
  separate cargo row. Admission rejects duplicate or incompatible role/kind
  rows and prefabs that do not resolve to a loadable mission-asset entity
  source. Captives must be boardable character prefabs with compartment access;
  ordinary payloads must be non-character entities. Current-schema restore
  reapplies this same contract and quarantines older malformed exact rows.

- Three `HST_ConvoyElementState` rows are the durable per-vehicle authority.
  Each row links the operation, mission, manifest vehicle slot, crew group,
  mission vehicle asset, optional cargo asset, and active-group ID. It preserves
  formation/current position, original and surviving crew count, vehicle
  damage/fuel/ammunition snapshot, physical/mobile flags, disposition, terminal
  reason, update time, and revision. Active groups and mission assets receive
  reciprocal convoy-element/vehicle-slot/assigned-carrier links so array order
  is no longer identity.

- The exact convoy advances virtually along its generated route, materializes
  near a player through the convoy-specific PhysicalWar path, and folds back
  only when clear of contact, player occupancy, cargo interaction, and pending
  population/seating work. Folding samples the latest provable route position,
  per-element state, and crew survivors without applying a casualty or mission
  outcome. The transfer preflights every root before mutating any of them.
  Confirmed dead member slots never return on rematerialization: exact slot-to-
  entity mappings retain a dead early seat while later seats and their frozen
  prefabs survive, instead of inferring casualties from an aggregate suffix. If a destroyed
  or captured vehicle still has living dismounted crew, only those exact crew
  slots reproject at the element position; the terminal vehicle stays absent.
  Bubble distance uses every separated living/recoverable root. Native
  population/defer zeros are not casualties; roster reduction requires a
  completed physical sample or durable elimination. A last-crew death during
  fold reclassifies the root crewless, and a terminal damage sample commits
  destruction instead of leaving an unusable abandoned vehicle.

- Arrival is no longer resolved by the legacy convoy timer for contract-version-
  1 missions. Virtual arrival follows the persisted route cursor; physical
  arrival requires two samples from distinct campaign seconds within the route-
  end radius. Existing mission-convoy outcome logic remains the once-only owner
  of reward/penalty/capture/delivery consequences, after which the operation
  records one typed settlement and retires its held roster.

- Crew elimination does not close an exact operation while its mission still
  owns unresolved recoverable cargo, captives, or vehicles. The operation enters
  an on-station recovery hold at the durable asset anchor, remains eligible for
  virtual fold/rematerialization, and settles only after the mission-specific
  delivery/capture/extraction outcome or another terminal mission result.

- Intact crewless vehicles persist as `ABANDONED` elements and can materialize
  or be captured without recreating crew while other convoy roots remain active.
  Their runtime entities stay unpublished until the whole projection becomes
  physical. Cargo/captives keep their frozen assigned-vehicle
  slot; a destroyed or captured carrier leaves the unresolved asset at its
  durable ground position for MissionRuntime recovery rather than reassigning
  it to another vehicle. Recovery proximity also considers secondary abandoned
  vehicles and detached cargo, not only one aggregate anchor. Exact vehicle
  capture reserves one garage row, unregisters the authoritative PhysicalWar
  vehicle handle, first detaches any frozen cargo/captive child, and deletes that
  handed-off world entity so a garage copy cannot coexist with the captured
  runtime root. Terminal ground cargo alone can materialize a recovery hold when
  no intact vehicle remains. Clean player-bound cargo no longer pins the convoy
  physical during delivery.

- Restore clears process-local vehicle/group handles and physical arrival
  samples, retains route progress, convoy-element state, cargo assignment, and
  exact crew casualty tombstones, then normalizes a coherent open operation to
  one virtual projection. Member-slot tombstones plus convoy-element survivors
  are canonical; derived active-group survivor counters are rebound only after
  the aggregate validates uniquely. Conflicting or incomplete current-schema
  authority fails closed instead of regenerating a partial convoy or falling
  back to the legacy timer. Disposition must agree with linked vehicle terminal
  facts. Quarantined version `-52` rows bypass generic convoy asset creation and
  repositioning until failed-mission cleanup, preserving corruption evidence.
  Casualty tombstones are legal only on member slots; open group, vehicle, and
  cargo roots remain registered noncasualty lifecycle authority. Open records
  must use an enumerated duty/resume and materialization/position pair.
  Missionless or partially unlinked exact-looking rows remain durable quarantine
  evidence after process-local cleanup instead of being deleted as generic
  inactive groups.
  Settled exact rows remain durable after runtime retirement, but one shared
  operational/combat-presence policy excludes settled and quarantined roots
  from generic spawning, survivor polling, zone pressure, threat, undercover,
  clear-area, spawn-placement, deterministic gameplay counts, and current-group
  UI. Stale process flags clear once after restore and do not churn revisions on
  later ticks.

- The route cursor owns outbound ETA both virtually and physically; legacy
  counter C remains disabled for exact convoys after staging. Exact recovery
  retains the aggregate current marker and suppresses legacy per-vehicle and
  separate outcome-marker identities while keeping one truthful aggregate
  destination for cargo or vehicle recovery. Supply cargo remains pending after
  crew elimination until its delivery/destruction outcome resolves.

- Every historical convoy mission restored from schema 51 or earlier remains
  operation contract version `0`. Migration deliberately does not invent a
  route, operation, manifest, held batch, vehicle slot, crew roster, cargo
  carrier, element row, or settlement receipt for it; the prior convoy runtime
  path continues to own those records.

- Before any real save-data capture, persistence synchronously reconciles each
  mapped physical exact-convoy member. An open outbound publication transaction,
  missing/conflicting mapping, or nonphysical operation retaining member
  mappings defers capture before an older snapshot is flushed or an engine
  savepoint is requested. Autosave/major-change intent remains pending and
  retries on the bounded cadence.

- The final stamped schema-52 tree identifies implementation
  `fa5e7e45dbd8741269e614e60c51d4edee6bf223`, passes repository validation, and
  passes a clean headless Workbench Game-module compile/create gate at 5,753 files/
  11,537 classes with CRC `e868739b`. A normal WorldEditor open created the same
  Game module and remained responsive for all 10 bounded samples without a
  script-error or native-crash signature. That is source/startup evidence, not
  packaged-runtime evidence.
  A republished server/client run must still prove three physical vehicles and
  crews, virtual travel, interception,
  casualty-preserving fold/rematerialization, arrival/outcome settlement,
  marker cleanup, and real process restart.

## Schema 51

- Schema 51 adds the durable authority kernel for one additional consumer only:
  newly admitted, exact infantry-only enemy defensive QRF orders of type
  `HST_ENEMY_ORDER_QRF`. A versioned order stores its immutable source zone,
  frozen manifest hash, operation/manifest/spawn/group backlinks, strategic
  service commit boundary, and an idempotent resource-settlement identity with
  accepted and surviving member counts. The reciprocal operation type and
  enemy-order backlink are explicit; the reciprocal active-group enemy-order
  backlink prevents restore normalization from reclassifying the projection as
  a garrison.

- The operation reuses schema 50's exact single-root infantry roster, direct
  route, virtual/physical materialization, and strategic position contracts.
  Enemy-QRF operation transitions add an on-station to return-origin success
  leg and typed `COMPLETED` terminal result. Physical arrival and physical
  return require two live-position confirmations from distinct campaign
  seconds. Those counters persist, reset whenever the route leg changes, and
  reset on restore so one process-local sample cannot be replayed after restart.
  The player-paid QRF settlement contract does not accept `COMPLETED`.

- Every pre-schema-51 enemy order is preserved at operation contract version
  `0`. Migration creates no source-zone claim, frozen roster, operation record,
  service commitment, resource settlement, or refund for legacy rows. It does
  not opt counterattacks, Petros attacks, roadblocks, patrols, rebuild orders,
  support calls, vehicle manifests, asset manifests, or multi-root forces into
  the new contract. A current-schema versioned row with incomplete or
  conflicting authority fails closed instead of falling back to the legacy
  timer or physical-support consumer; its evidence remains available for the
  runtime settlement owner.

## Schema 50

- Schema 50 advances the first canonical operation consumer—confirmed exact
  player-paid resistance infantry QRFs—from a lifecycle kernel to persistent
  strategic projection. The operation now stores a versioned direct-route
  cursor, route distance, conservative 2.5 m/s campaign speed, strategic
  position, bounded update clock, materialization decision evidence, and
  deterministic virtual-combat clock and damage carries. Quote ETA is derived
  from route distance and that speed. Strategic ticks catch up at no more than
  30 campaign seconds per invocation; no nearby player is required.

- The exact spawn batch can be held strategically. While held, its frozen
  manifest member slots are the exact living/dead roster and the physical spawn
  queue performs no work. Entering the configured player bubble releases the
  batch at the current route cursor. Leaving the larger materialize-out radius
  while clear of contact samples the live position and roster, retires only the
  disposable runtime projection, and requeues exactly the confirmed survivors.
  The fold boundary rebases the remaining direct route at that live position and
  resets the virtual-combat clock, so physical time is never replayed as abstract
  casualties. Abstract virtual contact is legally cleared during the v1 physical
  handoff until a physical contact adapter becomes authoritative.
  Projection deletion is neither a casualty nor a refund. Vehicles, cargo,
  assets, multi-root forces, and other support families deliberately fail closed
  or remain on their previous contracts.

- Virtual combat is initially limited to an on-station exact infantry QRF and
  the hostile abstract infantry garrison at its target. It advances in bounded
  30-second deterministic power steps, can retire at most one exact friendly
  member slot per processed step, and preserves its fractional damage carries.
  The contact clock resets on outbound-to-on-station arrival, so elapsed travel
  time cannot be backfilled as combat. Broader encounters, vehicle damage,
  ammunition, terrain modifiers, and capture outcomes remain future work.

- Pre-schema-50 coherent exact infantry-QRF operations receive this projection
  contract without changing economy or terminal history. Unsupported manifests
  remain operation-only. Every nonterminal projection restores as virtual with
  strategic position authority; pending, interrupted, ready, and previously
  successful physical batches become one held pending survivor projection with
  process-local IDs cleared. A successful physical batch records exactly one
  reprojection on that transition; already-held pending work does not. The saved
  live group position rebases the restored route before initialization, preventing
  a snap back to an older cursor. This prevents restart from duplicating physical
  entities. The linked support request is normalized at the same time:
  `m_bPhysicalized` is cleared, on-station duty publishes
  `exact_virtual_on_station`, and other open duty publishes
  `exact_restore_survivor_virtual`. Schema 50 also reclassifies the known woodland minor locality as a
  food resource and reduces its ambient civilian baseline while preserving any
  existing casualty ledger.

- Route preparation does not itself deliver the paid service. The operation may
  initialize its route while `STAGING`; `LinkOutboundVirtual` is the commit that
  advances it to `OUTBOUND`. Terminal failure before that commit refunds money
  and HR. Once committed, including `ON_STATION`, a materialization failure with
  no physical handoff retains money and refunds only
  `m_iLastVirtualFriendlyCount` HR. Both terminal paths are deterministic and
  idempotent.

- The same source boundary separates HQ clearances that previously shared one
  900-meter constant. Hostile operation staging retains that distance, location
  activation now uses the location capture footprint with a 150-meter fallback,
  and generated composition slots keep a 150-meter immediate clearance. This
  allows valid nearby locations to project without spawning entities on HQ.

- Schema 49 adds the first canonical `HST_OperationRecordState` contract for one
  already-exact consumer: confirmed player-paid resistance infantry QRFs. Quote
  issue retains the stable operation identity without creating an operation
  record. Successful confirmation creates exactly one version-1 record with
  immutable origin and assignment, mutable tactical target, duty, engagement,
  materialization, position-authority, settlement, terminal-result, policy,
  timing, and revision fields. Queue admission, physical handoff, arrival,
  restore reprojection, recall, and terminal settlement update that record
  through `HST_OperationService`; settlement replay is idempotent and a
  conflicting terminal result fails closed.

- Schema-48 migration backfills only uniquely coherent, accepted, nonterminal
  exact paid-QRF requests whose quote, manifest, and unique committed money/HR
  transaction authority agree, with optional queue/group links required to be
  unique and coherent when present. It changes
  no balance, ledger amount/status, request status, quote status, or manifest.
  Pre-exact requests, terminal history, ambiguous/incomplete rows, archived-only
  rows, and legacy `HST_QRFState` records remain operation contract version `0`
  rather than receiving invented authority. One bounded migration event records
  the created count and one conflict event records preserved candidates.

- Restored open operations never reacquire process-local physical authority.
  Saved `PHYSICAL` or `DEMATERIALIZING` records normalize to
  `MATERIALIZING` plus strategic position authority while retaining immutable
  assignment, current duty, and revision history until the exact adapter
  completes survivor reprojection. Once an accepted contract-version-1 QRF is
  terminal and backlink-free, schema-48 settlement archival additionally copies
  its operation contract version, settlement ID, revision, and typed terminal
  result into the compact tombstone and removes the full settled operation row.
  Packaged save/restart, migration, and archive replay remain runtime-unproven.

- Schema 48 adds bounded settlement archives for accepted exact-force planning
  history. An accepted garrison purchase or terminal paid QRF remains in full
  quote/manifest/ledger form for at least 600 campaign seconds and cannot compact
  while an active-group, enemy-order, or force-spawn backlink exists. Eligible
  rows compact atomically into one persisted tombstone containing quote,
  operation, actor, manifest-hash, aggregate, cost, refund, and transaction-status
  evidence. Issue, confirmation, and committed-ledger replay consult that compact
  row before any mutation or debit. Tombstones retain a minimum 86,400-second
  replay window, cap at 256 rows, and share a hard 320-row planning-history
  admission bound with full quotes. Pin-aware queue compaction now runs in the
  production coordinator so 128 retained terminal projections cannot deadlock
  later admission or permanently pin a settled manifest. Pre-schema-48 saves
  preserve every accepted row in full and initialize an empty archive; migration
  never invents a terminal settlement.

- The schema-48 support route-truth repair required no save-format bump,
  but it does reconcile pre-repair timer-derived states. An unspawned restored
  `support_arrived` row folds when it remains outside the player bubble or waits
  for physicalization inside it. A spawned arrived row without the new request-
  and-group proof provenance receives one current live-distance check, then is
  either stamped as proven or reopened as `support_active`. Spawned
  `support_recall_exited` likewise requires current live exit distance before
  retirement or HR settlement; eligible unspawned abstract recall remains valid.
  A succeeded exact QRF recalled while survivor reprojection is waiting for queue
  capacity now settles its durable survivors without inventing a physical exit,
  and legacy rows already stuck in unspawned `support_recalling` recover through
  the same once-only HR settlement when no root or adapter handles remain.

- The typed support-recall command repair also requires no save-format bump.
  `HST_SupportRecallResult` is transient; the existing support request,
  operation ID, command receipt, and ledger rows already persist the durable
  facts. New recall receipts derive applied/rejected status from the typed
  result and schedule persistence explicitly. Existing historical receipts are
  not reclassified. Exact paired full refunds now prevalidate both linked
  transaction identities, coherent refund state, settleability, and
  deterministic settlement IDs before either transaction mutates.

- Schema 47 adds the first durable exact-force runtime lifecycle for the paid
  infantry QRF path. Each successfully handed-off member slot now preserves
  ever-alive, casualty, retirement, timestamp, and revision evidence. The
  adapter samples authoritative life state before pruning transient handles,
  retires each dead slot idempotently, detaches the corpse from native/Game
  Master group ownership without deleting it, and updates the active group's
  durable living count. Once an ever-populated, spawn-completed roster reaches
  zero, the exact root and marker are retired and the support resolves without
  refunding dead HR or the already-delivered money cost. Successful paid-QRF
  restore clears process-local IDs and requeues one root plus only the durable
  surviving member slots; confirmed casualties remain retired. Reprojection
  failure retains the paid money and refunds only durable survivors. Pre-schema-
  47 successful batches gain one historical handoff plus ever-populated/living
  evidence from their registered slots. Migration never infers a casualty from
  a missing entity or aggregate count.

- Schema 46 adds the first exact player-paid support contract for resistance
  QRFs. Force quotes now persist support request/capability/asset identity,
  support type, ETA, cooldown, and expected war level beside the immutable
  manifest and transaction links. Confirmation reserves and commits the exact
  $250 money cost and one HR per authored catalog member, registers one linked
  support request, and marks the quote accepted last. The request then admits
  the same executable one-group manifest to SpawnQueue; terminal failure or
  cancellation refunds both transactions once, while recall uses the linked HR
  settlement policy. In schema 46, successful terminal projections that could
  not be restored failed closed with a full refund instead of inventing live
  runtime authority. Pre-schema-46 paid player-QRF rows keep their gameplay state and
  balances. Positive stored costs import as historical committed ledger rows
  without creating a quote, manifest, or new debit; stored HR refunds become
  historical partial/full refunds. Conflicting legacy transaction identity is
  preserved and summarized by a bounded migration warning rather than receiving
  an invented refund. Schema 47 supersedes schema 46's temporary full-refund
  fallback for successful paid-QRF restore with survivor-only reprojection.
- Schema 45 adds explicit force and projection identity fields to every active
  physical-group row and makes Game Master/editable-hierarchy verification
  mandatory for new successful spawn slots. Pre-schema-45 active groups derive
  missing IDs only from one spawn batch whose result, manifest, operation, and
  force links
  are conflict-free; ambiguous or missing links remain empty. Stable aggregate
  migration events record derived and unresolved linked rows without creating
  one event per group. Existing terminal spawn history remains historical even
  when it predates the new verification field. Cleanup work is acquired in
  dependency-safe asset, member, vehicle, then group order across bounded work
  waves. Once every exact slot is registered, the batch enters the durable,
  nonterminal `READY_FOR_HANDOFF` state. Physical-war finalization must succeed
  before `CompleteProjectionHandoff` records `SUCCEEDED`. Restoring a ready batch
  clears process-local evidence, advances its generation, and requeues exact
  realization; it never promotes an interrupted handoff to terminal success.
  Existing terminal successes remain historical and are not yet runtime-
  reprojected. Normal acquisition uses active campaign time, while setup and won/
  lost processing cancels nonterminal work and drains cleanup through a monotonic
  runtime-only clock without advancing the persisted campaign elapsed second.
- Schema 44 turns typed force-spawn results into durable, per-projection queue
  records. Each batch carries a unique request ID, immutable manifest-hash
  binding, projection ID, attempt generation, retry/deadline timestamps,
  cancellation intent, and explicit failure evidence. Each slot also preserves
  its spawned prefab and alive verification. Runtime queue limits are 64
  nonterminal batches, 512 nonterminal slots, 64 slots per request, and 128
  terminal rows; terminal compaction requires explicit backlink pins and a
  600-second minimum retention window. These queue bounds did not compact
  accepted quote/manifest/ledger settlement history until schema 48 added a
  separate accepted-aggregate archive.
  Pre-schema-44 nonterminal rows are never resumable, even if a partially
  populated save happens to contain newer identity fields, so migration
  finalizes them as explicit failures and records one migration event instead
  of inventing work. Existing terminal batches remain unchanged as historical
  evidence. A schema-44 nonterminal batch whose nonempty result, request, or
  projection key collides with any other row fails closed and emits one bounded
  normalization record. Campaign state also persists an actual-restore sequence
  and the last queue-reconciled sequence. Coordinator initialization reconciles
  once per actual restore before other authority recovery; nonterminal rows
  clear process-local evidence and advance generation, while terminal status,
  prefab, and verification history remain but entity/native-group IDs are
  cleared rather than reacquired as living authority.
- Schema 43 adds immutable force manifests, exact force quotes, typed spawn
  results, stable garrison IDs, and quote/manifest links on resource
  transactions and force-bearing aggregates. Legacy force counts remain
  unchanged and are explicitly recorded as unverified instead of receiving
  invented member slots, prices, or refunds. New visible garrison recruitment
  uses an expiring all-or-nothing server quote and persists its exact member
  slots and per-member money/HR cost before confirmation.
- Schema 42 adds the campaign authority foundation: a persisted monotonic ID
  sequence, stable operation links, bounded command receipts, resource
  transactions, and bounded campaign transition events. Legacy support,
  enemy-order, and active-group rows receive deterministic operation IDs from
  their existing durable IDs. Legacy resource changes are not retroactively
  invented as transactions. During coordinator initialization, any restored open
  reservation is cancelled and refunded once before normal service ticks; the
  reconciled state is then captured and tracked again.
- Schema 41 extends support request rows with selected roadblock garage vehicle
  id, prefab, display name, and consumed-state fields so established roadblock
  support survives save-data roundtrips with its consumed HQ vehicle evidence.
- Schema 40 adds durable gun shop mission state: generated shop item rows,
  seller/delivery asset ids, seller/delivery positions, purchase totals,
  purchase/delivery notice flags, and delivery runtime timing so purchased
  stock and delivery progress survive save-data copies.
- Schema 39 extends support request rows with player-support HR cost,
  planned infantry count, refunded HR, recall request timing, recall exit
  position, and recall-requested state so support recall/refund behavior
  survives save-data copies and active support rows remain inspectable.
- Schema 38 extends strategic-event rows with vehicle report before/after
  fields so runtime vehicle reports preserve vehicle runtime id, heat
  before/after/delta, reported before/after, and report-expiry deltas across
  save-data roundtrips.
- Schema 37 adds the durable strategic-event ledger so mission success/failure,
  mission-expiry, convoy-outcome, zone-capture, and support-near-HQ consequence
  rows preserve source identity, target zone/faction, applied status,
  before/after owner fields, and money/HR/support/capture/aggression/resource/
  HQ-knowledge deltas across save-data roundtrips.
- Schema 36 adds the selected active-group vehicle prefab so mixed
  infantry/vehicle response groups can respawn the same vehicle choice after a
  save/load roundtrip.
- Schema 35 adds persisted support deployment proof for physicalized support:
  deployment route id, placement type/summary, target/road/HQ distances,
  road resolution, vehicle-safe result, and whether vehicle-safe placement was
  required.
- Schema 34 adds persisted population-outcome metadata for campaign-end state:
  outcome mode, initial/remaining/killed population, FIA-supporting population,
  support percent, and controlled/total airfields.
- Schema 33 adds durable active-group source-link fields and original force
  count fields so support, mission, QRF, and garrison active groups can be
  traced through fold-back, cleanup, refunds, and save-data roundtrips.
- Schema 32 extends durable vehicle heat/report fields to garage vehicles so
  reported vehicle cover state survives capture, virtual storage, redeploy, and
  save-data roundtrips.
- Schema 31 adds durable runtime vehicle heat/report fields so reported
  civilian vehicles, passenger compromises, report expiry, and vehicle-cover
  eligibility remain inspectable across saves.
- Schema 30 adds durable town influence events plus town population and
  influence aggregate fields so aid, civilian casualties, security pressure,
  support-majority flips, and active/expired modifiers remain inspectable
  across saves.
- Schema 29 adds durable enemy support-spend ledgers and enemy-order refund
  stamps so QRF/support cooldowns, recent damage pressure, max-spend denials,
  and survivor fold-back refunds remain inspectable across saves.
- Schema 28 adds durable force-composition metadata to active groups, support
  requests, and enemy orders. Existing saves load with empty composition
  summaries and zero composition counts until the next force-planning event.
- Schema 27 adds a durable display-only player name field so member/commander
  roster UI can show readable player names while backend identity remains the
  authority for permissions and ownership.
- Schema 26 adds durable HQ spawn-point position/prefab fields and backfills
  older deployed HQ saves so the HQ runtime object rebuild can spawn and
  verify a physical respawn marker.
- Phase 24 balance, pacing, and campaign outcomes add durable campaign-end report fields and backfill legacy ended saves.
- Phase 23 UI and marker polish does not require a campaign schema bump because marker audits, command coverage, failed-action text, and menu summaries are derived from existing persisted state.
- Phase 22 HQ threat and Defend Petros add durable HQ threat diagnostics and active defense mission/order/support/group links.
- Phase 21 undercover enforcement and police/roadblocks add durable applied/enforcement state, detection score/source, compromise reason, and police/roadblock scan audit fields.
- Phase 20 civilians, town support, and undercover reports add durable town support values and detailed undercover eligibility reasons.
- Phase 19 support request lifecycle hardening adds durable support-request
  runtime fields while preserving runtime handles as non-persisted data.
- Phase 18 enemy commander physical responses add durable enemy-order
  runtime fields while preserving runtime handles as non-persisted data.
- The current save container captures campaign metadata, elapsed/save/restore
  counters, war resources, campaign-end state, HQ/Petros/cache/arsenal/tent/spawn-point
  fields, HQ threat/Defend Petros state, faction pools, players, zones,
  garrisons, active groups, QRFs, map markers, arsenal items, garage vehicles,
  vehicle cargo, runtime vehicles, saved loadouts, issued loadout items,
  captured emplacements, ammo points, active missions, generated sites/routes,
  mission objectives/runtime entities/assets, support requests, enemy orders,
  enemy support ledgers, civilian state, town influence events, strategic
  events, canonical exact-QRF operation records, immutable force manifests,
  force quotes, typed spawn results,
  undercover state, and campaign tasks.
- `HST_LoadoutEditorSessionState` records are runtime/editor state and are not
  copied into `HST_CampaignSaveData`; durable saved loadouts and issued-item
  ledgers are copied. At the Schema-43 checkpoint, personal templates were also
  written under the then-current, now-retired generated-profile loadout path with
  loadout file schema `2`. Current builds move that tree before consumers run and
  then write `$profile:Partisan/loadouts/v2`.
- Runtime settings are schema `22` and are migrated separately by
  `HST_RuntimeSettingsService`.
- Campaign save data is normally tracked through `PersistenceSystem`; when
  scripted persistence cannot flush, current builds write and restore the
  same-container data at `$profile:Partisan/HST_CampaignSaveData.json`. The
  Schema-43-era file lived under the now-retired generated-profile root. Current
  builds move it into the canonical tree, or archive it if canonical content
  differs, before persistence reads fallback state.
- Raw `IEntity`, `AIGroup`, waypoint, inventory-operation callback, and other
  runtime handles are not persisted as campaign truth.

## Schema 42

Campaign command authority and resource transaction foundation.

- `HST_CampaignState.SCHEMA_VERSION` is `42`.
- `m_iNextAuthoritySequence` allocates server-owned event and fallback request
  IDs without relying on elapsed time plus array length.
- `HST_CommandReceiptState` stores the request, actor, command, argument,
  typed terminal status, result, aggregate link, and receive/complete time.
- `HST_ResourceTransactionState` stores reservation/commit/refund/cancel
  status, exact amount, refunded amount, operation, command, actor, and stable
  settlement identity.
- `HST_CampaignEventState` records bounded command and resource transitions.
- Support requests, enemy orders, and active groups now carry a stable
  operation ID. Existing rows backfill that ID from their durable source ID.
- The first production ledger consumer is resistance training. Replaying one
  command request or transaction ID cannot spend money or increase training a
  second time.
- Save capture/restore deep-copies receipts, transactions, events, allocator
  state, and operation links. Restored reservations that never reached a
  terminal state are cancelled and refunded before normal campaign ticking.

## Schema 43

Exact force-planning authority foundation.

- `HST_ForceManifestState` freezes operation identity, faction, catalog and
  policy versions, exact ordered member/group/vehicle/asset slots, deterministic
  seed, and exact resource totals behind a stable hash.
- `HST_ForceQuoteState` stores actor, target, context hash, expiry, exact costs,
  all-or-nothing policy, status, and deterministic transaction links.
- `HST_ForceSpawnResultState` and per-slot results can encode queued,
  registered, deferred, retryable/final failure, and cancellation outcomes
  without treating an empty group root as success. Production garrison
  physicalization does not yet write or consume these queue results.
- Visible garrison recruitment first issues an exact server quote. A separate
  confirmation submits only the quote ID, revalidates the frozen context,
  reserves money and HR, registers the exact purchase-time aggregate increment
  plus acceptance provenance, verifies the delta, then commits both ledger rows.
  Restore reconciliation rolls back any `ISSUED` quote found with partial linked
  reservations, commits, or aggregate delivery.
- Schema 43 does not claim a living-slot roster: broad-alpha garrison
  physicalization does not yet consume the frozen member slots. Accepted
  settlement history and typed spawn-result history also await explicit bounded
  archive/retention policy.
- Schema-42 and older garrisons receive deterministic IDs. Existing aggregate
  force counts are preserved without synthetic manifests or retroactive
  accounting, and one bounded migration event records that limitation.

## Schema 44

Durable bounded spawn-queue state foundation.

- Every materialization attempt is keyed by its own spawn request and projection
  IDs. Manifest identity binds immutable input but is not the unique lookup or
  idempotency key because one manifest may be projected more than once.
- A batch persists the manifest hash, attempt generation, last/next attempt and
  update times, deadline, cancellation request, last failure, cleanup-pending
  state, and typed terminal result.
- A slot persists spawning/cleanup-pending states, the prefab actually passed to
  physical creation, its entity/group/projection links, and faction, group,
  projection, seat, and alive verification evidence.
- Save capture and restore deep-copy all queue fields and slot evidence.
- Campaign state persists an actual-restore sequence and the last restore
  sequence reconciled by the spawn queue. Applying a persisted campaign advances
  the actual-restore sequence exactly once before coordinator reconciliation;
  ordinary state normalization and new campaigns leave it at zero.
- Pre-schema-44 terminal success, final-failure, and cancellation batches are
  retained without synthetic request IDs or hashes. Every pre-schema-44 pending,
  deferred, retryable, in-progress, or cleanup-pending batch becomes an explicit
  final failure regardless of any partially populated newer identity fields.
  Every slot in such a batch has stale runtime entity and verification evidence
  cleared; existing final-failure and cancellation status/reason is preserved
  while registered and every other status becomes a final failure. Batch and
  slot migration timestamps are stamped with the exact migration second. One
  migration event records the conversion only when at least one row changed.
- On schema-44 restore, duplicate nonempty result, request, or projection keys
  invalidate every nonterminal batch carrying a conflicting key, including when
  the other key belongs to terminal history. Conflicting nonterminal rows fail
  closed with runtime group/entity evidence cleared; terminal historical rows
  remain unchanged. One stable event row provides bounded normalization evidence.
- Queue retention and active-work limits are enforced by the queue service.
  Migration never deletes valid terminal or settlement evidence merely to meet
  a cap.

## Schema 49

Canonical operation authority for the confirmed exact paid infantry-QRF slice.

- `HST_CampaignState` and `HST_CampaignSaveData` persist
  `HST_OperationRecordState` rows independently from legacy `HST_QRFState` rows.
  Each record binds one stable operation to its support request, accepted quote,
  immutable manifest, force-spawn result, projection, force, and active group as
  those execution identities become available.
- Quote issue allocates and persists operation identity in the existing planning
  aggregate but creates no `OperationRecord`. Successful exact confirmation opts
  the support request into operation contract version `1` and registers exactly
  one open record. Confirmation rollback may remove only a still-virtual,
  uncommitted record with no spawn/group authority.
- Origin and assignment remain immutable confirmation facts. Recall changes the
  tactical target toward the exit/origin without rewriting the assignment.
  Duty, engagement, materialization, position authority, and settlement are
  orthogonal typed states. Engagement exposes a legal
  `CLEAR -> CONTACT -> ENGAGED -> DISENGAGING -> CLEAR` API and preserves the
  resumable duty, but live combat-contact/disengagement detection is not wired in
  this schema.
- Queue admission records `OUTBOUND` plus `MATERIALIZING`; successful physical
  handoff records live position authority; confirmed arrival records
  `ON_STATION`; restore returns open physical records to strategic
  `MATERIALIZING`; recall records request/exiting duty; terminal ledger paths
  settle once with a typed recalled, spawn-failed, destroyed, cancelled, or
  invalidated result.
- Schema-48 and older saves receive a record only for a uniquely coherent,
  accepted, nonterminal exact paid player QRF. Migration preserves all economy,
  ledger, request, quote, manifest, queue, and group status. Every other family
  remains contract version `0`, including pre-exact player requests, terminal or
  archived-only history, ambiguous rows, enemy/legacy QRFs, and other support
  types.
- Schema-48 settlement archives remain the bounded historical container. A
  terminal contract-version-1 QRF can compact only when its full operation is
  coherently settled and existing backlink rules pass; its tombstone retains the
  operation settlement identity, contract version, revision, and typed terminal
  result while the full operation row is removed.
- `HST_OperationRecordProofService` includes focused current-schema restore,
  schema-48 coherent/ambiguous/incomplete/pre-exact/terminal migration, archive,
  confirmation replay, and legacy-QRF isolation assertions in the coordinator's
  force-authority debug case. They compile but have not yet run in a packaged
  runtime.
- This schema is not the complete operation/virtualization milestone. Strategic
  route progress, cursor/hysteresis, generalized physical/virtual transfer,
  vehicles/assets/multi-root forces, other support types, garrisons, missions,
  enemy orders, live engagement events, and client/JIP operation projection are
  still open. Packaged runtime and process-restart proof are also pending.

## Schema 48

Bounded accepted-settlement history and replay authority.

- Full accepted quote/manifest/transaction rows remain authoritative during a
  600-second minimum window and for the entire lifetime of every force-spawn,
  active-group, or enemy-order backlink.
- Garrison rows compact only after the exact purchase-time increment and its one
  accepted-manifest link are provable. The manifest ID then moves from the
  garrison's growing provenance list into the settlement tombstone; infantry is
  not added, removed, or recomposed.
- Paid-QRF rows compact only when the linked support request is resolved or
  cancelled and no physical/queue backlink remains. The tombstone preserves the
  final money/HR status, refunded amount, and settlement ID.
- Archived issue and confirmation requests reconstruct read-only summary objects
  and return an already-applied result. `ResourceLedgerService.ReserveCost`
  checks archived transaction IDs before `SpendResource`; exact committed replay
  succeeds without a debit, while identity conflicts and already-refunded rows
  fail closed.
- Tombstones are deep-copied by `HST_CampaignSaveData`. The oldest row can leave
  only after the 86,400-second replay window and only when the 256-row tombstone
  cap needs room. New planning fails closed at 320 combined full/tombstone rows
  when protected history cannot make room.
- Schema-47 saves receive the bounded
  `migration_schema48_force_settlement_archive` event and keep all prior accepted
  authority in full form. No quote, transaction, refund, or tombstone is derived
  merely because an old save was loaded.

## Schema 47

- `HST_CampaignState.SCHEMA_VERSION` is `47`.
- `HST_ForceSpawnSlotResultState` persists lifecycle revision, ever-alive,
  casualty-confirmed, casualty-second, and retirement-reason fields. The new
  terminal slot state `HST_FORCE_SLOT_RETIRED` is valid only for a member that
  was previously alive and has an explicitly confirmed casualty.
- `HST_ForceSpawnResultState` persists successful-handoff, reprojection, and
  lifecycle counters. A technical failure after at least one successful handoff
  is therefore distinguishable from initial deployment failure and cannot
  trigger an invented full refund.
- `HST_ActiveGroupState` persists `everPopulated`, `spawnCompleted`, durable
  living infantry, casualty/elimination seconds, and a lifecycle revision.
- Restore never reacquires an entity by its saved process-local ID. A successful
  paid-QRF batch clears those IDs, retains retired member tombstones, queues a
  new root plus only durable survivors, and records another successful handoff
  after exact verification. If no survivors remain, the support resolves as
  eliminated instead of respawning or refunding casualties.
- Runtime casualty polling accepts death only while the exact slot's entity is
  still present and authoritative life state reports it dead. A deleted or
  missing entity is not itself casualty evidence.
- Pre-schema-47 successful spawn batches receive one historical handoff.
  Registered alive slots receive ever-alive evidence, and uniquely linked
  active groups receive spawn-completed/ever-populated plus a durable living
  count. The bounded `migration_schema47_force_runtime_lifecycle` event records
  the aggregate backfill; no casualty or refund is invented.

## Schema 46

Exact player-paid QRF quote, ledger, executable manifest, and settlement
authority.

- `HST_ForceQuoteState` persists the exact support request, capability, asset
  profile, support type, ETA, cooldown, and quoted war-level context. Save
  capture/restore deep-copies every field.
- Selecting `support_qrf` with a map target issues an expiring quote only. The
  Forces tab exposes separate confirm/cancel actions; confirmation sends only
  the quote ID and the server resolves actor, target, context, catalog, and
  resources again.
- The frozen manifest contains exactly one authored infantry group execution
  root and every explicit ordered member slot. Money is a flat $250 and HR is
  derived from the exact slot count. No prefab-name manpower estimate or
  recomposition participates after issue.
- Confirmation reserves money and HR, registers and verifies one linked support
  request, commits both transactions, and changes the quote to `ACCEPTED` last.
  Replays remain idempotent after queue admission and after terminal refunds.
  Interrupted issued confirmations roll back linked aggregate and transaction
  state during restore before the generic reservation sweep.
- After ETA staging, the support service creates one queue-owned active-group
  projection with stable force/projection/result identities and submits the
  accepted manifest to SpawnQueue. The request is not physicalized until the
  queue reaches `SUCCEEDED` after physical-war handoff.
- Placement/admission failure while the operation is still `STAGING` and
  commander cancellation before strategic service commitment refund money and
  HR once and let a replacement quote bypass the historical cooldown. Once
  `LinkOutboundVirtual` commits `OUTBOUND`, money remains paid; a later
  no-handoff materialization failure refunds only the operation's last virtual
  survivor count through the HR transaction. Recall and post-handoff failure
  retain their survivor-only HR policies.
  The current no-schema-bump typed recall layer checks every settlement result,
  rejects lost-group or terminal-failure settlement conflicts explicitly, and
  preflights both legs of a full refund before either leg changes.
- Setup and terminal campaign frames continue queue cancellation/cleanup and run
  exact-support settlement even though campaign elapsed time is frozen.
- Schema 47 and schema 50 supersede the former restored-`SUCCEEDED` full-refund
  fallback. Restore now retains exact casualty history, requeues one held
  survivor projection, and normalizes the linked request to virtual authority.
- Legacy positive player-QRF cost fields are historical charge evidence only.
  Migration creates linked transaction rows without changing balances and never
  invents a quote or manifest. Ambiguous conflicts remain visible through one
  bounded warning event.

## Schema 45

Active-group projection identity and Game Master registration authority.

- `HST_ActiveGroupState` persists explicit `m_sForceId` and
  `m_sProjectionId` fields beside its operation, manifest, and spawn-result
  links. Save capture and restore deep-copy both fields.
- A pre-schema-45 active group receives missing force/projection IDs only when
  exactly one spawn batch is proven by conflict-free durable identity. A direct
  spawn-result link must agree with any stored manifest and operation links and
  with the group's force identity; without a direct result link, force,
  manifest, and operation must all match. Existing nonempty IDs are never
  overwritten.
- Linked groups with no unique provable batch stay unresolved. One stable event
  records the number of derived rows and one stable event records the number of
  unresolved linked rows, keeping migration evidence bounded.
- New spawn success requires alive, faction, native-group, Game Master,
  projection, and any applicable seat verification. The Game Master result is
  persisted, copied, replay-matched, cleared with nonterminal physical evidence,
  and included in final registered-slot readiness.
- Terminal history from older schemas is preserved without inventing Game
  Master proof. Actual restore still clears only process-local entity/native
  IDs from terminal rows while retaining their historical status, prefab, and
  recorded verification fields.
- Cleanup acquisition walks assets, members, vehicles, and group roots in that
  order. The normal per-tick action cap still applies, and ordering remains
  monotonic across successive cleanup waves.

## Runtime Settings Schema 22

True-town civilian traffic default and deterministic appearance correction.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `22`.
- Five civilian-driven traffic vehicles are the default for stock town-center
  locations. Minor localities do not inherit that town-scale traffic budget.
- A schema-21 value of `2`, the prior shipped default, migrates to `5`.
  Non-default operator values remain unchanged.
- Direct runtime civilian pools now use concrete stock appearance variants.
  The stock editor-randomized entries do not select a random default variant
  when directly spawned, which previously produced one repeated appearance per
  location.

## Runtime Settings Schema 21

Loot-setting key cleanup.

- `HST_RuntimeSettings.SCHEMA_VERSION` was `21`.
- Generated settings keep `lootSkipUnlockedItems` and
  `vehicleLootSkipUnlockedItems` as the canonical keys.
- Obsolete `lootOnlyLockedItems` and `vehicleLootOnlyLockedItems` aliases are
  no longer generated or read.
- Existing known gameplay values are preserved while the normalized settings
  file is rewritten.

## Runtime Settings Schema 20

Loot unlock defaults.

- `HST_RuntimeSettings.SCHEMA_VERSION` was `20`.
- Area and vehicle loot default to skipping already-unlimited items.
- Explosive and guided-launcher threshold unlocks default to enabled.

## Runtime Settings Schema 19

Ambient civilian traffic and generated settings comments.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `19`.
- Generated settings include JSON-safe `_comment` and `_comment_*` string
  fields that explain nearby settings.
- `civilianDrivingVehicleCountPerTown` controls how many civilian-driven
  ambient traffic vehicles can be active per active town.
- Existing settings migrate by rewriting the generated profile with comments
  and the traffic cap while preserving known gameplay values.

## Schema 41

Roadblock support garage vehicle state.

- `HST_CampaignState.SCHEMA_VERSION` is `41`.
- `HST_SupportRequestState` now stores the selected HQ garage vehicle id,
  vehicle prefab, vehicle display name, and whether the garage vehicle was
  consumed when creating a roadblock support request.
- Existing schema-40 and older support requests load with empty selected
  vehicle metadata and `m_bGarageVehicleConsumed == false`; only new
  player-requested roadblock support rows require and populate those fields.

## Schema 40

Gun shop mission stock and delivery runtime state.

- `HST_CampaignState.SCHEMA_VERSION` is `40`.
- `HST_ActiveMissionState` now stores generated gun shop item rows, seller and
  delivery asset ids, seller and delivery positions, purchase totals,
  delivery notice flags, and delivery start timing.
- Existing schema-39 and older active missions load with empty gun shop state;
  newly generated gun shop missions build their stock from the runtime arsenal
  item catalog and only start delivery after at least one purchase.

## Runtime Settings Schema 18

Generated settings comments.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `18`.
- Generated settings now include JSON-safe `_comment` and `_comment_*` string
  fields that explain nearby settings. The runtime loader ignores these fields
  and still reads only the scalar gameplay keys.
- Existing settings migrate by rewriting the generated profile with comments
  while preserving known gameplay values.

## Runtime Settings Schema 17

Generated settings hideout key removal.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `17`.
- `campaign.defaultHideoutId` is no longer generated or read. Initial HQ
  placement is selected through the setup map flow, so the profile config should
  not imply a separate default hideout source of truth.
- Existing settings migrate by rewriting the generated profile without the
  obsolete hideout key while preserving the remaining scalar settings.

## Runtime Settings Schema 16

Resistance support group marker tracking.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `16`.
- `features.trackResistanceSupportGroupsOnMap` defaults to `true`.
- Existing settings migrate the feature on so spawned player-requested
  resistance support groups keep live map markers until they are terminal or
  despawned.

## Schema 39

Support request HR costs and recall/refund state.

- `HST_CampaignState.SCHEMA_VERSION` is `39`.
- `HST_SupportRequestState` now persists HR cost, planned infantry count,
  refunded HR, recall request timing, recall exit position, and whether recall
  has been requested.
- Existing schema-38 and older support rows load with recall disabled, zero
  refunded HR, and zero HR cost unless force-composition metadata can backfill
  a planned infantry count for inspection.

## Schema 38

Vehicle-report strategic events.

- `HST_CampaignState.SCHEMA_VERSION` is `38`.
- `HST_StrategicEventState` now persists vehicle report fields: vehicle runtime
  id, heat before/after/delta, reported before/after, and report-expiry delta.
- Existing schema-37 and older strategic-event rows load with empty vehicle
  fields; new `vehicle_reported` events append rows when runtime vehicle heat is
  reported.

## Schema 37

Strategic event ledger.

- `HST_CampaignState.SCHEMA_VERSION` is `37`.
- `HST_StrategicEventState` records durable mission outcome, convoy outcome,
  zone-capture, and support-near-HQ consequences with event kind,
  source/mission ids, target zone/faction, applied status, summary,
  before/after owner fields, and deltas for money, HR, town support, capture
  progress, aggression, attack/support resources, and HQ knowledge.
- Existing schema-36 and older saves load with an empty strategic-event ledger;
  new mission success/failure outcomes, mission expiry, convoy outcomes,
  resistance zone captures, and hostile support-near-HQ knowledge changes append
  rows when they are applied.

## Schema 36

Active-group vehicle prefab.

- `HST_CampaignState.SCHEMA_VERSION` is `36`.
- `HST_ActiveGroupState` now persists the selected vehicle prefab for mixed
  infantry/vehicle groups.
- Existing schema-35 and older active groups load with an empty vehicle prefab;
  the runtime can select a valid faction vehicle when it next materializes a
  mixed active group that lacks a stored vehicle prefab.

## Schema 35

Support deployment proof.

- `HST_CampaignState.SCHEMA_VERSION` is `35`.
- `HST_SupportRequestState` now persists the deployment route id, placement
  type, placement summary, target/road/HQ distance meters, road resolution,
  vehicle-safe result, and whether vehicle-safe staging was required.
- Physical support requests now copy their placement proof into save data so a
  support request, linked active group, and folded support resolution can be
  audited after save/load.
- Vehicle-capable support requests require vehicle-safe staging. If vehicle-safe
  staging is specifically unavailable, the support deployment downgrades to
  infantry-only staging instead of failing the whole response.

## Schema 34

Population outcome metadata.

- `HST_CampaignState.SCHEMA_VERSION` is `34`.
- Campaign end state now persists the outcome mode, initial population,
  remaining population, killed population, FIA-supporting population, support
  percent, controlled airfields, and total airfields.
- Population victory is the default outcome mode: enough remaining population
  must support the resistance and all airfields must be controlled.
- Population loss is the default loss mode: killed population greater than one
  third of initial population ends the campaign.
- Existing ended saves backfill the new metadata from civilian town state and
  airfield ownership when possible; older control-based ended saves keep
  `legacy_control` as the persisted outcome mode.
- Runtime settings schema `15` adds `populationOutcomeEnabled`,
  `victoryPopulationSupportPercent`, and `legacyControlVictoryEnabled`.

## Schema 33

Active group source links and baseline force counts.

- `HST_CampaignState.SCHEMA_VERSION` is `33`.
- `HST_ActiveGroupState` now persists source identifiers for mission, support
  request, garrison zone, and QRF ownership, plus original infantry and vehicle
  counts.
- Migration backfills missing original counts from current active-group counts
  and links existing groups from support requests, QRF records, active mission
  guard/convoy group ids, or their zone id as a garrison-source fallback.

## Schema 32

Garage vehicle heat and undercover vehicle-cover handoff state.

- `HST_CampaignState.SCHEMA_VERSION` is `32`.
- `HST_GarageVehicleState` now persists reported state, civilian vehicle-cover
  eligibility, heat value, report timestamps, last report reason/zone, and
  passenger compromise count.
- Runtime-to-garage capture and garage-to-runtime redeploy copy vehicle heat
  metadata through the handoff. Existing schema-31 and older garage vehicles
  backfill civilian-cover eligibility from source faction, source kind, and
  civilian vehicle prefab hints, and start with no reported heat unless a newer
  save already carried it.

## Schema 31

Runtime vehicle heat and undercover vehicle-cover state.

- `HST_CampaignState.SCHEMA_VERSION` is `31`.
- `HST_RuntimeVehicleState` now persists whether a runtime vehicle is reported,
  whether it can provide civilian undercover cover, its heat value, report
  timestamps, last report reason/zone, and passenger compromise count.
- Existing schema-30 and older saves backfill civilian-cover eligibility from
  runtime vehicle faction, runtime kind, and civilian vehicle prefab hints.
  Existing runtime vehicles start with no reported heat unless they were already
  marked hot by newer state.

## Schema 30

Town influence events and population aggregates.

- `HST_CampaignState.SCHEMA_VERSION` is `30`.
- `HST_TownInfluenceEventState` persists event id, zone id, kind, source,
  reason, created/expiry seconds, support/reputation/heat/population/security
  deltas, and whether the event was applied.
- `HST_CivilianZoneState` now persists population remaining/killed, influence
  event counts, active/expired modifier counts, and the latest influence event
  metadata.
- Existing schema-29 and older saves backfill town population from civilian
  presence and mark legacy influence summaries from the last incident/security
  fields. Existing saves start with no influence event rows until the next
  town-support event is registered.

## Schema 29

Enemy support-spend ledgers and refunds.

- `HST_CampaignState.SCHEMA_VERSION` is `29`.
- `HST_EnemySupportLedgerState` persists per-faction/per-zone recent damage
  score, last damage time, attack/support spend, last spend time, same-zone
  cooldown, refund totals, and the latest decision reason.
- `HST_EnemyOrderState` now persists attack/support refund amounts and whether
  the resource refund has already been applied, preventing survivor fold-back
  refunds from replaying after reload.
- Existing schema-28 and older saves default ledgers to empty and refund stamps
  to zero/false; no destructive backfill is required.

## Schema 28

Force composition metadata.

- `HST_CampaignState.SCHEMA_VERSION` is `28`.
- `HST_ActiveGroupState`, `HST_SupportRequestState`, and
  `HST_EnemyOrderState` now persist composition request id, intent, selected
  tier, summary text, planned cost, planned manpower, planned vehicle count,
  and planned armed-vehicle count. Support requests and enemy orders also keep
  a composition failure reason.
- The persisted fields are diagnostics and replay hints only. Runtime entity,
  group, vehicle, and waypoint handles remain non-persisted.
- Existing schema-27 and older saves default these fields to empty strings and
  zero counts; no backfill is required.

## Schema 27

Player roster display names.

- `HST_CampaignState.SCHEMA_VERSION` is `27`.
- Runtime settings schema `13` adds `features.infiniteStaminaEnabled`,
  defaulting to `true` for generated and migrated settings. This is not a
  campaign-save migration.
- `HST_PlayerState` now persists `m_sDisplayName`, refreshed from the connected
  player manager when a player registers or is seen during menu/permission
  checks.
- Member and commander UI uses the display name for labels and only shows a
  shortened backend identity as secondary evidence.
- Permission checks, admin grants, loadouts, undercover state, and ownership
  records continue to use backend identity/SteamID64 values, not display names.
- Existing schema-26 and older saves default display names to empty and refresh
  them when the player reconnects.

## Schema 26

HQ spawn-point persistence and rebuild verification.

- `HST_CampaignState.SCHEMA_VERSION` is `26`.
- Campaign state now persists the HQ spawn-point position and prefab alongside
  Petros/cache/arsenal/tent HQ runtime metadata.
- Existing schema-25 and older deployed HQ saves backfill the spawn-point
  position near the HQ and clear the HQ runtime spawned flag so runtime objects
  are rebuilt with the new spawn point.
- Raw spawn-point entity handles remain runtime-only and are recreated by
  `HST_HQService`.

## Schema 25

Phase 24 balance, pacing, and campaign outcomes.

- `HST_CampaignState.SCHEMA_VERSION` is `25`.
- Campaign state now persists campaign-end reason, summary, elapsed second, control percent, war level, FIA/enemy zone counts, and whether an end report was generated.
- Existing schema-24 and older saves that were already WON/LOST backfill campaign-end fields from elapsed time, war level, and zone ownership counts.
- Runtime settings schema `9` adds balance knobs for starting training, war-level thresholds, victory/loss conditions, enemy income scaling, and aggression decay tuning.
- Balance and campaign-end reports are generated from existing zone/resource/mission state; raw world/runtime handles remain non-persistent.

## No Schema Bump: Phase 23 UI And Marker Polish

Phase 23 adds marker audits, command coverage reports, failed-action text, and
menu summary polish on top of existing persisted campaign state. These reports
are rebuilt from campaign fields such as markers, missions, support requests,
enemy orders, HQ/Petros state, civilian/undercover state, and runtime counters,
so no campaign save schema bump was required.

## Schema 24

Phase 22 HQ threat and Defend Petros.

- `HST_CampaignState.SCHEMA_VERSION` is `24`.
- Campaign state now persists HQ threat/knowledge diagnostics and active Defend Petros linkage.
- Defend Petros records persist linked mission/order/support/group ids, status, timing, attacker counts, failure reason, and outcome-applied flag.
- Existing schema-23 saves backfill HQ threat from HQ knowledge and mark existing Defend Petros mission ids as active.
- Raw Petros entities, attacker entities, AI groups, and runtime support entities remain runtime-only and are not persisted.

## Schema 23

Phase 21 undercover enforcement and police/roadblocks.

- `HST_CampaignState.SCHEMA_VERSION` is `23`.
- Undercover player records now persist applied/enforcement state, last compromise reason, detection source, enforcement zone, enforcement timestamps, detection score, and police/roadblock scan counters.
- Civilian town records now persist last police/roadblock scan times and the last security reason.
- Existing schema-22 records are backfilled from last eligibility/status/reason fields.
- Raw player entities, inventory handles, vehicle handles, and runtime civilian/police entities remain runtime-only and are not persisted.

## Schema 22

Phase 20 civilians, town support, and undercover reports.

- `HST_CampaignState.SCHEMA_VERSION` is `22`.
- Civilian town records now persist FIA support, occupier support, last incident reason, and last support-change time.
- Undercover player records now persist request state and the last detailed eligibility report: clothing, weapon/equipment, vehicle, off-road, enemy proximity, and wanted-heat reasons.
- Existing schema-21 civilian and undercover records are backfilled from reputation, support, police/roadblock presence, wanted heat, and last reason.
- FIA and occupier support values from Schema 22 or later are authoritative even
  when either value is zero. For non-town compatibility rows without canonical
  town-influence authority, inferred support backfill is restricted to restored
  schemas earlier than 22; current-schema restore must preserve explicit zeros.
- Raw player entities, inventory handles, vehicle handles, and runtime civilian entities remain runtime-only and are not persisted.

## Schema 21

Phase 19 support request lifecycle hardening.

- `HST_CampaignState.SCHEMA_VERSION` is `21`.
- Support request records now persist runtime status, resolution kind, physicalization mode, activated time, physicalized time, resolved time, physicalized flag, and outcome-applied flag.
- Existing schema-20 support requests are backfilled from status, group linkage, strike runtime ID, and ETA.
- Raw `IEntity`, `AIGroup`, support component handles, spawned entities, and callback references remain runtime-only and are not persisted.

## Schema 20

Phase 18 enemy commander physical responses.

- `HST_CampaignState.SCHEMA_VERSION` is `20`.
- Enemy order records now persist support/group linkage, runtime status, source/target positions, physicalization time, resolution time, resolution kind, failure reason, and outcome flags.
- Existing schema-19 enemy orders are backfilled from status and support-request linkage.
- Raw `IEntity`, `AIGroup`, support component handles, and spawned entity references remain runtime-only and are not persisted.

## Schema 19

Phase 15 garage and vehicle persistence hardening.

- `HST_CampaignState.SCHEMA_VERSION` is `19`.
- Garage vehicle records and runtime vehicle records now persist source-vehicle
  capability fields: ammo source, repair source, fuel source, and source vehicle kind.
- Existing garage/runtime vehicle records are backfilled from prefab-based capability rules.
- Schema 19+ restores preserve persisted source capability fields; prefab backfill is only
  for pre-schema-19 records or newly captured world vehicles.
- Garage redeploy runtime vehicles are eligible for persistent field-vehicle restore.
- No raw `IEntity`, `AIGroup`, inventory handles, or runtime pointers are persisted.

## Schema 18

Phase 11 mission-specific convoy outcomes.

- `HST_CampaignState.SCHEMA_VERSION` is `18`.
- Active missions now persist convoy outcome-applied flags and a convoy outcome
  summary so mission-specific rewards and penalties do not replay after reload.
- Mission assets now persist an outcome-applied flag and outcome kind so captured
  convoy vehicles, payloads, and captives cannot duplicate asset-level outcomes.
- Existing saves default the new booleans to false and strings to empty during
  normal save-data migration/copy.
- No raw physical entities or runtime handles are persisted.

## Schema 17

Phase 7 convoy waypoint-chain movement.

- `HST_CampaignState.SCHEMA_VERSION` is `17`.
- Active convoy groups now persist `m_iAssignedWaypointCount` as diagnostic
  route-assignment state.
- Schema-16 convoy groups that already recorded `convoy_waypoints` are
  backfilled from their matching generated route waypoint count during
  save-data migration.
- No raw waypoint entities or AI waypoint references are persisted.

## Schema 16

Phase 3 convoy route state.

- `HST_CampaignState.SCHEMA_VERSION` is `16`.
- Generated routes now persist ordered `HST_RouteWaypointState` records.
- Schema-15 routes are backfilled from their legacy start, midpoint, and end
  positions during save-data migration/copy.
- Existing route start/mid/end fields remain for compatibility and are kept in
  sync with waypoint records.
- Route vehicle-safety validation is diagnostic state and can be recomputed from
  generated route waypoints.

## Schema 15

Baseline for the Phase 0/1 stabilization work.

- `HST_CampaignState.SCHEMA_VERSION` remains `15`.
- Persistence smoke-test baselines are stored in the existing persisted `HST_CampaignTaskState` array using `hst_smoke_persistence_expected`.
- Phase 1 zone composition spawn slots are generated at runtime and do not add persisted fields.
- Phase 2 capture diagnostics and notification de-dupe state are runtime-only; capture tuning moved through runtime settings schema `8` without changing campaign save fields.
