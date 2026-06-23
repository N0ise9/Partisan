# h-istasi

h-istasi is an original Arma Reforger implementation of the Antistasi
resistance-campaign concept. The first preset targets Everon with a three-way
war:

- FIA: player resistance
- US: occupying force
- USSR: invading force

## Current Status

The repository contains a broad-alpha campaign foundation:

- Reforger base-game project dependency only
- APL-ND licensing and third-party attribution
- Original Everon and compact development scenario shells
- Data contracts for presets, factions, maps, zones, balance, and missions
- A versioned campaign-state model with arsenal, vehicle cargo, garage,
  saved-loadout, issued-item, mission-runtime, active-group, support, build-mode,
  campaign-end, and persistence metadata
- Server-authoritative campaign, economy, mission, persistence, arsenal, loot,
  loadout-editor, garage/build, and native-checkpoint services
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
  not used for item authority, issuing, or loadout accounting, and inherited
  stock arsenal actions are filtered away from the h-istasi action surface
- Custom h-istasi loadout editor with a client fullscreen widget, server-built
  payloads, live equipment/storage nodes, compatible candidate replacement,
  five fixed personal loadout slots under `$profile:h-istasi/loadouts/v2`,
  finite/INF accounting, atomic apply/rollback, and an issued-item ledger owned
  by campaign state
- Procedural Antistasi-style `I` key HQ menu mounted on both development
  worlds with Setup, Overview, HQ/Petros, Missions, Map/War, Forces,
  Arsenal/Loot, Garage/Build, Members, and Admin tabs
- First-load server settings generation at `$profile:h-istasi/HST_Settings.json`
  with config-backed defaults for campaign, factions, economy, membership,
  world activation, arsenal/loot, persistence, logging, and feature toggles
- Server-side area and vehicle loot actions that deposit eligible nearby gear
  into the campaign arsenal or vehicle cargo, remove transferred source items
  when configured, and reject raw visual/support assets
- Virtual garage and build-mode scaffolding for nearby vehicle capture,
  safe-root validation, cargo preservation, dry-ground redeploy placement,
  field-vehicle snapshot/restore, and HQ runtime-asset rebuilds
- Broad-alpha campaign scaffolding for generated Everon sites/routes, mission
  objectives, campaign tasks, physical mission-runtime primitives, support
  requests, enemy commander orders, civilian town state, and player undercover
  state
- HQ knowledge/threat state and Defend Petros linkage across mission, enemy
  order, support request, attacker-group, marker, and campaign-end state
- Enemy commander orders and support requests that can physicalize near players
  or resolve abstractly off-screen while preserving runtime status and outcome
  diagnostics
- Civilian town support plus undercover eligibility, request/application,
  detection, police/roadblock scan, compromise, and clear-state enforcement
- Commander-facing no-admin actions for initial HQ selection, random mission
  start, objective/runtime inspection, FIA supply requests, support
  cancellation, civilian aid, support reports, garage/build reports,
  vehicle-cargo reports, loadout-editor status/application, generated content
  reports, persistence status, HQ threat reports, marker audits,
  balance/pacing reports, campaign-end reports, and undercover status/checks
- Phase 0/1 foundation and mission diagnostics are complete: the alpha command
  menu exposes foundation status, explicit checkpoint reporting, active mission
  inspection, selected mission inspection, and null-safe runtime mission fields
- Phase 2 convoy runtime diagnostics are complete: the Missions tab exposes a
  convoy runtime report with vehicle assets, active convoy groups, crew counts,
  travel distance, fallback mode, and explicit movement/spawn failure reasons
- Broad-alpha UI/report polish exposes marker status/detail/audit reports,
  command coverage smoke paths, clear failed-action text, HQ threat/Defend
  Petros reports, enemy order and physical-response reports, support
  ETA/status reports, balance/pacing diagnostics, and campaign-end summaries
- Persistent campaign win/loss state with schema-25 end reason, summary,
  elapsed time, strategic control, war level, FIA/enemy zone counts, and report
  generation state
- Versioned campaign save-data container that is migrated, tracked through
  `PersistenceSystem`, and flushed before native `SaveGameManager`
  checkpoint requests when saving is possible, with a profile JSON fallback at
  `$profile:h-istasi/HST_CampaignSaveData.json`

This is a broad-alpha campaign foundation, not a public alpha. Petros and the
HQ arsenal have live contextual-action prefabs for the alpha menu path, while
the arsenal, garage, and loadout editor remain custom h-istasi economy systems
instead of stock/MSAR arsenal behavior. The campaign loop is now connected far
enough to track HQ exposure, queue Defend Petros pressure, resolve support and
enemy orders physically or abstractly, enforce undercover state, publish marker
coverage, and persist won/lost campaign outcomes. The systems are still rough:
cache/tent polish, save/restart soak testing, final surveyed Everon
coordinates, richer AI waypoints, full loadout-editor HST_Dev smoke, garage
progression polish, balance tuning, and mission-specific interactable props
still need to be connected incrementally.

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
campaign overview, markers and marker audits, economy, balance/pacing,
campaign-end state, zones, missions, mission runtime, manual checkpoint,
persistence status, income tick, training, first HQ selection, HQ movement, HQ
threat, Defend Petros, FIA recruitment, mission start, random mission
selection, objective progress, support requests/cancellation and ETA/status,
enemy order/physical-response reports, civilian aid, undercover
eligibility/request/check/clear flows, zone capture/activation, arsenal
reporting, vehicle cargo, garage capture/redeploy, nearby loot collection,
loadout editor status/application, generated content reports, HQ asset rebuilds,
command coverage and failed-action smoke reports, roster admin helpers,
campaign reset, and small debug resource awards.
Multiplayer clients use a player-owned request/RPC component;
the server resolves the caller from ownership instead of trusting a client
provided player ID. Petros opens this same menu path through contextual
interactions, and the visible HQ arsenal is backed by the same server commands
from the Arsenal/Loot tab.

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

See `docs/ARCHITECTURE.md`, `docs/PARITY.md`, and `docs/PHASE_PLAN.md` for
the implementation map and current phase roadmap.
