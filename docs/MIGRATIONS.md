# Campaign Save Migrations

## Current Schema

`HST_CampaignState.SCHEMA_VERSION` is currently `60`. Schema 60 is the latest
stamped source/Workbench checkpoint: implementation
`fdf78493dd15915afe8d53f61a8ad1efd65b5635`, UTC
`2026-07-11T23:24:55Z`, build label `schema60-exact-search-destroy`, and final
Workbench CRC `7aa80fc9`. Schema 59 is the preceding checkpoint.

## Schema 60

- Schema 60 adds a separately typed exact player Search-and-Destroy operation
  only for newly quoted and confirmed `HST_SUPPORT_SEARCH_AND_DESTROY`
  requests. Contract `1` links one immutable quote, infantry-only manifest,
  paired ledger transactions, support request, operation, held SpawnQueue batch,
  and active group. The quote costs $350 plus one HR for every frozen member
  slot; the selected catalog roster targets `3 + war level` members within the
  supported catalog bounds. Vehicles, assets, empty rosters, and multi-root
  manifests are rejected rather than shortened or substituted.
- The operation uses its own
  `HST_OPERATION_TYPE_PLAYER_SUPPORT_SEARCH_DESTROY` discriminator while
  reusing the exact infantry strategic-projection boundary. It persists direct-
  route progress, virtual/physical ownership, exact member casualties, virtual
  combat clocks, immutable assignment, mutable live position, and recall/
  settlement identity. Leaving the render bubble folds only observed survivors.
  A physical on-station group folded more than 75 meters from its assignment
  enters `RETURNING_TO_ASSIGNMENT`, travels back virtually, and resumes on-
  station behavior instead of fighting from the wrong strategic position.
- Off-screen combat uses the existing deterministic 30-second infantry power
  steps against the target's abstract hostile garrison. Eliminating the hostile
  infantry clears contact but does not auto-complete the purchased operation;
  it remains on station until commander recall. Recall follows the typed exit
  path and settles eligible living HR through the linked ledger without
  inventing survivors or replaying a refund.
- Physical fold, physical recall exit, and campaign-stop retirement first run
  an exhaustive projection-scoped casualty reconciliation. If survivors remain,
  retirement additionally requires one handed-off root and exactly one unique
  live adapter/PhysicalWar member binding per durable living slot. Persistence
  performs the global exhaustive exact-infantry pass, validates each physical
  exact-support request/operation/batch/group graph and binding cardinality, and
  refreshes the live group position before capture. Any missing or conflicting
  authority defers retirement or checkpoint rather than inventing a casualty.
  Held-batch cancellation snapshots the strategic living roster before queue
  cleanup so immediate recall refunds only the roster that actually survived.
- Pre-Schema-60 Search-and-Destroy requests remain contract `0` on their legacy
  path. Migration does not infer a quote, roster, transaction, operation, batch,
  group, casualty, or settlement. Malformed current exact claimants retain their
  evidence under quarantine contract `-60`, are prevented from falling through
  to legacy execution, and do not receive guessed balance changes. Their group
  quarantine mode/status is recognized by global campaign-state classification,
  so the retained evidence is neither operational nor combat-present.
- Expired exact-support archive capacity applies full receipt reciprocity to
  every positive typed player-support contract, including exact QRF and Search-
  and-Destroy. It prunes a tombstone and its paired terminal request atomically
  only when the tombstone is valid replay authority,
  the aggregate identity has exactly one claimant, no live quote/manifest/
  operation/transaction/batch/group backlink remains, and the terminal request
  fully reciprocates the archived identity, assignment, costs, schedule,
  settlement, and refund receipt. Historical contract-0 QRF retains its minimal
  compatibility match. A malformed or quarantined typed pair remains durable
  evidence and cannot be selected merely to make capacity.
- Schema 60 also removes the overlapping `town_maidens_bay` strategic row and
  keeps `resource_logistics_warehouse` as the one canonical Maiden's Bay
  location. Location normalization is anchor-gated: a save containing neither
  row is left byte-stable. More than one canonical row, or multiple legacy rows
  without one canonical authority, fails closed before any graph, projection,
  ledger, or generated-content rewrite; only one idempotent conflict audit is
  appended. A lone legacy row converts in place and retains its nonzero owner,
  economy, and aggregate-force values. When both rows exist, the canonical
  warehouse keeps its owner/economy and aggregate manpower; the duplicate town
  does not receive a fold-back or manpower credit.
- Once unique location authority exists, mutable contract-0 campaign references
  canonicalize to the warehouse and duplicate ambient/garrison/police
  projections retire directly. Any nonzero typed claimant remains frozen,
  including settled, malformed, quarantined, and graphless exact rows
  recognized from their group mode/status. Typed legacy garrison backlinks may
  retain one zero-manpower compatibility shell, but cannot add to the canonical
  garrison.
- Mutable generated sites/routes rekey to the canonical ID, with canonical
  collisions removing the legacy duplicate. A site or route referenced by
  frozen typed authority remains byte-stable and instead receives a deep
  canonical clone, including route endpoints, validation fields, and every
  waypoint rekey. General `FindZone(oldId)` calls resolve the canonical mutable
  row; exact historical validators may request a detached old-ID/old-position
  view. Runtime duplicate/admission checks treat the old and canonical IDs as
  equivalent without re-enumerating the retired town.
- Duplicate enemy-support ledgers merge only after their stale spend/damage
  windows expire. Spend and refund totals add, recent damage takes the bounded
  maximum, timestamps/cooldown take the maximum, and the newest applicable
  decision reason wins.
- The one-second stutter and horn repairs do not add durable save fields. Runtime
  reconciliation now avoids pure-vehicle count oscillation and redundant/no-
  convoy scans, amortizes unresolved radio discovery, reuses cached recurring
  player authority, and builds expensive visual diagnostics only when their
  throttled log will emit. The native wheeled-vehicle base override disables AI
  horn timing and perceived horn output; the existing ambient-driver input clear
  remains a scoped defensive layer.
- `HST_MaidensBayLocationMigrationProofService` is compiled and wired into
  Campaign Debug as `location_taxonomy.maidens_bay_schema60`. Its isolated
  in-memory fixtures cover both-row and old-only migration, generic-reference
  normalization, frozen-graph isolation, ledger/generated-content handling,
  idempotency, and lookup compatibility. That Campaign Debug assertion has not
  been runtime-executed, so it is not migration or restart evidence.
- Schema-60 full Foundation passes with 644 symbol references. Final stamped
  Workbench Game validation loaded 5,777 files/11,615 classes with CRC
  `7aa80fc9` and created the game. The correctly targeted hidden normal
  WorldEditor stayed alive/responding for 10/10 samples over 20 seconds with no
  first-party error/crash signature; diff check was clean apart from line-ending
  warnings. These are source/Workbench gates only. Packaged server/client,
  actual save/restart, rendered UI/marker, physical movement/combat, reconnect,
  JIP, stutter, and horn gates remain open.
  The Search-and-Destroy proof service is compiled and wired into Campaign Debug,
  but its Schema-60 assertions have not run. It now includes valid paired archive
  capacity prune plus save/restore and corrupt quarantine retention. Its
  deterministic fold/immediate-recall casualty case uses synthetic queue-slot
  state; live adapter casualty retirement and physical recall exit remain
  packaged-runtime gates.
- The compiled/wired `HST_OperationRecordProofService` archive assertion now
  also covers typed exact-QRF receipt mismatch retention under forced capacity;
  it has not been executed by Campaign Debug.

## Schema 59

- Schema 59 introduces one durable `HST_RadioSiteState` for every radio zone.
  The row owns a deterministic site ID and transmitter target ID, immutable
  authored prefab/position provenance plus the mutable current projection after
  resolution, typed ONLINE/DESTROYED/REBUILDING lifecycle,
  BORROWED_WORLD or GENERATED_CAMPAIGN ownership, one reciprocal active-mission
  lock, typed transition fingerprints, revision, timestamps, and destruction/
  rebuild receipts. A process-local entity handle is only a projection; losing
  it is never destruction evidence. The stable site target ID is separate from
  the unique deterministic physical runtime-entity ID assigned to each exact
  mission.

- The radio lifecycle service is the only transmitter projection owner. It
  adopts one unambiguous authored transmitter without taking deletion
  ownership, freezes that binding, and prevents zone composition or generic
  mission runtime from creating, repairing, completing, or deleting an exact
  radio target. Ambiguous authored candidates quarantine before world mutation.
  DESTROYED authored targets have their damage state reapplied after streaming
  or restart. A replacement transmitter is campaign-generated only after a
  rebuild actually finishes; the active stop-rebuild mission projects separate
  construction equipment rather than a second intact tower. The supported
  authored transmitter has retained multiphase damage behavior, so borrowing,
  destruction, rebind, and reset do not depend on deleting or replacing its
  world identity. Permanent generated ONLINE projections disable verbose
  witness logging and keep the nearby-entity witness query dormant until an
  exact asset/mission/role identity is configured.

- Only newly started `destroy_radio_tower` and
  `dynamic_stop_tower_rebuild` missions opt into radio-site contract `1`.
  Destroy admission requires a resolved ONLINE site and records ONLINE to
  ONLINE; physical destruction then commits ONLINE to DESTROYED. Stop-rebuild
  admission is the production DESTROYED to REBUILDING transition. Its success
  returns to DESTROYED and records one rebuild-attempt receipt without changing
  the tower destruction receipt or epoch. Only one stop-rebuild attempt is
  accepted per destruction epoch. Failure, expiry, or campaign stop completes
  the rebuild as ONLINE with GENERATED_CAMPAIGN ownership and a rebuild receipt.
  Normal and forced/debug starts share the same lifecycle gate, and generic
  objective progress or direct sabotage cannot manufacture success.

- Borrowed authored-target destroy reports are accepted only when the mission
  and site hold reciprocal active lock/revision state, the tracked damage manager
  is authoritatively DESTROYED, and its position matches the frozen binding.
  Generated-target explosive reports additionally require a matching live
  mission-asset component, a bounded projection position, and a durable unique
  evidence key in a persisted bounded dedupe set. Each mission asset snapshots
  the ownership and authored descriptor present at admission, so later ownership
  handoff cannot relabel historical demolition evidence. Raw damage-state
  completion is restricted to the borrowed authored destroy target; generated
  transmitter/equipment completion requires exact
  explosive-score evidence. New-campaign reset reacquires and restores the
  authored transmitter before state replacement or fails closed. Physical
  destroy/heal/rollback writes are verified after mutation. The immutable
  authored match stays within 0.75 meters; physical projection evidence allows
  the bounded 12-meter safe-ground offset.

- A borrowed projection that is temporarily absent keeps its reciprocal active
  aggregate but clears mission/asset/runtime spawned flags and records
  `radio_site_projection_pending`; already-destroyed evidence preserves
  `radio_site_target_destroyed`. Missing, duplicate, or cross-linked runtime
  rows quarantine instead of waiting out the mission. Generic runtime,
  composition, objective ticking, commander progress, and generic failure
  settlement skip exact and quarantined radio authority.

- Every accepted admission or outcome increments the site revision and copies
  it to the mission. An immediate retry of the same deterministic request and
  typed fingerprint is already-applied without mutation; changed fingerprints
  conflict, and an old request after a later lifecycle cycle is rejected by the
  stale revision. Only resolved exact ONLINE sites emit town
  `radio_broadcast` influence. DESTROYED, REBUILDING, unresolved, missing, and
  quarantined sites emit none, and income evaluates this state after lifecycle
  reconciliation.

- Restores from schema 58 or earlier record
  `migration_schema59_radio_site_authority`. They create one ONLINE/UNRESOLVED
  logical row per radio zone without inferring a physical binding, destruction,
  rebuild, mission outcome, receipt, or reward. Active legacy radio missions
  fail closed at contract `0`; terminal historical rows remain contract `0`.
  Current-schema duplicate identities, cross-site request/receipt reuse, remote
  or unsupported bindings, illegal lifecycle/outcome shapes, bad timestamps,
  and broken mission/site/asset revisions quarantine at `-59` and record
  `normalization_schema59_radio_site_authority_conflict`. Generated ONLINE
  restore requires a destruction receipt followed by a completed-rebuild
  receipt. Quarantine fails and cleans a corrupt current linked aggregate while
  preserving coherent already-terminal historical outcome semantics.

