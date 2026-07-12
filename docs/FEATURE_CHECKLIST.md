# h-istasi Feature Checklist

Active development is on campaign Schema 63 and runtime-settings
Schema 23. Schema 63 adds canonical crew-aware combat presence and persisted
`HOT -> COOLING -> COLD` zone diagnostics with a 30-second default cooling
window. Foundation passes with 681 script-symbol references. A normal Workbench
Script Editor open compiled/created the Game module at 5,788 files/11,670 classes
with CRC `a40056c5`, remained responsive without a crash, and produced no HST
script errors. Explicit validation passed for WORKBENCH, PC, XBOX, PS4, and PS5
with exit code `0`; all Workbench instances were closed after testing. Campaign
Debug, packaged runtime, save/restart, and multiplayer execution remain open.

The sealed Schema-63 checkpoint identifies implementation
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

This document tracks the feature-complete campaign target for h-istasi. It is
implementation-focused: every row should eventually map to state fields,
service ownership, server actions, reports, persistence, and a Full Campaign
Debug proof. Implemented, verified, and certified are deliberately separate:
code presence is not runtime evidence, and runtime evidence is not certification
when the proof mutates the campaign it is meant to inspect.

## Final Target

h-istasi should become a Reforger-native, server-authoritative, persistent,
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

The current development save contract is sealed Schema 63. It adds one
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
Script validation pass; Campaign Debug remains open.

Schema 63 layers on the sealed Schema-62 ownership contract. Every location has
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

