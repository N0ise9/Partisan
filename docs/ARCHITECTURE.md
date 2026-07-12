# h-istasi Architecture

## Runtime Ownership

`HST_CampaignCoordinatorComponent` is the server-side entry point. It owns a
single `HST_CampaignState` and delegates to small services. The cross-cutting
schema-42 through schema-61 authority services are:

Schema 60 also separates recurring enforcement reads from identity refreshes:
the one-second maintenance path reuses registered player authority, while
connect/setup and explicit visible actions retain the full backend identity and
membership refresh boundary. This removes repeated authority resolution from
the hot tick without making cached presentation state an authorization source.

- `HST_StableIdService`: allocates persisted, monotonic IDs for generated
  commands and events, and builds deterministic operation/transaction links.
- `HST_CampaignCommandService`: validates typed command envelopes and keeps a
  bounded persisted receipt history so replaying the same request ID cannot
  apply the same mutation twice. `CompleteExplicit` records an authoritative
  applied/rejected status supplied by a migrated command result; `Complete`
  remains a presentation-text compatibility classifier for unmigrated visible
  commands.
- `HST_ResourceLedgerService`: records reserve, commit, cancel, and refund
  transitions for resource mutations. Troop training, exact garrison
  confirmation, and exact player-QRF confirmation/settlement consume it.
- `HST_CampaignEventLogService`: appends bounded persisted campaign events for
  authority decisions and resource transitions.
- `HST_ForceCatalogService`: owns the versioned exact group templates and
  validates their ordered execution-prefab slots.
- `HST_ForcePlanningService` and `HST_ForcePlanningIntegrityService`: issue and
  reconcile immutable garrison/player-QRF/player-Search-and-Destroy quotes and
  manifests plus exact enemy defensive-QRF manifests, own
  deterministic hashing/catalog validation shared by queue admission, and keep
  accepted confirmation replay idempotent across later settlement.
- `HST_ForceSettlementArchiveService`: compacts only terminal, backlink-free
  accepted planning aggregates into bounded persisted replay tombstones. It owns
  full-row retention, archive capacity, replay reconstruction, and fail-closed
  admission when protected history cannot make room. For canonical exact-QRF
  operations, compaction additionally requires a coherently settled record,
  copies its typed terminal/revision/settlement evidence, and removes the full
  record with the other accepted aggregate rows. Expired exact-support capacity
  eviction treats the tombstone and retained terminal request as one pair. Every
  positive typed player-support contract, including exact QRF and Search-and-
  Destroy, must pass valid replay shape, unique aggregate identity, no live
  backlink, and complete terminal-receipt reciprocity before both rows leave.
  Historical contract-0 QRF retains its minimal compatibility match. Malformed
  or quarantined typed pairs remain durable evidence while another eligible row
  may be pruned.
- `HST_OperationService`: owns the version-1 canonical operation transition
  contracts for confirmed exact paid player QRFs, confirmed exact player Search-
  and-Destroy, and newly admitted exact enemy defensive-QRF orders, and supplies
  shared deterministic settlement identity
  to the schema-52 mission-convoy owner. For the QRF consumers it preserves immutable
  origin/assignment separately from the mutable tactical target; validates
  duty, materialization, engagement, return, and settlement transitions; and
  rejects identity or terminal replay conflicts. Schema 50 added strategic-
  route, projection-decision, and virtual-combat authority for the player QRF.
  Schema 60 adds a separately typed Search-and-Destroy assignment/recall/
  settlement policy while reusing that exact infantry projection boundary.
  Schema 51 adds the reciprocal enemy-order contract, two-sample physical
  arrival authority, return-to-origin duty, and typed `COMPLETED` settlement for
  the enemy defensive QRF without opting legacy rows or other order families in.
- `HST_StrategicMovementService`: owns the schema-50 direct-route cursor and
  conservative 2.5 m/s campaign movement shared by the two exact infantry-QRF
  consumers and Schema-60 player Search-and-Destroy. It derives ETA from route
  distance, advances only
  while strategic position authority is active, and caps catch-up at 30
  campaign seconds per invocation.
- `HST_MaterializationService`: evaluates either supported exact infantry QRF
  or exact player Search-and-Destroy
  against a player-bubble materialize-in distance and a larger materialize-out
  distance. The hysteresis band prevents churn; an engaged physical projection
  cannot fold.
- `HST_VirtualCombatService`: resolves only an on-station exact paid player
  infantry QRF or Search-and-Destroy operation against hostile abstract infantry
  in its target garrison. It advances in
  deterministic 30-second power steps, processes at most four steps per tick,
  retires exact manifest member slots, and persists fractional damage carries.
- `HST_OperationProjectionProofService`: adds focused movement, hysteresis,
  roster-transfer, and combat/restore assertions beside the existing eight
  operation-record assertions. These are source-level debug fixtures until a
  packaged runtime executes them.
- `HST_EnemyQRFOperationService`: owns schema 51 admission and runtime authority
  for newly planned infantry-only enemy defensive QRFs. It selects a distinct
  same-faction operational source for a same-faction defended target under resistance pressure, links one prepaid
  frozen manifest to one order, operation, held SpawnQueue batch, and active
  group; advances virtual/physical outbound and return legs; applies defensive
  capture-pressure reduction once on arrival; and settles survivor-proportional
  enemy resources once after return. Missing or conflicting authority fails
  closed instead of falling back to the legacy timer/support path.
- `HST_EnemyQRFOperationProofService`: contributes six focused
  `enemy_qrf.*` debug assertions for atomic admission, legacy isolation,
  projection transfer, survivor settlement, current-schema restore, and
  rejection/refund behavior. They are deterministic in-process fixtures, not
  packaged-runtime evidence.
- `HST_MissionConvoyOperationService`: owns schema 52 admission and runtime
  authority for newly started convoy missions. It freezes one persisted
  generated road route, exactly three vehicle slots, exactly three linked crew
  groups and their member slots, plus mission-kind-compatible cargo/captive
  authority assigned to vehicle zero. It advances the column virtually at 9 m/s,
  materializes
  within 1,800 m of a living player, folds beyond 2,200 m only when clear of
  contact, player occupancy, and unresolved interaction state, preserves exact
  crew casualties by frozen slot/entity identity and per-element vehicle state,
  confirms physical arrival
  from two distinct route-end samples, and settles only after the existing
  mission-convoy outcome owner has applied its once-only result. Historical
  restored convoy rows remain contract version `0` on the legacy timer/runtime
  path. Crew elimination can place unresolved money/prisoner/ammo/armored or
  other recoverable mission assets into an on-station recovery hold, keeping
  the exact operation open and reprojectable until the mission-specific outcome
  is actually resolved.
- `HST_MissionConvoyP1Policy`: supplies the small shared fail-closed policy
  surface for exact-convoy admission and restore. It enforces mission-specific
  cargo cardinality, role/kind, and loadable mission-asset entity requirements.
  Captives must be boardable characters with compartment access, while ordinary
  payloads must be non-character entities. The policy also owns
  the legal duty/resume and settlement/materialization/position pairs.
- `HST_MissionConvoySaveValidationService`: owns conservative schema-52
  migration, current-schema claimant validation, quarantine, derived-survivor
  normalization, and process-local restore cleanup. `HST_CampaignSaveData`
  delegates this boundary instead of carrying one compiler-heavy validator.
- `HST_MissionConvoyOperationProofService`: contributes nine focused
  `mission_convoy.*` assertions for atomic admission and rollback,
  virtual/projection and fold gating, casualty-stable restore, idempotent
  arrival/settlement, open/settled/recovery restore, aggregate-marker cleanup,
  and the materialization watchdog. Admission/corruption subfixtures reject
  invalid cargo, duplicate/hash/foreign authority, invalid seat topology,
  forged arrival receipts, illegal lifecycle pairs, and casualty authority on
  non-member roots, and preserve missionless exact-looking durable claimants.
  They are deterministic in-process fixtures, not packaged movement, rendering,
  or restart evidence.
- `HST_EnemyPatrolOperationService` and
  `HST_EnemyPatrolSaveValidationService`: own schema-53 exact authority for only
  newly queued enemy patrols, including proactive debit/settlement, generated-
  route outbound/lap/return state, exact projection transfer, legacy dispatch
  isolation, and `-53` quarantine.
- `HST_GarrisonPatrolOperationService`: owns schema-54 authority only for newly
  issued policy-v2 purchased resistance garrisons. It admits one exact arbitrary
  member roster under a non-self-populating group root, holds it as a virtual
  infinite local patrol, transfers only living slots through materialization/
  fold, and settles owner-change/all-dead/campaign-stop/setup or typed spawn/
  route-failure outcomes once without a refund. It never claims historical
  policy-v1, initial-map, enemy aggregate,
  vehicle, or multi-root garrisons.
- `HST_GarrisonPatrolSaveValidationService`: preserves every pre-schema-54
  garrison on its legacy representation, normalizes coherent current exact
  patrols to held survivor authority, and retains malformed current graphs under
  quarantine version `-54` without legacy conversion, refund, or guessed death.
- `HST_GarrisonPatrolOperationProofService`: contributes nine focused
  `garrison_patrol.*` assertions for admission, replay/rollback, roster
  projection, infinite route loop, projection/casualty hold, no-refund terminal
  settlement, restore, corruption quarantine, and marker lifecycle. They are
  deterministic source fixtures rather than packaged native behavior proof.
- `HST_MissionGuardOperationService`: owns schema-55 officer-guard, schema-56
  traitor-guard, and schema-57 spec-ops-guard authority. A newly started `assassinate_officer` mission uses
  contract `1`/policy `exact_assassinate_officer_guard_v1`; a newly started
  `assassinate_traitor` mission uses contract `2`/policy
  `exact_assassinate_traitor_guard_v1`; and a newly started
  `assassinate_specops` mission uses contract `3`/policy
  `exact_assassinate_specops_guard_v1` and intent `assassinate_specops_guard`.
  Generic spec-ops composition may propose multiple groups; contract `3`
  deterministically selects the strongest executable group, keeps the stable
  first group on ties, and freezes only that selected catalog roster. Discarded
  groups never become mission, operation, manifest, batch, or active-group
  authority.
  All three freeze one catalog-backed empty
  infantry root and ordered members, keep the HVT outside the manifest and
  operation asset graph, hold a route-less roster on station, transfer only
  durable survivors through materialization/fold, and map guard elimination or
  mission/owner/runtime outcomes to one zero-refund terminal receipt. Historical
  and pre-opt-in assassination missions and all unsupported families remain
  contract `0`.
