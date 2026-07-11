# h-istasi

h-istasi is an original Arma Reforger resistance-campaign implementation. The
first preset targets Everon with a three-way war:

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
  saved-loadout, issued-item, mission-runtime, active-group, support, canonical
  exact-QRF operation, build-mode, campaign-end, and persistence metadata
- Server-authoritative campaign, economy, mission, persistence, arsenal, loot,
  loadout-editor, garage/build, and native-checkpoint services
- A 39-entry configured mission-registry baseline
- Custom FIA HQ player spawn path for direct Workbench Play mode
- FIA spawnpoint/loadout authoring metadata for authored hideout candidates
- Setup-phase HQ selection, HQ movement, and Petros state hooks for the first
  playable resistance-campaign slice
- Abstract campaign systems for player lifecycle, town income/support, zone
  capture, garrisons, recruitment/training, and mission rewards/penalties
- Native respawn-request bootstrap with pending spawn tracking to avoid
  Workbench duplicate-spawn loops
- Everon alpha anchors, generated-route response movement, routed infantry
  move/sweep waypoints, mixed response vehicle spawning, and route-aware zone
  activation scaffolding for the physical AI war
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
- Procedural h-istasi `I` key HQ menu mounted on both development
  worlds with Setup, Overview, HQ/Petros, Missions, Map/War, Forces,
  Arsenal/Loot, Garage/Build, Members, and Admin tabs
- First-load server settings generation at `$profile:h-istasi/HST_Settings.json`
  with config-backed defaults for campaign, factions, economy, membership,
  world activation, arsenal/loot, persistence, logging, and feature toggles,
  including configurable infinite stamina, live resistance support group map
  tracking, and population-based campaign outcomes
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
- Physical enemy response fold-back for support/QRF groups that leave the event
  bubble, preserving survivor state while removing live runtime handles
- Mixed personnel/vehicle active groups now use living personnel—not an intact
  empty vehicle—as combat-effectiveness authority. Once previously observed
  personnel reach zero outside population grace, the group becomes terminal,
  contributes no capture pressure, retires its linked QRF marker, and preserves
  an intact attached vehicle as neutral salvage. Unadopted vehicles are session-
  only; an existing durable field record and cargo remain durable. Terminal zero-survivor
  truth is retained across save normalization; the bounded lifecycle proof is
  implemented, while real combat/salvage/restart observation remains open.
- Physical support route truth that keeps spawned support state on the live
  member centroid, treats ETA as the earliest arrival observation rather than
  completion, requires two live samples from distinct elapsed seconds within
  75m for arrival or
  recall exit, and rebuilds direct current-to-target/exit waypoint chains with
  at most three consecutive reissues without an 8m new-best distance
  improvement. Pre-repair restored arrival/exit rows receive one current live-
  distance revalidation. Fresh
  packaged movement, arrival, and recall proof remains open.
- Town influence event ledger with support, reputation, heat, population, and
  security-pressure deltas feeding political ownership flips and debug reports
- Runtime and garage vehicle heat/report state for undercover vehicle cover,
  with passenger compromise counts, report expiry, capture/redeploy handoff,
  save-data preservation, and one-button debug proof
- Request-driven force composition for support, mission, garrison, and debug probes, with
  serializable intent, tier, cost, manpower, vehicle-plan, skipped-prefab, and
  failure metadata retained on support, enemy-order, and active-group records
- A schema-51 force/operation authority boundary with durable per-projection results,
  explicit force/projection identity on active groups, exact required-slot
  admission, bounded priority/FIFO scheduling and retention, retry/deadline/
  cancellation handling, dependency-ordered cleanup, Game Master registration
  evidence, once-per-restore reconciliation, and canonical operation records
  with persistent strategic projection for confirmed exact paid infantry QRFs
  and newly planned exact enemy defensive QRFs
