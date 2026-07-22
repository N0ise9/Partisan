# Historical Local-Candidate Identity Evidence

## Current Authority Boundary

This directory is a historical and optional-QA evidence archive. It does not
contain or authorize a source `data.pak`, and no generated `.pak` belongs in the
repository. Current Gate 1 authority is the frozen clean source checkpoint plus,
in order, its Foundation result, all-target Workbench validation/compilation,
the five individually named source-native focused suites at JUnit 91/0/0/0, the
corrected source-native force-authority canary, and source-native Full Campaign
Debug.

The supported publication path is Workbench to Workshop; the game downloads
the published addon. The bespoke local candidate pack, manifest, ready seal,
and package digest retained here are not release authority and are not a manual
distribution workflow.

This directory retains portable, byte-identical copies of each historical
local candidate's `candidate.json` manifest and `candidate.ready.json` seal. The
generated current-status check verifies their hashes and cross-checks their
source, package, addon, toolchain, Workbench, and ready-seal identities.

These small tracked records are not substitutes for the immutable package and
raw evidence bundle. Under that workflow, proof attached to an active runtime
candidate had to consume the exact candidate ID and aggregate package SHA-256
named by its manifest. Rebuilding started a new candidate directory and
evidence chain; existing records are not rewritten.
Repository attributes keep both JSON records on canonical LF endings so their
byte hashes remain stable across checkouts.

## Historical Portable Schema-2 Runtime Evidence

The retired local-candidate workflow published focused evidence as one tracked
schema-2 aggregate
under `focused-autotest/`. The tracked JSON is only the portable index: the raw
five-suite/40-blob tree remains in the immutable external evidence root. Each
`sourceRuns[].runEnvelopePath` is relative to that supplied root and must use
`<candidate-id>/focused-autotest/<profile>/<run-leaf>/run.json`. Publication and
release-doc consumption independently reopen the five envelopes, exact eight-
blob inventories, candidate manifests/seals, JUnit results, logs, and hashes.
The exact suite testcase counts are 14, 13, 17, 6, and 41. The final local-
candidate contract required all 91 individually named testcases at JUnit
91/0/0/0, while preserving individual-case and whole-suite selection. The
journal suite binds its 51
aggregate diagnostics to exact testcase intervals: 10 stock plus 41 intentional
native-save diagnostics.
The index binds the focused-run harness Git blobs plus the committed aggregate
producer and release-doc consumer. A mismatch produces red
`replacement-required` evidence; it never rewrites an accepted aggregate.
Package identity is independently recomputed from the exact canonical four-file
manifest tuple set. Console admission censes all suite-start and profile-success
shapes, and a replacement receipt has authority only when its candidate ID plus
package, manifest, and ready-seal hashes match the retained candidate.
Focused success-marker parsing must remain encoding-stable in Windows PowerShell
5.1. These BOM-less UTF-8 tool sources spell the optional marker as ASCII regex
`(?:\u2705\s+)?` and build the real fixture character with
`[string][char]0x2705`; a literal non-ASCII source marker can be decoded through
the legacy code page before regex construction even when the retained engine log
contains correct U+2705 rows. Tests must admit the exact marker or no prefix and
must reject arbitrary replacement prefixes.
Historical five-testcase JUnit evidence remains valid only for candidate
ancestry at or before `075558ac7b6c14d1bb3e5829a2b87f3dbb608351`; it remains
history and cannot satisfy the active 91-testcase contract.

That workflow's corrected `force_authority` canaries and full Campaign Debug
runs share the portable schema-2 Campaign Debug release-index envelope. Their tracked
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

The historical consumer chooses Schema 1 or Schema 2 from the retained summary
schema and evidence kind, not from the shared `passed-noncertifying` status. Active Schema-2
evidence requires stationary current tool bytes. Historical Schema-2 evidence
instead reopens the recorded immutable Git blobs and ancestry so later legitimate
tool changes do not invalidate already accepted history.

Tool hashes in schema-2 evidence were admission boundaries. Run publication and
consumer self-tests only from a stationary committed tool set; dirty producer,
runner, candidate-module, or consumer bytes correctly fail closed.

## Historical Local-Candidate Release-Surface And Retention Evidence

