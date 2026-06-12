# h-istasi

h-istasi is an original Arma Reforger implementation of the Antistasi
resistance-campaign concept. The first preset targets Everon with a three-way
war:

- FIA: player resistance
- US: occupying force
- USSR: invading force

## Current Status

The repository contains the first engine-facing increment:

- Reforger base-game project dependency only
- APL-ND licensing and third-party attribution
- Original Everon and compact development scenario shells
- Data contracts for presets, factions, maps, zones, balance, and missions
- A versioned campaign-state model with arsenal, mission-runtime,
  active-group, support, and persistence metadata
- Server-authoritative campaign, economy, mission, persistence, and
  native-checkpoint services
- A Community Edition 3.11.1 mission registry baseline
- Custom FIA HQ player spawn path for direct Workbench Play mode
- FIA spawnpoint/loadout authoring metadata for authored hideout candidates
- Setup-phase HQ selection, HQ movement, and Petros state hooks for the first
  playable resistance-campaign slice
- Abstract Antistasi systems for player lifecycle, town income/support, zone
  capture, garrisons, recruitment/training, and mission rewards/penalties
- Native respawn-request bootstrap with pending spawn tracking to avoid
  Workbench duplicate-spawn loops
- Everon alpha anchors and route-aware zone activation scaffolding for the
  physical AI war
- Dedicated Petros character prefab at
  `Prefabs/Characters/HST/Character_HST_Petros.et` for HQ contextual actions,
  with GUID metadata and a base-FIA spawn fallback only if the custom resource
  fails to spawn
- HST-owned HQ arsenal action surface at
  `Prefabs/Objects/HST/HST_HQArsenal.et`; stock FIA arsenal/MSAR behavior is
  not used for item authority, issuing, or loadout accounting
- Custom h-istasi loadout-editor service scaffold with personal saved
  loadouts, preview mannequin lifecycle, apply/cancel commands, finite/INF
  accounting, and an issued-item ledger owned by campaign state
- Procedural Antistasi-style `I` key HQ menu mounted on both development
  worlds with Setup, Overview, HQ/Petros, Missions, Map/War, Forces,
  Arsenal/Loot, Members, and Admin tabs
- First-load server settings generation at `$profile:h-istasi/HST_Settings.json`
  with config-backed defaults for campaign, factions, economy, membership,
  world activation, arsenal/loot, persistence, logging, and feature toggles
- Server-side area loot action that deposits eligible nearby gear into the
  campaign arsenal and removes transferred source items when configured
- Broad-alpha campaign scaffolding for generated Everon sites/routes, mission
  objectives, campaign tasks, physical mission-runtime primitives, support
  requests, enemy commander orders, civilian town state, and player undercover
  state
- Commander-facing no-admin actions for initial HQ selection, random mission
  start, objective/runtime inspection, FIA supply requests, support
  cancellation, civilian aid, support reports, garage reports, generated
  content reports, persistence status, and undercover status
- Phase 0/1 foundation and mission diagnostics are complete: the alpha command
  menu exposes foundation status, explicit checkpoint reporting, active mission
  inspection, selected mission inspection, and null-safe runtime mission fields
- Versioned campaign save-data container that is migrated, tracked through
  `PersistenceSystem`, and flushed before native `SaveGameManager`
  checkpoint requests when saving is possible

This is a foundation build, not a public alpha. Petros and the HQ arsenal have
live contextual-action prefabs for the alpha menu path, while the arsenal and
loadout editor remain custom h-istasi economy systems instead of stock/MSAR
arsenal behavior. The broad-alpha systems are intentionally rough scaffolds:
mission objectives can now complete from world proximity/active-group
conditions, support calls are stateful with native-safe ground group
activation, and helicopter-style support remains abstract until an approved
asset/dependency path exists. Cache/tent polish, save/restart soak testing,
final surveyed Everon coordinates, richer AI waypoints, physical player
inventory insertion from editor transactions, and mission-specific interactable
props still need to be connected incrementally.

## Alpha Command Menu

Press `I` in `HST_Dev` or `HST_Everon` to open the h-istasi alpha command
menu. The menu is a client widget driven by server-built snapshots and renders
an Antistasi-style HQ interface with a resource bar, navigation, campaign
cards, action list, and activity/result feed. The Setup tab displays the
effective server config and lets the commander choose the first HQ hideout
before the campaign enters the active phase. `$profile:h-istasi/HST_Settings.json`
remains the source of truth for defaults that apply to newly created
campaigns.

- `MenuUp` / `MenuDown`: change selection
- `MenuSelect`: run the selected command
- `MenuBack` or `I`: close the menu

The menu routes through server-authoritative coordinator requests and covers
campaign overview, markers, economy, zones, missions, mission runtime, manual
checkpoint, persistence status, income tick, training, first HQ selection, HQ
movement, FIA recruitment, mission start, random mission selection, objective
progress, support requests/cancellation, civilian aid, zone capture/activation,
arsenal reporting, garage reporting, nearby loot collection, generated content
reports, roster admin helpers, campaign reset, and small debug resource awards.
Multiplayer clients use a player-owned request/RPC component;
the server resolves the caller from ownership instead of trusting a client
provided player ID. Petros opens this same menu path through contextual
interactions, and the visible HQ arsenal is backed by the same server commands
from the Arsenal/Loot tab.

## Requirements

- Arma Reforger 1.7.0.41 or newer
- No external faction, weapon, vehicle, or support-call addon is required.

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
