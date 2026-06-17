# h-istasi Phase Plan

This roadmap is the working implementation plan for h-istasi. It is meant to be
updated as phases complete, split, or get refined through HST_Dev smoke tests.

Status legend:

- Complete: implemented, validated statically, and smoke-tested enough to move on.
- In progress: current active slice.
- Planned: accepted direction, not implemented yet.

## Definition Of Done For Every Phase

Every phase should leave the project in a state that can be tested by a human
in HST_Dev and inspected through a clear server-side report path. Unless a phase
explicitly says otherwise, completion means:

- `tools/validate-foundation.ps1` passes or is intentionally extended and then
  passes.
- HST_Dev can launch far enough to exercise the changed system.
- The changed feature has a command-menu report, debug action, or log path that
  exposes its important runtime state.
- Campaign mutations remain server-authoritative and flow through services.
- Persistent campaign state does not store raw `IEntity` references.
- Missing Reforger APIs, invalid resources, unavailable services, or fragile
  runtime calls fail with visible reasons and preserve campaign state.
- No hidden addon dependency, non-base-game asset assumption, or save migration
  is introduced unless the phase explicitly calls for it.
- Phase status in this document is updated when the phase is completed.

## Current Baseline

h-istasi is already past the blank-project stage. The repository has a CE
3.11.1 mission-registry baseline, server-side campaign/economy/mission/
persistence/checkpoint services, player/HQ/Petros setup, custom arsenal and
loadout scaffolding, loot collection, generated Everon sites/routes, mission
objectives, physical mission primitives, support requests, enemy commander
orders, civilian/undercover state, and an Antistasi-style command menu.

The current server runtime is centered on
`HST_CampaignCoordinatorComponent`. It instantiates services, restores or
creates campaign state, tracks persistence, refreshes markers, and then ticks
mission timers, objectives, mission runtime, convoy runtime, income, enemy
resources, aggression decay, civilians, support requests, enemy orders, HQ
runtime objects, physical zone activation, zone capture, and civilian
population.

The state model already has the right Antistasi save-game spine:

- faction pools
- players
- zones
- garrisons
- active groups
- QRFs
- map markers
- arsenal items
- garage vehicles
- vehicle cargo
- runtime vehicles
- saved loadouts
- active missions
- generated sites
- generated routes
- mission objectives
- mission runtime entities
- mission assets
- support requests
- enemy orders
- civilian zone state
- undercover records
- campaign tasks

The plan should harden each existing service into a playable, testable vertical
slice. It should not rebuild the foundation.

## Game-Mode Target

The player fantasy is FIA resistance on Everon with almost nothing. The US
occupying force owns towns, outposts, resources, factories, ports, and airfields.
The USSR invading force owns or contests selected zones. Players loot, recruit,
train, complete missions, capture zones, build support, and gradually turn the
island against the occupier.

Campaign pacing target:

- Early game: scavenge weapons, use civilian cover, ambush patrols, loot bodies,
  and avoid direct fights.
- Mid game: complete missions, capture weak outposts, build garrisons, garage
  captured vehicles, and improve training.
- Late game: coordinate squads, defend against counterattacks, attack airfields
  and ports, respond to QRFs, and push toward full-map victory.

## Strategic Map Model

Zones are economic, military, mission-generation, and victory nodes. Every
strategic zone should eventually carry:

- `zoneId`
- `displayName`
- `type`
- `ownerFactionKey`
- `position`
- `captureRadius`
- `activationRadius`
- `incomeValue`
- `priority`
- `support`
- `resistanceCaptureProgress`
- `garrisonSlots`
- `compositionId`
- `spawnProfileId`
- `patrolRouteId`
- `qrfRouteId`
- `missionSiteId`
- `linkedZoneIds`

`HST_ZoneState` already contains most of this model. Future work should refine
the existing record instead of creating a parallel strategic map.

## Town Model

Towns should not flip just because soldiers die nearby. Town control should be
driven by support and civilian conditions.

Town ownership inputs:

- resistance support
- occupier support
- civilian population
- civilian casualties by faction
- wanted heat
- police presence
- roadblock presence
- undercover restrictions

Rules:

- Town flips to FIA when FIA support is greater than occupier support.
- Town flips back or becomes hostile when occupier support is greater than FIA
  support.
- Civilian deaths and failed support missions reduce FIA support or increase
  wanted heat.
- Supply/city-aid missions increase FIA support.
- Enemy supply convoys can increase occupier support if they arrive.

`HST_CivilianZoneState` already has the first usable fields: reputation, wanted
heat, civilian presence, police presence, roadblock presence, last incident, and
undercover restriction.

## Progression Model

h-istasi should use the Antistasi progression axes as campaign-facing state:

- HR: recruitment capacity.
- Faction money: commander spending, training, vehicles, and support.
- Personal money: player rewards, transfers, and personal buys.
- Training level: FIA AI quality and equipment quality.
- War level: enemy/friendly equipment tier and mission difficulty.
- Aggression: enemy response budget, QRF chance, Petros attacks, and roadblocks.
- Town support: town ownership, HR income, and mission availability.
- HQ knowledge: chance/frequency of defend-Petros attacks.
- Arsenal unlocks: resistance equipment modernization.
- Garage inventory: captured vehicle progression.

## Standing Rules

- `HST_CampaignState` is the persistent source of truth.
- Campaign mutations must be server-authoritative.
- Persistent campaign state must not store raw `IEntity` references.
- Off-screen forces stay abstract.
- Physical AI activates around players, active objectives, QRFs, convoys, and
  important mission runtime.
- Every new feature needs a debug/report path in the command menu or logs.
- Every feature must be testable in `HST_Dev` before Everon polish.
- Do not add hidden addon dependencies or non-base-game asset assumptions.
- If a Reforger API call is unavailable or fragile, fail with a clear reason
  and preserve campaign state.

## Architectural Principles

### Server-Authoritative Campaign State

All campaign changes go through server-side services. Client UI sends requests;
the server validates phase, permissions, player identity, target IDs, distances,
costs, mission state, and cooldowns.

Never let a client directly mutate:

- money
- HR
- arsenal counts
- garage records
- mission success/failure
- zone ownership
- garrisons
- support requests
- enemy orders
- undercover state

### Abstract First, Physical Second

For every system, define abstract campaign state before physical implementation.

Example:

- Abstract: garrison at an outpost has eight infantry and one vehicle.
- Physical: when players enter activation radius, spawn a group and vehicle.
- Fold-back: when players leave, count survivors and fold them back into the
  garrison.

`HST_PhysicalWarService` already follows this shape for zone activation and
survivor fold-back. New features should keep using this pattern.

### Physical Entities Are Disposable

Every spawned entity should have a state record that can survive the entity
being deleted or the server restarting:

- runtime entity ID
- mission instance ID, zone ID, or group ID
- prefab
- position
- angles
- spawned flag
- destroyed/recovered/deleted flags

If the server restarts, h-istasi should recreate, abstract, or safely discard
physical projections based on persistent state. It must not rely on raw entity
handles as truth.

### Deterministic Generation

Generated sites, routes, mission targets, convoy starts, and support spawn
points should come from stable inputs:

- campaign seed
- zone ID
- mission ID
- elapsed-second bucket
- war level
- faction key

Avoid pure random calls that cannot be reproduced or diagnosed after restart.

### Small Vertical Slices

Every implementation task should be shippable and testable in `HST_Dev` before
Everon polish. A good phase slice defines:

- goal
- files to inspect
- files likely to modify
- state fields changed
- server methods added
- UI/debug command added
- acceptance tests
- do-not-change constraints

## Service Ownership Map

### Coordinator

File: `Scripts/Game/HST/Components/HST_CampaignCoordinatorComponent.c`

Responsibilities:

- create services
- restore/create campaign state
- apply settings
- register players
- tick campaign services
- route UI/RPC commands
- broadcast mission/support/order/capture events
- refresh markers
- trigger persistence checkpoints

Rules:

- Keep coordinator thin.
- Do not put domain logic directly in coordinator.
- Add services for new domains.
- Add coordinator methods only as authenticated server entry points.
- Every coordinator command should return a human-readable result string and
  optionally emit a feed/event entry.

Baseline acceptance:

- Server starts `HST_Dev`.
- Coordinator initializes once.
- State schema version is current.
- Menu opens.
- Manual save works or reports a clear not-available reason.
- Mission start command works.
- No service throws null-state errors during a short idle smoke pass.

### Campaign State

File: `Scripts/Game/HST/State/HST_CampaignState.c`

Persistent state should remain the only durable truth. Future fields should fall
into one of three patterns.

Durable campaign facts:

- zone owner
- town support
- arsenal item count
- garage vehicle record
- player membership
- HQ position
- Petros alive/dead

Active runtime records:

- active mission
- active group
- support request
- enemy order
- convoy runtime
- runtime vehicle
- mission asset

Transient diagnostics:

- last spawn failure
- last vehicle target reason
- last UI result
- last mission runtime event

Acceptance pattern:

- Start campaign.
- Start mission.
- Spawn runtime assets.
- Save.
- Restart.
- Verify mission, assets, markers, resources, HQ, and players restore.
- Complete mission.
- Save again.
- Restart.
- Verify completed mission is not physically respawned.

## Phase Status

| Phase | Name | Status |
| --- | --- | --- |
| 0 | Stabilize project rules and validation | Complete |
| 1 | Mission runtime visibility and diagnostics | Complete |
| 2 | Convoy runtime report | Complete |
| 3 | Convoy route state | Complete |
| 4 | Convoy readiness gating | Complete |
| 5 | Convoy vehicle-control adapter | Complete |
| 6 | Real convoy crew seating | Complete |
| 7 | Convoy waypoint-chain movement | Complete |
| 8 | Convoy progress, stuck detection, and destination arrival | Complete |
| 9 | Convoy contact behavior | Complete |
| 10 | Generic convoy completion | Complete |
| 11 | Mission-specific convoy outcomes | Complete |
| 12 | Active mission persistence | Complete |
| 13 | Non-convoy mission primitive hardening | Complete |
| 14 | Arsenal, loot, and finite/infinite unlock loop | In progress - 1/2 complete checkpoint |
| 15 | Garage and vehicle persistence | Planned |
| 16 | Recruitment, training, and garrisons | Planned |
| 17 | Zone capture and ownership | Planned |
| 18 | Enemy commander physical responses | Planned |
| 19 | Support requests | Planned |
| 20 | Civilians, town support, and undercover reports | Planned |
| 21 | Undercover enforcement and police/roadblocks | Planned |
| 22 | HQ threat and Defend Petros | Planned |
| 23 | UI and map marker polish | Planned |
| 24 | Balance, campaign pacing, and victory/loss | Planned |
| 25 | Full-campaign soak testing | Planned |

## Roadmap Sequencing Rationale

The roadmap is ordered so each phase increases confidence before increasing
complexity. Phases 0-2 establish the diagnostic foundation: campaign health,
mission runtime state, and convoy internals are visible before movement,
persistence, and player-facing polish are expanded.

Phases 3-12 focus on convoy, mission, and persistence hardening because convoy
runtime crosses the highest-risk boundaries first: generated sites, routes,
spawned assets, active groups, AI behavior, mission cleanup, and save/load
restore. Getting that slice stable gives the rest of the campaign a repeatable
pattern for runtime projections of persistent state.

Phases 13-19 broaden the campaign loop into additional mission families,
commander behavior, physical war activation, zone capture, garrisons, and
support/QRF systems. Phases 20-24 then deepen the player-facing resistance
systems: civilians, undercover rules, economy, HQ logistics, UI, and balance.
Phase 25 is deliberately a hardening and soak phase rather than a feature phase;
it exists to prove the whole system can survive long server sessions and
save/load cycles.

## System Roadmap

These system notes are cross-cutting guidance for the phase list. They describe
where each major Antistasi subsystem is headed and what a future phase should
protect while implementing it.

### Setup And Campaign Phase

Purpose: start in setup, let the commander choose HQ, then move to active
campaign play.

Required behavior:

- `CAMPAIGN_SETUP`: allow admin/commander to select initial hideout; block
  missions, income, enemy commander, and capture.
- `CAMPAIGN_ACTIVE`: allow missions, income, enemy orders, support, capture,
  HQ movement, and checkpoints.
- `CAMPAIGN_ENDED`: freeze campaign mutation and show victory/loss state.

Future hardening:

- Add a central `CanMutateCampaign(actionId)` helper.
- Reject mission/economy/enemy/capture actions during setup.
- Add command-menu status explaining blocked actions.
- Add dev report for phase and HQ state.

Acceptance:

- Starting a mission before HQ selection returns a clear failure.
- Selecting HQ deploys Petros/cache/arsenal/tent.
- Moving HQ updates all HQ runtime positions.
- Save/load preserves selected HQ.

### Persistence And Schema Migration

Purpose: make the war survive restarts.

Persist:

- campaign metadata
- phase
- elapsed seconds
- resources
- players
- HQ
- zones
- garrisons
- active groups
- missions
- mission assets
- runtime vehicles
- arsenal
- garage
- support requests
- enemy orders
- civilian state
- undercover state
- generated sites/routes
- tasks

Do not persist as required truth:

- raw `IEntity` pointers
- `AIGroup` pointers
- spawned entity object handles
- temporary UI selection
- unsynchronized client-only data

Future smoke tools:

- `persistence_smoke_prepare`: creates test arsenal item, garage vehicle, active
  mission, mission asset, and active group, then forces checkpoint.
- `persistence_smoke_verify`: verifies restored records and reports missing
  categories separately.

Acceptance:

- Run prepare, save, restart, verify.
- Verification reports each category separately.

### Map Zones, Sites, And Routes

Purpose: drive income, missions, enemy orders, garrisons, QRFs, convoy routes,
town support, and victory.

Required zone types:

- town
- outpost
- resource
- factory
- radio tower
- airfield
- seaport
- bank
- police station
- roadblock
- support/stash/crash site
- HQ hideout

Required generated content:

- primary objective position
- secondary objective position
- roadblock/convoy ambush point
- support/cache point
- civilian stash
- crash/salvage point
- patrol route
- QRF route
- convoy route

Route model target:

```c
class HST_RouteWaypointState
{
	string m_sRouteId;
	int m_iIndex;
	vector m_vPosition;
	int m_iRadiusMeters;
	string m_sHint; // road, junction, bridge, destination, fallback
}

class HST_GeneratedRouteState
{
	string m_sRouteId;
	string m_sSourceZoneId;
	string m_sTargetZoneId;
	vector m_vStartPosition;
	vector m_vEndPosition;
	int m_iDistanceMeters;
	bool m_bRoadRoute;
	bool m_bValidatedForVehicles;
	ref array<ref HST_RouteWaypointState> m_aWaypoints;
}
```

Acceptance:

- Generated content report shows sites and routes.
- Every strategic zone has a primary site.
- Every outpost/resource/town has at least one mission-compatible site.
- Every convoy-capable zone has a vehicle-safe start and destination.
- Generated route positions are dry ground.
- No generated route starts inside HQ safe radius.

### Factions And Templates

Purpose: define units, vehicles, loadouts, equipment tiers, group prefabs,
patrol types, QRF types, support capabilities, and war-level scaling.

Fixed Everon preset:

- FIA: player resistance.
- US: occupying force.
- USSR: invading force.

Target faction contract:

```c
class HST_FactionTemplate
{
	string factionKey;
	string displayName;
	string sideKind; // resistance, occupier, invader
	array<string> infantryGroupPrefabs;
	array<string> patrolGroupPrefabs;
	array<string> qrfGroupPrefabs;
	array<string> vehiclePrefabsByTier;
	array<string> convoyVehiclePrefabsByTier;
	array<string> staticWeaponPrefabs;
	array<string> civilianVehiclePrefabs;
	array<string> startingRebelItemPrefabs;
	array<string> lootBlacklistPrefabs;
	array<string> unlockBlockedPrefabs;
	bool supportsHelicopter;
	bool supportsArmor;
	bool supportsArtillery;
	bool supportsAirstrike;
}
```

Slices:

- Keep base US/USSR/FIA templates and validate every prefab on server start.
- Add war-level equipment tier selection.
- Have mission runtime and physical war ask faction templates for composition.
- Add faction capability flags.
- Disable unsupported support calls and mission families honestly.
- Add future mod preset extension points without hidden dependencies.

Acceptance:

- Faction report lists valid group prefabs and vehicle prefabs.
- Invalid prefab is logged once and skipped.
- War level changes selected vehicle tier.
- Convoy vehicle selection never returns empty when convoy mission is enabled.

### Economy, HR, War Level, And Aggression

Purpose: pace the campaign.

Income tick:

- FIA towns generate HR and money.
- FIA resources generate money.
- FIA factories modify resource income.
- FIA seaports modify HR/vehicle availability.
- Enemy zones generate enemy attack/support resources.

War level increases from:

- captured territory
- captured high-value zones
- optional elapsed time
- optional successful missions

War level affects:

- enemy equipment tier
- convoy composition
- QRF strength
- mission difficulty
- support response type

Aggression increases from:

- killing enemies
- completing hostile missions
- taking flags
- destroying assets
- shooting civilians or prisoners
- capturing zones

Aggression decreases from:

- time decay
- failed rebel actions
- possibly aid/support actions

Aggression affects:

- QRF chance
- roadblocks
- patrol density
- Petros attack chance
- support calls
- enemy resource spending priorities