- Bounded accepted-settlement archives that replace eligible full quote,
  manifest, and linked ledger rows only after terminal aggregate proof and all
  physical/queue backlinks are gone. Compact rows preserve issue, confirmation,
  and committed-ledger replay for at least 86,400 campaign seconds; new planning
  fails closed when the shared history bound cannot safely make room.
- An engine-facing force-spawn adapter driven by the production coordinator once
  per active-campaign second. Its first exact slice creates one infantry group
  root plus every frozen member slot, records a durable nonterminal
  `READY_FOR_HANDOFF` boundary, finalizes the projection in physical war before
  recording queue success, and prevents older group-population paths from
  duplicating queue-owned projections
- The first exact player-paid support slice: selecting a QRF map target issues
  an immutable two-step server quote, confirmation charges a flat $250 plus one
  HR for every authored catalog member through linked ledger transactions, and
  the accepted one-group manifest is submitted unchanged to SpawnQueue. Failure
  and pre-success cancellation refund both transactions once; recall settles
  eligible HR through the same ledger.
- Support recall now returns a typed acceptance, mutation, terminal,
  disposition, request, and operation result. It is the first production
  visible command whose durable receipt status comes from that result instead
  of presentation wording. Exact full-refund settlement prevalidates both
  linked money and HR transaction identity, state, and deterministic settlement
  replay before refunding either transaction.
- Schema 50 advances each newly confirmed exact paid infantry QRF's versioned
  `OperationRecord`: immutable origin and assignment, mutable tactical target,
  typed duty/engagement/materialization/position/settlement state, stable
  execution links, revision, terminal settlement identity, direct-route cursor,
  strategic position, projection-decision evidence, and deterministic virtual-
  combat clocks. The first consumer can travel while virtual, materialize near a
  living player, fold clear of contact outside a larger radius, preserve exact
  survivor slots, and resolve bounded infantry-only combat against its target's
  abstract garrison. Physical combat contact, vehicles/assets, other operation
  families, and packaged runtime/restart proof remain open.
- Schema 51 makes newly planned infantry-only enemy defensive QRF the first
  exact enemy-order consumer. One distinct same-faction operational source, a
  same-faction defended target under resistance pressure, prepaid frozen roster,
  order, operation, held batch, and active group stay linked through
  strategic outbound travel, proximity materialization/fold, exact casualties,
  once-only defensive pressure, return to origin, and survivor-proportional
  attack/support settlement. Six `enemy_qrf.*` fixtures cover the source
  contract. Existing orders and every other order family remain legacy, and
  packaged movement/return/restart proof is still open.
- The first exact force-runtime lifecycle slice: handed-off member slots retain
  durable ever-alive/casualty evidence, confirmed dead members detach from the
  native and Game Master group without deleting their corpses, the last death
  retires the exact root/marker, and restart reprojects only durable survivors.
- Request-driven spawn placement for physical support and debug probes, with
  road/dry-ground/vehicle-safe validation, player/active-AI clearance checks,
  and visible placement failure reasons
- Enemy support-spend ledgers for QRF/support damage pressure, same-zone
  cooldowns, max defense spend caps, survivor refunds, reportable denial
  reasons, and separated proactive attack versus reactive support spending.
  The exact enemy defensive QRF adds a deterministic full admission-failure or
  survivor-proportional post-commit settlement identity without changing other
  order refund policies.
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
- A packaged schema-49 check verified that the restored config metadata brings
  back normal stock HUD and Game Master access. That run also exposed separate
  marker and ambience defects: invalid radio icon entries rendered as giant
  boxes, zone markers lacked location names, the map pointer fell below a
  confirmation dialog, nearby civilian projection was coupled to hostile-zone
  activation, town actors repeated one appearance, traffic defaulted too low,
  drivers held the horn, and radio sites could receive a duplicate tower.
  Later source contains focused repairs, but they still require a
  republished runtime check.
