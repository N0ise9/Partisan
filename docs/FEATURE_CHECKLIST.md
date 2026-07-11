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
| Needs Runtime Proof | The source path exists, but required compile/startup and/or isolated runtime evidence is still missing; the row names the open gate. |
| Unsafe On Live State | A debug or migration path can persist destructive test mutations and must be isolated before use on valuable state. |

## Current Delivery Gate — Campaign Runtime Integrity

`Designed` means the contract and dependency order are recorded. `Implemented`
means the named production slice exists. `Verified` means an appropriate proof
has actually run against that slice. `Certified` additionally requires a safe,
isolated runtime run with no unresolved hard failures or required external gaps.

The current development save contract is Schema 55. Its source implementation
adds exact authority only for newly started `assassinate_officer` guard infantry;
historical officer missions and every other mission family remain contract `0`.
Schema-55 source and Workbench gates pass on the stamped tree; packaged behavior
gates remain open.

| Gate | Designed | Implemented | Verified | Certified | Current evidence / blocker |
| --- | --- | --- | --- | --- | --- |
| CRI-0 Truth and baseline | Complete | Complete for the current Schema-55 source tree | Stamped Schema-55 source/Workbench checkpoint verified; packaged runtime pending | No | Implementation `552c2c4ff5ac7608fa248c614480a254769b61a4` passes the full foundation gate, clean Workbench Game validation at 5,763 files/11,570 classes with CRC `0ec8950e`, and a ten-sample/20-second normal editor open. The packaged schema-49 run proved restored stock HUD/Game Master and exposed later-repaired marker/dialog/radio/civilian defects. The latest Full Campaign Debug artifact predates schemas 43-55 and is not current certification evidence. |
| CRI-1 Authority foundation | Complete | Prior vertical slices plus Schema-55 exact officer-mission guard authority | Schema-55 stamped and Workbench-verified; packaged runtime pending | No | Only guard infantry created with a newly started `assassinate_officer` mission gains reciprocal mission/operation/manifest/held-batch/group authority. The HVT stays separate. Historical officer missions, all other mission families, policy-v1 purchases, and initial/enemy aggregate garrisons remain on prior contracts. Typed guard outcomes record one zero-refund receipt; malformed current rows use `-55` quarantine without fallback while the HVT remains playable. |
| CRI-2 Force manifests | Complete for foundation | Durable SpawnQueue, exact infantry adapter, five exact infantry consumers, and exact three-element mission convoy | Schema-55 source/Workbench gates pass; native projection/restart pending | No | Schema 55 freezes one catalog-backed `NotSpawned` empty execution root plus ordered officer-guard members, with no HVT, route, vehicle, asset, or resource entry. The generic adapter remains one-root infantry-only; the convoy retains its separate PhysicalWar path. Historical mission guards, aggregate garrisons, vehicles/assets, generic multi-root execution, and other consumers remain open. |
| CRI-3 Force runtime | Complete in source for five exact infantry operations, one exact mission convoy, and the crewless mixed-group slice | Exact casualty/reprojection, virtual/physical survivor transfer, patrol and mission-guard fold, convoy-element fold/rematerialization, and mixed-group cleanup | Schema-55 source/Workbench gates pass; native/package gates pending | No | The officer guard is a route-less held roster whose mapped casualties persist across fold/re-entry; all guards dead can settle the guard while leaving the HVT mission active. Exact and quarantined guard claimants are isolated from generic mission/group/zone mutation. Native creation, adapter bindings, casualties, return-to-anchor behavior, save/restart, owner-change, campaign setup, networking, and rendered UI remain unproven. |
| CRI-4 Operation records | Complete for six explicit operation consumers | Six explicit version-1 operation types with immutable assignment, typed materialization/position/settlement state, durable roster linkage, restore normalization, and fail-closed legacy isolation | Schema-55 source fixtures and Workbench gates pass; not packaged proof | No | Schema 55 adds `HST_OPERATION_TYPE_MISSION_GUARD` only for new officer-assassination guards: route-less on-station survivors, HVT-independent `DESTROYED`, mission-driven `COMPLETED`/`CANCELLED`, owner-change `INVALIDATED`, coherent spawn/assignment `SPAWN_FAILED`, zero refund, existing-HVT status projection, and `-55` quarantine. Historical officer missions and every other mission family remain contract `0`. |

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
| Stable authority IDs and receipts | Server mutations use persisted monotonic IDs, bounded receipts, and replay-safe results. | Implemented Foundation / Needs Runtime Proof | Schema 55 retains prior authority and adds reciprocal officer-mission/guard-operation/manifest/held-batch/group identity plus deterministic zero-refund terminal settlement. The HVT remains separate objective/runtime-asset authority. Historical officer missions, other mission families, aggregates, and legacy transactions still need equivalent boundaries; isolated runtime/restart evidence is required. | Highest |
| Durable operation state | Every force/support/order has an immutable assignment, duty state, engagement mode, materialization state, route progress, recall/return policy, and terminal result. | Implemented Foundation for six exact operation consumers / Needs Runtime Proof | Schema 55 adds a route-less officer guard that stays on station, transfers exact survivors, settles all-dead independently of its HVT, follows typed mission/owner/spawn outcomes, and refunds zero. There is no virtual guard combat. Off-screen encounter simulation, generalized contact, terrain/ammunition, vehicles/multi-root, JIP, historical missions/garrisons, and other families remain open. | Highest |
| Runtime diagnostics | Every major system has report/debug visibility. | Implemented Foundation | Enemy target scoring now has scored-candidate reporting plus relation-order branch proof; support placement now reports player/active-AI clearance; active-group reconciliation and UI layout diagnostics are throttled/off by default so server logs stay usable; add deeper decision reports for town influence and long-run support pressure. | High |