| Gate | Designed | Implemented | Verified | Certified | Current evidence / blocker |
| --- | --- | --- | --- | --- | --- |
| CRI-0 Truth and baseline | Complete for sealed Schema 63 | Schema 63 canonical combat presence is the latest sealed source/Workbench checkpoint; Schema 62 is the preceding canonical-ownership checkpoint and Schema 61 the marker-projection foundation beneath it | Schema-63 implementation `85a75c65e9c148a890d8d78b0288ae6483a5ccd9`, UTC `2026-07-12T08:22:05Z`, label `schema63-canonical-combat-presence`, Foundation 681, and Workbench CRC `a40056c5` at 5,788 files/11,670 classes are sealed; all five configurations validated, normal open was responsive/no-crash, and zero Workbench processes survived. Historical Schema-62 identity remains `7c93e0a485bcabe5a364c0b0cfeca235accb50f7`, UTC `2026-07-12T06:11:19Z`, label `schema62-canonical-ownership-transition` | No | Packaged/native/save-restart/rendered-UI/stutter-horn/setup/network/reconnect/JIP certification remains independently open. |
| CRI-1 Authority foundation | Complete | Prior vertical slices plus one exact durable radio-site owner | Schema-59 source proof and stamped-tree Workbench gates pass; packaged runtime pending | No | One site row per radio zone owns stable target binding, ONLINE/DESTROYED/REBUILDING state, ownership, mission lock, typed transition, revision, and receipts; each mission owns a distinct physical runtime identity. Stop-rebuild is once per tower-destruction epoch, and stopping its equipment does not advance that epoch. New exact radio missions use contract `1`; active legacy rows fail closed and malformed current rows quarantine at `-59`. |
| CRI-2 Force manifests | Complete for foundation | Durable SpawnQueue, exact infantry adapter, nine exact infantry-family consumers, one exact convoy, plus three externally managed captive slots | Schema-60 source/Workbench gates pass; native projection/restart pending for the new consumer | No | Search-and-Destroy freezes one catalog-backed infantry root/member roster with no vehicles/assets/multi-root fallback. Existing QRF, patrol, rescue, assassination-guard, and convoy shapes remain unchanged. Generic vehicle/asset/multi-root execution, historical mission guards, and unsupported consumers remain open. |
| CRI-3 Force runtime | Complete in source for nine exact infantry-family consumers, one exact convoy, the three-captive rescue lifecycle, and the crewless mixed-group slice | Exact casualty/reprojection, virtual/physical survivor transfer, Search-and-Destroy return-to-assignment, patrol/guard/rescue/convoy folds, and mixed-group cleanup | Schema-60 source/Workbench gates pass; native/package/restart gates pending | No | Search-and-Destroy keeps exact survivors across physical/virtual transitions and simulates hostile-garrison combat only while on station. Projection retirement and capture now fail closed unless exhaustive casualty reconciliation and living root/member binding cardinality succeed, and capture refreshes live position. The deterministic casualty/fold/immediate-recall fixture uses synthetic queue state; live adapter retirement and physical recall exit remain unproven alongside native entities, natural combat/vehicle seating, real save/restart, rendered UI, owner-change, setup, networking, reconnect, and JIP. |
| CRI-4 Operation records | Complete in source for eight operation types and ten explicit family consumers | Separate player Search-and-Destroy type plus the existing QRF, convoy, patrol, mission-guard, and rescue contracts | Schema-60 source/Workbench gates pass; packaged runtime pending | No | Only newly confirmed Search-and-Destroy uses contract `1`; pre-60 rows remain `0`, and malformed current claimants retain evidence under `-60` quarantine. Quarantined groups are globally non-operational/non-combat-present. Exact-support capacity eviction requires a valid unique backlink-free tombstone/terminal-request pair with reciprocal receipts; corrupt pairs remain. Schema 58 rescue and Schema 59 radio contracts remain unchanged. |
| CRI-5 Marker client projection | Complete for marker records in source | Revision/tombstone records, epoch/global sequence, bounded snapshot/delta journal, ownership-derived ACK/gap/resync sessions, JIP/reconnect snapshots, atomic client registry, and client-local native reconciliation | Proof code/Workbench compile only; fixtures not runtime-executed | No | A packaged host plus two clients must prove equal epoch/watermark/hash after initial join, ordered create/update/delete, forced gap/resync, disconnect/reconnect, late join, map close/reopen, and real save/restart. Native widget/root readiness and duplicate-free cleanup remain separate visual gates. Menu/tasks/notifications and dynamic player markers are outside this protocol. |
| CRI-6 Canonical ownership | Complete in Schema-62 source for location owner changes | One revisioned request/receipt routes military, mission, political, admin, debug, and migration causes; valid later requests become pristine queued receipts and array-order execution owns security, support, owner, derived policy, retaliation/economy/events, transactional parent-aware projection, notification, and persistence scheduling | Foundation 670 and Workbench CRC `22c13a32` pass; deterministic runtime execution pending | No | Source proof covers durable queued intent, malformed FIFO authority, full-marker staged rollback, resolver fail-close/unsafe-row purge, setup history across activation, exact support target/prefix and derived correlations, persistence deadline/re-arm, two-child atomic publication, shared logical snapshot construction, and two restart boundaries with exactly-once political completion. Package-prove these paths with exact-patrol settlement, setup/terminal retry, invalid-owner migration, admin accepted-pending reporting, post-liberation security, marker protocol-2 source revision, counterattack/economy/event effects, quarantine, retention, and all callers. Schema 63 changes hostile-pressure input but leaves this transaction boundary intact. |
| CRI-7 Canonical combat presence | Complete in sealed Schema-63 source; Foundation and Workbench validation pass | One shared crew-aware query and zone-heat service consumes fresh registered physical samples or eligible durable virtual infantry. Capture, missions, HQ, civilians, and enemy strategy use the same result; empty/cargo-only vehicles never block. Zone diagnostics follow `HOT -> COOLING -> COLD`, defaulting to 30 seconds, and activation uses a larger exit radius than entry. | Foundation passes at 681 references; normal Workbench open compiled/created 5,788 files/11,670 classes at CRC `a40056c5` with no HST script error or crash, and explicit validation passes for all five configurations. Campaign Debug has not run. | No | Prove conscious/unconscious, dismounted/cargo/pilot/turret, armed/unarmed, mobile/static, destroyed/burning/immobile, sample freshness, virtual casualty continuity, fail-closed authority gaps/player filtering, cooling deadline, conservative migration/restore, no per-second allocation/save churn, all shared consumers, and native activation/deactivation behavior. |

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
| Stable authority IDs and receipts | Server mutations use persisted monotonic IDs, bounded receipts, and replay-safe results. | Implemented Foundation / Needs Runtime Proof | Schema 58 adds reciprocal rescue mission/operation/manifest/batch/guard/captive identity plus stable escort, carrier, seat, request, casualty, extraction, and settlement receipts. Historical POWs and unsupported families remain unversioned; native replay/restart evidence is still required. | Highest |
| Durable operation state | Every force/support/order has an immutable assignment, duty state, engagement mode, materialization state, route progress, recall/return policy, and terminal result. | Implemented Foundation for eight operation types/ten family consumers / Needs Runtime Proof | Schema 60 adds only the player Search-and-Destroy type, with direct-route progress, exact survivor/combat state, displaced-fold return to assignment, and commander-recall settlement. Schema 58 rescue and all earlier narrow contracts remain unchanged. Generalized encounters, vehicle/multi-root policy, JIP, historical missions/garrisons, and other families remain open. | Highest |
| Runtime diagnostics | Every major system has report/debug visibility. | Implemented Foundation | Schema 63 adds bounded sorted combat-presence contributor IDs/facts, separate infantry/manned-mobile/static counts, operation/recent-fire context, revision, state, reason, cooling remainder, and deterministic hash. Foundation and explicit Workbench validation pass, but the source proof has not run; add rendered/admin reporting after runtime execution. | High |