- HQ proximity policy now keeps the 900m radius for hostile operation staging,
  while whole-location activation is excluded only inside that location's
  capture footprint (with a 150m legacy fallback) and composition clearance is
  limited to 150m. This source-only split prevents HQ safety from erasing nearby
  towns and bases while still keeping hostile spawns away from HQ.
- Admin-only campaign debug controls expose one-button smoke, physical, and
  full runtime verification profiles with status, cancel, cleanup, structured
  result artifacts, and bootstrap identity/admin evidence
- Persistent campaign win/loss state with schema-42 end reason, summary,
  elapsed time, strategic control, war level, FIA/enemy zone counts,
  population/support, airfield, outcome-mode, support deployment proof, and
  report generation metadata
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
still need to be connected incrementally. The current exact adapter
supports exactly one infantry group root and its exact member slots; vehicle,
asset, and multi-root manifests fail closed as unsupported. Player-paid QRF is
the first support type migrated to this path and, in schema 50, the first force
with persistent strategic movement, materialization hysteresis, exact virtual
roster authority, and bounded virtual combat. Campaign schema 51 adds the first
enemy consumer: newly planned infantry-only defensive QRF orders use one frozen
prepaid manifest, held strategic projection, live-position materialization,
return-to-origin duty, exact survivor-proportional settlement, and one operation
marker. Existing enemy orders and legacy QRF rows remain contract version zero;
counterattacks, patrols, roadblocks, support calls, vehicles, garrisons, mission
forces, and other support types remain on their prior state paths.
Current garrison purchase manifests contain purchase provenance rather than an
executable group root, so they remain intentionally nondeployable. Paid-QRF
restore clears process-local IDs and creates one held virtual batch with the
same confirmed casualty tombstones and durable survivor slots. Entering the
materialize-in radius then releases one new root plus only those survivors.
Exact enemy defensive-QRF restore uses the same held-survivor normalization,
clears process-local physical-arrival samples, and retains its immutable route
and prepaid resource-settlement authority. For the player QRF, initial deployment
failure still refunds money and HR; a technical reprojection failure retains the
already-delivered money cost and refunds surviving HR only.
General lifecycle authority beyond these two exact infantry-QRF consumers
remains open. Normal spawn
acquisition runs once per active-campaign second. During setup or after a won/
lost outcome, the coordinator cancels every nonterminal batch and drains its
cleanup with a monotonic runtime-only clock without advancing campaign elapsed
time. A batch whose exact slots are all verified remains durably
`READY_FOR_HANDOFF` until physical-war finalization succeeds; only then does the
queue record `SUCCEEDED`. Restoring pending, ready, or previously physical state
clears process-local evidence and returns the operation to the single held
virtual projection instead of treating interrupted handoff as success or
respawning immediately. The focused HST_Dev proofs are implemented, but they are
not runtime evidence until a fresh isolated run executes them.

## Current Delivery Priorities

The implementation blueprint's Campaign Runtime Integrity sequence controls
current work. Feature breadth already exists; the immediate goal is to make its
authority, runtime projection, persistence, and client evidence trustworthy:

1. Publish and test the current schema-51 build, including the schema-50 marker,
   map-dialog, radio-site, and civilian repairs. Require valid-sized marker
   icons, `Location | Owner` labels, a
   visible pointer over the support confirmation dialog, one authored radio
   tower per existing site, and correct destroy-target binding. The already
   restored stock HUD and Game Master access must remain intact.
2. Prove civilian projection in a fresh packaged run: zero unregistered group-
   member RPCs, varied concrete appearances, five driven vehicles per true
   town, two pedestrians at the known minor woodland locality, no stuck horns,
   and actual distance-over-time movement. Figari and Morton must project
   civilians and eligible military garrisons near a player; the separate
   900 m hostile-operation staging clearance must not erase either location.
3. Execute the isolated `HST_Dev` completion/cancellation/restart boundary and
   replace the historical Full Campaign Debug artifact with corrected evidence.