- `HST_AssassinationGuardSaveValidationService`: preserves pre-opt-in mission and
  HVT history at contract `0`, validates either supported exact mission/
  operation/manifest/batch/group graph, accepts compact settled authority,
  normalizes coherent physical state to held survivors, and retains malformed
  officer/traitor/spec-ops rows under `-55`/`-56`/`-57` quarantine without legacy fallback,
  guessed casualty, HVT ownership, or refund. Schema-56 migration creates no
  traitor authority and records `migration_schema56_exact_traitor_guard`;
  current traitor conflicts record
  `normalization_schema56_exact_traitor_guard_conflict`. Quarantine remains
  diagnostic and does not terminate the otherwise playable HVT mission.
  Schema-57 migration likewise creates no spec-ops authority and records
  `migration_schema57_exact_specops_guard`; current spec-ops conflicts record
  `normalization_schema57_exact_specops_guard_conflict`. Ordinary
  `mission_group_*` rows are not exact claimants.
- `HST_MissionGuardOperationProofService`: contributes focused
  `mission_guard.*` source assertions for admission/legacy isolation, survivor
  projection, HVT-independent settlement, migration/restore, corruption
  quarantine, and existing-HVT marker/UI status. They are not native entity,
  adapter, save/restart, rendered UI, owner-change, campaign-setup, or packaged
  multiplayer evidence.
- `HST_TraitorGuardOperationProofService`: contributes the same six focused
  source-proof categories for contract-2 traitor admission/isolation, survivor
  projection and HVT separation, typed settlement, restore/migration,
  corruption quarantine, and existing-HVT marker/UI status. It also proves the
  contract-1 officer path coexists and pre-56 traitor/spec-ops rows remain
  contract `0`. Native entities, real adapter casualties, actual save/restart,
  rendered UI, owner-change, setup, packaged networking, reconnect, and JIP are
  explicitly unclaimed.
- `HST_SpecOpsGuardOperationProofService`: contributes the same six focused
  source-proof categories for contract-3 spec-ops admission/family isolation,
  survivor projection and HVT separation, typed zero-refund settlement,
  restore/pre-57 migration, `-57` corruption quarantine, and existing-HVT
  marker/UI status. It preserves officer contract `1`, traitor contract `2`,
  and historical spec-ops contract `0`. Schema 57 is stamped at implementation
  `514ebdcbeb1ddfb2a383b19590382517113e2ff6`; native/package/restart/render/
  network gates remain explicitly open.
- `HST_RescuePOWOperationService`: owns Schema-58 contract-1 authority only for
  newly started `rescue_pows`. One mission-rescue operation binds one frozen
  composite manifest, one hostile catalog guard roster, one external-asset
  SpawnQueue batch, and exactly three typed captive rows. It owns captive
  transition/idempotency through a bounded typed command ledger, stable escort/
  carrier/seat evidence, disconnect release before grace, unbound fold/re-entry,
  damage-only casualty receipts, one frozen HQ extraction anchor, base-deadline
  plus 300-second custody-only grace, and zero-resource typed settlement. It
  releases surviving custody links before non-success settlement so historical
  rows do not depend on mutable carrier vehicles. Guard elimination and captive
  outcome authority stay independent.
- `HST_RescuePOWExternalAssetPolicy`: is the sole permission boundary for
  external manifest asset rows. Queue admission/replay/batch validation requires
  the frozen hash-valid rescue policy/intent/kind, one deterministic guard root
  and ordered members, no vehicles, and exactly three deterministic captive
  slots. Adapter execution additionally requires a unique reciprocal active
  contract-1 mission-rescue operation, batch, group, and all three typed captive
  rows before it excludes those asset descriptors from generic work.
- `HST_RescuePOWExternalAssetPolicy`: is the shared fail-closed exemption gate
  for Schema-58 captive manifest descriptors. Queue admission, replay, and
  durable-batch validation require the exact frozen/hash-valid shape; adapter
  execution additionally requires a unique reciprocal active mission-rescue
  contract-1 graph and exactly three matching durable captive rows before the
  generic adapter may omit those descriptors. External rescue root registration
  preserves the rescue mode discriminator used by runtime and restore checks.
- `HST_RescuePOWSaveValidationService`: preserves historical POWs and refugees
  at contract `0`, validates reciprocal site/mission/operation/manifest/batch/
  group/captive authority, clears process bindings without erasing stable
  escort/carrier/seat receipts, accepts compact terminal graphs, and quarantines
  malformed current or strong pre-58 claimants at `-58`. It records
  `migration_schema58_exact_rescue_pows` and
  `normalization_schema58_exact_rescue_pows_conflict` without inventing a
  captive, casualty, extraction, reward, fallback, or force transfer.
- `HST_RescuePOWOperationProofService`: contributes six deterministic source
  categories for admission isolation, composite/external authority, captive
  transitions (including old accepted/rejected replay, request collisions,
  terminal reserved capacity, and deep-copy preservation), guard independence,
  hitch-stable outcome/grace and disconnect policy, and restore/quarantine. It
  does not claim native entities, natural combat, vehicle seats, real process
  restart, rendered UI, networking, reconnect, or JIP.
- `HST_RadioSiteLifecycleService`: owns the Schema-59 contract-1 radio-site
  aggregate for every configured radio zone. One stable site/zone/target
  identity owns transmitter binding, ONLINE/DESTROYED/REBUILDING lifecycle,
  the active exact destroy-or-stop-rebuild mission lock, typed transition
  request, revision, and destruction/rebuild receipts. A uniquely discovered
  supported authored transmitter is borrowed and never deleted. Its immutable
  authored prefab/position provenance remains separate from the mutable current
  projection descriptor across generated-ownership handoff; each exact mission
  asset snapshots that ownership and authored provenance at admission. Its
  retained multiphase damage object is the physical evidence source, and no initial
  generated fallback is invented when binding is absent or ambiguous. After a
  completed rebuild, the campaign owns one generated replacement while any
  authored resurrection is suppressed. Permanent generated ONLINE projections
  disable verbose witness logging and keep nearby-entity scans dormant until an
  exact mission identity is configured. The service alone creates or retires
  radio projections, so generic mission runtime and zone composition cannot
  place a second tower beside the canonical target. A new-campaign reset must
  reacquire and restore the authored transmitter before the state swap or reject
  the reset without abandoning the old campaign. Physical destroy/heal/rollback
  writes are verified. Authored identity uses a 0.75-meter tolerance, while
  physical projection evidence allows the bounded 12-meter safe-ground offset.
  A streamed-out borrowed target moves reciprocal mission/asset/runtime flags to
  an explicit pending phase without manufacturing destruction. Schema 60
  amortizes initial authored-world discovery to one unresolved site per campaign
  tick; the post-mission pass still reconciles every already-bound or campaign-
  generated projection immediately.
- `HST_RadioSiteSaveValidationService`: conservatively creates only logical
  ONLINE/unresolved sites for pre-59 saves, leaves historical terminal radio
  missions at contract `0`, fails active legacy radio claims closed, validates
  exact reciprocal mission/asset/site authority, distinct stable-site versus
  unique per-mission physical runtime identities, frozen physical bindings,
  persisted bounded demolition-evidence keys, mission-time ownership snapshots,
  and receipt/status/lifecycle evidence. Generated ONLINE state additionally
  requires destruction followed by completed-rebuild provenance. It quarantines
  contradictory current strong claimants at `-59` without inventing target
  binding, physical destruction, rebuild completion, receipts, or rewards.
  Current corrupt aggregates fail and clean up together; coherent already-
  terminal historical outcomes keep their terminal meaning.
- `HST_RadioSiteLifecycleProofService`: contributes six `radio_site.*`
  source assertions for binding/admission isolation, all lifecycle outcomes,
  replay/stale revision behavior, restore/migration/quarantine, influence
  suppression, and borrowed/generated ownership with construction-equipment
  targeting. The harness calls the production admission, durable evidence, and
  outcome transitions, rejects direct repeat rebuild admission, checks linked
  quarantine cleanup, and replaces only physical projection seams. Native
  authored discovery, natural explosive damage, damage-state reapplication,
  generated replacement,
  streaming re-entry, real save/restart, rendered UI, networking, reconnect,
  and JIP remain packaged-runtime gates.
- `HST_PlayerSearchDestroySaveValidationService`: owns Schema-60 restore
  isolation for player Search-and-Destroy. Pre-60 requests remain contract `0`
  with no inferred quote, manifest, ledger, operation, batch, group, casualty,
  or refund. A current contract-1 claimant must restore as one reciprocal exact
  graph; malformed strong claimants retain diagnostic evidence under `-60`,
  terminalize their runtime projection, and cannot fall through to legacy
  execution or alter balances by inference. Global active-group classification
  treats their quarantine status/mode as non-operational and not combat-present.
- `HST_PlayerSearchDestroyOperationProofService`: exercises the production quote
  and confirmation replay, infantry-only roster/cost, direct route, exact
  virtual-combat casualties, materialization/fold/re-entry, displaced-fold
  return to immutable assignment, commander recall/ledger settlement, legacy
  isolation, malformed-current quarantine, valid archive-pair capacity prune and
  restore, and corrupt quarantine retention. The service is compiled and wired
  into Campaign Debug, but its Schema-60 assertions have not been executed there.
  Their in-memory seams are not native movement/combat, packaged server, or real
  restart evidence.
- `HST_OperationRecordProofService` now also retains a typed exact-QRF archive
  pair whose receipt fields mismatch under forced capacity. That assertion is
  compiled and wired into Campaign Debug but has not been executed; contract-0
  QRF remains on its historical minimal-compatibility path.