Future report:

- `BuildEconomyBreakdown(state)` showing income sources by zone type, enemy
  resource income by faction, and aggression decay timer.

### HQ, Petros, Members, Guests, And Commander

Purpose: define the resistance hub and permissions.

Required HQ objects:

- Petros
- arsenal/cache
- garage/vehicle box
- tent/rest point
- map/whiteboard
- flag/recruitment point
- optional build box

Petros behavior:

- Petros alive: missions and HQ actions are available.
- Petros killed: apply HR/money penalty, fail defend-Petros mission, increase
  HQ pressure, and enter recovery/campaign-loss flow depending final design.

Authorization shape:

- Admin: reset, force save, debug, membership management.
- Commander: spend faction money, move HQ, train troops, recruit squads, manage
  garrisons, request support, start missions, manage roadblocks/watchposts.
- Member: request missions, recruit personal squad, use arsenal within limits,
  garage vehicles if allowed, loot/deposit.
- Guest: limited access and no limited arsenal items unless policy allows.

Future hardening:

- `CanUseAction(playerId, actionId)`.
- Action category: admin, commander, member, guest.
- UI disabled reason instead of hiding all blocked actions.
- Membership report.

### Arsenal, Looting, Loadouts, And Unlocks

Purpose: complete the loot-to-unlock resistance progression loop.

Current direction:

- `HST_ArsenalService` owns deposits, counts, unlock thresholds, finite items,
  infinite unlocked items, and issued-item accounting.
- `HST_LootService` scans nearby entities, collects eligible inventory/loose
  items, skips blocked/friendly/player-owned items, and captures vehicles into
  garage records.

Target behavior:

- Area loot deposits eligible items into campaign arsenal.
- Vehicle loot transfers eligible cargo into nearby vehicle cargo.
- Vehicle cargo can transfer to arsenal.
- Arsenal counts finite items.
- Items unlock when threshold is reached.
- Unlocked items become infinite.
- Blocked items never unlock or never deposit depending config.
- Loadout editor validates costs before applying.
- Failed loadout application is atomic and does not lose items.
- AI recruit loadouts can eventually draw from unlocked arsenal tiers.

Acceptance:

- Depositing an item increases arsenal count.
- Item unlocks at configured threshold.
- Withdrawing finite item decrements count.
- Withdrawing unlocked item does not decrement count.
- Blocked item does not unlock.
- Loadout apply is atomic.
- Save/load preserves arsenal counts and unlock state.

### Garage And Vehicle Persistence

Purpose: make captured vehicles part of campaign progression.

Target behavior:

- Capture nearby vehicle into garage.
- Validate vehicle root and reject parts/proxies/scenery.
- Validate vehicle is not occupied.
- Capture physical and virtual cargo.
- Delete physical vehicle only after garage record is prepared.
- Redeploy vehicle near HQ at a safe slot.
- Restore cargo on redeploy.
- Track ammo/repair/fuel source vehicles.

Acceptance:

- Captured vehicle appears in garage report.
- Captured vehicle despawns from world.
- Vehicle cargo is preserved.
- Redeployed vehicle spawns at safe HQ position.
- Garage record is removed or updated according to redeploy policy.
- Ammo truck is recognized as ammo source.
- Save/load preserves garage vehicles and cargo.

### Mission Runtime

Purpose: make mission starts, objectives, runtime entities, cleanup, and
save/load deterministic.

Mission lifecycle:

- select eligible mission
- select target zone/site
- create active mission record
- create objective records
- initialize runtime primitive
- create mission assets/runtime entity records
- activate physical projection only when appropriate
- update objective progress
- complete/fail/expire with clear reason
- clean up disposable physical entities
- preserve durable campaign effects

Mission primitive targets:

- `kill_hvt`
- `hold_area`
- `clear_area`
- `destroy_target`
- `recover_cargo`
- `rescue_extract`
- `deliver_supplies`
- `convoy_intercept`
- `defend_petros`

Acceptance:

- Every active mission has an inspectable runtime report.
- Objective links are stable and visible.
- Missing target/site/entity data reports `none` or `missing:<id>`.
- Runtime cleanup does not erase durable rewards/penalties.
- Active missions survive save/load.

### Convoys

Purpose: make convoy missions a long-lived physical/abstract hybrid that can be
ambushed, tracked, saved, and resolved.

Generic convoy model:

- active mission owns convoy objective and mission assets
- each vehicle asset has source/current/target positions
- each active convoy group has faction, group prefab, crew count, vehicle state,
  fallback mode, spawn failure reason, and runtime status
- route state provides ordered waypoints
- readiness gate decides whether movement can begin
- vehicle-control adapter owns seating, driver, gunner, mobility, and route
  assignment checks
- progress tracker owns stuck detection and destination arrival
- contact detector owns ambush/contact phase transitions
- mission-specific hooks apply unique rewards/penalties

Mission-specific convoy targets:

- `convoy_ammo`: captured ammo vehicle creates ammo-source effect or ammo reward.
- `convoy_armored`: captured armor can be garaged; composition scales with war
  level.
- `convoy_money`: full payout requires money vehicle/cargo delivered to HQ.
- `convoy_prisoners`: prisoners must be freed/extracted for full reward.
- `convoy_reinforcements`: arrival strengthens target garrison.
- `convoy_supplies`: arrival increases occupier support; captured/delivered
  supplies increase FIA support.

Convoy persistence:

- Save mission state, phase, route ID, route waypoints, vehicle assets, current
  positions, crew group states, runtime vehicle states, ETA, objective progress,
  and destroyed/captured/delivered flags.
- On load, do not assume physical entities still exist.
- If players are near, respawn physical vehicles/crew at saved positions and
  reassign route from nearest waypoint.
- If players are far, keep convoy abstract and advance/resolve by state.

### Support Requests

Purpose: represent FIA and enemy support as stateful, testable requests.

FIA support targets:

- supply support
- ground reinforcement
- mortar support only if an honest asset path exists
- vehicle delivery
- optional extraction later

Enemy support targets:

- QRF
- patrol/search
- ground reinforcement
- abstract helicopter-style support when no safe asset path exists
- artillery only if honest asset path exists

Request contract:

- validates cost
- validates cooldown
- records source and target
- has ETA and status
- physicalizes near players
- resolves abstractly when off-screen

Acceptance:

- FIA support request appears in support report.
- Enemy support request spends enemy resources.
- Request has ETA and status.
- Request resolves physically when players are nearby.
- Request resolves abstractly when players are far.
- Cooldown prevents spam.
- Save/load preserves active support requests.

### UI, Map Markers, And Player Feedback

Purpose: make the campaign understandable without reading logs.

Current command menu tabs:

- Setup
- Overview
- HQ/Petros
- Missions
- Map/War
- Forces
- Arsenal/Loot
- Members
- Admin

Target UI panels:

- Overview: resources, HR, war level, aggression, support, active missions.
- HQ/Petros: HQ status, Petros health, move HQ, train troops, save.
- Missions: available missions, active missions, objective progress, runtime
  report, admin abandon/fail debug.
- Map/War: zones by owner, garrison strength, town support, enemy orders,
  QRF/support status.
- Forces: FIA garrisons, active groups, recruitment, training.
- Arsenal/Loot: loot nearby, loot to vehicle, arsenal report, loadout editor,
  garage report, capture/redeploy vehicle.
- Members: commander, members, guests, permissions.
- Admin: reset, save, debug resources, force mission, spawn test convoy,
  persistence smoke tests.

Marker targets:

- active mission marker
- objective marker
- convoy start marker, if useful and not too revealing
- convoy last-known marker
- convoy destination marker, possibly approximate
- QRF/support inbound marker
- capture progress marker
- zone ownership marker
- HQ visibility rules

## Testing And Validation Strategy

### Static And Compile Gates

- PowerShell foundation validation script.
- Resource path validation.
- Missing prefab report.
- Enum/string constant validation.
- State schema migration validation.
- No hidden dependency checks.

### Service Smoke Tests

In Reforger scripting these are usually dev commands or smoke services:

- economy tick test
- arsenal deposit/unlock/withdraw test
- garage capture/redeploy test
- mission start/complete/fail test
- route generation test
- convoy spawn test
- persistence prepare/verify test

### HST_Dev Scenario Tests

- Start campaign.
- Select HQ.
- Start each mission family.
- Spawn each runtime primitive.
- Complete/fail by interaction.
- Save/load.
- Repeat as local host.

### Full Everon Soak

- Two-hour idle with no players moving.
- Two-hour mission loop.
- Multi-client join/leave test.
- Save/restart under active mission.
- Active convoy save/restart.
- Zone capture/counterattack loop.

### Convoy-Specific Test Matrix

Route generation:

- valid route
- no route
- route too short
- route near HQ
- wet/water route

Spawn:

- three or more vehicles spawn
- one vehicle fails
- crew group fails
- no driver assigned
- invalid vehicle prefab

Movement:

- staging to moving
- waypoint assignment success
- waypoint assignment failure to static ambush/contact
- moving marker update
- stuck detection
- destination arrival

Combat:

- player approaches and contact starts
- kill one crew group and progress becomes 1/N
- kill all crews and objective succeeds
- destroy cargo vehicle and branch mission-specific result
- capture vehicle with living crew is blocked
- capture after crew eliminated is allowed

Persistence:

- save in staging
- save while moving
- save after contact
- save after one vehicle destroyed
- save after success before cleanup

## Task Template

Use this template for future implementation requests.

```text
You are working in N0ise9/h-istasi.

Goal:
  [one specific behavior]

Context:
  h-istasi is server-authoritative. Campaign state is the source of truth.
  Physical entities are runtime projections and must be restorable or safely
  abstracted. Do not add hidden dependencies or non-base-game assets.

Files to inspect first:
  [file list]

Files likely to modify:
  [file list]

Implement:
  1. [specific code change]
  2. [specific code change]
  3. [specific code change]

State changes:
  [new fields/classes, migration considerations]

UI/debug:
  [report/action/command to add]

Acceptance:
  1. [testable result]
  2. [testable result]
  3. [testable result]

Constraints:
  - Keep server-authoritative.
  - Do not trust client-supplied player IDs.
  - Do not store raw IEntity references in persistent state.
  - Do not break existing HST_Dev flow.
  - If native API is unavailable, fail with a clear reason and preserve state.
```

## Phase 0 - Stabilize Project Rules And Validation

Status: Complete

Goal: create a safe baseline so every later phase can be tested without
breaking the foundation.

Implementation:

- Run or extend existing validation tooling.
- Confirm all `HST_` prefixed scripts compile.
- Confirm `HST_Dev` starts.
- Confirm the campaign coordinator initializes once.
- Confirm the command menu opens.
- Confirm the campaign state schema version is current.
- Confirm manual save/checkpoint path does not error.
- Add a foundation status report if one is not already present.

Acceptance criteria:

- `HST_Dev` launches.
- Server coordinator initializes.
- Alpha command menu opens.
- Foundation status/report command returns campaign phase, schema version, HQ
  state, active mission count, and active group count.
- Manual checkpoint command returns a clear success or clear not-available
  reason.
- No new dependencies are introduced.

## Phase 1 - Mission Runtime Visibility And Diagnostics

Status: Complete

Goal: before improving missions, make active mission state visible and
debuggable.

Implementation:

- Add or improve mission runtime inspection.
- Report instance ID, mission ID, display name, target zone, site ID, runtime
  primitive, runtime phase, remaining seconds, objective totals, mission asset
  count, runtime entity count, and failure reason for each active mission.
- Add command-menu action `Inspect Active Missions`.
- Add per-active-mission inspection actions where an active mission exists.

Acceptance criteria:

- Starting any mission creates a visible active mission report.
- Report includes runtime primitive and runtime phase.
- Report includes objective progress.
- Report includes mission assets and runtime entities.
- Expired, failed, and completed missions show clear final status.
- Mission reports do not crash with missing target/site/entity data.

## Phase 2 - Convoy Runtime Report

Status: Complete

Goal: make convoy internals visible before changing AI movement.

Implementation:

- Add `BuildConvoyRuntimeReport(state, mission)`.
- Report mission instance ID, mission ID, phase, ETA, source position, target
  position, route/site ID, vehicle asset count, and mission failure reason.
- Report each convoy vehicle asset: asset ID, prefab, source position, current
  position, target position, spawned, destroyed, delivered/captured, and last
  interaction.
- Report each active convoy group: group ID, faction, prefab, spawned entity,
  runtime status, crew count, alive crew count, source position, target
  position, fallback mode, and spawn failure reason.
- Add Missions tab action `Convoy Runtime Report`.
- Add route/travel distance visibility and explicit static fallback when
  vehicle control is unavailable.

Acceptance criteria:

- Starting `convoy_ammo` in `HST_Dev` creates a convoy report.
- Convoy report lists all convoy vehicle assets.
- Convoy report lists all active convoy groups.
- Report shows whether each vehicle/group spawned.
- Report shows alive crew count.
- Report shows why movement or spawn failed.
- Report remains valid after convoy fails or completes.

Notes carried into Phase 3:

- Verified ground vehicle variety now uses live faction campaign entity
  catalogs first, then GUID-qualified base-game fallback resources.
- Real AI vehicle embark/movement is not implemented yet; convoy reports now
  expose this as a clear static-ambush fallback reason.

## Phase 3 - Convoy Route State

Status: Complete

Implementation/static validation complete. HST_Dev smoke test confirmed route
waypoint reporting, generated-route diagnostics, faction-catalog vehicle
variety, and clean convoy staging without resource-error spam.

Goal: move from start/end-only convoy behavior toward explicit route data.

Implementation:

- Add `HST_RouteWaypointState`.
- Extend `HST_GeneratedRouteState` with an ordered route waypoint array.
- Generate at least start, midpoint, and destination waypoints.
- Add route validation fields: road route, vehicle-safe, distance meters, and
  waypoint count.
- Add route reporting to generated content reports and convoy reports.
- Read base-game faction campaign entity catalogs for convoy vehicle
  candidates, then validate wheeled/tracked ground vehicles and exclude
  helicopters/aircraft.

Suggested state:

```c
[BaseContainerProps()]
class HST_RouteWaypointState
{
	string m_sRouteId;
	int m_iIndex;
	vector m_vPosition;
	int m_iRadiusMeters;
	string m_sHint;
}
```

Acceptance criteria:

- Generated content creates route waypoints.
- Every generated route has at least three waypoints.
- Convoy mission references a route or produces a clear fallback reason.
- Route report shows waypoint count and distance.
- Invalid route does not crash mission start.
- Convoy vehicle selection uses verified faction-catalog ground vehicle prefabs
  first, with GUID-qualified fallback prefabs only when the catalog is
  unavailable or sparse.
- Invalid or guessed vehicle prefab candidates are removed or skipped without
  resource errors.
- Ground-vehicle candidate report shows usable convoy vehicle counts per
  faction.

## Phase 4 - Convoy Readiness Gating

Status: Complete

Implementation/static validation and HST_Dev smoke testing complete.

Goal: prevent convoys from entering `convoy_moving` unless they are actually
ready to move.

Implementation:

- Separate vehicle asset exists, physical vehicle spawned, crew group spawned,
  crew alive, driver available, and waypoint/route assigned.
- Add readiness check before staging changes to moving.
- Remain staging while within grace period.
- Fall back to convoy contact/static ambush or fail with explicit reason after
  grace period.
- Add readiness status to convoy report.

Acceptance criteria:

- Convoy does not enter moving phase if no vehicles spawned.
- Convoy does not enter moving phase if crew groups failed.
- Convoy does not enter moving phase if route/waypoint assignment failed.
- Failure or fallback reason is visible in convoy report.
- Existing static fallback still lets players complete/fail the mission.
- Readiness report separates vehicle assets, spawned vehicles, crew groups,
  alive crew, driver availability, route assignment, and waypoint assignment.
- Convoy moving notifications, markers, and status changes are emitted only
  after readiness succeeds.

## Phase 5 - Convoy Vehicle-Control Adapter

Status: Complete

Goal: isolate Reforger vehicle seating, driver, gunner, and route-control logic
behind one adapter.

Implementation:

- Create `HST_ConvoyVehicleControlAdapter`.
- Add methods to bind crew, assign a vehicle route, count living crew, check
  for a living driver, and check whether a vehicle is mobile.
- Return explicit success/failure reasons from every method.
- Make `HST_PhysicalWarService` call the adapter instead of assuming
  `AddWaypoint` means the vehicle can move.

Suggested interface:

```c
class HST_ConvoyVehicleControlAdapter
{
	bool TryBindCrewToVehicle(
		HST_ActiveGroupState groupState,
		IEntity groupEntity,
		IEntity vehicleEntity,
		out string reason
	);

	bool TryAssignVehicleRoute(
		HST_ActiveGroupState groupState,
		IEntity groupEntity,
		IEntity vehicleEntity,
		array<vector> waypoints,
		out int assignedWaypointCount,
		out string reason
	);

	int CountLivingCrew(IEntity groupEntity);

	bool HasLivingDriver(IEntity groupEntity, IEntity vehicleEntity);

	bool IsVehicleMobile(IEntity vehicleEntity, out string reason);
}
```

Acceptance criteria:

- Project compiles with the new adapter.
- Convoy logic calls the adapter.
- Convoy report shows adapter result.
- Existing convoy behavior is preserved or fails with clearer reasons.
- No raw `IEntity` references are added to persistent state.