4. Runtime-prove exact paid-QRF creation and the schema-50 operation path:
   strategic travel from the direct-route cursor, bounded catch-up, proximity
   materialization/fold hysteresis, exact casualty/survivor transfer, on-station
   virtual combat, recall/settlement, archive replay, and once-only accounting
   across save/restart.
5. Runtime-prove the schema-51 exact infantry-only enemy defensive QRF: one
   prepaid frozen roster, no duplicate legacy response, strategic outbound and
   return travel, materialization/fold hysteresis, exact casualties, two-sample
   physical arrival, once-only defensive pressure, proportional survivor
   settlement, marker movement/cleanup, and save/restart replay.
6. Runtime-prove the convoy pre-seat vehicle registration and authority-local
   pilot entry repair with 3/3 seated living drivers, then prove the repaired
   support route-truth boundary through actual movement, two-sample arrival,
   recall exit, and bounded transactional waypoint reissue in scoped disposable
   profiles. In the same disposable boundary, reproduce the crewless mixed-QRF
   case and prove one terminal transition, zero capture/marker pressure, one
   neutral salvage detachment, no duplicate vehicle, and no survivor
   resurrection after restart.
7. Prove static marker widget readiness plus authoritative host/client/late-join
   projection before treating marker publication as player-visible truth.
8. Resume deeper mission, civilian, undercover, commander, logistics, and
   balance expansion only after those integrity gates produce reliable evidence.

The schema-50 player exact-QRF projection slice and schema-51 enemy defensive-
QRF slice are implemented in current source for two explicit infantry-only
consumers. Their virtual routes use a conservative direct
campaign cursor, materialization uses separate in/out distances, and the frozen
manifest slots remain the living/dead roster across fold and restore. Player
virtual combat is deliberately limited to its on-station infantry QRF against
abstract target-garrison infantry. The enemy defensive policy instead applies
bounded capture-pressure relief once, returns to origin, and settles surviving
resources. This does not complete the broader operations milestone: live
physical combat authority, terrain and ammunition effects, vehicles/assets,
broad legacy supports, garrisons, missions, other enemy orders, and client/JIP
projection remain future slices.

The last published schema-49 server/client check is runtime evidence that stock
HUD, Game Master, map markers, and civilian traffic initialize again. It is also
evidence for the newly reported marker, cursor, radio, classification,
appearance, horn, and military/civilian activation defects—not proof of the
schema-50 repairs. The final stamped schema-51 tree passes foundation validation
and a headless Workbench Game-module compile/create pass at 5,749 files/11,516
classes with CRC `85ccf2e0`. The prior schema-50 WorldEditor survival check is historical
startup evidence, not proof of the schema-51 edits. A republished server/client
test is still required before those repairs or either exact-QRF projection path
is called runtime-proven. The late-admin
role-change guard, campaign-debug
isolation, convoy seating, physical support movement/arrival/recall, and restart
boundaries remain open as separate gates.

For documentation authority, `docs/FEATURE_CHECKLIST.md` owns current feature
status and next tasks; `docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md` owns the
latest runtime evidence; `docs/MIGRATIONS.md` owns save-schema history; and the
top CRI sections of `docs/PHASE_PLAN.md` own delivery order. The numbered phase
details remain acceptance/history material and do not override those current
sections.

## Alpha Command Menu

Press `I` in `HST_Dev` or `HST_Everon` to open the h-istasi alpha command
menu. The menu is a client widget driven by server-built snapshots and renders
a campaign HQ interface with a resource bar, navigation, campaign
cards, action list, and activity/result feed. The Setup tab displays the
effective server config and lets the commander choose the first HQ hideout
before the campaign enters the active phase. `$profile:h-istasi/HST_Settings.json`
remains the source of truth for defaults that apply to newly created
campaigns, but initial HQ placement is selected through the setup map flow and
the generated runtime settings no longer expose a default hideout id. The file
uses JSON-safe `_comment` fields to explain settings because raw JSON comments
are not supported. The same settings file also controls h-istasi's Game Master budget
policy through `features.gameMasterBudgetsEnabled`, which defaults to `false`
so GM placement budgets are disabled automatically on game start.
`features.infiniteStaminaEnabled` defaults to `true`; when enabled, local
player stamina is refilled through the native stamina component and the sprint
exhaustion vignette/blur effect is suppressed.
`features.trackResistanceSupportGroupsOnMap` defaults to `true`; when enabled,
spawned player-requested resistance support groups keep a live map marker until
the group is killed, folded, or despawned.

