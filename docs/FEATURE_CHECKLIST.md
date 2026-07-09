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
| CRI-0 Truth and baseline | Complete | Complete for this checkpoint | Repository inventory, foundation validator, Workbench script validation, and latest runtime-evidence audit complete | No | Code, configuration, documentation, and runtime evidence now agree on schema 42 and one build-identity source. Debug profiles now fail closed outside the development world and use cloned campaign state, but a fresh runtime proof of completion, cancellation, crash recovery, and restart cleanliness is still required. |
| CRI-1 Authority foundation | Complete | Training vertical slice | Static contract checks and Workbench script validation pass | No | Persisted monotonic IDs, typed command receipts, bounded campaign events, and reserve/commit/cancel/refund resource transactions are integrated through paid training. The isolated authority debug case is implemented but has not yet supplied fresh runtime evidence. Support and garrison costs remain outside this exact-quote/ledger path. |
| CRI-2 Force manifests | Drafted | Not started | Not run | No | Exact priced force manifests must precede migration of support and garrison purchases into the shared ledger. |

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
| Domain services | Economy, missions, persistence, HQ, arsenal, loot, support, enemies, civilians, markers, garrisons, and physical war have broad service owners. | Implemented Foundation | Ownership is not yet uniformly enforced through typed request/result contracts; continue extracting coordinator-heavy mutations behind explicit service boundaries. | High |
| Server-authoritative actions | Clients request actions; the server resolves identity, permissions, phase, costs, targets, and mutation. | Broad Alpha | Visible training now carries a request ID through the owner bridge and records a durable receipt. Extend idempotent typed-command handling to other mutating actions after their exact input and pricing contracts exist. | Highest |
| Stable authority IDs and receipts | Server mutations use persisted monotonic IDs, bounded receipts, and replay-safe results. | Implemented Foundation / Needs Runtime Proof | Schema 42 persists the allocator, command receipts, resource transactions, campaign events, and stable operation links. Fresh isolated runtime and restart evidence is still required. | Highest |
| Runtime diagnostics | Every major system has report/debug visibility. | Implemented Foundation | Enemy target scoring now has scored-candidate reporting plus relation-order branch proof; support placement now reports player/active-AI clearance; active-group reconciliation and UI layout diagnostics are throttled/off by default so server logs stay usable; add deeper decision reports for town influence and long-run support pressure. | High |

### Persistence And Restart Safety

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Versioned save state | Durable campaign facts survive restarts and schema migration. | Implemented Foundation | Campaign schema 42 persists the authority allocator, command receipts, resource transactions, campaign events, stable operation links, and prior roadblock metadata. Workbench validation passes, but schema-42 process-restart proof is still open. | Highest |
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
| Generated routes | Convoys, QRFs, patrols, roadblocks, and mission movement use route-aware paths. | Broad Alpha | Support/QRF deployments now use generated route chains and assign routed infantry move/sweep waypoint chains after delayed group population is resolved; extend this into richer garrison behavior and roadblock/patrol-specific waypoint types. | Highest |
| Physical activation bubble | Near-player zones physicalize and off-screen forces stay abstract unless an active objective requires runtime entities. | Broad Alpha | Support/search/QRF/roadblock requests now create durable active-group state off-screen, advance simulated route position without runtime entities, and defer runtime spawning until the simulated group enters the player event bubble; active non-convoy mission target zones still force physical activation outside the player bubble. Tune activation/deactivation by performance, player count, and objective type. | High |

### Factions And Relations

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Resistance faction | Players, HQ, arsenal, recruits, and support use the resistance faction consistently. | Implemented Foundation | Continue replacing hardcoded faction checks with preset-driven helpers. | Keep |
| Occupier and invader factions | Enemy factions have separate pools, zones, support, and target behavior. | Partial | Relation helpers now drive enemy targeting/order decisions; continue migrating remaining broad faction checks across civilians, support, and missions. | Highest |
| Relation matrix | Code asks whether two factions are hostile/neutral/allied instead of assuming non-resistance means enemy. | Broad Alpha | Keep relation checks centralized and expand runtime proof wherever a system distinguishes resistance enemies, rival enemies, and neutral actors. | Highest |
| Faction templates | Group pools, vehicles, support capabilities, equipment tiering, and fallback rules are data-driven. | Broad Alpha | Move more template data into configs and validate resources on startup. | High |

