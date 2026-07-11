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

| Gate | Designed | Implemented | Verified | Certified | Current evidence / blocker |
| --- | --- | --- | --- | --- | --- |
| CRI-0 Truth and baseline | Complete | Complete for this checkpoint | Repository inventory/foundation validator, Workbench source validation, and packaged schema-49 evidence audit complete | No | Current code and documentation agree on campaign schema 53. The packaged schema-49 run proved restored stock HUD/Game Master and exposed marker/dialog/radio/civilian defects repaired later in source. The final stamped schema-52 tree remains identified by implementation `fa5e7e45dbd8741269e614e60c51d4edee6bf223`. The current unstamped schema-53 development tree has provisional clean headless Game-module compile/create evidence at 5,757 files/11,550 classes with CRC `acae965f`. That is source/compile evidence only; there is no packaged schema-53 proof. The latest Full Campaign Debug artifact predates schemas 43-53 and is not current certification evidence. |
| CRI-1 Authority foundation | Complete | Training, garrison-purchase, exact player-QRF, typed support-recall, exact player/enemy-QRF operations, exact mission-convoy, and exact enemy-patrol vertical slices | Schema-53 source validation passed; packaged runtime proof unavailable | No | Persisted IDs, receipts/events, ledgers, and versioned operation authority cover the prior narrow consumers plus newly queued patrol admission/route/projection/proactive settlement. Historical patrols and restored historical convoys remain contract version `0`. Other visible commands, paid support types, order families, and non-convoy missions still use prior contracts; packaged save/restart evidence is absent. |
| CRI-2 Force manifests | Complete for foundation | Garrison quote/confirm, durable SpawnQueue, exact infantry adapter, exact player/enemy QRFs, exact enemy patrol, and the exact three-element mission convoy | Source-validation checkpoints passed; physical and restart proof pending | No | Both exact infantry QRFs and each newly queued exact enemy patrol keep one frozen group/member roster. A newly started exact convoy freezes exactly three vehicle slots, three linked crew groups and ordered members, and optional cargo/captive assigned to vehicle zero. The generic SpawnQueue adapter still supports only one infantry root plus members; the convoy uses a separate narrow PhysicalWar projection path. Generic vehicle/asset/multi-root execution, executable garrison manifests, and other consumers remain open. |
| CRI-3 Force runtime | Complete in source for three exact infantry operations, one exact mission convoy, and the crewless mixed-group slice | Exact casualty/reprojection, virtual/physical survivor transfer, patrol contact hold/fold, convoy-element fold/rematerialization, and personnel-authoritative mixed-group cleanup | Source-validation checkpoints passed; packaged runtime proof unavailable | No | Exact QRF, exact patrol, and exact convoy crew slots preserve living/casualty authority across virtual, materializing, physical, folded, restored, and settled states. Patrol fold preserves the generated-route cursor and mapped casualties; contact holds route progress until clear. Convoy fold remains atomic across separated roots. Real patrol movement/contact/fold, three-vehicle seating/interception/fold, replication, cleanup, and process restart remain unproven in a package. |
| CRI-4 Operation records | Complete for exact player/enemy infantry-QRF, enemy-patrol, and narrow mission-convoy kernels | Four explicit version-1 operation types with immutable assignment, typed materialization/route/position/settlement state, durable roster linkage, restore normalization, and fail-closed legacy isolation | Schema-53 source validation passed; focused behavior has not run in a packaged runtime | No | Schema 50 covers player QRF projection/combat; schema 51 covers enemy defensive-QRF outbound/return/settlement; schema 52 covers exact convoy travel/intercept/outcome; schema 53 covers newly queued patrols with one frozen infantry root, proactive debit, persisted generated-route outbound travel, one closed lap, contact hold, fold/reprojection, return, survivor settlement, marker, and `-53` quarantine. Ten `enemy_patrol.*` assertions cover admission, replay/refund, route loop, projection/roster, contact hold, settlement, restore, corruption, dispatch isolation, and marker lifecycle. They remain source fixtures only. Historical patrol/convoy rows, legacy QRFs, and all other operation families remain outside the cutover. |

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
| Stable authority IDs and receipts | Server mutations use persisted monotonic IDs, bounded receipts, and replay-safe results. | Implemented Foundation / Needs Runtime Proof | Schema 53 retains prior allocator/receipt/force/queue/archive authority and adds reciprocal enemy patrol order/route/operation/manifest/batch/group identity plus deterministic proactive resource settlement. Other legacy transactions and operation families still need equivalent boundaries, and fresh isolated runtime/restart evidence is required. | Highest |
| Durable operation state | Every force/support/order has an immutable assignment, duty state, engagement mode, materialization state, route progress, recall/return policy, and terminal result. | Implemented Foundation for exact player/enemy QRFs, enemy patrols, and mission convoys / Needs Runtime Proof | The two infantry-QRF and exact-convoy policies retain their schema-50/51/52 behavior. Schema 53 adds exact patrol generated-route outbound travel, one closed lap, contact-held physical projection, fold/reprojection with mapped casualties, return, and proactive survivor settlement. Off-screen encounter simulation, generalized physical-contact events, terrain/threat/ammunition simulation, generic vehicle/multi-root realization, JIP projection, and every other operation family remain open. | Highest |
| Runtime diagnostics | Every major system has report/debug visibility. | Implemented Foundation | Enemy target scoring now has scored-candidate reporting plus relation-order branch proof; support placement now reports player/active-AI clearance; active-group reconciliation and UI layout diagnostics are throttled/off by default so server logs stay usable; add deeper decision reports for town influence and long-run support pressure. | High |