- Source proof and Workbench compilation cover state transitions, replay/stale
  rejection, ownership handoff, migration/roundtrip/quarantine, generic-owner
  isolation, influence suppression, marker/UI status, and coordinator ordering.
  The proof invokes production admission/outcome transitions through projection-
  only seams. It also exercises the production durable-evidence helper, rejects
  a direct second rebuild admission in the same epoch, and proves linked
  quarantine cleanup. Native authored-entity discovery, actual explosive destruction and
  damage-state reapplication, generated replacement visuals, streaming, real
  process restart, rendered UI, packaged networking, reconnect, and JIP remain
  packaged-runtime gates until a republished run supplies evidence.
  The stamped Schema-59 source/Workbench checkpoint identifies implementation
  `37fb5f0ffbc80c4bba3151ba1e5d8be6ffcf8a21` with build label
  `schema59-radio-site-lifecycle`. The full Foundation gate passes. Final
  Workbench validation loaded 5,773 files/11,608 classes with CRC `96914c26` and
  `Script validation successful`; a bounded normal open stayed alive/responding
  for 10/10 samples over 20 seconds with no script-compile/crash signature. Its
  one Steamworks stats-request error was nonfatal.

## Schema 58

- Schema 58 opts in only a newly started `rescue_pows` mission. The mission and
  reciprocal `HST_OPERATION_TYPE_MISSION_RESCUE` operation use contract `1`,
  manifest policy `exact_rescue_pows_v1`, guard intent `rescue_pows_guard`,
  projection contract `1`, and malformed-current quarantine version `-58`. It is
  the ninth explicit family consumer across the seventh operation type.
  Historical/pre-58 `rescue_pows`, `rescue_refugees`, generic captive rows, and
  every other unsupported family remain contract `0`.

- One operation owns one frozen composite manifest. Its executable subgraph is
  one catalog-backed hostile infantry root plus the exact ordered member slots.
  Its externally managed asset subgraph is exactly three required `captive`
  slots. The SpawnQueue batch records external asset authority and contains only
  group/member result rows; the mission-rescue service owns the three typed
  `HST_MissionAssetState` captive rows. No prefab-name manpower estimate,
  generic mission guard, vehicle slot, route, resource debit, refund, or generic
  asset-spawn result becomes authority.

- Captives persist an ordinal and typed UNKNOWN/HELD/FREED/FOLLOWING/BOARDING/
  BOARDED/EXTRACTED/KILLED disposition plus stable escort identity, carrier
  vehicle identity, seat token, last command result, a bounded typed
  command ledger (request, actor, command, result, and recorded revision),
  casualty receipt, extraction receipt, projection identity, transition second,
  revision, and projection generation. The mission and all three captives also
  retain one frozen HQ extraction position and one common base deadline.
  Process-local entity handles and compatibility carrier strings are cleared on
  restore. A missing projection is never death evidence; only observed
  authoritative damage state can record KILLED.

- HELD and FREED captives can fold to durable state outside the player/render
  bubble and reproject at their last authoritative position. FOLLOWING,
  BOARDING, and BOARDED remain projected because they are bound to a stable
  escort/carrier. Carrier discovery requires a connected stable escort and a
  replication-backed/runtime-tracked vehicle identity; the position-derived
  local fallback is rejected. Native seat evidence advances BOARDING to BOARDED.
  Outside grace, a disconnected escort releases custody to FREED without death;
  during grace it invalidates the frozen custody set. Terminal non-success
  settlement likewise releases surviving custody rows to FREED, so historical
  rescue validity never depends on a later mutable carrier-vehicle row.

- Guard elimination does not settle the rescue. Any one of the three required
  casualty receipts fails the mission; exactly three unique extraction receipts
  at the frozen HQ extraction point complete it through the existing mission
  outcome/reward path. The target town receives the configured support reward
  once when applicable. Timer expiry grants exactly 300 seconds of extraction
  grace only when all three living captives were already in stable custody; the
  grace accepts no new release or escort claim and cannot produce both expiry
  and success. Its end is always the frozen base deadline plus 300 seconds, so
  delayed ticks do not change durable timing authority. HQ relocation is blocked
  while that frozen extraction authority is open; settled history remains valid
  after a later HQ move.

- Restores from schema 57 or earlier record
  `migration_schema58_exact_rescue_pows`. They preserve historical POW missions,
  legacy `mission_group_*` guards, legacy captive actions, and rewards without
  inventing an exact mission, operation, manifest, guard roster, captive
  identity, carrier, seat, casualty, extraction, settlement, or reward receipt.

- Current-schema validation requires unique reciprocal mission/operation/
  manifest/batch/group identity, a valid generated mission site on the target
  zone, exact target/site position, one hostile catalog roster, three captive
  slot/row bijections, legal typed transitions, coherent grace and settlement,
  and no foreign resource/order/quote links. Valid open rows normalize only
  process bindings; command IDs are unique across exact captive rows,
  actor/command fingerprints and recorded revisions must match, extraction rows
  must be inside the frozen radius, and terminal mission/result combinations are
  exact. Compact settled rows may omit their terminal batch and guard group.
  Conflicts become `-58`, record
  `normalization_schema58_exact_rescue_pows_conflict`, and remain diagnostic
  without legacy fallback, guessed casualty, invented extraction, reward, or
  force transfer.

- Source proof covers exact admission/family isolation, composite manifest and
  external asset execution, captive transition/idempotency and carrier/seat
  evidence, render fold/re-entry, casualty/success/grace settlement, save
  roundtrip/migration, corruption quarantine, and state-derived UI/marker text.
  Schema 58 is an earlier stamped source/Workbench baseline at implementation
  `f0ba07ff2bc295d12542a3ea34b4c913e99b1869` with build label
  `schema58-exact-rescue-pows`. The full foundation gate passes. Final stamped-
  tree Workbench Game validation loaded 5,770 files/11,594 classes with CRC
  `aa73883a` and `Script validation successful`; the bounded hidden normal
  WorldEditor open stayed responsive for 10/10 samples over 20 seconds with zero
  crash/error matches. Native entity behavior, actual process restart, rendered
  UI, owner change, campaign setup, packaged networking, reconnect, and JIP
  remain open until a republished runtime run supplies those artifacts.

## Schema 57

- Schema 57 opts in only guard infantry belonging to a newly started
  `assassinate_specops` mission. The mission and reciprocal
  `HST_OPERATION_TYPE_MISSION_GUARD` operation use contract `3`; the frozen
  manifest policy is `exact_assassinate_specops_guard_v1`, the mission-family
  intent is `assassinate_specops_guard`, and malformed current authority uses
  quarantine version `-57`. Newly started officer and traitor guards retain
  contracts `1` and `2`, policies `exact_assassinate_officer_guard_v1` and
  `exact_assassinate_traitor_guard_v1`, and quarantine versions `-55` and `-56`.
  Historical or pre-schema-57 spec-ops missions and every unsupported mission
  family remain contract `0` (contract zero).

- Generic spec-ops composition may propose multiple groups. Contract `3`
  deterministically selects the strongest executable group, keeps the stable
  first group on ties, and freezes only that selected catalog roster. Discarded
  candidate groups never become mission, operation, manifest, batch, active-
  group, or restore authority.

- The contract-3 spec-ops aggregate reuses the exact assassination-guard shape:
  one catalog-backed `NotSpawned` empty execution root, exact ordered infantry
  member slots, one held SpawnQueue batch, and one mission-owned active group.
  It owns no route, generated cursor, vehicle, projected asset, virtual-combat
  clock, or resource cost. The HVT stays separate mission-objective/runtime-
  asset authority and is never a manifest member, operation asset, or group
  backlink.

- Durable member slots remain the sole guard-strength authority. Materialization
  releases only living slots; mapped physical deaths retire those exact slots;
  fold and re-entry preserve the same survivor set and last confirmed position.
  There is no virtual guard combat or inferred casualty. Exact and quarantined
  officer/traitor/spec-ops guard claimants are excluded from generic mission-
  group, garrison, route, survivor-repair, captured-zone, and cleanup owners.
  Ordinary historical `mission_group_*` rows are not exact claimants and gain no
  authority merely from a mission ID or matching instance backlink.

- Guard and HVT outcomes remain independent. All spec-ops guards dead settles
  the guard operation `DESTROYED` while its HVT mission may remain active. HVT
  success settles surviving guards `COMPLETED`; mission failure/expiry and
  campaign stop/setup settle `CANCELLED`; target-owner change settles
  `INVALIDATED`; coherent spawn/assignment failure settles `SPAWN_FAILED`.
  Every terminal path terminalizes its typed queue authority, records
  `exact_mission_guard_terminal` once, transfers no survivors, and applies a
  zero refund.

- Restores from schema 56 or earlier record
  `migration_schema57_exact_specops_guard`. Migration preserves coherent
  Schema-55 officer and Schema-56 traitor exact authority, while historical/
  pre-57 spec-ops missions, HVTs, objectives, and ordinary `mission_group_*`
  rows remain contract zero. It invents no manifest, roster, casualty,
  operation, projection, settlement, or claimant identity from mission IDs or
  aggregate counts.

- Current-schema validation dispatches by mission family and contract. A valid
  spec-ops graph requires unique reciprocal mission/operation/manifest/batch/
  group identity, policy `exact_assassinate_specops_guard_v1`, intent
  `assassinate_specops_guard`, exact empty-root/member bijection, offset
  stationary assignment, zero route/vehicle/asset/resource authority, legal
  projection state, separate HVT ownership, and the fixed terminal receipt.
  Coherent physical rows normalize to held survivors; compact settled graphs may
  omit their batch, group, and terminal HVT runtime rows.

- Malformed current spec-ops authority becomes contract `-57` and records
  `normalization_schema57_exact_specops_guard_conflict`. Quarantine is
  diagnostic and non-operational: it creates no legacy fallback, guessed
  casualty, HVT backlink, mission failure, refund, or force transfer. Existing
  HVT marker/UI rows append roster-authoritative guard status instead of adding
  a duplicate marker.

- `HST_SpecOpsGuardOperationProofService` covers six source-proof categories:
  admission/family isolation, survivor projection and HVT separation, typed
  zero-refund settlement, restore/migration, corruption quarantine, and
  existing-HVT marker status. Schema 57 is stamped at implementation
  `514ebdcbeb1ddfb2a383b19590382517113e2ff6` with build label
  `schema57-exact-specops-guard`. The full foundation gate passes, including
  Schema-55/56/57. Stamped Workbench Game validation loaded 5,765 files/11,576
  classes with CRC `e0b8578e` and `Script validation successful`; the bounded
  hidden normal WorldEditor open stayed alive for 10/10 samples over 20 seconds,
  and its log had no script-error/crash signature. Native entities, adapter
  casualties, actual save/restart, rendered UI, owner-change, campaign setup,
  packaged networking, reconnect, and JIP remain open.

- Schema 57 exhausts the assassination-guard family and opts in no rescue
  authority. Schema 58, documented above, is the next separate successor
  cutover for newly started `rescue_pows` only.

## Schema 56

- Schema 56 opts in only guard infantry belonging to a newly started
  `assassinate_traitor` mission. The mission and reciprocal
  `HST_OPERATION_TYPE_MISSION_GUARD` operation use contract `2`; the
  frozen manifest policy is `exact_assassinate_traitor_guard_v1`, and malformed
  current authority uses quarantine version `-56`. Newly started Schema-55
  `assassinate_officer` guards remain exact at contract `1` with `-55`
  quarantine. Historical or pre-schema-56 traitor missions,
  `assassinate_specops`, and every other unsupported mission family remain
  contract `0` (contract zero).

- The contract-2 traitor aggregate deliberately reuses the proven mission-guard
  shape: one catalog-backed `NotSpawned` empty execution root, exact ordered
  infantry member slots, one held SpawnQueue batch, and one mission-owned active
  group. It owns no route, generated cursor, vehicle, projected asset, or
  resource cost. The HVT remains separate mission-objective/runtime-asset
  authority and is never a manifest member, operation asset, or group backlink.

- The held member slots remain the only guard-strength authority. Near-player
  materialization releases only durable living members; mapped physical deaths
  retire their exact slots. Fold retires process-local projection ownership and
  requeues the same survivors at their last confirmed position. Re-entry realizes
  only those survivors. There is no virtual guard combat or inferred casualty.
  Generic mission-group, garrison, route, survivor-repair, captured-zone, and
  cleanup paths exclude exact and quarantined officer/traitor guard claimants.

- Guard and HVT outcomes remain independent. All traitor guards dead settles the
  guard operation `DESTROYED` while its HVT mission may remain active. HVT success
  settles surviving guards `COMPLETED`; mission failure/expiry and campaign stop/
  setup settle `CANCELLED`; target owner change settles `INVALIDATED`; coherent
  spawn/assignment failure settles `SPAWN_FAILED`. Every terminal path reconciles
  typed runtime authority, terminalizes its queue batch, records
  `exact_mission_guard_terminal` once, and refunds/transfers zero resources or
  legacy force. This is a zero refund policy on every terminal path.