### Economy, War Level, And Strategic Score

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| HR and faction money | Recruitment, support, training, garrisons, HQ movement, and logistics spend durable resources through exact, auditable transactions. | Broad Alpha | Paid training is the first production consumer of the schema-42 resource ledger. Support pricing still depends on planned rather than committed force counts, and garrison recruitment still needs exact per-unit quoting instead of its current flat HR behavior; migrate both only after force manifests exist. | Highest |
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
| Physical response | Near-player enemy responses spawn, move, fight, and fold back. | Broad Alpha / Needs Soak | QRF/support infantry now receives generated-route move/sweep AI waypoints, reapplies native RUN movement and tight formation on every routed response tick, reconciles spawned/live member counts from actual runtime agents, tracked member handles, nearby matching AI, and editable group size, and materializes linked unclaimed response vehicles only when the simulated support group is inside the player event bubble; off-bubble support/search groups remain active campaign state and move abstractly instead of spawning or resolving. Terminal group cleanup removes stale group roots/icons while preserving dead bodies for loot; player support call-ins stage offset from the selected destination with player/active-AI clearance before routing toward the target; roadblock support establishes a vehicle-safe road checkpoint from a consumed HQ garage vehicle; recalled player support routes toward an exit and refunds surviving FIA HR; Petros attackers stage in the closer HQ attack band; continue soaking response movement, late physicalization, and count visibility in real play. | Highest |

### Civilians, Town Influence, And Population

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Town support ledger | Events change support, reputation, heat, security, and population with explainable history. | Broad Alpha | Town influence now records durable compact strategic-event rows; radio towers apply bounded `radio_broadcast` support drift and town security applies `security_pressure` police/roadblock drift on income/resource ticks; add expiry/reversal and actor/target faction effects. | Highest |
| Political town flips | Town ownership changes by support majority with hysteresis, not only direct combat. | Broad Alpha | Tune majority thresholds and prevent flicker. | High |
| Population state | Population remaining/killed affects support, income, victory, and loss. | Broad Alpha / Needs Soak | Town income and town HR now scale/gate from remaining versus killed population, and income reports expose the population multiplier; next tune support-population math, expand event coverage, and soak save/restart terminal states. | Highest |
| Civilian runtime population | Town civilians spawn from the randomized CIV character prefab, receive CIV AI wander helpers and forced CIV faction/group affiliation through a dedicated CIV empty group root, civilian vehicles resolve from the CIV entity catalog, and configured civilian traffic spawns with drivers/routes that despawn outside the render bubble. | Broad Alpha | Soak pedestrian/traffic behavior with active combat, cleanup, persistence, actual faction-component proof, and multiclient render-bubble movement. | High |
| Police and roadblocks | Security systems create scan pressure and town-state consequences. | Broad Alpha | Police and roadblock density now drifts on income/resource ticks from town owner relation, wanted heat, occupier support margin, and war level; enemy-owned town garrisons select dedicated size-2/3/4/5 HST town-police group prefabs and live town-police groups receive cyclic patrol waypoints; enemy towns with civilian `policePresence` now also project a runtime-only town-police patrol even without abstract garrison manpower. Established roadblock support from any faction is visible on the map. Next add checkpoint-specific police content and deeper roadblock/police scan behavior. | High |

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
| Garrison state | Captured zones store abstract defender manpower and vehicles. | Implemented Foundation | Garrison add/remove actions use normal-map target selection, confirmation, server revalidation, and capacity checks. Exact force manifests and per-unit transactional pricing are still required before this is an authority-complete purchase path. | Highest |
| Recruitment and HR costs | Commander spends an exact, quoted amount of HR/money to recruit a committed force manifest. | Broad Alpha | Recruitment selects destination and FIA count and revalidates the zone server-side, but the current garrison HR charge is not yet an exact per-unit ledger transaction. Add manifest quoting, arsenal/equipment requirements, and replay-safe receipts. | Highest |
| Training | Training improves resistance AI quality and caps by war level. | Broad Alpha / Needs Runtime Proof | Training now uses the shared request ID, durable command receipt, and reserve/commit resource transaction path. Existing quality/capture behavior remains implemented, but the schema-42 authority path has only static and Workbench validation until an isolated runtime case runs. | Highest |
| Static defenses | Players can assign static weapons or emplacements to garrisons. | Missing | Add durable state, placement, capture, and spawn/fold behavior. | Medium |
| Garrison physicalization | Active zones spawn defenders from garrison and composition services. | Broad Alpha | Add route/position variety, vehicle plans, and survivor fold-back proof. | High |

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