### Persistence And Restart Safety

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Versioned save state | Durable campaign facts survive restarts and schema migration. | Implemented Foundation | Campaign schema 53 deep-copies the prior exact consumers plus patrol route waypoint/lap/leg clocks and exact order/resource authority. Before every real capture, persistence synchronously reconciles mapped physical exact-convoy and physical/dematerializing patrol members, proves a unique root/member adapter-to-PhysicalWar graph, and validates a live patrol position; ambiguous, aliased, missing, or unexplained deleted bindings defer capture without flushing stale state or requesting a savepoint, retain intent, and retry on the bounded debounce. Restore clears process-local patrol handles, retains cursor/casualties, and normalizes a coherent open patrol to strategic hold. Pre-schema-53 patrols remain contract version `0`; malformed current rows use `-53` quarantine, cannot save until both runtime registries are empty, and migration invents no exact authority or refund. Actual process restart, rematerialization, and settlement replay still need packaged proof. | Highest |
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
| Generated routes | Convoys, QRFs, patrols, roadblocks, and mission movement use route-aware paths. | Broad Alpha / Needs Runtime Proof | The two exact infantry-QRFs retain their direct 2.5 m/s strategic cursors. Schema 52 keeps the exact convoy generated-road polyline. Schema 53 adds a generic generated-route cursor for newly queued enemy patrols: outbound to the route start, exactly one closed lap through ordered waypoints, then a separate return-to-origin leg, with fold/restore preserving waypoint/lap/leg state. Historical patrols/convoys, legacy QRFs, counterattacks, roadblocks, garrisons, and non-convoy missions remain on prior paths. | Highest |
| Physical activation bubble | Near-player zones physicalize and off-screen forces stay abstract unless an active objective requires runtime entities. | Broad Alpha | Exact player/enemy QRFs, newly started exact mission convoys, and newly queued exact patrols are the four canonical operation consumers. The patrol uses the shared one-root infantry materialization/fold boundary, persists exact casualties and cursor state, and holds fold/route advancement while physical contact is present. The convoy retains its separated-root 1,800m/2,200m policy. Other support/order, garrison, historical patrol/convoy, and non-convoy mission families retain older paths. | Highest |
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
| Garrison state | Captured zones store abstract defender manpower and vehicles. | Implemented Foundation / Needs Runtime Proof | New visible recruitment links each accepted immutable manifest to a stable garrison ID and verifies the exact purchase-time aggregate delta before resource commit. The link records acceptance provenance while that garrison record exists; it is not a living-slot roster. Legacy aggregate helpers remain at internal/debug call sites, but neither visible-command dispatcher routes `recruit_zone` to them. Physical survivor identity still needs the ForceRuntime slice. | Highest |
| Recruitment and HR costs | Commander spends an exact, quoted amount of HR/money to recruit a committed force manifest. | Implemented Foundation / Needs Runtime Proof | Selection requests an expiring all-or-nothing server quote; the Forces tab displays its exact count, $50-per-member money total, and one-HR-per-member total; confirmation sends only the quote ID. Server context/catalog revalidation, linked reservations, exact purchase-time aggregate increment/provenance verification, commit/rollback, replay handling, and persistence are implemented. Arsenal/equipment consumption and runtime physical slot registration remain open. | Highest |
| Training | Training improves resistance AI quality and caps by war level. | Broad Alpha / Needs Runtime Proof | Training now uses the shared request ID, durable command receipt, and reserve/commit resource transaction path. Existing quality/capture behavior remains implemented, but the schema-42 authority path has only static and Workbench validation until an isolated runtime case runs. | Highest |
| Static defenses | Players can assign static weapons or emplacements to garrisons. | Missing | Add durable state, placement, capture, and spawn/fold behavior. | Medium |
| Garrison physicalization | Active zones spawn defenders from garrison and composition services. | Broad Alpha | Current activation still does not consume the paid manifest and can choose a different composition. Purchase-only garrison manifests have no executable group root and remain intentionally nondeployable. Build an executable garrison manifest/ForceRuntime path before claiming exact physical rosters, then add route/position variety, vehicle plans, survivor fold-back, successful restore/reprojection, and casualty proof. | Highest |
| Exact force spawn queue | Each force projection realizes one immutable executable manifest through bounded, retryable, restore-safe work and verified registration. | Implemented Foundation / Needs Runtime Proof | Schema 47/51/53 keep the generic queue/adapter limited to one infantry root plus exact members for the two QRF and exact patrol consumers. Schema 52 records the exact convoy's three groups, three vehicles, ordered crew members, and mission-kind-compatible cargo in a held roster batch, but a convoy-specific PhysicalWar adapter owns actual projection and fold. Confirmed casualties retire durable member slots and restore releases only survivors. Generic SpawnQueue vehicle/asset/multi-root execution, other consumer migrations, and event-driven casualty subscription remain open; neither deterministic fixtures nor Workbench compilation are packaged runtime evidence. | Highest |