- Restores from schema 55 or earlier record
  `migration_schema56_exact_traitor_guard`. Migration preserves coherent
  Schema-55 officer contract-1 authority exactly, while historical/pre-56
  traitor missions, HVTs, objectives, and legacy `mission_group_*` guards remain
  contract `0`. It never invents a traitor manifest, roster, casualty, operation,
  projection, or settlement from mission IDs or aggregate counts.

- Current-schema validation dispatches by mission family and contract. A valid
  traitor graph requires unique deterministic mission/operation/manifest/batch/
  group identity, policy `exact_assassinate_traitor_guard_v1`, exact empty-root/
  member bijection, offset stationary assignment, zero route/vehicle/asset/
  resource authority, legal virtual/physical state, separate HVT ownership, and
  the fixed terminal receipt. Coherent physical rows normalize to held survivors.
  Compact settled graphs may omit batch, group, and terminal HVT runtime rows; a
  settled `DESTROYED` guard may coexist with an active HVT mission.

- Malformed current traitor authority becomes contract `-56` and records
  `normalization_schema56_exact_traitor_guard_conflict`. Quarantine is
  non-operational and creates no legacy fallback, guessed casualty, HVT backlink,
  mission failure, refund, or force transfer. Existing HVT map/UI rows append
  roster-authoritative `guards N`, `guards neutralized`, or `guard authority
  unavailable`; no duplicate guard marker is published. Officer authority keeps
  its Schema-55 contract/policy/quarantine semantics alongside the new family.

- `HST_TraitorGuardOperationProofService` covers six source-proof categories:
  admission/isolation, survivor projection and HVT separation, typed settlement,
  restore/migration, corruption quarantine, and existing-HVT marker status. It
  also checks officer contract-1 coexistence and legacy traitor/spec-ops
  isolation. Native entities, real adapter handles/casualties, actual save/
  restart, rendered UI, owner-change, campaign setup, packaged networking,
  reconnect, and JIP remain unclaimed. The stamped Schema-56 implementation is
  `bab5748d817ba434dae701cfbb3b92805d463678`, build label
  `schema56-exact-traitor-guard`. It passes the full foundation gate; Workbench
  Game validation loaded 5,764 files/11,573 classes with CRC `a18c67a5` and
  `Script validation successful`. Its bounded hidden normal WorldEditor open
  stayed alive for all ten samples over 20 seconds and the latest log had no
  script-error/crash signature. These are source/Workbench gates only.

- Schema 57 later opts in newly started spec-ops guards through contract `3`.
  This Schema-56 section remains the stamped historical traitor-only boundary
  and does not retroactively convert pre-57 spec-ops missions.

## Schema 55

- Schema 55 opts in only guard infantry belonging to a newly started
  `assassinate_officer` mission. The mission contract and reciprocal
  `HST_OPERATION_TYPE_MISSION_GUARD` operation use version `1`; the frozen
  manifest policy is `exact_assassinate_officer_guard_v1`. Historical officer
  missions, `assassinate_traitor`, `assassinate_specops`, and every other mission
  family remain contract version `0`. The HVT remains a mission objective and
  mission-runtime asset: it is never a manifest member, projected asset, or
  operation-owned entity.

- Admission freezes one catalog-backed `NotSpawned` execution root and every
  ordered infantry member slot, with no vehicles, projected assets, money, HR,
  equipment, attack-resource, or support-resource cost. The mission, operation,
  manifest, held SpawnQueue batch, and mission-owned active group form one
  reciprocal aggregate. A deterministic guard anchor is offset from the HVT so
  the root and its members do not overlap the target; the HVT position remains
  the immutable tactical target. No route or generated-route cursor is created.

- The held roster is the only guard-strength authority. Entering the
  materialization radius releases the root and durable living member slots;
  mapped physical deaths retire only their member slots. Contact and recent
  casualties hold the physical projection. Leaving the larger radius retires
  the runtime projection and requeues the same survivors at their last confirmed
  position. Re-entry realizes only those survivors. Generic mission-group,
  zone-garrison, survivor-repair, route, captured-zone, and cleanup paths exclude
  exact and quarantined guard claimants.

- Guard elimination and HVT outcome remain independent. If every guard member
  dies, the operation settles `DESTROYED` with zero survivors while the active
  HVT objective remains playable. HVT success settles surviving guards
  `COMPLETED`; mission failure or expiry and campaign stop/setup settle
  `CANCELLED`; coherent spawn/assignment failure settles `SPAWN_FAILED`; target
  owner change settles `INVALIDATED`. Every terminal path first reconciles and
  retires typed runtime authority, terminalizes the queue batch, records the
  fixed settlement kind `exact_mission_guard_terminal` once, and applies zero
  refund or legacy-force transfer.

- Restores from schema 54 or earlier record
  `migration_schema55_exact_mission_guard` and preserve historical officer
  missions, HVTs, objectives, and `mission_group_*` guards exactly as legacy
  contract-zero state. Migration never infers a manifest, member roster,
  casualty, operation, projection, or settlement from aggregate counts or an
  officer mission ID. Unsupported pre-schema-55 exact-shaped operations are
  retained as quarantined evidence instead of being adopted.

- Current-schema validation requires unique deterministic mission/operation/
  manifest/batch/group identity, an exact empty-root/member bijection, the
  offset stationary assignment, zero vehicle/asset/resource authority, legal
  virtual/physical state, and the fixed terminal receipt. Coherent physical
  state restores to one held survivor roster with process-local IDs cleared.
  Compact settled graphs may legitimately have already removed their batch,
  group, and terminal HVT runtime row. A settled `DESTROYED` guard may coexist
  with an active HVT mission; other terminal outcomes require a non-active
  mission.

- Malformed current authority becomes contract version `-55`. Quarantined rows
  are non-operational and never create a legacy fallback, guessed casualty,
  refund, or HVT ownership backlink. Strongly owned successful projections use
  typed casualty reconciliation and projection-key cleanup; foreign or
  ambiguous rows remain untouched. This boundary records
  `normalization_schema55_exact_mission_guard_conflict`. Persistence defers while materialization is
  incomplete or a live/quarantined binding cannot be proven. Existing HVT map
  and mission UI rows append roster-authoritative `guards N`, `guards
  neutralized`, or `guard authority unavailable`; no duplicate guard marker is
  published.

- Deterministic source proofs cover admission/replay/rollback and legacy
  isolation, survivor/HVT separation, typed terminal mapping, restore/migration,
  corruption quarantine, and marker-status projection. Native entity creation,
  real adapter bindings and casualties, physical return-to-anchor behavior,
  actual process restart, rendered UI, packaged multiplayer, reconnect, and JIP
  remain open until a scoped packaged run proves them. Owner-change and campaign-
  setup settlement are also open engine/runtime gates. The stamped Schema-55
  tree identifies implementation `552c2c4ff5ac7608fa248c614480a254769b61a4`,
  passes the full foundation gate and clean Workbench Game validation at 5,763
  files/11,570 classes with CRC `0ec8950e`, and survives a ten-sample/20-second
  normal WorldEditor open. Those results are not packaged behavior proof.

- Schema 56 later opts in newly started traitor guards through their own
  contract-2 policy. This Schema-55 section remains the historical officer-only
  boundary and does not retroactively convert pre-56 traitor missions.

## Schema 54

- Schema 54 opts in only a newly issued, newly accepted policy-v2 purchased
  resistance garrison. Its manifest policy is `garrison_exact_patrol_2`; its
  canonical operation type is `HST_OPERATION_TYPE_GARRISON_PATROL` at contract
  version `1`. Admission freezes one executable `NotSpawned` group root that
  contributes no self-populated soldiers plus the arbitrary ordered infantry
  member slots selected and priced by the quote. The accepted quote, manifest,
  resource transactions, garrison backlink, operation, held SpawnQueue batch,
  generated local route, and active-group projection form one reciprocal graph.
  The legacy aggregate infantry count is not incremented for this policy; exact
  living slots reserve garrison capacity through their accepted-manifest link.

- The exact purchased roster begins in strategic hold and remains assigned to
  its immutable resistance-owned zone. Its four-point local generated route uses
  the shared persisted waypoint/lap/leg cursor as an infinite on-station patrol
  loop. Virtual ticks continue that route outside the render bubble. Entering
  the materialize-in radius releases the empty root plus only durable living
  member slots; leaving the larger radius after the casualty/contact hold folds
  the authoritative live position and survivors back into the same held batch.
  The adapter roster check now compares current infantry to the durable living
  slot count after first handoff, so a partial survivor roster can rematerialize
  without restoring the original purchased count. Confirmed dead slots never
  return.

- `HST_PhysicalWarService` recognizes `exact_garrison_patrol` ownership and
  excludes those groups from legacy aggregate zone activation, broad composition,
  garrison patrol assignment, population repair, survivor fold, route mutation,
  and cleanup. Legacy PhysicalWar remains the sole owner for initial-map, enemy,
  historical aggregate, vehicle, and other non-policy-v2 garrisons. Schema 54 is
  an infantry-only single-root cutover, not generic vehicle or multi-root
  garrison realization.

- This purchase has a zero refund policy: no survivor or terminal refund. Owner
  change, exact all-dead authority, campaign stop, setup, or typed spawn/route
  failure retires runtime ownership and records the
  deterministic settlement kind `exact_garrison_patrol_terminal` exactly once.
  Fold, rematerialization, quarantine, and terminal cleanup never mutate the
  committed money or HR transactions. Settlement removes the live garrison
  backlink only after adapter and PhysicalWar ownership are safely retired.

- Every save restored from schema 53 or earlier preserves accepted policy-v1
  purchases and all initial/enemy aggregate garrisons on their existing legacy
  representation. Migration records `migration_schema54_exact_garrison_patrol`
  and deliberately creates no exact operation, roster, batch, group, route,
  casualty, or settlement identity for historical rows. A pre-schema-54 save
  that somehow contains an exact garrison-patrol operation is unsupported and is
  retained as quarantine evidence rather than executed or converted to an
  aggregate count.

- A current-schema policy-v2 graph must validate unique quote/manifest/garrison/
  operation/batch/group/route identity, exact policy and hash, member-only
  casualty tombstones, legal lifecycle state, and terminal receipt. Coherent
  open physical-shaped state clears process-local handles and resumes as one
  held virtual survivor roster with the same local route cursor. Malformed,
  conflicting, non-unique, or unsupported current authority becomes contract
  version `-54`; it cannot fall into legacy activation, invent a casualty, or
  refund the purchase. Capture remains deferred while adapter or PhysicalWar
  runtime ownership survives, after which process-local residue may be cleared
  while the diagnostic graph remains durable.

- Persistence defers capture while an exact patrol is still materializing so a
  partial, not-yet-handed-off spawn wave cannot become durable or resurrect an
  unrecorded death. Once fully physical or dematerializing, persistence
  reconciles member deaths, exact binding cardinality, and live position before
  capture. An unexplained missing or aliased binding defers the checkpoint
  before stale tracked state is flushed.
  One roster-authoritative operation marker follows the virtual/live patrol and
  reports location, owner, role, and survivor count; Forces UI reports exact
  patrol infantry separately from legacy aggregate infantry. Nine deterministic
  `garrison_patrol.*` assertions cover admission, replay/rollback, roster
  projection, route loop, projection hold, settlement, restore, corruption, and
  marker lifecycle. These fixtures are source evidence. The stamped Schema-54
  tree identifies implementation `09a1470a4c27dbef866e8cbdba182a7df65fa027`,
  clean Workbench Game compile/create at 5,760 files/11,560 classes with CRC
  `c62de929`, and a ten-sample/20-second normal WorldEditor open. Packaged native
  waypoint movement,
  casualty observation, fold/rematerialization, save/restart, marker rendering,
  network replication, reconnect, and JIP proof remain open.

## Schema 53

- Schema 53 adds one canonical operation consumer only: an enemy `PATROL` order
  newly queued under the current schema. Its order contract version is `1`; the
  reciprocal operation type is `HST_OPERATION_TYPE_ENEMY_PATROL`. Admission
  freezes one executable infantry group root and every ordered member slot,
  links one generated route/order/operation/manifest/held-batch/active-group
  aggregate, and records the exact proactive attack-resource debit.

- The operation uses the generic generated-route cursor fields on
  `HST_OperationRecordState`: ordered waypoint index, lap count, leg sequence,
  loop start/completion seconds, route contract hash, strategic position, and
  bounded update clock. Outbound travel reaches route index zero, one closed lap
  advances through the ordered positions and returns to index zero, and the
  return-to-origin leg is represented separately with waypoint index `-1`.
  Virtual ticks advance that same cursor; physical fold rebases the current leg
  from the authoritative live position without changing aggregate identity.