- `HST_MaidensBayLocationSaveValidationService`: keeps
  `resource_logistics_warehouse` as the only enumerable strategic location,
  returns without mutation when neither location row exists, and fails closed
  before rewrites when canonical authority is ambiguous. With one authority it
  retires duplicate town civilian/influence/marker and ordinary aggregate-
  garrison state without fold-back credit, preserves the authoritative owner/
  economy, canonicalizes mutable generic references, and leaves all nonzero
  typed claimants frozen, including settled/quarantined/malformed and graphless
  exact rows. Mutable generated content rekeys; frozen site/route identity stays
  unchanged and receives a deep canonical clone. General lookup resolves the
  warehouse, while exact validators may use a detached old-ID/old-position view
  and runtime boundaries compare old/canonical IDs as equivalent. The retired
  town never becomes an income, marker, civilian, or new targeting source.
- `HST_MaidensBayLocationMigrationProofService`: builds isolated both-location
  and legacy-only saves, runs the production normalizer twice, and checks
  retirement, generic-reference rewrites, typed-graph isolation, ledger and
  generated-content results, idempotency, and compatibility lookup. It is
  compiled and wired to Campaign Debug assertion
  `location_taxonomy.maidens_bay_schema60`, but that assertion has not been
  runtime-executed and is not packaged save/restart evidence.
- `HST_MissionAssetComponent` publishes a minimal replicated rescue-action DTO
  (evaluated, exact contract, disposition, active, quarantine, revision).
  `HST_MissionCaptiveActionPolicy` fails closed while that classification is
  pending, then exposes only the legal action; unchanged values do not create a
  replication bump. The server command/transition owner remains authoritative.
- `HST_ForceSpawnQueueService`: owns schema-44 durable per-projection spawn
  batches, bounded priority/FIFO work acquisition, verified callbacks,
  retry/deadline/cancellation cleanup, pin-aware terminal retention, reporting,
  dependency-ordered cleanup, a durable nonterminal `READY_FOR_HANDOFF` state,
  explicit post-handoff completion, once-per-actual-restore reconciliation, and
  schema-50/51 strategic holds plus Schema-60 player Search-and-Destroy, the
  schema-52 convoy, schema-54 garrison-patrol, schema-55/56/57 mission-guard
  roster holds, and the executable guard subgraph of Schema-58 rescue. A rescue
  batch explicitly excludes its three
  externally managed captive asset slots from queue result rows only after the
  shared exact external-asset policy validates their complete frozen shape. A held
  batch performs no generic queue work and
  uses its frozen member slots as exact living/dead roster authority.
- `HST_ForceSpawnAdapterService`: consumes eligible released queue work from the production
  one-second active-campaign coordinator tick. The first engine-facing slice
  creates exactly one infantry `SCR_AIGroup` root plus all frozen member slots,
  returns exact prefab/liveness/faction/native-group/Game Master/projection
  evidence, finalizes ready projections in physical war, and only then asks the
  queue to record success. For successfully handed-off exact infantry, it also
  maps authoritative life state back to the exact member slot, detaches confirmed
  dead members without deleting their corpses, and drives last-death cleanup.
  Its current-infantry validation now uses durable living slots after first
  handoff, allowing a schema-54 partial garrison survivor roster to rematerialize
  without restoring or requiring the original purchased count.

The remaining domain services are:

- `HST_EconomyService`: HR, faction money, support, aggression, war level,
  strategic score/control percentage, pacing recommendations, victory readiness,
  and balance diagnostics.
- `HST_MissionService`: mission eligibility, activation, deadlines, and
  completion rewards.
- `HST_PersistenceService`: campaign save-data migration/tracking, native
  Reforger checkpoint requests, autosave debouncing, and profile JSON fallback
  saves. Every real capture first reconciles mapped physical exact-convoy and
  currently cut-over exact-infantry members. Exact player support additionally
  requires reciprocal ownership, exact living root/member binding cardinality,
  and a refreshed live position. Ambiguous or incomplete authority defers the
  capture and retains its checkpoint intent for bounded retry without flushing
  stale state or requesting a savepoint.
- `HST_PersistenceSmokeTestService`: deterministic persistence fixture seeding
  and restore verification reports.
- `HST_RuntimeSettingsService`: `$profile:h-istasi/HST_Settings.json`
  load/create/migration and settings application to preset/balance data.
- `HST_AuthorizationService`: persistent members, guests, admins, and the
  first commander-vacancy policy.
- `HST_StrategicService`: ownership changes, town support, Petros penalties,
  activation flags, population-first victory/loss evaluation, and durable
  campaign-end state/reporting.
- `HST_HQService`: initial HQ selection, HQ movement, Petros/cache/arsenal/tent/spawn-point
  runtime objects, rebuilds, Petros-loss state, HQ knowledge, HQ threat scans,
  and Defend Petros diagnostics.
- `HST_ArsenalService`: item counts, unlock thresholds, finite/INF withdrawals,
  garage records, and vehicle redeploy.
- `HST_LootService`: area loot, vehicle cargo, garage vehicle capture, field
  vehicle snapshot/restore, and safe vehicle-root scanning.
- `HST_LoadoutEditorService`: saved loadouts, live loadout/storage nodes,
  candidate replacement, finite/INF cost ledgers, atomic apply/rollback, and
  issued-item accounting.
- `HST_BuildModeService`: dry-ground placement resolution for garage redeploys
  and HQ runtime-asset rebuilds.
- `HST_ConvoyOutcomeService`: mission-specific convoy arrival, capture,
  delivery, ammo-point, garage-handoff, and outcome de-dupe effects.
- `HST_CommandUIService`: procedural command-menu payloads, visible action
  routing, reports, tab/action gating, marker/UI audit coverage, command/report
  coverage checks, and failed-action text.
- `HST_EnemyDirectorService`: scaled enemy attack/support resource income by
  owned zone value and war level, plus validated spending against separate
  attack and support pools.
- `HST_PlayerLifecycleService`: connected-player registration, deterministic
  Workbench identity fallback, personal money, and rank.
- `HST_TownService`: resistance income, HR income, and town support changes.
- `HST_GarrisonService`: legacy aggregate garrison creation/fold-back plus the
  schema-54 exact purchased-manifest backlink and living-slot capacity reader.
- `HST_RecruitmentService`: troop training and abstract garrison recruitment.
- `HST_ZoneCaptureService`: capture helpers around strategic ownership
  changes.
- `HST_PlayerSpawnService`: custom FIA HQ spawn, Workbench identity fallback,
  native respawn requests, pending spawn tracking, and spawned-player records.
- `HST_PhysicalWarService`: player-proximity zone activation over the abstract
  garrison model, plus exact adapter registration/handoff and guards that keep
  legacy population, repair, survivor, route, patrol, and cleanup paths from
  duplicating queue-owned projections. For spawned support groups it owns live-
  member-centroid route state, two distinct-time-sample arrival/recall
  confirmation within 75m, direct current-to-target/exit chains, bounded route reissue, and
  transactional replacement of service-owned waypoint entities. Exact QRF
  handoff normalizes into `support_active` so this same route authority owns it.
  For non-queue-managed mixed personnel/vehicle groups it owns a separate
  personnel terminal predicate: vehicle health cannot substitute for living
  infantry after prior population evidence and population grace. Terminal
  cleanup zeros strategic strength, fails an unresolved linked QRF, unregisters
  combat ownership, and releases an intact vehicle as neutral salvage. An
  existing durable field/loot/garage record remains durable; otherwise the
  salvage record is session-only. HQ safety uses separate scopes: 900m remains
  the hostile operation-staging exclusion, but whole-location activation is
  blocked only inside the location capture footprint (at least 150m for legacy
  records). Zone composition uses its own 150m immediate-clearance guard. This
  split is implemented in source and awaits packaged behavior proof. For a
  schema-52 exact mission convoy, PhysicalWar is also the sole engine projection
  adapter: it realizes the durable mobile crewed elements plus any separated
  intact crewless asset roots during outbound travel and recovery hold, samples current
  position/survivors/damage/fuel back into those elements, and folds owned
  vehicle/group handles without inventing casualties or applying a mission
  outcome. Materializing roots remain unpublished until the complete projection
  is physical. Commit revalidates and publishes every vehicle, group, mapped
  member, and cargo participant atomically before closing the outbound
  transaction; terminal ground cargo can be the only required recovery root, and
  clean player-bound cargo no longer pins the column during delivery.
  For a schema-53 exact enemy patrol it also exposes the live-position, contact,
  exact-route restart, and bounded route-recovery evidence consumed by the patrol
  operation service. Exact patrol groups are excluded from legacy garrison-
  patrol, survivor-repair, route, fold, and cleanup ownership.
  For a schema-54 exact purchased-garrison patrol it supplies the corresponding
  live-position and persisted local-route restart boundary while excluding the
  group from legacy aggregate activation, composition, waypoint, population,
  survivor, fold, and cleanup owners. The Schema-60 performance repair keeps
  pure-vehicle groups on vehicle-only living-count authority, skips the convoy
  whole-group survivor pass when no active convoy exists, and does not repeat a
  full member-count scan merely because a runtime entity already exists. Normal
  success diagnostics build costly visual evidence only after their 30-second
  throttle admits the log. These changes target the observed once-per-second
  stall cadence and still require a packaged server comparison.
- `HST_GeneratedContentService`: generated Everon mission sites, roadblock,
  support, stash, crashsite, and route records derived from stable zone
  anchors.
- `HST_MissionObjectiveService`: rough mission objectives, campaign task state,
  and no-admin objective progress hooks for broad-alpha mission families.
- `HST_MissionRuntimeService`: physical MVP mission primitives, world-condition
  objective polling, runtime inspection, and cleanup state. For a schema-52
  exact convoy it plans the three vehicle assets and optional cargo/captive
  before admission, then defers travel, arrival, and cargo-position authority
  to the mission-convoy operation instead of regenerating missing assets or
  resolving arrival from its legacy timer.