### Missions And Objectives

| Feature | Target behavior | Current status | Gap / next work | Priority |
| --- | --- | --- | --- | --- |
| Category mission selection | Commander chooses mission category; mission and valid target are selected server-side. | Broad Alpha | Gun Shop is now a rarer dynamic candidate; continue tuning category candidate rules and player-facing disabled reasons. | High |
| Mission runtime primitives | HVT, destroy, hold/clear, rescue, cargo, convoy, support, and gun-shop primitives have physical action paths. | Broad Alpha / Exact convoy source-complete | Schema 52 gives newly started convoy missions one exact three-vehicle/three-crew operation with mission-kind-compatible cargo/captive on vehicle zero, virtual generated-route travel, physical interception/fold, casualty persistence, route-authoritative arrival, and existing once-only convoy outcomes. Money/supplies require one cargo row, prisoners require one captive row, and ammo/armored/reinforcement convoys forbid a separate row; admission also requires the exact role/kind and a loadable mission-asset entity prefab. Captives must be boardable characters with compartment access, while ordinary payloads must be non-character entities. Terminal vehicle survivors continue as crew-only roots; an intact partially abandoned vehicle remains independently materializable/capturable, and unresolved frozen-carrier cargo becomes a ground recovery at a destroyed/captured carrier rather than silently moving slots. Crew elimination keeps every unresolved cargo/captive/vehicle recovery mission—including supplies—open and reprojectable instead of settling early. Historical convoy rows remain legacy. Packaged convoy movement/intercept/recovery/restart proof is open. | Highest |
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
| Command menu | One in-game menu exposes setup, missions, forces, map/war, arsenal, garage, members, and admin controls. | Broad Alpha / Needs Runtime Proof | Packaged schema 49 proved that stock HUD and Game Master access were restored and exercised map-target support confirmation; it does not certify later source. Current source keeps map overlays below the native pointer, and the final stamped schema-51 tree passes Workbench source validation. Republished proof is still required for HUD/Game Master preservation, both quote/confirmation flows, choose-again/ESC teardown, pointer layering, and supported resolutions. | Highest |
| Map markers | HQ, zones, missions, support, QRFs, orders, and active deliveries publish linked markers with cleanup proof. | Broad Alpha / Needs Runtime Proof | Later source repairs radio icons/labels/transmitter reuse. Schema 51 adds one exact enemy-QRF operation marker; schema 52 publishes one aggregate exact-convoy pair; schema 53 publishes one roster-authoritative exact-patrol marker at virtual/live position and removes it after terminal cleanup. Republish and prove correct icon size, labels, convoy/patrol cursor and terminal cleanup, and radio destroy/rebuild behavior. | Highest |
| Authoritative client projection and JIP | Host, clients, reconnects, and late joiners converge on the same snapshot watermark and ordered revisioned create/update/delete stream. | Missing | Current menu/marker publication is not a complete campaign projection protocol. Add DTO snapshots, global stream sequence/watermark, per-record revisions/tombstones, acknowledgements, gap detection, resync, and host/two-client/late-join equality proof. | Highest |
| Modal map targeting | Target selection owns map/input/cursor/modal state through one idempotent state machine. | Broad Alpha / Needs Runtime Proof | Normal map targeting and confirmation flows exist with ESC handling and duplicate-click guards. Prove Closed -> Selecting -> Confirming -> Submitting/Closing behavior, Choose Again re-arm after pointer release, cursor/modal layering, and atomic ESC teardown at supported resolutions. | Highest |
| Map/War information model | Players see contacted town pressure and resistance territory without redundant or misleading rows. | Partial | Replace generic first-zone pressure with contacted towns only, current town first, then ascending resistance support with stable ties/pagination. Add Resistance Territory grouped by canonical owner/type/name and remove redundant capture-status presentation. | High |
| Full Campaign Debug | One button runs a true runtime certification suite and writes structured artifacts without changing the campaign under test. | Broad Alpha / Needs Runtime Proof | The latest full run produced 663 cases: 367 PASS, 61 WARN, 218 FAIL, and 17 BLOCKED; it predates schemas 43-53 and contaminated its save. The runner isolation and later assertions remain unexecuted in a fresh package. Schema 52 adds nine `mission_convoy.*` fixtures; schema 53 adds ten `enemy_patrol.*` fixtures for admission, replay/refund, route loop, projection/roster, contact hold, settlement, restore, corruption, dispatch, and marker lifecycle. All remain source fixtures; engine movement/contact/fold and mandatory process restart still need packaged proof. | Highest |
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

1. Publish the current schema-53 build and run a focused client/server feedback pass. Confirm that
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
7. Execute the isolated campaign-debug suite in `HST_Dev`, prove completion,
   cancellation, interrupted-run recovery, and unchanged live persistence, then
   replace the historical contaminated artifact.
8. Add the authoritative snapshot/delta/JIP projection protocol and prove host,
   client, reconnect, and late-join equality with revisions and tombstones.
9. Introduce one idempotent ownership-transition service before adding virtual
   capture outcomes or wider encounter simulation.
10. Select the next source cutover from the implementation blueprint, then expand
    exact manifests and operation projection to generic vehicles/assets,
    garrisons, remaining supports/orders, and non-convoy mission forces one
    consumer at a time.
11. Run repeated packaged multiplayer restart/performance soak, then tune economy,
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
