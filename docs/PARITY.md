# Antistasi Community Edition 3.11.1 Parity Map

## Implemented Foundation

- Typed versioned campaign state
- Fixed FIA versus US versus USSR preset
- CE-style resource pools, HR, money, support, aggression, and war-level
  service surface
- Persistent member, guest, admin, commander-vacancy, player lifecycle,
  town-support, income, arsenal, vehicle-cargo, saved-loadout, issued-item,
  garage-record, runtime-vehicle, abstract-garrison, recruitment, and
  enemy-pool service surfaces
- Common mission lifecycle and CE 3.11.1 mission-registry baseline
- Native Reforger manual and periodic checkpoint requests with
  `PersistenceSystem` tracking for the scripted campaign save container
- Original Everon world shell and stable strategic-zone IDs
- Custom FIA HQ player spawn path that bypasses stock Deployment Setup and
  uses game-mode player callbacks, a short spawn sweep, native respawn
  requests, pending spawn tracking, and spawn-success callbacks
- FIA Scenario Framework spawnpoints and role-selection loadouts retained as
  authoring metadata and fallback scaffolding
- HQ lifecycle service for setup-driven initial hideout selection, HQ
  movement, Petros state, Petros/cache/tent runtime object positions, and
  Petros-loss penalties
- Versioned campaign save container for current state fields and nested arrays,
  with schema migration and restored-state application helpers
- Everon alpha anchors for strategic zones, towns, hideouts, routes, and
  mission sites
- Physical-war activation scaffold that marks nearby zones active, moves
  abstract garrison counts into route-aware active groups, and folds survivor
  counts back on deactivation
- Coordinator dev actions for zone capture, income ticks, mission
  success/failure, training, recruitment, and garrison fold-back
- Dedicated Petros character prefab that inherits from FIA rifleman but can be
  edited independently from player spawn characters; runtime spawning uses the
  GUID-indexed custom prefab first and falls back to the base FIA prefab only
  if needed
- HQ arsenal supply-cache prefab with contextual actions for opening the
  Arsenal/Loot tab, opening the custom loadout editor, and depositing nearby
  loot into campaign arsenal state; inherited stock arsenal actions are
  filtered from the h-istasi HQ arsenal surface
- Procedural Antistasi-style HQ menu with resource stats, overview, HQ/Petros,
  mission board, map/war, forces, arsenal/loot, garage/build, member, admin,
  action, and activity/result panels
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
- Mission runtime service that maps all 26 registry IDs into physical MVP
  primitives: kill HVT, hold/clear area, destroy target, recover cargo,
  rescue/extract, deliver supplies, and convoy intercept
- Stateful support request and enemy commander services inspired by
  DarcMissions/DarcChopper pacing: enemy pools can buy patrol/QRF/search
  requests, FIA can request supply support, and helicopter-style requests stay
  abstract/native-safe
- Civilian/undercover service with town reputation, wanted heat, police and
  roadblock presence, aid effects, and per-player undercover records
- Command menu actions for setup hideout selection, dynamic mission targets,
  mission runtime and persistence inspection, FIA support requests/cancel,
  arsenal withdrawal, vehicle-cargo collection/unload, nearby vehicle garage
  capture, build-mode garage redeploy, HQ runtime-asset rebuild, simple roster
  admin, and campaign reset
- Economy and enemy resource income now account for resource kind, priority,
  factories, ports, airfields, depots, radio towers, and police nodes
- Area and vehicle loot services with eligible-item scanning, base-game
  resource validation, blocked/finite-only/unlock policy checks, source item
  removal, HQ-object protection, and vehicle cargo reports
- Virtual garage scaffolding for safe root-vehicle capture, physical and
  virtual cargo preservation, verified despawn before record storage, dry-ground
  redeploy placement, runtime vehicle registration, and nearby field-vehicle
  snapshot/restore during checkpoint/restore flow
- Custom loadout editor path with server-authoritative HQ radius/member checks,
  live equipment and storage nodes, compatible candidate lists, five fixed
  personal save slots, `$profile:h-istasi/loadouts/v2` persistence, finite/INF
  cost ledgers, atomic apply/rollback, issued-item tracking, death-loss
  accounting, and removed external item purging
- Mission-specific convoy outcome state for delivered cargo/captives, captured
  vehicles, armored convoy garage handoff, ammo convoy ammo points, and outcome
  de-dupe across reloads

## Next Playable Increment

- Add restart/migration tests for `HST_CampaignSaveData` under native Reforger
  save/load
- Replace the 4x-style alpha survey with exact unpacked Conflict Remixed
  marker-coordinate audit once a Workbench/PAC extraction path is available
- Finish HST_Dev end-to-end smoke and UX polish for the custom loadout editor,
  including live inventory edge cases and save/load confidence
- Harden virtual garage/build scaffolding into a full vehicle progression loop
  with ammo/repair/fuel source behavior and longer restart testing
- Customize Petros appearance/loadout and replace tent/cache placeholders with
  authored h-istasi HQ entities
- Add full player-facing member, guest, commander election, and admin UI
- Replace remaining physical MVP mission completion shortcuts with
  mission-specific props, richer hold/clear checks, and mission-family polish
- Assign active groups real waypoints/routes and fold measured survivors back
  into abstract garrisons

## Later Alpha Increments

- Deep town support, civilians, police, factories, resources, radio towers,
  city-flip battles, and undercover enforcement
- Hybrid AI activation, QRF travel, attacks, reinforcements, counterattacks,
  Petros attacks, and enemy rebuild behavior
- Mission-specific world logic and unique content for every registry entry
- Victory, loss, exact full-Everon coordinate survey, and 16-player soak tests

## Deferred Capabilities

The base-game preset must disable mechanics that cannot be honestly
represented with stock Arma Reforger resources. Do not add hidden dependencies
or synthetic replacements.

- Fixed-wing aircraft support
- SEAD support when no suitable radar and SAM asset pair is available
- Any artillery support without a suitable physical base-game asset