- `HST_SupportRequestService`: stateful FIA/enemy support calls,
  ETA/status/cooldown reports, native-safe ground support activation, physical
  or abstract resolution records, the schema-46 exact paid-QRF bridge from
  accepted manifest to queue-owned projection, and schema-47 survivor restore,
  elimination, and linked ledger settlement policies. ETA is only the earliest
  physical-arrival observation; spawned requests do not become
  `physical_arrived` or complete recall until physical war confirms current
  living-member distance. Recall mutation returns `HST_SupportRecallResult`
  with explicit accepted/already-applied/state-changed/terminal flags,
  disposition, failure, request, and operation identity. Exact paired full
  refunds validate both linked transactions and their settlement eligibility
  before either refund begins. New confirmed exact paid infantry QRFs also opt
  into the canonical operation contract. In schema 50 they begin as held virtual
  projections, follow the strategic route, run the narrow on-station virtual-
  combat policy, materialize exact survivors near a player, and fold exact live
  survivors back to the held roster after players leave the larger radius while
  the operation is clear of contact. Queue admission, projection transfers,
  restore, recall, and settlement update the same operation/request/group/
  manifest/batch identities. Schema 60 applies the same exact projection owner
  only to newly confirmed player Search-and-Destroy, with a separate operation
  discriminator, assignment policy, and archive identity. It remains on station
  after hostile infantry clears until commander recall, and a physical fold
  away from its assignment first enters virtual return-to-assignment. Route
  initialization occurs while duty is still
  `STAGING`; `LinkOutboundVirtual` commits delivered strategic service by moving
  duty to `OUTBOUND`. A terminal failure before that commit fully refunds money
  and HR. A later no-handoff materialization failure retains money and refunds
  only the operation's last virtual-friendly count, including from
  `ON_STATION`, with replay-idempotent settlement. Restore also clears the linked
  request's physicalized flag and publishes a virtual runtime status so request,
  operation, and held batch cannot disagree about projection ownership.
- `HST_CivilianService`: town reputation/support, wanted heat,
  police/roadblock presence and scans, aid incidents, undercover eligibility,
  request/application, enforcement, compromise, clear-state records, and
  disposable pedestrian/traffic projections. Ambient actors receive dedicated
  CIV group roots that inherit the stock behavior/pathfinding/utility/
  replication stack; initial members attach through the engine's AI-composition
  path, and the service owns their group, driver, and waypoint cleanup. Current
  source classifies stock town-center locations separately from minor localities,
  permits civilian projection even when HQ safety suppresses hostile military
  activation, selects deterministic non-repeating concrete appearances, defaults
  true towns to five driven vehicles, limits the known woodland locality to two
  pedestrians, and clears horn input only on HST-owned ambient drivers. Schema
  60 additionally overrides the native wheeled-vehicle base AI horn timing and
  horn sound-power fields, covering inherited AI-driven wheeled vehicles beyond
  the ambient-service input path. These behavior corrections await a
  republished runtime test.
- `HST_EnemyCommanderService`: enemy resource spending into patrol, roadblock,
  QRF, counterattack, rebuild, support-call, and Petros attack orders with
  physical/abstract runtime state. Schema 51 routes only newly planned
  infantry-only defensive QRFs through `HST_EnemyQRFOperationService`; their
  exact runtime bypasses the fixed legacy resolve timer and legacy support-row
  physicalization. Schema 53 dispatches newly queued patrol orders by both type
  and contract version to `HST_EnemyPatrolOperationService`; their proactive
  attack debit, exact one-root roster, generated-route loop, physical/virtual
  transfer, return, and settlement bypass the legacy patrol timer. Historical
  patrols and every other order type keep contract-version-0 behavior.
- `HST_MapMarkerService`: native marker rebuild/publish behavior plus marker
  status/detail/audit reports for strategic zones, missions, objectives,
  Defend Petros, support, QRFs, HQ, and convoy state. Linked terminal-group
  state is checked before unresolved-QRF visibility, preventing a dead response
  from retaining a tactical marker. Schema-50 source labels static zones with
  location and owner and reports virtual exact player-QRF travel/on-station
  state. Schema 51 publishes one marker for every open exact enemy defensive-QRF
  operation at its strategic or live cursor with faction, living count, duty,
  immutable source/assignment, and ETA. Schema 52 publishes one aggregate
  marker for an open exact mission convoy at the operation's strategic/live
  position and suppresses the three misleading per-vehicle convoy markers.
  Recovery retains the same aggregate current and destination marker pair for
  cargo and vehicle outcomes; it never revives legacy per-vehicle/outcome IDs.
  Schema 53 publishes one roster-authoritative marker per open exact enemy patrol
  at its virtual cursor or live position, with duty and living-count context, and
  removes it after terminal cleanup.
  Schema 54 publishes one marker per open exact purchased-garrison patrol at its
  virtual/live position with assignment location, current owner, role, and
  durable survivor count. Forces UI reports exact patrol infantry separately
  from legacy aggregate infantry; neither presentation owns the roster. Schema
  61 makes this rebuild the source of a revisioned logical marker registry: an
  unchanged stable ID keeps its revision/sequence, create or content update
  advances both record revision and the global stream, and removal emits a
  bounded tombstone. Server-native campaign marker publication is retired;
  clients reconcile native campaign markers from the authoritative registry.
  Authored static markers bind through one cached exact
  `HST_ConflictMapMarker_<zoneId>` entity-name lookup; the prior periodic
  radius scan is not part of identity or refresh authority.
- `HST_MarkerProjectionCodec`: owns marker protocol version `1`, bounded string
  encoding/decoding, escaped fields, snapshot/delta headers, packet record and
  character limits, and deterministic live-registry hashing. It rejects malformed
  or oversized payloads before a client registry can mutate.
- `HST_ClientProjectionService`: owns the server's current marker registry,
  bounded ordered event journal, and one readiness/ACK session per connected
  player. It sends hashed chunked snapshots for first join, reconnect, late join,
  epoch/hash mismatch, unavailable history, or resync; otherwise it replays only
  a retained contiguous delta range. The slowest valid connected ACK constrains
  pruning.
- `HST_ClientMarkerProjectionService`: owns the widget-independent client
  registry and native-marker reconciliation. Snapshot chunks stage until the
  complete count and hash validate, then replace the registry atomically.
  Deltas must begin at the next global sequence and advance each marker's
  revision exactly once. Old duplicates are acknowledged idempotently; a gap or
  invalid revision requests resync without partial mutation. Map open/reopen
  only reconciles widgets from this already-current registry.
- `HST_MarkerProjectionSaveValidationService`: rebuilds pre-61 derived marker
  rows without inventing campaign facts and advances the epoch before replacing
  malformed current projection state. `HST_MarkerProjectionProofService`
  defines fixtures for snapshot/JIP, ordered create/update/delete, duplicate replay,
  dropped-delta resync, reconnect, ACK pruning, malformed input, and migration
  idempotency; they remain unexecuted deterministic source fixtures.
- `HST_ZoneCompositionService`: runtime alpha composition slots for zone and
  mission physicalization diagnostics. Schema 59 removes radio transmitters
  from this generic owner's spawn and cleanup authority. The radio-site service
  binds exactly one authored target when uniquely discoverable, never deletes a
  borrowed authored entity, and creates a generated transmitter only after a
  rebuild actually completes; unresolved initial sites receive no fallback.

Static marker rendering has a separate client lifecycle boundary. Schema 61
keeps the authoritative marker registry alive whether or not the map widget is
open, then creates/updates/removes client-local native campaign markers from that
registry when the native map surface is available. A small manager patch retries
root creation before the stock static-marker update and disables a marker that
remains rootless. The rendered-map proof waits for the delayed client pass and
inspects actual active roots and widget components; protocol registry equality
and native handle/widget readiness are distinct proof gates. Dynamic player
markers remain on the existing replicated-entity path and are not records in the
Schema-61 stream.
Config-backed modded classes on this path must also repeat the base class's
container metadata. A packaged schema-49 test verified that restoring the
original `BaseContainerProps` and custom-title attributes brought normal Game
Master and stock HUD initialization back. That run then exposed eighteen invalid
radio icon entries as giant boxes. Current source preserves the canonical placed-
marker table, appends or repairs a validated normal/glow radio icon by resource
identity instead of a hard-coded index, and rejects invalid entries. The map-
target prompt, target indicator, and dialog now share a map-local z-order below
the native workspace pointer. Those follow-up fixes still need packaged proof.

Static helper services keep repeated low-level behavior out of the coordinator:
`HST_WorldPositionService` resolves dry/safe positions and prefab spawning,
`HST_DisplayNameService` normalizes item and vehicle labels,
`HST_VehicleRootPolicy` centralizes safe vehicle-root eligibility,
`HST_VehicleCapabilityPolicy` classifies captured/source vehicles as ammo,
repair, fuel, armed, or transport sources, and
`HST_ConvoyVehicleControlAdapter` wraps native vehicle movement/seating calls,
registers valid pilotable vehicles with the crew utility before seating,
prefers forced authority-local entry for server-owned AI, and exposes a direct
retained-registration query for runtime proof.

The coordinator currently exposes server-only mutation methods that check
campaign phase, known IDs, and mission eligibility before changing state.
Client UI requests flow through player-owned request/RPC components and
ownership-resolved coordinator methods, so the server resolves the trusted
identity and enforces member, commander, and admin permissions instead of
trusting client-provided player IDs.
The same player-owned bridge carries marker-projection readiness, contiguous
acknowledgements, resync requests, and reliable owner-targeted payloads. Server
session identity is derived from the owned controller/component; a client-
supplied player ID is diagnostic input only and cannot select another session.

## Campaign Authority Boundary

