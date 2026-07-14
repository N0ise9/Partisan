# Partisan Architecture

## Current Schema 70 / Settings 24 Runtime-Integrity Boundary

Campaign Schema 70 and runtime-settings Schema 24 remain the persisted
contracts. The current active-demolition-witness checkpoint is stamped at
source `0e54f6cbc7f7084e5534fc603b491cba0d91b653`, UTC
`2026-07-14T18:31:39Z`, label
`schema70-settings24-active-demolition-witness`. It changes neither serialized
campaign shape nor runtime-settings migration. It includes the preceding
SpawnQueue resume checkpoint at `0b380f00fde65c4f2e22858faf8ddc6eab794131`,
the debug cleanup-ownership checkpoint at
`3ded248a4ded084dfb0e3aa8e54ae0a47d36cd5f`, the checkpoint-evidence correction
at `2508a735863c153f95bae94adb13f3037b4cdeef`, and the current-support restore
correction at `89b7754bcd9ac7e8c41f2a8d7604784b5c1c1c83`.

R20 `seed1985_t0_p1_u1784047342` remains the historical diagnosis that isolated
the checkpoint-evidence false negative and four untracked Phase 22 orders. R21
`seed1985_t0_p1_u1784049066` remains the cleaner cleanup/persistence comparison:
zero open orders and zero exact runtime claimants, an exact persistence report,
and an exact-zero isolated-state diff.

Current R23 `seed1985_t0_p1_u1784054690` ran build `0e54f6c` and completed 688
cases at 564 PASS, 51 WARN, 66 FAIL, and 7 BLOCKED, proving 5,440/5,650 required
assertions with 192 failed and 18 blocked; it is not certified. All seven
destroy-family definitions pass their mission-start, mission-runtime, and
primitive-runtime cases. The six generic destroy-target variants also pass
`primitive.destroy.no_ambient_witness_score` with zero damage, zero hits, no
source, zero evidence keys, and an undestroyed target before explicit damage.
The radio-tower definition uses its separate exact lifecycle primitive and does
not carry that generic destroy-target assertion. All seven cleanup rows remain
WARN because the runner used administrative cleanup; those rows are cleanup
evidence, not physical completion evidence.

R23 retains the complete `early_mechanics.spawn_queue` pass: retry generation
advances 1 -> 2, work is scheduled only for the failed slot, stale-generation
callbacks are rejected, completion/replay and four registrations are exact,
same-wave retry/sibling/terminal progression is exact, and restore advances
generation 1 -> 2 at sequence 9 with one first change, repeated no-op, and one
completion. Foundation, authority foundation, and all eight `local_security.*`
assertions also pass.

R23 preserves exact seeded persistence at 11/11 missions, 22/22 mission assets,
21/21 runtime entities, 9/9 groups, 10/10 runtime vehicles, and 1/1 field
vehicles. Only `persistence.real_restart` is blocked in that family, and final
tracked-state restore is exact zero. The unresolved exact-QRF settlement crash
window still leaves two open orders and two exact runtime claimants during the
isolated run, making typed cleanup FAIL and the leak snapshot WARN. Phase-
persistence smoke/report WARN and 12 Phase-24 cases fail later while that
remnant is still present; R23 does not isolate every downstream Phase-24 failure
to that single cause. The clone then restores exact-zero live state.
R21 therefore remains the cleaner cleanup/persistence comparison. World-scope
restoration, external restart, package, network, and soak gates stay open.

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
SpawnQueue resume-selection delta. The current active-demolition-witness tree
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

The active development tree uses Campaign Schema 70 while runtime settings
remain on Schema 24. Newly admitted enemy garrison rebuilds use a separate
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
  is deterministic source proof until an isolated Campaign Debug run executes
  it. Foundation passes at 681 references, normal Workbench compile/create
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
- `HST_TownInfluenceProofService` and `HST_MapWarProjectionProofService` are
  compiled and wired deterministic source fixtures. The full Foundation gate
  and normal/all-configuration Workbench checks pass, but Campaign Debug has not
  executed these fixtures and they are not native runtime evidence.
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
  enemy resources once after return. Missing or conflicting authority fails
  closed instead of falling back to the legacy timer/support path.
