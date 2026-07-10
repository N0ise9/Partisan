# h-istasi Feature Checklist

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
| Needs Runtime Proof | Static and compile validation pass, but the named production path has not run in a fresh isolated runtime proof. |
| Unsafe On Live State | A debug or migration path can persist destructive test mutations and must be isolated before use on valuable state. |

## Current Delivery Gate — Campaign Runtime Integrity

`Designed` means the contract and dependency order are recorded. `Implemented`
means the named production slice exists. `Verified` means an appropriate proof
has actually run against that slice. `Certified` additionally requires a safe,
isolated runtime run with no unresolved hard failures or required external gaps.

| Gate | Designed | Implemented | Verified | Certified | Current evidence / blocker |
| --- | --- | --- | --- | --- | --- |
| CRI-0 Truth and baseline | Complete | Complete for this checkpoint | Repository inventory, foundation checks, provisional schema-49 Workbench Game compile/create, prior normal project-open survival, and runtime-evidence audit complete | No | Current code and documentation agree on schema 49 and one build-identity source. The current work-in-progress tree provisionally loads 5,743 files/11,497 classes and creates the game with CRC `fb8cdf64`; final validation and a fresh normal project-open gate still belong to this pass. The latest packaged schema-48 server/client check exposed a separate real regression: Game Master and stock HUD were unavailable because modded config-backed editor, budget, and placed-marker classes omitted their original container/custom-title metadata. Native config loading then failed and editor/HUD initialization cascaded through a null editor manager. Current source restores the matching attributes, but runtime recovery cannot be claimed until the next published build. The earlier one-frame editor role-change listener guard also still needs packaged late-admin proof. Civilian group RPC/movement, convoy seating, physical support routing, crewless mixed-QRF salvage, isolated campaign debug, and restart evidence remain open as described below. The latest Full Campaign Debug artifact predates schemas 43-49 and is not current certification evidence. |
| CRI-1 Authority foundation | Complete | Training, garrison-purchase, player-QRF, typed support-recall, and first versioned operation vertical slices | Static contracts and provisional Workbench script validation pass | No | Persisted monotonic IDs, typed command receipts, bounded campaign events, reserve/commit/cancel/refund transactions, and the schema-49 operation contract cover paid training, exact garrison confirmation, and exact player-QRF confirmation/settlement. Support recall remains the first visible command whose receipt status comes from a typed domain result. Paired exact refunds prevalidate both ledger legs, while operation terminal settlement preflights typed result and deterministic settlement identity. Other visible commands, paid support types, and operation families still use legacy contracts, and packaged save/restart evidence is absent. |
| CRI-2 Force manifests | Complete for foundation | Garrison quote/confirm slice, durable SpawnQueue kernel, first exact infantry adapter, exact player-QRF consumer, first exact force-runtime lifecycle, and bounded accepted-settlement archive | Provisional schema-49 Game-module compile/create pass; physical HST_Dev runtime proof pending | No | Schema 49 retains the schema-48 immutable planning/queue/archive boundary and adds operation settlement evidence to eligible paid-QRF tombstones. Terminal backlink-free garrison/QRF settlements still compact only after 600 seconds into at most 256 replay tombstones with an 86,400-second minimum window; full quotes plus tombstones fail closed at 320 rows. Player QRF still charges $250 plus one HR per authored member and submits the same single-group manifest unchanged to SpawnQueue. The adapter supports only one infantry root plus members. Vehicle, asset, and multi-root execution, executable garrison manifests, event-driven death subscription, lifecycle authority for other consumers, and other paid-support migrations remain open. |
| CRI-3 Force runtime | Complete for the exact infantry and crewless mixed-group slices | Exact member casualty/reprojection plus personnel-authoritative mixed-group terminal transition, salvage disposition, QRF/marker/capture cleanup, and terminal save normalization | Deterministic lifecycle proof and Workbench compilation pass; packaged runtime open | No | Exact paid-QRF slots preserve living/casualty authority and reproject survivors; non-queue mixed groups separately treat living personnel as combat authority. The crewless transition remains source/deterministic proof only for real entity detachment, salvage, replication, and restart. Vehicle/asset/multi-root exact lifecycle and event-driven death subscription remain open. |
| CRI-4 Operation records | Complete for first exact paid infantry-QRF kernel | One versioned record after confirmation; immutable assignment, typed duty/engagement/materialization/position/settlement state, restore normalization, terminal/archive replay | Focused deterministic proof is coordinator-integrated and Workbench-compiles; packaged execution pending | No | New confirmed exact paid player infantry QRFs and uniquely coherent active schema-48 rows use contract version 1. Eight `operation_record.*` assertions cover issue/confirm, materialization, engagement, recall/settlement, restore, migration, archive, and legacy-QRF isolation inside the existing force-authority debug case, but they have not run in a published runtime. The engagement API has no live combat-contact caller. Strategic route cursor/hysteresis, generalized virtualization, vehicles/assets, other support, legacy/enemy QRFs, garrisons, missions, enemy orders, and client/JIP projection remain outside this slice. |

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
| Stable authority IDs and receipts | Server mutations use persisted monotonic IDs, bounded receipts, and replay-safe results. | Implemented Foundation / Needs Runtime Proof | Schema 49 persists the prior allocator/receipt/force/queue/archive identities plus one versioned canonical operation row for each opted-in exact QRF. Support recall receipts carry typed operation identity/status; operation settlement and compact tombstones preserve deterministic replay identity. Other legacy transactions and operation families still need equivalent boundaries, and fresh isolated runtime/restart evidence is required. | Highest |
| Durable operation state | Every force/support/order has an immutable assignment, duty state, engagement mode, materialization state, route progress, recall/return policy, and terminal result. | Implemented Foundation for exact paid infantry QRF / Needs Runtime Proof | Schema 49 separates immutable origin/assignment from mutable tactical target and persists typed duty, resumable engagement, materialization, position authority, settlement, policies, terminal result, timestamps, revision, and execution links for confirmed exact paid player infantry QRFs. Queue, handoff, arrival, restore, recall, terminal, and archive transitions are integrated. Live combat does not yet call the engagement API; strategic route cursor/hysteresis, generalized virtualization, JIP projection, and every other operation family remain open. | Highest |
| Runtime diagnostics | Every major system has report/debug visibility. | Implemented Foundation | Enemy target scoring now has scored-candidate reporting plus relation-order branch proof; support placement now reports player/active-AI clearance; active-group reconciliation and UI layout diagnostics are throttled/off by default so server logs stay usable; add deeper decision reports for town influence and long-run support pressure. | High |