- Exact patrols reuse the one-root infantry SpawnQueue/adapter boundary. The
  held member slots remain the living/casualty authority while virtual,
  materializing, physical, folded, and restored. A mapped physical casualty
  retires only that slot. Leaving the render bubble folds the surviving roster
  and restarts the current generated-route leg; re-entry realizes only survivors.
  Physical contact records a hold and consumes the route clock until a clear
  transition restarts the frozen current leg, so delayed virtual catch-up cannot
  skip time spent in contact.
  Every physical tick, fold, checkpoint, and campaign-stop settlement first
  proves a unique root, a unique live handle for each durable survivor, exact
  result/projection/slot ownership, non-aliased entity identity, and matching
  PhysicalWar membership. An unexplained deleted binding remains unresolved and
  cannot be converted into a casualty, refund, fold, or saved survivor.

- Resource ownership is distinct from the defensive-QRF ledger. Patrol admission
  spends proactive attack resources once. A post-debit admission rejection
  refunds the full debit once; after commitment, return or terminal settlement
  refunds only the integer survivor fraction once. Replays reuse the persisted
  settlement identity and cannot debit or refund twice. Fold/materialization
  never owns a refund.

- Enemy-order runtime dispatch now uses both order type and contract version.
  Exact patrol version `1`, exact defensive-QRF version `1`, legacy version `0`,
  quarantine, and unsupported nonzero rows have distinct owners. A current-
  schema patrol whose route, backlinks, roster, lifecycle, or settlement receipt
  is incomplete/conflicting is retained as diagnostic evidence under contract
  version `-53`; it never falls back to the legacy patrol timer or another exact
  owner. Admission scans secondary claimant identities and rollback removes only
  rows inserted by that attempt, so a foreign collision is retained unchanged.

- Every patrol restored from schema 52 or earlier remains contract version `0`.
  Migration records `migration_schema53_enemy_patrol_authority` with transition
  `legacy_enemy_patrols_preserved_contract_zero`, but does not invent a route,
  roster, operation, debit, refund, batch, or group backlink for a historical
  order. Current-schema exact rows must validate as one unique reciprocal graph.
  A coherent open row clears process-local entity/native-group mappings and
  resumes as one held strategic projection with the same cursor and casualty
  roster; a coherent settled row retains its receipt while runtime handles are
  retired.

- Before any real save-data capture, persistence reconciles mapped physical
  and dematerializing exact-patrol casualties, proves the complete adapter/
  PhysicalWar binding graph, and requires an authoritative live patrol position.
  A conflicting/missing member mapping, runtime claimant, or unverifiable live
  position defers capture before a stale tracked snapshot is flushed or a
  savepoint is requested. Pending save intent is retained for bounded retry.
  Quarantined open or settled graphs remain unsaveable while any adapter or
  PhysicalWar runtime owner exists; after both registries are empty, only process-
  local residue is normalized and all diagnostic rows remain without refund.

- One exact-patrol marker follows the persisted virtual cursor or live physical
  position and reports the durable living roster. It is removed after terminal
  cleanup. Ten deterministic `enemy_patrol.*` assertions cover admission,
  collision-safe replay/refund, route loop/return, queue/roster bookkeeping,
  contact transition, settlement, physical-shaped restore, backlink corruption,
  type/version/priority isolation, and marker lifecycle. These fixtures and the
  final stamped implementation `8ab694fdf61e56c6a5e343782f2225660d3aeeb7`, clean headless Workbench
  compile/create at 5,757 files/11,550 classes with CRC `3232b15a`, plus a normal
  WorldEditor open that remained alive
  for ten samples over 20 seconds without a crash signature, are source/Workbench
  evidence only; packaged movement, contact, fold/reprojection, accounting,
  marker rendering, and process-restart proof remain open.

## Schema 52

- Schema 52 adds one new canonical operation consumer only: a convoy mission
  newly started under the current schema. Its `HST_ActiveMissionState` stores
  operation contract version `1` plus reciprocal operation, manifest, held
  spawn-result, and settlement IDs. The linked operation has type
  `HST_OPERATION_TYPE_MISSION_CONVOY` and keeps immutable mission/source/target
  identity, a persisted generated-road-route cursor, strategic/live position
  authority, materialization state, arrival samples, and terminal settlement.

- Admission is all-or-nothing. It freezes exactly three vehicle slots, three
  linked crew-group slots, every ordered crew-member slot, and at most one
  mission cargo/captive asset assigned to vehicle slot zero. A held roster
  batch preserves stable living/casualty slots while the convoy is virtual; it
  is not generic SpawnQueue permission to realize arbitrary vehicle or
  multi-root manifests.
  Failed admission resets its uncommitted contract and matching asset links;
  only a complete reciprocal version-1 authority graph is retained. Generic
  queue restore, cancellation, duplicate handling, and compaction exclude this
  held exact roster.
  Money and supplies require exactly one cargo row, prisoners require exactly
  one captive row, and ammo, armored, and reinforcement convoys forbid a
  separate cargo row. Admission rejects duplicate or incompatible role/kind
  rows and prefabs that do not resolve to a loadable mission-asset entity
  source. Captives must be boardable character prefabs with compartment access;
  ordinary payloads must be non-character entities. Current-schema restore
  reapplies this same contract and quarantines older malformed exact rows.

- Three `HST_ConvoyElementState` rows are the durable per-vehicle authority.
  Each row links the operation, mission, manifest vehicle slot, crew group,
  mission vehicle asset, optional cargo asset, and active-group ID. It preserves
  formation/current position, original and surviving crew count, vehicle
  damage/fuel/ammunition snapshot, physical/mobile flags, disposition, terminal
  reason, update time, and revision. Active groups and mission assets receive
  reciprocal convoy-element/vehicle-slot/assigned-carrier links so array order
  is no longer identity.

- The exact convoy advances virtually along its generated route, materializes
  near a player through the convoy-specific PhysicalWar path, and folds back
  only when clear of contact, player occupancy, cargo interaction, and pending
  population/seating work. Folding samples the latest provable route position,
  per-element state, and crew survivors without applying a casualty or mission
  outcome. The transfer preflights every root before mutating any of them.
  Confirmed dead member slots never return on rematerialization: exact slot-to-
  entity mappings retain a dead early seat while later seats and their frozen
  prefabs survive, instead of inferring casualties from an aggregate suffix. If a destroyed
  or captured vehicle still has living dismounted crew, only those exact crew
  slots reproject at the element position; the terminal vehicle stays absent.
  Bubble distance uses every separated living/recoverable root. Native
  population/defer zeros are not casualties; roster reduction requires a
  completed physical sample or durable elimination. A last-crew death during
  fold reclassifies the root crewless, and a terminal damage sample commits
  destruction instead of leaving an unusable abandoned vehicle.

- Arrival is no longer resolved by the legacy convoy timer for contract-version-
  1 missions. Virtual arrival follows the persisted route cursor; physical
  arrival requires two samples from distinct campaign seconds within the route-
  end radius. Existing mission-convoy outcome logic remains the once-only owner
  of reward/penalty/capture/delivery consequences, after which the operation
  records one typed settlement and retires its held roster.

- Crew elimination does not close an exact operation while its mission still
  owns unresolved recoverable cargo, captives, or vehicles. The operation enters
  an on-station recovery hold at the durable asset anchor, remains eligible for
  virtual fold/rematerialization, and settles only after the mission-specific
  delivery/capture/extraction outcome or another terminal mission result.

- Intact crewless vehicles persist as `ABANDONED` elements and can materialize
  or be captured without recreating crew while other convoy roots remain active.
  Their runtime entities stay unpublished until the whole projection becomes
  physical. Cargo/captives keep their frozen assigned-vehicle
  slot; a destroyed or captured carrier leaves the unresolved asset at its
  durable ground position for MissionRuntime recovery rather than reassigning
  it to another vehicle. Recovery proximity also considers secondary abandoned
  vehicles and detached cargo, not only one aggregate anchor. Exact vehicle
  capture reserves one garage row, unregisters the authoritative PhysicalWar
  vehicle handle, first detaches any frozen cargo/captive child, and deletes that
  handed-off world entity so a garage copy cannot coexist with the captured
  runtime root. Terminal ground cargo alone can materialize a recovery hold when
  no intact vehicle remains. Clean player-bound cargo no longer pins the convoy
  physical during delivery.

- Restore clears process-local vehicle/group handles and physical arrival
  samples, retains route progress, convoy-element state, cargo assignment, and
  exact crew casualty tombstones, then normalizes a coherent open operation to
  one virtual projection. Member-slot tombstones plus convoy-element survivors
  are canonical; derived active-group survivor counters are rebound only after
  the aggregate validates uniquely. Conflicting or incomplete current-schema
  authority fails closed instead of regenerating a partial convoy or falling
  back to the legacy timer. Disposition must agree with linked vehicle terminal
  facts. Quarantined version `-52` rows bypass generic convoy asset creation and
  repositioning until failed-mission cleanup, preserving corruption evidence.
  Casualty tombstones are legal only on member slots; open group, vehicle, and
  cargo roots remain registered noncasualty lifecycle authority. Open records
  must use an enumerated duty/resume and materialization/position pair.
  Missionless or partially unlinked exact-looking rows remain durable quarantine
  evidence after process-local cleanup instead of being deleted as generic
  inactive groups.
  Settled exact rows remain durable after runtime retirement, but one shared
  operational/combat-presence policy excludes settled and quarantined roots
  from generic spawning, survivor polling, zone pressure, threat, undercover,
  clear-area, spawn-placement, deterministic gameplay counts, and current-group
  UI. Stale process flags clear once after restore and do not churn revisions on
  later ticks.

- The route cursor owns outbound ETA both virtually and physically; legacy
  counter C remains disabled for exact convoys after staging. Exact recovery
  retains the aggregate current marker and suppresses legacy per-vehicle and
  separate outcome-marker identities while keeping one truthful aggregate
  destination for cargo or vehicle recovery. Supply cargo remains pending after
  crew elimination until its delivery/destruction outcome resolves.

- Every historical convoy mission restored from schema 51 or earlier remains
  operation contract version `0`. Migration deliberately does not invent a
  route, operation, manifest, held batch, vehicle slot, crew roster, cargo
  carrier, element row, or settlement receipt for it; the prior convoy runtime
  path continues to own those records.

- Before any real save-data capture, persistence synchronously reconciles each
  mapped physical exact-convoy member. An open outbound publication transaction,
  missing/conflicting mapping, or nonphysical operation retaining member
  mappings defers capture before an older snapshot is flushed or an engine
  savepoint is requested. Autosave/major-change intent remains pending and
  retries on the bounded debounce.

- The final stamped schema-52 tree identifies implementation
  `fa5e7e45dbd8741269e614e60c51d4edee6bf223`, passes repository validation, and
  passes a clean headless Workbench Game-module compile/create gate at 5,753 files/
  11,537 classes with CRC `e868739b`. A normal WorldEditor open created the same
  Game module and remained responsive for all 10 bounded samples without a
  script-error or native-crash signature. That is source/startup evidence, not
  packaged-runtime evidence.
  A republished server/client run must still prove three physical vehicles and
  crews, virtual travel, interception,
  casualty-preserving fold/rematerialization, arrival/outcome settlement,
  marker cleanup, and real process restart.

## Schema 51

- Schema 51 adds the durable authority kernel for one additional consumer only:
  newly admitted, exact infantry-only enemy defensive QRF orders of type
  `HST_ENEMY_ORDER_QRF`. A versioned order stores its immutable source zone,
  frozen manifest hash, operation/manifest/spawn/group backlinks, strategic
  service commit boundary, and an idempotent resource-settlement identity with
  accepted and surviving member counts. The reciprocal operation type and
  enemy-order backlink are explicit; the reciprocal active-group enemy-order
  backlink prevents restore normalization from reclassifying the projection as
  a garrison.

- The operation reuses schema 50's exact single-root infantry roster, direct
  route, virtual/physical materialization, and strategic position contracts.
  Enemy-QRF operation transitions add an on-station to return-origin success
  leg and typed `COMPLETED` terminal result. Physical arrival and physical
  return require two live-position confirmations from distinct campaign
  seconds. Those counters persist, reset whenever the route leg changes, and
  reset on restore so one process-local sample cannot be replayed after restart.
  The player-paid QRF settlement contract does not accept `COMPLETED`.