Schema 43 extends the first campaign-authority foundation with exact force
planning. Schema 44 adds durable bounded SpawnQueue authority for executable
manifest projections. Schema 45 adds explicit force/projection identity on
active groups, durable Game Master registration evidence, dependency-ordered
cleanup, and the first engine-facing exact infantry adapter. Schema 46 makes
player-paid resistance QRF the first support consumer of the complete quote →
ledger → executable manifest → SpawnQueue → settlement path. None of these
schemas is a claim that every existing broad-alpha mutation uses the boundary.
Schema 47 adds durable exact-member casualty state, ever-populated/spawn-
completed terminal semantics, corpse-preserving root cleanup, and survivor-only
paid-QRF reprojection. Schema 48 bounds accepted quote/manifest/ledger history,
preserves compact issue/confirmation/transaction replay, and adds the missing
production caller for pin-aware terminal SpawnQueue maintenance.
Schema 49 adds a canonical, versioned operation aggregate for newly confirmed
exact paid player infantry QRFs and uniquely coherent active schema-48 rows.
That narrow slice is not a claim that legacy QRFs, other support, missions,
garrisons, enemy orders, or every force projection use the aggregate.
Schema 50 adds persistent strategic projection to that same narrow consumer:
direct-route progress, held exact roster authority, proximity hysteresis,
physical-to-virtual survivor transfer, bounded virtual infantry combat, and
restore normalization to one virtual projection. It does not migrate vehicles,
assets, convoys, garrisons, broad legacy supports, missions, or enemy orders.
Schema 51 adds a second, separately typed consumer: newly planned infantry-only
enemy defensive QRF orders. Admission freezes one prepaid roster from a distinct
same-faction operational source for a same-faction defended target under resistance pressure, links one operation/
batch/group aggregate, and suppresses a
parallel legacy QRF or support row for the same target. The force travels and
folds through the shared exact projection boundary, applies defensive pressure
once at its immutable assignment, returns to its immutable origin, and refunds
attack/support resources in proportion to exact survivors once. Pre-schema-51
orders, counterattacks, patrols, roadblocks, support calls, Petros attacks,
vehicles, garrisons, missions, and other force families remain contract version
`0` on their prior paths.
Schema 52 adds a third typed operation consumer and the first narrow exact
vehicle/multi-group aggregate: only a newly started `convoy_intercept` mission
is admitted with contract version `1`. Its frozen manifest contains exactly
three vehicle slots, one crew group per vehicle, ordered durable crew slots, and
an optional cargo/captive asset assigned to vehicle zero. Three durable convoy
element rows own per-vehicle position, survivor count, damage/fuel/ammunition
snapshot, disposition, and reciprocal mission/operation/manifest/runtime links.
  The operation owns generated-route progress, virtual movement, proximity
  materialization/fold, arrival, restore normalization, and terminal receipt;
  existing mission-outcome code remains the once-only reward/penalty owner.
  When crew elimination leaves cargo/captive/vehicle recovery unresolved, the
  operation remains open in an on-station recovery hold and may virtualize and
  rematerialize those assets instead of settling early.
Pre-schema-52 convoy rows remain contract version `0`, receive no invented
manifest/operation/element identity, and continue on the legacy path.
Schema 53 adds a fourth typed operation consumer: only newly queued enemy
`PATROL` orders receive contract version `1`. Admission freezes one infantry
root and its member slots, spends proactive attack resources once, and links one
order/route/operation/manifest/held-batch/group aggregate. A generic generated-
route cursor owns outbound travel, exactly one closed on-station lap, and a
separate return-to-origin leg. Virtual and physical projection share the frozen
roster; mapped casualties survive fold and restore, while physical contact holds
the route clock until clear. Return settles one survivor-proportional proactive-
attack refund. Type-plus-version dispatch keeps this policy isolated from the
exact defensive-QRF and legacy owners. Corrupt current-schema rows retain
diagnostic authority under quarantine version `-53` and never fall back to a
legacy timer. Historical patrol rows remain contract version `0` and receive no
invented route, roster, operation, or refund. Packaged schema-50 through
schema-53 certification remains independently open.
Schema 54 adds a fifth typed operation consumer: only a newly issued policy-v2
purchased resistance garrison receives `HST_OPERATION_TYPE_GARRISON_PATROL`
contract version `1`. Its immutable manifest contains one executable
`NotSpawned` container root and the arbitrary ordered members selected by the
quote. The accepted graph remains in strategic hold and walks a persisted local
route loop indefinitely while virtual; proximity projects only living slots and
fold retains exact casualties/cursor state. PhysicalWar excludes the exact group
from legacy aggregate garrison owners. Owner change, all-dead, campaign stop,
setup, or typed spawn/route failure records one
`exact_garrison_patrol_terminal` receipt with zero resource
refund and zero transfer into aggregate infantry. Policy-v1 purchases and all
initial/enemy aggregate, vehicle, and multi-root garrisons remain legacy.
Pre-schema-54 migration invents none of this authority; malformed current rows
retain quarantine version `-54`. Packaged schema-50 through schema-54
certification remains independently open.
Schema 55 adds a sixth typed operation consumer: only guard infantry for a
newly started `assassinate_officer` mission receives
`HST_OPERATION_TYPE_MISSION_GUARD` contract version `1`. The HVT remains outside
the exact manifest. One empty execution root and ordered member roster own a
route-less, offset guard assignment through strategic hold, physical projection,
mapped casualties, fold, re-entry, and zero-refund typed settlement. Guard
elimination settles `DESTROYED` without completing the HVT objective; HVT
success maps to `COMPLETED`, mission failure/expiry/campaign stop/setup to
`CANCELLED`, owner change to `INVALIDATED`, and coherent spawn/assignment failure
to `SPAWN_FAILED`. The operation has no route or virtual-combat owner. Historical
officer missions, other assassination variants, and all remaining mission
families remain contract `0`; pre-55 migration invents nothing. Malformed current
rows quarantine at `-55` without a legacy fallback, HVT backlink, guessed death,
or refund, and the diagnostic quarantine leaves the HVT mission playable. The
existing HVT marker and mission UI project guard strength instead of adding
another marker. The stamped Schema-55 tree identifies implementation
`552c2c4ff5ac7608fa248c614480a254769b61a4`, passes the full foundation gate and
clean Workbench Game validation at 5,763 files/11,570 classes with CRC
`0ec8950e`, and survives a ten-sample/20-second normal WorldEditor open. Native
entity/adapter/casualty behavior, save/restart, rendered UI, owner-change,
campaign-setup, packaged networking, reconnect, and JIP proof remain open.
Schema 56 adds a seventh explicit family consumer without adding a seventh
operation enum: guard infantry for only a newly started `assassinate_traitor`
mission uses `HST_OPERATION_TYPE_MISSION_GUARD` contract version `2`, manifest
policy `exact_assassinate_traitor_guard_v1`, and quarantine version `-56`.
Schema-55 officer guards remain contract `1` with `-55` quarantine. The traitor
path deliberately reuses the same route-less empty-root/member roster, separate
HVT authority, survivor-only materialization/fold, no-virtual-combat policy,
zero-refund typed terminal mapping, compact settled restore, and existing-HVT
status projection. Pre-56 and historical traitor missions, `assassinate_specops`,
and all other mission families remain contract `0`. Pre-56 migration records
`migration_schema56_exact_traitor_guard` and invents no authority; malformed
current traitor graphs record
`normalization_schema56_exact_traitor_guard_conflict` and remain diagnostic
without fallback or HVT failure. Six focused source-proof categories cover the
new contract, but Schema-56 native entities/adapter casualties, real save/
restart, rendered UI, owner-change, campaign setup, packaged networking,
reconnect, and JIP are unclaimed. The stamped Schema-56 tree identifies
implementation `bab5748d817ba434dae701cfbb3b92805d463678`, build label
`schema56-exact-traitor-guard`, stamp
`03a65cd33bee69c6320389803cdd5a2ec8576fb0`, and passes the full foundation gate. Workbench
Game validation loaded 5,764 files/11,573 classes with CRC `a18c67a5` and
reported `Script validation successful`; its bounded hidden normal WorldEditor
open stayed alive for all ten samples over 20 seconds and the latest log had no
script-error/crash signature. These source/Workbench gates do not close any
packaged behavior obligation.
Schema 57 adds an eighth explicit family consumer without adding an operation
enum: only guards for a newly started `assassinate_specops` mission use the
mission-guard operation at contract `3`, policy
`exact_assassinate_specops_guard_v1`, intent `assassinate_specops_guard`, and
quarantine `-57`. Officer `1`/`-55` and traitor `2`/`-56` remain unchanged. The
spec-ops path reuses the route-less empty-root/member roster, HVT separation,
survivor-only materialize/fold/re-entry, no virtual combat, typed zero-refund
settlement, compact restore, and existing-HVT status. Historical/pre-57 spec-ops
missions and unsupported families stay contract `0`; ordinary
`mission_group_*` rows are not claimants. Migration/conflict events are
`migration_schema57_exact_specops_guard` and
`normalization_schema57_exact_specops_guard_conflict`. Schema 57 is stamped at
implementation `514ebdcbeb1ddfb2a383b19590382517113e2ff6` with build label
`schema57-exact-specops-guard`. The full foundation gate passes, including its
Schema-55/56/57 checks. Stamped Workbench Game validation loaded 5,765 files/
11,576 classes with CRC `e0b8578e` and `Script validation successful`; the
bounded hidden normal WorldEditor open stayed alive for 10/10 samples over 20
seconds, and its log had no script-error/crash signature. These source/Workbench
gates leave every packaged/native/restart/UI/network gate open. The
assassination-guard family is exhausted. Schema 58 is the later separate rescue
cutover for newly started `rescue_pows` only; historical POWs,
`rescue_refugees`, and further rescue families remain legacy until explicitly
versioned.

Schema 58 adds the ninth explicit family consumer and seventh operation type:
only newly started `rescue_pows` use mission-rescue contract `1`, policy
`exact_rescue_pows_v1`, intent `rescue_pows_guard`, and quarantine `-58`. One
hash-valid composite manifest owns a separately executed exact guard roster and
three policy-validated captive descriptors. Typed captive rows, the frozen HQ
anchor, command receipts, custody/grace rules, damage-only death, exact
extraction receipts, and mission-owned terminal mapping remain authoritative;
historical POWs, refugees, and other mission families stay contract `0`.