Under the retired workflow, an active candidate could attach one paired
evidence set under `release-surface-audit/` and `gate1-runtime-retention/`.
Each tracked file is a portable `release-index.json`; the complete raw run
remains in the immutable external evidence root. The release-doc consumer
requires an explicit evidence root, reopens the raw `run.json` and terminal
`run.ready.json`, rehashes their
complete file census, verifies the clean harness Git ancestry and tool blobs,
and exact-matches the candidate ID, source HEAD, manifest and ready hashes,
package digest, and candidate binding. It then invokes each exact Git-bound
publisher in read-only verification mode so the full raw semantics, canonical
index bytes, strict scalar types, and terminal seal are revalidated by the same
contract that published them. One member without the other fails
closed, and an accepted Full Campaign Debug result cannot advance without both.

Surface identity bindings retain the candidate manifest's exact four-field
package rows: `path`, `indexPath`, `length`, and `sha256`. The consumer first
requires ordered equality with the trusted active-candidate manifest and the
canonical `package/` path relationship, then projects the three digest fields.
Synthetic consumer fixtures must preserve that production shape so producer and
consumer schemas cannot drift while their separate tests remain green.

Mount evidence is a structured candidate attestation, not a GUID substring
count. Parse only timestamped `ENGINE : gproj:` records, select the exact case-
sensitive candidate GUID, and require the exact guard-owned candidate path. An
accepted mode records `candidateMountAttestation` with `valid: true`,
`recordCount: 2`, `exactPathCount: 2`, `packedCount: 1`,
`invalidModeCount: 0`, `guidExact: true`, and `packed: true`. Base-project rows
and command-line echoes do not count. Focused raw consumption also derives the
run nonce and requires the relative suffix
`PartisanFocusedAutotest/<nonce>/candidate-addons/Partisan/addon.gproj`.

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
members. The source guard passes 15/15, the paired runner self-test passes 56
checks, the surface publisher passes 74 checks with 73 negatives and two guarded
receipts, the retention publisher passes 73/73, and the consumer passes 3
valid/optional plus 53 adversarial cases. These source and publisher checkpoints
join a 741-check focused-aggregate self-test and do not constitute a runtime
audit. The fourth and
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
halves required a fresh current-tool run against the unchanged package. At that
historical checkpoint the active candidate had no accepted pair, `STATUS-008`
was open, and release remained `NO-GO`.

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

Follow-up run `20260722T054405Z-592d89ac42b8` used the actual profile subtree
and proved manifest hashes, full census, exact copied rows, unique requested UUID,
`m_sMissionResource`, `2/1/8`, and nonempty `System/` payloads. The first
standard context received byte-exact input and reached online/GAME, but selected
`profile_fallback` after diagnostic-created native state entered `FAILURE` under
the different standard script topology. This is neither a missing-copy defect
nor standard native-restoration proof.

The same run's live console grew to 25,241 bytes while all 104 `ReadAllText`
polls conflicted with the held engine writer. Readiness now uses bounded shared-
handle snapshots, strict UTF-8, append-only prefix continuity, and two consecutive
semantic observations. A UUID-bearing stage accepts coherent `native` plus the
restoration marker or coherent `profile_fallback` without it; explicit missing
UUID/new-playthrough rejection still fails. The no-UUID stage remains exact
fallback. Bounded failure state is path-free, and the no-engine suite passes
71/71. The result remains `raw-retention-only` with
`standardSaveRestorationCertified=false`.

Fresh retention run `20260722T061934Z-41752660e5a2` completed all five
diagnostic and all five standard contexts, then failed before publication. The
real-only ordinary-library import replaced the publisher's generic typed
signature helper with a same-named string helper, so the first valid retained
row failed its comparison. Preserve its failure seal, cleanup, `run.json`, and
251 currently exact census rows as noncertifying harness evidence. It has no
release index or ready seal and must not be retrofitted. The publisher now uses
a producer-unique helper and the 72nd no-engine check reproduces the exact import
order and typed signature result.

Corrected run `20260722T070815Z-6e6d4849f5ad` completed all ten contexts,
published and ready-sealed a 251-file noncertifying index, passed independent
zero-write verification, and cleaned every owned boundary. Pair preparation
then proved that this index and the earlier surface index contained native CRLF
internally while their tracked destinations require LF. Git would change their
exact consumer-bound length and SHA-256, so neither was attached or modified.

Both publishers now route publication and recomputation through one
producer-specific LF-canonical serializer. Their self-tests require CR-free
output whose raw Git blob equals the blob after the tracked-path clean filter;
surface passes 66/66 and retention passes 73/73 without engines. The evidence
consumer still requires exact external/tracked bytes. Because both bound
producer blobs changed, a fresh surface and retention pair was required against
the unchanged candidate rather than weakening that rule or retrofitting either
old seal.