- Every pre-schema-51 enemy order is preserved at operation contract version
  `0`. Migration creates no source-zone claim, frozen roster, operation record,
  service commitment, resource settlement, or refund for legacy rows. It does
  not opt counterattacks, Petros attacks, roadblocks, patrols, rebuild orders,
  support calls, vehicle manifests, asset manifests, or multi-root forces into
  the new contract. A current-schema versioned row with incomplete or
  conflicting authority fails closed instead of falling back to the legacy
  timer or physical-support consumer; its evidence remains available for the
  runtime settlement owner.

## Schema 50

- Schema 50 advances the first canonical operation consumer—confirmed exact
  player-paid resistance infantry QRFs—from a lifecycle kernel to persistent
  strategic projection. The operation now stores a versioned direct-route
  cursor, route distance, conservative 2.5 m/s campaign speed, strategic
  position, bounded update clock, materialization decision evidence, and
  deterministic virtual-combat clock and damage carries. Quote ETA is derived
  from route distance and that speed. Strategic ticks catch up at no more than
  30 campaign seconds per invocation; no nearby player is required.

- The exact spawn batch can be held strategically. While held, its frozen
  manifest member slots are the exact living/dead roster and the physical spawn
  queue performs no work. Entering the configured player bubble releases the
  batch at the current route cursor. Leaving the larger materialize-out radius
  while clear of contact samples the live position and roster, retires only the
  disposable runtime projection, and requeues exactly the confirmed survivors.
  The fold boundary rebases the remaining direct route at that live position and
  resets the virtual-combat clock, so physical time is never replayed as abstract
  casualties. Abstract virtual contact is legally cleared during the v1 physical
  handoff until a physical contact adapter becomes authoritative.
  Projection deletion is neither a casualty nor a refund. Vehicles, cargo,
  assets, multi-root forces, and other support families deliberately fail closed
  or remain on their previous contracts.

- Virtual combat is initially limited to an on-station exact infantry QRF and
  the hostile abstract infantry garrison at its target. It advances in bounded
  30-second deterministic power steps, can retire at most one exact friendly
  member slot per processed step, and preserves its fractional damage carries.
  The contact clock resets on outbound-to-on-station arrival, so elapsed travel
  time cannot be backfilled as combat. Broader encounters, vehicle damage,
  ammunition, terrain modifiers, and capture outcomes remain future work.

- Pre-schema-50 coherent exact infantry-QRF operations receive this projection
  contract without changing economy or terminal history. Unsupported manifests
  remain operation-only. Every nonterminal projection restores as virtual with
  strategic position authority; pending, interrupted, ready, and previously
  successful physical batches become one held pending survivor projection with
  process-local IDs cleared. A successful physical batch records exactly one
  reprojection on that transition; already-held pending work does not. The saved
  live group position rebases the restored route before initialization, preventing
  a snap back to an older cursor. This prevents restart from duplicating physical
  entities. The linked support request is normalized at the same time:
  `m_bPhysicalized` is cleared, on-station duty publishes
  `exact_virtual_on_station`, and other open duty publishes
  `exact_restore_survivor_virtual`. Schema 50 also reclassifies the known woodland minor locality as a
  food resource and reduces its ambient civilian baseline while preserving any
  existing casualty ledger.

- Route preparation does not itself deliver the paid service. The operation may
  initialize its route while `STAGING`; `LinkOutboundVirtual` is the commit that
  advances it to `OUTBOUND`. Terminal failure before that commit refunds money
  and HR. Once committed, including `ON_STATION`, a materialization failure with
  no physical handoff retains money and refunds only
  `m_iLastVirtualFriendlyCount` HR. Both terminal paths are deterministic and
  idempotent.

- The same source boundary separates HQ clearances that previously shared one
  900-meter constant. Hostile operation staging retains that distance, location
  activation now uses the location capture footprint with a 150-meter fallback,
  and generated composition slots keep a 150-meter immediate clearance. This
  allows valid nearby locations to project without spawning entities on HQ.

- Schema 49 adds the first canonical `HST_OperationRecordState` contract for one
  already-exact consumer: confirmed player-paid resistance infantry QRFs. Quote
  issue retains the stable operation identity without creating an operation
  record. Successful confirmation creates exactly one version-1 record with
  immutable origin and assignment, mutable tactical target, duty, engagement,
  materialization, position-authority, settlement, terminal-result, policy,
  timing, and revision fields. Queue admission, physical handoff, arrival,
  restore reprojection, recall, and terminal settlement update that record
  through `HST_OperationService`; settlement replay is idempotent and a
  conflicting terminal result fails closed.

- Schema-48 migration backfills only uniquely coherent, accepted, nonterminal
  exact paid-QRF requests whose quote, manifest, and unique committed money/HR
  transaction authority agree, with optional queue/group links required to be
  unique and coherent when present. It changes
  no balance, ledger amount/status, request status, quote status, or manifest.
  Pre-exact requests, terminal history, ambiguous/incomplete rows, archived-only
  rows, and legacy `HST_QRFState` records remain operation contract version `0`
  rather than receiving invented authority. One bounded migration event records
  the created count and one conflict event records preserved candidates.

- Restored open operations never reacquire process-local physical authority.
  Saved `PHYSICAL` or `DEMATERIALIZING` records normalize to
  `MATERIALIZING` plus strategic position authority while retaining immutable
  assignment, current duty, and revision history until the exact adapter
  completes survivor reprojection. Once an accepted contract-version-1 QRF is
  terminal and backlink-free, schema-48 settlement archival additionally copies
  its operation contract version, settlement ID, revision, and typed terminal
  result into the compact tombstone and removes the full settled operation row.
  Packaged save/restart, migration, and archive replay remain runtime-unproven.

- Schema 48 adds bounded settlement archives for accepted exact-force planning
  history. An accepted garrison purchase or terminal paid QRF remains in full
  quote/manifest/ledger form for at least 600 campaign seconds and cannot compact
  while an active-group, enemy-order, or force-spawn backlink exists. Eligible
  rows compact atomically into one persisted tombstone containing quote,
  operation, actor, manifest-hash, aggregate, cost, refund, and transaction-status
  evidence. Issue, confirmation, and committed-ledger replay consult that compact
  row before any mutation or debit. Tombstones retain a minimum 86,400-second
  replay window, cap at 256 rows, and share a hard 320-row planning-history
  admission bound with full quotes. Pin-aware queue compaction now runs in the
  production coordinator so 128 retained terminal projections cannot deadlock
  later admission or permanently pin a settled manifest. Pre-schema-48 saves
  preserve every accepted row in full and initialize an empty archive; migration
  never invents a terminal settlement.

- The schema-48 support route-truth repair required no save-format bump,
  but it does reconcile pre-repair timer-derived states. An unspawned restored
  `support_arrived` row folds when it remains outside the player bubble or waits
  for physicalization inside it. A spawned arrived row without the new request-
  and-group proof provenance receives one current live-distance check, then is
  either stamped as proven or reopened as `support_active`. Spawned
  `support_recall_exited` likewise requires current live exit distance before
  retirement or HR settlement; eligible unspawned abstract recall remains valid.
  A succeeded exact QRF recalled while survivor reprojection is waiting for queue
  capacity now settles its durable survivors without inventing a physical exit,
  and legacy rows already stuck in unspawned `support_recalling` recover through
  the same once-only HR settlement when no root or adapter handles remain.

- The typed support-recall command repair also requires no save-format bump.
  `HST_SupportRecallResult` is transient; the existing support request,
  operation ID, command receipt, and ledger rows already persist the durable
  facts. New recall receipts derive applied/rejected status from the typed
  result and schedule persistence explicitly. Existing historical receipts are
  not reclassified. Exact paired full refunds now prevalidate both linked
  transaction identities, coherent refund state, settleability, and
  deterministic settlement IDs before either transaction mutates.

- Schema 47 adds the first durable exact-force runtime lifecycle for the paid
  infantry QRF path. Each successfully handed-off member slot now preserves
  ever-alive, casualty, retirement, timestamp, and revision evidence. The
  adapter samples authoritative life state before pruning transient handles,
  retires each dead slot idempotently, detaches the corpse from native/Game
  Master group ownership without deleting it, and updates the active group's
  durable living count. Once an ever-populated, spawn-completed roster reaches
  zero, the exact root and marker are retired and the support resolves without
  refunding dead HR or the already-delivered money cost. Successful paid-QRF
  restore clears process-local IDs and requeues one root plus only the durable
  surviving member slots; confirmed casualties remain retired. Reprojection
  failure retains the paid money and refunds only durable survivors. Pre-schema-
  47 successful batches gain one historical handoff plus ever-populated/living
  evidence from their registered slots. Migration never infers a casualty from
  a missing entity or aggregate count.

- Schema 46 adds the first exact player-paid support contract for resistance
  QRFs. Force quotes now persist support request/capability/asset identity,
  support type, ETA, cooldown, and expected war level beside the immutable
  manifest and transaction links. Confirmation reserves and commits the exact
  $250 money cost and one HR per authored catalog member, registers one linked
  support request, and marks the quote accepted last. The request then admits
  the same executable one-group manifest to SpawnQueue; terminal failure or
  cancellation refunds both transactions once, while recall uses the linked HR
  settlement policy. In schema 46, successful terminal projections that could
  not be restored failed closed with a full refund instead of inventing live
  runtime authority. Pre-schema-46 paid player-QRF rows keep their gameplay state and
  balances. Positive stored costs import as historical committed ledger rows
  without creating a quote, manifest, or new debit; stored HR refunds become
  historical partial/full refunds. Conflicting legacy transaction identity is
  preserved and summarized by a bounded migration warning rather than receiving
  an invented refund. Schema 47 supersedes schema 46's temporary full-refund
  fallback for successful paid-QRF restore with survivor-only reprojection.
- Schema 45 adds explicit force and projection identity fields to every active
  physical-group row and makes Game Master/editable-hierarchy verification
  mandatory for new successful spawn slots. Pre-schema-45 active groups derive
  missing IDs only from one spawn batch whose result, manifest, operation, and
  force links
  are conflict-free; ambiguous or missing links remain empty. Stable aggregate
  migration events record derived and unresolved linked rows without creating
  one event per group. Existing terminal spawn history remains historical even
  when it predates the new verification field. Cleanup work is acquired in
  dependency-safe asset, member, vehicle, then group order across bounded work
  waves. Once every exact slot is registered, the batch enters the durable,
  nonterminal `READY_FOR_HANDOFF` state. Physical-war finalization must succeed
  before `CompleteProjectionHandoff` records `SUCCEEDED`. Restoring a ready batch
  clears process-local evidence, advances its generation, and requeues exact
  realization; it never promotes an interrupted handoff to terminal success.
  Existing terminal successes remain historical and are not yet runtime-
  reprojected. Normal acquisition uses active campaign time, while setup and won/
  lost processing cancels nonterminal work and drains cleanup through a monotonic
  runtime-only clock without advancing the persisted campaign elapsed second.
- Schema 44 turns typed force-spawn results into durable, per-projection queue
  records. Each batch carries a unique request ID, immutable manifest-hash
  binding, projection ID, attempt generation, retry/deadline timestamps,
  cancellation intent, and explicit failure evidence. Each slot also preserves
  its spawned prefab and alive verification. Runtime queue limits are 64
  nonterminal batches, 512 nonterminal slots, 64 slots per request, and 128
  terminal rows; terminal compaction requires explicit backlink pins and a
  600-second minimum retention window. These queue bounds did not compact
  accepted quote/manifest/ledger settlement history until schema 48 added a
  separate accepted-aggregate archive.
  Pre-schema-44 nonterminal rows are never resumable, even if a partially
  populated save happens to contain newer identity fields, so migration
  finalizes them as explicit failures and records one migration event instead
  of inventing work. Existing terminal batches remain unchanged as historical
  evidence. A schema-44 nonterminal batch whose nonempty result, request, or
  projection key collides with any other row fails closed and emits one bounded
  normalization record. Campaign state also persists an actual-restore sequence
  and the last queue-reconciled sequence. Coordinator initialization reconciles
  once per actual restore before other authority recovery; nonterminal rows
  clear process-local evidence and advance generation, while terminal status,
  prefab, and verification history remain but entity/native-group IDs are
  cleared rather than reacquired as living authority.
- Schema 43 adds immutable force manifests, exact force quotes, typed spawn
  results, stable garrison IDs, and quote/manifest links on resource
  transactions and force-bearing aggregates. Legacy force counts remain
  unchanged and are explicitly recorded as unverified instead of receiving
  invented member slots, prices, or refunds. New visible garrison recruitment
  uses an expiring all-or-nothing server quote and persists its exact member
  slots and per-member money/HR cost before confirmation.