Schema 59 adds a separate exact strategic-site owner rather than another force
family. Each radio zone receives one durable site row. A destroy mission may
start only from a resolved ONLINE site and succeeds only after physical target
destruction. Borrowed authored-target destruction must match the reciprocal
active site/mission lock and revision, an authoritative DESTROYED damage manager,
and the frozen projection. Generated-target explosive scoring additionally needs
a live matching mission component, a bounded physical position, and a unique key
in the persisted bounded evidence set. Mission-time ownership/provenance prevents
later ownership handoff from relabeling that evidence. Success moves the site
to DESTROYED and suppresses its town influence. A stop-rebuild mission may start
only once for a given tower-destruction epoch, moves the site to REBUILDING on
admission, and targets construction equipment rather than a second transmitter.
Destroying that equipment records the rebuild attempt and returns the site to
DESTROYED without advancing the tower-destruction epoch; mission failure,
expiry, or campaign stop completes the rebuild and returns one campaign-
generated transmitter to ONLINE. The stable site target ID never doubles as a
mission projection handle: every mission owns a unique physical runtime-entity
ID. Every accepted admission or outcome uses a deterministic request, typed
from/to states, receipt, and monotonic revision. Generic mission runtime, zone
composition, objective ticks, commander progress, and generic failure settlement
skip exact and quarantined radio aggregates. Missing borrowed projections enter
an explicit dormant pending phase; broken reciprocal runtime rows quarantine.
Generated ONLINE restore requires a coherent destruction receipt followed by a
completed-rebuild receipt. Packaged proof of authored discovery, natural
explosives, damage reapplication, generated
replacement, stream re-entry, real restart, rendered markers/UI, networking,
reconnect, and JIP remains open.

Schema 60 adds the tenth explicit family consumer and eighth operation type:
only newly quoted and confirmed player Search-and-Destroy support uses contract
`1`, policy `support_search_destroy_exact_infantry_1`, and quarantine `-60`.
One catalog-selected infantry-only roster is frozen at $350 plus one HR per
member slot and linked through quote, ledger, request, operation, held batch, and
active group. It follows the direct strategic route, materializes/folds exact
survivors, and applies bounded virtual infantry combat at its assignment. A
physical fold farther than 75 meters from that assignment becomes
`RETURNING_TO_ASSIGNMENT`; hostile clearance leaves the force on station until
commander recall. Before fold, physical recall exit, or campaign-stop
retirement, the projection receives an exhaustive mapped-casualty
reconciliation; a nonzero roster must then prove one root plus exactly one
unique adapter and PhysicalWar member binding per durable living slot. Held-
batch cancellation first snapshots the strategic living roster. Persistence
applies an exhaustive global exact-infantry reconcile, repeats reciprocal and
cardinality checks for each physical exact player-support aggregate, and
refreshes its live position before capture; failure defers the retirement or
checkpoint. Pre-60 requests remain contract `0`, and malformed current claimants
cannot cross into legacy authority. Schema 60 is the preceding stamped source/
Workbench checkpoint: implementation `fdf78493dd15915afe8d53f61a8ad1efd65b5635`,
UTC `2026-07-11T23:24:55Z`, label `schema60-exact-search-destroy`, Foundation
pass with 644 symbol references, and final Workbench CRC `7aa80fc9` at 5,777
files/11,615 classes. Schema 59 is the preceding checkpoint. This provenance is
not packaged or live-behavior certification.

Schema 61 adds the first bounded authoritative client projection without adding
an operation family. The durable marker registry uses stable IDs, record
revisions, tombstones, an epoch, and a monotonic global sequence. A server
snapshot/delta journal and ownership-derived ACK sessions feed widget-independent
client registries, which in turn own local native campaign markers. One in-flight
delta batch, final-packet ACK, post-ACK catch-up, bounded readiness heartbeat,
and per-dispatch restart age prevent overlap while recovering lost ACKs or
incomplete streams. Builders and decoders enforce the same payload limits. The
client caches authored descriptor bindings and hides one only after its custom
zone replacement is live, restoring prior visibility on failure. Shared
priority/stable-ID ordering makes marker caps deterministic. The compiled source
proof defines ordering, idempotency, gaps/resync, reconnect/JIP-shaped snapshots,
rapid mutations, lost ACK, epoch reset, pruning, malformed/oversize input, and migration. Packaged host/two-client equality,
actual reconnect/late join, native widget behavior, and process restart remain
open, and its fixtures have not been executed as runtime evidence. Schema 61 is
the latest completed source/Workbench checkpoint at implementation
`27672e67ce4285810f313130293df1ac917c9bdf`, UTC `2026-07-12T01:02:39Z`, and
label `schema61-authoritative-marker-projection`. Full Foundation passes with
655 symbol references; final Workbench Game validation loaded 5,782 files/
11,631 classes with CRC `df41a779` and created the game. The hidden normal
WorldEditor stayed alive/responding for 10/10 samples over 20 seconds without a
first-party error/crash signature.

| Concern | Current implementation | Target architecture |
| --- | --- | --- |
| Stable identity | A persisted monotonic allocator creates authority IDs; garrisons, quotes, manifests, transactions, and selected support/order/group records carry explicit stable links. Schema 58 additionally binds the rescue operation/guard graph to three deterministic captive slots, one frozen extraction position, and stable escort, carrier, seat, command, casualty, extraction, and projection evidence. | Every durable operation, force, projection, command, transaction, and event has a stable ID and explicit links. |
| Command idempotency | Visible command requests carry request IDs; bounded receipts cover migrated training and quote/confirm commands. Support recall maps a typed result into explicit receipt status. Schema 58 additionally keeps accepted and rejected captive request/actor/command/result/revision rows, rejects cross-captive or changed-fingerprint reuse before live checks, and reserves one non-evicting terminal slot. Other visible commands still use the compatibility classifier. | Every player-visible and scheduled campaign mutation enters through a typed command envelope and produces one durable receipt. |
| Resource integrity | Troop training, visible garrison confirmation, player-QRF, and Schema-60 Search-and-Destroy confirmation/terminal settlement use the resource ledger. The new support freezes $350 plus exact roster HR and settles eligible living HR through commander recall. Enemy QRF and enemy patrol own separate proportional settlements. Other support/order consumers still use legacy services. | All resource changes use reserve/commit/cancel/refund transactions, with no direct debit paths outside the ledger. |
| Force exactness | Existing exact infantry/convoy/rescue shapes remain unchanged. Schema 60 adds one infantry-only Search-and-Destroy root/member manifest and rejects vehicles, assets, empty rosters, and multi-root substitution. Historical Search-and-Destroy, other support/mission families, policy-v1/initial/enemy aggregate garrisons, and unsupported vehicle/multi-root policy remain outside these cutovers. | A quoted immutable force manifest is the only input to paid creation, and creation is all-or-nothing before any physical or virtual projection is published. |
| Force realization | SpawnQueue accepts frozen, hash-valid, all-required one-root infantry manifests. Both QRFs, player Search-and-Destroy, exact enemy patrol, policy-v2 purchased-garrison patrol, and all three exact assassination guards begin held and release only durable living member slots; each empty root contributes no authored members. The schema-52 convoy keeps its separate three-element PhysicalWar adapter. Confirmed casualties remain retired across transfer/restore. Generic vehicle/asset/multi-root, historical mission guards, and historical aggregate-garrison realization remain unsupported. | One adapter realizes every supported manifest, registers each slot exactly once, restores successful projections safely, and feeds durable living-force/casualty/retirement authority without bypass paths. |
| Operation lifecycle | Schemas 50-59 retain their exact paths. Schema 60 adds a separately typed player Search-and-Destroy operation with direct-route progress, exact virtual/physical survivors, bounded virtual combat, return-to-assignment after displaced fold, on-station hold after hostile clear, commander recall, and fail-closed restore isolation. Historical requests remain outside the contract. | Every force/order uses one versioned operation aggregate with event-driven engagement, strategic movement progress, physical/virtual transfer, settlement, and client/JIP projection. |
| Event history | New command and ledger decisions append to a bounded persisted campaign event log. | All authoritative state transitions emit typed events consumed by projections, UI, diagnostics, and restore reconciliation. |
| Client marker projection | Schema 61 implements stable marker IDs with record revisions/tombstones, one epoch/global sequence, bounded hashed snapshot and ordered-delta packets, one in-flight batch, final-only ACK, post-ACK catch-up, readiness heartbeat/restart backoff, ownership-derived sessions, a widget-independent atomic registry, deterministic priority capping, and fail-safe client-local native reconciliation. Server-native campaign marker publication is retired; authored descriptors hide only behind a live replacement and restore on failure; dynamic player markers remain separate. | Extend the same explicit projection discipline only after each additional menu/task/notification model has a bounded DTO and authority contract. |
| Certification | Schema 61 is the latest completed stamped source/Workbench boundary at implementation `27672e67ce4285810f313130293df1ac917c9bdf`, UTC `2026-07-12T01:02:39Z`, label `schema61-authoritative-marker-projection`, Foundation 655, and Workbench CRC `df41a779` at 5,782 files/11,631 classes. Normal hidden WorldEditor remained responsive 10/10 over 20 seconds. Its deterministic fixtures and Workbench compilation are source evidence only. Every packaged server/client, actual restart, rendered-UI, stutter/horn, live-behavior, network/reconnect/JIP gate remains open. | Isolated physical runtime, save/load/reprojection, dedicated-server, reconnect, and JIP evidence certifies the full boundary. |

The next source dependency is the canonical idempotent ownership transition:
one service/result must own owner, garrison/security, support, facility/logistics,
economy, enemy consequence, event, persistence, and derived projection changes.
Schema 61 provides the marker consumer for that transition; it does not make the
currently distributed capture and political-flip mutations canonical by itself.

Concurrent open garrison quotes are capped and expired/terminal unreferenced
planning rows can be pruned. SpawnQueue terminal projection rows have explicit
backlink pins, a 600-second minimum retention window, and a 128-row admission
bound; the production coordinator now supplies pins and runs that maintenance.
Schema 48 retains accepted rows in full for at least 600 seconds and while any
projection backlink exists, then atomically moves replay identity and final
transaction evidence into at most 256 tombstones with an 86,400-second minimum
window. Full quotes plus tombstones share a hard 320-row admission bound. An old
evicted quote fails closed rather than recreating a debit or aggregate mutation.