During initial HQ placement the setup map owns keyboard input; complete HQ
placement first, then `I` opens the command menu and server snapshots add the
Admin tab for configured SteamID64 admins.

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
campaign reset, one-button campaign debug verification, and small debug
resource awards. Commander support and garrison call-ins use the normal in-game
map for target selection and are disabled when the commander has no map gadget.
Garrison and player-QRF targeting issue exact server quotes; the Forces tab then
shows the frozen count/cost and confirms or cancels only the actor-owned quote
ID through a confirmation dialog.
Multiplayer clients use a player-owned request/RPC component;
the server resolves the caller from ownership instead of trusting a client
provided player ID. Petros opens this same menu path through contextual
interactions, and the visible HQ arsenal is backed by the same server commands
from the Arsenal/Loot tab.

## Admin and Runtime Debug

To grant h-istasi admin rights on a local or dedicated server, add the player's
raw 17-digit SteamID64 to `membership.adminIdentityIds` in
`$profile:h-istasi/HST_Settings.json`. Backend UUIDs, BattleEye GUIDs,
`workbench_player_N` aliases, and per-session player IDs are not durable
h-istasi admin tokens.

The Admin tab exposes `Run Full Campaign Debug` as a one-button diagnostic
harness. It is not runtime certification by itself. In-process debug profiles
are restricted to `HST_Dev`, run against a cloned campaign state, suspend real
campaign checkpoints, and restore the untouched live state on completion or
cancel. A development-session restart is still required because world entities,
player state, and service caches are outside campaign save data. External,
restart, and soak profiles require a separately managed disposable profile and
launcher instead of starting through the in-process runner. Results are written under
`$profile:h-istasi/debug` as structured JSON, summary text, and state-diff text.
A valid admin grant should be visible in logs as a
`settings SteamID64` runtime grant, and command-menu input diagnostics log the
local `I` key/action path when troubleshooting menu access.

For dedicated server tests, repack/publish the Workbench addon before launching
the dedicated server. Server boot, admin diagnostics, command-menu readiness,
and structured debug artifacts must report the same runtime identity from
`HST_BuildInfo`: full commit SHA, UTC build time, label, campaign schema 51, and
runtime-settings schema. Missing or mismatched identity means the packaged
server/client runtime is stale or mixed, even if the repository is newer.

## Design Rules

- Prefix addon-owned scripts and resources with `HST_`.
- Keep the strategic campaign server-authoritative.
- Use native Reforger persistence and session saves.
- Keep off-screen forces abstract and activate physical AI only around
  players and active objectives.
- Track feature-complete campaign work in `docs/FEATURE_CHECKLIST.md` and keep
  planning docs focused on h-istasi behavior, status, gaps, and acceptance
  tests.
- Do not copy source assets from neighboring addons.
- Do not add hidden dependencies.

## Development Scenarios

- `Missions/HST_Everon.conf`: full-island campaign shell
- `Missions/HST_Dev.conf`: compact systems-development shell

See `docs/ARCHITECTURE.md`, `docs/FEATURE_CHECKLIST.md`,
`docs/HST_CAMPAIGN_DEBUG_VERIFICATION_AUDIT.md`, `docs/PARITY.md`, and
`docs/PHASE_PLAN.md` for the implementation map, feature checklist, runtime
verification audit, and current phase roadmap.