- Schema 42 adds the campaign authority foundation: a persisted monotonic ID
  sequence, stable operation links, bounded command receipts, resource
  transactions, and bounded campaign transition events. Legacy support,
  enemy-order, and active-group rows receive deterministic operation IDs from
  their existing durable IDs. Legacy resource changes are not retroactively
  invented as transactions. During coordinator initialization, any restored open
  reservation is cancelled and refunded once before normal service ticks; the
  reconciled state is then captured and tracked again.
- Schema 41 extends support request rows with selected roadblock garage vehicle
  id, prefab, display name, and consumed-state fields so established roadblock
  support survives save-data roundtrips with its consumed HQ vehicle evidence.
- Schema 40 adds durable gun shop mission state: generated shop item rows,
  seller/delivery asset ids, seller/delivery positions, purchase totals,
  purchase/delivery notice flags, and delivery runtime timing so purchased
  stock and delivery progress survive save-data copies.
- Schema 39 extends support request rows with player-support HR cost,
  planned infantry count, refunded HR, recall request timing, recall exit
  position, and recall-requested state so support recall/refund behavior
  survives save-data copies and active support rows remain inspectable.
- Schema 38 extends strategic-event rows with vehicle report before/after
  fields so runtime vehicle reports preserve vehicle runtime id, heat
  before/after/delta, reported before/after, and report-expiry deltas across
  save-data roundtrips.
- Schema 37 adds the durable strategic-event ledger so mission success/failure,
  mission-expiry, convoy-outcome, zone-capture, and support-near-HQ consequence
  rows preserve source identity, target zone/faction, applied status,
  before/after owner fields, and money/HR/support/capture/aggression/resource/
  HQ-knowledge deltas across save-data roundtrips.
- Schema 36 adds the selected active-group vehicle prefab so mixed
  infantry/vehicle response groups can respawn the same vehicle choice after a
  save/load roundtrip.
- Schema 35 adds persisted support deployment proof for physicalized support:
  deployment route id, placement type/summary, target/road/HQ distances,
  road resolution, vehicle-safe result, and whether vehicle-safe placement was
  required.
- Schema 34 adds persisted population-outcome metadata for campaign-end state:
  outcome mode, initial/remaining/killed population, FIA-supporting population,
  support percent, and controlled/total airfields.
- Schema 33 adds durable active-group source-link fields and original force
  count fields so support, mission, QRF, and garrison active groups can be
  traced through fold-back, cleanup, refunds, and save-data roundtrips.
- Schema 32 extends durable vehicle heat/report fields to garage vehicles so
  reported vehicle cover state survives capture, virtual storage, redeploy, and
  save-data roundtrips.
- Schema 31 adds durable runtime vehicle heat/report fields so reported
  civilian vehicles, passenger compromises, report expiry, and vehicle-cover
  eligibility remain inspectable across saves.
- Schema 30 adds durable town influence events plus town population and
  influence aggregate fields so aid, civilian casualties, security pressure,
  support-majority flips, and active/expired modifiers remain inspectable
  across saves.
- Schema 29 adds durable enemy support-spend ledgers and enemy-order refund
  stamps so QRF/support cooldowns, recent damage pressure, max-spend denials,
  and survivor fold-back refunds remain inspectable across saves.
- Schema 28 adds durable force-composition metadata to active groups, support
  requests, and enemy orders. Existing saves load with empty composition
  summaries and zero composition counts until the next force-planning event.
- Schema 27 adds a durable display-only player name field so member/commander
  roster UI can show readable player names while backend identity remains the
  authority for permissions and ownership.
- Schema 26 adds durable HQ spawn-point position/prefab fields and backfills
  older deployed HQ saves so the HQ runtime object rebuild can spawn and
  verify a physical respawn marker.
- Phase 24 balance, pacing, and campaign outcomes add durable campaign-end report fields and backfill legacy ended saves.
- Phase 23 UI and marker polish does not require a campaign schema bump because marker audits, command coverage, failed-action text, and menu summaries are derived from existing persisted state.
- Phase 22 HQ threat and Defend Petros add durable HQ threat diagnostics and active defense mission/order/support/group links.
- Phase 21 undercover enforcement and police/roadblocks add durable applied/enforcement state, detection score/source, compromise reason, and police/roadblock scan audit fields.
- Phase 20 civilians, town support, and undercover reports add durable town support values and detailed undercover eligibility reasons.
- Phase 19 support request lifecycle hardening adds durable support-request
  runtime fields while preserving runtime handles as non-persisted data.
- Phase 18 enemy commander physical responses add durable enemy-order
  runtime fields while preserving runtime handles as non-persisted data.
- The current save container captures campaign metadata, elapsed/save/restore
  counters, war resources, campaign-end state, HQ/Petros/cache/arsenal/tent/spawn-point
  fields, HQ threat/Defend Petros state, faction pools, players, zones,
  garrisons, active groups, QRFs, map markers, arsenal items, garage vehicles,
  vehicle cargo, runtime vehicles, saved loadouts, issued loadout items,
  captured emplacements, ammo points, active missions, generated sites/routes,
  mission objectives/runtime entities/assets, support requests, enemy orders,
  enemy support ledgers, civilian state, town influence events, strategic
  events, canonical exact-QRF operation records, immutable force manifests,
  force quotes, typed spawn results,
  undercover state, and campaign tasks.
- `HST_LoadoutEditorSessionState` records are runtime/editor state and are not
  copied into `HST_CampaignSaveData`; durable saved loadouts and issued-item
  ledgers are copied, and personal templates are also written under
  `$profile:h-istasi/loadouts/v2` with loadout file schema `2`.
- Runtime settings are schema `22` and are migrated separately by
  `HST_RuntimeSettingsService`.
- Campaign save data is normally tracked through `PersistenceSystem`; when
  scripted persistence cannot flush, the current same-container data can be
  written to and restored from `$profile:h-istasi/HST_CampaignSaveData.json`.
- Raw `IEntity`, `AIGroup`, waypoint, inventory-operation callback, and other
  runtime handles are not persisted as campaign truth.

## Schema 42

Campaign command authority and resource transaction foundation.

- `HST_CampaignState.SCHEMA_VERSION` is `42`.
- `m_iNextAuthoritySequence` allocates server-owned event and fallback request
  IDs without relying on elapsed time plus array length.
- `HST_CommandReceiptState` stores the request, actor, command, argument,
  typed terminal status, result, aggregate link, and receive/complete time.
- `HST_ResourceTransactionState` stores reservation/commit/refund/cancel
  status, exact amount, refunded amount, operation, command, actor, and stable
  settlement identity.
- `HST_CampaignEventState` records bounded command and resource transitions.
- Support requests, enemy orders, and active groups now carry a stable
  operation ID. Existing rows backfill that ID from their durable source ID.
- The first production ledger consumer is resistance training. Replaying one
  command request or transaction ID cannot spend money or increase training a
  second time.
- Save capture/restore deep-copies receipts, transactions, events, allocator
  state, and operation links. Restored reservations that never reached a
  terminal state are cancelled and refunded before normal campaign ticking.

## Schema 43

Exact force-planning authority foundation.

- `HST_ForceManifestState` freezes operation identity, faction, catalog and
  policy versions, exact ordered member/group/vehicle/asset slots, deterministic
  seed, and exact resource totals behind a stable hash.
- `HST_ForceQuoteState` stores actor, target, context hash, expiry, exact costs,
  all-or-nothing policy, status, and deterministic transaction links.
- `HST_ForceSpawnResultState` and per-slot results can encode queued,
  registered, deferred, retryable/final failure, and cancellation outcomes
  without treating an empty group root as success. Production garrison
  physicalization does not yet write or consume these queue results.
- Visible garrison recruitment first issues an exact server quote. A separate
  confirmation submits only the quote ID, revalidates the frozen context,
  reserves money and HR, registers the exact purchase-time aggregate increment
  plus acceptance provenance, verifies the delta, then commits both ledger rows.
  Restore reconciliation rolls back any `ISSUED` quote found with partial linked
  reservations, commits, or aggregate delivery.
- Schema 43 does not claim a living-slot roster: broad-alpha garrison
  physicalization does not yet consume the frozen member slots. Accepted
  settlement history and typed spawn-result history also await explicit bounded
  archive/retention policy.
- Schema-42 and older garrisons receive deterministic IDs. Existing aggregate
  force counts are preserved without synthetic manifests or retroactive
  accounting, and one bounded migration event records that limitation.

## Schema 44

Durable bounded spawn-queue state foundation.

- Every materialization attempt is keyed by its own spawn request and projection
  IDs. Manifest identity binds immutable input but is not the unique lookup or
  idempotency key because one manifest may be projected more than once.
- A batch persists the manifest hash, attempt generation, last/next attempt and
  update times, deadline, cancellation request, last failure, cleanup-pending
  state, and typed terminal result.
- A slot persists spawning/cleanup-pending states, the prefab actually passed to
  physical creation, its entity/group/projection links, and faction, group,
  projection, seat, and alive verification evidence.
- Save capture and restore deep-copy all queue fields and slot evidence.
- Campaign state persists an actual-restore sequence and the last restore
  sequence reconciled by the spawn queue. Applying a persisted campaign advances
  the actual-restore sequence exactly once before coordinator reconciliation;
  ordinary state normalization and new campaigns leave it at zero.
- Pre-schema-44 terminal success, final-failure, and cancellation batches are
  retained without synthetic request IDs or hashes. Every pre-schema-44 pending,
  deferred, retryable, in-progress, or cleanup-pending batch becomes an explicit
  final failure regardless of any partially populated newer identity fields.
  Every slot in such a batch has stale runtime entity and verification evidence
  cleared; existing final-failure and cancellation status/reason is preserved
  while registered and every other status becomes a final failure. Batch and
  slot migration timestamps are stamped with the exact migration second. One
  migration event records the conversion only when at least one row changed.
- On schema-44 restore, duplicate nonempty result, request, or projection keys
  invalidate every nonterminal batch carrying a conflicting key, including when
  the other key belongs to terminal history. Conflicting nonterminal rows fail
  closed with runtime group/entity evidence cleared; terminal historical rows
  remain unchanged. One stable event row provides bounded normalization evidence.
- Queue retention and active-work limits are enforced by the queue service.
  Migration never deletes valid terminal or settlement evidence merely to meet
  a cap.

## Schema 49

Canonical operation authority for the confirmed exact paid infantry-QRF slice.

- `HST_CampaignState` and `HST_CampaignSaveData` persist
  `HST_OperationRecordState` rows independently from legacy `HST_QRFState` rows.
  Each record binds one stable operation to its support request, accepted quote,
  immutable manifest, force-spawn result, projection, force, and active group as
  those execution identities become available.
- Quote issue allocates and persists operation identity in the existing planning
  aggregate but creates no `OperationRecord`. Successful exact confirmation opts
  the support request into operation contract version `1` and registers exactly
  one open record. Confirmation rollback may remove only a still-virtual,
  uncommitted record with no spawn/group authority.
- Origin and assignment remain immutable confirmation facts. Recall changes the
  tactical target toward the exit/origin without rewriting the assignment.
  Duty, engagement, materialization, position authority, and settlement are
  orthogonal typed states. Engagement exposes a legal
  `CLEAR -> CONTACT -> ENGAGED -> DISENGAGING -> CLEAR` API and preserves the
  resumable duty, but live combat-contact/disengagement detection is not wired in
  this schema.
- Queue admission records `OUTBOUND` plus `MATERIALIZING`; successful physical
  handoff records live position authority; confirmed arrival records
  `ON_STATION`; restore returns open physical records to strategic
  `MATERIALIZING`; recall records request/exiting duty; terminal ledger paths
  settle once with a typed recalled, spawn-failed, destroyed, cancelled, or
  invalidated result.
- Schema-48 and older saves receive a record only for a uniquely coherent,
  accepted, nonterminal exact paid player QRF. Migration preserves all economy,
  ledger, request, quote, manifest, queue, and group status. Every other family
  remains contract version `0`, including pre-exact player requests, terminal or
  archived-only history, ambiguous rows, enemy/legacy QRFs, and other support
  types.
- Schema-48 settlement archives remain the bounded historical container. A
  terminal contract-version-1 QRF can compact only when its full operation is
  coherently settled and existing backlink rules pass; its tombstone retains the
  operation settlement identity, contract version, revision, and typed terminal
  result while the full operation row is removed.
- `HST_OperationRecordProofService` includes focused current-schema restore,
  schema-48 coherent/ambiguous/incomplete/pre-exact/terminal migration, archive,
  confirmation replay, and legacy-QRF isolation assertions in the coordinator's
  force-authority debug case. They compile but have not yet run in a packaged
  runtime.