Until those target columns are closed, domain services remain authoritative for
their existing behavior, but they must not be described as uniformly
idempotent, ledger-backed, or exact-manifest driven.

Campaign-debug state isolation is fail-closed. Every in-process profile is
restricted to the development world. The persistence service first durably
captures the live campaign, the coordinator runs the suite against a deep-cloned
`HST_CampaignState`, checkpoints only isolated in-memory save data, and swaps the
untouched live state back on completion or cancellation. External/soak profiles
cannot enter this runner. This protects authoritative campaign persistence, but
does not roll back world entities, player inventory/health, delayed callbacks,
or service caches, so a development-session restart remains part of the safety
boundary.

## Authoring Contracts

The addon keeps authoring data separate from runtime state:

- `HST_CampaignPreset`: fixed scenario roles and capability switches.
- `HST_FactionTemplate`: faction identity and capability declarations.
- `HST_MapDefinition`: stable IDs for zones and hideout candidates.
- `HST_BalanceConfig`: campaign balance and progression values.
- `HST_MissionRegistryConfig`: mission definitions and deferred capabilities.

The current coordinator uses `HST_DefaultCatalog` as runtime authority. The
checked-in faction `.conf` resources are not loaded by that path and are not
guaranteed mirrors; they must not be cited as gameplay truth until one loader
and startup validator replaces the duplicated declarations. Schema-43 exact
force planning uses `HST_ForceCatalogService`, whose explicit catalog version
and ordered slots are validated against effective prefab containers at runtime.
Schema-44 queue admission consumes the frozen execution-prefab slots directly;
manifest identity binds the input but never substitutes for a projection or
idempotency key. Schema 45 keeps the group root and member entities as runtime
projections while persisting their force/projection links and registration
evidence. Schema 46 selects one authored player-QRF group, freezes all ordered
slots, and submits that exact manifest without invoking the broad composition
service. Schema 47 persists each exact member's living/retired lifecycle and
reuses the immutable manifest while excluding confirmed casualties from restore
reprojection. The current adapter deliberately rejects vehicle, asset, and multi-root
manifests instead of shortening them or falling back to broad composition.

## Persistence

The persistence service tracks `HST_CampaignSaveData` through
`PersistenceSystem`, applies restored state through a schema migration path,
and flushes the tracked scripted state before requesting
`SaveGameManager.RequestSavePoint` when saving is possible and allowed. The
service also writes `$profile:h-istasi/HST_CampaignSaveData.json` as a profile
fallback when scripted persistence cannot be flushed, and will load that file
if no restored `PersistenceSystem` state is available. The state model is
versioned from day one. `HST_CampaignSaveData` is the deep-copy save container
for current campaign fields and nested runtime arrays. Before autosave, major-
change, manual-checkpoint, or campaign-debug baseline capture, schemas 52 through
60 synchronously reconcile mapped physical exact-convoy members and every
currently cut-over physical or dematerializing exact-infantry family. Patrol,
mission-guard, and player-support projections must prove one unique handed-off
root, one unique live handle per durable survivor, matching result/projection
and slot keys, matching PhysicalWar membership, and an authoritative live
position where that family moves. Exact player support also requires one
unambiguous reciprocal request/operation/batch/group graph; its global exhaustive
casualty pass is followed by a per-aggregate root/member cardinality proof and a
live physical-position refresh before capture. An unexplained deleted binding
remains unresolved rather than becoming a casualty. An open outbound publication
transaction, materializing exact-infantry handoff, missing/conflicting mapping,
aliased or cross-key runtime ownership, invalid cardinality, unverifiable
physical position, or nonphysical operation retaining process-local member
mappings defers capture before an older tracked snapshot is flushed or an engine
savepoint is requested. Quarantined patrols also defer until both adapter and
PhysicalWar runtime ownership are empty, then retain the diagnostic graph under
strategic authority without refund. The pending checkpoint intent remains set
and retries on the bounded debounce. Schema 53 persists the
monotonic authority sequence, bounded command receipts, resource transactions,
campaign events, stable operation/garrison links, immutable force manifests,
expiring quotes, durable per-projection spawn batches/slot evidence,
Game Master registration evidence, explicit active-group force/projection IDs,
support quote/capability/schedule fields, linked exact-QRF aggregate identities,
successful-handoff/reprojection counters, member casualty tombstones, active-
group ever-populated/spawn-completed/living-force evidence, and the queue
restore/reconciliation epochs, plus bounded accepted-settlement replay tombstones
and their final resource-status/refund evidence. It also persists canonical
exact-QRF operation records with immutable assignment, mutable tactical target,
typed duty/engagement/materialization/position/settlement state, execution links,
policy IDs, timestamps, terminal result, revision, direct-route cursor and
distance, strategic speed/update clock, projection decisions, virtual-combat
clock/damage carries, exact virtual force counts, strategic batch holds, enemy-
order source/manifest/service-commit/resource-settlement authority, reciprocal
enemy-order/operation/group backlinks, and distinct-second physical-arrival
confirmation state, plus exact enemy-patrol generated-route waypoint/lap/leg
cursor and loop clocks, proactive resource settlement, contact-held projection,
and reciprocal route/order/operation/manifest/batch/group identity, plus exact
mission-convoy operation/manifest/spawn/
settlement links, three durable convoy elements, per-element vehicle/crew/cargo
identity and state, mission-asset vehicle-slot assignment, generated-route
cursor, and exact convoy arrival/settlement authority,
  plus schema-54 exact purchased-garrison quote/manifest/garrison/operation/
  generated-route/held-batch/group links, local loop cursor, exact casualties,
  and no-refund terminal receipt, plus schema-55/56/57 exact officer/traitor/spec-ops mission/
  operation/manifest/held-batch/group links, route-less guard anchors, exact
  casualties, typed terminal results, separate HVT authority, and mission-family
  contract/policy/quarantine identity, plus Schema-60 Search-and-Destroy quote/
  manifest/paired-ledger/request/operation/held-batch/group links, exact roster,
  route/combat/return-to-assignment state, recall/settlement identity, and
  contract-0/`-60` restore disposition, alongside campaign
metadata, resources, campaign-end
reason/summary/elapsed second/control/war/zone-count fields, outcome-mode,
population/support, airfield metadata, support deployment proof, active-group
vehicle prefab, active-group route waypoint counts, runtime infantry waypoint assignment and final-sweep state,
HQ/Petros/cache/arsenal/tent/spawn-point fields,
faction pools, players, zones, garrisons, active groups, QRFs, map markers with
Schema-61 per-record revision/stream/tombstone metadata plus the marker-
projection epoch and global sequence,
generated content, objectives, mission runtime, mission assets, support, enemy
order, civilian, undercover, arsenal, garage, vehicle cargo, runtime vehicle,
saved loadout, issued-item, ammo point, and captured-emplacement records.
Loadout editor sessions remain runtime/editor state, while durable saved
loadouts and issued-item ledgers are copied into the save container. Save compatibility
still needs broader Workbench restart/load soak testing before it is promised
to players. An actual persisted restore increments its epoch once, then the
coordinator reconciles the queue before garrison/player-QRF confirmation,
  schema-51 enemy-QRF authority, schema-52 mission-convoy normalization, schema-53
  enemy-patrol normalization, schema-54 purchased-garrison patrol normalization,
  schema-55 officer-mission guard normalization, schema-56 traitor-mission guard
  normalization, schema-57 spec-ops-mission guard normalization, Schema-60
  player Search-and-Destroy normalization, Maiden's Bay location retirement,
  Schema-61 marker-projection normalization, and
open-resource reservations. Accepted exact
QRFs never reacquire saved entity IDs. Schema 50 clears process-local root/
member/native-group evidence and keeps one nonterminal batch in strategic hold;
schema 51 applies that same one-owner restore rule to its opted-in enemy QRF and
also clears its physical-arrival sample counters. Confirmed-dead slots remain
retired and only exact survivors can be released for later materialization. Interrupted
pending, `READY_FOR_HANDOFF`, and previously successful physical batches all
normalize to that same virtual authority instead of assuming an interrupted
handoff succeeded or respawning immediately. A post-handoff technical failure
still retains paid money and refunds durable survivors only. Terminal prefab,
verification, handoff, and casualty history remains evidence, while terminal
entity/native-group IDs are cleared rather than reacquired as living authority.

The schema-49 migration from schema 48 creates a contract-version-1 operation
only for a uniquely coherent accepted nonterminal exact paid player QRF. It preserves request,
quote, manifest, queue, group, transaction, and balance state and leaves every
pre-exact, terminal, archived-only, incomplete, or ambiguous row on contract
version `0`. Schema-50 migration opts only coherent exact infantry-QRF
operations into strategic projection. On every restore, a nonterminal opted-in
projection clears process-local entity IDs, becomes one held virtual batch with
strategic position authority, and retains exact survivor/casualty slots, duty,
assignment, economy, and terminal history. Unsupported or ambiguous aggregates
remain operation-only. These rules have deterministic source coverage but no
packaged process-restart evidence.

Schema-51 migration never invents exact authority for a historical enemy order.
Every pre-schema-51 enemy row remains contract version `0` with no backfilled
source claim, manifest, operation, service commitment, or refund. A current-
schema versioned enemy defensive QRF must restore with one coherent reciprocal
order/operation/manifest/batch/group aggregate or fail closed and preserve its
evidence for once-only resource settlement. Its physical runtime handles are
discarded, its exact casualty roster and route cursor remain durable, and its
projection resumes as one held virtual batch. Deterministic roundtrip fixtures
cover this shape in source; a real process restart remains unproven.