### Persistence And Restart Safety

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Versioned save state | Durable campaign facts survive restarts and schema migration. | Implemented Foundation | Campaign schema 49 deep-copies the schema-48 force/queue/lifecycle/archive state plus canonical exact-QRF operations. Schema-48 backfill creates contract-version-1 records only for uniquely coherent accepted nonterminal exact QRFs and changes no economy, ledger, request, quote, manifest, queue, or group status; every ambiguous, pre-exact, terminal, archived-only, legacy/enemy, or other-support row remains contract `0`. Restore converts open physical/dematerializing operation state to strategic materializing while preserving duty and assignment. Actual process restart, migration, reprojection, and archive replay still need packaged proof. | Highest |
| Runtime settings migration | Generated profile settings migrate forward without keeping obsolete setup knobs. | Implemented Foundation | Schema 21 rewrites generated settings without the old loot alias keys, keeps JSON-safe explanatory comment fields, and preserves known gameplay values. | Keep |
| Profile fallback saves | Scripted saves work when native persistence is unavailable. | Implemented Foundation / Needs Soak | Repeat restart tests before promising long-campaign safety. | High |
| Active runtime restore | Active missions, support, enemy orders, groups, vehicles, garage records, and undercover state restore without duplication. | Broad Alpha / Needs Soak | Build one repeatable restart route that touches all active record types. | Highest |
| Terminal campaign restore | Won/lost campaign state stays ended and does not resume normal services after load. | Broad Alpha | Continue proving terminal-frame HQ/runtime object behavior. | High |

