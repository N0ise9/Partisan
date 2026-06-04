# h-istasi Architecture

## Runtime Ownership

`HST_CampaignCoordinatorComponent` is the server-side entry point. It owns a
single `HST_CampaignState` and delegates to small services:

- `HST_EconomyService`: HR, faction money, support, aggression, and war level.
- `HST_MissionService`: mission eligibility, activation, deadlines, and
  completion rewards.
- `HST_PersistenceService`: native Reforger checkpoint requests and autosave
  debouncing.
- `HST_AuthorizationService`: persistent members, guests, admins, and the
  first commander-vacancy policy.
- `HST_StrategicService`: ownership changes, town support, Petros penalties,
  activation flags, and victory evaluation.
- `HST_ArsenalService`: item counts, unlock thresholds, and abstract garage
  records.
- `HST_EnemyDirectorService`: validated spending against separate attack and
  support pools.
- `HST_PlayerLifecycleService`: connected-player registration, deterministic
  Workbench identity fallback, personal money, and rank.
- `HST_TownService`: resistance income, HR income, and town support changes.
- `HST_GarrisonService`: abstract garrison creation and survivor fold-back.
- `HST_RecruitmentService`: troop training and abstract garrison recruitment.
- `HST_ZoneCaptureService`: capture helpers around strategic ownership
  changes.
- `HST_PlayerSpawnService`: custom FIA HQ spawn, Workbench identity fallback,
  native respawn requests, pending spawn tracking, and spawned-player records.
- `HST_PhysicalWarService`: player-proximity zone activation over the
  abstract garrison model.

The coordinator currently exposes server-only mutation methods that check
campaign phase, known IDs, and mission eligibility before changing state.
Client RPCs are intentionally withheld until player-owned request components
can authenticate the caller identity and enforce member, commander, and admin
permissions without trusting client-provided IDs.

## Authoring Contracts

The addon keeps authoring data separate from runtime state:

- `HST_CampaignPreset`: fixed scenario roles and capability switches.
- `HST_FactionTemplate`: faction identity and capability declarations.
- `HST_MapDefinition`: stable IDs for zones and hideout candidates.
- `HST_BalanceConfig`: Community Edition-derived initial values.
- `HST_MissionRegistryConfig`: mission definitions and deferred capabilities.

The checked-in `.conf` resources are the intended data source. The current
coordinator uses matching defaults while the resource loading and serializer
prefabs are connected in Workbench.

## Persistence

The persistence service uses `SaveGameManager.RequestSavePoint` so local hosts
and dedicated servers follow Reforger's native session-save path. The state
model is versioned from day one. `HST_CampaignSaveData` is the deep-copy save
container for current campaign fields and nested runtime arrays. A later
Workbench pass must bind that container into Reforger's persistent component
loading path and add migration tests before save compatibility is promised.

## World Layout

`Worlds/HST_Everon/HST_Everon.ent` is an original subscene over vanilla
Everon. Named layer files reserve ownership boundaries for physical authoring.
Strategic IDs live in `Configs/HST/Maps/HST_Everon.conf` so persistence does
not depend on fragile entity names.

The checked-in world shells include a base-backed AI world with explicit Eden
soldier and vehicle navmesh configs, a perception manager, faction, loadout,
radio, chat, and respawn managers so Workbench can initialize without relying
on Conflict's strategic brain.

Direct `.ent` Play mode no longer depends on the stock Deployment Setup menu.
The coordinator is an `SCR_BaseGameModeComponent`, so it receives game-mode
state and player-connected callbacks directly. It also runs a short frame sweep
for connected players without controlled pawns, which covers Workbench timing
where player `1` exists before its respawn component is ready. The respawn
system remains as possession plumbing, but its spawn logic is
`HST_PlayerSpawnLogic`, which delegates to `HST_PlayerSpawnService`. The
service registers the connected player as FIA, submits an `SCR_FreeSpawnData`
request for the default FIA rifleman at the selected HQ hideout, tracks the
request as pending, and records player state only when the native spawn
callback reports success. This prevents the Workbench frame sweep from
creating duplicate bodies while Reforger is still finalizing ownership.

`StartingPoints.layer` still contains FIA-affiliated Scenario Framework
spawnpoint slots and FIA role-selection loadouts, but those are now authoring
metadata and a debug fallback, not the normal player-side path. RHS_USAF
remains the occupier in the strategic preset. Game Master-spawned characters
are still not expected to advance h-istasi's player lifecycle. Workbench
offline play may log blank identity ID errors from stock reconnect or
editable-entity systems; treat those as non-blocking if a character is spawned
and possessed.

`HST_HQService` owns the server-side HQ lifecycle: initial hideout selection,
HQ movement between authored hideouts, Petros/cache/arsenal/tent runtime
positions, and Petros-loss penalties. Runtime Petros spawning tries the custom
h-istasi prefab first through its GUID-qualified metadata resource and falls
back to the base FIA character only if that resource cannot spawn. The HQ
arsenal uses a GUID-indexed HST supply-cache prefab whose contextual actions
open the same Arsenal/Loot menu path used by the I-key menu, with a stock FIA
cache fallback if the custom object cannot spawn.
The current development bootstrap auto-selects the central hills hideout so the
campaign enters a playable active phase immediately; the setup UI increment
will replace that auto-selection with a player-facing choice.

The alpha HQ menu is procedural rather than layout-resource loaded. The server
keeps the existing `HST_MENU`, `TAB`, `STATUS`, `RESULT`, and `ACTION` payload
lines while adding optional `STAT`, `SECTION`, `ROW`, and `FEED` lines for the
Antistasi-style overview, HQ/Petros, missions, map/war, forces, arsenal/loot,
members, and admin panels. Contextual Petros and HQ arsenal actions call the
same request bridge as menu clicks so local hosts and MP clients follow one
server-authoritative command path.

## Antistasi Framework Spine

The first campaign loop is intentionally abstract. Zones carry type, position,
income, support, activation radius, route IDs, mission site IDs, and
garrison-slot data in `HST_CampaignState`; garrisons are stored as infantry and
vehicle counts. The physical-war service marks zones active when players enter
their activation radius and mirrors abstract garrison counts into runtime active
counts. Follow-on AI work should consume those active counts to spawn groups,
then fold survivors back before deactivation. Mission success, failure, and
timeout paths mutate economy and aggression state. Coordinator dev actions
expose deterministic server-only hooks for Workbench tests: register a player,
move HQ, capture a zone, complete or fail a mission, tick income, train troops,
recruit a garrison, and fold survivors back into abstract state.