Schema-52 migration never opts a historical active convoy into exact authority.
Every pre-schema-52 convoy remains operation contract version `0`; migration
does not invent a route, manifest, vehicle slot, crew roster, cargo carrier,
convoy element, or settlement receipt. A current-schema open exact convoy must
restore as one coherent mission/operation/manifest/held-batch/three-element
aggregate or fail closed. Restore discards process-local vehicle/group handles
and physical arrival samples while retaining route progress, element positions,
vehicle state, cargo assignment, and confirmed crew casualties, then resumes
the operation virtually. Only member slots may carry casualty tombstones, open
authority must use an enumerated legal duty/resume and materialization/position
pair, and missionless or partially unlinked exact-looking rows remain durable
quarantine evidence instead of being deleted by generic cleanup. This shape is
  source-validated; physical
  rematerialization and real process restart remain packaged-runtime-open.

Schema-54 migration never opts a historical garrison into exact patrol
authority. Policy-v1 accepted purchases, initial-map and enemy aggregates, and
vehicle counts retain their prior representation with no invented operation,
roster, route, batch, casualty, or settlement. A coherent current policy-v2
graph clears process-local handles and resumes one held virtual survivor roster
on the same local loop. A malformed current graph is retained at quarantine
version `-54` without refund, guessed death, or aggregate conversion. Physical
and dematerializing rows join pre-capture root/member/PhysicalWar binding and
live-position validation; capture defers during a partial materialization wave
until its all-required handoff or terminal result is authoritative. This shape
has deterministic source proof and passes the stamped Schema-54 Workbench
compile/open gates. Native movement, fold, save/restart, networking, and JIP
remain packaged-runtime-open.

Normal adapter acquisition runs only after the campaign enters the active phase.
During setup and won/lost phases, the coordinator requests cancellation for all
nonterminal batches and keeps draining dependency-ordered cleanup with a
monotonic runtime-only clock. That cleanup clock advances retry/defer eligibility
without advancing campaign elapsed time. Schema 47 implements successful
survivor reprojection and casualty/retirement authority for the exact paid
infantry-QRF consumer. It confirms physical death only from a present slot-
mapped entity's authoritative life state; deletion or a missing entity is not
casualty evidence. Schema 50 adds virtual roster transfer, direct strategic
movement, materialization hysteresis, and narrow virtual combat for that same
consumer. Schema 51 reuses the exact infantry projection and casualty boundary
  for newly planned enemy defensive QRFs, but not the player-QRF virtual-combat
  policy. Schema 52 adds a separate exact three-vehicle mission-convoy projection
and atomic fold path. Terminal destroyed/captured vehicle roots retain any
living crew as stationary crew-only projections without vehicle resurrection,
and proximity ownership takes the nearest separated living/recoverable root;
  it does not generalize the SpawnQueue adapter or simulate off-screen combat.
  Schema 53 reuses the one-root infantry queue/adapter for newly queued exact
  enemy patrols, but gives them a generated-route loop, contact-held route clock,
  return-origin duty, and proactive survivor-refund policy distinct from both
  QRF consumers. Schema 54 reuses the same adapter only for newly issued
  policy-v2 purchased resistance garrisons, using an empty executable root,
  arbitrary exact member roster, infinite local loop, survivor-only
  rematerialization, and no-refund settlement.
Schema-60 migration never promotes a historical Search-and-Destroy request.
  Pre-60 rows remain contract `0`; current exact-looking rows must validate one
  reciprocal quote/manifest/ledger/request/operation/batch/group graph or retain
  evidence under `-60` quarantine with no inferred balance, casualty, or refund.
The same migration keeps the Logistics Warehouse as the sole enumerable
Maiden's Bay site. No-anchor saves remain untouched and ambiguous authorities
fail closed. Generic references and mutable generated content canonicalize,
while nonzero typed graphs and their frozen generated rows remain unchanged;
deep canonical clones support new generation. Mutable lookup resolves the
warehouse, and detached old-ID lookup plus runtime ID equivalence protects
frozen historical links. The isolated migration proof is compiled and wired to
Campaign Debug but has not been executed; packaged restore/restart remains open.

Schema-61 migration treats map markers as derived projection state. Pre-61
records are cleared and rebuilt from zones, missions, forces, and other durable
owners with fresh revision/stream metadata; migration never infers one of those
owners from an old marker. A current-state projection with invalid epoch,
sequence, revision, tombstone, stable-ID, or stream relationships is also cleared
and rebuilt, but first advances the epoch so a connected client cannot continue
an incompatible stream. The migration and normalization events are idempotent.
The source proof exercises this boundary in memory; serialization and a real
process restart remain packaged-runtime gates.

Event-driven physical life-state subscription, generalized
vehicle/asset rosters, generalized folding, live physical engagement events,
and generalization to every force consumer remain open. Paid support is only
partially migrated to this path: player QRF and newly confirmed player Search-
and-Destroy are exact, while supply, roadblock, fire, and air support remain on
legacy services. Historical Search-and-Destroy and policy-v1 garrison purchase
manifests remain nondeployable; initial/enemy aggregate garrisons and garrison
vehicles/multi-root forces remain on legacy PhysicalWar paths.

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
metadata and a debug fallback, not the normal player-side path. US
remains the occupier in the strategic preset. Game Master-spawned characters
are still not expected to advance h-istasi's player lifecycle. Workbench
offline play may log blank identity ID errors from stock reconnect or
editable-entity systems; treat those as non-blocking if a character is spawned
and possessed.

`HST_HQService` owns the server-side HQ lifecycle: setup-driven initial hideout
selection, HQ movement between authored hideouts, Petros/cache/arsenal/tent/spawn-point
runtime positions, and Petros-loss penalties. Runtime Petros spawning tries the
custom h-istasi prefab first through its GUID-qualified metadata resource and
falls back to the base FIA character only if that resource cannot spawn. The HQ
arsenal uses a GUID-indexed HST supply-cache prefab whose contextual actions
open the same Arsenal/Loot menu path used by the I-key menu and the custom
loadout editor path, with inherited stock arsenal actions filtered out. A stock
FIA cache fallback is only used if the custom object cannot spawn.

The alpha HQ menu is procedural rather than layout-resource loaded. The server
keeps the existing `HST_MENU`, `TAB`, `STATUS`, `RESULT`, and `ACTION` payload
lines while adding optional `STAT`, `SECTION`, `ROW`, and `FEED` lines for the
h-istasi overview, HQ/Petros, missions, map/war, forces, arsenal/loot,
garage/build, members, and admin panels. The custom loadout editor has its own
`HST_LOADOUT_EDITOR` and `HST_LOADOUT_CANDIDATES` payloads but still routes
mutations through the same server-authoritative request bridge. Contextual
Petros, HQ arsenal, and vehicle cargo actions call the same bridge as menu
clicks so local hosts and MP clients follow one command path.

## Campaign Framework Spine

The first campaign loop is now connected as a physical/abstract hybrid. Zones
carry type, position, income, support, activation radius, route IDs, mission
site IDs, and garrison-slot data in `HST_CampaignState`; garrisons are stored
as infantry and vehicle counts. The physical-war service marks zones active
when players enter their activation radius, converts abstract garrison counts
into route-aware active groups, and folds survivor counts back before
deactivation. A mixed group with previously observed personnel becomes terminal
when its living infantry reaches zero after delayed-population and live-count
grace, even if its attached vehicle remains intact. That vehicle becomes
unclaimed, detached salvage and no longer contributes to garrison,
capture, QRF, or marker strength; vehicle-only projections keep their existing
separate semantics. Existing durable field ownership/cargo is preserved; an
unadopted salvage record is session-only. Eliminated/spawn-failed save normalization preserves zero
counts and clears process-local spawn identity rather than reconstructing
original force totals; folded rows retain the survivor counts already returned
to the abstract garrison.
Broad-alpha services generate mission sites/routes, attach
objectives/tasks to started missions, poll physical MVP mission primitives from
world conditions, spend scaled enemy pools into orders/support calls, and let
orders/support either physicalize near players or resolve abstractly off-screen.
The schema-50 exception is the newly confirmed exact paid infantry QRF: its
single operation and frozen member roster move continuously through virtual,
materializing, physical, dematerializing, and virtual states rather than using a
one-shot abstract outcome. Schema 51 applies the same projection ownership to
new infantry-only enemy defensive-QRF orders, with a distinct source/target,
arrival-pressure, return, and resource-settlement policy. Schema 52 gives only
new mission convoys their own generated-route virtual cursor and exact
three-element physical projection; it does not opt historical convoys or other
  missions in. Schema 53 opts only newly queued patrol orders into one generated-
  route outbound/lap/return policy; historical patrols remain legacy. Schema 54
  opts only newly issued policy-v2 purchased resistance garrisons into an exact
  indefinitely looping local patrol and leaves historical/initial/enemy aggregate
  garrisons legacy. Schema 60 separately opts only newly confirmed player Search-
  and-Destroy into direct strategic travel, exact survivor projection, bounded
  virtual combat, return-to-assignment, and commander-recall settlement. These
  narrow paths remain intentionally isolated from broad legacy support, non-
  convoy missions, and every other enemy order family.
HQ knowledge feeds HQ threat, Defend Petros, markers, and campaign-end pressure;
civilian town support and undercover enforcement feed wanted heat, roadblock and
police scans, and HQ exposure. Civilian render eligibility is separate from the
hostile military activation bit so HQ safety does not erase nearby town life.
Schema-59 radio sites borrow one uniquely resolved authored transmitter and
route destroy/rebuild missions through their durable lifecycle owner; generic
composition and mission runtime cannot create or delete that projection. Loot,
vehicle cargo,
virtual garage, build
placement, vehicle capability classification, and loadout editor systems are
owned by campaign state instead of stock arsenal behavior. Mission success,
failure, timeout, convoy outcome, support resolution, enemy orders, vehicle/cargo
paths, and civilian aid mutate economy, support, capture progress, arsenal,
garage, aggression, HQ threat, and victory/loss readiness. Coordinator hooks
expose deterministic server-only actions for Workbench tests and no-admin player
actions for setup, random missions, support requests/cancel, civilian aid,
undercover status/checks, looting, vehicle cargo, garage capture/redeploy,
loadout application, marker/command audits, balance/campaign-end reports,
income, training, recruitment, and HQ moves.