### Strategic Map, Zones, Sites, And Routes

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Stable zone IDs | All strategic state anchors to durable IDs. | Implemented Foundation | Curated Everon location-plan IDs are upserted on top of the existing extras; continue validating route/site coverage during runtime playtests. | High |
| Zone type model | Towns, outposts, resources, factories, radio towers, airfields, seaports, banks, police, hideouts, and mission sites matter. | Broad Alpha | Full Campaign Debug preflight now asserts curated minimum counts and known-ID categories; next add richer economy/support effects per strategic type. | High |
| Generated mission sites | Mission targets are generated from stable anchors and category/site rules. | Broad Alpha | Replace fallback sites with authored or validated site sets. | High |
| Generated routes | Convoys, QRFs, patrols, roadblocks, and mission movement use route-aware paths. | Broad Alpha | Eligible unspawned non-queue-managed legacy support groups can use abstract route progression; exact paid QRF remains a SpawnQueue realization exception. Spawned support/QRF groups instead retain their live member centroid and receive a direct current-to-target or current-to-exit chain, with at most three consecutive reissues until a new-best distance improvement of at least 8m resets the stall budget. Service-owned replacement waypoints are prepared as a complete chain before the old chain is removed. Fresh packaged movement and route-reissue proof remains open; richer garrison and roadblock/patrol waypoint types remain future work. | Highest |
| Physical activation bubble | Near-player zones physicalize and off-screen forces stay abstract unless an active objective requires runtime entities. | Broad Alpha | Legacy search/roadblock and debug-only QRF lifecycle paths can create durable active-group state off-screen, advance simulated routes, and defer spawning until entering the player bubble. Exact paid QRF currently resolves staging and enters SpawnQueue regardless of bubble so its accepted manifest can be realized all-or-nothing; virtual exact operations remain a later ForceRuntime stage. Active non-convoy mission target zones can still force physical activation outside the bubble. | High |
| Canonical ownership transition | One server transition updates owner, garrison/security policy, support, facilities/logistics, markers/GM/menu, economy, enemy consequences, events, and persistence. | Partial | Capture and political flips exist, but side effects remain distributed. Introduce one idempotent ownership-transition service with revisioned output and prove every projection plus save state changes together. | Highest |

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
| HR and faction money | Recruitment, support, training, garrisons, HQ movement, and logistics spend durable resources through exact, auditable transactions. | Broad Alpha | Paid training, visible garrison confirmation, and the paid infantry-QRF slice use the shared ledger. Garrison cost is derived from frozen accepted member slots at $50 and one HR each; player QRF charges a frozen $250 plus one HR per authored member and settles linked transactions once. Supply, search, roadblock, fire, air support, HQ movement, logistics, and remaining legacy mutations still need exact quote/transaction cutovers. | Highest |
| Personal money | Player rewards and personal purchases are distinct from faction money. | Partial | Add clear transfer and reward routing. | Medium |
| Town/resource income | Income scales from town support/population and captured resources. | Broad Alpha | Economy inspection now includes next income plus source-category totals for towns, resources, factories, seaports, airfields, banks, and other owned zones. Town money income is now multiplied by surviving civilian population share, town HR income requires enough surviving population, and Full Campaign Debug proves both the math and report evidence; next make resource-specific effects more visible. | High |
| War level | Strategic control and escalation drive enemy quality, support, missions, detection, and training caps. | Implemented Foundation | Resistance training cap now resolves from war level plus two, maxing at 10, with runtime proof; next tie war level consistently into AI skill/equipment, composition, garage limits, and airfield gates. | Highest |
| Strategic score tuning | Zone values and war thresholds produce a stable campaign pace. | Partial | Choose final score weights and report next meaningful captures. | High |

