# Antistasi Community Edition 3.11.1 Parity Map

## Implemented Foundation

- Typed versioned campaign state
- Fixed FIA versus RHS_USAF versus RHS_AFRF preset
- CE-style resource pools, HR, money, support, aggression, and war-level
  service surface
- Persistent member, guest, admin, commander-vacancy, player lifecycle,
  town-support, income, arsenal, garage-record, abstract-garrison,
  recruitment, and enemy-pool service surfaces
- Common mission lifecycle and CE 3.11.1 mission-registry baseline
- Native Reforger manual and periodic checkpoint requests
- Original Everon world shell and stable strategic-zone IDs
- Custom FIA HQ player spawn path that bypasses stock Deployment Setup and
  uses game-mode player callbacks, a short spawn sweep, native respawn
  requests, pending spawn tracking, and spawn-success callbacks
- FIA Scenario Framework spawnpoints and role-selection loadouts retained as
  authoring metadata and fallback scaffolding
- HQ lifecycle service for initial hideout, HQ movement, Petros state, and
  Petros/cache/tent runtime object positions, and Petros-loss penalties
- Versioned campaign save container for current state fields and nested arrays
- Everon alpha anchors for strategic zones, towns, hideouts, routes, and
  mission sites
- Physical-war activation scaffold that marks nearby zones active and mirrors
  abstract garrison counts into runtime active counts
- Coordinator dev actions for zone capture, income ticks, mission
  success/failure, training, recruitment, and garrison fold-back
- Dedicated Petros character prefab that inherits from FIA rifleman but can be
  edited independently from player spawn characters; runtime spawning uses the
  GUID-indexed custom prefab first and falls back to the base FIA prefab only
  if needed
- HQ arsenal supply-cache prefab with contextual actions for opening the
  Arsenal/Loot tab and depositing nearby loot into campaign arsenal state
- Procedural Antistasi-style HQ menu with resource stats, overview, HQ/Petros,
  mission board, map/war, forces, arsenal/loot, member, admin, action, and
  activity/result panels
- Broad-alpha persistent state for generated sites/routes, mission objectives,
  campaign tasks, support requests, enemy orders, civilian town state, and
  player undercover state
- Schema 7 zone metadata for display names, resource kinds, capture radii,
  priority, composition IDs, spawn profiles, and linked-zone hints
- Everon 4x-style alpha campaign graph expansion with additional outposts,
  factories, resource depots, seaports, radio towers, banks, and police nodes,
  authored as h-istasi-owned config/anchors/marker stubs
- Generated Everon content service that creates alpha mission sites,
  roadblock/support/stash/crashsite points, and simple route records from the
  existing strategic zone anchors
- Mission objective/task service that attaches rough objectives to started
  missions and lets the no-admin commander flow progress them into normal
  mission completion rewards and strategic outcomes
- Stateful support request and enemy commander services inspired by
  DarcMissions/DarcChopper pacing: enemy pools can buy patrol/QRF/search
  requests, FIA can request supply support, and helicopter-style requests stay
  abstract/native-safe
- Civilian/undercover service with town reputation, wanted heat, police and
  roadblock presence, aid effects, and per-player undercover records
- Command menu actions for dynamic mission targets, FIA support requests,
  arsenal withdrawal, nearby vehicle garage capture, and garage redeploy
- Economy and enemy resource income now account for resource kind, priority,
  factories, ports, airfields, depots, radio towers, and police nodes

## Next Playable Increment

- Bind `HST_CampaignSaveData` into Reforger's persistent component load path
  and add restart/migration tests
- Replace central-hideout auto-selection with first-start hideout selection UI
- Replace the 4x-style alpha survey with exact unpacked Conflict Remixed
  marker-coordinate audit once a Workbench/PAC extraction path is available
- Add proper Antistasi HQ spawn/loadout UI over the custom FIA spawn backend
- Customize Petros appearance/loadout and replace tent/cache placeholders with
  authored h-istasi HQ entities
- Add player-facing member, guest, commander election, and admin UI
- Replace menu-progressed mission objectives with world-object detection,
  spawned mission props, convoy movement, captive interactions, and hold/clear
  checks
- Spawn/deactivate richer physical garrisons from active zone counts, assign
  waypoints/routes, and fold real survivors back into abstract garrisons

## Later Alpha Increments

- Deep town support, civilians, police, factories, resources, radio towers,
  city-flip battles, and undercover enforcement
- Hybrid AI activation, QRF travel, attacks, reinforcements, counterattacks,
  Petros attacks, and enemy rebuild behavior
- Mission-specific world logic and unique content for every registry entry
- Victory, loss, exact full-Everon coordinate survey, and 16-player soak tests

## Deferred Capabilities

The RHS-only preset must disable mechanics that cannot be honestly
represented with base Reforger and RHS Status Quo. Do not add hidden
dependencies or synthetic replacements.

- Fixed-wing aircraft support
- SEAD support when no suitable radar and SAM asset pair is available
- Any artillery support without a suitable physical RHS asset