## Phase 6 - Real Convoy Crew Seating

Status: Complete

Implementation/static validation and HST_Dev smoke testing complete.

Goal: make convoy vehicles actually controllable by AI.

Implementation:

- In `HST_ConvoyVehicleControlAdapter`, discover living group-controlled AI
  agents and vehicle compartments.
- Assign the first suitable AI to the driver seat using animated compartment
  access.
- Assign the next suitable AI to gunner/turret seats if available.
- Assign remaining AI to cargo/passenger seats.
- Verify the driver is a living group-controlled occupant in a pilot slot.
- Add seating result fields to convoy reports: driver assigned, seated crew,
  turret seated, cargo seated, seating pending, and last seating reason.
- Retry seating during convoy staging before readiness checks.
- Keep convoy waypoint assignment blocked until every active convoy vehicle has
  a seated living AI driver.
- Enforce convoy start-to-end straight-line horizontal distance between 2000m
  and 5000m, and report planned distance, band validity, and whether generated
  route staging is vehicle-safe.

Acceptance criteria:

- Spawned convoy vehicle has an AI driver.
- Convoy report says driver assigned.
- Convoy report says how many crew were seated.
- Convoy report says planned distance is inside the 2000-5000m band.
- Convoy report says planned source/target are route-staged and vehicle-safe.
- Missing driver seats and missing crew agents produce clear reasons.
- Vehicle is not considered ready unless driver assignment succeeds.
- No persistent raw `IEntity` references or save-schema migrations are added.

## Phase 7 - Convoy Waypoint-Chain Movement

Status: Complete

Implementation/static validation and HST_Dev smoke testing have met the phase
criteria. Follow-up reliability work now belongs to Phase 8.

Goal: replace single destination waypoint with an ordered route waypoint chain.

Implementation:

- Build an ordered `array<vector>` from route waypoints.
- Spawn an AI waypoint for each route point.
- Add waypoints to the convoy AI group in order.
- Optionally stagger departure by vehicle index.
- Store assigned waypoint count in `HST_ActiveGroupState.m_iAssignedWaypointCount`
  and report.
- Keep old single-waypoint path only as fallback.

Acceptance criteria:

- Convoy report shows assigned waypoint count.
- Waypoint count is greater than one for generated routes.
- Convoy enters moving phase only after waypoint-chain assignment succeeds.
- If waypoint spawning fails, convoy falls back to static ambush/contact with a
  clear reason.
- Vehicles attempt to travel toward the destination.

## Phase 8 - Convoy Progress, Stuck Detection, And Destination Arrival

Status: Complete

Implementation/static validation target: Phase 8 now tracks transient convoy
progress in `HST_PhysicalWarService` without adding campaign save fields. A
smoke-test follow-up makes destination arrival win over single-group dismount
recovery, keeps convoy starts compact and vehicle-safe without shortening the
2000-5000m convoy trip, narrows arrival to 50 meters, and uses
RoadNetworkManager-resolved road endpoints, waypoints, and recovery snaps.
HST_Dev smoke testing passed for road-based spawning, movement, stuck recovery,
arrival failure with living crew, crew-neutralized completion, and inactive
convoy runtime cleanup.

Goal: track whether convoy vehicles are actually moving and respond if they get
stuck.

Implementation:

- Sync convoy vehicle positions every five seconds.
- Track distance to destination per vehicle.
- Track last movement/progress timestamp.
- Detect no-progress/stuck state.
- Reissue route once after a stuck threshold.
- Resolve convoy starts, destinations, assigned waypoint positions, and recovery
  snap points through RoadNetworkManager road geometry instead of terrain
  material or generic safe-ground checks.
- If stuck too long, snap the vehicle only to a nearby road-resolved point when
  the nearest player is at least 300 meters away.
- Keep physical and report the stuck reason when players are too close, no
  road snap point can be resolved, the point is too close to destination, the
  point would advance too far toward destination, or crew
  reseating cannot confirm a driver.
- Move unseated living crew to the recovered vehicle, reseat through the convoy
  vehicle adapter, orient the vehicle along route travel direction, then reissue
  route only after driver recovery succeeds.
- Keep marker refresh at 30 seconds.
- Fail convoy if it reaches destination with living crew.
- Resolve destination arrival before moving convoy dismount/contact fallback.
- Treat a single dismounted or driverless moving convoy group as a bounded
  reseat/rebind recovery issue instead of whole-convoy contact fallback.
- Stage convoy vehicle starts as compact road-resolved columns; destinations
  and all assigned waypoint positions must also be road-resolved.
- Choose a seeded random convoy start distance between 2000m and 5000m each
  time convoy assets are initialized.
- Pre-probe the full convoy vehicle column before creating mission assets; if
  any vehicle slot fails the safe-spawn or distance-band checks, reject that
  candidate and probe another instead of creating a partial convoy.
- Reject convoy spawn slots that are not RoadNetworkManager road points wide
  enough for vehicle footprint samples; terrain height/footprint checks remain
  secondary stability checks only.
- Re-check road placement during physical vehicle spawn and route-snap recovery,
  and use a smaller spawn lift to avoid dropping trucks onto uneven terrain.
- Preserve the existing 2000-5000m convoy travel-distance contract for every
  planned convoy vehicle slot.
- Keep convoy vehicle start slots in a compact route-column formation instead
  of scattering them through wide spawn-clearance searches.
- Wait for a reseated driver before consuming a route reissue attempt.
- Show live computed distance-to-destination in the convoy report, with sampled
  progress distance retained as a diagnostic.

Acceptance criteria:

- Moving convoy updates mission asset current positions.
- Convoy marker updates periodically.
- Convoy report shows distance to destination.
- Convoy report shows stuck/no-progress status.
- Convoy report shows RoadNetworkManager availability and road-resolved source,
  current, target, waypoint, and recovery placement results.
- Route is not reissued every tick.
- Convoy fails if destination is reached with living crew.
- Convoy does not falsely fail while still staging.
- A single dismounted convoy group does not immediately switch the mission to
  `convoy_contact`.
- Convoy report shows moving recovery reasons for dismounted or driverless
  groups.
- Convoy arrival failure uses a 50 meter destination radius.
- Convoy planned distance remains inside the 2000-5000m band for the convoy
  column, not only the first vehicle.
- Convoy setup creates either all planned convoy vehicle assets or none; it
  should not create a mission with later convoy vehicles guaranteed to fail
  physical spawn.
- Convoy pre-probing and physical spawn reject non-road, narrow-road, rocky, or
  uneven large-vehicle footprints instead of allowing trucks to spawn off-road,
  tipped, or on rocks.
- Convoy report shows route snap attempt/result, distance-to-destination, player
  gate, and crew reseat result.
- Convoy vehicles spawn as a compact column on generated route staging rather
  than hundreds of meters apart.
- Convoy Runtime Report shows the assigned road-snapped runtime waypoint chain
  per convoy group instead of treating static route-template waypoints as the
  authoritative movement path.

HST_Dev smoke steps:

1. Launch `Missions/HST_Dev.conf` in Workbench Play mode.
2. Press `I`, open Setup, and choose `Start HQ: central hills`.
3. Open Admin and run `Force: Ammo convoy`.
4. Open Missions and run `Convoy Runtime Report`.
5. Wait for the convoy to leave staging; verify phase becomes `convoy_moving`,
   waypoint count is greater than one, band valid is `yes`, and
   distance-to-destination appears. Vehicle source/current distances should all
   be inside the 2000-5000m band, and the vehicles should be a compact route
   column on roads, not scattered hundreds of meters apart, upside down, or
   sitting on visible rocks/steep side-tilt terrain.
6. Watch the map for convoy marker movement; it should refresh periodically,
   about every 30 seconds.
7. For unstuck testing, block a convoy vehicle or let terrain stop it, then
   move the player at least 300 meters away. After the hard-stuck window, the
   report should show route reissue, then route snap to a nearby road-resolved
   point, distance-to-destination for the snap point, road forward/orientation,
   and crew reseat result.
8. If one crew group dismounts, run `Convoy Runtime Report`; the mission should
   remain `convoy_moving` unless every moving group has lost control, and the
   report should show moving recovery status.
9. For arrival testing, do not kill the crew. When the convoy reaches
   within 50 meters of the destination with living crew, the mission should
   fail with `Convoy reached its destination with living crew.`
10. Repeat once with crew killed before arrival; the convoy should not falsely
   fail due to destination arrival after crew neutralization.

## Phase 9 - Convoy Contact Behavior

Status: Complete

Implementation/static validation: Phase 9 adds a 120 meter convoy contact
radius and transitions active convoy missions into `convoy_contact` when the
ambush starts. Contact is stored through existing active mission phase state and
does not add campaign schema fields. Static validation passed, and HST_Dev
smoke testing confirmed proximity contact without contact-only completion.