### Enemy Strategy And Support Pressure

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Enemy resource pools | Enemy attack and support capacity grows from map control and pressure. | Broad Alpha / Needs Soak | Tune attack/support income and costs after real background-war runs. | Highest |
| Support spend ledger | Same-zone support stacking, recent damage pressure, spend caps, cooldowns, and refunds are tracked. | Broad Alpha | Captures and mission outcomes now record short-lived threat signals when they raise aggression; keep this as the reactive defense gate and continue tuning chance/cap/cooldown values. | High |
| Enemy commander orders | Counterattacks, rebuilds, roadblocks, support calls, and HQ pressure queue durable orders. | Broad Alpha | Target scoring now exposes eligible candidates, weighted top-band selection, bookkeeping-zone exclusion, relation-aware owner scoring, relation-order branch proof, and a local-front gate that rejects disconnected rival/neutral targets unless the target is resistance-held or near/linked to same-faction territory. QRF/support/roadblock/counterattack decisions now require recent threat evidence, positive faction aggression, and a deterministic chance pass; enemy roadblock orders physicalize through support requests and publish established map markers; passive HQ knowledge now uses a smaller inner exposure radius and slower one-point gain while wider local activity remains threat-only; next add proactive timing and richer counterattack behavior. | Highest |
| Abstract resolution | Off-screen orders and support resolve without needing physical entities. | Broad Alpha | Add stronger survivor, vehicle, and garrison outcome math. | High |
| Physical response | Near-player enemy responses spawn, move, fight, and fold back. | Broad Alpha / Needs Soak | QRF/support infantry reapplies native RUN movement and tight formation, reconciles living runtime members, and materializes linked unclaimed response vehicles only inside the player event bubble; eligible off-bubble legacy groups remain abstract, while exact paid QRF realizes through SpawnQueue. Mixed personnel/vehicle groups now remain combat-effective only while personnel live: after prior population evidence and population grace, zero living infantry makes the group terminal, resolves any incomplete linked QRF as failed, zeros capture strength, retires the marker, unregisters combat handles, and keeps an intact vehicle as neutral disposable field salvage. Vehicle-only groups retain their existing semantics. Spawned support state follows the living-member centroid instead of elapsed-time interpolation. ETA is an earliest observation only, and normal arrival or recall exit requires two live samples from distinct elapsed seconds within 75m. Exact QRF handoff enters `support_active`; direct target/exit waypoint chains are replaced transactionally and have a three-attempt consecutive-stall budget reset only by a new-best distance improvement of at least 8m. Recall acceptance now follows typed routing/settlement outcomes rather than report prose, while physical exit and survivor HR settlement still require live-distance proof. Restored pre-repair arrival/exit rows are live-distance revalidated once. Fresh packaged actual movement, arrival, recall, crewless-QRF salvage, late-physicalization, and contact proof remains open. | Highest |