- `HST_EnemyQRFOperationProofService`: contributes six focused
  `enemy_qrf.*` debug assertions for atomic admission, legacy isolation,
  projection transfer, survivor settlement, current-schema restore, and
  rejection/refund behavior. They are deterministic in-process fixtures, not
  packaged-runtime evidence.
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
- A frozen linked-town support event does not deadlock against a later pristine
  queued top-level receipt that already owns the town backlink. The exact
  influence event applies once with immediate ownership reconciliation disabled;
  after FIFO work drains, periodic town policy owns any remaining threshold flip.
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
  publication. These fixtures are source evidence until Campaign Debug or
  packaged runtime executes the appropriate gates.
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
  Reforger checkpoint requests, autosave debouncing, and profile JSON fallback
  saves. Every real capture first reconciles mapped physical exact-convoy and
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
| Resource integrity | Troop training, visible garrison confirmation, player-QRF, and Schema-60 Search-and-Destroy confirmation/terminal settlement use the resistance resource ledger. Sealed Schema 67 separately makes versioned enemy attack/support/aggression pools and bounded strategic mutation receipts canonical; exact enemy QRF/patrol debit/refund policy links into that boundary while unsupported orders remain legacy. | All resource changes use reserve/commit/cancel/refund transactions or the typed per-enemy strategic mutation boundary, with no direct debit paths outside the owning authority. |
| Force exactness | Existing exact infantry/convoy/rescue shapes remain unchanged. Schema 60 adds one infantry-only Search-and-Destroy root/member manifest and rejects vehicles, assets, empty rosters, and multi-root substitution. Historical Search-and-Destroy, other support/mission families, policy-v1/initial/enemy aggregate garrisons, and unsupported vehicle/multi-root policy remain outside these cutovers. | A quoted immutable force manifest is the only input to paid creation, and creation is all-or-nothing before any physical or virtual projection is published. |
| Force realization | SpawnQueue accepts frozen, hash-valid, all-required one-root infantry manifests. Its bounded selector now admits due deferred/retryable slots when their batch can start another attempt, while preserving manifest/dependency validation and normalizing slots only after selection. Both QRFs, player Search-and-Destroy, exact enemy patrol, policy-v2 purchased-garrison patrol, all three exact assassination guards, exact Schema-69 enemy counterattacks, and exact Schema-70 enemy garrison rebuilds release only durable living member slots; each empty root contributes no authored members. Counterattack and rebuild physical/virtual transfer use the same casualty slots and never refill them. Delivered rebuild manifests remain exact held garrison authority instead of being copied into aggregate infantry. R22 passes retry, stale-generation, same-wave, and interrupted-restore queue proof. The schema-52 convoy keeps its separate three-element PhysicalWar adapter. Generic vehicle/asset/multi-root and historical aggregate consumers remain unsupported. | One adapter realizes every supported manifest, registers each slot exactly once, restores successful projections safely, and feeds durable living-force/casualty/retirement authority without bypass paths. |
| Operation lifecycle | Schemas 50-60 retain their exact paths. Schema 69 adds a separately typed enemy-counterattack operation with one direct route, deterministic virtual combat, casualty-preserving physical/virtual transfer, canonical ownership request/retry, return-to-origin duty, and survivor-proportional one-pool settlement. Schema 70 adds a separately typed enemy-garrison-rebuild operation with frozen capacity, exact held delivery under an `OPEN`/`ON_STATION` operation and zero-delta receipt, proportional prearrival survivor refund, later zero-refund terminal retirement, and PREPARED/SETTLED crash-resume finalization. Historical counterattacks and rebuilds remain outside their exact contracts. | Every force/order uses one versioned operation aggregate with event-driven engagement, strategic movement progress, physical/virtual transfer, settlement, and client/JIP projection. |
| Event history | New command and ledger decisions append to a bounded persisted campaign event log. | All authoritative state transitions emit typed events consumed by projections, UI, diagnostics, and restore reconciliation. |
| Canonical ownership | Schema 62 routes military capture, mission capture, political support, admin, debug seed, and migration repair through one revisioned, idempotent receipt. The receipt owns security, support consequences, owner, town/facility/logistics derivation, retaliation, economy/outcome, events, parent-aware projection, notification, and persistence scheduling. Schema 64 submits strict political threshold intent to this service and cannot publish town ownership itself. | Runtime-prove exact security settlement, political retry/resume, native projection, save/restart, counterattack/economy consequences, nested publication, and all caller routes. |
| Canonical combat presence | Sealed Schema 63 separates registered physical infantry/manned-mobile/static samples, eligible virtual infantry, bounded cached queries, and revisioned zone heat from render state and empty assets. Capture, missions, HQ, civilians, enemy strategy, and the legacy wrapper share one injected service. Foundation, normal Workbench compile/create, and explicit five-configuration Script validation pass. | Runtime-prove native occupant/platform classification, every consumer, fail-closed authority/player filtering, allocation/cache invalidation/order, 30-second cooling, conservative restore, no save-dirty churn, and activation/deactivation hysteresis before deepening encounter and town truth. |
| Canonical town influence | Sealed Schema 64 gives each curated town one revisioned support/population/contact/flip record, separate FIA/occupier/invader basis points, exact typed events, strict 8000/4000 hysteresis, conservative pre-64 migration/current `-64` quarantine, and legacy projections limited to canonical curated towns. Nonpolitical/non-town civilian rows retain their Schema-22 persisted support values, including zero; only pre-22 no-town restores receive the legacy FIA/occupier backfill. Current exact events persist population triples before/after mutation; absolute debug seeds use the same idempotent boundary, and restore verifies the complete population chain plus the final record. Mutation-time aggregates and due-expiry-only history scans replace the old unconditional one-second fold. Foundation passes at 696 references, and normal plus all-configuration Workbench checks pass at 5,793 files/11,695 classes with CRC `36d5b017` and zero HST script errors. | Execute deterministic proofs, then prove every production caller, formula goldens, equality, owner delegation, save/restart, expiry, distinct enemy support, town taxonomy, and no stutter regression. |
| Civilian consequences | Sealed Schema 65 source/Workbench adds exact town casualty/theft/combat events, optional aggression target/delta/before/after evidence plus one matching strategic receipt, persisted combat episodes/adopted floor/last-applied receipt, bounded 256-casualty and 64-theft queues with a combined four-transaction frame cap and indefinite bounded-backoff retry, exact-pilot post-promotion theft, and pedestrian panic/recovery with separate bounded route recovery. Minor localities remain panic-only and keep their exact fingerprint map in session memory. Foundation passes at 717 script-symbol references. Final stamped normal/all-five Workbench checks are clean at 5,802 Game files/11,728 classes with CRC `c0a672b9`, `Script validation successful`, zero HST script errors, and zero surviving processes. | Execute the deterministic fixtures, then package-prove native callback attribution, no duplicate casualty/theft/aggression, threat-driven RUN/calm WALK transitions, restore/quarantine, minor-locality restart behavior, and balance under multiplayer/soak load. |
| Enemy-town local security | Sealed Schema 66 source/Workbench gives each eligible canonical enemy town one deterministic exact patrol epoch backed by a frozen authored 2–5 member roster and held SpawnQueue slots. Casualties survive physical fold and restore; no generic police projection may refill or fold counts into it. Destruction applies exactly one police `-1` event, while owner/pressure clear, setup/stop, and spawn failure settle without loss. Same-epoch resurrection is forbidden; a newer owner revision or later positive police event is required for rearm. Resistance automatic police/roadblock targets are zero. Pre-66 migration preserves logical facts and drops only backlink-free disposable legacy projections. | R22 passes all eight isolated `local_security.*` assertions with no materialization deferral. While Campaign Debug holds the force-spawn worker, it also holds the ambient producer so no patrol can be released into a process-only MATERIALIZING state before checkpoint capture. Package-prove native roster, waypoints, casualties, fold/re-entry, no refill/no resurrection, exact rearm, ownership ordering, persistence/restart, campaign terminal cleanup, multiplayer, and balance. |
| Canonical enemy strategic resources | Sealed Schema 67 makes each versioned `HST_FactionPoolState` the unique attack/support/aggression/cadence owner for one enemy role. Compact periodic receipts and last-bucket checkpoints are separate from a contiguous un-compacted operational sequence, including zero-effect rows, capped at 4,096 per faction. One API owns live mutation; restore validates exact order/ledger/town/ownership backlinks. Pre-67 restore adopts baseline values/checkpoints without invented history; malformed current graphs quarantine at `-67`. | Real-restart cadence, cap, reciprocal backlinks, and no-bypass proof remain open. Schema-68 planning consumes this authority without mutating or repairing it. |
| Canonical enemy planning | Sealed Schema 68 adds one independent 180-second planning row per configured enemy, sorted commitment/target/source hashes, frozen input and decision fingerprints, bounded preparation/prepared retry, explicit committed/skipped/rejected completion, exact order/debit backlinks, conservative pre-68 baselines, and Schema-68 `-68` quarantine. The sealed bootstrap correction shares one production fresh-state factory, admits only the exact known quarantine signature, and throttles unchanged unavailable warnings. Commitment roots use blocking precedence, compatible penalties, and deterministic same-decision reranking; preparation remains freeze-only and admission is rechecked before debit. | R10 proves held ambient cadence, exact clock/fingerprint isolation, all five Phase 18 cases, and explicit Phase 22/24 production ticks. Package-prove restart, dedicated/live-server, multiplayer/networking, and soak behavior. |
| Exact enemy counterattack | Schema 69 gives new contract-`1` counterattacks one reciprocal order/operation/manifest/batch/group aggregate, one charged attack or support pool, direct virtual travel, deterministic combat, physical/virtual casualty continuity, canonical ownership transition, return, and proportional survivor refund. The appended `PREPARED` settlement intent enforces prepare, stage, refund, record, and finalize ordering across restore and same-session retry; derived-ID claimant scans reject ambiguous residue. Historical rows remain contract `0`; malformed Schema-69 graphs quarantine at `-69` without fabrication, deletion, settlement, refund, or outcome. | The scoped checkpoint is sealed and R10 passes the Phase 17 exact-counterattack case; the separate force-progress case still fails. Prove serialization/restart, native projection/combat, package/live-server, migration runtime, marker runtime, network/JIP/reconnect, and soak behavior. |
| Exact enemy garrison rebuild | Schema 70 gives new contract-`1` rebuilds one support-funded reciprocal aggregate, capacity-capped frozen infantry, owner-and-revision-bound target/source capability, casualty-preserving virtual/physical travel, and an exact manifest link at the destination. Delivery records a zero-delta receipt, remains `OPEN`/`ON_STATION`, and never double-counts aggregate infantry. Prearrival survivors return and refund proportionally; delivered terminal events unlink and retire without refund. Malformed or orphaned authority quarantines at `-70`, becomes non-executable, and remains retention-pinned. | R10 passes all five Phase 18 cases and typed rebuild settlement before cleanup. Package-prove native routing/projection, save/restart, dedicated-server ownership and settlement, markers, network/JIP/reconnect, and soak behavior. |
| Political Map/War projection | Sealed Schema 64 supplies contacted-only Zone Pressure with current-first/stable support ordering and complete deterministic Resistance Territory from published canonical ownership. Resistance Territory reuses the marker projection's completed-parent ownership resolver, preventing a nested child from appearing before its parent transition publishes. | Prove rendered rows, current-town detection, discovery, incomplete ownership fencing, no arbitrary truncation, save/restart, reconnect, and JIP. |
| Client marker projection | Schema 61 implements stable marker IDs with record revisions/tombstones, one epoch/global sequence, bounded hashed snapshot and ordered-delta packets, ownership-derived sessions, an atomic registry, deterministic priority capping, and client-local native reconciliation. Schema 62 adds ownership source revision, while the Schema-66 repair keeps protected campaign markers system-owned/non-removable and self-healing. Exact QRF, counterattack, garrison-rebuild, and patrol audit backing now calls the marker publisher's canonical-ID and operation-specific visibility predicates. | Execute the destructive owner-client probe, then package-prove edit/delete resistance, bounded self-heal, exact operation-marker continuity, snapshot/delta, map reopen, reconnect, and JIP. |
| Radio physical authority | Schema 59 keeps one exact lifecycle owner per site. The current adapter queries the generic base, exact stock `SCR_DestructionMultiPhaseComponent`, and destruction base, then returns shared health/state authority across admission, polling, writes, restore, and suppression. Generated demolition resources enable the existing inherited multiphase/RPL pair, and zero-health destruction uses the engine `Kill()` path. | R16 proves the isolated disposable destroy -> stop-rebuild chain, including normal callback, deterministic receipts, unchanged destruction epoch, exact `$450`/`$350` rewards, second-attempt rejection, exact cleanup, and zero final state diff. Packaged authored binding, restart/streaming reapplication, multiplayer, and soak proof remain open. |
| Destroy-target demolition witness | Nearby evidence must be an unparented physical projectile with active movement or a triggered blast; parented/inventory equipment is rejected before text classification. Witness scans and entity-backed callbacks share one canonical source key. A target retains at most 64 lifetime source receipts, fails closed at capacity, and writes local bookkeeping only after authoritative asset mutation. | Fresh Workbench validation passes. R23 passes all six generic `primitive.destroy.no_ambient_witness_score` assertions and all seven destroy-family start/runtime/primitive cases. Preserve one-source/one-score behavior through callback-plus-scan overlap, carried equipment, restore, multiplayer, and soak proof. |
| Campaign Debug isolation | The runner deep-clones campaign state, suspends normal persistence, and restores the live state. Bounded probes additionally capture/restore the shared clock and enemy-strategic fingerprint; the coordinator holds ambient commander cadence while the clone is active. It also holds ambient local-security progression whenever the matching force-spawn worker is held, preventing a debug-only release into MATERIALIZING with no process owner. | R23 passes Foundation checkpoint capture, all eight local-security assertions, the complete SpawnQueue family, exact seeded persistence counts, and exact-zero isolated-state restore. Its exact-QRF settlement failure leaves two open orders/two exact runtime claimants, directly fails typed cleanup, and makes the leak snapshot WARN. Phase-persistence warnings and 12 Phase-24 failures occur later while the remnant remains, without proving one cause for every downstream case; R21 remains the cleaner zero-open-order comparison. World entities, players, delayed callbacks, caches, and process restart remain outside the clone boundary; `world_scope` stays BLOCKED. |
| Workbench compiler shape | Large Campaign Debug methods use compact context/result objects and focused helpers. The render-bubble proof keeps clock state in `HST_CampaignDebugClockIsolationContext` rather than extending an already-large local frame. | Preserve this boundary and require a fresh Game compile plus bounded cold open for future large proof additions; repository text/static validation cannot exclude a native compiler heap failure. |
| Certification | Schema 70/settings 24 remains the current persisted contract, and the sealed Schema-70 focused checkpoint remains historical evidence. Current source is stamped `0e54f6cbc7f7084e5534fc603b491cba0d91b653`, label `schema70-settings24-active-demolition-witness`, with no schema/settings migration. | Fresh Workbench validation passes. R23 is the latest in-process diagnostic at 564 PASS/51 WARN/66 FAIL/7 BLOCKED and 5,440/5,650 required assertions proven, with 192 failed and 18 blocked. All six quiet generic destroy-target assertions and all seven destroy-family start/runtime/primitive cases pass; the SpawnQueue family, Foundation/authority foundation, all eight local-security assertions, exact seeded persistence counts, and exact-zero final tracked-state restore also pass. Its exact-QRF settlement crash-window defect leaves two open orders/two exact runtime claimants, directly breaks typed cleanup, and remains present during later phase-persistence warnings and 12 Phase-24 failures; R23 does not attribute every downstream failure solely to it. R21 remains the cleaner cleanup/persistence comparison. Repair that defect next while preserving the demolition-witness result. World-scope restart plus serialization/restart, package, physical/virtual combat, canonical ownership, profile migration, markers, rendered UI, performance, dedicated/live server, multiplayer/networking, reconnect, JIP, and soak gates remain open. |

The canonical ownership dependency and first shared crew-aware combat-presence/
heat dependency remain sealed through Schema 63. Sealed Schema 64 adds the
town-truth and political-map dependency and has passed Foundation plus normal
and all-configuration Workbench validation. Together they advance Blueprint
Phase 7 but do not finish it: native
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
and flushes the tracked scripted state before requesting
`SaveGameManager.RequestSavePoint` when saving is possible and allowed. The
service also writes `$profile:Partisan/HST_CampaignSaveData.json` as a profile
fallback when scripted persistence cannot be flushed, and will load that file
if no restored `PersistenceSystem` state is available. If the canonical file is
absent after tree migration, the compatibility resolver may still read the
remaining retired fallback so an incomplete move can be retried without data
loss. The state model is
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
evidence for once-only resource settlement. Its physical runtime handles are
discarded, its exact casualty roster and route cursor remain durable, and its
projection resumes as one held virtual batch. Deterministic roundtrip fixtures
cover this shape in source; a real process restart remains unproven.

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