Goal: make ambushes transition convoys into a combat/contact state.

Implementation:

- Detect contact when a player is within 120 meters, crew count decreases,
  convoy vehicle is destroyed, vehicle is captured, or later when
  shots/explosions are nearby.
- Set mission phase to `convoy_contact`.
- Apply `convoy_contact` status to active convoy groups.
- Keep the objective active.
- Do not instantly fail or complete just because contact started.
- Preserve `convoy_contact` when generic mission vehicle destroy/capture
  handlers update convoy vehicle assets.
- Show contact radius and contact reason in the convoy runtime report.

Acceptance criteria:

- Approaching convoy changes phase to `convoy_contact`.
- Killing convoy crew changes objective progress.
- Destroying a convoy vehicle updates asset/runtime state.
- Convoy can still complete after contact.
- Contact phase survives save/load.

HST_Dev smoke steps:

1. Launch `Missions/HST_Dev.conf` in Workbench Play mode.
2. Press `I`, open Setup, and choose `Start HQ: central hills`.
3. Open Admin and run `Force: Ammo convoy`.
4. Open Missions and run `Convoy Runtime Report`; wait until the convoy has
   spawned and preferably reaches `convoy_moving`.
5. Approach within 120 meters of the convoy without killing anyone. Run
   `Convoy Runtime Report` again and verify mission phase is `convoy_contact`,
   convoy groups show `convoy_contact`, mission status is still active, and
   objective progress has not completed or failed from contact alone.
6. Kill one convoy crew group. Run the report and verify crew neutralized
   progress updates, the mission remains active/contact, and it does not
   instantly complete unless all required convoy crew groups are neutralized.
7. Destroy one convoy vehicle. Run the report and verify the matching convoy
   vehicle asset shows destroyed/runtime destroyed state while the mission
   remains `convoy_contact`.
8. Try capturing a convoy vehicle while its crew is alive; it should be blocked.
   Neutralize that vehicle's crew, capture it again, and verify the asset shows
   captured/delivered without generic phase `captured` replacing
   `convoy_contact`.
9. Neutralize all convoy crew groups after contact. Verify the convoy can
   progress to `convoy_eliminated` and then complete through the normal mission
   outcome path.
10. While the mission is in `convoy_contact`, run `Manual checkpoint`, restart
   or reload HST_Dev, and verify the active mission still reports
   `convoy_contact`.

## Phase 10 - Generic Convoy Completion

Status: Complete

Implementation/static validation: Phase 10 makes convoy completion
server-authoritative through crew elimination and destination-arrival checks.
The convoy runtime report now exposes completion eligibility, crew progress,
living crew, and vehicle resolution counts. HST_Dev smoke testing confirmed
crew-gated capture, destroyed/captured vehicle exclusion, generic success on
all crews eliminated, failure on live-crewed arrival, preserved intact vehicles
after crew-elimination success, and failed convoy cleanup. Static validation
passed.

Goal: make generic convoy success/failure reliable before adding
mission-specific rewards.

Implementation:

- Count total convoy groups, eliminated convoy groups, and living convoy crew.
- Complete objective when all required convoy crews are eliminated.
- Fail mission when convoy arrives with living crew.
- Allow vehicle capture only after associated crew is neutralized.
- Cleanly mark destroyed/captured convoy vehicles.
- Preserve intact convoy vehicles after crew-elimination success so players can
  optionally capture them; destroyed vehicles and failed/arrived convoys still
  clean up.
- Keep rewards and penalties centralized through the coordinator's normal
  mission success/failure path.
- Report `can complete`, `must fail`, reason, required/eliminated groups,
  living crew, and active/destroyed/captured vehicle counts in Convoy Runtime
  Report.

Acceptance criteria:

- Killing one convoy crew updates progress 1/N.
- Killing all convoy crews completes objective.
- Capturing a vehicle with living crew is blocked.
- Capturing a vehicle after crew is eliminated succeeds.
- Destroyed convoy vehicle is not respawned.
- Intact convoy vehicles remain in-world after crew-elimination success.
- Live-crewed arrival fails the mission and cleans up convoy assets.
- Mission success applies generic reward once.
- Mission failure applies failure penalty once.

## Phase 11 - Mission-Specific Convoy Outcomes

Status: Complete

Goal: make convoy mission types meaningfully different.

Implementation:

- Add outcome hooks: `OnConvoyArrived`, `OnConvoyCrewEliminated`,
  `OnConvoyVehicleCaptured`, `OnConvoyCargoDelivered`, and
  `OnConvoyMissionExpired`.
- `convoy_ammo`: ammo truck capture creates an ammo source or ammo reward.
- `convoy_armored`: captured armor can be garaged; composition scales with war
  level.
- `convoy_money`: full payout requires money vehicle/cargo delivered to HQ.
- `convoy_prisoners`: prisoners/captive asset must be freed/extracted for full
  reward.
- `convoy_reinforcements`: arrival strengthens the target garrison.
- `convoy_supplies`: arrival increases occupier support; captured/delivered
  supplies increase FIA support.

Acceptance criteria:

- Reinforcements convoy arrival increases target garrison.
- Supplies convoy arrival increases occupier town support.
- Captured supplies delivered to town increases FIA support.
- Ammo truck capture creates an ammo-source effect or visible garage/source
  flag.
- Money convoy does not grant full payout unless delivered.
- Prisoner convoy can grant HR/support when prisoners are extracted.

Completion notes:

- Added mission-specific convoy outcome service, save guards, runtime reports,
  mission asset follow-up actions, I-menu guidance, and map marker transitions.
- HST_Dev smoke testing confirmed supply delivery and prisoner extraction
  outcomes; cleanup now preserves neutralized convoy vehicles after delivery so
  players can keep or capture them.
- `tools/validate-foundation.ps1` passes with schema 18.

## Phase 12 - Active Mission Persistence

Status: Complete

Goal: make active missions, especially convoys, survive save/load.

Implementation:

- Save active mission fields, objectives, mission assets, runtime entity
  records, active convoy groups, and runtime vehicle records.
- On restore, do not assume physical entities still exist.
- Respawn or abstract active runtime based on player proximity and mission
  phase.
- Add a persistence smoke test for active convoy.

Acceptance criteria:

- Save during convoy staging, restart, convoy remains staging.
- Save during convoy moving, restart, convoy remains active.
- Save during convoy contact, restart, objective progress remains.
- Destroyed convoy vehicles stay destroyed after restart.
- Captured convoy vehicles stay captured/garaged after restart.
- Mission does not duplicate vehicles after restore.
- Active non-convoy mission records preserve objectives, mission assets, and
  final status through save/load.
- Restore path does not duplicate mission runtime entities or active groups.

Current notes:

- Active mission restore now repairs runtime fields, objective-to-asset links,
  mission asset runtime records, and restored spawned flags without adding a
  new save schema.
- Restored convoy runtime normalizes staging/moving/contact phases, rebuilds
  missing live handles from saved convoy assets/groups, preserves destroyed or
  captured vehicle state, and removes the short restore grace-window dependency.
- Re-review hardening keeps preserved convoy crew counts alive through the
  Workbench zero-agent population window so restored or freshly respawned
  convoy groups do not fail before agents report live.
- Persistence smoke coverage now seeds staging, moving, and contact convoy
  records and reports duplicate mission assets, runtime entities, active
  groups, and runtime vehicles.
- Workbench follow-up: smoke fixtures are diagnostic-only and are skipped by
  map markers, mission runtime ticking, convoy physicalization, mission intel,
  and normal command UI surfaces so seeded smoke state does not appear as live
  campaign content.
- `tools/validate-foundation.ps1` passes with schema 18.
- HST_Dev Workbench smoke passed for active convoy restore across moving/contact:
  destroyed convoy slots remain terminal after restart, survivor groups do not
  respawn in a loop, restored convoy completion applies the FIA reward, and the
  completed convoy no longer falls through into failure.

## Phase 13 - Non-Convoy Mission Primitive Hardening

Status: Complete

Goal: bring other mission types up to the same reliability level as convoys.

Mission primitives:

- `kill_hvt`
- `hold_area`
- `clear_area`
- `destroy_target`
- `recover_cargo`
- `rescue_extract`
- `deliver_supplies`

Implementation:

- Runtime assets spawn reliably.
- Objectives link to correct assets/entities.
- Player interaction validates distance and state.
- Objective completion is deterministic.
- Failure reason is clear.
- Runtime cleanup is safe.
- Save/load restore works.

Acceptance criteria:

- Assassination can be completed by killing/sabotaging HVT.
- Destroy mission completes when target is destroyed.
- Logistics recovery completes on pickup; supply delivery completes on delivery.
- Rescue mission can free and deliver captives.
- Hold/clear mission progresses only when conditions are met.
- All mission types show useful runtime reports.
- All mission types survive save/load.

Current notes:

- Non-convoy primitive polling is now explicit for HVT, hold, clear, destroy,
  cargo recovery, rescue/extract, and supply delivery missions.
- Mission asset initialization is idempotent by role/count, so partial restored
  primitive state can self-repair without duplicating assets.
- Asset-driven objectives continue to progress when matching mission assets
  exist, even if a fallback prop is present.
- Recover-cargo objectives now remain pickup-complete. Restored legacy
  `hq_delivery` objectives for active recover-cargo missions are completed by
  the restore repair pass instead of blocking completion.
- Runtime reports now include asset interaction radius, cargo capacity,
  attached/carrier state, outcome state, and delivery distance.
- Persistence smoke coverage now seeds one active mission for each non-convoy
  primitive.
- Workbench follow-up: primitive smoke fixtures are excluded from ordinary
  mission reports, objective ticking, mission start duplicate gates, and map
  marker publication while remaining visible to the persistence smoke report.
- HST_Dev primitive testing passed well enough to close the phase: HVT kill,
  hold/conquest, clear-area, destroy target, resource cache, POW rescue/follow,
  city supplies, mission action visibility, save/load carrier restore, and
  persistence smoke are all covered by implementation and validation.
- Expired missions now preserve and continue player-bound cargo/POW extraction
  scenes, and expired convoy contact preserves the live firefight while players
  remain inside the render bubble.
- Radio tower behavior is acceptable for Phase 13 completion, but radio tower
  polish and any remaining duplicate/static composition cleanup should be
  revisited later during campaign soak follow-up.
- `tools/validate-foundation.ps1` passes with schema 18.

## Phase 14 - Arsenal, Loot, And Finite/Infinite Unlock Loop

Status: In progress - 1/2 complete checkpoint. Arsenal, loot, finite/INF
policy, vehicle cargo, and field-vehicle save/load restore have passed HST_Dev
smoke. The remaining half is the loadout editor end-to-end path.

Goal: complete the Antistasi loot-to-unlock progression loop.

Implementation:

- Area loot deposits eligible items into the arsenal.
- Vehicle loot stores eligible items in nearby vehicle cargo.
- Vehicle cargo can transfer to the arsenal.
- Arsenal counts finite items.
- Items unlock when threshold is reached.
- Unlocked items become infinite.
- Blocked items never unlock or never deposit, depending on config.
- Loadout editor validates finite/infinite cost before applying.
- Failed loadout application does not lose items.

Acceptance criteria:

- Depositing an item increases arsenal count.
- Item unlocks at configured threshold.
- Withdrawing a finite item decrements count.
- Withdrawing an unlocked item does not decrement count.
- Blocked item does not unlock.
- Loadout apply is atomic.
- Save/load preserves arsenal counts and unlock state.

Checkpoint notes:

- Arsenal item rules are typed balance config, with blocked and finite-only
  behavior visible in reports.
- Raw visual/support assets are rejected from arsenal and vehicle cargo.
- Area loot, vehicle cargo collection, vehicle unload, and reports have been
  exercised in HST_Dev.
- Nearby Workbench-spawned field vehicles are snapshotted before manual
  checkpoint and restored after save/load as `loot_vehicle` runtime records.
- Loadout editor smoke remains before Phase 14 can be marked complete.

## Phase 15 - Garage And Vehicle Persistence

Status: Planned

Goal: make captured vehicles a reliable campaign progression system.

Implementation:

- Capture nearby vehicle into garage.
- Validate vehicle root.
- Validate vehicle is not occupied.
- Capture physical and virtual cargo.
- Delete physical vehicle only after garage record is prepared.
- Redeploy vehicle near HQ.
- Restore cargo on redeploy.
- Track ammo/repair/fuel source vehicles.

Acceptance criteria:

- Captured vehicle appears in garage report.
- Captured vehicle despawns from world.
- Vehicle cargo is preserved.
- Redeployed vehicle spawns at safe HQ position.
- Garage record is removed or updated according to redeploy policy.
- Ammo truck is recognized as ammo source.
- Save/load preserves garage vehicles and cargo.

## Phase 16 - Recruitment, Training, And Garrisons

Status: Planned

Goal: connect HR, money, training, arsenal unlocks, and abstract garrisons.

Implementation:

- Recruit FIA units using HR and money.
- Train troops using faction money.
- Training level affects AI quality.
- Arsenal unlocks affect recruit loadouts.
- Commander can add/remove garrison units.
- Garrison records remain abstract until activated.
- Active group survivors fold back into garrison state.

Acceptance criteria:

- Recruiting spends HR and money.
- Training spends faction money and increases training level.
- Recruit report changes when weapons unlock.
- Garrison can be added to a friendly zone.
- Entering a zone activates garrison physically.
- Killing part of a garrison and leaving folds survivors back.
- Save/load preserves garrisons.

## Phase 17 - Zone Capture And Ownership

Status: Planned

Goal: make the map conquest loop work.

Implementation:

- Detect players/FIA inside capture radius.
- Detect hostile active groups inside capture radius.
- Progress capture only when FIA are present and hostiles are cleared or below
  threshold.
- Pause or decay progress when contested.
- Flip owner when progress reaches threshold.
- Create/update FIA garrison after capture.
- Update markers.
- Trigger enemy counterattack chance.

Acceptance criteria:

- Empty enemy outpost can be captured by holding.
- Hostile presence pauses capture.
- Capture progress appears in report/UI.
- Zone owner changes to FIA on completion.
- Marker changes owner/color.
- Enemy counterattack order can be queued.
- Save/load preserves captured owner and progress.

## Phase 18 - Enemy Commander Physical Responses

Status: Planned

Goal: turn enemy orders into visible war activity.

Enemy order types:

- Patrol
- QRF
- Counterattack
- Rebuild garrison
- Roadblock
- Support call
- Petros attack

Implementation:

- Keep enemy resource spending server-authoritative.
- Queue enemy orders from aggression, war level, capture events, and HQ
  knowledge.
- Physicalize QRF/counterattack only near players or active objectives.
- Resolve off-screen orders abstractly.
- Update garrisons, roadblocks, support requests, or mission state on
  resolution.

Acceptance criteria:

- Enemy order report shows queued/active/resolved orders.
- Capturing a zone can trigger counterattack.
- Counterattack spends enemy resources.
- If players stay nearby, counterattack can spawn physically.
- If players leave, order resolves abstractly.
- Rebuild order increases enemy garrison.
- Roadblock order increases roadblock presence.

## Phase 19 - Support Requests

Status: Planned

Goal: make FIA and enemy support requests stateful and testable.

Implementation:

- Player/FIA support requests: supply support and ground support; future
  mortar/air only if assets exist.
- Enemy support requests: QRF, patrol/search, ground support, and abstract
  helicopter-style support if no safe asset path exists.
- Each request validates cost/cooldown, records source/target, has ETA, and
  physicalizes or resolves abstractly.

Acceptance criteria:

- FIA support request appears in support report.
- Enemy support request spends enemy resources.
- Request has ETA and status.
- Request resolves physically when players are nearby.
- Request resolves abstractly when players are far.
- Cooldown prevents spam.
- Save/load preserves active support requests.

## Phase 20 - Civilians, Town Support, And Undercover Reports

Status: Planned

Goal: make the civilian layer visible and start enforcing undercover rules
gradually.

Implementation:

- Town support report: FIA support, occupier support, reputation, wanted heat,
  police presence, and roadblock presence.
- Undercover eligibility report: clothing reason, weapon reason, vehicle
  reason, off-road reason, enemy proximity reason, and wanted heat reason.
- Add player commands to request undercover, clear compromised undercover, and
  show undercover status.

Acceptance criteria:

- Town report shows support/reputation/wanted state.
- Undercover report gives explicit eligibility reason.
- Civilian clothing can be eligible.
- Military equipment can be ineligible.
- Wanted heat can block or compromise undercover.
- Save/load preserves civilian and undercover state.

## Phase 21 - Undercover Enforcement And Police/Roadblocks

Status: Planned

Goal: move from reports to actual undercover gameplay.

Implementation:

- Apply undercover state to player.
- Detect suspicious actions: weapon drawn/fired, military gear, military
  vehicle, off-road driving near enemies, restricted zone entry, and
  roadblock/checkpoint scan.
- Compromise player when detection succeeds.
- Increase wanted heat after hostile/civilian incidents.
- Roadblocks and police increase detection chance.
- Report why undercover was lost.

Acceptance criteria:

- Player can enter undercover when eligible.
- Player loses undercover when firing/drawing weapon near enemies.
- Military vehicle blocks undercover.
- Roadblock/police presence can compromise player.
- Compromise reason is visible.
- Wanted heat decays or clears according to rules.