### Persistence And Restart Safety

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Versioned save state | Durable campaign facts survive restarts and schema migration. | Implemented Foundation / Sealed Schema-63 Source Boundary | Schema 63 persists revisioned per-zone combat-presence state and bounded contributor diagnostics while invalidating runtime-only physical samples on every restore. Pre-63 and malformed current authority become canonical `COLD` without inferring combat from vehicle/marker/group rows; valid current heat/cooling shape may survive. Schema 62 ownership receipts, backlinks, marker source revisions, and setup history remain unchanged. Deterministic fixtures remain source evidence; Foundation and explicit Workbench validation pass, while real serialization, restart/resume, reconnect, rendered UI, and JIP proof are open. | Highest |
| Runtime settings migration | Generated profile settings migrate forward without keeping obsolete setup knobs. | Implemented Foundation / Sealed Schema 23 | Schema 23 adds `capture.combatPresenceCoolingSeconds`, defaults it to 30 seconds, normalizes it to `1..300`, and gives older settings the default during migration. Schema 22's five-vehicle true-town default and Schema 21 loot-key cleanup remain intact. Workbench generation/rewrite proof is pending. | Keep |
| Profile fallback saves | Scripted saves work when native persistence is unavailable. | Implemented Foundation / Needs Soak | Repeat restart tests before promising long-campaign safety. | High |
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
| Canonical ownership transition | One server transition updates owner, garrison/security policy, support, facilities/logistics, markers/GM/menu, economy, enemy consequences, events, and persistence. | Implemented in Schema-62 source / Needs Runtime Proof | All causes converge on one immutable receipt; valid followers remain pristine and execute FIFO. Support authority is an exact sorted target set with ordered applied prefix and same-row event/delta evidence. Parent publication stages the full logical marker snapshot, validates/releases the exact child receipt-zone-marker graph, then commits or fully rolls back. Ownership resolution precedes marker correlation; unsafe marker authority purges. Setup no-marker history stays immutable after activation. Schema 63 changes capture's hostile-pressure input, not this ownership transaction. Runtime-prove both boundaries, then continue Blueprint Phase 7 town-truth work. | Highest |

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
| Enemy resource pools | Enemy attack and support capacity grows from map control and pressure. | Broad Alpha / Needs Soak | Tune attack/support income and costs after real background-war runs. | Highest |
| Support spend ledger | Same-zone support stacking, recent damage pressure, spend caps, cooldowns, and refunds are tracked. | Broad Alpha / Exact QRF and patrol settlement source-complete | A schema-51 exact enemy defensive QRF debits its recorded attack/support costs once and settles the survivor fraction once. A schema-53 exact patrol separately debits proactive attack resources, fully refunds a post-debit admission rejection once, and refunds only the exact survivor fraction after commitment. Fold/materialization transfers never refund. Source fixtures cover replay/conflict behavior; packaged accounting and restart proof remain open. Other enemy order families retain older refund rules. | High |
| Enemy commander orders | Counterattacks, rebuilds, roadblocks, support calls, and HQ pressure queue durable orders. | Broad Alpha / Two exact enemy-order consumers source-complete | Newly planned infantry-only defensive QRFs retain their distinct-source outbound/return policy. Newly queued patrols now freeze one infantry manifest, spend proactive attack resources once, create one route/order/operation/batch/group aggregate, bypass the fixed legacy timer, complete one generated-route patrol lap, return, and settle exact survivors. Type-plus-version dispatch isolates QRF, patrol, legacy, quarantine, and unsupported owners. Historical patrol rows plus counterattacks, rebuilds, roadblocks, support calls, and Petros attacks remain legacy. Packaged behavior and long-window proof stay independently open. | Highest |
| Abstract resolution | Off-screen orders and support resolve without needing physical entities. | Broad Alpha / Exact QRF and patrol routes source-complete | The exact enemy defensive QRF continues outbound/return movement while virtual. The exact patrol continues outbound, one closed route lap, and return while virtual, preserves exact casualties, and holds its clock during physical contact. Broader survivor, vehicle, battle, and garrison outcome math remains open. | High |
| Physical response | Near-player enemy responses spawn, move, fight, and fold back. | Broad Alpha / Needs Soak | Newly planned defensive QRFs and newly queued patrols realize exact living slots through SpawnQueue and fold without refund. The patrol restarts its current generated-route leg from the live fold position, retains mapped casualties, and blocks fold/progress during contact. Legacy QRF/support behavior remains separate, including response vehicles and transactional waypoint reissue. Fresh packaged QRF/patrol movement, contact, loop/return, casualties, settlement, marker, duplicate isolation, and restart proof remains open. | Highest |