- This schema is not the complete operation/virtualization milestone. Strategic
  route progress, cursor/hysteresis, generalized physical/virtual transfer,
  vehicles/assets/multi-root forces, other support types, garrisons, missions,
  enemy orders, live engagement events, and client/JIP operation projection are
  still open. Packaged runtime and process-restart proof are also pending.

## Schema 48

Bounded accepted-settlement history and replay authority.

- Full accepted quote/manifest/transaction rows remain authoritative during a
  600-second minimum window and for the entire lifetime of every force-spawn,
  active-group, or enemy-order backlink.
- Garrison rows compact only after the exact purchase-time increment and its one
  accepted-manifest link are provable. The manifest ID then moves from the
  garrison's growing provenance list into the settlement tombstone; infantry is
  not added, removed, or recomposed.
- Paid-QRF rows compact only when the linked support request is resolved or
  cancelled and no physical/queue backlink remains. The tombstone preserves the
  final money/HR status, refunded amount, and settlement ID.
- Archived issue and confirmation requests reconstruct read-only summary objects
  and return an already-applied result. `ResourceLedgerService.ReserveCost`
  checks archived transaction IDs before `SpendResource`; exact committed replay
  succeeds without a debit, while identity conflicts and already-refunded rows
  fail closed.
- Tombstones are deep-copied by `HST_CampaignSaveData`. The oldest row can leave
  only after the 86,400-second replay window and only when the 256-row tombstone
  cap needs room. New planning fails closed at 320 combined full/tombstone rows
  when protected history cannot make room.
- Schema-47 saves receive the bounded
  `migration_schema48_force_settlement_archive` event and keep all prior accepted
  authority in full form. No quote, transaction, refund, or tombstone is derived
  merely because an old save was loaded.

## Schema 47

- `HST_CampaignState.SCHEMA_VERSION` is `47`.
- `HST_ForceSpawnSlotResultState` persists lifecycle revision, ever-alive,
  casualty-confirmed, casualty-second, and retirement-reason fields. The new
  terminal slot state `HST_FORCE_SLOT_RETIRED` is valid only for a member that
  was previously alive and has an explicitly confirmed casualty.
- `HST_ForceSpawnResultState` persists successful-handoff, reprojection, and
  lifecycle counters. A technical failure after at least one successful handoff
  is therefore distinguishable from initial deployment failure and cannot
  trigger an invented full refund.
- `HST_ActiveGroupState` persists `everPopulated`, `spawnCompleted`, durable
  living infantry, casualty/elimination seconds, and a lifecycle revision.
- Restore never reacquires an entity by its saved process-local ID. A successful
  paid-QRF batch clears those IDs, retains retired member tombstones, queues a
  new root plus only durable survivors, and records another successful handoff
  after exact verification. If no survivors remain, the support resolves as
  eliminated instead of respawning or refunding casualties.
- Runtime casualty polling accepts death only while the exact slot's entity is
  still present and authoritative life state reports it dead. A deleted or
  missing entity is not itself casualty evidence.
- Pre-schema-47 successful spawn batches receive one historical handoff.
  Registered alive slots receive ever-alive evidence, and uniquely linked
  active groups receive spawn-completed/ever-populated plus a durable living
  count. The bounded `migration_schema47_force_runtime_lifecycle` event records
  the aggregate backfill; no casualty or refund is invented.

## Schema 46

Exact player-paid QRF quote, ledger, executable manifest, and settlement
authority.

- `HST_ForceQuoteState` persists the exact support request, capability, asset
  profile, support type, ETA, cooldown, and quoted war-level context. Save
  capture/restore deep-copies every field.
- Selecting `support_qrf` with a map target issues an expiring quote only. The
  Forces tab exposes separate confirm/cancel actions; confirmation sends only
  the quote ID and the server resolves actor, target, context, catalog, and
  resources again.
- The frozen manifest contains exactly one authored infantry group execution
  root and every explicit ordered member slot. Money is a flat $250 and HR is
  derived from the exact slot count. No prefab-name manpower estimate or
  recomposition participates after issue.
- Confirmation reserves money and HR, registers and verifies one linked support
  request, commits both transactions, and changes the quote to `ACCEPTED` last.
  Replays remain idempotent after queue admission and after terminal refunds.
  Interrupted issued confirmations roll back linked aggregate and transaction
  state during restore before the generic reservation sweep.
- After ETA staging, the support service creates one queue-owned active-group
  projection with stable force/projection/result identities and submits the
  accepted manifest to SpawnQueue. The request is not physicalized until the
  queue reaches `SUCCEEDED` after physical-war handoff.
- Placement/admission failure while the operation is still `STAGING` and
  commander cancellation before strategic service commitment refund money and
  HR once and let a replacement quote bypass the historical cooldown. Once
  `LinkOutboundVirtual` commits `OUTBOUND`, money remains paid; a later
  no-handoff materialization failure refunds only the operation's last virtual
  survivor count through the HR transaction. Recall and post-handoff failure
  retain their survivor-only HR policies.
  The current no-schema-bump typed recall layer checks every settlement result,
  rejects lost-group or terminal-failure settlement conflicts explicitly, and
  preflights both legs of a full refund before either leg changes.
- Setup and terminal campaign frames continue queue cancellation/cleanup and run
  exact-support settlement even though campaign elapsed time is frozen.
- Schema 47 and schema 50 supersede the former restored-`SUCCEEDED` full-refund
  fallback. Restore now retains exact casualty history, requeues one held
  survivor projection, and normalizes the linked request to virtual authority.
- Legacy positive player-QRF cost fields are historical charge evidence only.
  Migration creates linked transaction rows without changing balances and never
  invents a quote or manifest. Ambiguous conflicts remain visible through one
  bounded warning event.

## Schema 45

Active-group projection identity and Game Master registration authority.

- `HST_ActiveGroupState` persists explicit `m_sForceId` and
  `m_sProjectionId` fields beside its operation, manifest, and spawn-result
  links. Save capture and restore deep-copy both fields.
- A pre-schema-45 active group receives missing force/projection IDs only when
  exactly one spawn batch is proven by conflict-free durable identity. A direct
  spawn-result link must agree with any stored manifest and operation links and
  with the group's force identity; without a direct result link, force,
  manifest, and operation must all match. Existing nonempty IDs are never
  overwritten.
- Linked groups with no unique provable batch stay unresolved. One stable event
  records the number of derived rows and one stable event records the number of
  unresolved linked rows, keeping migration evidence bounded.
- New spawn success requires alive, faction, native-group, Game Master,
  projection, and any applicable seat verification. The Game Master result is
  persisted, copied, replay-matched, cleared with nonterminal physical evidence,
  and included in final registered-slot readiness.
- Terminal history from older schemas is preserved without inventing Game
  Master proof. Actual restore still clears only process-local entity/native
  IDs from terminal rows while retaining their historical status, prefab, and
  recorded verification fields.
- Cleanup acquisition walks assets, members, vehicles, and group roots in that
  order. The normal per-tick action cap still applies, and ordering remains
  monotonic across successive cleanup waves.

## Runtime Settings Schema 22

True-town civilian traffic default and deterministic appearance correction.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `22`.
- Five civilian-driven traffic vehicles are the default for stock town-center
  locations. Minor localities do not inherit that town-scale traffic budget.
- A schema-21 value of `2`, the prior shipped default, migrates to `5`.
  Non-default operator values remain unchanged.
- Direct runtime civilian pools now use concrete stock appearance variants.
  The stock editor-randomized entries do not select a random default variant
  when directly spawned, which previously produced one repeated appearance per
  location.

## Runtime Settings Schema 21

Loot-setting key cleanup.

- `HST_RuntimeSettings.SCHEMA_VERSION` was `21`.
- Generated settings keep `lootSkipUnlockedItems` and
  `vehicleLootSkipUnlockedItems` as the canonical keys.
- Obsolete `lootOnlyLockedItems` and `vehicleLootOnlyLockedItems` aliases are
  no longer generated or read.
- Existing known gameplay values are preserved while the normalized settings
  file is rewritten.

## Runtime Settings Schema 20

Loot unlock defaults.

- `HST_RuntimeSettings.SCHEMA_VERSION` was `20`.
- Area and vehicle loot default to skipping already-unlimited items.
- Explosive and guided-launcher threshold unlocks default to enabled.

## Runtime Settings Schema 19

Ambient civilian traffic and generated settings comments.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `19`.
- Generated settings include JSON-safe `_comment` and `_comment_*` string
  fields that explain nearby settings.
- `civilianDrivingVehicleCountPerTown` controls how many civilian-driven
  ambient traffic vehicles can be active per active town.
- Existing settings migrate by rewriting the generated profile with comments
  and the traffic cap while preserving known gameplay values.

## Schema 41

Roadblock support garage vehicle state.

- `HST_CampaignState.SCHEMA_VERSION` is `41`.
- `HST_SupportRequestState` now stores the selected HQ garage vehicle id,
  vehicle prefab, vehicle display name, and whether the garage vehicle was
  consumed when creating a roadblock support request.
- Existing schema-40 and older support requests load with empty selected
  vehicle metadata and `m_bGarageVehicleConsumed == false`; only new
  player-requested roadblock support rows require and populate those fields.

## Schema 40

Gun shop mission stock and delivery runtime state.

- `HST_CampaignState.SCHEMA_VERSION` is `40`.
- `HST_ActiveMissionState` now stores generated gun shop item rows, seller and
  delivery asset ids, seller and delivery positions, purchase totals,
  delivery notice flags, and delivery start timing.
- Existing schema-39 and older active missions load with empty gun shop state;
  newly generated gun shop missions build their stock from the runtime arsenal
  item catalog and only start delivery after at least one purchase.

## Runtime Settings Schema 18

Generated settings comments.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `18`.
- Generated settings now include JSON-safe `_comment` and `_comment_*` string
  fields that explain nearby settings. The runtime loader ignores these fields
  and still reads only the scalar gameplay keys.
- Existing settings migrate by rewriting the generated profile with comments
  while preserving known gameplay values.

## Runtime Settings Schema 17

Generated settings hideout key removal.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `17`.
- `campaign.defaultHideoutId` is no longer generated or read. Initial HQ
  placement is selected through the setup map flow, so the profile config should
  not imply a separate default hideout source of truth.
- Existing settings migrate by rewriting the generated profile without the
  obsolete hideout key while preserving the remaining scalar settings.

## Runtime Settings Schema 16

Resistance support group marker tracking.

- `HST_RuntimeSettings.SCHEMA_VERSION` is `16`.
- `features.trackResistanceSupportGroupsOnMap` defaults to `true`.
- Existing settings migrate the feature on so spawned player-requested
  resistance support groups keep live map markers until they are terminal or
  despawned.

## Schema 39

Support request HR costs and recall/refund state.

- `HST_CampaignState.SCHEMA_VERSION` is `39`.
- `HST_SupportRequestState` now persists HR cost, planned infantry count,
  refunded HR, recall request timing, recall exit position, and whether recall
  has been requested.
- Existing schema-38 and older support rows load with recall disabled, zero
  refunded HR, and zero HR cost unless force-composition metadata can backfill
  a planned infantry count for inspection.

## Schema 38

Vehicle-report strategic events.

- `HST_CampaignState.SCHEMA_VERSION` is `38`.
- `HST_StrategicEventState` now persists vehicle report fields: vehicle runtime
  id, heat before/after/delta, reported before/after, and report-expiry delta.
- Existing schema-37 and older strategic-event rows load with empty vehicle
  fields; new `vehicle_reported` events append rows when runtime vehicle heat is
  reported.

## Schema 37

Strategic event ledger.

- `HST_CampaignState.SCHEMA_VERSION` is `37`.
- `HST_StrategicEventState` records durable mission outcome, convoy outcome,
  zone-capture, and support-near-HQ consequences with event kind,
  source/mission ids, target zone/faction, applied status, summary,
  before/after owner fields, and deltas for money, HR, town support, capture
  progress, aggression, attack/support resources, and HQ knowledge.
- Existing schema-36 and older saves load with an empty strategic-event ledger;
  new mission success/failure outcomes, mission expiry, convoy outcomes,
  resistance zone captures, and hostile support-near-HQ knowledge changes append
  rows when they are applied.

## Schema 36

Active-group vehicle prefab.

- `HST_CampaignState.SCHEMA_VERSION` is `36`.
- `HST_ActiveGroupState` now persists the selected vehicle prefab for mixed
  infantry/vehicle groups.
- Existing schema-35 and older active groups load with an empty vehicle prefab;
  the runtime can select a valid faction vehicle when it next materializes a
  mixed active group that lacks a stored vehicle prefab.