The first invocation after that correction used overlapping watched and spill
roots. Surface leaf `20260722T074139Z-a58c0363e057454082a54cd244f71b81`
failed preflight before any mode or engine ran, recorded `completedModeCount: 0`,
wrote its failure seal, and cleaned exactly. It remains permanent no-engine
forensic evidence.

The real four-field surface package binding then exposed a synthetic consumer-
fixture mismatch. After the consumer exact-matched those rows to the candidate
manifest and its suite passed 3 valid/optional plus 50 adversarial cases, fresh
surface leaf `20260722T081345Z-45f8c65a9dbc463abfa1c72e9a2a6042` and retention
leaf `20260722T081531Z-ecf73c44732b` passed under clean harness `52c7e2b`. The
surface index/ready SHA-256 values are
`c54c96ecbe128723fd09d662964659be4388de99260d5677feac9338973122d1` and
`d3387977b0f1df39486e27da20393c881f80faee37701f433a9806941b72a068`;
the retention values are
`5b5ad57e61b254188b99e5e53ea8936a4b76e170a8b96529a4b965d44d6625fb` and
`6d05989c1efc84da287baceab1af96c95e0855ee2fb02d2d81342ab4a64e435f`.
Both LF indexes and terminal seals independently reverified without writes, and
the release consumer accepted the pair together. `STATUS-008` is closed without
certification promotion.

That close remains an accurate historical CRI-097 checkpoint. The exact surface
and retention trees remain immutable, but a subsequent correction to bound
focused, release-document, and surface-consumer tools supersedes the `52c7e2b`
pair as the active current-tool pair and reopens `STATUS-008`. Candidate,
package, and gameplay bytes are unchanged.

Focused counterattack leaf
`20260722T084353Z-03d78e016b244d8fb1aea42f307af1d6`, envelope SHA-256
`8b174b8abebb17225f3737be6934f6ec8866d4c99978106a79f94d7e8ef6102f`, is
rejected forensic evidence. Its raw JUnit was 14/0/0/0, all 14 exact success
markers and the exact stamped build banner were present, its two diagnostics
were approved stock events, and cleanup was exact zero. The old parser still
counted six legitimate candidate/core/data `gproj` rows instead of selecting the
two candidate records, so the leaf cannot enter the five-suite aggregate.

The corrected current-tool recapture is accepted. Clean harness
`e2c38d2770d8ebaaa675326d1b8a91068db989e5` produced 41-file surface leaf
`20260722T103329Z-edea9d8417884dd8a2d2b313c4543ad0`. Its index and ready
SHA-256 values are
`205f9c2d2c3166bced4e92312ded1d16a633518732fad748b5178628e59d75b6` and
`7201bb54caccd48ee270bb34a9eba5e5e09596ea33b9855b9cbf5a2499c6a43a`, and its
structured candidate-mount attestation is exact at 2/2/1/0. Retention leaf
`20260722T103514Z-436f331b8659` retains 251 files; its index and ready SHA-256
values are
`94906067a7637e69a186926dc731320cf14011a04c9fe7a26117960f46e7f29a` and
`ff503f5f1f16ad7fbac45c292ad43a2b07984ca35ce8a188cbee2b55ddaea503`.
Both halves passed, independently reverified read-only, and were jointly consumed
against unchanged package
`af22d6322a215dbef466e49041fc07395cbb5ed7a5951fd3e0cee5f4a101f530`.
`STATUS-008` is closed again. The `52c7e2b` pair remains immutable superseded
history.

The first focused counterattack run after that close retained leaf
`20260722T105628Z-68bd71de0fea40f68b533feee1c9e86a`; its `run.json` SHA-256 is
`ec76dfb51f71247715a78336fc6cb1be030db4242c3dafc8aa5be580ebe27be8`.
Candidate binding and 2/2/1/0 mount attestation were exact, exit was `0`, JUnit
was 14/0/0/0, all 14 U+2705 success rows plus exact suite/build evidence were
present, the failed list was empty, the two diagnostics were approved stock with
zero intentional and zero unapproved diagnostics, and cleanup was exact zero.
The wrapper rejected the leaf because the success-marker census alone produced
`MarkerOrderExact: false`.