### Civilians, Town Influence, And Population

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Town support ledger | Events change support, reputation, heat, security, and population with explainable history. | Broad Alpha | Town influence now records durable compact strategic-event rows; radio towers apply bounded `radio_broadcast` support drift and town security applies `security_pressure` police/roadblock drift on income/resource ticks; add expiry/reversal and actor/target faction effects. | Highest |
| Political town flips | Town ownership changes by support majority with hysteresis, not only direct combat. | Broad Alpha / Canonical Schema-62 route | Both directions now use the same expected-owner/revision transition as military capture. Exact influence replay is idempotent, and a capture-triggered child flip delegates publication to its parent. Tune majority thresholds, prevent flicker, and prove nested/runtime behavior. | High |
| Population state | Population remaining/killed affects support, income, victory, and loss. | Broad Alpha / Needs Soak | Town income and town HR now scale/gate from remaining versus killed population, and income reports expose the population multiplier; next tune support-population math, expand event coverage, and soak save/restart terminal states. | Highest |
| Civilian runtime population | True towns use deterministic, non-repeating concrete stock civilian appearances and default to five driven traffic vehicles; minor localities project at most two pedestrians without town-scale parked/traffic vehicles. Civilian proximity is independent from the military zone-active bit. Pedestrians receive CIV wander groups, traffic receives registered vehicles/drivers/routes, ambient driver horn input is cleared, and the native wheeled-vehicle base disables AI horn timing/output for inherited wheeled vehicles. All helpers clean up outside the render bubble. | Broad Alpha / Needs Runtime Proof | Phase 20 checks distinct actors, traffic counts, movement, faction affiliation, scoped driver input reset, and cleanup. The base-vehicle horn override is in the stamped Schema-60 source/Workbench checkpoint; publish and listen around both ambient traffic and other AI-driven wheeled vehicles before treating continuous-horn suppression, multiclient projection, or five-car budgets as runtime-proven. | High |
| Police and roadblocks | Security systems create scan pressure and town-state consequences. | Broad Alpha / Schema-62 liberation policy | Enemy-owned town density still derives from owner relation, heat, support margin, and war level. A canonical resistance takeover now retires enemy police and roadblocks and creates at most two aggregate resistance infantry when capacity exists, with no automatic vehicle; enemy recapture restores scaled infantry and major-site vehicles. Existing exact new-owner authority is retained. Package-test this balance/native projection, then add checkpoint-specific content and deeper scan behavior. | High |

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
| Exact force spawn queue | Each force projection realizes one immutable executable manifest through bounded, retryable, restore-safe work and verified registration. | Implemented Foundation / Needs Runtime Proof | The generic adapter now serves two QRFs, player Search-and-Destroy, exact enemy/garrison patrols, and all three exact assassination-guard contracts with one root plus exact members. Current-count validation uses durable living slots after handoff. Schema 52 retains its convoy-specific path. Generic vehicles/assets/multi-root, historical mission/aggregate forces, event-driven casualty subscription, and packaged proof remain open. | Highest |