### Persistence And Restart Safety

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Versioned save state | Durable campaign facts survive restarts and schema migration. | Implemented Foundation | Campaign schema 55 adds exact officer-mission guard graph/casualty/settlement validation. Pre-55 officer missions, HVTs, and legacy `mission_group_*` guards stay contract `0` with no invented authority. Coherent current physical-shaped rows normalize to held survivors; compact settled graphs may omit batch/group/terminal HVT runtime rows. Malformed rows use `-55` quarantine with no fallback, refund, guessed casualty, or HVT backlink, and do not fail the playable HVT mission. Actual process restart, rematerialization, owner-change/setup, rendered marker/UI, and JIP proof remain open. | Highest |
| Runtime settings migration | Generated profile settings migrate forward without keeping obsolete setup knobs. | Implemented Foundation | Schema 22 retains the schema-21 loot-key cleanup, changes the prior shipped civilian-traffic default from two to five for true towns, and preserves non-default traffic values. | Keep |
| Profile fallback saves | Scripted saves work when native persistence is unavailable. | Implemented Foundation / Needs Soak | Repeat restart tests before promising long-campaign safety. | High |
| Active runtime restore | Active missions, support, enemy orders, groups, vehicles, garage records, and undercover state restore without duplication. | Broad Alpha / Needs Soak | Build one repeatable restart route that touches all active record types. | Highest |
| Terminal campaign restore | Won/lost campaign state stays ended and does not resume normal services after load. | Broad Alpha | Continue proving terminal-frame HQ/runtime object behavior. | High |