### Civilians, Town Influence, And Population

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Town support ledger | Events change support, reputation, heat, security, and population with explainable history. | Broad Alpha | Town influence now records durable compact strategic-event rows; radio towers apply bounded `radio_broadcast` support drift and town security applies `security_pressure` police/roadblock drift on income/resource ticks; add expiry/reversal and actor/target faction effects. | Highest |
| Political town flips | Town ownership changes by support majority with hysteresis, not only direct combat. | Broad Alpha | Tune majority thresholds and prevent flicker. | High |
| Population state | Population remaining/killed affects support, income, victory, and loss. | Broad Alpha / Needs Soak | Town income and town HR now scale/gate from remaining versus killed population, and income reports expose the population multiplier; next tune support-population math, expand event coverage, and soak save/restart terminal states. | Highest |
| Civilian runtime population | Town civilians spawn from the randomized CIV character prefab, receive CIV AI wander helpers and forced CIV faction/group affiliation through a dedicated empty root that now inherits the stock behavior/pathfinding/utility/replication group base, civilian vehicles resolve from the CIV entity catalog, and configured civilian traffic spawns with drivers/routes that despawn outside the render bubble. Initial ambient members use the stock AI-composition attach path instead of the player-group member RPC path; traffic drivers register their vehicle before preferring authority-local pilot entry. | Broad Alpha | Freshly prove zero unregistered group-member RPCs plus real distance-over-time pedestrian/traffic movement, then soak active combat, cleanup, persistence, faction components, and multiclient render-bubble behavior. | High |
| Police and roadblocks | Security systems create scan pressure and town-state consequences. | Broad Alpha | Police and roadblock density now drifts on income/resource ticks from town owner relation, wanted heat, occupier support margin, and war level; enemy-owned town garrisons select dedicated size-2/3/4/5 HST town-police group prefabs and live town-police groups receive cyclic patrol waypoints; enemy towns with civilian `policePresence` now also project a runtime-only town-police patrol even without abstract garrison manpower. Established roadblock support from any faction is visible on the map. Decide and document the liberated-town security policy, then add checkpoint-specific content and deeper scan behavior. | High |

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
| Garrison state | Captured zones store abstract defender manpower and vehicles. | Implemented Foundation / Needs Runtime Proof | New visible recruitment links each accepted immutable manifest to a stable garrison ID and verifies the exact purchase-time aggregate delta before resource commit. The link records acceptance provenance while that garrison record exists; it is not a living-slot roster. Legacy aggregate helpers remain at internal/debug call sites, but neither visible-command dispatcher routes `recruit_zone` to them. Physical survivor identity still needs the ForceRuntime slice. | Highest |
| Recruitment and HR costs | Commander spends an exact, quoted amount of HR/money to recruit a committed force manifest. | Implemented Foundation / Needs Runtime Proof | Selection requests an expiring all-or-nothing server quote; the Forces tab displays its exact count, $50-per-member money total, and one-HR-per-member total; confirmation sends only the quote ID. Server context/catalog revalidation, linked reservations, exact purchase-time aggregate increment/provenance verification, commit/rollback, replay handling, and persistence are implemented. Arsenal/equipment consumption and runtime physical slot registration remain open. | Highest |
| Training | Training improves resistance AI quality and caps by war level. | Broad Alpha / Needs Runtime Proof | Training now uses the shared request ID, durable command receipt, and reserve/commit resource transaction path. Existing quality/capture behavior remains implemented, but the schema-42 authority path has only static and Workbench validation until an isolated runtime case runs. | Highest |
| Static defenses | Players can assign static weapons or emplacements to garrisons. | Missing | Add durable state, placement, capture, and spawn/fold behavior. | Medium |
| Garrison physicalization | Active zones spawn defenders from garrison and composition services. | Broad Alpha | Current activation still does not consume the paid manifest and can choose a different composition. Purchase-only garrison manifests have no executable group root and remain intentionally nondeployable. Build an executable garrison manifest/ForceRuntime path before claiming exact physical rosters, then add route/position variety, vehicle plans, survivor fold-back, successful restore/reprojection, and casualty proof. | Highest |
| Exact force spawn queue | Each force projection realizes one immutable executable manifest through bounded, retryable, restore-safe work and verified registration. | Implemented Foundation / Needs Runtime Proof | Schema 47 connects player-paid QRF to the durable queue and the one-second engine adapter for exactly one infantry group root plus every frozen member. New slot success requires exact prefab, liveness, faction, native-group, Game Master, projection, and applicable seat evidence; cleanup runs asset -> member -> vehicle -> group. All exact slots first produce durable nonterminal `READY_FOR_HANDOFF`; physical-war finalization precedes `SUCCEEDED`. Confirmed casualties become retired slot tombstones, corpse detachment precedes last-death root cleanup, and successful restore queues only root plus durable survivors. QRF initial failure/cancel and post-handoff technical settlement use distinct full versus survivor-only policies. Vehicle/asset/multi-root execution, other paid-support migrations, event-driven casualty subscription, and generalization beyond player QRF remain open. A terminal queue row is historical evidence, and the implemented physical HST_Dev proof is not runtime evidence until run. | Highest |

### Missions And Objectives

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Category mission selection | Commander chooses mission category; mission and valid target are selected server-side. | Broad Alpha | Gun Shop is now a rarer dynamic candidate; continue tuning category candidate rules and player-facing disabled reasons. | High |
| Mission runtime primitives | HVT, destroy, hold/clear, rescue, cargo, convoy, support, and gun-shop primitives have physical action paths. | Broad Alpha | Gun Shop now has runtime stock, stationary seller interaction, fifteen-minute post-purchase expiry cap, no no-purchase penalty, HQ delivery, and arsenal deposit proof; continue replacing MVP primitives with mission-specific props, interactions, and consequences. | Highest |
| Mission persistence | Active missions, objectives, assets, runtime entities, and markers survive restart. | Broad Alpha / Needs Soak | Add active mission restart route to the full soak suite. | Highest |
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
| Radio and intelligence network | Radio sites affect communications, intercepted support, town influence, enemy knowledge, and player intel. | Partial | Radio towers currently drive bounded support drift. Add capability/communications gates, interception rewards, enemy-knowledge effects, and clear player-facing reports. | Medium |
| Medical and logistical recovery | Wounded forces, vehicles, fuel, ammo, repair, rearm, salvage, and strategic resources settle without duplication. | Partial | Vehicle source classification, cargo, garage, ammo points, and support scaffolding exist. Add durable recovery/salvage transactions and exact force/vehicle lifecycle integration. | High |

