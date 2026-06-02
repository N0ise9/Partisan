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
model is versioned from day one. A later increment must add a native scripted
serializer for `HST_CampaignState` and migration tests before save
compatibility is promised.

## World Layout

`Worlds/HST_Everon/HST_Everon.ent` is an original subscene over vanilla
Everon. Named layer files reserve ownership boundaries for physical authoring.
Strategic IDs live in `Configs/HST/Maps/HST_Everon.conf` so persistence does
not depend on fragile entity names.

The checked-in world shells include a base-backed AI world with explicit Eden
soldier and vehicle navmesh configs, a perception manager, faction, loadout,
radio, and chat managers so Workbench can initialize and play-test the plain
game mode without relying on Conflict's strategic brain.

Direct `.ent` Play mode currently uses a temporary RHS-backed spawn harness:
automatic player respawn is enabled, the spawn menu forces `PLAYERS`, and
`PLAYERS` resolves to `RHS_USAF` so stock Plain deployment can create and
possess a test character. `StartingPoints.layer` contains bootstrap deploy
anchors affiliated with that same faction. This is not the campaign identity:
the h-istasi preset, state, HQ service, and strategic ownership still treat FIA
as the resistance. The deploy anchors use Scenario Framework spawnpoint slots
that instantiate the stock editable USMC spawnpoint prefab; the older Conflict
military-base spawn groups remain campaign/HQ metadata only. Game Master-spawned
characters do not satisfy the respawn system and are not expected to close the
deployment menu. The HQ spawn increment will replace this harness with a custom
FIA hideout deployment flow. Workbench offline play may still log blank identity
ID errors from stock reconnect/editable-entity systems. Treat those as
non-blocking Workbench noise if a character is spawned and possessed.

`HST_HQService` owns the server-side HQ lifecycle: initial hideout selection,
HQ movement between authored hideouts, Petros position, and Petros-loss
penalties. The current development bootstrap auto-selects the central hills
hideout so the campaign enters a playable active phase immediately; the setup
UI increment will replace that auto-selection with a player-facing choice.

## Antistasi Framework Spine

The first campaign loop is intentionally abstract. Zones carry type, income,
support, and garrison-slot data in `HST_CampaignState`; garrisons are stored as
infantry and vehicle counts until the hybrid AI activation increment turns them
into physical units near players. Mission success, failure, and timeout paths
mutate economy and aggression state. Coordinator dev actions expose deterministic
server-only hooks for Workbench tests: register a player, move HQ, capture a
zone, complete or fail a mission, tick income, train troops, recruit a garrison,
and fold survivors back into abstract state.
