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
- RHS-backed Workbench Play-mode spawn harness with Scenario Framework
  spawnpoints for authored hideout candidates
- HQ lifecycle service for initial hideout, HQ movement, Petros state, and
  Petros-loss penalties
- Coordinator dev actions for zone capture, income ticks, mission
  success/failure, training, recruitment, and garrison fold-back

## Next Playable Increment

- Index the new resources in Workbench and verify script compile
- Add a native `HST_CampaignState` serializer and restart test
- Replace central-hideout auto-selection with first-start hideout selection UI
- Add physical Petros, FIA tent, cache, and map markers at the selected HQ
- Replace temporary RHS deployment with custom FIA HQ spawning
- Add player-facing member, guest, commander election, and admin UI
- Add arsenal quantities, garage records, recruitment, garrisons, and map UI

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
