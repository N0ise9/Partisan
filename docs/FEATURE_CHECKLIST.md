# h-istasi Feature Checklist

This document tracks the feature-complete campaign target for h-istasi. It is
implementation-focused: every row should eventually map to state fields,
service ownership, server actions, reports, persistence, and a Full Campaign
Debug proof.

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
Full Campaign Debug or scoped HST_Dev proof
```

Avoid adding durable truth to runtime entity handles. Physical entities are
projections of campaign state and must be restorable, foldable, or disposable.

## Feature Matrix

### Runtime Ownership And Service Spine

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Server-owned coordinator | One server-side component owns lifecycle and delegates domain logic. | Implemented Foundation | Keep moving feature logic from coordinator into services when it becomes reusable. | Keep |
| Domain services | Economy, missions, persistence, HQ, arsenal, loot, support, enemies, civilians, markers, garrisons, and physical war have service owners. | Implemented Foundation | Audit coordinator-heavy flows and extract service-level request/result contracts. | High |
| Server-authoritative actions | Clients request actions; server resolves identity, permissions, phase, costs, targets, and mutation. | Implemented Foundation | Expand disabled-action reasons instead of hiding actions. | High |
| Runtime diagnostics | Every major system has report/debug visibility. | Implemented Foundation | Enemy target scoring now has scored-candidate reporting plus relation-order branch proof; support placement now reports player/active-AI clearance; add deeper decision reports for town influence and long-run support pressure. | High |

### Persistence And Restart Safety

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Versioned save state | Durable campaign facts survive restarts and schema migration. | Implemented Foundation | Keep migration discipline for every durable field addition. | Keep |
| Runtime settings migration | Generated profile settings migrate forward without keeping obsolete setup knobs. | Implemented Foundation | Schema 18 rewrites generated settings with JSON-safe explanatory comment fields while preserving known gameplay values. | Keep |
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
| Physical activation bubble | Near-player zones physicalize and off-screen forces stay abstract unless an active objective requires runtime entities. | Broad Alpha | Active non-convoy mission target zones now force physical activation outside the player bubble; tune activation/deactivation by performance, player count, and objective type. | High |

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
| HR and faction money | Recruitment, support, training, garrisons, HQ movement, and logistics spend durable resources. | Implemented Foundation | Expand real spending surfaces and per-source reports. | High |
| Personal money | Player rewards and personal purchases are distinct from faction money. | Partial | Add clear transfer and reward routing. | Medium |
| Town/resource income | Income scales from town support/population and captured resources. | Broad Alpha | Make town/resource/factory/seaport contributions visible in economy reports. | High |
| War level | Strategic control and escalation drive enemy quality, support, missions, detection, and training caps. | Implemented Foundation | Tie war level consistently into AI skill/equipment, composition, garage limits, and airfield gates. | Highest |
| Strategic score tuning | Zone values and war thresholds produce a stable campaign pace. | Partial | Choose final score weights and report next meaningful captures. | High |

### Enemy Strategy And Support Pressure

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Enemy resource pools | Enemy attack and support capacity grows from map control and pressure. | Broad Alpha / Needs Soak | Tune attack/support income and costs after real background-war runs. | Highest |
| Support spend ledger | Same-zone support stacking, recent damage pressure, spend caps, cooldowns, and refunds are tracked. | Broad Alpha | Keep as the reactive defense gate and continue tuning caps/cooldowns. | High |
| Enemy commander orders | Counterattacks, rebuilds, roadblocks, support calls, and HQ pressure queue durable orders. | Broad Alpha | Target scoring now exposes eligible candidates, weighted top-band selection, bookkeeping-zone exclusion, relation-aware owner scoring, and relation-order branch proof for counterattack/QRF/rebuild/roadblock/support-call decisions; passive HQ knowledge now uses a smaller inner exposure radius and slower one-point gain while wider local activity remains threat-only; next add proactive timing and richer counterattack behavior. | Highest |
| Abstract resolution | Off-screen orders and support resolve without needing physical entities. | Broad Alpha | Add stronger survivor, vehicle, and garrison outcome math. | High |
| Physical response | Near-player enemy responses spawn, move, fight, and fold back. | Broad Alpha / Needs Soak | QRF/support infantry now receives generated-route move/sweep AI waypoints, reapplies native RUN movement and tight formation on every routed response tick, reconciles spawned/live member counts from actual runtime agents, and materializes linked unclaimed response vehicles; player support call-ins stage offset from the selected destination with player/active-AI clearance before routing toward the target; Petros attackers stage in the closer HQ attack band; continue soaking response movement and count visibility in real play. | Highest |

### Civilians, Town Influence, And Population

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Town support ledger | Events change support, reputation, heat, security, and population with explainable history. | Broad Alpha | Add more event types, expiry/reversal, actor/target faction, and radio influence. | Highest |
| Political town flips | Town ownership changes by support majority with hysteresis, not only direct combat. | Broad Alpha | Tune majority thresholds and prevent flicker. | High |
| Population state | Population remaining/killed affects support, income, victory, and loss. | Broad Alpha / Needs Soak | Tune support-population math, expand event coverage, and soak save/restart terminal states. | Highest |
| Civilian runtime population | Town civilians spawn from the randomized CIV character prefab; civilian vehicles resolve from the CIV entity catalog; probes stay bounded. | Broad Alpha | Soak with active combat, cleanup, and persistence. | High |
| Police and roadblocks | Security systems create scan pressure and town-state consequences. | Partial | Tie police/roadblock density to support, heat, and enemy strategy. | High |

### Undercover And Wanted Enforcement

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Undercover request/apply | Players can request and hold cover only when state and appearance allow it. | Broad Alpha | Finish equipment/clothing/weapon checks from live inventory. | Highest |
| Vehicle cover and heat | Civilian vehicles provide cover until reported, heated, or compromised. | Broad Alpha | Continue proving report/clear/garage/redeploy handoff in runtime runs. | High |
| Passenger compromise | Reported vehicles and compromised passengers affect player cover. | Broad Alpha | Expand to multi-passenger and roadblock/police scans. | High |
| Off-road and security scans | Driving behavior, visible gear, police, and roadblocks can compromise cover. | Partial | Implement real scan chance by war, heat, aggression, and town security. | High |
| Undercover reports | Failure reasons are explicit and actionable. | Implemented Foundation | Keep every new detection rule explainable in one report. | Keep |

### Garrison, Recruitment, Training, And Forces

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Garrison state | Captured zones store abstract defender manpower and vehicles. | Implemented Foundation | Garrison add/remove actions now use normal-map target selection with confirmation, map-in-inventory gating, zone confirmation, count selection for FIA recruitment, and zone-slot caps for add/fold-back; add player-facing management polish and costs. | High |
| Recruitment and HR costs | Commander spends HR/money to recruit forces. | Broad Alpha | Recruitment now selects the destination and FIA count from the normal gameplay map, revalidates eligible resistance zones server-side, and returns to the Forces menu after dispatch; add arsenal/equipment requirements and clearer UI. | High |
| Training | Training improves resistance AI quality and caps by war level. | Partial | Tie to composition, physical AI skill, and abstract resolution. | High |
| Static defenses | Players can assign static weapons or emplacements to garrisons. | Missing | Add durable state, placement, capture, and spawn/fold behavior. | Medium |
| Garrison physicalization | Active zones spawn defenders from garrison and composition services. | Broad Alpha | Add route/position variety, vehicle plans, and survivor fold-back proof. | High |

### Missions And Objectives

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Category mission selection | Commander chooses mission category; mission and valid target are selected server-side. | Broad Alpha | Tune category candidate rules and player-facing disabled reasons. | High |
| Mission runtime primitives | HVT, destroy, hold/clear, rescue, cargo, convoy, and support primitives have physical action paths. | Broad Alpha | Active non-convoy primitives force their target zone, assets, and mission guard group to physicalize outside the player bubble; replace MVP primitives with mission-specific props, interactions, and consequences. | Highest |
| Mission persistence | Active missions, objectives, assets, runtime entities, and markers survive restart. | Broad Alpha / Needs Soak | Add active mission restart route to the full soak suite. | Highest |
| Strategic mission effects | Missions affect money, HR, town support, aggression, HQ knowledge, enemy pools, and unlocks. | Broad Alpha | Mission success/failure and resistance zone captures now record durable strategic-event rows with before/after deltas; next route mission expiry, convoy outcome branches, vehicle reports, support-near-HQ, and town influence through the same ledger. | High |
| Mission reports | Reports explain active objective, target, assets, runtime phase, and failure blockers. | Implemented Foundation | Expand per-family detail as missions get unique content. | Keep |

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
| Command menu | One in-game menu exposes setup, missions, forces, map/war, arsenal, garage, members, and admin controls. | Broad Alpha | Support, supply, recruit, and garrison-removal actions now open the normal map for target selection, require a map gadget, confirm before dispatch, keep a passive selected-target cursor visible above the confirmation dialog, and return to the Forces menu; garrison recruitment prompts for FIA count before final target confirmation; Petros' final context action handles Relocate/Deploy HQ follow flow. | High |
| Map markers | HQ, zones, missions, support, QRFs, and orders publish linked markers with cleanup proof. | Broad Alpha | Player-requested resistance support groups now publish live group markers while spawned and update from actual runtime entity positions; terminal support groups force marker refresh; towns, bases, radar/radio sites, and QRFs use distinct native icons while resource nodes stay unchanged; continue owner-client visual proof and marker/backing consistency checks. | High |
| Full Campaign Debug | One button runs a true runtime certification suite and writes structured artifacts. | Broad Alpha | Added live support-group marker assertions, active-group-backed attacker marker proof, curated location taxonomy preflight, marker icon deconflict proof, recursive runtime vehicle-unclaimed audits, delayed route-assignment proof, expanded native marker publication checks, cleanup-time pending-population drains, convoy seat-bind evidence, convoy AI vehicle-usage registration assertions, a threshold-length convoy movement window before contact, relation-aware enemy target-scoring proof, global-aggression no-HQ-knowledge leak proof, outer-radius threat-without-HQ-reveal proof, zero-knowledge no-Petros gate proof, relation-order decision proof, map-target command gating/cursor-layer proof, support spawn offset/player-AI clearance proof, Petros attack staging-band proof, native response-run proof, runtime member-count proof, Petros relocate-action ordering proof, QRF-no-garrison order resolution proof, capped garrison fold-back proof, mission-target forced-physicalization proof, and mission success/failure strategic-event ledger proof; keep adding ARRANGE/ACT/OBSERVE/ASSERT/CLEANUP cases for every new feature. | Highest |
| Scoped debug profiles | Smaller profiles isolate feature families for fast iteration. | Implemented Foundation | Keep profiles explicit and never treat external/restart/soak gaps as PASS. | Keep |
| Build provenance | Runtime logs and artifacts identify the exact code build. | Implemented Foundation | Bump synchronized build markers for every runtime-proof behavior change. | Keep |

### Campaign End And Long-Run Soak

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Victory | Default victory depends on population support and decisive airfield control. | Broad Alpha / Needs Soak | Soak population/airfield outcomes across save/load and tune support thresholds. | Highest |
| Loss | Default loss depends on civilian catastrophe, with optional collapse settings. | Broad Alpha / Needs Soak | Soak killed-population outcomes across save/load and mission/civilian event paths. | Highest |
| Multiplayer soak | Campaign survives co-op, reconnect, restart, active missions, active support, and terminal saves. | Needs Soak | Build repeatable 2/4/8+ player test profiles. | Highest |
| Performance soak | Physical/abstract transitions do not leave stuck groups, duplicate vehicles, or missing markers. | Needs Soak | Add long-window background profile and cleanup certification. | Highest |

## Highest-Impact Next Tasks

1. Repack and rerun the dedicated server proof on the r93 HQ knowledge/response
   counts build, then compare passive HQ knowledge gain, attacker/support
   movement speed, and visible spawned/live group counts against playtest notes.
2. Extend routed response infantry into richer counterattack/HQ-pressure
   behavior after the r93 native response-run proof confirms support/attack pacing.
3. Finish undercover enforcement from live equipment, vehicle state, off-road
   behavior, and security scans.
4. Deepen town influence events into the primary political control layer.
5. Add player-facing garrison management, training effects, static defenses,
   and arsenal-driven AI loadout improvements.
6. Route mission expiry, convoy outcome branches, vehicle reports,
   support-near-HQ, and town influence through the strategic-event ledger.
7. Replace MVP mission primitives with mission-specific physical content and
   strategic outcomes.
8. Soak attack/support spend separation and population-based victory/loss
   through real restart, background-war, campaign-end restore, and
   civilian-event mission paths.
9. Run repeated save/load and multiplayer soak across missions, support,
   orders, garage, undercover, and campaign end states.
10. Tune economy, war level, aggression, support pressure, and mission pacing
    through repeated real campaign runs.

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