The retained log was correct. A literal U+2705 in three BOM-less UTF-8
PowerShell regex sources was decoded through the legacy code page by Windows
PowerShell 5.1 before regex construction. The ASCII-source `\u2705` correction
and runtime `[string][char]0x2705` fixture are tooling-only; the failed envelope
remains rejected forensic evidence. Because the correction changes the bound
release-document consumer, both the `52c7e2b` and `e2c38d2` pairs remain
immutable passed history under their recorded tool bytes but neither satisfied
the then-active current-consumer boundary. At that historical checkpoint,
`STATUS-008` reopened and the planned next order was a fresh same-package pair,
all five package-bound focused suites, the corrected canary, and Full Campaign
Debug only after an accepted canary.

CRI-101 accepts the fresh encoding-stable pair under clean harness
`7e2fe71d9619fd60cfe393d2776b4ed49f75186b`. Surface run
`release_surface_20260722T114802Z_1fb5e08619ef45b2aae2`, from
`2026-07-22T11:48:02.4105472Z` through `2026-07-22T11:48:44.3786578Z`, retains
41-file leaf `20260722T114802Z-1fb5e08619ef45b2aae20f2c5938b56c`. Its tracked
index and ready-seal SHA-256 values are
`832c91705120f54de5c2afa974492efdf23cb11f4f1ef9b8f2395c8ecf2399e8` and
`84079df17dc4293e23706f36e3a4eb6efefae29cf709396ecbee2530441dded0`.
Both per-mode mount attestations are exact at 2 records/2 paths/1 packed/0
invalid; retail retains the exact approved stock 6-raw/2-event cluster,
diagnostic retains 0 raw/0 events, and unapproved diagnostics are zero. Its
disposition is `passed-noncertifying-release-surface-audit` with no
certification promotion.

Retention run `gate1_20260722T115003Z_4e159a24384c4808aee0`, from
`2026-07-22T11:50:03.6835003Z` through `2026-07-22T11:55:56.7758898Z`, retains
251-file leaf `20260722T115003Z-4e159a24384c`. Its tracked index and ready-seal
SHA-256 values are
`5989778fef9a560b94dff311e8fa82547166f10ac2b0598504a56489163f5a4e` and
`023f17a4e36f73627fbe64bb456165991b7a66f3100908d5343f49ded00d4504`.
It covers five diagnostic lineage stages plus five standard contexts and keeps
the exact noncertifying boundary `standardSaveRestorationCertified: false` and
`certificationClaim: none`; its disposition is `passed-noncertifying-retention`.

Both tracked indexes byte-match their sealed external copies, every indexed row
and ready seal independently reverified read-only, and the release consumer
accepted the pair together against unchanged package
`af22d6322a215dbef466e49041fc07395cbb5ed7a5951fd3e0cee5f4a101f530` and
candidate binding
`06b5fa23ccd10bfd1f8621f6b03c25d9c36719cc9c7d08ebcc73dabbba8c9f5f`.
`STATUS-008` is closed again. The `52c7e2b` and `e2c38d2` pairs remain immutable
passed history, and focused leaves
`20260722T084353Z-03d78e016b244d8fb1aea42f307af1d6` and
`20260722T105628Z-68bd71de0fea40f68b533feee1c9e86a` remain immutable rejected
history. Candidate, package, gameplay, Foundation, and Workbench facts are
unchanged. At that historical checkpoint, the planned next order was all five
package-bound focused suites and their 91-case aggregate, then the corrected
canary, then Full Campaign Debug only after an accepted canary. Current
source-native Gate 1 remains incomplete and release remains `NO-GO`.

CRI-102 preserves the first focused batch under clean harness `87b5ad4` as one
byte-exact superseded/rejected forensic set. The four green service leaves are:

- `20260722T121635Z-77171d61a6434ff1a923122155fefd7d`, counterattack JUnit
  14/0/0/0, `run.json` SHA-256
  `4bb519bd1d92c24c83fab138efac6d39b6d2d7d72734a47ecf5575c215fafa85`;
- `20260722T121701Z-34505d25f4a4430e93357a4dfc8f3bc5`, garrison-rebuild
  JUnit 13/0/0/0, `run.json` SHA-256
  `0ca066328c15a4172e8b670a8cdddbe0416b58fa68f12b01408255ffe574797b`;
