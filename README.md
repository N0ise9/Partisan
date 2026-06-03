# h-istasi

h-istasi is an original Arma Reforger implementation of the Antistasi
resistance-campaign concept. The first preset targets Everon with a three-way
war:

- FIA: player resistance
- RHS_USAF: occupying force
- RHS_AFRF: invading force

## Current Status

The repository contains the first engine-facing increment:

- Reforger and RHS project dependencies
- APL-ND licensing and third-party attribution
- Original Everon and compact development scenario shells
- Data contracts for presets, factions, maps, zones, balance, and missions
- A versioned campaign-state model with arsenal metadata persistence
- Server-authoritative campaign, economy, mission, and native-checkpoint
  services
- A Community Edition 3.11.1 mission registry baseline
- Custom FIA HQ player spawn path for direct Workbench Play mode
- FIA spawnpoint/loadout authoring metadata for authored hideout candidates
- HQ and Petros state hooks for the first playable resistance-campaign slice
- Abstract Antistasi systems for player lifecycle, town income/support, zone
  capture, garrisons, recruitment/training, and mission rewards/penalties
- Native respawn-request bootstrap with pending spawn tracking to avoid
  Workbench duplicate-spawn loops
- Everon alpha anchors and zone activation scaffolding for the physical AI war
- Dedicated Petros character prefab at
  `Prefabs/Characters/HST/Character_HST_Petros.et` for later appearance and
  loadout customization, with a safe base-FIA spawn fallback until Workbench
  indexes the new prefab resource
- Widget-backed `I` key command menu mounted on both development worlds with
  Setup, General, Petros/HQ, Commander, Arsenal/Loot, and Admin tabs
- First-load server settings generation at `$profile:h-istasi/HST_Settings.json`
  with config-backed defaults for campaign, factions, economy, membership,
  world activation, arsenal/loot, persistence, logging, and feature toggles
- Server-side area loot action that deposits eligible nearby gear into the
  campaign arsenal and removes transferred source items when configured
- Versioned campaign save-data container ready for native persistence binding

This is a foundation build, not a public alpha. Petros now has a dedicated
editable prefab asset; once Workbench assigns it a GUID resource name, the HQ
service can spawn that custom asset directly. Cache/tent polish, broader
spawned AI, native save/load binding, final surveyed Everon coordinates, and
mission-specific world logic still need to be connected incrementally.

## Alpha Command Menu

Press `I` in `HST_Dev` or `HST_Everon` to open the h-istasi alpha command
menu. The menu is a client widget driven by server-built snapshots. The Setup
tab displays the effective server config, but `$profile:h-istasi/HST_Settings.json`
remains the source of truth and applies to newly created campaigns only.

- `MenuUp` / `MenuDown`: change selection
- `MenuSelect`: run the selected command
- `MenuBack` or `I`: close the menu

The menu routes through server-authoritative coordinator requests and covers
campaign overview, markers, economy, zones, missions, manual checkpoint,
income tick, training, HQ movement, FIA recruitment, mission start, zone
capture/activation, arsenal reporting, nearby loot collection, and small debug
resource awards. Multiplayer clients use a player-owned request/RPC component;
the server resolves the caller from ownership instead of trusting a client
provided player ID.

## Requirements

- Arma Reforger 1.7.0.41 or newer
- RHS: Status Quo

## Design Rules

- Prefix addon-owned scripts and resources with `HST_`.
- Keep the strategic campaign server-authoritative.
- Use native Reforger persistence and session saves.
- Keep off-screen forces abstract and activate physical AI only around
  players and active objectives.
- Do not copy source assets from neighboring addons.
- Do not add hidden dependencies.

## Development Scenarios

- `Missions/HST_Everon.conf`: full-island campaign shell
- `Missions/HST_Dev.conf`: compact systems-development shell

See `docs/ARCHITECTURE.md` and `docs/PARITY.md` for the implementation map.