### Strategic Map, Zones, Sites, And Routes

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Stable zone IDs | All strategic state anchors to durable IDs. | Implemented Foundation | Curated Everon location-plan IDs are upserted on top of the existing extras; continue validating route/site coverage during runtime playtests. | High |
| Zone type model | Towns, outposts, resources, factories, radio towers, airfields, seaports, banks, police, hideouts, and mission sites matter. | Broad Alpha | Figari and Morton remain stock town-center locations. Simon's Wood is normalized in place as a food/resource farm with a small guard footprint and two ambient civilians, rather than duplicating or treating it as a full town. Full Campaign Debug preflight asserts the taxonomy; continue reviewing the remaining minor named localities. | High |
| Generated mission sites | Mission targets are generated from stable anchors and category/site rules. | Broad Alpha | Replace fallback sites with authored or validated site sets. | High |
| Generated routes | Convoys, QRFs, patrols, roadblocks, and mission movement use route-aware paths. | Broad Alpha / Needs Runtime Proof | The earlier exact cursors retain their behavior. Schema 55 deliberately creates no route for officer guards: their immutable assignment is a deterministic offset from the separate HVT and they remain on station. Historical officer guards and all other mission families retain prior paths. | Highest |
| Physical activation bubble | Near-player zones physicalize and off-screen forces stay abstract unless an active objective requires runtime entities. | Broad Alpha | Six canonical operation consumers now exist. The schema-55 officer guard releases only living slots inside the bubble and folds the same survivors outside it while remaining on station virtually. PhysicalWar and generic mission/group owners isolate exact and quarantined guard claimants. Historical missions retain the older path; packaged native projection/fold remains open. | Highest |
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
| Support spend ledger | Same-zone support stacking, recent damage pressure, spend caps, cooldowns, and refunds are tracked. | Broad Alpha / Exact QRF and patrol settlement source-complete | A schema-51 exact enemy defensive QRF debits its recorded attack/support costs once and settles the survivor fraction once. A schema-53 exact patrol separately debits proactive attack resources, fully refunds a post-debit admission rejection once, and refunds only the exact survivor fraction after commitment. Fold/materialization transfers never refund. Source fixtures cover replay/conflict behavior; packaged accounting and restart proof remain open. Other enemy order families retain older refund rules. | High |
| Enemy commander orders | Counterattacks, rebuilds, roadblocks, support calls, and HQ pressure queue durable orders. | Broad Alpha / Two exact enemy-order consumers source-complete | Newly planned infantry-only defensive QRFs retain their distinct-source outbound/return policy. Newly queued patrols now freeze one infantry manifest, spend proactive attack resources once, create one route/order/operation/batch/group aggregate, bypass the fixed legacy timer, complete one generated-route patrol lap, return, and settle exact survivors. Type-plus-version dispatch isolates QRF, patrol, legacy, quarantine, and unsupported owners. Historical patrol rows plus counterattacks, rebuilds, roadblocks, support calls, and Petros attacks remain legacy. Packaged behavior and long-window proof stay independently open. | Highest |
| Abstract resolution | Off-screen orders and support resolve without needing physical entities. | Broad Alpha / Exact QRF and patrol routes source-complete | The exact enemy defensive QRF continues outbound/return movement while virtual. The exact patrol continues outbound, one closed route lap, and return while virtual, preserves exact casualties, and holds its clock during physical contact. Broader survivor, vehicle, battle, and garrison outcome math remains open. | High |
| Physical response | Near-player enemy responses spawn, move, fight, and fold back. | Broad Alpha / Needs Soak | Newly planned defensive QRFs and newly queued patrols realize exact living slots through SpawnQueue and fold without refund. The patrol restarts its current generated-route leg from the live fold position, retains mapped casualties, and blocks fold/progress during contact. Legacy QRF/support behavior remains separate, including response vehicles and transactional waypoint reissue. Fresh packaged QRF/patrol movement, contact, loop/return, casualties, settlement, marker, duplicate isolation, and restart proof remains open. | Highest |