### UI, Markers, Reports, And Debug Suite

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Command menu | One in-game menu exposes setup, missions, forces, map/war, arsenal, garage, members, and admin controls. | Broad Alpha / Needs Runtime Proof | Paid rows and two-step quote/confirmation flows exist, but the latest packaged schema-48 check could not initialize normal stock HUD or Game Master because modded config-backed classes lost their base container metadata. Matching editor, budget, and marker-entry attributes are restored in source and compile, but menu/HUD/editor recovery plus both two-step flows require a republished client/server proof. | Highest |
| Map markers | HQ, zones, missions, support, QRFs, orders, and active deliveries publish linked markers with cleanup proof. | Broad Alpha / Needs Runtime Proof | QRF visibility and the null-root update guard are implemented, but earlier client evidence still contains 6,806 static-marker exceptions. The newer packaged check failed even earlier because the placed-marker config type lost container/custom-title metadata alongside editor classes, preventing normal HUD/Game Master initialization. Current source restores that metadata. A fresh published client map-open must prove config loading, stock HUD/editor readiness, zero marker-update exceptions, terminal-QRF retirement, and full active-root readiness. | Highest |
| Authoritative client projection and JIP | Host, clients, reconnects, and late joiners converge on the same snapshot watermark and ordered revisioned create/update/delete stream. | Missing | Current menu/marker publication is not a complete campaign projection protocol. Add DTO snapshots, global stream sequence/watermark, per-record revisions/tombstones, acknowledgements, gap detection, resync, and host/two-client/late-join equality proof. | Highest |
| Modal map targeting | Target selection owns map/input/cursor/modal state through one idempotent state machine. | Broad Alpha / Needs Runtime Proof | Normal map targeting and confirmation flows exist with ESC handling and duplicate-click guards. Prove Closed -> Selecting -> Confirming -> Submitting/Closing behavior, Choose Again re-arm after pointer release, cursor/modal layering, and atomic ESC teardown at supported resolutions. | Highest |
| Map/War information model | Players see contacted town pressure and resistance territory without redundant or misleading rows. | Partial | Replace generic first-zone pressure with contacted towns only, current town first, then ascending resistance support with stable ties/pagination. Add Resistance Territory grouped by canonical owner/type/name and remove redundant capture-status presentation. | High |
| Full Campaign Debug | One button runs a true runtime certification suite and writes structured artifacts without changing the campaign under test. | Broad Alpha / Needs Runtime Proof | The latest full run produced 663 cases: 367 PASS, 61 WARN, 218 FAIL, and 17 BLOCKED; it predated isolation and contaminated its save. The runner now rejects every profile outside `HST_Dev`, blocks external/soak profiles, saves the live baseline, runs against a deep clone, diverts checkpoints, and restores the untouched state on complete or cancel. The six audited harness repairs remain unexecuted. Schema-48 archive cases and eight schema-49 `operation_record.*` assertions are now integrated into the force-authority case, but none is engine/runtime evidence until a fresh isolated run executes them; physical cleanup and mandatory session restart proof remain open. | Highest |
| Scoped debug profiles | Smaller profiles isolate feature families for fast iteration. | Implemented Foundation | Keep profiles explicit and never treat external/restart/soak gaps as PASS. | Keep |
| Build provenance | Runtime logs and artifacts identify the exact code build from one authoritative source. | Implemented Foundation / Needs Packaged Proof | Runtime, menu, admin, and debug artifact summaries now consume `HST_BuildInfo`; prove the stamped identity in a packaged dedicated-server/client run. | High |

### Campaign End And Long-Run Soak

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Victory | Default victory depends on population support and decisive airfield control. | Broad Alpha / Needs Soak | Soak population/airfield outcomes across save/load and tune support thresholds. | Highest |
| Loss | Default loss depends on civilian catastrophe, with optional collapse settings. | Broad Alpha / Needs Soak | Soak killed-population outcomes across save/load and mission/civilian event paths. | Highest |
| Multiplayer soak | Campaign survives co-op, reconnect, restart, active missions, active support, and terminal saves. | Needs Soak | Build repeatable 2/4/8+ player test profiles. | Highest |
| Performance soak | Physical/abstract transitions do not leave stuck groups, duplicate vehicles, or missing markers. | Needs Soak | Add long-window background profile and cleanup certification, including repeated mixed-QRF personnel loss, one neutral vehicle detach, zero duplicate runtime records, zero terminal survivor resurrection, and no stale capture or marker pressure. | Highest |