### Missions And Objectives

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Category mission selection | Commander chooses mission category; mission and valid target are selected server-side. | Broad Alpha | Gun Shop is now a rarer dynamic candidate; continue tuning category candidate rules and player-facing disabled reasons. | High |
| Mission runtime primitives | HVT, destroy, hold/clear, rescue, cargo, convoy, support, and gun-shop primitives have physical action paths. | Broad Alpha / Exact convoy, guards, first rescue, and radio lifecycle implemented in source | Schema 59 gives radio destruction/rebuild one exact physical-evidence path and separate rebuild equipment while preserving the Schema-58 rescue contract. Borrowed destruction requires reciprocal lock/revision plus authoritative tracked damage state; generated explosive scoring also requires a live mission component, bounded position, mission-time ownership provenance, and one unique key in the persisted bounded evidence set. Generic objective ticks and commander progress are fenced out. Native/packaged behavior remains open. | Highest |
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
| Map markers | HQ, zones, missions, support, QRFs, orders, garrison patrols, and active deliveries publish linked markers with cleanup proof. | Broad Alpha / Schema-62 Protocol-2 Projection / Needs Runtime Proof | Logical campaign markers stream to a widget-independent client registry and reconcile as client-local native markers; server-native campaign publication is retired. Protocol `2` carries zone source revision. Ownership publication is a staged full-snapshot transaction with exact rollback and commit only after all parent/child validation/release. Ownership resolves before retained-marker correlation; quarantine purges unsafe zone-marker rows. Republish and prove host/client atomicity, rendering, cleanup, map reopen, reconnect/JIP, and existing POW presentation. | Highest |
| Authoritative client projection and JIP | Host, clients, reconnects, and late joiners converge on the same snapshot watermark and ordered revisioned create/update/delete stream. | Implemented Foundation for marker records / Needs Runtime Proof | Schema 61 supplies the delivery/ACK model; Schema 62 extends each encoded row with source revision and includes it in equality/hash validation. Prove host/two-client/late-join equality, ownership revision correlation, native rendering, and restart in the packaged mod. Menu snapshots, campaign tasks, general notifications, and dynamic player markers are not yet unified under this protocol. | Highest |
| Modal map targeting | Target selection owns map/input/cursor/modal state through one idempotent state machine. | Broad Alpha / Needs Runtime Proof | Normal map targeting and confirmation flows exist with ESC handling and duplicate-click guards. Prove Closed -> Selecting -> Confirming -> Submitting/Closing behavior, Choose Again re-arm after pointer release, cursor/modal layering, and atomic ESC teardown at supported resolutions. | Highest |
| Map/War information model | Players see contacted town pressure and resistance territory without redundant or misleading rows. | Partial | Replace generic first-zone pressure with contacted towns only, current town first, then ascending resistance support with stable ties/pagination. Add Resistance Territory grouped by canonical owner/type/name and remove redundant capture-status presentation. | High |
| Full Campaign Debug | One button runs a true runtime certification suite and writes structured artifacts without changing the campaign under test. | Broad Alpha / Needs Runtime Proof | Schema 62 source fixtures now include staged marker rollback, pre-projection leak rejection, setup history through activation, exact support target/prefix and same-row event corruption, derived garrison/counterattack/event/marker correlations, persistence deadline/re-arm, FIFO restore, two-child atomic release, linked-support collision, two restarts with exactly-once political completion, all causes, exact-security rejection, migration/quarantine, and retention. The fixtures have not run in Campaign Debug and do not substitute for native entities, real restart, rendered UI, networking, reconnect, JIP, or mandatory process restart. | Highest |
| Scoped debug profiles | Smaller profiles isolate feature families for fast iteration. | Implemented Foundation | Keep profiles explicit and never treat external/restart/soak gaps as PASS. | Keep |
| Build provenance | Runtime logs and artifacts identify the exact code build from one authoritative source. | Implemented Foundation / Needs Packaged Proof | Runtime, menu, admin, and debug artifact summaries now consume `HST_BuildInfo`; prove the stamped identity in a packaged dedicated-server/client run. | High |