- `20260722T121720Z-6c80964f0cd14c0d9142d97589014c36`, planning-commitment
  JUnit 17/0/0/0, `run.json` SHA-256
  `087a7e6713d3c37b2abdb3776c8a4cf3dfcef15508241b5c176181ae574b427a`;
- `20260722T121742Z-acab14de0fc94dd2a06569d7182ed525`, QRF JUnit 6/0/0/0,
  `run.json` SHA-256
  `240f1d936ccb6140d3f9fc1b54fe0405c0495c08a8ce67bab31e999c8c49f382`.

Profile leaf `20260722T121803Z-ada938901086420ea9d51844b3cf4b2f`
retains `run.json` SHA-256
`c451a8aa07c3c62fb105a3268800625ad711ba472021afdc7d7f5ffb99fac66a` and
console SHA-256
`f12ce122ef085f18fe5db3a941259f162b481da2c46bcd5ce8def7287a58879b`.
It had exact package and 2/2/1/0 mount identity, exit zero, an empty failed list,
and JUnit 41/0/0/0, but the wrapper rejected. The raw hard census was 43: two
stock plus 41 intentional native-save failures. The old classifier approved
none of those 41 because its fixture incorrectly taught all 41 testcase
intervals to contain a detailed seam row that production emits only once.

The corrected contract is exact 41/41/1. Every named interval requires one
intentional native-save failure followed by one
`failed native callback non-mutating 1` summary before success. Exactly one
`setup/seam/request/bytes/journal 1/1/1/1/1` detail is required only in the
dedicated failed-callback interval, after its summary and before success, and is
forbidden in all other intervals. Missing, duplicate, misordered, wrong-case,
or additive evidence fails closed. All 41 intentional events are approved only
when the whole topology is exact; the two post-suite stock events remain
separate.

Changing the runner, aggregate producer, and release consumer means none of the
five batch leaves can transfer. It also makes the CRI-101 pair immutable passed
history rather than then-active current-tool evidence. `STATUS-008` was open;
candidate, package, gameplay, Foundation, and Workbench facts were unchanged.
At that historical checkpoint, the exact planned order was a fresh same-package
surface/retention pair, all five package-bound focused suites and their 91-case
aggregate, the corrected canary, and Full Campaign Debug only after an accepted
canary.

CRI-103 accepted fresh then-current-tool surface leaf
`20260722T133630Z-928aa875113841838469f7222a9c11fb` and retention leaf
`20260722T133757Z-45b0703ae767` under clean harness
`07e71c12159f5287c6af2dd7901a392ef9c16bb9`. The surface tracked-index and
ready-seal SHA-256 values are
`68a74d91208e1873c83258c9c15158f972914f40d4a1713d2f25c56d0ef7ad73` and
`9286c28a7b14b2a3b11b205e74a61c036a92097a763205aef914ba27412a273c`;
the retention values are
`f4170f2afd1342a958aaaf5daaacc968d66508324069f73dca3350c819a36e75` and
`9906f74c168ec99d98a6d8dbd5368b925fae03a19265485a3b5ed28cd1326bb8`.
Surface retained 41 files with exact per-mode 2/2/1/0 mount attestation, zero
hard diagnostics, and zero unapproved diagnostics. Retention retained 251 files
with exact candidate tuple, canonical digest, ten guarded receipts, and argument
topology across five diagnostic plus five standard contexts; it does not
project mount counts. Both halves independently reverified read-only and were
jointly consumed, closing `STATUS-008` again without certification promotion.
The immutable candidate package remains external and untracked as historical,
optional QA rather than a repository/source deliverable or release authority.
Workbench-to-Workshop remains the compilation and publication path, and the game
downloads the resulting addon. The former plan would next have run all five
focused suites and their 91-case aggregate, then the corrected canary, then Full
Campaign Debug only after an accepted canary.

CRI-104 preserves that next five-suite attempt as rejected forensic batch
`batch-71b276f-run-id-start-second-drift`. Clean harness
`71b276fa9d34a1100c1ba2a27ff509a37619dbe0` used focused-runner SHA-256
`095ccd952e9eaaaa6f94c40053299ed5c2c8e7147ce390a29b5bd14f14da0e93`.
The complete byte-exact set contains 45 files totaling 1,365,323 bytes. Raw
JUnit was 91/0/0/0, each suite carried exact 2/2/1/0 candidate-mount
attestation, hard diagnostics were exactly 51 = 10 approved stock + 41 approved
intentional + 0 unapproved, and cleanup/spill counters were zero.

