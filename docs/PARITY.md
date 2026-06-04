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

## Next Playable Increment

- Bind `HST_CampaignSaveData` into Reforger's persistent component load path
  and add restart/migration tests
- Replace central-hideout auto-selection with first-start hideout selection UI
- Add proper Antistasi HQ spawn/loadout UI over the custom FIA spawn backend
- Customize Petros appearance/loadout and replace tent/cache placeholders with
  authored h-istasi HQ entities
- Add player-facing member, guest, commander election, and admin UI
- Spawn/deactivate physical garrisons from active zone counts and fold
  survivors back into abstract garrisons

## Later Alpha Increments

- Town support, civilians, police, factories, resources, radio towers, and
  city-flip battles
- Hybrid AI activation, QRFs, attacks, reinforcements, and counterattacks
- Mission-specific world logic for every registry entry
- Victory, loss, full-Everon coordinate survey, and 16-player soak tests

## Deferred Capabilities

The RHS-only preset must disable mechanics that cannot be honestly
represented with base Reforger and RHS Status Quo. Do not add hidden
dependencies or synthetic replacements.

- Fixed-wing aircraft support
- SEAD support when no suitable radar and SAM asset pair is available
- Any artillery support without a suitable physical RHS asset