### Civilians, Town Influence, And Population

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Town support ledger | Events change support, reputation, heat, security, and population with explainable history. | Broad Alpha | Town influence now records durable compact strategic-event rows; radio towers apply bounded `radio_broadcast` support drift and town security applies `security_pressure` police/roadblock drift on income/resource ticks; add expiry/reversal and actor/target faction effects. | Highest |
| Political town flips | Town ownership changes by support majority with hysteresis, not only direct combat. | Broad Alpha | Tune majority thresholds and prevent flicker. | High |
| Population state | Population remaining/killed affects support, income, victory, and loss. | Broad Alpha / Needs Soak | Town income and town HR now scale/gate from remaining versus killed population, and income reports expose the population multiplier; next tune support-population math, expand event coverage, and soak save/restart terminal states. | Highest |
| Civilian runtime population | True towns use deterministic, non-repeating concrete stock civilian appearances and default to five driven traffic vehicles; minor localities project at most two pedestrians without town-scale parked/traffic vehicles. Civilian proximity is independent from the military zone-active bit, while the military path now suppresses only locations whose capture footprint actually contains HQ. Pedestrians receive CIV wander groups, traffic receives registered vehicles/drivers/routes, driver horn input is cleared every frame, and all helpers clean up outside the render bubble. | Broad Alpha / Needs Runtime Proof | Phase 20 now checks distinct pedestrian prefabs, inactive-zone civilian eligibility, traffic counts, driver-controller horn reset, movement, faction affiliation, and cleanup. Publish and soak the change before treating audible horn suppression, multiclient projection, or five-car traffic budgets as runtime-proven. | High |
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
| Garrison state | Captured zones store abstract defender manpower and vehicles. | Implemented Foundation / Needs Runtime Proof | A new policy-v2 resistance purchase links its immutable manifest to the stable garrison while the held batch's living slots, not `m_iInfantryCount`, own its exact manpower. Historical policy-v1/initial/enemy defenders remain aggregate. Capacity sums legacy infantry, exact living/reserved manifests, and active legacy infantry without double counting. Typed-quarantine projections reconcile mapped deaths, retire unambiguous runtime ownership, terminalize outside normal queue capacity, and remain non-operational so diagnostic survivors cannot exert capture or placement pressure. Vehicle and static-assignment roster authority remains open. | Highest |
| Recruitment and HR costs | Commander spends an exact, quoted amount of HR/money to recruit a committed force manifest. | Implemented Foundation / Needs Runtime Proof | Selection still uses an expiring all-or-nothing quote and confirmation sends only its ID. Policy-v2 now freezes an empty executable root and the arbitrary priced member roster, atomically commits linked money/HR transactions and exact patrol authority, and replays without duplicate charge. Reports, map eligibility, and legacy direct recruitment reserve exact living patrol slots alongside legacy/active infantry; direct recruitment rejects partial capacity and verifies its full delta before charging. Terminal operation outcomes refund zero. Arsenal/equipment consumption and packaged execution remain open. | Highest |
| Training | Training improves resistance AI quality and caps by war level. | Broad Alpha / Needs Runtime Proof | Training now uses the shared request ID, durable command receipt, and reserve/commit resource transaction path. Existing quality/capture behavior remains implemented, but the schema-42 authority path has only static and Workbench validation until an isolated runtime case runs. | Highest |
| Static defenses | Players can assign static weapons or emplacements to garrisons. | Missing | Add durable state, placement, capture, and spawn/fold behavior. | Medium |
| Garrison physicalization | Active zones spawn defenders from garrison and composition services. | Broad Alpha / Exact policy-v2 infantry slice source-complete | Only new policy-v2 purchased resistance garrisons use an exact empty-root/member manifest, held virtual local loop, survivor-only materialization/fold, restore normalization, no-refund terminal settlement, and marker/UI projection. PhysicalWar legacy owners are excluded from those groups. Historical policy-v1, initial-map and enemy aggregate garrisons still use broad composition and count fold; vehicles, multi-root plans, static assignments, and packaged native casualty/fold/save proof remain open. | Highest |
| Exact force spawn queue | Each force projection realizes one immutable executable manifest through bounded, retryable, restore-safe work and verified registration. | Implemented Foundation / Needs Runtime Proof | The generic adapter now serves two QRFs, the exact enemy patrol, exact purchased-garrison patrol, and exact officer-mission guard with one root plus exact members. Its current-count check uses durable living slots after handoff so partial survivor rematerialization remains valid. Schema 52 retains its convoy-specific path. Generic vehicles/assets/multi-root, historical mission/aggregate forces, event-driven casualty subscription, and packaged proof remain open. | Highest |