The five leaf identities and canonical `run.json` SHA-256 values are:

- counterattack
  `20260722T140834Z-a147cc36e35d4d43ba48b5f89f59de9e`:
  `f68f7a9185c687d61121ee0a0e8c71a3f877347c7861017a591ac5cff8c82853`;
- garrison rebuild
  `20260722T140940Z-a3bd4c5a9f874e7f85a19553ca3435e3`:
  `ef773327a415f376e61fff6fcdb5863d34c049f627ed0f5b5443f38d87b869f2`;
- planning commitment
  `20260722T141005Z-065cb0af71874e3f9ee9f137b7381b70`:
  `eb1279d4ee750b003342ed82b89b3e9453addffc0636320931a21d60d090dea8`;
- QRF `20260722T141031Z-06a4861e909b4794951b7aac82d052c2`:
  `d20d4ceedc23a98763fe230d2bc916b443cb16254d2a2338e36b01c8a7c4801a`;
- profile journal authority
  `20260722T141058Z-f74ed174600d436ba089f7060de5038a`:
  `4c69f6bd7a91120d5783d59a476146f0de302bd0d8a8c08307fb64d1c00624ae`.

Counterattack records start `2026-07-22T14:08:33.9995401Z`, one second before
its `140834Z` prefix. QRF records start `2026-07-22T14:10:30.9860749Z`, one
second before its `141031Z` prefix. A second clock read used for leaf naming
crossed the boundary. The other three leaves aligned by timing chance but still
share one rejected tool/harness batch and cannot transfer.

The aggregate producer correctly rejected `policy_drift`, returned
`RED/replacement-required`, and created no aggregate or durable receipt. The
runner now derives the leaf prefix and `run.json.startedUtc` from one exact
`evidenceStartUtc`; strict consumers remain unchanged and retain both
near-boundary positive and explicit mismatch-negative coverage. The optional
historical package-QA chain is frozen: its five package-bound suites, historical
CRI-103 pair, and closed `STATUS-008` remain immutable history. Current
source-native Gate 1 and release `NO-GO` remain open.

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

The then-active Gate 1 candidate under the retired local-QA workflow was
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
immutable, and untracked. The passed `52c7e2b`, `e2c38d2`, and CRI-101 surface/
retention pairs remain immutable history under their recorded tool bytes. The
first `87b5ad4` focused batch is preserved as superseded/rejected history.
CRI-103 accepted then-current-tool surface leaf
`20260722T133630Z-928aa875113841838469f7222a9c11fb` and retention leaf
`20260722T133757Z-45b0703ae767`; both independently reverified read-only and were
jointly consumed. Candidate, package, gameplay, Foundation, and Workbench
evidence were unchanged. At that historical checkpoint, `STATUS-008` was closed
and the planned next order was all five package-bound focused suites and their
91-case aggregate, the corrected canary, and Full Campaign Debug only after an
accepted canary. CRI-104 preserves the intervening
`71b276f` batch as rejected timestamp-identity evidence: raw JUnit 91/0/0/0
does not transfer because `policy_drift` prevented any aggregate or durable
receipt. All five package-bound suites would have to rerun after the one-clock
correction only if the optional historical chain were resumed. Current
source-native Gate 1 remains incomplete and release remains `NO-GO`.

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
The candidate/package remains immutable; under that retired workflow, source
fixes required a new candidate. Release remains `NO-GO`.

Source commit `12f87e9` subsequently repairs the obsolete classifier
expectation and attempts an HQ teardown identity repair. It requires
the 13 retained intentional convoy lines as 9 admission, 3 corruption, and 1
watchdog diagnostic, expands the classifier to 36 self-tests, and caches exact
HQ arsenal prefab identity before teardown. A later source-native canary
disproved that identity path; current source caches exact action-filter
component presence instead. This is repair history only: neither correction
alters the portable ee0 summary or its captured 33-check census.

Source commits `64d1f70` and `ebaaeca` then repair compacted paid-support
confirmation replay and bounded convoy-contact seating recovery. The archive
proof now intentionally supplies no live planning services; the convoy retry is
limited to exact degraded/rebind evidence, runtime roots, living unseated crew,
five-second cadence, and the 45-second grace. Those changes were sealed in the
then-active 5b candidate, but no portable runtime result is attached for them.

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
bound results could be attached to the later 5b candidate
`partisan-rc-5b1f2e98f931-20260721T193941Z` under that retired workflow.

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