### Campaign End And Long-Run Soak

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Victory | Default victory depends on population support and decisive airfield control. | Broad Alpha / Needs Soak | Soak population/airfield outcomes across save/load and tune support thresholds. | Highest |
| Loss | Default loss depends on civilian catastrophe, with optional collapse settings. | Broad Alpha / Needs Soak | Soak killed-population outcomes across save/load and mission/civilian event paths. | Highest |
| Multiplayer soak | Campaign survives co-op, reconnect, restart, active missions, active support, and terminal saves. | Needs Soak | Build repeatable 2/4/8+ player test profiles. | Highest |
| Performance soak | Physical/abstract transitions do not leave stuck groups, duplicate vehicles, missing markers, or periodic frame stalls. | Needs Soak / Stamped Source Repair | The latest user run showed a visible once-per-second stutter. Stamped Schema-60 source keeps pure-vehicle groups on vehicle-count authority, removes redundant and no-convoy whole-group scans, reuses recurring player authority, round-robins unresolved radio queries, and defers expensive visual evidence until its throttled log emits. Publish and measure freezes-per-minute before calling this fixed; then continue the mixed-QRF, duplicate-runtime, survivor-resurrection, and marker-pressure soak. | Highest |

## Highest-Impact Next Tasks

1. Publish the exact sealed Schema-63 package and require the
   same `HST_BuildInfo` identity in
   server/client evidence. Preserve stock HUD/Game Master recovery, then prove
   canonical ownership plus normal-sized radio icons, location-plus-owner
   labels, pointer-over-dialog ordering, one intact tower per radio site, and
   correct destroy/rebuild target behavior.
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
   activation/deactivation hysteresis. Then continue Blueprint Phase 7 with
   broader encounter and town-truth consequences.
3. Runtime-prove civilian projection at several towns plus Figari, Morton, and
   Simon's Wood: five moving traffic vehicles per true town, two pedestrians at
   the minor locality, unique pedestrian/driver appearances after replacement,
   silent horns, correct military strength, and clean render-bubble teardown.
   Require Maiden's Bay to appear only as the Logistics Warehouse and compare
   freezes-per-minute against the observed once-per-second stutter.
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
14. Runtime-prove Schema 61 with one host, two clients, reconnect, and late join.
    Require equal epoch/watermark/registry hash after initial snapshot, ordered
    create/update/delete, a forced dropped-delta resync, map close/reopen, native
    marker cleanup, and real save/restart. Confirm there is no duplicate server-
    native campaign marker set and keep dynamic player markers out of this claim.
15. Schema 58 completes the first separately versioned rescue vertical for
    newly started `rescue_pows`. Runtime-prove its three POW projections,
    vehicle seats, fold/re-entry, death/success/grace outcomes, restart,
    rendered UI, reconnect, and JIP before opting in another rescue family.
16. Runtime-prove Schema 59 radio-site authority: one authored binding with no
    duplicate, physical destroy success, influence removal before income,
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