## Phase 22 - HQ Threat And Defend Petros

Status: Planned

Goal: implement the signature enemy punishment loop around HQ knowledge and
Petros.

Implementation:

- Track HQ knowledge.
- Increase HQ knowledge from traitor failure, enemy patrol near HQ, player
  activity near HQ, and high-aggression events.
- Enemy commander can queue Petros attack.
- Spawn enemy attack groups at HQ standoff positions.
- Mission objective: defend Petros for duration or eliminate attackers.
- Failure: Petros killed, HR/money penalty, possible forced HQ move or
  recovery.

Acceptance criteria:

- HQ knowledge is visible in report.
- Enemy commander can queue Petros attack.
- Defend Petros mission starts.
- Attackers spawn outside HQ safe radius.
- Petros death fails mission.
- Successful defense clears/lowers threat or completes mission.
- Save/load preserves active defend-Petros mission.

## Phase 23 - UI And Map Marker Polish

Status: Planned

Goal: make the campaign understandable without reading logs.

Implementation:

- Improve menu tabs: Overview, HQ/Petros, Missions, Map/War, Forces,
  Arsenal/Loot, Members, and Admin.
- Improve marker behavior for zone owners, active missions, objectives,
  convoy last-known position, support/QRF, capture progress, and HQ marker
  visibility rules.

Acceptance criteria:

- Player can understand active mission state from the menu.
- Convoy marker updates while moving.
- Zone markers update after capture.
- Support/QRF markers appear when relevant.
- Failed actions show clear result text.
- Admin/debug actions are separated from normal player actions.
- UI preserves command/report paths for every feature introduced by the phase
  plan.
- Service failure reasons are shown verbatim or safely summarized instead of
  being hidden behind generic UI text.

## Phase 24 - Balance, Campaign Pacing, And Victory/Loss

Status: Planned

Goal: turn working systems into a coherent campaign loop.

Implementation:

- Tune starting HR/money/training.
- Tune income intervals and values.
- Tune arsenal unlock thresholds.
- Tune mission rewards and penalties.
- Tune aggression gain/decay.
- Tune enemy resource income/spending.
- Tune war-level thresholds.
- Add victory condition.
- Add loss condition if desired.
- Add campaign end report.

Acceptance criteria:

- War level increases as FIA gains territory.
- Enemy equipment/resource pressure scales with war level.
- Early game remains loot-focused.
- Mid game enables outpost/resource capture.
- Late game enables airfield/seaport pressure.
- Victory triggers when required island-control condition is met.
- Campaign end state persists.

## Phase 25 - Full-Campaign Soak Testing

Status: Planned

Goal: find long-session bugs, restart bugs, and multiplayer edge cases.

HST_Dev smoke plan:

- Start campaign.
- Select HQ.
- Start each mission family.
- Complete/fail each primitive.
- Save/restart after each.

Convoy soak plan:

- Start convoy.
- Save during staging, moving, and contact.
- Destroy one vehicle.
- Capture one vehicle.
- Let convoy arrive.
- Kill all crews.
- Verify no duplicate vehicles after restart.

Campaign-loop plan:

- Loot items.
- Unlock item.
- Recruit FIA.
- Capture zone.
- Trigger counterattack.
- Garage vehicle.
- Redeploy vehicle.
- Request support.
- Go undercover.
- Trigger wanted heat.

Multiplayer plan:

- Join as host.
- Join as second client.
- Verify guest/member/commander permissions.
- Disconnect/reconnect.
- Save with multiple players.
- Restart and rejoin.

Acceptance criteria:

- No duplicate player spawns.
- No duplicate convoy vehicles after restart.
- No active mission becomes impossible without a clear failure path.
- Save/load does not lose arsenal, garage, garrisons, zones, HQ, missions, or
  support requests.
- Server can run a two-hour session without campaign tick errors.
- Full Everon can run with multiple active systems without runaway entity
  spawning.
- Static validation passes before and after soak fixes.
- Soak report records tested build or commit, scenario, duration, active systems,
  failures, and follow-up issues.

## Master Checklist

Use this as the high-level progress tracker for the whole mode. Individual
phase acceptance criteria are more specific; this checklist is meant for broad
campaign readiness. Unchecked items here do not necessarily mean earlier
diagnostic phases are incomplete; they mean the full campaign feature still
needs to prove that capability before the project is considered broadly ready.

### Campaign Foundation

- [ ] setup phase
- [ ] active phase
- [ ] end phase
- [ ] server-authoritative commands
- [ ] save/load
- [ ] schema migration

### Map Strategy

- [ ] zones
- [ ] towns
- [ ] resources
- [ ] factories
- [ ] seaports
- [ ] airfields
- [ ] radio towers
- [ ] banks/police nodes
- [ ] hideouts
- [ ] generated sites
- [ ] generated routes
- [ ] map markers

### Economy And Progression

- [ ] HR
- [ ] personal money
- [ ] faction money
- [ ] war level
- [ ] training level
- [ ] aggression
- [ ] enemy attack/support pools
- [ ] town support
- [ ] HQ knowledge

### HQ

- [ ] Petros
- [ ] arsenal/cache
- [ ] garage
- [ ] tent
- [ ] map/whiteboard
- [ ] flag/recruitment
- [ ] move HQ
- [ ] defend HQ
- [ ] Petros death penalty

### Players And Permissions

- [ ] identity
- [ ] members
- [ ] guests
- [ ] admins
- [ ] commander
- [ ] elections/eligibility
- [ ] permission-gated actions

### Arsenal, Loot, And Loadouts

- [ ] area loot
- [ ] vehicle loot
- [ ] loot boxes
- [ ] finite counts
- [ ] unlock thresholds
- [ ] blocked items
- [ ] infinite unlocked items
- [ ] saved loadouts
- [ ] issued-item ledger
- [ ] AI loadout selection

### Garage And Vehicles

- [ ] capture vehicle
- [ ] delete physical after garage
- [ ] preserve cargo
- [ ] redeploy vehicle
- [ ] ammo source
- [ ] repair source
- [ ] fuel source
- [ ] vehicle locks
- [ ] vehicle sell/buy

### Missions

- [ ] mission registry
- [ ] eligibility
- [ ] target selection
- [ ] objective creation
- [ ] runtime primitive init
- [ ] markers/tasks
- [ ] success/failure/timeout
- [ ] cleanup
- [ ] persistence restore

### Mission Primitives

- [ ] kill HVT
- [ ] clear/hold area
- [ ] destroy target
- [ ] recover cargo
- [ ] rescue/extract
- [ ] deliver supplies
- [ ] convoy intercept
- [ ] defend Petros

### Convoys

- [ ] route selection
- [ ] route waypoints
- [ ] vehicle composition
- [ ] crew spawn
- [ ] driver/gunner/passenger assignment
- [ ] movement orders
- [ ] progress tracking
- [ ] stuck detection
- [ ] contact behavior
- [ ] destination arrival
- [ ] crew elimination
- [ ] vehicle capture
- [ ] mission-specific outcomes
- [ ] save/load restore

### Garrisons And Physical War

- [ ] abstract garrisons
- [ ] zone activation
- [ ] group spawn
- [ ] patrol routes
- [ ] survivor fold-back
- [ ] QRF
- [ ] counterattack
- [ ] rebuild garrison
- [ ] roadblocks
- [ ] active group cleanup

### Enemy Commander

- [ ] resource income
- [ ] target scoring
- [ ] patrol orders
- [ ] QRF orders
- [ ] counterattack orders
- [ ] support calls
- [ ] Petros attacks
- [ ] roadblock orders
- [ ] rebuild orders
- [ ] order reports

### Civilians And Undercover

- [ ] civilian population
- [ ] police presence
- [ ] wanted heat
- [ ] town reputation
- [ ] undercover eligibility
- [ ] detection
- [ ] reported vehicles
- [ ] roadblock checks
- [ ] civilian casualties
- [ ] aid actions

### Support

- [ ] FIA support requests
- [ ] enemy support requests
- [ ] ground support
- [ ] supply drops
- [ ] abstract helicopter-style support
- [ ] cooldowns
- [ ] costs
- [ ] support markers

### UI/UX

- [ ] overview
- [ ] HQ/Petros
- [ ] missions
- [ ] map/war
- [ ] forces
- [ ] arsenal/loot
- [ ] members
- [ ] admin
- [ ] event feed
- [ ] action results
- [ ] marker refresh

### Testing

- [ ] `HST_Dev` mission smoke tests
- [ ] save/load smoke tests
- [ ] convoy movement tests
- [ ] garage/loot tests
- [ ] zone capture tests
- [ ] enemy order tests
- [ ] undercover tests
- [ ] full Everon soak