### UI, Markers, Reports, And Debug Suite

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Command menu | One in-game menu exposes setup, missions, forces, map/war, arsenal, garage, members, and admin controls. | Broad Alpha | Paid rows show money/HR costs; support recall and commander-transfer use the shared scrollable choice modal; main tab rows use player-facing labels instead of prefab, raw position, request-id, or group-id noise, while explicit reports keep diagnostics; support, supply, recruit, and garrison-removal actions open the normal map for target selection, require a map gadget, confirm before dispatch, keep the passive selected-target cursor below confirmation dialogs, consume ESC to cancel/close the targeting map, and return to the Forces menu; opening the command menu is blocked while the native in-game map is already open and now has an owner-client runtime proof; garrison recruitment prompts for FIA count before final target confirmation and location selection is single-click guarded. | High |
| Map markers | HQ, zones, missions, support, QRFs, orders, and active deliveries publish linked markers with cleanup proof. | Broad Alpha / Runtime Failure Open | State-backed publication is broad, but the latest client evidence contains 6,806 static campaign-marker update exceptions caused by a null widget root. A published marker object is not proof that its rendered widget is ready; fix delayed/root readiness and add owner-client inspection before claiming marker certification. | Highest |
| Full Campaign Debug | One button runs a true runtime certification suite and writes structured artifacts without changing the campaign under test. | Broad Alpha / Needs Runtime Proof | The latest full run produced 663 cases: 367 PASS, 61 WARN, 218 FAIL, and 17 BLOCKED; it predated isolation and contaminated its save. The runner now rejects every profile outside `HST_Dev`, blocks external/soak profiles, saves the live baseline, runs against a deep clone, diverts checkpoints, and restores the untouched state on complete or cancel. Static and Workbench validation pass; runtime proof and a mandatory session restart remain open because world/player/service state is not rolled back. | Highest |
| Scoped debug profiles | Smaller profiles isolate feature families for fast iteration. | Implemented Foundation | Keep profiles explicit and never treat external/restart/soak gaps as PASS. | Keep |
| Build provenance | Runtime logs and artifacts identify the exact code build from one authoritative source. | Implemented Foundation / Needs Packaged Proof | Runtime, menu, admin, and debug artifact summaries now consume `HST_BuildInfo`; prove the stamped identity in a packaged dedicated-server/client run. | High |

### Campaign End And Long-Run Soak

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Victory | Default victory depends on population support and decisive airfield control. | Broad Alpha / Needs Soak | Soak population/airfield outcomes across save/load and tune support thresholds. | Highest |
| Loss | Default loss depends on civilian catastrophe, with optional collapse settings. | Broad Alpha / Needs Soak | Soak killed-population outcomes across save/load and mission/civilian event paths. | Highest |
| Multiplayer soak | Campaign survives co-op, reconnect, restart, active missions, active support, and terminal saves. | Needs Soak | Build repeatable 2/4/8+ player test profiles. | Highest |
| Performance soak | Physical/abstract transitions do not leave stuck groups, duplicate vehicles, or missing markers. | Needs Soak | Add long-window background profile and cleanup certification. | Highest |

## Highest-Impact Next Tasks

1. Prove the new campaign-debug isolation boundary in `HST_Dev`: reject a
   non-development-world request, exercise completion and cancellation, verify
   the live campaign reference and both persistence channels remain unchanged,
   simulate interrupted-run recovery, then restart the development session.
2. Fix static campaign-marker widget readiness and add a delayed owner-client
   rendered-root assertion; publication/model counts alone cannot certify the UI.
3. Repair the known harness false negatives: boolean `1` versus `true`, marker
   teardown timing, pre-tick versus post-tick economy comparison, the intentional
   12-site registry count, WARN masking later FAIL, and support sampling after
   group fold.
4. Reproduce and resolve the genuine convoy movement/seating, support route, and
   physical response failures with scoped disposable debug profiles before
   interpreting the remaining cascade.
5. Run the isolated schema-42 authority case and a real save/restart replay test
   proving one request ID produces one training mutation and one committed debit.
6. Implement exact force manifests and quotes, then migrate support and garrison
   purchases into the shared command/transaction ledger without partial spend or
   count drift.
7. Extend stable operation IDs and typed command results through missions,
   support, garrisons, enemy orders, and physical projections.
8. Resume mission, civilian, undercover, and town-influence depth only after the
   authority and certification foundations can produce trustworthy evidence.
9. Run repeated packaged multiplayer reconnect/restart soak across active
   missions, support, orders, garage, undercover, and terminal campaign states.
10. Tune economy, war level, aggression, support pressure, and mission pacing
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