### Missions And Objectives

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Category mission selection | Commander chooses mission category; mission and valid target are selected server-side. | Broad Alpha | Gun Shop is now a rarer dynamic candidate; continue tuning category candidate rules and player-facing disabled reasons. | High |
| Mission runtime primitives | HVT, destroy, hold/clear, rescue, cargo, convoy, support, and gun-shop primitives have physical action paths. | Broad Alpha / Exact convoy and officer-guard slices source-complete | Schema 52 retains its narrow exact convoy behavior. Schema 55 gives only newly started `assassinate_officer` guards one route-less empty-root/member operation while keeping the HVT outside manifest/runtime ownership. Guard elimination settles only that operation; mission outcomes settle surviving guards through typed zero-refund results. Historical officer missions, `assassinate_traitor`, `assassinate_specops`, and every other mission family remain contract `0`. Native guard behavior, HVT/guard independence, save/restart, owner-change, setup, and rendered UI remain open proof. | Highest |
| Mission persistence | Active missions, objectives, assets, runtime entities, and markers survive restart. | Broad Alpha / Needs Soak | Schema 55 preserves exact guard survivors and compact settlements while keeping the separate HVT mission playable under `-55` quarantine. Add a real active officer-mission restart route covering physical fold, all-guards-dead/HVT-active, typed mission outcomes, quarantine, and existing-marker/UI recovery to the full soak suite. | Highest |
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
| Command menu | One in-game menu exposes setup, missions, forces, map/war, arsenal, garage, members, and admin controls. | Broad Alpha / Needs Runtime Proof | Packaged schema 49 proved that stock HUD and Game Master access were restored and exercised map-target support confirmation; it does not certify later source. Current source keeps map overlays below the native pointer, and the final stamped schema-55 tree passes Workbench source validation. Republished proof is still required for HUD/Game Master preservation, both quote/confirmation flows, choose-again/ESC teardown, pointer layering, and supported resolutions. | Highest |
| Map markers | HQ, zones, missions, support, QRFs, orders, garrison patrols, and active deliveries publish linked markers with cleanup proof. | Broad Alpha / Needs Runtime Proof | Schema 55 reuses the existing HVT marker and mission UI row, appending `guards N`, `guards neutralized`, or `guard authority unavailable`; it does not publish a duplicate operation marker. Earlier radio/convoy/enemy-patrol/garrison fixes remain source-only. Republish and prove rendered status, cleanup, quarantine presentation, reconnect/JIP, and radio destroy/rebuild behavior. | Highest |
| Authoritative client projection and JIP | Host, clients, reconnects, and late joiners converge on the same snapshot watermark and ordered revisioned create/update/delete stream. | Missing | Current menu/marker publication is not a complete campaign projection protocol. Add DTO snapshots, global stream sequence/watermark, per-record revisions/tombstones, acknowledgements, gap detection, resync, and host/two-client/late-join equality proof. | Highest |
| Modal map targeting | Target selection owns map/input/cursor/modal state through one idempotent state machine. | Broad Alpha / Needs Runtime Proof | Normal map targeting and confirmation flows exist with ESC handling and duplicate-click guards. Prove Closed -> Selecting -> Confirming -> Submitting/Closing behavior, Choose Again re-arm after pointer release, cursor/modal layering, and atomic ESC teardown at supported resolutions. | Highest |
| Map/War information model | Players see contacted town pressure and resistance territory without redundant or misleading rows. | Partial | Replace generic first-zone pressure with contacted towns only, current town first, then ascending resistance support with stable ties/pagination. Add Resistance Territory grouped by canonical owner/type/name and remove redundant capture-status presentation. | High |
| Full Campaign Debug | One button runs a true runtime certification suite and writes structured artifacts without changing the campaign under test. | Broad Alpha / Needs Runtime Proof | The latest full run produced 663 cases: 367 PASS, 61 WARN, 218 FAIL, and 17 BLOCKED; it predates schemas 43-55 and contaminated its save. Schema-55 `mission_guard.*` admission/isolation, projection/HVT separation, settlement, restore/migration, quarantine, and marker-status fixtures are source evidence only. Native entity/adapter/casualty behavior, save/restart, owner-change/setup, rendered UI, networking, and mandatory process restart still need packaged proof. | Highest |
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