## Schema 35

Support deployment proof.

- `HST_CampaignState.SCHEMA_VERSION` is `35`.
- `HST_SupportRequestState` now persists the deployment route id, placement
  type, placement summary, target/road/HQ distance meters, road resolution,
  vehicle-safe result, and whether vehicle-safe staging was required.
- Physical support requests now copy their placement proof into save data so a
  support request, linked active group, and folded support resolution can be
  audited after save/load.
- Vehicle-capable support requests require vehicle-safe staging. If vehicle-safe
  staging is specifically unavailable, the support deployment downgrades to
  infantry-only staging instead of failing the whole response.

## Schema 34

Population outcome metadata.

- `HST_CampaignState.SCHEMA_VERSION` is `34`.
- Campaign end state now persists the outcome mode, initial population,
  remaining population, killed population, FIA-supporting population, support
  percent, controlled airfields, and total airfields.
- Population victory is the default outcome mode: enough remaining population
  must support the resistance and all airfields must be controlled.
- Population loss is the default loss mode: killed population greater than one
  third of initial population ends the campaign.
- Existing ended saves backfill the new metadata from civilian town state and
  airfield ownership when possible; older control-based ended saves keep
  `legacy_control` as the persisted outcome mode.
- Runtime settings schema `15` adds `populationOutcomeEnabled`,
  `victoryPopulationSupportPercent`, and `legacyControlVictoryEnabled`.

## Schema 33

Active group source links and baseline force counts.

- `HST_CampaignState.SCHEMA_VERSION` is `33`.
- `HST_ActiveGroupState` now persists source identifiers for mission, support
  request, garrison zone, and QRF ownership, plus original infantry and vehicle
  counts.
- Migration backfills missing original counts from current active-group counts
  and links existing groups from support requests, QRF records, active mission
  guard/convoy group ids, or their zone id as a garrison-source fallback.

## Schema 32

Garage vehicle heat and undercover vehicle-cover handoff state.

- `HST_CampaignState.SCHEMA_VERSION` is `32`.
- `HST_GarageVehicleState` now persists reported state, civilian vehicle-cover
  eligibility, heat value, report timestamps, last report reason/zone, and
  passenger compromise count.
- Runtime-to-garage capture and garage-to-runtime redeploy copy vehicle heat
  metadata through the handoff. Existing schema-31 and older garage vehicles
  backfill civilian-cover eligibility from source faction, source kind, and
  civilian vehicle prefab hints, and start with no reported heat unless a newer
  save already carried it.

## Schema 31

Runtime vehicle heat and undercover vehicle-cover state.

- `HST_CampaignState.SCHEMA_VERSION` is `31`.
- `HST_RuntimeVehicleState` now persists whether a runtime vehicle is reported,
  whether it can provide civilian undercover cover, its heat value, report
  timestamps, last report reason/zone, and passenger compromise count.
- Existing schema-30 and older saves backfill civilian-cover eligibility from
  runtime vehicle faction, runtime kind, and civilian vehicle prefab hints.
  Existing runtime vehicles start with no reported heat unless they were already
  marked hot by newer state.

## Schema 30

Town influence events and population aggregates.

- `HST_CampaignState.SCHEMA_VERSION` is `30`.
- `HST_TownInfluenceEventState` persists event id, zone id, kind, source,
  reason, created/expiry seconds, support/reputation/heat/population/security
  deltas, and whether the event was applied.
- `HST_CivilianZoneState` now persists population remaining/killed, influence
  event counts, active/expired modifier counts, and the latest influence event
  metadata.
- Existing schema-29 and older saves backfill town population from civilian
  presence and mark legacy influence summaries from the last incident/security
  fields. Existing saves start with no influence event rows until the next
  town-support event is registered.

## Schema 29

Enemy support-spend ledgers and refunds.

- `HST_CampaignState.SCHEMA_VERSION` is `29`.
- `HST_EnemySupportLedgerState` persists per-faction/per-zone recent damage
  score, last damage time, attack/support spend, last spend time, same-zone
  cooldown, refund totals, and the latest decision reason.
- `HST_EnemyOrderState` now persists attack/support refund amounts and whether
  the resource refund has already been applied, preventing survivor fold-back
  refunds from replaying after reload.
- Existing schema-28 and older saves default ledgers to empty and refund stamps
  to zero/false; no destructive backfill is required.

## Schema 28

Force composition metadata.

- `HST_CampaignState.SCHEMA_VERSION` is `28`.
- `HST_ActiveGroupState`, `HST_SupportRequestState`, and
  `HST_EnemyOrderState` now persist composition request id, intent, selected
  tier, summary text, planned cost, planned manpower, planned vehicle count,
  and planned armed-vehicle count. Support requests and enemy orders also keep
  a composition failure reason.
- The persisted fields are diagnostics and replay hints only. Runtime entity,
  group, vehicle, and waypoint handles remain non-persisted.
- Existing schema-27 and older saves default these fields to empty strings and
  zero counts; no backfill is required.

## Schema 27

Player roster display names.

- `HST_CampaignState.SCHEMA_VERSION` is `27`.
- Runtime settings schema `13` adds `features.infiniteStaminaEnabled`,
  defaulting to `true` for generated and migrated settings. This is not a
  campaign-save migration.
- `HST_PlayerState` now persists `m_sDisplayName`, refreshed from the connected
  player manager when a player registers or is seen during menu/permission
  checks.
- Member and commander UI uses the display name for labels and only shows a
  shortened backend identity as secondary evidence.
- Permission checks, admin grants, loadouts, undercover state, and ownership
  records continue to use backend identity/SteamID64 values, not display names.
- Existing schema-26 and older saves default display names to empty and refresh
  them when the player reconnects.

## Schema 26

HQ spawn-point persistence and rebuild verification.

- `HST_CampaignState.SCHEMA_VERSION` is `26`.
- Campaign state now persists the HQ spawn-point position and prefab alongside
  Petros/cache/arsenal/tent HQ runtime metadata.
- Existing schema-25 and older deployed HQ saves backfill the spawn-point
  position near the HQ and clear the HQ runtime spawned flag so runtime objects
  are rebuilt with the new spawn point.
- Raw spawn-point entity handles remain runtime-only and are recreated by
  `HST_HQService`.

## Schema 25

Phase 24 balance, pacing, and campaign outcomes.

- `HST_CampaignState.SCHEMA_VERSION` is `25`.
- Campaign state now persists campaign-end reason, summary, elapsed second, control percent, war level, FIA/enemy zone counts, and whether an end report was generated.
- Existing schema-24 and older saves that were already WON/LOST backfill campaign-end fields from elapsed time, war level, and zone ownership counts.
- Runtime settings schema `9` adds balance knobs for starting training, war-level thresholds, victory/loss conditions, enemy income scaling, and aggression decay tuning.
- Balance and campaign-end reports are generated from existing zone/resource/mission state; raw world/runtime handles remain non-persistent.

## No Schema Bump: Phase 23 UI And Marker Polish

Phase 23 adds marker audits, command coverage reports, failed-action text, and
menu summary polish on top of existing persisted campaign state. These reports
are rebuilt from campaign fields such as markers, missions, support requests,
enemy orders, HQ/Petros state, civilian/undercover state, and runtime counters,
so no campaign save schema bump was required.

## Schema 24

Phase 22 HQ threat and Defend Petros.

- `HST_CampaignState.SCHEMA_VERSION` is `24`.
- Campaign state now persists HQ threat/knowledge diagnostics and active Defend Petros linkage.
- Defend Petros records persist linked mission/order/support/group ids, status, timing, attacker counts, failure reason, and outcome-applied flag.
- Existing schema-23 saves backfill HQ threat from HQ knowledge and mark existing Defend Petros mission ids as active.
- Raw Petros entities, attacker entities, AI groups, and runtime support entities remain runtime-only and are not persisted.

## Schema 23

Phase 21 undercover enforcement and police/roadblocks.

- `HST_CampaignState.SCHEMA_VERSION` is `23`.
- Undercover player records now persist applied/enforcement state, last compromise reason, detection source, enforcement zone, enforcement timestamps, detection score, and police/roadblock scan counters.
- Civilian town records now persist last police/roadblock scan times and the last security reason.
- Existing schema-22 records are backfilled from last eligibility/status/reason fields.
- Raw player entities, inventory handles, vehicle handles, and runtime civilian/police entities remain runtime-only and are not persisted.

## Schema 22

Phase 20 civilians, town support, and undercover reports.

- `HST_CampaignState.SCHEMA_VERSION` is `22`.
- Civilian town records now persist FIA support, occupier support, last incident reason, and last support-change time.
- Undercover player records now persist request state and the last detailed eligibility report: clothing, weapon/equipment, vehicle, off-road, enemy proximity, and wanted-heat reasons.
- Existing schema-21 civilian and undercover records are backfilled from reputation, support, police/roadblock presence, wanted heat, and last reason.
- Raw player entities, inventory handles, vehicle handles, and runtime civilian entities remain runtime-only and are not persisted.

## Schema 21

Phase 19 support request lifecycle hardening.

- `HST_CampaignState.SCHEMA_VERSION` is `21`.
- Support request records now persist runtime status, resolution kind, physicalization mode, activated time, physicalized time, resolved time, physicalized flag, and outcome-applied flag.
- Existing schema-20 support requests are backfilled from status, group linkage, strike runtime ID, and ETA.
- Raw `IEntity`, `AIGroup`, support component handles, spawned entities, and callback references remain runtime-only and are not persisted.

## Schema 20

Phase 18 enemy commander physical responses.

- `HST_CampaignState.SCHEMA_VERSION` is `20`.
- Enemy order records now persist support/group linkage, runtime status, source/target positions, physicalization time, resolution time, resolution kind, failure reason, and outcome flags.
- Existing schema-19 enemy orders are backfilled from status and support-request linkage.
- Raw `IEntity`, `AIGroup`, support component handles, and spawned entity references remain runtime-only and are not persisted.

## Schema 19

Phase 15 garage and vehicle persistence hardening.

- `HST_CampaignState.SCHEMA_VERSION` is `19`.
- Garage vehicle records and runtime vehicle records now persist source-vehicle
  capability fields: ammo source, repair source, fuel source, and source vehicle kind.
- Existing garage/runtime vehicle records are backfilled from prefab-based capability rules.
- Schema 19+ restores preserve persisted source capability fields; prefab backfill is only
  for pre-schema-19 records or newly captured world vehicles.
- Garage redeploy runtime vehicles are eligible for persistent field-vehicle restore.
- No raw `IEntity`, `AIGroup`, inventory handles, or runtime pointers are persisted.

## Schema 18

Phase 11 mission-specific convoy outcomes.

- `HST_CampaignState.SCHEMA_VERSION` is `18`.
- Active missions now persist convoy outcome-applied flags and a convoy outcome
  summary so mission-specific rewards and penalties do not replay after reload.
- Mission assets now persist an outcome-applied flag and outcome kind so captured
  convoy vehicles, payloads, and captives cannot duplicate asset-level outcomes.
- Existing saves default the new booleans to false and strings to empty during
  normal save-data migration/copy.
- No raw physical entities or runtime handles are persisted.

## Schema 17

Phase 7 convoy waypoint-chain movement.

- `HST_CampaignState.SCHEMA_VERSION` is `17`.
- Active convoy groups now persist `m_iAssignedWaypointCount` as diagnostic
  route-assignment state.
- Schema-16 convoy groups that already recorded `convoy_waypoints` are
  backfilled from their matching generated route waypoint count during
  save-data migration.
- No raw waypoint entities or AI waypoint references are persisted.

## Schema 16

Phase 3 convoy route state.

- `HST_CampaignState.SCHEMA_VERSION` is `16`.
- Generated routes now persist ordered `HST_RouteWaypointState` records.
- Schema-15 routes are backfilled from their legacy start, midpoint, and end
  positions during save-data migration/copy.
- Existing route start/mid/end fields remain for compatibility and are kept in
  sync with waypoint records.
- Route vehicle-safety validation is diagnostic state and can be recomputed from
  generated route waypoints.

## Schema 15

Baseline for the Phase 0/1 stabilization work.

- `HST_CampaignState.SCHEMA_VERSION` remains `15`.
- Persistence smoke-test baselines are stored in the existing persisted `HST_CampaignTaskState` array using `hst_smoke_persistence_expected`.
- Phase 1 zone composition spawn slots are generated at runtime and do not add persisted fields.
- Phase 2 capture diagnostics and notification de-dupe state are runtime-only; capture tuning moved through runtime settings schema `8` without changing campaign save fields.