## Highest-Impact Next Tasks

1. Publish the config-container metadata repair and re-run a packaged dedicated-
   server/client connect. Require normal stock HUD initialization, working Game
   Master editor access, zero unknown editor/budget/placed-marker config types,
   and zero recursive player-role `ScriptInvoker` exceptions during late admin
   assignment.
2. Re-run civilian town activation with the stock-based helper root and native
   initial-AI attach path. Require zero unregistered group-member RPCs and real
   distance-over-time pedestrian/traffic movement, not helper counts alone.
3. Prove the new campaign-debug isolation boundary in `HST_Dev`: reject a
   non-development-world request, exercise completion and cancellation, verify
   the live campaign reference and both persistence channels remain unchanged,
   simulate interrupted-run recovery, then restart the development session.
4. Run a fresh owner-client map-open proof for the static-marker lifecycle guard.
   Require every active static marker to have a root and widget component on the
   delayed pass, at least one visible root, and zero update-time exceptions.
5. Runtime-prove the repaired harness observations: boolean `1` versus `true`,
   marker teardown timing, pre-tick economy comparison, the intentional 12-site
   registry count, FAIL-over-WARN headline severity, and support observation
   before group fold. Static validation, Game script compilation, and the normal
   20-second WorldEditor project-open gate pass; a fresh isolated debug artifact
   must replace the historical red rows.
6. Runtime-prove the convoy pre-seat vehicle registration and authority-local
   pilot entry repair with 3/3 seated living drivers. In the same scoped,
   disposable profiles, prove that spawned support actually advances, arrival
   requires two live samples within 75m, recall reaches its exit physically,
   and transactional waypoint reissue stays within the three-attempt cap. Also
   reproduce a previously populated mixed QRF at zero living infantry with an
   intact vehicle and prove one terminal transition, one neutral salvage
   detach, zero capture/marker pressure, replay idempotence, and restart-stable
   zero-survivor state.
7. Run the isolated schema-43 planning, schema-44 queue authority, schema-45
   adapter, schema-46 paid-QRF, schema-47 force-runtime, schema-48 archive, and
   schema-49 operation cases. Pair them with real save/restart replay
   proof that one request
   ID produces one training mutation and one committed debit, one garrison quote
   produces one exact purchase-time strategic increment plus two committed
   debits, and interrupted
   queue work reconciles once without duplicate registration. Typed support
   recall must record accepted terminal wording as APPLIED, reject identity or
   settleability conflicts before either paired refund, and replay the same
   durable receipt without a second settlement. The physical case
   must prove exact group/member registration and cleanup in an actual runtime;
   code presence alone does not satisfy it.
8. Extend exact paid support beyond the completed infantry-QRF slice only after
   vehicle/asset manifests and their settlement policies exist; supply, search,
   roadblock, fire, and air support remain legacy.
9. Runtime-prove bounded settlement archival: a terminal backlink-free garrison
   and paid QRF must compact once, survive restart, replay issue/confirmation/
   committed-ledger identity without mutation, reject conflicts, and retain every
   active projection pin. Drive enough disposable rows to prove both capacity
   admission and post-window eviction behavior.
10. Runtime-prove schema-47 paid-QRF survivor reprojection and exact-member death:
   native/GM/strategic living counts must agree, each corpse must detach without
   deletion, and the last death must remove the root and marker within five
   seconds. Then replace bounded life-state polling with event subscription and
   generalize the lifecycle to vehicle/assets and every exact force consumer.
11. Runtime-prove the schema-49 exact-QRF `OperationRecord` kernel through issue/
    confirmation boundaries, queue/handoff/arrival, legal and rejected
    engagement transitions, recall, typed terminal settlement, restore
    normalization, schema-48 migration, archival, and replay. Then add live
    contact/disengagement wiring, strategic route progress/cursor/hysteresis,
    generalized virtualization, and later operation families without widening
    the current exact paid-infantry-QRF claim.
12. Resume mission, civilian, undercover, and town-influence depth only after the
   authority and certification foundations can produce trustworthy evidence.
13. Run repeated packaged multiplayer reconnect/restart soak across active
   missions, support, orders, garage, undercover, and terminal campaign states.
14. Tune economy, war level, aggression, support pressure, and mission pacing
    only after runtime correctness failures are separated from harness defects.

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