1. Publish the stamped schema-55 build and run a focused client/server feedback pass. Confirm that
   the stock HUD/Game Master recovery already observed in packaged schema 49 is
   preserved, then require normal-sized radio icons, location-plus-owner labels,
   pointer-over-dialog ordering, one intact tower per radio site, and correct
   destroy/rebuild target behavior.
2. Runtime-prove civilian projection at several towns plus Figari, Morton, and
   Simon's Wood: five moving traffic vehicles per true town, two pedestrians at
   the minor locality, unique pedestrian/driver appearances after replacement,
   silent horns, correct military strength, and clean render-bubble teardown.
3. Runtime-prove the schema-50 exact paid infantry-QRF operation through ten
   off-bubble minutes, materialize/fold hysteresis, exact casualties, on-station
   virtual combat, recall, pre/post-commit failure settlement, and save/restart at
   every projection state. Do not promote source assertions to runtime proof.
4. Runtime-prove the schema-51 infantry-only enemy defensive QRF: one prepaid
   frozen roster from a distinct source, no parallel legacy response, strategic
   outbound/return travel, materialization/fold hysteresis, exact casualties,
   two-sample physical arrival, once-only defensive pressure, proportional
   survivor refund, marker movement/cleanup, and save/restart replay. Do not
   widen the claim to counterattacks, vehicles, patrols, or convoys.
5. Runtime-prove the schema-52 exact mission convoy: one frozen route, exactly
   three vehicle/crew elements, optional cargo/captive on vehicle zero, virtual
   travel without timer arrival, 3/3 physical drivers, interception, contact-
   to-clear transition, exact non-suffix casualty identity, partial crewless-
   vehicle and cargo-only ground recovery, player-bound cargo fold, duplicate-
   free capture-to-garage handoff, casualty-preserving fold/rematerialization,
   two-sample arrival, once-only outcome/settlement, aggregate current/
   destination marker cleanup, and save/restart. Historical
   restored convoys must remain contract version `0`.
6. Runtime-prove the schema-53 exact enemy patrol: one proactive debit, one
   frozen infantry root, outbound virtual movement, physical materialization,
   mapped casualty fold/reprojection, contact-held progress, one closed route
   lap, return, survivor refund, marker cleanup, and save/restart. Historical
   patrols must remain contract version `0`, and corrupt current rows must remain
   quarantined without legacy fallback.
7. Runtime-prove the schema-54 exact purchased-garrison patrol: only a new
   policy-v2 resistance purchase gets one exact empty root and arbitrary member
   roster, a held virtual infinite local loop, survivor-only materialization/
   fold, owner-change/all-dead/campaign-stop/setup and typed spawn/route-failure
   no-refund settlement, marker/UI cleanup, and save/restart/JIP continuity.
   Historical policy-v1, initial/enemy
    aggregate, vehicle, and multi-root garrisons must remain legacy.
8. Runtime-prove the schema-55 exact officer-mission guard: only a newly started
   `assassinate_officer` mission gets one route-less empty root and ordered guard
   roster. Prove survivor-only materialization/fold, all-guards-dead/HVT-active
   independence, all typed zero-refund outcomes, compact settlement, `-55`
   quarantine without fallback or HVT failure, existing-HVT marker/UI status,
   native adapter/casualties, save/restart, owner-change, campaign setup,
   networking, reconnect, and JIP. Historical officer missions and every other
   mission family must remain contract `0`.
9. Execute the isolated campaign-debug suite in `HST_Dev`, prove completion,
    cancellation, interrupted-run recovery, and unchanged live persistence, then
    replace the historical contaminated artifact.
10. Add the authoritative snapshot/delta/JIP projection protocol and prove host,
    client, reconnect, and late-join equality with revisions and tombstones.
11. Introduce one idempotent ownership-transition service before adding virtual
    capture outcomes or wider encounter simulation.
12. After Schema 55 is stamped, continue with one explicitly versioned mission-
    force consumer at a time. The next narrow target is guard infantry for newly
    started `assassinate_traitor` missions only. This is planned, not implemented;
    `assassinate_specops` and every other mission family remain legacy until their
    own cutover.
13. Run repeated packaged multiplayer restart/performance soak, then tune economy,
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
